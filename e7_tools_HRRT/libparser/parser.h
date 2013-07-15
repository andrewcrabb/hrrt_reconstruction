/*! \file parser.h
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
  \date 2004/07/27 added "--resvr" switch
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
  \date 2010/02/15 added "athr" margin
*/

#ifndef _PARSER_H
#define _PARSER_H

#include <string>
#include <vector>
#include "sinogram_conversion.h"

/*- class definitions -------------------------------------------------------*/

class Parser {
 public:
  // 3d coordinate
  typedef struct {
    float x,                            //< x-coordinate
      y,                                //< y-coordinate
      z;                                //< z-coordinate
  } tcoord3d;
  
  // command line parameters

  float gauss_fwhm_xy,              // FWHM of gaussian for x/y-smoothing in mm
    gauss_fwhm_z,                   // FWHM of gaussian for z-smoothing in mm
    umap_scalefactor,               // scale factor for \f$\mu\f$-map
    alpha,                          // relaxation parameter for \f$\mu\f$-map reconstruction
    beta,                           // regularisation parameter for smoothing prior
    beta_ip,                        // regularisation parameter for intensity prior
    *mu_mean,                       //< mean-values for gaussian priors
    *mu_std,                        // standard deviation for gaussian priors
    *gauss_intersect,               //< boundaries between segments
    gibbs_prior,                    //< gibbs prior
    zoom_factor,                    //< zoom factor for reconstruction
    tof_fwhm,                       //< FWHM of TOF gaussian in ns
    tof_width,                      //< width of TOF bin in ns
    scatter_scale_min,              // minimum value for scatter scaling
    scatter_scale_max,              // maximum value for scatter scaling
    asv,                            //< u-value for auto scaling
    pet_threshold,                  // threshold in PET image for conversion into u-map
    pet_uvalue,                     // u-value after conversion from PET image into u-map
    acf_threshold,                  // attenuation threshold for scatter scaling
    txscatter[2],                   // Transmission scatter factor a and b
    lber,                           // Back and Front LBER
    txblr,                          // TX/BL Ratio
    memory_limit;
  std::vector <float> bed_position; // list of bed positions in mm
  tcoord3d ct2pet_offset;           // offset between CT and PET coordinate system in mm
  std::vector <unsigned short int> kill_blocks,     // list of detector blocks to remove from sinogram
    // amount of available memory that should be used
        
    matrix_num;                     // list of matrix numbers to process
  unsigned short int iterations,    //< number of iterations
    subsets,                        //< number of subsets
    logcode,                        // logging code (level & device)
    mrd,                            //< maximum ring difference
    span,                           //< span
    rebin_r,                        //< radial rebinning factor
    rebin_t,                        // azimuthal rebinning factor
    first_mnr,                      // number of first matrix to process
    last_mnr,                       // number of last matrix to process
    umap_reco_model,                // reconstruction method for \f$\mu\f$-map
    iterations_ip,                  // number of iterations before prior is switched on
    aRhoSamples,                    // number of bins in input acf projection
    aThetaSamples,                  // number of azimuthal angles in input acf sinogram
    oaRhoSamples,                   // number of bins in output acf projection
    oaThetaSamples,                 // number of azimuthal angles in output acf sinogram
    bRhoSamples,                    // number of bins in blank projection
    bThetaSamples,                  // number of azimuthal angles in blank sinogram
    tRhoSamples,                    // number of bins in transmission projection
    tThetaSamples,                  // number of azimuthal angles in transmission sinogram
    nRhoSamples,                    // number of bins in norm projection
    nThetaSamples,                  // number of azimuthal angles in norm sinogram
    eRhoSamples,                    // number of bins in emission projection
    eThetaSamples,                  // number of azimuthal angles in emission sinogram
    sRhoSamples,                    // number of bins in scatter projection
    sThetaSamples,                  // number of azimuthal angles in scatter sinogram
    number_of_segments,             // number of segments in segmentation algorithm
    iwidth,                         //< width of image
    uzoom_xy_factor,                // zoom factor for \f$\mu\f$-map in x/y-direction
    uzoom_z_factor,                 // zoom factor for \f$\mu\f$-map in z-direction
    boxcar_width,                   // width of boxcar filter for scatter scaling
    a_lim,                          // axis up to which single slice rebinning is used
    w_lim,                          //< radial limit for FORE
    k_lim,                          //< azimuthal limit for FORE
    trim,                           // number of bins to trim at each side of sinogram
    scatter_skip,                   // number of border planes to skip in sinogram for scatter simulation
    tof_bins,                       //< number of TOF bins
    blank_db_frames,                // number of frames in blank database
    rs_port,                        // port for communication with reconstruction server
    norm_mask_bins,                 // number of bins to mask out at borders of norm
    acf_threshold_margin;
  std::string logpath,              // acf threshold margin for scatter scaling
    //< path for log file
    quality_path,                   // path for quality control data
    debug_path,                     //< path for debug data
    acf_filename,                   //< name of acf input file
    oacf_filename,                  //< name of acf output file
    blank_filename,                 //< name of blank file
    tra_filename,                   //< name of transmission file
    umap_filename,                  // name of \f$\mu\f$-map input file
    oumap_filename,                 // name of \f$\mu\f$-map output file
    emission_filename,              // name of emission input file
    oemission_filename,             // name of emission output file
    scatter_filename,               // name of scatter input file
    oscatter_filename,              // name of scatter output file
    norm_filename,                  //< name of norm file
    image_filename,                 // name of image input file
    oimage_filename,                // name of image output file
    ct_filename,                    //< name of CT image input file
    swap_path,                      //< name of swap file
    blank_db_path,                  //< path for blank database
    pguid,                          //< patient GUID
    sguid,                          //< study GUID
    cguid,                          // scanogram GUID
    patient_name;                   //< patient name
  std::vector <std::string> iguid;  //< image series GUIDs
  bool quality_control,             //< store quality control data ?
    debug,                          //< store debug data ?
    view_mode,                      //< store sinograms in view mode ?
    no_tilt,                        //< store sinograms untilted ?
    gaussian_smoothing_model,       // use gaussian smoothing model ?
    acf_arc_corrected,              // is input acf arc-corrected ?
    acf_arc_corrected_overwrite,    // is input acf arc-corrected, even if header says different ?
    oacf_arc_corrected,             // store acf with arc-correction ?
    blank_arc_corrected,            //< is blank arc-corrected ?
    blank_arc_corrected_overwrite,  // is blank arc-corrected, even if header says different ?
    tra_arc_corrected,              // is transmission arc-corrected ?
    tra_arc_corrected_overwrite,    // is transmission arc-corrected, even if header says different ?
    emission_arc_corrected,         // is input emission arc-correted ?
    emission_arc_corrected_overwrite, // is input emission arc-corrected, even if header says different ?
    scatter_arc_corrected,          // is input scatter arc-correted ?
    scatter_arc_corrected_overwrite,// is input scatter arc-corrected, even if header says different ?
    oemission_arc_corrected,        // store emission with arc-correction ?
    oscatter_2d,                    // save 2d sinogram and scaling factors ?
    oscatter_arc_corrected,         // store scatter sinogram with arc-correction ?
    segmentation_params_set,        // segmentation parameters specified ?
    randoms_smoothing,              //< randoms smoothing ?
    decay_corr,                     //< decay correction ?
    framelen_corr,                  //< frame-length correction ?
    scatter_corr,                   //< scatter correcion ?
    ucut,                           // cut-off border planes in \f$\mu\f$-map reconstruction ?
    overlap,                        // overlap beds in multi-bed reconstruction ?
    gap_filling,                    //< fill gaps in emission ?
    adim,                           //< number of dimensions in acf
    edim,                           //< number of dimensions in emission
    sdim,                           //< number of dimensions in scatter
    autoscale_umap,                 // scale \f$\mu\f$-map towards water-peak ?
    new_scatter_code,               //< use new scatter code ?
    tof_processing,                 //< use TOF algorithms ?
    debug_iter,                     // store intermediate images in OSEM ?
    p39_patient_norm,               //< is norm a P39 patient norm ?
    apply_ecf,                      //< apply ECF value to image
    force,                          //< overwrite existing files ?
    recoserver,                     // send progress and error information to reconstruction server ?
    append,                         // append output files to existing files ?
    no_radial_arc,                  // never calculate radial arc-corrections ?
    no_axial_arc,                   // never calculate axial arc-corrections ?
    undo_attcor;                    // uncorrect for attenuation before reconstruction ?
  double trues,                     //< number of trues for Poisson noise
    randoms;                        //< number of randoms for Poisson noise
  ALGO::talgorithm algorithm;       // reconstruction algorithm
  SinogramConversion::tirebin_method irebin_method; // rebinning method for 2d->3d
  SinogramConversion::trebin_method rebin_method;   // rebinning method for 3d->2d
  SinogramConversion::tscaninfo si;                 // information about scan
} tparam;
private:
// rule for legal and illegal switch combinations
typedef struct { std::string cmd_switch,                                           //< switch
    rule;                                      //< rule for switch
} truleset;
// rules for legal and illegal switch combinations
static const truleset rules[];
// length of line for usage information
static const unsigned short int LINE_LEN;
// help strings
static const std::string a_str[2], aarc_str[2], algo_str[2], app_str[2],
  asv_str[2], athr_str[2], athrm_str[2], aus_str[2], b_str[2],
  barc_str[2], bc_str[2], bp_str[2], c_str[2],
  cguid_str[2], d_str[2], d2_str[2], dc_str[2],
  dttx_str[2], e_str[2], earc_str[2], ecf_str[2],
  fl_str[2], force_str[2], gf_str[2], gibbs_str[2],
  gxy_str[2], gz_str[2], h_str[2], i_str[2],
  iguid_str[2], is_str[2], k_str[2], l_str[2],
  mat_str[2], mem_str[2], model_str[2], mrd_str[2],
  n_str[2], naarc_str[2], nbm_str[2], newsc_str[2],
  noise_str[2], ntilt_str[2], nosc_str[2],
  np_str[2], nrarc_str[2], oa_str[2], oaarc_str[2],
  oe_str[2], oearc_str[2], offs_str[2], oi_str[2],
  ol_str[2], os_str[2], os2d_str[2], osarc_str[2],
  ou_str[2], lber_str[2],
  p_str[2], pguid_str[2], pn_str[2], prj_str[2],
  q_str[2], qm_str[2], r_str[2], R_str[2],
  resrv_str[2], rs_str[2], s_str[2], sarc_str[2],
  scan_str[2], sguid_str[2], skip_str[2],
  span_str[2], ssf_str[2], swap_str[2], t_str[2],
  tarc_str[2], thr_str[2], tof_str[2], tofp_str[2],
  trim_str[2], txsc_str[2], txblr_str[2],
  u_str[2], ualgo_str[2], uac_str[2],
  ucut_str[2], uf_str[2], uzoom_str[2], v_str[2],
  view_str[2], w_str[2], zoom_str[2];
std::string prgname,                        //< name of application
  switches,                               //< available switches in application
  descr,                                  //< description of application
  date,                                   //< copyright date of application
  param,                                  //< regular expression for command line parameters
  regkey,                                 //< name of registry key
  envvar,                                 //< name of environment variable
  smoothing,                              //< name of dataset to which smoothing is applied
  sinogram,                               //< name of sinogram
  acf;                                    //< name of acf
unsigned short int iterations,              //< default number of iterations
  subsets;                                //< default number of subsets
Parser::tparam v;                           //< command line parameters
// convert string into float
float a2f(const std::string, const std::string, bool * const) const;
// check expression for switch
bool checkExpression(const std::string, const std::string,
                     std::string) const;
// check if file exists
bool checkFile(const std::string, const std::string, const std::string,
               const bool, const std::string) const;
// check if path exists
bool checkPath(const std::string, const std::string, const std::string,
               const std::string) const;
// check rule for switch
bool checkRule(const std::string, const std::string, std::string) const;
// check for illegal switch combinations
bool checkSemantics(std::string) const;
// replace '\' in a string by '/'
std::string convertSlash(std::string) const;
// parse filename from command line
void parseFilename(std::string, std::string * const, const std::string,
                   bool * const, unsigned short int * const,
                   unsigned short int * const, bool * const) const;
public:
// initialize parser
Parser(const std::string, const std::string, const std::string,
       const std::string, const std::string, const unsigned short int,
       const unsigned short int, const std::string, const std::string,
       const std::string, const std::string, const std::string);
~Parser();                                                                        // destroy object
// convert string into integer
static signed long int a2i(const std::string, const std::string,
                           bool * const);
// request structure with command line parameters
Parser::tparam *params() const;
bool parse(int, char **);                                           // parse command line
std::string patternKey() const;               // get key for swapping mechanism
void usage(const bool) const;                              // print usage information
};

#endif
