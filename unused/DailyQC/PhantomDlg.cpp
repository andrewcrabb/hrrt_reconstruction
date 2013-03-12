// PhantomDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DailyQC.h"
#include "PhantomDlg.h"
#include ".\phantomdlg.h"

// CPhantomDlg dialog

IMPLEMENT_DYNAMIC(CPhantomDlg, CDialog)
CPhantomDlg::CPhantomDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPhantomDlg::IDD, pParent)
	, m_Strength(0)
	, m_AssayTime(0)
	, m_sMRQC(_T(""))
	, m_dRadioMRQC(1)
{
}

CPhantomDlg::~CPhantomDlg()
{
}

void CPhantomDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UNIT, m_Unit);
	DDX_Control(pDX, IDC_DATE, m_AssayDate);
	DDX_Text(pDX, IDC_SOURCESTRENGTH, m_Strength);
	DDX_Text(pDX, IDC_MRQC, m_sMRQC);
	DDX_Radio(pDX, IDC_CREATEMRQC, m_dRadioMRQC);
}


BEGIN_MESSAGE_MAP(CPhantomDlg, CDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_CREATEMRQC, OnBnClickedCreatemrqc)
	ON_BN_CLICKED(IDC_SELECTMRQC, OnBnClickedSelectmrqc)
END_MESSAGE_MAP()


// CPhantomDlg message handlers

BOOL CPhantomDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	m_Unit.SetCurSel(1);
	m_AssayDate.SetTime(&m_AssayTime);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CPhantomDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	UpdateData(TRUE);
	if (m_Unit.GetCurSel() == 1)
		m_Strength = m_Strength;
	else
		m_Strength = m_Strength / 37;
	m_AssayDate.GetTime(m_AssayTime);
	CDialog::OnOK();
}

void CPhantomDlg::OnBnClickedBrowse()
{
	// TODO: Add your control notification handler code here
	CFileDialog dlg(TRUE,NULL,"*.dqc",OFN_HIDEREADONLY|OFN_READONLY,
			"DailyQC Files(*.dqc)|*.dqc");
	
	if (dlg.DoModal()==IDOK)
	{
		m_sMRQC = dlg.GetPathName();
		UpdateData(FALSE);
	}
	
}


void CPhantomDlg::OnBnClickedCreatemrqc()
{
	// TODO: Add your control notification handler code here
	m_dRadioMRQC = 0;
	m_sMRQC = "NULL";
	UpdateData(FALSE);
	CEdit * pMRQC = (CEdit *) GetDlgItem(IDC_MRQC);
	pMRQC->EnableWindow(FALSE);
	CButton * pBMRQC = (CButton*)GetDlgItem(IDC_BROWSE);
	pBMRQC->EnableWindow(FALSE);
}

void CPhantomDlg::OnBnClickedSelectmrqc()
{
	// TODO: Add your control notification handler code here
	m_dRadioMRQC = 1;
	UpdateData(FALSE);
	CEdit * pMRQC = (CEdit *) GetDlgItem(IDC_MRQC);
	pMRQC->EnableWindow(TRUE);
	CButton * pBMRQC = (CButton*)GetDlgItem(IDC_BROWSE);
	pBMRQC->EnableWindow(TRUE);
}
