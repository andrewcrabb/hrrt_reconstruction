#if !defined(AFX_HEADSELECTION_H__CB49BD88_0790_4591_AA71_9385B65CBFAA__INCLUDED_)
#define AFX_HEADSELECTION_H__CB49BD88_0790_4591_AA71_9385B65CBFAA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HeadSelection.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// HeadSelection dialog

class HeadSelection : public CDialog
{
// Construction
public:
	HeadSelection(CWnd* pParent = NULL);   // standard constructor

// Transfer Data
	long ScannerType;
	long NumHeads;
	long NumAllow;
	long HeadIndex[16];
	long HeadAllow[16];
	long Selection[16];

// Dialog Data
	//{{AFX_DATA(HeadSelection)
	enum { IDD = IDD_HEAD_SELECT };
	CButton	m_SelectAll;
	CButton	m_OkButton;
	CButton	m_CancelButton;
	CButton	m_Select9;
	CButton	m_Select8;
	CButton	m_Select7;
	CButton	m_Select6;
	CButton	m_Select5;
	CButton	m_Select4;
	CButton	m_Select3;
	CButton	m_Select2;
	CButton	m_Select15;
	CButton	m_Select14;
	CButton	m_Select13;
	CButton	m_Select12;
	CButton	m_Select11;
	CButton	m_Select10;
	CButton	m_Select1;
	CButton	m_Select0;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(HeadSelection)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(HeadSelection)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelectAll();
	afx_msg void OnSelectSingle();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HEADSELECTION_H__CB49BD88_0790_4591_AA71_9385B65CBFAA__INCLUDED_)
