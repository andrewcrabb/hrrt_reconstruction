// ReconGUIDlg.h : header file
//
/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				Integrate e-7tools, Cluster Reconstruction, Processing Thread and Queue
 *              to allow multiple jobs submission.
 */
#include "afxwin.h"
#if !defined(AFX_RECONGUIDLG_H__C7D1CBD2_8BD6_4C8A_984C_B54AE590648E__INCLUDED_)
#define AFX_RECONGUIDLG_H__C7D1CBD2_8BD6_4C8A_984C_B54AE590648E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <string>
#include <map>
//#include "../ReconQue/_ReconQue_i.c"
//#include "../ReconQue/_ReconQue.h"
#include "ProcessingThread.h"
/////////////////////////////////////////////////////////////////////////////
// CReconGUIDlg dialog
class CHeader;
class ProcessingTask;

#define MAX_NUM_TASKS 32
#define MAX_NUM_FRAMES 128

class CReconGUIDlg : public CDialog
{
// Construction
public:

//	IRGQue* m_pQue;
//	IErrorInfo* pErr;

	float m_calFactor[2]; // span 3 and span 9 calibration factors
	float m_DeltaXY;
	float m_DeltaZ;
	float m_MulFactor;
	float m_BranchFactor;
	std::string m_UnitType;
	int m_StartPlane;
	int m_EndPlane;
	int m_Weight_method; // OSEM3D weighting method (0=UW, 1=AW, 2=ANW, 3=OP)
	int m_Iteration;
	int m_SubSets;
	BOOL m_PSF;
	BOOL m_TX_TV3DReg;
	unsigned m_TX_Seg_Method;
;
	void GetEmFilename(int num, CString *name);
	void Get_MultiFrame_Files();
	float	m_Bone_Seg;
	float	m_Water_Seg;
	float	m_Noise_Seg;
	long CheckInputFiles();
	long CheckReconFiles();
	int Build_Recon_Command(int frame); // Build the recon command for the specified frame
	int m_Tx_Segment;

	/*
	 *	e7_tools and MAP-TR integration variables
	 */
	int Build_e7_atten_Command();
	int Build_e7_scatter_Command(int frame);

	std::map<CString, CString> map_Priors; // Map-TR prior parameter list 
	int InitPriors(const CString &spec); // initiatlize map_Priors
										 // with specification from configration file
	int get_num_priors();  //  returns  map_Priors list size
	float ss_limits[2];    // scatter scaling limits
	CString m_MAP_prior;   //  Current MAP-TR Prior parameter
	int get_prior(int index, 
		CString &name, CString &value); // get the specified parameter name and value from m_MAP_prior list
	int set_prior(int index, CString &value);		 // Replace with the specified parameter with the new value
	float m_MAP_smoothing; // User specified MAP-TR Beta smoothing
	BOOL m_MU_scale;	   // Flag to turn off the automatic scaling with water peak
	float m_MU_peak;	   // User specified mu value for the peak
	// If m_MU_scale is off, e7_atten will be called with the arguments --aus --asv m_Mu_peak
	// see e7_atten documentation for more details.

	/*
	 *	Cluster integration variable(s)
	 */
	// directory specified in the configuration, where the cluster reconstruction
	// job files are created. The directory can be changed with the UI
	CString submit_recon_dir;
	// Method to copy files to the cluster file server disk if necessary
	// The file server disk is derived from submit_recon_dir
	int copy_recon_file(CString &cluster_dir, CString &src_file, CString &target_file, FILE *batch_fp,
		int overwrite_flag=0);

	/*
	 *	Histogramming integration variable(s)
	 */
	// Root directory specified in the configuration, where histogrammed sinogram are created.
	// The sinogram files are created in the directory Root_Dir\PatientName, which is created
	// already if not existing
	CString sinogram_data_dir;
	// Utility method to update the UI item visibility 
	void HideAndShow(int dlg_item1, int dlg_item2, int revert_flag=0);

	int Histogram(CString &file, TaskType task_type); // Histogram a listmode file
	unsigned Histogram(); // Histogram listmode files if any
	// Method to show histograming message to warn the user that he should not start acquiring
	// if the listmode file is on the ACS
	void ShowHistogramming(ProcessingTask& task, int done_flag=0); 

	// Make output sinogram filename from input listmode filename
	// sinogram location is specified in configuration file
	int make_sino_filename(CString &lm_fname, CString &sino_fname);

	/*
	 *	Scatter and TX process QC
	 * Always create QC files in postscript format using gnuplot freeware
	 */
	
	CString m_gnuplot_Cmd; // gnuplot exe name specified in the configuration file
	CString m_QC_Path; // QC filename

	/*
	 *	Info, Warning and Error message logging
	 */
	FILE *log_fp;
	void log_message(const char *msg, int type=0);

	/*
	 *	Frame durations used for Random smoothing using windelays
	 */
	int   frame_duration[MAX_NUM_FRAMES]; // frame duration for each frame
	int num_frames(CHeader &hdr,
		int *frame_duration, int max_frames); // extract frame durations from Header

	/*
	 * Internal Queue Management
	 * The Queue contains a list of jobs. 
	 * Each job has one or many tasks. Each task is identified by a job and a task number.
	 */
	CButton	m_RemoveAll;  // Remove all Jobs, except the active one
	CButton	m_RemoveJob;  // Remove all tasks of the selected job except the active one if any. 
	CButton m_TopJob;	  // Move the all tasks of selected job to top of the queue
	CListBox	m_ListTask;
	void RemoveJob(int job); // Remove specified job_ID
	/*
	 *	TEMP files management
	 */
	CString tmp_files;
	CButton	m_KeepTempCheck;  // Option to automatically delete or keep temp files:
							  // Attenuation, scatter, job_files,

	long CheckScattFiles();
	long CheckAtFiles();
	long CheckTxFiles();
	long ValidateFile(CString Filename, int type, int span=-1); 
					// type 0=TX_SINO or BL_SINO,1=EM_SINO, 2=ATTEN or NORM or SCATTER, 4=MU_MAP
	void SaveIniFile();
	int SortData(CString &str, CString &newstr);
	void GetIniFile();
	int Build_Scatter_Command();
	char m_AppPath[1024];
	int m_Tx_Rebin;
	short m_Mu_Zoom[2];
	float m_Tx_Scatter_Factors[2];
  unsigned m_start_countrate;
	int m_Recon_Rebin;
	int Build_At_Command();
	int Build_Tx_Command();
	int m_Ary_inc;
	int m_Ary_Span[2];
	int m_Ary_Ring[2];
	int m_Tx_Method;
	int m_Blank_Count;
	int m_Tx_Count;
	int m_Sc_Debug;
	CString m_Sc_Debug_Path;
	UINT m_LLD;
	UINT m_ULD;
	CString m_HOSP_Cmd;
	CString m_Tx_File[5];
	CString m_Blank_File[5];
	CString m_Mu_Map_File;
	CString m_Em_File;
	CString m_Norm_9_File;
	CString m_Norm_3_File;
	CString m_3D_Atten_File;
	CString m_2D_Atten_File;
	CString m_Scatter_File;
	CString m_Atten_Correction;
	CString m_Image;
	CString m_Quan_Header;
	CString m_Tx_Command;
	CString m_At_Command;
	CString m_Sc_Command;
	CString m_Rc_Command;
	CString m_multi_emiss;
	CString m_dyn_File;
	int		m_num_of_emiss;
	bool	m_multi_frames;
	int m_Type;
	bool m_Tx_OK;
	bool m_Aten_OK;
	bool m_Scatter_OK;
	bool m_Recon_OK;
	enum {KOLN_SPECIAL=0, LSO_LYSO=1} head_types;
	CReconGUIDlg(CWnd* pParent = NULL);	// standard constructor
	BOOL RunAndForgetProcess(CString sCmdLine, LPTSTR sRunningDir, int *nRetValue);
	BOOL RunProcessAndWait(CString sCmdLine, LPTSTR sRunningDir, int *nRetValue);
// Dialog Data
	//{{AFX_DATA(CReconGUIDlg)
	enum { IDD = IDD_RECONGUI_DIALOG };
	CButton	m_Auto_Assign;
	CSpinButtonCtrl	m_Spn_Rng_Opt;
  CEdit  m_EM_HIST_Start;
	UINT	m_Span;
	UINT	m_Ring_Diff;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReconGUIDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	CBrush m_ClrBrush[4];
	static const COLORREF crColors[4];
	
	// Generated message map functions
	//{{AFX_MSG(CReconGUIDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	/*
	 *	Processing thread messages processing
	 */
	afx_msg LRESULT OnProcessingMessage(WPARAM msg, LPARAM param);
	
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButton2();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDestroy();
	afx_msg void OnButton3();
	afx_msg void OnChangeEdit5();
	afx_msg void OnButton4();
	afx_msg void OnButton8();
	afx_msg void OnButton5();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButton9();
	afx_msg void OnButton15();
	afx_msg void OnButton16();
	/*
	 *	Queue Management callbacks
	 */
	afx_msg void OnRemoveJob(); // Remove all tasks of the selected job except the active one if any. 
	afx_msg void OnRemoveAll(); // Remove all Jobs, except the active one
	afx_msg void OnTopJob();    // Move the all tasks of selected job to top of the queue
	afx_msg void OnSelchangeListTask(); // Enable/Disable buttons when selection changes

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:

	void AppendImageHeader(void);
	int QuantifyImage(void); // quantify current image filename
	int QuantifyImage(const char *fname); // quantify specified image filename
	void WriteImageHeader(void);
	int CreateHOSPFile(void);
	CButton m_Run_Tx;
	CButton m_Run_at;
	CButton m_Run_sc;
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedCheck3();
	afx_msg void OnBnClickedCheck4();
	afx_msg void OnBnClickedCheck5();
	CButton m_Run_rc;
	// CheckForWhiteSpace replaces all '.' by '_' except the last one.
	bool CheckForWhiteSpace(CString &filename, int single_dot_flag=0);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton6();
	long m_JobNum;
	long m_JobTime;

	/*
	 *	Processing thread 
	 */
	CWinThread * processing_thread;
	void NewJob(); // initialize a new job
	int SubmitTask(CString &cmd, TaskType type, const char *output_fname);
	// Method called after task completion to check output and do any required post-processing
	void post_process(ProcessingTask &); 
	void RefreshTaskList(); 	// Refresh the queue display

	/* 
	 * Add Quantification info for cluster recon
	 */
	int AddQuantInfo(CString &em_sino, CString& m_Quan_Header);

public:
  afx_msg void OnEnChangeEditEmHistStart();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RECONGUIDLG_H__C7D1CBD2_8BD6_4C8A_984C_B54AE590648E__INCLUDED_)
