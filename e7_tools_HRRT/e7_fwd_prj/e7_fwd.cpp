/*! \file e7_fwd.cpp
    \brief Calculate a 2d or 3d sinogram from an image or u-map.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/02/06 initial version
    \date 2005/01/20 added Doxygen style comments
    \date 2005/01/23 cleanup handling of unknown exceptions

    Calculate a 2d or 3d sinogram from an image or u-map.
 */

#include <iostream>
#include <string>
#include <limits>
#include <vector>
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
#include "sinogram_conversion.h"
#include "stopwatch.h"

/*---------------------------------------------------------------------------*/
/*! \brief Calculate 2d or 3d sinograms from image or u-map.
    \param[in] v   command line parameters

    Calculate 2d or 3d sinograms from image or u-map. The calculation consists
    of the following steps (depending on the command line switches):
    - load u-map or image
    - forward project image into sinogram
    - radial un-arc correct sinogram
    - tilt sinogram
    - add poisson noise
    - save sinogram
 */
/*---------------------------------------------------------------------------*/
void calculate3DSinogram(Parser::tparam * const v)
 { ImageConversion *image=NULL;
   SinogramConversion *sino=NULL;

   try
   { unsigned short int num_log_cpus, number_of_matrices, i;
     std::string img_filename=std::string(), sino_filename, str;
     std::vector <unsigned short int> matrices;

     num_log_cpus=logicalCPUs();
     if (!v->image_filename.empty()) img_filename=v->image_filename;
      else if (!v->umap_filename.empty()) img_filename=v->umap_filename;
     if (!v->oemission_filename.empty()) sino_filename=v->oemission_filename;
      else sino_filename=v->oacf_filename;
     if (v->acf_filename.empty())
      number_of_matrices=numberOfMatrices(img_filename);
      else number_of_matrices=numberOfMatrices(v->acf_filename);
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
     if (v->image_filename.empty()) str="acf";
      else str="sinogram";
     if (matrices.size() == 1)
      Logging::flog()->logMsg("calculate #1 for 1 matrix", 0)->arg(str);
      else Logging::flog()->logMsg("calculate #1 for #2 matrices", 0)->
            arg(str)->arg(matrices.size());
     for (unsigned short int num=0; num < matrices.size(); num++)
      { Progress::pro()->sendMsg(COM_EVENT::PROCESSING, 1,
                                 "calculate 3D-#1 (frame #2)")->arg(str)->
         arg(num);
        if (v->mrd == 0) v->mrd=GM::gm()->mrd();
        if (v->span == 0) v->span=GM::gm()->span();
        Logging::flog()->logMsg("span=#1", 1)->arg(v->span);
        Logging::flog()->logMsg("maximum ring difference=#1", 1)->arg(v->mrd);
        if (!v->umap_filename.empty() || !v->acf_filename.empty())
         { if (v->iwidth == 0) v->iwidth=GM::gm()->RhoSamples();
                                                // load u-map and calculate acf
           CONVERT::get_umap_acf(v, &image, &sino, !v->oacf_filename.empty(),
                                 !v->oumap_filename.empty(), v->span, v->mrd,
                                 num, num, num, 1, num_log_cpus);
           if (!v->umap_filename.empty())
            if (!v->oacf_filename.empty() && v->acf_filename.empty() &&
               (numberOfMatrices(v->umap_filename) == 1))
             { v->acf_filename=v->oacf_filename;
               v->oacf_filename=std::string();
             }
         }
         else { std::string nr;

                nr=toStringZero(matrices[num], 2);
                if (v->iwidth == 0) v->iwidth=GM::gm()->RhoSamples();
                image=new ImageConversion(v->iwidth,
                                          GM::gm()->BinSizeRho()*
                                          (float)GM::gm()->RhoSamples()/
                                           v->iwidth,
                                          GM::gm()->rings()*2-1,
                                          GM::gm()->planeSeparation(), 0,
                                          v->si.lld, v->si.uld,
                                          v->si.start_time,
                                          v->si.frame_duration,
                                          v->si.gate_duration,
                                          v->si.halflife, v->si.bedpos);
                                                                  // load image
                image->load(img_filename, v->image_filename.empty(),
                            matrices[num], "img", 1);
                                                 // convert image into sinogram
                sino=CONVERT::image2sinogram(image, v, num, "sino", 2,
                                             num_log_cpus);
                                         // un-arc correction of 3d attenuation
                if ((!v->oacf_arc_corrected && !v->oacf_filename.empty()) ||
                    (!v->oemission_arc_corrected &&
                     !v->oemission_filename.empty()))
                 sino->radialArcCorrection(v->no_radial_arc, false, 1,
                                           num_log_cpus);
                if (!v->no_tilt) sino->untilt(false, 1);       // tilt sinogram
                if (v->debug && (sino->axes() > 1))
                 sino->saveFlat(v->debug_path+"/sino3d_"+nr, false, 2);
                if (v->trues > 0.0)                      // add poisson noise ?
                 { sino->addPoissonNoise(v->trues, v->randoms, 2);
                   if (v->debug)
                    if (sino->axes() > 1)
                     sino->saveFlat(v->debug_path+"/sino3d_noise_"+nr, false,
                                    2);
                    else sino->saveFlat(v->debug_path+"/sino2d_noise_"+nr,
                                        false, 2);
                 }
                                                               // save sinogram
                if (sino->axes() > 1)
                 Logging::flog()->logMsg("save 3d sinogram matrix #1 in file "
                                         "#2", 1)->arg(matrices[num])->
                  arg(sino_filename);
                 else Logging::flog()->logMsg("save 2d sinogram matrix #1 in "
                                              "file #2", 1)->
                       arg(matrices[num])->arg(sino_filename);
                if (num == 0)
                 sino->copyFileHeader(image->ECAT7mainHeader(),
                                      image->InterfileHdr());
                sino->save(sino_filename, img_filename, image->matrixFrame(),
                           image->matrixPlane(), image->matrixGate(),
                           image->matrixData(), image->matrixBed(),
                           v->view_mode, image->bedPos(), num, 2);
              }
        if (sino != NULL) { delete sino;
                            sino=NULL;
                          }
        if (image != NULL) { delete image;
                             image=NULL;
                           }
      }
     GM::gm()->printUsedGMvalues();
   }
   catch (...)
    { if (image != NULL) delete image;
      if (sino != NULL) delete sino;
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
 { bool ret=true;
   std::string mu('mu');

   if (v->image_filename.empty() && v->umap_filename.empty() &&
       v->acf_filename.empty())
    { std::cerr << "An image, " << mu << "-map or acf input file needs to be "
                   "specified.\n";
      ret=false;
    }
   if (!v->image_filename.empty() && !v->umap_filename.empty())
    { std::cerr << "An image and " << mu << "-map input file can't be "
                   "specified together.\n";
      ret=false;
    }
   if (v->oemission_filename.empty() && v->oacf_filename.empty() &&
       v->oumap_filename.empty())
    { std::cerr << "An emission, " << mu << "-map or acf output file needs to"
                   " be specified.\n";
      ret=false;
    }
   if (!v->oemission_filename.empty() && !v->oumap_filename.empty())
    { std::cerr << "An emission and " << mu << "-map output file can't be "
                   "specified together.\n";
      ret=false;
    }
   if (!v->umap_filename.empty() && (v->tof_bins > 1))
    { std::cerr << "Time-Of-Flight sinograms can't be generated from " << mu
                << "-maps.\n";
      ret=false;
    }
   return(ret);
 }

/*---------------------------------------------------------------------------*/
/*! \brief main function of program to calculate a 2d or 3d sinogram from an
           image or u-map.
    \param[in] argc   number of arguments
    \param[in] argv   list of arguments
    \return 0 success
    \return 1 unknown failure
    \return error code

    main function of program to calculate a 2d or 3d sinogram from an image or
    u-map.
    \verbatim
e7_fwd - calculate a 2d or 3d sinogram from an image

Usage:

 e7_fwd ((-i image|-u µ-map [-w width])|-a acf[,r,t,dim] --oe emission|--oa acf|
         --ou µ-map
         [--model model|a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,
                        z1,z2,z3,z4,z1,z2,z3,z4,...] [-R r,t]
         [--scan a,b,c,d,e,f] [--span span] [--mrd mrd]
         [--prj ifore|issrb|fwdpj] [--view] [--ntilt] [--tofp b,w,f]
         [--noise trues,randoms] [--force] [--swap path] [--mem fraction]
         [-d path] [-l a[,path]] [--resrv port])|-v|-h|-?

  -i      name of image input file. Supported formats are ECAT7, Interfile and
          flat (IEEE-float, little endian). In case of a flat-file, the geometry
          of the gantry needs to be specified by the "--model" switch. The width
          and height of the image are assumed to be the same as the projection
          width of the gantry if this information is not available from the file
          header. The additional switch "-w" can be used to specify/overwrite
          this behaviour.
           Examples: -i file.v          (ECAT7)
                     -i file.mhdr       (Interfile)
                     -i file.dat        (flat)
                     -i file.dat -w 128 (flat with dimensions)
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
  -w      width/height of the image. The default value is the width of the
          projection.
           Example: -w 128
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
  --ou    name of µ-map output file. The file format depends on the format of
          the input files (ECAT7, Interfile or IEEE-float flat, little endian).
          µ-map files are stored with a width and height equal to the projection
          width of the gantry. The additional switch "-w" can be used to
          overwrite this behaviour.
           Examples: --ou file.v          (ECAT7)
                     --ou file.mhdr       (Interfile)
                     --ou file.dat        (flat)
                     --ou file.dat -w 128 (flat with width/height)
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
  -R      rebin input sinograms by radial and azimuthal factor. Allowed values
          are 1, 2 and 4.
           Example: -R 2,2
  --span  span for 3d sinogram. The default value is specific for a given
          gantry.
           Example: --span 7
  --mrd   maximum ring difference for 3d sinogram. The default value is
          specific for a given gantry.
           Example: --mrd 17
  --prj   algorithm for the calculation of the 3d sinogram from the 2d . The
          default algorithm is 2d-forward projection followed by inverse fourier
          rebinning. This switch is also used in the calculation of a 3d
          sinogram from a 2d sinogram. In this case are only the parameters
          "issrb" and "ifore" allowed.
           none    don't calculate 3d sinogram
           issrb   inverse single slice rebinning from 2d sinogram
           ifore   inverse fourier rebinning from 2d sinogram
           fwdpj   3d-forward projection from image
           Example: --prj issrb
  --view  store sinogram in view mode. Sinograms in debug files are always
          stored in volume mode.
  --ntilt store sinogram without intrinsic tilt of gantry
  --tofp  parameter for time-of-flight sinogram
           b   number of TOF bins
           w   width of TOF bin in ns
           f   FWHM for TOF bin in ns
           Example: --tofp 9,0.5,1.20422
  --noise add Poisson noise. The sinogram is scaled to the specified number of
          total trues and poisson noise is added.
           Example: --noise 2.2e8,5e7
  --force overwrite existing files
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


        if ( getenv( "GMINI" ) == NULL ) {
                printf( "Environment variable 'GMINI' not set\n" ) ;
                exit( EXIT_FAILURE ) ;
        }


   try
   {                                     // set new handler for "out of memory"

     std::set_new_handler(OutOfMemory);
                                                           // initialize parser
     cpar=new Parser("e7_fwd",
                     "calculate a 2d or 3d sinogram from an image",
                     "2004-2005",
                     "i u w a aarc anarc oe oearc oa oaarc ou model scan R "
                     "span mrd prj view ntilt tofp noise force swap mem d l "
                     "resrv v h ?",
                     "((i$|u$ [w])$|a$ oe|oa|$ ou$ [model]$ [R]$ [scan]$ "
                     "[span]$ [mrd]$ [prj]$ [view]$ [ntilt]$ [tofp]$ [noise]$ "
                     "[force]$ [swap]$ [mem]$ [d]$ [l]$ [resrv])$|v|h|?",
                     0, 0, reg_key+"\\"+logging_path,
                     logging_path, std::string(), "sinogram",
                     std::string());
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
        return(-1);
      }
     if (!validate(cpar->params()))         // validate command line parameters
      { delete cpar;
        cpar=NULL;
        GM::close();
        return(-1);
      }
                            // connect progess handler to reconstruction server
     Progress::pro()->connect(cpar->params()->recoserver,
                              cpar->params()->rs_port);
                                                          // initialize logging
     switch (cpar->params()->logcode % 10)
      { case 0:
         break;
        case 1:
         Logging::flog()->init("e7_fwd", cpar->params()->logcode/10,
                               cpar->params()->logpath, Logging::LOG_FILE);
         break;
        case 2:
         Logging::flog()->init("e7_fwd", cpar->params()->logcode/10,
                               cpar->params()->logpath, Logging::LOG_SCREEN);
         break;
        case 3:
         Logging::flog()->init("e7_fwd", cpar->params()->logcode/10,
                               cpar->params()->logpath,
                               Logging::LOG_FILE|Logging::LOG_SCREEN);
         break;
      }
     printHWInfo(0);                              // print hardware information
     Logging::flog()->logCmdLine(argc, argv);             // print command line
                                                // initialize memory controller
     MemCtrl::mc()->setSwappingPath(cpar->params()->swap_path);
     MemCtrl::mc()->searchForPattern(cpar->patternKey(), 0);
     MemCtrl::mc()->setMemLimit(cpar->params()->memory_limit);
     sw.start();                                             // start stopwatch
     calculate3DSinogram(cpar->params());      // calculate sinogram from image
                                                              // stop stopwatch
     Logging::flog()->logMsg("processing time #1", 0)->arg(secStr(sw.stop()));
     delete cpar;                                              // delete parser
     cpar=NULL;
     MemCtrl::mc()->printPattern(0);              // save memory access pattern
     Logging::close();                                         // close logging
     Progress::close();                               // close progress handler
     MemCtrl::deleteMemoryController();              // close memory controller
     GM::close();                                         // close gantry model
     return(EXIT_SUCCESS);
   }
   catch (const Exception r)                               // handle exceptions
    { sw.stop();
      std::cerr << "Error:: " << r.errStr().c_str() << "\n" << std::endl;
                                 // send error message to reconstruction server
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
