// MainFrm.cpp : implementation of the CMainFrame class
//
/* HRRT User Community modifications
   29-aug-2008 : Added build date to window title
*/

#include "stdafx.h"
#include "CorrectRun.h"
#include "CorrectRunDoc.h"

#include "CorrectRunView.h"

#include "MainFrm.h"
#include <afxdlgs.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(IDD_USER_DIALOG, OnUpdateUserDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_PAGE,
	//	ID_INDICATOR_CAPS,
	//	ID_INDICATOR_NUM,
	//	ID_INDICATOR_SCRL,
	//ID_XY_VALUE,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CString str;
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style = WS_OVERLAPPED | WS_SYSMENU | WS_BORDER;
    cs.cy = 1000;//::GetSystemMetrics(SM_CYSCREEN) / 3; 
    cs.cx = 1200;//::GetSystemMetrics(SM_CXSCREEN) / 3;
	cs.style &= ~FWS_ADDTOTITLE;

	str = "Image Correction Software, Build: "+(CString) __DATE__+",  "+(CString) __TIME__;
	CMainFrame::SetTitle (str);
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

/*
void CMainFrame::OnUpdatePage(CCmdUI *pCmdUI) 
{
 
}
*/

void CMainFrame::OnUpdateUserDialog(CCmdUI* pCmdUI) 
{
	// Get a pointer to the frame
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CCorrectRunView *pView = (CCorrectRunView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CCorrectRunDoc* pDoc = pView->GetDocument();

	pCmdUI->Enable(); 

    pCmdUI->SetText( pDoc->m_MyStr ); 
	
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

