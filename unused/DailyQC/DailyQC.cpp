// DailyQC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DailyQC.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "DailyQCDoc.h"
#include "DailyQCView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDailyQCApp

BEGIN_MESSAGE_MAP(CDailyQCApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


// CDailyQCApp construction

CDailyQCApp::CDailyQCApp()
: m_strL64FileName(_T(""))
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CDailyQCApp object

CDailyQCApp theApp;

// CDailyQCApp initialization

BOOL CDailyQCApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	// add DocTemplate for dqc file - dailyqc file
	pDocTemplate = new CMultiDocTemplate(IDR_DailyQCTYPE,
		RUNTIME_CLASS(CDailyQCDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CDailyQCView));
	AddDocTemplate(pDocTemplate);
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();
	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();
	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CDailyQCApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CDailyQCApp message handlers


// function added by Dongming Hu to start HRRT DailyQC without
// new document
BOOL CDailyQCApp::ProcessShellCommand(CCommandLineInfo& rCmdInfo)
{
	BOOL bResult = TRUE;
	switch (rCmdInfo.m_nShellCommand)
	{
		// when start program, no new document
	case CCommandLineInfo::FileNew:
		break;

		// If we've been asked to open a file, call OpenDocumentFile()
	case CCommandLineInfo::FileOpen:
		if (!OpenDocumentFile(rCmdInfo.m_strFileName))
			bResult = FALSE;
		break;

		// for others, use CWinApp function
	default:
		bResult = CWinApp::ProcessShellCommand(rCmdInfo);

	}
	return bResult;
}

CDocument* CDailyQCApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
	// TODO: Add your specialized code here and/or call the base class
	// first check if it is list mode file
	// if it is, load list mode file and new a daily qc document
	if (CString (lpszFileName).Right(4).MakeLower() == ".l64")
	{
		m_strL64FileName = lpszFileName;
		// todo: add code to new doc
		OnFileNew();
		return NULL;
	}
	// if it is dqc doc, then open it
	if (CString (lpszFileName).Right(4).MakeLower() == ".dqc")
	{
		return CWinApp::OpenDocumentFile(lpszFileName);
	}
	// otherwise propmt an error
	else
		AfxMessageBox("Can not open " +
			CString (lpszFileName) +
			".\nThis file format is not supported.");

	return NULL;
}
