/*
------------------------------------------------

   Component       : HRRT

   Authors         : Mark Lenox, Merence Sibomana
   Language        : C++
   Platform        : Microsoft Windows.

   Creation Date   : 04/02/2003
   Modification history:
          11/30/2005 : Add HRRT low Resolution and P39 support
                 Add nodoi support
          02/09/2006:  Exclude border crystals by default and provide an option to keep them (-ib)
          02/15/2006: Allow 2 low resoltion mode with 2mm and 2.4375mm binsize
          01/09/2009: Replace check_start_countrate() by
                      find_start_countrate() to locate histogramming start time
          20-MAY-2009: Use a single fast LUT rebinner
*/
#pragma once

#include <map>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include "LM_Reader_mp.hpp"

namespace bf = boost::filesystem;

enum class FILE_TYPE {
  SINO, 
  SINO_HDR, 
  LM_HC, 
  RA_S, 
  TR_S, 
  DYN, 
  HC, 
  L64_HDR, 
  FR_S
};
std::map <FILE_TYPE, std::string> FILE_EXTENSIONS = {
  // Constant names
  {FILE_TYPE::SINO    , ".s"},
  {FILE_TYPE::SINO_HDR, ".s.hdr"},
  {FILE_TYPE::LM_HC   , "_lm.hc"},
  {FILE_TYPE::RA_S    , ".ra.s"},
  {FILE_TYPE::TR_S    , ".tr.s"},
  {FILE_TYPE::DYN     , ".dyn"},
  {FILE_TYPE::HC      , ".hc"},
  {FILE_TYPE::L64_HDR , ".l64.hdr"},
  // Names with frame number
  {FILE_TYPE::FR_S    , "_frame{:2d}.s"}
};

// Histogram mode
enum HIST_MODE {HM_TRU = 0, HM_PRO_RAN = 1, HM_PRO = 2, HM_TRA = 7}; // 0=Trues (Default), 1=Prompts and Randoms, 2=Prompts only, 7=transmission
std::map <HIST_MODE, std::string> HISTOGRAM_MODES = {
  {HM_TRU    , "Nett trues"},
  {HM_PRO_RAN, "Prompts and randoms"},
  {HM_PRO    , "Prompts only"},
  {HM_TRA    , "Transmission"}
};

enum HC_FILE_COLUMNS {HC_SINGLE, HC_RANDOM, HC_PROMPT, HC_TIME};

 // Head Curve data structure  
typedef struct {
  long trues_rate; // in counts/second
  long randoms_rate;  // in counts/second
  long time;  // in seconds
  long singles; // in counts/second
} head_curve;

// Utility file-open used in histogram_mp and lmhistogram_mp
int open_istream(std::ifstream &ifstr, const bf::path &name, std::ios_base::openmode t_mode = std::ios::in );
int open_ostream(std::ofstream &ofstr, const bf::path &name, std::ios_base::openmode t_mode = std::ios::out );

 // Process time tag
extern long process_tagword(long tagword, long duration, std::ofstream &out_hc);
 // Set input listmode stream position to specified time
int goto_event( int target_time);

 // Histogram input listmode stream to sinogram, fill duration with time extracted from time tags
template <class T> int histogram(T *out_sino, char *delayed_sino, int sino_size, int &duration, std::ofstream &out_hc);

// Sort 64-bit events from src, decode event into 32-bit event, add event to dest until src is done or dest packet is full
void rebin_packet(L64EventPacket &src,  L32EventPacket &dst);

 // Check end of frame and stop producing packets until all packets are sorted and CH files are written
int check_end_of_frame(L64EventPacket &src);

 // Check start of frame 
int check_start_of_frame(L64EventPacket &src);

// find start countrate from file, returns time in msec
int find_start_countrate(const bf::path &infile);

// Process tagwords from packet
void process_tagword(const L32EventPacket &src, std::ofstream &out_hc);
void process_tagword(const L64EventPacket &src, std::ofstream &out_hc);

bf::path make_file_name(FILE_TYPE file_type);
bf::path make_file_name(FILE_TYPE file_type, bf::path stem, int frame_no);

 // Scan input listmode stream and extract head curve, fill duration with time extracted from time tags
void lmscan(std::ofstream &out, long *duration);

 // lmsplit: To be redesigned
head_curve *lmsplit(std::ofstream &out, long *duration);

 // get block singles count
int singles_rate(int block);
 // get frame prompts count
int64_t total_prompts();
int64_t total_tx_prompts(); // TX events for P39 simultaneous TX+EM
 // get frame randoms count
int64_t total_randoms();
int64_t total_tx_randoms(); // TX events for P39 simultaneous TX+EM

// Global variables
// extern int l64_flag;                // 1 if 64-bit mode, 0 otherwise
extern HIST_MODE g_hist_mode;                // 0=Trues (Default), 1=Prompts and Randoms, 2=Prompts only, 7=transmission
extern int g_max_rd;                   // maximum ring difference, default=GeometryInfo::MAX_RINGDIFF
extern int quiet;                      // default=0 (false)
extern int stop_count_;           // default 0 (Not applicable)
extern unsigned int start_countrate_;      // starting trues/sec  default=0 (Not applicable)
extern float g_tx_source_speed;        // msec per crystal, depends on axial_velocity value in transmission header
extern unsigned *p_coinc_map;          // crystal singles counters for prompts
                  // size = ncrystals
extern unsigned *d_coinc_map;   // crystal singles counters for delayed
                  // size = ncrystals

extern int write_coin_map(const boost::filesystem::path &datafile);
extern void reset_coin_map();
extern void log_message(const char *msg, int type=0);

extern int timetag_processing;
extern unsigned eg_rebinner_method;
typedef enum {HW_REBINNER=0x1, SW_REBINNER=0x2, IGNORE_BORDER_CRYSTAL=0x4, NODOI_PROCESSING=0x8} 
  RebinnerMethodMask;
extern int frame_start_time;
extern int frame_duration;
