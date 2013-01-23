#if !defined(AFX_GRAPHPROP_H__C607FD13_C8D8_4DDA_8CD7_11E1BDBA3453__INCLUDED_)
#define AFX_GRAPHPROP_H__C607FD13_C8D8_4DDA_8CD7_11E1BDBA3453__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphProp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGraphProp dialog

class CGraphProp : public CDialog
{
// Construction
public:
	CBrush m_ClrBrush[8];
	void GetManExp(long num, float* man, int* exp);
	CGraphProp(CWnd* pParent = NULL);   // standard constructor
	//CScanItDoc* pParent;
	CScanItView* pParent;
// Dialog Data
	//{{AFX_DATA(CGraphProp)
	enum { IDD = IDD_GRAPH_PROP };
	CButton	m_Chk_Move;
	CSpinButtonCtrl	m_Spn_Clr_Trues;
	CSpinButtonCtrl	m_Spn_Clr_Rndms;
	CEdit	m_Clr_Randoms;
	CEdit	m_Clr_Trues;
	CButton	m_Chk_Trs;
	CButton	m_Chk_Rndms;
	CSpinButtonCtrl	m_Ymax_Spn;
	CEdit	m_Ymax_Exp;
	CEdit	m_Ymin_Exp;
	CSpinButtonCtrl	m_Ymin_Spn;
	float	m_Ymax_Man;
	float	m_Ymin_Man;
	int		m_LinLog;
	long	m_Move;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphProp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGraphProp)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnRadioLin();
	afx_msg void OnRadioLog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDeltaposSpin1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpin2(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPROP_H__C607FD13_C8D8_4DDA_8CD7_11E1BDBA3453__INCLUDED_)
