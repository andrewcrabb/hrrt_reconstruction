#if !defined(AFX_PARAM_H__A382341E_2D43_4ADD_BFA2_9F874AB81BEE__INCLUDED_)
#define AFX_PARAM_H__A382341E_2D43_4ADD_BFA2_9F874AB81BEE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Param.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CParam dialog

class CParam : public CDialog
{
// Construction
public:
	void UploadFile(int head);
	CParam(CWnd* pParent = NULL);   // standard constructor
	int m_param[100];
	//CScanItDoc* pParent;
	//ICmd_list* m_pGantry;
// Dialog Data
	//{{AFX_DATA(CParam)
	enum { IDD = IDD_SYSTEM_CONFIG };
	CButton	m_msg_err;
	CButton	m_LogFile;
	CButton	m_CID;
	CButton	m_PID;
	CButton	m_Ran_Num;
	CButton	m_Pt_Name;
	CEdit	m_WorkDir;
	CComboBox	m_DHI;
	CComboBox	m_CC;
	CButton	m_HD7;
	CButton	m_HD6;
	CButton	m_HD5;
	CButton	m_HD4;
	CButton	m_HD3;
	CButton	m_HD2;
	CButton	m_HD1;
	CButton	m_HD0;
	int		m_HD;
	int		m_Row;
	int		m_Col;
	int		m_Cnfg;
	CString	m_ScannerType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParam)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CParam)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnButtonDir();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARAM_H__A382341E_2D43_4ADD_BFA2_9F874AB81BEE__INCLUDED_)
