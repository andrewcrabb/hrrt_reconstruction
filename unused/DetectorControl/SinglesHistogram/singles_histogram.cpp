#include "singles_histogram.h"
#include <stdio.h>
#include <stdlib.h>
#include "stdafx.h"

#include "gantryModelClass.h"
#include "isotopeInfo.h"

// Global values
static long m_Scanner = 0;
static long m_NumBlocks = 0;
static long m_Blocks_X = 0;
static long m_Blocks_Y = 0;
static long m_Crystals_X = 0;
static long m_Crystals_Y = 0;
static long m_Total_Crystals_X = 0;
static long m_Total_Crystals_Y = 0;
static long m_Type = 0;
static long m_Rebinner = 0;
static long m_Rotate = 0;
static long m_DataSize = 0;
static long m_Timing_Bins = 0;
static long m_Timing_Offset = 0;
static long m_Initialized = 0;

static unsigned long m_NumEvents = 0;
static unsigned long m_BadAddress = 0;
static unsigned long m_SyncProb = 0;

static long *buf[MAX_HEADS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static long P39_matrix[MAX_HEADS] = { 0,  1,  2,  3,  4,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static long P39_ps[MAX_HEADS] = {-1, 9, -1, -1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static long petspect_matrix[MAX_HEADS] = { 0,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static long HRRT_matrix[MAX_HEADS] = { 0,  1,  2,  3,  4,  5,  6,  7, -1, -1, -1, -1, -1, -1, -1, -1};
static long head_matrix[MAX_HEADS];
static long ptsrc_matrix[MAX_HEADS];
static long m_Process[MAX_HEADS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define NUM_XE 32
static long Head_A_xe_HRRT[NUM_XE] = {-1,  0,  0,  0,  0,  0,  1,  1,
									   1,  1,  1,  2,  2,  2,  2,  3,
									   3,  3,  4,  4,  5, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1};
static long Head_B_xe_HRRT[NUM_XE] = {-1,  2,  3,  4,  5,  6,  3,  4,  
									   5,  6,  7,  4,  5,  6,  7,  5,
									   6,  7,  6,  7,  7, -1, -1, -1, 
									  -1, -1, -1, -1, -1, -1, -1, -1};

static long Head_A_xe_P39[NUM_XE] = {-1,  0,  0,  0,  1,  1,  1,  2,
									  2,  3, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1};
static long Head_B_xe_P39[NUM_XE] = {-1,  2,  3,  4,  3,  4,  5,  4, 
									  5,  5, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1};

long Singles_Start(long Scanner, long Type, long Rebinner, long Process[MAX_HEADS], long Status[MAX_HEADS])
{
	// local variables
	long i = 0;
	long j = 0;
	long OverStatus = 0;

	// reset global counters and initialization flag
	m_NumEvents = 0;
	m_BadAddress = 0;
	m_SyncProb = 0;
	m_Initialized = 0;

	// instantiate a Gantry Object Model object
	CGantryModel model;

	// initialize head status to success
	for (i = 0 ; i < MAX_HEADS ; i++) Status[i] = 0;
	for (i = 0 ; i < MAX_HEADS ; i++) m_Process[i] = 0;

	// make sure all previous memory allocations are released
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if (buf[i] != NULL) {
#ifdef _WIN32_DCOM
			::CoTaskMemFree((void *) buf[i]);
#else
			free((void *) buf[i]);
#endif
			buf[i] = NULL;
		}
	}

	// set gantry model to model number passed in (allows the default scanner to be overridden)
	if (!model.setModel(Scanner)) OverStatus = 1;

	// set scanner parameters
	switch (model.modelNumber()) {

	// petspect
	case 311:
		m_Scanner = SCANNER_PETSPECT;
		m_Timing_Bins = 256;
		for (i = 0 ; i < MAX_HEADS ; i++) head_matrix[i] = petspect_matrix[i];
		for (i = 0 ; i < MAX_HEADS ; i++) ptsrc_matrix[i] = -1;
		break;

	// HRRT
	case 328:
		m_Scanner = SCANNER_HRRT;
		m_Timing_Bins = 256;
		for (i = 0 ; i < MAX_HEADS ; i++) head_matrix[i] = HRRT_matrix[i];
		for (i = 0 ; i < MAX_HEADS ; i++) ptsrc_matrix[i] = -1;
		break;

	// different flavors of P39
	case 391:
	case 392:
	case 393:
	case 394:
	case 395:
	case 396:
		m_Scanner = SCANNER_P39;
		m_Timing_Bins = 32;
		for (i = 0 ; i < MAX_HEADS ; i++) head_matrix[i] = P39_matrix[i];
		for (i = 0 ; i < MAX_HEADS ; i++) ptsrc_matrix[i] = P39_ps[i];
		break;

	// unknown, return error
	default:
		OverStatus = 1;
		break;
	}

	// global variables
	m_NumBlocks = model.blocks();
	if (model.isHeadRotated()) {
		m_Rotate = 1;
		m_Blocks_X = model.axialBlocks();
		m_Blocks_Y = model.transBlocks();
		m_Crystals_X = model.axialCrystals();
		m_Crystals_Y = model.angularCrystals();
	} else {
		m_Rotate = 0;
		m_Blocks_X = model.transBlocks();
		m_Blocks_Y = model.axialBlocks();
		m_Crystals_X = model.angularCrystals();
		m_Crystals_Y = model.axialCrystals();
	}

	// Derivitive values
	m_Total_Crystals_X = m_Blocks_X * m_Crystals_X;
	m_Total_Crystals_Y = m_Blocks_Y * m_Crystals_Y;
	m_Timing_Offset = m_Timing_Bins / 2;

	// set items based on data type
	if (OverStatus == 0) {
		switch (Type) {

		case DHI_MODE_POSITION_PROFILE:
		case DHI_MODE_CRYSTAL_ENERGY:
			m_DataSize = 256 * 256 * m_NumBlocks;
			break;

		case DHI_MODE_TUBE_ENERGY:
			m_DataSize = 256 * 4 * m_NumBlocks;
			break;

		case DHI_MODE_SHAPE_DISCRIMINATION:
			m_DataSize = 256 * m_NumBlocks;
			break;

		case DHI_MODE_RUN:
		case DHI_MODE_TRANSMISSION:
			m_DataSize = 128 * 128;
			break;

		case DHI_MODE_SPECT:
			m_DataSize = 256 * 128 * 128;
			break;

		// Time Difference
		case NUM_DHI_MODES:
			m_DataSize = m_Timing_Bins * 128 * 128;
			break;

		// unknown data type
		default:
			OverStatus = 2;
			break;
		}
	}

	// Transfer Type
	m_Type = Type;

	// Verify Rebinner State
	if (OverStatus == 0) {
		if ((Rebinner < 0) || (Rebinner > 1)) {
			OverStatus = 3;
		} else {
			m_Rebinner = Rebinner;
		}
	}

	// Rebinner not allowed for time histogram
	if (m_Type == NUM_DHI_MODES) m_Rebinner = 0;

	// Verify Head Flags
	for (i = 0 ; (i < MAX_HEADS) && (OverStatus == 0) ; i++) {
		if ((Process[i] < 0) || (Process[i] > 1)) {
			OverStatus = 5;
			Status[i] = 5;
		} else {
			m_Process[i] = Process[i];
		}
	}

	// allocate memory
	if (OverStatus == 0) {

		// for each possible head
		for (i = 0 ; (i < MAX_HEADS) && (OverStatus == 0) ; i++) {

			// if it is to be processed
			if (Process[i] == 1) {

				// allocate space
#ifdef _WIN32_DCOM
				buf[i] = (long *) ::CoTaskMemAlloc(m_DataSize * sizeof(long));
#else
				buf[i] = (long *) malloc(m_DataSize * sizeof(long));
#endif
				

				// if insufficient memory
				if (buf[i] == NULL) {

					// set error
					OverStatus = 6;
					Status[i] = 6;

					// release memory from previous heads
					for (j = 0 ; j < i ; j++) {
						if (Process[j] == 1) {
#ifdef _WIN32_DCOM
							::CoTaskMemFree((void *) buf[i]);
#else
							free((void *) buf[i]);
#endif
							buf[j] = NULL;
						}
					}

				// allocation successful
				} else {

					// zero out memory
					for (j = 0 ; j < m_DataSize ; j++) buf[i][j] = 0;
				}
			}
		}
	}

	// if no errors, then set to initialized
	if (OverStatus == 0) m_Initialized = 1;

	// return overall status
	return OverStatus;
}

long Singles_Histogram(unsigned char *buffer, long NumBytes)
{
	// local variables
	long i = 0;
	long curr = 0;
	long step = 0;
	long Head = 0;
	long Block = 0;
	long X = 0;
	long Y = 0;
	long Tube = 0;
	long Bin = 0;
	long Temp = 0;
	long Address = 0;

	double Energy_Time_A = 0.0;
	double Energy_Time_B = 0.0;
	double Ratio = 0.0;

	unsigned char local[4];

	// if not initialized, return failure
	if (m_Initialized != 1) return 9;

	// if Time Difference Data, then wrong mode
	if (m_Type == NUM_DHI_MODES) return 10;

	// determine the step between events
	step = (m_Rebinner == 1) ? 4 : 8;

	// loop through the data
	for (curr = 0 ; curr <= (NumBytes - step) ; curr += step) {

		// for 64 bit data, sync up
		while ((m_Rebinner == 0) && ((buffer[curr+7] & 0x80) != 0x80)) {
			m_SyncProb++;
			curr += 4;
		}

		// first two bytes always the same
		local[0] = buffer[curr+0];
		local[1] = buffer[curr+1];

		// rebinner converting to 32-bit, just transfer over
		if (m_Rebinner == 1) {
			local[2] = buffer[curr+2];
			local[3] = buffer[curr+3];

		// otherwise, rebin it ourself
		} else {
			local[2] = buffer[curr+4];
			local[3] = buffer[curr+5];
		}

		// if it is a tagword, then skip it
		if ((local[3] & 0x80) == 0x80) continue;

		// increment number of events
		m_NumEvents++;

		// Determine Head (needed for all)
		for (i = 0, Head = -1; (i < MAX_HEADS) && (Head == -1) ; i++) {
			if (((long)local[3] == head_matrix[i]) || ((long)local[3] == ptsrc_matrix[i])) Head = i;
		}

		// bad head numer - increment bad count and continue to next event
		if (Head == -1) {
			m_BadAddress++;
			continue;
		}

		// Head Not being processed - continue to next event
		if (m_Process[Head] == 0) continue;

		// Block Address Needed for these data types
		if ((m_Type == DHI_MODE_POSITION_PROFILE) || (m_Type == DHI_MODE_CRYSTAL_ENERGY) ||
			(m_Type == DHI_MODE_TUBE_ENERGY) || (m_Type == DHI_MODE_SHAPE_DISCRIMINATION)) {

			// Break out block number
			Block = (long)((local[2] >> 1) & 0x7f);
			if ((Block < 0) || (Block >= m_NumBlocks)) {
				m_BadAddress++;
				continue;
			}
		}

		// data type specific
		switch (m_Type) {

		// position profile accessed by block & X/Y coordinates
		// crystal energy accessed by crystal and bin
		case DHI_MODE_POSITION_PROFILE:
		case DHI_MODE_CRYSTAL_ENERGY:
			Y = (long) local[0];			// Crystal
			X = (long) local[1];			// Bin
			Address = (Block * 256 * 256) + (Y * 256) + X;
			break;

		// tube energy accessed by block, tube and bin
		case DHI_MODE_TUBE_ENERGY:
			Tube = (long) local[0];
			if (Tube > 3) {
				m_BadAddress++;
				continue;
			}
			Bin = (long) local[1];
			Address = (Block * 4 * 256) + (Tube * 256) + Bin;
			break;

		// pulse shape discrimination accessed by block and energy ratio
		case DHI_MODE_SHAPE_DISCRIMINATION:
			Energy_Time_A = (double) local[1];
			Energy_Time_B = (double) local[0];
			if (Energy_Time_B == 0.0) {
				m_BadAddress++;
				continue;
			}
			Ratio = (Energy_Time_A / Energy_Time_B) * 255.0;
			if (m_Scanner != SCANNER_HRRT) Ratio /= 3.0;
			if (Ratio > 255.0) {
				m_BadAddress++;
				continue;
			}
			Bin = (long) (Ratio + 0.5);
			Address = (Block * 256) + Bin;
			break;

		// Run and Transmission data are accessed by Crystal X/Y Location
		case DHI_MODE_RUN:
		case DHI_MODE_TRANSMISSION:

			// coordinates
			Y = (long) (local[0] & 0x7f);
			X = (long) (local[1] & 0x7f);

			// leave transmission detector events alone
			if ((long)local[3] != ptsrc_matrix[Head]) {
				if (m_Rotate == 1) {
					Temp = Y;
					Y = m_Total_Crystals_Y - X - 1;
					X = Temp;
				}
				if ((X < 0) || (X >= m_Total_Crystals_X) || (Y < 0) || (Y >= m_Total_Crystals_Y)) {
					m_BadAddress++;
					continue;
				}
			}
			Address = (Y * 128) + X;
			break;

		// SPECT data is accessed by Crystal X/Y Location and Energy Bin
		case DHI_MODE_SPECT:
			Bin = (long) local[0];
			Y = (long) (local[1] & 0x7f);
			X = (long) (local[2] & 0x7f);
			if (m_Rotate == 1) {
				Temp = Y;
				Y = m_Total_Crystals_Y - X - 1;
				X = Temp;
			}
			if ((X < 0) || (X >= m_Total_Crystals_X) || (Y < 0) || (Y >= m_Total_Crystals_Y)) {
				m_BadAddress++;
				continue;
			}
			Address = (((Y * 128) + X) * 256) + Bin;
			break;
		}

		// increment counter
		buf[Head][Address]++;
	}

	// return overall status
	return 0;
}

long Time_Histogram(unsigned char *buffer, long NumBytes)
{

	long index = 0;
	long Crystal_AX = 0;
	long Crystal_AY = 0;
	long Crystal_BX = 0;
	long Crystal_BY = 0;
	long xe = 0;
	long Head_A = 0;
	long Head_B = 0;
	long Address_A = 0;
	long Address_B = 0;
	long Temp = 0;
	long diff = 0;
	long prompt = 0;

	// if not initialized, return failure
	if (m_Initialized != 1) return 9;

	// if Time Difference Data, then wrong mode
	if (m_Type != NUM_DHI_MODES) return 10;
	
	// loop through the data
	for (index = 0 ; index <= (NumBytes - 8) ; index += 8) {


		// for 64 bit data, sync up
		while ((buffer[index+7] & 0x80) != 0x80) {
			m_SyncProb++;
			index += 4;
		}
		// if it is a tag word, then skip it
		if ((buffer[index+3] & 0x80) == 0x80) continue;

		// increment event
		m_NumEvents++;

		// scanner specific data
		switch (m_Scanner) {

		case SCANNER_PETSPECT:

			// id crystals
			Crystal_AX = (long) buffer[index+0] & 0x7f;
			Crystal_AY = (long) buffer[index+1] & 0x7f;
			Crystal_BX = (long) buffer[index+4] & 0x7f;
			Crystal_BY = (long) buffer[index+5] & 0x7f;

			// if transmission detectors, then move it to more appropriate place
			if (Crystal_AX == 0x7f) {
				Crystal_AX = (Crystal_AY - 0x7c) * m_Crystals_X;
				Crystal_AY = 8 * m_Crystals_Y;
			}
			if (Crystal_BX == 0x7f) {
				Crystal_BX = (Crystal_BY - 0x7c) * m_Crystals_X;
				Crystal_BY = 8 * m_Crystals_Y;
			}

			// heads are always 0 & 1 for petspect (2 head system)
			Head_A = 0;
			Head_B = 1;

			// break out time bin and shift it so that it peak forms in middle
			diff = (((long) buffer[index+2] & 0x18) >> 3) +
					(((long) buffer[index+6] & 0x18) >> 1) + 
					(((long) buffer[index+5] & 0x80) >> 3) +
					(((long) buffer[index+4] & 0x80) >> 2) +
					(((long) buffer[index+1] & 0x80) >> 1) +
					((long) buffer[index+0] & 0x80) + m_Timing_Offset;
   			if (diff >= m_Timing_Bins) diff -= m_Timing_Bins;
			break;

		case SCANNER_P39:

			// check if prompt
			prompt = (((long) buffer[index+7] & 0x40) == 0x40) ? 1 : 0;

			// id crystals
			Crystal_AX = (long) buffer[index+0] & 0x7f;
			Crystal_AY = (long) buffer[index+1] & 0x7f;
			Crystal_BX = (long) buffer[index+4] & 0x7f;
			Crystal_BY = (long) buffer[index+5] & 0x7f;

			// identify module pair and through it the heads
			xe = (((long) buffer[index+6] & 0x07) << 3) + ((long) buffer[index+2] & 0x07);
			Head_A = Head_A_xe_P39[xe];
			Head_B = Head_B_xe_P39[xe];

			// for rotated heads (3+ heads), the data needs to be unrotated
			if (m_Rotate == 1) {
				Temp = Crystal_AY;
				Crystal_AY = m_Total_Crystals_Y - Crystal_AX - 1;
				Crystal_AX = Temp;
				Temp = Crystal_BY;
				Crystal_BY = m_Total_Crystals_Y - Crystal_BX - 1;
				Crystal_BX = Temp;
			}

			// check for transmission detectors
			if (m_Rotate == 0) {
				if ((buffer[index+0] & 0x80) == 0x80) {
					Crystal_AX = Crystal_AX;
					Crystal_AY = 8 * m_Crystals_Y;
					Head_A = 4;
					Head_B = xe;
				}
				if ((buffer[index+4] & 0x80) == 0x80) {
					Crystal_BX = Crystal_BX;
					Crystal_BY = 8 * m_Crystals_Y;
					Head_A = xe;
					Head_B = 1;
				}
			} else {
				if ((buffer[index+1] & 0x80) == 0x80) {
					Crystal_AX = Crystal_AX;
					Crystal_AY = 8 * m_Crystals_Y;
					Head_A = 4;
					Head_B = xe;
				}
				if ((buffer[index+5] & 0x80) == 0x80) {
					Crystal_BX = Crystal_BX;
					Crystal_BY = 8 * m_Crystals_Y;
					Head_A = xe;
					Head_B = 1;
				}
			}

			// break out time bin and shift it so that it peak forms in middle
			diff = (((long) buffer[index+7] & 0x06) << 2) + (((long) buffer[index+3] & 0x0e) >> 1) + m_Timing_Offset;
   			if (diff >= m_Timing_Bins) diff -= m_Timing_Bins;
			break;

		case SCANNER_HRRT:

			// id crystals
			Crystal_AX = (long) buffer[index+0] & 0x7f;
			Crystal_AY = (long) buffer[index+1] & 0x7f;
			Crystal_BX = (long) buffer[index+4] & 0x7f;
			Crystal_BY = (long) buffer[index+5] & 0x7f;

			// identify module pair and through it the heads
			xe = (long) buffer[index+6] & 0x1f;
			Head_A = Head_A_xe_HRRT[xe];
			Head_B = Head_B_xe_HRRT[xe];

			// break out time bin and shift it so that it peak forms in middle
			diff = ((long) buffer[index+2] & 0xff) + m_Timing_Offset;
   			if (diff >= m_Timing_Bins) diff -= m_Timing_Bins;
			break;
		}

		// Check for bad addressing
		if ((Head_A == -1) || (Head_B == -1) || (diff < 0) || (diff >= m_Timing_Bins) ||
			(Crystal_AX < 0) || (Crystal_AX > 127) || (Crystal_AY < 0) || (Crystal_AY > 127) || 
			(Crystal_BX < 0) || (Crystal_BX > 127) || (Crystal_BY < 0) || (Crystal_BY > 127)) {
			m_BadAddress++;
			continue;
		}

		// address into histograms
		Address_A = (Crystal_AY * 128 * m_Timing_Bins) + (Crystal_AX * m_Timing_Bins) + diff;
		Address_B = (Crystal_BY * 128 * m_Timing_Bins) + (Crystal_BX * m_Timing_Bins) + (m_Timing_Bins - diff);

		// increment counter
		if ((m_Process[Head_A] == 1) && (m_Process[Head_B] == 1)) {
			buf[Head_A][Address_A]++;
			buf[Head_B][Address_B]++;
		}
	}

	// return overall status
	return 0;
}

long Singles_Done(long *buffer[MAX_HEADS], long *DataSize, unsigned long *NumEvents, unsigned long *BadAddress, unsigned long *SyncProb, long Status[MAX_HEADS])
{
	// local variables
	long Head = 0;
	long OverStatus = 0;

	// copy out counters
	*DataSize = m_DataSize;
	*NumEvents = m_NumEvents;
	*BadAddress = m_BadAddress;
	*SyncProb = m_SyncProb;

	// loop through heads
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// if head was processed
		if (m_Process[Head] == 1) {

			// must have data
			if (buf[Head] != NULL) {

				// transfer pointer
				buffer[Head] = buf[Head];

				// clear static pointer (calling program must release buffer now)
				buf[Head] = NULL;
			} else {
				OverStatus = 7;
				Status[Head] = 7;
			}
		}
	}

	// since memory is expected to be freed by calling routine, clear initialization
	m_Initialized = 0;

	// return overall status
	return OverStatus;
}

void Singles_Error(long Error, char *Message)
{
	switch (Error) {

	case 0:
		sprintf(Message, "Success");
		break;

	case 1:
		sprintf(Message, "Invalid Scanner Type");
		break;

	case 2:
		sprintf(Message, "Invalid Data Type");
		break;

	case 3:
		sprintf(Message, "Rebinner Flag Must Be 0 or 1");
		break;

	case 4:
		sprintf(Message, "Rotation Flag Must Be 0 or 1");
		break;

	case 5:
		sprintf(Message, "Process Head Flag Must Be 0 or 1");
		break;

	case 6:
		sprintf(Message, "Insufficient Memory");
		break;

	case 7:
		sprintf(Message, "Empty Memory");
		break;

	case 8:
		sprintf(Message, "Rebinner Not Allowed For Time Difference Histogram");
		break;

	case 9:
		sprintf(Message, "Arguements Not Initialized");
		break;

	case 10:
		sprintf(Message, "Improper Data Type For Processing Routine");
		break;

	default:
		sprintf(Message, "Unknown Error");
		break;
	}
	// return function status
	return;
}
