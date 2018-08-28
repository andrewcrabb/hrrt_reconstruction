// lmhistogram.cpp : Defines the entry point for the console application.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "Header.hpp"
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
#include <unistd.h>
#include <iostream>
#include <fmt/format.h>

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

int nsinos[] = {
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

int LR_nsinos[] = {
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

int P39_nsinos[] = {
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

fs::path g_in_fname;     // input listmode file name
fs::path g_in_fname_hdr; // input listmode header file name
fs::path g_out_fname;    // output sinogram file basename
fs::path g_out_fname_hc; // output Head Curve file name
fs::path g_out_fname_sino; //current frame sinogram file name
fs::path g_out_fname_hdr;  //current frame sinogram header file name
fs::path g_out_fname_ra;   //current frame sinogram randoms file name
fs::path g_out_fname_pr;   //current frame sinogram prompts file name
fs::path g_out_fname_tr;   //current frame sinogram Trues file name

// fs::path outfname_p39_em[FILENAME_MAX]; //current frame sinogram emission file name
// fs::path outfname_p39_tx[FILENAME_MAX]; //current frame sinogram transmission file name

// auto console = spdlog::stdout_color_mt("console");
std::string g_logfile;
auto console;

static int p39_nsegs = 25;
static int MODEL_HRRT = 328;
static const char *p39_seg_table = "{239,231,231,217,217,203,203,189,189,175,175,161,161,147,147,133,133,119,119,105,105,91,91,77,77}";

// static char g_in_fname[FILENAME_MAX];  // input listmode file name
// static char g_in_fname_hdr[FILENAME_MAX]; // input listmode header file name
// static char out_fname[FILENAME_MAX];  // output sinogram file basename
static char fname_p39_mhdr[FILENAME_MAX]; // input listmode main header file name
static char *outfname_mock = NULL; //Mock sinogram file name for transmission only

static FILE *g_out_hc = NULL;             // Head Curve output File Pointer
static FILE *out = NULL;    // output trues or prompts sinogram File Pointer
static FILE  *out2 = NULL;              // output randoms sinogram File Pointer
static FILE  *out3 = NULL;              // output Trues sinogram File Pointer for separate prompts and randoms mode

static int g_frames_duration[MAX_FRAMES];    // Frames duration
static int skip[MAX_FRAMES];        // Frames optional skip
static int dose_delay = 0;          // elapsed time between dose assay time and scan start time
static int relative_start = 0;       // Current frame start time
static int prev_duration = 0;        // Previous sinogram duration (when -add option)
static long scan_duration = 0;          // P39 TX duration is total time scan duration

// Parameters which default may be overrided in lmhistogram.ini
static int g_span = 9;                // Default span=9 for l64 if not specified in header
static size_t elem_size = 2;        // sinogram elem size: 1 for byte, 2 for short (default)
static int g_num_sino, g_sinogram_size;  // number of sinogram and size
static int compression = 0;         // 0=no compression (default), 1=ER (Efficient Representation)
static int output_ra = 0;           // output randoms

static int span_override = 0;       // Flag set span when span specified at command line
static int nframes = 0;             // Number of frames
bool do_scan = false;                // scan option , default=no
// static int split = 0;               // split option, default=no
static int out_l32 = 0;             // Output 32-bit listmode (output file extension .l32)
static int out_l64 = 0;             // Output 64-bit listmode (output file extension .l64)

std::string g_prev_sino;        // 1: add to existing normalization scan
// static char *log_fname = NULL;          // log file
static FILE *log_fp = NULL;         // log file ptr
static float decay_rate = 0.0;      // Default no decay correction in seconds
static int LLD = DEFAULT_LLD;

// ahc: hrrt_rebinner.lut is now a required cmd line arg
static char *lut_file = NULL;

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
    console->info("Using HRRT model");
    if (hist_mode != 7) {
      // emission
      console->debug("Using HRRT Emission: hist_mode = {}", hist_mode);
      g_num_sino = (LR_type == 0) ? nsinos[span] : LR_nsinos[span];
      if (hist_mode == 1) {
        // prompts and delayed
        sinogram_subsize = g_sinogram_size;
      }
    } else {
      // transmission
      console->debug("XX Using HRRT Transmission, hist_mode = {}", hist_mode);
      g_num_sino = (LR_type == 0) ? nsinos[0] : LR_nsinos[0];
      sinogram_subsize = g_sinogram_size; // mock memory
    }
    g_sinogram_size = m_nprojs * m_nviews * g_num_sino;
  }

  if (g_num_sino == 0) {
    console->error("Unsupported span : {}", init_span);
    return 0;
  }

  console->info("number of elements: {}"  << m_nprojs);
  console->info("number of angles: {}"    << m_nviews);
  console->info("number of sinograms: {}" << g_num_sino);
  console->info("bits per register: {}"   << r_size * 8);
  console->info("sinogram size: {}"       << g_sinogram_size);
  console->info("sinogram subsize: {}"    << sinogram_subsize);
  if (sino == NULL) {
    sino = (T *)calloc(g_sinogram_size + sinogram_subsize, r_size);
  } else {
    memset(sino, 0, (g_sinogram_size + sinogram_subsize)*r_size);
  }
  if (!sino) {
    console->error("Unable to allocate sinogram memory: {}", (g_sinogram_size + sinogram_subsize) * r_size);
    return 0;
  }

  // load previous scan if any
  if (prev_sino.length() > 0) {
    CHeader hdr;
    std::string hdr_fname;
    if (hist_mode != 0) {
      console->error("Adding to existing sinogram only supported Trues mode");
      free(sino);
      return 0;
    }
    hdr_fname = fmt::format("{}.hdr", prev_sino);
    if (hdr.OpenFile(hdr_fname) != OK) {
      console->error("Error opening {}", hdr_fname);
      free(sino);
      return 0;
    }
    int ok = hdr.Readint("image duration", duration);
    hdr.CloseFile();
    if (ok != 0) {
      console->error("{}: image duration not found", hdr_fname);
      free(sino);
      return 0;
    }
    FILE *prev_fp = fopen(prev_sino.c_str(), "rb");
    if (prev_fp != NULL) {
      int count = fread(sino, g_sinogram_size, r_size, prev_fp);
      if (count != r_size) {
        console->error("Error reading {}", prev_sino);
        fclose(prev_fp);
        free(sino);
        return 0;
      }
      fclose(prev_fp);
    } else {
      console->error("Could not open prev_sino: {}", prev_sino);
      free(sino);
      return 0;
    }
  }
  return 1;
}

// Set out_l32 or out_l64 depending on output file name

void on_arg_out(std::string out_fname) {
  std::cout << "on_arg_out: " << out_fname << std::endl;
  g_out_fname = out_fname;
}

void on_infile(std::string infile) {
    std::cout << "on_infile: " << infile << std::endl;
    g_in_fname = infile;
}

void on_span(int span) {
    std::cout << "on_span: " << span << std::endl;
    if (span != 3 && span != 9) {
      std::cerror << "Span must be 3 or 9" << endl;
      exit(1);
    }
    g_span = span;
}

void on_lrtype(LR_Type lr_type) {
  std::cout << "on_lrtype: " << lr_type << std::endl;
  switch (type) {
    case LR_20:
    LR_type = LR_20;
    break;
    case LR_24:
    LR_type = LR_24;
    break;
    default:
    cout << "*** invalid LR mode ***" << endl << endl;
    lmhistogram_usage(argv[0]);
    exit(0);
  }
  g_span = 7;
  max_rd = 38;
}

void on_pr(bool pr) {
  hist_mode = 1;
}

void on_ra(bool ra) {
  output_ra = 1;
}

void on_p(bool p) {
  hist_mode = 2;
}

void on_verbosity(int verbosity) {
  switch (verbosity) {
    case 0: console->set_level(spdlog::level::off  ); break;
    case 1: console->set_level(spdlog::level::err  ); break;
    case 2: console->set_level(spdlog::level::info ); break;
    case 3: console->set_level(spdlog::level::debug); break;
    default: 
    console->error("Bad log level: {}", verbosity)
  }
}

void parse_boost(int argc, char **argv) {
  std::string out_fname, infile;
  int span, verbosity;
  LR_Type lr_type;
  bool pr, ra;

  try {
    options_description desc{"Options"};
    desc.add_options()
    ("help,h", "Show options")
    ("out,o"        , po::value<string>(&out_fname)->notifier(on_arg_out) , "output sinogram, 32 or 64-bit listmode file from file extension")
    ("infile,i"     , po::value<std::string>(&infile)->notifier(on_infile), "Input file")
    ("lr_type,L"    , po::value<LR_Type>(&lr_type)->notifier(on_lrtype)   , "low resolution (mode 1/2: binsize 2/2.4375mm, nbins 160/128, span 7, maxrd 38)")
    ("span"         , po::value<int>(&span)->notifier(on_span)            , "Span size - valid values: 0(TX), 1,3,5,7,9")
    ("PR"           , po::bool_switch(&pr)->notifier(&on_pr)              , "Separate prompts and randoms")
    ("ra"           , po::bool_switch(&ra)->notifier(&on_ra)              , "Output randoms sinogram file (emission only)")
    ("prompts,P"    , po::bool_switch(&p)->notifier(&on_p)                , "Prompts only")
    ("model"        , po::value<int>(&model_number)->default_value(MODEL_HRRT), "Model: 328, 2393, 2395")
    ("scan"         , po::bool_switch(&do_scan)->default_value(false)     , "Scan file for prompts and randoms, print headcurve to console")
    ("verbosity,V"  , po::value<int>(&verbosity)->notifier(&on_verbosity) , "Logging verbosity: 0 off, 1 err, 2 info, 3 debug")
    ("add"          , po::value<std::string>(&g_prev_sino)                , "Add to existing sinogram file")
    ("log,l"        , po::value<std::string>(&g_logfile)                  , "Log file")

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



void lmhistogram_usage(char *pgm_name)
{
    cout << pgm_name << " " << pgm_id << " Build: " << g_sw_build_id << endl << endl;
    cout << "Usage : " << pgm_name << " fname.l32|fname.l64 " << endl;
    cout << "    -o file              - output sinogram or 32-bit listmode file depending on file extension" << endl;
    cout << "    -span X              - span size - valid values: 0(TX), 1,3,5,7,9" << endl;
    cout << "                         - emission default span is 9" << endl;
    //  cout << "    -T       - Transmission mode" << endl;
    cout << "    -P                   - prompts only" << endl;
    cout << "    -PR                  - separate prompts and randoms" << endl;
    //  cout << "    -byte      - 8-bit sinogram bins"  << endl;
    cout << "    -d duration[,skip]   - frame duration in seconds, optionaly skip in specified seconds" << endl;
    cout << "    -d duration[*repeat] - frame duration in seconds, optionaly repeat specified duration" << endl;
    cout << "    -scan                - scans file for prompts and randoms and prints headcurve to console" << endl;
    cout << "    -count N             - stop after N events" << endl;
    cout << "    -start N             - start histogramming when trues/sec is higher than N" << endl;
    cout << "    -v                   - verbose mode: print warning messages" << endl;
    cout << "    -q                   - quiet mode: don't print time messages" << endl;
    cout << "    -add file            - Add to existing sinogram file" << endl;
    cout << "    -mock mock_file      - Output shifted mock sinogram file (transmission only)" << endl;
    cout << "    -ra                  - Output randoms sinogram file (emission only)" << endl;
    cout << "    -l log_file          - append logging to the file or create the file if not existing" << endl;
    cout << "    -notimetag           - No timetag processing, use timetag count as time" << endl;
    cout << "    -nodoi               - No DoI processing, back layer events processed as front layer" << endl;
    cout << "    -L type              -low resolution mode (1:binsize 2mm, nbins=160, span 7, maxrd 38)" << endl;
    cout << "                                              (2:binsize 2.4375mm, nbins= 128, span 7, maxrd 38)" << endl;
    cout << "    -model  328|2393|2395 (default=328) " << endl;
    cout << "    -EB                  - Exclude Border crystals (Included by default)" << endl;
    // ahc
    cout << " *  -r <hrrt_rebinner.lut file>: (Required) full path to rebinner.lut file" << endl;
    cout << endl;
    cout << "Note:  Multiple -d specifiers indicate dynamic sequencing and will generate multiple" << endl;
    cout << "       output sinograms and headers" << endl;
    cout << "       ex: lmhistogram test.l64 -span 9 -d 300*12 -d 600*4" << endl;
    cout << "       will generate 16 span 9 sinograms:" << endl;
    cout << "       - 12 of 300seconds (test_frame0.s ... test_frame11.s)" << endl;
    cout << "       -  4 of 600seconds (test_frame12.s ... test_fram15.s)" << endl;
}


/*
void parse_valid(int argc, char **argv)
{
    int count, multi_spec[2];
    char buffer[128], *ext = NULL;
    if (argc == 1) lmhistogram_usage(argv[0]);
    if (flagval("-o", &argc, argv, buffer)) {
        strcpy(out_fname, buffer);
        if ((ext = strrchr(out_fname, '.')) != NULL) {
            if (strcasecmp(ext + 1, "l32") == 0) out_l32 = 1;
            else if (strcasecmp(ext + 1, "l64") == 0) out_l64 = 1;
        }
    }
    if (flagval("-L", &argc, argv, buffer, true)) {
        int type = atoi(buffer);
        switch (type) {
            case LR_20: LR_type = LR_20; break;
            case LR_24: LR_type = LR_24; break;
            default: cout << "*** invalid LR mode ***" << endl << endl;
            lmhistogram_usage(argv[0]);
            exit(0);
        }
        g_span = 7;  // default span in LR mode
        max_rd = 38; // default max_rd  in LR mode
    }
    if (flagval("-span", &argc, argv, buffer)) {
        span_override = 1;
        g_span = atoi(buffer);
        if (g_span < 0 || g_span > 9 || (g_span > 0 && (g_span % 2) == 0)) {
            cerr << "Invalid span argument: -span " << buffer << endl;
            exit(0);
        }
    }
    if (flagset("-PR", &argc, argv))        hist_mode = 1;
    if (flagset("-ra", &argc, argv))        output_ra = 1;
    if (flagset("-P", &argc, argv))         hist_mode = 2;
    if (flagval("-model", &argc, argv, buffer)) { model_number = atoi(buffer); }
    //  if (flagset("-byte",&argc,argv))      elem_size = 1;
    if (flagset("-scan", &argc, argv))        do_scan = 1;
    //  if (flagset("-split", &argc, argv))      split = 1;
    if (flagset("-v", &argc, argv))        verbose = 1;
    if (flagset("-q", &argc, argv))        quiet = 1;
    if (flagval("-add", &argc, argv, buffer))        g_prev_sino = strdup(buffer);

    if (flagval("-l", &argc, argv, buffer))
        log_fname = strdup(buffer);

    if (flagval("-mock", &argc, argv, buffer))
        outfname_mock = strdup(buffer);

    if (flagset("-notimetag", &argc, argv))
        timetag_processing = 0;

    if (flagset("-nodoi", &argc, argv))
        rebinner_method = rebinner_method | NODOI_PROCESSING;  // ignore DOI
    if (flagset("-EB", &argc, argv))
        rebinner_method = rebinner_method | IGNORE_BORDER_CRYSTAL;  // exclude Border crystals

    if (flagval("-count", &argc, argv, buffer)) {
        if ((count = atoi(buffer)) <= 0) {
            cerr << " Invalid -count " << buffer << " argument: must be positive" << endl;
            exit(1);
        }
        stop_count = count;
    }

    if (flagval("-start", &argc, argv, buffer)) {
        if ((count = atoi(buffer)) <= 0) {
            cerr << " Invalid -start " << buffer << " argument: must be positive" << endl;
            exit(1);
        }
        start_countrate = count;
    }

    // ahc: hrrt_rebinner.lut is now required arg 'r'
    if (flagval("-r", &argc, argv, buffer))
        lut_file = strdup(buffer);

    while (flagval("-d", &argc, argv, buffer)  && nframes < MAX_FRAMES) {
        if (sscanf(buffer, "%d*%d", &multi_spec[0], &multi_spec[1]) == 2) {
            // frame_duration*repeat format
            if (multi_spec[0] > 0) {
                cout << multi_spec[0] << "sec frame  repeated " << multi_spec[1] << endl;
                for (count = 0; count < multi_spec[1] && nframes < 256; count++)
                    g_frames_duration[nframes++] = multi_spec[0];
            } else {
                cerr << "Invalid frame duration argument: -d " << buffer << endl;
                exit(0);
            }
        } else {
            // frame_duration[,skip] format
            if (sscanf(buffer, "%d,%d", &g_frames_duration[nframes], &skip[nframes]) < 1) {
                cerr << "Invalid frame duration argument: -d " << buffer << endl;
                exit(0);
            }
            nframes++;
        }
    }

    //
    // Validate arguments
    //
    if (hist_mode != 0 && g_prev_sino != NULL) {
        cerr << "Adding to existing sinogram only supported in Trues mode"  << endl;
        exit(1);
    }

    // ahc
    if (lut_file == NULL) {
        cerr << "hrrt_rebinner.lut must be specified as argument 'r'" << endl;
        exit( 1 );
    }

    if (nframes > 1 && g_prev_sino != NULL) {
        cerr << "Adding to existing sinogram only supported multi-frame mode"  << endl;
        exit(1);
    }
    // by default there is at least one frame
    if (nframes == 0) nframes++;

    // locate the filename to work on
    if (argc > 1)
        strcpy(g_in_fname, argv[1]);

<<<<<<< Updated upstream
=======
    if (strstr(g_in_fname, ".l64")) {
        //      cerr << "ERROR:  Histogrammer only functions with rebinned data" << endl;
        //      cerr << "Press Enter to continue" << endl;
        //      getchar();
        l64_flag = 1;
    } else if (strstr(g_in_fname, ".l32") == NULL) {
        cerr << g_in_fname << "extension should be .l32 or .l64\n" << endl;
        exit(1);
    }

>>>>>>> Stashed changes
    // set skip to specified countrate
}
*/

// void start_reader_thread()
// Create buffers, Open file listmode file
void start_reader_thread()
{
    //
    // Create container synchronization event and
    // start reader thread and wait for container ready (half full)
    //
        L64EventPacket::frame_duration = (int *)calloc(nframes, sizeof(int));
        memcpy(L64EventPacket::frame_duration, g_frames_duration, nframes * sizeof(int));
        L64EventPacket::frame_skip = (int *)calloc(nframes, sizeof(int));
        memcpy(L64EventPacket::frame_skip, skip, nframes * sizeof(int));
        lm64_reader(g_in_fname);
}

inline void start_rebinner_thread() // Start thread and wait until it is ready
{
    int ID1 = 0, ID2 = 1, ID3 = 2, ID4 = 3;
    int max_try = 30;
    int ncrystals = NDOIS * NXCRYS * NYCRYS * NHEADS;
    int head_nblks = (NXCRYS / 8) * (NYCRYS / 8); // 9*13=117
    p_coinc_map = (unsigned *)calloc(ncrystals, sizeof(unsigned));
    d_coinc_map = (unsigned *)calloc(ncrystals, sizeof(unsigned));
    // h_threads[1] = (HANDLE) _beginthread(lm64_rebinner,0, &ID1);
}


inline int get_num_cpus()
{
    int num_cpus = 1;
    const char *s = getenv("NUMBER_OF_PROCESSORS");
    if (s != NULL) sscanf(s, "%d", &num_cpus);
    return num_cpus;

}

/**
 * @brief      Opens a file.
 * Replace this with streams.
 */

FILE *open_file(std::string filename, std::string mode = "w") {
    FILE *fileptr = fopen(filename, "w");
    if (!fileptr) {
        perror(filename);
        exit(1);
    }
    return fileptr;
}

/**
 * create_files:
 * Create/Open output files before starting histogramming.
 * Log error and exit if file creation fails
 */
static void create_files() {
  g_out_fname_hdr = make_file_name(SINO_HDR);
  g_out_fname_hc  = make_file_name(LM_HC);

    // strcpy(g_out_fname_hdr, g_out_fname_sino);
    // strcat(g_out_fname_hdr, ".hdr");
    // strcpy(g_out_fname_hc, g_out_fname_sino);
    // // Replace extension by "_lm.hc"
    // char *ext = strrchr(g_out_fname_hc, '.');
    // if (ext != NULL) strcpy(ext, "_lm.hc");
    // else strcat(g_out_fname_hc, "_lm.hc");

  g_out_hc = open_file(g_out_fname_hc, "w");

    // g_out_hc = fopen(g_out_fname_hc, "w");
    // if (g_out_hc == NULL) {
    // if (hist_mode == 1 || hist_mode == 7) {
        // prompt && ramdoms or transmission
        // int ext_pos = strlen(g_out_fname_sino) - 2; // default is .s
        // char *ext = strrchr(g_out_fname_sino, '.');
        // if (ext != NULL) ext_pos = (int)(ext - g_out_fname_sino);
        if (hist_mode == 7) {
            // transmission
            if (elem_size != 2) {
                printf("Byte format not supported in transmission mode\n");
                exit(1);
            }
          out = open_file(g_out_fname_sino, RW_MODE);
            // if ((out = fopen(g_out_fname_sino, RW_MODE)) == NULL) {
            if (outfname_mock) {
              out2 = open_file(outfname_mock, RW_MODE);
                // if ((out2 = fopen(outfname_mock, RW_MODE)) == NULL) {
            }
        } else if (hist_mode == 1) {
            // Keep original filename for prompts in prompts or prompts+delayed mode
            // strncpy(g_out_fname_pr,g_out_fname_sino, ext_pos);
            // strcpy(g_out_fname_pr+ext_pos,".pr.s");
          g_out_fname_pr = g_out_fname_sino;
          out = open_file(g_out_fname_pr);
            // strcpy(g_out_fname_pr, g_out_fname_sino);
            // if ((out = fopen(g_out_fname_pr, RW_MODE)) == NULL) {
          g_out_fname_ra = make_file_name(FT_RA_S);
          out2 = open_file(g_out_fname_ra, "rw");
            // strncpy(g_out_fname_ra, g_out_fname_sino, ext_pos);
            // strcpy(g_out_fname_ra + ext_pos, ".ra.s");
            // if (output_ra) {
            //     if ((out2 = fopen(g_out_fname_ra, RW_MODE)) == NULL) {
            if (elem_size == 2) {
              g_out_fname_tr = make_file_name(FT_TR_S);
              out3 = open_file(g_out_fname_tr, RW_MODE);
            //     strncpy(g_out_fname_tr, g_out_fname_sino, ext_pos);
            //     strcpy(g_out_fname_tr + ext_pos, ".tr.s");
            //     if ((out3 = fopen(g_out_fname_tr, RW_MODE)) == NULL) {
        }
    } else {
      out = open_file(g_out_fname_sino, RW_MODE);
        // if ((out = fopen(g_out_fname_sino, RW_MODE)) == NULL) {
    }
    cout << "Output File: " << g_out_fname << endl;
    cout << "Output File Header: " << g_out_fname_hdr << endl;
// }

/**
 * close_files:
 * Close output files created by create_files
 */
static void close_files()
{
    fclose(out);
    if (out2 != NULL) fclose(out2);
    if (out3 != NULL) fclose(out3);
    fclose(g_out_hc);
    out = out2 = out3 = NULL;
    g_out_hc = NULL;
}

/**
 * write_sino:
 * Write sinogram and header to file.
 * Reuse existing listmode header to create sinogram header.
 * Header file name is derived from data file name (data_fname.hdr).
 * Log error and exit when an I/O error is encountered.
 */
static void write_sino(char *sino, int sino_size, CHeader &hdr, int frame)
{
    // create the output header file
    char *p = NULL;
    short *ssino = (short *)sino;
    short *delayed = (short *)(sino + sino_size * elem_size);
    short *mock = (short *)(sino + sino_size * elem_size);
    int i = 0, span9_sino_size = m_nprojs * m_nviews * nsinos[9];

    hdr.WriteTag("image duration", prev_duration + g_frames_duration[frame]);
    hdr.WriteTag("image relative start time", relative_start);
    int decay_time = relative_start + dose_delay;
    float dcf1 = 1.0f;
    float dcf2 = 1.0f;
    if (decay_rate > 0.0) {
        dcf1 = (float)pow(2.0f, decay_time / decay_rate);
        //      dcf2 = ((float)log(2.0) * duration[frame]) / (decay_rate * (1.0 - (float)pow(2,-1.0*duration[frame]/decay_rate)));
        dcf2 = (float)((log(2.0) * g_frames_duration[frame]) / (decay_rate * (1.0 - pow(2.0f, -1.0f * g_frames_duration[frame] / decay_rate))));
    }
    hdr.WriteTag("decay correction factor", dcf1);
    hdr.WriteTag("decay correction factor2", dcf2);
    hdr.WriteTag("frame", frame);
    hdr.WriteTag("Total Prompts", total_prompts());
    hdr.WriteTag("Total Randoms", total_randoms());
    hdr.WriteTag("Total Net Trues", total_prompts() - total_randoms());
    int av_singles = 0;
    for (int block = 0; block < NBLOCKS; block++) {
        char tmp1[64];
        av_singles += singles_rate(block);
        sprintf(tmp1, "block singles %d", block);
        hdr.WriteTag(tmp1, singles_rate(block));
    }
    av_singles /= NBLOCKS;
    hdr.WriteTag("average singles per block", av_singles);
    cout << "LLD = " << LLD << "  Average Singles Per Block = " << av_singles << endl;
    hdr.WriteTag("Dead time correction factor", GetDTC(LLD, av_singles));

    // Write Trues or Prompts sinogram
    int count = elem_size, write_error = 0;
    printf("sinogram size: %d\n", sino_size);
    printf("element size: %d\n", elem_size);

    // Log message and set first data filename
    switch (hist_mode) {
    case 0: // Randoms substractions
    printf("Writing Trues Sinogram: %s\n", g_out_fname_sino);
    hdr.WriteTag("!name of data file", g_out_fname_sino);
    hdr.WriteTag("sinogram data type", "true");
    count = fwrite(sino, sino_size, elem_size, out);
    break;
    case 1: // Prompts and delayed: prompts is first
    printf("Writing Prompts Sinogram: %s\n", g_out_fname_pr);
    hdr.WriteTag("!name of data file", g_out_fname_pr);
    hdr.WriteTag("sinogram data type", "prompt");
    hdr.WriteTag("!name of true data file", g_out_fname_tr);
    sprintf(g_out_fname_hdr, "%s.hdr", g_out_fname_pr);
    count = fwrite(sino, sino_size, elem_size, out);
    break;
    case 2: //Prompts only
    printf("Writing Prompts Sinogram: %s\n", g_out_fname_sino);
    hdr.WriteTag("!name of data file", g_out_fname_sino);
    hdr.WriteTag("sinogram data type", "prompt");
    break;
    case 7: // Transmission
    printf("Writing EC corrected Sinogram: %s\n", g_out_fname_sino);
    sprintf(g_out_fname_hdr, "%s.hdr", g_out_fname_sino);
    hdr.WriteTag("!name of data file", g_out_fname_sino);
    for (int i = 0; i < sino_size; i++) {
        ssino[i] = ssino[i] - mock[i];
        if (ssino[i] < 0) ssino[i] = 0;
    }
    if (fwrite(sino, sino_size, elem_size, out) == elem_size) {
        hdr.WriteFile(g_out_fname_hdr);
    } else {
        printf("Error writing %s\n", g_out_fname_sino);
        write_error++;
    }
    break;
}

    // Write data
if (count != (int)elem_size) {
    printf("Error writing file\n");
    write_error++;
}
if (!write_error) hdr.WriteFile(g_out_fname_hdr);

    // Log message and write second data filename
if (elem_size == 2 && write_error == 0) switch (hist_mode) {
        case 1: // Prompts and delayed: trues is second if short type
        sprintf(g_out_fname_hdr, "%s.hdr", g_out_fname_tr);
        hdr.WriteTag("!name of data file", g_out_fname_tr);
        hdr.WriteTag("sinogram data type", "true");
            // convert span3 prompt sinogram to span9 and substract delayed

            for (i = 0; i < sino_size; i++) ssino[i] =  ssino[i] - delayed[i];
            if (g_span == 3) {
                printf ("Converting true sino to span9\n");
                convert_span9(ssino, max_rd, 104);
                hdr.WriteTag("axial compression", 9);
                hdr.WriteTag("matrix size [3]", nsinos[9]);
            }
            printf("Writing Trues Sinogram: %s\n", g_out_fname_tr);
            if (fwrite(ssino, g_span == 3 ? span9_sino_size : sino_size, elem_size, out3) == elem_size) {
                hdr.WriteFile(g_out_fname_hdr);
            } else {
                printf("Error writing %s\n", g_out_fname_tr);
                write_error++;
            }
            // Restore span3 header
            if (g_span == 3) {
                hdr.WriteTag("axial compression", 3);
                hdr.WriteTag("matrix size [3]", nsinos[3]);
            }
            break;
        case 7: // Transmission: EC corrected is second if short type
        if (out2 != NULL) {
            printf("Writing Mock Sinogram: %s\n", outfname_mock);
            sprintf(g_out_fname_hdr, "%s.hdr", outfname_mock);
            hdr.WriteTag("!name of data file", outfname_mock);
            if (fwrite(mock, sino_size, elem_size, out2) == elem_size) {
                hdr.WriteFile(g_out_fname_hdr);
            } else {
                printf("Error writing %s\n", outfname_mock);
                write_error++;
            }
            break;
        }
    }

    // Log message and write third data filename
    if (output_ra && write_error == 0 && hist_mode == 1) {
        printf("Writing Randoms Sinogram: %s\n", g_out_fname_ra);
        sprintf(g_out_fname_hdr, "%s.hdr", g_out_fname_ra);
        hdr.WriteTag("!name of data file", g_out_fname_ra);
        hdr.WriteTag("sinogram data type", "delayed");
        count = fwrite(delayed, span9_sino_size, elem_size, out2);
        if (count == (int)elem_size) {
            hdr.WriteFile(g_out_fname_hdr);
        } else {
            printf("Error writing %s\n", g_out_fname_ra);
            write_error++;
        }
    }
    if (write_error) exit(1);
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
  strcpy(g_out_fname_hc, g_in_fname);
  // Replace extension by "_lm.hc"
  char *ext = strrchr(g_out_fname_hc, '.');
  if (ext != NULL)
    strcpy(ext, "_lm.hc");
  else
    strcat(g_out_fname_hc, "_lm.hc");
  FILE *outfile_hc = fopen(g_out_fname_hc, "w");
  if (outfile_hc == NULL) {
    perror(g_out_fname_hc);
    exit(1);
  }
  memset(g_frames_duration, 0, sizeof(g_frames_duration));
  start_reader_thread();
  lmscan(outfile_hc, &scan_duration);
  fclose(outfile_hc);

        //      for (int index = 0; index < duration; index++)
        //          cout << hc[index].time << " " << hc[index].randoms_rate << " " << hc[index].trues_rate << " " << hc[index].singles << endl;

  cout << "Scan Duration = " << scan_duration << endl;

}

/**
 * @brief      Makes a file name from the input file g_in_fname
 * @param[in]  file_type  The file type
 * @return     String 
 */
std::string make_file_name(FILE_TYPE file_type) {
  std::string file_name = g_in_fname;

  switch (file_type) {
    case SINO: {  
      file_name = g_in_fname.replace_extension(".s");
      break;
    }
    case SINO_HDR: {
      file_name = g_in_fname.replace_extension(".s.hdr");
      break;
    }
    default: {
      assert(false);
    }
  }
  return file_name;
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
  console = spdlog::basic_logger_mt("basic_logger", g_logfile);
}

int main(int argc, char *argv[]) {
  int frame = 0;
  int count = 0, multi_spec[2];
  char *sinogram = NULL, *bsino = NULL, *delayed = NULL;
  short *ssino = NULL;
  int current_time = -1;
  char *cptr = NULL;
  // char datatype[32];
  string datatype;
  CHeader hdr;
  char msg[FILENAME_MAX];

  g_sw_build_id = fmt::format("{:s} {:s}", __DATE__, __TIME__);

  parse_boost(argc, argv);
  init_logging();
  // auto console = spdlog::stdout_color_mt("console");


  // set input header filename
  in_fname_hdr = fmt::format("{}.{}", in_fname, "hdr");
  sprintf(msg, "%s %s", argv[0], in_fname);
  log_message(msg);
  if (access(g_in_fname, 4) != 0) {
    sprintf(msg, "%s: File not found", g_in_fname);
    log_message(msg, 2);
    return 1;
  }
  if (hdr.OpenFile(g_in_fname_hdr) == 1) {
    sprintf(msg, "Using header: [%s]", g_in_fname_hdr);
    log_message(msg);
    hdr.CloseFile();
  } else {
    sprintf(msg, "header [%s] not found", g_in_fname_hdr);
    log_message(msg);
  }

  int num_cpus = get_num_cpus();
  if (num_cpus > MAX_THREADS)
    num_cpus = MAX_THREADS; // workaround for dual quadcore bug

  //
  // for transmission scans, always override span to 0 (segment 0 only)
  if (!hdr.Readchar("!PET data type", datatype)) {
    cout << "Data Type: '" << datatype << "'" << endl;

    // ahc this was failing because of trailing CR on DOS format hdr file.
    //        if (strcasecmp(datatype, "transmission") == 0) {
    cout << "XXX: " << (datatype == "transmission") << endl;
    if (datatype == "transmission") {
      float axial_velocity = 0.0;
      g_span = 0;
      hist_mode = 7;
      cout << "Data Type: " << datatype << ", hist_mode: " << hist_mode << endl;
      if (hdr.Readfloat("axial velocity", axial_velocity) == 0) {
        // tx speed=6msec/crystal for axial velocity=100%
        float scale_factor = axial_velocity / 100.0f;
        tx_source_speed = 6.25f / scale_factor;
        cout << "Axial velocity =" << axial_velocity << endl;
        cout << "Transmission source speed=" << tx_source_speed << " msec/crystal" << endl;
      }
    }
  }

  // Decode frame duration even when creating l32 file to output CH files
  //
  if (g_frames_duration[0] == 0) {
    // Frame duration not specified at command line; Use header
    // char framedef[256];
    // Need to convert this to C++ strings.
    cout << "COME BACK TO THIS" << endl;
    string framedef_str;
    hdr.Readchar("Frame definition", framedef_str);
    if (framedef_str.length()) {
      char framedef[1024];
      strcpy(framedef, framedef_str.c_str());
      nframes = 0;
      cout << "Frame definition: [" << framedef << "]" << endl;
      cptr = strtok(framedef, ", ");
      while (cptr) {
        if (sscanf(cptr, "%d*%d", &multi_spec[0], &multi_spec[1]) == 2) {
          if (multi_spec[0] > 0) {
            cout << multi_spec[0] << "sec frame  repeated " << multi_spec[1] << endl;
            for (count = 0; count < multi_spec[1] && nframes < 256; count++)
              g_frames_duration[nframes++] = multi_spec[0];
          } else {
            cout << g_in_fname_hdr << ": Invalid frame specification \"" << cptr << "\"" << endl;
            break;
          }
        } else {
          if (sscanf(cptr, "%d", &count) == 1 && count > 0) g_frames_duration[nframes++] = count;
          else {
            cout << g_in_fname_hdr << ": Invalid frame specification \"" << cptr << "\"" << endl;
            break;
          }
        }
        cptr = strtok(0, ", ");
      }
      cout << "number of frames: " << nframes << endl;
      // if something went wrong during parsing, set proper defaults
      if (nframes == 0) {
        nframes++;
        g_frames_duration[0] = 0;
      }
    } else
      cout << "Using commandline frame definition" << endl;
  }

  if (do_scan) {
    do_lmscan();
  } else {
    int axial_comp;
    float isotope_halflife = 0.0;
    // set output filename if an input has not been given
    if (g_out_fname.length() == 0) {
      g_out_fname = in_fname;
      std::size_t found = g_out_fname.find_first_of(".");
      if (found != std::string::npos) {
        g_out_fname.erase(found, string::npos);
        g_out_fname.append(".s")
        std::cout << fmt::format("g_out_fname: {}", g_out_fname) << std::endl;
      }
    }

    if (hdr.Readint("lower level", LLD))
      LLD = DEFAULT_LLD;
    cout << "LLD: " << LLD << endl;

    // Build the headers
    int sino_mode = 0;  // by default use net trues
    if (!hdr.Readint("Sinogram Mode", sino_mode)) {
      cout << "Using Sinogram Mode from header: " << sino_mode << endl;
      if (sino_mode != 0 && g_prev_sino != NULL) {
        cout << "Adding to existing sinogram only supported in Trues mode"  << endl;
        exit(1);
      }
      hist_mode = sino_mode;
    }

    hdr.Readfloat("isotope halflife", isotope_halflife);
    if (isotope_halflife != 0.0)
      decay_rate = (float)isotope_halflife;

    get_dose_delay(hdr, dose_delay);
    if (!hdr.Readint("axial compression", axial_comp)) {
      if (!span_override && axial_comp > 0)
        g_span = axial_comp;
      else
        hdr.WriteTag("axial compression", g_span);
    }

    // update the header with sinogram info for HRRT
    hdr.WriteTag("!originating system"      , "HRRT");
    hdr.WriteTag("!name of data file"       , g_out_fname_sino);
    hdr.WriteTag("number of dimensions"     , "3");
    hdr.WriteTag("matrix size [1]"          , num_projs(LR_type));
    hdr.WriteTag("matrix size [2]"          , num_views(LR_type));
    hdr.WriteTag("matrix size [3]"          , LR_type == LR_0 ? nsinos[g_span] : LR_nsinos[g_span]);
    hdr.WriteTag("data format"              , "sinogram");
    hdr.WriteTag("number format"            , "signed integer");
    hdr.WriteTag("number of bytes per pixel", (int)elem_size);
    hdr.WriteTag("!lmhistogram version"     , sw_version);
    hdr.WriteTag("!lmhistogram build ID"    , g_sw_build_id);
    hdr.WriteTag("!histogrammer revision"   , "2.0");


    int span_bak = g_span, rd_bak = max_rd;
    // ahc: lut_file now passed to init_rebinner instead of finding it there.
    // init_rebinner(g_span, max_rd);
    init_rebinner(g_span, max_rd, lut_file);
    sprintf(msg, "using rebinner LUT %s", rebinner_lut_file);
    log_message(msg);

    hdr.WriteTag("!LM rebinner method", (int)rebinner_method);
    hdr.WriteTag("axial compression", g_span);
    hdr.WriteTag("maximum ring difference", max_rd);
    g_span = span_bak; max_rd = rd_bak;
    hdr.WriteTag("scaling factor (mm/pixel) [1]", 10.0f * m_binsize); // cm to mm
    hdr.WriteTag("scaling factor [2]", 1); // angle
    hdr.WriteTag("scaling factor (mm/pixel) [3]", 10.0f * m_plane_sep); // cm to mm


    cout << "Span: " << g_span << endl;

    static const char *sino_mode_txt[3] = { "Net Trues", "Prompts & Randoms", "prompts only"};
    if (hist_mode == 7)
      cout << "Mode: Transmission" << endl;
    else
      cout << "Mode: " << sino_mode_txt[hist_mode] << endl;

    // remove the '.s' extension if it exists
    if (out_fname[strlen(out_fname) - 1] == 's')
      out_fname[strlen(out_fname) - 2] = '\0';

    // create the output filename with full path stripped.
    char out_fname_stripped[1024];
    if (cptr = strrchr(out_fname, '\\'))
      strcpy(out_fname_stripped, (cptr + 1));
    else
      strcpy(out_fname_stripped, out_fname);

    // create the .dyn dynamic description file if this is a dynamic study
    char outfname_dyn[1024];
    strcpy(outfname_dyn, out_fname);
    strcat(outfname_dyn, ".dyn");

    if (nframes > 1) {
      FILE *fp = fopen(outfname_dyn, "w");
      if (fp) {
        fprintf(fp, "%d\n", nframes);
        fclose(fp);
      }
    }

    // Check start countrate and set first frame skip
    if (start_countrate > 0) {
      int skip_msec = find_start_countrate(g_in_fname);
      if (skip_msec > 0) {
        skip[0] = skip_msec / 1000; //msec
        sprintf(msg, "%d sec will be skipped to start with %d trues/sec countrate",
                skip[0], start_countrate);
        log_message(msg);
      }
    }

    //start reader and rebinner threads
    start_reader_thread();
    start_rebinner_thread();        // start lm64_rebinner thread  and wait until ready

    // process each frame
    frame = 0;
    while (frame < nframes) {
      //
      // output histogram is filename_frameX.s
      // output histogram header is filename_frameX.s.hdr
      //
      if (nframes > 1) {
        sprintf(g_out_fname_sino, "%s_frame%d.s", out_fname, frame);
        FILE *fp = fopen(outfname_dyn, "a");
        if (fp) {
          fprintf(fp, "%s_frame%d.s\n", out_fname_stripped, frame);
          fclose(fp);
        }
      } else {
        sprintf(g_out_fname_sino, "%s.s", out_fname);
      }

      // Create output files; exit if fail
      create_files();

      if (elem_size == 2) {
        if (!init_sino<short>(ssino, delayed, g_span, g_prev_sino, prev_duration)) exit(1);
        sinogram = (char *)ssino;
      } else {
        if (!init_sino<char>(bsino, delayed, g_span, g_prev_sino, prev_duration)) exit(1);
        sinogram = bsino;
      }


      // Position input stream to time 0 or requested time
      cout << "Histogramming Frame " << frame << " duration=" << g_frames_duration[frame] << " seconds " << endl;
      if (skip[frame] > 0) {
        int time_pos = skip[frame];
        if (current_time > 0) time_pos += current_time;
        if (!goto_event(skip[frame])) {
          cout << "Unable to locate requested time " << time_pos << endl;
          exit(1);
        }
      } else {
        // skipping events before coincindence processor time reset.
        if (current_time < 0)
          if (!goto_event(0)) {
            cout << "Unable to locate time reset" << endl;
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
        printf("Invalid frame duration %d\n", g_frames_duration[frame]);
        exit(1);
      }

      // update current time
      current_time = relative_start + g_frames_duration[frame];
      scan_duration += g_frames_duration[frame];

      // Write sinogram and header files
      write_sino(sinogram, g_sinogram_size, hdr, frame);
      close_files();
      if (hist_mode != 7)
        write_coin_map(g_out_fname_sino);
      if (nframes > 1)
        sprintf(msg, "Frame time histogrammed = %d sec", g_frames_duration[frame]);
      else
        sprintf(msg, "Time histogrammed = %d sec", g_frames_duration[frame]);
      log_message(msg);
      frame++;

    }
    if (nframes > 1) {
      sprintf(msg, "Total time histogrammed = %d secs", current_time);
      log_message(msg);
    }
    // complete the main data files
    free(sinogram);

  }
  if (L64EventPacket::frame_duration != NULL) {
    free (L64EventPacket::frame_duration);
    free (L64EventPacket::frame_skip);
  }
  clean_segment_info();
  return 0;
}
