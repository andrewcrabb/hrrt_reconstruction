// DUGUIDlg.h : header file
//


// build dependent includes
#ifndef ECAT_SCANNER

#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

// for ring scanners, define a dummy data type for the error handler class
#else

#define CErrorEventSupport void

#endif

#if !defined(AFX_DUGUIDLG_H__4385A8C2_F47C_4CA7_8018_524A44C4FFBD__INCLUDED_)
#define AFX_DUGUIDLG_H__4385A8C2_F47C_4CA7_8018_524A44C4FFBD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

long Add_Error(CErrorEventSupport* pErrSup, char *Where, char *What, long Why);


/////////////////////////////////////////////////////////////////////////////
// CDUGUIDlg dialog

class CDUGUIDlg : public CDialog
{
// Construction
public:
	CDUGUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDUGUIDlg)
	enum { IDD = IDD_DUGUI_DIALOG };
	CButton	m_DefaultsButton;
	CButton	m_AnalyzeButton;
	CButton	m_SaveButton;
	CStatic	m_YOffLabel;
	CStatic	m_YLabel;
	CStatic	m_XLabel;
	CStatic	m_ValueLabel;
	CStatic	m_UnitsLabel;
	CStatic	m_ShapeLabel;
	CStatic	m_LayerLabel;
	CStatic	m_DisplayLabel;
	CStatic	m_CLabel;
	CStatic	m_AmountLabel;
	CStatic	m_ALabel;
	CStatic	m_XOffLabel;
	CStatic	m_ViewLabel;
	CStatic	m_GainLabel;
	CStatic	m_DLabel;
	CStatic	m_ConfigLabel;
	CStatic	m_CFDLabel;
	CStatic	m_BLabel;
	CStatic	m_AcquisitionLabel;
	CStatic	m_ProgressLabel;
	CProgressCtrl	m_AcquireProgress;
	CButton	m_PreviousButton;
	CButton	m_NextButton;
	CButton	m_EditButton;
	CButton	m_AcquireButton;
	CButton	m_AbortButton;
	CStatic	m_AcquireDate;
	CEdit	m_Block;
	CEdit	m_CFD;
	CButton	m_DNLCheck;
	CButton	m_DividerCheck;
	CStatic	m_FastLabel;
	CStatic	m_FastDash;
	CEdit	m_FastHigh;
	CEdit	m_FastLow;
	CButton	m_MasterCheck;
	CButton	m_PeaksCheck;
	CEdit	m_PMTB;
	CEdit	m_PMTA;
	CEdit	m_PMTC;
	CEdit	m_PMTD;
	CButton	m_RegionCheck;
	CEdit	m_Shape;
	CStatic	m_SlowLabel;
	CStatic	m_SlowDash;
	CEdit	m_SlowHigh;
	CEdit	m_SlowLow;
	CStatic	m_SmoothLabel;
	CEdit	m_Value;
	CSliderCtrl	m_SmoothSlider;
	CButton	m_ThresholdCheck;
	CButton	m_TransmissionCheck;
	CComboBox	m_TubeDrop;
	CEdit	m_VariableField;
	CStatic	m_VariableLabel;
	CEdit	m_XOffset;
	CEdit	m_XPos;
	CEdit	m_YOffset;
	CEdit	m_YPos;
	CEdit	m_Amount;
	CComboBox	m_AcqTypeDrop;
	CComboBox	m_LayerDrop;
	CComboBox	m_ViewDrop;
	CComboBox	m_ConfigDrop;
	CComboBox	m_AcqHeadDrop;
	CComboBox	m_HeadDrop;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDUGUIDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Build_Layer_List();
	void Enable_Controls();
	void Disable_Controls();
	void UpdateBlock();
	void UpdateHead();

	HICON m_hIcon;
	HBITMAP hColorBlock[256];
	HDC hdcColor[256];
	HBITMAP m_bmHead;
	HDC m_dcHead;
	HBITMAP m_bmBlock;
	HDC m_dcBlock;

	// Generated message map functions
	//{{AFX_MSG(CDUGUIDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnKillfocusAmount();
	afx_msg void OnDestroy();
	afx_msg void OnPrevious();
	afx_msg void OnNext();
	afx_msg void OnSelchangeAcquireHeaddrop();
	afx_msg void OnFileExit();
	afx_msg void OnDividerCheck();
	afx_msg void OnSelchangeViewdrop();
	afx_msg void OnSelchangeHeaddrop();
	afx_msg void OnSelchangeLayerdrop();
	afx_msg void OnRegionCheck();
	afx_msg void OnSelchangeTubedrop();
	afx_msg void OnReleasedcaptureSmoothSlider(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMasterCheck();
	afx_msg void OnThresholdCheck();
	afx_msg void OnPeaksCheck();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillfocusPmta();
	afx_msg void OnKillfocusPmtb();
	afx_msg void OnKillfocusPmtc();
	afx_msg void OnKillfocusPmtd();
	afx_msg void OnKillfocusXOff();
	afx_msg void OnKillfocusYOff();
	afx_msg void OnKillfocusVariableField();
	afx_msg void OnKillfocusCfd();
	afx_msg void OnKillfocusFastHigh();
	afx_msg void OnKillfocusFastLow();
	afx_msg void OnKillfocusShape();
	afx_msg void OnKillfocusSlowHigh();
	afx_msg void OnKillfocusSlowLow();
	afx_msg void OnEdit();
	afx_msg void OnAcquire();
	afx_msg LRESULT OnPostProgress(WPARAM, LPARAM);
	afx_msg LRESULT OnProcessingComplete(WPARAM, LPARAM);
	afx_msg LRESULT OnProcessingMessage(WPARAM, LPARAM);
	afx_msg void OnAbort();
	afx_msg void OnDownloadCrms();
	afx_msg void OnAcquireMasterTubeEnergy();
	afx_msg void OnSetEnergyRange();
	afx_msg void OnFileArchive();
	afx_msg void OnFileDownloadcrystalenergy();
	afx_msg void OnKillfocusBlock();
	afx_msg void OnSelchangeConfigdrop();
	afx_msg void OnAnalyze();
	afx_msg void OnSave();
	afx_msg void OnDefaults();
	afx_msg void OnFileDiagnostics();
	afx_msg void OnDnlCheck();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DUGUIDLG_H__4385A8C2_F47C_4CA7_8018_524A44C4FFBD__INCLUDED_)
