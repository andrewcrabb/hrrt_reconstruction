// histogram.cpp
/*
------------------------------------------------

     Component       : HRRT

     Authors         : Mark Lenox, Merence Sibomana
     Language        : C++
     Platform        : Microsoft Windows.

     Creation Date   : 04/02/2003
     Modification history:
                                        02/09/2005 : Remove time_sequence constraint
                                        05/17/2005 : Add reset_coinc_map function to reset the coincidence map counters.
                                        11/30/2005 : Add HRRT low Resolution and P39 support
                                                                 Add nodoi support
                                        02/09/2006:  Exclude border crystals by default and provide an option to keep them (-ib)
                                        03/13/2006: In span3, use span9 for delayed
                                        03/13/2006: Bug fix in rebin_packet: use signed int for crystal position.
                                                                Comparing unsigned position to signed fan limits give unexpected results.
                                        04/12/2006: Bug fix in lmscan_64
                                                                Missing Head line restored in lmscan_64
                                        04/18/2006: Changes WRT code review 04/13/06
                                                                Restore motion correction test in process_tagword
                                                                use "unsigned int ax,ay,bx,by" instead of "int" in rebin_packet
                                        04/26/2006: keep tx_source.z_low and tx_mock.z_low positive to allow comparison with unsigned.
                                        10/28/2007: Added check_start_of_frame
                                        01/09/2009: Display seconds in check_start_of_frame() skipping mode
                                            Replace check_start_countrate() by
                                            find_start_countrate() to locate histogramming start time
                    20-MAY-2009: Use a single fast LUT rebinner
*/
#include <cstdint>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "histogram_mp.hpp"
#include "LM_Reader_mp.hpp"
#include "LM_Rebinner_mp.hpp"

#include <gen_delays_lib/gen_delays.h>
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
// Already included by spdlog
// #include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/xpressive/xpressive.hpp>
#include <boost/lexical_cast.hpp>

// #if !defined(FMT_HEADER_ONLY)
// #define FMT_HEADER_ONLY
// #endif


#define SPDLOG_FMT_EXTERNAL
#include "spdlog/spdlog.h"
// #include "spdlog.h"

namespace bf = boost::filesystem;
namespace bx = boost::xpressive;

typedef struct {
  int64_t total_singles;
  long average_rate;
  int nsamples;
  long last_sample;
} singles_record;


typedef struct  _Event_32 {
  enum {TAG, PROMPT, DELAYED, UNKNOWN} type;
  long  value;
} Event_32; // coinc_event_word;

struct TX_SourcePosition {
  int timer;
  short valid, head, y, x;
  short z_low, z_high;
};


//
// Global variables
//
// extern auto g_logger;
extern std::shared_ptr<spdlog::logger> g_logger;

// Defined extern
unsigned int start_countrate_ = 0;

int quiet = 0;
HIST_MODE g_hist_mode = HM_TRU;              // 0=Trues (Default), 1=Prompts and Randoms, 2=Prompts only, 7=transmission
int timetag_processing = 1;     // 0=Use timetag count for time, 1=decode time from timetag event
unsigned eg_rebinner_method = SW_REBINNER;
int g_max_rd = 67;
int stop_count_ = 0;

int frame_start_time = -1;      //First time extracted from time tag in sec
int frame_duration = -1;        //First time extracted from time tag in sec

// crystal singles counters for prompts and delayed
// different array for each rebinner
unsigned *g_p_coinc_map = nullptr;
unsigned *g_d_coinc_map = nullptr;
//
// Local counters for 1 sec period
//
static long prompts = 0;        // In FOV prompts event counter
static long randoms = 0;        // In FOV randoms event counter

//
// Global counters for 1 frame period
//
static int64_t event_counter = 0;   // Events (prompts and randoms) counter
static int64_t tag_counter = 0;     // Tags counter
static int64_t t_prompts = 0;       // In FOV prompts event counter
static int64_t t_randoms = 0;       // In FOV randoms event counter
static int64_t tx_prompts = 0;      // In FOV TX prompts event counter
static int64_t tx_randoms = 0;      // In FOV TX randoms event counter
static singles_record singles[NBLOCKS]; // Block singles

//
// Timers
//
static long current_time = -1;          //Current time extracted from time tag in sec
static long current_time_msec = -1;
//
// Shifted Mock parameters
//
float g_tx_source_speed = 12.5; // msec
static TX_SourcePosition tx_source, tx_mock;


//
// Input Buffers and counters
//
static unsigned *listbuf = 0;       // Input buffer
static unsigned *listptr = 0;       // Input buffer position
static int nevents = 0, nsync = 0;  // # of events and syncs in current buffer

//
// Status Flags
//
static long terminate_frame = 0;        // End frame flag set by time tag process
static int clock_reset = 0;


int open_ostream(std::ofstream &t_outs, const bf::path &t_path, std::ios_base::openmode t_mode) {
  // boost::filesystem::ofstream outs();
  t_outs.open(t_path.string(), t_mode);
  if (!t_outs.is_open()) {
    g_logger->error("Could not open output file {}", t_path);
    return 1;
  }
  return 0;
}

int open_istream(std::ifstream &t_ins, const bf::path &t_path, std::ios_base::openmode t_mode) {
  t_ins.open(t_path.string(), t_mode);
  if (!t_ins.is_open()) {
    g_logger->error("Could not open input file {}", t_path);
    return 1;
  }
  return 0;
}


//
// Counters access functions
//
int singles_rate(int block) {
  return (singles[block].average_rate);
}
int64_t total_prompts() {
  return t_prompts;
}

int64_t total_randoms() {
  return t_randoms;
}
int64_t total_tx_prompts() {
  return tx_prompts;
}

int64_t total_tx_randoms() {
  return tx_randoms;
}

void reset_statistics() {
  event_counter = 0;
  tag_counter = 0;
  randoms = prompts = 0;
  t_prompts = t_randoms = 0;

  for (int i = 0; i < NBLOCKS; i++) {
    singles[i].total_singles = 0;
    singles[i].average_rate = 0;
    singles[i].nsamples = 0;
    singles[i].last_sample = 0;
  }
  if (current_time >= 0)
    frame_start_time = current_time;
  else
    frame_start_time = -1;
  terminate_frame = 0;
}

/**
 * process_tagword:
 * Output time, prompts, randoms, singles counters to out_hc if not nullptr.
 * Sets terminate_frame flag if duration>0 and  (time-frame_start_time)>=duration.
 * Only consistent time values are used: (consecutives t0,t1,t2 are consistent if t2=t1+1 and t1=t0+1).
 * Returns current time (>=0) or error (-1)
 */
long process_tagword(long tagword, long duration, std::ofstream &out_hc)
{
  static int prev_time = 0;
  if ((tagword & 0xE0000000) == 0x80000000) { // timetag found
    if (timetag_processing) {
      current_time_msec = tagword & 0x3fffffff;
    } else {
      current_time_msec++;
    }
    if (prev_time > current_time_msec) {
      if (!clock_reset) {
        g_logger->info("Time line break: previous time={}, current time={}", prev_time, current_time_msec);
        current_time = current_time_msec / 1000;
        clock_reset = 1;
        return -1; // reset all
      }
    } else if (prev_time > 0 && current_time_msec > prev_time + 1) {
      g_logger->error("Missing timetag : {}-{} = {} msec",
              current_time_msec - 1, prev_time, current_time_msec - prev_time - 1);
    }
    prev_time = current_time_msec;
    if (current_time != current_time_msec / 1000) {
      long total_singles = 0;
      current_time = current_time_msec / 1000;
      for (int i = 0; i < NBLOCKS; i++) {
        total_singles += singles[i].last_sample;
      }

      g_logger->info("{} {} {} {} {} ", current_time, randoms, prompts, total_singles, current_time_msec);
      // fmt::print(out_hc, "%ld,%ld,%ld,%ld\n", total_singles, randoms, prompts, current_time_msec);
      fmt::print(out_hc, "{},{},{},{}", total_singles, randoms, prompts, current_time_msec);
      t_prompts += prompts;
      t_randoms += randoms;
      prompts = randoms = 0;
      if (frame_start_time < 0)
        frame_start_time = current_time;
      if (duration > 0 && ((current_time - frame_start_time ) >=  duration))
        terminate_frame = 1;
    }
  } else if ((tagword & 0xE0000000) == 0xA0000000) { // block singles
    int block = (tagword & 0x1ff80000) >> 19;
    int bsingles = tagword & 0x0007ffff;
    //cout << "block: " << block << "  singles: " << bsingles << endl;
    if (block >= 0 && block < NBLOCKS) {
      singles[block].last_sample = bsingles;
      singles[block].total_singles += bsingles;
      singles[block].nsamples++;
      singles[block].average_rate = (long)(singles[block].total_singles / singles[block].nsamples);
    }
  } else if ((tagword & 0xF8000000) == 0xE8000000) { // Patient Motion
    // Keep this for motion correction to be implemented in the future
    // int tool = (tagword & 0x07000000) >> 24;
    // int direction = (tagword & 0x00E00000) >> 21;
    // int value = (tagword & 0x001fffff);
  }

  else if ((tagword & 0xE0000000) == 0xc0000000) { // Gantry Motions & positions
    // int head = (tagword & 0x000f0000) >> 16;
    // int tx_y = (tagword & 0x0000ff00) >> 8;
    // int tx_x = tagword & 0x000000ff;
    //  cout << "tx pos: " << tx_source.head << "," << tx_source.x << "," << tx_source.y << ": " << tx_source.timer << "msec" << endl;
    tx_source.head = (short)((tagword & 0x000f0000) >> 16);
    tx_source.y = (short)((tagword & 0x0000ff00) >> 8);
    tx_source.x = (short)(tagword & 0x000000ff);
    tx_source.z_low = tx_source.y - (tx_span / 2);
    tx_source.z_high = tx_source.y + (tx_span / 2);
    tx_source.timer = 0;
    tx_mock.head = tx_source.head;
    tx_mock.x = tx_source.x;
    tx_mock.y = (tx_source.y + GeometryInfo::NYCRYS / 2) % GeometryInfo::NYCRYS;
    tx_mock.z_low = tx_mock.y - (tx_span / 2);
    tx_mock.z_high = tx_mock.y + (tx_span / 2);
    // keep tx_low positive to allow comparison with unsigned
    if (tx_source.z_low < 0)
      tx_source.z_low = 0;
    if (tx_mock.z_low < 0)
      tx_mock.z_low = 0;
  }
  return current_time > 0 ? current_time : 0;
}



inline void load_buffer_32(unsigned *&buf, int &nevents)
{
  static L32EventPacket *current = nullptr;
  if (current != nullptr) {
    current->status = L32EventPacket::empty;
    current->num_events = 0;
  }
  if ((current = load_buffer_32()) != nullptr) {
    buf = current->events;
    nevents = current->num_events;
  } else {
    buf = nullptr;
    nevents = 0;
  }
  nsync = 0;
}

/**
 * next_event_32:
 * Reads the next buffer if the current one is empty
 * Builds an Event_32 structure from the next 32-bit event in the buffer.
 * Returns 0 if Success, 1 if end_of_frame and -error_code otherwise.
 * Error codes :
 * 1="memory allocation error".
 * 2="end of file"
 * 3="I/O error"
 */
static int next_event_32(Event_32 &cew, int scan_flag) {
  unsigned event, tag;

  if (terminate_frame)
    return 1;

  if (!nevents) {
    load_buffer_32(listbuf, nevents);
    if (nevents <= 0)  
      return -2;
    listptr = listbuf;
  }

  // get the event out of the buffer
  event = *listptr;

  tag = (event & 0x80000000) >> 31;

  if (tag) {
    cew.type = Event_32::TAG;
    cew.value = event;
    tag_counter++;
  } else {
    cew.type = Event_32::DELAYED;
    if ((event & 0x40000000) >> 30) {
      cew.type = Event_32::PROMPT;
      prompts++;
      event_counter++;
    } else {
      randoms++;
      event_counter--;
    }
    //     cew.value = event & 0x1fffffff; // allow 30 bits addresses for span 1
    cew.value = event & 0x3fffffff;
  }

  listptr++;
  nevents--;

  return 0;
}

/**
 * goto_event_32:
 * Reads the next buffer if the current one is empty
 * Builds an Event_32 structure from the next 32-bit event in the buffer.
 * Decode the time value if the event is a timetag.
 * Returns 1 (OK)
 * Logs an error and retun 0 otherwise.
 */
static int goto_event_32(int target)
{
  unsigned event, tag;
  int tmp_time = 0;
  // int prev_time;
  int terminate = 0;

  g_logger->info("Skipping to {} secs (goto_event_32, nevent {})", target, nevents);
  while (!terminate) {
    if (!nevents) {
      g_logger->info("load buffer");
      load_buffer_32(listbuf, nevents);
      g_logger->info("nevents now {}", nevents);
      if (nevents <= 0) {
        g_logger->info("end of file");
        return 0;
      }
      listptr = listbuf;
    }

    event = *listptr;
    tag = (event & 0x80000000);
    if (tag) {
      if ((event & 0xE0000000) == 0x80000000) {
        tmp_time = event & 0x3fffffff;
        // prev_time = tmp_time;
        if (tmp_time == 0 && target == 0) {
          // goto time 0
          current_time = 0;
          terminate = 1;
        } else if (current_time != tmp_time / 1000 ) {
          // goto time t
          current_time = tmp_time / 1000;
          g_logger->info("{} sec", current_time);
          if (current_time == target)
            terminate = 1;
        }
      }
    }
    listptr++;
    nevents--;
  }
  g_logger->info("done");
  return 1;
}


static unsigned int ewtypes[16] = {3, 3, 1, 0, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};

inline void load_buffer_64(unsigned *&buf, int &nevents, int &nsync)
{
  /*
          DWORD bytes_to_read=2*sizeof(unsigned)*MAX_EVENTS, bytes_read=0;
          ReadFile(in->handle, buf, bytes_to_read, &bytes_read, nullptr);
          nevents = bytes_read/(2*sizeof(unsigned));
          nsync = 0;
  */
  static L64EventPacket *current = nullptr;
  if (current != nullptr) current->status = L64EventPacket::empty;
  if ((current = load_buffer_64()) != nullptr) {
    buf = current->events;
    nevents = current->num_events;
  } else {
    buf = nullptr;
    nevents = 0;
  }
  nsync = 0;
}

/**
 * goto_event_64:
 * Reads the next buffer if the current one is empty
 * Builds an Event_32 structure from the next 64-bit event in the buffer.
 * Decode the time value if the event is a timetag.
 * Returns 1 (OK)
 * Logs an error and retun 0 otherwise.
 */
// Commented out as unused 1/1/19 ahc
/*
static int goto_event_64(int target)
{
  unsigned int ew1, ew2, type, tag;
  int tmp_time = 0;
  // int prev_time = 0;
  // int64_t tot_events = 0;
  int terminate = 0;

  g_logger->info("Skipping To {} secs (goto_event_64)", target);
  while (!terminate) {
    g_logger->info("nevents {}", nevents);
    if ((nevents - (nsync + 1) / 2) == 0) {
      load_buffer_64(listbuf, nevents, nsync);
      if (nevents <= 0) {
        g_logger->info("end of file");
        return 0;
      }
      listptr = listbuf;
    }
    ew1 = listptr[0];
    ew2 = listptr[1];
    type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
    while (type == 3) {
      // sync
      nsync++;
      listptr++;
      ew1 = listptr[0];
      ew2 = listptr[1];
      type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
      if ((nevents - (nsync + 1) / 2) == 0) {
        load_buffer_64(listbuf, nevents, nsync);
        if (!nevents) terminate = 2;
        listptr = listbuf;
      }
    }
    if (type == 2) { // tag word
      tag = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
      if ((tag & 0xE0000000) == 0x80000000) {
        tmp_time = tag & 0x3fffffff;
        // prev_time = tmp_time;
        if (tmp_time == 0 && target == 0) {
          // goto time 0
          current_time = 0;
          terminate = 1;
        } else if (current_time != tmp_time / 1000) {
          // goto to time t;
          current_time = tmp_time / 1000;
          g_logger->info("{} sec", current_time);
          if (current_time == target) terminate = 1;
        }
      }
    }
    listptr += 2;
    nevents--;
  }
  g_logger->info("done");
  return 1;
}
*/

/**
 * next_event_64:
 * Reads the next buffer if the current one is empty
 * Builds an Event_32 structure from the next 64-bit event in the buffer.
 * Returns 0 if Success, 1 if end_of_frame and -error_code otherwise.
 * Error codes :
 * 1="memory allocation error".
 * 2="end of file"
 * 3="I/O error"
 */
static int next_event_64(Event_32 &cew, int scan_flag)
{
  unsigned int ew1, ew2, type;
  int ax, ay, bx, by;
  int doia = 1, doib = 1;
  int mpe, error_flag = 0;
  if (terminate_frame)
    return 3;

  if ((nevents - (nsync + 1) / 2) == 0) {
    load_buffer_64(listbuf, nevents, nsync);
    if (!nevents) return 2;
    listptr = listbuf;
  }
  ew1 = listptr[0];
  ew2 = listptr[1];
  type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
  unsigned doi_processing = (eg_rebinner_method & NODOI_PROCESSING) == 0 ? 1 : 0;
  while (type == 3) {
    // sync
    nsync++;
    listptr++;
    ew1 = listptr[0];
    ew2 = listptr[1];
    type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
    if ((nevents - (nsync + 1) / 2) == 0) {
      load_buffer_64(listbuf, nevents, nsync);
      if (!nevents) return 2;
      listptr = listbuf;
    }
  }
  if (type == 2) { // tag word
    cew.type = Event_32::TAG;
    cew.value = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
    tag_counter++;
  } else {
    // 0=PROMPT, 1=DELAYED
    cew.type = type == 0 ? Event_32::PROMPT : Event_32::DELAYED;
    if (!scan_flag) {
      mpe = ((ew1 & 0x00070000) >> 16) | ((ew2 & 0x00070000) >> 13);
      ax = (ew1 & 0xff);
      ay = ((ew1 & 0xff00) >> 8);
      bx = (ew2 & 0xff);
      by = ((ew2 & 0xff00) >> 8);
      if (doi_processing) {
        doia = (ew1 & 0x01C00000) >> 22;
        doib = (ew2 & 0x01C00000) >> 22;
      }
      if (mpe < 1 || mpe > nmpairs) {
        //        if (verbose && (! error_flag % 10) )
        //    cerr << current_time << ": Invalid head pair " << mpe << endl;
        error_flag++;
      }
      if (ax < 0 || ax >= GeometryInfo::NXCRYS || ay < 0 || ay >= GeometryInfo::NYCRYS) {
        g_logger->error("{}: next_event_64: Invalid crystal pair A: ({},{}) (NXCRYS {}, GeometryInfo::NYCRYS {})", current_time, ax, ay, GeometryInfo::NXCRYS, GeometryInfo::NYCRYS);
        error_flag++;
      }
      if (bx < 0 || bx >= GeometryInfo::NXCRYS || by < 0 || by >= GeometryInfo::NYCRYS) {
        g_logger->error("{}: next_event_64: Invalid crystal pair B: ({},{}) (NXCRYS {}, GeometryInfo::NYCRYS {})", current_time, ax, ay, GeometryInfo::NXCRYS, GeometryInfo::NYCRYS);
        error_flag++;
      }
      if ((doia != 1 && doia != 0 && doia != 7) || (doib != 1 && doib != 0 && doib != 7)) {
        g_logger->error("{}: Invalid interaction layer ({},{}) ", current_time, doia, doib);
        error_flag++;
      }
      if (!error_flag) {
        if (scan_flag) cew.value = 0;
        else cew.value = rebin_event( mpe, doia, ax, ay, doib, bx, by/*, cew.type*/);
        if (cew.value >= 0) {
          if (cew.type == Event_32::PROMPT) {
            prompts++;
            event_counter++;
          } else {
            randoms++;
            event_counter--;
          }
        }
      }
    }
  }
  listptr += 2;
  nevents--;
  return 0;
}

/**
 * goto_event:
 * Calls got_event_64 when l64_flag is set and  goto_event_32 otherwise
*/
int goto_event(int target)
{
  if (!timetag_processing && target == 0) return 1;
  return goto_event_32(target);
}

/*
static double total(unsigned *v, int count)
{
        double sum = 0;
        for (int i=0; i<count; i++) sum += v[i];
        return sum;
}
*/

void reset_coin_map()
{
  unsigned ncrystals = GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS;
  memset(g_p_coinc_map, 0, ncrystals * sizeof(unsigned));
  memset(g_d_coinc_map, 0, ncrystals * sizeof(unsigned));
}

/**
 *  void write_coin_map(const char *datafile)
 *  Derive output file from sinogram file
 *  Sum all threads ch counters and output the sum
 */
int write_coin_map(const bf::path &datafile) {
  // int x = 0, y = 0, h = 0, b = 0, l = 0;
  // char out_file[FILENAME_MAX];
  // unsigned i = 0;
  unsigned ncrystals = GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS;
  // int rebinner_ID = 0;
  unsigned *coinh_p = g_p_coinc_map, *coinh_d = g_d_coinc_map;

  // Create coincidence histogram file with extension ".ch"
  bf::path ch_file_name = datafile;
  ch_file_name.replace_extension("ch");
  std::ofstream ch_file;
  if (open_ostream(ch_file, ch_file_name, std::ios::out | std::ios::app | std::ios::binary))
      return 1;

  int error_flag = 0;
  const char *coinh_p_c = reinterpret_cast<char *>(coinh_p);
  ch_file.write(coinh_p_c, ncrystals * sizeof(unsigned));
  if (ch_file.good()) {
    g_logger->info("Writing coincidence histogram prompts {}", ch_file_name);
    ch_file.write(reinterpret_cast<char *>(coinh_d), ncrystals * sizeof(unsigned));
    if (ch_file.good()) {
      g_logger->info("Writing coincidence histogram delayed {}: OK", ch_file_name);
    } else {
      g_logger->error("Writing coincidence histogram delayed {}: ERROR", ch_file_name);
      error_flag++;
    }
  } else {
    g_logger->error("ERROR writing ch file {}", ch_file_name);
      error_flag++;
  }
  ch_file.close();

  // cout << "Total Prompt CH " << total(coinh_p, ncrystals/2) << ", " << total(coinh_p+(ncrystals/2), ncrystals/2) << endl;
  // cout << "Total Delayed CH " << total(coinh_d, ncrystals/2) << ", " << total(coinh_d+(ncrystals/2), ncrystals/2) << endl;
  // ahc 8/23/18
  // create_sng code body was here, but not called from anywhere.

  reset_coin_map();
  return error_flag;
}

/*
ahc 8/23/18
int create_sng() {
    // Create crystal singles file with extension ".sng"
    //  Write  crystal singlers
    strcpy(out_file, datafile);
    if ((ext = strrchr(out_file, '.')) != nullptr) strcpy(ext, ".sng");
    else strcat(out_file, ".sng");
    if ((fp == fopen(out_file, RW_MODE)) == nullptr) {
        perror(out_file);
        exit(1);
    }
    rebinner_ID = 0;
    int head_nblks = (nxcrys / BLK_SIZE) * (nycrys / BLK_SIZE); // 9*13=117
    float *c_singles = (float *)calloc(ncrystals, sizeof(float));
    float *blk_sum = (float *)calloc(nheads * head_nblks, sizeof(float));
    if (c_singles == nullptr || blk_sum == nullptr) {
        cerr << "Memory allocation error creating crystal singles\n";
        exit(1);
    }

    //
    // Sum singles from delayed at block level
    for (x = 0; x < nxcrys; x++) {
        for (y = 0; y < nycrys; y++) {
            for (h = 0; h < nheads; h++) {
                b = head_nblks * h + (y / BLK_SIZE) * (nxcrys / BLK_SIZE) + (x / BLK_SIZE);
                for (l = 0; l < nlayers; l++) {
                    i = x + nxcrys * h + nxcrys * BLK_SIZE * y + nxcrys * BLK_SIZE * nycrys * l;
                    c_singles[i] = (float) coinh_d[i];   // crystal weight
                    blk_sum[b] += c_singles[i];          // block weight
                }
            }
        }
    }

    // calculate crystal singles from block singles using weights from
    // delayed coincidence histogram
    for (x = 0; x < nxcrys; x++) {
        for (y = 0; y < nycrys; y++) {
            for (h = 0; h < nheads; h++) {
                b = head_nblks * h + (y / BLK_SIZE) * (nxcrys / BLK_SIZE) + (x / BLK_SIZE);
                for (l = 0; l < nlayers; l++) {
                    i = x + nxcrys * h + nxcrys * BLK_SIZE * y + nxcrys * BLK_SIZE * nycrys * l;
                    float weight = c_singles[i] / blk_sum[b];
                    c_singles[i] = weight * singles_rate(b);
                }
            }
        }
    }
    free(blk_sum);

    // Write normalized crystal singles
    fwrite( c_singles, ncrystals, sizeof(float), fp);
    fclose( fp);
    g_logger->info("Crystal Singles stored in '%s'\n", out_file);
    free(c_singles);
}
*/

/*
 * int check_start_of_frame(L64EventPacket &packet)
 * Locate start of frame and stop producing packets until all packets
 * are sorted and CH files are writen
 */

int check_start_of_frame(L64EventPacket &src)
{
  unsigned int ew1, ew2, type, tag;
  const unsigned *in_buf = src.events;
  unsigned pos = 0, first_tag_pos = 0;
  while (pos < src.num_events * 2) {
    ew1 = in_buf[pos];
    ew2 = in_buf[pos + 1];
    type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
    if (type == 2) {
      // tag word
      tag = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
      if ((tag & 0xE0000000) == 0x80000000) {
        int current_time = ((tag & 0x3fffffff)); //msec
        if (current_time / 1000 == frame_start_time) {
          if (first_tag_pos > 0)   return first_tag_pos / 2;
          L64EventPacket::current_time = current_time; // msec
          return 0;
        } else {
          // skipping mode
          first_tag_pos = pos;
          if (L64EventPacket::current_time / 1000 != current_time / 1000) {
            g_logger->info("{} sec", current_time / 1000);
            fflush(stdout);
          }
          L64EventPacket::current_time = current_time; // msec;
        }
      }
    }
    pos += 2;
  }
  return src.num_events;
}

/*
 * int find_start_countrate_lm(const char *fname)
 * Locate starting countrate using listmode
 * Return time in msec
 */

static int find_start_countrate_lm(const bf::path &l64_file) {
  unsigned int ew1, ew2, type, tag;
  unsigned file_pos = 0, pos = 0, first_tag_pos = 0;
  unsigned current_countrate = 0, delayed = 0, prompt = 0;
  int current_time = L64EventPacket::current_time;
  // unsigned *buf = nullptr;
  unsigned num_events = 0;
  constexpr unsigned buf_size = 1024 * 1024;
  // FILE *fp = nullptr;

  // if ((fp = fopen(fname, "rb")) == nullptr) {
  //   perror(fname);
  //   exit(1);
  // }
  std::ifstream instream;
  open_istream(instream, l64_file, std::ios::in | std::ios::binary);
  // buf = (unsigned *)calloc(buf_size, 2 * sizeof(unsigned));

  char buf[buf_size * 2 * sizeof(unsigned)];
  // if (buf == nullptr) {
  //   g_logger->error("Error allocating {} bytes memory", buf_size * 2 * sizeof(unsigned));
  //   exit(1);
  // }
  g_logger->info("Locating starting countrate {} trues/sec", start_countrate_);
  while (current_countrate < start_countrate_) {
    file_pos += pos;
    pos = 0;
    // if ((num_events = fread(buf, 2 * sizeof(unsigned), buf_size, fp)) == 0) {
    instream.read(buf, 2 * sizeof(unsigned) * buf_size);
    if (instream.fail()) {
      g_logger->error("Error reading file {}", l64_file);
      exit(1);
    }
    while (pos < num_events * 2) {
      ew1 = buf[pos];
      ew2 = buf[pos + 1];
      type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
      switch (type) {
      case 3:   // sync
        pos += 1;
        continue;
      case 2:   // tag word
        tag = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
        if ((tag & 0xE0000000) == 0x80000000) {
          // time tag
          current_time = ((tag & 0x3fffffff)); //msec
          if (current_time / 1000 != L64EventPacket::current_time / 1000) {
            if (current_countrate >= start_countrate_) { // start countrate reached
              // fclose(fp);
              // free(buf);
              instream.close();
              return L64EventPacket::current_time; //first_tag_pos;
            } else { //reset counter
              g_logger->info("xxx {} {} {} {} {}", current_time, prompt, delayed, current_countrate, first_tag_pos);
              L64EventPacket::current_time = current_time;   //msec
              current_countrate = 0;
              delayed = prompt = 0;
              first_tag_pos = file_pos + pos * sizeof(unsigned);
            }
          }
        }
        break;
      case 1:     //delayed
        current_countrate--;
        delayed++;
        break;
      case 0:     //prompt
        current_countrate++;
        prompt++;
        break;
      }
      pos += 2;
    }
  }
  instream.close();
  // fclose(fp);
  // free(buf);
  g_logger->info("curr {} prompt {} delay {} countr {} first_pos {}", current_time, prompt, delayed, current_countrate, first_tag_pos);
  return L64EventPacket::current_time; //first_tag_pos;
}


// Return submitted path with extension given by file_type

bf::path &make_path(const bf::path &infile, FILE_TYPE file_type) {
  // Range checking is handled by enum.
  bf::path new_path = infile;
  return new_path.replace_extension(FILE_EXTENSIONS[file_type]);
}

  // Parse line of HC file with contents:
  // Singles,Randoms,Prompts,Time(ms)
  // 4742713,41128,161148,1676
  // Return vector<int> indexec by HC_FILE_COLUMNS enum

// int read_hc_line(const std::ifstream &instream) {
// }

/*
 * int find_start_countrate(const char *fname)
 * Locate starting countrate using hc file or listmode
 * Return time in msec
 * NOTE: This used to look for out of sequence hc files. Now throw exception if out of order.
 * Singles,Randoms,Prompts,Time(ms)
 * 534772,540,882,482
 * 534143,501,852,1617
 * 534107,541,889,2679
 */

int find_start_countrate(bf::path l64_file) {
  const bf::path hc_file = make_path(l64_file, FILE_TYPE::HC);
  if (! bf::is_regular_file(hc_file)) {
    g_logger->info("hc file {} not found; trying listmode file", hc_file.string());
    return find_start_countrate_lm(l64_file);
  }
  std::ifstream instream;
  if (open_istream(instream, hc_file, std::ios::in | std::ios::binary))
    return 1;
  // Read and omit the first line of the hc file
  std::string in_line;
  instream >> in_line;
  bx::sregex reg_hc = bx::sregex::compile("(?P<single>\\d+),(?P<prompt>\\d+),(?P<random>\\d+),(?P<time>\\d+)");
  bx::smatch what;
  int lasttime = -1;
  bool found = false;
  while (!found) {
    instream >> in_line;
    if (bx::regex_match(in_line, what, reg_hc)) {
      int time = boost::lexical_cast<int>(what["time"]);
      unsigned int prompt = boost::lexical_cast<unsigned int>(what["prompt"]);
      unsigned int random = boost::lexical_cast<unsigned int>(what["random"]);
      found = ((prompt - random) > (unsigned int)start_countrate_);
      if (lasttime > time) {
        g_logger->error("Time out of order in hc file {}: {}", hc_file.string(), in_line);
        throw;
      }
      lasttime = time;
    } else {
      g_logger->error("Bad hc file line: {}", in_line );
      throw;
    }
  }
  return lasttime;
}

// int find_start_countrate_OLD(const char *fname) {
//   char hc_fname[FILENAME_MAX], *ext = nullptr;
//   FILE *fp;
//   int singles, random[2], prompt[2], time[2];
//   char line[80];
//   int err_flag = 0;

//   std::string hc_fname = make_file_name(fname, FILE_TYPE::HC);
//   std::ifstream hc_file;
//   if (open_istream(hc_file, hc_fname, std::ios::in | std::ios::binary))
//     return 1;

//   strcpy(hc_fname, fname);
//   if ((ext = strrchr(hc_fname, '.')) != nullptr) 
//     strcpy(ext, ".hc");
//   if ((fp = fopen(hc_fname, "rt")) == nullptr)  {
//     // .hc file not there; use .l64 file.
//     return find_start_countrate_lm(fname);
//   }
//   fgets(line, sizeof(line), fp); // header
//   // get first and second entries
//   if (fscanf(fp, "%d,%d,%d,%d", &singles, &random[0], &prompt[0], &time[0]) != 4) 
//     err_flag++;
//   else if (fscanf(fp, "%d,%d,%d,%d", &singles, &random[1], &prompt[1], &time[1]) != 4) 
//     err_flag++;
//   if (err_flag) {
//     fclose(fp);
//     return -1;
//   }
//   if (time[1] < time[0]) { // first entry before time reset, move one line
//     random[0] = random[1]; prompt[0] = prompt[1]; time[0] = time[1];
//   }
//   while ((prompt[0] - random[0]) < (int)start_countrate_) {
//     if (fscanf(fp, "%d,%d,%d,%d", &singles, &random[0], &prompt[0], &time[0]) != 4) {
//       fclose(fp);
//       return -1;
//     }
//   }
//   fclose(fp);
//   return time[0];
// }


/*
 * int check_end_of_frame(L64EventPacket &packet)
 * Locate end of frame and stop producing packets until all packets
 * are sorted and CH files are writen
 */

int check_end_of_frame(L64EventPacket &src) {
  unsigned int ew1, ew2, type, tag;
  if (frame_duration <= 0 || frame_start_time < 0)
    return 0;
  int pos = (src.num_events - 1) * 2;
  const unsigned *in_buf = src.events;
  int frame_end = (frame_start_time + frame_duration) * 1000; // in msec
  int last_tag_pos = 0;
  while (pos >= 0) {
    ew1 = in_buf[pos];
    ew2 = in_buf[pos + 1];
    type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
    if (type == 2) {
      // tag word
      tag = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
      if ((tag & 0xE0000000) == 0x80000000) {
        int current_time = (tag & 0x3fffffff); //msec
        if (current_time < frame_end) {
          if (last_tag_pos > 0)
            return last_tag_pos / 2;
          L64EventPacket::current_time = current_time; // msec
          return 0;
        } else {
          last_tag_pos = pos;
          L64EventPacket::current_time = current_time; // msec;
        }
      }
    }
    pos -= 2;
  }
  return 0;
}

/*
 *  void rebin_packet(L64EventPacket &src, unsigned &src_pos, L32EventPacket &dst, unsigned &dst_pos)
 *  For each 64-bit events from src, decode the event into 32-bit event and add the event to dest
 *  until src is done or dest packet is full
 */
void rebin_packet(L64EventPacket &src, L32EventPacket &dst)
{
  unsigned int ew1, ew2, type, tag;
  int ax, ay, bx, by;
  int address;
  int ahead, bhead;
  // int ablk, bblk;
  int doia = 1, doib = 1;
  int mpe, error_flag = 0;
  const unsigned *in_buf = src.events;
  unsigned *out_buf = dst.events;
  unsigned  count = src.num_events * 2;

  unsigned *coinh_p = g_p_coinc_map;
  unsigned *coinh_d = g_d_coinc_map;
  int nvoxels = GeometryInfo::NUM_CRYSTALS_X_Y_HEADS;
  int npixels = GeometryInfo::NUM_CRYSTALS_X_Y;
  unsigned src_pos = 0, dst_pos = 0;
  unsigned ignore_border_crystal = eg_rebinner_method & IGNORE_BORDER_CRYSTAL;
  unsigned doi_processing = (eg_rebinner_method & NODOI_PROCESSING) == 0 ? 1 : 0;


  int xhigh = GeometryInfo::NXCRYS; //, yhigh=nycrys;
  if (ignore_border_crystal) {
    xhigh = GeometryInfo::NXCRYS - 1;
    //      yhigh = nycrys-1;
  }

  while ((src_pos + 1) < count && dst_pos < L32EventPacket::packet_size) {
    // if (! src_pos % 10000)
    //  cerr << "xxx src_pos " << src_pos << endl;
    ew1 = in_buf[src_pos];
    ew2 = in_buf[src_pos + 1];
    type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
    switch (type) {
    case 3: // not in sync
      src_pos++;
      break;
    case 2: // tag word
      src_pos += 2;
      tag = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
      if (g_hist_mode == HM_TRA) {
        // transmission mode
        if ((tag & 0xE0000000) == 0x80000000) {
          // timetag
          tx_source.timer++;
          out_buf[dst_pos] = tag;
          dst_pos++;
        } else if ((tag & 0xE0000000) == 0xc0000000) {
          // Gantry Motions & positions
          // int head = (tag & 0x000f0000) >> 16;
          // int tx_y = (tag & 0x0000ff00) >> 8;
          // int tx_x = tag & 0x000000ff;
          //  cerr << "xxx TX pos: " << tx_source.head << "," << tx_source.x << "," << tx_source.y << ": " << tx_mock.timer << "msec" << endl;
          tx_source.head = (tag & 0x000f0000) >> 16;
          tx_source.y = (tag & 0x0000ff00) >> 8;
          tx_source.x = tag & 0x000000ff;
          tx_source.z_low = tx_source.y - (tx_span / 2);
          tx_source.z_high = tx_source.y + (tx_span / 2);
          tx_source.timer = 0;
          tx_mock.head = tx_source.head;
          tx_mock.x = tx_source.x;
          tx_mock.y = (tx_source.y + GeometryInfo::NYCRYS / 2) % GeometryInfo::NYCRYS;
          tx_mock.z_low = tx_mock.y - (tx_span / 2);
          tx_mock.z_high = tx_mock.y + (tx_span / 2);
          // keep tx_low positive to allow comparison with unsigned
          if (tx_source.z_low < 0)
            tx_source.z_low = 0;
          if (tx_mock.z_low < 0)
            tx_mock.z_low = 0;
        } else {
          // other timetag
          out_buf[dst_pos] = tag;
          dst_pos++;
        }
      } else {
        out_buf[dst_pos] = tag;
        dst_pos++;
      }
      break;
    default:
      // event: 0=prompt, 1=delayed
      src_pos += 2;
      error_flag = 0;
      mpe = ((ew1 & 0x00070000) >> 16) | ((ew2 & 0x00070000) >> 13);
      ax = (ew1 & 0xff);
      ay = ((ew1 & 0xff00) >> 8);
      bx = (ew2 & 0xff);
      by = ((ew2 & 0xff00) >> 8);
      if (doi_processing || g_hist_mode == HM_TRA) {
        doia = (ew1 & 0x01C00000) >> 22;
        doib = (ew2 & 0x01C00000) >> 22;
        if (g_hist_mode == HM_TRA)
          if (doia != 7 && doib != 7) {
            g_logger->error("{}: Invalid crystal DOI fro TX event ({},{}) ({},{}) ", doia, ay, bx, by);
            error_flag++;
          }
      }
      if (mpe < 1 || mpe > nmpairs) {
        if (verbose)
          //    cerr << "current_time " << current_time << ": Invalid head pair " << mpe << ", error_flag " << error_flag << endl;
          error_flag++;
      }
      if (ax < 0 || ax >= GeometryInfo::NXCRYS || ay < 0 || ay >= GeometryInfo::NYCRYS) {
        if (verbose) 
          g_logger->error("{}: next_event_64: Invalid crystal pair A: ({},{}) (NXCRYS {}, GeometryInfo::NYCRYS {})", current_time, ax, ay, GeometryInfo::NXCRYS, GeometryInfo::NYCRYS);
        error_flag++;
      }
      if (bx < 0 || bx >= GeometryInfo::NXCRYS || by < 0 || by >= GeometryInfo::NYCRYS) {
        if (verbose) 
          g_logger->error("{}: next_event_64: Invalid crystal pair B: ({},{}) (NXCRYS {}, GeometryInfo::NYCRYS {})", current_time, ax, ay, GeometryInfo::NXCRYS, GeometryInfo::NYCRYS);
        error_flag++;
      }
      // if (ax >= GeometryInfo::NXCRYS ||  ay >= GeometryInfo::NYCRYS || bx >= GeometryInfo::NXCRYS || by >= GeometryInfo::NYCRYS) {
      //     if (verbose) cerr <<  current_time << ": Invalid crystal pair (" << ax << "," << ay << ") (" << bx << "," << by << ")" << endl;
      //     error_flag++;
      // }

      if (error_flag)
        continue;
      address = -1;
      if (doia == 7) {
        // Transmission : b is the single event
        // Ignore extra time per crystal due to accelaration/decceleration
        if (!doi_processing)
          doib = 1;
        if (tx_source.timer < g_tx_source_speed + 1) {
          if (by >= tx_source.z_low && by <= tx_source.z_high) {
            address = rebin_event_tx(mpe, doia, ax, ay, doib, bx, by);
            if (address >= 0) {
              address |=  0x20000000; // Transmission, , mock otherwise
            }
          } else if (by >= tx_mock.z_low && by <= tx_mock.z_high) {
            address = rebin_event_tx(mpe, doia,  tx_mock.x, tx_mock.y, doib, bx, by);
          }
          if (address >= 0) {
            address |= 0x40000000; // set prompt flag
          }
        }
      } else if (doib == 7) {
        // Transmission: a is the single event
        // Ignore extra time per crystal due to accelaration/decceleration
        if (!doi_processing)
          doia = 1;
        if (tx_source.timer < g_tx_source_speed + 1) {
          if ( ay >= tx_source.z_low && ay <= tx_source.z_high) {
            address = rebin_event_tx(mpe, doia, ax, ay, doib, bx, by);
            if (address >= 0) {
              address |=  0x20000000; // Transmission, mock otherwise
            }
          } else if (ay >= tx_mock.z_low && ay <= tx_mock.z_high) {
            address = rebin_event_tx(mpe, doia, ax, ay, doib, tx_mock.x, tx_mock.y);
          }
          if (address >= 0) {
            address |= 0x40000000; // set prompt flag
          }
        }
      } else {
        // Emission
        // mode 0=trues, 1=prompts&randoms, 2=prompts_only
        if ((doia == 1 || doia == 0) && (doib == 1 || doib == 0)) {
          //ignore border crytals : ax*ay*bx*by==0 if any crystal position==0
          //                 if (ignore_border_crystal &&
          //                     (ax*ay*bx*by==0 || ax==xhigh || ay==yhigh || bx==xhigh || by==yhigh))
          if (ignore_border_crystal && (ax * bx == 0 || ax == xhigh || bx == xhigh )) {
            address = -1;
          } else {
            if (g_hist_mode == HM_PRO) {
              if (type == 0) {
                // prompt
                address = rebin_event( mpe, doia, ax, ay, doib, bx, by /*, type*/);
                if (address >= 0) {
                  address |= 0x40000000; // set prompt flag
                }
              } else {
                // delayed: ignore by setting address to -1
                address = -1;
              }
            } else {
              address = rebin_event( mpe, doia, ax, ay, doib, bx, by /*, type*/);
              if (address >= 0 && type == 0) {
                address |= 0x40000000; // set prompt flag
              }
            }
          }
          //
          // Update coincidence histogram: increment singles in crystals indices
          // ablk = (ay / 8) * 9 + (ax / 8);
          // bblk = (by / 8) * 9 + (bx / 8);
          ahead = hrrt_mpairs[mpe][0];
          bhead = hrrt_mpairs[mpe][1];
          if (coinh_p && (type == 0)) {
            coinh_p[doia * nvoxels + ahead * GeometryInfo::NXCRYS + ax + ay * npixels]++;
            coinh_p[doib * nvoxels + bhead * GeometryInfo::NXCRYS + bx + by * npixels]++;
          } else if (coinh_d && (type == 1)) {
            coinh_d[doia * nvoxels + ahead * GeometryInfo::NXCRYS + ax + ay * npixels]++;
            coinh_d[doib * nvoxels + bhead * GeometryInfo::NXCRYS + bx + by * npixels]++;
          }
        }
      }
      if (address >= 0) {
        out_buf[dst_pos] =  address;
        dst_pos++;
      }
    }
  }
  dst.num_events = dst_pos;
}


/*
 *  void process_tagword(const L32EventPacket &src, FILE *out_hc)
 *  Process tagwords from the packet
 */
void process_tagword(const L32EventPacket &src, std::ofstream out_hc)
{
  unsigned *buf = src.events;
  int nevents = src.num_events;
  for (int i = 0; i < nevents; i++, buf++) {
    if (((*buf) & 0x80000000) != 0) {
      process_tagword(*buf, -1, out_hc);
    } else {
      if (((*buf) & 0x40000000) != 0)
        prompts++;
      else
        randoms++;
    }
  }
  L32EventPacket::current_time = current_time;
}

/*
 *  void process_tagword(const L64EventPacket &src, FILE *out_hc)
 *  Process tagwords from the packet
 */
void process_tagword(const L64EventPacket &src, std::ofstream out_hc)
{
  unsigned *buf = src.events;
  unsigned int ew1, ew2, type, tag;
  int i = 0, nevents = src.num_events;
  while (i < 2 * nevents) {
    ew1 = buf[i];
    ew2 = buf[i + 1];
    type = ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
    switch (type) {
    case 3: // not in sync
      i++;
      break;
    case 2: // tag word
      i += 2;
      tag = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
      if ((tag & 0x80000000) != 0)
        process_tagword(tag, -1, out_hc);
      break;
    default: // event
      i += 2;
      if (type == 0)
        prompts++;
      else
        randoms++;
    }
  }
}


/*
 * Histograms the input listmode stream to the output sinogram,
 * starting from the current input file position,
 * ending when the histogrammed time (time tag values) is greater or equal to the specified duration.
 * T type is either signed char (byte) or signed short
 *  Mode: 0="randoms substraction",
 *        1="separarate prompts and delayed",
 * Duration is filled back with the real histogrammed time.�
 * Histogram returns: frame start time (>=0),  -1 when an error is encountered.
 */
template <class T> int histogram(T *t_sino, char *delayed, int sino_size, int &t_duration, std::ofstream &t_out_hc) {
  int address = 0, tx_flag = 0;
  T *sub_sino = t_sino + sino_size;

  // int tx_sino_size = m_nprojs * m_nviews * (2 * GeometryInfo::NYCRYS - 1);
  fmt::print(t_out_hc, "Singles,Randoms,Prompts,Time(ms)\n");

  Event_32 cew;

  // reset the counters
  reset_statistics();
  frame_duration = t_duration;

  g_logger->info("Time(sec),Randoms,Prompts,Singles,Event Time(ms)");
  switch (g_hist_mode) {
  case HM_TRU: // randoms subtraction
  case HM_PRO: // prompts only
    while (next_event_32(cew, 0) == 0) {
      if (cew.type == Event_32::TAG) {
        process_tagword(cew.value, t_duration, t_out_hc);
      } else {
        if (cew.value >= 0 && cew.value < sino_size) {
          if (cew.type == Event_32::PROMPT)
            t_sino[cew.value]++;
          else
            t_sino[cew.value]--;
          //if (sinogram[cew.address] > overflow_size)
          //  cout << "Warning:  overflow sinogram, address=" << cew.address << endl;
        }
      }
      if (stop_count_ > 0 && event_counter >= stop_count_)
        break;
    }
    break;

  case HM_PRO_RAN: // separate randoms/prompts
    while (next_event_32(cew, 0) == 0) {
      if (cew.type == Event_32::TAG) {
        process_tagword(cew.value, t_duration, t_out_hc);
      } else {
        // use the prompt/random bit as the MSB of the address to create a virtual address space
        // twice as large as normal.   Lower half is randoms, upper half is prompts.
        if (cew.value >= 0  && cew.value < sino_size) {
          address = cew.value;
          if (cew.type == Event_32::PROMPT) {
            t_sino[address]++;
          } else {
            if (delayed == nullptr) {
              // delayed stored in same array as prompts
              t_sino[address + sino_size]++;
            } else {
              // delayed stored in different array
              if (delayed[address] < 255)
                delayed[address]++; // truncated to byte max
            }
          }
        }
      }
      if (stop_count_ > 0 && event_counter >= stop_count_)
        break;
    }
    break;
  case HM_TRA: // Transmission
    while (next_event_32(cew, 0) == 0) {
      if (cew.type == Event_32::TAG)
        process_tagword(cew.value, t_duration, t_out_hc);
      else {
        // use the mock flag address to create a virtual address space twice
        // as large as normal.   Lower half is randoms, upper half is prompts.
        if (cew.value >= 0) {
          tx_flag = cew.value & 0x20000000;
          address = cew.value & 0x1fffffff;
          if (address < sino_size) { // should always be
            if (tx_flag) {
              t_sino[address]++; // TX event
            } else {
              sub_sino[address]++;  // mock event
            }
          }
        }
      }
      if (stop_count_ > 0 && event_counter >= stop_count_)
        break;
    }
    break;
  }
  t_prompts += prompts;
  t_randoms += randoms;
  g_logger->info("Total Trues events={}", event_counter);
  g_logger->info("Sinogram events(prompts+randoms)=({}+{})={}", t_prompts, t_randoms, t_prompts + t_randoms);
  if (!terminate_frame) {
    // round current_time_msec as current_time
    current_time = (current_time_msec + 500) / 1000;
    g_logger->info("Current time {} msec rounded to {} sec", current_time_msec, current_time);
  }
  t_duration = (current_time - frame_start_time);
  return frame_start_time;
}

template int histogram(char *sino, char *delayed, int sino_size, int &duration,  std::ofstream &out_hc);
template int histogram(short *sino, char *delayed, int sino_size, int &duration, std::ofstream &out_hc);
/*
static void lmscan_32(std::ofstream &out, long *duration) {
  long prev_time = 0, time = 0;
  long prompts = 0, randoms = 0, total_singles = 0;
  // long max_plane = 0;

  // reset the counters
  reset_statistics();

  Event_32 cew;

  out << "Singles,Randoms,Prompts,Time(ms)" << std::endl;

  while (!next_event_32(cew, 1)) {
    if (cew.type == Event_32::PROMPT) prompts++;
    else if (cew.type == Event_32::DELAYED) randoms++;
    else if (cew.type == Event_32::TAG) {
      if ((cew.value & 0xE0000000) == 0x80000000) { // timetag found
        time = cew.value & 0x3fffffff;
        if (time / 1000 != prev_time) {
          prev_time = time / 1000;
          // calculate the gantry singles from individual values
          total_singles = 0;
          for (int block = 0; block < NBLOCKS; block++)
                 total_singles += singles_rate(block);
          fmt::print(out, "{},{},{},{}", total_singles, randoms, prompts, time);

          // reset the counters
          prompts = 0;
          randoms = 0;
          reset_statistics();
        }
      } else {
        // ahc 9/4/18
        // Have to assume this call to process_tagword(int, int) is an error and never called. See below.
        g_logger->error("Call to process_tagword(int, int) should never be made");
        assert(false);
        // process_tagword(cew.value, -1);
      }
    }
  }
  *duration = time;
}
*/

static void lmscan_64(std::ofstream &out, long *duration) {
  long prev_time = 0, time = 0;
  long prompts = 0, randoms = 0, total_singles = 0;
  // long max_plane = 0;

  // reset the counters
  reset_statistics();

  Event_32 cew;

  out << "Singles,Randoms,Prompts,Time(ms)" << std::endl;

  while (!next_event_64(cew, 1)) {
    if (cew.type == Event_32::PROMPT) {
      prompts++;
    } else if (cew.type == Event_32::DELAYED) {
      randoms++;
    } else if (cew.type == Event_32::TAG) {
      if ((cew.value & 0xE0000000) == 0x80000000) { // timetag found
        if (timetag_processing)
          time = cew.value & 0x3fffffff;
        else
          time++;
        if (time / 1000 != prev_time) {
          prev_time = time / 1000;
          // calculate the gantry singles from individual values
          total_singles = 0;
          for (int block = 0; block < NBLOCKS; block++)
            total_singles += singles_rate(block);
          out << fmt::format("{},{},{},{}", total_singles, randoms, prompts, time) << std::endl;
          // reset the counters
          prompts = 0;
          randoms = 0;
          reset_statistics();
        }
      } else {
        // ahc 9/4/18
        // Have to assume this call to process_tagword(int, int) is an error and never called.  Only prototypes in old code are:
        // long process_tagword(long tagword, long duration, FILE *out_hc)
        // void process_tagword(const L32EventPacket &src, FILE *out_hc)
        // void process_tagword(const L64EventPacket &src, FILE *out_hc)
        g_logger->error("Call to process_tagword(int, int) should never be made");
        assert(false);
        // process_tagword(cew.value, -1);
      }
    }
  }

  *duration = time;
}

void lmscan(std::ofstream &out, long *duration) {
  lmscan_64(out, duration);
}

head_curve *lmsplit(std::ofstream &out, long *duration) {
  /* REDESIGN NEEDED (MS 01-APR-04
          int ev_filter = 0;

          *duration = 0;

          Event_32 cew;

          while (!next_event(in,cew,0))
          {
                  if (cew.type == Event_32::TAG)
                  {
                          fwrite((void*)&cew.type,1,sizeof(long),out);
                          //cout << "tag: " << hex << cew.tag << endl;
                  }
                  else
                  {
                          // write one in 3 event words
                          if (!(ev_filter % 3))
                          {
                               //cout << "event: " << hex << cew.event << endl;
                               fwrite((void*)&cew.value,1,sizeof(long),out);
                          }
                          //else
                                  //cout << "skip: " << hex << cew.event << endl;

                          ev_filter++;
                  }
          }
  */
  return 0;
}

