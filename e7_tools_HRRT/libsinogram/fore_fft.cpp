/*! \class FORE_FFT fore_fft.h "fore_fft.h"
    \brief This class implements 2D-FFT and 2D-iFFT of a sinogram plane which
           covers 180 or 360 degrees.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/01/20 added Doxygen style comments

    Sinograms from segment 0 cover only 180 degress. In this case the sinogram
    is extended by its rho-reversed image to 360 degress. The resulting FT
    contains symmetries that are not stored. Sinograms from oblique segments
    cover already 360 degress (180 degrees from segment +s and 180 degrees from
    segment -s).
 */

#include <iostream>
#include <cstring>
#include "fastmath.h"
#include "fore_fft.h"
#include "fft.h"

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _RhoSamples     number of bins in projections
    \param[in] _ThetaSamples   number of angles in sinogram

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
FORE_FFT::FORE_FFT(const unsigned short int _RhoSamples,
                   const unsigned short int _ThetaSamples)
 { fft_rho=NULL;
   fft_theta=NULL;
   try
   { RhoSamples=_RhoSamples;
     ThetaSamples=_ThetaSamples;
                             // create tables for real-valued fourier transform
     fft_rho=new FFT(RhoSamples);
                                 // create tables for complex fourier transform
     fft_theta=new FFT(ThetaSamples*2);
   }
   catch (...)
    { if (fft_theta != NULL) { delete fft_theta;
                               fft_theta=NULL;
                             }
      if (fft_rho != NULL) { delete fft_rho;
                             fft_rho=NULL;
                           }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
FORE_FFT::~FORE_FFT()
 { if (fft_theta != NULL) delete fft_theta;
   if (fft_rho != NULL) delete fft_rho;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculates the forward or inverse FFT of a real-valued 360 degrees
           sinogram.
    \param[in] real_data     real-valued data
                             (RhoSamples*ThetaSamples*2 real values)
    \param[in] ft_data       FT of real-valued data
                             ((RhoSamples/2+1)*ThetaSamples*2 complex values)
    \param[in] forward       true=FFT, false=iFFT
    \param[in] max_threads   maximum number of threads to use

    Calculates the forward or inverse FFT of a real-valued sinogram. The
    sinogram is assumed to cover a full 360 degrees (ThetaSamples*2) and may be
    either a direct or oblique sinogram.
    NOTE that the input array is used as "scratch" during the operation and
    will be altered upon return.
 */
/*---------------------------------------------------------------------------*/
void FORE_FFT::sino_fft(float * const real_data, float * const ft_data,
                        const bool forward,
                        const unsigned short int max_threads) const
 { unsigned short int i;
   float *real_ptr, *ft_ptr;

   if (forward)
    {                                        // 1D-FFT in the x-direction (rho)
      fft_rho->rFFT_1D_block(real_data, true, ThetaSamples*2, max_threads);
      for (ft_ptr=ft_data,
           i=0; i < ThetaSamples*2; i++,
           ft_ptr+=RhoSamples+2)
       { memcpy(ft_ptr, &real_data[i*RhoSamples], RhoSamples*sizeof(float));
         ft_ptr[RhoSamples]=ft_ptr[1];
         ft_ptr[RhoSamples+1]=0.0f;
         ft_ptr[1]=0.0f;
       }
                                   // complex 1D-FFT in the y-direction (theta)
      fft_theta->FFT_1D_block(ft_data, ft_data+1, true, RhoSamples/2+1, 2,
                              RhoSamples+2, max_threads);
    }
    else {                 // inverse complex 1D-FFT in the y-direction (theta)
           fft_theta->FFT_1D_block(ft_data, ft_data+1, false, RhoSamples/2+1,
                                   2, RhoSamples+2, max_threads);
                                     // inverse 1D-FFT in the x-direction (rho)
           for (ft_ptr=ft_data,
                real_ptr=real_data,
                i=0; i < ThetaSamples*2; i++,
                ft_ptr+=RhoSamples+2,
                real_ptr+=RhoSamples)
            { memcpy(real_ptr, ft_ptr, RhoSamples*sizeof(float));
              real_ptr[1]=ft_ptr[RhoSamples];
            }
           fft_rho->rFFT_1D_block(real_data, false, ThetaSamples*2,
                                  max_threads);
         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculates the forward or inverse FFT of a real-valued 180 degrees
           sinogram.
    \param[in] real_data     real-valued data
                             (RhoSamples*ThetaSamples real values)
    \param[in] ft_data       FT of real-valued data
                             ((RhoSamples/2+1)*(ThetaSamples+1) complex values)
    \param[in] forward       true=FFT, false=iFFT
    \param[in] max_threads   maximum number of threads to use

    Calculates the forward or inverse FFT of a real-valued sinogram. The
    sinogram is assumed to be a "direct" sinogram and to cover only 180 degrees
    (ThetaSamples). The FT of the sinogram must cover a full 360 degress
    (ThetaSamples*2). The second "half" of the sinogram is the rho-reversed
    copy of the first half. Due to this "reverse" part the FT has a symmetry
    and only one half of it +1 (ThetaSamples+1) values need to be stored.
    NOTE that the input array is used as "scratch" during the operation and
    will be altered upon return.
 */
/*---------------------------------------------------------------------------*/
void FORE_FFT::sino_fft_2D(float * const real_data, float * const ft_data,
                           const bool forward,
                           const unsigned short int max_threads) const
 { float *real=NULL, *imag=NULL;

   try
   { unsigned short int i, j;
     float *real_ptr, *ft_ptr, *float_ptr;
     unsigned long int iptr;

     if (forward)
      {                                      // 1D-FFT in the x-direction (rho)
        fft_rho->rFFT_1D_block(real_data, true, ThetaSamples, max_threads);
        for (ft_ptr=ft_data,
             i=0; i < ThetaSamples; i++,
             ft_ptr+=RhoSamples+2)
         { memcpy(ft_ptr, &real_data[i*RhoSamples], RhoSamples*sizeof(float));
           ft_ptr[RhoSamples]=ft_ptr[1];
           ft_ptr[RhoSamples+1]=0.0f;
           ft_ptr[1]=0.0f;
         }
                                   // complex 1D-FFT in the y-direction (theta)
        real=new float[(unsigned long int)ThetaSamples*
                       (unsigned long int)(RhoSamples+2)];
        imag=new float[(unsigned long int)ThetaSamples*
                       (unsigned long int)(RhoSamples+2)];
                                                       // copy data into buffer
        for (iptr=0,
             i=0; i < RhoSamples+2; i+=2,
             iptr+=2*ThetaSamples)
         for (float_ptr=ft_data,
              j=0; j < ThetaSamples; j++,
              float_ptr+=RhoSamples+2)
          { float rv, iv;

            rv=float_ptr[i];
            iv=float_ptr[i+1];
            real[iptr+j]=rv;
            imag[iptr+j]=iv;
                                                    // create conjugate complex
            real[iptr+ThetaSamples+j]=rv;
            imag[iptr+ThetaSamples+j]=-iv;
          }
                                                                         // FFT
        fft_theta->FFT_1D_block(real, imag, forward, RhoSamples/2+1,
                                ThetaSamples*2, 1, max_threads);
                                      // copy one half+1 values of the FTs back
        for (iptr=0,
             i=0; i < RhoSamples+2; i+=2,
             iptr+=2*ThetaSamples)
         for (float_ptr=ft_data,
              j=0; j < ThetaSamples+1; j++,
              float_ptr+=RhoSamples+2)
          { float_ptr[i]=real[iptr+j];
            float_ptr[i+1]=imag[iptr+j];
          }
        delete[] imag;
        imag=NULL;
        delete[] real;
        real=NULL;
      }
      else {               // inverse complex 1D-FFT in the y-direction (theta)
             real=new float[(unsigned long int)ThetaSamples*
                            (unsigned long int)(RhoSamples+2)];
             imag=new float[(unsigned long int)ThetaSamples*
                            (unsigned long int)(RhoSamples+2)];
             for (iptr=0,
                  i=0; i < RhoSamples+2; i+=2,
                  iptr+=2*ThetaSamples)
              {                                        // copy data into buffer
                real[iptr]=ft_data[i];
                imag[iptr]=ft_data[i+1];
                float_ptr=&ft_data[ThetaSamples*(RhoSamples+2)+i];
                real[iptr+ThetaSamples]=float_ptr[0];
                imag[iptr+ThetaSamples]=float_ptr[1];
                for (float_ptr=ft_data+RhoSamples+2,
                     j=1; j < ThetaSamples-1; j+=2,
                     float_ptr+=2*(RhoSamples+2))
                 { float rv1, rv2, iv1, iv2;

                   rv1=float_ptr[i];
                   iv1=float_ptr[i+1];
                   rv2=float_ptr[RhoSamples+2+i];
                   iv2=float_ptr[RhoSamples+2+i+1];
                   real[iptr+j]=rv1;
                   real[iptr+j+1]=rv2;
                   imag[iptr+j]=iv1;
                   imag[iptr+j+1]=iv2;
                                         // create the symmetric part of the FT
                   real[iptr+2*ThetaSamples-j]=-rv1;
                   imag[iptr+2*ThetaSamples-j]=iv1;
                   real[iptr+2*ThetaSamples-j-1]=rv2;
                   imag[iptr+2*ThetaSamples-j-1]=-iv2;
                 }
                float_ptr=ft_data+(unsigned long int)(RhoSamples+2)*
                             (unsigned long int)(ThetaSamples-1);
                { float rv1, rv2, iv1, iv2;

                  rv1=float_ptr[i];
                  iv1=float_ptr[i+1];
                  rv2=float_ptr[RhoSamples+2+i];
                  iv2=float_ptr[RhoSamples+2+i+1];
                  real[iptr+ThetaSamples-1]=rv1;
                  real[iptr+ThetaSamples]=rv2;
                  real[iptr+ThetaSamples+1]=-rv1;
                  imag[iptr+ThetaSamples-1]=iv1;
                  imag[iptr+ThetaSamples]=iv2;
                  imag[iptr+ThetaSamples+1]=iv1;
                }
              }
                                                                         // FFT
             fft_theta->FFT_1D_block(real, imag, forward, RhoSamples/2+1,
                                     ThetaSamples*2, 1, max_threads);
                                                              // copy data back
             for (iptr=0,
                  i=0; i < RhoSamples+2; i+=2,
                  iptr+=2*ThetaSamples)
              for (float_ptr=ft_data,
                   j=0; j < ThetaSamples; j++,
                   float_ptr+=RhoSamples+2)
               { float_ptr[i]=real[iptr+j];
                 float_ptr[i+1]=imag[iptr+j];
               }
             delete[] imag;
             imag=NULL;
             delete[] real;
             real=NULL;
                                     // inverse 1D-FFT in the x-direction (rho)
             for (ft_ptr=ft_data,
                  real_ptr=real_data,
                  i=0; i < ThetaSamples; i++,
                  ft_ptr+=RhoSamples+2,
                  real_ptr+=RhoSamples)
              { memcpy(real_ptr, ft_ptr, RhoSamples*sizeof(float));
                real_ptr[1]=ft_ptr[RhoSamples];
              }
             fft_rho->rFFT_1D_block(real_data, false, ThetaSamples,
                                    max_threads);
           }
   }
   catch (...)
    { if (imag != NULL) delete[] imag;
      if (real != NULL) delete[] real;
      throw;
    }
 }
