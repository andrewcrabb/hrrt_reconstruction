/*! \file randoms_smoothing.h
    \brief This class implements the smoothing of a randoms sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/12/22 initial version
    \date 2003/12/31 added multi-threading
    \date 2004/09/13 added Doxygen style comments
 */

#ifndef _RANDOMS_SMOOTHING_H
#define _RANDOMS_SMOOTHING_H

#include <vector>
#include "const.h"
#include "semaphore_al.h"

/*- class definitions -------------------------------------------------------*/

class RandomsSmoothing
 {                             // declare wrapper functions as friend and allow
                               // them to call private methods of the class
    friend void *executeThread_RS_createSino(void *);
    friend void *executeThread_RS_total(void *);
   private:
                         /*! parameters for randoms rate calculation threads */
    typedef struct {                  /*! pointer to RandomsSmoothing object */
                     RandomsSmoothing *object;
                     unsigned short int first_ring,/*!< number of first ring */
                                        last_ring,  /*!< number of last ring */
                                            /*! number of bins in projection */
                                        RhoSamples,
                               /*! number of projections in a sinogram plane */
                                        ThetaSamples,
                                        mash,   /*!< mash factor of sinogram */
                                    /*! number of crystals per detector ring */
                                        crystals_per_ring,
                                     /*! number of crystal to skip in radial
                                         and axial direction due to gaps     */
                                        skipinterval;
                             /*! number of LORs contributing to each crystal */
                     std::vector <unsigned short int> *cnt;
                     float *sinogram;                  /*!< randoms sinogram */
                     std::vector <float> *singles;        /*!< singles rates */
                     std::vector <float> *epsilon;        /*!< randoms sens  */
                     unsigned short int thread_number; /*!< number of thread */
#ifdef XEON_HYPERTHREADING_BUG
                                 /*!< padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  8*sizeof(unsigned short int)-
                                  sizeof(unsigned short int *)-
                                  2*sizeof(float *)-
                                  sizeof(RandomsSmoothing *)];
#endif
                   } tthread_rs_params;
                                /*! parameters for randoms smoothing threads */
    typedef struct {                  /*! pointer to RandomsSmoothing object */
                     RandomsSmoothing *object;
                                                   /*! number of first plane */
                     unsigned short int first_plane,
                                        last_plane,/*!< number of last plane */
                                            /*! number of bins in projection */
                                        RhoSamples,
                               /*! number of projections in a sinogram plane */
                                        ThetaSamples,
                                        mash,   /*!< mash factor of sinogram */
                                    /*! number of crystals per detector ring */
                                        crystals_per_ring,
                                     /*! number of crystal to skip in radial
                                         and axial direction due to gaps     */
                                        skipinterval;
                     float *sinogram;                  /*!< randoms sinogram */
                     std::vector <float> *singles;        /*!< singles rates */
                     std::vector <float> *epsilon;        /*!< randoms sens  */
                     unsigned short int thread_number; /*!< number of thread */
#ifdef XEON_HYPERTHREADING_BUG
                                 /*!< padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  8*sizeof(unsigned short int)-
                                  2*sizeof(float *)-
                                  sizeof(RandomsSmoothing *)];
#endif
                   } tthread_cs_params;

/*             Start Mu         */
    bool flag_diamond_method; // diamonds has few counts
    unsigned short int crystal_in_each_group;   // number of crystal rings in
                                                // one group
/*             End Mu         */

    unsigned short int crystal_rings;/*!< number of crystal rings in scanner */
                                        /*! plane numbers for all ring pairs */
    std::vector <signed short int> plane_numbers;
                /*! semaphore for parallel calculation of total randoms rate */
    Semaphore *total_sem;
                                                // calculate total randoms rate
    void calculateTotalRandomsRate(const unsigned short int,
                                   const unsigned short int, float * const,
                                   const unsigned short int,
                                   const unsigned short int,
                                   const unsigned short int,
                                   const unsigned short int,
                                   const unsigned short int,
                                   std::vector <float> * const,
                                   std::vector <float> * const,
                                   std::vector <unsigned short int> * const,
                                   const unsigned short int, const bool);
                                                    // create smoothed sinogram
    void createSmoothedSinogram(const unsigned short int,
                                const unsigned short int, float * const,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                std::vector <float> * const,
                                std::vector <float> * const,
                                const unsigned short int, const bool) const;
   public:
                                                           // initialize object
    RandomsSmoothing(const std::vector <unsigned short int>,
                     const unsigned short int, const unsigned short int);
    ~RandomsSmoothing();                                      // destroy object
                                                     // smooth randoms sinogram
    void smooth(float * const, const unsigned short int,
                const unsigned short int, const unsigned short int,
                const unsigned short int, const unsigned short int,
                const unsigned short int, const unsigned short int);
/*             Start Mu         */
    unsigned short int number_groups;     // number of groups defined in a ring
/*             End Mu         */
 };

#endif
