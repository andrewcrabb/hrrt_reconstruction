// ScanItView.h : interface of the CScanItView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCANITVIEW_H__33E59443_0218_4C4C_81E4_9EF662CA30F3__INCLUDED_)
#define AFX_SCANITVIEW_H__33E59443_0218_4C4C_81E4_9EF662CA30F3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CScanItDoc;

class CScanItView : public CFormView
{
protected: // create from serialization only
	CScanItView();
	DECLARE_DYNCREATE(CScanItView)

public:
	//{{AFX_DATA(CScanItView)
	enum { IDD = IDD_SCANIT_FORM };
	CEdit	m_SCType;
	CEdit	m_FileName;
	CEdit	m_PName;
	CEdit	m_Tm_Rem;
	CEdit	m_Tot_Events;
	CEdit	m_Trs_Tot;
	CEdit	m_Rnd_Tot;
	CEdit	m_Tm;
	CEdit	m_Sngls;
	CEdit	m_Rndms;
	CEdit	m_Prmpts;
	CEdit	m_Status;
	//}}AFX_DATA

// Attributes
public:
	CScanItDoc* GetDocument();
	CScanItDoc* pDoc;

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScanItView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnDraw(CDC* pDc);
	//}}AFX_VIRTUAL

// Implementation
public:
	void DisableCloseBtn();
	void ProcessButton();
	void HideWindow();
	CPen m_BDPen;
	CFont m_font;
	void DrawMoveWin();
	COLORREF m_cr;
	long m_Move_Xmax;
	long m_Move_Xmin;
	int m_Auto_Range;
	long m_Move_Dur;
	int m_Move_En;
	//long m_Move_Xmax;
	//long m_Move_Xmin;
	//int m_Auto_Range;
	//long m_Move_Dur;
	//int m_Move_En;

	void DrawAxis(CDC* pDc);
	void ProcessIcon(bool a);
	int m_Plt_Tru_Flag;
	int m_Plt_Rnd_Flag;
	int m_Rndm_Clr;
	int m_Trues_Clr;
	//CBrush m_ClrBrush[8];
	CPen m_ClrPen[8];
	//CDC* m_pDC;
	void SaveGraphIniFile();
	long m_Xmin;
	long m_Xmax;
	long m_Ymin;
	long m_Ymax;
	long m_YminLog;
	long m_YmaxLog;
	long m_YminLin;
	long m_YmaxLin;
	int m_LogPlot;
	long nheight;
	long nwidth;
	void ReadGraphIniFile();
	void LinOrLog(long y, long *y1);
	static const COLORREF crColors[8];
	void DrawGraph();
	void DrawTics(CDC* pDc);
	virtual ~CScanItView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CScanItView)
	afx_msg void OnSetupGraphProp();
	afx_msg void OnSetupSysConfig();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnControlConfig();
	afx_msg void OnSetCom();
	afx_msg void OnControlPatient();
	afx_msg void OnHelpTopic();
	afx_msg void OnDiagnosticShow();
	afx_msg void OnDosageInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ScanItView.cpp
inline CScanItDoc* CScanItView::GetDocument()
   { return (CScanItDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCANITVIEW_H__33E59443_0218_4C4C_81E4_9EF662CA30F3__INCLUDED_)
