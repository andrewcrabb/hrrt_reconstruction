#if !defined(AFX_INFODIALOG_H__B19E1FA9_6F3F_409E_BD02_7057A190346D__INCLUDED_)
#define AFX_INFODIALOG_H__B19E1FA9_6F3F_409E_BD02_7057A190346D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InfoDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// InfoDialog dialog

class InfoDialog : public CDialog
{
// Construction
public:
	char	Message[1024];
	void	ShowMessage();
	InfoDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(InfoDialog)
	enum { IDD = IDD_INFO_DIALOG };
	CStatic	m_MsgStr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(InfoDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(InfoDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INFODIALOG_H__B19E1FA9_6F3F_409E_BD02_7057A190346D__INCLUDED_)
