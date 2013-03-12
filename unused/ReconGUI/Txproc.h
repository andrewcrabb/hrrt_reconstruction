/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				 MAP-TR integration
 */
#if !defined(AFX_TXPROC_H__1EF82613_F77A_4C6F_82A2_ED8325135AEC__INCLUDED_)
#define AFX_TXPROC_H__1EF82613_F77A_4C6F_82A2_ED8325135AEC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Txproc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTxproc dialog
#include "Edit_Drop.h"
#include "List_Drop.h"
#include "afxwin.h"

class CTxproc : public CDialog
{
// Construction
public:
//	CString m_Atten_Correction;
	CTxproc(CWnd* pParent = NULL);   // standard constructor
	void update_segment_UI();		// Update the segmentation UI w.r.t. selected method
									// MAP-TR segmentation is based on priors and other methods on thresholds

// Dialog Data
	//{{AFX_DATA(CTxproc)
	enum { IDD = IDD_TXPROC };
	CButton	m_MU_scale_Wnd;   // Enable auto-scaling Button
	CEdit	m_MU_Peak_Wnd;	  // Mu Peak value window
	CStatic	m_PeakLabel;	  // Mu Peak label window
	CEdit	m_PriorEdit;	  // Current prior parameter value window
	CComboBox	m_PriorSelection; // Prior parameter selection list window
	CButton	m_Segmentation;
	CList_Drop	m_Blank_List;
	CList_Drop	m_Tx_List;
	CEdit_Drop	m_2D_Atten;
	CEdit_Drop	m_Mu_Map;
	CButton	m_Rebin;
	int		m_Method;		//0=UWOSEM, 1=NEC, 2=MAP-TR
	float	m_Bone_Seg;
	float	m_Water_Seg;
	float	m_Noise_Seg;
	CString	m_MU_ImageSize;  // 128 or 256
	BOOL	m_MU_scale;     // auto-scaling flag set by auto-scaling Button
	CString	m_MAP_prior;    // Current prior parameter value
	float	m_MU_peak;		// Mu Peak value
	float	m_MAP_smoothing; // MAP-TR Beta smoothing
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTxproc)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTxproc)
	afx_msg void OnButton2();
	afx_msg void OnButton4();
	virtual void OnOK();
	afx_msg void OnButton3();
	afx_msg void OnButton5();
	virtual BOOL OnInitDialog();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButton6();
	afx_msg void OnMap_TR();     // MAP-TR method selection callback
	afx_msg void OnRadio1();	 // UWOSEM method selection callback
	afx_msg void OnRadio2();	 // NEC method selection callback
	afx_msg void OnSelchangePrior(); // MAP-TR prior selection change callback
	afx_msg void OnKillfocusEdit3(); // mu-map filename change callback
	afx_msg void OnMuSegButton();	 // Enable segmentation button callback
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TXPROC_H__1EF82613_F77A_4C6F_82A2_ED8325135AEC__INCLUDED_)
