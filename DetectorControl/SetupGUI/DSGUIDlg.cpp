// DSGUIDlg.cpp : implementation file
//
#include "stdafx.h"
#include "DSGUI.h"
#include "DSGUIDlg.h"
#include "LockMgr.h"
#include "LockMgr_i.c"
#include "DSCom.h"
#include "DSCom_i.c"
#include "LoadDlg.h"

#include "gantryModelClass.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_USER_POST_PROGRESS		(WM_USER+1)
#define WM_USER_PROCESSING_COMPLETE	(WM_USER+2)
#define WM_USER_CONTINUE_STATE		(WM_USER+3)

static long m_BlockSelect[MAX_TOTAL_BLOCKS];
static long m_BlockStatus[MAX_TOTAL_BLOCKS];
static long m_HeadPresent[MAX_HEADS];
static long m_PointSource[MAX_HEADS];
static long m_TransmissionStart[MAX_HEADS];
static long m_HeadIndex[MAX_HEADS];
static long m_DropIndex[MAX_HEADS];
static long m_HeadSelect[MAX_HEADS];
static long m_TypeIndex[NUM_SETUP_TYPES];
static long m_NumHeads;
static long m_NumBlocks;
static long m_Transmission;
static long m_Emission_X;
static long m_Emission_Y;
static long m_Emission;
static long m_CurrHead;
static long m_CurrType;
static long m_CurrConfig;
static long m_CurrEmTx;
static long m_NumConfigurations;
static long m_Mode;
static long m_Abort;
static long m_Continue;
static long m_ContinueState;
static long m_TotalPercent;
static long m_StagePercent;
static long m_ScannerType;
static long m_ModelNumber;
static long m_Pair_393_1;
static long m_Pair_393_2;
static long m_EmPair;
static long m_TxPair;
static long m_DelayTime;
static unsigned char m_StageStr[MAX_STR_LEN];

static char* BitField[15] = {"Initializing", "Determining Offset", "Coarse Gain", "Shape Discrimination", 
							 "Fine Gain", "Generating CRM", "Determining Crystal Energy Peaks", "Determining CFD Setting",
							 "Tuning Gains", "Transmission Gain", "Transmission CFD", "Time Alignment",
							 "Generating Run Mode Image", "Expanding CRM","Abort"};

static HANDLE m_MyWin;

static __int64 m_LockCode;

static ILockClass* pLock;

IDSMain* Get_DS_Ptr(CErrorEventSupport *pErrSup);
ILockClass* Get_Lock_Ptr(CErrorEventSupport *pErrSup);

CErrorEventSupport* pErrEvtSup;
CErrorEventSupport* pErrEvtSupThread;

// thread subroutines
void SetupThread(void *dummy);

// colors
#define BLACK  0x00010101
#define GREY   0x00D0D0D0
#define WHITE  0x00FFFFFF
#define RED    0x000000FF
#define GREEN  0x0000FF00
#define YELLOW 0x0000FFFF

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDSGUIDlg dialog

CDSGUIDlg::CDSGUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDSGUIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDSGUIDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDSGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDSGUIDlg)
	DDX_Control(pDX, IDC_DELAY_SPIN, m_DelaySpin);
	DDX_Control(pDX, IDC_DELAY_FIELD, m_DelayField);
	DDX_Control(pDX, IDCONTINUEBUTTON, m_ContinueButton);
	DDX_Control(pDX, IDC_STATUS3, m_Status3);
	DDX_Control(pDX, IDC_STATUS2, m_Status2);
	DDX_Control(pDX, IDC_STATUS1, m_Status1);
	DDX_Control(pDX, IDARCHIVE, m_ArchiveButton);
	DDX_Control(pDX, IDABORT, m_AbortButton);
	DDX_Control(pDX, IDCLEAR, m_ClearButton);
	DDX_Control(pDX, IDEXIT, m_ExitButton);
	DDX_Control(pDX, IDRESET, m_ResetButton);
	DDX_Control(pDX, IDSET, m_SetButton);
	DDX_Control(pDX, IDSTART, m_StartButton);
	DDX_Control(pDX, IDC_STAGE_LABEL, m_StageLabel);
	DDX_Control(pDX, IDC_TOTAL_PROGRESS, m_TotalProgress);
	DDX_Control(pDX, IDC_STAGE_PROGRESS, m_StageProgress);
	DDX_Control(pDX, IDC_DROPTYPE, m_DropType);
	DDX_Control(pDX, IDC_DROP_EMTX, m_DropEmTx);
	DDX_Control(pDX, IDC_DROP_CONFIG, m_DropConfig);
	DDX_Control(pDX, IDC_DROPHEAD, m_DropHead);
	DDX_Control(pDX, IDC_HEADLIST, m_HeadList);
	DDX_Control(pDX, IDC_HEADGRID, m_HeadGrid);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDSGUIDlg, CDialog)
	//{{AFX_MSG_MAP(CDSGUIDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDEXIT, OnExit)
	ON_LBN_SELCHANGE(IDC_HEADLIST, OnSelchangeHeadlist)
	ON_CBN_SELCHANGE(IDC_DROPHEAD, OnSelchangeDrophead)
	ON_BN_CLICKED(IDCLEAR, OnClear)
	ON_BN_CLICKED(IDSET, OnSet)
	ON_CBN_SELCHANGE(IDC_DROPTYPE, OnSelchangeDroptype)
	ON_CBN_SELCHANGE(IDC_DROP_CONFIG, OnSelchangeDropConfig)
	ON_CBN_SELCHANGE(IDC_DROP_EMTX, OnSelchangeDropEmtx)
	ON_BN_CLICKED(IDSTART, OnStart)
	ON_BN_CLICKED(IDRESET, OnReset)
	ON_BN_CLICKED(IDABORT, OnAbort)
	ON_BN_CLICKED(IDARCHIVE, OnArchive)
	ON_BN_CLICKED(IDCONTINUEBUTTON, OnContinue)
	ON_WM_DESTROY()
	ON_NOTIFY(UDN_DELTAPOS, IDC_DELAY_SPIN, OnDeltaposDelaySpin)
	//}}AFX_MSG_MAP

	// User define message maps
	ON_MESSAGE(WM_USER_POST_PROGRESS, OnPostProgress)
	ON_MESSAGE(WM_USER_PROCESSING_COMPLETE, OnProcessingComplete)
	ON_MESSAGE(WM_USER_CONTINUE_STATE, OnStateChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDSGUIDlg message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		23 Sep 03	T Gremillion	History Started
//				15 Apr 04	T Gremillion	removed automatic selection of first head
/////////////////////////////////////////////////////////////////////////////

BOOL CDSGUIDlg::OnInitDialog()
{
	// local variables
	long i = 0;
	long j = 0;
	long Block = 0;
	long index = 0;
	long NumHeads = 0;

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// instantiate a Gantry Object Model object
	CGantryModel model;
	pHead Head;		
	pConf Config;

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport(false, false);

	// if error support not establisted, note it in log
	if (pErrEvtSup != NULL) if (pErrEvtSup->InitComplete(hr) == false) {
		delete pErrEvtSup;
		pErrEvtSup = NULL;
	}

	// get pointer to lock manager
	pLock = Get_Lock_Ptr(pErrEvtSup);
	if (pLock == NULL) {
		Add_Error(pErrEvtSup, "OnInitDialog", "Failed to Get Lock Pointer", 0);
		MessageBox("Could Not Find Lock Manager", "Error", MB_OK);
		PostMessage(WM_CLOSE);
	}

	// get lock code
	hr = pLock->LockACS(&m_LockCode, 0);
	if (hr != S_OK) {

		// message
		if (FAILED(hr)) {
			Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pLock->LockACS), HRESULT", hr);
			MessageBox("Method Failed:  pLock->LockACS", "Error", MB_OK);
		} else if (hr == S_FALSE) {
			MessageBox("Servers Already Locked", "Error", MB_OK);
		}

		// close gui
		PostMessage(WM_CLOSE);
	}

	// set gantry model to model number held in register if none held, use the 393
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) model.setModel(393);
	
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

	// P39 family Phase IIA Electronics (2391 - 2396)
	case 2391:
	case 2392:
	case 2393:
	case 2394:
	case 2395:
	case 2396:
	default:
		m_ScannerType = SCANNER_P39_IIA;
		break;
	}

	// module pairs
	m_EmPair = model.emModulePair();
	m_TxPair = model.txModulePair();
	m_Pair_393_1 = 0x20;
	m_Pair_393_2 = 0x80;

	// number of blocks
	m_NumBlocks = model.blocks();
	if (model.isHeadRotated()) {
		m_Emission_X = model.axialBlocks();
		m_Emission_Y = model.transBlocks();
	} else {
		m_Emission_X = model.transBlocks();
		m_Emission_Y = model.axialBlocks();
	}
	m_Emission = m_Emission_X * m_Emission_Y;

	// initialize Head arrays
	m_Transmission = 0;
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_HeadPresent[i] = 0;
		m_PointSource[i] = 0;
		m_TransmissionStart[i] = 0;
		m_HeadSelect[i] = 0;
		m_HeadIndex[i] = 0;
		m_DropIndex[i] = 0;
	}

	// get the data for the next head
	Head = model.headInfo();

	// set head values
	m_NumHeads = model.nHeads();
	for (i = 0 ; i < m_NumHeads ; i++) {

		// indicate that the head is present
		m_HeadPresent[Head[i].address] = 1;

		// if head has point sources hooked up to it, get the vales
		if (Head[i].pointSrcData) {

			// number of point sources
			m_PointSource[Head[i].address] = model.nPointSources(Head[i].address);

			// starting block
			m_TransmissionStart[Head[i].address] = model.pointSourceStart(Head[i].address);
		}

		// if transmission detectors are present than allow that type of detector setup
		if (Head[i].pointSrcData) m_Transmission = 1;
	}

	// set the grid up to use up the whole area
	for (i = 0 ; i < MAX_ROWS ; i++) m_HeadGrid.SetRowHeight(i, 500);
	for (i = 0 ; i < MAX_COLUMNS ; i++) m_HeadGrid.SetColWidth(i, 500);

	// label blocks
	for (Block = 0 ; Block < m_NumBlocks ; Block++) {
		m_HeadGrid.SetRow(Block/m_Emission_X);
		m_HeadGrid.SetCol(Block%m_Emission_X);
		sprintf(Str, "%d  ", Block);
		m_HeadGrid.SetText(Str);
	}

	// set up the list of selectable heads
	for (i = 0 ; i < MAX_HEADS ; i++) {

		// if the head is present, then add it
		if (m_HeadPresent[i] == 1) {
			sprintf(Str, "Head %d", i);
			m_HeadList.InsertString(NumHeads, Str);
			m_HeadIndex[NumHeads++] = i;
		}
	}

	// Fill in first item of droplist as selected
	sprintf(Str, "Head %d", m_HeadIndex[0]);
	m_DropHead.InsertString(0, Str);
	m_DropHead.SetCurSel(0);
	m_DropIndex[0] = m_HeadIndex[0];
	m_CurrHead = m_DropIndex[0];

	// Clear the blocks
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
		m_BlockSelect[i] = 0;
		m_BlockStatus[i] = 0;
	}

	// detector types
	m_DropEmTx.InsertString(0, "Emission");
	if (m_Transmission != 0) {
		m_DropEmTx.InsertString(1, "Transmission");
		m_DropEmTx.InsertString(2, "Emission/Transmission");
	}
	m_DropEmTx.SetCurSel(0);
	m_CurrEmTx = 0;

	// Configurations
	m_NumConfigurations = model.nConfig();
	Config = model.configInfo();
	for (i = 0 ; i < m_NumConfigurations ; i++) m_DropConfig.InsertString(i, Config[i].name);
	m_DropConfig.SetCurSel(0);
	m_CurrConfig = 0;

	// Setup Types
	m_DropType.InsertString(0, "Full Setup");
	m_TypeIndex[0] = SETUP_TYPE_FULL_EMISSION;
	m_DropType.InsertString(1, "Tune Gains Setup");
	m_TypeIndex[1] = SETUP_TYPE_TUNE_GAINS;
	m_DropType.InsertString(2, "Correction Setup");
	m_TypeIndex[2] = SETUP_TYPE_CORRECTIONS;
	m_DropType.InsertString(3, "Crystal Energy Setup");
	m_TypeIndex[3] = SETUP_TYPE_CRYSTAL_ENERGY;
	m_DropType.InsertString(4, "Time Alignment");
	m_TypeIndex[4] = SETUP_TYPE_TIME_ALIGNMENT;
	if (SCANNER_HRRT == m_ScannerType) {
		m_DropType.InsertString(5, "Electronic Calibration");
		m_TypeIndex[5] = SETUP_TYPE_ELECTRONIC_CALIBRATION;
		m_DropType.InsertString(6, "Shape Discrimination");
		m_TypeIndex[6] = SETUP_TYPE_SHAPE;
	}
	m_DropType.SetCurSel(0);
	m_CurrType = m_TypeIndex[0];

	// Delay Time
	m_DelayTime = 0;
	m_DelayField.SetWindowText("0 Hr 0 Min");

	// display mode is selection
	m_Mode = 0;

	// clear abort flag
	m_Abort = 0;

	// Continue, Reset & Abort Not Active Yet
	m_AbortButton.EnableWindow(FALSE);
	m_ContinueButton.EnableWindow(FALSE);
	m_ResetButton.EnableWindow(FALSE);

	// display blocks
	Redisplay_Blocks();

	// keep track of the window handle
	m_MyWin = m_hWnd;
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSysCommand
// Purpose:		Called when dialog is minimized or restored
// Arguments:	nID		-	UINT = type of system command requested
//				lParam	-	LPARAM = If a Control-menu command is chosen with the mouse, 
//									lParam contains the cursor coordinates
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPaint
// Purpose:		GUI Refresh
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnPaint() 
{
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
	}
	else
	{
		CDialog::OnPaint();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnQueryDragIcon
// Purpose:		The system calls this to obtain the cursor to display while the user drags
//				the minimized window.
// Arguments:	None
// Returns:		HCURSOR = cursor handle
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

HCURSOR CDSGUIDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BEGIN_EVENTSINK_MAP(CDSGUIDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CDSGUIDlg)
	ON_EVENT(CDSGUIDlg, IDC_HEADGRID, -600 /* Click */, OnClickHeadgrid, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnClickHeadgrid
// Purpose:		This routine controls the selection of blocks within the flexgrid
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnClickHeadgrid() 
{
	// local variables
	long i = 0;
	long Row = 0;
	long Col = 0;
	long Block = 0;
	long index = 0;
	long masked = 0;
	long Next = 0;
	long local = 0;

	long Valid[MAX_ROWS*MAX_COLUMNS];

	char Str[3*MAX_STR_LEN];

	// determine valid positions
	for (i = 0 ; i < (MAX_ROWS * MAX_COLUMNS) ; i++) Valid[i] = 0;
	if ((m_CurrEmTx % 2) == 0) for (i = 0 ; i < m_Emission ; i++) Valid[i] = 1;
	if (m_CurrEmTx > 0) for (i = 0 ; i < m_PointSource[m_CurrHead] ;  i++) Valid[m_TransmissionStart[m_CurrHead]+i] = 1;
	

	// Get Location
	Row = m_HeadGrid.GetRow();
	Col = m_HeadGrid.GetCol();
	Block = (Row * m_Emission_X) + Col;

	// if valid location
	if ((Block < m_NumBlocks) && (Col < m_Emission_X) && (Valid[Block] == 1)) {

		// calculate block number and index
		index = (m_CurrHead * MAX_BLOCKS) + Block;

		// if selection mode, switch state
		if (m_Mode == 0) {

			// switch state
			if (m_CurrType != SETUP_TYPE_TIME_ALIGNMENT) m_BlockSelect[index] = abs(m_BlockSelect[index] - 1);

			// switch color according to state
			if (m_BlockSelect[index] == 0) {
				m_HeadGrid.SetCellBackColor(BLACK);
			} else {
				m_HeadGrid.SetCellBackColor(WHITE);
			}

		// otherwise, status mode
		} else {

			// Clear Status label values
			sprintf(&Str[0*MAX_STR_LEN], "");
			sprintf(&Str[1*MAX_STR_LEN], "");
			sprintf(&Str[2*MAX_STR_LEN], "");

			// start off identifying head and block
			sprintf(Str, "Head %d Block %d: ", m_CurrHead, Block);

			// if successfull
			if (m_BlockStatus[index] == 0) {

				// add to string and display in status label 1
				sprintf(Str, "%s Success", Str);

			// display errors and warnings
			} else {

				// local variable
				local = m_BlockStatus[index];

				// loop through fatal errors
				for (Next = 0, i = 1 ; i < 16 ; i++) {

					// if the shifted data shows fatal
					masked = (local >> (2 * i)) & BLOCK_FATAL;
					if (masked == BLOCK_FATAL) {
						if (Next < 3) sprintf(&Str[Next*MAX_STR_LEN], "%s Error - %s", Str, BitField[i-1]);
						Next++;
					}
				}

				// loop through warnings
				for (i = 1 ; i < 16 ; i++) {

					// if the shifted data shows fatal
					masked = (local >> (2 * i)) & BLOCK_FATAL;
					if (masked == BLOCK_NOT_FATAL) {
						switch (Next) {

						// first warning, no errors
						case 0:
							sprintf(&Str[Next*MAX_STR_LEN], "%s Warning - %s", Str, BitField[i-1]);
							break;

						// first warning, no errors
						case 1:
						case 2:
							sprintf(&Str[Next*MAX_STR_LEN], "Warning - %s", BitField[i-1]);
							break;

						// beyond the third warning/error
						default:
							sprintf(&Str[2*MAX_STR_LEN], "Multiple Warnings - See Log File");
							break;
						}
						Next++;
					}
				}
			}

			// display the messages
			m_Status1.SetWindowText(&Str[0*MAX_STR_LEN]);
			m_Status2.SetWindowText(&Str[1*MAX_STR_LEN]);
			m_Status3.SetWindowText(&Str[2*MAX_STR_LEN]);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnExit
// Purpose:		This routine controls the sequence when the "Exit" button is pressed
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnExit() 
{
	// Exit
	OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeHeadlist
// Purpose:		as heads are selected for setup, this routine controls the
//				selection of the associated blocks.
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnSelchangeHeadlist() 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long State = 0;

	// if time alignment, keep the previous state
	if (m_CurrType == SETUP_TYPE_TIME_ALIGNMENT) {

		// set state
		for (i = 0 ; i < m_NumHeads ; i++) {
			if (m_HeadSelect[m_HeadIndex[i]] == 1) m_HeadList.SetSel(i, TRUE);
		}

	// other types, turn on or off as directed
	} else {

		// check the current state
		for (i = 0 ; i < m_NumHeads ; i++) {

			// check each entry
			State = m_HeadList.GetSel(i);

			// if head has changed state
			if (State != m_HeadSelect[m_HeadIndex[i]]) {

				// switch state of head and blocks
				m_HeadSelect[m_HeadIndex[i]] = State;

				// start off by switching off all blocks for that head
				index = m_HeadIndex[i] * MAX_BLOCKS;
				for (j = 0 ; j < MAX_BLOCKS ; j++) m_BlockSelect[index+j] = 0;

				// it got switch on
				if (State == 1) {

					// make it the new current head
					m_CurrHead = m_HeadIndex[i];

					// if emission blocks selected switch them on
					if ((m_CurrEmTx % 2) == 0) {
						index = m_HeadIndex[i] * MAX_BLOCKS;
						for (j = 0 ; j < m_Emission ; j++) m_BlockSelect[index+j] = State;
					}

					// if transmission blocks selected switch them on
					if (m_CurrEmTx > 0) {
						index = (m_HeadIndex[i] * MAX_BLOCKS) + m_TransmissionStart[m_HeadIndex[i]];
						for (j = 0 ; j < m_PointSource[m_HeadIndex[i]] ; j++) m_BlockSelect[index+j] = State;
					}
				}
			}
		}
	}

	// Redisplay the block view
	Redisplay_Blocks();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Redisplay_Blocks
// Purpose:		This routine controls the colors of the blocks in the flexgrid
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::Redisplay_Blocks()
{
	// local variables
	long i = 0;
	long j = 0;
	long x1 = 0;
	long x2 = 0;
	long y1 = 0;
	long y2 = 0;
	long Block = 0;
	long index = 0;
	long Curr = -1;
	long Next = 0;
	long Status = 0;

	char Str[MAX_STR_LEN];

	// Clear the list
	while (m_DropHead.DeleteString(0) != -1);

	// rebuild the list
	for (i = 0 ; i < m_NumHeads ; i++) {

		// head is selected
		if (m_HeadSelect[m_HeadIndex[i]] == 1) {

			// if it the current head, then select it
			if (m_CurrHead == m_HeadIndex[i]) Curr = Next;

			// add to list
			m_DropIndex[Next] = m_HeadIndex[i];
			sprintf(Str, "Head %d", m_HeadIndex[i]);
			m_DropHead.InsertString(Next++, Str);
		}
	}

	// if current head is not found and at least one head present, select first head
	if ((Curr == -1) && (Next > 0)) {
		Curr = 0;
		m_CurrHead = m_DropIndex[Curr];
	}

	// show selection
	m_DropHead.SetCurSel(Curr);

	// no head selected, clear everything
	if (Curr == -1) {

		// clear whole block selection area
		for (j = 0 ; j < MAX_ROWS ; j++) for (i = 0 ; i < MAX_COLUMNS ; i++) {

			// set row and column
			m_HeadGrid.SetRow(j);
			m_HeadGrid.SetCol(i);

			// set color
			m_HeadGrid.SetCellBackColor(GREY);
		}

	// a head is selected
	} else {

		// start of block
		index = m_CurrHead * MAX_BLOCKS;

		// loop through blocks
		for (Block = 0 ; Block < m_NumBlocks ; Block++) {

			// break out row and column
			j = Block / m_Emission_X;
			i = Block % m_Emission_X;

			// set row and column
			m_HeadGrid.SetRow(j);
			m_HeadGrid.SetCol(i);

			// not selected - black
			if (m_BlockSelect[index+Block] == 0) {
				m_HeadGrid.SetCellBackColor(BLACK);

			// selected, depends on mode
			} else {

				// selection mode - white
				if (m_Mode == 0) {
					m_HeadGrid.SetCellBackColor(WHITE);

				// status mode depends on status
				} else {

					// break out overall status
					Status = m_BlockStatus[index+Block] & BLOCK_FATAL;

					switch (Status) {

					// no errors or warnings detected
					case 0:
						m_HeadGrid.SetCellBackColor(GREEN);
						break;

					// warnings detected
					case BLOCK_NOT_FATAL:
						m_HeadGrid.SetCellBackColor(YELLOW);
						break;

					// errors detected
					case BLOCK_FATAL:
						m_HeadGrid.SetCellBackColor(RED);
						break;

					// default case do nothing
					default:
						break;
					}
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeDrophead
// Purpose:		This routine controls what happens when a new head is 
//				selected for the block display (flexgrid)
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnSelchangeDrophead() 
{
	// change the current head
	m_CurrHead = m_DropIndex[m_DropHead.GetCurSel()];

	// switch the display
	Redisplay_Blocks();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnClear
// Purpose:		This routine controls what happens when a the "Clear" button
//				is pressed.  All blocks for the currently selected head are
//				deselected
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnClear() 
{
	// local variables
	long i = 0;
	long index = 0;

	// start of block
	index = m_CurrHead * MAX_BLOCKS;

	// clear all blocks
	if (m_CurrType != SETUP_TYPE_TIME_ALIGNMENT) {
		for (i = 0 ; i < MAX_BLOCKS ; i++) m_BlockSelect[index+i] = 0;
	}

	// switch the display
	Redisplay_Blocks();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSet
// Purpose:		This routine controls what happens when a the "Set" button
//				is pressed.  All blocks for the currently selected head are
//				selected
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnSet() 
{
	// local variables
	long i = 0;
	long index = 0;

	// start of block
	index = m_CurrHead * MAX_BLOCKS;

	// clear all blocks first
	for (i = 0 ; i < MAX_BLOCKS ; i++) m_BlockSelect[index+i] = 0;

	// if emission blocks selected switch them on
	if ((m_CurrEmTx % 2) == 0) for (i = 0 ; i < m_Emission ; i++) m_BlockSelect[index+i] = 1;

	// if transmission blocks selected switch them on
	if (m_CurrEmTx > 0) {
		index += m_TransmissionStart[m_CurrHead];
		for (i = 0 ; i < m_PointSource[m_CurrHead] ; i++) m_BlockSelect[index+i] = 1;
	}

	// switch the display
	Redisplay_Blocks();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeDroptype
// Purpose:		This routine controls what happens when a the selection in the
//				"Type" droplist is changed.  The restrictions imposed by the type
//				of setup selected are implemented.
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnSelchangeDroptype() 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;

	// Change the current selection
	m_CurrType = m_TypeIndex[m_DropType.GetCurSel()];

	// if time alignment, we need to turn everything on
	if (m_CurrType == SETUP_TYPE_TIME_ALIGNMENT) {

		// turn all blocks off initially
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) m_BlockSelect[i] = 0;

		// turn all heads on
		for (i = 0 ; i < m_NumHeads ; i++) {

			// start off turning head off
			m_HeadSelect[m_HeadIndex[i]] = 0;
			m_HeadList.SetSel(i, FALSE);

			// transmission only, only select if transmission is there
			if (m_CurrEmTx == 1) {

				// if point sources is there
				if (m_PointSource[m_HeadIndex[i]] > 0) {
					m_HeadSelect[m_HeadIndex[i]] = 1;
					m_HeadList.SetSel(i, TRUE);
				}

			// Emission only or Emission/Transmission do all heads
			} else {
				m_HeadSelect[m_HeadIndex[i]] = 1;
				m_HeadList.SetSel(i, TRUE);
			}

			// if new mode is emission or emission/transmission, turn on the emission blocks
			if ((m_CurrEmTx % 2) == 0) {
				index = m_HeadIndex[i] * MAX_BLOCKS;
				for (j = 0 ; j < m_Emission ; j++) m_BlockSelect[index+j] = 1;
			}

			// new mode is transmission or emission/transmission and transmission sources are present
			if ((m_CurrEmTx > 0) && (m_PointSource[m_HeadIndex[i]] > 0)) {
				index = (m_HeadIndex[i] * MAX_BLOCKS) + m_TransmissionStart[m_HeadIndex[i]];
				for (j = 0 ; j < m_PointSource[m_HeadIndex[i]] ; j++) m_BlockSelect[index+j] = 1;
			}
		}
	}

	// switch the display
	Redisplay_Blocks();

}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeDropConfig
// Purpose:		This routine controls what happens when a the selection in the
//				"Configuration" droplist is changed.  The current configuration 
//				is updated
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnSelchangeDropConfig() 
{
	// Change the current selection
	m_CurrConfig = m_DropConfig.GetCurSel();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeDropEmTx
// Purpose:		This routine controls what happens when a the selection in the
//				"Detector Type" droplist is changed.  The restrictions imposed by the type
//				of detectors are implemented.
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnSelchangeDropEmtx()
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long Old = 0;

	// store the old value
	Old = m_CurrEmTx;

	// Change the current selection
	m_CurrEmTx = m_DropEmTx.GetCurSel();

	// if it changed
	if (m_CurrEmTx != Old) {

		// first, turn off everything
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) m_BlockSelect[i] = 0;

		// if new mode is emission or emission/transmission
		if ((m_CurrEmTx % 2) == 0) {

			// loop through the heads
			for (i = 0 ; i < m_NumHeads ; i++) {

				// if time alignment, turn it on
				if (m_CurrType == SETUP_TYPE_TIME_ALIGNMENT) {
					m_HeadSelect[m_HeadIndex[i]] = 1;
					m_HeadList.SetSel(i, TRUE);
				}

				// if the head was already selected
				if (m_HeadSelect[m_HeadIndex[i]] == 1) {

					// turn all the emission blocks on
					index = m_HeadIndex[i] * MAX_BLOCKS;
					for (j = 0 ; j < m_Emission ; j++) m_BlockSelect[index+j] = 1;
				}
			}
		}

		// new mode is transmission or emission / transmission
		if (m_CurrEmTx > 0) {

			// loop through the heads
			for (i = 0 ; i < m_NumHeads ; i++) {

				// if the head was already selected
				if (m_HeadSelect[m_HeadIndex[i]] == 1) {

					// if the head has transmission sources
					if (m_PointSource[m_HeadIndex[i]] > 0) {

						// turn all the emission blocks on
						index = (m_HeadIndex[i] * MAX_BLOCKS) + m_TransmissionStart[m_HeadIndex[i]];
						for (j = 0 ; j < m_PointSource[m_HeadIndex[i]] ; j++) m_BlockSelect[index+j] = 1;

					// head does not have transmission, switch it off if transmission only mode
					} else if (m_CurrEmTx == 1) {

						// if it is the current head, then remove it
						if (m_CurrHead == m_HeadIndex[i]) m_CurrHead = -1;

						// de-select it
						m_HeadSelect[m_HeadIndex[i]] = 0;
						m_HeadList.SetSel(i, FALSE);
					}
				}
			}
		}

		// switch the display
		Redisplay_Blocks();

		// set the available types
		while (m_DropType.DeleteString(0) != -1);
		switch (m_CurrEmTx) {

		// Emission only
		case 0:
			m_DropType.InsertString(0, "Full Setup");
			m_TypeIndex[0] = SETUP_TYPE_FULL_EMISSION;
			m_DropType.InsertString(1, "Tune Gains Setup");
			m_TypeIndex[1] = SETUP_TYPE_TUNE_GAINS;
			m_DropType.InsertString(2, "Correction Setup");
			m_TypeIndex[2] = SETUP_TYPE_CORRECTIONS;
			m_DropType.InsertString(3, "Crystal Energy Setup");
			m_TypeIndex[3] = SETUP_TYPE_CRYSTAL_ENERGY;
			m_DropType.InsertString(4, "Time Alignment");
			m_TypeIndex[4] = SETUP_TYPE_TIME_ALIGNMENT;
			if (SCANNER_HRRT == m_ScannerType) {
				m_DropType.InsertString(5, "Electronic Calibration");
				m_TypeIndex[5] = SETUP_TYPE_ELECTRONIC_CALIBRATION;
				m_DropType.InsertString(6, "Shape Discrimination");
				m_TypeIndex[6] = SETUP_TYPE_SHAPE;
			}

			// set the the setup type
			if (m_CurrType == SETUP_TYPE_TIME_ALIGNMENT) {
				m_DropType.SetCurSel(4);
			} else {
				m_DropType.SetCurSel(0);
				m_CurrType = m_TypeIndex[0];
			}
			break;

		// transmission only
		case 1:
			m_DropType.InsertString(0, "Full Setup");
			m_TypeIndex[0] = SETUP_TYPE_TRANSMISSION_DETECTORS;
			m_DropType.InsertString(1, "Time Alignment");
			m_TypeIndex[1] = SETUP_TYPE_TIME_ALIGNMENT;

			// set the the setup type
			if (m_CurrType == SETUP_TYPE_TIME_ALIGNMENT) {
				m_DropType.SetCurSel(1);
			} else {
				m_DropType.SetCurSel(0);
				m_CurrType = m_TypeIndex[0];
			}
			break;

		// emission/transmission
		case 2:
		default:
			m_DropType.InsertString(0, "Full Setup");
			m_TypeIndex[0] = SETUP_TYPE_FULL;
			m_DropType.InsertString(1, "Time Alignment");
			m_TypeIndex[1] = SETUP_TYPE_TIME_ALIGNMENT;

			// set the the setup type
			if (m_CurrType == SETUP_TYPE_TIME_ALIGNMENT) {
				m_DropType.SetCurSel(1);
			} else {
				m_DropType.SetCurSel(0);
				m_CurrType = m_TypeIndex[0];
			}
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnStart
// Purpose:		This routine starts the setup thread
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnStart()
{
	// local variables
	long i = 0;

	HANDLE hSetupThread;

	// lock out controls except for abort button
	Control_State(FALSE);
	m_AbortButton.EnableWindow(TRUE);

	// change display mode
	m_Mode = 1;

	// start the processing thread
	m_StageLabel.SetWindowText("Start Setup");
	hSetupThread = (HANDLE) _beginthread(SetupThread, 0, NULL);

	// failed to start thread
	if (hSetupThread == NULL) {

		// fail all blocks
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) m_BlockStatus[i] = ABORT_FATAL;

		// display message
		m_StageLabel.SetWindowText("Failed To Start Setup");

		// switch the display
		Redisplay_Blocks();

		// re-enable exit, reset and head list
		m_ExitButton.EnableWindow(TRUE);
		m_ResetButton.EnableWindow(TRUE);
		m_DropHead.EnableWindow(TRUE);
		m_HeadGrid.EnableWindow(TRUE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Control_State
// Purpose:		This routine enables/disables all the dialog controls depending
//				on state.
// Arguments:	State		-	BOOL = (TRUE = enabled, FALSE = disabled)
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::Control_State(BOOL State)
{
	// Buttons
	m_AbortButton.EnableWindow(State);
	m_ClearButton.EnableWindow(State);
	m_ExitButton.EnableWindow(State);
	m_ArchiveButton.EnableWindow(State);
	m_ResetButton.EnableWindow(State);
	m_SetButton.EnableWindow(State);
	m_StartButton.EnableWindow(State);

	// Drop Lists
	m_DropHead.EnableWindow(State);
	m_DropType.EnableWindow(State);
	m_DropConfig.EnableWindow(State);
	m_DropEmTx.EnableWindow(State);

	// List Box
	m_HeadList.EnableWindow(State);

	// Grid
	m_HeadGrid.EnableWindow(State);

	// Spin Control
	m_DelaySpin.EnableWindow(State);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnReset
// Purpose:		This routine controls what happens when the "Reset" button is
//				pressed.  All controls revert from status to selection.
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnReset() 
{
	// local variables
	long i = 0;

	// clear all status
	for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) m_BlockStatus[i] = 0;

	// mode back to select
	m_Mode = 0;

	// clear abort flag
	m_Abort = 0;
	
	// progress to 0
	m_TotalProgress.SetPos(0);
	m_StageProgress.SetPos(0);
	m_StageLabel.SetWindowText("Current Stage:");

	// Clear Status labels
	m_Status1.SetWindowText("");
	m_Status2.SetWindowText("");
	m_Status3.SetWindowText("");

	// switch display
	Redisplay_Blocks();

	// controls back on
	Control_State(TRUE);

	// Continue, Reset & Abort Not Active Yet
	m_AbortButton.EnableWindow(FALSE);
	m_ContinueButton.EnableWindow(FALSE);
	m_ResetButton.EnableWindow(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnAbort
// Purpose:		This routine controls what happens when the "Abort" button is
//				pressed.  Any setup in progress is aborted.
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnAbort() 
{
	// local variables
	long Status = 0;

	IDSMain* pDS = NULL;

	HRESULT hr = S_OK;

	// set abort flag
	m_Abort = 1;

	// change state of abort button (show that it was recognized)
	m_AbortButton.EnableWindow(FALSE);

	// Create an instance of detector setup
	pDS = Get_DS_Ptr(pErrEvtSup);
	if (pDS == NULL) Status = Add_Error(pErrEvtSup, "OnAbort", "Failed to Get DS Pointer", 0);

	// if instance created successfully
	if (SUCCEEDED(hr)) {

		// abort setup
		hr = pDS->Abort();
		if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "OnAbort", "Method Failed (pDS->Abort), HRESULT", hr);

		// Release the server
		pDS->Release();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		SetupThread
// Purpose:		starts the setup down in the DSCOM COM Server and monitors its
//				progress updating the progress bars.  For time alignment, posts
//				requests for user actions and waits for "Continue" button to
//				be pressed before proceeding.
// Arguments:	none
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void SetupThread(void *dummy)
{
	// local variables
	long i = 0;
	long Status = 0;
	long ServerInitialized = 0;
	long Hours = 0;
	long Minutes = 0;
	long Seconds = 0;

	time_t CurrTime = 0;
	time_t StartTime = 0;

	IDSMain* pDS;

	HRESULT hr = S_OK;

	// initialize percentages
	m_TotalPercent = 0;
	m_StagePercent = 0;

	// determine when to start
	time(&CurrTime);
	StartTime = CurrTime + m_DelayTime;
	while ((CurrTime < StartTime) && (m_Abort == 0)) {
		Hours = (StartTime - CurrTime) / (60 * 60);
		Minutes = ((StartTime - CurrTime) % (60 * 60)) / 60;
		Seconds = (StartTime - CurrTime) % 60;
		sprintf((char *) m_StageStr, "Setup Starts In %d Hours %d Minutes %d Seconds", Hours, Minutes, Seconds);
		PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
		Sleep(1000);
		time(&CurrTime);
	}

	// abort at this stage just goes back to normal state
	if (m_Abort == 1) {

		// signify that this is an abort before execution
		m_Abort = -1;

		// signal completion
		PostMessage((HWND)m_MyWin, WM_USER_PROCESSING_COMPLETE, 0, 0);

		// exit
		return;
	}

	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if error support not establisted, note it in log
	if (pErrEvtSupThread != NULL) if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}


	// Create an instance of detector setup
	if (Status == 0) {
		pDS = Get_DS_Ptr(pErrEvtSupThread);
		if (pDS == NULL) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Failed to Get DS Pointer", 0);
		if (Status == 0) ServerInitialized = 1;
		if (Status != 0) {
			sprintf((char *) m_StageStr, "Setup Failed: CoCreateInstance");
			PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
		}
	}

	// if instance created successfully
	if (Status == 0) {

		// time alignment handled separately
		if (m_CurrType == SETUP_TYPE_TIME_ALIGNMENT) {

			// P39 requires movement of source
			if ((m_ScannerType == SCANNER_P39) || (m_ScannerType == SCANNER_P39_IIA)) {

				// emission blocks
				if ((m_CurrEmTx % 2) == 0) {

					// Model 393 has to be handled with two passes
					if ((m_ModelNumber == 393) || (m_ModelNumber == 2393)) {

						// instructions
						sprintf((char *) m_StageStr, "Place Source 10-15 cm From Center of Tunnel");
						PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);

						// wait for continue or abort
						m_Continue = 0;
						m_Abort = 0;
						m_ContinueState = 1;
						PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);
						while ((m_Continue == 0) && (m_Abort == 0)) Sleep(1000);
						m_ContinueState = 0;
						PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);

						// if not aborted
						if ((m_Abort == 0) && (Status == 0)) {

							// start time alignment - first stage
							hr = pDS->Time_Alignment(TIME_ALIGN_TYPE_393_1, m_Pair_393_1, &Status);
							if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Time_Alignment), HRESULT", hr);
							if (Status != 0) {
								sprintf((char *) m_StageStr, "Time Alignment Failed");
								PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
							}

							// loop for proogress
							m_TotalPercent = 0;
							m_StagePercent = 0;
							while ((m_StagePercent < 100) && (Status == 0)) {

								// wait 5 seconds between polling
								Sleep(5000);

								// poll for progress
								hr = pDS->Setup_Progress(m_StageStr, &m_StagePercent, &m_TotalPercent);
								if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Progress), HRESULT", hr);
								if (Status != 0) {
									sprintf((char *) m_StageStr, "Time Alignment Failed");
									PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
								}

								// update status indicators
								m_TotalPercent = 0;
								sprintf((char *) m_StageStr, "Time Align - First Stage");
								if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
							}

							// get the results
							if (Status == 0) {
								hr = pDS->Setup_Finished(m_BlockStatus);
								if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Finished), HRESULT", hr);
								if (Status != 0) {
									sprintf((char *) m_StageStr, "Time Alignment Failed");
									PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
								}
								for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
									if (m_BlockStatus[i] != 0) Status = -1;
								}

								// instructions for next step
								sprintf((char *) m_StageStr, "Place Source on Tunnel");
								PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);

								// wait for continue or abort
								m_Continue = 0;
								m_ContinueState = 1;
								PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);
								while ((m_Continue == 0) && (m_Abort == 0) && (Status == 0)) Sleep(1000);
								m_ContinueState = 0;
								PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);
							}

							// if not aborted
							if ((m_Abort == 0) && (Status == 0)) {

								// start time alignment - second stage
								hr = pDS->Time_Alignment(TIME_ALIGN_TYPE_393_2, m_Pair_393_2, &Status);
								if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Time_Alignment), HRESULT", hr);
								if (Status != 0) {
									sprintf((char *) m_StageStr, "Time Alignment Failed");
									PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
								}

								// loop for proogress
								m_TotalPercent = 0;
								m_StagePercent = 0;
								while ((m_StagePercent < 100) && (Status == 0)) {

									// wait 5 seconds between polling
									Sleep(5000);

									// poll for progress
									hr = pDS->Setup_Progress(m_StageStr, &m_StagePercent, &m_TotalPercent);
									if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Progress), HRESULT", hr);
									if (Status != 0) {
										sprintf((char *) m_StageStr, "Time Alignment Failed");
										PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
									}

									// update status indicators
									m_TotalPercent = 0;
									sprintf((char *) m_StageStr, "Time Align - Second Stage");
									if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
								}

								// get the results
								if (Status == 0) hr = pDS->Setup_Finished(m_BlockStatus);
								if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Finished), HRESULT", hr);
								if (Status != 0) {
									sprintf((char *) m_StageStr, "Time Alignment Failed");
									PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
								}
								for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
									if (m_BlockStatus[i] != 0) Status = -1;
								}
							}
						}

					// other p39s have a geometry that can support a single position
					} else {

						// instructions
						sprintf((char *) m_StageStr, "Place Source 20-25 cm From Center of Tunnel");
						PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);

						// wait for continue or abort
						m_Continue = 0;
						m_Abort = 0;
						m_ContinueState = 1;
						PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);
						while ((m_Continue == 0) && (m_Abort == 0)) Sleep(1000);
						m_ContinueState = 0;
						PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);

						// if not aborted
						if ((m_Abort == 0) && (Status == 0)) {

							// start time alignment - first stage
							hr = pDS->Time_Alignment(TIME_ALIGN_TYPE_395, m_EmPair, &Status);
							if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Time_Alignment), HRESULT", hr);
							if (Status != 0) {
								sprintf((char *) m_StageStr, "Time Alignment Failed");
								PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
							}

							// loop for proogress
							m_TotalPercent = 0;
							m_StagePercent = 0;
							while ((m_StagePercent < 100) && (Status == 0)) {

								// wait 5 seconds between polling
								Sleep(5000);

								// poll for progress
								hr = pDS->Setup_Progress(m_StageStr, &m_StagePercent, &m_TotalPercent);
								if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Progress), HRESULT", hr);
								if (Status != 0) {
									sprintf((char *) m_StageStr, "Time Alignment Failed");
									PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
								}

								// update status indicators
								m_TotalPercent = 0;
								sprintf((char *) m_StageStr, "Time Alignment");
								if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
							}

							// get the results
							if (Status == 0) {
								hr = pDS->Setup_Finished(m_BlockStatus);
								if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Finished), HRESULT", hr);
								if (Status != 0) {
									sprintf((char *) m_StageStr, "Time Alignment Failed");
									PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
								}
								for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
									if (m_BlockStatus[i] != 0) Status = -1;
								}
							}
						}
					}
				}

				// transmission blocks
				if ((m_CurrEmTx > 0) && (m_Abort == 0) && (Status == 0)) {

					// notification
					sprintf((char *) m_StageStr, "Transmission Source Time Alignment");
					PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);

					// start time alignment of odd numbered heads
					hr = pDS->Time_Alignment(TIME_ALIGN_TYPE_TRANSMISSION, m_TxPair, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Time_Alignment), HRESULT", hr);
					if (Status != 0) {
						sprintf((char *) m_StageStr, "Time Alignment Failed");
						PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
					}

					// loop for proogress
					m_TotalPercent = 0;
					m_StagePercent = 0;
					while ((m_StagePercent < 100) && (Status == 0)) {

						// wait 5 seconds between polling
						Sleep(5000);

						// poll for progress
						hr = pDS->Setup_Progress(m_StageStr, &m_StagePercent, &m_TotalPercent);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Progress), HRESULT", hr);
						if (Status != 0) {
							sprintf((char *) m_StageStr, "Time Alignment Failed");
							PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
						}

						// update status indicators
						m_TotalPercent = 0;
						sprintf((char *) m_StageStr, "Time Align Transmission Blocks");
						if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
					}

					// get the results
					if (Status == 0) {
						hr = pDS->Setup_Finished(m_BlockStatus);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Finished), HRESULT", hr);
						if (Status != 0) {
							sprintf((char *) m_StageStr, "Time Alignment Failed");
							PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
						}
						for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) if (m_BlockStatus[i] != 0) Status = -1;
					}
				}

				// if aborted, set block status
				if (m_Abort != 0) {
					for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
						if (m_BlockSelect[i] != 0) m_BlockStatus[i] |= ABORT_FATAL;
					}
				}

			// HRRT Time Alignment
			} else if (SCANNER_HRRT == m_ScannerType) {

				// instructions
				sprintf((char *) m_StageStr, "Attach Rotating Rod Source and Bypass Rebinner");
				PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);

				// wait for continue or abort
				m_Continue = 0;
				m_Abort = 0;
				m_ContinueState = 1;
				PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);
				while ((m_Continue == 0) && (m_Abort == 0)) Sleep(1000);
				m_ContinueState = 0;
				PostMessage((HWND) m_MyWin, WM_USER_CONTINUE_STATE, 0, 0);

				// if not aborted
				if ((m_Abort == 0) && (Status == 0)) {

					// start time alignment
					hr = pDS->Time_Alignment(0, m_EmPair, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Time_Alignment), HRESULT", hr);
					if (Status != 0) {
						sprintf((char *) m_StageStr, "Time Alignment Failed");
						PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
					}

					// loop for proogress
					m_TotalPercent = 0;
					m_StagePercent = 0;
					while ((m_StagePercent < 100) && (Status == 0)) {

						// wait 5 seconds between polling
						Sleep(5000);

						// poll for progress
						hr = pDS->Setup_Progress(m_StageStr, &m_StagePercent, &m_TotalPercent);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Progress), HRESULT", hr);
						if (Status != 0) {
							sprintf((char *) m_StageStr, "Time Alignment Failed");
							PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
						}

						// update status indicators
						if (m_ScannerType != SCANNER_HRRT) m_TotalPercent = 0;
						sprintf((char *) m_StageStr, "Time Alignment");
						if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
					}

					// get the results
					if (Status == 0) {
						hr = pDS->Setup_Finished(m_BlockStatus);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Finished), HRESULT", hr);
						if (Status != 0) {
							sprintf((char *) m_StageStr, "Time Alignment Failed");
							PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
						}
					}
				}

			// others not implemented yet
			} else {
				sprintf((char *) m_StageStr, "Not Yet Implemented");
				PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			}

		// singles setup
		} else {

			// Setup Head(s)
			hr = pDS->Setup(m_CurrType, m_CurrConfig, m_BlockSelect, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup), HRESULT", hr);
			if (Status != 0) {
				sprintf((char *) m_StageStr, "Setup Failed");
				PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			}

			// loop for proogress
			m_TotalPercent = 0;
			m_StagePercent = 0;
			while ((m_TotalPercent < 100) && (Status == 0)) {

				// wait 5 seconds between polling
				Sleep(5000);

				// poll for progress
				hr = pDS->Setup_Progress(m_StageStr, &m_StagePercent, &m_TotalPercent);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Progress), HRESULT", hr);
				if (Status != 0) {
					sprintf((char *) m_StageStr, "Setup Failed");
					PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
				}

				// update status indicators
				if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			}

			// get the results
			if (Status == 0) {
				hr = pDS->Setup_Finished(m_BlockStatus);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "SetupThread", "Method Failed (pDS->Setup_Finished), HRESULT", hr);
				if (Status != 0) {
					sprintf((char *) m_StageStr, "Setup Failed");
					PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
				}
			}
		}

		// Release the server
		if (ServerInitialized == 1) pDS->Release();
	}

	// Release COM
	::CoUninitialize();

	// string
	if (m_Abort == 1) {
		sprintf((char *) m_StageStr, "Current Stage: Aborted");
	} else {
		if (Status == 0) {
			sprintf((char *) m_StageStr, "Current Stage: Complete");
		} else {
			for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
				if (m_BlockSelect[i] != 0) m_BlockStatus[i] |= BLOCK_FATAL;
			}
		}
	}

	// All Complete
	PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);

	// let the gui know it is done
	PostMessage((HWND)m_MyWin, WM_USER_PROCESSING_COMPLETE, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPostProgress
// Purpose:		callback routine used to update the progress bars and stage
//				label from the setup thread.
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

LRESULT CDSGUIDlg::OnPostProgress(WPARAM wp, LPARAM lp)
{
	// update status
	m_StageProgress.SetPos(m_StagePercent);
	m_TotalProgress.SetPos(m_TotalPercent);
	m_StageLabel.SetWindowText((char *) m_StageStr);

	// requires return value
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnProcessingComplete
// Purpose:		callback to signal setup completion 
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

LRESULT CDSGUIDlg::OnProcessingComplete(WPARAM wp, LPARAM lp)
{
	// local variables
	long i = 0;

	// if timer was aborted, reset
	if (m_Abort == -1) {

		// clear all status
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) m_BlockStatus[i] = 0;

		// mode back to select
		m_Mode = 0;

		// clear abort flag
		m_Abort = 0;

		// clear display line
		m_StageLabel.SetWindowText("Current Stage:");

		// controls back on
		Control_State(TRUE);
	} else {

		// re-enable exit, archive, reset and head list
		m_ExitButton.EnableWindow(TRUE);
		m_ResetButton.EnableWindow(TRUE);
		m_DropHead.EnableWindow(TRUE);
		m_HeadGrid.EnableWindow(TRUE);
	}

	// switch display
	Redisplay_Blocks();

	// disable abort
	m_AbortButton.EnableWindow(FALSE);

	// requires return value
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnStateChange
// Purpose:		callback to signal continue button state has changed 
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

LRESULT CDSGUIDlg::OnStateChange(WPARAM wp, LPARAM lp)
{
	// update state
	if (m_ContinueState == 0) {
		m_ContinueButton.EnableWindow(FALSE);
	} else {
		m_ContinueButton.EnableWindow(TRUE);
	}

	// requires return value
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnArchive
// Purpose:		brings up the Archiving Dialog (LoadDlg.cpp) where the user 
//				can save the current setup or restore a previous one from archive 
// Arguments:	none
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnArchive() 
{
	// local variables
	CLoadDlg pDlg;

	// call gui
	pDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnContinue
// Purpose:		controls what happens when the "Continue" button is pressed.
//				the state of a global flag monitored by the setup thread is changed. 
// Arguments:	none
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnContinue() 
{
	m_Continue = 1;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDestroy
// Purpose:		Steps to be performed when the GUI is closed
// Arguments:	None
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnDestroy() 
{
	// local variables 
	HRESULT hr = S_OK;

	CDialog::OnDestroy();
	
	// release the lock
	if (m_LockCode > 0) {
		hr = pLock->UnlockACS(m_LockCode);
		if (hr != S_OK) {
			if (FAILED(hr)) Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pLock->UnlockACS), HRESULT", hr);
			MessageBox("Could Not Unlock Server", "Error", MB_OK);
		}
	}

	// release the server
	if (pLock != NULL) {
		hr = pLock->Release();
		if (hr != S_OK) {
			if (FAILED(hr)) Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pLock->Release), HRESULT", hr);
			MessageBox("Could Not Release Lock Manager", "Error", MB_OK);
		}
	}

	// release error event support
	if (pErrEvtSup != NULL) delete pErrEvtSup;
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
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(CErrorEventSupport* pErrSup, char *Where, char *What, long Why)
{
	// local variable
	long error_out = -1;
	long SaveState = 0;
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_DSG";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// fire message to event handler
	if (pErrEvtSup != NULL) hr = pErrEvtSup->Fire(GroupName, DSG_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);

	// return error code
	return error_out;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDeltaposDelaySpin
// Purpose:		controls the response when the "Delay" spin control is used
// Arguments:	*pNMHDR		- NMHDR = Windows Control Structure (automatically generated)
//				*pResult	- LRESULT = Windows return value (automatically generated)
// Returns:		void
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CDSGUIDlg::OnDeltaposDelaySpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	long TwoDays = 60 * 60 * 24 * 2;
	long Hours = 0;
	long Minutes = 0;

	char Str[MAX_STR_LEN];

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here

	// change value (by 10 minutes for each click) and then limit it
	m_DelayTime -= (pNMUpDown->iDelta) * 600;
	if (m_DelayTime < 0) m_DelayTime = 0;
	if (m_DelayTime > TwoDays) m_DelayTime = TwoDays;

	// display the new number
	Hours = m_DelayTime / (60 * 60);
	Minutes = (m_DelayTime % (60 * 60)) / 60;
	sprintf(Str, "%d Hr %d Min", Hours, Minutes);
	m_DelayField.SetWindowText(Str);

	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_DS_Ptr
// Purpose:		retrieve a pointer to the Detector Setup COM server (DSCOM)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		IDSMain* pointer to DSCOM
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

IDSMain* Get_DS_Ptr(CErrorEventSupport *pErrSup)
{
	// local variables
	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	IDSMain *Ptr = NULL;
	
	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// get server
	if (getenv("DSCOM_COMPUTER") == NULL) {
		sprintf(Server, "P39ACS");
	} else {
		sprintf(Server, "%s", getenv("DSCOM_COMPUTER"));
	}
	for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
	si.pwszName = Server_Name;

	// identify interface
	iid = IID_IDSMain;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	// get instance
	hr = ::CoCreateInstanceEx(CLSID_DSMain, NULL, CLSCTX_SERVER, &si, 1, mqi);
	if (FAILED(hr)) Add_Error(pErrSup, "Get_DS_Ptr", "Failed ::CoCreateInstanceEx(CLSID_DSMain, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (IDSMain*) mqi[0].pItf;

	return Ptr;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Lock_Ptr
// Purpose:		retrieve a pointer to the Lock Manager COM server (LockMgr)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		ILockClass* pointer to LockMgr
// History:		23 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

ILockClass* Get_Lock_Ptr(CErrorEventSupport *pErrSup)
{
	// local variables
	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	ILockClass *Ptr = NULL;
	
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
	if (FAILED(hr)) Add_Error(pErrSup, "Get_DU_Ptr", "Failed ::CoCreateInstanceEx(CLSID_DUMain, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (ILockClass*) mqi[0].pItf;

	return Ptr;
}
