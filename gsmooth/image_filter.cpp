//** file: filter.cpp
//** date: 2003/01/14

//** author: Frank Kehren
//** © CPS Innovations

/*- code design ---------------------------------------------------------------

The boxcar-, gauss- and median-filter are implemented in image space, while the
Butterworth-, Hamming-, Hann-, Parzen- and Shepp-Logan-filters are in frequency
space. This class provides 2d-filters for x/y-direction and 1d-filters for
z-direction. In case a filter is not separable (Butterwort, Shepp-Logan) a
filtering in x/y- followed by a filtering in z-direction is not a correct
3d filter.

-----------------------------------------------------------------------------*/
/*
   Modification history: Merence Sibomana for HRRT Users Community
        25-may-2010: Use SIMD for gaussian smoothing 
*/

#if defined(__LINUX__) && defined(__INTEL_COMPILER)
#include <mathimf.h>
#else
#include <cmath>
#endif
#include "image_filter.h"
#include "const.h"
#include "global_tmpl.h"
#ifndef GSMOOTH
//#include "fft.h"
#endif
#include <string.h>
#include <xmmintrin.h>
/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* ImageFilter: initialize filter object                                          */
/*  ixy_samples           width/height of image                              */
/*  iz_samples            depth of image                                     */
/*  ifilter               filter parameters                                  */
/*  ipixelsize            pixelsize of image in filter direction             */
/*  nyquist_pixelratio    ratio between scanner bin size and pixel size      */
/*  ixy_dir               filter in x/y-direction ? (otherwise z-direction)  */
/*  max_threads           maximum number of threads to use                   */
/*---------------------------------------------------------------------------*/
ImageFilter::ImageFilter(const unsigned short int ixy_samples,
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
   gauss_kernel=NULL;
   filter_array=NULL;
   fft_buffer=NULL;
#ifndef GSMOOTH
   fft=NULL;
#else
   gauss_kernel_ps =NULL;
#endif
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
          if (filter.name == GAUSS) filter.resolution *= (float)(4.0f*logf(2.0f)/M_PI);
          filter.restype=FWHM;
        }
       init_space_domain();
       break;
      case BUTTERWORTH:
      case HAMMING:
      case HANN:
      case PARZEN:
      case SHEPP_LOGAN:
#ifndef GSMOOTH
       domain=FREQUENCY;
       if (filter.restype == FWHM)
        { fwhm_to_nyquist(pixelsize/nyquist_pixelratio, max_threads);
          filter.restype=NYQUIST;
        }
       init_frequency_domain(nyquist_pixelratio);
#endif
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/* ~ImageFilter: destroy object                                                   */
/*---------------------------------------------------------------------------*/
ImageFilter::~ImageFilter()
 { if (gauss_kernel != NULL) delete[] gauss_kernel;
   if (fft_buffer != NULL) delete[] fft_buffer;
#ifndef GSMOOTH
   if (fft != NULL) delete fft;
#else
   if (gauss_kernel_ps != NULL) _mm_free(gauss_kernel_ps);
#endif
   if (filter_array != NULL) delete[] filter_array;
 }

/*---------------------------------------------------------------------------*/
/* boxcar_space: apply boxcar filter (space domain)                          */
/*  data   data to filter                                                    */
/*  offs   offset between neighbouring data elements                         */
/*  len    number of data elements                                           */
/*---------------------------------------------------------------------------*/
void ImageFilter::boxcar_space(float * const data, const unsigned long int offs,
                          const unsigned short int len) const
 { float *buff=NULL;

   try
   { float sf;
     unsigned long int i;
     unsigned short int half_width, filter_width;

#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
     filter_width=std::max(1+2*(unsigned short int)
                               round((filter.resolution-1.0f)/2.0f), 3);
#endif
#ifdef WIN32
     filter_width=maxi(1+2*(unsigned short int)
                           round((filter.resolution-1.0f)/2.0f), 3);
#endif
     half_width=(filter_width-1)/2;
     buff=new float[len+filter_width-1];
                                 // copy image data into temporary buffer,
                                 // to avoid separate handling of image borders
                                                      // duplicate image border
     for (i=0; i < half_width; i++)
      buff[i]=data[0];
                                           // copy image data and remove stride
     for (i=half_width; i < len+half_width; i++)
      buff[i]=data[(unsigned long int)(i-half_width)*offs];
                                                      // duplicate image border
     for (i=len+half_width; i < len+filter_width-1; i++)
      buff[i]=data[(unsigned long int)(len-1)*offs];
                                                   // convolution of image data
     sf=1.0f/(float)filter_width;
     for (i=0; i < len; i++)
      { float result;

        result=0.0f;
        for (unsigned long int j=0; j < filter_width; j++)
         result+=buff[i+j];
        data[(unsigned long int)i*offs]=result*sf;
      }
     delete[] buff;
     buff=NULL;
   }
   catch (...)
    { if (buff != NULL) delete[] buff;
      throw;
    }
 }
#ifndef GSMOOTH
/*---------------------------------------------------------------------------*/
/* calcFilter: perform filter operation                                      */
/*  image         image data                                                 */
/*  max_threads   maximum number of threads to use                           */
/*---------------------------------------------------------------------------*/
void ImageFilter::calcFilter(float * const image,
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
#else
/*---------------------------------------------------------------------------*/
/* calcFilter: perform gaussian filter operation                                      */
/*  image         image data                                                 */
/*  max_threads   maximum number of threads to use                           */
/*---------------------------------------------------------------------------*/
void ImageFilter::calcFilter(float * const image,
                        const unsigned short int max_threads) const
{ 
  if (filter.name == ALL_PASS) return;
  if (xy_dir) filterXY_space(image);
  else filterZ_space(image);
}
void ImageFilter::calcFilter_ps(float * const image) const
{ 
  if (filter.name == ALL_PASS) return;
  if (xy_dir) filterXY_space_ps(image);
  else filterZ_space_ps(image);
}
#endif

/*---------------------------------------------------------------------------*/
/* filter_value: calculate filter value for specified frequency              */
/*  freq   frequency                                                         */
/* return: filter value                                                      */
/*---------------------------------------------------------------------------*/
float ImageFilter::filter_value(float freq) const
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

#ifndef GSMOOTH
/*---------------------------------------------------------------------------*/
/* filterXY_freq: filter in x- and y-direction (frequency domain)            */
/*  image         image data                                                 */
/*  max_threads   maximum number of threads to use                           */
/*---------------------------------------------------------------------------*/
void ImageFilter::filterXY_freq(float * const image,
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
      for (unsigned long int xy=0; xy < padded_size; xy++)
       fft_buffer[xy]*=filter_array[xy];
      fft->rFFT_2D(fft_buffer, false, max_threads);    // iFFT of filtered data
                                          // copy filtered data back into image
      for (y=0; y < xy_samples; y++)
       memcpy(&ip[(unsigned long int)y*xy_samples],
              &fft_buffer[(unsigned long int)y*xy_samples_padded],
              xy_samples*sizeof(float));
    }
 }
#endif

/*---------------------------------------------------------------------------*/
/* filterXY_space: filter in x- and y-direction (space domain)               */
/*  image         image data                                                 */
/*---------------------------------------------------------------------------*/
void ImageFilter::filterXY_space(float * const image) const
 { float *ip;
   unsigned short int x, y, z;

   for (ip=image,
        z=0; z < z_samples; z++,
        ip+=image_slicesize)
    switch (filter.name)
     { case GAUSS:
        for (y=0; y < xy_samples; y++)
         gauss_space(&ip[(unsigned long int)y*xy_samples], 1, xy_samples);
        for (x=0; x < xy_samples; x++)
         gauss_space(&ip[x], xy_samples, xy_samples);
        break;
       case BOXCAR:
        for (y=0; y < xy_samples; y++)
         boxcar_space(&ip[(unsigned long int)y*xy_samples], 1, xy_samples);
        for (x=0; x < xy_samples; x++)
         boxcar_space(&ip[x], xy_samples, xy_samples);
        break;
       case MEDIAN:
        for (y=0; y < xy_samples; y++)
         median_space(&ip[(unsigned long int)y*xy_samples], 1, xy_samples);
        for (x=0; x < xy_samples; x++)
         median_space(&ip[x], xy_samples, xy_samples);
        break;
     }
 }
#ifdef GSMOOTH
void ImageFilter::filterXY_space_ps(float * const image) const
{
  if (filter.name == GAUSS)
  {
    float *ip;
    unsigned  x, y, z;
    for (ip=image, z=0; z < z_samples; z++, ip += image_slicesize)
    {
      // X direction smoothing
      for (y=0; y < xy_samples; y += 4)
        gauss_space_ps(&ip[y*xy_samples], xy_samples, 1, xy_samples);
      // smooth in y direction
      for (x=0; x < xy_samples; x += 4)
        gauss_space_ps(&ip[x], 1, xy_samples, xy_samples);
    }
  } else filterXY_space(image);
  return;
}
#endif
#ifndef GSMOOTH
/*---------------------------------------------------------------------------*/
/* filterZ_freq: filter in z-direction (frequency domain filters)            */
/*  image   image data                                                       */
/*---------------------------------------------------------------------------*/
void ImageFilter::filterZ_freq(float * const image) const
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
/* fwhm_to_nyquist_sub: calculate fwhm of current filter which is specified  */
/*                      in nyquist units                                     */
/*  binsize       width of bins in mm                                        */
/*  max_threads   maximum number of threads to use                           */
/* return: fwhm of filter                                                    */
/*---------------------------------------------------------------------------*/
float ImageFilter::fwhm_to_nyquist_sub(const float binsize,
                                  const unsigned short int max_threads) const
 { float *kernel=NULL, *buffer=NULL;
   FFT *fft=NULL;

   try
   { kernel=new float[128];
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
               buffer[y*128+x]=filter_value(freq);                // real part 
             }
          }
        fft=new FFT(128, 128);
                                          // transform filter into space domain
        fft->rFFT_2D(buffer, false, max_threads);
        delete fft;
        fft=NULL;
        memcpy(kernel, buffer, 128*sizeof(float));
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
             fft->rFFT_1D(kernel, false); // transform filter into space domain
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
     if (!found || (peak <= 0.0f) || (x == max_subscript))
      { delete[] kernel;
        kernel=NULL;
        return(-1.0f);
      }
                                // calculate exact fwhm by linear interpolation
     if (fabsf(kernel[x]-kernel[x-1]) < 0.0001f*0.5f*peak) fwhm=(float)(2*x-1);
      else fwhm=2.0f*((float)x-1.0f+(0.5f*peak-kernel[x-1])/
                                    (kernel[x]-kernel[x-1]));
     delete[] kernel;
     kernel=NULL;
     return(fwhm*binsize);
   }
   catch (...)
    { if (kernel != NULL) delete[] kernel;
      if (buffer != NULL) delete[] buffer;
      if (fft != NULL) delete fft;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* init_frequency_domain: initialize filter for frequency domain             */
/*  nyquist_pixelratio   ratio between scanner bin size and pixel size       */
/*---------------------------------------------------------------------------*/
void ImageFilter::init_frequency_domain(const float nyquist_pixelratio)
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
/* fwhm_to_nyquist: transform fwhm value of filter to nyquist value          */
/*  binsize       width of bins in mm                                        */
/*  max_threads   maximum number of threads to use                           */
/*---------------------------------------------------------------------------*/
void ImageFilter::fwhm_to_nyquist(const float binsize,
                             const unsigned short int max_threads)
 { float *fwhm_values=NULL, *nyquist_trial_values=NULL;

   try
   { float fwhm_at_low_end, fc_gauss, fc_lower, fc_upper, old_alpha,
           old_filter_resolution;
     unsigned short int i;
     bool found;
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
     fwhm_values=new float[11];
     nyquist_trial_values=new float[11];
     for (i=0; i < 11; i++)
      {                                             // nyquist value for filter
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
        if (fwhm_values[i] != -1.0f)                                 // valid ?
         { fwhm_values[count]=fwhm_values[i];
           nyquist_trial_values[count++]=nyquist_trial_values[i];
         }
       if (count < 2) { filter.resolution=fc_gauss;
                        delete[] nyquist_trial_values;
                        nyquist_trial_values=NULL;
                        delete[] fwhm_values;
                        fwhm_values=NULL;
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
                   delete[] nyquist_trial_values;
                   nyquist_trial_values=NULL;
                   delete[] fwhm_values;
                   fwhm_values=NULL;
                   return;
                 }
                   // calculate nyquist value of filter by linear interpolation
     filter.resolution=(filter.resolution-fwhm_values[i])*
                       (nyquist_trial_values[i+1]-nyquist_trial_values[i])/
                       (fwhm_values[i+1]-fwhm_values[i])+
                       nyquist_trial_values[i];
     delete[] nyquist_trial_values;
     nyquist_trial_values=NULL;
     delete[] fwhm_values;
     fwhm_values=NULL;
   }
   catch (...)
    { if (fwhm_values != NULL) delete[] fwhm_values;
      if (nyquist_trial_values != NULL) delete[] nyquist_trial_values;
      throw;
    }
 }

#endif

/*---------------------------------------------------------------------------*/
/* filterZ_space: filter in z-direction (space domain)                       */
/*  image   image data                                                       */
/*---------------------------------------------------------------------------*/
void ImageFilter::filterZ_space(float * const image) const
 { unsigned long int xy;

   switch (filter.name)
    { case GAUSS:
       for (xy=0; xy < image_slicesize; xy++)
        gauss_space(&image[xy], image_slicesize, z_samples);
       break;
      case BOXCAR:
       for (xy=0; xy < image_slicesize; xy++)
        boxcar_space(&image[xy], image_slicesize, z_samples);
       break;
      case MEDIAN:
       for (xy=0; xy < image_slicesize; xy++)
        median_space(&image[xy], image_slicesize, z_samples);
       break;
    }
 }
#ifdef GSMOOTH
void ImageFilter::filterZ_space_ps(float * const image) const
{ 
  if (filter.name == GAUSS)
  {
    for (unsigned xy=0; xy < image_slicesize; xy += 4)
        gauss_space_ps(&image[xy], 1, image_slicesize, z_samples);
  } else filterZ_space(image);
}
#endif


/*---------------------------------------------------------------------------*/
/* gauss_space: apply gaussian filter (space domain)                         */
/*  data   data to filter                                                    */
/*  offs   offset between neighbouring data elements                         */
/*  len    number of data elements                                           */
/*---------------------------------------------------------------------------*/
void ImageFilter::gauss_space(float * const data,
                         const unsigned long int offs,
                         const unsigned short int len) const
 { float *buff=NULL;

   try
   { unsigned long int i;
     unsigned short int half_width;

     if (filter.resolution <= 0.0f) return;
     half_width=(gauss_kernel_width-1)/2;
     buff=new float[len+gauss_kernel_width-1];
                                 // copy image data into temporary buffer,
                                 // to avoid separate handling of image borders
                                                      // duplicate image border
     for (i=0; i < half_width; i++)
      buff[i]=data[0];
                                           // copy image data and remove stride
     for (i=half_width; i < len+half_width; i++)
      buff[i]=data[(unsigned long int)(i-half_width)*offs];
                                                      // duplicate image border
     for (i=len+half_width; i < len+gauss_kernel_width-1; i++)
      buff[i]=data[(unsigned long int)(len-1)*offs];
                                                   // convolution of image data
     for (i=0; i < len; i++)
      { float result;

        result=0.0f;
        for (unsigned long int j=0; j < gauss_kernel_width; j++)
         result+=buff[i+j]*gauss_kernel[j];
        data[(unsigned long int)i*offs]=result;
      }
     delete[] buff;
     buff=NULL;
   }
   catch (...)
    { if (buff != NULL) delete[] buff;
      throw;
    }
 }


/*---------------------------------------------------------------------------*/
/* gauss_space: apply gaussian filter (space domain)                         */
/*  data   data to filter                                                    */
/*  offs   offset between neighbouring data elements                         */
/*  len    number of data elements                                           */
/*  Timing: 
/*         data access and packing xy=0.141sec, z=0.266sec                   */
/*         smoothing  xy=0.343sec, z=0.162sec                                */
/*---------------------------------------------------------------------------*/
void ImageFilter::gauss_space_ps(float * const data, int off_ps,
                         const unsigned long int offs,
                         const unsigned short int len) const
{
  float *buf=NULL, *result=NULL;
  float *src=NULL, *dst=NULL, v=0.0f;
  __m128 *buf_128=NULL, *gauss_kernel_128=NULL, *result_128=NULL;
  unsigned long int i,j,k;
  unsigned short int half_width, len_4;

  if (filter.resolution <= 0.0f) return;
  half_width=(gauss_kernel_width-1)/2;
  len_4 = ((len+3)/4)*4;
  buf = (float*)_mm_malloc((len_4+2*half_width)*sizeof(__m128),16);
  result = (float*)_mm_malloc(len_4*sizeof(__m128),16);
  buf_128 = (__m128 *)buf;
  result_128 = (__m128 *)result;
  gauss_kernel_128 = (__m128*)gauss_kernel_ps;
                                 // pack image data into temporary buffer,
                                 // with left and right padding
  src = data;
  buf_128[0] = _mm_set_ps(src[3*off_ps], src[2*off_ps], src[off_ps],src[0]);
  for (i=1; i<half_width;  i++) buf_128[i] = buf_128[0];
  for (k=0; k < len; k++, i++)
  {
    buf_128[i] = _mm_set_ps(src[3*off_ps], src[2*off_ps], src[off_ps],src[0]);
    src += offs;
  }
  j = i-1;
  for (k=0; k < half_width; k++, i++) buf_128[i] = buf_128[j];
                                           // convolution of __m128 buffer
  for (i=0; i<len; i++)
  { 
    result_128[i] = _mm_set1_ps(0.0f);
    for (j=0; j < gauss_kernel_width; j++)
    {
      result_128[i] = _mm_add_ps(result_128[i], 
                     _mm_mul_ps(buf_128[i+j], gauss_kernel_128[j]));
    }
  }
                        // unpack __m128 result
  dst = data;
  src = result;
  for (i=0; i<len; i++)
  {
    dst[0] = *src++;
    dst[off_ps] = *src++;
    dst[2*off_ps] = *src++;
    dst[3*off_ps] = *src++;

    dst += offs;
  }
  _mm_free(buf);
  _mm_free(result);
}

/*---------------------------------------------------------------------------*/
/* init_space_domain: initialize filter for space domain                     */
/*---------------------------------------------------------------------------*/
void ImageFilter::init_space_domain()
 { try
   { filter.resolution/=pixelsize;
     if ((filter.name == GAUSS) && (filter.resolution > 0.0f))
      { float sum=0.0f, *gp;
        unsigned short int filter_width, i;
                                                  // create gauss filter kernel
        filter_width=(unsigned short int)ceilf(filter.resolution);
        if (filter_width == (unsigned short int)filter.resolution)
         filter_width++;
        gauss_kernel_width=filter_width*2+1;
        if (gauss_kernel != NULL) delete[] gauss_kernel;
#ifdef GSMOOTH
        if (gauss_kernel_ps != NULL) _mm_free(gauss_kernel_ps);
        gauss_kernel_ps = (float*)_mm_malloc(gauss_kernel_width*sizeof(__m128),16);
#endif
        gauss_kernel=new float[gauss_kernel_width];
        for (gp=gauss_kernel+filter_width,
             i=0; i <= filter_width; i++)
         { float val;

           val=2.0f*(float)i/filter.resolution;
           gp[i]=powf(2.0f, -val*val);
           sum+=gp[i];
           if (i > 0)
            { gauss_kernel[filter_width-i]=gp[i];
              sum+=gp[i];
            }
         }
        for (i=0; i < gauss_kernel_width; i++)
         gauss_kernel[i]/=sum;
#ifdef GSMOOTH
        __m128 *g128 = (__m128*)gauss_kernel_ps;
        for (i=0; i < gauss_kernel_width; i++)
          g128[i] = _mm_set1_ps(gauss_kernel[i]);
#endif
      }
   }
  catch (...)
   { if (gauss_kernel != NULL) { delete[] gauss_kernel;
                                 gauss_kernel=NULL;
                               }
     throw;
   }
 }

/*---------------------------------------------------------------------------*/
/* median_space: apply median filter (space domain)                          */
/*  data   data to filter                                                    */
/*  offs   offset between neighbouring data elements                         */
/*  len    number of data elements                                           */
/*---------------------------------------------------------------------------*/
void ImageFilter::median_space(float * const data, const unsigned long int offs,
                          const unsigned short int len) const
 { float *buffer=NULL, *sort_buffer=NULL;

   try
   { unsigned short int filter_width, fw2, i;
                                                    // filter width must be odd
     filter_width=1+2*(unsigned short int)round((filter.resolution-1.0f)/2.0f);
     if (filter_width < 3) filter_width=3;
     fw2=(filter_width-1)/2;
     buffer=new float[len-filter_width+1];          // buffer for filtered data
     memset(buffer, 0, (len-filter_width+1)*sizeof(float));
     sort_buffer=new float[filter_width];
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
     delete[] sort_buffer;
     sort_buffer=NULL;
                                          // copy filtered data back into image
     for (i=fw2; i < len-fw2; i++)
      data[(unsigned long int)i*offs]=buffer[i-fw2];
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      if (sort_buffer != NULL) delete[] sort_buffer;
      throw;
    }
 }
