#pragma once


// CMRQCDlg dialog

class CMRQCDlg : public CDialog
{
	DECLARE_DYNAMIC(CMRQCDlg)

public:
	CMRQCDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMRQCDlg();

// Dialog Data
	enum { IDD = IDD_MRQC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_sFileName;
	afx_msg void OnBnClickedMrqcBrowse();
};
