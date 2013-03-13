// SSRDIFTDlg.cpp : implementation file
//
/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				Cluster integration
 */
#include "stdafx.h"
#include "ReconGUI.h"
#include "ReconGUIDlg.h"
#include "SSRDIFTDlg.h"
#include "Edit_Drop.h"
#include "Header.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSSRDIFTDlg dialog

extern HWND gbl_hWnd;
extern int gbl_dlgFlag;

CSSRDIFTDlg::CSSRDIFTDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSSRDIFTDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSSRDIFTDlg)
	m_Type = -1;
	m_Weight_method = -1;
	m_ImageSize = _T("128");
	m_Calibration = 1.0;
	m_PSF = FALSE;
	//}}AFX_DATA_INIT
}


void CSSRDIFTDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSSRDIFTDlg)
	DDX_Control(pDX, IDC_BUTTON13, m_ScatButton);
	DDX_Control(pDX, IDC_CHECK2, m_Rebin);
	DDX_Control(pDX, IDC_EDIT10, m_header);
	DDX_Control(pDX, IDC_EDIT9, m_Scatter);
	DDX_Control(pDX, IDC_EDIT7, m_Image);
	DDX_Control(pDX, IDC_EDIT8, m_3D_Atten);
	DDX_Control(pDX, IDC_EDIT6, m_Normalization);
	DDX_Control(pDX, IDC_EDIT3, m_Emission);
	DDX_Radio(pDX, IDC_RADIO1, m_Type);
	DDX_CBString(pDX, IDC_IMG_SIZE, m_ImageSize);
	DDX_Control(pDX, IDC_EDIT_CLUSTER_LOCATION, m_Cluster_Dir);
	DDX_Check(pDX, IDC_CHECK_PSF, m_PSF);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EDIT4, m_IterName);
	DDX_Control(pDX, IDC_EDIT11, m_SubName);
	DDX_Radio(pDX, IDC_RADIO5, m_Weight_method);
	DDX_Control(pDX, IDC_EDIT1, m_Iteration);
	DDX_Control(pDX, IDC_EDIT2, m_SubSets);
	DDX_Control(pDX, IDC_EDIT12, m_EndPlane);
}


BEGIN_MESSAGE_MAP(CSSRDIFTDlg, CDialog)
	//{{AFX_MSG_MAP(CSSRDIFTDlg)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_BUTTON10, OnButton10)
	ON_BN_CLICKED(IDC_BUTTON12, OnButton12)
	ON_BN_CLICKED(IDC_BUTTON11, OnButton11)
	ON_BN_CLICKED(IDC_BUTTON13, OnButton13)
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_BN_CLICKED(IDC_RADIO3, OnRadio3)
	ON_BN_CLICKED(IDC_RADIO4, OnRadio4)
	ON_BN_CLICKED(IDC_BUTTON14, OnButton14)
	ON_BN_CLICKED(IDC_CLUSTER_LOCATION, OnClusterLocation)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_RADIO9, OnRadio9)
END_MESSAGE_MAP()

static int 	get_patname(const char *path, CString &lastname, CString &firstname)
{
	int pos1=0, pos2=0;
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(path, drive, dir, fname, ext);
	lastname = fname;
	if ((pos1 = lastname.Find("-")) > 0)
	{
		if ((pos2 = lastname.Find("-", pos1+1)) > 0)
		{
			firstname = lastname.Mid(pos1+1,pos2-pos1-1);
			lastname = lastname.Left(pos1);
			return 1;
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CSSRDIFTDlg message handlers

void CSSRDIFTDlg::OnOK() 
{
	
	bool cm_ok = true;
	CString str;
	CString atn_patname, em_patname, sc_patname;
	CString firstname, lastname;
	CString recon_Norm_File;

	UpdateData (TRUE);
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	
	// check user input
	if (m_3D_Atten.GetWindowTextLength()>0)
	{
		m_3D_Atten.GetWindowText(pDlg->m_3D_Atten_File);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_3D_Atten_File))
		{  // allow cluster recon without scatter correction
			pDlg->m_3D_Atten_File = "";
			if (pDlg->submit_recon_dir.IsEmpty()) cm_ok = false;
		}
	}
	else
	{ // allow cluster recon without scatter correction
		pDlg->m_3D_Atten_File = "";
		if (pDlg->submit_recon_dir.IsEmpty()) cm_ok = false;
	}
		

	if (m_Normalization.GetWindowTextLength()<=0)
	{
		cm_ok = false;
	}
	else
	{
		// Check it for span 3 or span 9
		if (pDlg->m_Ary_inc==0)
			m_Normalization.GetWindowText(pDlg->m_Norm_3_File);
		else
			m_Normalization.GetWindowText(pDlg->m_Norm_9_File);
		m_Normalization.GetWindowText(recon_Norm_File);
		if (!pDlg->CheckForWhiteSpace(recon_Norm_File))
			cm_ok = false;	
	}
	if (m_Emission.GetWindowTextLength()<=0)
		cm_ok = false;
	else
	{
		m_Emission.GetWindowText(str);
		str.MakeLower();
		if (str.Find(".dyn") != -1)
		{
			pDlg->m_multi_frames = true;
			pDlg->m_dyn_File = str;
		}
		else
		{
			pDlg->m_multi_frames = false;
			pDlg->m_Em_File = str;
		}		
		if (!pDlg->CheckForWhiteSpace(pDlg->m_Em_File))
			cm_ok = false;
	}

	// Check scatter filename check if single-frame
	// and not UWOSEM method
	if (!pDlg->m_multi_frames && (m_Type!=3 || m_Weight_method>0))
	{
		if (m_Scatter.GetWindowTextLength()>0)
		{
			m_Scatter.GetWindowText(pDlg->m_Scatter_File);
			if (!pDlg->CheckForWhiteSpace(pDlg->m_Scatter_File))
			{
				pDlg->m_Scatter_File = "";
				if (pDlg->submit_recon_dir.IsEmpty()) cm_ok = false;
			}
		}
		else 
		{ // allow cluster recon without scatter correction
			pDlg->m_Scatter_File = "";
			if (pDlg->submit_recon_dir.IsEmpty()) cm_ok = false;
		}
	}

	if (m_Image.GetWindowTextLength()<=0)
		cm_ok = false;
	else
	{
		m_Image.GetWindowText(pDlg->m_Image);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_Image))
			cm_ok = false;
	}
	
	if (m_Cluster_Dir.GetWindowTextLength() > 0)
		m_Cluster_Dir.GetWindowText(pDlg->submit_recon_dir);

	if (m_Type < 0 || m_Type > 4)
		cm_ok = false;
	else
	{
		pDlg->m_Type = m_Type;
	}
	if(m_Type ==3)
	{	
		if (m_Weight_method >= 0 && m_Weight_method < 4)
			pDlg->m_Weight_method = m_Weight_method;	
		else
			cm_ok = false;
	}

	pDlg->m_PSF = m_PSF;

	m_Iteration.GetWindowText(str);
	pDlg->m_Iteration = atoi(str);
	if(m_Type < 4)
	{
		m_SubSets.GetWindowText(str);
		pDlg->m_SubSets = atoi(str);
	}
	else
	{
		m_SubSets.GetWindowText(str);
		pDlg->m_StartPlane = atoi(str);

		m_EndPlane.GetWindowText(str);
		pDlg->m_EndPlane = atoi(str);
	}
//	pDlg->m_Recon_Rebin = m_Rebin.GetCheck();
	if (m_ImageSize.Find(_T("128"))>=0) pDlg->m_Recon_Rebin = TRUE;
	else pDlg->m_Recon_Rebin = FALSE;
	m_header.GetWindowText(pDlg->m_Quan_Header);	
	if (!pDlg->CheckForWhiteSpace(pDlg->m_Quan_Header))
		cm_ok = false;

	// update span 3 or span 9 calibration factor
	if (pDlg->m_Span == 3) pDlg->m_calFactor[0] = m_Calibration;
	else if (pDlg->m_Span == 9) pDlg->m_calFactor[1] = m_Calibration;
	pDlg->Invalidate(TRUE);

	// Check for patient name mismatch in em, atn, and scatter
	if (get_patname(pDlg->m_Em_File, lastname, firstname))
	{
		em_patname = lastname + "-" + firstname;
		if (!pDlg->m_3D_Atten_File.IsEmpty()) // Empty for uncorrected recon
		{
			if (get_patname(pDlg->m_3D_Atten_File, lastname, firstname))
			{
				atn_patname = lastname + "-" + firstname;
				if (em_patname.CompareNoCase(atn_patname) != 0)
				{
					if (MessageBox("Emission=" + em_patname +  " Attenuation=" + atn_patname + "  Continue?",
						"Patient Name mismatch", MB_YESNO) == IDNO) cm_ok = false;
				}
			}
		}
		if (!pDlg->m_Scatter_File.IsEmpty()) // Empty for uncorrected recon
		{
			if (get_patname(pDlg->m_Scatter_File, lastname, firstname))
			{
				sc_patname = lastname + "-" + firstname;
				if (cm_ok && em_patname.CompareNoCase(sc_patname) != 0)
				{
					if (MessageBox("Emission=" + em_patname +  " Scatter=" + sc_patname + "  Continue?",
						"Patient Name mismatch", MB_YESNO) == IDNO) cm_ok = false;
				}
			}
		}
	}
	pDlg->m_Recon_OK = cm_ok;
	CDialog::OnOK();
}

BOOL CSSRDIFTDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	int len;
	CString strName;
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	
	CString recon_Norm_File = pDlg->m_Ary_inc==0? 
								pDlg->m_Norm_3_File:pDlg->m_Norm_9_File;
	m_3D_Atten.SetWindowText(pDlg->m_3D_Atten_File);
	m_Normalization.SetWindowText(recon_Norm_File);
	
	if (pDlg->m_Weight_method >= 0 && pDlg->m_Weight_method < 4)
		m_Weight_method = pDlg->m_Weight_method;
	else
		m_Weight_method = 0;

	m_PSF = pDlg->m_PSF;

	if (pDlg->m_multi_frames)
		m_Emission.SetWindowText(pDlg->m_dyn_File);
	else
		m_Emission.SetWindowText(pDlg->m_Em_File);

	if (!pDlg->m_Scatter_File.IsEmpty())
		m_Scatter.SetWindowText(pDlg->m_Scatter_File);
	if (pDlg->m_Type >= 0 && pDlg->m_Type < 5)
		m_Type = pDlg->m_Type;
	else
		m_Type = 0;

	// Disable scatter file selection for multi-frame
	if (pDlg->m_multi_frames)
	{
		m_Emission.SetWindowText(pDlg->m_dyn_File);
		m_Scatter.EnableWindow(FALSE);
		m_ScatButton.EnableWindow(FALSE);
	}
	else
	{
		m_Emission.SetWindowText(pDlg->m_Em_File);
		m_Scatter.EnableWindow(TRUE);
		m_ScatButton.EnableWindow(TRUE);
	}

	// Set the image filename
	if (!pDlg->m_Image.IsEmpty())
		m_Image.SetWindowText(pDlg->m_Image);
	else
	{
		if (!pDlg->m_Em_File.IsEmpty())
		{
			len = pDlg->m_Em_File.GetLength();		
			strName.Format("%s.i",pDlg->m_Em_File.Mid(0,len-2));	
			m_Image.SetWindowText(strName);
		}

	}
	if (pDlg->m_Recon_Rebin) m_ImageSize = _T("128");
	else  m_ImageSize = _T("256");
	m_header.SetWindowText(pDlg->m_Quan_Header);
	//m_SubName.SetWindowText("Subsets");
	//m_IterName.SetWindowText("Iterations");
	m_Cluster_Dir.SetWindowText(pDlg->submit_recon_dir);
	Iteration = pDlg->m_Iteration;
	SubSets = pDlg->m_SubSets;
	StartPlane = pDlg->m_StartPlane;
	EndPlane = pDlg->m_EndPlane;
	if (pDlg->m_Span == 3) m_Calibration = pDlg->m_calFactor[0];
	else if (pDlg->m_Span == 9) m_Calibration = pDlg->m_calFactor[1];
	UpdateData(FALSE);
	switch (m_Type)
	{
	case 0:
		OnRadio1();
		break;
	case 1:
		OnRadio2();
		break;
	case 2:
		OnRadio3();
		break;
	case 3:
		OnRadio4();
		break;
	case 4:
		OnRadio9();
		break;
	}
	
	gbl_hWnd = m_Emission.m_hWnd;
	gbl_dlgFlag = 3;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// Read EM filename
void CSSRDIFTDlg::OnButton6() 
{
	CString strFilter,strName;
	int reply;

	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	


	// Set the filter and set dialogfile 
//	strFilter = "Sinogram files (*.s)|*.s|Dynamic sinogram files (*.dyn)|*.dyn|All Files (*.*)|*.*||" ;
	strFilter = "Sinogram files (*.s)|*.s|Dynamic sinogram files (*.dyn)|*.dyn|listmode files (*.l32;*.l64)|*.l32;*.l64|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_Emission.SetWindowText(dlg.GetPathName());			
		if (pDlg->m_Auto_Assign.GetCheck() != 0)
		{
			UpdateData(TRUE);
			CString filename = dlg.GetPathName ();
			CString strName = filename;
			CString basename;
			int l64_pos = filename.Find(".l64"), l32_pos = filename.Find(".l32");
			if (l32_pos>0 || l64_pos>0)
			{
				// Replace original data path by sinogram data dir
				// when histogramming
				pDlg->make_sino_filename(filename, strName);
			}
			basename = strName;
			if (basename.Find(".dyn")>0) basename.Replace(".dyn", NULL);
			else basename.Replace(".s", NULL);

			if (m_Type == 0) 
				strName.Format("%s_SD.i",basename);
				
			else if (m_Type == 1)
				strName.Format("%s_FD.i",basename);
			else if (m_Type == 2)
				strName.Format("%s_FO.i",basename);
			else if (m_Type == 3)
				strName.Format("%s_3D.i",basename);
			else if (m_Type == 4)
				strName.Format("%s_FH.i",basename);

			m_Image.SetWindowText(strName);
			UpdateData(FALSE);

			// update scatter files if Run Scatter not enabled
			if (pDlg->m_Run_sc.GetCheck() == 0)
			{
				pDlg->m_Scatter_File.Format("%s_sc.s",basename);
				m_Scatter.SetWindowText(pDlg->m_Scatter_File);
			}

		}
	}	

}
// Read Normalization filename
void CSSRDIFTDlg::OnButton10() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Normalization files (*.n)|*.n|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_Normalization.SetWindowText(dlg.GetPathName());
	}		
	
}
// Get aatenuation filename
void CSSRDIFTDlg::OnButton12() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Attenuation files (*.a)|*.a|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_3D_Atten.SetWindowText(dlg.GetPathName());
	}		
}
// Get image filename
void CSSRDIFTDlg::OnButton11() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Image files (*.i)|*.i|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_Image.SetWindowText(dlg.GetPathName());
	}	
}
//Get Scatter Filename
void CSSRDIFTDlg::OnButton13() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Scatter files (*.s)|*.s|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_Scatter.SetWindowText(dlg.GetPathName());
	}		
	
}
// SD radio button option
void CSSRDIFTDlg::OnRadio1() 
{
	CString str;
	CWnd *pwnd;
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT1);
	pwnd->EnableWindow(FALSE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT2);
	pwnd->EnableWindow(FALSE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT11);
	pwnd->SetWindowText("Subsets");
	pwnd->EnableWindow(FALSE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT4);
	pwnd->EnableWindow(FALSE);
	pwnd->SetWindowText("Iteration");
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT13);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT12);
	pwnd->EnableWindow(FALSE);

	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO5);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO6);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO7);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO8);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_CHECK6);
	pwnd->EnableWindow(FALSE);

	// Subset and iteration
	str.Format("%d",SubSets);
	m_SubSets.SetWindowText(str);
	str.Format("%d",Iteration);
	m_Iteration.SetWindowText(str);
	if(m_Image.GetWindowTextLength() > 0)
	{
		m_Image.GetWindowText(str);
		if(str.Find("_FD.i",1) != -1)
			str.Replace("_FD.i","_SD.i");
		else if(str.Find("_FO.i",1) != -1)
			str.Replace("_FO.i","_SD.i");
		else if(str.Find("_3D.i",1) != -1)
			str.Replace("_3D.i","_SD.i");
		else if(str.Find("_FH.i",1) != -1)
			str.Replace("_FH.i","_SD.i");
		else if(str.Find("_SD.i",1) == -1)
			str.Replace(".i","_SD.i");

		m_Image.SetWindowText(str);

	}
}
// FD radio button option
void CSSRDIFTDlg::OnRadio2() 
{
	CString str;
	CWnd *pwnd;
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT1);
	pwnd->EnableWindow(FALSE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT2);
	pwnd->EnableWindow(FALSE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT11);
	pwnd->SetWindowText("Subsets");
	pwnd->EnableWindow(FALSE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT4);
	pwnd->EnableWindow(FALSE);
	pwnd->SetWindowText("Iteration");
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT13);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT12);
	pwnd->EnableWindow(FALSE);

	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO5);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO6);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO7);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO8);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_CHECK6);
	pwnd->EnableWindow(FALSE);

	// Subset and iteration
	str.Format("%d",SubSets);
	m_SubSets.SetWindowText(str);
	str.Format("%d",Iteration);
	m_Iteration.SetWindowText(str);

	if(m_Image.GetWindowTextLength() > 0)
	{
		m_Image.GetWindowText(str);
		if(str.Find("_SD.i",1) != -1)
			str.Replace("_SD.i","_FD.i");
		else if(str.Find("_FO.i",1) != -1)
			str.Replace("_FO.i","_FD.i");
		else if(str.Find("_3D.i",1) != -1)
			str.Replace("_3D.i","_FD.i");
		else if(str.Find("_FH.i",1) != -1)
			str.Replace("_FH.i","_FD.i");
		else if(str.Find("_FD.i",1) == -1)
			str.Replace(".i","_FD.i");

		m_Image.SetWindowText(str);

	}
}
// FO radio button option
void CSSRDIFTDlg::OnRadio3() 
{
	CString str;
	// Enable the text boxes
	CWnd *pwnd;
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT1);
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT2);
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT11);
	pwnd->SetWindowText("Subsets");
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT4);
	pwnd->EnableWindow(TRUE);
	pwnd->SetWindowText("Iteration");
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT13);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT12);
	pwnd->EnableWindow(FALSE);

	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO5);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO6);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO7);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO8);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_CHECK6);
	pwnd->EnableWindow(FALSE);

	// Subset and iteration
	str.Format("%d",SubSets);
	m_SubSets.SetWindowText(str);
	str.Format("%d",Iteration);
	m_Iteration.SetWindowText(str);

	if(m_Image.GetWindowTextLength() > 0)
	{
		m_Image.GetWindowText(str);
		if(str.Find("_SD.i",1) != -1)
			str.Replace("_SD.i","_FO.i");
		else if(str.Find("_FD.i",1) != -1)
			str.Replace("_FD.i","_FO.i");
		else if(str.Find("_3D.i",1) != -1)
			str.Replace("_3D.i","_FO.i");
		else if(str.Find("_FH.i",1) != -1)
			str.Replace("_FH.i","_FO.i");
		else if(str.Find("_FO.i",1) == -1)
			str.Replace(".i","_FO.i");

		m_Image.SetWindowText(str);

	}	
}
// Get quantification header filename
void CSSRDIFTDlg::OnButton14() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Header files (*.hdr)|*.hdr|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_header.SetWindowText(dlg.GetPathName());
		CHeader m_Quan_Header;
		if (m_Quan_Header.OpenFile(dlg.GetPathName()))
		{
			m_Quan_Header.Readfloat("calibration factor", &m_Calibration);
			UpdateData(FALSE);
			m_Quan_Header.CloseFile();
		}
	}		
}


// 3D OSEM radio button option
void CSSRDIFTDlg::OnRadio4()
{
	CString str;
	CWnd *pwnd;
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT1);
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT2);
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT11);
	pwnd->SetWindowText("Subsets");
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT4);
	pwnd->EnableWindow(TRUE);
	pwnd->SetWindowText("Iteration");
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT13);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT12);
	pwnd->EnableWindow(FALSE);

	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO5);
	pwnd->EnableWindow(TRUE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO6);
	pwnd->EnableWindow(TRUE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO7);
	pwnd->EnableWindow(TRUE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO8);
	pwnd->EnableWindow(TRUE);
	//pwnd = (CWnd*)GetDlgItem(IDC_CHECK6);
	//pwnd->EnableWindow(TRUE);

	

	// Subset and iteration
	str.Format("%d",SubSets);
	m_SubSets.SetWindowText(str);
	str.Format("%d",Iteration);
	m_Iteration.SetWindowText(str);

	if(m_Image.GetWindowTextLength() > 0)
	{
		m_Image.GetWindowText(str);
		if(str.Find("_SD.i",1) != -1)
			str.Replace("_SD.i","_3D.i");
		else if(str.Find("_FD.i",1) != -1)
			str.Replace("_FD.i","_3D.i");
		else if(str.Find("_FO.i",1) != -1)
			str.Replace("_FO.i","_3D.i");
		else if(str.Find("_FH.i",1) != -1)
			str.Replace("_FH.i","_3D.i");
		else if(str.Find("_3D.i",1) == -1)
			str.Replace(".i","_3D.i");

		m_Image.SetWindowText(str);

	}
}
// FH radio button option
void CSSRDIFTDlg::OnRadio9()
{
	CString str;
	// Enable the text boxes
	CWnd *pwnd;
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT1);
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT2);
	pwnd->EnableWindow(TRUE);
	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT11);
	pwnd->EnableWindow(TRUE);
	pwnd->SetWindowText("Start Plane");

	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT4);
	pwnd->EnableWindow(TRUE);
	pwnd->SetWindowText("Iteration");

	pwnd = (CWnd*)GetDlgItem(IDC_EDIT13);
	pwnd->EnableWindow(TRUE);
	pwnd->SetWindowText("End Plane");
	pwnd = (CWnd*)GetDlgItem(IDC_EDIT12);
	pwnd->EnableWindow(TRUE);

	//pwnd->ShowWindow(SW_SHOW);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO5);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO6);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO7);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_RADIO8);
	pwnd->EnableWindow(FALSE);
	pwnd = (CWnd*)GetDlgItem(IDC_CHECK6);
	pwnd->EnableWindow(FALSE);

	// Start and End Planes
	str.Format("%d",Iteration);
	m_Iteration.SetWindowText(str);

	str.Format("%d",StartPlane);
	m_SubSets.SetWindowText(str);
	str.Format("%d",EndPlane);
	m_EndPlane.SetWindowText(str);

	if(m_Image.GetWindowTextLength() > 0)
	{
		m_Image.GetWindowText(str);
		if(str.Find("_SD.i",1) != -1)
			str.Replace("_SD.i","_FH.i");
		else if(str.Find("_FD.i",1) != -1)
			str.Replace("_FD.i","_FH.i");
		else if(str.Find("_FO.i",1) != -1)
			str.Replace("_FO.i","_FH.i");
		else if(str.Find("_3D.i",1) != -1)
			str.Replace("_3D.i","_FH.i");
		else if(str.Find("_FH.i",1) == -1)
			str.Replace(".i","_FH.i");

		m_Image.SetWindowText(str);

	}	
}

// Check the cluster folder location
void CSSRDIFTDlg::OnClusterLocation()
{
	CString strDir;
  	//CParam* pSheet = (CParam*) GetParent();	

	LPMALLOC pMalloc;
    /* Gets the Shell's default allocator */
    if (::SHGetMalloc(&pMalloc) == NOERROR)
    {
        BROWSEINFO bi;
        char pszBuffer[MAX_PATH];
        LPITEMIDLIST pidl;
        // Get help on BROWSEINFO struct - it's got all the bit settings.
        bi.hwndOwner = GetSafeHwnd();
        bi.pidlRoot = NULL;
        bi.pszDisplayName = pszBuffer;
        bi.lpszTitle = _T("Select a Directory");
        bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
        bi.lpfn = NULL;
        bi.lParam = 0;
        // This next call issues the dialog box.
        if ((pidl = ::SHBrowseForFolder(&bi)) != NULL)
        {
            if (::SHGetPathFromIDList(pidl, pszBuffer))
            { 
            // At this point pszBuffer contains the selected path */.
                strDir =  pszBuffer;
				m_Cluster_Dir.SetWindowText(strDir);

            }
            // Free the PIDL allocated by SHBrowseForFolder.
            pMalloc->Free(pidl);
        }
        // Release the shell's allocator.
        pMalloc->Release();
    }	
}
