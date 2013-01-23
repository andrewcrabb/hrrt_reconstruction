//{{AFX_INCLUDES()
#include "calendar.h"
//}}AFX_INCLUDES

#if !defined(AFX_USERIN_H__85A63A12_7D24_11D5_BF62_00105A12EF2F__INCLUDED_)
#define AFX_USERIN_H__85A63A12_7D24_11D5_BF62_00105A12EF2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UserIn.h : header file
//

class CCorrectRunDoc; // Forward declairation
/////////////////////////////////////////////////////////////////////////////
// CUserIn dialog

class CUserIn : public CDialog
{
// Construction
public:
	CUserIn(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUserIn)
	enum { IDD = IDD_USERIN_DIALOG };
	CComboBox	m_cmbHour;
	CComboBox	m_cmbRAE;
	CComboBox	m_cmbActive;
	CComboBox	m_cmbVolume;
	float	m_ActivityVlu;
	float	m_VolumeVlu;
	CCalendar	m_MyCalendar;
	int		m_Minute;
	int		m_Second;
	int		m_ClrScale;
	int		m_Plane;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserIn)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUserIn)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	CCorrectRunDoc * m_pCRunpDoc;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERIN_H__85A63A12_7D24_11D5_BF62_00105A12EF2F__INCLUDED_)
