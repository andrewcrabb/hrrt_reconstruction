/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				Integrate e7_tools
 */
#if !defined(AFX_ATTEN_PROCESS_H__7987BBF4_5A09_4ECC_A1EA_8CC3A858B87F__INCLUDED_)
#define AFX_ATTEN_PROCESS_H__7987BBF4_5A09_4ECC_A1EA_8CC3A858B87F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Atten_Process.h : header file
//
#include "Edit_Drop.h"

/////////////////////////////////////////////////////////////////////////////
// CAtten_Process dialog

class CAtten_Process : public CDialog
{
// Construction
public:
	CAtten_Process(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAtten_Process)
	enum { IDD = IDD_ATTENUATION };
	CEdit_Drop	m_3D_Atten;
	CEdit_Drop	m_Mu_Map;  // Atten3D is now created from Mu-Map using e7_fwd.
	                       // It was previousely created from atten-2d wiith reconEng
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAtten_Process)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAtten_Process)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButton6();
	afx_msg void OnButton7();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTEN_PROCESS_H__7987BBF4_5A09_4ECC_A1EA_8CC3A858B87F__INCLUDED_)
