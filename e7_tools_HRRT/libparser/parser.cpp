/*! \class Parser parser.h "parser.h"
    \brief This class is used to parse the command line of an application.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2004/03/15 initial version
    \date 2004/05/18 added Doxygen style comments
    \date 2004/05/19 added "--dttx" switch
    \date 2004/05/19 added "--noise" switch
    \date 2004/05/25 convert '\' in path and filenames to '/'
    \date 2004/05/26 correct the regular expression for the "-l" switch
    \date 2004/05/27 correct the description of the "--ualgo" switch
    \date 2004/07/15 print error message if switch has too many parameters
    \date 2004/07/22 added "--ssf" switch
    \date 2004/07/23 added "--force" switch
    \date 2004/07/23 evaluate logical expression for switch combinations
    \date 2004/07/23 check for existance for file and pathnames
    \date 2004/07/27 added "--resrv" switch
    \date 2004/07/28 added "--asv" switch
    \date 2004/08/03 added "--append" switch
    \date 2004/08/05 use abstraction layer for gantry model
    \date 2004/08/11 added "--thr" switch
    \date 2004/09/03 updated help for --uf and --asv switches
    \date 2004/09/10 added "--athr" switch
    \date 2004/09/10 added "--nbm" switch
    \date 2004/09/16 added switches "-s", "--sarc", "--snarc", "--algo op-osem"
    \date 2004/09/22 added switches "--nrarc" and "--naarc"
    \date 2004/11/04 added switch "--uac"
    \date 2004/11/09 added switch "--mem"
    \date 2004/11/12 added switches "-c" and "--bp"
    \date 2004/11/16 added switches "--pguid" and "--sguid"
    \date 2004/11/17 added switch "--offs"
    \date 2004/11/23 added switches "--iguid" and "--pn"
    \date 2004/11/29 allow multiple GUIDs in "--iguid" switch
    \date 2004/12/16 fixed bug in parsing of "-r fore" switch with limits
    \date 2005/02/24 added switch "--cguid"
    \date 2005/02/24 allow float values for bed positions
    \date 2005/03/11 added "psf-aw-osem" reconstruction
    \date 2009/09/02 Bug fix (crash on computer without gantry model shared libs)
 */

#include <iostream>
#include <limits>
#include <sys/stat.h>
#include "parser.h"
#ifdef USE_GANTRY_MODEL
#include "gantryModelClass.h"
#else
#include "gantryinfo.h"
#endif
#include "getopt_wrapper.h"
#include "gm.h"
#include "str_tmpl.h"
#include "timestamp.h"

/*- constants ---------------------------------------------------------------*/

                                    /*! length of line for usage information */
const unsigned short int Parser::LINE_LEN=80;

                                              /*! description of "-a" switch */
const std::string Parser::a_str[2]={"name of acf input file",
      "name of the acf input file. Supported formats are ECAT7, Interfile and "
      "flat (IEEE-float or 16 bit signed int, little endian). In case of a "
      "flat-file, the geometry of the gantry needs to be specified by the "
      "\"--model\" switch. If the radial or azimuthal dimension of the "
      "sinogram in a flat file is different from the standard values of the "
      "gantry, both values can be specified after the filename. If flat files "
      "are used, the dimensionality of the sinogram can be specified as \"2d\""
      " or \"3d\". The default value in this case is \"3d\". acf files are "
      "assumed to be not arc-corrected if this information is not available "
      "from the file header. The switches \"--anarc\" or \"--aarc\" can be "
      "used instead of \"-a\" to specify/overwrite this behaviour for "
      "cylindrical detector systems.$"
      " Examples: -a file.a             (ECAT7)$"
      "           -a file.mhdr          (Interfile)$"
      "           -a file.dat           (flat, not arc-corrected)$"
      "           -a file.dat,144,72,3d (flat with dimen., not arc-correc.)$"
      "           --anarc file.a        (ECAT7, not arc-corrected)"};
                          /*! description of '--anarc" and "--aarc" switches */
const std::string Parser::aarc_str[2]={"!",
      "see \"-a\""};
                                          /*! description of "--algo" switch */
const std::string Parser::algo_str[2]={"reconstruction algorithm",
      "reconstruction algorithm$"
      " fbp        Filtered-Backprojection$"
      " dift       Direct Inverse Fourier Transform$"
      " mlem       Maximum-Likelihood Expectation-Maximization$"
      " uw-osem    unweighted OSEM (DEFAULT)$"
      " aw-osem    attenuation weighted OSEM$"
      " nw-osem    normalization weighted OSEM$"
      " anw-osem   attenuation and normalization weighted OSEM$"
      " op-osem    ordinary poisson OSEM"};
                                           /*! description of "--app" switch */
const std::string Parser::app_str[2]={"for internal use only",
      "this switch is used internally to be able to append sinograms and "
      "images to existing files in case of online reconstructions."};
                                           /*! description of "--asv" switch */
const std::string Parser::asv_str[2]={"@-value for auto-scaling",
      "@-value for auto-scaling. The default behaviour of the auto-scaling is"
      " to find the water peak in the histogram and scale this towards the "
      "@-value of water (0.096 1/cm). With this switch the scaling can be done"
      " towards a different @-value. The default value is 0.096. This switch "
      "can not be used in combination with the \"--aus\" switch.$"
      " Example: --asv 1.02"};
                                          /*! description of "--athr" switch */
const std::string Parser::athr_str[2]={"acf threshold for scatter "
      "scaling", "acf threshold for scatter scaling. Only the sinogram parts "
      "where the acf value is equal or below the threshold are used to "
      "calculate the scaling factors, excluded margin. The default value is 1.03,0.$"
      " Example: --athr 1.05,4"};
                                           /*! description of "--aus" switch */
const std::string Parser::aus_str[2]={"auto-scaling of @-map off",
      "switch the auto-scaling of the @-map off. The default behaviour of the "
      "segmentation algorithm is to find the water-peak in the u-map and scale"
      " the segments based on this. The switch \"--aus\" can be used to switch"
      " this automatic scaling off."};
                                              /*! description of "-b" switch */
const std::string Parser::b_str[2]={"name of blank input file",
      "name of the blank input file. Supported formats are ECAT7, Interfile "
      "and flat (IEEE-float or 16 bit signed int, little endian). In case of a"
      " flat-file, the geometry of the gantry needs to be specified by the "
      "\"--model\" switch. If the radial or azimuthal dimension of the "
      "sinogram in a flat file is different from the standard values of the "
      "gantry, both values can be specified after the filename. Blank files "
      "are assumed to be not arc-corrected if this information is not "
      "available from the file header. The switches \"--bnarc\" or \"--barc\" "
      "can be used instead of \"-b\" to specify/overwrite this behaviour for "
      "cylindrical detector systems.$"
      " Examples: -b file.s          (ECAT7)$"
      "           -b file.mhdr       (Interfile)$"
      "           -b file.dat        (flat, not arc-corrected)$"
      "           -b file.dat,144,72 (flat with dimensions, not arc-correc.)$"
      "           --barc file.s      (ECAT7, arc-corrected)"};
                            /*! description of "-barc" and "-bnarc" switches */
const std::string Parser::barc_str[2]={"!",
      "see \"-b\""};
                                            /*! description of "--bc" switch */
const std::string Parser::bc_str[2]={"width of boxcar-filter",
      "width of boxcar-filter in pixel for axial scatter scaling factors "
      "(odd number). The default value is #9. This switch doesn't work in "
      "combination with the \"--newsc\" switch.$"
      " Example: --bc 23"};
                                            /*! description of "--bp" switch */
const std::string Parser::bp_str[2]={"position of the PET beds",
      "The absolute positions of the PET beds in mm.$"
      " Example: --bp 143.9,476.9,809.9,1142.9"};
                                              /*! description of "-c" switch */
const std::string Parser::c_str[2]={"name of CT image input file",
      "name of CT image input file. Supported formats are ECAT7 and "
      "Interfile.$"
      " Examples: -c file.v    (ECAT7)$"
      "           -c file.mhdr (Interfile)"};
                                         /*! description of "--cguid" switch */
const std::string Parser::cguid_str[2]={"scanogram GUID for database",
      "scanogram GUID for database$"
      " Example: --cguid 83FD1E72-A5D5-4AF2-B1C3-16F0BD01DD8D"};
                                              /*! description of "-d" switch */
const std::string Parser::d_str[2]={"path for debug information",
      "serveral files with intermediate results are stored in the specified "
      "directory during reconstruction. The files that may be created are:$"
      " acf2d_00             2d-ACF$"
      " acf3d_00             3d-ACF$"
      " blank2d_00           2d-blank$"
      " emis2d_00            2d-emission$"
      " emis3d_00            3d-emission$"
      " emis2d_corr_00       corrected 2d-emission$"
      " emis3d_corr_00       corrected 3d-emission$"
      " image_00             image$"
      " norm_00              norm$"
      " norm3d_00            3d-norm$"
      " psmr_00              prompts and smoothed randoms$"
      " scatter_estim3d_00   3d scatter sinogram$"
      " sino2d_00            2d-sinogram$"
      " sino2d_noise_00      noise 2d-sinogram$"
      " sino3d_00            3d-sinogram$"
      " sino3d_noise_00      noisy 3d-sinogram$"
      " ssr_00               SSRB(e*n*a)$"
      " ssr_img_00           AW-OSEM(ssr_00)$"
      " ssr_img_filt_00      smoothed version of ssr_img_00$"
      " trans2d_00           2d-transmission$"
      " umap_00              @-map$"
      "where \"_00\" is replaced by the frame number.$"
      " Example: -d #10"};
                                            /*! description of "--d2" switch */
const std::string Parser::d2_str[2]={"store intermediate iterations",
      "store images of intermediate iterations. The images are stored in the "
      "same directory and under a similar name as the final image."};
                                            /*! description of "--dc" switch */
const std::string Parser::dc_str[2]={"decay correction",
      "decay correction"};
const std::string Parser::dttx_str[2]={"transmission deadtime correction",
      "transmission deadtime correction$"
      " path     path for blank database$"
      " frames   number of frames in blank database$"
      " Example: --dttx #10,9"};
                                              /*! description of "-e" switch */
const std::string Parser::e_str[2]={"name of emission input file",
      "name of the emission input file. Supported formats are ECAT7, Interfile"
      " and flat (IEEE-float or 16 bit signed int, little endian). In case of "
      "a flat-file, the geometry of the gantry needs to be specified by the "
      "\"--model\" switch. If the radial or azimuthal dimension of the "
      "sinogram in a flat file is different from the standard values of the "
      "gantry, both values can be specified after the filename. If flat files "
      "are used, the dimensionality of the sinogram can be specified as \"2d\""
      " or \"3d\". The default value in this case is \"3d\". emission files"
      " are assumed to be not arc-corrected if this information is not "
      "available from the file header. The switches \"--enarc\" or \"--earc\" "
      "can be used instead of \"-e\" to specify/overwrite this behaviour for "
      "cylindrical detector systems.$"
      " Examples: -e file.s             (ECAT7)$"
      "           -e file.mhdr          (Interfile)$"
      "           -e file.dat           (flat, not arc-corrected)$"
      "           -e file.dat,144,72,2d (flat with dimen., not arc-correc.)$"
      "           --enarc file.s        (ECAT7, not arc-corrected)"};
                          /*! description of "--earc" and "--enarc" switches */
const std::string Parser::earc_str[2]={"!",
      "see \"-e\""};
                                           /*! description of "--ecf" switch */
const std::string Parser::ecf_str[2]={"apply ECF after reconstruction",
      "apply ECF after reconstruction. The ECF value in the image header will "
      "be 1.0 which might lead some image viewers to the conclusion, that the"
      " image is not calibrated. If the ECF value is unknown, a default value"
      " of 1000000 is applied, to scale the PET data to a \"reasonable\" "
      "range."};
                                             /*! description of "-fl" switch */
const std::string Parser::fl_str[2]={"frame-length correction",
      "frame-length correction"};
                                         /*! description of "--force" switch */
const std::string Parser::force_str[2]={"overwrite existing files",
      "overwrite existing files"};
                                             /*! description of "-gf" switch */
const std::string Parser::gf_str[2]={"gap-filling",
      "use the normalization data to fill the gaps in the emission sinogram by"
      " linear interpolation"};
                                         /*! description of "--gibbs" switch */
const std::string Parser::gibbs_str[2]={"3d gibbs prior",
      "use 3d gibbs prior in MLEM or OSEM reconstruction$"
      " Example: --gibbs 0.3"};
                                           /*! description of "--gxy" switch */
const std::string Parser:: gxy_str[2]={"gaussian smoothing of #5 in "
      "x/y-direction", "gaussian smoothing of #5 in x/y-direction. The filter "
      "width is specified in mm FWHM.$"
      " Example: --gxy 3.2"};
                                            /*! description of "--gz" switch */
const std::string Parser::gz_str[2]={"gaussian smoothing of #5 in z-direction",
      "gaussian smoothing of #5 in z-direction. The filter width is "
      "specified in mm FWHM.$"
      " Example: --gz 6.78"};
                                              /*! description of "-h" switch */
const std::string Parser::h_str[2]={"print extended help",
      "print this information"};
                                              /*! description of "-i" switch */
const std::string Parser::i_str[2]={"name of image input file",
      "name of image input file. Supported formats are ECAT7, Interfile and "
      "flat (IEEE-float, little endian). In case of a flat-file, the geometry "
      "of the gantry needs to be specified by the \"--model\" switch. The "
      "width and height of the image are assumed to be the same as the "
      "projection width of the gantry if this information is not available "
      "from the file header. The additional switch \"-w\" can be used to "
      "specify/overwrite this behaviour.$"
      " Examples: -i file.v          (ECAT7)$"
      "           -i file.mhdr       (Interfile)$"
      "           -i file.dat        (flat)$"
      "           -i file.dat -w 128 (flat with dimensions)"};
                                         /*! description of "--iguid" switch */
const std::string Parser::iguid_str[2]={"image series GUIDs for database",
      "image series GUIDs for database. If there is more than one image frame,"
      " e.g. in dynamic images, the GUIDs have to be separated by ','.$"
      " Example: --iguid 83FD1E72-A5D5-4AF2-B1C3-16F0BD01DD8D"};
                                            /*! description of "--is" switch */
const std::string Parser::is_str[2]={"number of iterations and subsets",
      "number of iterations and subsets for iterative reconstruction of the "
      "#5. The default values are \"#1,#2\". If the number of azimuthal "
      "projections in the #6 can not be devided by the number of subsets, the "
      "number of subsets will be decreased. If ML-EM is chosen as "
      "reconstruction algorithm, the number of subsets needs to be 1.$"
      " Example: --is 8,12"};
                                              /*! description of "-k" switch */
const std::string Parser::k_str[2]={"simulate dead blocks in input sinograms",
      "counts measured by the detector blocks with the given numbers are "
      "removed from the input sinograms to simulate dead blocks. This switch "
      "is only allowed with cylindrical detector systems."};
                                              /*! description of "-l" switch */
const std::string Parser::l_str[2]={"output logging information",
      "output logging information. The first parameter specifies the level of "
      "detail from 0 to 7 in its first digit. The second digit specifies the "
      "output destination: 0=no logging, 1=logging to file, 2=logging to "
      "screen, 3=logging to file and screen. The optional second parameter "
      "specifies a path for the log files. The default path is stored in the "
      "registry variable \"#3\" (Windows) or the "
      "environment variable \"#4\" (Linux/Solaris/Mac OS X). If these "
      "are not defined, the local directory is used. The program creates one "
      "log file for each day of the month. After one month old log files are "
      "overwritten. The default parameter is \"72\".$"
      " Examples: -l 72          (output very detailed to screen)$"
#ifdef WIN32
      "           -l 73,C:\\      (output very detailed to screen and file)$"
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
      "           -l 73,/var/log (output very detailed to screen and file)$"
#endif
      "           -l 31          (output medium detailed to file in default$"
      "                           directory)"};
                                           /*! description of "--mat" switch */
const std::string Parser::mat_str[2]={"number(s) of matrix/matrices to "
      "process",
      "number(s) of matrix/matrices to process. The program can process a "
      "single matrix, a range of matrices or a set of matrices. Matrix "
      "numbers have to be specified in increasing order. The default is to "
      "process all matrices.$"
      " Examples: --mat 3     (process only the 4th(!) matrix of a file)$"
      "           --mat 0-2   (process the the first 3 matrices of a file)$"
      "           --mat 1,3,5 (process the 2nd, 4th and 6th matrix of a file)"
      };
                                           /*! description of "--mem" switch */
const std::string Parser::mem_str[2]={"set limit for memory controller",
      "set limit for memory controller. The given fraction of the physical "
      "memory will be used by the memory controller. Not all memory that is "
      "used by the program is controlled by the memory controller, therefore "
      "the fraction should be smaller than 1 to leave some physical memory "
      "for this \"uncontrolled\" use. The default value is 0.8. In case of "
      "\"memory exhausted\" errors a smaller value should help.$"
      " Example: --mem 0.7   (use 70% of the physical memory)"};
                                         /*! description of "--model" switch */
const std::string Parser::model_str[2]={"gantry model specification",
      "gantry model specification (only for flat files)$"
      " model   gantry model number$"
      " a       number of bins in a projection$"
      " b       width of bins in mm$"
      " c       number of azimuthal angles in sinogram$"
      " d       number of cystal rings$"
      " e       number of crystals per ring$"
      " f       ring spacing in mm$"
      " g       span$"
      " h       maximum ring difference$"
      " i       radius of detector ring in mm$"
      " j       interaction depth in mm$"
      " k       intrinsic tilt in degrees$"
      " l       transaxial crystals per block$"
      " m       axial crystals per block$"
      " n       transaxial blocks per bucket$"
      " o       axial blocks per bucket$"
      " p       number of bucket rings$"
      " q       number of buckets per ring$"
      " r       gap crystals per block in transaxial direction$"
      " s       gap crystals per block in axial direction$"
      " t       detector type (0=panel, 1=ring)$"
      " u       gantry type (0=PET/CT, 1=PET only)$"
      " v       axial arc correction required (0=no, 1=yes)$"
      " w       default lower level energy threshold in keV$"
      " x       default upper level energy threshold in keV$"
      " y       maximum z-FOV for scatter simulation in mm$"
      " z1      number of crystal layers$"
      " z2      crystal layer material (LSO, BGO, NaI, LYSO)$"
      " z3      crystal layer depth in mm$"
      " z4      FWHM energy resolution of crystal layer in %$"
      " z5      background energy ratio of crystal layer in %$"
      " Example: -model 924$"
      "          -model 320,2.2,256,120,512,2.208,7,87,436,10,0,12,12,7,10,1,$"
      "                 0,0,0,0,1,0,425,650,400.0,1,LSO,2.0,16.0,0.0"
      };
                                           /*! description of "--mrd" switch */
const std::string Parser::mrd_str[2]={"maximum ring difference for 3d #6",
      "maximum ring difference for 3d #6. The default value is specific "
      "for a given gantry.$"
      " Example: --mrd 17"};
                                              /*! description of "-n" switch */
const std::string Parser::n_str[2]={"name of norm input file",
      "name of the norm input file. Supported formats are ECAT7, Interfile and"
      " flat (IEEE-float or 16 bit signed int, little endian). In case of a "
      "flat-file, the geometry of the gantry needs to be specified by the "
      "\"--model\" switch. If the radial or azimuthal dimension of the "
      "sinogram in a flat file is different from the standard values of the "
      "gantry, both values can be specified after the filename. Norm files "
      "are assumed to be not arc-corrected.$"
      " Examples: -n file.s          (ECAT7)$"
      "           -n file.nhdr       (Interfile)$"
      "           -n file.dat        (flat)$"
      "           -n file.dat,144,72 (flat with dimensions)"};
                                         /*! description of "--naarc" switch */
const std::string Parser::naarc_str[2]={"no axial arc-corrections",
      "switch off all axial arc-corrections"};
                                           /*! description of "--nbm" switch */
const std::string Parser::nbm_str[2]={"number of bins to mask in norm",
      "the given number of bins is set to 0 at the borders of each projection "
      "in the norm sinogram.$"
      " Example: --nbm 3"};
                                         /*! description of "--newsc" switch */
const std::string Parser::newsc_str[2]={"use new version of scatter code",
      "use new version of scatter code. This scatter code requires an "
      "installed IDL runtime license und can currently only be used with the "
      "gantry models 1023, 1024, 1062, 1080 and 1090. The code doesn't use the"
      " gantry model files, instead it uses hardcoded values which may be "
      "different from the gantry model." };
                                         /*! description of "--noise" switch */
const std::string Parser::noise_str[2]={"add Poisson noise",
      "add Poisson noise. The sinogram is scaled to the specified number of "
      "total trues and poisson noise is added.$"
      " Example: --noise 2.2e8,5e7"};
                                         /*! description of "--ntilt" switch */
const std::string Parser::ntilt_str[2]={"store #6 without intrinsic tilt of "
      "gantry", "store #6 without intrinsic tilt of gantry"};
                                          /*! description of "--nosc" switch */
const std::string Parser::nosc_str[2]={"don't perform scatter correction",
      "don't perform scatter correction"};
                                            /*! description of "--np" switch */
const std::string Parser::np_str[2]={"name of P39 patient norm input file",
      "name of the P39 patient norm input file$"
      " Examples: --np file.nhdr"};
                                         /*! description of "--nrarc" switch */
const std::string Parser::nrarc_str[2]={"no radial arc-corrections",
      "switch off all radial arc-corrections"};
                                            /*! description of "--oa" switch */
const std::string Parser::oa_str[2]={"name of acf output file",
      "name of the acf output file. The file format depends on the format of "
      "the input files (ECAT7, Interfile or IEEE-float flat, little endian). "
      "acf files are stored without arc-correction. The switch \"--oaarc\" can"
      " be used instead of \"--oa\" to overwrite this behaviour for "
      "cylindrical detector systems.$"
      " Examples: --oa file.a      (ECAT7, not arc-corrected)$"
      "           --oa file.mhdr   (Interfile, not arc-corrected)$"
      "           --oa file.dat    (flat, not arc-corrected)$"
      "           --oaarc file.a   (ECAT7, arc-corrected)"};
                                         /*! description of "--oaarc" switch */
const std::string Parser::oaarc_str[2]={"!",
      "see \"--oa\""};
                                            /*! description of "--oe" switch */
const std::string Parser::oe_str[2]={"name of emission output file",
      "name of the emission output file. The file format depends on the "
      "format of the input files (ECAT7, Interfile or IEEE-float flat, little "
      "endian). emission files are stored without arc-correction. The switch "
      "\"--oearc\" can be used instead of \"--oe\" to overwrite this "
      "behaviour for cylindrical detector systems.$"
      " Examples: --oe file.s      (ECAT7, not arc-corrected)$"
      "           --oe file.mhdr   (Interfile, not arc-corrected)$"
      "           --oe file.dat    (flat, not arc-corrected)$"
      "           --oearc file.s   (ECAT7, arc-corrected)"};
                                         /*! description of "--oearc" switch */
const std::string Parser::oearc_str[2]={"!",
      "see \"--oe\""};
                                          /*! description of "--offs" switch */
const std::string Parser::offs_str[2]={"offset between CT and PET coordinate "
      "system",
      "x-, y- and z-offset to convert from the CT to the PET coordinate "
      "system$"
      " Example: --offs -0.3,-2.6,-1104.4"};
                                            /*! description of "--oi" switch */
const std::string Parser::oi_str[2]={"name of image output file",
      "name of image output file. The file format depends on the format of the"
      " input files (ECAT7, Interfile or IEEE-float flat, little endian).$"
      " Examples: --oi file.v          (ECAT7)$"
      "           --oi file.mhdr       (Interfile)$"
      "           --oi file.dat        (flat)"};
                                            /*! description of "--ol" switch */
const std::string Parser::ol_str[2]={"overlap beds",
      "overlap beds in reconstructed image. Images are allways stored in head "
      "first mode."};
                                            /*! description of "--os" switch */
const std::string Parser::os_str[2]={"name of scatter output file",
      "name of the scatter output file. The file format depends on the "
      "format of the input files (ECAT7, Interfile or IEEE-float flat, little "
      "endian). Scatter files are stored without arc-correction. The switch "
      "\"--osarc\" can be used instead of \"--os\" to overwrite this "
      "behaviour for cylindrical detector systems.$"
      " Examples: --os file.s      (ECAT7, not arc-corrected)$"
      "           --oi file.mhdr   (Interfile, not arc-corrected)$"
      "           --os file.dat    (flat, not arc-corrected)$"
      "           --osarc file.s   (ECAT7, arc-corrected)"};
                                         /*! description of "--osarc" switch */
const std::string Parser::osarc_str[2]={"!",
      "see \"--os\""};
                                         /*! description of "--os2d" switch */
const std::string Parser::os2d_str[2]={"save unscaled 2d scatter and scaling "
      "factors",
      "save unscaled 2d scatter and scaling factors instead of 3D sinogram "
      "to reduce file size"};
                                         /*! description of "--lber" switch */
const std::string Parser::lber_str[2]={"Specify back and front Layers "
      "Background Energy Ratio.",
      "Specify back and front Layers Background Energy ratio to override "
      "values from Gantry Model.$"
      " Example: --lber 12.0"};

                                            /*! description of "--ou" switch */
const std::string Parser::ou_str[2]={"name of @-map output file",
      "name of @-map output file. The file format depends on the format of "
      "the input files (ECAT7, Interfile or IEEE-float flat, little endian). "
      "@-map files are stored with a width and height equal to the projection "
      "width of the gantry. The additional switch \"-w\" can be used to "
      "overwrite this behaviour.$"
      " Examples: --ou file.v          (ECAT7)$"
      "           --ou file.mhdr       (Interfile)$"
      "           --ou file.dat        (flat)$"
      "           --ou file.dat -w 128 (flat with width/height)"};
                                              /*! description of "-p" switch */
const std::string Parser::p_str[2]={"parameters for segmentation of @-map if "
      "reconstructed with intensity prior",
      "parameters for segmentation of @-map if reconstructed with intensity "
      "prior. The default is$\"#8\".$"
      " segments  number of segments$"
      " mean      mean value for gaussian prior$"
      " std       standard deviation for gaussian prior$"
      " b         boundary between segments$"
      "If the switch \"--uf\" is used at the same time, the mean values and "
      "segment boundaries are given based on the scaled @-map.$"
      " Example: -p 4,0,0.002,0.03,0.06,0.05,0.06,0.096,0.02,0.015,0.04,0.065$"
      "          (use four gaussian priors with the mean and std values$"
      "           (0,0.002), (0.03,0.06), (0.05,0.06), (0.096,0.02) and the$"
      "           three segment boundaries 0.015, 0.04 and 0.065)"};
                                         /*! description of "--pguid" switch */
const std::string Parser::pguid_str[2]={"patient GUID for database",
      "patient GUID for database$"
      " Example: --pguid 83FD1E72-A5D5-4AF2-B1C3-16F0BD01DD8D"};
                                            /*! description of "--pn" switch */
const std::string Parser::pn_str[2]={"patient name",
      "patient name from the database. A list of all study GUIDs and series "
      "GUIDs will be printed to the log file.$"
      " Example: --pn \"Kehren^Frank\""};
                                           /*! description of "--prj" switch */
const std::string Parser::prj_str[2]={"algorithm for calculation of 3d #6 from"
      " 2d #11",
      "algorithm for the calculation of the 3d #6 from the 2d #11. The default"
      " algorithm is 2d-forward projection followed by inverse fourier "
      "rebinning. This switch is also used in the calculation of a 3d sinogram"
      " from a 2d sinogram. In this case are only the parameters \"issrb\" "
      "and \"ifore\" allowed.$"
      " none    don't calculate 3d #6$"
      " issrb   inverse single slice rebinning from 2d sinogram$"
      " ifore   inverse fourier rebinning from 2d sinogram$"
      " fwdpj   3d-forward projection from image$"
      " Example: --prj issrb"};
                                              /*! description of "-q" switch */
const std::string Parser::q_str[2]={"path for quality control information",
      "Gnuplot scripts are stored in the specified directory during "
      "reconstruction. Executing them leads to Postscript files with "
      "plots that support in quality control.$"
      " Example: -q #10"};
                                              /*! description of "-?" switch */
const std::string Parser::qm_str[2]={"print this information",
                                     "print short help"};
                                              /*! description of "-r" switch */
const std::string Parser::r_str[2]={"algorithm for calculation of 2d #6",
      "algorithm for the calculation of a 2d #6 from the 3d #6$"
      " fore   fourier rebinning$"
      " ssrb   single slice rebinning$"
      " seg0   use only segment 0$"
      "In case of \"fore\" three additional parameters can be used to specify "
      "the limits between single slice and fourier rebinning:$"
      " alim   axis up to which single slice rebinning is used$"
      " wlim   radial limit$"
      " klim   angular limit$"
      " Example: -r fore,0,2,2"};
                                              /*! description of "-R" switch */
const std::string Parser::R_str[2]={"rebin input sinograms by radial and "
      "azimuthal factor",
      "rebin input sinograms by radial and azimuthal factor. Allowed values "
      "are 1, 2 and 4.$"
      " Example: -R 2,2"};
                                         /*! description of "--resrv" switch */
const std::string Parser::resrv_str[2]={"for internal use only",
      "this switch is used internally to enable communication between the "
      "e7-Tool and the reconstruction server."};
                                            /*! description of "--rs" switch */
const std::string Parser::rs_str[2]={"randoms smoothing", "randoms smoothing"};
                                             /*! description of "-s" switch */
const std::string Parser::s_str[2]={"name of scatter input file",
      "name of the scatter input file. Supported formats are ECAT7, Interfile"
      " and flat (IEEE-float or 16 bit signed int, little endian). In case of "
      "a flat-file, the geometry of the gantry needs to be specified by the "
      "\"--model\" switch. If the radial or azimuthal dimension of the "
      "sinogram in a flat file is different from the standard values of the "
      "gantry, both values can be specified after the filename. If flat files "
      "are used, the dimensionality of the sinogram can be specified as \"2d\""
      " or \"3d\". The default value in this case is \"3d\". scatter files"
      " are assumed to be not arc-corrected if this information is not "
      "available from the file header. The switches \"--snarc\" or \"--sarc\" "
      "can be used instead of \"-s\" to specify/overwrite this behaviour for "
      "cylindrical detector systems.$"
      " Examples: -s file.s             (ECAT7)$"
      "           -s file.mhdr          (Interfile)$"
      "           -s file.dat           (flat, not arc-corrected)$"
      "           -s file.dat,144,72,2d (flat with dimen., not arc-correc.)$"
      "           --snarc file.s        (ECAT7, not arc-corrected)"};
                          /*! description of "--sarc" and "--snarc" switches */
const std::string Parser::sarc_str[2]={"!",
      "see \"-s\""};
                                          /*! description of "--scan" switch */
const std::string Parser::scan_str[2]={"information about scan",
      "information about scan (only for flat files)$"
      " a   lld in keV$"
      " b   uld in keV$"
      " c   start time of the scan in sec$"
      " d   frame duration of the scan in sec$"
      " e   gate duration of the scan in sec$"
      " f   halflife of isotope in sec$"
      " g   position of the bed in mm$"
      " Example: --scan 400,650,0,300,0,6586.2,245.3"};
                                         /*! description of "--sguid" switch */
const std::string Parser::sguid_str[2]={"study GUID for database",
      "study GUID for database$"
      " Example: --sguid 83FD1E72-A5D5-4AF2-B1C3-16F0BD01DD8D"};
                                          /*! description of "--skip" switch */
const std::string Parser::skip_str[2]={"skip border planes in scatter "
      "estimation",
      "the given number of planes are skipped at the beginning and end of each"
      " segment during the calculation of the single-slice rebinned sinogram, "
      "which is used during the 2d scatter estimation. The default is 1. This "
      "switch doesn't work in combination with the \"--newsc\" switch.$"
      " Example: --skip 2"};
                                          /*! description of "--span" switch */
const std::string Parser::span_str[2]={"span for 3d #6",
      "span for 3d #6. The default value is specific for a given gantry.$"
      " Example: --span 7"};
                                           /*! description of "--ssf" switch */
const std::string Parser::ssf_str[2]={"thresholds for scatter scaling factors",
      "lower and upper thresholds for scatter scaling factors. The default is "
      "not to apply thresholds. This switch doesn't work in combination with "
      "the \"--newsc\" switch.$"
      " Example: --ssf 0.3,1.9$"
      "          --ssf 1.0,1.0 (switch off scatter scaling)$"
      "          --ssf 0.0,0.0 (switch off thresholds)"};
                                          /*! description of "--swap" switch */
const std::string Parser::swap_str[2]={"path for swapping files",
      "path for swapping files. If the memory requirements are bigger than the"
      " amount of physical memory, parts of the data are swapped to disc. "
      "Performance increases if a directory on a separate disc is used for "
      "swapping. The default is the local directory.$"
      " Example: --swap #10"};
                                              /*! description of "-t" switch */
const std::string Parser::t_str[2]={"name of transmission input file",
      "name of the transmission input file. Supported formats are ECAT7, "
      "Interfile and flat (IEEE-float or 16 bit signed int, little endian). In"
      " case of a flat-file, the geometry of the gantry needs to be specified "
      "by the \"--model\" switch. If the radial or azimuthal dimension of the "
      "sinogram in a flat file is different from the standard values of the "
      "gantry, both values can be specified after the filename. Transmission "
      "files are assumed to be not arc-corrected if this information is not "
      "available from the file header. The switches \"--tnarc\" or \"--tarc\" "
      "can be used instead of \"-t\" to specify/overwrite this behaviour for "
      "cylindrical detector systems.$"
      " Examples: -t file.s          (ECAT7)$"
      "           -t file.mhdr       (Interfile)$"
      "           -t file.dat        (flat, not arc-corrected)$"
      "           -t file.dat,144,72 (flat with dimensions, not arc-correc.)$"
      "           --tnarc file.s     (ECAT7, not arc-corrected)"};
                          /*! description of "--tarc" and "--tnarc" switches */
const std::string Parser::tarc_str[2]={"!",
      "see \"-t\""};
                                           /*! description of "--thr" switch */
const std::string Parser::thr_str[2]={"threshold and @-value",
       "threshold and @-value. Replace all voxels in the image below the "
       "threshold by 0 and all voxels above the threshold by the @-value.$"
       " t   threshold for PET image$"
       " u   @-value for @-map$"
       " Example: --thr 0.15,0.096"};
                                           /*! description of "--tof" switch */
const std::string Parser::tof_str[2]={"reconstruct TOF datasets by TOF "
      "algorithms",
      "reconstruct TOF datasets by TOF algorithms. If this switch is not used,"
      " the time-of-flight bins in a TOF dataset are just added up. The TOF "
      "processing in the reconstruction can't be used in combination with "
      "DIFT." };
                                          /*! description of "--tofp" switch */
const std::string Parser::tofp_str[2]={"parameter for time-of-flight sinogram",
      "parameter for time-of-flight sinogram$"
      " b   number of TOF bins$"
      " w   width of TOF bin in ns$"
      " f   FWHM for TOF bin in ns$"
      " Example: --tofp 9,0.5,1.20422"};
                                          /*! description of "--trim" switch */
const std::string Parser::trim_str[2]={"trim sinogram",
      "the given number of bins is cut off at each side of a projection, "
      "before the sinogram is reconstructed. If the \"-R\" switch is used at "
      "the same time, the \"--trim\" switch has precedence.$"
      " Example: --trim 20"};

const std::string Parser::txsc_str[2]={"Transmission scatter correction "
      "factors a,b",
      "Transmission scatter correction a,b : ln(ACF) = a + b*ln(bl/tx) "
      "default (a,b)=(0,1.0)"};

const std::string Parser::txblr_str[2]={"Transmission/Blank ratio ",
      "Transmission/Blank ratio, computed from data by default"};

                                              /*! description of "-u" switch */
const std::string Parser::u_str[2]={"name of @-map input file",
      "name of @-map input file. Supported formats are ECAT7, Interfile and "
      "flat (IEEE-float, little endian). In case of a flat-file, the geometry "
      "of the gantry needs to be specified by the \"--model\" switch. The "
      "width and height of the @-map are assumed to be the same as the "
      "projection width of the gantry if this information is not available "
      "from the file header. The additional switch \"-w\" can be used to "
      "specify/overwrite this behaviour.$"
      " Examples: -u file.v          (ECAT7)$"
      "           -u file.mhdr       (Interfile)$"
      "           -u file.dat        (flat)$"
      "           -u file.dat -w 128 (flat with dimensions)"};
                                         /*! description of "--ualgo" switch */
const std::string Parser::ualgo_str[2]={"method for @-map reconstruction",
      "method for @-map reconstruction. The default is \"#7\".$"
      "  1   unweighted OSEM$"
      " 20   Gradient Ascending with smoothness constraints and gaussian$"
      "      smoothing model (== --ualgo 40,1.5,0.00025,0,0)$"
      " 21   Gradient Ascending with smoothness constraints and Geman-McClure$"
      "      smoothing model (== --ualgo 41,1.5,0.00025,0,0)$"
      " 30   Gradient Ascending full model and gaussian smoothing model$"
      "       (== --ualgo 40,1.5,1,0.05,5)$"
      " 31   Gradient Ascending full model and Geman-McClure smoothing model$"
      "       (== --ualgo 41,1.5,1,0.05,5)$"
      " 40   user defined model with gaussian smoothing model$"
      "       alpha    relaxation parameter$"
      "       ;        regularization parameter for smoothing prior$"
      "       ;_ip     regularization parameter for intensity prior$"
      "       iter_ip  first iteration for intensity prior$"
      " 41   user defined model with Gean-McClure smoothing model$"
      "       alpha    relaxation parameter$"
      "       ;        regularization parameter for smoothing prior$"
      "       ;_ip     regularization parameter for intensity prior$"
      "       iter_ip  first iteration for intensity prior$"
                                        " Example: --ualgo 40,1.5,3,0,0"};
                                           /*! description of "--uac" switch */
const std::string Parser::uac_str[2]={"undo the attenuation correction",
      "undo the attenuation correction. This will create an image that doesn't"
      " have an attenuation correction, although it might have a scatter "
      "correction."};
                                          /*! description of "--ucut" switch */
const std::string Parser::ucut_str[2]={"cut blank and transmission",
      "cut-off first and last plane of blank and transmission. If the switch "
      "\"--uzoom\" is used, the cutting is done after the axial resampling."};
                                            /*! description of "--uf" switch */
const std::string Parser::uf_str[2]={"scaling factor for segmentation",
      "This scaling factor is applied to the mean values and standard "
      "deviations of the gaussian prior and to the boundary values of the "
      "segments. E.g. using the switches$"
      "-p 4,0,0.002,0.03,0.06,0.05,0.06,0.096,0.02,0.015,0.04,0.065"
      " --uf 0.9 is equivalent to using$"
      "-p 4,0,0.00222,0.0333,0.0667,0.0556,0.0667,"
      "0.1067,0.0222,0.0167,$   0.0444,0.0722.$"
      " Example: --uf 0.95"};
                                         /*! description of "--uzoom" switch */
const std::string Parser::uzoom_str[2]={"zoom factors for the @-map",
      "zoom factors for the @-map. These zoom factors are applied during the "
      "calculation of the @-map from blank and transmission. Allowed values "
      "are 1,2 and 4 for the x/y-direction and 1,2,3 and 4 for the "
      "z-direction. The rebinning that can be selected with the \"-R\" "
      "switch would instead be applied on blank and transmission data - before"
      " the reconstruction.$"
      " Example: --uzoom 2,3 (reconstructs an 80x80x80 @-map if the blank and$"
      "                       transmission sinograms are 160 bins wide and$"
      "                       have 239 planes)"};
                                              /*! description of "-v" switch */
const std::string Parser::v_str[2]={"print version information",
      "prints the date and time when the program was compiled and the used "
      "compiler version"};
                                          /*! description of "--view" switch */
const std::string Parser::view_str[2]={"store #6 in view mode",
      "store #6 in view mode. Sinograms in debug files are always stored in "
      "volume mode."};
                                              /*! description of "-w" switch */
const std::string Parser::w_str[2]={"width/height of image",
      "width/height of the image. The default value is the width of the "
      "projection.$"
      " Example: -w 128"};
                                          /*! description of "--zoom" switch */
const std::string Parser::zoom_str[2]={"zoom factor for the image",
      "zoom factor for the image. This zoom factor is applied during the "
      "calculation of the image.$"
      " Example: --zoom 1.5"};

                                      /*! rules for combinations of switches */
const Parser::truleset Parser::rules[]={
      {"app", "!force"},
      {"asv", "!aus"},
      {"force", "!app"},
  /*
      {"a",     "oe|oearc|oi|os|ou"},
      {"aarc",  "oe|oearc|oi|os|ou"},
      {"anarc", "oe|oearc|oi|os|ou"},
      {"aus",   "(b|barc|bnarc|t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"b",     "(t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"barc",  "(t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"bnarc", "(t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"dttx",  "(b|barc|bnarc|t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"force", "oa|oaarc|oe|oearc|oi|os|ou"},
      {"gxy",   "oi|ou|b|barc|bnarc|t|tarc|tnarc"},
      {"gz",    "oi|ou|b|barc|bnarc|t|tarc|tnarc"},
      {"i",     "oe|oearc"},
      {"k",     "b|barc|bnarc|t|tarc|tnarc|e"},
      {"oa",    "(b|barc|bnarc|t|tarc|tnarc)|u"},
      {"oaarc", "(b|barc|bnarc|t|tarc|tnarc)|u"},
      {"oe",    "e|earc|i"},
      {"oearc", "e|earc|i"},
      {"ou",    "b|barc|bnarc|t|tarc|tnarc|a|aarc|anarc"},
      {"mat",   "a|arc|anarc|b|barc|bnarc|t|tarc|tnarc|e|i"},
      {"mrd",   "oa|oaarc|oe|oearc"},
      {"ntilt", "oa|oaarc|oe|oearc|os"},
      {"p",     "(b|barc|bnarc|t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"prj",   "oa|oaarc|oe|oearc"},
      {"q",     "(b|barc|bnarc|t|tarc|tnarc)"},
      {"R",     "a|aarc|anarc|b|barc|bnarc|t|tarc|tnarc|e"},
      {"rs",    "t|tarc|tnarc|e"},
      {"span",  "oa|oaarc|oe|oearc"},
      {"t",     "(b|barc|bnarc)&(oa|oaarc|ou)"},
      {"tarc",  "(b|barc|bnarc)&(oa|oaarc|ou)"},
      {"tnarc", "(b|barc|bnarc)&(oa|oaarc|ou)"},
      {"u",     "oa|oaarc|oe|oearc"},
      {"ualgo", "(b|barc|bnarc|t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"uf",    "(b|barc|bnarc|t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"ucut",  "(b|barc|bnarc|t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"uzoom", "(b|barc|bnarc|t|tarc|tnarc)&(oa|oaarc|ou)"},
      {"view",  "oa|oaarc|oe|oearc|os"},*/
      {"w",     "u|oi|ou"}
      };

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _prgname      name of application
    \param[in] _descr        description of application
    \param[in] _date         copyright date
    \param[in] _switches     allowed switches of application
    \param[in] _param        regular expression for command line parameters
    \param[in] _iterations   default number of iterations
    \param[in] _subsets      default number of subsets
    \param[in] _regkey       registry key for logging path
    \param[in] _envvar       environment variable for logging path
    \param[in] _smoothing    name of image to which smoothing is applied
    \param[in] _sinogram     name of sinogram
    \param[in] _acf          name of acf

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
Parser::Parser(const std::string _prgname, const std::string _descr,
               const std::string _date, const std::string _switches,
               const std::string _param, const unsigned short int _iterations,
               const unsigned short int _subsets, const std::string _regkey,
               const std::string _envvar, const std::string _smoothing,
               const std::string _sinogram, const std::string _acf)
 { try
   { std::string mu;
     char c[2];

     prgname=_prgname;
     switches=_switches;
     descr=_descr;
     date=_date;
     param=_param;
     iterations=_iterations;
     subsets=_subsets;
     regkey=_regkey;
     envvar=_envvar;
     smoothing=_smoothing;
     sinogram=_sinogram;
     acf=_acf;
     v.tRhoSamples=0;
     v.tThetaSamples=0;
     v.tra_filename=std::string();
     v.bRhoSamples=0;
     v.bThetaSamples=0;
     v.blank_filename=std::string();
     v.aRhoSamples=0;
     v.aThetaSamples=0;
     v.acf_filename=std::string();
     v.oaRhoSamples=0;
     v.oaThetaSamples=0;
     v.oacf_filename=std::string();
     v.umap_filename=std::string();
     v.oumap_filename=std::string();
     v.eRhoSamples=0;
     v.eThetaSamples=0;
     v.sRhoSamples=0;
     v.sThetaSamples=0;
     v.emission_filename=std::string();
     v.oemission_filename=std::string();
     v.scatter_filename=std::string();
     v.oscatter_filename=std::string();
     v.nRhoSamples=0;
     v.nThetaSamples=0;
     v.norm_filename=std::string();
     v.umap_reco_model=40;
     v.alpha=1.5f;
     v.beta=3.0f;
     v.beta_ip=0.0f;
     v.iterations_ip=0;
     v.gaussian_smoothing_model=true;
     v.gauss_fwhm_xy=0.0f;
     v.gauss_fwhm_z=0.0f;
     v.boxcar_width=17;
     v.number_of_segments=4;
     v.mu_mean=new float[v.number_of_segments];
     v.mu_mean[0]=0.0f;
     v.mu_mean[1]=0.03f;
     v.mu_mean[2]=0.05f;
     v.mu_mean[3]=0.096f;
     v.mu_std=new float[v.number_of_segments];
     v.mu_std[0]=0.002f;
     v.mu_std[1]=0.06f;
     v.mu_std[2]=0.06f;
     v.mu_std[3]=0.02f;
     v.gauss_intersect=new float[v.number_of_segments-1];
     v.gauss_intersect[0]=0.015f;
     v.gauss_intersect[1]=0.04f;
     v.gauss_intersect[2]=0.065;
     v.umap_scalefactor=1.0f;
     v.iterations=iterations;
     v.subsets=subsets;
     v.rebin_r=1;
     v.rebin_t=1;
     v.kill_blocks.clear();
     v.mrd=0;
     v.span=0;
     v.matrix_num.clear();
     v.first_mnr=0;
     v.last_mnr=std::numeric_limits <unsigned short int>::max();
     v.irebin_method=SinogramConversion::iFORE_REB;
     v.view_mode=false;
     v.no_tilt=false;
     v.quality_control=false;
     v.quality_path=std::string();
     v.debug=false;
     v.debug_path=std::string();
     v.logcode=72;
     v.logpath=std::string();
     v.uzoom_xy_factor=1;
     v.uzoom_z_factor=1;
     v.randoms_smoothing=false;
     v.decay_corr=false;
     v.framelen_corr=false;
     v.rebin_method=SinogramConversion::NO_REB;
     v.scatter_corr=true;
     v.oscatter_2d=false;
     v.a_lim=0;
     v.w_lim=2;
     v.k_lim=2;
     v.iwidth=0;
     v.algorithm=ALGO::OSEM_UW_Algo;
     v.image_filename=std::string();
     v.oimage_filename=std::string();
     v.trim=0;
     v.txscatter[0]=v.txscatter[1]=0.0f;
     v.lber=-1.0f;
     v.txblr = 0.0f;
     v.gibbs_prior=0.0f;
     v.zoom_factor=1.0f;
     v.overlap=false;
     v.si.lld=0;
     v.si.uld=0;
     v.si.start_time=0.0f;
     v.si.frame_duration=0.0f;
     v.si.gate_duration=0.0f;
     v.si.bedpos=0.0f;
     v.si.halflife=0.0f;
     v.ucut=false;
     v.gap_filling=false;
     v.swap_path=std::string();
     v.scatter_skip=1;
     v.autoscale_umap=true;
     v.new_scatter_code=false;
     v.tof_processing=false;
     v.tof_bins=1;
     v.debug_iter=false;
     v.blank_db_path=std::string();
     v.blank_db_frames=0;
     v.trues=0.0;
     v.randoms=0.0;
     v.p39_patient_norm=false;
     v.apply_ecf=false;
     v.scatter_scale_min=0.0f;
     v.scatter_scale_max=0.0f;
     v.force=false;
     v.recoserver=false;
     v.asv=0.096f;
     v.append=false;
     v.pet_threshold=0.15f;
     v.pet_uvalue=0.096f;
     v.acf_threshold=1.03f;
     v.acf_threshold_margin = 0;
     v.norm_mask_bins=0;
     v.no_radial_arc=false;
     v.no_axial_arc=false;
     v.undo_attcor=false;
     v.memory_limit=0.8f;
     v.ct_filename=std::string();
     v.bed_position.clear();
     v.pguid=std::string();
     v.sguid=std::string();
     v.cguid=std::string();
     v.iguid.clear();
     v.ct2pet_offset.x=0.0f;
     v.ct2pet_offset.y=0.0f;
     v.ct2pet_offset.z=0.0f;
     v.patient_name=std::string();

     c[1]=0;
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
     c[0]=(char)181;
     mu=std::string(c);
#endif
#ifdef WIN32
     mu='u';
#endif
     while (descr.find('@') != std::string::npos)
      descr.replace(descr.find('@'), 1, mu);
   }
   catch (...)
    { if (v.mu_mean != NULL) { delete[] v.mu_mean;
                               v.mu_mean=NULL;
                             }
      if (v.mu_std != NULL) { delete[] v.mu_std;
                              v.mu_std=NULL;
                            }
      if (v.gauss_intersect != NULL) { delete[] v.gauss_intersect;
                                       v.gauss_intersect=NULL;
                                     }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
Parser::~Parser()
 { if (v.mu_mean != NULL) delete[] v.mu_mean;
   if (v.mu_std != NULL) delete[] v.mu_std;
   if (v.gauss_intersect != NULL) delete[] v.gauss_intersect;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert string into float.
    \param[in]  switch_str   command line switch
    \param[in]  param        string with integer
    \param[out] no_error     no error detected ?

    Convert string into float.
 */
/*---------------------------------------------------------------------------*/
float Parser::a2f(const std::string switch_str, const std::string param,
                  bool * const no_error) const
 { if (param.find(',') == std::string::npos) return(atof(param.c_str()));
   std::cerr << "Error (" << switch_str << "): too many parameters";
   *no_error=false;
   return(0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert string into integer.
    \param[in]  switch_str   command line switch
    \param[in]  param        string with integer
    \param[out] no_error     no error detected ?

    Convert string into integer.
 */
/*---------------------------------------------------------------------------*/
signed long int Parser::a2i(const std::string switch_str,
                            const std::string param, bool * const no_error)
 { if (param.find(',') == std::string::npos) return(atoi(param.c_str()));
   std::cerr << "Error (" << switch_str << "): too many parameters";
   *no_error=false;
   return(0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check expression for switch.
    \param[in] used_swi   list of used command line switches
    \param[in] swi        switch to check
    \param[in] rule       rule for switch
    \return expression fulfilled ?

    Check expression for switch. An expression doesn't contain brackets.
 */
/*---------------------------------------------------------------------------*/
bool Parser::checkExpression(const std::string used_swi, const std::string swi,
                             std::string expr) const
 { bool res=true;

   do { std::string s;
        bool not_swi=false;

        s=" ";
        for (unsigned short int i=0; i < expr.length(); i++)
         if ((expr[i] != '|') && (expr[i] != '&'))
          { s+=expr[i];
            if (i == expr.length()-1) expr.erase(0, i);
          }
          else { expr.erase(0, i);
                 break;
               }
        s+=" ";
        if (s == " 1 ") res=true;
         else if (s == " 0 ") res=false;
               else if (s[1] == '!')
                     { s.erase(1, 1);
                       res=(used_swi.find(s) == std::string::npos);
                       not_swi=true;
                     }
                     else res=(used_swi.find(s) != std::string::npos);
        if (!res && ((expr[0] == '&') || (expr.length() == s.length()-2)))
         { std::cerr << "Error: the switch \"-";
           if (swi.length() > 1) std::cerr << "-";
           std::cerr << swi << "\" ";
           if (not_swi) std::cerr << "is not allowed in combination with";
            else std::cerr << "requires";
           std::cerr << " the switch \"-";
           if (s.length() > 3) std::cerr << "-";
           std::cerr << s.substr(1, s.length()-2) << "\"." << std::endl;
           return(false);
         }
        if (res && (expr[0] == '|')) return(true);
        expr.erase(0, 1);
      } while (!expr.empty());
   return(res);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if file exist.
    \param[in] used_swi        list of used command line switches
    \param[in] swi             switch for filename
    \param[in] filename        name of file
    \param[in] file_required   is file required ?
    \param[in] filetype        type of file
    \return file is required and exists or file is not required and doesn't
            exist
 */
/*---------------------------------------------------------------------------*/
bool Parser::checkFile(const std::string used_swi, const std::string swi,
                       const std::string filename, const bool file_required,
                       const std::string filetype) const
 { if (used_swi.find(" "+swi+" ") == std::string::npos) return(true);
   if (filename.empty())
    { std::cerr << "Error: the name of the " << filetype << " ";
      if (file_required) std::cerr << "input";
       else std::cerr << "output";
      std::cerr << " file needs to be specified." << std::endl;
      return(false);
    }
   if (file_required && !FileExist(filename))
    { std::cerr << "Error: the " << filetype << " input file '" << filename
                << "' doesn't exist." << std::endl;
      return(false);
    }
   if (!file_required && FileExist(filename))
    if (v.force) unlink(filename.c_str());
     else { std::cerr << "Error: the " << filetype << " output file '"
                      << filename << "' exists." << std::endl;
            return(false);
          }
   return(true);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if path exist.
    \param[in] used_swi   list of used command line switches
    \param[in] swi        switch for pathname
    \param[in] pathname   name of path
    \param[in] pathtype   type of path
    \return file is required and exists or file is not required and doesn't
            exist
 */
/*---------------------------------------------------------------------------*/
bool Parser::checkPath(const std::string used_swi, const std::string swi,
                       const std::string pathname,
                       const std::string pathtype) const
 { if (used_swi.find(" "+swi+" ") == std::string::npos) return(true);
   if (pathname.empty())
    { std::cerr << "Error: the name of the " << pathtype << " directory needs "
                   "to be specified." << std::endl;
      return(false);
    }
   if (!PathExist(pathname))
    { std::cerr << "The " << pathtype << " directory '" << pathname << "' "
                   "doesn't exist." << std::endl;
      return(false);
    }
   return(true);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check rule for switch.
    \param[in] used_swi   list of used command line switches
    \param[in] swi        switch to check
    \param[in] rule       rule for switch
    \return rule fulfilled ?

    Check rule for switch. The rule is separated into expression that don't
    contain brackets. Expressions are evaluated and replaced by '1' (true) or
    '0' (false).
 */
/*---------------------------------------------------------------------------*/
bool Parser::checkRule(const std::string used_swi, const std::string swi,
                       std::string rule) const
 { for (;;)
    { bool found=false;

      for (unsigned short int i=0; i < rule.length(); i++)
       if (found) break;
        else if (rule[i] == '(')
              for (unsigned short int j=i+1; j < rule.length(); j++)
               if (rule[j] == ')')
                { std::string expr, result;

                  expr=rule.substr(i+1, j-i-1);
                  if (checkExpression(used_swi, swi, expr)) result="1";
                   else result="0";
                  rule.replace(i, j-i+1, result);
                  found=true;
                  break;
                }
                else if (rule[j] == '(') break;
      if (rule.find("(") == std::string::npos)
       return(checkExpression(used_swi, swi, rule));
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check for illegal switch combinations.
    \param[in] switch_str   list of used command line switches
    \return alles rules fulfilled ?

    Check for illegal switch combinations.
 */
/*---------------------------------------------------------------------------*/
bool Parser::checkSemantics(std::string switch_str) const
 { unsigned short int num_rules;
   std::string used_swi, mu, double_swi;
   bool ret=true;
   { char c[2];

     c[1]=0;
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
     c[0]=(char)181;
     mu=std::string(c);
#endif
#ifdef WIN32
     mu='u';
#endif
   }

   double_swi=" ";
   used_swi=" "+switch_str;
   num_rules=sizeof(rules)/sizeof(truleset);
   do { std::string::size_type p;
        std::string swi;

        if ((p=switch_str.find(' ')) == std::string::npos) break;
        swi=switch_str.substr(0, p);
        switch_str.erase(0, p+1);
                                      // check is switch is used more than once
        if ((p=used_swi.find(" "+swi+" ")) != std::string::npos)
         if ((used_swi.substr(p+1).find(" "+swi+" ")) != std::string::npos)
          if (double_swi.find(" "+swi+" ") == std::string::npos)
           { std::cerr << "Error: the switch \"-";
             if (swi.length() > 1) std::cerr << "-";
             std::cerr << swi << "\" is used more than once." << std::endl;
             ret=false;
             double_swi+=swi+" ";
           }
                                                       // check rule for switch
        for (unsigned short int i=0; i < num_rules; i++)
         if (rules[i].cmd_switch == swi)
          if (!checkRule(used_swi, swi, rules[i].rule))
           { ret=false;
             std::cerr << "Error: the switch \"-";
             if (swi.length() > 1) std::cerr << "-";
             std::cerr << swi << "\" is not allowed in this context."
                       << std::endl;
           }
      } while (!switch_str.empty());
                                                             // check filenames
   ret&=checkFile(used_swi, "a", v.acf_filename, true, "acf");
   ret&=checkFile(used_swi, "aarc", v.acf_filename, true, "acf");
   ret&=checkFile(used_swi, "anarc", v.acf_filename, true, "acf");
   ret&=checkFile(used_swi, "b", v.blank_filename, true, "blank");
   ret&=checkFile(used_swi, "barc", v.blank_filename, true, "blank");
   ret&=checkFile(used_swi, "bnarc", v.blank_filename, true, "blank");
   ret&=checkFile(used_swi, "c", v.ct_filename, true, "CT-image");
   ret&=checkFile(used_swi, "e", v.emission_filename, true, "emission");
   ret&=checkFile(used_swi, "earc", v.emission_filename, true, "emission");
   ret&=checkFile(used_swi, "enarc", v.emission_filename, true, "emission");
   ret&=checkFile(used_swi, "i", v.image_filename, true, "image");
   ret&=checkFile(used_swi, "n", v.norm_filename, true, "norm");
   if (!v.append)
    { ret&=checkFile(used_swi, "oa", v.oacf_filename, false, "acf");
      ret&=checkFile(used_swi, "oaarc", v.oacf_filename, false, "acf");
      ret&=checkFile(used_swi, "oe", v.oemission_filename, false, "emission");
      ret&=checkFile(used_swi, "oearc", v.oemission_filename, false,
                     "emission");
      ret&=checkFile(used_swi, "oi", v.oimage_filename, false, "image");
      ret&=checkFile(used_swi, "os", v.oscatter_filename, false, "scatter");
      ret&=checkFile(used_swi, "osarc", v.oscatter_filename, false, "scatter");
      ret&=checkFile(used_swi, "ou", v.oumap_filename, false, mu+"-map");
    }
   ret&=checkFile(used_swi, "s", v.scatter_filename, true, "scatter");
   ret&=checkFile(used_swi, "sarc", v.scatter_filename, true, "scatter");
   ret&=checkFile(used_swi, "snarc", v.scatter_filename, true, "scatter");
   ret&=checkFile(used_swi, "t", v.tra_filename, true, "transmission");
   ret&=checkFile(used_swi, "tarc", v.tra_filename, true, "transmission");
   ret&=checkFile(used_swi, "tnarc", v.tra_filename, true, "transmission");
   ret&=checkFile(used_swi, "u", v.umap_filename, true, mu+"-map");
                                                            // check path names
   ret&=checkPath(used_swi, "d", v.debug_path, "debug");
   if (!v.logpath.empty())
    ret&=checkPath(used_swi, "l", v.logpath, "logging");
   ret&=checkPath(used_swi, "q", v.quality_path, "quality control");
   ret&=checkPath(used_swi, "swap", v.swap_path, "swapping");
   return(ret);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Replace '\' in a string by '/'.
    \param[in] str   string with '\'
    \return string with '/'

    Replace '\' in a string by '/'.
 */
/*---------------------------------------------------------------------------*/
std::string Parser::convertSlash(std::string str) const
 { while (str.find("\\") != std::string::npos)
    str.replace(str.find("\\"), 1, "/");
   return(str);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request structure with command line parameters.
    \return structure with command line parameters

    Request structure with command line parameters.
*/
/*---------------------------------------------------------------------------*/
Parser::tparam *Parser::params() const
 { return((Parser::tparam *)&v);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse command line arguments.
    \param[in] argc   number of command line arguments
    \param[in] argv   list of command line arguments
    \return error during parsing ?

    Parse command line arguments and fill parameter data structure.
 */
/*---------------------------------------------------------------------------*/
bool Parser::parse(int argc, char **argv)
 { try
   { char c=0;
     int optindex;
     bool no_error=true;
     std::string mu, short_options="a:b:c:d:e:hi:k:l:n:p:q:R:r:s:t:u:vw:?",
                 switch_str;

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
     static struct option long_options[]={ {   "gxy", 1, 0, 0 },
                                           {    "gz", 1, 0, 0 },
                                           {    "is", 1, 0, 0 },
                                           {   "mat", 1, 0, 0 },
                                           { "model", 1, 0, 0 },
                                           {   "mrd", 1, 0, 0 },
                                           { "ntilt", 0, 0, 0 },
                                           {   "prj", 1, 0, 0 },
                                           {  "span", 1, 0, 0 },
                                           { "ualgo", 1, 0, 0 },
                                           {    "uf", 1, 0, 0 },
                                           {  "view", 0, 0, 0 },
                                           {  "aarc", 1, 0, 0 },
                                           { "anarc", 1, 0, 0 },
                                           {  "barc", 1, 0, 0 },
                                           { "bnarc", 1, 0, 0 },
                                           {  "tarc", 1, 0, 0 },
                                           { "tnarc", 1, 0, 0 },
                                           { "uzoom", 1, 0, 0 },
                                           {  "earc", 1, 0, 0 },
                                           { "enarc", 1, 0, 0 },
                                           {    "oe", 1, 0, 0 },
                                           { "oearc", 1, 0, 0 },
                                           {    "oa", 1, 0, 0 },
                                           { "oaarc", 1, 0, 0 },
                                           {    "ou", 1, 0, 0 },
                                           {    "os", 1, 0, 0 },
                                           { "osarc", 1, 0, 0 },
                                           {    "rs", 0, 0, 0 },
                                           {    "dc", 0, 0, 0 },
                                           {    "fl", 0, 0, 0 },
                                           {    "bc", 1, 0, 0 },
                                           {  "scan", 1, 0, 0 },
                                           {  "nosc", 0, 0, 0 },
                                           {  "ucut", 0, 0, 0 },
                                           {    "oi", 1, 0, 0 },
                                           {  "algo", 1, 0, 0 },
                                           {  "trim", 1, 0, 0 },
                                           { "gibbs", 1, 0, 0 },
                                           {  "zoom", 1, 0, 0 },
                                           {    "ol", 0, 0, 0 },
                                           {    "gf", 0, 0, 0 },
                                           {  "swap", 1, 0, 0 },
                                           {  "skip", 1, 0, 0 },
                                           {   "aus", 0, 0, 0 },
                                           { "newsc", 0, 0, 0 },
                                           {   "tof", 0, 0, 0 },
                                           {  "tofp", 1, 0, 0 },
                                           {    "d2", 0, 0, 0 },
                                           {  "dttx", 1, 0, 0 },
                                           { "noise", 1, 0, 0 },
                                           {    "np", 1, 0, 0 },
                                           {   "ecf", 0, 0, 0 },
                                           {   "ssf", 1, 0, 0 },
                                           { "force", 0, 0, 0 },
                                           { "resrv", 1, 0, 0 },
                                           {   "asv", 1, 0, 0 },
                                           {   "app", 0, 0, 0 },
                                           {   "thr", 1, 0, 0 },
                                           {  "athr", 1, 0, 0 },
                                           {   "nbm", 1, 0, 0 },
                                           {  "sarc", 1, 0, 0 },
                                           { "snarc", 1, 0, 0 },
                                           { "naarc", 0, 0, 0 },
                                           { "nrarc", 0, 0, 0 },
                                           {   "uac", 0, 0, 0 },
                                           {   "mem", 1, 0, 0 },
                                           {    "bp", 1, 0, 0 },
                                           { "pguid", 1, 0, 0 },
                                           { "sguid", 1, 0, 0 },
                                           { "iguid", 1, 0, 0 },
                                           {  "offs", 1, 0, 0 },
                                           {    "pn", 1, 0, 0 },
                                           { "cguid", 1, 0, 0 },
                                           { "os2d",  0, 0, 0 },
                                           { "txsc",  1, 0, 0 },
                                           { "lber",  1, 0, 0 },
                                           { "txblr",  1, 0, 0 },
                                           {       0, 0, 0, 0 }
                                         };

     switch_str=std::string();
     v.segmentation_params_set=false;
     if (argc > 1)                                       // parse next argument
      while ((c=getopt_long(argc, argv, short_options.c_str(), long_options,
                            &optindex)) >= 0)
       { bool no_param;
         std::string optstr;

         no_param=false;
         if (c == 0)                                             // long option
          {                                                 // unknown switch ?
            if (switches.find(long_options[optindex].name) ==
                std::string::npos)
             { std::cerr << "Error (--" << long_options[optindex].name
                         << "): invalid switch";
               no_error=false;
               break;
             }
                               // argument for switch that shouldn't have one ?
            if ((long_options[optindex].has_arg == 0) &&
                ((optarg != NULL) && (optarg[0] != 0) &&
                 ((optarg[0] != '-') ||
                  ((optarg[1] >= '0') && (optarg[1] <= '9')))))
             { std::cerr << "Error (--" << long_options[optindex].name
                         << "): invalid parameter";
               no_error=false;
               break;
             }
                               // no argument for switch that should have one ?
            if ((long_options[optindex].has_arg == 1) &&
                ((optarg == NULL) || (optarg[0] == 0) ||
                 ((optarg[0] == '-') && (optarg[1] < '0') &&
                  (optarg[1] > '9'))))
             { std::cerr << "Error (--" << long_options[optindex].name
                         << "): ";
               no_param=true;
               no_error=false;
             }
            switch_str+=std::string(long_options[optindex].name)+" ";
          }
          else { char s[2];
                 std::string str;
                                                                // short option
                 s[0]=c;
                 s[1]=0;
                 str=std::string(s);
                                                            // unknown switch ?
                 if ((switches.find(" "+str+" ") == std::string::npos) &&
                     (switches.substr(0, 2) != str+" ") &&
                     (switches.substr(switches.length()-2, 2) != " "+str))
                  { std::cerr << "Error (-" << str << "): invalid switch";
                    no_error=false;
                    break;
                  }
                               // argument for switch that shouldn't have one ?
                 if ((short_options.find(str+":") == std::string::npos) &&
                     ((optarg != NULL) && (optarg[0] != 0) &&
                      ((optarg[0] != '-') ||
                       ((optarg[1] >= '0') && (optarg[1] <= '9')))))
                  { std::cerr << "Error (-" << str << "): invalid parameter";
                    no_error=false;
                    break;
                  }
                               // no argument for switch that should have one ?
                 if ((short_options.find(str+":") != std::string::npos) &&
                     ((optarg == NULL) || (optarg[0] == 0) ||
                      ((optarg[0] == '-') && (optarg[1] < '0') &&
                       (optarg[1] > '9'))))
                  { std::cerr << "Error (-" << str << "): invalid parameter";
                    no_error=false;
                    break;
                  }
                 switch_str+=str+" ";
               }
         if (optarg == NULL) optstr=std::string();
          else optstr=std::string(optarg);
                                                                // parse switch
         switch (c)
          { case 0:
             switch (optindex)
              { case 0:      // gxy: width of gaussian filter for x/y-direction
                 if (no_param) std::cerr << "filter width missing";
                  else { v.gauss_fwhm_xy=a2f("--gxy", optstr, &no_error);
                         if (v.gauss_fwhm_xy < 0.0f)
                          { std::cerr << "Error (--gxy): filter width is "
                                         "negative";
                            no_error=false;
                          }
                       }
                 break;
                case 1:         // gz: width of gaussian filter for z-direction
                 if (no_param) std::cerr << "filter width missing";
                  else { v.gauss_fwhm_z=a2f("--gz", optstr, &no_error);
                         if (v.gauss_fwhm_z < 0.0f)
                          { std::cerr << "Error (--z): filter width is "
                                         "negative";
                            no_error=false;
                          }
                       }
                 break;
                case 2:                 // is: number of iterations and subsets
                 if (no_param)
                  std::cerr << "number of iterations and subsets missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--is): wrong format";
                            no_error=false;
                          }
                          else { v.iterations=a2i("--is", optstr.substr(0, p),
                                                  &no_error);
                                 v.subsets=a2i("--is", optstr.substr(p+1),
                                               &no_error);
                                 if (v.iterations < 1)
                                  { std::cerr << "Error (--is): number of "
                                                "iterations is smaller than 1";
                                    no_error=false;
                                    break;
                                  }
                                 if (v.subsets < 1)
                                  { std::cerr << "Error (--is): number of "
                                                 "subsets is smaller than 1";
                                    no_error=false;
                                  }
                               }
                       }
                 break;
                case 3:                                // mat: matrix number(s)
                 if (no_param) std::cerr << "matrix number missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) != std::string::npos)
                          { v.matrix_num.clear();
                                                          // get matrix numbers
                            while ((p=optstr.find(',')) != std::string::npos)
                             { v.matrix_num.push_back(
                                 a2i("--mat", optstr.substr(0, p), &no_error));
                               optstr.erase(0, p+1);
                             }
                            v.matrix_num.push_back(a2i("--mat", optstr,
                                                       &no_error));
                            if (v.matrix_num[0] > 1000)
                             { std::cerr << "Error (--mat): matrix number is "
                                            "wrong";
                               no_error=false;
                               break;
                             }
                            for (unsigned short int i=1;
                                 i < v.matrix_num.size();
                                 i++)
                             if (v.matrix_num[i] < v.matrix_num[i-1])
                              { std::cerr << "Error (--mat): matrix number is "
                                             "wrong";
                                no_error=false;
                                break;
                              }
                            break;
                          }
                         if ((p=optstr.find('-')) != std::string::npos)
                          { v.first_mnr=a2i("--mat", optstr.substr(0, p),
                                            &no_error);
                            v.last_mnr=a2i("--mat", optstr.substr(p+1,
                                                           optstr.length()-p),
                                           &no_error);
                          }
                          else { v.first_mnr=a2i("--mat", optstr, &no_error);
                                 v.last_mnr=v.first_mnr;
                               }
                         if ((v.first_mnr > 1000) ||
                             (v.last_mnr < v.first_mnr))
                          { std::cerr << "Error (--mat): number of first or "
                                         "last matrix is wrong";
                            no_error=false;
                          }
                       }
                 break;
                case 4:                      // model: gantry model information
                 if (no_param) { std::cerr << "geometry description missing";
                                 break;
                               }
                 { std::string::size_type p;
                  if ((p=optstr.find(',')) == std::string::npos)
                    { 
#ifdef USE_GANTRY_MODEL
                      CGantryModel model;
                      GM::gm()->init(optstr);
                      if (!model.setModel((const char *)optstr.c_str()))
                       { std::cerr << "Error (--model): unknown gantry model "
                                      "number";
                         no_error=false;
                         break;
                       }
                      if (v.si.lld == 0) v.si.lld=model.defaultLLD();
                      if (v.si.uld == 0) v.si.uld=model.defaultULD();
#else
                      GM::gm()->init(optstr);
                      if (v.si.lld == 0) v.si.lld=GantryInfo::defaultLLD();
                      if (v.si.uld == 0) v.si.uld=GantryInfo::defaultULD();
#endif
                      break;
                    }
                   bool ring_architecture=false, is_pet_ct=false,
                        needs_axial_arc_correction=false;
                   float doi, ring_radius, intrinsic_tilt, plane_separation,
                         BinSizeRho, max_scatter_zfov;
                   unsigned short int crystals_per_ring, bucket_rings,
                                      transaxial_crystals_per_block,
                                      transaxial_blocks_per_bucket,
                                      axial_crystals_per_block,
                                      axial_blocks_per_bucket,
                                      transaxial_block_gaps, axial_block_gaps,
                                      buckets_per_ring, RhoSamples,
                                      ThetaSamples, span, mrd, crystal_layers,
                                      lld, uld, rings;
                   std::vector <GM::tcrystal_mat> crystal_layer_material;
                   std::vector <float> crystal_layer_depth,
                                       crystal_layer_fwhm_ergres,
                                       crystal_layer_background_ergratio;

                   RhoSamples=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   BinSizeRho=a2f("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   ThetaSamples=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   rings=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   crystals_per_ring=a2i("--model", optstr.substr(0, p),
                                         &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   plane_separation=a2f("--model", optstr.substr(0, p),
                                        &no_error)/2.0f;
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   span=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   mrd=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   ring_radius=a2f("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   doi=a2f("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   intrinsic_tilt=a2f("--model", optstr.substr(0, p),
                                      &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   transaxial_crystals_per_block=a2i("--model",
                                                     optstr.substr(0, p),
                                                     &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   axial_crystals_per_block=a2i("--model",
                                                optstr.substr(0, p),
                                                &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   transaxial_blocks_per_bucket=a2i("--model",
                                                    optstr.substr(0, p),
                                                    &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   axial_blocks_per_bucket=a2i("--model",
                                               optstr.substr(0, p),
                                               &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   bucket_rings=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   buckets_per_ring=a2i("--model", optstr.substr(0, p),
                                        &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   transaxial_block_gaps=a2i("--model",
                                             optstr.substr(0, p),
                                             &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   axial_block_gaps=a2i("--model", optstr.substr(0, p),
                                        &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   if (optstr.substr(0, p) == "1") ring_architecture=true;
                    else if (optstr.substr(0, p) == "0")
                          ring_architecture=false;
                    else { std::cerr << "Error (--model): format of gantry "
                                        "description is wrong";
                           no_error=false;
                         }
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   if (optstr.substr(0, p) == "1") is_pet_ct=false;
                    else if (optstr.substr(0, p) == "0") is_pet_ct=true;
                    else { std::cerr << "Error (--model): format of gantry "
                                        "description is wrong";
                           no_error=false;
                         }
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   if (optstr.substr(0, p) == "1")
                    needs_axial_arc_correction=true;
                    else if (optstr.substr(0, p) == "0")
                          needs_axial_arc_correction=false;
                    else { std::cerr << "Error (--model): format of gantry "
                                        "description is wrong";
                           no_error=false;
                         }
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   lld=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   uld=a2i("--model", optstr.substr(0, p), &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   max_scatter_zfov=a2f("--model", optstr.substr(0, p),
                                        &no_error);
                   optstr.erase(0, p+1);
                   if ((p=optstr.find(',')) == std::string::npos)
                    { std::cerr << "Error (--model): format of gantry "
                                   "description is wrong";
                      no_error=false;
                      break;
                    }
                   crystal_layers=a2i("--model", optstr.substr(0, p),
                                      &no_error);
                   optstr.erase(0, p+1);
                   crystal_layer_material.resize(crystal_layers);
                   crystal_layer_depth.resize(crystal_layers);
                   crystal_layer_fwhm_ergres.resize(crystal_layers);
                   crystal_layer_background_ergratio.resize(crystal_layers);
                   for (unsigned short int i=0; i < crystal_layers; i++)
                    { std::string material, s;

                      if ((p=optstr.find(',')) == std::string::npos)
                       { std::cerr << "Error (--model): format of gantry "
                                      "description is wrong";
                         no_error=false;
                         break;
                       }
                      material=optstr.substr(0, p);
                      if (material == "LSO") crystal_layer_material[i]=GM::LSO;
                       else if (material == "BGO")
                             crystal_layer_material[i]=GM::BGO;
                       else if (material == "NaI")
                             crystal_layer_material[i]=GM::NAI;
                       else if (material == "LYSO")
                             crystal_layer_material[i]=GM::LYSO;
                       else { std::cerr << "Error (--model): format of gantry "
                                           "description is wrong";
                              no_error=false;
                            }
                      optstr.erase(0, p+1);
                      if ((p=optstr.find(',')) == std::string::npos)
                       { std::cerr << "Error (--model): format of gantry "
                                      "description is wrong";
                         no_error=false;
                         break;
                       }
                      crystal_layer_depth[i]=a2f("--model",
                                                 optstr.substr(0, p),
                                                 &no_error);
                      optstr.erase(0, p+1);
                      if ((p=optstr.find(',')) == std::string::npos)
                       { std::cerr << "Error (--model): format of gantry "
                                      "description is wrong";
                         no_error=false;
                         break;
                       }
                      crystal_layer_fwhm_ergres[i]=a2f("--model",
                                                       optstr.substr(0, p),
                                                       &no_error);
                      optstr.erase(0, p+1);
                      if (i == crystal_layers-1)
                       { if ((p=optstr.find(',')) != std::string::npos)
                          { std::cerr << "Error (--model): format of gantry "
                                         "description is wrong";
                            no_error=false;
                            break;
                          }
                         s=optstr;
                       }
                       else if ((p=optstr.find(',')) == std::string::npos)
                             { std::cerr << "Error (--model): format of "
                                            "gantry description is wrong";
                               no_error=false;
                               break;
                             }
                             else { s=optstr.substr(0, p);
                                    optstr.erase(0, p+1);
                                  }
                      crystal_layer_background_ergratio[i]=a2f("--model", s,
                                                               &no_error);
                    }
                   if (!no_error) break;
                   GM::gm()->init(RhoSamples, ThetaSamples, BinSizeRho, rings,
                                  plane_separation, ring_radius, doi, span,
                                  mrd, intrinsic_tilt,
                                  transaxial_crystals_per_block,
                                  transaxial_blocks_per_bucket,
                                  axial_crystals_per_block,
                                  axial_blocks_per_bucket,
                                  transaxial_block_gaps, axial_block_gaps,
                                  bucket_rings, buckets_per_ring,
                                  crystals_per_ring, crystal_layers,
                                  crystal_layer_material, crystal_layer_depth,
                                  crystal_layer_fwhm_ergres,
                                  crystal_layer_background_ergratio,
                                  max_scatter_zfov, ring_architecture,
                                  is_pet_ct, needs_axial_arc_correction, lld,
                                  uld);
                 }
                 break;
                case 5:                         // mrd: maximum ring difference
                 if (no_param) std::cerr << "maximum ring difference missing";
                  else v.mrd=a2i("--mrd", optstr, &no_error);
                 break;
                case 6:         // ntilt: store sinogram without intrinsic tilt
                 v.no_tilt=true;
                 break;
                case 7:              // prj: rebinning algorithm for sino2d->3d
                 if (no_param) std::cerr << "projection algorithm missing";
                  else if (optstr == "none")
                        v.irebin_method=SinogramConversion::iNO_REB;
                        else if (optstr == "issrb")
                              v.irebin_method=SinogramConversion::iSSRB_REB;
                        else if (optstr == "ifore")
                              v.irebin_method=SinogramConversion::iFORE_REB;
                        else if (optstr == "fwdpj")
                              v.irebin_method=SinogramConversion::FWDPJ;
                        else { std::cerr << "Error (--prj): algorithm \""
                                         << optstr << "\" unknown";
                               no_error=false;
                             }
                 break;
                case 8:                                           // span: span
                 if (no_param) std::cerr << "span missing";
                  else v.span=atoi(optarg);
                 break;
                case 9:               // ualgo: reconstruction method for u-map
                 if (no_param)
                  { std::cerr << "number of reconstruction model missing";
                    no_error=false;
                  }
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { v.umap_reco_model=a2i("--ualgo", optstr,
                                                  &no_error);
                            switch (v.umap_reco_model)
                             { case 1:                               // UW-OSEM
                                v.alpha=0.0f;
                                v.beta=0.0f;
                                v.beta_ip=0.0f;
                                v.iterations_ip=0;
                                break;
                               case 20:             // CG with smoothness and
                                                    // gaussian smoothing model
                                v.alpha=1.5f;
                                v.beta=0.00025f;
                                v.beta_ip=0.0f;
                                v.iterations_ip=0;
                                v.gaussian_smoothing_model=true;
                                break;
                               case 21:        // CG with smoothness and
                                               // Geman-McClure smoothing model
                                v.alpha=1.5f;
                                v.beta=0.00025f;
                                v.beta_ip=0.0f;
                                v.iterations_ip=0;
                                v.gaussian_smoothing_model=false;
                                break;
                               case 30:             // full model with
                                                    // gaussian smoothing model
                                v.alpha=1.5f;
                                v.beta=1.0f;
                                v.beta_ip=0.05f;
                                v.iterations_ip=5;
                                v.gaussian_smoothing_model=true;
                                break;
                               case 31:        // full model with
                                               // Geman-McClure smoothing model
                                v.alpha=1.5f;
                                v.beta=1.0f;
                                v.beta_ip=0.05f;
                                v.iterations_ip=5;
                                v.gaussian_smoothing_model=false;
                                break;
                               default:
                                std::cerr << "Error (--ualgo): unknown "
                                             "reconstruction model";
                                no_error=false;
                                break;
                             }
                          }
                          else {                          // user defined model
                                 v.umap_reco_model=a2i("--ualgo",
                                                       optstr.substr(0, p),
                                                       &no_error);
                                 if ((v.umap_reco_model != 40) &&
                                     (v.umap_reco_model != 41))
                                  { std::cerr << "Error (--ualgo): wrong "
                                                 "reconstruction model";
                                    no_error=false;
                                    break;
                                  }
                                 optstr.erase(0, p+1);
                                 if ((p=optstr.find(',')) == std::string::npos)
                                  { std::cerr << "Error (--ualgo): wrong "
                                                 "format";
                                    no_error=false;
                                    break;
                                  }
                                 v.alpha=a2f("--ualgo", optstr.substr(0, p),
                                             &no_error);
                                 optstr.erase(0, p+1);
                                 if ((p=optstr.find(',')) == std::string::npos)
                                  { std::cerr << "Error (--ualgo): wrong "
                                                 "format";
                                    no_error=false;
                                    break;
                                  }
                                 v.beta=a2f("--ualgo", optstr.substr(0, p),
                                            &no_error);
                                 optstr.erase(0, p+1);
                                 if ((p=optstr.find(',')) == std::string::npos)
                                  { std::cerr << "Error (--ualgo): wrong "
                                                 "format";
                                    no_error=false;
                                    break;
                                  }
                                 v.beta_ip=a2f("--ualgo", optstr.substr(0, p),
                                               &no_error);
                                 v.iterations_ip=a2i("--ualgo",
                                                     optstr.substr(p+1),
                                                     &no_error);
                                 v.gaussian_smoothing_model=
                                                     (v.umap_reco_model == 40);
                               }
                       }
                 break;
                case 10:                        // uf: scaling factor for u-map
                 if (no_param) std::cerr << "scaling factor missing";
                  else { v.umap_scalefactor=a2f("--uf", optstr, &no_error);
                         if (v.umap_scalefactor <= 0.0f)
                          { std::cerr << "Error (--uf): scaling factor is non-"
                                         "positive";
                            no_error=false;
                          }
                       }
                 break;
                case 11:                  // view: store sinograms in view mode
                 v.view_mode=true;
                 break;
                case 12:                             // aarc: arc-corrected acf
                 if (no_param) std::cerr << "acf input filename is missing";
                  else { parseFilename(optstr, &(v.acf_filename), "--aarc",
                                       &no_error, &(v.aRhoSamples),
                                       &(v.aThetaSamples), &(v.adim));
                         v.acf_arc_corrected=true;
                         v.acf_arc_corrected_overwrite=true;
                       }
                 break;
                case 13:                          // anarc: unarc-corrected acf
                 if (no_param) std::cerr << "acf input filename is missing";
                  else { parseFilename(optstr, &(v.acf_filename), "--anarc",
                                       &no_error, &(v.aRhoSamples),
                                       &(v.aThetaSamples), &(v.adim));
                         v.acf_arc_corrected=false;
                         v.acf_arc_corrected_overwrite=true;
                       }
                 break;
                case 14:                           // barc: arc-corrected blank
                 if (no_param) std::cerr << "blank input filename is missing";
                  else { parseFilename(optstr, &(v.blank_filename), "--barc",
                                       &no_error, &(v.bRhoSamples),
                                       &(v.bThetaSamples), NULL);
                         v.blank_arc_corrected=true;
                         v.blank_arc_corrected_overwrite=true;
                       }
                 break;
                case 15:                        // bnarc: unarc-corrected blank
                 if (no_param) std::cerr << "blank input filename is missing";
                  else { parseFilename(optstr, &(v.blank_filename), "--bnarc",
                                       &no_error, &(v.bRhoSamples),
                                       &(v.bThetaSamples), NULL);
                         v.blank_arc_corrected=false;
                         v.blank_arc_corrected_overwrite=true;
                       }
                 break;
                case 16:                    // tarc: arc-corrected transmission
                 if (no_param)
                  std::cerr << "transmission input filename is missing";
                  else { parseFilename(optstr, &(v.tra_filename), "--tarc",
                                       &no_error, &(v.tRhoSamples),
                                       &(v.tThetaSamples), NULL);
                         v.tra_arc_corrected=true;
                         v.tra_arc_corrected_overwrite=true;
                       }
                 break;
                case 17:                 // tnarc: unarc-corrected transmission
                 if (no_param)
                  std::cerr << "transmission input filename is missing";
                  else { parseFilename(optstr, &(v.tra_filename), "--tnarc",
                                       &no_error, &(v.tRhoSamples),
                                       &(v.tThetaSamples), NULL);
                         v.tra_arc_corrected=false;
                         v.tra_arc_corrected_overwrite=true;
                       }
                 break;
                case 18:            // uzoom: zoom factor for u-map calculation
                 if (no_param) std::cerr << "zoom factor missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--uzoom): wrong format";
                            no_error=false;
                          }
                          else { v.uzoom_xy_factor=a2i("--uzoom",
                                                       optstr.substr(0, p),
                                                       &no_error);
                                 if ((v.uzoom_xy_factor != 1) &&
                                     (v.uzoom_xy_factor != 2) &&
                                     (v.uzoom_xy_factor != 4))
                                  { std::cerr << "Error (--uzoom): x/y-zoom "
                                                 "factor invalid";
                                    no_error=false;
                                  }
                                 v.uzoom_z_factor=a2i("--uzoom",
                                                      optstr.substr(p+1),
                                                      &no_error);
                                 if ((v.uzoom_z_factor < 1) ||
                                     (v.uzoom_z_factor > 4))
                                  { std::cerr << "Error (--uzoom): z-zoom "
                                                 "factor invalid";
                                    no_error=false;
                                  }
                               }
                       }
                 break;
                case 19:                        // earc: arc-corrected emission
                 if (no_param)
                  std::cerr << "emission input filename is missing";
                  else { parseFilename(optstr, &(v.emission_filename),
                                       "--earc", &no_error, &(v.eRhoSamples),
                                       &(v.eThetaSamples), &(v.edim));
                         v.emission_arc_corrected=true;
                         v.emission_arc_corrected_overwrite=true;
                       }
                 break;
                case 20:                     // enarc: unarc-corrected emission
                 if (no_param)
                  std::cerr << "emission input filename is missing";
                  else { parseFilename(optstr, &(v.emission_filename),
                                       "--enarc", &no_error, &(v.eRhoSamples),
                                       &(v.eThetaSamples), &(v.edim));
                         v.emission_arc_corrected=false;
                         v.emission_arc_corrected_overwrite=true;
                       }
                 break;
                case 21:                        // oe: unarc-corrected emission
                 if (no_param)
                  std::cerr << "emission output filename is missing";
                  else { parseFilename(optstr, &(v.oemission_filename), "--oe",
                                       &no_error, NULL, NULL, NULL);
                         v.oemission_arc_corrected=false;
                       }
                 break;
                case 22:                       // oearc: arc-corrected emission
                 if (no_param)
                  std::cerr << "emission output filename is missing";
                  else { parseFilename(optstr, &(v.oemission_filename),
                                       "--oearc", &no_error, NULL, NULL, NULL);
                         v.oemission_arc_corrected=true;
                       }
                 break;
                case 23:                             // oa: unarc-corrected acf
                 if (no_param) std::cerr << "acf output filename is missing";
                  else { parseFilename(optstr, &(v.oacf_filename), "--oa",
                                       &no_error, &(v.oaRhoSamples),
                                       &(v.oaThetaSamples), NULL);
                         v.oacf_arc_corrected=false;
                       }
                 break;
                case 24:                            // oaarc: arc-corrected acf
                 if (no_param) std::cerr << "acf output filename is missing";
                  else { parseFilename(optstr, &(v.oacf_filename), "--oaarc",
                                       &no_error, &(v.oaRhoSamples),
                                       &(v.oaThetaSamples), NULL);
                         v.oacf_arc_corrected=true;
                       }
                 break;
                case 25:                                  // ou: u-map filename
                 if (no_param) std::cerr << mu << "-map filename missing";
                  else if (optstr.find(',') == std::string::npos)
                        v.oumap_filename=optstr;
                        else { std::cerr << "Error (--ou): too many "
                                            "parameters";
                               no_error=false;
                             }
                 break;
                case 26:                         // os: unarc-corrected scatter
                 if (no_param)
                  std::cerr << "scatter output filename is missing";
                  else { parseFilename(optstr, &(v.oscatter_filename), "--os",
                                       &no_error, NULL, NULL, NULL);
                         v.oscatter_arc_corrected=false;
                       }
                 break;
                case 27:                        // osarc: arc-corrected scatter
                 if (no_param)
                  std::cerr << "scatter output filename is missing";
                  else { parseFilename(optstr, &(v.oscatter_filename),
                                       "--osarc", &no_error, NULL, NULL, NULL);
                         v.oscatter_arc_corrected=true;
                       }
                 break;
                case 28:                               // rs: randoms smoothing
                 v.randoms_smoothing=true;
                 break;
                case 29:                                // dc: decay correction
                 v.decay_corr=true;
                 break;
                case 30:                         // fl: frame-length correction
                 v.framelen_corr=true;
                 break;
                case 31:      // bc: width of boxcar filter for scatter scaling
                 if (no_param) std::cerr << "filter width missing";
                  else { v.boxcar_width=a2i("--bc", optstr, &no_error);
                         if ((v.boxcar_width & 0x1) == 0)
                          { std::cerr << "Error (--bc): filter width is not an"
                                         " odd number";
                            no_error=false;
                          }
                       }
                 break;
                case 32:                                // scan: scan parameter
                 if (no_param) std::cerr << "scan parameters are missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--scan): format of scan "
                                         "description is wrong";
                            no_error=false;
                            break;
                          }
                         v.si.lld=a2i("--scan", optstr.substr(0, p),
                                      &no_error);
                         optstr.erase(0, p+1);
                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--scan): format of scan "
                                         "description is wrong";
                            no_error=false;
                            break;
                          }
                         v.si.uld=a2i("--scan", optstr.substr(0, p),
                                      &no_error);
                         optstr.erase(0, p+1);
                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--scan): format of scan "
                                         "description is wrong";
                            no_error=false;
                            break;
                          }
                         v.si.start_time=a2i("--scan", optstr.substr(0, p),
                                             &no_error);
                         optstr.erase(0, p+1);
                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--scan): format of scan "
                                         "description is wrong";
                            no_error=false;
                            break;
                          }
                         v.si.frame_duration=a2i("--scan", optstr.substr(0, p),
                                                 &no_error);
                         optstr.erase(0, p+1);
                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--scan): format of scan "
                                         "description is wrong";
                            no_error=false;
                            break;
                          }
                         v.si.gate_duration=a2i("--scan", optstr.substr(0, p),
                                                &no_error);
                         optstr.erase(0, p+1);
                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--scan): format of scan "
                                         "description is wrong";
                            no_error=false;
                            break;
                          }
                         v.si.halflife=a2f("--scan", optstr.substr(0, p),
                                           &no_error);
                         v.si.bedpos=a2f("--scan", optstr.substr(p+1),
                                         &no_error);
                       }
                 break;
                case 33:                 // nosc: switch off scatter correction
                 v.scatter_corr=false;
                 break;
                case 34:                         // ucut: cut b/t border planes
                 v.ucut=true;
                 break;
                case 35:                           // oi: image output filename
                 if (no_param) std::cerr << "image output filename missing";
                  else if (optstr.find(',') == std::string::npos)
                        v.oimage_filename=convertSlash(optstr);
                        else { std::cerr << "Error (--oi): too many "
                                            "parameters";
                               no_error=false;
                             }
                 break;
                case 36:                      // algo: reconstruction algorithm
                 if (no_param) std::cerr << "reconstruction algorithm missing";
                  else if (optstr == "fbp") v.algorithm=ALGO::FBP_Algo;
                        else if (optstr == "dift")
                              v.algorithm=ALGO::DIFT_Algo;
                        else if (optstr == "mlem")
                              { v.algorithm=ALGO::MLEM_Algo;
                                v.subsets=1;
                              }
                        else if (optstr == "uw-osem")
                              v.algorithm=ALGO::OSEM_UW_Algo;
                        else if (optstr == "aw-osem")
                              v.algorithm=ALGO::OSEM_AW_Algo;
                        else if (optstr == "nw-osem")
                              v.algorithm=ALGO::OSEM_NW_Algo;
                        else if (optstr == "anw-osem")
                              v.algorithm=ALGO::OSEM_ANW_Algo;
                        else if (optstr == "op-osem")
                              v.algorithm=ALGO::OSEM_OP_Algo;
                        else if (optstr == "psf-aw-osem")
                              v.algorithm=ALGO::OSEM_PSF_AW_Algo;
                        else { std::cerr << "Error (--algo): algorithm \""
                                         << optstr << "\" unknown";
                               no_error=false;
                             }
                 break;
                case 37:                                // trim: trimming value
                 if (no_param) std::cerr << "trimming value missing";
                  else v.trim=a2i("--trim", optstr, &no_error);
                 break;
                case 38:                                  // gibbs: gibbs prior
                 if (no_param) std::cerr << "gibbs prior missing";
                  else { v.gibbs_prior=a2f("--gibbs", optstr, &no_error);
                         if (v.gibbs_prior < 0.0f)
                          { std::cerr << "Error (--gibbs): gibbs prior is "
                                         "negativ";
                            no_error=false;
                          }
                       }
                 break;
                case 39:                                   // zoom: zoom factor
                 if (no_param) std::cerr << "zoom factor missing";
                  else v.zoom_factor=a2f("--zoom", optstr, &no_error);
                 break;
                case 40:                                         // ol: overlap
                 v.overlap=true;
                 break;
                case 41:                                     // gf: gap-filling
                 v.gap_filling=true;
                 break;
                case 42:                           // swap: path for swap-files
                 if (optstr.find(',') == std::string::npos)
                  v.swap_path=convertSlash(optstr);
                  else { std::cerr << "Error (--swap): too many parameters";
                         no_error=false;
                       }
                 break;
                case 43:      // skip: skip border planes in scatter estimation
                 if (no_param) std::cerr << "number of planes missing";
                  else v.scatter_skip=a2i("--skip", optstr, &no_error);
                 break;
                case 44:                         // aus: auto-scale u-map off ?
                 v.autoscale_umap=false;
                 break;
                case 45:                       // newsc: use new scatter code ?
                 v.new_scatter_code=true;
                 break;
                case 46:                           // tof: use TOF processing ?
                 v.tof_processing=true;
                 break;
                case 47:                    // tofp: parameter for TOF sinogram
                 if (no_param) std::cerr << "TOF parameters are missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--tofp): format for TOF "
                                         "parameters is wrong";
                            no_error=false;
                            break;
                          }
                         v.tof_bins=a2i("--tofp", optstr.substr(0, p),
                                        &no_error);
                         optstr.erase(0, p+1);
                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--tofp): format for TOF "
                                         "parameters is wrong";
                            no_error=false;
                            break;
                          }
                         v.tof_width=a2f("--tofp", optstr.substr(0, p),
                                         &no_error);
                         v.tof_fwhm=a2f("--tofp", optstr.substr(p+1),
                                        &no_error);
                       }
                 break;
                case 48:               // d2: store image after every iteration
                 v.debug_iter=true;
                 break;
                case 49:          // dttx: deadtime correction for transmission
                 if (no_param) std::cerr << "parameters are missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--dttx): format for "
                                         "parameters is wrong";
                            no_error=false;
                            break;
                          }
                         v.blank_db_path=convertSlash(
                                                  optstr.substr(0, p).c_str());
                         v.blank_db_frames=a2i("--dttx", optstr.substr(p+1),
                                               &no_error);
                       }
                 break;
                case 50:                            // noise: add Poisson noise
                 if (no_param) std::cerr << "parameters are missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--noise): format for noise "
                                         "parameters is wrong";
                            no_error=false;
                            break;
                          }
                         v.trues=a2f("--noise", optstr.substr(0, p),
                                     &no_error);
                         v.randoms=a2f("--noise", optstr.substr(p+1),
                                       &no_error);
                       }
                 break;
                case 51:                                // np: P39 patient norm
                 if (no_param) std::cerr << "norm input filename is missing";
                  else { parseFilename(optstr, &(v.norm_filename), "--np",
                                       &no_error, NULL, NULL, NULL);
                         v.p39_patient_norm=true;
                       }
                 break;
                case 52:                                      // ecf: apply ECF
                 v.apply_ecf=true;
                 break;
                case 53:           // ssf: thresholds for scatter scale factors
                 if (no_param) std::cerr << "thresholds missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--ssf): wrong format";
                            no_error=false;
                          }
                          else { v.scatter_scale_min=a2f("--ssf",
                                                         optstr.substr(0, p),
                                                         &no_error);
                                 if (v.scatter_scale_min < 0.0f)
                                  { std::cerr << "Error (--ssf): minimum "
                                                 "threshold too small";
                                    no_error=false;
                                    break;
                                  }
                                 v.scatter_scale_max=a2f("--ssf",
                                                         optstr.substr(p+1),
                                                         &no_error);
                                 if (v.scatter_scale_max < v.scatter_scale_min)
                                  { std::cerr << "Error (--ssf): maximum "
                                                 "threshold too small";
                                    no_error=false;
                                  }
                               }
                       }
                 break;
                case 54:                     // force: overwrite existing files
                 v.force=true;
                 break;
                case 55:           // resrv: connect to reconstruction server ?
                 if (no_param) std::cerr << "port number is missing";
                  else { v.recoserver=true;
                         v.rs_port=a2i("--resrv", optstr, &no_error);
                       }
                 break;
                case 56:                       // asv: u-value for auto-scaling
                 if (no_param) std::cerr << mu << "-value is missing";
                  else v.asv=a2f("--asv", optstr, &no_error);
                 break;
                case 57:          // app: append output files to existing files
                 v.append=true;
                 break;
                case 58:                          // thr: threshold and u-value
                 if (no_param) std::cerr << "parameters are missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (--thr): format for conversion"
                                         " parameters is wrong";
                            no_error=false;
                            break;
                          }
                         v.pet_threshold=a2f("--thr", optstr.substr(0, p),
                                             &no_error);
                         v.pet_uvalue=a2f("--thr", optstr.substr(p+1),
                                          &no_error);
                       }
                 break;
                case 59:     // athr: attenuation threshold for scatter scaling
                 if (no_param) std::cerr << "acf-value is missing";
                 else { std::string::size_type p;
                       if ((p=optstr.find(',')) == std::string::npos)
                          v.acf_threshold=a2f("--athr", optstr, &no_error);
                       else { // acf margin specified
                         v.acf_threshold=a2f("--athr", optstr.substr(0, p), &no_error);
                         v.acf_threshold_margin=a2f("--athr", optstr.substr(p+1), &no_error);
                       }
                 }
                 break;
                case 60:                 // nbm: number of bins to mask in norm
                 if (no_param) std::cerr << "number of bins missing";
                  else v.norm_mask_bins=a2i("--nbm", optstr, &no_error);
                 break;
                case 61:                         // sarc: arc-corrected scatter
                 if (no_param)
                  std::cerr << "scatter input filename is missing";
                  else { parseFilename(optstr, &(v.scatter_filename),
                                       "--sarc", &no_error, &(v.sRhoSamples),
                                       &(v.sThetaSamples), &(v.sdim));
                         v.scatter_arc_corrected=true;
                         v.scatter_arc_corrected_overwrite=true;
                       }
                 break;
                case 62:                      // snarc: unarc-corrected scatter
                 if (no_param)
                  std::cerr << "scatter input filename is missing";
                  else { parseFilename(optstr, &(v.scatter_filename),
                                       "--snarc", &no_error, &(v.sRhoSamples),
                                       &(v.sThetaSamples), &(v.sdim));
                         v.scatter_arc_corrected=false;
                         v.scatter_arc_corrected_overwrite=true;
                       }
                 break;
                case 63:                     // naarc: no axial arc-corrections
                 v.no_axial_arc=true;
                 break;
                case 64:                    // nrarc: no radial arc-corrections
                 v.no_radial_arc=true;
                 break;
                case 65:                    // uac: undo attenuation correction
                 v.undo_attcor=true;
                 break;
                case 66:             // mem: fraction of physical memory to use
                 if (no_param) std::cerr << "fraction is missing";
                  else v.memory_limit=a2f("--mem", optstr, &no_error);
                 break;
                case 67:                                   // bp: bed positions
                 if (no_param) std::cerr << "bed positions missing";
                  else { std::string::size_type p;

                         v.bed_position.clear();
                                                           // get bed positions
                         while ((p=optstr.find(',')) != std::string::npos)
                          { v.bed_position.push_back(
                                  a2f("--bp", optstr.substr(0, p), &no_error));
                            optstr.erase(0, p+1);
                          }
                         v.bed_position.push_back(a2f("--bp", optstr,
                                                      &no_error));
                       }
                 break;
                case 68:                                 // pguid: patient GUID
                 if (no_param) std::cerr << "patient GUID is missing";
                  else { v.pguid=optstr;
                         if (v.pguid.at(0) != '{') v.pguid="{"+v.pguid;
                         if (v.pguid.at(v.pguid.length()-1) != '}')
                          v.pguid+="}";
                       }
                 break;
                case 69:                                  // sguid: series GUID
                 if (no_param) std::cerr << "study GUID is missing";
                  else { v.sguid=optstr;
                         if (v.sguid.at(0) != '{') v.sguid="{"+v.sguid;
                         if (v.sguid.at(v.sguid.length()-1) != '}')
                          v.sguid+="}";
                       }
                 break;
                case 70:                                 // iguid: series GUIDs
                 if (no_param) std::cerr << "image series GUID(s) missing";
                  else { std::string::size_type p;
                         std::string s;

                         v.iguid.clear();
                                                                   // get GUIDs
                         while ((p=optstr.find(',')) != std::string::npos)
                          { s=optstr.substr(0, p);
                            optstr.erase(0, p+1);
                            if (s.at(0) != '{') s="{"+s;
                            if (s.at(s.length()-1) != '}') s+="}";
                            v.iguid.push_back(s);
                          }
                         s=optstr;
                         if (s.at(0) != '{') s="{"+s;
                         if (s.at(s.length()-1) != '}') s+="}";
                         v.iguid.push_back(s);
                       }
                 break;
                case 71:        // offs: offsets between CT and PET coordinates
                 if (no_param) std::cerr << "offsets are missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (-r): wrong format";
                            no_error=false;
                            break;
                          }
                         v.ct2pet_offset.x=a2f("--offs", optstr.substr(0, p),
                                               &no_error);
                         optstr.erase(0, p+1);
                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (-r): wrong format";
                            no_error=false;
                            break;
                          }
                         v.ct2pet_offset.y=a2f("--offs", optstr.substr(0, p),
                                               &no_error);
                         v.ct2pet_offset.z=a2f("--offs", optstr.substr(p+1),
                                               &no_error);
                       }
                 break;
                case 72:                                    // pn: patient name
                 if (no_param) std::cerr << "patient name is missing";
                  else v.patient_name=optstr;
                 break;
                case 73:                               // cguid: scanogram GUID
                 if (no_param) std::cerr << "scanogram GUID is missing";
                  else { v.cguid=optstr;
                         if (v.cguid.at(0) != '{') v.cguid="{"+v.cguid;
                         if (v.cguid.at(v.cguid.length()-1) != '}')
                          v.cguid+="}";
                       }
                 break;
                 case 74:             // os2d: 2d scatter and scaling factors
                   v.oscatter_2d=true;
                 break;
                 case 75:        // txsc: TX scatter correction factors
                 if (no_param) std::cerr << "TX scatterfactors are missing";
                  else { std::string::size_type p;

                         if ((p=optstr.find(',')) == std::string::npos)
                          { std::cerr << "Error (-r): wrong format";
                            no_error=false;
                            break;
                          }
                         v.txscatter[0]=a2f("--txsc", optstr.substr(0, p),
                                               &no_error);
                          v.txscatter[1]=a2f("--txsc", optstr.substr(p+1),
                                               &no_error);
                       }
                 break;
                 case 76:        // lber: LBER values
                 if (no_param) std::cerr << "LBER value is missing";
                 else v.lber=a2f("--lber", optstr, &no_error);
                 break;
                 case 77:        // txblr: TX/BL ratio values
                 if (no_param) std::cerr << "txblr value is missing";
                 else v.txblr=a2f("--txblr", optstr, &no_error);
                 break;

            }
             break;
            case 'a':                                    // unarc-corrected acf
             if (no_param) std::cerr << "acf input filename is missing";
              else { parseFilename(optstr, &(v.acf_filename), "-a", &no_error,
                                   &(v.aRhoSamples), &(v.aThetaSamples),
                                   &(v.adim));
                     v.acf_arc_corrected=false;
                     v.acf_arc_corrected_overwrite=false;
                   }
             break;
            case 'b':                                  // unarc-corrected blank
             if (no_param) std::cerr << "blank input filename is missing";
              else { parseFilename(optstr, &(v.blank_filename), "-b",
                                   &no_error, &(v.bRhoSamples),
                                   &(v.bThetaSamples), NULL);
                     v.blank_arc_corrected=false;
                     v.blank_arc_corrected_overwrite=false;
                   }
             break;
            case 'c':                                          // CT image file
             if (no_param) std::cerr << "CT-image input filename is missing";
              else if (optstr.find(',') == std::string::npos)
                    v.ct_filename=optstr;
                    else { std::cerr << "Error (-c): too many parameters";
                           no_error=false;
                         }
             break;
            case 'd':                                             // debug path
             v.debug=true;
             if (no_param) std::cerr << "path for debug info missing";
              else { struct stat statbuf;

                     if (optstr.find(',') == std::string::npos)
                      { v.debug_path=convertSlash(optstr);
                                  // does file exist and is it a regular file ?
                        if ((stat(v.debug_path.c_str(), &statbuf) != 0) ||
#ifdef WIN32
                            (!(statbuf.st_mode & _S_IFDIR)))
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
                            !S_ISDIR(statbuf.st_mode))
#endif
                         { std::cerr << "Error (-d): path for debug info "
                                        "doesn't exist";
                           no_error=false;
                         }
                      }
                      else { std::cerr << "Error (-d): too many parameters";
                             no_error=false;
                           }
                   }
             break;
            case 'e':                               // unarc-corrected emission
             if (no_param) std::cerr << "emission input filename is missing";
              else { parseFilename(optstr, &(v.emission_filename), "-e",
                                   &no_error, &(v.eRhoSamples),
                                   &(v.eThetaSamples), &(v.edim));
                     v.emission_arc_corrected=false;
                     v.emission_arc_corrected_overwrite=false;
                   }
             break;
            case 'h':                                          // extended help
             usage(true);
             return(false);
            case 'i':                                         // image filename
             if (no_param) std::cerr << "image input filename missing";
              else if (optstr.find(',') == std::string::npos)
                    v.image_filename=optstr;
                    else { std::cerr << "Error (-i): too many parameters";
                           no_error=false;
                         }
             break;
            case 'k':                                            // kill blocks
             if (no_param) std::cerr << "block number(s) missing";
              else { std::string::size_type p;

                     v.kill_blocks.clear();
                                                           // get block numbers
                     while ((p=optstr.find(',')) != std::string::npos)
                      { v.kill_blocks.push_back(
                                    a2i("-k", optstr.substr(0, p), &no_error));
                        optstr.erase(0, p+1);
                      }
                     v.kill_blocks.push_back(a2i("-k", optstr, &no_error));
                   }
             break;
            case 'l':                                                // logging
             if (no_param) std::cerr << "logging code missing";
              else { std::string::size_type p;

                     if ((p=optstr.find(',')) == std::string::npos)
                      v.logcode=a2i("-l", optstr, &no_error);
                      else { v.logcode=a2i("-l", optstr.substr(0, p),
                                           &no_error);
                             optstr.erase(0, p+1);
                             if (optstr.find(',') == std::string::npos)
                              v.logpath=convertSlash(optstr);
                              else { std::cerr << "Error (-l): too many "
                                                  "parameters";
                                     no_error=false;
                                   }
                           }
                   }
             break;
            case 'n':                           // n: unarc-corrected norm file
             if (no_param) std::cerr << "norm input filename is missing";
              else parseFilename(optstr, &(v.norm_filename), "-n", &no_error,
                                 &(v.nRhoSamples), &(v.nThetaSamples), NULL);
             break;
            case 'p':                                // segmentation parameters
             if (no_param) std::cerr << "segmentation parameters missing";
              else { std::string::size_type p;
                     std::string str;
                     unsigned short int values, i;

                     if ((p=optstr.find(",")) == std::string::npos)
                      { std::cerr << "Error (-p): wrong format";
                        no_error=false;
                        break;
                      }
                     v.number_of_segments=a2i("-p", optstr.substr(0, p),
                                              &no_error);
                     optstr.erase(0, p+1);
                     str=optstr;             // count number of values in list
                     values=0;
                     while ((p=str.find(",")) != std::string::npos)
                      { values++;
                        str.erase(0, p+1);
                      }
                     if (values != 3*v.number_of_segments-2)
                      { std::cerr << "Error (-p): wrong format";
                        no_error=false;
                        break;
                      }
                     if (v.mu_mean != NULL) delete[] v.mu_mean;
                     v.mu_mean=new float[v.number_of_segments];
                     if (v.mu_std != NULL) delete[] v.mu_std;
                     v.mu_std=new float[v.number_of_segments];
                     if (v.gauss_intersect != NULL) delete[] v.gauss_intersect;
                     v.gauss_intersect=new float[v.number_of_segments-1];
                     for (i=0; i < v.number_of_segments; i++)
                      { p=optstr.find(",");
                        v.mu_mean[i]=a2f("-p", optstr.substr(0, p), &no_error);
                        optstr.erase(0, p+1);
                        p=optstr.find(",");
                        v.mu_std[i]=a2f("-p", optstr.substr(0, p), &no_error);
                        optstr.erase(0, p+1);
                      }
                     for (i=0; i < v.number_of_segments-2; i++)
                      { p=optstr.find(",");
                        v.gauss_intersect[i]=a2f("-p", optstr.substr(0, p),
                                                 &no_error);
                        optstr.erase(0, p+1);
                      }
                     v.gauss_intersect[v.number_of_segments-2]=
                                                  a2f("-p", optstr, &no_error);
                     v.segmentation_params_set=true;
                   }
             break;
            case 'q':                                   // quality control path
             v.quality_control=true;
             if (no_param)
              std::cerr << "path for quality control data missing";
              else { struct stat statbuf;

                     if (optstr.find(',') == std::string::npos)
                      { v.quality_path=convertSlash(optstr);
                                  // does file exist and is it a regular file ?
                        if ((stat(v.quality_path.c_str(), &statbuf) != 0) ||
#ifdef WIN32
                            (!(statbuf.st_mode & _S_IFDIR)))
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
                            !S_ISDIR(statbuf.st_mode))
#endif
                         { std::cerr << "Error (-q): path for quality control "
                                        "data doesn't exist";
                           no_error=false;
                         }
                      }
                      else { std::cerr << "Error (-q): too many parameters";
                             no_error=false;
                           }
                   }
             break;
            case 'r':                                              // rebinning
             if (no_param) std::cerr << "algorithm missing";
              else { std::string::size_type p;

                     if ((p=optstr.find(',')) == std::string::npos)
                      { if (optstr == "fore")
                         v.rebin_method=SinogramConversion::FORE_REB;
                         else if (optstr == "ssrb")
                               v.rebin_method=SinogramConversion::SSRB_REB;
                         else if (optstr == "seg0")
                               v.rebin_method=SinogramConversion::SEG0_REB;
                         else { std::cerr << "Error (-r): unknown rebinning "
                                             "method";
                                no_error=false;
                              }
                        break;
                      }
                     if (optstr.substr(0, 4) != "fore")
                      { std::cerr << "Error (-r): unknown rebinning method";
                        no_error=false;
                        break;
                      }
                     v.rebin_method=SinogramConversion::FORE_REB;
                     if ((p=optstr.find(',')) == std::string::npos)
                      { std::cerr << "Error (-r): wrong format";
                        no_error=false;
                        break;
                      }
                     optstr.erase(0, p+1);
                     if ((p=optstr.find(',')) == std::string::npos)
                      { std::cerr << "Error (-r): wrong format";
                        no_error=false;
                        break;
                      }
                     v.a_lim=a2i("-r", optstr.substr(0, p), &no_error);
                     optstr.erase(0, p+1);
                     if ((p=optstr.find(',')) == std::string::npos)
                      { std::cerr << "Error (-r): wrong format";
                        no_error=false;
                        break;
                      }
                     v.w_lim=a2i("-r", optstr.substr(0, p), &no_error);
                     v.k_lim=a2i("-r", optstr.substr(p+1), &no_error);
                   }
             break;
            case 'R':                         // radial and azimuthal rebinning
             if (no_param) std::cerr << "rebinning factors missing";
              else { std::string::size_type p;

                     if ((p=optstr.find(',')) == std::string::npos)
                      { std::cerr << "Error (-R): wrong format";
                        no_error=false;
                      }
                      else { v.rebin_r=a2i("-R", optstr.substr(0, p),
                                           &no_error);
                             if ((v.rebin_r != 1) && (v.rebin_r != 2) &&
                                 (v.rebin_r != 4))
                              { std::cerr << "Error (-R): radial rebinning "
                                             "factor invalid";
                                no_error=false;
                                break;
                              }
                             v.rebin_t=a2i("-R", optstr.substr(p+1),
                                           &no_error);
                             if ((v.rebin_t != 1) && (v.rebin_t != 2) &&
                                 (v.rebin_t != 4))
                              { std::cerr << "Error (-R): azimuthal rebinning"
                                             " factor invalid";
                                no_error=false;
                              }
                           }
                   }
             break;
            case 's':                                // unarc-corrected scatter
             if (no_param) std::cerr << "scatter input filename is missing";
              else { parseFilename(optstr, &(v.scatter_filename), "-s",
                                   &no_error, &(v.sRhoSamples),
                                   &(v.sThetaSamples), &(v.sdim));
                     v.scatter_arc_corrected=false;
                     v.scatter_arc_corrected_overwrite=false;
                   }
             break;
            case 't':                           // unarc-corrected transmission
             if (no_param)
              std::cerr << "transmission input filename is missing";
              else { parseFilename(optstr, &(v.tra_filename), "-t", &no_error,
                                   &(v.tRhoSamples), &(v.tThetaSamples), NULL);
                     v.tra_arc_corrected=false;
                     v.tra_arc_corrected_overwrite=false;
                   }
             break;
            case 'u':                                         // u-map filename
             if (no_param) std::cerr << mu << "-map input filename missing";
              else if (optstr.find(',') == std::string::npos)
                    v.umap_filename=convertSlash(optstr);
                    else { std::cerr << "Error (-u): too many parameters";
                           no_error=false;
                         }
             break;
            case 'v':                              // print version information
             timestamp(prgname, date);
             return(false);
            case 'w':                                  // width/height of image
             if (no_param) std::cerr << "width/height missing";
              else v.iwidth=a2i("-w", optstr, &no_error);
             break;
            case '?':                                             // short help
             usage(false);
             return(false);
            default:                                          // unknown option
             std::cerr << "unknown option";
             no_error=false;
             break;
          }
       }
     if (c == -2)                                           // unknown switch ?
      { std::cerr << "Error (" << argv[optind] << "): unknown "
                     "switch";
        no_error=false;
      }
     if (!no_error) { std::cerr << "\n";
                      return(false);
                    }
                                       // check for illegal switch combinations
     if (!checkSemantics(switch_str)) return(false);
                         // apply u-map scale factor to segmentation parameters
     if ((v.alpha != 0.0f) &&
         ((v.iterations_ip == 0) && (v.beta_ip > 0.0f)) &&
         (!v.blank_filename.empty())) v.quality_control=false;
#ifdef OLD_SCALE
                 // modify the segmentation parameters to get 0.096 for tissue;
                 // the segmentation would otherwise produce 0.088
     { unsigned short int i;

       for (i=0; i < v.number_of_segments; i++)
        { v.mu_mean[i]=v.mu_mean[i]/v.umap_scalefactor*0.088f/0.096f;
          v.mu_std[i]=v.mu_std[i]/v.umap_scalefactor;
        }
       for (i=0; i < v.number_of_segments-1; i++)
        v.gauss_intersect[i]=v.gauss_intersect[i]/v.umap_scalefactor*
                             0.088f/0.096f;
       v.umap_scalefactor*=0.096f/0.088f;
     }
#endif
     { unsigned short int i;

       for (i=0; i < v.number_of_segments; i++)
        { v.mu_mean[i]/=v.umap_scalefactor;
          v.mu_std[i]/=v.umap_scalefactor;
        }
       for (i=0; i < v.number_of_segments-1; i++)
        v.gauss_intersect[i]/=v.umap_scalefactor;
     }
     return(true);
   }
   catch (...)
    { if (v.mu_mean != NULL) { delete[] v.mu_mean;
                               v.mu_mean=NULL;
                             }
      if (v.mu_std != NULL) { delete[] v.mu_std;
                              v.mu_std=NULL;
                            }
      if (v.gauss_intersect != NULL) { delete[] v.gauss_intersect;
                                       v.gauss_intersect=NULL;
                                     }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse filename.
    \param[in]  str            string with filename
    \param[out] filename       name of file
    \param[in]  switch_str     switch
    \param[out] no_error       no error during parsing ?
    \param[out] RhoSamples     number of bins in a projection
    \param[out] ThetaSamples   number of projections in sinogram
    \param[out] sino2d         2d sinogram ?

    Parse filename specifications. The regular expression
    "filename[,RhoSamples,ThetaSamples[,2d|3d]]" is split into its separate
    components.
 */
/*---------------------------------------------------------------------------*/
void Parser::parseFilename(std::string str, std::string * const filename,
                           const std::string switch_str,
                           bool * const no_error,
                           unsigned short int * const RhoSamples,
                           unsigned short int * const ThetaSamples,
                           bool * const sino2d) const
 { std::string::size_type p;

   if (sino2d != NULL) *sino2d=false;
   if ((p=str.find(',')) == std::string::npos) *filename=str;
    else { *filename=str.substr(0, p);
           str.erase(0, p+1);
           if (((p=str.find(',')) == std::string::npos) ||
               (RhoSamples == NULL) || (ThetaSamples == NULL))
            { std::cerr << "Error (" << switch_str << "): wrong format";
              *no_error=false;
            }
            else { *RhoSamples=a2i(switch_str, str.substr(0, p), no_error);
                   str.erase(0, p+1);
                   if ((p=str.find(',')) == std::string::npos)
                    *ThetaSamples=a2i(switch_str, str, no_error);
                    else { *ThetaSamples=a2i(switch_str, str.substr(0, p),
                                             no_error);
                           str.erase(0, p+1);
                           if (sino2d != NULL)
                            if ((str == "2d") || (str == "2D")) *sino2d=true;
                             else if ((str == "3d") ||
                                      (str == "3D")) *sino2d=false;
                                   else { std::cerr << "Error (" << switch_str
                                                    << "): wrong format";
                                          *no_error=false;
                                        }
                         }
                 }
         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a unique key for the given command line parameters.
    \return key

    Create a key from the given command line parameters which will be used
    in the swapping mechanism. The memory access pattern of the application
    depends on the combination of command line switches. Combinations which
    result in the same pattern may have the same key. Combinations which result
    in different patterns, must have different keys.
 */
/*---------------------------------------------------------------------------*/
std::string Parser::patternKey() const
 { std::string str;
   unsigned long int bits=0;

   if ((v.gauss_fwhm_xy > 0.0f) || (v.gauss_fwhm_z > 0.0f)) bits|=0x1;
   if (v.umap_scalefactor != 1.0f) bits|=0x2;
   if (v.kill_blocks.size() > 0) bits|=0x4;
   if ((v.rebin_r > 1) || (v.rebin_t > 1)) bits|=0x8;
   if (v.iwidth > 0) bits|=0x10;
   if ((v.uzoom_xy_factor > 1) || (v.uzoom_z_factor > 1)) bits|=0x20;
   if (v.trim > 0)                         bits|=0x40;
   if (!v.acf_filename.empty())            bits|=0x80;
   if (!v.oacf_filename.empty())           bits|=0x100;
   if (!v.blank_filename.empty())          bits|=0x200;
   if (!v.tra_filename.empty())            bits|=0x400;
   if (!v.umap_filename.empty())           bits|=0x800;
   if (!v.oumap_filename.empty())          bits|=0x1000;
   if (!v.emission_filename.empty())       bits|=0x2000;
   if (!v.oemission_filename.empty())      bits|=0x4000;
   if (!v.scatter_filename.empty())        bits|=0x8000;
   if (!v.oscatter_filename.empty())       bits|=0x10000;
   if (!v.norm_filename.empty())           bits|=0x20000;
   if (!v.image_filename.empty())          bits|=0x40000;
   if (!v.oimage_filename.empty())         bits|=0x80000;
   if (v.debug)                            bits|=0x100000;
   if (v.no_tilt)                          bits|=0x200000;
   if (v.acf_arc_corrected)                bits|=0x400000;
   if (v.acf_arc_corrected_overwrite)      bits|=0x800000;
   if (v.oacf_arc_corrected)               bits|=0x1000000;
   if (v.blank_arc_corrected)              bits|=0x2000000;
   if (v.blank_arc_corrected_overwrite)    bits|=0x4000000;
   if (v.tra_arc_corrected)                bits|=0x8000000;
   if (v.tra_arc_corrected_overwrite)      bits|=0x10000000;
   if (v.emission_arc_corrected)           bits|=0x20000000;
   if (v.emission_arc_corrected_overwrite) bits|=0x40000000;
   if (v.oemission_arc_corrected)          bits|=0x80000000;
   str+=toString(bits);
   bits=0;
   if (v.scatter_arc_corrected)           bits|=0x1;
   if (v.scatter_arc_corrected_overwrite) bits|=0x2;
   if (v.oscatter_arc_corrected)          bits|=0x4;
   if (v.randoms_smoothing)               bits|=0x8;
   if (v.decay_corr || v.framelen_corr)   bits|=0x10;
   if (v.scatter_corr)                    bits|=0x20;
   if (v.overlap)                         bits|=0x40;
   if (v.gap_filling)                     bits|=0x80;
   if (v.adim)                            bits|=0x100;
   if (v.edim)                            bits|=0x200;
   if (v.tof_processing)                  bits|=0x400;
   if (v.debug_iter)                      bits|=0x800;
   if (v.trues > 0.0)                     bits|=0x1000;
   if (!v.blank_db_path.empty())          bits|=0x2000;
   if (v.new_scatter_code)                bits|=0x4000;
   if (v.p39_patient_norm)                bits|=0x8000;
   if (v.apply_ecf)                       bits|=0x10000;
   if (v.norm_mask_bins > 0)              bits|=0x20000;
   if (v.no_radial_arc)                   bits|=0x40000;
   if (v.no_axial_arc)                    bits|=0x80000;
   if (v.undo_attcor)                     bits|=0x100000;
   str+=toString(bits);
   str+=toString(v.memory_limit);
   str+=toString(v.iterations);
   str+=toString(v.subsets);
   str+=toString(v.mrd);
   str+=toString(v.span);
   str+=toString((unsigned long int)v.algorithm);
   str+=toString((unsigned long int)v.irebin_method);
   return(str);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print usage information.
    \param[in] extended   print extended usage information ?

    Print usage information.
 */
/*---------------------------------------------------------------------------*/
void Parser::usage(const bool extended) const
 { std::string mu, beta, cpy;
   unsigned short int count;

#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
   char c[2];

   c[1]=0;
   c[0]=(char)181;
   mu=std::string(c);
   c[0]=(char)223;
   beta=std::string(c);
   c[0]=(char)169;
   cpy=std::string(c);
#endif
#ifdef WIN32
   mu='u';
   beta="beta";
   cpy="(c)";
#endif
                                // print copyright information and command line
   std::cout << "\n";
   count=62;
   while (count-cpy.length()-date.length() > 0)
    { std::cout << " ";
      count--;
    }
   std::cout << cpy << " " << date << " CPS Innovations\n\n"
             << prgname << " - " << descr << "\n\n"
                "Usage:\n\n " << prgname << " ";

   std::string::size_type p=0;
   std::string lparam;
            // create string with regular expression for command line arguments
   while (p < param.length())
    { std::string::size_type p1, p2;
      std::string swi;
                                                              // extract switch
      p1=param.substr(p).find_first_not_of("[(|)]$ ");
      if (p+p1 == param.length()-1) p2=1;
       else p2=param.substr(p+p1).find_first_of("[(|)]$ ");
      swi=param.substr(p+p1, p2);
      lparam+=param.substr(p, p1);
                                                    // add arguments for switch
      if (swi == "fn") lparam+="ECAT7-filename [ECAT7-filename [...]]";
       else { lparam+="-";
              if (swi.length() > 1) lparam+="-";
              lparam+=swi;
            }
      if (swi == "a") lparam+=" acf[,r,t,dim]";
       else if (swi == "algo") lparam+=" algorithm";
       else if (swi == "asv") lparam+=" "+mu+"-value";
       else if (swi == "athr") lparam+=" acf-value[,margin]";
       else if (swi == "b") lparam+=" blank[,r,t]";
       else if (swi == "bc") lparam+=" width";
       else if (swi == "bp") lparam+=" p0,p1,p2,...";
       else if (swi == "c") lparam+=" CT-image";
       else if (swi == "cguid") lparam+=" guid";
       else if (swi == "d") lparam+=" path";
       else if (swi == "dttx") lparam+=" path,frames";
       else if (swi == "e") lparam+=" emission[,r,t,dim]";
       else if (swi == "gibbs") lparam+=" prior";
       else if (swi == "gxy") lparam+=" fwhm";
       else if (swi == "gz") lparam+=" fwhm";
       else if (swi == "i") lparam+=" image";
       else if (swi == "iguid") lparam+=" guid[,guid[...]]";
       else if (swi == "is") lparam+=" iterations,subsets";
       else if (swi == "k") lparam+=" a,b,...,n";
       else if (swi == "l") lparam+=" a[,path]";
       else if (swi == "mat") lparam+=" a|a-b|a,b,...,n";
       else if (swi == "mem") lparam+=" fraction";
       else if (swi == "model")
             lparam+=" model|a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,"
                     "y,$                z1,z2,z3,z4,z1,z2,z3,z4,...";
       else if (swi == "mrd") lparam+=" mrd";
       else if (swi == "n") lparam+=" norm[,r,t]";
       else if (swi == "nbm") lparam+=" bins";
       else if (swi == "np") lparam+=" norm";
       else if (swi == "noise") lparam+=" trues,randoms";
       else if (swi == "oa") lparam+=" acf";
       else if (swi == "oe") lparam+=" emission";
       else if (swi == "offs") lparam+=" x,y,z";
       else if (swi == "oi") lparam+=" image";
       else if (swi == "os") lparam+=" scatter";
       else if (swi == "ou") lparam+=" "+mu+"-map";
       else if (swi == "pn") lparam+=" name";
       else if (swi == "pguid") lparam+=" guid";
       else if (swi == "prj") lparam+=" ifore|issrb|fwdpj";
       else if (swi == "q") lparam+=" path";
       else if (swi == "r") lparam+=" fore[,alim,wlim,klim]|ssrb|seg0";
       else if (swi == "R") lparam+=" r,t";
       else if (swi == "resrv") lparam+=" port";
       else if (swi == "s") lparam+=" scatter[,r,t,dim]";
       else if (swi == "scan") lparam+=" a,b,c,d,e,f";
       else if (swi == "sguid") lparam+=" guid";
       else if (swi == "skip") lparam+=" n";
       else if (swi == "span") lparam+=" span";
       else if (swi == "ssf") lparam+=" min,max";
       else if (swi == "swap") lparam+=" path";
       else if (swi == "t") lparam+=" transmission[,r,t]";
       else if (swi == "thr") lparam+=" t,u";
       else if (swi == "tofp") lparam+=" b,w,f";
       else if (swi == "trim") lparam+=" bins";
       else if (swi == "ualgo")
             lparam+=" 1)|$  (--ualgo 20|21|30|31|"
                     "(40|41 alpha,;,;_ip,iter_ip)$   [-p segments,mean,std,"
                     "mean,std,...,b,b,...]";
       else if (swi == "u") lparam+=" "+mu+"-map";
       else if (swi == "uf") lparam+=" factor";
       else if (swi == "uzoom") lparam+=" xy_zoom,z_zoom";
       else if (swi == "w") lparam+=" width";
       else if (swi == "zoom") lparam+=" factor";
      p+=p1+p2;
    }
                                   // print regular expression with line breaks
   count=1+prgname.length()+1;
   p=0;
   while (p < lparam.length())
    { std::string::size_type p1;

      p1=lparam.substr(p).find("$");                   // possible line break ?
      if (p1 == std::string::npos) p1=lparam.length()-p;
      if (count+p1 > LINE_LEN)                             // need line break ?
       { std::cout << "\n";
         count=1+prgname.length()+1;
         for (unsigned short int i=0; i < count; i++)
          std::cout << " ";
       }
                                                                // print switch
      for (unsigned short int i=p; i < p+p1; i++)
       if (lparam[i] == ';') std::cout << beta;
        else std::cout << lparam[i];
      count+=p1;
      p+=p1+1;
    }
   std::cout << "\n\n";
                                                 // print parameter description
   p=0;
   while (p < switches.length())
    { std::string::size_type p1, p2;
      std::string swi, str;
      unsigned short int i, pos;
                                                              // extract switch
      p1=switches.substr(p).find_first_not_of(" ");
      if (p+p1 == switches.length()-1) p2=1;
       else p2=switches.substr(p+p1).find(" ");
      swi=switches.substr(p+p1, p2);
      if (extended) i=1;
       else i=0;
                                                  // get description for switch
      if (swi == "a") str=a_str[i];
       else if (swi == "algo") str=algo_str[i];
       else if ((swi == "aarc") || (swi == "anarc")) str=aarc_str[i];
       else if (swi == "app") str=app_str[i];
       else if (swi == "asv") str=asv_str[i];
       else if (swi == "athr") str=athr_str[i];
       else if (swi == "aus") str=aus_str[i];
       else if (swi == "b") str=b_str[i];
       else if ((swi == "barc") || (swi == "bnarc")) str=barc_str[i];
       else if (swi == "bc") str=bc_str[i];
       else if (swi == "bp") str=bp_str[i];
       else if (swi == "c") str=c_str[i];
       else if (swi == "cguid") str=cguid_str[i];
       else if (swi == "d") str=d_str[i];
       else if (swi == "d2") str=d2_str[i];
       else if (swi == "dc") str=dc_str[i];
       else if (swi == "dttx") str=dttx_str[i];
       else if (swi == "e") str=e_str[i];
       else if ((swi == "earc") || (swi == "enarc")) str=earc_str[i];
       else if (swi == "ecf") str=ecf_str[i];
       else if (swi == "fl") str=fl_str[i];
       else if (swi == "force") str=force_str[i];
       else if (swi == "gf") str=gf_str[i];
       else if (swi == "gibbs") str=gibbs_str[i];
       else if (swi == "gxy") str=gxy_str[i];
       else if (swi == "gz") str=gz_str[i];
       else if (swi == "h") str=h_str[i];
       else if (swi == "i") str=i_str[i];
       else if (swi == "iguid") str=iguid_str[i];
       else if (swi == "is") str=is_str[i];
       else if (swi == "k") str=k_str[i];
       else if (swi == "l") str=l_str[i];
       else if (swi == "lber") str=lber_str[i];
       else if (swi == "mat") str=mat_str[i];
       else if (swi == "mem") str=mem_str[i];
       else if (swi == "model") str=model_str[i];
       else if (swi == "mrd") str=mrd_str[i];
       else if (swi == "n") str=n_str[i];
       else if (swi == "naarc") str=naarc_str[i];
       else if (swi == "nbm") str=nbm_str[i];
       else if (swi == "newsc") str=newsc_str[i];
       else if (swi == "noise") str=noise_str[i];
       else if (swi == "nosc") str=nosc_str[i];
       else if (swi == "np") str=np_str[i];
       else if (swi == "nrarc") str=nrarc_str[i];
       else if (swi == "ntilt") str=ntilt_str[i];
       else if (swi == "oa") str=oa_str[i];
       else if (swi == "oaarc") str=oaarc_str[i];
       else if (swi == "oe") str=oe_str[i];
       else if (swi == "oearc") str=oearc_str[i];
       else if (swi == "offs") str=offs_str[i];
       else if (swi == "oi") str=oi_str[i];
       else if (swi == "ol") str=ol_str[i];
       else if (swi == "os") str=os_str[i];
       else if (swi == "os2d") str=os2d_str[i];
       else if (swi == "osarc") str=osarc_str[i];
       else if (swi == "ou") str=ou_str[i];
       else if (swi == "p") str=p_str[i];
       else if (swi == "pguid") str=pguid_str[i];
       else if (swi == "pn") str=pn_str[i];
       else if (swi == "prj") str=prj_str[i];
       else if (swi == "q") str=q_str[i];
       else if (swi == "r") str=r_str[i];
       else if (swi == "R") str=R_str[i];
       else if (swi == "resrv") str=resrv_str[i];
       else if (swi == "rs") str=rs_str[i];
       else if (swi == "s") str=s_str[i];
       else if ((swi == "sarc") || (swi == "snarc")) str=sarc_str[i];
       else if (swi == "scan") str=scan_str[i];
       else if (swi == "sguid") str=sguid_str[i];
       else if (swi == "skip") str=skip_str[i];
       else if (swi == "span") str=span_str[i];
       else if (swi == "ssf") str=ssf_str[i];
       else if (swi == "swap") str=swap_str[i];
       else if (swi == "t") str=t_str[i];
       else if ((swi == "tarc") || (swi == "tnarc")) str=tarc_str[i];
       else if (swi == "thr") str=thr_str[i];
       else if (swi == "tof") str=tof_str[i];
       else if (swi == "tofp") str=tofp_str[i];
       else if (swi == "trim") str=trim_str[i];
       else if (swi == "txsc") str=txsc_str[i];
       else if (swi == "txblr") str=txblr_str[i];
       else if (swi == "u") str=u_str[i];
       else if (swi == "ualgo") str=ualgo_str[i];
       else if (swi == "uac") str=uac_str[i];
       else if (swi == "ucut") str=ucut_str[i];
       else if (swi == "uf") str=uf_str[i];
       else if (swi == "uzoom") str=uzoom_str[i];
       else if (swi == "v") str=v_str[i];
       else if (swi == "view") str=view_str[i];
       else if (swi == "w") str=w_str[i];
       else if (swi == "zoom") str=zoom_str[i];
       else if (swi == "?") str=qm_str[i];
      if (str == "!") { p+=p1+p2;          // no help available for this switch
                        continue;
                      }
                                               // put paramaters into help text
      while (str.find("#10") != std::string::npos)
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
       str.replace(str.find("#10"), 3, "/home/my_directory");
#endif
#ifdef WIN32
       str.replace(str.find("#10"), 3, "C:\\my_directory");
#endif
      while (str.find("#11") != std::string::npos)
       str.replace(str.find("#11"), 3, acf);
      while (str.find("#1") != std::string::npos)
       str.replace(str.find("#1"), 2, toString(iterations));
      while (str.find("#2") != std::string::npos)
       str.replace(str.find("#2"), 2, toString(subsets));
      while (str.find("#3") != std::string::npos)
       str.replace(str.find("#3"), 2, regkey);
      while (str.find("#4") != std::string::npos)
       str.replace(str.find("#4"), 2, envvar);
      while (str.find("#5") != std::string::npos)
       str.replace(str.find("#5"), 2, smoothing);
      while (str.find("#6") != std::string::npos)
       str.replace(str.find("#6"), 2, sinogram);
      while (str.find("#7") != std::string::npos)
       str.replace(str.find("#7"), 2, toString(v.umap_reco_model)+","+
                   toString(v.alpha)+","+toString(v.beta)+","+
                   toString(v.beta_ip)+","+toString(v.iterations_ip));
      while (str.find("#8") != std::string::npos)
       { std::string par;

         par=toString(v.number_of_segments)+",";
         for (i=0; i < v.number_of_segments; i++)
          par+=toString(v.mu_mean[i])+","+toString(v.mu_std[i])+",";
         for (i=0; i < v.number_of_segments-1; i++)
          par+=toString(v.gauss_intersect[i])+",";
         str.replace(str.find("#8"), 2, par.substr(0, par.length()-1));
       }
      while (str.find("#9") != std::string::npos)
       str.replace(str.find("#9"), 2, toString(v.boxcar_width));
      while (str.find('@') != std::string::npos)
       str.replace(str.find('@'), 1, mu);
      while (str.find(';') != std::string::npos)
       str.replace(str.find(';'), 1, beta);
      while (str.find('$') != std::string::npos)
       str.replace(str.find('$'), 1, "\n");
                                            // print help text with line breaks
      str+=" ";
      count=6-swi.length();
      std::cout << "  -";
      if (swi.length() > 1) { std::cout << "-";                // long option ?
                              count--;
                            }
      std::cout << swi;
      for (i=0; i < count; i++)
       std::cout << " ";
      pos=10;
      while (!str.empty())
       { std::string::size_type w, w2;

         w=str.find(' ');                              // find end of next word
         if ((w2=str.find("\n")) != std::string::npos)
          if ((w2 < w) && (str.substr(w2, 2) != "\n ")) w=w2+1;
         if (pos+w+1 > LINE_LEN)                         // line break needed ?
          { if ((str[w-1] == '\n') && (pos+w == LINE_LEN))
             { std::cout << " " << str.substr(0, w-1)
                         << "\n         ";
               pos=9;
               str.erase(0, w);
               continue;
             }
            std::cout << "\n         ";
            pos=9;
          }
         std::cout << " " << str.substr(0, w);                    // print word
         pos+=w+1;
         if (w>0 && str[w-1] == '\n')                         // new line after word ?
          {  std::cout << "         ";
             pos=9;
             str.erase(0, w);
          }
          else str.erase(0, w+1);
       }
      std::cout << "\n";
      p+=p1+p2;
    }
 }
