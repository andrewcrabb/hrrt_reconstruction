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
#ifndef LM_Reader_h
#define LM_Reader_h
#include <iostream>
using namespace std;
typedef struct {
	HANDLE handle;
	DWORD BytesPerSector;
} FILE_IN;
extern int verbose;
typedef FILE_IN FILE_OUT;

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
	typedef enum {unknown=0, ready=0x1, hold=0x2, done=0x4, error=0x8,
		consumer_wait=0x10,  producer_wait=0x20} FileStatus;
	static unsigned packet_size;
	static unsigned file_status;
	static FILE *hc_out;
	static HANDLE container_ok; // L64EventPacket producer/consumer synchronization
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
	typedef enum {unknown=0, ready=0x1, hold=0x2, done=0x4, error=0x8, 
		consumer_wait=0x10, producer_wait=0x20} FileStatus;
	static unsigned packet_size;
	static unsigned file_status;
	static HANDLE container_ok; // L64EventPacket producer/consumer synchronization
	static int current_time;
	enum {empty, used} status; // processed means can be reused
	unsigned time, num_events;
	unsigned *events;
	L32EventPacket();
	~L32EventPacket();
};

// sinogram write  synchronization
// Event Readers and Rebinners are on hold during sinogram writing
extern HANDLE sinowrite_ok; 

#define PACKET_SIZE 1024*1024 // 4M events
#define MAX_CONTAINER_WAIT 1000		// 1000msec=1sec
#define MAX_FILEOPEN_WAIT 60000		// 1000msec=1sec
#define MAX_FILEWRITE_WAIT 120000		// 120000msec=2min
/*
 *	Containers arrays with NUM_PACKETS size
 */
extern L64EventPacket *l64_container;
extern int num_packets; // 4 packets
extern L32EventPacket *l32_container;


extern void lm64_reader(void *p); //const char *in_fname = (const char *)p;
extern void lm32_reader(void *p); //const char *in_fname = (const char *)p;

extern L64EventPacket *load_buffer_64();
extern L32EventPacket *load_buffer_32();

//
// LXX_consumer_wait:
// Set LXX consumer waiting flag
// Reset LXX container_ok Event
// Wait for container_ok event, which will be set
// when enough containers are made available
//
inline DWORD lm64_consumer_wait()
{
	L64EventPacket::file_status |= L64EventPacket::consumer_wait;
	ResetEvent(L64EventPacket::container_ok);
	return WaitForSingleObject(L64EventPacket::container_ok, MAX_CONTAINER_WAIT);
}
inline DWORD lm32_consumer_wait()
{
	L32EventPacket::file_status |= L32EventPacket::consumer_wait;
	ResetEvent(L32EventPacket::container_ok);
	return WaitForSingleObject(L32EventPacket::container_ok, MAX_CONTAINER_WAIT);
}


//
// LXX_producer_wait:
// Set LXX producer waiting flag
// Reset LXX container_ok Event
// Wait for container_ok event, which will be set
// when enough containers are made available
//
inline DWORD lm64_producer_wait()
{
	L64EventPacket::file_status |= L64EventPacket::producer_wait;
	ResetEvent(L64EventPacket::container_ok);
	return WaitForSingleObject(L64EventPacket::container_ok, MAX_CONTAINER_WAIT);
}
inline DWORD lm32_producer_wait()
{
	L32EventPacket::file_status |= L32EventPacket::producer_wait;
	ResetEvent(L32EventPacket::container_ok);
	return WaitForSingleObject(L32EventPacket::container_ok, MAX_CONTAINER_WAIT);
}

//
// LXX_consumer_ready:
// if LXX consumer waiting flag and enough containers are made available,
// Set LXXcontainer_ok Event (notify LXX consumer).
//
inline void lm64_consumer_ready(int pos)
{
	if ((L64EventPacket::file_status&L64EventPacket::consumer_wait))
	{
		if (l64_container[pos].status == L64EventPacket::used)
		{
			L64EventPacket::file_status = L64EventPacket::file_status & (~L64EventPacket::consumer_wait);
			if (verbose) cout << "lm64 consumer ready at " << pos<< endl;
			SetEvent(L64EventPacket::container_ok);
		}
	}
}

inline void lm32_consumer_ready(int pos)
{
	if ((L32EventPacket::file_status&L32EventPacket::consumer_wait))
	{
		if (l32_container[pos].status == L32EventPacket::used)
		{
			L32EventPacket::file_status = L32EventPacket::file_status & (~L32EventPacket::consumer_wait);
			if (verbose) cout << "lm32 consumer ready at " << pos << endl;
			SetEvent(L32EventPacket::container_ok);
		}
	}
}

//
// LXX_producer_ready:
// if LXX producer waiting flag and enough containers are made available,
// Set LXX container_ok Event (notify LXX producer).
//
inline void lm64_producer_ready(int pos)
{
	if ((L64EventPacket::file_status&L64EventPacket::producer_wait))
	{
		if (l64_container[pos].status == L64EventPacket::empty)
		{
			L64EventPacket::file_status = L64EventPacket::file_status & (~L64EventPacket::producer_wait);
			if (verbose) cout << "lm64 producer ready at " << pos<< endl;
			SetEvent(L64EventPacket::container_ok);
		}
	}
}
inline void lm32_producer_ready(int pos)
{
	if ((L32EventPacket::file_status&L32EventPacket::producer_wait))
	{
		if (l32_container[pos].status == L32EventPacket::empty)
		{
			L32EventPacket::file_status = L32EventPacket::file_status & (~L32EventPacket::producer_wait);
			if (verbose) cout << "lm32 producer ready at " << pos<< endl;
			SetEvent(L32EventPacket::container_ok);
		}
	}
}
#endif