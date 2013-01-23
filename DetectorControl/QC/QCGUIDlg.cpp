// QCGUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "QCGUI.h"
#include "QCGUIDlg.h"

#include "gantryModelClass.h"
#include "isotopeInfo.h"

#include "LockMgr.h"
#include "LockMgr_i.c"
#include "DHICom.h"
#include "DHICom_i.c"
#include "DUCom.h"
#include "DUCom_i.c"
#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

#include "DHI_Constants.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_USER_POST_PROGRESS		(WM_USER+1)
#define WM_USER_PROCESSING_COMPLETE	(WM_USER+2)

static long m_NumHeads;
static long m_XBlocks;
static long m_YBlocks;
static long m_XCrystals;
static long m_YCrystals;
static long m_NumBlocks;
static long m_NumSettings;
static long m_SetupEngIndex;
static long m_Abort;
static long m_Acquiring;
static long m_Display_X;
static long m_Display_Y;
static long m_CurrView;
static long m_CurrLayer;
static long m_CurrHead;

static long m_ModelNumber;
static long m_ScannerType;
static long m_OverStatus;

static long m_HeadPresent[MAX_HEADS];
static long m_NumLayers[MAX_HEADS];
static long m_HeadIndex[MAX_HEADS];

static long m_CurrTime[2*MAX_HEADS];

static long *m_CurrData[2*MAX_HEADS];
static long *m_MasterData[2*MAX_HEADS];

static long m_BlockStatus[2*MAX_HEADS*MAX_BLOCKS];
static long m_HeadStatus[2*MAX_HEADS];

static __int64 m_LockCode = -1;

char m_OutStr[MAX_STR_LEN];

static IDUMain *pDU;
static ILockClass* pLock;

IDUMain* Get_DU_Ptr(CErrorEventSupport *pErrSup);
IAbstract_DHI* Get_DAL_Ptr(CErrorEventSupport *pErrSup);
ILockClass* Get_Lock_Ptr(CErrorEventSupport *pErrSup);

static HANDLE m_MyWin;

CErrorEventSupport* pErrEvtSup;
CErrorEventSupport* pErrEvtSupThread;

long Add_Error(CErrorEventSupport* pErrSup, char *Where, char *What, long Why);
void AcqThread(void *dummy);

// view types
#define STATUS_VIEW 0
#define CURRENT_VIEW 1
#define MASTER_VIEW 2

// block status definitions
#define WARNING_STATUS				0xFFFF0000
#define ERROR_STATUS				0x0000FFFF
#define WARNING_NO_CURRENT_DATA		0x00010000
#define WARNING_NO_MASTER_DATA		0x00020000
#define WARNING_BLOCK_AVERAGE		0x00040000
#define ERROR_NO_CURRENT_EVENTS		0x00000001
#define ERROR_NO_MASTER_EVENTS		0x00000002

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
// CQCGUIDlg dialog

CQCGUIDlg::CQCGUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CQCGUIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CQCGUIDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CQCGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQCGUIDlg)
	DDX_Control(pDX, IDC_LAYER_DROP, m_LayerDrop);
	DDX_Control(pDX, IDC_HEAD_LABEL9, m_Head_9_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL8, m_Head_8_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL7, m_Head_7_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL6, m_Head_6_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL5, m_Head_5_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL4, m_Head_4_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL3, m_Head_3_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL2, m_Head_2_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL15, m_Head_15_Label);
	DDX_Control(pDX, IDC_GANTRY_LABEL, m_Gantry_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL13, m_Head_13_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL14, m_Head_14_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL12, m_Head_12_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL11, m_Head_11_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL10, m_Head_10_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL1, m_Head_1_Label);
	DDX_Control(pDX, IDC_HEAD_LABEL0, m_Head_0_Label);
	DDX_Control(pDX, IDC_MESSAGE, m_Message);
	DDX_Control(pDX, IDC_VIEW_DROP, m_ViewDrop);
	DDX_Control(pDX, IDABORT, m_AbortButton);
	DDX_Control(pDX, IDEXIT, m_ExitButton);
	DDX_Control(pDX, IDSTART, m_StartButton);
	DDX_Control(pDX, IDC_DATE_LABEL, m_DateLabel);
	DDX_Control(pDX, IDC_HEAD_DROP, m_HeadDrop);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CQCGUIDlg, CDialog)
	//{{AFX_MSG_MAP(CQCGUIDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDEXIT, OnExit)
	ON_BN_CLICKED(IDSTART, OnStart)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_HEAD_DROP, OnSelchangeHeadDrop)
	ON_MESSAGE(WM_USER_POST_PROGRESS, OnPostProgress)
	ON_MESSAGE(WM_USER_PROCESSING_COMPLETE, OnProcessingComplete)
	ON_BN_CLICKED(IDABORT, OnAbort)
	ON_CBN_SELCHANGE(IDC_VIEW_DROP, OnSelchangeViewDrop)
	ON_CBN_SELCHANGE(IDC_LAYER_DROP, OnSelchangeLayerDrop)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQCGUIDlg message handlers

BOOL CQCGUIDlg::OnInitDialog()
{
	// local variables
	long i = 0;
	long XPos = 0;
	long YPos = 0;
	long XSize = 512 + 140;
	long YSize = 512 + 90;
	long Next = 0;

	byte Force = 0;

	char Str[MAX_STR_LEN];

	WINDOWPLACEMENT Placement;

	HRESULT hr = S_OK;

	// Color
	COLORREF Color;
	HBRUSH hColorBrush;

	// instantiate a Gantry Object Model object
	CGantryModel model;
	pHead Head;		

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
	pErrEvtSup = new CErrorEventSupport(true, true);

	// if error support not establisted, note it in log
	if (pErrEvtSup == NULL) {
		MessageBox("Failed to Create ErrorEventSupport");

	// if it was not fully istantiated, note it in log and release it
	} else if (pErrEvtSup->InitComplete(hr) == false) {
		sprintf(Str, "ErrorEventSupport Failed During Initialization. HRESULT = %X", hr);
		MessageBox(Str);
		delete pErrEvtSup;
		pErrEvtSup = NULL;
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

	// P39 Phase IIA family (2391 - 2396)
	case 2391:
	case 2392:
	case 2393:
	case 2394:
	case 2395:
	case 2396:
		m_ScannerType = SCANNER_P39_IIA;
		break;

	// unknown gantry type - display message and exit
	default:
		MessageBox("Unrecognized Gantry Model", "Error", MB_OK);
		PostMessage(WM_CLOSE);
		return TRUE;
	}

	// number of blocks
	m_NumBlocks = model.blocks();
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

	// set views
	m_ViewDrop.InsertString(STATUS_VIEW, "Status");
	m_ViewDrop.InsertString(CURRENT_VIEW, "Current");
	m_ViewDrop.InsertString(MASTER_VIEW, "Master");

	// get the setting index for setup energy
	m_NumSettings = model.nAnalogSettings();
	m_SetupEngIndex = model.setupEnergySetting();

	// initialize Head arrays
	for (i = 0 ; i < MAX_HEADS ; i++) {
		m_HeadPresent[i] = 0;
		m_NumLayers[i] = 0;
		m_HeadIndex[i] = 0;
		m_CurrData[i] = NULL;
		m_CurrData[MAX_HEADS+i] = NULL;
		m_MasterData[i] = NULL;
		m_MasterData[MAX_HEADS+i] = NULL;
		m_CurrTime[i] = -1;
		m_CurrTime[MAX_HEADS+i] = -1;
	}

	// get the data for the next head
	Head = model.headInfo();

	// set head values
	m_NumHeads = model.nHeads();
	for (i = 0 ; i < m_NumHeads ; i++) {

		// globals
		m_HeadPresent[Head[i].address] = 1;
		m_HeadIndex[i] = Head[i].address;

		// number of layers
		switch (Head[i].type) {

		case HEAD_TYPE_LSO_ONLY:
			m_NumLayers[Head[i].address] = 1;
			break;

		case HEAD_TYPE_LSO_LSO:
			m_NumLayers[Head[i].address] = 2;
			break;

		case HEAD_TYPE_LSO_NAI:
			m_NumLayers[Head[i].address] = 2;
			break;

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
	}

	// device context
	CPaintDC dc(this);

	// Initialize Greyscale Color Table - TGG
	for (i = 0 ; i < 256 ; i++)	{
		Color = RGB(i, i, i);
		hdcColor[i] = CreateCompatibleDC(dc);
		hColorBlock[i] = CreateCompatibleBitmap(dc, 1, 1);
		SelectObject(hdcColor[i], hColorBlock[i]);
		hColorBrush = CreateSolidBrush(Color);
		SelectObject(hdcColor[i], hColorBrush);
		PatBlt(hdcColor[i], 0, 0, 1, 1, PATCOPY);
	}

	// Create a memory DC and bitmap and associate them for the run mode data
	m_bm = CreateCompatibleBitmap(dc, 128, 128);
	m_dc = ::CreateCompatibleDC(dc);
	SelectObject(m_dc, m_bm);

	// Initialize color red
	Color = RGB(255, 0, 0);
	m_red_dc = CreateCompatibleDC(dc);
	m_red_bm = CreateCompatibleBitmap(dc, 1, 1);
	SelectObject(m_red_dc, m_red_bm);
	hColorBrush = CreateSolidBrush(Color);
	SelectObject(m_red_dc, hColorBrush);
	PatBlt(m_red_dc, 0, 0, 1, 1, PATCOPY);
	DeleteObject(hColorBrush);

	// Initialize color yellow
	Color = RGB(255, 255, 0);
	m_yellow_dc = CreateCompatibleDC(dc);
	m_yellow_bm = CreateCompatibleBitmap(dc, 1, 1);
	SelectObject(m_yellow_dc, m_yellow_bm);
	hColorBrush = CreateSolidBrush(Color);
	SelectObject(m_yellow_dc, hColorBrush);
	PatBlt(m_yellow_dc, 0, 0, 1, 1, PATCOPY);
	DeleteObject(hColorBrush);

	// Initialize color green
	Color = RGB(0, 255, 0);
	m_green_dc = CreateCompatibleDC(dc);
	m_green_bm = CreateCompatibleBitmap(dc, 1, 1);
	SelectObject(m_green_dc, m_green_bm);
	hColorBrush = CreateSolidBrush(Color);
	SelectObject(m_green_dc, hColorBrush);
	PatBlt(m_green_dc, 0, 0, 1, 1, PATCOPY);
	DeleteObject(hColorBrush);

	// Create an instance of Lock Manager Server.
	pLock = Get_Lock_Ptr(pErrEvtSup);
	if (pLock == NULL) {
		Add_Error(pErrEvtSup, "OnInitDialog", "Failed to Get LockMgr Pointer", 0);
		MessageBox("Failed To Attach To Lock Manager", "Error", MB_OK);
		PostMessage(WM_CLOSE);
		return TRUE;
	}

	// lock abstraction layer
	hr = pLock->LockACS(&m_LockCode, Force);
	if (hr != S_OK) {

		// message
		if (FAILED(hr)) {
			Add_Error(pErrEvtSup, "OnInitDialog", "Method Failed (pLock->LockACS), HRESULT", hr);
			MessageBox("Method Failed:  pLock->LockACS", "Error", MB_OK);
		} else if (hr == S_FALSE) {
			MessageBox("Servers Already Locked", "Error", MB_OK);
		}

		// close dialog
		PostMessage(WM_CLOSE);
		return TRUE;
	}

	// Create an instance of Detector Utilities Server.
	pDU = Get_DU_Ptr(pErrEvtSup);
	if (pDU == NULL) {
		Add_Error(pErrEvtSup, "OnInitDialog", "Failed to Get DU Pointer", 0);
		MessageBox("Failed To Attach To Detector Utilities Server", "Error", MB_OK);
		PostMessage(WM_CLOSE);
	}

	// position of data display
	m_Display_X = 10;
	m_Display_Y = 25;

	// Positively position controls
	m_DateLabel.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X;
	Placement.rcNormalPosition.right = m_Display_X + 140;
	Placement.rcNormalPosition.top = 5;
	Placement.rcNormalPosition.bottom = 20;
	m_DateLabel.SetWindowPlacement(&Placement);

	m_ViewDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X + 150;
	Placement.rcNormalPosition.right = m_Display_X + 220;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 20;
	m_ViewDrop.SetWindowPlacement(&Placement);

	m_HeadDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X + 230;
	Placement.rcNormalPosition.right = m_Display_X + 300;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 20;
	m_HeadDrop.SetWindowPlacement(&Placement);

	m_LayerDrop.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X + 310;
	Placement.rcNormalPosition.right = m_Display_X + 380;
	Placement.rcNormalPosition.top = 0;
	Placement.rcNormalPosition.bottom = 20;
	m_LayerDrop.SetWindowPlacement(&Placement);

	m_Message.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X;
	Placement.rcNormalPosition.right = m_Display_X + 512;
	Placement.rcNormalPosition.top = m_Display_Y + 512 + 5;
	Placement.rcNormalPosition.bottom = m_Display_Y + 512 + 20;
	m_Message.SetWindowPlacement(&Placement);

	m_StartButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X + 512 + 20;
	Placement.rcNormalPosition.right = m_Display_X + 512 + 120;
	Placement.rcNormalPosition.top = m_Display_Y + 512 - 85;
	Placement.rcNormalPosition.bottom = m_Display_Y + 512 - 60;
	m_StartButton.SetWindowPlacement(&Placement);

	m_AbortButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X + 512 + 20;
	Placement.rcNormalPosition.right = m_Display_X + 512 + 120;
	Placement.rcNormalPosition.top = m_Display_Y + 512 - 55;
	Placement.rcNormalPosition.bottom = m_Display_Y + 512 - 30;
	m_AbortButton.SetWindowPlacement(&Placement);

	m_ExitButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X + 512 + 20;
	Placement.rcNormalPosition.right = m_Display_X + 512 + 120;
	Placement.rcNormalPosition.top = m_Display_Y + 512 - 25;
	Placement.rcNormalPosition.bottom = m_Display_Y + 512;
	m_ExitButton.SetWindowPlacement(&Placement);

	m_Gantry_Label.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
	Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
	Placement.rcNormalPosition.top = 5;
	Placement.rcNormalPosition.bottom = 20;
	m_Gantry_Label.SetWindowPlacement(&Placement);
	Next = m_Display_Y;

	if (1 == m_HeadPresent[0]) {
		m_Head_0_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_0_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_0_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[1]) {
		m_Head_1_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_1_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_1_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[2]) {
		m_Head_2_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_2_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_2_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[3]) {
		m_Head_3_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_3_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_3_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[4]) {
		m_Head_4_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_4_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_4_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[5]) {
		m_Head_5_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_5_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_5_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[6]) {
		m_Head_6_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_6_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_6_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[7]) {
		m_Head_7_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_7_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_7_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[8]) {
		m_Head_8_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_8_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_8_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[9]) {
		m_Head_9_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_9_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_9_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[10]) {
		m_Head_10_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_10_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_10_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[11]) {
		m_Head_11_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_11_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_11_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[12]) {
		m_Head_12_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_12_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_12_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[13]) {
		m_Head_13_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_13_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_13_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[14]) {
		m_Head_14_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_14_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_14_Label.ShowWindow(SW_HIDE);
	}

	if (1 == m_HeadPresent[15]) {
		m_Head_15_Label.GetWindowPlacement(&Placement);
		Placement.rcNormalPosition.left = m_Display_X + 512 + 10;
		Placement.rcNormalPosition.right = m_Display_X + 512 + 50;
		Placement.rcNormalPosition.top = Next;
		Placement.rcNormalPosition.bottom = Next + 15;
		m_Head_15_Label.SetWindowPlacement(&Placement);
		Next += 20;
	} else {
		m_Head_15_Label.ShowWindow(SW_HIDE);
	}

	// set Layer Drop List
	if ((SCANNER_HRRT == m_ScannerType) || (SCANNER_PETSPECT == m_ScannerType)) {
		m_LayerDrop.InsertString(0, "Fast");
		m_LayerDrop.InsertString(1, "Slow");
	} else {
		m_LayerDrop.InsertString(0, "All");
		m_LayerDrop.ShowWindow(SW_HIDE);
	}

	// initial state
	m_CurrView = CURRENT_VIEW;
	m_ViewDrop.SetCurSel(m_CurrView);
	m_LayerDrop.SetCurSel(0);
	m_CurrLayer = 0;
	m_HeadDrop.SetCurSel(0);
	m_CurrHead = m_HeadIndex[0];

	// keep track of the window handle
	m_MyWin = m_hWnd;

	// resize window
	::SetWindowPos((HWND) m_hWnd, HWND_TOP, XPos, YPos, XSize, YSize, SWP_NOMOVE);

	// Refresh Display
	Update_Current();
	Update_Master();
	Update_Status();
	Update();
	Refresh();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CQCGUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CQCGUIDlg::OnPaint() 
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
		// refresh image
		Refresh();

		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CQCGUIDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CQCGUIDlg::OnExit() 
{
	// Exit
	OnOK();
}

void CQCGUIDlg::Refresh()
{
	// local variables
	long x = 0;
	long x1 = 0;
	long x2 = 0;

	long y = 0;
	long y1 = 0;
	long y2 = 0;

	long Next = m_Display_Y;
	long Head = 0;
	long Block = 0;
	long View = 0;
	long ErrCount = 0;
	long WarnCount = 0;

	double Limit = 0.0;

	char Str[MAX_STR_LEN];

	// get current view
	View = m_ViewDrop.GetCurSel();

	// paint image
	CPaintDC dc(this);

	//copy the image memory dc into the screen dc
	::StretchBlt(dc.m_hDC,			//destination
					m_Display_X,
					m_Display_Y,
					(4 * m_XBlocks * m_XCrystals),
					(4 * m_YBlocks * m_YCrystals),
					m_dc,		//source
					0,
					0,
					(m_XBlocks * m_XCrystals),
					(m_YBlocks * m_YCrystals),
					SRCCOPY);

	// maximum allowable warnings is 5%
	Limit = ((double) (m_XBlocks * m_YBlocks)) * 0.05;

	// display composite status
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// if the head is present
		if (1 == m_HeadPresent[Head]) {

			// Error
			if (((m_HeadStatus[Head] | m_HeadStatus[MAX_HEADS+Head]) & ERROR_STATUS) == ERROR_STATUS) {

				// increment the error count
				ErrCount++;

				//copy the red memory dc into the screen dc
				::StretchBlt(dc.m_hDC,			//destination
								m_Display_X + 512 + 55,
								Next,
								15,
								15,
								m_red_dc,		//source
								0,
								0,
								1,
								1,
								SRCCOPY);

			// Warning
			} else if (((m_HeadStatus[Head] | m_HeadStatus[MAX_HEADS+Head]) & WARNING_STATUS) == WARNING_STATUS) {

				// if the number of warnings exceeds limits, then it is a warning for the head and the scanner
				if (((double)(m_HeadStatus[Head] & 0x0000FFFF) > Limit) || ((double)(m_HeadStatus[MAX_HEADS+Head] & 0x0000FFFF) > Limit)) {

					// increment the warning count
					WarnCount++;

					//copy the yellow memory dc into the screen dc
					::StretchBlt(dc.m_hDC,			//destination
									m_Display_X + 512 + 55,
									Next,
									15,
									15,
									m_yellow_dc,		//source
									0,
									0,
									1,
									1,
									SRCCOPY);

				// otherwise, mark the head as no problems
				} else {

					//copy the green memory dc into the screen dc
					::StretchBlt(dc.m_hDC,			//destination
									m_Display_X + 512 + 55,
									Next,
									15,
									15,
									m_green_dc,		//source
									0,
									0,
									1,
									1,
									SRCCOPY);
				}

			// no problem
			} else {

				//copy the green memory dc into the screen dc
				::StretchBlt(dc.m_hDC,			//destination
								m_Display_X + 512 + 55,
								Next,
								15,
								15,
								m_green_dc,		//source
								0,
								0,
								1,
								1,
								SRCCOPY);
			}

			// position for next status mark
			Next += 20;
		}
	}

	// error occurred somewhere
	if (ErrCount > 0) {

		// store count as a negative
		m_OverStatus = 0 - ErrCount;

		//copy the red memory dc into the screen dc
		::StretchBlt(dc.m_hDC,			//destination
						m_Display_X + 512 + 55,
						m_Display_Y - 20,
						15,
						15,
						m_red_dc,		//source
						0,
						0,
						1,
						1,
						SRCCOPY);

	// warning occurred
	} else if (WarnCount > 0) {

		// store count
		m_OverStatus = WarnCount;

		//copy the yellow memory dc into the screen dc
		::StretchBlt(dc.m_hDC,			//destination
						m_Display_X + 512 + 55,
						m_Display_Y - 20,
						15,
						15,
						m_yellow_dc,	//source
						0,
						0,
						1,
						1,
						SRCCOPY);

	// no problem
	} else {

		// clear status
		m_OverStatus = 0;

		//copy the green memory dc into the screen dc
		::StretchBlt(dc.m_hDC,			//destination
						m_Display_X + 512 + 55,
						m_Display_Y - 20,
						15,
						15,
						m_green_dc,		//source
						0,
						0,
						1,
						1,
						SRCCOPY);
	}

	// select white pen for divider lines
	CPen BlackPen(PS_SOLID, 1, RGB(0, 0, 0));
	dc.SelectObject(&BlackPen);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextAlign(VTA_CENTER);

	// outline blocks
	for (Block = 0 ; Block < (m_XBlocks * m_YBlocks) ; Block++) {

		// boundaries
		x1 = m_Display_X + (4 * ((Block % m_XBlocks) * m_XCrystals));
		x2 = x1 + (4 * m_XCrystals);
		y1 = m_Display_Y + (4 * ((Block / m_XBlocks) * m_YCrystals));
		y2 = y1 + (4 * m_YCrystals);

		// draw block
		dc.MoveTo(x1, y1);
		dc.LineTo(x2, y1);
		dc.LineTo(x2, y2);
		dc.LineTo(x1, y2);
		dc.LineTo(x1, y1);

		// for status view, put up the block number
		if (STATUS_VIEW == View) {

			// middle of block
			x = (x1 + x2) / 2;
			y = ((2 * y1) + y2) / 3;

			// convert block number into a string
			sprintf(Str, "%d", Block);

			// Display String
			TextOut(dc, x, y, Str, strlen(Str));
		}
	}
}

void CQCGUIDlg::OnStart() 
{
	// local variables
	HANDLE hAcqThread;

	// initialize abort variables
	m_Abort = 0;
	m_Acquiring = 0;

	// disable all controls
	m_StartButton.EnableWindow(FALSE);
	m_AbortButton.EnableWindow(FALSE);
	m_ExitButton.EnableWindow(FALSE);
	m_HeadDrop.EnableWindow(FALSE);

	// start the processing thread
	m_Message.SetWindowText("Start Acquisition");
	hAcqThread = (HANDLE) _beginthread(AcqThread, 0, NULL);

	// activate the abort button
	m_AbortButton.EnableWindow(TRUE);

	// failed to start thread
	if (hAcqThread == NULL) {

		// enable all controls
		m_StartButton.EnableWindow(TRUE);
		m_AbortButton.EnableWindow(FALSE);
		m_ExitButton.EnableWindow(TRUE);
		m_HeadDrop.EnableWindow(TRUE);

		// update the display
		Update();

		// post message
		m_Message.SetWindowText("Failed To Start Thread");
	}
}

void CQCGUIDlg::OnDestroy() 
{
	// local variables
	HRESULT hr = S_OK;

	CDialog::OnDestroy();

	// if server locked, then unlock it
	if (m_LockCode != -1) {
		hr = pLock->UnlockACS(m_LockCode);
		if (FAILED(hr)) Add_Error(pErrEvtSup, "OnDestroy", "Method Failed (pLock->UnlockACS), HRESULT", hr);
		if (hr != S_OK) MessageBox("Could Not Release ACS Servers", "Error", MB_OK);
	}

	// release com servers (if attached)
	if (pLock != NULL) pLock->Release();
	if (pDU != NULL) pDU->Release();
	if (pErrEvtSup != NULL) delete pErrEvtSup;

	// release COM
	::CoUninitialize( );
}

void CQCGUIDlg::Update()
{
	// local variables
	long Block = 0;
	long Block_X = 0;
	long Block_Y = 0;
	long x = 0;
	long x1 = 0;
	long x2 = 0;
	long y = 0;
	long y1 = 0;
	long y2 = 0;
	long index = 0;
	long Max = 0;
	long View = 0;
	long Head = 0;
	long Layer = 0;

	long *Data = NULL;
	long *Master = NULL;

	char Str[MAX_STR_LEN];

	// get the current settings
	View = m_ViewDrop.GetCurSel();
	Head = m_HeadIndex[m_HeadDrop.GetCurSel()];
	Layer = m_LayerDrop.GetCurSel();

	// display data based on view
	switch (View) {

	case STATUS_VIEW:

		// pointers to data
		Data = m_CurrData[(Layer*MAX_HEADS)+Head];
		Master = m_MasterData[(Layer*MAX_HEADS)+Head];

		// if no data, note the reason in the date field
		if ((NULL == Data) || (NULL == Master)) {
			if (NULL == Data) {
				sprintf(Str, "No Current Flood");
			} else {
				sprintf(Str, "No Master Flood");
			}

		// otherwise, the date is the date for the current data
		} else {
			sprintf(Str, "%s", ctime(&m_CurrTime[(Layer*MAX_HEADS)+Head]));
			Str[24] = '\0';
		}

		// fill in the field
		m_DateLabel.SetWindowText(Str);

		// fill in all rows
		for (Block_Y = 0 ; Block_Y < m_YBlocks ; Block_Y++) {

			// y crystals
			y1 = Block_Y * m_YCrystals;
			y2 = y1 + m_YCrystals - 1;

			// fill in all columns
			for (Block_X = 0 ; Block_X < m_XBlocks ; Block_X++) {

				// x crystals
				x1 = Block_X * m_XCrystals;
				x2 = x1 + m_XCrystals - 1;

				// block number
				Block = (Block_Y * m_XBlocks) + Block_X;

				// fill bit map
				index = (Layer * MAX_HEADS * MAX_BLOCKS) + (Head * MAX_BLOCKS) + Block;
				for (y = y1 ; y <= y2 ; y++) for (x = x1 ; x <= x2 ; x++) {

					// if block had error - red
					if ((m_BlockStatus[index] & ERROR_STATUS) != 0) {
						BitBlt(m_dc, x, y, 1, 1, m_red_dc, 0, 0, SRCCOPY);

					// if block had warning - yellow
					} else if ((m_BlockStatus[index] & WARNING_STATUS) != 0) {
						BitBlt(m_dc, x, y, 1, 1, m_yellow_dc, 0, 0, SRCCOPY);

					// no error or warning - green
					} else {
						BitBlt(m_dc, x, y, 1, 1, m_green_dc, 0, 0, SRCCOPY);
					}
				}
			}
		}
		break;

	case CURRENT_VIEW:
	case MASTER_VIEW:

		// assign data and date string value
		if (CURRENT_VIEW == View) {
			Data = m_CurrData[(Layer*MAX_HEADS)+Head];
			if (NULL == Data) {
				sprintf(Str, "No Current Flood");
			} else {
				sprintf(Str, "%s", ctime(&m_CurrTime[m_CurrHead]));
				Str[24] = '\0';
			}
		} else {
			Data = m_MasterData[(Layer*MAX_HEADS)+Head];
			if (NULL == Data) {
				sprintf(Str, "No Master Flood");
			} else {
				sprintf(Str, "Master Flood");
			}
		}
		m_DateLabel.SetWindowText(Str);

		// fill in bitmap from image if data present
		if (NULL != Data) {

			// find maximum value
			for (y = 0 ; y < (m_YBlocks * m_YCrystals) ; y++) for (x = 0 ; x < (m_XBlocks * m_XCrystals) ; x++) {
				index = (y * 128) + x;
				if (Data[index] > Max) Max = Data[index];
			}

			// fill in bitmap
			for (y = 0 ; y < (m_YBlocks * m_YCrystals) ; y++) for (x = 0 ; x < (m_XBlocks * m_XCrystals) ; x++) {
				index = (Data[(y * 128) + x] * 256) / (Max + 1);
				BitBlt(m_dc, x, y, 1, 1, hdcColor[index], 0, 0, SRCCOPY);
			}

		// no image data
		} else {

			// fill in black
			for (y = 0 ; y < (m_YBlocks * m_YCrystals) ; y++) for (x = 0 ; x < (m_XBlocks * m_XCrystals) ; x++) {
				BitBlt(m_dc, x, y, 1, 1, hdcColor[0], 0, 0, SRCCOPY);
			}
		}
		break;
	}

	// force redraw
	Invalidate(TRUE);
}

void CQCGUIDlg::OnSelchangeHeadDrop() 
{
	// get new selection
	m_CurrHead = m_HeadIndex[m_HeadDrop.GetCurSel()];

	// refresh
	Update_Current();
	Update_Master();
	Update();
}

void AcqThread(void *dummy)
{
	// local variables
	long i = 0;
	long j = 0;
	long Status = 0;
	long Config = 0;
	long Head = -1;
	long lld = 400;
	long uld = 650;
	long Amount = 60;
	long AcqType = ACQUISITION_SECONDS;
	long Percent = 0;

	long LayerSelect[MAX_HEADS];
	long HeadStatus[MAX_HEADS];
	long Settings[MAX_BLOCKS*MAX_SETTINGS];

	long Blank[MAX_HEADS];

	HRESULT hr = S_OK;
	
	IAbstract_DHI *pDHI = NULL;
	IDUMain* pDUT = NULL;

	// instantiate an ErrorEventSupport object
	pErrEvtSupThread = new CErrorEventSupport(true, false);

	// if it was not fully istantiated, note it in log and release it
	if (pErrEvtSupThread != NULL) if (pErrEvtSupThread->InitComplete(hr) == false) {
		delete pErrEvtSupThread;
		pErrEvtSupThread = NULL;
	}

	// Create an instance of DHI Server.
	if (Status == 0) {
		pDHI = Get_DAL_Ptr(pErrEvtSupThread);
		if (pDHI == NULL) Status = Add_Error(pErrEvtSupThread, "AcqThread", "Failed to Get DAL Pointer", 0);
	}

	// Create an instance of Detector Utilities Server.
	if (Status == 0) {
		pDUT = Get_DU_Ptr(pErrEvtSupThread);
		if (pDUT == NULL) Add_Error(pErrEvtSupThread, "AcqThread", "Failed to Get DU Pointer", 0);
	}

	// if instance created successfully
	if ((Status == 0) && (m_Abort == 0)) {

		// blank array for transmission acquisition
		for (i = 0 ; i < MAX_HEADS ; i++) Blank[i] = 0;

		// set setup energy to 511 KeV
		if ((Status == 0) && (m_Abort == 0)) {
			sprintf(m_OutStr, "Set to 511 KeV");
			PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			for (i = 0 ; (i < MAX_HEADS) && (Status == 0) ; i++) {
				if (m_HeadPresent[i] != 0) {

					// retrieve analog settings
					hr = pDHI->Get_Analog_Settings(Config, i, Settings, &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "AcqThread", "Method Failed (pDHI->Get_Analog_Settings), HRESULT", hr);

					// set to 511
					if (Status == 0) {

						// set value
						for (j = 0 ; j < m_NumBlocks ; j++) Settings[(j*m_NumSettings)+m_SetupEngIndex] = 511;

						// send to scanner
						hr = pDHI->Set_Analog_Settings(Config, i, Settings, &Status);
						if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "AcqThread", "Method Failed (pDHI->Set_Analog_Settings), HRESULT", hr);
					}
				}
			}
		}

		// load crystal energy peaks
		if ((Status == 0) && (m_Abort == 0)) {
			sprintf(m_OutStr, "Loading Crystal Energy Peaks");
			PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			if (Status == 0) hr = pDHI->Load_RAM(Config, Head, RAM_ENERGY_PEAKS, &Status);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "AcqThread", "Method Failed (pDHI->Load_RAM), HRESULT", hr);
			if (Status != 0) {
				sprintf(m_OutStr, "Failed to Load Crystal Energy Peaks");
				PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			}
		}

		// and then take singles
		if ((Status == 0) && (m_Abort == 0)) {
			sprintf(m_OutStr, "Acquiring Data");
			PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			for (i = 0 ; i < MAX_HEADS ; i++) LayerSelect[i] = LAYER_ALL;
			if (Status == 0) hr = pDUT->Acquire_Singles_Data(DHI_MODE_RUN, Config, m_HeadPresent, Blank, LayerSelect, AcqType, Amount, lld, uld, &Status);
			m_Acquiring = 1;
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "AcqThread", "Method Failed (pDUT->Acquire_Singles_Data), HRESULT", hr);
			if (Status != 0) {
				sprintf(m_OutStr, "Failed to Start Acquisition");
				PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			}
		}

		// check percentage
		Percent = 0;
		while ((Percent < 100) && (Status == 0) && (m_Acquiring == 1)) {
			hr = pDUT->Acquisition_Progress(&Percent, HeadStatus);
			if (FAILED(hr)) Status = Add_Error(pErrEvtSupThread, "AcqThread", "Method Failed (pDUT->Acquisition_Progress), HRESULT", hr);
			if (Percent == 0) {
				sprintf(m_OutStr, "Initializing");
			} else {
				sprintf(m_OutStr, "Acquire Data: %d%%", Percent);
			}
			PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);
			if (Percent < 100) Sleep(1000);
		}
	}

	// Release COM
	if (pDHI != NULL) pDHI->Release();
	if (pDUT != NULL) pDUT->Release();
	::CoUninitialize();

	// All Complete
	sprintf(m_OutStr, "Acquisition Complete");
	PostMessage((HWND) m_MyWin, WM_USER_POST_PROGRESS, 0, 0);

	// let the gui know it is done
	PostMessage((HWND)m_MyWin, WM_USER_PROCESSING_COMPLETE, 0, 0);

	// release error pointer
	delete pErrEvtSupThread;
}

LRESULT CQCGUIDlg::OnPostProgress(WPARAM wp, LPARAM lp)
{
	// update status
	m_Message.SetWindowText(m_OutStr);

	// requires a return value
	return 0;
}

LRESULT CQCGUIDlg::OnProcessingComplete(WPARAM wp, LPARAM lp)
{

	// enable all controls
	m_StartButton.EnableWindow(TRUE);
	m_AbortButton.EnableWindow(FALSE);
	m_ExitButton.EnableWindow(TRUE);
	m_HeadDrop.EnableWindow(TRUE);

	// update the display
	Update();

	// requires a return value
	return 0;
}

void CQCGUIDlg::OnAbort() 
{
	// local variables
	HRESULT hr = S_OK;

	// set the abort flag to true
	m_Abort = 1;

	// turn off the abort button (some some sign it has been activated)
	m_AbortButton.EnableWindow(FALSE);

	// if data is being acquired by the detector utilities server, tell it to abort
	if (m_Acquiring == 1) {
		hr = pDU->Acquisition_Abort();
		if (FAILED(hr)) Add_Error(pErrEvtSup, "OnAbort", "Method Failed (pDU->Acquisition_Abort), HRESULT", hr);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Add_Error
// Purpose:	send an error message to the event handler
//			put it up in a message box
/////////////////////////////////////////////////////////////////////////////

long Add_Error(CErrorEventSupport* pErrSup, char *Where, char *What, long Why)
{
	// local variable
	long error_out = -1;
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_QCG";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "%s: %s = %d (decimal) %X (hexadecimal)", Where, What, Why, Why);

	// fire message to event handler
	if (pErrEvtSup != NULL) hr = pErrSup->Fire(GroupName, QCG_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);

	// return error code
	return error_out;
}

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

void CQCGUIDlg::Update_Current()
{
	long Head = 0;
	long Size = 0;
	long Status = 0;

	HRESULT hr = S_OK;

	// release all current pointers
	for (Head = 0 ; Head < (2*MAX_HEADS) ; Head++) {
		if (NULL != m_CurrData[Head]) {
			::CoTaskMemFree(m_CurrData[Head]);
			m_CurrData[Head] = NULL;
		}
	}

	// loop through all the heads
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// if the head is present
		if (1 == m_HeadPresent[Head]) {

			// petspect and hrrt, get both fast and slow layers
			if ((SCANNER_HRRT == m_ScannerType) || (SCANNER_PETSPECT == m_ScannerType)) {

				// always retrieve current fast coincidence flood
				hr = pDU->Current_Head_Data(DATA_MODE_COINC_FLOOD, 0, Head, LAYER_FAST, &m_CurrTime[Head], &Size, &m_CurrData[Head], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "Update_Current", "Method Failed (pDU->Current_Head_Data), HRESULT", hr);

				// if dual layer, also get slow flood
				if (2 == m_NumLayers[Head]) {
					hr = pDU->Current_Head_Data(DATA_MODE_COINC_FLOOD, 0, Head, LAYER_SLOW, &m_CurrTime[MAX_HEADS+Head], &Size, &m_CurrData[MAX_HEADS+Head], &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "Update_Current", "Method Failed (pDU->Current_Head_Data), HRESULT", hr);
				}

			// otherwise, the combined layers
			} else {

				// retrieve current coincidence flood
				hr = pDU->Current_Head_Data(DATA_MODE_COINC_FLOOD, 0, Head, LAYER_ALL, &m_CurrTime[Head], &Size, &m_CurrData[Head], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "Update_Current", "Method Failed (pDU->Current_Head_Data), HRESULT", hr);
			}
		}
	}
}

void CQCGUIDlg::Update_Master()
{
	long Head = 0;
	long Size = 0;
	long Status = 0;

	HRESULT hr = S_OK;

	// release all current pointers
	for (Head = 0 ; Head < (2*MAX_HEADS) ; Head++) {
		if (NULL != m_MasterData[Head]) {
			::CoTaskMemFree(m_MasterData[Head]);
			m_MasterData[Head] = NULL;
		}
	}

	// loop through all the heads
	for (Head = 0 ; Head < MAX_HEADS ; Head++) {

		// if the head is present
		if (1 == m_HeadPresent[Head]) {

			// petspect and hrrt, get both fast and slow layers
			if ((SCANNER_HRRT == m_ScannerType) || (SCANNER_PETSPECT == m_ScannerType)) {

				// always retrieve current fast coincidence flood
				hr = pDU->Read_Master(DATA_MODE_COINC_FLOOD, 0, Head, LAYER_FAST, &Size, &m_MasterData[Head], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "Update_Master", "Method Failed (pDU->Read_Master), HRESULT", hr);

				// if dual layer, also get slow flood
				if (2 == m_NumLayers[Head]) {
					hr = pDU->Read_Master(DATA_MODE_COINC_FLOOD, 0, Head, LAYER_SLOW, &Size, &m_MasterData[MAX_HEADS+Head], &Status);
					if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "Update_Master", "Method Failed (pDU->Read_Master), HRESULT", hr);
				}

			// otherwise, the combined layers
			} else {

				// retrieve current coincidence flood
				hr = pDU->Read_Master(DATA_MODE_COINC_FLOOD, 0, Head, LAYER_ALL, &Size, &m_MasterData[Head], &Status);
				if (FAILED(hr)) Status = Add_Error(pErrEvtSup, "Update_Master", "Method Failed (pDU->Read_Master), HRESULT", hr);
			}
		}
	}
}

void CQCGUIDlg::OnSelchangeViewDrop() 
{
	// local variables
	long View = 0;

	// get current view
	View = m_ViewDrop.GetCurSel();

	// if the view has changed
	if (View != m_CurrView) {

		// if old view was not status, then we need to save the current layer
		if (STATUS_VIEW != m_CurrView) m_CurrLayer = m_LayerDrop.GetCurSel();

		// update the display
		Update();
		Refresh();

		// save for next time
		m_CurrView = View;
	}
}

void CQCGUIDlg::OnSelchangeLayerDrop() 
{
	// local variables
	long Layer = 0;

	// get value from gui
	Layer = m_LayerDrop.GetCurSel();

	// if the layer has changed
	if (Layer != m_CurrLayer) {

		// update the display
		Update();
		Refresh();

		// save for next time
		m_CurrLayer = Layer;
	}
	
}

void CQCGUIDlg::Update_Status()
{
	// local variables
	long x = 0;
	long y = 0;
	long index = 0;
	long Xtal = 0;
	long Block = 0;
	long Head = 0;
	long Layer = 0;
	long MaxLayer = 0;
	long ErrCount = 0;
	long WarnCount = 0;

	long *Data = NULL;
	long *Master = NULL;

	double Scale = 0.0;
	double MeanMaster = 0.0;
	double MeanCurrent = 0.0;
	double VarianceMaster = 0.0;
	double VarianceCurrent = 0.0;
	double StdDevMaster = 0.0;
	double StdDevCurrent = 0.0;

	// HRRT and PETSPECT have two layers
	if ((SCANNER_HRRT == m_ScannerType) || (SCANNER_PETSPECT == m_ScannerType)) MaxLayer = 1;

	// initialize block status
	for (Layer = 0 ; Layer < 2 ; Layer++) for (Head = 0 ; Head < MAX_HEADS ; Head++) for (Block = 0 ; Block < MAX_BLOCKS ; Block++) {
		index = (((Layer * MAX_HEADS) + Head) + MAX_BLOCKS) + Block;
		m_BlockStatus[index] = 0;
	}

	// loop through layers
	for (Layer = 0 ; Layer <= MaxLayer ; Layer++) {

		// loop through heads
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// if the head is present
			if (1 == m_HeadPresent[Head]) {

				// pointers to data
				Data = m_CurrData[(Layer*MAX_HEADS)+Head];
				Master = m_MasterData[(Layer*MAX_HEADS)+Head];

				// if there is data for both
				if ((Data != NULL) && (Master != NULL)) {

					// find the maximum value in each data set
					MeanMaster = 0.0;
					MeanCurrent = 0.0;
					for (y = 0 ; y < (m_YBlocks*m_YCrystals) ; y++) for (x = 0 ; x < (m_XBlocks*m_XCrystals) ; x++) {
						Xtal = (y * 128) + x;
						MeanMaster += (double) Master[Xtal];
						MeanCurrent += (double) Data[Xtal];
					}
					MeanMaster /= (double) ((m_XBlocks * m_XCrystals) * (m_YBlocks * m_YCrystals));
					MeanCurrent /= (double) ((m_XBlocks * m_XCrystals) * (m_YBlocks * m_YCrystals));

					// determine scaling factor
					if (MeanCurrent != 0) {
						Scale = ((double) MeanMaster) / ((double) MeanCurrent);
					} else {
						Scale = 0.0;
					}

					// scale the current data
					for (y = 0 ; y < (m_YBlocks*m_YCrystals) ; y++) for (x = 0 ; x < (m_XBlocks*m_XCrystals) ; x++) {
						Xtal = (y * 128) + x;
						Data[Xtal] = (long) ((Scale * Data[Xtal]) + 0.5);
					}
				}

				// loop through the emission blocks
				for (Block = 0 ; Block < (m_XBlocks*m_YBlocks) ; Block++) {

					// identify block
					index = (((Layer * MAX_HEADS) + Head) * MAX_BLOCKS) + Block;

					// if no data
					if (NULL == Data) m_BlockStatus[index] += WARNING_NO_CURRENT_DATA;
					if (NULL == Master) m_BlockStatus[index] += WARNING_NO_MASTER_DATA;
					if ((NULL == Data) || (NULL == Master)) continue;

					// determine the means of the block
					MeanMaster = 0.0;
					MeanCurrent = 0.0;
					for (y = 0 ; y < m_YCrystals ; y++) for (x = 0 ; x < m_XCrystals ; x++) {
						Xtal = ((((Block / m_XBlocks) * m_YCrystals) + y) * 128) + ((Block % m_XBlocks) * m_XCrystals) + x;
						MeanMaster += (double) Master[Xtal];
						MeanCurrent += (double) Data[Xtal];
					}
					MeanMaster /= (double) (m_XCrystals * m_YCrystals);
					MeanCurrent /= (double) (m_XCrystals * m_YCrystals);

					// if there are no events, error
					if (MeanMaster == 0.0) m_BlockStatus[index] = ERROR_NO_MASTER_EVENTS;
					if (MeanCurrent == 0.0) m_BlockStatus[index] = ERROR_NO_CURRENT_EVENTS;
					if ((MeanMaster == 0.0) || (MeanCurrent == 0.0)) continue;

					// if the Mean is off by more than 5%
					Scale = MeanCurrent / MeanMaster;
					if ((Scale < 0.95) || (Scale > 1.05)) {
						m_BlockStatus[index] += WARNING_BLOCK_AVERAGE;
						continue;
					}

					// determine the variance of the block
					VarianceMaster = 0.0;
					VarianceCurrent = 0.0;
					for (y = 0 ; y < m_YCrystals ; y++) for (x = 0 ; x < m_XCrystals ; x++) {
						Xtal = ((((Block / m_XBlocks) * m_YCrystals) + y) * 128) + ((Block % m_XBlocks) * m_XCrystals) + x;
						VarianceMaster += pow((MeanMaster  - (double) Master[Xtal]),2);
						VarianceCurrent += pow((MeanCurrent  - (double) Data[Xtal]),2);
					}
					VarianceMaster /= (double) ((m_XCrystals * m_YCrystals) - 1);
					VarianceCurrent /= (double) ((m_XCrystals * m_YCrystals) - 1);

					// Standard deviation
					StdDevMaster = sqrt(VarianceMaster);
					StdDevCurrent = sqrt(VarianceCurrent);
				}
			}
		}

		// set head status
		for (Head = 0 ; Head < MAX_HEADS ; Head++) {

			// initialize
			m_HeadStatus[(Layer*MAX_HEADS)+Head] = 0;

			// if head is present
			if (1 == m_HeadPresent[Head]) {

				// generate head statistics
				ErrCount = 0;
				WarnCount = 0;
				for (Block = 0 ; Block < (m_XBlocks * m_YBlocks) ; Block++) {

					// identify block
					index = (((Layer * MAX_HEADS) + Head) * MAX_BLOCKS) + Block;

					// Count the number of errors
					if ((m_BlockStatus[index] & ERROR_STATUS) != 0) ErrCount++;
					if ((m_BlockStatus[index] & WARNING_STATUS) != 0) WarnCount++;
				}

				// if errors occurred, mark head as having error and include the number of errors
				if (ErrCount > 0) {
					m_HeadStatus[(Layer*MAX_HEADS)+Head] = (ErrCount << 16) | ERROR_STATUS;

				// warnings, include number of warnings
				} else if (WarnCount > 0) {
					m_HeadStatus[(Layer*MAX_HEADS)+Head] = WarnCount | WARNING_STATUS;
				}
			}
		}
	}
}

void CQCGUIDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// local variables
	long index = 0;
	long View = 0;
	long Head = 0;

	char Str[MAX_STR_LEN];

	// field edges
	long X1 = m_Display_X;
	long X2 = X1 + (4 * m_XBlocks * m_XCrystals);
	long X3 = X1 + 512 + 55;
	long X4 = X3 + 15;
	long Y1 = m_Display_Y;
	long Y2 = Y1 + (4 * m_YBlocks * m_YCrystals);
	long Y3 = Y1 - 20;

	// get current view
	View = m_ViewDrop.GetCurSel();

	// if status view and click was in block display
	if ((STATUS_VIEW == View) && (point.x >= X1) && (point.x < X2) && (point.y >= Y1) && (point.y < Y2)) {

	// gantry status
	} else if ((point.x >= X3) && (point.x < X4) && (point.y >= Y3) && (((point.y - Y3) % 20) < 15)) {

		// determine the head
		index = ((point.y - Y3) / 20) - 1;

		// gantry
		if (-1 == index) {

			// errors
			if (m_OverStatus < 0) {

				// build string
				sprintf(Str, "%d Heads Had Errors", abs(m_OverStatus));

			// warnings
			} else if (m_OverStatus > 0) {

				// build string
				sprintf(Str, "%d Heads Had Warnings", m_OverStatus);

			// ok
			} else {

				// build string
				sprintf(Str, "Gantry OK");
			}

			// put string in message label
			m_Message.SetWindowText(Str);

		// head
		} else if (index < m_NumHeads) {

			// head number
			Head = m_HeadIndex[index];
		}
	}
	
	CDialog::OnLButtonDown(nFlags, point);
}
