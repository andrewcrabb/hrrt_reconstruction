//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Detector Abstraction Layer (DHICOM.exe)
// 
// Name:			Abstract_DHI.cpp - Implementation of CAbstract_DHI
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	April 4, 2002
// 
// Description:		This component is the Detector Abstraction Layar which is intended to
// 					provide a common interface to detector electronics
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DHICom.h"
#include "Abstract_DHI.h"
#include "gantryModelClass.h"

#include <sys/types.h>
#include <sys/stat.h>

#define DAL 0
#define CP 1
#define DHI 2

#define REVISION "1.0"

char *Month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char *CPModeStr[NUM_CP_MODES] = {"COINC", "PASS", "TAG", "TEST", "TIME"};
char *DHIModeStr[NUM_DHI_MODES] = {"pp", "en", "te", "sd", "run", "trans", "spect", "time", "test"};
char *LayerStr[NUM_LAYER_TYPES] = {"fast", "slow", ""};
char *LayerDash[NUM_LAYER_TYPES] = {" -", " -", ""};
char *SetTitle[MAX_SETTINGS] = { " pmta", " pmtb", " pmtc", " pmtd", "  cfd", "delay", 
								"off_x", "off_y", "off_e", " mode", "  KeV", 
								"shape", "  low", " high", "  low", " high",
								"c_set", "tgain", "t_off", "t_set", "cfd_0", 
								"cfd_1", "pkstd", "photo", " temp"};
char *SetCmd[MAX_SETTINGS] = {"pmta", "pmtb", "pmtc", "pmtd", "cfd", "cfd_del", 
								"x_offset", "y_offset", "e_offset", "mode", "setup_eng", 
								"sd", "slow_low", "slow_high", "fast_low", "fast_high", 
								"cfd_setup", "tdc_gain", "tdc_offset", "tdc_setup", 
								"cfd_eng0", "cfd_eng1", "pk_stdev", "photopeak", "setup_temp"};
long SettingNum[MAX_SETTINGS];
long OutOrder[MAX_SETTINGS];

// variables defined based on build
#ifndef ECAT_SCANNER

long ErrCode[3] = {DAL_GENERIC_DEVELOPER_ERR, DALCP_GENERIC_DEVELOPER_ERR, DALDHI_GENERIC_DEVELOPER_ERR};
long WarnCode[3] = {DAL_GENERIC_DEVELOPER_WARNING, DALCP_GENERIC_DEVELOPER_WARNING, DALDHI_GENERIC_DEVELOPER_WARNING};
long InfoCode[3] = {DAL_GENERIC_DEVELOPER_INFO, DALCP_GENERIC_DEVELOPER_INFO, DALDHI_GENERIC_DEVELOPER_INFO};

#endif

long m_LogState;

FILE *m_LogFile;

wchar_t m_Coincidence_Processor_Name[MAX_STR_LEN];

char m_RootPath[MAX_STR_LEN];
char m_LastSerialMsg[MAX_STR_LEN];
char m_SerialCommand[MAX_STR_LEN];
char m_LastAsyncMsg[MAX_STR_LEN];
long m_SerialReadSyncFlag;
long m_SerialWriteFlag;
long m_SerialAsyncFlag;

void SerialThread(void *dummy);

void Log_Message(char *message);

/////////////////////////////////////////////////////////////////////////////
// CAbstract_DHI

/////////////////////////////////////////////////////////////////////////////
// Routine:		FinalConstruct
// Purpose:		Server Initialization
// Arguments:	None
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	Not persistant for HRRT
/////////////////////////////////////////////////////////////////////////////

HRESULT CAbstract_DHI::FinalConstruct()
{
	// local variables
	HRESULT hr = S_OK;
	char filename[MAX_STR_LEN];
	FILE *fp = NULL;

	// start off logging messages
	m_LogState = 1;

	// initialize settings identifiers
	if (Init_Globals() != 0) {

		// add error message
		Log_Message("Abstract_DHI Server Not Initialized");

		// set the return status
		hr = S_FALSE;

	// add a reference to always keep server up
	} else {

		// don't do it for ring scanners or HRRT
		if ((SCANNER_RING != m_ScannerType) && (SCANNER_HRRT != m_ScannerType)) AddRef();
	}

	// turn off log messages
	m_LogState = 0;

	// for the HRRT, load previous state
	if (SCANNER_HRRT == m_ScannerType) {

		// open the persistant values file
		sprintf(filename, "%s\\DAL_Persistant.bin", m_RootPath);
		if ((fp = fopen(filename, "rb")) != NULL) {

			// write the values
			fread((void *) &m_LogState, sizeof(m_LogState), 1, fp);
			fread((void *) m_LastBlock, sizeof(long), MAX_HEADS, fp);
			fread((void *) m_LastMode, sizeof(long), MAX_HEADS, fp);

			// close the file
			fclose(fp);
		}
	}

	// return success
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Force_Reload
// Purpose:		Forces Analog Settings to be reloaded into memory and equations
//				to be reloaded on head
// Arguments:	None
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Force_Reload()
{
	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) return S_FALSE;

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Head = 0;
	long Config = 0;

	// change the last block set to one that can't occur to force a reload with the next equations
	// also, the analog card settings can no longer be trusted
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {
		m_LastBlock[Head] = -99;
		for (Config = 0 ; Config < MAX_CONFIGS ; Config++) m_Trust[(Config*MAX_HEADS)+Head] = 0;
	}

#endif

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Error_Lookup
// Purpose:		Retrieve the specified error's type and message
// Arguments:	ErrorCode		-	long = Error code to be looked up
//				*Code			-	long = Type of error
//				*ErrorString	-	unsigned char = Error descriptor
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Error_Lookup(long ErrorCode, long *Code, unsigned char ErrorString[])
{
	// verify error code
	if ((ErrorCode < 0) || (ErrorCode >= MAX_ERRORS)) {
		sprintf((char *) ErrorString, "Error Code Out of Range");
		*Code = UNKNOWN_CODE_PROBLEM;

	// valid code, fill in value
	} else {
		sprintf((char *) ErrorString, "%s", m_ErrorArray[ErrorCode]);
		*Code = m_ErrorCode[ErrorCode];
	}

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////
// Routine:		Zap
// Purpose:		Reset to default settings
// Arguments:	ZapMode			-	long = values to be reset
//					0 = All
//					1 = Analog Settings
//					2 = Time
//					3 = Energy
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Block			-	long = Block(s)
//					-1 = All Blocks
//					ohterwise = specific block
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				14 Apr 04	T Gremillion	Activated for Ring scanners (Time corrections)
///////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Zap(long ZapMode, long Configuration, long Head, long Block, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Zap";
	HRESULT hr = S_OK;

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

// block code unused for this build
#ifdef ECAT_SCANNER

	// only for time
	if (ZapMode != ZAP_TIME) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, ZapMode", ZapMode);
		return S_FALSE;
	}

	// must be for all blocks
	if (Block != ALL_BLOCKS) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// retrieve the peak locations
	if (GCI_SUCCESS != zero_bucket_time(Head)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::zero_bucket_time, Bucket", Head);
	}

#else

	// local variables
	long Percent = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char Str[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	if ((ZapMode < 0) || (ZapMode >= NUM_ZAP_TYPES)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, ZapMode", ZapMode);
		return S_FALSE;
	}

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify Block
	if ((Block < -1) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// if not simulation, issue command
	if (m_Port != PORT_SIMULATION) {

		// build the zap command
		sprintf(command, "Z %d %d %d", ZapMode, Configuration, Block);

		// issue command
		*pStatus = Internal_Pass(Head, RETRY, command, response);

		// wait for completion
		for (Percent = 0 ; (Percent < 100) && SUCCEEDED(hr) && (*pStatus == 0) ;) {
			hr = Progress(Head, &Percent, pStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);
			if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
		}

	// simulation, wait appropriate amount of time
	} else {

		// time remaining for previous command
		time(&CurrTime);
		RemainingTime = m_SimulatedFinish[Head] - CurrTime;

		// error if background task is already being executed
		if (RemainingTime > 0) {
			sprintf(Str, "Simulated Background Command In Progress, Head = %d, Remaining Time", Head);
			*pStatus = Add_Error(DAL, Subroutine, Str, RemainingTime);

		// command executed
		} else {

			// kick simulated head out of mode
			m_SimulatedMode[Head] = -1;

			// all blocks - wait 5 seconds
			if (Block == -1) {
				Sleep(5000);

			// single block - wait 1 second
			} else {
				Sleep(1000);
			}
		}
	}

	// if successful and zap affected them, then indicate that stored settings are not to be trusted
	if (((ZapMode == ZAP_ALL) || (ZapMode == ZAP_ANALOG_SETTINGS))) {
		m_Trust[(Configuration*MAX_HEADS)+Head] = 0;
	}

#endif

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Determine_Delay
// Purpose:		Have the DHI firmware calculate the preliminary CFD Delay for the HRRT
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Block			-	long = Block(s)
//					-1 = All Blocks
//					ohterwise = specific block
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Determine_Delay(long Configuration, long Head, long Block, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Zap";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long save_error = 0;
	long code = 0;
	long loop_head = 0;
	long start_head = 0;
	long end_head = 0;
	long Percent = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char Str[MAX_STR_LEN];
	char message[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// Only for HRRT
	if (m_ScannerType != SCANNER_HRRT) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < -1) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != -1) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify Block
	if ((Block < -1) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// determine start and end heads
	start_head = (Head < 0) ? 0 : Head;
	end_head = (Head < 0) ? (m_NumHeads-1) : Head;

	// if not simulation, issue command
	if (m_Port != PORT_SIMULATION) {

		// build command
		if (Block == -1) {
			sprintf(command, "X 1 %d", Configuration);
		} else {
			sprintf(command, "X 1 %d %d", Configuration, Block);
		}

		// issue the command
		for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {
			*pStatus = Internal_Pass(loop_head, RETRY, command, response);
		}

		// loop for progress
		for (Percent = 0 ; (Percent < 100) && (*pStatus == 0) ; ) {

			// issue command
			hr = Progress(Head, &Percent, pStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);

			// values generated out of range is not really an error
			if (*pStatus != 0) {
				hr = Error_Lookup(*pStatus, &code, (unsigned char *) message);
				if (FAILED(hr)) {
					*pStatus = Add_Error(DAL, Subroutine, "Method Failed (Error_Lookup)", hr);
				} else if (code == E_DELAY_OOR) {
					save_error = *pStatus;
					*pStatus = 0;
				}
			}

			// wait 1 second between polling
			if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
		}

	// simulation, wait appropriate amount of time
	} else {

		// loop through heads
		for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {

			// time remaining for previous command
			time(&CurrTime);
			RemainingTime = m_SimulatedFinish[loop_head] - CurrTime;

			// error if background task is already being executed
			if (RemainingTime > 0) {
				sprintf(Str, "Simulated Background Command In Progress, Head = %d, Remaining Time", loop_head);
				*pStatus = Add_Error(DAL, Subroutine, Str, RemainingTime);

			// command executed
			} else {

				// all blocks - wait 5 seconds
				if (Block == -1) {
					Sleep(5000);

				// single block - wait 1 second
				} else {
					Sleep(1000);
				}
			}
		}
	}

	// the analog settings can no longer be trusted
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) {
		if (m_HeadPresent[loop_head] == 1) m_Trust[(Configuration*MAX_HEADS)+loop_head] = 0;
	}

	// if no other errors were set, then return the stored error (no error if none)
	if (*pStatus == 0) *pStatus = save_error;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Determine_Offsets
// Purpose:		Have the DHI firmware calculate
//				X, Y & Energy Offset:	PETSPECT & Phase I P39
//				X & Y Offset:			HRRT
//				TDC Values:				Phase IIA P39
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Block			-	long = Block(s)				
//					-1 = All Blocks
//					ohterwise = specific block
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Determine_Offsets(long Configuration, long Head, long Block, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Determine_Offsets";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long save_error = 0;
	long code = 0;
	long loop_head = 0;
	long start_head = 0;
	long end_head = 0;
	long Percent = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char Str[MAX_STR_LEN];
	char message[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < -1) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != -1) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify Block
	if ((Block < -1) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// determine start and end heads
	start_head = (Head < 0) ? 0 : Head;
	end_head = (Head < 0) ? (m_NumHeads-1) : Head;

	// if not simulation, issue command
	if (m_Port != PORT_SIMULATION) {

		// build command
		if (Block == -1) {
			sprintf(command, "X 0 %d", Configuration);
		} else {
			sprintf(command, "X 0 %d %d", Configuration, Block);
		}

		// issue the command
		for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {
			*pStatus = Internal_Pass(loop_head, RETRY, command, response);
		}

		// loop for progress
		for (Percent = 0 ; (Percent < 100) && (*pStatus == 0) ; ) {

			// issue command
			hr = Progress(Head, &Percent, pStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);

			// values generated out of range is not really an error
			if (*pStatus != 0) {
				hr = Error_Lookup(*pStatus, &code, (unsigned char *) message);
				if (FAILED(hr)) {
					*pStatus = Add_Error(DAL, Subroutine, "Method Failed (Error_Lookup)", hr);
				} else if (code == E_XY_OFFSET_OOR) {
					save_error = *pStatus;
					*pStatus = 0;
				}
			}

			// wait 1 second between polling
			if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
		}

	// simulation, wait appropriate amount of time
	} else {

		// loop through heads
		for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {

			// time remaining for previous command
			time(&CurrTime);
			RemainingTime = m_SimulatedFinish[loop_head] - CurrTime;

			// error if background task is already being executed
			if (RemainingTime > 0) {
				sprintf(Str, "Simulated Background Command In Progress, Head = %d, Remaining Time", loop_head);
				*pStatus = Add_Error(DAL, Subroutine, Str, RemainingTime);

			// command executed
			} else {

				// all blocks - wait 5 seconds
				if (Block == -1) {
					Sleep(5000);

				// single block - wait 1 second
				} else {
					Sleep(1000);
				}
			}
		}
	}

	// the analog settings can no longer be trusted
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) {
		if (m_HeadPresent[loop_head] == 1) m_Trust[(Configuration*MAX_HEADS)+loop_head] = 0;
	}

	// if no other errors were set, then return the stored error (no error if none)
	if (*pStatus == 0) *pStatus = save_error;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Coincidence_Mode
// Purpose:		Put Coincidence Processor in Coincidence Mode
// Arguments:	Transmission	-	long = flag for whether or not point sources are to be extended
//				ModulePairs		-	long = bitfield for detector head pairs which are allowed to be in coiincidence
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				14 Apr 04	T Gremillion	added disable TOF and set default coincidence window for ring scanners
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Coincidence_Mode(long Transmission, long ModulePairs, long *pStatus)
{
	// name of subroutine and HRESULT regardless of code used
	char Subroutine[80] = "Coincidence_Mode";
	HRESULT hr = S_OK;

// code for ring scanners
#ifdef ECAT_SCANNER

	// not for panel detectors
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Improperly Compiled For Panel Detectors", 0);
		return S_FALSE;
	}

	// put the coincidence processor in coincidence mode
	if (!enable_coinc_mode()) *pStatus = Add_Error(DAL, Subroutine, "PetCTGCIDlll Failed to Put Coincidence Processor in Coincidence", 0);

	// turn off time of flight bits
	if (0 == *pStatus) {
		if (!disable_time_of_flight()) *pStatus = Add_Error(DAL, Subroutine, "PetCTGCIDlll Failed to Disable Time of Flight Bits", 0);
	}

	// set to maximum coincidence window
	if (0 == *pStatus) {
		if (!set_coincidence_window(m_DefaultTimingWindow)) *pStatus = Add_Error(DAL, Subroutine, "PetCTGCIDlll Failed to Set Timing Window, Window", m_DefaultTimingWindow);
	}

// code for panel detectors
#else

	// local variables
	long ErrorStatus = 0;
	long ShiftPair = 0;
	long NotPair = 0xFFFFE001;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char message[MAX_STR_LEN];
	char command[MAX_STR_LEN] = "S 0";
	char response[MAX_STR_LEN];

	// not for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Improperly Compiled For Ring Scanners", 0);
		return S_FALSE;
	}

	// Verify Transmission Flag
	if ((Transmission < 0) || (Transmission > 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Transmission Flag", Transmission);
		return S_FALSE;
	}

	// Verify Module Pairs (for P39 only)
	if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {
		if (((ModulePairs & NotPair) != 0) && (ModulePairs != 0)) {
			*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Module Pairs", ModulePairs);
			return S_FALSE;
		}
	}

	// Save the module pair and transmission information flags
	m_Transmission = Transmission;
	m_ModulePairs = ModulePairs;

	// act on port type
	switch (m_Port) {

	// ethernet - send to DCOM server interface
	case PORT_ETHERNET:

		// informational message to log
		sprintf(message, "m_pCPMain->Set_Mode: Coincidence %d %X", Transmission, ModulePairs);
		Log_Message(message);

		// bit shift module pairs to match up with coincidence processor
		ShiftPair = ModulePairs >> 1;

		// issue command
		hr = m_pCPMain->Set_Mode(CP_MODE_COINC, ShiftPair, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Set_Mode), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line - issue as a pass command
	case PORT_SERIAL:
		*pStatus = Internal_Pass(64, NORETRY, command, response);
		break;

	// simulation or unknown
	case PORT_SIMULATION:
	default:

		// time remaining for previous command
		time(&CurrTime);
		RemainingTime = m_SimulatedFinish[MAX_HEADS] - CurrTime;

		// error if background task is already being executed
		if (RemainingTime > 0) {
			*pStatus = Add_Error(DAL, Subroutine, "Simulated Background Command In Progress, Remaining Time", RemainingTime);

		// command executed
		} else {

			// get the current time and use it as the start time for simulated progress
			m_SimulatedStart[MAX_HEADS] = CurrTime;
			
			// if not already in coincidence
			if (m_Simulated_CP_Mode != CP_MODE_COINC) {
				m_Simulated_Data_Flow = 0;
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 90;
			} else {
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 2;
			}

			// remember mode
			m_Simulated_CP_Mode = CP_MODE_COINC;
		}
		break;
	}

#endif

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Time_Mode
// Purpose:		Put Coincidence Processor in Time Difference Mode
// Arguments:	Transmission	-	long = flag for whether or not point sources are to be extended
//				ModulePairs		-	long = bitfield for detector head pairs which are allowed to be in coiincidence
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				14 Apr 04	T Gremillion	activated for ring scanners
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Time_Mode(long Transmission, long ModulePairs, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Time_Mode";
	HRESULT hr = S_OK;

	// initialize return status
	*pStatus = 0;

// block code unused for this build
#ifdef ECAT_SCANNER

	// not for panel detectors
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Improperly Compiled For Panel Detectors", 0);
		return S_FALSE;
	}

	// put the coincidence processor in coincidence mode
	if (!enable_coinc_mode()) *pStatus = Add_Error(DAL, Subroutine, "PetCTGCIDlll Failed to Put Coincidence Processor in Coincidence", 0);

	// turn on time of flight bits
	if (0 == *pStatus) {
		if (!enable_time_of_flight()) *pStatus = Add_Error(DAL, Subroutine, "PetCTGCIDlll Failed to Enable Time of Flight Bits", 0);
	}

	// set to maximum coincidence window
	if (0 == *pStatus) {
		if (!set_coincidence_window(m_MaxTimingWindow)) *pStatus = Add_Error(DAL, Subroutine, "PetCTGCIDlll Failed to Set Timing Window, Window", m_MaxTimingWindow);
	}
#else

	// local variables
	long ErrorStatus = 0;
	long NotPair = 0xFFFFE001;
	long ShiftPair = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char message[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// Verify Transmission Flag
	if ((Transmission < 0) || (Transmission > 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Transmission Flag", Transmission);
		return S_FALSE;
	}

	// Verify Module Pairs (for P39 only)
	if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {
		if (((ModulePairs & NotPair) != 0) && (ModulePairs != 0)) {
			*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Module Pairs", ModulePairs);
			return S_FALSE;
		}
	}

	// Save the module pair and transmission information flags
	m_Transmission = Transmission;
	m_ModulePairs = ModulePairs;

	// Ethernet Time Mode only defined for P39
	if ((m_Port == PORT_ETHERNET) && (m_ScannerType != SCANNER_P39) && (m_ScannerType != SCANNER_P39_IIA)) {
		*pStatus = Add_Error(DAL, Subroutine, "Etherenet Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// act on port type
	switch (m_Port) {

	// ethernet - send to DCOM server interface
	case PORT_ETHERNET:

		// informational message to log
		sprintf(message, "m_pCPMain->Set_Mode: Timing %d %X", Transmission, ModulePairs);
		Log_Message(message);

		// bit shift module pairs to match up with coincidence processor
		ShiftPair = ModulePairs >> 1;

		// issue command
		hr = m_pCPMain->Set_Mode(CP_MODE_COINC, ShiftPair, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Set_Mode), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line - issue as a pass command
	case PORT_SERIAL:

		// build command (coincidence and time difference are the same on P39)
		if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {
			sprintf(command, "S 0");
		} else {
			sprintf(command, "S 2");
		}

		// execute command
		*pStatus = Internal_Pass(64, NORETRY, command, response);
		break;

	// simulation or unknown
	case PORT_SIMULATION:
	default:

		// time remaining for previous command
		time(&CurrTime);
		RemainingTime = m_SimulatedFinish[MAX_HEADS] - CurrTime;

		// error if background task is already being executed
		if (RemainingTime > 0) {
			*pStatus = Add_Error(DAL, Subroutine, "Simulated Background Command In Progress, Remaining Time", RemainingTime);

		// command executed
		} else {

			// get the current time and use it as the start time for simulated progress
			m_SimulatedStart[MAX_HEADS] = CurrTime;
			
			// if not already in coincidence
			if (((m_Simulated_CP_Mode != CP_MODE_COINC) && ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA))) || 
				((m_Simulated_CP_Mode != CP_MODE_TIME) && ((m_ScannerType != SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)))) {
				m_Simulated_Data_Flow = 0;
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 90;
			} else {
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 2;
			}

			// remember mode
			if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {
				m_Simulated_CP_Mode = CP_MODE_COINC;
			} else {
				m_Simulated_CP_Mode = CP_MODE_TIME;
			}
		}
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		PassThru_Mode
// Purpose:		Put Coincidence Processor in Singles PassThru Mode
// Arguments:	*Head			-	long = array of flags for whether or not the data from a head is to be passed thru
//				*PtSrc			-	long = array of flags for whether or not the data from a point source is to be passed thru
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	allow passthru to work for ring scanners
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::PassThru_Mode(long Head[], long PtSrc[], long *pStatus)
{
	// variables needed regardless of build type
	long i = 0;
	long BitField = 0;
	char Subroutine[80] = "PassThru_Mode";
	char message[MAX_STR_LEN];
	HRESULT hr = S_OK;

// code for ring scanners
#ifdef ECAT_SCANNER

	// not for panel detectors
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Improperly Compiled For Panel Detectors", 0);
		return S_FALSE;
	}

	// informational message to log
	sprintf(message, "PetCTGCIDll::enable_passthru_mode ");

	// verify selections and build bit field
	for (i = 0 ; i < MAX_HEADS ; i++) {

		// head selected
		if (Head[i] == 1) {

			// if it is present, add it to the module pair
			if (m_HeadPresent[i] != 0) {
				sprintf(message, "%s Bucket %d", message, i);
				BitField |= 0x01 << i;

			// otherwise, error
			} else {
				*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", i);
				return S_FALSE;
			}
		}
	}

	// message to log file
	sprintf(message, "%s Bucket Selection Bit Field %X", message, BitField);
	Log_Message(message);

	// put the coincidence processor in coincidence mode
	if (!enable_passthru_mode(BitField)) *pStatus = Add_Error(DAL, Subroutine, "PetCTGCIDll Failed to Put Coincidence Processor in PassThru", BitField);

// code for panel detectors
#else

	// local variables
	long ErrorStatus = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// P39 module pair flags
	long P39_Head[MAX_HEADS] = {0x0010, 0x0080, 0x0200, 0x0002, 0x0008, 0x0040, 0x0000, 0x0000,
								0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
	long P39_PtSrc[MAX_HEADS] = {0x0000, 0x0040, 0x0000, 0x0000, 0x0400, 0x0000, 0x0000, 0x0000,
								 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

	// not for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Improperly Compiled For Ring Scanners", 0);
		return S_FALSE;
	}

	// Ethernet PassThru Mode only defined for P39
	if ((m_Port == PORT_ETHERNET) && (m_ScannerType != SCANNER_P39) && (m_ScannerType != SCANNER_P39_IIA)) {
		*pStatus = Add_Error(DAL, Subroutine, "Ethernet Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// leave control up to the user
	m_Transmission = -1;

	// act on port type
	switch (m_Port) {

	// ethernet - send to DCOM server interface
	case PORT_ETHERNET:

		// informational message to log
		sprintf(message, "m_pCPMain->Set_Mode: PassThru");

		// verify selections and build bit field
		for (i = 0 ; i < MAX_HEADS ; i++) {

			// head selected
			if (Head[i] == 1) {

				// if it is present, add it to the bit field
				if (m_HeadPresent[i] != 0) {
					sprintf(message, "%s Head %d", message, i);
					BitField |= P39_Head[i];

				// otherwise, error
				} else {
					*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", i);
					return S_FALSE;
				}
			}

			// point source selected
			if (PtSrc[i] == 1) {

				// if it is present, add it to the bit field
				if ((m_HeadPresent[i] != 0) && (m_PointSource[i] != 0)) {
					sprintf(message, "%s PtSrc %d", message, i);
					BitField |= P39_PtSrc[i];

				// otherwise, error
				} else {
					if (m_HeadPresent[i] != 0) {
						*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", i);
					} else {
						*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Transmission Source On Head", i);
					}
					return S_FALSE;
				}
			}
		}

		// message to log file
		sprintf(message, "%s Bit Field %X", message, BitField);
		Log_Message(message);

		// issue command
		hr = m_pCPMain->Set_Mode(CP_MODE_PASSTHRU, BitField, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Set_Mode), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line - issue as a pass command
	case PORT_SERIAL:

		// HRRT is always all
		if (m_ScannerType == SCANNER_HRRT) {
			BitField = -1;

		} else {

			// no separation of emission and transmission
			for (i = 0 ; i < MAX_HEADS ; i++) Head[i] += PtSrc[i];

			// determine value
			for (BitField = -2, i = 0 ; (BitField != -1) && (i < MAX_HEADS) ; i++) {
				if (Head[i] != 0) {
					if (BitField == -2) {
						BitField = i;
					} else {
						BitField = -1;
					}
				}
			}
		}

		// build pass command (no second arguement for all heads)
		if (BitField == -1) {
			sprintf(command, "S 1");
		} else {
			sprintf(command, "S 1 %d", BitField);
		}

		// execute command
		*pStatus = Internal_Pass(64, NORETRY, command, response);
		break;

	// simulation or unknown
	case PORT_SIMULATION:
	default:

		// time remaining for previous command
		time(&CurrTime);
		RemainingTime = m_SimulatedFinish[MAX_HEADS] - CurrTime;

		// error if background task is already being executed
		if (RemainingTime > 0) {
			*pStatus = Add_Error(DAL, Subroutine, "Simulated Background Command In Progress, Remaining Time", RemainingTime);

		// command executed
		} else {

			// remember which heads are set
			for (i = 0 ; i < MAX_HEADS ; i++) {
				if ((Head[i] != 0) || (PtSrc[i] != 0)) {
					m_SimulatedPassThru[i] = 1;
				} else {
					m_SimulatedPassThru[i] = 0;
				}
			}

			// get the current time and use it as the start time for simulated progress
			m_SimulatedStart[MAX_HEADS] = CurrTime;

			// if not already in coincidence
			if (m_Simulated_CP_Mode != CP_MODE_PASSTHRU) {
				m_Simulated_Data_Flow = 0;
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 90;
			} else {
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 2;
			}

			// remember mode
			m_Simulated_CP_Mode = CP_MODE_PASSTHRU;
		}
	}

#endif

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		TagWord_Mode
// Purpose:		Put Coincidence Processor in Tagword only Mode
// Arguments:	*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::TagWord_Mode(long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "TagWord_Mode";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;
	long ModulePairs = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char message[MAX_STR_LEN];

	// Ethernet Tagword Mode only defined for P39
	if ((m_Port == PORT_ETHERNET) && (m_ScannerType != SCANNER_P39) && (m_ScannerType != SCANNER_P39_IIA)) {
		*pStatus = Add_Error(DAL, Subroutine, "Ethernet Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// act on port type
	switch (m_Port) {

	// ethernet - send to DCOM server interface
	case PORT_ETHERNET:

		// informational message to log
		sprintf(message, "m_pCPMain->Set_Mode: TagWord");
		Log_Message(message);

		// issue command
		hr = m_pCPMain->Set_Mode(CP_MODE_TAG, ModulePairs, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Set_Mode), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial command to coincidence processor
	case PORT_SERIAL:

		// not yet implemented
		*pStatus = Add_Error(DAL, Subroutine, "Serial Line Method Not Available For This Model, Model", m_ModelNumber);
		break;

	// simulation or unknown
	case PORT_SIMULATION:
	default:

		// time remaining for previous command
		time(&CurrTime);
		RemainingTime = m_SimulatedFinish[MAX_HEADS] - CurrTime;

		// error if background task is already being executed
		if (RemainingTime > 0) {
			*pStatus = Add_Error(DAL, Subroutine, "Simulated Background Command In Progress, Remaining Time", RemainingTime);

		// command executed
		} else {

			// get the current time and use it as the start time for simulated progress
			m_SimulatedStart[MAX_HEADS] = CurrTime;
				
			// if not already in coincidence
			if (m_Simulated_CP_Mode != CP_MODE_TAG) {
				m_Simulated_Data_Flow = 0;
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 90;
			} else {
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 2;
			}

			// remember mode
			m_Simulated_CP_Mode = CP_MODE_TAG;
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Test_Mode
// Purpose:		Put Coincidence Processor in Test Mode
// Arguments:	*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Test_Mode(long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Test_Mode";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;
	long ModulePairs = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char message[MAX_STR_LEN];

	// Ethernet Test Mode only defined for P39
	if ((m_Port == PORT_ETHERNET) && (m_ScannerType != SCANNER_P39) && (m_ScannerType != SCANNER_P39_IIA)) {
		*pStatus = Add_Error(DAL, Subroutine, "Ethernet Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// act on port type
	switch (m_Port) {

	// ethernet - send to DCOM server interface
	case PORT_ETHERNET:

		// informational message to log
		sprintf(message, "m_pCPMain->Set_Mode: Test");
		Log_Message(message);

		// issue command
		hr = m_pCPMain->Set_Mode(CP_MODE_TEST, ModulePairs, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Set_Mode), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);

	// serial command to coincidence processor
	case PORT_SERIAL:

		// not yet implemented
		*pStatus = Add_Error(DAL, Subroutine, "Serial Line Method Not Available For This Model, Model", m_ModelNumber);
		break;

	// simulation or unknown
	case PORT_SIMULATION:
	default:

		// time remaining for previous command
		time(&CurrTime);
		RemainingTime = m_SimulatedFinish[MAX_HEADS] - CurrTime;

		// error if background task is already being executed
		if (RemainingTime > 0) {
			*pStatus = Add_Error(DAL, Subroutine, "Simulated Background Command In Progress, Remaining Time", RemainingTime);

		// command executed
		} else {

			// get the current time and use it as the start time for simulated progress
			m_SimulatedStart[MAX_HEADS] = CurrTime;
				
			// if not already in coincidence
			if (m_Simulated_CP_Mode != CP_MODE_TEST) {
				m_Simulated_Data_Flow = 0;
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 90;
			} else {
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 2;
			}

			// remember mode
			m_Simulated_CP_Mode = CP_MODE_TEST;
		}
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Set_Head_Mode
// Purpose:		Put DHI in indicated data mode
// Arguments:	DataMode			-	long = Data Mode
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined 
//				Block			-	long = Block(s)
//					-1 = All Blocks
//					ohterwise = specific block
//				LLD				-	long = lower level discriminator
//				ULD				-	long = upper level discriminator
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	Corrected loop index for ring scanners
//				12 Nov 03	T Gremillion	when setting head (panel detector) mode,
//											and it is already supposed to be in that
//											mode, verify before preceding
//				19 Oct 04	T Gremillion	for the HRRT, if the head is not supposed
//											to be producing data, verify it.
//				23 Nov 04	T Gremillion	Initialize char *ptr = NULL to avoid compile warning
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Set_Head_Mode(long DataMode, long Configuration, long Head, long Layer, long Block, long LLD, long ULD, long *pStatus)
{
	// variables needed regardless of code built
	long start_head = 0;
	long end_head = 0;
	long loop_head = 0;
	long Counts = 0;
	long i = 0;

	char Subroutine[80] = "Set_Head_Mode";
	char *ptr = NULL;

	HRESULT hr = S_OK;

	// verify head number against full range
	if ((Head < -1) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != -1) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// initialize status
	*pStatus = 0;

	// break out range of heads to set
	start_head = (Head < 0) ? 0 : Head;
	end_head = (Head < 0) ? (m_NumHeads-1) : Head;

// ring scanner build
#ifdef ECAT_SCANNER

	// local varibales
	long gci_code;

	// convert data mode to one gci will recognize
	switch (DataMode) {

	case DHI_MODE_RUN:
		gci_code = POS_SCATTER_PROFILE;
		break;

	case DHI_MODE_POSITION_PROFILE:
		gci_code = POS_PROFILE;
		break;

	case DHI_MODE_TUBE_ENERGY:
		gci_code = TUBE_ENERGY_PROFILE;
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
		gci_code = CRYSTAL_ENERGY_PROFILE;
		break;

	case DHI_MODE_TIME:
		gci_code = TIME_HIST_PROFILE;
		break;

	case DHI_MODE_TEST:
		gci_code = OUTPUT_TEST_MODE;
		break;

	default:
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Data Mode", DataMode);
		return S_FALSE;
	}

	// loop through selected bucket(s)
	for (loop_head = start_head ; loop_head <= end_head ; loop_head++) {
		if (!set_histogram_mode(loop_head, gci_code)) {
			*pStatus = Add_Error(DAL, Subroutine, "Failed: set_histogram_mode, Bucket", loop_head);
			return S_FALSE;
		}
	}

// panel detector scanner build
#else

	// local variables
	long Percent = 0;
	long RemainingTime = 0;

	long Head_Mode = -1;
	long Head_Configuration = -1;
	long Head_Layer = -1;
	long Head_LLD = -1;
	long Head_ULD = -1;

	time_t CurrTime = 0;

	char Str[MAX_STR_LEN];
	char modality[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// verify data mode
	if ((DataMode < 0) || (DataMode >= NUM_DHI_MODES) || (DataMode == DHI_MODE_TEST)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Data Mode", DataMode);
		return S_FALSE;
	}

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify layer
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Layer (Out of Range)", Layer);
		return S_FALSE;
	}

	// verify layer phase II A electronics
	if ((Layer != LAYER_ALL) && (m_ScannerType == SCANNER_P39_IIA)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Layer (Not Present)", Layer);
		return S_FALSE;
	}

	// verify block
	if ((Block < -2) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// verify LLD
	if ((LLD < 0) || (LLD > 1500)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Lower Level Discriminator (Out of Range)", LLD);
		return S_FALSE;
	}

	// verify ULD
	if ((ULD < 0) || (ULD > 1500)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Upper Level Discriminator (Out of Range)", ULD);
		return S_FALSE;
	}

	// verify LLD and ULD against each other
	if (ULD <= LLD) {
		sprintf (response, "Invalid Argument, ULD %d Must Be Higher Than LLD", ULD);
		*pStatus = Add_Error(DAL, Subroutine, response, LLD);
		return S_FALSE;
	}

	// modality
	if (m_ScannerType == SCANNER_PETSPECT) {
		if (Configuration == 0) {
			sprintf(modality, " -pet");
		} else {
			sprintf(modality, " -spect");
		}
	} else {
		sprintf(modality, "");
	}

	// make sure head is still in the last mode it was set to
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {

		// should not have counts if the blocks are supposed to be turned off
		if (SCANNER_HRRT == m_ScannerType) {

			// build command for pulling singles from a head
			sprintf(command, "K");

			// issue the command
			*pStatus = Internal_Pass(loop_head, RETRY, command, response);

			// if successful, process out the singles
			ptr = &response[2];
			if (*pStatus == 0) for (i = 0 ; i < 14 ; i++) Counts = strtol(ptr, &ptr, 0);

			// counts should be 0
			if (((Counts == 0) && (Block != NO_BLOCKS)) ||
				((Counts != 0) && (Block == NO_BLOCKS))) {
				m_LastMode[loop_head] = -99;
				m_LastBlock[loop_head] = -99;
			}
		}

		// if it would be skipped
		if ((Block != m_LastBlock[loop_head]) && (DataMode == m_LastMode[loop_head])) {

			// query the head
			hr = Check_Head_Mode(loop_head, &Head_Mode, &Head_Configuration, &Head_Layer, &Head_LLD, &Head_ULD, pStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Check_Head_Mode), HRESULT", hr);

			// if one of the characteristics don't match, set the saved so it won't be
			if ((Head_Mode != DataMode) || 
				(Head_Configuration != Configuration) || 
				(Head_Layer != Layer) ||
				(Head_LLD != LLD) ||
				(Head_ULD != ULD)) {
				m_LastMode[loop_head] = -99;
				m_LastBlock[loop_head] = -99;
			}
		}
	}

	// set active blocks
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {
		if ((Block != m_LastBlock[loop_head]) || (DataMode != m_LastMode[loop_head])) {

			// if not simulation, issue command
			if (m_Port != PORT_SIMULATION) {

				// build command & if error, return
				sprintf(command, "I %d", Block);
				*pStatus = Internal_Pass(loop_head, NORETRY, command, response);
				if (*pStatus != 0) return S_FALSE;

				// wait until block set command completion & if error, return
				for (Percent = 0 ; (Percent < 100) && SUCCEEDED(hr) ;) {
					hr = Progress(loop_head, &Percent, pStatus);
					if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);
					if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
				}

			// simulation, just wait a sec
			} else {

				// time remaining for previous command
				time(&CurrTime);
				RemainingTime = m_SimulatedFinish[loop_head] - CurrTime;

				// error if background task is already being executed
				if (RemainingTime > 0) {
					sprintf(Str, "Simulated Background Command In Progress, Head = %d, Remaining Time", loop_head);
					*pStatus = Add_Error(DAL, Subroutine, Str, RemainingTime);

				// command executed
				} else {
					Sleep(1000);
				}
			}

			// store the current settings for future comparison
			m_LastBlock[loop_head] = Block;
			m_LastMode[loop_head] = DataMode;
		}
	}

	// build command to set mode
	sprintf(command, "M %d %s%s%s%s %d %d", Configuration, DHIModeStr[DataMode], modality, LayerDash[Layer], LayerStr[Layer], LLD, ULD);

	// build command to set mode - if error, return
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {

		// if not simulation, issue command to head
		if (m_Port != PORT_SIMULATION) {
			*pStatus = Internal_Pass(loop_head, NORETRY, command, response);
			if (*pStatus != 0) return S_FALSE;

		// simulation - register the amount of time to wait
		} else {
			time(&CurrTime);
			m_SimulatedStart[loop_head] = CurrTime;
			m_SimulatedFinish[loop_head] = CurrTime + 20;
			m_SimulatedMode[loop_head] = DataMode;
			m_SimulatedConfig[loop_head] = Configuration;
			m_SimulatedLLD[loop_head] = LLD;
			m_SimulatedULD[loop_head] = ULD;
			m_SimulatedLayer[loop_head] = Layer;
		}
	}

#endif

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Initialize_Scan
// Purpose:		Puts coincidence processor in coincidence mode
//				and heads in run mode with all crystal peaks, time offsets
//				and energy settings set
// Arguments:	Transmission	-	long = flag for whether or not point sources are to be extended
//				ModulePairs		-	long = bitfield for detector head pairs which are allowed to be in coiincidence
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined 
//				LLD				-	long = lower level discriminator
//				ULD				-	long = upper level discriminator
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Initialize_Scan(long Transmission, long ModulePairs, long Configuration, long Layer, long LLD, long ULD, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Initialize_Scan";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	long NumFields = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long set_heads = 0;
	long set_energy = 0;
	long Head_Mode = -1;
	long Head_Configuration = -1;
	long Head_Layer = -1;
	long Head_LLD = -1;
	long Head_ULD = -1;
	long mode = 0;
	long Settings[MAX_BLOCKS * MAX_SETTINGS];
	long NotPair = 0xFFFFE001;
	long Statistics[19];
	char *ptr = NULL;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// Verify Transmission Flag
	if ((Transmission < 0) || (Transmission > 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Transmission Flag", Transmission);
		return S_FALSE;
	}

	// Verify Module Pairs (for P39 only)
	if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {
		if (((ModulePairs & NotPair) != 0) && (ModulePairs != 0)) {
			*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Module Pairs", ModulePairs);
			return S_FALSE;
		}
	}

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify layer
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Layer (Out of Range)", Layer);
		return S_FALSE;
	}

	// verify layer
	if ((Layer != LAYER_ALL) && (m_ScannerType == SCANNER_P39_IIA)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Layer (Not Present)", Layer);
		return S_FALSE;
	}

	// verify LLD
	if ((LLD < 0) || (LLD > 1500)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Lower Level Discriminator (Out of Range)", LLD);
		return S_FALSE;
	}

	// verify ULD
	if ((ULD < 0) || (ULD > 1500)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Upper Level Discriminator (Out of Range)", ULD);
		return S_FALSE;
	}

	// verify LLD and ULD against each other
	if (ULD <= LLD) {
		sprintf (response, "Invalid Argument, ULD %d Must Be Higher Than LLD", ULD);
		*pStatus = Add_Error(DAL, Subroutine, response, LLD);
		return S_FALSE;
	}

	// head mode is based on whether or not transmission source is in use
	mode = (Transmission == 0) ? DHI_MODE_RUN : DHI_MODE_TRANSMISSION;

	// put the coincidence processor in coincidence mode - this has the potential for taking the longest
	hr = Coincidence_Mode(Transmission, ModulePairs, pStatus);
	if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Coincidence_Mode), HRESULT", hr);

	// check the analog card settings to make sure they are in the correct energy mode
	for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) && SUCCEEDED(hr) ; Head++) if (m_HeadPresent[Head] == 1) {

		// get the settings for the head/configuration
		hr = Get_Analog_Settings(Configuration, Head, Settings, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Get_Analog_Settings), HRESULT", hr);

		// if successful, check setting
		set_energy = 0;
		if (*pStatus == 0) {
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {
				index = (Block * m_NumSettings) + SettingNum[SETUP_ENG];
				if (Settings[index] != m_Energy[Configuration]) {
					Settings[index] = m_Energy[Configuration];
					set_energy = 1;
				}
			}

			// if a value was changed
			if (set_energy == 1) {

				// load new settings
				hr = Set_Analog_Settings(Configuration, Head, Settings, pStatus);
				if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Set_Analog_Settings), HRESULT", hr);

				// heads will now need to be set up
				set_heads = 1;
			}
		}
	}
	
	// check the stored values to see if the heads are in the correct mode
	for (Head = 0 ; (Head < m_NumHeads) && (set_heads == 0) && (*pStatus == 0) && SUCCEEDED(hr) ; Head++) if (m_HeadPresent[Head] == 1) {
		if ((m_LastBlock[Head] != -1) || (m_LastMode[Head] != mode)) {
			set_heads = 1;
		}
	}

	// if the head has pass so far, then see how the head is currently set
	for (Head = 0 ; (Head < m_NumHeads) && (set_heads == 0) && (*pStatus == 0) && SUCCEEDED(hr) ; Head++) if (m_HeadPresent[Head] == 1) {

		// query the head
		hr = Check_Head_Mode(Head, &Head_Mode, &Head_Configuration, &Head_Layer, &Head_LLD, &Head_ULD, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Check_Head_Mode), HRESULT", hr);

		// if successfull, compare head values
		if (*pStatus == 0) {
			if ((Head_Mode != mode) || (Head_Configuration != Configuration) || (Head_Layer != Layer) || (Head_LLD != LLD) || (Head_ULD != ULD)) {
				set_heads = 1;
			}
		}
	}

	// if the heads appear to be in the correct mode, then check if they are producing singles
	if ((set_heads == 0) && (*pStatus == 0)) {

		// build command for pulling singles from a head
		sprintf(command, "K");

		// establish the number of fields in a response
		switch (m_ScannerType) {

		case SCANNER_HRRT:
			NumFields = 18;
			break;

		case SCANNER_P39:
		case SCANNER_P39_IIA:
		case SCANNER_PETSPECT:
			NumFields = 14;
			break;
		}

		// loop through heads getting singles data
		for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) ; Head++) if (m_HeadPresent[Head] == 1) {

			// initialize
			Statistics[Head+3] = 0;

			// if simulation mode
			if (m_Port == PORT_SIMULATION) {

				// feed in a value
				Statistics[Head+3] = 975318;

			// otherwise get it from the head
			} else {

				// issue the command
				*pStatus = Internal_Pass(Head, RETRY, command, response);

				// if successful, process out the singles
				if (*pStatus == 0) {
					index = (Head < 10) ? 2 : 3;
					ptr = &response[index];

					if (*pStatus == 0) for (i = 0 ; i < NumFields ; i++) Statistics[Head+3] = strtol(ptr, &ptr, 0);
				}
			}
		}

		// if successfull, check singles
		for (Head = 0 ; (Head < m_NumHeads) & (*pStatus == 0) ; Head++) {
			if ((m_HeadPresent[Head] == 1) && (Statistics[Head+3] == 0)) {
				set_heads = 1;
			}
		}
	}

	// if the heads need to be set up, make sure time offsets are loaded
	if ((set_heads == 1) && (*pStatus == 0) && (SCANNER_HRRT != m_ScannerType)) {
		hr = Load_RAM(Configuration, -1, RAM_TIME_OFFSETS, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Load_RAM <Time>), HRESULT", hr);
		if (*pStatus != 0) Sleep(5000);
	}

	// if the heads need to be set up, make sure crystal energy peaks are loaded
	if ((set_heads == 1) && (*pStatus == 0)) {
		hr = Load_RAM(Configuration, -1, RAM_ENERGY_PEAKS, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Load_RAM <Energy Peaks>), HRESULT", hr);
		if (*pStatus != 0) Sleep(5000);
	}

	// if the heads need to be set up, put them in run mode
	if ((set_heads == 1) && (*pStatus == 0)) {
		hr = Set_Head_Mode(mode, Configuration, -1, Layer, -1, LLD, ULD, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Set_Head_Mode <run>), HRESULT", hr);
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Initialize_PET
// Purpose:		Puts coincidence processor in coincidence mode
//				and heads in run mode with all crystal peaks, time offsets
//				and energy settings set
// Arguments:	ScanType		-	long = flag for whether or not point sources are to be extended
//					0 = Emission
//					1 = Emission/Transmission
//					2 = Transmission 
//				LLD				-	long = lower level discriminator
//				ULD				-	long = upper level discriminator
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Initialize_PET(long ScanType, long LLD, long ULD, long *pStatus)
{
	// local variables
	long Configuration = 0;
	long Transmission = 0;
	long ModulePairs = 0;
	long Layer = LAYER_ALL;

	char Subroutine[80] = "Initialize_PET";

	HRESULT hr = S_OK;

	// verify scan type
	if ((ScanType < 0) || (ScanType > 2)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Scan Type", ScanType);
		return S_FALSE;
	}

	// verify scan type
	if ((m_ScannerType == SCANNER_HRRT) && (ScanType == 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Simultaneous Emission/Transmission Not Available for this Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify scan type
	if ((m_ScannerType == SCANNER_HRRT) && (ScanType == 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Separate Transmission Not Available for this Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// turn on transmission if requested
	if (ScanType > 0) Transmission = 1;

	// for P39, build up the modue pair
	if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {

		// scan type
		switch (ScanType) {

		// Emission only scan
		case SCAN_EM:
			ModulePairs = m_Module_Em;
			break;

		// Emission/Transmission scan
		case SCAN_EMTX:
			ModulePairs = m_Module_EmTx;
			break;

		// Transmission only scan
		case SCAN_TX:
			ModulePairs = m_Module_Tx;
			break;
		}
	}

	// redirect the call
	hr = Initialize_Scan(Transmission, ModulePairs, Configuration, Layer, LLD, ULD, pStatus);

	// return result
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Start_Scan
// Purpose:		Start flow of data from coincidence processor and extend point sources
//				if transmission scan
// Arguments:	*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Start_Scan(long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Start_Scan";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// extend trasmission source (will also make sure it is retracted if not desired)
	if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA) || (m_ScannerType == SCANNER_PETSPECT)) {
		if (m_Transmission != -1) hr = Point_Source(-1, m_Transmission, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Point_Source), HRESULT", hr);
	}

	// issue the "Go" command
	if (*pStatus == 0) {

		// act on port type
		switch (m_Port) {

		// ehternet command
		case PORT_ETHERNET:

			// informational message to log
			Log_Message("m_pCPMain->Start");

			// issue command
			hr = m_pCPMain->Start(&ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Start), HRESULT", hr);

			// check for error
			if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
			break;

		// serial line command
		case PORT_SERIAL:

			// P39 includes the module pairs
			if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {
				sprintf(command, "G %X", m_ModulePairs);
			} else {
				sprintf(command, "G");
			}

			// issue command
			*pStatus = Internal_Pass(64, NORETRY, command, response);
			break;

		case PORT_SIMULATION:
			m_Simulated_Data_Flow = 1;
			break;
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Stop_Scan
// Purpose:		Stop flow of data from coincidence processor and retract point sources
// Arguments:	*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Stop_Scan(long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Stop_Scan";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// always retract the point source
	if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA) || (m_ScannerType == SCANNER_PETSPECT)) {
		hr = Point_Source(-1, 0, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Point_Source), HRESULT", hr);
	}

	// act on port type
	switch (m_Port) {

	// ethernet command
	case PORT_ETHERNET:

		// informational message to log
		Log_Message("m_pCPMain->Stop");

		// issue command
		hr = m_pCPMain->Stop(&ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Stop), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line command
	case PORT_SERIAL:
		sprintf(command, "H");
		*pStatus = Internal_Pass(64, RETRY, command, response);
		break;

	// simulation
	case PORT_SIMULATION:
		m_Simulated_Data_Flow = 0;
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		DataStream
// Purpose:		Start/Stop flow of data from coincidence processor
// Arguments:	OnOff			-	long = flag for whether or not data is to be sent
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::DataStream(long OnOff, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "DataStream";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// verify OnOff argument
	if ((OnOff != 0) && (OnOff != 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Data Stream State", OnOff);
		return S_FALSE;
	}

	// Act on port type
	switch (m_Port) {

	// ethernet command
	case PORT_ETHERNET:

		// off
		if (OnOff == 0) {

			// informational message to log
			Log_Message("m_pCPMain->Stop");

			// issue command
			hr = m_pCPMain->Stop(&ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Stop), HRESULT", hr);

		// on
		} else {

			// informational message to log
			Log_Message("m_pCPMain->Start");

			// issue command
			hr = m_pCPMain->Start(&ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Start), HRESULT", hr);
		}

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line command
	case PORT_SERIAL:

		// off
		if (OnOff == 0) {
			sprintf(command, "H");

		// on
		} else {

			// P39 includes the module pairs
			if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {
				sprintf(command, "G %X", m_ModulePairs);
			} else {
				sprintf(command, "G");
			}
		}

		// issue command
		*pStatus = Internal_Pass(64, RETRY, command, response);
		break;

	// simulation
	case PORT_SIMULATION:
		m_Simulated_Data_Flow = OnOff;
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Refresh_Analog_Settings
// Purpose:		read the analog settings for a specified head/configuration
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				04 Mar 04	T Gremillion	Activated method for ring scanners
//				19 Oct 04	T Gremillion	remove unused file pointer
//											save to file for all scanner types
//											make sure analog settings from HRRT are not all zero
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Refresh_Analog_Settings(long Configuration, long Head, long *pStatus)
{
	// variables needed regardless of which method is active
	char Subroutine[80] = "Refresh_Analog_Settings";

	HRESULT hr = S_OK;

	long Block = 0;
	long Setting = 0;
	long index = 0;

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// informational message
	Log_Message("Refresh of Analog Settings");

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Sum;
	long Pass;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];
	char *ptr = NULL;

	// loop through all blocks of the specified head and configuration reading settings
	for (Block = 0 ; (Block < m_NumBlocks) && SUCCEEDED(hr) ; Block++) {

		// if scanner connected, get values
		if (m_Port != PORT_SIMULATION) {

			// build command
			sprintf(command, "G %d %d", Configuration, Block);

			// allow two tries for each block
			for (Sum = 0, Pass = 0 ; (Sum == 0) && (Pass < 2) ; Pass++) {

				// issure command
				*pStatus = Internal_Pass(Head, RETRY, command, response);

				// if error, return
				if (*pStatus != 0) return S_FALSE;

				// strip off command identifier
				index = (Head < 10) ? 2 : 3;
				ptr = &response[index];

				// break out the analog settings
				for (Setting = 0 ; (Setting < m_NumSettings) && SUCCEEDED(hr) ; Setting++) {
					index = (((((Configuration * MAX_HEADS) + Head) * m_NumBlocks) + Block) * m_NumSettings) + Setting;
					if (strlen(ptr) == 0) {
						Sum = 0;
						break;
					}
					m_Settings[index] = strtol(ptr, &ptr, 0);
					Sum += m_Settings[index];
				}

				// sum of zero is a problem
				if (0 == Sum) {
					if (0 == Pass) {
						Add_Warning(DHI, Subroutine, "WARNING - DHI returned bad settings for Block =", Block);
					} else {
						*pStatus = Add_Error(DHI, Subroutine, "ERROR - DHI returned bad settings for Block =", Block);
						return S_FALSE;
					}
				}
			}

		// otherwise, this is a simulation
		} else {

			// simulate delay
			if (Block == 0) {
				Sleep(1000);
			} else {
				Sleep(50);
			}

			// index to start of settings
			index = (((((Configuration * MAX_HEADS) + Head) * m_NumBlocks) + Block) * m_NumSettings);

			// generate values if the setup energy is not set to a valid value
			if ((m_Settings[index+SettingNum[SETUP_ENG]] != 190) && (m_Settings[index+SettingNum[SETUP_ENG]] != m_Energy[Configuration])) {

				// put in a recognizable pattern
				for (Setting = 0 ; Setting < m_NumSettings ; Setting++) m_Settings[index+Setting] = Block + Setting + 40;

				// default the energy setting to energy mode
				m_Settings[index+SettingNum[SETUP_ENG]] = m_Energy[Configuration];

				// other settings are scanner dependant
				switch (m_ScannerType) {

				case SCANNER_P39:
					m_Settings[index+SettingNum[CFD]] = 80;
					m_Settings[index+SettingNum[X_OFFSET]] = 50;
					m_Settings[index+SettingNum[Y_OFFSET]] = 55;
					m_Settings[index+SettingNum[E_OFFSET]] = 60;
					m_Settings[index+SettingNum[SHAPE]] = 0;
					m_Settings[index+SettingNum[SLOW_LOW]] = 0;
					m_Settings[index+SettingNum[SLOW_HIGH]] = 0;
					m_Settings[index+SettingNum[FAST_LOW]] = 0;
					m_Settings[index+SettingNum[FAST_HIGH]] = 300;
					break;

				case SCANNER_P39_IIA:
					m_Settings[index+SettingNum[CFD]] = 100;
					m_Settings[index+SettingNum[CFD_SETUP]] = 23;
					m_Settings[index+SettingNum[TDC_GAIN]] = 128;
					m_Settings[index+SettingNum[TDC_OFFSET]] = 128;
					m_Settings[index+SettingNum[TDC_SETUP]] = 32;
					break;

				case SCANNER_PETSPECT:
					m_Settings[index+SettingNum[CFD]] = 80;
					m_Settings[index+SettingNum[X_OFFSET]] = 50;
					m_Settings[index+SettingNum[Y_OFFSET]] = 55;
					m_Settings[index+SettingNum[E_OFFSET]] = 60;
					m_Settings[index+SettingNum[SHAPE]] = 65;
					m_Settings[index+SettingNum[SLOW_LOW]] = 35;
					m_Settings[index+SettingNum[SLOW_HIGH]] = 63;
					m_Settings[index+SettingNum[FAST_LOW]] = 67;
					m_Settings[index+SettingNum[FAST_HIGH]] = 95;
					break;

				case SCANNER_HRRT:
					m_Settings[index+SettingNum[CFD]] = 80;
					m_Settings[index+SettingNum[CFD_DELAY]] = 45;
					m_Settings[index+SettingNum[X_OFFSET]] = 50;
					m_Settings[index+SettingNum[Y_OFFSET]] = 55;
					m_Settings[index+SettingNum[SHAPE]] = 65;
					break;

				default:
					break;
				}
			}
		}
	}

#else

	// local variables
	long i = 0;
	long j = 0;

	char message[MAX_STR_LEN];

	// loop through all blocks of the specified head and configuration reading settings
	for (Block = 0 ; (Block < m_NumBlocks) && (*pStatus == 0) ; Block++) {

		// retrieve from bucket
		if (GCI_SUCCESS != get_asic_values(Head, Block, (int*) &m_Settings[((Head * m_NumBlocks) + Block) * m_NumSettings])) {
			sprintf(message, "Bucket %d Block %d - Could Not Retrieve Values From Head", Head, Block);
			*pStatus = Add_Error(DAL, Subroutine, message, GCI_ERROR);
		}
	}

#endif

	// if successful, write to file
	if (*pStatus == 0) *pStatus = Save_Settings(0, Head);

	// bad status return false
	if (*pStatus != 0) {
		hr = S_FALSE;

	// settings can now be trusted
	} else {
		m_Trust[(Configuration*MAX_HEADS)+Head] = 1;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Analog_Settings
// Purpose:		returns the analog settings for a specified head/configuration
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head
//				*Settings		-	long = array of analog settings for specified head
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				04 Mar 04	T Gremillion	Activated method for ring scanners
//											and added code to check for existing data
//											and compare checksums against buckets.
//				19 Oct 04	T Gremillion	Checksums can now also be checked on HRRT
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Get_Analog_Settings(long Configuration, long Head, long Settings[], long *pStatus)
{

	// local variables
	char Subroutine[80] = "Get_Analog_Settings";
	HRESULT hr = S_OK;
	long Block = 0;
	long Setting = 0;
	long index = 0;
	long index_out = 0;

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// initialize status
	*pStatus = 0;

	// for ring scanners, compare checksums to make sure the settings can be trusted
	if ((m_Trust[Head] == 1) && ((SCANNER_RING == m_ScannerType) || (SCANNER_HRRT == m_ScannerType))) {
		m_Trust[Head] = Compare_Checksum(Head);
	}

	// if the settings for this head are not to be trusted, then refresh them
	if (m_Trust[(Configuration*MAX_HEADS)+Head] != 1) {
		hr = Refresh_Analog_Settings(Configuration, Head, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Refresh_Analog_Settings), HRESULT", hr);
	}

	// transfer to output array
	for (Block = 0 ; (Block < m_NumBlocks) && (*pStatus == 0) ; Block++) {
		for (Setting = 0 ; (Setting < m_NumSettings) && SUCCEEDED(hr) ; Setting++) {

			// indicies into arrays
			index_out = (Block * m_NumSettings) + Setting;
			index = (((((Configuration * MAX_HEADS) + Head) * m_NumBlocks) + Block) * m_NumSettings) + Setting;

			// copy to output array
			Settings[index_out] = m_Settings[index];
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	// return status from refresh
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Set_Analog_Settings
// Purpose:		change the analog settings for a specified head/configuration
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head
//				*Settings		-	long = array of analog settings for specified head
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	Save to File every time they are set.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Set_Analog_Settings(long Configuration, long Head, long Settings[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Set_Analog_Settings";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	long cmd = -1;
	long Block = 0;
	long Setting = 0;
	long index = 0;
	long index_in = 0;

	long changed[MAX_BLOCKS * MAX_SETTINGS];
	long column_changed[MAX_SETTINGS];
	long column_value[MAX_SETTINGS];
	long different[MAX_SETTINGS];

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// if the settings for this head are not to be trusted, then refresh them
	if (m_Trust[(Configuration*MAX_HEADS)+Head] != 1) {
		hr = Refresh_Analog_Settings(Configuration, Head, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Refresh_Analog_Settings), HRESULT", hr);
	}

	// determine if any setting is being set to the same value for all blocks
	// loop through all settings
	for (Setting = 0 ; Setting < m_NumSettings ; Setting++) {

		// loop through all blocks
		for (Block = 0 ; Block < m_NumBlocks ; Block++) {

			// indicies into arrays
			index_in = (Block * m_NumSettings) + Setting;
			index = (((((Configuration * MAX_HEADS) + Head) * m_NumBlocks) + Block) * m_NumSettings) + Setting;

			// Block 0 - Initialize
			if (Block == 0) {

				// save value from first block
				column_value[Setting] = Settings[index_in];
				different[Setting] = 0;

				// if the setting has changed from the stored setting then set flag for block and initialize count to 1
				if (Settings[index_in] != m_Settings[index]) {
					changed[index_in] = 1;
					column_changed[Setting] = 1;

				// otherwise, clear flag for block and initialize count to 0
				} else {
					changed[index_in] = 0;
					column_changed[Setting] = 0;
				}

			// other blocks
			} else {

				// checking to see if the same value is used throughout
				if (Settings[index_in] != column_value[Setting]) different[Setting] = 1;

				// if the setting has changed from the stored setting then set flag for block increment count
				if (Settings[index_in] != m_Settings[index]) {
					changed[index_in] = 1;
					column_changed[Setting]++;

				// otherwise, clear flag for block and initialize count to 0
				} else {
					changed[index_in] = 0;
				}
			}
		}
	}

	// loop through the columns
	for (Setting = 0 ; (Setting < m_NumSettings) && SUCCEEDED(hr) ; Setting++) {
		
		// see which ones can be set with an all inclusive command
		if ((different[Setting] == 0) && (column_changed[Setting] > (m_NumBlocks / 4))) {

			// find command
			cmd = -1;
			for (i = 0 ; i < MAX_SETTINGS ; i++) {
				if (SettingNum[i] == Setting) {
					cmd = i;
					break;
				}
			}

			// build command
			sprintf(command, "S %d %d %s %d", Configuration, -1, SetCmd[cmd], column_value[Setting]);

			// issure command - if not simulation
			if (m_Port != PORT_SIMULATION) {
				*pStatus = Internal_Pass(Head, RETRY, command, response);

			// simulation - wait 2 seconds to simulation communications delay
			} else {
				m_SimulatedMode[Head] = -1;
				Sleep(2000);
			}

			// if error, return
			if (*pStatus != 0) return S_FALSE;

			// set to new value
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {

				// clear changed flag
				index_in = (Block * m_NumSettings) + Setting;
				changed[index_in] = 0;

				// update stored value
				index = (((((Configuration * MAX_HEADS) + Head) * m_NumBlocks) + Block) * m_NumSettings) + Setting;
				m_Settings[index] = column_value[Setting];
			}
		}
	}

	// special commands exist for setting multiple gain values faster
	for (Block = 0 ; Block < m_NumBlocks ; Block++) {

		// indicies into arrays
		index_in = (Block * m_NumSettings);
		index = (((((Configuration * MAX_HEADS) + Head) * m_NumBlocks) + Block) * m_NumSettings);

		// if the gain values have changed
		if ((changed[index_in+SettingNum[PMTA]] + changed[index_in+SettingNum[PMTB]] + 
			changed[index_in+SettingNum[PMTC]] + changed[index_in+SettingNum[PMTD]]) > 1) {

			// copy the new setting into memory
			m_Settings[index+SettingNum[PMTA]] = Settings[index_in+SettingNum[PMTA]];
			m_Settings[index+SettingNum[PMTB]] = Settings[index_in+SettingNum[PMTB]];
			m_Settings[index+SettingNum[PMTC]] = Settings[index_in+SettingNum[PMTC]];
			m_Settings[index+SettingNum[PMTD]] = Settings[index_in+SettingNum[PMTD]];

			// switch off the flags indicating they were changed
			changed[index_in+SettingNum[PMTA]] = 0;
			changed[index_in+SettingNum[PMTB]] = 0;
			changed[index_in+SettingNum[PMTC]] = 0;
			changed[index_in+SettingNum[PMTD]] = 0;

			// if all the values are the same, then use pmt*
			if ((m_Settings[index+SettingNum[PMTA]] == m_Settings[index+SettingNum[PMTB]]) &&
				(m_Settings[index+SettingNum[PMTA]] == m_Settings[index+SettingNum[PMTC]]) &&
				(m_Settings[index+SettingNum[PMTA]] == m_Settings[index+SettingNum[PMTD]])) {

				// build command
				sprintf(command, "S %d %d pmt* %d", Configuration, Block, m_Settings[index+SettingNum[PMTA]]);

				// issure command - if not simulation
				if (m_Port != PORT_SIMULATION) {
					*pStatus = Internal_Pass(Head, RETRY, command, response);

				// simulation - simulate small time delay
				} else {
					Sleep(50);
					m_SimulatedMode[Head] = -1;
				}

				// if error, return
				if (*pStatus != 0) return S_FALSE;

			// for different values, use gain
			} else {

				// build command
				sprintf(command, "S %d %d gain %d %d %d %d", Configuration, Block, m_Settings[index+SettingNum[PMTA]], 
					m_Settings[index+SettingNum[PMTB]], m_Settings[index+SettingNum[PMTC]], m_Settings[index+SettingNum[PMTD]]);

				// issure command
				*pStatus = Internal_Pass(Head, RETRY, command, response);

				// if error, return
				if (*pStatus != 0) return S_FALSE;
			}
		}
	}

	// compare all individual values against stored values
	for (Block = 0 ; (Block < m_NumBlocks) && SUCCEEDED(hr) ; Block++) {
		for (Setting = 0 ; (Setting < m_NumSettings) && SUCCEEDED(hr) ; Setting++) {

			// indicies into arrays
			index_in = (Block * m_NumSettings) + Setting;
			index = (((((Configuration * MAX_HEADS) + Head) * m_NumBlocks) + Block) * m_NumSettings) + Setting;

			// if the value has changed then set the new value
			if (changed[index_in] == 1) {

				// find command
				cmd = -1;
				for (i = 0 ; i < MAX_SETTINGS ; i++) {
					if (SettingNum[i] == Setting) {
						cmd = i;
						break;
					}
				}

				// build command
				sprintf(command, "S %d %d %s %d", Configuration, Block, SetCmd[cmd], Settings[index_in]);
				
				// issure command - if not simulation
				if (m_Port != PORT_SIMULATION) {
					*pStatus = Internal_Pass(Head, RETRY, command, response);

				// simulation - simulate small time delay
				} else {
					Sleep(50);
					m_SimulatedMode[Head] = -1;
				}

				// if error, return
				if (*pStatus != 0) return S_FALSE;

				// copy the new setting into memory
				m_Settings[index] = Settings[index_in];
			}
		}
	}

	// good status, save settings
	if (*pStatus == 0) *pStatus = Save_Settings(Configuration, Head);

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		File_Upload
// Purpose:		copy a file from the DHI to the calling routine
// Arguments:	Head			-	long = Detector Head
//				*Filename		-	unsigned char = name of file on head
//				*Filesize		-	long = size of file in bytes
//				*CheckSum		-	unsigned long = checksum of bytes in file
//				**buffer		-	unsigned char = file bytes
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::File_Upload(long Head, unsigned char Filename[], long *Filesize, unsigned long *CheckSum, unsigned char** buffer, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "File_Upload";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	long index = 0;
	long block = 0;
	long blocksize = 256;
	long finished = 0;
	long bytes = 0;
	long state = 0;
	long code = 0;
	long retry = 1;
	long pass = 0;
	long mem_error = 0;
	long ErrorStatus = 0;
	long buffer_size = 0;
	unsigned long check = 0;
	unsigned long local = 0;
	unsigned char data[MAX_STR_LEN];
	char message[MAX_STR_LEN];
	char *ptr = NULL;
	unsigned char *buf = NULL;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status, filesize and pointer
	*pStatus = 0;
	*Filesize = 0;
	*CheckSum = 0;
	*buffer = NULL;

	// verify head number against full range
	if (((Head < 0) || (Head >= m_NumHeads)) && (Head != 64)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != 64) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// simulation - wait 5 seconds and return empty file
	if (m_Port == PORT_SIMULATION) {
		Sleep(5000);
		return S_OK;
	}

	// different for ethernet based upload from coincidence processor
	if ((Head == 64) && (m_Port == PORT_ETHERNET)) {

		// Add to log file
		sprintf(message, "m_pOSMain->Up_file: %s", Filename);
		Log_Message(message);

		// issue command
		hr = m_pOSMain->Up_file(Filename, CheckSum, (unsigned long *) Filesize, buffer, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pOSMain->Up_file), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = OS_Error_Query(ErrorStatus);

	// serial line and/or detector head
	} else {

		// allow for multiple attempts
		while ((retry == 1) && (*pStatus == 0)) {

			// read from head
			for (block = 0, finished = 0 ; (finished != 1) && (*pStatus == 0) ; block++) {

				// state of file
				if (block == 0) {
					state = 0;
				} else {
					state = 1;
				} // if (block

				// build and send command
				sprintf(command, "U %s %d", Filename, state);
				*pStatus = Internal_Pass(Head, NORETRY, command, response);

				// if command successful then process response
				if (*pStatus == 0) {

					// strip off command identifier
					index = (Head < 10) ? 2 : 3;
					ptr = &response[index];

					// strip the command status and break out the checksum
					check = strtol(ptr, &ptr, 0);	// status
					check = strtol(ptr, &ptr, 0);	// checksum

					// skip the trailing space
					ptr++;

					// convert the hex string to character
					bytes = hextobin(ptr, data, 256);
				} // if (*pStatus

				// if an end of file error was returned, then it needs to be ignored
				if (*pStatus != 0) {

					// request the error code
					hr = Error_Lookup(*pStatus, &code, (unsigned char *) message);
					if (FAILED(hr)) {
						*pStatus = Add_Error(DAL, Subroutine, "Method Failed (Error_Lookup)", hr);

					// End of file is not really an error
					} else if (code == E_EOF) {
						Add_Information(DAL, Subroutine, "EOF is not really an error");
						bytes = 0;
						check = 0;
						*pStatus = 0;
					}
				} // if (*pStatus

				// compare checksum
				if (*pStatus == 0) {

					// calculate checksum
					for (i = 0, local = 0 ; i < bytes ; i++) local += (unsigned long) data[i];

					// make sure checksum matches
					if (check != local) {

						// close up this attempt
						finished = 1;

						// keep track of number of attempts
						pass++;

						// after 5 attempts, call it an error
						if (pass > 5) {

							// add message to log and set error return value
							sprintf(message, "Computed Checksum %d on %d Bytes Did Not Match DHI Checksum - Exiting After 5 Attempts, DHI Checksum", local, bytes);
							*pStatus = Add_Error(DAL, Subroutine, message, check);

							// do not try again
							retry = 0;

						// otherwise
						} else {

							// add message to log file, then clear error code
							sprintf(message, "Computed Checksum %d on %d Bytes Did Not Match DHI Checksum - Restarting File Upload, DHI Checksum", local, bytes);
							Add_Warning(DAL, Subroutine, message, check);

							// set flags to not write data and try again
							bytes = 0;
							retry = 1;
						}
					} else {
						
						// current pass is error free
						retry = 0;

						// check if finished
						if (bytes < blocksize) finished = 1;

						// if adding the data will get too big for the buffer, then expand it
						if ((*Filesize + bytes) > buffer_size) {
							buffer_size += 1024 * 1024;	
							buf = (unsigned char *) ::CoTaskMemRealloc((void *) *buffer, buffer_size);
							if (buf == NULL) {
								free((void *) *buffer);
								mem_error = 1;
								finished = 1;
							}
							*buffer = buf;
						}

						// add to buffer
						buf = *buffer;
						for (i = 0 ; i < bytes ; i++) buf[(block*blocksize)+i] = data[i];
						*Filesize += bytes;
					} // if check != local
				} // if (*pStatus
			} // for (block = 0

			// close file on head
			if (*pStatus == 0) {
				sprintf(command, "U %s 2", Filename);
				*pStatus = Internal_Pass(Head, NORETRY, command, response);
			}

			// if an end of file error was returned, then it needs to be ignored
			if (*pStatus != 0) {

				// request the error code
				hr = Error_Lookup(*pStatus, &code, (unsigned char *) message);
				if (FAILED(hr)) {
					*pStatus = Add_Error(DAL, Subroutine, "Method Failed (Error_Lookup)", hr);

				// End of file is not really an error
				} else if (code == E_EOF) {
					Add_Information(DAL, Subroutine, "EOF is not really an error");
					*pStatus = 0;
				}
			} // if (*pStatus
		} // while retry

		// if a memory error occurred
		if (mem_error == 1) *pStatus = Add_Error(DAL, Subroutine, "Allocating Memory, Requested Size", buffer_size);

		// if successful
		if (*pStatus == 0) {

			// trim the buffer down to the filesize
			buf = (unsigned char *) ::CoTaskMemRealloc((void *) *buffer, *Filesize);
			*buffer = buf;

			// calculate the checksum
			for (*CheckSum = 0, i = 0 ; i < *Filesize ; i++) *CheckSum += buf[i];
		}
	} // if ((head == 64

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		File_Download
// Purpose:		copy a file from the calling routine to the DHI
// Arguments:	Head			-	long = Detector Head
//				Filesize		-	long = size of file in bytes
//				CheckSum		-	unsigned long = checksum of bytes in file
//				*Filename		-	unsigned char = name of file on head
//				**buffer		-	unsigned char = file bytes
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::File_Download(long Head, long Filesize, unsigned long CheckSum, unsigned char Filename[], unsigned char** buffer, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "File_Download";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long block = 0;
	long finished = 0;
	long bytes = 0;
	long state = 0;
	long code = 0;
	long ch = 0;
	long pass = 0;
	long retry = 1;
	long blocksize = 256;
	long ErrorStatus = 0;
	unsigned long check = 0;

	unsigned char data[MAX_STR_LEN];
	char hex[2*MAX_STR_LEN];
	char message[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];
	unsigned char *ptr;

	// initialize status
	*pStatus = 0;

	// verify head number against full range
	if (((Head < 0) || (Head >= m_NumHeads)) && (Head != 64)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != 64) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify filesize
	if (Filesize < 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, File Size", Filesize);
		return S_FALSE;
	}

	// verify checksum
	ptr = *buffer;
	for (ch = 0 ; ch < Filesize ; ch++) check += (unsigned long) ptr[ch];
	if (check != CheckSum) {
		sprintf(response, "Invalid Argument, Checksum Does Not Match, Computeded = %d Argument", check);
		*pStatus = Add_Error(DAL, Subroutine, response, CheckSum);
		return S_FALSE;
	}

	// simulation - wait 5 seconds and return
	if (m_Port == PORT_SIMULATION) {
		Sleep(5000);
		return S_OK;
	}

	// block size dependent on communications protocol
	if ((Head == 64) && (m_Port == PORT_ETHERNET)) {

		// Add to log file
		sprintf(message, "m_pOSMain->Dn_file: %s", Filename);
		Log_Message(message);

		// issue command
		hr = m_pOSMain->Dn_file(Filename, Filesize, CheckSum, buffer, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pOSMain->Dn_file), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = OS_Error_Query(ErrorStatus);

	// serial line or detector head
	} else {

		// allow multiple attempts
		while ((retry == 1) && (*pStatus == 0)) {

			// loop through reading the file and sending it down to the head
			for (block = 0, finished = 0 ; (finished != 1) && (*pStatus == 0) ; block++) {

				// transfer the next bytes from the input buffer
				bytes = Filesize - (block * blocksize);
				if (bytes > blocksize) bytes = blocksize;
				for (ch = 0 ; ch < bytes ; ch++) data[ch] = ptr[(block * blocksize)+ch];

				// open
				if (block == 0) {
					state = 0;

				// write
				} else if (bytes > 0) {
					state = 1;

				// nothing left to write
				} else {
					finished = 1;
					break;
				}

				// determine the checksum (and convert to hexdecimal needed for serial communication);
				check = bintohex(data, hex, bytes);

				// build and send the command
				sprintf(command, "D %s %d %d %s", Filename, state, check, hex);
				*pStatus = Internal_Pass(Head, NORETRY, command, response);

				// check type of error
				if (*pStatus != 0) {

					// request the error code
					hr = Error_Lookup(*pStatus, &code, (unsigned char *) message);
					if (FAILED(hr)) {
						*pStatus = Add_Error(DAL, Subroutine, "Method Failed (Error_Lookup)", hr);

					// if checksum error
					} else if (code == E_CHECKSUM) {

						// close up this attempt
						finished = 1;

						// keep track of number of attempts
						pass++;

						// limited number of passes
						if (pass <= 5) {

							// set flags to close files and try again
							retry = 1;

							// add message to log file, then clear error code
							Add_Information(DAL, Subroutine, "Restarting File Download");
							*pStatus = 0;
						}
					}
				} else retry = 0;
			}

			// close the head file if successful
			if ((*pStatus == 0) && ((Head != 64)  || (m_Port != PORT_ETHERNET))) {
				sprintf(command, "D %s 2 0", Filename);
				*pStatus = Internal_Pass(Head, NORETRY, command, response);
			}
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	High_Voltage
// Purpose:	Turn the high voltage power supply on/off
// Arguments:	OnOff			-	long = State Flag for High Voltage
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::High_Voltage(long OnOff, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "High_Voltage";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Head = 0;
	char *HRRT_OffOn[2] = {"K", "J"};
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// verify OnOff
	if ((OnOff < 0) || (OnOff > 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, High Voltage State", OnOff);
		return S_FALSE;
	}

	// simulation - quick return
	if (m_Port == PORT_SIMULATION) {
		Sleep(50);
		m_Simulated_High_Voltage = OnOff;
		return S_OK;
	}

	// scanner based control
	switch (m_ScannerType) {

	// the hrrt switches high voltage on and off at the coincidence processor
	case SCANNER_HRRT:
		Head = 64;
		sprintf(command, "%s 0", HRRT_OffOn[OnOff]);
		break;

	// the P39 and PETSPECT can swithch high voltage on and off from any head (use the first one that is present)
	case SCANNER_P39:
	case SCANNER_P39_IIA:
	case SCANNER_PETSPECT:
		for (Head = 0 ; (Head < m_NumHeads) && (m_HeadPresent[Head] == 0) ; Head++);
		if (Head == m_NumHeads) *pStatus = Add_Error(DAL, Subroutine, "No Heads Present, Number of Heads", 0);
		sprintf(command, "H %d", OnOff);
		break;

	default:
		*pStatus = Add_Error(DAL, Subroutine, "Unknown Scanner, Model", m_ModelNumber);
		break;
	}

	// issue the command
	if (*pStatus == 0) *pStatus = Internal_Pass(Head, RETRY, command, response);

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Point_Source
// Purpose:	Extend/Retract the transmission point sources for the specified head(s)
// Arguments:	Head			-	long = Detector Head
//				OnOff			-	long = State Flag for extension of point source
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Point_Source(long Head, long OnOff, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Point_Source";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long start_head = 0;
	long end_head = 0;
	long loop_head = 0;
	long retract_head = 0;
	long Status = 0;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// verify scanner
	if ((m_ScannerType != SCANNER_P39) && (m_ScannerType != SCANNER_P39_IIA) && (m_ScannerType != SCANNER_PETSPECT)) {
		*pStatus = Add_Error(DAL, Subroutine, "Coincidence Point Sources Not On This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify OnOff
	if ((OnOff < 0) || (OnOff > 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Point Source State", OnOff);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < -1) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != -1) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify head number as having point source
	if (Head != -1) if (m_PointSource[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Point Source on Head", Head);
		return S_FALSE;
	}

	// determine heads
	start_head = (Head < 0) ? 0 : Head;
	end_head = (Head < 0) ? (m_NumHeads-1) : Head;

	// build command
	sprintf(command, "F %d", OnOff);

	// issue the command
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_PointSource[loop_head] == 1) {

		// simulation, wait 1 second for each
		if (m_Port == PORT_SIMULATION) {
			Sleep(1000);
			m_SimulatedSource[loop_head] = OnOff;
		} else {

			// issue command
			*pStatus = Internal_Pass(loop_head, RETRY, command, response);

			// if failed trying to extend, retract the ones that did extend
			if ((*pStatus != 0) && (OnOff != 0)) {

				// more meaningful message
				*pStatus = Add_Error(DAL, Subroutine, "Failed To Extend Point Source, Head", loop_head);

				// retract command
				sprintf(command, "F 0");

				// issue
				for (retract_head = start_head, Status = 0 ; (retract_head < loop_head) && (Status == 0) ; retract_head++) {
					if (m_PointSource[retract_head] == 1) Status = Internal_Pass(retract_head, RETRY, command, response);
				}
			}
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Routine:	Point_Source_Status
// Purpose:	Check the Extend/Retract status of the transmission point sources for the specified head(s)
// Arguments:	Head			-	long = Detector Head
//				*pOnOff			-	long = State Flag for extension of point source
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
///////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Point_Source_Status(long Head, long *pOnOff, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Point_Source_Status";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// verify scanner
	if ((m_ScannerType != SCANNER_P39) && (m_ScannerType != SCANNER_P39_IIA) && (m_ScannerType != SCANNER_PETSPECT)) {
		*pStatus = Add_Error(DAL, Subroutine, "Coincidence Point Sources Not On This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify head number as having point source
	if (m_PointSource[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Point Source on Head", Head);
		return S_FALSE;
	}

	// build command
	sprintf(command, "F");

	// if simulation, return stored value
	if (m_Port == PORT_SIMULATION) {
		Sleep(50);
		*pOnOff = m_SimulatedSource[Head];

	// connected to scanner - issue command
	} else {

		// issue the command
		*pStatus = Internal_Pass(Head, RETRY, command, response);

		// set the state (defauts to extended)
		*pOnOff = (strstr(response, "RETRACTED") != NULL) ? 0 : 1;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Get_Statistics
// Purpose:	Return the prompts (or counts) and randoms being processed by the 
//			coincidence processor as well as the mode the coincidence processor
//			is in and the singles being produced by each head
// Arguments:	*Statistics		-	long = array of event statistics
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Get_Statistics(long Statistics[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	long Head = 0;
	char Subroutine[80] = "Get_Statistics";
	HRESULT hr = S_OK;

// code for ring scanners
#ifdef ECAT_SCANNER

	// local variables
	int Corrected = 0;
	int Uncorrected = 0;
	int Diagnostic = 0;

	// wrong type for this code
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Wrong Code For Pane Detector Scanners", 0);
		return S_FALSE;
	}

	// for for all buckets present
	for (*pStatus = 0, Head = 0 ; Head < MAX_HEADS ; Head++) {

		// if the head is present
		if (0 != m_HeadPresent[Head]) {

			// get the bucket singles through the gci interface
			if (get_bucket_singles(Head, &Corrected, &Uncorrected, &Diagnostic)) {
				Statistics[Head+3] = Corrected;
			} else {
				*pStatus = Add_Error(DAL, Subroutine, "Failed To Get Bucket Singles", 0);
			}
		}
	}

// panel detector scanner
#else

	// wrong type for this code
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Wrong Code For Ring Scanners", 0);
		return S_FALSE;
	}

	// local variables
	long i = 0;
	long Mode = -1;
	long Singles = -1;
	long Prompts = -1;
	long Randoms = -1;
	long ErrorStatus = 0;
	long NumFields = 0;
	long index = 3;
	char *ptr = NULL;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// act on port type
	switch (m_Port) {

	// ethernet connection
	case PORT_ETHERNET:

		// informational message to log
		Log_Message("m_pCPMain->Stats");

		// issue command
		hr = m_pCPMain->Stats(Statistics, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Stats), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line
	case PORT_SERIAL:

		// issue command
		sprintf(command, "Q");
		*pStatus = Internal_Pass(64, RETRY, command, response);

		// if successfull, look for the coincidence processor mode
		for (i = 0 ; (i < NUM_CP_MODES) && (*pStatus == 0) ; i++) {

			// check for occurrence of string in response
			if (strstr(response, CPModeStr[i]) != NULL) {
				Mode = i;
				break;
			}
		}

		// transfer to array (error will have left it at -1)
		Statistics[2] = Mode;

		// if mode is coincidence, then get randoms and prompts
		if ((*pStatus == 0) && ((Mode == CP_MODE_COINC) || (Mode == CP_MODE_PASSTHRU))) {
			ptr = &response[index];
			Singles = strtol(ptr, &ptr, 0);
			Randoms = strtol(ptr, &ptr, 0);
			Prompts = strtol(ptr, &ptr, 0);
		} else {
			Singles = 0;
			Randoms = 0;
			Prompts = 0;
		}

		// transfer to array (error will have left it at -1)
		Statistics[0] = Prompts;
		Statistics[1] = Randoms;

		// build command for pulling singles from a head
		sprintf(command, "K");

		// establish the number of fields in a response
		NumFields = (m_ScannerType == SCANNER_HRRT) ? 18 : 14;

		// loop through heads getting singles data
		for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) ; Head++) if (m_HeadPresent[Head] == 1) {

			// issue the command
			*pStatus = Internal_Pass(Head, RETRY, command, response);

			// if successful, process out the singles
			index = (Head < 10) ? 2 : 3;
			ptr = &response[index];
			if (*pStatus == 0) for (i = 0 ; i < NumFields ; i++) Statistics[Head+3] = strtol(ptr, &ptr, 0);
		}
		break;

	// simulation
	case PORT_SIMULATION:

		// initialize to all zeros
		for (i = 0 ; i < (MAX_HEADS+3) ; i++) Statistics[i] = 0;

		// transfer cp mode
		Statistics[2] = m_Simulated_CP_Mode;

		// act on mode
		switch (m_Simulated_CP_Mode) {

		// Coincidence Mode
		case CP_MODE_COINC:
			if (m_Simulated_Data_Flow == 1) {
				Statistics[0] = 654321;
				Statistics[1] = 210987;
			}
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_HeadPresent[Head] == 1) {
					Statistics[Head+3] = 975318;
				} else {
					Statistics[Head+3] = -1;
				}
			}
			break;

		// Time Mode
		case CP_MODE_TIME:
			if (m_Simulated_Data_Flow == 1) {
				Statistics[0] = 654321;
				Statistics[1] = 543210;
			}
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_HeadPresent[Head] == 1) {
					Statistics[Head+3] = 975318;
				} else {
					Statistics[Head+3] = -1;
				}
			}
			break;

		// PassThru Mode
		case CP_MODE_PASSTHRU:
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_HeadPresent[Head] == 1) {
					if (m_SimulatedMode[Head] != -1) Statistics[Head+3] = 975318;
				} else {
					Statistics[Head+3] = -1;
				}
			}
			if (m_Simulated_Data_Flow == 1) {
				for (Head = 0 ; Head < MAX_HEADS ; Head++) Statistics[0] += (Statistics[Head+3] * m_SimulatedPassThru[Head]);
			}
			break;

		// tag, test and unknown
		case CP_MODE_TEST:
		case CP_MODE_TAG:
		default:
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_HeadPresent[Head] == 1) {
					Statistics[Head+3] = 975318;
				} else {
					Statistics[Head+3] = -1;
				}
			}
			break;
		}
		break;
	}

#endif

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Singles
// Purpose:	Return the number of singles each block on the head is producing
// Arguments:	Head			-	long = Detector Head
//				SinglesType		-	long = CFD (0) or Valid (1) singles
//				*Singles		-	long = array of block singles for the head
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Singles(long Head, long SinglesType, long Singles[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Singles";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long first_block = 0;
	long Block = 0;
	long index = 0;

	char *ptr = NULL;

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// simulation - wait 1 second
	if (m_Port == PORT_SIMULATION) {
		Sleep(100);
	}

	// loop through getting singles
	for (first_block = 0 ; first_block < m_NumBlocks ; first_block += m_XBlocks) {

		// if not simulation
		if (m_Port != PORT_SIMULATION) {

			// issue the command
			sprintf (command, "Q %d", (first_block / m_XBlocks));
			*pStatus = Internal_Pass(Head, RETRY, command, response);
			if (*pStatus != 0) return S_FALSE;

			// if it is the first check, then make sure it is the right type
			if (first_block == 0) {

				// strip off command identifier
				index = (Head < 10) ? 5 : 6;

				// if it is valids and cfds requested
				if (((SinglesType == SINGLES_CFD) && (response[index] == 'V')) ||
					((SinglesType == SINGLES_VALIDS) && (response[index] == 'C'))) {

					// change singles mode
					sprintf(command, "V 1 %d", SinglesType);
					*pStatus = Internal_Pass(Head, RETRY, command, response);
					if (*pStatus != 0) return S_FALSE;

					// wait 2 seconds for new statistics
					Sleep(2000);

					// re-issue the command to get singles
					sprintf (command, "Q %d", (first_block / m_XBlocks));
					*pStatus = Internal_Pass(Head, RETRY, command, response);
					if (*pStatus != 0) return S_FALSE;
				}
			}

			// strip off command identifier
			index = (Head < 10) ? 6 : 7;
			if ((first_block / m_XBlocks) > 9) index++;
			ptr = &response[index];

			// break out singles
			for (Block = first_block ; (Block < (first_block + m_XBlocks)) && (Block < m_NumBlocks) && SUCCEEDED(hr) ; Block++) {
				Singles[Block] = strtol(ptr, &ptr, 0);
			}

		// simulation
		} else {
			Sleep(50);
			for (Block = first_block ; (Block < (first_block + m_XBlocks)) && (Block < m_NumBlocks) ; Block++) Singles[Block] = 0;
			if (m_SimulatedMode[Head] != -1) {
				if ((first_block < (m_XBlocks * m_YBlocks)) || ((first_block >= m_TransmissionStart[Head]) && (m_PointSource[Head] == 1))) {
					for (Block = first_block ; (Block < (first_block + m_XBlocks)) && (Block < m_NumBlocks) ; Block++) Singles[Block] = 13933;
				}
			}
		}
	}

	// switch polling type back to valids if type is cfd
	if ((m_Port != PORT_SIMULATION) && (SinglesType == SINGLES_CFD)) {
		sprintf(command, "V 1 1");
		*pStatus = Internal_Pass(Head, RETRY, command, response);

	// simulation
	} else {
		Sleep(50);
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	// return success
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Progress
// Purpose:		Return the percent complete of the current command
// Arguments:	Head			-	long = Detector Head
//				*pPercent		-	long = percent complete executing last command
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				14 Apr 04	T Gremillion	Added to only check status if head is present
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Progress(long Head, long *pPercent, long *pStatus)
{
	// variables needed regardless of which section of code is compiled
	long start_head = 0;
	long end_head = 0;
	long loop_head = 0;

	char Subroutine[80] = "Progress";

	HRESULT hr = S_OK;

	// verify head number against full range
	if (((Head < -2) || (Head >= m_NumHeads)) && (Head != 64)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if ((Head >= 0) && (Head != 64)) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// initialize status
	*pStatus = 0;

	// initialize progress to 100 percent
	*pPercent = 100;

#ifdef ECAT_SCANNER

	// local variables
	long gci_status;

	// not for panel detectors
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Improperly Compiled For Panel Detectors", 0);
		return S_FALSE;
	}

	// no progress on coincidence processor
	if (Head == 64) {
		*pStatus = Add_Error(DAL, Subroutine, "Progress Not Define For Coincidence Processor", Head);
		return S_FALSE;
	}

	// determine range of heads
	start_head = (Head < 0) ? 0 : Head;
	end_head = (Head < 0) ? (m_NumHeads-1) : Head;

	// loop through checking buckets (as soon as one is found still in progress, exit)
	for (loop_head = start_head ; (loop_head <= end_head) && (*pPercent == 100) ; loop_head++) {

		// get a response from the bucket (12 is designated as all heads in the firmware)
		if (1 == m_HeadPresent[loop_head]) gci_status = select_block(loop_head, 12);

		// if the response was not success or still in progress, then it failed
		if ((gci_status != GCI_SUCCESS) && (gci_status != SETUP_IN_PROGRESS)) {
			*pStatus = Add_Error(DAL, Subroutine, "Invalid return code received from PetCTGCIDll", gci_status);
			return S_FALSE;
		}

		// if previous command is still in progress, return a value less than 100
		if (gci_status == SETUP_IN_PROGRESS) *pPercent = 10;
	}

// block code unused for this build
#else

	// local variables
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	unsigned char CP_Response[MAX_STR_LEN];
	unsigned char OS_Response[MAX_STR_LEN];

	long index = 0;
	long HeadPercent = 0;
	long OS_Percent = 0;
	long CP_Percent = 0;
	long ErrorStatus = 0;

	time_t CurrTime = 0;

	// not for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Improperly Compiled For Ring Scanners", 0);
		return S_FALSE;
	}

	// if simulation
	if (m_Port == PORT_SIMULATION) {

		// get current time
		time(&CurrTime);

		// coincidence processor
		if ((Head == -2) || (Head == 64)) {
			Sleep(50);
			if (m_SimulatedFinish[MAX_HEADS] == m_SimulatedStart[MAX_HEADS]) {
				CP_Percent = 100;
			} else {
				CP_Percent = (100 * (CurrTime - m_SimulatedStart[MAX_HEADS])) / (m_SimulatedFinish[MAX_HEADS] - m_SimulatedStart[MAX_HEADS]);
			}
			if (CP_Percent < 0) CP_Percent = 0;
			if (CP_Percent > 100) CP_Percent = 100;

			// we want to show the least amount of progress
			if (CP_Percent < *pPercent) *pPercent = CP_Percent;
		}

		// determine range of heads
		start_head = (Head < 0) ? 0 : Head;
		end_head = (Head < 0) ? (m_NumHeads-1) : Head;

		// loop through heads
		for (loop_head = start_head ; (loop_head <= end_head) && (loop_head != 64) ; loop_head++) {
			Sleep(50);
			if (m_SimulatedFinish[loop_head] == m_SimulatedStart[loop_head]) {
				HeadPercent = 100;
			} else {
				HeadPercent = (100 * (CurrTime - m_SimulatedStart[loop_head])) / (m_SimulatedFinish[loop_head] - m_SimulatedStart[loop_head]);
			}
			if (HeadPercent < 0) HeadPercent = 0;
			if (HeadPercent > 100) HeadPercent = 100;

			// we want to show the least amount of progress
			if (HeadPercent < *pPercent) *pPercent = HeadPercent;
		}

		// return
		return S_OK;
	}

	// ethernet ping of coincidence processor
	if (((Head == 64) || (Head == -2)) && (m_Port == PORT_ETHERNET)) {

		// Add to log file
		Log_Message("m_pOSMain->OS_Ping");

		// issue command to Failsafe Monitor
		hr = m_pOSMain->OS_Ping(OS_Response, (unsigned long *) &OS_Percent, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pOSMain->OS_Ping), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = OS_Error_Query(ErrorStatus);

		// issue command
		if (*pStatus == 0) {

			// Add to log file
			Log_Message("m_pCPMain->CP_Ping");

			hr = m_pCPMain->CP_Ping(CP_Response, &CP_Percent, &ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->CP_Ping), HRESULT", hr);

			// check for error
			if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		}

		// build output
		if (*pStatus == 0) *pPercent = (OS_Percent < CP_Percent) ? OS_Percent : CP_Percent;
	}

	// get head values
	if ((Head != 64) || (m_Port != PORT_ETHERNET)) {

		// build progress command
		sprintf(command, "P 0");

		// if we are checking everything over the serial line
		if ((Head == -2) && (m_Port != PORT_ETHERNET)) {

			// use the pass command to send to the coincidence processor
			*pStatus = Internal_Pass(64, RETRY, command, response);

			// if error, set percent complete to 0% and return
			if (*pStatus != 0) {
				*pPercent = 0;
				return S_FALSE;
			}

			// convert to integer
			*pPercent = atol(&response[3]);
		}

		// determine range of heads
		start_head = (Head < 0) ? 0 : Head;
		end_head = (Head < 0) ? (m_NumHeads-1) : Head;

		// loop through designated heads
		for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {

			// use the pass command to send to the head
			*pStatus = Internal_Pass(loop_head, RETRY, command, response);

			// if error, set percent complete to 0% and return
			if (*pStatus != 0) {
				*pPercent = 0;
				return S_FALSE;
			}

			// convert to integer
			index = (loop_head < 10) ? 2 : 3;
			HeadPercent = atol(&response[index]);

			// we want to show the least amount of progress
			if (HeadPercent < *pPercent) *pPercent = HeadPercent;
		}
	}

#endif

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	// code successfully completed
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Ping
// Purpose:		Return the firmware version of the head or coincidence processor
// Arguments:	Head			-	long = Detector Head
//				*BuildID		-	unsigned char = firmware version
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Ping(long Head, unsigned char BuildID[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Ping";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Percent = 0;
	long ErrorStatus = 0;
	long index = 0;

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	unsigned char CP_Response[MAX_STR_LEN];
	unsigned char OS_Response[MAX_STR_LEN];

	// verify head number against full range
	if (((Head < -1) || (Head >= m_NumHeads)) && (Head != 64)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if ((Head != -1) && (Head != 64)) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// initialize status and build id
	*pStatus = 0;
	sprintf((char *) BuildID, "");

	// ping of the server itself
	if (Head == -1) {

		// fill in build id
		sprintf((char *) BuildID, "Detector Abstraction Layer %s Compiled %s %s", REVISION, __DATE__,  __TIME__);

	// simulation
	} else if (m_Port == PORT_SIMULATION) {

		// fill in build id
		if (Head == 64) {
			sprintf((char *) BuildID, "Coincidence Processor: Simulation %dns", m_Simulated_Coincidence_Window);
		} else {
			sprintf((char *) BuildID, "Head %d: Simulateion", Head);
		}

	// ethernet ping of coincidence processor
	} else if ((Head == 64) && (m_Port == PORT_ETHERNET)) {

		// Add to log file
		Log_Message("m_pOSMain->OS_Ping");

		// issue command to Failsafe Monitor
		hr = m_pOSMain->OS_Ping(OS_Response, (unsigned long *) &Percent, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pOSMain->OS_Ping), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = OS_Error_Query(ErrorStatus);

		// issue command
		if (*pStatus == 0) {

			// Add to log file
			Log_Message("m_CPMain->CP_Ping");

			// send
			hr = m_pCPMain->CP_Ping(CP_Response, &Percent, &ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->CP_Ping), HRESULT", hr);

			// check for error
			if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		}

		// build output
		if (*pStatus == 0) sprintf((char *) BuildID, "OS: %s     CP: %s", (char *) OS_Response, (char *) CP_Response);

	// otherwise issue a serial line command
	} else {

		// build ping command (request build id)
		sprintf(command, "P 1");

		// use the pass command to send to the head
		*pStatus = Internal_Pass(Head, RETRY, command, response);

		// restrict to response
		if (*pStatus == 0) {
			index = (Head < 10) ? 2 : 3;
			sprintf((char *) BuildID, &response[index]);

		// if ping failed, remove from list of heads present
		} else {
			m_HeadPresent[Head] = 0;
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	// code successfully completed
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Report_Temperatures
// Purpose:		Return the allowed temperature range and the current temperature
//				of the coincidence processor, LTIs and DHIs
// Arguments:	*Temperature	-	long = array of temperature values
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Report_Temperature(float Temperature[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Report_Temperature";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	long ErrorStatus = 0;
	long Code = 0;
	long data_index = 3;
	long Head = 0;
	long high_voltage = 0;

	float high = 0.0;
	float low = 0.0;

	double volt_plus5 = 0.0;
	double volt_minus5 = 0.0;
	double volt_24 = 0.0;
	double LocalTemp = 0.0;
	double RemoteTemp = 0.0;

	char *ptr = NULL;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	unsigned char ErrorMsg[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// act on port type
	switch (m_Port) {

	// if the ethernet was being used, issue through the DCOM interface
	case PORT_ETHERNET:

		// Add to log file
		Log_Message("m_pCPMain->Rpt_Temps");

		// issue command
		hr = m_pCPMain->Rpt_Temps(Temperature, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Rpt_Temps), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line - issue separately to coincidence processor and heads
	case PORT_SERIAL:

		// get temperature range
		sprintf(command, "C");
		*pStatus = Internal_Pass(64, RETRY, command, response);

		// if command succeeded, break out values
		if (*pStatus == 0) {

			// fill in the high and low values
			ptr = &response[data_index];
			high = (float) strtod(ptr, &ptr);
			low = (float) strtod(ptr, &ptr);

			// fill in array
			Temperature[0] = high;
			Temperature[1] = low;
			Temperature[2] = 0.0;
		}

		// if not the HRRT (not present), get the LTI Temperatures
		for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) ; Head++) if (m_HeadPresent[Head] == 1) {

			// issue command
			sprintf(command, "A");
			*pStatus = Internal_Pass(Head, RETRY, command, response);

			// if status is a diagnostics error, then re-issue
			if (*pStatus != 0) {

				// get code
				hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

				// diagnostic codes are < -100 and reboot code is -68
				if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, command, response);
			}

			// if status is a diagnostics error, then re-issue
			if (*pStatus != 0) {

				// get code
				hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

				// diagnostic codes are < -100 and reboot code is -68
				if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, command, response);
			}

			// if command succeeded, break out value
			if (*pStatus == 0) {

				// index into output array and string
				data_index = (Head < 10) ? 2 : 3;
				ptr = &response[data_index];

				// voltages (not returned)
				high_voltage = strtol(ptr, &ptr, 0);
				if (SCANNER_HRRT == m_ScannerType) for (i = 0 ; i < 4 ; i++) high_voltage = strtol(ptr, &ptr, 0);
				volt_plus5 = strtod(ptr, &ptr);
				volt_24 = strtod(ptr, &ptr);
				volt_minus5 = strtod(ptr, &ptr);

				// Light Tight Interface value
				if (m_ScannerType != SCANNER_HRRT) {
					Temperature[(Head*2)+4] = (float) strtod(ptr, &ptr);
				} else {
					Temperature[(Head*2)+4] = 0.0;
					LocalTemp = (double) strtod(ptr, &ptr);
					RemoteTemp = (double) strtod(ptr, &ptr);
				}

				// Average the individual tempeatures
				Temperature[(Head*2)+3] = 0.0;
				for (i = 0 ; i < 6 ; i++) Temperature[(Head*2)+3] += (float) strtod(ptr, &ptr);
				Temperature[(Head*2)+3] /= 6.0;
			}
		}
		break;

	case PORT_SIMULATION:
	default:
		Sleep(50);
		Temperature[0] = 100.0;
		Temperature[1] = 0.0;
		Temperature[2] = 29.0;
		for (Head = 0 ; Head < m_NumHeads ; Head++) {
			if (m_HeadPresent[Head] == 1) {
				Sleep(50);
				Temperature[(Head*2)+3] = (float) 30.0 + Head;
				Temperature[(Head*2)+4] = (float) 30.5 + Head;
			} else {
				Temperature[(Head*2)+3] = 0.0;
				Temperature[(Head*2)+4] = 0.0;
			}
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Report_Voltage
// Purpose:		Return the voltages of the DHIs
// Arguments:	*Voltage		-	long = array of temperature values
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Report_Voltage(float Voltage[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Report_Voltage";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	long index = 0;
	long Code = 0;
	long data_index = 3;
	long Head = 0;
	char *ptr = NULL;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];
	unsigned char ErrorMsg[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// simulation - generate values
	if (m_Port == PORT_SIMULATION) {

		// loop through heads
		for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) ; Head++) {
			index = Head * 4;
			if (m_HeadPresent[Head] == 1) {
				Sleep(50);
				if (m_Simulated_High_Voltage == 1) {
					Voltage[index+0] = 1100.0;	// High Voltage
				} else {
					Voltage[index+0] = 0.0;	// High Voltage
				}
				Voltage[index+1] = 5.0;		// Positive 5 Volts
				Voltage[index+2] = 24.0;	// 24 Volts
				Voltage[index+3] = -5.0;	// Negative 5 Volts
			} else {
				Voltage[index+0] = 0.0;		// High Voltage
				Voltage[index+1] = 0.0;		// Positive 5 Volts
				Voltage[index+2] = 0.0;		// 24 Volts
				Voltage[index+3] = 0.0;		// Negative 5 Volts
			}
		}

		// return
		return S_OK;
	}

	// loop through the heads reading voltage
	for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) ; Head++) if (m_HeadPresent[Head] == 1) {

		// issue command
		sprintf(command, "A");
		*pStatus = Internal_Pass(Head, RETRY, command, response);

		// if status is a diagnostics error, then re-issue
		if (*pStatus != 0) {

			// get code
			hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

			// diagnostic codes are < -100 and reboot code is -68
			if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, command, response);
		}

		// if status is a diagnostics error, then re-issue
		if (*pStatus != 0) {

			// get code
			hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

			// diagnostic codes are < -100 and reboot code is -68
			if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, command, response);
		}

		// if command succeeded, break out value
		if (*pStatus == 0) {

			// index into output array and string
			index = Head * 4;
			data_index = (Head < 10) ? 2 : 3;
			ptr = &response[data_index];

			// voltages
			Voltage[index] = (float) strtod(ptr, &ptr);
			if (m_ScannerType == SCANNER_HRRT) {
				for (i = 0 ; i < 4 ; i++) Voltage[index] += (float) strtod(ptr, &ptr);
				Voltage[index] /= 5;
			}
			Voltage[index+1] = (float) strtod(ptr, &ptr);
			Voltage[index+2] = (float) strtod(ptr, &ptr);
			Voltage[index+3] = (float) strtod(ptr, &ptr);
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Report_HRRT_High_Voltage
// Purpose:		Return the multiple high voltages on the HRRT
// Arguments:	*Voltage		-	long = array of temperature values
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Report_HRRT_High_Voltage(long Voltage[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Report_HRRT_High_Voltage";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	long index = 0;
	long Code = 0;
	long data_index = 3;
	long Head = 0;
	char *ptr = NULL;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];
	unsigned char ErrorMsg[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// Only for HRRT
	if (m_ScannerType != SCANNER_HRRT) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// simulation
	if (m_Port == PORT_SIMULATION) {

		// loop through heads
		for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) ; Head++) {
			index = Head * 5;
			for (i = 0 ; i < 5 ; i++) {
				if ((m_HeadPresent[Head] == 1) && (m_Simulated_High_Voltage == 1)) {
					Voltage[index+i] = 1100 + Head;
				} else {
					Voltage[index+i] = 0;
				}
			}
		}

		// return
		return S_OK;
	}

	// loop through the heads reading voltage
	for (Head = 0 ; (Head < m_NumHeads) && (*pStatus == 0) ; Head++) if (m_HeadPresent[Head] == 1) {

		// issue command
		sprintf(command, "A");
		*pStatus = Internal_Pass(Head, RETRY, command, response);

		// if status is a diagnostics error, then re-issue
		if (*pStatus != 0) {

			// get code
			hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

			// diagnostic codes are < -100 and reboot code is -68
			if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, command, response);
		}

		// if status is a diagnostics error, then re-issue
		if (*pStatus != 0) {

			// get code
			hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

			// diagnostic codes are < -100 and reboot code is -68
			if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, command, response);
		}

		// if command succeeded, break out value
		if (*pStatus == 0) {

			// index into output array and string
			index = Head * 5;
			data_index = (Head < 10) ? 2 : 3;
			ptr = &response[data_index];

			// voltages
			for (i = 0 ; i < 5 ; i++) Voltage[index+i] = strtol(ptr, &ptr, 0);
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Hardware_Configuration
// Purpose:	Return a file containing the hardware configuration information
//			for the coincidence processor
// Arguments:	*Filesize		-	long = size of file in bytes
//				*CheckSum		-	unsigned long = checksum of bytes in file
//				**buffer		-	unsigned char = file bytes
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Hardware_Configuration(long *Filesize, unsigned long *CheckSum, unsigned char** buffer, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Hardware_Configuration";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;
	long pass = 0;
	long finished = 0;
	unsigned long i = 0;
	unsigned long local = 0;
	unsigned char *buf;
	char message[MAX_STR_LEN];

	// act on port
	switch (m_Port) {

	// ethernet connection (up to the coincidence processor)
	case PORT_ETHERNET:

		// allow for multiple attempts
		for (pass = 0, finished = 0 ; (pass < 5) && (finished == 0) ; pass++) {

			// Add to log file
			Log_Message("m_pCPMain->HW_Conf");

			// issue command
			hr = m_pCPMain->HW_Conf((unsigned long *) Filesize, CheckSum, buffer, &ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->HW_Conf), HRESULT", hr);

			// check for error
			if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);

			// calculate checksum
			buf = *buffer;
			for (i = 0, local = 0 ; (i < (unsigned long) *Filesize) && (*pStatus == 0) ; i++) local += (unsigned long) buf[i];

			if ((*CheckSum == local) && (*pStatus == 0)) finished = 1;
		}

		// no error, but no data
		if ((finished == 0) && (*pStatus == 0)) {

			// build error string
			if (*Filesize == 0) {
				*pStatus = Add_Error(DAL, Subroutine, "Invalid File Size", *Filesize);
			} else {
				sprintf(message, "File Checksum Error CP = %d Local", *CheckSum);
				*pStatus = Add_Error(DAL, Subroutine, message, local);
			}
		}
		break;

	// not implemented
	default:

		// not yet implemented
		*pStatus = Add_Error(DAL, Subroutine, "Serial Line Method Not Available For This Model, Model", m_ModelNumber);
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Check_Head_Mode
// Purpose:	Return the current data mode of the specified head
// Arguments:	Head				-	long = Detector Head 
//				*pDataMode			-	long = Data Mode
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//				*pConfigiuration	-	long = Electronics Configuration
//				*pLayer				-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined 
//				*pLLD				-	long = lower level discriminator
//				*pULD				-	long = upper level discriminator
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Check_Head_Mode(long Head, long *pDataMode, long *pConfiguration, long *pLayer, long *pLLD, long *pULD, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Check_Head_Mode";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Mode = 0;
	long Code = 0;
	long Layer = 0;
	char *ptr = NULL;
	char *LayerPtr = NULL;
	char response[MAX_STR_LEN];
	unsigned char ErrorMsg[MAX_STR_LEN];

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// simulation
	if (m_Port == PORT_SIMULATION) {
		Sleep(50);
		*pDataMode = m_SimulatedMode[Head];
		*pLayer = m_SimulatedLayer[Head];
		*pConfiguration = m_SimulatedConfig[Head];
		*pLLD = m_SimulatedLLD[Head];
		*pULD = m_SimulatedULD[Head];
		return S_OK;
	}

	// issue command to query head
	*pStatus = Internal_Pass(Head, RETRY, "A", response);

	// if status is a diagnostics error, then re-issue
	if (*pStatus != 0) {

		// get code
		hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

		// diagnostic codes are < -100 and reboot code is -68
		if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, "A", response);
	}

	// if successful, then break out information
	if (*pStatus == 0) {

		// check mode
		for (*pDataMode = -1, Mode = 0 ; (Mode < NUM_DHI_MODES) && (*pDataMode == -1) ; Mode++) {
			ptr = strstr(response, DHIModeStr[Mode]);
			if (ptr != NULL) *pDataMode = Mode;
		}

		// if valid mode was found, then reduce string for further parsing
		if (*pDataMode != -1) {

			// move pointer past mode string
			ptr = strstr(response, DHIModeStr[*pDataMode]);
			ptr = &ptr[strlen(DHIModeStr[*pDataMode])];

			// break out ULD & LLD
			if (SCANNER_HRRT == m_ScannerType) {
				*pLLD = strtol(ptr, &ptr, 0);
				*pULD = strtol(ptr, &ptr, 0);
			} else {
				*pULD = strtol(ptr, &ptr, 0);
				*pLLD = strtol(ptr, &ptr, 0);
			}

			// p39 phase IIA
			if (m_ScannerType == SCANNER_P39_IIA) {

				// always combined layer
				*pLayer = LAYER_ALL;

			// other scanners
			} else {

				// check layer
				LayerPtr = ptr;
				for (*pLayer = -1, Layer = 0 ; (Layer < NUM_LAYER_TYPES) && (*pLayer == -1) ; Layer++) {
					ptr = strstr(LayerPtr, LayerStr[Layer]);
					if (ptr != NULL) *pLayer = Layer;
				}

				// if valid layer was found
				if (*pLayer != -1) {

					// move pointer past layer string
					ptr = strstr(ptr, LayerStr[*pLayer]);
					ptr = &ptr[strlen(DHIModeStr[*pDataMode])];
				}

			}

			// if valid layer was found
			if (*pLayer != -1) {

				// break out configuration
				*pConfiguration = strtol(ptr, &ptr, 0);

			// data mode not found, clear all other values as well
			} else {
				*pDataMode = -1;
				*pConfiguration = -1;
				*pLayer = -1;
				*pLLD = -1;
				*pULD = -1;
			}

		// data mode not found, clear all other values as well
		} else {
			*pDataMode = -1;
			*pConfiguration = -1;
			*pLayer = -1;
			*pLLD = -1;
			*pULD = -1;
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Check_CP_Mode
// Purpose:	Return the current mode of the coincidence processor
// Arguments:	*pMode			-	long = Data Mode
//					0 = Coincidence
//					1 = Passthru
//					2 = Tagword Only
//					3 = Test
//					4 = Time Difference
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Check_CP_Mode(long *pMode, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Check_CP_Mode";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	long ErrorStatus = 0;
	char response[MAX_STR_LEN];

	// initialize status 
	*pStatus = 0;

	// act on port
	switch (m_Port) {

	// ethernet connection, query directly
	case PORT_ETHERNET:

		// Add to log file
		Log_Message("m_pCPMain->CP_Conf");

		// issue command
		hr = m_pCPMain->CP_Conf(pMode, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->CP_Conf), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line
	case PORT_SERIAL:

		// issue command
		*pStatus = Internal_Pass(64, RETRY, "Q", response);

		// if successfull, look for the coincidence processor mode
		*pMode = -1;
		for (i = 0 ; (i < NUM_CP_MODES) && (*pStatus == 0) ; i++) {

			// check for occurrence of string in response
			if (strstr(response, CPModeStr[i]) != NULL) {
				*pMode = i;
				break;
			}
		}
		break;

	case PORT_SIMULATION:
	default:
		Sleep(50);
		*pMode = m_Simulated_CP_Mode;
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Set_Temperature_Limits
// Purpose:	Set the valid temperature range for the CP and DHIs
// Arguments:	Low					-	float = low temperature cutoff value
//				High				-	float = high temperature cutoff value
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Set_Temperature_Limits(float Low, float High, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Set_Temperature_Limits";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];
	char message[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// validate lower limit
	if (Low < 10) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Lower Limit Too Low (Below 10 C), Lower Limit", (long) Low);
		return S_FALSE;
	}

	// validate upper limit
	if (High > 55) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Upper Limit Too High (Above 55 C), Upper Limit", (long) High);
		return S_FALSE;
	}

	// validate lower limit against upper limit
	if (Low >= High) {
		sprintf(response, "Invalid Argument, Lower Limit = %d below Upper Limit", (long) Low);
		*pStatus = Add_Error(DAL, Subroutine, response, (long) High);
		return S_FALSE;
	}

	// act on port type
	switch (m_Port) {

	// if the ethernet was being used, issue through the DCOM interface
	case PORT_ETHERNET:

		// Add to log file
		sprintf(message, "m_pCPMain->Set_Temps - Low: %d High %d", Low, High);
		Log_Message(message);

		// issue command
		hr = m_pCPMain->Set_Temps((float)(Low+0.5), (float)(High+0.5), &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Set_Temps), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial command to coincidence processor
	case PORT_SERIAL:

		// set temperature range
		sprintf(command, "C %f %f", High, Low);
		*pStatus = Internal_Pass(64, RETRY, command, response);
		break;

	case PORT_SIMULATION:
	default:
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Reset_CP
// Purpose:	Reset a portion (or all) of the coincidence processor
// Arguments:	Reset				-	long = Bitfield specifing portion of CP to reset
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Reset_CP(long Reset, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Reset_CP";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 1;
	long ErrorStatus = 0;
	long bit = 0;

	time_t CurrTime = 0;

	char response[MAX_STR_LEN];
	char message[MAX_STR_LEN];

	// initialize status 
	*pStatus = 0;

	// act on port type
	switch (m_Port) {

	// ethernet connection
	case PORT_ETHERNET:

		// Add to log file
		sprintf(message, "m_pCPMain->CP_Reset: %d", Reset);
		Log_Message(message);

		// issue command
		hr = m_pCPMain->CP_Reset(Reset, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->CP_Reset), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);

		// if successfull and reset will reboot, then release servers and reacquire
		if ((*pStatus == 0) && ((Reset & 1) != 0)) {
			Release_Servers();
			Sleep(210000);
			Init_Servers();
		}
		break;

	// serial line
	case PORT_SERIAL:

		// errors for resets that can't be done via serial line
		for (i = 1 ; i < 6 ; i++) {

			// mask out
			bit = (Reset >> i) & 1;

			// check if set
			if (bit == 1) {
				*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Reset Mode Not Available With Serial Communications, Reset Mode", Reset);
				return S_FALSE;
			}
		}

		// a reboot is all we are capable of via serial line
		bit = Reset & 1;
		if (bit == 1) *pStatus = Internal_Pass(64, RETRY, "R 999", response);
		break;

	// simulation
	case PORT_SIMULATION:
	default:
		bit = Reset & 1;
		if (bit == 1) {
			time(&CurrTime);
			m_SimulatedStart[MAX_HEADS] = CurrTime;
			m_SimulatedFinish[MAX_HEADS] = CurrTime + 240;
		} else {
			Sleep(100);
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Reset_Head
// Arguments:	Head				-	long = Detector Head
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
// Purpose:	Restart a DHI
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Reset_Head(long Head, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Reset_Head";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long loop_head = 0;
	long start_head = 0;
	long end_head = 0;

	time_t CurrTime = 0;

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// initialize status
	*pStatus = 0;

	// verify head number against full range
	if ((Head < -1) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != -1) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// build the reset command
	sprintf(command, "R 999");

	// determine start and stop head
	start_head = (Head < 0) ? 0 : Head;
	end_head = (Head < 0) ? (m_NumHeads-1) : Head;

	// loop through heads issuing command
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {

		// not simulation
		if (m_Port != PORT_SIMULATION) {
			*pStatus = Internal_Pass(loop_head, RETRY, command, response);

		// simulation 
		} else {
			time(&CurrTime);
			m_SimulatedStart[loop_head] = CurrTime;
			m_SimulatedFinish[loop_head] = CurrTime + 60;
			m_SimulatedMode[loop_head] = -1;
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Load_RAM
// Purpose:	Load analog settings, crystal peaks or time offsets from a file on the DHI
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				RAM_Type		-	long = values to be reset
//					0 = Energy
//					1 = Time
//					2 = Analog Settings
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				14 Apr 04	T Gremillion	Activated for ring scanners (Energy Peaks & Time Correction)
//				09 Jun 04	T Gremillion	Removed poll for completion for ring scanners
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Load_RAM(long Configuration, long Head, long RAM_Type, long *pStatus)
{
	// local variables
	long start_head = 0;
	long end_head = 0;
	long loop_head = 0;
	long Percent = 0;

	char Subroutine[80] = "Load_RAM";
	char type_str[NUM_RAM_TYPES][MAX_STR_LEN] = {"pk", "tm", "analog"};

	HRESULT hr = S_OK;

	// verify Configuration
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < -1) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != -1) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// initialize status
	*pStatus = 0;

	// determine range of heads
	start_head = (Head < 0) ? 0 : Head;
	end_head = (Head < 0) ? (m_NumHeads-1) : Head;

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// verify valid RAM type
	if ((RAM_Type != RAM_ENERGY_PEAKS) && (RAM_Type != RAM_TIME_OFFSETS) && (RAM_Type != RAM_ANALOG_SETTINGS)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, RAM Type", RAM_Type);
		return S_FALSE;
	}

	// time offsets not allowed for HRRT
	if ((SCANNER_HRRT == m_ScannerType) && (RAM_TIME_OFFSETS == RAM_Type)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument For HRRT, RAM Type", RAM_Type);
		return S_FALSE;
	}

	// simulation, wait a few seconds and return
	if (m_Port == PORT_SIMULATION) {
		m_SimulatedMode[Head] = -1;
		Sleep(3000);
		return S_OK;
	}

	// only one file for analog settings
	if (RAM_Type == RAM_ANALOG_SETTINGS) {

		// build command
		sprintf(command, "L %d %d %d_%s.txt", RAM_Type, Configuration, Configuration, type_str[RAM_Type]);

		// loop through designated heads
		for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {
			*pStatus = Internal_Pass(loop_head, NORETRY, command, response);
		}

		// wait one second before first check
		Sleep(1000);

		// wait for completion
		for (Percent = 0 ; SUCCEEDED(hr) && (Percent < 100) && (*pStatus == 0) ; ) {
			hr = Progress(Head, &Percent, pStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);
			if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
		}

		// the analog settings can no longer be trusted
		for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) {
			if (m_HeadPresent[loop_head] == 1) m_Trust[(Configuration*MAX_HEADS)+loop_head] = 0;
		}

	// other RAM types divided into fast and slow (both for P39 Phase IIA electronics)
	} else {

		// phase IIA electronics are both
		if (m_ScannerType == SCANNER_P39_IIA) {

			// build command
			sprintf(command, "L %d %d %d_%sboth.txt", RAM_Type, Configuration, Configuration, type_str[RAM_Type]);

			// loop through designated heads
			for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {
				*pStatus = Internal_Pass(loop_head, NORETRY, command, response);
			}

			// wait one second before first check
			Sleep(1000);

			// wait for completion
			for (Percent = 0 ; SUCCEEDED(hr) && (Percent < 100) && (*pStatus == 0) ; ) {
				hr = Progress(Head, &Percent, pStatus);
				if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);
				if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
			}
		}

		// fast layer for everything exect SPECT configurations on PETSPECT
		if ((m_ScannerType != SCANNER_P39_IIA) && ((Configuration == 0) || (m_ScannerType != SCANNER_PETSPECT))) {

			// build command
			sprintf(command, "L %d %d %d_%sfast.txt", RAM_Type, Configuration, Configuration, type_str[RAM_Type]);

			// loop through designated heads
			for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {
				*pStatus = Internal_Pass(loop_head, NORETRY, command, response);
			}

			// wait one second before first check
			Sleep(1000);

			// wait for completion
			for (Percent = 0 ; SUCCEEDED(hr) && (Percent < 100) && (*pStatus == 0) ; ) {
				hr = Progress(Head, &Percent, pStatus);
				if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);
				if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
			}
		}

		// slow layer if it exists
		if (*pStatus == 0) {

			// build command
			sprintf(command, "L %d %d %d_%sslow.txt", RAM_Type, Configuration, Configuration, type_str[RAM_Type]);

			// loop through designated heads
			for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) if (m_HeadPresent[loop_head] == 1) {
				if (m_NumLayers[loop_head] > 1) *pStatus = Internal_Pass(loop_head, NORETRY, command, response);
			}

			// wait one second before first check
			Sleep(1000);

			// wait for completion
			for (Percent = 0 ; SUCCEEDED(hr) && (Percent < 100) && (*pStatus == 0) ; ) {
				hr = Progress(Head, &Percent, pStatus);
				if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Progress), HRESULT", hr);
				if ((Percent < 100) && (*pStatus == 0)) Sleep(1000);
			}
		}
	}

#else

	// loop through selected heads
	for (loop_head = start_head ; (loop_head <= end_head) && (*pStatus == 0) ; loop_head++) {

		// load through PetCTGCIDll
		if (load_flash_ram(loop_head) != GCI_SUCCESS) 
			*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::load_time_offsets, Head", loop_head);
	}

#endif

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Time_Window
// Arguments:	WindowSize		-	long = Size of coincidence window in timing bins
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
// Purpose:	Set the coincidence time window
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Time_Window(long WindowSize, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Time_Window";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long i = 0;
	long ResponseLen = 0;
	long Percent = 0;
	long CurrentSize = -1;
	long CPMode = -1;
	long Changed = 0;
	long ShiftPair = 0;
	long ErrorStatus = 0;
	long RemainingTime = 0;

	time_t CurrTime = 0;

	char message[MAX_STR_LEN];

	unsigned char CP_Response[MAX_STR_LEN];

	// make sure head number is non-negative - CP will do the rest of the validation
	if (WindowSize < 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Window Size", WindowSize);
		return S_FALSE;
	}

	// act on port type
	switch (m_Port) {

	// ethernet connection
	case PORT_ETHERNET:

		// get the current mode of the coincidence processor
		hr = Check_CP_Mode(&CPMode, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Check_CP_Mode), HRESULT", hr);

		// current coincidence window
		if (*pStatus == 0) {

			// if in coincidence mode
			if (CPMode == CP_MODE_COINC) {

				// get it from the coincidence processor
				hr = m_pCPMain->CP_Ping(CP_Response, &Percent, &ErrorStatus);
				if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->CP_Ping), HRESULT", hr);
				if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);

				// break out the window from the string
				if (*pStatus == 0) {

					// length of response
					ResponseLen = strlen((char *) CP_Response);

					// skip "ns time window." (replace it with end of string
					CP_Response[ResponseLen-15] = 0;

					// find space before number of nanoseconds
					for (i = (ResponseLen-16) ; (i > 0) && (CP_Response[i] != ' ') ; i--);

					// current window
					CurrentSize = atol((char *) &CP_Response[i]);
				}

			// otherwise, setting the time window will not cost anything, so play it safe
			} else {
				CurrentSize = -1;
			}
		}

		// if change the window
		if ((*pStatus == 0) && (CurrentSize != WindowSize)) {

			// Add to log file
			sprintf(message, "m_pCPMain->Time_Window: %d", WindowSize);
			Log_Message(message);

			// issue command
			hr = m_pCPMain->Time_Window(WindowSize, &ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Time_Window), HRESULT", hr);
			if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = OS_Error_Query(ErrorStatus);

			// note that the window has been changed
			if (*pStatus == 0) Changed = 1;
		}

		// if the window has changed and the CP is in coincidence mode, issue the command to re-enter coincidence mode
		if ((*pStatus == 0) && (Changed == 1) && (CPMode == CP_MODE_COINC)) {

			// bit shift stored module pairs to match up with coincidence processor
			ShiftPair = m_ModulePairs >> 1;

			// issue command
			hr = m_pCPMain->Set_Mode(CP_MODE_COINC, ShiftPair, &ErrorStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Set_Mode), HRESULT", hr);
			if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		}
		break;

	// send over serial line
	case PORT_SERIAL:

		// Not Implemented
		*pStatus = Add_Error(DAL, Subroutine, "Serial Line Method Not Available For This Model, Model", m_ModelNumber);
		break;

	case PORT_SIMULATION:
	default:

		// time remaining for previous command
		time(&CurrTime);
		RemainingTime = m_SimulatedFinish[MAX_HEADS] - CurrTime;

		// error if background task is already being executed
		if (RemainingTime > 0) {
			*pStatus = Add_Error(DAL, Subroutine, "Simulated Background Command In Progress, Remaining Time", RemainingTime);

		// command executed
		} else {

			// get the current time and use it as the start time for simulated progress
			m_SimulatedStart[MAX_HEADS] = CurrTime;
			m_SimulatedFinish[MAX_HEADS] = CurrTime + 2;

			// simulate getting current mode
			Sleep(100);

			// if current mode is coincidence, then simulate getting current window
			if (m_Simulated_CP_Mode == CP_MODE_COINC) Sleep(100);

			// change window
			if (m_Simulated_Coincidence_Window != WindowSize) Sleep(100);

			// put back in coincidence
			if ((m_Simulated_CP_Mode == CP_MODE_COINC) && (m_Simulated_Coincidence_Window != WindowSize)) {
				Sleep(100);
				m_SimulatedFinish[MAX_HEADS] = CurrTime + 90;
			}
		}
		break;
	}

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	OS_Command
// Purpose:	execute an operating system command
// Arguments:	Head			-	long = Detector Head 
//				*CommandString	-	unsigned char = command to execute on head
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::OS_Command(long Head, unsigned char CommandString[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "OS_Command";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	unsigned char ComCommand[MAX_STR_LEN];
	char message[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// for unknown reasons, if the following two variables are moved errors could occur
	long ErrorStatus = 0;
	long Result = 0;

	// verify head number against full range
	if (((Head < 0) || (Head >= m_NumHeads)) && (Head != 64)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != 64) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// simulation
	if (m_Port == PORT_SIMULATION) {
		Sleep(1000);
		return S_OK;
	}

	// ethernet connection
	if ((Head == 64) && (m_Port == PORT_ETHERNET)) {

		// add a newline character
		sprintf((char *) ComCommand, "%s", CommandString);

		// Add to log file
		sprintf(message, "m_pOSMain->OS_OP: %s", CommandString);
		Log_Message(message);

		// issue command
		hr = m_pOSMain->OS_OP(ComCommand, &Result, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->OS_OP), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = OS_Error_Query(ErrorStatus);

	// otherwise, send over serial line
	} else {

		// build command
		sprintf(command, "O %s", CommandString);

		// execute command
		*pStatus = Internal_Pass(Head, RETRY, command, response);
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Pass_Command
// Purpose:	issue a low level firmware command to the DHI
// Arguments:	Head			-	long = Detector Head 
//				*CommandString	-	unsigned char = firmware command to execute on head
//				*ResponseString	-	unsigned char = firmware response
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Pass_Command(long Head, unsigned char CommandString[], unsigned char ResponseString[], long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Pass_Command";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// verify head number against full range
	if (((Head < 0) || (Head >= m_NumHeads)) && (Head != 64)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (Head != 64) if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// pass command does not work to coincidence processor when using DCOM server
	if ((Head == 64) && (m_Port == PORT_ETHERNET)) {
		*pStatus = Add_Error(DAL, Subroutine, "Ethernet Method Not Available To Coincidence Processor For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// simulation
	if (m_Port == PORT_SIMULATION) {
		Sleep(1000);

	// for real
	} else {

		// use internal routine
		*pStatus = Internal_Pass(Head, NORETRY, (char *) CommandString, (char *) ResponseString);
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Tag_Insert
// Purpose:	Insert tagword(s) into the data stream
// Arguments:	Size			-	long = Number of tagwords to insert 
//				**Tagword		-	ULONGLONG = array of tagwords
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Tag_Insert(long Size, ULONGLONG** Tagword, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Tag_Insert";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long ErrorStatus = 0;

	// initialize status 
	*pStatus = 0;

	// act on port type
	switch (m_Port) {

	// ethernet connection
	case PORT_ETHERNET:

		// Add to log file
		Log_Message("m_pCPMain->Tag_Insert");

		// issue command
		hr = m_pCPMain->Tag_Insert(Size, Tagword, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Tag_Insert), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line - not defined
	case PORT_SERIAL:

		// not yet implemented
		*pStatus = Add_Error(DAL, Subroutine, "Serial Line Method Not Available For This Model, Model", m_ModelNumber);
		break;

	// simulation
	case PORT_SIMULATION:
	default:
		Sleep(2000);
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Tag_Control
// Purpose:	turn on/off singles and time tags in data stream
// Arguments:	SinglesTag_OffOn	-	long = singles tagword insertion state flag 
//				TimeTag_OffOn		-	long = time tagword insertion state flag 
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				31 Mar 04	T Gremillion	added control of singles polling
//											for ring based scanners
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Tag_Control(long SinglesTag_OffOn, long TimeTag_OffOn, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Tag_Control";
	HRESULT hr = S_OK;

	// verify the singles flag
	if ((SinglesTag_OffOn < 0) || (SinglesTag_OffOn > 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Singles Tag State", SinglesTag_OffOn);
		return S_FALSE;
	}

// block code unused for this build
#ifdef ECAT_SCANNER

	// act on the singles polling flag
	if (0 == SinglesTag_OffOn) {
		disable_singles_polling();
	} else {
		enable_singles_polling();
	}

#else

	// local variables
	long ErrorStatus = 0;
	char message[MAX_STR_LEN];

	// verify the time flag
	if ((TimeTag_OffOn < 0) || (TimeTag_OffOn > 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Time Tag State", TimeTag_OffOn);
		return S_FALSE;
	}

	// initialize status 
	*pStatus = 0;

	// act on port type
	switch (m_Port) {

	// ethernet connection
	case PORT_ETHERNET:

		// Add to log file
		sprintf(message, "m_pCPMain->Tag_Control - SinglesTags: %d TimeTags: %d", SinglesTag_OffOn, TimeTag_OffOn);
		Log_Message(message);

		// issue command
		hr = m_pCPMain->Tag_Control(SinglesTag_OffOn, TimeTag_OffOn, &ErrorStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->Tag_Control), HRESULT", hr);

		// check for error
		if ((*pStatus == 0) && (ErrorStatus != 0)) *pStatus = CP_Error_Query(ErrorStatus);
		break;

	// serial line not defined
	case PORT_SERIAL:

		// not yet implemented
		*pStatus = Add_Error(DAL, Subroutine, "Serial Line Method Not Available For This Model, Model", m_ModelNumber);
		break;

	// simulation
	case PORT_SIMULATION:
	default:
		Sleep(50);
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Transmission_Trajectory
// Purpose:		Set the movement of the transmission source for the HRRT
// Arguments:	Transaxial_Speed	-	long
//				Axial_Step			-	long
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Transmission_Trajectory(long Transaxial_Speed, long Axial_Step, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Transmission_Trajectory";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];

	// HRRT Only
	if (m_ScannerType != SCANNER_HRRT) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify the transaxial speed
	if ((Transaxial_Speed < 10) || (Transaxial_Speed > 100)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Transaxial Speed", Transaxial_Speed);
		return S_FALSE;
	}

	// verify the time flag
	if ((Axial_Step < 1) || (Axial_Step > 30)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Axial Step", Axial_Step);
		return S_FALSE;
	}

	// initialize status 
	*pStatus = 0;

	// act on port type
	switch (m_Port) {

	// ethernet connection
	case PORT_ETHERNET:

		// not yet implemented
		*pStatus = Add_Error(DAL, Subroutine, "Ethernet Method Not Available For This Model, Model", m_ModelNumber);
		break;

	// serial line
	case PORT_SERIAL:

		// build command
		sprintf(command, "T %d %d", Transaxial_Speed, Axial_Step);

		// execute command
		*pStatus = Internal_Pass(64, RETRY, command, response);
		break;

	// simulation
	case PORT_SIMULATION:
	default:
		Sleep(100);
		break;
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Diagnostics
// Purpose:		Coincidence Processor Diagnostics
// Arguments:	*pResult			-	long
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Diagnostics(long *pResult, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Diagnostics";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// initialize status 
	*pStatus = 0;

	// not yet implemented
	*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Health_Check
// Purpose:		identify any failed components in the Detector system
// Arguments:	*pNumFailed			-	long = number of failed components
//				**Failed			-	long = array identifying failed components
//				*pStatus			-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Health_Check(long *pNumFailed, long **Failed, long *pStatus)
{
	// name of subroutine and HRESULT needed even if main body of code not compiled
	char Subroutine[80] = "Health_Check";
	HRESULT hr = S_OK;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return S_FALSE;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Head = 0;
	long Next = 0;
	long Status = 0;
	long PassFail = 1;

	long List[MAX_HEADS+1];

	unsigned char BuildID[MAX_STR_LEN];

	// Initialize to everything ok
	*pNumFailed = 0;
	*Failed = NULL;
	for (Head = 0 ; Head <= MAX_HEADS ; Head++) List[Head] = 0;

	// check the coincidence processor (uses head 64 as an address)
	hr = Ping(64, BuildID, &Status);
	if (Status == 0) if (FAILED(hr)) Status = -1;
	if (Status != 0) {
		PassFail = 0;
		List[MAX_HEADS] = 1;
		*pNumFailed++;
	}

	// loop through heads
	for (Head = 0 ; (Head < MAX_HEADS) && (List[MAX_HEADS] == 0) ; Head++) {

		// if the head is present
		if (m_HeadPresent[Head] == 1) {

			// check the head
			Status = 0;
			hr = Ping(Head, BuildID, &Status);
			if (Status == 0) if (FAILED(hr)) Status = -1;
			if (Status != 0) {
				PassFail = 0;
				List[Head] = 1;
				*pNumFailed++;
			}
		}
	}

	// if a component failed
	if (*pNumFailed > 0) {

		// indicate in return value
		hr = S_FALSE;

		// allocate list
		*Failed = (long *) ::CoTaskMemAlloc(*pNumFailed);
		if (*Failed == NULL) *pStatus = Add_Error(DAL, Subroutine, "Allocating Memory, Requested Size", *pNumFailed);

		// fill in list
		if (*Failed != NULL) {
			if (List[MAX_HEADS] == 1) *Failed[Next++] = 64;
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (List[Head] == 1) *Failed[Next++] = Head;
			}
		}
	}

#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Close_Globals
// Purpose:		close log file and save persistant values for HRRT
// Arguments:	None
// Returns:		long Status
// History:		01 Nov 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

void CAbstract_DHI::Close_Globals()
{
	// local variables
	char filename[MAX_STR_LEN];
	FILE *fp = NULL;

	// release coincidence processor
	switch (m_Port) {

	// if the ethernet was being used, release the COM servers
	case PORT_ETHERNET:
		Release_Servers();
		break;

	// serial line nothing to worry about
	case PORT_SERIAL:
	default:
		break;
	}

	// if the scanner is an HRRT save some persistant data
	if (SCANNER_HRRT == m_ScannerType) {

		// create a persistant values file
		sprintf(filename, "%s\\DAL_Persistant.bin", m_RootPath);
		if ((fp = fopen(filename, "wb")) != NULL) {

			// write the values
			fwrite((void *) &m_LogState, sizeof(m_LogState), 1, fp);
			fwrite((void *) m_LastBlock, sizeof(long), MAX_HEADS, fp);
			fwrite((void *) m_LastMode, sizeof(long), MAX_HEADS, fp);

			// close the file
			fclose(fp);
		}
	}

	// if the log file is open, close it
	if (NULL != m_LogFile) fclose(m_LogFile);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Init_Globals
// Purpose:		Retrieve values from the gantry model and set global values
// Arguments:	None
// Returns:		long Status
// History:		15 Sep 03	T Gremillion	History Started
//				29 Sep 03	T Gremillion	Added LYSO/LSO heads
//				03 Mar 04	T Gremillion	changed scanner type determination
//											to use case statement for panel detectors
//											and electronics phase for ring scanners.
//											Also added new ASIC settings for phase II
//											electronics and global variable containing
//											location of setup directory.
//				14 Apr 04	T Gremillion	added timing windows and loading of existing
//											settings information for ring scanners.
//				09 Jun 04	T Gremillion	Ring scanners log is in Service\Log
//				19 Oct 04	T Gremillion	identify settings file order
//											remove log entries older than 30 days.
//											read existing settings from file.
//				22 Nov 04	T Gremillion	pull analog settings from gantry model
//				23 Nov 04	T Gremillion	moved Configuration variable to be available 
//											for all scanner types
//				08 Mar 05	T Gremillion	get time window sizes from gantry model
//				17 Mar 05	T Gremillion	was using wrong value for time align window
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::Init_Globals()
{
	// local variables
	long i = 0;
	long j = 0;
	long gci_status = 0;
	long Status = 0;
	long NumFound = 0;
	long Bucket = 0;
	long Block = 0;
	long NextOut = 0;

	double TimingBinSize = 0.0;
	double CoincidenceWindow = 0.0;

	struct _stat file_status;

	char filename[MAX_STR_LEN];
	char oldfile[MAX_STR_LEN];
	char TitleStr[MAX_STR_LEN];
	char LogFile[MAX_STR_LEN];
	char DayStr[MAX_STR_LEN];
	char MonthStr[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Init_Globals";

	struct tm *timestruct;
	time_t currtime;
	time_t logtime;
	time_t ThirtyDays = 60 * 60 * 24 * 30;

	FILE *fp = NULL;

	HRESULT hr = S_OK;

	// instantiate a Gantry Object Model object
	CGantryModel model;
	pConf Config;

	// initialize global variables
	m_SecuritySet = 0;
	m_SettingsInitialized = 0;
	m_ModulePairs = 0;
	m_Transmission = 0;
	m_NextError = 1;
	sprintf(m_ErrorArray[0], "Valid Response");

	// set gantry model to model number held in register
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) return 1;

	// set gantry model
	m_ModelNumber = model.modelNumber();

	// number of blocks
	m_NumBlocks = model.blocks();

	// initialize Head arrays
	for (i = 0 ; i < MAX_HEADS ; i++) {
		for (j = 0 ; j < MAX_CONFIGS ; j++) m_Trust[(j*MAX_HEADS)+i] = 0;
		m_HeadPresent[i] = 0;
		m_PointSource[i] = 0;
		m_TransmissionStart[i] = 0;
		m_NumLayers[i] = 1;
		m_LastMode[i] = -99;
		m_LastBlock[i] = -99;
	}

	// initialize index settings
	for (i = 0 ; i < MAX_SETTINGS ; i++) SettingNum[i] = -1;

	// designate the log file
#ifdef ECAT_SCANNER
	if (getenv("SERVICEDIR") != NULL) {
		sprintf(LogFile, "%s\\Log\\Abstract_DHI.log", getenv("SERVICEDIR"));
#else
	if (getenv("LOGFILEDIR") != NULL) {
		sprintf(LogFile, "%s\\Abstract_DHI.log", getenv("LOGFILEDIR"));
#endif
	} else {
		sprintf(LogFile, "Abstract_DHI.log");
	}

	// move the existing file over
	sprintf(oldfile, "%s.old", LogFile);
	rename(LogFile, oldfile);

	// open the new file for writing
	m_LogFile = fopen(LogFile, "wt");

	// open the old file to be parsed
	fp = fopen(oldfile, "rt");
	if (fp != NULL) {

		// get the current time
		time(&currtime);
		timestruct = localtime(&currtime);

		// loop through old file
		while (!feof(fp)) {

			// read a line
			if (NULL != fgets(Str, MAX_STR_LEN, fp)) {

				// break out string
				sscanf(Str, "%3s %3s %2d %2d:%2d:%2d %4d", 
						DayStr, MonthStr, &timestruct->tm_mday, &timestruct->tm_hour, &timestruct->tm_min, &timestruct->tm_sec, &timestruct->tm_year);
				timestruct->tm_year -= 1900;

				// determine month
				for (timestruct->tm_mon = 0 ; timestruct->tm_mon <= 11 ; timestruct->tm_mon++) {
					if (!strcmp(Month[timestruct->tm_mon], MonthStr)) break;
				}

				// convert time to common format
				logtime = mktime(timestruct);

				// if the entry is more than 30 days old, skip it
				if ((currtime - logtime) > ThirtyDays) continue;

				// write the line
				fprintf(m_LogFile, Str);
			}
		}

		// release the old file and remove it
		fclose(fp);
		remove(oldfile);
	}

// ECAT (Ring) Scanners
#ifdef ECAT_SCANNER

	// set the electronics phase and scanner type
	m_Phase = model.electronicsPhase();
	m_ScannerType = SCANNER_RING;

	// only works for phase II electronics
	if (m_Phase == PHASE_ONE_ELECT) {

		// add message to log
		sprintf(Str, "Ring Scanner Electronics Phase Not Supported", m_Phase);
		Log_Message(Str);

		// return bad status
		return 1;
	}

	// number of emission blocks in each dimension
	m_XBlocks = model.transBlocks();
	m_YBlocks = model.axialBlocks();
	m_XCrystals = model.angularCrystals();
	m_YCrystals = model.axialCrystals();

	// pull timing information
	TimingBinSize = model.timeBitpsec();
	CoincidenceWindow = model.coincidenceWindow();

	// turn on the heads in sequence
	m_NumHeads = model.buckets();

	// loop through the buckets to determine which ones are really there
	for (i = 0 ; i < m_NumHeads ; i++) {

		// send a select all blocks (12) command to the bucket
		gci_status = select_block(i, 12);

		// if it returns with SUCCESS or SETTING_UP, the bucket is there
		if ((GCI_SUCCESS == gci_status) || (SETUP_IN_PROGRESS == gci_status))  {
			m_HeadPresent[i] = 1;
			NumFound++;
		} else {
			m_HeadPresent[i] = 0;
		}
	}

	// if no buckets were found, error out
	if (NumFound == 0) Add_Warning(DAL, Subroutine, "No Buckets Detected", 0);

	// timing windows
	m_MaxTimingWindow = (model.timingBins() - 1) / 2;
	m_DefaultTimingWindow = ((long)(CoincidenceWindow / TimingBinSize) - 1) / 2;

	// method for communicating with coincidence processor
	m_Port = PORT_GCI;

	// needs to come from gantry model
	m_NumDHITemps = 1;

	// communications interface name
	sprintf(m_CP_Name, "PetCTGCIdll");

	// get setup directory
	if (getenv("CPS_SETUP_DIR") == NULL) {
		sprintf(m_RootPath, "C:\\CPS\\Setup\\");
	} else {
		sprintf(m_RootPath, "%s", getenv("CPS_SETUP_DIR"));
	}

// Panel detectors (HRRT, PETSPECT, P39)
#else
	pHead Head;		

	// set gantry type from model number
	switch (m_ModelNumber) {

	// PETSPECT
	case 311:
		m_Phase = 1;
		m_ScannerType = SCANNER_PETSPECT;
		break;

	// HRRT
	case 328:
		m_Phase = 1;
		m_ScannerType = SCANNER_HRRT;
		break;

	// P39 family (391 - 396)
	case 391:
	case 392:
	case 393:
	case 394:
	case 395:
	case 396:
		m_Phase = 1;
		m_ScannerType = SCANNER_P39;
		break;

	// P39 IIA family (2391 - 2396)
	case 2391:
	case 2392:
	case 2393:
	case 2394:
	case 2395:
	case 2396:
		m_Phase = 2;
		m_ScannerType = SCANNER_P39_IIA;
		break;

	// not a panel detector - return error
	default:

		// add message to log
		sprintf(Str, "Model Not Recognized: %d", m_ModelNumber);
		Log_Message(Str);

		// return bad status
		return 1;
		break;
	}

	// number of emission blocks in each dimension
	m_Rotated = model.isHeadRotated();
	if (m_Rotated) {
		m_XBlocks = model.axialBlocks();
		m_YBlocks = model.transBlocks();
		m_XCrystals = model.axialCrystals();
		m_YCrystals = model.angularCrystals();
	} else {
		m_XBlocks = model.transBlocks();
		m_YBlocks = model.axialBlocks();
		m_XCrystals = model.angularCrystals();
		m_YCrystals = model.axialCrystals();
	}

	// get the data for the next head
	Head = model.headInfo();

	// set head values
	for (m_NumHeads = 0, i = 0 ; i < model.nHeads() ; i++) {

		// keeping track of the highest head number
		if (Head[i].address >= m_NumHeads) m_NumHeads = Head[i].address + 1;

		// indicate that the head is present
		m_HeadPresent[Head[i].address] = 1;

		// indicate if transmission sources are hooked up to this head
		if ((Head[i].pointSrcData != 0) && (model.nPointSources(Head[i].address) != 0)) {
			m_PointSource[Head[i].address] = 1;
			m_TransmissionStart[Head[i].address] = model.pointSourceStart(Head[i].address);
		}

		// number of layers
		switch (Head[i].type) {

		case HEAD_TYPE_LSO_ONLY:
			m_NumLayers[Head[i].address] = 1;
			break;

		case HEAD_TYPE_LSO_LSO:
		case HEAD_TYPE_LSO_NAI:
		case HEAD_TYPE_GSO_LSO:
		case HEAD_TYPE_LYSO_LSO:
			m_NumLayers[Head[i].address] = 2;
			break;

		// default head type is single layer
		default:
			m_NumLayers[Head[i].address] = 1;
			break;

		}
	}

	// module pairs
	m_Module_Em = model.emModulePair();
	m_Module_Tx = model.txModulePair();
	m_Module_EmTx = model.emtxModulePair();

	// needs to come from gantry model
	m_NumDHITemps = 6;

	// method for communicating with coincidence processor
	m_Port = model.cpComm();

	// get coincidence processor name from gantry model
	sprintf(m_CP_Name, "%s", model.cpName());
	for (i = 0 ; i < MAX_STR_LEN ; i++) m_Coincidence_Processor_Name[i] = m_CP_Name[i];

	// get setup directory
	sprintf(m_RootPath, "%s", model.SetupDir());

#endif

	// configuration values
	m_NumConfigs = model.nConfig();
	Config = model.configInfo();
	for (i = 0 ; i < m_NumConfigs ; i++) m_Energy[i] = Config[i].energy;

	// analog card settings
	m_NumSettings = model.nAnalogSettings();
	for (i = 0 ; i < MAX_SETTINGS ; i++) SettingNum[i] = -1;
	SettingNum[PMTA] = model.pmtaSetting();
	SettingNum[PMTB] = model.pmtbSetting();
	SettingNum[PMTC] = model.pmtcSetting();
	SettingNum[PMTD] = model.pmtdSetting();
	SettingNum[CFD] = model.cfdSetting();
	SettingNum[CFD_DELAY] = model.cfdDelaySetting();
	SettingNum[X_OFFSET] = model.xOffsetSetting();
	SettingNum[Y_OFFSET] = model.yOffsetSetting();
	SettingNum[E_OFFSET] = model.energyOffsetSetting();
	SettingNum[DHI_MODE] = model.dhiModeSetting();
	SettingNum[SETUP_ENG] = model.setupEnergySetting();
	SettingNum[SHAPE] = model.shapeThresholdSetting();
	SettingNum[SLOW_LOW] = model.slowLowSetting();
	SettingNum[SLOW_HIGH] = model.slowHighSetting();
	SettingNum[FAST_LOW] = model.fastLowSetting();
	SettingNum[FAST_HIGH] = model.fastHighSetting();
	SettingNum[CFD_SETUP] = model.cfdSetupSetting();
	SettingNum[TDC_GAIN] = model.tdcGainSetting();
	SettingNum[TDC_OFFSET] = model.tdcOffsetSetting();
	SettingNum[TDC_SETUP] = model.tdcSetupSetting();
	SettingNum[CFD_ENERGY_0] = model.cfd0Setting();
	SettingNum[CFD_ENERGY_1] = model.cfd1Setting();
	SettingNum[PEAK_STDEV] = model.stdevSetting();
	SettingNum[PHOTOPEAK] = model.peakSetting();
	SettingNum[SETUP_TEMP] = model.BucketTempSetting();

	// output order of settings
	if (SettingNum[PMTA] != -1) OutOrder[NextOut++] = SettingNum[PMTA];
	if (SettingNum[PMTB] != -1) OutOrder[NextOut++] = SettingNum[PMTB];
	if (SettingNum[PMTC] != -1) OutOrder[NextOut++] = SettingNum[PMTC];
	if (SettingNum[PMTD] != -1) OutOrder[NextOut++] = SettingNum[PMTD];
	if (SettingNum[CFD] != -1) OutOrder[NextOut++] = SettingNum[CFD];
	if (SettingNum[CFD_DELAY] != -1) OutOrder[NextOut++] = SettingNum[CFD_DELAY];
	if ((m_ScannerType == SCANNER_RING) && (SettingNum[CFD_SETUP] != -1)) OutOrder[NextOut++] = SettingNum[CFD_SETUP];
	if (SettingNum[CFD_ENERGY_0] != -1) OutOrder[NextOut++] = SettingNum[CFD_ENERGY_0];
	if (SettingNum[CFD_ENERGY_1] != -1) OutOrder[NextOut++] = SettingNum[CFD_ENERGY_1];
	if (SettingNum[X_OFFSET] != -1) OutOrder[NextOut++] = SettingNum[X_OFFSET];
	if (SettingNum[Y_OFFSET] != -1) OutOrder[NextOut++] = SettingNum[Y_OFFSET];
	if (SettingNum[E_OFFSET] != -1) OutOrder[NextOut++] = SettingNum[E_OFFSET];
	if (SettingNum[SHAPE] != -1) OutOrder[NextOut++] = SettingNum[SHAPE];
	if (SettingNum[DHI_MODE] != -1) OutOrder[NextOut++] = SettingNum[DHI_MODE];
	if ((m_ScannerType != SCANNER_RING) && (SettingNum[CFD_SETUP] != -1)) OutOrder[NextOut++] = SettingNum[CFD_SETUP];
	if (SettingNum[TDC_GAIN] != -1) OutOrder[NextOut++] = SettingNum[TDC_GAIN];
	if (SettingNum[TDC_OFFSET] != -1) OutOrder[NextOut++] = SettingNum[TDC_OFFSET];
	if (SettingNum[TDC_SETUP] != -1) OutOrder[NextOut++] = SettingNum[TDC_SETUP];
	if (SettingNum[SETUP_ENG] != -1) OutOrder[NextOut++] = SettingNum[SETUP_ENG];
	if (SettingNum[SLOW_LOW] != -1) OutOrder[NextOut++] = SettingNum[SLOW_LOW];
	if (SettingNum[SLOW_HIGH] != -1) OutOrder[NextOut++] = SettingNum[SLOW_HIGH];
	if (SettingNum[FAST_LOW] != -1) OutOrder[NextOut++] = SettingNum[FAST_LOW];
	if (SettingNum[FAST_HIGH] != -1) OutOrder[NextOut++] = SettingNum[FAST_HIGH];
	if (SettingNum[PEAK_STDEV] != -1) OutOrder[NextOut++] = SettingNum[PEAK_STDEV];
	if (SettingNum[PHOTOPEAK] != -1) OutOrder[NextOut++] = SettingNum[PHOTOPEAK];
	if (SettingNum[SETUP_TEMP] != -1) OutOrder[NextOut++] = SettingNum[SETUP_TEMP];

	// Attempt to read existing settings data from file
	for (Bucket = 0 ; Bucket < m_NumHeads ; Bucket++) {

		// build file name
		sprintf(filename, "%s\\Current\\Config_0\\Head_%d\\h%d_settings.txt", m_RootPath, Bucket, Bucket);

		// make sure file is there
		if (_stat(filename, &file_status) == -1) continue;

		// open the file
		if ((fp = fopen(filename, "rt")) == NULL) continue;

		// read the title string
		if (fgets(TitleStr, 120, fp) == NULL) {
			fclose(fp);
			continue;
		}

		// loop until end of file
		Block = 0;
		m_Trust[Bucket] = 1;
		while ((1 == m_Trust[Bucket]) && (feof(fp) == 0) && (Block < (m_NumBlocks-1))) {

			// read in block number
			if (fscanf(fp, "%d", &Block) != 1) m_Trust[Bucket] = 0;

			// read in the settings
			for (i = 0 ; (i < m_NumSettings) && (1 == m_Trust[Bucket]) ; i++) {
				if (fscanf(fp, "%d", &m_Settings[(((Bucket * m_NumBlocks) + Block) * m_NumSettings) + OutOrder[i]]) != 1) 
					m_Trust[Bucket] = 0;
			}
		}

		// close the file
		fclose(fp);
	}

	// simulation port name
	if (m_Port == PORT_SIMULATION) {
		sprintf(m_CP_Name, "Simulation");
	}

	// show which coincidenc processor is being attached to
	sprintf(Str, "Communications: %s", m_CP_Name);
	Log_Message(Str);

	// act on coincidence processor type
	m_SerialAsyncFlag = 0;
	switch (m_Port) {

	// Ethernet - Initialize COM Servers on Coincidence Processor
	case PORT_ETHERNET:
		Status = Init_Servers();
		break;

	// Serial port - Initialize it
	case PORT_SERIAL:
		Status = InitSerial();
		break;

	// PetCTGCIdll
	case PORT_GCI:
#ifdef ECAT_SCANNER

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
		if (FAILED(hr)) Status = Add_Error(DAL, Subroutine, "Failed CoInitializeSecurity, HRESULT", hr);
#endif
		break;

	// Simulation - set up simulated data
	case PORT_SIMULATION:
	default:

		// set simulated startup state
		m_Simulated_CP_Mode = -1;
		m_Simulated_Data_Flow = 0;
		m_Simulated_High_Voltage = 1;
		m_Simulated_Coincidence_Window = 6;
		for (i = 0 ; i <= MAX_HEADS ; i++) {
			m_SimulatedStart[i] = 0;
			m_SimulatedFinish[i] = 0;
			m_SimulatedSource[i] = 0;
			m_SimulatedMode[i] = -1;
		}
		m_SimulatedStart[MAX_HEADS] = 0;
		m_SimulatedFinish[MAX_HEADS] = 0;

#ifndef ECAT_SCANNER
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

		// informational message that DAL logging was started
		} else {
			Add_Information(DAL, Subroutine, "Error Logging Started - Simulation");
		}
#endif
		break;
	}

	// return status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Init_Servers
// Purpose:		For ethernet based communications, attach to DCOM servers
// Arguments:	None
// Returns:		long Status
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::Init_Servers()
{
	// name of subroutine and status needed even if main body of code not compiled
	char Subroutine[80] = "Init_Servers";
	long Status = 0;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return 1;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));
	si.pwszName = m_Coincidence_Processor_Name;

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid = IID_Icpmain;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	MULTI_QI mqi2[1];
	IID os_iid = IID_Iosmain;
	mqi2[0].pIID = &os_iid;
	mqi2[0].pItf = NULL;
	mqi2[0].hr = 0;

	// set security level
	if (m_SecuritySet == 0) {
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
		} else {
			m_SecuritySet = 1;
			Log_Message("Succeeded Setting Security Level");
		}
	}

	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport(false, false);

	// if error support not establisted, note it in log
	if (pErrEvtSup == NULL) {
		Log_Message("Failed to Create ErrorEventSupport");

	// if it was not fully istantiated, note it in log and release it
	} else if (pErrEvtSup->InitComplete(hr) == false) {
		sprintf(Str, "ErrorEventSupport Failed During Initialization. HRESULT = %X", hr);
		Log_Message(Str);
		delete pErrEvtSup;
		pErrEvtSup = NULL;

	// informational message that DAL logging was started
	} else {
		Add_Information(DAL, Subroutine, "Error Logging Started - Ethernet");
	}

	// if successful, get the OS interface.
	hr = E_FAIL;
	hr = ::CoCreateInstanceEx(CLSID_osmain, NULL, CLSCTX_REMOTE_SERVER, &si, 1, mqi2);
	if (FAILED(hr)) {
		Add_Error(DAL, Subroutine, "Failed ::CoCreateInstanceEx(CLSID_osmain, NULL, CLSCTX_REMOTE_SERVER, &si, 1, mqi2): HRESULT",hr);
	} else {
		m_pOSMain = (Iosmain *)mqi2[0].pItf;
		Add_Information(DAL, Subroutine, "Succeeded Attaching to OSMain");
	}

	// if successful, get the CP interface.
	if (SUCCEEDED(hr)) {
		hr = E_FAIL;
		hr = ::CoCreateInstanceEx(CLSID_cpmain, NULL, CLSCTX_REMOTE_SERVER, &si, 1, mqi);
		if (FAILED(hr)) {
			Add_Error(DAL, Subroutine, "Failed ::CoCreateInstanceEx(CLSID_cpmain, NULL, CLSCTX_REMOTE_SERVER, &si, 1, mqi): HRESULT",hr);
		} else {
			m_pCPMain = (Icpmain *)mqi[0].pItf;
			Add_Information(DAL, Subroutine, "Succeeded Attaching to CPMain");
		}
	}

	// if successful, get the DHI interface
	if (SUCCEEDED(hr)) {
		hr = E_FAIL;
		hr = m_pCPMain->QueryInterface(IID_Idhimain, (void**) &m_pDHIMain);
		if (FAILED(hr)) {
			Add_Error(DAL, Subroutine, "Failed m_pCPMain->QueryInterface(IID_Idhimain, (void**) &m_pDHIMain): HRESULT",hr);
		} else {
			Add_Information(DAL, Subroutine, "Succeeded Attaching to DHIMain");
		}
	}

	// if any of them failed, then set bad status
	if (FAILED (hr)) {
		Status = 1;
	} else {
		Add_Information(DAL, Subroutine, "Succeeded Attaching To Coincidence Processor");
	}

#endif

	// return status value
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		InitSerial
// Purpose:		For serial line based communication, set up the serial port
// Arguments:	None
// Returns:		long Status
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::InitSerial()
{
	// name of subroutine and status needed even if main body of code not compiled
	char Subroutine[80] = "Init_Serial";
	long Status = 0;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) {
		Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);
		return 1;
	}

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// serial communications control structures
	HANDLE hComPort = NULL;
	DCB dcbCommPort;
	COMMTIMEOUTS ctmoCommPort;

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

	// informational message that DAL logging was started
	} else {
		Add_Information(DAL, Subroutine, "Error Logging Started - Serial Line");
	}

	// open Serial Port - we will be closing it again, this just to make sure we can
	hComPort = CreateFile ((LPCTSTR)m_Coincidence_Processor_Name, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
							OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	// check for error opening serial port
	if (hComPort == INVALID_HANDLE_VALUE) {
		sprintf(Str, "Invalid Serial Port Handle: %s, NULL", m_CP_Name);
		Status = Add_Error(DAL, Subroutine, Str, 0);
	}

	// serial port opened
	if (Status == 0) {

		// control parameters
		memset (&ctmoCommPort, 0, sizeof (ctmoCommPort));
		ctmoCommPort.ReadIntervalTimeout = 0;
		ctmoCommPort.ReadTotalTimeoutMultiplier = 0;
		ctmoCommPort.ReadTotalTimeoutConstant = 500;   // 500 ms timeout
		SetCommTimeouts (hComPort, &ctmoCommPort);

		// communication parameters
		dcbCommPort.DCBlength = sizeof (DCB);
		GetCommState (hComPort, &dcbCommPort);
		dcbCommPort.BaudRate = CBR_115200;
		dcbCommPort.ByteSize = 8;
		dcbCommPort.StopBits = TWOSTOPBITS;
		dcbCommPort.Parity = NOPARITY;
		SetCommState (hComPort, &dcbCommPort);

		// informational message
		Add_Information(DAL, Subroutine, "Serial Port Opened");

		// close the serial port again - thread will re-open
		CloseHandle(hComPort);

		// start the serial port thread
		if (-1 == _beginthread(SerialThread, 0, NULL)) {
			Status = Add_Error(DAL, "InitSerial", "Could Not Start Serial Thread, Handle", -1);
		}
	}

#endif

	// successful
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		SerialResponse
// Purpose:		Retrieve a response from the serial line
// Arguments:	*buffer		- char = serial command response
//				timeout		- int = length of time to wait before timing out on response
// Returns:		long Status
// History:		15 Sep 03	T Gremillion	History Started
//				14 Apr 04	T Gremillion	Fix to re-issue failed commands
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::SerialResponse(char *buffer, int timeout)
{
	// name of subroutine and status needed even if main body of code not compiled
	char Subroutine[80] = "SerialResponse";

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) return Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	char Str[MAX_STR_LEN];

	// define Delay
	long finish_time = time(0) + timeout;
	long curr_time;

	// wait until the buffer is filled by the thread
	while ((m_SerialReadSyncFlag == 0) && ((curr_time = time(0)) < finish_time)) Sleep(5);

	// if it finished in time
	if (time(0) < finish_time) {

		// copy thread buffer to current buffer
		strcpy(buffer, m_LastSerialMsg);

		// clear the flag for the next message
		m_SerialReadSyncFlag = 0;

		// put the response in the log
		sprintf(Str, "Response: %s", buffer);
		Log_Message(Str);

		// success
		return 0;

	// otherwise, signal a timeout
	} else {
		return Add_Error(DAL, Subroutine, "Serial Timeout, Seconds Waited", timeout);
	}
#endif
	return Add_Error(DAL, Subroutine, "Ring Scanner Code Used for Panel Detector Scanner", 0);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		SerialCommand
// Purpose:		Issue a serial line command
// Arguments:	*buffer		- char = serial command
// Returns:		long Status
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::SerialCommand(char *buffer)
{
	// name of subroutine and status needed even if main body of code not compiled
	char Subroutine[80] = "SerialCommand";

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) return Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	char message[MAX_STR_LEN];

	// write command to log
	sprintf(message, "Command %s", buffer);
	Log_Message(message);

	// move the command to the thread buffer
	strcpy(m_SerialCommand, buffer);

	// set the flag that the buffer is to be written
	m_SerialWriteFlag = 1;

	// wait until it has been written
	while (m_SerialWriteFlag != 0) Sleep(5);

#endif

	// successful
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Internal_Pass
// Purpose:		Internal subroutine for sending low-level commands to the DHIs
// Arguments:	Head			-	long = Detector Head 
//				*CommandString	-	unsigned char = firmware command to execute on head
//				*ResponseString	-	unsigned char = firmware response
// Returns:		long Status
// History:		15 Sep 03	T Gremillion	History Started
//				22 Nov 04	T Gremillion	Change command timeout from 10 to 20 seconds
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::Internal_Pass(long Head, long Retry, char *CommandString, char *ResponseString)
{
	// name of subroutine and status needed even if main body of code not compiled
	char Subroutine[80] = "Internal_Pass";
	long ReturnStatus = 0;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) return Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];
	char message[MAX_STR_LEN];
	char CompareCommand[MAX_STR_LEN];
	char CompareResponse[MAX_STR_LEN];
	char error_message[MAX_STR_LEN];
	char *ptr = NULL;
	unsigned char ComCommand[MAX_STR_LEN];

	long ResponseHead = -1;
	long send_command = 1;
	long status = 0;
	long index = 0;
	long ErrorStatus = 0;
	long code = 0;
	long pass = 0;
	long len = 0;
	HRESULT hr = S_OK;

	// verify head number against full range
	if (((Head < 0) || (Head >= m_NumHeads)) && (Head != 64)) return Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);

	// verify head number as present
	if (Head != 64) if (m_HeadPresent[Head] == 0) return Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);

	// pass command does not work to coincidence processor when using DCOM server
	if ((Head == 64) && (m_Port == PORT_ETHERNET)) return Add_Error(DAL, Subroutine, "Ethernet Method Not Available To Coincidence Processor For This Model, Model", m_ModelNumber);

	// initialize response to empty
	sprintf((char *) ResponseString, "");

	// if the ethernet was being used, send to head through the DCOM interface
	if (m_Port == PORT_ETHERNET) {

		// append a new line character
		sprintf((char *) ComCommand, "%s\n", CommandString);

		// loop for resending command if necessary
		for ( ; (send_command == 1) && (pass < 5) && (ReturnStatus == 0); pass++) {

			// add command to log file
			sprintf(message, "m_pDHIMain->Send_DHI Command: %s to Head %d", CommandString, Head);
			Log_Message(message);

			// send command and check for coding/communication problem
			hr = m_pDHIMain->Send_DHI(Head, ComCommand, (unsigned char *) ResponseString, &ErrorStatus);
			if (FAILED(hr)) ReturnStatus = Add_Error(DAL, Subroutine, "Method Failed (m_pDHIMain->Send_DHI), HRESULT", hr);

			// check for error
			if ((ReturnStatus == 0) && (ErrorStatus != 0)) ReturnStatus = CP_Error_Query(ErrorStatus);

			// got a response
			if (ReturnStatus == 0) {

				// remove the newline character
				len = strlen(ResponseString);
				if (len > 0) ResponseString[len-1] = 0;

				// add to file
				sprintf(message, "m_pDHIMain->Send_DHI Response: %s", ResponseString);
				Log_Message(message);
			}

			// break out error code from DHI if present
			index = (Head < 10) ? 3 : 4;
			if ((ReturnStatus == 0) && (ResponseString[index] == '-')) {

				// break out the code
				ptr = &ResponseString[index];
				code = strtol(ptr, &ptr, 0);

				// if it is an xy offset or cfd_delay out of range put out a special code
				if ((E_XY_OFFSET_OOR == code) || (E_DELAY_OOR == code)) {
					sprintf(error_message, "Blocks %s Failed, %s Code", &ResponseString[index+5], error_description(code));
					ReturnStatus = Add_Error(DHI, Subroutine, error_message, code);

				// otherwise, the standard message
				} else {

					// put out the standard message
					sprintf(error_message, "%s, Code", error_description(code));
					ReturnStatus = Add_Error(DHI, Subroutine, error_message, code);
				}
			}

			// check response to make sure response is correct
			if (ReturnStatus == 0) {

				// build a comparison string
				sprintf(CompareCommand, "%d%c", Head, CommandString[0]);
				sprintf(CompareResponse, "%c%c", ResponseString[0], ResponseString[1]);

				// convert command string to uppercase if necessary (lower case "a" is 97)
				if (CompareCommand[1] >= 97) CompareCommand[1] -= 32;

				// if response does not match command, log message
				if (strcmp(CompareCommand, CompareResponse) != 0) {
					sprintf(message, "Response of %s does not match command of %s", CompareResponse, CompareCommand);
					Add_Error(DAL, Subroutine, message, 0);
				}

				// no need to loop further
				if ((strcmp(CompareCommand, CompareResponse) == 0) || (Retry == 0)) send_command = 0;

			// print error message
			} else {
				sprintf(message, "Send_DHI Error: %d", ReturnStatus);
				Log_Message(message);
			}
		}

	// otherwise, serial line is begin used
	} else {

		// copy command to local variable inserting head number
		sprintf(command, "%d%s", Head, CommandString);

		// send command
		status = SerialCommand(command);
		
		// define Delay
		int timeout = 180;
		long finish_time = time(0) + timeout;

		// get response from head
		while ((ResponseHead != Head) && (status == 0) && (ReturnStatus == 0) && (time(0) < finish_time) && (pass < 5)) {

			// get response
			status = SerialResponse(response, 20);

			// successful response
			if (status == 0) {

				// break out the response head
				ResponseHead = strtol(response, &ptr, 0);

				// error message returned as response
				index = (Head < 10) ? 3 : 4;
				if ((ReturnStatus == 0) && (response[index] == '-')) {

					// break out error code
					ptr = &response[index];
					code = strtol(ptr, &ptr, 0);

					// if there was a serial bus conflict and retries are allowed, then retry
					if ((Retry == RETRY) && (code == E_SERIALBUS)) {

						// wait a second for the command to clear
						Sleep(1000);

						// set incorrect response head so it won't drop out of loop
						ResponseHead = -1;

						// put message in log
						Add_Warning(DHI, Subroutine, "RS485 serial bus conflict: Retrying Last Command, Code", code);

						// reissue command
						status = SerialCommand(command);

						// restart timeout counter
						finish_time = time(0) + timeout;

						// keep track of number of passes
						pass++;

					// otherwise, add error message
					} else {

						// if it is an xy offset or cfd_delay out of range put out a special code
						if ((E_XY_OFFSET_OOR == code) || (E_DELAY_OOR == code)) {
							sprintf(error_message, "Blocks %s Failed, %s Code", &response[index+5], error_description(code));
							ReturnStatus = Add_Error(DHI, Subroutine, error_message, code);

						// otherwise, the standard error message
						} else {

							// regular error message
							sprintf(error_message, "%s, Code", error_description(code));
							ReturnStatus = Add_Error(DHI, Subroutine, error_message, code);
						}
					}
				}

			// failed response, re-issue if pass count not exceeded
			} else {
				if (pass++ < 5) status = SerialCommand(command);
			}

			// if it is not an async message and is not the correct response head, then re-issue command
			if ((strcspn(response, "!") == strlen(response)) && (ResponseHead != Head) && (ResponseHead != -1) && (ReturnStatus == 0)) {
				status = SerialCommand(command);
				pass++;
			}
		}

		// timeout occurred if response head is not command head
		if ((ReturnStatus == 0) && (ResponseHead != Head)) {

			// build error message
			sprintf(error_message, "Command Timeout: %s, NULL", command);
			ReturnStatus = Add_Error(DAL, Subroutine, error_message, 0);
		}
			
		// acceptable response, return it
		if (ReturnStatus == 0) sprintf((char *) ResponseString, response);
	}

#endif

	return ReturnStatus;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Add_Error
// Purpose:		send an error message to the event handler
//				add it to the log file and store it for recall
// Arguments:	System		- long = part of system where error originated
//					0 = this component
//					1 = coincidence processor
//					2 = detector head
//				*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
//				*Why		- pertinent integer value
// Returns:		long specific error code
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::Add_Error(long System, char *Where, char *What, long Why)
{
	// local variable
	long error_out = 0;
	long SaveState = 0;
	long RawDataSize = 0;
	long SubSystem = DAL;

	unsigned char GroupName[10] = "ARS_DAL";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// validate system (generic to dal if not identified)
	if ((System == DAL) || (System == CP) || (System == DHI)) SubSystem = System;

	// if it is the DHI, then store the error code
	m_ErrorCode[m_NextError] = 0;
	if (SubSystem == DHI) m_ErrorCode[m_NextError] = Why;

	// save the logging state
	SaveState = m_LogState;

	// add error to error log (after insuring logging is turned on)
	m_LogState = 1;
	Log_Message((char *) Message);

#ifndef ECAT_SCANNER
	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, ErrCode[SubSystem], Message, RawDataSize, EmptyStr, TRUE);
		if (FAILED(hr)) {
			sprintf((char *) Message, "Error Firing Event to Event Handler.  HRESULT = %X", hr);
			Log_Message((char *) Message);
		}
	}
#endif

	// restore logging state
	m_LogState = SaveState;

	// put timeout statement in error array
	sprintf(m_ErrorArray[m_NextError], "%s", Message);

	// transfer error number and increment
	error_out = m_NextError++;

	// check for roll over (0 is reserved for no error)
	if (m_NextError == MAX_ERRORS) m_NextError = 1;

	// return error code
	return error_out;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Add_Warning
// Purpose:		send an warning message to the event handler
//				and add it to the log file
// Arguments:	System		- long = part of system where error originated
//					0 = this component
//					1 = coincidence processor
//					2 = detector head
//				*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
//				*Why		- pertinent integer value
// Returns:		void
// History:		15 Sep 03	T Gremillion	History Started
//////////////////////////////////////////////////////////////////////////////

void CAbstract_DHI::Add_Warning(long System, char *Where, char *What, long Why)
{
	// local variable
	long SaveState = 0;
	long RawDataSize = 0;
	long SubSystem = DAL;

	unsigned char GroupName[10] = "ARS_DAL";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// validate system (generic to dal if not identified)
	if ((System == DAL) || (System == CP) || (System == DHI)) SubSystem = System;

	// save the logging state
	SaveState = m_LogState;

	// add error to error log (after insuring logging is turned on)
	m_LogState = 1;
	Log_Message((char *) Message);

#ifndef ECAT_SCANNER
	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, WarnCode[SubSystem], Message, RawDataSize, EmptyStr, TRUE);
		if (FAILED(hr)) {
			sprintf((char *) Message, "Error Firing Event to Event Handler.  HRESULT = %X", hr);
			Log_Message((char *) Message);
		}
	}
#endif

	// restore logging state
	m_LogState = SaveState;

	// return error code
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Add_Information
// Purpose:		send an informational message to the event handler
//				and add it to the log
// Arguments:	System		- long = part of system where error originated
//					0 = this component
//					1 = coincidence processor
//					2 = detector head
//				*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
// Returns:		void
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CAbstract_DHI::Add_Information(long System, char *Where, char *What)
{
	// local variable
	long SaveState = 0;
	long RawDataSize = 0;
	long SubSystem = DAL;

	unsigned char GroupName[10] = "ARS_DAL";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	// build the message
	sprintf((char *) Message, "%s: %s", Where, What);

	// validate system (generic to dal if not identified)
	if ((System == DAL) || (System == CP) || (System == DHI)) SubSystem = System;

	// save the logging state
	SaveState = m_LogState;

	// add error to error log (after insuring logging is turned on)
	m_LogState = 1;
	Log_Message((char *) Message);

#ifndef ECAT_SCANNER

	HRESULT hr = S_OK;

	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, InfoCode[SubSystem], Message, RawDataSize, EmptyStr, TRUE);
		if (FAILED(hr)) {
			sprintf((char *) Message, "Error Firing Event to Event Handler.  HRESULT = %X", hr);
			Log_Message((char *) Message);
		}
	}
#endif

	// restore logging state
	m_LogState = SaveState;

	// done
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		CP_Error_Query
// Purpose:		decipher an error code from the coincidence processor com server
//				and add it to the error list
// Arguments:	ErrorStatus		- long = coincidence processor error code
// Returns:		long specific error code
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::CP_Error_Query(long ErrorStatus)
{
	// name of subroutine and status needed even if main body of code not compiled
	char Subroutine[80] = "CP_Error_Query";
	long Status = 0;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) return Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Lookup_Status = 0;
	unsigned char message[MAX_STR_LEN];
	HRESULT hr = S_OK;

	// Add to log file
	sprintf((char *) message, "m_pCPMain->Error_Lookup: %d", &ErrorStatus);
	Log_Message((char *) message);

	// query DCOM Sever on meaning of Error
	hr = m_pCPMain->CP_Error_Lookup(ErrorStatus, message, &Lookup_Status);
	if (FAILED(hr)) Status = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->CP_Error_Lookup), HRESULT", hr);

	// if code worked correctly
	if (Status == 0) {

		// if there was a problem with the lookup
		if (Lookup_Status != 0) {

			// Add to log file
			sprintf((char *) message, "m_pCPMain->Error_Lookup: Lookup Problem %d", Lookup_Status);
			Log_Message((char *) message);

			// look up the lookup problem
			hr = m_pCPMain->CP_Error_Lookup(Lookup_Status, message, &Status);
			if (FAILED(hr)) {
				Status = Add_Error(DAL, Subroutine, "Method Failed (m_pCPMain->CP_Error_Lookup), HRESULT", hr);

			// unidentified problem identifying other problem
			} else if (Status != 0) {
				Status = Add_Error(CP, Subroutine, "Could Not Look Up Error In Coincidence Processor DCOM SERVER, Code", Lookup_Status);

			// coincidence processor identified problem
			} else {
				Status = Add_Error(CP, Subroutine, (char *) message, Lookup_Status);
			}

		// otherwise, the lookup worked, so store the message
		} else {
			Status = Add_Error(CP, Subroutine, (char *) message, ErrorStatus);
		}
	}

#endif

	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	OS_Error_Query
// Purpose:	decipher an error code from the coincidence processor generic 
//			com server and add it to the error list
// Arguments:	ErrorStatus		- long = coincidence processor error code
// Returns:		long specific error code
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::OS_Error_Query(long ErrorStatus)
{
	// name of subroutine and status needed even if main body of code not compiled
	char Subroutine[80] = "OS_Error_Query";
	long Status = 0;

	// not yet available for ring scanners
	if (SCANNER_RING == m_ScannerType) return Add_Error(DAL, Subroutine, "Not Defined For Ring Scanners", 0);

// block code unused for this build
#ifndef ECAT_SCANNER

	// local variables
	long Lookup_Status = 0;
	unsigned char message[MAX_STR_LEN];
	char LogMessage[MAX_STR_LEN];
	HRESULT hr = S_OK;

	// Add to log file
	sprintf(LogMessage, "m_pOSMain->OS_Error_Lookup: %d", ErrorStatus);
	Log_Message(LogMessage);

	// query DCOM Sever on meaning of Error
	hr = m_pOSMain->OS_Error_Lookup(ErrorStatus, message, &Lookup_Status);
	if (FAILED(hr)) Status = Add_Error(DAL, Subroutine, "Method Failed (m_pOSMain->OS_Error_Lookup), HRESULT", hr);

	// if code worked correctly
	if (Status == 0) {

		// if there was a problem with the lookup
		if (Lookup_Status != 0) {

			// Add to log file
			sprintf(LogMessage, "m_pOSMain->OS_Error_Lookup - Bad Lookup: %d", Lookup_Status);
			Log_Message(LogMessage);

			// look up the lookup problem
			hr = m_pOSMain->OS_Error_Lookup(Lookup_Status, message, &Status);
			if (FAILED(hr)) {
				Status = Add_Error(DAL, Subroutine, "Method Failed (m_pOSMain->OS_Error_Lookup), HRESULT", hr);

			// unidentified problem identifying other problem
			} else if (Status != 0) {
				Status = Add_Error(CP, Subroutine, "Could Not Look Up Error In Coincidence Processor DCOM SERVER, Code", Lookup_Status);
			} else {
				Status = Add_Error(CP, Subroutine, (char *) message, Lookup_Status);
			}

		// otherwise, the lookup worked, so store the message
		} else {
			Status = Add_Error(CP, Subroutine, (char *) message, ErrorStatus);
		}
	}

#endif

	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	hextobin
// Purpose:	convert hexadecimal string used for file upload to byte equivalent
// Arguments:	hex			- char = string of hexadecimal characters (in ascii)
//				data		- unsigned char = binary equivalint
//				maxlength	- maximum number of bytes
// Returns:		long number of bytes
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::hextobin(char *hex, unsigned char *data, long maxlength)
{
   long i = 0;
   long index = 0;
   long upper = 0;
   long lower = 0;
   long length = 0;
   char digit[MAX_STR_LEN];
   char *hexvals = "0123456789abcdefABCDEF";

   // determine number of bytes
   length = strlen(hex) / 2;

   // if too many bytes, then don't convert
   if (length > maxlength) return 0;

   // convert each hexadecimal pair to byte integer
   for (i = 0; i < length; i++) {
	   index = 2 * i;
	   strncpy(digit, &hex[index], 1);
	   digit[1] = 0;
	   upper = strcspn(hexvals, digit);
	   if (upper > 15) upper -= 6;
	   strncpy(digit, &hex[index+1], 1);
	   digit[1] = 0;
	   lower = strcspn(hexvals, digit);
	   if (lower > 15) lower -= 6;
	   data[i] = (unsigned char) ((upper << 4) | lower);
   }

   return length;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	bintohex
// Purpose:	convert byte data to hexadecimal string for file download
// Arguments:	data		- unsigned char = binary binary data
//				hex			- char = string of hexadecimal characters (in ascii)
//				count		- number of bytes to convert
// Returns:		unsigned long checksum
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

unsigned long CAbstract_DHI::bintohex(unsigned char *data, char *hex, long count)
{
   unsigned char *rawptr = data;
   unsigned int sumval;
   static char tmp[128];

   // initialize
   *hex = '\0';
   unsigned long csum = 0;

   // loop through the number of bytes passed in
   for (long i = 0; i < count; i++)
   {
	   // strip out the current byte and add it to the checksum
	   sumval = ((unsigned int)*rawptr) & 0xff;
	   csum += sumval;

	   // convert to hex
	   sprintf(tmp,"%X",sumval);
      if (strlen(tmp) < 2)
         strcat(hex,"0");
      strcat(hex,tmp);

      rawptr++;
   }

   return csum;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Log_Message
// Purpose:	add message to log file
// Arguments:	message		- char = string to add to log file
// Returns:		void
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Log_Message(char *message)
{
	// local variables
	char time_string[MAX_STR_LEN];
	time_t ltime;

	// only print it if logging is turned on
	if (m_LogState == 1) {

		// get current time and remove new line character
		time(&ltime);
		sprintf(time_string, "%s", ctime(&ltime));
		time_string[24] = '\0';

		// print to log file and then flush the buffer
		if (m_LogFile != NULL) {
			fprintf(m_LogFile, "%s %s\n", time_string, message);
			fflush(m_LogFile);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Release_Servers
// Purpose:		release coincidence processor DCOM servers
// Arguments:	none
// Returns:		void
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CAbstract_DHI::Release_Servers()
{
#ifndef ECAT_SCANNER

	m_pOSMain->Release();
	m_pCPMain->Release();
	m_pDHIMain->Release();

#endif
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Head_Status
// Purpose:		return the head data mode for all DHIs
// Arguments:	*pNumHeads		- long = number of heads
//				**Info			- array of structures with individual head status
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Head_Status(long *pNumHeads, HeadInfo **Info, long *pStatus)
{
	// local variables
	long Head = 0;
	long Mode = 0;
	long Config = 0;
	long Layer = 0;
	long LLD = 0;
	long ULD = 0;
	long SLD = 0;
	long Next = 0;

	char Subroutine[80] = "Head_Status";

	HeadInfo *Ptr;

	HRESULT hr = S_OK;

	// start off with a good status
	*pStatus = 0;

	// determine number of heads
	for (*pNumHeads = 0, Head = 0 ; Head < MAX_HEADS ; Head++) {
		if (m_HeadPresent[Head] != 0) (*pNumHeads)++;
	}

	// allocate output array of structures
	Ptr = (HeadInfo *) ::CoTaskMemAlloc(*pNumHeads * sizeof(HeadInfo));
	*Info = Ptr;
	if (*Info == NULL) {
		hr = S_FALSE;
		*pStatus = Add_Error(DAL, Subroutine, "Allocating Memory, Requested Size", (*pNumHeads * sizeof(HeadInfo)));
	}

	// Get individual head data
	for (Head = 0 ; (Head < MAX_HEADS) && (*pStatus == 0) ; Head++) {

		// if the head is present
		if (m_HeadPresent[Head] != 0) {

			// get head info
			hr = Check_Head_Mode(Head, &Mode, &Config, &Layer, &LLD, &ULD, pStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Check_Head_Mode), HRESULT", hr);

			// success, copy the data over
			if (*pStatus == 0) {
				Ptr[Next].Address = Head;
				Ptr[Next].Mode = Mode;
				Ptr[Next].Config = Config;
				Ptr[Next].Layer = Layer;
				Ptr[Next].LLD = LLD;
				Ptr[Next].ULD = ULD;
				Ptr[Next].SLD = SLD;
				Next++;
			}
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	CountRate
// Purpose:	return scan statistics
// Arguments:	*pCorrectedSingles		-	long = from all heads
//				*pUncorrectedSingles	-	long = from all heads
//				*pPrompts				-	long = prompt events
//				*pScatters				-	long = scatter events
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::CountRate(long *pCorrectedSingles, long *pUncorrectedSingles, long *pPrompts, long *pRandoms, long *pScatters, long *pStatus)
{
	// local variables
	long Head = 0;

	long Stats[19];

	char Subroutine[80] = "CountRate";

	HRESULT hr = S_OK;

	// Initialize
	*pCorrectedSingles = 0;
	*pUncorrectedSingles = 0;
	*pPrompts = 0;
	*pRandoms = 0;
	*pScatters = 0;
	*pStatus = 0;

	// pass on the call
	hr = Get_Statistics(Stats, pStatus);
	if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Get_Statistics), HRESULT", hr);

	// successful - transfer
	if (*pStatus == 0) {

		// if in coincidence mode (or Timing) provide prompts and randoms
		if ((Stats[2] == CP_MODE_COINC) || (Stats[2] == CP_MODE_TIME)) {
			*pRandoms = Stats[1];
			*pPrompts = Stats[0];
		}

		// uncorrected singles is total singles for all heads
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if (m_HeadPresent[Head] != 0) *pUncorrectedSingles += Stats[Head+3];
		}
	}

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Ring_Singles
// Purpose:		returns the average block singles count for each ring of blocks
// Arguments:	*pRings				-	long = number of rings
//				**RingSingles		-	long = average number of singles events on each ring
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Ring_Singles(long *pNumRings, long **RingSingles, long *pStatus)
{
	// local variables
	long Head = 0;
	long Block = 0;
	long Ring = 0;
	long Count = 0;
	long SinglesType = SINGLES_VALIDS;

	long BlockSingles[MAX_BLOCKS];

	long *Ptr;

	char Subroutine[80] = "Ring_Singles";

	HRESULT hr = S_OK;

	// determine the number of rings
	if (m_Rotated == 1) {
		*pNumRings = m_XBlocks;
	} else {
		*pNumRings = m_YBlocks;
	}

	// allocate space for return array
	Ptr = (long *) ::CoTaskMemAlloc(sizeof(long) * *pNumRings);
	*RingSingles = Ptr;
	if (*RingSingles == NULL) *pStatus = Add_Error(DAL, Subroutine, "Allocating Memory, Requested Size", (sizeof(long) * *pNumRings));

	// clear counts
	for (Ring = 0 ; (Ring < *pNumRings) && (*pStatus == 0) ; Ring++) Ptr[Ring] = 0;

	// loop through the heads
	for (Head = 0 ; (Head < MAX_HEADS) && (*pStatus == 0) ; Head++) {

		// if the head is present
		if (m_HeadPresent[Head] != 0) {

			// get the block singles
			hr = Singles(Head, SinglesType, BlockSingles, pStatus);
			if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Singles), HRESULT", hr);

			// if successfull, loop through blocks
			for (Block = 0 ; (Block < (m_XBlocks * m_YBlocks)) && (*pStatus == 0) ; Block++) {

				// determine which ring
				if (m_Rotated == 1) {
					Ring = Block % m_XBlocks;
				} else {
					Ring = Block / m_XBlocks;
				}

				// add to ring singles
				Ptr[Ring] += BlockSingles[Block];

				// keep track of number of blocks added to first ring
				if (Ring == 0) Count++;
			}
		}
	}

	// no counts for first ring = error
	if (Count == 0) *pStatus = Add_Error(DAL, Subroutine, "Nuber of Blocks Per Ring", Count);

	// average singles for ring
	for (Ring = 0 ; (Ring < *pNumRings) && (*pStatus == 0) ; Ring++) Ptr[Ring] /= Count;

	// bad status return false
	if (*pStatus != 0) hr = S_FALSE;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		KillSelf
// Purpose:		exit server
// Arguments:	none
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::KillSelf()
{
	char Subroutine[80] = "KillSelf";

	// put what is happening in the log
	Add_Information(DAL, Subroutine, "Shut Down Server");

	// cause the server to exit
	exit(0);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		SetLog
// Purpose:		turn on/off logging of incidental messages
// Arguments:	OnOff		- long - logging state flag
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::SetLog(long OnOff, long *pStatus)
{
	char Subroutine[80] = "SetLog";

	// validate input
	if ((OnOff != 0) && (OnOff != 1)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Log State", OnOff);
		return S_FALSE;
	}

	// if turning it on when it was off
	if ((m_LogState == 0) && (OnOff == 1)) {

		// turn it on
		m_LogState = 1;

		// record in log
		Log_Message("Logging Turned On");

	// if turning it off when it was on
	} else if ((m_LogState == 1) && (OnOff == 0)) {

		// record in log
		Log_Message("Logging Turned Off");

		// turn it on
		m_LogState = 0;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Report_Temperature_Voltage
// Purpose:	return the temperatures and voltages for all the DHIs and the
//			coincidence processor
// Arguments:	*pCP_Temperature			- float = coincidence processor temperature
//				*pMinimum_Temperature		- float = low temperature cutoff
//				*pMaximum_Temperature		- float = high temperature cutoff
//				*pNumber_DHI_Temperatures	- long = number of tempertures returned for each board
//				*pNumHeads					- long = number of detector heads
//				**Info						- structure of temperature information
//				*pStatus					- specific error code
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Report_Temperature_Voltage(float *pCP_Temperature, float *pMinimum_Temperature, float *pMaximum_Temperature, 
													   long *pNumber_DHI_Temperatures, long *pNumHeads, PhysicalInfo **Info, long *pStatus)
{
	// local variables
	long i = 0;
	long Head = 0;
	long Next = 0;
	long Code = 0;
	long data_index = 0;

	float LocalTemp = 0.0;
	float RemoteTemp = 0.0;

	float TemperatureArray[35];

	char command[MAX_STR_LEN];
	char response[MAX_STR_LEN];
	char Subroutine[80] = "Report_Temperature_Voltage";

	unsigned char ErrorMsg[MAX_STR_LEN];

	char *ptr = NULL;

	PhysicalInfo *Ptr;

	HRESULT hr = S_OK;

	// start off with a good status
	*pStatus = 0;

	// transfer number of temperatures
	*pNumber_DHI_Temperatures = m_NumDHITemps;

	// determine number of heads
	for (*pNumHeads = 0, Head = 0 ; Head < MAX_HEADS ; Head++) {
		if (m_HeadPresent[Head] != 0) (*pNumHeads)++;
	}

	// allocate output array of structures
	Ptr = (PhysicalInfo *) ::CoTaskMemAlloc(*pNumHeads * sizeof(PhysicalInfo));
	*Info = Ptr;
	if (*Info == NULL) {
		hr = S_FALSE;
		*pStatus = Add_Error(DAL, Subroutine, "Allocating Memory, Requested Size", (*pNumHeads * sizeof(PhysicalInfo)));
	}

	// get temperature information (most of this is redundant, but is the only way we can get the CP information)
	if (*pStatus == 0) {
		hr = Report_Temperature(TemperatureArray, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(DAL, Subroutine, "Method Failed (Report_Temperature), HRESULT", hr);
		if (hr == S_FALSE) return hr;
	}

	// fill in cp numbers
	*pCP_Temperature = TemperatureArray[2];
	*pMinimum_Temperature = TemperatureArray[1];
	*pMaximum_Temperature = TemperatureArray[0];

	// fill in head numbers
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// if the head is present
		if (m_HeadPresent[Head] != 0) {

			// if simulation
			if (m_Port == PORT_SIMULATION) {

				// address of head
				Ptr[Next].Address = Head;

				// fill in voltages
				Ptr[Next].HighVoltage = 0;
				if (m_Simulated_High_Voltage == 1) Ptr[Next].HighVoltage = 1100;
				Ptr[Next].Plus5Volts = 5.0;
				Ptr[Next].Minus5Volts = -5.0;
				Ptr[Next].OtherVoltage = 0.0;

				// light tight interface temperature
				if (m_ScannerType == SCANNER_HRRT) {
					Ptr[Next].LTI_Temperature = 0.0;
				} else {
					Ptr[Next].LTI_Temperature = (float) (Head + 30);
				}

				// dhi temperatures
				for (i = 0 ; i < m_NumDHITemps ; i++) Ptr[Next].DHI_Temperature[i] = (float)(27.5 + ((double) (Head + i)));

				// increment Head
				Next++;

			// otherwise fetch the information from the detector heads
			} else {

				// issue command
				sprintf(command, "A");
				*pStatus = Internal_Pass(Head, RETRY, command, response);

				// if status is a diagnostics error, then re-issue
				if (*pStatus != 0) {

					// get code
					hr = Error_Lookup(*pStatus, &Code, ErrorMsg);

					// diagnostic codes are < -100 and reboot code is -68
					if ((Code < -100) || (Code == -68)) *pStatus = Internal_Pass(Head, RETRY, command, response);
				}

				// if command succeeded, break out value
				if (*pStatus == 0) {

					// address of head
					Ptr[Next].Address = Head;

					// index into output array and string
					data_index = (Head < 10) ? 2 : 3;
					ptr = &response[data_index];

					// voltages
					Ptr[Next].HighVoltage = strtol(ptr, &ptr, 10);
					if (m_ScannerType == SCANNER_HRRT) {
						for (i = 0 ; i < 4 ; i++) Ptr[Next].HighVoltage += strtol(ptr, &ptr, 10);
						Ptr[Next].HighVoltage /= 5;
					}
					Ptr[Next].Plus5Volts = (float) strtod(ptr, &ptr);
					Ptr[Next].OtherVoltage = (float) strtod(ptr, &ptr);
					Ptr[Next].Minus5Volts = (float) strtod(ptr, &ptr);

					// light tight interface temperature
					if (m_ScannerType == SCANNER_HRRT) {
						Ptr[Next].LTI_Temperature = 0.0;
						LocalTemp = (float) strtod(ptr, &ptr);
						RemoteTemp = (float) strtod(ptr, &ptr);
					} else {
						Ptr[Next].LTI_Temperature = (float) strtod(ptr, &ptr);
					}

					// transfer the dhi temperatures
					for (i = 0 ; i < m_NumDHITemps ; i++) Ptr[Next].DHI_Temperature[i] = (float) strtod(ptr, &ptr);

					// next head
					Next++;

				// bad response - return that it failed
				} else {
					return S_FALSE;
				}
			}
		}
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	SerialThread
// Purpose:	Thread that keeps track of all serial line communication
// Arguments:	*dummy			- void
// Returns:		void
// History:		15 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	Prevent garbage from being read from
//											the line by throwing away any data
//											received that is not followed by another
//											character (or carriage return) within 1 second
//				22 Nov 04	T Gremillion	put asynchronous messages in buffer for retrieval
/////////////////////////////////////////////////////////////////////////////

void SerialThread(void *dummy)
{
#ifndef ECAT_SCANNER

	// local variables
	DWORD rc = 0;
	DWORD dwCount = 1;
	DWORD Remaining = 0;
	DWORD Event = 0;

	long MsgLen = 0;
	long AsyncPos = 0;
	long SaveState = 0;

	char c;
	char message[MAX_STR_LEN];
	char Str[MAX_STR_LEN];

	unsigned char GroupName[10] = "ARS_DAL";
	unsigned char EmptyStr[10] = "";
	long SubSystem = DAL;
	long RawDataSize = 0;

	time_t lasttime;
	time_t currtime;

	BOOL  bResult = TRUE;

	COMSTAT comstat;
	static OVERLAPPED ov;

	HRESULT hr = S_OK;

	// get starting time
	time(&lasttime);

	// declare error handler
	CErrorEventSupport *pErrSup = NULL;

	// clear the flag for sychronous reads and writes
	m_SerialReadSyncFlag = 0;
	m_SerialWriteFlag = 0;
	m_SerialAsyncFlag = 0;

	// serial communications control structures
	HANDLE hComPort = NULL;
	DCB dcbCommPort;
	COMMTIMEOUTS ctmoCommPort;

	// instantiate an ErrorEventSupport object
	pErrSup = new CErrorEventSupport(false, true);

	// if error support not establisted, note it in log
	if (pErrSup != NULL) if (pErrSup->InitComplete(hr) == false) {
		delete pErrSup;
		pErrSup = NULL;
	}

	// open Serial Port
	hComPort = CreateFile ((LPCTSTR)m_Coincidence_Processor_Name, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
							OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	// control parameters
	memset (&ctmoCommPort, 0, sizeof (ctmoCommPort));
	ctmoCommPort.ReadIntervalTimeout = 0;
	ctmoCommPort.ReadTotalTimeoutMultiplier = 0;
	ctmoCommPort.ReadTotalTimeoutConstant = 500;   // 500 ms timeout
	SetCommTimeouts (hComPort, &ctmoCommPort);

	// communication parameters
	dcbCommPort.DCBlength = sizeof (DCB);
	GetCommState (hComPort, &dcbCommPort);
	dcbCommPort.BaudRate = CBR_115200;
	dcbCommPort.ByteSize = 8;
	dcbCommPort.StopBits = TWOSTOPBITS;
	dcbCommPort.Parity = NOPARITY;
	SetCommState (hComPort, &dcbCommPort);

	while (true) {

		// initial check to see if there is a serial line event waiting
		bResult = WaitCommEvent(hComPort, &Event, &ov);
		if (!bResult) {

			// clear the event
			bResult = ClearCommError(hComPort, &Event, &comstat);

			// Number of characters waiting
			Remaining = comstat.cbInQue;
		}

		// loop while there are characters remaining in the queue
		while (Remaining > 0) {

			// get current time
			time(&currtime);

			// if it has been more than a second since the last valid character, clear the buffer before transferring
			if ((currtime - lasttime) > 1) rc = 0;

			// save current time for processing of next character
			lasttime = currtime;

			// read the character
			ReadFile (hComPort, &c, 1, &dwCount, NULL);

			// if read was successfull
			if (1 == dwCount) {

				// if it is an end of line
				if ('\n' == c) {
					// terminate string
					message[rc] = 0;

					// length of message and position of asynchronous flag
					MsgLen = strlen(message);
					AsyncPos = strcspn(message, "!");

					// if asynchronous flag found in body of message
					if (AsyncPos != MsgLen) {

						// log the asynchronous message
						sprintf(Str, "Asynchronous Message: %s", message);
						Log_Message(Str);

						// if the asynchronous message is an error, post it to the log (space separated error code according to the spec)
						// other asynchronous message are ignored
						if ('-' == message[AsyncPos+2]) {

							// build error message
							sprintf(Str, "Asynchronous Error: %s", &message[AsyncPos+2]);

#ifndef ECAT_SCANNER
							// Send to Event handler
							if (pErrSup != NULL) hr = pErrSup->Fire(GroupName, ErrCode[SubSystem], (unsigned char *) Str, RawDataSize, EmptyStr, TRUE);
#endif

							// write error message to log preserving log state
							SaveState = m_LogState;
							m_LogState = 1;
							Log_Message(Str);
							m_LogState = SaveState;
						}

						// copy the asynchronous message to the global buffer and set the flag
						strcpy(m_LastAsyncMsg, message);
						m_SerialAsyncFlag = 1;

					// synchronous message
					} else {

						// ignore a "-69" as response and wait for real response
						if ((message[3] == '-') && (message[4] == '6') && (message[5] == '9')) {

							// put out message
							SaveState = m_LogState;
							m_LogState = 1;
							Log_Message("Command Conflict (-69).  Waiting For Real Response");
							m_LogState = SaveState;

						} else {

							// transfer to synchronous message buffer
							strcpy(m_LastSerialMsg, message);

							//  set read buffer flag
							m_SerialReadSyncFlag = 1;
						}
					}

					// reset character count
					rc = 0;

				// reguular characters, add it to the string and increment the character count
				} else {
					message[rc++] = c;
				}
			}

			// see if there is still a serial line event waiting
			bResult = WaitCommEvent(hComPort, &Event, &ov);
			if (!bResult) {

				// clear the event
				bResult = ClearCommError(hComPort, &Event, &comstat);

				// Number of characters waiting
				Remaining = comstat.cbInQue;

			// no event, nothing waiting
			} else {
				Remaining = 0;
			}
		}

		// check if there is something to be written
		if (m_SerialWriteFlag != 0) {

			// send the command data
			dwCount = strlen(m_SerialCommand);
			WriteFile (hComPort, m_SerialCommand, dwCount, &dwCount, NULL);

			// send a terminator
			char c = '\n';
			dwCount = 1;
			WriteFile (hComPort, &c, dwCount, &dwCount, NULL);

			// clear the write flag
			m_SerialWriteFlag = 0;
		}

		// wait 5 milliseconds for the next loop
		Sleep(5);
	}

#endif
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Get_Block_Crystal_Position
// Purpose:	returns the x/y coordinates of crystals for specified block from bucket on ring scanners
// Arguments:	Head			-	long = bucket number
//				Block			-	long = block number
//				*Peaks			-	long = array of peak coordinates
//				*pStatus		-	specific error code
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	Corrected loop index for transfer to output arrays
//				14 Apr 04	T Gremillion	Removed Configuration and Layer from argument list
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Get_Block_Crystal_Position(long Head, long Block, long Peaks[], long *pStatus)
{
	// variables neede regardless of build
	long i = 0;

	char Subroutine[80] = "Get_Block_Crystal_Position";

	HRESULT hr = S_OK;

	// only valid for ring scanners
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify block
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

// build specific to ring scanners
#ifdef ECAT_SCANNER

	// local variables
	long NumCrystals = m_XCrystals * m_YCrystals;
	unsigned char locations[2*MAX_CRYSTALS];
	char row[MAX_CRYSTALS];
	char col[MAX_CRYSTALS];

	// retrieve the peak locations
	if (!upload_boundary_data (Head, Block, row, col, (char *) locations)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::upload_boudary_data, Bucket", Head);
		return S_FALSE;
	}

	// convert to expected format
	for (i = 0 ; i < NumCrystals ; i++) Peaks[(2*i)+0] = (long) locations[i];
	for (i = 0 ; i < NumCrystals ; i++) Peaks[(2*i)+1] = (long) locations[NumCrystals+i];
#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Get_Block_Crystal_Peaks
// Purpose:	returns the crystal energy peaks of crystals for specified block from bucket on ring scanners
// Arguments:	Head			-	long = bucket number
//				Block			-	long = block number
//				*Peaks			-	long = array of energy peaks
//				*pStatus		-	specific error code
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	Corrected loop index for transfer to output arrays
//				14 Apr 04	T Gremillion	Removed Configuration and Layer from argument list
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Get_Block_Crystal_Peaks(long Head, long Block, long Peaks[], long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;

	char Subroutine[80] = "Get_Block_Crystal_Peaks";

	HRESULT hr = S_OK;

	// only valid for ring scanners
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify block
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

// build specific to ring scanners
#ifdef ECAT_SCANNER

	// local variables
	long NumCrystals = m_XCrystals * m_YCrystals;
	unsigned char values[MAX_CRYSTALS];

	// retrieve the peak locations
	if (!get_crystal_energy_peaks(Head, Block, (char *) values)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::get_crystal_energy_peaks, Bucket", Head);
		return S_FALSE;
	}

	// convert to expected format
	for (i = 0 ; i < MAX_CRYSTALS ; i++) Peaks[i] = 0;
	for (j = 0 ; j < m_YCrystals ; j++) for (i = 0 ; i < m_XCrystals ; i++) {
		Peaks[(j*MAX_CRYSTALS_X)+i] = (long) values[(j*m_XCrystals)+i];
	}
#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Get_Block_Time_Correction
// Purpose:	returns the time correction coefficients of crystals for specified block from bucket on ring scanners
// Arguments:	Head			-	long = bucket number
//				Block			-	long = block number
//				*Correction		-	long = array of time correction coefficients
//				*pStatus		-	specific error code
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	Corrected loop index for transfer to output arrays
//				14 Apr 04	T Gremillion	Removed Configuration and Layer from argument list
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Get_Block_Time_Correction(long Head, long Block, long Correction[], long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;

	char Subroutine[80] = "Get_Block_Time_Correction";

	HRESULT hr = S_OK;

	// only valid for ring scanners
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify block
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

// build specific to ring scanners
#ifdef ECAT_SCANNER

	// local variables
	long NumCrystals = m_XCrystals * m_YCrystals;
	unsigned char values[MAX_CRYSTALS];

	// retrieve the peak locations
	if (GCI_SUCCESS != get_crystal_time_offsets(Head, Block, (char *) values)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::get_crystal_time_offsets, Bucket", Head);
		return S_FALSE;
	}

	// convert to expected format
	for (i = 0 ; i < MAX_CRYSTALS ; i++) Correction[i] = 0;
	for (j = 0 ; j < m_YCrystals ; j++) for (i = 0 ; i < m_XCrystals ; i++) {
		Correction[(j*MAX_CRYSTALS_X)+i] = (long) values[(j*m_XCrystals)+i];
	}
#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Compare_Checksum
// Purpose:	returns whether or not the locally computed checksum for a Head matches
//			the checksum generated on the bucket for a specified bucket
// Arguments:	Head			-	long = bucket number
// Returns:		long (0 = false, 1 = true)
// History:		04 Mar 04	T Gremillion	New Routine
//			19 Oct 04	T Gremillion	Handle HRRT checksum
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::Compare_Checksum(long Head)
{
	// local variables
	long index = 0;
	long Block = 0;
	long Setting = 0;
	long bucket_checksum = 0;
	long local_checksum = 0;
	long status = 0;

	char message[MAX_STR_LEN] = "";
	char response[MAX_STR_LEN] = "";
	char Subroutine[80] = "Compare_Checksum";

	// if the head is not present, we have to trust the values
	if (m_HeadPresent[Head] != 1) return 1;

#ifdef ECAT_SCANNER
	// retrieve checksum from bucket
	status = get_asic_checksum(Head, m_NumBlocks, (int*) &bucket_checksum);
	if (status != GCI_SUCCESS) {
		sprintf(message, "Failed to Retrieve Checksum for Bucket %d, Return Code", Head);
		Add_Error(DAL, Subroutine, message, status);
		return 0;
	}
#else

	// send command to get checksum from DHI
	status = Internal_Pass(Head, RETRY, "X 16 0", response);
	sscanf(&response[3], "%x", &bucket_checksum);

#endif

	// compute the local checksum
	for (Block = 0 ; Block < m_NumBlocks ; Block++) {
		for (Setting = 0 ; Setting < m_NumSettings ; Setting++) {
			index = (((Head * m_NumBlocks) + Block) * m_NumSettings) + Setting;
			local_checksum += m_Settings[index];
		}
	}

	// for the HRRT, convert it to a short integer checksum
	if (SCANNER_HRRT == m_ScannerType) local_checksum &= 0x0000FFFF;

	// return comparison result
	if (bucket_checksum == local_checksum) {
		return 1;
	} else {
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Set_Block_Time_Correction
// Purpose:	sends the time correction coefficients of crystals for specified block to bucket on ring scanners
// Arguments:	Head			-	long = bucket number
//				Block			-	long = block number
//				*Correction		-	long = array of time correction coefficients
//				*pStatus		-	specific error code
// Returns:		HRESULT
// History:		30 Mar 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Set_Block_Time_Correction(long Head, long Block, long Correction[], long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;

	char Subroutine[80] = "Set_Block_Time_Correction";

	HRESULT hr = S_OK;

	// only valid for ring scanners
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify block
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

// build specific to ring scanners
#ifdef ECAT_SCANNER

	// local variables
	long NumCrystals = m_XCrystals * m_YCrystals;
	unsigned char values[MAX_CRYSTALS];

	// convert to expected format
	for (j = 0 ; j < m_YCrystals ; j++) for (i = 0 ; i < m_XCrystals ; i++) {
		values[(j*m_XCrystals)+i] = (unsigned char) Correction[(j*MAX_CRYSTALS_X)+i];
	}

	// send time corrections
	if (GCI_SUCCESS != set_crystal_time_offsets(Head, Block, (char *) values)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::set_crystal_time_offsets, Bucket", Head);
		return S_FALSE;
	}
#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Set_Block_Crystal_Peaks
// Purpose:	sends the energy peak value of crystals for specified block to bucket on ring scanners
// Arguments:	Head			-	long = bucket number
//				Block			-	long = block number
//				*Peaks			-	long = array of crystal energy peaks
//				*pStatus		-	specific error code
// Returns:		HRESULT
// History:		27 Apr 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Set_Block_Crystal_Peaks(long Head, long Block, long Peaks[], long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;

	char Subroutine[80] = "Set_Block_Crystal_Peaks";

	HRESULT hr = S_OK;

	// only valid for ring scanners
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify block
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

// build specific to ring scanners
#ifdef ECAT_SCANNER

	// local variables
	long NumCrystals = m_XCrystals * m_YCrystals;
	unsigned char values[MAX_CRYSTALS];

	// convert to expected format
	for (j = 0 ; j < m_YCrystals ; j++) for (i = 0 ; i < m_XCrystals ; i++) {
		values[(j*m_XCrystals)+i] = (unsigned char) Peaks[(j*MAX_CRYSTALS_X)+i];
	}

	// send the peak locations
	if (GCI_SUCCESS != set_crystal_energy_peaks(Head, Block, (char *) values)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::set_crystal_time_offsets, Bucket", Head);
		return S_FALSE;
	}
#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Set_Block_Crystal_Position
// Purpose:	sends the energy peak value of crystals for specified block to bucket on ring scanners
// Arguments:	Head			-	long = bucket number
//				Block			-	long = block number
//				*Peaks			-	long = array of crystal x/y coordinates
//				StdDev			-	long = standard deviation
//				*pStatus		-	specific error code
// Returns:		HRESULT
// History:		27 Apr 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Set_Block_Crystal_Position(long Head, long Block, long Peaks[], long StdDev, long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;

	char Subroutine[80] = "Set_Block_Crystal_Position";

	HRESULT hr = S_OK;

	// only valid for ring scanners
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify block
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

// build specific to ring scanners
#ifdef ECAT_SCANNER

	// local variables
	long NumCrystals = m_XCrystals * m_YCrystals;
	unsigned char x[MAX_CRYSTALS];
	unsigned char y[MAX_CRYSTALS];
	long SetupTemp;
	long index;

	// convert to expected format
	for (i = 0 ; i < (m_XCrystals*m_YCrystals) ; i++) {
		x[i] = (unsigned char) Peaks[(2*i)+0];
		y[i] = (unsigned char) Peaks[(2*i)+1];
	}

	// send the peak locations
	if (GCI_SUCCESS != set_crystal_peak_locations(Head, Block, (char *) x, (char *) y, (short) StdDev, (int *) &SetupTemp)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::set_crystal_time_offsets, Bucket", Head);
		return S_FALSE;
	}

	// if the analog settings are trusted, then update the Standard Deviation and Setup Temperature
	if (1 == m_Trust[Head]) {
		index = ((Head * m_NumBlocks) + Block) * m_NumSettings;
		m_Settings[index + SettingNum[PEAK_STDEV]] = StdDev;
		m_Settings[index + SettingNum[SETUP_TEMP]] = SetupTemp;
		Save_Settings(0, Head);
	}
#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Save_Settings
// Purpose:		save the analog settings for specified configuration/head
// Arguments:	Config		- long = Configuration
//				Head		- long = Head
// Returns:		long		- Status
// History:		20 May 04	T Gremillion	History Started
//				19 Oct 04	T Gremillion	Order in file is not excatly
//											the same as order from scanner.
/////////////////////////////////////////////////////////////////////////////

long CAbstract_DHI::Save_Settings(long Config, long Head)
{
	// local variables
	long i;
	long j;
	long index;
	long Status = 0;

	char Str[MAX_STR_LEN];
	char filename[MAX_STR_LEN];
	char Subroutine[80] = "Save_Settings";

	FILE *fp = NULL;

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\h%d_settings.txt", m_RootPath, Config, Head, Head);

	// open the file
	if ((fp = fopen(filename, "wt")) == NULL) {
		sprintf(Str, "Could Not Open File: %s", filename);
		return Add_Error(DAL, Subroutine, Str, 0);
	}

	// build the title string
	sprintf(Str, " Block");
	for (i = 0 ; i < m_NumSettings ; i++) {
		for (j = 0 ; (j < MAX_SETTINGS) && (SettingNum[j] != OutOrder[i]) ; j++);
		sprintf(Str, "%s %s", Str, SetTitle[j]);
	}

	// write the title string
	fprintf(fp, "%s\n", Str);

	// write the data lines
	for (i = 0 ; i < m_NumBlocks ; i++) {

		// Block Number
		sprintf(Str, "%6d", i);

		// Settings
		for (j = 0 ; j < m_NumSettings ; j++) {
			index = (((((Config * MAX_HEADS) + Head) * m_NumBlocks) + i) * m_NumSettings) + OutOrder[j];
			sprintf(Str, "%s%6d", Str, m_Settings[index]);
		}

		// write the string
		fprintf(fp, "%s\n", Str);
	}

	// close the file
	fclose(fp);

	// return status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Build_CRM
// Purpose:	causes the crystal region map for the specified block to be built
// Arguments:	Head			-	long = bucket number
//				Block			-	long = block number
//				*pStatus		-	specific error code
// Returns:		HRESULT
// History:		09 Jun 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Build_CRM(long Head, long Block, long *pStatus)
{
	// local variables
	char Subroutine[80] = "Set_Block_Crystal_Position";

	// only valid for ring scanners
	if (SCANNER_RING != m_ScannerType) {
		*pStatus = Add_Error(DAL, Subroutine, "Method Not Available For This Model, Model", m_ModelNumber);
		return S_FALSE;
	}

	// verify head number against full range
	if ((Head < 0) || (Head >= m_NumHeads)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	}

	// verify head number as present
	if (m_HeadPresent[Head] == 0) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// verify block
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(DAL, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

// build specific to ring scanners
#ifdef ECAT_SCANNER

	// build the Crystal Region Map (CRM)
	if (GCI_SUCCESS != build_crm(Head, Block)) {
		*pStatus = Add_Error(DAL, Subroutine, "Failed PetCTGCIDll::build_crm, Bucket", Head);
		return S_FALSE;
	}
#endif

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Last_Async_Message
// Purpose:	retrieve the last asynchronous message from the serial line
// Arguments:	LastMsg		[out] char[2048] = Last Asynchronous Message 
// Returns:		HRESULT
//					S_OK = Success
//					S_FALSE = No new message since last request
// History:		11 Nov 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAbstract_DHI::Last_Async_Message(unsigned char LastMsg[])
{
	// If there has been a new message since the last time it was called, then copy it out and clear the flag
	if (m_SerialAsyncFlag) {
		strcpy((char *) LastMsg, m_LastAsyncMsg);
		m_SerialAsyncFlag = 0;
		return S_OK;

	// if there has not be a new message return an S_FALSE;
	} else return S_FALSE;
}
