// lmhistogram.cpp : Defines the entry point for the g_logger application.
/*
    ------------------------------------------------

    Component       : HRRT

    Authors         : Mark Lenox, Merence Sibomana
    Language        : C++
    Platform        : Microsoft Windows.

    Creation Date   : 04/02/2003
    Modification history:
    12/07/2004 : change pgm_id to "V2.0 07-DEC-2004"
    12/10/2004 : Bug 041210-1 fixed, change pgm_id to "V2.0 10-DEC-2004",
    Added code to make random sinogram output an option.
    12/21/2004 : use file_status to test reader startup, change pgm_id to "V2.0 27-DEC-2004"
    02/02/2005 : Add logging, set default span to 9 and remove -byte option
    05/17/2005 : Reset coincidence map counters before each frame.
    06/07/2005 : Close true sinogram files in PR mode
    Use float rather than double for variable read from header
    11/30/2005 : Add HRRT low Resolution and P39 support
    Add nodoi support
    bug fix: max number of frames 256, crash for mframes > 64
    12/21/2005: Use mhdr in .dyn file for P39
    Remove listmode mhdr information from sinogram headers
    Assume listmode.l32.hdr contains P39 main header (patient) information
    01/04/2006: Bug fix: Remove sinogram keys from P39 main header
    02/09/2006: Create random sino only when requested
    Trues  sino for scatter correction created in span 9 for both span 3 and 9
    Exclude border crystals by default and provide an option to keep them (-ib)
    Use bitmask hor rebinner method keyword
    02/15/2006: Allow 2 low resoltion mode with 2mm and 2.4375mm binsize
    Suppress global variables n_elements,n_angles and use nprojs,nviews
    03/10/2006: Bug fix, Include border by default
    10/28/2007: Removed  reset coincidence counters before each frame,
    Event skip was to moved lm64_reader calling check_start_of_frame()
    04/03/2008: Added -start option to start histogramming at specified countrate
    01/07/2009: Wait while file status OK to support skip mode in start_reader_thread()
    Clean warnings and log rebinner startup errors
    Make a local copy of skip and duration arrays to fix skip bug
    20-MAY-2009: Use a single fast LUT rebinner
    ahc
    2/6/13
    Made hrrt_rebinner.lut a required cmd line arg 'r'.
    Removed call to hrrt_rebinner_lut_path in LM_Rebinner_mp.cpp::init_rebinner()
    8/23/18
    Add Boost::ProgramOptions for cmd line arg parsing - remove home-made Flags.cpp
*/

// #include "Flags.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "CHeader.hpp"
#include "dtc.hpp"
#include "Errors.hpp"
#include "LM_Rebinner_mp.hpp"
#include "histogram_mp.hpp"
#include "LM_Reader_mp.hpp"
#include "convert_span9.hpp"
#include "hrrt_util.hpp"
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h" //support for stdout logging
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/rotating_file_sink.h" // support for rotating file logging

// ahc
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/ostream.h>

#define DEFAULT_LLD 400 // assume a value of 400 if not specified in listmode header
const std::string RW_MODE = "wb+";
const std::string pgm_id = "V2.1 ";  // Revision changed from 2.0 to 2.1 for LR and P39 support

#define MAX_FRAMES 256
#define MAX_THREADS 4

namespace bt = boost::posix_time;
namespace bg = boost::gregorian;
namespace po = boost::program_options; 
namespace fs = boost::filesystem;
// namespace bx = boost::xpressive;

const std::vector<int> NSINOS {
    207, // span 0 (specific to transmission)
    104, // span 1 (direct planes only)
    0,   // span 2 (invalid)
    6367,// span 3
    0,   // span 4 (invalid)
    3873,// span 5
    0,   // span 6 (invalid)
    2927,// span 7
    0,   // span 8 (invalid)
    2209,// span 9
};

std::vector<int> LR_NSINOS {
    103, // span 0 (specific to transmission)
    103, // span 1 (direct planes only)
    0,   // span 2 (invalid)
    0,  // span 3
    0,   // span 4 (invalid)
    1041,// span 5
    0,   // span 6 (invalid)
    773,// span 7
    0,   // span 8 (invalid)
    0,// span 9
};

std::vector<int> P39_NSINOS {
    239, // span 0 (specific to transmission)
    0, // span 1 (direct planes only)
    0,   // span 2 (invalid)
    0,// span 3
    0,   // span 4 (invalid)
    0,// span 5
    0,   // span 6 (invalid)
    3935,// span 7
    0,   // span 8 (invalid)
    0,// span 9
};

fs::path g_fname_l64;     // input listmode file name
fs::path g_fname_l64_hdr; // input listmode header file name
fs::path g_out_fname;    // output sinogram file basename
fs::path g_out_fname_hc; // output Head Curve file name
fs::path g_out_fname_sino; //current frame sinogram file name
fs::path g_out_fname_hdr;  //current frame sinogram header file name
fs::path g_out_fname_ra;   //current frame sinogram randoms file name
fs::path g_out_fname_pr;   //current frame sinogram prompts file name
fs::path g_out_fname_tr;   //current frame sinogram Trues file name

// fs::path outfname_p39_em[FILENAME_MAX]; //current frame sinogram emission file name
// fs::path outfname_p39_tx[FILENAME_MAX]; //current frame sinogram transmission file name

// auto g_logger = spdlog::stdout_color_mt("g_logger");
std::string g_logfile;

auto g_logger;

static int p39_nsegs = 25;
static int MODEL_HRRT = 328;
static const char *p39_seg_table = "{239,231,231,217,217,203,203,189,189,175,175,161,161,147,147,133,133,119,119,105,105,91,91,77,77}";

static char fname_p39_mhdr[FILENAME_MAX]; // input listmode main header file name
std::string g_outfname_mock; //Mock sinogram file name for transmission only

std::ofstream g_out_hc;             // Head Curve output File Pointer
std::ofstream g_out_true_prompt_sino;    // output trues or prompts sinogram File Pointer
std::ofstream g_out_ran_sino;   // output randoms sinogram File Pointer
std::ofstream g_out_true_sino;  // output Trues sinogram for separate prompts and randoms mode

std::vector<int> g_frames_duration;    // Frames duration
std::vector<int> g_skip;               // Frames optional skip

static int g_dose_delay = 0;          // elapsed time between dose assay time and scan start time
static int relative_start = 0;       // Current frame start time
static int prev_duration = 0;        // Previous sinogram duration (when -add option)
static long scan_duration = 0;          // P39 TX duration is total time scan duration

// Parameters which default may be overrided in lmhistogram.ini
static int g_span = 9;                // Default span=9 for l64 if not specified in header
static size_t elem_size = 2;        // sinogram elem size: 1 for byte, 2 for short (default)
static int g_num_sino, g_sinogram_size;  // number of sinogram and size
static int compression = 0;         // 0=no compression (default), 1=ER (Efficient Representation)
static int output_ra = 0;           // output randoms

static bool g_span_override = false;       // Flag set span when span specified at command line
// static int nframes = 0;             // Number of frames
bool do_scan = false;                // scan option , default=no
// static int split = 0;               // split option, default=no
static int out_l32 = 0;             // Output 32-bit listmode (output file extension .l32)
static int out_l64 = 0;             // Output 64-bit listmode (output file extension .l64)

std::string g_prev_sino;        // 1: add to existing normalization scan
// static char *log_fname = nullptr;          // log file
// static FILE *log_fp = nullptr;         // log file ptr
static float decay_rate = 0.0;      // Default no decay correction in seconds
static int g_lld = DEFAULT_LLD;

// ahc: hrrt_rebinner.lut is now a required cmd line arg
std::string g_lut_file;

//Crystals and Ring dimensions in cm
#define PITCH 0.24375f
#define RDIAM 46.9f
#define LTHICK 1.0f
#define BLK_SIZE 8      // Number of crystals per block

const char *sw_version = "HRRT_U 1.1";
std::string g_sw_build_id;


/**
 * init_sino:
 * Allocates the sinogram in memory and initializes it with 0 or sinogram loaded from the g_prev_sino argument.
 * Sets the duration with g_prev_sino duration value from g_prev_sino header.
 * Returns 1 if success and 0 otherwise
 */
template <class T> int init_sino(T *&sino, char *&delayed, int init_span, const std::string &prev_sino, int &duration) {
  int sinogram_subsize = 0;
  int r_size = sizeof(T);

  if (check_model(model_number, MODEL_HRRT)) {
    // HRRT
    g_logger->info("Using HRRT model");
    if (g_hist_mode != HM_TRA) {
      // emission
      g_logger->debug("Using HRRT Emission: g_hist_mode = {}", g_hist_mode);
      g_num_sino = (LR_type == 0) ? NSINOS[span] : LR_NSINOS[span];
      if (g_hist_mode == 1) {
        // prompts and delayed
        sinogram_subsize = g_sinogram_size;
      }
    } else {
      // transmission
      g_logger->debug("XX Using HRRT Transmission, g_hist_mode = {}", g_hist_mode);
      g_num_sino = (LR_type == 0) ? NSINOS[0] : LR_NSINOS[0];
      sinogram_subsize = g_sinogram_size; // mock memory
    }
    g_sinogram_size = m_nprojs * m_nviews * g_num_sino;
  }

  if (g_num_sino == 0) {
    g_logger->error("Unsupported span : {}", init_span);
    return 0;
  }

  g_logger->info("number of elements: {}"  << m_nprojs);
  g_logger->info("number of angles: {}"    << m_nviews);
  g_logger->info("number of sinograms: {}" << g_num_sino);
  g_logger->info("bits per register: {}"   << r_size * 8);
  g_logger->info("sinogram size: {}"       << g_sinogram_size);
  g_logger->info("sinogram subsize: {}"    << sinogram_subsize);
  if (sino == nullptr) {
    sino = (T *)calloc(g_sinogram_size + sinogram_subsize, r_size);
  } else {
    memset(sino, 0, (g_sinogram_size + sinogram_subsize)*r_size);
  }
  if (!sino) {
    g_logger->error("Unable to allocate sinogram memory: {}", (g_sinogram_size + sinogram_subsize) * r_size);
    return 0;
  }

  // load previous scan if any
  if (prev_sino.length() > 0) {
    CHeader hdr;
    std::string hdr_fname;
    if (g_hist_mode != 0) {
      g_logger->error("Adding to existing sinogram only supported Trues mode");
      free(sino);
      return 0;
    }
    hdr_fname = fmt::format("{}.hdr", prev_sino);
    if (hdr.OpenFile(hdr_fname) != OK) {
      g_logger->error("Error opening {}", hdr_fname);
      free(sino);
      return 0;
    }
    int ok = hdr.Readint("image duration", duration);
    hdr.CloseFile();
    if (ok != 0) {
      g_logger->error("{}: image duration not found", hdr_fname);
      free(sino);
      return 0;
    }
    // FILE *prev_fp = fopen(prev_sino.c_str(), "rb");
    std::ifstream prev_fp = open_istream(prev_sino, , ios::in | ios::binary)
    if (prev_fp.open()) {
      // int count = fread(sino, g_sinogram_size, r_size, prev_fp);
      prev_fp.read(sino, g_sinogram_size * r_size);
      if (!prev_fp.good()) {
        g_logger->error("Error reading {}", prev_sino);
        prev_fp.close();
        free(sino);
        return 0;
      }
      prev_fp.close();
    } else {
      g_logger->error("Could not open prev_sino: {}", prev_sino);
      free(sino);
      return 0;
    }
  }
  return 1;
}

void on_span(int span) {
    if (span != 3 && span != 9) {
      g_logger->errof("Span must be 3 or 9");
      exit(1);
    }
    g_span_override = true;
    g_span = span;
}

// Cmd line param 'out': Create output file path

void on_out(const std::string &outstr) {
  g_out_fname = bf::path(outstr);
}

void on_in(const std::string &instr) {
  g_fname_l64 = bf::path(instr);
}

void on_lrtype(LR_Type lr_type) {
  switch (type) {
  case LR_20:
    LR_type = LR_20;
    break;
  case LR_24:
    LR_type = LR_24;
    break;
  default:
    std::cerr << "*** invalid LR mode ***" << std::endl;
    exit(0);
  }
  g_span = 7;
  max_rd = 38;
}

void on_pr(bool pr) {
  if (pr) g_hist_mode = 1;
}

void on_ra(bool ra) {
  if (ra) output_ra = 1;
}

void on_p(bool p) {
  if (p) g_hist_mode = 2;
}

void on_notimetag(bool n) {
  if (n) timetag_processing = 0;
}

void on_nodoi(bool n) {
  if (n) rebinner_method = rebinner_method | NODOI_PROCESSING;  // ignore DOI
}

void on_eb(bool b) {
  if (b) rebinner_method = rebinner_method | IGNORE_BORDER_CRYSTAL;  // exclude Border crystals
}

void assert_positive(int count) {
  if (count < 0) {
    g_logger->error("Value must be positive");
    exit(1);
  }
}

// Parse duration string and fill in g_skip and g_frames_duration

void parse_duration_string(std::string durat_str) {
  regex re_skip("(?P<skip>[0-9]+)\\,(<?P<dura>[0-9]+)");
  regex re_mult("(?P<dura>[0-9]+)\\*(<?P<mult>[0-9]+)");

    boost::cmatch match;
    if (boost::regex_match(durat_str, match, re_skip)) {
      g_skip.push_back(match["skip"]);
      g_frames_duration.push_back(match["dura"]);
    } else if (boost::regex_match(durat_str, match, re_mult)) {
      vector<int> durats(match["mult"], match["dura"]);
      g_frames_duration.insert(std::end(g_frames_duration), std::begin(durats), std::end(durats));
    }
}

// Parse duration strings 

void on_duration(std::vector<std::string> vs) {
  for (auto it = vs.begin(); it != vs.end(); ++it) {
    parse_duration_string(it);
  }
}

void on_verbosity(int verbosity) {
  switch (verbosity) {
    case 0: g_logger->set_level(spdlog::level::off  ); break;
    case 1: g_logger->set_level(spdlog::level::err  ); break;
    case 2: g_logger->set_level(spdlog::level::info ); break;
    case 3: g_logger->set_level(spdlog::level::debug); break;
    default: 
    g_logger->error("Bad log level: {}", verbosity)
  }
}

void validate_arguments(void) {

  if (g_prev_sino) {
    std::string errmsg;
    if (g_hist_mode != 0)
      errmsg = "Adding to existing sinogram only supported in Trues mode";
    if (g_frames_duration.size() > 1)
      errmsg = "Adding to existing sinogram only supported multi-frame mode";
    if (errmsg.length() > 0) {
      g_logger->error(errmsg);
      exit(1);
    }
  }
}

void parse_boost(int argc, char **argv) {
  try {
    options_description desc("Options");
    desc.add_options()
    ("help,h", "Show options")
    ("out,o"        , po::value<string>()->notifier(&on_out)                                , "output sinogram, 32 or 64-bit listmode file from file extension")
    ("infile,i"     , po::value<std::string>(&g_fname_l64_str)                            , "Input file")
    ("lr_type,L"    , po::value<LR_Type>()->notifier(on_lrtype)                      , "low resolution (mode 1/2: binsize 2/2.4375mm, nbins 160/128, span 7, maxrd 38)")
    ("span"         , po::value<int>()->notifier(on_span)                            , "Span size - valid values: 0(TX), 1,3,5,7,9")
    ("PR"           , po::bool_switch()->notifier(&on_pr)                            , "Separate prompts and randoms")
    ("ra"           , po::bool_switch()->notifier(&on_ra)                            , "Output randoms sinogram file (emission only)")
    ("prompts,P"    , po::bool_switch()->notifier(&on_p)                             , "Prompts only")
    ("notimetag"    , po::bool_switch()->notifier(&on_notimetag)                     , "No timetag processing, use timetag count as time")
    ("nodoi"        , po::bool_switch()->notifier(&on_nodoi)                         , "No DoI processing, back layer events processed as front layer")
    ("scan"         , po::bool_switch(&do_scan)->default_value(false)                , "Scan file for prompts and randoms, print headcurve to g_logger")
    ("model"        , po::value<int>(&model_number)->default_value(MODEL_HRRT)       , "Model: 328, 2393, 2395")
    ("verbosity,V"  , po::value<int>(&verbosity)->notifier(&on_verbosity)            , "Logging verbosity: 0 off, 1 err, 2 info, 3 debug")
    ("add"          , po::value<std::string>(&g_prev_sino)                           , "Add to existing sinogram file")
    ("log,l"        , po::value<std::string>(&g_logfile)                             , "Log file")
    ("mock"         , po::value < std::string(&g_outfname_mock)                      , "Output shifted mock sinogram file (transmission only)")
    ("EB"           , po::bool_switch()->notifier(&on_eb)                            , "Exclude border crystals")
    ("count"        , po::value<int>(&stop_count)->notifier(&assert_positive)        , "Stop after N events")
    ("start"        , po::value<int>(&start_countrate)->notifier(&assert_positive)   , "Start histogramming when trues/sec is higher than N")
    ("lut_file,r"   , po::value<std::string>(&g_lut_file)->required()                , "Full path to rebinner.lut file" )
    ("duration,d"   , po::value<std::vector<std: string>>()
     ->multitoken()->composing()->notifier(&on_duration) , "Frame [duration] or [duration,skip] or [duration*repeat]")
    ;
    po::positional_options_description pos_opts;
    pos_opts.add("infile", 1);

    po::variables_map vm;
    po::store(
      po::command_line_parser(argc, argv)
      .options(desc)
      .style(
        po::command_line_style::unix_style
        | po::command_line_style::allow_long_disguise)
      .positional(pos_opts)
      .run(), vm
    );
    po::notify(vm);
    if (vm.count("help"))
      std::cout << desc << '\n';
  }
  catch (const po::error &ex) {
    std::cerr << ex.what() << std::endl;
  }
}

// void start_reader_thread()
// Create buffers, Open file listmode file
void start_reader_thread() {
  // Create container synchronization event and
  // start reader thread and wait for container ready (half full)
  L64EventPacket::frame_duration = (int *)calloc(g_frames_duration.size(), sizeof(int));
  memcpy(L64EventPacket::frame_duration, g_frames_duration, g_frames_duration.size() * sizeof(int));
  L64EventPacket::frame_skip = (int *)calloc(g_frames_duration.size(), sizeof(int));
  mcpy(L64EventPacket::frame_skip, g_frames_duration.size() * sizeof(int));
  lm64_reader(g_fname_l64);
}

// Start thread and wait until it is ready

inline void start_rebinner_thread() {
    int ID1 = 0, ID2 = 1, ID3 = 2, ID4 = 3;
    int max_try = 30;
    int ncrystals = NDOIS * NXCRYS * NYCRYS * NHEADS;
    int head_nblks = (NXCRYS / 8) * (NYCRYS / 8); // 9*13=117
    p_coinc_map = (unsigned *)calloc(ncrystals, sizeof(unsigned));
    d_coinc_map = (unsigned *)calloc(ncrystals, sizeof(unsigned));
}

inline int get_num_cpus() {
    int num_cpus = 1;
    const char *s = getenv("NUMBER_OF_PROCESSORS");
    if (s != nullptr) 
      sscanf(s, "%d", &num_cpus);
  if (num_cpus > MAX_THREADS)
    num_cpus = MAX_THREADS; // workaround for dual quadcore bug

    return num_cpus;
}

/**
 * create_files:
 * Create/Open output files before starting histogramming.
 * Log error and exit if file creation fails
 */
static void create_files() {
  g_out_fname_hdr = make_file_name(SINO_HDR);
  g_out_fname_hc  = make_file_name(LM_HC);

  g_out_hc = open_ostream(g_out_fname_hc);

  if (g_hist_mode == HM_TRA) {
    // transmission
    if (elem_size != 2) {
      g_logger->error("Byte format not supported in transmission mode");
      exit(1);
    }
    g_out_true_prompt_sino = open_ostream(g_out_fname_sino, ios::out | ios::app | ios::binary);
    if (g_outfname_mock.length() > 0) {
      g_out_ran_sino = open_ostream(g_outfname_mock, ios::out | ios::app | ios::binary);
    }
  } else if (g_hist_mode == 1) {
    // Keep original filename for prompts in prompts or prompts+delayed mode
    g_out_fname_pr = g_out_fname_sino;
    g_out_true_prompt_sino = open_ostream(g_out_fname_pr, ios::out | ios::app | ios::binary);
    g_out_fname_ra = make_file_name(FT_RA_S);
    g_out_ran_sino = open_ostream(g_out_fname_ra, ios::out | ios::app | ios::binary);
    if (elem_size == 2) {
      g_out_fname_tr = make_file_name(FT_TR_S);
      g_out_true_sino = open_ostream(g_out_fname_tr, ios::out | ios::app | ios::binary);
    }
  } else {
    g_out_true_prompt_sino = open_ostream(g_out_fname_sino, ios::out | ios::app | ios::binary);

  }
  g_logger->info("Output File: {}", g_out_fname);
  g_logger->info("Output File Header: {}", g_out_fname_hdr);
}

/**
 * close_files:
 * Close output files created by create_files
 */
static void close_files()
{
    g_out_true_prompt_sino.close();
    g_out_ran_sino.close();
    g_out_true_sino.close();
    g_out_hc.close();
}

/**
 * write_sino:
 * Write sinogram and header to file.
 * Reuse existing listmode header to create sinogram header.
 * Header file name is derived from data file name (data_fname.hdr).
 * Log error and exit when an I/O error is encountered.
 */
static void write_sino(char *sino, int sino_size, CHeader &hdr, int frame) {
    // create the output header file
    char *p = nullptr;
    short *ssino = (short *)sino;
    short *delayed = (short *)(sino + sino_size * elem_size);
    short *mock = (short *)(sino + sino_size * elem_size);
    int i = 0, span9_sino_size = m_nprojs * m_nviews * NSINOS[9];

    hdr.WriteTag(HDR_IMAGE_DURATION, prev_duration + g_frames_duration[frame]);
    hdr.WriteTag(HDR_IMAGE_RELATIVE_START_TIME, relative_start);
    int decay_time = relative_start + g_dose_delay;
    float dcf1 = 1.0f;
    float dcf2 = 1.0f;
    if (decay_rate > 0.0) {
        dcf1 = (float)pow(2.0f, decay_time / decay_rate);
        //      dcf2 = ((float)log(2.0) * duration[frame]) / (decay_rate * (1.0 - (float)pow(2,-1.0*duration[frame]/decay_rate)));
        dcf2 = (float)((log(2.0) * g_frames_duration[frame]) / (decay_rate * (1.0 - pow(2.0f, -1.0f * g_frames_duration[frame] / decay_rate))));
    }
    hdr.WriteTag(HDR_DECAY_CORRECTION_FACTOR, dcf1);
    hdr.WriteTag(HDR_DECAY_CORRECTION_FACTOR2, dcf2);
    hdr.WriteTag(HDR_FRAME, frame);
    hdr.WriteTag(HDR_TOTAL_PROMPTS, total_prompts());
    hdr.WriteTag(HDR_TOTAL_RANDOMS, total_randoms());
    hdr.WriteTag(HDR_TOTAL_NET_TRUES, total_prompts() - total_randoms());
    int av_singles = 0;
    for (int block = 0; block < NBLOCKS; block++) {
        std::string tmp1 = format::fmt("block singles {:d}", block);
        hdr.WriteTag(tmp1, singles_rate(block));
        av_singles += singles_rate(block);
    }
    av_singles /= NBLOCKS;
    hdr.WriteTag("average singles per block", av_singles);
    g_logger->info("g_lld = {}, Average Singles Per Block = ", g_lld, av_singles)
    hdr.WriteTag(HDR_DEAD_TIME_CORRECTION_FACTOR, GetDTC(g_lld, av_singles));

    // Write Trues or Prompts sinogram
    int count = elem_size, write_error = 0;
    g_logger->info("sinogram size: {:d}", sino_size);
    g_logger->info("element size: {:d}", elem_size);

    // Log message and set first data filename
    switch (g_hist_mode) {
    case 0: // Randoms substractions
    g_logger->info("Writing Trues Sinogram: {}", g_out_fname_sino);
    hdr.WriteTag(HDR_NAME_OF_DATA_FILE, g_out_fname_sino);
    hdr.WriteTag(HDR_SINOGRAM_DATA_TYPE, "true");
    // count = fwrite(sino, sino_size, elem_size, g_out_true_prompt_sino);
    g_out_true_prompt_sino.write(sino, sino_size * elem_size);
    break;
    case 1: // Prompts and delayed: prompts is first
    g_logger->info("Writing Prompts Sinogram: {}", g_out_fname_pr);
    hdr.WriteTag(HDR_NAME_OF_DATA_FILE, g_out_fname_pr);
    hdr.WriteTag(HDR_SINOGRAM_DATA_TYPE, "prompt");
    hdr.WriteTag("!name of true data file", g_out_fname_tr);
    g_out_fname_hdr = fmt::format("{}.hdr", g_out_fname_pr);
    g_out_true_prompt_sino.write(sino, sino_size * elem_size);
    break;
    case 2: //Prompts only
    g_logger->info("Writing Prompts Sinogram: {}", g_out_fname_sino);
    hdr.WriteTag(HDR_NAME_OF_DATA_FILE, g_out_fname_sino);
    hdr.WriteTag(HDR_SINOGRAM_DATA_TYPE, "prompt");
    break;
    case 7: // Transmission
    g_logger->info("Writing EC corrected Sinogram: {}", g_out_fname_sino);
    g_out_fname_hdr = fmt::format("{}.hdr", g_out_fname_sino);
    hdr.WriteTag(HDR_NAME_OF_DATA_FILE, g_out_fname_sino);
    for (int i = 0; i < sino_size; i++) {
        ssino[i] = ssino[i] - mock[i];
        if (ssino[i] < 0) ssino[i] = 0;
    }
    g_out_true_prompt_sino.write(sino, sino_size * elem_size);
    if (g_out_true_prompt_sino.good())
        hdr.WriteFile(g_out_fname_hdr);
    } else {
        g_logger->error("Error writing %s\n", g_out_fname_sino);
        write_error++;
    }
    break;
}

    // Write data
if (count != (int)elem_size) {
    g_logger->error("Error writing file");
    write_error++;
}
if (!write_error) 
  hdr.WriteFile(g_out_fname_hdr);

  // Log message and write second data filename
  if (elem_size == 2 && write_error == 0)
    switch (g_hist_mode) {
    case 1: // Prompts and delayed: trues is second if short type
      g_out_fname_hdr = fmt::format("{}.hdr", g_out_fname_tr);
      hdr.WriteTag(HDR_NAME_OF_DATA_FILE, g_out_fname_tr);
      hdr.WriteTag(HDR_SINOGRAM_DATA_TYPE, "true");
      // convert span3 prompt sinogram to span9 and substract delayed

      for (i = 0; i < sino_size; i++)
        ssino[i] =  ssino[i] - delayed[i];
      if (g_span == 3) {
        g_logger->info("Converting true sino to span9");
        convert_span9(ssino, max_rd, 104);
        hdr.WriteTag(HDR_AXIAL_COMPRESSION, 9);
        hdr.WriteTag(HDR_MATRIX_SIZE_3, NSINOS[9]);
      }
      g_logger->info("Writing Trues Sinogram: {}", g_out_fname_tr);
      // if (fwrite(ssino, g_span == 3 ? span9_sino_size : sino_size, elem_size, g_out_true_sino) == elem_size) {
      g_out_true_sino.write(ssino, elem_size * g_span == 3 ? span9_sino_size : sino_size);
      if (g_out_true_sino.good()) {
        hdr.WriteFile(g_out_fname_hdr);
      } else {
        g_logger->error("Error writing {}", g_out_fname_tr);
        write_error++;
      }
      // Restore span3 header
      if (g_span == 3) {
        hdr.WriteTag(HDR_AXIAL_COMPRESSION, 3);
        hdr.WriteTag(HDR_MATRIX_SIZE_3, NSINOS[3]);
      }
      break;
    case 7: // Transmission: EC corrected is second if short type
      if (g_out_ran_sino.is_open()) {
        g_logger->info("Writing Mock Sinogram: {}", g_outfname_mock);
        g_out_fname_hdr = fmt::format("{}.hdr", g_outfname_mock);
        hdr.WriteTag(HDR_NAME_OF_DATA_FILE, g_outfname_mock);
        // if (fwrite(mock, sino_size, elem_size, g_out_ran_sino) == elem_size) {
        g_out_ran_sino.write(mock, sino_size * elem_size);
        if (g_out_ran_sino.good()) {
          hdr.WriteFile(g_out_fname_hdr);
        } else {
          g_logger->error("Error writing {}", g_outfname_mock);
          write_error++;
        }
        break;
      }
    }

  // Log message and write third data filename
  if (output_ra && write_error == 0 && g_hist_mode == 1) {
    g_logger->info("Writing Randoms Sinogram: {}", g_out_fname_ra);
    g_out_fname_hdr = fmt::format("{}.hdr", g_out_fname_ra);
    hdr.WriteTag(HDR_NAME_OF_DATA_FILE, g_out_fname_ra);
    hdr.WriteTag(HDR_SINOGRAM_DATA_TYPE, "delayed");
    // count = fwrite(delayed, span9_sino_size, elem_size, g_out_ran_sino);
    // if (count == (int)elem_size) {
    g_out_ran_sino.write(delayed, span9_sino_size * elem_size);
    if (g_out_ran_sino.good()) {
      hdr.WriteFile(g_out_fname_hdr);
    } else {
      g_logger->error("Error writing {}", g_out_fname_ra);
      write_error++;
    }
  }
  if (write_error)
    exit(1);
}

// Move this to Header.cpp and remove Boost dependency from this translation unit.

static int get_dose_delay(CHeader &hdr, int &t) {
  bt::ptime assay_time, study_time;
  int ret = 1;

  if (!hdr.ReadTime(HDR_DOSE_ASSAY_TIME, assay_time)) {
    if (!hdr.ReadTime(HDR_STUDY_TIME, study_time)) {
      bt::time_duration diff = study_time - assay_time;
      t = diff.total_seconds();
      ret = 0;
    }
  }
  return ret;
}

int do_lmscan() {
  // transmission tag processing uses parameters that need to be initialized
  // reset frame duration otherwise lm_reader splits data into frames and waits
  // for the frame to be written.
  // strcpy(g_out_fname_hc, g_fname_l64);
  // // Replace extension by "_lm.hc"
  // char *ext = strrchr(g_out_fname_hc, '.');
  // if (ext != nullptr)
  //   strcpy(ext, "_lm.hc");
  // else
  //   strcat(g_out_fname_hc, "_lm.hc");

  // FILE *outfile_hc = fopen(g_out_fname_hc, "w");
  // if (outfile_hc == nullptr) {
  //   g_logger->error(g_out_fname_hc);
  //   exit(1);
  // }
  // memset(g_frames_duration, 0, sizeof(g_frames_duration));

  g_out_fname_hc = make_file_name(FT_SINO_HC);
  std::ofstream outfile_hc = open_ostream(g_out_fname_hc);
  g_frames_duration.clear();

  start_reader_thread();
  lmscan(outfile_hc, &scan_duration);
  fclose(outfile_hc);
        //      for (int index = 0; index < duration; index++)
        //          cout << hc[index].time << " " << hc[index].randoms_rate << " " << hc[index].trues_rate << " " << hc[index].singles << endl;
  g_logger->info("Scan Duration = {:d}", scan_duration)

}

/**
 * @brief      Makes a file name from the input file g_fname_l64
 * @param[in]  file_type  The file type
 * @return     String 
 */
fs::path make_file_name(FILE_TYPE file_type) {
  if (file_type >= FT_END) {
    logger->error("Invalid file_type: {}", file_type);
    exit(1);
  }
  fs::path file_path = g_fname_l64.replace_extension(FILE_EXTENSIONS[file_type]);
  g_logger->info("make_file_name({}): {}", file_type, file_path.string());
  return file_path;
}

const std::string time_string(void) {
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, 80, "%y%m%d_%H%M%S", timeinfo);
  std::string s(buffer);
  return s;
}

void init_logging(void) {
  if (g_logfile.length() == 0) {
    g_logfile = fmt::format("{}_lmhistogram.log", time_string());
  }
  g_logger = spdlog::basic_logger_mt("basic_logger", g_logfile);
}

int open_l64_hdr_file(void) {
  CHeader *hdr = new CHeader;
  g_fname_l64_hdr = make_file_name(FT_L64_HDR);
  if (hdr->OpenFile(g_fname_l64_hdr) != OK) {
    g_logger->error("header {} not found", make_file_name(FT_L64_HDR));
    hdr = nullptr;
  }
  return hdr;
}

int check_input_files(void) {
  int ret{0};
  if (! bf::is_regular_file(g_fname_l64)) {
    g_logger->error("{}: File not found", g_fname_l64);
    ret = 1;
  }
  return ret;
}

int set_tx_source_speed(const CHeader &hdr) {
  string datatype;
  // for transmission scans, always override span to 0 (segment 0 only)
  if (hdr.Readchar(HDR_PET_DATA_TYPE, datatype) == OK) {
    g_logger->info("Data Type: {}", datatype);
    // ahc this was failing because of trailing CR on DOS format hdr file.
    if (datatype == "transmission") {
      float axial_velocity = 0.0;
      g_span = 0;
      g_hist_mode = HM_TRA;
      g_logger->info("Data Type: {}, g_hist_mode: {}", datatype, g_hist_mode);
      if (hdr.Readfloat("axial velocity", axial_velocity) == 0) {
        // tx speed=6msec/crystal for axial velocity=100%
        g_tx_source_speed = 6.25f / axial_velocity / 100.0f;
        g_logger->info("Axial velocity = {}, TX source speed = {}", axial_velocity, g_tx_source_speed);
      }
    }
  }
  return 0;
}

int set_lld(const CHeader &hdr) {
  if (hdr.Readint("lower level", g_lld) != OK)
    g_lld = DEFAULT_LLD;
  g_logger->info("g_lld: {}", g_lld);
  return 0;
}

int set_histogramming_mode(const CHeader &hdr) {
  int sino_mode = HM_TRUE;  // by default use net trues
  if (hdr.Readint("Sinogram Mode", sino_mode) == OK) {
    g_logger->info("Using Sinogram Mode from header: {}", sino_mode);
    if ((sino_mode != HM_TRUE) && g_prev_sino) {
      g_logger->error("Adding to existing sinogram only supported in Trues mode");
      exit(1);
    }
    g_hist_mode = sino_mode;
  }
  return 0;
}

int set_isotope_halflife(const CHeader &hdr) {
  float isotope_halflife = 0.0;
  hdr.Readfloat("isotope halflife", isotope_halflife);
  if (isotope_halflife != 0.0) {
    g_decay_rate = (float)isotope_halflife;
  } else {
    g_logger->error("No isotope halflife");
    exit(1);
  }
}

int set_span(const CHeader &hdr) {
  int axial_comp;

  if (!hdr.Readint(HDR_AXIAL_COMPRESSION, axial_comp)) {
    if (!g_span_override && axial_comp > 0)
      g_span = axial_comp;
    else
      hdr.WriteTag(HDR_AXIAL_COMPRESSION, g_span);
  }
  return 0;
}

void do_histogramming(const CHeader &hdr) {

  // set output filename if an input has not been given
  if (g_out_fname.string().length() == 0) {
    g_out_fname = make_file_name(FT_SINO);
    g_logger->info("g_out_fname: {}", g_out_fname);
  }

  set_lld(hdr);
  set_histogramming_mode(hdr);
  set_isotope_halflife(hdr);
  get_dose_delay(hdr, g_dose_delay);
  set_span(hdr);

    // update the header with sinogram info for HRRT
    hdr.WriteTag("!originating system"      , "HRRT");
    hdr.WriteTag(HDR_NAME_OF_DATA_FILE      , g_out_fname_sino);
    hdr.WriteTag(HDR_NUMBER_OF_DIMENSIONS   , "3");
    hdr.WriteTag(HDR_MATRIX_SIZE_1          , num_projs(LR_type));
    hdr.WriteTag(HDR_MATRIX_SIZE_2          , num_views(LR_type));
    hdr.WriteTag(HDR_MATRIX_SIZE_3          , LR_type == LR_0 ? NSINOS[g_span] : LR_NSINOS[g_span]);
    hdr.WriteTag(HDR_DATA_FORMAT              , "sinogram");
    hdr.WriteTag(HDR_NUMBER_FORMAT            , "signed integer");
    hdr.WriteTag(HDR_NUMBER_OF_BYTES_PER_PIXEL, (int)elem_size);
    hdr.WriteTag("!lmhistogram version"     , sw_version);
    hdr.WriteTag("!lmhistogram build ID"    , g_sw_build_id);
    hdr.WriteTag("!histogrammer revision"   , "2.0");

    int span_bak = g_span, rd_bak = max_rd;
    // ahc: lut_file now passed to init_rebinner instead of finding it there.
    // init_rebinner(g_span, max_rd);
    init_rebinner(g_span, max_rd, g_lut_file);
    g_logger->info("using rebinner LUT {}", rebinner_lut_file);

    hdr.WriteTag("!LM rebinner method", (int)rebinner_method);
    hdr.WriteTag(HDR_AXIAL_COMPRESSION, g_span);
    hdr.WriteTag(HDR_MAXIMUM_RING_DIFFERENCE, max_rd);
    g_span = span_bak; max_rd = rd_bak;
    hdr.WriteTag(HDR_SCALING_FACTOR_1, 10.0f * m_binsize); // cm to mm
    hdr.WriteTag(HDR_SCALING_FACTOR_2, 1); // angle
    hdr.WriteTag(HDR_SCALING_FACTOR_3, 10.0f * m_plane_sep); // cm to mm

    g_logger->info("Span: {}", g_span);

    static const char *sino_mode_txt[3] = { "Net Trues", "Prompts & Randoms", "prompts only"};
    if (g_hist_mode == HM_TRA)
      g_logger->info("Mode: Transmission");
    else
      g_logger->info("Mode: {}", sino_mode_txt[g_hist_mode]);

    // remove the '.s' extension if it exists
    if (out_fname[strlen(out_fname) - 1] == 's')
      out_fname[strlen(out_fname) - 2] = '\0';

    // create the output filename with full path stripped.
    char out_fname_stripped[1024];
    if (cptr = strrchr(out_fname, '\\'))
      strcpy(out_fname_stripped, (cptr + 1));
    else
      strcpy(out_fname_stripped, out_fname);

    // create the .dyn dynamic description file from out_fname if this is a dynamic study
    std::ofstream outfile_dyn;
    if (g_frames_duration.size() > 1) {
      const std::string outfname_dyn = make_file_name(FT_DYN);
      outfile_dyn = open_ostream(outfname_dyn);
      fmt::print(outfile_dyn, "{:d}", g_frames_duration.size());
      outfile_dyn.close();
    }

    // Check start countrate and set first frame skip
    if (start_countrate > 0) {
      int skip_msec = find_start_countrate(g_fname_l64);
      if (skip_msec > 0) {
        skip[0] = skip_msec / 1000; //msec
        g_logger->info("{:d} sec will be skipped to start with {}e", g_skip[0], start_countrate);
      }
    }

    //start reader and rebinner threads
    start_reader_thread();
    start_rebinner_thread();        // start lm64_rebinner thread  and wait until ready

    // process each frame
    frame = 0;
    while (frame < g_frames_duration.size()) {
      // output histogram is filename_frameX.s
      // output histogram header is filename_frameX.s.hdr
      if (g_frames_duration.size() > 1) {
        g_out_fname_sino = fmt::format("{}_frame{:d}.s", out_fname, frame);

        fmt::print(outfile_dyn, "{}_frame{}.s", out_fname_stripped, frame);
        // FILE *fp = fopen(outfname_dyn, "a");
        //   fprintf(fp, "%s_frame%d.s\n", out_fname_stripped, frame);
      } else {
        g_out_fname_sino = fmt::format("{}.s", out_fname);
      }

      // Create output files; exit if fail
      create_files();

      if (elem_size == 2) {
        if (!init_sino<short>(ssino, delayed, g_span, g_prev_sino, prev_duration)) 
          exit(1);
        sinogram = (char *)ssino;
      } else {
        if (!init_sino<char>(bsino, delayed, g_span, g_prev_sino, prev_duration)) 
          exit(1);
        sinogram = bsino;
      }


      // Position input stream to time 0 or requested time
      g_logger->info("Histogramming Frame {:d} duration = {:d} seconds", frame, g_frames_duration[frame]);
      if g_skip[frame] > 0 {
        int time_pos = g_skip[frame];
        if (current_time > 0) 
          time_pos += current_time;
        if (!goto_event(g_skip[frame])) {
          g_logger->errof("Unable to locate requested time {:d}", time_pos);
          exit(1);
        }
      } else {
        // skipping events before coincindence processor time reset.
        if (current_time < 0)
          if (!goto_event(0)) {
            g_logger->errof("Unable to locate time reset");
            exit(1);
          }
      }

      // Histogram the frame
      if (elem_size == 2) {
        relative_start = histogram<short>(ssino, delayed, g_sinogram_size, g_frames_duration[frame], g_out_hc);
      } else {
        relative_start = histogram<char>(bsino, delayed, g_sinogram_size, g_frames_duration[frame], g_out_hc);
      }

      if (relative_start < 0) {
        // exit if error encountered, error message logged by histogram routine
        exit(1);
      }

      if (g_frames_duration[frame] <= 0) {
        // should not happen
        g_logger->error("Invalid frame duration {:d}", g_frames_duration[frame]);
        exit(1);
      }

      // update current time
      current_time = relative_start + g_frames_duration[frame];
      scan_duration += g_frames_duration[frame];

      // Write sinogram and header files
      write_sino(sinogram, g_sinogram_size, hdr, frame);
      close_files();
      if (g_hist_mode != 7)
        write_coin_map(g_out_fname_sino);
      if (g_frames_duration.size() > 1)
        g_logger->info("Frame time histogrammed = {:d} sec", g_frames_duration[frame]);
      else
        g_logger->info("Time histogrammed = {:d} sec", g_frames_duration[frame]);
      frame++;

    }
    if (g_frames_duration.size() > 1) {
      outfile_dyn.close();
      g_logger->info("Total time histogrammed = {:d} secs", current_time);
    }
    // complete the main data files
    free(sinogram);

}

int main(int argc, char *argv[]) {
  int frame{0};
  int count{0}, multi_spec[2];
  char *sinogram = nullptr, *bsino = nullptr, *delayed = nullptr;
  short *ssino = nullptr;
  int current_time{-1};
  char *cptr = nullptr;
  CHeader *hdr;

  char msg[FILENAME_MAX];

  g_sw_build_id = fmt::format("{:s} {:s}", __DATE__, __TIME__);

  parse_boost(argc, argv);
  init_logging();
  validate_arguments();
  if (check_input_files())
    exit(1);
  if (! hdr = open_l64_hdr_file())
    exit(1);
  set_tx_source_speed();

  // Frame duration not specified at command line; Use header
  // Decode frame duration even when creating l32 file to output CH files
  if (g_frames_duration.empty() {
    string framedef_str;
    hdr.Readchar(HDR_FRAME_DEFINITION, framedef_str);
    parse_duration_string(framedef_str);
  }

  if (do_scan) {
    do_lmscan();
  } else {
    do_histogramming(hdr);
  }
  if (L64EventPacket::frame_duration != nullptr) {
    free (L64EventPacket::frame_duration);
    free (L64EventPacket::frame_skip);
  }
  clean_segment_info();
  return 0;
}
