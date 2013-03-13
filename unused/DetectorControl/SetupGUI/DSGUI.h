// DSGUI.h : main header file for the DSGUI application
//

#if !defined(AFX_DSGUI_H__8139F8DD_1977_401C_A0BB_E3A5BB758F72__INCLUDED_)
#define AFX_DSGUI_H__8139F8DD_1977_401C_A0BB_E3A5BB758F72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDSGUIApp:
// See DSGUI.cpp for the implementation of this class
//

class CDSGUIApp : public CWinApp
{
public:
	CDSGUIApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDSGUIApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDSGUIApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DSGUI_H__8139F8DD_1977_401C_A0BB_E3A5BB758F72__INCLUDED_)
