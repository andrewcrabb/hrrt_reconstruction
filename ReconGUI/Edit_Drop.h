#if !defined(AFX_EDIT_DROP_H__26D10718_5CCB_4578_BD93_803435040E75__INCLUDED_)
#define AFX_EDIT_DROP_H__26D10718_5CCB_4578_BD93_803435040E75__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Edit_Drop.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEdit_Drop window

class CEdit_Drop : public CEdit
{
// Construction
public:
	CEdit_Drop();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEdit_Drop)
	//}}AFX_VIRTUAL

// Implementation
public:
	int InsertPathname(const CString& csFilename);
	virtual ~CEdit_Drop();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEdit_Drop)
		// NOTE - the ClassWizard will add and remove member functions here.
		afx_msg void OnDropFiles(HDROP dropInfo);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDIT_DROP_H__26D10718_5CCB_4578_BD93_803435040E75__INCLUDED_)
