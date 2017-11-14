/*! \file e7_atten.cpp
  \brief Calculate a u-map and an acf from a transmission and blank scan.
  \author Frank Kehren (frank.kehren@cpspet.com)
  \author Merence Sibomana - HRRT users community (sibomana@gmail.com)
  \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
  \date 2004/01/26 initial version
  \date 2005/01/20 added Doxygen style comments
  \date 2005/01/23 cleanup handling of unknown exceptions
  \date 2008/07/18 added --txsc for scatter correction
  \date 2008/10/23 added --txblr for TX/BL ratio
  \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

  Calculate a u-map and an acf from a transmission and blank scan.
*/

#include <iostream>
#include <string>
#include <limits>
#include <vector>
#include <iostream>
#if defined(__linux__) || defined(__SOLARIS__)
#include <new>
#endif
#ifdef WIN32
#include <new.h>
#endif
#include "convert.h"
#include "e7_common.h"
#include "exception.h"
#include "gm.h"
#include "hwinfo.h"
#include "image_conversion.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "parser.h"
#include "progress.h"
#include "registry.h"
#include "sinogram_conversion.h"
#include "stopwatch.h"
#include "str_tmpl.h"
#include "Tx_PB_3D.h"

/*- constants ---------------------------------------------------------------*/

/*! threshold for Geman-McClure smoothing model */
const float geman_mcclure_threshold=0.02f;
/*! default iterations for u-map reconstruction */
const unsigned short int iterations=10,
/*! default subsets for u-map reconstruction */
  subsets=8;

/*---------------------------------------------------------------------------*/
/*! \brief Calculate a 3d attenuation sinogram.
  \param[in] v   command line parameters

  Calculate a 3d attenuation sinogram. The calculation consists of the
  following steps (depending on the command line switches):
  - load blank sinogram
  - randoms smoothing & subtraction
  - mask dead blocks
  - untilt blank sinogram
  - axial arc-correction of blank sinogram
  - radial arc-correction of blank sinogram
  - rebin blank sinogram
  - load transmission sinogram
  - randoms smoothing & subtraction
  - mask dead blocks
  - untilt transmission sinogram
  - axial arc-correction of transmission sinogram
  - radial arc-correction of transmission sinogram
  - rebin transmission sinogram
  - replace border planes in sinograms by neighbours
  - calculate u-map
  - resize u-map
  - save u-map
  - calculate acf
  - radial un-arc correct acf
  - tilt acf
  - save acf
*/
/*---------------------------------------------------------------------------*/
void calculate3DAttenuation(Parser::tparam * const v)
{ float *bla_cpy=NULL, *acf3d=NULL;
  SinogramConversion *blank_sino=NULL, *tra_sino=NULL, *acf_sino=NULL;
  ImageConversion *umap_image=NULL;

  try
    { unsigned short int num_log_cpus, subsets, number_of_matrices, i;
      std::string str, mu;
      std::vector <unsigned short int> matrices;
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
      char c[2];

      c[1]=0;
      c[0]=(char)181;
      mu=std::string(c);
#endif
#ifdef WIN32
      mu='u';
#endif
      num_log_cpus=logicalCPUs();
      // load 2d blank sinogram
      Logging::flog()->logMsg("loading the 2d blank sinogram #1", 0)->
        arg(v->blank_filename);
      if (v->alpha == 0.0f) v->algorithm=ALGO::OSEM_UW_Algo;
      else v->algorithm=ALGO::MAPTR_Algo;
     
      /*
        PMB
      */
#ifdef __linux__
      float hrrt_tx_scatter_a, hrrt_tx_scatter_b;
#endif
      if (v->txscatter[0] != 0.0f) hrrt_tx_scatter_a = v->txscatter[0];
      if (v->txscatter[1] != 0.0f) hrrt_tx_scatter_b = v->txscatter[1];
      if (v->txblr != 0.0f) hrrt_blank_factor = v->txblr;

      // init geometry (for flat files)
      if (GM::gm()->initialized())
        { unsigned short int rs, ts;
          float bsr;

          if (v->mrd == 0) v->mrd=GM::gm()->mrd();
          if (v->span == 0) v->span=GM::gm()->span();
          rs=GM::gm()->RhoSamples();
          ts=GM::gm()->ThetaSamples();
          bsr=GM::gm()->BinSizeRho();
          if (v->bRhoSamples > 0)
            { rs=v->bRhoSamples;
              bsr*=(float)rs/(float)v->bRhoSamples;
            }
          if (v->bThetaSamples > 0) ts=v->bThetaSamples;
          blank_sino=new SinogramConversion(rs, bsr, ts, v->span, v->mrd,
                                            GM::gm()->planeSeparation(),
                                            GM::gm()->rings(),
                                            GM::gm()->intrinsicTilt(), true,
                                            v->si.lld, v->si.uld,
                                            v->si.start_time,
                                            v->si.frame_duration,
                                            v->si.gate_duration, v->si.halflife,
                                            v->si.bedpos, 0, true);
        }
      else { unsigned short int rs=0, ts=0;

        if (v->bRhoSamples > 0) rs=v->bRhoSamples;
        if (v->bThetaSamples > 0) ts=v->bThetaSamples;
        blank_sino=new SinogramConversion(rs, 0.0f, ts, v->span, v->mrd,
                                          0.0f, 0, 0.0f, true, v->si.lld,
                                          v->si.uld, v->si.start_time,
                                          v->si.frame_duration,
                                          v->si.gate_duration,
                                          v->si.halflife, v->si.bedpos, 0,
                                          true);
      }
      // load data
      blank_sino->load(v->blank_filename, true, v->blank_arc_corrected, 0,
                       "bla", 1);
      if (v->blank_arc_corrected_overwrite)
        if (v->blank_arc_corrected)
          blank_sino->correctionsApplied(blank_sino->correctionsApplied() |
                                         CORR_Radial_Arc_correction |
                                         CORR_Axial_Arc_correction);
        else blank_sino->correctionsApplied(blank_sino->correctionsApplied() ^
                                            (CORR_Axial_Arc_correction |
                                             CORR_Axial_Arc_correction));
      // calculate trues
      blank_sino->calculateTrues(v->randoms_smoothing, 1, num_log_cpus);
      blank_sino->maskOutBlocks(v->kill_blocks, 0);   // kill blocks in sinogram
      blank_sino->untilt(true, 0);                            // untilt sinogram
      // axial arc-correction
      blank_sino->axialArcCorrection(v->no_axial_arc, true, 0, num_log_cpus);
      // radial arc-correction
      blank_sino->radialArcCorrection(v->no_radial_arc, true, 0, num_log_cpus);
      // rebin sinogram
      blank_sino->resampleRT(blank_sino->RhoSamples()/v->rebin_r,
                             blank_sino->ThetaSamples()/v->rebin_t, 0,
                             num_log_cpus);
      // save debug file
      if (v->debug) blank_sino->saveFlat(v->debug_path+"/blank2d", false, 1);
      // check number of subsets for OSEM
      subsets=v->subsets;
      if ((blank_sino->ThetaSamples() % subsets) > 0)
        { unsigned short int os;

          os=subsets;
          while ((blank_sino->ThetaSamples() % subsets) > 0) subsets--;
          Logging::flog()->logMsg("*** number of subsets decreased to #1 (#2 % "
                                  "#3 > 0)", 1)->arg(subsets)->
            arg(blank_sino->ThetaSamples())->arg(os);
        }
      // get gantry model information
      if (v->mrd == 0) v->mrd=GM::gm()->mrd();
      if (v->span == 0) v->span=GM::gm()->span();
      if (v->quality_control)               // remove GNUplot script if existent
        { std::string fname;

          fname=v->quality_path+"/umap_qc.plt";
          unlink(fname.c_str());
        }
      // init geometry (for flat files)
      if (GM::gm()->initialized())
        { unsigned short int rs, ts;
          float bsr;

          if (v->mrd == 0) v->mrd=GM::gm()->mrd();
          if (v->span == 0) v->span=GM::gm()->span();
          rs=GM::gm()->RhoSamples();
          ts=GM::gm()->ThetaSamples();
          bsr=GM::gm()->BinSizeRho();
          if (v->tRhoSamples > 0)
            { rs=v->tRhoSamples;
              bsr*=(float)rs/(float)v->tRhoSamples;
            }
          if (v->tThetaSamples > 0) ts=v->tThetaSamples;
          tra_sino=new SinogramConversion(rs, bsr, ts, v->span, v->mrd,
                                          GM::gm()->planeSeparation(),
                                          GM::gm()->rings(),
                                          GM::gm()->intrinsicTilt(), true,
                                          v->si.lld, v->si.uld,
                                          v->si.start_time,
                                          v->si.frame_duration,
                                          v->si.gate_duration, v->si.halflife,
                                          v->si.bedpos, 0, true);
        }
      else { unsigned short int rs=0, ts=0;

        if (v->tRhoSamples > 0) rs=v->tRhoSamples;
        if (v->tThetaSamples > 0) ts=v->tThetaSamples;
        tra_sino=new SinogramConversion(rs, 0.0f, ts, v->span, v->mrd,
                                        0.0f, 0, 0.0f, true, v->si.lld,
                                        v->si.uld, v->si.start_time,
                                        v->si.frame_duration,
                                        v->si.gate_duration,
                                        v->si.halflife, v->si.bedpos, 0,
                                        true);
      }
      // get number of transmission frames
      number_of_matrices=numberOfMatrices(v->tra_filename);
      matrices.clear();
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
      if (!v->oacf_filename.empty())
        if (matrices.size() == 1)
          Logging::flog()->logMsg("calculate acf (span=#1, mrd=#2) and #3-map for"
                                  " 1 matrix", 0)->arg(v->span)->arg(v->mrd)->
            arg(mu);
        else Logging::flog()->logMsg("calculate acf (span=#1, mrd=#2) and "
                                     "#3-map for #4 matrices", 0)->
               arg(v->span)->arg(v->mrd)->arg(mu)->arg(matrices.size());
      else if (matrices.size() == 1)
        Logging::flog()->logMsg("calculate #1-map for 1 matrix", 0)->
          arg(mu);
      else Logging::flog()->logMsg("calculate #1-map for #2 matrices",
                                   0)->arg(mu)->arg(matrices.size());
      for (unsigned short int num=0; num < matrices.size(); num++)
        { std::string nr;
          unsigned long int plane_size;
          float *tra2d, *bp, fov_diameter;

          nr=toStringZero(matrices[num], 2);
          // load 2d transmission sinogram
          Logging::flog()->logMsg("loading the 2d transmission sinogram "
                                  "(matrix #1)", 1)->arg(matrices[num]);
          // load data
          tra_sino->load(v->tra_filename, true, v->tra_arc_corrected,
                         matrices[num], "tra", 2);
          if (v->tra_arc_corrected_overwrite)
            if (v->tra_arc_corrected)
              tra_sino->correctionsApplied(tra_sino->correctionsApplied() |
                                           CORR_Radial_Arc_correction |
                                           CORR_Axial_Arc_correction);
            else tra_sino->correctionsApplied(tra_sino->correctionsApplied() ^
                                              (CORR_Axial_Arc_correction |
                                               CORR_Axial_Arc_correction));
          // calculate trues
          tra_sino->calculateTrues(v->randoms_smoothing, 1, num_log_cpus);
          // kill blocks in sinogram
          tra_sino->maskOutBlocks(v->kill_blocks, 1);
          tra_sino->untilt(true, 1);                           // untilt sinogram
          // axial arc-correction
          tra_sino->axialArcCorrection(v->no_axial_arc, true, 1, num_log_cpus);
          // radial arc-correction
          tra_sino->radialArcCorrection(v->no_radial_arc, true, 1, num_log_cpus);
          // rebin sinogram
          tra_sino->resampleRT(tra_sino->RhoSamples()/v->rebin_r,
                               tra_sino->ThetaSamples()/v->rebin_t, 1,
                               num_log_cpus);
          // save debug file
          if (v->debug)
            tra_sino->saveFlat(v->debug_path+"/trans2d_"+nr, false, 2);
          // calculate reconstructed attenuation (u-map)
          // create backup of blank, since it is modified during reconstruction
          bla_cpy=new float[blank_sino->size()];
          bp=MemCtrl::mc()->getFloat(blank_sino->index(0, 0), 1);
          memcpy(bla_cpy, bp, blank_sino->size()*sizeof(float));
          // replace border planes by neighbours
          plane_size=(unsigned long int)blank_sino->RhoSamples()*
            (unsigned long int)blank_sino->ThetaSamples();
          memcpy(bp, bp+plane_size, plane_size*sizeof(float));
          memcpy(bp+(blank_sino->axesSlices()-1)*plane_size,
                 bp+(blank_sino->axesSlices()-2)*plane_size,
                 plane_size*sizeof(float));
          MemCtrl::mc()->put(blank_sino->index(0, 0));

          tra2d=MemCtrl::mc()->getFloat(tra_sino->index(0, 0), 1);
          plane_size=(unsigned long int)tra_sino->RhoSamples()*
            (unsigned long int)tra_sino->ThetaSamples();
          memcpy(tra2d, tra2d+plane_size, plane_size*sizeof(float));
          memcpy(tra2d+(tra_sino->axesSlices()-1)*plane_size,
                 tra2d+(tra_sino->axesSlices()-2)*plane_size,
                 plane_size*sizeof(float));
          MemCtrl::mc()->put(tra_sino->index(0, 0));
          // calculate u-map
          umap_image=CONVERT::bt2umap(blank_sino, tra_sino, v->rebin_r,
                                      v->rebin_t, blank_sino->RhoSamples(),
                                      blank_sino->axesSlices(), v->iterations,
                                      v->subsets, v->alpha, v->beta, v->beta_ip,
                                      v->iterations_ip,
                                      v->gaussian_smoothing_model,
                                      geman_mcclure_threshold,
                                      v->number_of_segments, v->mu_mean,
                                      v->mu_std, v->gauss_intersect,
                                      v->umap_scalefactor, v->autoscale_umap,
                                      v->asv, v->gauss_fwhm_xy, v->gauss_fwhm_z,
                                      v->uzoom_xy_factor, v->uzoom_z_factor,
                                      v->ucut, v->quality_control,
                                      v->quality_path, num, v->blank_db_path,
                                      v->blank_db_frames, &fov_diameter, 1,
                                      num_log_cpus);
          if (v->iwidth > 0)
            umap_image->resampleXY(v->iwidth,                      // resize u-map
                                   umap_image->DeltaXY()*
                                   (float)umap_image->XYSamples()/
                                   (float)v->iwidth, true, 1);
          if (v->debug)                                       // save debug files
            umap_image->saveFlat(v->debug_path+"/umap_"+nr, 2);
          if (!v->oumap_filename.empty())                           // save u-map
            { Logging::flog()->logMsg("save #1-map matrix #2 in file #3",
                                      1)->arg(mu)->arg(matrices[num])->
                arg(v->oumap_filename);
              if (num == 0)
                umap_image->copyFileHeader(tra_sino->ECAT7mainHeader(),
                                           tra_sino->InterfileHdr());
              umap_image->save(v->oumap_filename, v->tra_filename,
                               blank_sino->RhoSamples(),
                               blank_sino->ThetaSamples(),
                               tra_sino->matrixFrame(), tra_sino->matrixPlane(),
                               tra_sino->matrixGate(), tra_sino->matrixData(),
                               tra_sino->matrixBed(), tra_sino->intrinsicTilt(),
                               v, fov_diameter,
                               ImageConversion::SCATTER_NoScatter,
                               tra_sino->decayFactor(),
                               tra_sino->scatterFraction(), tra_sino->ECF(),
                               std::vector <float>(), false, false,false,
                               tra_sino->scanType(), 0, 2);
            }
          // restore blank scan
          memcpy(MemCtrl::mc()->getFloat(blank_sino->index(0, 0), 1), bla_cpy,
                 blank_sino->size()*sizeof(float));
          MemCtrl::mc()->put(blank_sino->index(0, 0));
          delete[] bla_cpy;
          bla_cpy=NULL;
          // calculate 3d acf
          if (!v->oacf_filename.empty())
            { Logging::flog()->logMsg("calculate 3d acf", 1);
              if (v->iwidth > 0)                                   // resize u-map
                umap_image->resampleXY(blank_sino->RhoSamples(),
                                       umap_image->DeltaXY()*
                                       (float)umap_image->XYSamples()/
                                       (float)blank_sino->RhoSamples(),  true, 2);
              acf_sino=CONVERT::image2sinogram(umap_image, v, num, "acf", 2,
                                               num_log_cpus);
              if (!v->oacf_arc_corrected)   // un-arc correction of 3d attenuation
                acf_sino->radialArcCorrection(v->no_radial_arc, false, 1,
                                              num_log_cpus);
              if (!v->no_tilt) acf_sino->untilt(false, 1);        // tilt sinogram
              acf_sino->cutBelow(1.0f, 1.0f, 1);           // cut-off small values
              // save debug file
              if (v->debug && (acf_sino->axes() > 1))
                acf_sino->saveFlat(v->debug_path+"/acf3d_"+nr, false, 2);
              // save 3d sinogram
              if (acf_sino->axes() > 1)
                Logging::flog()->logMsg("save 3d acf matrix #1 in file #2", 1)->
                  arg(matrices[num])->arg(v->oacf_filename);
              else Logging::flog()->logMsg("save 2d acf matrix #1 in file #2",
                                           1)->arg(matrices[num])->
                     arg(v->oacf_filename);
              if (num == 0) acf_sino->copyFileHeader(tra_sino->ECAT7mainHeader(),
                                                     tra_sino->InterfileHdr());
              acf_sino->save(v->oacf_filename, v->tra_filename,
                             tra_sino->matrixFrame(), tra_sino->matrixPlane(),
                             tra_sino->matrixGate(), tra_sino->matrixData(),
                             tra_sino->matrixBed(), v->view_mode,
                             tra_sino->bedPos(), num, 2);
              delete acf_sino;
              acf_sino=NULL;
            }
          delete umap_image;
          umap_image=NULL;
        }
      delete tra_sino;
      tra_sino=NULL;
      GM::gm()->printUsedGMvalues();
      delete blank_sino;
      blank_sino=NULL;
    }
  catch (...)
    { if (bla_cpy != NULL) delete[] bla_cpy;
      if (blank_sino != NULL) delete blank_sino;
      if (tra_sino != NULL) delete tra_sino;
      if (acf_sino != NULL) delete acf_sino;
      if (umap_image != NULL) delete umap_image;
      if (acf3d != NULL) delete[] acf3d;
      throw;
    }
}

/*---------------------------------------------------------------------------*/
/*! \brief Check semantics of command line parameters.
  \param[in] v   command line parameters

  Check semantics of command line parameters.
*/
/*---------------------------------------------------------------------------*/
bool validate(Parser::tparam * const v)
{ std::string mu;
  bool ret=true;
  {
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
    char c[2];

    c[1]=0;
    c[0]=(char)181;
    mu=std::string(c);
#endif
#ifdef WIN32
    mu='u';
#endif
  }
  if (v->segmentation_params_set && (v->umap_reco_model == 1))
    { std::cerr << "-p: segmentation only with reconstruction methods '20',"
        "'21','30','31','40' and '41'.\n";
      ret=false;
    }
  if (v->segmentation_params_set && (v->umap_reco_model > 1) &&
      (v->beta_ip == 0.0f))
    { std::cerr << "-p: segmentation only with intensity prior\n";
      ret=false;
    }
  if (v->blank_filename.empty())
    { std::cerr << "The name of the blank input file needs to be specified.\n";
      ret=false;
    }
  if (v->tra_filename.empty())
    { std::cerr << "The name of the transmission input file needs to be "
        "specified.\n";
      ret=false;
    }
  if (v->oacf_filename.empty() && v->oumap_filename.empty())
    { std::cerr << "An acf or " << mu << "-map output file needs to be "
        "specified.\n";
      ret=false;
    }
  return(ret);
}

/*---------------------------------------------------------------------------*/
/*! \brief main function of program to calculate a u-map and an acf from a
  transmission and blank scan.
  \param[in] argc   number of arguments
  \param[in] argv   command line switches
  \return 0 success
  \return 1 unknown failure
  \return error code

  main function of program to calculate a u-map and an acf from a
  transmission and blank scan. Possible command line switches are:
  \verbatim
  e7_atten - calculate a 2d or 3d acf from transmission and blank

  Usage:

  e7_atten (-t transmission[,r,t] -b blank[,r,t] [--ucut] [--oa acf]|
  [--ou �-map [-w width]] [(--ualgo 1)|
  (--ualgo 20|21|30|31|(40|41 alpha,beta,beta_ip,iter_ip)
  [-p segments,mean,std,mean,std,...,b,b,...]) [--gxy fwhm]
  [--gz fwhm]] [--asv �-value] [--aus] [--uf factor]
  [--uzoom xy_zoom,z_zoom]
  [--model model|a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,
  z1,z2,z3,z4,z1,z2,z3,z4,...] [--is iterations,subsets]
  [-R r,t] [-k a,b,...,n] [--rs] [--dttx path,frames] [--mrd mrd]
  [--span span] [--mat a|a-b|a,b,...,n] [--prj ifore|issrb|fwdpj]
  [--view] [--ntilt] [--force] [-q path] [--swap path] [--mem fraction]
  [-d path] [-l a[,path]] [--resrv port])|-v|-h|-?

  -t      name of the transmission input file. Supported formats are ECAT7,
  Interfile and flat (IEEE-float or 16 bit signed int, little endian).
  In case of a flat-file, the geometry of the gantry needs to be
  specified by the "--model" switch. If the radial or azimuthal
  dimension of the sinogram in a flat file is different from the
  standard values of the gantry, both values can be specified after the
  filename. Transmission files are assumed to be not arc-corrected if
  this information is not available from the file header. The switches
  "--tnarc" or "--tarc" can be used instead of "-t" to specify/overwrite
  this behaviour for cylindrical detector systems.
  Examples: -t file.s          (ECAT7)
  -t file.mhdr       (Interfile)
  -t file.dat        (flat, not arc-corrected)
  -t file.dat,144,72 (flat with dimensions, not arc-correc.)
  --tnarc file.s     (ECAT7, not arc-corrected)
  --tarc  see "-t"
  --tnarc see "-t"
  -b      name of the blank input file. Supported formats are ECAT7, Interfile
  and flat (IEEE-float or 16 bit signed int, little endian). In case of
  a flat-file, the geometry of the gantry needs to be specified by the
  "--model" switch. If the radial or azimuthal dimension of the sinogram
  in a flat file is different from the standard values of the gantry,
  both values can be specified after the filename. Blank files are
  assumed to be not arc-corrected if this information is not available
  from the file header. The switches "--bnarc" or "--barc" can be used
  instead of "-b" to specify/overwrite this behaviour for cylindrical
  detector systems.
  Examples: -b file.s          (ECAT7)
  -b file.mhdr       (Interfile)
  -b file.dat        (flat, not arc-corrected)
  -b file.dat,144,72 (flat with dimensions, not arc-correc.)
  --barc file.s      (ECAT7, arc-corrected)
  --barc  see "-b"
  --bnarc see "-b"
  --ucut  cut-off first and last plane of blank and transmission. If the switch
  "--uzoom" is used, the cutting is done after the axial resampling.
  --oa    name of the acf output file. The file format depends on the format of
  the input files (ECAT7, Interfile or IEEE-float flat, little endian).
  acf files are stored without arc-correction. The switch "--oaarc" can
  be used instead of "--oa" to overwrite this behaviour for cylindrical
  detector systems.
  Examples: --oa file.a      (ECAT7, not arc-corrected)
  --oa file.mhdr   (Interfile, not arc-corrected)
  --oa file.dat    (flat, not arc-corrected)
  --oaarc file.a   (ECAT7, arc-corrected)
  --oaarc see "--oa"
  --ou    name of �-map output file. The file format depends on the format of
  the input files (ECAT7, Interfile or IEEE-float flat, little endian).
  �-map files are stored with a width and height equal to the projection
  width of the gantry. The additional switch "-w" can be used to
  overwrite this behaviour.
  Examples: --ou file.v          (ECAT7)
  --ou file.mhdr       (Interfile)
  --ou file.dat        (flat)
  --ou file.dat -w 128 (flat with width/height)
  -w      width/height of the image. The default value is the width of the
  projection.
  Example: -w 128
  --ualgo method for �-map reconstruction. The default is "40,1.5,3,0,0".
  1   unweighted OSEM
  20   Gradient Ascending with smoothness constraints and gaussian
  smoothing model (== --ualgo 40,1.5,0.00025,0,0)
  21   Gradient Ascending with smoothness constraints and Geman-McClure
  smoothing model (== --ualgo 41,1.5,0.00025,0,0)
  30   Gradient Ascending full model and gaussian smoothing model
  (== --ualgo 40,1.5,1,0.05,5)
  31   Gradient Ascending full model and Geman-McClure smoothing model
  (== --ualgo 41,1.5,1,0.05,5)
  40   user defined model with gaussian smoothing model
  alpha    relaxation parameter
  beta        regularization parameter for smoothing prior
  beta_ip     regularization parameter for intensity prior
  iter_ip  first iteration for intensity prior
  41   user defined model with Gean-McClure smoothing model
  alpha    relaxation parameter
  beta        regularization parameter for smoothing prior
  beta_ip     regularization parameter for intensity prior
  iter_ip  first iteration for intensity prior
  Example: --ualgo 40,1.5,3,0,0
  --gxy   gaussian smoothing of �-map in x/y-direction. The filter width is
  specified in mm FWHM.
  Example: --gxy 3.2
  --gz    gaussian smoothing of �-map in z-direction. The filter width is
  specified in mm FWHM.
  Example: --gz 6.78
  -p      parameters for segmentation of �-map if reconstructed with intensity
  prior. The default is
  "4,0,0.002,0.03,0.06,0.05,0.06,0.096,0.02,0.015,0.04,0.065".
  segments  number of segments
  mean      mean value for gaussian prior
  std       standard deviation for gaussian prior
  b         boundary between segments
  If the switch "--uf" is used at the same time, the mean values and
  segment boundaries are given based on the scaled �-map.
  Example: -p 4,0,0.002,0.03,0.06,0.05,0.06,0.096,0.02,0.015,0.04,0.065
  (use four gaussian priors with the mean and std values
  (0,0.002), (0.03,0.06), (0.05,0.06), (0.096,0.02) and the
  three segment boundaries 0.015, 0.04 and 0.065)
  --asv   �-value for auto-scaling. The default behaviour of the auto-scaling
  is to find the water peak in the histogram and scale this towards the
  �-value of water (0.096 1/cm). With this switch the scaling can be
  done towards a different �-value. The default value is 0.096. This
  switch can not be used in combination with the "--aus" switch.
  Example: --asv 1.02
  --aus   switch the auto-scaling of the �-map off. The default behaviour of
  the segmentation algorithm is to find the water-peak in the u-map and
  scale the segments based on this. The switch "--aus" can be used to
  switch this automatic scaling off.
  --uf    This scaling factor is applied to the mean values and standard
  deviations of the gaussian prior and to the boundary values of the
  segments. E.g. using the switches
  -p 4,0,0.002,0.03,0.06,0.05,0.06,0.096,0.02,0.015,0.04,0.065 --uf 0.9
  is equivalent to using
  -p 4,0,0.00222,0.0333,0.0667,0.0556,0.0667,0.1067,0.0222,0.0167,
  0.0444,0.0722.
  Example: --uf 0.95
  --uzoom zoom factors for the �-map. These zoom factors are applied during the
  calculation of the �-map from blank and transmission. Allowed values
  are 1,2 and 4 for the x/y-direction and 1,2,3 and 4 for the
  z-direction. The rebinning that can be selected with the "-R" switch
  would instead be applied on blank and transmission data - before the
  reconstruction.
  Example: --uzoom 2,3 (reconstructs an 80x80x80 �-map if the blank and
  transmission sinograms are 160 bins wide and
  have 239 planes)
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
  --is    number of iterations and subsets for iterative reconstruction of the
  �-map. The default values are "10,8". If the number of azimuthal
  projections in the acf can not be devided by the number of subsets,
  the number of subsets will be decreased. If ML-EM is chosen as
  reconstruction algorithm, the number of subsets needs to be 1.
  Example: --is 8,12
  -R      rebin input sinograms by radial and azimuthal factor. Allowed values
  are 1, 2 and 4.
  Example: -R 2,2
  -k      counts measured by the detector blocks with the given numbers are
  removed from the input sinograms to simulate dead blocks. This switch
  is only allowed with cylindrical detector systems.
  --rs    randoms smoothing
  --dttx  transmission deadtime correction
  path     path for blank database
  frames   number of frames in blank database
  Example: --dttx C:\my_directory,9
  --mrd   maximum ring difference for 3d acf. The default value is specific for
  a given gantry.
  Example: --mrd 17
  --span  span for 3d acf. The default value is specific for a given gantry.
  Example: --span 7
  --mat   number(s) of matrix/matrices to process. The program can process a
  single matrix, a range of matrices or a set of matrices. Matrix
  numbers have to be specified in increasing order. The default is to
  process all matrices.
  Examples: --mat 3     (process only the 4th(!) matrix of a file)
  --mat 0-2   (process the the first 3 matrices of a file)
  --mat 1,3,5 (process the 2nd, 4th and 6th matrix of a file)
  --prj   algorithm for the calculation of the 3d acf from the 2d acf/�-map.
  The default algorithm is 2d-forward projection followed by inverse
  fourier rebinning. This switch is also used in the calculation of a 3d
  sinogram from a 2d sinogram. In this case are only the parameters
  "issrb" and "ifore" allowed.
  none    don't calculate 3d acf
  issrb   inverse single slice rebinning from 2d sinogram
  ifore   inverse fourier rebinning from 2d sinogram
  fwdpj   3d-forward projection from image
  Example: --prj issrb
  --view  store acf in view mode. Sinograms in debug files are always stored in
  volume mode.
  --ntilt store acf without intrinsic tilt of gantry
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
  umap_00              �-map
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
  -v      prints the date and time when the program was compiled and the used
  compiler version
  -h      print this information
  -?      print short help
  \endverbatim
*/
/*---------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
  Parser *cpar = NULL;
  StopWatch sw;

#if __linux__
  if ( getenv( "GMINI" ) == NULL )
    {
      printf( "Environment variable 'GMINI' not set\n" ) ;
      exit( EXIT_FAILURE ) ;
    }
#endif

  try
   
    {                                     // set new handler for "out of memory"
#ifdef WIN32
      _set_new_handler(OutOfMemory);
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
      std::set_new_handler(OutOfMemory);
#endif
      // initialize parser
      cpar=new Parser("e7_atten",
                      "calculate a 2d or 3d acf from transmission and blank",
                      "2002-2005",
                      "t tarc tnarc b barc bnarc ucut oa oaarc ou w ualgo gxy "
                      "gz p asv aus uf uzoom model is R k rs dttx mrd span mat "
                      "prj view ntilt force q swap mem d l resrv txsc txblr v h ?",
                      "(t$ b$ [ucut]$ [oa]|$ [ou [w]]$ [(ualgo) [gxy]$  [gz]$]$"
                      " [asv]$ [aus]$ [uf]$ [uzoom]$ [model]$ [is]$ [R]$ [k]$ "
                      "[rs]$ [dttx]$ [mrd]$ [span]$ [mat]$ [prj]$ [view]$ "
                      "[ntilt]$ [force]$ [q]$ [swap]$ [mem]$ [d]$ [l]$ [resrv]$ "
                      "[txsc] [txblr])"
                      "|$v|$h|$?",
                      iterations, subsets, reg_key+"\\"+logging_path,
                      logging_path, "@-map", "acf", "acf/@-map");
      if (argc == 1) {
        cpar->usage(false);            // print usage information
        delete cpar;
        cpar=NULL;
        GM::close();
        return(EXIT_SUCCESS);
      }
      if (!cpar->parse(argc, argv)) {
        // parse command line arguments
        delete cpar;
        cpar=NULL;
        GM::close();
        return(-1);
      }
      if (!validate(cpar->params())) {
        // validate command line arguments
          delete cpar;
          cpar=NULL;
          GM::close();
          return(-1);
        }
      // initialize progress reporting mechanism
      Progress::pro()->connect(cpar->params()->recoserver, cpar->params()->rs_port);
      // initialize logging mechanism
      switch (cpar->params()->logcode % 10) {
        case 0:
            break;
        case 1:
          Logging::flog()->init("e7_atten", cpar->params()->logcode/10,
                                cpar->params()->logpath, Logging::LOG_FILE);
          break;
        case 2:
          Logging::flog()->init("e7_atten", cpar->params()->logcode/10,
                                cpar->params()->logpath, Logging::LOG_SCREEN);
          break;
        case 3:
          Logging::flog()->init("e7_atten", cpar->params()->logcode/10,
                                cpar->params()->logpath,
                                Logging::LOG_FILE|Logging::LOG_SCREEN);
          break;
        }
      printHWInfo(0);                              // print hardware information
      Logging::flog()->logCmdLine(argc, argv); // print command line information
      // initialize memory controller
      MemCtrl::mc()->setSwappingPath(cpar->params()->swap_path);
      std::string patternKey = cpar->patternKey();
      // std::cout << "cpar->patternKey = '" << patternKey << "'" << std::endl;
      MemCtrl::mc()->searchForPattern(patternKey, 0);
      MemCtrl::mc()->setMemLimit(cpar->params()->memory_limit);
      sw.start();                                             // start stopwatch
      calculate3DAttenuation(cpar->params());         // calculate acf and u-map
      // stop stopwatch
      Logging::flog()->logMsg("processing time #1", 0)->arg(secStr(sw.stop()));
      // cleanup
      delete cpar;
      cpar=NULL;
      MemCtrl::mc()->printPattern(0);
      Logging::close();
      Progress::close();
      MemCtrl::deleteMemoryController();
      GM::close();
      return(EXIT_SUCCESS);
    }
  catch (const Exception r)                               // handle exceptions
    { 
      std::cerr << "Error: " << r.errStr().c_str() << "\n" << std::endl;
      // send error message to reconstruction server
      sw.stop();
      Progress::pro()->sendMsg(r.errCode(), 1, r.errStr());
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
      Progress::pro()->sendMsg(REC_UNKNOWN_EXCEPTION, 1,
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
