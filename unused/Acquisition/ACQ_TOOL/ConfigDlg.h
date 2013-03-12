#if !defined(AFX_CONFIGDLG_H__8C509B91_675F_4B0E_9239_8047AFE20805__INCLUDED_)
#define AFX_CONFIGDLG_H__8C509B91_675F_4B0E_9239_8047AFE20805__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigDlg.h : header file
//
#include "scanitdoc.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CDialog
{
// Construction
public:

	int m_Ary_inc;
	int m_Old_Scan_Mode;
	int m_Old_Scan_Type;
	int m_Old_uld;
	int m_Old_lld;
	long m_Old_Duration;
	int m_Old_DurType;
	int m_Old_Span;
	int m_Old_Ring;
	float m_Old_Speed;
	int m_Old_Plane;	
	CScanItDoc* pDoc;
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_CONFIG_DLG };
	CButton	m_ContScan;
	CButton	m_Auto_Start;
	CButton	m_64bit;
	CEdit	m_Sino;
	CButton	m_Chk_Seg;
	CSpinButtonCtrl	m_Spn_Rng_Opt;
	CComboBox	m_DType;
	CEdit	m_ST_ULD;
	CEdit	m_ST_Spn;
	CEdit	m_ST_SD;
	CEdit	m_ST_Rng;
	CEdit	m_ST_Pln;
	CEdit	m_ST_LLD;
	int		m_Plane;
	int		m_Ring;
	int		m_ULD;
	int		m_Span;
	float	m_Speed;
	int		m_Scan_Type;
	int		m_Scan_Mode;
	int		m_Count;
	int		m_Preset;
	long	m_Duration;
	long	m_KCPS;
	int		m_LLD;
	CString	m_DynaFrame;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigDlg)
	afx_msg void OnRadioEm();
	afx_msg void OnRadioTx();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeEdit1();
	afx_msg void OnCheck1();
	afx_msg void OnRadioScanMode();
	afx_msg void OnRadioScanHst();
	afx_msg void OnRadioPreset();
	afx_msg void OnRadioPresetCnt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGDLG_H__8C509B91_675F_4B0E_9239_8047AFE20805__INCLUDED_)
