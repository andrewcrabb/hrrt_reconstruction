#pragma once
#include "afxcmn.h"
#include "DailyQCView.h"


// CCrystalDlg dialog
class CDailyQCView; 
class CCrystalDlg : public CDialog
{
	DECLARE_DYNAMIC(CCrystalDlg)

public:
	CCrystalDlg(CWnd* pParent);   // standard constructor
	virtual ~CCrystalDlg();
	int m_dHead;
	int m_dBlock;
	CToolTipCtrl * m_pToolTip;

// Dialog Data
	enum { IDD = IDD_CRYSTAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CDailyQCView * pView;
	PlotDataRange range;
protected:
	virtual void PostNcDestroy();
	virtual void OnOK();
public:
	afx_msg void OnClose();
	CSpinButtonCtrl m_HeadSpinCtrl;
	CSpinButtonCtrl m_BlockSpinCtrl;
	afx_msg void OnEnChangeHeadNumber();
	afx_msg void OnEnChangeBlockNumber();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	BOOL ProduceTip( UINT id, NMHDR * pNMHDR, LRESULT * pResult );
	void SetRange(PlotDataRange rangeValue);
};
