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
*/

#include "Flags.h"
#include "Header.h"
#include "dtc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "LM_Rebinner_mp.h"
#include "histogram_mp.h"
#include "LM_Reader_mp.h"
#include "convert_span9.h"
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>

// ahc
#include <unistd.h>
#include <iostream>

#define DEFAULT_LLD 400 // assume a value of 400 if not specified in listmode header
#define RW_MODE "wb+"
#ifdef WIN32
#include <io.h>
#include <process.h>
#define strcasecmp _stricmp
#define strdup _strdup
#define access _access
#endif
static const char *pgm_id = "V2.1 ";  // Revision changed from 2.0 to 2.1 for LR and P39 support

#define MAX_FRAMES 256
#define MAX_THREADS 4

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
static int p39_nsegs = 25;
static const char *p39_seg_table = "{239,231,231,217,217,203,203,189,189,175,175,161,161,147,147,133,133,119,119,105,105,91,91,77,77}";

static char in_fname[FILENAME_MAX];  // input listmode file name
static char in_fname_hdr[FILENAME_MAX]; // input listmode header file name
static char out_fname[FILENAME_MAX];  // output sinogram file basename
static char outfname_hc[FILENAME_MAX]; // output Head Curve file name
static char outfname_sino[FILENAME_MAX]; //current frame sinogram file name
static char outfname_hdr[FILENAME_MAX]; //current frame sinogram header file name
static char outfname_ra[FILENAME_MAX]; //current frame sinogram randoms file name
static char outfname_pr[FILENAME_MAX]; //current frame sinogram prompts file name
static char outfname_tr[FILENAME_MAX]; //current frame sinogram Trues file name
static char outfname_p39_em[FILENAME_MAX]; //current frame sinogram emission file name
static char outfname_p39_tx[FILENAME_MAX]; //current frame sinogram transmission file name
static char fname_p39_mhdr[FILENAME_MAX]; // input listmode main header file name
static char *outfname_mock = NULL; //Mock sinogram file name for transmission only

static FILE *out_hc = NULL;             // Head Curve output File Pointer
static FILE *out = NULL;    // output trues or prompts sinogram File Pointer
static FILE  *out2 = NULL;              // output randoms sinogram File Pointer
static FILE  *out3 = NULL;              // output Trues sinogram File Pointer for separate prompts and randoms mode

static int duration[MAX_FRAMES], skip[MAX_FRAMES]; // Frames duration and optional skip
static int dose_delay = 0;          // elapsed time between dose assay time and scan start time
static int relative_start = 0;       // Current frame start time
static int prev_duration = 0;        // Previous sinogram duration (when -add option)
static long scan_duration = 0;          // P39 TX duration is total time scan duration

// Parameters which default may be overrided in lmhistogram.ini
static int span = 9;                // Default span=9 for l64 if not specified in header
static size_t elem_size = 2;        // sinogram elem size: 1 for byte, 2 for short (default)
static int num_sino, sinogram_size;  // number of sinogram and size
static int compression = 0;         // 0=no compression (default), 1=ER (Efficient Representation)
static int output_ra = 0;           // output randoms

static int span_override = 0;       // Flag set span when span specified at command line
static int nframes = 0;             // Number of frames
static int scan = 0;                // scan option , default=no
static int split = 0;               // split option, default=no
static int out_l32 = 0;             // Output 32-bit listmode (output file extension .l32)
static int out_l64 = 0;             // Output 64-bit listmode (output file extension .l64)

static char  *prev_sino = 0;        // 1: add to existing normalization scan
static char *log_fname = NULL;          // log file
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
char sw_build_id[80];

void lmhistogram_usage(char *pgm_name)
{
    cout << pgm_name << " " << pgm_id << " Build: " << sw_build_id << endl << endl;
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


/**
 * init_sino:
 * Allocates the sinogram in memory and initializes it with 0 or sinogram loaded from the prev_sino argument.
 * Sets the duration with prev_sino duration value from prev_sino header.
 * Returns 1 if success and 0 otherwise
 */
template <class T> int init_sino(T *&sino, char *&delayed, int span, const char *prev_sino, int &duration)

{
    int sinogram_subsize = 0;
    int r_size = sizeof(T);

    if (check_model(model_number, MODEL_HRRT)) {
        // HRRT
    		cout << "XX Using HRRT model" << endl;
        if (hist_mode != 7) {
            // emission
   	    		cout << "XX Using HRRT Emission: hist_mode = " << hist_mode << endl;
            num_sino = LR_type == 0 ? nsinos[span] : LR_nsinos[span];
            sinogram_size = m_nprojs * m_nviews * num_sino;
            if (hist_mode == 1) {
                // prompts and delayed
                sinogram_subsize = sinogram_size;
            }
        } else {
            // transmission
   	    		cout << "XX Using HRRT Transmission, hist_mode = " << hist_mode << endl;
   	    		num_sino = LR_type == 0 ? nsinos[0] : LR_nsinos[0];
            sinogram_size = m_nprojs * m_nviews * num_sino;
            sinogram_subsize = sinogram_size; // mock memory
        }
    } else { // P39
        num_sino = P39_nsinos[span];
        sinogram_size = m_nprojs * m_nviews * num_sino;
    }

    if (num_sino == 0) {
        cout << "Unsupported span : " << span << endl;
        return 0;
    }

    cout << "number of elements: " << m_nprojs << endl;
    cout << "number of angles: " << m_nviews << endl;
    cout << "number of sinograms: " << num_sino << endl;
    cout << "bits per register: " << r_size * 8 << endl;
    cout << "sinogram size: " << sinogram_size << endl;
    cout << "sinogram subsize: " << sinogram_subsize << endl;
    if (!check_model(model_number, MODEL_HRRT)) {
        // P39 : always prompt and separate delayed
        // add transmission sinogram size to prompt sinogram
        // allocate or clear memory for prompt and TX sinogram
        int tx_size =  m_nprojs * m_nviews * P39_nsinos[0];
        cout << " TX sinogram size: " << tx_size << endl;
        if (sino == NULL) sino = (T *)calloc((sinogram_size + tx_size), r_size);
        else memset(sino, 0, sinogram_size * r_size); // keep transmission events
        if (sino == NULL) {
            cout << "Unable to allocate sinogram memory: " << sinogram_size << " + " << tx_size << endl;
            return 0;
        }
        // allocate or clear memory for delayed sinogram
        if (delayed == NULL) delayed = (char *)calloc(sinogram_size, 1);
        else memset(delayed, 0, sinogram_size);
        if (delayed == NULL) {
            cout << "Unable to allocate delayed sinogram memory: " << sinogram_size << endl;
            return 0;
        }
    } else {   // HRRT
        if (sino == NULL)
            sino = (T *)calloc(sinogram_size + sinogram_subsize, r_size);
        else
            memset(sino, 0, (sinogram_size + sinogram_subsize)*r_size);

        if (!sino) {
            cout << "Unable to allocate sinogram memory: " << (sinogram_size + sinogram_subsize) * r_size << endl;
            return 0;
        }
    }

    // load previous scan if any
    if (prev_sino) {
        CHeader hdr;
        char hdr_fname[FILENAME_MAX];
        if (hist_mode != 0) {
            cout << "Adding to existing sinogram only supported Trues mode" << endl;
            free(sino);
            return 0;
        }
        sprintf(hdr_fname, "%s.hdr", prev_sino);
        if (hdr.OpenFile(hdr_fname) < 0) {
            cout << "Error opening " << hdr_fname << endl;
            free(sino);
            return 0;
        }
        int ok = hdr.Readint("image duration", &duration);
        hdr.CloseFile();
        if (ok != 0) {
            cout << hdr_fname << ": image duration not found" << endl;
            free(sino);
            return 0;
        }
        FILE *prev_fp = fopen(prev_sino, "rb");
        if (prev_fp != NULL) {
            int count = fread(sino, sinogram_size, r_size, prev_fp);
            if (count != r_size) {
                cout << "Error reading " << prev_sino << endl;
                fclose(prev_fp);
                free(sino);
                return 0;
            }
            fclose(prev_fp);
        } else {
            perror(prev_sino);
            free(sino);
            return 0;
        }
    }
    return 1;
}

/**
 * parse_valid:
 * Checks and sets parameters to user specified values
 * Log error and exit when invalid argument is encountered
 */
void parse_valid(int argc, char **argv)
{
    int count, multi_spec[2];
    char buffer[128], *ext = NULL;

    if (argc == 1) {
        lmhistogram_usage(argv[0]);
        exit(0);
    }

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
        span = 7;  // default span in LR mode
        max_rd = 38; // default max_rd  in LR mode
    }

    if (flagval("-span", &argc, argv, buffer)) {
        span_override = 1;
        span = atoi(buffer);
        if (span < 0 || span > 9 || (span > 0 && (span % 2) == 0)) {
            cerr << "Invalid span argument: -span " << buffer << endl;
            exit(0);
        }
    }

    if (flagset("-PR", &argc, argv))
        hist_mode = 1;
    if (flagset("-ra", &argc, argv))
        output_ra = 1;

    if (flagset("-P", &argc, argv))
        hist_mode = 2;

    if (flagval("-model", &argc, argv, buffer)) {
        model_number = atoi(buffer);
    }
    //  if (flagset("-byte",&argc,argv))
    //      elem_size = 1;

    if (flagset("-scan", &argc, argv))
        scan = 1;

    //  if (flagset("-split", &argc, argv))
    //      split = 1;

    if (flagset("-v", &argc, argv))
        verbose = 1;
    if (flagset("-q", &argc, argv))
        quiet = 1;
    if (flagval("-add", &argc, argv, buffer))
        prev_sino = strdup(buffer);

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
                    duration[nframes++] = multi_spec[0];
            } else {
                cerr << "Invalid frame duration argument: -d " << buffer << endl;
                exit(0);
            }
        } else {
            // frame_duration[,skip] format
            if (sscanf(buffer, "%d,%d", &duration[nframes], &skip[nframes]) < 1) {
                cerr << "Invalid frame duration argument: -d " << buffer << endl;
                exit(0);
            }
            nframes++;
        }
    }

    //
    // Validate arguments
    //
    if (hist_mode != 0 && prev_sino != NULL) {
        cerr << "Adding to existing sinogram only supported in Trues mode"  << endl;
        exit(1);
    }

    // ahc
    if (lut_file == NULL) {
        cerr << "hrrt_rebinner.lut must be specified as argument 'r'" << endl;
        exit( 1 );
    }

    if (nframes > 1 && prev_sino != NULL) {
        cerr << "Adding to existing sinogram only supported multi-frame mode"  << endl;
        exit(1);
    }
    // by default there is at least one frame
    if (nframes == 0) nframes++;

    // locate the filename to work on
    if (argc > 1)
        strcpy(in_fname, argv[1]);

    if (strstr(in_fname, ".l64")) {
        //      cerr << "ERROR:  Histogrammer only functions with rebinned data" << endl;
        //      cerr << "Press Enter to continue" << endl;
        //      getchar();
        l64_flag = 1;
    } else if (strstr(in_fname, ".l32") == NULL) {
        cerr << in_fname << "extension should be .l32 or .l64\n" << endl;
        exit(1);
    }

    // set skip to specified countrate
}

// void start_reader_thread()
// Create buffers, Open file listmode file
void start_reader_thread()
{
    //
    // Create container synchronization event and
    // start reader thread and wait for container ready (half full)
    //
    if (l64_flag) {
        L64EventPacket::frame_duration = (int *)calloc(nframes, sizeof(int));
        memcpy(L64EventPacket::frame_duration, duration, nframes * sizeof(int));
        L64EventPacket::frame_skip = (int *)calloc(nframes, sizeof(int));
        memcpy(L64EventPacket::frame_skip, skip, nframes * sizeof(int));
        lm64_reader(in_fname);

    } else {
        lm32_reader(in_fname);
    }
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
 * create_files:
 * Create/Open output files before starting histogramming.
 * Log error and exit if file creation fails
 */
static void create_files()
{
    strcpy(outfname_hdr, outfname_sino);
    strcat(outfname_hdr, ".hdr");
    strcpy(outfname_hc, outfname_sino);
    // Replace extension by "_lm.hc"
    char *ext = strrchr(outfname_hc, '.');
    if (ext != NULL) strcpy(ext, "_lm.hc");
    else strcat(outfname_hc, "_lm.hc");
    out_hc = fopen(outfname_hc, "w");
    if (out_hc == NULL) {
        perror(outfname_hc);
        exit(1);
    }
    if (hist_mode == 1 || hist_mode == 7) {
        // prompt && ramdoms or transmission
        int ext_pos = strlen(outfname_sino) - 2; // default is .s
        char *ext = strrchr(outfname_sino, '.');
        if (ext != NULL) ext_pos = (int)(ext - outfname_sino);
        if (hist_mode == 7) {
            // transmission
            if (elem_size != 2) {
                printf("Byte format not supported in transmission mode\n");
                exit(1);
            }
            if ((out = fopen(outfname_sino, RW_MODE)) == NULL) {
                perror(outfname_sino);
                exit(1);
            }
            if (outfname_mock != NULL) {
                if ((out2 = fopen(outfname_mock, RW_MODE)) == NULL) {
                    perror(outfname_mock);
                    exit(1);
                }
            }
        } else {
            // Keep original filename for prompts in prompts or prompts+delayed mode
            // strncpy(outfname_pr,outfname_sino, ext_pos);
            // strcpy(outfname_pr+ext_pos,".pr.s");
            strcpy(outfname_pr, outfname_sino);
            strncpy(outfname_ra, outfname_sino, ext_pos);
            strcpy(outfname_ra + ext_pos, ".ra.s");
            if ((out = fopen(outfname_pr, RW_MODE)) == NULL) {
                perror(outfname_pr);
                exit(1);
            }
            if (output_ra) {
                if ((out2 = fopen(outfname_ra, RW_MODE)) == NULL) {
                    perror(outfname_ra);
                    exit(1);
                }
            }
            if (elem_size == 2) {
                strncpy(outfname_tr, outfname_sino, ext_pos);
                strcpy(outfname_tr + ext_pos, ".tr.s");
                if ((out3 = fopen(outfname_tr, RW_MODE)) == NULL) {
                    perror(outfname_tr);
                    exit(1);
                }
            }
        }
    } else {
        if ((out = fopen(outfname_sino, RW_MODE)) == NULL) {
            perror(outfname_sino);
            exit(1);
        }
    }
    cout << "Output File: " << out_fname << endl;
    cout << "Output File Header: " << outfname_hdr << endl;
}

/**
 * p39_create_files:
 * Create/Open output files before starting histogramming.
 * Log error and exit if file creation fails
 */
static void p39_create_files()
{
    strcpy(outfname_hdr, outfname_sino);
    strcat(outfname_hdr, ".hdr");
    strcpy(outfname_hc, outfname_sino);
    strcpy(outfname_p39_em, outfname_sino);
    strcpy(outfname_p39_tx, outfname_sino);
    // Make HC filename
    char *ext = strrchr(outfname_hc, '.');
    if (ext != NULL) strcpy(ext, "_lm.hc");
    else strcat(outfname_hc, "_lm.hc");
    if ((out_hc = fopen(outfname_hc, "w")) == NULL) {
        perror(outfname_hc);
        exit(1);
    }
    //Make emission sino filename
    if (nframes == 1) {
        if ((ext = strrchr(outfname_p39_em, '.')) != NULL) strcpy(ext, "_em.s");
        else strcat(outfname_p39_em, "_em.s");
    }
    // else emission file names have _frameN.s extension
    if ((out = fopen(outfname_p39_em, RW_MODE)) == NULL) {
        perror(outfname_p39_em);
        exit(1);
    }
    //Make transmission sino filename
    if (nframes > 1) {
        // replace _frameN.s extension by _tx.s extension
        if ((ext = strstr(outfname_p39_tx, "_frame")) != NULL) strcpy(ext, "_tx.s");
        else strcat(outfname_p39_tx, "_tx.s");
    } else {
        if ((ext = strrchr(outfname_p39_tx, '.')) != NULL) strcpy(ext, "_tx.s");
        else strcat(outfname_p39_tx, "_tx.s");
    }
}

/**
 * close_files:
 * Close output files created by create_files
 */
static void close_files()
{
    fclose(out);
    if (out2 != NULL) fclose(out2);
    if (out3 != NULL) fclose(out3);
    fclose(out_hc);
    out = out2 = out3 = NULL;
    out_hc = NULL;
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

    hdr.WriteTag("image duration", prev_duration + duration[frame]);
    hdr.WriteTag("image relative start time", relative_start);
    int decay_time = relative_start + dose_delay;
    float dcf1 = 1.0f;
    float dcf2 = 1.0f;
    if (decay_rate > 0.0) {
        dcf1 = (float)pow(2.0f, decay_time / decay_rate);
        //      dcf2 = ((float)log(2.0) * duration[frame]) / (decay_rate * (1.0 - (float)pow(2,-1.0*duration[frame]/decay_rate)));
        dcf2 = (float)((log(2.0) * duration[frame]) / (decay_rate * (1.0 - pow(2.0f, -1.0f * duration[frame] / decay_rate))));
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
        printf("Writing Trues Sinogram: %s\n", outfname_sino);
        hdr.WriteTag("!name of data file", outfname_sino);
        hdr.WriteTag("sinogram data type", "true");
        count = fwrite(sino, sino_size, elem_size, out);
        break;
    case 1: // Prompts and delayed: prompts is first
        printf("Writing Prompts Sinogram: %s\n", outfname_pr);
        hdr.WriteTag("!name of data file", outfname_pr);
        hdr.WriteTag("sinogram data type", "prompt");
        hdr.WriteTag("!name of true data file", outfname_tr);
        sprintf(outfname_hdr, "%s.hdr", outfname_pr);
        count = fwrite(sino, sino_size, elem_size, out);
        break;
    case 2: //Prompts only
        printf("Writing Prompts Sinogram: %s\n", outfname_sino);
        hdr.WriteTag("!name of data file", outfname_sino);
        hdr.WriteTag("sinogram data type", "prompt");
        break;
    case 7: // Transmission
        printf("Writing EC corrected Sinogram: %s\n", outfname_sino);
        sprintf(outfname_hdr, "%s.hdr", outfname_sino);
        hdr.WriteTag("!name of data file", outfname_sino);
        for (int i = 0; i < sino_size; i++) {
            ssino[i] = ssino[i] - mock[i];
            if (ssino[i] < 0) ssino[i] = 0;
        }
        if (fwrite(sino, sino_size, elem_size, out) == elem_size) {
            hdr.WriteFile(outfname_hdr);
        } else {
            printf("Error writing %s\n", outfname_sino);
            write_error++;
        }
        break;
    }

    // Write data
    if (count != (int)elem_size) {
        printf("Error writing file\n");
        write_error++;
    }
    if (!write_error) hdr.WriteFile(outfname_hdr);

    // Log message and write second data filename
    if (elem_size == 2 && write_error == 0) switch (hist_mode) {
        case 1: // Prompts and delayed: trues is second if short type
            sprintf(outfname_hdr, "%s.hdr", outfname_tr);
            hdr.WriteTag("!name of data file", outfname_tr);
            hdr.WriteTag("sinogram data type", "true");
            // convert span3 prompt sinogram to span9 and substract delayed
            for (i = 0; i < sino_size; i++) ssino[i] =  ssino[i] - delayed[i];
            if (span == 3) {
                printf ("Converting true sino to span9\n");
                convert_span9(ssino, max_rd, 104);
                hdr.WriteTag("axial compression", 9);
                hdr.WriteTag("matrix size [3]", nsinos[9]);
            }
            printf("Writing Trues Sinogram: %s\n", outfname_tr);
            if (fwrite(ssino, span == 3 ? span9_sino_size : sino_size, elem_size, out3) == elem_size) {
                hdr.WriteFile(outfname_hdr);
            } else {
                printf("Error writing %s\n", outfname_tr);
                write_error++;
            }
            // Restore span3 header
            if (span == 3) {
                hdr.WriteTag("axial compression", 3);
                hdr.WriteTag("matrix size [3]", nsinos[3]);
            }
            break;
        case 7: // Transmission: EC corrected is second if short type
            if (out2 != NULL) {
                printf("Writing Mock Sinogram: %s\n", outfname_mock);
                sprintf(outfname_hdr, "%s.hdr", outfname_mock);
                hdr.WriteTag("!name of data file", outfname_mock);
                if (fwrite(mock, sino_size, elem_size, out2) == elem_size) {
                    hdr.WriteFile(outfname_hdr);
                } else {
                    printf("Error writing %s\n", outfname_mock);
                    write_error++;
                }
                break;
            }
        }

    // Log message and write third data filename
    if (output_ra && write_error == 0 && hist_mode == 1) {
        printf("Writing Randoms Sinogram: %s\n", outfname_ra);
        sprintf(outfname_hdr, "%s.hdr", outfname_ra);
        hdr.WriteTag("!name of data file", outfname_ra);
        hdr.WriteTag("sinogram data type", "delayed");
        count = fwrite(delayed, span9_sino_size, elem_size, out2);
        if (count == (int)elem_size) {
            hdr.WriteFile(outfname_hdr);
        } else {
            printf("Error writing %s\n", outfname_ra);
            write_error++;
        }
    }
    if (write_error) exit(1);
}

/**
 * p39_write_sino:
 * Write sinogram and header to file.
 * Reuse existing listmode header to create sinogram header.
 * Header file name is derived from data file name (data_fname.hdr).
 * Log error and exit when an I/O error is encountered.
 */
static void p39_write_sino(char *sino,  char *b_delayed, int sino_size,
                           CHeader &p39_mhdr, int frame)
{
    // create the output header file
    CHeader hdr;
    char *p = NULL;
    short *ssino = (short *)sino;
    char *delayed = sino + sino_size * elem_size; // b_delayed==NULL
    short *mock = (short *)(sino + sino_size * elem_size);
    char tmp[64];
    int bucket, num_buckets = 10;
    char *data_file = strrchr(outfname_p39_em, '\\');
    if (data_file == NULL) data_file = outfname_p39_em;
    else data_file++;

    hdr.WriteTag("image data byte order", "LITTLEENDIAN");
    hdr.WriteTag("!PET data type", "emission");
    hdr.WriteTag("%CPS data type", "sinogram subheader");
    hdr.WriteTag("!name of data file", data_file);
    hdr.WriteTag("data format", "sinogram");
    hdr.WriteTag("number format", "signed integer");
    hdr.WriteTag("!number of bytes per pixel",  2);
    hdr.WriteTag("number of dimensions", 3);
    hdr.WriteTag("matrix size [1]", 320);
    hdr.WriteTag("matrix size [2]", 256);
    hdr.WriteTag("matrix size [3]", 239);
    hdr.WriteTag("scale factor (mm/pixel) [1]", 2.2);
    hdr.WriteTag("scale factor (degree/pixel) [2]", 1.0);
    hdr.WriteTag("scale factor (mm/pixel) [3]", 2.208);
    hdr.WriteTag("start horizontal bed position (mm)", 0.0);
    hdr.WriteTag("start vertical bed position (mm)", 0.0);
    hdr.WriteTag("number of scan data types", 2);
    hdr.WriteTag("scan data type description [1]", "prompts");
    hdr.WriteTag("data offset in bytes [1]", 0);
    hdr.WriteTag("scan data type description [2]", "randoms");
    hdr.WriteTag("data offset in bytes [2]", 644710400);
    hdr.WriteTag("!image duration (sec)", prev_duration + duration[frame]);
    hdr.WriteTag("image relative start time (sec)", relative_start);
    hdr.WriteTag("%axial compression", span);
    hdr.WriteTag("%maximum ring difference", max_rd);
    hdr.WriteTag("%number of segments", p39_nsegs);
    hdr.WriteTag("%segment table", p39_seg_table);
    int decay_time = relative_start + dose_delay;
    float dcf1 = 1.0f;
    float dcf2 = 1.0f;
    if (decay_rate > 0.0) {
        dcf1 = (float)pow(2.0f, decay_time / decay_rate);
        //      dcf2 = ((float)log(2.0) * duration[frame]) / (decay_rate * (1.0 - (float)pow(2,-1.0*duration[frame]/decay_rate)));
        dcf2 = (float)((log(2.0) * duration[frame]) / (decay_rate * (1.0 - pow(2.0f, -1.0f * duration[frame] / decay_rate))));
    }
    //  hdr.WriteTag("decay correction factor",dcf1);
    //  hdr.WriteTag("decay correction factor2",dcf2);
    //  hdr.WriteTag("frame",frame);
    hdr.WriteTag("total prompts", total_prompts());
    hdr.WriteTag("%total randoms", total_randoms());
    hdr.WriteTag("%Total Net Trues", total_prompts() - total_randoms());
    int tot_singles = 0;
    for (bucket = 0; bucket < num_buckets ; bucket++)
        tot_singles += singles_rate(bucket);
    hdr.WriteTag("%total uncorrected singles", tot_singles);
    hdr.WriteTag("%number of bucket-rings ", num_buckets);
    for (bucket = 0; bucket < num_buckets ; bucket++) {
        sprintf(tmp, "%%bucket-ring singles[%d]", bucket + 1);
        hdr.WriteTag(tmp, singles_rate(bucket));
    }
    //  hdr.WriteTag("average singles per block",tot_singles/num_buckets);
    //  hdr.WriteTag("Dead time correction factor",GetDTC(LLD, tot_singles/num_buckets));

    // Write Prompts and Random sinogram
    int count = elem_size, write_error = 0;
    printf("sinogram size: %d\n", sino_size);
    printf("element size: %d\n", elem_size);
    printf("Writing Prompts Sinogram: %s\n", outfname_p39_em);
    sprintf(outfname_hdr, "%s.hdr", outfname_p39_em);
    if ((count = fwrite(sino, sino_size, elem_size, out)) == (int)elem_size) {
        printf("Converting Random Sinogram from byte to short\n");
        for (int i = 0; i < sino_size; i++) ssino[i] = b_delayed[i];
        printf("Writing Random Sinogram\n");
        count = fwrite(sino, sino_size, elem_size, out);
    }
    if (count == (int)elem_size) hdr.WriteFile(outfname_hdr);
    else printf("Error writing %s\n", outfname_p39_em);
    p39_mhdr.WriteTag("%number of emission data types", 2);
    p39_mhdr.WriteTag("%emission data type description[1]", "prompts");
    p39_mhdr.WriteTag("%emission data type description[2]", "randoms");
    p39_mhdr.WriteTag("%number of transmission data types", 1);
    p39_mhdr.WriteTag("%transmission data type description[1]", "corrected prompts");
    p39_mhdr.WriteTag("%DATA SET DESCRIPTION", "");
    p39_mhdr.WriteTag("!total number of data sets", 2);
    sprintf(tmp, "{0000000000,%s.hdr,%s}", data_file, data_file);
    p39_mhdr.WriteTag("%data set [1]", tmp);
    data_file = strrchr(outfname_p39_tx, '\\');
    if (data_file == NULL) data_file = outfname_p39_tx;
    else data_file++;
    sprintf(tmp, "{1000000000,%s.hdr,%s}", data_file, data_file);
    p39_mhdr.WriteTag("%data set [2]", tmp);
    if (nframes == 1) {
        // one mhdr for EM&TX
        strcpy(fname_p39_mhdr, outfname_sino);
    } else {
        // one mhdr per frame and one mhdr for TX
        strcpy(fname_p39_mhdr, outfname_p39_em);
    }
    char *ext = strrchr(fname_p39_mhdr, '.');
    if (ext != NULL) strcpy(ext, ".mhdr");
    else strcat(fname_p39_mhdr, ".mhdr");
    p39_mhdr.WriteFile(fname_p39_mhdr, 1);
}

inline int extract_date(const char *str, struct tm &tm)
{
    int ret = 0;
    printf("extract date %s\n", str);
    if (sscanf(str, "%d:%d:%d", &tm.tm_mday, &tm.tm_mon, &tm.tm_year) == 3) {
        printf("%d:%d:%d\n", tm.tm_mday, tm.tm_mon, tm.tm_year);
        // "12:00:00 AM" = unknown date
        if (tm.tm_year > 1900) {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            ret = 1;
            tm.tm_isdst = -1; // don't use daylight saving
        }
    }
    return ret;
}

inline int extract_time(const char *str, struct tm &tm)
{
    int ret = 0;
    tm.tm_isdst = -1; // don't use daylight saving
    if (sscanf(str, "%d:%d:%d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3)
        ret = 1;
    return ret;
}

static int get_dose_delay(CHeader &hdr, int &t)
{
    struct tm tm;
    time_t t0, t1;
    char buf[80];
    if (hdr.Readchar("dose assay date (dd:mm:yryr)", buf, sizeof(buf)) != 0 || !extract_date(buf, tm))
        return 0;
    if (hdr.Readchar("dose assay time (hh:mm:ss)", buf, sizeof(buf)) != 0 || !extract_time(buf, tm))
        return 0;
    t0 = mktime(&tm);
    if (hdr.Readchar("study date (dd:mm:yryr)", buf, sizeof(buf)) != 0 || !extract_date(buf, tm))
        return 0;
    if (hdr.Readchar("study time (hh:mm:ss)", buf, sizeof(buf)) != 0 || !extract_time(buf, tm))
        return 0;
    t1 = mktime(&tm);
    if (t1 <= t0) return 0;
    t = (int)(t1 - t0);
    return 1;
}

// Current time formating
inline void format_NN(int n, char *out)
{
    if (n < 10) sprintf(out, "0%d", n);
    else sprintf(out, "%d", n);
}

static const char *my_current_time()
{
    static char time_str[20];
    time_t now;
    time(&now);
    struct tm *ptm = localtime(&now);
    format_NN((1900 + ptm->tm_year) % 100, time_str);
    format_NN(ptm->tm_mon + 1, time_str + 2);
    format_NN(ptm->tm_mday, time_str + 4);
    format_NN(ptm->tm_hour, time_str + 6);
    format_NN(ptm->tm_min, time_str + 8);
    format_NN(ptm->tm_sec, time_str + 10);
    return time_str;
}

void log_message(const char *msg, int type)
{
    FILE *fp = log_fp != NULL ? log_fp : stdout;
    switch (type) {
    case 0:
        fprintf(fp, "%s\tInfo   \t%s\n", my_current_time(), msg);
        break;
    case 1:
        fprintf(fp, "%s\tWarning\t%s\n", my_current_time(), msg);
        break;
    case 2:
        fprintf(fp, "%s\tError  \t%s\n", my_current_time(), msg);
        break;
    }
    if (log_fp != NULL) fflush(log_fp);
}


int main(int argc, char *argv[])
{
    int frame = 0;
    int count = 0, multi_spec[2];
    char *sinogram = NULL, *bsino = NULL, *delayed = NULL;
    short *ssino = NULL;
    int current_time = -1;
    char *cptr = NULL;
    char datatype[32];
    CHeader hdr;
    char msg[FILENAME_MAX];


    sprintf(sw_build_id, "%s %s", __DATE__, __TIME__);
    parse_valid(argc, argv);

    // Create log file

    if (log_fname != NULL) log_fp = fopen(log_fname, "a+"); // create or append to log file

    // set input header filename
    strcpy(in_fname_hdr, in_fname);
    strcat(in_fname_hdr, ".hdr");
    sprintf(msg, "%s %s", argv[0], in_fname);
    log_message(msg);
    if (access(in_fname, 4) != 0) {
        sprintf(msg, "%s: File not found", in_fname);
        log_message(msg, 2);
        return 1;
    }
    if (hdr.OpenFile(in_fname_hdr) == 1) {
        sprintf(msg, "Using header: [%s]", in_fname_hdr);
        log_message(msg);
        hdr.CloseFile();
    } else {
        sprintf(msg, "header [%s] not found", in_fname_hdr);
        log_message(msg);
    }

    int num_cpus = get_num_cpus();
    if (num_cpus > MAX_THREADS)
        num_cpus = MAX_THREADS; // workaround for dual quadcore bug

    //
    // for transmission scans, always override span to 0 (segment 0 only)
    if (!hdr.Readchar("!PET data type", datatype, 32)) {
        cout << "Data Type: '" << datatype << "'" << endl;
				// ahc this was failing because of trailing CR on DOS format hdr file.
				//        if (strcasecmp(datatype, "transmission") == 0) {
				cout << "XXX: " << strstr(datatype, "transmission") << endl;
 				if (strstr(datatype, "transmission")) {
            float axial_velocity = 0.0;
            span = 0;
            hist_mode = 7;
            cout << "Data Type: " << datatype << ", hist_mode: " << hist_mode << endl;
            if (hdr.Readfloat("axial velocity", &axial_velocity) == 0) {
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
    if (duration[0] == 0) {
        // Frame duration not specified at command line; Use header
        char framedef[256];
        strcpy(framedef, "");
        hdr.Readchar("Frame definition", framedef, 256);
        if (strlen(framedef)) {
            nframes = 0;
            cout << "Frame definition: [" << framedef << "]" << endl;
            cptr = strtok(framedef, ", ");
            while (cptr) {
                if (sscanf(cptr, "%d*%d", &multi_spec[0], &multi_spec[1]) == 2) {
                    if (multi_spec[0] > 0) {
                        cout << multi_spec[0] << "sec frame  repeated " << multi_spec[1] << endl;
                        for (count = 0; count < multi_spec[1] && nframes < 256; count++)
                            duration[nframes++] = multi_spec[0];
                    } else {
                        cout << in_fname_hdr << ": Invalid frame specification \"" << cptr << "\"" << endl;
                        break;
                    }
                } else {
                    if (sscanf(cptr, "%d", &count) == 1 && count > 0) duration[nframes++] = count;
                    else {
                        cout << in_fname_hdr << ": Invalid frame specification \"" << cptr << "\"" << endl;
                        break;
                    }
                }
                cptr = strtok(0, ", ");
            }
            cout << "number of frames: " << nframes << endl;
            // if something went wrong during parsing, set proper defaults
            if (nframes == 0) {
                nframes++;
                duration[0] = 0;
            }
        } else
            cout << "Using commandline frame definition" << endl;
    }

    if (scan) {
        // transmission tag processing uses parameters that need to be initialized
        // reset frame duration otherwise lm_reader splits data into frames and waits
        // for the frame to be written.
        strcpy(outfname_hc, in_fname);
        // Replace extension by "_lm.hc"
        char *ext = strrchr(outfname_hc, '.');
        if (ext != NULL)
            strcpy(ext, "_lm.hc");
        else
            strcat(outfname_hc, "_lm.hc");
        FILE *out_hc = fopen(outfname_hc, "w");
        if (out_hc == NULL) {
            perror(outfname_hc);
            exit(1);
        }
        memset(duration, 0, sizeof(duration));
        start_reader_thread();
        lmscan(out_hc, &scan_duration);
        fclose(out_hc);

        //      for (int index = 0; index < duration; index++)
        //          cout << hc[index].time << " " << hc[index].randoms_rate << " " << hc[index].trues_rate << " " << hc[index].singles << endl;

        cout << "Scan Duration = " << scan_duration << endl;
    } else {
        int axial_comp;
        float isotope_halflife = 0.0;
        // set output filename if an input has not been given
        if (strlen(out_fname) == 0) {
            strcpy(out_fname, in_fname);
            if (cptr = strrchr(out_fname, '.'))
                * cptr = '\0';
            strcat(out_fname, ".s");
        }

        if (hdr.Readint("lower level", &LLD))
            LLD = DEFAULT_LLD;
        cout << "LLD: " << LLD << endl;

        // Build the headers
        int sino_mode = 0;  // by default use net trues
        if (!hdr.Readint("Sinogram Mode", &sino_mode)) {
            cout << "Using Sinogram Mode from header: " << sino_mode << endl;
            if (sino_mode != 0 && prev_sino != NULL) {
                cout << "Adding to existing sinogram only supported in Trues mode"  << endl;
                exit(1);
            }
            hist_mode = sino_mode;
        }

        hdr.Readfloat("isotope halflife", &isotope_halflife);
        if (isotope_halflife != 0.0)
            decay_rate = (float)isotope_halflife;

        get_dose_delay(hdr, dose_delay);
        if (!hdr.Readint("axial compression", &axial_comp)) {
            if (!span_override && axial_comp > 0)
                span = axial_comp;
            else
                hdr.WriteTag("axial compression", span);
        }

        // update the header with sinogram info for HRRT
        // BUT not for P39 where the header contains the main header information
        if (check_model(model_number, MODEL_HRRT)) {
            hdr.WriteTag("!originating system"      , "HRRT");
            hdr.WriteTag("!name of data file"       , outfname_sino);
            hdr.WriteTag("number of dimensions"     , "3");
            hdr.WriteTag("matrix size [1]"          , num_projs(LR_type));
            hdr.WriteTag("matrix size [2]"          , num_views(LR_type));
            hdr.WriteTag("matrix size [3]"          , LR_type == LR_0 ? nsinos[span] : LR_nsinos[span]);
            hdr.WriteTag("data format"              , "sinogram");
            hdr.WriteTag("number format"            , "signed integer");
            hdr.WriteTag("number of bytes per pixel", (int)elem_size);
            hdr.WriteTag("!lmhistogram version"     , sw_version);
            hdr.WriteTag("!lmhistogram build ID"    , sw_build_id);
            hdr.WriteTag("!histogrammer revision"   , "2.0");
        }

        if (l64_flag && check_model(model_number, MODEL_HRRT)) {
            int span_bak = span, rd_bak = max_rd;
            // ahc: lut_file now passed to init_rebinner instead of finding it there.
            // init_rebinner(span, max_rd);
            init_rebinner(span, max_rd, lut_file);
            sprintf(msg, "using rebinner LUT %s", rebinner_lut_file);
            log_message(msg);

            hdr.WriteTag("!LM rebinner method", (int)rebinner_method);
            hdr.WriteTag("axial compression", span);
            hdr.WriteTag("maximum ring difference", max_rd);
            span = span_bak; max_rd = rd_bak;
            hdr.WriteTag("scaling factor (mm/pixel) [1]", 10.0f * m_binsize); // cm to mm
            hdr.WriteTag("scaling factor [2]", 1); // angle
            hdr.WriteTag("scaling factor (mm/pixel) [3]", 10.0f * m_plane_sep); // cm to mm
        }

        cout << "Span: " << span << endl;

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
            int skip_msec = find_start_countrate(in_fname);
            if (skip_msec > 0) {
                skip[0] = skip_msec / 1000; //msec
                sprintf(msg, "%d sec will be skipped to start with %d trues/sec countrate",
                        skip[0], start_countrate);
                log_message(msg);
            }
        }

        //start reader and rebinner threads
        start_reader_thread();
        if (l64_flag)   start_rebinner_thread();        // start lm64_rebinner thread  and wait until ready

        // process each frame
        frame = 0;
        while (frame < nframes) {
            //
            // output histogram is filename_frameX.s
            // output histogram header is filename_frameX.s.hdr
            //
            if (nframes > 1) {
                sprintf(outfname_sino, "%s_frame%d.s", out_fname, frame);
                FILE *fp = fopen(outfname_dyn, "a");
                if (fp) {
                    if (check_model(model_number, MODEL_HRRT)) {
                        // HRRT
                        fprintf(fp, "%s_frame%d.s\n", out_fname_stripped, frame);
                    } else {
                        //P39
                        fprintf(fp, "%s_frame%d.mhdr\n", out_fname_stripped, frame);
                    }
                    fclose(fp);
                }
            } else
                sprintf(outfname_sino, "%s.s", out_fname);

            // Create output files; exit if fail
            if (check_model(model_number, MODEL_HRRT)) create_files();
            else p39_create_files();

            if (elem_size == 2) {
                if (!init_sino<short>(ssino, delayed, span, prev_sino, prev_duration)) exit(1);
                sinogram = (char *)ssino;
            } else {
                if (!init_sino<char>(bsino, delayed, span, prev_sino, prev_duration)) exit(1);
                sinogram = bsino;
            }


            // Position input stream to time 0 or requested time
            cout << "Histogramming Frame " << frame << " duration=" << duration[frame] << " seconds " << endl;
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
                relative_start = histogram<short>(ssino, delayed, sinogram_size, duration[frame], out_hc);
            } else {
                relative_start = histogram<char>(bsino, delayed, sinogram_size, duration[frame], out_hc);
            }

            if (relative_start < 0) {
                // exit if error encountered, error message logged by histogram routine
                exit(1);
            }

            if (duration[frame] <= 0) {
                // should not happen
                printf("Invalid frame duration %d\n", duration[frame]);
                exit(1);
            }

            // update current time
            current_time = relative_start + duration[frame];
            scan_duration += duration[frame];

            // Write sinogram and header files
            if (check_model(model_number, MODEL_HRRT))
                write_sino(sinogram, sinogram_size, hdr, frame);
            else
                p39_write_sino(sinogram, delayed, sinogram_size, hdr, frame);
            close_files();
            if (l64_flag && hist_mode != 7)
                write_coin_map(outfname_sino);
            if (nframes > 1)
                sprintf(msg, "Frame time histogrammed = %d sec", duration[frame]);
            else
                sprintf(msg, "Time histogrammed = %d sec", duration[frame]);
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
