/////////////////////////////////////////////////////////////////////////////
// ProcessingThread
// Thread where are executed reconstruction tasks to keep UI free 
// for user interaction
/*
 *  Modification History:
 *  07-Oct-2008: Added TX TV3DReg to MAP-TR segmentation methods list (Todo: design better interface)
*/
#ifndef ProcessingThread_h
#define ProcessingThread_h

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define WM_START_TASK		(WM_USER+1)
#define WM_START_TASK_OK	(WM_USER+2)
#define WM_START_TASK_FAIL	(WM_USER+3)
#define WM_TASK_DONE	(WM_USER+4)
#define WM_TASK_READY	(WM_USER+5)

#define WM_PROCESSING_THREAD (WM_USER+100)
#define WM_THREAD_QUIT (WM_USER+101)

typedef enum {Seg_None, Seg_MAP_TR, TX_TV3DReg} TX_Seg_Method;
class ProcessingTask
{
public:
	int job_ID;
	int task_ID;
	unsigned tx_Seg;
	enum {Waiting, Submitted, Active, Done} status;
	CString cmd_line;
	CString name;
	CString output_fname;
	CString qc_fname;
	int SetName(const char *output_fname);
};

typedef enum {TASK_BL_HISTOGRAM, TASK_TX_HISTOGRAM, TASK_EM_HISTOGRAM, TASK_TX_PROCESS, TASK_TX_TV3D, TASK_AT_PROCESS, TASK_SCAT_PROCESS,
 TASK_DELAYED_SMOOTHING, TASK_RECON, TASK_COPY_RECON_FILE, TASK_HOSP_RECON, TASK_QUANTIFY_IMAGE, TASK_CLEAR_TMP_FILES} TaskType;


class CProcessingThread : public CWinThread
{
	DECLARE_DYNCREATE(CProcessingThread)
protected:
	CProcessingThread();           // protected constructor used by dynamic creation
	virtual BOOL InitInstance( );
	enum {ready=0, busy=1} status;

// Attributes
public:
	CWnd* pReconGUI;
	void ClearTmpFiles(const CString& file_list);
// Implementation
protected:
	virtual ~CProcessingThread();
	//{{AFX_MSG(CReconGUIDlg)
	afx_msg void OnStartTask(UINT, LONG);
	afx_msg void OnTaskReady(UINT, LONG);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
