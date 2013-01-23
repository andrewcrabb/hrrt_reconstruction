/*! \class FBP fbp.h "fbp.h"
    \brief This class is used to reconstruct images from sinograms by filtered
           backprojection (FBP).
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/04/21 use correct center of rotation
    \date 2004/04/21 use vector code for filtering
    \date 2004/04/21 fixed shadow artefact in backprojector
    \date 2004/04/21 added Doxygen style comments
    \date 2004/04/22 added time-of-flight support
    \date 2004/04/22 reduced memory requirements in reconstruct
    \date 2004/05/13 replaced ramp filter
    \date 2004/05/13 added gaussian for time-of-flight reconstruction
    \date 2004/06/29 fixed quantitative scaling of image
    \date 2005/01/04 added progress reporting

    This class is used to reconstruct images from sinograms by filtered
    backprojection (FBP). Projections are padded, FFT'ed, filtered (ramp,0.5),
    iFFT'ed and backprojected. If the sinogram contains TOF information, the
    ramp filter is convoluted with a gaussian distribution, depending on the
    time resolution.

    The code is able to process non-TOF sinograms, TOF sinogram (shuffled or
    unshuffled), emission or transmission data.

    The reconstruction is multi-threaded, each thread calculates a set of
    planes. The number of threads is limited by the constant max_threads.
 */

#include <iostream>
#include <algorithm>
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__LINUX__) || defined(__SOLARIS__)
#include <alloca.h>
#endif
#ifdef WIN32
#include <malloc.h>
#endif
#endif
#ifdef WIN32
#include "global_tmpl.h"
#endif
#include "bckprj3d.h"
#include "const.h"
#include "fastmath.h"
#include "fbp.h"
#include "exception.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "progress.h"
#include "syngo_msg.h"
#include "thread_wrapper.h"
#include "types.h"
#include "vecmath.h"

/*- local functions ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to filter sinograms.
    \param[in] param   pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to filter sinograms.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_FBP_Filter_planes(void *param)
 { try
   { FBP::tthread_params *tp;

     tp=(FBP::tthread_params *)param;
#ifdef XEON_HYPERTHREADING_BUG
      // allocate some padding memory on the stack in front of the thread-stack
#if defined(__LINUX__) || defined(__SOLARIS__)
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     tp->object->filter_planes(tp->sinogram, tp->padded_view_sinogram,
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
/*! \brief Initialize object.
    \param[in] _RhoSamples     number of bins in projection
    \param[in] _BinSizeRho     width of bin in projection in mm
    \param[in] _ThetaSamples   number of projections in sinogram plane
    \param[in] planes          number of planes in sinogram
    \param[in] _XYSamples      width/height of image
    \param[in] _DeltaXY        width/height of pixel in image in mm
    \param[in] reb_factor      sinogram rebinning factor
    \param[in] tof_width       width of a TOF bin in ns
    \param[in] tof_fwhm        FWHM of TOF gaussian in ns
    \param[in] tof_bins        number of TOF bins
    \param[in] _loglevel       toplevel for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
FBP::FBP(const unsigned short int _RhoSamples, const float _BinSizeRho,
         const unsigned short int _ThetaSamples,
         const unsigned short int planes,
         const unsigned short int _XYSamples, const float _DeltaXY,
         const float reb_factor, const float tof_width, const float tof_fwhm,
         const unsigned short int tof_bins,
         const unsigned short int _loglevel)
 { init(_RhoSamples, _BinSizeRho, _ThetaSamples, planes, _XYSamples,
        _DeltaXY, reb_factor, tof_width, tof_fwhm, tof_bins, _loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] sino         sinogram
    \param[in] _XYSamples   width/height of image
    \param[in] _DeltaXY     width/height of pixel in image in mm
    \param[in] reb_factor   sinogram rebinning factor
    \param[in] _loglevel    toplevel for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
FBP::FBP(SinogramConversion * const sino, const unsigned short int _XYSamples,
         const float _DeltaXY, const float reb_factor,
         const unsigned short int _loglevel)
 { init(sino->RhoSamples(), sino->BinSizeRho(), sino->ThetaSamples(),
        sino->axisSlices()[0], _XYSamples, _DeltaXY, reb_factor,
        sino->TOFwidth(), sino->TOFfwhm(), sino->TOFbins(), _loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object and release used memory.

    Destroy the object and release the used memory.
 */
/*---------------------------------------------------------------------------*/
FBP::~FBP()
 { if (fft != NULL) delete fft;
   if (bckprj != NULL) delete bckprj;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Filter sinogram and convert into padded view-mode representation.
    \param[in]     sinogram               pointer to sinogram
    \param[in,out] padded_view_sinogram   pointer to padded view-mode sinogram
    \param[in]     z_start                first sinogram plane to filter
    \param[in]     z_end                  last sinogram plane+1 to filter

    Filter sinogram and convert into padded view-mode representation. Two
    projections of length n is padded by n 0-bins each. One of them is treated
    as real part, the other as imaginary part for a FFT of length 2n. The data
    is filtered in frequency space by a ramp filter and transformed back into
    sinogram space.
    In case of a TOF reconstruction the ramp filter is convoluted by a
    gaussian.
 */
/*---------------------------------------------------------------------------*/
void FBP::filter_planes(float * const sinogram,
                        float * const padded_view_sinogram,
                        const unsigned short int z_start,
                        const unsigned short int z_end) const
 { float *dp;
   unsigned short int z, tof_bins;
   std::string str;
   std::vector <float> real, imag;

   
   tof_bins=1;
   real.resize(RhoSamples*2);                    // buffer for real part of FFT
   imag.resize(RhoSamples*2);               // buffer for imaginary part of FFT
                                                             // filter sinogram
   for (dp=sinogram+(unsigned long int)z_start*sino_planesize_TOF,
        z=z_start; z < z_end; z++)
    { float *vp;

      vp=padded_view_sinogram+(unsigned long int)(z+1)*
                              (unsigned long int)RhoSamples_padded_TOF;
                            // filter two neighbouring projections in each step
      for (unsigned short int t=0; t < ThetaSamples; t+=2,
           dp+=RhoSamples_TOF*2,
           vp+=view_size_padded_TOF*2)
       for (unsigned short int w=0; w < tof_bins; w++)
        { unsigned short int r;
                                   // use first projection as real part for FFT
                                   // and second projection as imaginary part
          for (r=0; r < RhoSamples; r++)
           { real[r]=dp[r*tof_bins+w];
             imag[r]=dp[RhoSamples_TOF+r*tof_bins+w];
           }
                                                          // padding with zeros
          memset(&real[RhoSamples], 0, RhoSamples*sizeof(float));
          memset(&imag[RhoSamples], 0, RhoSamples*sizeof(float));
          fft->FFT_1D(&real[0], &imag[0], true);                 // forward FFT
                                                     // filter FT of projection
          vecMul(&real[0], &filter_kernel_real[0], &real[0], RhoSamples*2);
          vecMul(&imag[0], &filter_kernel_real[0], &imag[0], RhoSamples*2);
          fft->FFT_1D(&real[0], &imag[0], false);                       // iFFT
                                    // copy back into padded view-mode sinogram
          for (r=1; r <= RhoSamples; r++)
           { vp[r*tof_bins+w]=real[r-1];
             vp[view_size_padded_TOF+r*tof_bins+w]=imag[r-1];
           }
        }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object and ramp filter.
    \param[in] _RhoSamples      number of bins in projection
    \param[in] _BinSizeRho      width of bin in projection in mm
    \param[in] _ThetaSamples    number of projections in sinogram plane
    \param[in] planes           number of planes in sinogram
    \param[in] _XYSamples       width/height of image
    \param[in] _DeltaXY         width/height of pixel in image in mm
    \param[in] reb_factor       sinogram rebinning factor
    \param[in] tof_width        width of a TOF bin in ns
    \param[in] tof_fwhm         FWHM of TOF gaussian in ns
    \param[in] tof_bins         number TOF bins
    \param[in] _loglevel        toplevel for logging

    Initialize object and ramp filter.
 */
/*---------------------------------------------------------------------------*/
void FBP::init(const unsigned short int _RhoSamples, const float _BinSizeRho,
               const unsigned short int _ThetaSamples,
               const unsigned short int planes,
               const unsigned short int _XYSamples, const float _DeltaXY,
               const float reb_factor, const float tof_width,
               const float tof_fwhm, const unsigned short int tof_bins,
               const unsigned short int _loglevel)
 { fft=NULL;
   bckprj=NULL;
   try
   { std::vector <unsigned short int> as(1);

     loglevel=_loglevel;
     SyngoMsg("FBP");
     XYSamples=_XYSamples;
     DeltaXY=_DeltaXY;
     RhoSamples=_RhoSamples;
     BinSizeRho=_BinSizeRho;
     ThetaSamples=_ThetaSamples;
     // \exception REC_FBP_ANGLES_ODD number of projections in sinogram is odd
     //                               number
     // if (ThetaSamples & 0x1)
     //  throw Exception(REC_FBP_ANGLES_ODD,
     //                "The number of angles in the sinogram must be even for "
     //                "FBP.");
     Logging::flog()->logMsg("XYSamples=#1", loglevel)->arg(XYSamples);
     Logging::flog()->logMsg("DeltaXY=#1 mm", loglevel)->arg(DeltaXY);
     Logging::flog()->logMsg("RhoSamples=#1", loglevel)->arg(RhoSamples);
     Logging::flog()->logMsg("BinSizeRho=#1mm", loglevel)->arg(BinSizeRho);
     Logging::flog()->logMsg("ThetaSamples=#1", loglevel)->arg(ThetaSamples);
     Logging::flog()->logMsg("TOF bins=#1", loglevel)->arg(tof_bins);
     if (tof_bins > 1)
      { Logging::flog()->logMsg("TOF width=#1ns", loglevel)->arg(tof_width);
        Logging::flog()->logMsg("TOF FWHM=#1ns", loglevel)->arg(tof_fwhm);
      }
     image_planesize=(unsigned long int)XYSamples*
                     (unsigned long int)XYSamples;
     fft=new FFT(RhoSamples*2);                    // initialize object for FFT
     ramp_filter();                              // calculate the filter kernel
     RhoSamples_TOF=RhoSamples*tof_bins;
     RhoSamples_padded_TOF=RhoSamples_TOF+tof_bins*2;
     view_size_padded_TOF=(unsigned long int)(planes+2)*
                          (unsigned long int)RhoSamples_padded_TOF;
     sino_planesize_TOF=(unsigned long int)RhoSamples_TOF*
                      (unsigned long int)ThetaSamples;
     as[0]=planes;
     bckprj=new BckPrj3D(XYSamples, DeltaXY, RhoSamples, BinSizeRho,
                         ThetaSamples, as, 1, 2, 1.0f, reb_factor, 1, 1,
                         loglevel+2, 1);
   }
   catch (...)                                             // handle exceptions
    { if (fft != NULL) { delete fft;
                         fft=NULL;
                       }
      if (bckprj != NULL) { delete bckprj;
                            bckprj=NULL;
                          }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a ramp filter kernel.

    Create a ramp filter kernel with a cut-off value of 0.5 in frequency
    domain. The shifted filter is calculated in image space by:
    \f[
       kernel[\mbox{RhoSamples}-i]=\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{1}{4} & i=0\\
         0 & i\;\mbox{even}\\
         -\frac{1}{i^2\pi^2} & i\;\mbox{odd}\\
        \end{array}\right.\quad\forall\;\; 0\le i<\mbox{RhoSamples}
    \f]
    \note see also "Principles of Computerized Tomographic Imaging",
          Avinash C. Kak & Malcolm Slaney, IEEE press, 1999, p. 72

    It is mirrored by:
    \f[
        kernel[\mbox{RhoSamples}+i]=kernel[\mbox{RhoSamples}-i]\qquad
        \forall\;\; 0<i<\mbox{RhoSamples}
    \f]
    \image html "rampfilter_img.jpg" "Ramp filter in image space (RhoSamples=336)"
    Filtering will be done in frequency space, therefore the filter kernel is
    stored as:
    \f[
        filter\_kernel=|FFT(kernel)|
    \f]
    \image html "rampfilter_freq.jpg" "Ramp filter in frequency space (RhoSamples=336)"

    For TOF-FBP the ramp filter needs to be convoluted by a gaussian with the
    FWHM of the time resolution. The shifted gaussian distribution is
    calculated in image space by:
    \f[
        \sigma=\frac{fwhm}{2}\frac{SPEED\_OF\_LIGHT}{2\sqrt{-2*\ln(0.5)}}
    \f]
    \f[
        kernel[i]=\left\{
        \begin{array}{r@{\quad:\quad}l}
         e^{\frac{-(i\Delta_\rho)^2}{2\sigma^2}} & 0\le i<\mbox{RhoSamples}\\
         0 & i=\mbox{RhoSamples}
        \end{array}\right.
    \f]
    and mirrored by:
    \f[
        kernel[\mbox{RhoSamples}+i]=kernel[\mbox{RhoSamples}-i]\qquad
        \forall\;\; 0<i<\mbox{RhoSamples}
    \f]
    \image html "gaussfilter_toffbp.jpg" "Gaussian filter in image space (RhoSamples=336, fwhm=1.20422ns)"
    The frequency space convolution of both filters is done in image space by a
    multiplication:
    \f[
        filter\_kernel=|FFT(iFFT(filter\_kernel)\cdot kernel)|
    \f]
    resulting in a slightly modified ramp filter:
    \image html "filter_toffbp.jpg" "TOF-filter in frequency space (RhoSamples=336, fwhm=1.20422ns)"
 */
/*---------------------------------------------------------------------------*/
void FBP::ramp_filter()
 { unsigned short int i;
   std::vector <float> filter_kernel_imag;
                                               // create kernel in space domain
   filter_kernel_real.assign(RhoSamples*2, 0.0f);
   filter_kernel_real[RhoSamples]=0.25f;
   for (i=1; i < RhoSamples; i+=2)
    { filter_kernel_real[RhoSamples-i]=-1.0f/((float)(i*i)*M_PI*M_PI);
      filter_kernel_real[RhoSamples+i]=filter_kernel_real[RhoSamples-i];
    }
                                        // transform filter to frequency domain
   filter_kernel_imag.assign(RhoSamples*2, 0.0f);
   fft->FFT_1D(&filter_kernel_real[0], &filter_kernel_imag[0], true);
   vecCplxAbs(&filter_kernel_real[0], &filter_kernel_imag[0],
              &filter_kernel_real[0], RhoSamples*2);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Reconstruct 2d sinograms with filtered backprojection.
    \param[in,out] sino            sinogram
    \param[in]     delete_sino     delete sinogram after use ?
    \param[in]     num             matrix number for image
    \param[out]    _fov_diameter   diameter of reconstructed FOV in mm
    \param[in]     max_threads     maximum number of threads to use
    \return reconstructed image

    Reconstruct 2d sinograms with filtered backprojection.
    This method is able to process non-TOF sinograms, TOF sinogram
    (shuffled and unshuffled), emission or transmission data. TOF sinograms
    will be shuffled after reconstruction. If the diameter of the FOV is not
    needed, this parameter can be set to NULL.

    The method is multi-threaded. Each thread filters a range of sinogram
    planes.
*/
/*---------------------------------------------------------------------------*/
ImageConversion *FBP::reconstruct(SinogramConversion *sino,
                                  const bool delete_sino,
                                  const unsigned short int num,
                                  float * const _fov_diameter,
                                  const unsigned short int max_threads)
 { try
   { void *thread_result;
     unsigned short int threads=0, t, sino_view_idx=
                               std::numeric_limits <unsigned short int>::max();

     std::vector <bool> thread_running(0);
     std::vector <tthread_id> tid;
     ImageConversion *image=NULL;

     try
     { std::vector <tthread_params> tp;
       bool is_acf;
       float *view_sino;

       Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2,
                                "FBP reconstruction (frame #1)")->arg(num);
       sino->shuffleTOFdata(true, loglevel);             // reorganize TOF data
       is_acf=sino->isACF();
       sino->tra2Emi(loglevel);
       threads=std::min(max_threads, sino->axisSlices()[0]);
       if (_fov_diameter != NULL) *_fov_diameter=fov_diameter;
       view_sino=MemCtrl::mc()->createFloat(view_size_padded_TOF*
                                            (unsigned long int)ThetaSamples,
                                            &sino_view_idx, "sino", loglevel);
       memset(view_sino, 0, view_size_padded_TOF*
                            (unsigned long int)ThetaSamples*sizeof(float));
       Logging::flog()->logMsg("filter planes 0-#1", loglevel+1)->
        arg(sino->axisSlices()[0]-1);
       if (threads == 1)
        filter_planes(MemCtrl::mc()->getFloat(sino->index(0, 0), loglevel),
                      view_sino, 0, sino->axisSlices()[0]-1);
        else { unsigned short int i, slices, z_start;
               float *sp;

               tid.resize(threads);
               tp.resize(threads);
               thread_running.assign(threads, false);
               for (sp=MemCtrl::mc()->getFloat(sino->index(0, 0), loglevel),
                    slices=sino->axisSlices()[0],
                    z_start=0,
                    t=threads,
                    i=0; i < threads; i++,
                    t--)
                { tp[i].object=this;
                  tp[i].sinogram=sp;
                  tp[i].padded_view_sinogram=view_sino;
                  tp[i].z_start=z_start;
                  tp[i].z_end=z_start+slices/t;
                  tp[i].thread_number=i;
                  thread_running[i]=true;
                                                               // create thread
                  ThreadCreate(&tid[i], &executeThread_FBP_Filter_planes,
                               &tp[i]);
                  slices-=(tp[i].z_end-tp[i].z_start);
                  z_start=tp[i].z_end;
                }
                                              // wait for all threads to finish
               for (i=0; i < threads; i++)
                { ThreadJoin(tid[i], &thread_result);
                  thread_running[i]=false;
                  if (thread_result != NULL) throw (Exception *)thread_result;
                }
             }
       MemCtrl::mc()->put(sino->index(0, 0));
       if (delete_sino) sino->deleteData(0, 0);
        else if (is_acf) sino->emi2Tra(loglevel);

       Logging::flog()->logMsg("backproject #1 planes", loglevel+1)->
        arg(sino->axisSlices()[0]);
       image=new ImageConversion(XYSamples, DeltaXY, sino->axisSlices()[0],
                                 sino->planeSeparation(), num, sino->LLD(),
                                 sino->ULD(), sino->startTime(),
                                 sino->frameDuration(), sino->gateDuration(),
                                 sino->halflife(), sino->bedPos());
       image->initImage(is_acf, false, "img", loglevel);
       bckprj->back_proj3d(view_sino, MemCtrl::mc()->getFloat(image->index(),
                                                              loglevel), 0, 0,
                           false, max_threads);
       MemCtrl::mc()->put(sino_view_idx);
       MemCtrl::mc()->deleteBlock(&sino_view_idx);
       MemCtrl::mc()->put(image->index());
       // Logging::flog()->logMsg("scale #1 planes of image by #2", loglevel+1)->
       //        arg(sino->axisSlices()[0])->arg(M_PI/(float)ThetaSamples);
       // image->scale(M_PI/(float)ThetaSamples, loglevel+1);
                                                                 // scale image
       Logging::flog()->logMsg("scale image with #1", loglevel+2)->
        arg(BinSizeRho);
       image->scale(BinSizeRho, loglevel+2);
       { float sf;

         sf=((float)RhoSamples/(float)XYSamples)*
            ((float)RhoSamples/(float)XYSamples);
         Logging::flog()->logMsg("scale image with #1", loglevel+1)->arg(sf);
         image->scale(sf, loglevel+1);
         sf=(float)GM::gm()->ThetaSamples()/(float)ThetaSamples*
            (float)RhoSamples/(float)GM::gm()->RhoSamples();
         Logging::flog()->logMsg("scale image with #1", loglevel+1)->arg(sf);
         image->scale(sf, loglevel+1);
       }
       return(image);
     }
     catch (...)
      { thread_result=NULL;
        for (t=0; t < thread_running.size(); t++)
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
                       // if we catch exceptions from the terminating threads,
                       // we store only the first one and ignore the others
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
        if (image != NULL) delete image;
        MemCtrl::mc()->put(sino->index(0, 0));
        MemCtrl::mc()->put(sino_view_idx);
        MemCtrl::mc()->deleteBlock(&sino_view_idx);
                    // if the threads have thrown exceptions we throw the first
                    // one of them, not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Reconstruct 2d sinograms with filtered backprojection.
    \param[in]  sino            pointer to sinogram
    \param[in]  planes          number of planes in sinogram
    \param[out] _fov_diameter   diameter of reconstructed FOV in mm
    \param[in]  max_threads     maximum number of threads to use
    \return pointer to reconstructed image

    Reconstruct 2d sinograms with filtered backprojection.
    This method is only able to process non-TOF emission sinograms. If the
    diameter of the FOV is not needed, this parameter can be set to NULL.

    The method is multi-threaded. Each thread processes a range of sinogram
    planes.
*/
/*---------------------------------------------------------------------------*/
float *FBP::reconstruct(float * const sinogram,
                        const unsigned short int planes,
                        float * const _fov_diameter,
                        const unsigned short int max_threads)
 { ImageConversion *image=NULL;
   SinogramConversion *sino=NULL;
   float *img=NULL;

   try
   { sino=new SinogramConversion(RhoSamples, BinSizeRho, ThetaSamples, 0, 0,
                                 0.0f, 0, 0.0f, false, 0, 0, 0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 0, true);
     sino->copyData(sinogram, 0, 0, planes, 1, false, false, "emis", loglevel);
     image=reconstruct(sino, true, 0, _fov_diameter, max_threads);
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
