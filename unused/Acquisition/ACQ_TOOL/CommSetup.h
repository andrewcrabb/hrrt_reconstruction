#if !defined(AFX_COMMSETUP_H__416A11FB_5871_4FEB_A3AD_9B47B331CA7B__INCLUDED_)
#define AFX_COMMSETUP_H__416A11FB_5871_4FEB_A3AD_9B47B331CA7B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CommSetup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCommSetup dialog

class CCommSetup : public CDialog
{
// Construction
public:
	CCommSetup(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCommSetup)
	enum { IDD = IDD_COMMUNICATION };
	CComboBox	m_Com_Port;
	int		m_Buad;
	int		m_Data;
	int		m_Parity;
	int		m_Stop;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommSetup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCommSetup)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMSETUP_H__416A11FB_5871_4FEB_A3AD_9B47B331CA7B__INCLUDED_)
