/*! \file norm_ecat.h
    \brief This class implements the calculation of the normalization sinogram
           for ECAT scanners.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
 */

#ifndef _NORM_ECAT
#define _NORM_ECAT

#include <vector>

/*- class definitions -------------------------------------------------------*/

class NormECAT
 {  private:
                                                     /*! types of norm files */
     typedef enum { NORM_ONE,  /*!< one geometric profile for whole sinogram */
                    NORM_2D,    /*!< one geometric profile for each 2d plane */
                    NORM_3D     /*!< one geometric profile for each 3d plane */
                  } tnorm_type;
                     /*! number of planes per axis of normalization sinogram */
     std::vector <unsigned short int> axis_slices;
     unsigned short int span,                          /*!< span of sinogram */
                                    /*! maximum ring difference for sinogram */
                        max_ring_difference,
                        number_of_rings,     /*!< number of rings in scanner */
                              /*! number of planes in normalization sinogram */
                        axes_slices,
                        RhoSamples,       /*!< number of bins per projection */
                        ThetaSamples,      /*!< number of angles in sinogram */
                        mash,                   /*!< mash factor of sinogram */
                    /*! number of crystals per block in transaxial direction */
                        transaxial_crystals_per_block,
                     /*! number of blocks per bucket in transaxial direction */
                        transaxial_blocks_per_bucket,
                         /*! number of crystals per block in axial direction */
                        axial_crystals_per_block,
                        crystals_per_ring;  /*!< number of crystals per ring */
                          /*! array of ring pairs, contributing to axial LOR */
     std::vector <std::vector <unsigned short int> > ring_numbers;
     float *dtime_efficiency,   /*!< deadtime corrected crystal efficiencies */
           *planeGeomCorr,          /*!< data for plane geometric correction */
                                /*! data for crystal interference correction */
           *crystalInterferenceCorr,
                                  /*! data for crystal efficiency correction */
           *crystalEfficiencyCorr;
     tnorm_type norm_type;                                 /*!< type of norm */
                 // calculate table for deadtime corrected crystal efficiencies
     float *calcDeadtimeEfficiency(const std::vector <float>, float * const,
                                   float * const) const;
     void kill_block(const unsigned short int) const;             // kill block
                                   // calculate plane of normalization sinogram
     void normPlane(float * const, const std::vector <unsigned short int>,
                    float * const) const;
    public:
                                      // initialize object and calculate tables
     NormECAT(const unsigned short int, const unsigned short int,
              const std::vector <unsigned short int>,
              const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int,
              float * const, const unsigned short int, float * const,
              float * const, float * const, float * const,
              const std::vector <float>, const unsigned short int,
              unsigned short int * const);
     ~NormECAT();                                             // destroy object
                                            // calculate normalization sinogram
     unsigned short int calcNorm(const unsigned short int) const;
 };

#endif
