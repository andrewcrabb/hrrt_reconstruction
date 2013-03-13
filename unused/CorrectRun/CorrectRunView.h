// CorrectRunView.h : interface of the CCorrectRunView class
//
/////////////////////////////////////////////////////////////////////////////
#include "UserIn.h"

#if !defined(AFX_CORRECTRUNVIEW_H__8CCCDE6F_7636_11D5_BF5D_00105A12EF2F__INCLUDED_)
#define AFX_CORRECTRUNVIEW_H__8CCCDE6F_7636_11D5_BF5D_00105A12EF2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCorrectRunDoc;

class CCorrectRunView : public CFormView
{
protected: // create from serialization only
	CCorrectRunView();
	DECLARE_DYNCREATE(CCorrectRunView)

public:
	//{{AFX_DATA(CCorrectRunView)
	enum { IDD = IDD_CORRECTRUN_FORM };
	CProgressCtrl	m_ProgBar;
	CEdit	m_Radius;
	CEdit	m_PerLbl;
	CEdit	m_Strength;
	CEdit	m_Activity;
	CEdit	m_Status;
	CEdit	m_AvgPoint;
	CEdit	m_CalFactor;
	CButton	m_FindEdge;
	//}}AFX_DATA
	
// Attributes
public:

	CCorrectRunDoc* GetDocument();
	CUserIn	m_Userdlg;
// Operations
public:
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCorrectRunView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ProcessButton();
	CCorrectRunDoc* pDoc;
	void ProcessBar(int i);
	DWORD RenderData(int pltopt, double num);
	//void UpdateBitMap(CDC* pDC);
	void UpdateBitMap();
	int m_GrdDiv;
	int m_LnLg;
	int m_zLinScl;
	int m_DisType;
	double m_logz[256];
	int Zlr(int ii, int jj);
	void Draw3D();
	void DefineScale();
	bool m_EnblUserMenu;
	void DefineColors();
	COLORREF Color[256];
	COLORREF m_cr;
	int m_dxyc;
	int m_ClrRes;
	int m_XOffSet;
	int m_YOffSet;	

	virtual ~CCorrectRunView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CCorrectRunView)
	afx_msg void OnUserDialog();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnUpdateUserDialog(CCmdUI* pCmdUI);
	afx_msg void OnControlPlotopt();
	afx_msg void OnUpdateControlPlotopt(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAppExit(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in CorrectRunView.cpp
inline CCorrectRunDoc* CCorrectRunView::GetDocument()
   { return (CCorrectRunDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CORRECTRUNVIEW_H__8CCCDE6F_7636_11D5_BF5D_00105A12EF2F__INCLUDED_)
