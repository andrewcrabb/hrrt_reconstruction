#if !defined(AFX_DOSAGEINFO_H__19FD08B6_406D_4566_B3F3_867645FE4354__INCLUDED_)
#define AFX_DOSAGEINFO_H__19FD08B6_406D_4566_B3F3_867645FE4354__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DosageInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDosageInfo dialog

#include "ScanItDoc.h"

class CDosageInfo : public CDialog
{
// Construction
public:
	CDosageInfo(CWnd* pParent = NULL);   // standard constructor
	CScanItDoc* pDoc;

// Dialog Data
	//{{AFX_DATA(CDosageInfo)
	enum { IDD = IDD_DIALOG1 };
		// NOTE: the ClassWizard will add data members here
	CComboBox	m_cmbStrength;
	CComboBox	m_Iso_Dose;
	CListBox	m_ctrlList;
	float	m_Strength;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDosageInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDosageInfo)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOSAGEINFO_H__19FD08B6_406D_4566_B3F3_867645FE4354__INCLUDED_)
