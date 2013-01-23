/*! \file fore_fft.h
    \brief This class implements 2D-FFT and 2D-iFFT of a sinogram plane which
           covers 180 or 360 degrees.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/01/20 added Doxygen style comments
 */

#ifndef _FORE_FFT_H
#define _FORE_FFT_H

#include "fft.h"

/*- class definitions -------------------------------------------------------*/

class FORE_FFT
 { private:
    FFT *fft_rho,      /*!< object for fourier transform in radial direction */
        *fft_theta;   /*!< object for fourier transform in angular direction */
    unsigned short int RhoSamples,         /*!< number of bins in projection */
                       ThetaSamples;       /*!< number of angles in sinogram */
   public:
    FORE_FFT(const unsigned short int, const unsigned short int);// constructor
    ~FORE_FFT();                                              // destroy object
                                            // 2d-FFT of sinogram (360 degrees)
    void sino_fft(float * const, float * const, const bool,
                  const unsigned short int) const;
                                            // 2d-FFT of sinogram (180 degrees)
    void sino_fft_2D(float * const, float * const, const bool,
                     const unsigned short int) const;
 };

#endif
