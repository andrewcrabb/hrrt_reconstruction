// ScanTimeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DailyQC.h"
#include "ScanTimeDlg.h"


// CScanTimeDlg dialog

IMPLEMENT_DYNAMIC(CScanTimeDlg, CDialog)
CScanTimeDlg::CScanTimeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScanTimeDlg::IDD, pParent)
	, m_ScanDate(0)
	, m_ScanTime(0)
	, m_ScanDuration(0)
{
}

CScanTimeDlg::~CScanTimeDlg()
{
}

void CScanTimeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_DateTimeCtrl(pDX, IDC_SCANDATE, m_ScanDate);
	DDX_DateTimeCtrl(pDX, IDC_SCANTIME, m_ScanTime);
	DDX_Text(pDX, IDC_SCANDUR, m_ScanDuration);
}


BEGIN_MESSAGE_MAP(CScanTimeDlg, CDialog)
END_MESSAGE_MAP()


// CScanTimeDlg message handlers

void CScanTimeDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	UpdateData(TRUE);
	CDialog::OnOK();
}
