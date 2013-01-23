/*! \class SinogramConversion sinogram_conversion.h "sinogram_conversion.h"
    \brief This class provides methods to convert a sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/03/19 initial version
    \date 2004/03/29 added model based gap filling
    \date 2004/03/30 added axial arc-correction
    \date 2004/04/01 added Doxygen style comments
    \date 2004/04/22 reduce memory requirements in divide, multiply, subtract,
                     subtract2D and fillGaps
    \date 2004/04/22 increase performance of FORE and SSRB for TOF data
    \date 2004/04/29 added method to add Poisson noise
    \date 2004/05/06 don't allow radial arc correction on corrected data
    \date 2004/06/08 delete segment table of randoms dataset after subtraction
    \date 2004/07/15 added axial un-arc correction
    \date 2004/09/09 calculate correct axis size after trimming
    \date 2004/09/10 ignore norm border in gap filling
    \date 2004/09/16 added "multiplyRandoms" method
    \date 2004/09/21 added randoms smoothing for TOF data
    \date 2004/09/21 fixed model based gap filling if sinogram is not full size
    \date 2004/09/21 fixed addTOFbins for prompts and randoms data
    \date 2004/09/22 allow to switch off radial and axial arc corrections
    \date 2004/11/11 divide by zero in divide() method changed
    \date 2005/01/04 added progress reporting to sino3D2D()

    This class provides methods to convert a sinogram. Available conversions
    are:
    <ul>
     <li>adding up time-of-flight bins to a single sinogram</li>
     <li>axial arc-correction</li>
     <li>radial arc-correction</li>
     <li>calculate trues from prompt and randoms</li>
     <li>application of lower threshold</li>
     <li>decay- and frame-length correction</li>
     <li>divide sinogram by another sinogram</li>
     <li>convert emission into transmission</li>
     <li>fill gaps normalization or model based</li>
     <li>mask-out detector blocks</li>
     <li>multiply sinogram with another sinogram</li>
     <li>resample sinogram in radial and azimuthal direction</li>
     <li>resample 2d sinogram in z-direction</li>
     <li>rotate sinogram around z-axis</li>
     <li>shuffle time-of-flight data</li>
     <li>convert 2d sinogram into 3d sinogram</li>
     <li>convert 3d sinogram into 2d sinogram</li>
     <li>smooth randoms sinogram</li>
     <li>subtract a 2d sinogram</li>
     <li>subtract a sinogram</li>
     <li>convert transmission into emission</li>
     <li>trim sinogram</li>
     <li>tilt or untilt sinogram</li>
     <li>add Poisson noise</li>
     <li>mask border bins</li>
    </ul>
 */

#include <cstdlib>
#include <algorithm>
#include "sinogram_conversion.h"
#include "const.h"
#include "e7_common.h"
#include "fastmath.h"
#include "fore.h"
#include "gapfill.h"
#include "global_tmpl.h"
#include "progress.h"
#include "randoms_smoothing.h"
#include "rebin_sinogram.h"
#include "rng.h"
#include "sino2d_3d.h"
#include "ssr.h"
#include "types.h"
#include "vecmath.h"

/*- constants ---------------------------------------------------------------*/

                           /*! threshold for normalization based gap-filling */
const float SinogramConversion::gap_fill_threshold=0.5f;

/*---------------------------------------------------------------------------*/
/*! \brief Initialize the object.
    \param[in] si       scan information
    \param[in] sino2d   2d sinogram ? (otherwise: 3d)

    Initialize the object.
 */
/*---------------------------------------------------------------------------*/
SinogramConversion::SinogramConversion(const SinogramIO::tscaninfo si,
                                       const bool sino2d):
 SinogramIO(si, sino2d)
 {
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize the object.
    \param[in] _RhoSamples         number of bins in a projection
    \param[in] _BinSizeRho         width of bins in mm
    \param[in] _ThetaSamples       number of projections in a sinogram plane
    \param[in] _span               span
    \param[in] _mrd                maximum ring difference
    \param[in] _plane_separation   plane separation in mm
    \param[in] rings               number of crystal rings
    \param[in] _intrinsic_tilt     intrinsic tilt in degrees
    \param[in] tilted              is sinogram tilted ?
    \param[in] _lld                lower level energy threshold in keV
    \param[in] _uld                upper level energy threshold in keV
    \param[in] _start_time         start time of scan in sec
    \param[in] _frame_duration     duration of frame in sec
    \param[in] _gate_duration      duration of gate in sec
    \param[in] _halflife           halflife of isotope in sec
    \param[in] _bedpos             horizontal bed position in mm
    \param[in] _mnr                matrix number
    \param[in] sino2d              2d sinogram ? (otherwise: 3d)

    Initialize the object.
 */
/*---------------------------------------------------------------------------*/
SinogramConversion::SinogramConversion(const unsigned short int _RhoSamples,
                       const float _BinSizeRho,
                       const unsigned short int _ThetaSamples,
                       const unsigned short int _span,
                       const unsigned short int _mrd,
                       const float _plane_separation,
                       const unsigned short int rings,
                       const float _intrinsic_tilt, const bool tilted,
                       const unsigned short int _lld,
                       const unsigned short int _uld, const float _start_time,
                       const float _frame_duration, const float _gate_duration,
                       const float _halflife, const float _bedpos,
                       const unsigned short int _mnr, const bool sino2d):
 SinogramIO(_RhoSamples, _BinSizeRho, _ThetaSamples, _span, _mrd,
            _plane_separation, rings, _intrinsic_tilt, tilted, _lld, _uld,
            _start_time, _frame_duration, _gate_duration, _halflife, _bedpos,
            _mnr, sino2d)
 {
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add Poisson noise to sinogram.
    \param[in] trues      the total desired trues in the sinogram
    \param[in] randoms    the total randoms
    \param[in] loglevel   level for logging

    Scale sinogram to desired trues and add the randoms. Then simulate
    poisson noise for this sinogram and for the randoms and subtract the
    noisy randoms from the noisy sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::addPoissonNoise(const double trues,
                                         const double randoms,
                                         const unsigned short int loglevel)
 { RNG *rng=NULL;

   try
   { float rands;
     double sum_sino=0.0;
     unsigned short int s, axis;

     Logging::flog()->logMsg("calculate sum of counts in sinogram", loglevel);
     for (s=0; s < TOFbins(); s++)
      for (axis=0; axis < data[s].size(); axis++)
       { float *sp;
                                                 // get segment of TOF sinogram
         sp=MemCtrl::mc()->getFloatRO(data[s][axis], loglevel);
                                                     // calculate sum of counts
         for (unsigned long int ptr=0; ptr < axis_size[axis]; ptr++)
          sum_sino+=(double)sp[ptr];
         MemCtrl::mc()->put(data[s][axis]);
       }
                                               // calculate randoms per TOF bin
     rands=randoms/((double)size()*(double)TOFbins());
     Logging::flog()->logMsg("scale sinogram to #1 trues and add #2 randoms",
                             loglevel)->arg(trues)->arg(randoms);
     for (s=0; s < TOFbins(); s++)
      for (axis=0; axis < data[s].size(); axis++)
       { float *sp;
                                                 // get segment of TOF sinogram
         sp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
                                       // multiply sinogram with trues/sum_sino
         vecMulScalar(sp, (float)(trues/sum_sino), sp,
                      axis_size[axis]);
                                                    // add randoms to sinogram
         vecAddScalar(sp, rands, sp, axis_size[axis]);
         MemCtrl::mc()->put(data[s][axis]);
       }
     //saveFlat("prompts_nonoise.s", 1);
     rng=new RNG(-time(NULL));
     for (s=0; s < TOFbins(); s++)
      for (axis=0; axis < data[s].size(); axis++)
       { float *sp;

         Logging::flog()->logMsg("add Poisson noise to axis #1 and subtract "
                                 "randoms", loglevel)->arg(axis);
         sp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
                                      // add Poisson noise and subtract randoms
         for (unsigned long int ptr=0; ptr < axis_size[axis]; ptr++)
          { if (sp[ptr] >= 0.0f)
             sp[ptr]=rng->poissonDev(sp[ptr]);
            sp[ptr]-=rng->poissonDev(rands);
          }
         MemCtrl::mc()->put(data[s][axis]);
       }
     delete rng;
     rng=NULL;
   }
   catch (...)
    { if (rng != NULL) delete rng;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add up time-of-flight bins to a single sinogram.
    \param[in] loglevel   level for logging

    Add up time-of-flight bins to a single sinogram. This code works with
    shuffled and planar time-of-flight data.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::addTOFbins(const unsigned short int loglevel)
 { if (TOFbins() < 2) return;
   float *fp=NULL;

   try
   { if (!shuffled_tof_data)
      for (unsigned short int s=1; s < TOFbins(); s++)           // planar data
       { if (data[s].size() > 0)
          Logging::flog()->logMsg("add TOF bin #1 to bin 0", loglevel)->
           arg(s);
         for (unsigned short int axis=0; axis < data[s].size(); axis++)
          { float *rp;

            rp=MemCtrl::mc()->getFloat(data[0][axis], loglevel);
            fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
            vecAdd(rp, fp, rp, axis_size[axis]);
            delete[] fp;
            fp=NULL;
            MemCtrl::mc()->put(data[0][axis]);
          }
         data[s].clear();
       }
       else                                                    // shuffled data
            for (unsigned short int axis=0; axis < axes(); axis++)
             { float *dptr, *fp;
               unsigned short int idx;

               Logging::flog()->logMsg("add up TOF bins in axis #1",
                                       loglevel)->arg(axis);
               dptr=MemCtrl::mc()->createFloat(axis_size[axis]/TOFbins(),
                                               &idx, "emis", loglevel);
               fp=MemCtrl::mc()->getRemoveFloat(&data[0][axis], loglevel);
               for (unsigned long int i=0;
                    i < axis_size[axis]/TOFbins();
                    i++)
                dptr[i]=vecScaledSum(&fp[i*TOFbins()], 1.0f, TOFbins());
               delete[] fp;
               fp=NULL;
               MemCtrl::mc()->put(idx);
               axis_size[axis]/=TOFbins();
               data[0][axis]=idx;
             }
     if (prompts_and_randoms) { data[1]=data[tof_bins];
                                data[tof_bins].clear();
                              }
     tof_bins=1;
     shuffled_tof_data=false;
   }
   catch (...)
    { if (fp != NULL) delete[] fp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Arc-correction of sinogram in axial-direction.
    \param[in] never_correct   don't calculate anything ?
    \param[in] arc_corr        perform arc-correction ? (otherwise: un-arc)
    \param[in] loglevel        level for logging
    \param[in] max_threads     number of threads to use

    Perform an arc-correction of the sinogram in axial direction. The data is
    only corrected once - data that was already corrected will not be corrected
    again. Only sinograms from gantries with tilted block-rings will be
    corrected. This method is multi-threaded. Each thread processes a range
    of sinogram planes.

    The code first calculates the radii of all crystal rings. From this the
    medium radius for each sinogram plane is calculated under consideration of
    the span. Finally the planes are zoomed, so that all planes have the same
    radius.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::axialArcCorrection(const bool never_correct,
                                          const bool arc_corr,
                                          const unsigned short int loglevel,
                                          const unsigned short int max_threads)
 { if (never_correct) return;
   if (!GM::gm()->needsAxialArcCorrection()) return;
   if (arc_corr && (corrections_applied & CORR_Axial_Arc_correction)) return;
   if (!arc_corr && !(corrections_applied & CORR_Axial_Arc_correction))
    return;
   float *fp=NULL;
   RebinSinogram *reb=NULL;

   try
   { unsigned short int i;
     float c3;
     std::vector <float> radius, axial_zoom_factors;
     std::vector <std::vector <unsigned short int> > ring_numbers;

     if (arc_corr)
      Logging::flog()->logMsg("arc-correct sinogram in axial direction",
                              loglevel);
      else Logging::flog()->logMsg("un-arc-correct sinogram in axial "
                                   "direction", loglevel);
     { unsigned short int blocks_per_ring;
       float alf;
                                                   // number of blocks per ring
       blocks_per_ring=GM::gm()->crystalsPerRing()/
                       (GM::gm()->transaxialCrystalsPerBlock()+
                        GM::gm()->transaxialBlockGaps());
       alf=2.0f*M_PI/(float)blocks_per_ring;  // calculate angle of outer rings
                                                // calculate radii of all rings
       c3=GM::gm()->ringRadius()*(1.0f-cosf(3.0f*alf/2.0f)/cosf(alf/2.0f));
     }
     radius.assign(GM::gm()->rings(), GM::gm()->ringRadius());
                              // rings in outer block-rings have smaller radius
     for (i=0;
          i < GM::gm()->axialCrystalsPerBlock()+GM::gm()->axialBlockGaps();
          i++)
      { float v;

        v=(1.0f-(float)i/(float)(GM::gm()->axialCrystalsPerBlock()+
                                 GM::gm()->axialBlockGaps()-1))*




          c3;
        radius[i]-=v;
        radius[GM::gm()->rings()-i-1]-=v;
      }
     axial_zoom_factors.resize(axesSlices());
               // calculate numbers of ring pairs that contribute to each plane
     ring_numbers=calcRingNumbers(GM::gm()->rings(), span(), mrd());
                      // calculate medium radius for each plane and zoom factor
     for (i=0; i < axesSlices(); i++)
      { float rad=0.0f;

        for (unsigned short int j=0; j < ring_numbers[i].size(); j++)
         rad+=radius[ring_numbers[i][j]];
        rad/=(float)ring_numbers[i].size();
        if (arc_corr) axial_zoom_factors[i]=(rad+GM::gm()->DOI())/
                                            (GM::gm()->ringRadius()+
                                             GM::gm()->DOI());
         else axial_zoom_factors[i]=(GM::gm()->ringRadius()+
                                     GM::gm()->DOI())/(rad+GM::gm()->DOI());
      }
                                                        // zoom sinogram planes
     reb=new RebinSinogram(RhoSamples(), ThetaSamples(), RhoSamples(),
                           ThetaSamples(), RebinX <float>::NO_ARC,
                           GM::gm()->crystalsPerRing(), acf_data);
     for (unsigned short int s=0; s < MAX_SINO; s++)
      { unsigned short int p=0;

        for (unsigned short int axis=0; axis < data[s].size(); axis++)
         { float *sp;

           fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
           sp=reb->calcRebinSinogram(fp, axis_slices[axis],
                                     &axial_zoom_factors[p], &data[s][axis],
                                     loglevel, max_threads);
           delete[] fp;
           fp=NULL;
                      // when rebinning an acf sinogram, the resulting sinogram
                      // may have values below 1.0 at the borders
           if (acf_data)
            for (unsigned long int i=0; i < axis_size[axis]; i++)
             if (sp[i] < 1.0f) sp[i]=1.0f;
           MemCtrl::mc()->put(data[s][axis]);
           p+=axis_slices[axis];
         }
      }
     delete reb;
     reb=NULL;
                                                     // set flag for correction
     if (arc_corr) corrections_applied|=CORR_Axial_Arc_correction;
      else corrections_applied^=CORR_Axial_Arc_correction;
   }
   catch (...)
    { if (fp != NULL) delete[] fp;
      if (reb != NULL) delete reb;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the trues from prompt and randoms.
    \param[in] randoms_smoothing   smooth randoms
    \param[in] loglevel            level for logging
    \param[in] max_threads         number of threads to use

    Calculate the trues from prompt and randoms. If the randoms are not yet
    smoothed and the randoms_smoothing parameter is true, the randoms are
    smoothed. This works for panel and ring detectors. Afterwards the
    randoms are subtracted from the prompts to get the trues.
    The randoms smoothing for ring detectors is multi-threaded. Each thread
    processes a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::calculateTrues(const bool randoms_smoothing,
                                        const unsigned short int loglevel,
                                        const unsigned short int max_threads)
 { if (!prompts_and_randoms ||
       (data[0].size() != data[numberOfSinograms()-1].size()) ||
       (data[0].size() == 0)) return;
   RandomsSmoothing *rs=NULL;
   GapFill *gf=NULL;
   float *fp1=NULL;

   try
   { unsigned short int rnd_sino;

     rnd_sino=numberOfSinograms()-1;
     if (randoms_smoothing && !(corrections_applied & CORR_Randoms_Smoothing))
      { Logging::flog()->logMsg("smooth and subtract randoms", loglevel);
                           // calculate randoms smoothing for ring architecture
        if (GM::gm()->ringArchitecture())
         { unsigned short int skip_interval, rnd_idx, axis;
           float *rnd_buffer, *rp;

           if (GM::gm()->transaxialBlockGaps() > 0)
            skip_interval=GM::gm()->transaxialCrystalsPerBlock()+
                          GM::gm()->transaxialBlockGaps();
            else skip_interval=std::numeric_limits <unsigned short int>::max();
           rnd_buffer=MemCtrl::mc()->createFloat(size(), &rnd_idx, "rnd",
                                                 loglevel);
           rp=rnd_buffer;
           for (axis=0; axis < data[rnd_sino].size(); axis++)
            { fp1=MemCtrl::mc()->getRemoveFloat(&data[rnd_sino][axis],
                                                loglevel);
              memcpy(rp, fp1, size(axis)*sizeof(float));
              rp+=size(axis);
              delete[] fp1;
              fp1=NULL;
            }

           rs=new RandomsSmoothing(axis_slices, GM::gm()->rings(), span());
           rs->smooth(rnd_buffer, RhoSamples(), ThetaSamples(), axes_slices,
                      mash(), GM::gm()->crystalsPerRing(), skip_interval,
                      max_threads);
           delete rs;
           rs=NULL;
           if (tof_bins > 1)
            { Logging::flog()->logMsg("scale randoms sinogram by #1",
                                      loglevel+1)->arg(axis)->
               arg(1.0f/(float)tof_bins);
              vecMulScalar(rp, 1.0f/(float)tof_bins, rp, size());
            }
           for (unsigned short int s=0; s < numberOfSinograms()-1; s++)
            { rp=rnd_buffer;
              for (axis=0; axis < axes(); axis++)
               { float *fp0;
                                               // subtract randoms from prompts
                 fp0=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
                 vecSub(fp0, rp, fp0, axis_size[axis]);
                 MemCtrl::mc()->put(data[s][axis]);
                 rp+=size(axis);
               }
            }
           MemCtrl::mc()->put(rnd_idx);
           MemCtrl::mc()->deleteBlock(&rnd_idx);
         }
         else {           // calculate randoms smoothing for panel architecture
                gf=new GapFill(RhoSamples(), ThetaSamples());
 
                for (unsigned short int axis=0; axis < axes(); axis++)
                 { float *fp0;

                   fp1=MemCtrl::mc()->getRemoveFloat(&data[rnd_sino][axis],
                                                     loglevel);
                   fp0=fp1;
                   for (unsigned short int p=0; p < axis_slices[axis]; p++,
                        fp0+=(unsigned long int)RhoSamples()*
                             (unsigned long int)ThetaSamples())
                    gf->calcRandomsSmooth(fp0);
                   if (tof_bins > 1)
                    { Logging::flog()->logMsg("scale axis #1 of randoms "
                                              "sinogram by #1", loglevel+1)->
                       arg(axis)->arg(1.0f/(float)tof_bins);
                      vecMulScalar(fp1, 1.0f/(float)tof_bins, fp1,
                                   axis_size[axis]);
                    }
                                               // subtract randoms from prompts
                   for (unsigned short int s=0; s < numberOfSinograms()-1; s++)
                    { fp0=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
                      vecSub(fp0, fp1, fp0, axis_size[axis]);
                      MemCtrl::mc()->put(data[s][axis]);
                    }
                   delete[] fp1;
                   fp1=NULL;
                  }
                delete gf;
                gf=NULL;
              }
        data[rnd_sino].clear();
        corrections_applied|=CORR_Randoms_Smoothing | CORR_Randoms_Subtraction;
        return;
      }
                                                            // subtract randoms
     Logging::flog()->logMsg("subtract randoms", loglevel);
     for (unsigned short int axis=0; axis < axes(); axis++)
      { float *fp0;

        fp1=MemCtrl::mc()->getRemoveFloat(&data[rnd_sino][axis], loglevel);
        for (unsigned short int s=0; s < numberOfSinograms()-1; s++)
         { fp0=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
           vecSub(fp0, fp1, fp0, axis_size[axis]);
           MemCtrl::mc()->put(data[s][axis]);
         }
        delete[] fp1;
        fp1=NULL;
      }
     data[rnd_sino].clear();
                                                    // set flag for corrections
     corrections_applied|=CORR_Randoms_Subtraction;
   }
   catch (...)
    { if (rs != NULL) delete rs;
      if (gf != NULL) delete gf;
      if (fp1 != NULL) delete[] fp1;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Cut-off values below threshold.
    \param[in] threshold     threshold
    \param[in] replacement   replacement value
    \param[in] loglevel      level for logging

    All sinogram values below the threshold are replaced by a replacement
    value.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::cutBelow(const float threshold,
                                  const float replacement,
                                  const unsigned short int loglevel)
 { Logging::flog()->logMsg("replace values below #1 by #2", loglevel)->
    arg(threshold)->arg(replacement);
   for (unsigned short int s=0; s < MAX_SINO; s++)
    for (unsigned short int axis=0; axis < data[s].size(); axis++)
     { float *dp;

       dp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
       for (unsigned long int i=0; i < axis_size[axis]; i++)
        if (dp[i] < threshold) dp[i]=replacement;
       MemCtrl::mc()->put(data[s][axis]);
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Decay and frame-length correction.
    \param[in] decay_corr      calculate decay correction ?
    \param[in] framelen_corr   calculate frame-length correction ?
    \param[in] loglevel        level for logging

    Calculate the decay and frame-length correction. The decay factor is
    calculated by the following equations:
    \f[
        \lambda=-\frac{\ln(\frac{1}{2})}{halflife}
    \f]
    \f[
        decay\_factor=\frac{e^{\lambda frame\_start}\lambda frame\_duration}
                           {1-e^{-\lambda frame\_duration}}
    \f]
    Sinograms that are already corrected are not corrected again.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::decayCorrect(bool decay_corr, bool framelen_corr,
                                       const unsigned short int loglevel)
 {                                       // don't correct, if already corrected
   if (corrections_applied & CORR_Decay_correction) decay_corr=false;
   if (corrections_applied & CORR_FrameLen_correction) framelen_corr=false;
   if (!decay_corr && !framelen_corr) return;
   float scale_factor;
                // calculate decay correction factor and scale factor for image
   decay_factor=calcDecayFactor(startTime(), frameDuration(), gateDuration(),
                                halflife(), decay_corr, framelen_corr,
                                &scale_factor, loglevel);
   for (unsigned short int s=0; s < MAX_SINO; s++)            // scale sinogram
    scale(scale_factor, s, loglevel+1);
                                                   // set flags for corrections
   if (framelen_corr) corrections_applied|=CORR_FrameLen_correction;
   if (decay_corr)
    { corrections_applied|=CORR_Decay_correction;
      Logging::flog()->logMsg("decay correction factor=#1", loglevel+1)->
       arg(decay_factor);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide sinogram by another sinogram.
    \param[in,out] sino          divisor
    \param[in]     delete_sino   delete sinogram data after use ?
    \param[in]     cflag         flag for correction which is removed
    \param[in]     set_to_zero   set result to 0 after division by 0 ?
    \param[in]     loglevel      level for logging
    \param[in]     max_threads   number of threads

    Divide sinogram by another sinogram. This method is usually used to
    uncorrect a sinogram. If the size of the divisor sinogram is different from
    this sinogram, its rebinned. The rebinning of sinograms is multi-threaded.
    Each thread processes a range of sinogram planes.

    If the divisor sinogram has only one projection in a sinogram plane,
    like a normalization sinogram of a rotating panel detector system,
    this projection is used for all angles of the dividend sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::divide(SinogramConversion * const sino,
                                const bool delete_sino,
                                const unsigned short int cflag,
                                const bool set_to_zero,
                                const unsigned short int loglevel,
                                const unsigned short int max_threads)
 { RebinSinogram *rebin=NULL;

   try
   { Logging::flog()->logMsg("divide sinograms", loglevel);
                                               // does divisor need rebinning ?
     if ((RhoSamples() != sino->RhoSamples()) ||
          ((sino->ThetaSamples() > 1) &&
           (ThetaSamples() != sino->ThetaSamples())))
      { rebin=new RebinSinogram(sino->RhoSamples(), sino->ThetaSamples(),
                                RhoSamples(), ThetaSamples(),
                                RebinX <float>::NO_ARC, 0, sino->isACF());
        Logging::flog()->logMsg("rebin sinogram from #1x#2x#3 to #4x#5x#6",
                                loglevel+1)->arg(sino->RhoSamples())->
         arg(sino->ThetaSamples())->arg(sino->axesSlices())->
         arg(RhoSamples())->arg(ThetaSamples())->arg(sino->axesSlices());
      }
     for (unsigned short int s=0; s < MAX_SINO; s++)
      { unsigned short int sino_ds;
        bool del_sino;

        if (sino->index(0, s) ==
            std::numeric_limits <unsigned short int>::max()) sino_ds=0;
         else sino_ds=s;
        if ((s < MAX_SINO-1) && (data[s+1].size() > 0)) del_sino=false;
         else del_sino=delete_sino;
        for (unsigned short int axis=0; axis < data[s].size(); axis++)
         { float *fp, *np;
           unsigned short int np_idx;

           np=MemCtrl::mc()->getFloatRO(sino->index(axis, sino_ds), loglevel);
           if (rebin != NULL)                                  // rebin divisor
            { np=rebin->calcRebinSinogram(np, axis_slices[axis], NULL,
                                          &np_idx, loglevel+1, max_threads);
              MemCtrl::mc()->put(sino->index(axis, sino_ds));
              if (del_sino) sino->deleteData(axis, sino_ds);
            }
                                                            // divide sinograms
           fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
           for (unsigned short int p=0; p < axis_slices[axis]; p++)
            { for (unsigned short int t=0; t < ThetaSamples(); t++,
                   fp+=RhoSamples())
               { for (unsigned short int r=0; r < RhoSamples(); r++)
                  if (np[r] != 0.0f) fp[r]/=np[r];
                   else if (set_to_zero) fp[r]=0.0f;
                 if (sino->ThetaSamples() > 1) np+=RhoSamples();
               }
              if (sino->ThetaSamples() == 1) np+=RhoSamples();
            }
           MemCtrl::mc()->put(data[s][axis]);
           if (rebin != NULL)
            { MemCtrl::mc()->put(np_idx);
              MemCtrl::mc()->deleteBlock(&np_idx);
            }
            else { MemCtrl::mc()->put(sino->index(axis, sino_ds));
                   if (del_sino) sino->deleteData(axis, sino_ds);
                 }
         }
      }
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     corrections_applied&=cflag ^ 0xffffffff;                    // remove flag
   }
   catch (...)
    { if (rebin != NULL) delete rebin;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert emission sinogram into transmission sinogram.
    \param[in] loglevel   level for logging

    Convert an emission sinogram into a transmission sinogram:
    \f[
        s_i=\max(1, \exp(s_i))\quad \forall i
    \f]
    Negative values in the emission sinogram will be 1 in the transmission
    sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::emi2Tra(const unsigned short int loglevel)
 { if (acf_data) return;
   Logging::flog()->logMsg("convert emission into transmission data",
                           loglevel);
   for (unsigned short int s=0; s < MAX_SINO; s++)
    for (unsigned short int axis=0; axis < data[s].size(); axis++)
     { float *fp;

       fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
       for (unsigned long int i=0; i < axis_size[axis]; i++)
        if (fp[i] < 0.0f) fp[i]=1.0f;                // cut-off negative values
         else fp[i]=expf(fp[i]);
       MemCtrl::mc()->put(data[s][axis]);
     }
   acf_data=true;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Fill gaps in sinogram based on the gantry model.
    \param[in] loglevel      level for logging
    \param[in] max_threads   number of threads

    Fill gaps in sinogram based on the gantry model. Only gaps from missing
    crystals in a crystal ring are filled; gaps between rings are not filled.
    This method is multi-threaded. Each thread processes a range of sinogram
    planes.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::fillGaps(const unsigned short int loglevel,
                                  const unsigned short int max_threads)
 { if (GM::gm()->transaxialBlockGaps() == 0) return;
   GapFill *gf=NULL;

   try
   { unsigned short int origRhoSamples, origThetaSamples;

     Logging::flog()->logMsg("model based gap filling", loglevel);
     origRhoSamples=RhoSamples();
     origThetaSamples=ThetaSamples();
     resampleRT(GM::gm()->RhoSamples(), GM::gm()->ThetaSamples(), loglevel+1,
                max_threads);
     gf=new GapFill(RhoSamples(), ThetaSamples(),
                    GM::gm()->transaxialCrystalsPerBlock(),
                    GM::gm()->transaxialBlockGaps(),
                    GM::gm()->crystalsPerRing(),
                    intrinsicTilt());
     for (unsigned short int s=0; s < MAX_SINO; s++)
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { gf->fillGaps(MemCtrl::mc()->getFloat(data[s][axis], loglevel),
                      axis_slices[axis], loglevel+1, max_threads);
         MemCtrl::mc()->put(data[s][axis]);
       }
     delete gf;
     gf=NULL;
     resampleRT(origRhoSamples, origThetaSamples, loglevel+1, max_threads);
   }
   catch (...)
    { if (gf != NULL) delete gf;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Fill gaps in sinogram based on the normalization data.
    \param[in,out] norm            normalization sinogram
    \param[in]     mask_bins       number of bins to ignore on each side of
                                   norm
    \param[in]     delete_sino     delete normalization sinogram after use ?
    \param[in]     loglevel        level for logging
    \param[in]     max_threads     number of threads

    Fill gaps in sinogram based on the normalization data. If the value of a
    bin in the normalization data is below a threshold, the corresponding
    sinogram bin is treated as a gap and filled by linear interpolation in
    azimuthal direction. This method is multi-threaded. Each thread processes
    a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::fillGaps(SinogramConversion *norm,
                                  const unsigned short int mask_bins,
                                  const bool delete_sino,
                                  const unsigned short int loglevel,
                                  const unsigned short int max_threads)
 { GapFill *gf=NULL;

   try
   { Logging::flog()->logMsg("normalization based gap filling", loglevel);
     gf=new GapFill(RhoSamples(), ThetaSamples());
     for (unsigned short int s=0; s < MAX_SINO; s++)
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { float *sp, *np;

         sp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
         np=MemCtrl::mc()->getFloatRO(norm->index(axis, 0), loglevel);
         gf->fillGaps(sp, np, axis_slices[axis], gap_fill_threshold, mask_bins,
                      loglevel+1, max_threads);
         MemCtrl::mc()->put(norm->index(axis, 0));
         if (delete_sino && ((s == MAX_SINO-1) || (data[s+1].size() == 0)))
          norm->deleteData(axis, 0);
         MemCtrl::mc()->put(data[s][axis]);
       }
     delete gf;
     gf=NULL;
   }
   catch (...)
    { if (gf != NULL) delete gf;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Mask border bins.
    \param[in] t          number of bins to mask-out at each side of a
                          projection
    \param[in] loglevel   level for logging

    Mask border bins by replacing t bins at each side of a projection with 0.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::maskBorders(const unsigned short int t,
                                     const unsigned short int loglevel)
 { if (t == 0) return;
   Logging::flog()->logMsg("mask #1 bins at each side of a projection",
                           loglevel)->arg(t);
   for (unsigned short int s=0; s < MAX_SINO; s++)
    for (unsigned short int axis=0; axis < data[s].size(); axis++)
     { unsigned long int projections;

       projections=(unsigned long int)ThetaSamples()*
                   (unsigned long int)axis_slices[axis];
       switch (MemCtrl::mc()->dataType(data[s][axis]))
        { case MemCtrl::DT_FLT:
           { float *fp;

             fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
             for (unsigned long int i=0; i < projections; i++,
                  fp+=RhoSamples())
              { memset(fp, 0, t*sizeof(float));
                memset(fp+RhoSamples()-t, 0, t*sizeof(float));
              }
           }
           break;
          case MemCtrl::DT_SINT:
           { signed short int *ip;

             ip=MemCtrl::mc()->getSInt(data[s][axis], loglevel);
             for (unsigned long int i=0; i < projections; i++,
                  ip+=RhoSamples())
              { memset(ip, 0, t*sizeof(signed short int));
                memset(ip+RhoSamples()-t, 0, t*sizeof(signed short int));
              }
           }
           break;
          case MemCtrl::DT_UINT:
           { uint32 *ip;

             ip=MemCtrl::mc()->getUInt(data[s][axis], loglevel);
             for (unsigned long int i=0; i < projections; i++,
                  ip+=RhoSamples())
              { memset(ip, 0, t*sizeof(uint32));
                memset(ip+RhoSamples()-t, 0, t*sizeof(uint32));
              }
           }
           break;
          case MemCtrl::DT_CHAR:
           { char *cp;

             cp=MemCtrl::mc()->getChar(data[s][axis], loglevel);
             for (unsigned long int i=0; i < projections; i++,
                  cp+=RhoSamples())
              { memset(cp, 0, t*sizeof(char));
                memset(cp+RhoSamples()-t, 0, t*sizeof(char));
              }
           }
           break;
        }
       MemCtrl::mc()->put(data[s][axis]);
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Mask-out detector blocks from a sinogram.
    \param[in] mask_blocks   vector with block numbers that are masked-out
    \param[in] loglevel      level for logging

    Mask-out detector blocks from a sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::maskOutBlocks(
                            const std::vector <unsigned short int> mask_blocks,
                            const unsigned short int loglevel)
 { if (mask_blocks.size() == 0) return;
   std::vector <bool> mask;
   std::vector <std::vector <unsigned short int> > ring_numbers;

   ring_numbers=calcRingNumbers(GM::gm()->rings(), span(), mrd());
                            // calculate mask where all crystals are false
                            // where the block is masked-out and true elsewhere
   mask.assign((unsigned long int)GM::gm()->rings()*
               (unsigned long int)GM::gm()->crystalsPerRing(), true);
   for (unsigned short int k=0; k < mask_blocks.size(); k++)
    { unsigned short int buckets_per_ring, crystals_per_bucket;
      unsigned long int gantry_crystal=0;

      Logging::flog()->logMsg("mask block #1 in sinogram", loglevel)->
       arg(mask_blocks[k]);
      crystals_per_bucket=GM::gm()->transaxialBlocksPerBucket()*
                          GM::gm()->transaxialCrystalsPerBlock();
      buckets_per_ring=GM::gm()->crystalsPerRing()/crystals_per_bucket;
      for (unsigned short int ring=0; ring < GM::gm()->rings(); ring++)
       for (unsigned short int crystal=0;
            crystal < GM::gm()->crystalsPerRing();
            crystal++,
            gantry_crystal++)
        { unsigned short int block;

          block=crystal/GM::gm()->transaxialCrystalsPerBlock()+
                GM::gm()->transaxialBlocksPerBucket()*buckets_per_ring*
                (ring/GM::gm()->axialCrystalsPerBlock());
          if (block == mask_blocks[k]) mask[gantry_crystal]=false;
        }
    }
   for (unsigned short int s=0; s < MAX_SINO; s++)
    { unsigned short int pl=0;
                                                      // mask block in sinogram
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { unsigned short int segment_slices, p;
         float *fp;

         fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
         if (axis == 0) segment_slices=axis_slices[0];
          else segment_slices=axis_slices[axis]/2;
         for (p=0; p < segment_slices; p++,
              fp+=planesize,
              pl++)
          maskOutLORs(fp, mask, ring_numbers[pl]);
         if (axis > 0)
          for (p=0; p < segment_slices; p++,
               fp+=planesize,
               pl++)
           maskOutLORs(fp, mask, ring_numbers[pl]);
         MemCtrl::mc()->put(data[s][axis]);
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove physical LORs from a sinogram plane.
    \param[in,out] sino        sinogram plane
    \param[in]     mask        boolean-mask for crystals in a gantry
    \param[in]     ring_pair   numbers of ring pairs that contribute to this
                               plane

    Remove physical LORs from a sinogram plane. Each sinogram bin contains
    counts from several LORs, based on mash and span of the sinogram. These
    LORs can be masked-out by setting crystal values in the mask to false. The
    number of counts in a bin is reduced by the precentage of masked-out LORs
    that don't contribute to the bin anymore.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::maskOutLORs(float * sino,
                        const std::vector <bool> mask,
                        const std::vector <unsigned short int> ring_pair) const
 { for (unsigned short int t=0; t < ThetaSamples(); t++)
    for (signed short int r=-RhoSamples()/2; r < RhoSamples()/2; r++)
     { unsigned short int scale=0, factor=0;
                                    // add up the 0 or 1 values of the physical
                                    // LORS that are part of the sinogram LOR
       for (unsigned short int tm=t*mash(); tm < (t+1)*mash(); tm++)
        { unsigned short int crys_a, crys_b;

          crys_a=(tm+(r >> 1)+GM::gm()->crystalsPerRing()) %
                 GM::gm()->crystalsPerRing();
          crys_b=(tm-((r+1) >> 1)+GM::gm()->crystalsPerRing()+
                 ThetaSamples()*mash()) % GM::gm()->crystalsPerRing();
          for (unsigned short int pair=0; pair < ring_pair.size(); pair+=2)
           if (mask[crys_a+GM::gm()->crystalsPerRing()*ring_pair[pair]] &&
               mask[crys_b+GM::gm()->crystalsPerRing()*ring_pair[pair+1]])
            factor++;
          scale+=ring_pair.size()/2;           // count number of physical LORs
        }
       *sino++*=(float)factor/(float)scale;                   // scale sinogram
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply sinogram by another sinogram.
    \param[in,out] sino          factor
    \param[in]     delete_sino   delete sinogram data after use ?
    \param[in]     cflag         flag for correction
    \param[in]     loglevel      level for logging
    \param[in]     max_threads   number of threads

    Multiply sinogram by another sinogram. This method is usually used to
    correct a sinogram. If the size of the factor sinogram is different from
    this sinogram, its rebinned. The rebinning of sinograms is multi-threaded.
    Each thread processes a range of sinogram planes.

    If the factor sinogram has only one projection in a sinogram plane, like a
    normalization sinogram of a rotating panel detector system, this
    projection is used for all angles of the factor sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::multiply(SinogramConversion * const sino,
                                  const bool delete_sino,
                                  const unsigned short int cflag,
                                  const unsigned short int loglevel,
                                  const unsigned short int max_threads)
 { RebinSinogram *rebin=NULL;

   try
   { Logging::flog()->logMsg("multiply sinograms", loglevel);
                                                // does factor need rebinning ?
     if ((RhoSamples() != sino->RhoSamples()) ||
          ((sino->ThetaSamples() > 1) &&
           (ThetaSamples() != sino->ThetaSamples())))
      { rebin=new RebinSinogram(sino->RhoSamples(), sino->ThetaSamples(),
                                RhoSamples(), ThetaSamples(),
                                RebinX <float>::NO_ARC, 0, sino->isACF());
        Logging::flog()->logMsg("rebin sinogram from #1x#2x#3 to #4x#5x#6",
                                loglevel+1)->arg(sino->RhoSamples())->
         arg(sino->ThetaSamples())->arg(sino->axesSlices())->
         arg(RhoSamples())->arg(ThetaSamples())->arg(sino->axesSlices());
      }
     for (unsigned short int s=0; s < MAX_SINO; s++)
       { unsigned short int sino_ds;
         bool del_sino;

         if (sino->index(0, s) ==
             std::numeric_limits <unsigned short int>::max()) sino_ds=0;
          else sino_ds=s;
         if ((s < MAX_SINO-1) && (data[s+1].size() > 0)) del_sino=false;
          else del_sino=delete_sino;
         for (unsigned short int axis=0; axis < data[s].size(); axis++)
          { float *fp, *np;
            unsigned short int np_idx;

            np=MemCtrl::mc()->getFloatRO(sino->index(axis, sino_ds), loglevel);
            if (rebin != NULL)                                  // rebin factor
             { np=rebin->calcRebinSinogram(np, axis_slices[axis], NULL,
                                           &np_idx, loglevel+1, max_threads);
               MemCtrl::mc()->put(sino->index(axis, sino_ds));
               if (del_sino) sino->deleteData(axis, sino_ds);
             }
                                                          // multiply sinograms
            fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
            for (unsigned short int p=0; p < axis_slices[axis]; p++)
             { for (unsigned short int t=0; t < ThetaSamples(); t++,
                    fp+=RhoSamples())
                { vecMul(fp, np, fp, RhoSamples());
                  if (sino->ThetaSamples() > 1) np+=RhoSamples();
                }
               if (sino->ThetaSamples() == 1) np+=RhoSamples();
             }
            MemCtrl::mc()->put(data[s][axis]);
            if (rebin != NULL)
             { MemCtrl::mc()->put(np_idx);
               MemCtrl::mc()->deleteBlock(&np_idx);
             }
             else { MemCtrl::mc()->put(sino->index(axis, sino_ds));
                    if (del_sino) sino->deleteData(axis, sino_ds);
                  }
          }
       }
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     corrections_applied|=cflag;                     // set flag for correction
   }
   catch (...)
    { if (rebin != NULL) delete rebin;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply randoms sinogram by another sinogram.
    \param[in,out] sino       factor
    \param[in]     sf         scale factor for randoms
    \param[in]     loglevel   level for logging

    Multiply randoms sinogram by another sinogram. This method is usually used
    to normalize randoms for OP-OSEM.

    If the factor sinogram has only one projection in a sinogram plane, like a
    normalization sinogram of a rotating panel detector system, this
    projection is used for all angles of the factor sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::multiplyRandoms(SinogramConversion * const sino,
                                         const float sf,
                                         const unsigned short int loglevel)
 { unsigned short int rnd_sino;

   Logging::flog()->logMsg("multiply randoms sinogram with sinogram",
                           loglevel);
   rnd_sino=numberOfSinograms()-1;
   for (unsigned short int axis=0; axis < data[rnd_sino].size(); axis++)
    { float *fp, *np, *fp_orig;

      np=MemCtrl::mc()->getFloatRO(sino->index(axis, 0), loglevel);
      fp=MemCtrl::mc()->getFloat(data[rnd_sino][axis], loglevel);
      fp_orig=fp;
                                                          // multiply sinograms
      for (unsigned short int p=0; p < axis_slices[axis]; p++)
       { for (unsigned short int t=0; t < ThetaSamples(); t++,
              fp+=RhoSamples())
          { vecMul(fp, np, fp, RhoSamples());
            if (sino->ThetaSamples() > 1) np+=RhoSamples();
          }
         if (sino->ThetaSamples() == 1) np+=RhoSamples();
       }
      if (sf != 1.0f)
       { Logging::flog()->logMsg("scale axis #1 of randoms sinogram by #1",
                                 loglevel+1)->arg(axis)->arg(sf);
         vecMulScalar(fp_orig, sf, fp_orig, axis_size[axis]);
       }
      MemCtrl::mc()->put(data[rnd_sino][axis]);
      MemCtrl::mc()->put(sino->index(axis, 0));
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print statistics about sinogram.
    \param[in] loglevel   level for logging

    Print statistics about sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::printStat(const unsigned short int loglevel) const
 { for (unsigned short int s=0; s < MAX_SINO; s++)
    { float minv=std::numeric_limits <float>::max(),
            maxv=-std::numeric_limits <float>::max();
      double sum=0.0;

      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { float mi, ma;
         double su;

         ::printStat("counts in sinogram "+toString(s)+", axis "+
                     toString(axis), loglevel,
                     MemCtrl::mc()->getFloatRO(data[s][axis], loglevel),
                     axis_size[axis], &mi, &ma, &su);
         MemCtrl::mc()->put(data[s][axis]);
         if ((mi < minv) || (axis == 0)) minv=mi;
         if ((ma > maxv) || (axis == 0)) maxv=ma;
         sum+=su;
       }
      if (data[s].size() > 0)
       Logging::flog()->logMsg("counts in sinogram #1: min=#2  max=#3  "
                               "total=#4", loglevel)->arg(s)->arg(minv)->
        arg(maxv)->arg(sum);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Arc-correction of sinogram in radial direction.
    \param[in] never_correct   don't calculate anything ?
    \param[in] arc_corr        perform arc-correction ? (otherwise: un-arc)
    \param[in] loglevel        level for logging
    \param[in] max_threads     number of threads to use

    Perform an arc-correction of the sinogram in radial direction. The data is
    only corrected once - data that was already corrected will not be
    corrected again. Only sinograms from gantries with a ring-architecture will
    be corrected. This method is multi-threaded. Each thread processes a range
    of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::radialArcCorrection(const bool never_correct,
                                          const bool arc_corr,
                                          const unsigned short int loglevel,
                                          const unsigned short int max_threads)
 { if (never_correct) return;
                               // correct only if scanner has ring-architecture
   if (!GM::gm()->ringArchitecture()) return;
                                                 // is data already corrected ?
   if (arc_corr && (corrections_applied & CORR_Radial_Arc_correction)) return;
   if (!arc_corr && !(corrections_applied & CORR_Radial_Arc_correction))
    return;
   RebinSinogram *reb=NULL;
   float *fp=NULL;

   try
   { RebinX <float>::tarc_correction arc_correction;

     if (arc_corr)
      { Logging::flog()->logMsg("arc-correct sinogram in radial direction",
                              loglevel);
        arc_correction=RebinX <float>::ARC2UNIFORM;
      }
      else { Logging::flog()->logMsg("un-arc-correct sinogram in radial "
                                     "direction", loglevel);
             arc_correction=RebinX <float>::UNIFORM2ARC;
           }
     reb=new RebinSinogram(RhoSamples(), ThetaSamples(), RhoSamples(),
                           ThetaSamples(), arc_correction,
                           GM::gm()->crystalsPerRing(), acf_data);
     for (unsigned short int s=0; s < MAX_SINO; s++)
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { float *reb_data;

         fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
         reb_data=reb->calcRebinSinogram(fp, axis_slices[axis], NULL,
                                         &data[s][axis], loglevel,
                                         max_threads);
         delete[] fp;
         fp=NULL;
                      // when rebinning an acf sinogram, the resulting sinogram
                      // may have values below 1.0 at the borders
         if (acf_data)
          for (unsigned long int i=0; i < axis_size[axis]; i++)
           if (reb_data[i] < 1.0f) reb_data[i]=1.0f;
         MemCtrl::mc()->put(data[s][axis]);
       }
     delete reb;
     reb=NULL;
                                                     // set flag for correction
     if (arc_corr) corrections_applied|=CORR_Radial_Arc_correction;
      else corrections_applied^=CORR_Radial_Arc_correction;
   }
   catch (...)
    { if (reb != NULL) delete reb;
      if (fp != NULL) delete[] fp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Resample sinogram in radial and azimuthal direction.
    \param[in] newRhoSamples     number of bins in a projection after
                                 resampling
    \param[in] newThetaSamples   number of projections in a sinogram plane
                                 after resampling
    \param[in] loglevel          level for logging
    \param[in] max_threads       number of threads

    Resample sinogram in radial and azimuthal direction. This method is
    multi-threaded. Each thread processes a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::resampleRT(const unsigned short int newRhoSamples,
                                    const unsigned short int newThetaSamples,
                                    const unsigned short int loglevel,
                                    const unsigned short int max_threads)
 { RebinSinogram *reb=NULL;
   float *fp=NULL;

   try
   { if ((RhoSamples() == newRhoSamples) &&
         (ThetaSamples() == newThetaSamples)) return;
     Logging::flog()->logMsg("rebin sinogram from #1x#2x#3 to #4x#5x#6",
                             loglevel)->arg(RhoSamples())->
      arg(ThetaSamples())->arg(axes_slices)->arg(newRhoSamples)->
      arg(newThetaSamples)->arg(axes_slices);
     reb=new RebinSinogram(RhoSamples(), ThetaSamples(), newRhoSamples,
                           newThetaSamples, RebinX <float>::NO_ARC, 0,
                           acf_data);
     for (unsigned short int s=0; s < MAX_SINO; s++)
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
         reb->calcRebinSinogram(fp, axis_slices[axis], NULL, &data[s][axis],
                                loglevel, max_threads);
         delete[] fp;
         fp=NULL;
         axis_size[axis]=(unsigned long int)axis_slices[axis]*
                         (unsigned long int)newRhoSamples*
                         (unsigned long int)newThetaSamples;
         MemCtrl::mc()->put(data[s][axis]);
       }
     delete reb;
     reb=NULL;
     vBinSizeRho*=(float)RhoSamples()/(float)newRhoSamples;
     vRhoSamples=newRhoSamples;
     vThetaSamples=newThetaSamples;
     vmash=GM::gm()->crystalsPerRing()/2/ThetaSamples();
     planesize=(unsigned long int)RhoSamples()*
               (unsigned long int)ThetaSamples();
   }
   catch (...)
    { if (reb != NULL) delete reb;
      if (fp != NULL) delete[] fp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Resample sinogram in z-direction.
    \param[in] new_planes   number of planes after resampling
    \param[in] zoom         axial zoom factor
    \param[in] name         prefix for swap filenames
    \param[in] loglevel     level for logging

    Resample sinogram in z-direction by linear interpolation. The zoom factor
    is chosen independently from the number of planes. If needed, the last
    plane of the original sinogram is extended. This way it is possible,
    to resample 239 planes into 60 planes with an integral zoom factor of 4.
    This method works only for 2d sinograms.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::resampleZ(const unsigned short int new_planes,
                                   const unsigned short int zoom,
                                   const std::string name,
                                   const unsigned short int loglevel)
 { if (new_planes == axes_slices) return;
   if (axes() > 1) return;
   float *fp=NULL;

   try
   { unsigned long int new_size;

     Logging::flog()->logMsg("rebin sinogram from #1 planes to #2 planes",
                             loglevel)->arg(axes_slices)->arg(new_planes);
     new_size=planesize*(unsigned long int)new_planes;
     for (unsigned short int s=0; s < MAX_SINO; s++)
      { float *mp, *buffer;
        unsigned short int idx;

        if (data[s].size() == 0) continue;
        buffer=MemCtrl::mc()->createFloat(new_size, &idx, name, loglevel);
        mp=buffer;
        fp=MemCtrl::mc()->getRemoveFloat(&data[s][0], loglevel);
        data[s][0]=idx;
        for (unsigned short int z=0; z < new_planes; z++,
             mp+=planesize)
         { float zp, f, *sp2, *sp;
           unsigned short int sl;

           zp=(float)z/zoom;
           sl=(unsigned short int)zp;      // plane number in original sinogram
                                  // calculate weights for linear interpolation
           f=zp-(float)sl;
           sp=fp+(unsigned long int)sl*planesize;
           if (sl >= axes_slices) break;
            else if (sl == axes_slices-1) { sp2=sp;
                                            f=0.0f;
                                          }
                  else sp2=sp+planesize;
                                     // calculate plane by linear interpolation
           vecMulAdd(sp, 1.0f-f, sp2, f, mp, planesize);
         }
        delete[] fp;
        fp=NULL;
        axis_size[0]=new_size;
        MemCtrl::mc()->put(data[s][0]);
      }
     plane_separation/=zoom;
     axes_slices=new_planes;
     axis_slices[0]=new_planes;
   }
   catch (...)
    { if (fp != NULL) delete[] fp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rotate sinogram clockwise around z-axis.
    \param[in] data     sinogram data
    \param[in] bins     number of bins in a projection
    \param[in] angles   number of angles in a sinogram plane
    \param[in] slices   number of planes in sinogram
    \param[in] rt       number of steps to rotate (each step is
                        180/angles degrees)

    Rotate sinogram clockwise around z-axis.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void SinogramConversion::rotate(T * const data, const unsigned short int bins,
                                const unsigned short int angles,
                                const unsigned short int slices,
                                const signed short int rt) const
 { if (data == NULL) return;
   unsigned short int s;
   unsigned long int size_sinoslice, bytes, bytes2;
   std::vector <T> buffer, buffer2;

   if ((rt == 0) || (abs(rt) >= angles)) return;
   size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
   bytes=abs(rt)*bins;
   buffer.resize(bytes);                   // buffer for upper part of sinogram
   bytes2=(angles-abs(rt))*bins;
   if (rt > 0) buffer2.resize(bytes2);     // buffer for lower part of sinogram
   for (s=0; s < slices; s++)
    { unsigned short int t;
      T *sp;

      if (rt > 0)
       buffer.assign(data+bytes2+s*size_sinoslice,
                     data+bytes2+s*size_sinoslice+bytes);
       else buffer.assign(data+s*size_sinoslice, data+s*size_sinoslice+bytes);
                // mirror bins in upper part of each sinogram and shift one bin
      for (sp=&buffer[0],
           t=0; t < abs(rt); t++,
           sp+=bins)
       { for (unsigned short int r=0; r < bins/2-1; r++)
          std::swap(sp[r+1], sp[bins-1-r]);
         sp[0]=sp[1];                        // replace first bin bei neighbour
       }
                                     // rotate lines of sinogram
                                     // (move upper part of sinogram to bottom)
      if (rt > 0)
       { buffer2.assign(data+s*size_sinoslice, data+s*size_sinoslice+bytes2);
         memcpy(data+bytes+s*size_sinoslice, &buffer2[0], bytes2*sizeof(T));
         memcpy(data+s*size_sinoslice, &buffer[0], bytes*sizeof(T));
       }
       else { memcpy(data+s*size_sinoslice,
                     data+abs(rt)*bins+s*size_sinoslice, bytes2*sizeof(T));
              memcpy(data+bytes2+s*size_sinoslice, &buffer[0],
                     bytes*sizeof(T));
            }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Resort time-of-flight data.
    \param[in] shuffle_data   shuffle data ? (otherwise: unshuffle)
    \param[in] loglevel       level for logging

    Resort time-of-flight data from (r,t,z,seg,tof) to (tof,r,t,z,seg) format
    (shuffling) or from (tof,r,t,z,seg) to (r,t,z,seg,tof) (unshuffling).
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::shuffleTOFdata(const bool shuffle_data,
                                        const unsigned short int loglevel)
 { if (shuffled_tof_data == shuffle_data) return;
   if (TOFbins() < 2) return;
   float *fp=NULL;

   try
   { unsigned short int s;

     if (shuffle_data)                                          // shuffle data
      { std::vector <unsigned short int> dp;

        Logging::flog()->logMsg("shuffle TOF bins", loglevel);
        dp.resize(axes());
        for (unsigned short int axis=0; axis < axes(); axis++)
         { float *dptr;

           dptr=MemCtrl::mc()->createFloat(TOFbins()*axis_size[axis],
                                           &dp[axis], "emis", loglevel);
           for (s=0; s < TOFbins(); s++)
            { fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
              for (unsigned long int i=0; i < axis_size[axis]; i++)
               dptr[i*TOFbins()+s]=fp[i];
              delete[] fp;
              fp=NULL;
            }
           MemCtrl::mc()->put(dp[axis]);
           axis_size[axis]*=TOFbins();
         }
        for (s=0; s < TOFbins(); s++)
         data[s].clear();
        data[0]=dp;
        shuffled_tof_data=true;
        return;
      }
                                                              // unshuffle data
     Logging::flog()->logMsg("unshuffle TOF bins", loglevel);
     for (s=1; s < TOFbins(); s++)
      resizeIndexVec(s, axes());
     for (unsigned short int axis=0; axis < axes(); axis++)
      { unsigned short int idx;
        float *fptr;

        idx=data[0][axis];
        fptr=MemCtrl::mc()->getFloatRO(idx, loglevel);
        for (s=0; s < TOFbins(); s++)
         { float *dptr;

           dptr=MemCtrl::mc()->createFloat(axis_size[axis]/TOFbins(),
                                           &data[s][axis], "emis", loglevel);
           for (unsigned long int i=0; i < axis_size[axis]/TOFbins(); i++)
            dptr[i]=fptr[i*TOFbins()+s];
           MemCtrl::mc()->put(data[s][axis]);
         }
        MemCtrl::mc()->put(idx);
        MemCtrl::mc()->deleteBlock(&idx);
        axis_size[axis]/=TOFbins();
      }
     shuffled_tof_data=false;
   }
   catch (...)
    { if (fp != NULL) delete[] fp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert 2d sinogram into 3d sinogram.
    \param[in] reb           rebinning method
    \param[in] _span         span of 3d sinogram
    \param[in] _mrd          maximum ring difference of 3d sinogram
    \param[in] name          prefix for swap filenames
    \param[in] loglevel      level for logging
    \param[in] max_threads   number of threads

    Convert 2d sinogram into 3d sinogram. Available methods are inverse
    single-slice rebinning and inverse fourier rebinning. The inverse fourier
    rebinning is multi-threaded. Each thread processes one axis of the
    sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::sino2D3D(SinogramConversion::tirebin_method reb,
                                  const unsigned short int _span,
                                  const unsigned short int _mrd,
                                  const std::string name,
                                  const unsigned short int loglevel,
                                  const unsigned short int max_threads)
 { if ((reb == iNO_REB) || (reb == FWDPJ)) return;
   Sino2D_3D *ifore=NULL;
   float *sino3d=NULL;

   try
   { unsigned short int new_axes, new_axes_slices, axis, old_axes_slices;
     std::string str;
     std::vector <unsigned short int> new_axis_slices;

     if (axes() != 1) return;
     vspan=_span;
     vmrd=_mrd;
     Logging::flog()->logMsg("span=#1", loglevel)->arg(span());
     Logging::flog()->logMsg("maximum ring difference=#1", loglevel)->
      arg(mrd());
                                                       // create new axes table
     old_axes_slices=axes_slices;
     new_axes=(mrd()-(span()-1)/2)/span()+1;
     new_axis_slices.resize(new_axes);
     new_axis_slices[0]=2*GM::gm()->rings()-1;
     new_axes_slices=axis_slices[0];
     axis_size.resize(new_axes);
     axis_size[0]=(unsigned long int)new_axis_slices[0]*planesize;
     for (axis=1; axis < new_axes; axis++)
      { new_axis_slices[axis]=
                           2*(new_axis_slices[0]-2*(axis*span()-(span()-1)/2));
        axis_size[axis]=(unsigned long int)new_axis_slices[axis]*planesize;
        new_axes_slices+=new_axis_slices[axis];
      }
     for (unsigned short int s=0; s < MAX_SINO; s++)
      switch (reb)
       { case iSSRB_REB:                      // inverse single-slice rebinning
          if (data[s].size() != 1) continue;
          Logging::flog()->logMsg("iSSRB sinogram #1 from #2x#3x#4 to "
                                  "#5x#6x#7", loglevel)->arg(s+1)->
           arg(RhoSamples())->arg(ThetaSamples())->arg(old_axes_slices)->
           arg(RhoSamples())->arg(ThetaSamples())->arg(new_axes_slices);
          resizeIndexVec(s, new_axes);
          switch (MemCtrl::mc()->dataType(data[s][0]))
           { case MemCtrl::DT_CHAR:
              { char *cp0;

                cp0=MemCtrl::mc()->getChar(data[s][0], loglevel);
                for (axis=1; axis < new_axes; axis++)
                 { unsigned short int offs;
                   char *cp;

                   cp=MemCtrl::mc()->createChar(axis_size[axis],
                                                &data[s][axis], name,
                                                loglevel);
                   offs=(new_axis_slices[0]-new_axis_slices[axis]/2)/2;
                   memcpy(cp, cp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(signed char));
                   memcpy(cp+axis_size[axis]/2,
                          cp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(signed char));
                   MemCtrl::mc()->put(data[s][axis]);
                 }
                MemCtrl::mc()->put(data[s][0]);
              }
              break;
             case MemCtrl::DT_SINT:
              { signed short int *sp0;

                sp0=MemCtrl::mc()->getSInt(data[s][0], loglevel);
                for (axis=1; axis < new_axes; axis++)
                 { unsigned short int offs;
                   signed short int *sp;

                   sp=MemCtrl::mc()->createSInt(axis_size[axis],
                                                &data[s][axis], name,
                                                loglevel);
                   offs=(new_axis_slices[0]-new_axis_slices[axis]/2)/2;
                   memcpy(sp, sp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(signed short int));
                   memcpy(sp+axis_size[axis]/2,
                          sp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(signed short int));
                   MemCtrl::mc()->put(data[s][axis]);
                 }
                MemCtrl::mc()->put(data[s][0]);
              }
              break;
             case MemCtrl::DT_UINT:
              { uint32 *sp0;

                sp0=MemCtrl::mc()->getUInt(data[s][0], loglevel);
                for (axis=1; axis < new_axes; axis++)
                 { unsigned short int offs;
                   uint32 *sp;

                   sp=MemCtrl::mc()->createUInt(axis_size[axis],
                                                &data[s][axis], name,
                                                loglevel);
                   offs=(new_axis_slices[0]-new_axis_slices[axis]/2)/2;
                   memcpy(sp, sp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(uint32));
                   memcpy(sp+axis_size[axis]/2,
                          sp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(uint32));
                   MemCtrl::mc()->put(data[s][axis]);
                 }
                MemCtrl::mc()->put(data[s][0]);
              }
              break;
             case MemCtrl::DT_FLT:
              { float *fp0;

                fp0=MemCtrl::mc()->getFloat(data[s][0], loglevel);
                for (axis=1; axis < new_axes; axis++)
                 { unsigned short int offs;
                   float *fp;

                   fp=MemCtrl::mc()->createFloat(axis_size[axis],
                                                 &data[s][axis], name,
                                                 loglevel);
                   offs=(new_axis_slices[0]-new_axis_slices[axis]/2)/2;
                   memcpy(fp, fp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(float));
                   memcpy(fp+axis_size[axis]/2,
                          fp0+(unsigned long int)offs*planesize,
                          axis_size[axis]/2*sizeof(float));
                   MemCtrl::mc()->put(data[s][axis]);
                 }
                MemCtrl::mc()->put(data[s][0]);
              }
              break;
           }
          break;
         case iFORE_REB:                           // inverse fourier rebinning
          if (data[s].size() != 1) continue;
          Logging::flog()->logMsg("iFORE sinogram #1 from #2x#3x#4 to "
                                  "#5x#6x#7", loglevel)->arg(s+1)->
           arg(RhoSamples())->arg(ThetaSamples())->arg(old_axes_slices)->
           arg(RhoSamples())->arg(ThetaSamples())->arg(new_axes_slices);
          ifore=new Sino2D_3D(this, new_axis_slices, loglevel+1);
          ifore->calcSino3D(this, s, name, max_threads);
          delete ifore;
          ifore=NULL;
          break;
         case iNO_REB:                                          // can't happen
         case FWDPJ:
          break;
       }
     axes_slices=new_axes_slices;
     axis_slices=new_axis_slices;
     Logging::flog()->logMsg("axes=#1", loglevel)->arg(axes());
     str=toString(axis_slices[0]);
     for (axis=1; axis < axes(); axis++)
      str+=","+toString(axis_slices[axis]);
     Logging::flog()->logMsg("axis table=#1 {#2}", loglevel)->
      arg(axes_slices)->arg(str);
   }
   catch (...)
    { if (ifore != NULL) delete ifore;
      if (sino3d != NULL) delete[] sino3d;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert 3d sinogram into 2d sinogram.
    \param[in] reb           rebinning method
    \param[in] a_lim         axis limit for single-slice rebinning in FORE
    \param[in] w_lim         radial limit for single-slice rebinning in FORE
    \param[in] k_lim         azimuthal limit for single-slice rebinning in FORE
    \param[in] name          prefix for swap filenames
    \param[in] num           matrix number
    \param[in] loglevel      level for logging
    \param[in] max_threads   number of threads

    Convert 3d sinogram into 2d sinogram. Available methods are Seg0,
    single-slice rebinning and fourier rebinning. The FFT used in the fourier
    rebinning is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::sino3D2D(SinogramConversion::trebin_method reb,
                                  const unsigned short int a_lim,
                                  const unsigned short int w_lim,
                                  const unsigned short int k_lim,
                                  const std::string name,
                                  const unsigned short int num,
                                  const unsigned short int loglevel,
                                  const unsigned short int max_threads)
 { FORE *fore_rebin=NULL;
   SSR *ssr=NULL;
   float *fp=NULL;

   try
   { switch (reb)
      { case SSRB_REB:
         Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2, "SSRB (frame #1)")->
          arg(num);
         ssr=new SSR(RhoSamples(), ThetaSamples(), axis_slices, 0, loglevel);
         break;
        case FORE_REB:
         Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2, "FORE (frame #1)")->
          arg(num);
         fore_rebin=new FORE(RhoSamples(), BinSizeRho(), ThetaSamples(),
                             axis_slices, span(), GM::gm()->planeSeparation(),
                             GM::gm()->ringRadius()+GM::gm()->DOI(), a_lim,
                             w_lim, k_lim, loglevel+1);
         break;
        case NO_REB:                                       // don't do anything
         break;
        case SEG0_REB:
         Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2,
                                  "SEG0 (frame #1)")->arg(num);
         break;
      }
     for (unsigned short int s=0; s < MAX_SINO; s++)
      { unsigned short int axis;

        switch (reb)
         { case SEG0_REB:                                 // use only segment 0
            if (data[s].size() == 0) continue;
            Logging::flog()->logMsg("delete oblique segments", loglevel);
            for (axis=1; axis < data[s].size(); axis++)
             MemCtrl::mc()->deleteBlock(&data[s][axis]);
            resizeIndexVec(s, 1);
            break;
           case SSRB_REB:                             // single-slice rebinning
            if (data[s].size() == 0) continue;
            Logging::flog()->logMsg("SSRB sinogram from #1x#2x#3 to #4x#5x#6",
                                    loglevel)->arg(RhoSamples())->
             arg(ThetaSamples())->arg(axes_slices)->arg(RhoSamples())->
             arg(ThetaSamples())->arg(axis_slices[0]);
            for (axis=0; axis < data[s].size(); axis++)
             { Logging::flog()->logMsg("rebin axis #1", loglevel+1)->arg(axis);
               fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
               ssr->Rebinning(fp, axis);
               delete[] fp;
               fp=NULL;
             }
            resizeIndexVec(s, 1);
            Logging::flog()->logMsg("normalize 2d sinogram", loglevel+1);
            data[s][0]=ssr->Normalize();
            break;
           case FORE_REB:                                  // fourier rebinning
            if (data[s].size() == 0) continue;
            Logging::flog()->logMsg("FORE sinogram from #1x#2x#3 to #4x#5x#6",
                                    loglevel)->arg(RhoSamples())->
             arg(ThetaSamples())->arg(axes_slices)->arg(RhoSamples())->
             arg(ThetaSamples())->arg(axis_slices[0]);
            for (axis=0; axis < data[s].size(); axis++)
             { Logging::flog()->logMsg("rebin axis #1", loglevel+1)->arg(axis);
               fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
               fore_rebin->Rebinning(fp, axis, max_threads);
               delete[] fp;
               fp=NULL;
             }
            resizeIndexVec(s, 1);
            Logging::flog()->logMsg("normalize 2d sinogram", loglevel+1);
            fore_rebin->Normalize(name, &data[s][0], max_threads);
            break;
           case NO_REB:                                    // don't do anything
            break;
         }
      }
     switch (reb)
      { case SEG0_REB:
         corrections_applied|=CORR_SEG0;
         break;
        case SSRB_REB:
         delete ssr;
         ssr=NULL;
         corrections_applied|=CORR_SSRB;
         break;
        case FORE_REB:
         delete fore_rebin;
         fore_rebin=NULL;
         corrections_applied|=CORR_FORE;
         break;
        case NO_REB:                                       // don't do anything
         break;
      }
                                                       // create new axes table
     axis_slices.resize(1);
     axis_slices[0]=2*GM::gm()->rings()-1;
     axes_slices=axis_slices[0];
     axis_size.resize(1);
     axis_size[0]=(unsigned long int)RhoSamples()*
                  (unsigned long int)ThetaSamples()*
                  (unsigned long int)axis_slices[0];
   }
   catch (...)
    { if (fore_rebin != NULL) delete fore_rebin;
      if (ssr != NULL) delete ssr;
      if (fp != NULL) delete fp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Smooth randoms sinogram.
    \param[in] loglevel      level for logging
    \param[in] max_threads   number of threads

    Smooth randoms sinogram. This method works for panel and ring detectors.
    The randoms smoothing for ring detectors is multi-threaded. Each thread
    processes a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::smoothRandoms(const unsigned short int loglevel,
                                       const unsigned short int max_threads)
 { if (!prompts_and_randoms) return;
   if (corrections_applied & CORR_Randoms_Smoothing) return;
   RandomsSmoothing *rs=NULL;
   GapFill *gf=NULL;

   try
   { unsigned short int rnd_sino;

     rnd_sino=numberOfSinograms()-1;
     Logging::flog()->logMsg("smooth randoms", loglevel);
                           // calculate randoms smoothing for ring architecture
     if (GM::gm()->ringArchitecture())
      { unsigned short int skip_interval, rnd_idx, axis;
        float *rnd_buffer, *rp;

        if (GM::gm()->transaxialBlockGaps() > 0)
         skip_interval=GM::gm()->transaxialCrystalsPerBlock()+
                       GM::gm()->transaxialBlockGaps();
         else skip_interval=std::numeric_limits <unsigned short int>::max();
        rnd_buffer=MemCtrl::mc()->createFloat(size(), &rnd_idx, "rnd",
                                              loglevel);
        rp=rnd_buffer;
        for (axis=0; axis < data[rnd_sino].size(); axis++)
         { memcpy(rp, MemCtrl::mc()->getFloatRO(data[rnd_sino][axis],
                                                loglevel),
                  size(axis)*sizeof(float));
           rp+=size(axis);
           MemCtrl::mc()->put(data[rnd_sino][axis]);
         }
        rs=new RandomsSmoothing(axis_slices, GM::gm()->rings(), span());
        rs->smooth(rnd_buffer, RhoSamples(), ThetaSamples(), axes_slices,
                   mash(), GM::gm()->crystalsPerRing(), skip_interval,
                   max_threads);
        delete rs;
        rs=NULL;

        rp=rnd_buffer;
        for (axis=0; axis < data[rnd_sino].size(); axis++)
         { memcpy(MemCtrl::mc()->getFloat(data[rnd_sino][axis], loglevel), rp,
                  size(axis)*sizeof(float));
           rp+=size(axis);
           MemCtrl::mc()->put(data[rnd_sino][axis]);
         }
        MemCtrl::mc()->put(rnd_idx);
        MemCtrl::mc()->deleteBlock(&rnd_idx);
      }
      else {              // calculate randoms smoothing for panel architecture
             gf=new GapFill(RhoSamples(), ThetaSamples());
             for (unsigned short int axis=0;
                  axis < data[rnd_sino].size();
                  axis++)
              { float *fp, *fp1;

                Logging::flog()->logMsg("smooth #1 planes of axis #2",
                                        loglevel+1)->arg(axis_slices[axis])->
                 arg(axis);
                fp=MemCtrl::mc()->getFloat(data[rnd_sino][axis], loglevel);
                fp1=fp;
                for (unsigned long int p=0; p < axis_slices[axis]; p++,
                     fp1+=planesize)
                 gf->calcRandomsSmooth(fp1);
                MemCtrl::mc()->put(data[rnd_sino][axis]);
              }
             delete gf;
             gf=NULL;
           }
     corrections_applied|=CORR_Randoms_Smoothing;    // set flag for correction
   }
   catch (...)
    { if (rs != NULL) delete rs;
      if (gf != NULL) delete gf;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Subtract sinogram from sinogram.
    \param[in,out] sino          subtrahend sinogram
    \param[in]     delete_sino   delete subtrahend after use ?
    \param[in]     cflag         flag for correction
    \param[in]     loglevel      level for logging

    Subtract sinogram from sinogram. This method is usually used to correct a
    sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::subtract(SinogramConversion * const sino,
                                  const bool delete_sino,
                                  const unsigned short int cflag,
                                  const unsigned short int loglevel)
 { Logging::flog()->logMsg("subtract sinograms", loglevel);
   for (unsigned short int s=0; s < MAX_SINO; s++)
    { unsigned short int sino_ds;
      bool del_sino;

      if (sino->index(0, s) ==
          std::numeric_limits <unsigned short int>::max()) sino_ds=0;
       else sino_ds=s;
      if ((s < MAX_SINO-1) && (data[s+1].size() > 0)) del_sino=false;
       else del_sino=delete_sino;
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { float *fp;

         fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
         vecSub(fp, MemCtrl::mc()->getFloatRO(sino->index(axis, sino_ds),
                                              loglevel), fp, axis_size[axis]);
         MemCtrl::mc()->put(data[s][axis]);
         MemCtrl::mc()->put(sino->index(axis, sino_ds));
         if (del_sino) sino->deleteData(axis, sino_ds);
       }
    }
   corrections_applied|=cflag;                       // set flag for correction
 }

/*---------------------------------------------------------------------------*/
/*! \brief Subtract 2d sinogram.
    \param[in,out] sino2d             2d subtrahend sinogram
    \param[in]     delete_sino        delete subtrahend after use ?
    \param[in]     cflag              flag for correction
    \param[in,out] total_minuend      sum of counts in minuend sinogram
    \param[in,out] total_subtrahend   sum of counts in subtrahend sinogram
    \param[in]     loglevel           level for logging

    Subtract 2d sinogram. The 2d sinogram is expanded into a 3d sinogram by
    inverse single-slice rebinning, if required. If the count numbers for the
    minuend and subtrahend are not needed, the parameters total_minuend and
    total_subtrahend can be set to NULL.

    \todo This code works only if the 2d sinogram dataset has the same number
    of bins and angles as the sinogram. It should be changed to resample the 2d
    sinogram as needed.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::subtract2D(SinogramConversion * const sino2d,
                                    const bool delete_sino,
                                    const unsigned short int cflag,
                                    double * const total_minuend,
                                    double * const total_subtrahend,
                                    const unsigned short int loglevel)
 { Logging::flog()->logMsg("subtract sinogram", loglevel);
   if (total_minuend != NULL) *total_minuend=0.0;
   if (total_subtrahend != NULL) *total_subtrahend=0.0f;
#if 0
   for (unsigned short int s=0; s < MAX_SINO; s++)
    {                                   // convert 2d sinogram into 3d sinogram
      sino2d->sino2D3D(iSSRB_REB, span(), mrd(), "sino", loglevel+1, 1);
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { float *fp, *sp;
         unsigned long int i;

         fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
         sp=MemCtrl::mc()->getFloatRO(sino2d->index(axis, s), loglevel);
         if (s == 0)                                     // calculate statistic
          { if (total_minuend != NULL)
             (*total_minuend)+=vecScalarAdd(fp, axis_size[axis]);
            if (total_subtrahend != NULL)
             (*total_subtrahend)+=vecScalarAdd(sp, axis_size[axis]);
          }
         vecSub(fp, sp, axis_size[axis]);                  // subtract sinogram
         MemCtrl::mc()->put(sino2d->index(axis, s));
         if (delete_sino) sino2d->deleteData(axis, s);
         MemCtrl::mc()->put(data[s][axis]);
       }
    }
#else
   for (unsigned short int s=0; s < MAX_SINO; s++)
    { float *sp;

      sp=MemCtrl::mc()->getFloatRO(sino2d->index(0, s), loglevel);
      for (unsigned short int axis=0; axis < data[s].size(); axis++)
       { float *fp;
         unsigned long int size, sc_offset;
                                // do inverse single-slice rebinning on the fly
         if (axis == 0) { size=axis_size[0];
                          sc_offset=0;
                        }
          else { size=axis_size[axis]/2;
                 sc_offset=(unsigned long int)
                           ((axis_slices[0]-axis_slices[axis]/2)/2)*
                           (unsigned long int)RhoSamples()*
                           (unsigned long int)ThetaSamples();
               }
         fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
         if (s == 0)                                    // calculate statistics
          { if (total_minuend != NULL)
             (*total_minuend)+=vecScalarAdd(fp, axis_size[axis]);
            if (total_subtrahend != NULL)
             { (*total_subtrahend)+=vecScalarAdd(&sp[sc_offset], size);
               if (axis > 0)
                (*total_subtrahend)+=vecScalarAdd(&sp[sc_offset], size);
             }
          }
                                                           // subtract sinogram
         vecSub(fp, &sp[sc_offset], fp, size);
         if (axis > 0)
          vecSub(&fp[size], &sp[sc_offset], &fp[size], size);
         MemCtrl::mc()->put(data[s][axis]);
       }
      MemCtrl::mc()->put(sino2d->index(0, s));
      if (delete_sino) sino2d->deleteData(0, s);
    }
#endif
   corrections_applied|=cflag;                       // set flag for correction
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert transmission sinogram into emission sinogram.
    \param[in] loglevel   level for logging

    Convert a transmission sinogram into an emission sinogram:
    \f[
        s_i=\max(1, \log(s_i))\quad \forall i
    \f]
    Values below 1 will be 0 in the emission sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::tra2Emi(const unsigned short int loglevel)
 { if (!acf_data) return;
   Logging::flog()->logMsg("convert transmission into emission data",
                           loglevel);
   for (unsigned short int s=0; s < MAX_SINO; s++)
    for (unsigned short int axis=0; axis < data[s].size(); axis++)
     { float *fp;

       fp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
       for (unsigned long int i=0; i < axis_size[axis]; i++)
        if (fp[i] > 1.0f) fp[i]=logf(fp[i]);
         else fp[i]=0.0f;                      // don't produce negative values
       MemCtrl::mc()->put(data[s][axis]);
     }
   acf_data=false;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Trim sinogram.
    \param[in] t          number of bins to cut of at each side of a projection
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging

    Trim sinogram by cutting of t bins at each side of a projection.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::trim(const unsigned short int t,
                              const std::string name,
                              const unsigned short int loglevel)
 { if (t == 0) return;
   float *fp=NULL;
   signed short int *ip=NULL;
   char *cp=NULL;
   uint32 *up=NULL;

   try
   { unsigned short int RhoSamplesTrim, axis;
     unsigned long int new_planesize;

     RhoSamplesTrim=RhoSamples()-2*t;
     Logging::flog()->logMsg("trim sinogram from #1x#2x#3 to #4x#5x#6",
                             loglevel)->arg(RhoSamples())->
      arg(ThetaSamples())->arg(axes_slices)->arg(RhoSamplesTrim)->
      arg(ThetaSamples())->arg(axes_slices);
     new_planesize=(unsigned long int)RhoSamplesTrim*
                   (unsigned long int)ThetaSamples();
     for (unsigned short int s=0; s < MAX_SINO; s++)
      for (axis=0; axis < data[s].size(); axis++)
       { switch (MemCtrl::mc()->dataType(data[s][axis]))
          { case MemCtrl::DT_FLT:
             { float *tsp, *fpp;

               fp=MemCtrl::mc()->getRemoveFloat(&data[s][axis], loglevel);
               fpp=fp;
               tsp=MemCtrl::mc()->createFloat(
                                          (unsigned long int)axis_slices[axis]*
                                          new_planesize, &data[s][axis], name,
                                          loglevel);
               for (unsigned long int i=0;
                    i < (unsigned long int)ThetaSamples()*
                        (unsigned long int)axis_slices[axis];
                    i++,
                    tsp+=RhoSamplesTrim,
                    fpp+=RhoSamples())
                memcpy(tsp, fpp+t, RhoSamplesTrim*sizeof(float));
               delete[] fp;
               fp=NULL;
             }
             break;
            case MemCtrl::DT_UINT:
             { uint32 *tsp, *ipp;

               up=MemCtrl::mc()->getRemoveUInt(&data[s][axis], loglevel);
               ipp=up;
               tsp=MemCtrl::mc()->createUInt(
                                          (unsigned long int)axis_slices[axis]*
                                          new_planesize, &data[s][axis], name,
                                          loglevel);
               for (unsigned long int i=0;
                    i < (unsigned long int)ThetaSamples()*
                        (unsigned long int)axis_slices[axis];
                    i++,
                    tsp+=RhoSamplesTrim,
                    ipp+=RhoSamples())
                memcpy(tsp, ipp+t, RhoSamplesTrim*sizeof(uint32));
               delete[] up;
               up=NULL;
             }
             break;
            case MemCtrl::DT_SINT:
             { signed short int *tsp, *ipp;

               ip=MemCtrl::mc()->getRemoveSInt(&data[s][axis], loglevel);
               ipp=ip;
               tsp=MemCtrl::mc()->createSInt(
                                          (unsigned long int)axis_slices[axis]*
                                          new_planesize, &data[s][axis], name,
                                          loglevel);
               for (unsigned long int i=0;
                    i < (unsigned long int)ThetaSamples()*
                        (unsigned long int)axis_slices[axis];
                    i++,
                    tsp+=RhoSamplesTrim,
                    ipp+=RhoSamples())
                memcpy(tsp, ipp+t, RhoSamplesTrim*sizeof(signed short int));
               delete[] ip;
               ip=NULL;
             }
             break;
            case MemCtrl::DT_CHAR:
             { char *tsp, *cpp;

               cp=MemCtrl::mc()->getRemoveChar(&data[s][axis], loglevel);
               cpp=cp;
               tsp=MemCtrl::mc()->createChar(
                                          (unsigned long int)axis_slices[axis]*
                                          new_planesize, &data[s][axis], name,
                                          loglevel);
               for (unsigned long int i=0;
                    i < (unsigned long int)ThetaSamples()*
                        (unsigned long int)axis_slices[axis];
                    i++,
                    tsp+=RhoSamplesTrim,
                    cpp+=RhoSamples())
                memcpy(tsp, cpp+t, RhoSamplesTrim*sizeof(char));
               delete[] cp;
               cp=NULL;
             }
             break;
          }
         axis_size[axis]=(unsigned long int)axis_slices[axis]*new_planesize;
         MemCtrl::mc()->put(data[s][axis]);
       }
     vRhoSamples=RhoSamplesTrim;
     planesize=new_planesize;
   }
   catch (...)
    { if (fp != NULL) delete[] fp;
      if (ip != NULL) delete[] ip;
      if (up != NULL) delete[] up;
      if (cp != NULL) delete[] cp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Tilt or untilt sinogram.
    \param[in] remove_tilt   remove tilt from sinogram ? (otherwise: add tilt)
    \param[in] loglevel      level for logging

    Tilt or untilt sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramConversion::untilt(const bool remove_tilt,
                                const unsigned short int loglevel)
 { if (intrinsic_tilt == 0.0f) return;
   if (remove_tilt && (corrections_applied & CORR_untilted)) return;
   if (!remove_tilt && !(corrections_applied & CORR_untilted)) return;
   signed short int angle;

   if (remove_tilt)
    Logging::flog()->logMsg("rotate sinogram to remove intrinsic tilt of #1 "
                            "degrees", loglevel)->arg(intrinsic_tilt);
    else Logging::flog()->logMsg("rotate sinogram to add intrinsic tilt of #1 "
                                 "degrees", loglevel)->arg(intrinsic_tilt);
   angle=(signed short int)(-(float)ThetaSamples()/180.0f*intrinsic_tilt);
   if (!remove_tilt) angle=-angle;
   for (unsigned short int s=0; s < MAX_SINO; s++)
    for (unsigned short int axis=0; axis < data[s].size(); axis++)
     { switch (MemCtrl::mc()->dataType(data[s][axis]))
       { case MemCtrl::DT_FLT:
          rotate(MemCtrl::mc()->getFloat(data[s][axis], loglevel),
                 RhoSamples(), ThetaSamples(), axis_slices[axis], angle);
          break;
         case MemCtrl::DT_UINT:
          rotate(MemCtrl::mc()->getUInt(data[s][axis], loglevel), RhoSamples(),
                 ThetaSamples(), axis_slices[axis], angle);
          break;
         case MemCtrl::DT_SINT:
          rotate(MemCtrl::mc()->getSInt(data[s][axis], loglevel), RhoSamples(),
                 ThetaSamples(), axis_slices[axis], angle);
          break;
         case MemCtrl::DT_CHAR:
          rotate(MemCtrl::mc()->getChar(data[s][axis], loglevel), RhoSamples(),
                 ThetaSamples(), axis_slices[axis], angle);
          break;
       }
       MemCtrl::mc()->put(data[s][axis]);
     }
                                                     // set flag for correction
   if (remove_tilt) corrections_applied|=CORR_untilted;
    else corrections_applied^=CORR_untilted;
 }
