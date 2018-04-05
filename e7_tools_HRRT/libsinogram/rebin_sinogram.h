/*! \file rebin_sinogram.h
    \brief This class implements the rebinning of a sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
    \date 2005/02/25 use vectors
 */

#pragma once

#include "e7_tools_const.h"
#include "rebin_x.h"

/*- class definitions -------------------------------------------------------*/

class RebinSinogram
 {                              // declare wrapper function as friend and allow
                                // it to call private methods of the class
   friend void *executeThread_RebinSinogram(void *);
   private:
                                     /*! parameters for RebinSinogram thread */
    typedef struct {                    /*!< pointer to RebinSinogram object */
                     RebinSinogram *object;
                     float *sinogram,               /*!< pointer to sinogram */
                           *new_sinogram,  /*!< pointer to rebinned sinogram */
                           *plane_zoom_factor;  /*!< zoom factors for planes */
                     unsigned short int slices;        /*!< number of slices */
#ifdef XEON_HYPERTHREADING_BUG
                     unsigned short int thread_number; /*!< number of thread */
                                  /*! padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  2*sizeof(unsigned short int)-
                                  3*sizeof(float *)-sizeof(RebinSinogram *)];
#endif
                   } tthread_params;
                                 /*! object for rebinning in theta-direction */
    RebinX <float> *angular_rebin;
    unsigned short int width,                /*!< number of bins in sinogram */
                       new_width,        /*!< number of bins after rebinning */
                       height,             /*!< number of angles in sinogram */
                       new_height,     /*!< number of angles after rebinning */
                       dn,                 /*!< number of angles for padding */
                       nangles_padded,   /*!< number of angles after padding */
                                   /*! number of crystals in a detector ring */
                       crystals_per_ring;
    bool preserve_values,                 /*!< preserve values in sinogram ? */
         rebin_x;                           /*!< rebin in radial direction ? */
                                               /*! method for arc correction */
    RebinX <float>::tarc_correction arc_correction;
                                 // rebinning of sinogram in r- and t-direction
    void calc_rebin_sinogram(float * const, float * const,
                             const unsigned short int, float * const);
   public:
                                   // initialize tables for rebinning algorithm
    RebinSinogram(const unsigned short int, const unsigned short int,
                  const unsigned short int, const unsigned short int,
                  const RebinX <float>::tarc_correction,
                  const unsigned short int, const bool);
    ~RebinSinogram();                                         // destroy object
            // rebinning of sinogram in r- and t-direction with multi-threading
    float *calcRebinSinogram(float * const, unsigned short int, float * const,
                             unsigned short int * const,
                             const unsigned short int,
                             const unsigned short int);
 };

