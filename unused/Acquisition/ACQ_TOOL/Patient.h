#if !defined(AFX_PATIENT_H__D6F776C8_1B0D_47D2_9D22_BB307835CDFB__INCLUDED_)
#define AFX_PATIENT_H__D6F776C8_1B0D_47D2_9D22_BB307835CDFB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Patient.h : header file
//
#include "ScanItDoc.h"
#include <afxdb.h>
#include <odbcinst.h> 

/////////////////////////////////////////////////////////////////////////////
// CPatient dialog

class CPatient : public CDialog
{
// Construction
public:
	BOOL ShowDataLink( _bstr_t * bstr_ConnectString );
	void SaveDBConn();
	void GetDBConn();
	CString m_ConnString;
	bool CheckExist();
	void ExeSql(char* sSql);
	CScanItDoc* pDoc;
	CPatient(CWnd* pParent = NULL);   // standard constructor
	CDatabase db;
	CRecordset rs;

// Dialog Data
	//{{AFX_DATA(CPatient)
	enum { IDD = IDD_PATIENT_INFO };
	//CComboBox	m_cmbStrength;
	CButton	m_LNSearch;
	//CComboBox	m_Iso_Dose;
	CListBox	m_ctrlList;
	COleDateTime	m_DOB;
	CString	m_FName;
	CString	m_LName;
	int		m_Sex;
	CString	m_PatientID;
	//float	m_Strength;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPatient)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPatient)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnSearchAll();
	afx_msg void OnLnSearch();
	afx_msg void OnSelchangeList1();
	afx_msg void OnFnSearch();
	afx_msg void OnDobSearch();
	afx_msg void OnDestroy();
	afx_msg void OnIdSearch();
	afx_msg void OnButton1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PATIENT_H__D6F776C8_1B0D_47D2_9D22_BB307835CDFB__INCLUDED_)
