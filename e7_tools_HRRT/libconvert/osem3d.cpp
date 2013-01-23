/*! \class OSEM3D osem3d.h "osem3d.h"
    \brief This class implements a 2D/3D-OSEM reconstruction, 2D/3D-forward and
           back-projection and maximum intensity projection.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2003/12/12 reconstruction with 3D Gibbs prior
    \date 2003/12/12 reconstruction with 3D-OP-OSEM
    \date 2004/01/02 image mask is centered in image, independent from COR
    \date 2004/01/08 add Unicode support for WIN32 version
    \date 2004/01/14 create LUTs only when needed for the first time, not at
                     initialization
    \date 2004/01/30 use sinogram rebinning factor instead of COR
    \date 2004/02/27 use SinogramConversion and ImageConversion classes
    \date 2004/04/02 use vector code
    \date 2004/04/07 use memory controller in forward-projector
    \date 2004/04/08 use memory controller where possible
    \date 2004/04/08 add TOF-AW-OSEM
    \date 2004/09/14 added Doxygen style comments
    \date 2004/09/15 added TOF support to Gibbs-OSEM
    \date 2004/09/21 calculate shift for TOF-OP-OSEM
    \date 2004/09/27 fixed NW/ANW/OP-OSEM for cases where norm is 0
    \date 2004/10/04 use faster backprojector
    \date 2004/11/11 NW- and ANW-OSEM fixed for scanners with gaps
    \date 2005/01/04 added progress reporting

    The OSEM reconstruction consists of 4 steps:
    - initialize object
    - calculate normalization matrix
    - uncorrect sinogram (for AW/NW/ANW-OSEM)
    - reconstruct image
    - delete object

    The class provides separate methods for these steps and differentiates
    for UW-, AW-, NW- and ANW-OSEM. It can handle trues and TOF sinograms.
    The class provides also methods to use a Gibbs prior during reconstruction.

    \todo complete cluster support
 */

#include <iostream>
#include <vector>
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <sys/utsname.h>
#include <netdb.h>
#endif
#ifdef WIN32
#include <atlbase.h>
#include <windows.h>
#endif
#include "osem3d.h"
#include "e7_common.h"
#include "fastmath.h"
#include "global_tmpl.h"
#include "mem_ctrl.h"
#include "progress.h"
#include "rebin_sinogram.h"
#include "red_client.h"
#include "stopwatch.h"
#include "str_tmpl.h"
#include "syngo_msg.h"
#include "vecmath.h"

/*- constants ---------------------------------------------------------------*/

             /*! \f$\epsilon\f$-threshold to avoid large errors in divisions */
const float OSEM3D::epsilon=0.0001f;
const float OSEM3D::image_max=100000.0f; /*!< maximum allowed value in image */
     /*! name of OSEM slave executable for distributed memory reconstruction */
#ifdef WIN32
const std::string OSEM3D::slave_executable="osem_slave.exe";
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
const std::string OSEM3D::slave_executable="osem_slave";
#endif
            /*! size of receive buffer in bytes for communication with slave */
const unsigned long int OSEM3D::OS_RECV_BUFFER_SIZE=10000000;
              /*! size of send buffer in byytes for communication with slave */
const unsigned long int OSEM3D::OS_SEND_BUFFER_SIZE=10000000;

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _XYSamples   width/height of image
    \param[in] _DeltaXY            with/height of voxels in mm
    \param[in] _RhoSamples         number of bins in projections
    \param[in] _BinSizeRho         width of bins in projections in mm
    \param[in] _ThetaSamples       number of angles in sinogram
    \param[in] _axis_slices        number of planes for axes of 3d sinogram
    \param[in] _span               span of sinogram
    \param[in] _mrd                maximum ring difference of sinogram
    \param[in] _plane_separation   plane separation in sinogram in mm
    \param[in] reb_factor          sinogram rebinning factor
    \param[in] _subsets            number of subsets for reconstruction
    \param[in] _algo               reconstruction algorithm
    \param[in] _lut_level          0=don't use lookup-tables
                                   1=use small lookup-table
                                   2=use large lookup-tables (only for 3d OSEM)
    \param[in] slaves              IP numbers of slave computers to use
    \param[in] _loglevel           top level for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
OSEM3D::OSEM3D(const unsigned short int _XYSamples, const float _DeltaXY,
               const unsigned short int _RhoSamples, const float _BinSizeRho,
               const unsigned short int _ThetaSamples,
               const std::vector <unsigned short int> _axis_slices,
               const unsigned short int _span, const unsigned short int _mrd,
               const float _plane_separation, const float _reb_factor,
               const unsigned short int _subsets, const ALGO::talgorithm _algo,
               const unsigned short int _lut_level,
               std::list <tnode> * const slaves,
               const unsigned short int _loglevel)
 { init(_XYSamples, _DeltaXY, _RhoSamples, _BinSizeRho, _ThetaSamples,
        _axis_slices, _span, _mrd, _plane_separation, _reb_factor, _subsets,
        _algo, _lut_level, slaves, _loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] sino          sinogram
    \param[in] _XYSamples    width/height of image
    \param[in] _DeltaXY      with/height of voxels in mm
    \param[in] reb_factor    sinogram rebinning factor
    \param[in] _subsets      number of subsets for reconstruction
    \param[in] _algo         reconstruction algorithm
    \param[in] _lut_level    0=don't use lookup-tables
                             1=use small lookup-table
                             2=use large lookup-tables (only for 3d OSEM)
    \param[in] slaves        IP numbers of slave computers to use
    \param[in] _loglevel     top level for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
OSEM3D::OSEM3D(SinogramConversion * const sino,
               const unsigned short int _XYSamples, const float _DeltaXY,
               const float _reb_factor, const unsigned short int _subsets,
               const ALGO::talgorithm _algo,
               const unsigned short int _lut_level,
               std::list <tnode> * const slaves,
               const unsigned short int _loglevel)
 { init(_XYSamples, _DeltaXY, sino->RhoSamples(), sino->BinSizeRho(),
        sino->ThetaSamples(), sino->axisSlices(), sino->span(), sino->mrd(),
        sino->planeSeparation(), _reb_factor, _subsets, _algo, _lut_level,
        slaves, _loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destory object.
 */
/*---------------------------------------------------------------------------*/
OSEM3D::~OSEM3D()
 { unsigned short int i;
 
   for (i=0; i < norm_factors.size(); i++)
    MemCtrl::mc()->deleteBlockForce(&norm_factors[i]);
   for (i=0; i < offset_matrix.size(); i++)
    MemCtrl::mc()->deleteBlockForce(&offset_matrix[i]);
   if (bp != NULL) delete bp;
   if (fwd != NULL) delete fwd;
   { std::vector <CommSocket *>::iterator it;

     for (it=os_slave.begin(); it != os_slave.end(); ++it)
      if (*it != NULL) { terminateSlave(*it);
                         delete *it;
                       }
   }
   if (wbuffer != NULL) delete wbuffer;
   if (rbuffer != NULL) delete rbuffer;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate 2d or 3d back-projection of sinogram.
    \param[in]  sinogram      sinogram object
    \param[out] image_index   memory controller index for image
    \param[in]  max_threads   maximum number of threads to use
    \return pointer to image data

    Calculate 2d or 3d back-projection of sinogram. The method adds padding to
    the sinogram and calculates the back-projection into an image. The input
    sinogram remains unchanged.

    The calculation is split into subsets and progress information is printed
    after every subsets. The same result can be achieved with one subset, but
    less progress informatiom will be printed.

    The method returns a pointer to the image data, which is stored in the
    memory controller.
 */
/*---------------------------------------------------------------------------*/
float *OSEM3D::bck_proj3d(SinogramConversion *sinogram,
                          unsigned short int * const image_index,
                          const unsigned short int max_threads)
 { unsigned short int tof_bins, buffer_index;
   float *image, *buffer;
   StopWatch sw;

   tof_bins=1;
   initBckProjector(max_threads);
   flog->logMsg("start backprojection", loglevel+1);
   flog->logMsg("time of flight bins=#1", loglevel+2)->arg(tof_bins);
   sw.start();
   image=MemCtrl::mc()->createFloat(image_volume_size, image_index, "img",
                                    loglevel+2);;
   memset(image, 0, image_volume_size*sizeof(float));
   buffer=MemCtrl::mc()->createFloat(subset_size_padded_TOF, &buffer_index,
                                     "bfr", loglevel+2);
                                                    // back-project all subsets
   for (unsigned short int subset=0; subset < subsets; subset++)
    { 
      flog->logMsg("start backprojection of subset #1 from #2x#3x#4 to "
                    "#5x#6x#7", loglevel+2)->arg(subset)->arg(RhoSamples)->
        arg(ThetaSamples)->arg(axes_slices)->arg(XYSamples)->arg(XYSamples)->
        arg(axis_slices[0]);
      sw.start();
      for (unsigned short int segment=0; segment < segments; segment++)
       { unsigned short int planes, t, axis;
         unsigned long int offs;
         float *sp, *sino_ptr, *bufp;

         axis=(segment+1)/2;
                 // copy sinogram views of subset of segment into padded buffer
         if (segment == 0) planes=axis_slices[0];
          else planes=axis_slices[axis]/2;
                                                   // copy sinogram into buffer
         sp=MemCtrl::mc()->getFloatRO(sinogram->index(axis, 0), loglevel+3);
         if ((segment == 0) || (segment & 0x1)) offs=0;
          else offs=(unsigned long int)axis_slices[axis]/2*
                    sino_plane_size_TOF;
         memset(buffer, 0, subset_size_padded_TOF*sizeof(float));
         for (bufp=buffer+RhoSamples_padded_TOF,
              sino_ptr=&sp[offs+(unsigned long int)order[subset]*
                                (unsigned long int)RhoSamples_TOF],
              t=0; t < ThetaSamples/subsets; t++,
              sino_ptr+=(unsigned long int)subsets*
                        (unsigned long int)RhoSamples_TOF,
              bufp+=view_size_padded_TOF)
          { float *bufp1, *sino_ptr2;
            unsigned short int p;

            for (bufp1=bufp+(unsigned long int)z_bottom[segment]*
                            (unsigned long int)RhoSamples_padded_TOF,
                 sino_ptr2=sino_ptr,
                 p=0; p < planes; p++,
                 sino_ptr2+=sino_plane_size_TOF,
                 bufp1+=RhoSamples_padded_TOF)
             memcpy(bufp1+tof_bins, sino_ptr2, RhoSamples_TOF*sizeof(float));
          }
         MemCtrl::mc()->put(sinogram->index(axis, 0));
                                  // back-project subset of segment of sinogram
         bp->back_proj3d(buffer, image, subset, segment, false, max_threads);
       }
      flog->logMsg("finished backprojection of subset #1 in #2 sec",
                   loglevel+2)->arg(subset)->arg(sw.stop());
    }
   MemCtrl::mc()->put(buffer_index);
   MemCtrl::mc()->deleteBlock(&buffer_index);
   flog->logMsg("finished backprojection in #1 sec", loglevel+1)->
    arg(sw.stop());
   return(image);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate 2d or 3d back-projection of sinogram.
    \param[in] sino          pointer to sinogram
    \param[in] max_threads   maximum number of threads to use
    \return pointer to image data

    Calculate 2d or 3d back-projection of sinogram. A sinogram object is
    created and filled with the sinogram data. The resulting image object is
    deleted and a pointer to the flat image data is returned. This method does
    the same as the one it overloads, except that the user doesn't handle
    sinogram and image objects.
 */
/*---------------------------------------------------------------------------*/
float *OSEM3D::bck_proj3d(float * const sino,
                          const unsigned short int max_threads)
 { SinogramConversion *sinogram=NULL;

   try
   { unsigned short int image_index;

     sinogram=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamples,
                                     span, 0, plane_separation, 0, 0.0f, false,
                                     0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                     axes == 1);
     sinogram->copyData(sino, 0, axes, axis_slices[0], 1, false, true, "emis",
                        loglevel);
     bck_proj3d(sinogram, &image_index, max_threads);
     delete sinogram;
     sinogram=NULL;
     MemCtrl::mc()->put(image_index);
     return(MemCtrl::mc()->getRemoveFloat(&image_index, loglevel));
   }
   catch (...)
    { if (sinogram != NULL) delete sinogram;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate bkproj(1) normalization matrix.
    \param[in] max_threads   maximum number of threads to use for the
                             backprojection

    Calculate the bkproj(1) normalization matrix for UW-OSEM. The backprojector
    expects a padded sinogram. The resulting matrix is stored by the memory
    controller and the indices of the memory segments are stored in the
    norm_factors array.

    The calculation is split into subsets and progress information is printed
    after every subsets. The same result can be achieved with one subset, but
    less progress informatiom will be printed.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcNormFactorsUW(const unsigned short int max_threads)
 { std::string size_str;
   unsigned short int norm_prj_index;
   float *norm_prj;
   StopWatch sw;

   initBckProjector(max_threads);
   flog->logMsg("start calculation of normalization factors for UW-OSEM",
                loglevel+1);
   sw.start();
            // subset of segment from 1 sinogram for backprojection (view-mode)
   norm_prj=MemCtrl::mc()->createFloat(subset_size_padded, &norm_prj_index,
                                       "npj", loglevel+2);
   { double size;

     size=(double)(image_volume_size*(unsigned long int)subsets)*
          sizeof(float)/1024.0f;
     size_str=toString(size/1024.0f)+" MByte";
   }
   flog->logMsg("allocate #1 for normalization factors", loglevel+2)->
    arg(size_str);
   norm_factors.resize(subsets);
                                                       // calculate all subsets
   for (unsigned short int subset=0; subset < subsets; subset++)
    { float *normfac;

      SyngoMsg(syngo_msg+": calculate normalization matrix (subset "+
               toString(subset)+")");
      flog->logMsg("start calculation of normalization factors for subset #1",
                   loglevel+2)->arg(subset);
      sw.start();
      normfac=MemCtrl::mc()->createFloat(image_volume_size,
                                         &norm_factors[subset], "nfac",
                                         loglevel+2);
      memset(normfac, 0, image_volume_size*sizeof(float));
                                                      // calculate all segments
      for (unsigned short int segment=0; segment < segments; segment++)
       { unsigned short int t, planes;
         float *np;

         flog->logMsg("calculate segment #1, subset #2 of '1' sinogram",
                      loglevel+3)->arg(segStr(segment))->arg(subset);
         if (segment == 0) planes=axis_slices[0];
          else planes=axis_slices[(segment+1)/2]/2;
                               // calculate subset of segment from '1' sinogram
         memset(norm_prj, 0, subset_size_padded*sizeof(float));
                                           // copy views for subset into buffer
         for (np=norm_prj+RhoSamples_padded,
              t=0; t < ThetaSamples/subsets; t++,
              np+=view_size_padded)
          { float *np1;
            unsigned short int p;

            for (np1=np+(unsigned long int)z_bottom[segment]*
                        (unsigned long int)RhoSamples_padded,
                 p=0; p < planes; p++,
                 np1+=RhoSamples_padded)
             for (unsigned short int r=0; r < RhoSamples; r++)
              np1[r+1]=1.0f;
          }
                                          // backproject subset of this segment
         flog->logMsg("backproject segment #1, subset #2 of sinogram from "
                      "#3x#4x#5 to #6x#7x#8", loglevel+3)->
          arg(segStr(segment))->arg(subset)->arg(RhoSamples)->
          arg(ThetaSamples)->arg(planes)->arg(XYSamples)->arg(XYSamples)->
          arg(axis_slices[0]);
         bp->back_proj3d(norm_prj, normfac, subset, segment, true,
                         max_threads);
       }
      MemCtrl::mc()->put(norm_factors[subset]);
      flog->logMsg("finished calculation of normalization factors for subset "
                   "#1 in #2 sec", loglevel+2)->arg(subset)->arg(sw.stop());
    }
   MemCtrl::mc()->put(norm_prj_index);
   MemCtrl::mc()->deleteBlock(&norm_prj_index);
   flog->logMsg("finished calculation of normalization factors in #1 sec",
                loglevel+1)->arg(sw.stop());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate bkproj(1/A) normalization matrix.
    \param[in] acf           acf object
    \param[in] max_threads   maximum number of threads to use for the
                             backprojection

    Calculate the bkproj(1/A) normalization matrix for AW-OSEM. The
    backprojector expects a padded sinogram. The acf data is rebinned to the
    size of the emission sinogram if needed, but the acf object is left
    unchanged. Values below 1 are cut-off from the acf before backprojection:
    \f[
       \mbox{bckproj}\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{1}{acf} & acf > 1\\
         1 & acf \le 1
        \end{array}
        \right\}
    \f]
    The resulting matrix is stored by the memory controller and the
    indices of the memory segments are stored in the norm_factors array.

    The calculation is split into subsets and progress information is printed
    after every subsets. The same result can be achieved with one subset, but
    less progress informatiom will be printed.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcNormFactorsAW(SinogramConversion *acf,
                               const unsigned short int max_threads)
 { if (acf == NULL)
    { calcNormFactorsUW(max_threads);
      return;
    }
   RebinSinogram *rebin=NULL;
   StopWatch sw;

   try
   { std::string size_str;
     unsigned short int norm_prj_index;
     float *norm_prj;

     initBckProjector(max_threads);
     flog->logMsg("start calculation of normalization factors for AW-OSEM",
                  loglevel+1);
     sw.start();
          // subset of segment from 1/A sinogram for backprojection (view-mode)
     norm_prj=MemCtrl::mc()->createFloat(subset_size_padded, &norm_prj_index,
                                         "npj", loglevel+2);
     { double size;

       size=(double)(image_volume_size*(unsigned long int)subsets)*
            sizeof(float)/1024.0f;
       size_str=toString(size/1024.0f)+" MByte";
     }
     flog->logMsg("allocate #1 for normalization factors", loglevel+2)->
      arg(size_str);
     norm_factors.resize(subsets);
     if ((RhoSamples != acf->RhoSamples()) ||
         (ThetaSamples != acf->ThetaSamples()))
      rebin=new RebinSinogram(acf->RhoSamples(), acf->ThetaSamples(),
                              RhoSamples, ThetaSamples,
                              RebinX <float>::NO_ARC, 0, true);
                                                       // calculate all subsets
     for (unsigned short int subset=0; subset < subsets; subset++)
      { float *normfac;

        SyngoMsg(syngo_msg+": calculate normalization matrix (subset "+
                 toString(subset)+")");
        flog->logMsg("start calculation of normalization factors for subset "
                     "#1", loglevel+2)->arg(subset);
        sw.start();
        normfac=MemCtrl::mc()->createFloat(image_volume_size,
                                           &norm_factors[subset], "nfac",
                                           loglevel+2);
        memset(normfac, 0, image_volume_size*sizeof(float));
                                                      // calculate all segments
        for (unsigned short int segment=0; segment < segments; segment++)
         { unsigned short int planes, axis, t, attp_idx;
           float *attp, *np, *ap;

           axis=(segment+1)/2;
           if (segment == 0) planes=axis_slices[0];
            else planes=axis_slices[axis]/2;
                               // calculate subset of segment from 1/A sinogram
           flog->logMsg("calculate segment #1, subset #2 of 1/a sinogram",
                       loglevel+3)->arg(segStr(segment))->arg(subset);
           memset(norm_prj, 0, subset_size_padded*sizeof(float));
                                           // copy views for subset into buffer
           attp=MemCtrl::mc()->getFloatRO(acf->index(axis, 0), loglevel+4);
           if ((segment > 0) && !(segment & 0x1))
            attp+=(unsigned long int)planes*acf->planeSize();
           if (rebin != NULL)
            { flog->logMsg("rebin segment #1 from #2x#3x#4 to #5x#6x#7",
                           loglevel+4)->arg(segStr(segment))->
               arg(acf->RhoSamples())->arg(acf->ThetaSamples())->arg(planes)->
               arg(RhoSamples)->arg(ThetaSamples)->arg(planes);
              attp=rebin->calcRebinSinogram(attp, planes, NULL, &attp_idx,
                                            loglevel+4, max_threads);

              MemCtrl::mc()->put(acf->index(axis, 0));
            }
           for (np=norm_prj+RhoSamples_padded,
                ap=&attp[(unsigned long int)subset*
                         (unsigned long int)RhoSamples],
                t=0; t < ThetaSamples/subsets; t++,
                ap+=(unsigned long int)subsets*(unsigned long int)RhoSamples,
                np+=view_size_padded)
            { float *ap1, *np1;
              unsigned short int p;

              for (ap1=ap,
                   np1=np+(unsigned long int)z_bottom[segment]*
                          (unsigned long int)RhoSamples_padded,
                   p=0; p < planes; p++,
                   np1+=RhoSamples_padded,
                   ap1+=sino_plane_size)
               for (unsigned short int r=0; r < RhoSamples; r++)
                if (ap1[r] > 1.0f) np1[r+1]=1.0f/ap1[r];
                 else np1[r+1]=1.0f;
            }
           if (rebin != NULL)
            { MemCtrl::mc()->put(attp_idx);
              MemCtrl::mc()->deleteBlock(&attp_idx);
            }
            else MemCtrl::mc()->put(acf->index(axis, 0));
                                          // backproject subset of this segment
           flog->logMsg("backproject segment #1, subset #2 of sinogram from "
                        "#3x#4x#5 to #6x#7x#8", loglevel+3)->
            arg(segStr(segment))->arg(subset)->arg(RhoSamples)->
            arg(ThetaSamples)->arg(planes)->arg(XYSamples)->arg(XYSamples)->
            arg(axis_slices[0]);
           bp->back_proj3d(norm_prj, normfac, subset, segment, true,
                           max_threads);
         }
        MemCtrl::mc()->put(norm_factors[subset]);
        flog->logMsg("finished calculation of normalization factors for subset"
                     " #1 in #2 sec", loglevel+2)->arg(subset)->arg(sw.stop());
      }
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     MemCtrl::mc()->put(norm_prj_index);
     MemCtrl::mc()->deleteBlock(&norm_prj_index);
     flog->logMsg("finished calculation of normalization factors in #1 sec",
                  loglevel+1)->arg(sw.stop());
   }
   catch (...)
    { sw.stop();
      if (rebin != NULL) delete rebin;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate bkproj(1/A) normalization matrix.
    \param[in] acfdata             pointer to acf data
    \param[in] RhoSamplesAtten     number of bins in acf projections
    \param[in] ThetaSamplesAtten   number of angles in acf plane
    \param[in] max_threads         maximum number of threads to use for the
                                   backprojection

    Calculate the bkproj(1/A) normalization matrix for AW-OSEM. A sinogram
    object is created and filled with the acf data. This method does the same
    as the one it overloads, except that the user doesn't handle sinogram
    objects.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcNormFactorsAW(float * const acfdata,
                               const unsigned short int RhoSamplesAtten,
                               const unsigned short int ThetaSamplesAtten,
                               const unsigned short int max_threads)
 { if (acfdata == NULL)
    { calcNormFactorsUW(max_threads);
      return;
    }
   SinogramConversion *acf=NULL;

   try
   { acf=new SinogramConversion(RhoSamplesAtten,
                                BinSizeRho*(float)RhoSamples/
                                (float)RhoSamplesAtten, ThetaSamplesAtten,
                                span, 0, plane_separation, 0, 0.0f, false, 0,
                                0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, axes == 1);
     acf->copyData(acfdata, 0, axes, axis_slices[0], 1, false, true, "acf",
                   loglevel);
     calcNormFactorsAW(acf, max_threads);
     delete acf;
     acf=NULL;
   }
   catch (...)
    { if (acf != NULL) delete acf;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate bkproj(1/N) normalization matrix.
    \param[in] norm          norm object
    \param[in] max_threads   maximum number of threads to use for the
                             backprojection

    Calculate the bkproj(1/N) normalization matrix for NW-OSEM. The
    backprojector expects a padded sinogram. Values below \f$\epsilon\f$ are
    cut-off from the norm before backprojection:
    \f[
       \mbox{bckproj}\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{1}{norm} & norm > \epsilon\\
         0 & norm \le\epsilon
        \end{array}
        \right\}
    \f]
    The resulting matrix is stored by the memory controller and the indices of
    the memory segments are stored in the norm_factors array.

    The calculation is split into subsets and progress information is printed
    after every subsets. The same result can be achieved with one subset, but
    less progress informatiom will be printed.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcNormFactorsNW(SinogramConversion *norm,
                               const unsigned short int max_threads)
 { if (norm == NULL)
    { calcNormFactorsUW(max_threads);
      return;
    }
   StopWatch sw;
   std::string size_str;
   unsigned short int norm_prj_index;
   float *norm_prj;

   initBckProjector(max_threads);
   flog->logMsg("start calculation of normalization factors for NW-OSEM",
                loglevel+1);
   sw.start();
          // subset of segment from 1/N sinogram for backprojection (view-mode)
   norm_prj=MemCtrl::mc()->createFloat(subset_size_padded, &norm_prj_index,
                                       "npj", loglevel+2);
   { double size;

     size=(double)(image_volume_size*(unsigned long int)subsets)*
          sizeof(float)/1024.0f;
     size_str=toString(size/1024.0f)+" MByte";
   }
   flog->logMsg("allocate #1 for normalization factors", loglevel+2)->
    arg(size_str);
   norm_factors.resize(subsets);
                                                       // calculate all subsets
   for (unsigned short int subset=0; subset < subsets; subset++)
    { float *normfac;

      SyngoMsg(syngo_msg+": calculate normalization matrix (subset "+
               toString(subset)+")");
      flog->logMsg("start calculation of normalization factors for subset #1",
                   loglevel+2)->arg(subset);
      sw.start();
      normfac=MemCtrl::mc()->createFloat(image_volume_size,
                                         &norm_factors[subset], "nfac",
                                         loglevel+2);
      memset(normfac, 0, image_volume_size*sizeof(float));
                                                      // calculate all segments
      for (unsigned short int segment=0; segment < segments; segment++)
       { unsigned short int t, planes, axis;
         float *np, *np1, *nop, *normp;

         axis=(segment+1)/2;
         if (segment == 0) planes=axis_slices[0];
          else planes=axis_slices[axis]/2;
                               // calculate subset of segment from 1/N sinogram
         flog->logMsg("calculate segment #1, subset #2 of 1/n sinogram",
                     loglevel+3)->arg(segStr(segment))->arg(subset);
         memset(norm_prj, 0, subset_size_padded*sizeof(float));
         normp=MemCtrl::mc()->getFloatRO(norm->index(axis, 0), loglevel+4);
         if ((segment > 0) && !(segment & 0x1))
          normp+=(unsigned long int)planes*norm->planeSize();
                                           // copy views for subset into buffer
         if (norm->ThetaSamples() == 1)
          for (np=norm_prj+RhoSamples_padded,
               nop=normp,
               t=0; t < ThetaSamples/subsets; t++,
               np+=view_size_padded)
           { float *nop1;
             unsigned short int p;

             for (nop1=nop,
                  np1=np+(unsigned long int)z_bottom[segment]*
                         (unsigned long int)RhoSamples_padded,
                  p=0; p < planes; p++,
                  np1+=RhoSamples_padded,
                  nop1+=RhoSamples)
              for (unsigned short int r=0; r < RhoSamples; r++)
               if (nop1[r] > OSEM3D::epsilon) np1[r+1]=1.0f/nop1[r];
                else np1[r+1]=0.0f;
           }
          else for (np=norm_prj+RhoSamples_padded,
                    nop=&normp[(unsigned long int)subset*
                               (unsigned long int)RhoSamples],
                    t=0; t < ThetaSamples/subsets; t++,
                    nop+=(unsigned long int)subsets*
                         (unsigned long int)RhoSamples,
                    np+=view_size_padded)
                { float *nop1;
                  unsigned short int p;

                  for (nop1=nop,
                       np1=np+(unsigned long int)z_bottom[segment]*
                              (unsigned long int)RhoSamples_padded,
                       p=0; p < planes; p++,
                       np1+=RhoSamples_padded,
                       nop1+=sino_plane_size)
                   for (unsigned short int r=0; r < RhoSamples; r++)
                    if (nop1[r] > OSEM3D::epsilon) np1[r+1]=1.0f/nop1[r];
                     else np1[r+1]=0.0f;
                }
         MemCtrl::mc()->put(norm->index(axis, 0));
                                          // backproject subset of this segment
         flog->logMsg("backproject segment #1, subset #2 of sinogram from "
                      "#3x#4x#5 to #6x#7x#8", loglevel+3)->
          arg(segStr(segment))->arg(subset)->arg(RhoSamples)->
          arg(ThetaSamples)->arg(planes)->arg(XYSamples)->arg(XYSamples)->
          arg(axis_slices[0]);
         bp->back_proj3d(norm_prj, normfac, subset, segment, true,
                         max_threads);
       }
      MemCtrl::mc()->put(norm_factors[subset]);
      flog->logMsg("finished calculation of normalization factors for subset "
                   "#1 in #2 sec", loglevel+2)->arg(subset)->arg(sw.stop());
    }
   MemCtrl::mc()->put(norm_prj_index);
   MemCtrl::mc()->deleteBlock(&norm_prj_index);
   flog->logMsg("finished calculation of normalization factors in #1 sec",
                loglevel+1)->arg(sw.stop());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate bkproj(1/N) normalization matrix.
    \param[in] normdata           pointer to norm data
    \param[in] ThetaSamplesNorm   number of angles in norm plane
    \param[in] max_threads        maximum number of threads to use for the
                                  backprojection

    Calculate the bkproj(1/N) normalization matrix for NW-OSEM. A sinogram
    object is created and filled with the norm data. This method does
    the same as the one it overloads, except that the user doesn't handle
    sinogram objects.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcNormFactorsNW(float * const normdata,
                               const unsigned short int ThetaSamplesNorm,
                               const unsigned short int max_threads)
 { if (normdata == NULL)
    { calcNormFactorsUW(max_threads);
      return;
    }
   SinogramConversion *norm=NULL;

   try
   { norm=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamplesNorm,
                                 span, 0, plane_separation, 0, 0.0f, false, 0,
                                 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                 axes == 1);
     norm->copyData(normdata, 0, axes, axis_slices[0], 1, false, true, "norm",
                    loglevel);
     calcNormFactorsNW(norm, max_threads);
     delete norm;
     norm=NULL;
   }
   catch (...)
    { if (norm != NULL) delete norm;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate bkproj(1/(A*N)) normalization matrix.
    \param[in] acf_sino      acf object
    \param[in] norm          norm object
    \param[in] max_threads   maximum number of threads to use for the
                             backprojection

    Calculate the bkproj(1/(A*N)) normalization matrix for ANW-OSEM. The
    backprojector expects a padded sinogram. The acf data is rebinned to the
    size of the emission sinogram if needed, but the acf object is left
    unchanged. Norm values below \f$\epsilon\f$ and acf values below 1 are
    cut-off before backprojection:
    \f[
       \mbox{bckproj}\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{1}{norm} & (acf \le 1) \wedge (norm > \epsilon)\\
         \frac{1}{norm\cdot acf} & (acf > 1) \wedge (norm > \epsilon)\\
         0 & norm \le\epsilon
        \end{array}
        \right\}
    \f]
    The resulting matrix is stored by the memory controller and the
    indices of the memory segments are stored in the norm_factors array.

    The calculation is split into subsets and progress information is printed
    after every subsets. The same result can be achieved with one subset, but
    less progress informatiom will be printed.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcNormFactorsANW(SinogramConversion *acf_sino,
                                SinogramConversion *norm,
                                const unsigned short int max_threads)
 { if (acf_sino == NULL)
    { calcNormFactorsNW(norm, max_threads);
      return;
    }
   if (norm == NULL)
    { calcNormFactorsAW(acf_sino, max_threads);
      return;
    }
   RebinSinogram *rebin=NULL;
   StopWatch sw;

   try
   { std::string size_str;
     unsigned short int norm_prj_index, buffer_index;
     float *norm_prj, *buffer;

     initBckProjector(max_threads);
     flog->logMsg("start calculation of normalization factors for ANW-OSEM",
                  loglevel+1);
     sw.start();
      // subset of segment from 1/(A*N) sinogram for backprojection (view-mode)
     norm_prj=MemCtrl::mc()->createFloat(subset_size_padded, &norm_prj_index,
                                         "npj", loglevel+2);
     { double size;

       size=(double)(image_volume_size*(unsigned long int)subsets)*
            sizeof(float)/1024.0f;
       size_str=toString(size/1024.0f)+" MByte";
     }
     flog->logMsg("allocate #1 for normalization factors", loglevel+2)->
      arg(size_str);
     norm_factors.resize(subsets);
     if ((RhoSamples != acf_sino->RhoSamples()) ||
         (ThetaSamples != acf_sino->ThetaSamples()))
                                    // rebin attenuation to reconstruction size
      rebin=new RebinSinogram(acf_sino->RhoSamples(), acf_sino->ThetaSamples(),
                              RhoSamples, ThetaSamples, RebinX <float>::NO_ARC,
                              0, true);
     buffer=MemCtrl::mc()->createFloat((unsigned long int)RhoSamples*
                                       (unsigned long int)ThetaSamples*
                                       (unsigned long int)axis_slices[0],
                                       &buffer_index, "buf", loglevel+2);
                                                       // calculate all subsets
     for (unsigned short int subset=0; subset < subsets; subset++)
      { float *normfac;

        SyngoMsg(syngo_msg+": calculate normalization matrix (subset "+
                 toString(subset)+")");
        flog->logMsg("start calculation of normalization factors for subset "
                     "#1", loglevel+2)->arg(subset);
        sw.start();
        normfac=MemCtrl::mc()->createFloat(image_volume_size,
                                           &norm_factors[subset], "nfac",
                                           loglevel+2);
        memset(normfac, 0, image_volume_size*sizeof(float));
                                                      // calculate all segments
        for (unsigned short int segment=0; segment < segments; segment++)
         { unsigned short int t, planes, axis, attp_idx;
           float *np, *bpp, *ap_reb, *attp, *normp;

           axis=(segment+1)/2;
           if (segment == 0) planes=axis_slices[0];
            else planes=axis_slices[axis]/2;
                           // calculate subset of segment from 1/(A*N) sinogram
           flog->logMsg("calculate segment #1, subset #2 of 1/(a*n) sinogram",
                        loglevel+3)->arg(segStr(segment))->arg(subset);
           memset(norm_prj, 0, subset_size_padded*sizeof(float));

           normp=MemCtrl::mc()->getFloatRO(norm->index(axis, 0), loglevel+4);
           attp=MemCtrl::mc()->getFloatRO(acf_sino->index(axis, 0),
                                          loglevel+4);
           if ((segment > 0) && !(segment & 0x1))
            { normp+=(unsigned long int)planes*norm->planeSize();
              attp+=(unsigned long int)planes*acf_sino->planeSize();
            }
           if (rebin != NULL)
            { flog->logMsg("rebin segment #1 of acf from #2x#3x#4 to #5x#6x#7",
                           loglevel+3)->arg(segStr(segment))->
               arg(acf_sino->RhoSamples())->arg(acf_sino->ThetaSamples())->
               arg(planes)->arg(RhoSamples)->arg(ThetaSamples)->arg(planes);
              attp=rebin->calcRebinSinogram(attp, planes, NULL, &attp_idx,
                                            loglevel+3, max_threads);
              MemCtrl::mc()->put(acf_sino->index(axis, 0));
            }
           bpp=buffer;
           ap_reb=attp;
           for (unsigned short int p=0; p < planes; p++)
            { for (t=0; t < ThetaSamples; t++,
                   ap_reb+=RhoSamples,
                   bpp+=RhoSamples)
               { for (unsigned short int r=0; r < RhoSamples; r++)
                  if (normp[r] <= OSEM3D::epsilon) bpp[r]=0.0f;
                  else if (ap_reb[r] > 1.0f) bpp[r]=1.0f/(normp[r]*ap_reb[r]);
                         else bpp[r]=1.0f/normp[r];
                 if (norm->ThetaSamples() > 1) normp+=RhoSamples;
               }
              if (norm->ThetaSamples() == 1) normp+=RhoSamples;
            }
           if (rebin != NULL)
            { MemCtrl::mc()->put(attp_idx);
              MemCtrl::mc()->deleteBlock(&attp_idx);
            }
            else MemCtrl::mc()->put(acf_sino->index(axis, 0));
           MemCtrl::mc()->put(norm->index(axis, 0));
                                           // copy views for subset into buffer
           for (np=norm_prj+RhoSamples_padded,
                bpp=&buffer[(unsigned long int)subset*
                            (unsigned long int)RhoSamples],
                t=0; t < ThetaSamples/subsets; t++,
                bpp+=(unsigned long int)subsets*(unsigned long int)RhoSamples,
                np+=view_size_padded)
            { float *bp1, *np1;
              unsigned short int p;

              for (bp1=bpp,
                   np1=np+(unsigned long int)z_bottom[segment]*
                          (unsigned long int)RhoSamples_padded,
                   p=0; p < planes; p++,
                   np1+=RhoSamples_padded,
                   bp1+=sino_plane_size)
               memcpy(np1+1, bp1, RhoSamples*sizeof(float));
            }
                                          // backproject subset of this segment
           flog->logMsg("backproject segment #1, subset #2 of sinogram from "
                        "#3x#4x#5 to #6x#7x#8", loglevel+3)->
            arg(segStr(segment))->arg(subset)->arg(RhoSamples)->
            arg(ThetaSamples)->arg(planes)->arg(XYSamples)->arg(XYSamples)->
            arg(axis_slices[0]);
           bp->back_proj3d(norm_prj, normfac, subset, segment, true,
                           max_threads);
         }
        MemCtrl::mc()->put(norm_factors[subset]);
        flog->logMsg("finished calculation of normalization factors for subset"
                     " #1 in #2 sec", loglevel+2)->arg(subset)->arg(sw.stop());
      }
     MemCtrl::mc()->put(buffer_index);
     MemCtrl::mc()->deleteBlock(&buffer_index);
     MemCtrl::mc()->put(norm_prj_index);
     MemCtrl::mc()->deleteBlock(&norm_prj_index);
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     flog->logMsg("finished calculation of normalization factors in #1 sec",
                  loglevel+1)->arg(sw.stop());
   }
   catch (...)
    { sw.stop();
      if (rebin != NULL) delete rebin;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate bkproj(1/(A*N)) normalization matrix.
    \param[in] acfdata             pointer to acf data
    \param[in] RhoSamplesAtten     number of bins in acf projections
    \param[in] ThetaSamplesAtten   number of angles in acf plane
    \param[in] normdata           pointer to norm data
    \param[in] ThetaSamplesNorm   number of angles in norm plane
    \param[in] max_threads        maximum number of threads to use for the
                                  backprojection

    Calculate the bkproj(1/(A*N)) normalization matrix for ANW-OSEM. Sinogram
    objects are created and filled with the norm and acf data. This method does
    the same as the one it overloads, except that the user doesn't handle
    sinogram objects.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcNormFactorsANW(float * const acfdata,
                                const unsigned short int RhoSamplesAtten,
                                const unsigned short int ThetaSamplesAtten,
                                float * const normdata,
                                const unsigned short int ThetaSamplesNorm,
                                const unsigned short int max_threads)
 { if (acfdata == NULL)
    { calcNormFactorsNW(normdata, ThetaSamplesNorm, max_threads);
      return;
    }
   if (normdata == NULL)
    { calcNormFactorsAW(acfdata, RhoSamplesAtten, ThetaSamplesAtten,
                        max_threads);
      return;
    }
   SinogramConversion *acf=NULL, *norm=NULL;

   try
   { acf=new SinogramConversion(RhoSamplesAtten,
                                BinSizeRho*(float)RhoSamples/
                                (float)RhoSamplesAtten, ThetaSamplesAtten,
                                span, 0, plane_separation, 0, 0.0f, false, 0,
                                0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, axes == 1);
     acf->copyData(acfdata, 0, axes, axis_slices[0], 1, false, true, "acf",
                   loglevel);
     norm=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamplesNorm,
                                 span, 0, plane_separation, 0, 0.0f, false, 0,
                                 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                 axes == 1);
     norm->copyData(normdata, 0, axes, axis_slices[0], 1, false, true, "norm",
                    loglevel);
     calcNormFactorsANW(acf, norm, max_threads);
     delete norm;
     norm=NULL;
     delete acf;
     acf=NULL;
   }
   catch (...)
    { if (acf != NULL) delete acf;
      if (norm != NULL) delete norm;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate (smooth(r)*n+s)*a offset for OP-OSEM.
    \param[in,out] emi_sino   emission sinogram with prompt and normalized
                              smoothed randoms
    \param[in,out] scatter_sino   scatter sinogram
    \param[in,out] acf_sino       acf sinogram
    \param[in]     max_threads    maximum number of threasd to use for the
                                  rebinning

    Calculate (smooth(r)*n+s)*a offset for OP-OSEM. If the acf has a different
    size than the emission sinogram, it will be rebinned. If the scatter
    sinogram is 2d and the emission is 3d, iSSRB will be used to create a 3d
    scatter sinogram. The randoms, scatter and acf sinograms are deleted after
    use. In case of a TOF reconstruction the shift sinogram will be stored in
    shuffled format.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::calcOffsetOP(SinogramConversion *emi_sino,
                          SinogramConversion *scatter_sino,
                          SinogramConversion *acf_sino,
                          const unsigned short int max_threads)
 { RebinSinogram *rebin=NULL, *rebin_s=NULL;

   try
   { unsigned short int rnd_sino, tof_bins=1;

     rnd_sino=emi_sino->numberOfSinograms()-1;
 
     flog->logMsg("calculate (smooth(r)*n+s)*a", loglevel);
     for (unsigned short int i=0; i < offset_matrix.size(); i++)
      MemCtrl::mc()->deleteBlock(&offset_matrix[i]);
     if ((acf_sino->RhoSamples() != emi_sino->RhoSamples()) ||
         (acf_sino->ThetaSamples() != emi_sino->ThetaSamples()))
      { flog->logMsg("rebin acf from #1x#2x#3 to #4x#5x#6", loglevel+1)->
         arg(acf_sino->RhoSamples())->arg(acf_sino->ThetaSamples())->
         arg(acf_sino->axesSlices())->arg(emi_sino->RhoSamples())->
         arg(emi_sino->ThetaSamples())->arg(emi_sino->axesSlices());
        rebin=new RebinSinogram(acf_sino->RhoSamples(),
                                acf_sino->ThetaSamples(),
                                emi_sino->RhoSamples(),
                                emi_sino->ThetaSamples(),
                                RebinX <float>::NO_ARC, 0, true);
      }
     if ((scatter_sino->RhoSamples() != emi_sino->RhoSamples()) ||
         (scatter_sino->ThetaSamples() != emi_sino->ThetaSamples()))
      { flog->logMsg("rebin scatter from #1x#2x#3 to #4x#5x#6", loglevel+1)->
         arg(scatter_sino->RhoSamples())->arg(scatter_sino->ThetaSamples())->
         arg(scatter_sino->axesSlices())->arg(emi_sino->RhoSamples())->
         arg(emi_sino->ThetaSamples())->arg(emi_sino->axesSlices());
        rebin_s=new RebinSinogram(scatter_sino->RhoSamples(),
                                  scatter_sino->ThetaSamples(),
                                  emi_sino->RhoSamples(),
                                  emi_sino->ThetaSamples(),
                                  RebinX <float>::NO_ARC, 0, false);
      }
     offset_matrix.resize(axes);
     for (unsigned short int axis=0; axis < axes; axis++)
      { float *rp, *fp;
        unsigned long int axis_size, i;
        unsigned short int s;

        axis_size=emi_sino->size(axis);
                                                           // get smoothed(r)*n
        rp=MemCtrl::mc()->createFloat(axis_size*(unsigned long int)tof_bins,
                                      &offset_matrix[axis], "opof", loglevel);
        fp=MemCtrl::mc()->getFloatRO(emi_sino->index(axis, rnd_sino),
                                     loglevel);
        for (i=0; i < axis_size; i++)
         for (s=0; s < tof_bins; s++)
          rp[i*(unsigned long int)tof_bins+s]=fp[i];
        MemCtrl::mc()->put(emi_sino->index(axis, rnd_sino));
        emi_sino->deleteData(axis, rnd_sino);
                                               // add scatter: smooothed(r)*n+s
        for (s=0; s < tof_bins; s++)
         if (scatter_sino->axes() > 1)                   // 3d-scatter sinogram
          { if (rebin_s != NULL)
             { unsigned short int scat_idx;

               fp=rebin_s->calcRebinSinogram(
                        MemCtrl::mc()->getFloatRO(scatter_sino->index(axis, s),
                                                  loglevel), axis_slices[axis],
                        NULL, &scat_idx, loglevel, max_threads);
               for (i=0; i < axis_size; i++)
                rp[i*(unsigned long int)tof_bins+s]+=fp[i];
               MemCtrl::mc()->put(scat_idx);
               MemCtrl::mc()->deleteBlock(&scat_idx);
             }
             else { fp=MemCtrl::mc()->getFloatRO(scatter_sino->index(axis, s),
                                                 loglevel);
                    for (i=0; i < axis_size; i++)
                     rp[i*(unsigned long int)tof_bins+s]+=fp[i];
                  }
            MemCtrl::mc()->put(scatter_sino->index(axis, s));
            scatter_sino->deleteData(axis, s);
          }
          else {                                         // 2d-scatter sinogram
                 unsigned short int planes;
                 unsigned long int sc_offset;
                 float *sp;
                                // do inverse single-slice rebinning on the fly
                 if (axis == 0) { planes=axis_slices[0];
                                  sc_offset=0;
                                }
                  else { planes=axis_slices[axis]/2;
                         sc_offset=(unsigned long int)
                                   ((emi_sino->axisSlices()[0]-
                                     emi_sino->axisSlices()[axis]/2)/2)*
                                   (unsigned long int)emi_sino->RhoSamples()*
                                   (unsigned long int)emi_sino->ThetaSamples();
                       }
                 sp=MemCtrl::mc()->getFloatRO(scatter_sino->index(0, s),
                                              loglevel);
                 if (rebin_s != NULL)
                  { unsigned short int scat_idx;

                    sp=rebin_s->calcRebinSinogram(sp, planes, NULL, &scat_idx,
                                                  loglevel, max_threads);
                    if (axis == 0)
                     for (i=0; i < axis_size; i++)
                      rp[i*(unsigned long int)tof_bins+s]+=sp[i];
                     else { for (i=0; i < axis_size/2; i++)
                             rp[i*(unsigned long int)tof_bins+s]+=
                              sp[sc_offset+i];
                            for (i=0; i < axis_size/2; i++)
                             rp[axis_size/2+i*(unsigned long int)tof_bins+s]+=
                              sp[sc_offset+i];
                          }
                    MemCtrl::mc()->put(scat_idx);
                    MemCtrl::mc()->deleteBlock(&scat_idx);
                  }
                  else if (axis == 0)
                        for (i=0; i < axis_size; i++)
                         rp[i*(unsigned long int)tof_bins+s]+=sp[i];
                        else { for (i=0; i < axis_size/2; i++)
                                rp[i*(unsigned long int)tof_bins+s]+=
                                 sp[sc_offset+i];
                               for (i=0; i < axis_size/2; i++)
                                rp[axis_size/2+
                                   i*(unsigned long int)tof_bins+s]+=
                                 sp[sc_offset+i];
                             }
                 MemCtrl::mc()->put(scatter_sino->index(0, s));
               }
                                      // multiply with acf: (smoothed(r)*n+s)*a
         if (rebin != NULL)
          { unsigned short int acf_idx;

            fp=rebin->calcRebinSinogram(
                            MemCtrl::mc()->getFloatRO(acf_sino->index(axis, 0),
                                                      loglevel),
                            axis_slices[axis], NULL, &acf_idx, loglevel,
                            max_threads);
            for (i=0; i < axis_size; i++)
             for (s=0; s < tof_bins; s++)
              rp[i*(unsigned long int)tof_bins+s]*=fp[i];
            MemCtrl::mc()->put(acf_idx);
            MemCtrl::mc()->deleteBlock(&acf_idx);
          }
          else { fp=MemCtrl::mc()->getFloatRO(acf_sino->index(axis, 0),
                                              loglevel);
                 for (i=0; i < axis_size; i++)
                  for (s=0; s < tof_bins; s++)
                   rp[i*(unsigned long int)tof_bins+s]*=fp[i];
               }
        MemCtrl::mc()->put(acf_sino->index(axis, 0));
        acf_sino->deleteData(axis, 0);
        MemCtrl::mc()->put(offset_matrix[axis]);
      }
     for (unsigned short int s=0; s < tof_bins; s++)
      scatter_sino->deleteData(s);
     emi_sino->deleteData(rnd_sino);
     acf_sino->deleteData(0);
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     if (rebin_s != NULL) { delete rebin_s;
                            rebin_s=NULL;
                          }
   }
   catch (...)
    { if (rebin != NULL) delete rebin;
      if (rebin_s != NULL) delete rebin_s;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Factorize a number into a product of primes.
    \param[in] number   number to factorize
    \return vector with prime factors

    Factorize a number into a product of primes.
 */
/*---------------------------------------------------------------------------*/
std::vector <unsigned short int> OSEM3D::factorize(
                                               unsigned short int number) const
 { unsigned short int i, last;
   std::vector <unsigned short int> p, factors;

   factors.clear();
   p=primes(number+1);                             // get list of prime numbers
   for (last=0; number > 1;)
    for (i=last; i < p.size(); i++)
     if ((number % p[i]) == 0) // can number be devided by prime without rest ?
      { factors.push_back(p[i]);                          // store prime factor
        number/=p[i];                                 // devide number by prime
        last=i;             // try this prime or big ones in the next iteration
        break;
      }
   return(factors);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate 2d or 3d forward-projection of image.
    \param[in] image         image object
    \param[in] max_threads   maximum number of threads to use
    \return sinogram object

    Calculate 2d or 3d forward-projection of image. The method adds padding to
    the image, calculates the forward-projection into a padded sinogram and
    removes the padding from the sinogram. The input image remains unchanged.

    The calculation is split into subsets and progress information is printed
    after every subsets. The same result can be achieved with one subset, but
    less progress informatiom will be printed.
 */
/*---------------------------------------------------------------------------*/
SinogramConversion *OSEM3D::fwd_proj3d(ImageConversion * const image,
                                       const unsigned short int max_threads)
 { SinogramConversion *sinogram=NULL;
   StopWatch sw;

   try
   { unsigned short int tof_bins, img_index, estimate_index;
     float *img, *estimate;
     
     tof_bins=1;
     initFwdProjector(max_threads);
     flog->logMsg("start forward projection", loglevel);
     sw.start();
     sinogram=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamples,
                                     span, mrd, plane_separation,
                                     (axis_slices[0]+1)/2, 0.0f,
                                     false, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                     0, axes == 1);
     for (unsigned short int axis=0; axis < axes; axis++)
      { sinogram->initFloatAxis(axis, 0, tof_bins, true, false, "sino",
                                loglevel+1);
        MemCtrl::mc()->put(sinogram->index(axis, 0));
      }
                                         // copy image into padded image buffer
     img=MemCtrl::mc()->createFloat(image_volume_size_padded, &img_index,
                                    "img", loglevel+1);
     memset(img, 0, image_volume_size_padded*sizeof(float));
     { float *ip, *rp;

       ip=img+image_plane_size_padded+1;
       rp=MemCtrl::mc()->getFloatRO(image->index(), loglevel+2);
       for (unsigned short int z=0; z < axis_slices[0]; z++)
        { ip+=XYSamples_padded;
          for (unsigned short int y=0; y < XYSamples; y++,
               rp+=XYSamples,
               ip+=XYSamples_padded)
           memcpy(ip, rp, XYSamples*sizeof(float));
          ip+=XYSamples_padded;
        }
       MemCtrl::mc()->put(image->index());
     }
                                                 // forward project all subsets
     estimate=MemCtrl::mc()->createFloat(subset_size_padded_TOF,
                                         &estimate_index, "est", loglevel+1);
     for (unsigned short int subset=0; subset < subsets; subset++)
      {
        flog->logMsg("start forward projection of subset #1 from #2x#3x#4 to "
                      "#5x#6x#7", loglevel+1)->arg(subset)->arg(XYSamples)->
          arg(XYSamples)->arg(axis_slices[0])->arg(RhoSamples)->
          arg(ThetaSamples)->arg(axes_slices);
        sw.start();
        for (unsigned short int segment=0; segment < segments; segment++)
         { unsigned short int planes, t, axis;
           unsigned long int offs;
           float *tp, *ep, *sp;

           flog->logMsg("project segment #1", loglevel+2)->
            arg(segStr(segment));
           axis=(segment+1)/2;
           if (segment == 0) planes=axis_slices[0];
            else planes=axis_slices[axis]/2;
                               // forward-project subset of segment of sinogram
           fwd->forward_proj3d(img, estimate, subset, segment, max_threads);
           sp=MemCtrl::mc()->getFloat(sinogram->index(axis, 0), loglevel+2);
           if ((segment == 0) || (segment & 0x1)) offs=0;
            else offs=(unsigned long int)axis_slices[axis]/2*
                      sino_plane_size_TOF;
                                                // copy result into 3d sinogram
           for (tp=&sp[offs+(unsigned long int)subset*
                            (unsigned long int)RhoSamples_TOF],
                ep=&estimate[(unsigned long int)z_bottom[segment]*
                             (unsigned long int)RhoSamples_padded_TOF+
                             RhoSamples_padded_TOF+tof_bins],
                t=0; t < ThetaSamples/subsets; t++,
                ep+=view_size_padded_TOF,
                tp+=(unsigned long int)subsets*
                    (unsigned long int)RhoSamples_TOF)
            for (unsigned short int p=0; p < planes; p++)
             memcpy(tp+(unsigned long int)p*sino_plane_size_TOF,
                    ep+(unsigned long int)p*
                       (unsigned long int)RhoSamples_padded_TOF,
                    RhoSamples_TOF*sizeof(float));
           MemCtrl::mc()->put(sinogram->index(axis, 0));
         }
        flog->logMsg("finished forward projection of subset #1 in #2 sec",
                     loglevel+1)->arg(subset)->arg(sw.stop());
      }
     MemCtrl::mc()->put(estimate_index);
     MemCtrl::mc()->deleteBlock(&estimate_index);
     MemCtrl::mc()->put(img_index);
     MemCtrl::mc()->deleteBlock(&img_index);
     flog->logMsg("scale #1 planes of sinogram by #2", loglevel+1)->
      arg(axes_slices)->arg(1.0f/DeltaXY);
     sinogram->scale(1.0f/DeltaXY, 0, loglevel+1);            // scale sinogram
     flog->logMsg("finished forward projection in #1 sec", loglevel)->
      arg(sw.stop());
     return(sinogram);
   }
   catch (...)
    { sw.stop();
      if (sinogram != NULL) delete sinogram;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate 2d or 3d forward-projection of image.
    \param[in] img           pointer to image
    \param[in] max_threads   maximum number of threads to use
    \return pointer to sinogram data

    Calculate 2d or 3d forward-projection of image. An image object is
    created and filled with the image data. The resulting sinogram object is
    deleted and a pointer to the flat sinogram data is returned. This method
    does the same as the one it overloads, except that the user doesn't handle
    sinogram and image objects.
 */
/*---------------------------------------------------------------------------*/
float *OSEM3D::fwd_proj3d(float *img, const unsigned short int max_threads)
 { ImageConversion *image=NULL;
   SinogramConversion *sinogram=NULL;
   float *sino=NULL, *sptr=NULL;

   try
   { float *sp;

     image=new ImageConversion(XYSamples, DeltaXY, axis_slices[0],
                               plane_separation, 0, 0, 0, 0.0f, 0.0f, 0.0f,
                               0.0f, 0.0f);
     image->imageData(&img, false, false, "img", loglevel);
     sinogram=fwd_proj3d(image, max_threads);
     image->getRemove(loglevel);
     delete image;
     image=NULL;
     sino=new float[sinogram->size()];
     sp=sino;
     for (unsigned short int axis=0; axis < sinogram->axes(); axis++)
      { unsigned short int idx;

        idx=sinogram->index(axis, 0);
        sptr=MemCtrl::mc()->getRemoveFloat(&idx, loglevel);
        memcpy(sp, sptr, sinogram->size(axis)*sizeof(float));
        delete[] sptr;
        sptr=NULL;
        sp+=sinogram->size(axis);
      }
     delete sinogram;
     sinogram=NULL;
     return(sino);
   }
   catch (...)
    { if (image != NULL) delete image;
      if (sinogram != NULL) delete sinogram;
      if (sino != NULL) delete[] sino;
      if (sptr != NULL) delete[] sptr;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate Herman-Meyer permutation of a sequence.
    \param[in] number   upper threshold for sequence
    \return vector with permutated sequence

    Calculate Herman-Meyer permutation of a sequence which is a good sequence
    for the subsets in an OSEM reconstruction. The next subset will always have
    a largedistance to all previous subsets.

    The method first calculates all prime factors that are smaller or equal to
    a given number. E.g. for 30 (subsets) we get the prime factors 2,3 and 5
    (2*3*5=30). These numbers are the bases of a numbering system. The numbers
    of this system are shown in the second column of the following table, with
    the decimal equivalent in the first column.

    <TABLE>
     <TR><TD>decimal</TD><TD>hm-system</TD><TD>flipped</TD><TD>decimal</TD>
     </TR>
     <TR><TD>0</TD><TD>(0,0,0)</TD><TD>(0,0,0)</TD><TD>0*(5*3)+0*(5)+0=0</TD>
     </TR>
     <TR><TD>1</TD><TD>(0,0,1)</TD><TD>(1,0,0)</TD><TD>1*(5*3)+0*(5)+0=15</TD>
     </TR>
     <TR><TD>2</TD><TD>(0,1,0)</TD><TD>(0,1,0)</TD><TD>0*(5*3)+1*(5)+0=5</TD>
     </TR>
     <TR><TD>3</TD><TD>(0,1,1)</TD><TD>(1,1,0)</TD><TD>1*(5*3)+1*(5)+0=20</TD>
     </TR>
     <TR><TD>4</TD><TD>(0,2,0)</TD><TD>(0,2,0)</TD><TD>0*(5*3)+2*(5)+0=10</TD>
     </TR>
     <TR><TD>5</TD><TD>(0,2,1)</TD><TD>(1,2,0)</TD><TD>1*(5*3)+2*(5)+0=25</TD>
     </TR>
     <TR><TD>6</TD><TD>(1,0,0)</TD><TD>(0,0,1)</TD><TD>0*(5*3)+0*(5)+1=1</TD>
     </TR>
     <TR><TD>7</TD><TD>(1,0,1)</TD><TD>(1,0,1)</TD><TD>1*(5*3)+0*(5)+1=16</TD>
     </TR>
     <TR><TD>8</TD><TD>(1,1,0)</TD><TD>(0,1,1)</TD><TD>0*(5*3)+1*(5)+1=6</TD>
     </TR>
     <TR><TD>9</TD><TD>(1,1,1)</TD><TD>(1,1,1)</TD><TD>1*(5*3)+1*(5)+1=21</TD>
     </TR>
     <TR><TD>10</TD><TD>(1,2,0)</TD><TD>(0,2,1)</TD><TD>0*(5*3)+2*(5)+1=11</TD>
     </TR>
     <TR><TD>11</TD><TD>(1,2,1)</TD><TD>(1,2,1)</TD><TD>1*(5*3)+2*(5)+1=26</TD>
     </TR>
     <TR><TD>12</TD><TD>(2,0,0)</TD><TD>(0,0,2)</TD><TD>0*(5*3)+0*(5)+2=2</TD>
     </TR>
     <TR><TD>13</TD><TD>(2,0,1)</TD><TD>(1,0,2)</TD><TD>1*(5*3)+0*(5)+2=17</TD>
     </TR>
     <TR><TD>14</TD><TD>(2,1,0)</TD><TD>(0,1,2)</TD><TD>0*(5*3)+1*(5)+2=7</TD>
     </TR>
     <TR><TD>15</TD><TD>(2,1,1)</TD><TD>(1,1,2)</TD><TD>1*(5*3)+1*(5)+2=22</TD>
     </TR>
     <TR><TD>16</TD><TD>(2,2,0)</TD><TD>(0,2,2)</TD><TD>0*(5*3)+2*(5)+2=12</TD>
     </TR>
     <TR><TD>17</TD><TD>(2,2,1)</TD><TD>(1,2,2)</TD><TD>1*(5*3)+2*(5)+2=27</TD>
     </TR>
     <TR><TD>18</TD><TD>(3,0,0)</TD><TD>(0,0,3)</TD><TD>0*(5*3)+0*(5)+3=3</TD>
     </TR>
     <TR><TD>19</TD><TD>(3,0,1)</TD><TD>(1,0,3)</TD><TD>1*(5*3)+0*(5)+3=18</TD>
     </TR>
     <TR><TD>20</TD><TD>(3,1,0)</TD><TD>(0,1,3)</TD><TD>0*(5*3)+1*(5)+3=8</TD>
     </TR>
     <TR><TD>21</TD><TD>(3,1,1)</TD><TD>(1,1,3)</TD><TD>1*(5*3)+1*(5)+3=23</TD>
     </TR>
     <TR><TD>22</TD><TD>(3,2,0)</TD><TD>(0,2,3)</TD><TD>0*(5*3)+2*(5)+3=13</TD>
     </TR>
     <TR><TD>23</TD><TD>(3,2,1)</TD><TD>(1,2,3)</TD><TD>1*(5*3)+2*(5)+3=28</TD>
     </TR>
     <TR><TD>24</TD><TD>(4,0,0)</TD><TD>(0,0,4)</TD><TD>0*(5*3)+0*(5)+4=4</TD>
     </TR>
     <TR><TD>25</TD><TD>(4,0,1)</TD><TD>(1,0,4)</TD><TD>1*(5*3)+0*(5)+4=19</TD>
     </TR>
     <TR><TD>26</TD><TD>(4,1,0)</TD><TD>(0,1,4)</TD><TD>0*(5*3)+1*(5)+4=9</TD>
     </TR>
     <TR><TD>27</TD><TD>(4,1,1)</TD><TD>(1,1,4)</TD><TD>1*(5*3)+1*(5)+4=24</TD>
     </TR>
     <TR><TD>28</TD><TD>(4,2,0)</TD><TD>(0,2,4)</TD><TD>0*(5*3)+2*(5)+4=14</TD>
     </TR>
     <TR><TD>29</TD><TD>(4,2,1)</TD><TD>(1,2,4)</TD><TD>1*(3*5)+2*(5)+4=29</TD>
     </TR>
     </TABLE>

    Flipping the number in the hm-system leads to the third column in the
    table. The permutation is finally calculated by a bijective transformation
    of the flipped hm-numbers into decimal numbers, shown in the last column.
    The \f$n\f$-th digit of the hm-number is multiplied with the product of the
    \f$p-n\f$ largest prime factors, where \f$p\f$ is the number of prime
    factors.

    For the more common number of 8 susets the prime factors will be 2, 2 and
    2, resulting in the permutation shown in the next table.
 
    <TABLE>
     <TR><TD>decimal</TD><TD>hm-system</TD><TD>flipped</TD><TD>decimal</TD>
     </TR>
     <TR><TD>0</TD><TD>(0,0,0)</TD><TD>(0,0,0)</TD><TD>0*(2*2)+0*(2)+0=0</TD>
     </TR>
     <TR><TD>0</TD><TD>(0,0,1)</TD><TD>(1,0,0)</TD><TD>1*(2*2)+0*(2)+0=4</TD>
     </TR>
     <TR><TD>0</TD><TD>(0,1,0)</TD><TD>(0,1,0)</TD><TD>0*(2*2)+1*(2)+0=2</TD>
     </TR>
     <TR><TD>0</TD><TD>(0,1,1)</TD><TD>(1,1,0)</TD><TD>1*(2*2)+1*(2)+0=6</TD>
     </TR>
     <TR><TD>0</TD><TD>(1,0,0)</TD><TD>(0,0,1)</TD><TD>0*(2*2)+0*(2)+1=1</TD>
     </TR>
     <TR><TD>0</TD><TD>(1,0,1)</TD><TD>(1,0,1)</TD><TD>1*(2*2)+0*(2)+1=5</TD>
     </TR>
     <TR><TD>0</TD><TD>(1,1,0)</TD><TD>(0,1,1)</TD><TD>0*(2*2)+1*(2)+1=3</TD>
     </TR>
     <TR><TD>0</TD><TD>(1,1,1)</TD><TD>(1,1,1)</TD><TD>1*(2*2)+1*(2)+1=7</TD>
     </TR>
    </TABLE>

    \note see also "Algebraic Reconstruction Techniques Can Be Made
          Computationally Efficient", G. T. Herman and Lorraine B. Meyer, IEEE
          Transactions on Medical Imaging, vol. 12, no. 3, pp. 600-609, 1993
 */
/*---------------------------------------------------------------------------*/
std::vector <unsigned short int> OSEM3D::herman_meyer_permutation(
                                                     unsigned short int number)
 { unsigned short int i;
   std::vector <unsigned short int> prime_factors, values, factors, counter;

   if (number == 1) { values.resize(1);
                      values[0]=0;
                      return(values);
                    }
   prime_factors=factorize(number);              // get prime factors of number
                          // build the factors for the bijective transformation
   factors.resize(prime_factors.size());
   factors[0]=1;
   for (i=1; i < prime_factors.size(); i++)
    factors[i]=prime_factors[prime_factors.size()-i]*factors[i-1];
                                        // set counter in numbering system to 0
   counter.assign(prime_factors.size(), 0);
                                               // calculate permutated sequence
   values.resize(number);
   for (i=0; i < number; i++)
    {                       // calculate next number in sequence as dot product
      for (unsigned short int k=0; k < prime_factors.size(); k++)
       values[i]+=counter[k]*factors[k];
                                                           // increment counter
      for (signed short int j=prime_factors.size()-1; j >= 0; j--)
       if (++counter[j] < prime_factors[prime_factors.size()-j-1]) break;
        else counter[j]=0;
    }
   return(values);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object, look-up-tables and image masks.
    \param[in] _XYSamples          width/height of image
    \param[in] _DeltaXY            with/height of voxels in mm
    \param[in] _RhoSamples         number of bins in projections
    \param[in] _BinSizeRho         width of bins in projections in mm
    \param[in] _ThetaSamples       number of angles in sinogram
    \param[in] _axis_slices        number of planes for axes of 3d sinogram
    \param[in] _span               span of sinogram
    \param[in] _mrd                maximum ring difference of sinogram
    \param[in] _plane_separation   plane separation in sinogram in mm
    \param[in] reb_factor          sinogram rebinning factor
    \param[in] _subsets            number of subsets for reconstruction
    \param[in] _algo               reconstruction algorithm
    \param[in] _lut_level          0=don't use lookup-tables
                                   1=use small lookup-table
                                   2=use large lookup-tables (only for 3d OSEM)
    \param[in] slaves              IP numbers of slave computers to use
    \param[in] _loglevel           top level for logging
    \exception REC_CLIENT_DOESNT_CONNECT client process doesn't connect to
                                         TCP/IP socket of server

    Initialize object, look-up-tables and image masks. Connections to the OSEM
    slaves are established if the code is running on a cluster.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::init(const unsigned short int _XYSamples, const float _DeltaXY,
                  const unsigned short int _RhoSamples,
                  const float _BinSizeRho,
                  const unsigned short int _ThetaSamples,
                  const std::vector <unsigned short int> _axis_slices,
                  const unsigned short int _span,
                  const unsigned short int _mrd,
                  const float _plane_separation,
                  const float _reb_factor, const unsigned short int _subsets,
                  const ALGO::talgorithm _algo,
                  const unsigned short int _lut_level,
                  std::list <tnode> * const slaves,
                  const unsigned short int _loglevel)
 { bp=NULL;
   fwd=NULL;
   wbuffer=NULL;
   rbuffer=NULL;
   os_slave.clear();
   norm_factors.clear();
   offset_matrix.clear();
   try
   { flog=Logging::flog();
     algorithm=_algo;
     XYSamples=_XYSamples;
     DeltaXY=_DeltaXY;
     RhoSamples=_RhoSamples;
     BinSizeRho=_BinSizeRho;
     ThetaSamples=_ThetaSamples;
     reb_factor=_reb_factor;
     subsets=_subsets;
     lut_level=_lut_level;
     span=_span;
     mrd=_mrd;
     plane_separation=_plane_separation;
     loglevel=_loglevel;
     axes=_axis_slices.size();
     axis_slices=_axis_slices;
                                                            // print parameters
     flog->logMsg("XYSamples=#1", loglevel)->arg(XYSamples);
     flog->logMsg("DeltaXY=#1mm", loglevel)->arg(DeltaXY);
     flog->logMsg("RhoSamples=#1", loglevel)->arg(RhoSamples);
     flog->logMsg("BinSizeRho=#1mm", loglevel)->arg(BinSizeRho);
     flog->logMsg("ThetaSamples=#1", loglevel)->arg(ThetaSamples);
     if (axes > 1)
      { flog->logMsg("span=#1", loglevel)->arg(span);
        flog->logMsg("maximum ring difference=#1", loglevel)->arg(mrd);
      }
     flog->logMsg("plane separation=#1mm", loglevel)->arg(plane_separation);
     flog->logMsg("shift center of rotation by #1 bins (sinogram was rebinned "
                  "by factor #2)", loglevel)->arg(0.5f-0.5f/reb_factor)->
      arg(reb_factor);
     flog->logMsg("subsets=#1", loglevel)->arg(subsets);
     if ((ThetaSamples % subsets) > 0)           // number of subsets allowed ?
      { unsigned short int os;

        os=subsets;
        while ((ThetaSamples % subsets) > 0) subsets++;
        flog->logMsg("*** number of subsets increased to #1 (#2 % #3 > 0)",
                     loglevel+1)->arg(subsets)->arg(ThetaSamples)->arg(os);
      }
     flog->logMsg("axes=#1", loglevel)->arg(axes);
     { std::string str;

       str=toString(axis_slices[0]);
       axes_slices=axis_slices[0];
       for (unsigned short int i=1; i < axes; i++)
        { str+=","+toString(axis_slices[i]);
          axes_slices+=axis_slices[i];
        }
       flog->logMsg("axis table=#1 {#2}", loglevel)->arg(axes_slices)->
        arg(str);
     }
     switch (algorithm)
      { case ALGO::OSEM_UW_Algo:
         flog->logMsg("algorithm=UW-OSEM", loglevel);
         syngo_msg="UW-OSEM";
         break;
        case ALGO::OSEM_AW_Algo:
         flog->logMsg("algorithm=AW-OSEM", loglevel);
         syngo_msg="AW-OSEM";
         break;
        case ALGO::OSEM_NW_Algo:
         flog->logMsg("algorithm=NW-OSEM", loglevel);
         syngo_msg="NW-OSEM";
         break;
        case ALGO::OSEM_ANW_Algo:
         flog->logMsg("algorithm=ANW-OSEM", loglevel);
         syngo_msg="ANW-OSEM";
         break;
        case ALGO::OSEM_OP_Algo:
         flog->logMsg("algorithm=OP-OSEM", loglevel);
         syngo_msg="OP-OSEM";
         break;
        default:
         break;
      }
     if (axes > 1) syngo_msg="3D-"+syngo_msg;
      else syngo_msg="2D-"+syngo_msg;
     SyngoMsg(syngo_msg+": initialize");
     switch (lut_level)
      { case 0:
         flog->logMsg("use LUT=no", loglevel);
         break;
        case 1:
         flog->logMsg("use LUT=small", loglevel);
         break;
        case 2:
         flog->logMsg("use LUT=large", loglevel);
         break;
      }
                                                           // start OSEM slaves
     if (slaves != NULL)
      if (slaves->size() > 0)
       { std::list <tnode>::iterator it;
         unsigned short int num=0;
         hostent *h;
         std::string om_ip, local_name;

#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
         { struct utsname un;
                                             // get IP number of local computer
           uname(&un);
           local_name=std::string(un.nodename);
         }
#endif
#ifdef WIN32
         { DWORD size=MAX_COMPUTERNAME_LENGTH+1;
           TCHAR name[MAX_COMPUTERNAME_LENGTH+1];
            USES_CONVERSION;
           GetComputerName(name, &size);
           local_name=std::string(T2A(name));
         }
#endif
         h=gethostbyname(local_name.c_str());
         if (h->h_addr_list != NULL)
          { unsigned char *p;

            p=(unsigned char *)h->h_addr_list[0];
            om_ip=toString((unsigned short int)p[0])+"."+
                  toString((unsigned short int)p[1])+"."+
                  toString((unsigned short int)p[2])+"."+
                  toString((unsigned short int)p[3]);
                                // create buffers for communication with slaves
            rbuffer=new StreamBuffer(OSEM3D::OS_RECV_BUFFER_SIZE);
            wbuffer=new StreamBuffer(OSEM3D::OS_SEND_BUFFER_SIZE);
                                                                // start slaves
            for (it=slaves->begin(); it != slaves->end(); ++it,
                 num++)
             { flog->logMsg("install OSEM slave on #1", loglevel)->
                arg((*it).ip);
               if (RED_ping((*it).ip))
                { RED_install((*it).ip, OSEM3D::slave_executable, (*it).path);
                  os_slave.push_back(new CommSocket(rbuffer, wbuffer));
                  flog->logMsg("start OSEM slave on #1", loglevel)->
                   arg((*it).ip);
                  RED_execute((*it).ip,
                              (*it).path+"/"+OSEM3D::slave_executable,
                              om_ip+" "+toString(os_slave[num]->portNumber())+
                              " "+toString(num));
                  if (!os_slave[num]->waitForConnect(10, 0))
                   throw Exception(REC_CLIENT_DOESNT_CONNECT,
                               "Client process '#1' doesn't connect to TCP/IP "
                               " socket of server.").
                          arg(OSEM3D::slave_executable);
                }
                else flog->logMsg("installation failed on #1", loglevel)->
                      arg((*it).ip);
             }
          }
                                // tell slaves to initialize their OSEM objects
         for (unsigned short int i=0; i < os_slave.size(); i++)
          { os_slave[i]->newMessage();
            *(os_slave[i]) << OS_CMD_INIT_OSEM << XYSamples << DeltaXY
                           << RhoSamples << BinSizeRho << ThetaSamples
                           << axes;
            os_slave[i]->writeData(&_axis_slices[0], axes);
            *(os_slave[i]) << span << plane_separation
                           << GM::gm()->ringRadius() << GM::gm()->DOI()
                           << reb_factor << subsets << (int)algorithm
                           << lut_level << std::endl;
          }
       }
                                                      // size of sinogram plane
     sino_plane_size=(unsigned long int)RhoSamples*
                     (unsigned long int)ThetaSamples;
                                                     // calculate segment table
     segments=2*axes-1;
     seg_plane_offset.resize(segments);
     z_bottom.resize(segments);
     z_top.resize(segments);
     seg_plane_offset[0]=0;          // first plane of segment 0 in 3d sinogram
                                           // first and last plane of segment 0
     z_bottom[0]=0;
     z_top[0]=axis_slices[0]-1;
     for (unsigned short int axis=1; axis < axes; axis++)
      { unsigned short int sl;
                     // first and last planes of segments relative to segment 0
        z_bottom[axis*2]=(axis_slices[0]-axis_slices[axis]/2)/2;
        z_top[axis*2]=z_bottom[axis*2]+axis_slices[axis]/2-1;
        z_bottom[axis*2-1]=z_bottom[axis*2];
        z_top[axis*2-1]=z_top[axis*2];
                            // first planes of segments relative to 3d sinogram
        sl=0;
        for (unsigned short int i=0; i < axis; i++)
         sl+=axis_slices[i];
        seg_plane_offset[axis*2-1]=sl;
        seg_plane_offset[axis*2]=sl+axis_slices[axis]/2;
      }
                                                         // size of image plane
     image_plane_size=(unsigned long int)XYSamples*
                      (unsigned long int)XYSamples;
                                                        // size of image volume
     image_volume_size=image_plane_size*
                       (unsigned long int)axis_slices[0];

     XYSamples_padded=XYSamples+2;
     image_plane_size_padded=(unsigned long int)XYSamples_padded*
                             (unsigned long int)XYSamples_padded;
     image_volume_size_padded=image_plane_size_padded*
                              (unsigned long int)(axis_slices[0]+2);
     view_size=(unsigned long int)axis_slices[0]*
               (unsigned long int)RhoSamples;
     RhoSamples_padded=RhoSamples+2;
     view_size_padded=(unsigned long int)(axis_slices[0]+2)*
                      (unsigned long int)RhoSamples_padded;
     subset_size_padded=(unsigned long int)(ThetaSamples/subsets)*
                        view_size_padded;
     RhoSamples_TOF=RhoSamples;
     RhoSamples_padded_TOF=RhoSamples_padded;
     view_size_padded_TOF=view_size_padded;
     sino_plane_size_TOF=sino_plane_size;
     subset_size_padded_TOF=subset_size_padded;
                                                      // initialize image masks
     { float radius_fov, final_fov;

       radius_fov=0.95f*DeltaXY*(float)(XYSamples/2);
       final_fov=0.94f*DeltaXY*(float)((XYSamples-4)/2);
       fov_diameter=final_fov*2.0f;
       initMask(radius_fov, final_fov);
     }
     order=herman_meyer_permutation(subsets);            // ordering of subsets
   }
   catch (...)
    { if (bp != NULL) { delete bp;
                        bp=NULL;
                      }
      if (wbuffer != NULL) { delete wbuffer;
                             wbuffer=NULL;
                           }
      if (rbuffer != NULL) { delete rbuffer;
                             rbuffer=NULL;
                           }
      { std::vector <CommSocket *>::iterator it;

        for (it=os_slave.begin(); it != os_slave.end(); ++it)
         if (*it != NULL) { terminateSlave(*it);
                            delete *it;
                          }
      }
      os_slave.clear();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize back-projector.
    \param[in] max_threads   maximum number of threads to use by back-projector

    Initialize back-projector.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::initBckProjector(const unsigned short int max_threads)
 {                                                  // initialize backprojector
   if (bp == NULL)
    bp=new BckPrj3D(XYSamples, DeltaXY, RhoSamples, BinSizeRho, ThetaSamples,
                    axis_slices, axes, span, plane_separation, reb_factor,
                    subsets, lut_level, loglevel, max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize forward-projector.
    \param[in] max_threads   maximum number of threads to use by
                             forward-projector

    Initialize forward-projector.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::initFwdProjector(const unsigned short int max_threads)
 {                                              // initialize forward-projector
   if (fwd == NULL)
    fwd=new FwdPrj3D(XYSamples, DeltaXY, RhoSamples, BinSizeRho, ThetaSamples,
                     axis_slices, axes, span, plane_separation, reb_factor,
                     subsets, lut_level, loglevel, max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize starting image.
    \param[out] image_index   memory controller index for image
    \param[in]  loglevel      level for logging
    \return pointer to image data

    Initialize starting image with 1 and apply circular mask.
 */
/*---------------------------------------------------------------------------*/
float *OSEM3D::initImage(unsigned short int * const image_index,
                         const unsigned short int loglevel) const
 { float *ip, *image;

   image=MemCtrl::mc()->createFloat(image_volume_size_padded, image_index,
                                    "img", loglevel);
   memset(image, 0, image_volume_size_padded*sizeof(float));
   ip=image+image_plane_size_padded;
                                            // init image with 1 and apply mask
   for (unsigned short int z=0; z < axis_slices[0]; z++)
    { ip+=XYSamples_padded;
      for (unsigned short int y=0; y < XYSamples; y++,
           ip+=XYSamples_padded)
       for (unsigned short int x=mask[y].start+1; x <= mask[y].end+1; x++)
        ip[x]=1.0f;
      ip+=XYSamples_padded;
    }
   return(image);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate circular masks for image.
    \param[in] radius_fov   radius of field-of-view (iteration mask)
    \param[in] final_fov    radius of field-of-view (final mask)

    Calculate circular masks for image. The centers of the masks are in the
    center of the image matrix. The masks are stored by the index of the first
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
void OSEM3D::initMask(const float radius_fov, const float final_fov)
 { float xy_off, rf, ff;

   rf=radius_fov*radius_fov;
   ff=final_fov*final_fov;
   xy_off=(float)(XYSamples/2);
   mask.resize(XYSamples);
   mask2.resize(XYSamples);
   for (unsigned short int y=0; y < XYSamples; y++)
    { float yv;
      bool found_in=false, found_in2=false;

      yv=((float)y-xy_off+0.5f)*DeltaXY;
      yv*=yv;
      mask[y].start=XYSamples;
      mask[y].end=0;
      mask2[y].start=XYSamples;
      mask2[y].end=0;
                          // find first and last voxel of circular mask in line
      for (unsigned short int x=0; x < XYSamples; x++)
       { float xv;

         xv=((float)x-xy_off+0.5f)*DeltaXY;
         if (xv*xv+yv <= rf)
          { if (!found_in) { found_in=true;
                             mask[y].start=x;
                           }
            mask[y].end=x;
          }
         if (xv <= ff)
          { if (!found_in2) { found_in2=true;
                              mask2[y].start=x;
                            }
            mask2[y].end=x;
          }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize look-up tables for TOF reconstruction.
    \param[in] tof_width   width of a TOF bin in ns
    \param[in] tof_fwhm    fwhm of time-of-flight-gaussian in ns
    \param[in] tof_bins    number of time-of-flight bins

    Initialize look-up tables for TOF reconstruction.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::initTOF(const float tof_width, const float tof_fwhm,
                     const unsigned short int tof_bins)
 {
   flog->logMsg("time of flight bins=#1", loglevel)->arg(tof_bins);
   if (tof_bins > 1)
    { Logging::flog()->logMsg("TOF width=#1ns", loglevel)->arg(tof_width);
      Logging::flog()->logMsg("TOF FWHM=#1ns", loglevel)->arg(tof_fwhm);
    }
   RhoSamples_TOF=RhoSamples*tof_bins;
   RhoSamples_padded_TOF=RhoSamples_TOF+tof_bins*2;
   view_size_padded_TOF=(unsigned long int)(axis_slices[0]+2)*
                        (unsigned long int)RhoSamples_padded_TOF;
   sino_plane_size_TOF=(unsigned long int)RhoSamples_TOF*
                       (unsigned long int)ThetaSamples;
   subset_size_padded_TOF=(unsigned long int)(ThetaSamples/subsets)*
                          view_size_padded_TOF;
   // tof_lut->plotTOF();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate maximum intensity projection of image.
    \param[in] image         pointer to image
    \param[in] max_threads   maximum number of threads to use
    \return projection images

    Calculate maximum intensity projection of image. The method adds padding to
    the image, calculates the forward-projection with the MIP-projector into a
    padded MIP image and removes the padding from the image. The input image
    remains unchanged.

    The calculation is split into subsets and progress information is printed
    after every subsets. The same result can be achieved with one subset, but
    less progress informatiom will be printed.
 */
/*---------------------------------------------------------------------------*/
float *OSEM3D::mip_proj(float * const image,
                        const unsigned short int max_threads)
 { float *estimate=NULL, *img=NULL, *mip=NULL;
   StopWatch sw;

   try
   { initFwdProjector(max_threads);
     flog->logMsg("start MIP projection of sinogram", loglevel+1);
     sw.start();
     mip=new float[sino_plane_size*(unsigned long int)axes_slices];
                                         // copy image into padded image buffer
     img=new float[image_volume_size_padded];
     memset(img, 0, image_volume_size_padded*sizeof(float));
     { float *ip, *rp;

       ip=img+image_plane_size_padded+1;
       rp=image;
       for (unsigned short int z=0; z < axis_slices[0]; z++)
        { ip+=XYSamples_padded;
          for (unsigned short int y=0; y < XYSamples; y++,
               rp+=XYSamples,
               ip+=XYSamples_padded)
           memcpy(ip, rp, XYSamples*sizeof(float));
          ip+=XYSamples_padded;
        }
     }
                                                 // forward project all subsets
     estimate=new float[subset_size_padded];
     for (unsigned short int subset=0; subset < subsets; subset++)
      { unsigned short int t;
        float *tp, *ep;

        flog->logMsg("start MIP projection of subset #1 from #2x#3x#4 to "
                     "#5x#6x#7", loglevel+2)->arg(subset)->arg(XYSamples)->
         arg(XYSamples)->arg(axis_slices[0])->arg(RhoSamples)->
         arg(ThetaSamples)->arg(axis_slices[0]);
        sw.start();
                                                      // forward-project subset
        fwd->mip_proj(img, estimate, subset, max_threads);
                                                  // copy result into mip image
        for (tp=&mip[(unsigned long int)seg_plane_offset[0]*
                     sino_plane_size+(unsigned long int)subset*
                                     (unsigned long int)RhoSamples],
             ep=&estimate[(unsigned long int)z_bottom[0]*
                          (unsigned long int)RhoSamples_padded+1],
             t=0; t < ThetaSamples/subsets; t++,
             ep+=view_size_padded,
             tp+=(unsigned long int)subsets*
                 (unsigned long int)RhoSamples)
         for (unsigned short int p=0; p < axis_slices[0]; p++)
          memcpy(tp+(unsigned long int)p*sino_plane_size,
                 ep+(unsigned long int)(p+1)*
                    (unsigned long int)RhoSamples_padded,
                 RhoSamples*sizeof(float));
        flog->logMsg("finished MIP projection of subset #1 in #2 sec",
                     loglevel+2)->arg(subset)->arg(sw.stop());
      }
     delete[] estimate;
     estimate=NULL;
     delete[] img;
     img=NULL;
     flog->logMsg("finished MIP projection of sinogram in #1 sec",
                  loglevel+1)->arg(sw.stop());
     return(mip);
   }
   catch (...)
    { sw.stop();
      if (estimate != NULL) delete[] estimate;
      if (img != NULL) delete[] img;
      if (mip != NULL) delete[] mip;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate sequence of prime numbers.
    \param[in] max   upper threshold for sequence
    \return vector with prime numbers in ascending order

    Calculate sequence of prime numbers. First all odd number are assumed to
    be prime. The first odd prime number is 3. All odd multiples of 3 are not
    prime, i.e. 9, 15, 21,... The next odd number that is still known to be
    prime is 5. The odd multiples are 15, 25, 35,...
    9 is the first odd number that is not prime, since it is a odd multiple of
    3.
 */
/*---------------------------------------------------------------------------*/
std::vector <unsigned short int> OSEM3D::primes(
                                            const unsigned short int max) const
 { unsigned short int mom, count;
   std::vector <bool> is_prime;
   std::vector <unsigned short int> prime;

   prime.clear();
   if (max < 2) return(prime);
   count=1;                          // we will find at least 1 prime number: 2
   is_prime.assign(max/2, true);              // mark every odd number as prime
                                    // check for odd numbers that are not prime
   for (unsigned short int i=3; i < max; i+=2)
    if (is_prime[i/2-1])               // is number still assumed to be prime ?
     { count++;
                             // mark all odd multiples of a number as not prime
       for (mom=3*i; mom < max; mom+=i*2)
        is_prime[mom/2-1]=false;
     }
                                 // all numbers that are still marked are prime
   prime.resize(count);
   prime[0]=2;                                // add the only even prime number
   for (mom=1,
        count=3; count < max; count+=2)
    if (is_prime[count/2-1]) prime[mom++]=count;
   return(prime);
 }

/*---------------------------------------------------------------------------*/
/*! \brief OSEM reconstruction.
    \param[in]  sino            sinogram (TOF data will be shuffled after
                                reconstruction)
    \param[in]  delete_sino     delete sinogram after use ?
    \param[in]  iterations      number of iterations
    \param[in]  num             frame number
    \param[out] _fov_diameter   diameter of FOV after reconstruction
    \param[in]  debugname       name of image file for each iteration; '#1' is
                                replaced by number of frame and iterations
    \param[in]  op_osem         OP-OSEM reconstruction ?
    \param[in]  max_threads     maximum number of threads to use
    \return reconstructed image
    \exception REC_OFFSET_MATRIX Offset-matrix for OP-OSEM not initialized.

    Reconstruct an image from a sinogram by Ordered-Subset Expectation
    Maximum-Likelihood Expectation-Maximization (OSEM). The reconstruction
    consists of the following steps:
    \image html "osemdiag.png"
    following the MLEM equation:
    \f[
        x^{(n+1)}=\frac{x^{(n)}}{bkprj(1)}
                  bkprj\left(\frac{b}{fwdprj(x^{(n)})}\right)
    \f]
    where \f$b\f$ is the input sinogram and \f$x^{(n)}\f$ is the image in
    iteration \f$n\f$. The constant denominater \f$bkprj(1)\f$ for UW-OSEM
    (or \f$bkprj(1/A)\f$ for AW-OSEM; \f$bkprj(1/N)\f$ for NW-OSEM,
     \f$bkprj(1/(A*N))\f$ for ANW-OSEM) is precalculated.

    When calculating the ratio between the input sinogram and the estimated
    sinogram, the following thresholds are applied:
    \f[
       \mbox{ratio}=\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{em}{est} & (em > 0) \wedge (est > 0.00005)\\
         0 & (em \le 0) \wedge (est > 0.00005)\\
         1 & est \le 0.00005
        \end{array}
        \right.
    \f]
    When the image for the next iteration is calculated the following threshold
    is applied:
    \f[
        x^{(n+1)}=\left\{
        \begin{array}{r@{\quad:\quad}l}
         x^{(n+1)} & x^{(n+1)} < image\_max\\
         image\_max & x^{(n+1)} \ge image\_max
        \end{array}
        \right.
    \f]

    In case of an OP-OSEM reconstruction for data that requires an
    arc-correction, the equation
    \f[
        x^{(n+1)}=\frac{x^{(n)}}{bkprj\left(\frac{1}{A}\right)}
                  bkprj\left(\frac{arc(PN)}{fwdprj(x^{(n)})+
                  (arc(\bar{R}N)+S)A}\right)
    \f]
    is used, where \f$\bar{R}\f$ is the smoothed randoms, \f$N\f$ is the norm,
    \f$S\f$ the scatter, \f$A\f$ the acf and \f$P\f$ the prompts sinogram.
    The function \f$arc()\f$ is the arc-correction. For data from panel
    detectors the equation
    \f[
        x^{(n+1)}=\frac{x^{(n)}}{bkprj\left(\frac{1}{AN}\right)}
                  bkprj\left(\frac{P}{fwdprj(x^{(n)})+
                  (\bar{R}N+S)A}\right)
    \f]
    is used.

    For OP-OSEM the first threshold changes to:
    \f[
       \mbox{ratio}=\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{em}{est+(\bar{R}N+S)A)} &
          (em > 0) \wedge (est+(\bar{R}N+S)A > 0.05)\\
         0 & (em \le 0) \wedge (est+(\bar{R}N+S)A > 0.05)\\
         1 & est+(\bar{R}N+S)A \le 0.05
        \end{array}
        \right.
    \f]

    After reconstruction a final mask is applied to the resulting image and its
    scaled by
    \f[
        sf=\Delta\rho\left(\frac{\mbox{RhoSamples}}{\mbox{XYSamples}}\right)^2
    \f]
    with the bin size \f$\Delta\rho\f$,
    to get the correct count rate. In case of a TOF image an additional
    scaling factor is applied, depending on the number and FWHM of the TOF
    gaussians.

    The method can handle emission and transmission data.
    If the input sinogram contains TOF information which is stored as one
    sinogram per TOF bin, the sinogram is reordered from (r,t,z,seg,tof)
    to (tof,r,t,z,seg) to increase data-locality during reconstruction.
 */
/*---------------------------------------------------------------------------*/
ImageConversion *OSEM3D::reconstruct(SinogramConversion * const sino,
                                     const bool delete_sino,
                                     const unsigned short int iterations,
                                     const unsigned short int num,
                                     float * const _fov_diameter,
                                     const std::string debugname,
                                     const bool op_osem,
                                     const unsigned short int max_threads)
 { ImageConversion *result_image=NULL;
   StopWatch sw;

   try
   { std::string str;
     bool is_acf;
     unsigned short int correction_mem_idx, estimate_mem_idx, tof_bins,
                        image_index;
     float *correction, *estimate, *image, tf=0.0f, tb=0.0f;

     if (op_osem && (offset_matrix.size() != axes))
      throw Exception(REC_OFFSET_MATRIX, "Offset-matrix for OP-OSEM not "
                      "initialized.");
     is_acf=sino->isACF();
     tof_bins=1;
     sino->tra2Emi(loglevel+1);      // convert transmission into emission data
                                                       // initialize projectors
     initBckProjector(max_threads);
     initFwdProjector(max_threads);
     if (tof_bins == 1)
      flog->logMsg("start reconstruction from #1x#2x#3 to #4x#5x#6",
                   loglevel+1)->arg(RhoSamples)->arg(ThetaSamples)->
       arg(axes_slices)->arg(XYSamples)->arg(XYSamples)->arg(axis_slices[0]);
      else flog->logMsg("start reconstruction from #1x#2x#3x#4 to #5x#6x#7",
                        loglevel+1)->arg(RhoSamples)->arg(tof_bins)->
            arg(ThetaSamples)->arg(axes_slices)->arg(XYSamples)->
            arg(XYSamples)->arg(axis_slices[0]);
     str="subset order for OSEM reconstruction: 0 ";
     for (unsigned short int i=1; i < subsets; i++)
      str+=toString(order[i])+" ";
     flog->logMsg(str, loglevel+2);
     sw.start();
     image=initImage(&image_index, loglevel+2);             // initialize image
     estimate=MemCtrl::mc()->createFloat(subset_size_padded_TOF,
                                         &estimate_mem_idx, "esti",
                                         loglevel+2);
     correction=MemCtrl::mc()->createFloat(image_volume_size,
                                           &correction_mem_idx, "corr",
                                           loglevel+2);

     for (unsigned short int iter=0; iter < iterations; iter++)
      { Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2,
                                 "OSEM reconstruction (iteration #1, "
                                 "frame #2)")->arg(iter)->arg(num);
        for (unsigned short int subset=0; subset < subsets; subset++)
         { SyngoMsg(syngo_msg+": iteration "+toString(iter+1)+" subset "+
                    toString(order[subset]));
           flog->logMsg("start iteration #1, subset #2", loglevel+2)->
            arg(iter+1)->arg(order[subset]);
           sw.start();
           memset(correction, 0, image_volume_size*sizeof(float));
           for (unsigned short int segment=0; segment < segments; segment++)
            { unsigned short int t, z, axis;
              unsigned long int offs;
              float *spp, *ep, *spp2, *ep2, *sp, *op, *opp=NULL, *opp2;

              axis=(segment+1)/2;
                               // forward-project subset of segment of sinogram
              sw.start();
              fwd->forward_proj3d(image, estimate, order[subset], segment,
                                  max_threads);
              tf+=sw.stop();
                                                 // divide emission by estimate
              sp=MemCtrl::mc()->getFloatRO(sino->index(axis, 0), loglevel+3);
              if ((segment == 0) || (segment & 0x1)) offs=0;
               else offs=(unsigned long int)axis_slices[axis]/2*
                         sino_plane_size_TOF;
              if (op_osem)
               { op=MemCtrl::mc()->getFloatRO(offset_matrix[axis], loglevel+3);
                 opp=&op[offs+(unsigned long int)order[subset]*
                              (unsigned long int)RhoSamples_TOF];
               }

              for (spp=&sp[offs+(unsigned long int)order[subset]*
                                (unsigned long int)RhoSamples_TOF],
                   ep=&estimate[(unsigned long int)z_bottom[segment]*
                                (unsigned long int)RhoSamples_padded_TOF+
                                tof_bins],
                   t=0; t < ThetaSamples/subsets; t++,
                   ep+=view_size_padded_TOF,
                   spp+=(unsigned long int)subsets*
                        (unsigned long int)RhoSamples_TOF)
               { for (opp2=opp,
                      spp2=spp,
                      ep2=ep+RhoSamples_padded_TOF,
                      z=z_bottom[segment]; z <= z_top[segment]; z++,
                      ep2+=RhoSamples_padded_TOF,
                      spp2+=sino_plane_size_TOF)
                  if (op_osem)
                   { for (unsigned short int r=0; r < RhoSamples_TOF; r++)
                      { float den;

                        den=ep2[r]+opp2[r];
                        if (den > 0.05f) ep2[r]=spp2[r]/den;
                         else ep2[r]=1.0f;
                      }
                     opp2+=sino_plane_size_TOF;
                   }
                   else for (unsigned short int r=0; r < RhoSamples_TOF; r++)
                         if (ep2[r] > 0.00005f)
                          ep2[r]=std::max(0.0f, spp2[r])/ep2[r];
                          else ep2[r]=1.0f;
                 if (op_osem)
                  opp+=(unsigned long int)subsets*
                       (unsigned long int)RhoSamples_TOF;
               }
              if (op_osem) MemCtrl::mc()->put(offset_matrix[axis]);
              MemCtrl::mc()->put(sino->index(axis, 0));
              if (delete_sino)
               if ((iter == iterations-1) && (subset == subsets-1) &&
                   (segment == segments-1))
                sino->deleteData(axis, 0);
                                                        // backproject quotient
              sw.start();
              bp->back_proj3d(estimate, correction, order[subset], segment,
                              false, max_threads);
              tb+=sw.stop();
            }
                                          // calculate image for next iteration
           { float *ip, *cp, *normfac;

             ip=image+image_plane_size_padded+1;
             cp=correction;
                                        // load normalization matrix for subset
             normfac=MemCtrl::mc()->getFloatRO(norm_factors[order[subset]],
                                               loglevel+2);
             for (unsigned short int z=0; z < axis_slices[0]; z++)
              { ip+=XYSamples_padded;
                for (unsigned short int y=0; y < XYSamples; y++,
                     ip+=XYSamples_padded,
                     cp+=XYSamples,
                     normfac+=XYSamples)
                 for (unsigned short int x=mask[y].start;
                      x <= mask[y].end;
                      x++)
                  if (normfac[x] > OSEM3D::epsilon)
                   ip[x]=std::min(ip[x]*cp[x]/normfac[x], OSEM3D::image_max);
                ip+=XYSamples_padded;
              }
             MemCtrl::mc()->put(norm_factors[order[subset]]);
           }
           flog->logMsg("finished iteration #1, subset #2 in #3 sec",
                        loglevel+2)->arg(iter+1)->arg(order[subset])->
            arg(sw.stop());
         }
        if (debugname != std::string())
         { std::string fname;

           result_image=new ImageConversion(XYSamples, DeltaXY, axis_slices[0],
                                          sino->planeSeparation(), num,
                                          sino->LLD(), sino->ULD(),
                                          sino->startTime(),
                                          sino->frameDuration(),
                                          sino->gateDuration(),
                                          sino->halflife(), sino->bedPos());
           result_image->initImage(is_acf, false, "img", loglevel+2);
           { float *ip, *rp;
                  // apply final mask to reconstructed image and remove padding
             ip=image+image_plane_size_padded+1;
             rp=MemCtrl::mc()->getFloat(result_image->index(), loglevel+2);
             for (unsigned short int z=0; z < axis_slices[0]; z++)
              { ip+=XYSamples_padded;
                for (unsigned short int y=0; y < XYSamples; y++,
                     rp+=XYSamples,
                     ip+=XYSamples_padded)
                 memcpy(rp+mask2[y].start, ip+mask2[y].start,
                        (mask2[y].end-mask2[y].start+1)*sizeof(float));
                ip+=XYSamples_padded;
              }
             MemCtrl::mc()->put(result_image->index());
           }
           result_image->scale(BinSizeRho, loglevel+2);
           fname=debugname;
           fname.replace(fname.find("#1"), 2,
                         "f"+toStringZero(num, 2)+
                         "_i"+toStringZero(iter+1, 2));
           result_image->saveFlat(fname, loglevel+2);
           delete result_image;
           result_image=NULL;
         }
      }
     MemCtrl::mc()->put(correction_mem_idx);
     MemCtrl::mc()->deleteBlock(&correction_mem_idx);
     MemCtrl::mc()->put(estimate_mem_idx);
     MemCtrl::mc()->deleteBlock(&estimate_mem_idx);
                                                      // create resulting image
     flog->logMsg("mask #1 planes of image", loglevel+2)->arg(axis_slices[0]);
     result_image=new ImageConversion(XYSamples, DeltaXY, axis_slices[0],
                                      sino->planeSeparation(), num,
                                      sino->LLD(), sino->ULD(),
                                      sino->startTime(), sino->frameDuration(),
                                      sino->gateDuration(), sino->halflife(),
                                      sino->bedPos());
     result_image->initImage(is_acf, false, "img", loglevel+2);
     { float *ip, *rp;
                  // apply final mask to reconstructed image and remove padding
       ip=image+image_plane_size_padded+1;
       rp=MemCtrl::mc()->getFloat(result_image->index(), loglevel+2);
       for (unsigned short int z=0; z < axis_slices[0]; z++)
        { ip+=XYSamples_padded;
          for (unsigned short int y=0; y < XYSamples; y++,
               rp+=XYSamples,
               ip+=XYSamples_padded)
           memcpy(rp+mask2[y].start, ip+mask2[y].start,
                  (mask2[y].end-mask2[y].start+1)*sizeof(float));
          ip+=XYSamples_padded;
        }
       MemCtrl::mc()->put(result_image->index());
     }
     MemCtrl::mc()->put(image_index);
     MemCtrl::mc()->deleteBlock(&image_index);
                          // scale image to get count rates as in ECAT software
     flog->logMsg("scale image with #1", loglevel+2)->arg(BinSizeRho);
     result_image->scale(BinSizeRho, loglevel+2);
     { float sf;

       sf=((float)RhoSamples/(float)XYSamples)*
          ((float)RhoSamples/(float)XYSamples);
       flog->logMsg("scale image with #1", loglevel+2)->arg(sf);
       result_image->scale(sf, loglevel+2);
     }
                                   // convert emission back into transmission ?
     if (is_acf && !delete_sino) sino->emi2Tra(loglevel+1);
     if (_fov_diameter != NULL) *_fov_diameter=fov_diameter;
     flog->logMsg("finished reconstruction in #1 sec", loglevel+1)->
      arg(sw.stop());
     flog->logMsg("forward projector: #1 sec", loglevel+1)->arg(tf);
     flog->logMsg("backprojector: #1 sec", loglevel+1)->arg(tb);
     return(result_image);
   }
   catch (...)
    { sw.stop();
      if (result_image != NULL) delete result_image;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief OSEM reconstruction.
    \param[in]  sinogram        sinogram (TOF data will be shuffled after
                                reconstruction)
    \param[in]  iterations      number of iterations
    \param[out] _fov_diameter   diameter of FOV after reconstruction
    \param[in]  debug_name      name of image file for each iteration; '#1' is
                                replaced by number of frame and iterations
    \param[in]  max_threads     maximum number of threads to use
    \return reconstructed image

    This method does the same as the one it overloads, except that the user
    doesn't handle sinogram and image objects.
 */
/*---------------------------------------------------------------------------*/
float *OSEM3D::reconstruct(float * const sinogram,
                           const unsigned short int iterations,
                           float * const _fov_diameter,
                           const std::string debug_name,
                           const unsigned short int max_threads)
 { return(reconstructGibbs(sinogram, iterations, 0.0f, _fov_diameter,
                           debug_name, max_threads));
 }

/*---------------------------------------------------------------------------*/
/*! \brief OSEM reconstruction with Gibbs prior.
    \param[in]  sino            sinogram (TOF data will be shuffled after
                                reconstruction)
    \param[in]  delete_sino     delete sinogram after use ?
    \param[in]  iterations      number of iterations
    \param[in]  beta            Gibbs prior
    \param[in]  num             frame number
    \param[out] _fov_diameter   diameter of FOV after reconstruction
    \param[in]  debugname       name of image file for each iteration; '#1' is
                                replaced by number of frame and iterations
    \param[in]  max_threads     maximum number of threads to use
    \return reconstructed image

    Reconstruct an image from a sinogram by Ordered-Subset Expectation
    Maximum-Likelihood Expectation-Maximization (OSEM). The reconstruction
    consists of the following steps:
    \image html "osemdiag.png"
    following the MLEM equation:
    \f[
        x^{(n+1)}=\frac{x^{(n)}}{bkprj(1)}
                  bkprj\left(\frac{b}{fwdprj(x^{(n)})}\right)
    \f]
    where \f$b\f$ is the input sinogram and \f$x^{(n)}\f$ is the image in
    iteration \f$n\f$. The constant denominater \f$bkprj(1)\f$ for UW-OSEM
    (or \f$bkprj(1/A)\f$ for AW-OSEM; \f$bkprj(1/N)\f$ for NW-OSEM,
     \f$bkprj(1/(A*N))\f$ for ANW-OSEM) is precalculated. For Gibbs OSEM the
    denominator is replaced by:
    \f[
        \left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{3}{4}bkprj(1) & bkprj(1)+prior \le \frac{3}{4}bkprj(1)\\
         \frac{5}{4}bkprj(1) & bkprj(1)+prior \ge \frac{5}{4}bkprj(1)\\
         bkprj(1)+prior &
          \frac{3}{4}bkprj(1) <  bkprj(1)+prior < \frac{5}{4}bkprj(1)
        \end{array}
        \right.
    \f]
    for UW-OSEM and acordingly for weighted OSEM. The prior is calculated by
    folding the image from the last iteratiom with a 3d filter:
    \f[
      \begin{array}{rcl}
       prior & = & \frac{2\beta}{subsets}\left(
       \left[
        \left(
         \begin{array}{ccc}
           0 & 0 & 0\\
           0 & -1 & 0\\
           0 & 0 & 0
         \end{array}
        \right)
        \left(
         \begin{array}{ccc}
           0 & -1 & 0\\
           -1 & 6 & -1\\
           0 & -1 & 0
         \end{array}
        \right)
       \left(
         \begin{array}{ccc}
           0 & 0 & 0\\
           0 & -1 & 0\\
           0 & 0 & 0
         \end{array}
        \right)
       \right]
       \star x^{(n)}\right)+\\
       && \sqrt{\left(\frac{1}{3}\right)}\left(
       \left[
        \left(
         \begin{array}{ccc}
           -1 & -1 & -1\\
           -1 & 0 & -1\\
           -1 & -1 & -1
         \end{array}
        \right)
        \left(
         \begin{array}{ccc}
           -1 & 0 & -1\\
           0 & 20 & 0\\
           -1 & 0 & -1
         \end{array}
        \right)
        \left(
         \begin{array}{ccc}
           -1 & -1 & -1\\
           -1 & 0 & -1\\
           -1 & -1 & -1
         \end{array}
        \right)
       \right]
       \star x^{(n)}\right)
       \end{array}
    \f]

    When calculating the ratio between the input sinogram and the estimated
    sinogram, the following thresholds are applied:
    \f[
       \mbox{ratio}=\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{em}{est} & (em > 0) \wedge (est > 0.00005)\\
         0 & (em \le 0) \wedge (est > 0.00005)\\
         1 & est \le 0.00005
        \end{array}
        \right.
    \f]
    When the image for the next iteration is calculated the following threshold
    is applied:
    \f[
        x^{(n+1)}=\left\{
        \begin{array}{r@{\quad:\quad}l}
         x^{(n+1)} & x^{(n+1)} < image\_max\\
         image\_max & x^{(n+1)} \ge image\_max
        \end{array}
        \right.
    \f]

    After reconstruction a final mask is applied to the resulting image and its
    scaled by
    \f[
        sf=\Delta\rho\left(\frac{\mbox{RhoSamples}}{\mbox{XYSamples}}\right)^2
    \f]
    with the bin size \f$\Delta\rho\f$,
    to get the correct count rate. In case of a TOF image an additional
    scaling factor is applied, depending on the number and FWHM of the TOF
    gaussians.

    The method can handle emission and transmission data.
    If the input sinogram contains TOF information which is stored as one
    sinogram per TOF bin, the sinogram is reordered from (r,t,z,seg,tof)
    to (tof,r,t,z,seg) to increase data-locality during reconstruction.
 */
/*---------------------------------------------------------------------------*/
ImageConversion *OSEM3D::reconstructGibbs(SinogramConversion * const sino,
                                          const bool delete_sino,
                                          const unsigned short int iterations,
                                          float const beta,
                                          const unsigned short int num,
                                          float * const _fov_diameter,
                                          const std::string debugname,
                                          const unsigned short int max_threads)
 { if (beta == 0.0f)                                 // don't use Gibbs prior ?
    return(reconstruct(sino, delete_sino, iterations, num,_fov_diameter,
                       std::string(), false, max_threads));
   ImageConversion *result_image=NULL;
   StopWatch sw;

   try
   { unsigned long int gibbs_size;
     unsigned short int image_index, image_buffer_index, gibbs_index,
                        estimate_index, correction_index, tof_bins;
     float *image, *image_buffer, *gibbs, *estimate, *correction, tf=0.0f,
           tb=0.0f;
     std::string str;
     bool is_acf;

     is_acf=sino->isACF();
     tof_bins=1;
     sino->tra2Emi(loglevel+1);      // convert transmission into emission data
                                                       // initialize projectors
     initBckProjector(max_threads);
     initFwdProjector(max_threads);
     if (tof_bins == 1)
      flog->logMsg("start reconstruction from #1x#2x#3 to #4x#5x#6",
                   loglevel+1)->arg(RhoSamples)->arg(ThetaSamples)->
       arg(axes_slices)->arg(XYSamples)->arg(XYSamples)->arg(axis_slices[0]);
      else flog->logMsg("start reconstruction from #1x#2x#3x#4 to #5x#6x#7",
                        loglevel+1)->arg(RhoSamples)->arg(tof_bins)->
            arg(ThetaSamples)->arg(axes_slices)->arg(XYSamples)->
            arg(XYSamples)->arg(axis_slices[0]);
#if defined(__LINUX__) || defined(__SOLARIS__)
     flog->logMsg("Gibbs prior: #1=#2", loglevel+2)->arg((char)223)->arg(beta);
#endif
#ifdef __MACOSX__
     flog->logMsg("Gibbs prior: #1=#2", loglevel+2)->arg((char)167)->arg(beta);
#endif
#ifdef WIN32
     flog->logMsg("Gibbs prior: beta=#1", loglevel+2)->arg(beta);
#endif
     str="subset order for OSEM reconstruction: 0 ";
     for (unsigned short int i=1; i < subsets; i++)
      str+=toString(order[i])+" ";
     flog->logMsg(str, loglevel+2);
     sw.start();
     result_image=new ImageConversion(XYSamples, DeltaXY, axis_slices[0],
                                      sino->planeSeparation(), num,
                                      sino->LLD(), sino->ULD(),
                                      sino->startTime(), sino->frameDuration(),
                                      sino->gateDuration(), sino->halflife(),
                                      sino->bedPos());
     image=initImage(&image_index, loglevel+2);             // initialize image
     image_buffer=MemCtrl::mc()->createFloat(image_volume_size_padded,
                                             &image_buffer_index, "ibf",
                                             loglevel+2);
     estimate=MemCtrl::mc()->createFloat(subset_size_padded_TOF,
                                         &estimate_index, "est", loglevel+2);
     correction=MemCtrl::mc()->createFloat(image_volume_size,
                                           &correction_index, "cor",
                                           loglevel+2);
     gibbs_size=(unsigned long int)XYSamples_padded*
                (unsigned long int)XYSamples;
     gibbs=MemCtrl::mc()->createFloat(gibbs_size, &gibbs_index, "gib",
                                      loglevel+2);
     for (unsigned short int iter=0; iter < iterations; iter++)
      { for (unsigned short int subset=0; subset < subsets; subset++)
         { SyngoMsg(syngo_msg+": iteration "+toString(iter+1)+" subset "+
                    toString(order[subset]));
           flog->logMsg("start iteration #1, subset #2", loglevel+2)->
            arg(iter+1)->arg(order[subset]);
           sw.start();
           memset(correction, 0, image_volume_size*sizeof(float));
           for (unsigned short int segment=0; segment < segments; segment++)
            { unsigned short int t, z, axis;
              unsigned long int offs;
              float *tp, *ep, *tp2, *ep2, *sp;

              axis=(segment+1)/2;
                               // forward-project subset of segment of sinogram
              sw.start();
              fwd->forward_proj3d(image, estimate, order[subset], segment,
                                  max_threads);
              tf+=sw.stop();
                                                 // divide emission by estimate
              sp=MemCtrl::mc()->getFloatRO(sino->index(axis, 0), loglevel+3);
              if ((segment == 0) || (segment & 0x1)) offs=0;
               else offs=(unsigned long int)axis_slices[axis]/2*
                         sino_plane_size_TOF;
              for (tp=&sp[offs+(unsigned long int)order[subset]*
                               (unsigned long int)RhoSamples_TOF],
                   ep=&estimate[(unsigned long int)z_bottom[segment]*
                                (unsigned long int)RhoSamples_padded_TOF+
                                tof_bins],
                   t=0; t < ThetaSamples/subsets; t++,
                   ep+=view_size_padded_TOF,
                   tp+=(unsigned long int)subsets*
                       (unsigned long int)RhoSamples_TOF)
               for (tp2=tp,
                    ep2=ep+RhoSamples_padded_TOF,
                    z=z_bottom[segment]; z <= z_top[segment]; z++,
                    ep2+=RhoSamples_padded_TOF,
                    tp2+=sino_plane_size_TOF)
                for (unsigned short int r=0; r < RhoSamples_TOF; r++)
                 if (ep2[r] > 0.00005f) ep2[r]=std::max(0.0f, tp2[r])/ep2[r];
                  else ep2[r]=1.0f;
              MemCtrl::mc()->put(sino->index(axis, 0));
              if (delete_sino)
               if ((iter == iterations-1) && (subset == subsets-1) &&
                   (segment == segments-1))
                sino->deleteData(axis, 0);
                                                        // backproject quotient
              sw.start();
              bp->back_proj3d(estimate, correction, order[subset], segment,
                              false, max_threads);
              tb+=sw.stop();
            }
                                          // calculate image for next iteration
           { float *ip, *cp, *ipb, *normfac;

             memcpy(image_buffer, image,
                    image_volume_size_padded*sizeof(float));
             ip=image+image_plane_size_padded;
             ipb=image_buffer+image_plane_size_padded+1;
             cp=correction;
                                        // load normalization matrix for subset
             normfac=MemCtrl::mc()->getFloatRO(norm_factors[order[subset]],
                                               loglevel+2);
             for (unsigned short int z=0; z < axis_slices[0]; z++)
              { float *gp;

                ipb+=XYSamples_padded;
                memset(gibbs, 0, gibbs_size*sizeof(float));
                gp=gibbs+1;
                for (unsigned short int y=0; y < XYSamples; y++,
                     gp+=XYSamples_padded,
                     ipb+=XYSamples_padded,
                     cp+=XYSamples,
                     normfac+=XYSamples)
                 for (unsigned short int x=mask[y].start;
                      x <= mask[y].end;
                      x++)
                  if (normfac[x] > OSEM3D::epsilon)
                   { float prior, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9;

                     p2=&ipb[x];
                     if (y > 0) p1=p2-XYSamples_padded;
                      else p1=p2;
                     if (y < XYSamples-1) p3=p2+XYSamples_padded;
                      else p3=p2;
                     /*
                     // the 2D Gibbs prior is defined as:
                     // ( 0 -1  0)           (-1  0 -1)
                     // (-1  4 -1)+sqrt(1/2)*( 0  4  0)
                     // ( 0 -1  0)           (-1  0 -1)
                     prior=2.0f*beta/(float)subsets*
                          (4.0f*p2[0]-p2[-1]-p2[1]-p1[0]-p3[0]+
                           sqrtf(1.0/2.0f)*
                           (4.0f*p2[0]-p1[-1]-p1[1]-p3[-1]-p3[1]));
                     */
                     if (z > 0)
                      { p4=p1-image_plane_size_padded;
                        p5=p2-image_plane_size_padded;
                        p6=p3-image_plane_size_padded;
                      }
                      else { p4=p1;
                             p5=p2;
                             p6=p3;
                           }
                     if (z < axis_slices[0]-1)
                      { p7=p1+image_plane_size_padded;
                        p8=p2+image_plane_size_padded;
                        p9=p3+image_plane_size_padded;
                      }
                      else { p7=p1;
                             p8=p2;
                             p9=p3;
                           }
                     // the 3D Gibbs prior is defined as:
                     // ( 0  0  0) ( 0 -1  0) ( 0  0  0)
                     // ( 0 -1  0) (-1  6 -1) ( 0 -1  0)
                     // ( 0  0  0) ( 0 -1  0) ( 0  0  0)
                     // +sqrt(1/3)*
                     // (-1 -1 -1) (-1  0 -1) (-1 -1 -1)
                     // (-1  0 -1) ( 0 20  0) (-1  0 -1)
                     // (-1 -1 -1) (-1  0 -1) (-1 -1 -1)
                     prior=2.0f*beta/(float)subsets*
                           (6.0f*p2[0]-p2[-1]-p2[1]-p1[0]-p3[0]-p5[0]-p8[0]+
                            sqrtf(1.0f/3.0f)*
                            (20.0f*p2[0]-p1[-1]-p1[1]-p3[-1]-p3[1]-p4[-1]-
                                         p4[0]-p4[1]-p5[-1]-p5[1]-p6[-1]-p6[0]-
                                         p6[1]-p7[-1]-p7[0]-p7[1]-p8[-1]-p8[1]-
                                         p9[-1]-p9[0]-p9[1]));
                     gp[x]=std::min(ipb[x]*cp[x]/
                                    std::max(0.75f*normfac[x],
                                             std::min(1.25f*normfac[x],
                                                      normfac[x]+prior)),
                                    OSEM3D::image_max);
                   }
                memcpy(ip+XYSamples_padded, gibbs, gibbs_size*sizeof(float));
                ip+=image_plane_size_padded;
                ipb+=XYSamples_padded;
              }
             MemCtrl::mc()->put(norm_factors[order[subset]]);
           }
           flog->logMsg("finished iteration #1, subset #2 in #3 sec",
                        loglevel+2)->arg(iter+1)->arg(order[subset])->
            arg(sw.stop());
         }
        if (debugname != std::string())
         { std::string fname;

           result_image=new ImageConversion(XYSamples, DeltaXY, axis_slices[0],
                                          sino->planeSeparation(), num,
                                          sino->LLD(), sino->ULD(),
                                          sino->startTime(),
                                          sino->frameDuration(),
                                          sino->gateDuration(),
                                          sino->halflife(), sino->bedPos());
           result_image->initImage(is_acf, false, "img", loglevel+2);
           { float *ip, *rp;
                  // apply final mask to reconstructed image and remove padding
             ip=image+image_plane_size_padded+1;
             rp=MemCtrl::mc()->getFloat(result_image->index(), loglevel+2);
             for (unsigned short int z=0; z < axis_slices[0]; z++)
              { ip+=XYSamples_padded;
                for (unsigned short int y=0; y < XYSamples; y++,
                     rp+=XYSamples,
                     ip+=XYSamples_padded)
                 memcpy(rp+mask2[y].start, ip+mask2[y].start,
                        (mask2[y].end-mask2[y].start+1)*sizeof(float));
                ip+=XYSamples_padded;
              }
             MemCtrl::mc()->put(result_image->index());
           }
           result_image->scale(BinSizeRho, loglevel+2);
           fname=debugname;
           fname.replace(fname.find("#1"), 2,
                         "f"+toStringZero(num, 2)+
                         "_i"+toStringZero(iter+1, 2));
           result_image->saveFlat(fname, loglevel+2);
           delete result_image;
           result_image=NULL;
         }
      }
     MemCtrl::mc()->put(image_buffer_index);
     MemCtrl::mc()->deleteBlock(&image_buffer_index);
     MemCtrl::mc()->put(correction_index);
     MemCtrl::mc()->deleteBlock(&correction_index);
     MemCtrl::mc()->put(estimate_index);
     MemCtrl::mc()->deleteBlock(&estimate_index);
     MemCtrl::mc()->put(gibbs_index);
     MemCtrl::mc()->deleteBlock(&gibbs_index);
                                                      // create resulting image
     flog->logMsg("mask #1 planes of image", loglevel+2)->arg(axis_slices[0]);
     result_image->initImage(is_acf, false, "img", loglevel+2);
     { float *ip, *rp;
                                     // apply final mask to reconstructed image
       ip=image+image_plane_size_padded+1;
       rp=MemCtrl::mc()->getFloat(result_image->index(), loglevel+2);
       for (unsigned short int z=0; z < axis_slices[0]; z++)
        { ip+=XYSamples_padded;
          for (unsigned short int y=0; y < XYSamples; y++,
               rp+=XYSamples,
               ip+=XYSamples_padded)
           memcpy(rp+mask2[y].start, ip+mask2[y].start,
                  (mask2[y].end-mask2[y].start+1)*sizeof(float));
          ip+=XYSamples_padded;
        }
       MemCtrl::mc()->put(result_image->index());
     }
     MemCtrl::mc()->put(image_index);
     MemCtrl::mc()->deleteBlock(&image_index);
                         // scale image to get count rates as in ECAT software
     flog->logMsg("scale image with #1", loglevel+2)->arg(BinSizeRho);
     result_image->scale(BinSizeRho, loglevel+2);
     { float sf;

       sf=((float)RhoSamples/(float)XYSamples)*
          ((float)RhoSamples/(float)XYSamples);
       flog->logMsg("scale image with #1", loglevel+2)->arg(sf);
       result_image->scale(sf, loglevel+2);
     }
                                  // convert emission black into transmission ?
     if (is_acf && !delete_sino) sino->emi2Tra(loglevel+1);
     if (_fov_diameter != NULL) *_fov_diameter=fov_diameter;
     flog->logMsg("finished reconstruction in #1 sec", loglevel+1)->
      arg(sw.stop());
     flog->logMsg("forward projector: #1 sec", loglevel+1)->arg(tf);
     flog->logMsg("backprojector: #1 sec", loglevel+1)->arg(tb);
     return(result_image);
   }
   catch (...)
    { sw.stop();
      if (result_image != NULL) delete result_image;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief OSEM reconstruction with Gibbs prior.
    \param[in]  sinogram        sinogram (TOF data will be shuffled after
                                reconstruction)
    \param[in]  iterations      number of iterations
    \param[in]  beta            Gibbs prior
    \param[out] _fov_diameter   diameter of FOV after reconstruction
    \param[in]  debug_name      name of image file for each iteration; '#1' is
                                replaced by number of frame and iterations
    \param[in]  max_threads     maximum number of threads to use
    \return reconstructed image

    A sinogram object is created and filled with the sinogram data. The
    resulting image object is deleted and a pointer to the flat image data is
    returned. This method does the same as the one it overloads, except that
    the user doesn't handle sinogram and image objects.
 */
/*---------------------------------------------------------------------------*/
float *OSEM3D::reconstructGibbs(float * const sinogram,
                                const unsigned short int iterations,
                                float const beta, float * const _fov_diameter,
                                const std::string debug_name,
                                const unsigned short int max_threads)
 { SinogramConversion *sino=NULL;
   ImageConversion *image=NULL;
   float *img=NULL;

   try
   { sino=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamples, span, 0,
                                 plane_separation, 0, 0.0f, false, 0, 0, 0.0f,
                                 0.0f, 0.0f, 0.0f, 0.0f, 0, axes == 1);
     sino->copyData(sinogram, 0, axes, axis_slices[0], 1, false, false, "emis",
                    loglevel);
     if (beta == 0)
      image=reconstruct(sino, true, iterations, 0, _fov_diameter,
                        std::string(), false, max_threads);
      else image=reconstructGibbs(sino, true, iterations, beta, 0,
                                  _fov_diameter, debug_name, max_threads);
     delete sino;
     sino=NULL;
     img=image->getRemove(loglevel);
     delete image;
     image=NULL;
     return(img);
   }
   catch (...)
    { if (sino != NULL) delete sino;
      if (image != NULL) delete image;
      if (img != NULL) delete[] img;
      throw;
    }
 }

#ifdef UNUSED
/*---------------------------------------------------------------------------*/
/* reconstructMP_master: 3D-OSEM reconstruction (with multi-processing)      */
/*  sino            sinogram data                                            */
/*  iterations      number of iterations                                     */
/*  _fov_diameter   diameter of FOV after reconstruction                     */
/*  max_threads     maximum number of threads to use                         */
/* return: reconstructed image                                               */
/*---------------------------------------------------------------------------*/
float *OSEM3D::reconstructMP_master(float * const sino,
                                    const unsigned short int iterations,
                                    float * const _fov_diameter,
                                    const unsigned short int max_threads)
 { float *estimate=NULL, *correction=NULL, *result=NULL, *estimate2=NULL;
   StopWatch sw;

   try
   { std::string str;
     unsigned short int image_index;
     float *image;

     initBckProjector(max_threads);
     initFwdProjector(max_threads);
     flog->logMsg("start reconstruction from #1x#2x#3 to #4x#5x#6",
                  loglevel+1)->arg(RhoSamples)->arg(ThetaSamples)->
      arg(axes_slices)->arg(XYSamples)->arg(XYSamples)->arg(axis_slices[0]);
     str="subset order for OSEM reconstruction: 0 ";
     for (unsigned short int i=1; i < subsets; i++)
      str+=toString(order[i])+" ";
     flog->logMsg(str, loglevel+2);
     sw.start();
     image=initImage(&image_index, loglevel+2);             // initialize image
     estimate=new float[subset_size_padded];
     estimate2=new float[subset_size_padded];
     correction=new float[image_volume_size];
     for (unsigned short int iter=0; iter < iterations; iter++)
      { for (unsigned short int subset=0; subset < subsets; subset++)
         { flog->logMsg("start iteration #1, subset #2", loglevel+2)->
            arg(iter+1)->arg(subset);
           sw.start();
           memset(correction, 0, image_volume_size*sizeof(float));
           for (unsigned short int segment=0; segment < segments; segment++)
            { unsigned short int t, z, proc, num_procs;
              float *tp, *ep, *tp2, *ep2;

            num_procs=2;
            proc=0;
                               // forward-project subset of segment of sinogram
              fwd->forward_proj3d(image, estimate, order[subset], segment,
                                  max_threads, proc, num_procs);

              proc=1;
                               // forward-project subset of segment of sinogram
              fwd->forward_proj3d(image, estimate2, order[subset], segment,
                                  max_threads, proc, num_procs);
              // collect estimate (only z_bottom bis z_top)
              vecAdd(estimate, estimate2, estimate, subset_size_padded);

                                                 // divide emission by estimate
              for (tp=&sino[(unsigned long int)seg_plane_offset[segment]*
                            sino_plane_size+
                            (unsigned long int)order[subset]*
                            (unsigned long int)RhoSamples],
                   ep=&estimate[(unsigned long int)z_bottom[segment]*
                                (unsigned long int)RhoSamples_padded+1],
                   t=0; t < ThetaSamples/subsets; t++,
                   ep+=view_size_padded,
                   tp+=(unsigned long int)subsets*
                       (unsigned long int)RhoSamples)
               for (tp2=tp,
                    ep2=ep+RhoSamples_padded,
                    z=z_bottom[segment]; z <= z_top[segment]; z++,
                    ep2+=RhoSamples_padded,
                    tp2+=sino_plane_size)
                for (unsigned short int r=0; r < RhoSamples; r++)
                 if (ep2[r] > 0.00005f) ep2[r]=std::max(0.0f, tp2[r])/ep2[r];
                  else ep2[r]=1.0f;

              proc=0;
                                                        // backproject quotient
              bp->back_proj3d(estimate, correction, order[subset], segment,
                              max_threads, proc, num_procs);
              proc=1;
                                                        // backproject quotient
              bp->back_proj3d(estimate, correction, order[subset], segment,
                              max_threads, proc, num_procs);
            }
           // correction einsammeln und aufaddieren
                                          // calculate image for next iteration
           { float *ip, *cp, *normfac;
                                        // load normalization matrix for subset
             normfac=MemCtrl::mc()->getFloatRO(norm_factors[order[subset]],
                                               loglevel+2);
             ip=image+image_plane_size_padded+1;
             cp=correction;
             for (unsigned short int z=0; z < axis_slices[0]; z++)
              { ip+=XYSamples_padded;
                for (unsigned short int y=0; y < XYSamples; y++,
                     ip+=XYSamples_padded,
                     cp+=XYSamples,
                     normfac+=XYSamples)
                 for (unsigned short int x=mask[y].start;
                      x <= mask[y].end;
                      x++)
                  if (normfac[x] > OSEM3D::epsilon)
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
                   ip[x]=std::min(ip[x]*cp[x]/normfac[x], OSEM3D::image_max);
#endif
#ifdef WIN32
                   ip[x]=mini(ip[x]*cp[x]/normfac[x], OSEM3D::image_max);
#endif
                ip+=XYSamples_padded;
              }
             MemCtrl::mc()->put(norm_factors[order[subset]]);
           }
           flog->logMsg("finished iteration #1, subset #2 in #3 sec",
                        loglevel+2)->arg(iter+1)->arg(subset)->arg(sw.stop());
         }
      }
     delete[] correction;
     correction=NULL;
     delete[] estimate2;
     estimate2=NULL;
     delete[] estimate;
     estimate=NULL;
     flog->logMsg("mask #1 planes of image", loglevel+2)->arg(axis_slices[0]);
     result=new float[image_volume_size];
     memset(result, 0, image_volume_size*sizeof(float));
     { float *ip, *rp, factor;
                                     // apply final mask to reconstructed image
       ip=image+image_plane_size_padded+1;
       rp=result;
                          // scale image to get count rates as in ECAT software
       factor=BinSizeRho;
       flog->logMsg("scale image with #1", loglevel+2)->arg(factor);
       for (unsigned short int z=0; z < axis_slices[0]; z++)
        { ip+=XYSamples_padded;
          for (unsigned short int y=0; y < XYSamples; y++,
               rp+=XYSamples,
               ip+=XYSamples_padded)
           vecMulScalar(&ip[mask2[y].start], factor, &rp[mask2[y].start],
                        mask2[y].end-mask2[y].start+1);
          ip+=XYSamples_padded;
        }
     }
     MemCtrl::mc()->put(image_index);
     MemCtrl::mc()->deleteBlock(&image_index);
     if (_fov_diameter != NULL) *_fov_diameter=fov_diameter;
     flog->logMsg("finished reconstruction in #1 sec", loglevel+1)->
      arg(sw.stop());
     return(result);
   }
   catch (...)
    { sw.stop();
      if (estimate != NULL) delete[] estimate;
      if (correction != NULL) delete[] correction;
      if (result != NULL) delete[] result;
      throw;
    }
 }
#if 0
 { float *correction=NULL, *result=NULL, *correct_plane=NULL, *estimate=NULL;
   Stopwatch sw;

   try
   { std::string str;
     unsigned short int i, image_index;
     float *image;
                                               // distribute sinogram to slaves
     flog->logMsg("distribute sinogram to slaves", loglevel+1);
     for (i=0; i < os_slave.size(); i++)
      { float *sp;

        os_slave[i]->newMessage();
        *(os_slave[i]) << OS_CMD_GET_EMISSION << std::endl;
        sp=sino;
        for (unsigned short int p=0; p < axes_slices; p++,
             sp+=sino_plane_size)
         { os_slave[i]->newMessage();
           os_slave[i]->writeData(sp, sino_plane_size);
           *(os_slave[i]) << std::endl;
         }
      }

     flog->logMsg("start reconstruction from #1x#2x#3 to #4x#5x#6",
                  loglevel+1)->arg(RhoSamples)->arg(ThetaSamples)->
      arg(axes_slices)->arg(XYSamples)->arg(XYSamples)->arg(axis_slices[0]);
     str="subset order for OSEM reconstruction: 0 ";
     for (i=1; i < subsets; i++)
      str+=toString(order[i])+" ";
     flog->logMsg(str, loglevel+2);
     sw.start();
     image=initImage(&image_index, loglevel+2);             // initialize image
     estimate=new float[subset_size_padded];
     correction=new float[image_volume_size];
     correct_plane=new float[image_plane_size];
     if (!store_normmat)
      { normfac=new float[image_volume_size];
        rio_normfac=new RawIO <float>(normfac_file, false, false);
      }
     for (unsigned short int iter=0; iter < iterations; iter++)
      { for (unsigned short int subset=0; subset < subsets; subset++)
         { flog->logMsg("start iteration #1, subset #2", loglevel+2)->
            arg(iter+1)->arg(subset);
           sw.start();
           memset(correction, 0, image_volume_size*sizeof(float));
#if 1
                                          // distribute current image to slaves
           flog->logMsg("distribute image to slaves", loglevel+2);
           for (i=0; i < os_slave.size(); i++)
            { float *ip;

              os_slave[i]->newMessage();
              *(os_slave[i]) << OS_CMD_RECONSTRUCT << order[subset]
                             << std::endl;
              ip=image+image_plane_size_padded;
              for (unsigned short int z=0; z < axis_slices[0]; z++,
                   ip+=image_plane_size_padded)
               { os_slave[i]->newMessage();
                 os_slave[i]->writeData(ip, image_plane_size_padded);
                 *(os_slave[i]) << std::endl;
               }
            }
                                               // calculate reconstruction step
           for (unsigned short int segment=0; segment < segments; segment++)
            { unsigned short int num;

              num=segment % os_slave.size();
              os_slave[num]->newMessage();
              *(os_slave[num]) << segment << std::endl;
            }
           for (i=0; i < os_slave.size(); i++)
            { os_slave[i]->newMessage();
              *(os_slave[i]) << (unsigned short int)9999 << std::endl;
            }
                                                  // collect correction factors
           memset(correction, 0, image_volume_size*sizeof(float));
           for (i=0; i < os_slave.size(); i++)
            { float *cp;

              cp=correction;
              for (unsigned short int z=0; z < axis_slices[0]; z++,
                   cp+=image_plane_size)
               { os_slave[i]->readData(correct_plane, image_plane_size);
                 vecAdd(cp, correct_plane, cp, image_plane_size);
               }
            }
#endif
#if 0
           for (unsigned short int segment=0; segment < segments; segment++)
            reconstructMP_slave(sino, image, correction, estimate,
                                order[subset], segment, max_threads);
#endif
           flog->logMsg("update image", loglevel+2);
                                          // calculate image for next iteration
           { float *ip, *cp, *normfac;
                                        // load normalization matrix for subset
             normfac=MemCtrl::mc()->getFloatRO(norm_factors[order[subset]],
                                               loglevel+2);
             ip=image+image_plane_size_padded+1;
             cp=correction;
             for (unsigned short int z=0; z < axis_slices[0]; z++)
              { ip+=XYSamples_padded;
                for (unsigned short int y=0; y < XYSamples; y++,
                     ip+=XYSamples_padded,
                     cp+=XYSamples,
                     normfac+=XYSamples)
                 for (unsigned short int x=mask[y].start;
                      x <= mask[y].end;
                      x++)
                  if (normfac[x] > OSEM3D::epsilon)
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
                   ip[x]=std::min(ip[x]*cp[x]/normfac[x], OSEM3D::image_max);
#endif
#ifdef WIN32
                   ip[x]=mini(ip[x]*cp[x]/normfac[x], OSEM3D::image_max);
#endif
                ip+=XYSamples_padded;
              }
             MemCtrl::mc()->put(norm_factors[order[subset]]);
           }
           flog->logMsg("finished iteration #1, subset #2 in #3 sec",
                        loglevel+2)->arg(iter+1)->arg(subset)->arg(sw.stop());
         }
      }
     delete[] correct_plane;
     correct_plane=NULL;
     delete[] correction;
     correction=NULL;
     delete[] estimate;
     estimate=NULL;
     flog->logMsg("mask #1 planes of image", loglevel+2)->arg(axis_slices[0]);
     result=new float[image_volume_size];
     memset(result, 0, image_volume_size*sizeof(float));
     { float *ip, *rp, factor;
                                     // apply final mask to reconstructed image
       ip=image+image_plane_size_padded+1;
       rp=result;
       factor=(float)RhoSamples*(float)RhoSamples*BinSizeRho/
              ((float)XYSamples*(float)XYSamples);
       for (unsigned short int z=0; z < axis_slices[0]; z++)
        { ip+=XYSamples_padded;
          for (unsigned short int y=0; y < XYSamples; y++,
               rp+=XYSamples,
               ip+=XYSamples_padded)
           vecMulScalar(&ip[mask2[y].start], factor, &rp[mask2[y].start],
                        mask2[y].end-mask2[y].start+1);
          ip+=XYSamples_padded;
        }
     }
     MemCtrl::mc()->put(image_index);
     MemCtrl::mc()->deleteBlock(&image_index);
     if (_fov_diameter != NULL) *_fov_diameter=fov_diameter;
     flog->logMsg("finished reconstruction in #1 sec", loglevel+1)->
      arg(sw.stop());
     return(result);
   }
   catch (...)
    { sw.stop();
      if (correction != NULL) delete[] correction;
      if (correct_plane != NULL) delete[] correct_plane;
      if (result != NULL) delete[] result;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* testcode, not used yet                                                    */
/*---------------------------------------------------------------------------*/
void OSEM3D::reconstructMP_slave(float * const sino, float * const image,
                                 float * const correction,
                                 float * const estimate,
                                 const unsigned short int subset,
                                 const unsigned short int segment,
                                 const unsigned short int max_threads)
 { unsigned short int t, z;
   float *tp, *ep, *tp2, *ep2;

   initBckProjector(max_threads);
   initFwdProjector(max_threads);
                               // forward-project subset of segment of sinogram
   fwd->forward_proj3d(image, estimate, subset, segment, max_threads);
                                                 // divide emission by estimate
   for (tp=&sino[(unsigned long int)seg_plane_offset[segment]*
                 sino_plane_size+
                 (unsigned long int)order[subset]*
                 (unsigned long int)RhoSamples],
        ep=&estimate[(unsigned long int)z_bottom[segment]*
                     (unsigned long int)RhoSamples_padded+1],
        t=0; t < ThetaSamples/subsets; t++,
        ep+=view_size_padded,
        tp+=(unsigned long int)subsets*
            (unsigned long int)RhoSamples)
    for (tp2=tp,
         ep2=ep+RhoSamples_padded,
         z=z_bottom[segment]; z <= z_top[segment]; z++,
         ep2+=RhoSamples_padded,
         tp2+=sino_plane_size)
     for (unsigned short int r=0; r < RhoSamples; r++)
      if (ep2[r] > 0.00005f) ep2[r]=std::max(0.0f, tp2[r])/ep2[r];
       else ep2[r]=1.0f;
                                                        // backproject quotient
   bp->back_proj3d(estimate, correction, subset, segment, false, max_threads);
 }
#endif
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Terminate OSEM slave.
    \param[in] cs   communication channel to slave
    \exception REC_TERMINATE_SLAVE_ERROR OSEM master can't terminate OSEM slave

    Terminate OSEM slave by sending the terminate command and waiting for the
    correct answer.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::terminateSlave(CommSocket *cs)
 { if (cs == NULL) return;                          // connection not available
   unsigned short int answer;
                                                             // terminate slave
   cs->newMessage();
   *cs << OS_CMD_EXIT << std::endl;
   *cs >> answer;                                // wait until slave terminates
   if (answer != OS_CMD_EXIT)                                   // can't happen
    throw Exception(REC_TERMINATE_SLAVE_ERROR,
                    "OSEM master can't terminate OSEM slave.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide emission by acf.
    \param[in,out] sino          emission sinogram
    \param[in]     acf           acf sinogram
    \param[in]     delete_acf    delete acf after use ?
    \param[in]     max_threads   maximum number of threads to use for rebinning

    The emission sinogram is divided by the acf as a preparation for AW-OSEM.
    If necessary the acf will be rebinned to the size of the emission. The
    acf object remains unchanged.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::uncorrectEmissionAW(SinogramConversion *sino,
                                 SinogramConversion *acf,
                                 const bool delete_acf,
                                 const unsigned short int max_threads) const
 { if ((algorithm != ALGO::OSEM_AW_Algo) || (acf == NULL)) return;
   sino->divide(acf, delete_acf, CORR_Measured_Attenuation_Correction |
                                 CORR_Calculated_Attenuation_Correction,
                false, loglevel, max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide emission by acf.
    \param[in,out] sino                pointer to emission data
    \param[in]     acf_data            pointer to acf data
    \param[in]     RhoSamplesAtten     number of bins in acf projections
    \param[in]     ThetaSamplesAtten   number of angles in acf plane
    \param[in]     delete_acf          delete acf after use ?
    \param[in]     max_threads         maximum number of threads to use for
                                       rebinning

    The emission sinogram is divided by the acf as a preparation for AW-OSEM.
    Sinogram objects are created and filled with the emission and acf data.
    This method does the same as the one it overloads, except that the user
    doesn't handle sinogram objects.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::uncorrectEmissionAW(float * const sino, float * const acf_data,
                                 const unsigned short int RhoSamplesAtten,
                                 const unsigned short int ThetaSamplesAtten,
                                 const unsigned short int max_threads) const
 { if ((algorithm != ALGO::OSEM_AW_Algo) || (acf_data == NULL)) return;
   SinogramConversion *emi_sino=NULL, *acf_sino=NULL;

   try
   { float *sp;
                                                  // create object for emission
     emi_sino=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamples,
                                     span, 0, plane_separation, 0, 0.0f, false,
                                     0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                     axes == 1);
     emi_sino->copyData(sino, 0, axes, axis_slices[0], 1, false, true, "emis",
                        loglevel);
                                                       // create object for acf
     acf_sino=new SinogramConversion(RhoSamplesAtten,
                                     BinSizeRho*(float)RhoSamples/
                                     (float)RhoSamplesAtten, ThetaSamplesAtten,
                                     span, 0, plane_separation, 0, 0.0f, false,
                                     0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                     axes == 1);
     acf_sino->copyData(acf_data, 0, axes, axis_slices[0], 1, false, true,
                        "acf", loglevel);
                                                  // uncorrect emission for acf
     uncorrectEmissionAW(emi_sino, acf_sino, true, max_threads);
     delete acf_sino;
     acf_sino=NULL;
                                     // copy emission back into original buffer
     sp=sino;
     for (unsigned short int axis=0; axis < axes; axis++)
      { memcpy(sp, MemCtrl::mc()->getFloatRO(emi_sino->index(axis, 0),
                                             loglevel),
               emi_sino->size(axis)*sizeof(float));
        MemCtrl::mc()->put(emi_sino->index(axis, 0));
        sp+=emi_sino->size(axis);
      }
     delete emi_sino;
     emi_sino=NULL;
   }
   catch (...)
    { if (emi_sino != NULL) delete emi_sino;
      if (acf_sino != NULL) delete acf_sino;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide emission by norm.
    \param[in,out] sino          emission sinogram
    \param[in]     norm          norm sinogram
    \param[in]     delete_norm   delete norm after use ?
    \param[in]     max_threads   maximum number of threads to use for rebinning

    The emission sinogram is divided by the norm as a preparation for NW-OSEM.
    If necessary the norm will be rebinned to the size of the emission. The
    norm object remains unchanged.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::uncorrectEmissionNW(SinogramConversion *sino,
                                 SinogramConversion *norm,
                                 const bool delete_norm,
                                 const unsigned short int max_threads) const
 { if ((algorithm != ALGO::OSEM_NW_Algo) || (norm == NULL)) return;
   sino->divide(norm, delete_norm, CORR_Normalized, true, loglevel,
                max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide emission by norm.
    \param[in,out] sino               pointer to emission data
    \param[in]     normdata           pointer to norm data
    \param[in]     RhoSamplesNorm     number of bins in norm projections
    \param[in]     ThetaSamplesNorm   number of angles in nrm plane
    \param[in]     delete_norm        delete norm after use ?
    \param[in]     max_threads        maximum number of threads to use for
                                      rebinning

    The emission sinogram is divided by the norm as a preparation for NW-OSEM.
    Sinogram objects are created and filled with the emission and norm data.
    This method does the same as the one it overloads, except that the user
    doesn't handle sinogram objects.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::uncorrectEmissionNW(float * const sino, float * const normdata,
                                 const unsigned short int RhoSamplesNorm,
                                 const unsigned short int ThetaSamplesNorm,
                                 const unsigned short int max_threads) const
 { if ((algorithm != ALGO::OSEM_NW_Algo) || (normdata == NULL)) return;
   SinogramConversion *emi_sino=NULL, *norm=NULL;

   try
   { float *sp;
                                                  // create object for emission
     emi_sino=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamples,
                                     span, 0, plane_separation, 0, 0.0f, false,
                                     0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                     axes == 1);
     emi_sino->copyData(sino, 0, axes, axis_slices[0], 1, false, true, "emis",
                        loglevel);
                                                      // create object for norm
     norm=new SinogramConversion(RhoSamplesNorm,
                                 BinSizeRho*(float)RhoSamples/
                                 (float)RhoSamplesNorm, ThetaSamplesNorm, span,
                                 0, plane_separation, 0, 0.0f, false, 0, 0,
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, axes == 1);
     norm->copyData(normdata, 0, axes, axis_slices[0], 1, false, true, "norm",
                    loglevel);
                                                 // uncorrect emission for norm
     uncorrectEmissionNW(emi_sino, norm, true, max_threads);
     delete norm;
     norm=NULL;
                                     // copy emission back into original buffer
     sp=sino;
     for (unsigned short int axis=0; axis < axes; axis++)
      { memcpy(sp, MemCtrl::mc()->getFloatRO(emi_sino->index(axis, 0),
                                             loglevel),
               emi_sino->size(axis)*sizeof(float));
        MemCtrl::mc()->put(emi_sino->index(axis, 0));
        sp+=emi_sino->size(axis);
      }
     delete emi_sino;
     emi_sino=NULL;
   }
   catch (...)
    { if (emi_sino != NULL) delete emi_sino;
      if (norm != NULL) delete norm;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide emission by norm and acf.
    \param[in,out] sino          emission sinogram
    \param[in]     acf           acf sinogram
    \param[in]     delete_acf    delete acf after use ?
    \param[in]     norm          norm sinogram
    \param[in]     delete_norm   delete norm after use ?
    \param[in]     max_threads   maximum number of threads to use for rebinning

    The emission sinogram is divided by the norm and the acf as a preparation
    for ANW-OSEM. If necessary the norm and acf will be rebinned to the size of
    the emission. The norm and acf objects remain unchanged.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::uncorrectEmissionANW(SinogramConversion *sino,
                                  SinogramConversion *acf,
                                  const bool delete_acf,
                                  SinogramConversion *norm,
                                  const bool delete_norm,
                                  const unsigned short int max_threads) const
 { if (algorithm != ALGO::OSEM_ANW_Algo) return;
   if (norm != NULL)
    sino->divide(norm, delete_norm, CORR_Normalized, true, loglevel,
                 max_threads);
   if (acf != NULL)
    sino->divide(acf, delete_acf, CORR_Measured_Attenuation_Correction |
                                  CORR_Calculated_Attenuation_Correction,
                 false, loglevel, max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide emission by norm and acf.
    \param[in,out] sino                pointer to emission data
    \param[in]     acf_data            pointer to acf data
    \param[in]     RhoSamplesAtten     number of bins in acf projections
    \param[in]     ThetaSamplesAtten   number of angles in acf plane
    \param[in]     normdata            pointer to norm data
    \param[in]     RhoSamplesNorm      number of bins in norm projections
    \param[in]     ThetaSamplesNorm    number of angles in nrm plane
    \param[in]     delete_norm         delete norm after use ?
    \param[in]     max_threads         maximum number of threads to use for
                                       rebinning

    The emission sinogram is divided by the norm and acf as a preparation for
    ANW-OSEM. Sinogram objects are created and filled with the emission, norm
    and acf data. This method does the same as the one it overloads, except
    that the user doesn't handle sinogram objects.
 */
/*---------------------------------------------------------------------------*/
void OSEM3D::uncorrectEmissionANW(float * const sino, float * const acf_data,
                                  const unsigned short int RhoSamplesAtten,
                                  const unsigned short int ThetaSamplesAtten,
                                  float * const normdata,
                                  const unsigned short int RhoSamplesNorm,
                                  const unsigned short int ThetaSamplesNorm,
                                  const unsigned short int max_threads) const
 { if (algorithm != ALGO::OSEM_ANW_Algo) return;
   SinogramConversion *emi_sino=NULL, *acf_sino=NULL, *norm=NULL;

   try
   { float *sp;
                                                  // create object for emission
     emi_sino=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamples,
                                     span, 0, plane_separation, 0, 0.0f, false,
                                     0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                     axes == 1);
     emi_sino->copyData(sino, 0, axes, axis_slices[0], 1, false, true, "emis",
                        loglevel);
     if (acf_data != NULL)                             // create object for acf
      { acf_sino=new SinogramConversion(RhoSamplesAtten,
                                        BinSizeRho*(float)RhoSamples/
                                        (float)RhoSamplesAtten,
                                        ThetaSamplesAtten, span, 0,
                                        plane_separation, 0, 0.0f, false, 0, 0,
                                        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                        axes == 1);
        acf_sino->copyData(acf_data, 0, axes, axis_slices[0], 1, false, true,
                           "acf", loglevel);
      }
     if (normdata != NULL)                            // create object for norm
      { norm=new SinogramConversion(RhoSamplesNorm,
                                    BinSizeRho*(float)RhoSamples/
                                    (float)RhoSamplesNorm, ThetaSamplesNorm,
                                    span, 0, plane_separation, 0, 0.0f, false,
                                    0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0,
                                    axes == 1);
        norm->copyData(normdata, 0, axes, axis_slices[0], 1, false, true,
                       "norm", loglevel);
      }
                                         // uncorrect emission for norm and acf
     uncorrectEmissionANW(emi_sino, acf_sino, true, norm, true, max_threads);
     if (norm != NULL) { delete norm;
                         norm=NULL;
                       }
     if (acf_sino != NULL) { delete acf_sino;
                             acf_sino=NULL;
                           }
                                     // copy emission back into original buffer
     sp=sino;
     for (unsigned short int axis=0; axis < axes; axis++)
      { memcpy(sp, MemCtrl::mc()->getFloatRO(emi_sino->index(axis, 0),
                                             loglevel),
               emi_sino->size(axis)*sizeof(float));
        MemCtrl::mc()->put(emi_sino->index(axis, 0));
        sp+=emi_sino->size(axis);
      }
     delete emi_sino;
     emi_sino=NULL;
   }
   catch (...)
    { if (emi_sino != NULL) delete emi_sino;
      if (acf_sino != NULL) delete acf_sino;
      if (norm != NULL) delete norm;
      throw;
    }
 }
