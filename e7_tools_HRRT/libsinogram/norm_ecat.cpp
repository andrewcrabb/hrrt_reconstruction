/*! \class NormECAT norm_ecat.h "norm_ecat.h"
    \brief This class implements the calculation of the normalization sinogram
           for ECAT scanners.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments

    This class implements the calculation of the normalization sinogram from
    a component norm.
 */

#include <iostream>
#include "norm_ecat.h"
#include "e7_common.h"
#include "exception.h"
#include "logging.h"
#include "mem_ctrl.h"

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object and create tables.
    \param[in] _RhoSamples                      number of bins in projections
    \param[in] _ThetaSamples                    number of angles in sinogram
    \param[in] _axis_slices                     number of planes per axis in
                                                sinogram
    \param[in] _transaxial_crystals_per_block   number of crystals per block in
                                                transaxial direction
    \param[in] _transaxial_blocks_per_bucket    number of blocks per bucket in
                                                transaxial direction
    \param[in] _axial_crystals_per_block        number of crystals per block in
                                                axial direction
    \param[in] _crystals_per_ring               number of crystals per ring
    \param[in] _planeGeomCorr                   data for plane geometric
                                                correction
    \param[in] _crystalInterferenceCorr         data for crystal interference
                                                correction
    \param[in] _crystalEfficiencyCorr           data for crystal efficiency
                                                correction
    \param[in] ring_dtcor1                      data for deadtime correction of
                                                crystals
    \param[in] ring_dtcor2                      data for deadtime correction of
                                                crystals
    \param[in] singles                          singles values from emission
    \param[in] kill_blocks_cnt                  number of blocks to kill
    \param[in] kill_blocks                      array of block numbers to kill

    Initialize object and create tables.
 */
/*---------------------------------------------------------------------------*/
NormECAT::NormECAT(const unsigned short int _RhoSamples,
                   const unsigned short int _ThetaSamples,
                   const std::vector <unsigned short int> _axis_slices,
                   const unsigned short int _transaxial_crystals_per_block,
                   const unsigned short int _transaxial_blocks_per_bucket,
                   const unsigned short int _axial_crystals_per_block,
                   const unsigned short int _crystals_per_ring,
                   float * const _planeGeomCorr,
                   const unsigned short int gcp,
                   float * const _crystalInterferenceCorr,
                   float * const _crystalEfficiencyCorr,
                   float * const ring_dtcor1, float * const ring_dtcor2,
                   const std::vector <float> singles,
                   const unsigned short int kill_blocks_cnt,
                   unsigned short int * const kill_blocks)
 { dtime_efficiency=NULL;
   RhoSamples=_RhoSamples;
   ThetaSamples=_ThetaSamples;
   transaxial_crystals_per_block=_transaxial_crystals_per_block;
   transaxial_blocks_per_bucket=_transaxial_blocks_per_bucket;
   axial_crystals_per_block=_axial_crystals_per_block;
   planeGeomCorr=_planeGeomCorr;
   crystalInterferenceCorr=_crystalInterferenceCorr;
   crystalEfficiencyCorr=_crystalEfficiencyCorr;
   axis_slices=_axis_slices;
   crystals_per_ring=_crystals_per_ring;
   mash=crystals_per_ring/2/ThetaSamples;
   axes_slices=0;
   for (unsigned short int axis=0; axis < axis_slices.size(); axis++)
    axes_slices+=axis_slices[axis];
   if (gcp == 1) norm_type=NORM_ONE;
    else if (gcp == axis_slices[0]) norm_type=NORM_2D;
          else norm_type=NORM_3D;
   span=axis_slices[0]-axis_slices[1]/2-1;
   max_ring_difference=((axis_slices.size()*2-1)*span-1)/2;
   number_of_rings=(axis_slices[0]+1)/2;
                    // calculate array of ring pairs, contributing to axial LOR
   ring_numbers=calcRingNumbers(number_of_rings, span, max_ring_difference);
                 // calculate table for deadtime corrected crystal efficiencies
   dtime_efficiency=calcDeadtimeEfficiency(singles, ring_dtcor1, ring_dtcor2);
                      // set deadtime efficiency to 0 for certain "dead" blocks
   for (unsigned short int i=0; i < kill_blocks_cnt; i++)
    { Logging::flog()->logMsg("kill block #1 in norm", 3)->arg(kill_blocks[i]);
      kill_block(kill_blocks[i]);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
NormECAT::~NormECAT()
 { if (dtime_efficiency != NULL) delete[] dtime_efficiency;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate table for deadtime corrected crystal efficiencies.
    \param[in] singles       singles values from emission
    \param[in] ring_dtcor1   data for deadtime correction of crystals
    \param[in] ring_dtcor2   data for deadtime correction of crystals
    \return table with crystal efficiencies

    Calculate table for deadtime corrected crystal efficiencies.
 */
/*---------------------------------------------------------------------------*/
float *NormECAT::calcDeadtimeEfficiency(const std::vector <float> singles,
                                        float * const ring_dtcor1,
                                        float * const ring_dtcor2) const
 { float *dte=NULL;

   try
   { unsigned short int crystals_per_bucket, gantry_crystal, buckets_per_ring;

     crystals_per_bucket=transaxial_blocks_per_bucket*
                         transaxial_crystals_per_block;
     buckets_per_ring=crystals_per_ring/crystals_per_bucket;
     dte=new float[(unsigned long int)number_of_rings*
                   (unsigned long int)crystals_per_ring];
     gantry_crystal=0;
     for (unsigned short int ring=0; ring < number_of_rings; ring++)
      for (unsigned short int crystal=0;
           crystal < crystals_per_ring;
           crystal++,
           gantry_crystal++)
       { unsigned short int bucket;
         float rate;
                                                            // number of bucket
         bucket=crystal/crystals_per_bucket+
                buckets_per_ring*(ring/axial_crystals_per_block);
                       // singles rate for each transaxial block in this bucket
         if (singles.size() > bucket)
          rate=singles[bucket]/(float)transaxial_blocks_per_bucket;
          else rate=1.0f/(float)transaxial_blocks_per_bucket;
         dte[gantry_crystal]=
                     crystalEfficiencyCorr[gantry_crystal]*
                     (1.0f+ring_dtcor1[ring]*rate+ring_dtcor2[ring]*rate*rate);
       }
     return(dte);
   }
   catch (...)
    { if (dte != NULL) delete[] dte;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate normalization sinogram.
    \param[in] loglevel   level for logging
    \return memory controller index of normalization sinogram

    Calculate normalization sinogram. Over the time there have been different
    formats for the component norm file. Some have only one geometric profile
    for all planes of the sinogram, some have one profile for every plane of
    segment 0 and some have one profile for every plane of the 3d sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int NormECAT::calcNorm(const unsigned short int loglevel) const
 { unsigned short int pl=0, norm_idx;
   unsigned long int plane_size;
   float *n3dp, *pgcp, *norm3d;

   plane_size=(unsigned long int)RhoSamples*(unsigned long int)ThetaSamples;
   norm3d=MemCtrl::mc()->createFloat(plane_size*
                                     (unsigned long int)axes_slices,
                                     &norm_idx, "norm", loglevel);
                                                   // init normalization with 1
   for (unsigned long int i=0;
        i < plane_size*(unsigned long int)axes_slices;
        i++)
    norm3d[i]=1.0f;
   n3dp=norm3d;
   pgcp=planeGeomCorr;
   for (unsigned short int axis=0; axis < axis_slices.size(); axis++)
    { unsigned short int segment_slices, p;

      Logging::flog()->logMsg("calculate axis #1 of norm (#2x#3x#4)", 3)->
       arg(axis)->arg(RhoSamples)->arg(ThetaSamples)->arg(axis_slices[axis]);
      if (axis == 0) segment_slices=axis_slices[0];
       else segment_slices=axis_slices[axis]/2;
      switch (norm_type)
       { case NORM_ONE:             // one geometric profile for whole sinogram
                                                            // positive segment
          for (p=0; p < segment_slices; p++,
               n3dp+=plane_size,
               pl++)
           normPlane(n3dp, ring_numbers[pl], pgcp);
          if (axis > 0)                                     // negative segment
           for (p=0; p < segment_slices; p++,
                n3dp+=plane_size,
                pl++)
            normPlane(n3dp, ring_numbers[pl], pgcp);
          break;
         case NORM_2D:      // geometric profiles for all planes of 2d sinogram
          { unsigned short int plane_2d;
            if (axis == 0) plane_2d=0;
             else plane_2d=(axis_slices[0]-axis_slices[axis]/2)/2;
            pgcp=&planeGeomCorr[plane_2d*RhoSamples];
                                                            // positive segment
            for (p=0; p < segment_slices; p++,
                 n3dp+=plane_size,
                 pl++,
                 pgcp+=RhoSamples)
             normPlane(n3dp, ring_numbers[pl], pgcp);
            if (axis > 0)                                   // negative segment
             { pgcp=&planeGeomCorr[plane_2d*RhoSamples];
               for (p=0; p < segment_slices; p++,
                    n3dp+=plane_size,
                    pl++,
                    pgcp+=RhoSamples)
                normPlane(n3dp, ring_numbers[pl], pgcp);
             }
          }
          break;
         case NORM_3D:      // geometric profiles for all planes of 3d sinogram
                                                            // positive segment
          for (p=0; p < segment_slices; p++,
               n3dp+=plane_size,
               pl++,
               pgcp+=RhoSamples)
           normPlane(n3dp, ring_numbers[pl], pgcp);
          if (axis > 0)                                     // negative segment
           for (p=0; p < segment_slices; p++,
                n3dp+=plane_size,
                pl++,
                pgcp+=RhoSamples)
            normPlane(n3dp, ring_numbers[pl], pgcp);
          break;
       }
    }
   MemCtrl::mc()->put(norm_idx);
   return(norm_idx);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Kill block by setting the dead-time-efficiency to zero.
    \param[in] block_num   number of block

    Kill block by setting the dead-time-efficiency to zero.
 */
/*---------------------------------------------------------------------------*/
void NormECAT::kill_block(const unsigned short int block_num) const
 { unsigned short int buckets_per_ring, crystals_per_bucket;
   unsigned long int gantry_crystal=0;

   crystals_per_bucket=transaxial_blocks_per_bucket*
                       transaxial_crystals_per_block;
   buckets_per_ring=crystals_per_ring/crystals_per_bucket;
   for (unsigned short int ring=0; ring < number_of_rings; ring++)
    for (unsigned short int crystal=0; crystal < crystals_per_ring; crystal++,
         gantry_crystal++)
     { unsigned short int block;

       block=crystal/transaxial_crystals_per_block+
             transaxial_blocks_per_bucket*buckets_per_ring*
             (ring/axial_crystals_per_block);
       if (block == block_num) dtime_efficiency[gantry_crystal]=0.0f;
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate plane of normalization sinogram.
    \param[in,out] norm_plane         plane of normalization sinogram
    \param[in]     ring_pair          ring pairs contributing to this plane
    \param[in]     planeGeomCorrPtr   data for plane geometric correction

    Calculate one plane of the normalization sinogram.
 */
/*---------------------------------------------------------------------------*/
void NormECAT::normPlane(float * const norm_plane,
                         const std::vector <unsigned short int> ring_pair,
                         float * const planeGeomCorrPtr) const
 { float *n3dp;

   n3dp=norm_plane;
   for (unsigned short int t=0; t < ThetaSamples; t++)
    { float *geoptr, *interferptr;

      geoptr=&planeGeomCorrPtr[RhoSamples/2];
      interferptr=crystalInterferenceCorr;
      for (signed short int r=-RhoSamples/2; r < RhoSamples/2; r++,
           interferptr+=transaxial_crystals_per_block)
       { float view_efficiency=0.0f;

         for (unsigned short int tm=t*mash; tm < (t+1)*mash; tm++)
          { float lor_efficiency=0.0f;
            unsigned short int crys_a, crys_b;

            crys_a=(tm+(r >> 1)+crystals_per_ring) % crystals_per_ring;
            crys_b=(tm-((r+1) >> 1)+crystals_per_ring+ThetaSamples*mash) %
                   crystals_per_ring;
            for (unsigned short int pair=0; pair < ring_pair.size(); pair+=2)
             lor_efficiency+=
                  dtime_efficiency[crys_a+crystals_per_ring*ring_pair[pair]]*
                  dtime_efficiency[crys_b+crystals_per_ring*ring_pair[pair+1]];
            view_efficiency+=lor_efficiency*
                             interferptr[tm % transaxial_crystals_per_block];
          }
         if (view_efficiency <= 0.0f) *n3dp++=0.0f;
          else *n3dp++/=view_efficiency*geoptr[r];
       }
    }
 }
