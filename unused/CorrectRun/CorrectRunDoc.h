// CorrectRunDoc.h : interface of the CCorrectRunDoc class
//

/* HRRT User Community modifications
   29-aug-2008 : Add interframe decay correction
                 Add scanner dependent deadtime constant 
*/

/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_CORRECTRUNDOC_H__8CCCDE6D_7636_11D5_BF5D_00105A12EF2F__INCLUDED_)
#define AFX_CORRECTRUNDOC_H__8CCCDE6D_7636_11D5_BF5D_00105A12EF2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "process.h"

class CCorrectRunView;

class CCorrectRunDoc : public CDocument
{
protected: // create from serialization only
	CCorrectRunDoc();
	DECLARE_DYNCREATE(CCorrectRunDoc)

// Attributes
public:
	//CUserIn	m_Userdlg;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCorrectRunDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCorrectRunDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	float m_BranchFactor;
	CString m_NormFileName;
	long m_Span;
	void FreeAll();
	CString m_appPath;
	void GetConfigFile();
	void GetAppPath(CString *AppPath);
	void SaveConfigFile();
	bool m_ThreadDone;
	double m_NewActivity;
	double m_CalFactor;
	long m_ScanDuration;
	double m_dtc, m_FrameDecay;
	double m_MstPlnAvg;
	int m_MasterPlane;
	float m_Radius;
	void CalFactor();
	int m_plane;
	void StoreFactor();
	bool m_nAlloc;
	bool m_fAlloc;
	void CalPlaneAvg();
	float m_ScaleFactor;
	void RecalAvg();
	double m_TopPercent;
	CString m_MyStr;
	int m_OldClrSelect;
	int m_ClrSelect;
	float m_DisLow;
	float m_DisHigh;
	bool m_CallocFlag;
	int m_Cx;
	int m_Cy;
	int m_OrgYear;
	int m_OrgMonth;
	int m_OrgDay;
	int m_OrgSecond;
	int m_OrgMinute;
	int m_OrgHour;
	int m_Year;
	int m_Month;
	int m_Day;
	int m_Second;
	int m_Minute;
	int m_Hour;
	int m_FindEdge;
	float m_InPer;
	int m_HighLight;
	double *m_CalPlnFct;
	int *m_PlotOpt;
	DWORD *m_TopView;
	DWORD *m_FrontView;
	DWORD *m_SideView;

	void FindInWord(int x, int y,long* num, double * avgsum);
	int m_LineLen;
	float m_Edge;
	void FindEdge();
	int m_RAE;
	float m_Volume;
	float m_Activity;
	int m_VolumeType;
	int m_ActiveType;
	bool m_GotData;
	void SaveAvgData();
	double *m_PlnAvg;
	double *m_AvgPoint;
	float *m_DisAvgPoint;
	//long *m_DisAvgPtBk;
	void CalSumAvg();
	float *m_fData;
	int *m_nData;
	void GetDateTime(CString str,int*d, int*m,int*y);
	void GetDataFile();
	void SortData(CString str, CString *newstr);
	void SortData(CString str, long *num);
	void GetHeaderFile();
	long m_SinPerBlock;
	double m_EnergyLevel;
	double m_DeadtimeConstant; // scanner dependent  constant
	long m_ZSize;
	long m_YSize;
	long m_XSize;
	int m_dimType;
	//CString m_NumFormat;
	bool m_HRRTFlag;
	CString m_ImageFile;
	CString m_HdrFileName;
	CString m_NrmFileName;
	void InitThread();
	CCorrectRunView* pView;
	unsigned int tid;
	HANDLE hThread;

protected:

private:
	static unsigned int WINAPI BkGndProc(void *p);

// Generated message map functions
protected:
	//{{AFX_MSG(CCorrectRunDoc)
	afx_msg void OnFileOpen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CORRECTRUNDOC_H__8CCCDE6D_7636_11D5_BF5D_00105A12EF2F__INCLUDED_)
