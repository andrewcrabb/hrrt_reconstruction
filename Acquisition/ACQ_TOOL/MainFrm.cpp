// MainFrm.cpp : implementation of the CMainFrame class
//
/* HRRT User Community modifications
   Authors: Merence Sibomana
   22-July-2009 : Add Build ID
*/


#include "stdafx.h"
#include "ScanIt.h"

#include "MainFrm.h"
#include "ScanItView.h"
#include "ScanItDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "serialbus.h"
extern CSerialBus pSerBus;


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI(ID_PROCESS_START, OnUpdateProcessStart)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_STOP, OnUpdateProcessStop)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_ABORT, OnUpdateProcessAbort)
	ON_COMMAND(ID_PROCESS_START, OnProcessStart)
	ON_COMMAND(ID_PROCESS_STOP, OnProcessStop)
	ON_COMMAND(ID_PROCESS_ABORT, OnProcessAbort)
	ON_UPDATE_COMMAND_UI(ID_APP_EXIT, OnUpdateAppExit)
	ON_UPDATE_COMMAND_UI(ID_CONTROL_CONFIG, OnUpdateControlConfig)
	ON_UPDATE_COMMAND_UI(ID_CONTROL_PATIENT, OnUpdateControlPatient)
	ON_UPDATE_COMMAND_UI(ID_DIAGNOSTIC_SHOW, OnUpdateDiagnosticShow)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateFileOpen)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_SETUP_COM, OnUpdateSetupCom)
	ON_UPDATE_COMMAND_UI(ID_SETUP_SYS_CONFIG, OnUpdateSetupSysConfig)
	ON_UPDATE_COMMAND_UI(ID_DOSAGE_INFO, OnUpdateDosageInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_ICON,
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
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	
	// add this code to keep track of recent files
	//PostMessage(WM_COMMAND,ID_FILE_MRU_FILE1,0); 
	//UpdateIcon(FALSE);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CString str;
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	
	str.Format("Scan Control Software, Build %s %s", __DATE__, __TIME__);
	CMainFrame::SetTitle(str);
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



void CMainFrame::UpdateIcon(bool a)
{

	// TODO: Add your command handler code here
	try
	{
		static int iconstate = 0;
		static int icons[] = {IDI_ICON1, IDI_ICON2, IDI_ICON3, IDI_ICON4, IDI_ICON5};
		//if(st.m_hWnd == NULL)

		//ASSERT(st.m_hWnd != NULL);
		if(st.m_hWnd == NULL)
		{ /* create it */
			 //c_StatusIcon.Create(&m_wndStatusBar, ID_INDICATOR_ICON, WS_VISIBLE | SS_ICON | SS_CENTERIMAGE);
			//CRect r,r1;
			int i;
			
			i = m_wndStatusBar.CommandToIndex(ID_INDICATOR_ICON);

			m_wndStatusBar.GetItemRect(i, &r);
			m_wndStatusBar.SetPaneText(i, "");
			
			if(r.IsRectEmpty())
			{
				m_wndStatusBar.GetWindowRect(&r1); // get parent width
				r.left = r1.right + 1;
				r.top =  r1.top;
				r.right = r1.right + 2;
				r.bottom = r1.bottom;
	
			}

			BOOL a = st.Create(NULL,WS_VISIBLE | SS_ICON | SS_CENTERIMAGE | WS_CHILD, r, &m_wndStatusBar, ID_INDICATOR_ICON);
			//BOOL result = CStatic::Create(NULL, WS_VISIBLE | SS_ICON | SS_CENTERIMAGE | WS_CHILD, r, &m_wndStatusBar, ID_INDICATOR_ICON);
			//CFont * f = m_wndStatusBar.GetFont();
			//SetFont(f);
			HICON icon = (HICON)::LoadImage(AfxGetInstanceHandle(),
							 MAKEINTRESOURCE(icons[4]),
							 IMAGE_ICON, 16, 16, LR_SHARED);
			st.SetIcon(icon);
			//f->DeleteObject();
			BOOL x =  DestroyIcon(icon);   // handle to icon to destroy

			//iconstate++;
		}
		else
		{ /* destroy it */
			HICON icon;
			if (a)
			{
				iconstate++;
				if(iconstate > 3)
					iconstate = 0;
				
				icon = (HICON)::LoadImage(AfxGetInstanceHandle(),
							 MAKEINTRESOURCE(icons[iconstate]),
							 IMAGE_ICON, 16, 16, LR_SHARED);
			}
			else
			{
				icon = (HICON)::LoadImage(AfxGetInstanceHandle(),
							 MAKEINTRESOURCE(icons[4]),
							 IMAGE_ICON, 16, 16, LR_SHARED);
			}
			st.SetIcon(icon);
			BOOL x =  DestroyIcon(icon);   // handle to icon to destroy
		} /* destroy it */

	}
	catch(_EXCEPTION_POINTERS &e)
	{
		EXCEPTION_RECORD a;
		a.ExceptionRecord = e.ExceptionRecord; 
	}

}


void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	//CWnd* wnd = (CWnd *)this;
	//UINT id = ::GetWindowLong(wnd->m_hWnd, GWL_ID);
	//CRect r;
	if(st.m_hWnd != NULL)
	{
		UINT id = ::GetWindowLong(st.m_hWnd, GWL_ID);
		int i = m_wndStatusBar.CommandToIndex(id);
		m_wndStatusBar.GetItemRect(i,&r);
		//r.left = cx - r.Width();
		st.SetWindowPos(&wndTop, r.left, r.top, r.Width(), r.Height(),0);
		
	}
	//wnd->SetWindowPos(&wndTop, r.left, r.top, r.Width(), r.Height(),0);

	
	
}



void CMainFrame::OnUpdateProcessStart(CCmdUI* pCmdUI) 
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();

	if(pDoc->m_Dn_Step <6)	
		pCmdUI->Enable(FALSE);
	else
		if(pDoc->m_RunFlag)
			pCmdUI->Enable(FALSE);
		else
			pCmdUI->Enable(TRUE);	
}


void CMainFrame::UpdateButtons()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view
	pFrame->SendMessage(WM_ACTIVATE, 1,0);

}

void CMainFrame::OnUpdateProcessStop(CCmdUI* pCmdUI) 
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();
	
	CMenu* pMenu = pFrame->GetSystemMenu(false);

	if(pDoc->m_Dn_Step <6)	
	{
		pCmdUI->Enable(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
	}
	else
	{
		if(!pDoc->m_RunFlag)
		{
			pCmdUI->Enable(FALSE);
			pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED );
		}
		else
		{
			pCmdUI->Enable(TRUE);
			pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		}
	}
	//BOOL x = pMenu->DestroyMenu();
}

void CMainFrame::OnUpdateProcessAbort(CCmdUI* pCmdUI) 
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();

	if(pDoc->m_Dn_Step <6)	
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(pDoc->m_RunFlag);

}


void CMainFrame::OnProcessStart() 
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();
	pDoc->m_RunFlag = TRUE;
	pDoc->m_Dn_Step = 6;
}

void CMainFrame::OnProcessStop() 
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();
	pDoc->StopScan();	
}

void CMainFrame::OnProcessAbort() 
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();
	pDoc->AbortScan();	
}

void CMainFrame::OnUpdateAppExit(CCmdUI* pCmdUI) 
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();
	CMenu* pMenu = pFrame->GetSystemMenu(false);

	if(pDoc->m_Dn_Step <6)	
	{
		pCmdUI->Enable(TRUE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);

	}
	else
	{
		if(pDoc->m_RunFlag)
		{
			pCmdUI->Enable(FALSE);
			pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		}
		else
		{
			pCmdUI->Enable(TRUE);	
			pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED );
		}
	}
	//BOOL x = pMenu->DestroyMenu();
}

void CMainFrame::OnUpdateControlConfig(CCmdUI* pCmdUI) 
{

	EnDisMenu(pCmdUI);
	
}


void CMainFrame::OnUpdateControlPatient(CCmdUI* pCmdUI) 
{
	EnDisMenu(pCmdUI);
}


void CMainFrame::EnDisMenu(CCmdUI *pCmdUI)
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view

	CScanItView *pView = (CScanItView *) pFrame->GetActiveView();
	//Get a pointer to the Document
	CScanItDoc* pDoc = pView->GetDocument();

	if(pDoc->m_RunFlag)
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(TRUE);	
}

void CMainFrame::OnUpdateDiagnosticShow(CCmdUI* pCmdUI) 
{
	EnDisMenu(pCmdUI);
	
}

void CMainFrame::OnUpdateFileOpen(CCmdUI* pCmdUI) 
{
	EnDisMenu(pCmdUI);
	
}

void CMainFrame::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	EnDisMenu(pCmdUI);
	
}

void CMainFrame::OnUpdateSetupCom(CCmdUI* pCmdUI) 
{
	EnDisMenu(pCmdUI);
	
}

void CMainFrame::OnUpdateSetupSysConfig(CCmdUI* pCmdUI) 
{
	EnDisMenu(pCmdUI);
}

void CMainFrame::OnUpdateDosageInfo(CCmdUI* pCmdUI) 
{
	//EnDisMenu(pCmdUI);
	
}
