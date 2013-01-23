// ConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ScanIt.h"
#include "ConfigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog


CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigDlg)
	m_Plane = 0;
	m_Ring = 0;
	m_ULD = 0;
	m_Span = 0;
	m_Speed = 0.0f;
	m_Scan_Type = -1;
	m_Scan_Mode = -1;
	m_Count = -1;
	m_Preset = -1;
	m_Duration = 0;
	m_KCPS = 0;
	m_LLD = 0;
	m_DynaFrame = _T("");
	//}}AFX_DATA_INIT
}


void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDlg)
	DDX_Control(pDX, IDC_CHECK4, m_ContScan);
	DDX_Control(pDX, IDC_CHECK3, m_Auto_Start);
	DDX_Control(pDX, IDC_CHECK2, m_64bit);
	DDX_Control(pDX, IDC_EDIT_SINO, m_Sino);
	DDX_Control(pDX, IDC_CHECK1, m_Chk_Seg);
	DDX_Control(pDX, IDC_SPIN1, m_Spn_Rng_Opt);
	DDX_Control(pDX, IDC_COMBO1, m_DType);
	DDX_Control(pDX, IDC_STATIC_ULD, m_ST_ULD);
	DDX_Control(pDX, IDC_STATIC_SPN, m_ST_Spn);
	DDX_Control(pDX, IDC_STATIC_SPD_DUR, m_ST_SD);
	DDX_Control(pDX, IDC_STATIC_RNG, m_ST_Rng);
	DDX_Control(pDX, IDC_STATIC_PLN, m_ST_Pln);
	DDX_Control(pDX, IDC_STATIC_LLD, m_ST_LLD);
	DDX_Text(pDX, IDC_EDIT_PLANE, m_Plane);
	DDV_MinMaxInt(pDX, m_Plane, 1, 16);
	DDX_Text(pDX, IDC_EDIT_RING, m_Ring);
	DDX_Text(pDX, IDC_EDIT_ULD, m_ULD);
	DDV_MinMaxInt(pDX, m_ULD, 200, 800);
	DDX_Text(pDX, IDC_EDIT_SPAN, m_Span);
	DDX_Text(pDX, IDC_EDIT_SPEED, m_Speed);
	DDV_MinMaxFloat(pDX, m_Speed, 10.f, 100.f);
	DDX_Radio(pDX, IDC_RADIO_EM, m_Scan_Type);
	DDX_Radio(pDX, IDC_RADIO_SCAN_MODE, m_Scan_Mode);
	DDX_Radio(pDX, IDC_RADIO_COUNT, m_Count);
	DDX_Radio(pDX, IDC_RADIO_PRESET, m_Preset);
	DDX_Text(pDX, IDC_EDIT_DURATION, m_Duration);
	DDX_Text(pDX, IDC_EDIT_AC_KCPS, m_KCPS);
	DDX_Text(pDX, IDC_EDIT_LLD, m_LLD);
	DDV_MinMaxInt(pDX, m_LLD, 200, 800);
	DDX_Text(pDX, IDC_EDIT2, m_DynaFrame);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	ON_BN_CLICKED(IDC_RADIO_EM, OnRadioEm)
	ON_BN_CLICKED(IDC_RADIO_TX, OnRadioTx)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
	ON_BN_CLICKED(IDC_RADIO_SCAN_MODE, OnRadioScanMode)
	ON_BN_CLICKED(IDC_RADIO_SCAN_HST, OnRadioScanHst)
	ON_BN_CLICKED(IDC_RADIO_PRESET, OnRadioPreset)
	ON_BN_CLICKED(IDC_RADIO_PRESET_CNT, OnRadioPresetCnt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg message handlers

void CConfigDlg::OnRadioEm() 
{
	CString str;

	// Hide the Plane text box
	CWnd *pwnd = (CWnd *)GetDlgItem(IDC_EDIT_PLANE);
	pwnd->ShowWindow(SW_HIDE);
	// Hide the plane lable
	pwnd = (CWnd *)GetDlgItem(IDC_STATIC_PLN);
	pwnd->ShowWindow(SW_HIDE);
	// Show the duration text box
	pwnd = (CWnd *)GetDlgItem(IDC_EDIT_DURATION);
	pwnd->ShowWindow(SW_SHOW);
	// Show the speed text box
	pwnd = (CWnd *)GetDlgItem(IDC_EDIT_SPEED);
	pwnd->ShowWindow(SW_HIDE);
	// Set the duration lable
	m_DType.ShowWindow(SW_SHOW);
	m_ST_SD.SetWindowText("Duration");
	// Enable the Time and count Radio buttons
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_PRESET);
	pwnd->EnableWindow(TRUE);
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_PRESET_CNT);
	pwnd->EnableWindow(TRUE);
	
	if(m_Chk_Seg.GetCheck())
	{
		str = "207";	
	}
	else
	{
		str.Format("%d",pDoc->m_en_sino[m_Ary_inc]);
		
	}
	m_Sino.SetWindowText(str);
	m_Scan_Type = 0;
	m_LLD = 400;
	m_ULD = 650;
	UpdateData(FALSE);
}

void CConfigDlg::OnRadioTx() 
{
	CString str;
	// Show the plane text box
	CWnd *pwnd = (CWnd *)GetDlgItem(IDC_EDIT_PLANE);
	pwnd->ShowWindow(SW_SHOW);
	// Show the plane lable
	pwnd = (CWnd *)GetDlgItem(IDC_STATIC_PLN);
	pwnd->ShowWindow(SW_SHOW);
	//Hide the duration text box
	pwnd = (CWnd *)GetDlgItem(IDC_EDIT_DURATION);
	pwnd->ShowWindow(SW_HIDE);
	// shoe the speed text box
	pwnd = (CWnd *)GetDlgItem(IDC_EDIT_SPEED);
	pwnd->ShowWindow(SW_SHOW);
	// Setup teh Speed lable
	m_DType.ShowWindow(SW_HIDE);
	m_ST_SD.SetWindowText("Speed");
	// Disable the Time and count Radio buttons
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_PRESET);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_PRESET_CNT);
	pwnd->EnableWindow(FALSE);
	
	//CRect rect;
	//GetWindowRect(&rect);
	//pwnd = (CWnd *)GetDlgItem(IDD_CONFIG_DLG);
	//SetWindowPos(pwnd,rect.left, rect.top, 360,rect.Height(),SWP_NOMOVE );
	
	str = "207";	
	m_Sino.SetWindowText(str);
	m_Scan_Type = 1;
	m_LLD = 550;
	m_ULD = 800;
	UpdateData(FALSE);
}

BOOL CConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	pDoc = dynamic_cast<CScanItDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	


	m_Ary_inc = pDoc->m_Ary_Select;
	// TODO: Add extra initialization here
	m_ST_LLD.SetWindowText("LLD");
	m_ST_ULD.SetWindowText("ULD");
	m_ST_Pln.SetWindowText("Crystal");
	m_ST_Spn.SetWindowText("Span");
	m_ST_Rng.SetWindowText("Ring Diff");
	// store the original values
	m_Old_Scan_Mode = pDoc->m_Scan_Mode;
	m_Old_Scan_Type = pDoc->m_Scan_Type;
	m_Old_uld = pDoc->m_uld;
	m_Old_lld = pDoc->m_lld;
	m_Old_Duration = pDoc->m_Duration;
	m_Old_DurType = pDoc->m_DurType;
	m_Old_Span = pDoc->m_Span;
	m_Old_Ring = pDoc->m_Ring;
	m_Old_Speed = pDoc->m_Speed;
	m_Old_Plane = pDoc->m_Plane;	

	m_ContScan.SetCheck(pDoc->m_Cont_Scan);

	//  retrive the data

	m_Plane = pDoc->m_Plane;
	m_Ring = pDoc->m_Ring;
	m_Span = pDoc->m_Span;
	m_Speed = pDoc->m_Speed;
	m_Duration =  pDoc->m_Duration;
	m_DType.SetCurSel(pDoc->m_DurType);
	m_DynaFrame = pDoc->m_DynaFrame; 

	if (pDoc->m_Scan_Type==0)
	{
		OnRadioEm();
		m_Scan_Type = pDoc->m_Scan_Type;
	}
	else
	{
		OnRadioTx();
		m_Scan_Type = pDoc->m_Scan_Type;
	}

	if (pDoc->m_Scan_Mode == 0)
		OnRadioScanMode();
	else
		OnRadioScanHst();

	m_Scan_Mode = pDoc->m_Scan_Mode;
	m_Chk_Seg.SetCheck(pDoc->m_Segment_Zero_Flag);

	m_Preset = pDoc->m_Preset_Opt;
	m_Count = pDoc->m_Count_Opt;
	m_64bit.SetCheck(pDoc->m_List_64bit);
	m_KCPS = pDoc->m_KCPS;
	m_Auto_Start.SetCheck(pDoc->m_Auto_Start);


	m_LLD = pDoc->m_lld;
	m_ULD = pDoc->m_uld;

	UpdateData(FALSE);
	m_Spn_Rng_Opt.SetRange(0,1);
	m_Spn_Rng_Opt.SetBuddy ((CWnd *)GetDlgItem(IDC_EDIT1));		
	m_Spn_Rng_Opt.SetPos(pDoc->m_Ary_Select);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigDlg::OnRadioScanMode() 
{

	CWnd *pwnd;
	// Enable the 64 bit check box
	pwnd = (CWnd *)GetDlgItem(IDC_CHECK2);
	pwnd->EnableWindow(TRUE);
	// Disable the count option and set the time option
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_PRESET_CNT);
	pwnd->EnableWindow(FALSE);
	m_Preset = 0;	

	// Disable the Net Trues
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_COUNT);
	pwnd->EnableWindow(FALSE);
	// Disable the Prompts + Randoms
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_COUNT_RP);
	pwnd->EnableWindow(FALSE);

	// Enable the frame definition
	pwnd = (CWnd *)GetDlgItem(IDC_EDIT2);
	pwnd->EnableWindow(TRUE);


	m_Scan_Mode = 0;
	//UpdateData(FALSE);	


}

void CConfigDlg::OnRadioScanHst() 
{
	CWnd *pwnd;
	// Disable the 64 bit check box
	pwnd = (CWnd *)GetDlgItem(IDC_CHECK2);
	pwnd->EnableWindow(FALSE);

	// Enable the count option and set the time option
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_PRESET);
	if (pwnd->IsWindowEnabled())
	{
		pwnd = (CWnd *)GetDlgItem(IDC_RADIO_PRESET_CNT);
		pwnd->EnableWindow(TRUE);
	}
	//m_Preset = pDoc->m_Preset_Opt;
	
	// enable the Net Trues
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_COUNT);
	pwnd->EnableWindow(TRUE);
	// enable the Prompts + Randoms
	pwnd = (CWnd *)GetDlgItem(IDC_RADIO_COUNT_RP);
	pwnd->EnableWindow(TRUE);
	
	// Enable the frame definition
	pwnd = (CWnd *)GetDlgItem(IDC_EDIT2);
	pwnd->EnableWindow(FALSE);

	m_Scan_Mode = 1;
	//UpdateData(FALSE);

}


void CConfigDlg::OnOK() 
{
	bool flag = FALSE;
	// uptade the changes
	UpdateData(TRUE);
	 
	pDoc->m_lld = m_LLD;
	pDoc->m_uld = m_ULD;
	pDoc->m_Plane = m_Plane;
	pDoc->m_Ring = m_Ring;
	pDoc->m_Span = m_Span;
	pDoc->m_Speed = m_Speed;
	pDoc->m_Duration = m_Duration;
	pDoc->m_DurType = m_DType.GetCurSel();
	

	pDoc->m_Scan_Type = m_Scan_Type;

	pDoc->m_Scan_Mode = m_Scan_Mode;
	pDoc->m_config = m_Scan_Mode;
	
	pDoc->pSinoInfo.fileType = m_Scan_Mode+1;
	pDoc->pSinoInfo.gantry = GNT_HRRT; // HRRT for now
	pDoc->pSinoInfo.scanType = m_Scan_Type;
	pDoc->pSinoInfo.span = m_Span;
	pDoc->pSinoInfo.ringDifference = m_Ring;

	pDoc->m_Ary_Select = m_Spn_Rng_Opt.GetPos();
	
	pDoc->pSinoInfo.lld = m_LLD;
	pDoc->pSinoInfo.uld = m_ULD;
	if(m_Chk_Seg.GetCheck() == 0)
		pDoc->m_Segment_Zero_Flag = FALSE;
	else
		pDoc->m_Segment_Zero_Flag = TRUE;

	// Continous Scan
	pDoc->m_Cont_Scan = m_ContScan.GetCheck();
	// see if any thing changed
	if(m_Old_Scan_Mode != pDoc->m_Scan_Mode) flag = TRUE;
	if(m_Old_Scan_Type = pDoc->m_Scan_Type) flag = TRUE;
	if(m_Old_uld = pDoc->m_uld) flag = TRUE;
	if(m_Old_lld = pDoc->m_lld) flag = TRUE;
	if(m_Old_Duration = pDoc->m_Duration) flag = TRUE;
	if(m_Old_DurType = pDoc->m_DurType) flag = TRUE;
	if(m_Old_Span = pDoc->m_Span) flag = TRUE;
	if(m_Old_Ring = pDoc->m_Ring) flag = TRUE;
	if(m_Old_Speed = pDoc->m_Speed) flag = TRUE;
	if(m_Old_Plane = pDoc->m_Plane) flag = TRUE;
	pDoc->m_config = pDoc->m_Scan_Type;

	pDoc->m_Preset_Opt = m_Preset;
	pDoc->m_Count_Opt = m_Count;
	pDoc->m_List_64bit = m_64bit.GetCheck();
	pDoc->m_KCPS = m_KCPS;
	pDoc->m_Auto_Start = m_Auto_Start.GetCheck();

	// Temperarly modified
	pDoc->m_Dn_Step = 1;
	pDoc->m_bDownFlag = TRUE;
	
	pDoc->m_DynaFrame = m_DynaFrame; 
	CDialog::OnOK();

//	if (flag)
//	{
//		int a = MessageBox("Would you like to save the changes to the configuration file?","Configuration Change",MB_YESNO | MB_ICONQUESTION);
//		if (a == 6)
//			pDoc->UpDateSaveFileFlag();
//	}
	//pDoc->m_bDownFlag = TRUE;

	
}


void CConfigDlg::OnChangeEdit1() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	CString str;
	m_Ary_inc = m_Spn_Rng_Opt.GetPos();

	if(m_Chk_Seg.GetCheck() || m_Scan_Type==1)
	{
		str = "207";	
	}
	else
	{
		str.Format("%d",pDoc->m_en_sino[m_Ary_inc]);
		
	}
	m_Sino.SetWindowText(str);
	m_Span = pDoc->m_Ary_Span[m_Ary_inc];
	m_Ring = pDoc->m_Ary_Ring[m_Ary_inc];
	UpdateData(FALSE);	
}

void CConfigDlg::OnCheck1() 
{
	UpdateData(TRUE);
	CString str;
	if(m_Chk_Seg.GetCheck() || m_Scan_Type==1)
	{
		str = "207";	
	}
	else
	{
		str.Format("%d",pDoc->m_en_sino[m_Ary_inc]);
		
	}
	m_Sino.SetWindowText(str);
}


void CConfigDlg::OnRadioPreset() 
{
	// Show the Combobox
	CWnd *pwnd;
	pwnd = (CWnd *)GetDlgItem(IDC_COMBO1);
	pwnd->ShowWindow(SW_SHOW);
		
	pwnd = (CWnd *)GetDlgItem(IDC_STATIC_SPD_DUR);
	m_ST_SD.SetWindowText("Duration");

		
}

void CConfigDlg::OnRadioPresetCnt() 
{
	// Hide the Combobox
	CWnd *pwnd;
	pwnd = (CWnd *)GetDlgItem(IDC_COMBO1);
	pwnd->ShowWindow(SW_HIDE);
	
	pwnd = (CWnd *)GetDlgItem(IDC_STATIC_SPD_DUR);
	m_ST_SD.SetWindowText("Counts");
	
}
