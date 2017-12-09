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
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "LM_Reader.h"
/**
 * Head Curve data structure	
 */
typedef struct
{
	long trues_rate; // in counts/second
	long randoms_rate;  // in counts/second
	long time;  // in seconds
	long singles; // in counts/second
} head_curve;

/**
 * Process time tag
 */
extern long process_tagword(long tagword, long duration, FILE *out_hc=NULL);
/**
 * Set input listmode stream position to specified time
 */
int goto_event(FILE_IN *in, int target_time);

/**
 * Histogram input listmode stream to sinogram, fill duration with time extracted from time tags
 */
template <class T> int histogram(FILE_IN *in, T *out_sino, char *delayed_sino, int sino_size, 
								 int &duration, FILE *out_hc=NULL);

/**
 *	Sort  64-bit events from src, decode the event into 32-bit event and add the event to dest
 *  until src is done or dest packet is full
 */
void rebin_packet(L64EventPacket &src,  L32EventPacket &dst);
/**
 * Check end of frame and stop producing packets until all packets are sorted and CH files are written
 */
int check_end_of_frame(L64EventPacket &src);
/**
 * Check start of frame 
 */
int check_start_of_frame(L64EventPacket &src);

/**
 * find start countrate from file, returns time in msec
 */
int find_start_countrate(const char *filename);

/**
 *	Process tagwords from packet
 */
void process_tagword(const L32EventPacket &src, FILE *out_hc);
void process_tagword(const L64EventPacket &src, FILE *out_hc);

/**
 * Scan input listmode stream and extract head curve, fill duration with time extracted from time tags
 */
void lmscan(FILE_IN *in, FILE *out, long *duration);

/**
 * lmsplit: To be redesigned
 */
head_curve *lmsplit(FILE_IN *in, FILE *out, long *duration);

#ifdef unix
typedef unsigned int __int64;
#endif

/**
 * get block singles count
 */
int singles_rate(int block);
/**
 * get frame prompts count
 */
__int64 total_prompts();
__int64 total_tx_prompts(); // TX events for P39 simultaneous TX+EM
/**
 * get frame randoms count
 */
__int64 total_randoms();
__int64 total_tx_randoms(); // TX events for P39 simultaneous TX+EM

//
// Global variables
//
extern int l64_flag;				// 1 if 64-bit mode, 0 otherwise
extern int hist_mode;					// 0=Trues (Default), 1=Prompts and Randoms, 
									// 2=Prompts only, 7=transmission
extern int max_rd;					// maximum ring difference, default=67
extern int quiet;					// default=0 (false)
extern unsigned stop_count;			// default 0 (Not applicable)
extern unsigned start_countrate;			// starting trues/sec  default=0 (Not applicable)
extern float tx_source_speed;			// number of msec per crystal, depends on 
									// axial_velocity value in transmission header
extern unsigned *p_coinc_map;		// crystal singles counters for prompts
									// size = ncrystals
extern unsigned *d_coinc_map;		// crystal singles counters for delayed
									// size = ncrystals

extern void write_coin_map(const char *datafile);
extern void reset_coin_map();
extern void log_message(const char *msg, int type=0);

extern int timetag_processing;
extern unsigned rebinner_method;
typedef enum {HW_REBINNER=0x1, SW_REBINNER=0x2, IGNORE_BORDER_CRYSTAL=0x4, NODOI_PROCESSING=0x8} 
	RebinnerMethodMask;
extern int frame_start_time;
extern int frame_duration;

#endif