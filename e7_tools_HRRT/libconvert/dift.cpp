/*! \class DIFT dift.h "dift.h"
    \brief This class implements the DIFT (direct inverse fourier transform)
           reconstruction of 2d sinograms.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/02/02 fixed the distortion-correction function, if the image
                     size is not a power of 2
    \date 2005/01/25 added Doxygen style comments

    The FFTs of the projections are calculated and shifted to the center of
    rotation, and then the polar projections are distributed into the cartesian
    grid of frequency space. The reconstructed image is iFFT'ed into image
    space, corrected for distortions and shifted by one quadrant. The planes of
    the image are calculated in parallel. Each thread calculates an even number
    of planes. The number of threads is limited by the max_threads constant.
    This code is originally based on Jim Hammils IDL DIFT reconstruction.
 */

#include <algorithm>
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__)
#include <alloca.h>
#endif
#ifdef WIN32
#include <malloc.h>
#endif
#endif
#include "const.h"
#include "dift.h"
#include "exception.h"
#include "fastmath.h"
#include "fft.h"
#include "global_tmpl.h"
#include "logging.h"
#include "progress.h"
#include "syngo_msg.h"
#include "thread_wrapper.h"
#include "types.h"

/*- constants ---------------------------------------------------------------*/

                            // expand projections by this factor (with padding)
const unsigned short int DIFT::EXPAND=4;
const unsigned short int DIFT::N_CORNERS=4;              // corners to a square
const float DIFT::SMALL_VALUE=0.0001f;  // weights smaller than this are zeroed

/*- local functions ---------------------------------------------------------*/

 // The thread API is a C-API so C linkage and calling conventions have to be
 // used, when creating a thread. To use a method as thread, a helper function
 // in C is needed, that calls the method.

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to execute DIFT reconstruction.
    \param[in] param   pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to execute DIFT reconstruction.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_DIFT(void *param)
 { try
   { DIFT::tthread_params *tp;

     tp=(DIFT::tthread_params *)param;
#ifdef XEON_HYPERTHREADING_BUG
      // allocate some padding memory on the stack in front of the thread-stack
#if defined(__linux__) || defined(__SOLARIS__)
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     tp->object->calc_dift(tp->sinogram, tp->image, tp->slices,
                           tp->fov_diameter, tp->max_threads,
                           tp->thread_number, true);
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
    \param[in] _ThetaSamples   number of angles in sinogram
    \param[in] _DeltaXY        with/height of voxel in image
    \param[in] _loglevel       top level for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
DIFT::DIFT(const unsigned short int _RhoSamples,
           const unsigned short int _ThetaSamples, const float _DeltaXY,
           const unsigned short int _loglevel)
 { fft_rho_expand=NULL;
   fft2d=NULL;
   try
   { SyngoMsg("DIFT");
     XYSamples=_RhoSamples;
     DeltaXY=_DeltaXY;
     origRhoSamples=_RhoSamples;
     ThetaSamples=_ThetaSamples;
     loglevel=_loglevel;
     Logging::flog()->logMsg("XYSamples=#1", loglevel)->arg(XYSamples);
     Logging::flog()->logMsg("DeltaXY=#1 mm", loglevel)->arg(DeltaXY);
     Logging::flog()->logMsg("RhoSamples=#1", loglevel)->arg(origRhoSamples);
     Logging::flog()->logMsg("ThetaSamples=#1", loglevel)->arg(ThetaSamples);
     RhoSamples=2;
     while (RhoSamples < origRhoSamples) RhoSamples*=2;
     RhoSamplesExpand=RhoSamples*EXPAND;
     XYSamples_padded=RhoSamples;
     image_plane_size=(unsigned long int)XYSamples*
                      (unsigned long int)XYSamples;
     image_plane_size_padded=(unsigned long int)XYSamples_padded*
                             (unsigned long int)XYSamples_padded;
     sino_plane_size=(unsigned long int)RhoSamples*
                     (unsigned long int)ThetaSamples;
                   // transformation into frequency space by real-valued 1D-FFT
     fft_rho_expand=new FFT(RhoSamplesExpand);
                    // transformation into image space by complex-valued 2D-FFT
     fft2d=new FFT(XYSamples_padded, XYSamples_padded);
      // array for weights used in coordinate transform from polar to cartesian
     c.resize(N_CORNERS*(RhoSamplesExpand+1)*ThetaSamples);
     indices.resize(c.size());// array for indices used in coordinate transform
     dift_pol_cartes(&c, &indices);   // create tables for coordinate transform
                             // create table for complex exp-function, used for
                             // shifting of projections in frequency space
     cor_array=init_shift_table();
     init_sinc_table();      // tables for distortion-correction and image mask
   }
   catch (...)
    { if (fft2d != NULL) { delete fft2d;
                           fft2d=NULL;
                         }
      if (fft_rho_expand != NULL) { delete fft_rho_expand;
                                    fft_rho_expand=NULL;
                                  }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
DIFT::~DIFT()
 { if (fft2d != NULL) delete fft2d;
   if (fft_rho_expand != NULL) delete fft_rho_expand;
 }

/*---------------------------------------------------------------------------*/
/*! \brief DIFT reconstruction.
    \param[in]     sinogram       sinogram data
    \param[in,out] image          reconstructed image
    \param[in]     slices         number of sinogram slices
    \param[out]    fov_diameter   diameter of FOV in mm
    \param[in]     max_threads    maximum number of threads to use
    \param[in]     thread_num     number of thread
    \param[in]     threaded       method called as a thread ?

    DIFT reconstruction of a stack of slices. This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void DIFT::calc_dift(float * const sinogram, float * const image,
                     const unsigned short int slices,
                     float * const fov_diameter,
                     const unsigned short int max_threads,
                     const unsigned short int thread_num, const bool threaded)
 { unsigned short int z;
   float *sp, *ip;
   std::string str;
   std::vector <float> image_buffer;

   if (threaded) str="thread "+toString(thread_num+1)+": ";
    else str=std::string();
   Logging::flog()->logMsg(str+"reconstruct #1 planes", loglevel+1)->
    arg(slices);
   image_buffer.resize(image_plane_size*2);
   for (sp=sinogram,
        ip=image,
        z=0;  z < slices; z+=2,
        ip+=2*image_plane_size,
        sp+=sino_plane_size)
    { bool fake_odd_slice;
      float *c_ptr;
      unsigned short int t;
      unsigned long int *ind_ptr;
      std::vector <float> image_buffer_padded;
                                 // if number of slices is odd, the last slice
                                 // (to make it an even number) is a zero slice
      fake_odd_slice=(z == slices-1) && (slices & 0x1);
      image_buffer_padded.assign(image_plane_size_padded*2, 0);
      for (ind_ptr=&indices[0],
           c_ptr=&c[0],
           t=0; t < ThetaSamples; t++,
           c_ptr+=N_CORNERS*(RhoSamplesExpand+1),
           ind_ptr+=N_CORNERS*(RhoSamplesExpand+1),
           sp+=RhoSamples)
                           // calculate FFT of projections and distribute polar
                           // projections into cartesian frequency domain image
       distribute(dift_pad_ft(sp, fake_odd_slice), &image_buffer_padded[0],
                  ind_ptr, c_ptr);
                             // transform reconstruction back into image domain
      fft2d->FFT_2D(&image_buffer_padded[0],
                    &image_buffer_padded[image_plane_size_padded], false,
                    max_threads);
                                                              // remove padding
      { unsigned short int offs, xs_2;

        offs=XYSamples_padded-XYSamples;
        xs_2=XYSamples/2;
        for (unsigned short int plane=0; plane < 2; plane++)
         for (unsigned short int y=0; y < XYSamples; y++)
          { float *ip, *is;

            ip=&image_buffer[plane*image_plane_size+y*XYSamples];
            is=&image_buffer_padded[plane*image_plane_size_padded+
                                    y*XYSamples_padded];
            if (y >= xs_2) is+=offs*XYSamples_padded;
            memcpy(ip, is, xs_2*sizeof(float));
            memcpy(ip+xs_2, is+offs+xs_2, xs_2*sizeof(float));
          }
      }
                   // correct two image slices for distortion, apply image mask
                   // and shift images by one quadrant
      correct_image(&image_buffer[0], fov_diameter);
      shift_image(&image_buffer[0], ip);
      if (!fake_odd_slice)
       { correct_image(&image_buffer[image_plane_size], NULL);
         shift_image(&image_buffer[image_plane_size], ip+image_plane_size);
       }
    }
   Logging::flog()->logMsg(str+"scale #1 planes", loglevel+1)->arg(slices);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Correct for the distortions of the polar to cartesian coordinate
           transformation.
    \param[in,out] image          data of image slice
    \param[out]    fov_diameter   diameter of FOV in mm

    Correct for the distortions of the polar to cartesian coordinate
    transformation by dividing by \f$sinc^2(x)*sinc^2(y)\f$ and mask image.
 */
/*---------------------------------------------------------------------------*/
void DIFT::correct_image(float *image, float * const fov_diameter)
 { unsigned long int rsq_max;
                          // multiply image with 1/sinc function and mask image
   rsq_max=image_plane_size/4;
   if (fov_diameter != NULL)
    *fov_diameter=sqrtf((float)rsq_max)*DeltaXY*2.0f;
   for (unsigned short int y=0; y < XYSamples; y++)
    { unsigned long int y2;

      y2=x2_table[y];
      for (unsigned short int x=0; x < XYSamples; x++)
       if (x2_table[x]+y2 <= rsq_max)
        *image++*=(float)(sinc_table[x]*sinc_table[y]);
        else *image++=0.0f;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Distribute array of polar-coordinates points into cartesian grid of
           frequency domain.
    \param[in]     p                  (RhoSamplesExpand+1) data points,
                                      representing complex FT of two
                                      projections
    \param[in,out] image_frq_slice1   two slices of frequency domain image
    \param[in]     indices            indices used in converting from polar to
                                      cartesian coordinates
    \param[in]     c                  weights used in converting from polar to
                                      cartesian coordinates

    Distribute array of polar-coordinates points into cartesian grid of
    frequency domain.
 */
/*---------------------------------------------------------------------------*/
void DIFT::distribute(const std::vector<float> p,
                      float * const image_frq_slice1,
                      const unsigned long int *indices, const float *c) const
 { float *image_frq_slice2;

   image_frq_slice2=image_frq_slice1+image_plane_size_padded;
   for (unsigned short int r=0; r <= RhoSamplesExpand; r++)
    { float real, imag;
        // distribute complex freq. domain value to 4 corners of cartesian grid
      real=p[2*r];
      imag=p[2*r+1];
      for (unsigned short int i=0; i < N_CORNERS; i++)
       { image_frq_slice1[*indices]+=*c*real;
         image_frq_slice2[*indices++]+=*c++*imag;
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Padding of projections, FFT, shift to center-of-rotation and put
           adjacent slices in the real and imaginary part of a complex slice.
    \param[in]  projection       projection data (RhoSamples values)
    \param[in]  fake_odd_slice   algorithm needs an even number of slices
    \return FT of projections ((RhoSamplesExpand+1) complex values)

    Padding of projections, FFT, shift to center-of-rotation and put adjacent
    slices in the real and imaginary part of a complex slice.
 */
/*---------------------------------------------------------------------------*/
 std::vector <float> DIFT::dift_pad_ft(const float * const projection,
                                       const bool fake_odd_slice) const
 { float *pb1r, *pb1i;
   std::vector <float> p, buf_even, buf_odd;

   p.resize(2*RhoSamplesExpand+2);
   buf_even.resize(RhoSamplesExpand+2);
                            // pad, FFT and shift of projection from even slice
   dift_pft(projection, &buf_even[0], cor_array);
   if (fake_odd_slice)                                // no odd slice available
    { float *pom;
      unsigned short int x;
                                                   // output positive frequency
      memcpy(&p[0], &buf_even[0], buf_even.size()*sizeof(float));
                                                   // output negative frequency
      for (pb1r=&buf_even[2],
           pb1i=&buf_even[3],
           pom=&p[2*RhoSamplesExpand+1],
           x=1; x <= RhoSamplesExpand/2; x++,
           pb1r+=2,
           pb1i+=2) { *pom--=-*pb1i;
                      *pom--=*pb1r;
                    }
    }
    else  { float *pb2r, *pb2i, *pop, *pom;
            unsigned short int x;

            buf_odd.resize(RhoSamplesExpand+2);
                             // pad, FFT and shift of projection from odd slice
            dift_pft(projection+sino_plane_size, &buf_odd[0], cor_array);
                                                  // combine FTs of projections
            p[0]=buf_even[0]-buf_odd[1];
            p[1]=buf_even[1]+buf_odd[0];
            for (pb1r=&buf_even[2],
                 pb1i=&buf_even[3],
                 pb2r=&buf_odd[2],
                 pb2i=&buf_odd[3],
                 pop=&p[2],
                 pom=&p[2*RhoSamplesExpand+1],
                 x=1; x <= RhoSamplesExpand/2; x++,
                 pb1r+=2,
                 pb1i+=2,
                 pb2r+=2,
                 pb2i+=2)
             { *pop++=*pb1r-*pb2i;
               *pop++=*pb2r+*pb1i;
               *pom--=*pb2r-*pb1i;
               *pom--=*pb1r+*pb2i;
             }
          }
   return(p);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Padding and FFT of a projection and shifting of FFT.
    \param[in]  projection   data of projection
    \param[out] buffer       resulting FT of projection
    \param[in]  cor_array    lookup-table for complex exp() function

    Padding and FFT of a projection and shifting of FFT.
 */
/*---------------------------------------------------------------------------*/
void DIFT::dift_pft(const float * const projection, float *buffer,
                    const std::vector <float> cor_array) const
 { memcpy(buffer, projection, RhoSamples*sizeof(float));
                                                       // padding of projection
   memset(buffer+RhoSamples, 0, (RhoSamplesExpand-RhoSamples)*sizeof(float));
   fft_rho_expand->rFFT_1D(buffer, true);                  // FFT of projection
   buffer[RhoSamplesExpand]=buffer[1];
   buffer[RhoSamplesExpand+1]=0.0f;
   buffer[1]=0.0f;
                                         // shift projection in frequency space
   for (unsigned short int x=0; x <= RhoSamplesExpand/2; x++)
    { float ar, ai, br, bi;

      ar=buffer[2*x];
      ai=buffer[2*x+1];
      br=cor_array[2*x];
      bi=cor_array[2*x+1];
      buffer[2*x]=ar*br-ai*bi;
      buffer[2*x+1]=ar*bi+ai*br;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Find the four Cartesian coordinates points (u,v) surrounding a polar
           point (k,m), and also the coefficients C(u,v;k,m) defined by
           equation (21) in the report.
    \param[out] c         table of coefficients subscripted by
                          [view angle][radius][corner number]
    \param[out] indices   table of indices, for a given radius and angle in
                          frequency space, identify the lexigraphic indices
                          2*(x+N*y) of the four complex-number corners

    Find the four Cartesian coordinates points (u,v) surrounding a polar
    point (k,m), and also the coefficients C(u,v;k,m) defined by equation (21)
    in the report.
 */
/*---------------------------------------------------------------------------*/
void DIFT::dift_pol_cartes(std::vector <float> * const c,
                         std::vector <unsigned long int> * const indices) const
 { unsigned short int mask_n;
   unsigned long int *indptr;
   float DeltaTheta, *cptr;
   std::vector <float> rho, norm;
                                                       // set up the vector rho
   rho.resize(RhoSamplesExpand+2);
   { unsigned short int r, r_retr;

     rho[0]=0.0f;
     for (r_retr=RhoSamplesExpand,
          r=1; r <= RhoSamplesExpand/2; r++,
          r_retr--)
      { rho[r]=(float)r/(float)EXPAND;
        rho[r_retr]=-rho[r];
      }
   }
                                                 // create normalization matrix
   norm.assign((unsigned long int)RhoSamples*(unsigned long int)RhoSamples*2,
               0.0f);
   mask_n=RhoSamples-1;
                       // compute the unnormalized coefficients and the indices
   indptr=&(*indices)[0];
   cptr=&(*c)[0];
   DeltaTheta=M_PI/(float)ThetaSamples;
   for (unsigned short int t=0; t < ThetaSamples; t++)
    { float *rptr;
      unsigned short int r;
      double cos_phi, sin_phi;
      { double theta;

        theta=DeltaTheta*t;
        cos_phi=cos(theta);
        sin_phi=sin(theta);
      }
      if (t == ThetaSamples-1) DeltaTheta*=0.5f;
      for (rptr=&rho[0],
           r=0; r <= RhoSamplesExpand; r++,
           rptr++)
       { signed short int ixw, ixe, iys, iyn;
         float val, value, val2, wxw, wxe, wys, wyn;
         double x, y, xe, yn;
         signed long int ri;
               // Locate the Cartesian grid points around each polar one and
               // compute the weights of each point.  The indices are converted
               // to non-negative with a bit-by-bit logical AND.
         x=*rptr*cos_phi;
         y=*rptr*sin_phi;
         xe=ceil(x);
         yn=ceil(y);
         wxw=xe-x;
         wys=yn-y;
         wxe=1.0f-wxw;
         wyn=1.0f-wys;
         ixe=(signed long int)xe;
         ixw=(ixe-1) & mask_n;
         ixe=ixe & mask_n;
         iyn=(signed long int)yn;
         iys=(iyn-1) & mask_n;
         iyn=iyn & mask_n;
         if (r == 0) val=0.25f/(float)EXPAND*DeltaTheta;
          else val=fabsf(*rptr)*DeltaTheta;
                    // store the unnormalized coefficients for the four corners
         val2=val*wys;
         value=val2*wxw;                                           // SW corner
         ri=(signed long int)RhoSamples*iys;
         *indptr=ri+ixw;
         if (value >= SMALL_VALUE) { norm[*indptr]+=value;
                                     *cptr++=value;
                                   }
          else *cptr++=0.0f;
         indptr++;
         value=val2*wxe;                                           // SE corner
         *indptr=ri+ixe;
         if (value >= SMALL_VALUE) { norm[*indptr]+=value;
                                     *cptr++=value;
                                   }
          else *cptr++=0.0f;
         indptr++;
         val2=val*wyn;
         value=val2*wxw;                                           // NW corner
         ri=(signed long int)RhoSamples*iyn;
         *indptr=ri+ixw;
         if (value >= SMALL_VALUE) { norm[*indptr]+=value;
                                     *cptr++=value;
                                   }
          else *cptr++=0.0f;
         indptr++;
         value=val2*wxe;                                           // NE corner
         *indptr=ri+ixe;
         if (value >= SMALL_VALUE) { norm[*indptr]+=value;
                                     *cptr++=value;
                                   }
          else *cptr++=0.0f;
         indptr++;
       }
    }
                                                  // normalize the coefficients
   for (unsigned long int i=0;
        i < (unsigned long int)ThetaSamples*N_CORNERS*
            (unsigned long int)(RhoSamplesExpand+1);
        i++)
    if ((*c)[i] > 0.0f) (*c)[i]/=norm[(*indices)[i]];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create tables for distortion correction and masking.

    Create \f$\frac{1}{sinc^2(x)}\f$-table used to correct for the distortions
    of the polar to cartesian coordinate transformation and \f$x^2\f$-table
    used to mask the image.
 */
/*---------------------------------------------------------------------------*/
void DIFT::init_sinc_table()
 { double coefficient;
   unsigned short int idx, idx_retr;
   unsigned long int x;

   sinc_table.resize(XYSamples);
   x2_table.resize(XYSamples);
                                   // initialize tables for 1/sinc^2(x) and x^2
   coefficient=M_PI/(double)XYSamples_padded;
   sinc_table[0]=1.0f;                                         // 1/sinc^2 at 0
   x2_table[0]=0;
   for (idx=1,
        idx_retr=XYSamples-1,
        x=1; x <= (unsigned long int)XYSamples/2; x++,
        idx_retr--,
        idx++)
    { double arg;

      arg=coefficient*(double)x;
      arg/=sin(arg);
      sinc_table[idx]=arg*arg;
      sinc_table[idx_retr]=sinc_table[idx];
      x2_table[idx]=x*x;
      x2_table[idx_retr]=x2_table[idx];
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create table for complex exp-function, used for shifting of
           projections in frequency domain.
    \return cor_array   complex exponentials

    Create table for complex exp-function, used for shifting of projections in
    frequency domain.
 */
/*---------------------------------------------------------------------------*/
 std::vector <float> DIFT::init_shift_table() const
 { double coeff;
   std::vector <float> cor_array;

   cor_array.clear();
   coeff=-M_PI/(double)EXPAND;
   for (unsigned short int i=0; i <= RhoSamplesExpand/2; i++)
    { double arg;

      arg=coeff*(double)i;
      cor_array.push_back(cosf((float)arg));
      cor_array.push_back(sinf((float)arg));
    }
   return(cor_array);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform a multi-threaded DIFT reconstruction.
    \param[in]  sinogram       sinogram data
    \param[in]  slices         number of sinogram slices
    \param[in]  num            matrix number
    \param[out] fov_diameter   diameter of FOV after reconstruction
    \param[in]  max_threads    maximum number of threads to use
    \return reconstructed image

    Perform a multi-threaded DIFT reconstruction.
 */
/*---------------------------------------------------------------------------*/
float *DIFT::reconstruct(float *sinogram, unsigned short int slices,
                         const unsigned short int num,
                         float * const fov_diameter,
                         const unsigned short int max_threads)
 { try
   { float *image=NULL, *sino_pad=NULL;
     std::vector <tthread_id> tid;
     std::vector <bool> thread_running;
     void *thread_result;
     unsigned short int threads=0, t;

     try
     { Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2,
                                "DIFT reconstruction (frame #1)")->arg(num);
                            // number of bins in projections must be power of 2
       if (RhoSamples != origRhoSamples)
        { float *sp, *osp;

          Logging::flog()->logMsg("padding of sinogram from #1x#2x#3 to "
                                  "#4x#5x#6", loglevel)->arg(origRhoSamples)->
           arg(ThetaSamples)->arg(slices)->arg(RhoSamples)->arg(ThetaSamples)->
           arg(slices);
          sino_pad=new float[sino_plane_size*(unsigned long int)slices];
          memset(sino_pad, 0,
                 sino_plane_size*(unsigned long int)slices*sizeof(float));
          sp=sino_pad+(RhoSamples-origRhoSamples)/2;
          osp=sinogram;
          for (unsigned long int pt=0;
               pt < (unsigned long int)ThetaSamples*(unsigned long int)slices;
               pt++,
               sp+=RhoSamples,
               osp+=origRhoSamples)
           memcpy(sp, osp, origRhoSamples*sizeof(float));
          sinogram=sino_pad;
        }
       threads=std::min(max_threads, slices);
                                       // create buffer for reconstructed image
       image=new float[image_plane_size*(unsigned long int)slices];
                                                           // single threaded ?
       if (threads == 1)
        { calc_dift(sinogram, image, slices, fov_diameter, max_threads, 1,
                    false);
          return(image);
        }
       float *sp, *ip;
       unsigned short int i;
       std::vector <tthread_params> tp;

       tid.resize(threads);
       tp.resize(threads);
       thread_running.assign(threads, false);
       for (sp=sinogram,
            ip=image,
            t=threads,
            i=0; i < threads; i++,// distribute sinogram onto different threads
            t--)
        { tp[i].object=this;
          tp[i].sinogram=sp;
          tp[i].image=ip;
          tp[i].slices=slices/t;
          if (i == 0) tp[i].fov_diameter=fov_diameter;
           else tp[i].fov_diameter=NULL;
          tp[i].thread_number=i;
          tp[i].max_threads=max_threads;
                 // number of slices per thread must be even except last thread
          if ((tp[i].slices & 0x1) != 0 && (t > 1)) tp[i].slices++;
          thread_running[i]=true;
                                                                // start thread
          Logging::flog()->logMsg("start thread #1 to reconstruct #2 planes",
                                  loglevel)->arg(i+1)->arg(tp[i].slices);
          ThreadCreate(&tid[i], &executeThread_DIFT, &tp[i]);
          slices-=tp[i].slices;
          sp+=(unsigned long int)tp[i].slices*sino_plane_size;
          ip+=(unsigned long int)tp[i].slices*image_plane_size;
        }
       for (i=0; i < threads; i++)       // wait until all threads are finished
        { ThreadJoin(tid[i], &thread_result);
          thread_running[i]=false;
          Logging::flog()->logMsg("thread #1 finished", loglevel)->arg(i+1);
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
       if (sino_pad != NULL) { delete[] sino_pad;
                               sino_pad=NULL;
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
        if (image != NULL) delete[] image;
        if (sino_pad != NULL) { delete[] sino_pad;
                                sino_pad=NULL;
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
/*! \brief Shift image to new center.
    \param[in]  image_buffer   image_data
    \param[out] image          shifted image data

    Shift image to new center ((0,0) -> (n/2, n/2)).
 */
/*---------------------------------------------------------------------------*/
void DIFT::shift_image(const float * const image_buffer,
                       float * const image) const
 { unsigned short int size, y;
   unsigned long int offs1, offs2, offs3, offs4;

   size=XYSamples/2*sizeof(float);
   for (offs1=0,
        offs2=XYSamples/2,
        offs3=image_plane_size/2,
        offs4=image_plane_size/2+XYSamples/2,
        y=0; y < XYSamples/2; y++,
        offs4+=XYSamples,
        offs3+=XYSamples,
        offs2+=XYSamples,
        offs1+=XYSamples)
    { memcpy(image+offs1, image_buffer+offs4, size);
      memcpy(image+offs2, image_buffer+offs3, size);
      memcpy(image+offs3, image_buffer+offs2, size);
      memcpy(image+offs4, image_buffer+offs1, size);
    }
 }
