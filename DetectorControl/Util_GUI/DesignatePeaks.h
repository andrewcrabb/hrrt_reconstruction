#if !defined(AFX_DESIGNATEPEAKS_H__87A5149D_D78B_4DDD_89AA_ED8189886730__INCLUDED_)
#define AFX_DESIGNATEPEAKS_H__87A5149D_D78B_4DDD_89AA_ED8189886730__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DesignatePeaks.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DesignatePeaks dialog

class DesignatePeaks : public CDialog
{
// Construction
public:
	DesignatePeaks(CWnd* pParent = NULL);   // standard constructor

// Transfer Data
	long XCrystals;
	long YCrystals;
	long Accept;
	long Curr;

	long Pk_X[256];
	long Pk_Y[256];
	long pp[256 * 256];

// Dialog Data
	//{{AFX_DATA(DesignatePeaks)
	enum { IDD = IDD_DESIGNATE_PEAKS };
	CButton	m_OKButton;
	CButton	m_CancelButton;
	CButton	m_UndoButton;
	CStatic	m_Count;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DesignatePeaks)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Expand();
	HBITMAP hCRMBlock[256];
	HDC CRMColor[256];
	HBITMAP m_pp_bm;
	HDC m_pp_dc;
	HBITMAP m_pk_bm;
	HDC m_pk_dc;
	HBITMAP m_red_bm;
	HDC m_red_dc;
	HBITMAP m_blue_bm;
	HDC m_blue_dc;

	long m_Win_X;
	long m_Win_Y;

	// Generated message map functions
	//{{AFX_MSG(DesignatePeaks)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnUndo();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DESIGNATEPEAKS_H__87A5149D_D78B_4DDD_89AA_ED8189886730__INCLUDED_)
