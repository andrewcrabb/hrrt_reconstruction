// DosageInfo.cpp : implementation file
//

#include "stdafx.h"
#include "scanit.h"
#include "DosageInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDosageInfo dialog


CDosageInfo::CDosageInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CDosageInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDosageInfo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDosageInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDosageInfo)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_COMBO_STRENGTH, m_cmbStrength);
	DDX_Control(pDX, IDC_COMBO_ISO, m_Iso_Dose);
	DDX_Text(pDX, IDC_EDIT_STRENGTH, m_Strength);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDosageInfo, CDialog)
	//{{AFX_MSG_MAP(CDosageInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDosageInfo message handlers

void CDosageInfo::OnOK() 
{
	UpdateData(TRUE);
	pDoc->m_Pt_Dose = m_Iso_Dose.GetCurSel();	
	pDoc->m_Strength_Type = m_cmbStrength.GetCurSel();
	pDoc->m_Strength = m_Strength;

	CDialog::OnOK();
}

BOOL CDosageInfo::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString str;

	pDoc = dynamic_cast<CScanItDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	
	for(int i=0;i<17;i++)
	{
		str.Format("%s",doses[i]);
		m_Iso_Dose.AddString(str);
	}
	m_Iso_Dose.SetCurSel(pDoc->m_Pt_Dose);
	m_cmbStrength.SetCurSel(pDoc->m_Strength_Type);
	m_Strength = pDoc->m_Strength;
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
