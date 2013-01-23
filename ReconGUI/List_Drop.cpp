// List_Drop.cpp : implementation file
//

#include "stdafx.h"
#include "ReconGUI.h"
#include "List_Drop.h"
#include "TxProc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HWND gbl_hWnd;
extern int gbl_dlgFlag;

/////////////////////////////////////////////////////////////////////////////
// CList_Drop

CList_Drop::CList_Drop()
{
}

CList_Drop::~CList_Drop()
{
}


BEGIN_MESSAGE_MAP(CList_Drop, CListBox)
	//{{AFX_MSG_MAP(CList_Drop)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CList_Drop message handlers
void CList_Drop::OnDropFiles(HDROP dropInfo)
{
	int cnt, len;
	UINT nNumFilesDropped = DragQueryFile(dropInfo, 0xFFFFFFFF, NULL, 0);
	
	//
	// Iterate through the pathnames and process each one
	//
	TCHAR szFilename[MAX_PATH + 1];
	CString csPathname;
	CString csExpandedFilename;

	for (UINT nFile = 0 ; nFile < nNumFilesDropped; nFile++)
	{
		//
		// Get the pathname
		//
		DragQueryFile(dropInfo, nFile, szFilename, MAX_PATH + 1);

		//
		// It might be shortcut, so try and expand it
		//
		csPathname = szFilename;
		//csExpandedFilename = ExpandShortcut(csPathname);
	}
	cnt = GetCount();
	if (cnt< 5)
		InsertPathname(csPathname);
	
	if (m_hWnd	== gbl_hWnd)
	{
		CTxproc * pDlg = dynamic_cast<CTxproc*>(GetParent());	
		CFile fl(szFilename, CFile::shareDenyNone );
		
		CString strName, strPath, str;
		//fl.SetFilePath( szFilename);

		str = fl.GetFileName();
		len = str.GetLength();		
		//strName.Replace(".s", ".i");
		strName.Format("%s.i",str.Mid(0,len-2));

		strPath = fl.GetFilePath();

		strPath.Replace(fl.GetFileName(),NULL);
		

		str.Format("%s%s",strPath, strName);
		pDlg->m_Mu_Map.SetWindowText(str);
		strName.Replace(".i", ".a");
		
		str.Format("%sAtten_2d_%s",strPath, strName);
//		pDlg->m_2D_Atten.SetWindowText(str);
//		pDlg->m_Atten_Correction.Format("%sAt_Correction_%s",strPath, strName);
		//pDlg->m_2D_Atten.GetWindowText(pDlg->m_3D_Atten_File);
		//pDlg->pDlg->m_3D_Atten_File.Replace("Atten_2d_", "Atten_3d_");
		//pDlg->strName.Replace(".a", ".s");
		//pDlg->pDlg->m_Scatter_File.Format("%sScatter_%s",strPath, strName);			
		
	}
	//
	// Free the dropped-file info that was allocated by windows
	//
	DragFinish(dropInfo);
}

int CList_Drop::InsertPathname(const CString &csFilename)
{
	
	AddString(csFilename);
	return 1;
	
}
