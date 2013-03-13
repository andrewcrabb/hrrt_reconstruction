#if !defined(AFX_CRMEDIT_H__9CE5F9BA_87C0_44AE_AAA2_A3D1C71E8388__INCLUDED_)
#define AFX_CRMEDIT_H__9CE5F9BA_87C0_44AE_AAA2_A3D1C71E8388__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CRMEdit.h : header file
//
#include "DHI_Constants.h"

#define PROFILE_SIZE (256 * 256)

/////////////////////////////////////////////////////////////////////////////
// CCRMEdit dialog

class CCRMEdit : public CDialog
{
// Construction
public:
	CCRMEdit(CWnd* pParent = NULL);   // standard constructor

// Transfer Data
	long Config;
	long Head;
	long Layer;
	long Block;
	long XBlocks;
	long YBlocks;
	long XCrystals;
	long YCrystals;
	long CrossCheck;
	long CrossDist;
	long EdgeDist;
	long State;
	long Simulation;
	long ScannerType;

	long pp[256 * 256];

// Dialog Data
	//{{AFX_DATA(CCRMEdit)
	enum { IDD = IDD_CRMEDIT };
	CButton	m_UndoButton;
	CButton	m_DesignateButton;
	CButton	m_DownloadButton;
	CButton	m_OrderButton;
	CButton	m_SaveButton;
	CSpinButtonCtrl	m_SpinEdge;
	CSpinButtonCtrl	m_SpinPeaks;
	CButton	m_CrosstalkEdgeButton;
	CButton	m_CrosstalkPeaksButton;
	CButton	m_NumberCheck;
	CStatic	m_EdgeDistance;
	CStatic	m_Distance;
	CButton	m_CrosstalkCheck;
	CButton	m_OKButton;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCRMEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Let_Go();
	void Update_Display(CPoint point);
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
	long m_XPos;
	long m_YPos;
	long m_XOff;
	long m_YOff;
	long m_Verts_X;
	long m_Verts_Y;
	long m_Changed;
	long m_Current;

	long m_Verts[MAX_HEAD_VERTICES*2];
	long m_OrigVerts[MAX_HEAD_VERTICES*2];
	unsigned char m_CRM[PROFILE_SIZE];
	unsigned char m_OrigCRM[PROFILE_SIZE];

	// Generated message map functions
	//{{AFX_MSG(CCRMEdit)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnCrosstalkCheck();
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnUndo();
	afx_msg void OnClose();
	virtual void OnOK();
	afx_msg void OnSave();
	afx_msg void OnCrosstalkButton();
	afx_msg void OnEdgeButton();
	afx_msg void OnDeltaposSpinPeaks(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinEdge(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNumberCheck();
	afx_msg void OnOrderButton();
	afx_msg void OnDesignate();
	afx_msg void OnDownload();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRMEDIT_H__9CE5F9BA_87C0_44AE_AAA2_A3D1C71E8388__INCLUDED_)
