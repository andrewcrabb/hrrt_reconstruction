#if !defined(AFX_DIAGNOSTIC_H__59B2A915_B093_45EA_88AC_2A1EDE4B1AAE__INCLUDED_)
#define AFX_DIAGNOSTIC_H__59B2A915_B093_45EA_88AC_2A1EDE4B1AAE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Diagnostic.h : header file
//

#include "ScanItDoc.h"
#include "process.h"

/////////////////////////////////////////////////////////////////////////////
// CDiagnostic dialog

class CDiagnostic : public CDialog
{
// Construction
public:
	bool m_diagFlag[9];
	bool ProcessCmd();
	void UploadFile(int head);
	CString m_UploadFile;
	bool m_upload[9];
	bool m_heads[9];
	int m_cmd;
	void InitThread();
	void DisplayError(int ErrNum);
	CString m_sourceName;
	CString m_targetName;
	//CString m_cmbVar[10];
	CDiagnostic(CWnd* pParent = NULL);   // standard constructor
	unsigned int tid;
	HANDLE hThread;
	CScanItDoc* pDoc;
	#define MAX_EVENTS 10240
	long event_counter;
	long sync_error_counter;

	typedef struct
	{
	   unsigned char out2;
	   unsigned char out1;
	   unsigned char out0;
	   unsigned char head;
	} event_word32;

	event_word32 ew_low;
	event_word32 ew_high;

	int list_pe_test(FILE *in, FILE *out, int rebinner);
	int get_event(FILE *in, FILE *out, event_word32 *ew, int rebinner, int include_tag);


private:
	static unsigned int WINAPI BkGndProc(void *p);
// Dialog Data
	//{{AFX_DATA(CDiagnostic)
	enum { IDD = IDD_DIAGNOSTIC };
	CEdit	m_DOSCmd;
	CListBox	m_StatList;
	CEdit	m_EditFile;
	CEdit	m_SName;
	CComboBox	m_cmbHead;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDiagnostic)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDiagnostic)
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	virtual BOOL OnInitDialog();
	afx_msg void OnButton3();
	afx_msg void OnButtonReset();
	afx_msg void OnButtonId();
	afx_msg void OnButtonPid();
	afx_msg void OnButtonAsd();
	virtual void OnOK();
	afx_msg void OnButtonLpe();
	afx_msg void OnButtonDos();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIAGNOSTIC_H__59B2A915_B093_45EA_88AC_2A1EDE4B1AAE__INCLUDED_)
