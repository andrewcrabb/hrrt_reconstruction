#if !defined(AFX_PLOTOPT_H__8299D69C_8A9A_47B6_8281_8BFF19C2A482__INCLUDED_)
#define AFX_PLOTOPT_H__8299D69C_8A9A_47B6_8281_8BFF19C2A482__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PlotOpt.h : header file
//
class CCorrectRunDoc;
class CCorrectRunView;

/////////////////////////////////////////////////////////////////////////////
// CPlotOpt dialog

class CPlotOpt : public CDialog
{
// Construction
public:
	CPlotOpt(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPlotOpt)
	enum { IDD = IDD_GRAPH_OPTION };
	CButton	m_FindEdge;
	CButton	m_HighLight;
	int		m_ClrScale;
	int		m_plane;
	float	m_Radius;
	int		m_DisType;
	int		m_LinLog;
	float	m_Edge;
	float	m_InWord;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlotOpt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CCorrectRunDoc* m_pDoc;
	CCorrectRunView* m_pView;
	// Generated message map functions
	//{{AFX_MSG(CPlotOpt)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnGraphType();
	afx_msg void OnGraphType2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PLOTOPT_H__8299D69C_8A9A_47B6_8281_8BFF19C2A482__INCLUDED_)
