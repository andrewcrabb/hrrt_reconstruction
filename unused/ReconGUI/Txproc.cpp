// Txproc.cpp : implementation file
//
/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				Integrate MAP-TR
 *  07-Oct-2008: Added TX TV3DReg to MAP-TR segmentation methods list (Todo: design better interface)
*/
#include "stdafx.h"
#include "ReconGUI.h"
#include "ReconGUIDlg.h"
#include "Txproc.h"
//#include "Edit_Drop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTxproc dialog

extern HWND gbl_hWnd;
extern int gbl_dlgFlag;

CTxproc::CTxproc(CWnd* pParent /*=NULL*/)
	: CDialog(CTxproc::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTxproc)
	m_Method = -1;
	m_Bone_Seg = 0.0f;
	m_Water_Seg = 0.0f;
	m_Noise_Seg = 0.0f;
	m_MU_scale = TRUE;
	m_MAP_prior = _T("");
	m_MU_peak = 0.096f;
	m_MU_ImageSize = _T("128");
	m_MAP_smoothing = 0.0f;
	//}}AFX_DATA_INIT
	// Default priors
}


void CTxproc::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTxproc)
	DDX_Control(pDX, IDC_MU_SCALE, m_MU_scale_Wnd);
	DDX_Control(pDX, IDC_MU_PEAK, m_MU_Peak_Wnd);
	DDX_Control(pDX, IDC_MU_PEAK_LBL, m_PeakLabel);
	DDX_Control(pDX, IDC_MAPSEG_PAR, m_PriorEdit);
	DDX_Control(pDX, IDC_COMBO_MAP_PRIOR, m_PriorSelection);
	DDX_Control(pDX, IDC_CHECK2, m_Segmentation);
	DDX_Control(pDX, IDC_LIST2, m_Blank_List);
	DDX_Control(pDX, IDC_LIST3, m_Tx_List);
	DDX_Control(pDX, IDC_EDIT4, m_2D_Atten);
	DDX_Control(pDX, IDC_EDIT3, m_Mu_Map);
	DDX_Control(pDX, IDC_CHECK1, m_Rebin);
	DDX_Radio(pDX, IDC_RADIO1, m_Method);
	DDX_Text(pDX, IDC_EDIT1, m_Bone_Seg);
	DDX_Text(pDX, IDC_EDIT2, m_Water_Seg);
	DDX_Text(pDX, IDC_EDIT5, m_Noise_Seg);
	DDX_Check(pDX, IDC_MU_SCALE, m_MU_scale);
	DDX_Text(pDX, IDC_MAPSEG_PAR, m_MAP_prior);
	DDX_Text(pDX, IDC_MU_PEAK, m_MU_peak);
	DDX_CBString(pDX, IDC_MU_IMGSIZE, m_MU_ImageSize);
	DDX_Text(pDX, IDC_MAP_SMOOTH, m_MAP_smoothing);
	DDV_MinMaxFloat(pDX, m_MAP_smoothing, 10.f, 90.f);
	//}}AFX_DATA_MAP
	DDV_MinMaxFloat(pDX, m_MU_peak, 0.001f, 10.0f);
}


BEGIN_MESSAGE_MAP(CTxproc, CDialog)
	//{{AFX_MSG_MAP(CTxproc)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_BN_CLICKED(IDC_BUTTON5, OnButton5)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_RADIO3, OnMap_TR)
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_CBN_SELCHANGE(IDC_COMBO_MAP_PRIOR, OnSelchangePrior)
	ON_EN_KILLFOCUS(IDC_EDIT3, OnKillfocusEdit3)
	ON_BN_CLICKED(IDC_CHECK2, OnMuSegButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTxproc message handlers

// Get blank files
void CTxproc::OnButton2() 
{
	CString strFilter;
	int reply, cnt;

	// Set the filter and set dialogfile 
//	strFilter = "Sinogram files (*.s)|*.s|All Files (*.*)|*.*||" ;
	strFilter = "Sinogram files (*.s)|*.s|listmode files (*.l64)|*.l64|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		cnt = m_Blank_List.GetCount();
		if (cnt< 5)
			m_Blank_List.AddString(dlg.GetPathName());
		
	}
}

// Remove blank file
void CTxproc::OnButton4() 
{
	// Remove selection from blank listbox
	int sel;
	
	sel = m_Blank_List.GetCurSel();
	m_Blank_List.DeleteString(sel);
}



void CTxproc::OnOK() 
{
	int num,i;
	CString cmd, str;
	bool cm_ok = true;

	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	UpdateData(TRUE);

	// Build the command
	num = m_Blank_List.GetCount();
	pDlg->m_Blank_Count = num;

	if (!(num > 0 && num < 6))
	{

		// no balnk file are selected
		cm_ok = false;
		if (num < 1)
			pDlg->m_Blank_Count = 0;
		else
			pDlg->m_Blank_Count = 5;
		// Error Message
	}
	else
	{	for(i=0;i<num;i++)
		{	m_Blank_List.GetText(i,pDlg->m_Blank_File[i]);
			if (!pDlg->CheckForWhiteSpace(pDlg->m_Blank_File[i]))
				cm_ok = false;
		}
	}

	// Add Tx files
	// Build the command
	num = m_Tx_List.GetCount();
	pDlg->m_Tx_Count = num;
	if (!(num > 0 && num < 6))
	{
		if (num < 1)
			pDlg->m_Tx_Count = 0;
		else
			pDlg->m_Tx_Count = 5;		
	}
	else
	{	for(i=0;i<num;i++)
		{	m_Tx_List.GetText(i,pDlg->m_Tx_File[i]);
			if (!pDlg->CheckForWhiteSpace(pDlg->m_Tx_File[i]))
				cm_ok = false;
		}
	}


	//str.Format("-d %d -s %d ", pDlg->m_Ary_Ring[m_Ary_inc], pDlg->m_Ary_Span[m_Ary_inc] );
	//cmd.operator += ( str );

		
	if (m_Mu_Map.GetWindowTextLength()<=0)
	{
		cm_ok = false;
		// Error message
	}
	else
	{
		int single_dot_flag=1;
		m_Mu_Map.GetWindowText(pDlg->m_Mu_Map_File);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_Mu_Map_File, single_dot_flag))
			cm_ok = false;
	}

	// Update MAP prior
	pDlg->set_prior(m_PriorSelection.GetCurSel(), m_MAP_prior);

/*
 	if (m_2D_Atten.GetWindowTextLength()<=0)
	{
		cm_ok = false;
	}
	else
	{	m_2D_Atten.GetWindowText(pDlg->m_2D_Atten_File);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_2D_Atten_File))
			cm_ok = false;
		else
			if (pDlg->m_Auto_Assign.GetCheck() != 0)
			{
				m_2D_Atten.GetWindowText(pDlg->m_3D_Atten_File);
				pDlg->m_3D_Atten_File.Replace("Atten_2d_", "Atten_3d_");

				m_2D_Atten.GetWindowText(m_Atten_Correction);
				m_Atten_Correction.Replace("Atten_2d_", "At_Correction_");			

				m_2D_Atten.GetWindowText(pDlg->m_Scatter_File);
				pDlg->m_Scatter_File.Replace("Atten_2d_", "Scatter_");
				pDlg->m_Scatter_File.Replace(".a", ".s");
			}
	}

	if (m_Atten_Correction.IsEmpty())
	{
		cm_ok = false;
	}
	else
		pDlg->m_Atten_Correction = m_Atten_Correction;
*/	
	
	pDlg->m_Tx_Method = m_Method ;
//	pDlg->m_Tx_Rebin = m_Rebin.GetCheck();
	if (m_MU_ImageSize.Find("128") >= 0) pDlg->m_Tx_Rebin = TRUE;
	else pDlg->m_Tx_Rebin = FALSE;

	pDlg->m_MU_scale = m_MU_scale;
	pDlg->m_MU_peak = m_MU_peak;
//	if (m_Method == 2) pDlg->m_Tx_Segment = m_MAP_segment;
//	else
	pDlg->m_Tx_Segment = m_Segmentation.GetCheck();

	if ((m_Bone_Seg >= m_Water_Seg) &&  (m_Bone_Seg>= m_Noise_Seg) && (m_Bone_Seg >0.f))
		pDlg->m_Bone_Seg = m_Bone_Seg;
	else
		cm_ok = false;

	if ( (m_Water_Seg >= m_Noise_Seg) && (m_Water_Seg >0.f))
		pDlg->m_Water_Seg = m_Water_Seg;
	else
		cm_ok = false;
	if (m_Noise_Seg >0.f)
		pDlg->m_Noise_Seg = m_Noise_Seg;
	else
		cm_ok = false;

	pDlg->m_Tx_OK = cm_ok;
	pDlg->m_MAP_smoothing = m_MAP_smoothing;
	
	pDlg->Invalidate(TRUE);

	CDialog::OnOK();
}

// Get Tx file
void CTxproc::OnButton3() 
{
	// Add selection to TX listbox
	CString strFilter, strPath, strName,str;
	int reply, cnt, len;
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	

	// Set the filter and set dialogfile 
//	strFilter = "Sinogram files (*.s)|*.s|All Files (*.*)|*.*||" ;
	strFilter = "Sinogram files (*.s)|*.s|listmode files (*.l64)|*.l64|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;
	
	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		cnt = m_Tx_List.GetCount();		
		if (cnt< 5)
		{
			m_Tx_List.AddString(dlg.GetPathName());
			
			int single_dot_flag = 1;

			str = dlg.GetFileName();

			//strName = dlg.GetFileName ();
			//strName.Replace(dlg.GetFileExt(), "i");
			strPath = dlg.GetPathName();

			// Replace original data path by sinogram data dir
			// when histogramming
			CString sinoPath;
			if (str.Find(".l64")>0  && pDlg->make_sino_filename(strPath, sinoPath))
			{
				strPath = sinoPath;
				str.Replace(".l64", ".s");
			}

			len = str.GetLength();		
			strName.Format("%s.i",str.Mid(0,len-2));
			strPath.Replace(str,NULL);				
		
			pDlg->CheckForWhiteSpace(strName, single_dot_flag);
			if (pDlg->m_Auto_Assign.GetCheck() != 0)
			{
				str.Format("%s%s",strPath, strName);
				m_Mu_Map.SetWindowText(str);
				strName.Replace(".i", ".a");
				
//				str.Format("%sAtten_2d_%s",strPath, strName);
//				m_2D_Atten.SetWindowText(str);
//				m_Atten_Correction.Format("%sAt_Correction_%s",strPath, strName);
//				m_2D_Atten.GetWindowText(pDlg->m_3D_Atten_File);
//				pDlg->m_3D_Atten_File.Replace("Atten_2d_", "Atten_3d_");
				pDlg->m_3D_Atten_File.Format("%s%s",strPath, strName);
				pDlg->m_2D_Atten_File.Format("%sAtten_2d_%s",strPath, strName);
				pDlg->m_Atten_Correction.Format("%sAt_Correction%s",strPath, strName);
				strName.Replace(".a", ".s");
//				pDlg->m_Scatter_File.Format("%sScatter_%s",strPath, strName);
			}
			else
			{ // auto assign for 2D_Atten_File and Atten_Correction
				strName.Replace(".i", ".a");
				pDlg->m_2D_Atten_File.Format("%sAtten_2d_%s",strPath, strName);
				pDlg->m_Atten_Correction.Format("%sAt_Correction%s",strPath, strName);
			}
		}
		UpdateData(FALSE);
		
	}
	
}
// remove tx files
void CTxproc::OnButton5() 
{
	// Remove current selection from TX listbox
	int sel;
	
	sel = m_Tx_List.GetCurSel();
	m_Tx_List.DeleteString(sel);	
}

BOOL CTxproc::OnInitDialog() 
{
	int num,i;
	CWnd *pwnd = NULL;
	CDialog::OnInitDialog();
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	
	m_Method = pDlg->m_Tx_Method;
	num = pDlg->m_Blank_Count;
	for(i=0;i<num;i++)
		if (i< 5)
			m_Blank_List.AddString(pDlg->m_Blank_File[i]);
	num = pDlg->m_Tx_Count;
	for(i=0;i<num;i++)
		if (i < 5)
			m_Tx_List.AddString(pDlg->m_Tx_File[i]);
//	m_Rebin.SetCheck(pDlg->m_Tx_Rebin);
	if (pDlg->m_Tx_Rebin) m_MU_ImageSize = _T("128");
	else m_MU_ImageSize = _T("256");

	m_Segmentation.SetCheck(pDlg->m_Tx_Segment);
	m_Bone_Seg = pDlg->m_Bone_Seg;
	m_Water_Seg = pDlg->m_Water_Seg;
	int num_priors = pDlg->get_num_priors();
	m_MAP_prior = pDlg->m_MAP_prior;
	m_MU_scale = pDlg->m_MU_scale;
	m_MU_peak = pDlg->m_MU_peak;
	m_MAP_smoothing = pDlg->m_MAP_smoothing;

	if (num_priors > 1)
	{
		CString name, value;
		for (int index=0; index<num_priors; index++)
		{
			pDlg->get_prior(index, name, value);
			m_PriorSelection.AddString(name);
			if (index==0)
				m_PriorSelection.SetCurSel(0);
			else if (value == m_MAP_prior) 
			{
				m_PriorSelection.SetCurSel(index);
			}
		}
	}
	else m_PriorSelection.ShowScrollBar(SW_HIDE);

//	if (m_MAP_prior.GetLength() > 0) m_MAP_segment = TRUE;
//	else
//	{
//		m_MAP_segment = false;
//		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAPSEG)) != NULL)
//			pwnd->EnableWindow(FALSE);
//	}

	m_Mu_Map.SetWindowText(pDlg->m_Mu_Map_File);
//	m_2D_Atten.SetWindowText(pDlg->m_2D_Atten_File);
	m_Noise_Seg = pDlg->m_Noise_Seg;
	UpdateData (FALSE);
	gbl_hWnd = m_Tx_List.m_hWnd;
	gbl_dlgFlag = 0;

	//CWnd* pwnd = m_Tx_List.GetParent();

	update_segment_UI();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTxproc::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
// Ge mu map filename
void CTxproc::OnButton6() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "HRRT image files (*.i)|*.i|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{

			m_Mu_Map.SetWindowText( dlg.GetPathName());
		
	}	
}
// Get 2D attenuation filename
/*
 void CTxproc::OnButton7() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Attenuation files (*.a)|*.a|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{

			m_2D_Atten.SetWindowText( dlg.GetPathName());
			m_2D_Atten.GetWindowText(m_Atten_Correction);
			if (m_Atten_Correction.Find("Atten_2d_",1)!= -1)
				m_Atten_Correction.Replace("Atten_2d_", "At_Correction_");			
			else
				m_Atten_Correction.Replace(".a", "_atc.a");	

	}	
}
*/

/*
 *	void CTxproc::update_segment_UI() 
 *  If MAP-TR is selected, hide UWOSEM/NEC segmentation UI and show MAP-TR segementation UI
 *  Otherwise hide MAP-TR segementation UI and show UWOSEM/NEC segmentation UI
 */
void CTxproc::update_segment_UI() 
{
	CWnd *pwnd;
	CButton *map_tr_button = (CButton*)GetDlgItem(IDC_RADIO3);
	if (map_tr_button->GetCheck())
	{
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT1_LBL)) != NULL)
			pwnd->ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT2_LBL)) != NULL)
			pwnd->ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT5_LBL)) != NULL)
			pwnd->ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT1)) != NULL)
			pwnd->ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT2)) != NULL)
			pwnd->ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT5)) != NULL)
			pwnd->ShowWindow(SW_HIDE);


		m_PriorSelection.ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAPSEG_PAR)) != NULL)
			pwnd->ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAPSEG_LBL)) != NULL)
			pwnd->ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAP_SMOOTH)) != NULL)
			pwnd->EnableWindow(TRUE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAP_SMOOTH_LBL)) != NULL)
			pwnd->EnableWindow(TRUE);

		if (m_Segmentation.GetCheck())
		{
			m_MU_scale_Wnd.EnableWindow(FALSE);
			m_MU_Peak_Wnd.EnableWindow(FALSE);
			m_PeakLabel.EnableWindow(FALSE);
		}
		else
		{  
			m_MU_scale_Wnd.EnableWindow(TRUE);
			m_MU_Peak_Wnd.EnableWindow(FALSE);   // Keep MU Peak disabled (MS 07-aug-2008
			m_PeakLabel.EnableWindow(TRUE);
		}			
	}
	else
	{
		m_PriorSelection.ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAPSEG_PAR)) != NULL)
			pwnd->ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAPSEG_LBL)) != NULL)
			pwnd->ShowWindow(SW_HIDE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAP_SMOOTH)) != NULL)
			pwnd->EnableWindow(FALSE);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_MAP_SMOOTH_LBL)) != NULL)
			pwnd->EnableWindow(FALSE);

		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT1_LBL)) != NULL)
			pwnd->ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT2_LBL)) != NULL)
			pwnd->ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT5_LBL)) != NULL)
			pwnd->ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT1)) != NULL)
			pwnd->ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT2)) != NULL)
			pwnd->ShowWindow(SW_SHOW);
		if ((pwnd = (CWnd*)GetDlgItem(IDC_EDIT5)) != NULL)
			pwnd->ShowWindow(SW_SHOW);

		// show Mu Peak scale value
		m_MU_scale_Wnd.EnableWindow(FALSE);
		m_MU_Peak_Wnd.EnableWindow(FALSE);
		m_PeakLabel.EnableWindow(FALSE);
	}
}

/*
 *	void CTxproc::OnMap_TR() 
 * MAP-TR method selection callback: update segmentation UI
 */
void CTxproc::OnMap_TR() 
{
	update_segment_UI();
}

/*
 *	void CTxproc::OnRadio1() 
 * UWOSEM method selection callback: update segmentation UI
 */
void CTxproc::OnRadio1() 
{
	update_segment_UI();
}

/*
 *	void CTxproc::OnRadio2() 
 *  NEC method selection callback: update segmentation UI
 */
void CTxproc::OnRadio2() 
{
	update_segment_UI();
}

/*
 *	void CTxproc::OnSelchangePrior() 
 *  MAP-TR prior list selection change callback: update current prior value
 */
void CTxproc::OnSelchangePrior() 
{
	CString name, value;
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	if (pDlg->get_prior(m_PriorSelection.GetCurSel(), name, value))
	{
		m_MAP_prior = value;
		m_PriorEdit.SetWindowText(m_MAP_prior);
		if (name.Find("TX_TV3DReg") == -1)	pDlg->m_TX_TV3DReg = FALSE;
		else pDlg->m_TX_TV3DReg = TRUE;
	}
}

/*
 *	void CTxproc::OnKillfocusEdit3() 
 *  Mu-Map filename change callback: update attenuation filename if applicable
 */
void CTxproc::OnKillfocusEdit3() 
{
	// TODO: Add your control notification handler code here
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	if (pDlg->m_Auto_Assign.GetCheck() != 0)
	{
		CString dir, basename;
		m_Mu_Map.GetWindowText(pDlg->m_Mu_Map_File);
		const char *full_path = pDlg->m_Mu_Map_File;
		const char *file_name = strrchr(full_path,'\\');
		int len = 0;
		if (file_name != NULL) 
		{
			file_name++;
			len = (int)(file_name - full_path);
			dir = pDlg->m_Mu_Map_File.Left(len);
			basename = file_name;
			if ((len=basename.GetLength())>2) basename = basename.Left(len-2);
			pDlg->m_3D_Atten_File.Format("%s%s.a",dir, basename);
		}
	}
}

/*
 *	void CTxproc::OnMuSegButton() 
 * Enable segmentation button callback: update segmentation UI
 */
void CTxproc::OnMuSegButton() 
{
	update_segment_UI();
}
