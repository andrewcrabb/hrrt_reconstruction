/*! \file image_filter.h
    \brief This class implements different filter for frequency and image space.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/02/14 Altivec support
    \date 2005/01/20 added Doxygen style comments
 */

#pragma once

#include <vector>
#include "fft.h"

/*- type declarations -------------------------------------------------------*/
                                                            /*! filter names */
typedef enum { ALL_PASS,                                /*!< all-pass filter */
               BOXCAR,                                    /*!< boxcar filter */
               BUTTERWORTH,                          /*!< butterworth filter */
               GAUSS,                                      /*!< gauss filter */
               HAMMING,                                  /*!< hamming filter */
               HANN,                                        /*!< hann filter */
               MEDIAN,                                    /*!< median filter */
               PARZEN,                                    /*!< parzen filter */
               SHEPP_LOGAN                           /*!< shepp-logan filter */
             } tfilter_name;
                                                        /*! resolution types */
typedef enum { NYQUIST,                           /*!< resolution in Nyquist */
               FWHM                                  /*!< resolution in FWHM */
             } trestype;
                                                       /*! filter parameters */
typedef struct { tfilter_name name;                      /*!< name of filter */
                                             /*! order of butterworth filter */
                 unsigned short int butterworth_order;
                 float hamming_alpha;    /*!< alpha value for hamming filter */
                 trestype restype;                      /*!< resolution type */
                 float resolution;                 /*!< resolution of filter */
               } tfilter;

/*- definitions -------------------------------------------------------------*/

class Filter
 { protected:
                                                           /*! filter domain */
    typedef enum { SPACE,                        /*!< filter in space domain */
                   FREQUENCY                 /*!< filter in frequency domain */
                 } tdomain;
    unsigned short int xy_samples,                /*!< width/height of image */
                                     /*! width/height of image after padding */
                       xy_samples_padded,
                       z_samples;                        /*!< depth of image */
    unsigned long int image_slicesize,              /*!< size of image slice */
                      padded_size;           /*!< size of padded image slice */
    bool xy_dir;   /*!< filtering in x/y-direction ? (otherwise z-direction) */
    tdomain domain;                                       /*!< filter domain */
    float pixelsize,                      /*!< pixelsize in filter-direction */
          *filter_array,              /*!< buffer for frequency space filter */
          *fft_buffer;                                   /*!< buffer for FFT */
#ifdef __ALTIVEC__
    std::vector <float> gauss_kernel2d;      /*!< kernel for 2d gauss filter */
#endif
    std::vector <float> kernel1d;      /*!< filter kernel for 1d convolution */
    tfilter filter;                                   /*!< filter parameters */
    FFT *fft;                         /*!< object for fast fourier transform */
                                                     // filter in x/y-direction
    void filterXY_freq(float * const, const unsigned short int) const;
    void filterXY_space(float * const) const;
    void filterZ_freq(float * const) const;            // filter in z-direction
    void filterZ_space(float * const) const;
                           // calculate filter value of frequency domain filter
    float filter_value(float) const;
                         // calculate fwhm of filter specified in nyquist units
    float fwhm_to_nyquist_sub(const float, const unsigned short int) const;
                                 // transform fwhm of filter into nyquist units
    void fwhm_to_nyquist(const float, const unsigned short int);
                                    // init tables for frequency domain filters
    void init_frequency_domain(const float);
    void init_space_domain();           // init tables for space domain filters
                                                               // median filter
    void median_space(float * const, const unsigned long int,
                      const unsigned short int) const;
   public:
                                                    // initialize filter object
    Filter(const unsigned short int, const unsigned short int, const tfilter,
           const float, const float, const bool, const unsigned short int);
    ~Filter();                                         // destroy filter object
                                                    // perform filter operation
    void calcFilter(float * const, const unsigned short int) const;
 };

