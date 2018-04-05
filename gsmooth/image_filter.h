//** file: filter.h
//** date: 2003/01/14

//** author: Frank Kehren
//** © CPS Innovations

//** This class implements different filter for frequency and image space.

/*- usage examples ------------------------------------------------------------

 tfilter f;
                                                  // filtering in x/y-direction
 f.name=GAUSS;
 f.restype=FWHM;
 f.resolution=filter_xy;                          // filter-width in mm FWHM
 filter=new Filter(XYSamples, ZSamples, f, DeltaXY, BinSizeRho/DeltaXY, true);
 filter->calcFilter(image);
 delete filter;

-----------------------------------------------------------------------------*/
/*
   Modification history: Merence Sibomana for HRRT Users Community
        25-may-2010: Use SIMD for gaussian smoothing 
*/

# pragma once

#include "fft.h"

/*- type declarations -------------------------------------------------------*/
                                                                // filter names
typedef enum { ALL_PASS,                                     // all-pass filter
               BOXCAR,                                         // boxcar filter
               BUTTERWORTH,                               // butterworth filter
               GAUSS,                                           // gauss filter
               HAMMING,                                       // hamming filter
               HANN,                                             // hann filter
               MEDIAN,                                         // median filter
               PARZEN,                                         // parzen filter
               SHEPP_LOGAN                                // shepp-logan filter
             } tfilter_name;
                                                            // resolution types
typedef enum { NYQUIST,                                // resolution in Nyquist
               FWHM                                       // resolution in FWHM
             } trestype;
                                                           // filter parameters
typedef struct { tfilter_name name;                           // name of filter
                                                 // order of butterworth filter
                 unsigned short int butterworth_order;
                 float hamming_alpha;         // alpha value for hamming filter
                 trestype restype;                           // resolution type
                 float resolution;                      // resolution of filter
               } tfilter;

/*- definitions -------------------------------------------------------------*/

class ImageFilter
 { protected:
    typedef enum { SPACE,                             // filter in space domain
                   FREQUENCY                      // filter in frequency domain
                 } tdomain;
    unsigned short int xy_samples,                     // width/height of image
                       xy_samples_padded,// width/height of image after padding
                       z_samples,                             // depth of image
                       gauss_kernel_width;      // width of gauss filter kernel
    unsigned long int image_slicesize,                   // size of image slice
                      padded_size;                // size of padded image slice
    bool xy_dir;        // filtering in x/y-direction ? (otherwise z-direction)
    tdomain domain;                                            // filter domain
    float pixelsize,                           // pixelsize in filter-direction
          *filter_array,                   // buffer for frequency space filter
          *fft_buffer,                                        // buffer for FFT
          *gauss_kernel;                          // kernel for gauss filtering
    tfilter filter;                                        // filter parameters
#ifdef GSMOOTH
    float *gauss_kernel_ps;
#else
    FFT *fft;                              // object for fast fourier transform
    void init_frequency_domain(const float);
    float fwhm_to_nyquist_sub(const float, const unsigned short int) const;
    void filterZ_freq(float * const) const;            // filter in z-direction
    void filterXY_freq(float * const, const unsigned short int) const;
 #endif
                                                               // boxcar filter
    void boxcar_space(float * const, const unsigned long int,
                      const unsigned short int) const;
                                                     // filter in x/y-direction
    void filterXY_space(float * const) const;
    void filterZ_space(float * const) const;
                           // calculate filter value of frequency domain filter
    float filter_value(float) const;
                         // calculate fwhm of filter specified in nyquist units
                                 // transform fwhm of filter into nyquist units
    void fwhm_to_nyquist(const float, const unsigned short int);
                                                                // gauss filter
    void gauss_space(float * const, const unsigned long int,
                     const unsigned short int) const;
                                    // init tables for frequency domain filters
    void init_space_domain();           // init tables for space domain filters
                                                               // median filter
    void median_space(float * const, const unsigned long int,
                      const unsigned short int) const;
   public:
                                                    // initialize filter object
    ImageFilter(const unsigned short int, const unsigned short int, const tfilter,
           const float, const float, const bool, const unsigned short int);
    ~ImageFilter();                                         // destroy filter object
                                                    // perform filter operation
    void calcFilter(float * const, const unsigned short int) const;
#ifdef GSMOOTH
                                                     // filter in x/y-direction
    void filterXY_space_ps(float * const) const;
    void filterZ_space_ps(float * const) const;
    void gauss_space_ps(float *const,  int, const unsigned long int,
                     const unsigned short int) const;
    void calcFilter_ps(float * const) const;
#endif
 };
