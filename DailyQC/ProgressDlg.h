#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#define PROCESS_IDLE			0
#define PROCESS_GOING			1
#define PROCESS_CANCEL			2
#define PROCESS_FAIL			3
#define PROCESS_OUTSYNC			4

// CProgressDlg dialog

class CProgressDlg : public CDialog
{
	DECLARE_DYNAMIC(CProgressDlg)

public:
	CProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressDlg();

// Dialog Data
	enum { IDD = IDD_PROGRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_ProgCtrl;
	CButton m_button;
	virtual BOOL OnInitDialog();
protected:
	virtual void OnCancel();
public:
	// processing status
	int m_dProcess;
	// background thread for analysis
	CWinThread* pThread;
};
