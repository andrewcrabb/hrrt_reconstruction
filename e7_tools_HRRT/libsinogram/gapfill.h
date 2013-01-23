/*! \file gapfill.h
    \brief This class provides methods to fill gaps in sinograms and smooth
           randoms.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/03/29 added model based gap filling
    \date 2004/04/08 reduce amount of logging messages
    \date 2004/09/10 mask border bins in norm
 */

#ifndef _GAPFILL_H
#define _GAPFILL_H

#include <vector>
#include "const.h"

/*- class definitions -------------------------------------------------------*/

class GapFill
 {                              // declare wrapper function as friend and allow
                                // it to call private methods of the class
   friend void *executeThread_GapFill(void *);
   public:
                                                   /*! parameter for threads */
    typedef struct { GapFill *object;         /*!< pointer to GapFill object */
                     float *sinogram,               /*!< pointer to sinogram */
                           *norm,         /*!< pointer to normalization data */
                       /*! threshold for normalization data to identify gaps */
                           gap_fill_threshold;
                           /*! number of bins to ignore on each side of norm */
                     unsigned short int mask_bins,
                                        slices,        /*!< number of slices */
                                        thread_number; /*!< number of thread */
#ifdef XEON_HYPERTHREADING_BUG
                                  /*! padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  3*sizeof(unsigned short int)-
                                  sizeof(float *)-sizeof(GapFill *)];
#endif
                   } tthread_params;
   private:
    unsigned short int RhoSamples,        /*!< number of bins in projections */
                       ThetaSamples;      /*!< number of angles in sinograms */
    unsigned long int planesize;               /*!< size of a sinogram plane */
    std::vector <bool> gap_mask;                          /*!< mask for gaps */
                                           /*! indices of 1d sinogram vector */
    std::vector <unsigned long int> m_index;
    void calc_indices();             // calculate indices of 1d sinogram vector
                                           // calculate model based gap filling
    void calcGapFill(float *, const unsigned short int) const;
                                    // calculate normalization base gap filling
    void calcGapFill(float *, const float *, const unsigned short int,
                     const float, const unsigned short int) const;
                                             // 1d boxcar smoothing of sinogram
    void smooth(float * const, const unsigned long int,
                unsigned long int) const;
   public:
                                // initialize object for norm based gap filling
    GapFill(const unsigned short int, const unsigned short int);
                               // initialize object for model based gap filling
    GapFill(const unsigned short int, const unsigned short int,
            const unsigned short int, const unsigned short int,
            const unsigned short int, const float);
    void calcRandomsSmooth(float * const) const; // calculate randoms smoothing
                                         // fill gaps in sinogram (model based)
    void fillGaps(float * const, unsigned short int, const unsigned short int,
                  const unsigned short int);
                                 // fill gaps in sinogram (normalization based)
    void fillGaps(float * const, float * const, unsigned short int,
                  const float, const unsigned short int,
                  const unsigned short int, const unsigned short int);
 };

#endif

