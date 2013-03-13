// UserIn.cpp : implementation file
//


#include "stdafx.h"
#include "CorrectRun.h"
#include "UserIn.h"
//#include "calendar.h"

#include "CorrectRunDoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserIn dialog


CUserIn::CUserIn(CWnd* pParent /*=NULL*/)
	: CDialog(CUserIn::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUserIn)
	m_ActivityVlu = 0.0f;
	m_VolumeVlu = 0.0f;
	m_Minute = 0;
	m_Second = 0;
	m_ClrScale = -1;
	m_Plane = 0;
	//}}AFX_DATA_INIT
}


void CUserIn::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserIn)
	DDX_Control(pDX, IDC_HOUR_COMBO, m_cmbHour);
	DDX_Control(pDX, IDC_COMBO1, m_cmbRAE);
	DDX_Control(pDX, IDC_COMBO2, m_cmbActive);
	DDX_Control(pDX, IDC_COMBO3, m_cmbVolume);
	DDX_Text(pDX, IDC_EDIT1, m_ActivityVlu);
	DDX_Text(pDX, IDC_EDIT3, m_VolumeVlu);
	DDV_MinMaxFloat(pDX, m_VolumeVlu, 0.f, 1.e+007f);
	DDX_Control(pDX, IDC_CALENDAR1, m_MyCalendar);
	DDX_Text(pDX, IDC_EDIT_MINUTE, m_Minute);
	DDV_MinMaxInt(pDX, m_Minute, 0, 59);
	DDX_Text(pDX, IDC_EDIT_SECOND, m_Second);
	DDV_MinMaxInt(pDX, m_Second, 0, 59);
	DDX_Text(pDX, IDC_EDIT7, m_Plane);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUserIn, CDialog)
	//{{AFX_MSG_MAP(CUserIn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserIn message handlers


void CUserIn::OnOK() 
{
	// TODO: Add extra validation here

	// stored changed values to the document
	UpdateData(TRUE);
	m_pCRunpDoc->m_ActiveType = m_cmbActive.GetCurSel ();
	m_pCRunpDoc->m_Activity = m_ActivityVlu;
	if (m_pCRunpDoc->m_ActiveType == 1)
	{  // convert mCi to MBq
		m_pCRunpDoc->m_Activity *= 37;
		m_pCRunpDoc->m_ActiveType = 0;
	}
	m_pCRunpDoc->m_VolumeType = m_cmbVolume.GetCurSel ();
	m_pCRunpDoc->m_Volume = m_VolumeVlu;
	if (m_pCRunpDoc->m_VolumeType == 1)
	{ // convert liter to cc
		m_pCRunpDoc->m_Volume *= 1000;
		m_pCRunpDoc->m_VolumeType = 0;
	}
	m_pCRunpDoc->m_RAE = m_cmbRAE.GetCurSel ();

	
	m_pCRunpDoc->m_OrgDay = m_MyCalendar.GetDay();
	m_pCRunpDoc->m_OrgMonth = m_MyCalendar.GetMonth();
	m_pCRunpDoc->m_OrgYear = m_MyCalendar.GetYear();
	m_pCRunpDoc->m_OrgHour  = m_cmbHour.GetCurSel();
	m_pCRunpDoc->m_OrgMinute  = m_Minute ;
	m_pCRunpDoc->m_OrgSecond = m_Second;
	
	m_pCRunpDoc->m_MasterPlane = m_Plane;	
	

	// Reset the old edge values to zero
/*	for (a=0;a<(m_pCRunpDoc->m_XSize * m_pCRunpDoc->m_YSize);++a)
	{
			m_pCRunpDoc->m_PlotOpt[a] = 0;
			m_pCRunpDoc->m_PlotOpt[a] = 0;
	}
*/	
	//m_pCRunpDoc->FindEdge ();
	//m_pCRunpDoc->FindInWord();

	m_pCRunpDoc->CalFactor();
	//m_pCRunpDoc->StoreFactor();
	m_pCRunpDoc->SaveConfigFile();

	CDialog::OnOK();


	CString strFilter, str;
	bool res = FALSE;
	// Set the filter and set dialogfile 
	strFilter = "Header files (*.hdr)|*.hdr|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,"hdr",NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);


	int reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_pCRunpDoc->m_NrmFileName = dlg.GetPathName();

		m_pCRunpDoc->StoreFactor();		
	
	}	

}

BOOL CUserIn::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	// Get the current values
	m_pCRunpDoc = dynamic_cast<CCorrectRunDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	
	m_ActivityVlu = m_pCRunpDoc->m_Activity;
	m_cmbActive.SetCurSel(m_pCRunpDoc->m_ActiveType);
	m_VolumeVlu = m_pCRunpDoc->m_Volume ;
	m_cmbVolume.SetCurSel(m_pCRunpDoc->m_VolumeType);
	m_cmbRAE.SetCurSel(m_pCRunpDoc->m_RAE);

	m_MyCalendar.SetDay(m_pCRunpDoc->m_OrgDay);
	m_MyCalendar.SetMonth(m_pCRunpDoc->m_OrgMonth);
	m_MyCalendar.SetYear(m_pCRunpDoc->m_OrgYear);
	m_cmbHour.SetCurSel(m_pCRunpDoc->m_OrgHour );
	m_Minute = m_pCRunpDoc->m_OrgMinute;
	m_Second = m_pCRunpDoc->m_OrgSecond;

	
	
	m_Plane = m_pCRunpDoc->m_MasterPlane;	

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

