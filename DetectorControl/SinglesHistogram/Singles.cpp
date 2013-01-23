//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Command Line Singles Histogramming (Singles.exe)
// 
// Name:			Singles.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	June 1, 2002
// 
// Description:		This component provides the capability to histogram singles data
//					from an existing listmode data file
//
// Arguments:		<type>			-	"pp", "en", "te", "shape", "run", "trans", 
//										"spect", "ti", "test", "time", "coinc"
//					<input file>	-	input listmode data file
//					<output root>	-	path and root name of output file(s)
//
// Flags:			-r				-	Rebinner was used
//					-h <head>		-	Process data for <head>
//					-<model>		-	override default scanner model number
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//
// History:		19 Oct 04	T Gremillion	handle databuffer for HRRT combined run mode.
//////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_DATA_BUFFER_SIZE 102400

#include "stdafx.h"
#include "stdio.h"
#include "iostream.h"
#include <string.h>
#include <stdlib.h>
#include "SinglesHistogram.h"

#include "gantryModelClass.h"

// not used on ring based scanners
#ifndef ECAT_SCANNER

#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

CErrorEventSupport* pErrEvtSup;

#endif

/////////////////////////////////////////////////////////////////////////////
// Routine:		flagval
// Purpose:		determines whether or not a particular flag was set ont the command line
// Arguments:	search		-	char = flag to search for
//				argc		-	int = number of command line arguments
//				argv		-	char = array of command line arguments
//				buffer		-	char = command line argument following found flag (output)
// Returns:		long argument where flag was found
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long flagval(char* search, int argc, char* argv[], char buffer[MAX_STR_LEN])
{
	// initialize
	long success = 0;

	// clear the buffer
	strcpy(buffer, "");

	// loop through the command line arguements looking for search string
	for (long i = 1 ; (i < argc) && (success == 0) ; i++) {

		// check arguement
		if (strcmp(argv[i], search) == 0) {

			// copy the next arguement
			if ((i+1) < argc) strcpy(buffer, argv[i+1]);

			// indicate where was found
			success = i;
		}
	}

	// return whether or not the value was found
	return success;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Add_Error
// Purpose:	send an error message to the event handler
//			add it to the log file and store it for recall
// Arguments:	What		-	char = what was the error
//				Why			-	long = pertinent integer information
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(char *What, long Why)
{
	// local variable
	unsigned char Message[MAX_STR_LEN];

	// build the message
	sprintf((char *) Message, "Command Line: %s = %d (decimal) %X (hexadecimal)", What, Why, Why);

	// print error
	cout << Message << endl;

#ifndef ECAT_SCANNER
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_SAQ";
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, SAQ_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);
		if (FAILED(hr)) {
			sprintf((char *) Message, "Error Firing Event to Event Handler.  HRESULT = %X", hr);
			cout << Message << endl;
		}
	}
#endif

	// return error code
	return -1;
}

int main(int argc, char* argv[])
{
	// local variables
	long Status = 0;
	long i = 0;
	long j = 0;
	long Scanner = DEFAULT_SYSTEM_MODEL;
	long NumBlocks = 0;
	long Timing_Bins = 0;
	long Arguement = 0;
	long Type = -1;
	long Rebinner = 0;
	long Head = -1;
	long HeadSet = 0;
	long NumBytes = 0;
	long NumItems = 0;
	long DataSize = 0;
	long BlockCount = 0;
	long Flag = 0;

	long Process[MAX_HEADS];
	long HeadStatus[MAX_HEADS];
	long HeadPresent[MAX_HEADS];
	long *data[MAX_HEADS];

	unsigned long NumEvents = 0;
	unsigned long BadAddress = 0;
	unsigned long SyncProb = 0;
	unsigned long MaxKEvents = 0;

	char Str[MAX_STR_LEN];
	char Filename[MAX_STR_LEN];
	char buffer[MAX_STR_LEN];
	char *TypeStr[NUM_DHI_MODES+2] = {"pp", "en", "te", "shape", "run", "trans", "spect", "ti", "test", "time", "coinc"};
	char *TypeExt[NUM_DHI_MODES+2] = {"pp", "en", "te", "sd", "run", "trans", "spect", "ti", "test", "time", "coinc"};

	unsigned char buf[MAX_DATA_BUFFER_SIZE];

	FILE *fp = NULL;

	// instantiate a Gantry Object Model object
	CGantryModel model;

	// translate all arguments to lower case
	for (i = 1 ; i < argc ; i++) _strlwr(argv[i]);

#ifndef ECAT_SCANNER
	HRESULT hr = S_OK;

	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport;

	// if error support not establisted, note it in log
	if (pErrEvtSup == NULL) {
		cout << "Failed to Create ErrorEventSupport" << endl;

	// if it was not fully istantiated, note it in log and release it
	} else if (pErrEvtSup->InitComplete(hr) == false) {
		cout << "ErrorEventSupport Failed During Initialization. HRESULT = " << hr << endl;
		delete pErrEvtSup;
		pErrEvtSup = NULL;
	}
#endif

	// initialize data pointers to NULL
	for (i = 0 ; i < MAX_HEADS ; i++) data[i] = NULL;

	// if not enough arguements, then display usage
	if (argc < 4) {
		cout << argv[0] << " <Type> <input file> <output root> [-r] [-h <head>] [-P39|-PETSPECT|-HRRT] [-n]" << endl;
		cout << "\t<Type> \t\t\t- pp, en, te, shape, run, trans, spect, time" << endl;
		cout << "\t<input file> \t\t- input listmode data file" << endl;
		cout << "\t<output root> \t\t- root name of file to be written.  Head Number and Extension Will Be Added" << endl;
		cout << "\t[-r] \t\t\t- Rebinner was in use" << endl;
		cout << "\t[-h <head>] \t\t- Head to be histogrammed (-1 means all heads - default)" << endl;
		cout << "\t[-P39|-391|-392|-393|-394|-395|-396|-IIA|-2391|-2392|-2393|-2394|-2395|-2396|-PETSPECT|-311|-HRRT|-328|-924|-980|-1024|-1080] \t- scanner selection (P39 - default)" << endl;
		return Status;
	}

	// Initialize
	for (i = 0 ; i < MAX_HEADS ; i++) {
		Process[i] = 0;
		HeadStatus[i] = 0;
		HeadPresent[i] = 0;
		data[i] = NULL;
	}

	// process flags
	if (flagval("-r",argc,argv,buffer) > 0) Rebinner = 1;
	if (flagval("-311",argc,argv,buffer) > 0) Scanner = 311;
	if (flagval("-PETSPECT",argc,argv,buffer) > 0) Scanner = 311;
	if (flagval("-328",argc,argv,buffer) > 0) Scanner = 328;
	if (flagval("-HRRT",argc,argv,buffer) > 0) Scanner = 328;
	if (flagval("-391",argc,argv,buffer) > 0) Scanner = 391;
	if (flagval("-392",argc,argv,buffer) > 0) Scanner = 392;
	if (flagval("-393",argc,argv,buffer) > 0) Scanner = 393;
	if (flagval("-394",argc,argv,buffer) > 0) Scanner = 394;
	if (flagval("-395",argc,argv,buffer) > 0) Scanner = 395;
	if (flagval("-396",argc,argv,buffer) > 0) Scanner = 396;
	if (flagval("-P39",argc,argv,buffer) > 0) Scanner = 393;
	if (flagval("-2391",argc,argv,buffer) > 0) Scanner = 391;
	if (flagval("-2392",argc,argv,buffer) > 0) Scanner = 392;
	if (flagval("-2393",argc,argv,buffer) > 0) Scanner = 393;
	if (flagval("-2394",argc,argv,buffer) > 0) Scanner = 394;
	if (flagval("-2395",argc,argv,buffer) > 0) Scanner = 395;
	if (flagval("-2396",argc,argv,buffer) > 0) Scanner = 396;
	if (flagval("-IIA",argc,argv,buffer) > 0) Scanner = 2393;
	if (flagval("-924",argc,argv,buffer) > 0) Scanner = 924;
	if (flagval("-980",argc,argv,buffer) > 0) Scanner = 980;
	if (flagval("-1024",argc,argv,buffer) > 0) Scanner = 1024;
	if (flagval("-1080",argc,argv,buffer) > 0) Scanner = 1080;

	// set for scanner
	if (!model.setModel(Scanner)) return Add_Error("Failed To Set Scanner, Model", Scanner);

	// model
	Scanner = model.modelNumber();

#ifndef ECAT_SCANNER
	//Number of timing bins
	pHead HeadInfo;
	Timing_Bins = model.timingBins();

	// get the data for the next head
	HeadInfo = model.headInfo();

	// what heads are present for this model
	for (i = 0 ; i < model.nHeads() ; i++) HeadPresent[HeadInfo[i].address] = 1;
#else
	//Number of timing bins
	Timing_Bins = 32;
	
	// what buckets are present for this model
	for (i = 0 ; i < model.buckets() ; i++) HeadPresent[i] = 1;
#endif

	// Number of addressable blocks per head
	NumBlocks = model.blocks();

	// process any head selections
	while (((Arguement = flagval("-h",argc,argv,buffer)) > 0) && (Status == 0)) {

		// set flag indicating that the flag has been specifically set by the user
		HeadSet = 1;

		// remove that arguement from the rest of the list
		sprintf(argv[Arguement], "-x");

		// assign head
		Head = atol(buffer);

		// if all heads, then switch on by scanner
		// also, Rotation is not valid for PETSPECT & HRRT so make sure it is off
		if (Head == -1) {
			for (i = 0 ; i < MAX_HEADS ; i++) Process[i] = HeadPresent[i];

		// otherwise, just the one head
		} else {

			if ((Head < 0) || (Head >= MAX_HEADS)) {
				Status = Add_Error("Invalid Argument (Out of Range), Head", Head);
			} else if (HeadPresent[Head] != 1) {
				Status = Add_Error("Invalid Argument (Not Present), Head", Head);
			} else {
				Process[Head] = 1;
			}
		}
	}

	// if the head was not set, then set for all heads
	if (HeadSet == 0) for (i = 0 ; i < MAX_HEADS ; i++) Process[i] = HeadPresent[i];

	// determine data type
	for (i = 0 ; i < (NUM_DHI_MODES+2) ; i++) if (strcmp(TypeStr[i], argv[1]) == 0) Type = i;
	if (Type == -1) Status = Add_Error("Invalid Argument, Data Type", Type);

	// rebinner swithch ignored for time difference
	if ((Type == DATA_MODE_TIMING) || (Type == DATA_MODE_COINC_FLOOD)) Rebinner = 0;

	// open the input file
	if (Status == 0) {
		fp = fopen(argv[2], "rb");
		if (fp == NULL) {
			sprintf(Str, "Could Not Open File: %s", argv[2]);
			Status = Add_Error(Str, 0);
		}
	}

	// set items based on data type
	if (Status == 0) {
		switch (Type) {

		case DHI_MODE_POSITION_PROFILE:
		case DHI_MODE_CRYSTAL_ENERGY:
		case DHI_MODE_TIME:
			DataSize = 256 * 256 * NumBlocks;
			break;

		case DHI_MODE_TUBE_ENERGY:
			DataSize = 256 * 4 * NumBlocks;
			break;

		case DHI_MODE_SHAPE_DISCRIMINATION:
			DataSize = 256 * NumBlocks;
			break;

		case DHI_MODE_RUN:
		case DHI_MODE_TRANSMISSION:
		case DATA_MODE_COINC_FLOOD:
			DataSize = 128 * 128;
			break;

		case DHI_MODE_SPECT:
			DataSize = 256 * 128 * 128;
			break;

		// Time Difference
		case DATA_MODE_TIMING:
			DataSize = Timing_Bins * 128 * 128;
			break;
			
			// Test data
		case DHI_MODE_TEST:
			DataSize = 256 * 256 * 2;
			break;

		// unknown data type
		default:
			Status = Add_Error("Invalid Argument, Data Type", Type);
			break;
		}
	}

	// allocate memory
	if (Status == 0) {

		// for each possible head
		for (i = 0 ; (i < MAX_HEADS) && (Status == 0) ; i++) {

			// if it is to be processed
			if (Process[i] == 1) {

				// allocate space
				if ((328 == Scanner) && (DHI_MODE_RUN == Type)) {
					data[i] = (long *) malloc(3 * DataSize * sizeof(long));
				} else {
					data[i] = (long *) malloc(DataSize * sizeof(long));
				}

				// if insufficient memory
				if (data[i] == NULL) {

					// set error
					cout << "Could Not Allocate Memory For Head: " << i << endl;
					Status = Add_Error("Allocating Memory, Requested Size", (DataSize * sizeof(long)));

					// release memory from previous heads
					for (j = 0 ; j < i ; j++) {
						if (Process[j] == 1) {
							free((void *) data[i]);
						}
					}

				// allocation successful
				} else {

					// zero out memory
					for (j = 0 ; j < DataSize ; j++) data[i][j] = 0;
				}
			}
		}
	}

	// set up for histograming
	if (Status == 0) Status = Singles_Start(Scanner, Type, Rebinner, MaxKEvents, Process, data, HeadStatus);

	// successful preparation and input file open
	if (Status == 0) {

		// process data
		while ((Status == 0) && ((NumBytes = fread((void *) buf, sizeof(unsigned char), MAX_DATA_BUFFER_SIZE, fp)) != 0)) {
			cout << "\rBlocks: " << ++BlockCount << flush;
			Status = Histogram_Buffer(buf, NumBytes, Flag);
		}

		// final pass
		Flag = 1;
		NumBytes = 0;
		Status = Histogram_Buffer(buf, NumBytes, Flag);

		// release file
		fclose(fp);
	}

	// fetch results
	if (Status == 0) Status = Singles_Done(&NumEvents, &BadAddress, &SyncProb, HeadStatus);

	// success
	if (Status == 0) {

		// write out processing information
		cout << endl;
		cout << "KEvents: " << (unsigned long) NumEvents << endl;
		cout << "Bad Addresses: " << (unsigned long) BadAddress << endl;
		if (Rebinner == 0) cout << "Sync Errors: " << (unsigned long) SyncProb << endl;

		// write the head data
		for (i = 0 ; i < MAX_HEADS ; i++) if (Process[i] == 1) {

			// must have allocated space
			if (data[i] != NULL) {

				// build filename
				sprintf(Filename, "%s%d.%s", argv[3], i, TypeExt[Type]);

				// open file
				fp = fopen(Filename, "wb");
				if (fp == NULL) {
					cout << "Error Opening Output File: " << Filename << endl;
				} else {

					// write data
					NumItems = fwrite((void *) data[i], sizeof(long), DataSize, fp);
					if (NumItems != DataSize) {
						cout << "Error: Head " << i << " - Words Written: " << NumItems << " Of: " << DataSize << endl;
					}

					// release the file
					fclose(fp);

					// release the memory
					free((void *) data[i]);
					data[i] = NULL;
				}

			// no data buffer
			} else {
				cout << "Error: Head " << i << " Has No Data Buffer" << endl;
			}
		}
	}

	// print out any errors
	if (Status > 0) cout << "Check System Log For Errors" << endl;

	// finished
	return Status;
}
