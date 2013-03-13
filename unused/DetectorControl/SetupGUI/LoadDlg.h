#if !defined(AFX_LOADDLG_H__6087DB70_65D5_4A8B_AFED_41B7E6BC5D63__INCLUDED_)
#define AFX_LOADDLG_H__6087DB70_65D5_4A8B_AFED_41B7E6BC5D63__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoadDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg dialog

class CLoadDlg : public CDialog
{
// Construction
public:
	CLoadDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoadDlg)
	enum { IDD = IDD_LOAD_DIALOG };
	CButton	m_SaveButton;
	CButton	m_LoadButton;
	CButton	m_DeleteButton;
	CButton	m_CancelButton;
	CStatic	m_Message;
	CComboBox	m_LoadConfig;
	CListBox	m_ChooseArchive;
	CListBox	m_ChooseHead;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpDateArchive();

	// Generated message map functions
	//{{AFX_MSG(CLoadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeLoadConfig();
	afx_msg void OnSelchangeHeadList();
	afx_msg void OnLoad();
	afx_msg void OnDelete();
	afx_msg void OnAbort();
	afx_msg void OnSave();
	afx_msg void OnDestroy();
	//}}AFX_MSG

	// user defined
	afx_msg LRESULT OnPostUpdate(WPARAM, LPARAM);
	afx_msg LRESULT OnArchiveComplete(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOADDLG_H__6087DB70_65D5_4A8B_AFED_41B7E6BC5D63__INCLUDED_)
