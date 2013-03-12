#if !defined(AFX_CRYSTALTIME_H__14B533CD_E3AF_4B11_95A8_3022AEF1B4A2__INCLUDED_)
#define AFX_CRYSTALTIME_H__14B533CD_E3AF_4B11_95A8_3022AEF1B4A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CrystalTime.h : header file
//

#include "DHI_Constants.h"

/////////////////////////////////////////////////////////////////////////////
// CCrystalTime dialog

class CCrystalTime : public CDialog
{
// Construction
public:
	CCrystalTime(CWnd* pParent = NULL);   // standard constructor

// Public Data
	long CrystalNum;
	long TimingBins;
	long XCrystals;
	long YCrystals;
	long State;
	long Scanner;
	long View;

	long Time[256];
	long Data[256 * 256];
	long All[256];

// Dialog Data
	//{{AFX_DATA(CCrystalTime)
	enum { IDD = IDD_CRYSTAL_TIME };
	CButton	m_ResetButton;
	CButton	m_OKButton;
	CButton	m_PrevButton;
	CButton	m_NextButton;
	CStatic	m_OffsetText;
	CStatic	m_CrystalText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCrystalTime)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Refresh();

	HBITMAP hGreyMap[256];
	HDC GreyColor[256];
	HBITMAP m_Time_bm;
	HDC m_Time_dc;
	HBITMAP m_Offset_bm;
	HDC m_Offset_dc;
	HBITMAP m_red_bm;
	HDC m_red_dc;

	long m_Off_X;
	long m_Off_Y;
	long m_Time_X;
	long m_Time_Y;
	long m_Plot_X;
	long m_Plot_Y;
	long Border_X1;
	long Border_X2;
	long Border_Y1;
	long Border_Y2;
	long m_PlotRange;

	long m_OrigTime[MAX_CRYSTALS];

	// Generated message map functions
	//{{AFX_MSG(CCrystalTime)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnNext();
	afx_msg void OnPrevious();
	afx_msg void OnReset();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRYSTALTIME_H__14B533CD_E3AF_4B11_95A8_3022AEF1B4A2__INCLUDED_)
