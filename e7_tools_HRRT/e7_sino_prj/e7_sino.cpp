/*! \file e7_sino.cpp
    \brief Calculate fully corrected 2d or 3d sinogram or calculate 3d scatter
           sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2004/01/01 initial version
    \date 2005/01/19 added Doxygen style comments
    \date 2005/01/23 cleanup handling of unknown exceptions
    \date 2005/02/22 use LLD,ULD from gantry model if not specified by "--scan"
    \date 2008/05/23 added --os2d to output unscaled 2D scatter and factors
    \date 2008/07/23 added --lber to override scatter BG from gmini 
    \date 2010/02/16 added --athr value[,margin] to override scatter BG from gmini 

    Calculate fully corrected 2d or 3d sinogram or calculate 3d scatter
    sinogram.
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <limits>
#if defined(__LINUX__) || defined(__SOLARIS__)
#include <new>
#endif
#ifdef WIN32
#include <new.h>
#endif
#include "convert.h"
#include "dift.h"
#include "e7_common.h"
#include "exception.h"
#include "gm.h"
#include "hwinfo.h" 
#ifdef SUPPORT_NEW_SCATTER_CODE
#if defined(__LINUX__) || defined(WIN32) || defined(__SOLARIS__)
#include "idl_interface.h"
#endif
#endif
#include "image_conversion.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "parser.h"
#include "progress.h"
#include "sinogram_conversion.h"
#include "stopwatch.h"
#include "types.h"

/*- constants ---------------------------------------------------------------*/

                       /*! default iterations for OSEM in scatter estimation */
const unsigned short int default_iterations_scatter_osem=3,
                          /*! default subsets for OSEM in scatter estimation */
                         default_subsets_scatter=8;

/*---------------------------------------------------------------------------*/
/*! \brief Calculate a fully corrected 2d or 3d sinogram or calculate a scatter
           sinogram.
    \param[in] v   command line parameters

    Calculate a fully corrected 2d or 3d sinogram or calculate a scatter
    sinogram. The calculation consists of the following steps (depending on
    the command line switches):
    - load emission
    - randoms smoothing
    - randoms subtraction
    - mask dead blocks in emission sinogram
    - untilt emission sinogram
    - radial un-arc correction
    - axial un-arc correction
    - load/calculate u-map and acf
    - load norm data
    - calculate norm sinogram
    - mask dead blocks in norm sinogram
    - untilt norm sinogram
    - mask borders of norm sinogram
    - normalize emission
    - fill gaps in normalized emission
    - radial arc correction of normalized emission
    - axial arc correction of normalized emission
    - trim normalized emission
    - rebin normalized emission
    - calculate scatter
    - subtract scatter
    - correct for attenuation
    - decay and framelength correction
    - tilt sinogram
    - rebin 3d -> 2d
    - save fully corrected emission sinogram
    - save scatter sinogram
 */
/*---------------------------------------------------------------------------*/
void calculateSinogram(Parser::tparam * const v)
 { float *acf3d=NULL, *umap=NULL;
   SinogramConversion *emi_sino=NULL, *acf_sino=NULL, *norm_sino=NULL,
                      *scatter_sino=NULL;
   ImageConversion *umap_image=NULL;
   DIFT *dift=NULL;

   try
   { unsigned short int num_log_cpus, i, number_of_matrices, mat_offset;
     std::string str, mu;
     std::vector <unsigned short int> matrices;

#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
     char c[2];

     c[1]=0;
     c[0]=(char)181;
     mu=std::string(c);
#endif
#ifdef WIN32
     mu='u';
#endif
     num_log_cpus=logicalCPUs();
                                              // init geometry (for flat files)
     if (GM::gm()->initialized())
      { unsigned short int rs, ts;
        float bsr;

        if (v->mrd == 0) v->mrd=GM::gm()->mrd();
        if (v->span == 0) v->span=GM::gm()->span();
        if (v->si.lld == 0) v->si.lld=GM::gm()->lld();
        if (v->si.uld == 0) v->si.uld=GM::gm()->uld();
        rs=GM::gm()->RhoSamples();
        ts=GM::gm()->ThetaSamples();
        bsr=GM::gm()->BinSizeRho();
        if (v->eRhoSamples > 0)
         { rs=v->eRhoSamples;
           bsr*=(float)rs/(float)v->eRhoSamples;
         }
        if (v->eThetaSamples > 0) ts=v->eThetaSamples;
                                                               // init sinogram
        emi_sino=new SinogramConversion(rs, bsr, ts, v->span, v->mrd,
                                        GM::gm()->planeSeparation(),
                                        GM::gm()->rings(),
                                        GM::gm()->intrinsicTilt(), true,
                                        v->si.lld, v->si.uld, v->si.start_time,
                                        v->si.frame_duration,
                                        v->si.gate_duration, v->si.halflife,
                                        v->si.bedpos, 0, v->edim);
      }
      else { unsigned short int rs=0, ts=0;

             if (v->eRhoSamples > 0) rs=v->eRhoSamples;
             if (v->eThetaSamples > 0) ts=v->eThetaSamples;
                                                               // init sinogram
             emi_sino=new SinogramConversion(rs, 0.0f, ts, v->span, v->mrd,
                                             0.0f, 0, 0.0f, true, v->si.lld,
                                             v->si.uld, v->si.start_time,
                                             v->si.frame_duration,
                                             v->si.gate_duration,
                                             v->si.halflife, v->si.bedpos, 0,
                                             v->edim);
          }
                                               // get number of emission frames
     number_of_matrices=numberOfMatrices(v->emission_filename);
     matrices.clear();
     if (v->append)         // take existing matrices into account if appending
      { try { Logging::flog()->loggingOn(false);
              mat_offset=numberOfMatrices(v->oemission_filename);
              Logging::flog()->loggingOn(true);
            }
        catch (const Exception r)
         { Logging::flog()->loggingOn(true);
           if (r.errCode() == REC_FILE_DOESNT_EXIST) mat_offset=0;
            else throw;
         }
        matrices.assign(mat_offset, 0);
      }
      else mat_offset=0;
     if (v->matrix_num.size() > 0)
      { for (i=0; i < v->matrix_num.size(); i++)
         if (v->matrix_num[i] < number_of_matrices)
          matrices.push_back(v->matrix_num[i]);
      }
      else if (v->last_mnr == std::numeric_limits <unsigned short int>::max())
            for (i=0; i < number_of_matrices; i++)
             matrices.push_back(i);
      else if (v->first_mnr < number_of_matrices)
            if (v->last_mnr >= number_of_matrices)
             for (i=v->first_mnr; i < number_of_matrices; i++)
              matrices.push_back(i);
             else for (i=v->first_mnr; i <= v->last_mnr; i++)
                   matrices.push_back(i);
                                                          // process all frames
     if (matrices.size() == 1)
      Logging::flog()->logMsg("calculate sinogram for 1 matrix", 0);
     else
       Logging::flog()->logMsg("calculate sinogram for #3 matrices", 0)->
            arg(matrices.size());
     for (unsigned short int num=mat_offset; num < matrices.size(); num++)
      { std::string nr;
        bool simulate_scatter=false, attenuation_corrected;

        nr=toStringZero(matrices[num], 2);
                                                       // load 3d emission scan
        Logging::flog()->logMsg("loading the 3d emission scan (matrix #1)",
                                1)->arg(matrices[num]);
                                                               // load emission
        emi_sino->load(v->emission_filename, false, v->emission_arc_corrected,
                       matrices[num], "emis", 2);
        if (v->emission_arc_corrected_overwrite)
         if (v->emission_arc_corrected)
          emi_sino->correctionsApplied(emi_sino->correctionsApplied() |
                                       CORR_Radial_Arc_correction |
                                       CORR_Axial_Arc_correction);
          else emi_sino->correctionsApplied(emi_sino->correctionsApplied() ^
                                            (CORR_Radial_Arc_correction |
                                             CORR_Axial_Arc_correction));
        if (!v->tof_processing) emi_sino->addTOFbins(1);        // add TOF bins
                                                   // perform randoms smoothing
        if (v->randoms_smoothing && v->debug)
         { emi_sino->smoothRandoms(1, num_log_cpus);
                                                             // save debug file
           emi_sino->saveFlat(v->debug_path+"/psmr_"+nr, false, 2);
         }
                                                             // calculate trues
        emi_sino->calculateTrues(v->randoms_smoothing && !v->debug, 1,
                                 num_log_cpus);
                                                     // kill blocks in emission
        emi_sino->maskOutBlocks(v->kill_blocks, 1);
        emi_sino->untilt(true, 1);                           // untilt emission
                                                               // unarc-correct
        emi_sino->radialArcCorrection(v->no_radial_arc, false, 1,
                                      num_log_cpus);
        emi_sino->axialArcCorrection(v->no_axial_arc, false, 1, num_log_cpus);
     // emi_sino->trim(v->trim, "emis", 1);                    // trim sinogram
        if (v->debug)                                        // save debug file
         emi_sino->saveFlat(v->debug_path+"/emis3d_"+nr, false, 2);

                                         // calculate the 3d normalization scan
        if ((((emi_sino->correctionsApplied() & CORR_Normalized) == 0) ||
             v->gap_filling) && !v->norm_filename.empty())
         { Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2,
                                   "calculate normalization data (frame #1)")->
            arg(matrices[num]);
           Logging::flog()->logMsg("calculating the 3d normalisation scan "
                                   "(matrix #1)", 1)->arg(matrices[num]);
                                              // init geometry (for flat files)
           if (GM::gm()->initialized())
            { unsigned short int rs, ts;
              float bsr;

              if (v->mrd == 0) v->mrd=GM::gm()->mrd();
              if (v->span == 0) v->span=GM::gm()->span();
              rs=GM::gm()->RhoSamples();
              ts=GM::gm()->ThetaSamples();
              bsr=GM::gm()->BinSizeRho();
              if (v->nRhoSamples > 0)
               { rs=v->nRhoSamples;
                 bsr*=(float)rs/(float)v->nRhoSamples;
               }
              if (v->nThetaSamples > 0) ts=v->nThetaSamples;
                                                               // init sinogram
              norm_sino=new SinogramConversion(rs, bsr, ts, v->span, v->mrd,
                                               GM::gm()->planeSeparation(),
                                               GM::gm()->rings(),
                                               GM::gm()->intrinsicTilt(), true,
                                               v->si.lld, v->si.uld,
                                               v->si.start_time,
                                               v->si.frame_duration,
                                               v->si.gate_duration,
                                               v->si.halflife, v->si.bedpos,
                                               num, false);
           }
           else { unsigned short int rs=0, ts=0;

                  if (v->nRhoSamples > 0) rs=v->nRhoSamples;
                  if (v->nThetaSamples > 0) ts=v->nThetaSamples;
                                                               // init sinogram
                  norm_sino=new SinogramConversion(rs, 0.0f, ts, v->span,
                                                   v->mrd, 0.0f, 0, 0.0f, true,
                                                   v->si.lld, v->si.uld,
                                                   v->si.start_time,
                                                   v->si.frame_duration,
                                                   v->si.gate_duration,
                                                   v->si.halflife,
                                                   v->si.bedpos,
                                                   num, v->adim);
                }
                                                        // load and expand norm
           norm_sino->loadNorm(v->norm_filename, norm_sino->RhoSamples(),
                               norm_sino->ThetaSamples(),
                               emi_sino->axisSlices(), 1.0f,
                               emi_sino->singles(), v->p39_patient_norm, 2);
                                                     // kill blocks in sinogram
           norm_sino->maskOutBlocks(v->kill_blocks, 1);
           norm_sino->untilt(true, 1);                           // untilt norm
                                                    // mask border bins of norm
           norm_sino->maskBorders(v->norm_mask_bins, 1);
        // norm_sino->trim(v->trim, "norm", 1);                // trim sinogram
           if (v->debug)                                     // save debug file
            norm_sino->saveFlat(v->debug_path+"/norm3d_"+nr, false, 2);
           if ((emi_sino->correctionsApplied() & CORR_Normalized) == 0)
            {                                          // normalize 3d emission
              Logging::flog()->logMsg("normalize 3d emission", 1);
              emi_sino->setECF(norm_sino->ECF());
              emi_sino->setDeadTimeFactor(norm_sino->deadTimeFactor());
              emi_sino->setCalibrationTime(norm_sino->calibrationTime());
              emi_sino->multiply(norm_sino, !v->gap_filling, CORR_Normalized,
                                 2, num_log_cpus);
            }
           if (v->gap_filling)                         // fill Gaps in emission
            { Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 2,
                                       "gap filling (frame #1)")->
               arg(matrices[num]);
              if ((GM::gm()->modelNumber() != "1080") &&
                  (GM::gm()->modelNumber() != "1090"))
               emi_sino->fillGaps(norm_sino, v->norm_mask_bins, true, 1,
                                  num_log_cpus);
               else emi_sino->fillGaps(1, num_log_cpus);
            }
           delete norm_sino;
           norm_sino=NULL;
         }
         else if (v->gap_filling)                      // fill Gaps in emission
               emi_sino->fillGaps(1, num_log_cpus);

                                            // do we need to simulate scatter ?
        if (v->scatter_corr)
         if (((emi_sino->correctionsApplied() &
               (CORR_2D_scatter_correction |
                CORR_3D_scatter_correction)) == 0) ||
             !v->oscatter_filename.empty())
          if (!v->acf_filename.empty() || !v->umap_filename.empty())
           if ((emi_sino->correctionsApplied() & CORR_Normalized) ||
               !v->norm_filename.empty())
            simulate_scatter=true;
        attenuation_corrected=(emi_sino->correctionsApplied() &
                               (CORR_Measured_Attenuation_Correction |
                                CORR_Calculated_Attenuation_Correction));

        if (simulate_scatter ||
            (!attenuation_corrected &&
             (!v->acf_filename.empty() || !v->umap_filename.empty())) ||
            (!v->acf_filename.empty() && !v->oumap_filename.empty()) ||
            (!v->umap_filename.empty() && !v->oacf_filename.empty()))
         { unsigned short int a_mnr=0, u_mnr=0;
           float em_pos, umap_pos, acf_pos;
                      // search attenuation bed that is closest to emission bed
           em_pos=emi_sino->bedPos();
           Logging::flog()->logMsg("bed position in emission=#1mm", 2)->
            arg(em_pos);
           if (!v->umap_filename.empty())
            { u_mnr=findMNR(v->umap_filename, false, em_pos, &umap_pos);
              Logging::flog()->logMsg("bed position in u-map=#1mm", 2)->
               arg(umap_pos);
            }
           if (!v->acf_filename.empty())
            { a_mnr=findMNR(v->acf_filename, true, em_pos, &acf_pos);
              Logging::flog()->logMsg("bed position in acf=#1mm", 2)->
               arg(acf_pos);
            }
           if (v->iwidth == 0) v->iwidth=GM::gm()->RhoSamples();
           CONVERT::get_umap_acf(v, &umap_image, &acf_sino,
                                 simulate_scatter || !attenuation_corrected,
                                 simulate_scatter, emi_sino->span(),
                                 emi_sino->mrd(), a_mnr, u_mnr, num,//matrices[num],
                                 1, num_log_cpus);
           if (!v->umap_filename.empty())
            if (!v->oacf_filename.empty() && v->acf_filename.empty() &&
                (numberOfMatrices(v->umap_filename) == 1))
             { v->acf_filename=v->oacf_filename;
               v->oacf_filename=std::string();
             }
         }
                                               // rebin acf to size of emission
        if (acf_sino != NULL)
         { acf_sino->resampleRT(emi_sino->RhoSamples()/v->rebin_r-2*v->trim,
                                emi_sino->ThetaSamples()/v->rebin_t, 1,
                                num_log_cpus);
           acf_sino->cutBelow(1.0f, 1.0f, 1);
         }
                                                        // axial arc-correction
        emi_sino->axialArcCorrection(v->no_axial_arc, true, 1, num_log_cpus);
                                               // arc correction of 3d emission
        emi_sino->radialArcCorrection(v->no_radial_arc, true, 1, num_log_cpus);
        emi_sino->trim(v->trim, "emis", 1);
        emi_sino->resampleRT(emi_sino->RhoSamples()/v->rebin_r,
                             emi_sino->ThetaSamples()/v->rebin_t, 1,
                             num_log_cpus);
                                                // simulate 2d scatter sinogram
#ifndef USE_OLD_SCATTER_CODE
        std::vector<float> scale_factors;
#endif
        if (((emi_sino->correctionsApplied() &
              (CORR_2D_scatter_correction |
               CORR_3D_scatter_correction)) == 0) ||
            !v->oscatter_filename.empty())
         if ((acf_sino != NULL) && v->scatter_corr &&
             (emi_sino->correctionsApplied() & CORR_Normalized))
          scatter_sino=CONVERT::eau2scatter(emi_sino, acf_sino, umap_image,
                                            v->trim, v->iterations, v->subsets,
                                            v->boxcar_width, v->scatter_skip,
                                            v->scatter_scale_min,
                                            v->scatter_scale_max,
                                            v->acf_threshold, 
                                            v->acf_threshold_margin, 
                                            v->rebin_r,
                                            v->quality_control,
                                            v->quality_path, v->debug,
                                            v->debug_path, matrices[num],
                                            v->new_scatter_code, 
#ifndef USE_OLD_SCATTER_CODE
                                           scale_factors,
#endif
                                            1, num_log_cpus);
        if (umap_image != NULL) { delete umap_image;
                                  umap_image=NULL;
                                }
        if (!v->oemission_filename.empty())
         { if (acf_sino != NULL)
            { double total_trues=0.0f, total_scatter=0.0f;
                                       // calculate fully corrected 3d sinogram
              if (emi_sino->correctionsApplied() & CORR_Normalized) str="e*n";
               else str="e";
              if ((scatter_sino != NULL) && v->scatter_corr)
               str="("+str+"-s)*a";
               else str+="*a";
              Logging::flog()->logMsg("calculate corrected 3d sinogram #1",
                                      1)->arg(str);
              if (v->scatter_corr && (scatter_sino != NULL))
               if (emi_sino->TOFbins() > 1)
                if (scatter_sino->axes() > 1)
                 emi_sino->subtract(scatter_sino, v->oscatter_filename.empty(),
                                    CORR_3D_scatter_correction, 2);
                 else emi_sino->subtract2D(scatter_sino,
                                           v->oscatter_filename.empty(),
                                           CORR_3D_scatter_correction, NULL,
                                           NULL, 2);
                 else if (scatter_sino->axes() > 1)
                       emi_sino->subtract(scatter_sino,
                                         v->oscatter_filename.empty(),
                                         CORR_3D_scatter_correction, 2);
                       else emi_sino->subtract2D(scatter_sino,
                                      v->oscatter_filename.empty(),
                                      CORR_3D_scatter_correction, &total_trues,
                                      &total_scatter, 2);
              if (v->oscatter_filename.empty())
               if (scatter_sino != NULL) { delete scatter_sino;
                                           scatter_sino=NULL;
                                         }
              if (total_trues > 0.0)
               { Logging::flog()->logMsg("total statistics in 3D normalized "
                                         "true scan is #1", 3)->
                  arg(total_trues);
                 Logging::flog()->logMsg("total statistics in 3D scaled "
                                          "scattered scan is #1", 3)->
                  arg(total_scatter);
                 Logging::flog()->logMsg("scatter fraction is #1%", 3)->
                  arg(total_scatter/total_trues*100.0);
                 emi_sino->setScatterFraction((float)(total_scatter/
                                                      total_trues));
               }
              Logging::flog()->logMsg("correct for attenuation", 2);
              emi_sino->multiply(acf_sino, true,
                                 CORR_Measured_Attenuation_Correction, 3,
                                 num_log_cpus);
              delete acf_sino;
              acf_sino=NULL;
            }
                                           // frame-length and decay correction
           if (v->decay_corr || v->framelen_corr)
            emi_sino->decayCorrect(v->decay_corr, v->framelen_corr, 1);
           if (!v->oemission_arc_corrected) // un-arc correction of 3d sinogram
            { emi_sino->radialArcCorrection(v->no_radial_arc, false, 1,
                                            num_log_cpus);
              emi_sino->axialArcCorrection(v->no_axial_arc, false, 1,
                                           num_log_cpus);
            }
           if (!v->no_tilt) emi_sino->untilt(false, 1);        // tilt sinogram
           if (v->debug)                                     // save debug file
            emi_sino->saveFlat(v->debug_path+"/emis3d_corr_"+nr, false, 2);
                                          // rebin 3d emission into 2d emission
           if (v->rebin_method != SinogramConversion::NO_REB)
            { emi_sino->sino3D2D(v->rebin_method, v->a_lim, v->w_lim, v->k_lim,
                                 "emis", matrices[num], 1, num_log_cpus);
              if (v->debug)                                  // save debug file
               emi_sino->saveFlat(v->debug_path+"/emis2d_corr_"+nr, false, 2);
            }
           Logging::flog()->logMsg("save corrected sinogram matrix #1 in file "
                                   "'#2'", 1)->arg(matrices[num])->
            arg(v->oemission_filename);
                                                     // save corrected sinogram
           emi_sino->save(v->oemission_filename, v->emission_filename,
                          emi_sino->matrixFrame(), emi_sino->matrixPlane(),
                          emi_sino->matrixGate(), emi_sino->matrixData(),
                          emi_sino->matrixBed(), v->view_mode,
                          emi_sino->bedPos(), num/*matrices[num]*/, 2);
         }
        if (acf_sino != NULL) { delete acf_sino;
                                acf_sino=NULL;
                              }

        if (!v->oscatter_filename.empty())
         {                             // un-arc correction of scatter sinogram
           if (!v->oscatter_arc_corrected)
            scatter_sino->radialArcCorrection(v->no_radial_arc, false, 1,
                                              num_log_cpus);
           if (!v->no_tilt) scatter_sino->untilt(false, 1);    // tilt sinogram


           if (!v->oscatter_2d)
            {
              Logging::flog()->logMsg(
                "Create 3D scatter sinogram and apply scaling factors", 1);
              scatter_sino->sino2D3D(SinogramConversion::iSSRB_REB,
                emi_sino->span(), emi_sino->mrd(), "scat", 2, num_log_cpus);
              unsigned short int spl=0;
              for (unsigned short int axis=0; axis < scatter_sino->axes(); axis++)
               {
                 float *sssp;
                 sssp=MemCtrl::mc()->getFloat(scatter_sino->index(axis, 0), 1);
                 for (unsigned short int p=0; p < scatter_sino->axisSlices()[axis];
                   p++, spl++)
                   for (unsigned short int t=0; t < scatter_sino->ThetaSamples(); 
                     t++, sssp += scatter_sino->RhoSamples())
                     for (unsigned long int r=0; r < scatter_sino->RhoSamples(); r++) 
                       sssp[r] *= scale_factors[spl]; 
                 MemCtrl::mc()->put(scatter_sino->index(axis, 0)); 
              }
            }
                                                             // save debug file
           if (v->debug)
            scatter_sino->saveFlat(v->debug_path+"/scatter_estim3d_"+nr, false,
                                   2);
                                                    // save 3d scatter sinogram
           if (v->oscatter_2d)
             Logging::flog()->logMsg("save unscaled 2d scatter matrix #1 in file #2", 1)->
                arg(matrices[num])->arg(v->oscatter_filename);
           else
             Logging::flog()->logMsg("save 3d scatter matrix #1 in file #2", 1)->
                arg(matrices[num])->arg(v->oscatter_filename);
           if (num == 0)
            scatter_sino->copyFileHeader(emi_sino->ECAT7mainHeader(),
                                         emi_sino->InterfileHdr());

           scatter_sino->save(v->oscatter_filename, v->emission_filename,
                              emi_sino->matrixFrame(), emi_sino->matrixPlane(),
                              emi_sino->matrixGate(), emi_sino->matrixData(),
                              emi_sino->matrixBed(), v->view_mode,
                              emi_sino->bedPos(), num/*matrices[num]*/, 2);
           if (v->oscatter_2d) // append scaling factors
            {
              FILE *fp;
              Logging::flog()->logMsg("Append #1 scale factors in file #2", 1)->
                arg(scale_factors.size())->arg(v->oscatter_filename);
              if ((fp = fopen(v->oscatter_filename.c_str(), "ab+")) != NULL)
               {
                 if (fseek(fp,0,SEEK_END) == 0)
                  {
                   if (fwrite(&scale_factors[0], sizeof(float),
                               scale_factors.size(), fp) != scale_factors.size())
                    {
                      throw Exception(REC_FILE_WRITE,
                                     "Can't store data in the file '#1'. File "
                                     "system full ?").arg(v->oscatter_filename);
                    }
                  }
               } else throw Exception(REC_FILE_DOESNT_EXIST,
                          "The file '#1' doesn't exist.").arg(v->oscatter_filename);
           }
        }
        if (scatter_sino != NULL) { delete scatter_sino;
                                    scatter_sino=NULL;
                                  }
      }
     GM::gm()->printUsedGMvalues();
     delete emi_sino;
     emi_sino=NULL;
   }
   catch (...)
    { if (acf3d != NULL) delete[] acf3d;
      if (emi_sino != NULL) delete emi_sino;
      if (acf_sino != NULL) delete acf_sino;
      if (norm_sino != NULL) delete norm_sino;
      if (umap_image != NULL) delete umap_image;
      if (scatter_sino != NULL) delete scatter_sino;
      if (dift != NULL) delete dift;
      if (umap != NULL) delete[] umap;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check semantics of command line parameters.
    \param[in] v   parameters
    \return parameters correct ?

    Check semantics of command line parameters.
 */
/*---------------------------------------------------------------------------*/
bool validate(Parser::tparam * const v)
 { std::string mu;
   bool ret=true;
   {
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
     char c[2];

     c[1]=0;
     c[0]=(char)181;
     mu=std::string(c);
#endif
#ifdef WIN32
     mu='u';
#endif
   }
   if (v->emission_filename.empty())
    { std::cerr << "The name of the emission input file needs to be specified."
                   "\n";
      ret=false;
    }
   if (v->oemission_filename.empty() && v->oscatter_filename.empty())
    { std::cerr << "An emission or scatter output file needs to be specified."
                   "\n";
      ret=false;
    }
   if (!v->oscatter_filename.empty())
    if (v->acf_filename.empty() && v->umap_filename.empty())
     { std::cerr << "The calculation of a scatter sinogram requires an acf or "
                 << mu << "-map.\n";
       ret=false;
     }
   /*
   if (v->gap_filling && v->norm_filename.empty())
    { std::cerr << "Gap-Filling requires a norm file\n";
      ret=false;
    }
   */
   return(ret);
 }

/*---------------------------------------------------------------------------*/
/*! \brief main function of program to calculate a fully corrected 2d or 3d
           sinogram or a 3d scatter sinogram.
    \param[in] argc   number of arguments
    \param[in] argv   list of arguments
    \return 0 success
    \return 1 unknown failure
    \return error code

    main function of program to calculate a fully corrected 2d or 3d sinogram
    or a 3d scatter sinogram. Possible command line switches are:
    \verbatim
e7_sino - calculate fully corrected 2d or 3d sinogram or
          calculate 3d scatter sinogram

Usage:

 e7_sino (-e emission[,r,t,dim]
          (--oe emission|--os scatter) [-n norm[,r,t]|--np norm [--gf]]
          [[-a acf[,r,t,dim]|--oa acf]|[-u µ-map|--ou µ-map [-w width]]
           [--prj ifore|issrb|fwdpj]] [--mrd mrd] [--span span]
          [--mat a|a-b|a,b,...,n]
          [--model model|a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,
                         z1,z2,z3,z4,z1,z2,z3,z4,...] [--scan a,b,c,d,e,f]
          [--trim bins] [--nbm bins] [-R r,t] [-k a,b,...,n] [--rs]
          [--is iterations,subsets] [--newsc|--nosc] [--bc width] [--skip n]
          [--os2d] [--ssf min,max] [--athr acf-value[,margin]] [--lber value]
          [-r fore[,alim,wlim,klim]|ssrb|seg0] [--gf] [--tof] [--view] [--ntilt]
          [--fl] [--dc] [--nrarc] [--naarc] [--force] [-q path] [--swap path]
          [--mem fraction] [-d path] [-l a[,path]] [--resrv port] [--app])|-v|
         -h|-?

  -e      name of the emission input file. Supported formats are ECAT7,
          Interfile and flat (IEEE-float or 16 bit signed int, little endian).
          In case of a flat-file, the geometry of the gantry needs to be
          specified by the "--model" switch. If the radial or azimuthal
          dimension of the sinogram in a flat file is different from the
          standard values of the gantry, both values can be specified after the
          filename. If flat files are used, the dimensionality of the sinogram
          can be specified as "2d" or "3d". The default value in this case is
          "3d". emission files are assumed to be not arc-corrected if this
          information is not available from the file header. The switches
          "--enarc" or "--earc" can be used instead of "-e" to specify/overwrite
          this behaviour for cylindrical detector systems.
           Examples: -e file.s             (ECAT7)
                     -e file.mhdr          (Interfile)
                     -e file.dat           (flat, not arc-corrected)
                     -e file.dat,144,72,2d (flat with dimen., not arc-correc.)
                     --enarc file.s        (ECAT7, not arc-corrected)
  --earc  see "-e"
  --enarc see "-e"
  --oe    name of the emission output file. The file format depends on the
          format of the input files (ECAT7, Interfile or IEEE-float flat, little
          endian). emission files are stored without arc-correction. The switch
          "--oearc" can be used instead of "--oe" to overwrite this behaviour
          for cylindrical detector systems.
           Examples: --oe file.s      (ECAT7, not arc-corrected)
                     --oe file.mhdr   (Interfile, not arc-corrected)
                     --oe file.dat    (flat, not arc-corrected)
                     --oearc file.s   (ECAT7, arc-corrected)
  --oearc see "--oe"
  --os    name of the scatter output file. The file format depends on the
          format of the input files (ECAT7, Interfile or IEEE-float flat, little
          endian). Scatter files are stored without arc-correction. The switch
          "--osarc" can be used instead of "--os" to overwrite this behaviour
          for cylindrical detector systems.
           Examples: --os file.s      (ECAT7, not arc-corrected)
                     --oi file.mhdr   (Interfile, not arc-corrected)
                     --os file.dat    (flat, not arc-corrected)
                     --osarc file.s   (ECAT7, arc-corrected)
  --osarc see "--os"
  -n      name of the norm input file. Supported formats are ECAT7, Interfile
          and flat (IEEE-float or 16 bit signed int, little endian). In case of
          a flat-file, the geometry of the gantry needs to be specified by the
          "--model" switch. If the radial or azimuthal dimension of the sinogram
          in a flat file is different from the standard values of the gantry,
          both values can be specified after the filename. Norm files are
          assumed to be not arc-corrected.
           Examples: -n file.s          (ECAT7)
                     -n file.nhdr       (Interfile)
                     -n file.dat        (flat)
                     -n file.dat,144,72 (flat with dimensions)
  --np    name of the P39 patient norm input file
           Examples: --np file.n
  -a      name of the acf input file. Supported formats are ECAT7, Interfile
          and flat (IEEE-float or 16 bit signed int, little endian). In case of
          a flat-file, the geometry of the gantry needs to be specified by the
          "--model" switch. If the radial or azimuthal dimension of the sinogram
          in a flat file is different from the standard values of the gantry,
          both values can be specified after the filename. If flat files are
          used, the dimensionality of the sinogram can be specified as "2d" or
          "3d". The default value in this case is "3d". acf files are assumed to
          be not arc-corrected if this information is not available from the
          file header. The switches "--anarc" or "--aarc" can be used instead of
          "-a" to specify/overwrite this behaviour for cylindrical detector
          systems.
           Examples: -a file.a             (ECAT7)
                     -a file.mhdr          (Interfile)
                     -a file.dat           (flat, not arc-corrected)
                     -a file.dat,144,72,3d (flat with dimen., not arc-correc.)
                     --anarc file.a        (ECAT7, not arc-corrected)
  --aarc  see "-a"
  --anarc see "-a"
  --oa    name of the acf output file. The file format depends on the format of
          the input files (ECAT7, Interfile or IEEE-float flat, little endian).
          acf files are stored without arc-correction. The switch "--oaarc" can
          be used instead of "--oa" to overwrite this behaviour for cylindrical
          detector systems.
           Examples: --oa file.a      (ECAT7, not arc-corrected)
                     --oa file.mhdr   (Interfile, not arc-corrected)
                     --oa file.dat    (flat, not arc-corrected)
                     --oaarc file.a   (ECAT7, arc-corrected)
  -u      name of µ-map input file. Supported formats are ECAT7, Interfile and
          flat (IEEE-float, little endian). In case of a flat-file, the geometry
          of the gantry needs to be specified by the "--model" switch. The width
          and height of the µ-map are assumed to be the same as the projection
          width of the gantry if this information is not available from the file
          header. The additional switch "-w" can be used to specify/overwrite
          this behaviour.
           Examples: -u file.v          (ECAT7)
                     -u file.mhdr       (Interfile)
                     -u file.dat        (flat)
                     -u file.dat -w 128 (flat with dimensions)
  --ou    name of µ-map output file. The file format depends on the format of
          the input files (ECAT7, Interfile or IEEE-float flat, little endian).
          µ-map files are stored with a width and height equal to the projection
          width of the gantry. The additional switch "-w" can be used to
          overwrite this behaviour.
           Examples: --ou file.v          (ECAT7)
                     --ou file.mhdr       (Interfile)
                     --ou file.dat        (flat)
                     --ou file.dat -w 128 (flat with width/height)
  -w      width/height of the image. The default value is the width of the
          projection.
           Example: -w 128
  --prj   algorithm for the calculation of the 3d sinogram from the 2d
          acf/µ-map. The default algorithm is 2d-forward projection followed by
          inverse fourier rebinning. This switch is also used in the calculation
          of a 3d sinogram from a 2d sinogram. In this case are only the
          parameters "issrb" and "ifore" allowed.
           none    don't calculate 3d sinogram
           issrb   inverse single slice rebinning from 2d sinogram
           ifore   inverse fourier rebinning from 2d sinogram
           fwdpj   3d-forward projection from image
           Example: --prj issrb
  --mrd   maximum ring difference for 3d sinogram. The default value is
          specific for a given gantry.
           Example: --mrd 17
  --span  span for 3d sinogram. The default value is specific for a given
          gantry.
           Example: --span 7
  --mat   number(s) of matrix/matrices to process. The program can process a
          single matrix, a range of matrices or a set of matrices. Matrix
          numbers have to be specified in increasing order. The default is to
          process all matrices.
           Examples: --mat 3     (process only the 4th(!) matrix of a file)
                     --mat 0-2   (process the the first 3 matrices of a file)
                     --mat 1,3,5 (process the 2nd, 4th and 6th matrix of a file)
  --model gantry model specification (only for flat files)
           model   gantry model number
           a       number of bins in a projection
           b       width of bins in mm
           c       number of azimuthal angles in sinogram
           d       number of cystal rings
           e       number of crystals per ring
           f       ring spacing in mm
           g       span
           h       maximum ring difference
           i       radius of detector ring in mm
           j       interaction depth in mm
           k       intrinsic tilt in degrees
           l       transaxial crystals per block
           m       axial crystals per block
           n       transaxial blocks per bucket
           o       axial blocks per bucket
           p       number of bucket rings
           q       number of buckets per ring
           r       gap crystals per block in transaxial direction
           s       gap crystals per block in axial direction
           t       detector type (0=panel, 1=ring)
           u       gantry type (0=PET/CT, 1=PET only)
           v       axial arc correction required (0=no, 1=yes)
           w       default lower level energy threshold in keV
           x       default upper level energy threshold in keV
           y       maximum z-FOV for scatter simulation in mm
           z1      number of crystal layers
           z2      crystal layer material (LSO, BGO, NaI, LYSO)
           z3      crystal layer depth in mm
           z4      FWHM energy resolution of crystal layer in %
           z5      background energy ratio of crystal layer in %
           Example: -model 924
                    -model 320,2.2,256,120,512,2.208,7,87,436,10,0,12,12,7,10,1,
                           0,0,0,0,1,0,425,650,400.0,1,LSO,2.0,16.0,0.0
  --scan  information about scan (only for flat files)
           a   lld in keV
           b   uld in keV
           c   start time of the scan in sec
           d   frame duration of the scan in sec
           e   gate duration of the scan in sec
           f   halflife of isotope in sec
           g   position of the bed in mm
           Example: --scan 400,650,0,300,0,6586.2,245.3
  --trim  the given number of bins is cut off at each side of a projection,
          before the sinogram is reconstructed. If the "-R" switch is used at
          the same time, the "--trim" switch has precedence.
           Example: --trim 20
  --nbm   the given number of bins is set to 0 at the borders of each
          projection in the norm sinogram.
           Example: --nbm 3
  -R      rebin input sinograms by radial and azimuthal factor. Allowed values
          are 1, 2 and 4.
           Example: -R 2,2
  -k      counts measured by the detector blocks with the given numbers are
          removed from the input sinograms to simulate dead blocks. This switch
          is only allowed with cylindrical detector systems.
  --rs    randoms smoothing
  --is    number of iterations and subsets for iterative reconstruction of the
          scatter image. The default values are "3,8". If the number of
          azimuthal projections in the sinogram can not be devided by the number
          of subsets, the number of subsets will be decreased. If ML-EM is
          chosen as reconstruction algorithm, the number of subsets needs to be
          1.
           Example: --is 8,12
  --newsc use new version of scatter code. This scatter code requires an
          installed IDL runtime license und can currently only be used with the
          gantry models 1023, 1024, 1062, 1080 and 1090. The code doesn't use
          the gantry model files, instead it uses hardcoded values which may be
          different from the gantry model.
  --nosc  don't perform scatter correction
  --os2d  save unscaled scatter 2d segment and scaling factors instead
          of 3D sinogram to reduce the file size.
  --lber  specify front and back layers background energy ratio
           Example --lber 12.0
  --bc    width of boxcar-filter in pixel for axial scatter scaling factors
          (odd number). The default value is 17. This switch doesn't work in
          combination with the "--newsc" switch.
           Example: --bc 23
  --skip  the given number of planes are skipped at the beginning and end of
          each segment during the calculation of the single-slice rebinned
          sinogram, which is used during the 2d scatter estimation. The default
          is 1. This switch doesn't work in combination with the "--newsc"
          switch.
           Example: --skip 2
  --ssf   lower and upper thresholds for scatter scaling factors. The default
          is not to apply thresholds. This switch doesn't work in combination
          with the "--newsc" switch.
           Example: --ssf 0.3,1.9
                    --ssf 1.0,1.0 (switch off scatter scaling)
                    --ssf 0.0,0.0 (switch off thresholds)
  --athr  acf threshold for scatter scaling. Only the sinogram parts where the
          acf value is equal or below the threshold are used to calculate the
          scaling factors, excluded radial margin bins. The default value is 1.03,0.
           Example: --athr 1.05,4
  -r      algorithm for the calculation of a 2d sinogram from the 3d sinogram
           fore   fourier rebinning
           ssrb   single slice rebinning
           seg0   use only segment 0
          In case of "fore" three additional parameters can be used to specify
          the limits between single slice and fourier rebinning:
           alim   axis up to which single slice rebinning is used
           wlim   radial limit
           klim   angular limit
           Example: -r fore,0,2,2
  --gf    use the normalization data to fill the gaps in the emission sinogram
          by linear interpolation
  --tof   reconstruct TOF datasets by TOF algorithms. If this switch is not
          used, the time-of-flight bins in a TOF dataset are just added up. The
          TOF processing in the reconstruction can't be used in combination with
          DIFT.
  --view  store sinogram in view mode. Sinograms in debug files are always
          stored in volume mode.
  --ntilt store sinogram without intrinsic tilt of gantry
  --fl    frame-length correction
  --dc    decay correction
  --nrarc switch off all radial arc-corrections
  --naarc switch off all axial arc-corrections
  --force overwrite existing files
  -q      Gnuplot scripts are stored in the specified directory during
          reconstruction. Executing them leads to Postscript files with plots
          that support in quality control.
           Example: -q C:\my_directory
  --swap  path for swapping files. If the memory requirements are bigger than
          the amount of physical memory, parts of the data are swapped to disc.
          Performance increases if a directory on a separate disc is used for
          swapping. The default is the local directory.
           Example: --swap C:\my_directory
  --mem   set limit for memory controller. The given fraction of the physical
          memory will be used by the memory controller. Not all memory that is
          used by the program is controlled by the memory controller, therefore
          the fraction should be smaller than 1 to leave some physical memory
          for this "uncontrolled" use. The default value is 0.8. In case of
          "memory exhausted" errors a smaller value should help.
           Example: --mem 0.7   (use 70% of the physical memory)
  -d      serveral files with intermediate results are stored in the specified
          directory during reconstruction. The files that may be created are:
           acf2d_00             2d-ACF
           acf3d_00             3d-ACF
           blank2d_00           2d-blank
           emis2d_00            2d-emission
           emis3d_00            3d-emission
           emis2d_corr_00       corrected 2d-emission
           emis3d_corr_00       corrected 3d-emission
           image_00             image
           norm_00              norm
           norm3d_00            3d-norm
           psmr_00              prompts and smoothed randoms
           scatter_estim3d_00   3d scatter sinogram
           sino2d_00            2d-sinogram
           sino2d_noise_00      noise 2d-sinogram
           sino3d_00            3d-sinogram
           sino3d_noise_00      noisy 3d-sinogram
           ssr_00               SSRB(e*n*a)
           ssr_img_00           AW-OSEM(ssr_00)
           ssr_img_filt_00      smoothed version of ssr_img_00
           trans2d_00           2d-transmission
           umap_00              µ-map
          where "_00" is replaced by the frame number.
           Example: -d C:\my_directory
  -l      output logging information. The first parameter specifies the level
          of detail from 0 to 7 in its first digit. The second digit specifies
          the output destination: 0=no logging, 1=logging to file, 2=logging to
          screen, 3=logging to file and screen. The optional second parameter
          specifies a path for the log files. The default path is stored in the
          registry variable "Software\CPS\Queue\LoggingPath" (Windows) or the
          environment variable "LoggingPath" (Linux/Solaris/Mac OS X). If these
          are not defined, the local directory is used. The program creates one
          log file for each day of the month. After one month old log files are
          overwritten. The default parameter is "72".
           Examples: -l 72          (output very detailed to screen)
                     -l 73,C:\      (output very detailed to screen and file)
                     -l 31          (output medium detailed to file in default
                                     directory)
  --resrv this switch is used internally to enable communication between the
          e7-Tool and the reconstruction server.
  --app   this switch is used internally to be able to append sinograms and
          images to existing files in case of online reconstructions.
  -v      prints the date and time when the program was compiled and the used
          compiler version
  -h      print this information
  -?      print short help
    \endverbatim
 */
/*---------------------------------------------------------------------------*/
int main(int argc, char **argv)
 { Parser *cpar=NULL;
   StopWatch sw;

#if __LINUX__
        if ( getenv( "GMINI" ) == NULL )
        {
                printf( "Environment variable 'GMINI' not set\n" ) ;
                exit( EXIT_FAILURE ) ;
        }
#endif

   try
   {                                         // set handler for "out of memory"
#ifdef WIN32
     _set_new_handler(OutOfMemory);
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
     std::set_new_handler(OutOfMemory);
#endif
                                              // initialize command line parser
     cpar=new Parser("e7_sino",
                     "calculate fully corrected 2d or 3d sinogram or\n        "
                     "  calculate 3d scatter sinogram", "2002-2005",
                     "e earc enarc oe oearc os os2d osarc n np a aarc anarc "
                     "oa u ou w prj mrd span mat model scan trim nbm R k rs "
                     "is newsc nosc bc skip ssf athr r gf tof view ntilt fl "
                     "lber dc nrarc naarc force q swap mem d l resrv app "
                     "v h ?",
                     "(e$ (oe|os) [n|np [gf]]$ [[a|oa]|$[u|ou [w]]$  [prj]]$ "
                     "[mrd]$ [span]$ [mat]$ [model]$ [scan]$ [trim]$ [nbm]$ "
                     "[R]$ [k] [rs]$ [is]$ [newsc|nosc]$ [bc]$ [skip]$ [ssf]$ "
                     "[os2d]$ [athr]$ [r]$ [gf]$ [tof]$ [view]$ [ntilt]$ [fl]$ "
                     "[lber]$ [dc]$ [nrarc]$ [naarc]$ [force]$ [q]$ [swap]$ "
                     "[mem]$ [d]$ [l]$ [resrv]$ [app])$|$v|$h|$?",
                     default_iterations_scatter_osem, default_subsets_scatter,
                     reg_key+"\\"+logging_path, logging_path, "scatter image",
                     "sinogram", "acf/@-map");
     if (argc == 1) { cpar->usage(false);            // print usage information
                      delete cpar;
                      cpar=NULL;
                      GM::close();
                      return(EXIT_SUCCESS);
                    }
     if (!cpar->parse(argc, argv))                        // parse command line
      { delete cpar;
        cpar=NULL;
        GM::close();
        return(EXIT_FAILURE);
      }
     if (!validate(cpar->params()))         // validate command line parameters
      { delete cpar;
        cpar=NULL;
        GM::close();
        return(EXIT_FAILURE);
      }

                            // connect progess handler to reconstruction server
     Progress::pro()->connect(cpar->params()->recoserver,
                              cpar->params()->rs_port);
                                                          // initialize logging
     switch (cpar->params()->logcode % 10)
      { case 0:
         break;
        case 1:
         Logging::flog()->init("e7_sino", cpar->params()->logcode/10,
                               cpar->params()->logpath, Logging::LOG_FILE);
         break;
        case 2:
         Logging::flog()->init("e7_sino", cpar->params()->logcode/10,
                               cpar->params()->logpath, Logging::LOG_SCREEN);
         break;
        case 3:
         Logging::flog()->init("e7_sino", cpar->params()->logcode/10,
                               cpar->params()->logpath,
                               Logging::LOG_FILE|Logging::LOG_SCREEN);
         break;
      }
              // check for IDL runtime license (used for new scatter code only)
#ifdef SUPPORT_NEW_SCATTER_CODE
#if defined(__LINUX__) || defined(WIN32) || defined(__SOLARIS__)
     if (cpar->params()->new_scatter_code) IDL_Interface::idl();
#endif
#endif
     printHWInfo(0);                        // print information about hardware
     Logging::flog()->logCmdLine(argc, argv);             // print command line
                                                // initialize memory controller
     MemCtrl::mc()->setSwappingPath(cpar->params()->swap_path);
     MemCtrl::mc()->searchForPattern(cpar->patternKey(), 0);
     MemCtrl::mc()->setMemLimit(cpar->params()->memory_limit);
     sw.start();                                             // start stopwatch

#ifndef USE_GANTRY_MODEL
                           // Override LBER 
     if (cpar->params()->lber>0.0f)
       Logging::flog()->logMsg("Default crystalLayerBackgroundErgRatio: #1,#2",
       0)->arg(GM::gm()->crystalLayerBackgroundErgRatio(0))->
       arg(GM::gm()->crystalLayerBackgroundErgRatio(1));
     if (cpar->params()->lber>0.0f)
       {
         GantryInfo::set("crystalLayerBackgroundErgRatio[0]",
                         cpar->params()->lber);
         GantryInfo::set("crystalLayerBackgroundErgRatio[1]",
                          cpar->params()->lber);
         Logging::flog()->logMsg("Specified crystalLayerBackgroundErgRatio: #1,#2",
                          0)->arg(GM::gm()->crystalLayerBackgroundErgRatio(0))->
                          arg(GM::gm()->crystalLayerBackgroundErgRatio(1));
       }
#endif

     calculateSinogram(cpar->params());                   // calculate sinogram
                                                              // stop stopwatch
     Logging::flog()->logMsg("processing time #1", 0)->arg(secStr(sw.stop()));
     delete cpar;                                              // delete parser
     cpar=NULL;
     MemCtrl::mc()->printPattern(0);              // save memory access pattern
#ifdef SUPPORT_NEW_SCATTER_CODE
#if defined(__LINUX__) || defined(WIN32) || defined(__SOLARIS__)
     IDL_Interface::IDL_close();                                  // unload IDL
#endif
#endif
     Logging::close();                                         // close logging
     Progress::close();                               // close progress handler
     MemCtrl::deleteMemoryController();              // close memory controller
     GM::close();                                         // close gantry model
     return(EXIT_SUCCESS);
   }
   catch (const Exception r)                               // handle exceptions
    { sw.stop();
      std::cerr << "Error: " << r.errStr().c_str() << "\n" << std::endl;
                                 // send error message to reconstruction server
      Progress::pro()->sendMsg(r.errCode(), 2, r.errStr());
                                                                     // cleanup
      if (cpar != NULL) delete cpar;
      Logging::close();
      Progress::close();
      MemCtrl::deleteMemoryController();
      GM::close();
      return(r.errCode());
    }
   catch (...)
    { std::cerr << "Error: caught unknown exception" << std::endl;
                                // send error message to reconstruction server
      Progress::pro()->sendMsg(REC_UNKNOWN_EXCEPTION, 2,
                               "caught unknown exception");
                                                                     // cleanup
      if (cpar != NULL) delete cpar;
      Logging::close();
      Progress::close();
      MemCtrl::deleteMemoryController();
      GM::close();
      return(EXIT_FAILURE);
    }
 }
