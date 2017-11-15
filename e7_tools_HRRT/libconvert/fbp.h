/*! \file fbp.h
    \brief This class is used to reconstruct images from sinograms by filtered
           backprojection (FBP).
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/04/21 use correct center of rotation
    \date 2004/04/21 use vector code for filtering
    \date 2004/04/21 fix shadow artefact in backprojector
    \date 2004/04/21 added Doxygen style comments
    \date 2004/04/22 added time-of-flight support
    \date 2004/04/22 reduced memory requirements in reconstruct
    \date 2004/05/13 replaced ramp filter
    \date 2004/05/13 added gaussian for time-of-flight reconstruction
    \date 2004/06/29 fix quantitative scaling of image
    \date 2005/01/04 added progress reporting
 */

#ifndef _FBP_H
#define _FBP_H

#include <vector>
#include "bckprj3d.h"
#include "e7_tools_const.h"
#include "fft.h"
#include "image_conversion.h"
#include "sinogram_conversion.h"


/*- class definitions -------------------------------------------------------*/

class FBP
 {                              // declare wrapper function as friend and allow
                                // it to call private methods of the class
   friend void *executeThread_FBP_Filter_planes(void *);
   private:
                                                  /*! parameters for threads */
    typedef struct { FBP *object;                 /*!< pointer to FBP object */
                     float *sinogram,            /*!< pointer to 2d sinogram */
                                    /*! pointer to padded view-mode sinogram */
                           *padded_view_sinogram;
                     unsigned short int z_start,  /*!< first slice to filter */
                                        z_end,     /*!< last slice to filter */
                                        thread_number; /*!< number of thread */
#ifdef XEON_HYPERTHREADING_BUG
                                  /*! padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  3*sizeof(unsigned short int)-
                                  2*sizeof(float *)-sizeof(FBP *)];
#endif
                   } tthread_params;

    unsigned short int XYSamples,    /*!< width/heigt of reconstructed image */
                       RhoSamples,         /*!< number of bins in projection */
                       RhoSamples_TOF, /*!< number of bins in TOF projection */
                                 /*! padded number of bins in TOF projection */
                       RhoSamples_padded_TOF,
                               /*! number of projections in a sinogram plane */
                       ThetaSamples,
                       loglevel;                      /*!< level for logging */
    unsigned long int sino_planesize_TOF,    /*!< size of TOF sinogram plane */
                                    /*!< size of padded view of TOF sinogram */
                      view_size_padded_TOF,
                      image_planesize;              /*!< size of image plane */
    float BinSizeRho,                                 /*!< size of bin in mm */
          DeltaXY,                          /*!< width/height of voxel in mm */
          fov_diameter;      /*!< diameter of FOV after reconstruction in mm */
                                           /*! real values of 1d-ramp filter */
    std::vector <float> filter_kernel_real;
    FFT *fft;                                            /*!< object for FFT */
    BckPrj3D *bckprj;                         /*!< object for backprojection */
                                     // filter sinogram planes with ramp filter
    void filter_planes(float * const, float * const, const unsigned short int,
                       const unsigned short int) const;
                                                           // initialize object
    void init(const unsigned short int, const float, const unsigned short int,
              const unsigned short int, const unsigned short int, const float,
              const float, const float, const float, const unsigned short int,
              const unsigned short int);
    void ramp_filter();                         // calculate ramp filter matrix
   public:
                                                                // constructors
    FBP(const unsigned short int, const float, const unsigned short int,
        const unsigned short int, const unsigned short int, const float,
        const float, const float, const float, const unsigned short int,
        const unsigned short int);
    FBP(SinogramConversion * const, const unsigned short int, const float,
        const float, const unsigned short int);
    ~FBP();                                                   // destroy object
                                    // filtered backprojection (multi-threaded)
    ImageConversion *reconstruct(SinogramConversion * const, const bool,
                                 const unsigned short int, float * const,
                                 const unsigned short int);
    float *reconstruct(float * const, const unsigned short int, float * const,
                       const unsigned short int);
 };

#endif
