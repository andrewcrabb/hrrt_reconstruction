#if !defined(AFX_SCATTER_CONFIG_H__1F572C40_A9BE_4675_AD57_5020C7EBF6BF__INCLUDED_)
#define AFX_SCATTER_CONFIG_H__1F572C40_A9BE_4675_AD57_5020C7EBF6BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Scatter_Config.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScatter_Config dialog
#include "Edit_Drop.h"

class CScatter_Config : public CDialog
{
// Construction
public:
	CScatter_Config(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CScatter_Config)
	enum { IDD = IDD_SCATTER_CORRECTTION };
	CButton	m_ScatQCButton;
	CButton	m_ScatButton;
	CEdit_Drop	m_QC_file;
	CButton	m_QC;
	CEdit_Drop	m_3D_Atten;
	CEdit_Drop	m_Mu_Map;
	CEdit_Drop	m_Normalization;
	CEdit_Drop	m_Scatter;
	CEdit_Drop	m_Emission;
	UINT	m_LLD;
	UINT	m_ULD;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScatter_Config)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CScatter_Config)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButton6();
	afx_msg void OnButton10();
	afx_msg void OnButton11();
	afx_msg void OnButton12();
	afx_msg void OnButton7();
	afx_msg void OnButton13();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCATTER_CONFIG_H__1F572C40_A9BE_4675_AD57_5020C7EBF6BF__INCLUDED_)
