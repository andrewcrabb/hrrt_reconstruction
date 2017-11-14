/*! \class Sino2D_3D sino2d_3d.h "sino2d_3d.h"
    \brief This class implements the fourier projection algorithm to convert a
           2d sinogram into a 3d sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
 
    This class implements the fourier projection algorithm to convert a 2d
    sinogram into a 3d sinogram. Segment 0 of the 3d sinogram is the same as
    the 2d sinogram. The other segments of the 3d sinogram are calculated by
    the fourier rebinning algorithm, described in

    "Exact and Approximate Rebinning Algorithms for 3D PET Data",
    M. Defrise, et. al., pp. 145-158 of the April 1997 issue of
    IEEE Transactions on Medical Imaging.

    The 3d FFT of the 2d sinogram is calculated and one thread is generated
    per axis to calculate the 3d sinogram for that axis from the 3d FFTof the
    2d sinogram. In principle all axes could be calculated in parallel, but
    this would need lots of memory. Therefore the number of axes calculated in
    parallel is limited by the max_threads constant.
    The number of angles in the 2d sinogram must be a power of 2.
 */

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
#include <alloca.h>
#endif
#ifdef WIN32
#include <malloc.h>
#endif
#endif
#include "sino2d_3d.h"
#include "const.h"
#include "exception.h"
#include "fastmath.h"
#include "global_tmpl.h"
#include "gm.h"
#include "logging.h"
#include "raw_io.h"
#include "str_tmpl.h"
#include "thread_wrapper.h"
#include "vecmath.h"

/*- local functions ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to execute axis calculation.
    \param[in] param   pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to execute axis calculation.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_CalcSino3D_Axis(void *param)
 { try
   { Sino2D_3D::tthread_params *tp;

     tp=(Sino2D_3D::tthread_params *)param;
#ifdef XEON_HYPERTHREADING_BUG
      // allocate some padding memory on the stack in front of the thread-stack
#if defined(__linux__) || defined(__SOLARIS__)
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     tp->object->CalcSino3D_Axis(tp->sino_3d_axis, tp->axis_size,
                                 tp->startplane, tp->axis_planes, tp->rd,
                                 tp->axis, tp->sino_2d_fft, tp->transmission,
                                 tp->max_threads, tp->thread_number, true);
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
/*! \brief Convert emission into transmission by applying exp-function.
    \param[in,out] data   pointer to sinogram
    \param[in]     size   size of dataset

    Convert emission into transmission by applying exp-function. Negative
    values in the sinogram are cut-off.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Emi2Tra(T * const data, const unsigned long int size)
 { for (unsigned long int i=0; i < size; i++)
    if (data[i] < T()) data[i]=1;                   // cut-off negative values
     else data[i]=(T)exp(data[i]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert transmission into emission by applying log-function.
    \param[in,out] data   pointer to sinogram
    \param[in]     size   size of dataset

    Convert transmission into emission by applying log-function. Values smaller
    than 1 are cut-off.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Tra2Emi(T * const data, const unsigned long int size)
 { for (unsigned long int i=0; i < size; i++)
    if (data[i] > 1.0) data[i]=(T)log(data[i]);
     else data[i]=T();                         // don't produce negative values
 }

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _RhoSamples     number of bins in projections
    \param[in] _ThetaSamples   number of angles in sinograms
    \param[in] _axis_slices    number of planes for axes of 3d sinogram
    \param[in] _BinSizeRho     width of bins in 2d sinogram in mm
    \param[in] _ring_spacing   distance between two detector rings in mm
    \param[in] _loglevel       top level for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
Sino2D_3D::Sino2D_3D(const unsigned short int _RhoSamples,
                     const unsigned short int _ThetaSamples,
                     const std::vector <unsigned short int> _axis_slices,
                     const float _BinSizeRho, const float _ring_spacing,
                     const unsigned short int _loglevel)
 { init(_RhoSamples, _ThetaSamples, _axis_slices, _BinSizeRho, _ring_spacing,
        _loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] sino           2d sinogram
    \param[in] _axis_slices   number of planes for axes of 3d sinogram
    \param[in] _loglevel      top level for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
Sino2D_3D::Sino2D_3D(SinogramConversion * const sino,
                     const std::vector <unsigned short int> _axis_slices,
                     const unsigned short int _loglevel)
 { init(sino->RhoSamples(), sino->ThetaSamples(), _axis_slices,
        sino->BinSizeRho(), sino->planeSeparation()*2.0f, _loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
Sino2D_3D::~Sino2D_3D()
 { if (fft != NULL) delete fft;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate 3d sinogram from 2d sinogram.
    \param[in,out] sino_2d       2d sinogram on input, 3d sinogram on output
    \param[in]     sino_nr       number of sinogram
    \param[in]     name          name for swap file
    \param[in]     max_threads   maximum number of threads to use

    Calculate 3d sinogram from 2d sinogram. The segments of the 3d sinogram are
    calculated in parallel.

    \todo switch on the multi-threading for the parallel calculation of the
          segments
 */
/*---------------------------------------------------------------------------*/
void Sino2D_3D::calcSino3D(SinogramConversion * const sino_2d,
                           const unsigned short int sino_nr,
                           const std::string name,
                           const unsigned short int max_threads)
 { try
   { std::vector <tthread_id> tid;
     unsigned short int threads=0, t;
     std::vector <bool> thread_running;
     void *thread_result;

     try
     { unsigned long int axis_size;
       float *s2dp, *sino_2dp, *sino_2d_padded, *sino_2d_emi;
       unsigned short int span, axes_slices=0, s2dp_idx, axis, sino_2d_emi_idx;
       bool acf_data;
       std::vector <Sino2D_3D::tthread_params> tp;

       axis_size=sino_plane_size*(unsigned long int)axis_slices[0];
       acf_data=sino_2d->isACF();
                                              // cut-off negative values in ACF
       if (acf_data) sino_2d->cutBelow(1.0f, 1.0f, loglevel);
       Logging::flog()->logMsg("copy #1 planes of segment 0", loglevel)->
        arg(axis_slices[0]);
       for (axis=0; axis < axis_slices.size(); axis++)
        axes_slices+=axis_slices[axis];
                                                            // copy 2d sinogram
       sino_2d_emi=MemCtrl::mc()->createFloat(axis_size, &sino_2d_emi_idx,
                                              "sino2d", loglevel);
       memcpy(sino_2d_emi, MemCtrl::mc()->getFloatRO(sino_2d->index(0,
                                                                    sino_nr),
                                                     loglevel),
              axis_size*sizeof(float));
       MemCtrl::mc()->put(sino_2d->index(0, sino_nr));
       if (acf_data)                      // convert transmission into emission
        Tra2Emi(sino_2d_emi,
                sino_plane_size*(unsigned long int)axis_slices[0]);
                                      // create padded sinogram for 360 degrees
       Logging::flog()->logMsg("create 360 degrees sinogram (#1x#2x#3)",
                               loglevel)->arg(RhoSamples_padded)->
        arg(ThetaSamples*2)->arg(axis_slices[0]);
       sino_2d_padded=MemCtrl::mc()->createFloat(sinosize_padded, &s2dp_idx,
                                                 "s360", loglevel);
       memset(sino_2d_padded, 0, sinosize_padded*sizeof(float));
       s2dp=sino_2d_padded;
       sino_2dp=sino_2d_emi;
       for (unsigned short int p=0; p < axis_slices[0]; p++,
            s2dp+=sino_plane_size_padded)
        for (unsigned short int t=0; t < ThetaSamples; t++,
             s2dp+=RhoSamples_padded,
             sino_2dp+=RhoSamples)
         { unsigned short int r;
           float *sp;
                                                             // [0,180) degrees
           memcpy(s2dp, sino_2dp+rs_2, rs_2*sizeof(float));

           memcpy(s2dp+RhoSamples_padded-rs_2, sino_2dp, rs_2*sizeof(float));
                                                           // [180,360) degrees
           sp=s2dp+sino_plane_size_padded+RhoSamples_padded-rs_2;
           sino_2dp+=RhoSamples-1;
           for (r=1; r < rs_2; r++)
            sp[r]=*sino_2dp--;

           sp=s2dp+sino_plane_size_padded;
           for (r=0; r < rs_2; r++)
            sp[r]=*sino_2dp--;
         }
       MemCtrl::mc()->put(sino_2d_emi_idx);
       MemCtrl::mc()->deleteBlock(&sino_2d_emi_idx);
       if (axis_slices.size() == 1) span=0;
        else span=axis_slices[0]-axis_slices[1]/2-1;
                                                // 3D-FFT of padded 2d sinogram
       Logging::flog()->logMsg("calculate 3d fourier transform of sinogram",
                               loglevel);
       fft->rFFT_3D(sino_2d_padded, true, max_threads);
                                          // calculate number of threads to use
       threads=std::min(max_threads,
                        (unsigned short int)(axis_slices.size()-1));
threads=1; // if this is not set, the multithreaded code sometimes leads to
           // a deadlock when allocating the memory for an axis
       sino_2d->putAxesTable(axis_slices);
       sino_2d->resizeIndexVec(sino_nr, axis_slices.size());
                                                  // calculate oblique segments
       if (threads == 1)                         // single-threaded calculation
        { for (unsigned short int axis=1; axis < axis_slices.size(); axis++)
           { float *ptr;
 
             Logging::flog()->logMsg("calculate #1 planes of axis #2",
                                     loglevel)->arg(axis_slices[axis])->
              arg(axis);
                                                              // calculate axis
             ptr=sino_2d->initFloatAxis(axis, sino_nr, 1, false, false, name,
                                        loglevel);
             CalcSino3D_Axis(ptr, sino_plane_size*
                             (unsigned long int)axis_slices[axis],
                             axis*span-(span-1)/2, axis_slices[axis],
                             span*axis, axis, sino_2d_padded, acf_data,
                             max_threads, 1, false);
             MemCtrl::mc()->put(sino_2d->index(axis, sino_nr));
             Logging::flog()->logMsg("finished with axis #1", loglevel)->
              arg(axis);
           }
          MemCtrl::mc()->put(s2dp_idx);
          MemCtrl::mc()->deleteBlock(&s2dp_idx);
          sino_2d->isACF(acf_data);
          return;
        }
                                                  // calculate oblique segments
                                                  // multi-threaded calculation
       unsigned short int next=0;

       tid.resize(threads);
       tp.resize(threads);
       thread_running.assign(threads, false);
       for (axis=1; axis < axis_slices.size(); axis++)
        { if (axis > threads)                         // wait for free thread ?
           { ThreadJoin(tid[next], &thread_result);
             thread_running[next]=false;
             MemCtrl::mc()->put(sino_2d->index(tp[next].axis, sino_nr));
             if (thread_result != NULL) throw (Exception *)thread_result;
             Logging::flog()->logMsg("thread #1 finished with axis #2",
                                     loglevel)->arg(next+1)->
              arg(tp[next].axis);
           }
          Logging::flog()->logMsg("start thread #1 to calculate #2 planes of "
                                  "axis #3", loglevel)->arg(next+1)->
           arg(axis_slices[axis])->arg(axis);
                                                          // create next thread
          axis_size=sino_plane_size_padded*
                    (unsigned long int)axis_slices[axis];
          tp[next].object=this;
          tp[next].sino_3d_axis=sino_2d->initFloatAxis(axis, sino_nr, 1, false,
                                                       false, name, loglevel);
          tp[next].axis_size=axis_size;
          tp[next].startplane=axis*span-(span-1)/2;
          tp[next].axis_planes=axis_slices[axis];
          tp[next].rd=span*axis;
          tp[next].axis=axis;
          tp[next].sino_2d_fft=sino_2d_padded;
          tp[next].transmission=acf_data;
          tp[next].thread_number=next;
          tp[next].max_threads=max_threads;
          thread_running[next]=true;
          ThreadCreate(&tid[next], &executeThread_CalcSino3D_Axis, &tp[next]);
          // this leads to out-of-sync problems in the logging with Windows:
          // next=(next+1) % threads;                  // number of next thread
          if (next == threads-1) next=0;
           else next++;
        }
                                              // wait for all threads to finish
       for (t=0; t < threads; t++)
        { ThreadJoin(tid[next], &thread_result);
          thread_running[next]=false;
          MemCtrl::mc()->put(sino_2d->index(tp[next].axis, sino_nr));
          Logging::flog()->logMsg("thread #1 finished with axis #2",
                                  loglevel)->arg(next+1)->arg(tp[next].axis);
          if (thread_result != NULL) throw (Exception *)thread_result;
          next=(next+1) % threads;                     // number of next thread
        }
       MemCtrl::mc()->put(s2dp_idx);
       MemCtrl::mc()->deleteBlock(&s2dp_idx);
       sino_2d->isACF(acf_data);
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
/*! \brief Calculate 3d sinogram from 3d fft of stack of 2d emission sinograms
           for the two oblique segments of one axis
    \param[in,out] sino_3d_axis   data for axis of 3d sinogram
    \param[in]     axis_size      size of axis dataset
    \param[in]     startplane     start plane of this axis
    \param[in]     axis_planes    number of planes for axis
    \param[in]     rd             ring difference of this axis
    \param[in]     axis           number of axis
    \param[in,out] e2d            3d-FFT of 2d emission sinograms
    \param[in]     transmission   transmission data ?
                                  (otherwise: emission data)
    \param[in]     max_threads    maximum number of threads to use
    \param[in]     thread_num     number of thread
    \param[in]     threaded       method called as a thread ?

    Calculate 3d sinogram from 3d fft of stack of 2d emission sinograms for the
    two oblique segments of one axis.
 */
/*---------------------------------------------------------------------------*/
void Sino2D_3D::CalcSino3D_Axis(float * const sino_3d_axis,
                                const unsigned long int axis_size,
                                const unsigned short int startplane,
                                const unsigned short int axis_planes,
                                const unsigned short int rd,
                                const unsigned short int axis,
                                float * const e2d,
                                const bool transmission,
                                const unsigned short int max_threads,
                                const unsigned short int thread_num,
                                const bool threaded) const
 { float *ep_p, *ep_m, *e3dp, *e3d;
   unsigned short int p, e3d_idx;
   std::string str;

   if (threaded) str="thread "+toString(thread_num+1)+": ";
    else str=std::string();
   e3d=MemCtrl::mc()->createFloat(sinosize_padded, &e3d_idx, "e3d",
                                  loglevel+1);
   memset(e3d, 0, sinosize_padded*sizeof(float));
   Logging::flog()->logMsg(str+"calculate fourier projection of axis #1",
                           loglevel+1)->arg(axis);
   exact_fore(e2d, e3d, rd);                 // exact fourier rebinning of axis
                                                         // inverse FFT of axis
   Logging::flog()->logMsg(str+"calculate inverse fourier transform of axis "
                           "#1", loglevel+1)->arg(axis);
   fft->rFFT_3D(e3d, false, startplane, startplane+axis_planes/2,
                max_threads);
   Logging::flog()->logMsg(str+"split axis #1 into 2 segments", loglevel+1)->
    arg(axis);
                                           // extract data of segment +s and -s
   for (ep_p=sino_3d_axis+axis_size/2,
        ep_m=sino_3d_axis+RhoSamples-1,
        e3dp=e3d+(unsigned long int)startplane*sino_plane_size_padded*2,
        p=0; p < axis_planes/2; p++)
    { unsigned short int t;

      for (t=0; t < ThetaSamples; t++,                            // segment +s
           e3dp+=RhoSamples_padded,
           ep_p+=RhoSamples)
       { memcpy(ep_p, e3dp+RhoSamples_padded-rs_2, rs_2*sizeof(float));
         memcpy(ep_p+rs_2, e3dp, rs_2*sizeof(float));
       }
      for (t=ThetaSamples; t < ThetaSamples*2; t++,               // segment -s
           e3dp+=RhoSamples_padded,
           ep_m+=RhoSamples*2-1)
       { unsigned short int r;
         float *ep;

         ep=e3dp+RhoSamples_padded-rs_2+1;
         for (r=0; r < rs_2-1; r++)
          *ep_m--=ep[r];
         for (r=0; r < rs_2; r++)
          *ep_m--=e3dp[r];
       }
    }
   MemCtrl::mc()->put(e3d_idx);
   MemCtrl::mc()->deleteBlock(&e3d_idx);
   Logging::flog()->logMsg(str+"calculate weighting of axis #1", loglevel+1)->
    arg(axis);
   Weighting(sino_3d_axis, axis_size, rd);                 // weighting of axis
                               // calculate exp of 3d emission sinogram segment
                               // (emission -> transmission)
   if (transmission) Emi2Tra(sino_3d_axis, axis_size);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Estimate 3d-FFT of a 3d sinogram from the 3d-FFT of segment 0
    \param[in,out] s2D   3d-FFT of 2d sinogram
    \param[in,out] s3D   3d-FFT of 3d sinogram
    \param[in]     rd    ring difference for 3d sinogram

    Estimate the 3d-FFT of a stack of 3d sinograms with a given ring difference
    from the 3d-FFT of a stack of 2d sinograms with ring difference 0
    (formula 14).

    Features:
     - linear interpolation in the radial frequency
     - uses the fact that the phase in equation 16 is odd in each (radial,
       angular, axial) frequency
 
    Data organization with the real FFT (1D). Example for the radial dimension.

    real input x[r], r=0...RhoSamples

    output contains DFT for frequencies nur=del*r, r=0..RhoSamples/2,
    del=1/(RhoSamples*sampling)

    real part in x[r]

    imaginary part in x[rI] where rI=RhoSamples-r, this imaginary part exists
    only if 0 < r < RhoSamples/2.

    We call the real part the C term (for cosinus) and the imaginary part the
    S term (for sinus).

    Data organization for the 3D FFT:

    output contains DFT for frequencies
     nuz=delz*z, z=0..planes_padded/2, delz=1/(planes_padded*slice spacing)
     k=0..ThetaSamples,
     nur=del*r, r=0..RhoSamples/2, del=1/(RhoSamples*sampling)

    and also, by symmetry, over the negative frequencies. For each 3D
    frequency, there are normally 8 terms in the matrix, corresponding to
    indices (r or rI) x (k or kI=ThetaSamples2-k) x (z or zI=planes_padded-z).
    This yields terms denoted CCC,CCS,CSC,SCC,SSC,SCS,CSS,SSS.
    E.g. SSC=a[zI][kI][r]. Then:

    real part for frequency delz*z,k,del*r is CCC-CSS-SCS-SSC

    imaginary part for frequency delz*z,k,del*r is SCC+CSC+CCS-SSS

    The DFT for negative frequency is obtained by changing the sign of the S
    terms where frequency is negative. For example:

    real part for frequency delz*z,-k,del*r is CCC+CSS-SCS+SSC

    imaginary part for frequency delz*z,-k,del*r is SCC-CSC+CCS+SSS

    or:

    real part for frequency delz*z,-k,-del*r is CCC-CSS+SCS+SSC

    imaginary part for frequency delz*z,-k,-del*r is SCC-CSC-CCS-SSS

    When one of the indices is equal to 0 or to half the dimension, all S
    terms disappear, e.g.

    real part for frequency 0,k,del*r is CCC-CSS

    imaginary part for frequency 0,k,del*r is CSC+CCS

    Conversely, from the real (imaginary) part of the DFT for positive and
    negative frequencies, one can recover the four terms CCC,CSS,CSC,SSC
    (SCC,CCS,CSC,SSS).

    Rebinning: simplified by the fact that phase is odd in k, in r and in z.
    If CCC' represents the CCC term for the shifted frequency, etc.... one
    obtains:

     CCC=cos(phase) CCC'-sin(phase) SSS'

     SSS=sin(phase) CCC'+cos(phase) SSS'

    and the same rotation for the other pairs (e.g. CSS and SCC etc).
 */
/*---------------------------------------------------------------------------*/
void Sino2D_3D::exact_fore(float * const s2D, float * const s3D,
                           const unsigned short int rd) const
 { float nurx, nuz, phase, fact, cc, ss, p, q, *s3D_z_v_r, *s3D_zI_v_r,
         *s3D_z_vI_r, *s3D_zI_vI_r, *s3D_z_v_rI, *s3D_zI_v_rI, *s3D_z_vI_rI,
         *s3D_zI_vI_rI, *s2D_z_v_r1, *s2D_z_v_r2, *s2D_zI_v_r1, *s2D_zI_v_r2,
         *s2D_z_v_r1I, *s2D_z_v_r2I, *s2D_zI_v_r1I, *s2D_zI_v_r2I,
         *s2D_z_vI_r1, *s2D_z_vI_r2, *s2D_zI_vI_r1, *s2D_zI_vI_r2,
         *s2D_z_vI_r1I, *s2D_z_vI_r2I, *s2D_zI_vI_r1I, *s2D_zI_vI_r2I, cc_p,
         cc_q, ss_p, ss_q, *s3D_z, *s3D_zI, *s2D_z, *s2D_zI;
   unsigned short int r1, r2, r1I, r2I, rs_2, v;
   unsigned long int offs;

   rs_2=RhoSamples_padded/2;
   offs=(unsigned long int)(ThetaSamples*2-1)*
        (unsigned long int)RhoSamples_padded;
                                                  // factor for axial frequency
   //   delnu=1.0/((float)RhoSamples*BinSizeRho);
   //   delz=1.0/(planes_padded*ring_spacing/2.0);
   //   fact=delz*rd*ring_spacing/(ring_diameter*delnu);
   fact=(float)(RhoSamples_padded*rd*2)*BinSizeRho/
        (ring_diameter*(float)planes_padded);
              // zero the frequency rs_2 (maximum radial frequency) to simplify
              // the coding by reducing the number of exceptions; the resulting
              // errors are below 1% and only visible with noise free data
   { float *sp;

     sp=s2D+rs_2;
     for (unsigned long int zv=0; zv < (unsigned long int)planes_padded*
                                       (unsigned long int)ThetaSamples*2; zv++,
          sp+=RhoSamples_padded)
      *sp=0.0f;
   }
                                    // rebinning for planes [1,planes_padded/2)
   for (unsigned short int z=1; z < planes_padded/2; z++)
    { unsigned short int zI;

                // axial frequency multiplied by r, in unit of radial frequency
      nuz=z*fact;
      zI=planes_padded-z;           // index of sinus component for frequency z
      s3D_z=s3D+z*sino_plane_size_padded*2;
      s3D_zI=s3D+zI*sino_plane_size_padded*2;
      s2D_z=s2D+z*sino_plane_size_padded*2;
      s2D_zI=s2D+zI*sino_plane_size_padded*2;
                                  // rebinning for radial bins [1,RhoSamples/2)
      for (unsigned short int r=1; r < rs_2; r++)
       { unsigned short int rI;

         rI=RhoSamples_padded-r;    // index of sinus component for frequency r
         nurx=sqrtf(r*r+nuz*nuz);// modified radial frequency in sampling units
                                     // indices of the shifted radial frequency
                 // since we have put the rs_2 component = 0 in the input data,
                 // we can set an index equal to rs_2 whenever a component
                 // should be zero, thereby avoiding many "if" statements
         r1=std::min((unsigned short int)nurx, rs_2);
         r1I=RhoSamples_padded-r1;
         r2=std::min((unsigned short int)(r1+1), rs_2);
         r2I=RhoSamples_padded-r2;
                                           // linear interpolation coefficients
         p=nurx-r1;
         q=1.0-p;
         phase=atanf(nuz/r);                        // phase shift (formula 21)
         s3D_z_v_r=s3D_z+r;
         s3D_zI_v_r=s3D_zI+r;
         s3D_z_v_rI=s3D_z+rI;
         s3D_zI_v_rI=s3D_zI+rI;

         s2D_z_v_r1=s2D_z+r1;
         s2D_z_v_r2=s2D_z+r2;
         s2D_zI_v_r1=s2D_zI+r1;
         s2D_zI_v_r2=s2D_zI+r2;
         s2D_z_v_r1I=s2D_z+r1I;
         s2D_z_v_r2I=s2D_z+r2I;
         s2D_zI_v_r1I=s2D_zI+r1I;
         s2D_zI_v_r2I=s2D_zI+r2I;
                                          // rebinning for angle 0 (-> phase=0)
         *s3D_z_v_r=q**s2D_z_v_r1+p**s2D_z_v_r2;
         *s3D_zI_v_r=q**s2D_zI_v_r1+p**s2D_zI_v_r2;
         *s3D_z_v_rI=q**s2D_z_v_r1I+p**s2D_z_v_r2I;
         *s3D_zI_v_rI=q**s2D_zI_v_r1I+p**s2D_zI_v_r2I;
                                                    // rebinning for odd angles
         for (s3D_z_v_r+=RhoSamples_padded,
              s3D_zI_v_r+=RhoSamples_padded,
              s3D_z_v_rI+=RhoSamples_padded,
              s3D_zI_v_rI+=RhoSamples_padded,
              s3D_z_vI_r=s3D_z+offs+r,
              s3D_zI_vI_r=s3D_zI+offs+r,
              s3D_z_vI_rI=s3D_z+offs+rI,
              s3D_zI_vI_rI=s3D_zI+offs+rI,
              s2D_z_v_r1+=RhoSamples_padded,
              s2D_z_v_r2+=RhoSamples_padded,
              s2D_zI_v_r1+=RhoSamples_padded,
              s2D_zI_v_r2+=RhoSamples_padded,
              s2D_z_v_r1I+=RhoSamples_padded,
              s2D_z_v_r2I+=RhoSamples_padded,
              s2D_zI_v_r1I+=RhoSamples_padded,
              s2D_zI_v_r2I+=RhoSamples_padded,
              s2D_z_vI_r1=s2D_z+offs+r1,
              s2D_z_vI_r2=s2D_z+offs+r2,
              s2D_zI_vI_r1=s2D_zI+offs+r1,
              s2D_zI_vI_r2=s2D_zI+offs+r2,
              s2D_z_vI_r1I=s2D_z+offs+r1I,
              s2D_z_vI_r2I=s2D_z+offs+r2I,
              s2D_zI_vI_r1I=s2D_zI+offs+r1I,
              s2D_zI_vI_r2I=s2D_zI+offs+r2I,
              v=1; v < ThetaSamples; v++,
              s2D_zI_vI_r2I-=RhoSamples_padded,
              s2D_zI_vI_r1I-=RhoSamples_padded,
              s2D_z_vI_r2I-=RhoSamples_padded,
              s2D_z_vI_r1I-=RhoSamples_padded,
              s2D_zI_vI_r2-=RhoSamples_padded,
              s2D_zI_vI_r1-=RhoSamples_padded,
              s2D_z_vI_r2-=RhoSamples_padded,
              s2D_z_vI_r1-=RhoSamples_padded,
              s2D_zI_v_r2I+=RhoSamples_padded,
              s2D_zI_v_r1I+=RhoSamples_padded,
              s2D_z_v_r2I+=RhoSamples_padded,
              s2D_z_v_r1I+=RhoSamples_padded,
              s2D_zI_v_r2+=RhoSamples_padded,
              s2D_zI_v_r1+=RhoSamples_padded,
              s2D_z_v_r2+=RhoSamples_padded,
              s2D_z_v_r1+=RhoSamples_padded,
              s3D_z_v_r+=RhoSamples_padded,
              s3D_zI_v_r+=RhoSamples_padded,
              s3D_z_v_rI+=RhoSamples_padded,
              s3D_zI_v_rI+=RhoSamples_padded,
              s3D_z_vI_r-=RhoSamples_padded,
              s3D_zI_vI_r-=RhoSamples_padded,
              s3D_z_vI_rI-=RhoSamples_padded,
              s3D_zI_vI_rI-=RhoSamples_padded)
          { cc=cosf((float)v*phase);
            cc_p=cc*p;
            cc_q=cc*q;
            ss=sinf((float)v*phase);
            ss_p=ss*p;
            ss_q=ss*q;
             // calculate the 8 output terms corresponding to this frequency
             // (as can be seen from formula 6, for the 2D sinograms, when v is
             //  odd, only the xxS terms are non zero, i.e. the terms in r1I or
             //  r1I, while the terms in r1 and r2 should be zero)
            if (v & 0x1)
             { *s3D_z_v_r=-ss_q**s2D_zI_vI_r1I-ss_p**s2D_zI_vI_r2I;
               *s3D_zI_v_r=ss_q**s2D_z_vI_r1I+ss_p**s2D_z_vI_r2I;
               *s3D_z_vI_r=ss_q**s2D_zI_v_r1I+ss_p**s2D_zI_v_r2I;
               *s3D_z_v_rI=cc_q**s2D_z_v_r1I+cc_p**s2D_z_v_r2I;
               *s3D_zI_vI_r=-ss_q**s2D_z_v_r1I-ss_p**s2D_z_v_r2I;
               *s3D_zI_v_rI=cc_q**s2D_zI_v_r1I+cc_p**s2D_zI_v_r2I;
               *s3D_z_vI_rI=cc_q**s2D_z_vI_r1I+cc_p**s2D_z_vI_r2I;
               *s3D_zI_vI_rI=cc_q**s2D_zI_vI_r1I+cc_p**s2D_zI_vI_r2I;
             }
             else {
             // (as can be seen from formula 6, for the 2D sinograms, when v is
             //  even, only the xxC terms are non zero, i.e. the terms in r1 or
             //  r2, while the terms in r1I and r2I should be zero)
                    *s3D_z_v_r=cc_q**s2D_z_v_r1+cc_p**s2D_z_v_r2;
                    *s3D_zI_v_r=cc_q**s2D_zI_v_r1+cc_p**s2D_zI_v_r2;
                    *s3D_z_vI_r=cc_q**s2D_z_vI_r1+cc_p**s2D_z_vI_r2;
                    *s3D_z_v_rI=ss_q**s2D_zI_vI_r1+ss_p**s2D_zI_vI_r2;
                    *s3D_zI_vI_r=cc_q**s2D_zI_vI_r1+cc_p**s2D_zI_vI_r2;
                    *s3D_zI_v_rI=-ss_q**s2D_z_vI_r1-ss_p**s2D_z_vI_r2;
                    *s3D_z_vI_rI=-ss_q**s2D_zI_v_r1-ss_p**s2D_zI_v_r2;
                    *s3D_zI_vI_rI=ss_q**s2D_z_v_r1+ss_p**s2D_z_v_r2;
                  }
          }
                                            // rebinning for angle ThetaSamples
         cc=cosf(ThetaSamples*phase);
         cc_p=cc*p;
         cc_q=cc*q;
         *s3D_z_vI_r=cc_q**s2D_z_vI_r1+cc_p**s2D_z_vI_r2;
         *s3D_zI_vI_r=cc_q**s2D_zI_vI_r1+cc_p**s2D_zI_vI_r2;
         *s3D_z_vI_rI=cc_q**s2D_z_vI_r1I+cc_p**s2D_z_vI_r2I;
         *s3D_zI_vI_rI=cc_q**s2D_zI_vI_r1I+cc_p**s2D_zI_vI_r2I;
       }
                                                  // rebinning for radial bin 0
      nurx=nuz;                  // modified radial frequency in sampling units
                                     // indices of the shifted radial frequency
                 // since we have put the rs_2 component = 0 in the input data,
                 // we can set an index equal to rs_2 whenever a component
                 // should be zero, thereby avoiding many "if" statements
      r1=std::min((unsigned short int)nurx, rs_2);
      if (r1 == 0) r1I=rs_2;
       else r1I=RhoSamples_padded-r1;
      r2=std::min((unsigned short int)(r1+1), rs_2);
      r2I=RhoSamples_padded-r2;
                                           // linear interpolation coefficients
      p=nurx-r1;
      q=1.0-p;
      s3D_z_v_r=s3D_z;
      s3D_zI_v_r=s3D_zI;

      s2D_z_v_r1=s2D_z+r1;
      s2D_z_v_r2=s2D_z+r2;
      s2D_zI_v_r1=s2D_zI+r1;
      s2D_zI_v_r2=s2D_zI+r2;
                                          // rebinning for angle 0 (-> phase=0)
      *s3D_z_v_r=q**s2D_z_v_r1+p**s2D_z_v_r2;
      *s3D_zI_v_r=q**s2D_zI_v_r1+p**s2D_zI_v_r2;
                                       // rebinning for angles [1,ThetaSamples)
      for (s3D_z_v_r+=RhoSamples_padded,
           s3D_zI_v_r+=RhoSamples_padded,
           s3D_z_vI_r=s3D_z+offs,
           s3D_zI_vI_r=s3D_zI+offs,
           s2D_z_v_r1+=RhoSamples_padded,
           s2D_z_v_r2+=RhoSamples_padded,
           s2D_zI_v_r1+=RhoSamples_padded,
           s2D_zI_v_r2+=RhoSamples_padded,
           s2D_zI_vI_r1I=s2D_zI+offs+r1I,
           s2D_zI_vI_r2I=s2D_zI+offs+r2I,
           s2D_z_vI_r1I=s2D_z+offs+r1I,
           s2D_z_vI_r2I=s2D_z+offs+r2I,
           s2D_z_vI_r1=s2D_z+offs+r1,
           s2D_z_vI_r2=s2D_z+offs+r2,
           s2D_zI_v_r1I=s2D_zI+RhoSamples_padded+r1I,
           s2D_zI_v_r2I=s2D_zI+RhoSamples_padded+r2I,
           s2D_zI_vI_r1=s2D_zI+offs+r1,
           s2D_zI_vI_r2=s2D_zI+offs+r2,
           s2D_z_v_r1I=s2D_z+RhoSamples_padded+r1I,
           s2D_z_v_r2I=s2D_z+RhoSamples_padded+r2I,
           v=1; v < ThetaSamples; v++,
           s2D_z_v_r2I+=RhoSamples_padded,
           s2D_z_v_r1I+=RhoSamples_padded,
           s2D_zI_vI_r2-=RhoSamples_padded,
           s2D_zI_vI_r1-=RhoSamples_padded,
           s2D_zI_v_r2I+=RhoSamples_padded,
           s2D_zI_v_r1I+=RhoSamples_padded,
           s2D_z_vI_r2-=RhoSamples_padded,
           s2D_z_vI_r1-=RhoSamples_padded,
           s2D_z_vI_r2I-=RhoSamples_padded,
           s2D_z_vI_r1I-=RhoSamples_padded,
           s2D_zI_vI_r2I-=RhoSamples_padded,
           s2D_zI_vI_r1I-=RhoSamples_padded,
           s2D_zI_v_r2+=RhoSamples_padded,
           s2D_zI_v_r1+=RhoSamples_padded,
           s2D_z_v_r2+=RhoSamples_padded,
           s2D_z_v_r1+=RhoSamples_padded,
           s3D_z_v_r+=RhoSamples_padded,
           s3D_zI_v_r+=RhoSamples_padded,
           s3D_z_vI_r-=RhoSamples_padded,
           s3D_zI_vI_r-=RhoSamples_padded)
       { cc=cosf((float)v*M_PI_2);
         cc_p=cc*p;
         cc_q=cc*q;
         ss=sinf((float)v*M_PI_2);
         ss_p=ss*p;
         ss_q=ss*q;
         *s3D_z_v_r=cc_q**s2D_z_v_r1+cc_p**s2D_z_v_r2-
                    ss_q**s2D_zI_vI_r1I-ss_p**s2D_zI_vI_r2I;
         *s3D_zI_v_r=cc_q**s2D_zI_v_r1+cc_p**s2D_zI_v_r2+
                     ss_q**s2D_z_vI_r1I+ss_p**s2D_z_vI_r2I;
         *s3D_z_vI_r=cc_q**s2D_z_vI_r1+cc_p**s2D_z_vI_r2+
                     ss_q**s2D_zI_v_r1I+ss_p**s2D_zI_v_r2I;
         *s3D_zI_vI_r=cc_q**s2D_zI_vI_r1+cc_p**s2D_zI_vI_r2-
                      ss_q**s2D_z_v_r1I-ss_p**s2D_z_v_r2I;
       }
                                            // rebinning for angle ThetaSamples
      cc=cosf((float)ThetaSamples*M_PI_2);
      cc_p=cc*p;
      cc_q=cc*q;
      *s3D_z_vI_r=cc_q**s2D_z_vI_r1+cc_p**s2D_z_vI_r2;
      *s3D_zI_vI_r=cc_q**s2D_zI_vI_r1+cc_p**s2D_zI_vI_r2;
    }
                                 // rebinning for z=0 -> no phase, no rescaling
   memcpy(s3D, s2D, sino_plane_size_padded*2*sizeof(float));
                                             // rebinning for z=planes_padded/2
   { unsigned short int z;

     z=planes_padded/2;
     s3D_z=s3D+z*sino_plane_size_padded*2;
     s2D_z=s2D+z*sino_plane_size_padded*2;
     nuz=z*fact;// axial frequency multiplied by r, in unit of radial frequency
                                  // rebinning for radial bins [1,RhoSamples/2)
     for (unsigned short int r=1; r < rs_2; r++)
      { unsigned short int rI;

        rI=RhoSamples_padded-r;            // index of sinus component for frequency r
        nurx=sqrtf(r*r+nuz*nuz); // modified radial frequency in sampling units
                                     // indices of the shifted radial frequency
                 // since we have put the rs_2 component = 0 in the input data,
                 // we can set an index equal to rs_2 whenever a component
                 // should be zero, thereby avoiding many "if" statements
        r1=std::min((unsigned short int)nurx, rs_2);
        r1I=RhoSamples_padded-r1;
        r2=std::min((unsigned short int)(r1+1), rs_2);
        r2I=RhoSamples_padded-r2;
                                           // linear interpolation coefficients
        p=nurx-r1;
        q=1.0-p;

        phase=atanf(nuz/r);                         // phase shift (formula 21)
        s3D_z_v_r=s3D_z+r;
        s3D_z_v_rI=s3D_z+rI;

        s2D_z_v_r1=s2D_z+r1;
        s2D_z_v_r2=s2D_z+r2;
        s2D_z_v_r1I=s2D_z+r1I,
        s2D_z_v_r2I=s2D_z+r2I,
                                          // rebinning for angle 0 (-> phase=0)
        *s3D_z_v_r=q**s2D_z_v_r1+p**s2D_z_v_r2;
        *s3D_z_v_rI=q**s2D_z_v_r1I+p**s2D_z_v_r2I;
                                       // rebinning for angles [1,ThetaSamples)
        for (s3D_z_v_r+=RhoSamples_padded,
             s3D_z_v_rI+=RhoSamples_padded,
             s3D_z_vI_r=s3D_z+offs+r,
             s3D_z_vI_rI=s3D_z+offs+rI,
             s2D_z_v_r1+=RhoSamples_padded,
             s2D_z_v_r2+=RhoSamples_padded,
             s2D_z_v_r1I+=RhoSamples_padded,
             s2D_z_v_r2I+=RhoSamples_padded,
             s2D_z_vI_r1=s2D_z+offs+r1,
             s2D_z_vI_r2=s2D_z+offs+r2,
             s2D_z_vI_r1I=s2D_z+offs+r1I,
             s2D_z_vI_r2I=s2D_z+offs+r2I,
             v=1; v < ThetaSamples; v++,
             s2D_z_vI_r2I-=RhoSamples_padded,
             s2D_z_vI_r1I-=RhoSamples_padded,
             s2D_z_vI_r2-=RhoSamples_padded,
             s2D_z_vI_r1-=RhoSamples_padded,
             s2D_z_v_r1I+=RhoSamples_padded,
             s2D_z_v_r2I+=RhoSamples_padded,
             s2D_z_v_r1+=RhoSamples_padded,
             s2D_z_v_r2+=RhoSamples_padded,
             s3D_z_vI_rI-=RhoSamples_padded,
             s3D_z_vI_r-=RhoSamples_padded,
             s3D_z_v_rI+=RhoSamples_padded,
             s3D_z_v_r+=RhoSamples_padded)
         { cc=cosf((float)v*phase);
           cc_p=cc*p;
           cc_q=cc*q;
           *s3D_z_v_r=cc_q**s2D_z_v_r1+cc_p**s2D_z_v_r2;
           *s3D_z_vI_r=cc_q**s2D_z_vI_r1+cc_p**s2D_z_vI_r2;
           *s3D_z_v_rI=cc_q**s2D_z_v_r1I+cc_p**s2D_z_v_r2I;
           *s3D_z_vI_rI=cc_q**s2D_z_vI_r1I+cc_p**s2D_z_vI_r2I;
         }
                                            // rebinning for angle ThetaSamples
        cc=cosf((float)ThetaSamples*phase);
        cc_p=cc*p;
        cc_q=cc*q;
        *s3D_z_vI_r=cc_q**s2D_z_vI_r1+cc_p**s2D_z_vI_r2;
        *s3D_z_vI_rI=cc_q**s2D_z_vI_r1I+cc_p**s2D_z_vI_r2I;
      }
                                                  // rebinning for radial bin 0
     nurx=nuz;                   // modified radial frequency in sampling units
                                     // indices of the shifted radial frequency
                 // since we have put the rs_2 component = 0 in the input data,
                 // we can set an index equal to rs_2 whenever a component
                 // should be zero, thereby avoiding many "if" statements
     r1=std::min((unsigned short int)nurx, rs_2);
     if (r1 == 0) r1I=rs_2;
      else r1I=RhoSamples_padded-r1;
     r2=std::min((unsigned short int)(r1+1), rs_2);
     r2I=RhoSamples_padded-r2;
                                            // linear interpolation coefficient
     p=nurx-r1;
     q=1.0-p;
     s3D_z_v_r=s3D_z;
     s2D_z_v_r1=s2D_z+r1;
     s2D_z_v_r2=s2D_z+r2;
                                          // rebinning for angle 0 (-> phase=0)
     *s3D_z_v_r=q**s2D_z_v_r1+p**s2D_z_v_r2;
                                       // rebinning for angles [1,ThetaSamples)
     for (s3D_z_v_r+=RhoSamples_padded,
          s3D_z_vI_r=s3D_z+offs,
          s2D_z_v_r1+=RhoSamples_padded,
          s2D_z_v_r2+=RhoSamples_padded,
          s2D_z_vI_r1=s2D_z+offs+r1,
          s2D_z_vI_r2=s2D_z+offs+r2,
          v=1; v < ThetaSamples; v++,
          s2D_z_vI_r2-=RhoSamples_padded,
          s2D_z_vI_r1-=RhoSamples_padded,
          s2D_z_v_r1+=RhoSamples_padded,
          s2D_z_v_r2+=RhoSamples_padded,
          s3D_z_v_r+=RhoSamples_padded,
          s3D_z_vI_r-=RhoSamples_padded)
      { cc=cosf((float)v*M_PI_2);
        cc_p=cc*p;
        cc_q=cc*q;
        *s3D_z_v_r=cc_q**s2D_z_v_r1+cc_p**s2D_z_v_r2;
        *s3D_z_vI_r=cc_q**s2D_z_vI_r1+cc_p**s2D_z_vI_r2;
      }
                                            // rebinning for angle ThetaSamples
     *s3D_z_vI_r=cosf((float)ThetaSamples*M_PI_2)*
                 (q**s2D_z_vI_r1+p**s2D_z_vI_r2);
   }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _RhoSamples     number of bins in projections
    \param[in] _ThetaSamples   number of angles in sinograms
    \param[in] _axis_slices    number of planes for axes of 3d sinogram
    \param[in] _BinSizeRho     width of bins in 2d sinogram in mm
    \param[in] _ring_spacing   distance between two detector rings in mm
    \param[in] _loglevel       top level for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
void Sino2D_3D::init(const unsigned short int _RhoSamples,
                     const unsigned short int _ThetaSamples,
                     const std::vector <unsigned short int> _axis_slices,
                     const float _BinSizeRho, const float _ring_spacing,
                     const unsigned short int _loglevel)
 { fft=NULL;
   try
   { RhoSamples=_RhoSamples;
     RhoSamples_padded=RhoSamples*2;
     ThetaSamples=_ThetaSamples;
     axis_slices=_axis_slices;
     ring_diameter=(GM::gm()->ringRadius()+GM::gm()->DOI())*2.0f,
     ring_spacing=_ring_spacing;
     BinSizeRho=_BinSizeRho;
     rs_2=RhoSamples/2;
     loglevel=_loglevel;
                                    // to avoid ringing artefacts from the FFTs
                                    // the calculations are done on padded data
     planes_padded=2;
     while (planes_padded < axis_slices[0]) planes_padded*=2;
     sino_plane_size=(unsigned long int)RhoSamples*
                     (unsigned long int)ThetaSamples;
     sino_plane_size_padded=(unsigned long int)RhoSamples_padded*
                            (unsigned long int)ThetaSamples;
     sinosize_padded=sino_plane_size_padded*2*(unsigned long int)planes_padded;
                                                           // initialize 3d FFT
     fft=new FFT(RhoSamples_padded, ThetaSamples*2, planes_padded);
   }
   catch (...)
    { if (fft != NULL) { delete fft;
                         fft=NULL;
                       }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform weighting of axis (formula 3).
    \param[in,out] data   pointer to 3d sinogram of axis
    \param[in]     size   size of 3d sinogram of axis
    \param[in]     rd     ring difference

    Perform weighting of axis (formula 3).
 */
/*---------------------------------------------------------------------------*/
void Sino2D_3D::Weighting(float * const data, const unsigned long int size,
                          const unsigned short int rd) const
 { float scale;

   scale=(float)rd*ring_spacing/ring_diameter;               // ring difference
   vecMulScalar(data, sqrtf(1.0f+scale*scale), data, size);
 }
