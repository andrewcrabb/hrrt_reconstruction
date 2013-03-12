// GraphProp.cpp : implementation file
//

#include "stdafx.h"
#include "ScanIt.h"
#include "scanitview.h"

#include "GraphProp.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraphProp dialog


CGraphProp::CGraphProp(CWnd* pParent /*=NULL*/)
	: CDialog(CGraphProp::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGraphProp)
	m_Ymax_Man = 0.0f;
	m_Ymin_Man = 0.0f;
	m_LinLog = -1;
	m_Move = 0;
	//}}AFX_DATA_INIT
}


void CGraphProp::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGraphProp)
	DDX_Control(pDX, IDC_CHECK_MOVE, m_Chk_Move);
	DDX_Control(pDX, IDC_SPIN2, m_Spn_Clr_Trues);
	DDX_Control(pDX, IDC_SPIN1, m_Spn_Clr_Rndms);
	DDX_Control(pDX, IDC_EDIT_CLR_RNDMS, m_Clr_Randoms);
	DDX_Control(pDX, IDC_EDIT_CLR_PRMPTS, m_Clr_Trues);
	DDX_Control(pDX, IDC_CHECK_TRUES, m_Chk_Trs);
	DDX_Control(pDX, IDC_CHECK_RANDOMS, m_Chk_Rndms);
	DDX_Control(pDX, IDC_SPIN_YMAX, m_Ymax_Spn);
	DDX_Control(pDX, IDC_EDIT_YMAX_EXP, m_Ymax_Exp);
	DDX_Control(pDX, IDC_EDIT_YMIN_EXP, m_Ymin_Exp);
	DDX_Control(pDX, IDC_SPIN_YMIN, m_Ymin_Spn);
	DDX_Text(pDX, IDC_EDIT_YMAX_MAN, m_Ymax_Man);
	DDV_MinMaxFloat(pDX, m_Ymax_Man, 0.f, 9.9f);
	DDX_Text(pDX, IDC_EDIT_YMIN_MAN, m_Ymin_Man);
	DDV_MinMaxFloat(pDX, m_Ymin_Man, 0.f, 9.9f);
	DDX_Radio(pDX, IDC_RADIO_LIN, m_LinLog);
	DDX_Text(pDX, IDC_EDIT_MOVE, m_Move);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGraphProp, CDialog)
	//{{AFX_MSG_MAP(CGraphProp)
	ON_BN_CLICKED(IDC_RADIO_LIN, OnRadioLin)
	ON_BN_CLICKED(IDC_RADIO_LOG, OnRadioLog)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, OnDeltaposSpin1)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN2, OnDeltaposSpin2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphProp message handlers

BOOL CGraphProp::OnInitDialog() 
{
	CDialog::OnInitDialog();
	int exp,a;
	float man;

	//pParent = dynamic_cast<CScanItView*>(GetParent());	
	//pParent = dynamic_cast<CScanItDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	
	pParent = dynamic_cast<CScanItView*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveView ());
	
	m_Ymin_Spn.SetRange(0,10);	
	m_Ymax_Spn.SetRange(0,10);	
	m_Ymin_Spn.SetBuddy ((CWnd *)GetDlgItem(IDC_EDIT_YMIN_EXP));	
	m_Ymax_Spn.SetBuddy ((CWnd *)GetDlgItem(IDC_EDIT_YMAX_EXP));		

	if(pParent->m_LogPlot==0)
	{
		GetManExp(pParent->m_Ymin, &man, &exp);
		m_Ymin_Spn.SetPos(exp);
		m_Ymin_Man = man;

		GetManExp(pParent->m_Ymax, &man, &exp);
		m_Ymax_Spn.SetPos(exp);
		m_Ymax_Man = man;
	}
	else
	{
		m_Ymin_Man = 1;
		m_Ymin_Spn.SetPos(pParent->m_Ymin);	
		m_Ymax_Man = 1;
		m_Ymax_Spn.SetPos(pParent->m_Ymax);

	}
	m_LinLog = pParent->m_LogPlot;
	// Set the color properties
	m_Spn_Clr_Rndms.SetRange(0,7);	
	m_Spn_Clr_Trues.SetRange(0,7);	
	m_Spn_Clr_Rndms.SetBuddy ((CWnd *)GetDlgItem(IDC_EDIT_CLR_RNDMS2));	
	m_Spn_Clr_Trues.SetBuddy ((CWnd *)GetDlgItem(IDC_EDIT_CLR_PRMPTS2));	

	m_Spn_Clr_Rndms.SetPos(pParent->m_Rndm_Clr);
	m_Spn_Clr_Trues.SetPos(pParent->m_Trues_Clr);

	for(a=0;a<8;a++)
		m_ClrBrush[a].CreateSolidBrush(pParent->crColors[a]);
		
	if (pParent->m_Plt_Rnd_Flag ==0)
		m_Chk_Rndms.SetCheck(0);
	else
		m_Chk_Rndms.SetCheck(1);
	if (pParent->m_Plt_Tru_Flag ==0)
		m_Chk_Trs.SetCheck(0);
	else
		m_Chk_Trs.SetCheck(1);

	if(pParent->m_Move_En==0)
		m_Chk_Move.SetCheck(0);
	else
		m_Chk_Move.SetCheck(1);

	m_Move = pParent->m_Move_Dur;

	UpdateData(FALSE);


	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGraphProp::GetManExp(long num, float *man, int *exp)
{
	int i;
	double z;
	if(num<=0)
	{
		*man = 0.0;
		*exp=0;
	}
	else
	{
		i=0;
		z = (double)num;
		while(z >= 10.0)
		{
			z=z/10.0;
			i++;
		}
		*man = (float)z;
		*exp = i;

	}

}

void CGraphProp::OnOK() 
{
	// TODO: Add extra validation here
	int a;
	UpdateData(TRUE);
	
	
	pParent->m_LogPlot = m_LinLog;

	if(m_LinLog==0)
	{
		pParent->m_Ymin =  (long)(m_Ymin_Man * pow(10., (double)m_Ymin_Spn.GetPos()));
		pParent->m_Ymax =  (long)(m_Ymax_Man * pow(10., (double)m_Ymax_Spn.GetPos()));
		pParent->m_YminLin = pParent->m_Ymin;
		pParent->m_YmaxLin = pParent->m_Ymax;
	}
	else
	{
		pParent->m_Ymin =  (long)m_Ymin_Spn.GetPos();
		pParent->m_Ymax =  (long)m_Ymax_Spn.GetPos();
		pParent->m_YminLog = pParent->m_Ymin;
		pParent->m_YmaxLog = pParent->m_Ymax;
	}

	pParent->m_Rndm_Clr = m_Spn_Clr_Rndms.GetPos();
	pParent->m_Trues_Clr = m_Spn_Clr_Trues.GetPos();

	if(m_Chk_Trs.GetCheck() ==0)
		pParent->m_Plt_Tru_Flag = 0;
	else
		pParent->m_Plt_Tru_Flag = 1;

	if(m_Chk_Rndms.GetCheck() ==0)
		pParent->m_Plt_Rnd_Flag = 0;
	else
		pParent->m_Plt_Rnd_Flag = 1;


	pParent->Invalidate();
	for(a=0;a<8;a++)
		m_ClrBrush[a].DeleteObject();
	
	if(m_Chk_Move.GetCheck() ==0)
		pParent->m_Move_En = 0;
	else
		pParent->m_Move_En = 1;

	pParent->m_Move_Dur = m_Move;
	pParent->m_Move_Xmax = m_Move;
	pParent->m_Move_Xmin = 0;

	pParent->SaveGraphIniFile();
	
	CDialog::OnOK();
}

void CGraphProp::OnRadioLin() 
{
	// Linear
	int exp;
	float man;


	GetManExp(pParent->m_YminLin, &man, &exp);
	m_Ymin_Spn.SetPos(exp);
	m_Ymin_Man = man;

	GetManExp(pParent->m_YmaxLin, &man, &exp);
	m_Ymax_Spn.SetPos(exp);
	m_Ymax_Man = man;
	m_LinLog = 0;
	UpdateData(FALSE);
	
}

void CGraphProp::OnRadioLog() 
{
	// Log

	m_Ymin_Man = 1;
	m_Ymin_Spn.SetPos(pParent->m_YminLog);	
	m_Ymax_Man = 1;
	m_Ymax_Spn.SetPos(pParent->m_YmaxLog);
	m_LinLog = 1;
	UpdateData(FALSE);

}

HBRUSH CGraphProp::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	int a;
	HBRUSH hbr;
	hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_EDIT_CLR_RNDMS:
		a = m_Spn_Clr_Rndms.GetPos();
		if (a>=0 && a<8)
		{
			pDC->SetBkColor(pParent->crColors[a]);
			pDC->SetTextColor(pParent->crColors[a]);
			hbr = (HBRUSH) m_ClrBrush[a];
		}
		break;
	case IDC_EDIT_CLR_PRMPTS:
		a = m_Spn_Clr_Trues.GetPos();
		if (a>=0 && a<8)
		{
			pDC->SetBkColor(pParent->crColors[a]);
			pDC->SetTextColor(pParent->crColors[a]);
			hbr = (HBRUSH) m_ClrBrush[a];
		}
		break;
	default:
		hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
		break;
	}

	return hbr;
}


void CGraphProp::OnDeltaposSpin1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	Invalidate();
	*pResult = 0;
}

void CGraphProp::OnDeltaposSpin2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	Invalidate();

	*pResult = 0;
}
