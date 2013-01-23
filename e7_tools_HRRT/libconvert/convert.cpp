/*! \file convert.cpp
    \brief This module provides methods to convert sinograms into images and
           vice versa.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/04/01 initial version
    \date 2004/05/06 no arc-correction for acf from PET/CT gantry
    \date 2004/05/28 call IDL scatter code
    \date 2004/10/01 added Doxygen style comments
    \date 2004/12/17 added support for 2395
    \date 2005/01/04 added progress reporting to get_umap_acf() and
                     eau2scatter()
    \date 2005/03/08 fixed loading of u-map if GM is not initialized
    \date 2005/03/11 added PSF-AW-OSEM

    This module provides methods to convert sinograms into images and vice
    versa.
 */

#include <cstdio>
#include <cstdlib>
#ifdef __LINUX__
#include <dlfcn.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif
#include "convert.h"
#include "atten_reco.h"
#include "dift.h"
#include "e7_common.h"
#include "fbp.h"
#include "global_tmpl.h"
#ifdef SUPPORT_NEW_SCATTER_CODE
#if defined(WIN32) || defined(__SOLARIS__) || defined(__LINUX__)
#include "idl_interface.h"
#endif
#endif // SUPPORT_NEW_SCATTER_CODE
#include "logging.h"
#include "mem_ctrl.h"
#include "osem3d.h"
#include "progress.h"
//#include "psf_matrix.h"
#include "scatter.h"
#include "stopwatch.h"
#include "swap_tmpl.h"
#include "types.h"

namespace CONVERT
{
/*- constants ---------------------------------------------------------------*/

                          /*! max number of scatter samples in x/y-direction */
const unsigned short int max_scatter_samps_xy=23;
                            /*! max number of scatter samples in z-direction */
const unsigned short int max_scatter_samps_z=45;
                       /*! zoom factor for images used in scatter correction */
const unsigned short int scatter_img_zoom=2;
                  /*! sample spacing along rays in mm for scatter simulation */
const float step_size=20;
           /*! axial interval in mm at which scatter sinograms are simulated */
const float scatter_z_step=25.0f;
                   /*! width/height of voxels in scatter sampling grid in mm */
const float sampling_grid_xy=22.5f;
                          /*! depth of voxels in scatter sampling grid in mm */
const float sampling_grid_z=22.5f;
                           /*! correction factor for scatter scaling factors */
const float underestimate=1.0f;//0.9f;

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Calculate u-map from blank and transmission scan.
    \param[in] blank_sino                blank sinogram
    \param[in] tra_sino                  transmission sinogram
    \param[in] rebin_r                   radial rebinning factor that was
                                         applied to sinogram
    \param[in] rebin_t                   azimuthal rebinning factor that was
                                         applied to sinogram
    \param[in] XYSamples                 width/height of u-map
    \param[in] ZSamples                  number of slices in u-map
    \param[in] iterations                number of iterations for OSEM
                                         reconstruction
    \param[in] subsets                   number of subsets for OSEM
                                         reconstruction

    \param[in] alpha                     relaxation parameter
    \param[in] beta                     regularization parameter for
                                         smoothing prior
    \param[in] beta_ip                   regularisation parameter for
                                         intensity prior
    \param[in] iterations_ip             first iteration for intensity prior
    \param[in] gaussian_smoothing        use gaussian smoothing model ?
                                         (otherwise: Geman-McClure smoothing
                                                     model)
    \param[in] geman_mcclure_threshold   threshold for Geman-McClure smoothing
    \param[in] number_of_segments        number of segments for segmentation of
                                         u-map
    \param[in] mu_mean                   mean values of gaussian priors
    \param[in] mu_std                    standard deviations for gaussian
                                         priors
    \param[in] gauss_intersect           boundaries between segments
    \param[in] scaling_factor            scaling factor for u-map
    \param[in] autoscale                 auto-scale u-map towards water-peak ?
    \param[in] autoscalevalue            u-value for auto-scaling
    \param[in] gauss_fwhm_xy             fwhm in mm of gauss-filter for
                                         x/y-direction
    \param[in] gauss_fwhm_z              fwhm in mm of gauss-filter for
                                         z-direction
    \param[in] umap_xy_zoom              zoom factor for u-map (x/y-direction)
    \param[in] umap_z_zoom               zoom factor for u-map (z-direction)
    \param[in] skip_outer_planes         skip first and last plane ?
    \param[in] umap_qc                   store quality control data of u-map
                                         reconstruction ?
    \param[in] umap_qc_path              path for quality control data of u-map
                                         reconstruction
    \param[in] mnr                       matrix number
    \param[in] db_path                   path for blank database
    \param[in] db_frames                 number of frames in blank database
    \param[in] fov_diameter              diameter of reconstruced FOV
    \param[in] loglevel                  top level for logging
    \param[in]  max_threads              maximum number of threads to use
    \return u-map
 
    Calculate a u-map from a blank and a transmission scan.
 */
/*---------------------------------------------------------------------------*/
ImageConversion *bt2umap(SinogramConversion * blank_sino,
                         SinogramConversion * tra_sino,
                         const unsigned short int rebin_r,
                         const unsigned short int rebin_t,
                         const unsigned short int XYSamples,
                         const unsigned short int ZSamples,
                         const unsigned short int iterations,
                         const unsigned short int subsets,
                         const float alpha, const float beta,
                         const float beta_ip,
                         const unsigned short int iterations_ip,
                         const bool gaussian_smoothing,
                         const float geman_mcclure_threshold,
                         const unsigned short int number_of_segments,
                         float * const mu_mean, float * const mu_std,
                         float * const gauss_intersect,
                         const float scaling_factor, const bool autoscale,
                         const float autoscalevalue,
                         const float gauss_fwhm_xy,
                         const float gauss_fwhm_z,
                         const unsigned short int umap_xy_zoom,
                         const unsigned short int umap_z_zoom,
                         const bool skip_outer_planes, const bool umap_qc,
                         const std::string umap_qc_path,
                         const unsigned short int mnr,
                         const std::string db_path,
                         const unsigned short int db_frames,
                         float * const fov_diameter,
                         const unsigned short int loglevel,
                         const unsigned short int max_threads)
 { AttenReco *tr=NULL;
   ImageConversion *umap_image=NULL;
   float *umap=NULL;
   StopWatch sw;

   try
   { unsigned short int umap_z, uidx;
     std::string str, mu;
     uint64 tx_singles=0;

#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
     char c[2];

     c[1]=0;
     c[0]=(char)181;
     mu=std::string(c);
#endif
#ifdef WIN32
     mu='u';
#endif
     umap_z=(unsigned short int)(ZSamples/umap_z_zoom);
     if (umap_z*umap_z_zoom < ZSamples) umap_z++;

     Logging::flog()->logMsg("start calculation of #1-map", loglevel)->arg(mu);
     sw.start();
                                                         // create u-map object
     umap_image=new ImageConversion((unsigned short int)
                                    (XYSamples/umap_xy_zoom),
                                    blank_sino->BinSizeRho()*umap_xy_zoom,
                                    umap_z,
                                    GM::gm()->planeSeparation()*umap_z_zoom,
                                    mnr, tra_sino->LLD(), tra_sino->ULD(),
                                    tra_sino->startTime(),
                                    tra_sino->frameDuration(),
                                    tra_sino->gateDuration(),
                                    tra_sino->halflife(), tra_sino->bedPos());
                                        // initialize object for reconstruction
     tr=new AttenReco(umap_image->XYSamples(), blank_sino->RhoSamples(),
                      blank_sino->BinSizeRho(), blank_sino->ThetaSamples(),
                      rebin_r, rebin_t, ZSamples, umap_xy_zoom, umap_z_zoom,
                      loglevel+1);
     printStat("Blank", loglevel+1,
               MemCtrl::mc()->getFloatRO(blank_sino->index(0, 0), loglevel+1),
               blank_sino->size(), (float *)NULL, (float *)NULL, NULL);
     MemCtrl::mc()->put(blank_sino->index(0, 0));
     printStat("Trans", loglevel+1,
               MemCtrl::mc()->getFloatRO(tra_sino->index(0, 0), loglevel+1),
               tra_sino->size(), (float *)NULL, (float *)NULL, NULL);
     MemCtrl::mc()->put(tra_sino->index(0, 0));
     Logging::flog()->logMsg("scale factor for #1-values=#2", loglevel+1)->
      arg(mu)->arg(scaling_factor);
     if (!db_path.empty())
      for (unsigned short int i=0; i < tra_sino->singles().size(); i++)
       tx_singles+=(uint64)tra_sino->singles()[i];
                                                           // reconstruct u-map
     tr->reconstruct(MemCtrl::mc()->getFloat(blank_sino->index(0, 0),
                                             loglevel+1),
                     MemCtrl::mc()->getFloat(tra_sino->index(0, 0),
                                              loglevel+1),
                     iterations, subsets, beta, alpha, beta_ip, iterations_ip,
                     gaussian_smoothing, geman_mcclure_threshold,
                     number_of_segments, mu_mean, mu_std, gauss_intersect,
                     scaling_factor, skip_outer_planes, umap_qc, umap_qc_path,
                     autoscale, autoscalevalue, mnr, tx_singles, db_path,
                     db_frames, &uidx, max_threads);
     MemCtrl::mc()->put(tra_sino->index(0, 0));
     MemCtrl::mc()->put(blank_sino->index(0, 0));
     delete tr;
     tr=NULL;
     umap=MemCtrl::mc()->getRemoveFloat(&uidx, loglevel);
     umap_image->imageData(&umap, true, false, "umap", loglevel);
     umap_image->correctionsApplied(tra_sino->correctionsApplied() |
                                    umap_image->correctionsApplied());
     Logging::flog()->logMsg("finished calculation of #1-map in #2 sec",
                             loglevel)->arg(mu)->arg(sw.stop());
                                                                // smooth u-map
     umap_image->gaussFilter(gauss_fwhm_xy, gauss_fwhm_z, loglevel,
                             max_threads);
                                               // mask u-map with circular disc
     umap_image->maskXY(fov_diameter, loglevel);
     umap_image->cutBelow(0.01f, 0.0f, loglevel);       // cut-off small values
                                  // rebin u-map from umap_z slices to ZSamples
                                  // slices with linear interpolation
     umap_image->resampleZ(ZSamples, umap_z_zoom, loglevel);
     return(umap_image);
   }
   catch (...)
    { sw.stop();
      if (tr != NULL) delete[] tr;
      if (umap != NULL) delete[] umap;
      if (umap_image != NULL) delete umap_image;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate a scatter sinogram.
    \param[in] emi_sino            normalized emission sinogram
    \param[in] acf_sino            acf sinogram
    \param[in] umap_image          u-map
    \param[in] trim                trimming that was used on emission sinogram
    \param[in] iterations          number of iterations for AW-OSEM
                                   reconstruction of emission input image
                                   (AW-OSEM(SSRB(e*n*a)))
    \param[in] subsets             number of subsets for AW-OSEM reconstruction
                                   of emission input image
                                   (AW-OSEM(SSRB(e*n*a)))
    \param[in] boxcar_width        width of boxcar filter for scatter scaling
                                   factors
    \param[in] scatter_skip        number of planes to skip when calculating
                                   the scatter scale factors
    \param[in] scatter_scale_min   minimum scatter scale factor
    \param[in] scatter_scale_max   maximum scatter scale factor
    \param[in] acf_threshold       upper threshold for acf values when
                                   calculating the scatter scale factors
    \param[in] rebin_r             radial rebinning factor that was applied
                                   on emission sinogram
    \param[in] quality_control     store quality control information ?
    \param[in] quality_path        path for quality control files
    \param[in] debug               store debug files ?
    \param[in] debug_path          path for debug files
    \param[in] num                 matrix number
    \param[in] new_scatter_code    use IDL scatter code ?
    \param[in] loglevel            level for logging
    \param[in] max_threads         number of threads to use
    \return scatter sinogram

    Calculate a scaled scatter sinogram from a normalized emission, acf and
    u-map. The calculation is either done by IDL code which creates a 2d
    sinogram with an iterative method or by C++ code which creates a 3d
    sinogram.
 */
/*---------------------------------------------------------------------------*/
SinogramConversion *eau2scatter(SinogramConversion *emi_sino,
                                SinogramConversion *acf_sino,
                                ImageConversion *umap_image,
                                const unsigned short int trim,
                                const unsigned short int iterations,
                                const unsigned short int subsets,
                                const unsigned short int boxcar_width,
                                const unsigned short int scatter_skip,
                                const float scatter_scale_min,
                                const float scatter_scale_max,
                                const float acf_threshold,
                                const unsigned short int acf_threshold_margin,
                                const unsigned short int rebin_r,
                                const bool quality_control,
                                const std::string quality_path,
                                const bool debug,
                                const std::string debug_path,
                                const unsigned short int num,
                                bool new_scatter_code,
#ifndef USE_OLD_SCATTER_CODE
                                 std::vector<float> &scale_factors,
#endif
                                const unsigned short int loglevel,
                                const unsigned short int max_threads)
 { Scatter *scatter=NULL;
   SinogramConversion *scatter_sino=NULL;

   try
   { Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2,
                              "scatter correction (frame #1)")->arg(num);
     Logging::flog()->logMsg("estimate scatter sinogram", loglevel);
     if (new_scatter_code &&
         (GM::gm()->modelNumber() != "1023") &&
         (GM::gm()->modelNumber() != "1024") &&
         (GM::gm()->modelNumber() != "1062") &&
         (GM::gm()->modelNumber() != "1080") &&
         (GM::gm()->modelNumber() != "1090") &&
         (GM::gm()->modelNumber() != "2393") &&
         (GM::gm()->modelNumber() != "2395") &&
         (GM::gm()->modelNumber() != "393") &&
         (GM::gm()->modelNumber() != "395"))
      { Logging::flog()->logMsg("The new scatter code can't be used with the "
                                "gantry model #1. Fall back to old code.",
                                loglevel)->arg(GM::gm()->modelNumber());
        new_scatter_code=false;
      }
#ifdef SUPPORT_NEW_SCATTER_CODE
#if defined(__LINUX__) || defined(WIN32) || defined(__SOLARIS__)
     if (new_scatter_code)                            // use IDL scatter code ?
      { unsigned short int emis_idx;
        std::string scatter_code, scale_code, nr, sav_path, scatter_data_path;
        std::vector <unsigned short int> dim;
        float *emis_buffer, *sumptr;

        nr=toStringZero(num, 2);
        { char *env;

          if ((env=getenv("CAPP_DIR")) != NULL)
           sav_path=std::string(env)+"/IDL_BIN/";
           else sav_path=std::string();
          if ((env=getenv("CAPP_DIR")) != NULL)
           scatter_data_path=std::string(env)+"/";
           else scatter_data_path=std::string();
        }
        scatter_code=sav_path+"cars_scatter.sav";
                             // create continous dataset with emission sinogram
        emis_buffer=MemCtrl::mc()->createFloat(emi_sino->size(), &emis_idx,
                                               "emis", loglevel);
        memset(emis_buffer, 0, emi_sino->size()*sizeof(float));
        sumptr=emis_buffer;
        for (unsigned short int axis=0; axis < emi_sino->axes(); axis++)
         { for (unsigned short int s=0; s < emi_sino->TOFbins(); s++)
            { float *ptr;

              ptr=MemCtrl::mc()->getFloatRO(emi_sino->index(axis, s),
                                            loglevel);
              vecAdd(sumptr, ptr, sumptr, emi_sino->size(axis));
              MemCtrl::mc()->put(emi_sino->index(axis, s));
            }
           sumptr+=emi_sino->size(axis);
         }
        dim.resize(3);
                                                 // pass parameters to IDL code
        dim[0]=emi_sino->RhoSamples();
        dim[1]=emi_sino->ThetaSamples();
        dim[2]=emi_sino->axesSlices();
        IDL_Interface::idl()->inputArray("emisdata", dim, emis_buffer);

        dim[0]=acf_sino->RhoSamples();
        dim[1]=acf_sino->ThetaSamples();
        dim[2]=acf_sino->axisSlices()[0];
        IDL_Interface::idl()->inputArray("acfdata", dim,
                   MemCtrl::mc()->getFloatRO(acf_sino->index(0, 0), loglevel));

        IDL_Interface::idl()->setValue("nbins", emi_sino->RhoSamples());
        IDL_Interface::idl()->setValue("nangles", emi_sino->ThetaSamples());
        IDL_Interface::idl()->setValue("binsize", emi_sino->BinSizeRho());
        IDL_Interface::idl()->setValue("planesep",
                                       GM::gm()->planeSeparation());
        IDL_Interface::idl()->setValue("rings",
                                       (emi_sino->axisSlices()[0]+1)/2);
        IDL_Interface::idl()->setValue("mrd", emi_sino->mrd());
        IDL_Interface::idl()->setValue("span", emi_sino->span());
        IDL_Interface::idl()->setValue("ringradius", GM::gm()->ringRadius());
        IDL_Interface::idl()->setValue("doi", GM::gm()->DOI());
        IDL_Interface::idl()->setValue("lld", emi_sino->LLD());
        IDL_Interface::idl()->setValue("uld", emi_sino->ULD());
        if ((GM::gm()->modelNumber() == "395") ||
            (GM::gm()->modelNumber() == "2393") ||
            (GM::gm()->modelNumber() == "2395"))
         IDL_Interface::idl()->setValue("gantry_type", 393);
         else IDL_Interface::idl()->setValue("gantry_type",
                                        atoi(GM::gm()->modelNumber().c_str()));
        IDL_Interface::idl()->setValue("qc_filename",
                     emi_sino->stripExtension(stripPath(emi_sino->fileName()))+
                     "_f"+toStringZero(num, 2));
        IDL_Interface::idl()->setValue("scatter_data_path", scatter_data_path);

        IDL_Interface::idl()->IDL_execute("restore,\""+scatter_code+"\"");
                          // call IDL function to calculate 2d scatter sinogram
        IDL_Interface::idl()->IDL_execute("err=simulate_scatter(emisdata,"
                                           "acfdata,nbins,nangles,binsize,"
                                           "planesep,rings,mrd,span,"
                                           "ringradius,doi,lld,uld,"
                                           "gantry_type,qc_filename,"
                                           "scatter_data_path,mask,nleft,"
                                           "nright,scattersino)");
        MemCtrl::mc()->put(emis_idx);
        MemCtrl::mc()->deleteBlock(&emis_idx);
        MemCtrl::mc()->put(acf_sino->index(0, 0));
                                                     // get error code from IDL
        if (IDL_Interface::idl()->valueUSInt("err") != 0)
         { Logging::flog()->logMsg("*** error during scatter simulation -> "
                                   "switch off scatter correction", loglevel);
           return(NULL);
           //if (IDL_Interface::idl()->valueUSInt("err") == 42) return(NULL);
           // else throw Exception(REC_IDL_SCATTER,
           //                      "Error during scatter simulation.");
         }
                                                        // fill sinogram object
        scatter_sino=new SinogramConversion(emi_sino->RhoSamples(),
                                        emi_sino->BinSizeRho(),
                                        emi_sino->ThetaSamples(),
                                        emi_sino->span(),
                                        emi_sino->mrd(),
                                        emi_sino->planeSeparation(),
                                        (emi_sino->axisSlices()[0]+1)/2,
                                        emi_sino->intrinsicTilt(), false,
                                        emi_sino->LLD(), emi_sino->ULD(),
                                        emi_sino->startTime(),
                                        emi_sino->frameDuration(),
                                        emi_sino->gateDuration(),
                                        emi_sino->halflife(),
                                        emi_sino->bedPos(), num, true);

        if (emi_sino->TOFbins() > 1)        // scale scatter sinogram for TOF ?
         {
           /*
           unsigned short int nleft, nright, mask_idx, scat_idx;
           float *mask, *scatter2d;

           nleft=IDL_Interface::idl()->valueUSInt("nleft");
           nright=IDL_Interface::idl()->valueUSInt("nright");
           size=emi_sino->planeSize()*
                (unsigned long int)emi_sino->axisSlices()[0];
           mask=MemCtrl::mc()->createFloat(size, &mask_idx, "mask", loglevel);
           memcpy(mask, IDL_Interface::idl()->valueFArr("mask"),
                  size*sizeof(float));
           IDL_Interface::idl()->setValue("mask", 0);

           scatter2d=MemCtrl::mc()->createFloat(size, &scat_idx, "scat",
                                                loglevel);
           memcpy(scatter2d, IDL_Interface::idl()->valueFArr("scattersino"),
                  size*sizeof(float));
           IDL_Interface::idl()->setValue("scattersino", 0);

           for (unsigned short int s=0; s < emi_sino->TOFbins(); s++)
            { scatter_sino->copyData(scatter2d, s, scatter_sino->axes(),
                                     scatter_sino->axisSlices()[0],
                                     emi_sino->TOFbins(), false, false, "scat",
                                     loglevel);

            }
           MemCtrl::mc()->put(mask_idx);
           MemCtrl::mc()->deleteBlock(&mask_idx);
           MemCtrl::mc()->put(scat_idx);
           MemCtrl::mc()->deleteBlock(&scat_idx);
           */

           float *fdata;
           unsigned short int fdata_idx;
           { char *env;

             if ((env=getenv("CAPP_DIR")) != NULL)
              scale_code=std::string(env)+"/IDL_BIN/";
              else scale_code=std::string();
           }
           scale_code+="cars_scale.sav";
                                                 // pass parameters to IDL code
           IDL_Interface::idl()->setValue("nbins_rebinned",
                                          emi_sino->RhoSamples());
           IDL_Interface::idl()->setValue("nangles_rebinned",
                                          emi_sino->ThetaSamples());
           IDL_Interface::idl()->IDL_execute("z_elements=intarr("+
                                             toString(emi_sino->axes())+")");
           for (unsigned short int axis=0; axis < emi_sino->axes(); axis++)
            IDL_Interface::idl()->IDL_execute("z_elements["+toString(axis)+
                                  "]="+toString(emi_sino->axisSlices()[axis]));
           IDL_Interface::idl()->setValue("tof_bins", emi_sino->TOFbins());

           IDL_Interface::idl()->IDL_execute("restore,\""+scale_code+"\"");

           fdata=MemCtrl::mc()->createFloat(scatter_sino->size(), &fdata_idx,
                                            "scat", loglevel);
           std::vector <unsigned short int> dim;

           dim.resize(3);
           dim[0]=emi_sino->RhoSamples();
           dim[1]=emi_sino->ThetaSamples();
           dim[2]=emi_sino->axisSlices()[0];
           IDL_Interface::idl()->inputArray("tofscat", dim, fdata);
                                         // scale every TOF segment individualy
           for (unsigned short int s=0; s < emi_sino->TOFbins(); s++)
            { IDL_Interface::idl()->inputArray("emisdata", dim,
                   MemCtrl::mc()->getFloatRO(emi_sino->index(0, s), loglevel));
              IDL_Interface::idl()->setValue("tof_bin",s);
                                                               // call IDL code
              IDL_Interface::idl()->IDL_execute("err=scale_TOF_scatter("
                                    "emisdata,nbins_rebinned,nangles_rebinned,"
                                    "z_elements,scattersino,mask,nleft,nright,"
                                    "tof_bin,tof_bins,tofscat)");
              MemCtrl::mc()->put(emi_sino->index(0, s));
              if (IDL_Interface::idl()->valueUSInt("err") != 0)
               throw Exception(REC_IDL_SCALING,
                               "Error during scatter scaling.");
              scatter_sino->copyData(fdata, s, scatter_sino->axes(),
                                     scatter_sino->axisSlices()[0],
                                     emi_sino->TOFbins(), false, false, "scat",
                                     loglevel);
            }
           //           IDL_Interface::idl()->IDL_run(scale_code);
           MemCtrl::mc()->put(fdata_idx);
           MemCtrl::mc()->deleteBlock(&fdata_idx);
         }
         else scatter_sino->copyData(IDL_Interface::idl()->valueFArr(
                                                                "scattersino"),
                                     0, 1, emi_sino->axisSlices()[0], 1,
                                     false, false, "scat", loglevel);
        IDL_Interface::idl()->setValue("mask", 0);
        IDL_Interface::idl()->setValue("scattersino", 0);
        if (debug)
         scatter_sino->saveFlat(debug_path+"/scatter_estim2d_"+nr, false,
                                loglevel);
        return(scatter_sino);
      }
#endif
#endif //SUPPORT_NEW_SCATTER_CODE
                              // use C++ code top calculate 3d scatter sinogram
     scatter=new Scatter(emi_sino->RhoSamples(), emi_sino->BinSizeRho(),
                         emi_sino->ThetaSamples(), emi_sino->axisSlices(),
                         emi_sino->RhoSamples()/scatter_img_zoom,
                         GM::gm()->planeSeparation(), subsets, scatter_skip,
                         rebin_r, quality_control, quality_path, loglevel+1);
     scatter_sino=scatter->estimateScatter(acf_sino, umap_image, emi_sino,
                                     trim, step_size, scatter_z_step,
                                     sampling_grid_xy, sampling_grid_z,
                                     max_scatter_samps_xy, max_scatter_samps_z,
                                     underestimate, acf_threshold,
                                     acf_threshold_margin,
                                     emi_sino->LLD(), emi_sino->ULD(), true,
                                     num, iterations, boxcar_width,
                                     scatter_scale_min, scatter_scale_max,
                                     debug,
#ifndef USE_OLD_SCATTER_CODE
                                     scale_factors,
#endif
                                     debug_path, max_threads);
     delete scatter;
     scatter=NULL;
     return(scatter_sino);
   }
   catch (...)
    { if (scatter != NULL) delete scatter;
      if (scatter_sino != NULL) delete scatter_sino;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Get a u-map and acf from file.
    \param[in]  v                command line parameter
    \param[out] umap_image       u-map
    \param[out] acf_sino         acf sinogram
    \param[in]  calculate_acf    calculate acf from u-map if acf file doesn't
                                 exist ?
    \param[in]  calculate_umap   calculate u-map from acf if u-map file doesn't
                                 exist ?
    \param[in]  span             span of acf
    \param[in]  mrd              maximum ring difference of acf
    \param[in]  a_mnr            matrix number of acf in file
    \param[in]  u_mnr            matrix number of u-map in file
    \param[in]  num              matrix number
    \param[in]  loglevel         level for logging
    \param[in]  max_threads      number of threads to use

    Get a u-map and acf from file. If one of these doesn't exist, its
    calculated from the other one. To calculate the acf from a u-map the
    algorithm that was specified in the command line is used. If the acf in the
    file is only 2d, a 3d acf will be calculated with the algorithm that was
    specified in the command line. To calculate a u-map from an acf, DIFT is
    used. If the required command line parameters were used, the u-map and/or
    acf will be stored in a file.
 */
/*---------------------------------------------------------------------------*/
void get_umap_acf(Parser::tparam * const v, ImageConversion **umap_image,
                  SinogramConversion **acf_sino, const bool calculate_acf,
                  const bool calculate_umap, unsigned short int span,
                  unsigned short int mrd, const unsigned short int a_mnr,
                  const unsigned short int u_mnr, const unsigned short int num,
                  const unsigned short int loglevel,
                  const unsigned short int max_threads)
 { std::string mu, nr;
   float fov_diameter=0.0f;

#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
   char c[2];

   c[1]=0;
   c[0]=(char)181;
   mu=std::string(c);
#endif
#ifdef WIN32
   mu='u';
#endif
   nr=toStringZero(num, 2);
   if (!v->umap_filename.empty())
    {                                                    // get u-map from file
      Logging::flog()->logMsg("loading the #1-map (matrix #2) from file #3",
                              loglevel)->arg(mu)->arg(u_mnr)->
       arg(v->umap_filename);
      if (GM::gm()->initialized())
       *umap_image=new ImageConversion(v->iwidth,
                                      (float)GM::gm()->RhoSamples()*
                                       GM::gm()->BinSizeRho()/(float)v->iwidth,
                                       GM::gm()->rings()*2-1,
                                       GM::gm()->planeSeparation(), num, 0, 0,
                                       0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
       else *umap_image=new ImageConversion(v->iwidth, 0.0f, 0, 0.0f, num, 0,
                                            0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                                                                  // load u-map
      (*umap_image)->load(v->umap_filename, true, u_mnr, "umap", loglevel+1);
                                          // uncorrect for depth-of-interaction
      (*umap_image)->uncorrectDOI(loglevel+1);
      if (calculate_acf && v->acf_filename.empty())          // calculate acf ?
       { *acf_sino=CONVERT::image2sinogram(*umap_image, v, num, "acf",
                                           loglevel, max_threads);
                                                          // kill blocks in acf
         (*acf_sino)->maskOutBlocks(v->kill_blocks, loglevel);
       }
    }
   if (!v->acf_filename.empty())
    {                                                      // get acf from file
      Logging::flog()->logMsg("loading the acf (matrix #1)", loglevel)->
       arg(a_mnr);
                                              // init geometry (for flat files)
      if (GM::gm()->initialized())
       { unsigned short int rs, ts;
         float bsr;

         if (span == 0)
          { if (v->span == 0) v->span=GM::gm()->span();
            span=v->span;
          }
         if (mrd == 0)
          { if (v->mrd == 0) v->mrd=GM::gm()->mrd();
            mrd=v->mrd;
          }

         rs=GM::gm()->RhoSamples();
         ts=GM::gm()->ThetaSamples();
         bsr=GM::gm()->BinSizeRho();
         if (v->aRhoSamples > 0)
          { rs=v->aRhoSamples;
            bsr*=(float)rs/(float)v->aRhoSamples;
          }
         if (v->aThetaSamples > 0) ts=v->aThetaSamples;
                                                               // init sinogram
         *acf_sino=new SinogramConversion(rs, bsr, ts, span, mrd,
                                          GM::gm()->planeSeparation(),
                                          GM::gm()->rings(),
                                          GM::gm()->intrinsicTilt(), false,
                                          v->si.lld, v->si.uld,
                                          v->si.start_time,
                                          v->si.frame_duration,
                                          v->si.gate_duration,
                                          v->si.halflife, v->si.bedpos, num,
                                          v->adim);
       }
       else { unsigned short int rs=0, ts=0;

              if (v->aRhoSamples > 0) rs=v->aRhoSamples;
              if (v->aThetaSamples > 0) ts=v->aThetaSamples;
                                                               // init sinogram
              *acf_sino=new SinogramConversion(rs, 0.0f, ts, span, mrd,
                                               0.0f, 0, 0.0f, false, v->si.lld,
                                               v->si.uld, v->si.start_time,
                                               v->si.frame_duration,
                                               v->si.gate_duration,
                                               v->si.halflife, v->si.bedpos,
                                               num, v->adim);
           }
                                                                    // load acf
      (*acf_sino)->load(v->acf_filename, true, v->acf_arc_corrected, a_mnr,
                        "acf", loglevel+1);
      if (v->acf_arc_corrected_overwrite)
       if (v->acf_arc_corrected)
        (*acf_sino)->correctionsApplied((*acf_sino)->correctionsApplied() |
                                        CORR_Radial_Arc_correction |
                                        CORR_Axial_Arc_correction);
        else (*acf_sino)->correctionsApplied(
                                            (*acf_sino)->correctionsApplied() ^
                                            (CORR_Axial_Arc_correction |
                                             CORR_Axial_Arc_correction));

                                                          // kill blocks in acf
      (*acf_sino)->maskOutBlocks(v->kill_blocks, loglevel);
      (*acf_sino)->untilt(true, loglevel);                        // untilt acf
                                      // calculate 3d acf from 2d acf if needed
      (*acf_sino)->sino2D3D(v->irebin_method, span, mrd, "acf", loglevel,
                            max_threads);
                                                    // arc correction of 3d acf
      if (!GM::gm()->isPETCT())
       (*acf_sino)->radialArcCorrection(v->no_radial_arc, true, loglevel,
                                        max_threads);
      (*acf_sino)->trim(v->trim, "acf", 1);                    // trim sinogram
      if (calculate_umap)
       if (*umap_image == NULL)                     // calculate u-map from acf
        { Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 1,
                                   "calculate #1-map from acf (frame #2)")->
           arg(mu)->arg(num);
          *umap_image=CONVERT::sinogram2image(*acf_sino, NULL, NULL, NULL,
                                              (*acf_sino)->RhoSamples(),
                                              (*acf_sino)->BinSizeRho(),
                                              ALGO::DIFT_Algo, 0, 0, 0.0f,
                                              false,
                                              GM::gm()->RhoSamples()/
                                              (*acf_sino)->RhoSamples(), num,
                                              &fov_diameter, std::string(),
                                              loglevel, max_threads);
          (*umap_image)->initACQcode((*acf_sino)->matrixFrame(),
                                     (*acf_sino)->matrixPlane(),
                                     (*acf_sino)->matrixGate(),
                                     (*acf_sino)->matrixData(),
                                     (*acf_sino)->matrixBed());
        }
    }
   if (v->debug)
    { if (*acf_sino != NULL)
       if (v->irebin_method == SinogramConversion::iNO_REB)
        (*acf_sino)->saveFlat(v->debug_path+"/acf2d_"+nr, false, loglevel+1);
        else if ((*acf_sino)->axes() > 1)
              (*acf_sino)->saveFlat(v->debug_path+"/acf3d_"+nr, false,
                                    loglevel+1);
      if (*umap_image != NULL)
       (*umap_image)->saveFlat(v->debug_path+"/umap_"+nr, loglevel+1);
    }
   if (!v->oumap_filename.empty())                                // save u-map
    { if (v->iwidth > 0)
       (*umap_image)->resampleXY(v->iwidth,
                                 (*umap_image)->DeltaXY()*
                                 (float)(*umap_image)->XYSamples()/
                                 (float)v->iwidth, true, loglevel);
      Logging::flog()->logMsg("save #1-map matrix #2 in file #3", loglevel)->
       arg(mu)->arg(num)->arg(v->oumap_filename);
      if (num == 0)
       (*umap_image)->copyFileHeader((*acf_sino)->ECAT7mainHeader(),
                                     (*acf_sino)->InterfileHdr());
      (*umap_image)->save(v->oumap_filename, v->acf_filename,
                          (*acf_sino)->RhoSamples(),
                          (*acf_sino)->ThetaSamples(),
                          (*acf_sino)->matrixFrame(),
                          (*acf_sino)->matrixPlane(),
                          (*acf_sino)->matrixGate(),
                          (*acf_sino)->matrixData(), (*acf_sino)->matrixBed(),
                          (*acf_sino)->intrinsicTilt(), v, fov_diameter,
                          ImageConversion::SCATTER_NoScatter,
                          (*acf_sino)->decayFactor(),
                          (*acf_sino)->scatterFraction(), (*acf_sino)->ECF(),
                          std::vector <float>(), v->overlap,
                          (*acf_sino)->feetFirst(), (*acf_sino)->bedMovesIn(),
                          (*acf_sino)->scanType(), 0, loglevel+1);
    }
   if (!v->oacf_filename.empty())                                   // save acf
    {                                    // un-arc correction of 3d attenuation
      if (!v->oacf_arc_corrected && !GM::gm()->isPETCT())
       (*acf_sino)->radialArcCorrection(v->no_radial_arc, false, loglevel,
                                        max_threads);
      if (!v->no_tilt) (*acf_sino)->untilt(false, loglevel);   // tilt sinogram
      if ((*acf_sino)->axes() > 1)
       Logging::flog()->logMsg("save 3d acf matrix #1 in file #2", loglevel)->
        arg(num)->arg(v->oacf_filename);
       else Logging::flog()->logMsg("save 2d acf matrix #1 in file #2",
                                    loglevel)->arg(num)->arg(v->oacf_filename);
      if ((num == 0) && v->acf_filename.empty())
       (*acf_sino)->copyFileHeader((*umap_image)->ECAT7mainHeader(),
                                   (*umap_image)->InterfileHdr());
      (*acf_sino)->save(v->oacf_filename, v->umap_filename,
                        (*umap_image)->matrixFrame(),
                        (*umap_image)->matrixPlane(),
                        (*umap_image)->matrixGate(),
                        (*umap_image)->matrixData(),
                        (*umap_image)->matrixBed(), v->view_mode,
                        (*umap_image)->bedPos(), num, loglevel+1);
      if (!GM::gm()->isPETCT())
       (*acf_sino)->radialArcCorrection(v->no_radial_arc, true, loglevel,
                                        max_threads);
      (*acf_sino)->untilt(true, loglevel);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert an image into a sinogram.
    \param[in]     image         image
    \param[in,out] v             command line parameters
    \param[in]     num           matrix number
    \param[in]     name          prefix for swap files
    \param[in]     loglevel      level for logging
    \param[in]     max_threads   number of threads to use
    \return sinogram

    Convert an image into a sinogram by a 3d forward-projection (FWDPJ) or
    by a 2d forward projection followed by either inverse single-slice
    rebinning (iSSRB) or inverse fourier rebinning (iFORE). This function works
    with u-maps and emission images.
 */
/*---------------------------------------------------------------------------*/
SinogramConversion *image2sinogram(ImageConversion *image,
                                   Parser::tparam * const v,
                                   const unsigned short int num,
                                   const std::string name,
                                   const unsigned short int loglevel,
                                   const unsigned short int max_threads)
 { SinogramConversion *sino=NULL;
   OSEM3D *osem3d=NULL;

   try
   { unsigned short int axes_slices;
     std::vector <unsigned short int> axis_slices;

     image->uncorrectDOI(loglevel);
     if (image->isUMap())
      image->scale(image->DeltaXY()/10.0f, loglevel);          // unscale u-map
                                             // get scan information from image
     v->si.lld=image->LLD();
     v->si.uld=image->ULD();
     v->si.start_time=image->startTime();
     v->si.frame_duration=image->frameDuration();
     v->si.gate_duration=image->gateDuration();
     v->si.halflife=image->halflife();
     v->si.bedpos=image->bedPos();
     if (v->span == 0) v->span=GM::gm()->span();
     if (v->mrd == 0) v->mrd=GM::gm()->mrd();
                                                        // create segment table
     if (v->irebin_method == SinogramConversion::FWDPJ)
      axis_slices.resize((v->mrd-(v->span-1)/2)/v->span+1);
      else axis_slices.resize(1);
     axis_slices[0]=2*GM::gm()->rings()-1;
     axes_slices=axis_slices[0];
     for (unsigned short int axis=1; axis < axis_slices.size(); axis++)
      { axis_slices[axis]=2*(axis_slices[0]-2*(axis*v->span-(v->span-1)/2));
        axes_slices+=axis_slices[axis];
      }
     if (v->tof_bins == 1)
      Logging::flog()->logMsg("calculate sinogram by forward-projection from "
                              "#1x#2x#3 to #4x#5x#6", loglevel)->
       arg(image->XYSamples())->arg(image->XYSamples())->
       arg(image->ZSamples())->arg(GM::gm()->RhoSamples()/v->rebin_r)->
       arg(GM::gm()->ThetaSamples()/v->rebin_t)->arg(axes_slices);
       else Logging::flog()->logMsg("calculate sinogram by forward-projection "
                                    "from #1x#2x#3 to #4x#5x#6x#7", loglevel)->
             arg(image->XYSamples())->arg(image->XYSamples())->
             arg(image->ZSamples())->arg(GM::gm()->RhoSamples()/v->rebin_r)->
             arg(v->tof_bins)->arg(GM::gm()->ThetaSamples()/v->rebin_t)->
             arg(axes_slices);
                                       // calculate 2d or 3d forward projection
     osem3d=new OSEM3D(image->XYSamples(), image->DeltaXY(),
                       GM::gm()->RhoSamples()/v->rebin_r,
                       GM::gm()->BinSizeRho()*(float)v->rebin_r,
                       GM::gm()->ThetaSamples()/v->rebin_t, axis_slices,
                       v->span, v->mrd, image->DeltaZ(), v->rebin_r, 1,
                       ALGO::OSEM_UW_Algo, 0, NULL, loglevel);
     osem3d->initTOF(v->tof_width, v->tof_fwhm, v->tof_bins);
     sino=osem3d->fwd_proj3d(image, max_threads);
     delete osem3d;
     osem3d=NULL;
     sino->shuffleTOFdata(false, loglevel);
     if (image->isUMap())
      { sino->emi2Tra(loglevel);
        image->scale(1.0f/(image->DeltaXY()/10.0f), loglevel);   // scale u-map
      }
      if (v->irebin_method != SinogramConversion::FWDPJ)  // use iSSRB or iFORe
      { if (v->debug)                                        // save debug file
         { std::string nr;

           nr=toStringZero(num, 2);
           if (image->isUMap())
            sino->saveFlat(v->debug_path+"/acf2d_"+nr, false, loglevel+1);
            else sino->saveFlat(v->debug_path+"/sino2d_"+nr, false,
                                loglevel+1);
         }
        sino->sino2D3D(v->irebin_method, sino->span(), sino->mrd(), name,
                       loglevel, max_threads);
      }
     sino->correctionsApplied(image->correctionsApplied());
     return(sino);
   }
   catch (...)
    { if (sino != NULL) delete sino;
      if (osem3d != NULL) delete osem3d;
      throw;
    }
  }

/*---------------------------------------------------------------------------*/
/*! \brief Convert a sinogram into an image.
    \param[in,out] sino           (corrected) emission sinogram or
                                  transmission sinogram
    \param[in,out] acf_sino       acf sinogram
    \param[in,out] norm_sino      norm sinogram
    \param[in,out] scatter_sino   scatter sinogram
    \param[in]     width          width/height of image
    \param[in]     DeltaXY        width/height of image voxel in mm
    \param[in]     algorithm      reconstruction algorithm
    \param[in]     iterations     number of iterations for OSEM
    \param[in]     subsets        number of subsets for OSEM
    \param[in]     gibbs_prior    Gibbs prior for Gibbs-OSEM
    \param[in]     undo_attcor    undo attenuation correction ?
    \param[in]     reb_factor     radial rebinning factor that was applied on
                                  sinogram
    \param[in]     num            matrix number
    \param[in]     fov_diameter   diameter of reconstructed field-of-view in mm
    \param[in]     debugname      prefix for debug filename
    \param[in]     loglevel       level for logging
    \param[in]     max_threads    number of threads to use

    Convert a sinogram into an image by reconstruction with FBP, DIFT, UW-OSEM,
    AW-OSEM, NW-OSEM, ANW-OSEM or OP-OSEM. The acf, norm and scatter sinograms
    will be deleted as soon as possible. The function works with emission and
    transmission data.
 */
/*---------------------------------------------------------------------------*/
ImageConversion *sinogram2image(SinogramConversion *sino,
                                SinogramConversion **acf_sino,
                                SinogramConversion **norm_sino,
                                SinogramConversion **scatter_sino,
                                const unsigned short int width,
                                const float DeltaXY,
                                const ALGO::talgorithm algorithm,
                                const unsigned short int iterations,
                                const unsigned short int subsets,
                                const float gibbs_prior,
                                const bool undo_attcor,
                                const unsigned short int reb_factor,
                                const unsigned short int num,
                                float * const fov_diameter,
                                std::string debugname,
                                const unsigned short int loglevel,
                                const unsigned short int max_threads)
 { ImageConversion *image=NULL;
   float *img=NULL;
   DIFT *dift=NULL;
   FBP *fbp=NULL;
   OSEM3D *osem3d=NULL;
//   PSF_Matrix *psf_matrix=NULL;
   StopWatch sw;

   try
   { bool is_acf;
     std::string::size_type p;
     unsigned short int lut_level=2;

     if (undo_attcor && (acf_sino != NULL) &&    // uncorrect for attenuation ?
         (sino->correctionsApplied() &
          (CORR_Measured_Attenuation_Correction |
           CORR_Calculated_Attenuation_Correction)))
      { Logging::flog()->logMsg("un-correct for attenuation", loglevel);
        sino->divide(*acf_sino, true, CORR_Measured_Attenuation_Correction |
                                      CORR_Calculated_Attenuation_Correction,
                     false, loglevel+1, max_threads);
        if (acf_sino != NULL) { delete *acf_sino;
                                *acf_sino=NULL;
                              }
      }
     Logging::flog()->logMsg("start calculation of image", loglevel);
     is_acf=sino->isACF();
     sw.start();
     if (debugname != std::string())
      { if ((p=debugname.rfind(".")) != std::string::npos)
         debugname.erase(p);
        debugname+="_#1";
      }
                                     // look-up table in OSEM is only useful if
                                     // more than 1 iterations is calculated
     if (iterations < 2) lut_level=1;
                    // look-up table is only useful if sinogram is not too big,
                    // since otherwise swapping would be required
     if (sino->axes() > 7) 
      if (sino->RhoSamples() > 160) lut_level=0;
       else lut_level=2;
      else if ((sino->axes() > 1) && (sino->TOFbins() > 1)) lut_level=1;
     switch (algorithm)
      { case ALGO::FBP_Algo:                             // FBP reconstruction
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         fbp=new FBP(sino, width, DeltaXY, reb_factor, loglevel+1);
         image=fbp->reconstruct(sino, false, num, fov_diameter, max_threads);
         delete fbp;
         fbp=NULL;
         break;
        case ALGO::DIFT_Algo:                            // DIFT reconstruction
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         sino->tra2Emi(loglevel+1);
         image=new ImageConversion(sino->RhoSamples(), sino->BinSizeRho(),
                                   sino->axisSlices()[0],
                                   GM::gm()->planeSeparation(), num,
                                   sino->LLD(), sino->ULD(), sino->startTime(),
                                   sino->frameDuration(), sino->gateDuration(),
                                   sino->halflife(), sino->bedPos());
         dift=new DIFT(sino->RhoSamples(), sino->ThetaSamples(),
                       sino->BinSizeRho(), loglevel+1);
         img=dift->reconstruct(MemCtrl::mc()->getFloat(sino->index(0, 0),
                                                       loglevel+1),
                               sino->axisSlices()[0], num, fov_diameter,
                               max_threads);
         MemCtrl::mc()->put(sino->index(0, 0));
         delete dift;
         dift=NULL;
         image->imageData(&img, is_acf, false, "img", loglevel+1);
         image->resampleXY(width, DeltaXY, is_acf, loglevel+1);
         if (is_acf) sino->emi2Tra(loglevel+1);
         break;
        case ALGO::MLEM_Algo:                            // MLEM reconstruction
        case ALGO::OSEM_UW_Algo:                      // UW-OSEM reconstruction
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         osem3d=new OSEM3D(sino, width, DeltaXY, reb_factor, subsets,
                           algorithm, lut_level, NULL, loglevel+1);
         osem3d->initTOF(sino->TOFwidth(), sino->TOFfwhm(), sino->TOFbins());
         osem3d->calcNormFactorsUW(max_threads);
         if (gibbs_prior == 0.0f)
          image=osem3d->reconstruct(sino, false, iterations, num, fov_diameter,
                                    debugname, false, max_threads);
          else image=osem3d->reconstructGibbs(sino, false, iterations,
                                              gibbs_prior, num, fov_diameter,
                                              debugname, max_threads);
         delete osem3d;
         osem3d=NULL;
         break;
        case ALGO::OSEM_AW_Algo:                      // AW-OSEM reconstruction
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         if ((*acf_sino)->axes() == 1)
          (*acf_sino)->resampleRT(sino->RhoSamples(), sino->ThetaSamples(),
                                  loglevel+1, max_threads);
         osem3d=new OSEM3D(sino, width, DeltaXY, reb_factor, subsets,
                           algorithm, lut_level, NULL, loglevel+1);
         osem3d->initTOF(sino->TOFwidth(), sino->TOFfwhm(), sino->TOFbins());
         osem3d->calcNormFactorsAW(*acf_sino, max_threads);
         osem3d->uncorrectEmissionAW(sino, *acf_sino, true, max_threads);
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         if (gibbs_prior == 0.0f)
          image=osem3d->reconstruct(sino, false, iterations, num, fov_diameter,
                                    debugname, false, max_threads);
          else image=osem3d->reconstructGibbs(sino, false, iterations,
                                              gibbs_prior, num, fov_diameter,
                                              debugname, max_threads);
         image->correctionsApplied(image->correctionsApplied() |
                                   CORR_Measured_Attenuation_Correction);
         delete osem3d;
         osem3d=NULL;
         break;
        case ALGO::OSEM_NW_Algo:                      // NW-OSEM reconstruction
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         osem3d=new OSEM3D(sino, width, DeltaXY, reb_factor, subsets,
                           algorithm, lut_level, NULL, loglevel+1);
         osem3d->initTOF(sino->TOFwidth(), sino->TOFfwhm(), sino->TOFbins());
         osem3d->calcNormFactorsNW(*norm_sino, max_threads);
         osem3d->uncorrectEmissionNW(sino, *norm_sino, true, max_threads);
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         if (gibbs_prior == 0.0f)
          image=osem3d->reconstruct(sino, false, iterations, num, fov_diameter,
                                    debugname, false, max_threads);
          else image=osem3d->reconstructGibbs(sino, false, iterations,
                                              gibbs_prior, num, fov_diameter,
                                              debugname, max_threads);
         image->correctionsApplied(image->correctionsApplied() |
                                   CORR_Normalized);
         delete osem3d;
         osem3d=NULL;
         break;
        case ALGO::OSEM_ANW_Algo:                    // ANW-OSEM reconstruction
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         osem3d=new OSEM3D(sino, width, DeltaXY, reb_factor, subsets,
                           algorithm, lut_level, NULL, loglevel+1);
         osem3d->initTOF(sino->TOFwidth(), sino->TOFfwhm(), sino->TOFbins());
         osem3d->calcNormFactorsANW(*acf_sino, *norm_sino, max_threads);
         osem3d->uncorrectEmissionANW(sino, *acf_sino, true, *norm_sino,
                                      true, max_threads);
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         if (gibbs_prior == 0.0f)
          image=osem3d->reconstruct(sino, false, iterations, num, fov_diameter,
                                    debugname, false, max_threads);
          else image=osem3d->reconstructGibbs(sino, false, iterations,
                                              gibbs_prior, num, fov_diameter,
                                              debugname, max_threads);
         image->correctionsApplied(image->correctionsApplied() |
                                   CORR_Measured_Attenuation_Correction |
                                   CORR_Normalized);
         delete osem3d;
         osem3d=NULL;
         break;
        case ALGO::OSEM_OP_Algo:                      // OP-OSEM reconstruction
         osem3d=new OSEM3D(sino, width, DeltaXY, reb_factor, subsets,
                           algorithm, lut_level, NULL, loglevel+1);
         osem3d->initTOF(sino->TOFwidth(), sino->TOFfwhm(), sino->TOFbins());
         osem3d->calcNormFactorsANW(*acf_sino, *norm_sino, max_threads);
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         osem3d->calcOffsetOP(sino, *scatter_sino, *acf_sino, max_threads);
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         image=osem3d->reconstruct(sino, false, iterations, num, fov_diameter,
                                   debugname, true, max_threads);
         image->correctionsApplied(image->correctionsApplied() |
                                   CORR_Measured_Attenuation_Correction |
                                   CORR_Normalized |
                                   CORR_3D_scatter_correction);
         delete osem3d;
         osem3d=NULL;
         break;
        case ALGO::OSEM_PSF_AW_Algo:                        // AW-OSEM with PSF
         if (norm_sino != NULL) { delete *norm_sino;
                                  *norm_sino=NULL;
                                }
         if (scatter_sino != NULL) { delete *scatter_sino;
                                     *scatter_sino=NULL;
                                   }
         (*acf_sino)->resampleRT(sino->RhoSamples(), sino->ThetaSamples(),
                                 loglevel+1, max_threads);
         (*acf_sino)->correctionsApplied((*acf_sino)->correctionsApplied() |
                                         CORR_Radial_Arc_correction |
                                         CORR_Axial_Arc_correction);
         (*acf_sino)->radialArcCorrection(false, false, loglevel+1,
                                          max_threads);
         (*acf_sino)->axialArcCorrection(false, false, loglevel+1,
                                         max_threads);
         sino->radialArcCorrection(false, false, loglevel+1, max_threads);
         sino->axialArcCorrection(false, false, loglevel+1, max_threads);

         // for 4mm pixel size:
         //  (*acf_sino)->saveFlat("psf_acf", true, loglevel+1);
         sino->cutBelow(0.0f, 0.0f, loglevel+1);
         // sino->saveFlat("psf_em", true, loglevel+1);

#if 0
         psf_matrix=new PSF_Matrix(2.0f, sino->axisSlices(),
                                   sino->axesSlices(), max_threads,
                                   loglevel+1);
         {
           PSF_Recon_rebinned Recon(GM::gm()->RhoSamples(),
                                    GM::gm()->rings()*2-1,
                                    GM::gm()->RhoSamples(),
                                    GM::gm()->ThetaSamples(),
                                    GM::gm()->BinSizeRho(), GM::gm()->mrd(),
                                    GM::gm()->span(), 2, 2, 2);
           
           Recon.Matrix_Rho_Compute();
           // Recon.Matrix_Dzeta_Compute(max_threads);
           // Recon.read_sino("psf_em.s");
           Recon.read_mask("/home/frank/prog/3dpet/acsiii/mask_rebin.s");
           image=Recon.OSEM(iterations, subsets, sino, *acf_sino, NULL,
                            false);
         }

         delete psf_matrix;
         psf_matrix=NULL;
#endif
         if (acf_sino != NULL) { delete *acf_sino;
                                 *acf_sino=NULL;
                               }
         image->correctionsApplied(image->correctionsApplied() |
                                   CORR_Measured_Attenuation_Correction);
         break;
        case ALGO::MAPTR_Algo:                                  // can't happen
         break;
      }
     if (is_acf)                                         // scale u-map to 1/cm
      image->scale(1.0f/(image->DeltaXY()/10.0f), loglevel+1);
     image->correctionsApplied(sino->correctionsApplied() |
                               image->correctionsApplied());
     Logging::flog()->logMsg("finished calculation of image in #1 sec",
                             loglevel)->arg(sw.stop());
     return(image);
   }
   catch (...)
    { sw.stop();
      if (image != NULL) delete image;
      if (img != NULL) delete[] img;
      if (dift != NULL) delete dift;
      if (fbp != NULL) delete fbp;
      if (osem3d != NULL) delete osem3d;
#if 0
      if (psf_matrix != NULL) delete psf_matrix;
#endif
      throw;
    }
 }
}
