// MRQCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DailyQC.h"
#include "MRQCDlg.h"


// CMRQCDlg dialog

IMPLEMENT_DYNAMIC(CMRQCDlg, CDialog)
CMRQCDlg::CMRQCDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMRQCDlg::IDD, pParent)
	, m_sFileName(_T(""))
{
}

CMRQCDlg::~CMRQCDlg()
{
}

void CMRQCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MRQC_EDIT, m_sFileName);
}


BEGIN_MESSAGE_MAP(CMRQCDlg, CDialog)
	ON_BN_CLICKED(IDC_MRQC_BROWSE, OnBnClickedMrqcBrowse)
END_MESSAGE_MAP()


// CMRQCDlg message handlers

void CMRQCDlg::OnBnClickedMrqcBrowse()
{
	// TODO: Add your control notification handler code here
	CFileDialog dlg(TRUE,NULL,"*.dqc",OFN_HIDEREADONLY|OFN_READONLY,
			"DailyQC Files(*.dqc)|*.dqc");
	
	if (dlg.DoModal()==IDOK)
	{
		m_sFileName = dlg.GetPathName();
		UpdateData(FALSE);
	}
}
