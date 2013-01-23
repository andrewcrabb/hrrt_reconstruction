/*! \file sino2d_3d.h
    \brief This class implements the fourier projection algorithm to convert a
           2d sinogram into a 3d sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
 */

#ifndef _SINO2D_3D_H
#define _SINO2D_3D_H

#include <string>
#include <vector>
#include "fft.h"
#include "sinogram_conversion.h"

/*- class definitions -------------------------------------------------------*/

class Sino2D_3D
 {                              // declare wrapper function as friend and allow
                                // it to call private methods of the class
   friend void *executeThread_CalcSino3D_Axis(void *);
   private:
                                         /*! parameters for Sino2D_3D thread */
    typedef struct { Sino2D_3D *object;     /*!< pointer to Sino2D_3D object */
                     float *sino_3d_axis;/*!< pointer to 3d sinogram of axis */
                     unsigned long int axis_size;  /*!< size of axis dataset */
                                                /*! start plane of this axis */
                     unsigned short int startplane,
                                               /*! number of planes for axis */
                                        axis_planes,
                                        rd,     /*!< ring difference of axis */
                                        axis,            /*!< number of axis */
                                /*! maximum number of threads to use for FFT */
                                        max_threads;
                     float *sino_2d_fft;          /*!< 3d-FFT of 2d sinogram */
                     bool transmission;    /*!< transmission ? (or emission) */
                     unsigned short int thread_number; /*!< number of thread */
#ifdef XEON_HYPERTHREADING_BUG
                                  /*! padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  7*sizeof(unsigned short int)-
                                  2*sizeof(float *)-sizeof(unsigned long int)-
                                  sizeof(bool)-sizeof(Sino2D_3D *)];
#endif
                   } tthread_params;
                                /*! number of planes for axes of 3d sinogram */
    std::vector <unsigned short int> axis_slices;
    unsigned short int RhoSamples,        /*!< number of bins in projections */
                                     /*! number of bins in padded projection */
                       RhoSamples_padded,
                       ThetaSamples,      /*!< number of angles in sinograms */
                                    /*!< number of planes in padded sinogram */
                       planes_padded,
                       rs_2,         /*!< half number of bins in projections */
                       loglevel;                  /*!< top level for logging */
    unsigned long int sino_plane_size,           /*!< size of sinogram plane */
                                           /*! size of padded sinogram plane */
                      sino_plane_size_padded,
                      sinosize_padded;          /*!< size of padded sinogram */
    float ring_diameter,      /*!< effective diameter of detector ring in mm */
          ring_spacing,        /*!< distance between to detector rings in mm */
          BinSizeRho;                   /*!< width of bins in sinogram in mm */
    FFT *fft;                                         /*!< object for 3d-FFT */
                  // calculate 3d sinogram from 3d-FFT of stack of 2d sinograms
    void CalcSino3D_Axis(float * const, const unsigned long int,
                         const unsigned short int, const unsigned short int,
                         const unsigned short int, const unsigned short int,
                         float * const, const bool, const unsigned short int,
                         const unsigned short int, const bool) const;
                                                           // initialize object
    void init(const unsigned short int, const unsigned short int,
              const std::vector <unsigned short int>, const float, const float,
              const unsigned short int);
        // calculate 3d FFT of 3d sinogram from 3d FFT of stack of 2d sinograms
    void exact_fore(float * const, float * const,
                    const unsigned short int) const;
                                                 // calculate weighting of axis
    void Weighting(float * const, const unsigned long int,
                   const unsigned short int) const;
   public:
                                                                // constructors
    Sino2D_3D(const unsigned short int, const unsigned short int,
              const std::vector <unsigned short int>, const float, const float,
              const unsigned short int);
    Sino2D_3D(SinogramConversion * const,
              const std::vector <unsigned short int>,
              const unsigned short int);
    ~Sino2D_3D();                                                 // destructor
                                      // calculate 3d sinogram from 2d sinogram
    void calcSino3D(SinogramConversion * const, const unsigned short int,
                    const std::string, const unsigned short int);
 };

#endif
