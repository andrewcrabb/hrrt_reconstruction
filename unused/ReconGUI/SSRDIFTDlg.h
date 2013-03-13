/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				Cluster integration
 */
#if !defined(AFX_SSRDIFTDLG_H__6CA1CE05_9598_4E76_B945_C14F1B2A68BD__INCLUDED_)
#define AFX_SSRDIFTDLG_H__6CA1CE05_9598_4E76_B945_C14F1B2A68BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SSRDIFTDlg.h : header file
//
#include "Edit_Drop.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CSSRDIFTDlg dialog

class CSSRDIFTDlg : public CDialog
{
// Construction
public:
	CSSRDIFTDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSSRDIFTDlg)
	enum { IDD = IDD_SSRDIFT };
	CButton	m_ScatButton;		// Scatter Button window
	CButton	m_Rebin;
	CEdit_Drop	m_header;
	CEdit_Drop	m_Scatter;
	CEdit_Drop	m_Image;
	CEdit_Drop	m_3D_Atten;
	CEdit_Drop	m_Normalization;
	CEdit_Drop	m_Emission;
	int		m_Type;
	int		m_Weight_method;
	CString	m_ImageSize;		// Reconstructed image size: 128 or 256
	float	m_Calibration;		// Absolute calibration value in Bq/ml
	BOOL	m_PSF;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSSRDIFTDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int Iteration;
	int SubSets;
	int StartPlane;
	int EndPlane;
	// Generated message map functions
	//{{AFX_MSG(CSSRDIFTDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButton6();
	afx_msg void OnButton10();
	afx_msg void OnButton12();
	afx_msg void OnButton11();
	afx_msg void OnButton13();
	afx_msg void OnRadio1();
	afx_msg void OnRadio2();
	afx_msg void OnRadio3();
	afx_msg void OnRadio4();
	afx_msg void OnButton14();
	afx_msg void OnClusterLocation();	// Cluster directory Button callback
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_IterName;
	CEdit m_SubName;
	afx_msg void OnBnClickedRadio4();
	afx_msg void OnRadio9();
	CEdit m_Iteration;
	CEdit m_SubSets;
	CEdit m_EndPlane;
	CEdit_Drop m_Cluster_Dir;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SSRDIFTDLG_H__6CA1CE05_9598_4E76_B945_C14F1B2A68BD__INCLUDED_)
