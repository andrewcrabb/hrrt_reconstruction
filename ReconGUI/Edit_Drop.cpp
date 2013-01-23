// Edit_Drop.cpp : implementation file
//

#include "stdafx.h"
#include "ReconGUI.h"
#include "Edit_Drop.h"
#include "ssrdiftdlg.h"
#include "ReconGUIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HWND gbl_hWnd;
extern int gbl_dlgFlag;

/////////////////////////////////////////////////////////////////////////////
// CEdit_Drop

CEdit_Drop::CEdit_Drop()
{
}

CEdit_Drop::~CEdit_Drop()
{
}

BEGIN_MESSAGE_MAP(CEdit_Drop, CEdit)
	//{{AFX_MSG_MAP(CEdit_Drop)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_WM_DROPFILES()

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEdit_Drop message handlers

void CEdit_Drop::OnDropFiles(HDROP dropInfo)
{
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

	InsertPathname(csPathname);


	if (m_hWnd	== gbl_hWnd)
	{
		if (gbl_dlgFlag == 3)
		{
			CSSRDIFTDlg * pDlg = dynamic_cast<CSSRDIFTDlg*>(GetParent());	
			CFile fl(szFilename, CFile::shareDenyNone );
			int len;
			CString strName, strPath, str;
			//fl.SetFilePath( szFilename);

			strPath = fl.GetFilePath();

			strPath.Replace(fl.GetFileName(),NULL);

			strName = fl.GetFileName();
			strPath += strName;
			str += strPath;

			len = str.GetLength();		
			
			if (pDlg->m_Type == 0) 
				str.Format("%s_SD.i",str.Mid(0,len-2));
				
			else if (pDlg->m_Type == 1)
				str.Format("%s_FD.i",str.Mid(0,len-2));
			else if (pDlg->m_Type == 2)
				str.Format("%s_FO.i",str.Mid(0,len-2));
			else if (pDlg->m_Type == 4)
				str.Format("%s_3D.i",str.Mid(0,len-2));
			else if (pDlg->m_Type == 5)
				str.Format("%s_FH.i",str.Mid(0,len-2));

			pDlg->m_Image.SetWindowText(str);

		}
/*		if (gbl_dlgFlag == 2)
		{
			//CScatter_Config * pDlg = dynamic_cast<CScatter_Config*>(GetParent());	
			CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	

			CFile fl(szFilename, CFile::shareDenyNone );
			int len;
			CString strName, strPath, str;
			//fl.SetFilePath( szFilename);

			strPath = fl.GetFilePath();

			strPath.Replace(fl.GetFileName(),NULL);

			strName = fl.GetFileName();
			strPath += strName;
			str += strPath;

			len = str.GetLength();	
			str.Format("%s.i",str.Mid(0,len-2));

			pDlg->m_Image =str;
		}
*/
	}

	//
	// Free the dropped-file info that was allocated by windows
	//
	DragFinish(dropInfo);
}

int CEdit_Drop::InsertPathname(const CString &csFilename)
{
	
	SetWindowText(csFilename);
	return 1;
	
}
