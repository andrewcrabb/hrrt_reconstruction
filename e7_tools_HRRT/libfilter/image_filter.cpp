/*! \class Filter image_filter.h "image_filter.h"
    \brief This class implements different filters for frequency and image
           space.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/02/14 Altivec support
    \date 2005/01/20 added Doxygen style comments

    This class implements different filters for frequency and image space. The
    boxcar-, gauss- and median-filter are implemented in image space, while the
    Butterworth-, Hamming-, Hann-, Parzen- and Shepp-Logan-filters are in
    frequency space. This class provides 2d-filters for x/y-direction and
    1d-filters for z-direction. In case a filter is not separable (Butterworth,
    Shepp-Logan) a filtering in x/y- followed by a filtering in z-direction is
    not a correct 3d filter.
 */

#include <iostream>
#include <limits>
#include <algorithm>
#include <string.h>
#ifdef __ALTIVEC__
#include "vecLib/vecLib.h"
#include "Accelerate/Accelerate.h"
#endif
#include "image_filter.h"
#include "e7_tools_const.h"
#include "fastmath.h"
#include "fft.h"
#include "vecmath.h"

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] ixy_samples           width/height of image
    \param[in] iz_samples            depth of image
    \param[in] ifilter               filter parameters
    \param[in] ipixelsize            pixelsize of image in filter direction
    \param[in] nyquist_pixelratio    ratio between scanner bin size and pixel
                                     size
    \param[in] ixy_dir               filter in x/y-direction ?
                                     (otherwise z-direction)
    \param[in] max_threads           maximum number of threads to use

    Initialize object. If the filter size is specified in Nyquist units for
    an image space filter, the size is converted into FWHM by:
    \f[
        ifilter.resolution=\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{\frac{\mbox{ipixelsize}}{\mbox{nyquist\_pixelratio}}}
              {ifilter.resolution} & ifilter.resolution < 0.5\\
         1.0 & otherwise
        \end{array}\right.
    \f]
    and additionaly for gaussian smoothing:
    \f[
        ifilter.resolution=ifilter.resolution\frac{4\log(2)}{\Pi}
    \f]
    If the filter size is specified in FWHM for a frequency space filter, the
    size is converted into Nyquits units.
 */
/*---------------------------------------------------------------------------*/
Filter::Filter(const unsigned short int ixy_samples,
               const unsigned short int iz_samples,
               const tfilter ifilter, const float ipixelsize,
               const float nyquist_pixelratio, const bool ixy_dir,
               const unsigned short int max_threads)
 { xy_samples=ixy_samples;
   z_samples=iz_samples;
   xy_dir=ixy_dir;
   filter=ifilter;
   pixelsize=ipixelsize;
   image_slicesize=(unsigned long int)xy_samples*(unsigned long int)xy_samples;
   filter_array=NULL;
   fft_buffer=NULL;
   fft=NULL;
                                                           // initialize filter
   switch (filter.name)
    { case ALL_PASS:
       return;
      case BOXCAR:
      case GAUSS:
      case MEDIAN:
       domain=SPACE;
       if (filter.restype == NYQUIST)
        { float binsize;
                                                   // convert Nyquist into FWHM
          binsize=pixelsize/nyquist_pixelratio;
          if (filter.resolution >= 0.5f) filter.resolution=1.0f;
           else filter.resolution=binsize/filter.resolution;
          if (filter.name == GAUSS) filter.resolution*=4.0f*logf(2.0f)/M_PI;
          filter.restype=FWHM;
        }
       init_space_domain();
       break;
      case BUTTERWORTH:
      case HAMMING:
      case HANN:
      case PARZEN:
      case SHEPP_LOGAN:
       domain=FREQUENCY;
       if (filter.restype == FWHM)                 // convert FWHM into Nyquist
        { fwhm_to_nyquist(pixelsize/nyquist_pixelratio, max_threads);
          filter.restype=NYQUIST;
        }
       init_frequency_domain(nyquist_pixelratio);
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
Filter::~Filter()
 { if (fft_buffer != NULL) delete[] fft_buffer;
   if (fft != NULL) delete fft;
   if (filter_array != NULL) delete[] filter_array;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform filter operation.
    \param[in] image         image data
    \param[in] max_threads   maximum number of threads to use

    Perform filter operation in x/y- or z-direction.
 */
/*---------------------------------------------------------------------------*/
void Filter::calcFilter(float * const image,
                        const unsigned short int max_threads) const
 { if (filter.name == ALL_PASS) return;
   switch (domain)
    { case SPACE:
       if (xy_dir) filterXY_space(image);
        else filterZ_space(image);
       break;
      case FREQUENCY:
       if (xy_dir) filterXY_freq(image, max_threads);
        else filterZ_freq(image);
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate filter value for specified frequency.
    \param[in] freq   frequency
    \return filter value

    Calculate filter the value for the specified frequency. The Butterworth
    filter is calculated with:
    \f[
        v=\left\{
        \begin{array}{r@{\quad:\quad}l}
        1 & |\mbox{freq}| <
            \mbox{resolution}\cdot{10^{-6}}^\frac{1}{2\cdot\mbox{order}}\\
        0 & |\mbox{freq}| >
            \frac{\mbox{resolution}}{{10^{-6}}^\frac{1}{2\cdot\mbox{order}}}\\
        \frac{1}{1+\left(\frac{|\mbox{freq}|}
                        {\mbox{resolution}}\right)^{2\cdot\mbox{order}}} &
            otherwise
        \end{array}\right.
    \f]
    The Hamming filter uses this equation:
    \f[
        v=\left\{
        \begin{array}{r@{\quad:\quad}l}
        0 & \mbox{freq} > \mbox{resolution}\\
        \alpha+(1-\alpha)
        \cos\left(\frac{\Pi\cdot\mbox{freq}}{\mbox{resolution}}\right) &
        otherwise
        \end{array}\right.
    \f]
    The Hann filter uses:
    \f[
        v=\left\{
        \begin{array}{r@{\quad:\quad}l}
        0 & \mbox{freq} > \mbox{resolution}\\
        \frac{1}{2}+\frac{1}{2}
        \cos\left(\frac{\Pi\cdot\mbox{freq}}{\mbox{resolution}}\right) &
        otherwise
        \end{array}\right.
    \f]
    The Parzen filter is calculated by:
    \f[
       v=\left\{
       \begin{array}{r@{\quad:\quad}l}
       0 & |\mbox{freq}| > \mbox{resolution}\\
       1-6\cdot\frac{freq}{resolution}^2\cdot(1-\frac{freq}{resolution})
          & |\mbox{freq}| \le \frac{\mbox{resolution}}{2}\\
       2(1-\frac{freq}{resolution})^3 & otherwise
       \end{array}\right.
    \f]
    and the Shepp-Logan filter by:
    \f[
        v=\left\{
        \begin{array}{r@{\quad:\quad}l}
         \frac{\sin(10^{-5})}{10^{-5}} &
          |\frac{\Pi}{2}\frac{freq}{resolution}| < 10^{-5}\\
           \frac{\sin(|\frac{\Pi}{2}\frac{freq}{resolution}|)}
                {|\frac{\Pi}{2}\frac{freq}{resolution}|} & otherwise
        \end{array}\right.
    \f]
 */
/*---------------------------------------------------------------------------*/
float Filter::filter_value(float freq) const
 { switch (filter.name)
   { case BUTTERWORTH:                                    // butterworth filter
       { float eps_root;

         freq=fabsf(freq);
         eps_root=powf(1.0E-6f, 1.0f/(2.0f*filter.butterworth_order));
         if (freq < filter.resolution*eps_root) return(1.0f);
         if (freq > filter.resolution/eps_root) return(0.0f);
       }
       return(1.0f/(1.0f+powf(freq/filter.resolution,
                              2.0f*filter.butterworth_order)));
      case HAMMING:                                           // hamming filter
       if (freq > filter.resolution) return(0.0f);
       return(filter.hamming_alpha+
              (1.0f-filter.hamming_alpha)*cosf(M_PI*freq/filter.resolution));
      case HANN:                                                 // hann filter
       if (freq > filter.resolution) return(0.0f);
       return(0.5f+0.5f*cosf(M_PI*freq/filter.resolution));
      case PARZEN:                                             // parzen filter
       freq=fabsf(freq);
       if (freq > filter.resolution) return(0.0f);
       { float arg, f;

         arg=freq/filter.resolution;
         f=1.0f-arg;
         if (freq <= 0.5f*filter.resolution) return(1.0f-6.0f*arg*arg*f);
         return(2.0f*f*f*f);
       }
      case SHEPP_LOGAN:                                   // shepp-logan filter
       { float arg;

         arg=fabsf(M_PI*0.5f*freq/filter.resolution);
         if (arg < 1.0E-5f) arg=1.0E-5f;
         return(sinf(arg)/arg);
       }
      default:
       return(1.0f);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Filter in x- and y-direction (frequency domain).
    \param[in] image         image data
    \param[in] max_threads   maximum number of threads to use

    Apply frequency domain filter in x- and y-direction. The image is first
    padded with 0s to the next power of 2, then transformed into frequency
    space, then multiplied by the filter and the transformed back into image
    space. The fourier transforms are multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void Filter::filterXY_freq(float * const image,
                           const unsigned short int max_threads) const
 { float *ip;
   unsigned short int y, z;

   for (ip=image,
        z=0; z < z_samples; z++,
        ip+=image_slicesize)
    { memset(fft_buffer, 0, padded_size*sizeof(float));
      for (y=0; y < xy_samples; y++)
       memcpy(&fft_buffer[(unsigned long int)y*xy_samples_padded],
              &ip[(unsigned long int)y*xy_samples],
              xy_samples*sizeof(float));
      fft->rFFT_2D(fft_buffer, true, max_threads);         // FFT of image data
                                                           // filter image data
      vecMul(fft_buffer, filter_array, fft_buffer, padded_size);
      fft->rFFT_2D(fft_buffer, false, max_threads);    // iFFT of filtered data
                                          // copy filtered data back into image
      for (y=0; y < xy_samples; y++)
       memcpy(&ip[(unsigned long int)y*xy_samples],
              &fft_buffer[(unsigned long int)y*xy_samples_padded],
              xy_samples*sizeof(float));
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Filter in x- and y-direction (space domain).
    \param[in] image   image data

    Apply space domain filter in x- and y-direction. For gaussian and boxcar
    filter a convolution between the image and the filter kernel is calculated.
    For the median filter the filter function is applied in y- and x-direction.
 */
/*---------------------------------------------------------------------------*/
void Filter::filterXY_space(float * const image) const
 {
#ifdef __ALTIVEC__
   vImage_Buffer b;
   char *conv_buffer=NULL;

   b.data=NULL;
#endif
   try
   { float *ip;
     unsigned short int x, y, z;

#ifdef __ALTIVEC__
     vImage_Buffer a;

     b.data=new float[image_slicesize];
     b.width=xy_samples;
     b.height=xy_samples;
     b.rowBytes=xy_samples*sizeof(float);
     a.width=xy_samples;
     a.height=xy_samples;
     a.rowBytes=xy_samples*sizeof(float);
     conv_buffer=new char[vImageGetMinimumTempBufferSizeForConvolution(&a, &b,
                                        kernel1d.size(), kernel1d.size(),
                                        kvImageEdgeExtend, sizeof(float))];
#endif
     for (ip=image,
          z=0; z < z_samples; z++,
          ip+=image_slicesize)
      switch (filter.name)
       { case GAUSS:                                         // gaussian filter
#ifdef __ALTIVEC__
          a.data=ip;
          vImageConvolve_PlanarF(&a, &b, conv_buffer, 0, 0, &gauss_kernel2d[0],
                                 kernel1d.size(), kernel1d.size(), 0.0f,
                                 kvImageEdgeExtend);
          memcpy(ip, b.data, image_slicesize*sizeof(float));
          break;
#endif
         case BOXCAR:                                          // boxcar filter
          for (y=0; y < xy_samples; y++)
           vecConvolution(&ip[(unsigned long int)y*xy_samples], 1, xy_samples,
                          &kernel1d[0], kernel1d.size(),
                          &ip[(unsigned long int)y*xy_samples]);
          for (x=0; x < xy_samples; x++)
           vecConvolution(&ip[x], xy_samples, xy_samples, &kernel1d[0],
                          kernel1d.size(), &ip[x]);
          break;
         case MEDIAN:                                          // median filter
          for (y=0; y < xy_samples; y++)
           median_space(&ip[(unsigned long int)y*xy_samples], 1, xy_samples);
          for (x=0; x < xy_samples; x++)
           median_space(&ip[x], xy_samples, xy_samples);
          break;
         default:
          break;
       }
#ifdef __ALTIVEC__
     delete[] conv_buffer;
     conv_buffer=NULL;
     delete[] b.data;
     b.data=NULL;
#endif
   }
   catch (...)
    {
#ifdef __ALTIVEC__
      if (b.data != NULL) delete[] b.data;
      if (conv_buffer != NULL) delete[] conv_buffer;
#endif
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Filter in z-direction (frequency domain).
    \param[in] image         image data

    Apply frequency domain filter in z-direction. The image is first
    extrapolated to the next power of 2 by
    \f[
        v_i=v_{n-1}+\frac{v_0-v_{n-1}}{2}
            \left(1-\cos\left(\Pi\frac{i-n}{m-n-1}\right)\right)
            \quad\quad\forall n\le m
    \f]
    where \f$n\f$ is the image size and \f$m\f$ is the next power of 2. Then
    it is transformed into frequency space, then multiplied by the filter and
    finally transformed back into image space.
 */
/*---------------------------------------------------------------------------*/
void Filter::filterZ_freq(float * const image) const
 { for (unsigned long int xy=0; xy < image_slicesize; xy++)
    { float *ip2, y0, y1;
      unsigned short int i;
                                                  // filter in frequency domain
                             // copy image data into left side of padded buffer
      for (ip2=&image[xy],
           i=0; i < z_samples; i++)
       fft_buffer[i]=ip2[(unsigned long int)i*image_slicesize];
                                // create right side of buffer by interpolation
      y0=fft_buffer[0];
      y1=fft_buffer[z_samples-1];
      for (i=z_samples; i < padded_size; i++)
       { float interp_coeff;

         interp_coeff=0.5f*(1.0f-cosf(M_PI*(float)(i-z_samples)/
                                           (float)(padded_size-z_samples-1)));
         fft_buffer[i]=y1+(float)(y0-y1)*interp_coeff;
       }
      fft->rFFT_1D(fft_buffer, true);                            // FFT of data
                                                       // filter real valued FT
      fft_buffer[1]*=filter_array[1]*filter_array[0];
      for (i=2; i < padded_size; i++)
       fft_buffer[i]*=filter_array[i/2];
      fft->rFFT_1D(fft_buffer, false);                          // iFFT of data
                                          // copy filtered data back into image
      for (ip2=&image[xy],
           i=0; i < z_samples; i++)
       ip2[(unsigned long int)i*image_slicesize]=fft_buffer[i];
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Filter in z-direction (space domain).
    \param[in] image   image data

    Apply space domain filter in z-direction. For gaussian and boxcar filter a
    convolution between the image and the filter kernel is calculated.
    For the median filter the filter function is applied z-direction.
 */
/*---------------------------------------------------------------------------*/
void Filter::filterZ_space(float * const image) const
 { unsigned long int xy;

   switch (filter.name)
    { case GAUSS:
      case BOXCAR:
       for (xy=0; xy < image_slicesize; xy++)
        vecConvolution(&image[xy], image_slicesize, z_samples, &kernel1d[0],
                       kernel1d.size(), &image[xy]);
       break;
      case MEDIAN:
       for (xy=0; xy < image_slicesize; xy++)
        median_space(&image[xy], image_slicesize, z_samples);
       break;
      default:
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FWHM of current filter from Nyquist units.
    \param[in] binsize       width of bins in mm
    \param[in] max_threads   maximum number of threads to use
    \return FWHM of filter

    Calculate FWHM of current filter from Nyquist units. The filter kernel is
    calculated in frequency space with the size given in Nyquist units. Then
    the filter is transformed into image space. To determine the FWHM of the
    filter, the peak is searched and then the reight neighbour that is below
    the half maximum. The exact FWHM is calculated by linear interpolation
    between this point and its immediate left neighbour.
 */
/*---------------------------------------------------------------------------*/
float Filter::fwhm_to_nyquist_sub(const float binsize,
                                  const unsigned short int max_threads) const
 { float *buffer=NULL;
   FFT *fft=NULL;

   try
   { std::vector <float> kernel(128);
 
     if (xy_dir)                                     // generate 2D-filter mask
      { buffer=new float[128*128];       // generate filter in frequency domain
        memset(buffer, 0, 128*128*sizeof(float));        // imaginary part is 0
        for (unsigned short int y=0; y < 128; y++)
         { float fy2;

           if (y < 64) fy2=(float)y;
            else fy2=(float)y-128.0f;
           fy2/=64.0f;
           fy2*=fy2;
           for (unsigned short int x=0; x <= 64; x++)
            if (((x > 0) && (x < 64)) || (y <=64))
             { float fx, freq;

               if (x < 64) fx=(float)x/64.0f;
                else fx=-1.0f;
               freq=sqrtf(fx*fx+fy2);
               buffer[y*128+x]=filter_value(freq);                 // real part
             }
          }
        fft=new FFT(128, 128);
                                          // transform filter into space domain
        fft->rFFT_2D(buffer, false, max_threads);
        delete fft;
        fft=NULL;
        memcpy(&kernel[0], buffer, kernel.size()*sizeof(float));
        delete[] buffer;
        buffer=NULL;
      }
      else {                             // generate filter in frequency domain
             kernel[0]=filter_value(0.0f);
             kernel[1]=filter_value(-1.0f);
             for (unsigned short int x=1; x < 64; x++)
              { kernel[x*2]=filter_value((float)x/64.0f);          // real part
                kernel[x*2+1]=0.0f;                           // imaginary part
              }
             fft=new FFT(128);
                                          // transform filter into space domain
             fft->rFFT_1D(&kernel[0], false);
             delete fft;
             fft=NULL;
           }
     float peak, fwhm;
     unsigned short int max_subscript, x;
     bool found;
                                                       // search peak of filter
     peak=kernel[0];
     max_subscript=0;
     for (x=1; x < 64; x++)
      if (kernel[x] > peak) { peak=kernel[x];
                              max_subscript=x;
                            }
                    // find right neighbour of peak which is below half maximum
     found=false;
     for (x=max_subscript; x < 64; x++)
      if (kernel[x] < 0.5f*peak) { found=true;
                                   break;
                                 }
     if (!found || (peak <= 0.0f) || (x == max_subscript)) return(-1.0f);
                                // calculate exact fwhm by linear interpolation
     if (fabsf(kernel[x]-kernel[x-1]) < 0.0001f*0.5f*peak) fwhm=(float)(2*x-1);
      else fwhm=2.0f*((float)x-1.0f+(0.5f*peak-kernel[x-1])/
                                    (kernel[x]-kernel[x-1]));
     return(fwhm*binsize);
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      if (fft != NULL) delete fft;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate Nyquist units of current filter from FWHM.
    \param[in] binsize       width of bins in mm
    \param[in] max_threads   maximum number of threads to use

    Calculate Nyquist units of current filter from FWHM. This method uses the
    Nyquist to FWHM conversion to calculate FWHM values for different Nyquist
    values. The FWHM value is then calculated by linear interpolation between
    the Nyquist values of the two neighbouring FWHM values.
 */
/*---------------------------------------------------------------------------*/
void Filter::fwhm_to_nyquist(const float binsize,
                             const unsigned short int max_threads)
 { float fwhm_at_low_end, fc_gauss, fc_lower, fc_upper, old_alpha,
         old_filter_resolution;
   unsigned short int i;
   bool found;
   std::vector <float> fwhm_values(11), nyquist_trial_values(11);
                                     // calculate gaussian for initial estimate
   if (filter.resolution < binsize) fc_gauss=4.0f*logf(2.0f)/M_PI;
    else fc_gauss=4.0f*logf(2.0f)/M_PI*binsize/filter.resolution;
                        // calculate fwhm of filter with 1.0*nyquist as low-end
   old_filter_resolution=filter.resolution;
   old_alpha=filter.hamming_alpha;
   switch (filter.name)
    { case BUTTERWORTH:
       filter.resolution=1.0f;
       break;
      case HAMMING:
       filter.hamming_alpha=0.54f;
       filter.resolution=1.0f/(acosf((0.5f-filter.hamming_alpha)/
                                     (1.0f-filter.hamming_alpha))/M_PI);
       break;
      case HANN:
       filter.resolution=1.0f/0.5f;
       break;
      case PARZEN:
       filter.resolution=1.0f/0.362f;
       break;
      case SHEPP_LOGAN:
       filter.resolution=1.0f/1.21f;
       break;
      default:
       return;
    }
   fwhm_at_low_end=fwhm_to_nyquist_sub(binsize, max_threads);
   if (old_filter_resolution < fwhm_at_low_end) return;
                                 // calculate fwhm for different nyquist values
   fc_lower=fc_gauss/5.0f;
   fc_upper=fc_gauss*5.0f;
   if ((filter.name == BUTTERWORTH) && (filter.butterworth_order < 2))
    { fc_lower=0.3f*fc_lower;
      fc_upper=0.3f*fc_upper;
    }
    else if (filter.name == SHEPP_LOGAN) fc_lower=2.5f*fc_lower;
   for (i=0; i < 11; i++)
    {                                               // nyquist value for filter
      filter.resolution=expf(logf(fc_lower)+
                             (logf(fc_upper)-logf(fc_lower))*i/11.0f);
      nyquist_trial_values[i]=filter.resolution;
                                                 // fwhm for this nyquist value
      fwhm_values[i]=fwhm_to_nyquist_sub(binsize, max_threads);
    }
   filter.resolution=old_filter_resolution;
   filter.hamming_alpha=old_alpha;
                                            // reduce list to valid fwhm values
   { unsigned short int count=0;

     for (i=0; i < 11; i++)
      if (fwhm_values[i] != -1.0f)                                   // valid ?
       { fwhm_values[count]=fwhm_values[i];
         nyquist_trial_values[count++]=nyquist_trial_values[i];
       }
     if (count < 2) { filter.resolution=fc_gauss;
                      return;
                    }
                            // find neighbouring fwhm values for fwhm of filter
     found=false;
     for (i=0; i < count-1; i++)
      if (((fwhm_values[i] <= filter.resolution) &&
           (fwhm_values[i+1] >= filter.resolution)) ||
          ((fwhm_values[i] >= filter.resolution) &&
           (fwhm_values[i+1] <= filter.resolution))) { found=true;
                                                       break;
                                                     }
   }
   if (!found) { filter.resolution=fc_gauss;
                 return;
               }
                   // calculate nyquist value of filter by linear interpolation
   filter.resolution=(filter.resolution-fwhm_values[i])*
                     (nyquist_trial_values[i+1]-nyquist_trial_values[i])/
                     (fwhm_values[i+1]-fwhm_values[i])+
                     nyquist_trial_values[i];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize filter for frequency domain.
    \param[in] nyquist_pixelratio   ratio between scanner bin size and pixel
                                    size

    Initialize filter for frequency domain and initialize FFT object.
 */
/*---------------------------------------------------------------------------*/
void Filter::init_frequency_domain(const float nyquist_pixelratio)
 { try
   { filter.resolution*=nyquist_pixelratio;
     if (xy_dir)                                  // filtering in x/y-direction
      { unsigned short int y;
                                                     // create 2D filter matrix
        xy_samples_padded=1;
        do { xy_samples_padded*=2;
           } while (xy_samples_padded < 2*xy_samples);
        padded_size=(unsigned long int)xy_samples_padded*
                    (unsigned long int)xy_samples_padded;
        if (filter_array != NULL) delete[] filter_array;
        filter_array=new float[padded_size];
                          // real and imaginary part of the filter are the same
                          // filter is to be applied after rFFT_2D
        for (y=0; y < xy_samples_padded; y++)
         { float fy2;
           unsigned short int x;

           if (y < xy_samples_padded/2) fy2=(float)y;
            else fy2=(float)y-(float)xy_samples_padded;
           fy2/=0.5f*(float)xy_samples_padded;
           fy2*=fy2;
           for (x=0; x <= xy_samples_padded/2; x++)
            if (((x > 0) && (x < xy_samples_padded/2)) ||
                (y <= xy_samples_padded/2))
            { float fx, freq;

              if (x < xy_samples_padded/2) fx=(float)x;
               else fx=(float)x-(float)xy_samples_padded;
              fx/=0.5f*(float)xy_samples_padded;
              freq=sqrtf(fx*fx+fy2);
              filter_array[y*xy_samples_padded+x]=filter_value(freq);
            }
                                             // copy filter into imaginary part
           for (x=xy_samples_padded/2; x < xy_samples_padded; x++)
            filter_array[y*xy_samples_padded+x]=
             filter_array[y*xy_samples_padded+xy_samples_padded-x];
         }
                                             // copy filter into imaginary part
        for (y=xy_samples_padded/2; y < xy_samples_padded; y++)
         { filter_array[y*xy_samples_padded]=
            filter_array[(xy_samples_padded-y)*xy_samples_padded];
           filter_array[y*xy_samples_padded+xy_samples_padded/2]=
            filter_array[(xy_samples_padded-y)*xy_samples_padded+
                         xy_samples_padded/2];
         }
                                                           // initialize 2D-FFT
        if (fft_buffer != NULL) delete[] fft_buffer;
        fft_buffer=new float[padded_size];
        if (fft != NULL) delete fft;
        fft=new FFT(xy_samples_padded, xy_samples_padded);
      }
      else {                                        // filtering in z-direction
                                                     // create 1D filter matrix
             padded_size=1;
             do { padded_size*=2;
                } while ((unsigned short int)padded_size < 2*z_samples);
             if (filter_array != NULL) delete[] filter_array;
             filter_array=new float[padded_size/2];
             for (unsigned short int z=1; z < padded_size/2; z++)
              filter_array[z]=filter_value((float)z/(0.5f*(float)padded_size));
             filter_array[0]=filter_value(-1.0f);
                                                           // initialize 1D-FFT
             if (fft != NULL) delete fft;
             fft=new FFT(padded_size);
             if (fft_buffer != NULL) delete[] fft_buffer;
             fft_buffer=new float[padded_size];
           }
   }
   catch (...)
    { if (fft_buffer != NULL) { delete[] fft_buffer;
                                fft_buffer=NULL;
                              }
      if (fft != NULL) { delete[] fft;
                         fft=NULL;
                       }
      if (filter_array != NULL) { delete[] filter_array;
                                  filter_array=NULL;
                                }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize filter for space domain.

    Initialize gaussian or boxcar filter kernel for space domain.
 */
/*---------------------------------------------------------------------------*/
void Filter::init_space_domain()
 { filter.resolution/=pixelsize;
   if ((filter.name == GAUSS) && (filter.resolution > 0.0f))
    { float sum=0.0f, *gp;
      unsigned short int filter_width, i;
                                                  // create gauss filter kernel
      filter_width=(unsigned short int)ceilf(filter.resolution);
      if (filter_width == (unsigned short int)filter.resolution)
       filter_width++;
      kernel1d.resize(filter_width*2+1);
      for (gp=&kernel1d[filter_width],
           i=0; i <= filter_width; i++)
       { float val;

         val=2.0f*(float)i/filter.resolution;
         gp[i]=powf(2.0f, -val*val);
         sum+=gp[i];
         if (i > 0)
          { kernel1d[filter_width-i]=gp[i];
            sum+=gp[i];
          }
       }
#ifdef __ALTIVEC__
      gauss_kernel2d.resize(kernel1d.size()*kernel1d.size());
      for (i=0; i <= filter_width; i++)
       { for (unsigned short int j=0; j <= filter_width; j++)
          { float vx, vy;

            vx=2.0f*(float)j/filter.resolution;
            vy=2.0f*(float)i/filter.resolution;
            gauss_kernel2d[(i+filter_width)*kernel1d.size()+
                           j+filter_width]=powf(2.0f, -(vx*vx+vy*vy));
            if (j > 0)
             gauss_kernel2d[(i+filter_width)*kernel1d.size()-
                            j+filter_width]=
                            gauss_kernel2d[(i+filter_width)*kernel1d.size()+
                                           j+filter_width];
          }
         if (i > 0)
          memcpy(&gauss_kernel2d[(filter_width-i)*kernel1d.size()],
                 &gauss_kernel2d[(filter_width+i)*kernel1d.size()],
                 kernel1d.size()*sizeof(float));
       }
      sum=0.0f;
      for (i=0; i < gauss_kernel2d.size(); i++)
       sum+=gauss_kernel2d[i];
      for (i=0; i < gauss_kernel2d.size(); i++)
       gauss_kernel2d[i]/=sum;
#endif
      }
      else if (filter.name == BOXCAR)
            kernel1d.assign(std::max(1+2*(unsigned long int)
                                         round((filter.resolution-1.0f)/2.0f),
                                     (unsigned long int)3), 1.0f);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Apply median filter (space domain).
    \param[in] data   data to filter
    \param[in] offs   offset between neighbouring data elements
    \param[in] len    number of data elements

    Apply median filter (space domain).
 */
/*---------------------------------------------------------------------------*/
void Filter::median_space(float * const data, const unsigned long int offs,
                          const unsigned short int len) const
 { float *buffer=NULL;

   try
   { unsigned short int filter_width, fw2, i;
     std::vector <float> sort_buffer;
                                                    // filter width must be odd
     filter_width=1+2*(unsigned short int)round((filter.resolution-1.0f)/2.0f);
     if (filter_width < 3) filter_width=3;
     fw2=(filter_width-1)/2;
     buffer=new float[len-filter_width+1];          // buffer for filtered data
     memset(buffer, 0, (len-filter_width+1)*sizeof(float));
     sort_buffer.resize(filter_width);
     for (i=fw2; i < len-fw2; i++)                               // filter data
      { unsigned short int j;
        float *dp;
                                                       // copy data into buffer
        for (dp=data+(i-fw2)*offs,
             j=0; j < filter_width; j++)
         sort_buffer[j]=dp[j*offs];
                   // sort first half of buffer (only median element is needed)
                   // (Bubblesort; fast enough for small filter_width)
        for (j=0; j <= fw2; j++)
         for (unsigned short int k=j; k < filter_width; k++)
          if (sort_buffer[j] > sort_buffer[k])
           std::swap(sort_buffer[j], sort_buffer[k]);
        buffer[i-fw2]=sort_buffer[fw2];                         // store median
      }
                                          // copy filtered data back into image
     for (i=fw2; i < len-fw2; i++)
      data[(unsigned long int)i*offs]=buffer[i-fw2];
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }
