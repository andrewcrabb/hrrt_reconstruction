#pragma once
#include "afxwin.h"
#include "afxdtctl.h"
#include "atltime.h"


// CPhantomDlg dialog

class CPhantomDlg : public CDialog
{
	DECLARE_DYNAMIC(CPhantomDlg)

public:
	CPhantomDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPhantomDlg();

// Dialog Data
	enum { IDD = IDD_PHANTOM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	// unit of radioactivity
	CComboBox m_Unit;
	// assay date
	CDateTimeCtrl m_AssayDate;
	virtual BOOL OnInitDialog();
	// source strength
	float m_Strength;
protected:
	virtual void OnOK();
public:
	// stored CTime value of assay date
	CTime m_AssayTime;
	// file name of mater record QC
	CString m_sMRQC;
	afx_msg void OnBnClickedBrowse();
	// radio button for master record
	int m_dRadioMRQC;
	afx_msg void OnBnClickedCreatemrqc();
	afx_msg void OnBnClickedSelectmrqc();
};
