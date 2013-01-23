/*! \file dift.h
    \brief This class implements the DIFT (direct inverse fourier transform)
           reconstruction of 2d sinograms.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/02/02 fixed the distortion-correction function, if the image
                     size is not a power of 2
    \date 2005/01/25 added Doxygen style comments
 */

#ifndef _DIFT_H
#define _DIFT_H

#include <vector>
#include "fft.h"

/*- class definitions -------------------------------------------------------*/

class DIFT
 {                              // declare wrapper function as friend and allow
                                // it to call private methods of the class
   friend void *executeThread_DIFT(void *);
  protected:
                                              /*! parameters for DIFT thread */
   typedef struct { DIFT *object;                /*!< pointer to DIFT object */
                    float *sinogram,                /*!< pointer to sinogram */
                          *image,                      /*!< pointer to image */
                              /*! diameter of FOV after reconstruction in mm */
                          *fov_diameter;
                    unsigned short int slices,         /*!< number of slices */
                                          /*! max. number of threads for FFT */
                                       max_threads;
                    unsigned short int thread_number;  /*!< number of thread */
#ifdef XEON_HYPERTHREADING_BUG
                                  /*! padding bytes to avoid cache conflicts */
                    char padding[2*CACHE_LINE_SIZE-
                                 3*sizeof(unsigned short int)-
                                 3*sizeof(float *)-sizeof(DIFT *)];
#endif
                  } tthread_params;
   private:
                        /*! expand projections by this factor (with padding) */
    static const unsigned short int EXPAND,
                                    N_CORNERS;      /*!< corners to a square */
    static const float SMALL_VALUE;/*!< weights smaller than this are zeroed */
    unsigned short int XYSamples,    /*!< width/heigt of reconstructed image */
                         /*! width/heigt of reconstructed image with padding */
                       XYSamples_padded,
                       origRhoSamples,     /*!< number of bins in projection */
                          /*! number of bins in projection (next power of 2) */
                       RhoSamples,
                       ThetaSamples,       /*!< number of angles in sinogram */
                       RhoSamplesExpand,      /*!< size of padded projection */
                       loglevel;                  /*!< top level for logging */
    unsigned long int image_plane_size,             /*!< size of image plane */
                                              /*! size of padded image plane */
                      image_plane_size_padded,
                      sino_plane_size;           /*!< size of sinogram plane */
                              /*! exp-function for shift in frequency domain */
    std::vector <float> cor_array;
    float DeltaXY;                       /*!< width/height of voxel in image */
                /*! weights for coordinate transform from polar to cartesian */
    std::vector <float> c;
                /*! indices for coordinate transform from polar to cartesian */
    std::vector <unsigned long int> indices,
                                             /*! table of \f$x^2\f$ function */
                                    x2_table;
                            /*! table for \f$\frac{1}{sinc^2(x)}\f$ function */
    std::vector <double> sinc_table;
    FFT *fft_rho_expand,                     /*!< FFT for padded projections */
        *fft2d;                                         /*!< iFFT for images */
                                                         // DIFT reconstruction
    void calc_dift(float * const, float * const, const unsigned short int,
                   float * const, const unsigned short int,
                   const unsigned short int, const bool);
                                      // correct for distortions and mask image
    void correct_image(float *, float * const);
                                   // distribute projection into cartesian grid
    void distribute(const std::vector<float>, float * const,
                    const unsigned long int *, const float *) const;
                                   // padding, FFT and shift of two projections
    std::vector <float> dift_pad_ft(const float * const, const bool) const;
                                        // padding, FFT and shift of projection
    void dift_pft(const float * const, float *,
                  const std::vector <float>) const;
              // create tables for coordinate transform from polar to cartesian
    void dift_pol_cartes(std::vector <float> * const,
                         std::vector <unsigned long int> * const) const;
                         // create table for \f$\frac{1}{sinc^2(x)}\f$ function
    void init_sinc_table();
                                               // create table for exp-function
    std::vector <float> init_shift_table() const;
                                                 // shift image by one quadrant
    void shift_image(const float * const, float * const) const;
   public:
                                                           // initialize object
    DIFT(const unsigned short int, const unsigned short int, const float,
         const unsigned short int);
    ~DIFT();                                                  // destroy object
                                    // DIFT reconstruction with multi-threading
    float *reconstruct(float *, unsigned short int, const unsigned short int,
                       float * const, const unsigned short int);
 };

#endif
