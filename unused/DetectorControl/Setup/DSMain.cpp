//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Detector Setup COM Server (DSCOM.exe)
// 
// Name:			DSMain.cpp - Implementation of CDSMain
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	June 19, 2002
// 
// Description:		This component is the Detector Setup COM server which does
//					the does the processing for detector setup
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//
// File History:	29 Sep 03	T Gremillion	moved peak_analyze to find_peak_1d.cpp
//					19 Oct 04	T Gremillion	removed isotopeinfo include file
//////////////////////////////////////////////////////////////////////////////////////////////

// DSMain.cpp : Implementation of CDSMain
#include "stdafx.h"
#include "DSCom.h"
#include "DSMain.h"
#include "DHICom.h"
#include "DHICom_i.c"
#include "DUCom.h"
#include "DUCom_i.c"
#include "find_peak_1d.h"
#include "position_profile_tools.h"
#include "ecodes.h"
#include <time.h>
#include <process.h>
#include <math.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gantryModelClass.h"
#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

// global variables
static long m_TotalPercent;
static long m_StagePercent;
static long m_AcquirePercent;
static long m_SetupFlag;

static char m_Stage[MAX_STR_LEN];
static char m_RootPath[MAX_STR_LEN];

// global transfer variables
static long m_ScannerType;
static long m_ModelNumber;
static long m_FineGainIterations;
static long m_TuneGainIterations;
static long m_CFDIterations;
static long m_TxCFDIterations;
static long m_TxCFDStart;
static long m_NumConfigs;
static long m_NumHeads;
static long m_NumBlocks;
static long m_NumSettings;
static long m_XBlocks;
static long m_YBlocks;
static long m_EmissionBlocks;
static long m_XCrystals;
static long m_YCrystals;
static long m_NumCrystals;
static long m_Offset_Min;
static long m_Offset_Max;
static long m_Delay_Min;
static long m_Delay_Max;
static long m_ShapeLLD;
static long m_ShapeULD;
static long m_TubeEnergyLLD;
static long m_TubeEnergyULD;
static long m_CrystalEnergyLLD;
static long m_CrystalEnergyULD;
static long m_PositionProfileLLD;
static long m_PositionProfileULD;
static long m_MinGain;
static long m_MaxGain;
static long m_StepGain;
static long m_MaxExpand;
static long m_HeadPresent[MAX_HEADS];
static long m_PointSource[MAX_HEADS];
static long m_TransmissionStart[MAX_HEADS];
static long m_NumLayers[MAX_HEADS];
static long m_AimPoint[MAX_HEADS];
static long m_FineGainLayer[MAX_HEADS];
static long m_CFDLayer[MAX_HEADS];
static long m_CFDStart[MAX_HEADS];
static long m_HeadType[MAX_HEADS];
static long m_HeadBlocks[MAX_HEADS];
static long m_Rotate;
static long m_SetupType;
static long m_ModulePair;
static long m_TimingBins;
static long m_DataMode;
static long m_Configuration;
static long m_SetupSelect[MAX_TOTAL_BLOCKS];
static long m_AcquireSelect[MAX_HEADS];
static long m_BlankSelect[MAX_HEADS];
static long m_AcquireLayer[MAX_HEADS];
static long m_ConfigurationEnergy[MAX_CONFIGS];
static long m_ConfigurationLLD[MAX_CONFIGS];
static long m_ConfigurationULD[MAX_CONFIGS];
static long m_AcqType;
static long m_Amount;
static long m_lld;
static long m_uld;
static long m_EmTimeAlignDuration;
static long m_TxTimeAlignDuration;
static long m_TimeAlignIterations;

static long m_SetupStatus[MAX_TOTAL_BLOCKS];
static long m_AcquireStatus[MAX_HEADS];
static long m_Active[4*MAX_TOTAL_BLOCKS];

static double m_CFDRatio;
static double m_TransmissionCFDRatio;

static double m_SlowPercent[MAX_HEADS];

static FILE* m_fpLog;

// thread handles
static HANDLE hSetupThread;

// pointers to the classes
CErrorEventSupport *pErrEvtSup;
CErrorEventSupport *pErrEvtSupThread;
IAbstract_DHI *pDHI = NULL;
IAbstract_DHI *pDHIThread;
IDUMain *pDU = NULL;
IDUMain *pDUThread;

#define EMISSION_BLOCKS 0
#define TRANSMISSION_BLOCKS 1
#define HEAD_BLOCKS 2

// constants
static char *LayerID[NUM_LAYER_TYPES] = {"Fast_", "Slow_", ""};
static char *LayerStr[NUM_LAYER_TYPES] = {"Fast\\", "Slow\\", ""};
static char *StrLayer[NUM_LAYER_TYPES] = {"fast", "slow", "both"};
static char *ModeExt[NUM_DHI_MODES+1] = {"pp", "en", "te", "sd", "run", "trans", "spect", "time"};
static char *SetTitle[MAX_SETTINGS] = {" pmta", " pmtb", " pmtc", " pmtd", " cfd ", "delay", 
										"off_x", "off_y", "off_e", " mode", " KeV ", 
										"shape", " low ", " high", " low ", " high"};
static char *MonthStr[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *Letter[4] = {"A", "B", "C", "D"};
static char* BitField[15] = {"Initializing", "Determining Offset", "Coarse Gain", "Shape Discrimination", 
							 "Fine Gain", "Generating CRM", "Determining Crystal Energy Peaks", "Determining CFD Setting",
							 "Tuning Gains", "Transmission Gain", "Transmission CFD", "Time Alignment",
							 "Generating Run Mode Image", "Expanding CRM","Abort"};
static long SetIndex[MAX_SETTINGS];

// thread subroutines
void SetupThread(void *dummy);
void P39TimeAlignThread(void *dummy);
void HRRT_TimeAlignThread(void *dummy);

// stage subroutines
void Zap_Stage();
void Offset_Stage();
void Coarse_Stage();
void Shape_Stage();
void Fine_Stage();
void Tune_Stage();
void CRM_Stage();
void CFD_Stage();
void Peak_Stage();
void Run_Stage();
void TDC_Stage();
void Expand_Stage();
void Transmission_Gain_Stage();
void Transmission_CFD_Stage();

// internal processing subroutines
long Blocks_From_Head(long Type, long Head, long BlockSelect[MAX_BLOCKS]);
bool Remaining(long Head, long Block, long *Change);
void Set_Head_Status(long Head, long Status);
void Log_Message(char* Message);
long Local_Backup(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long *HeadSelect);
long Local_Restore(CErrorEventSupport *pErrSup, IAbstract_DHI *pDHILocal, long *HeadSelect);
void CRM_Initial_Trim();
long Set_Setup_Energy(long Configuration, long Head, long Value);

// error messaging
long Add_Error(CErrorEventSupport *pErrSup, char *Where, char *What, long Why);

IAbstract_DHI* Get_DAL_Ptr(CErrorEventSupport *pErrSup);
IDUMain* Get_DU_Ptr(CErrorEventSupport *pErrSup);

/////////////////////////////////////////////////////////////////////////////
// CDSMain


/////////////////////////////////////////////////////////////////////////////
// Routine:		FinalConstruct
// Purpose:		Server Initialization
// Arguments:	None
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

HRESULT CDSMain::FinalConstruct()
{
	// local variables
	long Status = 0;

	HRESULT hr = S_OK;

	// initialize settings identifiers
	if (Init_Globals() != 0) {

		// add error message
		Log_Message("DSCOM Server Not Initialized");

		// set the return status
		hr = S_FALSE;
	}

	// return success
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Abort
// Purpose:		Abort Setup
// Arguments:	None
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDSMain::Abort()
{
	// local variables
	long i = 0;
	long j = 0;
	long Selected = 0;
	long Status = 0;
	long Head = -1;
	long Off = 0;

	long HeadSelect[MAX_HEADS];

	char Subroutine[80] = "Abort";

	HRESULT hr = S_OK;

	// setup thread
	if (hSetupThread != NULL) {

		// fill stage string with abort message
		sprintf(m_Stage, "Aborting....");

		//add abort message to log
		Log_Message("Aborting");

		// kill thread
		TerminateThread(hSetupThread, 0);

		// fill stage string with abort message again (it might have been changed prior to termination)
		sprintf(m_Stage, "Aborting....");

		// Release Handle
		CloseHandle(hSetupThread);

		// clear handle
		hSetupThread = NULL;

		// block status to abort
		for (i = 0 ; i < MAX_HEADS ; i++) HeadSelect[i] = 0;
		for (i = 0 ; i < (MAX_TOTAL_BLOCKS) ; i++) {
			if (m_SetupSelect[i] == 1) {
				m_SetupStatus[i] = ABORT_FATAL;
				HeadSelect[i/MAX_BLOCKS] = 1;
			}
		}

		// Turn High Voltage On
		hr = pDHI->High_Voltage(1, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, Subroutine, "Method Failed (pDHI->High_Voltage), HRESULT", hr);

		// retract point sources
		hr = pDHI->Point_Source(Head, Off, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, Subroutine, "Method Failed (pDHI->Point_Source), HRESULT", hr);

		// unzip backup file
		Status = Local_Restore(pErrEvtSupThread, pDHI, HeadSelect);
	}

	// set completion
	m_AcquirePercent = 100;
	m_StagePercent = 100;
	m_TotalPercent = 100;

	// clear flags
	m_SetupFlag = 0;

	// clear stage string
	sprintf(m_Stage, "Setup Idle");

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Setup
// Purpose:		Start setting up scanner based on arguments
// Arguments:	Type			-	long = type of setup
//					0	-	Full Emission/Transmission
//					1	-	Full Emission
//					2	-	Tune Gains
//					3	-	Correction
//					4	-	Crystal Energy
//					5	-	Full Transmission
//					6	-	Time Alignment
//					7	-	Electronic Calibration
//					8	-	Shape Discrimination
//				Configuration	-	long = Electronics Configuration
//				BlockSelect		-	long = flag array of blocks to be set up
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDSMain::Setup(long Type, long Configuration, long BlockSelect[], long *pStatus)
{
	// local variables
	long i = 0;
	char Subroutine[80] = "Setup";

	// initialize status
	*pStatus = 0;

	// argument check - setup type
	if ((Type < 0) || (Type >= NUM_SETUP_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Setup Type", Type);
		return S_OK;
	}

	// time alignment not handled by this method
	if (SETUP_TYPE_TIME_ALIGNMENT == Type) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Separate Method For Time Alignment, Setup Type", Type);
		return S_OK;
	}

	// hardware calibration for HRRT only
	if ((SCANNER_HRRT != m_ScannerType) && (SETUP_TYPE_ELECTRONIC_CALIBRATION == Type)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Electronic Calibration Not Available For This Model, Model", m_ModelNumber);
		return S_OK;
	}

	// argument check - configuration number
	if ((Configuration < 0) || (Configuration >= m_NumConfigs)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Configuration", Configuration);
		return S_OK;
	}

	// check validity of blocks selected
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
		if (BlockSelect[i] != 0) {

			// head must be present
			if (m_HeadPresent[i/MAX_BLOCKS] != 1) {
				*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Present), Head", (i/MAX_BLOCKS));
				return S_OK;
			}

			// for emission type setups, it must be an emission block
			if ((i % MAX_BLOCKS) >= m_EmissionBlocks) {
				if ((Type == SETUP_TYPE_FULL_EMISSION) || (Type == SETUP_TYPE_TUNE_GAINS) || 
					(Type == SETUP_TYPE_CORRECTIONS) || (Type == SETUP_TYPE_CRYSTAL_ENERGY)) {
					*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument (Not Emission), Block", (i%MAX_BLOCKS));
					return S_OK;
				}
			}
		}
	}

	// if a setup is already running, then return
	if (m_SetupFlag != 0) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Setup Already Running, Flag", m_SetupFlag);
		return S_OK;
	}

	// set setup flag
	m_SetupFlag = 1;

	// Transfer inputs to global variables
	m_SetupType = Type;
	m_Configuration = Configuration;
	for (i = 0 ; i < (MAX_TOTAL_BLOCKS) ; i++) m_SetupSelect[i] = BlockSelect[i];
	for (i = 0 ; i < (MAX_TOTAL_BLOCKS) ; i++) m_SetupStatus[i] = 0;

	// Reset progress
	m_TotalPercent = 0;

	// start up thread
	hSetupThread = (HANDLE) _beginthread(SetupThread, 0, NULL);

	// check for error starting thread
	if ((long)hSetupThread == -1) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Setup Thread Not Started, HANDLE", (long)hSetupThread);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Time_Alignment
// Purpose:		Start time aligning scanner based on arguments
// Arguments:	Type			-	long = type of setup
//					0	-	P39 Stage I (3 head)
//					1	-	P39 Stage II (3 head)
//					2	-	Transmission
//					3	-	P39 > 3 heads
//				ModulePair	-	long = specifies which heads are allowed to be in coincidence
//				*pStatus		-	long = specific return status
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDSMain::Time_Alignment(long Type, long ModulePair, long *pStatus)
{
	// local variables
	long i = 0;
	char Subroutine[80] = "Time_Alignment";

	// initialize status
	*pStatus = 0;

	// argument check - setup type
	if ((Type < 0) || (Type >= NUM_TIME_ALIGN_TYPES)) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Time Alignment Type", Type);
		return S_OK;
	}

	// argument check - module pair
	if (((SCANNER_P39 == m_ScannerType) || (SCANNER_P39_IIA == m_ScannerType)) && ((ModulePair <= 0) || (ModulePair > 0x1FFE))) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Invalid Argument, Module Pair", ModulePair);
		return S_OK;
	}

	// if a setup is already running, then return
	if (m_SetupFlag != 0) {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Setup Already Running, Flag", m_SetupFlag);
		return S_OK;
	}

	// set setup flag
	m_SetupFlag = 1;

	// Transfer inputs to global variables
	m_SetupType = Type;
	m_ModulePair = ModulePair;
	for (i = 0 ; i < (MAX_TOTAL_BLOCKS) ; i++) m_SetupStatus[i] = 0;

	// Reset progress
	m_TotalPercent = 0;
	m_StagePercent = 0;

	// start up thread
	if (SCANNER_HRRT == m_ScannerType) {
		hSetupThread = (HANDLE) _beginthread(HRRT_TimeAlignThread, 0, NULL);
	} else if ((SCANNER_P39 == m_ScannerType) || (SCANNER_P39_IIA == m_ScannerType)) {
		hSetupThread = (HANDLE) _beginthread(P39TimeAlignThread, 0, NULL);
	} else {
		*pStatus = Add_Error(pErrEvtSup, Subroutine, "Time Alignment Not Defined For Scanner Type, Type", m_ScannerType);
	}

	// check for error starting thread
	if ((*pStatus == 0) && ((long)hSetupThread == -1)) *pStatus = Add_Error(pErrEvtSup, Subroutine, "Time Alignment (P39) Thread Not Started, HANDLE", (long)hSetupThread);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Error_Lookup
// Purpose:		Retrieve the specified error's type and message
// Arguments:	ErrorCode		-	long = Error code to be looked up
//				*Code			-	long = Type of error
//				*ErrorString	-	unsigned char = Error descriptor
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDSMain::Error_Lookup(long Code, unsigned char ErrorStr[])
{
	char Subroutine[80] = "Error_Lookup";

	// act on code
	switch (Code) {

	case 0:
		sprintf((char *) ErrorStr, "Successful Completion");
		break;

	default:
		sprintf((char *) ErrorStr, "Unknown Error Code");
		break;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Setup_Progress
// Purpose:		returns percent complete of current stage and of total setup
// Arguments:	Stage			-	unsigned char = stage of setup currently being run
//				pStagePercent	-	long = percent of stage completion
//				pTotalPercent	-	long = percent of total completion
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDSMain::Setup_Progress(unsigned char Stage[], long *pStagePercent, long *pTotalPercent)
{
	char Subroutine[80] = "Setup_Progress";

	// transfer values
	*pStagePercent = m_StagePercent;
	*pTotalPercent = m_TotalPercent;
	sprintf((char *) Stage, m_Stage);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Setup_Finished
// Purpose:		Retrieve the status at completion
// Arguments:	BlockStatus		-	long = setup status of each block
// Returns:		HRESULT
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDSMain::Setup_Finished(long BlockStatus[])
{
	char Subroutine[80] = "Setup_Finished";

	// local variables
	long i = 0;

	// transfer status values
	for (i = 0 ; i < (MAX_TOTAL_BLOCKS) ; i++) BlockStatus[i] = m_SetupStatus[i];

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Init_Globals
// Purpose:		Retrieve values from the gantry model and set global values
// Arguments:	None
// Returns:		long Status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long CDSMain::Init_Globals()
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long Status = 0;

	char Subroutine[80] = "Init_Globals";

	HRESULT hr = S_OK;

	// instantiate a Gantry Object Model object
	CGantryModel model;
	pHead Head;		
	pConf Config;
	pPtSrc PointSource;

	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport(false, true);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSup->InitComplete(hr) == false) {
		delete pErrEvtSup;
		pErrEvtSup = NULL;
	}

	// set gantry model to model number held in register if none held, use the 393
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) if (!model.setModel(393)) return 1;

	// set scanner type from gantry model
	m_ModelNumber = model.modelNumber();
	switch (m_ModelNumber) {

	// PETSPECT
	case 311:
		m_ScannerType = SCANNER_PETSPECT;
		break;

	// HRRT
	case 328:
		m_ScannerType = SCANNER_HRRT;
		break;

	// P39 family (391 - 396)
	case 391:
	case 392:
	case 393:
	case 394:
	case 395:
	case 396:
		m_ScannerType = SCANNER_P39;
		break;

	// P39 Phase IIA Electronics family (2391 - 2396)
	case 2391:
	case 2392:
	case 2393:
	case 2394:
	case 2395:
	case 2396:
		m_ScannerType = SCANNER_P39_IIA;
		break;

	// not a panel detector - return error
	default:
		return 1;
		break;
	}

	// number of blocks
	m_NumBlocks = model.blocks();
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

	// initialize head arrays
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_HeadPresent[i] = 0;
		m_PointSource[i] = 0;
		m_TransmissionStart[i] = 0;
		m_NumLayers[i] = 1;
		m_AimPoint[i] = 150;
		m_FineGainLayer[i] = LAYER_ALL;
		m_CFDLayer[i] = LAYER_ALL;
		m_CFDStart[i] = 100;
		m_HeadType[i] = HEAD_TYPE_LSO_ONLY;
		m_SlowPercent[i] = 0.0;
	}

	// clear active flags for point sources
	for (i = 0 ; i < (4*MAX_TOTAL_BLOCKS) ; i++) m_Active[i] = -1;

	// set head values
	for (m_NumHeads = 0, i = 0 ; i < model.nHeads() ; i++) {

		// keeping track of the highest head number
		if (Head[i].address >= m_NumHeads) m_NumHeads = Head[i].address + 1;

		// indicate that the head is present
		m_HeadPresent[Head[i].address] = 1;

		// if head has point sources hooked up to it, get the vales
		if (Head[i].pointSrcData) {

			// number of point sources
			m_PointSource[Head[i].address] = model.nPointSources(Head[i].address);

			// starting block
			m_TransmissionStart[Head[i].address] = model.pointSourceStart(Head[i].address);

			// associate active tubes with their data
			PointSource = model.headPtSrcInfo(Head[i].address);
			for (j = 0 ; j < m_PointSource[Head[i].address] ; j++) {
				index = ((MAX_BLOCKS * Head[i].address) + m_TransmissionStart[Head[i].address] + j) * 4;
				m_Active[index+0] = PointSource[j].pmta;
				m_Active[index+1] = PointSource[j].pmtb;
				m_Active[index+2] = PointSource[j].pmtc;
				m_Active[index+3] = PointSource[j].pmtd;
			}
		}

		// Gain information
		m_AimPoint[Head[i].address] = Head[i].GainAimPoint;
		m_FineGainLayer[Head[i].address] = Head[i].GainLayer;

		// cfd starting value
		m_CFDStart[Head[i].address] = Head[i].cfdStart;

		// number of layers
		m_HeadType[Head[i].address] = Head[i].type;
		switch (m_HeadType[Head[i].address]) {

		case HEAD_TYPE_LSO_ONLY:
			m_CFDLayer[Head[i].address] = LAYER_ALL;
			m_NumLayers[Head[i].address] = 1;
			m_SlowPercent[Head[i].address] = 0.0;
			break;

		case HEAD_TYPE_LSO_LSO:
			m_CFDLayer[Head[i].address] = LAYER_FAST;
			m_NumLayers[Head[i].address] = 2;
			m_SlowPercent[Head[i].address] = 0.658;
			break;

		case HEAD_TYPE_LSO_NAI:
			m_CFDLayer[Head[i].address] = LAYER_SLOW;
			m_NumLayers[Head[i].address] = 2;
			m_SlowPercent[Head[i].address] = 0.2;
			break;

		case HEAD_TYPE_GSO_LSO:
		case HEAD_TYPE_LYSO_LSO:
			m_CFDLayer[Head[i].address] = LAYER_SLOW;
			m_NumLayers[Head[i].address] = 2;
			m_SlowPercent[Head[i].address] = 0.25;
			break;

		// default head type is single layer
		default:
			m_NumLayers[Head[i].address] = 1;
			break;
		}
	}

	// configuration values
	m_NumConfigs = model.nConfig();
	Config = model.configInfo();
	for (i = 0 ; i < m_NumConfigs ; i++) {
		m_ConfigurationEnergy[i] = Config[i].energy;
		m_ConfigurationLLD[i] = Config[i].lld;
		m_ConfigurationULD[i] = Config[i].uld;
	}

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

	// setup values
	sprintf(m_RootPath, model.SetupDir());
	m_FineGainIterations = model.fineGainIterations();
	m_TuneGainIterations = model.fineGainIterations();
	m_Offset_Min = model.offsetMin();
	m_Offset_Max = model.offsetMax();
	m_Delay_Min = model.minCFDDelay();
	m_Delay_Max = model.maxCFDDelay();
	m_ShapeLLD = model.LLDshape();
	m_ShapeULD = model.ULDshape();
	m_TubeEnergyLLD = model.LLDtubeEnergy();
	m_TubeEnergyULD = model.ULDtubeEnergy();
	m_CrystalEnergyLLD = model.LLDcrystalEnergy();
	m_CrystalEnergyULD = model.ULDcrystalEnergy();
	m_PositionProfileLLD = model.LLDprofile();
	m_PositionProfileULD = model.ULDprofile();
	m_MinGain = model.minCoarseGain();
	m_MaxGain = model.maxCoarseGain();
	m_StepGain = model.stepCoarseGain();
	m_CFDIterations = model.cfdIterations();
	m_MaxExpand = model.maxExpandCRM();
	m_CFDRatio = model.CFDRatio();
	m_TransmissionCFDRatio = model.txCFDRatio();
	m_TxCFDIterations = model.txCFDIterations();
	m_TxCFDStart = model.txCFDStart();
	m_EmTimeAlignDuration = model.emTimeAlignDuration();
	m_TxTimeAlignDuration = model.txTimeAlignDuration();
	m_TimeAlignIterations = model.timeAlignIterations();
	m_TimingBins = model.timingBins();

	// initialize global values
	m_SetupFlag = 0;
	m_StagePercent = 100;
	m_TotalPercent = 100;
	m_AcquirePercent = 100;
	m_fpLog = NULL;

	// calculated values
	m_EmissionBlocks = m_XBlocks * m_YBlocks;
	m_NumCrystals = m_XCrystals * m_YCrystals;

	// strings
	sprintf(m_Stage, "Setup Idle");

	// clear handles
	hSetupThread = NULL;

	// Create an instance of DHI Server.
	pDHI = Get_DAL_Ptr(pErrEvtSup);
	if (pDHI == NULL) Status = Add_Error(pErrEvtSup, Subroutine, "Failed to Get DAL Pointer", 0);

	// Create an instance of Detector Utilities Server.
	if (Status == 0) {
		pDU = Get_DU_Ptr(pErrEvtSup);

		// if it failed, release the dhi pointer
		if (pDU == NULL) {
			pDHI->Release();
			Status = Add_Error(pErrEvtSup, Subroutine, "Failed to Get DU Pointer", 0);

		// if it succeeded, releease it (this is just a pre-test to make sure it exists)
		} else {
			pDU->Release();
		}
	}

	// return status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		SetupThread
// Purpose:		Work thread for setup processing.  based on setup type, it 
//				runs through the appropriate stages
// Arguments:	dummy		- void
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	Removed Expansion of CRMS from Full Setup for HRRT
//											Added 10 second wait after turning on high voltage
/////////////////////////////////////////////////////////////////////////////

void SetupThread(void *dummy)
{
	// local variables
	long i = 0;
	long j = 0;
	long local = 0;
	long masked = 0;
	long Head = 0;
	long Block = 0;
	long NumBlocks = 0;
	long TotalBlocks = 0;
	long Status = 0;
	long NumStages = 0;
	long LastStage = 0;
	long Server_Locked = 0;
	long SourcesOn = 0;
	long Off = 0;
	long On = 1;

	long BlockSelect[MAX_BLOCKS];
	long DownSelect[MAX_TOTAL_BLOCKS];
	long DownStatus[MAX_TOTAL_BLOCKS];

	HRESULT hr = S_OK;

	// flag for each stage
	long Zap_Flag = 0;
	long Offset_Flag = 0;
	long Coarse_Flag = 0;
	long Shape_Flag = 0;
	long Fine_Flag = 0;
	long Tune_Flag = 0;
	long CRM_Flag = 0;
	long CFD_Flag = 0;
	long Peak_Flag = 0;
	long Run_Flag = 0;
	long Trim_Flag = 0;
	long Expand_Flag = 0;
	long TDC_Flag = 0;
	long Transmission_CoarseGain_Flag = 0;
	long Transmission_CFD_Flag = 0;
	long Transmission_FineGain_Flag = 0;
	long Time_Align_Flag = 0;

	long HeadSelect[MAX_HEADS];

	char LogFile[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "SetupThread";

	unsigned char CommandString[MAX_STR_LEN];
	unsigned char ResponseString[MAX_STR_LEN];

    struct tm *newtime;
    time_t long_time;

	// blank selection to turn off all transmission or all emission
	for (i = 0 ; i < MAX_HEADS ; i++) m_BlankSelect[i] = 0;

	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}

	// initialize stage percentage
	m_StagePercent = 0;
	m_TotalPercent = 0;
	sprintf(m_Stage, "Starting Setup");

	// log file name
    time(&long_time);
    newtime = localtime(&long_time);
	sprintf(LogFile, "%sLog\\%2.2d%s%4.4d_%2.2d%2.2d.log", m_RootPath, newtime->tm_mday, 
			MonthStr[newtime->tm_mon], 1900+newtime->tm_year, newtime->tm_hour, newtime->tm_min);

	// open log file
	m_fpLog = fopen(LogFile, "wt");
	if (m_fpLog == NULL) {
		sprintf(m_Stage, "Failed To Open Log File");
		Sleep(15000);
		sprintf(Str, "Failed To Open Setup Log File: %s", LogFile);
		Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
	}

	// log start message
	Log_Message("Starting Setup");

	// log model information
	sprintf(Str, "Model Number: %d", m_ModelNumber);
	Log_Message(Str);

	// log scanner type
	switch (m_ScannerType) {

	case SCANNER_P39:
		Log_Message("Scanner Type: P39 (Phase I Electronics)");
		break;

	case SCANNER_P39_IIA:
		Log_Message("Scanner Type: P39 (Phase IIA Electronics)");
		break;

	case SCANNER_PETSPECT:
		Log_Message("Scanner Type: PETSPECT");
		break;

	case SCANNER_HRRT:
		Log_Message("Scanner Type: HRRT");
		break;

	default:
		Log_Message("Scanner Type: Unknown");
		Status = Add_Error(pErrEvtSupThread, Subroutine, "Unknown Scanner Type, Model", m_ModelNumber);
		break;
	}

	// Create an instance of DHI Server.
	if (Status == 0) {
		pDHIThread = Get_DAL_Ptr(pErrEvtSupThread);
		if (pDHIThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get DAL Pointer", 0);
		if (Status != 0) {
			Log_Message("Failed Attaching to Detector Abstraction Layer Server");
		}
	}

	// Create an instance of Detector Utilities Server.
	if (Status == 0) {
		pDUThread = Get_DU_Ptr(pErrEvtSupThread);
		if (pDUThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get DU Pointer", 0);
		if (Status != 0) {
			pDHIThread->Release();
			Log_Message("Failed Attaching to Detector Utilities Server");
		}
	}

	// bad status, set fatal error
	if (Status != 0) {
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) if (m_SetupSelect[i] != 0) m_SetupStatus[i] = BLOCK_FATAL;
	}

	// set flag for each processing stage based on setup type
	switch (m_SetupType) {

	case SETUP_TYPE_FULL:
		Log_Message("Setup Type: Full Emission/Transmission");
		Zap_Flag = 1;
		if (m_ScannerType != SCANNER_P39_IIA) Offset_Flag = 1;
		Coarse_Flag = 1;
		if (m_ScannerType != SCANNER_P39_IIA) Shape_Flag = 1;
		Fine_Flag = 1;
		CRM_Flag = 1;
		CFD_Flag = 1;
		Peak_Flag = 1;
		if (m_ScannerType != SCANNER_HRRT) Expand_Flag = 1;
		Run_Flag = 1;
		if (m_ScannerType == SCANNER_P39_IIA) TDC_Flag = 1;
		Transmission_CoarseGain_Flag = 1;
		Transmission_CFD_Flag = 1;
		Transmission_FineGain_Flag = 1;
		break;

	case SETUP_TYPE_FULL_EMISSION:
		Log_Message("Setup Type: Full Emission");
		Zap_Flag = 1;
		if ((m_ScannerType != SCANNER_P39_IIA) && (m_ScannerType != SCANNER_HRRT)) Offset_Flag = 1;
		Coarse_Flag = 1;
		if (m_ScannerType != SCANNER_P39_IIA) Shape_Flag = 1;
		Fine_Flag = 1;
		CRM_Flag = 1;
		CFD_Flag = 1;
		Peak_Flag = 1;
		if (m_ScannerType != SCANNER_HRRT) Expand_Flag = 1;
		Run_Flag = 1;
		if (m_ScannerType == SCANNER_P39_IIA) TDC_Flag = 1;
		break;

	case SETUP_TYPE_TUNE_GAINS:
		Log_Message("Setup Type: Tune Gains");
		Zap_Flag = 1;
		Tune_Flag = 1;
		Peak_Flag = 1;
		Run_Flag = 1;
		break;

	case SETUP_TYPE_CORRECTIONS:
		Log_Message("Setup Type: Corrections");
		Zap_Flag = 1;
		CFD_Flag = 1;
		Peak_Flag = 1;
		Trim_Flag = 1;
		Expand_Flag = 1;
		Run_Flag = 1;
		break;

	case SETUP_TYPE_CRYSTAL_ENERGY:
		Log_Message("Setup Type: Crystal Energy Peaks");
		Zap_Flag = 1;
		Peak_Flag = 1;
		Run_Flag = 1;
		break;

	case SETUP_TYPE_TRANSMISSION_DETECTORS:
		Log_Message("Setup Type: Full Transmission");
		Zap_Flag = 1;
		Transmission_CoarseGain_Flag = 1;
		Transmission_CFD_Flag = 1;
		Transmission_FineGain_Flag = 1;
		break;

	case SETUP_TYPE_TIME_ALIGNMENT:
		Log_Message("Setup Type: Time Alignment");
		Time_Align_Flag = 1;
		break;

	case SETUP_TYPE_ELECTRONIC_CALIBRATION:
		Log_Message("Setup Type: Electronic Calibration");
		Zap_Flag = 1;
		Offset_Flag = 1;
		break;

	case SETUP_TYPE_SHAPE:
		Log_Message("Setup Type: Shape Discrimination");
		Zap_Flag = 1;
		Shape_Flag = 1;
		Run_Flag = 1;
		break;

	default:
		Log_Message("Setup Type Unknown");
		break;
	}

	// list number of blocks on each head to be processed
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {
		if (m_HeadPresent[Head] == 1) {
			NumBlocks = Blocks_From_Head(HEAD_BLOCKS, Head, BlockSelect);
			m_HeadBlocks[Head] = NumBlocks;
			TotalBlocks += NumBlocks;
			sprintf(Str, "Head %d: %d Blocks Being Processed", Head, NumBlocks);
			Log_Message(Str);
		}
	}

	// total number of blocks to be processed
	if (TotalBlocks > 0) {
		sprintf(Str, "Total Number of Blocks To Be Processed: %d", TotalBlocks);
	} else {
		sprintf(Str, "No Blocks Selected: Exiting");
		Status = -1;
	}
	Log_Message(Str);

	// provide backup
	for (Head = 0 ; Head < MAX_HEADS ; Head++) HeadSelect[Head] = 0;
	for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {
		if (Blocks_From_Head(HEAD_BLOCKS, Head, BlockSelect) > 0) HeadSelect[Head] = 1;
	}
	Status = Local_Backup(pErrEvtSupThread, pDHIThread, HeadSelect);

	// determine the number of stages
	NumStages = Zap_Flag + Offset_Flag + Coarse_Flag + Shape_Flag + Fine_Flag + Tune_Flag + CRM_Flag + Trim_Flag +
				CFD_Flag + Peak_Flag + Run_Flag + Expand_Flag + TDC_Flag + Time_Align_Flag + 
				Transmission_CoarseGain_Flag + Transmission_CFD_Flag + Transmission_FineGain_Flag;

	// check if a zap is to be done
	if ((Status == 0) && (Zap_Flag == 1)) {

		// call the stage
		Zap_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if a offsets are to be done
	if ((Status == 0) && (Offset_Flag == 1)) {

		// call the stage
		Offset_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if coarse gain is to be done
	if ((Status == 0) && (Coarse_Flag == 1)) {

		// call the stage
		Coarse_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if shape discrimination is to be done
	if ((Status == 0) && (Shape_Flag == 1)) {

		// call the stage
		Shape_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if fine gain is to be done
	if ((Status == 0) && (Fine_Flag == 1)) {

		// call the stage
		Fine_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if tune gain is to be done
	if ((Status == 0) && (Tune_Flag == 1)) {

		// call the stage
		Tune_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if CRM Generation is to be done
	if ((Status == 0) && (CRM_Flag == 1)) {

		// call the stage
		CRM_Stage();

		// log message
		Log_Message("CRM Download");

		// transfer selection
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
			DownSelect[i] = 0;
			if ((m_SetupSelect[i] != 0) && ((m_SetupStatus[i] & BLOCK_FATAL) != BLOCK_FATAL)) DownSelect[i] = 1;
		}

		// download CRMs
		hr = pDUThread->Download_CRM(m_Configuration, DownSelect, DownStatus);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Download_CRM), HRESULT", hr);

		// check for download errors
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
			if ((DownSelect[i] == 1) && (DownStatus[i] != 0)) {
				m_SetupStatus[i] |= BLOCK_CRM_FATAL;
				sprintf(Str, "Head %d Block %d:  Download Error", (i / MAX_BLOCKS), (i % MAX_BLOCKS));
				Log_Message(Str);
			}
		}

		// log message
		Log_Message("Generate - CRM Download Complete");

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// if CRMs, were not just generated, but are to be expanded, they need to be trimmed first
	if ((Status == 0) && (Trim_Flag == 1)) {

		// trim them
		CRM_Initial_Trim();

		// log message
		Log_Message("CRM Download");

		// transfer selection
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
			DownSelect[i] = 0;
			if ((m_SetupSelect[i] != 0) && ((m_SetupStatus[i] & BLOCK_FATAL) != BLOCK_FATAL)) DownSelect[i] = 1;
		}

		// download CRMs
		hr = pDUThread->Download_CRM(m_Configuration, DownSelect, DownStatus);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Download_CRM), HRESULT", hr);

		// check for download errors
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
			if ((DownSelect[i] == 1) && (DownStatus[i] != 0)) {
				m_SetupStatus[i] |= BLOCK_EXPAND_FATAL;
				sprintf(Str, "Head %d Block %d:  Download Error", (i / MAX_BLOCKS), (i % MAX_BLOCKS));
				Log_Message(Str);
			}
		}

		// log message
		Log_Message("Trim - CRM Download Complete");

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if CFD setting is to be done
	if ((Status == 0) && (CFD_Flag == 1)) {

		// call the stage
		CFD_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if the crystal energy peaks are to be done
	if ((Status == 0) && (Peak_Flag == 1)) {

		// call the stage
		Peak_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if the CRMs are to be expanded
	if ((Status == 0) && (Expand_Flag == 1)) {

		// call the stage
		Expand_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if the run mode image is to be acquired
	if ((Status == 0) && (Run_Flag == 1)) {

		// call the stage
		Run_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if the tdc values are to be set by firmware
	if ((Status == 0) && (TDC_Flag == 1)) {

		// call the stage
		TDC_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// if any transmission setup is being done, extend the appropriate sources
	if ((Status == 0) && ((Transmission_CoarseGain_Flag == 1) || (Transmission_FineGain_Flag == 1) || (Transmission_CFD_Flag == 1))) {

		// Set Flag
		SourcesOn = 1;

		// loop through the heads
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// if transmission blocks on the head have been selected
			if (Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect) > 0) {

				// Log Message
				sprintf(Str, "Head %d: Extending Transmission Source", Head);
				Log_Message(Str);

				// Extend Source
				hr = pDHIThread->Point_Source(Head, On, &Status);

				// if error occurred, set blocks to fail (use transmission gain codes)
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Point_Source), HRESULT", hr);
				if (Status != 0) {

					// Log Message
					Log_Message("Failed Extending Transmission Source");

					// loop through blocks
					for (i = 0 ; i < MAX_BLOCKS ; i++) {
						if (BlockSelect[i] == 1) m_SetupStatus[(Head*MAX_BLOCKS)+i] |= BLOCK_TRANSMISSION_GAIN_FATAL;
					}

					// clear the status (we could continue with other heads)
					Status = 0;
				}
			}
		}
	}

	// check if the transmission source tube gains are to be determined
	if ((Status == 0) && (Transmission_CoarseGain_Flag == 1)) {

		// call the stage
		Transmission_Gain_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if the transmission source cfd values are to be determined
	if ((Status == 0) && (Transmission_CFD_Flag == 1)) {

		// call the stage
		Transmission_CFD_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// check if the transmission source tube gains are to be determined
	if ((Status == 0) && (Transmission_FineGain_Flag == 1)) {

		// call the stage
		Transmission_Gain_Stage();

		// increment stage counters
		LastStage++;
		m_TotalPercent = (LastStage * 100) / NumStages;
	}

	// if any transmission setup is being done, extend the appropriate sources
	if (SourcesOn == 1) {

		// loop through the heads
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// if the head has any pointsources
			if (m_PointSource[Head] > 0) {

				// Log Message
				sprintf(Str, "Head %d: Retracting Transmission Source", Head);
				Log_Message(Str);

				// Retract Source
				hr = pDHIThread->Point_Source(Head, Off, &Status);

				// if error occurred, set blocks to fail (use transmission gain codes)
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Point_Source), HRESULT", hr);
				if (Status != 0) {

					// Log Message
					Log_Message("Failed Retracting Transmission Source");

					// clear the status (we need to try all heads)
					Status = 0;
				}
			}
		}
	}

	// Log final status of each block
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {

		// if selected
		if (m_SetupSelect[i] != 0) {

			// Head and block numbers
			Head = i / MAX_BLOCKS;
			Block = i % MAX_BLOCKS;

			// build message
			sprintf(Str, "Head %d Block %d:", Head, Block);

			if (m_SetupStatus[i] == 0) {

				// success
				sprintf(Str, "%s\tSuccess", Str);
				Log_Message(Str);

			// log errors and warnings
			} else {

				// print head and block number as lead in
				Log_Message(Str);

				// use local variable
				local = m_SetupStatus[i];

				// if failed to lock server
				if (local == BLOCK_FATAL) Log_Message("Unable to Lock Server");

				// loop through fatal errors
				for (j = 1 ; j < 16 ; j++) {

					// if the shifted data shows fatal
					masked = (local >> (2 * j)) & BLOCK_FATAL;
					if (masked == BLOCK_FATAL) {
						sprintf(Str, "\tError - %s", BitField[j-1]);
						Log_Message(Str);
					}
				}

				// loop through warnings
				for (j = 1 ; j < 16 ; j++) {

					// if the shifted data shows fatal
					masked = (local >> (2 * j)) & BLOCK_FATAL;
					if (masked == BLOCK_NOT_FATAL) {
						sprintf(Str, "\tWarning - %s", BitField[j-1]);
						Log_Message(Str);
					}
				}
			}
		}
	}

	// switch all heads to all blocks
	for (Head = 0 ; (Head < MAX_HEADS) && (pDHIThread != NULL) && (m_ScannerType == SCANNER_HRRT) ; Head++) {
		if (m_HeadPresent[Head] == 1) {

			// log message
			sprintf(Str, "Turning On All Blocks For Head %d", Head);
			Log_Message(Str);

			// send command
			sprintf((char *) CommandString, "I -1");
			hr = pDHIThread->Pass_Command(Head, CommandString, ResponseString, &Status);
			if (FAILED(hr)) Add_Error(pErrEvtSupThread, Subroutine, "Failed pDHITHread->Pass_Command(Head, CommandString, ResponseString, &Status), HRESULT =", hr);
		}
	}

	// close log file
	if (m_fpLog != NULL) fclose(m_fpLog);

	// release pointers
	pDHIThread->Release();
	pDUThread->Release();
	delete pErrEvtSupThread;

	// finish off Percentages
	m_StagePercent = 100;
	m_TotalPercent = 100;

	// clear setup flag
	m_SetupFlag = 0;

	// clear stage string
	sprintf(m_Stage, "Setup Idle");

	// finished
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Zap_Stage
// Purpose:		Resets head(s) to default values either fully or partially
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Zap_Stage()
{
	// local variables
	long i = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long HeadCount = 0;
	long PerHead = 0;
	long StartPercent = 0;
	long SetupType = 0;

	long BlockSelect[MAX_BLOCKS];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Zap_Stage";

	HRESULT hr = S_OK;

	// initialize stage percentage
	sprintf(m_Stage, "Initializing Heads");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(HEAD_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// percentage per head
	PerHead = 100 / NumHeads;

	// for the hrrt, electronic calibration sets the offsets and cfd delay (flex circuits are disconnected).
	// setting them at any other time would corrupt them, but the crystatl energy still needs to be zapped
	SetupType = m_SetupType;
	if ((SCANNER_HRRT == m_ScannerType) && (SETUP_TYPE_FULL_EMISSION == m_SetupType)) SetupType = SETUP_TYPE_CRYSTAL_ENERGY;

	// log message
	Log_Message("Making Sure High Voltage is On");

	// issue command
	hr = pDHIThread->High_Voltage(1, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->High_Voltage), HRESULT", hr);
	if (Status != 0) {
		Log_Message("Failed To Turn On High Voltage");
		for (Head = 0 ; Head < m_NumHeads ; Head++) {
			if (Blocks_From_Head(HEAD_BLOCKS, Head, BlockSelect) > 0) Set_Head_Status(Head, BLOCK_ZAP_FATAL);
		}
	} else Sleep(10000);

	// loop through the heads
	for (Head = 0 ; (Head < m_NumHeads) && (Status == 0) ; Head++) {

		// reset status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(HEAD_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// increment stage percentage
		m_StagePercent = (HeadCount * 100) / NumHeads;
		StartPercent = m_StagePercent;
		HeadCount++;

		// zap mode based on setup type
		switch (SetupType) {

		// full setup requires all settings to be reset to defaults
		case SETUP_TYPE_FULL:
		case SETUP_TYPE_FULL_EMISSION:
		case SETUP_TYPE_TRANSMISSION_DETECTORS:
		case SETUP_TYPE_ELECTRONIC_CALIBRATION:

			// if all the blocks on the head are being set up, then just zap all on the whole head
			if (NumBlocks == m_NumBlocks) {

				// Log Message
				sprintf(Str, "Head %d: Zapping All Values", Head);
				Log_Message(Str);

				// issue command
				hr = pDHIThread->Zap(ZAP_ALL, m_Configuration, Head, ALL_BLOCKS, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Zap), HRESULT", hr);
				if (Status != 0) Log_Message("Zap Failed");

				// stage percentage
				m_StagePercent = StartPercent + ((97 * PerHead) / 100);

			// otherwise do it the long way
			} else {

				// Log Message
				sprintf(Str, "Head %d: Zapping All Crystal Energy Peaks", Head);
				Log_Message(Str);

				// zap the energy for the whole head
				hr = pDHIThread->Zap(ZAP_ENERGY, m_Configuration, Head, ALL_BLOCKS, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Zap), HRESULT", hr);
				if (Status != 0) Log_Message("Zap Failed");

				// stage percentage
				m_StagePercent = StartPercent + ((25 * PerHead) / 100);

				// zap all for each individual block
				for (Block = 0 ; (Block < m_NumBlocks) && (Status == 0) ; Block++) {
					if (BlockSelect[Block] != 0) {

						// stage percentage
						m_StagePercent = StartPercent + ((PerHead * (25 + ((72 * Block) / NumBlocks))) / 100);

						// Log Message
						sprintf(Str, "Head %d: Zapping All Values For Block %d", Head, Block);
						Log_Message(Str);

						// issue command
						hr = pDHIThread->Zap(ZAP_ALL, m_Configuration, Head, Block, &Status);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Zap), HRESULT", hr);
						if (Status != 0) Log_Message("Zap Failed");
					}
				}
			}
			break;

		// other emission setups just need the energy peaks cleared out
		case SETUP_TYPE_TUNE_GAINS:
		case SETUP_TYPE_CORRECTIONS:
		case SETUP_TYPE_CRYSTAL_ENERGY:

			// Log Message
			sprintf(Str, "Head %d: Zapping All Crystal Energy Peaks", Head);
			Log_Message(Str);

			// issue command
			hr = pDHIThread->Zap(ZAP_ENERGY, m_Configuration, Head, ALL_BLOCKS, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Zap), HRESULT", hr);
			if (Status != 0) Log_Message("Zap Failed");
			break;

		case SETUP_TYPE_TIME_ALIGNMENT:
			break;

		default:
			break;
		}

		// except for time alignment, we need the detectors in bin mode (time alignment will make sure it is in the correct mode)
		if ((Status == 0) && (m_SetupType != SETUP_TYPE_TIME_ALIGNMENT)) Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// if some part failed, then set whole head to fatal error
		if (Status != 0) Set_Head_Status(Head, BLOCK_ZAP_FATAL);
	}

	// Stage Complete
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 100;
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Offset_Stage
// Purpose:		Executes the firmware on the detector heads that determine
//				x,y & e offsets P39 phase I
//				x & y offsets HRRT
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
//				22 Nov 04	T Gremillion	Added extra time for High Voltage to change states
//											only process warnings for blocks that have been selected.
//											correctly identify when cfd_delay is being set.
/////////////////////////////////////////////////////////////////////////////

void Offset_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long NumBlocks = 0;
	long ErrCode = 0;
	long NumHeads = 0;
	long HeadCount = 0;
	long VoltageOff = 0;
	long PerHead = 0;

	long BlockSelect[MAX_BLOCKS];
	long Before[MAX_SETTINGS*MAX_BLOCKS];
	long After[MAX_SETTINGS*MAX_BLOCKS];

	char *ptr = NULL;
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Offset_Stage";

	unsigned char ErrMsg[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// initialize stage percentage
	sprintf(m_Stage, "Determining Block Offsets");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// 40 % percent for actual processing
	PerHead = 40 / NumHeads;

	// turn off the high voltage if at lease one head is to be set
	if (NumHeads > 0) {

		// log message
		Log_Message("Turning Off High Voltage");

		// issue command
		hr = pDHIThread->High_Voltage(0, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->High_Voltage), HRESULT", hr);

		// turned off high voltage, give it 30 seconds to settle down
		if (Status == 0) {
			VoltageOff = 1;
			for (m_StagePercent = 0 ; m_StagePercent < 30 ; m_StagePercent += 1) Sleep(1000);

		// high voltage not turned off, can't continue
		} else {

			// log message
			Log_Message("Failed to Turn Off High Voltage");

			// fail all blocks
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (Blocks_From_Head(HEAD_BLOCKS, Head, BlockSelect) > 0) Set_Head_Status(Head, BLOCK_OFFSET_FATAL);
			}
		}
	}

	// loop through the heads
	for (Head = 0 ; (Head < m_NumHeads) && (VoltageOff == 1) ; Head++) {

		// reset status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// increment stage percentage
		m_StagePercent = (HeadCount * 100) / NumHeads;
		HeadCount++;

		// if not all the blocks, then get the current settings
		if (NumBlocks != m_NumBlocks) {

			// Log Message
			sprintf(Str, "Head %d: Not All Blocks Selected - Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get settings
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, Before, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// set the offsets for the entire head
		if (Status == 0) {

			// Log Message
			sprintf(Str, "Head %d: Firmware Setting Offsets", Head);
			Log_Message(Str);

			// let firmware determine offset settings
			hr = pDHIThread->Determine_Offsets(m_Configuration, Head, ALL_BLOCKS, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Determine_Offsets), HRESULT", hr);

			// if a bad status was returned, it is not necessarily fatal
			if (SUCCEEDED(hr) && (Status != 0)) {

				// retrieve the error code and message
				hr = pDHIThread->Error_Lookup(Status, &ErrCode, ErrMsg);
				if (FAILED(hr)) Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Error_Lookup), HRESULT", hr);

				// if error was just values set out of range, clear it - will look for out of range
				if (SUCCEEDED(hr)) if (ErrCode == E_XY_OFFSET_OOR) {

					// list of blocks out of range
					ptr = (char *) &ErrMsg[22];

					// loop through list
					while ('F' != ptr[1]) {

						// Block Number
						Block = strtol(ptr, &ptr, 0);

						// if the block was selected
						if (m_SetupSelect[(Head*MAX_BLOCKS)+Block] != 0) {

							// set warning flag
							m_SetupStatus[(Head*MAX_BLOCKS)+Block] = BLOCK_OFFSET_NOT_FATAL;

							// note it in the log
							sprintf(Str, "Head %d Block %d - Firmware Failed to Correctly Set Offsets", Head, Block);
							Log_Message(Str);
						}
					}

					// clear the general message
					Status = 0;
				}
			}
			if (Status != 0) Log_Message("Failed to Set Offsets");
			if (m_ScannerType == SCANNER_HRRT) {
				m_StagePercent += PerHead / 2;

			// stage precentage
			} else {
				m_StagePercent += PerHead;
			}
		}

		// set the cfd delay for the entire head if HRRT
		if ((Status == 0) && (m_ScannerType == SCANNER_HRRT)){

			// correctly identify the stage
			sprintf(m_Stage, "Determining CFD Delay");

			// Log Message
			sprintf(Str, "Head %d: Firmware Setting CFD Delay", Head);
			Log_Message(Str);

			// let firmware determine offset settings
			hr = pDHIThread->Determine_Delay(m_Configuration, Head, ALL_BLOCKS, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Determine_Delay), HRESULT", hr);

			// if a bad status was returned, it is not necessarily fatal
			if (SUCCEEDED(hr) && (Status != 0)) {

				// retrieve the error code and message
				hr = pDHIThread->Error_Lookup(Status, &ErrCode, ErrMsg);
				if (FAILED(hr)) Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Error_Lookup), HRESULT", hr);

				// // if error was just values set out of range, clear it - will look for out of range
				if (SUCCEEDED(hr)) if (ErrCode == E_DELAY_OOR) {

					// list of blocks out of range
					ptr = (char *) &ErrMsg[22];

					// loop through list
					while ('F' != ptr[1]) {

						// Block Number
						Block = strtol(ptr, &ptr, 0);

						// if the block was selected
						if (m_SetupSelect[(Head*MAX_BLOCKS)+Block] != 0) {

							// set warning flag
							m_SetupStatus[(Head*MAX_BLOCKS)+Block] = BLOCK_OFFSET_NOT_FATAL;

							// note it in the log
							sprintf(Str, "Head %d Block %d - Firmware Failed to Correctly Set CFD Delay", Head, Block);
							Log_Message(Str);
						}
					}

					// clear the general message
					Status = 0;
				}
			}

			// stage percentage
			m_StagePercent += PerHead / 2;
		}

		// get the resulting settings
		if (Status == 0) {

			// Log Message
			sprintf(Str, "Head %d: Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get settings
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, After, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// transfer values from before if not all set
		if ((Status == 0) && (NumBlocks != m_NumBlocks)) {
			for (i = 0 ; i < m_NumBlocks ; i++) {
				Block = (Head * MAX_BLOCKS) + i;
				if (BlockSelect[i] == 0) {
					for (j = 0 ; j < m_NumSettings ; j++) After[(i*m_NumSettings)+j] = Before[(i*m_NumSettings)+j];
				}
			}
		}

		// send settings back to head
		if (Status == 0) {

			// update analog settings
			sprintf(Str, "Head %d: Updating Analog Settings", Head);
			Log_Message(Str);

			// issue command
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, After, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed go Update Analog Settings");
		}

		// check for values out-of-range (OOR)
		if (Status == 0) {
			for (i = 0 ; i < m_NumBlocks ; i++) {
				Block = (Head * MAX_BLOCKS) + i;
				if ((BlockSelect[i] != 0) && ((m_SetupStatus[Block] & BLOCK_OFFSET_NOT_FATAL) != BLOCK_OFFSET_NOT_FATAL)) {

					// check X offset
					if ((After[(i*m_NumSettings)+SetIndex[X_OFFSET]] < m_Offset_Min) ||
						(After[(i*m_NumSettings)+SetIndex[X_OFFSET]] > m_Offset_Max)) {
						m_SetupStatus[Block] |= BLOCK_OFFSET_NOT_FATAL;
						sprintf(Str, "Head %d Block %d: Warning on X Offset Value %d", Head, i, After[(i*m_NumSettings)+SetIndex[X_OFFSET]]);
						Log_Message(Str);
					}

					// check Y offset
					if ((After[(i*m_NumSettings)+SetIndex[Y_OFFSET]] < m_Offset_Min) ||
						(After[(i*m_NumSettings)+SetIndex[Y_OFFSET]] > m_Offset_Max)) {
						m_SetupStatus[Block] |= BLOCK_OFFSET_NOT_FATAL;
						sprintf(Str, "Head %d Block %d: Warning on Y Offset Value %d", Head, i, After[(i*m_NumSettings)+SetIndex[Y_OFFSET]]);
						Log_Message(Str);
					}

					// if available, check energy offset
					if (SetIndex[E_OFFSET] != -1) {
						if ((After[(i*m_NumSettings)+SetIndex[E_OFFSET]] < m_Offset_Min) ||
							(After[(i*m_NumSettings)+SetIndex[E_OFFSET]] > m_Offset_Max)) {
							m_SetupStatus[Block] |= BLOCK_OFFSET_NOT_FATAL;
							sprintf(Str, "Head %d Block %d: Warning on Energy Offset Value %d", Head, i, After[(i*m_NumSettings)+SetIndex[E_OFFSET]]);
							Log_Message(Str);
						}
					}

					// if available, check cfd delay
					if (SetIndex[CFD_DELAY] != -1) {
						if ((After[(i*m_NumSettings)+SetIndex[CFD_DELAY]] < m_Delay_Min) ||
							(After[(i*m_NumSettings)+SetIndex[CFD_DELAY]] > m_Delay_Max)) {
							m_SetupStatus[Block] |= BLOCK_OFFSET_NOT_FATAL;
							sprintf(Str, "Head %d Block %d: Warning on CFD Delay Value %d", Head, i, After[(i*m_NumSettings)+SetIndex[CFD_DELAY]]);
							Log_Message(Str);
						}
					}
				}
			}
		}

		// if some part failed, then set whole head to fatal error
		if (Status != 0) Set_Head_Status(Head, BLOCK_OFFSET_FATAL);
	}

	// turn on the high voltage if it was turned off
	if (VoltageOff == 1) {

		// log message
		Log_Message("Turning On High Voltage");
		
		// issue command
		hr = pDHIThread->High_Voltage(1, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->High_Voltage), HRESULT", hr);

		// turned on high voltage, give it 15 seconds to settle down
		if (Status == 0) {
			for (m_StagePercent = 55 ; m_StagePercent < 97 ; m_StagePercent += 1) Sleep(1000);

		// high voltage not turned on, can't continue
		} else {

			// log message
			Log_Message("Failed to Turn On High Voltage");

			// fail all blocks
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (Blocks_From_Head(HEAD_BLOCKS, Head, BlockSelect) > 0) Set_Head_Status(Head, BLOCK_OFFSET_FATAL);
			}

		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		TDC_Stage
// Purpose:		Executes the firmware that determines the TDC values (P39 phase IIA)
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void TDC_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long NumBlocks = 0;
	long ErrCode = 0;
	long NumHeads = 0;
	long HeadCount = 0;
	long PerHead = 0;

	long BlockSelect[MAX_BLOCKS];
	long Before[MAX_SETTINGS*MAX_BLOCKS];
	long After[MAX_SETTINGS*MAX_BLOCKS];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "TDC_Stage";

	unsigned char ErrMsg[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// initialize stage percentage
	sprintf(m_Stage, "Determining Block TDC Values");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// 100 % percent for actual processing
	PerHead = 100 / NumHeads;

	// loop through the heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// increment stage percentage
		m_StagePercent = (HeadCount * 100) / NumHeads;
		HeadCount++;

		// if not all the blocks, then get the current settings
		if (NumBlocks != m_NumBlocks) {

			// Log Message
			sprintf(Str, "Head %d: Not All Blocks Selected - Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get settings
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, Before, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// set the offsets for the entire head
		if (Status == 0) {

			// Log Message
			sprintf(Str, "Head %d: Firmware Setting TDC Values", Head);
			Log_Message(Str);

			// let firmware determine offset settings
			hr = pDHIThread->Determine_Offsets(m_Configuration, Head, ALL_BLOCKS, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Determine_Offsets), HRESULT", hr);

			// if a bad status was returned, it is not necessarily fatal
			if (SUCCEEDED(hr) && (Status != 0)) {

				// retrieve the error code and message
				hr = pDHIThread->Error_Lookup(Status, &ErrCode, ErrMsg);
				if (FAILED(hr)) Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Error_Lookup), HRESULT", hr);

				// // if error was just values set out of range, clear it - will look for out of range
				if (SUCCEEDED(hr)) if (ErrCode == E_XY_OFFSET_OOR) {
					sprintf(Str, "Firmware Detected Problems - Response: %s", (char *) ErrMsg);
					Log_Message(Str);
					Status = 0;
				}
			}
			if (Status != 0) Log_Message("Failed to Set TDC Values");

			// stage precentage
			m_StagePercent += PerHead;
		}

		// get the resulting settings
		if (Status == 0) {

			// Log Message
			sprintf(Str, "Head %d: Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get settings
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, After, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// transfer values from before if not all set
		if ((Status == 0) && (NumBlocks != m_NumBlocks)) {
			for (i = 0 ; i < m_NumBlocks ; i++) {
				Block = (Head * MAX_BLOCKS) + i;
				if (BlockSelect[i] == 0) {
					for (j = 0 ; j < m_NumSettings ; j++) After[(i*m_NumSettings)+j] = Before[(i*m_NumSettings)+j];
				}
			}
		}

		// send settings back to head
		if (Status == 0) {

			// update analog settings
			sprintf(Str, "Head %d: Updating Analog Settings", Head);
			Log_Message(Str);

			// issue command
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, After, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed go Update Analog Settings");
		}

		// if some part failed, then set whole head to fatal error
		if (Status != 0) Set_Head_Status(Head, BLOCK_OFFSET_FATAL);
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Coarse_Stage
// Purpose:		Coarsely sets the PMT gains
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Coarse_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long NumRemaining = 0;
	long CountThreshold = 50;
	long index = 0;
	long Gain = 0;
	long Percent = 0;
	long lld = 0;
	long uld = 0;

	long BlockSelect[MAX_BLOCKS];
	long Settings[MAX_SETTINGS*MAX_TOTAL_BLOCKS];
	long Singles[MAX_BLOCKS];

	long Flag[MAX_TOTAL_BLOCKS];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Coarse_Stage";

	HRESULT hr = S_OK;

	// initialize Stage Percentage
	sprintf(m_Stage, "Determining Coarse Gain Settings");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// initialize arrays
	for (i = 0 ; i < (MAX_TOTAL_BLOCKS) ; i++) Flag[i] = 0;

	// for each head
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// switch to bin mode
		Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// get Current analog settings
		if (Status == 0) {

			// log message
			sprintf(Str, "Head %d: Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get the original settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// if failed to get settings,then fatal error for each block
		if (Status != 0) {
			Set_Head_Status(Head, BLOCK_COARSE_FATAL);

		// success, add to heads/blocks to be processed
		} else {

			// increment number of heads being processed and the number of blocks to be processed
			NumHeads++;
			NumRemaining += NumBlocks;

			// set block to be processed
			for (Block = 0 ; Block < m_NumBlocks ; Block++) Flag[(Head*MAX_BLOCKS)+Block] = BlockSelect[Block];
		}
	}

	// Loop through possible gains
	for (Gain = m_MinGain ; (Gain <= m_MaxGain) && (NumHeads > 0) && (NumRemaining > 0) ; Gain += m_StepGain) {

		// Log Message
		sprintf(Str, "Remaining %d - Setting PMT Gains to %d", NumRemaining, Gain);
		Log_Message(Str);

		// for each head, set to new gain value and put into tube energy mode
		for (Head = 0 ; (Head < m_NumHeads) && (NumRemaining > 0) ; Head++) {

			// reset status for each head
			Status = 0;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks selected, then go on to next head
			if (NumBlocks == 0) continue;

			// index into Settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);

			// change gain for blocks
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {
				if ((BlockSelect[Block] == 1) && (Flag[(Head*MAX_BLOCKS)+Block] != 0)) {

					// log message
					sprintf(Str, "Setting Head %d Block %d Gain %d", Head, Block, Gain);
					Log_Message(Str);

					// set gain
					Settings[index+(Block*m_NumSettings)+SetIndex[PMTA]] = Gain;
					Settings[index+(Block*m_NumSettings)+SetIndex[PMTB]] = Gain;
					Settings[index+(Block*m_NumSettings)+SetIndex[PMTC]] = Gain;
					Settings[index+(Block*m_NumSettings)+SetIndex[PMTD]] = Gain;

					// also set the CFD to its starting value for HRRT (on first pass)
					if ((SCANNER_HRRT == m_ScannerType) && (Gain == m_MinGain)) {
						Settings[index+(Block*m_NumSettings)+SetIndex[CFD]] = m_CFDStart[Head];
					}
				}
			}

			// store send to head
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Set Gains");

			// put head in tube energy mode
			if (Status == 0) {

				// log message
				sprintf(Str, "Head %d: Tube Energy Mode", Head);
				Log_Message(Str);

				// issue command
				lld = m_AimPoint[Head] + 15;
				if (lld > 200) lld = 200;
				uld = m_AimPoint[Head] + 75;
				if (uld > 255) uld = 255;
				hr = pDHIThread->Set_Head_Mode(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, LAYER_ALL, ALL_BLOCKS, lld, uld, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Set_Head_Mode), HRESULT", hr);
				if (Status != 0) Log_Message("Failed Tube Energy Mode Command");
			}

			// if failed,then fatal error for each block
			if (Status != 0) Set_Head_Status(Head, BLOCK_COARSE_FATAL);
		}

		// wait for completion
		Percent = 0;
		Status = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDHIThread->Progress(WAIT_ALL_HEADS, &Percent, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Tube Energy Progress");
		}

		// wait an additional 5 seconds for singles to be accumulated
		if (NumRemaining > 0) Sleep(5000);

		// for each head, get the singles for each block and record maximum level
		for (Head = 0 ; (Head < m_NumHeads) && (NumRemaining > 0) ; Head++) {

			// reset status for each head
			Status = 0;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks selected, then go on to next head
			if (NumBlocks == 0) continue;

			// index for head
			index = Head * MAX_BLOCKS;

			// log message
			sprintf(Str, "Head %d: Retrieving Block Singles", Head);
			Log_Message(Str);

			// get singles
			hr = pDHIThread->Singles(Head, SINGLES_VALIDS, Singles, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Singles), HRESULT", hr);
			if (Status != 0) Log_Message("Singles Not Retrieved");

			// if the block is still being processed
			for (Block = 0 ; (Block < m_NumBlocks) && (NumRemaining > 0) && (Status == 0) ; Block++) {
				if ((BlockSelect[Block] == 1) && (Flag[index+Block] != 0)) {

					if (Singles[Block] > CountThreshold) {

						// if this is the second time, then accept it
						if (Flag[index+Block] == 2) {
							Flag[index+Block] = 0;
							NumRemaining--;

						// if it is the first time, then mark it
						} else {
							Flag[index+Block] = 2;
						}

					// it has not exceeded it yet
					} else {
						Flag[index+Block] = 1;
					}
				}
			}

			// if failed,then fatal error for each block
			if (Status != 0) Set_Head_Status(Head, BLOCK_COARSE_FATAL);
		}

		// update stage percentage (+1 used so it will top out < 100% set at end)
		m_StagePercent = ((Gain - m_MinGain) * 100) / (m_MaxGain - m_MinGain + 1);
	}

	// loop through blocks
	for (index = 0 ; index < MAX_TOTAL_BLOCKS ; index++) {

		// break out information
		Head = index / MAX_BLOCKS;
		Block = index % MAX_BLOCKS;

		// if the gain was not found
		if (Flag[index] != 0) {

			// set error
			m_SetupStatus[index] |= BLOCK_COARSE_NOT_FATAL;

			// error message for log
			sprintf(Str, "\t********** WARNING: Coarse Gain High For Head: %d Block %d", Head, Block);
			Log_Message(Str);
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Shape_Stage
// Purpose:		determines the shape discrimination threshold(s)
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Shape_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long Num_Acquire = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long index = 0;
	long BlocksHead = 0;
	long Percent = 0;
	long FileTime = 0;
	long DataSize = 0;
	long NumPeaks = 0;
	long NumValleys = 0;
	long pk1 = 0;
	long pk2 = 0;
	long Thresh = 0;
	long Slow_Low = 0;
	long Slow_High = 0;
	long Fast_Low = 0;
	long Fast_High = 0;

	long Peaks[256];
	long Valleys[256];

	long *Data = NULL;

	double Sum = 0.0;
	double EventThresh = 0.0;
	double Bins[256];
	double Portion[256];

	long BlockSelect[MAX_BLOCKS];
	long Settings[MAX_TOTAL_BLOCKS*MAX_SETTINGS];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Shape_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Determining Shape Discrimination Settings");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) {
		if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;
	}

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// for each head, get the original settings
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// switch to bin mode
		Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// get Current analog settings
		if (Status == 0) {

			// log message
			sprintf(Str, "Head %d: Retrieve Analog Settings", Head);
			Log_Message(Str);

			// get the original settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Settings");
		}

		// if failed to get settings,then fatal error for each block
		if (Status != 0) Set_Head_Status(Head, BLOCK_SHAPE_FATAL);
	}

	// loop through heads determining which heads need to be acquired
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// clear process flag
		m_AcquireSelect[Head] = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;
		
		// if two layers, data will need to be acquired
		if (m_NumLayers[Head] == 2) {
			m_AcquireSelect[Head] = 1;
			Num_Acquire++;

		// otherwise, set defaults
		} else {

			// log message
			sprintf(Str, "Head %d: Single Layer Head - Setting Default Values", Head);
			Log_Message(Str);

			// loop through the blocks
			for (Block = 0 ; Block < MAX_BLOCKS ; Block++) if (BlockSelect[Block] == 1) {

				// calculate index
				index = (Head * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

				// HRRT different from the others
				if (m_ScannerType == SCANNER_HRRT) {
					Settings[index+SetIndex[SHAPE]] = 1;

				// the others have more to set
				} else {
					Settings[index+SetIndex[SHAPE]] = 0;
					Settings[index+SetIndex[SLOW_LOW]] = 0;
					Settings[index+SetIndex[SLOW_HIGH]] = 0;
					Settings[index+SetIndex[FAST_LOW]] = 0;
					Settings[index+SetIndex[FAST_HIGH]] = 300;
				}
			}
		}
	}

	// if data is to be acquired
	if (Num_Acquire > 0) {

		// log message
		Log_Message("Acquire Shape Discrimination Data");

		// transfer variables for singles acquisition thread
		m_DataMode = DHI_MODE_SHAPE_DISCRIMINATION;
		for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = LAYER_ALL;
		m_AcqType = ACQUISITION_SECONDS;
		m_Amount = 60;

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect,
												m_AcquireLayer, m_AcqType, m_Amount, m_ShapeLLD, m_ShapeULD, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_SHAPE_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Progress Check");
			m_StagePercent = (Percent * 95) / 100;
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_SHAPE_FATAL);
			}
		}

		// process each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// get blocks in that head to be processed
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks, then continue to next head
			if (BlocksHead == 0) continue;

			// log message
			sprintf(Str, "Head %d: Retrieving Acquired Data", Head);
			Log_Message(Str);

			// retrieve the just acquired shape discrimination data
			hr = pDUThread->Current_Head_Data(DHI_MODE_SHAPE_DISCRIMINATION, m_Configuration, Head, LAYER_ALL, &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Log_Message("Failed To Retrieve Data");
				Set_Head_Status(Head, BLOCK_SHAPE_FATAL);
			}

			// re-check blocks in that head to be processed
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks, then continue to next head
			if (BlocksHead == 0) continue;

			// loop through the blocks
			for (i = 0 ; (i < MAX_BLOCKS) && (Status == 0) ; i++) {

				// index into global array
				Block = (Head * MAX_BLOCKS) + i;

				// if the block is to be processed
				if (BlockSelect[i] == 1) {

					// the bins for the block
					Smooth(3, 10, &Data[(i*256)], Bins);

					// determine the total number of events
					for (j = 0, Sum = 0 ; j < 256 ; j++) {
						Portion[j] = Sum;
						Sum += (double) Data[(i*256)+j];
					}

					// determine the threshold in number of events
					EventThresh = Sum * m_SlowPercent[Head];

					// find last bin less than event threshold
					for (Thresh = 0, j = 0 ; j < 256 ; j++) {
						if (Portion[j] < EventThresh) Thresh = j;
					}

					// could not determine threshold 
					if ((Thresh == 0) || (Thresh == 255)) {

						// log message
						sprintf(Str, "Head %d Block %d: WARNING Extreame Value For Threshold - Using Default", Head, i);
						Log_Message(Str);

						// use default
						switch (m_HeadType[Head]) {

						case HEAD_TYPE_LSO_LSO:
							Thresh = 180;
							break;

						case HEAD_TYPE_LSO_NAI:
							Thresh = 45;
							break;

						case HEAD_TYPE_GSO_LSO:
						case HEAD_TYPE_LYSO_LSO:
							Thresh = 165;
							break;

						case HEAD_TYPE_LSO_ONLY:
						default:
							Thresh = 1;
							break;
						}

						// non-fatal error
						m_SetupStatus[(Head*MAX_BLOCKS)+i] |= BLOCK_SHAPE_NOT_FATAL;
					}

					// find the peak(s) and valley(s)
					NumPeaks = Find_Peak_1d(Bins, Peaks);
					NumValleys = Find_Valley_1d(Bins, Valleys);

					// if the second peak is less than 5% of the biggest peak, there is only one peak
					if (NumPeaks > 1) if (Data[(i*256)+Peaks[1]] < (Data[(i*256)+Peaks[0]]/20)) NumPeaks = 1;

					// if two (or more) peaks were found, then the dip between them is the threshold
					if (NumPeaks >= 2) {

						// order peaks
						pk1 = (Peaks[1] > Peaks[0]) ? Peaks[0] : Peaks[1];
						pk2 = (Peaks[1] > Peaks[0]) ? Peaks[1] : Peaks[0];

						// the threshold found must be between these two peaks
						if ((pk1 < Thresh) && (Thresh < pk2)) {

							// find the deepest valley between the two peaks
							for (j = 0, Thresh = -1 ; (j < NumValleys) && (Thresh == -1) ; j++) {
								if ((Valleys[j] > pk1) && (Valleys[j] < pk2)) Thresh = Valleys[j];
							}

							// if no valley, then find lowest point
							if (Thresh == -1) {
								for (j = pk1, Thresh = pk1 ; j <= pk2 ; j++) {
									if (Bins[j] < Bins[Thresh]) Thresh = j;
								}
							}

							// calculate event layer windows
							if (m_ScannerType != SCANNER_HRRT) {
								for (Slow_Low = pk1 ; (Slow_Low > 0) && (Bins[Slow_Low] > (Bins[pk1] / 10.0)) ; Slow_Low--);
								for (Slow_High = pk1 ; (Slow_High < Thresh) && (Bins[Slow_High] > (Bins[pk1] / 10.0)) ; Slow_High++);
								for (Fast_Low = pk2 ; (Fast_Low > Thresh) && (Bins[Fast_Low] > (Bins[pk2] / 10.0)) ; Fast_Low--);
								for (Fast_High = pk2 ; (Fast_High < 255) && (Bins[Fast_High] > (Bins[pk2] / 10.0)) ; Fast_High++);
							}

						// if the two peaks did not bracket the threshold, then fall back on default (based on event threshold)
						} else {
							NumPeaks = 1;
						}
					}

					// otherwise event layer windows will be fixed distance from threshold
					if (NumPeaks < 2) {
						Slow_Low = Thresh - 25;
						if (Slow_Low < 0) Slow_Low = 0;
						Slow_High = Thresh;
						Fast_Low = Thresh;
						Fast_High = Thresh + 25;
						if (Fast_High > 255) Fast_High = 255;
					}

					// calculate the index
					index = (Head * MAX_BLOCKS * MAX_SETTINGS) + (i * m_NumSettings);

					// HRRT different from the others
					if (m_ScannerType == SCANNER_HRRT) {

						// set threshold
						Settings[index+SetIndex[SHAPE]] = (long)((100.0 * ((double) Thresh / 255.0)) + 0.5);

						// log message
						sprintf(Str, "Head %d Block %d: Threshold = %d", Head, i, Settings[index+SetIndex[SHAPE]]);
						Log_Message(Str);

					// the others have more to set
					} else {

						// set values
						Settings[index+SetIndex[SHAPE]] = (long)((300.0 * ((double) Thresh / 255.0)) + 0.5);
						Settings[index+SetIndex[SLOW_LOW]] = (long)((300.0 * ((double) Slow_Low / 255.0)) + 0.5);
						Settings[index+SetIndex[SLOW_HIGH]] = (long)((300.0 * ((double) Slow_High / 255.0)) + 0.5);
						Settings[index+SetIndex[FAST_LOW]] = (long)((300.0 * ((double) Fast_Low / 255.0)) + 0.5);
						Settings[index+SetIndex[FAST_HIGH]] = (long)((300.0 * ((double) Fast_High / 255.0)) + 0.5);

						// log message
						sprintf(Str, "Head %d Block %d: Threshold %d  Slow %d - %d Fast %d - %d", Head, i, Settings[index+SetIndex[SHAPE]], 
									Settings[index+SetIndex[SLOW_LOW]], Settings[index+SetIndex[SLOW_HIGH]], 
									Settings[index+SetIndex[FAST_LOW]], Settings[index+SetIndex[FAST_HIGH]]);
						Log_Message(Str);
					}
				}
			}

			// release the data buffer
			if (Data != NULL) CoTaskMemFree(Data);
		}
	}

	// for each head, set the settings
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// log message
		sprintf(Str, "Head %d: Setting Shape Values", Head);
		Log_Message(Str);

		// set the new settings
		index = Head * (MAX_BLOCKS * MAX_SETTINGS);
		hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
		if (Status != 0) Log_Message("Values Not Set");

		// if failed to get settings,then fatal error for each block
		if (Status != 0) Set_Head_Status(Head, BLOCK_SHAPE_FATAL);
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Fine_Stage
// Purpose:		finely adjust the pmt gains until the tube energy peaks align
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
//				22 Nov 04	T Gremillion	if not all blocks being setup and master not present, still store
//											current data as master.
//											use different size variable for master because it is zero if not there.
/////////////////////////////////////////////////////////////////////////////

void Fine_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long Tube = 0;
	long Peak = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long index = 0;
	long Percent = 0;
	long BlocksRemaining = 0;
	long Iteration = 0;
	long BlocksHead = 0;
	long DataSize = 0;
	long MasterSize = 0;
	long FileTime = 0;
	long Old_Gain = 0;
	long Close = 1;
	long CloseSum = 0;
	long Count = 0;
	long MinPeak = 0;

	long *Data = NULL;
	long *Master = NULL;

	long New_Gain[4];
	long CloseFlag[4];
	long BlockSelect[MAX_BLOCKS];
	long Settings[MAX_SETTINGS*MAX_TOTAL_BLOCKS];
	long BlocksLeft[MAX_TOTAL_BLOCKS];
	long Up[MAX_HEADS*MAX_BLOCKS*4];
	long Dn[MAX_HEADS*MAX_BLOCKS*4];

	double Ratio = 0.0;
	double Low = 0.0;
	double High = 0.0;

	double Bins[256];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Fine_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Determining Fine Gain Settings");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// initialize ranges
	for (i = 0 ; i < (MAX_HEADS*MAX_BLOCKS*4) ; i++) {
		Dn[i] = 0;
		Up[i] = 255;
	}

	// transfer variables for singles acquisition thread
	m_DataMode = DHI_MODE_TUBE_ENERGY;
	for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = m_FineGainLayer[i];
	m_AcqType = ACQUISITION_SECONDS;
	m_Amount = 60;

	// for each head, get the original settings
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// switch to bin mode
		Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// get Current analog settings
		if (Status == 0) {

			// log message
			sprintf(Str, "Head %d: Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get the original settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Settings");
		}

		// if failed to get settings,then fatal error for each block
		if (Status != 0) Set_Head_Status(Head, BLOCK_FINE_FATAL);
	}

	// determine the original number of blocks to be done
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
		if ((m_SetupSelect[i] != 0) && ((m_SetupStatus[i] & BLOCK_FATAL) != BLOCK_FATAL)) {
			BlocksLeft[i] = 1;
		} else {
			BlocksLeft[i] = 0;
		}
	}

	// loop through number of passes
	for (Iteration = 0 ; Iteration <= m_FineGainIterations ; Iteration++) {

		// update stage percentage
		m_StagePercent = (Iteration * 100) / (m_FineGainIterations+1);

		// update the closeness level;
		if (Iteration == m_FineGainIterations) Close = 10;

		// check on the number of blocks remaining
		for (BlocksRemaining = 0, i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
			if (BlocksLeft[i] == 1) {
				if ((m_SetupStatus[i] & BLOCK_FATAL) == BLOCK_FATAL) {
					BlocksLeft[i] = 0;
				} else {
					BlocksRemaining++;
				}
			}
		}

		// Log message
		sprintf(Str, "Iteration %d of %d - Blocks Remaining: %d", Iteration, m_FineGainIterations, BlocksRemaining);
		Log_Message(Str);

		// if there are no blocks left to set, go to next loop
		if (BlocksRemaining == 0) continue;

		// loop through heads putting them in tube energy mode
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// clear process flag
			m_AcquireSelect[Head] = 0;

			// how many blocks does head have left to set?
			for (Block = 0, BlocksHead = 0 ; (Block < m_NumBlocks) && (BlocksHead == 0) ; Block++) {
				if (BlocksLeft[(Head*MAX_BLOCKS)+Block] == 1) BlocksHead++;
			}

			// if there are blocks remaining, then take data from that head
			if (BlocksHead > 0) m_AcquireSelect[Head] = 1;
		}

		// log message
		Log_Message("Acquire Tube Energy Data");

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect, m_AcquireLayer, 
												m_AcqType, m_Amount, m_TubeEnergyLLD, m_TubeEnergyULD, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_FINE_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// bad status, fill acquisition status with value
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) m_AcquireStatus[Head] = Status;
			}
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_FINE_FATAL);
			}
		}

		// process each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] == 0)) {

			// clear the status for each head
			Status = 0;

			// set the minimum for each head
			MinPeak = m_AimPoint[Head] - 50;

			// index to start of settings for head
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);

			// log message
			sprintf(Str, "Head %d: Retrieving Tube Energy Data Just Acquired", Head);
			Log_Message(Str);

			// retrieve the just acquired tube energy data
			hr = pDUThread->Current_Head_Data(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSup, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Data = NULL;
				Log_Message("Data Not Retrieved");
				Set_Head_Status(Head, BLOCK_FINE_FATAL);
			}

			// re-check blocks in that head to be processed
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks, then continue to next head
			if (BlocksHead == 0) continue;

			// loop through the blocks
			for (i = 0 ; (i < MAX_BLOCKS) && (Status == 0) ; i++) {

				// index into global array
				Block = (Head * MAX_BLOCKS) + i;

				// if the block is still remaining
				if ((BlocksLeft[Block] == 1) && (BlockSelect[i] != 0)) {

					// log message
					sprintf(Str, "\tBlock %d:", i);
					Log_Message(Str);

					// reset closeness flags
					for (Tube = 0 ; Tube < 4 ; Tube++) CloseFlag[Tube] = 0;

					// for each tube
					for (Tube = 0 ; Tube < 4 ; Tube++) {

						// zero out the pileup bin (255)
						Data[(i*4*256)+(Tube*256)+255] = 0;

						// the bins for the tube
						Smooth(3, 10, &Data[(i*4*256)+(Tube*256)], Bins);

						// find bin with the most counts (+/- 10 bins)
						Peak = Peak_Most_Counts(Bins);

						// try to make sure the peak hasn't pushed off the high end (only for first 1/5) of iterations
						if (Iteration < (m_FineGainIterations / 5)) {

							// compare ratio of head to tail
							for (j = 0, Low = 0, High = 0 ; j <= 10 ; j++) {
								Low += Bins[80+j];
								High += Bins[240+j];
							}
							if ((10*High) > Low) Peak = 240;
						}

						// apply limits
						if (Peak < MinPeak) Peak = MinPeak;
						if (Peak > 240) Peak = 240;

						// check if close enough
						if (abs(Peak - m_AimPoint[Head]) <= Close) CloseFlag[Tube] = 1;

						// calculate new gain
						Ratio = ((((double) m_AimPoint[Head] / (double)Peak) - 1.0) * 0.4) + 1.0;
						Old_Gain = Settings[index+(i*m_NumSettings)+SetIndex[PMTA]+Tube];
						New_Gain[Tube] = (long)((Old_Gain * Ratio) + 0.5);

						// last 5 iterations, keep it from oscillating
						if (Iteration >= (m_FineGainIterations - 5)) {

							// compare against limits
							if (New_Gain[Tube] >= Up[(Head*MAX_BLOCKS*4)+(i*4)+Tube]) New_Gain[Tube] = (Old_Gain + Up[(Head*MAX_BLOCKS*4)+(i*4)+Tube]) / 2;
							if (New_Gain[Tube] <= Dn[(Head*MAX_BLOCKS*4)+(i*4)+Tube]) New_Gain[Tube] = (Old_Gain + Dn[(Head*MAX_BLOCKS*4)+(i*4)+Tube]) / 2;

							// New limits
							if ((Peak > m_AimPoint[Head]) && (Old_Gain < Up[(Head*MAX_BLOCKS*4)+(i*4)+Tube])) Up[(Head*MAX_BLOCKS*4)+(i*4)+Tube] = Old_Gain;
							if ((Peak < m_AimPoint[Head]) && (Old_Gain > Dn[(Head*MAX_BLOCKS*4)+(i*4)+Tube])) Dn[(Head*MAX_BLOCKS*4)+(i*4)+Tube] = Old_Gain;
						}

						// keep new value within absolute range
						if (New_Gain[Tube]  < 1) New_Gain[Tube]  = 1;
						if (New_Gain[Tube]  > 255) New_Gain[Tube]  = 255;

						// log message
						if (Iteration < m_FineGainIterations) {
							sprintf(Str, "\t   %s:\tOld: %3.1d\tNew: %3.1d\tPeak: %3.1d", Letter[Tube], Old_Gain, New_Gain[Tube], Peak);
						} else {
							sprintf(Str, "\t   %s:\tFinal Value: %3.1d", Letter[Tube], Old_Gain);
						}
						Log_Message(Str);
					}

					// add up the closeness flags
					for (Tube = 0, CloseSum = 0  ; Tube < 4 ; Tube++) CloseSum += CloseFlag[Tube];

					// if all four were within range, then block is done
					if (CloseSum == 4) {

						// remove from consideration
						BlocksLeft[Block] = 0;
						BlocksRemaining--;

						// log message
						sprintf(Str, "\t\tAll Tubes Converged - Removing From Loop");
						Log_Message(Str);

					// otherwise, if it is not the last iteration, apply changes
					} else if (Iteration < m_FineGainIterations) {
						for (Tube = 0 ; Tube < 4 ; Tube++) Settings[index+(i*m_NumSettings)+SetIndex[PMTA]+Tube] = New_Gain[Tube];
					}
				}
			}

			// release the memory
			if (Data != NULL) CoTaskMemFree(Data);

			// set the new settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);

			// failed to set the gains, log error message and set fatal error
			if (Status != 0) {
				Log_Message("Failed To Set Gains");
				Set_Head_Status(Head, BLOCK_FINE_NOT_FATAL);
			}
		}
	}

	// any blocks left, set as a non-fatal error
	for (Block = 0 ; Block < MAX_TOTAL_BLOCKS ; Block++) {
		if (BlocksLeft[Block] == 1) m_SetupStatus[Block] |= BLOCK_FINE_NOT_FATAL;
	}

	// loop through the heads making sure data was acquired
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// retrieve the last data acquired for the head
		hr = pDUThread->Current_Head_Data(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], &FileTime, &DataSize, &Data, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
		if (Status != 0) {
			Data = NULL;
			Log_Message("Last Data Not Retrieved");
			Set_Head_Status(Head, BLOCK_FINE_FATAL);
			continue;
		}

		// for each block
		for (Block = 0 ; Block < MAX_BLOCKS ; Block++) {

			// if it was selected
			if (BlockSelect[Block] == 1) {

				// check each tube
				for (Tube = 0 ; Tube < 4 ; Tube++) {

					// make sure there is data
					index = (Block * 4 * 256) + (Tube * 256);
					for (i = 0, Count = 0 ; i < 256 ; i++) Count +=Data[index+i];

					// if no counts, then fatal error
					if (Count == 0) {
						sprintf(Str, "Head %d Block %d Tube %d Had No Data", Head, Block, Tube);
						Log_Message(Str);
						m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_FINE_FATAL;
					}
				}
			}
		}
	}

	// loop through the heads making sure data was acquired
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// retrieve the last data acquired for the head
		hr = pDUThread->Current_Head_Data(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], &FileTime, &DataSize, &Data, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
		if (Status != 0) {
			Data = NULL;
			Log_Message("Last Data Not Retrieved");
			Set_Head_Status(Head, BLOCK_FINE_FATAL);
			continue;
		} else {
			sprintf(Str, "Head %d - Last Tube Energy Data Retrieved", Head);
			Log_Message(Str);
		}

		// if not all emission blocks being set up
		if (m_HeadBlocks[Head] < m_EmissionBlocks) {

			// try to retrieve master data
			hr = pDUThread->Read_Master(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], &MasterSize, &Master, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Read_Master), HRESULT", hr);
			if (Status != 0) {
				Master = NULL;
				Log_Message("Master Data Not Retrieved");
			}

			// do not fail setup on lack of previous master tube energy data
			Status = 0;

		// all emission blocks being set up, master not needed
		} else {
			Master = NULL;
		}

		// for each block
		for (Block = 0 ; Block < m_EmissionBlocks ; Block++) {

			// if it was not selected and master tube energy is available
			if ((BlockSelect[Block] == 0) && (Master != NULL)) {

				// transfer the data
				index = Block * 4 * 256;
				for (i = 0 ; i < (4 * 256) ; i++) Data[index+i] = Master[index+i];
			}
		}

		// save master tube energy data
		hr = pDUThread->Write_Master(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], DataSize, &Data, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Write_Master), HRESULT", hr);
		if (Status != 0) {
			sprintf(Str, "Head %d - Master Tube Energy Not Stored - ERROR", Head);
			Log_Message(Str);
			Set_Head_Status(Head, BLOCK_FINE_FATAL);
		}

		// release the memory
		if (Data != NULL) CoTaskMemFree(Data);
		if (Master != NULL) CoTaskMemFree(Master);
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Tune_Stage
// Purpose:		finely adjusts the pmt gains until tube energy peaks match
//				those of previously taken master data
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Tune_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long Tube = 0;
	long Peak = 0;
	long MasterPeak = 0;
	long NumPeaks = 0;
	long NumMasterPeaks = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long index = 0;
	long Percent = 0;
	long BlocksRemaining = 0;
	long Iteration = 0;
	long BlocksHead = 0;
	long DataSize = 0;
	long FileTime = 0;
	long Old_Gain = 0;
	long Close = 0;
	long CloseSum = 0;
	long Count = 0;
	long MinPeak = 0;

	long *Data = NULL;
	long *Master = NULL;

	long MasterPeaks[256];
	long New_Gain[4];
	long CloseFlag[4];
	long BlockSelect[MAX_BLOCKS];
	long Settings[MAX_SETTINGS*MAX_TOTAL_BLOCKS];
	long BlocksLeft[MAX_TOTAL_BLOCKS];
	long Last[MAX_HEADS*MAX_BLOCKS*4];

	double Ratio = 0.0;
	double Low = 0.0;
	double High = 0.0;

	double Bins[256];
	double MasterBins[256];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Tune_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Adjusting Tube Gain Settings");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// transfer variables for singles acquisition thread
	m_DataMode = DHI_MODE_TUBE_ENERGY;
	for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = m_FineGainLayer[i];
	m_AcqType = ACQUISITION_SECONDS;
	m_Amount = 60;

	// for each head, get the original settings
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// switch to bin mode
		Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// get Current analog settings
		if (Status == 0) {

			// log message
			sprintf(Str, "Head %d: Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get the original settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
		}

		// if failed to get settings,then fatal error for each block
		if (Status != 0) {
			Set_Head_Status(Head, BLOCK_TUNE_FATAL);
			Log_Message("Failed to Retrieve Analog Settings");
		}
	}

	// determine the original number of blocks to be done
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
		if ((m_SetupSelect[i] != 0) && ((m_SetupStatus[i] & BLOCK_FATAL) != BLOCK_FATAL)) {
			BlocksLeft[i] = 1;
		} else {
			BlocksLeft[i] = 0;
		}
	}

	// loop through number of passes
	for (Iteration = 0 ; Iteration <= m_TuneGainIterations ; Iteration++) {

		// update stage percentage
		m_StagePercent = (Iteration * 100) / (m_TuneGainIterations+1);

		// update the closeness level;
		if (Iteration == m_TuneGainIterations) Close = 3;

		// check on the number of blocks remaining
		for (BlocksRemaining = 0, i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
			if (BlocksLeft[i] == 1) {
				if ((m_SetupStatus[i] & BLOCK_FATAL) == BLOCK_FATAL) {
					BlocksLeft[i] = 0;
				} else {
					BlocksRemaining++;
				}
			}
		}

		// Log message
		sprintf(Str, "Iteration %d of %d - Blocks Remaining: %d", Iteration, m_TuneGainIterations, BlocksRemaining);
		Log_Message(Str);

		// if there are no blocks left to set, go to next loop
		if (BlocksRemaining == 0) continue;

		// loop through heads putting them in tube energy mode
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// clear process flag
			m_AcquireSelect[Head] = 0;

			// how many blocks does head have left to set?
			for (Block = 0, BlocksHead = 0 ; (Block < m_NumBlocks) && (BlocksHead == 0) ; Block++) {
				if (BlocksLeft[(Head*MAX_BLOCKS)+Block] == 1) BlocksHead++;
			}

			// if there are blocks remaining, then take data from that head
			if (BlocksHead > 0) m_AcquireSelect[Head] = 1;
		}

		// log message
		Log_Message("Acquire Tube Energy Data");

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect, m_AcquireLayer, 
												m_AcqType, m_Amount, m_TubeEnergyLLD, m_TubeEnergyULD, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_TUNE_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_TUNE_FATAL);
			}
		}

		// process each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] == 0)) {

			// set the minimum for each head
			MinPeak = m_AimPoint[Head] - 50;

			// index to start of settings for head
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);

			// log message
			sprintf(Str, "Head %d: Retrieve Tube Energy Data Just Acquired", Head);
			Log_Message(Str);

			// retrieve the just acquired tube energy data
			hr = pDUThread->Current_Head_Data(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Data = NULL;
				Log_Message("Failed To Retrieve Tube Energy Data");
				Set_Head_Status(Head, BLOCK_TUNE_FATAL);
			}

			// re-check blocks in that head to be processed
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks, then continue to next head
			if (BlocksHead == 0) continue;

			// retrieve the master tube energy data for this head
			if (Status == 0) {

				// log message
				sprintf(Str, "Head %d: Retrieving Master Tube Energy Data", Head);
				Log_Message(Str);

				// retrieve data
				hr = pDUThread->Read_Master(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], &DataSize, &Master, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Read_Master), HRESULT", hr);
				if (Status != 0) {
					Master = NULL;
					Set_Head_Status(Head, BLOCK_TUNE_FATAL);
					Log_Message("Failed To Retrieve Master Tube Energy Data");
				}
			}

			// re-check blocks in that head to be processed
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks, then continue to next head
			if (BlocksHead == 0) continue;

			// loop through the blocks
			for (i = 0 ; (i < MAX_BLOCKS) && (Status == 0) ; i++) {

				// index into global array
				Block = (Head * MAX_BLOCKS) + i;

				// if the block is still remaining
				if (BlocksLeft[Block] == 1) {

					// log message
					sprintf(Str, "\tBlock %d", i);
					Log_Message(Str);

					// reset closeness flags
					for (Tube = 0 ; Tube < 4 ; Tube++) CloseFlag[Tube] = 0;

					// for each tube
					for (Tube = 0 ; Tube < 4 ; Tube++) {

						// zero out the pileup bin (255)
						Data[(i*4*256)+(Tube*256)+255] = 0;
						Master[(i*4*256)+(Tube*256)+255] = 0;

						// the bins for the tube
						Smooth(3, 10, &Data[(i*4*256)+(Tube*256)], Bins);
						Smooth(3, 10, &Master[(i*4*256)+(Tube*256)], MasterBins);

						// find the peak(s)
						NumMasterPeaks = Find_Peak_1d(MasterBins, MasterPeaks);

						// No peaks found, set it to the bottom of the range
						if (NumMasterPeaks == 0) {
							MasterPeak = MinPeak;

						// otherwise, peaks are sorted largest first
						} else {
							MasterPeak = MasterPeaks[0];
						}

						// apply limits
						if (MasterPeak < MinPeak) MasterPeak = MinPeak;
						if (MasterPeak > 240) MasterPeak = 240;

						// determine gains
						Old_Gain = Settings[index+(i*m_NumSettings)+SetIndex[PMTA]+Tube];
						New_Gain[Tube] = Determine_Gain(Old_Gain, MasterPeak, Bins, MasterBins);

						// check if close enough
						if (abs(New_Gain[Tube] - Old_Gain) <= Close) CloseFlag[Tube] = 1;

						// last 3 iterations, keep it from oscillating
						if (Iteration >= (m_TuneGainIterations - 3)) {

							// if new gain is above a value already known to be too high
							if ((Last[(Head*MAX_BLOCKS*4)+(i*4)+Tube] > Old_Gain) && (New_Gain[Tube] >= Last[(Head*MAX_BLOCKS*4)+(i*4)+Tube])) {
								New_Gain[Tube] = (Last[(Head*MAX_BLOCKS*4)+(i*4)+Tube] + Old_Gain) / 2;

							// if new gain is below a value already known to be too low
							} else if ((Last[(Head*MAX_BLOCKS*4)+(i*4)+Tube] < Old_Gain) && (New_Gain[Tube] <= Last[(Head*MAX_BLOCKS*4)+(i*4)+Tube])) {
								New_Gain[Tube] = (Last[(Head*MAX_BLOCKS*4)+(i*4)+Tube] + Old_Gain) / 2;
							}
						}

						// keep track of the last value
						Last[(Head*MAX_BLOCKS*4)+(i*4)+Tube] = Old_Gain;

						// log message
						sprintf(Str, "\t\t%s:\tOld: %3.1d\t\tNew: %3.1d", Letter[Tube], Old_Gain, New_Gain[Tube]);
						Log_Message(Str);
					}

					// add up the closeness flags
					for (Tube = 0, CloseSum = 0  ; Tube < 4 ; Tube++) CloseSum += CloseFlag[Tube];

					// if all four were within range, then block is done
					if (CloseSum == 4) {

						// remove from loop
						BlocksLeft[Block] = 0;
						BlocksRemaining--;

						// log message
						sprintf(Str, "\t\tAll Tubes Converged - Removing From Loop");
						Log_Message(Str);

					// otherwise, if it is not the last iteration, apply changes
					} else if (Iteration < m_TuneGainIterations) {
						for (Tube = 0 ; Tube < 4 ; Tube++) Settings[index+(i*m_NumSettings)+SetIndex[PMTA]+Tube] = New_Gain[Tube];
					}
				}
			}

			// release the memory
			if (Data != NULL) CoTaskMemFree(Data);
			if (Master != NULL) CoTaskMemFree(Master);

			// set the new settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Set Gains");
		}
	}

	// any blocks left, set as a non-fatal error
	for (Block = 0 ; Block < MAX_TOTAL_BLOCKS ; Block++) {
		if (BlocksLeft[Block] == 1) m_SetupStatus[Block] |= BLOCK_TUNE_NOT_FATAL;
	}

	// loop through the heads making sure data was acquired
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// retrieve the last data acquired for the head
		hr = pDUThread->Current_Head_Data(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_FineGainLayer[Head], &FileTime, &DataSize, &Data, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
		if (Status != 0) {
			Data = NULL;
			Log_Message("Last Data Not Retrieved");
			Set_Head_Status(Head, BLOCK_FINE_FATAL);
			continue;
		}

		// for each block
		for (Block = 0 ; Block < MAX_BLOCKS ; Block++) {

			// if it was selected
			if (BlockSelect[Block] == 1) {

				// check each tube
				for (Tube = 0 ; Tube < 4 ; Tube++) {

					// make sure there is data
					index = (Block * 4 * 256) + (Tube * 256);
					for (i = 0, Count = 0 ; i < 256 ; i++) Count +=Data[index+i];

					// if no counts, then fatal error
					if (Count == 0) {
						sprintf(Str, "Block %d Tube %d Had No Data", Block, Tube);
						Log_Message(Str);
						m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_TUNE_FATAL;
					}
				}
			}
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		CRM_Stage
// Purpose:		generates Crystal Region Maps for selected blocks
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
//				22 Nov 04	T Gremillion	if not all blocks being setup and previous positions not present,
//											store default pattern for all
/////////////////////////////////////////////////////////////////////////////

void CRM_Stage()
{
	// local variables
	long i = 0;
	long Head = 0;
	long Block = 0;
	long BlocksHead = 0;
	long Num_Acquire = 0;
	long NumHeads = 0;
	long Percent = 0;
	long Status = 0;
	long Layer = 0;
	long LocationLayer = 0;
	long FileTime = 0;
	long DataSize = 0;
	long index = 0;
	long NumPixels = 0;
	long high = 0;
	long high_x = 0;
	long high_y = 0;
	long x = 0;
	long y = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long VertsBlock = 0;
	long Select = 0;
	long PerHead = 0;
	long HeadProcess = 0;
	long MasterFound = 1;
	long Count = 0;

	long *Data = NULL;

	long Position[MAX_HEAD_VERTICES*2];
	long Master[MAX_HEAD_VERTICES*2];
	long BlockSelect[MAX_BLOCKS];
	long Region[65536];
	long Verts[2178];				// 1089 = 33 * 33, 33 = (16 * 2) + 1, 16 = Maximum Crystals in row or column
	long NewVerts[2178];

	long Cent_X[9] = {104, 128, 152, 152, 152, 128, 104, 104, 104};
	long Cent_Y[9] = {104, 104, 104, 128, 152, 152, 152, 128, 104};

	double BestFit;

	double Fit[16];
	double pp[65536];

	unsigned char CRM[PP_SIZE];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "CRM_Stage";

	HRESULT hr = S_OK;

	// Start Stage
	sprintf(m_Stage, "Generating Crystal Region Maps");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// clear all positions
	for (i = 0 ; i < (MAX_HEAD_VERTICES*2) ; i++) Position[i] = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// Stage percent per head processed
	PerHead = 70 / NumHeads;

	// number of verticies per block
	Verts_X = (2 * m_XCrystals) + 1;
	Verts_Y = (2 * m_YCrystals) + 1;
	VertsBlock = Verts_X * Verts_Y;

	// Multiple layers
	for (Layer = 0 ; Layer < NUM_LAYER_TYPES ; Layer++) {

		// HRRT and P39 Phase IIA are combined layer only
		if (((m_ScannerType == SCANNER_HRRT) || (m_ScannerType == SCANNER_P39_IIA)) && (Layer != LAYER_ALL)) continue;

		// HRRT and P39 Phase IIA are only combined layer
		if ((m_ScannerType != SCANNER_HRRT) && (m_ScannerType != SCANNER_P39_IIA) && (Layer == LAYER_ALL)) continue;

		// PETSPECT does not do fast layer for SPECT
		if ((m_ScannerType == SCANNER_PETSPECT) && (Layer == LAYER_FAST) && (m_Configuration != 0)) continue;

		// loop through the heads determining which ones need to be acquired
		for (Num_Acquire = 0, Head = 0 ;  Head < MAX_HEADS ; Head++) {

			// Clear Acquire Flag
			m_AcquireSelect[Head] = 0;

			// slow layer not done for single layer heads
			if ((m_NumLayers[Head] == 1) && (Layer == LAYER_SLOW)) continue;

			// check if any blocks are to be prcoessed
			if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) {
				Num_Acquire++;
				m_AcquireSelect[Head] = 1;
				m_AcquireStatus[Head] = 0;
			}
		}

		// if no data is to be acquired, then skip loop
		if (Num_Acquire == 0) continue;

		// log message
		sprintf(Str, "Layer: %s", StrLayer[Layer]);
		Log_Message(Str);

		// transfer variables for singles acquisition thread
		m_DataMode = DHI_MODE_POSITION_PROFILE;
		for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = Layer;
		m_AcqType = ACQUISITION_SECONDS;
		m_Amount = 300;

		// log message
		Log_Message("Acquiring Position Profile Data");
		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect, m_AcquireLayer, 
												m_AcqType, m_Amount, m_PositionProfileLLD, m_PositionProfileULD, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_CRM_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
			m_StagePercent = (25 * Percent) / 100;
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_CRM_FATAL);
			}
		}

		// Process each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// if no blocks selected, then skip to next head
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
			if (BlocksHead == 0) continue;

			// stage percentage
			m_StagePercent = 25 + (PerHead * HeadProcess);

			// log message
			sprintf(Str, "Head %d: Retrieving Position Profile Data", Head);
			Log_Message(Str);

			// designate the location layer (HRRT stores CRMs in different place than data)
			LocationLayer = Layer;
			if (m_ScannerType == SCANNER_HRRT) LocationLayer = LAYER_FAST;

			// retrieve the just acquired position profile data
			hr = pDUThread->Current_Head_Data(DHI_MODE_POSITION_PROFILE, m_Configuration, Head, Layer, &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);

			// fail head if data not retrieved
			if (Status != 0) {
				Log_Message("Failed to Retrieve Position Profile Data");
				Set_Head_Status(Head, BLOCK_CRM_FATAL);
			}

			// re-evaluate blocks selected, then skip to next head
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
			if (BlocksHead == 0) continue;

			// retrieve the Master positions
			if (Status == 0) {

				// log message
				sprintf(Str, "Head %d: Retrieving Master Crystal Locations", Head);
				Log_Message(Str);

				// retrieve current positions
				hr = pDUThread->Get_Master_Crystal_Location(m_Configuration, Head, Layer, Master, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Get_Master_Crystal_Location), HRESULT", hr);

				// if failed to retrieve the positions, use a standard pattern for a master
				if (Status != 0) {

					// switch flag
					MasterFound = 0;

					// log message
					Log_Message("Failed To Retrieve Master Crystal Locations - Using Default Pattern");

					// generate values
					Standard_CRM_Pattern(m_EmissionBlocks, m_XCrystals, m_YCrystals, Master);
					Status = 0;
				}

				// set flag in profile processing routines
				SetMasterFlag(MasterFound);
			}

			// retrieve the crystal positions
			if ((Status == 0) && (m_HeadBlocks[Head] != m_EmissionBlocks)) {

				// log message
				sprintf(Str, "Head %d: Retrieving Current Crystal Locations", Head);
				Log_Message(Str);

				// retrieve current positions
				hr = pDUThread->Get_Stored_Crystal_Position(m_Configuration, Head, LocationLayer, Position, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Get_Stored_Crystal_Position), HRESULT", hr);

				// if failed to retrieve the positions
				if (Status != 0) {
					Log_Message("Failed To Retrieve Current Locations - Using Default Pattern");

					// generate values
					Standard_CRM_Pattern(m_EmissionBlocks, m_XCrystals, m_YCrystals, Position);
					Status = 0;
				}
			}

			// loop through the blocks
			for (Block = 0 ; (Block < m_EmissionBlocks) && (Status == 0) ; Block++) if (BlockSelect[Block] == 1) {

				// stage percentage
				m_StagePercent = 25 + (PerHead * HeadProcess) + ((Block * PerHead) / m_NumBlocks);

				// log message
				sprintf(Str, "\tBlock %d: Generating Crystal Region Map", Block);
				Log_Message(Str);

				// number of counts
				index = Block * PP_SIZE;
				for (i = 0, Count = 0 ; i < PP_SIZE ; i++) Count +=Data[index+i];

				// if no counts, then fatal error
				if (Count == 0) {
					sprintf(Str, "Block %d Had No Data", Block);
					Log_Message(Str);
					m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_CRM_FATAL;
					continue;
				}

				// transfer verticies to local array from master
				for (i = 0 ; i < (2 * VertsBlock) ; i++) Verts[i] = Master[(Block*VertsBlock*2)+i];

				// fill in the non-peak verticies
				Fill_CRM_Verticies(4, m_XCrystals, m_YCrystals, Verts);

				// smooth the data
				Smooth_2d(0, 3, 3, &Data[index], pp);

				// define the central region
				NumPixels = Find_Region(Cent_X, Cent_Y, Region);

				// find the highest point in the central region
				high = region_high(pp, NumPixels, Region);
				high_x = high % PP_SIZE_X;
				high_y = high / PP_SIZE_X;

				// try each of the central 4 x 4 peaks as the high point found to see which is the best fit
				for (i = 0 ; i < 16 ; i++) {

					// peak coordinates
					x = (2 * ((i % 4) + (m_XCrystals / 2) - 2)) + 1;
					y = (2 * ((i / 4) + (m_YCrystals / 2) - 2)) + 1;

					// determine fit
					Fit[i] = Shifted_Peaks(pp, Verts, high_x, high_y, x, y, m_XCrystals, m_YCrystals, NewVerts);
				}

				// which had the best fit (smallest variance)
				for (Select = 0, BestFit = Fit[0], i = 1 ; i < 16 ; i++) if (Fit[i] < BestFit) {
					BestFit = Fit[i];
					Select = i;
				}

				// Get the verticies for the Selected peak
				x = (2 * ((Select % 4) + (m_XCrystals / 2) - 2)) + 1;
				y = (2 * ((Select / 4) + (m_YCrystals / 2) - 2)) + 1;
				BestFit = Shifted_Peaks(pp, Verts, high_x, high_y, x, y, m_XCrystals, m_YCrystals, NewVerts);

				// if the block is on the bottom row, it is safe to take the bottom edge out to 5 pixels from edge
				if ((Block / m_XBlocks) == 0) {
					for (i = 0 ; i < Verts_X ; i++) {
						index = (i * 2) + 1;
						if (NewVerts[index] > 5) NewVerts[index] = 5;
					}
				}

				// if the block is on the top row, it is safe to take the bottom edge out to 5 pixels from edge
				if ((Block / m_XBlocks) == (m_YBlocks-1)) {
					for (i = 0 ; i < Verts_X ; i++) {
						index = ((((Verts_Y-1) * Verts_X) + i) * 2) + 1;
						if (NewVerts[index] < (PP_SIZE_Y - 5)) NewVerts[index] = (PP_SIZE_Y - 5);
					}
				}

				// if the block is on the left edge, it is safe to take the bottom edge out to 5 pixels from edge
				if ((Block % m_XBlocks) == 0) {
					for (i = 0 ; i < Verts_Y ; i++) {
						index = i * Verts_X * 2;
						if (NewVerts[index] > 5) NewVerts[index] = 5;
					}
				}

				// if the block is on the right edge, it is safe to take the bottom edge out to 5 pixels from edge
				if ((Block % m_XBlocks) == (m_XBlocks-1)) {
					for (i = 0 ; i < Verts_Y ; i++) {
						index = ((i+1) * Verts_X * 2) - 2;
						if (NewVerts[index] < (PP_SIZE_X - 5)) NewVerts[index] = (PP_SIZE_X - 5);
					}
				}

				// build the CRM
				Build_CRM(NewVerts, m_XCrystals, m_YCrystals, CRM);

				// log message
				sprintf(Str, "\tBlock %d: Storing Crystal Region Map", Block);
				Log_Message(Str);

				// write the CRM
				hr = pDUThread->Store_CRM(m_Configuration, Head, LocationLayer, Block, CRM, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_CRM), HRESULT", hr);
				if (Status != 0) {
					m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_CRM_FATAL;
					sprintf(Str, "\tFailed to store Crystal Region Map");
					Log_Message(Str);
				}

				// transfer verticies to head array
				for (i = 0 ; i < (2 * VertsBlock) ; i++) Position[(Block*VertsBlock*2)+i] = NewVerts[i];
			}

			// increment head processed
			HeadProcess++;

			// Store the crystal positions
			if (Status == 0) {

				// log message
				sprintf(Str, "Head %d %s Storing Crystal Locations", Head, StrLayer[LocationLayer]);
				Log_Message(Str);

				// issue command
				hr = pDUThread->Store_Crystal_Position(m_Configuration, Head, LocationLayer, Position, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Crystal_Position), HRESULT", hr);

				// fail head if crystral locations not stored
				if (Status != 0) {
					Log_Message("Failed to Store Crystal Locations");
					Set_Head_Status(Head, BLOCK_CRM_FATAL);
				}
			}

			// release the memory
			if (Data != NULL) CoTaskMemFree(Data);
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		CFD_Stage
// Purpose:		determines the constant fraction discriminator for selected blocks
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	if CFD value ends up at either extreame,
//											then set it to default value.
//				22 Nov 04	T Gremillion	if not all blocks being setup and previous positions not present,
//											store default pattern for all
/////////////////////////////////////////////////////////////////////////////

void CFD_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long Tube = 0;
	long Peak = 0;
	long NumPeaks = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long index = 0;
	long Percent = 0;
	long Step = 0;
	long Iteration = 0;
	long BlocksHead = 0;
	long DataSize = 0;
	long FileTime = 0;
	long EnergyPeak = 0;
	long Cutoff = 0;
	long Counts = 0;
	long MinValue = 0;
	long MaxValue = 0;

	long *Data = NULL;

	long Bins[256];
	long Peaks[256];
	long BlockSelect[MAX_BLOCKS];
	long Settings[MAX_SETTINGS*MAX_TOTAL_BLOCKS];

	double Body = 0.0;
	double Threshold = 0.0;
	double CFD_Total = 0.0;

	double dBins[256];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "CFD_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Determining CFD Settings");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// for each head, get the original settings
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// switch to bin mode
		Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// get Current analog settings
		if (Status == 0) {

			// log message
			sprintf(Str, "Head %d: Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get the original settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// if failed to get settings,then fatal error for each block
		if (Status != 0) Set_Head_Status(Head, BLOCK_CFD_FATAL);

		// log message
		sprintf(Str, "Head %d: Setting CFD to Starting Value: %d", Head, m_CFDStart[Head]);
		Log_Message(Str);

		// set to initial value
		for (Block = 0 ; (Block < m_NumBlocks) && (Status == 0) ; Block++) {
			if (BlockSelect[Block] != 0) {
				Settings[index+(Block*m_NumSettings)+SetIndex[CFD]] = m_CFDStart[Head];
			}
		}
	}

	// loop through number of passes
	for (Iteration = m_CFDIterations-1 ; Iteration >= -1 ; Iteration--) {

		// Calculate the step
		Step = (Iteration > 0) ? ((long) pow(2.0, (double) Iteration)) : 1;

		// log message
		sprintf(Str, "Iteration %d of %d - Step %d", (m_CFDIterations-Iteration-1), m_CFDIterations, Step);
		Log_Message(Str);

		// update stage percentage
		m_StagePercent = ((m_CFDIterations - Iteration - 1) * 100) / (m_CFDIterations+1);

		// loop through heads updating the settings and putting them in crystal energy mode
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// clear process flag
			m_AcquireSelect[Head] = 0;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
			if (NumBlocks == 0) continue;

			// set the new settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
			if (Status != 0) {
				sprintf(Str, "Head %d: Failed To Set Values");
				Log_Message(Str);
			}

			// mark to acquire
			if (Status == 0) m_AcquireSelect[Head] = 1;
		}

		// transfer variables for singles acquisition thread
		m_DataMode = DHI_MODE_CRYSTAL_ENERGY;
		for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = m_CFDLayer[i];
		m_AcqType = ACQUISITION_SECONDS;
		m_Amount = 60;
		m_lld = 1;
		m_uld = 255;

		// log message
		Log_Message("Acquiring Crystal Energy Data");

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect, m_AcquireLayer, 
												m_AcqType, m_Amount, m_lld, m_uld, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_CFD_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_CFD_FATAL);
			}
		}

		// process each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] == 0)) {

			// index to start of settings for head
			index = Head * MAX_BLOCKS * m_NumSettings;

			// get block flags
			NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
			if (NumBlocks == 0) continue;

			// log message
			sprintf(Str, "Head %d: Retrieving Crystal Energy Data - Layer %s", Head, StrLayer[m_CFDLayer[Head]]);
			Log_Message(Str);

			// retrieve the just acquired crystal energy data
			hr = pDUThread->Current_Head_Data(DHI_MODE_CRYSTAL_ENERGY, m_Configuration, Head, m_CFDLayer[Head], &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Log_Message("Failed to Retrieve Data");
				Data = NULL;
				Set_Head_Status(Head, BLOCK_CFD_FATAL);
			}

			// re-evaluate blocks selected, then skip to next head
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
			if (BlocksHead == 0) continue;

			// loop through the blocks
			for (i = 0 ; (i < MAX_BLOCKS) && (Status == 0) ; i++) if (BlockSelect[i] == 1) {

				// compress all the crystals (except edge) to one histogram
				for (j = 0, Counts = 0 ; j < 256 ; j++) {
					Bins[j] = 0;
					for (k = 0 ; k < MAX_CRYSTALS ; k++) {
						if (((k % 16) >= 0) && ((k % 16) <= (m_XCrystals-1)) && 
							((k / 16) >= 0) && ((k / 16) <= (m_YCrystals-1))) {
							Bins[j] += Data[(i*MAX_CRYSTALS*256)+(k*256)+j];
							Counts += Data[(i*MAX_CRYSTALS*256)+(k*256)+j];
						}
					}
				}

				// zero out the pileup bin
				Bins[255] = 0;

				// if no data
				if (Counts == 0) {
					sprintf(Str, "Block %d Had No Data", i);
					Log_Message(Str);
					m_SetupStatus[(Head*MAX_BLOCKS)+i] |= BLOCK_CFD_FATAL;
					continue;
				}

				// Find peaks
				Smooth(3, 50, Bins, dBins);
				NumPeaks = Find_Peak_1d(dBins, Peaks);

				// determine energy peak
				switch (NumPeaks) {

				case 0:
					EnergyPeak = m_AimPoint[Head];
					break;

				case 1:
					EnergyPeak = Peaks[0];
					break;

				default:
					if ((Peaks[0] < 100) && (Peaks[1] >= 100)) {
						EnergyPeak = Peaks[1];
					} else {
						EnergyPeak = Peaks[0];
					}
					break;
				}

				// calculate the cutoff as a bin number
				Cutoff = EnergyPeak / 2;

				// Calculate the threshold
				for (Body = 0.0, j = (Cutoff+1) ; j < 256 ; j++) Body += (double)Bins[j];
				Threshold = (double) Body * m_CFDRatio;

				// total up all counts below the cutoff energy
				for (CFD_Total = 0.0, j = 0 ; j <= Cutoff ; j++) CFD_Total += (double)Bins[j];

				// if the cutoff bit has too many counts, then increment the counter
				index = (Head * MAX_BLOCKS * MAX_SETTINGS) + (i * m_NumSettings);
				if (CFD_Total > Threshold) {
					Settings[index+SetIndex[CFD]] += Step;
				} else {
					if (Iteration != -1) Settings[index+SetIndex[CFD]] -= Step;
				}

				// calculate the extreame values and set back to default if outside
				MaxValue = m_CFDStart[Head] + (long) pow(2.0, (double) m_CFDIterations) - 1;
				MinValue = m_CFDStart[Head] - (long) pow(2.0, (double) m_CFDIterations) + 1;
				if ((Settings[index+SetIndex[CFD]] >= MaxValue) || (Settings[index+SetIndex[CFD]] <= MinValue)) {
					Settings[index+SetIndex[CFD]] = m_CFDStart[Head];
					Log_Message("CFD Limits Exceeded");
					m_SetupStatus[(Head*MAX_BLOCKS)+i] |= BLOCK_CFD_NOT_FATAL;
				}

				// log message
				sprintf(Str, "\tBlock %d: New Value = %d", i, Settings[index+SetIndex[CFD]]);
				Log_Message(Str);
			}

			// release the memory
			if (Data != NULL) CoTaskMemFree(Data);

			// set the new settings from the last iteration
			if (Iteration == -1) {
				index = Head * (MAX_BLOCKS * MAX_SETTINGS);
				hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
				if (Status != 0) {
					sprintf(Str, "Head %d: Failed to Set Values", Head);
					Log_Message(Str);
				}
			}
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Peak_Stage
// Purpose:		Determines the crystal energy peak for each crystal of selected blocks
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Peak_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long Head = 0;
	long Block = 0;
	long Num_Acquire = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long index = 0;
	long BlocksHead = 0;
	long Percent = 0;
	long FileTime = 0;
	long DataSize = 0;
	long NumPeaks = 0;
	long Layer = 0;
	long Value = 0;
	long MinRange = 0;
	long Status = 0;
	long Counts = 0;

	long x[4] = {0, (m_XCrystals-1), 0, (m_XCrystals-1)};
	long x1[4] = {1, (m_XCrystals-2), 1, (m_XCrystals-2)};
	long x2[4] = {0, (m_XCrystals-1), 0, (m_XCrystals-1)};
	long y[4] = {0, 0, (m_YCrystals-1), (m_YCrystals-1)};
	long y1[4] = {0, 0, (m_YCrystals-1), (m_YCrystals-1)};
	long y2[4] = {1, 1, (m_YCrystals-2), (m_YCrystals-2)};

	long Peaks[256];

	long *Data = NULL;

	double Bins[256];

	long CrystalPeaks[MAX_BLOCKS*MAX_CRYSTALS];
	long BlockSelect[MAX_BLOCKS];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Peak_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Determining Crystal Energy Peak Values");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	for (Layer = 0 ; Layer < NUM_LAYER_TYPES ; Layer++) {

		// stage percent
		m_StagePercent = (Layer * 100) / NUM_LAYER_TYPES;

		// combined layer only used for P39 phase IIA
		if ((Layer == LAYER_ALL) && (m_ScannerType != SCANNER_P39_IIA)) continue;
		if ((Layer != LAYER_ALL) && (m_ScannerType == SCANNER_P39_IIA)) continue;

		// No Fast Layer for SPECT
		if ((m_ScannerType == SCANNER_PETSPECT) && (m_Configuration != 0) && (Layer != LAYER_SLOW)) continue;

		// loop through heads determining which heads need to be acquired
		for (Num_Acquire = 0, Head = 0 ; Head < MAX_HEADS ; Head++) {

			// clear process flag
			m_AcquireSelect[Head] = 0;

			// if single layer, slow layer not allowed
			if ((m_NumLayers[Head] == 1) && (Layer == LAYER_SLOW)) continue;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if there are blocks remaining, acquire the head
			if (NumBlocks > 0) {
				m_AcquireSelect[Head] = 1;
				Num_Acquire++;
			}
		}

		// if no data is to be acquired the skip this layer
		if (Num_Acquire == 0) continue;

		// log message
		sprintf(Str, "Acquiring Crystal Energy Data For Layer %s", StrLayer[Layer]);
		Log_Message(Str);

		// transfer variables for singles acquisition thread
		m_DataMode = DHI_MODE_CRYSTAL_ENERGY;
		for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = Layer;
		m_AcqType = ACQUISITION_SECONDS;
		m_Amount = 300;

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect, m_AcquireLayer, 
												m_AcqType, m_Amount, m_CrystalEnergyLLD, m_CrystalEnergyULD, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_CRYSTAL_ENERGY_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		for (Head = 0 ; Head < MAX_HEADS ; Head++) m_AcquireStatus[Head] = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_CRYSTAL_ENERGY_FATAL);
			}
		}

		// process each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// reset status for each head
			Status = 0;

			// get blocks in that head to be processed
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks, then continue to next head
			if (BlocksHead == 0) continue;

			// log message
			sprintf(Str, "Head %d: Retrieving Crystal Energy Data", Head);
			Log_Message(Str);

			// retrieve the just acquired crystal energy data
			hr = pDUThread->Current_Head_Data(DHI_MODE_CRYSTAL_ENERGY, m_Configuration, Head, Layer, &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Log_Message("Failed to Retrieve Crystal Energy Data");
				Set_Head_Status(Head, BLOCK_CRYSTAL_ENERGY_FATAL);
			}

			// re-evaluate blocks selected, then skip to next head
			BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
			if (BlocksHead == 0) continue;

			// if not all the crystal peaks are being generated, fetch the current values otherwise clear them
			if (Status == 0) {
				if (m_HeadBlocks[Head] < m_EmissionBlocks) {

					// log message
					sprintf(Str, "Head %d: Retrieving Crystal Energy Peaks", Head);
					Log_Message(Str);

					// retrieve data
					hr = pDUThread->Get_Stored_Energy_Peaks(m_Configuration, Head, Layer, CrystalPeaks, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Get_Stored_Energy_Peaks), HRESULT", hr);
					if (Status != 0) {

						// put message in log
						Log_Message("Failed to Retrieve Crystal Energy Peaks - Using Default Value");

						// set values to tube aim point
						for (Block = 0 ; Block < m_NumBlocks ; Block++) {
							for (j = 1 ; j < (m_YCrystals-1) ; j++) {
								for (i = 1 ; i < (m_XCrystals-1) ; i++) {
									index = (Block * MAX_CRYSTALS) + (j * 16) + i;
									CrystalPeaks[index] = m_AimPoint[Head];
								}
							}
						}


						// clear status
						Status = 0;
					}
				} else {
					for (i = 0 ; i < (MAX_BLOCKS*MAX_CRYSTALS) ; i++) CrystalPeaks[i] = 0;
				}
			}

			// loop through the blocks
			for (Block = 0 ; (Block < m_NumBlocks) && (Status == 0) ; Block++) {

				// if the block is selected
				if (BlockSelect[Block] == 1) {

					// log message
					sprintf(Str, "\tProcessing Block %d", Block);
					Log_Message(Str);

					// compress all the crystals (except edge) to one histogram
					for (i = 0, Counts = 0 ; i < 256 ; i++) {
						for (j = 0 ; j < MAX_CRYSTALS ; j++) {
							if (((j % 16) >= 0) && ((j % 16) <= (m_XCrystals-1)) && 
								((j / 16) >= 0) && ((j / 16) <= (m_YCrystals-1))) {
								Counts += Data[(Block*MAX_CRYSTALS*256)+(j*256)+i];
							}
						}
					}

					// if no data
					if (Counts == 0) {
						sprintf(Str, "Block %d Had No Data", Block);
						Log_Message(Str);
						m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_CRYSTAL_ENERGY_FATAL;
						continue;
					}

					// loop through the crystals (central region)
					for (j = 1 ; j < (m_YCrystals-1) ; j++) for (i = 1 ; i < (m_XCrystals-1) ; i++) {

						// isolate crystal data
						index = ((Block * MAX_CRYSTALS) + (j * 16) + i) * 256;
						Data[index+255] = 0;
						Smooth(3, 10, &Data[index], Bins);

						// find the peak(s)
						NumPeaks = Find_Peak_1d(Bins, Peaks);

						// act on number of peaks found
						switch (NumPeaks) {

						// no peaks found, kill the crystal
						case 0:
							Value = 0;
							break;

						// only one peak found, use it
						case 1:
							Value = Peaks[0];
							break;

						// more than one peak found
						default:

							// if the biggest peak was < 100 (high potential for being CFD) 
							// and the second highest peak isn't, then used second highest peak
							if ((Peaks[0] < 100) && (Peaks[1] >= 100)) {
								Value = Peaks[1];

							// otherwise, use highest peak
							} else {
								Value = Peaks[0];
							}
							break;
						}

						// insert into array
						index = (Block * MAX_CRYSTALS) + (j * 16) + i;
						CrystalPeaks[index] = Value;
					}

					// loop through the crystals rows
					for (j = 0 ; j < m_YCrystals ; j++) {

						// loop through crystal columns
						for (i = 0 ; i < m_XCrystals ; i++) {

							// skip if not an edge
							if ((i != 0) && (i != (m_XCrystals-1)) && (j != 0) && (j != (m_YCrystals-1))) continue;

							// isolate crystal data
							index = ((Block * MAX_CRYSTALS) + (j * 16) + i) * 256;
							Data[index+255] = 0;
							Smooth(3, 10, &Data[index], Bins);

							// determine the peak that will contribute
							if (i == 0) {
								index = (Block * MAX_CRYSTALS) + (j * 16) + i + 1;
							} else if (i == (m_XCrystals-1)) {
								index = (Block * MAX_CRYSTALS) + (j * 16) + i - 1;
							} else if (j == 0) {
								index = (Block * MAX_CRYSTALS) + ((j+1) * 16) + i;
							} else if (j == (m_YCrystals-1)) {
								index = (Block * MAX_CRYSTALS) + ((j-1) * 16) + i;
							}
							MinRange = CrystalPeaks[index] - DELTA_CRYSTAL_PEAK;
							if (MinRange < 0) MinRange = 0;

							// find the highest peak within the range
							for (k = MinRange, Value = k ; k < 256 ; k++) {
								if (Bins[k] > Bins[Value]) Value = k;
							}

							// insert into array
							index = (Block * MAX_CRYSTALS) + (j * 16) + i;
							CrystalPeaks[index] = Value;
						}
					}

					// loop through the corners
					for (i = 0 ; i < 4 ; i++) {

						// isolate crystal data
						index = ((Block * MAX_CRYSTALS) + (y[i] * 16) + x[i]) * 256;
						Data[index+255] = 0;
						Smooth(3, 10, &Data[index], Bins);

						// neighboring X pixel
						index = (Block * MAX_CRYSTALS) + (y1[i] * 16) + x1[i];
						MinRange = CrystalPeaks[index];
						index = (Block * MAX_CRYSTALS) + (y2[i] * 16) + x2[i];
						if (CrystalPeaks[index] < MinRange) MinRange = CrystalPeaks[index];
						MinRange = CrystalPeaks[index] - DELTA_CRYSTAL_PEAK;
						if (MinRange < 0) MinRange = 0;

						// find the highest peak within the range
						for (k = MinRange, Value = k ; k < 256 ; k++) {
							if (Bins[k] > Bins[Value]) Value = k;
						}

						// insert into array
						index = (Block * MAX_CRYSTALS) + (y[i] * 16) + x[i];
						CrystalPeaks[index] = Value;
					}
				}
			}

			// release the data array
			if (Data != NULL) CoTaskMemFree(Data);

			// store the new values
			if (Status == 0) {

				// log message
				sprintf(Str, "Head %d: Storing & Setting Crystal Energy Peaks", Head);
				Log_Message(Str);

				// store values
				hr = pDUThread->Store_Energy_Peaks(SEND, m_Configuration, Head, Layer, CrystalPeaks, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Energy_Peaks), HRESULT", hr);
				if (Status != 0) Log_Message("Failed to Store Crystal Energy Peaks");
			}

			// set fatal error if there was a bad status
			if (Status != 0) Set_Head_Status(Head, BLOCK_CRYSTAL_ENERGY_FATAL);
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Run_Stage
// Purpose:		Acquires a run mode (flood) image for quality check
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Run_Stage()
{
	// local variables
	long i = 0;
	long Status = 0;
	long Head = 0;
	long Num_Acquire = 0;
	long Block = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long BlocksHead = 0;
	long Percent = 0;
	long Layer = 0;

	long BlockSelect[MAX_BLOCKS];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Run_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Acquiring Run Mode Data");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// loop through heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) {
		
		// if head being processed
		if (Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect) > 0) {

			// log message
			sprintf(Str, "Head %d: Loading Crystal Energy Peaks", Head);
			Log_Message(Str);

			// load the crystal peaks
			Status = 0;
			hr = pDHIThread->Load_RAM(m_Configuration, Head, RAM_ENERGY_PEAKS, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Load_RAM), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Load Crystal Energy Peaks");

			// switch to correct energy
			if (Status == 0) Status = Set_Setup_Energy(m_Configuration, Head, m_ConfigurationEnergy[m_Configuration]);

			// increment the number of heads
			NumHeads++;
		}
	}

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	for (Layer = 0 ; Layer < NUM_LAYER_TYPES ; Layer++) {

		// stage percent
		m_StagePercent = (Layer * 100) / NUM_LAYER_TYPES;

		// HRRT & P39 is only combined data
		if ((m_ScannerType != SCANNER_PETSPECT) && (Layer != LAYER_ALL)) continue;

		// PETSPECT is separate layers
		if ((m_ScannerType == SCANNER_PETSPECT) && (Layer == LAYER_ALL)) continue;

		// PETSPECT only takes slow data for SPECT
		if ((m_ScannerType == SCANNER_PETSPECT) && (m_Configuration != 0) && (Layer != LAYER_SLOW)) continue;

		// loop through heads determining which heads need to be acquired
		for (Num_Acquire = 0, Head = 0 ; Head < MAX_HEADS ; Head++) {

			// clear process flag
			m_AcquireSelect[Head] = 0;

			// if single layer, slow layer not allowed
			if ((m_NumLayers[Head] == 1) && (Layer == LAYER_SLOW)) continue;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

			// if there are blocks remaining, acquire the head
			if (NumBlocks > 0) {
				m_AcquireSelect[Head] = 1;
				Num_Acquire++;
			}
		}

		// if no data is to be acquired the skip this layer
		if (Num_Acquire == 0) continue;

		// log message
		sprintf(Str, "Acquiring Run Mode Data For Layer %s", StrLayer[Layer]);
		Log_Message(Str);

		// transfer variables for singles acquisition thread
		m_DataMode = DHI_MODE_RUN;
		for (Head = 0 ; Head < MAX_HEADS ; Head++) m_AcquireLayer[Head] = Layer;
		m_AcqType = ACQUISITION_SECONDS;
		m_Amount = 60;
		m_lld = m_ConfigurationLLD[m_Configuration];
		m_uld = m_ConfigurationULD[m_Configuration];

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect, 
												m_AcquireLayer, m_AcqType, m_Amount, m_lld, m_uld, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_RUN_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_RUN_FATAL);
			}
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Expand_Stage
// Purpose:		Expands the borders of the CRMs until the edge crystals have
//				the same number of counts as the center ones
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Expand_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long data_index = 0;
	long x = 0;
	long x1 = 0;
	long x2 = 0;
	long y = 0;
	long y1 = 0;
	long y2 = 0;
	long Status = 0;
	long Head = 0;
	long Num_Acquire = 0;
	long Block = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long BlocksHead = 0;
	long Percent = 0;
	long Layer = 0;
	long LocationLayer = 0;
	long NumRemaining = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long VertsBlock = 0;
	long FileTime = 0;
	long DataSize = 0;
	long row = 0;
	long col = 0;
	long NumSide = 0;
	long vx = 0;
	long vy = 0;
	long LoadCRM = 0;
	long Download_Error = 0;
	long Iteration = 0;

	long *Data = NULL;

	long Position[MAX_HEAD_VERTICES*2];
	long Change[64*MAX_BLOCKS*MAX_HEADS];
	long Download[MAX_HEADS*MAX_BLOCKS];
	long DownStatus[MAX_HEADS*MAX_BLOCKS];
	long BlockSelect[MAX_BLOCKS];
	long Vert[2178];				// 2178 = 2 * 33 * 33, 33 = (16 * 2) + 1, 16 = Maximum Crystals in row or column

	double Mean = 0.0;
	double Threshold = 0.0;

	unsigned char CRM[PP_SIZE];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Expand_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Expanding CRM Edges");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// number of verticies per block
	Verts_X = (2 * m_XCrystals) + 1;
	Verts_Y = (2 * m_YCrystals) + 1;
	VertsBlock = Verts_X * Verts_Y;

	// loop through heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) {
		
		// if head being processed
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
		if (NumBlocks > 0) {

			// log message
			sprintf(Str, "Head %d: Loading Crystal Energy Peaks", Head);
			Log_Message(Str);

			// load the crystal peaks
			Status = 0;
			hr = pDHIThread->Load_RAM(m_Configuration, Head, RAM_ENERGY_PEAKS, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Load_RAM), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Load Crystal Energy Peaks");

			// switch to correct energy
			if (Status == 0) Status = Set_Setup_Energy(m_Configuration, Head, m_ConfigurationEnergy[m_Configuration]);

			// increment the number of heads
			if (Status == 0) {
				NumHeads++;

			// bad status encountered
			} else {
				Set_Head_Status(Head, BLOCK_EXPAND_FATAL);
			}
		}
	}

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	for (Layer = 0 ; Layer < NUM_LAYER_TYPES ; Layer++) {

		// stage percentage
		m_StagePercent = (Layer * 100) / NUM_LAYER_TYPES;

		// HRRT is only combined data
		if (((m_ScannerType == SCANNER_HRRT) || (m_ScannerType == SCANNER_P39_IIA)) && (Layer != LAYER_ALL)) continue;

		// combined data not taken if not HRRT
		if ((m_ScannerType != SCANNER_HRRT) && (m_ScannerType != SCANNER_P39_IIA) && (Layer == LAYER_ALL)) continue;

		// PETSPECT only takes slow data for SPECT
		if ((m_ScannerType == SCANNER_PETSPECT) && (m_Configuration != 0) && (Layer != LAYER_SLOW)) continue;

		// designate the location layer (HRRT stores CRMs in different place than data)
		LocationLayer = Layer;
		if (m_ScannerType == SCANNER_HRRT) LocationLayer = LAYER_FAST;

		// number of blocks to process
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// if single layer, slow layer not allowed
			if ((m_NumLayers[Head] == 1) && (Layer == LAYER_SLOW)) continue;

			// initializing number of blocks remaining
			NumRemaining += Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
		}

		// initialize to allow change
		for (i = 0 ; i < (64*MAX_BLOCKS*MAX_HEADS) ; i++) Change[i] = 1;

		// loop while there are blocks remaining (but no more than 10 times)
		while ((NumRemaining > 0) && (Iteration < m_MaxExpand)) {

			// update Percentage
			m_StagePercent = (Iteration * 100) / (m_MaxExpand + 1);

			// clear the number remaining and increment the iteration number
			NumRemaining = 0;
			Iteration++;

			// log message
			sprintf(Str, "Expanding Crystal Region Maps - Iteration %d", Iteration);
			Log_Message(Str);

			// loop through heads determining which heads need to be acquired
			for (Num_Acquire = 0, Head = 0 ; Head < MAX_HEADS ; Head++) {

				// clear process flag
				m_AcquireSelect[Head] = 0;

				// if single layer, slow layer not allowed
				if ((m_NumLayers[Head] == 1) && (Layer == LAYER_SLOW)) continue;

				// Determine the number of blocks from the head are to be set up
				NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

				// if there are blocks remaining, acquire the head
				if (NumBlocks > 0) {
					m_AcquireSelect[Head] = 1;
					Num_Acquire++;
				}
			}

			// if no data is to be acquired the skip this layer
			if (Num_Acquire == 0) continue;

			// log messages
			sprintf(Str, "Acquire Run Mode Data - Layer %s", StrLayer[Layer]);
			Log_Message(Str);

			// transfer variables for singles acquisition thread
			m_DataMode = DHI_MODE_RUN;
			for (Head = 0 ; Head < MAX_HEADS ; Head++) m_AcquireLayer[Head] = Layer;
			m_AcqType = ACQUISITION_SECONDS;
			m_Amount = 60;
			m_lld = m_ConfigurationLLD[m_Configuration];
			m_uld = m_ConfigurationULD[m_Configuration];

			// start up acquisition
			hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_AcquireSelect, m_BlankSelect, 
													m_AcquireLayer, m_AcqType, m_Amount, m_lld, m_uld, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
			if (Status != 0) {
				for (Head = 0 ; Head < MAX_HEADS ; Head++) {
					if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_EXPAND_FATAL);
				}
				Log_Message("Failed To Start Acquisition");
			}

			// poll for progress of acquisition
			Percent = 0;
			while ((Percent < 100) && (Status == 0)) {
				hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
				if (Percent < 100) Sleep(1000);
				if (Status != 0) Log_Message("Failed Acquisition Progress");
			}

			// do status for each head
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
					sprintf(Str, "Head %d: Acquisition Failed", Head);
					Log_Message(Str);
					Set_Head_Status(Head, BLOCK_EXPAND_FATAL);
				}
			}

			// clear download flags
			for (i = 0 ; i < (MAX_HEADS*MAX_BLOCKS) ; i++) {
				Download[i] = 0;
				DownStatus[i] = 0;
			}

			// Process each head
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {

				// reset status for each head
				Status = 0;

				// get blocks in that head to be processed
				BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

				// if no blocks, then continue to next head
				if (BlocksHead == 0) continue;

				// log message
				sprintf(Str, "Head %d %s: Retrieving Run Mode Data", Head, StrLayer[Layer]);
				Log_Message(Str);

				// retrieve the just acquired run mode data
				hr = pDUThread->Current_Head_Data(DHI_MODE_RUN, m_Configuration, Head, Layer, &FileTime, &DataSize, &Data, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
				if (Status != 0) {
					Log_Message("Failed to Retrieve Run Mode Data");
					Set_Head_Status(Head, BLOCK_EXPAND_FATAL);
				}

				// re-evaluate blocks selected, then skip to next head
				BlocksHead = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);
				if (BlocksHead == 0) continue;

				// if successful, retrieve the peak locations
				if (Status == 0) {

					// log message
					sprintf(Str, "Head %d %s: Retrieving Crystal Positions", Head, StrLayer[Layer]);
					Log_Message(Str);

					// retrieve positions
					hr = pDUThread->Get_Stored_Crystal_Position(m_Configuration, Head, LocationLayer, Position, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Get_Stored_Crystal_Position), HRESULT", hr);
					if (Status != 0) Log_Message("Failed to Retrieve Positons");
				}

				// if successful, process the blocks
				if (Status == 0) {

					// loop through the blocks
					for (Block = 0 ; Block < MAX_BLOCKS ; Block++) {

						// if the block was selected and is remaining
						if ((BlockSelect[Block] != 0) && Remaining(Head, Block, Change)) {

							// log message
							sprintf(Str, "\tBlock %d", Block);
							Log_Message(Str);

							// increment the number remaining
							NumRemaining++;

							// this CRM at lease will be downloaded
							LoadCRM = 1;
							Download[(Head*MAX_BLOCKS)+Block] = 1;

							// transfer verticies to local array
							for (i = 0 ; i < (2 * VertsBlock) ; i++) Vert[i] = Position[(Block*VertsBlock*2)+i];

							// define central part of block
							x1 = ((Block % m_XBlocks) * m_XCrystals) + 1;
							x2 = x1 + m_XCrystals - 3;
							y1 = ((Block / m_XBlocks) * m_YCrystals) + 1;
							y2 = y1 + m_YCrystals - 3;

							// mean of the central part
							for (Mean = 0.0, y = y1 ; y <= y2 ; y++) for (x = x1 ; x <= x2 ; x++) {
								index = (y * 128) + x;
								Mean += Data[index];
							}
							Threshold = Mean / ((double) ((m_XCrystals - 2) * (m_YCrystals - 2)));

							// if no data, then fatal error
							if (Threshold == 0) {
								sprintf(Str, "Block %d Had No Data", Block);
								Log_Message(Str);
								m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_EXPAND_FATAL;
								continue;
							}

							// loop through the Bottom crystals
							NumSide = 0;
							for (x = 0, y = 0 ; x < m_XCrystals ; x++) {

								// if changes are still allowed
								index = (((Head*MAX_BLOCKS) + Block) * 64) + x;
								if (Change[index] == 1) {

									vx = (x * 2) + 1;
									vy = 0;

									// if the crystal value is above the mean of the center, 
									// then back it off one and stop processing that crystal
									data_index = ((((Block / m_XBlocks) * m_YCrystals) + y) * 128) +
												   ((Block % m_XBlocks) * m_XCrystals) + x;
									if (Data[data_index] > Threshold) {

										// increment the y vertex (moves toward center of block)
										Vert[(((vy*Verts_X)+vx)*2)+1]++;

										// changes no longer allowed on this crystal
										Change[index] = 0;

									// otherwise, push it out one
									} else {

										// decrement the y vertex (moves toward edge of block)
										if (Iteration < m_MaxExpand) Vert[(((vy*Verts_X)+vx)*2)+1]--;

										// don't let too close to edge (within 5 pixels)
										if (Vert[(((vy*Verts_X)+vx)*2)+1] < 5) {
											Vert[(((vy*Verts_X)+vx)*2)+1] = 5;
											Change[index] = 0;
										}
									}
								}

								// sum up number on this side
								NumSide += Change[index];
							}

							// log message
							sprintf(Str, "\t\tBottom: %d", NumSide);
							Log_Message(Str);

							// if half or more have stopped, then stop them all on this side
							if (NumSide <= (m_XCrystals/2)) {
								for (x = 0, y = 0 ; x < m_XCrystals ; x++) {
									index = (((Head*MAX_BLOCKS) + Block) * 64) + x;
									Change[index] = 0;
								}
							}

							// loop through the Top crystals
							NumSide = 0;
							for (x = 0, y = (m_YCrystals-1) ; x < m_XCrystals ; x++) {

								// if changes are still allowed
								index = (((Head*MAX_BLOCKS) + Block) * 64) + m_XCrystals + x;
								if (Change[index] == 1) {

									vx = (x * 2) + 1;
									vy = (y * 2) + 2;

									// if the crystal value is above the mean of the center, 
									// then back it off one and stop processing that crystal
									data_index = ((((Block / m_XBlocks) * m_YCrystals) + y) * 128) +
												   ((Block % m_XBlocks) * m_XCrystals) + x;
									if (Data[data_index] > Threshold) {

										// decrement the x vertex (moves toward center of block)
										Vert[(((vy*Verts_X)+vx)*2)+1]--;

										// changes no longer allowed on this crystal
										Change[index] = 0;

									// otherwise, push it out one
									} else {

										// increment the x vertex (moves toward edge of block)
										if (Iteration < m_MaxExpand) Vert[(((vy*Verts_X)+vx)*2)+1]++;

										// don't let too close to edge (within 5 pixels)
										if (Vert[(((vy*Verts_X)+vx)*2)+1] > (PP_SIZE_Y - 5)) {
											Vert[(((vy*Verts_X)+vx)*2)+1] = PP_SIZE_Y - 5;
											Change[index] = 0;
										}
									}
								}

								// sum up number on this side
								NumSide += Change[index];
							}

							// log message
							sprintf(Str, "\t\tTop: %d", NumSide);
							Log_Message(Str);

							// if half or more have stopped, then stop them all on this side
							if (NumSide <= (m_XCrystals/2)) {
								for (x = 0 ; x < m_XCrystals ; x++) {
									index = (((Head*MAX_BLOCKS) + Block) * 64) + m_XCrystals + x;
									Change[index] = 0;
								}
							}

							// loop through the Left crystals
							NumSide = 0;
							for (x = 0, y = 0 ; y < m_YCrystals ; y++) {

								// if changes are still allowed
								index = (((Head*MAX_BLOCKS) + Block) * 64) + (2*m_XCrystals) + y;
								if (Change[index] == 1) {

									vx = 0;
									vy = (y * 2) + 1;

									// if the crystal value is above the mean of the center, 
									// then back it off one and stop processing that crystal
									data_index = ((((Block / m_XBlocks) * m_YCrystals) + y) * 128) +
												   ((Block % m_XBlocks) * m_XCrystals) + x;
									if (Data[data_index] > Threshold) {

										// increment the x vertex (moves toward center of block)
										Vert[(((vy*Verts_X)+vx)*2)]++;

										// changes no longer allowed on this crystal
										Change[index] = 0;

									// otherwise, push it out one
									} else {

										// decrement the x vertex (moves toward edge of block)
										if (Iteration < m_MaxExpand) Vert[(((vy*Verts_X)+vx)*2)]--;

										// don't let too close to edge (within 5 pixels)
										if (Vert[(((vy*Verts_X)+vx)*2)] < 5) {
											Vert[(((vy*Verts_X)+vx)*2)] = 5;
											Change[index] = 0;
										}
									}
								}

								// sum up number on this side
								NumSide += Change[index];
							}

							// log message
							sprintf(Str, "\t\tLeft: %d", NumSide);
							Log_Message(Str);

							// if half or more have stopped, then stop them all on this side
							if (NumSide <= (m_XCrystals/2)) {
								for (y = 0 ; y < m_YCrystals ; y++) {
									index = (((Head*MAX_BLOCKS) + Block) * 64) + (2*m_XCrystals) + y;
									Change[index] = 0;
								}
							}

							// loop through the Right crystals
							NumSide = 0;
							for (x = (m_XCrystals-1), y = 0 ; y < m_YCrystals ; y++) {

								// if changes are still allowed
								index = (((Head*MAX_BLOCKS) + Block) * 64) + (2*m_XCrystals) + m_YCrystals + y;
								if (Change[index] == 1) {

									vx = (x * 2) + 2;
									vy = (y * 2) + 1;

									// if the crystal value is above the mean of the center, 
									// then back it off one and stop processing that crystal
									data_index = ((((Block / m_XBlocks) * m_YCrystals) + y) * 128) +
												   ((Block % m_XBlocks) * m_XCrystals) + x;
									if (Data[data_index] > Threshold) {

										// decrement the x vertex (moves toward center of block)
										Vert[(((vy*Verts_X)+vx)*2)]--;

										// changes no longer allowed on this crystal
										Change[index] = 0;

									// otherwise, push it out one
									} else {

										// increment the x vertex (moves toward edge of block)
										if (Iteration < m_MaxExpand) Vert[(((vy*Verts_X)+vx)*2)]++;

										// don't let too close to edge (within 5 pixels)
										if (Vert[(((vy*Verts_X)+vx)*2)] > (PP_SIZE_X - 5)) {
											Vert[(((vy*Verts_X)+vx)*2)] = PP_SIZE_X - 5;
											Change[index] = 0;
										}
									}
								}

								// sum up number on this side
								NumSide += Change[index];
							}

							// log message
							sprintf(Str, "\t\tRight: %d", NumSide);
							Log_Message(Str);

							// if half or more have stopped, then stop them all on this side
							if (NumSide <= (m_XCrystals/2)) {
								for (y = 0 ; y < m_YCrystals ; y++) {
									index = (((Head*MAX_BLOCKS) + Block) * 64) + (2*m_XCrystals) + m_YCrystals + y;
									Change[index] = 0;
								}
							}

							// fill in the other edge vertices
							// left and right edges
							row = 2 * Verts_X;
							for (y = 2 ; y <= (Verts_Y-3) ; y += 2) {

								// left
								index = y * Verts_X * 2;
								Vert[index+0] = (Vert[index-row+0] + Vert[index+row+0]) / 2;
								Vert[index+1] = (Vert[index-row+1] + Vert[index+row+1]) / 2;

								// right
								index = (y * Verts_X * 2) + (Verts_X * 2) - 2;
								Vert[index+0] = (Vert[index-row+0] + Vert[index+row+0]) / 2;
								Vert[index+1] = (Vert[index-row+1] + Vert[index+row+1]) / 2;
							}

							// top and bottom
							for (x = 2 ; x <= (Verts_X-3) ; x += 2) {

								// top
								index = ((Verts_Y - 1) * Verts_X * 2) + (x * 2);
								Vert[index+0] = (Vert[index-2] + Vert[index+2]) / 2;
								Vert[index+1] = (Vert[index-1] + Vert[index+3]) / 2;

								// bottom
								index = x * 2;
								Vert[index+0] = (Vert[index-2] + Vert[index+2]) / 2;
								Vert[index+1] = (Vert[index-1] + Vert[index+3]) / 2;
							}

							// bottom left corner
							index = 0;
							Vert[index+0] = (Vert[index+2] + (3*Vert[index+(2*Verts_X)+0])) / 4;
							Vert[index+1] = ((3*Vert[index+3]) + Vert[index+(2*Verts_X)+1]) / 4;

							// bottom right corner
							index = 2 * Verts_X - 2;
							Vert[index+0] = (Vert[index-2] + (3*Vert[index+(2*Verts_X)+0])) / 4;
							Vert[index+1] = ((3*Vert[index-1]) + Vert[index+(2*Verts_X)+1]) / 4;

							// top left corner
							index = (Verts_Y - 1) * Verts_X * 2;
							Vert[index+0] = (Vert[index+2] + (3*Vert[index-(2*Verts_X)+0])) / 4;
							Vert[index+1] = ((3*Vert[index+3]) + Vert[index-(2*Verts_X)+1]) / 4;

							// top right corner
							index = (VertsBlock * 2) - 2;
							Vert[index+0] = (Vert[index-2] + (3*Vert[index-(2*Verts_X)+0])) / 4;
							Vert[index+1] = ((3*Vert[index-1]) + Vert[index-(2*Verts_X)+1]) / 4;

							// build the CRM
							Build_CRM(Vert, m_XCrystals, m_YCrystals, CRM);

							// log message
							sprintf(Str, "\tBlock %d: Storing Crystal Region Map", Block);
							Log_Message(Str);

							// write the CRM
							hr = pDUThread->Store_CRM(m_Configuration, Head, LocationLayer, Block, CRM, &Status);
							if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_CRM), HRESULT", hr);
							if (Status != 0) Log_Message("Failed to Store CRM");

							// transfer verticies to head array
							if (Status == 0) {
								for (i = 0 ; i < (2 * VertsBlock) ; i++) Position[(Block*VertsBlock*2)+i] = Vert[i];
							} else {
								m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_EXPAND_FATAL;
							}
						}
					}

					// log message
					sprintf(Str, "Head %d %s: Storing Crystal Positions", Head, StrLayer[Layer]);
					Log_Message(Str);

					// store peak locations
					hr = pDUThread->Store_Crystal_Position(m_Configuration, Head, LocationLayer, Position, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Crystal_Position), HRESULT", hr);
					if (Status != 0) Log_Message("Failed to Store Peak Locations");
				}

				// bad status
				if (Status != 0) Set_Head_Status(Head, BLOCK_EXPAND_FATAL);

				// release the data array
				if (Data != NULL) CoTaskMemFree(Data);
			}

			// Download the CRMs in preparation for the next pass
			if (LoadCRM == 1) {

				// log message
				Log_Message("Downloading Crystal Region Maps");

				// download
				Download_Error = 0;
				hr = pDUThread->Download_CRM(m_Configuration, Download, DownStatus);
				if (FAILED(hr)) {
					Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Download_CRM), HRESULT", hr);
					for (i = 0 ; i < (MAX_HEADS*MAX_BLOCKS) ; i++) DownStatus[i] = Status;
				}
				for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
					if (DownStatus[i] != 0) {
						m_SetupStatus[i] |= BLOCK_EXPAND_FATAL;
						Download_Error = 1;
					}
				}
				if (Download_Error == 1) Log_Message("Download Error");

				// clear for the next round
				LoadCRM = 0;
				for (i = 0 ; i < (MAX_HEADS*MAX_BLOCKS) ; i++) {
					Download[i] = 0;
					DownStatus[i] = 0;
				}
			}
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Transmission_Gain_Stage
// Purpose:		sets the gain for the transmission detectors (P39)
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Transmission_Gain_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long Tube = 0;
	long TubeIndex = 0;
	long DataIndex = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long NumRemaining = 0;
	long CountThreshold = 50;
	long index = 0;
	long Gain = 0;
	long Percent = 0;
	long lld = 0;
	long uld = 0;
	long FileTime = 0;
	long DataSize = 0;
	long SettingIndex = 0;
	long HighPoint = 0;

	long BlockSelect[MAX_BLOCKS];
	long Settings[MAX_SETTINGS*MAX_TOTAL_BLOCKS];

	long Flag[4*MAX_TOTAL_BLOCKS];

	long *Data = NULL;

	double Singles = 0.0;
	double TotalCounts = 0.0;

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Transmission_Gain_Stage";

	HRESULT hr = S_OK;

	// initialize Stage Percentage
	sprintf(m_Stage, "Determining Transmission Gain Settings");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// acquisition settings
	m_DataMode = DHI_MODE_TUBE_ENERGY;
	for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = LAYER_ALL;
	m_AcqType = ACQUISITION_SECONDS;
	m_Amount = 10;
	m_lld = 1;
	m_uld = 255;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) {
		if (Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;
	}

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// initialize arrays
	for (i = 0 ; i < (4*MAX_TOTAL_BLOCKS) ; i++) Flag[i] = 0;

	// for each head
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// increment number of heads being processed
		NumHeads++;

		// switch to bin mode
		Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// get Current analog settings
		if (Status == 0) {

			// log message
			sprintf(Str, "Head %d: Retrieving Analog Settings", Head);
			Log_Message(Str);

			// get the original settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// if failed to get settings,then fatal error for each block
		if (Status != 0) {
			Set_Head_Status(Head, BLOCK_TRANSMISSION_GAIN_FATAL);

		// set all events to fast layer
		} else {

			// log message
			sprintf(Str, "Head %d: Set Shape Discrimination Values To Put All Events in Fast Layer", Head);
			Log_Message(Str);

			// loop through transmission blocks
			for (Block = m_TransmissionStart[Head] ; Block < (m_TransmissionStart[Head]+m_PointSource[Head]) ; Block++) {

				// if selected
				if (BlockSelect[Block] == 1) {

					// set the flags
					for (Tube = 0 ; Tube < 4 ; Tube++) {
						TubeIndex = (((Head * MAX_BLOCKS) + Block) * 4) + Tube;
						if (m_Active[TubeIndex] != -1) {
							Flag[TubeIndex] = 1;
							NumRemaining++;
						}
					}

					// set layer threshold values based on scanner type
					SettingIndex = index + (Block*m_NumSettings);
					switch (m_ScannerType) {

					case SCANNER_PETSPECT:
						Settings[SettingIndex+SetIndex[SHAPE]] = 3;
						Settings[SettingIndex+SetIndex[SLOW_LOW]] = 1;
						Settings[SettingIndex+SetIndex[SLOW_HIGH]] = 2;
						Settings[SettingIndex+SetIndex[FAST_LOW]] = 4;
						Settings[SettingIndex+SetIndex[FAST_HIGH]] = 300;
						break;

					case SCANNER_P39:
						Settings[SettingIndex+SetIndex[SHAPE]] = 0;
						break;

					case SCANNER_HRRT:
					default:
						Settings[SettingIndex+SetIndex[SHAPE]] = 1;
						break;
					}
				}
			}
		}
	}

	// Loop through possible gains
	for (Gain = m_MinGain ; (Gain <= m_MaxGain) && (NumHeads > 0) && (NumRemaining > 0) ; Gain += m_StepGain) {

		// for each head, set to new gain value and put into tube energy mode
		for (Head = 0 ; Head < m_NumHeads ; Head++) {

			// reset status for each head
			Status = 0;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks selected, then go on to next head
			if (NumBlocks == 0) continue;

			// log message
			sprintf(Str, "Head %d: Set Transmissin Gains to %d", Head, Gain);
			Log_Message(Str);

			// index into Settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);

			// change gain for blocks
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {

				// if the block is being processed
				if (BlockSelect[Block] == 1) {

					// loop through the tubes
					for (Tube = 0 ; Tube < 4 ; Tube++) {

						// indecies to flags and settings
						TubeIndex = (((Head * MAX_BLOCKS) + Block) * 4) + Tube;
						SettingIndex = index + (Block * m_NumSettings);

						// if the tube has data
						if ((m_Active[TubeIndex] != -1) && (Flag[TubeIndex] != 0)) {

							// if it is still being processed
							if (Flag[TubeIndex] != 0) {

								// log message
								sprintf(Str, "Head %d Block %d Tube %d Set to %d", Head, Block, Tube, Gain);
								Log_Message(Str);

								// set gain
								Settings[SettingIndex + SetIndex[Tube+PMTA]] = Gain;
							}

						// not active, clear gain
						} else {
							Settings[SettingIndex + SetIndex[Tube+PMTA]] = 0;
						}
					}
				}
			}

			// store and send to head
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Set Values");

			// if failed,then fatal error for each block
			if (Status != 0) Set_Head_Status(Head, BLOCK_TRANSMISSION_GAIN_FATAL);
		}

		// loop through heads putting them in tube energy mode
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// clear process flag
			m_AcquireSelect[Head] = 0;

			// how many blocks does head have left to set?
			if (Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect) > 0) m_AcquireSelect[Head] = 1;
		}

		// log message
		Log_Message("Acquire Tube Energy Data");

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_BlankSelect, m_AcquireSelect,
												m_AcquireLayer, m_AcqType, m_Amount, m_lld, m_uld, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_TRANSMISSION_GAIN_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// bad status, fill acquisition status with value
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) m_AcquireStatus[Head] = Status;
			}
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_TRANSMISSION_GAIN_FATAL);
			}
		}

		// process each head
		for (Head = 0 ; (Head < m_NumHeads) && (NumRemaining > 0) ; Head++) {

			// reset status for each head
			Status = 0;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);

			// if no blocks selected, then go on to next head
			if (NumBlocks == 0) continue;

			// log message
			sprintf(Str, "Head %d: Retrieve Tube Energy Data", Head);
			Log_Message(Str);

			// retrieve the just acquired tube energy data
			hr = pDUThread->Current_Head_Data(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, m_AcquireLayer[Head], &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Data = NULL;
				Log_Message("Failed to Retrieve Tube Energy Data");
				Set_Head_Status(Head, BLOCK_TRANSMISSION_GAIN_FATAL);
			}

			// re-evaluate blocks selected, then skip to next head
			NumBlocks = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);
			if (NumBlocks == 0) continue;

			// compare for each selected block, switch to new setting if counts are higher
			for (Block = 0 ; (Block < m_NumBlocks) && (Status == 0) ; Block++) {

				// if the block is being processed
				if (BlockSelect[Block] == 1) {

					// loop through the tubes
					for (Tube = 0 ; Tube < 4 ; Tube++) {

						// index to flags
						TubeIndex = (((Head * MAX_BLOCKS) + Block) * 4) + Tube;

						// if it is still being processed
						if ((m_Active[TubeIndex] != -1) && (Flag[TubeIndex] != 0)) {

							// singles within range
							DataIndex = ((Block * 4) + m_Active[TubeIndex]) * 256;
							for (Singles = 0, i = 200 ; i <= 255 ; i++) {
								Singles += Data[DataIndex+i];
							}

							// if the number of counts has exceeded the threshold, we have data in range
							if (Singles > CountThreshold) {

								// find the high point of the data beyond bin 100 (skip 255 because of pileup)
								for (HighPoint = 100, i = 100 ; i <= 254 ; i++) {
									if (Data[DataIndex+i] > Data[DataIndex+HighPoint]) HighPoint = i;
								}

								// if it has reached/exceeded the neighborhood of the aimpoint, stop
								if (HighPoint >= (m_AimPoint[Head]-10)) {
									Flag[TubeIndex] = 0;
									NumRemaining--;
								}
							}
						}
					}
				}
			}

			// if failed,then fatal error for each block
			if (Status != 0) Set_Head_Status(Head, BLOCK_TRANSMISSION_GAIN_FATAL);
		}

		// update stage percentage (+1 used so it will top out < 100% set at end)
		m_StagePercent = ((Gain - m_MinGain) * 100) / (m_MaxGain - m_MinGain + 1);
	}

	// log any gains not set
	for (TubeIndex = 0 ; TubeIndex < (MAX_TOTAL_BLOCKS*4) ; TubeIndex++) {

		// if the flag is still set (never exceeded threshold)
		if (Flag[TubeIndex] != 0) {

			// break out values
			Head = TubeIndex / (MAX_BLOCKS*4);
			Block = (TubeIndex / 4) % MAX_BLOCKS;
			Tube = TubeIndex % 4;

			// log warning
			sprintf(Str, "********** WARNING *** Head %d Block %d Tube %d Has High Gain/Low Counts", Head, Block, Tube);
			Log_Message(Str);

			// set warning
			index = TubeIndex / 4;
			m_SetupStatus[index] |= BLOCK_TRANSMISSION_GAIN_NOT_FATAL;
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Transmission_CFD_Stage
// Purpose:		sets the constant fraction discriminators for the transmission detectors (P39)
// Arguments:	none
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Transmission_CFD_Stage()
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long Status = 0;
	long Head = 0;
	long Block = 0;
	long Tube = 0;
	long Peak = 0;
	long NumPeaks = 0;
	long NumBlocks = 0;
	long NumHeads = 0;
	long index = 0;
	long Percent = 0;
	long Step = 0;
	long Iteration = 0;
	long BlocksHead = 0;
	long DataSize = 0;
	long FileTime = 0;
	long EnergyPeak = 0;
	long Cutoff = 0;
	long Counts = 0;

	long *Data = NULL;

	long Bins[256];
	long Peaks[256];
	long BlockSelect[MAX_BLOCKS];
	long Settings[MAX_SETTINGS*MAX_TOTAL_BLOCKS];

	double Body = 0.0;
	double Threshold = 0.0;
	double CFD_Total = 0.0;

	double dBins[256];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Transmission_CFD_Stage";

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Determining Transmission CFD Settings");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// determine the number of heads
	for (Head = 0 ; Head < m_NumHeads ; Head++) if (Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect) > 0) NumHeads++;

	// if no heads are to be processed, then return
	if (NumHeads == 0) {
		m_StagePercent = 100;
		sprintf(Str, "Exit: %s", m_Stage);
		Log_Message(Str);
		return;
	}

	// for each head, get the original settings
	for (Head = 0 ; Head < m_NumHeads ; Head++) {

		// reset Status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks selected, then go on to next head
		if (NumBlocks == 0) continue;

		// switch to bin mode
		Status = Set_Setup_Energy(m_Configuration, Head, 190);

		// get Current analog settings
		if (Status == 0) {

			// log message
			sprintf(Str, "Head %d: Retrieve Analog Settings", Head);
			Log_Message(Str);

			// get the original settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}

		// if failed to get settings,then fatal error for each block
		if (Status != 0) Set_Head_Status(Head, BLOCK_TRANSMISSION_CFD_FATAL);

		// log message
		sprintf(Str, "Head %d: Set CFD to Starting Value: %d", Head, m_TxCFDStart);
		Log_Message(Str);

		// set to initial value
		for (Block = 0 ; (Block < m_NumBlocks) && (Status == 0) ; Block++) {
			if (BlockSelect[Block] != 0) {
				Settings[index+(Block*m_NumSettings)+SetIndex[CFD]] = m_TxCFDStart;
			}
		}
	}

	// loop through number of passes
	for (Iteration = m_TxCFDIterations-1 ; Iteration >= -1 ; Iteration--) {

		// Calculate the step
		Step = (Iteration > 0) ? ((long) pow(2.0, (double) Iteration)) : 1;

		// log message
		sprintf(Str, "Iteration %d of %d - Step %d", (m_TxCFDIterations-Iteration-1), m_TxCFDIterations, Step);
		Log_Message(Str);

		// update stage percentage
		m_StagePercent = ((m_TxCFDIterations - Iteration - 1) * 100) / (m_TxCFDIterations+1);

		// loop through heads updating the settings and putting them in crystal energy mode
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// clear process flag
			m_AcquireSelect[Head] = 0;

			// Determine the number of blocks from the head are to be set up
			NumBlocks = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);
			if (NumBlocks == 0) continue;

			// set the new settings
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Set Values");

			// mark to acquire
			if (Status == 0) m_AcquireSelect[Head] = 1;
		}

		// transfer variables for singles acquisition thread
		m_DataMode = DHI_MODE_TUBE_ENERGY;
		for (i = 0 ; i < MAX_HEADS ; i++) m_AcquireLayer[i] = LAYER_FAST;
		m_AcqType = ACQUISITION_SECONDS;
		m_Amount = 10;
		m_lld = 1;
		m_uld = 255;

		// log message
		Log_Message("Acquiring Tube Energy Data");

		// start up acquisition
		hr = pDUThread->Acquire_Singles_Data(m_DataMode, m_Configuration, m_BlankSelect, m_AcquireSelect, 
												m_AcquireLayer, m_AcqType, m_Amount, m_lld, m_uld, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Singles_Data), HRESULT", hr);
		if (Status != 0) {
			for (Head = 0 ; Head < MAX_HEADS ; Head++) {
				if (m_AcquireSelect[Head] == 1) Set_Head_Status(Head, BLOCK_TRANSMISSION_CFD_FATAL);
			}
			Log_Message("Failed To Start Acquisition");
		}

		// poll for progress of acquisition
		Percent = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// do status for each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Acquisition Failed", Head);
				Log_Message(Str);
				Set_Head_Status(Head, BLOCK_TRANSMISSION_CFD_FATAL);
			}
		}

		// process each head
		for (Head = 0 ; Head < MAX_HEADS ; Head++) if ((m_AcquireSelect[Head] == 1) && (m_AcquireStatus[Head] == 0)) {

			// index to start of settings for head
			index = Head * MAX_BLOCKS * m_NumSettings;

			// get block flags
			NumBlocks = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);

			// log message
			sprintf(Str, "Head %d: Retrieving Tube Energy Data", Head);
			Log_Message(Str);

			// retrieve the just acquired tube energy data
			hr = pDUThread->Current_Head_Data(DHI_MODE_TUBE_ENERGY, m_Configuration, Head, LAYER_FAST, &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Data = NULL;
				Log_Message("Failed to Retrieve Tube Energy Data");
				Set_Head_Status(Head, BLOCK_TRANSMISSION_CFD_FATAL);
			}

			// re-evaluate blocks selected, then skip to next head
			BlocksHead = Blocks_From_Head(TRANSMISSION_BLOCKS, Head, BlockSelect);
			if (BlocksHead == 0) continue;

			// loop through the blocks
			for (i = 0 ; (i < MAX_BLOCKS) && (Status == 0) ; i++) if (BlockSelect[i] == 1) {

				// compress all the tubes to one histogram
				for (j = 0, Counts = 0 ; j < 256 ; j++) {
					Bins[j] = 0;
					for (k = 0 ; k < 4 ; k++) {
						Bins[j] += Data[(i*4*256)+(k*256)+j];
						Counts += Data[(i*4*256)+(k*256)+j];
					}
				}

				// if no counts then error
				if (Counts == 0) {
					sprintf(Str, "Block %d Had No Data", i);
					Log_Message(Str);
					m_SetupStatus[(Head*MAX_BLOCKS)+i] |= BLOCK_TRANSMISSION_CFD_FATAL;
					continue;
				}

				// since pileup accumulates in bin 255, clear it out so we don't consider it for a peak
				Bins[255] = 0;

				// Find peaks
				Smooth(3, 50, Bins, dBins);
				NumPeaks = Find_Peak_1d(dBins, Peaks);

				// determine energy peak
				switch (NumPeaks) {

				case 0:
					EnergyPeak = m_AimPoint[Head];
					break;

				case 1:
					EnergyPeak = Peaks[0];
					break;

				default:
					if ((Peaks[0] < 100) && (Peaks[1] >= 100)) {
						EnergyPeak = Peaks[1];
					} else {
						EnergyPeak = Peaks[0];
					}
					break;
				}

				// calculate the cutoff as a bin number
				Cutoff = EnergyPeak / 2;

				// Calculate the threshold
				for (Body = 0.0, j = (Cutoff+1) ; j < 256 ; j++) Body += (double)Bins[j];
				Threshold = (double) Body * m_TransmissionCFDRatio;

				// total up all counts below the cutoff energy
				for (CFD_Total = 0.0, j = 0 ; j <= Cutoff ; j++) CFD_Total += (double)Bins[j];

				// if the cutoff bit has too many counts, then increment the counter
				index = (Head * MAX_BLOCKS * MAX_SETTINGS) + (i * m_NumSettings);
				if (CFD_Total > Threshold) {
					Settings[index+SetIndex[CFD]] += Step;
				} else {
					if (Iteration != -1) Settings[index+SetIndex[CFD]] -= Step;
				}

				// log message
				sprintf(Str, "\tBlock %d: New Value = %d", i, Settings[index+SetIndex[CFD]]);
				Log_Message(Str);
			}

			// release the memory
			if (Data != NULL) CoTaskMemFree(Data);

			// set the new settings from the last iteration
			if (Iteration == -1) {
				index = Head * (MAX_BLOCKS * MAX_SETTINGS);
				hr = pDUThread->Store_Settings(SEND, m_Configuration, Head, &Settings[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
				if (Status != 0) Log_Message("Failed to Set Values");
			}
		}
	}

	// Stage Complete
	m_StagePercent = 100;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Blocks_From_Head
// Purpose:		returns the blocsk which were selected and have not had a fatal error
// Arguments:	Type		-	long = emission or transmission
//				Head		-	detector head
//				BlockSelect	-	long = return array indicating which blocks are still selected
// Returns:		long number of remaining blcoks
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Blocks_From_Head(long Type, long Head, long BlockSelect[])
{
	// variables
	long i = 0;
	long index = 0;
	long NumBlocks = 0;
	long First = 0;
	long Last = 0;

	// initialize blocks selected
	for (i = 0 ; i < MAX_BLOCKS ; i++) BlockSelect[i] = 0;

	// if head not present, return 0
	if (m_HeadPresent[Head] == 0) return 0;

	// determine first and last block to check
	switch (Type) { 

	case EMISSION_BLOCKS:
		First = 0;
		Last = m_EmissionBlocks - 1;
		break;

	case TRANSMISSION_BLOCKS:
		First = m_TransmissionStart[Head];
		Last = First + m_PointSource[Head] - 1;
		break;

	case HEAD_BLOCKS:
	default:
		First = 0;
		Last = MAX_BLOCKS - 1;
		break;
	}

	// count blocks
	for (i = First ; i <= Last ; i++) {
		index = (Head * MAX_BLOCKS) + i;
		if ((m_SetupSelect[index] != 0) && ((m_SetupStatus[index] & BLOCK_FATAL) != BLOCK_FATAL)) {
			NumBlocks++;
			BlockSelect[i] = 1;
		}
	}

	// return value
	return NumBlocks;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Set_Head_Status
// Purpose:		sets all blocks on the indicated head to the indicated status
// Arguments:	Head		-	long = detector head
//				Status		-	long = status to set blocks to
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Set_Head_Status(long Head, long Status)
{
	// local variables
	long i = 0;
	long index = 0;

	// loop through all blocks in head
	for (i = 0 ; i < MAX_BLOCKS ; i++) {

		// if the block is selected, or with current status
		index = (Head * MAX_BLOCKS) + i;
		if (m_SetupSelect[index] == 1) m_SetupStatus[index] |= Status;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Remaining
// Purpose:		returns the number of remaining crystals that can have the crm
//				expanded for a specific block.
// Arguments:	Head		-	long = detector head
//				Block		-	long = Block number
//				Change		-	long = flag array specifying whether or not each
//										crystal can still be changed
// Returns:		void
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

bool Remaining(long Head, long Block, long *Change)
{
	// local
	long i = 0;
	long Num = 0;
	long index = 0;
	long NumSide = 0;

	// number of side vertices per block
	NumSide = (2 * m_XCrystals)+(2 * m_YCrystals);

	// index into start of array
	index = ((Head * MAX_BLOCKS) + Block) * 64;

	// loop through
	for (i = 0 ; i < NumSide ; i++) Num += Change[index+i];

	return (Num > 0);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Release_Servers
// Purpose:		releases the DAL server
// Arguments:	none
// Returns:		void
// History:		18 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSMain::Release_Servers()
{
	// release dhi server
	if (pDHI != NULL) pDHI->Release();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Log_Message
// Purpose:		add a message to the setup log
// Arguments:	Message		-	char = message to be added
// Returns:		void
// History:		18 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Log_Message(char *Message)
{
	// local variables
	char time_string[MAX_STR_LEN];
	time_t ltime;

	// get current time and remove new line character
	time(&ltime);
	sprintf(time_string, "%s", ctime(&ltime));
	time_string[24] = '\0';

	// print to log file and then flush the buffer
	if (m_fpLog != NULL) {
		fprintf(m_fpLog, "%s %s\n", time_string, Message);
		fflush(m_fpLog);
	}
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

			// reset status
			Status = 0;

			// log message
			sprintf(m_Stage, "Backup Up Head %d", Head);
			Log_Message(m_Stage);

			// if scanner is a P39, then indidual files have to be copied to a backup directory
			if (m_ScannerType == SCANNER_P39) {

				// delete any pre-existing backup directory and contents (failure is not an error)
				if (Status == 0) {
					Log_Message("Remove Previous Backup Directory");
					sprintf((char *) Command, "rmdir Backup /s /q");
					hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (Status != 0) Status = 0;
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
				}

				// create the backup directory
				if (Status == 0) {
					Log_Message("Make New Backup Directory");
					sprintf((char *) Command, "mkdir Backup");
					if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (Status != 0) Status = 0;
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
				}

				// copy crm files
				if (Status == 0) {
					Log_Message("Copy CRM Files");
					sprintf((char *) Command, "copy %d_*.crm /y Backup\\", m_Configuration);
					if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (Status != 0) Status = 0;
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
				}

				// copy chi files
				if (Status == 0) {
					Log_Message("Copy .chi Files");
					sprintf((char *) Command, "copy %d_*.chi /y Backup\\", m_Configuration);
					if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (Status != 0) Status = 0;
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
				}

				// copy crystal energy peak and crystal time offset files
				if (Status == 0) {
					Log_Message("Copy Crystal Peaks and Time Offsets");
					sprintf((char *) Command, "copy %d_*.txt /y Backup\\", m_Configuration);
					if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (Status != 0) Status = 0;
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
				}

				// copy crystal location files
				if (Status == 0) {
					Log_Message("Copy Crystal Locations");
					sprintf((char *) Command, "copy %d_pk*.bin /y Backup\\", m_Configuration);
					if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (Status != 0) Status = 0;
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
				}

			// other scanner types, we can create a zip file with the data we want to save
			} else {

				// zip files on head
				Log_Message("Zipping Files");
				sprintf((char *) Command, "pkzip Backup.zip");
				sprintf((char *) Command, "%s %d_*.crm", (char *) Command, m_Configuration);
				sprintf((char *) Command, "%s %d_*.chi", (char *) Command, m_Configuration);
				sprintf((char *) Command, "%s %d_*.txt", (char *) Command, m_Configuration);
				sprintf((char *) Command, "%s %d_pk*.bin", (char *) Command, m_Configuration);
				if (Status == 0) {
					hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
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

	HRESULT hr = S_OK;

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Local_Restore";

	unsigned char Command[MAX_STR_LEN];

	// loop through the heads
	for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

		// if the head is selected
		if (HeadSelect[Head] != 0) {

			// log message
			sprintf(Str, "Restore Head %d", Head);
			Log_Message(Str);

			// if scanner is a P39, then indidual files have to be copied to a backup directory
			if (m_ScannerType == SCANNER_P39) {

				// copy all files from backup directory back up
				sprintf((char *) Command, "copy Backup\\*.* /y .\\");
				if (Status == 0) hr = pDHILocal->OS_Command(Head, Command, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);

			// other scanner types, we can create a zip file with the data we want to save
			} else {

				// zip files on head
				sprintf((char *) Command, "pkunzip -o Backup.zip");
				if (Status == 0) {
					hr = pDHILocal->OS_Command(Head, Command, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->OS_Command), HRESULT", hr);
				}
			}
		}
	}

	// force a reload of all values
	if (Status == 0) {
		hr = pDHILocal->Force_Reload();
		if (FAILED(hr)) Status = Add_Error(pErrSup, Subroutine, "Method Failed (pDHILocal->Force_Reload), HRESULT", hr);
	}

	// return the status
	return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		CRM_Initial_Trim
// Purpose:		trims CRMs for selected blocks back prior to expansion
// Arguments:	none
// Returns:		void
// History:		18 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CRM_Initial_Trim()
{
	// local variables
	long i = 0;
	long index = 0;
	long Status = 0;
	long Head = 0;
	long Layer = 0;
	long LocationLayer = 0;
	long Block = 0;
	long NumBlocks = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long VertsBlock = 0;

	long BlockSelect[MAX_BLOCKS];
	long Position[MAX_HEAD_VERTICES*2];
	long Verts[2178];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "CRM_Initial_Trim";

	unsigned char CRM[PP_SIZE];

	HRESULT hr = S_OK;

	// initialize stage completion
	sprintf(m_Stage, "Trimming CRM(s)");
	sprintf(Str, "Start: %s", m_Stage);
	Log_Message(Str);
	m_StagePercent = 0;

	// number of verticies per block
	Verts_X = (2 * m_XCrystals) + 1;
	Verts_Y = (2 * m_YCrystals) + 1;
	VertsBlock = Verts_X * Verts_Y;

	// loop through the heads
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// reset status for each head
		Status = 0;

		// Determine the number of blocks from the head are to be set up
		NumBlocks = Blocks_From_Head(EMISSION_BLOCKS, Head, BlockSelect);

		// if no blocks are to be set up, then skip to next head
		if (NumBlocks == 0) continue;

		// loop through the layers
		for (Layer = 0 ; (Layer < NUM_LAYER_TYPES) && (Status == 0) ; Layer++) {

			// if this layer is not used for this head, then next layer
			if (((m_ScannerType == SCANNER_HRRT) || (m_ScannerType == SCANNER_P39_IIA)) && (Layer != LAYER_ALL)) continue;
			if ((m_ScannerType != SCANNER_HRRT) && (m_ScannerType != SCANNER_P39_IIA) && (Layer == LAYER_ALL)) continue;
			if ((m_NumLayers[Head] == 1) && (Layer == LAYER_SLOW)) continue;
			if ((m_ScannerType == SCANNER_PETSPECT) && (m_Configuration != 0) && (Layer != LAYER_SLOW)) continue;

			// designate the location layer (HRRT stores CRMs in different place than data)
			LocationLayer = Layer;
			if (m_ScannerType == SCANNER_HRRT) LocationLayer = LAYER_FAST;

			// get the crystal locations
			sprintf(Str, "Head %d %s: Retrieving Crystal Positions", Head, StrLayer[Layer]);
			Log_Message(Str);
			hr = pDUThread->Get_Stored_Crystal_Position(m_Configuration, Head, LocationLayer, Position, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Get_Stored_Crystal_Position), HRESULT", hr);

			// bad status, fail all blocks on head
			if (Status != 0) {
				Log_Message("Failed to Retrieve Positons");
				Set_Head_Status(Head, BLOCK_EXPAND_FATAL);
			}

			// process each block
			for (Block = 0 ; (Block < MAX_BLOCKS) && (Status == 0) ; Block++) {

				// if Block is selected
				if (BlockSelect[Block] != 0) {

					// transfer verticies to local array from master
					for (i = 0 ; i < (2 * VertsBlock) ; i++) Verts[i] = Position[(Block*VertsBlock*2)+i];

					// fill in the non-peak verticies
					Fill_CRM_Verticies(4, m_XCrystals, m_YCrystals, Verts);

					// if the block is on the bottom row, it is safe to take the bottom edge out to 5 pixels from edge
					if ((Block / m_XBlocks) == 0) {
						for (i = 0 ; i < Verts_X ; i++) {
							index = (i * 2) + 1;
							if (Verts[index] > 5) Verts[index] = 5;
						}
					}

					// if the block is on the top row, it is safe to take the bottom edge out to 5 pixels from edge
					if ((Block / m_XBlocks) == (m_YBlocks-1)) {
						for (i = 0 ; i < Verts_X ; i++) {
							index = ((((Verts_Y-1) * Verts_X) + i) * 2) + 1;
							if (Verts[index] < (PP_SIZE_Y - 5)) Verts[index] = (PP_SIZE_Y - 5);
						}
					}

					// if the block is on the left edge, it is safe to take the bottom edge out to 5 pixels from edge
					if ((Block % m_XBlocks) == 0) {
						for (i = 0 ; i < Verts_Y ; i++) {
							index = i * Verts_X * 2;
							if (Verts[index] > 5) Verts[index] = 5;
						}
					}

					// if the block is on the right edge, it is safe to take the bottom edge out to 5 pixels from edge
					if ((Block % m_XBlocks) == (m_XBlocks-1)) {
						for (i = 0 ; i < Verts_Y ; i++) {
							index = ((i+1) * Verts_X * 2) - 2;
							if (Verts[index] < (PP_SIZE_X - 5)) Verts[index] = (PP_SIZE_X - 5);
						}
					}

					// build the CRM
					Build_CRM(Verts, m_XCrystals, m_YCrystals, CRM);

					// log message
					sprintf(Str, "\tBlock %d: Storing Crystal Region Map", Block);
					Log_Message(Str);

					// write the CRM
					hr = pDUThread->Store_CRM(m_Configuration, Head, LocationLayer, Block, CRM, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_CRM), HRESULT", hr);
					if (Status != 0) {
						m_SetupStatus[(Head*MAX_BLOCKS)+Block] |= BLOCK_EXPAND_FATAL;
						sprintf(Str, "\tFailed to store Crystal Region Map");
						Log_Message(Str);
					}

					// transfer verticies to head array
					for (i = 0 ; i < (2 * VertsBlock) ; i++) Position[(Block*VertsBlock*2)+i] = Verts[i];
				}
			}

			// get the crystal locations
			sprintf(Str, "Head %d %s: Storing Crystal Positions", Head, StrLayer[Layer]);
			Log_Message(Str);
			hr = pDUThread->Store_Crystal_Position(m_Configuration, Head, LocationLayer, Position, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Crystal_Position), HRESULT", hr);
		}
	}

	// Stage Complete - building the crms takes the trivial amount of time - downloading will take the other 80%
	m_StagePercent = 20;
	sprintf(Str, "Exit: %s", m_Stage);
	Log_Message(Str);
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		P39TimeAlignThread
// Purpose:		Work thread for time alignment on the P39
// Arguments:	dummy		- void
// Returns:		void
// History:		18 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void P39TimeAlignThread(void *dummy)
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long Iteration = 0;
	long index = 0;
	long Config = 0;
	long Layer = LAYER_ALL;
	long Head = 0;
	long Block = 0;
	long Blk_X = 0;
	long Blk_Y = 0;
	long Xtal = 0;
	long Percent = 0;
	long FileTime = 0;
	long DataSize = 0;
	long Status = 0;
	long Duration = 0;
	long Server_Locked = 0;
	long WindowChanged = 0;
	long Transmission = 0;
	long Shift = 0;
	long Counts = 0;
	long SubBin = 0;

	long *Data = NULL;

	long HeadSelect[MAX_HEADS];
	long LayerSelect[MAX_HEADS];
	long Offsets[MAX_HEAD_CRYSTALS*MAX_HEADS];

	long TxBlock[10] = {80, 80, 80, 81, 81, 81, 82, 82, 83, 83};

	double Thresh = 0.0;
	double Sum = 0.0;

	HRESULT hr = S_OK;

	char LogFile[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "P39TimeAlignThread";

    struct tm *newtime;
    time_t long_time;

	// combined layer if phase IIA electronics
	if (m_ScannerType == SCANNER_P39_IIA) Layer = LAYER_ALL;

	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}

	// initialize stage percentage
	m_StagePercent = 0;
	m_TotalPercent = 0;
	sprintf(m_Stage, "Starting Time Alignment Pass");

	// log file name
    time(&long_time);
    newtime = localtime(&long_time);
	sprintf(LogFile, "%sLog\\%2.2d%s%4.4d_%2.2d%2.2d.log", m_RootPath, newtime->tm_mday, 
			MonthStr[newtime->tm_mon], 1900+newtime->tm_year, newtime->tm_hour, newtime->tm_min);

	// open log file
	m_fpLog = fopen(LogFile, "wt");
	if (m_fpLog == NULL) {
		sprintf(Str, "Failed To Open Time Alignment Log File: %s", LogFile);
		Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
	}

	// log start message
	Log_Message("Starting Time Alignment");

	// log model information
	sprintf(Str, "Model Number: %d", m_ModelNumber);
	Log_Message(Str);

	// Clear Status
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
		m_SetupSelect[i] = 0;
		m_SetupStatus[i] = 0;
	}

	// Time Alignment Type
	for (Head = 0 ; Head < MAX_HEADS ; Head++) HeadSelect[Head] = 0;
	for (Head = 0 ; Head < MAX_HEADS ; Head++) LayerSelect[Head] = Layer;
	switch (m_SetupType) {

	// time align heads 1 & 4 on a 393 - done first
	case TIME_ALIGN_TYPE_393_1:
		sprintf(Str, "Time Align - First Stage: ");
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_HeadPresent[Head] != 0) && ((Head == 1) || (Head == 4))) {
				HeadSelect[Head] = 1;
				sprintf(Str, "%s %d", Str, Head);
				for (Block = 0 ; Block < m_EmissionBlocks ; Block++) m_SetupSelect[(Head*MAX_BLOCKS)+Block] = 1;
			}
		}
		break;

	// time align head 2 on a 393 - done second
	case TIME_ALIGN_TYPE_393_2:
		sprintf(Str, "Time Align - Second Stage: ");
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if ((m_HeadPresent[Head] != 0) && (Head == 2)) {
				HeadSelect[Head] = 1;
				sprintf(Str, "%s %d", Str, Head);
				for (Block = 0 ; Block < m_EmissionBlocks ; Block++) m_SetupSelect[(Head*MAX_BLOCKS)+Block] = 1;
			}
		}
		break;

	// time align all heads on a 395
	case TIME_ALIGN_TYPE_395:
		sprintf(Str, "Time Align - Second Stage: ");
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {
			if (m_HeadPresent[Head] != 0) {
				HeadSelect[Head] = 1;
				sprintf(Str, "%s %d", Str, Head);
				for (Block = 0 ; Block < m_EmissionBlocks ; Block++) m_SetupSelect[(Head*MAX_BLOCKS)+Block] = 1;
			}
		}
		break;

	// time align the transmission detectors
	case TIME_ALIGN_TYPE_TRANSMISSION:
		sprintf(Str, "Time Align Transmission Detectors On Heads: ");
		for (Head = 0 ; Head < MAX_HEADS ; Head++)  {
			if ((m_HeadPresent[Head] != 0) && (m_PointSource[Head] != 0)) {
				HeadSelect[Head] = 1;
				sprintf(Str, "%s %d", Str, Head);
				for (Block = 0 ; Block < m_PointSource[Head] ; Block++) m_SetupSelect[(Head*MAX_BLOCKS)+m_TransmissionStart[Head]+Block] = 1;
			}
		}
		break;

	}

	// Create an instance of DHI Server.
	if (Status == 0) {
		pDHIThread = Get_DAL_Ptr(pErrEvtSupThread);
		if (pDHIThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get DAL Pointer", 0);
		if (Status != 0) Log_Message("Failed Attaching to Detector Abstraction Layer Server");
	}

	// Create an instance of Detector Utilities Server.
	if (Status == 0) {
		pDUThread = Get_DU_Ptr(pErrEvtSupThread);
		if (pDUThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get DU Pointer", 0);
		if (Status != 0) Log_Message("Failed Attaching to Detector Utilities Server");
	}

	// bad status, set fatal error
	if (Status != 0) for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) if (m_SetupSelect[i] != 0) m_SetupStatus[i] = BLOCK_TIME_ALIGN_FATAL;

	// provide backup
	if (Status == 0) Status = Local_Backup(pErrEvtSupThread, pDHIThread, HeadSelect);

	// retrieve the current offsets
	for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

		if (HeadSelect[Head] == 1) {

			// log message
			sprintf(Str, "Head %d: Retrieving Starting Time Offsets", Head);
			Log_Message(Str);

			// retrieve data
			index = Head * MAX_HEAD_CRYSTALS;
			hr = pDUThread->Get_Stored_Crystal_Time(Config, Head, Layer, &Offsets[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Get_Stored_Crystal_Time), HRESULT", hr);
			if (Status != 0) {
				for (i = 0 ; i < MAX_HEAD_CRYSTALS ; i++) Offsets[index+i] = 0;
				Log_Message("Failed to Retrieve Crystal Time Offsets - Continuing");
				Status = 0;
			}

			// clear the offsets
			if (m_SetupType == TIME_ALIGN_TYPE_TRANSMISSION) {
				for (Block = 0 ; (Block < m_PointSource[Head]) && (Status == 0) ; Block++) {
					for (Xtal = 0 ; Xtal < MAX_CRYSTALS ; Xtal++) {
						index = (Head * MAX_HEAD_CRYSTALS) + ((m_TransmissionStart[Head]+Block) * MAX_CRYSTALS) + Xtal;
						Offsets[index] = 0;
					}
				}
			} else {
				for (Block = 0 ; (Block < m_EmissionBlocks) && (Status == 0) ; Block++) {
					for (Xtal = 0 ; Xtal < MAX_CRYSTALS ; Xtal++) {
						index = (Head * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + Xtal;
						Offsets[index] = 0;
					}
				}
			}
		}
	}

	// set duration
	if (m_SetupType == TIME_ALIGN_TYPE_TRANSMISSION) {
		Transmission = 1;
		Duration = m_TxTimeAlignDuration;
	} else {
		Duration = m_EmTimeAlignDuration;
	}

	// switch to time alignment coincidence window
	if (Status == 0) {
		Log_Message("Changing Coincidence Window to 62 ns");
		hr = pDHIThread->Time_Window(62, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Time_Window), HRESULT", hr);
		if (Status == 0) WindowChanged = 1;
		if (Status != 0) Log_Message("Failed to Set Coincidence Window");
	}

	// loop through the number of iterations designated
	for (Iteration = 0 ; (Iteration <= m_TimeAlignIterations) && (Status == 0) ; Iteration++) {

		// update stage percentage
		m_StagePercent = (Iteration * 100) / (m_TimeAlignIterations + 1);
		sprintf(Str, "Iteration %d of %d", Iteration, m_TimeAlignIterations);
		Log_Message(Str);

		// switch to high resolution coincidence window (phase IIA electronics only)
		if ((Status == 0) && (Iteration == 1) && (m_ScannerType == SCANNER_P39_IIA)) {
			Log_Message("Changing Coincidence Window to High Resolution");
			hr = pDHIThread->Time_Window(10, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Time_Window), HRESULT", hr);
			if (Status == 0) WindowChanged = 1;
			if (Status != 0) Log_Message("Failed to Change Coincidence Window to High Resolution");
		}

		// download the current time offsets
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {
			if (HeadSelect[Head] == 1) {
				sprintf(Str, "Head %d: Downloading Crystal Time", Head);
				Log_Message(Str);
				index = Head * MAX_HEAD_CRYSTALS;
				hr = pDUThread->Store_Crystal_Time(SEND, Config, Head, Layer, &Offsets[index], &Status);
				if (FAILED(hr)) {
					Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Crystal_Time), HRESULT", hr);
					sprintf(Str, "Head %d: Failed to Download Crystal Time", Head);
					Log_Message(Str);
				}
			}
		}

		// force a new acquire to reload time values
		if (Status == 0) {
			hr = pDHIThread->Force_Reload();
			if (FAILED(hr)) {
				Log_Message("Failed To Force Reload");
				Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Force_Reload), HRESULT", hr);
			}
		}

		// acquire the crystal time data
		if (Status == 0) {
			Log_Message("Starting Time Alignment Acquisition");
			hr = pDUThread->Acquire_Coinc_Data(DATA_MODE_TIMING, Config, HeadSelect, LayerSelect, m_ModulePair, Transmission, 
												ACQUISITION_SECONDS, Duration, m_ConfigurationLLD[Config], m_ConfigurationULD[Config], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Coinc_Data), HRESULT", hr);
			if (Status != 0) {
				for (Head = 0 ; Head < MAX_HEADS ; Head++) {
					if (HeadSelect[Head] == 1) Set_Head_Status(Head, BLOCK_TIME_ALIGN_FATAL);
				}
				Log_Message("Failed To Start Acquisition");
			}
		}

		// poll for progress of acquisition
		Percent = 0;
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) m_AcquireStatus[Head] = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// success message
		if (Status == 0) Log_Message("Acquisition Complete");

		// do status for each head
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {
			if ((HeadSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Failed Acquisition", Head);
				Log_Message(Str);
				Status = -1;
			}
		}

		// don't process last iteration
		if (Iteration == m_TimeAlignIterations) continue;

		// process the heads
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

			// if head is being processed
			if (HeadSelect[Head] == 0) continue;

			// log message
			sprintf(Str, "Head %d: Retrieving Crystal Time Data", Head);
			Log_Message(Str);

			// retrieve the just acquired crystal timing data
			hr = pDUThread->Current_Head_Data(DATA_MODE_TIMING, Config, Head, Layer, &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Log_Message("Failed to Retrieve Crystal Timing Data");
				Set_Head_Status(Head, BLOCK_TIME_ALIGN_FATAL);
			}

			// different processing for emission and transmission
			if (m_SetupType == TIME_ALIGN_TYPE_TRANSMISSION) {

				// log message
				Log_Message("Processing Transmission");

				// loop through the crystals
				for (Xtal = 0 ; (Xtal < 10) & (Status == 0) ; Xtal++) {

					// block position
					Blk_Y = TxBlock[Xtal] / m_XBlocks;

					// sum data
					index = (((Blk_Y * m_YCrystals) * 128) + Xtal) * m_TimingBins;
					for (Sum = 0, k = 1 ; k < m_TimingBins ; k++) Sum += Data[index+k];

					// find the point where half the counts are on either side
					for (k = 1, Counts = 0 ; (k < m_TimingBins) && (Counts < (Sum / 2)) ; k++) Counts += Data[index+k];

					// remove the last bin
					Counts -= Data[index+(--k)];

					// check the sub-bin resolution to see what side it falls on
					SubBin = (long)(((double)((Sum / 2) - Counts) / (double)Data[index+k]) + 0.5);
					if (SubBin == 0) k--;

					// shift value
					Shift = (m_TimingBins / 2) - k;

					// first iteration, use full value
					if (Iteration == 0) {

						// phase IIA electronics starts with 1 ns resolution, but 0.5 ns control
						if (m_ScannerType == SCANNER_P39_IIA) Shift *= 2;

					// after first iteration, don't allow extreme changes
					} else {
						if (Shift > 0) Shift = 1;
						if (Shift < 0) Shift = -1;
					}

					// Apply Offset
					index = (Head * MAX_HEAD_CRYSTALS) + (TxBlock[Xtal] * MAX_CRYSTALS) + Xtal;
					Offsets[index] += Shift;
				}
			} else {

				// determine the average number of counts for crystal and use 20% of that as a threshold
				for (Thresh = 0.0, j = 0 ; j < (m_YBlocks * m_YCrystals) ; j++) {
					for (i = 0 ; i < (m_XBlocks * m_XCrystals) ; i++) {
						index = ((j * 128) + i) * m_TimingBins;
						for (k = 0 ; k < m_TimingBins ; k++) Thresh += Data[index+k];
					}
				}
				Thresh /= (m_YBlocks * m_YCrystals) * (m_XBlocks * m_XCrystals);
				Thresh *= 0.25;

				// loop through the blocks
				for (Block = 0 ; (Block < m_EmissionBlocks) && (Status == 0) ; Block++) {

					// log message
					sprintf(Str, "Head %d Block %d", Head, Block);
					Log_Message(Str);

					// Block Values
					Blk_X = Block % m_XBlocks;
					Blk_Y = Block / m_XBlocks;

					// loop through the crystals
					for (j = 0 ; (j < m_YCrystals) & (Status == 0) ; j++) {
						for (i = 0 ; (i < m_XCrystals) & (Status == 0) ; i++) {

							// sum data
							index = ((((Blk_Y * m_YCrystals) + j) * 128) + (Blk_X * m_XCrystals) + i) * m_TimingBins;
							for (Sum = 0, k = 1 ; k < m_TimingBins ; k++) Sum += Data[index+k];

							// if there were enough counts determine offset
							if (Sum > Thresh) {

								// find the point where half the counts are on either side
								for (k = 1, Counts = 0 ; (k < m_TimingBins) && (Counts < (Sum / 2)) ; k++) Counts += Data[index+k];

								// remove the last bin
								Counts -= Data[index+(--k)];

								// check the sub-bin resolution to see what side it falls on
								SubBin = (long)(((double)((Sum / 2) - Counts) / (double)Data[index+k]) + 0.5);
								if (SubBin == 0) k--;

								// shift value
								Shift = (m_TimingBins / 2) - k;

								// first iteration, use full value
								if (Iteration == 0) {

									// phase IIA electronics starts with 1 ns resolution, but 0.5 ns control
									if (m_ScannerType == SCANNER_P39_IIA) Shift *= 2;

								// after first iteration, don't allow extreme changes
								} else {
									if (Shift > 0) Shift = 1;
									if (Shift < 0) Shift = -1;
								}

								// Apply Offset
								index = (Head * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + (j * 16) + i;
								Offsets[index] += Shift;
							}
						}
					}
				}
			}
		}
	}

	// bad status, set fatal error
	if (Status != 0) for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) if (m_SetupSelect[i] != 0) m_SetupStatus[i] = BLOCK_TIME_ALIGN_FATAL;

	// switch window back to 6 ns
	if (WindowChanged == 1) {
		Log_Message("Changing Coincidence Window Back to 6 ns");
		hr = pDHIThread->Time_Window(6, &Status);
		if (FAILED(hr)) Status = Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Time_Window), HRESULT", hr);
		if (Status != 0) Log_Message("Failed To Change Coincidence Window Back to 6 ns");
	}

	// close log file
	if (m_fpLog != NULL) fclose(m_fpLog);

	// release pointers
	pDHIThread->Release();
	pDUThread->Release();
	delete pErrEvtSupThread;

	// finish off Percentages
	m_StagePercent = 100;

	// clear setup flag
	m_SetupFlag = 0;

	// clear stage string
	sprintf(m_Stage, "Setup Idle");

	// finished
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		HRRT_TimeAlignThread
// Purpose:		Work thread for time alignment on the HRRT
// Arguments:	dummy		- void
// Returns:		void
// History:		18 Sep 03	T Gremillion	History Started
//				03 Oct 03	T Gremillion	Added printing of average fwhm and 
//											centroid for each head for each pass
//				22 Nov 04	T Gremillion	release memory after first retrieval of data
//											changed final range from 0.125 to 0.15 to prevent flip-flop
/////////////////////////////////////////////////////////////////////////////

void HRRT_TimeAlignThread(void *dummy)
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long Iteration = 0;
	long index = 0;
	long Config = 0;
	long Layer = LAYER_ALL;
	long Head = 0;
	long Block = 0;
	long Blk_X = 0;
	long Blk_Y = 0;
	long Xtal = 0;
	long Percent = 0;
	long FileTime = 0;
	long DataSize = 0;
	long Status = 0;
	long Duration = 0;
	long Server_Locked = 0;
	long WindowChanged = 0;
	long Transmission = 0;
	long ModulePair = 0;
	long Counts = 0;
	long SubBin = 0;
	long Old_Value = 0;

	long *Data = NULL;

	long Line[256];
	long HeadSelect[MAX_HEADS];
	long LayerSelect[MAX_HEADS];
	long Settings[MAX_SETTINGS*MAX_TOTAL_BLOCKS];

	double Thresh = 100 * m_XCrystals * m_YCrystals;
	double Sum = 0.0;
	double Fraction = 0.0;
	double Shift = 0.0;
	double fwhm = 0.0;
	double Centroid = 0.0;

	HRESULT hr = S_OK;

	char LogFile[MAX_STR_LEN];
	char Str[MAX_STR_LEN];
	char Subroutine[80] = "HRRT_TimeAlignThread";

    struct tm *newtime;
    time_t long_time;

	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if Error Event Support was not fully istantiated, release it
	if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}

	// initialize stage percentage
	m_StagePercent = 0;
	m_TotalPercent = 0;
	sprintf(m_Stage, "Starting Time Alignment Pass");

	// log file name
    time(&long_time);
    newtime = localtime(&long_time);
	sprintf(LogFile, "%sLog\\%2.2d%s%4.4d_%2.2d%2.2d.log", m_RootPath, newtime->tm_mday, 
			MonthStr[newtime->tm_mon], 1900+newtime->tm_year, newtime->tm_hour, newtime->tm_min);

	// open log file
	m_fpLog = fopen(LogFile, "wt");
	if (m_fpLog == NULL) {
		sprintf(Str, "Failed To Open Time Alignment Log File: %s", LogFile);
		Status = Add_Error(pErrEvtSupThread, Subroutine, Str, 0);
	}

	// log start message
	Log_Message("Starting Time Alignment");

	// log model information
	sprintf(Str, "Model Number: %d", m_ModelNumber);
	Log_Message(Str);

	// only HRRT
	if (m_ScannerType != SCANNER_HRRT) Status = Add_Error(pErrEvtSupThread, Subroutine, "Invalid Scanner Type (HRRT Only), Model", m_ModelNumber);

	// Clear Status
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
		m_SetupSelect[i] = 0;
		m_SetupStatus[i] = 0;
	}

	// Time Alignment Type
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {
		if (m_HeadPresent[Head] != 0) {
			HeadSelect[Head] = 1;
			for (Block = 0 ; Block < m_NumBlocks ; Block++) m_SetupSelect[(Head*MAX_BLOCKS)+Block] = 1;
		} else {
			HeadSelect[Head] = 0;
		}
		LayerSelect[Head] = Layer;
	}


	// Create an instance of DHI Server.
	if (Status == 0) {
		pDHIThread = Get_DAL_Ptr(pErrEvtSupThread);
		if (pDHIThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get DAL Pointer", 0);
		if (Status != 0) Log_Message("Failed Attaching to Detector Abstraction Layer Server");
	}

	// Create an instance of Detector Utilities Server.
	if (Status == 0) {
		pDUThread = Get_DU_Ptr(pErrEvtSupThread);
		if (pDUThread == NULL) Status = Add_Error(pErrEvtSupThread, Subroutine, "Failed to Get DU Pointer", 0);
		if (Status != 0) Log_Message("Failed Attaching to Detector Utilities Server");
	}

	// bad status, set fatal error
	if (Status != 0) for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) if (m_SetupSelect[i] != 0) m_SetupStatus[i] = BLOCK_TIME_ALIGN_FATAL;

	// provide backup
	if (Status == 0) Status = Local_Backup(pErrEvtSupThread, pDHIThread, HeadSelect);

	// retrieve the current settings
	for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

		if (HeadSelect[Head] == 1) {

			// log message
			sprintf(Str, "Head %d: Retrieving Starting Analog Settings", Head);
			Log_Message(Str);

			// retrieve data
			index = Head * (MAX_BLOCKS * MAX_SETTINGS);
			hr = pDHIThread->Get_Analog_Settings(Config, Head, &Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
			if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
		}
	}

	// set duration
	Duration = m_EmTimeAlignDuration;

	// loop through the number of iterations designated
	for (Iteration = 0 ; (Iteration <= m_TimeAlignIterations) && (Status == 0) ; Iteration++) {

		// update stage percentage
		m_StagePercent = 0;
		m_TotalPercent = (Iteration * 95) / (m_TimeAlignIterations + 1);
		sprintf(Str, "Iteration %d of %d", Iteration, m_TimeAlignIterations);
		Log_Message(Str);

		// send the analog settings to the heads
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {
			if (HeadSelect[Head] == 1) {
				sprintf(Str, "Head %d: Downloading Analog Settings", Head);
				Log_Message(Str);
				index = Head * (MAX_BLOCKS * MAX_SETTINGS);
				hr = pDUThread->Store_Settings(SEND, Config, Head, &Settings[index], &Status);
				if (FAILED(hr)) {
					Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
					sprintf(Str, "Head %d: Failed to Download Analog Settings", Head);
					Log_Message(Str);
				}
			}
		}

		// shape discrimination values need to be redetermined each iteration
		if (Status == 0) {
		
			// determine new shape values
			Shape_Stage();

			// clear the stage percent complete and set the total percent complete
			m_StagePercent = 0;
			m_TotalPercent = (((2*Iteration)+1) * 95) / (2 * (m_TimeAlignIterations + 1));

			// retrieve the current settings again so that we have the correct shape discriminator values
			for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

				if (HeadSelect[Head] == 1) {

					// log message
					sprintf(Str, "Head %d: Retrieving Post Shape Discrimination Analog Settings", Head);
					Log_Message(Str);

					// retrieve data
					index = Head * (MAX_BLOCKS * MAX_SETTINGS);
					hr = pDHIThread->Get_Analog_Settings(Config, Head, &Settings[index], &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
					if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");
				}
			}
		}

		// acquire the crystal time data
		if (Status == 0) {
			Log_Message("Starting Time Alignment Acquisition");
			hr = pDUThread->Acquire_Coinc_Data(DATA_MODE_TIMING, Config, HeadSelect, LayerSelect, ModulePair, Transmission, 
												ACQUISITION_SECONDS, Duration, m_ConfigurationLLD[Config], m_ConfigurationULD[Config], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquire_Coinc_Data), HRESULT", hr);
			if (Status != 0) {
				for (Head = 0 ; Head < MAX_HEADS ; Head++) {
					if (HeadSelect[Head] == 1) Set_Head_Status(Head, BLOCK_TIME_ALIGN_FATAL);
				}
				Log_Message("Failed To Start Acquisition");
			}
		}

		// poll for progress of acquisition
		Percent = 0;
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) m_AcquireStatus[Head] = 0;
		while ((Percent < 100) && (Status == 0)) {
			hr = pDUThread->Acquisition_Progress(&Percent, m_AcquireStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Acquisition_Progress), HRESULT", hr);
			if (Status == 0) m_StagePercent = (Percent * 95) / 100;
			if (Percent < 100) Sleep(1000);
			if (Status != 0) Log_Message("Failed Acquisition Progress");
		}

		// success message
		if (Status == 0) Log_Message("Acquisition Complete");

		// do status for each head
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {
			if ((HeadSelect[Head] == 1) && (m_AcquireStatus[Head] != 0)) {
				sprintf(Str, "Head %d: Failed Acquisition", Head);
				Log_Message(Str);
				Status = -1;
			}
		}

		// print the composite centroid and fwhm for each head
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

			// if head is not being processed, go to next head
			if (HeadSelect[Head] == 0) continue;

			// log message
			sprintf(Str, "Head %d: Retrieving Crystal Time Data", Head);
			Log_Message(Str);

			// retrieve the just acquired crystal timing data
			hr = pDUThread->Current_Head_Data(DATA_MODE_TIMING, Config, Head, Layer, &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Log_Message("Failed to Retrieve Crystal Timing Data");
				Set_Head_Status(Head, BLOCK_TIME_ALIGN_FATAL);
			}

			// clear the composite histogram
			for (i = 0 ; i < m_TimingBins ; i++) Line[i] = 0;

			// loop through the blocks
			for (Block = 0 ; (Block < m_EmissionBlocks) && (Status == 0) ; Block++) {

				// Block Values
				Blk_X = Block % m_XBlocks;
				Blk_Y = Block / m_XBlocks;

				// loop through the crystals adding the counts and building a block histogram
				for (j = 0 ; (j < m_YCrystals) & (Status == 0) ; j++) {
					for (i = 0 ; (i < m_XCrystals) & (Status == 0) ; i++) {
						index = ((((Blk_Y * m_YCrystals) + j) * 128) + (Blk_X * m_XCrystals) + i) * m_TimingBins;
						for (k = 1 ; k < m_TimingBins ; k++) {
							Line[k] += Data[index+k];
						}
					}
				}
			}

			// find the centroid
			fwhm = peak_analyze(Line, m_TimingBins, &Centroid);
			sprintf(Str, "Head %d Average FWHM: % f bins,  Average Centroid %f", Head, fwhm, Centroid);
			Log_Message(Str);

			// release the data buffer
			if (Data != NULL) CoTaskMemFree(Data);
		}

		// don't process last iteration
		if (Iteration == m_TimeAlignIterations) continue;

		// process the heads
		for (Head = 0 ; (Head < MAX_HEADS) && (Status == 0) ; Head++) {

			// if head is not being processed, go to next head
			if (HeadSelect[Head] == 0) continue;

			// log message
			sprintf(Str, "Head %d: Retrieving Crystal Time Data", Head);
			Log_Message(Str);

			// retrieve the just acquired crystal timing data
			hr = pDUThread->Current_Head_Data(DATA_MODE_TIMING, Config, Head, Layer, &FileTime, &DataSize, &Data, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Current_Head_Data), HRESULT", hr);
			if (Status != 0) {
				Log_Message("Failed to Retrieve Crystal Timing Data");
				Set_Head_Status(Head, BLOCK_TIME_ALIGN_FATAL);
			}

			// loop through the blocks
			for (Block = 0 ; (Block < m_EmissionBlocks) && (Status == 0) ; Block++) {

				// log message
				sprintf(Str, "Head %d Block %d", Head, Block);
				Log_Message(Str);

				// Block Values
				Blk_X = Block % m_XBlocks;
				Blk_Y = Block / m_XBlocks;

				// clear the block histogram
				for (i = 0 ; i < m_TimingBins ; i++) Line[i] = 0;

				// loop through the crystals adding the counts and building a block histogram
				for (j = 0 ; (j < m_YCrystals) & (Status == 0) ; j++) {
					for (i = 0 ; (i < m_XCrystals) & (Status == 0) ; i++) {
						index = ((((Blk_Y * m_YCrystals) + j) * 128) + (Blk_X * m_XCrystals) + i) * m_TimingBins;
						for (Sum = 0.0, k = 1 ; k < m_TimingBins ; k++) {
							Sum += (double) Data[index+k];
							Line[k] += Data[index+k];
						}
					}
				}

				// number of counts
				sprintf(Str, "\tTotal Counts: %f", Sum);
				Log_Message(Str);

				// an average of 100 counts per pixel in the block (6400 in block)
				if (Sum >= Thresh) {

					// find the centroid
					fwhm = peak_analyze(Line, m_TimingBins, &Centroid);
					sprintf(Str, "\tFWHM: % f bins,  Centroid %f", fwhm, Centroid);
					Log_Message(Str);

					// reduce it down to the fraction
					Fraction = Centroid - (double)(m_TimingBins / 2);

					// apply the change (if it not likely to oscillate in the other direction)
					index = (Head * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings) + SetIndex[CFD_DELAY];
					Old_Value = Settings[index];
					if (fabs(Fraction) > 0.15) {
						if (Fraction > 0.0) {
							Settings[index]--;
						} else {
							Settings[index]++;
						}
					}

					// print change
					sprintf(Str, "\tOld: %3.1d New: %3.1d", Old_Value, Settings[index]);
					Log_Message(Str);
				} else {

					// not enough events, put warning on block
					Log_Message("Not Enough Counts");
					m_SetupStatus[(Head*MAX_BLOCKS)+Block] = BLOCK_TIME_ALIGN_NOT_FATAL;
				}
			}

			// release the data buffer
			if (Data != NULL) CoTaskMemFree(Data);
		}
	}

	// bad status, set fatal error
	if (Status != 0) for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) if (m_SetupSelect[i] != 0) m_SetupStatus[i] = BLOCK_TIME_ALIGN_FATAL;

	// close log file
	if (m_fpLog != NULL) fclose(m_fpLog);

	// release pointers
	pDHIThread->Release();
	pDUThread->Release();
	delete pErrEvtSupThread;

	// clear setup flag
	m_SetupFlag = 0;

	// clear stage string
	sprintf(m_Stage, "Setup Idle");

	// finished
	m_StagePercent = 100;
	m_TotalPercent = 100;
	Log_Message("Exiting HRRT Time Alignment");
	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		KillSelf
// Purpose:		exit server
// Arguments:	none
// Returns:		HRESULT
// History:		18 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDSMain::KillSelf()
{
	// release any servers
	Release_Servers();

	// cause the server to exit
	exit(0);

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
// History:		18 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(CErrorEventSupport *pErrSup, char *Where, char *What, long Why)
{
	// local variable
	long error_out = -1;
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_PDS";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// fire message to event handler
	if (pErrEvtSup != NULL) hr = pErrSup->Fire(GroupName, DSU_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);

	// return error code
	return error_out;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_DAL_Ptr
// Purpose:		retrieve a pointer to the Detector Abstraction Layer COM server (DHICOM)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		IAbstract_DHI* pointer to DHICOM
// History:		18 Sep 03	T Gremillion	History Started
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
// Routine:		Get_DU_Ptr
// Purpose:		retrieve a pointer to the Detector Utilities COM server (DUCOM)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		IDUMain* pointer to DUCOM
// History:		18 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

IDUMain* Get_DU_Ptr(CErrorEventSupport *pErrSup)
{
	// local variables
	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	IDUMain *Ptr = NULL;
	
	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// get server
	if (getenv("DUCOM_COMPUTER") == NULL) {
		sprintf(Server, "P39ACS");
	} else {
		sprintf(Server, "%s", getenv("DUCOM_COMPUTER"));
	}
	for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
	si.pwszName = Server_Name;

	// identify interface
	iid = IID_IDUMain;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	// get instance
	hr = ::CoCreateInstanceEx(CLSID_DUMain, NULL, CLSCTX_SERVER, &si, 1, mqi);
	if (FAILED(hr)) Add_Error(pErrSup, "Get_DU_Ptr", "Failed ::CoCreateInstanceEx(CLSID_DUMain, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (IDUMain*) mqi[0].pItf;

	return Ptr;
}

////////////////////////////////////////////////////////////////////////////////
// Name:	Set_Setup_Energy
// Purpose:	Generic Routine for Seting setup energy
// Arguments:	Configuration	-	long = Electronics Configuration
//				Head			-	long = Detector Head
//				Value			-	long = Value to set setup energy to
// Returns:		long status
// History:		18 Sep 03	T Gremillion	History Started
////////////////////////////////////////////////////////////////////////////////
long Set_Setup_Energy(long Configuration, long Head, long Value)
{
	// local variables
	long Block = 0;
	long Status = 0;

	long Settings[MAX_SETTINGS*MAX_BLOCKS];

	char Str[MAX_STR_LEN];
	char Subroutine[80] = "Set_Setup_Energy";

	HRESULT hr = S_OK;

	// Log Message
	sprintf(Str, "Configuration %d Head %d: Switching setup_eng to %d", Configuration, Head, Value);
	Log_Message(Str);

	// get the settings
	hr = pDHIThread->Get_Analog_Settings(Configuration, Head, Settings, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDHIThread->Get_Analog_Settings), HRESULT", hr);
	if (Status != 0) Log_Message("Failed to Retrieve Analog Settings");

	// succeeded getting settings
	if (Status == 0) {

		// set value to value
		for (Block = 0 ; Block < m_NumBlocks ; Block++) Settings[(Block*m_NumSettings)+SetIndex[SETUP_ENG]] = Value;

		// store in file and send to head
		hr = pDUThread->Store_Settings(SEND, Configuration, Head, Settings, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, Subroutine, "Method Failed (pDUThread->Store_Settings), HRESULT", hr);
		if (Status != 0) Log_Message("Failed to Set Analog Settings");
	}

	// return status value
	return Status;
}
