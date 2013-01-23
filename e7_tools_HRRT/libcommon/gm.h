/*! \file gm.h
    \brief This class stores information about the gantry model.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/08/05 initial version
    \date 2004/08/12 comparison of float values with some tolerance
    \date 2004/09/28 relaxed comparison of gantry models when number known
 */

#ifndef _GM_H
#define _GM_H

#include <string>
#include <vector>
#include "types.h"

/*- class definitions -------------------------------------------------------*/

class GM
 { public:
                                    /*! different detector crystal materials */
    typedef enum { LSO,                /*!< LSO (lutetium oxyortho-silicate) */
                   BGO,                   /*!< BGO (bismuth germanium oxyde) */
                   NAI,                             /*!< NaI (sodium iodide) */
                   LYSO       /*!< LYSO (lutetium yttrium oxyortho-silicate) */
                 } tcrystal_mat;
   private:
    static GM *instance;         /*!< pointer to only instance of this class */
    bool values_stored; /*!< are gantry model values stored in this object ? */
    std::string model_number;                       /*!< gantry model number */
    unsigned short int iRhoSamples,        /*!< number of bins in projection */
                                 /*! number of projections in sinogram plane */
                       iThetaSamples,
                       ispan,                          /*!< span of sinogram */
                       imrd,                    /*!< maximum ring difference */
                                 /*! number of transaxial crystals per block */
                       transaxial_crystals_per_block,
                     /*! number of blocks per bucket in transaxial direction */
                       transaxial_blocks_per_bucket,
                          /*! number of blocks per bucket in axial direction */
                       axial_blocks_per_bucket,
                         /*! number of crystals per block in axial direction */
                       axial_crystals_per_block,
                       crystals_per_ring,   /*!< number of crystals per ring */
                               /*! gap crystals per block in axial direction */
                       axial_block_gaps,
                          /*! gap crystals per block in transaxial direction */
                       transaxial_block_gaps,
                       bucket_rings,             /*!< number of bucket rings */
                       buckets_per_ring,     /*!< number of buckets per ring */
                       illd,        /*!< lower level energy threshold in keV */
                       iuld,        /*!< upper level energy threshold in keV */
                       crystal_layers,         /*!< number of crystal layers */
                       crystal_rings;           /*!< number of crystal rings */
    float iBinSizeRho,                                   /*!< bin size in cm */
          plane_separation,                         /*!< plane spacing in cm */
          ring_radius,                                /*!< ring radius in cm */
          doi,                               /*!< depth of interaction in cm */
          intrinsic_tilt,                     /*!< intrinsic tilt in degrees */
          max_scatter_zfov;        /*!< maximum z-FOV for scatter estimation */
    bool ring_architecture,                    /*!< ring or panel detector ? */
                                   /*! sinogram needs axial arc-correction ? */
         needs_axial_arc_correction,
         is_pet_ct;                   /*!< is scanner a PET/CT or PET only ? */
                                                  /*! crystal layer material */
    std::vector <GM::tcrystal_mat> crystal_layer_material;
                                            /*! depth of crystal layer in cm */
    std::vector <float> crystal_layer_depth,
                         /*! FWHM of energy resolution of crystal layer in % */
                        crystal_layer_fwhm_ergres,
                          /*! background energie ratio of crystal layer in % */
                        crystal_layer_background_ergratio;
                                                        // is a value defined ?
    template <typename T>
     std::string def(T v) const;
                                     // is a unsigned short int value defined ?
    std::string def(unsigned short int) const;
    bool equal(const float, const float) const; // are two float values equal ?
   protected:
    GM();                                                      // create object
   public:
                 // request number of gap crystals per block in axial direction
    unsigned short int axialBlockGaps() const;
                                   // request number of axial blocks per bucket
    unsigned short int axialBlocksPerBucket() const;
                                  // request number of axial crystals per block
    unsigned short int axialCrystalsPerBlock() const;
                                                      // request bin size in mm
    float BinSizeRho() const;
    unsigned short int bucketRings() const;   // request number of bucket rings
                                          // request number of buckets per ring
    unsigned short int bucketsPerRing() const;
    static void close();                                      // close log file
                       // request background energy ratio of crystal layer in %
    float crystalLayerBackgroundErgRatio(const unsigned short int) const;
                                        // request depth of crystal layer in mm
    float crystalLayerDepth(const unsigned short int) const;
                     // request FWHM of energy resolution of crystal layer in %
    float crystalLayerFWHMErgRes(const unsigned short int) const;
                                           // request material of crystal layer
    GM::tcrystal_mat crystalLayerMaterial(const unsigned short int) const;
                                            // request number of crystal layers
    unsigned short int crystalLayers() const;
                                         // request number of crystals per ring
    unsigned short int crystalsPerRing() const;
    float DOI() const;                    // request depth of interaction in mm
    static GM *gm();                       // get pointer to instance of object
    void init(const std::string);        // initialize from gantry model number
                                     // initialize from gantry model parameters
    void init(const unsigned short int, const unsigned short int, const float,
              const unsigned short int, const float, const float, const float,
              const unsigned short int, const unsigned short int, const float,
              const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int,
              const std::vector <GM::tcrystal_mat>,
              const std::vector <float>, const std::vector <float>,
              const std::vector <float>, const float, const bool, const bool,
              const bool, const unsigned short int, const unsigned short int);
    bool initialized() const;                        // is object initialized ?
    float intrinsicTilt() const;           // request intrinsic tilt in degress
    bool isPETCT() const;                              // is scanner a PET/CT ?
                                 // request lower level energy threshold in keV
    unsigned short int lld() const;
                                // request maximum z-FOV for scatter estimation
    float maxScatterZfOV() const;
    std::string modelNumber() const;          // request model number of gantry
                                 // request maximum ring difference of sinogram
    unsigned short int mrd() const;
                                 // does scanner need an axial arc-correction ?
    bool needsAxialArcCorrection() const;
    float planeSeparation() const;               // request plane spacing in mm
    void printUsedGMvalues();                      // print gantry model values
                                        // request number of bins in projection
    unsigned short int RhoSamples() const;
    bool ringArchitecture() const;           // has scanner ring architecture ?
    float ringRadius() const;                      // request ring radius in mm
    unsigned short int rings() const;        // request number of crystal rings
    unsigned short int span() const;                // request span of sinogram
                             // request number of projections in sinogram plane
    unsigned short int ThetaSamples() const;
            // request number of gap crystals per block in transaxial direction
    unsigned short int transaxialBlockGaps() const;
                              // request number of transaxial blocks per bucket
    unsigned short int transaxialBlocksPerBucket() const;
                             // request number of transaxial crystals per block
    unsigned short int transaxialCrystalsPerBlock() const;
                                 // request upper level energy threshold in keV
    unsigned short int uld() const;
 };

#ifndef _GM_CPP
#define _GM_TMPL_CPP
#include "gm.cpp"
#endif

#endif
