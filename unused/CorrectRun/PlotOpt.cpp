// PlotOpt.cpp : implementation file
//

#include "stdafx.h"
#include "CorrectRun.h"
#include "CorrectRunDoc.h"
#include "CorrectRunView.h"

#include "PlotOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlotOpt dialog


CPlotOpt::CPlotOpt(CWnd* pParent /*=NULL*/)
	: CDialog(CPlotOpt::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPlotOpt)
	m_ClrScale = -1;
	m_plane = 0;
	m_Radius = 0.0f;
	m_DisType = -1;
	m_LinLog = -1;
	m_Edge = 0.0f;
	m_InWord = 0.0f;
	//}}AFX_DATA_INIT
}


void CPlotOpt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlotOpt)
	DDX_Control(pDX, IDC_CHECK2, m_FindEdge);
	DDX_Control(pDX, IDC_CHECK1, m_HighLight);
	DDX_Radio(pDX, IDC_RADIO_SCALE, m_ClrScale);
	DDX_Text(pDX, IDC_EDIT1, m_plane);
	DDV_MinMaxInt(pDX, m_plane, 0, 206);
	DDX_Text(pDX, IDC_EDIT2, m_Radius);
	DDV_MinMaxFloat(pDX, m_Radius, 0.f, 156.f);
	DDX_Radio(pDX, IDC_GRAPH_TYPE, m_DisType);
	DDX_Radio(pDX, IDC_LIN_LOG, m_LinLog);
	DDX_Text(pDX, IDC_EDIT5, m_Edge);
	DDX_Text(pDX, IDC_EDIT8, m_InWord);
	DDV_MinMaxFloat(pDX, m_InWord, 0.f, 100.f);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlotOpt, CDialog)
	//{{AFX_MSG_MAP(CPlotOpt)
	ON_BN_CLICKED(IDC_GRAPH_TYPE, OnGraphType)
	ON_BN_CLICKED(IDC_GRAPH_TYPE2, OnGraphType2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlotOpt message handlers

BOOL CPlotOpt::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_pDoc = dynamic_cast<CCorrectRunDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	
	m_pView = dynamic_cast<CCorrectRunView*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveView ());

	m_HighLight.SetCheck(m_pDoc->m_HighLight);
	m_FindEdge.SetCheck(m_pDoc->m_FindEdge);
	m_ClrScale = m_pDoc->m_ClrSelect;
	m_plane = m_pDoc->m_plane;
	m_Radius = m_pDoc->m_Radius;
	m_DisType = m_pView->m_DisType;
	m_Edge = m_pDoc->m_Edge ;
	m_InWord = m_pDoc->m_InPer ;

	if (m_pView->m_DisType)
		OnGraphType2();
	else
		OnGraphType();

	m_LinLog = m_pView->m_LnLg;
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPlotOpt::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

	m_pDoc->m_ClrSelect = m_ClrScale;
	m_pDoc->m_HighLight = m_HighLight.GetCheck();
	m_pDoc->m_FindEdge = m_FindEdge.GetCheck();
	m_pDoc->m_Radius = m_Radius;	
	m_pDoc->m_plane = m_plane;	
	m_pDoc->m_Edge = m_Edge;
	m_pDoc->m_InPer = m_InWord;

	m_pView->DefineColors();
	m_pView->m_DisType = m_DisType;
	
	m_pView->m_LnLg = m_LinLog;
	
	if (m_pDoc->m_GotData && m_pDoc->m_ThreadDone)
	{
		
		m_pDoc->CalPlaneAvg();
		// Refresh the data to the view
		m_pView->Invalidate();
	}

	m_pView->Invalidate();
	m_pDoc->SaveConfigFile();

	CDialog::OnOK();
}

void CPlotOpt::OnGraphType() 
{
	CWnd *pwnd = (CWnd *)GetDlgItem(IDC_LIN_LOG);
	pwnd->ShowWindow(SW_HIDE);

	pwnd = (CWnd *)GetDlgItem(IDC_LIN_LOG2);
	pwnd->ShowWindow(SW_HIDE);

	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_SCALE);
	pwnd->ShowWindow(SW_SHOW);

	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_SCALE2);
	pwnd->ShowWindow(SW_SHOW);	

	
}

void CPlotOpt::OnGraphType2() 
{
	CWnd *pwnd = (CWnd *)GetDlgItem(IDC_LIN_LOG);
	pwnd->ShowWindow(SW_SHOW);

	pwnd = (CWnd *)GetDlgItem(IDC_LIN_LOG2);
	pwnd->ShowWindow(SW_SHOW);

	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_SCALE);
	pwnd->ShowWindow(SW_HIDE);

	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_SCALE2);
	pwnd->ShowWindow(SW_HIDE);	

	
}
