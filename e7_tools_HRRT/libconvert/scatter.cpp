/*! \class Scatter scatter.h "scatter.h"
    \brief This class implements the calculation of a scatter sinogram based on
           an emission sinogram, normalization dataset and u-map.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
    \date 2005/03/29 delete OSEM object after use to reduce swapping

    This class implements the calculation of a scatter sinogram based on an
    emission sinogram, normalization dataset and u-map. The 3d emission
    sinogram is corrected for attenuation and normalized. Then it is converted
    into a 2d sinogram by single slice rebinning. This sinogram is OSEM
    reconstructed and the resulting image is smoothed by a gaussian filter.
    The u-map is rebinned to the same size as this emission image and both
    are used to simulate a scatter sinogram in smaller size. The small scatter
    sinogram is rebinned by cubic b-splines to the original size. Scaling
    factors for the slices of the sinogram are calculated and applied to
    adapt the scatter sinogram to the countrate of the emission sinogram.
 */

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <sys/stat.h>
#include "scatter.h"
#include "e7_tools_const.h"
#include "e7_common.h"
#include "exception.h"
#include "fastmath.h"
#include "global_tmpl.h"
#include "gm.h"
#include "image_filter.h"
#include "logging.h"
#include "hrrt_common/math_matrix.hpp"
#include "mem_ctrl.h"
#include "osem3d.h"
#include "rebin_sinogram.h"
#include "scatter_sim.h"
#include "ssr.h"
#include "stopwatch.h"
#include "str_tmpl.h"
#include "types.h"
#include "vecmath.h"

/*- constants ---------------------------------------------------------------*/

#ifdef _SCATTER_TMPL_CPP
                                        /*! threshold for gap filling (HRRT) */
const float Scatter::gap_fill_threshold=0.5f;
#endif
#ifndef _SCATTER_TMPL_CPP
             /*! number of detector sample points around ring; multiple of 4 */
const unsigned short int Scatter::num_angular_samps=68;

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _RhoSamples          number of bins in projections
    \param[in] _BinSizeRho          bin size in mm
    \param[in] _ThetaSamples        number of angles in sinograms
    \param[in] _axis_slices         number of planes for axes of 3d sinogram
    \param[in] _XYSamplesScatter    width/height of images used for scatter
                                    simulation
    \param[in] _DeltaZ              plane separation in mm
    \param[in] _subsets             number of subsets for OSEM reconstruction
    \param[in] _skip_outer_planes   number of planes to skip
    \param[in] _rebin_r             radial rebinning factor
    \param[in] _scatter_qc          store quality control data of scatter
                                    correction ?
    \param[in] _scatter_qc_path     path for quality control data of scatter
                                    correction
    \param[in] _loglevel            top level for logging

    Initialize object.

    \todo Don't throw exceptions in Constructor.
 */
/*---------------------------------------------------------------------------*/
Scatter::Scatter(const unsigned short int _RhoSamples, const float _BinSizeRho,
                 const unsigned short int _ThetaSamples,
                 const std::vector <unsigned short int> _axis_slices,
                 const unsigned short int _XYSamplesScatter,
                 const float _DeltaZ, const unsigned short int _subsets,
                 const unsigned short int _skip_outer_planes,
                 const unsigned short int _rebin_r, const bool _scatter_qc,
                 const std::string _scatter_qc_path,
                 const unsigned short int _loglevel)
 { //osem=NULL;
   try
   { //std::vector <unsigned short int> as;

     RhoSamples=_RhoSamples;
     BinSizeRho=_BinSizeRho;
     ThetaSamples=_ThetaSamples;
     axis_slices=_axis_slices;
     DeltaZ=_DeltaZ;
     rebin_r=_rebin_r;
     loglevel=_loglevel;
     RhoSamplesScatterEstim=num_angular_samps/4;
     ThetaSamplesScatterEstim=num_angular_samps/2;
     skip_outer_planes=_skip_outer_planes;
                                  // size for reconstructed transmission image
                                  // and reconstructed ssr emission image
     XYSamplesScatter=_XYSamplesScatter;
     scatter_qc=_scatter_qc;
     scatter_qc_path=_scatter_qc_path;
     subsets=_subsets;
     if ((ThetaSamples % subsets) > 0)
      { unsigned short int os;

        os=subsets;
        while ((ThetaSamples % subsets) > 0) subsets--;
        Logging::flog()->logMsg("*** number of subsets decreased to #1 (#2 "
                                "modulo #3 > 0)", loglevel)->arg(subsets)->
         arg(ThetaSamples)->arg(os);
      }
                                              // initialize OSEM reconstruction
#if 0
     as.resize(1);
     as[0]=axis_slices[0];
     osem=new OSEM3D(XYSamplesScatter,
                     BinSizeRho*(float)RhoSamples/(float)XYSamplesScatter,
                     RhoSamples, BinSizeRho, ThetaSamples, as, 2, 0, DeltaZ,
                     rebin_r, subsets, ALGO::OSEM_AW_Algo, 1, NULL,
                     loglevel+1);
#endif
   }
   catch (...)
    { /*if (osem != NULL) { delete osem;
                          osem=NULL;
                        }*/
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
Scatter::~Scatter()
 { //if (osem != NULL) delete osem;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate cubic b-splines.
    \param[in] nelements   number of elements for each spline
    \param[in] nsamples    number of splines
    \param[in] interval    interval between splines
    \param[in] offset      offset for splines
    \param[in] pad         number of samples extrapolated on each end
    \return matrix with splines
    \exception REC_BSPLINE_ERROR inconsistent input to bspline function

    Calculate cubic b-splines.
 */
/*---------------------------------------------------------------------------*/
Matrix <float> *Scatter::bspline(const unsigned short int nelements,
                                 const unsigned short int nsamples,
                                 const unsigned short int interval,
                                 const unsigned short int offset,
                                 const unsigned short int pad) const
 { Matrix <float> *a=NULL, *tmp=NULL, *c=NULL;

   try
   { if ((nsamples-1)*interval+1+offset > nelements)
      throw Exception(REC_BSPLINE_ERROR,
                      "Inconsistent input to bspline function.");
     unsigned short int nsamps, i;
     std::vector <float> basis;

     nsamps=nsamples+2*pad;
                                                       // generate basis spline
     basis.resize(4*interval+1);
     for (i=0; i < interval; i++)
      { float x, b0, b1;

        x=(float)i/(float)interval;
        b0=0.25f*x*x*x;
        b1=0.25f+0.75f*(x+x*x-x*x*x);
        basis[i]=b0;
        basis[i+interval]=b1;
        basis[interval*4-i]=b0;
        basis[interval*3-i]=b1;
      }
     basis[2*interval]=1.0f;
     { unsigned short int int2;
                                              // put basis spline in tmp matrix
       int2=2*interval;
       tmp=new Matrix <float>(nelements, nsamps);
       for (signed short int n=0; n < nsamps; n++)
        { signed short int left, right, point;

          point=offset+(n-pad)*interval;
          left=std::min((signed short int)int2, point);
          if (left > -int2)
           { right=std::min((signed short int)int2,
                            (signed short int)(nelements-point-1));
             if (right > -int2)
              for (signed short int j=-left; j <= right; j++)
               (*tmp)(point+j, n)=basis[int2+j];
           }
        }
     }
                                     // calculate inverse of coefficient matrix
     a=new Matrix <float>(nsamps, nsamps);
     a->identity();
     for (i=0; i < a->rows(); i++)
      if (i < nsamps-1) { (*a)(i, i+1)=0.25f;
                          (*a)(i+1, i)=0.25f;
                        }
     a->invert();
           // To avoid ringing at the edges, linearly extrapolate pad number of
           // samples on each end. This is implemented as a matrix multiply and
           // folded into the bspline matrix.
     if (pad > 0)
      { c=new Matrix <float>(nsamples, nsamps);
        for (i=0; i < nsamples; i++)
         (*c)(i, i+pad)=1.0f;
        for (i=0; i < pad; i++)
         { (*c)(0, i)=1.0f-i+pad;
           (*c)(1, i)=i-pad;
           (*c)(nsamples-2, nsamps-pad+i)=-1.0f-i;
           (*c)(nsamples-1, nsamps-pad+i)=2.0f+i;
         }
        *a*=*c;
        delete c;
        c=NULL;
        a->transpose();
      }
     *a*=*tmp;
     delete tmp;
     tmp=NULL;
     return(a);
   }
   catch (...)
    { if (tmp != NULL) delete tmp;
      if (a != NULL) delete a;
      if (c != NULL) delete c;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform scatter estimation.
    \param[in,out] acf_sino                 3d attenuation sinogram
    \param[in,out] umap_image               (OSEM) reconstruction of 2d
                                            attenuation
    \param[in]     emi_sino                 pointer to normalized 3d emission
                                            sinogram
    \param[in]     trim                     number of bins trimmed in sinogram
    \param[in]     step_size                sample spacing along rays in mm
    \param[in]     scatter_z_step           axial interval in mm at which
                                            scatter sinograms are simulated
    \param[in]     sampling_grid_xy         width/height of voxels in sampling
                                            grid in mm
    \param[in]     sampling_grid_z          depth of voxels in sampling grid in
                                            mm
    \param[in]     max_scatter_samps_xy     max number of scatter samples in
                                            x/y-direction
    \param[in]     max_scatter_samps_z      max number of scatter samples in
                                            z-direction
    \param[in]     underestimate            correction factor for scatter
                                            scaling factors
    \param[in]     acf_threshold            acf threshold for scatter scaling
    \param[in]     lld                      lower level energy discriminator
    \param[in]     uld                      upper level energy discriminator
    \param[in]     uniform_plane_sampling   produce sinogram with uniform sampling on
                                            plane ?
                                            (otherwise: uniform sampling on ring)
    \param[in]     mnr                      matrix number
    \param[in]     iterations               number of OSEM iterations
    \param[in]     filter_width             width of boxcar filter in pixel
    \param[in]     scatter_scale_min        lower threshold for scatter scale
                                            factors
    \param[in]     scatter_scale_max        uppper threshold for scatter scale
                                            factors
    \param[in]     debug                    debug mode ?
    \param[in]     debug_path               path for debugging information
    \param[in]     max_threads              maximum number of threads to use

    Perform scatter estimation. The normalized emission sinogram is first
    attenuation corrected and single-slice rebinned. This sinogram is then
    OSEM reconstructed and gauss filtered. The u-map is resampled to the same
    size. From the emission image and u-map a 2d scatter sinogram is estimated.
    The scatter sinogram is converted into a 3d sinogram by inverse single-
    slice rebinning. Scale factors are calculated to scale the scatter sinogram
    to the countrate of the normalized emission sinogram. The scatter sinogram
    is finally scaled and data for the scatter QC is stored.
 */
/*---------------------------------------------------------------------------*/
SinogramConversion *Scatter::estimateScatter(SinogramConversion *acf_sino,
                                 ImageConversion *umap_image,
                                 SinogramConversion *emi_sino,
                                 const unsigned short int trim,
                                 const float step_size,
                                 const float scatter_z_step,
                                 const float sampling_grid_xy,
                                 const float sampling_grid_z,
                                 const unsigned short int max_scatter_samps_xy,
                                 const unsigned short int max_scatter_samps_z,
                                 const float underestimate,
                                 const float acf_threshold,
                                 const unsigned short int acf_threshold_margin,
                                 const float lld, const float uld,
                                 const bool uniform_plane_sampling,
                                 const unsigned short int mnr,
                                 const unsigned short int iterations,
                                 unsigned short int filter_width,
                                 const float scatter_scale_min,
                                 const float scatter_scale_max,
                                 const bool debug,
#ifndef USE_OLD_SCATTER_CODE
                                 std::vector<float> &scale_factors,
#endif
                                 const std::string debug_path,
                                 const unsigned short int max_threads)
 { std::ofstream *file=NULL;
   SinogramConversion *ssr_sino=NULL, *scatter_sino=NULL;
   ImageConversion *ssr_image=NULL;
   StopWatch sw;
   OSEM3D *osem=NULL;

   try
   { std::string nr;
     unsigned short int sss_idx, spl;
#ifdef USE_OLD_SCATTER_CODE
     std::vector <float> scale_factors;
#endif
     std::vector <unsigned short int> as;
#ifndef USE_OLD_SCATTER_CODE
     float *scatter_2d=NULL;
     int seg_offset=0, seg_planes=0;
#endif
     umap_image->printStat(loglevel);                // print min and max value
     nr=toStringZero(mnr, 2);
            // ssr of attenuation corrected and normalized 3d emission sinogram
     ssr_sino=SingleSliceRebinnedSinogram(acf_sino, emi_sino, mnr,
                                          max_threads);
     if (debug) ssr_sino->saveFlat(debug_path+"/ssr_"+nr, false, loglevel);
                                                 // reconstruct 2d ssr sinogram
     Logging::flog()->logMsg("start AW-OSEM reconstruction (#1 iterations, #2 "
                             "subsets) of emission from #3x#4x#5 to #6x#7x#8",
                             loglevel)->arg(iterations)->arg(subsets)->
      arg(emi_sino->RhoSamples())->arg(emi_sino->ThetaSamples())->
      arg(emi_sino->axisSlices()[0])->arg(XYSamplesScatter)->
      arg(XYSamplesScatter)->arg(emi_sino->axisSlices()[0]);
     sw.start();
                                              // initialize OSEM reconstruction
     as.resize(1);
     as[0]=axis_slices[0];
     osem=new OSEM3D(XYSamplesScatter,
                     BinSizeRho*(float)RhoSamples/(float)XYSamplesScatter,
                     RhoSamples, BinSizeRho, ThetaSamples, as, 2, 0, DeltaZ,
                     rebin_r, subsets, ALGO::OSEM_AW_Algo, 1, NULL,
                     loglevel+1);
     osem->calcNormFactorsAW(acf_sino, max_threads);
     osem->uncorrectEmissionAW(ssr_sino, acf_sino, false, max_threads);
     ssr_image=osem->reconstruct(ssr_sino, false, iterations, mnr, NULL,
                                 std::string(), false, max_threads);
     delete osem;
     osem=NULL;

     Logging::flog()->logMsg("finished reconstruction of emission in #1 sec",
                            loglevel)->arg(sw.stop());
     if (debug) ssr_image->saveFlat(debug_path+"/ssr_img_"+nr, loglevel);
                                                      // gauss filter ssr image
     ssr_image->gaussFilter(1.0f*emi_sino->BinSizeRho()*
                            (float)emi_sino->RhoSamples()/
                            (float)XYSamplesScatter,
                            1.0f*emi_sino->planeSeparation(), loglevel,
                            max_threads);
     if (debug) ssr_image->saveFlat(debug_path+"/ssr_img_filt_"+nr, loglevel);
                   // scale image to be consistent with scatter simulation code
     { float sf;

       sf=10.0f/ssr_image->DeltaXY();                                   // 1/cm
       Logging::flog()->logMsg("scale image by #1", loglevel)->arg(sf);
       ssr_image->scale(sf, 0);
     }
     delete ssr_sino;
     ssr_sino=NULL;
     Logging::flog()->logMsg("size of emission image: #1x#2x#3 (#4mm x #5mm x "
                             "#6mm)", loglevel)->arg(ssr_image->XYSamples())->
      arg(ssr_image->XYSamples())->arg(ssr_image->ZSamples())->
      arg(ssr_image->DeltaXY())->arg(ssr_image->DeltaXY())->
      arg(ssr_image->DeltaZ());
     Logging::flog()->logMsg("size of u-map: #1x#2x#3 (#4mm x #5mm x "
                             "#6mm)", loglevel)->arg(umap_image->XYSamples())->
      arg(umap_image->XYSamples())->arg(umap_image->ZSamples())->
      arg(umap_image->DeltaXY())->arg(umap_image->DeltaXY())->
      arg(umap_image->DeltaZ());

     if (trim > 0)
      { if (umap_image->DeltaXY()*(float)umap_image->XYSamples()-
            ssr_image->DeltaXY()*(float)ssr_image->XYSamples() ==
            umap_image->DeltaXY()*(float)trim)
         umap_image->resampleXY(umap_image->XYSamples()*2,
                                umap_image->DeltaXY()*0.5f, true, loglevel);
        if (umap_image->DeltaXY()*(float)(umap_image->XYSamples()-2*trim) ==
            ssr_image->DeltaXY()*(float)ssr_image->XYSamples())
         umap_image->trim(trim, "umap", loglevel);
      }
                                       // rebin u-map to size of emission image
     umap_image->resampleXY(XYSamplesScatter,
                            umap_image->DeltaXY()*
                            (float)umap_image->XYSamples()/
                            (float)XYSamplesScatter, true, loglevel);
     Logging::flog()->logMsg("size of emission image: #1x#2x#3 (#4mm x #5mm x "
                             "#6mm)", loglevel)->arg(ssr_image->XYSamples())->
      arg(ssr_image->XYSamples())->arg(ssr_image->ZSamples())->
      arg(ssr_image->DeltaXY())->arg(ssr_image->DeltaXY())->
      arg(ssr_image->DeltaZ());
     Logging::flog()->logMsg("size of u-map: #1x#2x#3 (#4mm x #5mm x "
                             "#6mm)", loglevel)->arg(umap_image->XYSamples())->
      arg(umap_image->XYSamples())->arg(umap_image->ZSamples())->
      arg(umap_image->DeltaXY())->arg(umap_image->DeltaXY())->
      arg(umap_image->DeltaZ());

                                                   // simulate scatter sinogram
     Logging::flog()->logMsg("start scatter estimation", loglevel);
     sw.start();
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
                                        emi_sino->bedPos(), mnr, true);
     sss_idx=ScatterEstimation(MemCtrl::mc()->getFloat(ssr_image->index(),
                                                       loglevel),
                               MemCtrl::mc()->getFloat(umap_image->index(),
                                                       loglevel),
                               scatter_sino->BinSizeRho()*
                               (float)scatter_sino->RhoSamples()/
                               (float)XYSamplesScatter,
                               scatter_sino->planeSeparation(),
                               scatter_sino->RhoSamples(),
                               scatter_sino->BinSizeRho(),
                               scatter_sino->ThetaSamples(),
                               scatter_sino->axesSlices(), step_size,
                               scatter_z_step, sampling_grid_xy,
                               sampling_grid_z, max_scatter_samps_xy,
                               max_scatter_samps_z, lld, uld,
                               uniform_plane_sampling, max_threads);
     MemCtrl::mc()->put(ssr_image->index());

     delete ssr_image;
     ssr_image=NULL;
     MemCtrl::mc()->put(umap_image->index());
     scatter_sino->copyData(MemCtrl::mc()->getFloatRO(sss_idx, loglevel+1), 0,
                            1, scatter_sino->axesSlices(), 1, false, false,
                            "scat", loglevel+1);
     MemCtrl::mc()->put(sss_idx);
     MemCtrl::mc()->deleteBlock(&sss_idx);
     Logging::flog()->logMsg("finished scatter estimation in #1 sec",
                             loglevel)->arg(sw.stop());
                                // convert 2d scatter sinogram into 3d sinogram
     Logging::flog()->logMsg("convert 2d scatter sinogram into 3d sinogram",
                             loglevel);
      
#ifndef USE_OLD_SCATTER_CODE
     scatter_2d = MemCtrl::mc()->getFloatRO(scatter_sino->index(0, 0),loglevel);
      if ((filter_width & 0x1) == 0) filter_width++;
     Logging::flog()->logMsg("calculate axial scatter scale factors (smoothed "
                             "with boxcar #1)", loglevel)->arg(filter_width);
     scale_factors=ScatterScaleFactors(acf_sino, emi_sino, scatter_sino,
                                       underestimate, acf_threshold,
                                       acf_threshold_margin,  mnr, 
                                       filter_width, scatter_scale_min,
                                       scatter_scale_max, max_threads);
    spl=0;
#else
     scatter_sino->sino2D3D(SinogramConversion::iSSRB_REB, emi_sino->span(),
                            emi_sino->mrd(), "scat", loglevel+1, max_threads);
                              // calculate scaling factors for scatter sinogram
     if ((filter_width & 0x1) == 0) filter_width++;
     Logging::flog()->logMsg("calculate axial scatter scale factors (smoothed "
                             "with boxcar #1)", loglevel)->arg(filter_width);
     scale_factors=ScatterScaleFactors(acf_sino, emi_sino, scatter_sino,
                                       underestimate, acf_threshold,
                                       acf_threshold_margin, mnr,
                                       filter_width, scatter_scale_min,
                                       scatter_scale_max, max_threads);

                                                      // scale scatter sinogram
     Logging::flog()->logMsg("calculate scaled scatter sinogram", loglevel);
     spl=0;
     for (unsigned short int axis=0; axis < scatter_sino->axes(); axis++)
      { float *sssp;

        sssp=MemCtrl::mc()->getFloat(scatter_sino->index(axis, 0), loglevel);
        for (unsigned short int p=0; p < scatter_sino->axisSlices()[axis]; p++)
         for (unsigned short int t=0; t < scatter_sino->ThetaSamples(); t++,
              sssp+=scatter_sino->RhoSamples())
          for (unsigned long int r=0; r < scatter_sino->RhoSamples(); r++)
           sssp[r]*=scale_factors[spl+p];
        MemCtrl::mc()->put(scatter_sino->index(axis, 0));
        spl+=scatter_sino->axisSlices()[axis];
      }
     if (debug)
      scatter_sino->saveFlat(debug_path+"/scatter_estim3d_"+nr, false,
                             loglevel);
#endif

     { double tot_sss=0.0, tot_ena=0.0;

       for (unsigned short int axis=0; axis < emi_sino->axes(); axis++)
        { unsigned long int i;
          float *ssp, *emp;
#ifndef USE_OLD_SCATTER_CODE
          unsigned int ii=0;
          unsigned short int p=0;
#endif

          emp=MemCtrl::mc()->getFloatRO(emi_sino->index(axis, 0), loglevel);
          for (i=0; i < emi_sino->size(axis); i++)
           tot_ena+=(double)emp[i];
          MemCtrl::mc()->put(emi_sino->index(axis, 0));
#ifndef USE_OLD_SCATTER_CODE
          if (axis>0) seg_planes = emi_sino->axisSlices()[axis]/2;
          else seg_planes = emi_sino->axisSlices()[0];
          seg_offset = (scatter_sino->axisSlices()[0] - seg_planes)/2;
          ssp = scatter_2d+seg_offset*scatter_sino->planeSize();
          for (p=0; p < seg_planes; p++)
          {
            for (ii=0; ii<scatter_sino->planeSize(); ii++)
              tot_sss += ssp[ii]*scale_factors[spl+p];
            ssp += scatter_sino->planeSize();
          }
          spl += seg_planes;
                // negative segment
          if (axis>0)
          {
            ssp = scatter_2d+seg_offset*scatter_sino->planeSize();
            for (p=0; p < seg_planes; p++)
            {
              for (ii=0; ii<scatter_sino->planeSize(); ii++)
                tot_sss += ssp[ii]*scale_factors[spl+p];
              ssp += scatter_sino->planeSize();
            }
            spl += seg_planes;
          }
#else
          ssp=MemCtrl::mc()->getFloatRO(scatter_sino->index(axis, 0),
                                        loglevel);
          for (i=0; i < scatter_sino->size(axis); i++)
           tot_sss+=(double)ssp[i];
          MemCtrl::mc()->put(scatter_sino->index(axis, 0));
#endif
        }
       Logging::flog()->logMsg("scatter fraction at first pass=#1%",
                               loglevel)->arg(tot_sss/tot_ena*100.0);
       scatter_sino->setScatterFraction((float)(tot_sss/tot_ena*100.0));
     }
                               // store values of ssr for quality control plots
     if (scatter_qc)
     { unsigned short int spl=0;

        for (unsigned short int segment=0;
             segment < emi_sino->axes()*2-1;
             segment++)
         { std::string fname, seg;
           unsigned short int z1, zinc, z2, axis, planes, offs;
           unsigned long int t1, t2, t3;
           float *emp, *sssp;

           axis=(segment+1)/2;
           seg=toStringZero(segment, 2);
           if (axis == 0) planes=emi_sino->axisSlices()[0];
            else planes=emi_sino->axisSlices()[axis]/2;
           if ((segment > 0) && ((segment & 0x1) == 0)) offs=planes;
            else offs=0;
           fname=scatter_qc_path+"/scatter_qc_"+nr+"_"+seg+".dat";
           file=new std::ofstream(fname.c_str());
           z1=(unsigned short int)(0.05f*(float)planes);
           z2=planes-2*z1;
           zinc=(z2-z1)/4;
           t1=(unsigned long int)(emi_sino->ThetaSamples()/4-1)*
              (unsigned long int)emi_sino->RhoSamples();
           t2=(unsigned long int)(emi_sino->ThetaSamples()/2-1)*
              (unsigned long int)emi_sino->RhoSamples();
           t3=(unsigned long int)(emi_sino->ThetaSamples()*3/4-1)*
              (unsigned long int)emi_sino->RhoSamples();
           emp=MemCtrl::mc()->getFloatRO(emi_sino->index(axis, 0), loglevel)+
               (unsigned long int)offs*emi_sino->planeSize();
 
#ifndef USE_OLD_SCATTER_CODE
          seg_offset = (scatter_sino->axisSlices()[0] - planes)/2;
          sssp = scatter_2d+seg_offset*scatter_sino->planeSize();
#else

           sssp=MemCtrl::mc()->getFloatRO(scatter_sino->index(axis, 0),
                                          loglevel)+
                (unsigned long int)offs*scatter_sino->planeSize();
#endif
           for (unsigned short int r=0; r < emi_sino->RhoSamples(); r++)
            { float sum[12];

              for (unsigned short int i=0; i < 5; i++)
               { unsigned short int zb, ze;

                 memset(sum, 0, 12*sizeof(float));
                 zb=z1+i*zinc;
                 ze=std::min(zb+10, planes-1);
                 for (unsigned short int p=zb; p <= ze; p++)
                  { unsigned long int offset;

                    offset=(unsigned long int)p*emi_sino->planeSize()+r;
                    for (unsigned short int t=0; t < 10; t++)
                     { unsigned long int off2;

                       off2=offset+t*emi_sino->RhoSamples();
                       // red line (emission sinogram)
                       sum[0]+=emp[off2];
                       sum[1]+=emp[off2+t1];
                       sum[2]+=emp[off2+t2];
                       sum[3]+=emp[off2+t3];

                       // blue line (scaled scatter sinogram)
#ifndef USE_OLD_SCATTER_CODE
                       sum[4]+=sssp[off2]*scale_factors[spl+p];
                       sum[5]+=sssp[off2+t1]*scale_factors[spl+p];
                       sum[6]+=sssp[off2+t2]*scale_factors[spl+p];
                       sum[7]+=sssp[off2+t3]*scale_factors[spl+p];
#else
                       sum[4]+=sssp[off2];
                       sum[5]+=sssp[off2+t1];
                       sum[6]+=sssp[off2+t2];
                       sum[7]+=sssp[off2+t3];

                       // green line (unscaled scatter sinogram)
                       /*
                       sum[8]+=sssp[off2]/scale_factors[spl+p];
                       sum[9]+=sssp[off2+t1]/scale_factors[spl+p];
                       sum[10]+=sssp[off2+t2]/scale_factors[spl+p];
                       sum[11]+=sssp[off2+t3]/scale_factors[spl+p];
                       */
#endif
                     }
                  }
                 for (unsigned short int j=0; j < 12; j++)
                  *file << sum[j] << " ";
               }
              *file << "\n";
            }
#ifdef USE_OLD_SCATTER_CODE
           MemCtrl::mc()->put(scatter_sino->index(axis, 0));
#endif
           MemCtrl::mc()->put(emi_sino->index(axis, 0));
           file->close();
           delete file;
           file=NULL;
           spl+=planes;
         }
#ifdef USE_OLD_SCATTER_CODE
        GNUPlotScript(scatter_sino->RhoSamples(), scatter_sino->ThetaSamples(),
                      scatter_sino->axisSlices(), mnr);
#else
        GNUPlotScript(emi_sino->RhoSamples(), emi_sino->ThetaSamples(),
                      emi_sino->axisSlices(), mnr);
#endif
      }
#ifndef USE_OLD_SCATTER_CODE
     MemCtrl::mc()->put(scatter_sino->index(0, 0));
#endif

     return(scatter_sino);
   }
   catch (...)
    { sw.stop();
      if (ssr_image != NULL) delete ssr_image;
      if (ssr_sino != NULL) delete ssr_sino;
      if (scatter_sino != NULL) delete scatter_sino;
      if (osem != NULL) delete osem;
      if (debug)
       if (file != NULL) { file->close();
                           delete file;
                         }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create GNUPlot script for quality control.
    \param[in] RhoSamples     number of bins in a projection
    \param[in] ThetaSamples   number of angles in a sinogram plane
    \param[in] axis_slices    number of planes in sinogram axes
    \param[in] mnr            number of matrix

    Create GNUPlot script for quality control.
 */
/*---------------------------------------------------------------------------*/
void Scatter::GNUPlotScript(const unsigned short int RhoSamples,
                            const unsigned short int ThetaSamples,
                            const std::vector <unsigned short int> axis_slices,
                            const unsigned short int mnr) const
 { std::ofstream *file=NULL;

   try
   { std::string nr, fname;
     unsigned short int z1, zinc, z2, segments, pages, segment;

     nr=toStringZero(mnr, 2);
     fname=scatter_qc_path+"/scatter_qc_"+nr+".plt";
     file=new std::ofstream(fname.c_str());
     *file << "set terminal postscript landscape color \"Helvetica\" 8\n"
              "set output \"scatter_qc_"+nr+".ps\"\n";
     segments=axis_slices.size()*2-1;
     pages=segments/16;
     segment=0;
     for (unsigned short int page=0; page <= pages; page++)
      { *file << "set title \"Quality Control for Scatter Correction (Matrix "
              << nr << ")\"\n"
                 "set multiplot\n"
                 "set xlabel \"slice number\"\n"
                 "set yrange [0:]\n\n";
        for (unsigned short int ys=0; ys < 4; ys++)
         for (unsigned short int xs=0; xs < 4; xs++)
          if (segment < segments)
           { unsigned short int planes;
             std::string seg;

             seg=toStringZero(segment, 2);
             if (segment == 0) planes=axis_slices[0];
              else planes=axis_slices[(segment+1)/2]/2;
             *file << "set xrange [0:" << planes << "]\n"
                      "set size 0.25,0.25\n"
                      "set origin " << 0.25f*xs << "," << 0.25f*(3-ys) << "\n"
                      "set title \"segment " << segStr(segment) << "\"\n"
                      "plot \"scatter_scale_" << nr << "_" << seg
                   << ".dat\" using 1 notitle with lines,\\\n"
                      "     \"scatter_scale_" << nr << "_" << seg
                   << ".dat\" using 2 notitle with lines\n\n";
             segment++;
           }
        *file << "set nomultiplot\n"
                 "reset\n\n";
      }

     for (segment=0; segment < segments; segment++)
      { std::string seg;
        unsigned short int axis, planes;

        seg=toStringZero(segment, 2);
        axis=(segment+1)/2;
        if (axis == 0) planes=axis_slices[0];
         else planes=axis_slices[axis]/2;
        *file << "set multiplot\n"
                 "set label \"Quality Control for Scatter Correction (Matrix "
              << mnr << ", Segment " << segStr(segment) << ")\" at screen "
                 "0.5,0.99 center\n"
                 "set ylabel \"\"\n"
                 "set xrange [0:" << RhoSamples-1 << "]\n";
        z1=(unsigned short int)(0.05f*(float)planes);
        z2=planes-2*z1;
        zinc=(z2-z1)/4;
        for (unsigned short int i=0; i <= 4; i++)
         { unsigned short int zb, ze, t1;

           zb=z1+i*zinc;
           ze=std::min(zb+10, planes-1);
           *file << "set origin 0.0," << (4-i)*0.2 << "\n";
           if (i == 0) *file << "set title \"0 degrees\"\n";
            else if (i == 1) *file << "set title\n";
            else if (i == 4) *file << "set xlabel \"bin\"\n";
           t1=0;
           *file << "set ylabel \"Cts(z=" << zb << "-" << ze << ",t=" << t1
                 << "-" << t1+9 << ")\"\n"
                 << "set size 0.25, 0.2\n"
                    "plot \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+1 << " title \"ena\"  with lines,\\\n"
             //     "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
             //  << i*12+9 << " title \"scat\" with lines,\\\n"
                    "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+5 << " title \"sss\" with lines\n";
           t1=ThetaSamples/4-1;
           *file << "set origin 0.25," << (4-i)*0.2 << "\n";
           if (i == 0) *file << "set title \"45 degrees\"\n";
           *file << "set ylabel \"Cts(z=" << zb << "-" << ze << ",t=" << t1
                 << "-" << t1+9 << ")\"\n"
                 << "set size 0.25, 0.2\n"
                    "plot \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+2 << " notitle with lines,\\\n"
                    "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
             //  << i*12+10 << " notitle with lines,\\\n"
             //     "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+6 << " notitle with lines\n";
           t1=ThetaSamples/2-1;
           *file << "set origin 0.5," << (4-i)*0.2 << "\n";
           if (i == 0) *file << "set title \"90 degrees\"\n";
           *file << "set ylabel \"Cts(z=" << zb << "-" << ze << ",t=" << t1
                 << "-" << t1+9 << ")\"\n"
                 << "set size 0.25, 0.2\n"
                    "plot \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+3 << " notitle with lines,\\\n"
                    "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
             //  << i*12+11 << " notitle with lines,\\\n"
             //     "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+7 << " notitle with lines\n";

           t1=ThetaSamples*3/4-1;
           *file << "set origin 0.75," << (4-i)*0.2 << "\n";
           if (i == 0) *file << "set title \"135 degrees\"\n";
           *file << "set ylabel \"Cts(z=" << zb << "-" << ze << ",t=" << t1
                 << "-" << t1+9 << ")\"\n"
                 << "set size 0.25, 0.2\n"
                    "plot \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+4 << " notitle with lines,\\\n"
                    "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
             //  << i*12+12 << " notitle with lines,\\\n"
             //     "     \"scatter_qc_" << nr << "_" << seg << ".dat\" using "
                 << i*12+8 << " notitle with lines\n";
         }
        *file << "set nomultiplot\n"
              << "reset\n\n";
      }
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Use linear interpolation to increase width of matrix.
    \param[in] new_width   width of new matrix
    \param[in] new_samp    sample points of new matrix
    \param[in] old_width   width of original matrix
    \param[in] old_samp    sample points of original matrix
    \param[in] num_rows    number of rows in matrices
    \param[in] mat         original matrix
    \return interpolated matrix

    Use linear interpolation to increase width of matrix.
 */
/*---------------------------------------------------------------------------*/
Matrix <float> *Scatter::interpol(const unsigned short int new_width,
                                  const float * const new_samp,
                                  const unsigned short int old_width,
                                  float * const old_samp,
                                  const unsigned short int num_rows,
                                  const Matrix <float> * const mat) const
 { Matrix <float> *mat_new=NULL;

   try
   { std::vector <signed short int> s;
              // search indices of new_samp values in monotonic old_samp vector
     s.resize(new_width);
     { signed short int max_index;

       max_index=(signed short int)mat->columns()-2;
       for (unsigned short int cn=0; cn < new_width; cn++)
        { s[cn]=-1;
          for (unsigned short int c=0; c < old_width; c++)
           if (new_samp[cn] > old_samp[c]) s[cn]++;
            else break;
                                      // limit s[cn] to interval [0, max_index]
#ifdef __GNUG__
             // The GNU optimizer (at least version 3.2 and 3.2.1) has problems
             // with the min/max construct. Therefore we use this code:
          if (s[cn] < 0) s[cn]=0;
           else if (s[cn] > max_index) s[cn]=max_index;
#else
          s[cn]=std::min(max_index, std::max((signed short int)0, s[cn]));
#endif
                         // new_samp[cn] is somewhere between s[cn] and s[cn]+1
        }
     }
                                                     // create resampled matrix
     mat_new=new Matrix <float>(new_width, num_rows);
     for (unsigned short int r=0; r < num_rows; r++)
      for (unsigned short int c=0; c < new_width; c++)
       { float *ip_val;
                                                // perform linear interpolation
         ip_val=&old_samp[s[c]];
         (*mat_new)(c, r)=(new_samp[c]-*ip_val)*
                          ((*mat)(s[c]+1, r)-(*mat)(s[c], r))/
                           (*(ip_val+1)-*ip_val)+
                          (*mat)(s[c], r);
       }
     return(mat_new);
   }
   catch (...)
    { if (mat_new != NULL) delete mat_new;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Simulate scatter in smaller dataset and use cubic b-splines to
           interpolate into original size.
    \param[in] emission_image           image of single-slice-rebinned and corrected
                                        emission
    \param[in] transmission_image       image of transmission
    \param[in] DeltaXY                  width/height of voxels in images
    \param[in] DeltaZ                   depth of voxels in images
    \param[in] RhoSamples               number of bins in scatter sinogram
    \param[in] BinSizeRho               bin size in scatter sinogram
    \param[in] ThetaSamples             number of angles in scatter sinogram
    \param[in] planes                   number of planes in scatter sinogram
    \param[in] step_size                sample spacing along rays in mm
    \param[in] scatter_z_step           axial interval in mm at which scatter sinograms
                                        are simulated
    \param[in] sampling_grid_xy         width/height of voxels in sampl. grid in mm
    \param[in] sampling_grid_z          depth of voxels in sampling grid in mm
    \param[in] max_scatter_samps_xy     max number of scatter samples in x/y-direction
    \param[in] max_scatter_samps_z      max number of scatter samples in z-direction
    \param[in] lld                      lower level energy discriminator
    \param[in] uld                      upper level energy discriminator
    \param[in] uniform_plane_sampling   produce sinogram with uniform sampling on
                                        plane ? (otherwise: uniform sampling on ring)
    \param[in] max_threads              maximum number of threads to use
    \return memory controller index of scatter sinogram

    Simulate scatter in smaller dataset and use cubic b-splines to interpolate
    the scatter sinogram into original size.
 */
/*---------------------------------------------------------------------------*/
unsigned short int Scatter::ScatterEstimation(float * const emission_image,
                                 float * const transmission_image,
                                 const float DeltaXY, const float DeltaZ,
                                 const unsigned short int RhoSamples,
                                 const float BinSizeRho,
                                 const unsigned short int ThetaSamples,
                                 const unsigned short int planes,
                                 const float step_size,
                                 const float scatter_z_step,
                                 const float sampling_grid_xy,
                                 const float sampling_grid_z,
                                 const unsigned short int max_scatter_samps_xy,
                                 const unsigned short int max_scatter_samps_z,
                                 float lld, const float uld,
                                 const bool uniform_plane_sampling,
                                 const unsigned short int max_threads) const
 { float *scatt_sim=NULL;
   Matrix <float> *splines=NULL, *zsplines=NULL, *ss=NULL, *result=NULL,
                  *rsplines_resamp=NULL, *asplines_resamp=NULL;
   ScatterSim *scsim=NULL;
   StopWatch sw;

   try
   { unsigned short int scat_idx;
     unsigned long int i, scatter_plane_size;
     float *scat;
     std::vector <unsigned short int> zplanes;

     Logging::flog()->logMsg("calculate matrix for beta-spline interpolation",
                             loglevel+1);
     scatter_plane_size=(unsigned long int)RhoSamples*
                        (unsigned long int)ThetaSamples;
                     // calculate interpolation matrix for radial interpolation
     { unsigned short int interp_fact, nbins_interp;
       std::vector <float> new_samp, old_samp;

       interp_fact=(unsigned short int)
                   ((2.0*M_PI)/(float)num_angular_samps/
                    asinf(BinSizeRho/
                          (GM::gm()->ringRadius()+GM::gm()->DOI())))+1;
       if (interp_fact <= 2)
        throw Exception(REC_BINSIZE_TOO_LARGE,
                        "Bin size is too large for interpolation.");
       nbins_interp=(RhoSamplesScatterEstim+2)*interp_fact;
                                                   // calculate cubic b-splines
       splines=bspline(nbins_interp, RhoSamplesScatterEstim+2, interp_fact,
                       (unsigned short int)(nbins_interp/2)-
                       (unsigned short int)((RhoSamplesScatterEstim+2)/2)*
                       interp_fact, 4);
                                        // resample splines to uniform sampling
       new_samp.resize(RhoSamples);
       for (i=0; i < RhoSamples; i++)
        new_samp[i]=(signed long int)i-RhoSamples/2;
       old_samp.resize(nbins_interp);
       for (i=0; i < nbins_interp; i++)
        { double a, b;

          a=(2.0*M_PI)/(double)(num_angular_samps*interp_fact)*
            (double)((signed long int)i-nbins_interp/2);
          b=BinSizeRho/(GM::gm()->ringRadius()+GM::gm()->DOI());
                      // correct to uniform sampling on plane -> arc correction
          if (uniform_plane_sampling) old_samp[i]=sinf(a)/b;
                                     // add samples in uniform sampling on ring
           else old_samp[i]=a/asinf(b);
        }
       rsplines_resamp=interpol(RhoSamples, &new_samp[0], nbins_interp,
                                &old_samp[0], RhoSamplesScatterEstim+2,
                                splines);
       delete splines;
       splines=NULL;
     }
                    // calculate interpolation matrix for angular interpolation
     { unsigned short int angular_interp_fact, nangles_interp;
       std::vector <float> new_samp, old_samp;

       angular_interp_fact=(unsigned short int)
                           ((float)ThetaSamples/
                            (float)ThetaSamplesScatterEstim)+1;
       nangles_interp=angular_interp_fact*ThetaSamplesScatterEstim;
                                                   // calculate cubic b-splines
       splines=bspline(nangles_interp, ThetaSamplesScatterEstim,
                       angular_interp_fact, 0, 4);
                                        // resample splines to uniform sampling
       new_samp.resize(ThetaSamples);
       for (i=0; i < ThetaSamples; i++)
        new_samp[i]=(float)i/(float)ThetaSamples*M_PI;
       old_samp.resize(nangles_interp);
       for (i=0; i < nangles_interp; i++)
        old_samp[i]=(float)i/(float)nangles_interp*M_PI;
       asplines_resamp=interpol(ThetaSamples, &new_samp[0], nangles_interp,
                                &old_samp[0], ThetaSamplesScatterEstim,
                                splines);
       delete splines;
       splines=NULL;
       asplines_resamp->transpose();
     }
                          // calculate interpolation matrix for z-interpolation
     { unsigned short int z_step_int;

       z_step_int=std::max((unsigned short int)round(scatter_z_step/DeltaZ),
                           (unsigned short int)1);
       zplanes.resize((planes-1)/z_step_int+1);
       zplanes[0]=(planes-(1+z_step_int*(zplanes.size()-1)))/2;
       for (i=1; i < zplanes.size(); i++)
        zplanes[i]=zplanes[i-1]+z_step_int;
       zsplines=bspline(planes, zplanes.size(), z_step_int, zplanes[0], 4);
       zsplines->transpose();
     }
     Logging::flog()->logMsg("start scatter simulation", loglevel+1);
     sw.start();
                                                         // apply offset to LLD
                                        // correct the LLD for several scanners
     switch (atoi(GM::gm()->modelNumber().c_str()))
      { case 921:
        case 922:
        case 923:
         lld-=22;
         Logging::flog()->logMsg("lower LLD by 22 keV for gantry model #1",
                                 loglevel+2)->arg(GM::gm()->modelNumber());
         break;
        case 962:
        case 966:
        case 1062:
         lld-=30;
         Logging::flog()->logMsg("lower LLD by 30 keV for gantry model #1",
                                 loglevel+2)->arg(GM::gm()->modelNumber());
         break;
      }
                                                            // simulate scatter
     scsim=new ScatterSim(transmission_image, XYSamplesScatter, DeltaXY,
                          planes, DeltaZ, lld, uld, RhoSamplesScatterEstim+2,
                          ThetaSamplesScatterEstim, num_angular_samps,
                          step_size, max_scatter_samps_xy,
                          max_scatter_samps_z, sampling_grid_xy,
                          sampling_grid_z, loglevel+2);
                          // get scatter sinogram with uniform sampling on ring
     scatt_sim=scsim->simulateScatter(emission_image, &zplanes[0],
                                      zplanes.size(), max_threads);
     delete scsim;
     scsim=NULL;
     Logging::flog()->logMsg("finished scatter simulation in #1 sec",
                             loglevel+1)->arg(sw.stop());
     Logging::flog()->logMsg("calculate beta-spline interpolation from "
                             "#1x#2x#3 to #4x#5x#6", loglevel+1)->
      arg(RhoSamplesScatterEstim)->arg(ThetaSamplesScatterEstim)->
      arg(zplanes.size())->arg(RhoSamples)->arg(ThetaSamples)->arg(planes);
                                      // create scatter matrix in original size
     scat=MemCtrl::mc()->createFloat(scatter_plane_size*
                                     (unsigned long int)planes, &scat_idx,
                                     "scat", loglevel+1);
     memset(scat, 0, scatter_plane_size*(unsigned long int)planes*
                     sizeof(float));
                                     // interpolate scatter matrix in radial an
                                     // angular direction to get desired size
     for (unsigned short int p=0; p < zplanes.size(); p++)
      { ss=new Matrix <float>(scatt_sim+
                               (unsigned long int)p*
                               (unsigned long int)(RhoSamplesScatterEstim+2)*
                               (unsigned long int)ThetaSamplesScatterEstim,
                              RhoSamplesScatterEstim+2,
                              ThetaSamplesScatterEstim);
                         // multiply scatter matrix with interpolation matrices
        result=new Matrix <float>();
                                   // result=asplines_resamp*ss*rsplines_resamp
        *result=*asplines_resamp;
        *result*=*ss;
        delete ss;
        ss=NULL;
        *result*=*rsplines_resamp;
        memcpy(scat+(unsigned long int)zplanes[p]*scatter_plane_size,
               result->data(), scatter_plane_size*sizeof(float));
        delete result;
        result=NULL;
      }
     delete[] scatt_sim;
     scatt_sim=NULL;
     delete asplines_resamp;
     asplines_resamp=NULL;
     delete rsplines_resamp;
     rsplines_resamp=NULL;
                                   // interpolate scatter matrix in z-direction
     for (i=0; i < planes; i++)
      { bool found=false;
                                                  // does plane already exist ?
        for (unsigned short int j=0; j < zplanes.size(); j++)
         if (i == zplanes[j]) { found=true;
                                break;
                              }
        if (!found)
         { float *scat_plane;
                                    // calculate scatter plane by interpolation
           scat_plane=scat+i*scatter_plane_size;
           for (unsigned short int j=0; j < zplanes.size(); j++)
            { float *sp;

              sp=scat+zplanes[j]*scatter_plane_size;
              for (unsigned long int k=0; k < scatter_plane_size; k++)
               scat_plane[k]+=(*zsplines)(j, i)*sp[k];
            }
         }
      }
     delete zsplines;
     zsplines=NULL;
                                  // cut off negative value in scatter sinogram
     for (i=0; i < scatter_plane_size*(unsigned long int)planes; i++)
      if (scat[i] < 0.0f) scat[i]=0.0f;
     MemCtrl::mc()->put(scat_idx);
     return(scat_idx);
   }
   catch (...)
    { sw.stop();
      if (result != NULL) delete result;
      if (ss != NULL) delete ss;
      if (scsim != NULL) delete scsim;
      if (scatt_sim != NULL) delete[] scatt_sim;
      if (zsplines != NULL) delete zsplines;
      if (asplines_resamp != NULL) delete asplines_resamp;
      if (rsplines_resamp != NULL) delete rsplines_resamp;
      if (splines != NULL) delete splines;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate scaling factors for scatter estimation.
    \param[in] acf_sino            3d attenuation sinogram
    \param[in] emi_sino            normalized emission sinogram
    \param[in] scatter_sino        scatter estimation sinogram
    \param[in] underestimate       correction factor for scatter scaling factors
    \param[in] acf_threshold       threshold for acf
    \param[in] mnr                 matrix number
    \param[in] filter_width        with of boxcar filter in pixel
    \param[in] scatter_scale_min   minimum value for scatter scale factors
    \param[in] scatter_scale_max   maximum value for scatter scale factors
    \param[in] max_threads         maximum number of threads
    \return scatter scale factors for each slice of the 3d sinogram

    Calculate scaling factors for scatter estimation. The factors are
    calculated based on the countrate of the emission scan. Then the lower and
    upper thresholds are applied and the factors are smoothed with a boxcar
    filter. Finally some data for the scatter QC is stored.
 */
/*---------------------------------------------------------------------------*/
std::vector <float> Scatter::ScatterScaleFactors(SinogramConversion *acf_sino,
                                    SinogramConversion *emi_sino,
                                    SinogramConversion *scatter_sino,
                                    const float underestimate,
                                    const float acf_threshold,
                                    const unsigned short int acf_threshold_margin,
                                    const unsigned short int mnr,
                                    unsigned short int filter_width,
                                    const float scatter_scale_min,
                                    const float scatter_scale_max,
                                    const unsigned short int max_threads) const
 { std::ofstream *file=NULL;
   Filter *filter=NULL;
   RebinSinogram *rebin=NULL;
   FILE *mask_fp=NULL;

   try
   { float *a3dp, *emp, *sss, *atten3d_axis;
#ifndef USE_OLD_SCATTER_CODE
     float *scatter_2d=0;
     int seg_offset=0;
#endif
     unsigned short int p, acf_idx, spl=0;
     tfilter f;
     std::vector <float> scale, scale_smooth;

     scale.resize(emi_sino->axesSlices());
     scale_smooth.resize(emi_sino->axesSlices());
     if ((acf_sino->RhoSamples() != emi_sino->RhoSamples()) ||
         (acf_sino->ThetaSamples() != emi_sino->ThetaSamples()))
      rebin=new RebinSinogram(acf_sino->RhoSamples(), acf_sino->ThetaSamples(),
                              emi_sino->RhoSamples(), emi_sino->ThetaSamples(),
                              RebinX <float>::NO_ARC, 0, true);
     Logging::flog()->logMsg("acf threshold=#1", loglevel+1)->
      arg(acf_threshold);
     int nprojs = emi_sino->RhoSamples();
#ifndef USE_OLD_SCATTER_CODE
     scatter_2d = MemCtrl::mc()->getFloatRO(scatter_sino->index(0, 0),
                                      loglevel+1);
     if (acf_threshold_margin>0) 
     { std::string mask_fname =scatter_qc_path+"/acf_mask.a";
       mask_fp = fopen(mask_fname.c_str(), "wb");
     }
#endif
   for (unsigned short int segment=0;
          segment < emi_sino->axes()*2-1;
          segment++)
      { unsigned short int planes, axis, offs;

        Logging::flog()->logMsg("calculate scale factors for segment #1",
                                loglevel+1)->arg(segStr(segment));
        axis=(segment+1)/2;
        if (segment == 0) planes=emi_sino->axisSlices()[0];
         else planes=emi_sino->axisSlices()[axis]/2;
        if ((segment > 0) && ((segment & 0x1) == 0))
         offs=planes+skip_outer_planes;
         else offs=skip_outer_planes;
        emp=MemCtrl::mc()->getFloatRO(emi_sino->index(axis, 0), loglevel+1)+
            (unsigned long int)offs*emi_sino->planeSize();
#ifndef USE_OLD_SCATTER_CODE
        offs=skip_outer_planes;
        seg_offset = (scatter_sino->axisSlices()[0]-planes)/2;
        sss = scatter_2d+(offs+seg_offset)*scatter_sino->planeSize();
#else
        sss=MemCtrl::mc()->getFloatRO(scatter_sino->index(axis, 0),
                                      loglevel+1)+
            (unsigned long int)offs*scatter_sino->planeSize();   
#endif
    
        Logging::flog()->logMsg("scatter size #1x#2x#3",
                                loglevel+1)->arg(emi_sino->RhoSamples())->arg(emi_sino->ThetaSamples())->arg(planes);

        atten3d_axis=MemCtrl::mc()->getFloatRO(acf_sino->index(axis, 0),
                                               loglevel+1)+
                     (unsigned long int)offs*acf_sino->planeSize();
            // resize attenuation planes to size of scatter planes if necessary
        if (rebin != NULL)
         { atten3d_axis=rebin->calcRebinSinogram(atten3d_axis,
                                                 planes-2*skip_outer_planes,
                                                 NULL, &acf_idx, loglevel+1,
                                                 max_threads);
           MemCtrl::mc()->put(acf_sino->index(axis, 0));
         }
        for (a3dp=atten3d_axis,
             p=skip_outer_planes; p < planes-skip_outer_planes; p++,
             sss+=scatter_sino->planeSize(),
             emp+=emi_sino->planeSize(),
             a3dp+=emi_sino->planeSize())
         { float sum_nom, sum_den, sum_emi, sum_sss;

           sum_nom=0.0f;
           sum_den=0.0f;
           sum_emi=0.0f;
           sum_sss=0.0f;
           /*
           unsigned short int tm;

           tm=emi_sino->ThetaSamples();

           for (unsigned short int t=0; t < emi_sino->ThetaSamples(); t++)
            if ((t > tm/2-tm/6) && (t < tm/2+tm/6))
             for (unsigned short int r=0; r < emi_sino->RhoSamples(); r++)
              { unsigned long int tr;

                tr=(unsigned long int)t*
                   (unsigned long int)emi_sino->RhoSamples()+r;
                if (a3dp[tr] <= acf_threshold)
                 { sum_nom+=sss[tr]*sss[tr]*emp[tr];
                   sum_den+=sss[tr]*sss[tr]*sss[tr];
                   sum_sss+=sss[tr];
                   sum_emi+=emp[tr];
                 }
              }
              }
           */

           for (int view=0; view < emi_sino->ThetaSamples(); view++)
           {
             int tr=0, tr_end=view*emi_sino->RhoSamples(), proj=0;
             // search object border and use left bins excluding margin
             while (a3dp[tr_end] <= acf_threshold && proj < emi_sino->RhoSamples())
             { tr_end++; 
               proj++;
             }
             for (tr=view*emi_sino->RhoSamples(); tr<tr_end-acf_threshold_margin; tr++)
             { sum_nom+=sss[tr]*sss[tr]*emp[tr];
               sum_den+=sss[tr]*sss[tr]*sss[tr];
               sum_sss+=sss[tr];
               sum_emi+=emp[tr];
             }
             // show margin
             while (tr<tr_end) a3dp[tr++] = 2.5f;

            // search object border and use left bins excluding margin
             tr_end=(view+1)*emi_sino->RhoSamples()-1, proj=emi_sino->RhoSamples()-1;
             while (a3dp[tr_end] <= acf_threshold && proj > 0)
             { tr_end--; 
               proj--;
             }
             for (tr=(view+1)*emi_sino->RhoSamples()-1; tr > tr_end+acf_threshold_margin; tr--)
             { sum_nom+=sss[tr]*sss[tr]*emp[tr];
               sum_den+=sss[tr]*sss[tr]*sss[tr];
               sum_sss+=sss[tr];
               sum_emi+=emp[tr];
             }
             // show margin
             while (tr>tr_end) a3dp[tr--] = 2.5f;
           }
           if (sum_den != 0.0f) scale[spl+p]=underestimate*sum_nom/sum_den;
           else scale[spl+p]=1.0f;
           if (mask_fp != NULL) 
             fwrite(a3dp, sizeof(float), emi_sino->planeSize(), mask_fp);
//  if (segment==0 && p>104) exit(1);
//            Logging::flog()->logMsg("ScatterScaleFactors #1*#2/#3=#4",loglevel)->arg(underestimate)->arg(sum_nom)->arg(sum_den)->arg(scale[spl+p]);
        }
#ifdef USE_OLD_SCATTER_CODE
        MemCtrl::mc()->put(scatter_sino->index(axis, 0));
#endif
        MemCtrl::mc()->put(emi_sino->index(axis, 0));
        if (rebin != NULL)
         { MemCtrl::mc()->put(acf_idx);
           MemCtrl::mc()->deleteBlock(&acf_idx);
         }
         else MemCtrl::mc()->put(acf_sino->index(axis, 0));
                                              // copy values for missing planes
        for (p=0; p < skip_outer_planes; p++)
         { scale[spl+p]=scale[spl+skip_outer_planes];
           scale[spl+planes-skip_outer_planes+p]=
            scale[spl+planes-skip_outer_planes-1];
         }
                                                            // apply thresholds

        if ((scatter_scale_min != 0.0f) || (scatter_scale_max != 0.0f))
         { Logging::flog()->logMsg("apply lower and upper thresholds #1 and "
                                   "#2", loglevel+2)->arg(scatter_scale_min)->
            arg(scatter_scale_max);
           for (p=0; p < planes; p++)
            if (scale[spl+p] < scatter_scale_min)
             scale[spl+p]=scatter_scale_min;
             else if (scale[spl+p] > scatter_scale_max)
                   scale[spl+p]=scatter_scale_max;
         }
                                      // smooth scaling factors (boxcar filter)
        memcpy(&scale_smooth[spl], &scale[spl], planes*sizeof(float));
        f.name=BOXCAR;
        f.restype=FWHM;
        f.resolution=filter_width;                     // filter-width in units
        filter=new Filter(1, planes, f, 1.0f, 1.0f, false, max_threads);
        filter->calcFilter(&scale_smooth[spl], max_threads);
        delete filter;
        filter=NULL;
        spl+=planes;
      }

#ifndef USE_OLD_SCATTER_CODE
        MemCtrl::mc()->put(scatter_sino->index(0, 0));
#endif
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     if (mask_fp!=NULL) fclose(mask_fp);
                                                         // create files for QC
     if (scatter_qc)
      { unsigned short int spl=0;

        for (unsigned short int segment=0;
             segment < emi_sino->axes()*2-1;
             segment++)
         { std::string fname;
           unsigned short int planes;

           if (segment == 0) planes=emi_sino->axisSlices()[0];
            else planes=emi_sino->axisSlices()[(segment+1)/2]/2;

           fname=scatter_qc_path+"/scatter_scale_"+toStringZero(mnr, 2)+"_"+
                 toStringZero(segment, 2)+".dat";
           file=new std::ofstream(fname.c_str());
           *file << "# scatter scaling factors (axial) for segment "
                 << segment << "\n";
           for (p=0; p < planes; p++)
            *file << scale[spl+p] << " " << scale_smooth[spl+p] << "\n";
           file->close();
           delete file;
           file=NULL;
           spl+=planes;
         }
      }
     return(scale_smooth);
   }
   catch (...)
    { if (scatter_qc)
       if (file != NULL) { file->close();
                           delete file;
                         }
      if (filter != NULL) delete filter;
      throw;
    }
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Calculate single-slice rebinned sinogram with attenuation
           correction.
    \param[in] acf_sino       3d attenuation sinogram
    \param[in] emi_sino       normalized 3d emission sinogram
    \param[in] mnr            matrix number
    \param[in] max_threads    maximum number of threads to use
    \return 2d sinogram

    Calculate single-slice rebinned sinogram with attenuation correction. The
    rebinning of the attenuation sinogram is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
SinogramConversion *Scatter::SingleSliceRebinnedSinogram(
                                          SinogramConversion *acf_sino,
                                          SinogramConversion *emi_sino,
                                          const unsigned short int mnr,
                                          const unsigned short int max_threads)
 { SSR *ssrebin=NULL;
   SinogramConversion *ssr_sino=NULL;
   RebinSinogram *rebin=NULL;
   StopWatch sw;

   sw.start();
   try
   { unsigned short int ssr_idx;

     ssr_sino=new SinogramConversion(emi_sino->RhoSamples(),
                                     emi_sino->BinSizeRho(),
                                     emi_sino->ThetaSamples(),
                                     emi_sino->span(), emi_sino->mrd(),
                                     emi_sino->planeSeparation(),
                                     (emi_sino->axisSlices()[0]+1)/2,
                                     emi_sino->intrinsicTilt(), false,
                                     emi_sino->LLD(),
                                     emi_sino->ULD(), emi_sino->startTime(),
                                     emi_sino->frameDuration(),
                                     emi_sino->gateDuration(),
                                     emi_sino->halflife(),
                                     emi_sino->bedPos(), mnr, true);
     Logging::flog()->logMsg("start single-slice rebinning from #1x#2x#3 to "
                             "#4x#5x#6", loglevel)->
      arg(emi_sino->RhoSamples())->arg(emi_sino->ThetaSamples())->
      arg(emi_sino->axesSlices())->arg(emi_sino->RhoSamples())->
      arg(emi_sino->ThetaSamples())->arg(ssr_sino->axesSlices());
     ssrebin=new SSR(emi_sino->RhoSamples(), emi_sino->ThetaSamples(),
                     emi_sino->axisSlices(), skip_outer_planes, loglevel);
     if ((acf_sino->RhoSamples() != emi_sino->RhoSamples()) ||
         (acf_sino->ThetaSamples() != emi_sino->ThetaSamples()))
      rebin=new RebinSinogram(acf_sino->RhoSamples(), acf_sino->ThetaSamples(),
                              emi_sino->RhoSamples(), emi_sino->ThetaSamples(),
                              RebinX <float>::NO_ARC, 0, true);
     for (unsigned short int axis=0; axis < emi_sino->axes(); axis++)
      { float *e3dp, *sino_corr, *atten_3d;
        unsigned long int axis_size;
        unsigned short int sino_corr_idx, acf_idx;

        if (skip_outer_planes > 0)
         Logging::flog()->logMsg("calculate e*n*a of axis #1 (skip #2 border "
                                 "planes)", loglevel+1)->arg(axis)->
          arg(skip_outer_planes);
         else Logging::flog()->logMsg("calculate #1 of axis #2", loglevel+1)->
               arg(axis);
        axis_size=emi_sino->planeSize()*
                  (unsigned long int)emi_sino->axisSlices()[axis];
                                                        // get attenuation data
        atten_3d=MemCtrl::mc()->getFloatRO(acf_sino->index(axis, 0),
                                           loglevel+2);
        if (rebin != NULL)          // rebin attenuation plane to emission size
         { Logging::flog()->logMsg("rebin acf axis #1 from #2x#3x#4 to "
                                   "#5x#6x#7", loglevel+2)->arg(axis)->
            arg(acf_sino->RhoSamples())->arg(acf_sino->ThetaSamples())->
            arg(acf_sino->axisSlices()[axis])->arg(emi_sino->RhoSamples())->
            arg(emi_sino->ThetaSamples())->arg(emi_sino->axisSlices()[axis]);
           atten_3d=rebin->calcRebinSinogram(atten_3d,
                                             acf_sino->axisSlices()[axis],
                                             NULL, &acf_idx, loglevel+2,
                                             max_threads);
           MemCtrl::mc()->put(acf_sino->index(axis, 0));
         }
        e3dp=MemCtrl::mc()->getFloatRO(emi_sino->index(axis, 0), loglevel+2);
                                           // correct emission with attenuation
        Logging::flog()->logMsg("calculate axis #1 of corrected sinogram",
                                loglevel+2)->arg(axis);
        sino_corr=MemCtrl::mc()->createFloat(axis_size, &sino_corr_idx, "corr",
                                             loglevel+2);
        memcpy(sino_corr, e3dp, axis_size*sizeof(float));
        MemCtrl::mc()->put(emi_sino->index(axis, 0));
        vecMul(sino_corr, atten_3d, sino_corr, axis_size);
        if (rebin != NULL)
         { MemCtrl::mc()->put(acf_idx);
           MemCtrl::mc()->deleteBlock(&acf_idx);
         }
         else MemCtrl::mc()->put(acf_sino->index(axis, 0));
                                                      // single-slice rebinning
        Logging::flog()->logMsg("calculate single-slice rebinning of axis #1",
                                loglevel+1)->arg(axis);
        ssrebin->Rebinning(sino_corr, axis);
        MemCtrl::mc()->put(sino_corr_idx);
        MemCtrl::mc()->deleteBlock(&sino_corr_idx);
      }
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     Logging::flog()->logMsg("normalize sinogram", loglevel+1);
     ssr_idx=ssrebin->Normalize();
     delete ssrebin;
     ssrebin=NULL;
     ssr_sino->copyData(MemCtrl::mc()->getFloatRO(ssr_idx, loglevel+1), 0, 1,
                        ssr_sino->axesSlices(), 1, false, false, "ssr",
                        loglevel+1);
     MemCtrl::mc()->put(ssr_idx);
     MemCtrl::mc()->deleteBlock(&ssr_idx);
     Logging::flog()->logMsg("finished single-slice rebinning in #1 sec",
                             loglevel)->arg(sw.stop());
     return(ssr_sino);
   }
   catch (...)
    { sw.stop();
      if (ssrebin != NULL) delete ssrebin;
      if (ssr_sino != NULL) delete ssr_sino;
      throw;
    }
 }
