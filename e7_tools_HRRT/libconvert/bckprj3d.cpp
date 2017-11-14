/*! \class BckPrj3D bckprj3d.cpp "bckprj3d.h"
    \brief This class provides a method to backproject sinograms into images.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/01/02 image mask is centered in image, independent from COR
    \date 2004/01/30 COR for image and for projections are independent
    \date 2004/03/06 uses memCtrl for memory handling
    \date 2004/03/17 add time-of-flight support
    \date 2004/04/02 use vector code for jacobians
    \date 2004/04/06 added Doxygen style comments
    \date 2004/08/09 use fact that length of norm vector in TOF is constant
    \date 2004/08/10 fix bug in calculation of positon along LOR for TOF
    \date 2004/10/28 add new lookup-table for radial, x/y- and image
                     coordinates and interpolation factors

    The back-projector uses a bilinear interpolation between the four LORs, of
    one view that may hit a voxel. In case of a 2d backprojection this
    simplifies to a linear interpolation between two LORs.

    During the back-projection a circular mask is applied to the image (95% of
    image width). Every part of the image outside the mask will be zero.

    There are different kinds of algorithmic optimizations used in the
    implementation of this class:
     - avoid if-then-else decisions inside of backprojector-loop by using a
       lookup-table (since the table may be very large, this is optional, LUT
       mode 2) and padding of the sinogram
     - use lookup-tables for trigonometric calculations
     - use lookup tables to reduce number of calculations that need to be done
       during the backprojector loop (interpolation factors, image and
       sinogram coordinates)
     - calculate the simpler segment 0 case separately
     - use sinogram in view-mode to increase data locality
     - multi-threading to use all CPUs in a shared memory system
     - multi-processing to use all nodes in a cluster

    The back-projector can use two different sets of lookup-tables to speed up
    the calculation. The selection is done by choosing a LUT mode. In mode 0
    no LUTs are used, in mode 1 and 2 different LUTs are used.
    The tables that are used in mode 1 store interpolation factors and
    coordinates that will be used during backprojection, thus reducing the
    amount of calculations that is done on-the-fly.
    The table that is used in mode 2 stores the first and last voxel of every
    image row, that is hit by a sinogram view. This way we don't need to
    check during backprojection, if a voxel is hit by a view or not. This
    optimization makes only sense, if more than one back-projection is
    calculated (e.g. in OSEM reconstructions with more than one iteration).
    There are two methods used to fill this lookup-table:
    \em searchIndicesBkp_oblique and \em searchIndicesBkp_seg0, for oblique
    segments and segment 0.

    The LUTs in mode 1 and 2 have different memory requirements, depending on
    the image and sinogram size. Also the LUTs for mode 1 can be calculated
    much faster and are even useful if the backprojector is used only once.
    There are also differences in speed that depent on the hardware and the
    number of threads that are used.

    There are ten implementations of the backprojector: with different
    lookup-tables, time-of-flight or non-time-of-flight, oblique segments or
    segment 0:
    - with LUT mode 2:
     - non-time-of-flight:
      - \em back_proj3d_oblique_table2,
      - \em back_proj3d_seg0_table2
     - time-of-flight:
      - \em back_proj3d_oblique_table2_tof,
      - \em back_proj3d_seg0_table2_tof
    - with LUT mode 1:
     - non-time-of-flight:
      - \em back_proj3d_oblique_table1,
      - \em back_proj3d_seg0_table1
    - without LUT (mode 0):
     - non-time-of-flight:
      - \em back_proj3d_oblique
      - \em back_proj3d_seg0
     - time-of-flight:
      - \em back_proj3d_oblique_tof,
      - \em back_proj3d_seg0_tof

    The sinogram used as input to the back-projector needs to be stored in
    view-mode and padded in \f$\rho\f$-direction by 2*tof_bins and in
    z-direction by 2, i.e. the real size of the sinogram is
    \f[
        (\mbox{tof\_bins}+\mbox{RhoSamples}+\mbox{tof\_bins})
        \left(\left(1+\mbox{axis\_slices}[0]+1\right)+
        2\cdot\left(1+\frac{\mbox{axis\_slices}[1]}{2}+1\right)+...\right)
        \mbox{ThetaSamples}
    \f]
    The first and last row and the first and last tof_bins columns of every
    view need to be 0. This way we don't need special cases for the bilinear
    interpolations at the borders of a view.
    The back-projection is calculated in parallel where each thread calculates
    some slices of the resulting image and the number of threads is limited by
    the \em max_threads constant.
*/

#include <string>
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
#include <alloca.h>
#endif
#ifdef WIN32
#include <malloc.h>
#endif
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <unistd.h>
#endif
#ifdef WIN32
#include "global_tmpl.h"
#endif
#include "bckprj3d.h"
#include "const.h"
#include "e7_common.h"
#include "exception.h"
#include "fastmath.h"
#include "gm.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "str_tmpl.h"
#include "thread_wrapper.h"
#include "vecmath.h"

/*- definitions -------------------------------------------------------------*/

// define some abbreviations for the linear and bilinear interpolation code
// This is dirty and not type safe, compared to inline functions but for some
// unknown reason its faster.

                                       // linear interpolation between two bins
#define _linearInterp(f, val1, val2) (val1+(f)*(val2-(val1)))

          // add result of linear interpolation between two bins to image voxel
#define _linearInterpolation(img, f, val1, val2)\
 img+=_linearInterp(f, val1, val2);

                                    // bilinear interpolation between four bins
#define _bilinearInterp(v, f1, f2, val1, val2, val3, val4)\
 { float _a, _b;\
\
   _a=_linearInterp(f1, val1, val2);\
   _b=_linearInterp(f1, val3, val4);\
   v=_linearInterp(f2, _a, _b);\
 }

       // add result of bilinear interpolation between four bins to image voxel
#define _bilinearInterpolation(img, f1, f2, val1, val2, val3, val4)\
 { float _v;\
\
   _bilinearInterp(_v, f1, f2, val1, val2, val3, val4);\
   img+=_v;\
 }

/*- constants ---------------------------------------------------------------*/

                               /*! offset for center of image in voxel units */
const float BckPrj3D::image_cor=0.5f;

/*- local functions ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to calculate backprojection.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to calculate backprojection.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_Backproject(void *param)
 { try
   { BckPrj3D::tthread_params *tp;

     tp=(BckPrj3D::tthread_params *)param;
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     switch (tp->lut_mode)                                    // LUT mode
            { case 0:
               if (tp->segment == 0)                       // segment 0, no LUT
                tp->object->back_proj3d_seg0(tp->image, tp->proj,
                                             tp->viewsize_padded, tp->subset,
                                             tp->z_start, tp->z_end, tp->proc,
                                             tp->num_procs);
                                                     // oblique segment, no LUT
                else tp->object->back_proj3d_oblique(tp->image, tp->proj,
                                                     tp->viewsize_padded,
                                                     tp->subset, tp->segment,
                                                     tp->z_start, tp->z_end,
                                                     tp->proc, tp->num_procs);
               break;
              case 1:
               if (tp->segment == 0)                   // segment 0, mode 1 LUT
                tp->object->back_proj3d_seg0_table1(tp->image, tp->proj,
                                                    tp->viewsize_padded,
                                                    tp->subset, tp->z_start,
                                                    tp->z_end, tp->lut_fact,
                                                    tp->lut_r, tp->lut_xy,
                                                    tp->proc, tp->num_procs);
                                                 // oblique segment, mode 1 LUT
                else tp->object->back_proj3d_oblique_table1(tp->image,
                                                           tp->proj,
                                                           tp->viewsize_padded,
                                                           tp->subset,
                                                           tp->segment,
                                                           tp->z_start,
                                                           tp->z_end,
                                                           tp->lut_fact,
                                                           tp->lut_r,
                                                           tp->lut_xy,
                                                           tp->lut_p,
                                                           tp->proc,
                                                           tp->num_procs);
               break;
              case 2:
               if (tp->segment == 0)                   // segment 0, mode 2 LUT
                tp->object->back_proj3d_seg0_table2(tp->tbp, tp->image,
                                                    tp->proj,
                                                    tp->viewsize_padded,
                                                    tp->subset, tp->z_start,
                                                    tp->z_end, tp->proc,
                                                    tp->num_procs);
                                                 // oblique segment, mode 2 LUT
                else tp->object->back_proj3d_oblique_table2(tp->tbp, tp->image,
                                                           tp->proj,
                                                           tp->viewsize_padded,
                                                           tp->subset,
                                                           tp->segment,
                                                           tp->z_start,
                                                           tp->z_end, tp->proc,
                                                           tp->num_procs);
               break;
            }
     return(NULL);
   }
   catch (const Exception r)
    { return(new Exception(r.errCode(), r.errStr()));
    }
   catch (...)
    { return(new Exception(REC_UNKNOWN_EXCEPTION,
                           "Unknown exception generated."));
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to calculate lookup-table for oblique segments of
           backprojection.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to calculate lookup-table for oblique segments of
    backprojection.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_searchBkp(void *param)
 { try
   { BckPrj3D::tthread_params *tp;

     tp=(BckPrj3D::tthread_params *)param;
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     tp->object->searchIndicesBkp_oblique(tp->tbp, tp->t, tp->segment,
                                          tp->z_start, tp->z_end);
     return(NULL);
   }
   catch (const Exception r)
    { return(new Exception(r.errCode(), r.errStr()));
    }
   catch (...)
    { return(new Exception(REC_UNKNOWN_EXCEPTION,
                           "Unknown exception generated."));
    }
 }

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object and create lookup-tables.
    \param[in] _XYSamples          width/height of image
    \param[in] _DeltaXY            with/height of voxels in mm
    \param[in] _RhoSamples         number of bins in projections
    \param[in] _BinSizeRho         width of bins in projections in mm
    \param[in] _ThetaSamples       number of projections in sinogram plane
    \param[in] _axis_slices        axes table of sinogram
    \param[in] _axes               number of axes in 3d sinogram
    \param[in] _span               span of sinogram
    \param[in] _plane_separation   plane separation in sinogram in mm
    \param[in] reb_factor          sinogram rebinning factor
    \param[in] _subsets            number of subsets for reconstruction
    \param[in] _lut_mode           0=don't use lookup-tables
                                   1=use mode 1 lookup-tables
                                   2=use mode 2 lookup-table
    \param[in] _tof_lut            kernel for time-of-flight backprojection
    \param[in] _loglevel           toplevel for logging
    \param[in] max_threads         maximum number of threads to use for
                                   initialization of lookup-tables
    \exception REC_SPAN_RD span of sinogram is 0

    Initialize object and create lookup-tables. The trigonometric functions
    \f$\sin\theta\f$, \f$\cos\theta\f$, \f$\sin\phi\f$ and \f$\cos\phi\f$ are
    stored in tables:
    \f[
        \mbox{sin\_theta}[t]=\sin\theta=\sin\left(\frac{t\pi}{T}\right)
        \qquad\forall\quad 0\le t<T
    \f]
    \f[
        \mbox{cos\_theta}[t]=\cos\theta=\cos\left(\frac{t\pi}{T}\right)=
        \sqrt{1-\sin^2\theta}
        \qquad\forall\quad 0\le t<T
    \f]
    \f[
        \mbox{sin\_phi}[segment]=\sin\phi=\left\{
        \begin{array}{r@{\quad:\quad}l}
         -\frac{2\left\lfloor\frac{\mbox{segment}+1}{2}\right\rfloor
                \mbox{span}\Delta_z}
               {\sqrt{\left(2\left\lfloor\frac{\mbox{segment}+1}{2}
                      \right\rfloor\mbox{span}\Delta_z\right)^2+
                      \left(2\cdot\mbox{radius}\right)^2}} &
         \mbox{segment odd}\\
         \frac{2\left\lfloor\frac{\mbox{segment}+1}{2}\right\rfloor
               \mbox{span}\Delta_z}
               {\sqrt{\left(2\left\lfloor\frac{\mbox{segment}+1}{2}
                      \right\rfloor\mbox{span}\Delta_z\right)^2+
                      \left(2\cdot\mbox{radius}\right)^2}} &
         \mbox{segment even}
        \end{array}
        \right.\qquad\forall\quad 0\le\mbox{segment}\le 2\cdot\mbox{axes-1}
    \f]
    \f[
        \mbox{cos\_phi}[segment]=\cos\phi=
        \frac{2\cdot\mbox{radius}}
              {\sqrt{\left(2\left\lfloor\frac{\mbox{segment}+1}{2}
                     \right\rfloor\mbox{span}\Delta_z\right)^2+
                     \left(2\cdot\mbox{radius}\right)^2}}
        \qquad\forall\quad 0\le\mbox{segment}\le 2\cdot\mbox{axes-1}
    \f]
    with \f$\mbox{radius}=crystal\_radius+doi\f$.
    The lookup-tables \f$\mbox{z\_bottom}\f$ and \f$\mbox{z\_top}\f$ store the
    number of the first and last plane of each segment, relative to segment 0:
    \f[
        \mbox{z\_bottom}[\mbox{segment}]=
        \frac{\mbox{axis\_slices}[0]-
              \frac{\mbox{axis\_slices}\left[\frac{\mbox{segment}+1}{2}\right]}
                   {2}}
             {2}
        \qquad\forall\quad 0\le\mbox{segment}\le 2\cdot\mbox{axes-1}
    \f]
    \f[
        \mbox{z\_top}[\mbox{segment}]=\mbox{z\_bottom}[\mbox{segment}]+
        \frac{\mbox{axis\_slices}[\frac{\mbox{segment}+1}{2}]}{2}-1
        \qquad\forall\quad 0\le\mbox{segment}\le 2\cdot\mbox{axes-1}
    \f]
    Each plane of the backprojected image is circular. The voxel coordinates of
    the borders of this circle are stored in the \f$\mbox{mask}\f$ structure.

    The constructor also initializes the lookup-tables if LUT mode 1 or 2 is
    used.
 */
/*---------------------------------------------------------------------------*/
BckPrj3D::BckPrj3D(const unsigned short int _XYSamples, const float _DeltaXY,
                   const unsigned short int _RhoSamples,
                   const float _BinSizeRho,
                   const unsigned short int _ThetaSamples,
                   const std::vector <unsigned short int> _axis_slices,
                   const unsigned short int _axes,
                   const unsigned short int _span,
                   const float _plane_separation, const float reb_factor,
                   const unsigned short int _subsets,
                   const unsigned short int _lut_mode,
                   const unsigned short int _loglevel,
                   const unsigned short int max_threads)
 { XYSamples=_XYSamples;
   DeltaXY=_DeltaXY;
   RhoSamples=_RhoSamples;
   BinSizeRho=_BinSizeRho;
   ThetaSamples=_ThetaSamples;
   plane_separation=_plane_separation;
   eff_crystal_radius=GM::gm()->ringRadius()+GM::gm()->DOI();
   loglevel=_loglevel;
   lut_mode=_lut_mode;
   tof_bins=1;
                    // center of projections
                    // (depends on rebinning of sinogram before reconstruction)
   rho_center=(float)(RhoSamples/2)-(0.5f-0.5f/reb_factor);
   image_center=(float)(XYSamples/2)-0.5f+image_cor;         // center of image

   subsets=_subsets;
   span=_span;
   if (span == 0) throw Exception(REC_SPAN_RD, "Span is 0.");
   Logging::flog()->logMsg("initialize backprojector", loglevel);
                                                     // calculate segment table
   axes=_axes;
   axis_slices=_axis_slices;
   segments=2*axes-1;
   z_bottom.resize(segments);
   z_top.resize(segments);
                                          // first and last plane of segments 0
   z_bottom[0]=0;
   z_top[0]=axis_slices[0]-1;
   sin_phi.resize(segments);
   cos_phi.resize(segments);
                                 // sine and cosine of axial angle of segment 0
   sin_phi[0]=0.0f;
   cos_phi[0]=1.0f;
   for (unsigned short int axis=1; axis < axes; axis++)
    { float tmp_flt;
                     // first and last planes of segments relative to segment 0
      z_bottom[axis*2]=(axis_slices[0]-axis_slices[axis]/2)/2;
      z_top[axis*2]=z_bottom[axis*2]+axis_slices[axis]/2-1;
      z_bottom[axis*2-1]=z_bottom[axis*2];
      z_top[axis*2-1]=z_top[axis*2];
                                   // sine and cosine of axial angle of segment
      tmp_flt=(float)axis*(float)span*plane_separation*2.0f;
      tmp_flt=sqrtf(tmp_flt*tmp_flt+
                    2.0f*eff_crystal_radius*2.0f*eff_crystal_radius);
      sin_phi[axis*2-1]=-(float)axis*(float)span*plane_separation*2.0f/tmp_flt;
      cos_phi[axis*2-1]=2.0f*eff_crystal_radius/tmp_flt;
      sin_phi[axis*2]=-sin_phi[axis*2-1];
      cos_phi[axis*2]=cos_phi[axis*2-1];
    }
   for (unsigned short int segment=0; segment < segments; segment++)
    Logging::flog()->logMsg("angle of segment #1: #2 degrees", loglevel+1)->
     arg(segStr(segment))->arg(asinf(sin_phi[segment])*180.0f/M_PI);
                                          // sine and cosine of azimuthal angle
   sin_theta.resize(ThetaSamples);
   cos_theta.resize(ThetaSamples);
   sin_theta[0]=0.0f;
   cos_theta[0]=1.0f;
   sin_theta[ThetaSamples/2]=1.0f;
   cos_theta[ThetaSamples/2]=0.0f;
   for (unsigned short int t=1; t < ThetaSamples/2; t++)
    { sin_theta[t]=sinf((float)t*M_PI/(float)ThetaSamples);
      cos_theta[t]=sqrtf(1.0f-sin_theta[t]*sin_theta[t]);
      sin_theta[ThetaSamples-t]=sin_theta[t];
      cos_theta[ThetaSamples-t]=-cos_theta[t];
    }
                                                         // size of image plane
   image_plane_size=(unsigned long int)XYSamples*
                    (unsigned long int)XYSamples;
   RhoSamples_padded=(RhoSamples+2)*tof_bins;       // width of padded sinogram
                                                         // size of padded view
   view_size_padded=(unsigned long int)(axis_slices[0]+2)*
                    (unsigned long int)RhoSamples_padded;
   initMask(0.95f*DeltaXY*(float)(XYSamples/2));       // initialize image mask
                                 // initialize lookup-tables for backprojection
   lut_fact=MemCtrl::MAX_BLOCK;
   lut_r=MemCtrl::MAX_BLOCK;
   lut_xy=MemCtrl::MAX_BLOCK;
   lut_p=MemCtrl::MAX_BLOCK;
   if (lut_mode == 1) initTable1();
    else if (lut_mode == 2) initTable2(max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object and release used memory.

    Destroy the object and release the used memory.
 */
/*---------------------------------------------------------------------------*/
BckPrj3D::~BckPrj3D()
 { for (unsigned short int i=0; i < table_bkp.size(); i++)
    MemCtrl::mc()->deleteBlockForce(&table_bkp[i]);
   MemCtrl::mc()->deleteBlockForce(&lut_r);
   MemCtrl::mc()->deleteBlockForce(&lut_fact);
   MemCtrl::mc()->deleteBlockForce(&lut_xy);
   MemCtrl::mc()->deleteBlockForce(&lut_p);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Backproject one subset of one segment.
    \param[in]     sino          padded sinogram data in view mode
    \param[in,out] img           image data
    \param[in]     subset        number of subset
    \param[in]     segment       number of segment
    \param[in]     sum_bins      sum up time-of-flight bins
    \param[in]     max_threads   maximum number of parallel threads to use
    \param[in]     proc          number of node
    \param[in]     num_procs     number of nodes in distributed memory system

    Backproject one subset of one segment. The sinogram is multiplied by
    \f[
        \frac{\cos^3\phi\cdot\mbox{span}\Delta_z}{radius}
    \f]
    with \f$radius=crystal\_radius+doi\f$ before backprojection.

    If the \em sum_bins parameter is true, a time-of-flight backprojection is
    calculated although the sinogram contains only one time-of-flight bin. In
    this case its assumed that the missing TOF bins have the same number of
    counts. This functionality is used to backproject the normalization matrix
    (1/1, 1/A, 1/N or 1/(AW)) for the TOF-OSEM reconstruction.

    This method is multi-threaded. Every thread processes a range of image
    slices.

    The method can also be used in a multi-processing (distributed memory)
    situation. In this case the calculation is distributed on several nodes and
    the node number and the overall number of nodes must be specified. The
    calculation on each node is multi-threaded to utilize all CPUs of a node.
    The caller is responsible for distributing the sinogram data to the nodes
    and collecting and summing the resulting images.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::back_proj3d(float *sino, float * const img,
                           const unsigned short int subset,
                           const unsigned short int segment,
                           const bool sum_bins,
                           const unsigned short int max_threads,
                           const unsigned short int proc,
                           const unsigned short int num_procs)
 { try
   { unsigned short int threads=0, t;
     void *thread_result;
     std::vector <bool> thread_running(0);
     std::vector <tthread_id> tid;

     try
     { std::vector <tthread_params> tp;
       signed short int *lr=NULL;
       float *lf=NULL, *lp=NULL;
       uint32 *lxy=NULL;
       tmaskline *tbp=NULL;
       unsigned long int vs_padded;
                                                      // calculate size of view
       if (sum_bins)
        { unsigned short int rs_padded;

          rs_padded=RhoSamples+2;
          vs_padded=(unsigned long int)(axis_slices[0]+2)*
                    (unsigned long int)rs_padded;
        }
        else vs_padded=view_size_padded;
            // calculate Jacobian to replace linear integral by x or y integral
                                          //  apply jacobian to sinogram
       vecMulScalar(sino, cos_phi[segment]*cos_phi[segment]*cos_phi[segment]*
                          span*plane_separation/eff_crystal_radius, sino,
                    vs_padded*(unsigned long int)(ThetaSamples/subsets));
                                                    // number of threads to use
       threads=std::min(max_threads, axis_slices[0]);
       if (threads > 1)
        { tid.resize(threads);                          // table for thread-IDs
          tp.resize(threads);                    // table for thread parameters
          thread_running.assign(threads, false);     // table for thread status
        }
                                           // get lookup-table for this segment
       if (table_bkp.size() > 0)
        tbp=(tmaskline *)MemCtrl::mc()->getSIntRO(table_bkp[segment],
                                                  loglevel);
       if ((tof_bins == 1) && (lut_mode == 1))
        { lf=MemCtrl::mc()->getFloatRO(lut_fact, loglevel);
          lr=MemCtrl::mc()->getSIntRO(lut_r, loglevel);
          lxy=MemCtrl::mc()->getUIntRO(lut_xy, loglevel);
          if (axes > 1) lp=MemCtrl::mc()->getFloatRO(lut_p, loglevel);
        }
       if (threads == 1)                      // single-threaded backprojection
         switch (lut_mode)                                     // LUT mode
               { case 0:
                  if (segment == 0)                                // segment 0
                   back_proj3d_seg0(img, sino, vs_padded, subset, 0,
                                    axis_slices[0], proc, num_procs);
                                                             // oblique segment
                   else back_proj3d_oblique(img, sino, vs_padded, subset,
                                            segment, 0, axis_slices[0], proc,
                                            num_procs);
                  break;
                 case 1:
                  if (segment == 0)                                // segment 0
                   back_proj3d_seg0_table1(img, sino, vs_padded, subset, 0,
                                           axis_slices[0], lf, lr, lxy, proc,
                                           num_procs);
                                                             // oblique segment
                   else back_proj3d_oblique_table1(img, sino, vs_padded,
                                                   subset, segment, 0,
                                                   axis_slices[0], lf, lr, lxy,
                                                   lp, proc, num_procs);
                  break;
                 case 2:
                  if (segment == 0)                                // segment 0
                   back_proj3d_seg0_table2(tbp, img, sino, vs_padded, subset,
                                           0, axis_slices[0], proc, num_procs);
                                                             // oblique segment
                   else back_proj3d_oblique_table2(tbp, img, sino, vs_padded,
                                                   subset, segment, 0,
                                                   axis_slices[0], proc,
                                                   num_procs);
                  break;
               }
        else { unsigned short int slices, z_start, i;
                                                     // parallel backprojection
               for (slices=axis_slices[0],
                    z_start=0,
                    t=threads,
                    i=0; i < threads; i++,
                    t--)
                {                                // calculate thread parameters
                  tp[i].object=this;
                  tp[i].image=img+(unsigned long int)z_start*image_plane_size;
                  tp[i].proj=sino;
                  tp[i].viewsize_padded=vs_padded;
                  tp[i].subset=subset;
                  tp[i].tbp=tbp;
                  tp[i].segment=segment;
                  tp[i].z_start=z_start;                // first slice of image
                  tp[i].z_end=z_start+slices/t;       // last slice +1 of image
                  tp[i].lut_mode=lut_mode;
                  tp[i].lut_fact=lf;
                  tp[i].lut_r=lr;
                  tp[i].lut_xy=lxy;
                  tp[i].lut_p=lp;
                  tp[i].sum_bins=sum_bins;
                  tp[i].tof_bins=tof_bins;
                  tp[i].proc=proc;
                  tp[i].num_procs=num_procs;
#ifdef XEON_HYPERTHREADING_BUG
                  tp[i].thread_number=i;
#endif
                  thread_running[i]=true;
                                                                // start thread
                  ThreadCreate(&tid[i], &executeThread_Backproject, &tp[i]);
                  slices-=(tp[i].z_end-tp[i].z_start);
                  z_start=tp[i].z_end;
                }
                                         // wait until all threads are finished
               for (i=0; i < threads; i++)
                { ThreadJoin(tid[i], &thread_result);
                  thread_running[i]=false;
                  if (thread_result != NULL) throw (Exception *)thread_result;
                }
             }
       if ((tof_bins == 1) || (lut_mode == 1))
        { MemCtrl::mc()->put(lut_r);
          MemCtrl::mc()->put(lut_fact);
          MemCtrl::mc()->put(lut_xy);
          if (axes > 1) MemCtrl::mc()->put(lut_p);
        }
                                    // return lookup-table to memory controller
       if (table_bkp.size() > 0)
        MemCtrl::mc()->put(table_bkp[segment]);
     }
     catch (...)                                // handle exceptions of threads
      { thread_result=NULL;
                                   // wait until all other threads are finished
        for (t=0; t < thread_running.size(); t++)
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
        if ((table_bkp.size() > 0) && (lut_mode == 2))
         MemCtrl::mc()->put(table_bkp[segment]);
        if ((tof_bins == 1) || (lut_mode == 1))
         { MemCtrl::mc()->put(lut_r);
           MemCtrl::mc()->put(lut_fact);
           MemCtrl::mc()->put(lut_xy);
           if (axes > 1) MemCtrl::mc()->put(lut_p);
         }
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)              //  handle exceptions of this method
    { std::string err_str;
      unsigned long int err_code;

      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the backprojection of one subset of segment 0 without
           LUTs.
    \param[in,out] img               image data
    \param[in]     sino              padded sinogram in view mode
    \param[in]     viewsize_padded   size of padded view
    \param[in]     subset            number of subset
    \param[in]     z_start           first slice of image to calculate
    \param[in]     z_end             last slice+1 of image to calculate
    \param[in]     proc              number of node
    \param[in]     num_procs         number of nodes in distributed memory
                                     system

    Calculate the backprojection of segment 0 into the image without LUTs for
    all azimuthal angles of one subset. For each voxel (x,y,z) the coordinate
    (r) in the sinogram projection is calculated, where the LOR that intersects
    the voxel hits the view (t,z,0) in a \f$90^{o}\f$ angle:
    \f[
        r=\rho_c+\left(\left(y-i_c\right)\sin\theta+
                       \left(x-i_c\right)\cos\theta\right)
          \frac{\Delta_{xy}}{\Delta_p}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    the center of a projection at:
    \f[
       \rho_c=\frac{RhoSamples}{2}-
              \left(0.5-\frac{0.5}{\mbox{reb\_factor}}\right)
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$ and the sinogram bin size
    \f$\Delta_\rho\f$.

    The sinogram value at the coordinate (r,t,z,0) is calculated by
    linear interpolation:
    \f[
        r_1=r-\lfloor r\rfloor \qquad r_2=1-r_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{sino}(r,t,z,0) & = &
         r_2\mbox{sino}(\lfloor r\rfloor,t,z,0)+\\
         && r_1\mbox{sino}(\lceil r\rceil,t,z,0)\\
         &=& \mbox{sino}(\lfloor r\rfloor,t,z,0)+(r-\lfloor r\rfloor)
             (\mbox{sino}(\lceil r\rceil,t,z,0)-
              \mbox{sino}(\lfloor r\rfloor,t,z,0))
        \end{array}
    \f]
    and added to the voxel (x,y,z).

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image. If running on a distributed memory system with
    n nodes, only every n\em th plane of the image is processed.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::back_proj3d_seg0(float * const img, float *sino,
                                const unsigned long int viewsize_padded,
                                const unsigned short int subset,
                                const unsigned short int z_start,
                                const unsigned short int z_end,
                                const unsigned short int proc,
                                const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=viewsize_padded)
    { float del_r_x, del_r_y, r000, *p2dp, *ip;
                                   // calculate increments to follow view plane
      { float tmp;

        tmp=DeltaXY/BinSizeRho;
        del_r_x=cos_theta[t]*tmp;
        del_r_y=sin_theta[t]*tmp;
      }
                                               // calculate start point of view
      r000=rho_center-image_center*(del_r_x+del_r_y);
      p2dp=sino+(unsigned long int)(z_start+1+proc)*
                (unsigned long int)RhoSamples_padded;
                                    // follow view through volume and calculate
                                    // weights by linear interpolation
      ip=img+proc*image_plane_size;
      for (unsigned short int z=z_start+proc; z < z_end; z+=num_procs,
           p2dp+=RhoSamples_padded*num_procs,
           ip+=(unsigned long int)(num_procs-1)*image_plane_size)
       { float r00;
         unsigned short int y;

         for (r00=r000,
              y=0; y < XYSamples; y++,
              r00+=del_r_y,
              ip+=XYSamples)
          { float r;
            unsigned short int x;

            for (r=r00+(float)mask[y].start*del_r_x,
                        // exclude voxels in line, that are not hit by the view
                 x=mask[y].start; x <= mask[y].end; x++,
                 r+=del_r_x)
             if ((r >= -1.0f) && (r < (float)RhoSamples))
              { signed short int rfloor;

                rfloor=(signed short int)(r+1.0f)-1;  // number of bin in plane
                                // the two LORs that hit the voxel image[x,y,z]
                                // are view[z,r] and  view[z,r+1]
                _linearInterpolation(ip[x], r-(float)rfloor, p2dp[rfloor+1],
                                     p2dp[rfloor+2]);
              }
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the backprojection of one subset of segment 0 with mode 1
           LUTs.
    \param[in,out] img               image data
    \param[in]     sino              padded sinogram in view mode
    \param[in]     viewsize_padded   size of padded view
    \param[in]     subset            number of subset
    \param[in]     z_start           first slice of image to calculate
    \param[in]     z_end             last slice+1 of image to calculate
    \param[in]     lf                LUT for radial interpolation factors
    \param[in]     lr                LUT for radial coordinates
    \param[in]     lxy               LUT for x/y-coordinates of image
    \param[in]     proc              number of node
    \param[in]     num_procs         number of nodes in distributed memory
                                     system

    Calculate the backprojection of segment 0 into the image for all azimuthal
    angles of one subset..
    similar to the BckPrj3D::back_proj3d_view_seg0 method, but use a LUT for
    the radial coordinates \f$\lfloor r\rfloor+1\f$ and interpolation factors
    \f$r-\lfloor r\rfloor\f$.

    The sinogram value at the coordinate (r,t,z,0) is calculated by
    linear interpolation:
    \f[
        f=r-\lfloor r\rfloor
    \f]
    \f[
        c=(\lfloor r\rfloor)
    \f]
    \f[
        \mbox{sino}(r,t,z,0)=
         (1-f)\cdot\mbox{sino}(c,t,z,0)+f\cdot\mbox{sino}(c+1,t,z,0)
    \f]
    and added to the voxel (x,y,z).The interpolation factors \f$f\f$ are stored
    in the \em lf LUT. The \f$c\f$ coordinate of the sinogram is stored in the
    \em lr LUT and the x/y-coordinate of the image is stored in the \em lxy
    LUT.

    If running on a distributed memory system with n nodes, only every n\em th
    plane of the image is processed.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::back_proj3d_seg0_table1(float * const img, float *sino,
                                       const unsigned long int viewsize_padded,
                                       const unsigned short int subset,
                                       const unsigned short int z_start,
                                       const unsigned short int z_end,
                                       const float * const lf,
                                       const signed short int * const lr,
                                       const uint32 * const lxy,
                                       const unsigned short int proc,
                                       const unsigned short int num_procs)
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=viewsize_padded)
    { float *p2dp, *ip;

      p2dp=sino+(unsigned long int)(z_start+1+proc)*
                (unsigned long int)RhoSamples_padded;
                                    // follow view through volume and calculate
                                    // weights by linear interpolation
      ip=img+proc*image_plane_size;
      for (unsigned short int z=z_start+proc; z < z_end; z+=num_procs,
           p2dp+=RhoSamples_padded*num_procs,
           ip+=image_plane_size*(unsigned long int)num_procs)
       for (unsigned long int xy=lut_pos[t]; xy < lut_pos[t+1]; xy++)
        { float *ptr;

          ptr=&p2dp[lr[xy]];
          _linearInterpolation(ip[lxy[xy]], lf[xy], ptr[0], ptr[1]);
        }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the backprojection of one subset of segment 0 with mode 2
           LUT.
    \param[in]     tbp               pointer to lookup-table
    \param[in,out] img               image data
    \param[in]     sino              padded sinogram in view mode
    \param[in]     viewsize_padded   size of padded view
    \param[in]     subset            number of projection in sinogram plane
    \param[in]     z_start           first slice of image to calculate
    \param[in]     z_end             last slice+1 of image to calculate
    \param[in]     proc              number of node
    \param[in]     num_procs         number of nodes in distributed memory
                                     system

    Calculate the backprojection of segment 0 into the image for all azimuthal
    angles of on subset. For each voxel (x,y,z) the coordinate (r) in the
    sinogram projection is calculated, where the LOR that intersects the voxel
    hits the view (t,z,0) in a \f$90^{o}\f$ angle:
    \f[
        r=\rho_c+\left(\left(y-i_c\right)\sin\theta+
                       \left(x-i_c\right)\cos\theta\right)
          \frac{\Delta_{xy}}{\Delta_p}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    the center of a projection at:
    \f[
       \rho_c=\frac{RhoSamples}{2}-
              \left(0.5-\frac{0.5}{\mbox{reb\_factor}}\right)
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$ and the sinogram bin size
    \f$\Delta_\rho\f$.

    The sinogram value at the coordinate (r,t,z,0) is calculated by
    linear interpolation:
    \f[
        r_1=r-\lfloor r\rfloor \qquad r_2=1-r_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{sino}(r,t,z,0) & = &
         r_2\mbox{sino}(\lfloor r\rfloor,t,z,0)+\\
         && r_1\mbox{sino}(\lceil r\rceil,t,z,0)
        \end{array}
    \f]
    and added to the voxel (x,y,z).

    The code uses a lookup-table to skip image voxels that are not hit by the
    view. If running on a distributed memory system with n nodes, only every
    n\em th plane of the image is processed.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::back_proj3d_seg0_table2(tmaskline * const tbp,
                                      float * const img, float *sino,
                                      const unsigned long int viewsize_padded,
                                      const unsigned short int subset,
                                      const unsigned short int z_start,
                                      const unsigned short int z_end,
                                      const unsigned short int proc,
                                      const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=viewsize_padded)
    { float del_r_x, del_r_y, r000, *p2dp, *ip;
      unsigned long int idx;
                                   // calculate increments to follow view plane
      { float tmp;

        tmp=DeltaXY/BinSizeRho;
        del_r_x=cos_theta[t]*tmp;
        del_r_y=sin_theta[t]*tmp;
      }
                                               // calculate start point of view
      r000=rho_center-image_center*(del_r_x+del_r_y);
                              // pointer into lookup-table with line-masks that
                              // excludes voxels which are not hit by the view
      idx=(unsigned long int)t*(unsigned long int)XYSamples;
      p2dp=sino+(unsigned long int)(z_start+1+proc)*
                (unsigned long int)RhoSamples_padded;
                                    // follow view through volume and calculate
                                    // weights by linear interpolation
      ip=img+proc*image_plane_size;
      for (unsigned short int z=z_start+proc; z < z_end; z+=num_procs,
           p2dp+=RhoSamples_padded*num_procs,
           ip+=(unsigned long int)(num_procs-1)*image_plane_size)
       { float r00;
         unsigned short int y;

         for (r00=r000,
              y=0; y < XYSamples; y++,
              r00+=del_r_y,
              ip+=XYSamples)
          { float r;
            unsigned short int x;

            for (r=r00+(float)tbp[idx+y].start*del_r_x,
                        // exclude voxels in line, that are not hit by the view
                 x=tbp[idx+y].start; x <= tbp[idx+y].end; x++,
                 r+=del_r_x)
             { signed short int rfloor;

               rfloor=(signed short int)(r+1.0f)-1;   // number of bin in plane
                            // the two LORs that hit the voxel image[x,y,z] are
                            // view[z,r], view[z,r+1]
               _linearInterpolation(ip[x], r-(float)rfloor, p2dp[rfloor+1],
                                    p2dp[rfloor+2]);
             }
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the backprojection of one subset of an oblique segment.
    \param[in,out] img               image data
    \param[in]     sino              padded sinogram in view mode
    \param[in]     viewsize_padded   size of padded view
    \param[in]     subset            number of subset
    \param[in]     segment           number of segment
    \param[in]     z_start           first slice of image to calculate
    \param[in]     z_end             last slice+1 of image to calculate
    \param[in]     proc              number of node
    \param[in]     num_procs         number of nodes in distributed memory
                                     system

    Calculate the backprojection of one susbet of an oblique segment into the
    image. For each voxel (x,y,z) the coordinate (r,p) in the sinogram view is
    calculated, where the LOR that intersects the voxel hits the view
    (t,segment) in a \f$90^{o}\f$ angle:
    \f[
        r=\rho_c+\left(\left(y-i_c\right)\sin\theta+
                       \left(x-i_c\right)\cos\theta\right)
          \frac{\Delta_{xy}}{\Delta_p}
    \f]
    \f[
        p=z+\left(-\left(y-i_c\right)\cos\theta+
                  \left(x-i_c\right)\sin\theta\right)
            \frac{\sin\phi\Delta_{xy}}{\cos\phi\Delta_z}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    the center of a projection at:
    \f[
       \rho_c=\frac{RhoSamples}{2}-
              \left(0.5-\frac{0.5}{\mbox{reb\_factor}}\right)
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$, the voxel depth
    \f$\Delta_z\f$ and the sinogram bin size \f$\Delta_\rho\f$.

    The sinogram value at the coordinate (r,t,p,segment) is calculated by
    bilinear interpolation:
    \f[
        r_1=r-\lfloor r\rfloor \qquad p_1=p-\lfloor p\rfloor \qquad
        r_2=1-r_1 \qquad p_2=1-p_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{sino}(r,t,p,segment) & = &
         p_2r_2\mbox{sino}(\lfloor r\rfloor,t,\lfloor p\rfloor,segment)+\\
         && p_2r_1\mbox{sino}(\lceil r\rceil,t,\lfloor p\rfloor,segment)+\\
         && p_1r_2\mbox{sino}(\lfloor r\rfloor,t,\lceil p\rceil,segment)+\\
         && p_1r_1\mbox{sino}(\lceil r\rceil,t,\lceil p\rceil,segment)
        \end{array}
    \f]
    and added to the voxel (x,y,z).

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image. If running on a distributed memory system with
    n nodes, only every n\em th plane of the image is processed.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::back_proj3d_oblique(float * const img, float *sino,
                                   const unsigned long int viewsize_padded,
                                   const unsigned short int subset,
                                   const unsigned short int segment,
                                   const unsigned short int z_start,
                                   const unsigned short int z_end,
                                   const unsigned short int proc,
                                   const unsigned short int num_procs) const
 { float zb, zt;

   zb=(float)(z_bottom[segment]-1);
   zt=(float)(z_top[segment]+1);
   for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=viewsize_padded)
    { float del_r_x, del_r_y, del_z_x, del_z_y, r000, p000, *ip;
                                   // calculate increments to follow view plane
      { float tmp;

        tmp=DeltaXY/BinSizeRho;
        del_r_x=cos_theta[t]*tmp;
        del_r_y=sin_theta[t]*tmp;
        tmp=sin_phi[segment]*DeltaXY/(cos_phi[segment]*plane_separation);
        del_z_x=sin_theta[t]*tmp;
        del_z_y=-cos_theta[t]*tmp;
     }
                                               // calculate start point of view
      r000=rho_center-image_center*(del_r_x+del_r_y);
      p000=-image_center*(del_z_x+del_z_y);
                                    // follow view through volume and calculate
                                    // weights by bilinear interpolation
      ip=img+proc*image_plane_size;
      for (unsigned short int z=z_start+proc; z < z_end; z+=num_procs,
           ip+=(num_procs-1)*image_plane_size)
       { float r00, p00;
         unsigned short int y;

         for (p00=p000+(float)z,
              r00=r000,
              y=0; y < XYSamples; y++,
              r00+=del_r_y,
              p00+=del_z_y,
              ip+=XYSamples)
          { float r, p;
            unsigned short int x;

            for (r=r00+(float)mask[y].start*del_r_x,
                 p=p00+(float)mask[y].start*del_z_x,
                        // exclude voxels in line, that are not hit by the view
                 x=mask[y].start; x <= mask[y].end; x++,
                 r+=del_r_x,
                 p+=del_z_x)
             if ((p >= zb) && (p < zt))
              if ((r >= -1.0f) && (r < (float)RhoSamples))
               { signed short int rfloor, pfloor;
                 float *p2dp;

                 pfloor=(signed short int)(p+1.0f)-1;        // number of plane
                 rfloor=(signed short int)(r+1.0f)-1; // number of bin in plane
                       // the four LORs that hit the voxel image[x,y,z] are
                       // view[p,r], view[p+1,r], view[p,r+1] and view[p+1,r+1]
                 p2dp=sino+(unsigned long int)(pfloor+1)*
                           (unsigned long int)RhoSamples_padded+rfloor+1;
                 _bilinearInterpolation(ip[x], r-(float)rfloor,
                                        p-(float)pfloor, p2dp[0], p2dp[1],
                                        p2dp[RhoSamples_padded],
                                        p2dp[RhoSamples_padded+1]);
               }
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the backprojection of one subset of an oblique segment
           with mode 1 LUTs.
    \param[in,out] img               image data
    \param[in]     sino              padded sinogram in view mode
    \param[in]     viewsize_padded   size of padded view
    \param[in]     subset            number of subset
    \param[in]     segment           number of segment
    \param[in]     z_start           first slice of image to calculate
    \param[in]     z_end             last slice+1 of image to calculate
    \param[in]     lf                LUT for radial interpolation factors
    \param[in]     lr                LUT for radial coordinates
    \param[in]     lxy               LUT for x/y-coordinates of image
    \param[in]     lp                LUT for sinogram coordinate
    \param[in]     proc              number of node
    \param[in]     num_procs         number of nodes in distributed memory
                                     system

    Calculate the backprojection of one subset of an oblique segment into the
    image. For each voxel (x,y,z) the coordinate (r,p) in the sinogram view is
    calculated, where the LOR that intersects the voxel hits the view
    (t,segment) in a \f$90^{o}\f$ angle:
    \f[
        p=-i_c(\sin\theta-\cos\theta)
          \frac{\sin\phi\Delta_{xy}}{\cos\phi\Delta_z}+
          z+\frac{\sin\phi}{\cos\phi}p_{xy}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$ and the voxel depth
    \f$\Delta_z\f$.

    The sinogram value at the coordinate (r,t,p,segment) is calculated by
    bilinear interpolation:
    \f[
        f=r-\lfloor r\rfloor
    \f]
    \f[
        c=(\lfloor r\rfloor)
    \f]
    \f[
        p_1=p-\lfloor p\rfloor \qquad p_2=1-p_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{sino}(r,t,p,segment) & = &
         p_2(1-f)\cdot\mbox{sino}(c,t,\lfloor p\rfloor,segment)+
         p_2f\cdot\mbox{sino}(c+1,t,\lfloor p\rfloor,segment)+\\
         && p_1(1-f)\cdot\mbox{sino}(c,t,\lceil p\rceil,segment)+
            p_1f\cdot\mbox{sino}(c+1,t,\lceil p\rceil,segment)
        \end{array}
    \f]
    and added to the voxel (x,y,z).
    The offset \f$p_{xy}\f$ is stored in the \em lp LUT. The interpolation
    factors \f$f\f$ are stored in the \em lf LUT and the sinogram coordinate
    \f$c\f$ is stored in the \em lr LUT. The x/y-coordinate of the image is
    stored in the \em lxy LUT.

    If running on a distributed memory system with n nodes, only every n\em th
    plane of the image is processed.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::back_proj3d_oblique_table1(float * const img, float *sino,
                                       const unsigned long int viewsize_padded,
                                       const unsigned short int subset,
                                       const unsigned short int segment,
                                       const unsigned short int z_start,
                                       const unsigned short int z_end,
                                       const float * const lf,
                                       const signed short int * const lr,
                                       const uint32 * const lxy,
                                       const float * const lp,
                                       const unsigned short int proc,
                                       const unsigned short int num_procs)
 { float zb, zt;
   std::vector <float> p00;

   zb=(float)(z_bottom[segment]-1);
   zt=(float)(z_top[segment]+1);
   for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=viewsize_padded)
    { float p000, *ip, tan_phi;

      tan_phi=sin_phi[segment]/cos_phi[segment];
                                               // calculate start point of view
      p000=-image_center*(sin_theta[t]-cos_theta[t])*tan_phi*
            DeltaXY/plane_separation;
      ip=img+proc*image_plane_size;

      p00.resize(lut_pos[t+1]-lut_pos[t]);
      for (unsigned long int xy=lut_pos[t]; xy < lut_pos[t+1]; xy++)
       p00[xy-lut_pos[t]]=p000+tan_phi*lp[xy];
      for (unsigned short int z=z_start+proc; z < z_end; z+=num_procs,
           ip+=num_procs*image_plane_size)
       for (unsigned long int xy=lut_pos[t]; xy < lut_pos[t+1]; xy++)
        { float p;

          p=(float)z+p00[xy-lut_pos[t]];
          if ((p >= zb) && (p < zt))
           { signed short int pfloor;
             float *p2dp;

             pfloor=(signed short int)(p+1.0f)-1;            // number of plane
                       // the four LORs that hit the voxel image[x,y,z] are
                       // view[p,r], view[p+1,r], view[p,r+1] and view[p+1,r+1]
                                // calculate weights for bilinear interpolation
             p2dp=&sino[(unsigned long int)(pfloor+1)*
                        (unsigned long int)RhoSamples_padded+lr[xy]];
             _bilinearInterpolation(ip[lxy[xy]], lf[xy], p-(float)pfloor,
                                    p2dp[0], p2dp[1], p2dp[RhoSamples_padded],
                                    p2dp[RhoSamples_padded+1]);
           }
        }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the backprojection of one subset of an oblique segment
           with mode 2 LUTs.
    \param[in]     tbp               pointer to lookup-table
    \param[in,out] img               image data
    \param[in]     sino              padded sinogram in view mode
    \param[in]     viewsize_padded   size of padded view
    \param[in]     subset            number of subset
    \param[in]     segment           number of segment
    \param[in]     z_start           first slice of image to calculate
    \param[in]     z_end             last slice+1 of image to calculate
    \param[in]     proc              number of node
    \param[in]     num_procs         number of nodes in distributed memory
                                     system

    Calculate the backprojection of one subset of an oblique segment into the
    image. For each voxel (x,y,z) the coordinate (r,p) in the sinogram view is
    calculated, where the LOR that intersects the voxel hits the view
    (t,segment) in a \f$90^{o}\f$ angle:
    \f[
        r=\rho_c+\left(\left(y-i_c\right)\sin\theta+
                       \left(x-i_c\right)\cos\theta\right)
          \frac{\Delta_{xy}}{\Delta_p}
    \f]
    \f[
        p=z+\left(-\left(y-i_c\right)\cos\theta+
                  \left(x-i_c\right)\sin\theta\right)
            \frac{\sin\phi\Delta_{xy}}{\cos\phi\Delta_z}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    the center of a projection at:
    \f[
       \rho_c=\frac{RhoSamples}{2}-
              \left(0.5-\frac{0.5}{\mbox{reb\_factor}}\right)
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$, the voxel depth
    \f$\Delta_z\f$ and the sinogram bin size \f$\Delta_\rho\f$.

    The sinogram value at the coordinate (r,t,p,segment) is calculated by
    bilinear interpolation:
    \f[
        r_1=r-\lfloor r\rfloor \qquad p_1=p-\lfloor p\rfloor \qquad
        r_2=1-r_1 \qquad p_2=1-p_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{sino}(r,t,p,segment) & = &
         p_2r_2\mbox{sino}(\lfloor r\rfloor,t,\lfloor p\rfloor,segment)+\\
         && p_2r_1\mbox{sino}(\lceil r\rceil,t,\lfloor p\rfloor,segment)+\\
         && p_1r_2\mbox{sino}(\lfloor r\rfloor,t,\lceil p\rceil,segment)+\\
         && p_1r_1\mbox{sino}(\lceil r\rceil,t,\lceil p\rceil,segment)
        \end{array}
    \f]
    and added to the voxel (x,y,z).

    The code uses a lookup-table to skip image voxels that are not hit by the
    view. If running on a distributed memory system with n nodes, only every
    n\em th plane of the image is processed.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::back_proj3d_oblique_table2(tmaskline * const tbp,
                                      float * const img, float *sino,
                                      const unsigned long int viewsize_padded,
                                      const unsigned short int subset,
                                      const unsigned short int segment,
                                      const unsigned short int z_start,
                                      const unsigned short int z_end,
                                      const unsigned short int proc,
                                      const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=viewsize_padded)
    { float del_r_x, del_r_y, del_z_x, del_z_y, r000, p000, *ip;
      unsigned long int idx;
                                   // calculate increments to follow view plane
      { float tmp;

        tmp=DeltaXY/BinSizeRho;
        del_r_x=cos_theta[t]*tmp;
        del_r_y=sin_theta[t]*tmp;
        tmp=sin_phi[segment]*DeltaXY/(cos_phi[segment]*plane_separation);
        del_z_x=sin_theta[t]*tmp;
        del_z_y=-cos_theta[t]*tmp;
      }
                                               // calculate start point of view
      r000=rho_center-image_center*(del_r_x+del_r_y);
      p000=-image_center*(del_z_x+del_z_y);
                              // pointer into lookup-table with line-masks that
                              // excludes voxels which are not hit by the view
      idx=((unsigned long int)t*
           (unsigned long int)axis_slices[0]+z_start+proc)*
          (unsigned long int)XYSamples;
                                    // follow view through volume and calculate
                                    // weights by bilinear interpolation
      ip=img+proc*image_plane_size;
      for (unsigned short int z=z_start+proc; z < z_end; z+=num_procs,
           ip+=(unsigned long int)(num_procs-1)*image_plane_size,
           idx+=(num_procs-1)*XYSamples)
       { float r00, p00;
         unsigned short int y;

         for (p00=p000+(float)z,
              r00=r000,
              y=0; y < XYSamples; y++,
              r00+=del_r_y,
              p00+=del_z_y,
              ip+=XYSamples,
              idx++)
          { float r0, p0;
            unsigned short int x;

            for (r0=r00+(float)tbp[idx].start*del_r_x,
                 p0=p00+(float)tbp[idx].start*del_z_x,
                        // exclude voxels in line, that are not hit by the view
                 x=tbp[idx].start; x <= tbp[idx].end; x++,
                 r0+=del_r_x,
                 p0+=del_z_x)
             { signed short int rfloor, pfloor;
               float *p2dp;

               pfloor=(signed short int)(p0+1.0f)-1;         // number of plane
               rfloor=(signed short int)(r0+1.0f)-1;  // number of bin in plane
                       // the four LORs that hit the voxel image[x,y,z] are
                       // view[p,r], view[p+1,r], view[p,r+1] and view[p+1,r+1]
               p2dp=sino+(unsigned long int)(pfloor+1)*
                         (unsigned long int)RhoSamples_padded+rfloor+1;
               _bilinearInterpolation(ip[x], r0-(float)rfloor,
                                      p0-(float)pfloor, p2dp[0], p2dp[1],
                                      p2dp[RhoSamples_padded],
                                      p2dp[RhoSamples_padded+1]);
             }
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate circular mask for image.
    \param[in] radius_fov   radius of field-of-view in mm

    Calculate a circular mask for the image. The center of the mask is in the
    center of the image matrix. The mask is stored by the index of the first
    and last pixel of each image line (start and end), that is inside the
    mask. A pixel (x,y) is inside the mask, if:
    \f[
        \left(\left(y-\frac{\mbox{XYSamples}}{2}+0.5\right)\Delta_{xy}
        \right)^2+
        \left(\left(x-\frac{\mbox{XYSamples}}{2}+0.5\right)\Delta_{xy}
        \right)^2\le\mbox{radius\_fov}^2
        \qquad\forall\quad 0\le x,y<XYSamples
    \f]
    If a complete line is outside of the mask, the start value will be larger
    than the end value.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::initMask(const float radius_fov)
 { float xy_off, rf;

   rf=radius_fov*radius_fov;
   xy_off=(float)(XYSamples/2);
   mask.resize(XYSamples);
   for (unsigned short int y=0; y < XYSamples; y++)
    { float yv;
      bool found_in=false;

      yv=((float)y-xy_off+0.5f)*DeltaXY;
      yv*=yv;
      mask[y].start=XYSamples;   // first voxel of this row that is inside mask
      mask[y].end=0;              // last voxel of this row that is inside mask
                           // find first and last voxel of circular mask in row
      for (unsigned short int x=0; x < XYSamples; x++)
       { float xv;

         xv=((float)x-xy_off+0.5f)*DeltaXY;
         if (xv*xv+yv <= rf)
          { if (!found_in) { found_in=true;
                             mask[y].start=x;
                           }
            mask[y].end=x;
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate mode 1 LUTs for backprojection.

    Calculate mode 1 LUTs for backprojection.
    For each voxel (x,y) the coordinate (r) in the sinogram projection is
    calculated, where the LOR that intersects the voxel hits the projection (t)
    in a \f$90^{o}\f$ angle:
    \f[
        r=\rho_c+\left(\left(y-i_c\right)\sin\theta+
                       \left(x-i_c\right)\cos\theta\right)
          \frac{\Delta_{xy}}{\Delta_p}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    the center of a projection at:
    \f[
       \rho_c=\frac{RhoSamples}{2}-
              \left(0.5-\frac{0.5}{\mbox{reb\_factor}}\right)
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$ and the sinogram bin size
    \f$\Delta_\rho\f$.

    The values \f$r-\lfloor r\rfloor\f$ is stored in the \em lf LUT.
    The values \f$\lfloor r\rfloor+1\f$ are stored in the \em lr LUT.
    The values
    \f[
        p_{xy}=(-y\cos\theta+x\sin\theta)\frac{\Delta_{xy}}{\Delta_z}
    \f]
    are stored in the \em lp LUT with the x/y-coordinates in the \em lxy LUT.

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::initTable1()
 { float *lf, *lf_tmp, *lp, *lp_tmp=NULL;
   signed short int *lr, *lr_tmp;
   uint32 *lxy, *lxy_tmp;
   double size;
   unsigned short int sf, lut_fact_tmp, lut_r_tmp, lut_xy_tmp, lut_p_tmp;
   unsigned long int tabsize, count=0;

   if (axes > 1) sf=2;
    else sf=1;
   tabsize=(unsigned long int)ThetaSamples*image_plane_size;
   size=tabsize*(sf*sizeof(float)+sizeof(signed short int)+sizeof(uint32))/
        1024.0;
   Logging::flog()->logMsg("create mode 1 lookup-table for backprojector: #1"
                           " MByte", loglevel+1)->arg(size/1024.0);
   lf_tmp=MemCtrl::mc()->createFloat(tabsize, &lut_fact_tmp, "bplt1",
                                     loglevel+1);
   lr_tmp=MemCtrl::mc()->createSInt(tabsize, &lut_r_tmp, "bplt1", loglevel+1);
   lxy_tmp=MemCtrl::mc()->createUInt(tabsize, &lut_xy_tmp, "bplt1",
                                     loglevel+1);
   if (axes > 1)
    lp_tmp=MemCtrl::mc()->createFloat(tabsize, &lut_p_tmp, "bplt1",
                                      loglevel+1);

   lut_pos.resize(ThetaSamples+1);
   lut_pos[0]=0;
   for (unsigned short int t=0; t < ThetaSamples; t++)
    { float del_r_x, del_r_y, r000, r00;
      unsigned short int y;
                                   // calculate increments to follow view plane
      { float tmp;

        tmp=DeltaXY/BinSizeRho;
        del_r_x=cos_theta[t]*tmp;
        del_r_y=sin_theta[t]*tmp;
      }
                                               // calculate start point of view
      r000=rho_center-image_center*(del_r_x+del_r_y);
      for (r00=r000,
           y=0; y < XYSamples; y++,
           r00+=del_r_y)
       { float r;
         unsigned short int x;

         for (r=r00+(float)mask[y].start*del_r_x,
                        // exclude voxels in line, that are not hit by the view
              x=mask[y].start; x <= mask[y].end; x++,
              r+=del_r_x)
          if ((r >= -1.0f) && (r < (float)RhoSamples))
           { signed short int rfloor;

             rfloor=(signed short int)(r+1.0f)-1;     // number of bin in plane
             lr_tmp[count]=rfloor+1;
             lf_tmp[count]=r-(float)rfloor;
             lxy_tmp[count]=(unsigned long int)y*(unsigned long int)XYSamples+
                            x;
             if (axes > 1)
              lp_tmp[count]=(-(float)y*cos_theta[t]+(float)x*sin_theta[t])*
                            DeltaXY/plane_separation;
             count++;
           }
       }
      lut_pos[t+1]=count;
    }
   tabsize=lut_pos[ThetaSamples];
   size=tabsize*(sf*sizeof(float)+sizeof(signed short int)+sizeof(uint32))/
        1024.0;
   Logging::flog()->logMsg("shrink mode 1 lookup-table for backprojector: #1"
                           " MByte", loglevel+1)->arg(size/1024.0);

   lr=MemCtrl::mc()->createSInt(tabsize, &lut_r, "bplt1", loglevel+1);
   memcpy(lr, lr_tmp, tabsize*sizeof(signed short int));
   MemCtrl::mc()->put(lut_r);
   MemCtrl::mc()->put(lut_r_tmp);
   MemCtrl::mc()->deleteBlock(&lut_r_tmp);

   lf=MemCtrl::mc()->createFloat(tabsize, &lut_fact, "bplt1", loglevel+1);
   memcpy(lf, lf_tmp, tabsize*sizeof(float));
   MemCtrl::mc()->put(lut_fact);
   MemCtrl::mc()->put(lut_fact_tmp);
   MemCtrl::mc()->deleteBlock(&lut_fact_tmp);

   lxy=MemCtrl::mc()->createUInt(tabsize, &lut_xy, "bplt1", loglevel+1);
   memcpy(lxy, lxy_tmp, tabsize*sizeof(uint32));
   MemCtrl::mc()->put(lut_xy);
   MemCtrl::mc()->put(lut_xy_tmp);
   MemCtrl::mc()->deleteBlock(&lut_xy_tmp);

   if (axes > 1)
    { lp=MemCtrl::mc()->createFloat(tabsize, &lut_p, "bplt1", loglevel+1);
      memcpy(lp, lp_tmp, tabsize*sizeof(float));
      MemCtrl::mc()->put(lut_p);
      MemCtrl::mc()->put(lut_p_tmp);
      MemCtrl::mc()->deleteBlock(&lut_p_tmp);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize mode 2 LUT for backprojection.
    \param[in] max_threads   maximum number of parallel threads to use

    Initialize mode 2 LUT for backprojection. This method is multi-threaded.
    Each thread calculates the lookup-table for a range of image slices.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::initTable2(const unsigned short int max_threads)
 { double size;
   std::string str;
   unsigned long int pairs;
                                                     // calculate size of table
   pairs=((unsigned long int)((segments-1)*ThetaSamples)*
          (unsigned long int)axis_slices[0]+ThetaSamples)*
         (unsigned long int)XYSamples;
   size=pairs*sizeof(tmaskline)/1024.0;
   if (axes > 1) str=toString(size/1024.0)+" MByte";
    else str=toString(size)+" KByte";
   Logging::flog()->logMsg("create mode 2 lookup-table for backprojector: "+
                           str, loglevel+1);
   table_bkp.assign(segments, MemCtrl::MAX_BLOCK);
                                            // calculate table for all segments
   for (unsigned short int segment=0; segment < segments; segment++)
    { tmaskline *tbp;
      unsigned long int size;

      size=(unsigned long int)ThetaSamples*(unsigned long int)XYSamples;
      if (segment > 0) size*=(unsigned long int)axis_slices[0];
      tbp=(tmaskline *)MemCtrl::mc()->createSInt(size*2, &table_bkp[segment],
                                                 "bplt2", loglevel+1);
      searchIndicesBkp(tbp, segment, max_threads);
      MemCtrl::mc()->put(table_bkp[segment]);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate mode 2 LUT for one segment of the backprojection.
    \param[in,out] tbp           pointer to lookup-table
    \param[in]     segment       number of segment
    \param[in]     max_threads   maximum number of parallel threads to use

    Calculate mode 2 LUT for one segment of the backprojection. The calculation
    for oblique segments is multi-threaded. Every thread processes a range of
    image slices.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::searchIndicesBkp(tmaskline * const tbp,
                                const unsigned short int segment,
                                const unsigned short int max_threads)
 { try
   { unsigned short int threads=0, t;
     void *thread_result;
     std::vector <bool> thread_running(0);
     std::vector <tthread_id> tid;

     try
     { std::vector <tthread_params> tp;
                                                    // number of threads to use
       threads=std::min(max_threads, axis_slices[0]);
       if (threads > 1)
        { tid.resize(threads);                          // table for thread-IDs
          tp.resize(threads);                    // table for thread parameters
                                                     // table for thread status
          thread_running.assign(threads, false);
        }
                                             // calculate all angles of segment
       for (unsigned short int theta=0; theta < ThetaSamples; theta++)
        {                                     // sequential calculation of view
          if (segment == 0)
           { searchIndicesBkp_seg0(tbp, theta);                    // segment 0
             continue;
           }
           else if (threads == 1)
                 {                           // single-threaded oblique segment
                   searchIndicesBkp_oblique(tbp, theta, segment, 0,
                                            axis_slices[0]);
                   continue;
                 }
          unsigned short int slices, z_start, i;
                                    // parallel calculation of oblique segments
          for (slices=axis_slices[0],
               z_start=0,
               t=threads,
               i=0; i < threads; i++,
               t--)
           {                                     // calculate thread parameters
             tp[i].object=this;
             tp[i].t=theta;
             tp[i].tbp=tbp;
             tp[i].segment=segment;
             tp[i].z_start=z_start;                     // first slice of image
             tp[i].z_end=z_start+slices/t;               // last slice of image
#ifdef XEON_HYPERTHREADING_BUG
             tp[i].thread_number=i;
#endif
             thread_running[i]=true;
                                                                // start thread
             ThreadCreate(&tid[i], &executeThread_searchBkp, &tp[i]);
             slices-=(tp[i].z_end-tp[i].z_start);
             z_start=tp[i].z_end;
           }
                                         // wait until all threads are finished
          for (i=0; i < threads; i++)
           { ThreadJoin(tid[i], &thread_result);
             thread_running[i]=false;
             if (thread_result != NULL) throw (Exception *)thread_result;
           }
        }
     }
     catch (...)                                // handle exceptions of threads
      { thread_result=NULL;
                                         // wait until all threads are finished
        for (t=0; t < thread_running.size(); t++)
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)              //  handle exceptions of this method
    { std::string err_str;
      unsigned long int err_code;

      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create mode 2 LUT for segment 0.
    \param[in,out] tbp   pointer to lookup-table
    \param[in]     t     number of projection in sinogram plane

    This method searches the voxels in every line of the image, that are hit
    by the view (t,segment) of an oblique segment. For each voxel (x,y,0) the
    coordinate (r,p) in the sinogram view is calculated, where the LOR that
    intersects the voxel hits the view (t,segment) in a
    \f$90^{o}\f$ angle:
    \f[
        r=\rho_c+\left(\left(y-i_c\right)\sin\theta+
                       \left(x-i_c\right)\cos\theta\right)
          \frac{\Delta_{xy}}{\Delta_p}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    the center of a projection at:
    \f[
       \rho_c=\frac{RhoSamples}{2}-
              \left(0.5-\frac{0.5}{\mbox{reb\_factor}}\right)
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$ and the sinogram bin size
    \f$\Delta_\rho\f$.

    The coordinates of the first and last voxel of every image line are stored
    where the coordinate (r,p) is inside the view (t,0). The table will contain
    \f$T*\mbox{XYSamples}\f$ of these pairs, where \f$T\f$ is the number of
    projections in a sinogram plane.

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::searchIndicesBkp_seg0(tmaskline * const tbp,
                                     const unsigned short int t) const
 { float del_r_x, del_r_y, r00;
   unsigned long int idx;
                                   // calculate increments to follow view plane
   { float tmp;

     tmp=DeltaXY/BinSizeRho;
     del_r_x=cos_theta[t]*tmp;
     del_r_y=sin_theta[t]*tmp;
   }
                                               // calculate start point of view
   r00=rho_center-image_center*(del_r_x+del_r_y);
                              // pointer into lookup-table with line-masks that
                              // excludes voxels which are not hit by the view
   idx=(unsigned long int)t*(unsigned long int)XYSamples;
                                                  // follow view through volume
   for (unsigned short int y=0; y < XYSamples; y++,
        r00+=del_r_y,
        idx++)
    { unsigned short int *xfirst, *xlast, x;
      bool found_xf=false;
      float r;
                   // find voxels at beginning and end of line that are not hit
                   // and apply circular mask at the same time
      xfirst=&tbp[idx].start;
      *xfirst=1;
      xlast=&tbp[idx].end;
      *xlast=0;
      for (r=r00+(float)mask[y].start*del_r_x,
           x=mask[y].start; x <= mask[y].end; x++,
           r+=del_r_x)
       if ((r >= -1.0f) && (r < (float)RhoSamples))
        { if (!found_xf) { *xfirst=x;
                           found_xf=true;
                         }
          *xlast=x;
        }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create mode 2 LUT for oblique segment.
    \param[in,out] tbp         pointer to lookup-table
    \param[in]     t           number of projection in sinogram plane
    \param[in]     segment     number of segment
    \param[in]     z_start     first slice of image to calculate
    \param[in]     z_end       last slice+1 of image to calculate

    This method searches the voxels in every line of the image, that are hit
    by the view (t,segment) of an oblique segment. For each voxel (x,y,z) the
    coordinate (r,p) in the sinogram view is calculated, where the LOR that
    intersects the voxel hits the view (t,segment) in a
    \f$90^{o}\f$ angle:
    \f[
        r=\rho_c+\left(\left(y-i_c\right)\sin\theta+
                       \left(x-i_c\right)\cos\theta\right)
          \frac{\Delta_{xy}}{\Delta_p}
    \f]
    \f[
        p=z+\left(-\left(y-i_c\right)\cos\theta+
                  \left(x-i_c\right)\sin\theta\right)
            \frac{\sin\phi\Delta_{xy}}{\cos\phi\Delta_z}
    \f]
    with the center of the image at:
    \f[
        i_c=\frac{\mbox{XYSamples}}{2}-0.5+\mbox{image\_cor}
    \f]
    the center of a projection at:
    \f[
       \rho_c=\frac{RhoSamples}{2}-
              \left(0.5-\frac{0.5}{\mbox{reb\_factor}}\right)
    \f]
    and the voxel width/height \f$\Delta_{xy}\f$, the voxel depth
    \f$\Delta_z\f$ and the sinogram bin size \f$\Delta_\rho\f$.

    The coordinates of the first and last voxel of every image line are stored
    where the coordinate (r,p) is inside the view (t,segment). The table will
    contain \f$T\cdot\mbox{axis\_slices}[0]\cdot\mbox{XYSamples}\f$ of these
    pairs, where \f$T\f$ is the number of projections in a sinogram plane.

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image.
 */
/*---------------------------------------------------------------------------*/
void BckPrj3D::searchIndicesBkp_oblique(tmaskline * const tbp,
                                        const unsigned short int t,
                                        const unsigned short int segment,
                                        const unsigned short int z_start,
                                        const unsigned short int z_end) const
 { float del_r_x, del_r_y, del_z_x, del_z_y, r000, p000;
   unsigned short int zbot, ztop;
   unsigned long int idx;
                                   // calculate increments to follow view plane
   { float tmp;

     tmp=DeltaXY/BinSizeRho;
     del_r_x=cos_theta[t]*tmp;
     del_r_y=sin_theta[t]*tmp;
     tmp=sin_phi[segment]*DeltaXY/(cos_phi[segment]*plane_separation);
     del_z_x=sin_theta[t]*tmp;
     del_z_y=-cos_theta[t]*tmp;
   }
                                               // calculate start point of view
   r000=rho_center-image_center*(del_r_x+del_r_y);
   p000=-image_center*(del_z_x+del_z_y);
   zbot=z_bottom[segment];
   ztop=z_top[segment];
                              // pointer into lookup-table with line-masks that
                              // excludes voxels which are not hit by the view
   idx=((unsigned long int)t*(unsigned long int)axis_slices[0]+z_start)*
       (unsigned long int)XYSamples;
                                                  // follow view through volume
   for (unsigned short int z=z_start; z < z_end; z++)
    { float r00, p00;
      unsigned short int y;

      for (p00=p000+(float)z,
           r00=r000,
           y=0; y < XYSamples; y++,
           r00+=del_r_y,
           p00+=del_z_y,
           idx++)
       { unsigned short int *xfirst, *xlast, x;
         bool found_xf=false;
         float r, p;
                   // find voxels at beginning and end of line that are not hit
                   // and apply circular mask at the same time
         xfirst=&tbp[idx].start;
         *xfirst=1;
         xlast=&tbp[idx].end;
         *xlast=0;
         for (r=r00+(float)mask[y].start*del_r_x,
              p=p00+(float)mask[y].start*del_z_x,
              x=mask[y].start; x <= mask[y].end; x++,
              r+=del_r_x,
              p+=del_z_x)
          if ((p >= (float)(zbot-1)) && (p < (float)(ztop+1)))
           if ((r >= -1.0f) && (r < (float)RhoSamples))
            { if (!found_xf) { *xfirst=x;
                               found_xf=true;
                             }
              *xlast=x;
            }
       }
    }
 }
