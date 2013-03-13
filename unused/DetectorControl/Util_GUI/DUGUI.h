// DUGUI.h : main header file for the DUGUI application
//

#if !defined(AFX_DUGUI_H__5872DB9C_4EBB_41A9_BB0E_2473319CBF4E__INCLUDED_)
#define AFX_DUGUI_H__5872DB9C_4EBB_41A9_BB0E_2473319CBF4E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDUGUIApp:
// See DUGUI.cpp for the implementation of this class
//

class CDUGUIApp : public CWinApp
{
public:
	CDUGUIApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDUGUIApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDUGUIApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DUGUI_H__5872DB9C_4EBB_41A9_BB0E_2473319CBF4E__INCLUDED_)
