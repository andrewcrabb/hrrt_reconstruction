// DSGUIDlg.h : header file
//
//{{AFX_INCLUDES()
#include "msflexgrid.h"
//}}AFX_INCLUDES

#include "Setup_Constants.h"
#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

#define MAX_COLUMNS 13
#define MAX_ROWS 13

#if !defined(AFX_DSGUIDLG_H__058A0012_5DBF_4DFA_A7C2_4F746F085F8E__INCLUDED_)
#define AFX_DSGUIDLG_H__058A0012_5DBF_4DFA_A7C2_4F746F085F8E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

long Add_Error(CErrorEventSupport* pErrSup, char *Where, char *What, long Why);

/////////////////////////////////////////////////////////////////////////////
// CDSGUIDlg dialog

class CDSGUIDlg : public CDialog
{
// Construction
public:
	CDSGUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDSGUIDlg)
	enum { IDD = IDD_DSGUI_DIALOG };
	CSpinButtonCtrl	m_DelaySpin;
	CEdit	m_DelayField;
	CButton	m_ContinueButton;
	CStatic	m_Status3;
	CStatic	m_Status2;
	CStatic	m_Status1;
	CButton	m_ArchiveButton;
	CButton	m_AbortButton;
	CButton	m_ClearButton;
	CButton	m_ExitButton;
	CButton	m_ResetButton;
	CButton	m_SetButton;
	CButton	m_StartButton;
	CStatic	m_StageLabel;
	CProgressCtrl	m_TotalProgress;
	CProgressCtrl	m_StageProgress;
	CComboBox	m_DropType;
	CComboBox	m_DropEmTx;
	CComboBox	m_DropConfig;
	CComboBox	m_DropHead;
	CListBox	m_HeadList;
	CMSFlexGrid	m_HeadGrid;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDSGUIDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Control_State(BOOL State);
	void Redisplay_Blocks();
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDSGUIDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClickHeadgrid();
	afx_msg void OnExit();
	afx_msg void OnSelchangeHeadlist();
	afx_msg void OnSelchangeDrophead();
	afx_msg void OnClear();
	afx_msg void OnSet();
	afx_msg void OnSelchangeDroptype();
	afx_msg void OnSelchangeDropConfig();
	afx_msg void OnSelchangeDropEmtx();
	afx_msg void OnStart();
	afx_msg void OnReset();
	afx_msg void OnAbort();
	afx_msg void OnArchive();
	afx_msg void OnContinue();
	afx_msg void OnDestroy();
	afx_msg void OnDeltaposDelaySpin(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG

	// user defined
	afx_msg LRESULT OnPostProgress(WPARAM, LPARAM);
	afx_msg LRESULT OnProcessingComplete(WPARAM, LPARAM);
	afx_msg LRESULT OnStateChange(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DSGUIDLG_H__058A0012_5DBF_4DFA_A7C2_4F746F085F8E__INCLUDED_)
