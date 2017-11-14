/*! \class ImageConversion image_conversion.h "image_conversion.h"
    \brief This class provides methods to convert an image.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/03/23 initial version
    \date 2004/09/10 added trim() method
    \date 2004/09/10 fix memory leaks in resampleXY() and resampleZ()
    \date 2004/11/12 added CT2UMAP() method
    \date 2005/01/04 added progress reporting to CT2UMAP()

    This class provides methods to convert an image. Available conversions are:
    <ul>
     <li>application of a lower threshold</li>
     <li>decay- and frame-length correction</li>
     <li>gaussian filter in x/y- and z-direction</li>
     <li>application of a circular mask</li>
     <li>resampling in x/y-direction</li>
     <li>resampling in z-direction</li>
     <li>trimming in x/y-direction</li>
     <li>convert CT into u-map</li>
    </ul>
 */

#include <cmath>
#include "image_conversion.h"
#include "const.h"
#include "e7_common.h"
#include "global_tmpl.h"
#include "image_filter.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "progress.h"
#include "rebin_xy.h"
#include "types.h"
#include "vecmath.h"

/*- definitions -------------------------------------------------------------*/

// define some abbreviations for the linear and bilinear interpolation code
// This is dirty and not type safe, compared to inline functions but for some
// unknown reason its faster.

                                       // linear interpolation between two bins
#define _linearInterp(f, val1, val2) (val1+(f)*(val2-(val1)))

          // add result of linear interpolation between two bins to image voxel
#define _linearInterpolation(img, f, val1, val2)\
 img+=_linearInterp(f, val1, val2);

                                    // bilinear interpolation between four bins
#define _bilinearInterp(v, f1, f2, val1, val2, val3, val4)\
 { float _a, _b;\
\
   _a=_linearInterp(f1, val1, val2);\
   _b=_linearInterp(f1, val3, val4);\
   v=_linearInterp(f2, _a, _b);\
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize the object.
    \param[in] _XYSamples        width/height of the image
    \param[in] _DeltaXY          width/height of a voxel in mm
    \param[in] _ZSamples         depth of the image
    \param[in] _DeltaZ           depth of a voxel in mm
    \param[in] _mnr              matrix number of image
    \param[in] _lld              lower level energy threshold in keV
    \param[in] _uld              upper level energy threshold in kev
    \param[in] _start_time       start time of scan in sec
    \param[in] _frame_duration   duration of scan frame in sec
    \param[in] _gate_duration    duration of scan gate in sec
    \param[in] _halflife         halflife of isotope in sec
    \param[in] _bedpos           horizontal position of the bed

    Initialize the object.
 */
/*---------------------------------------------------------------------------*/
ImageConversion::ImageConversion(const unsigned short int _XYSamples,
                                 const float _DeltaXY,
                                 const unsigned short int _ZSamples,
                                 const float _DeltaZ,
                                 const unsigned short int _mnr,
                                 const unsigned short int _lld,
                                 const unsigned short int _uld,
                                 const float _start_time,
                                 const float _frame_duration,
                                 const float _gate_duration,
                                 const float _halflife,
                                 const float _bedpos):
 ImageIO(_XYSamples, _DeltaXY, _ZSamples, _DeltaZ, _mnr, _lld, _uld,
         _start_time, _frame_duration, _gate_duration, _halflife, _bedpos)
 {
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert a CT image into a series of PET u-maps.
    \param[in] bed_position    positions of the PET beds in mm
    \param[in] ct2pet_offs_x   x-offet from CT to PET coordinates
    \param[in] ct2pet_offs_y   y-offet from CT to PET coordinates
    \param[in] ct2pet_offs_z   z-offet from CT to PET coordinates
    \param[in] gauss_fwhm_xy   FWHM of gaussian in x/y-direction in mm
    \param[in] gauss_fwhm_z    FWHM of gaussian in z-direction in mm
    \param[in] loglevel        level for logging
    \param[in] max_threads     number of threads to use

    Convert a CT image into a series of PET u-maps. The CT image is rebinned
    into the size of the resulting u-map. Then bilinear interpolation is used
    to adjust the CT image to the PET position. The Houndsfield units of the
    CT are converted into the 511 keV range of the PET u-map and the u-maps
    are smoothed with a 3d gaussian.
    The smoothing operations is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::CT2UMAP(const std::vector <float> bed_position,
                              const float ct2pet_offs_x,
                              const float ct2pet_offs_y,
                              const float ct2pet_offs_z,
                              const float gauss_fwhm_xy,
                              const float gauss_fwhm_z,
                              const unsigned short int loglevel,
                              const unsigned short int max_threads)
 { RebinXY *reb=NULL;
   float *temp=NULL;

   try
   { float *dp, ct_pixsize, ct_rebin_factor, pet_pixsize, ct2pet_xy_fov_offset;
     unsigned short int ct_XYSamples, pet_XYSamples, i;
     signed short int ct_direction, pet_direction, ct2pet_direction;
     unsigned long int umap_slicesize, umap_size, j;
     std::vector <float> xind, yind, ct_zpos, conversion_parms;
     std::vector <unsigned short int> pet_umap_idx;
     std::string mu;
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     char c[2];

     c[1]=0;
     c[0]=(char)181;
     mu=std::string(c);
#endif
#ifdef WIN32
     mu='u';
#endif
                 // conversion parameters from Houndsfield units to 511 keV PET
     conversion_parms.resize(4);
     conversion_parms[0]=0.0000961245f;
     conversion_parms[1]=0.0000789431f;
     conversion_parms[2]=0.0f;
     conversion_parms[3]=1276.0f;
                             // positions of CT slices in PET coordinate system
     ct_zpos.resize(ZSamples());
     for (i=0; i < ct_zpos.size(); i++)
      ct_zpos[i]=-DeltaZ()*(float)i+ct2pet_offs_z+bedPos();
// std::cerr << "CT slice " << ct_zpos[i] << " " << ct2pet_offs_z << " " << bedPos() << std::endl;

     dp=MemCtrl::mc()->getFloat(index(), loglevel);
             // add offset for the Toshiba CT values (units from HU to HU+1024)
     for (j=0; j < size(); j++)
      if (dp[j] > -1024.0f) dp[j]+=1024.0f;
       else dp[j]=0.0f;
                                                    // calculate size of u-maps
     pet_XYSamples=128;
     ct_XYSamples=XYSamples();
     if (ct_XYSamples % pet_XYSamples)
      { while (ct_XYSamples > 128) ct_XYSamples>>=1;
        pet_XYSamples=ct_XYSamples;
      }
     ct_rebin_factor=(float)ct_XYSamples/(float)pet_XYSamples;
     ct_XYSamples/=ct_rebin_factor;
     ct_pixsize=DeltaXY()*ct_rebin_factor;
     pet_pixsize=(GM::gm()->RhoSamples()*GM::gm()->BinSizeRho()/
                  (float)pet_XYSamples)*
                 (GM::gm()->ringRadius()+GM::gm()->DOI())/
                 GM::gm()->ringRadius();
     ct2pet_xy_fov_offset=(pet_XYSamples*pet_pixsize-
                           ct_XYSamples*ct_pixsize)/2.0f;
                                // calculate factors for bilinear interpolation
     xind.resize(ct_XYSamples);
     yind.resize(ct_XYSamples);
     for (i=0; i < pet_XYSamples; i++)
      { float xyind;

        xyind=(float)i*pet_pixsize/ct_pixsize-0.5f-
              ct2pet_xy_fov_offset/ct_pixsize;
        xind[i]=xyind+ct2pet_offs_x/ct_pixsize;
        yind[i]=xyind+ct2pet_offs_y/ct_pixsize;
      }
     // pet_total_planes=petplan.nbeds*petplan.nplanes-
     //                  (petplan.nbeds-1)*petplan.nover;
     // pet_axial_scan_length=(float)pet_total_planes*petplan.planeSeparation;
     pet_direction=-1;
     // pet_start_zpos=petplan.start_bedpos;
     // pet_end_zpos=pet_start_zpos+(float)pet_direction*pet_axial_scan_length;
     // petct_zoverlap1=compute_zoverlap(ct_zpos, pet_start_zpos,
     //                                  pet_end_zpos);
     /*
     pet_direction=-1;
     pet_end_zpos=pet_start_zpos+(float)pet_direction*pet_axial_scan_length;
     petct_zoverlap_minus1=compute_zoverlap(ct_zpos, pet_start_zpos,
                                            pet_end_zpos);
     if (petct_zoverlap1 > petct_zoverlap_minus1) pet_direction=1;
     pet_direction=1;*/
     if (ct_zpos[0] > ct_zpos[ct_zpos.size()-1]) ct_direction=-1;
      else ct_direction=1;
     ct2pet_direction=pet_direction*ct_direction;
     umap_slicesize=(unsigned long int)pet_XYSamples*
                    (unsigned long int)pet_XYSamples;
     umap_size=umap_slicesize*(unsigned long int)(GM::gm()->rings()*2-1);
     pet_umap_idx.assign(bed_position.size(), MemCtrl::MAX_BLOCK);
     reb=new RebinXY(XYSamples(), 1, XYSamples(), 1, ct_XYSamples,
                     (float)XYSamples()/(float)ct_XYSamples, ct_XYSamples,
                     (float)XYSamples()/(float)ct_XYSamples, true);
     for (unsigned short int bed=0; bed < bed_position.size(); bed++)
      { float *pet_umap_vol, *pptr;
        signed short int ct_zstart_idx;

        Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 1,
                                 "convert CT image into u-map (bed #1)")->
         arg(bed);
        Logging::flog()->logMsg("create bed #1 of #2-map (#3x#4x#5 "
                                "#6mm x #7mm x #8mm) at position #9mm",
                                loglevel)->arg(bed)->arg(mu)->
         arg(pet_XYSamples)->arg(pet_XYSamples)->arg(GM::gm()->rings()*2-1)->
         arg(pet_pixsize)->arg(pet_pixsize)->arg(GM::gm()->planeSeparation())->
         arg(bed_position[bed]);
        pet_umap_vol=MemCtrl::mc()->createFloat(umap_size, &pet_umap_idx[bed],
                                                "ctimg", loglevel);
        memset(pet_umap_vol, 0, umap_size*sizeof(float));
        pptr=pet_umap_vol;
        ct_zstart_idx=round((ct_zpos[0]-bed_position[bed])/DeltaZ());
        //std::cerr << "start idx=" << bed_position[bed] << " " << ct_zstart_idx << " " << ct_zpos[0] << std::endl;
        for (unsigned short int plane=0;
             plane < GM::gm()->rings()*2-1;
             plane++,
             pptr+=umap_slicesize)
         { signed short int ct_p;
                                  // calculate number of CT slice for PET slice
           ct_p=ct_zstart_idx+round((float)ct2pet_direction*(float)plane*
                                    GM::gm()->planeSeparation()/DeltaZ());
           //std::cerr << "use CT slice " << ct_p << std::endl;
           if ((ct_p >= 0) && (ct_p < ZSamples()))
            {                             // resample CT slice to size of u-map
              temp=reb->calcRebinXY(&dp[(unsigned long int)ct_p*
                                        (unsigned long int)XYSamples()*
                                        (unsigned long int)XYSamples()], 1);
                // use bilinear interpolation to adjust position from CT to PET
              for (unsigned short int y=0; y < pet_XYSamples; y++)
               { signed short int yfloor;

                 yfloor=(signed short int)yind[y];
                 if ((yfloor >= 0) && (yfloor < pet_XYSamples-1))
                  for (unsigned short int x=0; x < pet_XYSamples; x++)
                   { signed short int xfloor;

                     xfloor=(signed short int)xind[x];
                     if ((xfloor >= 0) && (xfloor < pet_XYSamples-1))
                      { float *ctptr;

                        ctptr=temp+(unsigned long int)yfloor*
                                   (unsigned long int)ct_XYSamples+xfloor;
                        _bilinearInterp(pptr[(unsigned long int)y*
                                           (unsigned long int)pet_XYSamples+x],
                                        xind[x]-(float)xfloor,
                                        yind[y]-(float)yfloor, ctptr[0],
                                        ctptr[1], ctptr[ct_XYSamples],
                                        ctptr[ct_XYSamples+1]);
                      }
                   }
               }
              delete[] temp;
              temp=NULL;
            }
         }
        Logging::flog()->logMsg("convert Houndsfield units to 511 keV PET",
                                loglevel);
                                        // change units from HU+1024 to HU+1000
        vecAddScalar(pet_umap_vol, -24.0f, pet_umap_vol, umap_size);
        for (j=0; j < umap_size; j++)
         { if (pet_umap_vol[j] <= conversion_parms[3])
            pet_umap_vol[j]*=conversion_parms[0];                // soft tissue
            else pet_umap_vol[j]=conversion_parms[1]*pet_umap_vol[j]+   // bone
                                 conversion_parms[2];
                                                     // cut off negative values
           if (pet_umap_vol[j] < 0.0f) pet_umap_vol[j]=0.0f;
         }
        MemCtrl::mc()->put(pet_umap_idx[bed]);
      }
     delete reb;
     reb=NULL;
     MemCtrl::mc()->put(index());
     MemCtrl::mc()->deleteBlock(&data_idx[0]);
     data_idx=pet_umap_idx;
     vDeltaXY=pet_pixsize;
     vDeltaZ=GM::gm()->planeSeparation();
     vXYSamples=pet_XYSamples;
     vZSamples=GM::gm()->rings()*2-1;
     doi_corrected=true;
                                          // calculate size of the image volume
     image_size=(unsigned long int)XYSamples()*(unsigned long int)XYSamples()*
                (unsigned long int)ZSamples();
     bedpos=bed_position;
                                                                // filter u-map
     gaussFilter(gauss_fwhm_xy, gauss_fwhm_z, loglevel, max_threads);
     if ((inf != NULL) && (bedpos.size() > 1))
      if (bedpos[1] > bedpos[0]) inf->Main_scan_direction(KV::SCAN_OUT);
       else inf->Main_scan_direction(KV::SCAN_IN);
   }
   catch (...)
    { if (reb != NULL) delete reb;
      if (temp != NULL) delete temp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Cut-off values above threshold.
    \param[in] threshold     threshold
    \param[in] replacement   replacement value
    \param[in] loglevel      level for logging

    All voxel values above the threshold are replaced by a replacement value.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::cutAbove(const float threshold, const float replacement,
                               const unsigned short int loglevel) const
 { float *dp;

   Logging::flog()->logMsg("replace values above #1 by #2", loglevel)->
    arg(threshold)->arg(replacement);
   dp=MemCtrl::mc()->getFloat(index(), loglevel);
   for (unsigned long int i=0; i < size(); i++)
    if (dp[i] > threshold) dp[i]=replacement;
   MemCtrl::mc()->put(index());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Cut-off values below threshold.
    \param[in] threshold     threshold
    \param[in] replacement   replacement value
    \param[in] loglevel      level for logging

    All voxel values below the threshold are replaced by a replacement value.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::cutBelow(const float threshold, const float replacement,
                               const unsigned short int loglevel) const
 { float *dp;

   Logging::flog()->logMsg("replace values below #1 by #2", loglevel)->
    arg(threshold)->arg(replacement);
   dp=MemCtrl::mc()->getFloat(index(), loglevel);
   for (unsigned long int i=0; i < size(); i++)
    if (dp[i] < threshold) dp[i]=replacement;
   MemCtrl::mc()->put(index());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Decay and frame-length correction.
    \param[in] decay_corr      calculate decay correction ?
    \param[in] framelen_corr   calculate frame-length correction ?
    \param[in] loglevel        level for logging
    \return decay factor

    Calculate the decay and frame-length correction. The decay factor is
    calculated by the following equations:
    \f[
        \lambda=-\frac{\ln(\frac{1}{2})}{halflife}
    \f]
    \f[
        decay\_factor=\frac{e^{\lambda frame\_start}\lambda frame\_duration}
                           {1-e^{-\lambda frame\_duration}}
    \f]
    Images that are already corrected are not corrected again.
 */
/*---------------------------------------------------------------------------*/
float ImageConversion::decayCorrect(bool decay_corr, bool framelen_corr,
                                    const unsigned short int loglevel)
 {                                       // don't correct, if already corrected
   if (corrections_applied & CORR_Decay_correction) decay_corr=false;
   if (corrections_applied & CORR_FrameLen_correction) framelen_corr=false;
   if (!decay_corr && !framelen_corr) return(1.0f);
   float decay_factor, scale_factor;
                // calculate decay correction factor and scale factor for image
   decay_factor=calcDecayFactor(startTime(), frameDuration(), gateDuration(),
                                halflife(), decay_corr, framelen_corr,
                                &scale_factor, loglevel);
   scale(scale_factor, loglevel+1);                              // scale image
                                                     // update correction flags
   if (framelen_corr) corrections_applied|=CORR_FrameLen_correction;
   if (decay_corr)
    { corrections_applied|=CORR_Decay_correction;
      Logging::flog()->logMsg("decay correction factor=#1", loglevel+1)->
       arg(decay_factor);
    }
   return(decay_factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Gaussian smoothing of image in x/y- and z-direction.
    \param[in] fwhm_xy       FWHM of gaussian in x/y-direction in mm
    \param[in] fwhm_z        FWHM of gaussian in z-direction in mm
    \param[in] loglevel      level for logging
    \param[in] max_threads   number of threads to use

    Apply a gaussian filter to smooth the image. The smoothing is done in
    image space. The max_threads parameter is currently ignored.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::gaussFilter(const float fwhm_xy, const float fwhm_z,
                                  const unsigned short int loglevel,
                                  const unsigned short int max_threads)
 { Filter *filter=NULL;

   try
   { if ((fwhm_xy <= 0.0f) && (fwhm_z <= 0.0f)) return;
     if (fwhm_xy <= 0.0f)
      Logging::flog()->logMsg("gaussian filter in z-direction (#1mm)",
                              loglevel)->arg(fwhm_z);
      else if (fwhm_z <= 0.0f)
            Logging::flog()->logMsg("gaussian filter in x- and y-direction "
                                    "(#1mm,#2mm)", loglevel)->arg(fwhm_xy)->
             arg(fwhm_xy);
            else Logging::flog()->logMsg("gauss filter in x-,y- and "
                                         "z-direction (#1mm,#2mm)",
                                         loglevel)->arg(fwhm_xy)->
                  arg(fwhm_xy)->arg(fwhm_z);
     if (fwhm_xy > 0.0f)
      { tfilter f;
        float sf;

        sf=(GM::gm()->ringRadius()+GM::gm()->DOI())/GM::gm()->ringRadius();
                                                  // filtering in x/y-direction
        f.name=GAUSS;
        f.restype=FWHM;
        f.resolution=fwhm_xy;                        // filter-width in mm FWHM
        filter=new Filter(XYSamples(), ZSamples(), f, DeltaXY()*sf, 1.0f, true,
                          max_threads);
        for (unsigned short int i=0; i < data_idx.size(); i++)
         { filter->calcFilter(MemCtrl::mc()->getFloat(data_idx[i], loglevel),
                              max_threads);
           MemCtrl::mc()->put(data_idx[i]);
         }
        delete filter;
        filter=NULL;
        corrections_applied|=CORR_XYSmoothing;
      }
     if (fwhm_z > 0.0f)
      { tfilter f;
                                                    // filtering in z-direction
        f.name=GAUSS;
        f.restype=FWHM;
        f.resolution=fwhm_z;                         // filter-width in mm FWHM
        filter=new Filter(XYSamples(), ZSamples(), f, DeltaZ(), 1.0f, false,
                          max_threads);
        for (unsigned short int i=0; i < data_idx.size(); i++)
         { filter->calcFilter(MemCtrl::mc()->getFloat(data_idx[i], loglevel),
                              max_threads);
           MemCtrl::mc()->put(data_idx[i]);
         }
        delete filter;
        filter=NULL;
        corrections_applied|=CORR_ZSmoothing;
      }
     if (gauss_fwhm_xy == 0.0f) gauss_fwhm_xy=fwhm_xy;
      else gauss_fwhm_xy=sqrt(fwhm_xy*fwhm_xy+gauss_fwhm_xy*gauss_fwhm_xy);
     if (gauss_fwhm_z == 0.0f) gauss_fwhm_z=fwhm_z;
      else gauss_fwhm_z=sqrt(fwhm_z*fwhm_z+gauss_fwhm_z*gauss_fwhm_z);
   }
   catch (...)
    { if (filter != NULL) delete filter;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Apply circular mask to image.
    \param[in,out] fov_diameter   diameter of mask in mm
    \param[in]     loglevel       level for logging

    Apply circular mask to image. The diameter of the mask equals the width of
    the image. Voxels outside of the mask are set to zero. The parameter
    fov_diameter can be set to NULL, if this value is not needed.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::maskXY(float * const fov_diameter,
                             const unsigned short int loglevel) const
 { signed long int rsq_max;
   float *rp;

   Logging::flog()->logMsg("mask image", loglevel);
   rsq_max=(unsigned long int)(XYSamples()/2-2)*
           (unsigned long int)(XYSamples()/2-2);
   if (fov_diameter != NULL)
    *fov_diameter=(float)(XYSamples()/2-2)*DeltaXY()*2.0f;
   rp=MemCtrl::mc()->getFloat(index(), loglevel);
   for (unsigned short int z=0; z < ZSamples(); z++)
    for (unsigned short int y=0; y < XYSamples(); y++)
     { signed long int yp2;

       yp2=(signed long int)y-(signed long int)XYSamples()/2;
       yp2*=yp2;
       for (unsigned short int x=0; x < XYSamples(); x++,
            rp++)
        { signed long int xp;

          xp=(signed long int)x-(signed long int)XYSamples()/2;
          if (xp*xp+yp2 > rsq_max) *rp=0.0f;
        }
     }
   MemCtrl::mc()->put(index());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print statistics about image.
    \param[in] loglevel   level for logging

    Print statistics about image.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::printStat(const unsigned short int loglevel) const
 { ::printStat("counts in image", loglevel,
               MemCtrl::mc()->getFloatRO(index(), loglevel), size(),
               (float *)NULL, (float *)NULL, NULL);
   MemCtrl::mc()->put(index());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Resample the image in x/y-direction.
    \param[in] newXYSamples      new width/height of image
    \param[in] newDeltaXY        new voxel with/height in mm
    \param[in] preserve_values   preserve voxel values (otherwise preserve sum
                                 of counts)
    \param[in] loglevel          level for logging

    Resample the image in x/y-direction by bilinear interpolation.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::resampleXY(const unsigned short int newXYSamples,
                                 const float newDeltaXY,
                                 const bool preserve_values,
                                 const unsigned short int loglevel)
 { if ((newXYSamples == XYSamples()) && (newDeltaXY == DeltaXY())) return;
   RebinXY *reb=NULL;
   float *reb_data=NULL, *fp=NULL;

   try
   { Logging::flog()->logMsg("rebin image from #1x#2x#3 (#4x#5x#6 mm^3) to "
                             "#7x#8x#9 (#10x#11x#12 mm^3)",
                             loglevel)->arg(XYSamples())->arg(XYSamples())->
      arg(ZSamples())->arg(DeltaXY())->arg(DeltaXY())->arg(DeltaZ())->
      arg(newXYSamples)->arg(newXYSamples)->arg(ZSamples())->
      arg(newDeltaXY)->arg(newDeltaXY)->arg(DeltaZ());
     reb=new RebinXY(XYSamples(), DeltaXY(), XYSamples(), DeltaXY(),
                     newXYSamples, newDeltaXY, newXYSamples, newDeltaXY,
                     preserve_values);
     fp=getRemove(loglevel);
     reb_data=reb->calcRebinXY(fp, ZSamples());
     delete reb;
     reb=NULL;
     delete[] fp;
     fp=NULL;
     vDeltaXY=newDeltaXY;
     vXYSamples=newXYSamples;
     image_size=(unsigned long int)XYSamples()*
                (unsigned long int)XYSamples()*
                (unsigned long int)ZSamples();
     MemCtrl::mc()->putFloat(&reb_data, image_size, &data_idx[0], "img",
                             loglevel);
   }
   catch (...)
    { if (reb != NULL) delete reb;
      if (reb_data != NULL) delete[] reb_data;
      if (fp != NULL) delete[] fp;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Resample the image in z-direction.
    \param[in] newZSamples   new depth of image
    \param[in] zoom          zoom factor
    \param[in] loglevel      level for logging

    Resample the image in z-direction by linear interpolation. The zoom factor
    is chosen independently from the number of planes. If needed, the last
    plane of the original image is extended. This way it is possible, to
    resample 239 planes into 60 planes with an integral zoom factor of 4.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::resampleZ(const unsigned short int newZSamples,
                                const unsigned short int zoom,
                                const unsigned short int loglevel)
 { if (newZSamples == ZSamples()) return;
   float *up=NULL;

   try
   { float *buffer;
     unsigned long int new_image_size, image_planesize;

     Logging::flog()->logMsg("rebin image from #1 planes to #2 planes",
                             loglevel)->arg(ZSamples())->arg(newZSamples);
     image_planesize=(unsigned long int)XYSamples()*
                     (unsigned long int)XYSamples();
     new_image_size=image_planesize*(unsigned long int)newZSamples;
     up=getRemove(loglevel);
     data_idx.resize(1);
     buffer=MemCtrl::mc()->createFloat(new_image_size, &data_idx[0], "img",
                                       loglevel);
     memset(buffer, 0, new_image_size*sizeof(float));
     for (unsigned short int z=0; z < newZSamples; z++,
          buffer+=image_planesize)
      { float zp, f, *up2_ptr, *up_ptr;
        unsigned short int sl;

        zp=(float)z/zoom;
        sl=(unsigned short int)zp;
        f=zp-(float)sl;
        up_ptr=up+(unsigned long int)sl*image_planesize;
        if (sl >= ZSamples()) break;
         else if (sl == ZSamples()-1) { up2_ptr=up_ptr;
                                        f=0.0f;
                                      }
               else up2_ptr=up_ptr+image_planesize;
                                                        // linear interpolation
        vecMulAdd(up_ptr, 1.0f-f, up2_ptr, f, buffer, image_planesize);
      }
     delete[] up;
     up=NULL;
     MemCtrl::mc()->put(index());
     vDeltaZ/=zoom;
     vZSamples=newZSamples;
     image_size=new_image_size;
   }
   catch (...)
    { if (up != NULL) delete[] up;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Trim image.
    \param[in] t          number of pixels to cut of at each side of a plane
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging

    Trim image by cutting of t pixels at each side of a plane.
 */
/*---------------------------------------------------------------------------*/
void ImageConversion::trim(const unsigned short int t, const std::string name,
                           const unsigned short int loglevel)
 { if (t == 0) return;
   float *fp=NULL;

   try
   { unsigned short int XYSamplesTrim;
     unsigned long int image_planesize, new_image_size;
     float *fpp, *ip;

     XYSamplesTrim=XYSamples()-2*t;
     Logging::flog()->logMsg("trim image from #1x#2x#3 to #4x#5x#6",
                             loglevel)->arg(XYSamples())->
      arg(XYSamples())->arg(ZSamples())->arg(XYSamplesTrim)->
      arg(XYSamplesTrim)->arg(ZSamples());
     image_planesize=(unsigned long int)XYSamples()*
                     (unsigned long int)XYSamples();
     new_image_size=(unsigned long int)XYSamplesTrim*
                    (unsigned long int)XYSamplesTrim*
                    (unsigned long int)ZSamples();
     fp=getRemove(loglevel);
     fpp=fp;
     data_idx.resize(1);
     ip=MemCtrl::mc()->createFloat(new_image_size, &data_idx[0], name,
                                   loglevel);
     for (unsigned short int z=0; z < ZSamples(); z++,
          fpp+=image_planesize)
      for (unsigned long int y=0; y < XYSamplesTrim; y++,
           ip+=XYSamplesTrim)
       memcpy(ip, fpp+(unsigned long int)(t+y)*
                      (unsigned long int)XYSamplesTrim+t,
              XYSamplesTrim*sizeof(float));
     delete[] fp;
     fp=NULL;
     MemCtrl::mc()->put(index());
     image_size=new_image_size;
     vXYSamples=XYSamplesTrim;
   }
   catch (...)
    { if (fp != NULL) delete[] fp;
      throw;
    }
 }
