#if !defined(AFX_CRYSTALPEAKS_H__15618768_DB99_41BA_A82C_1A9C10FC438B__INCLUDED_)
#define AFX_CRYSTALPEAKS_H__15618768_DB99_41BA_A82C_1A9C10FC438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CrystalPeaks.h : header file
//

#include "DHI_Constants.h"

/////////////////////////////////////////////////////////////////////////////
// CCrystalPeaks dialog

class CCrystalPeaks : public CDialog
{
// Construction
public:
	CCrystalPeaks(CWnd* pParent = NULL);   // standard constructor

// Public Data
	long CrystalSmooth;
	long CrystalNum;
	long XCrystals;
	long YCrystals;
	long State;
	long ScannerType;
	long ScannerLLD;
	long ScannerULD;

	long Peak[256];
	long Data[256 * 256];

// Dialog Data
	//{{AFX_DATA(CCrystalPeaks)
	enum { IDD = IDD_CRYSTALPEAKS };
	CSpinButtonCtrl	m_LLD_Spin;
	CStatic	m_LLD_Text;
	CSpinButtonCtrl	m_ULD_Spin;
	CStatic	m_ULD_Text;
	CButton	m_ResetButton;
	CButton	m_OKButton;
	CSpinButtonCtrl	m_SmoothSpin;
	CButton	m_PrevButton;
	CButton	m_NextButton;
	CStatic	m_SmoothText;
	CStatic	m_CrystalText;
	CStatic	m_PeakText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCrystalPeaks)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Refresh();

	HBITMAP hGreyMap[256];
	HDC GreyColor[256];
	HBITMAP m_en_bm;
	HDC m_en_dc;
	HBITMAP m_pk_bm;
	HDC m_pk_dc;
	HBITMAP m_red_bm;
	HDC m_red_dc;

	long m_Pk_X;
	long m_Pk_Y;
	long m_En_X;
	long m_En_Y;
	long m_Plot_X;
	long m_Plot_Y;
	long Border_X1;
	long Border_X2;
	long Border_Y1;
	long Border_Y2;

	long m_OrigPeak[MAX_CRYSTALS];

	// Generated message map functions
	//{{AFX_MSG(CCrystalPeaks)
	afx_msg void OnDeltaposSmoothSpin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPrevious();
	afx_msg void OnNext();
	afx_msg void OnReset();
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnDeltaposUldSpin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposLldSpin(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRYSTALPEAKS_H__15618768_DB99_41BA_A82C_1A9C10FC438B__INCLUDED_)
