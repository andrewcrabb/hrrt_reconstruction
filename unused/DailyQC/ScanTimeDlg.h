#pragma once
#include "atltime.h"


// CScanTimeDlg dialog

class CScanTimeDlg : public CDialog
{
	DECLARE_DYNAMIC(CScanTimeDlg)

public:
	CScanTimeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CScanTimeDlg();

// Dialog Data
	enum { IDD = IDD_SCANTIME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	// scan date
	CTime m_ScanDate;
	// scan time
	CTime m_ScanTime;
	// scan duration
	ULONG m_ScanDuration;
protected:
	virtual void OnOK();
};
