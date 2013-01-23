// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DailyQC.h"
#include "ProgressDlg.h"


// CProgressDlg dialog

IMPLEMENT_DYNAMIC(CProgressDlg, CDialog)
CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressDlg::IDD, pParent)
	, m_dProcess(0)
	, pThread(NULL)
{
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_ProgCtrl);
	DDX_Control(pDX, IDCANCEL, m_button);
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
END_MESSAGE_MAP()


// CProgressDlg message handlers

BOOL CProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	m_ProgCtrl.SetRange(0,100);
	m_ProgCtrl.SetPos(0);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CProgressDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	m_dProcess = PROCESS_CANCEL;
	// wait for thread to be terminated
	WaitForSingleObject(pThread->m_hThread,500);
	CDialog::OnCancel();
}


