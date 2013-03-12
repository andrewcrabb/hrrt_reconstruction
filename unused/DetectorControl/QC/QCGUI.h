// QCGUI.h : main header file for the QCGUI application
//

#if !defined(AFX_QCGUI_H__5238C41E_2542_4DF7_B703_EBFB42701102__INCLUDED_)
#define AFX_QCGUI_H__5238C41E_2542_4DF7_B703_EBFB42701102__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CQCGUIApp:
// See QCGUI.cpp for the implementation of this class
//

class CQCGUIApp : public CWinApp
{
public:
	CQCGUIApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQCGUIApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CQCGUIApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QCGUI_H__5238C41E_2542_4DF7_B703_EBFB42701102__INCLUDED_)
