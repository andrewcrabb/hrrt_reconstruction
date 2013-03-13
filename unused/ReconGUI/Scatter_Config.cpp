// Scatter_Config.cpp : implementation file
//
/* Modification history
 *  31-Jul-2008: Always use span9 for scatter (sibomana@gmail.com)
 *  22-Aug-2008: Use umap input in e7_sino, create it if not existing prior to calling e7_sino
*/

#include "stdafx.h"
#include "ReconGUI.h"
#include "ReconGUIDlg.h"
#include "Scatter_Config.h"
#include "Edit_Drop.h"
#include "Header.h"
#include "Shlwapi.h"
#include <io.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScatter_Config dialog

extern HWND gbl_hWnd;
extern int gbl_dlgFlag;

CScatter_Config::CScatter_Config(CWnd* pParent /*=NULL*/)
	: CDialog(CScatter_Config::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScatter_Config)
	m_LLD = 0;
	m_ULD = 0;
	//}}AFX_DATA_INIT
}


void CScatter_Config::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScatter_Config)
	DDX_Control(pDX, IDC_BUTTON13, m_ScatQCButton);
	DDX_Control(pDX, IDC_BUTTON7, m_ScatButton);
	DDX_Control(pDX, IDC_EDIT9, m_QC_file);
	DDX_Control(pDX, IDC_CHECK2, m_QC);
	DDX_Control(pDX, IDC_EDIT8, m_3D_Atten);
	DDX_Control(pDX, IDC_EDIT7, m_Mu_Map);
	DDX_Control(pDX, IDC_EDIT6, m_Normalization);
	DDX_Control(pDX, IDC_EDIT4, m_Scatter);
	DDX_Control(pDX, IDC_EDIT3, m_Emission);
	DDX_Text(pDX, IDC_EDIT1, m_LLD);
	DDX_Text(pDX, IDC_EDIT2, m_ULD);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScatter_Config, CDialog)
	//{{AFX_MSG_MAP(CScatter_Config)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_BUTTON10, OnButton10)
	ON_BN_CLICKED(IDC_BUTTON11, OnButton11)
	ON_BN_CLICKED(IDC_BUTTON12, OnButton12)
	ON_BN_CLICKED(IDC_BUTTON7, OnButton7)
	ON_BN_CLICKED(IDC_BUTTON13, OnButton13)
	//}}AFX_MSG_MAP
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
// CScatter_Config message handlers

BOOL CScatter_Config::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	
	m_Mu_Map.SetWindowText(pDlg->m_Mu_Map_File);
	m_3D_Atten.SetWindowText(pDlg->m_3D_Atten_File);
	m_Normalization.SetWindowText(pDlg->m_Norm_9_File);
	m_Scatter.SetWindowText(pDlg->m_Scatter_File);

	// Disable scatter and QC file selection for multi-frame
	if (pDlg->m_multi_frames)
	{
		m_Emission.SetWindowText(pDlg->m_dyn_File);
		m_Scatter.EnableWindow(FALSE);
		m_ScatButton.EnableWindow(FALSE);
		m_ScatQCButton.EnableWindow(FALSE);
		m_QC_file.EnableWindow(FALSE); // QC file editing
		m_QC.EnableWindow(FALSE);		// QC button 
	}
	else
	{
		m_Emission.SetWindowText(pDlg->m_Em_File);
		// Enable scatter and QC file selection
		m_Scatter.EnableWindow(TRUE);
		m_ScatButton.EnableWindow(TRUE);
		m_QC_file.EnableWindow(TRUE);	// QC file editing
		m_QC.EnableWindow(TRUE);		// QC button 
	}

	m_LLD = pDlg->m_LLD;
	m_ULD = pDlg->m_ULD;
	
	m_QC.SetCheck(pDlg->m_Sc_Debug);
	m_QC_file.SetWindowText(pDlg->m_QC_Path);
	UpdateData (FALSE);
	
	gbl_hWnd = m_Emission.m_hWnd;
	gbl_dlgFlag = 2;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScatter_Config::OnOK() 
{
	bool cm_ok = true;
	CString str;
	CString atn_patname, em_patname, sc_patname;
	CString firstname, lastname;
	UpdateData (TRUE);
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	

	// Check the user input 

	// Mu not required for e7_sino
	/*
	if (m_Mu_Map.GetWindowTextLength()<=0)
	{
		cm_ok = false;
	}
	else	
	{	m_Mu_Map.GetWindowText(pDlg->m_Mu_Map_File);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_Mu_Map_File))
			cm_ok = false;
	}	
	*/
	if (m_3D_Atten.GetWindowTextLength()<=0)
	{
		cm_ok = false;
	}
	else
	{	m_3D_Atten.GetWindowText(pDlg->m_3D_Atten_File);
		if (pDlg->CheckForWhiteSpace(pDlg->m_3D_Atten_File))
		{ // update umap file
			pDlg->m_Mu_Map_File = pDlg->m_3D_Atten_File;
			pDlg->m_Mu_Map_File.Replace(".a", ".i");
		}
		else
		{
			cm_ok = false;
		}
	}

	// Skip output filename check if multi-frame
	if (!pDlg->m_multi_frames)
	{
		if (m_Scatter.GetWindowTextLength()<=0)
		{
		cm_ok = false;
		}
		else		
		{	m_Scatter.GetWindowText(pDlg->m_Scatter_File);
			if (!pDlg->CheckForWhiteSpace(pDlg->m_Scatter_File))
				cm_ok = false;
		}
	}

	if (m_Normalization.GetWindowTextLength()<=0)
	{
		cm_ok = false;
	}
	else
	{   // always use span9 for scatter
		m_Normalization.GetWindowText(pDlg->m_Norm_9_File);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_Norm_9_File))
			cm_ok = false;
	}
	if (m_Emission.GetWindowTextLength()<=0)
	{
		cm_ok = false;
	}
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

	if (m_LLD < 0 || m_LLD > 800)
	{
		cm_ok = false;
	}
	else
		pDlg->m_LLD = m_LLD;
	
	if (m_ULD < 0 || m_ULD > 800)
	{
		cm_ok = false;
	}
	else
		pDlg->m_ULD = m_ULD;
	
	
/*
 * Always create scatter QC file	
 *
	if (m_QC.GetCheck() == 1)
	{
		if (m_QC_file.GetWindowTextLength() > 0)
		{
			m_QC_file.GetWindowText(str);
			// Remove trailing '\'
			int len = str.GetLength(); 
			if(str[len-1] == '\\')	str = str.Left(len-1);
//			BOOL ret = PathFileExists(str);
//			if (!ret)
			if (_access(str,0) == -1)
			{
				MessageBox("You have entered an invalid debug path directory.  Please re-enter a valid debug path.","Debug Path",MB_OK);
				cm_ok = false;	
			}
			else
			{
				pDlg->m_Sc_QC = 1;
				pDlg->m_Sc_QC_Path = str;		
				if (!pDlg->CheckForWhiteSpace(pDlg->m_Sc_QC_Path))
					cm_ok = false;	
			}
		}

		else
		{
			pDlg->m_Sc_QC = 0;
			cm_ok = false;
		}

	}
	else pDlg->m_Sc_QC = 0;
	*/

	// Get current  QC file name
	if (m_QC_file.GetWindowTextLength() > 0)
	{
		m_QC_file.GetWindowText(str);
		pDlg->m_QC_Path = str;
		if (!pDlg->CheckForWhiteSpace(pDlg->m_QC_Path))	cm_ok = false;	
	}

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
	pDlg->m_Scatter_OK = cm_ok;
	pDlg->Invalidate(TRUE);
	
	CDialog::OnOK();
}

// Read in EM file
void CScatter_Config::OnButton6() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
//	strFilter = "Sinogram files (*.s)|*.s|All Files (*.*)|*.*||" ;
//	strFilter = "Sinogram files (*.s)|*.s|Dynamic sinogram files (*.dyn)|*.dyn|All Files (*.*)|*.*||" ;
	strFilter = "Sinogram files (*.s)|*.s|Dynamic sinogram files (*.dyn)|*.dyn|listmode files (*.l32;*.l64)|*.l32;*.l64|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		CHeader hdr;
		char line[256];
		CString filename = dlg.GetPathName();
		CString hdr_fname = "";

		CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
		m_Emission.SetWindowText(filename);
		if (filename.Find(".dyn") > 0)
		{
			FILE *fp = fopen(filename,"rt");
			if (fp!=NULL)
			{
				fgets(line,256,fp); // number of frames
				if (fgets(line,256,fp) != NULL)
				{
					line[strlen(line)-1] = '\0'; // remove endofline
					hdr_fname.Format("%s.hdr", line);
				}
			}
			// Disable scatter and QC file selection for multi-frame
			m_Scatter.EnableWindow(FALSE);
			m_ScatButton.EnableWindow(FALSE);
			m_ScatQCButton.EnableWindow(FALSE);
			m_QC_file.EnableWindow(FALSE);	// QC file editing
			m_QC.EnableWindow(FALSE);		// QC button 
		}
		else 
		{
			hdr_fname.Format("%s.hdr", (const char*)filename);
			// Enable scatter and QC file selection
			m_Scatter.EnableWindow(TRUE);
			m_ScatButton.EnableWindow(TRUE);
			m_ScatQCButton.EnableWindow(TRUE);
			m_QC_file.EnableWindow(TRUE);	// QC file editing
			m_QC.EnableWindow(TRUE);		// QC button 
		}
		if (hdr.OpenFile(hdr_fname))
		{
			int value=0;
			if (hdr.Readint("energy window lower level[1]", &value) == 0)
				pDlg->m_LLD = m_LLD = value;
			if (hdr.Readint("energy window upper level[1]", &value) == 0)
				pDlg->m_ULD = m_ULD = value;
		}
		if (pDlg->m_Auto_Assign.GetCheck() != 0)
		{
			CString basename = dlg.GetFileName();
			CString sinoPath = filename;
			int l64_pos = basename.Find(".l64"), l32_pos = basename.Find(".l32");
			if (l32_pos>0 || l64_pos>0)
			{
				// Replace original data path by sinogram data dir
				// when histogramming
				pDlg->make_sino_filename(filename, sinoPath);
				if (l64_pos>0) basename.Replace(".l64", ".s");
				else basename.Replace(".l32", ".s");
				sinoPath.Replace(basename, NULL);
			}
			else
			{
				sinoPath.Replace(basename, NULL);
			}
			if (basename.Find(".dyn")>0) basename.Replace(".dyn", NULL);
			else basename.Replace(".s", NULL);
			pDlg->m_Scatter_File.Format("%s%s_sc.s",sinoPath, basename);
			m_Scatter.SetWindowText(pDlg->m_Scatter_File);
			// Create scatter QC in QC directory
			pDlg->m_QC_Path.Format("%sQC\\%s_sc_qc.ps",sinoPath, basename);
			m_QC_file.SetWindowText(pDlg->m_QC_Path);

			// Overwrite image name
			filename = sinoPath + basename;
			switch(pDlg->m_Type) {
			case 0:	filename += "_SD.i"; break;
			case 1: filename += "_FD.i"; break;
			case 2: filename += "_FO.i"; break;
			case 4: filename += "_3D.i"; break;
			case 5: filename += "_FH.i"; break;
			default: filename += ".i";
			}
			pDlg->m_Image = filename;
		}
		UpdateData (FALSE);
	}	
}
// Read in Normalization file
void CScatter_Config::OnButton10() 
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
// Read in mumap file
void CScatter_Config::OnButton11() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Image files (*.i)|*.i|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_Mu_Map.SetWindowText(dlg.GetPathName());		
	}		
}
// Read in 3D attenuation file
void CScatter_Config::OnButton12() 
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
// Get in the scatter filename to be saved
void CScatter_Config::OnButton7() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Sinogram files (*.s)|*.s|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_Scatter.SetWindowText(dlg.GetPathName());
	}		
}


# ifdef OLD_CODE 
// Get in the working directory
void CScatter_Config::OnButton13() 
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
				m_WorkDir.SetWindowText(strDir+"\\");

            }
            // Free the PIDL allocated by SHBrowseForFolder.
            pMalloc->Free(pidl);
        }
        // Release the shell's allocator.
        pMalloc->Release();
    }	
}

#else

// New interface: User specifies the QC output file rather than a working directory
void CScatter_Config::OnButton13() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Sinogram files (*.ps)|*.ps|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);;

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_QC_file.SetWindowText(dlg.GetPathName());
	}		
}

#endif
