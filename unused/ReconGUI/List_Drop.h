#if !defined(AFX_LIST_DROP_H__6B5CB6D1_5460_412C_AAE4_6CC766DBB0C3__INCLUDED_)
#define AFX_LIST_DROP_H__6B5CB6D1_5460_412C_AAE4_6CC766DBB0C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// List_Drop.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CList_Drop window

class CList_Drop : public CListBox
{
// Construction
public:
	CList_Drop();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CList_Drop)
	//}}AFX_VIRTUAL

// Implementation
public:
	int InsertPathname(const CString& csFilename);
	virtual ~CList_Drop();

	// Generated message map functions
protected:
	//{{AFX_MSG(CList_Drop)
		// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnDropFiles(HDROP dropInfo);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIST_DROP_H__6B5CB6D1_5460_412C_AAE4_6CC766DBB0C3__INCLUDED_)
