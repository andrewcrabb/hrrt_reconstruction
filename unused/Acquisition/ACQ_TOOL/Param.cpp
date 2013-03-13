// Param.cpp : implementation file
//

#include "stdafx.h"


#include "ScanIt.h"
//#include "ScanItDoc.h"

#include "ECodes.h"
#include "Param.h"
#include "cdhi.h"

extern CCDHI pDHI;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CParam dialog


CParam::CParam(CWnd* pParent /*=NULL*/)
	: CDialog(CParam::IDD, pParent)
{
	//{{AFX_DATA_INIT(CParam)
	m_HD = 0;
	m_Row = 0;
	m_Col = 0;
	m_Cnfg = 0;
	m_ScannerType = _T("");
	//}}AFX_DATA_INIT
}


void CParam::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CParam)
	DDX_Control(pDX, IDC_CHECK5, m_msg_err);
	DDX_Control(pDX, IDC_CHECK4, m_LogFile);
	DDX_Control(pDX, IDC_CHECK_CID, m_CID);
	DDX_Control(pDX, IDC_CHECK_PID, m_PID);
	DDX_Control(pDX, IDC_CHECK_NUM, m_Ran_Num);
	DDX_Control(pDX, IDC_CHECK_NAME, m_Pt_Name);
	DDX_Control(pDX, IDC_EDIT_WORJ_DIR, m_WorkDir);
	DDX_Control(pDX, IDC_CHECK_HD7, m_HD7);
	DDX_Control(pDX, IDC_CHECK_HD6, m_HD6);
	DDX_Control(pDX, IDC_CHECK_HD5, m_HD5);
	DDX_Control(pDX, IDC_CHECK_HD4, m_HD4);
	DDX_Control(pDX, IDC_CHECK_HD3, m_HD3);
	DDX_Control(pDX, IDC_CHECK_HD2, m_HD2);
	DDX_Control(pDX, IDC_CHECK_HD1, m_HD1);
	DDX_Control(pDX, IDC_CHECK_HD0, m_HD0);
	DDX_Text(pDX, IDC_EDIT_HD, m_HD);
	DDV_MinMaxInt(pDX, m_HD, 1, 8);
	DDX_Text(pDX, IDC_EDIT_ROWS, m_Row);
	DDV_MinMaxInt(pDX, m_Row, 1, 15);
	DDX_Text(pDX, IDC_EDIT_COLS, m_Col);
	DDV_MinMaxInt(pDX, m_Col, 1, 15);
	DDX_Text(pDX, IDC_EDIT_CNFG, m_Cnfg);
	DDV_MinMaxInt(pDX, m_Cnfg, 1, 4);
	DDX_Text(pDX, IDC_EDIT_SCANNER_TYPE, m_ScannerType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CParam, CDialog)
	//{{AFX_MSG_MAP(CParam)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_DIR, OnButtonDir)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CParam message handlers

BOOL CParam::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	//pParent = dynamic_cast<CScanItDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	

	//CScanItDlg* pParent = dynamic_cast<CScanItDlg*>(GetParent());	

	if (pDHI.m_HRRT)
		m_ScannerType = "HRRT";
	else
		m_ScannerType = "Unknown";

	m_HD = pDHI.m_TOTAL_HD;
	m_Row = pDHI.m_MAX_ROW;
	m_Col = pDHI.m_MAX_COL;
	m_Cnfg = pDHI.m_MAX_CNFG;
	m_HD0.SetCheck(pDHI.m_Hd_En[0]);
	//m_Btn_hd0.EnableWindow(pDHI.m_Hd_En[0]);
	m_HD1.SetCheck(pDHI.m_Hd_En[1]);
	//m_Btn_hd1.EnableWindow(pDHI.m_Hd_En[1]);
	m_HD2.SetCheck(pDHI.m_Hd_En[2]);
	//m_Btn_hd2.EnableWindow(pDHI.m_Hd_En[2]);
	m_HD3.SetCheck(pDHI.m_Hd_En[3]);
	//m_Btn_hd3.EnableWindow(pDHI.m_Hd_En[3]);
	m_HD4.SetCheck(pDHI.m_Hd_En[4]);
	//m_Btn_hd4.EnableWindow(pDHI.m_Hd_En[4]);
	m_HD5.SetCheck(pDHI.m_Hd_En[5]);
	//m_Btn_hd5.EnableWindow(pDHI.m_Hd_En[5]);
	m_HD6.SetCheck(pDHI.m_Hd_En[6]);
	//m_Btn_hd6.EnableWindow(pDHI.m_Hd_En[6]);
	m_HD7.SetCheck(pDHI.m_Hd_En[7]);
	//m_Btn_hd7.EnableWindow(pDHI.m_Hd_En[7]);
	
	m_WorkDir.SetWindowText(pDHI.m_Work_Dir);

	m_Pt_Name.SetCheck(pDHI.m_Opt_Name);
	m_PID.SetCheck(pDHI.m_Opt_PID);
	m_CID.SetCheck(pDHI.m_Opt_CID);
	m_Ran_Num.SetCheck(pDHI.m_Opt_Num);
	m_LogFile.SetCheck(pDHI.m_LogFlag);
	m_msg_err.SetCheck(pDHI.m_msg_err);

	UpdateData(FALSE);
 
	return TRUE;
}

void CParam::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	//m_pGantry->Release ();
}


void CParam::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);
	CString str;

	pDHI.m_TOTAL_HD = m_HD;
	pDHI.m_MAX_ROW = m_Row;
	pDHI.m_MAX_COL = m_Col;
	pDHI.m_MAX_CNFG = m_Cnfg;
	if(m_HD0.GetCheck()==0)
		pDHI.m_Hd_En[0] = FALSE;
	else
		pDHI.m_Hd_En[0] = TRUE;

	if(m_HD1.GetCheck()==0)
		pDHI.m_Hd_En[1] = FALSE;
	else
		pDHI.m_Hd_En[1] = TRUE;

	if(m_HD2.GetCheck()==0)
		pDHI.m_Hd_En[2] = FALSE;
	else
		pDHI.m_Hd_En[2] = TRUE;
	
	if(m_HD3.GetCheck()==0)
		pDHI.m_Hd_En[3] = FALSE;
	else
		pDHI.m_Hd_En[3] = TRUE;
	
	if(m_HD4.GetCheck()==0)
		pDHI.m_Hd_En[4] = FALSE;
	else
		pDHI.m_Hd_En[4] = TRUE;
	
	if(m_HD5.GetCheck()==0)
		pDHI.m_Hd_En[5] = FALSE;
	else
		pDHI.m_Hd_En[5] = TRUE;
	
	if(m_HD6.GetCheck()==0)
		pDHI.m_Hd_En[6] = FALSE;
	else
		pDHI.m_Hd_En[6] = TRUE;

	if(m_HD7.GetCheck()==0)
		pDHI.m_Hd_En[7] = FALSE;
	else
		pDHI.m_Hd_En[7] = TRUE;

	m_WorkDir.GetWindowText(pDHI.m_Work_Dir);
	if (pDHI.m_Work_Dir.Mid(pDHI.m_Work_Dir.GetLength()-1,1) != "\\")
		pDHI.m_Work_Dir += "\\";

	//pDHI.m_Cal_Factor = m_Cal_Factor;
	if (m_Pt_Name.GetCheck() == 0)
		pDHI.m_Opt_Name = FALSE;
	else
		pDHI.m_Opt_Name = TRUE;
	if (m_PID.GetCheck() == 0)
		pDHI.m_Opt_PID = FALSE;
	else
		pDHI.m_Opt_PID = TRUE;
	if (m_CID.GetCheck() == 0)
		pDHI.m_Opt_CID = FALSE;
	else
		pDHI.m_Opt_CID = TRUE;
	if (m_Ran_Num.GetCheck() == 0)
		pDHI.m_Opt_Num = FALSE;
	else
		pDHI.m_Opt_Num = TRUE;

	if(m_LogFile.GetCheck() == 0)
		pDHI.m_LogFlag = FALSE;
	else
		pDHI.m_LogFlag = TRUE;

	if(m_msg_err.GetCheck() == 0)
		pDHI.m_msg_err = FALSE;
	else
		pDHI.m_msg_err = TRUE;
	
	pDHI.WriteIniFile();
	
	str.Format("%s\\log",pDHI.m_Work_Dir);
	CreateDirectory(str,NULL);

	CDialog::OnOK();
}


void CParam::OnButtonDir() 
{
	/*
	CString strFilter, str;
	bool res = FALSE;

	// Set the filter and set dialogfile 
	strFilter = "All Files (*.*)|*.*||" ;
	if(!pDHI.m_Work_Dir.IsEmpty())
	{
		str = pDHI.m_Work_Dir;
		str += "*.*";
	}
	else
		str = "*.*";
	CFileDialog dlg(TRUE,NULL,str,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);	
	int reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		str = dlg.GetPathName ();
		
		str.Replace(dlg.GetFileName (),"");
		m_WorkDir.SetWindowText(str);

		
	}
	*/

	CString strDir;
  	CParam* pSheet = (CParam*) GetParent();	

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
				m_WorkDir.SetWindowText(strDir);

            }
            // Free the PIDL allocated by SHBrowseForFolder.
            pMalloc->Free(pidl);
        }
        // Release the shell's allocator.
        pMalloc->Release();
    }
	
	/*typedef struct _SECURITY_ATTRIBUTES { // sa 
		DWORD  nLength; 
		LPVOID lpSecurityDescriptor; 
		BOOL   bInheritHandle; 
	} SECURITY_ATTRIBUTES; 
*/

}




void CParam::UploadFile(int head)
{
	CString filename,file2beupld, str;
	int a;
	file2beupld = "startup.rpt";
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	filename.Format("%sHead %d startup.rpt",pDHI.m_Work_Dir, head);
	a = pDHI.Upload(filename,file2beupld,head);
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	if (a==0)
	{
		filename.Format("notepad %sHead %d startup.rpt",pDHI.m_Work_Dir, head);
		WinExec(filename,TRUE);
	}
	else
	{
		if(a<0 && a>MAXERRORCODE)
			str.Format("%s",e_short_text[-1*a]);
		else
			str = "Unknown Error";
		MessageBox(str,"Download Error",MB_OK);
	}
	
}



