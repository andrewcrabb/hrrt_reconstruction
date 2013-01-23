// CorrectRun.h : main header file for the CORRECTRUN application
//

#if !defined(AFX_CORRECTRUN_H__8CCCDE67_7636_11D5_BF5D_00105A12EF2F__INCLUDED_)
#define AFX_CORRECTRUN_H__8CCCDE67_7636_11D5_BF5D_00105A12EF2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunApp:
// See CorrectRun.cpp for the implementation of this class
//

class CCorrectRunApp : public CWinApp
{
public:
	CCorrectRunApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCorrectRunApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CCorrectRunApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CORRECTRUN_H__8CCCDE67_7636_11D5_BF5D_00105A12EF2F__INCLUDED_)
