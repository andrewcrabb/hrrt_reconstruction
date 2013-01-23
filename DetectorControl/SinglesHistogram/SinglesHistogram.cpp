//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Singles Histogramming (SinglesHistogram.dll)
// 
// Name:			SinglesHistogram.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	June 3, 2002 (as singles_histogram.cpp)
//					January 17, 2003 (as SinglesHistogram.cpp)
// 
// Description:		This component provides the capability to histogram singles data
//					according to the originating detector heads and coincidence data
//					according to the component detector heads.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "SinglesHistogram.h"
#include <stdio.h>
#include <stdlib.h>

#include "gantryModelClass.h"

#ifndef ECAT_SCANNER
#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"
#endif

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
static long m_Timing_Bins = 0;
static long m_Timing_Offset = 0;
static long m_Initialized = 0;

static _int64 m_NumEvents = 0;
static unsigned long m_MaxKEvents = 0;
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

#define NUM_XE 64
static long Head_A_xe_HRRT[NUM_XE] = {-1,  0,  0,  0,  0,  0,  1,  1,
									   1,  1,  1,  2,  2,  2,  2,  3,
									   3,  3,  4,  4,  5, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1};
static long Head_B_xe_HRRT[NUM_XE] = {-1,  2,  3,  4,  5,  6,  3,  4,  
									   5,  6,  7,  4,  5,  6,  7,  5,
									   6,  7,  6,  7,  7, -1, -1, -1, 
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1};

static long Head_A_xe_P39[NUM_XE] = {-1,  0,  0,  0,  1,  1,  1,  2,
									  2,  3, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1};
static long Head_B_xe_P39[NUM_XE] = {-1,  2,  3,  4,  3,  4,  5,  4, 
									  5,  5, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1,
									 -1, -1, -1, -1, -1, -1, -1, -1};

static long Head_A_xe_Ring[NUM_XE] = {-1,  0,  0,  0,  0,  0,  0,  0,
							 		   1,  1,  1,  1,  1,  1,  1,  2,
									   2,  2,  2,  2,  2,  2,  3,  3,
									   3,  3,  3,  3,  4,  4,  4,  4,
									   4,  5,  5,  5,  5,  6,  6,  6,
									   7,  7,  8, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1};
static long Head_B_xe_Ring[NUM_XE] = {-1,  3,  4,  5,  6,  7,  8,  9,
							 		   4,  5,  6,  7,  8,  9, 10,  5,
									   6,  7,  8,  9, 10, 11,  6,  7,
									   8,  9, 10, 11,  7,  8,  9, 10,
									  11,  8,  9, 10, 11,  9, 10, 11,
									  10, 11, 11, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1,
									  -1, -1, -1, -1, -1, -1, -1, -1};

long Add_Error(char *Where, char *What, long Why);

/////////////////////////////////////////////////////////////////////////////
// Routine:		Singles_Start
// Purpose:		initialize the DLL for histogramming
// Arguments:	Scanner		-	long = scanner model number (for gantry model)
//				Type		-	long = type of data being histogrammed
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//					8 = Bucket Test
//	`				9 = Time Difference
//					10 = Coincidence Flood
//				Rebinner	-	long = flag whether or not is to be used (present)
//				MaxKEvents	-	long = acquisition by number of events (in 1024 increments)
//				Process		-	long = array specifying heads to process
//				*buffer		-	long = pointers to memory to store histogrammed data
//				Status		-	long = status for each head
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				14 Nov 03	T Gremillion	Added Bucket Test Data
//				19 Nov 03	T Gremillion	corrected number of timing bins
//				15 Apr 04	T Gremillion	uses electronics phase instead of model number for ring scanners
//				19 Oct 04	T Gremillion	Removed unneeded variable m_Datasize
/////////////////////////////////////////////////////////////////////////////

long Singles_Start(long Scanner, long Type, long Rebinner, unsigned long MaxKEvents, long Process[MAX_HEADS], long *buffer[MAX_HEADS], long Status[MAX_HEADS])
{
	// instantiate a Gantry Object Model object
	CGantryModel model;

	// local variables
	long i = 0;
	long OverStatus = 0;
	
	char Subroutine[80] = "Singles_Start";

	// reset global counters and initialization flag
	m_NumEvents = 0;
	m_BadAddress = 0;
	m_SyncProb = 0;
	m_Initialized = 0;
	m_MaxKEvents = MaxKEvents;

	// initialize head status to success
	for (i = 0 ; i < MAX_HEADS ; i++) Status[i] = 0;
	for (i = 0 ; i < MAX_HEADS ; i++) m_Process[i] = 0;

	// transfer memory pointers
	for (i = 0 ; i < MAX_HEADS ; i++) buf[i] = buffer[i];

	// set gantry model to model number passed in (allows the default scanner to be overridden)
	if (!model.setModel(Scanner)) OverStatus = Add_Error(Subroutine, "Scanner Model Not Recognized By Gantry Model DLL, Model", Scanner);

	// global variables
	m_NumBlocks = model.blocks();

// ECAT (Ring) Scanners
#ifdef ECAT_SCANNER

	// only define for phase II electronics
	if (model.electronicsPhase() == 1) OverStatus = Add_Error(Subroutine, "ACS Histogramming Not Define for Electronics Phase", 1);

	// initialize
	m_Scanner = SCANNER_RING;
	for (i = 0 ; i < MAX_HEADS ; i++) head_matrix[i] = i;
	for (i = 0 ; i < MAX_HEADS ; i++) ptsrc_matrix[i] = -1;

	// number of timing bins
	m_Timing_Bins = 32;

	// emission blocks & crystals in each dimension
	m_Rotate = 0;
	m_Blocks_X = model.transBlocks();
	m_Blocks_Y = model.axialBlocks();
	m_Crystals_X = model.angularCrystals();
	m_Crystals_Y = model.axialCrystals();

#else

	// set scanner parameters
	switch (model.modelNumber()) {

	// petspect
	case 311:
		m_Scanner = SCANNER_PETSPECT;
		for (i = 0 ; i < MAX_HEADS ; i++) head_matrix[i] = petspect_matrix[i];
		for (i = 0 ; i < MAX_HEADS ; i++) ptsrc_matrix[i] = -1;
		break;

	// HRRT
	case 328:
		m_Scanner = SCANNER_HRRT;
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
		for (i = 0 ; i < MAX_HEADS ; i++) head_matrix[i] = P39_matrix[i];
		for (i = 0 ; i < MAX_HEADS ; i++) ptsrc_matrix[i] = P39_ps[i];
		break;

	// different flavors of P39 Phase IIA Electronics
	case 2391:
	case 2392:
	case 2393:
	case 2394:
	case 2395:
	case 2396:
		m_Scanner = SCANNER_P39_IIA;
		for (i = 0 ; i < MAX_HEADS ; i++) head_matrix[i] = P39_matrix[i];
		for (i = 0 ; i < MAX_HEADS ; i++) ptsrc_matrix[i] = P39_ps[i];
		break;

	// unknown, return error
	default:
		OverStatus = Add_Error(Subroutine, "Scanner Model Not Recognized As Panel Detector Model, Model", Scanner);
		break;
	}

	// number of timing bins
	m_Timing_Bins = model.timingBins();

	// emission blocks & crystals in each dimension
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
#endif

	// Derivitive values
	m_Total_Crystals_X = m_Blocks_X * m_Crystals_X;
	m_Total_Crystals_Y = m_Blocks_Y * m_Crystals_Y;
	m_Timing_Offset = m_Timing_Bins / 2;

	// Transfer Type
	m_Type = Type;

	// Verify Rebinner State
	if (OverStatus == 0) {
		if ((Rebinner < 0) || (Rebinner > 1)) {
			OverStatus = Add_Error(Subroutine, "Invalid Argument, Rebinner", Rebinner);
		} else {
			m_Rebinner = Rebinner;
		}
	}

	// Rebinner not allowed for time histogram
	if ((m_Type == DATA_MODE_TIMING) || (m_Type == DATA_MODE_COINC_FLOOD)) m_Rebinner = 0;

	// Verify Head Flags
	for (i = 0 ; (i < MAX_HEADS) && (OverStatus == 0) ; i++) {
		if ((Process[i] < 0) || (Process[i] > 1)) {
			OverStatus = Add_Error(Subroutine, "Invalid Argument, Process[Head]", Process[i]);
			Status[i] = OverStatus;
		} else {
			m_Process[i] = Process[i];
		}
	}

	// if no errors, then set to initialized
	if (OverStatus == 0) m_Initialized = 1;

	// return overall status
	return OverStatus;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Singles_Histogram
// Purpose:		does the histogramming for the singles data types
// Arguments:	*buffer		-	unsigned char = listmode data
//				NumBytes	-	long = number of bytes in buffer
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	Changes made for ring scanners
//											Head Number is shifted right by 1
//											Block number is in x & y nibbles
//											Crystal Time is organized more like run mode
//											tube 4 is valid as the area outside the tubes
//				14 Nov 03	T Gremillion	Added Bucket Test Data Type
//				09 Jan 04	T Gremillion	Added check for invalid crystal to ring
//											TDC time processing
//				19 Oct 04	T Gremillion	split out layer data for HRRT run mode.
//											prevent reading beyond end of buffer
//				14 Mar 05	T Gremillion	corrected layer for HRRT
/////////////////////////////////////////////////////////////////////////////

long Singles_Histogram(unsigned char *buffer, long NumBytes)
{
	// local variables
	long i = 0;
	long curr = 0;
	long step = 0;
	long Head = 0;
	long Channel = 0;
	long Block = 0;
	long X = 0;
	long Y = 0;
	long Tube = 0;
	long Bin = 0;
	long Temp = 0;
	long Address = 0;
	long HeadEntry = 0;
	long Layer = 0;

	double Energy_Time_A = 0.0;
	double Energy_Time_B = 0.0;
	double Ratio = 0.0;

	unsigned char local[4];

	// if not initialized, return failure
	if (m_Initialized != 1) return 9;

	// if Time Difference Data, then wrong mode
	if ((m_Type == DATA_MODE_TIMING) || (m_Type == DATA_MODE_COINC_FLOOD)) return 10;

	// determine the step between events
	step = (m_Rebinner == 1) ? 4 : 8;

	// loop through the data
	for (curr = 0 ; curr <= (NumBytes - step) ; curr += step) {

		// for 64 bit data, sync up
		while ((m_Rebinner == 0) && ((buffer[curr+7] & 0x80) != 0x80)) {
			m_SyncProb++;
			curr += 4;
			if ((curr+8) > NumBytes) break;
		}

		// if data will go beyond end of buffer, skip it
		if ((curr+step) > NumBytes) break;

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

		// break out the head entry
		if (SCANNER_RING == m_Scanner) {
			HeadEntry = (long)(local[3] >> 1);
		} else {
			HeadEntry = (long)local[3];
		}

		// Determine Head (needed for all)
		for (i = 0, Head = -1; (i < MAX_HEADS) && (Head == -1) ; i++) {
			if ((HeadEntry == head_matrix[i]) || (HeadEntry == ptsrc_matrix[i])) Head = i;
		}

		// bad head numer - increment bad count and continue to next event
		if (Head == -1) {
			m_BadAddress++;
			continue;
		}

		// Head Not being processed - continue to next event
		if (m_Process[Head] == 0) continue;

		// Block Address Needed for these data types
		if ((m_Type == DHI_MODE_POSITION_PROFILE) || (m_Type == DHI_MODE_CRYSTAL_ENERGY) ||	(m_Type == DHI_MODE_TIME) ||
			(m_Type == DHI_MODE_TUBE_ENERGY) || (m_Type == DHI_MODE_SHAPE_DISCRIMINATION) || (m_Type == DHI_MODE_TEST)) {

			// Break out block number
			if (SCANNER_RING == m_Scanner) {
				X = (long)(local[0] & 0x03);
				Y = (long)((local[0] & 0x30) >> 4);
				Block = (m_Blocks_Y * X) + Y;
			} else {
				Block = (long)((local[2] >> 1) & 0x7f);
			}
			if ((Block < 0) || (Block >= m_NumBlocks)) {
				m_BadAddress++;
				continue;
			}
		}

		// ring scanner data ordered differently than panel detector scanners
		if (SCANNER_RING == m_Scanner) {

			// data type specific
			switch (m_Type) {

			// crystal time accessed by crystal x/y and bin
			case DHI_MODE_TIME:

				// extract data
				X = (long) (local[0] & 0x3f);
				Y = (long) (local[2] & 0x3f);
				Bin = (long) local[1];

				// check for invalid crystal
				if ((X < 0) || (X >= (m_Blocks_X * m_Crystals_X)) ||
					(Y < 0) || (Y >= (m_Blocks_Y * m_Crystals_Y))) {
					m_BadAddress++;
					continue;
				}

				// reformat for storage
				Block = (Y / m_Crystals_Y) + ((X / m_Crystals_X) * m_Blocks_Y);
				Y = ((Y % m_Crystals_Y) * 16) + (X % m_Crystals_X);

				// Calculate Address
				Address = (Block * 256 * 256) + (Y * 256) + Bin;
				break;

			// position profile accessed by block & X/Y coordinates
			// crystal energy accessed by crystal and bin
			case DHI_MODE_POSITION_PROFILE:
			case DHI_MODE_CRYSTAL_ENERGY:
				X = (long) local[1];			// Bin
				Y = (long) local[2];			// Crystal
				Address = (Block * 256 * 256) + (Y * 256) + X;
				break;

			// test data has two 256 data fields and a channel
			case DHI_MODE_TEST:
				X = (long) local[1];
				Y = (long) local[2];
				Channel = (long) (local[3] & 0x01);
				Address = (Channel * 256 * 256) + (Y * 256) + X;
				break;

			// tube energy accessed by block, tube and bin
			case DHI_MODE_TUBE_ENERGY:
				Bin = (long) local[1];
				Tube = (long) local[2];

				// tubes outside of range (4 is area outside of tube areas)
				if (Tube > 3) {
					if (4 != Tube) m_BadAddress++;
					continue;
				}
				Address = (Block * 4 * 256) + (Tube * 256) + Bin;
				break;

			// Run and Transmission data are accessed by Crystal X/Y Location
			case DHI_MODE_RUN:

				// coordinates
				X = (long) (local[0] & 0x3f);
				Y = (long) (local[2] & 0x3f);
				Address = (Y * 128) + X;
				break;

			default:
				m_BadAddress++;
				continue;
				break;
			}

		// panel detector scanners
		} else {

			// data type specific
			switch (m_Type) {

			// position profile accessed by block & X/Y coordinates
			// crystal energy accessed by crystal and bin
			case DHI_MODE_POSITION_PROFILE:
			case DHI_MODE_CRYSTAL_ENERGY:
			case DHI_MODE_TIME:
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
				Layer = 2 - (((long) (local[1] & 0x80)) >> 7);

				// leave transmission detector events alone
				if ((long)local[3] != ptsrc_matrix[Head]) {
					if (m_Rotate == 1) {
						Temp = Y;
						Y = m_Total_Crystals_Y - X - 1;
						X = Temp;
					}
					if ((X < 0) || (X > 127) || (Y < 0) || (Y > 127)) {
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

			default:
				m_BadAddress++;
				continue;
				break;
			}
		}

		// increment counter
		buf[Head][Address]++;
		if ((SCANNER_HRRT == m_Scanner) && (DHI_MODE_RUN == m_Type)) buf[Head][(Layer*128*128)+Address]++;
	}

	// return overall status
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Coinc_Histogram
// Purpose:		does the histogramming for the coincidence data types
// Arguments:	*buffer		-	unsigned char = listmode data
//				NumBytes	-	long = number of bytes in buffer
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				19 Nov 03	T Gremillion	Corrected bit pattern for ring scanners
//				09 Dec 03	T Gremillion	Changed to do randoms subtraction.
//				19 Oct 04	T Gremillion	Prevent reading beyond end of buffer
/////////////////////////////////////////////////////////////////////////////

long Coinc_Histogram(unsigned char *buffer, long NumBytes)
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
	if ((m_Type != DATA_MODE_TIMING) && (m_Type != DATA_MODE_COINC_FLOOD)) return 10;
	
	// loop through the data
	for (index = 0 ; index <= (NumBytes - 8) ; index += 8) {

		// for 64 bit data, sync up
		while ((buffer[index+7] & 0x80) != 0x80) {
			m_SyncProb++;
			index += 4;
			if ((index+8) > NumBytes) break;
		}

		// if data will go beyond end of buffer, skip it
		if ((index+8) > NumBytes) break;

		// if it is a tag word, then skip it
		if ((buffer[index+3] & 0x40) == 0x40) continue;

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
		case SCANNER_P39_IIA:

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

			// assign always as prompt
			prompt = 1;

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

		case SCANNER_RING:

			// check if prompt
			prompt = (((long) buffer[index+7] & 0x40) == 0x40) ? 1 : 0;

			// id crystals
			Crystal_AX = (long) buffer[index+0] & 0x7f;
			Crystal_AY = (long) buffer[index+1] & 0x7f;
			Crystal_BX = (long) buffer[index+4] & 0x7f;
			Crystal_BY = (long) buffer[index+5] & 0x7f;

			// module pair
			xe = (((long) buffer[index+6] & 0x07) << 3) + ((long) buffer[index+2] & 0x07);
			Head_A = Head_A_xe_Ring[xe];
			Head_B = Head_B_xe_Ring[xe];

			// break out time bin and shift it so that it peak forms in middle
			diff = (((long) buffer[index+7] & 0x06) << 2) + (((long) buffer[index+3] & 0x0e) >> 1) + m_Timing_Offset;
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
		if (m_Type == DATA_MODE_TIMING) {
			Address_A = (Crystal_AY * 128 * m_Timing_Bins) + (Crystal_AX * m_Timing_Bins) + diff;
			Address_B = (Crystal_BY * 128 * m_Timing_Bins) + (Crystal_BX * m_Timing_Bins) + (m_Timing_Bins - diff);
		} else {
			Address_A = (Crystal_AY * 128) + Crystal_AX;
			Address_B = (Crystal_BY * 128) + Crystal_BX;
		}

		// increment counter if prompt
		if (prompt) {
			if (m_Process[Head_A] == 1) buf[Head_A][Address_A]++;
			if (m_Process[Head_B] == 1) buf[Head_B][Address_B]++;

		// decrement if random
		} else {
			if (m_Process[Head_A] == 1) buf[Head_A][Address_A]--;
			if (m_Process[Head_B] == 1) buf[Head_B][Address_B]--;
		}
	}

	// return overall status
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Singles_Done
// Purpose:		returns the statistics and status after completion
// Arguments:	NumKEvents		-	unsigned long = thousands of events processed
//				BadAddress		-	unsigned long = number of bad events
//				SyncProb		-	unsigned long = number of sync problems in 64-bit data
//				Status			-	long = array of statsu by head
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Singles_Done(unsigned long *NumKEvents, unsigned long *BadAddress, unsigned long *SyncProb, long Status[MAX_HEADS])
{
	// local variables
	long OverStatus = 0;

	// copy out counters
	*NumKEvents = (unsigned long) (m_NumEvents / 1024);
	*BadAddress = m_BadAddress;
	*SyncProb = m_SyncProb;

	// clear initialization
	m_Initialized = 0;

	// return overall status
	return OverStatus;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Histogram_Buffer
// Purpose:		provides the switching between singles and coincidence histogramming
// Arguments:	*buffer		-	unsigned char = listmode data
//				NumBytes	-	long = number of bytes in buffer
//				Flag		-	long = unused
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Histogram_Buffer(unsigned char *buffer, long NumBytes, long Flag)
{
	// long local variables
	long Status = 0;

	// different routine for histogramming timing information
	if ((m_Type == DATA_MODE_TIMING) || (m_Type == DATA_MODE_COINC_FLOOD)) {
		Status = Coinc_Histogram(buffer, NumBytes);
	} else {
		Status = Singles_Histogram(buffer, NumBytes);
	}

	// only status of 0 or 1 recognized
	if (Status != 0) Status = 1;

	// if maximum number of events set and has been exceeded, then signal a stop
	if ((m_MaxKEvents > 0) && (m_NumEvents > (m_MaxKEvents * 1024))) Status = 1;

	// return Status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		CountPercent
// Purpose:		returns the percent complete of acquisition by KEvents
// Arguments:	none
// Returns:		double percent complete
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

double CountPercent()
{
	// local variables
	double Value = 100.0;

	// avoid a divide by zero
	if (m_MaxKEvents == 0) return Value;

	// calculate
	Value = 100.0 * ((double) m_NumEvents / (double)(m_MaxKEvents * 1024));

	// restrict the value
	if (Value < 0.0) Value = 0.0;
	if (Value > 100.0) Value = 100.0;

	// return the percentage
	return Value;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Add_Error
// Purpose:		send an error message to the event handler
//				put it up in a message box
// Arguments:	*Where		-	char = what subroutine did error occur in
//				*What		-	char = what was the error
//				*Why		-	long = error specific integer
// Returns:		long specific error code
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(char *Where, char *What, long Why)
{
	// local variable
	long error_out = -1;
#ifndef ECAT_SCANNER
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_SAQ";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	CErrorEventSupport *pErrEvtSup;

	// pointer to event handler
	pErrEvtSup = new CErrorEventSupport(false, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSup->InitComplete(hr) == false) {
		delete pErrEvtSup;
		pErrEvtSup = NULL;
	}

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, DSU_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);
		delete pErrEvtSup;
	}
#endif

	// return error code
	return error_out;
}

