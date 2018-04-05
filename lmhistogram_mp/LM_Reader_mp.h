// LM_Reader : 64-bit and 32-bit Listmode Readers thread entry point for the lmhistogram application.
//
/*[ LM_Reader.h
------------------------------------------------

   Component       : HRRT

   Name            : LM_Reader.h - Listmode Reader thread interface
   Authors         : Merence Sibomana
   Language        : C++
   Platform        : Microsoft Windows.

   Creation Date   : 04/25/2004
   Description     :  lm32_reader and  lm64_reader are respectively 32-bit and 64-bit listmode
					  read thread entry points.
					  The thread reads NUM_PACKECTS buffers (containers).
					  Each container capacity is PACKET_SIZE events.
					  The event size is 8 bytes for a 64-bit event and 4 bytes for a 32-bit event.
					  The thread use a producer/consumer method and Windows synchronization events.

  Modification history:
					12/07/2004: Increase MAX_FILEWRITE_WAIT from 10sec to 120sec
          20-MAY-2009: Use a single fast LUT rebinner
					  
               Copyright (C) CPS Innovations 2004 All Rights Reserved.

---------------------------------------------------------------------*/
# pragma once

#include <iostream>
using namespace std;

extern int verbose;

#define PACKET_SIZE 1024*1024 // 4M events

/*
 *	Container syncronization Events for producer/consumer
 *  1. The consumer thread is started waiting for the event which will be set
 *     by the producer when half containers are full or when the producer terminates.
 *  2. The event is reset by the producer when all containers are full and 
 *     set by the consumer when half containers are empty.
 *  3. The event is reset by the consumer when all containers are empty and 
 *     set by the producer when half containers are full.
 */

class L64EventPacket {
public:
	static unsigned packet_size;
  static char *in_fname;
  static FILE* in_fp;
	static FILE *hc_out;
	static int current_frame;
	static int *frame_duration;
	static int *frame_skip;
	static int current_time;
	enum {empty, used} status;  // processed means can be reused
	unsigned time, num_events;
	unsigned *events;
	L64EventPacket();
	~L64EventPacket();
};

class L32EventPacket {
public:
	static unsigned packet_size;
  static char *in_fname;
  static FILE* in_fp;
	static int current_time;
	enum {empty, used} status; // processed means can be reused
	unsigned time, num_events;
	unsigned *events;
	L32EventPacket();
	~L32EventPacket();
};

/*
 *	Containers arrays with NUM_PACKETS size
 */
extern L64EventPacket *l64_container;
extern L32EventPacket *l32_container;


extern void lm64_reader(void *p); //const char *in_fname = (const char *)p;
extern void lm32_reader(void *p); //const char *in_fname = (const char *)p;

extern L64EventPacket *load_buffer_64();
extern L32EventPacket *load_buffer_32();
