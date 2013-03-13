//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Archiving Graphical User Interface
// 
// Name:			LoadDlg.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	August 9, 2002
// 
// Description:		This component is the Detector Setup/Utilities Archiver.  With it, a
//					user is able to select an archive for full or partial restoration or to
//					create a new archive of the current setup information.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

// LoadDlg.cpp : implementation file
//

#include "stdafx.h"
#ifndef INCLUDE_DUCOM_I
#include "DSGUI.h"
#include "DSGUIDlg.h"
#else
#include "DUGUI.h"
#include "DUGUIDlg.h"
#endif
#include "LoadDlg.h"
#include "DUCom.h"
#ifndef INCLUDE_DUCOM_I
#include "DUCom_i.c"
#endif
#include "DHI_Constants.h"

#include "gantryModelClass.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define 	WM_USER_POST_UPDATE			(WM_USER+3)
#define		WM_USER_ARCHIVE_COMPLETE	(WM_USER+4)

static long m_NumConfigurations;
static long m_NumHeads;
static long m_CurrConfig;
static long m_Init;
static long m_Abort;
static long m_ArchivePercent;

static long m_HeadIndex[MAX_HEADS];
static long m_HeadPresent[MAX_HEADS];
static long m_HeadSel[MAX_HEADS];
static long m_HeadStatus[MAX_HEADS];

static unsigned char m_Archive[MAX_STR_LEN];
static unsigned char m_StageStr[MAX_STR_LEN];

static IDUMain* pDUMain;

IDUMain* Get_Load_DU_Ptr(CErrorEventSupport *pErrSup);

static HANDLE m_MyWin;

CErrorEventSupport* pArchiveErrEvtSup;
CErrorEventSupport* pArchiveErrEvtSupThread;

// thread subroutines
void LoadThread(void *dummy);
void SaveThread(void *dummy);

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg dialog


CLoadDlg::CLoadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoadDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoadDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CLoadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoadDlg)
	DDX_Control(pDX, IDSAVE, m_SaveButton);
	DDX_Control(pDX, IDLOAD, m_LoadButton);
	DDX_Control(pDX, IDDELETE, m_DeleteButton);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDC_MESSAGE_LABEL, m_Message);
	DDX_Control(pDX, IDC_LOAD_CONFIG, m_LoadConfig);
	DDX_Control(pDX, IDC_ARCHIVE_LIST, m_ChooseArchive);
	DDX_Control(pDX, IDC_HEAD_LIST, m_ChooseHead);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CLoadDlg)
	ON_CBN_SELCHANGE(IDC_LOAD_CONFIG, OnSelchangeLoadConfig)
	ON_LBN_SELCHANGE(IDC_HEAD_LIST, OnSelchangeHeadList)
	ON_BN_CLICKED(IDLOAD, OnLoad)
	ON_BN_CLICKED(IDDELETE, OnDelete)
	ON_BN_CLICKED(IDABORT, OnAbort)
	ON_BN_CLICKED(IDSAVE, OnSave)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP

	// User define message maps
	ON_MESSAGE(WM_USER_POST_UPDATE, OnPostUpdate)
	ON_MESSAGE(WM_USER_ARCHIVE_COMPLETE, OnArchiveComplete)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

BOOL CLoadDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	long i = 0;
	long Status = 0;

	char Str[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// instantiate a Gantry Object Model object
	CGantryModel model;
	
	// initialize flags
	m_Abort = 0;
	m_Init = 0;

#ifndef ECAT_SCANNER
	// instantiate an ErrorEventSupport object
	pArchiveErrEvtSup = new CErrorEventSupport(false, false);

	// if error support not establisted, note it in log
	if (pArchiveErrEvtSup == NULL) if (pArchiveErrEvtSup->InitComplete(hr) == false) {
		delete pArchiveErrEvtSup;
		pArchiveErrEvtSup = NULL;
	}
#endif

	// set gantry model to model number held in register if none held, use the 393
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) model.setModel(393);

	// initialize head variables
	for (i = 0 ; i < MAX_HEADS ; i++) m_HeadPresent[i] = 0;
	for (i = 0 ; i < MAX_HEADS ; i++) m_HeadSel[i] = 0;

// ECAT (Ring) Scanners
#ifdef ECAT_SCANNER

	// heads
	m_NumHeads = model.buckets();
	for (i = 0 ; i < m_NumHeads ; i++) {
		m_HeadPresent[i] = 1;
		m_HeadIndex[i] = i;
		sprintf(Str, "Bucket %d", i);
		m_ChooseHead.InsertString(i, Str);
		m_ChooseHead.SetSel(i, TRUE);
	}

	// configurations
	m_NumConfigurations = 1;
	m_LoadConfig.InsertString(0, "PET");

#else
	pConf Config;
	pHead Head;

	// determine which heads are available
	Head = model.headInfo();
	m_NumHeads = model.nHeads();
	for (i = 0 ; i < m_NumHeads ; i++) {
		m_HeadPresent[Head[i].address] = 1;
		m_HeadIndex[i] = Head[i].address;
		sprintf(Str, "Head %d", Head[i].address);
		m_ChooseHead.InsertString(i, Str);
		m_ChooseHead.SetSel(i, TRUE);
	}

	// Configurations
	m_NumConfigurations = model.nConfig();
	Config = model.configInfo();
	for (i = 0 ; i < m_NumConfigurations ; i++) m_LoadConfig.InsertString(i, Config[i].name);
#endif

	// initialize configuration
	m_LoadConfig.SetCurSel(0);
	m_CurrConfig = 0;
	m_LoadConfig.ShowWindow(SW_HIDE);

	// Create an instance of detector utilities
	if (Status == 0) {
		pDUMain = Get_Load_DU_Ptr(pArchiveErrEvtSup);
		if (pDUMain == NULL) {
			Add_Error(pArchiveErrEvtSup, "LoadDlg:OnInitDialog", "Failed to Get DU Pointer", 0);
		} else {
			m_Init = 1;
		}
	}

	// update the archive list
	UpDateArchive();

	// keep track of the window handle
	m_MyWin = m_hWnd;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		UpdateArchive
// Purpose:		updates the list of archives displayed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::UpDateArchive()
{
	// local variables
	long i = 0;
	long Status = 0;
	long NumFound = 0;

	unsigned char *List;

	HRESULT hr = S_OK;

	// if not initialized, then don't try
	if (m_Init != 1) return;

	// loop through heads determining which are selected
	for (i = 0 ; i < m_NumHeads ; i++) {
		if (m_ChooseHead.GetSel(i) != 0) {
			m_HeadSel[m_HeadIndex[i]] = 1;
		} else {
			m_HeadSel[m_HeadIndex[i]] = 0;
		}
	}

	// get the list of archives
	hr = pDUMain->List_Setup_Archives(m_CurrConfig, m_HeadSel, &NumFound, &List, &Status);
	if (FAILED(hr)) Status = Add_Error(pArchiveErrEvtSup, "LoadDlg:OnInitDialog", "Method Failed (pDUMain->List_Setup_Archives), HRESULT", hr);

	// Clear the list
	while (m_ChooseArchive.DeleteString(0) != -1);

	// loop through filling list
	for (i = 0 ; i < NumFound ; i++) m_ChooseArchive.InsertString(i, (char *) &List[i*MAX_STR_LEN]);

	// Clear the selection
	m_ChooseArchive.SetCurSel(-1);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeLoadConfig
// Purpose:		update the archive list with repect to the configurations
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::OnSelchangeLoadConfig() 
{
	// get the new configuration
	m_CurrConfig = m_LoadConfig.GetCurSel();

	// reset archive list
	UpDateArchive();	
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelchangeHeadList
// Purpose:		update the archive list with repect to the head(s) selected
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::OnSelchangeHeadList() 
{
	// reset archive list
	UpDateArchive();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnLoad
// Purpose:		starts load of a selected archive
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::OnLoad()
{
	// local variables
	long Sel = 0;

	HANDLE hLoadThread;

	// Determine Selection
	Sel = m_ChooseArchive.GetCurSel();

	// if current selection is empty
	if (Sel == -1) {

		// display message
		m_Message.SetWindowText("Load: No Archive Selected");
		return;
	}

	// Desensitize
	m_LoadButton.EnableWindow(FALSE);
	m_SaveButton.EnableWindow(FALSE);
	m_DeleteButton.EnableWindow(FALSE);
	m_CancelButton.EnableWindow(FALSE);
	m_ChooseArchive.EnableWindow(FALSE);
	m_ChooseHead.EnableWindow(FALSE);
	m_LoadConfig.EnableWindow(FALSE);

	// break out archive name
	m_ChooseArchive.GetText(Sel, (char *) m_Archive);

	// start the load thread
	hLoadThread = (HANDLE) _beginthread(LoadThread, 0, NULL);

	// failed to create thread
	if (hLoadThread == NULL) {

		// reactivate controls
		m_LoadButton.EnableWindow(TRUE);
		m_SaveButton.EnableWindow(TRUE);
		m_DeleteButton.EnableWindow(TRUE);
		m_CancelButton.EnableWindow(TRUE);
		m_ChooseArchive.EnableWindow(TRUE);
		m_ChooseHead.EnableWindow(TRUE);
		m_LoadConfig.EnableWindow(TRUE);

		// display informational message
		m_Message.SetWindowText("Failed To Start Load");
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDelete
// Purpose:		delete the selected archive
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::OnDelete() 
{
	// local variables
	long Sel = 0;
	long Status = 0;

	unsigned char Archive[MAX_STR_LEN];

	HRESULT hr = S_OK;

	// Determine Selection
	Sel = m_ChooseArchive.GetCurSel();

	// if current selection is empty
	if (Sel == -1) {

		// display message
		m_Message.SetWindowText("Delete: No Archive Selected");
		return;
	}

	// break out archive name
	m_ChooseArchive.GetText(Sel, (char *) Archive);

	// tell the COM Server to load selected Archive
	hr = pDUMain->Remove_Setup_Archive(Archive, &Status);
	if (FAILED(hr)) Status = Add_Error(pArchiveErrEvtSup, "LoadDlg:OnInitDialog", "Method Failed (pDUMain->Remove_Setup_Archives), HRESULT", hr);

	// reset archive list
	UpDateArchive();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnAbort
// Purpose:		aborts an archive save or load
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::OnAbort() 
{
	// local variables
	long Status = 0;

	HRESULT hr = S_OK;

	// set abort flag
	m_Abort = 1;

	// if instance created successfully
	if (SUCCEEDED(hr)) {

		// abort setup
		hr = pDUMain->Archive_Abort();
		if (FAILED(hr)) Status = Add_Error(pArchiveErrEvtSup, "LoadDlg:OnInitDialog", "Method Failed (pDUMain->Archive_Abort), HRESULT", hr);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSave
// Purpose:		starts save of a new archive based on configuration and head(s) selected
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::OnSave() 
{
	// local variables
	HANDLE hSaveThread;

	// Desensitize
	m_LoadButton.EnableWindow(FALSE);
	m_SaveButton.EnableWindow(FALSE);
	m_DeleteButton.EnableWindow(FALSE);
	m_CancelButton.EnableWindow(FALSE);
	m_ChooseArchive.EnableWindow(FALSE);
	m_ChooseHead.EnableWindow(FALSE);
	m_LoadConfig.EnableWindow(FALSE);

	// start the load thread
	hSaveThread = (HANDLE) _beginthread(SaveThread, 0, NULL);

	// failed to create thread
	if (hSaveThread == NULL) {

		// reactivate controls
		m_LoadButton.EnableWindow(TRUE);
		m_SaveButton.EnableWindow(TRUE);
		m_DeleteButton.EnableWindow(TRUE);
		m_CancelButton.EnableWindow(TRUE);
		m_ChooseArchive.EnableWindow(TRUE);
		m_ChooseHead.EnableWindow(TRUE);
		m_LoadConfig.EnableWindow(TRUE);

		// display informational message
		m_Message.SetWindowText("Failed To Start Save");
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		LoadThread
// Purpose:		loads selected archive
// Arguments:	*dummy	- void
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	bug fix - changed "=" to "==" in condition statement
/////////////////////////////////////////////////////////////////////////////

void LoadThread(void *dummy)
{
	// local variables
	long i = 0;
	long Status = 0;

	IDUMain* pDU;

	HRESULT hr = S_OK;

#ifdef ECAT_SCANNER
	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	// instantiate an ErrorEventSupport object
	pArchiveErrEvtSup = new CErrorEventSupport(true, false);

	// if error support not establisted, note it in log
	if (pArchiveErrEvtSupThread != NULL) if (pArchiveErrEvtSupThread->InitComplete(hr) == false) {
		delete pArchiveErrEvtSupThread;
		pArchiveErrEvtSupThread = NULL;
	}
#endif

	// Create an instance of detector utilities
	if (Status == 0) {
		pDU = Get_Load_DU_Ptr(pArchiveErrEvtSupThread);
		if (pDU == NULL) Status = Add_Error(pArchiveErrEvtSupThread, "LoadThread", "Failed to Get DU Pointer", 0);
	}

	// if instance created successfully
	if (Status == 0) {

		// Setup Head(s)
		for (i = 0 ; i < MAX_HEADS ; i++) m_HeadStatus[i] = 0;
		hr = pDU->Load_Setup_Archive(m_Archive, m_HeadSel, m_HeadStatus);
		if (FAILED(hr)) Status = Add_Error(pArchiveErrEvtSupThread, "LoadThread", "Method Failed (pDU->Load_Setup_Archive), HRESULT", hr);

		// loop for proogress
		m_ArchivePercent = 0;
		while ((m_ArchivePercent < 100) && (Status == 0)) {

			// wait 5 seconds between polling
			Sleep(5000);

			// poll for progress
			hr = pDU->Archive_Progress(&m_ArchivePercent, m_StageStr, m_HeadStatus);
			if (FAILED(hr)) Status = Add_Error(pArchiveErrEvtSupThread, "LoadThread", "Method Failed (pDU->Archive_Progress), HRESULT", hr);

			// update status indicators
			if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_UPDATE, 0, 0);
		}

		// Release pointers
		if (pDU != NULL) pDU->Release();
#ifndef ECAT_SCANNER
		if (pArchiveErrEvtSupThread != NULL) delete pArchiveErrEvtSupThread;
#endif
	}

	// Release COM
	::CoUninitialize();

	// string
	if (m_Abort == 1) {
		sprintf((char *) m_StageStr, "Archive Aborted");
	} else {
		sprintf((char *) m_StageStr, "Archive Complete");
	}

	// All Complete
	PostMessage((HWND) m_MyWin, WM_USER_POST_UPDATE, 0, 0);

	// let the gui know it is done
	PostMessage((HWND)m_MyWin, WM_USER_ARCHIVE_COMPLETE, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		SaveThread
// Purpose:		saves selected archive
// Arguments:	*dummy	- void
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				19 Oct 04	T Gremillion	bug fix - changed "=" to "==" in condition statement
/////////////////////////////////////////////////////////////////////////////

void SaveThread(void *dummy)
{
	// local variables
	long i = 0;
	long Status = 0;

	IDUMain* pDU;

	HRESULT hr = S_OK;

#ifdef ECAT_SCANNER
	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	// instantiate an ErrorEventSupport object
	pArchiveErrEvtSup = new CErrorEventSupport(true, false);

	// if error support not establisted, note it in log
	if (pArchiveErrEvtSupThread != NULL) if (pArchiveErrEvtSupThread->InitComplete(hr) == false) {
		delete pArchiveErrEvtSupThread;
		pArchiveErrEvtSupThread = NULL;
	}
#endif

	// Create an instance of detector utilities
	if (Status == 0) {
		pDU = Get_Load_DU_Ptr(pArchiveErrEvtSupThread);
		if (pDU == NULL) Status = Add_Error(pArchiveErrEvtSupThread, "SaveThread", "Failed to Get DU Pointer", 0);
	}

	// if instance created successfully
	if (Status == 0) {

		// Setup Head(s)
		for (i = 0 ; i < MAX_HEADS ; i++) m_HeadStatus[i] = 0;
		hr = pDU->Save_Setup_Archive(m_CurrConfig, m_HeadSel, m_HeadStatus);
		if (FAILED(hr)) Status = Add_Error(pArchiveErrEvtSupThread, "SaveThread", "Method Failed (pDU->Save_Setup_Archive), HRESULT", hr);

		// loop for proogress
		m_ArchivePercent = 0;
		while ((m_ArchivePercent < 100) && (Status == 0)) {

			// wait 5 seconds between polling
			Sleep(5000);

			// poll for progress
			hr = pDU->Archive_Progress(&m_ArchivePercent, m_StageStr, m_HeadStatus);
			if (FAILED(hr)) Status = Add_Error(pArchiveErrEvtSupThread, "SaveThread", "Method Failed (pDU->Archive_Progress), HRESULT", hr);

			// update status indicators
			if (Status == 0) PostMessage((HWND) m_MyWin, WM_USER_POST_UPDATE, 0, 0);
		}

		// Release pointers
		if (pDU != NULL) pDU->Release();
#ifndef ECAT_SCANNER
		if (pArchiveErrEvtSupThread != NULL) delete pArchiveErrEvtSupThread;
#endif
	}

	// Release COM
	::CoUninitialize();

	// string
	if (m_Abort == 1) {
		sprintf((char *) m_StageStr, "Archive Aborted");
	} else {
		sprintf((char *) m_StageStr, "Archive Complete");
	}

	// All Complete
	PostMessage((HWND) m_MyWin, WM_USER_POST_UPDATE, 0, 0);

	// let the gui know it is done
	PostMessage((HWND)m_MyWin, WM_USER_ARCHIVE_COMPLETE, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPostUpdate
// Purpose:		callback to Update progress bar during archive operation 
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

LRESULT CLoadDlg::OnPostUpdate(WPARAM wp, LPARAM lp)
{
	// local variable
	char Message[MAX_STR_LEN];

	// update status
	sprintf(Message, "%d%% %s", m_ArchivePercent, m_StageStr);
	m_Message.SetWindowText(Message);

	// requires return value
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnArchiveComplete
// Purpose:		callback to signal archive operation completion 
// Arguments:	wp	-	WPARAM = (required)
//				lp	-	LPARAM = (required)
// Returns:		LRESULT
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

LRESULT CLoadDlg::OnArchiveComplete(WPARAM wp, LPARAM lp)
{
	// local variables
	long i = 0;
	long HeadFailed = 0;

	char Message[MAX_STR_LEN];

	// re-enable exit, reset and head list
	m_LoadButton.EnableWindow(TRUE);
	m_SaveButton.EnableWindow(TRUE);
	m_DeleteButton.EnableWindow(TRUE);
	m_CancelButton.EnableWindow(TRUE);
	m_ChooseArchive.EnableWindow(TRUE);
	m_ChooseHead.EnableWindow(TRUE);
	m_LoadConfig.EnableWindow(TRUE);

	// message label
	if (m_Abort == 0) {
		sprintf(Message, "Archive Complete:");

		// did any heads fail
		for (i = 0 ; (i < MAX_HEADS) && (HeadFailed == 0) ; i++) if ((m_HeadSel[i] == 1) && (m_HeadStatus[i] != 0)) HeadFailed = 1;

		// head failed, add to message
		if (HeadFailed == 1) {

			// Failed Message
			sprintf(Message, "%s Failed Heads", Message);

			// add head numbers
			for (i = 0 ; i < MAX_HEADS ; i++) if ((m_HeadSel[i] == 1) && (m_HeadStatus[i] != 0)) sprintf(Message, "%s %d", Message, i);
		}

	} else {

		// set message
		sprintf(Message, "Archive Aborted");

		// clear flag
		m_Abort = 0;
	}

	// display message
	m_Message.SetWindowText(Message);

	// update archive list

	// reset archive list
	UpDateArchive();	

	// requires return value
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDestroy
// Purpose:		Steps to be performed when the GUI is closed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CLoadDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// release pointers
	if (pDUMain != NULL) pDUMain->Release();
#ifndef ECAT_SCANNER
	if (pArchiveErrEvtSup != NULL) delete pArchiveErrEvtSup;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Load_DU_Ptr
// Purpose:		retrieve a pointer to the Detector Utilities COM server (DUCOM)
// Arguments:	*pErrSup	-	CErrorEventSupport = Error handler
// Returns:		IDUMain* pointer to DUCOM
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

IDUMain* Get_Load_DU_Ptr(CErrorEventSupport *pErrSup)
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
	if (FAILED(hr)) Add_Error(pErrSup, "Get_Load_DU_Ptr", "Failed ::CoCreateInstanceEx(CLSID_DUMain, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (IDUMain*) mqi[0].pItf;

	return Ptr;
}
