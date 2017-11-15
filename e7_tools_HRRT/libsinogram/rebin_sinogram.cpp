/*! \class RebinSinogram rebin_sinogram.h "rebin_sinogram.h"
    \brief This class implements the rebinning of a sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
    \date 2005/02/25 use vectors

    This class implements the rebinning of a sinogram. The RebinX template is
    used to rebin in r- and t-direction. Rebinning in t-direction needs
    special care to avoid discontinuencies at 0 or 180 degrees. The
    calculations are done in parallel which each thread rebinning a set of
    slices. The number of threads is limited by the max_threads constant.
 */

#include <iostream>
#include <algorithm>
#include <limits>
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
#include <alloca.h>
#endif
#ifdef WIN32
#include <malloc.h>
#endif
#endif
#include "rebin_sinogram.h"
#include "e7_tools_const.h"
#include "exception.h"
#include "fastmath.h"
#include "mem_ctrl.h"
#include "thread_wrapper.h"

/*- local functions ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to execute sinogram rebinning.
    \param[in] param   pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to execute sinogram rebinning.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_RebinSinogram(void *param)
 { try
   { RebinSinogram::tthread_params *tp;

     tp=(RebinSinogram::tthread_params *)param;
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
      // allocate some padding memory on the stack in front of the thread-stack
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     tp->object->calc_rebin_sinogram(tp->sinogram, tp->new_sinogram,
                                     tp->slices, tp->plane_zoom_factor);
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
/*! \brief Initialize tables for rebinning algorithm.
    \param[in] _width               width of slice
    \param[in] _height              height of slice
    \param[in] _new_width           width of slice after rebinning
    \param[in] _new_height          height of slice after rebinning
    \param[in] _arc_correction      kind of arc correction
    \param[in] _crystals_per_ring   number of crystals per ring (only for arc-
                                    correction)
    \param[in] values               preserve values (otherwise preserve counts)

    Initialize tables for rebinning algorithm.
 */
/*---------------------------------------------------------------------------*/
RebinSinogram::RebinSinogram(const unsigned short int _width,
                         const unsigned short int _height,
                         const unsigned short int _new_width,
                         const unsigned short int _new_height,
                         const RebinX <float>::tarc_correction _arc_correction,
                         const unsigned short int _crystals_per_ring,
                         const bool values)
 { angular_rebin=NULL;
   try
   { width=_width;
     height=_height;
     new_width=_new_width;
     new_height=_new_height;
     preserve_values=values;
     arc_correction=_arc_correction;
     rebin_x=((new_width != width) ||
              (arc_correction != RebinX <float>::NO_ARC));
     crystals_per_ring=_crystals_per_ring;
     if (new_height != height)
      { float k, angular_zoom;
        bool linear;

        if (height < new_height) { linear=true;
                                   k=1.0f;
                                 }
         else { linear=false;
                k=0.5f;
              }
        dn=(unsigned short int)ceilf(k*(float)height/(float)new_height);
        nangles_padded=height+dn+1;
        angular_zoom=(float)nangles_padded/(float)height;
                          // initialize object for rebinning in theta-direction
        angular_rebin=new RebinX <float>(nangles_padded, new_width, new_height,
                                         RebinX <float>::NO_ARC, 0, (float)dn,
                                         0.0f, angular_zoom, linear,
                                         true, RebinX <float>::EDGE_NONE);
      }
   }
   catch (...)
    { if (angular_rebin != NULL) { delete angular_rebin;
                                   angular_rebin=NULL;
                                 }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
RebinSinogram::~RebinSinogram()
 { if (angular_rebin != NULL) delete angular_rebin;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multi-threaded rebinning of sinogram in r- and t-direction.
    \param[in]     sinogram            sinogram
    \param[in]     slices              number of slices
    \param[in]     plane_zoom_factor   zoom factor
    \param[in,out] sino_idx            memory controller index of sinogram
    \param[in]     loglevel            level for logging
    \param[in]     max_threads         maximum number of threads to use
    \return rebinned sinogram

    Multi-threaded rebinning of sinogram in r- and t-direction. Every thread
    rebinns a couple of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
float *RebinSinogram::calcRebinSinogram(float * const sinogram,
                                        unsigned short int slices,
                                        float * const plane_zoom_factor,
                                        unsigned short int * const sino_idx,
                                        const unsigned short int loglevel,
                                        const unsigned short int max_threads)
 { try
   { std::vector <tthread_id> tid;
     std::vector <bool> thread_running;
     unsigned short int threads=0, t;
     void *thread_result;

     try
     { unsigned long int new_sino_size;
       float *new_sino;
                                         // create buffer for rebinned sinogram
       new_sino_size=(unsigned long int)new_width*
                     (unsigned long int)new_height;

       new_sino=MemCtrl::mc()->createFloat(new_sino_size*
                                           (unsigned long int)slices,
                                           sino_idx, "sino", loglevel);
       if (!rebin_x)
        if (plane_zoom_factor != NULL)
         for (unsigned short int p=0; p < slices; p++)
          if (fabsf(plane_zoom_factor[p]-1.0f) > 1.0f/new_width)
           { rebin_x=true;
             break;
           }
                                                              // no rebinning ?
       if (!rebin_x && (angular_rebin == NULL))
        { memcpy(new_sino, sinogram,
                 new_sino_size*(unsigned long int)slices*sizeof(float));
          return(new_sino);
        }
       threads=std::min(max_threads, slices);
                                                           // single threaded ?
       if (threads == 1)
        { calc_rebin_sinogram(sinogram, new_sino, slices, plane_zoom_factor);
          return(new_sino);
        }
       unsigned short int i;
       unsigned long int sino_size;
       float *sp, *nsp, *zf;
       std::vector <RebinSinogram::tthread_params> tp;

       tid.resize(threads);
       tp.resize(threads);
       sino_size=(unsigned long int)width*(unsigned long int)height;
       thread_running.assign(threads, false);
       for (sp=sinogram,
            nsp=new_sino,
            zf=plane_zoom_factor,
            t=threads,
            i=0; i < threads; i++,// distribute sinogram onto different threads
            t--)
        { tp[i].object=this;
          tp[i].sinogram=sp;
          tp[i].new_sinogram=nsp;
          tp[i].slices=slices/t;
          tp[i].plane_zoom_factor=zf;
#ifdef XEON_HYPERTHREADING_BUG
          tp[i].thread_number=i;
#endif
          thread_running[i]=true;
                                                                // start thread
          ThreadCreate(&tid[i], &executeThread_RebinSinogram, &tp[i]);
          slices-=tp[i].slices;
          sp+=(unsigned long int)tp[i].slices*sino_size;
          nsp+=(unsigned long int)tp[i].slices*new_sino_size;
          if (zf != NULL) zf+=tp[i].slices;
        }
       for (i=0; i < threads; i++)       // wait until all threads are finished
        { ThreadJoin(tid[i], &thread_result);
          thread_running[i]=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
       return(new_sino);
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
/*! \brief Rebin sinogram planes in r- and t-direction.
    \param[in]     sino                sinogram
    \param[in,out] new_sino            rebinned sinogram
    \param[in]     slices              number of slices
    \param[in]     plane_zoom_factor   zoom factor

    Rebin sinogram planes in r- and t-direction.
 */
/*---------------------------------------------------------------------------*/
void RebinSinogram::calc_rebin_sinogram(float * const sino,
                                        float * const new_sino,
                                        const unsigned short int slices,
                                        float * const plane_zoom_factor)
 { RebinX <float> *radial_rebin=NULL;

   try
   { unsigned short int z;
     unsigned long int size_r;
     float *sp, *rp;
     std::vector <float> sino_rx, sinogram, buffer;

     size_r=(unsigned long int)new_width*(unsigned long int)height;
     if (rebin_x && (angular_rebin == NULL))   // rebin in rho-direction only ?
      {                     // initialize object for rebinning in rho-direction
        for (sp=sino,
             rp=new_sino,
             z=0; z < slices; z++,
             sp+=(unsigned long int)width*(unsigned long int)height,
             rp+=size_r)
         { float zf=1.0f;

           if (plane_zoom_factor != NULL) zf=plane_zoom_factor[z];
           radial_rebin=new RebinX <float>(width, height, new_width,
                                      arc_correction, crystals_per_ring,
                                      (float)width/2.0f, (float)new_width/2.0f,
                                      zf, (width < new_width*zf),
                                      preserve_values,
                                      RebinX <float>::EDGE_TRUNCATE);
           radial_rebin->calcRebinX(sp, rp, 1);
           delete radial_rebin;
           radial_rebin=NULL;
         }
        return;
      }
     unsigned long int sino_size;

     if (rebin_x) sino_rx.resize(size_r);
     sino_size=(unsigned long int)nangles_padded*(unsigned long int)new_width;
     buffer.resize((unsigned long int)new_width*(unsigned long int)new_height);
     for (sp=sino,
          rp=new_sino,
          z=0; z < slices; z++,
          sp+=(unsigned long int)width*(unsigned long int)height)
      { unsigned short int x, y;
        float *bp, *bp2, *sp2;

        if (rebin_x)
         if ((plane_zoom_factor != NULL) || (width != new_width) ||
              arc_correction)
          { float zf=1.0f;

            if (plane_zoom_factor != NULL) zf=plane_zoom_factor[z];
            radial_rebin=new RebinX <float>(width, height, new_width,
                                      arc_correction, crystals_per_ring,
                                      (float)width/2.0f, (float)new_width/2.0f,
                                      zf, (width < new_width*zf),
                                      preserve_values,
                                      RebinX <float>::EDGE_TRUNCATE);
            radial_rebin->calcRebinX(sp, &sino_rx[0], 1);
            delete radial_rebin;
            radial_rebin=NULL;
            sp2=&sino_rx[0];
          }
          else sp2=sp;
         else sp2=sp;
        sinogram.assign(sino_size, 0.0f);
                           // transposed copy of the radially rebinned sinogram
        for (bp=&sinogram[dn],
             y=0; y < height; y++,
             bp++,
             sp2+=new_width)
         for (bp2=bp,
              x=0; x < new_width; x++,
              bp2+=nangles_padded)
          *bp2=sp2[x];
              // copy the angle range below 180 degrees to the angle range left
              // of 0 degress and reverse radially, all except radius=0
        for (bp=&sinogram[nangles_padded+dn-1],
             bp2=&sinogram[sino_size-2],
             y=1; y < new_width; y++,
             bp2-=nangles_padded,
             bp+=nangles_padded)
         memcpy(bp, bp2, dn*sizeof(float));
                      // copy the zero angle to the 180 degree position, with a
                      // radial flip, all except radius=0
        for (bp=&sinogram[nangles_padded*2-1],
             bp2=&sinogram[sino_size-nangles_padded+dn],
             y=1; y < new_width; y++,
             bp2-=nangles_padded,
             bp+=nangles_padded)
         *bp=*bp2;
              // at angles left of 0 degress extrapolate radius=0 from radius=1
        memcpy(&sinogram[0], &sinogram[nangles_padded], dn*sizeof(float));
                          // at 180 degress, extrapolate radius=0 from radius=1
        bp=&sinogram[height];
        bp[0]=bp[nangles_padded];
                                                    // rebin in theta-direction
        angular_rebin->calcRebinX(&sinogram[0], &buffer[0], 1);
                                                              // transpose back
        for (bp=&buffer[0],
             y=0; y < new_height; y++,
             bp++,
             rp+=new_width)
         for (bp2=bp,
              x=0; x < new_width; x++,
              bp2+=new_height)
          rp[x]=bp2[0];
      }
     if (radial_rebin != NULL) { delete radial_rebin;
                                 radial_rebin=NULL;
                               }
   }
   catch (...)
    { if (radial_rebin != NULL) delete radial_rebin;
      throw;
    }
 }
