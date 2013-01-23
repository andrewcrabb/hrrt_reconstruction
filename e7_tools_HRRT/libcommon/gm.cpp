/*! \class GM gm.h "gm.h"
    \brief This class stores information about the gantry model.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Merence Sibomana (sibomana@gmail.com for HRRT user community)
    \date 2004/08/05 initial version
    \date 2004/08/12 comparison of float values with some tolerance
    \date 2004/09/28 relaxed comparison of gantry models when number known
    \date 2008/04/06 Replaced GantryModel by GantryInfo to avoid dll hell

    This class stores information about the gantry model.
 */

#include <cmath>
#ifndef _GM_CPP
#define _GM_CPP
#include "gm.h"
#endif
#include "exception.h"
#ifdef USE_GANTRY_MODEL
#include "gantryModelClass.h"
#else
#include "gantryinfo.h"
using namespace HRRT;
#endif
#include "logging.h"
#include <stdlib.h>

/*- methods -----------------------------------------------------------------*/
#ifndef _GM_TMPL_CPP

GM *GM::instance=NULL;               /*!< pointer to only instance of object */

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
GM::GM()
 { values_stored=false;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of gap crystals per block in axial direction.
    \return number of gap crystals per block in axial direction.
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of gap crystals per block in axial direction.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::axialBlockGaps() const
 { if (values_stored) return(axial_block_gaps);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of axial blocks per bucket.
    \return number of axial blocks per bucket
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of axial blocks per bucket.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::axialBlocksPerBucket() const
 { if (values_stored) return(axial_blocks_per_bucket);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of axial crystals per block.
    \return number of axial crystals per block
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of axial crystals per block.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::axialCrystalsPerBlock() const
 { if (values_stored) return(axial_crystals_per_block);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request bin size.
    \return bin size in mm
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request bin size in mm.
 */
/*---------------------------------------------------------------------------*/
float GM::BinSizeRho() const
 { if (values_stored) return(iBinSizeRho*10.0f);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bucket rings.
    \return number of bucket rings
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of bucket rings.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::bucketRings() const
 { if (values_stored) return(bucket_rings);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of buckets per ring.
    \return number of buckets per ring
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of buckets per ring.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::bucketsPerRing() const
 { if (values_stored) return(buckets_per_ring);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete instance of object.

    Delete instance of object.
 */
/*---------------------------------------------------------------------------*/
void GM::close()
 { if (instance != NULL) { delete instance;
                           instance=NULL;
                         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request background energy ratio of crystal layer.
    \param[in] layer   number of crystal layer
    \return background energy ratio of crystal layerin %
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request background energy ratio of crystal layer in %.
 */
/*---------------------------------------------------------------------------*/
#ifdef USE_GANTRY_MODEL
float GM::crystalLayerBackgroundErgRatio(const unsigned short int layer) const
 { if (values_stored && (crystal_layer_background_ergratio.size() > layer))
    return(crystal_layer_background_ergratio[layer]);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }
#else
float GM::crystalLayerBackgroundErgRatio(const unsigned short int layer) const
 { 
   return GantryInfo::crystalLayerBackgroundErgRatio(layer);
 }
#endif
/*---------------------------------------------------------------------------*/
/*! \brief Request depth of crystal layer.
    \param[in] layer   number of crystal layer
    \return depth of crystal layer in mm
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request depth of crystal layer in mm.
 */
/*---------------------------------------------------------------------------*/
float GM::crystalLayerDepth(const unsigned short int layer) const
 { if (values_stored && (crystal_layer_depth.size() > layer))
    return(crystal_layer_depth[layer]*10.0f);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request FWHM of energy resolution of crystal layer.
    \param[in] layer   number of crystal layer
    \return FWHM of energy resolution of crystal layer in %
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request FWHM of energy resolution of crystal layer in %.
 */
/*---------------------------------------------------------------------------*/
float GM::crystalLayerFWHMErgRes(const unsigned short int layer) const
 { if (values_stored && (crystal_layer_fwhm_ergres.size() > layer))
    return(crystal_layer_fwhm_ergres[layer]);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }


/*---------------------------------------------------------------------------*/
/*! \brief Request material of crystal layer.
    \param[in] layer   number of crystal layer
    \return material of crystal layer
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request material of crystal layer.
 */
/*---------------------------------------------------------------------------*/
GM::tcrystal_mat GM::crystalLayerMaterial(
                                          const unsigned short int layer) const
 { if (values_stored && (crystal_layer_material.size() > layer))
    return(crystal_layer_material[layer]);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of crystal layers.
    \return number of crystal layers
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of crystal layers.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::crystalLayers() const
 { if (values_stored) return(crystal_layers);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of crystals per ring.
    \return number of crystals per ring
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of crystals per ring.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::crystalsPerRing() const
 { if (values_stored) return(crystals_per_ring);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Is a value defined ?
    \param[in] v   value
    \return string with value or "unknown"

    Is a value defined ? Only undefined values are negative.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
std::string GM::def(T v) const
 { if (v < 0) return("unknown");
   return(toString(v));
 }

#ifndef _GM_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Is a unsigned short int value defined ?
    \param[in] v   value
    \return string with value or "unknown"

    Is a unsigned short int value defined ? Only undefined values are 65535.
 */
/*---------------------------------------------------------------------------*/
std::string GM::def(unsigned short int v) const
 { if (v == 65535) return("unknown");
   return(toString(v));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request depth of interaction.
    \return depth of interaction in mm
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request depth of interaction in mm.
 */
/*---------------------------------------------------------------------------*/
float GM::DOI() const
 { if (values_stored) return(doi*10.0f);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Are two float values equal ?
    \param[in] a   first float value
    \param[in] b   second float value
    \return are the values equal ?

    Are two float values equal ?
 */
/*---------------------------------------------------------------------------*/
bool GM::equal(const float a, const float b) const
 { return(fabs(a-b) < 1e-4);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to instance of singleton object.
    \return pointer to instance of singleton object

    Request pointer to the only instance of the GM object. The GM object is a
    singleton object. If it is not initialized, the constructor will be called.
 */
/*---------------------------------------------------------------------------*/
GM *GM::gm()
 { if (instance == NULL) instance=new GM();
   return(instance);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object from gantry model number.
    \param[in] model_num   gantry model number
    \exception REC_GANTRY_MODEL the gantry model number is unknown
    \exception REC_GANTRY_MODEL_MATCH the gantry model information doesn't
                                      match to the already stored information
    \exception REC_UNKNOWN_CRYSTAL_MATERIAL material of crystal layer unknown

    Initialize object from gantry model number.
 */
/*---------------------------------------------------------------------------*/
#ifdef USE_GANTRY_MODEL
void GM::init(const std::string model_num)
 { CGantryModel model;

   if (!model.setModel((const char *)model_num.c_str()))
    throw Exception(REC_GANTRY_MODEL,
                    "The gantry model '#1' is unknown.").arg(model_num);
   if (values_stored)
    { if (model_number == "0") model_number=model_num;
       else if (model_number != model_num)
             throw Exception(REC_GANTRY_MODEL_MATCH,
                             "The gantry model information doesn't match.");
#if 0
      // not needed
      if (iRhoSamples != (unsigned short int)model.defaultElements())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (iThetaSamples != (unsigned short int)model.defaultAngles())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(iBinSizeRho, model.binSize()))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(plane_separation, model.planeSep()))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(ring_radius, model.crystalRadius()))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(doi, model.sinogramDepthOfInteraction()))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (ispan != (unsigned short int)model.defPlaneDefSpan3D())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (imrd != (unsigned short int)model.defRingDiffMax())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(intrinsic_tilt, model.sinogramIntrinsicTilt()))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (transaxial_crystals_per_block !=
          (unsigned short int)model.transCrystals())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (transaxial_blocks_per_bucket !=
          (unsigned short int)model.transBlocks())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (axial_blocks_per_bucket != (unsigned short int)model.axialBlocks())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (axial_crystals_per_block !=
          (unsigned short int)model.axialCrystals())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (axial_block_gaps != (unsigned short int)model.axialBlockGaps())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (transaxial_block_gaps != (unsigned short int)model.transBlockGaps())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (bucket_rings != (unsigned short int)model.bucketRings())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (buckets_per_ring != (unsigned short int)model.bucketsPerRing())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (ring_architecture != model.isRingArch())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (is_pet_ct != !model.isPETOnlyArch())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      switch (atoi(model_number.c_str()))
       { case 921:
         case 922:
         case 923:
         case 924:
         case 1023:
         case 1024:
         case 1080:
         case 1090:
          if (!needs_axial_arc_correction)
           throw Exception(REC_GANTRY_MODEL_MATCH,
                           "The gantry model information doesn't match.");
          break;
         default:
          if (needs_axial_arc_correction)
           throw Exception(REC_GANTRY_MODEL_MATCH,
                           "The gantry model information doesn't match.");
          break;
       }
      if (illd != (unsigned short int)model.defaultLLD())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (iuld != (unsigned short int)model.defaultULD())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (crystal_layers != (unsigned short int)model.nCrystalLayers())
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(max_scatter_zfov, model.maxScatterZfOV()))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      for (unsigned short int i=0; i < crystalLayers(); i++)
       { GM::tcrystal_mat mat;
         std::string material;

         if (!equal(crystal_layer_depth[i], model.crystalLayerDepth(i)))
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
         material=model.crystalLayerMaterial(i);
         if (material == "LSO") mat=LSO;
          else if (material == "BGO") mat=BGO;
          else if (material == "NaI") mat=NAI;
          else if (material == "LYSO") mat=LYSO;
          else throw Exception(REC_UNKNOWN_CRYSTAL_MATERIAL,
                               "Material of crystal layer unknown.");
         if (crystal_layer_material[i] != mat)
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
         if (!equal(crystal_layer_fwhm_ergres[i],
                    model.crystalLayerFWHMErgRes(i)))
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
         if (!equal(crystal_layer_background_ergratio[i],
                    model.crystalLayerBackgroundErgRatio(i)))
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
       }
#endif
    }
    else { model_number=model_num;
           iRhoSamples=model.defaultElements();
           iThetaSamples=model.defaultAngles();
           iBinSizeRho=model.binSize();
           plane_separation=model.planeSep();
           ring_radius=model.crystalRadius();
           crystal_rings=model.directPlanes();
           doi=model.sinogramDepthOfInteraction();
           ispan=model.defPlaneDefSpan3D();
           imrd=model.defRingDiffMax();
           intrinsic_tilt=model.sinogramIntrinsicTilt();
           transaxial_crystals_per_block=model.transCrystals();
           transaxial_blocks_per_bucket=model.transBlocks();
           axial_blocks_per_bucket=model.axialBlocks();
           axial_crystals_per_block=model.axialCrystals();
           axial_block_gaps=model.axialBlockGaps();
           transaxial_block_gaps=model.transBlockGaps();
           bucket_rings=model.bucketRings();
           buckets_per_ring=model.bucketsPerRing();
                                // flat panel scanners don't have crystal rings
           switch (atoi(model_number.c_str()))
            { case 328:
               crystals_per_ring=256*2;
               break;
              case 393:
              case 395:
              case 2393:
              case 2395:
               crystals_per_ring=256*2;
               break;
              default:
               crystals_per_ring=(transaxial_crystals_per_block+
                                  transaxial_block_gaps)*
                                 transaxial_blocks_per_bucket*buckets_per_ring;
               break;
            }
           ring_architecture=model.isRingArch();
           is_pet_ct=!model.isPETOnlyArch();
           switch (atoi(model_number.c_str()))
            { case 921:
              case 922:
              case 923:
              case 924:
              case 1023:
              case 1024:
              case 1080:
              case 1090:
               needs_axial_arc_correction=true;
               break;
              default:
               needs_axial_arc_correction=false;
               break;
            }
           illd=model.defaultLLD();
           iuld=model.defaultULD();
           crystal_layers=model.nCrystalLayers();
           crystal_layer_material.resize(crystal_layers);
           crystal_layer_depth.resize(crystal_layers);
           crystal_layer_fwhm_ergres.resize(crystal_layers);
           crystal_layer_background_ergratio.resize(crystal_layers);
           for (unsigned short int i=0; i < crystal_layers; i++)
            { GM::tcrystal_mat mat;
              std::string material;

              material=model.crystalLayerMaterial(i);
              if (material == "LSO") mat=LSO;
               else if (material == "BGO") mat=BGO;
               else if (material == "NaI") mat=NAI;
               else if (material == "LYSO") mat=LYSO;
               else throw Exception(REC_UNKNOWN_CRYSTAL_MATERIAL,
                                    "Material of crystal layer unknown.");
              crystal_layer_material[i]=mat;
              crystal_layer_depth[i]=model.crystalLayerDepth(i);
              crystal_layer_fwhm_ergres[i]=model.crystalLayerFWHMErgRes(i);
              crystal_layer_background_ergratio[i]=
               model.crystalLayerBackgroundErgRatio(i);
            }
           max_scatter_zfov=model.maxScatterZfOV();
           values_stored=true;
         }
 }
#else // USE_GANTRY_MODEL
void GM::init(const std::string model_num)
{ 
	if (GantryInfo::load(atoi(model_num.c_str())) == 0)
    throw Exception(REC_GANTRY_MODEL,
                    "The gantry model '#1' is unknown.").arg(model_num);
   if (values_stored)
    { if (model_number == "0") model_number=model_num;
       else if (model_number != model_num)
             throw Exception(REC_GANTRY_MODEL_MATCH,
                             "The gantry model information doesn't match.");
   }
    else { model_number=model_num;
           iRhoSamples=GantryInfo::defaultElements();
           iThetaSamples=GantryInfo::defaultAngles();
           iBinSizeRho=GantryInfo::binSize()/10; //cm
           plane_separation=GantryInfo::planeSep()/10; //cm
           ring_radius=GantryInfo::ringRadius()/10; //cm
           crystal_rings=GantryInfo::directPlanes();
           doi=GantryInfo::DOI();
           ispan=GantryInfo::span();
           imrd=GantryInfo::mrd();
           intrinsic_tilt=GantryInfo::intrinsicTilt();
           transaxial_crystals_per_block=GantryInfo::transCrystals();
           transaxial_blocks_per_bucket=GantryInfo::transBlocks();
           axial_blocks_per_bucket=GantryInfo::axialBlocks();
           axial_crystals_per_block=GantryInfo::axialCrystals();
           axial_block_gaps=0;
           transaxial_block_gaps=0;
           bucket_rings=1;
           buckets_per_ring= 0xffff;
                                // flat panel scanners don't have crystal rings
           switch (atoi(model_number.c_str()))
            { case 328:
               crystals_per_ring=256*2;
			   ring_architecture=false;
               break;
              case 393:
              case 395:
              case 2393:
              case 2395:
               crystals_per_ring=256*2;
			   ring_architecture=false;
               break;
              default:
			   ring_architecture=true;
			   crystals_per_ring=(transaxial_crystals_per_block+
                                  transaxial_block_gaps)*
                                 transaxial_blocks_per_bucket*buckets_per_ring;
               break;
            }
		   // petct gantry models:1062,1024,1080,1093,1094
           is_pet_ct=(atoi(model_number.c_str())/10==10);
           switch (atoi(model_number.c_str()))
            { case 921:
              case 922:
              case 923:
              case 924:
              case 1023:
              case 1024:
              case 1080:
              case 1093:
              case 1094:
               needs_axial_arc_correction=true;
               break;
              default:
               needs_axial_arc_correction=false;
               break;
            }
           illd=GantryInfo::defaultLLD();
           iuld=GantryInfo::defaultULD();
           crystal_layers=GantryInfo::crystalLayers();
           crystal_layer_material.resize(crystal_layers);
           crystal_layer_depth.resize(crystal_layers);
           crystal_layer_fwhm_ergres.resize(crystal_layers);
           crystal_layer_background_ergratio.resize(crystal_layers);
           for (unsigned short int i=0; i < crystal_layers; i++)
            { GM::tcrystal_mat mat;

              switch(GantryInfo::crystalLayerMaterial(i))
              {case GantryInfo::LSO: mat=GM::LSO; break;
              case GantryInfo::LYSO: mat=GM::LYSO; break; 
              case GantryInfo::BGO: mat=GM::BGO; break; 
              case GantryInfo::NAI: mat=GM::NAI; break; 
               default:
                   throw Exception(REC_UNKNOWN_CRYSTAL_MATERIAL,
                                    "Material of crystal layer unknown.");
              }
              crystal_layer_material[i]=mat;
              crystal_layer_depth[i]=GantryInfo::crystalLayerDepth(i);
              crystal_layer_fwhm_ergres[i]=GantryInfo::crystalLayerFWHMErgRes(i);
              crystal_layer_background_ergratio[i]=
               GantryInfo::crystalLayerBackgroundErgRatio(i);
            }
           max_scatter_zfov=GantryInfo::maxScatterZfOV()/10; //cm
           values_stored=true;
         }
}
#endif // USE_GANTRY_MODEL

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object from gantry model parameters.
    \param[in] _RhoSamples                      number of bins in projection
    \param[in] _ThetaSamples                    number of projections in
                                                sinogram plane
    \param[in] _BinSizeRho                      bin size in mm
    \param[in] rings                            number of crystal rings
    \param[in] _plane_separation                plane spacing in mm
    \param[in] _ring_radius                     ring radius in mm
    \param[in] _doi                             depth of interaction in mm
    \param[in] _span                            span of sinogram
    \param[in] _mrd                             maximum ring difference
    \param[in] _intrinsic_tilt                  intrinsic tilt in degrees
    \param[in] _transaxial_crystals_per_block   number of transaxial crystals
                                                per block
    \param[in] _transaxial_blocks_per_bucket    number of blocks per bucket in
                                                transaxial direction
    \param[in] _axial_crystals_per_block        number of axial crystals per
                                                block
    \param[in] _axial_blocks_per_bucket         number of blocks per bucket in
                                                axial direction
    \param[in] _transaxial_block_gaps           gap crystals per block in
                                                transaxial direction
    \param[in] _axial_block_gaps                gap crystals per block in
                                                axial direction
    \param[in] _bucket_rings                    number of bucket rings
    \param[in] _buckets_per_ring                number of buckets per ring
    \param[in] _crystals_per_ring               number of crystals per ring
    \param[in] _crystal_layers                  number of crystal layers
    \param[in] _crystal_layer_material          crystal layer material
    \param[in] _crystal_layer_depth             depth of crystal layer in mm
    \param[in] _crystal_layer_fwhm_ergres       FWHM of energy resolution of
                                                crystal layer in %
    \param[in] _crystal_layer_background_ergratio   background energie ratio of
                                                    crystal layer in %
    \param[in] _max_scatter_zfov                maximum z-FOV for scatter
                                                estimation
    \param[in] _ring_architecture               ring or panel detector ?
    \param[in] _is_pet_ct                       is scanner a PET/CT or PET
                                                only ?
    \param[in] _needs_axial_arc_correction      sinogram needs axial
                                                arc-correction ?
    \param[in] _lld                             lower level energy threshold in
                                                keV
    \param[in] _uld                             upper level energy threshold in
                                                keV
    \exception REC_GANTRY_MODEL_MATCH the gantry model information doesn't
                                      match to the already stored information

    Initialize object from gantry model parameters.
 */
/*---------------------------------------------------------------------------*/
void GM::init(const unsigned short int _RhoSamples,
              const unsigned short int _ThetaSamples, const float _BinSizeRho,
              const unsigned short int rings, const float _plane_separation,
              const float _ring_radius, const float _doi,
              const unsigned short int _span, const unsigned short int _mrd,
              const float _intrinsic_tilt,
              const unsigned short int _transaxial_crystals_per_block,
              const unsigned short int _transaxial_blocks_per_bucket,
              const unsigned short int _axial_crystals_per_block,
              const unsigned short int _axial_blocks_per_bucket,
              const unsigned short int _transaxial_block_gaps,
              const unsigned short int _axial_block_gaps,
              const unsigned short int _bucket_rings,
              const unsigned short int _buckets_per_ring,
              const unsigned short int _crystals_per_ring,
              const unsigned short int _crystal_layers,
              const std::vector <GM::tcrystal_mat> _crystal_layer_material,
              const std::vector <float> _crystal_layer_depth,
              const std::vector <float> _crystal_layer_fwhm_ergres,
              const std::vector <float> _crystal_layer_background_ergratio,
              const float _max_scatter_zfov, const bool _ring_architecture,
              const bool _is_pet_ct, const bool _needs_axial_arc_correction,
              const unsigned short int _lld, const unsigned short int _uld)
 { if (values_stored)
    { if (iRhoSamples != _RhoSamples)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (iThetaSamples != _ThetaSamples)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(iBinSizeRho, _BinSizeRho/10.0f))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(plane_separation, _plane_separation/10.0f))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(ring_radius, _ring_radius/10.0f))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(doi, _doi/10.0f))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (ispan != _span)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (imrd != _mrd)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(intrinsic_tilt, _intrinsic_tilt))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (transaxial_crystals_per_block != _transaxial_crystals_per_block)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (transaxial_blocks_per_bucket != _transaxial_blocks_per_bucket)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (axial_crystals_per_block != _axial_crystals_per_block)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (axial_blocks_per_bucket != _axial_blocks_per_bucket)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (transaxial_block_gaps != _transaxial_block_gaps)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (axial_block_gaps != _axial_block_gaps)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (bucket_rings != _bucket_rings)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (buckets_per_ring != _buckets_per_ring)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (crystals_per_ring != _crystals_per_ring)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (ring_architecture != _ring_architecture)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (is_pet_ct != _is_pet_ct)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (needs_axial_arc_correction != _needs_axial_arc_correction)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (illd != _lld)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (iuld != _uld)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (crystal_layers != _crystal_layers)
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      if (!equal(max_scatter_zfov, _max_scatter_zfov/10.0f))
       throw Exception(REC_GANTRY_MODEL_MATCH,
                       "The gantry model information doesn't match.");
      for (unsigned short int i=0; i < crystalLayers(); i++)
       { if (!equal(crystal_layer_depth[i], _crystal_layer_depth[i]/10.0f))
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
         if (crystal_layer_material[i] != _crystal_layer_material[i])
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
         if (!equal(crystal_layer_fwhm_ergres[i],
                    _crystal_layer_fwhm_ergres[i]))
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
         if (!equal(crystal_layer_background_ergratio[i],
                    _crystal_layer_background_ergratio[i]))
          throw Exception(REC_GANTRY_MODEL_MATCH,
                          "The gantry model information doesn't match.");
       }
    }
    else { model_number="0";
           iRhoSamples=_RhoSamples;
           iThetaSamples=_ThetaSamples;
           iBinSizeRho=_BinSizeRho/10.0f;
           plane_separation=_plane_separation/10.0f;
           ring_radius=_ring_radius/10.0f;
           doi=_doi/10.0f;
           ispan=_span;
           imrd=_mrd;
           intrinsic_tilt=_intrinsic_tilt;
           transaxial_crystals_per_block=_transaxial_crystals_per_block;
           transaxial_blocks_per_bucket=_transaxial_blocks_per_bucket;
           axial_crystals_per_block=_axial_crystals_per_block;
           axial_blocks_per_bucket=_axial_blocks_per_bucket;
           transaxial_block_gaps=_transaxial_block_gaps;
           axial_block_gaps=_axial_block_gaps;
           bucket_rings=_bucket_rings;
           buckets_per_ring=_buckets_per_ring;
           crystals_per_ring=_crystals_per_ring;
           ring_architecture=_ring_architecture;
           is_pet_ct=_is_pet_ct;
           needs_axial_arc_correction=_needs_axial_arc_correction;
           illd=_lld;
           iuld=_uld;
           crystal_layers=_crystal_layers;
           max_scatter_zfov=_max_scatter_zfov/10.0f;
           crystal_layer_depth.resize(crystal_layers);
           for (unsigned short int i=0; i < crystal_layers; i++)
            crystal_layer_depth[i]=_crystal_layer_depth[i]/10.0f;
           crystal_layer_material=_crystal_layer_material;
           crystal_layer_fwhm_ergres=_crystal_layer_fwhm_ergres;
           crystal_layer_background_ergratio=
            _crystal_layer_background_ergratio;
           values_stored=true;
           crystal_rings=rings;
           //(axial_crystals_per_block+axial_block_gaps)*
           //            axial_blocks_per_bucket*bucket_rings-axial_block_gaps;
         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Is object initialized ?
    \return object is initialized

    Is object initialized ?
 */
/*---------------------------------------------------------------------------*/
bool GM::initialized() const
 { return(values_stored);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request intrinsic tilt.
    \return intrinsic tilt in degrees
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request intrinsic tilt in degress.
 */
/*---------------------------------------------------------------------------*/
float GM::intrinsicTilt() const
 { if (values_stored) return(intrinsic_tilt);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Is scanner a PET/CT ?
    \return scanner is a PET/CT
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Is scanner a PET/CT ?
 */
/*---------------------------------------------------------------------------*/
bool GM::isPETCT() const
 { if (values_stored) return(is_pet_ct);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request lower level energy threshold.
    \return lower level energy threshold in keV
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request lower level energy threshold in keV.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::lld() const
 { if (values_stored) return(illd);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request maximum z-FOV for scatter estimation.
    \return maximum z-FOV for scatter estimation in mm
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request maximum z-FOV for scatter estimation in mm.
 */
/*---------------------------------------------------------------------------*/
float GM::maxScatterZfOV() const
 { if (values_stored) return(max_scatter_zfov);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request model number of gantry.
    \return model number of gantry
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request model number of gantry.
 */
/*---------------------------------------------------------------------------*/
std::string GM::modelNumber() const
 { if (values_stored) return(model_number);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request maximum ring difference of sinogram.
    \return maximum ring difference of sinogram
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request maximum ring difference of sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::mrd() const
 { if (values_stored) return(imrd);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Does scanner need an axial arc-correction ?
    \return scanner needs an axial arc-correction
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Does scanner need an axial arc-correction ?
 */
/*---------------------------------------------------------------------------*/
bool GM::needsAxialArcCorrection() const
 { if (values_stored) return(needs_axial_arc_correction);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request plane spacing.
    \return plane spacing in mm
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request plane spacing in mm.
 */
/*---------------------------------------------------------------------------*/
float GM::planeSeparation() const
 { if (values_stored) return(plane_separation*10.0f);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print gantry model values into Logging object.

    Print gantry model values into Logging object.
 */
/*---------------------------------------------------------------------------*/
void GM::printUsedGMvalues()
 { Logging *fl;

   fl=Logging::flog();
   fl->logMsg(" ", 0);
   fl->logMsg("gantry model values that may have been used:", 0);
   fl->logMsg("model number=#1", 1)->arg(model_number);
   fl->logMsg("defaultElements()=#1", 1)->arg(def(RhoSamples()));
   fl->logMsg("binSize()=#1cm", 1)->arg(BinSizeRho()/10.0f);
   fl->logMsg("defaultAngles()=#1", 1)->arg(def(ThetaSamples()));
   fl->logMsg("planeSep()=#1cm", 1)->arg(planeSeparation()/10.0f);
   fl->logMsg("crystalRadius()=#1cm", 1)->arg(ringRadius()/10.0f);
   fl->logMsg("sinogramDepthOfInteraction()=#1cm", 1)->
    arg(DOI()/10.0f);
   fl->logMsg("sinogramIntrinsicTilt()=#1 degrees", 1)->
    arg(def(intrinsicTilt()));
   if (ringArchitecture()) fl->logMsg("isRingArch()=true", 1);
    else fl->logMsg("isRingArch()=false", 1);
   if (isPETCT()) fl->logMsg("isPETOnlyArch()=false", 1);
    else fl->logMsg("isPETOnlyArch()=true", 1);
   fl->logMsg("defPlaneDefSpan3D()=#1", 1)->arg(def(span()));
   fl->logMsg("defRingDiffMax()=#1", 1)->arg(def(mrd()));
   fl->logMsg("bucketsPerRing()=#1", 1)->arg(def(bucketsPerRing()));
   fl->logMsg("bucketRings()=#1", 1)->arg(def(bucketRings()));
   fl->logMsg("axialBlocks()=#1", 1)->arg(def(axialBlocksPerBucket()));
   fl->logMsg("transBlocks()=#1", 1)->arg(def(transaxialBlocksPerBucket()));
   fl->logMsg("axialCrystals()=#1", 1)->arg(def(axialCrystalsPerBlock()));
   fl->logMsg("transCrystals()=#1", 1)->arg(def(transaxialCrystalsPerBlock()));
   fl->logMsg("axialBlockGaps()=#1", 1)->arg(def(axialBlockGaps()));
   fl->logMsg("transBlockGaps()=#1", 1)->arg(def(transaxialBlockGaps()));
   fl->logMsg("directPlanes()=#1", 1)->arg(rings());
   fl->logMsg("defaultLLD()=#1 keV", 1)->arg(def(lld()));
   fl->logMsg("defaultULD()=#1 keV", 1)->arg(def(uld()));
   fl->logMsg("nCrystalLayers()=#1", 1)->arg(def(crystalLayers()));
   for (unsigned short int i=0; i < crystalLayers(); i++)
    { std::string str;

      switch (crystalLayerMaterial(i))
       { case LSO:  str="LSO";
                    break;
         case BGO:  str="BGO";
                    break;
         case NAI:  str="NaI";
                    break;
         case LYSO: str="LYSO";
                    break;
       }
      fl->logMsg("crystalLayerMaterial(#1)=#2", 2)->arg(i)->arg(str);
      fl->logMsg("crystalLayerBackgroundErgRatio(#1)=#2%", 2)->
       arg(i)->arg(def(crystalLayerBackgroundErgRatio(i)));
      fl->logMsg("crystalLayerFWHMErgRes(#1)=#2%", 2)->arg(i)->
       arg(def(crystalLayerFWHMErgRes(i)));
      fl->logMsg("crystalLayerDepth(#1)=#2cm", 2)->arg(i)->
       arg(crystalLayerDepth(i)/10.0f);
    }
   fl->logMsg("maxScatterZfOV()=#1cm", 1)->arg(def(maxScatterZfOV()));
   fl->logMsg(" ", 0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bins in projection.
    \return number of bins in projection
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of bins in projection.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::RhoSamples() const
 { if (values_stored) return(iRhoSamples);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Has scanner ring architecture ?
    \return scanner has ring architecture
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Has scanner ring architecture ?
 */
/*---------------------------------------------------------------------------*/
bool GM::ringArchitecture() const
 { if (values_stored) return(ring_architecture);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request ring radius.
    \return ring radius in mm
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request ring radius in mm.
 */
/*---------------------------------------------------------------------------*/
float GM::ringRadius() const
 { if (values_stored) return(ring_radius*10.0f);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of crystal rings.
    \return number of crystal rings
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of crystal rings.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::rings() const
 { if (values_stored) return(crystal_rings);
   //    return((axial_crystals_per_block+axial_block_gaps)*
   //           axial_blocks_per_bucket*bucket_rings-axial_block_gaps);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request span of sinogram.
    \return span of sinogram
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request span of sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::span() const
 { if (values_stored) return(ispan);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of projections in sinogram plane.
    \return number of projections in sinogram plane
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of projections in sinogram plane.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::ThetaSamples() const
 { if (values_stored) return(iThetaSamples);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of gap crystals per block in transaxial direction.
    \return number of gap crystals per block in transaxial direction.
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of gap crystals per block in transaxial direction.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::transaxialBlockGaps() const
 { if (values_stored) return(transaxial_block_gaps);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of transaxial blocks per bucket.
    \return number of transaxial blocks per bucket
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of transaxial blocks per bucket.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::transaxialBlocksPerBucket() const
 { if (values_stored) return(transaxial_blocks_per_bucket);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of transaxial crystals per block.
    \return number of transaxial crystals per block
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request number of transaxial crystals per block.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::transaxialCrystalsPerBlock() const
 { if (values_stored) return(transaxial_crystals_per_block);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request upper level energy threshold.
    \return upper level energy threshold in keV
    \exception REC_GANTRY_MODEL_UNKNOWN_VALUE access to an unknown gantry model
                                              value

    Request upper level energy threshold in keV.
 */
/*---------------------------------------------------------------------------*/
unsigned short int GM::uld() const
 { if (values_stored) return(iuld);
   throw Exception(REC_GANTRY_MODEL_UNKNOWN_VALUE,
                   "Access to an unknown gantry model value.");
 }
#endif
