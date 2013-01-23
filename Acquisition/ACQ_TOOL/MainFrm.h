// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__669B61F8_E464_4EE9_AD07_26859E4DBF02__INCLUDED_)
#define AFX_MAINFRM_H__669B61F8_E464_4EE9_AD07_26859E4DBF02__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void EnDisMenu(CCmdUI* pCmdUI);
	void UpdateButtons();
	CRect r;
	CRect r1;
	CStatic st;
	void UpdateIcon(bool a);
	virtual ~CMainFrame();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CToolBar    m_wndToolBar;
	CStatusBar  m_wndStatusBar;
// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateProcessStart(CCmdUI* pCmdUI);
	afx_msg void OnUpdateProcessStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateProcessAbort(CCmdUI* pCmdUI);
	afx_msg void OnProcessStart();
	afx_msg void OnProcessStop();
	afx_msg void OnProcessAbort();
	afx_msg void OnUpdateAppExit(CCmdUI* pCmdUI);
	afx_msg void OnUpdateControlConfig(CCmdUI* pCmdUI);
	afx_msg void OnUpdateControlPatient(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDiagnosticShow(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSetupCom(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSetupSysConfig(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDosageInfo(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__669B61F8_E464_4EE9_AD07_26859E4DBF02__INCLUDED_)
