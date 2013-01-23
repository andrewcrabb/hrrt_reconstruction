// QCGUIDlg.h : header file
//

#if !defined(AFX_QCGUIDLG_H__4CE989EC_1043_4F7F_B9E9_555D28588B98__INCLUDED_)
#define AFX_QCGUIDLG_H__4CE989EC_1043_4F7F_B9E9_555D28588B98__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CQCGUIDlg dialog

class CQCGUIDlg : public CDialog
{
// Construction
public:
	CQCGUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CQCGUIDlg)
	enum { IDD = IDD_QCGUI_DIALOG };
	CComboBox	m_LayerDrop;
	CStatic	m_Head_9_Label;
	CStatic	m_Head_8_Label;
	CStatic	m_Head_7_Label;
	CStatic	m_Head_6_Label;
	CStatic	m_Head_5_Label;
	CStatic	m_Head_4_Label;
	CStatic	m_Head_3_Label;
	CStatic	m_Head_2_Label;
	CStatic	m_Head_15_Label;
	CStatic	m_Gantry_Label;
	CStatic	m_Head_13_Label;
	CStatic	m_Head_14_Label;
	CStatic	m_Head_12_Label;
	CStatic	m_Head_11_Label;
	CStatic	m_Head_10_Label;
	CStatic	m_Head_1_Label;
	CStatic	m_Head_0_Label;
	CStatic	m_Message;
	CComboBox	m_ViewDrop;
	CButton	m_AbortButton;
	CButton m_ExitButton;
	CButton m_StartButton;
	CStatic	m_DateLabel;
	CComboBox	m_HeadDrop;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQCGUIDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Update_Status();
	void Update_Master();
	void Update_Current();
	void Update();
	void Refresh();

	HICON m_hIcon;
	HBITMAP hColorBlock[256];
	HDC hdcColor[256];
	HBITMAP m_bm;
	HDC m_dc;
	HBITMAP m_red_bm;
	HDC m_red_dc;
	HBITMAP m_yellow_bm;
	HDC m_yellow_dc;
	HBITMAP m_green_bm;
	HDC m_green_dc;

	// Generated message map functions
	//{{AFX_MSG(CQCGUIDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnExit();
	afx_msg void OnStart();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeHeadDrop();
	afx_msg LRESULT OnPostProgress(WPARAM, LPARAM);
	afx_msg LRESULT OnProcessingComplete(WPARAM, LPARAM);
	afx_msg void OnAbort();
	afx_msg void OnSelchangeViewDrop();
	afx_msg void OnSelchangeLayerDrop();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QCGUIDLG_H__4CE989EC_1043_4F7F_B9E9_555D28588B98__INCLUDED_)
