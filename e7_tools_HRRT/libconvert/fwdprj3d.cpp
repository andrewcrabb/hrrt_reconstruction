/*! \class FwdPrj3D fwdprj3d.h "fwdprj3d.h"
    \brief This class provides a method to forward-project images into
           sinograms.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/01/02 image mask is centered in image, independent from COR
    \date 2004/01/30 COR for image and for projections are independent
    \date 2004/03/08 uses memCtrl for memory handling
    \date 2004/03/22 add time-of-flight support
    \date 2004/04/02 use vector code for jacobians
    \date 2004/04/20 added Doxygen style comments
    \date 2004/08/09 use fact that length of norm vector in TOF is constant
    \date 2004/08/10 fix bug in calculation of positon along LOR for TOF
    \date 2004/10/28 add new lookup-tables for radial, x/y- and image
                     coordinates and interpolation factors
    \date 2005/03/29 LUT2 was using twice as much memory as required

    The forward-projector uses a bilinear interpolation between the four voxels
    in a x/z- or y/z-slice that may be hit by a LOR. In case of a 2d forward-
    projection this simplifies to a linear interpolation between two voxels.

    During the forward-projection a circular mask is applied to the image
    (95% of image width). Every part of the image outside the mask is assumed
    to be zero.

    The forward-projector takes into account, that the axial angle of the
    border planes of an oblique segment is slightly smaller than in the center
    of the segment due to the span.

    There are different kinds of algorithmic optimizations used in the
    implementation of this class:
     - avoid if-then-else inside of forward-projector-loop by using a lookup
       table (since the table may be very large, this is optional, LUT mode 2)
       and padding of the image
     - use lookup tables for trigonometric calculations
     - use lookup tables to reduce number of calculations that need to be done
       during the forward-projector loop (interpolation factors, image and
       sinogram coordinates)
     - calculate the simpler segment 0 case separately
     - use sinogram in view-mode to increase data locality
     - multi-threading to use all CPUs in a shared memory system
     - multi-processing to use all nodes in a cluster

    The forward-projector can use two different sets of lookup-tables to speed
    up the calculation. The selection is done by choosing a LUT mode. In mode 0
    no LUTs are used, in mode 1 and 2 different LUTs are used.
    The tables that are used in mode 1 store interpolation factors and
    coordinates that will be used during forward-projection, thus reducing the
    amount of calculations that is done on-the-fly.
    The table that is used in mode 2 stores the first and last bin of every
    sinogram projection, that is hit by an image projection. This way we don't
    need to check during forward-projection, if a bin is hit by a image
    projection or not. This optimization makes only sense, if more than one
    forward-projection is calculated (e.g. in OSEM reconstructions with more
    than one iteration).
    There are two methods used to fill the mode 2 lookup-table:
    \em searchIndicesFwd_oblique and \em searchIndicesFwd_seg0, for oblique
    segments and segment 0.

    The LUTs in mode 1 and 2 have different memory requirements, depending on
    the image and sinogram size. Also the LUTs for mode 1 can be calculated
    much faster and are even useful if the forward-projector is used only once.
    There are also differences in speed that depent on the hardware and the
    number of threads that are used.

    There are ten implementations of the forward-projector: with different
    lookup-tables, time-of-flight or non-time-of-flight, oblique segments or
    segment 0:
    - with LUT mode 2:
     - non-time-of-flight:
      - \em forward_proj3d_oblique_table2,
      - \em forward_proj3d_seg0_table2
     - time-of-flight:
      - \em forward_proj3d_oblique_table2_tof,
      - \em forward_proj3d_seg0_table2_tof
    - with LUT mode 1:
     - non-time-of-flight:
      - \em forward_proj3d_oblique_table1,
      - \em forward_proj3d_seg0_table1
    - without LUT (mode 0):
     - non-time-of-flight:
      - \em forward_proj3d_oblique
      - \em forward_proj3d_seg0
     - time-of-flight:
      - \em forward_proj3d_oblique_tof,
      - \em forward_proj3d_seg0_tof

    The image used as input to the forward-projector needs to be padded in x-,
    y- and z-direction by 2 voxels, i.e. the size of the image is
    \f[
        (1+\mbox{XYSamples}+1)(1+\mbox{XYSamples}+1)
        (1+\mbox{axis\_slices}[0]+1)
    \f]
    This way we don't need special cases for the bilinear interpolations at the
    borders of an image. The forward-projection is calculated in parallel where
    each thread calculates some planes of the resulting sinogram and the number
    of threads is limited by the \em max_threads constant.

    The produced sinogram is also padded. This is not really necessary here,
    but the sinogram is usually used as input for a back-projector in an ML-EM
    algorithm and the backprojector expects a padded sinogram for performance
    reasons.
    The sinogram produced as output is stored in view-mode and padded in
    \f$\rho\f$-direction by 2*tof_bins and in z-direction by 2 bins, i.e. the
    size of the sinogram is
    \f[
        (\mbox{tof\_bins}+\mbox{RhoSamples}+\mbox{tof\_bins})
        \left(\left(1+\mbox{axis\_slices}[0]+1\right)+
        2\cdot\left(1+\frac{\mbox{axis\_slices}[1]}{2}+1\right)+...\right)
        \mbox{ThetaSamples}
    \f]
    The first and last row and the first and last tof_bins columns of every
    view will be 0.

    The class provides also a method to calculate maximum intensity
    projections. These can be calculated with or without mode 2 lookup-table.
 */

#include <iostream>
#include <limits>
#include <string>
#include <cstring>
#include <unistd.h>
#include "fwdprj3d.h"
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

                                     // linear interpolation between two voxels
#define _linearInterp(f, val1, val2) (val1+(f)*(val2-(val1)))

       // add result of linear interpolation between two voxels to sinogram bin
#define _linearInterpolation(sino, f, val1, val2)\
 sino+=_linearInterp(f, val1, val2);

                                  // bilinear interpolation between four voxels
#define _bilinearInterp(v, f1, f2, val1, val2, val3, val4)\
 { float _a, _b;\
\
   _a=_linearInterp(f1, val1, val2);\
   _b=_linearInterp(f1, val3, val4);\
   v=_linearInterp(f2, _a, _b);\
 }

    // add result of bilinear interpolation between four voxels to sinogram bin
#define _bilinearInterpolation(sino, f1, f2, val1, val2, val3, val4)\
 { float _v;\
\
   _bilinearInterp(_v, f1, f2, val1, val2, val3, val4);\
   sino+=_v;\
 }

/*- constants ---------------------------------------------------------------*/

                               /*! offset for center of image in voxel units */
const float FwdPrj3D::image_cor=0.5f;

/*- local functions ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to calculate forward-projection.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to calculate forward-projection.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_Fwdproject(void *param)
 { try
   { FwdPrj3D::tthread_params *tp;

     tp=(FwdPrj3D::tthread_params *)param;
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
     switch (tp->lut_mode)                                    // LUT mode
            { case 0:
               if (tp->segment == 0)                       // segment 0, no LUT
                tp->object->forward_proj3d_seg0(tp->image, tp->proj,
                                                tp->subset, tp->z_start,
                                                tp->z_end, tp->proc,
                                                tp->num_procs);
                                                     // oblique segment, no LUT
                else tp->object->forward_proj3d_oblique(tp->image, tp->proj,
                                                        tp->subset,
                                                        tp->segment,
                                                        tp->z_start, tp->z_end,
                                                        tp->proc,
                                                        tp->num_procs);
               break;
              case 1:
               if (tp->segment == 0)                   // segment 0, mode 1 LUT
                tp->object->forward_proj3d_seg0_table1(tp->image, tp->proj,
                                                       tp->subset, tp->z_start,
                                                       tp->z_end, tp->lut_fact,
                                                       tp->lut_idx,
                                                       tp->lut_r, tp->proc,
                                                       tp->num_procs);
                                                 // oblique segment, mode 1 LUT
                else tp->object->forward_proj3d_oblique_table1(tp->image,
                                                               tp->proj,
                                                               tp->subset,
                                                               tp->segment,
                                                               tp->z_start,
                                                               tp->z_end,
                                                               tp->lut_fact,
                                                               tp->lut_idx,
                                                               tp->lut_r,
                                                               tp->lut_xy,
                                                               tp->proc,
                                                               tp->num_procs);
               break;
              case 2:
               if (tp->segment == 0)                   // segment 0, mode 2 LUT
                tp->object->forward_proj3d_seg0_table2(tp->tfp, tp->image,
                                                       tp->proj, tp->subset,
                                                       tp->z_start,
                                                       tp->z_end, tp->proc,
                                                       tp->num_procs);
                                                 // oblique segment, mode 2 LUT
                else tp->object->forward_proj3d_oblique_table2(tp->tfp,
                                                               tp->image,
                                                               tp->proj,
                                                               tp->subset,
                                                               tp->segment,
                                                               tp->z_start,
                                                               tp->z_end,
                                                               tp->proc,
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
/*! \brief Start thread to calculate MIP-projection.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to calculate MIP-projection.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_MIPproject(void *param)
 { try
   { FwdPrj3D::tthread_params *tp;

     tp=(FwdPrj3D::tthread_params *)param;
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
     if (tp->lut_mode == 2)                                        // use LUT ?
      tp->object->mip_proj_table2(tp->tfp, tp->image, tp->proj, tp->subset,
                                  tp->z_start, tp->z_end);
      else tp->object->mip_proj(tp->image, tp->proj, tp->subset, tp->z_start,
                                tp->z_end);
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
/*! \brief Start thread to calculate mode 2 LUT for oblique segments of
           forward-projection.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to calculate mode 2 LUT for oblique segments of forward-
    projection.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_searchFwd(void *param)
 { try
   { FwdPrj3D::tthread_params *tp;

     tp=(FwdPrj3D::tthread_params *)param;
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
     tp->object->searchIndicesFwd_oblique(tp->tfp, tp->segment, tp->z_start,
                                          tp->z_end);
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
                                   2=use mode 2 lookup-tables
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
        \mbox{sin\_phi}[segment,s]=\sin\phi=\left\{
        \begin{array}{r@{\quad:\quad}l}
         -\frac{2\left(\left\lfloor\frac{\mbox{segment}+1}{2}\right\rfloor
                \mbox{span}-\frac{span-1}{2}+\frac{s+1}{2}\right)\Delta_z}
               {\sqrt{\left(2\left(\left\lfloor\frac{\mbox{segment}+1}{2}
                      \right\rfloor\mbox{span}-\frac{span-1}{2}+\frac{s+1}{2}
                      \right)\Delta_z\right)^2+
                      \left(2\cdot\mbox{radius}\right)^2}} &
         \mbox{segment odd}\\
         \frac{2\left(\left\lfloor\frac{\mbox{segment}+1}{2}\right\rfloor
               \mbox{span}-\frac{span-1}{2}+\frac{s+1}{2}\right)\Delta_z}
               {\sqrt{\left(2\left(\left\lfloor\frac{\mbox{segment}+1}{2}
                      \right\rfloor\mbox{span}-\frac{span-1}{2}+\frac{s+1}{2}
                      \right)\Delta_z\right)^2+
                      \left(2\cdot\mbox{radius}\right)^2}} &
         \mbox{segment even}
        \end{array}
        \right.\qquad\forall\quad 0\le\mbox{segment}\le 2\cdot\mbox{axes-1},
        0<s\le span-2
    \f]
    \f[
        \mbox{cos\_phi}[segment,s]=\cos\phi=
        \frac{2\cdot\mbox{radius}}
              {\sqrt{\left(2\left(\left\lfloor\frac{\mbox{segment}+1}{2}
                     \right\rfloor\mbox{span}-\frac{span-1}{2}+
                     \frac{s+1}{2}\right)\Delta_z\right)^2+
                     \left(2\cdot\mbox{radius}\right)^2}}
        \qquad\forall\quad 0\le\mbox{segment}\le 2\cdot\mbox{axes-1},
        0<s\le span-2
    \f]
    with \f$\mbox{radius}=crystal\_radius+doi\f$. The axial angles at the
    border of a segment are smaller, since the span is smaller in these
    planes.

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
    Each plane of the image is circular. The voxel coordinates of the borders
    of this circle are stored in the \f$\mbox{mask}\f$ structure.

    The constructor also initializes the lookup-tables if LUT mode 1 or 2 is
    used.
 */
/*---------------------------------------------------------------------------*/
FwdPrj3D::FwdPrj3D(const unsigned short int _XYSamples, const float _DeltaXY,
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
 { unsigned short int axis;

   loglevel=_loglevel;
   XYSamples=_XYSamples;
   DeltaXY=_DeltaXY;
   RhoSamples=_RhoSamples;
   BinSizeRho=_BinSizeRho;
   ThetaSamples=_ThetaSamples;
   plane_separation=_plane_separation;
   lut_mode=_lut_mode;
   tof_bins=1;
                    // center of projections
                    // (depends on rebinning of sinogram before reconstruction)
   rho_center=(float)(RhoSamples/2)-(0.5f-0.5f/reb_factor);
   image_center=((float)(XYSamples/2)-0.5f+image_cor);       // center of image
   subsets=_subsets;
   span=_span;
   if (span == 0) throw Exception(REC_SPAN_RD, "Span is 0.");
   axes=_axes;
   Logging::flog()->logMsg("initialize forward-projector", loglevel);
                                                     // calculate segment table
   axis_slices=_axis_slices;
   axes_slices=0;
   for (axis=0; axis < axes; axis++)
    axes_slices+=axis_slices[axis];
   segments=2*axes-1;
   z_bottom.resize(segments);
   z_top.resize(segments);
                                           // first and last plane of segment 0
   z_bottom[0]=0;
   z_top[0]=axis_slices[0]-1;
   if (axes > 1)
    { vsin_phi.resize(axes*(span-1));
      vcos_phi.resize(axes*(span-1));
      for (axis=1; axis < axes; axis++)
       { float tmp_flt;
                     // first and last planes of segments relative to segment 0
         z_bottom[axis*2]=(axis_slices[0]-axis_slices[axis]/2)/2;
         z_top[axis*2]=z_bottom[axis*2]+axis_slices[axis]/2-1;
         z_bottom[axis*2-1]=z_bottom[axis*2];
         z_top[axis*2-1]=z_top[axis*2];
                 // sine and cosine of axial angle for border planes of segment
         for (unsigned short int z=0; z < span-2; z++)
          { float tmp_flt;
            unsigned short int mean_rd;

            mean_rd=axis*span-(span-1)/2+(z+1)/2;
            tmp_flt=mean_rd*plane_separation*2.0f;
            tmp_flt=sqrtf(tmp_flt*tmp_flt+
                          2.0f*(GM::gm()->ringRadius()+GM::gm()->DOI())*
                          2.0f*(GM::gm()->ringRadius()+GM::gm()->DOI()));
            vcos_phi[(axis-1)*(span-1)+z]=2.0f*(GM::gm()->ringRadius()+
                                                GM::gm()->DOI())/tmp_flt;
            vsin_phi[(axis-1)*(span-1)+z]=-mean_rd*plane_separation*2.0f/
                                          tmp_flt;
          }
                // sine and cosine of axial angle for central planes of segment
         tmp_flt=(float)axis*(float)span*plane_separation*2.0f;
         tmp_flt=sqrtf(tmp_flt*tmp_flt+
                       2.0f*(GM::gm()->ringRadius()+GM::gm()->DOI())*
                       2.0f*(GM::gm()->ringRadius()+GM::gm()->DOI()));
         vsin_phi[(axis-1)*(span-1)+span-2]=-(float)axis*(float)span*
                                             plane_separation*2.0f/tmp_flt;
         vcos_phi[(axis-1)*(span-1)+span-2]=2.0f*(GM::gm()->ringRadius()+
                                                  GM::gm()->DOI())/tmp_flt;
       }
    }
   Logging::flog()->logMsg("angles of segment 0", loglevel+1);
   Logging::flog()->logMsg(" plane 0-#1: 0 degrees", loglevel+1)->
    arg(axis_slices[0]-1);
   for (axis=1; axis < axes; axis++)
    { unsigned short int z;
      std::string str;

      Logging::flog()->logMsg("angles of segment +#1", loglevel+1)->
       arg(axis);
      for (z=0; z <= span-2; z++)
       { str=" plane "+toString(z);
         if (z < span-2) str+=",";
          else str+="-";
         Logging::flog()->logMsg(str+"#1: #2 degrees", loglevel+1)->
          arg(axis_slices[axis]/2-z-1)->
          arg(asinf(vsin_phi[(axis-1)*(span-1)+z])*180.0f/M_PI);
       }
      Logging::flog()->logMsg("angles of segment -#1", loglevel+1)->
       arg(axis);
      for (z=0; z <= span-2; z++)
       { str=" plane "+toString(z);
         if (z < span-2) str+="," ;
          else str+="-";
         Logging::flog()->logMsg(str+"#1: #2 degrees", loglevel+1)->
          arg(axis_slices[axis]/2-z-1)->
          arg(-asinf(vsin_phi[(axis-1)*(span-1)+z])*180.0f/M_PI);
       }
    }
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
   XYSamples_padded=XYSamples+2;                // width/height of padded image
                                                  // size of padded image slice
   image_plane_size_padded=(unsigned long int)XYSamples_padded*
                           (unsigned long int)XYSamples_padded;
   RhoSamples_padded=(RhoSamples+2)*tof_bins;       // width of padded sinogram
                                                         // size of padded view
   view_size_padded=(unsigned long int)(axis_slices[0]+2)*
                    (unsigned long int)RhoSamples_padded;
                                                       // size of padded subset
   subset_size_padded=(unsigned long int)(ThetaSamples/subsets)*
                      view_size_padded;
   initMask(0.95f*DeltaXY*(float)(XYSamples/2));       // initialize image mask
                            // initialize look-up-tables for forward-projection
   lut_fact=MemCtrl::MAX_BLOCK;
   lut_idx=MemCtrl::MAX_BLOCK;
   lut_r=MemCtrl::MAX_BLOCK;
   lut_xy=MemCtrl::MAX_BLOCK;
   if ((tof_bins == 1) && (lut_mode == 1)) initTable1();
    else if (lut_mode == 2) initTable2(max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object and release used memory.

    Destroy the object and release the used memory.
 */
/*---------------------------------------------------------------------------*/
FwdPrj3D::~FwdPrj3D()
 { for (unsigned short int i=0; i < table_fwd.size(); i++)
    MemCtrl::mc()->deleteBlockForce(&table_fwd[i]);
   MemCtrl::mc()->deleteBlockForce(&lut_xy);
   MemCtrl::mc()->deleteBlockForce(&lut_r);
   MemCtrl::mc()->deleteBlockForce(&lut_idx);
   MemCtrl::mc()->deleteBlockForce(&lut_fact);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate axial angle of plane in oblique segment.
    \param[in]  segment    number of segment
    \param[in]  p          number of plane relative to segment 0
    \param[out] sinphi     sinus of axial angle of plane
    \param[out] tanphi     tangent of axial angle of plane
    \param[out] v_idx      index into look-up table for angle phi

    Calculate axial angle of plane in oblique segment.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::calculatePlaneAngle(const unsigned short int segment,
                                   const unsigned short int p,
                                   float * const sinphi,
                                   float * const tanphi,
                                   unsigned short int * const v_idx) const
 { unsigned short int zshift;
                      // border planes of oblique segments have different angle
   zshift=p-z_bottom[segment];
   if (zshift >= span-1) { zshift=z_top[segment]-p;
                           if (zshift > span-2) zshift=span-2;
                         }
   *v_idx=(segment-1)/2*(span-1)+zshift;
   *sinphi=vsin_phi[*v_idx];
   if ((segment % 2) == 0) *sinphi=-vsin_phi[*v_idx];
    else *sinphi=vsin_phi[*v_idx];
   *tanphi=*sinphi/vcos_phi[*v_idx];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Forward-project one subset of one segment.
    \param[in]  img           image data
    \param[out] sino          padded sinogram data in view mode
    \param[in]  subset        number of subset
    \param[in]  segment       number of segment
    \param[in]  max_threads   maximum number of parallel threads to use
    \param[in]  proc          number of node
    \param[in]  num_procs     number of nodes in distributed memory system

    Forward-project one subset of one segment.

    This method is multi-threaded. Every thread processes a range of sinogram
    planes.

    The method can also be used in a multi-processing (distributed memory)
    situation. In this case the calculation is distributed on several nodes and
    the node number and the overall number of nodes must be specified. The
    calculation on each node is multi-threaded to utilize all CPUs of a node.
    The caller is responsible for distributing the image data to the nodes
    and collecting and summing the resulting sinogram parts.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::forward_proj3d(float * const img, float *sino,
                              const unsigned short int subset,
                              const unsigned short int segment,
                              const unsigned short int max_threads,
                              const unsigned short int proc,
                              const unsigned short int num_procs)
 { try
   { unsigned short int threads=0, t;
     void *thread_result;
     std::vector <bool> thread_running(0);
     std::vector <tthread_id> tid;

     try
     { uint32 *lidx=NULL;
       float *lf=NULL;
       signed short int *lr=NULL, *lxy=NULL;
       tmaskline *tfp=NULL;
       std::vector <FwdPrj3D::tthread_params> tp;

       memset(sino, 0, subset_size_padded*sizeof(float));// initialize sinogram
                                                    // number of threads to use
       threads=std::min(max_threads, axis_slices[0]);
       if (threads > 1)
        { tid.resize(threads);                          // table for thread-IDs
          tp.resize(threads);                    // table for thread parameters
          thread_running.assign(threads, false);     // table for thread status
        }
                                          // get lookup-tables for this segment
       if (table_fwd.size() > 0)
        tfp=(tmaskline *)MemCtrl::mc()->getSIntRO(table_fwd[segment],
                                                  loglevel);
       if ((tof_bins == 1) && (lut_mode == 1))
        { lf=MemCtrl::mc()->getFloatRO(lut_fact, loglevel);
          lidx=MemCtrl::mc()->getUIntRO(lut_idx, loglevel);
          lr=MemCtrl::mc()->getSIntRO(lut_r, loglevel);
          if (segment > 0) lxy=MemCtrl::mc()->getSIntRO(lut_xy, loglevel);
        }
                                   // forward-project all angles of this subset
       if (threads == 1)                       // sequential forward-projection
         switch (lut_mode)                                     // LUT mode
               { case 0:
                  if (segment == 0)                                // segment 0
                   forward_proj3d_seg0(img, sino, subset, 0, axis_slices[0],
                                       proc, num_procs);
                                                             // oblique segment
                   else forward_proj3d_oblique(img,
                                     sino+(unsigned long int)z_bottom[segment]*
                                          (unsigned long int)RhoSamples_padded,
                                     subset, segment, z_bottom[segment],
                                     z_top[segment]+1, proc, num_procs);
                  break;
                 case 1:
                  if (segment == 0)                                // segment 0
                   forward_proj3d_seg0_table1(img, sino, subset, 0,
                                              axis_slices[0], lf, lidx, lr,
                                              proc, num_procs);
                                                             // oblique segment
                   else forward_proj3d_oblique_table1(img,
                                     sino+(unsigned long int)z_bottom[segment]*
                                          (unsigned long int)RhoSamples_padded,
                                     subset, segment, z_bottom[segment],
                                     z_top[segment]+1, lf, lidx, lr, lxy, proc,
                                     num_procs);
                  break;
                 case 2:
                  if (segment == 0)                                // segment 0
                   forward_proj3d_seg0_table2(tfp, img, sino, subset, 0,
                                              axis_slices[0], proc, num_procs);
                                                             // oblique segment
                   else forward_proj3d_oblique_table2(tfp, img,
                                     sino+(unsigned long int)z_bottom[segment]*
                                          (unsigned long int)RhoSamples_padded,
                                     subset, segment, z_bottom[segment],
                                     z_top[segment]+1, proc, num_procs);
                  break;
               }
        else { unsigned short int slices, z_start, i;
                                                 // parallel forward-projection
               slices=z_top[segment]-z_bottom[segment]+1;
               for (z_start=z_bottom[segment],
                    t=threads,
                    i=0; i < threads; i++,
                    t--)
                {                                // calculate thread parameters
                  tp[i].object=this;
                  tp[i].image=img;
                  tp[i].proj=sino+(unsigned long int)z_start*
                                  (unsigned long int)RhoSamples_padded;
                  tp[i].subset=subset;
                  tp[i].tfp=tfp;
                  tp[i].segment=segment;
                  tp[i].z_start=z_start;             // first plane of sinogram
                  tp[i].z_end=z_start+slices/t;    // last plane +1 of sinogram
                  tp[i].lut_mode=lut_mode;
                  tp[i].lut_fact=lf;
                  tp[i].lut_idx=lidx;
                  tp[i].lut_r=lr;
                  tp[i].lut_xy=lxy;
                  tp[i].tof_bins=tof_bins;
                  tp[i].proc=proc;
                  tp[i].num_procs=num_procs;
#ifdef XEON_HYPERTHREADING_BUG
                  tp[i].thread_number=i;
#endif
                  thread_running[i]=true;
                                                                // start thread
                  ThreadCreate(&tid[i], &executeThread_Fwdproject, &tp[i]);
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
                                   // return lookup-tables to memory controller
       if ((tof_bins == 1) && (lut_mode == 1))
        { if (segment > 0) MemCtrl::mc()->put(lut_xy);
          MemCtrl::mc()->put(lut_r);
          MemCtrl::mc()->put(lut_idx);
          MemCtrl::mc()->put(lut_fact);
        }
       if (table_fwd.size() > 0) MemCtrl::mc()->put(table_fwd[segment]);
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
        if (table_fwd.size() > 0) MemCtrl::mc()->put(table_fwd[segment]);
        if ((tof_bins == 1) && (lut_mode == 1))
         { MemCtrl::mc()->put(lut_xy);
           MemCtrl::mc()->put(lut_r);
           MemCtrl::mc()->put(lut_idx);
           MemCtrl::mc()->put(lut_fact);
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
/*! \brief Calculate the forward-projection of image into one subset of
           segment 0 without LUTs.
    \param[in]     img         image data
    \param[in,out] sino        padded sinogram data in view mode
    \param[in]     subset      number of subset
    \param[in]     z_start     first planee of sinogram to calculate
    \param[in]     z_end       last plane+1 of sinogram to calculate
    \param[in]     proc        number of node
    \param[in]     num_procs   number of nodes in distributed memory system

    Calculate the forward-projection of the image into into segment 0 without
    LUTs for all azimuthal angles of one subset.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate for
    each bin of the sinogram view (r,p) and line (y,z) of the image the
    coordinate x, where the LOR hits the image line. Otherwise we calculate for
    each bin of the sinogram view (r,p) and line (x,z) of the image the
    coordinate y, where the LOR hits the image line.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
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

    The image value at the coordinate (x,y,z) is calculated by linear
    interpolation for \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$:
    \f[
        x_1=x-\lfloor x\rfloor \qquad x_2=1-x_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         x_2\mbox{image}(\lfloor x\rfloor,y,z)+\\
         && x_1\mbox{image}(\lceil x\rceil,y,z)
        \end{array}
    \f]
    respectively for \f$\left|\sin\theta\right|\ge\left|\cos\theta\right|\f$:
    \f[
        y_1=y-\lfloor y\rfloor \qquad y_2=1-y_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         y_2\mbox{image}(x,\lfloor y\rfloor,z)+\\
         && y_1\mbox{image}(x,\lceil y\rceil,z)
        \end{array}
    \f]
    and added to the sinogram bin (r,t,p,segment). The view (r,p) is afterwards
    multiplied by
    \f[
        \left\{
         \begin{array}{r@{\quad:\quad}l}
          \frac{\Delta_{xy}}{\left|\cos\theta\right|} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
          \frac{\Delta_{xy}}{\left|\sin\theta\right|} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|\\
         \end{array}\right.
    \f]

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image. If running on a distributed memory system with
    n nodes, only every n\em th plane of the sinogram is processed.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::forward_proj3d_seg0(float * const img, float *sino,
                                   const unsigned short int subset,
                                   const unsigned short int z_start,
                                   const unsigned short int z_end,
                                   const unsigned short int proc,
                                   const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=view_size_padded)
    { float incx, incy, *view;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
      view=sino+RhoSamples_padded*(1+proc);
      if (fabsf(incy) > fabsf(incx))
       { float delxx0;

         delxx0=-sin_theta[t]/cos_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
         for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
              view+=RhoSamples_padded*num_procs)
          { float *ip, xx0;
                                         // calculate start point of projection
            xx0=image_center*(1.0f-delxx0)-rho_center*incx;
            ip=img+XYSamples_padded;
            for (unsigned short int y=0; y < XYSamples; y++,
                 ip+=XYSamples_padded,
                 xx0+=delxx0)
             { float x, *ip2;
               unsigned short int r;

               ip2=ip+(unsigned long int)(p+1)*image_plane_size_padded;
               for (x=xx0,
                    r=1; r <= RhoSamples; r++,
                    x+=incx)
                   // exclude bins in projection, that are not hit by the image
                if ((x >= (float)(mask[y].start-1)) &&
                    (x < (float)(mask[y].end+1)))
                 { signed short int xfloor;

                   xfloor=(signed short int)(x+1.0f)-1;
                   _linearInterpolation(view[r], x-(float)xfloor,
                                        ip2[xfloor+1], ip2[xfloor+2]);
                 }
             }
                                                // apply Jacobian to projection
            vecMulScalar(view, DeltaXY/fabsf(cos_theta[t]), view,
                         RhoSamples_padded);
          }
         continue;
       }
      float delyy0, *ip;

      delyy0=-cos_theta[t]/sin_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
      ip=img+(unsigned long int)(z_start+proc+1)*image_plane_size_padded;
      for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
           view+=RhoSamples_padded*num_procs,
           ip+=image_plane_size_padded*num_procs)
       { float yy0;
                                         // calculate start point of projection
         yy0=image_center*(1.0f-delyy0)-rho_center*incy;
         for (unsigned short int x=0; x < XYSamples; x++,
              yy0+=delyy0)
          { float y, *ip2;
            unsigned short int r;

            ip2=ip+x+1;
            for (y=yy0,
                 r=1; r <= RhoSamples; r++,
                 y+=incy)
                   // exclude bins in projection, that are not hit by the image
             { signed short int yfloor;
               bool hit=false;

               yfloor=(signed short int)(y+2.0f)-2;
               if ((yfloor >= -1) && (yfloor < XYSamples))
                { if (yfloor >= 0)
                   if ((x >= mask[yfloor].start) && (x <= mask[yfloor].end))
                    hit=true;
                  if (yfloor < XYSamples-1)
                   if ((x >= mask[yfloor+1].start) &&
                       (x <= mask[yfloor+1].end)) hit=true;
                }
               if (hit)
                { float *ip3;
                            // the two voxels that are hit by the LOR view[r,p]
                            // are image[x,yfloor,z] and image[x,yfloor+1,z]
                                  // calculate weights for linear interpolation
                  ip3=ip2+(unsigned long int)(yfloor+1)*
                          (unsigned long int)XYSamples_padded;
                  _linearInterpolation(view[r], y-(float)yfloor, ip3[0],
                                       ip3[XYSamples_padded]);
                }
             }
          }
                                               // apply Jacobian to projection
         vecMulScalar(view, DeltaXY/fabsf(sin_theta[t]), view,
                      RhoSamples_padded);
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the forward-projection of image into one subset of
           segment 0 with mode 1 LUTs.
    \param[in]     img         image data
    \param[in,out] sino        padded sinogram data in view mode
    \param[in]     subset      number of subset
    \param[in]     z_start     first planee of sinogram to calculate
    \param[in]     z_end       last plane+1 of sinogram to calculate
    \param[in]     lf          LUT for interpolation factors
    \param[in]     lidx        LUT for image coordinates
    \param[in]     lr          LUT for r indices
    \param[in]     proc        number of node
    \param[in]     num_procs   number of nodes in distributed memory system

    Calculate the forward-projection of the image into segment 0 for all
    azimuthal angles of one subset.
    The image value at the coordinate (x,y,z) is calculated by linear
    interpolation:
    \f[
        f=x-\lfloor x\rfloor \vee f=y-\lfloor y\rfloor
    \f]
    \f[
        c=(\lfloor x\rfloor,y) \vee c=(x,\lfloor y\rfloor)
    \f]
    \f[
        \mbox{image}(x,y,z)=
         (1-f)\cdot\mbox{image}(c,z)+f\cdot\mbox{image}(c+\mbox{offset},z)
    \f]
    and added to the sinogram bin (r,t,p,segment). The interpolation factors
    \f$f\f$ are stored in the \em lf LUT. The r coordinate of
    the sinogram is stored in the \em lr LUT and the \f$c\f$ coordinate pair of
    the image is stored in the \em lidx LUT.
    The view (r,p) is afterwards multiplied by
    \f[
        \left\{
         \begin{array}{r@{\quad:\quad}l}
          \frac{\Delta_{xy}}{\left|\cos\theta\right|} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
          \frac{\Delta_{xy}}{\left|\sin\theta\right|} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|\\
         \end{array}\right.
    \f]

    If running on a distributed memory system with n nodes, only every n\em th
    plane of the sinogram is processed.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::forward_proj3d_seg0_table1(float * const img, float *sino,
                                      const unsigned short int subset,
                                      const unsigned short int z_start,
                                      const unsigned short int z_end,
                                      const float * const lf,
                                      const uint32 * const lidx,
                                      const signed short int * const lr,
                                      const unsigned short int proc,
                                      const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=view_size_padded)
    { float *ip, factor, *view;
      unsigned short int offs;
                   // calculate offset to next image voxel in x- or y-direction
                   // and factor for jacobian
      if (fabsf(sin_theta[t]) < fabsf(cos_theta[t]))
       { offs=1;
         factor=DeltaXY/fabsf(cos_theta[t]);
       }
       else { offs=XYSamples_padded;
              factor=DeltaXY/fabsf(sin_theta[t]);
            }
      view=sino+RhoSamples_padded*(1+proc);
      ip=img+(unsigned long int)(z_start+proc+1)*image_plane_size_padded;
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
      for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
           view+=RhoSamples_padded*num_procs,
           ip+=image_plane_size_padded*(unsigned long int)num_procs)
       { for (unsigned long int ry=lut_pos[t]; ry < lut_pos[t+1]; ry++)
          { float *ptr;

            ptr=&ip[lidx[ry]];
            _linearInterpolation(view[lr[ry]], lf[ry], ptr[0], ptr[offs]);
          }
                                                // apply Jacobian to projection
         vecMulScalar(view, factor, view, RhoSamples_padded);
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the forward-projection of image into one subset of segment
           0 with mode 2 LUT.
    \param[in]     tfp         pointer to lookup-table
    \param[in]     img         image data
    \param[in,out] sino        padded sinogram data in view mode
    \param[in]     subset      number of suset
    \param[in]     z_start     first plane of view to calculate
    \param[in]     z_end       last plane+1 of view to calculate
    \param[in]     proc        number of node
    \param[in]     num_procs   number of nodes in distributed memory system

    Calculate the forward-projection of the image into segment 0 for all
    azimuthal angles of one subset. If
    \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate
    for each bin of the sinogram view (r,p) and line (y,z) of the image the
    coordinate x, where the LOR hits the image line. Otherwise we calculate
    for each bin of the sinogram view (r,p) and line (x,z) of the image the
    coordinate y, where the LOR hits the image line.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
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

    The image value at the coordinate (x,y,z) is calculated by linear
    interpolation for \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$:
    \f[
        x_1=x-\lfloor x\rfloor \qquad x_2=1-x_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         x_2\mbox{image}(\lfloor x\rfloor,y,z)+\\
         && x_1\mbox{image}(\lceil x\rceil,y,z)
        \end{array}
    \f]
    respectively for \f$\left|\sin\theta\right|\ge\left|\cos\theta\right|\f$:
    \f[
        y_1=y-\lfloor y\rfloor \qquad y_2=1-y_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         y_2\mbox{image}(x,\lfloor y\rfloor,z)+\\
         && y_1\mbox{image}(x,\lceil y\rceil,z)
        \end{array}
    \f]
    and added to the sinogram bin (r,t,p,segment). The view (r,p) is afterwards
    multiplied by
    \f[
        \left\{
         \begin{array}{r@{\quad:\quad}l}
          \frac{\Delta_{xy}}{\left|\cos\theta\right|} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
          \frac{\Delta_{xy}}{\left|\sin\theta\right|} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|\\
         \end{array}\right.
    \f]

    The code uses a lookup-table to skip bins that won't hit the cylindrical
    image. If running on a distributed memory system with n nodes, only every
    n\em th plane of the sinogram is processed.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::forward_proj3d_seg0_table2(tmaskline * const tfp,
                                      float * const img, float *sino,
                                      const unsigned short int subset,
                                      const unsigned short int z_start,
                                      const unsigned short int z_end,
                                      const unsigned short int proc,
                                      const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=view_size_padded)
    { float incx, incy, *view;
      unsigned long int idx;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
                    // pointer into look-up table with line-masks that excludes
                    // bins of the sinogram view that don't hit the image
      idx=(unsigned long int)t*(unsigned long int)XYSamples;
      view=sino+RhoSamples_padded*(1+proc);
      if (fabsf(incy) > fabsf(incx))
       { float delxx0;

         delxx0=-sin_theta[t]/cos_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
         for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
              view+=RhoSamples_padded*num_procs)
          { float *ip, xx0;
                                         // calculate start point of projection
            xx0=image_center*(1.0f-delxx0)-rho_center*incx;
            ip=img+XYSamples_padded;
            for (unsigned short int y=0; y < XYSamples; y++,
                 ip+=XYSamples_padded,
                 xx0+=delxx0)
             { float x, *ip2;
               unsigned short int r;

               ip2=ip+(unsigned long int)(p+1)*image_plane_size_padded;
               for (x=xx0+(float)tfp[idx+y].start*incx,
                   // exclude bins in projection, that are not hit by the image
                    r=tfp[idx+y].start+1; r <= tfp[idx+y].end+1; r++,
                    x+=incx)
                { signed short int xfloor;

                  xfloor=(signed short int)(x+1.0f)-1;
                            // the two voxels that are hit by the LOR view[r,p]
                            // are image[xfloor,y,z] and image[xfloor+1,y,z]
                                  // calculate weights for linear interpolation
                  _linearInterpolation(view[r], x-(float)xfloor, ip2[xfloor+1],
                                       ip2[xfloor+2]);
                }
             }
                                                // apply Jacobian to projection
            vecMulScalar(view, DeltaXY/fabsf(cos_theta[t]), view,
                         RhoSamples_padded);
          }
         continue;
       }
      float delyy0, *ip;

      delyy0=-cos_theta[t]/sin_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
      ip=img+(unsigned long int)(z_start+proc+1)*image_plane_size_padded;
      for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
           view+=RhoSamples_padded*num_procs,
           ip+=image_plane_size_padded*(unsigned long int)num_procs)
       { float yy0;
                                         // calculate start point of projection
         yy0=image_center*(1.0f-delyy0)-rho_center*incy;
         for (unsigned short int x=0; x < XYSamples; x++,
              yy0+=delyy0)
          { float y, *ip2;
            unsigned short int r;

            ip2=ip+x+1;
            for (y=yy0+(float)tfp[idx+x].start*incy,
                   // exclude bins in projection, that are not hit by the image
                 r=tfp[idx+x].start+1; r <= tfp[idx+x].end+1; r++,
                 y+=incy)
             { signed short int yfloor;
               float *ip3;

               yfloor=(signed short int)(y+1.0f)-1;
                            // the two voxels that are hit by the LOR view[r,p]
                            // are image[x,yfloor,z] and image[x,yfloor+1,z]
                                  // calculate weights for linear interpolation
               ip3=ip2+(unsigned long int)(yfloor+1)*
                       (unsigned long int)XYSamples_padded;
               _linearInterpolation(view[r], y-(float)yfloor, ip3[0],
                                    ip3[XYSamples_padded]);
             }
          }
                                                // apply Jacobian to projection
         vecMulScalar(view, DeltaXY/fabsf(sin_theta[t]), view,
                      RhoSamples_padded);
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the forward-projection of image into one subset of an
           oblique segment.
    \param[in]     img         image data
    \param[in,out] sino        padded sinogram data in view mode
    \param[in]     subset      number of suset
    \param[in]     segment     number of segment
    \param[in]     z_start     first plane of view to calculate
    \param[in]     z_end       last plane+1 of view to calculate
    \param[in]     proc        number of node
    \param[in]     num_procs   number of nodes in distributed memory system

    Calculate the forward-projection of the image into an oblique segment for
    all azimuthal angles of one suset.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate for
    each bin of the sinogram view (r,p) and plane y of the image the coordinate
    (x,z), where the LOR hits the image plane. Otherwise we calculate for each
    bin of the sinogram view (r,p) and plane x of the image the coordinate
    (y,z), where the LOR hits the image plane.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
    \f]
    \f[
        z=\left\{
         \begin{array}{r@{\quad:\quad}l}
          p+\left(\rho_c-r\right)
          \frac{\sin\phi\sin\theta\Delta_\rho}{\cos\phi\cos\theta\Delta_z}-
          \left(i_c-y\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\theta\cos\phi\Delta_z} &
          \left|\sin\theta\right|<\left|\cos\theta\right|\\
          p-\left(\rho_c-r\right)
          \frac{\sin\phi\cos\theta\Delta_\rho}{\cos\phi\sin\theta\Delta_z}+
          \left(i_c-x\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\phi\sin\theta\Delta_z} &
          \left|\sin\theta\right|\ge\left|\cos\theta\right|
         \end{array}\right.
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

    The image value at the coordinate (x,y,z) is calculated by bilinear
    interpolation for \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$:
    \f[
        x_1=x-\lfloor x\rfloor \qquad z_1=z-\lfloor z\rfloor \qquad
        x_2=1-x_1 \qquad z_2=1-z_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         z_2x_2\mbox{image}(\lfloor x\rfloor,y,\lfloor z\rfloor)+\\
         && z_2x_1\mbox{image}(\lceil x\rceil,y,\lfloor z\rfloor)+\\
         && z_1x_2\mbox{image}(\lfloor x\rfloor,y,\lceil z\rceil)+\\
         && z_1x_1\mbox{image}(\lceil x\rceil,y,\lceil z\rceil)
        \end{array}
    \f]
    respectively for \f$\left|\sin\theta\right|\ge\left|\cos\theta\right|\f$:
    \f[
        y_1=y-\lfloor y\rfloor \qquad z_1=z-\lfloor z\rfloor \qquad
        y_2=1-y_1 \qquad z_2=1-z_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         z_2y_2\mbox{image}(x,\lfloor y\rfloor,\lfloor z\rfloor)+\\
         && z_2y_1\mbox{image}(x,\lceil y\rceil,\lfloor z\rfloor)+\\
         && z_1y_2\mbox{image}(x,\lfloor y\rfloor,\lceil z\rceil)+\\
         && z_1y_1\mbox{image}(x,\lceil y\rceil,\lceil z\rceil)
        \end{array}
    \f]
    and added to the sinogram bin (r,t,p,segment). The view (r,p) is afterwards
    multiplied by
    \f[
        \left\{
         \begin{array}{r@{\quad:\quad}l}
          \frac{\Delta_{xy}}{\left|\cos\theta\cos\phi\right|} &
           \sin\theta<\cos\theta\\
          \frac{\Delta_{xy}}{\left|\sin\theta\cos\phi\right|} &
           \sin\theta\ge\cos\theta\\
         \end{array}\right.
    \f]

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image. If running on a distributed memory system with
    n nodes, only every n\em th plane of the sinogram is processed.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::forward_proj3d_oblique(float * const img, float *sino,
                                      const unsigned short int subset,
                                      const unsigned short int segment,
                                      const unsigned short int z_start,
                                      const unsigned short int z_end,
                                      const unsigned short int proc,
                                      const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=view_size_padded)
    { float incx, incy, *view;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
      view=sino+RhoSamples_padded*(1+proc);
      if (fabsf(incy) > fabsf(incx))
       { float delxx0, tmp;

         delxx0=-sin_theta[t]/cos_theta[t];
         tmp=1.0f/(cos_theta[t]*plane_separation);
                               // follow sinogram view through image volume and
                               // calculate weights by bilinear interpolation
         for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
              view+=RhoSamples_padded*num_procs)
          { float *ip, incz, zz0, delzz0, xx0, tanphi, sinphi;
            unsigned short int v_idx;
                                    // get angle of sinogram plane and Jacobian
            calculatePlaneAngle(segment, p, &sinphi, &tanphi, &v_idx);
                                         // calculate start point of projection
            xx0=image_center*(1.0f-delxx0)-rho_center*incx;
            incz=-tanphi*BinSizeRho*sin_theta[t]*tmp;
            delzz0=DeltaXY*tanphi*tmp;
            zz0=(float)p-rho_center*incz-image_center*delzz0;
            ip=img+XYSamples_padded;
            for (unsigned short int y=0; y < XYSamples; y++,
                 ip+=XYSamples_padded,
                 xx0+=delxx0,
                 zz0+=delzz0)
             { float x, z;
               unsigned short int r;

               for (x=xx0,
                    z=zz0,
                    r=1; r <= RhoSamples; r++,
                    x+=incx,
                    z+=incz)
                   // exclude bins in projection, that are not hit by the image
                if ((x >= (float)(mask[y].start-1)) &&
                    (x < (float)(mask[y].end+1)))
                 if ((z >= -1.0f) && (z < (float)axis_slices[0]))
                  { signed short int xfloor, zfloor;
                    float *ip2;

                    xfloor=(signed short int)(x+1.0f)-1;
                    zfloor=(signed short int)(z+2.0f)-2;
                       // the four voxels that are hit by the LOR view[r,p] are
                       // image[xfloor,y,zfloor], image[xfloor+1,y,zfloor],
                       // image[xfloor,y,zfloor+1] and
                       // image[xfloor+1,y,zfloor+1]
                                // calculate weights for bilinear interpolation
                    ip2=ip+
                        (unsigned long int)(zfloor+1)*image_plane_size_padded+
                        xfloor+1;
                    _bilinearInterpolation(view[r], x-(float)xfloor,
                                           z-(float)zfloor, ip2[0], ip2[1],
                                           ip2[image_plane_size_padded],
                                           ip2[image_plane_size_padded+1]);
                  }
             }
                                                // apply Jacobian to projection
            vecMulScalar(view, DeltaXY/fabsf(cos_theta[t]*vcos_phi[v_idx]),
                         view, RhoSamples_padded);
          }
         continue;
       }
      float delyy0, tmp;

      delyy0=-cos_theta[t]/sin_theta[t];
      tmp=1.0f/(sin_theta[t]*plane_separation);
                               // follow sinogram view through image volume and
                               // calculate weights by bilinear interpolation
      for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
           view+=RhoSamples_padded*num_procs)
       { float incz, zz0, delzz0, sinphi, tanphi, yy0;
         unsigned short int v_idx;
                                    // get angle of sinogram plane and Jacobian
         calculatePlaneAngle(segment, p, &sinphi, &tanphi, &v_idx);
                                         // calculate start point of projection
         yy0=image_center*(1.0f-delyy0)-rho_center*incy;
         incz=tanphi*BinSizeRho*cos_theta[t]*tmp;
         delzz0=-DeltaXY*tanphi*tmp;
         zz0=(float)p-rho_center*incz-image_center*delzz0;
         for (unsigned short int x=0; x < XYSamples; x++,
              yy0+=delyy0,
              zz0+=delzz0)
          { float y, z;
            unsigned short int r;

            for (y=yy0,
                 z=zz0,
                 r=1; r <= RhoSamples; r++,
                 y+=incy,
                 z+=incz)
                   // exclude bins in projection, that are not hit by the image
             if ((z >= -1.0f) && (z < (float)axis_slices[0]))
              { signed short int yfloor;
                bool hit=false;

                yfloor=(signed short int)(y+2.0f)-2;
                if ((yfloor >= -1) && (yfloor < XYSamples))
                 { if (yfloor >= 0)
                    if ((x >= mask[yfloor].start) &&
                        (x <= mask[yfloor].end)) hit=true;
                   if (yfloor < XYSamples-1)
                    if ((x >= mask[yfloor+1].start) &&
                       (x <= mask[yfloor+1].end)) hit=true;
                 }
                if (hit)
                 { signed short int zfloor;
                   float *ip;

                   zfloor=(signed short int)(z+2.0f)-2;
                       // the four voxels that are hit by the LOR view[r,p] are
                       // image[x,yfloor,zfloor], image[x,yfloor+1,zfloor],
                       // image[x,yfloor,zfloor+1] and
                       // image[x,yfloor+1,zfloor+1]
                                // calculate weights for bilinear interpolation
                   ip=img+
                      (unsigned long int)(zfloor+1)*image_plane_size_padded+
                      (unsigned long int)(yfloor+1)*
                      (unsigned long int)XYSamples_padded+x+1;
                   _bilinearInterpolation(view[r], y-(float)yfloor,
                                 z-(float)zfloor, ip[0], ip[XYSamples_padded],
                                 ip[image_plane_size_padded],
                                 ip[image_plane_size_padded+XYSamples_padded]);
                 }
              }
          }
                                                // apply Jacobian to projection
         vecMulScalar(view, DeltaXY/fabsf(sin_theta[t]*vcos_phi[v_idx]), view,
                      RhoSamples_padded);
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the forward-projection of image into one subset of an
           oblique segment with mode 1 LUTs.
    \param[in]     img         image data
    \param[in,out] sino        padded sinogram data in view mode
    \param[in]     subset      number of suset
    \param[in]     segment     number of segment
    \param[in]     z_start     first plane of view to calculate
    \param[in]     z_end       last plane+1 of view to calculate
    \param[in]     lf          LUT for interpolation factors
    \param[in]     lidx        LUT for image coordinates
    \param[in]     lr          LUT for r indices
    \param[in]     lxy         LUT for x/y coordinate
    \param[in]     proc        number of node
    \param[in]     num_procs   number of nodes in distributed memory system

    Calculate the forward-projection of the image into an oblique segment for
    all azimuthal angles of one suset with mode 1 LUTs.
    We calculate for each bin of the sinogram view (r,p) and plane x or y of
    the image the coordinate (z), where the LOR hits the image plane.
    \f[
        z=\left\{
         \begin{array}{r@{\quad:\quad}l}
          p+\left(\rho_c-r\right)
          \frac{\sin\phi\sin\theta\Delta_\rho}{\cos\phi\cos\theta\Delta_z}-
          \left(i_c-y\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\theta\cos\phi\Delta_z} &
          \left|\sin\theta\right|<\left|\cos\theta\right|\\
          p-\left(\rho_c-r\right)
          \frac{\sin\phi\cos\theta\Delta_\rho}{\cos\phi\sin\theta\Delta_z}+
          \left(i_c-x\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\phi\sin\theta\Delta_z} &
          \left|\sin\theta\right|\ge\left|\cos\theta\right|
         \end{array}\right.
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

    The image value at the coordinate (x,y,z) is calculated by bilinear
    interpolation:
    \f[
        f=x-\lfloor x\rfloor \vee f=y-\lfloor y\rfloor
        \qquad z_1=z-\lfloor z\rfloor \qquad z_2=1-z_1
    \f]
    \f[
        c=(\lfloor x\rfloor,y) \vee c=(x,\lfloor y\rfloor)
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         z_2(1-f)\cdot\mbox{image}(c,\lfloor z\rfloor)+
         z_2f\cdot\mbox{image}(c+\mbox{offset},\lfloor z\rfloor)+\\
         && z_1(1-f)\cdot\mbox{image}(c,\lceil z\rceil)+
            z_1f\cdot\mbox{image}(c+\mbox{offset},\lceil z\rceil)
        \end{array}
    \f]
    and added to the sinogram bin (r,t,p,segment). The interpolation factors
    \f$f\f$ are stored in the \em lf LUT. The r coordinate of the sinogram is
    stored in the \em lr LUT and the \f$c\f$ coordinate pair of the image is
    stored in the \em lidx LUT. The \f$x\f$ or \f$y\f$ coordinate that is
    needed for the calculation of \f$z\f$ is stored in the \em lxy LUT. The
    view (r,p) is afterwards multiplied by
    \f[
        \left\{
         \begin{array}{r@{\quad:\quad}l}
          \frac{\Delta_{xy}}{\left|\cos\theta\cos\phi\right|} &
           \sin\theta<\cos\theta\\
          \frac{\Delta_{xy}}{\left|\sin\theta\cos\phi\right|} &
           \sin\theta\ge\cos\theta\\
         \end{array}\right.
    \f]

    If running on a distributed memory system with n nodes, only every n\em th
    plane of the sinogram is processed.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::forward_proj3d_oblique_table1(float * const img, float *sino,
                                      const unsigned short int subset,
                                      const unsigned short int segment,
                                      const unsigned short int z_start,
                                      const unsigned short int z_end,
                                      const float * const lf,
                                      const uint32 * const lidx,
                                      const signed short int * const lr,
                                      const signed short int * const lxy,
                                      const unsigned short int proc,
                                      const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=view_size_padded)
    { float fact1, fact2, tmp, *view;
      unsigned short int offs;
                   // calculate offset to next image voxel in x- or y-direction
                   // and factor for jacobian
      if (fabs(sin_theta[t]) < fabsf(cos_theta[t]))
       { offs=1;
         fact1=-sin_theta[t];
         fact2=cos_theta[t];
         tmp=DeltaXY/(fact2*plane_separation);
       }
       else { offs=XYSamples_padded;
              fact1=cos_theta[t];
              fact2=sin_theta[t];
              tmp=-DeltaXY/(fact2*plane_separation);
            }
      fact1*=BinSizeRho/(fact2*plane_separation);
      view=sino+RhoSamples_padded*(1+proc);
                               // follow sinogram view through image volume and
                               // calculate weights by bilinear interpolation
      for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
           view+=RhoSamples_padded*num_procs)
       { float incz, zz0, delzz0, tanphi, sinphi;
         unsigned short int v_idx;
                                    // get angle of sinogram plane and Jacobian
         calculatePlaneAngle(segment, p, &sinphi, &tanphi, &v_idx);
                                         // calculate start point of projection
         incz=tanphi*fact1;
         delzz0=tanphi*tmp;
         zz0=(float)p-rho_center*incz-image_center*delzz0;

         for (unsigned long int ry=lut_pos[t]; ry < lut_pos[t+1]; ry++)
          { signed short int zfloor;
            float *ip, z;
            unsigned short int rp1;

            rp1=lr[ry];
            z=zz0+(float)lxy[ry]*delzz0+(float)(rp1-1)*incz;
            zfloor=(signed short int)(z+2.0f)-2;
                                        // calculate for bilinear interpolation
            ip=&img[(unsigned long int)(zfloor+1)*image_plane_size_padded+
                    lidx[ry]];
            _bilinearInterpolation(view[rp1], lf[ry], z-(float)zfloor,
                                   ip[0], ip[offs],
                                   ip[image_plane_size_padded],
                                   ip[image_plane_size_padded+offs]);
          }
                                                // apply Jacobian to projection
         vecMulScalar(view, DeltaXY/fabsf(fact2*vcos_phi[v_idx]), view,
                      RhoSamples_padded);
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the forward-projection of image into one subset of an
           oblique segment with mode 2 LUT.
    \param[in]     tfp         pointer to lookup-table
    \param[in]     img         image data
    \param[in,out] sino        padded sinogram data in view mode
    \param[in]     subset      number of suset
    \param[in]     segment     number of segment
    \param[in]     z_start     first plane of view to calculate
    \param[in]     z_end       last plane+1 of view to calculate
    \param[in]     proc        number of node
    \param[in]     num_procs   number of nodes in distributed memory system

    Calculate the forward-projection of the image into one subset of an oblique
    segment with mode 2 LUT.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate for
    each bin of the sinogram view (r,p) and plane y of the image the coordinate
    (x,z), where the LOR hits the image plane. Otherwise we calculate for each
    bin of the sinogram view (r,p) and plane x of the image the coordinate
    (y,z), where the LOR hits the image plane.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
    \f]
    \f[
        z=\left\{
         \begin{array}{r@{\quad:\quad}l}
          p+\left(\rho_c-r\right)
          \frac{\sin\phi\sin\theta\Delta_\rho}{\cos\phi\cos\theta\Delta_z}-
          \left(i_c-y\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\theta\cos\phi\Delta_z} &
          \left|\sin\theta\right|<\left|\cos\theta\right|\\
          p-\left(\rho_c-r\right)
          \frac{\sin\phi\cos\theta\Delta_\rho}{\cos\phi\sin\theta\Delta_z}+
          \left(i_c-x\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\phi\sin\theta\Delta_z} &
          \left|\sin\theta\right|\ge\left|\cos\theta\right|
         \end{array}\right.
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

    The image value at the coordinate (x,y,z) is calculated by bilinear
    interpolation for \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$:
    \f[
        x_1=x-\lfloor x\rfloor \qquad z_1=z-\lfloor z\rfloor \qquad
        x_2=1-x_1 \qquad z_2=1-z_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         z_2x_2\mbox{image}(\lfloor x\rfloor,y,\lfloor z\rfloor)+\\
         && z_2x_1\mbox{image}(\lceil x\rceil,y,\lfloor z\rfloor)+\\
         && z_1x_2\mbox{image}(\lfloor x\rfloor,y,\lceil z\rceil)+\\
         && z_1x_1\mbox{image}(\lceil x\rceil,y,\lceil z\rceil)
        \end{array}
    \f]
    respectively for \f$\left|\sin\theta\right|\ge\left|\cos\theta\right|\f$:
    \f[
        y_1=y-\lfloor y\rfloor \qquad z_1=z-\lfloor z\rfloor \qquad
        y_2=1-y_1 \qquad z_2=1-z_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         z_2y_2\mbox{image}(x,\lfloor y\rfloor,\lfloor z\rfloor)+\\
         && z_2y_1\mbox{image}(x,\lceil y\rceil,\lfloor z\rfloor)+\\
         && z_1y_2\mbox{image}(x,\lfloor y\rfloor,\lceil z\rceil)+\\
         && z_1y_1\mbox{image}(x,\lceil y\rceil,\lceil z\rceil)
        \end{array}
    \f]
    and added to the sinogram bin (r,t,p,segment). The view (r,p) is afterwards
    multiplied by
    \f[
        \left\{
         \begin{array}{r@{\quad:\quad}l}
          \frac{\Delta_{xy}}{\left|\cos\theta\cos\phi\right|} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
          \frac{\Delta_{xy}}{\left|\sin\theta\cos\phi\right|} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|\\
         \end{array}\right.
    \f]

    The code uses a lookup-table to skip bins that won't hit the cylindrical
    image. If running on a distributed memory system with n nodes, only every
    n\em th plane of the sinogram is processed.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::forward_proj3d_oblique_table2(tmaskline * const tfp,
                                      float * const img, float *sino,
                                      const unsigned short int subset,
                                      const unsigned short int segment,
                                      const unsigned short int z_start,
                                      const unsigned short int z_end,
                                      const unsigned short int proc,
                                      const unsigned short int num_procs) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        sino+=view_size_padded)
    { float incx, incy, *view;
      unsigned long int idx;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
                    // pointer into look-up table with line-masks that excludes
                    // bins of the sinogram view that don't hit the image
      idx=(unsigned long int)
          ((unsigned long int)t*
           (unsigned long int)axis_slices[(segment+1)/2]/2+
           z_start-z_bottom[segment])*(unsigned long int)XYSamples;
      view=sino+RhoSamples_padded*(1+proc);
      if (fabsf(incy) > fabsf(incx))
       { float delxx0, tmp;

         delxx0=-sin_theta[t]/cos_theta[t];
         tmp=1.0f/(cos_theta[t]*plane_separation);
                               // follow sinogram view through image volume and
                               // calculate weights by bilinear interpolation
         for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
              view+=RhoSamples_padded*num_procs,
              idx+=XYSamples*(num_procs-1))
          { float *ip, incz, zz0, delzz0, xx0, sinphi, tanphi;
            unsigned short int v_idx;
                                    // get angle of sinogram plane and Jacobian
            calculatePlaneAngle(segment, p, &sinphi, &tanphi, &v_idx);
                                         // calculate start point of projection
            xx0=image_center*(1.0f-delxx0)-rho_center*incx;
            incz=-tanphi*BinSizeRho*sin_theta[t]*tmp;
            delzz0=DeltaXY*tanphi*tmp;
            zz0=(float)p-rho_center*incz-image_center*delzz0;
            ip=img+XYSamples_padded;
            for (unsigned short int y=0; y < XYSamples; y++,
                 ip+=XYSamples_padded,
                 xx0+=delxx0,
                 zz0+=delzz0,
                 idx++)
             { float x, z;
               unsigned short int r;

               for (x=xx0+(float)tfp[idx].start*incx,
                    z=zz0+(float)tfp[idx].start*incz,
                   // exclude bins in projection, that are not hit by the image
                    r=tfp[idx].start+1; r <= tfp[idx].end+1; r++,
                    x+=incx,
                    z+=incz)
                { signed short int xfloor, zfloor;
                  float *ip2;

                  xfloor=(signed short int)(x+1.0f)-1;
                  zfloor=(signed short int)(z+2.0f)-2;
                       // the four voxels that are hit by the LOR view[r,p] are
                       // image[xfloor,y,zfloor], image[xfloor+1,y,zfloor],
                       // image[xfloor,y,zfloor+1] and
                       // image[xfloor+1,y,zfloor+1]
                                // calculate weights for bilinear interpolation
                  ip2=ip+(unsigned long int)(zfloor+1)*image_plane_size_padded+
                      xfloor+1;
                  _bilinearInterpolation(view[r], x-(float)xfloor,
                                         z-(float)zfloor, ip2[0], ip2[1],
                                         ip2[image_plane_size_padded],
                                         ip2[image_plane_size_padded+1]);
                }
             }
                                                // apply Jacobian to projection
            vecMulScalar(view, DeltaXY/fabsf(cos_theta[t]*vcos_phi[v_idx]),
                         view, RhoSamples_padded);
          }
         continue;
       }
      float delyy0, tmp;

      delyy0=-cos_theta[t]/sin_theta[t];
      tmp=1.0f/(sin_theta[t]*plane_separation);
                               // follow sinogram view through image volume and
                               // calculate weights by bilinear interpolation
      for (unsigned short int p=z_start+proc; p < z_end; p+=num_procs,
           view+=RhoSamples_padded*num_procs,
           idx+=XYSamples*(num_procs-1))
       { float incz, zz0, delzz0, yy0, sinphi, tanphi;
         unsigned short int v_idx;
                                    // get angle of sinogram plane and Jacobian
         calculatePlaneAngle(segment, p, &sinphi, &tanphi, &v_idx);
                                         // calculate start point of projection
         yy0=image_center*(1.0f-delyy0)-rho_center*incy;
         incz=tanphi*BinSizeRho*cos_theta[t]*tmp;
         delzz0=-DeltaXY*tanphi*tmp;
         zz0=(float)p-rho_center*incz-image_center*delzz0;
         for (unsigned short int x=0; x < XYSamples; x++,
              yy0+=delyy0,
              zz0+=delzz0,
              idx++)
          { float y, z;
            unsigned short int r;

            for (y=yy0+(float)tfp[idx].start*incy,
                 z=zz0+(float)tfp[idx].start*incz,
                   // exclude bins in projection, that are not hit by the image
                 r=tfp[idx].start+1; r <= tfp[idx].end+1; r++,
                 y+=incy,
                 z+=incz)
             { signed short int yfloor, zfloor;
               float *ip;

               yfloor=(signed short int)(y+1.0f)-1;
               zfloor=(signed short int)(z+2.0f)-2;
                       // the four voxels that are hit by the LOR view[r,p] are
                       // image[x,yfloor,zfloor], image[x,yfloor+1,zfloor],
                       // image[x,yfloor,zfloor+1] and
                       // image[x,yfloor+1,zfloor+1]
                                // calculate weights for bilinear interpolation
               ip=img+(unsigned long int)(zfloor+1)*image_plane_size_padded+
                      (unsigned long int)(yfloor+1)*
                      (unsigned long int)XYSamples_padded+x+1;
               _bilinearInterpolation(view[r], y-(float)yfloor,
                                 z-(float)zfloor, ip[0], ip[XYSamples_padded],
                                 ip[image_plane_size_padded],
                                 ip[image_plane_size_padded+XYSamples_padded]);
             }
          }
                                                // apply Jacobian to projection
         vecMulScalar(view, DeltaXY/fabsf(sin_theta[t]*vcos_phi[v_idx]), view,
                      RhoSamples_padded);
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
void FwdPrj3D::initMask(const float radius_fov)
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
/*! \brief Calculate mode 1 LUTs for forward-projection.

    Calculate mode 1 LUTs for forward-projection.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate for
    each bin of the sinogram view (r,p) and plane y of the image the coordinate
    (x,z), where the LOR hits the image plane. Otherwise we calculate for each
    bin of the sinogram view (r,p) and plane x of the image the coordinate
    (y,z), where the LOR hits the image plane.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
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

    For \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ the interpolation
    factor \f$x_1\f$
    \f[
        x_1=x-\lfloor x\rfloor
    \f]
    and the coordinate pair \f$(\lfloor x\rfloor,y)\f$,
    respectively for \f$\left|\sin\theta\right|\ge\left|\cos\theta\right|\f$
    the interpolation factor \f$y_1\f$
    \f[
        y_1=y-\lfloor y\rfloor
    \f]
    and the coordinate pair \f$(x,\lfloor y\rfloor)\f$ are stored in the \em lf
    respectively \em lidx lookup-tables. The sinogram coordinate r is stored in
    the \em lr LUT and the x/y coordinate in the \em lxy LUT.

    The lookup-tables are sorted, based on the \em lidx coordinate pairs to
    increase data locality during forward-projection.

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::initTable1()
 { float *lf_tmp, *lf;
   uint32 *lidx_tmp, *lidx;
   signed short int *lr_tmp, *lr, *lxy_tmp=NULL, *lxy;
   double size;
   unsigned long int tabsize, taboffs, count=0, bufsize;
   unsigned short int lut_fact_tmp, lut_idx_tmp, lut_r_tmp, lut_xy_tmp, sf;
   std::vector <std::vector <float> > lf_buf;
   std::vector <std::vector <unsigned short int> > lr_buf, lxy_buf;
   std::vector <bool> lidx_buf;

   if (axes > 1) sf=2;
    else sf=1;
                                                   // size of temporary buffers
   bufsize=(unsigned long int)XYSamples_padded*
           (unsigned long int)XYSamples_padded;

   taboffs=(unsigned long int)RhoSamples*(unsigned long int)XYSamples;
                                               // maximum size of lookup-tables
   tabsize=(unsigned long int)ThetaSamples*taboffs;
   size=tabsize*(sizeof(float)+sizeof(uint32)+
                 sf*sizeof(signed short int))/1024.0;
   Logging::flog()->logMsg("create mode 1 lookup-table for forward-projector: "
                           "#1 MByte", loglevel+1)->arg(size/1024.0);
                                    // allocate lookup-tables with maximum size
   lf_tmp=MemCtrl::mc()->createFloat(tabsize, &lut_fact_tmp, "fwlt1",
                                     loglevel+1);
   lidx_tmp=MemCtrl::mc()->createUInt(tabsize, &lut_idx_tmp, "fwlt1",
                                      loglevel+1);
   lr_tmp=MemCtrl::mc()->createSInt(tabsize, &lut_r_tmp, "fwlt1", loglevel+1);
   if (axes > 1)
    lxy_tmp=MemCtrl::mc()->createSInt(tabsize, &lut_xy_tmp, "fwlt1",
                                      loglevel+1);
                           // indices for start of LUT for each azimuthal angle
   lut_pos.resize(ThetaSamples+1);
   lut_pos[0]=0;
   lf_buf.resize(bufsize);
   lr_buf.resize(bufsize);
   if (axes > 1) lxy_buf.resize(bufsize);
   for (unsigned short int t=0; t < ThetaSamples; t++)
    { float incx, incy;
      unsigned long int i;

      lidx_buf.assign(bufsize, false);
      for (i=0; i < bufsize; i++)
       { lf_buf[i].clear();
         lr_buf[i].clear();
         if (axes > 1) lxy_buf[i].clear();
       }
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
      if (fabsf(incy) > fabsf(incx))
       { float delxx0, xx0;
 
         delxx0=-sin_theta[t]/cos_theta[t];
                                         // calculate start point of projection
         xx0=image_center*(1.0f-delxx0)-rho_center*incx;
         for (unsigned short int y=0; y < XYSamples; y++,
              xx0+=delxx0)
          { float x;
            unsigned short int r;

            for (x=xx0,
                 r=0; r < RhoSamples; r++,
                 x+=incx)
                   // exclude bins in projection, that are not hit by the image
             if ((x >= (float)(mask[y].start-1)) &&
                 (x < (float)(mask[y].end+1)))
              { signed short int xfloor;
                unsigned long int idx;

                xfloor=(signed short int)(x+1.0f)-1;
                idx=xfloor+1+(y+1)*XYSamples_padded;
                if (idx > 0)
                 { lidx_buf[idx]=true;                         // index is used
                   // store values in LUT; indices might be used more than one,
                   // therefore each index has a vector of values
                   lf_buf[idx].push_back(x-(float)xfloor);
                   lr_buf[idx].push_back(r+1);
                   if (axes > 1) lxy_buf[idx].push_back(y);
                 }
              }
          }
       }
       else { float delyy0, yy0;

              delyy0=-cos_theta[t]/sin_theta[t];
                                         // calculate start point of projection
              yy0=image_center*(1.0f-delyy0)-rho_center*incy;
              for (unsigned short int x=0; x < XYSamples; x++,
                   yy0+=delyy0)
               { float y;
                 unsigned short int r;

                 for (y=yy0,
                      r=0; r < RhoSamples; r++,
                      y+=incy)
                   // exclude bins in projection, that are not hit by the image
                  { signed short int yfloor;
                    bool hit=false;

                    yfloor=(signed short int)(y+2.0f)-2;
                    if ((yfloor >= -1) && (yfloor < XYSamples))
                     { if (yfloor >= 0)
                        if ((x >= mask[yfloor].start) &&
                            (x <= mask[yfloor].end)) hit=true;
                       if (yfloor < XYSamples-1)
                        if ((x >= mask[yfloor+1].start) &&
                            (x <= mask[yfloor+1].end)) hit=true;
                     }
                    if (hit) { unsigned long int idx;

                               idx=(unsigned long int)(yfloor+1)*
                                   (unsigned long int)XYSamples_padded+x+1;
                               if (idx > 0)
                                { lidx_buf[idx]=true;          // index is used
                   // store values in LUT; indices might be used more than one,
                   // therefore each index has a vector of values
                                  lf_buf[idx].push_back(y-(float)yfloor);
                                  lr_buf[idx].push_back(r+1);
                                  if (axes > 1) lxy_buf[idx].push_back(x);
                                }
                             }
                  }
               }
            }
                // copy all used indices with their values into the sorted LUTs
      for (i=0; i < lidx_buf.size(); i++)
       if (lidx_buf[i])                                      // is index used ?
        for (unsigned short int j=0; j < lf_buf[i].size(); j++)
         { lidx_tmp[count]=i;
           lf_tmp[count]=lf_buf[i][j];
           lr_tmp[count]=lr_buf[i][j];
           if (axes > 1) lxy_tmp[count]=lxy_buf[i][j];
           count++;
         }
      lut_pos[t+1]=count;      // store start position for next azimuthal angle
    }
                                                        // shrink lookup tables
   tabsize=lut_pos[ThetaSamples];
   size=tabsize*(sizeof(float)+sizeof(uint32)+
                 sf*sizeof(signed short int))/1024.0;
   Logging::flog()->logMsg("shrink mode 1 lookup-table for forward-projector: "
                           "#1 MByte", loglevel+1)->arg(size/1024.0);
   lr=MemCtrl::mc()->createSInt(tabsize, &lut_r, "fwlt1", loglevel+1);
   memcpy(lr, lr_tmp, tabsize*sizeof(signed short int));
   MemCtrl::mc()->put(lut_r);
   MemCtrl::mc()->put(lut_r_tmp);
   MemCtrl::mc()->deleteBlock(&lut_r_tmp);

   if (axes > 1)
    { lxy=MemCtrl::mc()->createSInt(tabsize, &lut_xy, "fwlt1", loglevel+1);
      memcpy(lxy, lxy_tmp, tabsize*sizeof(signed short int));
      MemCtrl::mc()->put(lut_xy);
      MemCtrl::mc()->put(lut_xy_tmp);
      MemCtrl::mc()->deleteBlock(&lut_xy_tmp);
    }

   lf=MemCtrl::mc()->createFloat(tabsize, &lut_fact, "fwlt1", loglevel+1);
   memcpy(lf, lf_tmp, tabsize*sizeof(float));
   MemCtrl::mc()->put(lut_fact);
   MemCtrl::mc()->put(lut_fact_tmp);
   MemCtrl::mc()->deleteBlock(&lut_fact_tmp);

   lidx=MemCtrl::mc()->createUInt(tabsize, &lut_idx, "fwlt1", loglevel+1);
   memcpy(lidx, lidx_tmp, tabsize*sizeof(uint32));
   MemCtrl::mc()->put(lut_idx);
   MemCtrl::mc()->put(lut_idx_tmp);
   MemCtrl::mc()->deleteBlock(&lut_idx_tmp);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize mode 2 LUT for forward-projection.
    \param[in] max_threads   maximum number of parallel threads to use

    Initialize mode 2 LUT for forward-projection. This method is
    multi-threaded. Each thread calculates the lookup-table for a range of
    sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::initTable2(const unsigned short int max_threads)
 { double size;
   std::string str;
   unsigned long int pairs;
                                                     // calculate size of table
   pairs=(unsigned long int)ThetaSamples*
         (unsigned long int)(axes_slices-axis_slices[0]+1)*
         (unsigned long int)XYSamples;
   size=pairs*sizeof(tmaskline)/1024.0;
   if (axes > 1) str=toString(size/1024.0)+" MByte";
    else str=toString(size)+" KByte";
   Logging::flog()->logMsg("create mode 2 look-up table for forward-projector:"
                           " "+str, loglevel);
   table_fwd.assign(segments, MemCtrl::MAX_BLOCK);
   for (unsigned short int segment=0; segment < segments; segment++)
    { tmaskline *tfp;
      unsigned long int size;

      size=(unsigned long int)ThetaSamples*(unsigned long int)XYSamples;
      if (segment > 0) size*=(unsigned long int)axis_slices[(segment+1)/2]/2;
      tfp=(tmaskline *)MemCtrl::mc()->createSInt(size*2, &table_fwd[segment],
                                                 "fplt", loglevel);
      searchIndicesFwd(tfp, segment, max_threads);
      MemCtrl::mc()->put(table_fwd[segment]);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate one subset of maximum intensity projection of image.
    \param[in]  img           image data
    \param[out] mip           MIP data
    \param[in]  subset        number of subset
    \param[in]  max_threads   maximum number of parallel threads to use

    Calculate one subset of the maximum intensity projection of an image.
    This method is multi-threaded. Every thread processes a range of projection
    planes.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::mip_proj(float * const img, float *mip,
                        const unsigned short int subset,
                        const unsigned short int max_threads)
 { try
   { std::vector <tthread_id> tid;
     std::vector <bool> thread_running(0);
     unsigned short int threads=0, t;
     void *thread_result;

     try
     { tmaskline *tfp=NULL;
       std::vector <FwdPrj3D::tthread_params> tp;
                                                        // initialize mip image
       memset(mip, 0, subset_size_padded*sizeof(float));
                                                    // number of threads to use
       threads=std::min(max_threads, axis_slices[0]);
       if (threads > 1)
        { tid.resize(threads);                          // table for thread-IDs
          tp.resize(threads);                    // table for thread parameters
          thread_running.assign(threads, false);     // table for thread status
        }
       if (table_fwd.size() > 0)
        tfp=(tmaskline *)MemCtrl::mc()->getSIntRO(table_fwd[0], loglevel);
                                   // forward-project all angles of this subset
       if (threads == 1)                       // sequential forward-projection
        if (table_fwd.size() > 0)                                  // use LUT ?
         mip_proj_table2(tfp, img, mip, subset, 0, axis_slices[0]);
         else mip_proj(img, mip, subset, 0, axis_slices[0]);
        else { unsigned short int slices, z_start, i;
                                                 // parallel forward-projection
               slices=z_top[0]-z_bottom[0]+1;
               for (z_start=z_bottom[0],
                    t=threads,
                    i=0; i < threads; i++,
                    t--)
                {                                // calculate thread parameters
                  tp[i].object=this;
                  tp[i].image=img;
                  tp[i].proj=mip+(unsigned long int)z_start*
                                 (unsigned long int)RhoSamples_padded;
                  tp[i].subset=subset;
                  tp[i].tfp=tfp;
                  tp[i].z_start=z_start;                // first slice of image
                  tp[i].z_end=z_start+slices/t;       // last slice +1 of image
                  tp[i].lut_mode=lut_mode;
#ifdef XEON_HYPERTHREADING_BUG
                  tp[i].thread_number=i;
#endif
                  thread_running[i]=true;
                                                                // start thread
                  ThreadCreate(&tid[i], &executeThread_MIPproject, &tp[i]);
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
       if (table_fwd.size() > 0) MemCtrl::mc()->put(table_fwd[0]);
     }
     catch (...)                                // handle exceptions of threads
      { thread_result=NULL;
        for (t=0; t < threads; t++)      // wait until all threads are finished
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
        if (table_fwd.size() > 0) MemCtrl::mc()->put(table_fwd[0]);
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)             //  handle exceptions of this method
    { std::string err_str;
      unsigned long int err_code;

      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate maximum intensity projection of image into one subset.
    \param[in]     img       image data
    \param[in,out] mip       padded mip data
    \param[in]     subset    number of subset
    \param[in]     z_start   first plane of view to calculate
    \param[in]     z_end     last plane+1 of view to calculate

    Calculate the maximum intensity projection of the image into one subset.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate for
    each bin of the sinogram view (r,p) and line (y,z) of the image the
    coordinate x, where the LOR hits the image line. Otherwise we calculate for
    each bin of the sinogram view (r,p) and line (x,z) of the image the
    coordinate y, where the LOR hits the image line.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
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

    The image value at the coordinate (x,y,z) is calculated by linear
    interpolation for \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$:
    \f[
        x_1=x-\lfloor x\rfloor \qquad x_2=1-x_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         x_2\mbox{image}(\lfloor x\rfloor,y,z)+\\
         && x_1\mbox{image}(\lceil x\rceil,y,z)
        \end{array}
    \f]
    respectively for \f$\left|\sin\theta\right|\ge\left|\cos\theta\right|\f$:
    \f[
        y_1=y-\lfloor y\rfloor \qquad y_2=1-y_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         y_2\mbox{image}(x,\lfloor y\rfloor,z)+\\
         && y_1\mbox{image}(x,\lceil y\rceil,z)
        \end{array}
    \f]
    The MIP value (r,t,p) is the maximum of the MIP value and the image value.

    The code uses the \f$mask\f$ lookup-table to skip image voxels that are not
    part of the circular image. If running on a distributed memory system with
    n nodes, only every n\em th plane of the sinogram is processed.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::mip_proj(float * const img, float *mip,
                        const unsigned short int subset,
                        const unsigned short int z_start,
                        const unsigned short int z_end) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        mip+=view_size_padded)
    { float incx, incy, *view;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
      view=mip+RhoSamples_padded;
      if (fabsf(incy) > fabsf(incx))
       { float delxx0;

         delxx0=-sin_theta[t]/cos_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
         for (unsigned short int p=z_start; p < z_end; p++,
              view+=RhoSamples_padded)
          { float *ip, xx0;
                                         // calculate start point of projection
            xx0=image_center*(1.0f-delxx0)-rho_center*incx;
            ip=img+XYSamples_padded;
            for (unsigned short int y=0; y < XYSamples; y++,
                 ip+=XYSamples_padded,
                 xx0+=delxx0)
             { float x, *ip2;
               unsigned short int r;

               ip2=ip+(unsigned long int)(p+1)*image_plane_size_padded;
               for (x=xx0,
                    r=1; r <= RhoSamples; r++,
                    x+=incx)
                   // exclude bins in projection, that are not hit by the image
                if ((x >= (float)(mask[y].start-1)) &&
                    (x < (float)(mask[y].end+1)))
                 { signed short int xfloor;
 
                   xfloor=(signed short int)(x+1.0f)-1;
                            // the two voxels that are hit by the LOR view[r,p]
                            // are image[xfloor,y,z] and image[xfloor+1,y,z]
                                  // calculate weights for linear interpolation
                   view[r]=std::max(view[r],
                                    _linearInterp(x-(float)xfloor,
                                                  ip2[xfloor+1],
                                                  ip2[xfloor+2]));
                 }
             }
          }
         continue;
       }
      float delyy0, *ip;

      delyy0=-cos_theta[t]/sin_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
      ip=img+(unsigned long int)(z_start+1)*image_plane_size_padded;
      for (unsigned short int p=z_start; p < z_end; p++,
           view+=RhoSamples_padded,
           ip+=image_plane_size_padded)
       { float yy0;
                                         // calculate start point of projection
         yy0=image_center*(1.0f-delyy0)-rho_center*incy;
         for (unsigned short int x=0; x < XYSamples; x++,
              yy0+=delyy0)
          { float y, *ip2;
           unsigned short int r;

            ip2=ip+x+1;
            for (y=yy0,
                 r=1; r <= RhoSamples; r++,
                 y+=incy)
                   // exclude bins in projection, that are not hit by the image
             { signed short int yfloor;
               bool hit=false;

               yfloor=(signed short int)(y+2.0f)-2;
               if ((yfloor >= -1) && (yfloor < XYSamples))
                { if (yfloor >= 0)
                   if ((x >= mask[yfloor].start) && (x <= mask[yfloor].end))
                    hit=true;
                  if (yfloor < XYSamples-1)
                   if ((x >= mask[yfloor+1].start) &&
                       (x <= mask[yfloor+1].end))
                    hit=true;
                }
               if (hit)
                { float *ip3;
                            // the two voxels that are hit by the LOR view[r,p]
                            // are image[x,yfloor,z] and image[x,yfloor+1,z]
                                  // calculate weights for linear interpolation
                  ip3=ip2+(unsigned long int)(yfloor+1)*
                          (unsigned long int)XYSamples_padded;
                  view[r]=std::max(view[r], _linearInterp(y-(float)yfloor,
                                                       ip3[0],
                                                       ip3[XYSamples_padded]));
                }
             }
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate maximum intensity projection of image into subset with
           mode 2 LUT.
    \param[in]     tfp       pointer to lookup-table
    \param[in]     img       image data
    \param[in,out] mip       padded mip data
    \param[in]     subset    number of subset
    \param[in]     z_start   first plane of view to calculate
    \param[in]     z_end     last plane+1 of view to calculate

    Calculate the maximum intensity projection of the image into one subset
    with mode 2 LUT.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we
    calculate for each bin of the sinogram view (r,p) and line (y,z) of the
    image the coordinate x, where the LOR hits the image line. Otherwise we
    calculate for each bin of the sinogram view (r,p) and line (x,z) of the
    image the coordinate y, where the LOR hits the image line.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
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

    The image value at the coordinate (x,y,z) is calculated by linear
    interpolation for \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$:
    \f[
        x_1=x-\lfloor x\rfloor \qquad x_2=1-x_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         x_2\mbox{image}(\lfloor x\rfloor,y,z)+\\
         && x_1\mbox{image}(\lceil x\rceil,y,z)
        \end{array}
    \f]
    respectively for \f$\left|\sin\theta\right|\ge\left|\cos\theta\right|\f$:
    \f[
        y_1=y-\lfloor y\rfloor \qquad y_2=1-y_1
    \f]
    \f[
        \begin{array}{rcl}
         \mbox{image}(x,y,z) & = &
         y_2\mbox{image}(x,\lfloor y\rfloor,z)+\\
         && y_1\mbox{image}(x,\lceil y\rceil,z)
        \end{array}
    \f]
    The MIP value (r,t,p) is the maximum of the MIP value and the image value.

    The code uses a lookup-table to skip bins that won't hit the cylindrical
    image.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::mip_proj_table2(tmaskline * const tfp, float * const img,
                               float *mip, const unsigned short int subset,
                               const unsigned short int z_start,
                               const unsigned short int z_end) const
 { for (unsigned short int t=subset; t < ThetaSamples; t+=subsets,
        mip+=view_size_padded)
    { float incx, incy, *view;
      unsigned long int idx;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
                    // pointer into look-up table with line-masks that excludes
                    // bins of the sinogram view that don't hit the image
      idx=(unsigned long int)t*(unsigned long int)XYSamples;
      view=mip+RhoSamples_padded;
      if (fabsf(incy) > fabsf(incx))
       { float delxx0;

         delxx0=-sin_theta[t]/cos_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
         for (unsigned short int p=z_start; p < z_end; p++,
              view+=RhoSamples_padded)
          { float *ip, xx0;
                                         // calculate start point of projection
            xx0=image_center*(1.0f-delxx0)-rho_center*incx;
            ip=img+XYSamples_padded;
            for (unsigned short int y=0; y < XYSamples; y++,
                 ip+=XYSamples_padded,
                 xx0+=delxx0)
             { float x, *ip2;
               unsigned short int r;

               ip2=ip+(unsigned long int)(p+1)*image_plane_size_padded;
               for (x=xx0+(float)tfp[idx+y].start*incx,
                   // exclude bins in projection, that are not hit by the image
                    r=tfp[idx+y].start+1; r <= tfp[idx+y].end+1; r++,
                    x+=incx)
                { signed short int xfloor;

                  xfloor=(signed short int)(x+1.0f)-1;
                            // the two voxels that are hit by the LOR view[r,p]
                            // are image[xfloor,y,z] and image[xfloor+1,y,z]
                                  // calculate weights for linear interpolation
                  view[r]=std::max(view[r], _linearInterp(x-(float)xfloor,
                                                          ip2[xfloor+1],
                                                          ip2[xfloor+2]));
                }
             }
          }
         continue;
       }
      float delyy0, *ip;

      delyy0=-cos_theta[t]/sin_theta[t];
                               // follow sinogram view through image volume and
                               // calculate weights by linear interpolation
      ip=img+(unsigned long int)(z_start+1)*image_plane_size_padded;
      for (unsigned short int p=z_start; p < z_end; p++,
           view+=RhoSamples_padded,
           ip+=image_plane_size_padded)
       { float yy0;
                                         // calculate start point of projection
         yy0=image_center*(1.0f-delyy0)-rho_center*incy;
         for (unsigned short int x=0; x < XYSamples; x++,
              yy0+=delyy0)
          { float y, *ip2;
            unsigned short int r;

            ip2=ip+x+1;
            for (y=yy0+(float)tfp[idx+x].start*incy,
                   // exclude bins in projection, that are not hit by the image
                 r=tfp[idx+x].start+1; r <= tfp[idx+x].end+1; r++,
                 y+=incy)
             { signed short int yfloor;
               float *ip3;

               yfloor=(signed short int)(y+1.0f)-1;
                            // the two voxels that are hit by the LOR view[r,p]
                            // are image[x,y,z] and image[x,y+1,z]
                                  // calculate weights for linear interpolation
               ip3=ip2+(unsigned long int)(yfloor+1)*
                       (unsigned long int)XYSamples_padded;
               view[r]=std::max(view[r], _linearInterp(y-(float)yfloor, ip3[0],
                                                       ip3[XYSamples_padded]));
             }
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate mode 2 LUT for one segment of the forward-projection.
    \param[in,out] tfp           pointer to lookup-table
    \param[in]     segment       number of segment
    \param[in]     max_threads   maximum number of parallel threads to use

    Calculate mode 2 lookup-table for one segment of the forward-projection.
    The calculation for oblique segments is multi-threaded. Every thread
    processes a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::searchIndicesFwd(tmaskline * const tfp,
                                const unsigned short int segment,
                                const unsigned short int max_threads)
 { try
   { std::vector <tthread_id> tid;
     std::vector <bool> thread_running;
     unsigned short int threads=0, t;
     void *thread_result;

     try
     { std::vector <FwdPrj3D::tthread_params> tp;
                                                    // number of threads to use
       threads=std::min(max_threads, axis_slices[0]);
       if (axes == 1) threads=1;
        else if (threads > 1)
              { tid.resize(threads);                    // table for thread-IDs
                tp.resize(threads);              // table for thread parameters
                                                     // table for thread status
                thread_running.assign(threads, false);
              }
                                             // calculate all angles of segment
                                              // sequential calculation of view
       if (segment == 0) searchIndicesFwd_seg0(tfp);               // segment 0
        else if (threads == 1)               // single-threaded oblique segment
              searchIndicesFwd_oblique(tfp, segment, z_bottom[segment],
                                       z_top[segment]+1);
        else { unsigned short int slices, z_start, i;
                                                // parallel calculation of view
               slices=z_top[segment]-z_bottom[segment]+1;
               for (z_start=z_bottom[segment],
                    t=threads,
                    i=0; i < threads; i++,
                    t--)
                {                                // calculate thread parameters
                  tp[i].object=this;
                  tp[i].tfp=tfp;
                  tp[i].segment=segment;
                  tp[i].z_start=z_start;                // first slice of image
                  tp[i].z_end=z_start+slices/t;       // last slice +1 of image
#ifdef XEON_HYPERTHREADING_BUG
                  tp[i].thread_number=i;
#endif
                  thread_running[i]=true;
                                                                // start thread
                  ThreadCreate(&tid[i], &executeThread_searchFwd, &tp[i]);
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
/*! \brief Create mode 2 lookup-table for segment 0.
    \param[in,out] tfp   pointer to lookup-table

    This method searches the bins in every projection of segment 0 of the
    sinogram, that hit the cylindrical image.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate for
    each bin of the sinogram view (r,p) and plane (y,z) of the image the
    coordinate x, where the LOR hits the image line. Otherwise we calculate for
    each bin of the sinogram view (r,p) and plane (x,z) of the image the
    coordinate y, where the LOR hits the image line.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
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

    The coordinates of the first and last bin of every projection are stored
    where the coordinate (x,y,z) is inside a circular mask in the image. The
    code uses the \f$mask\f$ lookup-table to determine the borders of the
    circular mask.

    The table will contain \f$T\cdot\mbox{XYSamples}\f$ of these pairs,
    where \f$T\f$ is the number of projections in a sinogram plane.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::searchIndicesFwd_seg0(tmaskline * const tfp) const
 { for (unsigned short int t=0; t < ThetaSamples; t++)
    { float incx, incy;
      unsigned long int idx;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
                    // pointer into look-up table with line-masks that excludes
                    // bins of the sinogram view that don't hit the image
      idx=(unsigned long int)t*(unsigned long int)XYSamples;
      if (fabsf(incy) > fabsf(incx))
       { float delxx0, xx0;

         delxx0=-sin_theta[t]/cos_theta[t];
                                   // follow sinogram view through image volume
                                         // calculate start point of projection
         xx0=image_center*(1.0f-delxx0)-rho_center*incx;
         for (unsigned short int y=0; y < XYSamples; y++,
              xx0+=delxx0,
              idx++)
          { unsigned short int *rfirst, *rlast, r;
            bool found_rf=false;
            float x;
                                    // find bins at beginning and end of
                                    // projection that are not hit by the image
            rfirst=&tfp[idx].start;
            *rfirst=1;
            rlast=&tfp[idx].end;
            *rlast=0;
            for (x=xx0,
                 r=0; r < RhoSamples; r++,
                 x+=incx)
             if ((x >= (float)(mask[y].start-1)) &&
                 (x < (float)(mask[y].end+1)))
              { if (!found_rf) { *rfirst=r;
                                 found_rf=true;
                               }
                *rlast=r;
              }
          }
         continue;
       }
      float delyy0, yy0;

      delyy0=-cos_theta[t]/sin_theta[t];
                                   // follow sinogram view through image volume
                                         // calculate start point of projection
      yy0=image_center*(1.0f-delyy0)-rho_center*incy;
      for (unsigned short int x=0; x < XYSamples; x++,
           yy0+=delyy0,
           idx++)
       { unsigned short int *rfirst, *rlast, r;
         bool found_rf=false;
         float y;

         rfirst=&tfp[idx].start;
         *rfirst=1;
         rlast=&tfp[idx].end;
         *rlast=0;
         for (y=yy0,
              r=0; r < RhoSamples; r++,
              y+=incy)
          { signed short int yfloor;
            bool hit=false;
                                    // find bins at beginning and end of
                                    // projection that are not hit by the image
            yfloor=(signed short int)(y+2.0f)-2;
            if ((yfloor >= -1) && (yfloor < XYSamples))
             { if (yfloor >= 0)
                if ((x >= mask[yfloor].start) && (x <= mask[yfloor].end))
                 hit=true;
               if (yfloor < XYSamples-1)
                if ((x >= mask[yfloor+1].start) && (x <= mask[yfloor+1].end))
                 hit=true;
             }
            if (hit)
             { if (!found_rf) { *rfirst=r;
                                found_rf=true;
                              }
               *rlast=r;
             }
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create mode 2 lookup-table for oblique segment.
    \param[in,out] tfp       pointer to lookup-table
    \param[in]     segment   number of segment
    \param[in]     z_start   first plane of view to calculate
    \param[in]     z_end     last plane+1 of view to calculate

    This method searches the bins in every projection of an oblique segment of
    the sinogram, that hit the cylindrical image.
    If \f$\left|\sin\theta\right|<\left|\cos\theta\right|\f$ we calculate for
    each bin of the sinogram view (r,p) and line y of the image the coordinate
    (x,z), where the LOR hits the image plane. Otherwise we calculate for each
    bin of the sinogram view (r,p) and line x of the image the coordinate
    (y,z), where the LOR hits the image plane.
    \f[
        \left\{
          \begin{array}{r@{\quad:\quad}l}
           x=i_c+\left(i_c-y\right)\frac{\sin\theta}{\cos\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\cos\theta} &
           \left|\sin\theta\right|<\left|\cos\theta\right|\\
           y=i_c+\left(i_c-x\right)\frac{\cos\theta}{\sin\theta}-
           \left(\rho_c-r\right)\frac{\Delta_\rho}{\Delta_{xy}\sin\theta} &
           \left|\sin\theta\right|\ge\left|\cos\theta\right|
          \end{array}\right.
    \f]
    \f[
        z=\left\{
         \begin{array}{r@{\quad:\quad}l}
          p+\left(\rho_c-r\right)
          \frac{\sin\phi\sin\theta\Delta_\rho}{\cos\phi\cos\theta\Delta_z}-
          \left(i_c-y\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\theta\cos\phi\Delta_z} &
          \left|\sin\theta\right|<\left|\cos\theta\right|\\
          p-\left(\rho_c-r\right)
          \frac{\sin\phi\cos\theta\Delta_\rho}{\cos\phi\sin\theta\Delta_z}+
          \left(i_c-x\right)
          \frac{\sin\phi\Delta_{xy}}{\cos\phi\sin\theta\Delta_z} &
          \left|\sin\theta\right|\ge\left|\cos\theta\right|
         \end{array}\right.
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

    The coordinates of the first and last bin of every projection are stored
    where the coordinate (x,y,z) is inside a circular mask in the image. The
    code uses the \f$mask\f$ lookup-table to determine the borders of the
    circular mask.

    The table will contain
    \f$T\cdot\mbox{axis\_slices}[axis]\cdot\mbox{XYSamples}\f$ of these pairs,
    where \f$T\f$ is the number of projections in a sinogram plane.
 */
/*---------------------------------------------------------------------------*/
void FwdPrj3D::searchIndicesFwd_oblique(tmaskline * const tfp,
                                        const unsigned short int segment,
                                        const unsigned short int z_start,
                                        const unsigned short int z_end) const
 { for (unsigned short int t=0; t < ThetaSamples; t++)
    { float incx, incy;
      unsigned long int idx;
                                // calculate increments to follow sinogram view
      if (cos_theta[t] == 0.0f) incx=std::numeric_limits <float>::max();
       else incx=BinSizeRho/(DeltaXY*cos_theta[t]);
      if (sin_theta[t] == 0.0f) incy=std::numeric_limits <float>::max();
       else incy=BinSizeRho/(DeltaXY*sin_theta[t]);
                    // pointer into look-up table with line-masks that excludes
                    // bins of the sinogram view that don't hit the image
      idx=(unsigned long int)
          ((unsigned long int)t*
           (unsigned long int)axis_slices[(segment+1)/2]/2+
           z_start-z_bottom[segment])*(unsigned long int)XYSamples;
      if (fabsf(incy) > fabsf(incx))
       { float delxx0, tmp;

         delxx0=-sin_theta[t]/cos_theta[t];
         tmp=1.0f/(cos_theta[t]*plane_separation);
                               // follow sinogram view through image volume and
                               // calculate weights by bilinear interpolation
         for (unsigned short int p=z_start; p < z_end; p++)
          { float incz, zz0, delzz0, sinphi, tanphi, xx0;
            unsigned short int v_idx;
                                                 // get angle of sinogram plane
            calculatePlaneAngle(segment, p, &sinphi, &tanphi, &v_idx);
                                         // calculate start point of projection
            xx0=image_center*(1.0f-delxx0)-rho_center*incx;
            incz=-tanphi*BinSizeRho*sin_theta[t]*tmp;
            delzz0=DeltaXY*tanphi*tmp;
            zz0=(float)p-rho_center*incz-image_center*delzz0;
            for (unsigned short int y=0; y < XYSamples; y++,
                 xx0+=delxx0,
                 zz0+=delzz0,
                 idx++)
             { unsigned short int *rfirst, *rlast, r;
               bool found_rf=false;
               float x, z;
                                    // find bins at beginning and end of
                                    // projection that are not hit by the image
               rfirst=&tfp[idx].start;
               *rfirst=1;
               rlast=&tfp[idx].end;
               *rlast=0;
               for (x=xx0,
                    z=zz0,
                    r=0; r < RhoSamples; r++,
                    x+=incx,
                    z+=incz)
                if ((x >= (float)(mask[y].start-1)) &&
                    (x < (float)(mask[y].end+1)))
                 if ((z >= -1.0f) && (z < (float)axis_slices[0]))
                  { if (!found_rf) { *rfirst=r;
                                     found_rf=true;
                                   }
                    *rlast=r;
                  }
             }
          }
         continue;
       }
      float delyy0, tmp;

      delyy0=-cos_theta[t]/sin_theta[t];
      tmp=1.0f/(sin_theta[t]*plane_separation);
                               // follow sinogram view through image volume and
                               // calculate weights by bilinear interpolation
      for (unsigned short int p=z_start; p < z_end; p++)
       { float incz, zz0, delzz0, sinphi, tanphi, yy0;
         unsigned short int v_idx;
                                                 // get angle of sinogram plane
         calculatePlaneAngle(segment, p, &sinphi, &tanphi, &v_idx);
                                         // calculate start point of projection
         yy0=image_center*(1.0f-delyy0)-rho_center*incy;
         incz=tanphi*BinSizeRho*cos_theta[t]*tmp;
         delzz0=-DeltaXY*tanphi*tmp;
         zz0=(float)p-rho_center*incz-image_center*delzz0;
         for (unsigned short int x=0; x < XYSamples; x++,
              yy0+=delyy0,
              zz0+=delzz0,
              idx++)
          { unsigned short int *rfirst, *rlast, r;
            bool found_rf=false;
            float y, z;

            rfirst=&tfp[idx].start;
            *rfirst=1;
            rlast=&tfp[idx].end;
            *rlast=0;
            for (y=yy0,
                 z=zz0,
                 r=0; r < RhoSamples; r++,
                 y+=incy,
                 z+=incz)
             if ((z >= -1.0f) && (z < (float)axis_slices[0]))
              { signed short int yfloor;
                bool hit=false;
                                    // find bins at beginning and end of
                                    // projection that are not hit by the image
                yfloor=(signed short int)(y+2.0f)-2;
                if ((yfloor >= -1) && (yfloor < XYSamples))
                 { if (yfloor >= 0)
                    if ((x >= mask[yfloor].start) && (x <= mask[yfloor].end))
                     hit=true;
                   if (yfloor < XYSamples-1)
                    if ((x >= mask[yfloor+1].start) &&
                        (x <= mask[yfloor+1].end))
                     hit=true;
                 }
                if (hit)
                 { if (!found_rf) { *rfirst=r;
                                    found_rf=true;
                                  }
                   *rlast=r;
                 }
              }
          }
       }
    }
 }
