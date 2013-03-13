//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Detector Utilities COM Server (DUCOM.exe)
// 
// Name:			DUMain.cpp - Implementation of CDUMain
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	July 25, 2002
// 
// Description:		This component is the Detector Utilities COM server which does
//					the file handling for detector setup and detector utilities.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DUCom.h"
#include "DUMain.h"
#include "DHICom.h"
#include "DHICom_i.c"
#include "Acquisition.h"
#include "Acquisition_i.c"
#include "position_profile_tools.h"

// build dependent includes
#ifndef ECAT_SCANNER

#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

// for ring scanners, define a dummy data type for the error handler class
#else

#define CErrorEventSupport void

#endif

#include <time.h>
#include <process.h>
#include <math.h>
#include <io.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gantryModelClass.h"

// global variables
static long m_HeadTestSet;
static long m_DataMode;
static long m_Configuration;
static long m_AcquirePercent;
static long m_AcquireFlag;
static long m_ArchivePercent;
static long m_ArchiveFlag;
static long m_ScannerType;
static long m_ModulePair;
static long m_Transmission;
static long m_NumConfigs;
static long m_NumHeads;
static long m_NumBlocks;
static long m_NumSettings;
static long m_XBlocks;
static long m_YBlocks;
static long m_EmissionBlocks;
static long m_XCrystals;
static long m_YCrystals;
static long m_ArchiveMode;
static long m_Rotate;
static long m_AcqType;
static long m_Amount;
static long m_lld;
static long m_uld;
static long m_Acquiring;
static long m_AcqAbort;
static long m_AcquisitionLimit;
static long m_TimingBins;

static unsigned long m_NumKEvents;
static unsigned long m_BadAddress;
static unsigned long m_SyncProb;

static long m_HeadPresent[MAX_HEADS];
static long m_PointSource[MAX_HEADS];
static long m_NumLayers[MAX_HEADS];
static long m_MasterLayer[MAX_HEADS];
static long m_HeadType[MAX_HEADS];
static long m_AcquireSelect[MAX_HEADS];
static long m_SourceSelect[MAX_HEADS];
static long m_ArchiveSelect[MAX_HEADS];
static long m_AcquireLayer[MAX_HEADS];
static long m_AcquireStatus[MAX_HEADS];
static long m_ArchiveStatus[MAX_HEADS];

static char m_RootPath[MAX_STR_LEN];
static char m_Archive[MAX_STR_LEN];
static char m_ArchiveStage[MAX_STR_LEN];
static char m_ArchiveDir[MAX_STR_LEN];
static char m_MarkerFile[MAX_STR_LEN];

// constants
static char *LayerID[NUM_LAYER_TYPES] = {"Fast_", "Slow_", ""};
static char *LayerStr[NUM_LAYER_TYPES] = {"Fast\\", "Slow\\", ""};
static char *StrLayer[NUM_LAYER_TYPES] = {"fast", "slow", "both"};
static char *ModeExt[NUM_DHI_MODES+2] = {"pp", "en", "te", "sd", "run", "trans", "spect", "ti", "test", "time", "coinc"};
static char *SetTitle[MAX_SETTINGS] = { " pmta", " pmtb", " pmtc", " pmtd", "  cfd", "delay", 
										"off_x", "off_y", "off_e", " mode", "  KeV", 
										"shape", "  low", " high", "  low", " high",
										"c_set", "tgain", "t_off", "t_set", "cfd_0", 
										"cfd_1", "pkstd", "photo", " temp"};
static char *MonthStr[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static long SetIndex[MAX_SETTINGS];
static long OutOrder[MAX_SETTINGS];

static long m_Phase;

// thread subroutines
void Acquire_Singles_Thread(void *dummy);
void Acquire_Coinc_Thread(void *dummy);
void Archive_Load_Thread(void *dummy);
void Archive_Save_Thread(void *dummy);

static HANDLE hAcquireThread;
static HANDLE hArchiveThread;

// internal subroutines
long Local_Download(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Head, char *Filename, unsigned char *Headname);
long Local_Upload(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Head, unsigned char *Headname, char *Filename);
long Local_Backup(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long *HeadSelect);
long Local_Restore(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long *HeadSelect);
long Local_Save(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Configuration, long Head, long *pSize, unsigned char **buffer);
long Local_Load(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Configuration, long Head, long Size, unsigned char **buffer);
void Local_Download_CRM(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Configuration, long BlockSelect[], long BlockStatus[]);
long Add_Error(CErrorEventSupport *pErrSup, char *Where, char *What, long Why);
void Add_Information(CErrorEventSupport *pErrSup, char *Where, char *What);

// pointers to the classes
CErrorEventSupport *pErrEvtSup;
CErrorEventSupport *pErrEvtSupThread;
IAbstract_DHI *pDHI;
IACQMain *pACQ;
IAbstract_DHI *pAcqThreadDHI;
IACQMain *pAcqThreadACQ;

IAbstract_DHI* Get_DAL_Ptr(CErrorEventSupport *pErrSup);
IACQMain* Get_ACQ_Ptr(CErrorEventSupport *pErrSup);

#define PP_SIZE_X 256
#define PP_SIZE_Y 256
#define PP_SIZE 65536		// PP_SIZE_X * PP_SIZE_Y// DUMain.cpp : Implementation of CDUMain

#define ARCHIVE_LOAD 0
#define ARCHIVE_SAVE 1

/////////////////////////////////////////////////////////////////////////////
// CDUMain

/////////////////////////////////////////////////////////////////////////////
// Routine:		FinalConstruct
// Purpose:		Server Initialization
// Arguments:	None
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

HRESULT CDUMain::FinalConstruct()
{
	// local variables
	long Status = 0;

	HRESULT hr = S_OK;

	// if initialization fails, set return status
	if (Init_Globals() != 0) hr = S_FALSE;

	// return success
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Singles_Data
// Purpose:		control acquisition of new singles data and store in in the
//				appropriate file.
// Arguments:	DataMode			-	long = Data Mode
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//					8 = test data
//				Configiuration	-	long = Electronics Configuration
//				HeadSelect		-	long = Array of flags denoting whether or
//											not each head has been selected 
//				SourceSelect	-	long = Array of flags denoting whether or
//											not each point source has been selected 
//				LayerSelect		-	long = Array specifying layer for each head
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				AcquisitionType	-	long = (0 Seconds, 1 KEvents) units for amount
//				Amount			-	long = amount of data to acquire
//				lld				-	long = lower level discriminator
//				uld				-	long = upper level discriminator
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	Block check of lld/uld for Ring scanners
//				17 Nov 03	T Gremillion	added test data to header
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Acquire_Singles_Data(long DataMode, long Configuration, long HeadSelect[], long SourceSelect[], long LayerSelect[], long AcquisitionType, long Amount, long lld, long uld, long *pStatus)
{

	// local variables
	long i = 0;

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Acquire_Singles_Data";

	// initialize status
	m_AcqAbort = 0;
	m_Acquiring = 0;
	m_AcquirePercent = 0;
	*pStatus = 0;

	// argument check - data mode
	if ((DataMode < 0) || (DataMode >= NUM_DHI_MODES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, DataMode", DataMode);
		return S_FALSE;
	}

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if ((HeadSelect[i] == 1) && (m_HeadPresent[i] != 1)) {
			*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Head (Not Present)", i);
			return S_FALSE;
		}
	}

	// argument check - head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if ((SourceSelect[i] == 1) && (m_HeadPresent[i] != 1)) {
			*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Head For Point Source (Not Present)", i);
			return S_FALSE;
		}
	}

	// argument check - head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if ((SourceSelect[i] == 1) && (m_PointSource[i] == 0)) {
			*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Point Source (Not Present)", i);
			return S_FALSE;
		}
	}

	// argument check - layer selection
	for (i = 0 ; i < MAX_HEADS ; i++) if (HeadSelect[i] == 1) {
		if ((LayerSelect[i] < 0) && (LayerSelect[i] >= NUM_LAYER_TYPES)) {
			sprintf(Str, "Invalid Argument, Head %d Layer", i);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, LayerSelect[i]);
			return S_FALSE;
		}
	}

	// argument check - acquisition type
	if ((AcquisitionType < 0) || (AcquisitionType > 1)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Acquisition Type", AcquisitionType);
		return S_FALSE;
	}

	// argument check - amount of data
	if (Amount < 0) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Amount", Amount);
		return S_FALSE;
	}

#ifndef ECAT_SCANNER

	// verify lld
	if ((lld < 0) || (lld > 1500)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Lower Level Discriminator", lld);
		return S_FALSE;
	}

	// verify uld
	if ((uld < 1) || (uld > 1500)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Upper Level Discriminator", uld);
		return S_FALSE;
	}

	// verify lld and uld against each other
	if (uld <= lld) {
		sprintf(Str, "Invalid Argument, LLD = %d Must Be Less Than ULD", lld);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, uld);
		return S_FALSE;
	}

#endif

	// if an acquisition is already running, then return
	if (m_AcquireFlag != 0) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Acquisition Already in Progress, State", m_AcquireFlag);
		return S_FALSE;
	}

	// set setup flag
	m_AcquireFlag = 1;

	// start thread
	*pStatus = Acquire_Singles_Driver(DataMode, Configuration, HeadSelect, SourceSelect, LayerSelect, AcquisitionType, Amount, lld, uld);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Coinc_Data
// Purpose:		control acquisition of new coincidence data and store in in the
//				appropriate file.
// Arguments:	DataMode			-	long = Data Mode
//					8 = Time Difference
//					9 = Coincidence Flood
//				Configiuration	-	long = Electronics Configuration
//				HeadSelect		-	long = Array of flags denoting whether or
//											not each head has been selected 
//				LayerSelect		-	long = Array specifying layer for each head
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				ModulePair		-	long = bitfield for detector head pairs which are allowed to be in coiincidence
//				Transmission	-	long = flag for whether or not point sources are to be extended
//				AcquisitionType	-	long = (0 Seconds, 1 KEvents) units for amount
//				Amount			-	long = amount of data to acquire
//				lld				-	long = lower level discriminator
//				uld				-	long = upper level discriminator
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Acquire_Coinc_Data(long DataMode, long Configuration, long HeadSelect[], long LayerSelect[], long ModulePair, long Transmission, long AcquisitionType, long Amount, long lld, long uld, long *pStatus)
{
	// local variables
	long i = 0;

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Acquire_Coinc_Data";

	// initialize status
	m_AcqAbort = 0;
	m_Acquiring = 0;
	m_AcquirePercent = 0;
	*pStatus = 0;

	// argument check - data mode
	if ((DataMode != DATA_MODE_TIMING) && (DataMode != DATA_MODE_COINC_FLOOD)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, DataMode", DataMode);
		return S_FALSE;
	}

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if ((HeadSelect[i] == 1) && (m_HeadPresent[i] != 1)) {
			*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Head (Not Present)", i);
			return S_FALSE;
		}
	}

	// argument check - layer selection
	for (i = 0 ; i < MAX_HEADS ; i++) if (HeadSelect[i] == 1) {
		if ((LayerSelect[i] < 0) && (LayerSelect[i] >= NUM_LAYER_TYPES)) {
			sprintf(Str, "Invalid Argument, Head %d Layer", i);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, LayerSelect[i]);
			return S_FALSE;
		}
	}

	// Module Pair
	if (((SCANNER_P39 == m_ScannerType) || (SCANNER_P39_IIA == m_ScannerType)) && ((ModulePair <= 0) || (ModulePair > 0x1FFE))) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Module Pair", ModulePair);
		return S_FALSE;
	}

	// argument check - acquisition type
	if ((AcquisitionType < 0) || (AcquisitionType > 1)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Acquisition Type", AcquisitionType);
		return S_FALSE;
	}

	// argument check - amount of data
	if (Amount < 0) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Amount", Amount);
		return S_FALSE;
	}

	// verify lld
	if ((lld < 0) || (lld > 1500)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Lower Level Discriminator", lld);
		return S_FALSE;
	}

	// verify uld
	if ((uld < 1) || (uld > 1500)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Upper Level Discriminator", uld);
		return S_FALSE;
	}

	// verify lld and uld against each other
	if (uld <= lld) {
		sprintf(Str, "Invalid Argument, LLD = %d Must Be Less Than ULD", lld);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, uld);
		return S_FALSE;
	}

	// if an acquisition is already running, then return
	if (m_AcquireFlag != 0) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Acquisition Already in Progress, State", m_AcquireFlag);
		return S_FALSE;
	}

	// set setup flag
	m_AcquireFlag = 1;

	// start thread
	*pStatus = Acquire_Coinc_Driver(DataMode, Configuration, HeadSelect, LayerSelect, ModulePair, Transmission, AcquisitionType, Amount, lld, uld);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquisition_Abort
// Purpose:		abort acquisition in progress
// Arguments:	none
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
//				pointer to acquisition server needes to be acquired and released
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Acquisition_Abort()
{
	// local variables
	HRESULT hr = S_OK;

	// set abort flag
	m_AcqAbort = 1;

	// if data is being acquired, issue abort to acquisition server
	if (m_Acquiring == 1) {

		// get a global pointer to the acquisition server
		pACQ = Get_ACQ_Ptr(pErrEvtSup);

		// if pointer acquired
		if (pACQ != NULL) {

			// issue abort command
			hr = pACQ->Abort();

			// release acquisition server - reconnect as needed
			pACQ->Release();
			pACQ = NULL;
		}
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquisition_Progress
// Purpose:		reports the percent complete of an acquisition
//				(0% indicates the acquisition is still being initialized)
// Arguments:	pPercent		-	long = percent complete
//				HeadStatus		-	long = array containing acquisition status for each head
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Acquisition_Progress(long *pPercent, long HeadStatus[])
{
	// local variables 
	long i = 0;

	// transfer values
	*pPercent = m_AcquirePercent;

	// if finished, then release thread handle
	if ((m_AcquirePercent == 100) && (hAcquireThread != NULL)) {

		// transfer status
		for (i = 0 ; i < MAX_HEADS ; i++) HeadStatus[i] = m_AcquireStatus[i];

		// clear acquisition flag
		m_AcquireFlag = 0;

		// clear handle
		hAcquireThread = NULL;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Archive_Abort
// Purpose:		abort archive save or load in progress
// Arguments:	none
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Archive_Abort()
{
	// local variables
	long i = 0;
	long Status = 0;

	unsigned char Command[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// setup thread
	if ((hArchiveThread != NULL) && (m_ArchiveMode != -1)) {

		// informational message
		Add_Information(pErrEvtSup, "Archive Abort", "Archive Aborted");

		// kill thread
		TerminateThread(hArchiveThread, 0);

		// Release Handle
		CloseHandle(hArchiveThread);

		// clear handle
		hArchiveThread = NULL;

		// head status to abort
		for (i = 0 ; i < MAX_HEADS ; i++) {
			if (m_ArchiveSelect[i] == 1) m_ArchiveStatus[i] = -1;
		}

		// if loading, restore backup
		if (m_ArchiveMode = ARCHIVE_LOAD) {

			// restore the backup
			Local_Restore(pErrEvtSup, pDHI, m_ArchiveSelect);

		// saving, delete files already created
		} else {

			// delete file
			sprintf((char *) Command, "erase %s\\Archive\\%s /q /f", m_RootPath, m_MarkerFile);
			system((char *) Command);

			// delete directory
			sprintf((char *) Command, "rmdir %s\\Archive\\%s /s /q", m_RootPath, m_ArchiveDir);
			system((char *) Command);
		}
	}

	// set completion
	m_ArchiveMode = -1;
	m_ArchivePercent = 100;

	// clear stage string
	sprintf(m_ArchiveStage, "Archive Idle");

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Archive_Progress
// Purpose:		reports the percent complete of an archive load or save
// Arguments:	pPercent		-	long = percent complete
//				StageStr		-	unsigned char = string percentage
//				HeadStatus		-	long = array containing acquisition status for each head
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Archive_Progress(long *pPercent, unsigned char StageStr[], long HeadStatus[])
{
	// local variables 
	long i = 0;

	// transfer values
	*pPercent = m_ArchivePercent;
	sprintf((char *) StageStr, "%s", m_ArchiveStage);

	// if finished, then release thread handle
	if ((m_ArchivePercent == 100) && (hArchiveThread != NULL)) {

		// transfer status
		for (i = 0 ; i < MAX_HEADS ; i++) HeadStatus[i] = m_ArchiveStatus[i];

		// clear acquisition flag
		m_ArchiveFlag = 0;

		// clear handle
		hArchiveThread = NULL;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Current_Head_Data
// Purpose:		returns array containing requested data from file system
// Arguments:	DataMode			-	long = Data Mode
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//	`				8 = Time Difference
//					9 = Coincidence Flood
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				*pTime			-	long = time of file creation
//				*pSize			-	long = size of data in bytes
//				**Data			-	long = output data array
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
//				20 Nov 03	T Gremillion	added break after DHI_MODE_TEST
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Current_Head_Data(long DataMode, long Configuration, long Head, long Layer, long *pTime, long *pSize, long **Data, long *pStatus)
{
	// local variables
	unsigned long ByteSize = 0;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Current_Head_Data";
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - data mode
	if ((DataMode < 0) || (DataMode > (NUM_DHI_MODES+1))) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, DataMode", DataMode);
		return S_FALSE;
	}

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_data.%s", m_RootPath, Configuration, Head, LayerStr[Layer], Head, ModeExt[DataMode]);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// retrieve date/time created
	*pTime = file_status.st_mtime;

	// Determine the Size
	switch (DataMode) {

	// position profile an crystal energy are same size
	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_CRYSTAL_ENERGY:
	case DHI_MODE_TIME:
		*pSize = 256 * 256 * m_NumBlocks;
		break;

	// tube energy
	case DHI_MODE_TUBE_ENERGY:
		*pSize = 256 * 4 * m_NumBlocks;
		break;

	// shape discrimination
	case DHI_MODE_SHAPE_DISCRIMINATION:
		*pSize = 256 * m_NumBlocks;
		break;

	// run mode and transmission mode are the same
	case DHI_MODE_RUN:
	case DHI_MODE_TRANSMISSION:
	case DATA_MODE_COINC_FLOOD:
		*pSize = 128 * 128;
		break;

	// SPECT
	case DHI_MODE_SPECT:
		*pSize = 256 * 128 * 128;
		break;

	// timing
	case DATA_MODE_TIMING:
		*pSize = m_TimingBins * 128 * 128;
		break;

	// test data
	case DHI_MODE_TEST:
		*pSize = 256 * 256 * 2;
		break;

	// unknown
	default:
		*pSize = 0;
		*Data = NULL;
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Unknown Data Mode", DataMode);
		break;
	}

	// allocate the memory
	ByteSize = sizeof(long) * *pSize;
	if (ByteSize > 0) {
		*Data = (long *) ::CoTaskMemAlloc(ByteSize);
		if (*Data == NULL) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Allocating Memory, Requested Size", ByteSize);
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// read file
		if (*pSize != (long) fread((void *) *Data, sizeof(long), *pSize, fp)) {
			sprintf(Str, "Reading File (%s), Requested Elements", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, *pSize);
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Current_Block_Data
// Purpose:		returns array containing requested data from file system
// Arguments:	DataMode			-	long = Data Mode
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//	`				8 = Time Difference
//					9 = Coincidence Flood
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				Block			-	long = Block number
//				*pSize			-	long = size of data in bytes
//				**Data			-	long = output data array
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	For Crystal based data types, block
//											numbering is different for ring scanners
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Current_Block_Data(long DataMode, long Configuration, long Head, long Layer, long Block, long *pSize, long **Data, long *pStatus)
{
	// local variables
	unsigned long ByteSize = 0;
	long RowSize = 0;
	long CrystalSize = 0;
	long StartRow = 0;
	long StartCol = 0;
	long Row = 0;
	long DataStart = 0;
	long *ptr;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Current_Block_Data";
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;


	// argument check - data mode
	if ((DataMode < 0) || (DataMode > (NUM_DHI_MODES+1))) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, DataMode", DataMode);
		return S_FALSE;
	}

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// argument check - block number
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_data.%s", m_RootPath, Configuration, Head, LayerStr[Layer], Head, ModeExt[DataMode]);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// Determine the Size
	switch (DataMode) {

	// position profile an crystal energy are same size
	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_CRYSTAL_ENERGY:
	case DHI_MODE_TIME:
		*pSize = 256 * 256;
		break;

	// tube energy
	case DHI_MODE_TUBE_ENERGY:
		*pSize = 256 * 4;
		break;

	// shape discrimination
	case DHI_MODE_SHAPE_DISCRIMINATION:
		*pSize = 256;
		break;

	// run mode and transmission mode are the same
	case DHI_MODE_RUN:
	case DHI_MODE_TRANSMISSION:
	case DATA_MODE_COINC_FLOOD:
		CrystalSize = 1;
		*pSize = CrystalSize * m_XCrystals * m_YCrystals;
		break;

	// SPECT
	case DHI_MODE_SPECT:
		CrystalSize = 256;
		*pSize = CrystalSize * m_XCrystals * m_YCrystals;
		break;

	// timing
	case DATA_MODE_TIMING:
		CrystalSize = m_TimingBins;
		*pSize = CrystalSize * m_XCrystals * m_YCrystals;
		break;

	// unknown
	default:
		CrystalSize = 0;
		*pSize = 0;
		*Data = NULL;
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Unknown Data Mode", DataMode);
		break;
	}

	// allocate the memory
	ByteSize = sizeof(long) * *pSize;
	if (ByteSize > 0) {
		*Data = (long *) ::CoTaskMemAlloc(ByteSize);
		if (*Data == NULL) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Allocating Memory, Requested Size", ByteSize);
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// simple seek and read for pp, en, ti, te & shape
		if ((DataMode == DHI_MODE_POSITION_PROFILE) || (DataMode == DHI_MODE_CRYSTAL_ENERGY) || (DataMode == DHI_MODE_TIME) ||
			(DataMode == DHI_MODE_TUBE_ENERGY) || (DataMode == DHI_MODE_SHAPE_DISCRIMINATION)) {

			// move to the start of the block
			DataStart = *pSize * sizeof(long) * Block;
			if (fseek(fp, DataStart, SEEK_SET) == 0) {
				if (*pSize != (long) fread((void *) *Data, sizeof(long), *pSize, fp)) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Reading Data, Elements", *pSize);
			} else *pStatus = Add_Error(pErrEvtSup, Subroutine, "Moving To Start of Data, Position", DataStart);

		// others are crystal based, but a row from the same block is contiguous
		} else {

			// Determine Row Size
			RowSize = *pSize / m_YCrystals;

			// Determine Starting position
			if (SCANNER_RING == m_ScannerType) {
				StartRow = (Block % m_YBlocks) * m_YCrystals;
				StartCol = (Block / m_YBlocks) * m_XCrystals;
			} else {
				StartRow = (Block / m_XBlocks) * m_YCrystals;
				StartCol = (Block % m_XBlocks) * m_XCrystals;
			}
			DataStart = ((StartRow * 128) + StartCol) * CrystalSize * sizeof(long);

			// loop through the rows
			ptr = *Data;
			for (Row = 0 ; Row < m_YCrystals ; Row++) {

				// move to the new position and read the data
				if (*pStatus == 0) {
					if (fseek(fp, DataStart, SEEK_SET) == 0) {
						if (RowSize != (long) fread((void *) ptr, sizeof(long), RowSize, fp)) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Reading Data, Elements", RowSize);
					} else *pStatus = Add_Error(pErrEvtSup, Subroutine, "Moving To Start of Data, Position", DataStart);
				}

				// increment the memory pointer to the next row
				DataStart += 128 * CrystalSize * sizeof(long);
				ptr += m_XCrystals * CrystalSize;
			}
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Download_CRM
// Purpose:		downloads specified crms to heads
//				calls local routine to do download
// Arguments:	Configiuration	-	long = Electronics Configuration
//				BlockSelect		-	long = array of flags indicating which crms to download
//				BlockStatus		-	long = download status of each block
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Download_CRM(long Configuration, long BlockSelect[], long BlockStatus[])
{
	// call the local function
	Local_Download_CRM(pErrEvtSup, pDHI, Configuration, BlockSelect, BlockStatus);

	// successful return
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Local_Download_CRM
// Purpose:		downloads specified crms to heads
// Arguments:	pErrSup			-	CErrorEventSupport = error handler
//				pDHILocal		-	IAbstract_DHI = pointer to DAL
//				Configiuration	-	long = Electronics Configuration
//				BlockSelect		-	long = array of flags indicating which crms to download
//				BlockStatus		-	long = download status of each block
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	Give Head time to unzip file before preceeding
/////////////////////////////////////////////////////////////////////////////

void Local_Download_CRM(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Configuration, long BlockSelect[], long BlockStatus[])
{
	// local variables
	long i = 0;
	long index = 0;
	long Head = 0;
	long Layer = 0;
	long Block = 0;
	long NumBlocks = 0;
	long NumBytes = 0;
	long Result = 0;
	long Status = 0;

	char command[MAX_STR_LEN];
	char filein[MAX_STR_LEN];
	char fileout[MAX_STR_LEN];
	char WorkDir[MAX_STR_LEN];
	char Origfile[MAX_STR_LEN];
	char Downfile[MAX_STR_LEN];
	char Command[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Local_Download_CRM";

	unsigned char CRM[65536];

	FILE *fp_in = NULL;
	FILE *fp_out = NULL;

	HRESULT hr = S_OK;

	// initialize status
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) BlockStatus[i] = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		Status = Add_Error(pErrSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) if (BlockSelect[i] != 0) BlockStatus[i] = Status;
		return;
	}

	// check validity of blocks selected
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {

		// if block selected
		if (BlockSelect[i] != 0) {

			// head must be present and must be in valid range for a head
			if ((m_HeadPresent[i/MAX_BLOCKS] != 1) || ((i % MAX_BLOCKS) >= m_EmissionBlocks)) {
				BlockStatus[i] = Add_Error(pErrSup, Subroutine, "Invalid Argument, Block", i);
				return;
			}
		}
	}

	// erase all crm files from the temporary directory
	sprintf(command, "erase %s\\Current\\Temp\\*.crm /q /f", m_RootPath);
	Result = system(command);

	// Loop through all the heads
	for (Head = 0 ; Head < MAX_HEADS ; Head++) if (m_HeadPresent[Head] == 1) {

		// initialize number of blocks
		NumBlocks = 0;

		// index to first block in head
		index = Head * MAX_BLOCKS;

		// copy the blocks for this head over
		for (Block = 0 ; Block < m_NumBlocks ; Block++) {

			// block not selected, skip to the next one
			if (BlockSelect[index+Block] != 1) continue;

			// increment number of blocks
			NumBlocks++;

			// output file name
			sprintf(fileout, "%s\\Current\\Temp\\%d_%d.crm", m_RootPath, Configuration, Block);

			// scanner based
			switch (m_ScannerType) {

			case SCANNER_HRRT:
			case SCANNER_P39_IIA:

				// open the output file
				if ((fp_out = fopen(fileout, "wb")) == NULL) {
					sprintf(Str, "Could Not Create Temporary CRM File: %s, Block", fileout);
					Status = Add_Error(pErrSup, Subroutine, Str, Block);
					BlockStatus[index+Block] = Status;
					continue;
				}

				// original file name
				if (m_ScannerType == SCANNER_HRRT) {
					sprintf(filein, "%s\\Current\\Config_%d\\Head_%d\\Fast\\%d_%d.crm", m_RootPath, Configuration, Head, Configuration, Block);
				} else {
					sprintf(filein, "%s\\Current\\Config_%d\\Head_%d\\%d_%d.crm", m_RootPath, Configuration, Head, Configuration, Block);
				}

				if ((fp_in = fopen(filein, "rb")) != NULL) {

					// read the CRM
					NumBytes = fread((void *) CRM, sizeof(char), 65536, fp_in);

					// write the data out twice
					fwrite((void *) CRM, sizeof(char), NumBytes, fp_out);

					// close files
					fclose(fp_out);
					fclose(fp_in);

				} else {
					sprintf(Str, "Could Not Open CRM File: %s, Block", filein);
					Status = Add_Error(pErrSup, Subroutine, Str, Block);
					BlockStatus[index+Block] = Status;
					continue;
				}
				break;

			case SCANNER_P39:

				// open the output file
				if ((fp_out = fopen(fileout, "wb")) == NULL) {
					sprintf(Str, "Could Not Create Temporary CRM File: %s, Block", fileout);
					Status = Add_Error(pErrSup, Subroutine, Str, Block);
					BlockStatus[index+Block] = Status;
					continue;
				}

				// original file name
				sprintf(filein, "%s\\Current\\Config_%d\\Head_%d\\Fast\\%d_%d.crm", m_RootPath, Configuration, Head, Configuration, Block);

				if ((fp_in = fopen(filein, "rb")) != NULL) {

					// read the CRM
					NumBytes = fread((void *) CRM, sizeof(char), 65536, fp_in);

					// write the data out twice (dual layer CRM)
					fwrite((void *) CRM, sizeof(char), NumBytes, fp_out);
					fwrite((void *) CRM, sizeof(char), NumBytes, fp_out);

					// close files
					fclose(fp_out);
					fclose(fp_in);

				} else {
					sprintf(Str, "Could Not Open CRM File: %s, Block", filein);
					Status = Add_Error(pErrSup, Subroutine, Str, Block);
					BlockStatus[index+Block] = Status;
					continue;
				}
				break;

			case SCANNER_PETSPECT:

				// open the output file
				if ((fp_out = fopen(fileout, "wb")) == NULL) {
					sprintf(Str, "Could Not Create Temporary CRM File: %s, Block", fileout);
					Status = Add_Error(pErrSup, Subroutine, Str, Block);
					BlockStatus[index+Block] = Status;
					continue;
				}

				// only look for fast layer if PET
				if (Configuration == 0) {

					// original file name - fast layer
					sprintf(filein, "%s\\Current\\Config_%d\\Head_%d\\Fast\\%d_%d.crm", m_RootPath, Configuration, Head, Configuration, Block);

					if ((fp_in = fopen(filein, "rb")) != NULL) {

						// read the CRM
						NumBytes = fread((void *) CRM, sizeof(char), 65536, fp_in);

						// write the data out twice (dual layer CRM)
						fwrite((void *) CRM, sizeof(char), NumBytes, fp_out);

						// close files
						fclose(fp_in);

					} else {
						sprintf(Str, "Could Not Open CRM File: %s, Block", filein);
						Status = Add_Error(pErrSup, Subroutine, Str, Block);
						BlockStatus[index+Block] = Status;
						continue;
					}
				}

				// original file name - slow layer
				sprintf(filein, "%s\\Current\\Config_%d\\Head_%d\\Slow\\%d_%d.crm", m_RootPath, Configuration, Head, Configuration, Block);

				if ((fp_in = fopen(filein, "rb")) != NULL) {

					// read the CRM
					NumBytes = fread((void *) CRM, sizeof(char), 65536, fp_in);

					// write the data out twice (dual layer CRM)
					fwrite((void *) CRM, sizeof(char), NumBytes, fp_out);

					// for spect modes, write it a second time (dual layer CRM)
					if (Configuration != 0) fwrite((void *) CRM, sizeof(char), NumBytes, fp_out);

					// close files
					fclose(fp_in);
					fclose(fp_out);

				} else {
					sprintf(Str, "Could Not Open CRM File: %s, Block", filein);
					Status = Add_Error(pErrSup, Subroutine, Str, Block);
					BlockStatus[index+Block] = Status;
					continue;
				}
				break;

			default:
				break;
			}
		}

		// if no blocks processed, then continue to next loop
		if (NumBlocks == 0) continue;

		// copy peak position files
		for (Layer = 0 ; Layer < NUM_LAYER_TYPES ; Layer++) {

			// combined only for P39 Phase IIA
			if ((m_ScannerType == SCANNER_P39_IIA) && (Layer != LAYER_ALL)) continue;

			// only P39 Phase IIA uses combined
			if ((m_ScannerType != SCANNER_P39_IIA) && (Layer == LAYER_ALL)) continue;

			// fast layer only for P39
			if ((m_ScannerType == SCANNER_P39) && (Layer != LAYER_FAST)) continue;

			// HRRT is fast only
			if ((m_ScannerType == SCANNER_HRRT) && (Layer != LAYER_FAST) && (Configuration != 0)) continue;

			// petspect does not use fast layer except for PET
			if ((m_ScannerType == SCANNER_PETSPECT) && (Layer == LAYER_FAST) && (Configuration != 0)) continue;

			// copy the file (the destination file must be cleared first)
			sprintf(Origfile, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_%d_peaks.bin", m_RootPath, Configuration, Head, LayerStr[Layer], Head, Configuration);
			sprintf(Downfile, "%s\\Current\\Temp\\%d_pk%s.bin", m_RootPath, Configuration, StrLayer[Layer]);
			sprintf(Command, "erase %s /f /q", Downfile);
			system(Command);
			sprintf(Command, "copy %s %s", Origfile, Downfile);
			system(Command);
		}

		// if blocks were copied, then download them
		if (NumBlocks > 0) {

			// change the working directory
			sprintf(WorkDir, "%s\\Current\\Temp", m_RootPath);
			_chdir(WorkDir);

			// build zip file
			sprintf(command, "pkzip temp.zip *.crm %d_pk*.bin", Configuration);
			Result = system(command);

			// download the zip file
			Status = Local_Download(pErrSup, pDHILocal, Head, "temp.zip", (unsigned char *) "temp.zip");

			// erase all crm files from the temporary directory
			sprintf(command, "erase *.crm /q /f");
			Result = system(command);

			// erase zip file from the temporary directory
			sprintf(command, "erase temp.zip /q /f");
			Result = system(command);

			// wait for head to finish unzipping
			Sleep(30000);
		}
	}

	// force the heads to reload equations the next time through
	hr = pDHILocal->Force_Reload();
	if ((Status == 0) && FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Force_Reload)", hr);

	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Stored_CRM
// Purpose:		returns array containing crystal region map for a specific block
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				Block			-	long = Block number
//				CRM				-	unsigned char = crystal region map
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	moved value assignment outside of loop
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Get_Stored_CRM(long Configuration, long Head, long Layer, long Block, unsigned char CRM[], long *pStatus)
{
	// local variables
	long i = 0;
	long curr = 0;
	long next = 0;
	long value = 0;
	long X = 0;
	long Y = 0;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Get_Stored_CRM";
	unsigned char buffer[131072];		// 256 * 256 * 2 - worst case
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// argument check - block number
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%s%d_%d.crm", m_RootPath, Configuration, Head, LayerStr[Layer], Configuration, Block);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// read file
		if (file_status.st_size != (long) fread((void *) buffer, sizeof(unsigned char), file_status.st_size, fp)) {
			sprintf(Str, "Reading File (%s), Requested Elements", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, file_status.st_size);
		}

		// close the file
		fclose(fp);
	}

	// decode
	if (*pStatus == 0) for (next = 0, curr = 0 ; next < file_status.st_size ; next += 2) {
		value = (long) buffer[next];
		for (i = 0 ; i < (long) buffer[next+1] ; i++, curr++) {
			if (value == 255) {
				CRM[curr] = 255;
			} else {
				X = value % 16;
				Y = value / 16;
				CRM[curr] = (unsigned char)((Y * m_XCrystals) + X);
			}
		}
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Stored_Crystal_Position
// Purpose:		returns array containing crystal positions for a specific head
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				CrystalPosition	-	long = crystal positions
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Get_Stored_Crystal_Position(long Configuration, long Head, long Layer, long CrystalPosition[], long *pStatus)
{
	// local variables
	long i = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long FileSize = 0;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Get_Stored_Crystal_Position";
	unsigned char buffer[278784];					// 128 * ((2 * 16) + 1) * ((2 * 16) + 1) * 2 - worst case
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_%d_peaks.bin", m_RootPath, Configuration, Head, LayerStr[Layer], Head, Configuration);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// calculate file size (also included vertices between crystals
		if (m_ScannerType != SCANNER_RING) {
			Verts_X = (m_XCrystals * 2) + 1;
			Verts_Y = (m_YCrystals * 2) + 1;
			FileSize = m_EmissionBlocks * Verts_X * Verts_Y * 2;
		} else {
			FileSize = m_EmissionBlocks * m_XCrystals * m_YCrystals * 2;
		}

		// read file
		if (FileSize != (long) fread((void *) buffer, sizeof(unsigned char), FileSize, fp)) {
			sprintf(Str, "Reading File (%s), Requested Elements", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, FileSize);
		}

		// close the file
		fclose(fp);
	}

	// transfer to long integer array
	if (*pStatus == 0) {
		for (i = 0 ; i < FileSize ; i++) CrystalPosition[i] = (long) buffer[i];
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Master_Crystal_location
// Purpose:		returns array containing master crystal positions for a specific head
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				CrystalPosition	-	long = crystal positions
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Get_Master_Crystal_Location(long Configuration, long Head, long Layer, long CrystalPosition[], long *pStatus)
{
	// local variables
	long i = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long FileSize = 0;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Get_Master_Crystal_Location";
	unsigned char buffer[278784];					// 128 * ((2 * 16) + 1) * ((2 * 16) + 1) * 2 - worst case
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%sDefaults\\h%d_%d_%sPeaks.bin", m_RootPath, Head, Configuration, LayerID[Layer]);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// calculate file size (also included vertices between crystals
		Verts_X = (m_XCrystals * 2) + 1;
		Verts_Y = (m_YCrystals * 2) + 1;
		FileSize = m_EmissionBlocks * Verts_X * Verts_Y * 2;

		// read file
		if (FileSize != (long) fread((void *) buffer, sizeof(unsigned char), FileSize, fp)) {
			sprintf(Str, "Reading File (%s), Requested Elements", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, FileSize);
		}

		// close the file
		fclose(fp);
	}

	// transfer to long integer array
	if (*pStatus == 0) {
		for (i = 0 ; i < FileSize ; i++) CrystalPosition[i] = (long) buffer[i];
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Stored_Crystal_Time
// Purpose:		returns array containing crystal time offsets for a specific head
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				CrystalTime		-	long = crystal time offsets
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Get_Stored_Crystal_Time(long Configuration, long Head, long Layer, long CrystalTime[], long *pStatus)
{
	// local variables
	long i = 0;
	long Block = 0;
	char BlockLayer[MAX_STR_LEN];
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Get_Stored_Crystal_Time";
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_time.txt", m_RootPath, Configuration, Head, LayerStr[Layer], Head);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rt")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// loop until end of file
		while ((Block < (m_NumBlocks-1)) && (*pStatus == 0) && (feof(fp) == 0)) {

			// read in block number and layer
			if (fscanf(fp, "%d %4s ", &Block, &BlockLayer) != 2) {
				sprintf(Str, "Reading File (%s), Requested Elements", filename);
				*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 2);
			}

			// read in the crystal values
			for (i = 0 ; (i < 256) && (*pStatus == 0) ; i++) {
				if (fscanf(fp, "%d", &CrystalTime[(Block*256)+i]) != 1) {
					sprintf(Str, "Reading File (%s), Requested Elements", filename);
					*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 1);
				}
			}
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Stored_Energy_Peaks
// Purpose:		returns array containing crystal energy peaks for a specific head
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				CrystalPeaks	-	long = crystal energy peaks
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Get_Stored_Energy_Peaks(long Configuration, long Head, long Layer, long CrystalPeaks[], long *pStatus)
{
	// local variables
	long i = 0;
	long Block = 0;
	char BlockLayer[MAX_STR_LEN];
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Get_Stored_Energy_Peaks";
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_ce.txt", m_RootPath, Configuration, Head, LayerStr[Layer], Head);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rt")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// loop until end of file
		while ((Block < (m_EmissionBlocks-1)) && (*pStatus == 0) && (feof(fp) == 0)) {

			// read in block number and layer
			if (fscanf(fp, "%d %4s ", &Block, &BlockLayer) != 2) {
				sprintf(Str, "Reading File (%s), Requested Elements", filename);
				*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 2);
			}

			// read in the crystal values
			for (i = 0 ; (i < 256) && (*pStatus == 0) ; i++) {
				if (fscanf(fp, "%d", &CrystalPeaks[(Block*256)+i]) != 1) {
					sprintf(Str, "Reading File (%s), Requested Elements", filename);
					*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 1);
				}
			}
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Stored_Settings
// Purpose:		returns array containing ASIC settings
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Settings		-	long = ASIC settings
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Get_Stored_Settings(long Configuration, long Head, long Settings[], long *pStatus)
{
	// local variables
	long i = 0;
	long Block = 0;
	char TitleStr[MAX_STR_LEN];
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Get_Stored_Settings";
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\h%d_settings.txt", m_RootPath, Configuration, Head, Head);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rt")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// read the title string
		if (fgets(TitleStr, 120, fp) == NULL) {
			sprintf(Str, "Reading File (%s), Maximum Requested Elements", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 120);
		}

		// loop until end of file
		while ((*pStatus == 0) && (feof(fp) == 0) && (Block < (m_NumBlocks-1))) {

			// read in block number
			if (fscanf(fp, "%d", &Block) != 1) {
				sprintf(Str, "Reading File (%s), Requested Elements", filename);
				*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 1);
			}

			// read in the settings
			for (i = 0 ; (i < m_NumSettings) && (*pStatus == 0) ; i++) {
				if (fscanf(fp, "%d", &Settings[(Block*m_NumSettings)+OutOrder[i]]) != 1) {
					sprintf(Str, "Reading File (%s), Requested Elements", filename);
					*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 1);
				}
			}
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		List_Setup_Archives
// Purpose:		returns array of strings with names of existing archives meeting criteria
// Arguments:	Configiuration	-	long = Electronics Configuration
//				HeadSelect		-	long = Flag array specifying heads which must be in archive 
//				*pNumFound		-	long = number of archives found meeting criteria
//				**List			-	unsigned char = array of strings with archve names
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::List_Setup_Archives(long Configuration, long HeadSelect[], long *pNumFound, unsigned char **List, long *pStatus)
{
	// local variables
	long i = 0;
	long ListHandle = 0;
	long finished = 0;
	long FilesFound = 0;
	char search[MAX_STR_LEN];
	char Subroutine[80] = "List_Setup_Archives";
	unsigned char *ptr = NULL;
	_finddata_t FindList;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if ((HeadSelect[i] == 1) && (m_HeadPresent[i] != 1)) {
			*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Head (Not Present)", i);
			return S_FALSE;
		}
	}

	// initialize return values
	*List = NULL;

	// build search string
	sprintf(search, "%sArchive\\*.h*", m_RootPath);
	for (i = 0 ; i < MAX_HEADS ; i++) if (HeadSelect[i] == 1) sprintf(search, "%s%d*", search, i);
	sprintf(search, "%s_%d", search, Configuration);

	// find any files that match the search string
	ListHandle = _findfirst(search, &FindList);
	if (ListHandle != -1) {

		// find all the files
		while (finished == 0) {

			// increment the number found
			FilesFound++;

			// pull out the next file name
			finished = _findnext(ListHandle, &FindList);
		}

		// release the resource - will also reset finished to 0
		finished = _findclose(ListHandle);
	}

	// if files were found
	if (FilesFound > 0) {

		// allocate memory
		*List = (unsigned char *) ::CoTaskMemAlloc((unsigned long) (FilesFound * 2048));

		// if the memory allocated ok
		if (*List != NULL) {

			// transfer memory pointer
			ptr = *List;

			// get the first match again
			ListHandle = _findfirst(search, &FindList);

			// find all the files
			while (finished == 0) {

				// copy filename
				sprintf((char *) ptr, "%s", FindList.name);

				// pull out the next file name
				finished = _findnext(ListHandle, &FindList);

				// incremnt memory pointer
				ptr += 2048;
			}

				// release the resource
				finished = _findclose(ListHandle);
		} else {
			*pStatus = Add_Error(pErrEvtSup, Subroutine, "Allocating Memory, Requested Size", (FilesFound * 2048));
		}
	}

	// transfer the number of files found 
	*pNumFound = FilesFound;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Load_Setup_Archive
// Purpose:		loads a specified setup archive
// Arguments:	Archive			-	unsigned char = name of selected archive
//				HeadSelect		-	long = flag array specifying heads to load 
//				HeadStatus		-	long = archive load status for each head
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Load_Setup_Archive(unsigned char Archive[], long HeadSelect[], long HeadStatus[])
{
	// local variables
	long i = 0;
	char Subroutine[80] = "Load_Setup_Archive";

	// initialize status
	for (i = 0 ; i < MAX_HEADS ; i++) HeadStatus[i] = 0;

	// argument check - head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if ((HeadSelect[i] == 1) && (m_HeadPresent[i] != 1)) {
			HeadStatus[i] = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Head (Not Present)", i);
			return S_FALSE;
		}
	}

	// transfer variables
	for (i = 0 ; i < MAX_HEADS ; i++) m_ArchiveSelect[i] = HeadSelect[i];
	sprintf(m_Archive, "%s", Archive);

	// initialize percent complete to 0
	m_ArchivePercent = 0;
	sprintf(m_ArchiveStage, "Load Archive Start");

	// start up thread
	hArchiveThread = (HANDLE) _beginthread(Archive_Load_Thread, 0, NULL);

	// check for error starting thread
	if ((long)hArchiveThread == -1) for (i = 0 ; i < MAX_HEADS ; i++) HeadStatus[i] = 4;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Remove_Setup_Archive
// Purpose:		deletes the files containing the specified archive
// Arguments:	Archive			-	long = Setup archive to be deleted
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Remove_Setup_Archive(unsigned char Archive[], long *pStatus)
{
	long i = 0;
	char filename[MAX_STR_LEN];
	char dirname[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Remove_Setup_Archive";

	// initialize status
	*pStatus = 0;

	// full path to file
	sprintf(filename, "%sArchive\\%s", m_RootPath, Archive);

	// associated directory
	sprintf(dirname, "%s", filename);
	for (i = strlen(dirname)-1 ; (i > 0) && (dirname[i] != 46) ; i--) dirname[i] = 0;
	dirname[i] = 0;

	// erase the archive file
	sprintf(command, "erase %s /f /q", filename);
	if (system(command) == -1) {
		sprintf(Str, "Erasing File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// erase the archive file
	sprintf(command, "rmdir %s /s /q", dirname);
	if (system(command) == -1) {
		sprintf(Str, "Erasing Directory (%s)", dirname);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Save_Setup_Archive
// Purpose:		saves a setup archive
// Arguments:	Configurations	-	long = electronics configuration
//				HeadSelect		-	long = flag array specifying heads to save 
//				HeadStatus		-	long = archive save status for each head
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Save_Setup_Archive(long Configuration, long HeadSelect[], long HeadStatus[])
{
	// local variables
	long i = 0;
	long Status = 0;
	char Subroutine[80] = "Save_Setup_Archives";

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		Status = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		for (i = 0 ; i < MAX_HEADS ; i++) if (HeadSelect[i] == 1) HeadStatus[i] = Status;
		return S_FALSE;
	}

	// argument check - head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if ((HeadSelect[i] == 1) && (m_HeadPresent[i] != 1)) {
			HeadStatus[i] = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Head (Not Present)", i);
			return S_FALSE;
		}
	}

	// initialize status
	for (i = 0 ; i < MAX_HEADS ; i++) HeadStatus[i] = 0;

	// transfer variables
	for (i = 0 ; i < MAX_HEADS ; i++) m_ArchiveSelect[i] = HeadSelect[i];
	m_Configuration = Configuration;

	// initialize percent complete to 0
	m_ArchivePercent = 0;
	sprintf(m_ArchiveStage, "Save Archive Start");

	// start up thread
	hArchiveThread = (HANDLE) _beginthread(Archive_Save_Thread, 0, NULL);

	// check for error starting thread
	if ((long)hArchiveThread == -1) for (i = 0 ; i < MAX_HEADS ; i++) HeadStatus[i] = 4;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Store_CRM
// Purpose:		stores the crystal region map for a specific block to a file
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				Block			-	long = Block number
//				CRM				-	unsigned char = crystal region map
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Store_CRM(long Configuration, long Head, long Layer, long Block, unsigned char CRM[], long *pStatus)
{
	// local variables
	long i = 0;
	long curr = 0;
	long next = 0;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Store_CRM";
	unsigned char value = 0;
	unsigned char x = 0;
	unsigned char y = 0;
	unsigned char buffer[131072];		// 256 * 256 * 2 - worst case
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// argument check - block number
	if ((Block < 0) || (Block >= m_NumBlocks)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Block", Block);
		return S_FALSE;
	}

	// encode
	while (curr < 65536) {

		// translate value to 16 x 16 block value and insert in field
		value = CRM[curr];
		if (value == 255) {
			buffer[next] = 255;
		} else {
			x = value % (unsigned char) m_XCrystals;
			y = value / (unsigned char) m_XCrystals;
			buffer[next] = (16 * y) + x;
		}

		// initialize count and move to the next value
		buffer[next+1] = 1;
		curr++;

		// loop while the same value
		if (curr < 65450) {
			*pStatus = 0;
		}
		while ((curr < 65536) && (CRM[curr] == value) && (buffer[next+1] < 63)) {
			buffer[next+1]++;
			curr++;
		}

		// next piece
		next += 2;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%s%d_%d.crm", m_RootPath, Configuration, Head, LayerStr[Layer], Configuration, Block);

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "wb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// write and close
	if (*pStatus == 0) {

		// write file
		if (next != (long) fwrite((void *) buffer, sizeof(unsigned char), next, fp)) {
			sprintf(Str, "Writing To File (%s)", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Store_Crystal_Position
// Purpose:		saves crystal positions for a specific head to a file
// Arguments:	Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				CrystalPosition	-	long = crystal positions
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Store_Crystal_Position(long Configuration, long Head, long Layer, long CrystalPosition[], long *pStatus)
{
	// local variables
	long i = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long FileSize = 0;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Store_Crystal_Position";
	unsigned char buffer[278784];					// 128 * ((2 * 16) + 1) * ((2 * 16) + 1) * 2 - worst case
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// calculate file size (also included vertices between crystals for panel detectors)
	if (m_ScannerType != SCANNER_RING) {
		Verts_X = (m_XCrystals * 2) + 1;
		Verts_Y = (m_YCrystals * 2) + 1;
		FileSize = m_EmissionBlocks * Verts_X * Verts_Y * 2;
	} else {
		FileSize = m_EmissionBlocks * m_XCrystals * m_YCrystals * 2;
	}

	// transfer to unsigned char array
	if (*pStatus == 0) {
		for (i = 0 ; i < FileSize ; i++) buffer[i] = (unsigned char) CrystalPosition[i];
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_%d_peaks.bin", m_RootPath, Configuration, Head, LayerStr[Layer], Head, Configuration);

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "wb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// write and close
	if (*pStatus == 0) {

		// write file
		if (FileSize != (long) fwrite((void *) buffer, sizeof(unsigned char), FileSize, fp)) {
			sprintf(Str, "Writing To File (%s)", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Store_Crystal_Time
// Purpose:		stores crystal time offsets for a specific head to a file
// Arguments:	Send			-	long = flag whether or not to send values to the head
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				CrystalTime		-	long = crystal time offsets
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
//				15 Apr 04	T Gremillion	Added for ring scanners
//				16 Jun 04	T Gremillion	store as +/- values
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Store_Crystal_Time(long Send, long Configuration, long Head, long Layer, long CrystalTime[], long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;
	long Block = 0;
	long CheckSum = 0;
	long Value;

	long download[512];

	char OutStr[MAX_STR_LEN];
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Store_Crystal_Time";

	unsigned char headname[MAX_STR_LEN];

	HRESULT hr = S_OK;

	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_time.txt", m_RootPath, Configuration, Head, LayerStr[Layer], Head);

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "wt")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// write and close
	if (*pStatus == 0) {

		// loop until end of file
		for (i = 0 ; i < m_NumBlocks ; i++) {

			// build up the line for each block
			sprintf(OutStr, "%3d %s", i, StrLayer[Layer]);
			for (j = 0 ; j < 256 ; j++) {
				Value = CrystalTime[(i*256)+j];
				if ((SCANNER_RING == m_ScannerType) && (Value > 32)) Value -= 64;
				sprintf(OutStr, "%s %3d", OutStr, Value);
			}

			// write out the line
			fprintf(fp, "%s\n", OutStr);
		}

		// close the file
		fclose(fp);
	}

	// send the values to the head if requested
	if ((*pStatus == 0) && (Send == 1)) {

		// panel detectors
		if (m_ScannerType != SCANNER_RING) {

			// filename on head
			sprintf((char *) headname, "%d_tm%s.txt", Configuration, StrLayer[Layer]);

			// download the file
			*pStatus = Local_Download(pErrEvtSup, pDHI, Head, filename, headname);

		// ring scanners - phase II electronics
		} else {

			// loop through the blocks
			for (Block = 0 ;  (Block < m_NumBlocks) && (*pStatus == 0) ; Block++) {

				// format the values properly for the bucket
				for (i = 0 ; i < (m_XCrystals*m_YCrystals) ; i++) {
					j = ((i / m_XCrystals) * 16) + (i % m_XCrystals);
					download[i] = CrystalTime[(Block*256)+j];
				}

				// send values to bucket
				hr = pDHI->Set_Block_Time_Correction(Head, Block, download, pStatus);
				if (FAILED(hr)) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Method Failed (pDHI->Set_Block_Time_Correction)", hr);
			}
		}
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Store_Energy_Peaks
// Purpose:		stores crystal energy peaks for a specific head to a file
// Arguments:	Send			-	long = flag whether or not to send values to head
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				CrystalPeaks	-	long = crystal energy peaks
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Store_Energy_Peaks(long Send, long Configuration, long Head, long Layer, long CrystalPeaks[], long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;
	long Block = 0;
	long CheckSum = 0;
	char OutStr[MAX_STR_LEN];
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Store_Energy_Peaks";

	unsigned char headname[MAX_STR_LEN];

	HRESULT hr = S_OK;

	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_ce.txt", m_RootPath, Configuration, Head, LayerStr[Layer], Head);

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "wt")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// write and close
	if (*pStatus == 0) {

		// loop until end of file
		for (i = 0 ; i < m_EmissionBlocks ; i++) {

			// build up the line for each block
			sprintf(OutStr, "%3d %s", i, StrLayer[Layer]);
			for (j = 0 ; j < 256 ; j++) sprintf(OutStr, "%s %3d", OutStr, CrystalPeaks[(i*256)+j]);

			// write out the line
			fprintf(fp, "%s\n", OutStr);
		}

		// close
		fclose(fp);
	}

	// send the values to the head if requested
	if ((*pStatus == 0) && (Send == 1)) {

		// filename on head
		sprintf((char *) headname, "%d_pk%s.txt", Configuration, StrLayer[Layer]);

		// download the file
		*pStatus = Local_Download(pErrEvtSup, pDHI, Head, filename, headname);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Store_Settings
// Purpose:		stores ASIC settings to file
// Arguments:	Send			-	long = flag whether or not to send values to head
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Settings		-	long = ASIC settings
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Store_Settings(long Send, long Configuration, long Head, long Settings[], long *pStatus)
{
	// local variables
	long i = 0;
	long j = 0;
	long Block = 0;
	char OutStr[MAX_STR_LEN];
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Store_Settings";

	FILE *fp;

	HRESULT hr = S_OK;

	// initialize status
	*pStatus = 0;

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\h%d_settings.txt", m_RootPath, Configuration, Head, Head);

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "wt")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// write and close
	if (*pStatus == 0) {

		// build and write the title string
		sprintf(OutStr, "# block");
		for (i = 0 ; i < m_NumSettings ; i++) {
			for (j = 0 ; j < MAX_SETTINGS ; j++) {
				if (SetIndex[j] == OutOrder[i]) sprintf(OutStr, "%s %s", OutStr, SetTitle[i]);
			}
		}
		fprintf(fp, "%s\n", OutStr);

		// write settings for all blocks
		for (i = 0 ; i < m_NumBlocks ; i++) {

			// block number
			sprintf(OutStr, "%6d", i);

			// add the settings
			for (j = 0 ; j < m_NumSettings ; j++) sprintf(OutStr, "%s%6d", OutStr, Settings[(i*m_NumSettings)+OutOrder[j]]);
			fprintf(fp, "%s\n", OutStr);
		}

		// close the file
		fclose(fp);
	}

	// send to head if requested
	if ((*pStatus == 0) && (Send == 1)) {
		hr = pDHI->Set_Analog_Settings(Configuration, Head, Settings, pStatus);
		if (FAILED(hr)) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Method Failed (pDHI->Set_Analog_Settings), HRESULT", hr);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Read_Master
// Purpose:		returns array containing requested master data from file system
// Arguments:	DataMode		-	long = Data Mode
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//	`				8 = Time Difference
//					9 = Coincidence Flood
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				*pSize			-	long = size of data in bytes
//				**Data			-	long = output data array
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Read_Master(long DataMode, long Configuration, long Head, long Layer, long *pSize, long **Data, long *pStatus)
{
	// local variables
	unsigned long ByteSize = 0;
	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Read_Master";
	struct _stat file_status;
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - data mode
	if ((DataMode < 0) || (DataMode > (NUM_DHI_MODES+1))) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Data Mode", DataMode);
		return S_FALSE;
	}

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// build file name
	sprintf(filename, "%sDefaults\\h%d_%d_%smaster.%s", m_RootPath, Head, Configuration, LayerID[Layer], ModeExt[DataMode]);

	// make sure file is there
	if (_stat(filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		return S_FALSE;
	}

	// Determine the Size
	switch (DataMode) {

	// position profile an crystal energy are same size
	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_CRYSTAL_ENERGY:
	case DHI_MODE_TIME:
		*pSize = 256 * 256 * m_NumBlocks;
		break;

	// tube energy
	case DHI_MODE_TUBE_ENERGY:
		*pSize = 256 * 4 * m_NumBlocks;
		break;

	// shape discrimination
	case DHI_MODE_SHAPE_DISCRIMINATION:
		*pSize = 256 * m_NumBlocks;
		break;

	// run mode and transmission mode are the same
	case DHI_MODE_RUN:
	case DHI_MODE_TRANSMISSION:
	case DATA_MODE_COINC_FLOOD:
		*pSize = 128 * 128;
		break;

	// SPECT
	case DHI_MODE_SPECT:
		*pSize = 256 * 128 * 128;
		break;

	// timing
	case DATA_MODE_TIMING:
		*pSize = m_TimingBins * 128 * 128;
		break;

	default:
		*pSize = 0;
		break;
	}

	// allocate the memory
	ByteSize = sizeof(long) * *pSize;
	if (ByteSize > 0) {
		*Data = (long *) ::CoTaskMemAlloc(ByteSize);
		if (*Data == NULL) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Allocating Memory, Requested Size", ByteSize);
	}

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "rb")) == NULL)  {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// read and close
	if (*pStatus == 0) {

		// read file
		if (*pSize != (long) fread((void *) *Data, sizeof(long), *pSize, fp)) {
			sprintf(Str, "Reading File (%s), Requested Elements", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, *pSize);
		}

		// close the file
		fclose(fp);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Write_Master
// Purpose:		writes the data out as the master data of that type for the head specified
// Arguments:	DataMode		-	long = Data Mode
//					0 = Position Profile
//					1 = Crystal Energy
//					2 = tube energy
//					3 = shape discrimination
//					4 = run
//					5 = transmission
//					6 = SPECT
//					7 = Crystal Time
//	`				8 = Time Difference
//					9 = Coincidence Flood
//				Configiuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head 
//				Layer			-	long = Crystal Layer
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				*pSize			-	long = size of data in bytes
//				**Data			-	long = input data array
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Write_Master(long DataMode, long Configuration, long Head, long Layer, long Size, long **Data, long *pStatus)
{
	// local variables
	long LocalSize = 0;
	char filename[MAX_STR_LEN];
	char headname[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Write_Master";
	FILE *fp;

	// initialize status
	*pStatus = 0;

	// argument check - data mode
	if ((DataMode < 0) || (DataMode > (NUM_DHI_MODES+1))) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Data Mode", DataMode);
		return S_FALSE;
	}

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// argument check - layer selection
	if ((Layer < 0) || (Layer >= NUM_LAYER_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Layer", Layer);
		return S_FALSE;
	}

	// Determine what the Size should be
	switch (DataMode) {

	// position profile an crystal energy are same size
	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_CRYSTAL_ENERGY:
	case DHI_MODE_TIME:
		LocalSize = 256 * 256 * m_NumBlocks;
		break;

	// tube energy
	case DHI_MODE_TUBE_ENERGY:
		LocalSize = 256 * 4 * m_NumBlocks;
		break;

	// shape discrimination
	case DHI_MODE_SHAPE_DISCRIMINATION:
		LocalSize = 256 * m_NumBlocks;
		break;

	// run mode and transmission mode are the same
	case DHI_MODE_RUN:
	case DHI_MODE_TRANSMISSION:
	case DATA_MODE_COINC_FLOOD:
		LocalSize = 128 * 128;
		break;

	// SPECT
	case DHI_MODE_SPECT:
		LocalSize = 256 * 128 * 128;
		break;

	// timing
	case DATA_MODE_TIMING:
		LocalSize = m_TimingBins * 128 * 128;
		break;

	default:
		LocalSize = 0;
		break;
	}

	// argument check - file size
	if (Size != LocalSize) {
		sprintf(Str, "Incorrect Number of Elements, Expected = %d Input", LocalSize);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, Size);
		return S_FALSE;
	}

	// build file names
	sprintf(filename, "%sDefaults\\h%d_%d_%smaster.%s", m_RootPath, Head, Configuration, LayerID[Layer], ModeExt[DataMode]);
	sprintf(headname, "%d_Master.%s", Configuration, ModeExt[DataMode]);

	// open the file
	if (*pStatus == 0) if ((fp = fopen(filename, "wb")) == NULL) {
		sprintf(Str, "Opening File (%s)", filename);
		*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
	}

	// write and close
	if (*pStatus == 0) {

		// read file
		if (Size != (long) fwrite((void *) *Data, sizeof(long), Size, fp)) {
			sprintf(Str, "Writing To File (%s)", filename);
			*pStatus = Add_Error(pErrEvtSup, Subroutine, Str, 0);
		}

		// close the file
		fclose(fp);

		// download file to head
		if (*pStatus == 0) *pStatus = Local_Download(pErrEvtSup, pDHI, Head, filename, (unsigned char *) headname);
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Init_Globals
// Purpose:		COM Server initialization
// Arguments:	none
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				29 Sep 03	T Gremillion	If no heads/buckets detectected, error
//											Added LYSO/LSO head type
//				05 Nov 03	T Gremillion	Added CPS_SETUP_DIR environment variable
//				19 Nov 03	T Gremillion	Different number of max acquisition counts for ring scanners
//											and added number timing bins for ring scanners
//				15 Apr 04	T Gremillion	use electronics phase for ring scanners
//				14 Jun 04	T Gremillion	increased the maximum counts per second for ring scanners
/////////////////////////////////////////////////////////////////////////////

long CDUMain::Init_Globals()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long NextOut = 0;

	char Subroutine[80] = "Init_Globals";

	HRESULT hr = S_OK;

	// instantiate a Gantry Object Model object
	CGantryModel model;
	
	// is this a head test set?
	m_HeadTestSet = 0;
	if (getenv("HEAD_TEST_SET") != NULL) if (strcmp(getenv("HEAD_TEST_SET"), "1") == 0) m_HeadTestSet = 1;

#ifdef ECAT_SCANNER

	// local variables
	char Str[MAX_STR_LEN];

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
	if (FAILED(hr)) return Add_Error(pErrEvtSup, Subroutine, "Failed CoInitializeSecurity, HRESULT", hr);
#else
	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport(false, true);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSup->InitComplete(hr) == false) {
		delete pErrEvtSup;
		pErrEvtSup = NULL;
	}
#endif

	// set gantry model to model number held in register
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) return 1;

	// number of blocks per bucket/head
	m_NumBlocks = model.blocks();

	// initialize head arrays
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_HeadPresent[i] = 0;
		m_PointSource[i] = 0;
		m_NumLayers[i] = 1;
		m_MasterLayer[i] = LAYER_ALL;
		m_HeadType[i] = HEAD_TYPE_LSO_ONLY;
	}

	// initialize index settings
	for (i = 0 ; i < MAX_SETTINGS ; i++) SetIndex[i] = -1;

// ECAT (Ring) Scanners
#ifdef ECAT_SCANNER

	// set the electronics phase and scanner type
	m_Phase = model.electronicsPhase();
	m_ScannerType = SCANNER_RING;

	// only works for phase II electronics
	if (m_Phase == 1) {

		// return bad status
		sprintf(Str, "Ring Scanner Electronics Phase One Not Supported", m_Phase);
		return Add_Error(pErrEvtSup, Subroutine, Str, model.modelNumber());
	}

	// number of emission blocks & crystals in each dimension
	m_XBlocks = model.transBlocks();
	m_YBlocks = model.axialBlocks();
	m_XCrystals = model.angularCrystals();
	m_YCrystals = model.axialCrystals();

	// turn on the heads in sequence
	m_NumHeads = model.buckets();
	for (i = 0 ; i < m_NumHeads ; i++) m_HeadPresent[i] = 1;

	// one configuration - 511 KeV
	m_NumConfigs = 1;

	// ASIC Settings
	SetIndex[PMTA] = 0;
	SetIndex[PMTB] = 1;
	SetIndex[PMTC] = 2;
	SetIndex[PMTD] = 3;
	SetIndex[CFD] = 4;
	SetIndex[CFD_SETUP] = 5;
	SetIndex[CFD_ENERGY_0] = 6;
	SetIndex[CFD_ENERGY_1] = 7;
	SetIndex[TDC_GAIN] = 8;
	SetIndex[TDC_OFFSET] = 9;
	SetIndex[TDC_SETUP] = 10;
	SetIndex[PEAK_STDEV] = 11;
	SetIndex[PHOTOPEAK] = 12;
	SetIndex[SETUP_TEMP] = 13;
	m_NumSettings = 14;	

	// get server
	if (getenv("CPS_SETUP_DIR") == NULL) {
		sprintf(m_RootPath, "C:\\CPS\\Setup\\");
	} else {
		sprintf(m_RootPath, "%s", getenv("CPS_SETUP_DIR"));
	}

	// timing bins
	m_TimingBins = 32;

	// need in gantry model
	m_AcquisitionLimit = 2 * 1024 * 1024;

// Panel detectors (HRRT, PETSPECT, P39)
#else
	pHead Head;

	// set scanner type from gantry model
	switch (model.modelNumber()) {

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

	// P39 Phase IIA Electronics family (2391 - 2396)
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
		return Add_Error(pErrEvtSup, Subroutine, "Unknown Scanner, Model", model.modelNumber());
		break;
	}

	// number of emission blocks & crystals in each dimension
	if (model.isHeadRotated()) {
		m_Rotate = 1;
		m_XBlocks = model.axialBlocks();
		m_YBlocks = model.transBlocks();
		m_XCrystals = model.axialCrystals();
		m_YCrystals = model.angularCrystals();
	} else {
		m_Rotate = 0;
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
		m_MasterLayer[Head[i].address] = Head[i].GainLayer;

		// indicate if transmission sources are hooked up to this head
		if (Head[i].pointSrcData) m_PointSource[Head[i].address] = 1;

		// number of layers
		m_HeadType[Head[i].address] = Head[i].type;
		switch (m_HeadType[Head[i].address]) {

		case HEAD_TYPE_LSO_ONLY:
			m_NumLayers[Head[i].address] = 1;
			break;

		case HEAD_TYPE_LSO_LSO:
			m_NumLayers[Head[i].address] = 2;
			break;

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

	// timing bins
	m_TimingBins = model.timingBins();

	// configuration values
	m_NumConfigs = model.nConfig();

	// analog card settings
	m_NumSettings = model.nAnalogSettings();
	SetIndex[PMTA] = model.pmtaSetting();
	SetIndex[PMTB] = model.pmtbSetting();
	SetIndex[PMTC] = model.pmtcSetting();
	SetIndex[PMTD] = model.pmtdSetting();
	SetIndex[CFD] = model.cfdSetting();
	SetIndex[CFD_DELAY] = model.cfdDelaySetting();
	SetIndex[X_OFFSET] = model.xOffsetSetting();
	SetIndex[Y_OFFSET] = model.yOffsetSetting();
	SetIndex[E_OFFSET] = model.energyOffsetSetting();
	SetIndex[DHI_MODE] = model.dhiModeSetting();
	SetIndex[SETUP_ENG] = model.setupEnergySetting();
	SetIndex[SHAPE] = model.shapeThresholdSetting();
	SetIndex[SLOW_LOW] = model.slowLowSetting();
	SetIndex[SLOW_HIGH] = model.slowHighSetting();
	SetIndex[FAST_LOW] = model.fastLowSetting();
	SetIndex[FAST_HIGH] = model.fastHighSetting();
	SetIndex[CFD_SETUP] = model.cfdSetupSetting();
	SetIndex[TDC_GAIN] = model.tdcGainSetting();
	SetIndex[TDC_OFFSET] = model.tdcOffsetSetting();
	SetIndex[TDC_SETUP] = model.tdcSetupSetting();

	// strings
	sprintf(m_RootPath, model.SetupDir());

	// need in gantry model
	m_AcquisitionLimit = 2 * 1024 * 1024;
#endif

	// output order of settings
	if (SetIndex[PMTA] != -1) OutOrder[NextOut++] = SetIndex[PMTA];
	if (SetIndex[PMTB] != -1) OutOrder[NextOut++] = SetIndex[PMTB];
	if (SetIndex[PMTC] != -1) OutOrder[NextOut++] = SetIndex[PMTC];
	if (SetIndex[PMTD] != -1) OutOrder[NextOut++] = SetIndex[PMTD];
	if (SetIndex[CFD] != -1) OutOrder[NextOut++] = SetIndex[CFD];
	if (SetIndex[CFD_DELAY] != -1) OutOrder[NextOut++] = SetIndex[CFD_DELAY];
	if (SetIndex[CFD_SETUP] != -1) OutOrder[NextOut++] = SetIndex[CFD_SETUP];
	if (SetIndex[CFD_ENERGY_0] != -1) OutOrder[NextOut++] = SetIndex[CFD_ENERGY_0];
	if (SetIndex[CFD_ENERGY_1] != -1) OutOrder[NextOut++] = SetIndex[CFD_ENERGY_1];
	if (SetIndex[X_OFFSET] != -1) OutOrder[NextOut++] = SetIndex[X_OFFSET];
	if (SetIndex[Y_OFFSET] != -1) OutOrder[NextOut++] = SetIndex[Y_OFFSET];
	if (SetIndex[E_OFFSET] != -1) OutOrder[NextOut++] = SetIndex[E_OFFSET];
	if (SetIndex[SHAPE] != -1) OutOrder[NextOut++] = SetIndex[SHAPE];
	if (SetIndex[DHI_MODE] != -1) OutOrder[NextOut++] = SetIndex[DHI_MODE];
	if (SetIndex[CFD_SETUP] != -1) OutOrder[NextOut++] = SetIndex[CFD_SETUP];
	if (SetIndex[TDC_GAIN] != -1) OutOrder[NextOut++] = SetIndex[TDC_GAIN];
	if (SetIndex[TDC_OFFSET] != -1) OutOrder[NextOut++] = SetIndex[TDC_OFFSET];
	if (SetIndex[TDC_SETUP] != -1) OutOrder[NextOut++] = SetIndex[TDC_SETUP];
	if (SetIndex[SETUP_ENG] != -1) OutOrder[NextOut++] = SetIndex[SETUP_ENG];
	if (SetIndex[SLOW_LOW] != -1) OutOrder[NextOut++] = SetIndex[SLOW_LOW];
	if (SetIndex[SLOW_HIGH] != -1) OutOrder[NextOut++] = SetIndex[SLOW_HIGH];
	if (SetIndex[FAST_LOW] != -1) OutOrder[NextOut++] = SetIndex[FAST_LOW];
	if (SetIndex[FAST_HIGH] != -1) OutOrder[NextOut++] = SetIndex[FAST_HIGH];
	if (SetIndex[PEAK_STDEV] != -1) OutOrder[NextOut++] = SetIndex[PEAK_STDEV];
	if (SetIndex[PHOTOPEAK] != -1) OutOrder[NextOut++] = SetIndex[PHOTOPEAK];
	if (SetIndex[SETUP_TEMP] != -1) OutOrder[NextOut++] = SetIndex[SETUP_TEMP];

	// initialize global values
	m_AcquireFlag = 0;
	m_AcquirePercent = 100;
	m_ArchiveFlag = 0;
	m_ArchivePercent = 100;
	sprintf(m_ArchiveStage, "Archive Idle");
	m_EmissionBlocks = m_XBlocks * m_YBlocks;
	m_ArchiveMode = -1;
	m_NumKEvents = 0;
	m_BadAddress = 0;
	m_SyncProb = 0;

	// clear handles
	hAcquireThread = NULL;
	hArchiveThread = NULL;

	// Create an instance of DHI Server.
	pDHI = Get_DAL_Ptr(pErrEvtSup);
	if (pDHI == NULL) Status = Add_Error(pErrEvtSup, Subroutine, "Failed To Get DAL Pointer", 0);

	// Create an instance of Acquisition Server.
	if (Status == 0) {
		pACQ = Get_ACQ_Ptr(pErrEvtSup);
		if (pACQ == NULL) {
			pDHI->Release();
			Status = Add_Error(pErrEvtSup, Subroutine, "Failed To Get ACQ Pointer", 0);
		}
	}

	// release acquisition server - reconnect as needed
	if (Status == 0) {
		pACQ->Release();
		pACQ = NULL;
	}

	// return status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Singles_Thread
// Purpose:		thread that does the work to acquire singles data.  It uses the
//				DAL to initialize the scanner, starts the acquisition, monitors it
//				and then writes the data to files when completed.
// Arguments:	dummy	- void
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				30 Sep 03	T Gremillion	data mode needs to be DHI_MODE_RUN, all layers 
//											when restoring after acquisition for ring scanners
//				22 Jan 04	T Gremillion	for ring scanners, always restore to 
//											coincidence mode.
//				15 Apr 04	T Gremillion	correct release of pointers on failure
//				19 Oct 04	T Gremillion	handle split run mode data for HRRT
/////////////////////////////////////////////////////////////////////////////

void Acquire_Singles_Thread(void *dummy)
{
	// local variables
	long i = 0;
	long Head = 0;
	long NumHeads = 0;
	long NumPasses = 0;
	long Next = 0;
	long Current = 0;
	long CurrPass = 0;
	long LastHead = -1;
	long Status = 0;
	long Percent = 0;
	long DataSize = 0;
	long Transmission = 0;
	long *buffer;

	unsigned long NumKEvents = 0;
	unsigned long BadAddress = 0;
	unsigned long SyncProb = 0;

	long Statistics[19];
	long HistSelect[MAX_HEADS];
	long PassSelect[MAX_HEADS];
	long PassHeadSelect[MAX_HEADS];
	long PassSourceSelect[MAX_HEADS];
	long HeadPass[MAX_HEADS];

	double PassPercent = 0.0;

	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Acquire_Singles_Thread";

	HRESULT hr = S_OK;
	
	FILE *fp;

#ifdef ECAT_SCANNER
	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}
#endif

	// clear heads to be histogramed
	for (i = 0 ; i < MAX_HEADS ; i++) {
		HistSelect[i] = 0;
		HeadPass[i] = -1;
	}

	// initialize percent complete to 0
	m_AcquirePercent = 0;

	// initialize COM pointers to NULL
	pAcqThreadDHI = NULL;
	pAcqThreadACQ = NULL;

	// Create an instance of DHI Server.
	if (Status == 0) {

		// Create an instance of DHI Server.
		pAcqThreadDHI = Get_DAL_Ptr(pErrEvtSupThread);
		if (pAcqThreadDHI == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed To Get DAL Pointer", 0);
	}

	// put coincidence processor in passthru mode - has the potential to take the longest
	if ((Status == 0) && (m_AcqAbort == 0)) {
		hr = pAcqThreadDHI->PassThru_Mode(m_AcquireSelect, m_SourceSelect, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->PassThru_Mode)", hr);
	}

	// loop through the heads putting them in the desired mode
	for (Head = 0 ; (Head < m_NumHeads) && (m_AcqAbort == 0) && (Status == 0) ; Head++) if (m_HeadPresent[Head] == 1) {

		// if the head was selected
		if ((m_AcquireSelect[Head] == 1) || (m_SourceSelect[Head] == 1)) {

			// note if it is using transmission sources
			if (m_SourceSelect[Head] == 1) Transmission = 1;

			// select the head for histograming
			HistSelect[Head] = 1;

			// put it in mode with all blocks on (block = -1)
			hr = pAcqThreadDHI->Set_Head_Mode(m_DataMode, m_Configuration, Head, m_AcquireLayer[Head], ALL_BLOCKS, m_lld, m_uld, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Set_Head_Mode)", hr);

			// keep track of the number of heads
			NumHeads++;

		// otherwise, we want the head on, but no blocks (not for ring scanners)
		} else if (SCANNER_RING != m_ScannerType) {

			// put it in mode with all blocks off (block = -2)
			hr = pAcqThreadDHI->Set_Head_Mode(m_DataMode, m_Configuration, Head, LAYER_ALL, NO_BLOCKS, m_lld, m_uld, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Set_Head_Mode)", hr);
		}
	}

	// wait for completion of mode command and switch to passthru mode (Progress head = -2)
	Percent = 0;
	while ((Percent < 100) && (Status == 0)) {
		hr = pAcqThreadDHI->Progress(WAIT_EVERYTHING, &Percent, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Progress)", hr);
		if (Percent < 100) Sleep(1000);
	}

	// start the data flowing
	if ((Status == 0) && (m_AcqAbort == 0) && (SCANNER_RING != m_ScannerType)) {
		hr = pAcqThreadDHI->Start_Scan(&Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Start_Scan)", hr);
	}

	// Create an instance of Acquisition Server now that it is needed
	if (Status == 0) {
		pAcqThreadACQ = Get_ACQ_Ptr(pErrEvtSupThread);
		if (pAcqThreadACQ == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get ACQ Pointer",0);
	}

	// number of acquisition passes
	if (Status == 0) {

		// with transission sources, each head gets its own acquisition
		if (Transmission == 1) {
			NumPasses = NumHeads;
			for (Head = 0, Next = 0 ; Head < MAX_HEADS ; Head++) {
				if (HistSelect[Head] == 1) HeadPass[Head] = Next++;
			}

		// otherwise, base it on the statistics from each head
		} else {

			// wait 2 seconds to allow CP to generate statistics
			Sleep(2000);

			// retrieve the acquisition statistics
			hr = pAcqThreadDHI->Get_Statistics(Statistics, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Get_Statistics)", hr);

			// Determine number of and assign passes
			for (Head = 0, NumPasses = 0, Current = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

				// if the head was selected
				if (HistSelect[Head] == 1) {

					// if it belongs in current pass
					if ((Current == 0) || ((Current+Statistics[Head+3]) < m_AcquisitionLimit)) {
						HeadPass[Head] = NumPasses;
						Current += Statistics[Head+3];

					// increment pass
					} else {
						HeadPass[Head] = ++NumPasses;
						Current = Statistics[Head+3];
					}
				}
			}

			// Number of passes is one more than the last pass number
			NumPasses++;
		}
	}

	// percent complete per pass
	PassPercent = 100.0 / (double) NumPasses;

	// clear global statistics
	m_NumKEvents = 0;
	m_BadAddress = 0;
	m_SyncProb = 0;
	for (Head = 0 ; Head < m_NumHeads ; Head++) m_AcquireStatus[Head] = 0;

	// loop through the number of acquisition passes
	for (CurrPass = 0 ; (CurrPass < NumPasses) && (Status == 0)&& (m_AcqAbort == 0) ; CurrPass++) {

		// current pass selection
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

			// clear selection
			PassSelect[Head] = 0;
			PassHeadSelect[Head] = 0;
			PassSourceSelect[Head] = 0;

			// select
			if (HeadPass[Head] == CurrPass) {
				PassSelect[Head] = 1;
				PassHeadSelect[Head] = m_AcquireSelect[Head];
				PassSourceSelect[Head] = m_SourceSelect[Head];

				// for the HRRT, the head will need to be put in mode again
				if (SCANNER_HRRT == m_ScannerType) {

					// put it in mode with all blocks on (block = -1)
					hr = pAcqThreadDHI->Set_Head_Mode(m_DataMode, m_Configuration, Head, m_AcquireLayer[Head], ALL_BLOCKS, m_lld, m_uld, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Set_Head_Mode)", hr);
				}

			// for the HRRT heads not currently being acquired need to be turned off
			} else if ((SCANNER_HRRT == m_ScannerType) && (m_HeadPresent[Head] != 0)) {

				// put it in mode with all blocks off (block = -2)
				hr = pAcqThreadDHI->Set_Head_Mode(m_DataMode, m_Configuration, Head, m_AcquireLayer[Head], NO_BLOCKS, m_lld, m_uld, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Set_Head_Mode)", hr);
			}
		}

		// issue new passthru command for current head(s)
		if (Status == 0) {
			hr = pAcqThreadDHI->PassThru_Mode(PassHeadSelect, PassSourceSelect, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->PassThru_Mode)", hr);
		}

		// wait for completion of mode command and switch to passthru mode (Progress head = -2)
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pAcqThreadDHI->Progress(WAIT_EVERYTHING, &Percent, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Progress)", hr);
			if (Percent < 100) Sleep(1000);
		}

		// acquire data
		if ((Status == 0) && (m_AcqAbort == 0)) {

			// start acquisition
			hr = pAcqThreadACQ->Acquire_Singles(m_DataMode, PassSelect, m_AcqType, m_Amount, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Acquire_Singles)", hr);

			// set flag to show data is being acquired by the acquisition server
			if (Status == 0) {
				m_Acquiring = 1;
			} else {

				// set bad status if failed at this point
				for (Head = 0 ; Head < m_NumHeads ; Head++) m_AcquireStatus[Head] = Status;

				// clear acquisition flag
				m_AcquirePercent = 100;
				m_AcquireFlag = 0;

				// release servers
				if (pAcqThreadDHI != NULL) pAcqThreadDHI->Release();
				if (pAcqThreadACQ != NULL) pAcqThreadACQ->Release();
#ifndef ECAT_SCANNER
				if (pErrEvtSupThread != NULL) delete pErrEvtSupThread;
#endif

				// exit
				return;
			}
		}

		// poll acquisition for completion percentage
		Percent = 0;
		while ((Percent < 100) && (Status == 0) && (m_Acquiring == 1)) {
			hr = pAcqThreadACQ->Progress(&Percent, &Status);
			m_AcquirePercent = (long)((CurrPass * PassPercent) + ((Percent / 103.0) * PassPercent));
			if (m_AcquirePercent < 1) m_AcquirePercent = 1;
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Progress)", hr);
			if ((Status == 0) && (Percent < 100)) Sleep(1000);
		}

		// retrieve statistics
		if (Status == 0) {
			hr = pAcqThreadACQ->Statistics(&NumKEvents, &BadAddress, &SyncProb);
			if (FAILED(hr)) {
				Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Statistics)", hr);
			} else {
				m_NumKEvents += NumKEvents;
				m_BadAddress += BadAddress;
				m_SyncProb += SyncProb;
			}
		}

		// write files
		if ((Status == 0) && (m_Acquiring == 1)) {
			for (Head = 0 ; Head < m_NumHeads ; Head++) {
				if ((PassSelect[Head] == 1) && (m_AcquireStatus[Head] == 0)) {

					// fetch finished data, if there is an error, continue to next head
					if (Status == 0) {
						hr = pAcqThreadACQ->Retrieve_Singles(Head, &DataSize, &buffer, &Status);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Retrieve_Singles)", hr);
						if (Status != 0) {
							m_AcquireStatus[Head] = Status;
							continue;
						}
					}

					// for the HRRT in run mode, the data size is 1/3
					if ((SCANNER_HRRT == m_ScannerType) && (DHI_MODE_RUN == m_DataMode)) DataSize /= 3;

					// build file name
					sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_data.%s", 
							m_RootPath, m_Configuration, Head, LayerStr[m_AcquireLayer[Head]], Head, ModeExt[m_DataMode]);

					// open file
					if ((fp = fopen(filename, "wb")) != NULL) {

						// write file
						if ((long) fwrite(buffer, sizeof(long), DataSize, fp) != DataSize) {
							sprintf(Str, "Writing To File (%s)", filename);
							m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
						}

						// close file
						fclose(fp);

					// otherwise, error for this head
					} else {
						sprintf(Str, "Opening File (%s)", filename);
						m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
					}

					if ((SCANNER_HRRT == m_ScannerType) && (DHI_MODE_RUN == m_DataMode) && (LAYER_ALL == m_AcquireLayer[Head])) {

						// build file name
						sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\Fast\\h%d_data.%s", 
								m_RootPath, m_Configuration, Head, Head, ModeExt[m_DataMode]);

						// open file
						if ((fp = fopen(filename, "wb")) != NULL) {

							// write file
							if ((long) fwrite(&buffer[DataSize], sizeof(long), DataSize, fp) != DataSize) {
								sprintf(Str, "Writing To File (%s)", filename);
								m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
							}

							// close file
							fclose(fp);

						// otherwise, error for this head
						} else {
							sprintf(Str, "Opening File (%s)", filename);
							m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
						}

						// build file name
						sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\Slow\\h%d_data.%s", 
								m_RootPath, m_Configuration, Head, Head, ModeExt[m_DataMode]);

						// open file
						if ((fp = fopen(filename, "wb")) != NULL) {

							// write file
							if ((long) fwrite(&buffer[2*DataSize], sizeof(long), DataSize, fp) != DataSize) {
								sprintf(Str, "Writing To File (%s)", filename);
								m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
							}

							// close file
							fclose(fp);

						// otherwise, error for this head
						} else {
							sprintf(Str, "Opening File (%s)", filename);
							m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
						}
					}

					// release the memory for that head
					::CoTaskMemFree((void *) buffer);
				}
			}
		}

		// bad status
		if (Status != 0) {
			for (Head = 0 ; Head < m_NumHeads ; Head++) if (PassSelect[Head] == 1) m_AcquireStatus[Head] = Status;
		}

		// clear acquisition flag
		m_AcquireFlag = 0;
	}

	// for ring scanners, the scanner is returned to normal after each acquisition
	if (SCANNER_RING == m_ScannerType) {

		// if the status is bad, wait 30 seconds for any previous commands to clear
		if (Status != 0) Sleep(30*1000);

		// put coincidence processor in coincidence mode (we always want to attempt this)
		hr = pAcqThreadDHI->Coincidence_Mode(0, 0, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Coincidence_Mode)", hr);

		// loop through the heads putting them in the desired mode
		for (Head = 0 ; Head < m_NumHeads ; Head++) if (m_HeadPresent[Head] == 1) {

			// put it in mode with all blocks on (block = -1)
			hr = pAcqThreadDHI->Set_Head_Mode(DHI_MODE_RUN, 0, Head, LAYER_ALL, ALL_BLOCKS, m_lld, m_uld, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Set_Head_Mode)", hr);
		}

		// wait for completion of mode command and switch to passthru mode (Progress head = -2)
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pAcqThreadDHI->Progress(WAIT_EVERYTHING, &Percent, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Progress)", hr);
			if (Percent < 100) Sleep(1000);
		}
	}

	// clear acquisition flag
	m_AcquirePercent = 100;
	m_AcquireFlag = 0;

	// release servers
	if (pAcqThreadDHI != NULL) pAcqThreadDHI->Release();
	if (pAcqThreadACQ != NULL) pAcqThreadACQ->Release();
#ifndef ECAT_SCANNER
	if (pErrEvtSupThread != NULL) delete pErrEvtSupThread;
#endif

	// finished
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Coinc_Thread
// Purpose:		thread that does the work to acquire coincidence data.  It uses the
//				DAL to initialize the scanner, starts the acquisition, monitors it
//				and then writes the data to files when completed.
// Arguments:	dummy	- void
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				01 Oct 03	T Gremillion	only heads selected produce data
//				15 Apr 04	T Gremillion	Added support for ring scanners
/////////////////////////////////////////////////////////////////////////////

void Acquire_Coinc_Thread(void *dummy)
{
	// local variables
	long i = 0;
	long Head = 0;
	long NumHeads = 0;
	long LastHead = -1;
	long Status = 0;
	long Percent = 0;
	long ScanStarted = 0;
	long DataSize = 0;
	long StillNeeded = 0;
	long MostLayer = LAYER_ALL;
	long DataMode = DHI_MODE_RUN;

	long *buffer;

	long LayerCount[NUM_LAYER_TYPES];

	long P39_Head_Pair[MAX_HEADS] = {	0x000E, // head 0 is part of module pairs 1, 2, 3
										0x0070, // head 1 is part of module pairs 4, 5, 6
										0x0582, // head 2 is part of module pairs 1, 7, 8, 10
										0x0A14,	// head 3 is part of module pairs 2, 4, 9, 11
										0x10A8, // head 4 is part of module pairs 3, 5, 7, 12
										0x0340, // head 5 is part of module pairs 6, 8, 9
										0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	// max of 6 heads on P39 (including transmision sources)


	char filename[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Acquire_Coinc_Thread";

	HRESULT hr = S_OK;
	
	FILE *fp;

#ifdef ECAT_SCANNER
	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}
#endif

	// initialize percent complete to 0
	m_AcquirePercent = 0;

	// if transmission, change data mode
	if (m_Transmission != 0) DataMode = DHI_MODE_TRANSMISSION;

	// Create an instance of DHI Server.
	if (Status == 0) {
		pAcqThreadDHI = Get_DAL_Ptr(pErrEvtSupThread);
		if (pAcqThreadDHI == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed To Get DAL Pointer", 0);
	}

	// determine the dominate layer
	for (i = 0 ; i < NUM_LAYER_TYPES ; i++) LayerCount[i] = 0;
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if (m_AcquireSelect[i] != 0) {
			LayerCount[m_AcquireLayer[i]]++;
		}
	}
	for (i = 0 ;  i < NUM_LAYER_TYPES ; i++) {
		if (LayerCount[i] > LayerCount[MostLayer]) MostLayer = i;
	}

	// ring scanners should already be set for coincidence scans
	if (SCANNER_RING != m_ScannerType) {

		// set for scan
		hr = pAcqThreadDHI->Initialize_Scan(m_Transmission, m_ModulePair, m_Configuration, MostLayer, m_lld, m_uld, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Initialize_Scan)", hr);

		// wait for completion of mode command and switch to passthru mode (Progress head = -2)
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pAcqThreadDHI->Progress(WAIT_EVERYTHING, &Percent, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Progress)", hr);
			if (Percent < 100) Sleep(1000);
		}
	}

	// switch the coincidence processor to timing mode if acquiring timing data
	if ((Status == 0) && (m_DataMode == DATA_MODE_TIMING)) {
		hr = pAcqThreadDHI->Time_Mode(m_Transmission, m_ModulePair, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Time_Mode)", hr);
	}

	// loop through the heads
	for (Head = 0 ; (Head < m_NumHeads) && (Status == 0) ; Head++) {

		// if the head is selected, but layer is wrong, change it
		if ((m_AcquireSelect[Head] == 1) && (m_AcquireLayer[Head] != MostLayer)) {
			hr = pAcqThreadDHI->Set_Head_Mode(DataMode, m_Configuration, Head, m_AcquireLayer[Head], ALL_BLOCKS, m_lld, m_uld, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Set_Head_Mode)", hr);
		}
	}

	// wait for completion of mode command and switch to passthru mode (Progress head = -2)
	Percent = 0;
	while ((Percent < 100) && (Status == 0)) {
		hr = pAcqThreadDHI->Progress(WAIT_EVERYTHING, &Percent, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Progress)", hr);
		if (Percent < 100) Sleep(1000);
	}

	// start the data flowing
	if ((SCANNER_RING != m_ScannerType) && (Status == 0)) {
		hr = pAcqThreadDHI->Start_Scan(&Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Start_Scan)", hr);
		if (Status == 0) ScanStarted = 1;
	}

	// Create an instance of Acquisition Server now that its needed
	if (Status == 0) {
		pAcqThreadACQ = Get_ACQ_Ptr(pErrEvtSupThread);
		if (pAcqThreadACQ == NULL) {

			// release the dhi server
			pAcqThreadDHI->Release();

			// set error status
			Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get ACQ Pointer",0);
		}
	}

	// clear global statistics
	m_NumKEvents = 0;
	m_BadAddress = 0;
	m_SyncProb = 0;

	// acquire data
	if (Status == 0) {
		hr = pAcqThreadACQ->Acquire_Singles(m_DataMode, m_AcquireSelect, m_AcqType, m_Amount, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Acquire_Singles)", hr);

		// set flag to show data is being acquired by the acquisition server
		if (Status == 0) {
			m_Acquiring = 1;

		} else {

			// immediate return if failed at this point
			for (Head = 0 ; Head < m_NumHeads ; Head++) m_AcquireStatus[Head] = Status;
			return;
		}
	}

	// poll acquisition for completion percentage
	Percent = 0;
	while ((Percent < 100) && (Status == 0)) {
		hr = pAcqThreadACQ->Progress(&Percent, &Status);
		m_AcquirePercent = (Percent * 97) / 100;
		if (m_AcquirePercent < 1) m_AcquirePercent = 1;
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Progress)", hr);
		if ((Status == 0) && (Percent < 100)) Sleep(1000);
	}

	// halt the scan (retract the point sources)
	if ((SCANNER_RING != m_ScannerType) && (ScanStarted == 1)) {
		hr =  pAcqThreadDHI->Stop_Scan(&Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Stop_Scan)", hr);
	}

	// retrieve statistics
	if (Status == 0) {
		hr = pAcqThreadACQ->Statistics(&m_NumKEvents, &m_BadAddress, &m_SyncProb);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Statistics)", hr);
	}

	// write files
	if (Status == 0) {
		for (Head = 0 ; Head < m_NumHeads ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] == 0)) {

				// fetch finished data, if there is an error, continue to next head
				if (Status == 0) {
					hr = pAcqThreadACQ->Retrieve_Singles(Head, &DataSize, &buffer, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadACQ->Retrieve_Singles)", hr);
					if (Status != 0) {
						m_AcquireStatus[Head] = Status;
						continue;
					}
				}

				// build file name
				sprintf(filename, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_data.time", 
						m_RootPath, m_Configuration, Head, LayerStr[m_AcquireLayer[Head]], Head);

				// open file
				if ((fp = fopen(filename, "wb")) != NULL) {

					// write file
					if ((long) fwrite(buffer, sizeof(long), DataSize, fp) != DataSize) {
						sprintf(Str, "Writing To File (%s)", filename);
						m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
					}

					// close file
					fclose(fp);

				// otherwise, error for this head
				} else {
					sprintf(Str, "Opening File (%s)", filename);
					m_AcquireStatus[Head] = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
				}

				// release the memory for that head
				::CoTaskMemFree((void *) buffer);
			}
		}
	}

	// bad status
	if (Status != 0) {
		for (Head = 0 ; Head < m_NumHeads ; Head++) if (m_AcquireSelect[Head] == 1) m_AcquireStatus[Head] = Status;
	}

	// switch the coincidence processor back to normal coincidence mode (ring scanners only)
	if ((SCANNER_RING == m_ScannerType) && (m_DataMode == DATA_MODE_TIMING)) {
		hr = pAcqThreadDHI->Coincidence_Mode(m_Transmission, m_ModulePair, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pAcqThreadDHI->Coincidence_Mode)", hr);
	}

	// clear acquisition flag
	m_AcquirePercent = 100;
	m_AcquireFlag = 0;

	// release servers
	if (NULL != pAcqThreadDHI) pAcqThreadDHI->Release();
	if (NULL != pAcqThreadACQ) pAcqThreadACQ->Release();
#ifndef ECAT_SCANNER
	if (pErrEvtSupThread != NULL) delete pErrEvtSupThread;
#endif

	// finished
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Archive_Load_Thread
// Purpose:		thread that does the work to load a setup archive.  It retrieves
//				the data from files, copies it to current file locations and uses
//				the DAL to send it to the head(s)
// Arguments:	dummy	- void
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	bug fixes - allocate memory, correct order of read code
/////////////////////////////////////////////////////////////////////////////

void Archive_Load_Thread(void *dummy)
{
	// local variables
	long i = 0;
	long Config = 0;
	long Status = 0;
	long NumHeads = 0;
	long FileSize = 0;
	long ServerAttached = 0;

	long HeadIndex[MAX_HEADS];

	char WorkDir[MAX_STR_LEN];
	char ArchiveDir[MAX_STR_LEN];
	char ArchiveFile[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Archive_Load_Thread";

	unsigned char *filebuf = NULL;

	FILE *fp;

	struct _stat file_status;

	IAbstract_DHI *pDHIThread;

	HRESULT hr = S_OK;

#ifdef ECAT_SCANNER
	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}
#endif

	// set mode
	m_ArchiveMode = ARCHIVE_LOAD;

	// chage the working directory to archive directory
	sprintf(WorkDir, "%s\\Current\\Temp", m_RootPath);
	_chdir(WorkDir);

	// get archive directory
	sprintf(ArchiveDir, "%sArchive\\%s", m_RootPath, m_Archive);

	// Configuration Number
	Config = atol(&m_Archive[strlen(m_Archive)-1]);

	// associated directory
	for (i = strlen(ArchiveDir)-1 ; (i > 0) && (ArchiveDir[i] != 46) ; i--) ArchiveDir[i] = 0;
	ArchiveDir[i] = 0;

	// determine heads
	for (i = 0 ; i < MAX_HEADS ; i++) {
		HeadIndex[i] = -1;
		if (m_ArchiveSelect[i] == 1) {
			HeadIndex[NumHeads++] = i;
		}
	}

	// attach to com server
	if (Status == 0) {
		pDHIThread = Get_DAL_Ptr(pErrEvtSupThread);
		if (pDHIThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed To Get DAL Pointer", 0);
	}
	if (Status == 0) ServerAttached = 1;

	// backup files on head
	if (Status == 0) Status = Local_Backup(pErrEvtSupThread, pDHIThread, m_ArchiveSelect);

	// loop through the heads
	for (i = 0 ; i < NumHeads ; i++) {

		// update percentage
		m_ArchivePercent = (i * 100) / NumHeads;

		// if an error has been encountered, skip loop
		if (Status != 0) continue;

		// stage information
		sprintf(m_ArchiveStage, "Head %d: Downloading Files", HeadIndex[i]);

		// input file
		sprintf(ArchiveFile, "%s\\Head%d_%d.har", ArchiveDir, HeadIndex[i], Config);

		// make sure file is there and get information on it
		if (_stat(ArchiveFile, &file_status) == -1) {
			sprintf(Str, "File Not Present (%s)", ArchiveFile);
			Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
		}

		// get file size
		FileSize = file_status.st_size;

		// allocate memory
		filebuf = (unsigned char*) ::CoTaskMemAlloc((unsigned long) FileSize);
		if (NULL == filebuf) Status = Add_Error(pErrEvtSupThread, Subroutine, "Could Not Allocate Memory", 0);

		// open file
		if (Status == 0) if ((fp = fopen(ArchiveFile, "rb")) == NULL) {
			sprintf(Str, "Opening File (%s)", ArchiveFile);
			Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
		}

		// file open
		if (Status == 0) {

			// read
			if (FileSize != (long) fread((void *) filebuf, sizeof(unsigned char), FileSize, fp)) {
				sprintf(Str, "Reading File (%s)", ArchiveFile);
				Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
			}

			// close regardless
			fclose(fp);
		}

		// use the local load routine
		if (Status == 0) Status = Local_Load(pErrEvtSupThread, pDHIThread, Config, HeadIndex[i], FileSize, &filebuf);

		// set head status
		m_ArchiveStatus[HeadIndex[i]] = Status;
	}

	// if there was a problem restore all the heads
	if (Status != 0) Status = Local_Restore(pErrEvtSupThread, pDHIThread, m_ArchiveSelect);

	// release server
#ifndef ECAT_SCANNER
	if (pErrEvtSupThread != NULL) delete pErrEvtSupThread;
#endif
	if (ServerAttached == 1) pDHIThread->Release();

	// done
	m_ArchiveMode = -1;
	m_ArchivePercent = 100;
	sprintf(m_ArchiveStage, "Load Complete");
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Archive_Save_Thread
// Purpose:		thread that does the work to save a setup archive.  It retrieves
//				the data from the head(s) using the DAL and saves it in the archive files.
// Arguments:	dummy	- void
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	bug fix - release memory when done.
/////////////////////////////////////////////////////////////////////////////

void Archive_Save_Thread(void *dummy)
{
	// local variables
	long i = 0;
	long NumHeads = 0;
	long FileSize = 0;
	long ServerAttached = 0;
	long Status = 0;

	long HeadIndex[MAX_HEADS];

	char Command[MAX_STR_LEN];
	char WorkDir[MAX_STR_LEN];
	char ArchiveFile[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Archive_Save_Thread";

	unsigned char *filebuf = NULL;

    struct tm *newtime;
    time_t long_time;

	FILE *fp_marker = NULL;
	FILE *fp = NULL;

	IAbstract_DHI *pDHIThread;

	HRESULT hr = S_OK;

#ifdef ECAT_SCANNER
	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}
#endif

	// archive mode save
	m_ArchiveMode = ARCHIVE_SAVE;

	// chage the working directory to archive directory
	sprintf(WorkDir, "%sArchive", m_RootPath);
	_chdir(WorkDir);

	// create the archive directory
    time(&long_time);
    newtime = localtime(&long_time);
	sprintf(m_ArchiveDir, "%2.2d%s%4.4d_%2.2d%2.2d", newtime->tm_mday, MonthStr[newtime->tm_mon], 1900+newtime->tm_year, newtime->tm_hour, newtime->tm_min);
	sprintf(Command, "mkdir %s", m_ArchiveDir);
	system(Command);

	// attach to com server
	if (Status == 0) {
		pDHIThread = Get_DAL_Ptr(pErrEvtSupThread);
		if (pDHIThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed To Get DAL Pointer", 0);
	}

	// open the marker file
	sprintf(m_MarkerFile, "%s.h", m_ArchiveDir);
	for (i = 0 ; i < MAX_HEADS ; i++) {
		if (m_ArchiveSelect[i] == 1) sprintf(m_MarkerFile, "%s%d", m_MarkerFile, i);
	}
	sprintf(m_MarkerFile, "%s_%d", m_MarkerFile, m_Configuration);
	if ((fp_marker = fopen(m_MarkerFile, "wa")) == NULL) {
		sprintf(Str, "Opening File (%s)", m_MarkerFile);
		Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
	}

	// determine heads
	for (i = 0 ; i < MAX_HEADS ; i++) if (m_ArchiveSelect[i] == 1) HeadIndex[NumHeads++] = i;

	// loop through the heads
	for (i = 0 ; i < NumHeads ; i++) {

		// update percentage
		m_ArchivePercent = (i * 100) / NumHeads;

		// if an error has been encountered, skip loop
		if (Status != 0) continue;

		// stage information
		sprintf(m_ArchiveStage, "Head %d: Uploading Files", HeadIndex[i]);

		// use the local save routine
		Status = Local_Save(pErrEvtSupThread, pDHIThread, m_Configuration, HeadIndex[i], &FileSize, &filebuf);

		// output file
		sprintf(ArchiveFile, "%s\\Head%d_%d.har", m_ArchiveDir, HeadIndex[i], m_Configuration);

		// open file
		if (Status == 0) if ((fp = fopen(ArchiveFile, "wb")) == NULL) {
			sprintf(Str, "Opening File (%s)", ArchiveFile);
			Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
		}
		// file open
		if (Status == 0) {

			// write
			if (FileSize != (long) fwrite((void *) filebuf, sizeof(unsigned char), FileSize, fp)) {
				sprintf(Str, "Writing To File (%s)", ArchiveFile);
				Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
			}

			// release the memory
			::CoTaskMemFree((void *) filebuf);

			// close regardless
			fclose(fp);
		}

		// set head status
		m_ArchiveStatus[HeadIndex[i]] = Status;
	}

	// release the marker file
	fclose(fp_marker);

	// release server
#ifndef ECAT_SCANNER
	if (pErrEvtSupThread != NULL) delete pErrEvtSupThread;
#endif
	if (ServerAttached == 1) pDHIThread->Release();

	// if failed, remove any files created
	if (Status != 0) {

		// delete file
		sprintf((char *) Command, "erase %s\\Archive\\%s /q /f", m_RootPath, m_MarkerFile);
		system((char *) Command);

		// delete directory
		sprintf((char *) Command, "rmdir %s\\Archive\\%s /s /q", m_RootPath, m_ArchiveDir);
		system((char *) Command);
	}

	// done
	m_ArchiveMode = -1;
	m_ArchivePercent = 100;
	sprintf(m_ArchiveStage, "Save Complete");
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Singles_Driver
// Purpose:		control acquisition of new singles data and store in the
//				appropriate file - local routine
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
//				HeadSelect		-	long = Array of flags denoting whether or
//											not each head has been selected 
//				SourceSelect	-	long = Array of flags denoting whether or
//											not each point source has been selected 
//				LayerSelect		-	long = Array specifying layer for each head
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				AcquisitionType	-	long = (0 Seconds, 1 KEvents) units for amount
//				Amount			-	long = amount of data to acquire
//				lld				-	long = lower level discriminator
//				uld				-	long = upper level discriminator
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CDUMain::Acquire_Singles_Driver(long DataMode, long Configuration, long HeadSelect[], long SourceSelect[], long LayerSelect[], long AcquisitionType, long Amount, long lld, long uld)
{
	// local variables
	long i = 0;
	long Status = 0;

	char Subroutine[80] = "Acquire_Singles_Driver";

	// transfer variables
	m_DataMode = DataMode;
	m_Configuration = Configuration;
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_AcquireSelect[i] = HeadSelect[i];
		m_SourceSelect[i] = SourceSelect[i];
		m_AcquireLayer[i] = LayerSelect[i];
	}
	m_AcqType = AcquisitionType;
	m_Amount = Amount;
	m_lld = lld;
	m_uld = uld;

	// initialize percent complete to 0
	m_AcquirePercent = 0;

	// start up thread
	hAcquireThread = (HANDLE) _beginthread(Acquire_Singles_Thread, 0, NULL);

	// check for error starting thread
	if ((long)hAcquireThread == -1) Status = Add_Error(pErrEvtSup, Subroutine, "Could Not Start Acquisition Thread, Thread Handle", (long)hAcquireThread);

	// return status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquire_Coinc_Driver
// Purpose:		control acquisition of new coincidence data and store in in the
//				appropriate file. - local routine
// Arguments:	DataMode			-	long = Data Mode
//					8 = Time Difference
//					9 = Coincidence Flood
//				Configiuration	-	long = Electronics Configuration
//				HeadSelect		-	long = Array of flags denoting whether or
//											not each head has been selected 
//				LayerSelect		-	long = Array specifying layer for each head
//					0 = Fast
//					1 = Slow
//					2 = Combined
//				ModulePair		-	long = bitfield for detector head pairs which are allowed to be in coiincidence
//				Transmission	-	long = flag for whether or not point sources are to be extended
//				AcquisitionType	-	long = (0 Seconds, 1 KEvents) units for amount
//				Amount			-	long = amount of data to acquire
//				lld				-	long = lower level discriminator
//				uld				-	long = upper level discriminator
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CDUMain::Acquire_Coinc_Driver(long DataMode, long Configuration, long HeadSelect[], long LayerSelect[], long ModulePair, long Transmission, long AcquisitionType, long Amount, long lld, long uld)
{
	// local variables
	long i = 0;
	long Status = 0;

	char Subroutine[80] = "Acquire_Coinc_Driver";

	// transfer variables
	m_DataMode = DataMode;
	m_Configuration = Configuration;
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_AcquireSelect[i] = HeadSelect[i];
		m_AcquireLayer[i] = LayerSelect[i];
	}
	m_AcqType = AcquisitionType;
	m_Amount = Amount;
	m_ModulePair = ModulePair;
	m_Transmission = Transmission;
	m_lld = lld;
	m_uld = uld;

	// initialize percent complete to 0
	m_AcquirePercent = 0;

	// start up thread
	hAcquireThread = (HANDLE) _beginthread(Acquire_Coinc_Thread, 0, NULL);

	// check for error starting thread
	if ((long)hAcquireThread == -1) {
		Status = Status = Add_Error(pErrEvtSup, Subroutine, "Could Not Start Acquisition Thread, Thread Handle", (long)hAcquireThread);
		m_AcquirePercent = 100;
	}

	// return status
	return Status;

}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Release_Servers
// Purpose:		releases the acquisition and DAL servers
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUMain::Release_Servers()
{
	// release dhi server
	if (pDHI != NULL) pDHI->Release();

	// release acquisition server
	if (pACQ != NULL) pACQ->Release();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Local_Download
// Purpose:		local routine to do generic download of files to head
// Arguments:	pErrSup		-	CErrorEventSupport = Event handler
//				pDHILocal	-	IAbstract_DHI = pointer to DAL
//				Head		-	long = head to download to
//				Filename	-	char = name of file on ACS
//				Headname	-	unsigned char = name of file on head
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Local_Download(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Head, char *Filename, unsigned char *Headname)
{
	// local variables
	long i = 0;
	long Status = 0;
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Local_Download";
	unsigned long CheckSum = 0;
	unsigned char *FileData;
	FILE *fp;
	struct _stat file_status;
	HRESULT hr = S_OK;

	// get size of file in bytes
	if (Status == 0) if (_stat(Filename, &file_status) == -1) {
		sprintf(Str, "File Not Present (%s)", Filename);
		Status = Add_Error(pErrSup, Subroutine, Str, 0);
	}

	// allocate space
	if (Status == 0) if ((FileData = (unsigned char *) ::CoTaskMemAlloc(file_status.st_size)) == NULL) {
		Status = Add_Error(pErrSup, Subroutine, "Allocating Memory, Requested Size", file_status.st_size);
	}

	// open the file as binary to read
	if (Status == 0) if ((fp = fopen(Filename, "rb")) == NULL) {
		sprintf(Str, "Opening File (%s)", Filename);
		Status = Add_Error(pErrSup, Subroutine, Str, 0);
	}

	// successfull open
	if (Status == 0) {
		
		// read the data
		if (file_status.st_size != (long) fread((void *) FileData, sizeof(unsigned char), file_status.st_size, fp)) {
			sprintf(Str, "Reading File (%s), Requested Elements", Filename);
			Status = Add_Error(pErrSup, Subroutine, Str, file_status.st_size);
			::CoTaskMemFree((void *) FileData);
		}

		// close the file
		fclose(fp);
	}

	// download the file
	if (Status == 0) {

		// calculate the checksum
		for (CheckSum = 0, i = 0 ; i < file_status.st_size ; i++) CheckSum += (long) FileData[i];

		// download
		hr = pDHILocal->File_Download(Head, (long) file_status.st_size, CheckSum, Headname, &FileData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->File_Download)", hr);

		// free memory
		::CoTaskMemFree((void *) FileData);
	}

	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Local_Upload
// Purpose:		local routine to do generic upload of files from head
// Arguments:	pErrSup		-	CErrorEventSupport = Event handler
//				pDHILocal	-	IAbstract_DHI = pointer to DAL
//				Head		-	long = head to download to
//				Headname	-	unsigned char = name of file on head
//				Filename	-	char = name of file on ACS
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Local_Upload(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Head, unsigned char *Headname, char *Filename)
{
	// local variables
	long i = 0;
	long Status = 0;
	long Filesize = 0;
	long BufferRead = 0;

	unsigned long Check = 0;
	unsigned long CheckSum = 0;

	unsigned char *buffer = NULL;
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Local_Upload";

	FILE *fp = NULL;

	HRESULT hr = S_OK;

	// upload the file
	hr = pDHILocal->File_Upload(Head, Headname, &Filesize, &CheckSum, &buffer, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->FileUpload)", hr);

	// Checksum
	if (Status == 0) {

		// set flag that the buffer was read in (and will need to be released);
		BufferRead = 1;

		// build local checksum
		for (Check = 0, i = 0 ; i < Filesize ; i++) Check += (unsigned long) buffer[i];

		// compare against checksum from server
		if (Check != CheckSum) {
			sprintf(Str, "Checksum Did Not Match (File %s), Server = %d Local", Headname, CheckSum);
			Status = Add_Error(pErrSup, Subroutine, Str, Check);;
		}
	}

	// write the file
	if (Status == 0) {

		// open file
		if ((fp = fopen((char *) Filename, "wb")) != NULL) {

			// write data
			if ((long) fwrite((void *) buffer, sizeof(unsigned char), Filesize, fp) != Filesize) {
				sprintf(Str, "Writing To File (%s)", Filename);
				Status = Add_Error(pErrSup, Subroutine, Str, 0);
			}

			// file was opened, so close it
			fclose(fp);

		// failed open
		} else {
			sprintf(Str, "Opening File (%s)", Filename);
			Status = Add_Error(pErrSup, Subroutine, Str, 0);
		}
	}

	// release the memory
	if (BufferRead == 1) {
		if (buffer != NULL) ::CoTaskMemFree(buffer);
		buffer = NULL;
	}

	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Local_Backup
// Purpose:		local routine to backup setup data on head before starting new setup
// Arguments:	pErrSup		-	CErrorEventSupport = Event handler
//				pDHILocal	-	IAbstract_DHI = pointer to DAL
//				HeadSelect	-	long = flag array specifying which heads to back up
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Local_Backup(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long *HeadSelect) 
{
	// local variables
	long i = 0;
	long Head = 0;
	long Status = 0;

	char Subroutine[80] = "Local_Backup";

	HRESULT hr = S_OK;

	unsigned char Command[MAX_STR_LEN];

	// loop through the heads
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// if the head is selected
		if (HeadSelect[Head] != 0) {

			// if scanner is a P39, then indidual files have to be copied to a backup directory
			if (m_ScannerType == SCANNER_P39) {

				// delete any pre-existing backup directory and contents
				sprintf((char *) Command, "rmdir Backup /s /q");
				if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);

				// create the backup directory
				sprintf((char *) Command, "mkdir Backup");
				if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);

				// copy crm files
				sprintf((char *) Command, "copy %d_*.crm Backup\\", m_Configuration);
				if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);

				// copy chi files
				sprintf((char *) Command, "copy %d_*.chi Backup\\", m_Configuration);
				if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);

				// copy crystal energy peak and crystal time offset files
				sprintf((char *) Command, "copy %d_*.txt Backup\\", m_Configuration);
				if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);

				// copy crystal location files
				sprintf((char *) Command, "copy %d_pk*.bin Backup\\", m_Configuration);
				if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);

			// other scanner types, we can create a zip file with the data we want to save
			} else {

				// zip files on head
				sprintf((char *) Command, "pkzip Backup.zip");
				sprintf((char *) Command, "%s %d_*.crm", (char *) Command, m_Configuration);
				sprintf((char *) Command, "%s %d_*.chi", (char *) Command, m_Configuration);
				sprintf((char *) Command, "%s %d_*.txt", (char *) Command, m_Configuration);
				sprintf((char *) Command, "%s %d_pk*.bin", (char *) Command, m_Configuration);
				if (Status == 0) {
					hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);
				}
			}
		}
	}

	// return the status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Local_Restore
// Purpose:		local routine to restore backed up setup data
// Arguments:	pErrSup		-	CErrorEventSupport = Event handler
//				pDHILocal	-	IAbstract_DHI = pointer to DAL
//				HeadSelect	-	long = flag array specifying which heads to back up
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Local_Restore(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long *HeadSelect)
{
	// local variables
	long i = 0;
	long Head = 0;
	long Status = 0;

	char Subroutine[80] = "Local_Restore";

	HRESULT hr = S_OK;

	unsigned char Command[MAX_STR_LEN];

	// loop through the heads
	for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

		// if the head is selected
		if (HeadSelect[Head] != 0) {

			// if scanner is a P39, then indidual files have to be copied to a backup directory
			if (m_ScannerType == SCANNER_P39) {

				// copy all files from backup directory back up
				sprintf((char *) Command, "copy Backup\\*.* /y .\\");
				if (Status == 0) {
					hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);
				}

			// other scanner types, we can create a zip file with the data we want to save
			} else {

				// zip files on head
				sprintf((char *) Command, "pkunzip -o Backup.zip");
				if (Status == 0) {
					hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command)", hr);
				}
			}
		}
	}

	// force a reload of all values
	if (Status == 0) {
		hr = pDHILocal->Force_Reload();
		if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Force_Reload)", hr);
	}

	// return the status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		KillSelf
// Purpose:		exit server
// Arguments:	none
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::KillSelf()
{
	// try to release servers
	Release_Servers();

	// cause the server to exit
	exit(0);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Save_Head
// Purpose:		save setup data for a specific head and configuration
// Arguments:	Configuration		-	long = Electronics configuration
//				Head				-	long = Detector Head
//				*pSize				-	long = size of buffer in bytes
//				**buffer			-	unsigned char = head setup information
//				*pStatus			-	long = specific error code
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Save_Head(long Configuration, long Head, long *pSize, unsigned char **buffer, long *pStatus)
{
	char Subroutine[80] = "Save_Head";

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// use the local routine
	*pStatus = Local_Save(pErrEvtSup, pDHI, Configuration, Head, pSize, buffer);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Load_Head
// Purpose:		restore setup data for a specific head and configuration
// Arguments:	Configuration		-	long = Electronics configuration
//				Head				-	long = Detector Head
//				Size				-	long = size of buffer in bytes
//				**buffer			-	unsigned char = head setup information
//				*pStatus			-	long = specific error code
// Returns:		HRESULT
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Load_Head(long Configuration, long Head, long Size, unsigned char **buffer, long *pStatus)
{
	char Subroutine[80] = "Load_Head";

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_FALSE;
	}

	// argument check - head selection
	if ((Head < 0) || (Head >= MAX_HEADS)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Out of Range), Head", Head);
		return S_FALSE;
	} else if (m_HeadPresent[Head] != 1) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", Head);
		return S_FALSE;
	}

	// issue the load
	*pStatus = Local_Load(pErrEvtSup, pDHI, Configuration, Head, Size, buffer);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Add_Error
// Purpose:		send an error message to the event handler
//				add it to the log file and store it for recall
// Arguments:	*pErrSup	- CErrorEventSupport = Error handler
//				*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
//				*Why		- pertinent integer value
// Returns:		long specific error code
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(CErrorEventSupport *pErrSup, char *Where, char *What, long Why)
{
	// local variable
	long error_out = -1;
#ifndef ECAT_SCANNER
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_DSU";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// fire message to event handler
	if (pErrEvtSup != NULL) hr = pErrEvtSup->Fire(GroupName, DSU_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);

#endif
	// return error code
	return error_out;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Add_Information
// Purpose:	send an informational message to the event handler
//			and add it to the log
// Arguments:	*pErrSup	- CErrorEventSupport = Error handler
//				*Where		- char = which subroutine did the error occur in
//				*What		- char = what was the error
//				*Why		- pertinent integer value
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Add_Information(CErrorEventSupport *pErrSup, char *Where, char *What)
{
	// local variable
#ifndef ECAT_SCANNER
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_DSU";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s", Where, What);

	// fire message to event handler
	if (pErrEvtSup != NULL) hr = pErrEvtSup->Fire(GroupName, SAQ_GENERIC_DEVELOPER_INFO, Message, RawDataSize, EmptyStr, TRUE);

#endif
	// done
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_DAL_Ptr
// Purpose:		retrieve a pointer to the Detector Abstraction Layer COM server (DHICOM)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		IAbstract_DHI* pointer to DHICOM
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

IAbstract_DHI* Get_DAL_Ptr(CErrorEventSupport *pErrSup)
{
	// local variables
	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	IAbstract_DHI *Ptr = NULL;
	
	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// get server
	if (getenv("DHICOM_COMPUTER") == NULL) {
		sprintf(Server, "P39ACS");
	} else {
		sprintf(Server, "%s", getenv("DHICOM_COMPUTER"));
	}
	for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
	si.pwszName = Server_Name;

	// identify interface
	iid = IID_IAbstract_DHI;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	// get instance
	hr = ::CoCreateInstanceEx(CLSID_Abstract_DHI, NULL, CLSCTX_SERVER, &si, 1, mqi);
	if (FAILED(hr)) Add_Error(pErrSup, "Get_DAL_Ptr", "Failed ::CoCreateInstanceEx(CLSID_Abstract_DHI, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (IAbstract_DHI*) mqi[0].pItf;

	return Ptr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_ACQ_Ptr
// Purpose:		retrieve a pointer to the Singles Acquisition COM server (Acquisition)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		IACQMain* pointer to Acquisition
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

IACQMain* Get_ACQ_Ptr(CErrorEventSupport *pErrSup)
{
	// local variables
	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	IACQMain *Ptr = NULL;
	
	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// get server
	if (getenv("SINGLES_ACQUISITION_COMPUTER") == NULL) {
		sprintf(Server, "P39ACS");
	} else {
		sprintf(Server, "%s", getenv("SINGLES_ACQUISITION_COMPUTER"));
	}
	for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
	si.pwszName = Server_Name;

	// identify interface
	iid = IID_IACQMain;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	// get instance
	hr = ::CoCreateInstanceEx(CLSID_ACQMain, NULL, CLSCTX_SERVER, &si, 1, mqi);
	if (FAILED(hr)) Add_Error(pErrSup, "Get_ACQ_Ptr", "Failed ::CoCreateInstanceEx(CLSID_ACQMain, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (IACQMain*) mqi[0].pItf;

	return Ptr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Loacl_Save
// Purpose:		save setup data for a specific head and configuration
// Arguments:	*pErrSup			-	CErrorEventSupport = Error handler
//				pDHILocal			-	IAbstract_DHI = pointer to DHICOM
//				Configuration		-	long = Electronics configuration
//				Head				-	long = Detector Head
//				*pSize				-	long = size of buffer in bytes
//				**buffer			-	unsigned char = head setup information
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Local_Save(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Configuration, long Head, long *pSize, unsigned char **buffer)
{
	// local variables
	long i = 0;
	long Layer = 0;
	long CurrentSize = 0;
	long HeadSize = 0;
	long Pos = 0;
	long Status;

	unsigned long HeadSum = 0;
	unsigned long LocalSum = 0;

	long Settings[MAX_BLOCKS * MAX_SETTINGS];

	long *longptr = NULL;

	unsigned char *buf = NULL;
	unsigned char *hdbuf = NULL;

	char Subroutine[80] = "Local_Save";
	char Str[MAX_STR_LEN];
	unsigned char HeadFile[MAX_STR_LEN];

	HRESULT hr = S_OK;

	FILE *fp = NULL;

	// flag variables
	long PositionFlag[NUM_LAYER_TYPES] = {0, 0, 0};
	long PeaksFlag[NUM_LAYER_TYPES] = {0, 0, 0};
	long TimeFlag[NUM_LAYER_TYPES] = {0, 0, 0};

	// size variables
	long SettingsSize = m_NumBlocks * m_NumSettings;

	// flags based on scanner type
	switch (m_ScannerType) {

	case SCANNER_P39:
		PositionFlag[LAYER_FAST] = 1;
		PeaksFlag[LAYER_FAST] = 1;
		TimeFlag[LAYER_FAST] = 1;
		break;

	case SCANNER_P39_IIA:
		PositionFlag[LAYER_ALL] = 1;
		PeaksFlag[LAYER_ALL] = 1;
		TimeFlag[LAYER_ALL] = 1;
		break;

	case SCANNER_HRRT:
		PositionFlag[LAYER_FAST] = 1;
		PeaksFlag[LAYER_FAST] = 1;
		PeaksFlag[LAYER_SLOW] = 1;
		break;

	case SCANNER_PETSPECT:
		if (Configuration == 0) {
			PositionFlag[LAYER_FAST] = 1;
			PeaksFlag[LAYER_FAST] = 1;
			TimeFlag[LAYER_FAST] = 1;
			TimeFlag[LAYER_SLOW] = 1;
		}
		PositionFlag[LAYER_SLOW] = 1;
		PeaksFlag[LAYER_SLOW] = 1;
		break;

	default:
		return -1;
	}

	// get the current settings from the head
	hr = pDHILocal->Get_Analog_Settings(Configuration, Head, Settings, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Get_Analog_Settings), HRESULT", hr);
	if (Status != 0) return Status;

	// allocate first for settings
	CurrentSize = (SettingsSize * sizeof(long)) + sizeof(long);
	buf = (unsigned char*) ::CoTaskMemAlloc(CurrentSize);
	if (buf == NULL) return Add_Error(pErrSup, Subroutine, "Allocating Memory, Requested Size", CurrentSize);
	*buffer = buf;

	// transfer the settings
	longptr = (long*) *buffer;
	longptr[0] = SettingsSize;
	for (i = 0, Pos = 4 ; i < SettingsSize ; i++, Pos += 4) longptr[i+1] = Settings[i];

	// go through the layers
	for (Layer = 0 ; Layer < NUM_LAYER_TYPES ; Layer++) {

		// if there are positions for this layer
		if (PositionFlag[Layer] == 1) {

			// head file name
			sprintf((char *) HeadFile, "%d_pk%s.bin", Configuration, StrLayer[Layer]);

			// upload the file
			hr = pDHILocal->File_Upload(Head, HeadFile, &HeadSize, &HeadSum, &hdbuf, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Force_Upload)", hr);
			if (Status != 0) {
				::CoTaskMemFree((void *) *buffer);
				return Status;
			}

			// Increase the memory allocation
			CurrentSize += HeadSize + sizeof(long);
			buf = (unsigned char *) ::CoTaskMemRealloc((void *) *buffer, (unsigned long) CurrentSize);
			if (buf == NULL) {
				::CoTaskMemFree((void *) *buffer);
				::CoTaskMemFree((void *) hdbuf);
				return Add_Error(pErrSup, Subroutine, "Re-Allocating Memory, Requested Size", CurrentSize);
			}
			*buffer = buf;

			// insert the size
			longptr = (long*) &buf[Pos];
			longptr[0] = HeadSize;
			Pos += sizeof(long);

			// transfer
			for (LocalSum = 0, i = 0 ; i < HeadSize ; i++) {
				buf[Pos++] = hdbuf[i];
				LocalSum += (unsigned long) hdbuf[i];
			}

			// release buffer from head
			::CoTaskMemFree((void *) hdbuf);

			// if checksum did not match
			if (LocalSum != HeadSum) return -1;
		}

		// if there are crystal energy peaks for this layer
		if (PeaksFlag[Layer] == 1) {

			// head file name
			sprintf((char *) HeadFile, "%d_pk%s.txt", Configuration, StrLayer[Layer]);

			// upload the file
			hr = pDHILocal->File_Upload(Head, HeadFile, &HeadSize, &HeadSum, &hdbuf, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Force_Upload)", hr);
			if (Status != 0) {
				::CoTaskMemFree((void *) *buffer);
				return Status;
			}

			// Increase the memory allocation
			CurrentSize += HeadSize + sizeof(long);
			buf = (unsigned char *) ::CoTaskMemRealloc((void *) *buffer, (unsigned long) CurrentSize);
			if (buf == NULL) {
				::CoTaskMemFree((void *) *buffer);
				::CoTaskMemFree((void *) hdbuf);
				return Add_Error(pErrSup, Subroutine, "Re-Allocating Memory, Requested Size", CurrentSize);
			}
			*buffer = buf;

			// insert the size
			longptr = (long*) &buf[Pos];
			longptr[0] = HeadSize;
			Pos += sizeof(long);

			// transfer
			for (LocalSum = 0, i = 0 ; i < HeadSize ; i++) {
				buf[Pos++] = hdbuf[i];
				LocalSum += (unsigned long) hdbuf[i];
			}

			// release buffer from head
			::CoTaskMemFree((void *) hdbuf);

			// if checksum did not match
			if (LocalSum != HeadSum) return -1;
		}

		// if there are crystal time offsets for this layer
		if (TimeFlag[Layer] == 1) {

			// if this is a head test set
			if (m_HeadTestSet == 1) {

				// local file
				sprintf((char *) HeadFile, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_time.txt", m_RootPath, Configuration, Head, LayerStr[Layer], Head);

				// open the file
				if ((fp = fopen((char *) HeadFile, "rb")) == NULL) {
					::CoTaskMemFree((void *) *buffer);
					sprintf(Str, "Opening File: %s", HeadFile);
					return Add_Error(pErrSup, Subroutine, Str, 0);
				}

				// get the size of the file
				fseek(fp, 0, SEEK_END);
				HeadSize = ftell(fp);
				fseek(fp, 0, SEEK_SET);

				// allocate the buffer
				hdbuf = (unsigned char*) ::CoTaskMemAlloc(HeadSize);
				if (hdbuf == NULL) {
					::CoTaskMemFree((void *) *buffer);
					return Add_Error(pErrSup, Subroutine, "Allocating Memory, Requested Size", HeadSize);
				}

				// read the file
				if ((unsigned long) HeadSize != fread((void *) hdbuf, sizeof(unsigned char), HeadSize, fp)) {
					::CoTaskMemFree((void *) *buffer);
					::CoTaskMemFree((void *) hdbuf);
					fclose(fp);
					return Add_Error(pErrSup, Subroutine, "Reading File, Requested Bytes", HeadSize);
				}

				// close file
				fclose(fp);

				// Increase the memory allocation
				CurrentSize += HeadSize + sizeof(long);
				buf = (unsigned char *) ::CoTaskMemRealloc((void *) *buffer, (unsigned long) CurrentSize);
				if (buf == NULL) {
					::CoTaskMemFree((void *) *buffer);
					::CoTaskMemFree((void *) hdbuf);
					return Add_Error(pErrSup, Subroutine, "Re-Allocating Memory, Requested Size", CurrentSize);
				}
				*buffer = buf;

				// insert the size
				longptr = (long*) &buf[Pos];
				longptr[0] = HeadSize;
				Pos += sizeof(long);

				// transfer
				for (i = 0, LocalSum = 0 ; i < HeadSize ; i++) buf[Pos++] = hdbuf[i];

				// release buffer from head
				::CoTaskMemFree((void *) hdbuf);

			// otherwise, they should be on the head
			} else {

				// head file name
				sprintf((char *) HeadFile, "%d_tm%s.txt", Configuration, StrLayer[Layer]);

				// upload the file
				hr = pDHILocal->File_Upload(Head, HeadFile, &HeadSize, &HeadSum, &hdbuf, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Force_Upload)", hr);
				if (Status != 0) {
					::CoTaskMemFree((void *) *buffer);
					return Status;
				}

				// Increase the memory allocation
				CurrentSize += HeadSize + sizeof(long);
				buf = (unsigned char *) ::CoTaskMemRealloc((void *) *buffer, (unsigned long) CurrentSize);
				if (buf == NULL) {
					::CoTaskMemFree((void *) *buffer);
					::CoTaskMemFree((void *) hdbuf);
					return Add_Error(pErrSup, Subroutine, "Re-Allocating Memory, Requested Size", CurrentSize);
				}
				*buffer = buf;

				// insert the size
				longptr = (long*) &buf[Pos];
				longptr[0] = HeadSize;
				Pos += sizeof(long);

				// transfer
				for (LocalSum = 0, i = 0 ; i < HeadSize ; i++) {
					buf[Pos++] = hdbuf[i];
					LocalSum += (unsigned long) hdbuf[i];
				}

				// release buffer from head
				::CoTaskMemFree((void *) hdbuf);

				// if checksum did not match
				if (LocalSum != HeadSum) return -1;
			}
		}
	}

	// master tube energy data
	sprintf((char *) HeadFile, "%d_Master.te", Configuration);

	// upload the file
	hr = pDHILocal->File_Upload(Head, HeadFile, &HeadSize, &HeadSum, &hdbuf, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Force_Upload)", hr);
	if (Status != 0) {
		::CoTaskMemFree((void *) *buffer);
		return Status;
	}

	// Increase the memory allocation
	CurrentSize += HeadSize + sizeof(long);
	buf = (unsigned char *) ::CoTaskMemRealloc((void *) *buffer, (unsigned long) CurrentSize);
	if (buf == NULL) {
		::CoTaskMemFree((void *) *buffer);
		::CoTaskMemFree((void *) hdbuf);
		return Add_Error(pErrSup, Subroutine, "Re-Allocating Memory, Requested Size", CurrentSize);
	}
	*buffer = buf;

	// insert the size
	longptr = (long*) &buf[Pos];
	longptr[0] = HeadSize;
	Pos += sizeof(long);

	// transfer
	for (LocalSum = 0, i = 0 ; i < HeadSize ; i++) {
		buf[Pos++] = hdbuf[i];
		LocalSum += (unsigned long) hdbuf[i];
	}

	// release buffer from head
	::CoTaskMemFree((void *) hdbuf);

	// transfer size to output variable
	*pSize = CurrentSize;

	// if checksum did not match
	if (LocalSum != HeadSum) return -1;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Local_load
// Purpose:		load setup data for a specific head and configuration
// Arguments:	*pErrSup			-	CErrorEventSupport = Error handler
//				pDHILocal			-	IAbstract_DHI = pointer to DHICOM
//				Configuration		-	long = Electronics configuration
//				Head				-	long = Detector Head
//				Size				-	long = size of buffer in bytes
//				**buffer			-	unsigned char = head setup information
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	bug fixes - write
/////////////////////////////////////////////////////////////////////////////

long Local_Load(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long Configuration, long Head, long Size, unsigned char **buffer)
{
	// local variables
	long i = 0;
	long index = 0;
	long curr = 0;
	long next = 0;
	long Layer = 0;
	long Block = 0;
	long Verts_X = (2 * m_XCrystals) + 1;
	long Verts_Y = (2 * m_YCrystals) + 1;
	long CurrentSize = 0;
	long HeadSize = 0;
	long SettingsSize = 0;
	long Pos = 0;
	long BytesWritten = 0;
	long Status = 0;

	unsigned long HeadSum = 0;
	unsigned long LocalSum = 0;

	long BlockSelect[MAX_TOTAL_BLOCKS];
	long BlockStatus[MAX_TOTAL_BLOCKS];
	long Position[2178]; // 33 * 33 * 2;
	long Settings[MAX_BLOCKS * MAX_SETTINGS];

	long *longptr = NULL;

	unsigned char x = 0;
	unsigned char y = 0;
	unsigned char value = 0;

	unsigned char *buf = NULL;
	unsigned char *hdbuf = NULL;

	unsigned char CRM[PP_SIZE];
	unsigned char HeadFile[MAX_STR_LEN];
	unsigned char crmbuf[131072];		// 256 * 256 * 2 - worst case

	char LocalFile[MAX_STR_LEN];
	char MasterFile[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Local_Load";

	HRESULT hr = S_OK;

	FILE *fp = NULL;

	// flag variables
	long PositionFlag[NUM_LAYER_TYPES] = {0, 0, 0};
	long PeaksFlag[NUM_LAYER_TYPES] = {0, 0, 0};
	long TimeFlag[NUM_LAYER_TYPES] = {0, 0, 0};

	// flags based on scanner type
	switch (m_ScannerType) {

	case SCANNER_P39:
		PositionFlag[LAYER_FAST] = 1;
		PeaksFlag[LAYER_FAST] = 1;
		TimeFlag[LAYER_FAST] = 1;
		break;

	case SCANNER_P39_IIA:
		PositionFlag[LAYER_ALL] = 1;
		PeaksFlag[LAYER_ALL] = 1;
		TimeFlag[LAYER_ALL] = 1;
		break;

	case SCANNER_HRRT:
		PositionFlag[LAYER_FAST] = 1;
		PeaksFlag[LAYER_FAST] = 1;
		PeaksFlag[LAYER_SLOW] = 1;
		break;

	case SCANNER_PETSPECT:
		if (Configuration == 0) {
			PositionFlag[LAYER_FAST] = 1;
			PeaksFlag[LAYER_FAST] = 1;
			TimeFlag[LAYER_FAST] = 1;
			TimeFlag[LAYER_SLOW] = 1;
		}
		PositionFlag[LAYER_SLOW] = 1;
		PeaksFlag[LAYER_SLOW] = 1;
		break;

	default:
		return -1;
	}

	// transfer the settings
	buf = *buffer;
	longptr = (long *) &buf[0];
	SettingsSize = longptr[0];
	for (i = 0, Pos = 4 ; i < SettingsSize ; i++, Pos += 4) Settings[i] = longptr[i+1];

	// send the new settings to the head
	hr = pDHILocal->Set_Analog_Settings(Configuration, Head, Settings, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Set_Analog_Settings)", hr);
	if (Status != 0) {
		::CoTaskMemFree((void *) buffer);
		return Status;
	}

	// go through the layers
	for (Layer = 0 ; Layer < NUM_LAYER_TYPES ; Layer++) {

		// if there are positions for this layer
		if (PositionFlag[Layer] == 1) {

			// file names
			sprintf((char *) HeadFile, "%d_pk%s.bin", Configuration, StrLayer[Layer]);
			sprintf(LocalFile, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_%d_peaks.bin", m_RootPath, Configuration, Head, LayerStr[Layer], Head, Configuration);

			// get the size
			longptr = (long*) &buf[Pos];
			HeadSize = longptr[0];
			Pos += sizeof(long);

			// allocate buffer to send to head
			hdbuf = (unsigned char *) ::CoTaskMemAlloc((unsigned long) HeadSize);
			if (hdbuf == NULL) {
				::CoTaskMemFree((void *) buffer);
				return Add_Error(pErrSup, Subroutine, "Allocating Memory, Requested Size", HeadSize);
			}

			// transfer
			for (i = 0, LocalSum = 0 ; i < HeadSize ; i++) {
				hdbuf[i] = buf[Pos++];
				LocalSum += (long) hdbuf[i];
			}

			// download the file
			hr = pDHILocal->File_Download(Head, HeadSize, LocalSum, HeadFile, &hdbuf, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->File_Download), HRESULT", hr);
			if (Status != 0) {
				::CoTaskMemFree((void *) hdbuf);
				::CoTaskMemFree((void *) buffer);
				return Status;
			}

			// open the local file
			if ((fp = fopen(LocalFile, "wb")) == NULL) {
				::CoTaskMemFree((void *) hdbuf);
				::CoTaskMemFree((void *) buffer);
				sprintf(Str, "Opening File: %s", LocalFile);
				return Add_Error(pErrSup, Subroutine, Str, 0);
			}

			// write the data
			BytesWritten = (long) fwrite((void *) hdbuf, sizeof(unsigned char), HeadSize, fp);

			// close the file
			fclose(fp);

			// make sure the correct number of bytes were written
			if (BytesWritten != HeadSize) {
				::CoTaskMemFree((void *) hdbuf);
				return -1;
			}

			// generate the CRM for each emission block
			for (Block = 0 ; Block < m_EmissionBlocks ; Block++) {

				// transfer verticies
				index = 2 * Block * Verts_X * Verts_Y;
				for (i = 0 ; i < (2 * Verts_X * Verts_Y) ; i++) Position[i] = (long) hdbuf[index+i];

				// build the CRM
				Build_CRM(Position, m_XCrystals, m_YCrystals, CRM);

				// CRM file name
				sprintf(LocalFile, "%s\\Current\\Config_%d\\Head_%d\\%s%d_%d.crm", m_RootPath, Configuration, Head, LayerStr[Layer], Configuration, Block);

				// encode
				curr = 0;
				next = 0;
				while (curr < PP_SIZE) {

					// translate value to 16 x 16 block value and insert in field
					value = CRM[curr];
					if (value == 255) {
						crmbuf[next] = 255;
					} else {
						x = value % (unsigned char) m_XCrystals;
						y = value / (unsigned char) m_XCrystals;
						crmbuf[next] = (16 * y) + x;
					}

					// initialize count and move to the next value
					crmbuf[next+1] = 1;
					curr++;

					// loop while the same value
					while ((curr < PP_SIZE) && (CRM[curr] == value) && (crmbuf[next+1] < 63)) {
						crmbuf[next+1]++;
						curr++;
					}

					// next piece
					next += 2;
				}

				// open file
				if ((fp = fopen(LocalFile, "wb")) == NULL) {
					::CoTaskMemFree((void *) hdbuf);
					return -1;
				}

				// write the data
				BytesWritten = (long) fwrite((void *) crmbuf, sizeof(unsigned char), next, fp);

				// release file
				fclose(fp);
			}

			// release local buffer
			::CoTaskMemFree((void *) hdbuf);
		}

		// if there are crystal energy peaks for this layer
		if (PeaksFlag[Layer] == 1) {

			// head file name
			sprintf((char *) HeadFile, "%d_pk%s.txt", Configuration, StrLayer[Layer]);
			sprintf(LocalFile, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_ce.txt", m_RootPath, Configuration, Head, LayerStr[Layer], Head);

			// get the size
			longptr = (long*) &buf[Pos];
			HeadSize = longptr[0];
			Pos += sizeof(long);

			// allocate buffer to send to head
			hdbuf = (unsigned char *) ::CoTaskMemAlloc((unsigned long) HeadSize);
			if (hdbuf == NULL) {
				::CoTaskMemFree((void *) buffer);
				return Add_Error(pErrSup, Subroutine, "Allocating Memory, Requested Size", HeadSize);
			}

			// transfer
			for (i = 0, LocalSum = 0 ; i < HeadSize ; i++) {
				hdbuf[i] = buf[Pos++];
				LocalSum += (long) hdbuf[i];
			}

			// download the file
			hr = pDHILocal->File_Download(Head, HeadSize, LocalSum, HeadFile, &hdbuf, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->File_Download), HRESULT", hr);
			if (Status != 0) {
				::CoTaskMemFree((void *) hdbuf);
				::CoTaskMemFree((void *) buffer);
				return Status;
			}

			// open the local file
			if ((fp = fopen(LocalFile, "wb")) == NULL) {
				::CoTaskMemFree((void *) hdbuf);
				::CoTaskMemFree((void *) buffer);
				sprintf(Str, "Opening File: %s", LocalFile);
				return Add_Error(pErrSup, Subroutine, Str, 0);
			}

			// write the data
			BytesWritten = (long) fwrite((void *) hdbuf, sizeof(unsigned char), HeadSize, fp);

			// release local buffer
			::CoTaskMemFree((void *) hdbuf);

			// close the file
			fclose(fp);

			// make sure the correct number of bytes were written
			if (BytesWritten != HeadSize) return -1;
		}

		// if there are crystal time offsets for this layer
		if (TimeFlag[Layer] == 1) {

			// head file name
			sprintf((char *) HeadFile, "%d_tm%s.txt", Configuration, StrLayer[Layer]);
			sprintf(LocalFile, "%s\\Current\\Config_%d\\Head_%d\\%sh%d_time.txt", m_RootPath, Configuration, Head, LayerStr[Layer], Head);

			// get the size
			longptr = (long*) &buf[Pos];
			HeadSize = longptr[0];
			Pos += sizeof(long);

			// allocate buffer to send to head
			hdbuf = (unsigned char *) ::CoTaskMemAlloc((unsigned long) HeadSize);
			if (hdbuf == NULL) {
				::CoTaskMemFree((void *) buffer);
				return Add_Error(pErrSup, Subroutine, "Allocating Memory, Requested Size", HeadSize);
			}

			// transfer
			for (i = 0, LocalSum = 0 ; i < HeadSize ; i++) {
				hdbuf[i] = buf[Pos++];
				LocalSum += (long) hdbuf[i];
			}

			// download the file
			hr = pDHILocal->File_Download(Head, HeadSize, LocalSum, HeadFile, &hdbuf, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->File_Download), HRESULT", hr);
			if (Status != 0) {
				::CoTaskMemFree((void *) hdbuf);
				::CoTaskMemFree((void *) buffer);
				return Status;
			}

			// open the local file
			if ((fp = fopen(LocalFile, "wb")) == NULL) {
				::CoTaskMemFree((void *) hdbuf);
				::CoTaskMemFree((void *) buffer);
				sprintf(Str, "Opening File: %s", LocalFile);
				return Add_Error(pErrSup, Subroutine, Str, 0);
			}

			// write the data
			BytesWritten = (long) fwrite((void *) hdbuf, sizeof(unsigned char), HeadSize, fp);

			// release local buffer
			::CoTaskMemFree((void *) hdbuf);

			// close the file
			fclose(fp);

			// make sure the correct number of bytes were written
			if (BytesWritten != HeadSize) return -1;
		}
	}

	// select blocks for download
	index = Head * MAX_BLOCKS;
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) BlockSelect[i] = 0;
	for (i = 0 ; i < m_EmissionBlocks ; i++) BlockSelect[index+i] = 1;

	// download the crms
	Local_Download_CRM(pErrSup, pDHILocal, Configuration, BlockSelect, BlockStatus);

	// master tube energy data
	sprintf((char *) HeadFile, "%d_Master.te", Configuration);
	sprintf(MasterFile, "%sDefaults\\h%d_%d_%smaster.te", m_RootPath, Head, Configuration, LayerID[m_MasterLayer[Head]]);

	// get the size
	longptr = (long*) &buf[Pos];
	HeadSize = longptr[0];
	Pos += sizeof(long);

	// allocate buffer to send to head
	hdbuf = (unsigned char *) ::CoTaskMemAlloc((unsigned long) HeadSize);
	if (hdbuf == NULL) {
		::CoTaskMemFree((void *) buffer);
		return Add_Error(pErrSup, Subroutine, "Allocating Memory, Requested Size", HeadSize);
	}

	// transfer
	for (i = 0, LocalSum = 0 ; i < HeadSize ; i++) {
		hdbuf[i] = buf[Pos++];
		LocalSum += (long) hdbuf[i];
	}

	// download the file
	hr = pDHILocal->File_Download(Head, HeadSize, LocalSum, HeadFile, &hdbuf, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->File_Download), HRESULT", hr);
	if (Status != 0) {
		::CoTaskMemFree((void *) hdbuf);
		::CoTaskMemFree((void *) buffer);
		return Status;
	}

	// open the local file
	if ((fp = fopen(MasterFile, "wb")) == NULL) {
		::CoTaskMemFree((void *) hdbuf);
		::CoTaskMemFree((void *) buffer);
		sprintf(Str, "Opening File: %s", LocalFile);
		return Add_Error(pErrSup, Subroutine, Str, 0);
	}

	// write the data
	BytesWritten = (long) fwrite((void *) hdbuf, sizeof(unsigned char), HeadSize, fp);

	// release buffers
	::CoTaskMemFree((void *) hdbuf);
	::CoTaskMemFree((void *) buffer);

	// close the file
	fclose(fp);

	// make sure the correct number of bytes were written
	if (BytesWritten != HeadSize) {
		sprintf("Incorrect Number of Elements Written to File: %s, Expect = %d Actual", MasterFile, HeadSize);
		return Add_Error(pErrSup, Subroutine, Str, BytesWritten);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Acquisition_Statistics
// Purpose:		Return the last set of histogramming statistics to the user
// Arguments:	pNumKEvents		- pointer to unsigned long, number of KEvents (1024) processed
//				pBadAddress		- pointer to unsigned long, number of Events with Bad Addresses
//				pSyncProb		- pointer to unsigned long, number of 64-bit data sync errors
// Returns:		HRESULT
// History:		09 Dec 03	T Gremillion	Created
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDUMain::Acquisition_Statistics(unsigned long *pNumKEvents, unsigned long *pBadAddress, unsigned long *pSyncProb)
{
	// Copy statistics values
	*pNumKEvents = m_NumKEvents;
	*pBadAddress = m_BadAddress;
	*pSyncProb = m_SyncProb;

	return S_OK;
}
