// ReconGUIDlg.cpp : implementation file
//
/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				Integrate e-7tools, Cluster Reconstruction, Processing Thread and Queue
 *              to allow multiple jobs submission.
 *  12-DEC-2004: Merence Sibomana
 *               Lower Memory threshold (--mem 0.7 option) when computing scatter in span 3
 *  02-FEB-2005: Add support for lmhistogram logging 
 *				 Add calibration factor, calibration units and calibration file to sinogram header.
 *				 Add non-attenuation correction support for cluster reconstruction 
 *
 *  12-APR-2005: Exit if another instance is already running
 *               Bug fix: number of subsets missing in the cluster recon job file
 *
 *  28-JUN-2005: Bug fix: scatter correction was using prompt instead of true sinogram for l64 input
 *               with multi-frame
 *               Always start histogramming tasks first regardless of the job priority.
 *               Bug fix: Listmode were histogrammed with -PR for other methods than OSEM3D when
 *               OPO3D is selected as OSEM3D weighting method.
 *  28-AUG-2007: Allow OPO3D for non-cluster recon
 *  07-JAN-2008: Remove --ucut in e7_atten for creating mumap
 *  22-MAY-2008: Use HRRT_U commands (e7_atten_u, e7_fwd_u, e7_sino_u, hrrt_osem3d)
 *  31-Jul-2008: Always use span9 for scatter (sibomana@gmail.com)
 *				 Add m_Mu_Zoom and m_TX_Scatter_Factors(sibomana@gmail.com)
 *  06-Aug-2008: Use existing *_ra_smo.s if existing or .ch otherwise for reconstruction delayed.
 *               Change default scatter scaling limits from [0.4,1.0] to [0.25,2.0]
 *	07-Aug-2006: Added Build Date Time stamp to Toolbar and about menu
 *  20-Aug-2008: Use "file.i" for image filename instead of "file_" for cluster reconstruction
 *  21-Aug-2008: Always use -notimetag option fot TX histogramming
 *  22-Aug-2008: Use umap input in e7_sino, create it if not existing prior to calling e7_sino
 *  07-Oct-2008: Added TX TV3DReg to MAP-TR segmentation methods list (Todo: design better interface)
 *  13-Nov-2008: Ignore auto-scaling flag when segmentation enabled
 *               ignore transmission scatter correction when using MAP-TR
 *               Read and use LBER from norm header for scatter correction
 *  20-Nov-2008: Create temp TX sinogram header for listmode TX input to get axial velocity
 *  09-Jan-2009: Allow mu_zoom (2,2) and implement EM histogram start countrate
 *  02-Dec-2009: Remove EM histogram start countrate from recon.ini
*/

#include "stdafx.h"
#include "ReconGUI.h"
#include "ReconGUIDlg.h"
#include "txproc.h"
#include "Atten_Process.h"
#include "Scatter_Config.h"
#include "SSRDIFTDlg.h"
#include "header.h"
#include "ProcessingThread.h"
#include <process.h>

#include <winbase.h>
#include <math.h>
#include <io.h>
#include <direct.h>
#include <string>
#include <map>
#include <vector>
#include "reconguidlg.h"
#include "afxwin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define unlink _unlink
#define strnicmp _strnicmp
#define stricmp _stricmp
#define mkdir _mkdir


#define HISTOGRAM_BL 0x1
#define HISTOGRAM_TX 0x2
#define HISTOGRAM_EM 0x4

#define LOGFILEDIR "LOGFILEDIR"
#define LOGFILECOUNT 3

static float eps = 1.0e-6f;

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
//	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// 	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
//	CEdit m_DateTimeLbl;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}
/*
BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString str;
	str = "ReconGUI_u 1.1, Build: "+(CString) __DATE__+", "+(CString) __TIME__;

	m_DateTimeLbl.SetWindowTextA((LPCTSTR)str);
	return TRUE;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EDIT1, m_DateTimeLbl);
}
*/
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

inline 	void AddTmpFile(CString &list, const char *filename)
{
	if (list.GetLength()>0) list += " ";
	list += filename;
}


// Current time formating
inline void format_NN(int n, char *out)
{ 
  if (n < 10) sprintf(out,"0%d", n);
  else sprintf(out,"%d", n);
}

static const char *my_current_time()
{
	static char time_str[20];
	time_t now;
	time(&now);
	struct tm *ptm = localtime(&now);
	format_NN((1900+ptm->tm_year)%100, time_str);
	format_NN(ptm->tm_mon+1, time_str+2);
	format_NN(ptm->tm_mday, time_str+4);
	format_NN(ptm->tm_hour, time_str+6);
	format_NN(ptm->tm_min, time_str+8);
	format_NN(ptm->tm_sec, time_str+10);
	return time_str;
}

typedef std::map<int, ProcessingTask*> TaskQueue;
typedef std::map<int, ProcessingTask*>::iterator TaskQueueIterator;
typedef std::map<CString, CString>::iterator MAP_TR_PriorMapIterator;
static TaskQueue queue;

static	const char *global_log_dir = NULL;
static char histogram_log_file[FILENAME_MAX];

static void get_directory(const char* in_file, CString &out_dir)
{
	char drive[_MAX_DRIVE];
	char in_dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath((const char*)in_file, drive, in_dir, fname, ext);
	int len = strlen(in_dir);
	if (len>0) in_dir[len-1] = '\0'; // remove trailing separator
	out_dir.Format("%s%s",drive,in_dir);
}

/*
 *	int CReconGUIDlg::InitPriors(const CString &spec)
 *	initiatlize map_Priors with specification from configration file
 */
int CReconGUIDlg::InitPriors(const CString &spec)
{
	int count=0;
	char *buf = new char[spec.GetLength()+1];
	strcpy(buf, (const char*)spec);
	char *pos=buf;
	while (pos != NULL)
	{
		char *name = pos;
		char *end = strchr(pos,';');
		if (end!=NULL) 
		{
			*end = '\0';
			pos = end+1;
		}
		else pos = NULL;
		char *value = strchr(name,'=');
		if (value!=NULL)
		{
			*value++ = '\0';
			map_Priors.insert(std::make_pair(name,value));
			count++;
		}
	}
	delete[] buf;
	return count;
}

/////////////////////////////////////////////////////////////////////////////
// CReconGUIDlg dialog

CReconGUIDlg::CReconGUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReconGUIDlg::IDD, pParent)
	, m_JobNum(0)
	, m_JobTime(0)
{
	//{{AFX_DATA_INIT(CReconGUIDlg)
	m_Span = 0;
	m_Ring_Diff = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	char old_log_file[FILENAME_MAX], log_file[FILENAME_MAX], base[FILENAME_MAX];
	int count = 0;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//
	// Default priors
	m_MAP_prior = "Human head=4,0.000,0.005,0.030,10.,0.096,0.005,0.110,10.,0.025,0.05,0.1;Water phantom=2,0.000,0.005,0.096,0.050,0.050";
	submit_recon_dir = _T("");
	m_gnuplot_Cmd = "pgnuplot";
	m_MAP_smoothing = 30.0f;

	global_log_dir = getenv(LOGFILEDIR);
	// Rename old log file and create new one
	if (global_log_dir != NULL)
	{
		sprintf(base,"%s\\recon_gui", global_log_dir);
	}
	else strcpy(base,"recon_gui");
	sprintf(log_file,"%s.log", base);
	if (_access(log_file,0) == 0)
	{	// recon_gui.log exists
		// try to rename it to check if it is locked by another process
		// no easier method found (write access is granted even if
		// the file is open by another instance)
		sprintf(old_log_file,"%s_0.log", base);
		if (_access(old_log_file,0)) unlink(old_log_file); //make sure the file doesn't exist 
		if (rename(log_file,old_log_file) != 0)
		{ // file locked by another process : exit
			MessageBox("An other instance is already running","ReconGUI startup",MB_ICONSTOP);
			exit(1);
		}
	}
	sprintf(old_log_file,"%s_%d.log", base,LOGFILECOUNT);
	if (_access(old_log_file,0) != -1) _unlink(old_log_file);
	for (count=LOGFILECOUNT-1; count>0; count--)
	{
		sprintf(log_file,"%s_%d.log", base, count+1);
		sprintf(old_log_file,"%s_%d.log", base, count);
		if (_access(old_log_file,0) != -1) rename(old_log_file, log_file);
	}
	sprintf(log_file,"%s_1.log",base );
	sprintf(old_log_file,"%s_0.log", base);
	if (_access(old_log_file,0) == 0) rename(old_log_file, log_file);
	sprintf(log_file, "%s.log", base);
	log_fp = fopen(log_file,"wt");

	// Rename previous histogram log files
	if (global_log_dir != NULL)
	{
		sprintf(base,"%s\\lmhistogram", global_log_dir);
	}
	else strcpy(base,"lmhistogram");
	sprintf(old_log_file,"%s_%d.log", base,LOGFILECOUNT);
	if (_access(old_log_file,0) != -1) _unlink(old_log_file);
	for (count=LOGFILECOUNT-1; count>0; count--)
	{
		sprintf(log_file,"%s_%d.log", base, count+1);
		sprintf(old_log_file,"%s_%d.log", base, count);
		if (_access(old_log_file,0) != -1) rename(old_log_file, log_file);
	}
	sprintf(log_file,"%s_1.log",base );
	sprintf(old_log_file,"%s.log", base);
	if (_access(old_log_file,0) != -1) rename(old_log_file, log_file);
	sprintf(histogram_log_file, "%s.log", base);

	// Create histogram_log_file
	FILE *hist_log_fp = fopen(histogram_log_file, "wt");
	if (hist_log_fp != NULL) fclose(hist_log_fp);
	
	processing_thread = NULL;
	ss_limits[0] = 0.25f; 
	ss_limits[1] = 2.0f;
	m_MU_scale = TRUE;
	m_MU_peak = 0.096f;
	m_calFactor[0] = m_calFactor[1] = 1.0f;
	head_types = LSO_LYSO;
	m_MulFactor = 1.0f;
}

void CReconGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReconGUIDlg)
	DDX_Control(pDX, IDC_KEEP_TMP_FILES, m_KeepTempCheck);
	DDX_Control(pDX, IDC_REMOVE_ALL, m_RemoveAll);
	DDX_Control(pDX, IDC_REMOVE_JOB, m_RemoveJob);
	DDX_Control(pDX, IDC_TOP_JOB, m_TopJob);
	DDX_Control(pDX, IDC_LIST_TASK, m_ListTask);
	DDX_Control(pDX, IDC_CHECK1, m_Auto_Assign);
	DDX_Control(pDX, IDC_SPIN1, m_Spn_Rng_Opt);
	DDX_Control(pDX, IDC_EDIT_EM_HIST_START, m_EM_HIST_Start);
	DDX_Text(pDX, IDC_EDIT1, m_Span);
	DDX_Text(pDX, IDC_EDIT2, m_Ring_Diff);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHECK2, m_Run_Tx);
	DDX_Control(pDX, IDC_CHECK3, m_Run_at);
	DDX_Control(pDX, IDC_CHECK4, m_Run_sc);
	DDX_Control(pDX, IDC_CHECK5, m_Run_rc);
	DDX_Text(pDX, IDC_EDIT6, m_JobNum);
	DDX_Text(pDX, IDC_EDIT15, m_JobTime);
}

BEGIN_MESSAGE_MAP(CReconGUIDlg, CDialog)
	//{{AFX_MSG_MAP(CReconGUIDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_EN_CHANGE(IDC_EDIT5, OnChangeEdit5)
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_CLICKED(IDC_BUTTON8, OnButton8)
	ON_BN_CLICKED(IDC_BUTTON5, OnButton5)
	ON_BN_CLICKED(IDC_BUTTON9, OnButton9)
	ON_BN_CLICKED(IDC_BUTTON15, OnButton15)
	ON_BN_CLICKED(IDC_BUTTON16, OnButton16)
	ON_BN_CLICKED(IDC_REMOVE_JOB, OnRemoveJob)
	ON_BN_CLICKED(IDC_REMOVE_ALL, OnRemoveAll)
	ON_LBN_SELCHANGE(IDC_LIST_TASK, OnSelchangeListTask)
	ON_BN_CLICKED(IDC_TOP_JOB, OnTopJob)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CHECK2, OnBnClickedCheck2)
	ON_BN_CLICKED(IDC_CHECK3, OnBnClickedCheck3)
	ON_BN_CLICKED(IDC_CHECK4, OnBnClickedCheck4)
	ON_BN_CLICKED(IDC_CHECK5, OnBnClickedCheck5)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON6, OnBnClickedButton6)
	ON_MESSAGE(WM_PROCESSING_THREAD, OnProcessingMessage)
    ON_EN_CHANGE(IDC_EDIT_EM_HIST_START, OnEnChangeEditEmHistStart)
END_MESSAGE_MAP()

// Colors schemes : Red, Green, Gray, Orange
const COLORREF CReconGUIDlg::crColors[4]={RGB(255,0,0),RGB(0,255,0),RGB(200,200,200), RGB(255,133,0)};

/////////////////////////////////////////////////////////////////////////////
// CReconGUIDlg message handlers

BOOL CReconGUIDlg::OnInitDialog()
{
  CString str;

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	GetCurrentDirectory(1024,m_AppPath);
	// TODO: Add extra initialization here
	m_ClrBrush[0].CreateSolidBrush(crColors[0]);
	m_ClrBrush[1].CreateSolidBrush(crColors[1]);
	m_ClrBrush[2].CreateSolidBrush(crColors[2]);
	m_ClrBrush[3].CreateSolidBrush(crColors[3]);

	m_Tx_OK = false;
	m_Aten_OK = false;
	m_Scatter_OK = false;
	m_Recon_OK = false;

	// Default values	
	m_LLD = 400;
	m_ULD = 650;
	m_Tx_Method = 0;
	m_Tx_Rebin = 1;
	m_Mu_Zoom[0] = 2;
	m_Mu_Zoom[1] = 3;
  m_start_countrate=0;
	m_Tx_Scatter_Factors[0]=0.0f;
	m_Tx_Scatter_Factors[1]=1.0f;
	m_Recon_Rebin = 0;

	// Set Span and RD values
	m_Ary_inc = 0;
	m_Ary_Span[0] = 3;
	m_Ary_Span[1] = 9;
	m_Ary_Ring[0] = 67;
	m_Ary_Ring[1] = 67;
	m_Blank_Count = 0;
	m_Tx_Count = 0;
	m_Tx_Segment = 0;
	m_Spn_Rng_Opt.SetRange(0,1);
	m_Spn_Rng_Opt.SetBuddy ((CWnd *)GetDlgItem(IDC_EDIT5));		
	m_Sc_Debug = 1;
	m_Sc_Debug_Path = "";
	m_Type = 0;
	m_Bone_Seg = 0.12f;
	m_Water_Seg = 0.065f;
	m_Noise_Seg = 0.0085f;
	m_Iteration = 2;
	m_SubSets = 16;
	m_Weight_method = 2; //ANWO3D
	m_StartPlane = 10;
	m_EndPlane = 200;
	m_Run_Tx.SetCheck(1);
	m_Run_at.SetCheck(1);
	m_Run_sc.SetCheck(1);
	m_Run_rc.SetCheck(1);
	m_BranchFactor=1.f;
	m_PSF = m_TX_TV3DReg = FALSE;
	// Get the ini file
	GetIniFile();
	
	m_Spn_Rng_Opt.SetPos(m_Ary_inc);
	
	m_multi_frames = false;
	//m_multi_emiss = NULL;
	// Enable or diable the atten and Tx if by pass option is selected

	OnBnClickedCheck2();
	OnBnClickedCheck3();
	OnBnClickedCheck4();
	OnBnClickedCheck5();

	m_RemoveJob.EnableWindow(FALSE);
	m_TopJob.EnableWindow(FALSE);
	m_RemoveAll.EnableWindow(FALSE);

/*
	HRESULT hr = ::CoCreateInstance (CLSID_CRGQue, NULL,
		CLSCTX_SERVER, IID_IRGQue, (void**) &m_pQue);
	if (FAILED (hr)) 
	{
		CWnd *pwnd;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON1);
		pwnd->EnableWindow(FALSE);
		//MessageBox ("CoCreateInstance failed", "Error", MB_ICONSTOP | MB_OK);
		m_pQue = NULL;
	}
*/
/*
	COSERVERINFO si;
	COAUTHINFO ai;
	COAUTHIDENTITY aid;
	//TCHAR uid[1024],psw[1024], dmn[1024];
	BSTR uid, psw, dmn;

	ZeroMemory(&si,sizeof(si));
	ZeroMemory(&ai,sizeof(ai));
	ZeroMemory(&aid,sizeof(aid));

	ai.dwAuthnSvc = RPC_C_AUTHN_WINNT;
	ai.dwAuthzSvc = RPC_C_AUTHZ_NONE;
	ai.pwszServerPrincName = NULL;
	ai.dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
	ai.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
	ai.dwCapabilities = EOAC_NONE;
	ai.pAuthIdentityData = &aid;
	
	//wsprintf(uid,"ReconQue");
	//wsprintf(psw,"");
	//wsprintf(dmn,"");
	uid = A2BSTR("ReconQue");
	psw = A2BSTR("hrrt");
	dmn = A2BSTR("");

	aid.User = const_cast<USHORT*>((LPCWSTR)uid);
	aid.UserLength = 8;
	aid.Password = const_cast<USHORT*>((LPCWSTR)psw);
	aid.PasswordLength = 4;
	aid.Domain = const_cast<USHORT*>((LPCWSTR)dmn);
	aid.DomainLength = 0;
	aid.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
	
	si.pAuthInfo = &ai;
	si.pwszName=OLESTR("hrrt-rec-pc");

	MULTI_QI mqi[1];
	IID iid = IID_IRGQue;
	mqi[0].pIID=&iid;
	mqi[0].pItf=NULL;
	mqi[0].hr=0;

	HRESULT hr = ::CoCreateInstanceEx (CLSID_CRGQue, NULL,
		CLSCTX_REMOTE_SERVER, &si,1, mqi);

	if (SUCCEEDED (hr)) 
	{
		m_pQue = (IRGQue *) mqi[0].pItf;

	}
	else
	{
		CWnd *pwnd;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON1);
		pwnd->EnableWindow(FALSE);
		//MessageBox ("CoCreateInstance failed", "Error", MB_ICONSTOP | MB_OK);
		m_pQue = NULL;
	}
*/

  str.Format("%d", m_start_countrate);
  m_EM_HIST_Start.SetWindowText(str);

	str = "ReconGUI_u 1.1,  Build: "+(CString) __DATE__+", "+(CString) __TIME__;

	SetWindowTextA((LPCTSTR)str);

	Invalidate(TRUE);
	processing_thread = (CWinThread*)AfxBeginThread(RUNTIME_CLASS(CProcessingThread));
	((CProcessingThread*)processing_thread)->pReconGUI = this;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CReconGUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CReconGUIDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CReconGUIDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
/*
void CReconGUIDlg::OnButton1() 
{


	CString sCmdLine = "c:\\cdata.exe -H f:\\water\\homphantom_christmas01_EM_68h_ifill-hosp_scorr.i.hdr -I f:\\water\\homphantom_christmas01_EM_68h_ifill-hosp_scorr.i -N f:\\water\\EffFactor_vc.hdr";
	CString sRunningDir = "C:\\";
	//int nRetValue;

	//RunAndForgetProcess(sCmdLine,sRunningDir, &nRetValue);
	//RunProcessAndWait(sCmdLine,sRunningDir, &nRetValue);
	//int a = 0;
	LPCTSTR lpApplicationName = "C:\\CData.exe";
	LPTSTR lpCommandLine = "-H header.hdr";
//	UINT a=  WinExec("C:\\CData.exe",1);
	LPSTARTUPINFO lpStartupInfo;  // pointer to STARTUPINFO
	LPPROCESS_INFORMATION lpProcessInformation;  // pointer to PROCESS_INFORMATION
	
	lpStartupInfo->cbReserved2 = 0;
	lpStartupInfo->lpReserved  = NULL;
	lpStartupInfo->lpReserved2  = NULL;
	lpStartupInfo->lpTitle = "Recon CData";
	lpStartupInfo->lpDesktop = NULL;


	BOOL a = CreateProcess(
	  lpApplicationName, lpCommandLine,NULL, NULL, FALSE, CREATE_NEW_CONSOLE| NORMAL_PRIORITY_CLASS, NULL, NULL,
	  lpStartupInfo, lpProcessInformation);
	
}
*/
BOOL CReconGUIDlg::RunAndForgetProcess(CString sCmdLine, LPTSTR sTitle, int *nRetValue)
{
	// int nRetWait;
	int nError=0;
	STARTUPINFO stInfo;
	PROCESS_INFORMATION prInfo;
	BOOL bResult;
	LPVOID lpMsgBuf;
	ZeroMemory( &stInfo, sizeof(stInfo) );
	stInfo.cb = sizeof(stInfo);
	stInfo.dwFlags=STARTF_USESHOWWINDOW;
	stInfo.wShowWindow=SW_SHOWDEFAULT;//SW_MINIMIZE;
	stInfo.lpTitle = sTitle;
	log_message(sCmdLine);
	//try
	//{
		bResult = CreateProcess( NULL, 
							 (LPSTR)(LPCSTR)sCmdLine, 
							 NULL, 
							 NULL, 
							 TRUE,
							 CREATE_NEW_CONSOLE 
							 | NORMAL_PRIORITY_CLASS ,
							 NULL,
							 (LPCSTR)NULL ,
							 &stInfo, 
							 &prInfo);
		if (!bResult)
		*nRetValue = nError = GetLastError();
	//}
	//catch (CException r)
	//{
		// Get the error if there is one
		if (!(nError == 0 || nError == 126 || nError == 2 || nError == 127))
		{
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
			);
			// Process any inserts in lpMsgBuf.
			// ...
			// Display the string.
			MessageBox(  (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
		}
		else
			*nRetValue  = nError = 0;
	//}
	// Close the Handle
	CloseHandle(prInfo.hThread); 

	// We are all done
	CloseHandle(prInfo.hProcess);
	if (nError == 126) *nRetValue  = nError = 0;
	//if (!bResult) return FALSE;
	return TRUE;	
}


BOOL CReconGUIDlg::RunProcessAndWait(CString sCmdLine, LPTSTR sTitle, int *nRetValue)
{
	int nRetWait;
	int nError;
	CString msg;

	// That means wait 300 s before returning an error
	//DWORD dwTimeout = 1000 *300; 
					
					
	STARTUPINFO stInfo;
	PROCESS_INFORMATION prInfo;
	BOOL bResult;
	LPVOID lpMsgBuf;
	ZeroMemory( &stInfo, sizeof(stInfo) );
	stInfo.cb = sizeof(stInfo);
	stInfo.dwFlags=STARTF_USESHOWWINDOW;
	stInfo.wShowWindow=SW_SHOWDEFAULT;//SW_MINIMIZE;
	stInfo.lpTitle = sTitle;

	log_message(sCmdLine);
	bResult = CreateProcess(NULL, 
						 (LPSTR)(LPCSTR)sCmdLine, 
						 NULL, 
						 NULL, 
						 TRUE,
						 CREATE_NEW_CONSOLE 
						 | NORMAL_PRIORITY_CLASS,
						 NULL,
						 (LPCSTR)NULL,
						 &stInfo, 
						 &prInfo);
	*nRetValue = nError = GetLastError();
	if (!bResult) 
	{
		msg.Format("CreateProcess(%s) failed", (const char*)sCmdLine);
		log_message(msg, 2);
		return FALSE;
	}
	//nRetWait =  WaitForSingleObject(prInfo.hProcess,dwTimeout);
	nRetWait =  WaitForSingleObject(prInfo.hProcess,INFINITE);
	// Get the error if there is one
	if (!(nError == 0 || nError == 126 || nError == 2 || nError == 127))
	{
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox(  (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
	}
	else
		*nRetValue = nError = 0;

	// Close the handles
	CloseHandle(prInfo.hThread); 
	CloseHandle(prInfo.hProcess); 
	
	if (nError == 126) *nRetValue  = nError = 0;
	
	if (nRetWait == WAIT_TIMEOUT) return FALSE;
	return TRUE;
}

void CReconGUIDlg::OnButton2() 
{
	CTxproc pDlg;
	pDlg.DoModal();
}

HBRUSH CReconGUIDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{

	HBRUSH hbr;
	hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	int tx_hist=0, em_hist=0;
	for (TaskQueueIterator cur = queue.begin(); cur != queue.end(); cur++)
	{
		if (cur->second->task_ID == TASK_TX_HISTOGRAM && 
			cur->second->status==ProcessingTask::Waiting) tx_hist = 1;
		else if (cur->second->task_ID == TASK_EM_HISTOGRAM &&
			cur->second->status==ProcessingTask::Waiting) em_hist = 1;
	}

	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_EDIT_TX:
		if (m_Tx_OK)
		{
			if(m_Run_Tx.GetCheck())
			{
				if (tx_hist || m_Blank_File[0].Find(".l32")>0 || m_Blank_File[0].Find(".l64")>0 ||
					m_Tx_File[0].Find(".l32")>0 || m_Tx_File[0].Find(".l64")>0 )
				{ // Orange Color when histogramming needed
					pDC->SetBkColor(crColors[3]);
					hbr = (HBRUSH) m_ClrBrush[3];
				} 
				else 
				{
					pDC->SetBkColor(crColors[1]);
					hbr = (HBRUSH) m_ClrBrush[1];
				}
			}
			else
			{
				pDC->SetBkColor(crColors[2]);
				hbr = (HBRUSH) m_ClrBrush[2];
			}
		}
		else
		{
			pDC->SetBkColor(crColors[0]);
			hbr = (HBRUSH) m_ClrBrush[0];
		}
		break;
	case IDC_EDIT_AT:
		if (m_Aten_OK)
		{
			if(m_Run_at.GetCheck() == 0)
			{	pDC->SetBkColor(crColors[2]);
				hbr = (HBRUSH) m_ClrBrush[2];
			}
			else
			{	pDC->SetBkColor(crColors[1]);
				hbr = (HBRUSH) m_ClrBrush[1];
			}
		}
		else
		{
			pDC->SetBkColor(crColors[0]);
			hbr = (HBRUSH) m_ClrBrush[0];
		}
		break;
	case IDC_EDIT_SC:
		if (m_Scatter_OK)
		{
			if(m_Run_sc.GetCheck())
			{
				// Orange Color when histogramming needed
				if (em_hist || m_Em_File.Find(".l32")>0 || m_Em_File.Find(".l64")>0)
				{
					pDC->SetBkColor(crColors[3]);
					hbr = (HBRUSH) m_ClrBrush[3];
				}
				else
				{
					pDC->SetBkColor(crColors[1]);
					hbr = (HBRUSH) m_ClrBrush[1];
				}
			}
			else
			{
				pDC->SetBkColor(crColors[2]);
				hbr = (HBRUSH) m_ClrBrush[2];
			}
		}
		else
		{
			pDC->SetBkColor(crColors[0]);
			hbr = (HBRUSH) m_ClrBrush[0];
		}
		break;
	case IDC_EDIT_RECON:
		if (m_Recon_OK)
		{
			if(m_Run_rc.GetCheck() == 0)
			{
				// Orange Color when histogramming needed
				if (em_hist || m_Em_File.Find(".l32")>0 || m_Em_File.Find(".l64")>0)
				{
					pDC->SetBkColor(crColors[3]);
					hbr = (HBRUSH) m_ClrBrush[3];
				}
				else 
				{
					pDC->SetBkColor(crColors[2]);
					hbr = (HBRUSH) m_ClrBrush[2];
				}
			}
			else
			{
				pDC->SetBkColor(crColors[1]);
				hbr = (HBRUSH) m_ClrBrush[1];
			}
		}
		else
		{
			pDC->SetBkColor(crColors[0]);
			hbr = (HBRUSH) m_ClrBrush[0];
		}
		break;

	default:
		hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
		break;
	}
	
	return hbr;
}

void CReconGUIDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
//	if (m_pQue != NULL)
//		CoUninitialize();

	DeleteObject( m_ClrBrush[0]);	
	DeleteObject( m_ClrBrush[1]);	
}

void CReconGUIDlg::OnButton3() 
{
	int nRetValue;
	long valErr;
	NewJob();
	unsigned hist_status = Histogram();
	nRetValue = Build_Tx_Command();
	if(m_Tx_OK && nRetValue ==0)
	{
		valErr = CheckTxFiles();
		if (valErr == 0)
		{	// Run Transmission Process
//			RunAndForgetProcess(m_Tx_Command,"TX Process", &nRetValue);
			SubmitTask(m_Tx_Command,TASK_TX_PROCESS, m_Mu_Map_File);
			//RunProcessAndWait(m_Tx_Command,"TX Process", &nRetValue);
			//MessageBox(m_Tx_Command,"This is the command to execute Transmission Process",MB_OK);
			if (nRetValue != 0)
				// Error Message Only
				MessageBox("A Problem Encountered with Transmission Process.  Aborting Process","Transmission Process Run Error",MB_ICONSTOP);
		}
	}
		
	else
		MessageBox("Transmission Process is not configured Properly.","Transmission Process Configuration Error",MB_ICONSTOP);
}

void CReconGUIDlg::OnChangeEdit5() 
{
	m_Ary_inc = m_Spn_Rng_Opt.GetPos();
	m_Span = m_Ary_Span[m_Ary_inc];
	m_Ring_Diff = m_Ary_Ring[m_Ary_inc];
	UpdateData(FALSE);	
}

/*
 *	void CReconGUIDlg::log_message
 *  Log information (type=0), Warning(type=1) or Error (type=2) to the current log file.
 *  Call fflush to force the new message to be written so the user can see it.
 */
void CReconGUIDlg::log_message(const char *msg, int type)
{
	if (log_fp == NULL) return;
	switch(type)
	{
	case 0:
		fprintf(log_fp,"%s\tInfo   \t%s\n", my_current_time(), msg);
		break;
	case 1:
		fprintf(log_fp,"%s\tWarning\t%s\n", my_current_time(), msg);
		break;
	case 2:
		fprintf(log_fp,"%s\tError  \t%s\n", my_current_time(), msg);
		break;
	}
	fflush(log_fp);
}

/*
 *	int CReconGUIDlg::Build_e7_atten_Command()
 *  Build m_Tx_Command command line using "e7_atten" and the current parameter values to
 *  create the mu-map file.
 */
int CReconGUIDlg::Build_e7_atten_Command()
{
	CString cmd, str, msg;
	CString bl_c, tx_c;
	CString dir_path, qc_dir, log_dir;
	CHeader *bl_hdr=NULL, *tx_hdr=NULL;
	int count=0;
	char line[1024];
	std::string tag;
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	float bl_speed=0.0f, tx_speed=0.0f;
	m_TX_Seg_Method = Seg_None;

	if 	(m_Run_Tx.GetCheck() == 0)
	{
		log_message("CReconGUIDlg::Build_e7_atten_Command Run TX not selected", 2);
		return 0;
	}

	// at least one blank and one transmission
	if (m_Blank_Count < 1 || m_Tx_Count < 1) return 1;
	// Build the command
	/*
		e7_atten -b bl.s -t tx.s -w 128 --ou mu.i \
		--ualgo 40,1.5,30.,10.,5 -l 53 --model 328 --ucut \
		--prj ifore --oa atten_3d.a --span 9 --mrd 67 \
		-p 4,0,0.002,0.03,0.06,0.05,0.06,0.096,0.02,0.015,0.04,0.065
	*/
	cmd = "e7_atten_u --model 328";
	str.Format(" -b %s", (const char*)m_Blank_File[0]);
	cmd += str;
	str.Format(" -t %s", (const char*) m_Tx_File[0]);
	cmd += str;

	// Get blank and transmission speed
	bl_hdr = new CHeader();
	if (bl_hdr->OpenFile(m_Blank_File[0]+".hdr") >= 0)
		bl_hdr->Readfloat("axial velocity",&bl_speed);
	delete bl_hdr;

	tx_hdr = new CHeader();
	if (tx_hdr->OpenFile(m_Tx_File[0]+".hdr") >= 0)
		tx_hdr->Readfloat("axial velocity",&tx_speed);
	delete tx_hdr;
	
	if (bl_speed>0.0f && tx_speed>0.0f) 
	{
		str.Format(" --txblr %g", bl_speed/tx_speed);
		cmd += str;
	}


	if (m_Mu_Zoom[0]!=1 || m_Mu_Zoom[1]!=1) 
	{
		str.Format(" --uzoom %d,%d", m_Mu_Zoom[0], m_Mu_Zoom[1]);
		cmd += str;
	}
	if (m_Tx_Rebin) cmd += " -w 128";


	if (!m_Mu_Map_File.IsEmpty())
	{
		// Force overwrite existing file with --force
		if (_access((const char*)m_Mu_Map_File, 0) != -1)
			log_message("Existing mu-map file will be overwritten");
		cmd += " --force --ou " + m_Mu_Map_File;
	}
	else
	{
		log_message("output mumap filename empty");
		return 3;
	}


	// Enable segmentation
	if (m_Tx_Segment != 0 && m_MAP_prior.GetLength()>0 && !m_TX_TV3DReg)
	{
		str.Format(" --ualgo 40,1.5,%g,10.,5", m_MAP_smoothing);
//		cmd += " --ualgo 40,1.5,30.,10.,5";
		cmd += str;
		cmd += " -p ";
		cmd += m_MAP_prior;
		m_TX_Seg_Method = Seg_MAP_TR;
	}
	else
	{
		// cmd += " --ualgo 40,1.5,30.,0.,0";
		str.Format(" --ualgo 40,1.5,%g,0.,0", m_MAP_smoothing);
		cmd += str;
	}

	// Create QC directory
  get_directory(m_Mu_Map_File, qc_dir);
  qc_dir += "\\QC";
  if (_access(qc_dir,0) != 0)
  {
    if (_mkdir(qc_dir) != 0)
    {
      msg.Format("Error creating QC directory %s", (const char*)qc_dir);
      log_message(msg,2);
      return 0;
    }
	}
  
  // Create log directory
  get_directory(m_Mu_Map_File, log_dir);
  log_dir += "\\log";
  if (_access(log_dir,0) != 0)
  {
    if (_mkdir(log_dir) != 0)
    {
      msg.Format("Error creating log directory %s", (const char*)log_dir);
      log_message(msg,2);
      return 0;
    }
  }

	// set Logging path
	_splitpath((const char*)m_Mu_Map_File, drive, dir, fname, ext);
	str.Format(" -l 53,%s -q %s",(const char*)log_dir, (const char*)qc_dir);
	cmd += str;

	if (fabs(m_Tx_Scatter_Factors[1]-1.0f) > eps)
	{
		// Ignore scatter correction when using MAP-TR
		if (m_TX_Seg_Method != Seg_MAP_TR)
		 {
			 str.Format(" --txsc %g,%g", m_Tx_Scatter_Factors[0], 
				 m_Tx_Scatter_Factors[1]);
			 cmd += str;
		 }
	}
	else if (m_TX_TV3DReg)
	{   // Disable TX_TV3D Regularisation if no scatter correction
		m_TX_Seg_Method = Seg_None;
		m_TX_TV3DReg = FALSE; 
	}

	// ignore auto-scaling flag when segmentation enabled
	if (m_Tx_Segment == 0)
		if (!m_MU_scale) cmd += " --aus";

	OutputDebugString(cmd+"\n");
	
	m_Tx_Command = cmd;

	
	// write image header
	FILE* in_file = fopen(m_Tx_File[0]+".hdr","r");
	FILE *out_file = fopen(m_Mu_Map_File+".hdr","w");
	int image_size = m_Tx_Rebin? 128:256;
	float bin_size = 1.218750; // mm
	float pixel_size = m_Tx_Rebin? 2*bin_size:bin_size;
	
	if ((in_file != NULL) &&  (out_file != NULL))
	{
		while (fgets(line,1024,in_file) != NULL )
		{		
			tag.assign(line);
			//  Print the data file name
			if(tag.find("name of data file")!=-1 )
				fprintf( out_file, "name of data file := %s.i\n", fname);
			else if(tag.find("matrix size [1]")!=-1 )
				fprintf( out_file, "matrix size [1] := %d\n",image_size);
			else if(tag.find("matrix size [2]")!=-1 )
				fprintf( out_file, "matrix size [2] := %d\n",image_size);
			else if(tag.find("matrix size [3]")!=-1 )
				fprintf( out_file, "matrix size [3] := %d\n", 207);
			else if(tag.find("data format")!=-1 )
				fprintf( out_file, "data format := image\n");
			else if(tag.find("number format")!=-1 )
				fprintf( out_file, "number format := float\n");
			else if(tag.find("number of bytes per pixel")!=-1 )
				fprintf( out_file, "number of bytes per pixel := 4\n");
			else if(tag.find("scale factor (mm/pixel) [1]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [1] := %f\n", pixel_size);
			else if(tag.find("scale factor [2]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [2] := %f\n", pixel_size);	
			else if(tag.find("scale factor (mm/pixel) [3]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [3] := %f\n", bin_size);

			else if(tag.find("scaling factor (mm/pixel) [1]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [1] := %f\n", pixel_size);
			else if(tag.find("scaling factor [2]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [2] := %f\n", pixel_size);	
			else if(tag.find("scaling factor (mm/pixel) [3]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [3] := %f\n", bin_size);				
			else
			{
				if (tag.find("singles") == -1) fprintf( out_file, "%s", tag.c_str());
			}			
		}
		fclose(in_file);
	}
	else
	{
		if (out_file != NULL)
		{
			fprintf( out_file, "!INTERFILE\n");
			fprintf( out_file, "name of data file := %s\n",fname,ext);
			fprintf( out_file, "matrix size [1] := %d\n", image_size);		
			fprintf( out_file, "matrix size [2] := %d\n", image_size);		
			fprintf( out_file, "matrix size [3] := %d\n", 207);
			fprintf( out_file, "data format := image\n");
			fprintf( out_file, "number format := float\n");
			fprintf( out_file, "number of byte per pixel := 4\n");
			fprintf( out_file, "scaling factor (mm/pixel) [1] := %f\n", pixel_size);
			fprintf( out_file, "scaling factor (mm/pixel) [2] := %f\n", pixel_size);	
			fprintf( out_file, "scaling factor (mm/pixel) [3] := %f\n", bin_size);				
		}
	}
	if (out_file != NULL)
	{
		fprintf( out_file, "blank file name used := %s\n", (const char*)m_Blank_File[0]);
		fprintf( out_file, "transmission file name used := %s\n", (const char*)m_Tx_File[0]);
		if (m_Tx_Segment && m_TX_TV3DReg) 
		{
			fprintf( out_file, "segmentation method := TX_TV3DReg 0.5,2,3\n");
		}
		else if (m_TX_Seg_Method == Seg_MAP_TR) 
			fprintf(out_file, "segmentation method := MAP-TR %s\n", m_MAP_prior);

		fclose(out_file);
	}
	return 0;
}

int CReconGUIDlg::Build_Tx_Command()
{
	int num,i;
	CString cmd, str;

	if 	(m_Run_Tx.GetCheck() == 0)
		return 0;

	// Histogram
	Histogram(m_Blank_File[0], TASK_BL_HISTOGRAM);
	Histogram(m_Tx_File[0], TASK_TX_HISTOGRAM);
	
	// Build the command
	if (m_Tx_Method == 2) return Build_e7_atten_Command();
	
	cmd = "Tx_Process ";

	num = m_Blank_Count;
	if (num > 0 && num < 6)
	{
		for (i=0;i<num;i++)
			cmd.operator += ("-b " + m_Blank_File[i] + " ");
	}
	else
		return 1; // no balnk file are selected

	// Add Tx files
	// Build the command
	num = m_Tx_Count;
	if (num > 0 && num < 6)
	{
		for (i=0;i<num;i++)
			cmd.operator += ("-t " + m_Tx_File[i] + " ");
	}
	else		
		return 2; // no balnk file are selected


	str.Format("-d %d -s %d ", m_Ary_Ring[m_Ary_inc], m_Ary_Span[m_Ary_inc] );
	cmd.operator += ( str );

	if(m_Tx_Method == 0)
		str = "-w 0 ";
	else
		str = "-w 1 ";

	cmd.operator += (str);

	// Enable rebin
	if (m_Tx_Rebin)	
	{
		str = "-r 1 ";
		cmd.operator += (str);
	}
	if (!m_Mu_Map_File.IsEmpty())
	{
		str.Format("-i %s ", m_Mu_Map_File);
	}
	else
	{
		return 3;
	}
	cmd.operator += (str);


	if (!m_2D_Atten_File.IsEmpty())
	{
		str.Format("-a %s ", m_2D_Atten_File);
	}
	else
	{
		return 4;
	}

	cmd.operator += (str);

	if (!m_Atten_Correction.IsEmpty())
	{
		str.Format("-c %s ", m_Atten_Correction);
	}
	else
	{
		return 5;
	}

	cmd.operator += (str);

	// Enable segmentation
	if (m_Tx_Segment != 0)
	{
		str.Format("-m %f,%f,%f ",m_Bone_Seg,m_Water_Seg,m_Noise_Seg);
		cmd.operator += (str);
	}

	OutputDebugString(cmd+"\n");
	
	m_Tx_Command = cmd;

	return 0;
}


void CReconGUIDlg::OnButton4() 
{
	CAtten_Process pDlg;
	pDlg.DoModal();
		
}

void CReconGUIDlg::OnButton8() 
{
	CScatter_Config pDlg;
	pDlg.DoModal();
			
}

void CReconGUIDlg::OnButton5() 
{
	int nRetValue=0;
	long valErr;
	nRetValue = Build_At_Command();
	if(m_Aten_OK && nRetValue ==0)
	{
		
		valErr = CheckAtFiles();
		if (valErr == 0)
		{	// Run Attenuation Process
			// RunAndForgetProcess(m_At_Command,"Attenuation Process", &nRetValue);
			SubmitTask(m_At_Command, TASK_AT_PROCESS, m_3D_Atten_File);
			//MessageBox(m_At_Command,"This is the command to execute Attenuation Process",MB_OK);
			if (nRetValue != 0)
				// Error Message Only
				MessageBox("A Problem Encountered with Attenuation Process.  Aborting Process","Attenuation Process Run Error",MB_ICONSTOP);
		}

	}
	else
		if (nRetValue ==0)
			MessageBox("Attenuation Process is not configured Properly.","Attenuation Process Configuration Error",MB_ICONSTOP);
	
}

int CReconGUIDlg::Build_At_Command()
{
	CString cmd, str, log_dir, msg;

	if 	(m_Run_at.GetCheck() == 0)
		return 0;

	//cmd = "HRRT_Foreproj ";

/*	cmd = "ReconEng ";
	// Inputfile 2D atten
	if (!m_2D_Atten_File.IsEmpty())
	{
		str.Format("-a %s ", m_2D_Atten_File);
	}
	else
	{
		return 1;
	}
	cmd.operator += (str);
	
	// Output file 3D atten
	if (!m_3D_Atten_File.IsEmpty())
	{
		str.Format("-o %s ", m_3D_Atten_File);
	}
	else
	{
		return 2;
	}
	cmd.operator += (str);


	str.Format("-m %d,%d ",m_Ary_Span[m_Ary_inc], m_Ary_Ring[m_Ary_inc]);
	cmd.operator += (str);

	str = "-v 2 ";
	cmd.operator += (str);
	if (!m_2D_Atten_File.IsEmpty())
		str.Format("-A %s ", m_2D_Atten_File);
	else
		return 1;
	cmd.operator += (str);
*/
	cmd = "e7_fwd_u --model 328 ";
	if (!m_Mu_Map_File.IsEmpty())
		str.Format("-u %s ", m_Mu_Map_File);
	else
		return 1;
	cmd.operator  += (str);
	if (m_Tx_Rebin) cmd += " -w 128 ";
	else cmd += " -w 256 ";
	// Output file 3D atten
	if (!m_3D_Atten_File.IsEmpty())
		str.Format("--oa %s ", m_3D_Atten_File);
	else
		return 2;
	cmd.operator  += (str);

	if (_access((const char*)m_3D_Atten_File,0)!= -1)
	{
		log_message("Existing attenuation file will be ovewritten");
	}

	// Force  overwrite existing file with --force
	str.Format("--span %d --mrd %d --prj ifore --force",m_Ary_Span[m_Ary_inc], m_Ary_Ring[m_Ary_inc]);
	cmd.operator += (str);

  get_directory(m_3D_Atten_File, log_dir);
  log_dir += "\\log";
  if (_access(log_dir,0) != 0)
  {
    if (_mkdir(log_dir) != 0)
    {
      msg.Format("Error creating log directory", (const char*)log_dir);
      log_message(msg,2);
      return 0;
    }
  }
  cmd += " -l 73,";
  cmd += log_dir;

	OutputDebugString(cmd+"\n");
	
	m_At_Command = cmd;

	return 0;
}

void CReconGUIDlg::OnOK() 
{
	int nRetValue, st=0, en=1, l;
	long valErr;
	CString str;
	char cstr[128];
	NewJob();
	unsigned hist_status = Histogram();
	nRetValue = Build_Tx_Command();
	nRetValue = Build_At_Command();

	if (nRetValue ==  0)
	{
		if (m_multi_frames)
		{
			Get_MultiFrame_Files();
			st = 0;
			en = m_num_of_emiss;
		}
		// check the normalization and emission file if the scatter or recon is selected
		if 	((m_Run_sc.GetCheck() != 0)	|| (m_Run_sc.GetCheck() != 0))
			valErr = CheckInputFiles();
		else
			valErr = 0;

		if (valErr == 0)
		{
			if(m_Aten_OK && m_Tx_OK && m_Scatter_OK && m_Recon_OK)
			{
				if 	(m_Run_Tx.GetCheck() != 0)
					valErr = CheckTxFiles();
				if (valErr == 0)
				{
					nRetValue = 0;
					if 	(m_Run_Tx.GetCheck() != 0)
//						RunProcessAndWait(m_Tx_Command,"TX Process", &nRetValue);
						SubmitTask(m_Tx_Command, TASK_TX_PROCESS, m_Mu_Map_File);
					if (nRetValue == 0)
					{
						// Attenuation Process already done when using MAP-TR
						if (m_Tx_Method != 2 && m_Run_at.GetCheck() != 0)
								valErr = CheckAtFiles();
						if (valErr == 0)
						{	// Run attenuation Process
							if 	(m_Run_at.GetCheck() != 0)
//								RunProcessAndWait(m_At_Command,"Attenuation Process", &nRetValue);
								SubmitTask(m_At_Command, TASK_AT_PROCESS, m_3D_Atten_File);
							if (nRetValue == 0)
							{
								for(l=st;l<en;l++)
								{
//									log_message("Get EM File");
									if (m_multi_frames)
									{ 
										GetEmFilename(l,&m_Em_File);
										if (l>st) 
										{												
											SubmitTask(tmp_files, TASK_CLEAR_TMP_FILES, NULL);
											NewJob();
										}
										// last frame, clean the atten file if being copy to cluster
										//if (l==en-1)
										//{   
										//	AddTmpFile(tmp_files, m_3D_Atten_File);
										//}
									}
//									log_message("Get EM File OK");

	//								Build_Scatter_Command();

									Build_e7_scatter_Command(l);
									
									// Check scatter files only for first frame
									if 	(l==st)
									{
										log_message("Frame 0 : check scatter files");
										if (m_Run_sc.GetCheck() != 0)
										valErr = CheckScattFiles();
									}
									else log_message("Skip check scatter files for not first frame");
									if (valErr == 0)
									{	// Run Scatter Process
										if 	(m_Run_sc.GetCheck() != 0)
//											RunProcessAndWait(m_Sc_Command,"Scatter Correction Process", &nRetValue);
											SubmitTask(m_Sc_Command, TASK_SCAT_PROCESS, m_Scatter_File);
										//MessageBox(m_Sc_Command,"This the command to execute scatter correction",MB_OK);
										if (nRetValue == 0)
										{
											if 	(m_Run_rc.GetCheck() != 0)
											{
												// run image reconstruction
												// Check Recon files only for first frame
												if 	(l==st)
													valErr = CheckReconFiles();
												else log_message("Skip check recon files for not first frame");
												if (valErr == 0)
												{
													if (!m_Quan_Header.IsEmpty()) AddQuantInfo(m_Em_File, m_Quan_Header);
													// call Build_Recon_Command after scatter task submission
													// to make sure scatter file will be created before 
													// starting recon_copy_files tasks
													if (m_multi_frames) Build_Recon_Command(l);
													else Build_Recon_Command(-1);
													// Run Reconstruction Process
													if (m_Type == 0)
														sprintf(cstr,"SSR/DIFT Reconstruction Process");
													else if (m_Type == 1)
														sprintf(cstr, "FORE/DIFT Reconstruction Process");
													else if (m_Type == 2)
														sprintf(cstr ,"FORE/OSEM Reconstruction Process");
													else if (m_Type == 3)
														sprintf(cstr ,"OSEM3D Reconstruction Process");
													else if (m_Type == 4)
														sprintf(cstr ,"FORE/HOSP Reconstruction Process");
//													RunProcessAndWait(m_Rc_Command,cstr, &nRetValue);
													if (m_Type == 3 && m_multi_frames && submit_recon_dir.GetLength() > 0)
														SubmitTask(m_Rc_Command,TASK_COPY_RECON_FILE, NULL);
													else
														SubmitTask(m_Rc_Command,TASK_RECON, m_Image);

													if (m_Type == 4)
													{	
														CreateHOSPFile();
//														RunProcessAndWait(m_HOSP_Cmd,cstr, &nRetValue);
														SubmitTask(m_HOSP_Cmd,TASK_HOSP_RECON, m_Image);
													}

													// Write Header and quantify if not cluster reconstruction
													if (submit_recon_dir.GetLength() == 0) 
													{
														if (m_Type == 3 ||	m_Type == 4)// If Local OSEM3d then Quantify
														{
															// Write Image Header
															WriteImageHeader();
															// Quantify image
															//QuantifyImage();
															/* Don't quantify images to be comaptible with cluster reconstruction
															SubmitTask(m_Image, TASK_QUANTIFY_IMAGE, m_Image);
															*/
														}
														AppendImageHeader();
													}
												}
												else
												{
													str.Format("%d",nRetValue);	// Error Message Only
													MessageBox("A Problem Encountered with Reconstruction Process.  Aborting Process. "+ str,"Scatter Process Run Error",MB_ICONSTOP);
												}
											}
										}
										else
										{
											str.Format("%d",nRetValue);	// Error Message Only
											MessageBox("A Problem Encountered with Scatter Correction Process.  Aborting Process. "+ str,"Scatter Process Run Error",MB_ICONSTOP);
										}	
									}
								}
							}
							else
							{
								str.Format("%d",nRetValue);						// An error occured with Attenuation Process
								MessageBox("A Problem Encountered with Attenuation Process.  Aborting Process. " + str,"Atten Process Run Error",MB_ICONSTOP);
							}
						}
					}
					else
					{
						str.Format("%d",nRetValue);
						// An error occured with Tx Process
						MessageBox("A Problem Encountered with Tx Process.  Aborting Process. " + str,"Tx Process Run Error",MB_ICONSTOP);
					}
					SubmitTask(tmp_files, TASK_CLEAR_TMP_FILES, NULL);
				}
			}
			else
				MessageBox("One of the Processes is not set up properly.  Aborting Process","Command Setup Error",MB_ICONSTOP);
		}
	}
}

int CReconGUIDlg::Build_Scatter_Command()
{
	if 	(m_Run_sc.GetCheck() == 0)
		return 0;
	CString cmd, str;
	CFile file;
	CString qc_dir;


	// Create QC directory
	if (!m_QC_Path.IsEmpty())
	{
		get_directory(m_QC_Path, qc_dir);
		if (_access(qc_dir,0) != 0)
		{
			if (_mkdir(qc_dir) != 0)
			{
				str.Format("Error creating directory", (const char*)qc_dir);
				log_message(str,2);
				return 0;
			}
		}
	}
	//cmd = "Cal_Scatter ";
	cmd = "ReconEng ";
	// Inputfile 3D atten
	if (!m_3D_Atten_File.IsEmpty())
	{
		str.Format("-a %s ", m_3D_Atten_File);
	}
	else
	{
		return 1;
	}
	cmd.operator += (str);
	
	// Input file emission 
	if (!m_Em_File.IsEmpty())
	{
		str.Format("-e %s ", m_Em_File);
	}
	else
	{
		return 2;
	}
	cmd.operator += (str);

	// Input file normalization
	if (!m_Norm_9_File.IsEmpty())
	{
		str.Format("-n %s ", m_Norm_9_File);
	}
	else
	{
		return 3;
	}
	cmd.operator += (str);


	// Input file normalization
	if (!m_Mu_Map_File.IsEmpty())
	{
		str.Format("-i %s ", m_Mu_Map_File);
	}
	else
	{
		return 4;
	}
	cmd.operator += (str);

	// Output file normalization
	if (!m_Scatter_File.IsEmpty())
	{
		str.Format("-o %s ", m_Scatter_File);
	}
	else
	{
		return 5;
	}
	cmd.operator += (str);


	// LLD and ULD
	str.Format("-l %d -u %d ", m_LLD, m_ULD );
	cmd.operator += ( str );

	// Span and Ring Difference
	str.Format("-d %d -s %d ", m_Ary_Ring[m_Ary_inc], m_Ary_Span[m_Ary_inc] );
	cmd.operator += ( str );

	if(!m_QC_Path.IsEmpty())
	{
//		if(m_Sc_Debug_Path.Mid(m_Sc_Debug_Path.GetLength()-1) != "\\")
//			m_Sc_Debug_Path += "\\";
		cmd += "-g 1 -p ";
		cmd += qc_dir;
		cmd += "\\";
	}
	else
		return 6;

	OutputDebugString(cmd+"\n");
	
	m_Sc_Command = cmd;
	
	m_Scatter_OK = true;

	return 0;
}


/*
 *	int CReconGUIDlg::Build_e7_scatter_Command(int frame_num)
 *  Build m_Sc_Command command line using "e7_sino" and the current parameter values to
 *  create the scatter file.
 */

int CReconGUIDlg::Build_e7_scatter_Command(int frame_num)
{
	if 	(m_Run_sc.GetCheck() == 0)
		return 0;
	CString cmd, str,msg;
	CString qc_dir, log_dir;
	CFile file;
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	// Create QC directory
	if (!m_QC_Path.IsEmpty())
	{
		get_directory(m_QC_Path, qc_dir);
		if (_access(qc_dir,0) != 0)
		{
			if (_mkdir(qc_dir) != 0)
			{
				msg.Format("Error creating directory", (const char*)qc_dir);
				log_message(msg,2);
				return 0;
			}
		}
	}
	//cmd = "Cal_Scatter ";
	cmd = "e7_sino_u ";
	// Inputfile 3D atten
	if (!m_3D_Atten_File.IsEmpty())
	{
		str.Format("-a %s -u %s -w 128 ", m_3D_Atten_File, m_Mu_Map_File);
	}
	else
	{
		log_message("CReconGUIDlg::Build_e7_scatter_Command Input atten_3d is empty",2);
		return 1;
	}
	cmd.operator += (str);
	
	// Validate Emission File if no previous task is queued
	int em_hist_key = TASK_EM_HISTOGRAM + m_JobNum*MAX_NUM_TASKS;
	if (frame_num>0) em_hist_key = TASK_EM_HISTOGRAM + (m_JobNum-frame_num)*MAX_NUM_TASKS;
	if (queue.find(em_hist_key) == queue.end())
	{ 
		if (!m_Em_File.IsEmpty())
		{
			CHeader em_hdr;
			int prompt_flag=0;
			char line[FILENAME_MAX];
			char true_filename[FILENAME_MAX];
			str.Format("-e %s ", m_Em_File);
			if (em_hdr.OpenFile(m_Em_File + ".hdr") >= 0)
			{
				if (em_hdr.Readchar("sinogram data type", line, FILENAME_MAX) == 0)
				{
					if (strnicmp(line,"prompt", 6) == 0)
					{ // get true data file
						if (em_hdr.Readchar("name of true data file", line, FILENAME_MAX) == 0)
						{ 	// Use in prompts dir
							_splitpath(line,drive, dir, fname, ext);
							sprintf(true_filename,"%s%s", fname,ext);
							_splitpath(m_Em_File,drive, dir, fname, ext);
							sprintf(line,"%s%s%s",drive,dir, true_filename);
							if (_access(line,0) != 0)
							{
								msg.Format("true data file ( %s ) not found for scatter correction", line);
								log_message(msg);
								return 2;
							}
							str.Format("-e %s ", line);
						}
						else
						{
							log_message("prompt data: true data file not specified for scatter correction", 2);
							return 2;
						}
					}
				}
				else
				{
					log_message("emission sinogram data type not specified: assuming true");
				}
			}
			else
			{
				log_message("Error opening emission sinogram header: assuming true");
			}
		}
		else
		{
			log_message("CReconGUIDlg::Build_e7_scatter_Command Input Emission is empty",2);
			return 2;
		}
	}
	else
	{
		str.Format("-e %s ", m_Em_File);
		if (m_Weight_method == 3) str.Replace(".s", ".tr.s"); // OP-OSEM
	}
	cmd.operator += (str);

	// Input file normalization
	if (!m_Norm_9_File.IsEmpty())
	{
		float lber;
		CHeader *norm_hdr= new CHeader();
		str.Format("-n %s ", m_Norm_9_File);
		if (norm_hdr->OpenFile(m_Norm_9_File + ".hdr") >= 0)
		{
			if (norm_hdr->Readfloat("LBER",&lber)==0)
				str.Format("-n %s --lber %g ", m_Norm_9_File, lber);
			else log_message("CReconGUIDlg::Build_e7_scatter_Command "
				"LBER not found in normalization header");
		} else log_message("CReconGUIDlg::Build_e7_scatter_Command "
			"normalization header not found");
		delete norm_hdr;
	}
	else
	{
		log_message("CReconGUIDlg::Build_e7_scatter_Command Input Normalization is empty",2);
		return 3;
	}
	cmd.operator += (str);

	// Output file 
	if (!m_Scatter_File.IsEmpty())
	{
		// Force overwrite existing file with --force
		if (_access((const char*)m_Scatter_File, 0) != -1)
			log_message("Existing scatter file will be overwritten");
		str.Format("--force --os %s --os2d ", m_Scatter_File);
	}
	else
	{
		log_message("CReconGUIDlg::Build_e7_scatter_Command output scatter filename empty", 2);
		return 5;
	}
	cmd.operator += (str);



	// set gap fill flag and model number
	cmd += "--gf --model 328 --skip 2 ";

	// Span and Ring Difference
	str.Format("--mrd %d --span %d", m_Ary_Ring[m_Ary_inc], m_Ary_Span[m_Ary_inc] );
	// use --mem .7 for span 3 to avoid exhausted memory error
	if (m_Ary_Span[m_Ary_inc] == 3) str += " --mem 0.7";
	cmd.operator += ( str );

  get_directory(m_Scatter_File, log_dir);
  log_dir += "\\log";
  if (_access(log_dir,0) != 0)
  {
    if (_mkdir(log_dir) != 0)
    {
      msg.Format("Error creating log directory", (const char*)log_dir);
      log_message(msg,2);
      return 0;
    }
  }

	if(!m_QC_Path.IsEmpty())
	{	
		if (_access((const char*)qc_dir,2)!= -1)
		{
			str.Format(" -l 73,%s -q %s",(const char*)log_dir, (const char*)qc_dir);
			cmd.operator += ( str );
		}
		else
		{
			msg.Format("CReconGUIDlg::Build_e7_scatter_Command debug directory %s not writeable",
				(const char*)qc_dir);
			log_message(msg,2);
			return 6;
		}
	}
	else
	{
		log_message("CReconGUIDlg::Build_e7_scatter_Command QC path is empty");
	}

	// Scatter scaling limits
	str.Format(" --ssf %g,%g", ss_limits[0], ss_limits[1]);
	cmd += str;
	OutputDebugString(cmd+"\n");
	
	log_message(cmd);

	m_Sc_Command = cmd;
	
	m_Scatter_OK = true;
	return 0;
}

void CReconGUIDlg::GetIniFile()
{
	CString str,newstr;
	int num=0, len=0, offset=0;
	int continue_flag=0;
	char t[256];

	str.Format("%s\\recon.ini",m_AppPath);
	// Open the header file
	FILE  *in_file;
	in_file = fopen(str,"r");
	if( in_file!= NULL )
	{
		// Read the file line by line and sort data
		while (fgets(t,256,in_file) != NULL )
		{
			len = strlen(t); // remove trailing spaces and "\n";
			while (len>0 && isspace(t[len-1]))
			{
				len--;
				t[len] = '\0';
			}
			if (continue_flag)
			{// remove heading spaces
				offset = 0;
				while (isspace(t[offset]) && offset<len) offset++;
				if (t[len-1] == '$')
				{  
					t[len-1] = '\0';
					str += (t+offset);
				}
				else
				{
					str += (t+offset);
					continue_flag = 0;
				}
			}
			else
			{
				if (t[len-1] == '$') 
				{  // remove continue character
					t[len-1] = '\0';
					continue_flag = 1;
				}
				str = t;
			}
				//  What type of system
			if (!SortData (str, newstr)) continue;
			if(str.Find("Blank Filename :=")!=-1)
			{
				m_Blank_File[m_Blank_Count] = newstr;
				m_Blank_Count ++;
			}
			/*if(str.Find("Tx Filename :=")!=-1)
			{
				m_Tx_File[m_Tx_Count] = newstr;
				m_Tx_Count ++;
			}*/
			
			if(str.Find("Normalization Filename Span 9:=")!=-1)
				m_Norm_9_File = newstr;

			if(str.Find("Normalization Filename Span 3:=")!=-1)
				m_Norm_3_File = newstr;



			if(str.Find("Rebin Tx Option (0 disabled, else enabled) :=")!=-1)
				if (atoi(newstr) == 0)
					m_Tx_Rebin = 0;
				else
					m_Tx_Rebin = 1;

			if(str.Find("Mu Zoom :=")!=-1)
			{
				int tx_sino_rebin[2];
				if (sscanf(newstr, "%d,%d", &tx_sino_rebin[0], &tx_sino_rebin[1]) == 2)
				{ 
					if (tx_sino_rebin[0] == 2) m_Mu_Zoom[0] = 2; // supported values are 1,2
					if (tx_sino_rebin[1] == 1 || tx_sino_rebin[1] == 2 ) 
            m_Mu_Zoom[1] = tx_sino_rebin[1]; // supported values are 1,2,3
				}
			}

 /*  Removed 02-dec-2009
			if(str.Find("EM histogram start countrate :=")!=-1)
				sscanf(newstr, "%d", &m_start_countrate);
*/
      if(str.Find("TX Scatter Factors :=")!=-1)
				sscanf(newstr, "%g,%g", &m_Tx_Scatter_Factors[0], &m_Tx_Scatter_Factors[1]);
			
			if(str.Find("Span and Ring Difference Option (0 3/67, 1 9/67) :=")!=-1)
				if (atoi(newstr)==0)
					m_Ary_inc = 0;
				else
					m_Ary_inc = 1;
		
			if(str.Find("Lower Level Energy Window LLD :=")!=-1)
			{
				m_LLD = atoi(newstr);
			}

			if(str.Find("Upper Level Energy Window ULD :=")!=-1)
				m_ULD = atoi(newstr);
		
			if(str.Find("Auto Assign Output Filename :=")!=-1)
				if (atoi(newstr) == 0)
					m_Auto_Assign.SetCheck(0);
				else
					m_Auto_Assign.SetCheck(1);

			if(str.Find("Keep Temp Files :=")!=-1)
			{
				if (atoi(newstr) == 0) m_KeepTempCheck.SetCheck(0);
				else m_KeepTempCheck.SetCheck(1);
			}

			if(str.Find("Segmentation of mu map (0=disable, 1=enable) :=")!=-1)
				if (atoi(newstr) == 0)
					m_Tx_Segment = 0;
				else
					m_Tx_Segment = 1;

			if(str.Find("Segmentation of bone threshold :=")!=-1)
				m_Bone_Seg = (float)atof(newstr);
			if(str.Find("Segmentation of water threshold :=")!=-1)
				m_Water_Seg = (float)atof(newstr);
			if(str.Find("Segmentation of noise suppress threshold :=")!=-1)
				m_Noise_Seg = (float)atof(newstr);


			if(str.Find("Scatter debug option (0=disable, 1=enable) :=")!=-1)
				if (atoi(newstr) == 0)
					m_Sc_Debug = 0;
				else
					m_Sc_Debug = 1;


			/*
			 *	QC working directory will be derived from QC output file
			 */
			//if(str.Find("Scatter debug path :=")!=-1)
			//	m_Sc_QC_Path = newstr;

			if(str.Find("Sinogram Data Dir :=")!=-1)
				sinogram_data_dir = newstr;

			if(str.Find("gnuplot command :=")!=-1)	m_gnuplot_Cmd = newstr;

			if(str.Find("Image reconstruction Option ")!=-1)
				if (atoi(newstr) >= 0 && atoi(newstr) < 5)
					m_Type = atoi(newstr);
				else
					m_Type = 0;
				
			if(str.Find("Recon Rebin Option (0 disabled, else enabled) :=")!=-1)
				if (atoi(newstr) == 0)
					m_Recon_Rebin = 0;
				else
					m_Recon_Rebin = 1;	
				
			if(str.Find("Quantification header name and path :=")!=-1)
			{
				m_Quan_Header = newstr;
			}
			if(str.Find("Number of subsets for OSEM :=")!=-1)
				m_SubSets = atoi(newstr);
			if(str.Find("Number of iterations for OSEM :=")!=-1)
				m_Iteration = atoi(newstr);

			if (str.Find("OSEM3D weighting method (0=UW, 1=AW, 2=ANW, 3=OP) :=") != -1)
				m_Weight_method = atoi(newstr);

			if(str.Find("Tx Rebin Option (0 disabled, else enabled) :=")!=-1)
				if (atoi(newstr) == 0)
					m_Tx_Rebin = 0;
				else
					m_Tx_Rebin = 1;	

			// Mu reconstruction Option (0=UWOSEM, 1=NEC, 2=MAP-TR)
			// Don't add MAP-TR to search string for previous recon.ini files
			if(str.Find("Mu reconstruction Option (0=UWOSEM, 1=NEC")!=-1)
				switch(atoi(newstr))
				{
					case 0:	m_Tx_Method = 0; break;
					case 1: m_Tx_Method = 1; break;
					default: m_Tx_Method = 2; break; 
				}
			if(str.Find("Use PSF") != -1)
				if (atoi(newstr) != 0) m_PSF = TRUE;
				else m_PSF = FALSE;

			if(str.Find("Start Plane for HOSP :=")!=-1)
				m_StartPlane = atoi(newstr);
			if(str.Find("End Plane for HOSP :=")!=-1)
				m_EndPlane = atoi(newstr);			

			if(str.Find("Run TX Process on Execute All (0 disabled, else enabled) :=") !=-1)
			{	num = atoi(newstr);
				m_Run_Tx.SetCheck(num);
			}			
			if(str.Find("Run Atten Process on Execute All (0 disabled, else enabled) :=") !=-1)
			{	num = atoi(newstr);
				m_Run_at.SetCheck(num);
			}
			if(str.Find("Run Scatter Process on Execute All (0 disabled, else enabled) :=") !=-1)
			{	num = atoi(newstr);
				m_Run_sc.SetCheck(num);
			}
			if(str.Find("Run Recon Process on Execute All (0 disabled, else enabled) :=") !=-1)
			{	num = atoi(newstr);
				m_Run_rc.SetCheck(num);
			}
			if(str.Find("MAP-TR Prior :=") !=-1)
			{
				InitPriors(newstr);
			}
			if(str.Find("Submit Recon Dir :=") !=-1)
			{
				submit_recon_dir = newstr;
			}
			if(str.Find("MAP-TR Beta Smoothing :=") !=-1)
				sscanf((const char*)newstr,"%g", &m_MAP_smoothing);

			if(str.Find("Head Types (0=Cologne, 1=Standard) :=") !=-1)
			{
				num = atoi(newstr);
				head_types = num==0? KOLN_SPECIAL: LSO_LYSO;
			}
			if(str.Find("Scatter scaling limits :=") !=-1)
			{
				sscanf(newstr, "%g,%g", &ss_limits[0], &ss_limits[1]);
				head_types = num==0? KOLN_SPECIAL: LSO_LYSO;
			}
		}
		fclose(in_file);
	}
	if (submit_recon_dir.GetLength()==0)
		log_message("\"Submit Recon Dir\" not found: Cluster reconstruction will not be used");
	if (map_Priors.size() == 0)
	{ // missing or invalid priors list
		InitPriors(m_MAP_prior);
	}
	m_MAP_prior = map_Priors.begin()->second;
	if (map_Priors.begin()->first.Find("TX_TV3DReg") == -1) m_TX_TV3DReg = FALSE;
	else m_TX_TV3DReg = TRUE;
}

int CReconGUIDlg::SortData(CString &str, CString &newstr)
{
	int y = str.Find(":=");
	newstr = "";
	if (y>=0)
	{
		int z = str.GetLength();
		y += 2;
		while (y<z && isspace(str[y])) y++;
		if (z>y) newstr = str.Mid(y,z - y);
	}
	return newstr.GetLength();
}

void CReconGUIDlg::SaveIniFile()
{
	int i;
	CString str;
	FILE  *out_file;
	

	str.Format("%s\\recon.ini",m_AppPath);
	out_file = fopen(str,"w");
	if (out_file != NULL)
	{

			// Save termination characters
		fprintf (out_file,"Head Types (0=Cologne, 1=Standard) := %d\n", head_types);
		for (i=0;i<m_Blank_Count;i++)
			fprintf( out_file, "Blank Filename := %s\n",m_Blank_File[i]);
//		for (i=0;i<m_Tx_Count;i++)
//			fprintf( out_file, "Tx Filename := %s\n",m_Tx_File[i]);

		fprintf( out_file, "Normalization Filename Span 3:= %s\n",m_Norm_3_File);
		fprintf( out_file, "Normalization Filename Span 9:= %s\n",m_Norm_9_File);
		
		
		fprintf( out_file, "Span and Ring Difference Option (0 3/67, 1 9/67) := %d\n",m_Ary_inc);

		fprintf( out_file, "Lower Level Energy Window LLD := %d\n",m_LLD);
		fprintf( out_file, "Upper Level Energy Window ULD := %d\n",m_ULD);

		fprintf( out_file, "Auto Assign Output Filename := %d\n",m_Auto_Assign.GetCheck());
		fprintf( out_file, "Keep Temp Files := %d\n",m_KeepTempCheck.GetCheck());
		
		fprintf( out_file, "Segmentation of mu map (0=disable, 1=enable) := %d\n",m_Tx_Segment);
		fprintf( out_file, "Segmentation of bone threshold := %f\n",m_Bone_Seg);
		fprintf( out_file, "Segmentation of water threshold := %f\n",m_Water_Seg);
		fprintf( out_file, "Segmentation of noise suppress threshold := %f\n",m_Noise_Seg);

		fprintf( out_file, "Scatter debug option (0=disable, 1=enable) := %d\n",m_Sc_Debug);
		fprintf( out_file, "Scatter scaling limits := %g,%g\n", ss_limits[0], ss_limits[1]);

//		fprintf( out_file, "Scatter debug path := %s\n",m_Sc_Debug_Path);

		fprintf( out_file, "Sinogram Data Dir := %s\n",sinogram_data_dir);
		fprintf( out_file, "gnuplot command := %s\n",m_gnuplot_Cmd);

		fprintf( out_file,"Image reconstruction Option (0=SSR/DIFT, 1=FORE/DIFT, 2=FORE/OSEM 3=OSEM3D) := %d\n", m_Type);

		fprintf( out_file, "Recon Rebin Option (0 disabled, else enabled) := %d\n",m_Recon_Rebin);
		fprintf( out_file, "Quantification header name and path := %s\n",m_Quan_Header);


		fprintf( out_file, "Number of subsets for OSEM := %d\n",m_SubSets);
		fprintf( out_file, "Number of iterations for OSEM := %d\n",m_Iteration);
		fprintf( out_file, "OSEM3D weighting method (0=UW, 1=AW, 2=ANW, 3=OP) := %d\n",	m_Weight_method);
		fprintf( out_file, "Use PSF (0=NO, 1=YES) := %d\n",	m_PSF? 1: 0);


		fprintf( out_file,"Tx Rebin Option (0 disabled, else enabled) := %d\n",m_Tx_Rebin);
		fprintf( out_file,"Mu Zoom := %d,%d\n",m_Mu_Zoom[0], m_Mu_Zoom[1]);
		fprintf( out_file, "TX Scatter Factors := %g,%g\n", m_Tx_Scatter_Factors[0], m_Tx_Scatter_Factors[1]);
/* Removed 02-dec-2009
    fprintf( out_file, "EM histogram start countrate := %d\n", m_start_countrate);
*/

		fprintf( out_file,"Mu reconstruction Option (0=UWOSEM, 1=NEC, 2=MAP-TR) := %d\n",m_Tx_Method);
		if (map_Priors.size()>0) 
		{
			CString spec;
			for (MAP_TR_PriorMapIterator it=map_Priors.begin(); it!=map_Priors.end(); it++ )
			{
				if (spec.GetLength()>0)	spec += ";$\n\t"; // continued line
				spec += it->first;
				spec += "=";
				spec += it->second;
			}			
			fprintf( out_file,"MAP-TR Prior := %s\n", (const char*)spec);
		}
		if (submit_recon_dir.GetLength()>0) 
		{
			fprintf( out_file,"Submit Recon Dir := %s\n", (const char*)submit_recon_dir);
		}
		fprintf( out_file,"MAP-TR Beta Smoothing := %g\n",m_MAP_smoothing);
		fprintf( out_file,"Start Plane for HOSP := %d\n",m_StartPlane);
		fprintf( out_file,"End Plane for HOSP := %d\n",m_EndPlane);
				
		fprintf( out_file,"Run TX Process on Execute All (0 disabled, else enabled) := %d\n",m_Run_Tx.GetCheck());		
		fprintf( out_file,"Run Atten Process on Execute All (0 disabled, else enabled) := %d\n",m_Run_at.GetCheck());

		fprintf( out_file,"Run Scatter Process on Execute All (0 disabled, else enabled) := %d\n",m_Run_sc.GetCheck());		
		fprintf( out_file,"Run Recon Process on Execute All (0 disabled, else enabled) := %d\n",m_Run_rc.GetCheck());
	
		fclose(out_file);

	}
}

void CReconGUIDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
//	if (m_multi_frames)
//		free(m_multi_emiss);
	if (queue.size() > 0)
	{
		if (MessageBox("Waiting tasks will be lost! Exit Now?", "Confirm Exit", MB_YESNO) == IDNO) return;
	}
	SaveIniFile();
	CDialog::OnCancel();
}

long CReconGUIDlg::ValidateFile(CString FilenamePath, int type, int span_id)
// type 0=TX_SINO or BL_SINO,1=EM_SINO, 2=ATTEN or NORM or SCATTER, 4=MU_MAP
{
	long pErr = 0;
	int span= m_Ary_inc==0? 3: 9;
	DWORD err = 0, lbSize=0, hbSize=0, acSize=0, imgSize=0, sc2dSize=0;
	HANDLE hFile = CreateFile ( FilenamePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	
	err = GetLastError();
	if (span_id>=0) span=span_id==0? 3: 9;
	if (err == 0)
	{
		hbSize = GetFileSize(hFile,&lbSize);
		
		switch (type)
		{
		case 0:	// tx or blank files
			acSize = 30523392; //256 * 288 * 207 * 2
			if (hbSize != acSize)
				pErr = -1;
			break;
		case 1:	// Emission file
			if (span == 3)
				acSize = 256 * 288 * 6367 * sizeof(short);
			else
				acSize = 256 * 288 * 2209 * sizeof(short);
			if (hbSize != acSize)
				pErr = -1;
			break;
		case 2:	// Normalization, 3D attenuation file and scatter
			if (span == 3)
				acSize = 256 * 288 * 6367 * sizeof(float);
			else
				acSize = 256 * 288 * 2209 * sizeof(float);
			sc2dSize = (256 * 288 * 207 + 2209) * sizeof(float);
			if (hbSize != acSize && hbSize != sc2dSize)
				pErr = -1;
			break;
		case 3:	// 2D attenuation
			acSize = 256 * 288 * 207 * sizeof(float);
			if (hbSize != acSize)
				pErr = -1;
			break;
		case 4:	// Image file 128*128 or 256*256
			imgSize = 256 * 256 * 207 * sizeof(float);
			if (hbSize != imgSize && hbSize != imgSize/4)
				pErr = -1;
			break;
		}
	}
	else
		pErr = (long)err;
	CloseHandle(hFile);	

//	CString msg = FilenamePath;
//	if (pErr!=0) msg.Format("%s size = %d != expected size = %d", FilenamePath, hbSize, acSize);
//	msg.Format("%s size = %d == expected size = %d", FilenamePath, hbSize, acSize);
//	log_message(msg);
	
	if (pErr != 0 && type==2)
	{
		CString cur_fname, filename;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath(FilenamePath,drive, dir, fname, ext);
		filename = fname;
		filename += ext;
		// Return OK if a task producing the filename is queued for attenuation or scatter correction
		for (TaskQueueIterator it = queue.begin(); it != queue.end(); it++)
		{
			ProcessingTask* cur = it->second;
			if (cur->task_ID == TASK_AT_PROCESS || cur->task_ID == TASK_SCAT_PROCESS)
			{
				_splitpath(cur->output_fname,drive, dir, fname, ext);
				cur_fname = fname; cur_fname += ext;				
				if (cur_fname == filename) return 0;
//				msg.Format ("%s != %s", filename, cur_fname);
//				log_message(msg);
			}
		}
	}
	return pErr;
}

long CReconGUIDlg::CheckTxFiles()
{
	int i;
	long valErr = 0;
	CString str1;

	str1 = ".  Please make sure the file exists and with the correct span and ring difference";
	// Validate blank file
	int bl_hist_key = TASK_BL_HISTOGRAM + m_JobNum*MAX_NUM_TASKS;
	int tx_hist_key = TASK_TX_HISTOGRAM + m_JobNum*MAX_NUM_TASKS;

	// Validate Blank sinogram if no blank histogramming task is queued
	if (queue.find(bl_hist_key) == queue.end())
	{
		for (i=0;i<m_Blank_Count;i++)
		{
			valErr = ValidateFile(m_Blank_File[i], 0);
			if (valErr != 0)
			{
				MessageBox("Error Validating " + m_Blank_File[i] + str1 ,"Tx Process Configuration Error",MB_ICONSTOP);
				return valErr;
			}
		}
	}

	// Validate TX sinogram if no TX histogramming task is queued
	if (queue.find(tx_hist_key) == queue.end())
	{
		for (i=0;i<m_Tx_Count;i++)
		{
			valErr = ValidateFile(m_Tx_File[i], 0);
			if (valErr != 0)
			{
				MessageBox("Error Validating " + m_Tx_File[i] + str1 ,"Tx Process Configuration Error",MB_ICONSTOP);
				return valErr;
			}
		}
	}
	return valErr;
}

long CReconGUIDlg::CheckAtFiles()
{
	long valErr = 0;
	CString str1;

	str1 = ".  Please make sure the file exists and with the correct span and ring difference";

	valErr = ValidateFile(m_Mu_Map_File, 4);
	if (valErr != 0)
	{
		MessageBox("Error Validating " + m_Mu_Map_File + str1 ,"Attenuation Process Configuration Error",MB_ICONSTOP);
		return valErr;
	}
	return valErr;
}

long CReconGUIDlg::CheckScattFiles()
{
	long valErr = 0;
	CString str1;
	int span_id = 1; // scatter always done in span 9

	str1 = ".  Please make sure the file exists and with the correct span and ring difference";
	// Validate Normalization File
	valErr = ValidateFile(m_Norm_9_File, 2, span_id);
	if (valErr != 0)
	{
		MessageBox("Error Validating " + m_Norm_9_File + str1 ,"Scatter Correction Process Configuration Error",MB_ICONSTOP);
		return valErr;
	}

	// Validate 3D Attenuation File if no previous task is queued
	int tx_key = TASK_TX_PROCESS + m_JobNum*MAX_NUM_TASKS;
	int atn_key = TASK_AT_PROCESS + m_JobNum*MAX_NUM_TASKS;
	if (queue.find(tx_key) == queue.end() && queue.find(atn_key) == queue.end())
	{
		if (m_3D_Atten_File.IsEmpty())
		{  // Allow cluster recon without attenuation
			if (!submit_recon_dir.IsEmpty() && m_Type == 3) valErr = 0;
			else valErr = -1;
		}
		else
		{
			valErr = ValidateFile(m_3D_Atten_File, 2, span_id);
		}
		if (valErr != 0)
		{
			MessageBox("Error Validating " + m_3D_Atten_File + str1 ,"Scatter Correction Process Configuration Error",MB_ICONSTOP);
			return valErr;
		}

		// backproject attenuation to create mu-map file if not existing
		if (_access((const char*)m_Mu_Map_File, 0) == -1)
		{
			CString umap_cmd;
			umap_cmd.Format("e7_fwd_u  --model 328 -a %s --ou %s -w 128  --mrd %d --span %d ",
				m_3D_Atten_File, m_Mu_Map_File, m_Ary_Ring[m_Ary_inc], m_Ary_Span[m_Ary_inc]);

			SubmitTask(umap_cmd, TASK_TX_PROCESS, m_Mu_Map_File);
		}
	}
	// Validate Emission File
	int em_hist_key = TASK_EM_HISTOGRAM + m_JobNum*MAX_NUM_TASKS;
	if (queue.find(em_hist_key) == queue.end())
	{
		valErr = ValidateFile(m_Em_File, 1);
		if (valErr != 0)
		{
			MessageBox("Error Validating " + m_Em_File + str1 ,"Scatter Correction Process Configuration Error",MB_ICONSTOP);
			return valErr;
		}
	}
	// Validate u map File
	/* Mu no longer required for scatter
	valErr = ValidateFile(m_Mu_Map_File, 4);
	if (valErr != 0)
	{
		MessageBox("Error Validating " + m_Mu_Map_File + str1 ,"Scatter Correction Process Configuration Error",MB_ICONSTOP);
		return valErr;
	}
	*/
	return valErr;
}


void CReconGUIDlg::OnButton9() 
{
	int nRetValue;
	long valErr;
	unsigned hist_status=0;
	NewJob();
	if (Histogram(m_Em_File, TASK_EM_HISTOGRAM)) hist_status |= HISTOGRAM_EM;
//	nRetValue = Build_Scatter_Command();
	nRetValue = Build_e7_scatter_Command(-1);
	if(m_Scatter_OK && nRetValue ==0)
	{	
		valErr = CheckScattFiles();
		if (valErr == 0)
		{	// Run Scatter Process
//			RunAndForgetProcess(m_Sc_Command,"Scatter Correction Process", &nRetValue);
			SubmitTask(m_Sc_Command, TASK_SCAT_PROCESS, m_Scatter_File);
			//MessageBox(m_Sc_Command,"This is the command to execute scatter correction",MB_OK);
			if (nRetValue != 0)
				// Error Message Only
				MessageBox("A Problem Encountered with Scatter Correction Process.  Aborting Process","Scatter Process Run Error",MB_ICONSTOP);
		}
	}
	else
		if (nRetValue ==0)
			MessageBox("Scatter Process is not configured Properly.","Scatter Process Configuration Error",MB_ICONSTOP);
	

}


void CReconGUIDlg::OnButton15() 
{
	CSSRDIFTDlg pDlg;
	pDlg.DoModal();
	
}

int CReconGUIDlg::AddQuantInfo(CString &em_sino, CString& m_Quan_Header)
{
	char txt[FILENAME_MAX];
	CString em_sino_hdr = em_sino + ".hdr";
	CHeader quant_hdr, hdr;
	int done = 0;

	if (hdr.OpenFile(em_sino_hdr) != 1) return done;
	hdr.CloseFile();
	if (!m_Quan_Header.IsEmpty() && quant_hdr.OpenFile(m_Quan_Header)>=0)
	{
		if (quant_hdr.Readfloat("calibration factor",&m_calFactor[0]) == 0)
		{
			if (m_Span == 3 && m_calFactor[0] != 1.0f)
			{
				hdr.WriteTag("calibration factor", m_calFactor[0]);
				done++;
			}
		} 
		if (quant_hdr.Readfloat("calibration factor",&m_calFactor[1]) == 0)
		{
			if (m_Span == 9 && m_calFactor[1] != 1.0f)
			{
				hdr.WriteTag("calibration factor", m_calFactor[1]);
				done++;
			}
		}
		if (done>0 && quant_hdr.Readchar("calibration unit",txt, FILENAME_MAX) == 0)
		{
			hdr.WriteTag("calibration unit", txt);
			done++;
		}
		quant_hdr.CloseFile();
		if (done>0) 
		{
			strncpy(txt, (const char*)m_Quan_Header, FILENAME_MAX-1 );
			txt[FILENAME_MAX-1] = '\0';
			hdr.WriteTag("quantification header file name", txt);
			hdr.WriteFile(em_sino_hdr);
		}
	}
	return done;
}

//
// TODO Make sure one sinogoam is configured for reconstruction (not .dyn)
//

void CReconGUIDlg::OnButton16() 
{
	int nRetValue;
	long valErr;
	CString str;
	NewJob();
	Histogram(m_Em_File, TASK_EM_HISTOGRAM);
	nRetValue = Build_Recon_Command(-1);
	if(m_Recon_OK && nRetValue ==0)
	{	
		valErr = CheckReconFiles();
		if (valErr == 0)
		{	// Run Recontruction Process
//			if (m_Type == 0)
//				sprintf(cstr,"SSR/DIFT Reconstruction Process");
//			else if (m_Type == 1)
//				sprintf(cstr, "FORE/DIFT Reconstruction Process");
//			else if (m_Type == 2)
//				sprintf(cstr ,"FORE/OSEM Reconstruction Process");
//			else if (m_Type == 3)
//				sprintf(cstr ,"OSEM3D Reconstruction Process");
//			else if (m_Type == 4)
//				sprintf(cstr ,"FORE/HOSP Reconstruction Process");
//			RunAndForgetProcess(m_Rc_Command,cstr, &nRetValue);

			if (!m_Quan_Header.IsEmpty()) AddQuantInfo(m_Em_File, m_Quan_Header);

			SubmitTask(m_Rc_Command,TASK_RECON, m_Image);
			//MessageBox(m_Sc_Command,"This is the command to execute scatter correction",MB_OK);
			if (nRetValue != 0)
				// Error Message Only
				MessageBox("A Problem Encountered with Reconstruction Process.  Aborting Process","Reconstruction Run Error",MB_ICONSTOP);
			else
				if (m_Type == 4)
				{	
					CreateHOSPFile();
//					RunProcessAndWait(m_HOSP_Cmd,cstr, &nRetValue);
					SubmitTask(m_HOSP_Cmd,TASK_HOSP_RECON, m_Image);
				}
				if ((m_Type == 3 || m_Type == 4) && submit_recon_dir.IsEmpty())
				{
					// Write Image Header
					WriteImageHeader();
					// Quantify image
					//QuantifyImage();
					/* Don't quantify images to be compatible with cluster reconstruction
					SubmitTask(m_Image, TASK_QUANTIFY_IMAGE, m_Image);
					*/
				}
			SubmitTask(tmp_files, TASK_CLEAR_TMP_FILES, NULL);
		}
		else
		{
			str.Format("%d",nRetValue);	// Error Message Only
			MessageBox("A Problem Encountered with Reconstruction Process.  Aborting Process. "+ str,"ReconstructionRun Error",MB_ICONSTOP);
		}
	}
	else
		if (nRetValue ==0)
			MessageBox("Reconstruction Process is not configured Properly.","Reconstruction Configuration Error",MB_ICONSTOP);
}

/*
 *	void CReconGUIDlg::NewJob()
 *  Initialize a new job (initialize tmp file list) and increment the job number
 */
void CReconGUIDlg::NewJob()
{
	tmp_files = "";
	m_JobNum++;
}

/*
 *	int  CReconGUIDlg::copy_recon_file(CString &dest_dir, CString &src_file, CString &target_file,
 *							   FILE *batch_fp, int overwrite_flag)
 *	Add to a batch file the copy command to transfer a source file to a destination file to the cluster file server disk.
 *  The command is not added if the destination file exists and overwrite_flag is not 0.
 */

int  CReconGUIDlg::copy_recon_file(CString &dest_dir, CString &src_file, CString &target_file,
								   FILE *batch_fp, int overwrite_flag)
{
	char drive[_MAX_DRIVE], cluster_drive[_MAX_DRIVE];
	char dir[_MAX_DIR], cluster_dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	CString msg;
	target_file = src_file;
	_splitpath((const char*)dest_dir, cluster_drive, cluster_dir, fname, ext);
//	msg.Format("Cluster: drive=%s, dir=%s, fname=%s, ext=%s",  cluster_drive, cluster_dir, fname, ext);
//	log_message(msg);
	_splitpath((const char*)src_file, drive, dir, fname, ext);
//	msg.Format("Source File: drive=%s, dir=%s, fname=%s, ext=%s",  drive, dir, fname, ext);
//	log_message(msg);
	if (stricmp(drive, cluster_drive) != 0)
	{
		// Transfer file to cluster drive and use relative path
		CString target_dir;
		// remove trailing '\\' from src directory
		int len = strlen(dir);
		if (len>0 && dir[len-1]=='\\') dir[len-1] = '\0';
		char *relative_dir = strrchr(dir,'\\');
		if (relative_dir == NULL) relative_dir = dir;
		else relative_dir++;
		target_dir.Format("%s%s%s", cluster_drive, cluster_dir,relative_dir);
		if (_access(target_dir, 0) != 0)
		{
			msg.Format("Create directory %s", (const char*)target_dir);
			log_message(msg);
			if (_mkdir(target_dir) != 0)
			{
				msg.Format("Error creating directory %s", (const char*)target_dir);
				log_message(msg,2);
				return 0;
			}
		}
		target_file.Format("%s\\%s%s", target_dir, fname, ext);
		if (_access(target_file,0) == 0)
		{
			if (overwrite_flag) msg.Format("existing %s file will be overwritten", target_file);
			else msg.Format("using existing %s file", target_file);
			log_message(msg);
			if (!overwrite_flag)
			{
				target_file.Format("%s/%s%s", relative_dir, fname, ext);
				return 1;
			}
		}
		fprintf(batch_fp,"copy %s %s\n", (const char*)src_file, target_dir);
		fprintf(batch_fp,"copy %s.hdr %s\n", (const char*)src_file, target_dir);
//		SubmitTask(cmd, TASK_COPY_RECON_FILE);
		target_file.Format("%s/%s%s", relative_dir, fname, ext);
		return 1;
	}
	else
	{
		// use absolute path
		return 0;
	}
}

int CReconGUIDlg::Build_Recon_Command(int frame_num)
{
	FILE *fp = NULL;
	CString msg;
	if 	(m_Run_rc.GetCheck() == 0)
		return 0;
	CString str,m_SSR_DIFT_nat, strn, stra, stre, stri;
  CString cmd, work_dir, log_dir;
  char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	int relative_flag=0; // 0: absolute path, 1: copy files to cluster using relative path
	// Image voxel value
	m_DeltaZ = m_DeltaXY = 1.21875f;
	CString recon_Norm_File = m_Ary_inc==0? m_Norm_3_File:m_Norm_9_File;
	if (m_Type < 3 || m_Type == 4) 
	{    // Not OSEM-3D
		//cmd = "Cal_Scatter ";
		cmd = "ReconEng ";
		// Input file emission 
		if (!m_Em_File.IsEmpty())
		{
			str.Format("-e %s ", m_Em_File);
		}
		else
		{
			log_message("CReconGUIDlg::Build_Recon_Command Input emission empty", 2);
			return 2;
		}
		cmd.operator += (str);

		// Input file normalization
		if (!recon_Norm_File.IsEmpty())
		{
			str.Format("-n %s ", recon_Norm_File);
		}
		else
		{
			log_message("CReconGUIDlg::Build_Recon_Command Input normalization empty", 2);
			return 3;
		}
		cmd.operator += (str);
		
		// Inputfile 3D atten
		if (!m_3D_Atten_File.IsEmpty())
		{
			str.Format("-a %s ", m_3D_Atten_File);
			cmd.operator += (str);	
		}

		
		if (!m_Scatter_File.IsEmpty())
		{
		
			str.Format("-o %s ",m_Scatter_File);
			cmd.operator += (str);	
		}

		if (!m_Image.IsEmpty())
		{	
			str.Format("-b %s ",m_Image);
			cmd.operator += (str);
		}
		else	
		{
			log_message("CReconGUIDlg::Build_Recon_Command output image filename empty", 2);
			return 4;
		}

		//if (m_Type < 0 || m_Type > 2)
		//	return 5;
		//else
		//{
		if (m_Type < 3)
			str.Format("-t %d ",m_Type+1);
		else
			str.Format("-t %d ",m_Type);

		cmd += str;
		if(m_Type == 2)
		{
			str.Format("-I %d -S %d ",m_Iteration, m_SubSets);
			cmd += str;
		}
		//}

		// Span and Ring Difference
		str.Format("-d %d -s %d ", m_Ary_Ring[m_Ary_inc], m_Ary_Span[m_Ary_inc] );
		cmd.operator += ( str );

		if(m_Recon_Rebin)
		{	
			str.Format("-r 1 ");
			cmd += str;
		}

		if (!m_Quan_Header.IsEmpty())
		{
			str.Format("-h %s ",m_Quan_Header);
			cmd += str;
		}
	}
	else if (m_Type == 3)  // OSEM3D
	{
		cmd = "hrrt_osem3d ";

		// Input file emission 
		if (!m_Em_File.IsEmpty())
		{
			if (m_Weight_method == 3) str.Format("-p %s ", m_Em_File);
			else str.Format("-t %s ", m_Em_File);
		}
		else
		{
			log_message("CReconGUIDlg::Build_Recon_Command Input emission empty", 2);
			return 2;
		}
		cmd.operator += (str);

		// Input file normalization
		if (!recon_Norm_File.IsEmpty())
		{
			str.Format("-n %s ", recon_Norm_File);
		}
		else
		{
			log_message("CReconGUIDlg::Build_Recon_Command Input Normalization empty", 2);
			return 3;
		}
		cmd.operator += (str);
		
		// Inputfile 3D atten
		if (!m_3D_Atten_File.IsEmpty())
		{
			str.Format("-a %s ", m_3D_Atten_File);
			cmd.operator += (str);	
		}

		
		if (!m_Scatter_File.IsEmpty())
		{
		
			str.Format("-s %s ",m_Scatter_File);
			cmd.operator += (str);	
		}

		if (!m_Image.IsEmpty())
		{	
			// use image output directory to create normfac
			get_directory(m_Image, work_dir);
			str.Format("-o %s -D %s ",m_Image, work_dir);
			cmd.operator += (str);
		}
		else
		{
			log_message("CReconGUIDlg::Build_Recon_Command output image filename empty", 2);
			return 4;
		}
		// Weighting method
		if (m_Weight_method > 0 && m_Weight_method < 4)
		{
			str.Format("-W %d ",m_Weight_method);
			cmd.operator += (str);
		}
		else if (submit_recon_dir.GetLength() == 0)
		{
			msg.Format("CReconGUIDlg::Build_Recon_Command Invalid OSEM-3D weighting method %d",
				m_Weight_method);
			log_message(msg,2);
			return 5;
		}

		// Submit random smoothing for OP-OSEM
		CString em_dir, win_delay_cmd, ch_filename, ra_smo;
		CHeader em_hdr;
		ch_filename = m_Em_File;
		ch_filename.Replace(".s", ".ch");
		if (m_Weight_method == 3)
		{
			// OP assume header contains:
			// name of true data file := true_data_file
			// prompt_filename.ch : crystal histogram
			// build randoms filename
			int duration = 0;
			ra_smo = m_Em_File;
			ra_smo.Replace(".s", "_ra_smo.s");
			// Reuse random smoothed file if existing for back-compatibility
			if (ValidateFile(ra_smo, 2) == 0)
			{
				log_message(ra_smo + " : Using existing file");
				str.Format("-d %s ", ra_smo);
			}
			else // use ch file 
			{   

				str.Format("-d %s ", ch_filename); // use ch
			} 
			cmd += str;
		}

		// Iteration and subsets
		str.Format("-I %d -S %d ",m_Iteration, m_SubSets);
		cmd += str;

		// Span and Ring Difference
		str.Format("-m %d,%d ",  m_Ary_Span[m_Ary_inc],m_Ary_Ring[m_Ary_inc] );
		cmd.operator += ( str );

		// PSF Blurring flag
		if (m_PSF) cmd += " -B 0,0,0";

		// add logging
		get_directory(m_Image, log_dir);
		log_dir += "\\log";
		if (_access(log_dir,0) != 0)
		{
			if (_mkdir(log_dir) != 0)
			{
				msg.Format("Error creating log directory %s", (const char*)log_dir);
				log_message(msg,2);
				return 0;
			}
		}
		_splitpath(m_Image, drive, dir, fname,ext);
    str.Format(" -L %s\\recon_%s.log,3", log_dir, fname); //3=log to file and console
		cmd += str;

		//  Submit multi-frame reconstruction to Cluster 
		// if ANWOSEM (m_Weight_method==2) or OP-OSEM(m_Weight_method=4)
		// and submit_recon_dir specified
		if (submit_recon_dir.GetLength() > 0 && frame_num>=0)
		{
			// Relative files if data not on file server disk
			CString em_file, norm_file, scatter_file, atten_file, image_file, recon_job_file;
			CString quant_hdr;
//			cmd.Format("recon_job_%d_copy_files.bat", m_JobNum);
			get_directory(m_Em_File, em_dir);
			if (frame_num>=0)
				cmd.Format("%s\\srecon_job_%s_%d_copy_files.bat", (const char*)em_dir, my_current_time(), frame_num);
			else cmd.Format("%s\\recon_job_%s_copy_files.bat", (const char*)em_dir, my_current_time());
			FILE *batch_fp = fopen((const char*)cmd, "w");
			if (batch_fp == NULL)
			{
				msg.Format("Error creating %s", (const char*)cmd);
				log_message(msg,2);
				m_Recon_OK = false;
				return 6;
			}
			AddTmpFile(tmp_files, cmd);
//			recon_job_file.Format("recon_job_%d.txt", m_JobNum);
			if (frame_num>=0) 
				recon_job_file.Format("%s\\recon_job_%s_%d.txt", (const char*)em_dir, my_current_time(), frame_num);
			else recon_job_file.Format("%s\\recon_job_%s.txt", (const char*)em_dir, my_current_time());
			fp = fopen((const char*)recon_job_file, "w");

			if (fp == NULL)
			{
				msg.Format("CReconGUIDlg::Build_Recon_Command can't create %s",
					(const char*)str);
				log_message(msg,2);
				m_Recon_OK = false;
				return 6;
			}
			AddTmpFile(tmp_files, recon_job_file);
//			else 
//			{
//				msg.Format("Recon job file %s created",(const char*)recon_job_file);
//				log_message(msg);
//			}

			int resol = m_Recon_Rebin? 128:256;
			relative_flag = copy_recon_file(submit_recon_dir, m_Em_File, em_file, batch_fp, 1); // overwrite when existing
			copy_recon_file(submit_recon_dir, recon_Norm_File, norm_file, batch_fp,0); // not overwrite when existing
			copy_recon_file(submit_recon_dir, m_3D_Atten_File, atten_file, batch_fp, 0); // not overwrite when existing
			if (!m_Scatter_File.IsEmpty())
				copy_recon_file(submit_recon_dir, m_Scatter_File, scatter_file, batch_fp, 1); // overwrite when existing
			if (!m_Quan_Header.IsEmpty()) 
				copy_recon_file(submit_recon_dir, m_Quan_Header, quant_hdr, batch_fp,0); // not overwrite when existing
			fprintf(fp,"#!clcrecon\n\n");
			fprintf(fp,"scanner_model            999\n");
			fprintf(fp,"span                     %d\n\n", m_Span);
			fprintf(fp,"resolution                %d\n", resol);
			image_file = m_Image;
			if (relative_flag)
			{
				int pos = em_file.ReverseFind('/');
				if (pos > 0)
				{
					CString relative_dir = em_file.Left(pos);
					// Remove directory name from image basename
					if ((pos = image_file.ReverseFind('\\')) > 0 ) 
					{
						CString basename = image_file.Right(image_file.GetLength()-pos-1);
						image_file.Format("%s/%s", relative_dir, basename);		
					}
					else
					{
						msg.Format("CReconGUIDlg::Build_Recon_Command can't locate directoy name from %s",
							(const char*)em_file);
						log_message(msg,2);
						m_Recon_OK = false;
						return 6;
					}
				}

			}
			fprintf(fp,"outputimage                %s\n\n", (const char*)image_file);
			fprintf(fp,"norm                      %s\n", (const char*)norm_file);
			if (!m_3D_Atten_File.IsEmpty())
				fprintf(fp,"atten                     %s\n", (const char*)atten_file);
			if (!m_Scatter_File.IsEmpty())
				fprintf(fp,"scatter                   %s\n\n", (const char*)scatter_file);
			if (!m_Quan_Header.IsEmpty())
				fprintf(fp,"calibration                %s\n\n", (const char*)quant_hdr);
			fprintf(fp,"iterations                %d\n", m_Iteration);
			fprintf(fp,"subsets                   %d\n", m_SubSets);
			if (m_PSF) fprintf(fp,"psf\n");				// set PSF FLAG
			switch(m_Weight_method)
			{
			default:
			case 0: // Unweighted
				fprintf(fp,"weighting                 0\n");
				fprintf(fp,"true                     %s\n", (const char*)em_file);
				break;
			case 1: // Attenuation weighted
				fprintf(fp,"weighting                 1\n");
				fprintf(fp,"true                     %s\n", (const char*)em_file);
				break;
			case 2: // Attenuation and Normalization weighted
				fprintf(fp,"weighting                 2\n");
				fprintf(fp,"true                     %s\n", (const char*)em_file);
				break;
			case 3: // OP-OSEM
				fprintf(fp,"weighting                 3\n");
				fprintf(fp,"prompt                   %s\n", (const char*)em_file);
				copy_recon_file(submit_recon_dir, ch_filename, ch_filename, batch_fp, 1);
				fprintf(fp,"delayed                    %s\n", (const char*)ch_filename);
				break;
			}
			fclose(fp);
			fprintf(batch_fp, "copy %s %s\n", (const char*)recon_job_file, (const char*)submit_recon_dir);
			fclose(batch_fp);
		}
	} 
	else 
		log_message("submit_recon_dir empty: using PC OSEM-3D reconstruction");
	OutputDebugString(cmd+"\n");
//	log_message(cmd);
	m_Rc_Command = cmd;
	
	if (m_KeepTempCheck.GetCheck()==0)
	{
		if (!m_multi_frames && relative_flag) AddTmpFile(tmp_files, m_3D_Atten_File);
		if (relative_flag) AddTmpFile(tmp_files, m_Scatter_File);
	}
	m_Recon_OK = true;
	return 0;
}

long CReconGUIDlg::CheckReconFiles()
{
	long valErr = 0;
	CString str1;

	str1 = ".  Please make sure the file exists and with the correct span and ring difference";
	// Validate Normalization File
	CString recon_Norm_File = m_Ary_inc == 0?m_Norm_3_File:m_Norm_9_File;
	valErr = ValidateFile(recon_Norm_File, 2);
	if (valErr != 0)
	{
		MessageBox("Error Validating " + recon_Norm_File + str1 ,"Reconstruction Configuration Error",MB_ICONSTOP);
		return valErr;
	}
	// Validate 3D Attenuation File if no previous task is queued
	int tx_key = TASK_TX_PROCESS + m_JobNum*MAX_NUM_TASKS;
	int atn_key = TASK_AT_PROCESS + m_JobNum*MAX_NUM_TASKS;
	if (queue.find(tx_key) == queue.end() && queue.find(atn_key) == queue.end())
	{
		if (m_3D_Atten_File.IsEmpty())
		{ // Allow cluster recon without attenuation
			if (!submit_recon_dir.IsEmpty() && m_Type == 3) valErr = 0;
			else valErr = -1;
		} 
		else
		{
			valErr = ValidateFile(m_3D_Atten_File, 2);
		}
		if (valErr != 0)
		{
			MessageBox("Error Validating " + m_3D_Atten_File + str1 ,"Reconstruction Process Configuration Error",MB_ICONSTOP);
			return valErr;
		}
	} 
//	else log_message("attenuation task pending: attenuation file validation skipped");

	// Validate Emission File if no previous task is queued
	int em_hist_key = TASK_EM_HISTOGRAM + m_JobNum*MAX_NUM_TASKS;
	if (queue.find(em_hist_key) == queue.end())
	{
		valErr = ValidateFile(m_Em_File, 1);
		if (valErr != 0)
		{
			MessageBox("Error Validating " + m_Em_File + str1 ,"Scatter Correction Process Configuration Error",MB_ICONSTOP);
			return valErr;
		}
	}
//	else log_message("Emission histogramming task pending: emission sinogram file validation skipped");


	// Validate Scatter File if no previous task is queued
	int scat_key = TASK_SCAT_PROCESS + m_JobNum*MAX_NUM_TASKS;
	if (queue.find(scat_key) == queue.end())
	{
		if (m_Scatter_File.IsEmpty())
		{ // allow cluster recon without scatter correction
			if (!submit_recon_dir.IsEmpty() && m_Type == 3) valErr = 0;
			else valErr = -1;
		}
		else
		{
			valErr = ValidateFile(m_Scatter_File, 2);
		}
		if (valErr != 0)
		{
			MessageBox("Error Validating " + m_Scatter_File + str1 ,"Scatter Correction Process Configuration Error",MB_ICONSTOP);
			return valErr;
		}
	}
//	else log_message("Scatter task pending: scatter file validation skipped");

	return valErr;

}

long CReconGUIDlg::CheckInputFiles()
{
	long valErr = 0;
	CString str1;
	int st=0,en=1,l;

	str1 = ".  Please make sure the file exists and with the correct span and ring difference";
	// Validate Normalization File
	CString recon_Norm_File = m_Ary_inc == 0?m_Norm_3_File:m_Norm_9_File;
	valErr = ValidateFile(recon_Norm_File, 2);
	if (valErr != 0)
	{
		MessageBox("Error Validating " + recon_Norm_File + str1 ,"Reconstruction Configuration Error",MB_ICONSTOP);
		return valErr;
	}

	int em_hist_key = TASK_EM_HISTOGRAM + m_JobNum*MAX_NUM_TASKS;
	if (queue.find(em_hist_key) == queue.end())
	{
		if (m_multi_frames)
		{
			st = 0;
			en = m_num_of_emiss;
		}

		for(l=st;l<en;l++)
		{
			if (m_multi_frames)
				GetEmFilename(l,&m_Em_File);	// Validate Emission File
			valErr = ValidateFile(m_Em_File, 1);
			if (valErr != 0)
			{
				MessageBox("Error Validating " + m_Em_File + str1 ,"Reconstruction Configuration Error",MB_ICONSTOP);
				return valErr;
			}
		}
	}
//	else log_message("Emission histogramming task pending: emission sinogram file validation skipped");

	return valErr;
}

void CReconGUIDlg::Get_MultiFrame_Files()
{
	int i;
	FILE *fp;
	char t[256];
	CString  strName, strPath, str;
	

//  Check pending emission histogramming task and return default filename
	int em_hist_key = TASK_EM_HISTOGRAM + m_JobNum*MAX_NUM_TASKS;
	if (queue.find(em_hist_key) != queue.end()) 
	{
		m_multi_emiss.Empty();
		return;
	}
	
	if ((fp = fopen(m_dyn_File,"r")) == NULL)
		{
			str.Format("Error opening %s", (const char*)m_dyn_File);
			log_message(str, 2);
		}
		fgets(t,256,fp);
		m_num_of_emiss = atoi(t);


	CFile fl(m_dyn_File, CFile::shareDenyNone );
	
	
	//fl.SetFilePath( szFilename);

	strName = fl.GetFileName();
	
	//strName.Replace(".s", ".i");
	
	strPath = fl.GetFilePath();

	strPath.Replace(fl.GetFileName(),NULL);

//	if(m_multi_emiss !=NULL)
//		free(m_multi_emiss);
//	m_multi_emiss = (CString *)calloc(m_num_of_emiss,sizeof(CString));
	//CString *m_multi_emiss = new m_multi_emiss[6]
	m_multi_emiss.Empty();

	for (i=0;i<m_num_of_emiss;i++)
	{
		fgets(t,256,fp);
		str.Format("$%d%s%s#%d", i,strPath,t,i);
		
		m_multi_emiss += str;
		
	}
	fclose(fp);

}

void CReconGUIDlg::GetEmFilename(int num, CString *name)
{

	int  st, en, ln=0;
	CString str, strName, strPath;
	
	if (m_multi_emiss.GetLength() > 0) 
	{
		str.Format("$%d",num);
		st = m_multi_emiss.Find(str);
		str.Format("#%d",num);
		//en = m_multi_emiss.Find(".s\n",st+1);
		en = m_multi_emiss.Find(str)-3;
		//en = m_multi_emiss.GetLength();
		/*if (num < m_num_of_emiss-1)
		{	str.Format("$%d",num+1);
			en = m_multi_emiss.Find(str);
		}
		else
			en = m_multi_emiss.GetLength()-1;
		*/
		if(num > 9 && num < 99)
			ln = 1;
		else if (num > 99 && num < 999)
			ln = 2;
		else if (num > 999 && num < 9999)
			ln = 3;
		else if (num > 9999 && num < 99999)
			ln = 4;
		str = m_multi_emiss.Mid(st+2+ln,en-st-ln);


		// Get the DYN filename and path
		CFile fl(m_dyn_File, CFile::shareDenyNone );
		// Strip the name
		strName = fl.GetFileName();
		// strip the name
		strPath = fl.GetFilePath();
		strPath.Replace(fl.GetFileName(),NULL);
		// Get sino filename
		CFile fn(str, CFile::shareDenyNone );
		strName = fn.GetFileName();
		m_QC_Path.Format("%sQC\\%s_sc_qc.ps",strPath, strName);
		m_QC_Path.Replace(".s", "_qc.ps");
	}
	else
	{  //  Assume pending emission histogramming task and return default filename
		char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
		_splitpath(m_dyn_File, drive, dir, fname,ext);
		strPath.Format("%s%s",drive,dir);
		str.Format("%s%s%s_frame%d.s", drive,dir,fname,num);
		strName.Format("%s_frame%d.s", fname, num);
		m_QC_Path.Format("%s%s\\QC\\%s_frame%d_sc_qc.ps",drive,dir,fname,num);
	}
//	m_Scatter_File.Format("%sScatter_%s", strPath,strName);
	m_Scatter_File = str;
	m_Scatter_File.Replace(".s", "_sc.s");
	*name = str;
	m_Image = str;
	if (m_Image.Find(".s")>=0) m_Image.Replace(".s", NULL);
	else if (m_Image.Find(".dyn")>=0) m_Image.Replace(".dyn", NULL);

	switch(m_Type)
	{
	case 0:
		m_Image += "_SD.i";
		break;
	case 1:
		m_Image += "_FD.i";
		break;
	case 2:
		m_Image += "_FO.i";
		break;
	case 3:
		m_Image +=  "_3D.i";
		break;
	case 4:
		m_Image += "_FH.i";
		break;
	default:
		m_Image += ".i";
		break;
	}
}

void CReconGUIDlg::AppendImageHeader(void)
{
	CString ImgHeaderName;
	int i;
	FILE * fp;
	ImgHeaderName.Format("%s.hdr",m_Image);
	fp = fopen(ImgHeaderName,"a");
	if (fp != NULL)	
	{
		for (i=0;i<m_Blank_Count;i++)
			fprintf(fp, "blank file name used := %s\n", m_Blank_File[i]);
		for (i=0;i<m_Tx_Count;i++)
			fprintf(fp, "tx file name used := %s\n", m_Tx_File[i]);

		fclose(fp);
	}

}

int CReconGUIDlg::QuantifyImage()
{
	CHeader sinoHdr, imgHdr, qnHdr;
	long avgSPB, duration,x, tsize, inc=0;
	float dtc = 1.f,  decfac = 1.f,plnEff[207],reco_method = 1.f,*img;
	int err, i, cnt=0;
	char sinoHeaderName[1024], t[1024], QuanHdr[1024];
	FILE *fp;
	int xsize, ysize,zsize;
	zsize = 207;
	m_calFactor[0] = m_calFactor[1] =1.0f;
	if(m_Recon_Rebin)
		xsize = ysize = 128; //256;
	else
		xsize = ysize = 256;
	tsize = xsize* ysize*zsize;

	img = (float *) calloc(tsize , sizeof(float));
	// Read image file to quantify
	fp = fopen(m_Image,"rb");
	if (fp != NULL)
	{	fread(img,tsize* sizeof(float),1,fp);
		fclose(fp);
	}
	else
	{	free(img);
		return 10;
	}

	sprintf(sinoHeaderName, "%s.hdr",(const char*)m_Em_File);
	err = sinoHdr.OpenFile(sinoHeaderName);
	CString imgHeaderName;
	imgHeaderName.Format("%s.hdr",(const char*)m_Image);
	int imgHdr_OK = imgHdr.OpenFile(sinoHeaderName);
	if (err >=0 && imgHdr_OK>=0)
	{
		if(sinoHdr.Readfloat("Dead time correction",&dtc) != 0)
			dtc = 1.f;
		else imgHdr.WriteTag("Applied Dead time correction", dtc);
		if(sinoHdr.Readlong("image duration",&duration) != 0)
			duration = 1;
		else imgHdr.WriteTag("Applied image correction", 1.0f/duration);
		//else
		//	duration/=1000;
		if(sinoHdr.Readfloat("branching factor",&m_BranchFactor) != 0)
			m_BranchFactor = 1.f;
		else imgHdr.WriteTag("Applied branching factor", m_BranchFactor);
		
		if(sinoHdr.Readlong("average singles per block", &avgSPB) != 0)
			avgSPB = 1;
		if(sinoHdr.Readfloat("decay correction factor",&decfac) != 0)
			decfac = 1.f;
		else imgHdr.WriteTag("Applied decay correction factor", decfac);
		sinoHdr.CloseFile();
	}
	else
	{
		// Display error
		free(img);
		return err;
	}
	
	sprintf(QuanHdr,"%s",(const char*)m_Quan_Header);
	err = qnHdr.OpenFile(QuanHdr);
	if(err >=0)
	{
		switch (m_Type)
		{
		case 3: // 3DOSEM
			if (m_Span == 3)
				qnHdr.Readfloat("OSEM3D span 3",&reco_method);
			else if (m_Span == 9)
				qnHdr.Readfloat("OSEM3D span 9",&reco_method);	
			break;
		case 4:	// HOSP
			if (m_Span == 3)
				qnHdr.Readfloat("HOSP span 3",&reco_method);
			else if (m_Span == 9)
				qnHdr.Readfloat("HOSP span 9",&reco_method);	
		break;
		}
/*
 *		Removed M.Sibomana Feb-04-2005
 		if(qnHdr.Readchar("calibration unit", t, sizeof(t)) !=0)
		{
			m_UnitType.assign( "Unknown");
			m_MulFactor = 1.f;
		}
		else
		{
			m_UnitType.assign(t);
			if(m_UnitType.find("Ci")!=-1)
			{	m_MulFactor = 1e6f;
				m_UnitType.assign("Nano-Ci");
			}
			else
			{	m_MulFactor = 1e-6f;
				m_UnitType.assign("Bq");
			}
		}
*/
		if(qnHdr.Readfloat("calibration factor span 3",&m_calFactor[0]) != 0)
			m_calFactor[0] = 1.0f;
		if(qnHdr.Readfloat("calibration factor span 9",&m_calFactor[1]) != 0)
			m_calFactor[1] = 1.0f;
		for (i=0;i<zsize;i++)
		{
			sprintf(t,"efficient factor for plane %d",i);
			if(qnHdr.Readfloat(t,&plnEff[i]) != 0)
				plnEff[i] = 1.f;
		}
		qnHdr.CloseFile();
	}
	else
	{
		free(img);
		return err;
	}
	// Calculate 
	if (reco_method <=0)
		reco_method = 1.0;
	if (dtc <= 0)
		dtc = (float)exp((double)m_LLD*(double)avgSPB);

	float calib = m_Span==3 ? m_calFactor[0] : m_calFactor[1];
	if (calib) imgHdr.WriteTag("Applied calibration factor", calib);
	strcpy(t, (const char*)m_Quan_Header);
	imgHdr.WriteTag("Applied plane efficiency factors",t );

	for (x=0;x<tsize;++x)
	{	// Approved by Klaus Wienhard and Mark Lenox
		img[x] = img[x] * reco_method * m_MulFactor *dtc*calib*plnEff[cnt]/((float)duration * m_BranchFactor);
		//img[x] = img[x] * reco_method * dtc*calib*plnEff[cnt]/(float)duration;
		inc++;
		if (inc >= xsize*ysize)//256 * 256
		{
			cnt++;
			inc = 0;
		}

	}
	// Write image
	fp = fopen(m_Image,"wb");
	if (fp != NULL)
	{	fwrite(img,tsize* sizeof(float),1,fp);
		fclose(fp);
	}
	else
	{	free(img);
		return 10;
	}
	free(img);
	
	// Update image header
	if (imgHdr_OK>=0) 
	{
		imgHdr.CloseFile();
		imgHdr.WriteFile(imgHeaderName);
	}
	return 0;
}

int CReconGUIDlg::QuantifyImage(const char *img_fname)
{
	CHeader  imgHdr, qnHdr;
	long avgSPB, duration,x, tsize, inc=0;
	float dtc = 1.f,  decfac = 1.f,plnEff[207],reco_method = 1.f,*img;
	int err, i, cnt=0;
	char imgHeaderName[1024], t[1024], QuanHdr[1024];
	FILE *fp;
	int xsize, ysize,zsize;
	zsize = 207;
	m_calFactor[0] = m_calFactor[1] = 1.0f;
	if(m_Recon_Rebin)
		xsize = ysize = 128; //256;
	else
		xsize = ysize = 256;
	tsize = xsize* ysize*zsize;

	img = (float *) calloc(tsize , sizeof(float));
	// Read image file to quantify
	fp = fopen(img_fname,"rb");
	if (fp != NULL)
	{	fread(img,tsize* sizeof(float),1,fp);
		fclose(fp);
	}
	else
	{	free(img);
		return 10;
	}

	sprintf(imgHeaderName, "%s.hdr",img_fname);
	err = imgHdr.OpenFile(imgHeaderName);
	if (err >=0)
	{
		if(imgHdr.Readfloat("Dead time correction",&dtc) != 0)
			dtc = 1.f;
		if(imgHdr.Readlong("image duration",&duration) != 0)
			duration = 1;
		//else
		//	duration/=1000;
		if(imgHdr.Readfloat("branching factor",&m_BranchFactor) != 0)
			m_BranchFactor = 1.f;
		
		if(imgHdr.Readlong("average singles per block", &avgSPB) != 0)
			avgSPB = 1;
		if(imgHdr.Readfloat("decay correction factor",&decfac) != 0)
			decfac = 1.f;

		imgHdr.CloseFile();
	}
	else
	{
		// Display error
		free(img);
		return err;
	}
	
	sprintf(QuanHdr,"%s",m_Quan_Header);
	err = qnHdr.OpenFile(QuanHdr);
	if(err >=0)
	{
		switch (m_Type)
		{
		case 3: // 3DOSEM
			if (m_Span == 3)
				qnHdr.Readfloat("OSEM3D span 3",&reco_method);
			else if (m_Span == 9)
				qnHdr.Readfloat("OSEM3D span 9",&reco_method);	
			break;
		case 4:	// HOSP
			if (m_Span == 3)
				qnHdr.Readfloat("HOSP span 3",&reco_method);
			else if (m_Span == 9)
				qnHdr.Readfloat("HOSP span 9",&reco_method);	
		break;
		}
		if(qnHdr.Readchar("calibration unit", t, sizeof(t)) !=0)
		{
			m_UnitType.assign( "Unknown");
			m_MulFactor = 1.f;
		}
		else
		{
			m_UnitType.assign(t);
			if(m_UnitType.find("Ci")!=-1)
			{	m_MulFactor = 1e6f;
				m_UnitType.assign("Nano-Ci");
			}
			else
			{	m_MulFactor = 1e-6f;
				m_UnitType.assign("Bq");
			}
		}
		if(qnHdr.Readfloat("calibration factor span 3",&m_calFactor[0]) != 0)
			m_calFactor[0] = 1.f;
		if(qnHdr.Readfloat("calibration factor span 9",&m_calFactor[1]) != 0)
			m_calFactor[1] = 1.f;
		for (i=0;i<zsize;i++)
		{
			sprintf(t,"efficient factor for plane %d",i);
			if(qnHdr.Readfloat(t,&plnEff[i]) != 0)
				plnEff[i] = 1.f;
		}
		qnHdr.CloseFile();
	}
	else
	{
		free(img);
		return err;
	}
	// Calculate 
	if (reco_method <=0)
		reco_method = 1.0;
	if (dtc <= 0)
		dtc = (float)exp((double)m_LLD*(double)avgSPB);

	for (x=0;x<tsize;++x)
	{	// Approved by Klaus Wienhard and Mark Lenox
		float calib = m_Span==3 ? m_calFactor[0] : m_calFactor[1];
		img[x] = img[x] * reco_method * m_MulFactor *dtc*calib*plnEff[cnt]/((float)duration * m_BranchFactor);
		//img[x] = img[x] * reco_method * dtc*calib*plnEff[cnt]/(float)duration;
		inc++;
		if (inc >= xsize*ysize)//256 * 256
		{
			cnt++;
			inc = 0;
		}

	}
	// Write image header
	fp = fopen(m_Image,"wb");
	if (fp != NULL)
	{	fwrite(img,tsize* sizeof(float),1,fp);
		fclose(fp);
	}
	else
	{	free(img);
		return 10;
	}
	free(img);
	return 0;
}

void CReconGUIDlg::WriteImageHeader(void)
{
	char sinoHeaderName[1024], t[1024], ImgHeaderName[1024];
	FILE  *in_file, *out_file;
	std::string tag;
	int xsize, ysize,zsize;
	zsize = 207;
	if(m_Recon_Rebin)
		xsize = ysize = 128;
	else
		xsize = ysize = 256;

	CString recon_Norm_File = m_Ary_inc==0? m_Norm_3_File:m_Norm_9_File;
	printf("Writing image header file \n" );
	// write image header
	sprintf(sinoHeaderName, "%s.hdr",m_Em_File);
	sprintf(ImgHeaderName, "%s.hdr",m_Image);
	in_file = fopen(sinoHeaderName,"r");
	out_file = fopen(ImgHeaderName,"w");
	
	if ((in_file != NULL) &&  (out_file != NULL))
	{
		while (fgets(t,1024,in_file) != NULL )
		{		
			tag.assign(t);
			//  Print the data file name
			if(tag.find("!originating system")!=-1 )
				fprintf( out_file, "!originating system := HRRT\n");
			else if(tag.find("name of data file")!=-1 )
				fprintf( out_file, "name of data file := %s\n", m_Image);
			else if(tag.find("scan data type description [1]")!=-1 )
				fprintf( out_file, "scan data type description [1] := corrected trues\n");
			else if(tag.find("matrix size [1]")!=-1 )
				fprintf( out_file, "matrix size [1] := %d\n",xsize);
			else if(tag.find("matrix size [2]")!=-1 )
				fprintf( out_file, "matrix size [2] := %d\n",ysize);
			else if(tag.find("matrix size [3]")!=-1 )
				fprintf( out_file, "matrix size [3] := %d\n",zsize);
			else if(tag.find("data format")!=-1 )
				fprintf( out_file, "data format := image\n");
			else if(tag.find("number format")!=-1 )
				fprintf( out_file, "number format := float\n");
			else if(tag.find("number of bytes per pixel")!=-1 )
				fprintf( out_file, "number of bytes per pixel := 4\n");
			else if(tag.find("scale factor (mm/pixel) [1]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [1] := %f\n", m_DeltaXY);
			else if(tag.find("scale factor [2]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [2] := %f\n", m_DeltaXY);	
			else if(tag.find("scale factor (mm/pixel) [3]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [3] := %f\n", m_DeltaZ);

			else if(tag.find("scaling factor (mm/pixel) [1]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [1] := %f\n", m_DeltaXY);
			else if(tag.find("scaling factor [2]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [2] := %f\n", m_DeltaXY);	
			else if(tag.find("scaling factor (mm/pixel) [3]")!=-1 )
				fprintf( out_file, "scaling factor (mm/pixel) [3] := %f\n", m_DeltaZ);				
			else
				fprintf( out_file, "%s", tag.c_str());
			
		}
	}
	else
	{
		if (out_file != NULL)
		{

			fprintf( out_file, "!INTERFILE\n");
			fprintf( out_file, "number of dimensions := 3\n");
			fprintf( out_file, "name of data file := %s\n", m_Image);
			fprintf( out_file, "matrix size [1] := %d\n",xsize);		
			fprintf( out_file, "matrix size [2] := %d\n",ysize);		
			fprintf( out_file, "matrix size [3] := %d\n",zsize);
			fprintf( out_file, "scaling factor (mm/pixel) [1] := %f\n", m_DeltaXY);		
			fprintf( out_file, "scaling factor (mm/pixel) [2] := %f\n", m_DeltaXY);		
			fprintf( out_file, "scaling factor (mm/pixel) [3] := %f\n", m_DeltaZ);

			fprintf( out_file, "data format := image\n");
			fprintf( out_file, "number format := float\n");
			fprintf( out_file, "number of bytes per pixel := 4\n");

			fprintf( out_file, "quantification header file name used := %s\n", m_Quan_Header);
			fprintf( out_file, "calibration unit := %s\n", m_UnitType.c_str());
			if (m_BranchFactor !=1.f)
				fprintf(out_file,"branching factor corrected := true\n");
			else
				fprintf(out_file,"branching factor corrected := false\n");


		}
		
	}
	if (out_file != NULL)
	{
		fprintf( out_file, "norm file used := %s\n", recon_Norm_File);
		fprintf( out_file, "atten file used := %s\n", m_3D_Atten_File);
		fprintf( out_file, "emission file used := %s\n", m_Em_File);
		fprintf( out_file, "scatter file used := %s\n", m_Scatter_File);
		//fprintf( out_file, "quantification header file name used := %s\n", m_Quan_Header);
		if (!m_Quan_Header.IsEmpty())
		{
			fprintf( out_file, "quantification header file name used := %s\n", m_Quan_Header);
			fprintf( out_file, "calibration unit := %s\n", m_UnitType.c_str());
			if (m_calFactor[0] != 1.0f) fprintf( out_file, "calibration factor span 3 := %f\n", m_calFactor[0]);
			if (m_calFactor[1] != 1.0f) fprintf( out_file, "calibration factor span 9 := %f\n", m_calFactor[1]);
			if (m_BranchFactor !=1.f)
				fprintf(out_file,"branching factor corrected := true\n");
			else
				fprintf(out_file,"branching factor corrected := false\n");
		}

		switch (m_Type)
		{
		case 0:
			fprintf( out_file, "method of reconstruction used := SSR/DIFT\n");
			break;	
		case 1:
			fprintf( out_file, "method of reconstruction used := FORE/DIFT\n");
			break;
		case 2:
			fprintf( out_file, "method of reconstruction used := FORE/OSEM\n");
			break;
		case 3:
			if (m_Weight_method != 3)
				fprintf( out_file, "method of reconstruction used := OSEM3D\n");
			else 
				fprintf( out_file, "method of reconstruction used := OP-OSEM3D\n");

			fprintf( out_file, "number of iterations := %d\n", m_Iteration);
			fprintf( out_file, "number of subsets := %d\n", m_SubSets);
			break;
		case 4:
			fprintf( out_file, "method of reconstruction used := FORE/HOSP\n");
			break;
		}
		
		fclose(out_file);
	}
	if (in_file != NULL)
		fclose(in_file);

}


int CReconGUIDlg::CreateHOSPFile(void)
{
	std::string fore_file, emission_filename, TxtFile;
	int num;
	FILE *fp;
	emission_filename.assign(m_Em_File);

	num = emission_filename.length();
	fore_file = emission_filename;
	TxtFile = emission_filename;
	fore_file.replace(num-2, 6, "_Fore.s");
	TxtFile.replace(num-2, 8, "_Fore.txt");
	
	fp = fopen(TxtFile.c_str(),"w");
	if (fp != NULL)
	{
		fprintf(fp,"SinogramFile_flat\t\t%s\n",fore_file.c_str());
		fprintf(fp,"ImageFile_flat\t\t%s\n",m_Image);
		fprintf(fp,"Nplanes\t\t207\n");
		fprintf(fp,"Nelements\t\t256\n");
		fprintf(fp,"Nviews\t\t256\n");
		fprintf(fp,"SkipHeaderBytes\t\t0\n");
		fprintf(fp,"StartPlane\t\t%d\n",m_StartPlane);
		fprintf(fp,"EndPlane\t\t%d\n",m_EndPlane);
		fprintf(fp,"FillPlanesZero\t\t1\n");
		fprintf(fp,"ThetaOffset\t\t0.0\n");
		fprintf(fp,"IterMax\t\t%d\n", m_Iteration);
		fprintf(fp,"Overrelaxation_z\t\t24.0\n");
		fprintf(fp,"EffectiveIterationCount\t\t-1\n");
		fprintf(fp,"SubbinningFactor_q\t\t8\n");
		fclose(fp);

	}
	else
		return 1;
	m_HOSP_Cmd.Format("HOSP %s",TxtFile.c_str()); 
	return 0;
}
// Tx Process enable check box
void CReconGUIDlg::OnBnClickedCheck2()
{
	CWnd *pwnd;

	if(m_Run_Tx.GetCheck() == 0)
	{
		m_Tx_OK = true;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON2);
		pwnd->EnableWindow(FALSE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON3);
		pwnd->EnableWindow(FALSE);
	}
	else
	{
		m_Tx_OK = false;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON2);
		pwnd->EnableWindow(TRUE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON3);
		pwnd->EnableWindow(TRUE);
	}


	Invalidate(TRUE);

}

void CReconGUIDlg::OnBnClickedCheck3()
{
	CWnd *pwnd;
	if(m_Run_at.GetCheck() == 0)
	{
		m_Aten_OK = true;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON4);
		pwnd->EnableWindow(FALSE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON5);
		pwnd->EnableWindow(FALSE);
	}
	else
	{
		m_Aten_OK = false;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON4);
		pwnd->EnableWindow(TRUE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON5);
		pwnd->EnableWindow(TRUE);
	}

	Invalidate(TRUE);
}

void CReconGUIDlg::OnBnClickedCheck4()
{
	CWnd *pwnd;
	if(m_Run_sc.GetCheck() == 0)
	{
		m_Scatter_OK = true;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON8);
		pwnd->EnableWindow(FALSE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON9);
		pwnd->EnableWindow(FALSE);
	}
	else
	{
		m_Scatter_OK = false;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON8);
		pwnd->EnableWindow(TRUE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON9);
		pwnd->EnableWindow(TRUE);
	}
	Invalidate(TRUE);
}

void CReconGUIDlg::OnBnClickedCheck5()
{
	CWnd *pwnd;
	if(m_Run_rc.GetCheck() == 0)
	{
		m_Recon_OK = true;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON15);
		pwnd->EnableWindow(FALSE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON16);
		pwnd->EnableWindow(FALSE);
	}
	else
	{
		m_Recon_OK = false;
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON15);
		pwnd->EnableWindow(TRUE);
		pwnd = (CWnd*)GetDlgItem(IDC_BUTTON16);
		pwnd->EnableWindow(TRUE);
	}

	Invalidate(TRUE);
}

bool CReconGUIDlg::CheckForWhiteSpace(CString &filename, int single_dot_flag)
{
	CString str;
	if (filename.Find(' ')==-1)
	{
		int pos = filename.GetLength()-1;
		int ext_found=0;
		if (single_dot_flag)
		{
			while(pos>0 && filename[pos] != '\\')
			{
				if (filename[pos] == '.')
				{
					if (ext_found) filename.SetAt(pos, '_');
					else ext_found = 1;
				}
				pos--;
			}
		}
		return true;
	}
	else
	{
		str.Format("Cannot have white spaces in the path.  Filename %s.",filename);
		MessageBox(str,"White Space Check",MB_OK);
		return false;
	}

}

int CReconGUIDlg::num_frames(CHeader &hdr, int *frame_duration, int max_frames)
{
	int nframes=0, count=0, multi_spec[2];
	char framedef[256];
	strcpy(framedef,"");
	frame_duration[0] = 0;
	hdr.Readchar("Frame definition",framedef, sizeof(framedef));
	if (strlen(framedef))
	{
//		cout << "Frame definition: [" << framedef << "]" << endl;
		char *cptr = strtok(framedef,", ");
		while (cptr)
		{
			if (sscanf(cptr,"%d*%d", &multi_spec[0], &multi_spec[1]) == 2)
			{
				if (multi_spec[0]>0) 
				{
//					cout << multi_spec[0] << "sec frame  repeated " << multi_spec[1] << endl;
					for (count=0; count<multi_spec[1] && nframes<256; count++)
					  if (nframes<max_frames) frame_duration[nframes++] = multi_spec[0];
				}
				else 
				{
//					cout << hdr_fname <<": Invalid frame specification \"" << cptr << "\"" << endl;
					break;
				}
			}
			else
			{
				if (sscanf(cptr,"%d", &multi_spec[0]) == 1 && multi_spec[0]>0)
					frame_duration[nframes++] = multi_spec[0];
				else 
				{
				//	cout << hdr_fname <<": Invalid frame specification \"" << cptr << "\"" << endl;
					break;
				}
			}
			cptr = strtok(0,", ");
		}
	}
	if (nframes==0)
	{
		hdr.Readint("image duration", frame_duration);
		nframes++;
	}

	return nframes;
}

/*
 *	void CReconGUIDlg::HideAndShow(int dlg_item1, int dlg_item2, int revert_flag)
 *  Retrieve windows with specified IDs, show the first and hide the second or vice-versa.  
 */
void CReconGUIDlg::HideAndShow(int dlg_item1, int dlg_item2, int revert_flag)
{
	CWnd *w1 = (CWnd*)GetDlgItem(dlg_item1);
	CWnd *w2 = (CWnd*)GetDlgItem(dlg_item2);
	if (w1!=NULL && w2!=NULL)
	{
		w1->ShowWindow(revert_flag? SW_SHOW:SW_HIDE);
		w2->ShowWindow(revert_flag? SW_HIDE:SW_SHOW);
	}
}

/*
 *	void CReconGUIDlg::ShowHistogramming(ProcessingTask& task, int done_flag)
 *  Display the histograming sign in front of the task requiring histogramming (TX or
 *  Scatter process) 
 */
void CReconGUIDlg::ShowHistogramming(ProcessingTask& task, int done_flag)
{
	// Revert when done
	if (task.task_ID == TASK_BL_HISTOGRAM || task.task_ID == TASK_TX_HISTOGRAM )
	{ // Blank or TX histogramming
		HideAndShow(IDC_EDIT_TX, IDC_HISTOGRAMMING1, done_flag);
	}
	else if (task.task_ID == TASK_EM_HISTOGRAM)
	{ // Emission histogramming
		if (m_Scatter_OK)
			HideAndShow(IDC_EDIT_SC, IDC_HISTOGRAMMING2, done_flag);
		else HideAndShow(IDC_EDIT_RECON, IDC_HISTOGRAMMING3, done_flag);
	}
}

/*
 *	void CReconGUIDlg::post_process(ProcessingTask& task)
 * Create postscript for scatter QC
 *  Check the output file
 */
void CReconGUIDlg::post_process(ProcessingTask& task)
{
	CString lmsg, hdr_fname;
	char cw_dir[FILENAME_MAX];
	CString qc_dir, normfac;
	CString cmd_line;
	switch(task.task_ID)
	{
	case TASK_SCAT_PROCESS:
		if (ValidateFile(task.output_fname, 2)!= 0) 
		{
			lmsg.Format("invalid scatter process output file %s: Job aborted",
				(const char*)task.output_fname);
			log_message(lmsg);
			RemoveJob(task.job_ID);
		}
		// remove .h33 header
		hdr_fname = task.output_fname;
		if (hdr_fname.Replace(".s", ".h33") == 1) unlink(hdr_fname);
		// Create postcript QC file
		if (!task.qc_fname.IsEmpty())
		{
			get_directory(task.qc_fname, qc_dir);
			char *back_dir = _getcwd(cw_dir, FILENAME_MAX);
			if (_chdir(qc_dir)==0)
			{
				lmsg.Format("current dir: %s", (const char*)qc_dir);
				log_message(lmsg);
				if (!m_gnuplot_Cmd.IsEmpty()) 
				{
					cmd_line = m_gnuplot_Cmd;
					cmd_line += " scatter_qc_00.plt";
					log_message(cmd_line);
					system(cmd_line);
					// remove old file if existing
					if (_access(task.qc_fname,0) == 0) _unlink(task.qc_fname);					
					rename("scatter_qc_00.ps",task.qc_fname);
					lmsg = "rename scatter_qc_00.ps ";
					lmsg += task.qc_fname;
					log_message(lmsg);
				}
			}
			else
			{
				lmsg.Format("unable to change directory to %s", (const char*)qc_dir);
				log_message(lmsg,2);
			}
			if (back_dir != NULL) 
			{
				if (_chdir(cw_dir) == 0)
				{
					lmsg.Format("current dir: %s", cw_dir);
					log_message(lmsg);
				}
			}
		}
		break;
	case TASK_AT_PROCESS:
		if (ValidateFile(task.output_fname, 2) != 0)
		{
			lmsg.Format("invalid attenuation process output file %s: Job aborted",
				(const char*)task.output_fname);
			log_message(lmsg);
			RemoveJob(task.job_ID);
		}
		// remove .h33 header
		hdr_fname = task.output_fname;
		if (hdr_fname.Replace(".a", ".h33") ==1 ) unlink(hdr_fname);
		break;
	case TASK_TX_PROCESS:
	case TASK_TX_TV3D:
		if (ValidateFile(task.output_fname, 4) != 0)
		{
			lmsg.Format("invalid mu-map process output file %s: Job aborted",
				(const char*)task.output_fname);
			log_message(lmsg);
			RemoveJob(task.job_ID);
		}

		// remove .h33 header
		hdr_fname = task.output_fname;
		if (hdr_fname.Replace(".i", ".h33") == 1) unlink(hdr_fname);
		
		break;

	case TASK_RECON:
		// Clean normfac after reconstruction
		get_directory(task.output_fname, normfac);
		normfac += "\\normfac.i";
		if (_access(normfac,0) == 0) 
		{
			if (_unlink(normfac) == 0) log_message(normfac + " deleted");
			else log_message(normfac + " delete failed", 2);
		}
		break;

	}
	ShowHistogramming(task,1); // Show histogramming done signal
}

/*
 *	afx_msg LRESULT CReconGUIDlg::OnProcessingMessage(WPARAM msg, LPARAM param)
 *  Process a message from the Processing thread. Check the output file if the message is
 *  a task completion.
 */
afx_msg LRESULT CReconGUIDlg::OnProcessingMessage(WPARAM msg, LPARAM param)
{
	CString lmsg;
	int key=0;
	ProcessingTask *ptask = (ProcessingTask*)param;
	switch(msg) {
	case WM_START_TASK_OK:
		lmsg.Format("Start OK: %s", (const char*)ptask->cmd_line);
		ptask->status = ProcessingTask::Active;
		ShowHistogramming(*ptask);
		RefreshTaskList();
		log_message(lmsg);
		break;
	case WM_START_TASK_FAIL:
		lmsg.Format("Start FAIL: %s", (const char*)ptask->cmd_line);
		log_message(lmsg);
		break;
	case WM_TASK_DONE:
		if (ptask->task_ID == TASK_CLEAR_TMP_FILES)
			lmsg.Format("Task Done: Clear TMP Files %s", (const char*)ptask->cmd_line);
		else if (ptask->task_ID == TASK_COPY_RECON_FILE) 
			lmsg = "Task Done: Copy Reconstruction files";
		else
			lmsg.Format("Task Done: %s", (const char*)ptask->cmd_line);
		log_message(lmsg);

//		lmsg.Format("Remove task %d, new queue size = %d", key, queue.size());
//		log_message(lmsg);
		key = ptask->task_ID + ptask->job_ID*MAX_NUM_TASKS;
		queue.erase(key);
		post_process(*ptask);
		delete ptask;
		if (queue.size()> 0)
		{	// Promote histogramming first
			TaskQueueIterator first = queue.begin();
			TaskQueueIterator hist = queue.begin();
			while (hist !=  queue.end() && (hist->second->task_ID != TASK_TX_HISTOGRAM && 
				hist->second->task_ID != TASK_EM_HISTOGRAM)) hist++;
			if (hist != queue.end()) 
				processing_thread->PostThreadMessage(WM_START_TASK, 1, (LPARAM)(hist->second));
			else
				processing_thread->PostThreadMessage(WM_START_TASK, 1, (LPARAM)(first->second));
		}
		RefreshTaskList();
		break;
	default:
		log_message("unknown processing thread message");
		return 0;
	}
	return 1;
}

/*
 *	void CReconGUIDlg::RefreshTaskList()
 *  Clear the current task lisk display and update it with the current queue content
 */
void CReconGUIDlg::RefreshTaskList()
{
	CString item_txt;
	char job_txt[4], task_txt[4];
	int h_extent = 800;
	m_ListTask.ResetContent();
	m_RemoveJob.EnableWindow(FALSE);
	m_TopJob.EnableWindow(FALSE);
	m_RemoveAll.EnableWindow(FALSE);
	for (TaskQueueIterator it = queue.begin(); it != queue.end(); it++)
	{
		format_NN(it->second->job_ID, job_txt);
		format_NN(it->second->task_ID, task_txt);
		item_txt.Format("%s:%s", job_txt, task_txt);
		switch(it->second->status) {
		case ProcessingTask::Active:
			item_txt += "   Active    ";
			break;
		case ProcessingTask::Waiting:
			item_txt += "   Waiting   ";
			break;
		case ProcessingTask::Submitted:
			item_txt += "   Submitted ";
			break;
		default:
			item_txt += "   Done     ";
			break;
		}
		item_txt += it->second->name;
		m_ListTask.AddString(item_txt);
	}
	m_ListTask.SetHorizontalExtent(h_extent);
	if (m_ListTask.GetCount()>0) m_RemoveAll.EnableWindow(TRUE);
}

/*
 *	int CReconGUIDlg::SubmitTask(CString &cmd, TaskType type, const char *output_fname)
 *  Makes a task with the provided arguments and add the task to the queue and
 *  Automatically submit the task if the queue was empty.
 *  If job number is invalid and the task is not created and the method return is 0;
 *  otherwise it return 1
 */
int CReconGUIDlg::SubmitTask(CString &cmd, TaskType type, const char *output_fname)
{
	ProcessingTask *ptask = new ProcessingTask;
	if (m_JobNum<0) return 0;
	int key = type + m_JobNum*MAX_NUM_TASKS;
	ptask->job_ID = m_JobNum;
	ptask->task_ID = type;
	ptask->cmd_line = cmd;
	ptask->SetName(output_fname);
	if (type == TASK_SCAT_PROCESS && !m_QC_Path.IsEmpty())
		ptask->qc_fname = m_QC_Path;
	if (type == TASK_CLEAR_TMP_FILES) 
	{
		CString msg = "Clear TMP Files ";
		msg += cmd;
		log_message(msg);
	}
	else if (type == TASK_CLEAR_TMP_FILES) 
	{
		log_message("Copy Reconstruction files");
	}
	else log_message(cmd);
	
	if (queue.size() == 0)
	{
		ptask->status = ProcessingTask::Submitted;
		processing_thread->PostThreadMessage(WM_START_TASK, 1, (LPARAM)(ptask));
	}
	else ptask->status = ProcessingTask::Waiting;
	queue.insert(std::make_pair(key, ptask));
	RefreshTaskList();
	if (type == TASK_TX_PROCESS && m_Tx_Segment && m_TX_TV3DReg)
	{
		CString tv3d_cmd;
		tv3d_cmd.Format("TX_TV3DReg -i %s", output_fname);
		SubmitTask(tv3d_cmd, TASK_TX_TV3D, output_fname) ;
	}
	return 1;
}

/*
 *	int CReconGUIDlg::make_sino_filename(CString &lm_fname, CString &sino_fname)
 *  Makes the sinogram filename from the listmode filename and the current sinogram directory setting.
 *  The filename is not filled if the sinogram directory is no set and the method returns 0,
 *  otherwise it returns 1
 */
int CReconGUIDlg::make_sino_filename(CString &lm_fname, CString &sino_fname)
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	if (sinogram_data_dir.GetLength()==0) 
	{
		log_message("\"Sinogram Data Dir\" not defined in configuration file", 2);
		return 0;
	}
	_splitpath((const char*)lm_fname, drive, dir, fname, ext);
	// remove trailing '\\' from src directory
	if (stricmp(ext,".l32")==0 || stricmp(ext, ".l64")==0)
	{
		int len = strlen(dir);
		if (len>0 && dir[len-1]=='\\') dir[len-1] = '\0';
		char *relative_dir = strrchr(dir,'\\');
		if (relative_dir != NULL)
		{
			relative_dir++;
			// Beta 1 transmission files were created in Transmission directory
			// Ignore the directory if this is the case
			if (stricmp(relative_dir,"transmission") == 0)
			{
				// remove Transmission part
				relative_dir--;
				*relative_dir = '\0';
				if ((relative_dir = strrchr(dir,'\\')) != NULL)
				{
					relative_dir++;
					sino_fname.Format("%s\\%s\\%s.s",
						(const char*)sinogram_data_dir, relative_dir, fname);
					return 1;
				}
			}
			else
			{
				sino_fname.Format("%s\\%s\\%s.s",
					(const char*)sinogram_data_dir, relative_dir, fname);
				return 1;
			}
		}
	}
	return 0;
}

/*
 * int CReconGUIDlg::Histogram(CString &file, TaskType  task_type)
 * Submit the Histogramming task with the file and fill back the filename with the sinogram filename.
 * Return 0 if error and 1 if success
 */
int CReconGUIDlg::Histogram(CString &file, TaskType  task_type)
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	CString msg;

	_splitpath((const char*)file, drive, dir, fname, ext);
	if (stricmp(ext,".l32")==0 || stricmp(ext, ".l64")==0)
	{
		CString out_file;
		if (make_sino_filename(file, out_file))
		{
			char txt[FILENAME_MAX];
			CString hdr_fname = file + ".hdr";
			CString out_dir;
			CString out_hdr_fname = out_file + ".hdr";
			CHeader hdr, quant_hdr;
			CString cmd;
			get_directory(out_file, out_dir);
			// create out directory if not existing
			if (_access(out_dir,0) !=0)
			{
				if (mkdir((const char*)out_dir)!= 0) 
				{
					msg.Format("Can't create %s", (const char*)out_dir);
					log_message(msg, 2);
					return 0;
				}
			}
			if (task_type == TASK_EM_HISTOGRAM)
			{ // emission
				if (m_start_countrate>0) 
        {
          cmd.Format("lmhistogram_u %s -o %s -span %d -start %d", (const char*)file,
					(const char*)out_file, m_Span, m_start_countrate);
        }
        else 
        {
          cmd.Format("lmhistogram_u %s -o %s -span %d", (const char*)file,
					(const char*)out_file, m_Span);
        }
				if (m_Type == 3 && m_Weight_method == 3)
				{// OP-OSEM3D
					cmd += " -PR";
				}
				
			  // set out_dir as current directory to create local files here
				_chdir(out_dir);
			}
			else
			{ // Transmission
				// Create a temporary sinogram header to get the axial velocity
				if ((hdr.OpenFile(hdr_fname)) >= 0)
				{
					hdr.WriteTag("!name of data file", (const char*)out_file);
					hdr.WriteTag("data format", "sinogram");
					hdr.WriteTag("axial compression","21");
					hdr.WriteTag("maximum ring difference","10");
					hdr.WriteTag("number of dimensions","3");
					hdr.WriteTag("matrix size [1]", "256");
					hdr.WriteTag("matrix size [2]", "288");
					hdr.WriteTag("matrix size [3]", "207");
					hdr.WriteTag("number format", "signed integer");
					hdr.WriteTag("number of bytes per pixel", "2");
					hdr.WriteTag("scaling factor (mm/pixel) [1]", "1.218750");
					hdr.WriteTag("scaling factor [2]", "1");
					hdr.WriteTag("scaling factor (mm/pixel) [3]", "1.218750");
					hdr.WriteTag("image relative start time", "0");
					hdr.WriteFile(out_hdr_fname);
					hdr.CloseFile();
				}
				cmd.Format("lmhistogram_u %s -o %s -notimetag", (const char*)file,(const char*)out_file);
			}
			m_num_of_emiss = 1;
			m_multi_frames = false;

			if (hdr.OpenFile((const char*)hdr_fname)>=0)
			{
				if (task_type == TASK_EM_HISTOGRAM) 
				{
					m_num_of_emiss = num_frames(hdr, frame_duration, MAX_NUM_FRAMES);
					// Create the output sinogram header: the duration is used for windelays 
					if (m_Weight_method == 3)
					{   //OP-OSEM
						hdr.WriteTag("sinogram data type", "prompt");
						CString true_data_file = out_file;
						true_data_file.Replace(".s", ".tr.s");
						// Write tag has no method for const char, use txt
						strcpy(txt, (const char*)true_data_file);
						hdr.WriteTag("name of true data file", txt);
						hdr.WriteFile((const char*)out_hdr_fname);
					}
					hdr.CloseFile();
				} 
			}
			cmd += " -l ";
			cmd += histogram_log_file;
			SubmitTask(cmd, task_type, out_file);
			if (m_num_of_emiss>1)
			{
				out_file.Replace(".s", ".dyn");
				m_multi_frames = true;
				m_dyn_File = out_file;
			}
			file = out_file;
			return 1;
		}
	}
	return 0;
}

/*
 * int CReconGUIDlg::Histogram()
 * Submit the Histogramming tasks for the blank, TX and emission if listmode
 * and fill back the filename with the sinogram filename.
 * Return the status indicating wihch tasks have been submitted.
 */
 unsigned CReconGUIDlg::Histogram()
{
	unsigned status = 0;
	if (Histogram(m_Blank_File[0], TASK_BL_HISTOGRAM)) status |= HISTOGRAM_BL;
	if (Histogram(m_Tx_File[0], TASK_TX_HISTOGRAM)) status |= HISTOGRAM_TX;
	if (Histogram(m_Em_File, TASK_EM_HISTOGRAM)) 
	{
		status |= HISTOGRAM_EM;
		if (m_Auto_Assign.GetCheck() != 0)
		{ // update output filename
			int len = m_Em_File.GetLength();		
			if (m_Type == 0) 
				m_Image.Format("%s_SD.i",m_Em_File.Mid(0,len-2));
				
			else if (m_Type == 1)
				m_Image.Format("%s_FD.i",m_Em_File.Mid(0,len-2));
			else if (m_Type == 2)
				m_Image.Format("%s_FO.i",m_Em_File.Mid(0,len-2));
			else if (m_Type == 4)
				m_Image.Format("%s_3D.i",m_Em_File.Mid(0,len-2));
			else if (m_Type == 5)
				m_Image.Format("%s_FH.i",m_Em_File.Mid(0,len-2));
		}
	}
	return status;
}

void CReconGUIDlg::OnBnClickedButton1()
{
	int nRetValue=1, st=0, en=1, l, jn=0, memalloc=0;
	long valErr;//,sts;
	CString str;
	char **cd;
//	HRESULT hr;


	unsigned hist_status = Histogram();
	if (m_multi_frames)
	{
		Get_MultiFrame_Files();
		st = 0;
		en = m_num_of_emiss;
	}
	// See how much memory to allocate
	if 	(m_Run_rc.GetCheck() != 0)
	{	memalloc += en;
		if (m_Type == 3 || m_Type == 4)
		memalloc += 2 * en;
	}
	if 	(m_Run_sc.GetCheck() != 0)
		memalloc += en;

	memalloc += 10;

	cd = (char **)malloc(memalloc * sizeof(char *));
	for (l=0;l<memalloc;l++)
		cd[l] = (char *)malloc(1024 * sizeof(char));


	if 	(m_Run_Tx.GetCheck() != 0)
	{	nRetValue = Build_Tx_Command();	// build the command
		// Check the files
		sprintf(cd[jn],"%s",m_Tx_Command);
		jn++;
		//hr = m_pQue->SendCmd(1,cd,&sts);
	}

	if (m_Run_at.GetCheck() != 0)
	{	nRetValue = Build_At_Command();
		sprintf(cd[jn],"%s",m_At_Command);
		jn++;
		//hr = m_pQue->SendCmd(1,cd,&sts);
	}

	if 	(m_Run_rc.GetCheck() != 0 || m_Run_sc.GetCheck() != 0)
	{
		valErr = CheckInputFiles();
		if (valErr == 0)
		{
					
			for(l=st;l<en;l++)
			{
				if (m_multi_frames)
					GetEmFilename(l,&m_Em_File);
				// Run Scatter Process
				if 	(m_Run_sc.GetCheck() != 0)
				{	// submit the job
//					Build_Scatter_Command();
					Build_e7_scatter_Command(l);
					sprintf(cd[jn],"%s",m_Sc_Command);
					jn++;
					//hr = m_pQue->SendCmd(1,cd,&sts);
				}	
				if 	(m_Run_rc.GetCheck() != 0)
				{	// submit the job
					if (m_multi_frames) Build_Recon_Command(l);
					else Build_Recon_Command(-1);
					sprintf(cd[jn],"%s",m_Rc_Command);
					jn++;
					//hr = m_pQue->SendCmd(1,cd,&sts);
				}
				if (m_Type == 4)
				{	
					CreateHOSPFile();
					sprintf(cd[jn],"%s",m_HOSP_Cmd);
					jn++;
					//hr = m_pQue->SendCmd(1,cd,&sts);
				}
				if (m_Type == 3 || m_Type == 4)// If OSEM3d then Quantify
				{
					// Write Image Header
					WriteImageHeader();
					// Quantify image
					QuantifyImage();
				}

				//AppendImageHeader();
			}
		}
	}
//	if (jn > 0)
//		hr = m_pQue->SendCmd(jn,cd,&sts);
	for (l=0;l<memalloc;l++)
		free(cd[l]);
	free(cd);

}

void CReconGUIDlg::OnBnClickedButton6()
{
//	long num,jtime;
//	HRESULT hr;

//	hr = m_pQue->GetJobStatus(&num, &jtime);
//	m_JobNum = num;
//	m_JobTime = jtime;
	UpdateData(FALSE);
}

int CReconGUIDlg::get_num_priors()
{
	return map_Priors.size();
}

int CReconGUIDlg::get_prior(int index, CString &name, CString &value)
{
	int pos=0;
	for (MAP_TR_PriorMapIterator it=map_Priors.begin(); it!=map_Priors.end(); it++ )
	{
		if (pos==index) 
		{
			name = it->first;
			value = it->second;
			return 1;
		}
		pos++;
	}		
	return 0;
}

int CReconGUIDlg::set_prior(int index, CString &value)
{
	int pos=0;
	for (MAP_TR_PriorMapIterator it=map_Priors.begin(); it!=map_Priors.end(); it++ )
	{
		if (pos==index) 
		{
			m_MAP_prior = value;
			it->second = m_MAP_prior;
			return 1;
		}
		pos++;
	}		
	return 0;
}

/*
 *	void CReconGUIDlg::RemoveJob(int job) 
 *   Remove specified job number and update the queue display list
 */
void CReconGUIDlg::RemoveJob(int job) 
{
	TaskQueue tmp;
	TaskQueueIterator it;
	for (it = queue.begin(); it != queue.end(); it++)	tmp.insert(*it);
	queue.clear();
	for (it = tmp.begin(); it != tmp.end(); it++)
	{
		if (it->second->job_ID == job &&
			it->second->status == ProcessingTask::Waiting) delete it->second;
		else queue.insert(*it);
	}
	RefreshTaskList();
}

/*
 *	void CReconGUIDlg::OnRemoveJob() 
 *  RemoveJob button callback
 */
void CReconGUIDlg::OnRemoveJob() 
{
	CString txt;
	int job=0, task=0;
	int	sel = m_ListTask.GetCurSel();
	if (sel != LB_ERR)
	{
		m_ListTask.GetText(sel, txt);
		if (sscanf(txt, "%d:%d", &job, &task) == 2) RemoveJob(job);
	}
}	

/*
 *	void CReconGUIDlg::OnRemoveAll() 
 *  RemoveAll button callback
 */
void CReconGUIDlg::OnRemoveAll() 
{
	TaskQueue tmp;
  TaskQueueIterator it;
	CString txt;
	int job=0, task=0;
	for (it = queue.begin(); it != queue.end(); it++)
		tmp.insert(*it);
	queue.clear();
	for (it = tmp.begin(); it != tmp.end(); it++)
	{
		if (it->second->status == ProcessingTask::Waiting) delete it->second;
		else queue.insert(*it);
	}
	RefreshTaskList();
}

/*
 *	void CReconGUIDlg::OnSelchangeListTask() 
 *  Task selection callback
 */
void CReconGUIDlg::OnSelchangeListTask() 
{
	int top = 0;
	int job=0, task=0;
	CString txt;
	int	sel = m_ListTask.GetCurSel();
	if (sel != LB_ERR)
	{
		m_RemoveJob.EnableWindow(TRUE);	
		if (sel>0) 
		{
			m_ListTask.GetText(sel, txt);
			if (sscanf(txt, "%d:%d", &job, &task) == 2)
			{
				TaskQueueIterator it = queue.begin();
				if (job != it->second->job_ID)
				{ // not first job
					top = it->second->job_ID-1;
				}
			}
		}
	}
	else
	{
		m_RemoveJob.EnableWindow(FALSE);
	}
	if (top > 0) m_TopJob.EnableWindow(TRUE);
	else m_TopJob.EnableWindow(FALSE);
}

/*
 *	void CReconGUIDlg::OnTopJob() 
 *  TopJob button callback
 */
void CReconGUIDlg::OnTopJob() 
{
	TaskQueue tmp;
	CString txt;
	int job=0, task=0;
	int	sel = m_ListTask.GetCurSel();
	if (sel>0)
	{
		TaskQueueIterator it = queue.begin();
		m_ListTask.GetText(sel, txt);
		int top = it->second->job_ID-1;
		if (top>0 && sscanf(txt, "%d:%d", &job, &task) == 2)
		{
			for (it = queue.begin(); it != queue.end(); it++)
			{
				ProcessingTask* cur = it->second;
				if (cur->job_ID == job) cur->job_ID = top;
				int key = cur->task_ID + cur->job_ID*MAX_NUM_TASKS;
				tmp.insert(std::make_pair(key, cur));
			}
			queue.clear();
			for (it = tmp.begin(); it != tmp.end(); it++)
				queue.insert(*it);
		}
		RefreshTaskList();
	}
}
void CReconGUIDlg::OnEnChangeEditEmHistStart()
{
  CString str;
  m_EM_HIST_Start.GetWindowText(str);
  int val = atoi(str);
  if (val>= 0 && val<100000) 
  { // valid value
    m_start_countrate = val;
  }
  else 
  { // restore previous value
    str.Format("%d", m_start_countrate);
    m_EM_HIST_Start.SetWindowText(str);
  }
}
