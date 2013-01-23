//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Detector Utilities Graphical User Interface Primary Dialog
// 
// Name:			DUGUIDlg.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	December 4, 2002
// 
// Description:		This component is the Detector Utilities main user interface.
//					It displays thumbnails of  all data for a selected head as well as
//					close up data and the current settings for a selected block.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#ifndef ECAT_SCANNER
#include "LockMgr.h"
#include "LockMgr_i.c"
#endif
#include "DUGUI.h"
#include "DUGUIDlg.h"
#include "CRMEdit.h"
#include "CrystalPeaks.h"
#include "CrystalTime.h"
#include "HeadSelection.h"
#include "LoadDlg.h"
#include "EnergyRange.h"
#include "InfoDialog.h"

#include "gantryModelClass.h"

#include "DHICom.h"
#include "DHICom_i.c"
#include "DUCom.h"
#include "DUCom_i.c"

#include "Setup_Constants.h"
#include "find_peak_1d.h"
#include "position_profile_tools.h"
#include "Validate.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_VIEWS 16
#define DATA_PMT_GAINS (NUM_DHI_MODES+1)
#define DATA_CRYSTAL_PEAKS (NUM_DHI_MODES+2)
#define DATA_TIME_OFFSETS (NUM_DHI_MODES+3)

#define STATE_SETTINGS_CHANGED 1
#define STATE_CRM_CHANGED 2
#define STATE_FAST_ENERGY_CHANGED 4
#define STATE_SLOW_ENERGY_CHANGED 8
#define STATE_FAST_TIME_CHANGED 16
#define STATE_SLOW_TIME_CHANGED 32

#define WM_USER_POST_PROGRESS		(WM_USER+1)
#define WM_USER_PROCESSING_COMPLETE	(WM_USER+2)
#define WM_USER_PROCESSING_MESSAGE	(WM_USER+3)

static long m_NumHeads;
static long m_XBlocks;
static long m_YBlocks;
static long m_XCrystals;
static long m_YCrystals;
static long m_NumBlocks;
static long m_TimingBins;
static long m_NumSettings;
static long m_ResetWait;
static long m_NumConfigurations;
static long m_ModelNumber;
static long m_ScannerType;
static long m_Display_X;
static long m_Display_Y;
static long m_Head_Pos_X;
static long m_Head_Pos_Y;
static long m_Block_Pos_X;
static long m_Block_Pos_Y;
static long m_PP_Smooth;
static long m_TE_Smooth;
static long m_PP_Min_Smooth;
static long m_PP_Max_Smooth;
static long m_TE_Min_Smooth;
static long m_TE_Max_Smooth;
static long m_ShapeLLD;
static long m_ShapeULD;
static long m_TubeEnergyLLD;
static long m_TubeEnergyULD;
static long m_CrystalEnergyLLD;
static long m_CrystalEnergyULD;
static long m_PositionProfileLLD;
static long m_PositionProfileULD;
static long m_CrossCheck;
static long m_CrossDist;
static long m_EdgeDist;
static long m_AcquirePercent;
static long m_Abort;
static long m_SaveCrystal;
static long m_SaveSmooth;
static long m_PeaksLLD[4];
static long m_PeaksULD[4];
static long m_TimeCrystal;
static long m_AcquireMaster;
static long m_Transmission;
static long m_HeadTestSet;
static long m_Simulation;
static long m_PositionWarningShown;
static long m_DisplayDownScale;
static long m_DisplayUpScale;

static long m_CurrView;
static long m_CurrHead;
static long m_CurrAcqHead;
static long m_CurrLayer;
static long m_CurrConfig;
static long m_CurrTube;
static long m_CurrBlock;
static long m_CurrAmount;
static long m_DataScaling;

static long m_ControlState;
static long m_AcquisitionFlag;

static long m_Phase;

static long m_Border_X1;
static long m_Border_X2;
static long m_Border_Y1;
static long m_Border_Y2;
static long m_Border_Range;

static __int64 m_LockCode = -1;
static 	__int64 m_BlockCounts;

static long m_NumLayers[MAX_HEADS];
static long m_MasterLayer[MAX_HEADS];
static long m_EnergyState[MAX_HEADS*MAX_CONFIGS];
static long m_HeadSelect[MAX_HEADS];
static long m_ConfigurationEnergy[MAX_CONFIGS];
static long m_ConfigurationLLD[MAX_CONFIGS];
static long m_ConfigurationULD[MAX_CONFIGS];
static long SetIndex[MAX_SETTINGS];
static long m_HeadPresent[MAX_HEADS];
static long m_HeadComm[MAX_HEADS];
static long m_SourcePresent[MAX_HEADS];
static long m_HeadIndex[MAX_HEADS+1];
static long m_LayerIndex[NUM_LAYER_TYPES];
static long m_ViewIndex[MAX_VIEWS];
static long m_Settings[MAX_CONFIGS*MAX_SETTINGS*MAX_TOTAL_BLOCKS];
static long m_Verts[MAX_HEAD_VERTICES*2];
static long m_Peaks[2*MAX_CONFIGS*MAX_HEADS*MAX_HEAD_CRYSTALS];
static long m_Time[2*MAX_CONFIGS*MAX_HEADS*MAX_HEAD_CRYSTALS];
static long m_PeaksValid[2*MAX_CONFIGS*MAX_HEADS];
static long m_TimeValid[2*MAX_CONFIGS*MAX_HEADS];
static long m_BlockState[MAX_CONFIGS*MAX_TOTAL_BLOCKS];

static long m_TubeOrder[4] = {PMTD, PMTB, PMTC, PMTA};

static double m_TickScale[5] = {1.0, 2.0, 2.5, 5.0, 10.0};

static long m_BlockImg[PP_SIZE];

static long *m_HeadData;
static long *m_MasterData;

static char m_MessageStr[MAX_STR_LEN];

static IAbstract_DHI *pDHI = NULL;
static IDUMain *pDU = NULL;

#ifndef ECAT_SCANNER
static ILockClass* pLock = NULL;
ILockClass* Get_Lock_Ptr(CErrorEventSupport *pErrSup);
#endif

static HANDLE m_hMutex = NULL;

IDUMain* Get_DU_Ptr(CErrorEventSupport *pErrSup);
IAbstract_DHI* Get_DAL_Ptr(CErrorEventSupport *pErrSup);

CErrorEventSupport* pErrEvtSup;
CErrorEventSupport* pErrEvtSupThread;

// thread handles
static HANDLE hProgressThread;

// window handle
static HANDLE m_MyWin;

// thread subroutines
void ProgressThread(void *dummy);

/////////////////////////////////////////////////////////////////////////////
// CDUGUIDlg dialog

CDUGUIDlg::CDUGUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDUGUIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDUGUIDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDUGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDUGUIDlg)
	DDX_Control(pDX, IDC_DEFAULTS, m_DefaultsButton);
	DDX_Control(pDX, IDC_ANALYZE, m_AnalyzeButton);
	DDX_Control(pDX, IDC_SAVE, m_SaveButton);
	DDX_Control(pDX, IDC_YOFF_LABEL, m_YOffLabel);
	DDX_Control(pDX, IDC_Y_LABEL, m_YLabel);
	DDX_Control(pDX, IDC_X_LABEL, m_XLabel);
	DDX_Control(pDX, IDC_VALUE_LABEL, m_ValueLabel);
	DDX_Control(pDX, IDC_UNITS_LABEL, m_UnitsLabel);
	DDX_Control(pDX, IDC_SHAPE_LABEL, m_ShapeLabel);
	DDX_Control(pDX, IDC_LAYER_LABEL, m_LayerLabel);
	DDX_Control(pDX, IDC_DISPLAY_LABEL, m_DisplayLabel);
	DDX_Control(pDX, IDC_C_LABEL, m_CLabel);
	DDX_Control(pDX, IDC_AMOUNT_LABEL, m_AmountLabel);
	DDX_Control(pDX, IDC_A_LABEL, m_ALabel);
	DDX_Control(pDX, IDC_XOFF_LABEL, m_XOffLabel);
	DDX_Control(pDX, IDC_VIEW_LABEL, m_ViewLabel);
	DDX_Control(pDX, IDC_GAIN_LABEL, m_GainLabel);
	DDX_Control(pDX, IDC_D_LABEL, m_DLabel);
	DDX_Control(pDX, IDC_CONFIG_LABEL, m_ConfigLabel);
	DDX_Control(pDX, IDC_CFD_LABEL, m_CFDLabel);
	DDX_Control(pDX, IDC_B_LABEL, m_BLabel);
	DDX_Control(pDX, IDC_ACQUISITION_LABEL, m_AcquisitionLabel);
	DDX_Control(pDX, IDC_PROGRESS_LABEL, m_ProgressLabel);
	DDX_Control(pDX, IDC_PROGRESS, m_AcquireProgress);
	DDX_Control(pDX, IDC_PREVIOUS, m_PreviousButton);
	DDX_Control(pDX, IDC_NEXT, m_NextButton);
	DDX_Control(pDX, IDC_EDIT, m_EditButton);
	DDX_Control(pDX, IDC_ACQUIRE, m_AcquireButton);
	DDX_Control(pDX, IDC_ABORT, m_AbortButton);
	DDX_Control(pDX, IDC_ACQDATE, m_AcquireDate);
	DDX_Control(pDX, IDC_BLOCK, m_Block);
	DDX_Control(pDX, IDC_CFD, m_CFD);
	DDX_Control(pDX, IDC_DNL_CHECK, m_DNLCheck);
	DDX_Control(pDX, IDC_DIVIDER_CHECK, m_DividerCheck);
	DDX_Control(pDX, IDC_FAST, m_FastLabel);
	DDX_Control(pDX, IDC_FAST_DASH, m_FastDash);
	DDX_Control(pDX, IDC_FAST_HIGH, m_FastHigh);
	DDX_Control(pDX, IDC_FAST_LOW, m_FastLow);
	DDX_Control(pDX, IDC_MASTER_CHECK, m_MasterCheck);
	DDX_Control(pDX, IDC_PEAKS_CHECK, m_PeaksCheck);
	DDX_Control(pDX, IDC_PMTB, m_PMTB);
	DDX_Control(pDX, IDC_PMTA, m_PMTA);
	DDX_Control(pDX, IDC_PMTC, m_PMTC);
	DDX_Control(pDX, IDC_PMTD, m_PMTD);
	DDX_Control(pDX, IDC_REGION_CHECK, m_RegionCheck);
	DDX_Control(pDX, IDC_SHAPE, m_Shape);
	DDX_Control(pDX, IDC_SLOW, m_SlowLabel);
	DDX_Control(pDX, IDC_SLOW_DASH, m_SlowDash);
	DDX_Control(pDX, IDC_SLOW_HIGH, m_SlowHigh);
	DDX_Control(pDX, IDC_SLOW_LOW, m_SlowLow);
	DDX_Control(pDX, IDC_SMOOTH_LABEL, m_SmoothLabel);
	DDX_Control(pDX, IDC_VALUE, m_Value);
	DDX_Control(pDX, IDC_SMOOTH_SLIDER, m_SmoothSlider);
	DDX_Control(pDX, IDC_THRESHOLD_CHECK, m_ThresholdCheck);
	DDX_Control(pDX, IDC_TRANSMISSION_CHECK, m_TransmissionCheck);
	DDX_Control(pDX, IDC_TUBEDROP, m_TubeDrop);
	DDX_Control(pDX, IDC_VARIABLE_FIELD, m_VariableField);
	DDX_Control(pDX, IDC_VARIABLE_LABEL, m_VariableLabel);
	DDX_Control(pDX, IDC_X_OFF, m_XOffset);
	DDX_Control(pDX, IDC_X_POS, m_XPos);
	DDX_Control(pDX, IDC_Y_OFF, m_YOffset);
	DDX_Control(pDX, IDC_Y_POS, m_YPos);
	DDX_Control(pDX, IDC_AMOUNT, m_Amount);
	DDX_Control(pDX, IDC_ACQUIRE_TYPEDROP, m_AcqTypeDrop);
	DDX_Control(pDX, IDC_LAYERDROP, m_LayerDrop);
	DDX_Control(pDX, IDC_VIEWDROP, m_ViewDrop);
	DDX_Control(pDX, IDC_CONFIGDROP, m_ConfigDrop);
	DDX_Control(pDX, IDC_ACQUIRE_HEADDROP, m_AcqHeadDrop);
	DDX_Control(pDX, IDC_HEADDROP, m_HeadDrop);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDUGUIDlg, CDialog)
	//{{AFX_MSG_MAP(CDUGUIDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_KILLFOCUS(IDC_AMOUNT, OnKillfocusAmount)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_PREVIOUS, OnPrevious)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_CBN_SELCHANGE(IDC_ACQUIRE_HEADDROP, OnSelchangeAcquireHeaddrop)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	ON_BN_CLICKED(IDC_DIVIDER_CHECK, OnDividerCheck)
	ON_CBN_SELCHANGE(IDC_VIEWDROP, OnSelchangeViewdrop)
	ON_CBN_SELCHANGE(IDC_HEADDROP, OnSelchangeHeaddrop)
	ON_CBN_SELCHANGE(IDC_LAYERDROP, OnSelchangeLayerdrop)
	ON_BN_CLICKED(IDC_REGION_CHECK, OnRegionCheck)
	ON_CBN_SELCHANGE(IDC_TUBEDROP, OnSelchangeTubedrop)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SMOOTH_SLIDER, OnReleasedcaptureSmoothSlider)
	ON_BN_CLICKED(IDC_MASTER_CHECK, OnMasterCheck)
	ON_BN_CLICKED(IDC_THRESHOLD_CHECK, OnThresholdCheck)
	ON_BN_CLICKED(IDC_PEAKS_CHECK, OnPeaksCheck)
	ON_WM_LBUTTONDOWN()
	ON_EN_KILLFOCUS(IDC_PMTA, OnKillfocusPmta)
	ON_EN_KILLFOCUS(IDC_PMTB, OnKillfocusPmtb)
	ON_EN_KILLFOCUS(IDC_PMTC, OnKillfocusPmtc)
	ON_EN_KILLFOCUS(IDC_PMTD, OnKillfocusPmtd)
	ON_EN_KILLFOCUS(IDC_X_OFF, OnKillfocusXOff)
	ON_EN_KILLFOCUS(IDC_Y_OFF, OnKillfocusYOff)
	ON_EN_KILLFOCUS(IDC_VARIABLE_FIELD, OnKillfocusVariableField)
	ON_EN_KILLFOCUS(IDC_CFD, OnKillfocusCfd)
	ON_EN_KILLFOCUS(IDC_FAST_HIGH, OnKillfocusFastHigh)
	ON_EN_KILLFOCUS(IDC_FAST_LOW, OnKillfocusFastLow)
	ON_EN_KILLFOCUS(IDC_SHAPE, OnKillfocusShape)
	ON_EN_KILLFOCUS(IDC_SLOW_HIGH, OnKillfocusSlowHigh)
	ON_EN_KILLFOCUS(IDC_SLOW_LOW, OnKillfocusSlowLow)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_ACQUIRE, OnAcquire)
	ON_MESSAGE(WM_USER_POST_PROGRESS, OnPostProgress)
	ON_MESSAGE(WM_USER_PROCESSING_COMPLETE, OnProcessingComplete)
	ON_MESSAGE(WM_USER_PROCESSING_MESSAGE, OnProcessingMessage)
	ON_BN_CLICKED(IDC_ABORT, OnAbort)
	ON_COMMAND(ID_DOWNLOAD_CRMS, OnDownloadCrms)
	ON_COMMAND(ID_ACQUIRE_MASTER_TUBE_ENERGY, OnAcquireMasterTubeEnergy)
	ON_COMMAND(ID_SET_ENERGY_RANGE, OnSetEnergyRange)
	ON_COMMAND(ID_FILE_ARCHIVE, OnFileArchive)
	ON_COMMAND(ID_FILE_DOWNLOADCRYSTALENERGY, OnFileDownloadcrystalenergy)
	ON_EN_KILLFOCUS(IDC_BLOCK, OnKillfocusBlock)
	ON_CBN_SELCHANGE(IDC_CONFIGDROP, OnSelchangeConfigdrop)
	ON_BN_CLICKED(IDC_ANALYZE, OnAnalyze)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_DEFAULTS, OnDefaults)
	ON_COMMAND(ID_FILE_DIAGNOSTICS, OnFileDiagnostics)
	ON_BN_CLICKED(IDC_DNL_CHECK, OnDnlCheck)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDUGUIDlg message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
//				16 Sep 03	T Gremillion	title bar is "Detector Analyzer" for ring scanners
//				29 Sep 03	T Gremillion	if no buckets detected, exit
//				21 Nov 03	T Gremillion	initialize acquisition flag
//				12 Dec 03	T Gremillion	added global variables to store LLD/ULD for each configuration
//				15 Apr 04	T Gremillion	use electronics phase instead of model number for ring scanners
//				14 Jun 04	T Gremillion	commented out standard icon
//				19 Oct 04	T Gremillion	changed default spacing for crosstalk from peaks to 8 from 6
//											at the request of Ziad Burbar and Raymond Roddy
/////////////////////////////////////////////////////////////////////////////

BOOL CDUGUIDlg::OnInitDialog()
{
	// local variables
	long i = 0;
	long j = 0;
	long X = 0;
	long Y = 0;
	long index = 0;
	long Rows = 0;
	long Layer = 0;
	long DataLayer = 0;
	long Next = 0;
	long Status = 0;
	long FromDAL = 1;

	long XPos = 0;
	long YPos = 0;
	long XSize = 1000;
	long YSize = 150;
	long Max_X = 640;
	long Max_Y = 832;

	long Amount_Offset = 0;
	long Unit_Offset = 0;
	long Button_Offset = 0;

	byte Force = 0;

	char Str[MAX_STR_LEN];

	unsigned char CommandString[MAX_STR_LEN] = "";
	unsigned char ResponseString[MAX_STR_LEN] = "";

	InfoDialog *pInfoDlg = NULL;

	WINDOWPLACEMENT Placement;

	HRESULT hr = S_OK;

	// Color
	COLORREF Color;
	HBRUSH hColorBrush;

	// instantiate a Gantry Object Model object
	CGantryModel model;
	pConf Config;

	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
//	SetIcon(m_hIcon, TRUE);			// Set big icon
//	SetIcon(m_hIcon, FALSE);		// Set small icon

	// get mutex handle
	m_hMutex = ::CreateMutex(NULL, TRUE, "DUGUI");

	// if there is already an instance of detector utilities, exit
	if (::GetLastError() != ERROR_SUCCESS) {
		PostMessage(WM_CLOSE);
		return TRUE;
	}

#ifdef ECAT_SCANNER

	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		MessageBox("Failed CoInitializeEx");
		PostMessage(WM_CLOSE);
		return TRUE;
	}

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
		MessageBox("Failed CoInitializeSecurity");
		PostMessage(WM_CLOSE);
		return TRUE;
	}
#else
	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport(true, true);

	// if error support not establisted, note it in log
	if (pErrEvtSup == NULL) {
		MessageBox("Failed to Create ErrorEventSupport");

	// if it was not fully istantiated, note it in log and release it
	} else if (pErrEvtSup->InitComplete(hr) == false) {
		delete pErrEvtSup;
		pErrEvtSup = NULL;
	}
#endif
	// Initialize Lock Code
	m_LockCode = -1;

	// initialize acquire master flag to false
	m_AcquireMaster = 0;
	m_AcquisitionFlag = 0;

	// clear crystal peaks and time
	for (i = 0 ; i < (2*MAX_CONFIGS*MAX_HEADS*MAX_HEAD_CRYSTALS) ; i++) {
		m_Peaks[i] = 0;
		m_Time[i] = 0;
	}
	
	// is this a head test set?
	m_HeadTestSet = 0;
	if (getenv("HEAD_TEST_SET") != NULL) if (strcmp(getenv("HEAD_TEST_SET"), "1") == 0) m_HeadTestSet = 1;

#ifndef ECAT_SCANNER
	// Create an instance of Lock Manager Server.
	if (Status == 0) {
		pLock = Get_Lock_Ptr(pErrEvtSup);
		if (pLock == NULL) {
			Status = Add_Error(pErrEvtSup, "OnInitDialog", "Failed to Get Lock Pointer", 0);
			PostMessage(WM_CLOSE);
			return TRUE;
		}
	}
#endif

	// Create an instance of Detector Utilities Server.
	if (Status == 0) {
		pDU = Get_DU_Ptr(pErrEvtSup);
		if (pDU == NULL) {
			Status = Add_Error(pErrEvtSup, "OnInitDialog", "Failed to Get DU Pointer", 0);
			MessageBox("Failed To Attach To Utilities Server", "Error", MB_OK);
			PostMessage(WM_CLOSE);
			return TRUE;
		}
	}

	// Create an instance of DHI Server.
	if (Status == 0) {
		pDHI = Get_DAL_Ptr(pErrEvtSup);
		if (pDHI == NULL) {
			Status = Add_Error(pErrEvtSup, "OnInitDialog", "Failed to Get DAL Pointer", 0);
			MessageBox("Failed To Attach To Detector Abstraction Layer Server", "Error", MB_OK);
			PostMessage(WM_CLOSE);
			return TRUE;
		}
	}

#ifndef ECAT_SCANNER
	// if instance created successfully, lock server
	if (Status == 0) {
		hr = pLock->LockACS(&m_LockCode, Force);

		// if server did not lock
		if (hr != S_OK) {

			// message
			if (FAILED(hr)) {
				Status = Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pLock->LockACS), HRESULT", hr);
				MessageBox("Method Failed:  pLock->LockACS", "Error", MB_OK);
			} else if (hr == S_FALSE) {
				Status = -1;
				MessageBox("Servers Already Locked", "Error", MB_OK);
			}

			// close dialog
			PostMessage(WM_CLOSE);
			return TRUE;
		}
	}
#endif

	// initialize block state
	for (i = 0 ; i < (MAX_CONFIGS*MAX_TOTAL_BLOCKS) ; i++) m_BlockState[i] = 0;

	// set gantry model to model number held in register
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) {
		MessageBox("Default Gantry Model Not Set", "Error", MB_OK);
		PostMessage(WM_CLOSE);
		return TRUE;
	}

	// set scanner model
	m_ModelNumber = model.modelNumber();

	// number of blocks
	m_NumBlocks = model.blocks();

	// initialize index settings
	for (i = 0 ; i < MAX_SETTINGS ; i++) SetIndex[i] = -1;

	// initialize Head arrays
	m_Transmission = 0;
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_HeadPresent[i] = 0;
		m_HeadComm[i] = 0;
		m_SourcePresent[i] = 0;
		m_HeadIndex[i] = 0;
		m_NumLayers[i] = 1;
		m_MasterLayer[i] = LAYER_FAST;

		// energy state ambiguous
		for (j = 0 ; j < m_NumConfigurations ; j++) m_EnergyState[(j*MAX_HEADS)+i] = -1;
	}

	// analog card settings
	m_NumSettings = model.nAnalogSettings();
	for (i = 0 ; i < MAX_SETTINGS ; i++) SetIndex[i] = -1;
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
	SetIndex[CFD_ENERGY_0] = model.cfd0Setting();
	SetIndex[CFD_ENERGY_1] = model.cfd1Setting();
	SetIndex[PEAK_STDEV] = model.stdevSetting();
	SetIndex[PHOTOPEAK] = model.peakSetting();
	SetIndex[SETUP_TEMP] = model.BucketTempSetting();

	// Configurations
	m_NumConfigurations = model.nConfig();
	Config = model.configInfo();
	for (i = 0 ; i < m_NumConfigurations ; i++) {
		m_ConfigDrop.InsertString(i, Config[i].name);
		m_ConfigurationEnergy[i] = Config[i].energy;
		m_ConfigurationLLD[i] = Config[i].lld;
		if (-1 == m_ConfigurationLLD[i]) m_ConfigurationLLD[i] = model.defaultLLD();
		m_ConfigurationULD[i] = Config[i].uld;
		if (-1 == m_ConfigurationULD[i]) m_ConfigurationULD[i] = model.defaultULD();
	}

	// number of timing bins
	m_TimingBins = model.timingBins();

// ECAT (Ring) Scanners
#ifdef ECAT_SCANNER

	// local variable
	long Percent = 0;
	long NumFound = 0;

	// set gantry type
	m_Phase = model.electronicsPhase();
	m_ScannerType = SCANNER_RING;
	Layer = LAYER_ALL;

	// phase I electronics not supported
	if (1 == m_Phase) {
		MessageBox("Phase I Electronics Not Supported", "Error", MB_OK);
		PostMessage(WM_CLOSE);
		return TRUE;
	}

	// dhi server is not in simulation
	m_Simulation = 0;

	// data scaling is linear for ring scanners
	m_DataScaling = 1;

	// number of emission blocks in each dimension
	m_XBlocks = model.transBlocks();
	m_YBlocks = model.axialBlocks();
	m_XCrystals = model.angularCrystals();
	m_YCrystals = model.axialCrystals();

	// number of seconds to wait after head reset
	m_ResetWait = 10 * 1000;

	// turn on the heads in sequence
	m_NumHeads = model.buckets();
	for (i = 0 ; i < m_NumHeads ; i++) {

		// mark as present
		m_HeadPresent[i] = 1;
		m_HeadComm[i] = 1;
		m_HeadIndex[i] = i;

		// if progress fails, then head is not there (progress is implemented for ring scanners)
		if (S_OK != pDHI->Progress(i, &Percent, &Status)) m_HeadComm[i] = 0;

		// droplist label
		sprintf(Str, "Bucket %d", i);
		m_HeadDrop.InsertString(i, Str);
		m_AcqHeadDrop.InsertString(i, Str);
	}
#else
	pHead Head;	

	// data scaling is histogram equalized for panel detector scanners
	m_DataScaling = 0;

	// set gantry type from model number
	switch (m_ModelNumber) {

	// PETSPECT
	case 311:
		m_Phase = 1;
		m_ScannerType = SCANNER_PETSPECT;
		Layer = LAYER_FAST;
		break;

	// HRRT
	case 328:

		// set scanner type and default layer
		m_Phase = 1;
		m_ScannerType = SCANNER_HRRT;
		Layer = LAYER_ALL;

		// send command - turn off singles polling
		sprintf((char *) CommandString, "L 0");
		hr = pDHI->Pass_Command(64, CommandString, ResponseString, &Status);
		if (FAILED(hr)) Add_Error(pErrEvtSup, "OnInitDialog", "Failed pDHI->Pass_Command(64, CommandString, ResponseString, &Status), HRESULT =", hr);

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
		Layer = LAYER_FAST;
		break;

	// P39 Phase IIA family (2391 - 2396)
	case 2391:
	case 2392:
	case 2393:
	case 2394:
	case 2395:
	case 2396:
		m_Phase = 2;
		m_ScannerType = SCANNER_P39_IIA;
		Layer = LAYER_ALL;
		break;

	// unknown gantry type - display message and exit
	default:
		MessageBox("Unrecognized Gantry Model", "Error", MB_OK);
		PostMessage(WM_CLOSE);
		return TRUE;
	}

	// determine if the DHI Server is a simulation
	m_Simulation = 0;
	if (PORT_SIMULATION == model.cpComm()) m_Simulation = 1;

	// number of emission blocks in each dimension
	if (model.isHeadRotated()) {
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

	// number of seconds to wait after head reset
	m_ResetWait = model.resetWaitDuration() * 1000;

	// initial lower and upper level discriminators for data types
	m_ShapeLLD = model.LLDshape();
	m_ShapeULD = model.ULDshape();
	m_TubeEnergyLLD = model.LLDtubeEnergy();
	m_TubeEnergyULD = model.ULDtubeEnergy();
	m_CrystalEnergyLLD = model.LLDcrystalEnergy();
	m_CrystalEnergyULD = model.ULDcrystalEnergy();
	m_PositionProfileLLD = model.LLDprofile();
	m_PositionProfileULD = model.ULDprofile();

	// get the data for the next head
	Head = model.headInfo();

	// dialog for informational messages
	pInfoDlg = new InfoDialog;

	// set head values
	m_NumHeads = model.nHeads();
	for (i = 0 ; i < m_NumHeads ; i++) {

		// globals
		m_HeadPresent[Head[i].address] = 1;
		m_HeadComm[Head[i].address] = 1;
		m_SourcePresent[Head[i].address] = Head[i].pointSrcData;
		if (m_SourcePresent[Head[i].address] != 0) m_Transmission = 1;
		m_HeadIndex[i] = Head[i].address;
		m_MasterLayer[Head[i].address] = Head[i].GainLayer;

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

		// droplist label
		sprintf(Str, "Head %d", Head[i].address);
		m_HeadDrop.InsertString(i, Str);
		m_AcqHeadDrop.InsertString(i, Str);

		// get analog settings, crystal peaks and time offsets for each configuration
		for (j = 0 ; j < m_NumConfigurations ; j++) {

			// put up informational message
			sprintf(pInfoDlg->Message, "Head %d Loading Configuration Settings", m_HeadIndex[i]);
			pInfoDlg->Create(IDD_INFO_DIALOG, NULL);
			pInfoDlg->ShowWindow(SW_SHOW);
			pInfoDlg->ShowMessage();

			// analog setting index
			index = ((j * MAX_HEADS) + m_HeadIndex[i]) * MAX_BLOCKS * MAX_SETTINGS;

			// if simulated DHI, retrieve stored values (if available)
			if (m_Simulation) {

				// stored values
				hr = pDU->Get_Stored_Settings(j, m_HeadIndex[i], &m_Settings[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pDU->Get_Stored_Settings), HRESULT", hr);
				if (Status == 0) FromDAL = 0;
			}

			// pull values from DAL
			if (FromDAL) {
				hr = pDHI->Get_Analog_Settings(j, m_HeadIndex[i], &m_Settings[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pDHI->Get_Analog_Settings), HRESULT", hr);
				if (Status != 0) {
					sprintf(Str, "Head %d: Failed to Retrieve All Settings", m_HeadIndex[i]);
					MessageBox(Str, "Error", MB_OK);
					PostMessage(WM_CLOSE);
				}
			}

			// if fast layer used (only not used for petspect in spect mode)
			if ((m_ScannerType != SCANNER_PETSPECT) || (j == 0)) {

				// P39 phase IIA electronics only have one layer, fast is used to store it
				DataLayer = LAYER_FAST;
				if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

				// get crystal peaks
				index = (j * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (m_HeadIndex[i] * MAX_HEAD_CRYSTALS * 2);
				hr = pDU->Get_Stored_Energy_Peaks(j, m_HeadIndex[i], DataLayer, &m_Peaks[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pDU->Get_Stored_Energy_Peaks), HRESULT", hr);
				if (Status != 0) {
					m_PeaksValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2)] = 0;
				} else {
					m_PeaksValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2)] = 1;
				}

				// get crystal time offsets
				hr = pDU->Get_Stored_Crystal_Time(j, m_HeadIndex[i], DataLayer, &m_Time[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pDU->Get_Stored_Crystal_Time), HRESULT", hr);
				if (Status != 0) {
					m_TimeValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2)] = 0;
				} else {
					m_TimeValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2)] = 1;
				}
			} else {
				m_PeaksValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2)] = 0;
				m_TimeValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2)] = 0;
			}

			// if slow layer used 
			if (m_NumLayers[m_HeadIndex[i]] != 1) {

				// get crystal peaks
				index = (j * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (m_HeadIndex[i] * MAX_HEAD_CRYSTALS * 2) + MAX_HEAD_CRYSTALS;
				hr = pDU->Get_Stored_Energy_Peaks(j, m_HeadIndex[i], LAYER_SLOW, &m_Peaks[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pDU->Get_Stored_Energy_Peaks), HRESULT", hr);
				if (Status != 0) {
					m_PeaksValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2) + 1] = 0;
				} else {
					m_PeaksValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2) + 1] = 1;
				}

				// get crystal time offsets
				hr = pDU->Get_Stored_Crystal_Time(j, m_HeadIndex[i], LAYER_SLOW, &m_Time[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pDU->Get_Stored_Crystal_Time), HRESULT", hr);
				if (Status != 0) {
					m_TimeValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2) + 1] = 0;
				} else {
					m_TimeValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2) + 1] = 1;
				}
			} else {
				m_PeaksValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2) + 1] = 0;
				m_TimeValid[(j * MAX_HEADS * 2) + (m_HeadIndex[i] * 2) + 1] = 0;
			}

			// remove the information window
			pInfoDlg->DestroyWindow();
		}

	}
#endif

	// initialize crystal peaks editor lld and uld
	for (i = 0 ; i < m_NumConfigurations ; i++) m_PeaksLLD[i] = m_ConfigurationLLD[i];
	for (i = 0 ; i < m_NumConfigurations ; i++) m_PeaksULD[i] = m_ConfigurationULD[i];

	// initialize head selection
	if (SCANNER_RING == m_ScannerType) {
		m_AcqHeadDrop.InsertString(m_NumHeads, "Multiple Buckets");
	} else {
		m_AcqHeadDrop.InsertString(m_NumHeads, "Multiple Heads");
	}
	m_HeadIndex[m_NumHeads] = -1;
	m_HeadDrop.SetCurSel(0);
	m_AcqHeadDrop.SetCurSel(0);
	m_CurrHead = 0;
	m_CurrAcqHead = 0;

	// determine downsize scaling
	for (m_DisplayDownScale = 1 ; (((PP_SIZE_X * m_XBlocks) / m_DisplayDownScale) > Max_X) || (((PP_SIZE_Y * m_YBlocks) / m_DisplayDownScale) > Max_Y) ; m_DisplayDownScale *= 2);

	// display size
	m_Display_X = m_XBlocks * (PP_SIZE_X / m_DisplayDownScale);
	if (m_Display_X < 512) m_Display_X = 512;
	Rows = ((m_NumBlocks % m_XBlocks) == 0) ? (m_NumBlocks / m_XBlocks) : ((m_NumBlocks / m_XBlocks) + 1);
	m_Display_Y = Rows * (PP_SIZE_Y / m_DisplayDownScale);
	if (m_Display_Y < 512) m_Display_Y = 512;
	YSize += m_Display_Y;

	// determine upsize scaling
	if (SCANNER_RING == m_ScannerType) {
		for (m_DisplayUpScale = 1 ; ((m_DisplayUpScale * m_XBlocks * m_XCrystals) < m_Display_X) && ((m_DisplayUpScale * m_YBlocks * m_YCrystals) < m_Display_Y) ; m_DisplayUpScale++);
		if (m_DisplayUpScale > 1) m_DisplayUpScale--;
	} else {
		m_DisplayUpScale = 4;
	}

	// resize window
	::SetWindowPos((HWND) m_hWnd, HWND_TOP, XPos, YPos, XSize, YSize, SWP_NOMOVE);

	// change the window title if ring scanner
	if (SCANNER_RING == m_ScannerType) ::SetWindowText((HWND) m_hWnd, "Detector Analyzer");

	// initialize configuration drop list
	m_ConfigDrop.SetCurSel(0);
	m_CurrConfig = 0;

	// Views (first ones that are always there)
	i = 0;
	m_ViewIndex[i] = DHI_MODE_POSITION_PROFILE;
	m_ViewDrop.InsertString(i, "Position Profile");

	i++;
	m_ViewIndex[i] = DHI_MODE_CRYSTAL_ENERGY;
	m_ViewDrop.InsertString(i, "Crystal Energy");

	i++;
	m_ViewIndex[i] = DHI_MODE_TUBE_ENERGY;
	m_ViewDrop.InsertString(i, "Tube Energy");

	i++;
	m_ViewIndex[i] = DHI_MODE_RUN;
	m_ViewDrop.InsertString(i, "Run Mode");

	if (SCANNER_RING != m_ScannerType) {
		i++;
		m_ViewIndex[i] = DATA_MODE_TIMING;
		m_ViewDrop.InsertString(i, "System Crystal Time");
	}

	if (SCANNER_RING != m_ScannerType) {
		i++;
		m_ViewIndex[i] = DATA_PMT_GAINS;
		m_ViewDrop.InsertString(i, "PMT Gains");
	}

	if (SCANNER_RING != m_ScannerType) {
		i++;
		m_ViewIndex[i] = DATA_CRYSTAL_PEAKS;
		m_ViewDrop.InsertString(i, "Crystal Peaks");
	}

	// variable entries start
	if ((m_ScannerType == SCANNER_HRRT) || (m_ScannerType == SCANNER_PETSPECT)) {
		i++;
		m_ViewIndex[i] = DHI_MODE_SHAPE_DISCRIMINATION;
		m_ViewDrop.InsertString(i, "Shape");
	}

	if ((m_ScannerType != SCANNER_HRRT) && (m_ScannerType != SCANNER_RING)) {
		i++;
		m_ViewIndex[i] = DATA_TIME_OFFSETS;
		m_ViewDrop.InsertString(i, "Time Offsets");
	}

	if (m_ScannerType == SCANNER_PETSPECT) {
		i++;
		m_ViewIndex[i] = DHI_MODE_SPECT;
		m_ViewDrop.InsertString(i, "SPECT");
	}

	if ((m_ScannerType == SCANNER_P39_IIA) || (m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_HRRT) || (m_ScannerType == SCANNER_RING)) {
		i++;
		m_ViewIndex[i] = DHI_MODE_TIME;
		m_ViewDrop.InsertString(i, "TDC Crystal Time");
	}

	// initial view for ring scanners is position profile, run for panel detectors
	if (SCANNER_RING == m_ScannerType) {
		m_CurrView = DHI_MODE_POSITION_PROFILE;
	} else {
		m_CurrView = DHI_MODE_RUN;
	}
	for (j = 0 ; (j <= i) && (m_ViewIndex[j] != m_CurrView) ; j++);
	if (m_ViewIndex[j] == m_CurrView) {
		m_ViewDrop.SetCurSel(j);
	} else {
		m_CurrView = m_ViewIndex[0];
		m_ViewDrop.SetCurSel(0);
	}

	// Layer
	if ((m_ScannerType != SCANNER_P39_IIA) && (m_ScannerType != SCANNER_RING)) {
		m_LayerDrop.InsertString(LAYER_FAST, "Fast Layer");
		m_LayerIndex[LAYER_FAST] = LAYER_FAST;
		m_LayerDrop.InsertString(LAYER_SLOW, "Slow Layer");
		m_LayerIndex[LAYER_SLOW] = LAYER_SLOW;
		m_LayerDrop.InsertString(LAYER_ALL, "Combined Layers");
		m_LayerIndex[LAYER_ALL] = LAYER_ALL;
		m_LayerDrop.SetCurSel(Layer);
	} else {
		m_LayerDrop.InsertString(0, "Only Layer");
		m_LayerIndex[0] = Layer;
		m_LayerDrop.SetCurSel(0);
	}
	m_CurrLayer = Layer;

	// Acquisition Type (Seconds or Blocks)
	m_AcqTypeDrop.InsertString(ACQUISITION_SECONDS, "Seconds");
	m_AcqTypeDrop.InsertString(ACQUISITION_BLOCKS, "KEvents");
	m_AcqTypeDrop.SetCurSel(ACQUISITION_SECONDS);

	// Tubes
	m_TubeDrop.InsertString(0, "Tube A");
	m_TubeDrop.InsertString(1, "Tube B");
	m_TubeDrop.InsertString(2, "Tube C");
	m_TubeDrop.InsertString(3, "Tube D");
	m_TubeDrop.InsertString(4, "All Tubes");
	m_CurrTube = 4;
	m_TubeDrop.SetCurSel(m_CurrTube);

	// Initial values for fields not dependent on head/block information
	m_CurrAmount = 60;
	m_Amount.SetWindowText("60");
	m_CurrBlock = 0;
	m_Block.SetWindowText("0");

	// device context
	CPaintDC dc(this);

	// Initialize Greyscale Color Table
	for (i = 0 ; i < 256 ; i++)	{
		Color = RGB(i, i, i);
		hdcColor[i] = CreateCompatibleDC(dc);
		hColorBlock[i] = CreateCompatibleBitmap(dc, 1, 1);
		SelectObject(hdcColor[i], hColorBlock[i]);
		hColorBrush = CreateSolidBrush(Color);
		SelectObject(hdcColor[i], hColorBrush);
		PatBlt(hdcColor[i], 0, 0, 1, 1, PATCOPY);
		DeleteObject(hColorBrush);
	}

	// Create a memory DC and bitmap and associate them for the data
	m_bmHead = CreateCompatibleBitmap(dc, 640, 832);
	m_dcHead = ::CreateCompatibleDC(dc);
	SelectObject(m_dcHead, m_bmHead);
	m_bmBlock = CreateCompatibleBitmap(dc, 256, 256);
	m_dcBlock = ::CreateCompatibleDC(dc);
	SelectObject(m_dcBlock, m_bmBlock);

	// sub-window positions
	m_Head_Pos_X = 10;
	m_Head_Pos_Y = 100;
	m_Block_Pos_X = 660;
	m_Block_Pos_Y = 155;

	// initialize controls
	m_DividerCheck.SetCheck(1);
	m_RegionCheck.SetCheck(1);
	m_ThresholdCheck.SetCheck(1);
	m_PeaksCheck.SetCheck(1);
	m_PP_Smooth = 1;
	m_TE_Smooth = 0;
	m_PP_Min_Smooth = 1;
	m_PP_Max_Smooth = 15;
	m_TE_Min_Smooth = 0;
	m_TE_Max_Smooth = 25;
	m_CrossCheck = 0;
	m_CrossDist = 8;
	m_EdgeDist = 5;
	m_SaveCrystal = 0;
	m_TimeCrystal = 0;
	m_SaveSmooth = 0;

	// initialize progress thread handle
	hProgressThread = NULL;

	// Positively position controls
	m_ViewLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 10;
	Placement.rcNormalPosition.right = 130;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 16;
	m_ViewLabel.SetWindowPlacement(&Placement);

	m_ViewDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 10;
	Placement.rcNormalPosition.right = 130;
	Placement.rcNormalPosition.top = 17;
	Placement.rcNormalPosition.bottom = 33;
	m_ViewDrop.SetWindowPlacement(&Placement);

	m_DisplayLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 10;
	Placement.rcNormalPosition.right = 130;
	Placement.rcNormalPosition.top = 39;
	Placement.rcNormalPosition.bottom = 55;
	m_DisplayLabel.SetWindowPlacement(&Placement);
	if (SCANNER_RING == m_ScannerType) m_DisplayLabel.SetWindowText("Display Bucket");

	m_HeadDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 10;
	Placement.rcNormalPosition.right = 130;
	Placement.rcNormalPosition.top = 56;
	Placement.rcNormalPosition.bottom = 72;
	m_HeadDrop.SetWindowPlacement(&Placement);

	m_AcquireDate.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 10;
	Placement.rcNormalPosition.right = 220;
	Placement.rcNormalPosition.top = 78;
	Placement.rcNormalPosition.bottom = 94;
	m_AcquireDate.SetWindowPlacement(&Placement);
	m_Head_Pos_Y = Placement.rcNormalPosition.bottom + 2;

	m_ConfigLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 135;
	Placement.rcNormalPosition.right = 255;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 16;
	m_ConfigLabel.SetWindowPlacement(&Placement);

	m_ConfigDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 135;
	Placement.rcNormalPosition.right = 255;
	Placement.rcNormalPosition.top = 17;
	Placement.rcNormalPosition.bottom = 33;
	m_ConfigDrop.SetWindowPlacement(&Placement);

	m_LayerLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 135;
	Placement.rcNormalPosition.right = 255;
	Placement.rcNormalPosition.top = 39;
	Placement.rcNormalPosition.bottom = 55;
	m_LayerLabel.SetWindowPlacement(&Placement);

	m_LayerDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 135;
	Placement.rcNormalPosition.right = 255;
	Placement.rcNormalPosition.top = 56;
	Placement.rcNormalPosition.bottom = 72;
	m_LayerDrop.SetWindowPlacement(&Placement);

	// acquisition controls need to be offset if layer and configuration not shown (RING SCANNERS)
	if (SCANNER_RING == m_ScannerType) {
		Amount_Offset = 70;
		Unit_Offset = 95;
		Button_Offset = 120;
	}

	m_AcquireButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 260 - Button_Offset;
	Placement.rcNormalPosition.right = 340 - Button_Offset;
	Placement.rcNormalPosition.top = 10;
	Placement.rcNormalPosition.bottom = 38;
	m_AcquireButton.SetWindowPlacement(&Placement);

	m_AbortButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 260 - Button_Offset;
	Placement.rcNormalPosition.right = 340 - Button_Offset;
	Placement.rcNormalPosition.top = 49;
	Placement.rcNormalPosition.bottom = 77;
	m_AbortButton.SetWindowPlacement(&Placement);

	m_ProgressLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 290 - Amount_Offset;
	Placement.rcNormalPosition.right = 340 - Amount_Offset;
	Placement.rcNormalPosition.top = 78;
	Placement.rcNormalPosition.bottom = 94;
	m_ProgressLabel.SetWindowPlacement(&Placement);

	m_AcquisitionLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 345 - Unit_Offset;
	Placement.rcNormalPosition.right = 465 - Unit_Offset;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 16;
	m_AcquisitionLabel.SetWindowPlacement(&Placement);
	if (SCANNER_RING == m_ScannerType) m_AcquisitionLabel.SetWindowText("Acquisition Bucket");

	m_AcqHeadDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 345 - Unit_Offset;
	Placement.rcNormalPosition.right = 465 - Unit_Offset;
	Placement.rcNormalPosition.top = 17;
	Placement.rcNormalPosition.bottom = 33;
	m_AcqHeadDrop.SetWindowPlacement(&Placement);

	m_UnitsLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 345 - Unit_Offset;
	Placement.rcNormalPosition.right = 465 - Unit_Offset;
	Placement.rcNormalPosition.top = 39;
	Placement.rcNormalPosition.bottom = 55;
	m_UnitsLabel.SetWindowPlacement(&Placement);

	m_AcqTypeDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 345 - Unit_Offset;
	Placement.rcNormalPosition.right = 465 - Unit_Offset;
	Placement.rcNormalPosition.top = 56;
	Placement.rcNormalPosition.bottom = 72;
	m_AcqTypeDrop.SetWindowPlacement(&Placement);

	m_AcquireProgress.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 345 - Amount_Offset;
	Placement.rcNormalPosition.right = 590 - Amount_Offset;
	Placement.rcNormalPosition.top = 81;
	Placement.rcNormalPosition.bottom = 92;
	m_AcquireProgress.SetWindowPlacement(&Placement);

	m_TransmissionCheck.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 470 - Amount_Offset;
	Placement.rcNormalPosition.right = 590 - Amount_Offset;
	Placement.rcNormalPosition.top = 17;
	Placement.rcNormalPosition.bottom = 33;
	m_TransmissionCheck.SetWindowPlacement(&Placement);

	m_AmountLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 470 - Amount_Offset;
	Placement.rcNormalPosition.right = 590 - Amount_Offset;
	Placement.rcNormalPosition.top = 39;
	Placement.rcNormalPosition.bottom = 55;
	m_AmountLabel.SetWindowPlacement(&Placement);

	m_Amount.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 470 - Amount_Offset;
	Placement.rcNormalPosition.right = 590 - Amount_Offset;
	Placement.rcNormalPosition.top = 56;
	Placement.rcNormalPosition.bottom = 77;
	m_Amount.SetWindowPlacement(&Placement);

	m_GainLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 595;
	Placement.rcNormalPosition.right = 705;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 16;
	m_GainLabel.SetWindowPlacement(&Placement);

	m_DLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 595;
	Placement.rcNormalPosition.right = 605;
	Placement.rcNormalPosition.top = 22;
	Placement.rcNormalPosition.bottom = 38;
	m_DLabel.SetWindowPlacement(&Placement);

	m_CLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 595;
	Placement.rcNormalPosition.right = 605;
	Placement.rcNormalPosition.top = 44;
	Placement.rcNormalPosition.bottom = 60;
	m_CLabel.SetWindowPlacement(&Placement);

	m_PMTD.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 605;
	Placement.rcNormalPosition.right = 650;
	Placement.rcNormalPosition.top = 17;
	Placement.rcNormalPosition.bottom = 38;
	m_PMTD.SetWindowPlacement(&Placement);

	m_PMTC.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 605;
	Placement.rcNormalPosition.right = 650;
	Placement.rcNormalPosition.top = 39;
	Placement.rcNormalPosition.bottom = 60;
	m_PMTC.SetWindowPlacement(&Placement);

	m_PMTB.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 655;
	Placement.rcNormalPosition.right = 700;
	Placement.rcNormalPosition.top = 17;
	Placement.rcNormalPosition.bottom = 38;
	m_PMTB.SetWindowPlacement(&Placement);

	m_PMTA.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 655;
	Placement.rcNormalPosition.right = 700;
	Placement.rcNormalPosition.top = 39;
	Placement.rcNormalPosition.bottom = 60;
	m_PMTA.SetWindowPlacement(&Placement);

	m_BLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 700;
	Placement.rcNormalPosition.right = 710;
	Placement.rcNormalPosition.top = 22;
	Placement.rcNormalPosition.bottom = 38;
	m_BLabel.SetWindowPlacement(&Placement);

	m_ALabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 700;
	Placement.rcNormalPosition.right = 710;
	Placement.rcNormalPosition.top = 44;
	Placement.rcNormalPosition.bottom = 60;
	m_ALabel.SetWindowPlacement(&Placement);

	m_XOffLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 710;
	Placement.rcNormalPosition.right = 770;
	Placement.rcNormalPosition.top = 5;
	Placement.rcNormalPosition.bottom = 21;
	m_XOffLabel.SetWindowPlacement(&Placement);

	m_YOffLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 710;
	Placement.rcNormalPosition.right = 770;
	Placement.rcNormalPosition.top = 27;
	Placement.rcNormalPosition.bottom = 43;
	m_YOffLabel.SetWindowPlacement(&Placement);

	m_VariableLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 710;
	Placement.rcNormalPosition.right = 770;
	Placement.rcNormalPosition.top = 49;
	Placement.rcNormalPosition.bottom = 65;
	m_VariableLabel.SetWindowPlacement(&Placement);

	m_XOffset.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 775;
	Placement.rcNormalPosition.right = 815;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 22;
	m_XOffset.SetWindowPlacement(&Placement);

	m_YOffset.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 775;
	Placement.rcNormalPosition.right = 815;
	Placement.rcNormalPosition.top = 22;
	Placement.rcNormalPosition.bottom = 44;
	m_YOffset.SetWindowPlacement(&Placement);

	m_VariableField.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 775;
	Placement.rcNormalPosition.right = 815;
	Placement.rcNormalPosition.top = 44;
	Placement.rcNormalPosition.bottom = 66;
	m_VariableField.SetWindowPlacement(&Placement);

	m_CFDLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 820;
	Placement.rcNormalPosition.right = 880;
	Placement.rcNormalPosition.top = 5;
	Placement.rcNormalPosition.bottom = 21;
	m_CFDLabel.SetWindowPlacement(&Placement);

	m_ShapeLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 820;
	Placement.rcNormalPosition.right = 880;
	Placement.rcNormalPosition.top = 27;
	Placement.rcNormalPosition.bottom = 43;
	m_ShapeLabel.SetWindowPlacement(&Placement);

	m_SlowLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 820;
	Placement.rcNormalPosition.right = 880;
	Placement.rcNormalPosition.top = 49;
	Placement.rcNormalPosition.bottom = 65;
	m_SlowLabel.SetWindowPlacement(&Placement);

	m_FastLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 820;
	Placement.rcNormalPosition.right = 880;
	Placement.rcNormalPosition.top = 71;
	Placement.rcNormalPosition.bottom = 87;
	m_FastLabel.SetWindowPlacement(&Placement);

	m_CFD.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 885;
	Placement.rcNormalPosition.right = 925;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 21;
	m_CFD.SetWindowPlacement(&Placement);

	m_Shape.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 885;
	Placement.rcNormalPosition.right = 925;
	Placement.rcNormalPosition.top = 22;
	Placement.rcNormalPosition.bottom = 43;
	m_Shape.SetWindowPlacement(&Placement);

	m_SlowLow.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 885;
	Placement.rcNormalPosition.right = 925;
	Placement.rcNormalPosition.top = 44;
	Placement.rcNormalPosition.bottom = 65;
	m_SlowLow.SetWindowPlacement(&Placement);

	m_SlowDash.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 925;
	Placement.rcNormalPosition.right = 935;
	Placement.rcNormalPosition.top = 44;
	Placement.rcNormalPosition.bottom = 65;
	m_SlowDash.SetWindowPlacement(&Placement);

	m_SlowHigh.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 935;
	Placement.rcNormalPosition.right = 975;
	Placement.rcNormalPosition.top = 44;
	Placement.rcNormalPosition.bottom = 65;
	m_SlowHigh.SetWindowPlacement(&Placement);

	m_FastLow.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 885;
	Placement.rcNormalPosition.right = 925;
	Placement.rcNormalPosition.top = 66;
	Placement.rcNormalPosition.bottom = 87;
	m_FastLow.SetWindowPlacement(&Placement);

	m_FastDash.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 925;
	Placement.rcNormalPosition.right = 935;
	Placement.rcNormalPosition.top = 66;
	Placement.rcNormalPosition.bottom = 87;
	m_FastDash.SetWindowPlacement(&Placement);

	m_FastHigh.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = 935;
	Placement.rcNormalPosition.right = 975;
	Placement.rcNormalPosition.top = 66;
	Placement.rcNormalPosition.bottom = 87;
	m_FastHigh.SetWindowPlacement(&Placement);

	m_DividerCheck.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 70;
	Placement.rcNormalPosition.top = m_Head_Pos_Y;
	Placement.rcNormalPosition.bottom = m_Head_Pos_Y + 21;
	m_DividerCheck.SetWindowPlacement(&Placement);

	m_XLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 10;
	Placement.rcNormalPosition.top = m_Head_Pos_Y + 32;
	Placement.rcNormalPosition.bottom = m_Head_Pos_Y + 48;
	m_XLabel.SetWindowPlacement(&Placement);
	m_Block_Pos_Y = Placement.rcNormalPosition.bottom + 5;

	m_XPos.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 10;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 70;
	Placement.rcNormalPosition.top = m_Head_Pos_Y + 27;
	Placement.rcNormalPosition.bottom = m_Head_Pos_Y + 48;
	m_XPos.SetWindowPlacement(&Placement);

	m_YLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 70;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 80;
	Placement.rcNormalPosition.top = m_Head_Pos_Y + 32;
	Placement.rcNormalPosition.bottom = m_Head_Pos_Y + 48;
	m_YLabel.SetWindowPlacement(&Placement);

	m_YPos.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 80;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 140;
	Placement.rcNormalPosition.top = m_Head_Pos_Y + 27;
	Placement.rcNormalPosition.bottom = m_Head_Pos_Y + 48;
	m_YPos.SetWindowPlacement(&Placement);

	m_ValueLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 140;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 170;
	Placement.rcNormalPosition.top = m_Head_Pos_Y + 32;
	Placement.rcNormalPosition.bottom = m_Head_Pos_Y + 48;
	m_ValueLabel.SetWindowPlacement(&Placement);

	m_Value.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 170;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 256;
	Placement.rcNormalPosition.top = m_Head_Pos_Y + 27;
	Placement.rcNormalPosition.bottom = m_Head_Pos_Y + 48;
	m_Value.SetWindowPlacement(&Placement);

	// ring scanners - don't show regions or peaks on main display (time considerations)
	Next = 0;
	if (SCANNER_RING != m_ScannerType) { 
		m_RegionCheck.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Block_Pos_X + 260;
		Placement.rcNormalPosition.right = m_Block_Pos_X + 260 + 70;
		Placement.rcNormalPosition.top = m_Block_Pos_Y + (Next * 25);
		Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 22;
		m_RegionCheck.SetWindowPlacement(&Placement);
		Next++;

		m_PeaksCheck.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Block_Pos_X + 260;
		Placement.rcNormalPosition.right = m_Block_Pos_X + 260 + 70;
		Placement.rcNormalPosition.top = m_Block_Pos_Y + (Next * 25);
		Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 47;
		m_PeaksCheck.SetWindowPlacement(&Placement);
		Next++;
	}

	// dnl check box if not phase IIA electronics
	if ((SCANNER_P39_IIA != m_ScannerType) && (m_Phase != 2.0)) {
		m_DNLCheck.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Block_Pos_X + 260;
		Placement.rcNormalPosition.right = m_Block_Pos_X + 260 + 70;
		Placement.rcNormalPosition.top = m_Block_Pos_Y + (Next * 25);
		Placement.rcNormalPosition.bottom = Placement.rcNormalPosition.top + 22;
		Next++;
		m_DNLCheck.SetWindowPlacement(&Placement);
		m_DNLCheck.SetCheck(1);
	}

	// shape theshold check box only if hrrt or petspect
	if ((m_ScannerType == SCANNER_HRRT) || (m_ScannerType == SCANNER_PETSPECT)) {
		m_ThresholdCheck.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Block_Pos_X + 260;
		Placement.rcNormalPosition.right = m_Block_Pos_X + 260 + 70;
		Placement.rcNormalPosition.top = m_Block_Pos_Y + (Next * 25);
		Placement.rcNormalPosition.bottom = Placement.rcNormalPosition.top + 22;
		Next++;
		m_ThresholdCheck.SetWindowPlacement(&Placement);
	}

	// master tube energy not available for ring scanner
	if (SCANNER_RING != m_ScannerType) {
		m_MasterCheck.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Block_Pos_X + 260;
		Placement.rcNormalPosition.right = m_Block_Pos_X + 260 + 70;
		Placement.rcNormalPosition.top = m_Block_Pos_Y + (Next * 25);
		Placement.rcNormalPosition.bottom = Placement.rcNormalPosition.top + 22;
		Next++;
		m_MasterCheck.SetWindowPlacement(&Placement);
	}

	m_TubeDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 260;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 260 + 70;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + (Next * 25);
	Placement.rcNormalPosition.bottom = Placement.rcNormalPosition.top + 100;
	Next++;
	m_TubeDrop.SetWindowPlacement(&Placement);

	m_PreviousButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 50;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 260;
	Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 260 + 22;
	m_PreviousButton.SetWindowPlacement(&Placement);

	m_Block.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 55;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 105;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 260;
	Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 260 + 22;
	m_Block.SetWindowPlacement(&Placement);

	m_NextButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 110;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 160;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 260;
	Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 260 + 22;
	m_NextButton.SetWindowPlacement(&Placement);

	m_EditButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X + 185;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 255;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 260;
	Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 260 + 22;
	m_EditButton.SetWindowPlacement(&Placement);
	if (SCANNER_RING == m_ScannerType) m_EditButton.SetWindowText("View");

	m_SmoothLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 50;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 290;
	Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 312;
	m_SmoothLabel.SetWindowPlacement(&Placement);

	m_SmoothSlider.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 255;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 312;
	Placement.rcNormalPosition.bottom = m_Block_Pos_Y + 332;
	m_SmoothSlider.SetWindowPlacement(&Placement);

	m_DefaultsButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Block_Pos_X;
	Placement.rcNormalPosition.right = Placement.rcNormalPosition.left + 80;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 342;
	Placement.rcNormalPosition.bottom= m_Block_Pos_Y + 362;
	m_DefaultsButton.SetWindowPlacement(&Placement);

	m_AnalyzeButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left  = m_Block_Pos_X + 85;
	Placement.rcNormalPosition.right  = m_Block_Pos_X + 165;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 342;
	Placement.rcNormalPosition.bottom= m_Block_Pos_Y + 362;
	m_AnalyzeButton.SetWindowPlacement(&Placement);

	m_SaveButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left  = m_Block_Pos_X + 170;
	Placement.rcNormalPosition.right = m_Block_Pos_X + 255;
	Placement.rcNormalPosition.top = m_Block_Pos_Y + 342;
	Placement.rcNormalPosition.bottom= m_Block_Pos_Y + 362;
	m_SaveButton.SetWindowPlacement(&Placement);

	// if the DAL is in simulation, the following menu options won't work so are disabled
	if (m_Simulation) {
		CMenu *pMenu = GetMenu();
		pMenu->EnableMenuItem(ID_ACQUIRE_MASTER_TUBE_ENERGY, MF_GRAYED);
		pMenu->EnableMenuItem(ID_FILE_ARCHIVE, MF_GRAYED);
		pMenu->EnableMenuItem(ID_FILE_DIAGNOSTICS, MF_GRAYED);
		pMenu->EnableMenuItem(ID_DOWNLOAD_CRMS, MF_GRAYED);
		pMenu->EnableMenuItem(ID_FILE_DOWNLOADCRYSTALENERGY, MF_GRAYED);
		pMenu->EnableMenuItem(ID_SET_ENERGY_RANGE, MF_GRAYED);
	}

	// if ring scanner
	if (SCANNER_RING == m_ScannerType) {
		CMenu *pMenu = GetMenu();
		pMenu->DeleteMenu(ID_ACQUIRE_MASTER_TUBE_ENERGY, MF_BYCOMMAND);
		pMenu->DeleteMenu(ID_FILE_ARCHIVE, MF_BYCOMMAND);
		pMenu->DeleteMenu(ID_FILE_DIAGNOSTICS, MF_BYCOMMAND);
		pMenu->DeleteMenu(ID_DOWNLOAD_CRMS, MF_BYCOMMAND);
		pMenu->DeleteMenu(ID_FILE_DOWNLOADCRYSTALENERGY, MF_BYCOMMAND);
		pMenu->DeleteMenu(ID_SET_ENERGY_RANGE, MF_BYCOMMAND);
	}

	// initial data
	UpdateHead();

	// keep track of the window handle
	m_MyWin = m_hWnd;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPaint
// Purpose:		GUI Refresh
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				03 Oct 03	T Gremillion	swithched border variables over to global
//				21 Nov 03	T Gremillion	Max Value for scaling not allowed below 8 for scaling (Shape)
//				12 Jan 04	T Gremillion	Added display of total counts for block
//											for Tube Energy and Shape Discrimination
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnPaint() 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long View = 0;
	long Head = 0;
	long Config = 0;
	long Block = 0;
	long Layer = 0;
	long Tube = 0;
	long x1 = 0;
	long x2 = 0;
	long y1 = 0;
	long y2 = 0;
	long XVerts = 0;
	long YVerts = 0;
	long BlockVerts = 0;
	long MaxValue = 0;
	long LastValue = 0;
	long YRng = 0;
	long NumTicks = 0;
	long Power = 0;
	long TickDist = 0;
	long Max_X = 0;
	long Tick_X = 0;
	long ShapeValue = 0;

	long Line_X[4] = {1, 2, 1, 0};
	long Line_Y[4] = {-1, 0, 1, 0};

	double TickBase = 0.0;
	double TickScale = 0.0;
	double XScale = 0.0;
	double YScale = 0.0;
	double Value = 0.0;
	double MaxData = 0.0;
	double MaxMaster = 0;
	double Compression = 0.0;
	double Step = 0;

	double LineData[256];

	char Str[MAX_STR_LEN];

	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	
	} else {

		// initialize border values
		m_Border_X1 = 0;
		m_Border_X2 = 0;
		m_Border_Y1 = 0;
		m_Border_Y2 = 0;
		m_Border_Range = 0;

		// Number of Vertices
		XVerts = (m_XCrystals * 2) + 1;
		YVerts = (m_YCrystals * 2) + 1;
		BlockVerts = XVerts * YVerts;

		// paint image
		CPaintDC dc(this);

		// Create Pens
		CPen BlackPen(PS_SOLID, 1, RGB(0, 0 , 0));
		CPen RedPen(PS_SOLID, 1, RGB(255, 0 , 0));
		CPen GreenPen(PS_SOLID, 1, RGB(0, 255, 0));
		CPen BluePen(PS_SOLID, 1, RGB(0, 0, 255));
		CPen WhitePen(PS_SOLID, 1, RGB(255, 255, 255));
		CPen CyanPen(PS_SOLID, 1, RGB(0, 255, 255));
		CPen YellowPen(PS_SOLID, 1, RGB(255, 255, 0));

		//copy the image memory dc into the screen dc
		::StretchBlt(dc.m_hDC,			//destination
						m_Head_Pos_X,
						m_Head_Pos_Y,
						m_Display_X,
						m_Display_Y,
						m_dcHead,		//source
						0,
						0,
						m_Display_X,
						m_Display_Y,
						SRCCOPY);

		//copy the image memory dc into the screen dc
		::StretchBlt(dc.m_hDC,			//destination
						m_Block_Pos_X,
						m_Block_Pos_Y,
						256,
						256,
						m_dcBlock,		//source
						0,
						0,
						256,
						256,
						SRCCOPY);

		// get the view information
		View = m_ViewIndex[m_ViewDrop.GetCurSel()];
		Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
		Config = m_ConfigDrop.GetCurSel();
		Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];
		m_Block.GetWindowText(Str, 10);
		Block = atol(Str);

		// special processing for each view
		switch (View) {

		case DHI_MODE_RUN:
		case DATA_CRYSTAL_PEAKS:
		case DATA_TIME_OFFSETS:
			
			// select the black pen
			dc.SelectObject(&BlackPen);

			// horizontal
			Step = 256.0 / m_YCrystals;
			x1 = m_Block_Pos_X;
			x2 = x1 + 256;
			for (i = 1 ; i < m_YCrystals ; i++) {
				y1 = m_Block_Pos_Y + (long) ((i * Step) + 0.5);
				dc.MoveTo(x1, y1);
				dc.LineTo(x2, y1);
			}

			// vertical
			Step = 256.0 / m_XCrystals;
			y1 = m_Block_Pos_Y;
			y2 = y1 + 256;
			for (i = 1 ; i < m_YCrystals ; i++) {
				x1 = m_Block_Pos_X + (long) ((i * Step) + 0.5);
				dc.MoveTo(x1, y1);
				dc.LineTo(x1, y2);
			}
			break;

		case DHI_MODE_POSITION_PROFILE:

			// if regions are to be drawn
			if ((m_RegionCheck.GetCheck() == 1) && (m_HeadData != NULL)) {

				// select the green pen
				dc.SelectObject(&GreenPen);

				// loop through the horizontal lines
				for (i = 0 ; i <= YVerts ; i += 2) {

					// starting point
					index = (Block * (2 * BlockVerts)) + (i * (2 * XVerts));
					x1 = m_Block_Pos_X + m_Verts[index+0];
					y1 = m_Block_Pos_Y + m_Verts[index+1];
					dc.MoveTo(x1, y1);

					// points along line
					for (j = 1 ; j < XVerts ; j++) {
						index = (Block * (2 * BlockVerts)) + (i * (2 * XVerts)) + (j * 2);
						x2 = m_Block_Pos_X + m_Verts[index+0];
						y2 = m_Block_Pos_Y + m_Verts[index+1];
						dc.LineTo(x2, y2);
					}
				}

				// loop through the vertical lines
				for (i = 0 ; i <= XVerts ; i += 2) {

					// starting point
					index = (Block * (2 * BlockVerts)) + (i * 2);
					x1 = m_Block_Pos_X + m_Verts[index+0];
					y1 = m_Block_Pos_Y + m_Verts[index+1];
					dc.MoveTo(x1, y1);

					// points along line
					for (j = 1 ; j < YVerts ; j++) {
						index = (Block * (2 * BlockVerts)) + (i * 2) + (j * (2 * XVerts));
						x2 = m_Block_Pos_X + m_Verts[index+0];
						y2 = m_Block_Pos_Y + m_Verts[index+1];
						dc.LineTo(x2, y2);
					}
				}
			}
			break;

		case DHI_MODE_CRYSTAL_ENERGY:

			// if energy peaks are to be designated
			if ((m_PeaksCheck.GetCheck() == 1) && (m_HeadData != NULL) && (SCANNER_RING != m_ScannerType)) {

				// select the red pen
				dc.SelectObject(&RedPen);
				
				// fast layer
				if ((Layer == LAYER_FAST) || (Layer == LAYER_ALL)) {

					// loop through the crystals
					for (i = 0 ; i < MAX_CRYSTALS ; i++) {

						// if the energy peak is non-zero
						index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2) + (LAYER_FAST * MAX_HEAD_CRYSTALS);
						if (m_Peaks[index+(Block*MAX_CRYSTALS)+i] |= 0) {

							// starting point
							x1 = m_Block_Pos_X + m_Peaks[index+(Block*MAX_CRYSTALS)+i] - 1;
							y1 = m_Block_Pos_Y + i;
							dc.MoveTo(x1, y1);

							// draw rest of symbol
							for (j = 0 ; j < 4 ; j++) {
								x2 = x1 + Line_X[j];
								y2 = y1 + Line_Y[j];
								dc.LineTo(x2, y2);
							}
						}
					}
				}
				
				// slow layer
				if ((Layer == LAYER_SLOW) || (Layer == LAYER_ALL)) {

					// loop through the crystals
					for (i = 0 ; i < MAX_CRYSTALS ; i++) {

						// if the energy peak is non-zero
						index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2) + (LAYER_SLOW * MAX_HEAD_CRYSTALS);
						if (m_Peaks[index+(Block*MAX_CRYSTALS)+i] |= 0) {

							// starting point
							x1 = m_Block_Pos_X + m_Peaks[index+(Block*MAX_CRYSTALS)+i] - 1;
							y1 = m_Block_Pos_Y + i;
							dc.MoveTo(x1, y1);

							// draw rest of symbol
							for (j = 0 ; j < 4 ; j++) {
								x2 = x1 + Line_X[j];
								y2 = y1 + Line_Y[j];
								dc.LineTo(x2, y2);
							}
						}
					}
				}
			}
			break;

		case DHI_MODE_TUBE_ENERGY:

			// if there is block data
			if (m_HeadData != NULL) {

				// set the white pen as the color
				dc.SelectObject(&WhitePen);

				// determine the maximum value for the block
				index = Block * 4 * 256;
				for (MaxValue = 0, Tube = 0 ; Tube < 4 ; Tube++) {
					for (i = 1 ; i < 255 ; i++) {
						if (m_HeadData[index+(Tube*256)+i] > MaxValue) {
							MaxValue = m_HeadData[index+(Tube*256)+i];
						}
					}
				}

				// determine the maximum end of the y range (if valid)
				if (MaxValue < 8) MaxValue = 8;
				Power = (long) log10((double) MaxValue);
				TickBase = pow(10.0, (double) Power);
				YRng = MaxValue + ((long) TickBase - (MaxValue % (long) TickBase));
				if (((double) YRng / TickBase) < 3.0) {
					Power--;
					TickBase = pow(10.0, (double) Power);
					YRng = MaxValue + ((long) TickBase - (MaxValue % (long) TickBase));
				}

				// tick marks
				for (i = 0 ; (i < 5) && (YRng / (m_TickScale[i] * TickBase) >= 3) ; i++);
				TickScale = m_TickScale[i-1];
				NumTicks = (long) ((double) YRng / (TickScale * TickBase)) + 1;

				// Put up borders
				m_Border_X1 = m_Block_Pos_X + 35;
				if ((NumTicks * TickScale * TickBase) >= 10000) m_Border_X1 += 20;
				m_Border_X2 = m_Block_Pos_X + 256 - 10;
				m_Border_Y1 = m_Block_Pos_Y + 10;
				m_Border_Y2 = m_Block_Pos_Y + 256 - 25;
				dc.MoveTo(m_Border_X1, m_Border_Y1);
				dc.LineTo(m_Border_X1, m_Border_Y2);
				dc.LineTo(m_Border_X2, m_Border_Y2);
				dc.LineTo(m_Border_X2, m_Border_Y1);
				dc.LineTo(m_Border_X1, m_Border_Y1);

				// Tick Marks - Vertical
				dc.SetTextColor(RGB(255, 255, 255));
				dc.SetBkColor(RGB(0, 0, 0));
				dc.SetTextAlign(TA_RIGHT);
				TickDist = (m_Border_Y2 - m_Border_Y1) / NumTicks;
				for (i = 0 ; i <= NumTicks ; i++) {

					// draw tick mark
					x1 = m_Border_X1 - 2;
					y1 = m_Border_Y2 - (i * TickDist);
					x2 = x1 + 2;
					y2 = y1;
					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);

					// display value
					Value = i * TickScale * TickBase;
					sprintf(Str, "%1.4g", Value);
					x2 = m_Border_X1 - 2;
					y2 -= 10;
					dc.TextOut(x2, y2, Str);
				}

				// Tick Marks - Horizontal
				dc.SetTextAlign(TA_CENTER);
				for (i = 50 ; i < 256 ; i += 50) {

					// draw tick mark
					x1 = m_Border_X1 + ((i * (m_Border_X2 - m_Border_X1)) / 256);
					y1 = m_Border_Y2 - 2;
					x2 = x1;
					y2 = y1 + 2;
					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);

					// display value
					y2 = m_Border_Y2 + 5;
					sprintf(Str, "%d", i);
					dc.TextOut(x2, y2, Str);
				}

				// Scales
				XScale = (double) (m_Border_X2 - m_Border_X1) / 256.0;
				YScale = (double) (m_Border_Y2 - m_Border_Y1) / (double) (NumTicks * TickScale * TickBase);

				// display tube data
				for (m_BlockCounts = 0, Tube = 0 ; Tube < 4 ; Tube++) {

					if ((m_CurrTube == 4) || (m_CurrTube == Tube)) {

						// tube color
						switch (Tube) {

						case 0:				// Tube A
							dc.SelectObject(&CyanPen);
							dc.SetTextColor(RGB(0, 255, 255));
							dc.TextOut(m_Border_X1+30, m_Border_Y1+30, "A");
							break;

						case 1:				// Tube B
							dc.SelectObject(&RedPen);
							dc.SetTextColor(RGB(255, 0, 0));
							dc.TextOut(m_Border_X1+30, m_Border_Y1+10, "B");
							break;

						case 2:				// Tube C
							dc.SelectObject(&GreenPen);
							dc.SetTextColor(RGB(0, 255, 0));
							dc.TextOut(m_Border_X1+10, m_Border_Y1+30, "C");
							break;

						case 3:				// Tube D
							dc.SelectObject(&YellowPen);
							dc.SetTextColor(RGB(255, 255, 0));
							dc.TextOut(m_Border_X1+10, m_Border_Y1+10, "D");
							break;
						}

						// Smooth the data
						Smooth(3, m_TE_Smooth, &m_HeadData[index+(Tube*256)], LineData);

						// first postition
						x1 = m_Border_X1;
						y1 = m_Border_Y2 - (long) ((LineData[0] * YScale) + 0.5);
						if (y1 < m_Border_Y1) y1 = m_Border_Y1;
						dc.MoveTo(x1, y1);
						m_BlockCounts += m_HeadData[index+(Tube*256)];

						// subsequent data
						for (MaxData = LineData[0], i = 1 ; i < 256 ; i++) {
							x2 = m_Border_X1 + (long) ((i * XScale) + 0.5);
							y2 = m_Border_Y2 - (long) ((LineData[i] * YScale) + 0.5);
							if (y2 < m_Border_Y1) y2 = m_Border_Y1;
							if (LineData[i] > MaxData) MaxData = LineData[i];
							dc.LineTo(x2, y2);
							m_BlockCounts += m_HeadData[index+(Tube*256)+i];
						}

						// if master selected
						if ((m_MasterCheck.GetCheck() == 1) && (m_MasterData != NULL)) {

							// draw master in white
							dc.SelectObject(&WhitePen);

							// Smooth the data
							Smooth(3, m_TE_Smooth, &m_MasterData[index+(Tube*256)], LineData);

							// find maximum value in master data
							for (MaxMaster = 0.0, i = 1 ; i < 255 ; i++) {
								if (LineData[i] > MaxMaster) MaxMaster = LineData[i];
							}
							YScale *= MaxData / MaxMaster;

							// first postition
							x1 = m_Border_X1;
							y1 = m_Border_Y2 - (long) ((LineData[0] * YScale) + 0.5);
							if (y1 < m_Border_Y1) y1 = m_Border_Y1;
							dc.MoveTo(x1, y1);

							// subsequent data
							for (i = 1 ; i < 256 ; i++) {
								x2 = m_Border_X1 + (long) ((i * XScale) + 0.5);
								y2 = m_Border_Y2 - (long) ((LineData[i] * YScale) + 0.5);
								if (y2 < m_Border_Y1) y2 = m_Border_Y1;
								dc.LineTo(x2, y2);
							}
						}
					}
				}
			}
			break;

		case DHI_MODE_SHAPE_DISCRIMINATION:

			// if there is block data
			if (m_HeadData != NULL) {

				// set the white pen as the color
				dc.SelectObject(&WhitePen);

				// determine the maximum value for the block
				index = Block * 256;
				for (m_BlockCounts = 0, MaxValue = 0, LastValue = 0, i = 1 ; i < 255 ; i++) {
					m_BlockCounts += m_HeadData[index+i];
					if (m_HeadData[index+i] > MaxValue) MaxValue = m_HeadData[index+i];
					if (m_HeadData[index+i] > 0) LastValue = i;
				}

				// determine the maximum end of the y range
				if (MaxValue < 8) MaxValue = 8;
				Power = (long) log10((double) MaxValue);
				TickBase = pow(10.0, (double) Power);
				YRng = MaxValue + ((long) TickBase - (MaxValue % (long) TickBase));
				if (((double) YRng / TickBase) < 3.0) {
					Power--;
					TickBase = pow(10.0, (double) Power);
				}

				// tick marks
				for (i = 0 ; (i < 5) && (YRng / (m_TickScale[i] * TickBase) >= 3) ; i++);
				TickScale = m_TickScale[i-1];
				NumTicks = (long) ((double) YRng / (TickScale * TickBase)) + 1;

				// Put up borders
				m_Border_X1 = m_Block_Pos_X + 35;
				if ((NumTicks * TickScale * TickBase) >= 10000) m_Border_X1 += 20;
				m_Border_X2 = m_Block_Pos_X + 256 - 10;
				m_Border_Y1 = m_Block_Pos_Y + 10;
				m_Border_Y2 = m_Block_Pos_Y + 256 - 25;
				dc.MoveTo(m_Border_X1, m_Border_Y1);
				dc.LineTo(m_Border_X1, m_Border_Y2);
				dc.LineTo(m_Border_X2, m_Border_Y2);
				dc.LineTo(m_Border_X2, m_Border_Y1);
				dc.LineTo(m_Border_X1, m_Border_Y1);

				// Tick Marks - Vertical
				dc.SetTextColor(RGB(255, 255, 255));
				dc.SetBkColor(RGB(0, 0, 0));
				dc.SetTextAlign(TA_RIGHT);
				TickDist = (m_Border_Y2 - m_Border_Y1) / NumTicks;
				for (i = 0 ; i <= NumTicks ; i++) {

					// draw tick mark
					x1 = m_Border_X1 - 2;
					y1 = m_Border_Y2 - (i * TickDist);
					x2 = x1 + 2;
					y2 = y1;
					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);

					// display value
					Value = i * TickScale * TickBase;
					sprintf(Str, "%1.4g", Value);
					x2 = m_Border_X1 - 2;
					y2 -= 10;
					dc.TextOut(x2, y2, Str);
				}

				// different scales for diffenet scanners
				if (m_ScannerType == SCANNER_HRRT) {
					Max_X = 100;
					m_Border_Range = 100;
					Tick_X = 25;
				} else {
					Max_X = 300;
					if (LastValue > ((100 * 256) / 300)) {
						m_Border_Range = 300;
						Tick_X = 100;
					} else {
						m_Border_Range = 100;
						Tick_X = 25;
					}
				}

				// Tick Marks - Horizontal
				dc.SetTextAlign(TA_CENTER);
				for (i = Tick_X ; i <= m_Border_Range ; i += Tick_X) {

					// draw tick mark
					x1 = m_Border_X1 + ((((i * 256) / m_Border_Range) * (m_Border_X2 - m_Border_X1)) / 256);
					y1 = m_Border_Y2 - 2;
					x2 = x1;
					y2 = y1 + 2;
					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);

					// display value
					y2 = m_Border_Y2 + 5;
					sprintf(Str, "%d", i);
					dc.TextOut(x2, y2, Str);
				}

				// Scales
				XScale = (double) (m_Border_X2 - m_Border_X1) / m_Border_Range;
				YScale = (double) (m_Border_Y2 - m_Border_Y1) / (double) (NumTicks * TickScale * TickBase);

				// Smooth the data
				Smooth(3, m_TE_Smooth, &m_HeadData[index], LineData);

				// first postition
				x1 = m_Border_X1;
				y1 = m_Border_Y2 - (long) ((LineData[0] * YScale) + 0.5);
				if (y1 < m_Border_Y1) y1 = m_Border_Y1;
				dc.MoveTo(x1, y1);

				// subsequent data
				Compression = (double) Max_X / 256.0;
				for (i = 1 ; (i * Compression) < m_Border_Range ; i++) {
					x2 = m_Border_X1 + (long) (((i * Compression) * XScale) + 0.5);
					y2 = m_Border_Y2 - (long) ((m_HeadData[(Block*256)+i] * YScale) + 0.5);
					if (y2 < m_Border_Y1) y2 = m_Border_Y1;
					dc.LineTo(x2, y2);
				}

				// if the shape discrimination line is to be shown
				if (m_ThresholdCheck.GetCheck() == 1) {

					// beginning of analog settings for block
					index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

					// Shape Threshold
					dc.SelectObject(&RedPen);
					ShapeValue = m_Settings[index+SetIndex[SHAPE]];
					x1 = m_Border_X1 + (long) ((ShapeValue * XScale) + 0.5);
					dc.MoveTo(x1, m_Border_Y1);
					dc.LineTo(x1, m_Border_Y2);

					// petspect or P39 show window boundaries
					if ((m_ScannerType == SCANNER_PETSPECT) || (m_ScannerType == SCANNER_P39)) {

						// lines in green
						dc.SelectObject(&GreenPen);

						// draw lines
						ShapeValue = m_Settings[index+SetIndex[SLOW_LOW]];
						x1 = m_Border_X1 + (long) (((ShapeValue * Compression) * XScale) + 0.5);
						if (x1 > m_Border_X2) x1 = m_Border_X2;
						dc.MoveTo(x1, m_Border_Y1);
						dc.LineTo(x1, m_Border_Y2);

						ShapeValue = m_Settings[index+SetIndex[SLOW_HIGH]];
						x1 = m_Border_X1 + (long) (((ShapeValue * Compression) * XScale) + 0.5);
						if (x1 > m_Border_X2) x1 = m_Border_X2;
						dc.MoveTo(x1, m_Border_Y1);
						dc.LineTo(x1, m_Border_Y2);

						ShapeValue = m_Settings[index+SetIndex[FAST_LOW]];
						x1 = m_Border_X1 + (long) (((ShapeValue * Compression) * XScale) + 0.5);
						if (x1 > m_Border_X2) x1 = m_Border_X2;
						dc.MoveTo(x1, m_Border_Y1);
						dc.LineTo(x1, m_Border_Y2);

						ShapeValue = m_Settings[index+SetIndex[FAST_HIGH]];
						x1 = m_Border_X1 + (long) (((ShapeValue * Compression) * XScale) + 0.5);
						if (x1 > m_Border_X2) x1 = m_Border_X2;
						dc.MoveTo(x1, m_Border_Y1);
						dc.LineTo(x1, m_Border_Y2);
					}
				}
			}
			break;

		default:
			break;
		}

		// display the total number of counts for the block (if meaningful data type)
		if ((DHI_MODE_TUBE_ENERGY == View) || (DHI_MODE_SHAPE_DISCRIMINATION == View)) {
			m_XPos.SetWindowText("All");
			m_YPos.SetWindowText("All");
			sprintf(Str, "%d", m_BlockCounts);
			m_Value.SetWindowText(Str);
		}

		// if the divider box is checked
		if (m_DividerCheck.GetCheck()) {

			// set the white pen as the color
			dc.SelectObject(&WhitePen);

			// lines based on view type
			switch (View) {

			// position profile and crystal energy data
			case DHI_MODE_POSITION_PROFILE:
			case DHI_MODE_CRYSTAL_ENERGY:
			case DHI_MODE_TIME:
			case DATA_MODE_TIMING:

				// vertical lines
				for (i = 1 ; i < m_XBlocks ; i++) {

					// coordinates
					x1 = m_Head_Pos_X + (i * (PP_SIZE_X / m_DisplayDownScale));
					x2 = x1;
					y1 = m_Head_Pos_Y;
					y2 = y1 + (m_YBlocks * (PP_SIZE_Y / m_DisplayDownScale)) - 1;

					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);
				}

				// horizontal lines
				for (i = 1 ; i < m_YBlocks ; i++) {

					// coordinates
					x1 = m_Head_Pos_X;
					x2 = x1 + (m_XBlocks * (PP_SIZE_X / m_DisplayDownScale)) - 1;
					y1 = m_Head_Pos_Y + (i * (PP_SIZE_Y / m_DisplayDownScale));
					y2 = y1;

					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);
				}

				// red pen for block
				dc.SelectObject(&RedPen);

				// identify edges
				if (SCANNER_RING == m_ScannerType) {
					x1 = m_Head_Pos_X + ((Block / m_YBlocks) * (PP_SIZE_X / m_DisplayDownScale));
					x2 = x1 + (PP_SIZE_X / m_DisplayDownScale);
					y1 = m_Head_Pos_Y + ((Block % m_YBlocks) * (PP_SIZE_Y / m_DisplayDownScale));
					y2 = y1 + (PP_SIZE_Y / m_DisplayDownScale);
				} else {
					x1 = m_Head_Pos_X + ((Block % m_XBlocks) * (PP_SIZE_X / m_DisplayDownScale));
					x2 = x1 + (PP_SIZE_X / m_DisplayDownScale);
					y1 = m_Head_Pos_Y + ((Block / m_XBlocks) * (PP_SIZE_Y / m_DisplayDownScale));
					y2 = y1 + (PP_SIZE_Y / m_DisplayDownScale);
				}

				// draw
				dc.MoveTo(x1, y1);
				dc.LineTo(x2, y1);
				dc.LineTo(x2, y2);
				dc.LineTo(x1, y2);
				dc.LineTo(x1, y1);
				break;

			// shape discrimination & tube energy, do not draw
			case DHI_MODE_SHAPE_DISCRIMINATION:
			case DHI_MODE_TUBE_ENERGY:
				break;

			case DHI_MODE_RUN:
			case DATA_CRYSTAL_PEAKS:
			case DATA_TIME_OFFSETS:
			case DATA_PMT_GAINS:

				// vertical lines
				for (i = 1 ; i < m_XBlocks ; i++) {

					// coordinates
					x1 = m_Head_Pos_X + (i * m_DisplayUpScale * m_XCrystals);
					x2 = x1;
					y1 = m_Head_Pos_Y;
					y2 = y1 + (m_YBlocks * m_DisplayUpScale * m_YCrystals) - 1;

					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);
				}

				// horizontal lines
				for (i = 1 ; i < m_YBlocks ; i++) {

					// coordinates
					x1 = m_Head_Pos_X;
					x2 = x1 + (m_XBlocks * m_DisplayUpScale * m_XCrystals) - 1;
					y1 = m_Head_Pos_Y + (i * m_DisplayUpScale * m_YCrystals);
					y2 = y1;

					dc.MoveTo(x1, y1);
					dc.LineTo(x2, y2);
				}

				// red pen for block
				dc.SelectObject(&RedPen);

				// identify edges
				if (SCANNER_RING == m_ScannerType) {
					x1 = m_Head_Pos_X + ((Block / m_YBlocks) * m_DisplayUpScale * m_XCrystals);
					x2 = x1 + (m_DisplayUpScale * m_XCrystals);
					y1 = m_Head_Pos_Y + ((Block % m_YBlocks) * m_DisplayUpScale * m_YCrystals);
					y2 = y1 + (m_DisplayUpScale * m_YCrystals);
				} else {
					x1 = m_Head_Pos_X + ((Block % m_XBlocks) * m_DisplayUpScale * m_XCrystals);
					x2 = x1 + (m_DisplayUpScale * m_XCrystals);
					y1 = m_Head_Pos_Y + ((Block / m_XBlocks) * m_DisplayUpScale * m_YCrystals);
					y2 = y1 + (m_DisplayUpScale * m_YCrystals);
				}

				// draw
				dc.MoveTo(x1, y1);
				dc.LineTo(x2, y1);
				dc.LineTo(x2, y2);
				dc.LineTo(x1, y2);
				dc.LineTo(x1, y1);
				break;

			default:
				break;
			}
		}

		// Destroy Pens
		DeleteObject(BlackPen);
		DeleteObject(RedPen);
		DeleteObject(GreenPen);
		DeleteObject(BluePen);
		DeleteObject(WhitePen);
		DeleteObject(CyanPen);
		DeleteObject(YellowPen);

		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDUGUIDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		UpdateHead
// Purpose:		Updates the head display and main data storage
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				05 Nov 03	T Gremillion	block numbering is different for
//											ring scanners
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::UpdateHead()
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long index_big = 0;
	long index_small = 0;
	long x = 0;
	long y = 0;
	long bx = 0;
	long by = 0;
	long cx = 0;
	long cy = 0;
	long Time = 0;
	long Size = 0;
	long Small_X = 0;
	long Small_Y = 0;
	long Start_X = 0;
	long Start_Y = 0;
	long SmallSize = 0;
	long Status = 0;
	long MaxValue = 0;
	long color = 0;
	long Ratio = 0;

	long Block = 0;
	long Xtal = 0;
	long Bin = 0;
	long Tube = 0;
	long View = 0;
	long Head = 0;
	long Config = 0;
	long Layer = 0;
	long DataLayer = 0;

	long Large[PP_SIZE];
	long Small[PP_SIZE];
	long Img[PP_SIZE];

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// calculate the size of the small array
	Small_X = PP_SIZE_X / m_DisplayDownScale;
	Small_Y = PP_SIZE_Y / m_DisplayDownScale;
	SmallSize = Small_X * Small_Y;

	// Disable all controls
	Disable_Controls();

	// no warning has been given yet, that position information is not available
	m_PositionWarningShown = 0;

	// retrieve fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];

	// layer containing data
	DataLayer = Layer;
	if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_FAST;

	// fill in black initially
	for (y = 0 ; y < m_Display_Y ; y++) for (x = 0 ; x < m_Display_X ; x++) {
		BitBlt(m_dcHead, x, y, 1, 1, hdcColor[0], 0, 0, SRCCOPY);
	}

	// data retrieval is base on view type
	switch (View) {

	// position profile and crystal energy displayed the same
	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_CRYSTAL_ENERGY:
	case DHI_MODE_TIME:

		// release the current memory chunk if pointer not null
		if (m_HeadData != NULL) CoTaskMemFree(m_HeadData);

		// retrieve current head data
		hr = pDU->Current_Head_Data(View, Config, Head, Layer, &Time, &Size, &m_HeadData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "UpdateHead", "Method Failed (pDU->Current_Head_Data[PP|EN]), HRESULT", hr);

		// successful
		if (Status == 0) {

			// make sure there is sufficient data
			if (Size == (m_NumBlocks * 256 * 256)) {

				// fill in the date
				sprintf(Str, "Acquired On: %s", ctime(&Time));
				Str[37] = '\0';
				m_AcquireDate.SetWindowText(Str);

				// loop through the data
				for (Block = 0 ; Block < m_NumBlocks ; Block++) {

					// Clear the small data array
					for (i = 0 ; i < SmallSize ; i++) Small[i] = 0;

					// shrink data
					for (y = 0 ; y < 256 ; y++) for (x = 0 ; x < 256 ; x++) {

						// data index
						index_big = (Block * 256 * 256) + (y * 256) + x;
						index_small = ((y / m_DisplayDownScale) * Small_Y) + (x / m_DisplayDownScale);

						// add to small data array
						Small[index_small] += m_HeadData[index_big];
					}

					// average data
					for (i = 0 ; i < SmallSize ; i++) Small[i] /= (m_DisplayDownScale * m_DisplayDownScale);

					// histogram equalize the data
					hist_equal(m_DataScaling, SmallSize, Small, Img);

					// determine where the data is to be placed
					if (SCANNER_RING == m_ScannerType) {
						Start_X = (Block / m_YBlocks) * Small_X;
						Start_Y = (Block % m_YBlocks) * Small_Y;
					} else {
						Start_X = (Block % m_XBlocks) * Small_X;
						Start_Y = (Block / m_XBlocks) * Small_Y;
					}

					// apply to bitmap
					for (y = 0 ; y < Small_Y ; y++) for (x = 0 ; x < Small_X ; x++) {
						index_small = (y * Small_X) + x;
						BitBlt(m_dcHead, Start_X+x, Start_Y+y, 1, 1, hdcColor[Img[index_small]], 0, 0, SRCCOPY);
					}
				}

			} else {
				MessageBox("Insufficient Data", "Error", MB_OK);
			}

		// failed to retrieve data, post message
		} else {
			m_AcquireDate.SetWindowText("Could Not Retrieve Data");
		}

		break;

	// tube energy
	case DHI_MODE_TUBE_ENERGY:

		// release the current memory chunk if pointer not null
		if (m_HeadData != NULL) CoTaskMemFree(m_HeadData);

		// retrieve current head data
		hr = pDU->Current_Head_Data(View, Config, Head, Layer, &Time, &Size, &m_HeadData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "UpdateHead", "Method Failed (pDU->Current_Head_Data[TE]), HRESULT", hr);

		// successful
		if (Status == 0) {

			// make sure there is sufficient data
			if (Size == (m_NumBlocks * 256 * 4)) {

				// fill in the date
				sprintf(Str, "Acquired On: %s", ctime(&Time));
				Str[37] = '\0';
				m_AcquireDate.SetWindowText(Str);

				// loop through the blocks
				for (Block = 0 ; Block < m_NumBlocks ; Block++) {

					// loop through tubes
					for (Tube = 0 ; Tube < 4 ; Tube++) {

						// calculate y position
						y = (Block * 4) + Tube;

						// find maximum value for Tube
						for (MaxValue = 0, x = 0 ; x < 256 ; x++) {
							index = (Block * 4 * 256) + (Tube * 256) + x;
							if (m_HeadData[index] > MaxValue) MaxValue = m_HeadData[index];
						}

						// apply to bitmap
						for (x = 0 ; x < 512 ; x++) {
							index = (Block * 4 * 256) + (Tube * 256) + (x / 2);
							color = (long) ((256.0 * (double) m_HeadData[index]) / (double) (MaxValue + 1));
							BitBlt(m_dcHead, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
						}
					}
				}

				// release the current memory chunk if pointer not null
				if (m_MasterData != NULL) CoTaskMemFree(m_MasterData);

				// retrieve master data
				hr = pDU->Read_Master(View, Config, Head, Layer, &Size, &m_MasterData, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "UpdateHead", "Method Failed (pDU->Read_Master[TE]), HRESULT", hr);

			} else {
				MessageBox("Insufficient Data", "Error", MB_OK);
			}

		// failed to retrieve data, post message
		} else {
			m_AcquireDate.SetWindowText("Could Not Retrieve Data");
		}

		break;

	// shape discrimination
	case DHI_MODE_SHAPE_DISCRIMINATION:

		// release the current memory chunk if pointer not null
		if (m_HeadData != NULL) CoTaskMemFree(m_HeadData);

		// retrieve current head data
		hr = pDU->Current_Head_Data(View, Config, Head, Layer, &Time, &Size, &m_HeadData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "UpdateHead", "Method Failed (pDU->Current_Head_Data[SD]), HRESULT", hr);

		// successful
		if (Status == 0) {

			// make sure there is sufficient data
			if (Size == (m_NumBlocks * 256)) {

				// fill in the date
				sprintf(Str, "Acquired On: %s", ctime(&Time));
				Str[37] = '\0';
				m_AcquireDate.SetWindowText(Str);

				// loop through the blocks
				for (Block = 0 ; Block < m_NumBlocks ; Block++) {

					// find maximum value for Tube
					for (MaxValue = 0, x = 0 ; x < 256 ; x++) {
						index = (Block * 256) + x;
						if (m_HeadData[index] > MaxValue) MaxValue = m_HeadData[index];
					}

					// apply to bitmap
					y = Block * 4;
					for (x = 0 ; x < 512 ; x++) {
						index = (Block * 256) + (x / 2);
						color = (long) ((256.0 * (double) m_HeadData[index]) / (double) (MaxValue + 1));
						BitBlt(m_dcHead, x, y+0, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
						BitBlt(m_dcHead, x, y+1, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
						BitBlt(m_dcHead, x, y+2, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
						BitBlt(m_dcHead, x, y+3, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
					}
				}

			} else {
				MessageBox("Insufficient Data", "Error", MB_OK);
			}

		// failed to retrieve data, post message
		} else {
			m_AcquireDate.SetWindowText("Could Not Retrieve Data");
		}

		break;

	// run mode
	case DHI_MODE_RUN:

		// release the current memory chunk if pointer not null
		if (m_HeadData != NULL) CoTaskMemFree(m_HeadData);

		// retrieve current head data
		hr = pDU->Current_Head_Data(View, Config, Head, Layer, &Time, &Size, &m_HeadData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "UpdateHead", "Method Failed (pDU->Current_Head_Data[RUN]), HRESULT", hr);

		// successful
		if (Status == 0) {

			// make sure there is sufficient data
			if (Size == (128 * 128)) {

				// fill in the date
				sprintf(Str, "Acquired On: %s", ctime(&Time));
				Str[37] = '\0';
				m_AcquireDate.SetWindowText(Str);

				// find max value
				for (y = 0 ; y < 128 ; y++) for (x = 0 ; x < 128 ; x++) {
					index = (y * 128) + x;
					if (m_HeadData[index] > MaxValue) MaxValue = m_HeadData[index];
				}

				// apply to bitmap
				for (y = 0 ; y < 512 ; y++) for (x = 0 ; x < 512 ; x++) {
					index = ((y / m_DisplayUpScale) * 128) + (x / m_DisplayUpScale);
					color = (long) ((256.0 * (double) m_HeadData[index]) / (double) (MaxValue + 1));
					BitBlt(m_dcHead, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
				}

			} else {
				MessageBox("Insufficient Data", "Error", MB_OK);
			}

		// failed to retrieve data, post message
		} else {
			m_AcquireDate.SetWindowText("Could Not Retrieve Data");
		}

		break;

	// timing data
	case DATA_MODE_TIMING:

		// release the current memory chunk if pointer not null
		if (m_HeadData != NULL) CoTaskMemFree(m_HeadData);

		// retrieve current head data
		hr = pDU->Current_Head_Data(View, Config, Head, Layer, &Time, &Size, &m_HeadData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "UpdateHead", "Method Failed (pDU->Current_Head_Data[TIMING]), HRESULT", hr);

		// successful
		if (Status == 0) {

			// make sure there is sufficient data
			if (Size == (128 * 128 * m_TimingBins)) {

				// fill in the date
				sprintf(Str, "Acquired On: %s", ctime(&Time));
				Str[37] = '\0';
				m_AcquireDate.SetWindowText(Str);

				// loop through the data
				for (Block = 0 ; Block < m_NumBlocks ; Block++) {

					// block coordinates
					bx = Block % m_XBlocks;
					by = Block / m_XBlocks;

					// Clear the large data array
					for (i = 0 ; i < PP_SIZE ; i++) Large[i] = 0;

					// each crystal gets its own line in the large array
					Ratio = PP_SIZE_X / m_TimingBins;
					for (j = 0 ; j < PP_SIZE_Y ; j++) {

						// crystal coordinates
						cx = j % 16;
						cy = j / 16;

						// identify crystal starting position
						index = ((((by * m_YCrystals) + cy) * 128) + ((bx * m_XCrystals) + cx)) * m_TimingBins;

						// transfer the data (if a valid crystal)
						if ((cx < m_XCrystals) && (cy < m_YCrystals)) {
							for (i = 0 ; i < PP_SIZE_X ; i++) Large[(j*PP_SIZE_X)+i] = m_HeadData[index+(i/Ratio)];
						}
					}

					// Clear the small data array
					for (i = 0 ; i < SmallSize ; i++) Small[i] = 0;

					// fit the data
					for (j = 0 ; j < PP_SIZE_Y; j++) for (i = 0 ; i < PP_SIZE_X ; i++) {

						// data index
						index_big = (j * 256) + i;
						index_small = ((j / m_DisplayDownScale) * Small_X) + (i / m_DisplayDownScale);

						// add to small data array
						Small[index_small] += Large[index_big];
					}

					// find maximum value
					for (MaxValue = 0, i = 0 ; i < SmallSize ; i++) {
						if (Small[i] > MaxValue) MaxValue = Small[i];
					}

					// apply to bitmap
					for (y = 0 ; y < (PP_SIZE_Y / m_DisplayDownScale) ; y++) for (x = 0 ; x < (PP_SIZE_X / m_DisplayDownScale) ; x++) {
						index_small = (y * (PP_SIZE_X / m_DisplayDownScale)) + x;
						color = (long) ((256.0 * (double) Small[index_small]) / (double) (MaxValue + 1));
						BitBlt(m_dcHead, ((Block % m_XBlocks)*(PP_SIZE_X / m_DisplayDownScale))+x, ((Block/m_XBlocks)*(PP_SIZE_Y / m_DisplayDownScale))+y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
					}
				}

			} else {
				MessageBox("Insufficient Data", "Error", MB_OK);
			}

		// failed to retrieve data, post message
		} else {
			m_AcquireDate.SetWindowText("Could Not Retrieve Data");
		}

		break;

	// Time Offsets
	case DATA_TIME_OFFSETS:

		// fill in date field
		m_AcquireDate.SetWindowText("Current Data");

		// only valid for separate layers
		if (DataLayer != LAYER_ALL) {

			// if the data was read correctly
			if (m_TimeValid[(Config * MAX_HEADS * 2) + (Head * 2) + DataLayer] == 1) {

				// loop through row
				for (y = 0 ; y < 512 ; y++) {

					// if outside of range, skip it
					if ((y / 4) >= (m_YBlocks*m_YCrystals)) continue;

					for (x = 0 ; x < 512 ; x++) {

						// if outside of range, skip it
						if ((x / 4) >= (m_XBlocks*m_XCrystals)) continue;

						// determine block
						if (SCANNER_RING == m_ScannerType) {
							Block = ((y / 4) / m_YCrystals) + (((x / 4) / m_XCrystals) * m_XBlocks);
						} else {
							Block = (((y / 4) / m_YCrystals) * m_XBlocks) + ((x / 4) / m_XCrystals);
						}

						// determine Crystal
						i = (x / 4) - ((Block % m_XBlocks) * m_XCrystals);
						j = (y / 4) - ((Block / m_XBlocks) * m_YCrystals);
						index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + (j * 16) + i;

						// retrieve value (assuming these values are +/- 8)
						color = 128 + (m_Time[index] * 4);
						BitBlt(m_dcHead, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
					}
				}
			}
		}

		break;

	// PMT Gains
	case DATA_PMT_GAINS:

		// fill in date field
		m_AcquireDate.SetWindowText("Current Data");

		// loop through row
		for (y = 0 ; y < 512 ; y++) {

			// if outside of range, skip it
			if ((y / 4) >= (m_YBlocks*m_YCrystals)) continue;

			for (x = 0 ; x < 512 ; x++) {

				// if outside of range, skip it
				if ((x / 4) >= (m_XBlocks*m_XCrystals)) continue;

				// determine block
				if (SCANNER_RING == m_ScannerType) {
					Block = ((y / 4) / m_YCrystals) + (((x / 4) / m_XCrystals) * m_XBlocks);
				} else {
					Block = (((y / 4) / m_YCrystals) * m_XBlocks) + ((x / 4) / m_XCrystals);
				}

				// determine tube
				index = (2 * (((y / 4) % m_YCrystals) / (m_YCrystals / 2))) + (((x / 4) % m_XCrystals) / (m_XCrystals / 2));
				Tube = SetIndex[m_TubeOrder[index]];

				// retrieve value
				index = ((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS;
				color = m_Settings[index+(Block*m_NumSettings)+Tube];
				BitBlt(m_dcHead, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
			}
		}

		break;

	// Crystal Energy Peaks
	case DATA_CRYSTAL_PEAKS:

		// fill in date field
		m_AcquireDate.SetWindowText("Current Data");

		// only valid for separate layers
		if (DataLayer != LAYER_ALL) {

			// if the data was read correctly
			if (m_PeaksValid[(Config * MAX_HEADS * 2) + (Head * 2) + DataLayer] == 1) {

				// loop through row
				for (y = 0 ; y < 512 ; y++) {

					// if outside of range, skip it
					if ((y / 4) >= (m_YBlocks*m_YCrystals)) continue;

					for (x = 0 ; x < 512 ; x++) {

						// if outside of range, skip it
						if ((x / 4) >= (m_XBlocks*m_XCrystals)) continue;

						// determine block
						if (SCANNER_RING == m_ScannerType) {
							Block = ((y / 4) / m_YCrystals) + (((x / 4) / m_XCrystals) * m_XBlocks);
						} else {
							Block = (((y / 4) / m_YCrystals) * m_XBlocks) + ((x / 4) / m_XCrystals);
						}

						// determine Crystal
						i = (x / 4) - ((Block % m_XBlocks) * m_XCrystals);
						j = (y / 4) - ((Block / m_XBlocks) * m_YCrystals);
						index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + (j * 16) + i;

						// retrieve value
						color = m_Peaks[index];
						BitBlt(m_dcHead, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
					}
				}
			}
		}

		break;
	}

	// Whenever the head is updated, the block must be updated as well
	UpdateBlock();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		UpdateBlock
// Purpose:		Updates the block display and settings
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				06 Nov 03	T Gremillion	Modified for different block numbering of ring scanners
//				12 Jan 04	T Gremillion	Added display of total counts for block
//											for run, pp, en, ti, time
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::UpdateBlock()
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x = 0;
	long y = 0;
	long bx = 0;
	long by = 0;
	long cx = 0;
	long cy = 0;
	long Time = 0;
	long Size = 0;
	long Status = 0;
	long MaxValue = 0;
	long Ratio = 0;
	long color = 0;

	long Block = 0;
	long View = 0;
	long Head = 0;
	long Config = 0;
	long Layer = 0;
	long DataLayer = 0;
	long PositionLayer = 0;
	long Tube = 0;
	long ApplyDNL = 0;

	long Data[256 * 256];

	double SmoothData[256 * 256];

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// initialize number of counts in block
	m_BlockCounts = 0;

	// Diable all controls
	Disable_Controls();

	// fill in black
	for (y = 0 ; y < 256 ; y++) for (x = 0 ; x < 256 ; x++) {
		BitBlt(m_dcBlock, x, y, 1, 1, hdcColor[0], 0, 0, SRCCOPY);
	}

	// retrieve fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// layer containing data
	DataLayer = Layer;
	if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_FAST;

	// update the fields
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);
	sprintf(Str, "%d", m_Settings[index+SetIndex[PMTA]]);
	m_PMTA.SetWindowText(Str);
	sprintf(Str, "%d", m_Settings[index+SetIndex[PMTB]]);
	m_PMTB.SetWindowText(Str);
	sprintf(Str, "%d", m_Settings[index+SetIndex[PMTC]]);
	m_PMTC.SetWindowText(Str);
	sprintf(Str, "%d", m_Settings[index+SetIndex[PMTD]]);
	m_PMTD.SetWindowText(Str);
	sprintf(Str, "%d", m_Settings[index+SetIndex[CFD]]);
	m_CFD.SetWindowText(Str);

	// clear value fields
	m_XPos.SetWindowText("");
	m_YPos.SetWindowText("");
	m_Value.SetWindowText("");

	// additional fields based on system type
	switch (m_ScannerType) {

	case SCANNER_P39:
	case SCANNER_PETSPECT:
		sprintf(Str, "%d", m_Settings[index+SetIndex[X_OFFSET]]);
		m_XOffset.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[Y_OFFSET]]);
		m_YOffset.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[SHAPE]]);
		m_Shape.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[E_OFFSET]]);
		m_VariableField.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[SLOW_LOW]]);
		m_SlowLow.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[SLOW_HIGH]]);
		m_SlowHigh.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[FAST_LOW]]);
		m_FastLow.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[FAST_HIGH]]);
		m_FastHigh.SetWindowText(Str);
		break;

	case SCANNER_P39_IIA:
		m_XOffLabel.SetWindowText("TDC Gain");
		sprintf(Str, "%d", m_Settings[index+SetIndex[TDC_GAIN]]);
		m_XOffset.SetWindowText(Str);
		m_YOffLabel.SetWindowText("TDC Offset");
		sprintf(Str, "%d", m_Settings[index+SetIndex[TDC_OFFSET]]);
		m_YOffset.SetWindowText(Str);
		m_VariableLabel.SetWindowText("TDC Setup");
		sprintf(Str, "%d", m_Settings[index+SetIndex[TDC_SETUP]]);
		m_VariableField.SetWindowText(Str);
		m_ShapeLabel.SetWindowText("CFD Setup");
		sprintf(Str, "%d", m_Settings[index+SetIndex[CFD_SETUP]]);
		m_Shape.SetWindowText(Str);
		break;


	case SCANNER_HRRT:
		sprintf(Str, "%d", m_Settings[index+SetIndex[X_OFFSET]]);
		m_XOffset.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[Y_OFFSET]]);
		m_YOffset.SetWindowText(Str);
		m_VariableLabel.SetWindowText("CFD Delay");
		sprintf(Str, "%d", m_Settings[index+SetIndex[CFD_DELAY]]);
		m_VariableField.SetWindowText(Str);
		sprintf(Str, "%d", m_Settings[index+SetIndex[SHAPE]]);
		m_Shape.SetWindowText(Str);
		break;

	default:
		break;
	}

	// data retrieval is base on view type
	switch (View) {

	// position profile and crystal energy displayed the same
	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_CRYSTAL_ENERGY:
	case DHI_MODE_TIME:

		// if the data buffer is loaded
		if (m_HeadData != NULL) {

			// smooth data (position profile only)
			if (DHI_MODE_POSITION_PROFILE == View) {
				if (((SCANNER_HRRT == m_ScannerType) || (SCANNER_PETSPECT == m_ScannerType) || (SCANNER_P39 == m_ScannerType)) && (m_DNLCheck.GetCheck() == 1)) ApplyDNL = 1;
				Smooth_2d(ApplyDNL, m_PP_Smooth, 1, &m_HeadData[Block * 256 * 256], SmoothData);
				for (i = 0 ; i < PP_SIZE ; i++) {
					Data[i] = (long) (SmoothData[i] + 0.5);
					m_BlockCounts += (__int64) m_HeadData[(Block * 256 * 256)+i];
				}
			} else {
				for (i = 0 ; i < PP_SIZE ; i++) {
					Data[i] = m_HeadData[(Block * 256 * 256)+i];
					m_BlockCounts += (__int64) Data[i];
				}
			}

			// histogram equalize the data
			hist_equal(m_DataScaling, PP_SIZE, Data, m_BlockImg);

			// apply to bitmap
			for (y = 0 ; y < 256 ; y++) for (x = 0 ; x < 256 ; x++) {
				index = (y * 256) + x;
				BitBlt(m_dcBlock, x, y, 1, 1, hdcColor[m_BlockImg[index]], 0, 0, SRCCOPY);
			}

			// if position profile or run mode, then peak locations may be needed (not for ring scanners)
			if ((View == DHI_MODE_POSITION_PROFILE) && (m_ScannerType != SCANNER_RING)) {

				// correct layer for peak positions
				switch (m_ScannerType) {

				case SCANNER_HRRT:
					PositionLayer = LAYER_FAST;
					break;

				case SCANNER_P39:
					PositionLayer = LAYER_FAST;
					break;

				default:
					PositionLayer = Layer;
					break;
				}

				// get the crystal positions
				hr = pDU->Get_Stored_Crystal_Position(Config, Head, PositionLayer, m_Verts, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "UpdateBlock", "Method Failed (pDU->Get_Stored_Crystal_Position), HRESULT", hr);

				// if no positions
				if (Status != 0) {

					// put up message
					if (0 == m_PositionWarningShown) MessageBox("No Crystal Positions", "Error", MB_OK);
					m_PositionWarningShown = 1;
				}
			}
		}

		break;

	// timing data - display the block
	case DATA_MODE_TIMING:

		// if the data buffer is loaded
		if (m_HeadData != NULL) {

			// block coordinates
			bx = Block % m_XBlocks;
			by = Block / m_XBlocks;

			// Clear the large data array
			for (i = 0 ; i < PP_SIZE ; i++) Data[i] = 0;

			// each crystal gets its own line in the large array
			Ratio = PP_SIZE_X / m_TimingBins;
			for (j = 0 ; j < PP_SIZE_Y ; j++) {

				// crystal coordinates
				cx = j % 16;
				cy = j / 16;

				// identify crystal starting position
				index = ((((by * m_YCrystals) + cy) * 128) + ((bx * m_XCrystals) + cx)) * m_TimingBins;

				// transfer the data (if a valid crystal)
				if ((cx < m_XCrystals) && (cy < m_YCrystals)) {
					for (i = 0 ; i < PP_SIZE_X ; i++) Data[(j*PP_SIZE_X)+i] = m_HeadData[index+(i/Ratio)];
				}
			}

			// find the maximum value
			for (MaxValue = 0, i = 0 ; i < PP_SIZE ; i++) {
				if (Data[i] > MaxValue) MaxValue = Data[i];
				m_BlockCounts += (__int64) Data[i];
			}

			// apply to bitmap
			for (y = 0 ; y < 256 ; y++) for (x = 0 ; x < 256 ; x++) {
				index = (y * 256) + x;
				color = (long) ((256.0 * (double) Data[index]) / (double) (MaxValue+1));
				BitBlt(m_dcBlock, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
			}
		}
		break;

	// tube energy - no action
	case DHI_MODE_TUBE_ENERGY:
		break;

	// shape discrimination - no action
	case DHI_MODE_SHAPE_DISCRIMINATION:
		break;

	// run mode
	case DHI_MODE_RUN:

		// if the data buffer is loaded
		if (m_HeadData != NULL) {

			// determine number of counts
			for (y = 0 ; y < m_YCrystals ; y++) for (x = 0 ; x < m_XCrystals ; x++) {

				// ring scanners are ordered different from panel detectors
				if (SCANNER_RING == m_ScannerType) {
					index = ((Block % m_YBlocks) * m_YCrystals * 128) + (y * 128) + ((Block / m_YBlocks) * m_XCrystals) + x;
				} else {
					index = ((Block / m_XBlocks) * m_YCrystals * 128) + (y * 128) + ((Block % m_XBlocks) * m_XCrystals) + x;
				}

				// add crystal counts
				m_BlockCounts += (__int64) m_HeadData[index];
			}

			// find max value
			for (y = 0 ; y < 128 ; y++) for (x = 0 ; x < 128 ; x++) {
				index = (y * 128) + x;
				if (m_HeadData[index] > MaxValue) MaxValue = m_HeadData[index];
			}

			// apply to bitmap
			for (j = 0 ; j < 256 ; j++) for (i = 0 ; i < 256 ; i++) {
				x = (i * m_XCrystals) / 256;
				y = (j * m_YCrystals) / 256;
				if (SCANNER_RING == m_ScannerType) {
					index = ((Block % m_YBlocks) * m_YCrystals * 128) + (y * 128) + ((Block / m_YBlocks) * m_XCrystals) + x;
				} else {
					index = ((Block / m_XBlocks) * m_YCrystals * 128) + (y * 128) + ((Block % m_XBlocks) * m_XCrystals) + x;
				}
				color = (long) ((256.0 * (double) m_HeadData[index]) / (double) (MaxValue + 1));
				BitBlt(m_dcBlock, i, j, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
			}
		}

		break;

	// PMT Gains
	case DATA_PMT_GAINS:

		// loop through row
		for (y = 0 ; y < 256 ; y++) {

			for (x = 0 ; x < 256 ; x++) {

				// determine tube
				index = (2 * (y / 128)) + (x / 128);
				Tube = SetIndex[m_TubeOrder[index]];

				// retrieve value
				index = ((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS;
				color = m_Settings[index+(Block*m_NumSettings)+Tube];
				BitBlt(m_dcBlock, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
			}
		}

		break;

	// Crystal Energy Peaks
	case DATA_CRYSTAL_PEAKS:

		// loop through row
		for (y = 0 ; y < 256 ; y++) {

			for (x = 0 ; x < 256 ; x++) {

				// determine crystal
				i = (x * m_XCrystals) / 256;
				j = (y * m_YCrystals) / 256;
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + (j * 16) + i;

				// retrieve value
				color = m_Peaks[index];
				BitBlt(m_dcBlock, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
			}
		}

		break;

	// Crystal Time offset
	case DATA_TIME_OFFSETS:

		// loop through row
		for (y = 0 ; y < 256 ; y++) {

			for (x = 0 ; x < 256 ; x++) {

				// determine crystal
				i = (x * m_XCrystals) / 256;
				j = (y * m_YCrystals) / 256;
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + (j * 16) + i;

				// retrieve value (assuming these values are +/- 8)
				color = 128 + (m_Time[index] * 4);
				BitBlt(m_dcBlock, x, y, 1, 1, hdcColor[color], 0, 0, SRCCOPY);
			}
		}

		break;
	}

	// display the total number of counts for the block (if meaningful data type)
	if ((DHI_MODE_CRYSTAL_ENERGY == View) || 
		(DHI_MODE_RUN == View) ||
		(DHI_MODE_TIME == View) ||
		(DATA_MODE_TIMING == View) ||
		(DHI_MODE_POSITION_PROFILE == View)) {
		m_XPos.SetWindowText("All");
		m_YPos.SetWindowText("All");
		sprintf(Str, "%d", m_BlockCounts);
		m_Value.SetWindowText(Str);
	}

	// force redraw
	Invalidate();

	// Re-enable controls
	Enable_Controls();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDestroy
// Purpose:		Steps to be performed when the GUI is closed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				21 Nov 03	T Gremillion	if an acquisition is in progress, abort it
//				19 Oct 04	T Gremillion	on HRRT, query user if they wish the scanner
//											to be put in acquisition mode.
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnDestroy() 
{
	// local variables
	long i = 0;
	long index = 0;
	long Percent = 0;
	long BlockState = 0;
	long TotalState = 0;
	long Status = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long DataLayer = 0;
	long Send = SEND;

	BOOL InitScan = FALSE;

	InfoDialog *pInfoDlg = NULL;

	char Str[MAX_STR_LEN];

	unsigned char CommandString[MAX_STR_LEN];
	unsigned char ResponseString[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// for HRRT, ask user if they want the scanner in scan mode
	if (SCANNER_HRRT == m_ScannerType) {
		if (MessageBox("Put Scanner in Acquisition Mode?", "Question", MB_YESNO) == IDYES) InitScan = TRUE;
	}

	// if an acquisitin is in progress, issue an abort
	if (m_AcquisitionFlag != 0) OnAbort();

	// if in simulation mode, don't try to send data to the heads
	if (m_Simulation) Send = DONT_SEND;

	// composite block state
	for (i = 0 ; i < (MAX_CONFIGS*MAX_TOTAL_BLOCKS) ; i++) TotalState |= m_BlockState[i];

	// if any crms were modified but not downloaded, warn user
	if ((TotalState & STATE_CRM_CHANGED) != 0) MessageBox("CRMs Changed, But Not Downloaded", "Warning", MB_OK);

	// if any crystal energy peaks were changed but not saved
	if (((TotalState & STATE_FAST_ENERGY_CHANGED) != 0) || ((TotalState & STATE_SLOW_ENERGY_CHANGED) != 0)) {

		// ask user if the changes are to be saved
		if (MessageBox("Crystal Energy Peaks Changed, But Not Saved.  Save Now?", "Question", MB_YESNO) == IDYES) {

			// loop through the heads and configurations
			for (Config = 0 ; Config < m_NumConfigurations ; Config++) for (Head = 0 ; Head < MAX_HEADS ; Head++) {

				// if the head is present
				if (m_HeadComm[Head] != 0) {

					// find the state of the head
					index = ((Config * MAX_HEADS) + Head) * MAX_BLOCKS;
					for (Block = 0, BlockState = 0 ; Block < m_NumBlocks ; Block++) BlockState |= m_BlockState[index+Block];

					// if the fast layer is used
					if ((Config == 0) || (m_ScannerType != SCANNER_PETSPECT)) {

						// if Fast Crystal Peaks have changed
						if ((BlockState & STATE_FAST_ENERGY_CHANGED) != 0) {

							// determine where it is to be stored
							DataLayer = LAYER_FAST;
							if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

							// store and download
							index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2);
							hr = pDU->Store_Energy_Peaks(Send, Config, Head, DataLayer, &m_Peaks[index], &Status);

							// if peaks could not be downloaded
							if (Status != 0) {
								sprintf(Str, "Could Not Download Fast Crystal Peaks Configuration: %d Head: %d", Config, Head);
								MessageBox(Str, "Error", MB_OK);
							}
						}
					}

					// if the slow layer is used
					if (m_NumLayers[Head] == 2) {

						// if Slow Crystal Peaks have changed
						if ((BlockState & STATE_SLOW_ENERGY_CHANGED) != 0) {

							// store and download
							index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2) + MAX_HEAD_CRYSTALS;
							hr = pDU->Store_Energy_Peaks(Send, Config, Head, LAYER_SLOW, &m_Peaks[index], &Status);

							// if peaks could not be downloaded
							if (Status != 0) {
								sprintf(Str, "Could Not Download Slow Crystal Peaks Configuration: %d Head: %d", Config, Head);
								MessageBox(Str, "Error", MB_OK);
							}
						}
					}
				}
			}
		}
	}

	// if any time offsets were changed but not saved
	if (((TotalState & STATE_FAST_TIME_CHANGED) != 0) || ((TotalState & STATE_SLOW_TIME_CHANGED) != 0)) {

		// ask user if the changes are to be saved
		if (MessageBox("Crystal Time Offsets Changed, But Not Saved.  Save Now?", "Question", MB_YESNO) == IDYES) {

			// loop through the heads and configurations
			for (Config = 0 ; Config < m_NumConfigurations ; Config++) for (Head = 0 ; Head < MAX_HEADS ; Head++) {

				// if the head is present
				if (m_HeadComm[i] != 0) {

					// find the state of the head
					index = ((Config * MAX_HEADS) + Head) * MAX_BLOCKS;
					for (Block = 0 ; Block < m_NumBlocks ; Block++) BlockState |= m_BlockState[index+Block];

					// if the fast layer is used
					if ((Config == 0) || (m_ScannerType != SCANNER_PETSPECT)) {

						// if Fast Crystal Peaks have changed
						if ((BlockState & STATE_FAST_TIME_CHANGED) != 0) {

							// determine where it is to be stored
							DataLayer = LAYER_FAST;
							if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

							// store and download
							index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2);
							hr = pDU->Store_Crystal_Time(Send, Config, Head, DataLayer, &m_Time[index], &Status);

							// if peaks could not be downloaded
							if (Status != 0) {
								sprintf(Str, "Could Not Download Fast Crystal Time Offsets Configuration: %d Head: %d", Config, Head);
								MessageBox(Str, "Error", MB_OK);
							}
						}
					}

					// if the slow layer is used
					if (m_NumLayers[i] == 2) {

						// if Slow Crystal Peaks have changed
						if ((BlockState & STATE_SLOW_TIME_CHANGED) != 0) {

							// store and download
							index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2) + MAX_HEAD_CRYSTALS;
							hr = pDU->Store_Crystal_Time(Send, Config, Head, LAYER_SLOW, &m_Time[index], &Status);

							// if peaks could not be downloaded
							if (Status != 0) {
								sprintf(Str, "Could Not Download Slow Crystal Crystal Time Configuration: %d Head: %d", Config, Head);
								MessageBox(Str, "Error", MB_OK);
							}
						}
					}
				}
			}
		}
	}

	// if any alalog settings were changed but not saved
	if ((TotalState & STATE_SETTINGS_CHANGED) != 0) {

		// ask user if the changes are to be saved
		if (MessageBox("Analog Settings Changed, But Not Saved.  Save Now?", "Question", MB_YESNO) == IDYES) {

			// loop through the heads and configurations
			for (Config = 0 ; Config < m_NumConfigurations ; Config++) for (Head = 0 ; Head < MAX_HEADS ; Head++) {

				// if the head is present
				if (m_HeadComm[Head] != 0) {

					// find the state of the head
					index = ((Config * MAX_HEADS) + Head) * MAX_BLOCKS;
					for (Block = 0 ; Block < m_NumBlocks ; Block++) BlockState |= m_BlockState[index+Block];

					// if analog card settings changed for this head
					if ((BlockState & STATE_SETTINGS_CHANGED) != 0) {

						// send the current settings
						index = ((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS;
						hr = pDU->Store_Settings(Send, Config, Head, &m_Settings[index], &Status);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDestroy", "Method Failed (pDU->Store_Settings), HRESULT", hr);

						// if failed to send settings
						if (Status != 0) {
							sprintf(Str, "Error Sending Analog Settings Head: %d", i);
							MessageBox(Str, "Error", MB_OK);
						}
					}
				}
			}
		}
	}

	// switch all heads to all blocks
	for (Head = 0 ; (Head < MAX_HEADS) && (pDHI != NULL) && (m_ScannerType == SCANNER_HRRT) ; Head++) {
		if (m_HeadComm[Head] == 1) {

			// send command
			sprintf((char *) CommandString, "I -1");
			hr = pDHI->Pass_Command(Head, CommandString, ResponseString, &Status);
			if (FAILED(hr)) Add_Error(pErrEvtSup, "OnDestroy", "Failed pDHI->Pass_Command(Head, CommandString, ResponseString, &Status), HRESULT =", hr);
		}
	}

	// put scanner in scan mode if requested
	if ((pDHI != NULL) && InitScan) {

		// put in scan mode
		hr = pDHI->Initialize_PET(SCAN_EM, m_ConfigurationLLD[0], m_ConfigurationULD[0], &Status);
		if (hr != S_OK) {
			MessageBox("Failed Putting Scanner in Scan Mode", "Error", MB_OK);
		} else {

			// create dialog
			pInfoDlg = new InfoDialog;
			sprintf(pInfoDlg->Message, "Putting Scanner in Acquisition Mode");
			pInfoDlg->Create(IDD_INFO_DIALOG, NULL);
			pInfoDlg->ShowWindow(SW_SHOW);
			pInfoDlg->ShowMessage();

			// wait for scanner to finish going into scan mode
			for (Percent = 0 ; (Percent < 100) && (hr == S_OK) ; ) {
				hr = pDHI->Progress(WAIT_ALL_HEADS, &Percent, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDestroy", "Method Failed (pDHI->Progress), HRESULT", hr);
				if ((Percent < 100) && (Status == 0)) Sleep(1000);
			}

			// destroy information dialog
			pInfoDlg->DestroyWindow();
		}
	}

	CDialog::OnDestroy();

	// release com servers (if attached)
	if (pDHI != NULL) pDHI->Release();
	if (pDU != NULL) pDU->Release();
#ifndef ECAT_SCANNER

	// if server locked, then unlock it
	if (m_LockCode != -1) hr = pLock->UnlockACS(m_LockCode);

	// release lock manager and error event support
	if (pLock != NULL) pLock->Release();
	if (pErrEvtSup != NULL) delete pErrEvtSup;
#endif
	if (m_hMutex != NULL) {
		::ReleaseMutex(m_hMutex);
		::CloseHandle(m_hMutex);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPrevious
// Purpose:		Actions taken when the "Prevoius" block button is pressed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnPrevious() 
{
	// local variables
	char Str[MAX_STR_LEN];

	// if not the first block, decrement it
	if (m_CurrBlock > 0) {

		// decrement
		m_CurrBlock--;

		// insert back
		sprintf(Str, "%d", m_CurrBlock);
		m_Block.SetWindowText(Str);

		// refresh block
		UpdateBlock();
	} 
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnNext
// Purpose:		Actions taken when the "Next" block button is pressed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnNext() 
{
	// local variables
	char Str[MAX_STR_LEN];

	// increment and redisplay if it will stay in range
	if (m_CurrBlock < (m_NumBlocks-1)) {

		// decrement
		m_CurrBlock++;

		// insert back
		sprintf(Str, "%d", m_CurrBlock);
		m_Block.SetWindowText(Str);

		// refresh block
		UpdateBlock();
	} 
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeAcquireHeaddrop
// Purpose:		Actions taken when a new selection is made from the Acquisition 
//				head drop list
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				21 Nov 03	T Gremillion	save selection to compare against next change
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSelchangeAcquireHeaddrop() 
{
	// local variables
	long CurrAcq = 0;
	long Curr = 0;

	// if the selection changed
	CurrAcq = m_AcqHeadDrop.GetCurSel();
	if (m_CurrAcqHead != CurrAcq) {

		// save it for the next time
		m_CurrAcqHead = CurrAcq;

		// if acquisition head is not multiple, then change it with the current head
		if (CurrAcq < m_NumHeads) {

			// set the acquisition head
			m_HeadDrop.SetCurSel(CurrAcq);
			m_CurrHead = m_HeadIndex[CurrAcq];

			// update head data
			UpdateHead();
			Invalidate();
		}

		// let the control state be updated
		Enable_Controls();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnFileExit
// Purpose:		Actions taken when the menu selection File->Exit is selected
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnFileExit() 
{
	OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Disable_Controls()
// Purpose:		Disables all controls on GUI to prevent further user input while
//				the previous input is being processed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::Disable_Controls()
{
	// disable bitmap click
	m_ControlState = 0;

	// hourglass cursor
	BeginWaitCursor();

	// Buttons
	m_PreviousButton.EnableWindow(FALSE);
	m_NextButton.EnableWindow(FALSE);
	m_EditButton.EnableWindow(FALSE);
	m_AcquireButton.EnableWindow(FALSE);
	m_AbortButton.EnableWindow(FALSE);
	m_DefaultsButton.EnableWindow(FALSE);
	m_AnalyzeButton.EnableWindow(FALSE);
	m_SaveButton.EnableWindow(FALSE);

	// Check Boxes
	m_DNLCheck.EnableWindow(FALSE);
	m_DividerCheck.EnableWindow(FALSE);
	m_MasterCheck.EnableWindow(FALSE);
	m_PeaksCheck.EnableWindow(FALSE);
	m_RegionCheck.EnableWindow(FALSE);
	m_ThresholdCheck.EnableWindow(FALSE);
	m_TransmissionCheck.EnableWindow(FALSE);

	// Drop Lists
	m_TubeDrop.EnableWindow(FALSE);
	m_AcqTypeDrop.EnableWindow(FALSE);
	m_LayerDrop.EnableWindow(FALSE);
	m_ViewDrop.EnableWindow(FALSE);
	m_ConfigDrop.EnableWindow(FALSE);
	m_AcqHeadDrop.EnableWindow(FALSE);
	m_HeadDrop.EnableWindow(FALSE);

	// Edit Boxes
	m_Block.EnableWindow(FALSE);
	m_CFD.EnableWindow(FALSE);
	m_Shape.EnableWindow(FALSE);
	m_SlowLow.EnableWindow(FALSE);
	m_SlowHigh.EnableWindow(FALSE);
	m_FastLow.EnableWindow(FALSE);
	m_FastHigh.EnableWindow(FALSE);
	m_PMTA.EnableWindow(FALSE);
	m_PMTB.EnableWindow(FALSE);
	m_PMTC.EnableWindow(FALSE);
	m_PMTD.EnableWindow(FALSE);
	m_XOffset.EnableWindow(FALSE);
	m_YOffset.EnableWindow(FALSE);
	m_XPos.EnableWindow(FALSE);
	m_YPos.EnableWindow(FALSE);
	m_Value.EnableWindow(FALSE);
	m_Amount.EnableWindow(FALSE);
	m_VariableField.EnableWindow(FALSE);

	// Slider
	m_SmoothSlider.EnableWindow(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Enable_Controls
// Purpose:		Enables appropriate controls on GUI after the previous input 
//				has been processed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				01 Oct 03	T Gremillion	Enabled Acquire button for system crystal
//											time (acquisition head set to multiple)
//				12 Jan 04	T Gremillion	Activate Edit button for crystal time
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::Enable_Controls()
{
	// local variables
	long Block = 0;
	long View = 0;
	long Config = 0;
	long Head = 0;
	long AcquireHead = 0;
	long Layer = 0;
	long DataLayer = 0;

	char Str[MAX_STR_LEN];

	// retrieve decisive values
	Config = m_ConfigDrop.GetCurSel();
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	AcquireHead = m_AcqHeadDrop.GetCurSel();
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];

	// determine data layer
	DataLayer = Layer;
	if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_FAST;

	// make sure progress bar is properly labeled
	m_ProgressLabel.SetWindowText("Progress:");

	// hide transmission check box if no coincidence point source
	if (0 == m_Transmission) m_TransmissionCheck.ShowWindow(SW_HIDE);

	// analyze and save only available if test set
	if (m_HeadTestSet == 1) {
		m_DefaultsButton.EnableWindow(TRUE);
		m_AnalyzeButton.EnableWindow(TRUE);
		m_SaveButton.EnableWindow(TRUE);
	} else {
		m_DefaultsButton.EnableWindow(FALSE);
		m_AnalyzeButton.EnableWindow(FALSE);
		m_SaveButton.EnableWindow(FALSE);
		m_DefaultsButton.ShowWindow(SW_HIDE);
		m_AnalyzeButton.ShowWindow(SW_HIDE);
		m_SaveButton.ShowWindow(SW_HIDE);
	}

	// previous button only active if not currently the first block
	if (Block > 0) {
		m_PreviousButton.EnableWindow(TRUE);
	} else {
		m_PreviousButton.EnableWindow(FALSE);
	}

	// next button only active if not currently the last block
	if (Block < (m_NumBlocks-1)) {
		m_NextButton.EnableWindow(TRUE);
	} else {
		m_NextButton.EnableWindow(FALSE);
	}

	// Edit available for run mode, position profile, crystal energy and crystal energy peaks
	switch (View) {

	// editing combined layer for petspect is ambiguous
	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_RUN: 
		if (m_ScannerType != SCANNER_PETSPECT) {
			m_EditButton.EnableWindow(TRUE);
		} else {
			m_EditButton.EnableWindow(FALSE);
		}
		break;

	// editing combined layer for dual layer heads is ambiguous
	case DHI_MODE_CRYSTAL_ENERGY:
	case DATA_CRYSTAL_PEAKS:

		// turn edit button on
		m_EditButton.EnableWindow(TRUE);

		// reasons to turn it off
		if ((LAYER_ALL == Layer) && (2 == m_NumLayers[Head])) m_EditButton.EnableWindow(FALSE);
		break;

	case DATA_MODE_TIMING:
	case DATA_TIME_OFFSETS:

		// turn edit button on
		m_EditButton.EnableWindow(TRUE);

		// reasons to turn it off
		if ((LAYER_ALL == Layer) && (2 == m_NumLayers[Head]) && (SCANNER_HRRT != m_ScannerType)) m_EditButton.EnableWindow(FALSE);
		if ((LAYER_ALL != Layer) && (SCANNER_HRRT == m_ScannerType)) m_EditButton.EnableWindow(FALSE);
		break;

	// always on for tdc timing data
	case DHI_MODE_TIME:
		m_EditButton.EnableWindow(TRUE);
		break;

	// off for the other types
	default:
		m_EditButton.EnableWindow(FALSE);
		break;
	}

	// Crystal Peaks, Gain Values and time offset values are not acquired
	m_TransmissionCheck.EnableWindow(FALSE);
	if ((View == DATA_CRYSTAL_PEAKS) || (View == DATA_PMT_GAINS) || (View == DATA_TIME_OFFSETS)) {
		m_AcquireButton.EnableWindow(FALSE);
		m_Amount.EnableWindow(FALSE);
		m_AcqHeadDrop.EnableWindow(FALSE);
		m_AcqTypeDrop.EnableWindow(FALSE);
	} else {
		m_AcquireButton.EnableWindow(TRUE);
		m_Amount.EnableWindow(TRUE);
		if (View == DATA_MODE_TIMING) {
			m_AcqHeadDrop.EnableWindow(FALSE);
			m_AcqHeadDrop.SetCurSel(m_NumHeads);
			m_CurrAcqHead = m_NumHeads;
		} else {
			m_AcqHeadDrop.EnableWindow(TRUE);
		}
		m_AcqTypeDrop.EnableWindow(TRUE);
		if (m_Transmission == 1) m_TransmissionCheck.EnableWindow(TRUE);
	}

	// Abort is only enabled during an acquisition
	m_AbortButton.EnableWindow(FALSE);

	// tube drop list and master check box only valid in tube energy mode
	if (View == DHI_MODE_TUBE_ENERGY) {
		m_TubeDrop.EnableWindow(TRUE);
		if (m_CurrTube == 4) {
			m_MasterCheck.EnableWindow(FALSE);
		} else {
			m_MasterCheck.EnableWindow(TRUE);
		}
	} else {
		m_TubeDrop.EnableWindow(FALSE);
		m_MasterCheck.EnableWindow(FALSE);
	}

	// dnl removal and regions only for position profile
	if (View == DHI_MODE_POSITION_PROFILE) {
		m_DNLCheck.EnableWindow(TRUE);
		m_RegionCheck.EnableWindow(TRUE);
	} else {
		m_DNLCheck.EnableWindow(FALSE);
		m_RegionCheck.EnableWindow(FALSE);
	}

	// mark crystal energy peaks only for crystal energy data
	if (View == DHI_MODE_CRYSTAL_ENERGY) {
		if (m_PeaksValid[(Config*MAX_HEADS*2)+(Head*2)+DataLayer] == 0) {
			m_PeaksCheck.EnableWindow(FALSE);
		} else {
			m_PeaksCheck.EnableWindow(TRUE);
		}
	} else {
		m_PeaksCheck.EnableWindow(FALSE);
	}

	// dividers available for head display not in Shape and tube energy
	if ((View == DHI_MODE_SHAPE_DISCRIMINATION) || (View == DHI_MODE_TUBE_ENERGY)) {
		m_DividerCheck.EnableWindow(FALSE);
	} else {
		m_DividerCheck.EnableWindow(TRUE);
	}

	// shape threshold
	if (View == DHI_MODE_SHAPE_DISCRIMINATION) {
		m_ThresholdCheck.EnableWindow(TRUE);
	} else {
		m_ThresholdCheck.EnableWindow(FALSE);
	}

	// Drop Lists
	m_ViewDrop.EnableWindow(TRUE);
	m_ConfigDrop.EnableWindow(TRUE);
	m_HeadDrop.EnableWindow(TRUE);

	// shape discrimination, you can not change layer
	if ((View == DHI_MODE_SHAPE_DISCRIMINATION) || ((View == DATA_MODE_TIMING) && (SCANNER_HRRT == m_ScannerType))) {
		m_LayerDrop.EnableWindow(FALSE);
	} else {
		m_LayerDrop.EnableWindow(TRUE);
	}

	// swindows affected by scanner type
	switch (m_ScannerType) {

	// PETSPECT, energy offset and shape windows are active
	case SCANNER_PETSPECT:
		m_VariableField.EnableWindow(TRUE);
		m_Shape.EnableWindow(TRUE);
		m_SlowLow.EnableWindow(TRUE);
		m_SlowHigh.EnableWindow(TRUE);
		m_FastLow.EnableWindow(TRUE);
		m_FastHigh.EnableWindow(TRUE);
		break;

	// P39, energy offset is active, shape is not
	case SCANNER_P39:

		// enable Energy offset window
		m_VariableField.EnableWindow(TRUE);

		// disable
		m_Shape.EnableWindow(FALSE);
		m_SlowLow.EnableWindow(FALSE);
		m_SlowHigh.EnableWindow(FALSE);
		m_FastLow.EnableWindow(FALSE);
		m_FastHigh.EnableWindow(FALSE);
		m_ThresholdCheck.EnableWindow(FALSE);

		// hide
		m_ShapeLabel.ShowWindow(SW_HIDE);
		m_Shape.ShowWindow(SW_HIDE);
		m_SlowLabel.ShowWindow(SW_HIDE);
		m_SlowLow.ShowWindow(SW_HIDE);
		m_SlowHigh.ShowWindow(SW_HIDE);
		m_SlowDash.ShowWindow(SW_HIDE);
		m_FastLabel.ShowWindow(SW_HIDE);
		m_FastLow.ShowWindow(SW_HIDE);
		m_FastHigh.ShowWindow(SW_HIDE);
		m_FastDash.ShowWindow(SW_HIDE);
		m_ThresholdCheck.ShowWindow(SW_HIDE);
		break;

	// P39 phase IIA electronics
	case SCANNER_P39_IIA:

		// the Variable field is used for TDC Setup the shape window is used for CFD Setup (not changeable)
		m_VariableField.EnableWindow(FALSE);
		m_Shape.EnableWindow(FALSE);

		// disable shape
		m_SlowLow.EnableWindow(FALSE);
		m_SlowHigh.EnableWindow(FALSE);
		m_FastLow.EnableWindow(FALSE);
		m_FastHigh.EnableWindow(FALSE);
		m_DNLCheck.EnableWindow(FALSE);
		m_ThresholdCheck.EnableWindow(FALSE);

		// hide unused fields
		m_SlowLabel.ShowWindow(SW_HIDE);
		m_SlowLow.ShowWindow(SW_HIDE);
		m_SlowHigh.ShowWindow(SW_HIDE);
		m_SlowDash.ShowWindow(SW_HIDE);
		m_FastLabel.ShowWindow(SW_HIDE);
		m_FastLow.ShowWindow(SW_HIDE);
		m_FastHigh.ShowWindow(SW_HIDE);
		m_FastDash.ShowWindow(SW_HIDE);
		m_DNLCheck.ShowWindow(SW_HIDE);
		m_ThresholdCheck.ShowWindow(SW_HIDE);
		break;

	// HRRT, they are not editable, and not visible
	case SCANNER_HRRT:

		// enable shape and CFD Delay windows
		m_Shape.EnableWindow(TRUE);
		m_VariableField.EnableWindow(TRUE);

		// disable
		m_SlowLow.EnableWindow(FALSE);
		m_SlowHigh.EnableWindow(FALSE);
		m_FastLow.EnableWindow(FALSE);
		m_FastHigh.EnableWindow(FALSE);

		// hide
		m_SlowLabel.ShowWindow(SW_HIDE);
		m_SlowLow.ShowWindow(SW_HIDE);
		m_SlowHigh.ShowWindow(SW_HIDE);
		m_SlowDash.ShowWindow(SW_HIDE);
		m_FastLabel.ShowWindow(SW_HIDE);
		m_FastLow.ShowWindow(SW_HIDE);
		m_FastHigh.ShowWindow(SW_HIDE);
		m_FastDash.ShowWindow(SW_HIDE);
		break;

	case SCANNER_RING:
		m_GainLabel.ShowWindow(SW_HIDE);
		m_ALabel.ShowWindow(SW_HIDE);
		m_PMTA.ShowWindow(SW_HIDE);
		m_BLabel.ShowWindow(SW_HIDE);
		m_PMTB.ShowWindow(SW_HIDE);
		m_CLabel.ShowWindow(SW_HIDE);
		m_PMTC.ShowWindow(SW_HIDE);
		m_DLabel.ShowWindow(SW_HIDE);
		m_PMTD.ShowWindow(SW_HIDE);
		m_XOffLabel.ShowWindow(SW_HIDE);
		m_XOffset.ShowWindow(SW_HIDE);
		m_YOffLabel.ShowWindow(SW_HIDE);
		m_YOffset.ShowWindow(SW_HIDE);
		m_VariableLabel.ShowWindow(SW_HIDE);
		m_VariableField.ShowWindow(SW_HIDE);
		m_CFDLabel.ShowWindow(SW_HIDE);
		m_CFD.ShowWindow(SW_HIDE);
		m_ShapeLabel.ShowWindow(SW_HIDE);
		m_Shape.ShowWindow(SW_HIDE);
		m_SlowLabel.ShowWindow(SW_HIDE);
		m_SlowDash.ShowWindow(SW_HIDE);
		m_SlowHigh.ShowWindow(SW_HIDE);
		m_SlowLow.ShowWindow(SW_HIDE);
		m_FastLabel.ShowWindow(SW_HIDE);
		m_FastDash.ShowWindow(SW_HIDE);
		m_FastHigh.ShowWindow(SW_HIDE);
		m_FastLow.ShowWindow(SW_HIDE);
		m_LayerDrop.ShowWindow(SW_HIDE);
		m_LayerLabel.ShowWindow(SW_HIDE);
		m_ConfigDrop.ShowWindow(SW_HIDE);
		m_ConfigLabel.ShowWindow(SW_HIDE);
		m_DNLCheck.ShowWindow(SW_HIDE);
		m_ThresholdCheck.ShowWindow(SW_HIDE);
		m_RegionCheck.ShowWindow(SW_HIDE);
		m_PeaksCheck.ShowWindow(SW_HIDE);
		m_MasterCheck.ShowWindow(SW_HIDE);
		break;

	// undefined
	default:
		break;
	}

	// set smoothing control
	switch (View) {

	case DHI_MODE_POSITION_PROFILE:
		m_SmoothSlider.EnableWindow(TRUE);
		m_SmoothSlider.SetRange(m_PP_Min_Smooth, m_PP_Max_Smooth, TRUE);
		m_SmoothSlider.SetPos(m_PP_Smooth);
		break;

	case DHI_MODE_TUBE_ENERGY:
		m_SmoothSlider.EnableWindow(TRUE);
		m_SmoothSlider.SetRange(m_TE_Min_Smooth, m_TE_Max_Smooth, TRUE);
		m_SmoothSlider.SetPos(m_TE_Smooth);
		break;

	default:
		m_SmoothSlider.EnableWindow(FALSE);
		break;
	}

	// X and Y Positions never editable
	m_XPos.EnableWindow(FALSE);
	m_YPos.EnableWindow(FALSE);
	m_Value.EnableWindow(FALSE);

	// Edit Boxes
	m_Block.EnableWindow(TRUE);
	m_CFD.EnableWindow(TRUE);
	m_PMTA.EnableWindow(TRUE);
	m_PMTB.EnableWindow(TRUE);
	m_PMTC.EnableWindow(TRUE);
	m_PMTD.EnableWindow(TRUE);
	m_XOffset.EnableWindow(TRUE);
	m_YOffset.EnableWindow(TRUE);

	// if the DAL is in simulation mode, there is no scanner, so acquisition is disabled
	if (m_Simulation) m_AcquireButton.EnableWindow(FALSE);
	if ((AcquireHead != m_NumHeads) && (0 == m_HeadComm[m_HeadIndex[AcquireHead]])) m_AcquireButton.EnableWindow(FALSE);

	// normal cursor
	EndWaitCursor();

	// re-enable bitmap click
	m_ControlState = 1;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDividerCheck
// Purpose:		causes the display to be refreshed when the state of the 
//				divider check box changes.
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnDividerCheck() 
{
	// redisplay
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeViewdrop
// Purpose:		Actions taken when a new selection is made from the View drop list
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSelchangeViewdrop()
{
	// local variable
	long View = 0;

	// drop list values
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];

	// if it is not the view already selected
	if (m_CurrView != View) {

		// build the new layers list
		Build_Layer_List();

		// update the head data
		UpdateHead();

		// update the global variable
		m_CurrView = View;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeHeaddrop
// Purpose:		Actions taken when a new selection is made from the Display 
//				head drop list
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSelchangeHeaddrop() 
{
	// local variables
	long CurrAcq = 0;
	long Head = 0;
	long Index = 0;

	// if it is not the head already selected
	Index = m_HeadDrop.GetCurSel();
	Head = m_HeadIndex[Index];
	if (m_CurrHead != Head) {

		// update the layer list
		Build_Layer_List();

		// get the current acquisition head
		CurrAcq = m_AcqHeadDrop.GetCurSel();

		// if acquisition head is not multiple, then change it with the current head
		if (CurrAcq < m_NumHeads) {
			m_AcqHeadDrop.SetCurSel(Index);
			m_CurrAcqHead = Head;
		}

		// update head data
		UpdateHead();

		// update the global variable
		m_CurrHead = Head;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeConfigdrop
// Purpose:		Actions taken when a new selection is made from the Configuration
//				drop list
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSelchangeConfigdrop() 
{
	// local variables
	long Config = 0;

	// if it is not the same as the current layer, then change it
	Config = m_ConfigDrop.GetCurSel();
	if (m_CurrConfig != Config) {

		// update head data
		UpdateHead();

		// update the global variable
		m_CurrConfig = Config;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeLayerdrop
// Purpose:		Actions taken when a new selection is made from the Layer drop list
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSelchangeLayerdrop() 
{
	// local variables
	long Layer = 0;

	// if it is not the same as the current layer, then change it
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];
	if (m_CurrLayer != Layer) {

		// update head data
		UpdateHead();

		// update the global variable
		m_CurrLayer = Layer;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnRegionCheck
// Purpose:		causes the display to be refreshed when the state of the 
//				regions check box changes.
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnRegionCheck() 
{
	Invalidate();	
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeTubedrop
// Purpose:		Actions taken when a new selection is made from the Tube drop list
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSelchangeTubedrop() 
{
	// local variable
	long Tube = 0;

	// get the tube selection
	Tube = m_TubeDrop.GetCurSel();

	// if the tube selection has changed
	if (Tube != m_CurrTube) {

		// change the tube
		m_CurrTube = Tube;

		// master tube energy is not available if all tubes displayed
		if (m_CurrTube == 4) {
			m_MasterCheck.SetCheck(0);
			m_MasterCheck.EnableWindow(FALSE);
		} else {
			m_MasterCheck.EnableWindow(TRUE);
		}

		// redisplay
		Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnReleasecaptureSmoothSlider
// Purpose:		causes the display to be refreshed when the state of the 
//				smoothing slider changes.
// Arguments:	*pNMHDR		- NMHDR = Windows Control Structure (automatically generated)
//				*pResult	- LRESULT = Windows return value (automatically generated)
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnReleasedcaptureSmoothSlider(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// local variables
	long View = 0;
	long Pos = 0;

	// get pertinent view information
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Pos = m_SmoothSlider.GetPos();

	// act on view
	switch (View) {

	case DHI_MODE_POSITION_PROFILE:

		// keep to even numbers
		if (Pos > m_PP_Smooth) {
			if ((Pos % 2) == 0) Pos++;
		} else {
			if ((Pos % 2) == 0) Pos--;
		}

		// keep in range
		if (Pos < m_PP_Min_Smooth) Pos = m_PP_Min_Smooth;
		if (Pos > m_PP_Max_Smooth) Pos = m_PP_Max_Smooth;

		// set new value
		m_PP_Smooth = Pos;
		m_SmoothSlider.SetPos(Pos);

		// block data will have to be re-generated
		UpdateBlock();
		break;

	case DHI_MODE_TUBE_ENERGY:

		// set new value
		m_TE_Smooth = Pos;

		// redisplay
		Invalidate();
		break;
	}

	// fill in a result
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnMasterCheck
// Purpose:		causes the display to be refreshed when the state of the 
//				master check box changes.
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnMasterCheck() 
{
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnThresholdCheck
// Purpose:		causes the display to be refreshed when the state of the 
//				threshold check box changes.
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnThresholdCheck() 
{
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPeaksCheck
// Purpose:		causes the display to be refreshed when the state of the 
//				peaks check box changes.
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnPeaksCheck() 
{
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnLButtonDown
// Purpose:		controls the response when the left mouse button is pressed in
//				the head or block display areas
// Arguments:	nFlags	-	UINT = Indicates whether various virtual keys are down
//							(automatically generated)
//				point	-	CPoint = x/y coordinates of mouse when left button pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				03 Oct 03	T Gremillion	Clicking in Block display while in shape
//											view changes the shape threshold.
//				21 Jan 04	T Gremillion	corrected block run mode counts retrieved 
//											from for ring scanners (PR 3524)
//				23 Nov 04	T Gremillion	Block number needs to be retrieved and shape value updated
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// local variables
	long x = 0;
	long y = 0;
	long index = 0;
	long View = 0;
	long Block = 0;
	long Config = 0;
	long Head = 0;
	long Layer = 0;
	long DataLayer = 0;
	long NewShape = 0;

	char Str[MAX_STR_LEN];

	// if the controls are disabled, return
	if (m_ControlState != 1) return;

	// change the focus (force a killfocus on any fields)
	SetFocus();

	// get pertinent view information
	Config = m_ConfigDrop.GetCurSel();
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// determine data layer
	DataLayer = Layer;
	if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_FAST;

	// change the focus (force a killfocus on any fields)
	SetFocus();

	// determine if click was in head area
	if ((point.x >= m_Head_Pos_X) && (point.x < (m_Head_Pos_X + m_Display_X)) &&
		(point.y >= m_Head_Pos_Y) && (point.y < (m_Head_Pos_Y + m_Display_Y))) {

		// determine the block number by view type
		switch (View) {

		case DHI_MODE_POSITION_PROFILE:
		case DHI_MODE_CRYSTAL_ENERGY:
		case DHI_MODE_TIME:
		case DATA_MODE_TIMING:

			// convert pixel x/y to block x/y
			x = (point.x - m_Head_Pos_X) / (PP_SIZE_X / m_DisplayDownScale);
			y = (point.y - m_Head_Pos_Y) / (PP_SIZE_Y / m_DisplayDownScale);

			// if it is within the valid x range
			if (((x < m_XBlocks) && (SCANNER_RING != m_ScannerType)) || 
				((y < m_YBlocks) && (SCANNER_RING == m_ScannerType))) {

				// convert to block number
				if (SCANNER_RING == m_ScannerType) {
					Block = (x * m_YBlocks) + y;
				} else {
					Block = (y * m_XBlocks) + x;
				}

				// if it is a valid block but not the current block
				if ((Block < m_NumBlocks) && (m_CurrBlock != Block)) {

					// make it the current block
					m_CurrBlock = Block;
					sprintf(Str, "%d", Block);
					m_Block.SetWindowText(Str);

					// refresh block
					UpdateBlock();
				}
			}
			break;

		case DHI_MODE_TUBE_ENERGY:
		case DHI_MODE_SHAPE_DISCRIMINATION:

			// convert y to block
			Block = (point.y - m_Head_Pos_Y) / 4;

			// if it is a valid block but not the current block
			if ((Block < m_NumBlocks) && (m_CurrBlock != Block)) {

				// make it the current block
				m_CurrBlock = Block;
				sprintf(Str, "%d", Block);
				m_Block.SetWindowText(Str);

				// refresh block
				UpdateBlock();
			}
			break;

		case DHI_MODE_RUN:
		case DATA_TIME_OFFSETS:
		case DATA_CRYSTAL_PEAKS:
		case DATA_PMT_GAINS:

			// convert pixel x/y to block x/y
			x = (point.x - m_Head_Pos_X) / (m_XCrystals * m_DisplayUpScale);
			y = (point.y - m_Head_Pos_Y) / (m_YCrystals * m_DisplayUpScale);

			// if it is within the valid x range
			if (((x < m_XBlocks) && (SCANNER_RING != m_ScannerType)) || 
				((y < m_YBlocks) && (SCANNER_RING == m_ScannerType))) {

				// convert to block number
				if (SCANNER_RING == m_ScannerType) {
					Block = (x * m_YBlocks) + y;
				} else {
					Block = (y * m_XBlocks) + x;
				}

				// if it is a valid block but not the current block
				if ((Block < m_NumBlocks) && (m_CurrBlock != Block)) {

					// make it the current block
					m_CurrBlock = Block;
					sprintf(Str, "%d", Block);
					m_Block.SetWindowText(Str);

					// refresh block
					UpdateBlock();
				}
			}
			break;

		default:
			break;
		}
	}


	// determine if click was in block area
	if ((point.x >= m_Block_Pos_X) && (point.x < (m_Block_Pos_X + 256)) &&
		(point.y >= m_Block_Pos_Y) && (point.y < (m_Block_Pos_Y + 256))) {

		// action based on view
		switch (View) {

		case DHI_MODE_RUN:

			// if data available
			if (m_HeadData != NULL) {

				// translate to crystal
				x = (m_XCrystals * (point.x - m_Block_Pos_X)) / 256;
				y = (m_YCrystals * (point.y - m_Block_Pos_Y)) / 256;

				// display location
				sprintf(Str, "%d", x);
				m_XPos.SetWindowText(Str);
				sprintf(Str, "%d", y);
				m_YPos.SetWindowText(Str);

				// get value at location
				if (SCANNER_RING == m_ScannerType) {
					index = ((((m_CurrBlock % m_YBlocks) * m_YCrystals) + y) * 128) + ((m_CurrBlock / m_YBlocks) * m_XCrystals) + x;
				} else {
					index = ((((m_CurrBlock / m_XBlocks) * m_YCrystals) + y) * 128) + ((m_CurrBlock % m_XBlocks) * m_XCrystals) + x;
				}
				sprintf(Str, "%d", m_HeadData[index]);
				m_Value.SetWindowText(Str);
			}
			break;

		case DHI_MODE_POSITION_PROFILE:

			// if data available
			if (m_HeadData != NULL) {

				// translate to crystal
				x = point.x - m_Block_Pos_X;
				y = point.y - m_Block_Pos_Y;

				// display location
				sprintf(Str, "%d", x);
				m_XPos.SetWindowText(Str);
				sprintf(Str, "%d", y);
				m_YPos.SetWindowText(Str);

				// get value at location
				index = (m_CurrBlock * PP_SIZE) + (y * PP_SIZE_X) + x;
				sprintf(Str, "%d", m_HeadData[index]);
				m_Value.SetWindowText(Str);
			}
			break;

		case DATA_CRYSTAL_PEAKS:

			// if data available
			if (m_PeaksValid[(Config*MAX_HEADS*2)+(Head*2)+DataLayer] == 1) {

				// translate to crystal
				x = (m_XCrystals * (point.x - m_Block_Pos_X)) / 256;
				y = (m_YCrystals * (point.y - m_Block_Pos_Y)) / 256;

				// display location
				sprintf(Str, "%d", x);
				m_XPos.SetWindowText(Str);
				sprintf(Str, "%d", y);
				m_YPos.SetWindowText(Str);

				// get value at location
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (m_CurrBlock * MAX_CRYSTALS) + (y * 16) + x;
				sprintf(Str, "%d", m_Peaks[index]);
				m_Value.SetWindowText(Str);
			}
			break;

		case DHI_MODE_CRYSTAL_ENERGY:

			// if data available
			if ((m_HeadData != NULL) && (m_PeaksValid[(Config*MAX_HEADS*2)+(Head*2)+DataLayer] == 1)) {

				// translate to crystal
				x = (point.y - m_Block_Pos_Y) % 16;
				if (x >= m_XCrystals) x = m_XCrystals - 1;
				y = (point.y - m_Block_Pos_Y) / 16;

				// if beyond data
				if (y >= m_YCrystals) {
					x = m_XCrystals - 1;
					y = m_YCrystals - 1;
				}

				// display location
				sprintf(Str, "%d", x);
				m_XPos.SetWindowText(Str);
				sprintf(Str, "%d", y);
				m_YPos.SetWindowText(Str);

				// get value at location
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (m_CurrBlock * MAX_CRYSTALS) + (y * 16) + x;
				sprintf(Str, "%d", m_Peaks[index]);
				m_Value.SetWindowText(Str);
			}
			break;

		case DHI_MODE_TIME:

			// if data available
			if (m_HeadData != NULL) {

				// translate to crystal
				x = (point.x - m_Block_Pos_X);
				y = (point.y - m_Block_Pos_Y);

				// display location
				sprintf(Str, "%d", x);
				m_XPos.SetWindowText(Str);
				sprintf(Str, "%d", y);
				m_YPos.SetWindowText(Str);

				// get value at location
				index = (Block * PP_SIZE) + (y * PP_SIZE_X) + x;
				sprintf(Str, "%d", m_HeadData[index]);
				m_Value.SetWindowText(Str);
			}
			break;

		case DATA_MODE_TIMING:

			// if data available
			if ((m_HeadData != NULL) && (m_PeaksValid[(Config*MAX_HEADS*2)+(Head*2)+DataLayer] == 1)) {

				// translate to crystal
				x = (point.y - m_Block_Pos_Y) % 16;
				if (x >= m_XCrystals) x = m_XCrystals - 1;
				y = (point.y - m_Block_Pos_Y) / 16;

				// if beyond data
				if (y >= m_YCrystals) {
					x = m_XCrystals - 1;
					y = m_YCrystals - 1;
				}

				// display location
				sprintf(Str, "%d", x);
				m_XPos.SetWindowText(Str);
				sprintf(Str, "%d", y);
				m_YPos.SetWindowText(Str);

				// get value at location
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (m_CurrBlock * MAX_CRYSTALS) + (y * 16) + x;
				sprintf(Str, "%d", m_Time[index]);
				m_Value.SetWindowText(Str);
			}
			break;

		case DATA_TIME_OFFSETS:

			// if data available
			if (m_TimeValid[(Config*MAX_HEADS*2)+(Head*2)+DataLayer] == 1) {

				// translate to crystal
				x = (m_XCrystals * (point.x - m_Block_Pos_X)) / 256;
				y = (m_YCrystals * (point.y - m_Block_Pos_Y)) / 256;

				// display location
				sprintf(Str, "%d", x);
				m_XPos.SetWindowText(Str);
				sprintf(Str, "%d", y);
				m_YPos.SetWindowText(Str);

				// get value at location
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (m_CurrBlock * MAX_CRYSTALS) + (y * 16) + x;
				sprintf(Str, "%d", m_Time[index]);
				m_Value.SetWindowText(Str);
			}
			break;

		case DHI_MODE_SHAPE_DISCRIMINATION:

			// if data available
			if (m_HeadData != NULL) {

				// if click was within graph borders
				if ((point.x >= m_Border_X1) && (point.x <= m_Border_X2) && 
					(point.y >= m_Border_Y1) && (point.y <= m_Border_Y2)) {

					// determine new value
					NewShape = (long)(((double)((point.x - m_Border_X1) * m_Border_Range) / (m_Border_X2 - m_Border_X1)) + 0.5);

					// if it has changed
					index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);
					if (NewShape != m_Settings[index+SetIndex[SHAPE]]) {

						// set the new value
						m_Settings[index+SetIndex[SHAPE]] = NewShape;

						// updaate the shape field
						sprintf(Str, "%d", NewShape);
						m_Shape.SetWindowText(Str);

						// indicate it has changed in the global variables
						index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
						m_BlockState[index] |= STATE_SETTINGS_CHANGED;

						// redisplay
						Invalidate();
					}
				}
			}
			break;

		default:
			break;
		}
	}
	
	CDialog::OnLButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusAmount
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the Amount
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusAmount() 
{
	// local variables
	long Value = 0;

	char Str[MAX_STR_LEN];

	// retrieve
	m_Amount.GetWindowText(Str, 10);

	// if it is not in the valid range, set to previous value
	if (Validate(Str, 1, 2147483647, &Value) != 0) Value = m_CurrAmount;

	// remember the current value
	m_CurrAmount = Value;

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_Amount.SetWindowText(Str);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusPmta
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the PMTA
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusPmta() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_PMTA.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[PMTA]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_PMTA.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[PMTA]]) {

		// set new value
		m_Settings[index+SetIndex[PMTA]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if pmt gains are being displayed, then refresh
		if (View == DATA_PMT_GAINS) UpdateHead();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusPmtb
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the PMTB
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusPmtb() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_PMTB.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[PMTB]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_PMTB.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[PMTB]]) {

		// set new value
		m_Settings[index+SetIndex[PMTB]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if pmt gains are being displayed, then refresh
		if (View == DATA_PMT_GAINS) UpdateHead();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusPmtc
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the PMTC
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusPmtc() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_PMTC.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[PMTC]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_PMTC.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[PMTC]]) {

		// set new value
		m_Settings[index+SetIndex[PMTC]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if pmt gains are being displayed, then refresh
		if (View == DATA_PMT_GAINS) UpdateHead();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusPmtd
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the PMTD
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusPmtd() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_PMTD.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[PMTD]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_PMTD.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[PMTD]]) {

		// set new value
		m_Settings[index+SetIndex[PMTD]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if pmt gains are being displayed, then refresh
		if (View == DATA_PMT_GAINS) UpdateHead();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusXOff
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the X Offset
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusXOff() 
{
	// local variables
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;
	long MinValue = 0;
	long MaxValue = 0;
	long Setting = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// this is variable field, so we need to get the correct values
	if (m_ScannerType == SCANNER_P39_IIA) {
		Setting = SetIndex[TDC_GAIN];
		MinValue = 0;
		MaxValue = 255;
	} else {
		Setting = SetIndex[X_OFFSET];
		MinValue = 0;
		MaxValue = 255;
	}

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_XOffset.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, MinValue, MaxValue, &Value) != 0) Value = m_Settings[index+Setting];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_XOffset.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+Setting]) {

		// set new value
		m_Settings[index+Setting] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusYOff
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the Y Offset
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusYOff() 
{
	// local variables
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;
	long MinValue = 0;
	long MaxValue = 0;
	long Setting = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// this is variable field, so we need to get the correct values
	if (m_ScannerType == SCANNER_P39_IIA) {
		Setting = SetIndex[TDC_OFFSET];
		MinValue = 0;
		MaxValue = 255;
	} else {
		Setting = SetIndex[Y_OFFSET];
		MinValue = 0;
		MaxValue = 255;
	}

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_YOffset.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, MinValue, MaxValue, &Value) != 0) Value = m_Settings[index+Setting];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_YOffset.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+Setting]) {

		// set new value
		m_Settings[index+Setting] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusVariableField
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the text field 
//				used for Energy offset on P39 phase I and CFD Delay on HRRT.
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusVariableField() 
{
	// local variables
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;
	long MinValue = 0;
	long MaxValue = 0;
	long Setting = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// set variable value
	if (m_ScannerType == SCANNER_HRRT) {
		Setting = SetIndex[CFD_DELAY];
		MinValue = 0;
		MaxValue = 255;
	} else if (m_ScannerType == SCANNER_P39_IIA) {
		Setting = SetIndex[TDC_SETUP];
		MinValue = 0;
		MaxValue = 255;
	} else {
		Setting = SetIndex[E_OFFSET];
		MinValue = 0;
		MaxValue = 255;
	}

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_VariableField.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, MinValue, MaxValue, &Value) != 0) Value = m_Settings[index+Setting];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_VariableField.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+Setting]) {

		// set new value
		m_Settings[index+Setting] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusCfd
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the CFD
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusCfd() 
{
	// local variables
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_CFD.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[CFD]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_CFD.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[CFD]]) {

		// set new value
		m_Settings[index+SetIndex[CFD]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusFastHigh
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the Shape 
//				discrimination fast window high threshold text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusFastHigh() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_FastHigh.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[FAST_HIGH]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_FastHigh.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[FAST_HIGH]]) {

		// set new value
		m_Settings[index+SetIndex[FAST_HIGH]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if shape data is being displayed, then refresh
		if (View == DHI_MODE_SHAPE_DISCRIMINATION) Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusFastLow
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the Shape 
//				discrimination fast window low threshold text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusFastLow() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_FastLow.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[FAST_LOW]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_FastLow.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[FAST_LOW]]) {

		// set new value
		m_Settings[index+SetIndex[FAST_LOW]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if shape data is being displayed, then refresh
		if (View == DHI_MODE_SHAPE_DISCRIMINATION) Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusShape
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the Shape 
//				discrimination text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusShape() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;
	long MinValue = 0;
	long MaxValue = 0;
	long Setting = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// this is variable field, so we need to get the correct values
	if (m_ScannerType == SCANNER_P39_IIA) {
		Setting = SetIndex[CFD_SETUP];
		MinValue = 0;
		MaxValue = 255;
	} else {
		Setting = SetIndex[SHAPE];
		MinValue = 0;
		MaxValue = 255;
	}

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_Shape.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, MinValue, MaxValue, &Value) != 0) Value = m_Settings[index+Setting];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_Shape.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+Setting]) {

		// set new value
		m_Settings[index+Setting] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if shape data is being displayed, then refresh
		if ((View == DHI_MODE_SHAPE_DISCRIMINATION) && (m_ScannerType != SCANNER_P39_IIA)) Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusSlowHigh
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the Shape 
//				discrimination slow window high threshold text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusSlowHigh() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_SlowHigh.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[SLOW_HIGH]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_SlowHigh.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[SLOW_HIGH]]) {

		// set new value
		m_Settings[index+SetIndex[SLOW_HIGH]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if shape data is being displayed, then refresh
		if (View == DHI_MODE_SHAPE_DISCRIMINATION) Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusSlowLow
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the Shape 
//				discrimination slow window low threshold text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusSlowLow() 
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long index = 0;
	long Value = 0;

	char Str[MAX_STR_LEN];

	// get data from fields
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// index into settings array
	index = (((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);

	// retrieve
	m_SlowLow.GetWindowText(Str, 10);

	// compare against range
	if (Validate(Str, 0, 255, &Value) != 0) Value = m_Settings[index+SetIndex[SLOW_LOW]];

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_SlowLow.SetWindowText(Str);

	// if the value has changed
	if (Value != m_Settings[index+SetIndex[SLOW_LOW]]) {

		// set new value
		m_Settings[index+SetIndex[SLOW_LOW]] = Value;

		// indicate it has changed in the global variables
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
		m_BlockState[index] |= STATE_SETTINGS_CHANGED;

		// if shape data is being displayed, then refresh
		if (View == DHI_MODE_SHAPE_DISCRIMINATION) Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		PreTranslateMessage
// Purpose:		causes the focus to be changed when the "Enter" key is pressed 
// Arguments:	*pMsg	- MSG = identifies the key that was pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				11 Jun 04	T Gremillion	modified to block Escape Key as well
/////////////////////////////////////////////////////////////////////////////

BOOL CDUGUIDlg::PreTranslateMessage(MSG* pMsg)
{
	// capture the user clicking on the return (Enter) key to kill focus for an edit field
	// or the Escape key to prevent application termination
	if (((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN)) ||
		((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))) {

		// set focus to the main Dialog (activates the killfocus on text fields)
		SetFocus();

		// no need to do a msg translation, so quit. 
		// that way no further processing gets done  
		return TRUE; 
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnEdit
// Purpose:		determines the actions to be performed when the "Edit" button
//				is pressed based on the view. 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				12 Dec 03	T Gremillion	added LLD/ULD for each configuration for crystal peaks
//											added calls to time editor for crystal time
//				15 Apr 04	T Gremillion	config and layer not used to get data on ring scanners
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnEdit() 
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long bx = 0;
	long by = 0;
	long cx = 0;
	long cy = 0;
	long index = 0;
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Layer = 0;
	long DataLayer = 0;
	long Block = 0;
	long Size = 0;
	long ApplyDNL = 0;
	long Status = 0;

	long *BlockData;

	long Peaks[MAX_CRYSTALS];

	double SmoothData[PP_SIZE];

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// editing dialogs
	CCRMEdit *pCRMDlg;
	CCrystalPeaks *pPeaksDlg;
	CCrystalTime *pTimeDlg;

	// further action based on view
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];
	m_Block.GetWindowText(Str, 10);
	Block = atol(Str);

	// if there is no data
	if ((View == DHI_MODE_POSITION_PROFILE) || (View == DHI_MODE_CRYSTAL_ENERGY) || (View == DATA_MODE_TIMING) || (View == DHI_MODE_TIME)) {
		if (m_HeadData == NULL) {
			MessageBox("No Data", "Information", MB_OK);
			return;
		}
	}

	// Disable controls
	Disable_Controls();

	// set data layer
	DataLayer = Layer;
	if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_FAST;

	// transfer values to proper dialog class
	switch (View) {

	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_RUN:

		// allocate
		pCRMDlg = new CCRMEdit;

		// assign values
		pCRMDlg->ScannerType = m_ScannerType;
		pCRMDlg->Config = Config;
		pCRMDlg->Head = Head;
		pCRMDlg->Layer = Layer;
		if (m_ScannerType == SCANNER_HRRT) pCRMDlg->Layer = LAYER_FAST;
		if (m_ScannerType == SCANNER_P39) pCRMDlg->Layer = LAYER_FAST;
		pCRMDlg->Block = Block;
		pCRMDlg->XBlocks = m_XBlocks;
		pCRMDlg->YBlocks = m_YBlocks;
		pCRMDlg->XCrystals = m_XCrystals;
		pCRMDlg->YCrystals = m_YCrystals;
		pCRMDlg->CrossCheck = m_CrossCheck;
		pCRMDlg->CrossDist = m_CrossDist;
		pCRMDlg->EdgeDist = m_EdgeDist;
		pCRMDlg->Simulation = m_Simulation;
		pCRMDlg->State = 0;
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
	case DATA_CRYSTAL_PEAKS:

		// allocate
		pPeaksDlg = new CCrystalPeaks;

		// assign values
		pPeaksDlg->State = 0;
		pPeaksDlg->XCrystals = m_XCrystals;
		pPeaksDlg->YCrystals = m_YCrystals;
		pPeaksDlg->CrystalNum = m_SaveCrystal;
		pPeaksDlg->CrystalSmooth = m_SaveSmooth;
		pPeaksDlg->ScannerType = m_ScannerType;
		pPeaksDlg->ScannerLLD = m_PeaksLLD[Config];
		pPeaksDlg->ScannerULD = m_PeaksULD[Config];

		// ring scanners need to fetch crystal peaks
		if (SCANNER_RING == m_ScannerType) {

			// retrieve the crystral peaks (if they can't be retrieved, issue warning, but continue
			for (i = 0 ; i < MAX_CRYSTALS ; i++) Peaks[i] = 0;
			hr = pDHI->Get_Block_Crystal_Peaks(Head, Block, Peaks, &Status);
			if (FAILED(hr)) Add_Error(pErrEvtSup, "OnEdit", "Method Failed (pDHI->Get_Block_Crystal_Peaks), HRESULT", hr);
			for (i = 0 ; i < MAX_CRYSTALS ; i++) pPeaksDlg->Peak[i] = Peaks[i];

		// crystal peaks have already been read in for panel detectors
		} else {
			for (i = 0 ; i < MAX_CRYSTALS ; i++) {
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + i;
				pPeaksDlg->Peak[i] = m_Peaks[index];
			}
		}
		break;

	case DATA_TIME_OFFSETS:
	case DATA_MODE_TIMING:
	case DHI_MODE_TIME:

		// allocate
		pTimeDlg = new CCrystalTime;

		// assign values
		pTimeDlg->State = 0;
		pTimeDlg->TimingBins = m_TimingBins;
		if (DHI_MODE_TIME == View) pTimeDlg->TimingBins = 256;
		pTimeDlg->XCrystals = m_XCrystals;
		pTimeDlg->YCrystals = m_YCrystals;
		pTimeDlg->CrystalNum = m_TimeCrystal;
		pTimeDlg->Scanner = m_ScannerType;
		pTimeDlg->View = View;

		// system time needs time offset values
		if (View != DHI_MODE_TIME) {

			// ring scanners need to fetch crystal peaks
			if (SCANNER_RING == m_ScannerType) {

				// retrieve the crystral peaks
				for (i = 0 ; i < MAX_CRYSTALS ; i++) Peaks[i] = 0;
				hr = pDHI->Get_Block_Time_Correction(Head, Block, Peaks, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnEdit", "Method Failed (pDHI->Get_Block_Time_Correction), HRESULT", hr);
				for (i = 0 ; i < MAX_CRYSTALS ; i++) pTimeDlg->Time[i] = Peaks[i];

			// time corrections have already been read in for panel detectors
			} else {
				for (i = 0 ; i < MAX_CRYSTALS ; i++) {
					index = (((Config * MAX_HEADS * 2) + (Head * 2) + DataLayer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + i;
					pTimeDlg->Time[i] = m_Time[index];
				}
			}
		}
		break;

	default:
		break;
	}

	// call correct editing dialog
	switch (View) {

	case DHI_MODE_POSITION_PROFILE:

		// transfer data
		for (i = 0 ; i < PP_SIZE ; i++) pCRMDlg->pp[i] = m_BlockImg[i];

		// Bring up the the CRM Edit Dialog
		pCRMDlg->DoModal();
		break;

	case DHI_MODE_RUN:

		// Retrieve the block data
		hr = pDU->Current_Block_Data(DHI_MODE_POSITION_PROFILE, Config, Head, Layer, Block, &Size, &BlockData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnEdit", "Method Failed (pDU->Current_Block_Data[PP]), HRESULT", hr);

		// if data not retrieved
		if ((Status != 0) || (Size != PP_SIZE)) {

			// Let User Know
			MessageBox("Unable To Retrieve Position Profile", "Error", MB_OK);

		// otherwise
		} else {

			// smooth data
			if (((SCANNER_HRRT == m_ScannerType) || (SCANNER_PETSPECT == m_ScannerType) || (SCANNER_P39 == m_ScannerType)) && (m_DNLCheck.GetCheck() == 1)) ApplyDNL = 1;
			Smooth_2d(ApplyDNL, m_PP_Smooth, 1, BlockData, SmoothData);
			for (i = 0 ; i < PP_SIZE ; i++) BlockData[i] = (long) (SmoothData[i] + 0.5);

			// histogram equalize the data
			hist_equal(m_DataScaling, PP_SIZE, BlockData, m_BlockImg);

			// release the memory
			CoTaskMemFree(BlockData);

			// transfer data
			for (i = 0 ; i < PP_SIZE ; i++) pCRMDlg->pp[i] = m_BlockImg[i];

			// Bring up the the CRM Edit Dialog
			pCRMDlg->DoModal();
		}
		break;

	case DHI_MODE_CRYSTAL_ENERGY:

		// transfer data
		index = Block * PP_SIZE;
		for (i = 0 ; i < PP_SIZE ; i++) pPeaksDlg->Data[i] = m_HeadData[index+i];

		// put up edit dialog
		pPeaksDlg->DoModal();
		break;

	case DATA_CRYSTAL_PEAKS:

		// Retrieve the block data
		hr = pDU->Current_Block_Data(DHI_MODE_CRYSTAL_ENERGY, Config, Head, Layer, Block, &Size, &BlockData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnEdit", "Method Failed (pDU->Current_Block_Data[EN]), HRESULT", hr);

		// if data not retrieved
		if ((Status != 0) || (Size != PP_SIZE)) {

			// Let User Know
			MessageBox("Unable To Retrieve Crystal Energy", "Error", MB_OK);

		// otherwise
		} else {

			// Transfer Data
			for (i = 0 ; i < PP_SIZE ; i++) pPeaksDlg->Data[i] = BlockData[i];

			// release the memory
			CoTaskMemFree(BlockData);

			// Bring up the the CRM Edit Dialog
			pPeaksDlg->DoModal();
		}
		break;

	case DATA_MODE_TIMING:

		// block coordinates
		bx = Block % m_XBlocks;
		by = Block / m_XBlocks;

		// Clear the data array
		for (i = 0 ; i < PP_SIZE ; i++) pTimeDlg->Data[i] = 0;

		// each row of crystals
		for (j = 0 ; j < m_YCrystals ; j++) {

			// each crystal
			for (i = 0 ; i < m_XCrystals ; i++) {

				// each Bin
				index = ((((by * m_YCrystals) + j) * 128) + ((bx * m_XCrystals) + i)) * m_TimingBins;
				for (k = 0 ; k < m_TimingBins ; k++) {

					// transfer
					pTimeDlg->Data[(((j*m_XCrystals)+i)*m_TimingBins)+k] = m_HeadData[index+k];
				}
			}
		}

		// put up edit dialog
		pTimeDlg->DoModal();
		break;

	case DATA_TIME_OFFSETS:

		// Retrieve the block data
		hr = pDU->Current_Block_Data(DATA_MODE_TIMING, Config, Head, Layer, Block, &Size, &BlockData, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnEdit", "Method Failed (pDU->Current_Block_Data[TIMING]), HRESULT", hr);

		// if data not retrieved
		if ((Status != 0) || (Size != (m_TimingBins * m_XCrystals * m_YCrystals))) {

			// Let User Know
			MessageBox("Unable To Retrieve Crystal Time", "Error", MB_OK);

		// otherwise
		} else {

			// Transfer Data
			for (i = 0 ; i < (m_TimingBins * m_XCrystals * m_YCrystals) ; i++) pTimeDlg->Data[i] = BlockData[i];

			// release the memory
			CoTaskMemFree(BlockData);

			// Bring up the Edit Dialog
			pTimeDlg->DoModal();
		}
		break;

	case DHI_MODE_TIME:

		// transfer data
		for (i = 0 ; i < PP_SIZE ; i++) pTimeDlg->Data[i] = m_BlockImg[i];

		// Bring up the Edit Dialog
		pTimeDlg->DoModal();
		break;

	default:
		break;
	}

	// transfer values from proper dialog class
	switch (View) {

	case DHI_MODE_POSITION_PROFILE:
	case DHI_MODE_RUN:

		// persistant data
		m_CrossCheck = pCRMDlg->CrossCheck;
		m_CrossDist = pCRMDlg->CrossDist;
		m_EdgeDist = pCRMDlg->EdgeDist;

		// if the CRM was saved but not downloaded, make a note of it
		if (pCRMDlg->State == 1) {

			// indicate it in the global variables
			index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
			m_BlockState[index] |= STATE_CRM_CHANGED;
		}

		// release dialog pointer
		delete pCRMDlg;
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
	case DATA_CRYSTAL_PEAKS:
		m_SaveCrystal = pPeaksDlg->CrystalNum;
		m_SaveSmooth = pPeaksDlg->CrystalSmooth;
		m_PeaksLLD[Config] = pPeaksDlg->ScannerLLD;
		m_PeaksULD[Config] = pPeaksDlg->ScannerULD;
		for (i = 0 ; i < MAX_CRYSTALS ; i++) {
			if ((m_NumLayers[Head] == 1) && (Layer == LAYER_ALL)) {
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + LAYER_FAST) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + i;
			} else {
				index = (((Config * MAX_HEADS * 2) + (Head * 2) + Layer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + i;
			}
			m_Peaks[index] = pPeaksDlg->Peak[i];
		}

		// if the Peak was changed, make a note of it
		if (pPeaksDlg->State == 1) {

			// indicate it in the global variables
			index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
			if ((Layer == LAYER_FAST) || ((m_NumLayers[Head] == 1) && (Layer == LAYER_ALL))) {
				m_BlockState[index] |= STATE_FAST_ENERGY_CHANGED;
			} else {
				m_BlockState[index] |= STATE_SLOW_ENERGY_CHANGED;
			}
		}

		// release dialog pointer
		delete pPeaksDlg;
		break;

	case DATA_TIME_OFFSETS:
	case DATA_MODE_TIMING:
	case DHI_MODE_TIME:

		// last crystal viewed
		m_TimeCrystal = pTimeDlg->CrystalNum;
		
		// transfer back offsets
		if (View != DHI_MODE_TIME) {

			// transfer
			for (i = 0 ; i < MAX_CRYSTALS ; i++) {
				if ((m_NumLayers[Head] == 1) && (Layer == LAYER_ALL)) {
					index = (((Config * MAX_HEADS * 2) + (Head * 2) + LAYER_FAST) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + i;
				} else {
					index = (((Config * MAX_HEADS * 2) + (Head * 2) + Layer) * MAX_HEAD_CRYSTALS) + (Block * MAX_CRYSTALS) + i;
				}
				m_Time[index] = pTimeDlg->Time[i];
			}

			// if the Peak was changed, make a note of it
			if (pTimeDlg->State == 1) {

				// indicate it in the global variables
				index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS) + Block;
				if ((Layer == LAYER_FAST) || ((m_NumLayers[Head] == 1) && (Layer == LAYER_ALL))) {
					m_BlockState[index] |= STATE_FAST_TIME_CHANGED;
				} else {
					m_BlockState[index] |= STATE_SLOW_TIME_CHANGED;
				}
			}
		}

		// release dialog pointer
		delete pTimeDlg;
		break;

	default:
		break;
	}

	// refresh block data (and head data if appropriate)
	if ((View == DATA_CRYSTAL_PEAKS) || (View == DATA_TIME_OFFSETS)) {
		UpdateHead();
	} else {
		UpdateBlock();
	}

	// Re-enable controls
	Enable_Controls();

	// Redisplay
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnAcquire
// Purpose:		causes an acquisition of the selected data type for the selected
//				heads to be started for the amount specified. 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				01 Oct 03	T Gremillion	Added ability to acquire system timing data
//				09 Mar 04	T Gremillion	Removed variable head selection for timing
//											data, all is acquired.  Also, removed loading
//											of crystal energy peaks for timing data (redundant)
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnAcquire() 
{
	// local variables
	long i = 0;
	long index = 0;
	long AcqType = 0;
	long Amount = 0;
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long BlockState = 0;
	long Layer = 0;
	long DataLayer = 0;
	long Xmit = 0;
	long Status = 0;
	long lld = 0;
	long uld = 0;
	long setup_eng = 190;
	long NumAcquire = 0;
	long ModulePair = 0;

	long SourceSelect[MAX_HEADS];
	long LayerSelect[MAX_HEADS];

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	HeadSelection pSelectDlg;

	// retrieve values from GUI
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();
	Head = m_HeadIndex[m_AcqHeadDrop.GetCurSel()];
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];
	Xmit = m_TransmissionCheck.GetCheck();
	m_Amount.GetWindowText(Str, 10);
	Amount = atol(Str);
	AcqType = m_AcqTypeDrop.GetCurSel();

	// lld and uld based on view type
	switch (View) {

	case DHI_MODE_POSITION_PROFILE:
		lld = m_PositionProfileLLD;
		uld = m_PositionProfileULD;
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
		lld = m_CrystalEnergyLLD;
		uld = m_CrystalEnergyULD;
		break;

	case DHI_MODE_TUBE_ENERGY:
		lld = m_TubeEnergyLLD;
		uld = m_TubeEnergyULD;
		break;

	case DHI_MODE_SHAPE_DISCRIMINATION:
		lld = m_ShapeLLD;
		uld = m_ShapeULD;
		break;

	case DHI_MODE_RUN:
	case DHI_MODE_TIME:
	default:
		lld = m_ConfigurationLLD[Config];
		uld = m_ConfigurationULD[Config];
		setup_eng = m_ConfigurationEnergy[Config];
		break;
	}

	// initialize head values
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_HeadSelect[i] = 0;
		SourceSelect[i] = 0;
		LayerSelect[i] = Layer;
	}

	// if "Multiple Heads" selected
	if (Head == -1) {

		// for timing data, all heads/buckets present are used
		if (DATA_MODE_TIMING == View) {

			// acquisition head selection
			for (i = 0 ; i < MAX_HEADS ; i++) {
				if (0 != m_HeadPresent[i]) {
					m_HeadSelect[i] = 1;
					NumAcquire++;
				}
			}

			// P39 needs to have a module pair built up
			if ((SCANNER_P39 == m_ScannerType) || (SCANNER_P39_IIA == m_ScannerType)) {

				// make adjustments to module pair (heads)
				if ((m_HeadSelect[0] != 0) && (m_HeadSelect[2] != 0)) ModulePair |= 0x0002;
				if ((m_HeadSelect[0] != 0) && (m_HeadSelect[3] != 0)) ModulePair |= 0x0004;
				if ((m_HeadSelect[0] != 0) && (m_HeadSelect[4] != 0)) ModulePair |= 0x0008;
				if ((m_HeadSelect[1] != 0) && (m_HeadSelect[3] != 0)) ModulePair |= 0x0010;
				if ((m_HeadSelect[1] != 0) && (m_HeadSelect[4] != 0)) ModulePair |= 0x0020;
				if ((m_HeadSelect[1] != 0) && (m_HeadSelect[5] != 0)) ModulePair |= 0x0040;
				if ((m_HeadSelect[2] != 0) && (m_HeadSelect[4] != 0)) ModulePair |= 0x0080;
				if ((m_HeadSelect[2] != 0) && (m_HeadSelect[5] != 0)) ModulePair |= 0x0100;
				if ((m_HeadSelect[3] != 0) && (m_HeadSelect[5] != 0)) ModulePair |= 0x0200;

				// make adjustments to module pair (point sources)
				if ((m_HeadSelect[1] != 0) && (SourceSelect[1] != 0)) ModulePair |= 0x0400;
				if ((m_HeadSelect[2] != 0) && (SourceSelect[1] != 0)) ModulePair |= 0x0800;
				if ((m_HeadSelect[3] != 0) && (SourceSelect[1] != 0)) ModulePair |= 0x1000;
				if ((m_HeadSelect[1] != 0) && (SourceSelect[4] != 0)) ModulePair |= 0x0040;		// the point source on head 4 shares the
				if ((m_HeadSelect[2] != 0) && (SourceSelect[4] != 0)) ModulePair |= 0x0100;		// module pairs with head 5 since they
				if ((m_HeadSelect[3] != 0) && (SourceSelect[4] != 0)) ModulePair |= 0x0200;		// can't be on the same system
			}

		// singles data being acquired, use the head selection guil
		} else {

			// transfer information
			pSelectDlg.ScannerType = m_ScannerType;
			pSelectDlg.NumHeads = m_NumHeads;
			for (i = 0 ; i < m_NumHeads ; i++) {
				pSelectDlg.HeadIndex[i] = m_HeadIndex[i];
				pSelectDlg.HeadAllow[i] = m_HeadComm[m_HeadIndex[i]];
			}

			// put up selection dialog
			pSelectDlg.DoModal();

			// transfer seletion
			for (i = 0 ; i < m_NumHeads ; i++) {
				m_HeadSelect[m_HeadIndex[i]] = pSelectDlg.Selection[i];
				SourceSelect[m_HeadIndex[i]] = pSelectDlg.Selection[i] * Xmit * m_SourcePresent[m_HeadIndex[i]];
				NumAcquire += m_HeadSelect[m_HeadIndex[i]];
			}
		}

		// if no heads selected, then exit
		if (0 == NumAcquire) return;

	// single head selected
	} else {

		// select head
		m_HeadSelect[Head] = 1;
		SourceSelect[Head] = Xmit * m_SourcePresent[Head];
	}

	// switch label
	m_ProgressLabel.SetWindowText("Initializing:");

	// Disable all controls
	Disable_Controls();

	// for each head
	for (i = 0 ; (i < MAX_HEADS) && (SCANNER_RING != m_ScannerType) ; i++) {

		// if selected
		if (m_HeadSelect[i] == 1) {

			// set the setup energy for each block
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {
				index = (((Config * MAX_HEADS) + i) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);
				m_Settings[index+SetIndex[SETUP_ENG]] = setup_eng;
			}

			// send the current settings
			index = ((Config * MAX_HEADS) + i) * MAX_BLOCKS * MAX_SETTINGS;
			hr = pDU->Store_Settings(SEND, Config, i, &m_Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDU->Store_Settings), HRESULT", hr);

			// if failed to send settings
			if (Status != 0) {
				sprintf(Str, "Error Sending Analog Settings Head: %d", i);
				MessageBox(Str, "Error", MB_OK);
				Enable_Controls();
				return;
			}

			// clear analog settings state change flag for each block
			index = (Config * MAX_TOTAL_BLOCKS) + (i * MAX_BLOCKS);
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {
				m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_SETTINGS_CHANGED;
			}

			// for run mode, the crystal energy peaks need to be loaded (and possibly downloaded)
			// for system timing (which takes data in run mode), it might need to be downloaded
			// (initializing the scanner for acquisition will take care of the load if needed)
			if ((View == DHI_MODE_RUN) || (View == DATA_MODE_TIMING)) {

				// check if they need to be downloaded
				index = (Config * MAX_TOTAL_BLOCKS) + (i * MAX_BLOCKS);
				for (BlockState = 0, Block = 0 ; Block < m_NumBlocks ; Block++) BlockState |= m_BlockState[index+Block];

				// for system timing, load the fast time offsets
				if ((DATA_MODE_TIMING == View) && (SCANNER_HRRT != m_ScannerType) && ((Config == 0) || (m_ScannerType != SCANNER_PETSPECT))) {

					// if Fast Crystal time offests
					if ((BlockState & STATE_FAST_TIME_CHANGED) != 0) {

						// determine data layer
						DataLayer = LAYER_FAST;
						if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

						// store and download
						index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (i * MAX_HEAD_CRYSTALS * 2);
						hr = pDU->Store_Crystal_Time(SEND, Config, i, DataLayer, &m_Time[index], &Status);

						// if peaks could not be downloaded
						if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDU->Store_Crytal_Time), HRESULT", hr);
						if (Status != 0) {
							sprintf(Str, "Could Not Download Fast Crystal Time Offets Head: %d", i);
							MessageBox(Str, "Error", MB_OK);
							Enable_Controls();
							return;
						}

						// clear fast crystal time offsets change flag for each block
						index = (Config * MAX_TOTAL_BLOCKS) + (i * MAX_BLOCKS);
						for (Block = 0 ; Block < m_NumBlocks ; Block++) {
							m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_FAST_TIME_CHANGED;
						}
					}
				}

				// for run or system time, load fast energy peaks
				if ((Config == 0) || (m_ScannerType != SCANNER_PETSPECT)) {

					// if Fast Crystal Peaks have changed
					if ((BlockState & STATE_FAST_ENERGY_CHANGED) != 0) {

						// determine data layer
						DataLayer = LAYER_FAST;
						if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

						// store and download
						index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (i * MAX_HEAD_CRYSTALS * 2);
						hr = pDU->Store_Energy_Peaks(SEND, Config, i, DataLayer, &m_Peaks[index], &Status);

						// if peaks could not be downloaded
						if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDU->Store_Energy_Peaks), HRESULT", hr);
						if (Status != 0) {
							sprintf(Str, "Could Not Download Fast Crystal Peaks Head: %d", i);
							MessageBox(Str, "Error", MB_OK);
							Enable_Controls();
							return;
						}

						// clear fast crystal peaks state change flag for each block
						index = (Config * MAX_TOTAL_BLOCKS) + (i * MAX_BLOCKS);
						for (Block = 0 ; Block < m_NumBlocks ; Block++) {
							m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_FAST_ENERGY_CHANGED;
						}
					}
				}

				// if the slow layer is used
				if (m_NumLayers[i] == 2) {

					// slow crystal time offsets
					if ((DATA_MODE_TIMING == View) && (SCANNER_HRRT != m_ScannerType)) {

						// if Fast Crystal time offests
						if ((BlockState & STATE_SLOW_TIME_CHANGED) != 0) {

							// store and download
							index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (i * MAX_HEAD_CRYSTALS * 2);
							hr = pDU->Store_Crystal_Time(SEND, Config, i, LAYER_SLOW, &m_Time[index], &Status);

							// if peaks could not be downloaded
							if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDU->Store_Crytal_Time), HRESULT", hr);
							if (Status != 0) {
								sprintf(Str, "Could Not Download Slow Crystal Time Offets Head: %d", i);
								MessageBox(Str, "Error", MB_OK);
								Enable_Controls();
								return;
							}

							// clear fast crystal time offsets change flag for each block
							index = (Config * MAX_TOTAL_BLOCKS) + (i * MAX_BLOCKS);
							for (Block = 0 ; Block < m_NumBlocks ; Block++) {
								m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_SLOW_TIME_CHANGED;
							}
						}
					}


					// if Slow Crystal Peaks have changed
					if ((BlockState & STATE_SLOW_ENERGY_CHANGED) != 0) {

						// store and download
						index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (i * MAX_HEAD_CRYSTALS * 2) + MAX_HEAD_CRYSTALS;
						hr = pDU->Store_Energy_Peaks(SEND, Config, i, LAYER_SLOW, &m_Peaks[index], &Status);

						// if peaks could not be downloaded
						if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDU->Store_Energy_Peaks), HRESULT", hr);
						if (Status != 0) {
							sprintf(Str, "Could Not Download Slow Crystal Peaks Head: %d", i);
							MessageBox(Str, "Error", MB_OK);
							Enable_Controls();
							return;
						}

						// clear slow crystal peaks state change flag for each block
						index = (Config * MAX_TOTAL_BLOCKS) + (i * MAX_BLOCKS);
						for (Block = 0 ; Block < m_NumBlocks ; Block++) {
							m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_SLOW_ENERGY_CHANGED;
						}
					}
				}

				// if energy rams not currently loaded
				if ((View == DHI_MODE_RUN) && (m_EnergyState[(Config*MAX_HEADS)+i] != 1)) {

					// load them
					hr = pDHI->Load_RAM(Config, i, RAM_ENERGY_PEAKS, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDHI->Load_RAM), HRESULT", hr);

					// if peaks could not be loaded into RAM
					if (Status != 0) {
						sprintf(Str, "Could Not Load Crystal Energy Peaks Head: %d", i);
						MessageBox(Str, "Error", MB_OK);
						Enable_Controls();
						return;
					}

					// energy rams are now loaded
					m_EnergyState[(Config*MAX_HEADS)+i] = 1;
				}

			// other data types, the crystal energy peaks might need to be zapped
			} else {

				// only zap if ambiguous or known to be set
				if (m_EnergyState[(Config*MAX_HEADS)+i] != 0) {

					// issue zap command
					hr = pDHI->Zap(ZAP_ENERGY, Config, i, ALL_BLOCKS, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDHI->Zap), HRESULT", hr);

					// if the head could not be zapped
					if (Status != 0) {
						sprintf(Str, "Could Not Zap Head: %d", i);
						MessageBox(Str, "Error", MB_OK);
						Enable_Controls();
						return;
					}

					// set state as zapped
					m_EnergyState[(Config*MAX_HEADS)+i] = 0;
				}
			}
		}
	}

	// start acquisition
	m_Abort = 0;
	if (View == DATA_MODE_TIMING) {
		hr = pDU->Acquire_Coinc_Data(View, Config, m_HeadSelect, LayerSelect, ModulePair, Xmit, AcqType, Amount, lld, uld, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDU->Acquire_Coinc_Data), HRESULT", hr);
	} else {
		hr = pDU->Acquire_Singles_Data(View, Config, m_HeadSelect, SourceSelect, LayerSelect, AcqType, Amount, lld, uld, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquire", "Method Failed (pDU->Acquire_Singles_Data), HRESULT", hr);
	}

	// if error starting acquisition, inform user and exit
	if (Status != 0) {
		MessageBox("Error Starting Acquisition", "Error", MB_OK);
		return;
	}

	// Enable the abort button
	m_AbortButton.EnableWindow(TRUE);

	// set progress bar to 0%
	m_AcquirePercent = 0;
	m_AcquireProgress.SetPos(m_AcquirePercent);

	// launch progress thread
	m_AcquireMaster = 0;
	hProgressThread = (HANDLE) _beginthread(ProgressThread, 0, NULL);

	// if failed to start thread
	if (hProgressThread == NULL) {

		// inform user
		MessageBox("Failed To Start Progess Thread", "Error", MB_OK);

		// Re-enable controls
		Enable_Controls();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		ProgressThread
// Purpose:		monitors the progress of an acquisition and updates progress bar 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				21 Nov 03	T Gremillion	set flag when acquisition is in progress
/////////////////////////////////////////////////////////////////////////////

void ProgressThread(void *dummy)
{
	// local variables
	long i = 0;
	long Status = 0;

	long HeadStatus[MAX_HEADS];

	HRESULT hr = S_OK;

	IDUMain *pThreadDU = NULL;

#ifdef ECAT_SCANNER
	
	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

#else
	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if error support not establisted, note it in log
	if (pErrEvtSupThread != NULL)  if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}
#endif

	// set a flag show that an acquisition is in progress
	m_AcquisitionFlag = 1;

	// Create an instance of Detector Utilities Server. for this thread
	pThreadDU = Get_DU_Ptr(pErrEvtSup);
	if (pThreadDU == NULL) {
		Add_Error(pErrEvtSupThread, "ProgressThread", "Failed to Get DU Pointer", 0);
		sprintf(m_MessageStr, "Failed To Attach To Utilities Server");
		PostMessage((HWND) m_MyWin, WM_USER_PROCESSING_MESSAGE, 0, 0);
		m_AcquireMaster = 0;
	} else {

		// increment counter
		for ( ; m_AcquirePercent < 100 ; ) {
			hr = pThreadDU->Acquisition_Progress(&m_AcquirePercent, HeadStatus);
			if (FAILED(hr)) {
				Status = Add_Error(pErrEvtSupThread, "ProgressThread", "Method Failed (pThreadDU->Acquisition_Progress), HRESULT", hr);
				for (i = 0 ; i < MAX_HEADS ; i++) HeadStatus[i] = Status;
				m_AcquirePercent = 100;
			}
			PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			Sleep(100);
		}

		// check for acquisition errors
		for (i = 0 ; i < MAX_HEADS ; i++) {
			if ((HeadStatus[i] != 0) && (m_HeadSelect[i] != 0) && (m_Abort == 0)) {
				sprintf(m_MessageStr, "Problems Acquiring Head %d", i);
				PostMessage((HWND) m_MyWin, WM_USER_PROCESSING_MESSAGE, 0, 0);
				m_AcquireMaster = 0;
			}
		}

		// release pointers
		if (pThreadDU != NULL) pThreadDU->Release();
#ifndef ECAT_SCANNER
		if (pErrEvtSupThread != NULL) delete pErrEvtSupThread;
#endif
	}

	// clear acquisition flag
	m_AcquisitionFlag = 0;

	// acquisition complete
	PostMessage((HWND) m_MyWin, WM_USER_PROCESSING_COMPLETE, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPostProgress
// Purpose:		callback to Update progress bar during acquisition 
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		16 Sep 03	T Gremillion	History Started
//				22 Jan 04	T Gremillion	when aborting, label is "Aborting"
//											and progress bar is 100%
/////////////////////////////////////////////////////////////////////////////

LRESULT CDUGUIDlg::OnPostProgress(WPARAM wp, LPARAM lp)
{
	// set progress bar
	m_AcquireProgress.SetPos(m_AcquirePercent);

	// if abort button has been pressed,switch label and set progress bar to 100 %
	if (m_Abort == 1) {
		m_ProgressLabel.SetWindowText("Aborting:");
		m_AcquireProgress.SetPos(100);

	// otherwise if acquisition has started, then switch label
	} else if (m_AcquirePercent > 0) 
		m_ProgressLabel.SetWindowText("Acquiring:");

	// requires result
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnProcessingComplete
// Purpose:		callback to signal acquisition completion 
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

LRESULT CDUGUIDlg::OnProcessingComplete(WPARAM wp, LPARAM lp)
{
	// local variables
	long View = 0;
	long Config = 0;
	long Head = 0;
	long Layer = 0;
	long Time = 0;
	long Size = 0;
	long Status = 0;

	long *HeadData;

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// get current selections
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();

	// if master tube energy is being acquired
	if (m_AcquireMaster == 1) {

		// change message
		m_ProgressLabel.SetWindowText("Downloading");

		// loop though the heads
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// if the head was selected
			if (m_HeadSelect[Head] != 0) {

				// get the layer for the current head
				Layer = m_MasterLayer[Head];

				// retrieve the tube energy data
				hr = pDU->Current_Head_Data(DHI_MODE_TUBE_ENERGY, Config, Head, Layer, &Time, &Size, &HeadData, &Status);
				if (FAILED(hr)) {
					Status = Add_Error(pErrEvtSup, "OnProcessingComplete", "Method Failed (pDU->Current_Head_Data), HRESULT", hr);
					MessageBox("Failed Method: pDU->Current_Head_Data");
				} else if (Status != 0) {
					sprintf(Str, "Failed To Retrieve Tube Energy Data For Head: %d, Config: %d", Head, Config);
					MessageBox(Str);
				} else if (Size != (m_NumBlocks * 256 * 4)) {
					sprintf(Str, "Incorrect Tube Energy Data Size For Head: %d, Config: %d", Head, Config);
					MessageBox(Str);
				} else {

					// write it as a master
					hr = pDU->Write_Master(DHI_MODE_TUBE_ENERGY, Config, Head, Layer, Size, &HeadData, &Status);
					if (FAILED(hr)) {
						Status = Add_Error(pErrEvtSup, "OnProcessingComplete", "Method Failed (pDU->Write_Master), HRESULT", hr);
						MessageBox("Failed Method: pDU->Write_Master");
					} else if (Status != 0) {
						sprintf(Str, "Failed To Write Master Tube Energy Data For Head: %d, Config: %d", Head, Config);
						MessageBox(Str);
					}

					// Release memory
					CoTaskMemFree(HeadData);
				}
			}
		}
	}

	// set progress bar back to 0%
	m_AcquirePercent = 0;
	m_AcquireProgress.SetPos(m_AcquirePercent);

	// update display if applicable
	if ((m_AcquireMaster == 0) || (View == DHI_MODE_TUBE_ENERGY)) UpdateHead();

	// re-enable controls
	Enable_Controls();

	// requires result
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnProcessingMessage
// Purpose:		callback to post messages during acquisition 
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

LRESULT CDUGUIDlg::OnProcessingMessage(WPARAM wp, LPARAM lp)
{
	// error message box
	MessageBox(m_MessageStr, "Error", MB_OK);

	// requires result
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnAbort
// Purpose:		causes an acquisition to abort. 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				22 Jan 04	T Gremillion	when aborting, label is "Aborting"
//											and disable button
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnAbort() 
{
	// local variables
	HRESULT hr = S_OK;

	// flag as an abort
	m_Abort = 1;

	// disable abort button
	m_AbortButton.EnableWindow(FALSE);

	// put up message
	m_ProgressLabel.SetWindowText("Aborting:");

	// issue the abort command to the detector utilities COM server
	hr = pDU->Acquisition_Abort();
	
	// inform user if there was a problem
	if (FAILED(hr)) {
		MessageBox("Server Problem Stopping Acquisition", "Error", MB_OK);
		Add_Error(pErrEvtSup, "OnAbort", "Method Failed (pDU->Acquisition_Abort), HRESULT", hr);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDownloadCrms
// Purpose:		Menu File->Download CRMS - causes all CRMs for selected head(s)
//				to be downloaded
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnDownloadCrms() 
{
	// local variables
	long i = 0;
	long index = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long NumDownload = 0;
	long Status = 0;

	long BlockSelect[MAX_TOTAL_BLOCKS];
	long BlockStatus[MAX_TOTAL_BLOCKS];

	HRESULT hr = S_OK;

	HeadSelection pSelectDlg;

	// get the configuration from the main GUI
	Config = m_ConfigDrop.GetCurSel();

	// initialize head selection
	for (i = 0 ; i < MAX_HEADS ; i++) m_HeadSelect[i] = 0;

	// transfer information
	pSelectDlg.ScannerType = m_ScannerType;
	pSelectDlg.NumHeads = m_NumHeads;
	for (i = 0 ; i < m_NumHeads ; i++) {
		pSelectDlg.HeadIndex[i] = m_HeadIndex[i];
		pSelectDlg.HeadAllow[i] = m_HeadComm[m_HeadIndex[i]];
	}

	// put up selection dialog
	pSelectDlg.DoModal();

	// transfer seletion
	for (i = 0 ; i < m_NumHeads ; i++) {
		m_HeadSelect[m_HeadIndex[i]] = pSelectDlg.Selection[i];
		NumDownload += m_HeadSelect[m_HeadIndex[i]];
	}

	// if no heads selected, then return to dialog
	if (NumDownload == 0) return;

	// initialize blocks
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
		BlockSelect[i] = 0;
		BlockStatus[i] = 0;
	}

	// select blocks
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {
		if (m_HeadSelect[Head] == 1) {
			for (Block = 0 ; Block < (m_XBlocks * m_YBlocks) ; Block++) BlockSelect[(Head*MAX_BLOCKS)+Block] = 1;
		}
	}

	// download
	hr = pDU->Download_CRM(Config, BlockSelect, BlockStatus);
	for (Block = 0 ; Block < MAX_TOTAL_BLOCKS ; Block++) Status += BlockStatus[Block];
	if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDownloadCrms", "Method Failed (pDU->Download_CRM), HRESULT", hr);

	// if it could not be downloaded
	if (Status != 0) {
		MessageBox("Could Not Download CRMs", "Error", MB_OK);
		return;
	}

	// clear any flags
	for (Block = 0 ; Block < MAX_TOTAL_BLOCKS ; Block++) {
		index = (Config * MAX_TOTAL_BLOCKS) + Block;
		if ((BlockSelect[Block] == 1) && ((m_BlockState[index] & STATE_CRM_CHANGED) != 0)) {
			m_BlockState[index] -= STATE_CRM_CHANGED;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnAcquireMasterTubeEnergy
// Purpose:		causes an acquisition of tube energy data for the selected
//				heads to be started and copied to the master files when completed. 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnAcquireMasterTubeEnergy() 
{
	// local variables
	long i = 0;
	long index = 0;
	long View = DHI_MODE_TUBE_ENERGY;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long Amount = 60;
	long AcqType = ACQUISITION_SECONDS;
	long NumAcquire = 0;
	long Status = 0;
	long lld = m_TubeEnergyLLD;
	long uld = m_TubeEnergyULD;
	long setup_eng = 190;

	long SourceSelect[MAX_HEADS];
	long LayerSelect[MAX_HEADS];

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	HeadSelection pSelectDlg;

	// get the configuration from the main GUI
	Config = m_ConfigDrop.GetCurSel();

	// initialize head selection
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_HeadSelect[i] = 0;
		SourceSelect[i] = 0;
	}

	// transfer information
	pSelectDlg.ScannerType = m_ScannerType;
	pSelectDlg.NumHeads = m_NumHeads;
	for (i = 0 ; i < m_NumHeads ; i++) {
		pSelectDlg.HeadIndex[i] = m_HeadIndex[i];
		pSelectDlg.HeadAllow[i] = m_HeadComm[m_HeadIndex[i]];
	}

	// put up selection dialog
	pSelectDlg.DoModal();

	// transfer seletion
	for (i = 0 ; i < m_NumHeads ; i++) {
		m_HeadSelect[m_HeadIndex[i]] = pSelectDlg.Selection[i];
		NumAcquire += m_HeadSelect[m_HeadIndex[i]];
	}

	// if no heads selected, then return to dialog
	if (NumAcquire == 0) return;

	// possible to have a different layer for each head
	for (i = 0 ; i < MAX_HEADS ; i++) LayerSelect[i] = m_MasterLayer[i];

	// switch label
	m_ProgressLabel.SetWindowText("Initializing:");

	// Disable all controls
	Disable_Controls();

	// for each head
	for (i = 0 ; i < MAX_HEADS ; i++) {

		// if selected
		if (m_HeadSelect[i] == 1) {

			// set the setup energy for each block
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {
				index = (((Config * MAX_HEADS) + i) * MAX_BLOCKS * MAX_SETTINGS) + (Block * m_NumSettings);
				m_Settings[index+SetIndex[SETUP_ENG]] = setup_eng;
			}

			// send the current settings
			index = ((Config * MAX_HEADS) + i) * MAX_BLOCKS * MAX_SETTINGS;
			hr = pDU->Store_Settings(SEND, Config, i, &m_Settings[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquireMasterTubeEnergy", "Method Failed (pDU->Store_Settings), HRESULT", hr);

			// if failed to send settings
			if (Status != 0) {
				sprintf(Str, "Error Sending Analog Settings Head: %d", i);
				MessageBox(Str, "Error", MB_OK);
				Enable_Controls();
				return;
			}

			// clear analog settings state change flag for each block
			index = (Config * MAX_TOTAL_BLOCKS) + (i * MAX_BLOCKS);
			for (Block = 0 ; Block < m_NumBlocks ; Block++) {
				m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_SETTINGS_CHANGED;
			}

			// only zap if ambiguous or known to be set
			if (m_EnergyState[(Config*MAX_HEADS)+i] != 0) {

				// issue zap command
				hr = pDHI->Zap(ZAP_ENERGY, Config, i, ALL_BLOCKS, &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquireMasterTubeEnergy", "Method Failed (pDHI->Zap), HRESULT", hr);

				// if the head could not be zapped
				if (Status != 0) {
					sprintf(Str, "Could Not Zap Head: %d", i);
					MessageBox(Str, "Error", MB_OK);
					Enable_Controls();
					return;
				}

				// set state as zapped
				m_EnergyState[(Config*MAX_HEADS)+i] = 0;
			}
		}
	}

	// start acquisition
	m_Abort = 0;
	hr = pDU->Acquire_Singles_Data(View, Config, m_HeadSelect, SourceSelect, LayerSelect, AcqType, Amount, lld, uld, &Status);
	if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAcquireMasterTubeEnergy", "Method Failed (pDU->Acquire_Singles_Data), HRESULT", hr);

	// if error starting acquisition, inform user and exit
	if (Status != 0) {
		MessageBox("Error Starting Acquisition", "Error", MB_OK);
		return;
	}

	// Enable the abort button
	m_AbortButton.EnableWindow(TRUE);

	// set progress bar to 0%
	m_AcquirePercent = 0;
	m_AcquireProgress.SetPos(m_AcquirePercent);

	// launch progress thread
	m_AcquireMaster = 1;
	hProgressThread = (HANDLE) _beginthread(ProgressThread, 0, NULL);

	// if failed to start thread
	if (hProgressThread == NULL) {

		// inform user
		MessageBox("Failed To Start Progess Thread", "Error", MB_OK);

		// Re-enable controls
		Enable_Controls();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSetEnergyRange
// Purpose:		Menu File->Set Energy Range - brings up the Energy Range Dialog 
//				(EnergyRange.cpp) where the user can change the lld and uld for 
//				the current session. 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSetEnergyRange() 
{
	// local variables
	long Config = 0;

	EnergyRange pRngDlg;

	// get the configuration
	Config = m_ConfigDrop.GetCurSel();

	// get the seed view
	pRngDlg.RangeView = m_ViewIndex[m_ViewDrop.GetCurSel()];
	if (pRngDlg.RangeView == DHI_MODE_TIME) pRngDlg.RangeView = DHI_MODE_RUN;

	// if the view is not one of the acquirable data types, then set to position profile
	if ((pRngDlg.RangeView != DHI_MODE_POSITION_PROFILE) &&
		(pRngDlg.RangeView != DHI_MODE_CRYSTAL_ENERGY) &&
		(pRngDlg.RangeView != DHI_MODE_TUBE_ENERGY) &&
		(pRngDlg.RangeView != DHI_MODE_SHAPE_DISCRIMINATION) &&
		(pRngDlg.RangeView != DHI_MODE_RUN)) pRngDlg.RangeView = DHI_MODE_POSITION_PROFILE;

	// transfer values
	pRngDlg.Accept = 0;
	pRngDlg.RangePP_lld = m_PositionProfileLLD;
	pRngDlg.RangePP_uld = m_PositionProfileULD;
	pRngDlg.RangeEN_lld = m_CrystalEnergyLLD;
	pRngDlg.RangeEN_uld = m_CrystalEnergyULD;
	pRngDlg.RangeTE_lld = m_TubeEnergyLLD;
	pRngDlg.RangeTE_uld = m_TubeEnergyULD;
	pRngDlg.RangeSD_lld = m_ShapeLLD;
	pRngDlg.RangeSD_uld = m_ShapeULD;
	pRngDlg.RangeRun_lld = m_ConfigurationLLD[Config];
	pRngDlg.RangeRun_uld = m_ConfigurationULD[Config];
	pRngDlg.ScannerType = m_ScannerType;

	// put up dialog
	pRngDlg.DoModal();

	// transfer values back
	if (pRngDlg.Accept == 1) {
		m_PositionProfileLLD = pRngDlg.RangePP_lld;
		m_PositionProfileULD = pRngDlg.RangePP_uld;
		m_CrystalEnergyLLD = pRngDlg.RangeEN_lld;
		m_CrystalEnergyULD = pRngDlg.RangeEN_uld;
		m_TubeEnergyLLD = pRngDlg.RangeTE_lld;
		m_TubeEnergyULD = pRngDlg.RangeTE_uld;
		m_ShapeLLD = pRngDlg.RangeSD_lld;
		m_ShapeULD = pRngDlg.RangeSD_uld;
		m_ConfigurationLLD[Config] = pRngDlg.RangeRun_lld;
		m_ConfigurationULD[Config] = pRngDlg.RangeRun_uld;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnFileArchive
// Purpose:		Menu File->Archive - brings up the Archiving Dialog 
//				(LoadDlg.cpp) where the user can save the current setup
//				or restore a previous one from archive 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnFileArchive() 
{
	// declare gui
	CLoadDlg pDlg;

	// call gui
	pDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDownloadcrystalenergy
// Purpose:		Menu File->Download Crystal Energy - sends the current crystal
//				energy peaks data to the selected head(s) 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnFileDownloadcrystalenergy() 
{
	// local variables
	long i = 0;
	long index = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long NumDownload = 0;
	long Status = 0;
	long DataLayer = 0;

	char Str[MAX_STR_LEN];

	HeadSelection pSelectDlg;

	HRESULT hr = S_OK;

	// get the configuration from the main GUI
	Config = m_ConfigDrop.GetCurSel();

	// initialize head selection
	for (i = 0 ; i < MAX_HEADS ; i++) m_HeadSelect[i] = 0;

	// transfer information
	pSelectDlg.ScannerType = m_ScannerType;
	pSelectDlg.NumHeads = m_NumHeads;
	for (i = 0 ; i < m_NumHeads ; i++) {
		pSelectDlg.HeadIndex[i] = m_HeadIndex[i];
		pSelectDlg.HeadAllow[i] = m_HeadComm[m_HeadIndex[i]];
	}

	// put up selection dialog
	pSelectDlg.DoModal();

	// disable the controls
	Disable_Controls();

	// loop through the heads
	for (i = 0 ; i < MAX_HEADS ; i++) {

		// if selected
		if (pSelectDlg.Selection[i] != 0) {

			// get head address
			Head = m_HeadIndex[i];

			// download fast layer if not petspect in spect mode
			if ((m_ScannerType != SCANNER_PETSPECT) || (Config == 0)) {

				// determine data layer
				DataLayer = LAYER_FAST;
				if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

				// store and download
				index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2);
				hr = pDU->Store_Energy_Peaks(SEND, Config, Head, DataLayer, &m_Peaks[index], &Status);

				// if peaks could not be downloaded
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnFileDownloadcrystalenergy", "Method Failed (pDU->Store_Energy_Peaks), HRESULT", hr);
				if (Status != 0) {
					sprintf(Str, "Could Not Download Fast Crystal Peaks Head: %d", Head);
					MessageBox(Str, "Error", MB_OK);
					Enable_Controls();
					return;
				}

				// clear fast crystal peaks state change flag for each block
				index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS);
				for (Block = 0 ; Block < m_NumBlocks ; Block++) {
					m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_FAST_ENERGY_CHANGED;
				}
			}

			// download slow layer if present
			if (m_NumLayers[Head] == 2) {

				// store and download
				index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2) + MAX_HEAD_CRYSTALS;
				hr = pDU->Store_Energy_Peaks(SEND, Config, Head, LAYER_SLOW, &m_Peaks[index], &Status);

				// if peaks could not be downloaded
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnFileDownloadcrystalenergy", "Method Failed (pDU->Store_Energy_Peaks), HRESULT", hr);
				if (Status != 0) {
					sprintf(Str, "Could Not Download Slow Crystal Peaks Head: %d", Head);
					MessageBox(Str, "Error", MB_OK);
					Enable_Controls();
					return;
				}

				// clear slow crystal peaks state change flag for each block
				index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS);
				for (Block = 0 ; Block < m_NumBlocks ; Block++) {
					m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_SLOW_ENERGY_CHANGED;
				}
			}
		}
	}

	// re-enable controls
	Enable_Controls();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusBlock
// Purpose:		causes the block number to be updated when focus is removed 
//				from the block number text field 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnKillfocusBlock() 
{
	// local variables
	long Value = 0;

	char Str[MAX_STR_LEN];

	// retrieve
	m_Block.GetWindowText(Str, 10);

	// if it is not in the valid range, set to previous value
	if (Validate(Str, 0, m_NumBlocks-1, &Value) != 0) Value = m_CurrBlock;

	// convert back
	sprintf(Str, "%d", Value);

	// insert back
	m_Block.SetWindowText(Str);

	// if a new value
	if (Value != m_CurrBlock) {

		// get the new value
		m_CurrBlock = Value;

		// refresh block
		UpdateBlock();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnAnalyze
// Purpose:		not yet defined
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnAnalyze() 
{
	// TODO: Add your control notification handler code here
	
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSave
// Purpose:		Saves the setup data for the current head in a generic format 
//				when "Save" button is pressed
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnSave() 
{
	// local variables
	long Config = 0;
	long Head = 0;
	long Size = 0;
	long Status = 0;

	char filename[MAX_STR_LEN];

	unsigned char *buffer;

	FILE *fp = NULL;

	HRESULT hr = S_OK;

	CFileDialog DhiFileDlg(FALSE, "har", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "*.har");

	// break out dialog information
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Config = m_ConfigDrop.GetCurSel();

	// get the file name
	if (DhiFileDlg.DoModal() != IDOK) return;

	// local copy of filename
	sprintf(filename, DhiFileDlg.GetPathName());

	// disable controls
	Disable_Controls();

	// save the head information
	hr = pDU->Save_Head(Config, Head, &Size, &buffer, &Status);

	// if succeeded
	if ((hr == S_OK) && (Status == 0)) {

		// open the file
		if ((fp = fopen(filename, "wb")) != NULL) {

			// write file
			if (Size != (long) fwrite((void *) buffer, sizeof(unsigned char), Size, fp)) MessageBox("Error Writing File", "Error", MB_OK);

			// close file
			fclose(fp);

		} else {
			MessageBox("Error Opening File", "Error", MB_OK);
		}

		// release the memory
		CoTaskMemFree(buffer);

	// failed in some way
	} else {
		MessageBox("Failed to Generate Archive From Head", "Error", MB_OK);
	}

	// re-enable controls
	Enable_Controls();

	return;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDefaults
// Purpose:		resets current head to default settings when "Defaults" button
//				is pressed 
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnDefaults() 
{
	// local variables
	long i = 0;
	long index = 0;
	long Config = 0;
	long Head = 0;
	long Block = 0;
	long Crystal = 0;
	long X = 0;
	long Y = 0;
	long Status = 0;
	long DataLayer = 0;

	HRESULT hr = S_OK;

	// get confirmation
	if (MessageBox("Replace Current Settings With Defaults?", "Confirmation", MB_OKCANCEL) == IDOK) {

		// disable controls
		Disable_Controls();

		// break out dialog information
		Config = m_ConfigDrop.GetCurSel();
		Head = m_HeadIndex[m_HeadDrop.GetCurSel()];

		hr = pDHI->Zap(ZAP_ALL, Config, Head, ALL_BLOCKS, &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDefaults", "Method Failed (pDHI->Zap), HRESULT", hr);
		if (Status != 0) {
			MessageBox("Error Zapping Head", "Error", MB_OK);
			Enable_Controls();
			return;
		}

		// retrieve new settings
		index = ((Config * MAX_HEADS) + Head) * MAX_BLOCKS * MAX_SETTINGS;
		hr = pDHI->Get_Analog_Settings(Config, Head, &m_Settings[index], &Status);
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDefaults", "Method Failed (pDHI->Get_Analog_Settings), HRESULT", hr);
		if (Status != 0) {
			MessageBox("Error Retrieving New Settings", "Error", MB_OK);
			Enable_Controls();
			return;
		}

		// clear time offsets and crystal peaks
		index = (Config * MAX_HEADS * MAX_HEAD_CRYSTALS * 2) + (Head * MAX_HEAD_CRYSTALS * 2);
		for (i = 0 ; i < (MAX_HEAD_CRYSTALS * 2) ; i++) {

			// time offsets
			m_Time[index+i] = 0;

			// break out block and crystal informaion
			Block = (i / MAX_CRYSTALS) % MAX_BLOCKS;
			Crystal = i % 256;
			X = Crystal % 16;
			Y = Crystal / 16;

			// if it is an emission crystal, set it to 190
			if ((Block < (m_XBlocks * m_YBlocks)) && (X < m_XCrystals) && (Y < m_YCrystals)) m_Peaks[index+i] = 190;
		}

		// store fast values if not PETSPECT SPECT mode
		if ((m_ScannerType != SCANNER_PETSPECT) || (Config == 0)) {

			// set data layer
			DataLayer = LAYER_FAST;
			if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

			// crystal peaks
			hr = pDU->Store_Energy_Peaks(DONT_SEND, Config, Head, DataLayer, &m_Peaks[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDefaults", "Method Failed (pDU->Store_Energy_Peaks), HRESULT", hr);
			if (Status != 0) {
				MessageBox("Error Storing Crystal Peaks To File", "Error", MB_OK);
				Enable_Controls();
				return;
			}

			// crystal time offsets - not for HRRT
			if (m_ScannerType != SCANNER_HRRT) {

				// determine where it is to be stored
				DataLayer = LAYER_FAST;
				if (m_ScannerType == SCANNER_P39_IIA) DataLayer = LAYER_ALL;

				// store values
				hr = pDU->Store_Crystal_Time(DONT_SEND, Config, Head, DataLayer, &m_Time[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDefaults", "Method Failed (pDU->Store_Crystal_Time), HRESULT", hr);
				if (Status != 0) {
					MessageBox("Error Storing Crystal Time Offsets To File", "Error", MB_OK);
					Enable_Controls();
					return;
				}
			}
		}

		// store slow values if dual layer head
		index += MAX_HEAD_CRYSTALS;
		if (m_NumLayers[Head] == 2) {

			// crystal peaks
			hr = pDU->Store_Energy_Peaks(DONT_SEND, Config, Head, LAYER_SLOW, &m_Peaks[index], &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDefaults", "Method Failed (pDU->Store_Energy_Peaks), HRESULT", hr);
			if (Status != 0) {
				MessageBox("Error Storing Crystal Peaks To File", "Error", MB_OK);
				Enable_Controls();
				return;
			}

			// crystal time offsets - not for HRRT
			if (m_ScannerType != SCANNER_HRRT) {
				hr = pDU->Store_Crystal_Time(DONT_SEND, Config, Head, LAYER_SLOW, &m_Time[index], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnDefaults", "Method Failed (pDU->Store_Crystal_Time), HRESULT", hr);
				if (Status != 0) {
					MessageBox("Error Storing Crystal Time Offsets To File", "Error", MB_OK);
					Enable_Controls();
					return;
				}
			}
		}

		// set states as unchanged
		index = (Config * MAX_TOTAL_BLOCKS) + (Head * MAX_BLOCKS);
		for (Block = 0 ; Block < m_NumBlocks ; Block++) {
			m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_FAST_ENERGY_CHANGED;
			m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_SLOW_ENERGY_CHANGED;
			m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_FAST_TIME_CHANGED;
			m_BlockState[index+Block] -= m_BlockState[index+Block] & STATE_SLOW_TIME_CHANGED;
		}
	}

	// redisplay
	UpdateHead();

	// re-enable controls
	Enable_Controls();	
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnFileDiagnostics
// Purpose:		menu File->Diagnostics - resets the selected head(s) and then
//				uploads the startup.rpt file and checks for failures
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnFileDiagnostics() 
{
	// local variables
	long i = 0;
	long NumHeads = 0;
	long Head = 0;
	long Percent = 0;
	long Size = 0;
	long BytesWritten = 0;
	long Found = 0;
	long Status = 0;

	unsigned long HeadSum = 0;
	unsigned long LocalSum = 0;

	char Str[MAX_STR_LEN];
	char Filename[MAX_STR_LEN];

	FILE *fp = NULL;

	unsigned char *buffer;

	HeadSelection pSelectDlg;

	HRESULT hr = S_OK;

	// transfer information
	pSelectDlg.ScannerType = m_ScannerType;
	pSelectDlg.NumHeads = m_NumHeads;
	for (i = 0 ; i < m_NumHeads ; i++) {
		pSelectDlg.HeadIndex[i] = m_HeadIndex[i];
		pSelectDlg.HeadAllow[i] = m_HeadComm[m_HeadIndex[i]];
	}

	// put up selection dialog
	pSelectDlg.DoModal();

	// disable the controls
	Disable_Controls();

	// loop through the heads issuing reset commands
	for (i = 0 ; i < MAX_HEADS ; i++) {

		// if selected
		if (pSelectDlg.Selection[i] != 0) {

			// count the number of heads selected
			NumHeads++;

			// get head address
			Head = m_HeadIndex[i];

			// issue the reset command
			hr = pDHI->Reset_Head(Head, &Status);
			if (hr != S_OK) {
				if (FAILED(hr)) {
					Add_Error(pErrEvtSup, "OnFileDiagnostics", "Method Failed (pDHI->Reset_Head), HRESULT", hr);
					sprintf(Str, "Communications Failure Resetting Head %d", Head);
				} else {
					sprintf(Str, "Error Resetting Head %d", Head);
				}
				MessageBox(Str, "Error", MB_OK);
				Enable_Controls();
				return;
			}
		}
	}

	// if no heads were selected, then finished
	if (NumHeads == 0) {
		Enable_Controls();
		return;
	}

	// the heads need to be given some time before starting to poll for progress
	Sleep(m_ResetWait);

	// in case there is an error on each head, run through the progress polling once for each head
	for (Head = 0 ; Head < NumHeads ; Head++) {

		// reset status for each loop
		Status = 0;

		// poll for progress
		for (Percent = 0 ; (Percent < 100) && (Status == 0) ; ) {
			hr = pDHI->Progress(WAIT_ALL_HEADS, &Percent, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnFileDiagnostics", "Method Failed (pDHI->Progress), HRESULT", hr);
			if ((Percent < 100) && (Status == 0)) Sleep(1000);
		}
	}

	// loop through the heads again
	for (i = 0 ; i < MAX_HEADS ; i++) {

		// if selected
		if (pSelectDlg.Selection[i] != 0) {

			// get head address
			Head = m_HeadIndex[i];

			// upload the startup report file
			sprintf(Filename, "startup.rpt");
			hr = pDHI->File_Upload(Head, (unsigned char *) Filename, &Size, &HeadSum, &buffer, &Status);
			if (hr != S_OK) {
				if (FAILED(hr)) {
					Add_Error(pErrEvtSup, "OnFileDiagnostics", "Method Failed (pDHI->File_Upload), HRESULT", hr);
					sprintf(Str, "Communications Failure Uploading Report - Head %d", Head);
				} else {
					sprintf(Str, "Error Uploading Report - Head %d", Head);
				}
				MessageBox(Str, "Error", MB_OK);
				Enable_Controls();
				return;
			}

			// checksum
			for (LocalSum = 0, i = 0 ; i < Size ; i++) LocalSum += (unsigned long) buffer[i];
			if (HeadSum != LocalSum) {
				sprintf(Str, "CheckSum Did Not Match - Head %d", Head);
				MessageBox(Str, "Error", MB_OK);
			}

			// parse through file
			for (Found = 0, i = 0 ; (i <= (Size-6)) && (Found == 0) ; i++) {

				// look for "f a i l e d" in any combination of upper and lower case
				if (((char) buffer[i+0] != 'F') && ((char) buffer[i+0] != 'f')) continue;
				if (((char) buffer[i+1] != 'A') && ((char) buffer[i+1] != 'a')) continue;
				if (((char) buffer[i+2] != 'I') && ((char) buffer[i+2] != 'i')) continue;
				if (((char) buffer[i+3] != 'L') && ((char) buffer[i+3] != 'l')) continue;
				if (((char) buffer[i+4] != 'E') && ((char) buffer[i+4] != 'e')) continue;
				if (((char) buffer[i+5] != 'D') && ((char) buffer[i+5] != 'd')) continue;

				// if it made it passed the checks, then "failed" was found
				Found = 1;
			}

			// build string
			if (Found == 1) {
				sprintf(Str, "Head %d: FAILED - Save File?", Head);
			} else {
				sprintf(Str, "Head %d: PASSED - Save File?", Head);
			}

			// let user choose if the file is to be saved
			if (MessageBox(Str, "Save File?", MB_OKCANCEL) != IDOK) continue;

			// user selects file name or cancels
			sprintf(Filename, "h%d_diagnostics.txt", Head);
			CFileDialog FileDlg(FALSE, "txt", Filename, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "*.txt");
			if (FileDlg.DoModal() == IDOK) {

				// local copy of filename
				sprintf(Filename, FileDlg.GetPathName());

				// open the file
				if ((fp = fopen(Filename, "wb")) != NULL) {

					// write file
					BytesWritten = (long) fwrite((void *) buffer, sizeof(unsigned char), Size, fp);

					// close file
					fclose(fp);

					// if all bytes not written
					if (BytesWritten != Size) {
						MessageBox("Error Writing File", "Error", MB_OK);
						Enable_Controls();
						return;
					}

				} else {
					MessageBox("Error Opening File", "Error", MB_OK);
					Enable_Controls();
					return;
				}
			}
		}
	}

	// Re-enable controls
	Enable_Controls();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Add_Error
// Purpose:		send an error message to the event handler
//				put it up in a message box
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
//				*Where		-	char = what subroutine did error occur in
//				*What		-	char = what was the error
//				*Why		-	long = error specific integer
// Returns:		long specific error code
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(CErrorEventSupport* pErrSup, char *Where, char *What, long Why)
{
	// local variable
	long error_out = -1;
	long SaveState = 0;
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_DUG";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

#ifndef ECAT_SCANNER
	// fire message to event handler
	if (pErrEvtSup != NULL) hr = pErrEvtSup->Fire(GroupName, DUG_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);
#endif

	// return error code
	return error_out;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_DAL_Ptr
// Purpose:		retrieve a pointer to the Detector Abstraction Layer COM server (DHICOM)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		IAbstract_DHI* pointer to DHICOM
// History:		16 Sep 03	T Gremillion	History Started
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
// History:		16 Sep 03	T Gremillion	History Started
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

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Lock_Ptr
// Purpose:		retrieve a pointer to the Lock Manager COM server (LockMgr)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		ILockClass* pointer to LockMgr
// History:		16 Sep 03	T Gremillion	History Started
//				20 Nov 03	T Gremillion	Move compiler directive
/////////////////////////////////////////////////////////////////////////////

#ifndef ECAT_SCANNER
ILockClass* Get_Lock_Ptr(CErrorEventSupport *pErrSup)
{
	// local variables
	ILockClass *Ptr = NULL;

	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	
	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// get server
	if (getenv("LOCKMGR_COMPUTER") == NULL) {
		sprintf(Server, "P39ACS");
	} else {
		sprintf(Server, "%s", getenv("LOCKMGR_COMPUTER"));
	}
	for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
	si.pwszName = Server_Name;

	// identify interface
	iid = IID_ILockClass;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	// get instance
	hr = ::CoCreateInstanceEx(CLSID_LockClass, NULL, CLSCTX_SERVER, &si, 1, mqi);
	if (FAILED(hr)) Add_Error(pErrSup, "Get_Lock_Ptr", "Failed ::CoCreateInstanceEx(CLSID_LockClass, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (ILockClass*) mqi[0].pItf;

	return Ptr;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Routine:		Build_Layer_List
// Purpose:		build a list of crystal layers applicable to current data type
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::Build_Layer_List()
{
	// local variable
	long View = 0;
	long Layer = 0;
	long Head = 0;
	long Num = 0;
	long Curr = 0;
	long Found = 0;

	// drop list values
	View = m_ViewIndex[m_ViewDrop.GetCurSel()];
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Layer = m_LayerIndex[m_LayerDrop.GetCurSel()];

	// Clear the list
	while (m_LayerDrop.DeleteString(0) != -1);

	// shape discrimination, only combined layer
	if ((View == DHI_MODE_SHAPE_DISCRIMINATION) || ((View == DATA_MODE_TIMING) && (SCANNER_HRRT == m_ScannerType))) {
		m_LayerDrop.InsertString(0, "Combined Layers");
		m_LayerIndex[0] = LAYER_ALL;
		Num = 1;

	// no combined data for crystal peaks or crystal time offsets for combined data on dual layer heads
	} else if (((DATA_CRYSTAL_PEAKS == View) || (DATA_TIME_OFFSETS == View)) && (m_NumLayers[Head] == 2)) {
		m_LayerDrop.InsertString(0, "Fast Layer");
		m_LayerIndex[0] = LAYER_FAST;
		m_LayerDrop.InsertString(1, "Slow Layer");
		m_LayerIndex[1] = LAYER_SLOW;
		Num = 2;

	// otherwise, full selection
	} else {
		m_LayerDrop.InsertString(0, "Fast Layer");
		m_LayerIndex[0] = LAYER_FAST;
		m_LayerDrop.InsertString(1, "Slow Layer");
		m_LayerIndex[1] = LAYER_SLOW;
		m_LayerDrop.InsertString(2, "Combined Layers");
		m_LayerIndex[2] = LAYER_ALL;
		Num = 3;
	}

	// set the current layer
	for (Curr = 0, Found = 0 ; (Curr < Num) && (Found == 0) ; Curr++) {
		if (Layer == m_LayerIndex[Curr]) {
			m_LayerDrop.SetCurSel(Curr);
			Found = 1;
		}
	}

	// if the previous layer isn't available, go with the first choice
	if (Found == 0) {
		m_LayerDrop.SetCurSel(0);
		m_CurrLayer = m_LayerIndex[0];
	}

}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDnlCheck
// Purpose:		causes the display to be refreshed when the state of the 
//				DNL check box changes.
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDUGUIDlg::OnDnlCheck() 
{
	// Update the block
	UpdateBlock();
	
}
