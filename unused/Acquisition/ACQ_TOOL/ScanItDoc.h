// ScanItDoc.h : interface of the CScanItDoc class
//
/////////////////////////////////////////////////////////////////////////////
/* HRRT User Community modifications
   Authors: Merence Sibomana
   9-July-2009 : Add milliseconds study time in the header, key "!study time (hh:mm:ss.sss)"
                 Bug fix in CScanItDoc::SendCmd
   25-Sep-2009 : Move setting scan start time after Go signal ACK
                 Add "Dead time constant" to scanner configuration and header files 
*/

#if !defined(AFX_SCANITDOC_H__9C52C516_E38A_4169_AA91_1952D656692C__INCLUDED_)
#define AFX_SCANITDOC_H__9C52C516_E38A_4169_AA91_1952D656692C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <process.h>
#include <atlbase.h>
#include <winsock2.h>


// Listmode and histogram dlls
#include "LmHistBeUtils.h"
#include "LmHistStats.h"
#include "HistogramDLL.h"
#include "LmHistRebinnerInfo.h"
#include "ListmodeDLL.h"
#include "sinoinfo.h"
#include "histogram_st.h"


#include "gantryModelClass.h"	

#define THREADKILL_TO		1000
#define THREADWAIT_TO		30000
#define TICK				500
#define THREADEXIT_SUCCESS	0x1234
#define SERVERPORT			80

class CScanItView;

class CCallB : public CCallback
{
public:
	static enum {Preset, Stop, Abort, Error};
	bool m_done;
	bool m_writeFile;
	bool m_ARS_Err;
	char m_str_Error[256];
	void notify (int error_code, char *extra_info);
	void acqComplete(int enum_flag);
	void acqPostComplete(int enum_flag, int fileWriteSuccess);
	CCallB(){m_writeFile = FALSE;m_done = FALSE;m_ARS_Err = FALSE;};
};



class CScanItDoc : public CDocument
{
protected: // create from serialization only
	CScanItDoc();
	DECLARE_DYNCREATE(CScanItDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScanItDoc)
	public:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	CString m_DynaFrame;
	CString m_FileName;
	int m_Cont_Scan;
	void InitSinoInfo();
	CString m_errfile;
	int m_err_num;
	int SendCmd(char *cmdSnd,char *cmdRcv, int hd, int timeout);
	void ConfigRebinner();
	void ListHeaderFile();
	void GetTruesAvg();
	long m_Total_Trues[20];
	long m_Trues_Move_Avg;
	int m_KCPS;
	long m_Auto_Start;
	void AbortScan();
	void StopScan();
	long m_Mem_Alloc;
	void SetFileName();
	long orgtime;
	void UpDateSaveFileFlag();
	int m_Strength_Type;
	float m_Strength;
	void AppendHeaderFile();
	void SetScanParam();
	bool m_Segment_Zero_Flag;
	CString m_Pt_Lst_Name;
	CString m_Pt_Fst_Name;
	CString m_Pt_DOB;
	CString m_Pt_ID;
	CString m_Case_ID;
	int m_Pt_Sex;
	int m_Pt_Dose;
	unsigned long m_nprojs;
	unsigned long m_nviews;
	unsigned long m_n_em_sinos;
	unsigned long m_n_tx_sinos;
	unsigned long m_nframes;
	int m_Ary_Span[4];
	int m_Ary_Ring[4];
	int m_en_sino[4];
	int m_Ary_Select;
	int InitializeHIST();
	int InitializeLM();
	void CheckError(int err);
	bool m_HISTInit_Flag;
	bool m_LMInit_Flag;
	void SaveConfigFile();
	CString m_CfgFileName;
	bool GetConfigFile();
	int m_Preset_Opt;
	int m_Count_Opt;
	int m_List_64bit;
	int m_Scan_Mode;
	int m_Scan_Type;
	int m_uld;
	int m_lld;
	long m_Duration;
	int m_DurType;
	int m_Span;
	int m_Ring;
	float m_Speed;
	int m_Plane;
	bool m_RunFlag;
  bool m_ShuttingDown;
	void GetData(CString str);
	void Download();
	unsigned int tid;
	WSAEVENT	ShutdownEvent;				
	HANDLE	ThreadLaunchedEvent;
	HANDLE	hThread;
	void Shutdown();
	void InitThread();
	bool m_AllocFlag;
	long m_index;
	long *lData;
	long *lTrues;
	long *lIndex;
	bool m_bDownFlag;
	long m_Prompts;
	long m_Randoms;
	long m_Singles;
	__int64 m_Rnd_Tot;
	__int64 m_Trs_Tot;
	long m_Time;
	int m_head;
	int m_config;
	FILE  *hc_file;
	CString m_ext_fname;
	//long m_Time;
	unsigned long m_preset;
	CString m_FName;
	int m_Dn_Step;
	COleDateTime m_timeStart;
	int m_timeStart_msec;
	COleDateTime m_timeEnd;
	HIST_stats pHISTStats;
	LM_stats pLMStats;
	CCallB pCB;
	CSinoInfo pSinoInfo;
	Rebinner_info m_rebinnerInfo;

	CScanItView* pView;
// gantry model
	CGantryModel *model;   // MAKE SURE TO delete model; by calling the destructer
	
	


	virtual ~CScanItDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
private:
//	static unsigned int WINAPI BkGndProc(void *p);
// Generated message map functions
protected:
	//{{AFX_MSG(CScanItDoc)
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCANITDOC_H__9C52C516_E38A_4169_AA91_1952D656692C__INCLUDED_)
