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
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <regex>

#include "CHeader.hpp"
#include "dtc.hpp"
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
#include <boost/regex.hpp>
#include <boost/xpressive/xpressive.hpp>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h" //support for stdout logging
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/rotating_file_sink.h" // support for rotating file logging

#include <fmt/format.h>
#include <fmt/ostream.h>

#define DEFAULT_LLD 400 // assume a value of 400 if not specified in listmode header
constexpr char RW_MODE[] = "wb+";
constexpr char pgm_id[] = "V2.1 ";  // Revision changed from 2.0 to 2.1 for LR and P39 support
constexpr int MAX_FRAMES  = 256;
constexpr int MAX_THREADS =  4;

namespace bt = boost::posix_time;
namespace bg = boost::gregorian;
namespace po = boost::program_options; 
namespace bf = boost::filesystem;
namespace bx = boost::xpressive;

constexpr std::array<int, 10> NSINOS {
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

constexpr std::array<int, 10> LR_NSINOS {
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

constexpr std::array<int, 10> P39_NSINOS {
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

enum ELEM_SIZE {
  ELEM_SIZE_BYTE = 1, 
  ELEM_SIZE_SHORT = 2
};

bf::path g_fname_l64;     // input listmode file name
bf::path g_fname_l64_hdr; // input listmode header file name
bf::path g_out_fname;    // output sinogram file basename
bf::path g_out_fname_hc; // output Head Curve file name
bf::path g_out_fname_sino; //current frame sinogram file name
bf::path g_out_fname_hdr;  //current frame sinogram header file name
bf::path g_out_fname_ra;   //current frame sinogram randoms file name
bf::path g_out_fname_pr;   //current frame sinogram prompts file name
bf::path g_out_fname_tr;   //current frame sinogram Trues file name

std::string g_logfile;

extern std::shared_ptr<spdlog::logger> g_logger;

// static int p39_nsegs = 25;
// static int MODEL_HRRT = 328;
// static const char *p39_seg_table = "{239,231,231,217,217,203,203,189,189,175,175,161,161,147,147,133,133,119,119,105,105,91,91,77,77}";

std::string g_outfname_mock; //Mock sinogram file name for transmission only
std::ofstream g_out_hc;             // Head Curve output File Pointer
std::ofstream g_out_true_prompt_sino;    // output trues or prompts sinogram File Pointer
std::ofstream g_out_ran_sino;   // output randoms sinogram File Pointer
std::ofstream g_out_true_sino;  // output Trues sinogram for separate prompts and randoms mode

std::vector<int> g_frames_duration;    // Frames duration
std::vector<int> g_skip;               // Frames optional skip

static int g_dose_delay = 0;          // elapsed time between dose assay time and scan start time
static int relative_start = 0;       // Current frame start time
static int g_prev_duration = 0;        // Previous sinogram duration (when -add option)
static long scan_duration = 0;          // P39 TX duration is total time scan duration

// Parameters which default may be overrided in lmhistogram.ini
static int g_span = 9;                // Default span=9 for l64 if not specified in header
static int g_elem_size = ELEM_SIZE_SHORT;        // sinogram elem size: 1 for byte, 2 for short (default)
static int g_num_sino, g_sinogram_size;  // number of sinogram and size
static int compression = 0;         // 0=no compression (default), 1=ER (Efficient Representation)
static int output_ra = 0;           // output randoms

static bool g_span_override = false;       // Flag set span when span specified at command line
bool do_scan = false;                // scan option , default=no
static int out_l32 = 0;             // Output 32-bit listmode (output file extension .l32)
static int out_l64 = 0;             // Output 64-bit listmode (output file extension .l64)

std::string g_prev_sino;        // 1: add to existing normalization scan
static float decay_rate = 0.0;      // Default no decay correction in seconds
static int g_lld = DEFAULT_LLD;

// ahc: hrrt_rebinner.lut is now a required cmd line arg
std::string g_lut_file;

//Crystals and Ring dimensions in cm
constexpr float PITCH = 0.24375f;
constexpr float RDIAM = 46.9f;
constexpr float LTHICK = 1.0f;
constexpr int BLK_SIZE = 8;      // Number of crystals per block

constexpr string sw_version("HRRT_U 1.1");
std::string g_sw_build_id;


/**
 * init_sino:
 * Allocates the sinogram in memory and initializes it with 0 or sinogram loaded from the g_prev_sino argument.
 * Sets the duration with g_prev_sino duration value from g_prev_sino header.
 * Returns 1 if success and 0 otherwise
 */
template <class T> int init_sino(T *&sino, int t_span) {
  int sinogram_subsize = 0;
  int r_size = sizeof(T);

  g_logger->info("Using HRRT model");
  if (g_hist_mode != HM_TRA) {
    // emission
    g_logger->debug("Using HRRT Emission: g_hist_mode = {}", g_hist_mode);
    g_num_sino = (LR_type == 0) ? NSINOS[t_span] : LR_NSINOS[t_span];
    g_sinogram_size = m_nprojs * m_nviews * g_num_sino;
    if (g_hist_mode == HM_PRO_RAN) {
      // prompts and delayed
      sinogram_subsize = g_sinogram_size;
    }
  } else {
    // transmission
    g_logger->debug("XX Using HRRT Transmission, g_hist_mode = {}", g_hist_mode);
    g_num_sino = (LR_type == 0) ? NSINOS[0] : LR_NSINOS[0];
    g_sinogram_size = m_nprojs * m_nviews * g_num_sino;
    sinogram_subsize = g_sinogram_size; // mock memory
  }

  if (g_num_sino == 0) {
    g_logger->error("Unsupported span : {}", t_span);
    return 0;
  }

  g_logger->info("number of elements: {}", m_nprojs);
  g_logger->info("number of angles: {}"    , m_nviews);
  g_logger->info("number of sinograms: {}" , g_num_sino);
  g_logger->info("bits per register: {}"   , r_size * 8);
  g_logger->info("sinogram size: {}"       , g_sinogram_size);
  g_logger->info("sinogram subsize: {}"    , sinogram_subsize);
  if (sino) {
    // memset(sino, 0, (g_sinogram_size + sinogram_subsize) * r_size);
    std::fill_n(sino, g_sinogram_size + sinogram_subsize, 0);
  } else {
    // sino = (T *)calloc(g_sinogram_size + sinogram_subsize, r_size);
    sino = new T[g_sinogram_size + sinogram_subsize]();
  }
  if (!sino) {
    g_logger->error("Unable to allocate sinogram memory: {}", g_sinogram_size + sinogram_subsize);
    return 0;
  }
  return 1;
}

template <class T> int fill_prev_sino(T &sino, const std::string &prev_sino) {
  if (g_hist_mode != HM_TRU) {
    g_logger->error("Adding to existing sinogram only supported Trues mode");
    return 0;
  }

  if (prev_sino.length() > 0) {
    std::ifstream prev_fp;
    if (open_istream(prev_fp, prev_sino, std::ios::in | std::ios::binary)) {
      g_logger->error("Could not open prev_sino: {}", prev_sino);
      return 0;
    } else {
      // prev_fp.read(sino, g_sinogram_size * r_size);
      prev_fp.read(sino, g_sinogram_size * sizeof(T));
      if (!prev_fp.good()) {
        g_logger->error("Error reading {}", prev_sino);
        prev_fp.close();
        return 0;
      }
      prev_fp.close();
    }
  }
}


void on_span(int span) {
    if (span != 3 && span != 9) {
      g_logger->error("Span must be 3 or 9");
      exit(1);
    }
    g_span_override = true;
    g_span = span;
}

// Cmd line param 'out': Create output file path

void on_out(const std::string &outstr) {
  g_out_fname = bf::path(outstr);
}

void on_infile(const std::string &instr) {
  g_fname_l64 = bf::path(instr);
}

void on_lrtype(LR_Type lr_type) {
  switch (lr_type) {
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
  g_max_rd = 38;
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
  bx::sregex re_skip = bx::sregex::compile("(?P<skip>[0-9]+)\\,(<?P<dura>[0-9]+)");
  bx::sregex re_mult = bx::sregex::compile("(?P<dura>[0-9]+)\\*(<?P<mult>[0-9]+)");

    bx::smatch match;
    if (bx::regex_match(durat_str, match, re_skip)) {
      g_skip.push_back(boost::lexical_cast<int>(match["skip"]));
      g_frames_duration.push_back(boost::lexical_cast<int>(match["dura"]));
    } else if (bx::regex_match(durat_str, match, re_mult)) {
      std::vector<int> durats(boost::lexical_cast<int>(match["mult"]), boost::lexical_cast<int>(match["dura"]));
      g_frames_duration.insert(std::end(g_frames_duration), std::begin(durats), std::end(durats));
    }
}

// Parse duration strings 

void on_duration(std::vector<std::string> vs) {
  for (auto& it : vs) {
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
    g_logger->error("Bad log level: {}", verbosity);
  }
}

void validate_arguments(void) {

  if (!g_prev_sino.empty()) {
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
    po::options_description desc("Options");
    desc.add_options()
    ("help,h", "Show options")
    ("out,o"        , po::value<std::string>()->notifier(&on_out)                    , "output sinogram, 32 or 64-bit listmode file from file extension")
    ("infile,i"     , po::value<std::string>()->notifier(&on_infile)                 , "Input file")
    ("lr_type,L"    , po::value<LR_Type>()->notifier(on_lrtype)                      , "low resolution (mode 1/2: binsize 2/2.4375mm, nbins 160/128, span 7, maxrd 38)")
    ("span"         , po::value<int>()->notifier(on_span)                            , "Span size - valid values: 0(TX), 1,3,5,7,9")
    ("PR"           , po::bool_switch()->notifier(&on_pr)                            , "Separate prompts and randoms")
    ("ra"           , po::bool_switch()->notifier(&on_ra)                            , "Output randoms sinogram file (emission only)")
    ("prompts,P"    , po::bool_switch()->notifier(&on_p)                             , "Prompts only")
    ("notimetag"    , po::bool_switch()->notifier(&on_notimetag)                     , "No timetag processing, use timetag count as time")
    ("nodoi"        , po::bool_switch()->notifier(&on_nodoi)                         , "No DoI processing, back layer events processed as front layer")
    ("scan"         , po::bool_switch(&do_scan)->default_value(false)                , "Scan file for prompts and randoms, print headcurve to g_logger")
    ("model"        , po::value<int>(&model_number)->default_value(MODEL_HRRT)       , "Model: 328, 2393, 2395")
    ("verbosity,V"  , po::value<int>()->notifier(&on_verbosity)                      , "Logging verbosity: 0 off, 1 err, 2 info, 3 debug")
    ("add"          , po::value<std::string>(&g_prev_sino)                           , "Add to existing sinogram file")
    ("log,l"        , po::value<std::string>(&g_logfile)                             , "Log file")
    ("mock"         , po::value<std::string>(&g_outfname_mock)                      , "Output shifted mock sinogram file (transmission only)")
    ("EB"           , po::bool_switch()->notifier(&on_eb)                            , "Exclude border crystals")
    ("count"        , po::value<int>(&stop_count_)->notifier(&assert_positive)        , "Stop after N events")
    ("start"        , po::value<int>(&start_countrate_)->notifier(&assert_positive)  , "Start histogramming when trues/sec is higher than N")
    ("lut_file,r"   , po::value<std::string>(&g_lut_file)->required()                , "Full path to rebinner.lut file" )
    ("duration,d"   , po::value<std::vector<std::string>>()->multitoken()
     ->composing()
     ->notifier(&on_duration) , "Frame [duration] or [duration,skip] or [duration*repeat]")
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

// Create buffers, Open file listmode file
void start_reader_thread() {
  // Create container synchronization event, start reader thread, wait for container ready (half full)
  // L64EventPacket::frame_duration = (int *)calloc(g_frames_duration.size(), sizeof(int));
  // memcpy(L64EventPacket::frame_duration, g_frames_duration, g_frames_duration.size() * sizeof(int));
  // L64EventPacket::frame_skip = (int *)calloc(g_frames_duration.size(), sizeof(int));
  // mcpy(L64EventPacket::frame_skip, g_frames_duration.size() * sizeof(int));
  L64EventPacket::frame_duration = g_frames_duration;
  L64EventPacket::frame_skip = g_skip;

  lm64_reader(g_fname_l64);
}

// Start thread and wait until it is ready

void start_rebinner_thread() {
    // int ID1 = 0, ID2 = 1, ID3 = 2, ID4 = 3;
    // int max_try = 30;
    // int head_nblks = (NXCRYS / 8) * (NYCRYS / 8); // 9*13=117
    int ncrystals = NDOIS * NXCRYS * NYCRYS * NHEADS;
    p_coinc_map = (unsigned *)calloc(ncrystals, sizeof(unsigned));
    d_coinc_map = (unsigned *)calloc(ncrystals, sizeof(unsigned));
}

int get_num_cpus() {
    int num_cpus = 1;
    const char *s = getenv("NUMBER_OF_PROCESSORS");
    if (s != nullptr) 
      sscanf(s, "%d", &num_cpus);
  if (num_cpus > MAX_THREADS)
    num_cpus = MAX_THREADS; // workaround for dual quadcore bug

    return num_cpus;
}

/**
 * create_histogram_files:
 * Create/Open output files before starting histogramming.
 * Log error and exit if file creation fails
 */
static void create_histogram_files() {
  g_out_fname_hdr = make_file_name(FT_SINO_HDR);
  g_out_fname_hc  = make_file_name(FT_LM_HC);

  open_ostream(g_out_hc, g_out_fname_hc);

  if (g_hist_mode == HM_TRA) {
    // transmission
    if (g_elem_size != ELEM_SIZE_SHORT) {
      g_logger->error("Byte format not supported in transmission mode");
      exit(1);
    }
    open_ostream(g_out_true_prompt_sino, g_out_fname_sino, std::ios::out | std::ios::app | std::ios::binary);
    if (g_outfname_mock.length() > 0) {
      open_ostream(g_out_ran_sino, g_outfname_mock, std::ios::out | std::ios::app | std::ios::binary);
    }
  } else if (g_hist_mode == HM_PRO_RAN) {
    // Keep original filename for prompts in prompts or prompts+delayed mode
    g_out_fname_pr = g_out_fname_sino;
    open_ostream(g_out_true_prompt_sino, g_out_fname_pr, std::ios::out | std::ios::app | std::ios::binary);
    g_out_fname_ra = make_file_name(FT_RA_S);
    open_ostream(g_out_ran_sino, g_out_fname_ra, std::ios::out | std::ios::app | std::ios::binary);
    if (g_elem_size == ELEM_SIZE_SHORT) {
      g_out_fname_tr = make_file_name(FT_TR_S);
      open_ostream(g_out_true_sino, g_out_fname_tr, std::ios::out | std::ios::app | std::ios::binary);
    }
  } else {
    open_ostream(g_out_true_prompt_sino, g_out_fname_sino, std::ios::out | std::ios::app | std::ios::binary);

  }
  g_logger->info("Output File: {}", g_out_fname);
  g_logger->info("Output File Header: {}", g_out_fname_hdr);
}

/**
 * close_files:
 * Close output files created by create_histogram_files
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
static void write_sino(char *t_sino, int t_sino_size, CHeader &t_hdr, int t_frame) {
  // create the output header file
  char *p = nullptr;
  short *ssino = (short *)t_sino;
  short *delayed = (short *)(t_sino + t_sino_size * g_elem_size);
  short *mock = (short *)(t_sino + t_sino_size * g_elem_size);
  int span9_sino_size = m_nprojs * m_nviews * NSINOS[9];

  t_hdr.WriteInt(HDR_IMAGE_DURATION, g_prev_duration + g_frames_duration[t_frame]);
  t_hdr.WriteInt(HDR_IMAGE_RELATIVE_START_TIME, relative_start);
  int decay_time = relative_start + g_dose_delay;
  float dcf1 = 1.0f;
  float dcf2 = 1.0f;
  if (decay_rate > 0.0) {
    dcf1 = (float)pow(2.0f, decay_time / decay_rate);
    //      dcf2 = ((float)log(2.0) * duration[t_frame]) / (decay_rate * (1.0 - (float)pow(2,-1.0*duration[t_frame]/decay_rate)));
    dcf2 = (float)((log(2.0) * g_frames_duration[t_frame]) / (decay_rate * (1.0 - pow(2.0f, -1.0f * g_frames_duration[t_frame] / decay_rate))));
  }
  t_hdr.WriteFloat(HDR_DECAY_CORRECTION_FACTOR, dcf1);
  t_hdr.WriteFloat(HDR_DECAY_CORRECTION_FACTOR_2, dcf2);
  t_hdr.WriteInt(HDR_FRAME, t_frame);
  t_hdr.WriteLong(HDR_TOTAL_PROMPTS, total_prompts());
  t_hdr.WriteLong(HDR_TOTAL_RANDOMS, total_randoms());
  t_hdr.WriteLong(HDR_TOTAL_NET_TRUES, total_prompts() - total_randoms());
  int av_singles = 0;
  for (int block = 0; block < NBLOCKS; block++) {
    std::string tmp1 = fmt::format("block singles {:d}", block);
    t_hdr.WriteInt(tmp1, singles_rate(block));
    av_singles += singles_rate(block);
  }
  av_singles /= NBLOCKS;
  t_hdr.WriteInt(HDR_AVERAGE_SINGLES_PER_BLOCK, av_singles);
  g_logger->info("g_lld = {}, Average Singles Per Block = ", g_lld, av_singles);
  t_hdr.WriteFloat(HDR_DEAD_TIME_CORRECTION_FACTOR, GetDTC(g_lld, av_singles));

  // Write Trues or Prompts sinogram
  int count = g_elem_size, write_error = 0;
  g_logger->info("sinogram size: {:d}", t_sino_size);
  g_logger->info("element size: {:d}", g_elem_size);

  // Log message and set first data filename
  switch (g_hist_mode) {
  case HM_TRU:
    // Randoms substractions
    g_logger->info("Writing Trues Sinogram: {}", g_out_fname_sino);
    t_hdr.WritePath(HDR_NAME_OF_DATA_FILE, g_out_fname_sino);
    t_hdr.WriteChar(HDR_SINOGRAM_DATA_TYPE, "true");
    g_out_true_prompt_sino.write(t_sino, t_sino_size * g_elem_size);
    break;
  case HM_PRO_RAN:
    // Prompts and delayed: prompts is first
    g_logger->info("Writing Prompts Sinogram: {}", g_out_fname_pr);
    t_hdr.WritePath(HDR_NAME_OF_DATA_FILE, g_out_fname_pr);
    t_hdr.WriteChar(HDR_SINOGRAM_DATA_TYPE, "prompt");
    t_hdr.WritePath(HDR_NAME_OF_TRUE_DATA_FILE, g_out_fname_tr);
    g_out_fname_hdr = fmt::format("{}.hdr", g_out_fname_pr);
    g_out_true_prompt_sino.write(t_sino, t_sino_size * g_elem_size);
    break;
  case HM_PRO:
    // Prompts only
    g_logger->info("Writing Prompts Sinogram: {}", g_out_fname_sino);
    t_hdr.WritePath(HDR_NAME_OF_DATA_FILE, g_out_fname_sino);
    t_hdr.WriteChar(HDR_SINOGRAM_DATA_TYPE, "prompt");
    break;
  case HM_TRA:
    // Transmission
    g_logger->info("Writing EC corrected Sinogram: {}", g_out_fname_sino);
    g_out_fname_hdr = fmt::format("{}.hdr", g_out_fname_sino);
    t_hdr.WritePath(HDR_NAME_OF_DATA_FILE, g_out_fname_sino);
    for (int i = 0; i < t_sino_size; i++) {
      ssino[i] = ssino[i] - mock[i];
      if (ssino[i] < 0)
        ssino[i] = 0;
    }
    g_out_true_prompt_sino.write(t_sino, t_sino_size * g_elem_size);
    if (g_out_true_prompt_sino.good()) {
      t_hdr.WriteFile(g_out_fname_hdr);
    } else {
      g_logger->error("Error writing %s\n", g_out_fname_sino);
      write_error++;
    }
    break;
  }

  // Write data
  if (!write_error)
    t_hdr.WriteFile(g_out_fname_hdr);

  // Log message and write second data filename
  if (g_elem_size == ELEM_SIZE_SHORT && write_error == 0)
    switch (g_hist_mode) {
    case 1: // Prompts and delayed: trues is second if short type
      g_out_fname_hdr = fmt::format("{}.hdr", g_out_fname_tr);
      t_hdr.WritePath(HDR_NAME_OF_DATA_FILE, g_out_fname_tr);
      t_hdr.WriteChar(HDR_SINOGRAM_DATA_TYPE, "true");
      // convert span3 prompt sinogram to span9 and substract delayed

      for (int i = 0; i < t_sino_size; i++)
        ssino[i] =  ssino[i] - delayed[i];
      if (g_span == 3) {
        g_logger->info("Converting true t_sino to span9");
        convert_span9(ssino, g_max_rd, 104);
        t_hdr.WriteInt(HDR_AXIAL_COMPRESSION, 9);
        t_hdr.WriteInt(HDR_MATRIX_SIZE_3, NSINOS[9]);
      }
      g_logger->info("Writing Trues Sinogram: {}", g_out_fname_tr);
      // if (fwrite(ssino, g_span == 3 ? span9_sino_size : t_sino_size, g_elem_size, g_out_true_sino) == g_elem_size) {
      g_out_true_sino.write(reinterpret_cast<char *>(ssino), g_span == 3 ? span9_sino_size * g_elem_size: t_sino_size * g_elem_size);
      if (g_out_true_sino.good()) {
        t_hdr.WriteFile(g_out_fname_hdr);
      } else {
        g_logger->error("Error writing {}", g_out_fname_tr);
        write_error++;
      }
      // Restore span3 header
      if (g_span == 3) {
        t_hdr.WriteInt(HDR_AXIAL_COMPRESSION, 3);
        t_hdr.WriteInt(HDR_MATRIX_SIZE_3, NSINOS[3]);
      }
      break;
    case 7: // Transmission: EC corrected is second if short type
      if (g_out_ran_sino.is_open()) {
        g_logger->info("Writing Mock Sinogram: {}", g_outfname_mock);
        g_out_fname_hdr = fmt::format("{}.hdr", g_outfname_mock);
        t_hdr.WritePath(HDR_NAME_OF_DATA_FILE, g_outfname_mock);
        // if (fwrite(mock, t_sino_size, g_elem_size, g_out_ran_sino) == g_elem_size) {
        g_out_ran_sino.write(reinterpret_cast<char *>(mock), t_sino_size * g_elem_size);
        if (g_out_ran_sino.good()) {
          t_hdr.WriteFile(g_out_fname_hdr);
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
    t_hdr.WritePath(HDR_NAME_OF_DATA_FILE, g_out_fname_ra);
    t_hdr.WriteChar(HDR_SINOGRAM_DATA_TYPE, "delayed");
    // count = fwrite(delayed, span9_sino_size, g_elem_size, g_out_ran_sino);
    // if (count == (int)g_elem_size) {
    g_out_ran_sino.write(reinterpret_cast<char *>(delayed), span9_sino_size * g_elem_size);
    if (g_out_ran_sino.good()) {
      t_hdr.WriteFile(g_out_fname_hdr);
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

  g_out_fname_hc = make_file_name(FT_LM_HC);
  std::ofstream outfile_hc;
  open_ostream(outfile_hc, g_out_fname_hc);
  g_frames_duration.clear();

  start_reader_thread();
  lmscan(outfile_hc, &scan_duration);
  outfile_hc.close();
        //      for (int index = 0; index < duration; index++)
        //          cout << hc[index].time << " " << hc[index].randoms_rate << " " << hc[index].trues_rate << " " << hc[index].singles << endl;
  g_logger->info("Scan Duration = {:d}", scan_duration);

}

bf::path make_file_name(FILE_TYPE file_type) {
  make_file_name(file_type, g_fname_l64, -1);
}

/**
 * @brief      Makes a file name from the input file g_fname_l64
 * @param[in]  file_type  The file type
 * @param[in]  frame_no   Optional frame number, when required
 * @return     String 
 */
bf::path make_file_name(FILE_TYPE file_type, bf::path stem, int frame_no) {
  bf::path file_path;

  switch (file_type) {
  // File names with frame number
  case FT_FR_S:
  {
    std::string frame_part = fmt::format(FILE_EXTENSIONS[file_type], frame_no);
    file_path = stem.replace_extension(frame_part);
  }
  break;
  default:
    // File names without frame number
    file_path = stem.replace_extension(FILE_EXTENSIONS[file_type]);
  }

  g_logger->info("make_file_name({}): {}", file_type, file_path);
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

CHeader *open_l64_hdr_file(void) {
  CHeader *hdr = new CHeader;
  g_fname_l64_hdr = make_file_name(FT_L64_HDR);
  if (hdr->OpenFile(g_fname_l64_hdr) != E_OK) {
    g_logger->error("header {} not found", g_fname_l64_hdr.string());
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

int set_tx_source_speed(const CHeader *hdr) {
  string datatype;
  // for transmission scans, always override span to 0 (segment 0 only)
  if (hdr->Readchar(HDR_PET_DATA_TYPE, datatype) == E_OK) {
    g_logger->info("Data Type: {}", datatype);
    // ahc this was failing because of trailing CR on DOS format hdr file.
    if (datatype == "transmission") {
      float axial_velocity = 0.0;
      g_span = 0;
      g_hist_mode = HM_TRA;
      g_logger->info("Data Type: {}, g_hist_mode: {}", datatype, g_hist_mode);
      if (hdr->Readfloat("axial velocity", axial_velocity) == 0) {
        // tx speed=6msec/crystal for axial velocity=100%
        g_tx_source_speed = 6.25f / axial_velocity / 100.0f;
        g_logger->info("Axial velocity = {}, TX source speed = {}", axial_velocity, g_tx_source_speed);
      }
    }
  }
  return 0;
}

int set_lld(const CHeader &hdr) {
  if (hdr.Readint("lower level", g_lld) != E_OK)
    g_lld = DEFAULT_LLD;
  g_logger->info("g_lld: {}", g_lld);
  return 0;
}

int set_histogramming_mode(const CHeader &hdr) {
  int sino_mode = HM_TRU;  // by default use net trues
  if (hdr.Readint("Sinogram Mode", sino_mode) == E_OK) {
    g_logger->info("Using Sinogram Mode from header: {}", sino_mode);
    if ((sino_mode != HM_TRU) && g_prev_sino) {
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
      hdr.WriteInt(HDR_AXIAL_COMPRESSION, g_span);
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
    hdr.WriteChar(HDR_ORIGINATING_SYSTEM      , "HRRT");
    hdr.WritePath(HDR_NAME_OF_DATA_FILE       , g_out_fname_sino);
    hdr.WriteInt(HDR_NUMBER_OF_DIMENSIONS     , "3");
    hdr.WriteInt(HDR_MATRIX_SIZE_1            , num_projs(LR_type));
    hdr.WriteInt(HDR_MATRIX_SIZE_2            , num_views(LR_type));
    hdr.WriteInt(HDR_MATRIX_SIZE_3            , LR_type == LR_0 ? NSINOS[g_span] : LR_NSINOS[g_span]);
    hdr.WriteChar(HDR_DATA_FORMAT             , "sinogram");
    hdr.WriteChar(HDR_NUMBER_FORMAT           , "signed integer");
    hdr.WriteInt(HDR_NUMBER_OF_BYTES_PER_PIXEL, (int)g_elem_size);
    hdr.WriteString(HDR_LMHISTOGRAM_VERSION   , sw_version);
    hdr.WriteString(LMHISTOGRAM_BUILD_ID         , g_sw_build_id);
    hdr.WriteChar(HDR_HISTOGRAMMER_REVISION    , "2.0");

    int span_bak = g_span;
    int rd_bak = g_max_rd;
    init_rebinner(g_span, g_max_rd, g_lut_file);
    hdr.WriteInt(HDR_LM_REBINNER_METHOD, (int)rebinner_method);
    hdr.WriteInt(HDR_AXIAL_COMPRESSION, g_span);
    hdr.WriteInt(HDR_MAXIMUM_RING_DIFFERENCE, g_max_rd);
    g_span = span_bak; 
    g_max_rd = rd_bak;
    hdr.WriteFloat(HDR_SCALING_FACTOR_1, 10.0f * m_binsize); // cm to mm
    hdr.WriteInt(HDR_SCALING_FACTOR_2, 1); // angle
    hdr.WriteFloat(HDR_SCALING_FACTOR_3, 10.0f * m_plane_sep); // cm to mm

    g_logger->info("using rebinner LUT {}", rebinner_lut_file);
    g_logger->info("Span: {}", g_span);
    g_logger->info("Mode: {}", HISTOGRAM_MODES[g_hist_mode]);

    // create the .dyn dynamic description file from out_fname if this is a dynamic study
    std::ofstream outfile_dyn;
    if (g_frames_duration.size() > 1) {
      const std::string outfname_dyn = make_file_name(FT_DYN);
      open_ostream(outfile_dyn, outfname_dyn);
      fmt::print(outfile_dyn, "{:d}", g_frames_duration.size());
    }

    // Check start countrate and set first frame skip
    if (start_countrate_ > 0) {
      int skip_msec = find_start_countrate(g_fname_l64);
      if (skip_msec > 0) {
        skip[0] = skip_msec / 1000; //msec
        g_logger->info("{:d} sec will be skipped to start with start_countrate {}", g_skip[0], start_countrate_);
      }
    }

    //start reader and rebinner threads
    start_reader_thread();
    start_rebinner_thread();        // start lm64_rebinner thread  and wait until ready

    // process each frame

    for (int frame_no = 0; frame_no < g_frames_duration.size(); frame_no++) {
      do_histogram_frame(frame_no, outfile_dyn);
    }
    if (g_frames_duration.size() > 1) {
      outfile_dyn.close();
      g_logger->info("Total time histogrammed = {:d} secs", current_time);
    }
    // complete the main data files
    free(sinogram);
}

bool is_short(void) {
  return (g_elem_size == ELEM_SIZE_SHORT);
}

void do_histogram_frame(const inst &t_frame_no, const std::ofstream &t_outfile_dyn) {
  int current_time{-1};
  char *sinogram = nullptr, *bsino = nullptr, *delayed = nullptr;
  short *ssino = nullptr;

  // output histogram is filename_frameX.s, histogram header is filename_frameX.s.hdr
  if (g_frames_duration.size() > 1) {
    g_out_fname_sino = make_file_name(FT_FR_S, nullptr, t_frame_no);
    fmt::print(t_outfile_dyn, "{}", g_out_fname_sino);
  } else {
    g_out_fname_sino = fmt::format("{}.s", out_fname);
  }

  // Create output files; exit if fail
  create_histogram_files();

  int init_sino_ok;
  if is_short() {
    init_sino_ok = init_sino<short>(ssino, g_span);
    sinogram = (char *)ssino;
  } else {
    init_sino_ok = init_sino<char>(bsino, g_span);
    sinogram = bsino;
  }
  if (!init_sino_ok) {
    delete[](sinogram);
    exit(1);
  }

  if (g_prev_sino.length() > 0) {
    // Program parameter 'add'
    int fill_ok = fill_prev_sino(is_short() ? ssino : bsino, g_prev_sino);
    CHeader hdr;
    hdr.ReadHeaderNum<int>(make_file_name(FT_SINO, prev_sino), "image duration", g_prev_duration);
  }

  // Position input stream to time 0 or requested time
  g_logger->info("Histogramming Frame {:d} duration = {:d} seconds", t_frame_no, g_frames_duration[t_frame_no]);
  if g_skip[t_frame_no] > 0 {
    int time_pos = g_skip[t_frame_no];
    if (current_time > 0)
      time_pos += current_time;
    if (!goto_event(g_skip[t_frame_no])) {
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
  if (g_elem_size == ELEM_SIZE_SHORT) {
    relative_start = histogram<short>(ssino, delayed, g_sinogram_size, g_frames_duration[t_frame_no], g_out_hc);
  } else {
    relative_start = histogram<char>(bsino, delayed, g_sinogram_size, g_frames_duration[t_frame_no], g_out_hc);
  }

  if (relative_start < 0) {
    // exit if error encountered, error message logged by histogram routine
    exit(1);
  }

  if (g_frames_duration[t_frame_no] <= 0) {
    // should not happen
    g_logger->error("Invalid t_frame_no duration {:d}", g_frames_duration[t_frame_no]);
    exit(1);
  }

  // update current time
  current_time = relative_start + g_frames_duration[t_frame_no];
  scan_duration += g_frames_duration[t_frame_no];

  // Write sinogram and header files
  write_sino(sinogram, g_sinogram_size, hdr, t_frame_no);
  close_files();
  if (g_hist_mode != 7)
    write_coin_map(g_out_fname_sino);
  g_logger->info("Frame time histogrammed = {:d} sec", g_frames_duration[t_frame_no]);

}

int main(int argc, char *argv[]) {
  int frame{0};
  int count{0}, multi_spec[2];
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
  set_tx_source_speed(hdr);

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
  // if (L64EventPacket::frame_duration != nullptr) {
  //   free (L64EventPacket::frame_duration);
  //   free (L64EventPacket::frame_skip);
  // }
  clean_segment_info();
  delete hdr;
  return 0;
}
