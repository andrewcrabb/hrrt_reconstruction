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
 *	Container synchronization Events for producer/consumer
 *  1. The consumer thread is started waiting for the event which will be set
 *     by the producer when half containers are full or when the producer terminates.
 *  2. The event is reset by the producer when all containers are full and 
 *     set by the consumer when half containers are empty.
 *  3. The event is reset by the consumer when all containers are empty and 
 *     set by the producer when half containers are full.
 */


#include "stdafx.h"
#include "LM_Reader.h"
#include "LM_Rebinner.h"
#include "histogram.h"
#include <stdlib.h>
#include <iostream>
using namespace std;


L64EventPacket *l64_container=NULL;
L32EventPacket *l32_container=NULL;
int num_packets = 4;
int verbose = 0;

HANDLE L64EventPacket::container_ok=INVALID_HANDLE_VALUE;
HANDLE L32EventPacket::container_ok=INVALID_HANDLE_VALUE;
FILE *L64EventPacket::hc_out = NULL;

unsigned L64EventPacket::packet_size = PACKET_SIZE;
unsigned L64EventPacket::file_status = L64EventPacket::unknown;

int L64EventPacket::current_frame=0;
int L64EventPacket::current_time=0;
int *L64EventPacket::frame_duration=NULL;
int *L64EventPacket::frame_skip=NULL;


unsigned L32EventPacket::packet_size = PACKET_SIZE;
unsigned L32EventPacket::file_status = L32EventPacket::unknown;
int L32EventPacket::current_time=0;

L64EventPacket::L64EventPacket()
{
	events = (unsigned*)calloc(packet_size, 2*sizeof(unsigned));
	status = empty;
	num_events = 0;
	if (events == NULL) throw("memory allocation error");
}

L64EventPacket::~L64EventPacket()
{
	if (events != NULL) free(events);
}

L32EventPacket::L32EventPacket()
{
	events = (unsigned*)calloc(packet_size, sizeof(unsigned));
	status = empty;
	num_events = 0;
	if (events == NULL) throw("memory allocation error");
}

L32EventPacket::~L32EventPacket()
{
	if (events != NULL) free(events);
}

static void get_sector_size(const char *full_path, DWORD &BytesPerSector)
{
	
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char dir_path[_MAX_DIR];
	char ext[_MAX_EXT];
	DWORD  SectorsPerCluster, NumberOfFreeClusters, TotalNumberOfClusters;

	if (full_path != NULL && strlen(full_path)>1)
	{
		dir_path[0] = '\0';
		_splitpath(full_path, drive, dir, fname, ext);
		if (strlen(drive) > 0)
			sprintf(dir_path,"%s\\", drive);
		else if (strlen(dir)> 0) strcpy(dir_path,dir);
		if (dir_path[0] == '\0')
			GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
		else GetDiskFreeSpace( dir_path, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
		int err = GetLastError();
		if (err == NO_ERROR) return;
		cerr << full_path << ": Unable to get disk info" << endl;
	}  else cerr << "Invalid input file name" << endl;
	exit(1);
}


/*
 *	void lm64_reader(void *arg=NULL): 
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
static unsigned int ewtypes[16] = {3,3,1,0,3,3,2,2,3,3,3,3,3,3,3,3};
void lm64_reader (void *arg)
{
	HANDLE handle=INVALID_HANDLE_VALUE;
	DWORD BytesPerSector=512;
	DWORD count1=0, count2=0;
	fpos_t file_size=0, count=0;
	int next_pos = 0;
	int next_frame_num_events = 0, next_frame_offset=0;
	const char *in_fname = (const char *)arg;
	int num_frames = 1;
  int new_frame_flag=1;

	if (in_fname == NULL)
	{
		cerr << "lm64_reader: invalid argument" << endl;
		return;
	}
	get_sector_size( in_fname, BytesPerSector);
	handle = CreateFile (in_fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	if (handle == INVALID_HANDLE_VALUE)
	{
		cerr << "lm64_reader: error opening " << in_fname << endl;
		return;
	}
	if ((count1 = GetFileSize(handle, &count2)) == 0xFFFFFFFF)
	{
		cerr << "lm64_reader: error opening " << in_fname << endl;
		CloseHandle(handle);
		return;
	}
	file_size = ((fpos_t)count2<<32) + count1; 
			if (verbose) cout << "file size " << count2  << " " << count1 << " ready" << endl;
	if (l64_container == NULL) 	try 
	{
		L64EventPacket::packet_size = (L64EventPacket::packet_size/BytesPerSector)*BytesPerSector;
		l64_container = new L64EventPacket[num_packets];
		for (int i=0; i<num_packets; i++) cout << "L64-Packet " << i << " status=" << l64_container[i].status << endl;
	}
	catch (const char *err_msg)
	{
		cerr << "Error: " << err_msg << endl;
		delete[] l64_container;
		CloseHandle(handle);
		return;
	}

	// set frame duration for first frame
	if (L64EventPacket::frame_duration != NULL)
	{
		if (verbose) cout << "lm64_reader: FRAME 0" << endl;
		frame_start_time = 0;
		frame_duration = L64EventPacket::frame_duration[0];
		if (L64EventPacket::frame_skip != NULL )
			frame_start_time += L64EventPacket::frame_skip[0];
		while (L64EventPacket::frame_duration[num_frames]>0) num_frames++;
	}
	L64EventPacket::file_status |= L64EventPacket::ready; // notify consumer
	while(L64EventPacket::file_status&L64EventPacket::ready)
	{
		while (l64_container[next_pos].status != L64EventPacket::empty)
		{  // container full, wait for next free packet
			if ((L64EventPacket::file_status&L64EventPacket::hold) == 0)
			{
				if (verbose) cout << "lm64_reader:Container full, wait" << endl;
				if (lm64_producer_wait() != WAIT_OBJECT_0)
				{
					if (verbose) cout << "lm64 reader: No consummer, Rebinner not present?" << endl;
//					CloseHandle(handle);
//					return;
				}
			}
			else
			{
				ResetEvent(sinowrite_ok);
				if (verbose) cout << "L64EventPacket::hold" << endl;
				WaitForSingleObject(sinowrite_ok, MAX_FILEWRITE_WAIT);
			}
		}
		DWORD bytes_to_read=2*sizeof(unsigned)*L64EventPacket::packet_size, bytes_read=0;

		if (next_frame_num_events == 0)
		{
			ReadFile(handle, l64_container[next_pos].events, bytes_to_read, &bytes_read, NULL);
			l64_container[next_pos].num_events = bytes_read/(2*sizeof(unsigned));
			count += bytes_read;
		}
		else
		{ // copy previous buffer and reset next_frame_num_events
			if (L64EventPacket::frame_duration != NULL)
			{
        new_frame_flag=1;
				if (verbose) cout << "lm64_reader: NEXT FRAME" << endl;
				L64EventPacket::current_frame++;
				frame_start_time = L64EventPacket::current_time/1000; // in sec
				frame_duration = L64EventPacket::frame_duration[L64EventPacket::current_frame];
				if (L64EventPacket::frame_skip != NULL )
					frame_start_time += L64EventPacket::frame_skip[L64EventPacket::current_frame];
				if (frame_duration <=0)
				{ // Requested frames done
					L64EventPacket::file_status = L64EventPacket::done; 
					SetEvent(L64EventPacket::container_ok);
					if (verbose) cout << "lm64_reader: done" << endl;
					return;
				}
			}
			int prev_pos = next_pos==0? num_packets-1 : next_pos-1;
			memcpy(l64_container[next_pos].events, l64_container[prev_pos].events+(2*next_frame_offset), 
				next_frame_num_events*2*sizeof(unsigned));
			l64_container[next_pos].num_events = next_frame_num_events;
			next_frame_num_events = 0;
		}
			
		// Check skip and discard events in skip window
		if (L64EventPacket::frame_skip[L64EventPacket::current_frame]> 0)
		{
			unsigned start_pos = check_start_of_frame(l64_container[next_pos]);
      if (new_frame_flag)
      {
        printf("Skipping to %d secs ...\n", frame_start_time);
        new_frame_flag=0;
      }
			if (start_pos >= l64_container[next_pos].num_events) 
			{
				if (verbose) cout << "lm64_reader: packet " << next_pos  <<
					" skipped " << l64_container[next_pos].num_events << " ready" << endl;
				continue;
			}
			else
			{
				unsigned *dest=l64_container[next_pos].events;
				unsigned *src=l64_container[next_pos].events+start_pos*2;
				if (verbose) cout << "lm64_reader: packet " << next_pos  << 
					" " << start_pos << " events skipped" << endl;

        // todo: find a better way to copy the data bloc
				for (unsigned j=start_pos*2;
					j<l64_container[next_pos].num_events*2; j++) *dest++ = *src++;
 
        // Reset skip counter for this frame so the test is done once
        L64EventPacket::frame_skip[L64EventPacket::current_frame] = 0;
			}
		}

		if (frame_duration>0)
		{
			int eof_pos = check_end_of_frame(l64_container[next_pos]);
			if (eof_pos>0)
			{  // include the end-of-frame timetag in this packet
				// and set next_frame_num_events
				next_frame_num_events = l64_container[next_pos].num_events - (eof_pos+1);
				next_frame_offset = eof_pos+1;
				l64_container[next_pos].num_events = eof_pos+1;
			}
		}

		if (l64_container[next_pos].num_events > 0)
		{
			l64_container[next_pos].status = L64EventPacket::used;
			if (verbose) cout << "lm64_reader: packet " << next_pos  << " num events " << l64_container[next_pos].num_events << " ready" << endl;
			// Notify consumer if waiting and enough containers are made available
			lm64_consumer_ready(next_pos);
		}
		else if (next_frame_num_events==0)
		{
			if (GetLastError() != NO_ERROR) 
			{
				cout << "lm64_reader: read error" << endl;
				L64EventPacket::file_status = L64EventPacket::error; // notify consumer
			}
			else
			{
				// Need to reopen the file with fopen?;
				break;
			}
		}


		if (next_frame_num_events>0) 
		{ // wait for next sinowrite_ok and shift events
			if (verbose)	cout << "lm64_reader: end of frame" << endl;
			ResetEvent(sinowrite_ok);
			if (WaitForSingleObject(sinowrite_ok, MAX_FILEWRITE_WAIT) != WAIT_OBJECT_0)
			{
				cout << "lm64_reader: Writing Frame sinogram(s) timeout reached" << endl;
				L64EventPacket::file_status = L64EventPacket::error; // notify consumer
			}
		}

		next_pos = (next_pos+1)% num_packets;
	}
	CloseHandle(handle);
	if (L64EventPacket::file_status != L64EventPacket::error && count<file_size)
	{  // reopen file to read remaining events
		size_t remaining = ((size_t)(file_size-count))/(2*sizeof(unsigned));
		if (verbose) cout << "lm64_reader: reopening " << in_fname << " to read " << 
			remaining << " remaining events" << endl;
		FILE *fp = fopen(in_fname, "rb");
		if (fp!=NULL)
		{
			if (fsetpos(fp, &count) == 0)
			{
				count1 = fread(l64_container[next_pos].events, 2*sizeof(unsigned), remaining, fp);
				if (count1 == remaining)
				{
					l64_container[next_pos].num_events = remaining;
					l64_container[next_pos].status = L64EventPacket::used;
					fclose(fp);
				}
				else
				{
					cerr << "lm64_reader: error reading " << in_fname << " to read " <<
						remaining << " remaining events" << endl,
					fclose(fp);
				}
			}
			else
			{
				cerr << "lm64_reader: error positionning " << in_fname << " to read " <<
					remaining << " remaining events" << endl,
				fclose(fp);
			}
		}
		else
		{
			cerr << "lm64_reader: error reopening " << in_fname << " to read " <<
				remaining << " remaining events" << endl;
		}
	}
	//
	// notify consumer: set done status and signal container ok
	// 
	L64EventPacket::file_status = L64EventPacket::done; 
	SetEvent(L64EventPacket::container_ok);
	if (verbose) cout << "lm64_reader: done" << endl;
}

/*
 *	void lm32_reader(void *arg=NULL): 
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
void lm32_reader (void *arg)
{
	HANDLE handle=INVALID_HANDLE_VALUE;
	DWORD BytesPerSector=512;
	DWORD count1=0, count2=0;
	fpos_t file_size=0, count=0;
	int next_pos = 0;

	const char *in_fname = (const char *)arg;
	if (in_fname == NULL)
	{
		cerr << "lm32_reader: invalid argument" << endl;
		return;
	}
	//
	// Open input file and get the number of bytes per sector.
	// The file is open without buffering and the bytes to read must be a multiple of
	// bytes per sector.
	//
	get_sector_size( in_fname, BytesPerSector);
	handle = CreateFile (in_fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	if (handle == INVALID_HANDLE_VALUE)
	{
		cerr << "lm32_reader: error opening " << in_fname << endl;
		return;
	}
	if ((count1 = GetFileSize(handle, &count2)) == 0xFFFFFFFF)
	{
		cerr << "lm32_reader: error opening " << in_fname << endl;
		CloseHandle(handle);
		return;
	}
	file_size = ((fpos_t)count2<<32) + count1; 
	if (l32_container == NULL) 	try 
	{
		L32EventPacket::packet_size = (L32EventPacket::packet_size/BytesPerSector)*BytesPerSector;
		l32_container = new L32EventPacket[num_packets];
	}
	catch (const char *err_msg)
	{
		cerr << "Error: " << err_msg << endl;
		delete[] l32_container;
		CloseHandle(handle);
		return;
	}
	L32EventPacket::file_status |= L32EventPacket::ready; // notify consumer
	while(L32EventPacket::file_status&L32EventPacket::ready)
	{
		while (l32_container[next_pos].status == L32EventPacket::used)
		{ // container full, wait for next free packet
			if ((L32EventPacket::file_status&L32EventPacket::hold) == 0)
			{
				if (verbose) cout << "lm32_reader:Container full, wait" << endl;
				if (lm32_producer_wait() != WAIT_OBJECT_0)
				{
					cout << "lm32 reader: No consummer, Histogrammer not present?" << endl;
//					CloseHandle(handle);
//					return;
				}
			}
			else
			{
				ResetEvent(sinowrite_ok);
				if (verbose) cout << "L32EventPacket::hold" << endl;
				WaitForSingleObject(sinowrite_ok, MAX_FILEWRITE_WAIT);
			}
		}
		DWORD bytes_to_read=sizeof(unsigned)*L32EventPacket::packet_size, bytes_read=0;
		ReadFile(handle, l32_container[next_pos].events, bytes_to_read, &bytes_read, NULL);
		l32_container[next_pos].num_events = bytes_read/sizeof(unsigned);
		count += bytes_read;
		if (l32_container[next_pos].num_events > 0)
		{
			if (verbose) cout << "lm32_reader: packet " << next_pos << " ready" << endl;
			l32_container[next_pos].status = L32EventPacket::used; 
			// Notify consumer if waiting and enough containers are made available
			lm32_consumer_ready(next_pos);
		}
		else
		{
			if (GetLastError() != NO_ERROR) 
			{
				cerr << "lm32_reader: read error" << endl;
				L32EventPacket::file_status = L32EventPacket::error; // notify consumer
			}
			else
			{ 
				// Need to reopen the file with fopen?;
				break;
			}
		}
		next_pos = (next_pos+1)% num_packets;
	}
	CloseHandle(handle);
	if (L32EventPacket::file_status != L32EventPacket::error && count<file_size)
	{  // reopen file to read remaining events
		size_t remaining = ((size_t)(file_size-count))/sizeof(unsigned);
		if (verbose) cout << "lm32_reader: reopening " << in_fname << " to read " << 
			remaining << " remaining events" << endl;
		FILE *fp = fopen(in_fname, "rb");
		if (fp!=NULL)
		{
			if (fsetpos(fp, &count) == 0)
			{
				count1 = fread(l32_container[next_pos].events, sizeof(unsigned), remaining, fp);
				if (count1 == remaining)
				{
					l32_container[next_pos].num_events = remaining;
					l32_container[next_pos].status = L32EventPacket::used;
					fclose(fp);
				}
				else
				{
					cerr << "lm32_reader: error reading remaining events" << in_fname << " read " <<
						remaining << " returns " << count1 << endl;
					fclose(fp);
				}
			}
			else
			{
				cerr << "lm32_reader: error positioning " << in_fname << " to read " <<
					remaining << " remaining events" << endl;
				fclose(fp);
			}
		}
		else
		{
			cerr << "lm32_reader: error reopening " << in_fname << " to read " <<
					remaining << " remaining events" << endl;
		}
	}
	//
	// notify consumer: set done status and signal container ok
	// 
	L32EventPacket::file_status = L32EventPacket::done; 
	SetEvent(L32EventPacket::container_ok);
	if (verbose) cout << "lm32_reader: done" << endl;
}
/*
 *	L64EventPacket *load_buffer_64()
 *  Returns the first available L64EventPacket packet read by lm64_reader.
 *  It waits up to MAX_TRY*TIME_SLICE when no packet is available.
 */
L64EventPacket *load_buffer_64()
{
	static int next_pos=0;

	next_pos = next_pos%num_packets;
	// Notify producer if waiting and previous containers are made available
	for (int i=next_pos-1; i>=0; i--)
	{
		if (l64_container[i].status == L64EventPacket::empty) 
		{
			lm64_producer_ready(i);
			break;
		}
	}
	while (l64_container[next_pos].status == L64EventPacket::empty)
	{
		if (L64EventPacket::file_status == L64EventPacket::done) return NULL;
		if (verbose) cout  << "load_buffer_64: Container empty, wait" << endl;
		if (lm64_consumer_wait() != WAIT_OBJECT_0) 
		{
			cout << "load_buffer_64: No producer, l64_reader not present?" << endl;
//			return NULL;
		}
	}
	// Notify producer if waiting and enough containers are made available
	// and return
	lm64_producer_ready(next_pos);
	return &l64_container[next_pos++];
}

/*
 *	L32EventPacket *load_buffer_32()
 *  Returns the first available L32EventPacket packet read by lm32_reader or sorted by
 *  lm64_rebinner.
 *  It waits up to MAX_TRY*TIME_SLICE when no packet is available.
 */
L32EventPacket *load_buffer_32()
{
	static int next_pos=0;
	
	next_pos = next_pos%num_packets;

	// Notify producer if waiting and previous containers are made available
	for (int i=next_pos-1; i>=0; i--)
	{
		if (l32_container[i].status == L32EventPacket::empty) 
		{
			lm32_producer_ready(i);
			break;
		}
	}

	if (verbose) cout << "load_buffer_32: packet " << next_pos << endl;
	while (l32_container[next_pos].status == L32EventPacket::empty)
	{
		if (L32EventPacket::file_status == L32EventPacket::done) return NULL;
		if (verbose) cout << "load_buffer_32: container empty, wait" << endl;
		if (lm32_consumer_wait() != WAIT_OBJECT_0) 
		{
			if (verbose) cout << "load_buffer_32: No producer, l32_reader not present?" << endl;
//			return NULL;
		}
	}
	if (verbose) cout << "load_buffer_32: packet " << next_pos << " #events=" <<  l32_container[next_pos].num_events << " ready" << endl;
	if (verbose) cout << "load_buffer_32: packet " << next_pos << " status=" <<  l32_container[next_pos].status << endl;
	return &l32_container[next_pos++];
}
