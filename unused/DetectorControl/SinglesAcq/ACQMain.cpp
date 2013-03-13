//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Singles Acquisition COM Server (Acquisition.exe)
// 
// Name:			ACQMain.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	July 25, 2002
// 
// Description:		This component is the singles acquisition server which acquires 
//					data from the PFT card throught the SinglesAcqDLL.dll and histograms
//					it for retrieval by DUCOM
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

// ACQMain.cpp : Implementation of CACQMain
#include "stdafx.h"
#include "Acquisition.h"
#include "ACQMain.h"
#include "SinglesHistogram.h"
#include "SinglesAcqDLL.h"
#include "DHI_Constants.h"
#include <process.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gantryModelClass.h"

// build based variable
#ifndef ECAT_SCANNER

	CErrorEventSupport* pErrEvtSup;

#endif

/////////////////////////////////////////////////////////////////////////////
// CACQMain

static unsigned long m_NumKEvents;
static unsigned long m_BadAddress;
static unsigned long m_SyncProb;

static long m_AcquirePercent;
static long m_AcquireFinished;
static long m_AcquireStatus;
static long m_Amount;
static long m_DataSize;
static long m_Scanner;
static long m_StartTime;
static long m_StopTime;
static long m_NumBlocks;
static long m_Timing_Bins;
static long m_Type;
static long m_Mode;
static long m_Rebinner;
static long m_Acquiring;
static long m_SystemRebinner;

static long *m_HeadBuffer[MAX_HEADS];
static long m_HeadPresent[MAX_HEADS];
static long m_HeadStatus[MAX_HEADS];
static long m_Process[MAX_HEADS];

static char *ModeStr[NUM_DHI_MODES+2] = {"Position Profile", "Crystal Energy", "Tube Energy", "Shape Discrimination", "Run", "Trans", "SPECT", "Crystal Time", "Bucket Test", "Timing", "Coincidenc Flood"};

void Acquire_Thread(void *dummy);
long Add_Error(char *Where, char *What, long Why);
void Add_Information(char *Where, char *What);
void Add_Warning(char *Where, char *What, long Why);
void Log_Message(char *message);

FILE *m_LogFile;

/////////////////////////////////////////////////////////////////////////////
// Routine:		FinalConstruct
// Purpose:		Server Initialization
// Arguments:	None
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

HRESULT CACQMain::FinalConstruct()
{
	// local variables
	HRESULT hr = S_OK;

	// if initialization fails, set the return code
	if (Init_Globals() != 0) hr = S_FALSE;

	// return success
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Singles
// Purpose:		allocates memory for histogramming and starts acquisition
// Arguments:	Mode			-	long = acquisition in seconds (0) or KEvents (1)
//				AcquireSelect	-	long = array of flags indicating whether or not a head is selected
//				Type			-	long = type of data being histogrammed
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
//				Amount			-	long = amount of data to collect
//				*pStatus		-	long = specific error code
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
//				14 Nov 03	T Gremillion	Added Bucket Test Mode
//				16 Jan 04	T Gremillion	changed thread start to get 
//											rid of warning on compile
//				12 Jul 04	T Gremillion	if amount is KEvents, it is specified for each head/bucket
//				19 Oct 04	T Gremillion	HRRT run mode - allocate 3x memory to handle both layers
//											individually at same time as handling them as combined.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CACQMain::Acquire_Singles(long Mode, long AcquireSelect[], long Type, long Amount, long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;
	long MaxKEvents = 0;

	long HeadStatus[MAX_HEADS];

	char Str[MAX_STR_LEN];

	// initialize status
	m_Acquiring = 0;
	m_AcquirePercent = 0;
	*pStatus = 0;
	m_NumKEvents = 0;
	m_BadAddress = 0;
	m_SyncProb = 0;


	// Verify Data Mode Argument
	if ((Mode < 0) || (Mode > (NUM_DHI_MODES+1))) {
		*pStatus = Add_Error("Acquire_Singles", "Invalid Argument, Mode", Mode);
		return S_FALSE;
	} 

	// Verify Head Selection(s)
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if (AcquireSelect[i] == 1) {
			if (m_HeadPresent[i] != 1) {
				*pStatus = Add_Error("Acquire_Singles", "Invalid Argument (Not Present), Head", i);
				return S_FALSE;
			}
		}
	}

	// verify acquisition type
	if ((Type != ACQUISITION_SECONDS) && (Type != ACQUISITION_BLOCKS)) {
		*pStatus = Add_Error("Acquire_Singles", "Invalid Argument, Acquisition Type", Type);
		return S_FALSE;
	}

	// amount must simply not be a negative number
	if (Amount < 0) {
		*pStatus = Add_Error("Acquire_Singles", "Invalid Argument, Amount", Amount);
		return S_FALSE;
	}

	// transfer values
	m_Mode = Mode;
	m_Type = Type;
	m_Amount = Amount;
	for (i = 0 ; i < MAX_HEADS ; i++) m_Process[i] = AcquireSelect[i];

	// set items based on data type
	if (*pStatus == 0) {
		switch (Mode) {

		case DHI_MODE_POSITION_PROFILE:
		case DHI_MODE_CRYSTAL_ENERGY:
		case DHI_MODE_TIME:
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
		case DATA_MODE_COINC_FLOOD:
			m_DataSize = 128 * 128;
			if (328 == m_Scanner) m_DataSize *= 3;		// HRRT
			break;

		case DHI_MODE_SPECT:
			m_DataSize = 256 * 128 * 128;
			break;

		// Time Difference
		case DATA_MODE_TIMING:
			m_DataSize = m_Timing_Bins * 128 * 128;
			break;

		// Bucket Test
		case DHI_MODE_TEST:
			m_DataSize = 2 * 256 * 256;
			break;

		// unknown data type
		default:
			*pStatus = Add_Error("Acquire_Singles", "Invalid Argument (Unknown), Mode", Mode);
			return S_FALSE;
			break;
		}
	}

	// allocate memory
	if (*pStatus == 0) {

		// for each possible head
		for (i = 0 ; (i < MAX_HEADS) && (*pStatus == 0) ; i++) {

			// if there is currently a memory pointer, then Free the memory
			if (m_HeadBuffer[i] != NULL) {
				::CoTaskMemFree((void *) m_HeadBuffer[i]);
				m_HeadBuffer[i] = NULL;
			}

			// if it is to be processed
			if (m_Process[i] == 1) {

				// allocate space
				m_HeadBuffer[i] = (long *) ::CoTaskMemAlloc(m_DataSize * sizeof(long));				

				// if insufficient memory
				if (m_HeadBuffer[i] == NULL) {

					// set error
					*pStatus = Add_Error("Acquire_Singles", "Allocating Memory, Requested Size", (m_DataSize * sizeof(long)));

					// release memory from previous heads
					for (j = 0 ; j < i ; j++) {
						if (m_Process[j] == 1) {
							::CoTaskMemFree((void *) m_HeadBuffer[i]);
							m_HeadBuffer[i] = NULL;
						}
					}

				// allocation successful
				} else {

					// zero out memory
					for (j = 0 ; j < m_DataSize ; j++) m_HeadBuffer[i][j] = 0;
				}
			}
		}
	}

	// coincidence data takes 64-bit data
	m_Rebinner = m_SystemRebinner;
	if ((Mode == DATA_MODE_TIMING) || (Mode == DATA_MODE_COINC_FLOOD)) m_Rebinner = 0;

	// if time based
	if (Type == ACQUISITION_SECONDS) {

		// set the start and end times
		time(&m_StartTime);
		m_StopTime = m_StartTime + m_Amount;

		// add to log
		sprintf(Str, "Starting Scan: %d Seconds, Rebinner: %d, Mode: %s", m_Amount, m_Rebinner, ModeStr[Mode]);
		Add_Information("Acquire_Singles", Str);

	// event based
	} else {

		// set the number of events
		for (MaxKEvents = 0, i = 0 ; i < MAX_HEADS ; i++) {
			if (AcquireSelect[i] == 1) MaxKEvents += m_Amount;
		}

		// add to log
		sprintf(Str, "Starting Scan: %d Events, Rebinner: %d, Mode: %s", MaxKEvents * 1024, m_Rebinner, ModeStr[Mode]);
		Add_Information("Acquire_Singles", Str);
	}

	// note the heads selected in the log
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if (AcquireSelect[i] == 1) {
			sprintf(Str, "     Acquiring Head %d", i);
			Add_Information("Acquire_Singles", Str);
		}
	}

	// set up for histograming
	*pStatus = Singles_Start(m_Scanner, Mode, m_Rebinner, MaxKEvents, AcquireSelect, m_HeadBuffer, HeadStatus);

	// start up thread
	if (*pStatus == 0) {

		// reset completion variables
		m_AcquireFinished = 0;

		// start thread
		if (-1 == _beginthread(Acquire_Thread, 0, NULL)) Add_Error("Acquire_Singles", "Failed to Start Thread, Status", *pStatus);

	// singles_start failed
	} else {
		Add_Error("Acquire_Singles", "Failed to Start Scan, Status", *pStatus);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Progress
// Purpose:		reports percent completion of acquisition (time or number of events)
// Arguments:	pPercent		-	long = Percent complete
//				*pStatus		-	long = specific error code
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CACQMain::Progress(long *pPercent, long *pStatus)
{
	// local variables
	long CurrentTime = 0;
	long OverTime = 0;

	HRESULT hr = S_OK;

	// initialize status
	*pStatus = 0;

	// if acquiring data
	if (m_AcquireFinished == 0) {

		// if data is being acquired, then generate percentage
		if (m_Acquiring == 1) {

			// if it is count based, retrieve the counts
			if (m_Type == ACQUISITION_BLOCKS) {

				// retrieve the percentage from the histograming DLL
				m_AcquirePercent = (long) CountPercent();

			// otherwise, it is time based
			} else {

				// get the current time
				time(&CurrentTime);

				// caculate the percentage based on elapsed time
				m_AcquirePercent = (long) (100.0 * ((double) (CurrentTime - m_StartTime) / 
													(double) (m_StopTime - m_StartTime)));

				// abort if the acquisition if exceeding its time limit (by 5 seconds)
				OverTime = CurrentTime - m_StopTime;
				if (OverTime >= 5) {

					// add message to log file
					Add_Warning("Progress", "Scan Exceeeded Stop Time, Excess Seconds", OverTime);

					// Issue Stop to Acquisition DLL
					hr = Abort();
				}
			}

			// limit values
			if (m_AcquirePercent < 1) m_AcquirePercent = 1;
			if (m_AcquirePercent > 99) m_AcquirePercent = 99;

		// otherwise it is initializing
		} else {
			m_AcquirePercent = 0;
		}

	// if the acquisition has completed
	} else {
		m_AcquirePercent = 100;
	}

	// transfer the percentage
	*pPercent = m_AcquirePercent;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Retrieve_Singles
// Purpose:		returns histogrammed data from specified head
// Arguments:	Head		-	long = Detector head
//				*pDataSize	-	long = number of elements in data array
//				**buffer	-	long = histogrammed data array
//				*pStatus		-	long = specific error code
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CACQMain::Retrieve_Singles(long Head, long *pDataSize, long **buffer, long *pStatus)
{
	// local variables
	char Str[MAX_STR_LEN];

	// make sure head is in proper range
	if ((Head < 0)  || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error("Retrieve_Singles", "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// make sure head exists
	if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error("Retrieve_Singles", "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// make sure head was selected
	if (m_Process[Head] != 1) {
		*pStatus = Add_Error("Retrieve_Singles", "Invalid Argument (Not Processed), Head", Head);
		return S_FALSE;
	}

	// copy status
	*pStatus = m_HeadStatus[Head];

	// if successfull
	if (*pStatus == 0) {

		// message to log file
		sprintf(Str, "Head %d - Data Retrieved", Head);
		Add_Information("Retrieve_Singles", Str);

		// set the data size
		*pDataSize = m_DataSize;

		// attach to the memory
		*buffer = m_HeadBuffer[Head];
	}

	// it is now the responsibility of the calling routine to free this memory
	m_HeadBuffer[Head] = NULL;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Init_Globals
// Purpose:		COM Server initialization
// Arguments:	none
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				19 Nov 03	T Gremillion	number of timing bins corrected for ring scanners
//				09 Jun 04	T Gremillion	moved ring scanner log file to service\log
/////////////////////////////////////////////////////////////////////////////

long CACQMain::Init_Globals()
{
	// local variables
	long i = 0;

	char Str[MAX_STR_LEN];
	char LogFile[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// instantiate a Gantry Object Model object
	CGantryModel model;

	// initialize percent completion
	m_AcquireStatus = 0;
	m_AcquirePercent = 100;

	// get the rebinner state
	m_SystemRebinner = 1;
	if (getenv("ACS_REBINNER") != NULL) if (strcmp(getenv("ACS_REBINNER"), "0") == 0) m_SystemRebinner = 0;

	// set gantry model to model number held in register if none held, use the 393
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) if (!model.setModel(393)) return 1;

	// scanner type
	m_Scanner = model.modelNumber();

#ifdef ECAT_SCANNER

	// Open the Log File
	if (getenv("SERVICEDIR") != NULL) {
		sprintf(LogFile, "%s\\Log\\SinglesHistogram.log", getenv("SERVICEDIR"));
	} else {
		sprintf(LogFile, "SinglesHistogram.log");
	}
	m_LogFile = fopen(LogFile, "a+");

		// initialize security
		hr = CoInitializeSecurity(NULL,		// Allow access to everyone
									-1,		// Use default authentication service
								  NULL,		// Use default authorization service
								  NULL,		// Reserved
				RPC_C_AUTHN_LEVEL_NONE,		// Authentication Level=None
		   RPC_C_IMP_LEVEL_IMPERSONATE,		// Impersonation Level= Impersonate
								  NULL,		// Reserved
							 EOAC_NONE,		// Capabilities Flag
								  NULL);	// Reserved
		if (FAILED(hr)) {
			sprintf(Str, "Failed CoInitializeSecurity: HRESULT %X", hr);
			Log_Message(Str);
		}
#else

	// Open the Log File
	if (getenv("LOGFILEDIR") != NULL) {
		sprintf(LogFile, "%s\\SinglesHistogram.log", getenv("LOGFILEDIR"));
	} else {
		sprintf(LogFile, "SinglesHistogram.log");
	}
	m_LogFile = fopen(LogFile, "a+");

	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport(false, true);

	// if error support not establisted, note it in log
	if (pErrEvtSup == NULL) {
		Log_Message("Failed to Create ErrorEventSupport");

	// if it was not fully istantiated, note it in log and release it
	} else if (pErrEvtSup->InitComplete(hr) == false) {
		sprintf(Str, "ErrorEventSupport Failed During Initialization. HRESULT = %X", hr);
		Log_Message(Str);
		delete pErrEvtSup;
		pErrEvtSup = NULL;

	// informational message that SAQ logging was started
	} else {
		Add_Information("Init_Globals", "Error Logging Started");
	}
#endif

	// clear all heads
	for (i = 0 ; i < MAX_HEADS ; i++) m_HeadPresent[i] = 0;

	// number of blocks
	m_NumBlocks = model.blocks();

// ECAT (Ring) Scanners
#ifdef ECAT_SCANNER

	// set head values
	for (i = 0 ; i < model.buckets() ; i++) m_HeadPresent[i] = 1;

	// Number of timing bins
	m_Timing_Bins = 32;
#else

	// head information for panel detector scanners
	pHead Head;		

	// get the data for the next head
	Head = model.headInfo();

	// set head values
	for (i = 0 ; i < model.nHeads() ; i++) m_HeadPresent[Head[i].address] = 1;

	// Number of timing bins
	m_Timing_Bins = model.timingBins();
#endif

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Thread
// Purpose:		Worker thread that starts acquisition and retrieves statistics when done
// Arguments:	dummy		- void
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
//				14 Nov 03	T Gremillion	Made Problem Variables Global
//				15 Apr 04	T Gremillion	added asignment of dummy variable to remove
//											compile warning (per Ellen Pearson)
/////////////////////////////////////////////////////////////////////////////

void Acquire_Thread(void *dummy)
{
	// local variables
	int PresetTime = 0;
	int TimeOut = 10;			// timeout acquisition after 10 seconds with no data

	long i = 0;
	long NotUsed = 0;

	// assign the dummy argument to the unused local variable to prevent a compile warning
	dummy = (void *) &NotUsed;

	// set progress values
	m_AcquirePercent = 0;
	m_AcquireFinished = 0;

	// now that the stop time has been set, we  do a time based percentage complete
	m_Acquiring = 1;

	// if acquisition is time based
	if (m_Type == ACQUISITION_SECONDS) PresetTime = (int) m_Amount;

	// start acquisition
	if (AcquireSingles(PresetTime, (int) m_Rebinner, TimeOut)) {

		// add informational message
		Add_Information("Acquire_Thread", "Acquisition Completed");

		// fetch results if acquisition finished normally
		m_AcquireStatus = Singles_Done(&m_NumKEvents, &m_BadAddress, &m_SyncProb, m_HeadStatus);
	} else {

		// add informational message
		Add_Information("Acquire_Thread", "Acquisition Failed");

		// set a bad status for each head
		for (i = 0 ; i < MAX_HEADS ; i++) m_HeadStatus[i] = -1;
	}

	// Finished
	m_AcquireFinished = 1;
	m_AcquirePercent = 100;

	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Abort
// Purpose:		aborts acquisition in progress
// Arguments:	none
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CACQMain::Abort()
{
	// local variables
	int Status = 0;

	// Issue Stop to Acquisition DLL
	Status = StopAcq ();

	// insert message in log
	if (Status == 0) {
		Add_Warning("Abort", "Scan Aborted, Status", Status);
	} else {
		Add_Error("Abort", "Scan Aborted, Status", Status);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Log_Message
// Purpose:	add message to log file
// Arguments:	message		- char = string to add to log file
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Log_Message(char *message)
{
	// local variables
	char time_string[MAX_STR_LEN];
	time_t ltime;

	// get current time and remove new line character
	time(&ltime);
	sprintf(time_string, "%s", ctime(&ltime));
	time_string[24] = '\0';

	// print to log file and then flush the buffer
	fprintf(m_LogFile, "%s %s\n", time_string, message);
	fflush(m_LogFile);

}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Add_Error
// Purpose:		send an error message to the event handler
//				add it to the log file and store it for recall
// Arguments:	*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
//				*Why		- pertinent integer value
// Returns:		long specific error code
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(char *Where, char *What, long Why)
{
	// local variable
	long error_out = -1;
	unsigned char Message[MAX_STR_LEN];

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// add error to error log
	Log_Message((char *) Message);

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
			Log_Message((char *) Message);
		}
	}
#endif

	// return error code
	return error_out;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Add_Warning
// Purpose:	send an warning message to the event handler
//			and add it to the log file
// Arguments:	*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
//				*Why		- pertinent integer value
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Add_Warning(char *Where, char *What, long Why)
{
	// local variable
	unsigned char Message[MAX_STR_LEN];

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// add error to error log (after insuring logging is turned on)
	Log_Message((char *) Message);

#ifndef ECAT_SCANNER
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_SAQ";
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, SAQ_GENERIC_DEVELOPER_WARNING, Message, RawDataSize, EmptyStr, TRUE);
		if (FAILED(hr)) {
			sprintf((char *) Message, "Error Firing Event to Event Handler.  HRESULT = %X", hr);
			Log_Message((char *) Message);
		}
	}
#endif

	// return error code
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Add_Information
// Purpose:	send an informational message to the event handler
//			and add it to the log
// Arguments:	*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Add_Information(char *Where, char *What)
{
	// local variable
	unsigned char Message[MAX_STR_LEN];

	// build the message
	sprintf((char *) Message, "%s: %s", Where, What);

	// add error to error log (after insuring logging is turned on)
	Log_Message((char *) Message);

#ifndef ECAT_SCANNER
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_SAQ";
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, SAQ_GENERIC_DEVELOPER_INFO, Message, RawDataSize, EmptyStr, TRUE);
		if (FAILED(hr)) {
			sprintf((char *) Message, "Error Firing Event to Event Handler.  HRESULT = %X", hr);
			Log_Message((char *) Message);
		}
	}
#endif

	// done
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		KillSelf
// Purpose:		exit server
// Arguments:	none
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CACQMain::KillSelf()
{
	// put message in log file
	Add_Information("KillSelf", "Forcing Server Shutdown");

	// cause the server to exit
	exit(0);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Statistics
// Purpose:		Return the last set of histogramming statistics to the user
// Arguments:	pNumKEvents		- pointer to unsigned long, number of KEvents (1024) processed
//				pBadAddress		- pointer to unsigned long, number of Events with Bad Addresses
//				pSyncProb		- pointer to unsigned long, number of 64-bit data sync errors
// Returns:		HRESULT
// History:		09 Dec 03	T Gremillion	Created
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CACQMain::Statistics(unsigned long *pNumKEvents, unsigned long *pBadAddress, unsigned long *pSyncProb)
{
	// Copy statistics values
	*pNumKEvents = m_NumKEvents;
	*pBadAddress = m_BadAddress;
	*pSyncProb = m_SyncProb;

	return S_OK;
}
