// LM_Rebinner : 64-bit Listmode Rebinners thread entry point for the lmhistogram application.
//
/*[ LM_Rebinner.h
------------------------------------------------

   Component       : HRRT

   Name            : LM_Rebinner.h - Listmode Reader thread interface
   Authors         : Merence Sibomana
   Language        : C++
   Platform        : Microsoft Windows.

   Creation Date   : 04/25/2004
   Modification history:
					12/11/2004: Added max_rebinners variable for initial number of rebinners 
          20-MAY-2009: Use a single fast LUT rebinner

   Description     : 
                      lm64_rebinner is an entry-point for 64-bit listmode rebinner thread.
					  The argument is a pointer to the thread ID (ID = *(int*)p).
					  At startup, lm64_rebinner creates the 32-bit circular list of buffers
					  and waits for the container_ok signal set by the 64-bit listmode reader
					  when some 64-bit buffers are full.
					  It then sort the assigned buffers (index=ID+n*num_rebinners) until the
					  input container is empty. The thread ID=0 also sets the signal container_ok
					  when some input buffers are empty (in case the consumer is waiting).
					  Each thread is terminated when the file_status is done and 
					  all assigned input buffers are empty.
					  
               Copyright (C) CPS Innovations 2004 All Rights Reserved.

---------------------------------------------------------------------*/

# pragma once

#include <stdio.h>
#include <string>

constexpr int MODEL_HRRT = 328;

/**
 *	Number of rebinner threads
 */

/**
 * lm_rebinner(void *p):
 * Thread entry point for single rebinner thread.
 * The argument is not used.
 * lm64_rebinner sorts l64 packets from the l64 container to l32 packets into the l32 container
 */
extern void lm64_rebinner(void *p);

/**
 * Get span from configuration file (gm328.ini) for transmission mode (span=0)
 * and initialize the rebinner with the specified span and max_rd
 */
// ahc: lut file now passed rather than found in this fn
// int init_rebinner(int &span, int &max_rd);
int init_rebinner(int &span, int &max_rd, std::string const &lut_file);

extern int rebin_event(int mp, int alayer, int ax, int ay, int blayer, int bx, int by 
							 /*,unsigned delayed_flag*/);

extern int rebin_event_tx(int mp, int alayer, int ax, int ay, int blayer, int bx, int by);

extern int model_number;

// ahc from here.

namespace LM_Rebinner {
	extern std::string rebinner_lut_file;
};

enum class HeadType { 
	LSO_LSO  = 0, 
	LSO_NAI  = 1, 
	LSO_ONLY = 2, 
	LSO_GSO  = 3, 
	LSO_LYSO = 4 
} ;

extern int tx_span;
