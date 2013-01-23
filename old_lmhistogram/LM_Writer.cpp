#include "stdafx.h"
#include "LM_Writer.h"
#include "histogram.h"
#include <stdlib.h>
#include <iostream>
using namespace std;

/*
 *	void lm32_writer(void *arg): 32-bit listmode writer single thread
 *  arg is a pointer to a structure containing the output file HANDLE and
 *  output device bytes per sector.
 *  The thread has 2 states:
 *  1. The producer thread (lm64_rebinner)is busy; waits for ready packets and writes them.
 *     It returns if no packet has been ready after MAX_TRY*TIME_SLICE.
 *  2. Writes all remaining packets.
 */


void lm32_writer(void *arg)
{
	static int next_pos=0;
	int num_try=0;
	DWORD size=0, count=0,  total_size=0;
	HANDLE handle=INVALID_HANDLE_VALUE;
	const char *fname = (const char*)arg;
	L32EventPacket *buf=NULL;
	char outfname_hc[FILENAME_MAX];
	char sino_fname[FILENAME_MAX];
	if (fname == NULL)
	{
		cerr << "lm32_writer: invalid argument" << endl;
		return;
	}

	if (fname == NULL)
	{
		cerr << "lm32_writer: invalid argument" << endl;
		return;
	}
	handle = CreateFile (fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	if (handle == INVALID_HANDLE_VALUE)
	{
		cerr << "lm32_writer: error opening " << fname << endl;
		return;
	}
	strcpy(outfname_hc,fname);
	// Replace extension by "_lm.hc"
	char *ext = strrchr(outfname_hc,'.');
	if (ext != NULL) strcpy(ext, "_lm.hc");
	else strcat(outfname_hc,"_lm.hc");
	FILE *out_hc = fopen(outfname_hc,"w");
	if (out_hc == NULL)
	{
		perror(outfname_hc);
		exit(1);
	}

	if ((buf = new L32EventPacket) == NULL)
	{
		cerr << "lm32_writer: error allocating memory" << endl;
		return;
	}
	
	cout << "lm32_writer: started" << endl;
	while ((L32EventPacket::file_status&L32EventPacket::ready) !=0  ||
		l32_container[next_pos].status == L32EventPacket::used) 
	{
		// Check end of frame and output coincidence histogram
		if (l32_container[next_pos].status == L32EventPacket::empty )
		{
			if (frame_duration>0 && L32EventPacket::current_time-frame_start_time>=frame_duration)
			{
				strcpy(sino_fname,fname);
				if (L64EventPacket::frame_duration[L64EventPacket::current_frame] > 0)
				{ // requested frame duration: Replace extension by "_framenum.s"
					ext = strrchr(sino_fname,'.');
					if (ext == NULL) ext = sino_fname + strlen(sino_fname);
					sprintf(ext,"_frame%d.s", L64EventPacket::current_frame);
				}
				//Writing ch files may take too much time:  Set hold signal to notify reader 
				L64EventPacket::file_status |=  L64EventPacket::hold;
				write_coin_map(sino_fname);
				// Notify L64 Reader to continue reading
				L64EventPacket::file_status = L64EventPacket::file_status & (~L64EventPacket::hold);
				SetEvent(sinowrite_ok);
			}
		}
		while (l32_container[next_pos].status == L32EventPacket::empty)
		{
			if (verbose) cout << "lm32_writer: L32 container empty wait: " << L32EventPacket::file_status << endl;
			if (lm32_consumer_wait() != WAIT_OBJECT_0)
			{
				if (verbose) cout << "lm32_writer: No producer: " << L32EventPacket::file_status << endl;
//				CloseHandle(handle);
//				return;
			}
			if ((L32EventPacket::file_status&L32EventPacket::ready) == 0) break;
			if (verbose) cout << "lm32_writer: L32 container empty wait: " << L32EventPacket::file_status << endl;
		}
 
		if (l32_container[next_pos].status == L32EventPacket::empty) break;
		process_tagword(l32_container[next_pos], out_hc);
		// Copy l32 events into buffer
		unsigned remaining=0, added = l32_container[next_pos].num_events;
		if (buf->num_events+ added > L32EventPacket::packet_size)
		{
			remaining = buf->num_events+added - L32EventPacket::packet_size;
			added -= remaining;
		}
		memcpy(buf->events+buf->num_events, l32_container[next_pos].events, added*sizeof(unsigned));
		buf->num_events += added;
		if (buf->num_events == L32EventPacket::packet_size )
		{
			// complete and write buffer
			size = sizeof(unsigned)*L32EventPacket::packet_size;
			WriteFile(handle, buf->events, size, &count, NULL);
			if (count != size)
			{
				cerr << "Error writing 32-bit listmode" << endl;
				CloseHandle(handle);
				return;
			}
			if (verbose) cout << "lm32_writer: Writing packet " << next_pos << " done" << endl;
			total_size += size;

			// Copy remaining l32 events in buf
			memcpy(buf->events, l32_container[next_pos].events+added, remaining*sizeof(unsigned));
			buf->num_events = remaining;
		}
		l32_container[next_pos].num_events = 0;
		l32_container[next_pos].status = L32EventPacket::empty;
		// Notify producer if waiting and enough containers are made available
		lm32_producer_ready(next_pos);
		next_pos = (next_pos+1)% num_packets;
	}
	CloseHandle(handle);

	if (verbose) cout << "lm32_writer: write remaining " << buf->num_events << " events" << endl;
	if (buf->num_events > 0)
	{
		FILE *fp=NULL;
		size = sizeof(unsigned)*buf->num_events;
		if ((fp=fopen(fname,"ab+")) == NULL)
		{
			cerr << "lm32_writer: can not open file " << fname << endl;
			return;
		}
		if (fwrite(buf->events, size, 1, fp) != 1)
			cerr << "Error writing last packet to 32-bit listmode" << endl;
		fclose(fp);
		total_size += size;
	}

	if (verbose) cout << "Writing 32-bit listmode done: " << total_size << " bytes" << endl;

	// Output coincidence histogram
	strcpy(sino_fname,fname);
	ext = strrchr(sino_fname,'.');
	if (ext == NULL) ext = sino_fname + strlen(sino_fname);
	if (L64EventPacket::current_frame>0)
	{	// dynamic
		if (L64EventPacket::current_time-frame_start_time*1000 > 0)
		{
			// Replace extension by "_framenum.s"
			sprintf(ext,"_frame%d.s", L64EventPacket::current_frame);
			write_coin_map(sino_fname);
		}
		// else is empty frame
	}
	else
	{
		// single frame
		strcpy(ext,".s");
		write_coin_map(sino_fname);
	}
	cout << "L64EventPacket::current_frame="  << L64EventPacket::current_frame << endl;
	cout << sino_fname << " done" << endl;
	ResetEvent(sinowrite_ok);
	return;
}


/*
 *	void lm64_writer(void *arg): 64-bit listmode writer single thread
 *  arg is a pointer to a structure containing the output file HANDLE and
 *  output device bytes per sector.
 *  The thread has 2 states:
 *  1. The producer thread (lm64_reader)is busy; waits for ready packets and writes them.
 *     It returns if no packet has been ready after MAX_TRY*TIME_SLICE.
 *  2. Writes all remaining packets.
 */


void lm64_writer(void *arg)
{
	static int next_pos=0;
	int num_try=0;
	DWORD size=0, count=0,  total_size=0;
	HANDLE handle=INVALID_HANDLE_VALUE;
	const char *fname = (const char*)arg;
	char outfname_hc[FILENAME_MAX];
	if (fname == NULL)
	{
		cerr << "lm64_writer: invalid argument" << endl;
		return;
	}

	if (fname == NULL)
	{
		cerr << "lm64_writer: invalid argument" << endl;
		return;
	}
	handle = CreateFile (fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	if (handle == INVALID_HANDLE_VALUE)
	{
		cerr << "lm64_writer: error opening " << fname << endl;
		return;
	}
	strcpy(outfname_hc,fname);
	// Replace extension by "_lm.hc"
	char *ext = strrchr(outfname_hc,'.');
	if (ext != NULL) strcpy(ext, "_lm.hc");
	else strcat(outfname_hc,"_lm.hc");
	FILE *out_hc = fopen(outfname_hc,"w");
	if (out_hc == NULL)
	{
		perror(outfname_hc);
		exit(1);
	}
	cout << "lm64_writer: started" << endl;
	size = 2*sizeof(unsigned)*L32EventPacket::packet_size;
	while ((L64EventPacket::file_status&L64EventPacket::ready) !=0  ||
		l64_container[next_pos].status == L64EventPacket::used) 
	{
		while (l64_container[next_pos].status == L64EventPacket::empty)
		{   //L64EventPacket::file_status != L64EventPacket::done
			if (verbose) cout << "lm64_writer: L64 container empty wait: " << L64EventPacket::file_status << endl;
			if (lm64_consumer_wait() != WAIT_OBJECT_0)
			{
				cout << "lm64_writer: No producer: " << L64EventPacket::file_status << endl;
				CloseHandle(handle);
				return;
			}
			if (verbose) cout << "lm64_writer: L64 container empty wait exit: " << L64EventPacket::file_status << endl;
		}
		// process hc and  write buffer
		process_tagword(l64_container[next_pos], out_hc);
		if (l64_container[next_pos].num_events < L32EventPacket::packet_size) 
		{
			// end of file, write remaining events if any
			CloseHandle(handle);
			if (l64_container[next_pos].num_events > 0)
			{
				FILE *fp=NULL;
				size = 2*sizeof(unsigned)*l64_container[next_pos].num_events;
				if ((fp=fopen(fname,"ab+")) == NULL)
				{
					cerr << "lm64_writer: can not open file " << fname << endl;
					return;
				}
				if (fwrite(l64_container[next_pos].events, size, 1, fp) != 1)
					cerr << "Error writing last packet to 64-bit listmode" << endl;
				fclose(fp);
				total_size += size;
			}
			return;
		}

		WriteFile(handle, l64_container[next_pos].events, size, &count, NULL);
		if (count != size)
		{
			cerr << "Error writing 32-bit listmode" << endl;
			CloseHandle(handle);
			return;
		}
		total_size += size;
		l64_container[next_pos].status = L64EventPacket::empty;
		// Notify producer if waiting and enough containers are made available
		lm64_producer_ready(next_pos);
		next_pos = (next_pos+1)% num_packets;
	}
	CloseHandle(handle);
	return;
}

