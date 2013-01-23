#if !defined(AFX_ENERGYRANGE_H__AC9A1855_3155_4ECA_A9DF_044F7E2D06EB__INCLUDED_)
#define AFX_ENERGYRANGE_H__AC9A1855_3155_4ECA_A9DF_044F7E2D06EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EnergyRange.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// EnergyRange dialog

class EnergyRange : public CDialog
{
// Construction
public:
	EnergyRange(CWnd* pParent = NULL);   // standard constructor

// public data
	long Accept;
	long RangeView;
	long RangePP_lld;
	long RangePP_uld;
	long RangeEN_lld;
	long RangeEN_uld;
	long RangeTE_lld;
	long RangeTE_uld;
	long RangeSD_lld;
	long RangeSD_uld;
	long RangeRun_lld;
	long RangeRun_uld;
	long ScannerType;

// Dialog Data
	//{{AFX_DATA(EnergyRange)
	enum { IDD = IDD_ENERGY_RANGE };
	CComboBox	m_ViewDropRange;
	CEdit	m_ULD;
	CEdit	m_LLD;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(EnergyRange)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(EnergyRange)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnKillfocusUld();
	afx_msg void OnKillfocusLld();
	afx_msg void OnSelchangeViewdropRange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENERGYRANGE_H__AC9A1855_3155_4ECA_A9DF_044F7E2D06EB__INCLUDED_)
