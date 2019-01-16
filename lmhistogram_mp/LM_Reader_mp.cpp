// LM_Reader : Defines Listmode Reader thread entry
//
/*[ LM_Reader.cpp
------------------------------------------------

   Component       : HRRT

   Name            : LM_Reader.h - lm64_reader / lm32_reader modules interface
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 05/14/2004
   Description     : 
      lm64_reader and lm32_reader are entry-point for 64-bit and 32-bit listmode reader thread
      At startup, lm64_reader creates the 64-bit circular list of buffers and
      lm32_reader creates the 32-bit circular list of buffers.
      The thread reads the input file in the circular buffers and stops when
      all buffers are full. When all buffers are full, the thread resets the container_ok signal
      and waits for the signal to be set by the consumer when some buffers have been processed.
      The thread sets the status file_status=done to notify the consumers.

  Modification History:
        12/07/2004 : Check if the previous frame sinogram(s) have been succesfully written
              to disk before reading in next frame events.
        10-MAR-2006: Check for more than 2 frames in a buffer
        07-JAN-2009: Add new_frame_flag 
                    Remove start_countrate code
        20-MAY-2009: Use a single fast LUT rebinner

      
               Copyright (C) CPS Innovations 2004 All Rights Reserved.

---------------------------------------------------------------------*/

/*
 *  Container synchronization Events for producer/consumer
 *  1. The consumer thread is started waiting for the event which will be set
 *     by the producer when half containers are full or when the producer terminates.
 *  2. The event is reset by the producer when all containers are full and 
 *     set by the consumer when half containers are empty.
 *  3. The event is reset by the consumer when all containers are empty and 
 *     set by the producer when half containers are full.
 */

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/stat.h>
#include <cstdint>

#include "LM_Reader_mp.hpp"
#include "LM_Rebinner_mp.hpp"
#include "histogram_mp.hpp"

namespace fs = boost::filesystem;
using namespace std;

L64EventPacket *g_l64_container = NULL;
L32EventPacket *g_l32_container = NULL;
int verbose = 0;

// char *L64EventPacket::in_fname = NULL;
// FILE *L64EventPacket::in_fp = NULL;
boost::filesystem::path L64EventPacket::in_fname;
std::ifstream L64EventPacket::in_fp;
FILE *L64EventPacket::hc_out = NULL;

unsigned L64EventPacket::packet_size = PACKET_SIZE;

int L64EventPacket::current_frame = 0;
int L64EventPacket::current_time = 0;
std::vector<int> L64EventPacket::frame_duration;
std::vector<int> L64EventPacket::frame_skip;

// char *L32EventPacket::in_fname = NULL;
// FILE *L32EventPacket::in_fp = NULL;
boost::filesystem::path L32EventPacket::in_fname;
unsigned L32EventPacket::packet_size = PACKET_SIZE;
int L32EventPacket::current_time = 0;

L64EventPacket::L64EventPacket() {
  events = (unsigned*)calloc(packet_size, 2*sizeof(unsigned));
  status = empty;
  num_events = 0;
  if (events == NULL) 
    throw("memory allocation error");
}

L64EventPacket::~L64EventPacket() {
  if (events != NULL) 
    free(events);
}

L32EventPacket::L32EventPacket() {
  events = (unsigned*)calloc(packet_size, sizeof(unsigned));
  status = empty;
  num_events = 0;
  if (events == 
    NULL) throw("memory allocation error");
}

L32EventPacket::~L32EventPacket() {
  if (events != NULL) free(events);
}

/*
 *  void lm64_reader(void *arg = NULL): 
 *  lm64_reader is the function called to initialize the 64-bit listmode reader thread.
 *  arg is a pointer to a structure containing the input file HANDLE and output device bytes per sector.
 *  The function:
 *  1. Sets the packet size to a multiple of bytes per sector and allocates memory for packets;
 *     it returns if memory allocation fails
 *  2. Sets L64EventPacket::file_status to L64EventPacket::ready to notify consumer
 *  3. Reads packets from file int memory packets. It waits up to MAX_TRY*TIME_SLICE when all
 *     packets are full, and exits if no packet has been freed after within this limit time.
 *  4. Sets L64EventPacket::file_status to L64EventPacket::done when read is done or to
 *     L64EventPacket::error to notify consumer and returns
 */
// static unsigned int ewtypes[16] = {3,3,1,0,3,3,2,2,3,3,3,3,3,3,3,3};

void lm64_reader (const fs::path &infile) {
  L64EventPacket::in_fname = infile;
  // L64EventPacket::in_fp = open_istream(L64EventPacket::in_fname, ios::in | ios::binary);
  open_istream(L64EventPacket::in_fp, L64EventPacket::in_fname, ios::in | ios::binary);
  L64EventPacket::in_fp.read((char *)g_l64_container[0].events, sizeof(int64_t) * L64EventPacket::packet_size);
  if (L64EventPacket::in_fp.good()) {
    g_l64_container = new L64EventPacket[2];
    g_l32_container = new L32EventPacket;
    g_l64_container[0].num_events = L64EventPacket::packet_size;
    g_l64_container[0].status = L64EventPacket::used;
  } else {
    cerr << "lm64_reader: error reading " << L64EventPacket::in_fname.string() << endl;
  }
}

/*
 *  void lm32_reader(void *arg = NULL): 
 *  lm32_reader is the function called to initialize the 32-bit listmode reader thread.
 *  arg is a pointer to a structure containing the input file HANDLE and output device bytes per sector.
 *  The function:
 *  1. Sets the packet size to a multiple of bytes per sector and allocates memory for packets;
 *     it returns if memory allocation fails
 *  2. Sets L32EventPacket::file_status to L32EventPacket::ready to notify consumer
 *  3. Reads packets from file int memory packets. It waits up to MAX_TRY*TIME_SLICE when all
 *     packets are full, and exits if no packet has been freed after within this limit time.
 *  4. Sets L32EventPacket::file_status to L32EventPacket::done when read is done or
 *     to  L32EventPacket::error to notify consumer and returns
 */
void lm32_reader (const fs::path &infile) {
  L32EventPacket::in_fname = infile;
  open_istream(L64EventPacket::in_fp, L32EventPacket::in_fname, ios::in | ios::binary);
  L32EventPacket::in_fp.read((char *)g_l32_container->events, 2 * sizeof(unsigned) * L32EventPacket::packet_size);
  if (L32EventPacket::in_fp.good()) {
    g_l32_container = new L32EventPacket[2];
    g_l32_container[0].num_events = L32EventPacket::packet_size;
    g_l32_container[0].status = L32EventPacket::used;
  } else {
    cerr << "lm64_reader: error reading " << L32EventPacket::in_fname << endl;
  }
}

/*
 *  L64EventPacket *load_buffer_64()
 *  Returns the first available L64EventPacket packet read by lm64_reader.
 *  It waits up to MAX_TRY*TIME_SLICE when no packet is available.
 */
L64EventPacket *load_buffer_64() {
  if (g_l64_container[0].status == L64EventPacket::empty) {
    // copy buffer 1 if busy
    if (g_l64_container[1].status == L64EventPacket::used) {
      memcpy(g_l64_container[0].events, g_l64_container[1].events, g_l64_container[1].num_events * sizeof(int64_t));
      g_l64_container[1].status = L64EventPacket::empty;
    } else {
      // Read in new buffer
      L64EventPacket::in_fp.read((char *)g_l64_container[0].events, sizeof(int64_t) * L64EventPacket::packet_size);
      if (L64EventPacket::in_fp.fail()) {
        std::cerr << "lm64_reader: error reading " << L64EventPacket::in_fname.string() << std::endl;
        exit(1);
      }
      if (L64EventPacket::in_fp.eof()) {
        std::cout << "lm64_reader: " << L64EventPacket::in_fname.string() << " file done" << std::endl;
        return NULL;
      }
      g_l64_container[0].num_events = L64EventPacket::packet_size;
    }
  }
  if (g_l64_container[0].num_events < 1)
    return NULL;
  g_l64_container[0].status = L64EventPacket::used;

  // check end of frame
  if (frame_duration > 0) {
    int eof_pos = check_end_of_frame(g_l64_container[0]);
    if (eof_pos > 0) {
      // include the end-of-frame timetag in this packet and set next_frame_num_events
      g_l64_container[1].num_events = g_l64_container[0].num_events - (eof_pos + 1);
      g_l64_container[0].num_events = eof_pos + 1;
      memcpy(g_l64_container[1].events, g_l64_container[0].events + (2 * (eof_pos + 1)), g_l64_container[1].num_events * sizeof(int64_t));
    }
  }
  return &g_l64_container[0];
}

/*
 *  L32EventPacket *load_buffer_32()
 *  Returns the first available L32EventPacket packet read by lm32_reader or sorted by
 *  lm64_rebinner.
 *  It waits up to MAX_TRY*TIME_SLICE when no packet is available.
 */
L32EventPacket *load_buffer_32() {
  L64EventPacket *l64;
  if ((l64 = load_buffer_64()) == NULL) 
    return NULL;
  rebin_packet(*l64, *g_l32_container);
  l64->status = L64EventPacket::empty;
  return (g_l32_container->num_events > 0) ? g_l32_container : NULL;
}
