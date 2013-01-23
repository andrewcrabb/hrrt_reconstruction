
/******************************************************************************
*
* histogram_dll.c
*
*	Prototype program for 8.0
*
*	Collection of NT-DLL  to do histogramming from list mode data file with
*	memory and various histogramming control option.
*
*	adapt from a preliminary prototype developed in March 1998.
*
*	And used to create sinograms for phantom and other data sets on the HRRT
*	scanner, in Cologne, Germany.
*
*	Also used to create data sets for SNM-show & RSNA demos as a standalone
*	component since Jun, 1998
*
*
*	**	May, 1998		P. Luk
*	   Origin version
*
*	**	August 1999		P. Luk
*		Add API to support MedComp BE
*
*	**	Dec 1999			P. Luk	
*		modifications for PetSpect beta release, specifically targeted for
*		the PetSpect WB protocol.	:
*
*			create 2 files, emission histogram & transmission sinograms
*				in case of simultaneous Em/Tx scans
*			support only one pass histogramming, the case where there is enough
*				physical memory to hold one temporal frame and other overhead
*				memory requirements
*			support only static scans
*
*	** May 19, 2000			P. Luk
*		modify to write INTERFILE ascii header file
*
*	** May 22, 2000			P. Luk
*		write a dedicated subroutine to write INTERFILE header
*		separate the ascii header from subroutine frame_write
*
*	** Sept 11, 2002		S Tolbert
*		modified HISTBuf() added code to prevent division by 0
*
*   ** Oct 01, 2002			S Tolbert
*		added code to use virtural malloc to allocate the memory
*		functions added histo_allocate(...) and histo_deallocate(...)
******************************************************************************/

#define _WIN32_WINNT 0x0400
		
#include <windows.h>			// MicroSoft Windows include files
#include <winbase.h>

#include <stdio.h>				// standard C library include files
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <string.h>
#include <limits.h>

#include "histogram_st.h"		// include file for internal histogram control
#include "histogram_errors.h"	// include file for histogram errors

#include "LmHistBeUtils.h"		// include files for MedComp backend
#include "LmHistStats.h"

//#include "StdAfx.h"
#include "gantryModelClass.h"	// use generic gantry model object, 12/07/01

// defines for memory allocation/deallocation functions
#define ALLOC_malloc		0
#define	ALLOC_win32Heap		1
#define ALLOC_win32Virtual	2
#define ALLOC_new			3

// variables for head memory allocation
static short int *sbuf=NULL ;
static HANDLE hHeap=NULL;

//VAR for DEBUGGING, for now
//static int		GS_nticks ;
// ZYB
static __int64	GS_nticks ;
static DWORD	GS_t0 ;				// 23-Sept-2001, add to support scan duration
static DWORD	GS_t1 ;				// for stopping by counts
static DWORD	GS_t0_statref ;

// variables for MedComp call-backs and histogram statistics
static HIST_stats	*pHS ;		// MedCom Histogram Statistic Object
static CCallback	*pCB ;		// MedCom Histogram Call Back Object
static CSinoInfo	*pSI ;		// MedCom Sinogram info structure
static int			GS_MedCompFlag ;	// for Histogram Status report to MedComp

// ZYB
/*
// variables for scan statistics
static long GS_em_prompts, GS_em_randoms;
static long GS_tx_prompts, GS_tx_randoms;
static long GS_old_em_prompts;
static long GS_old_em_randoms;
static long GS_old_tx_prompts;
static long GS_old_tx_randoms;
static long GS_em_prompts_rate ;
static long GS_em_randoms_rate ;
static long GS_tx_prompts_rate ;
static long GS_tx_randoms_rate ;
static long GS_em_trues_rate ;
static float GS_TotalSinglesRate ;
static long GS_NumSinglesSamples ;
*/

// variables for scan statistics
static __int64 GS_em_prompts, GS_em_randoms;
static __int64 GS_tx_prompts, GS_tx_randoms;
static __int64 GS_old_em_prompts;
static __int64 GS_old_em_randoms;
static __int64 GS_old_tx_prompts;
static __int64 GS_old_tx_randoms;
static __int64 GS_em_prompts_rate ;
static __int64 GS_em_randoms_rate ;
static __int64 GS_tx_prompts_rate ;
static __int64 GS_tx_randoms_rate ;
static __int64 GS_em_trues_rate ;
static __int64 GS_TotalSinglesRate ;
static __int64 GS_NumSinglesSamples ;


// variables for state definiton, state transition & control
static int	GS_histo_state ;			// stores the state of histogram FSA
static int	GS_histo_pending_command ;	// stores command received but not realized yet

// variables for thread serving asynchronous API response
static HANDLE hWriteEvent ;			// event signal for write thread
static HANDLE hWriteThread ;		// pseudo handle for write thread
static DWORD dhWriteThreadId ;		// thread ID for write thread
static HANDLE hReplayEvent ;		// event signal for offline histo-thread
static HANDLE hReplayThread ;		// pseudo handle for offline histo-thread
static DWORD dhReplayThreadId ;	// thread ID for offline thread

// variables for timer to computer scan statistics
static HANDLE	hTimer ;			// Timer handle
static HANDLE	hTimerThread ;		// pseudo handle for timer thread
static DWORD	dTimerThreadId ;	// thread ID for timer thread
static HANDLE	hOnlineEvent ;		// Event to trigger statistical polling	

// variables and definitions for error handling and reporting
static char	GS_error_message[256] ;
extern int errno ;					// error number from C library
extern char *sys_errlist[] ;		// error message from C library

// C-structures for performing histogram
struct _histo_st		histo_st ;		// C-struct for basic histogram usage
struct _frame_def_st	frame_def_st ;	// C-struct for frame definition & control
struct _tag_st			tag_st ;		// C-struct for tag words
struct _header_st		header_st ;

// Forward declarations to support basic histogram
int histo_buf_net_trues_1pass(long*, int, int, struct _histo_st*, struct _tag_st *, struct _frame_def_st*) ;
int frame_write(struct _histo_st *, struct _tag_st *, struct _frame_def_st *) ;
int write_interfile_header(int, struct _histo_st *, struct _tag_st *, struct _frame_def_st *) ;
int write_interfile_header2(char *, int, int, struct _histo_st *, struct _tag_st *, struct _frame_def_st *) ;
int histo_tag(struct _tag_st *, unsigned long, struct _frame_def_st *) ;
int	histogram() ;
BOOL histo_allocate(DWORD dwNbytes, int allocCode);
BOOL histo_deallocate(LPVOID lpHistoMem, int AllocCode);

// Forward declarations for API supporting functions
void unconfig(struct _histo_st*, struct _frame_def_st*, struct _tag_st*) ;
int set_attributes(struct _histo_st*, int, int, int, int) ;
void show_attributes(struct _histo_st) ;
void show_frames(struct _frame_def_st) ;
int set_sinogram_mode(struct _histo_st*, int, int, int) ;
int config(struct _histo_st*, struct _frame_def_st*, struct _tag_st*, char*, char*) ;
int define_frames(struct _frame_def_st *, int, int, long) ;

// Forward declarations for thread initialization
int histo_init(HIST_stats*, CCallback*) ;
int init_offline() ;
int init_frame_write() ;
void TimerService() ;

/******************************************************************
*
*	HISTInit API
*
*		-Perform one time initialization.
*		-Statistics structure and callback object created by the MedComp
*			backend are passed.
*
*	Steps:
*			copy Medcomp callback and statistics pointers
*			create OFF-line win32 event to synchronize Offline histogram driver
*			create ON-line win32 event for Online histogram statistics polling
*			initialize thread for offline-histogram driver
*			create event for asynchronous frame-write
*			initialize thread for asynchronous writing to disk
*			create the timer for statistics polling
*			create thread for timer service
*			notify MedComp BE
*
*	Returns:
*			1	MedComp_OK
*		or	0	MedComp_ERR
*
*******************************************************************/

__declspec(dllexport)
int HISTInit(HIST_stats *in_pHS, CCallback *in_pCB, CSinoInfo *in_pSI)
{
  int status ;

	printf("Entering HISTInit API version 3.0 with block singles...\n") ;

	// copy Medcomp callback and statistics pointers
	pHS = in_pHS ;
	pCB = in_pCB ;
	pSI = in_pSI ;

	// create OFF-line event for Offline histogram driver
	hReplayEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
							TRUE,
							FALSE,
							(LPTSTR) NULL) ;
	if (hReplayEvent == NULL)
	{
		printf("HISTInit: Could NOT create Synchronous Event\n") ;
		return(MEDCOMP_ERR) ;
	}

	// create ON-line event for Online histogram statistics polling
	hOnlineEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
							TRUE,
							FALSE,
							(LPTSTR) NULL) ;
	if (hOnlineEvent == NULL)
	{
		printf("HISTInit: Could NOT create Synchronous Event\n") ;
		return(MEDCOMP_ERR) ;
	}

	// initialize thread for offline-histogram driver
	status = init_offline() ;
	if (status != OK) return(MEDCOMP_ERR) ;

	// Create event for asynchronous frame-write
	hWriteEvent = CreateEvent((LPSECURITY_ATTRIBUTES) NULL,
							FALSE,
							FALSE,
							(LPTSTR) NULL) ;
	if (hWriteEvent == NULL)
	{
		printf("HISTInit: Could NOT create Synchronous Event\n") ;
		return(MEDCOMP_ERR) ;
	}

	// set model from BE information, IF the gantry model is not PETSPECT or HRRT -12/07/01
	// S Tolbert 19 Sept 02 - Set the correct model information
	CGantryModel model ;			// common area for generic gantry model object
	if (pSI->gantry == GNT_PETCT)	
	{
		printf("HISTInit: SetModel for Gantry Object = %d, %s\n", pSI->gantry, pSI->gantryModel) ;
		status = model.setModel(pSI->gantryModel) ;
		if (!status)
		{
		  printf("HISTInit: Could NOT set model for generic gantry model object\n");
		  return(MEDCOMP_ERR) ;
		}
	
		header_st.ndetblks = model.buckets() * model.axialBlocks();	// Ring Arch Scanner
		header_st.nrings = model.directPlanes() ;
		header_st.binsize = (float) model.binSize() ;
		header_st.plane_separation = (float) model.planeSep() ;
		header_st.binsize *= 10.0 ;			// from cm to mm
		header_st.plane_separation *= 10.0 ; // from cm to mm
	}

	if (pSI->gantry == GNT_P39)	
	{
		printf("HISTInit: SetModel for Gantry Object = %d, %s\n", pSI->gantry, pSI->gantryModel) ;
		status = model.setModel(pSI->gantryModel) ;
		if (!status)
		{
		  printf("HISTInit: Could NOT set model for generic gantry model object\n");
		  return(MEDCOMP_ERR) ;
		}
	
		header_st.ndetblks = model.axialBlocks();		// Panel Arch Scanner
		header_st.nrings = model.directPlanes() ;
		header_st.binsize = (float) model.binSize() ;
		header_st.plane_separation = (float) model.planeSep() ;
		header_st.binsize *= 10.0 ;			// from cm to mm
		header_st.plane_separation *= 10.0 ; // from cm to mm
	}

	// initialize thread for asynchronous writing to disk
	status = init_frame_write() ;
	if (status != OK) return(MEDCOMP_ERR) ;

	// create the timer for statistics polling
	 // this was created previously to poll statistics periodically.
	 // it is not used anymore since stats are updated per buffer completion
	 // the mechanics is left behind here in case a waitable timer is needed later on
	hTimer = CreateWaitableTimer((LPSECURITY_ATTRIBUTES) NULL,
								 (BOOL) FALSE,
								 (LPCTSTR) NULL) ;

	// notify MedComp BE
	pCB->notify(MEDCOMP_OK, "HistInit: Histogram initialized") ;

	GS_histo_state = HISTO_INITIALIZED ;
	return(MEDCOMP_OK) ;
}

/****************************************************************
*
*	HISTStart API
*
*		Invoke to start Online or Replay histogramming
*
*	Returns:
*			1	MedComp_OK
*		or	0	MedComp_ERR
*
******************************************************************/

__declspec(dllexport)
int HISTStart()
{
	BOOL ok ;

	printf("Entering HISTStart API...\n") ;

	// MEDCOMP BE, add code here to acknowledge START-API received
	pCB->notify(1, "HISTStart: Start ACK") ;

	printf("HISTStart: histo-state = %d\n", GS_histo_state) ;

	if (GS_histo_state != HISTO_CONFIGURED)	// system not ready to start
		return(MEDCOMP_ERR) ;
	else							// system ready to start
	{
	  if (!histo_st.online_flag)	// offline start
	  {
		printf("HISTStart: trigger event to start Off-line histogram\n") ;

		ok = SetEvent(hReplayEvent) ;	// set event to trigger Off-line Histogramming

		if (ok) printf("HISTStart: offline start event triggered successfully\n");
		else printf("HISTStart: offline start event trigger FAILED\n") ;
	  }
	  else		// online start, to trigger online statistics polling
	  {
		  printf("HISTStart: trigger event to start online-histogram\n") ;

		  ok = SetEvent(hOnlineEvent) ;	// set event to trigger On-line Histogram

		  if (ok) printf("HISTStart: online start event triggered successfully\n");
		  else printf("HISTStart: online start event trigger FAILED\n") ;
	  }
	  if (ok)
	  {
		  // initialize count statistics variables
		  pHS->n_updates = 0;
		  pHS->prompts_rate = 0;
		  pHS->random_rate = 0;
		  pHS->net_trues_rate = 0;
		  pHS->total_singles_rate = 0;
		  GS_t0 = GetTickCount() ;	// 23-Sept-2001, add to support stop by counts
		  GS_t0_statref = GS_t0 ;	// 27-Sept-2001, add to support tallying count stats
		  return(MEDCOMP_OK) ;
	  }
	  else return(MEDCOMP_ERR) ;
	}

}

/****************************************************************
*
*	HISTStop API
*
*		Stop the current online or offline histogramming process.
*		Provides Asynchronous response.
*
*	Returns:
*			1	MedComp_OK
*
******************************************************************/

__declspec(dllexport)
int HISTStop()
{
	printf("Entering HISTStop API ...\n") ;
	printf("HISTStop: GS_histo_state = %d\n", GS_histo_state) ;

	// MEDCOMP BE, invoke callback to notify client on Stop-API command received
	pCB->notify(1, "HISTStop: Stop ACK") ;

	// the conditional below will signal to stop processing and store the data
	if (GS_histo_state == HISTO_ACTIVE || GS_histo_state == HISTO_WAIT)
	{
		printf("HISTStop: set flag to STOP pending\n") ;
		GS_histo_pending_command = HISTO_STOP_PENDING ;
	}

	// signal to stop statistical polling
	if (histo_st.online_flag) ResetEvent(hOnlineEvent) ;
	else ResetEvent(hReplayEvent) ;

	return(MEDCOMP_OK) ;
}


/***************************************************************
*
*	HISTAbort API
*
*		Abort the online or offline histogram process
*
*	Returns:
*			1	MedComp_OK
*
*****************************************************************/

__declspec(dllexport)
int HISTAbort()
{
	printf("Entering HISTAbort ...\n") ;

	// MEDCOMP BE, invoke callback to notify client on Abort-API command received
	pCB->notify(1, "HISTAbort: Abort ACK") ;

	// terminate thread for Offline histogram driver
	//if (GS_histo_state == HISTO_ACTIVE || GS_histo_state == HISTO_WAIT)
	//{
		GS_histo_pending_command = HISTO_ABORT_PENDING ;
	//}

	// signal to stop statistical polling
	if (histo_st.online_flag) ResetEvent(hOnlineEvent) ;
	else ResetEvent(hReplayEvent) ;

	return(MEDCOMP_OK) ;
}

/***************************************************************
*
*	HISTBuf API
*
*		DLL to interface with the list mode module, use as a stub
*		to call the histogram sorting subroutine
*
*		returns: 1		OK
*				 0		end-of-scan
*				 
******************************************************************/

__declspec(dllexport)
int	HISTBuf(char *lmbuf, int nbytes, int preset)
{
	int status ;
	DWORD	t1, dt ;

	//printf("HISTBuf: preset = %d\n", preset) ;
	if (preset == pCB->Stop) GS_histo_pending_command = HISTO_STOP_PENDING ;

	t1 = GetTickCount();
	dt = t1 - GS_t0_statref ;
	GS_t0_statref = t1 ;

	status = histo_buf_net_trues_1pass((long*) lmbuf,
								nbytes,
								preset,
								&histo_st,
								&tag_st,
								&frame_def_st) ;

	//printf("HISTBuf: returns %d\n", status);

	// S Tolbert 11 Sept 02 - prevent division by 0
	// compute prompts, randoms & net-trues rates
	if (dt > 0 )
	{
		GS_em_prompts_rate = 1000*(GS_em_prompts - GS_old_em_prompts)/dt;
		GS_em_randoms_rate = 1000*(GS_em_randoms - GS_old_em_randoms)/dt;
		GS_em_trues_rate = GS_em_prompts_rate - GS_em_randoms_rate ;
	}

	// MEDCOMP BE callback
	// put in actual HIST_stats call here ...
	pHS->prompts_rate = GS_em_prompts_rate;
	pHS->random_rate = GS_em_randoms_rate;
	pHS->net_trues_rate = GS_em_trues_rate;
	pHS->total_singles_rate = GS_em_prompts - GS_em_randoms;
	pHS->n_updates++;

	// preparation for next round
	GS_old_em_prompts = GS_em_prompts ;
	GS_old_em_randoms = GS_em_randoms ;
	GS_old_tx_prompts = GS_tx_prompts ;
	GS_old_tx_randoms = GS_tx_randoms ;

	if (status != TAG_END_OF_SCAN) return(MEDCOMP_OK) ;
	else
	{
		// signal to stop statistical polling
		if (histo_st.online_flag) ResetEvent(hOnlineEvent) ;
		else ResetEvent(hReplayEvent) ;
		return(0) ;
	}
}


/***************************************************************
*
*	HISTSinoMode API
*
*		sets emission and transmission sinogram mode
*		sinogram mode definition:
*			0 = net trues
*			1 = prompts and randoms
*
*	DEFAULTS to net trues if no API call was invoked
*
*	Returns:
*			1	MedComp_OK
*			0	MedComp_Err
*
******************************************************************/

__declspec(dllexport)
int	HISTSinoMode(int em_sinm, int tx_sinm, int acquisition_mode)
{
  int status ;

	printf("Entering HISTSinoMode API...\n") ;

	status=set_sinogram_mode(&histo_st, em_sinm, tx_sinm, acquisition_mode) ;
	printf("HISTSinoMode: histo_state = %d\n", GS_histo_state) ;

	return(status) ;
}

/**************************************************************
*
*	HISTDefineFrames API
*
*		Defines dynamic frames
*
*	Returns:
*			nframes_defined		(> 0)
*		or	0, Error, no frames defined successfully
*	
***************************************************************/

__declspec(dllexport)
int HISTDefineFrames(unsigned long nframes,
						unsigned long frame_def_code,
						unsigned long preset_value)
{
  int nframes_defined ;

	printf("Entering HISTDefineFrames API ...\n") ;

	printf("HISTODefineFrames: nf, code, value = %d, %d, %d\n",
		nframes, frame_def_code, preset_value) ;

	nframes_defined = define_frames(&frame_def_st,
							(int) nframes,
							(int) frame_def_code,
							(int) preset_value) ;

	printf("HISTDefineFrames: histo_state = %d\n", GS_histo_state) ;

	if (nframes_defined > 0) return(MEDCOMP_OK) ;
	else return(MEDCOMP_ERR) ;
}


/******************************************************************
*
*	HISTSetAttributes API
*
*		Sets histogram attributes relating to sinogram dimensions
*		and memory allocation
*
*	Returns:
*			1	MedComp_OK
*		or	0	MedComp_ERR
*
********************************************************************/

__declspec(dllexport)
int HISTSetAttributes(unsigned long nprojs,
						  unsigned long nviews,
						  unsigned long n_em_sinos,
						  unsigned long n_tx_sinos)

{
  int	status ;

	printf("Entering HISTSetAttributes API ...\n") ;

	status = set_attributes(&histo_st,
					(int) nprojs,
					(int) nviews,
					(int) n_em_sinos,
					(int) n_tx_sinos) ;

	printf("HISTSetAttributes: histo_state = %d\n", GS_histo_state) ;

	return(status) ;
}


/**************************************************************************
*
*	HISTUnconfig API
*
*		Clear buffer and deallocate resources.
*		Serve as the starting delimiter to configure a histogramming session
*
*	Returns:
*			1	MedComp_OK
*
***************************************************************************/
__declspec(dllexport)
int HISTUnconfig()

{
	printf("Entering HistUnconfig API ...\n") ;

	unconfig(&histo_st, &frame_def_st, &tag_st) ;

	printf("HISTUnconfig: histo_state = %d\n", GS_histo_state) ;

	return(MEDCOMP_OK) ;
}


/*****************************************************************************
*
*	HISTConfig API
*
*		Configure & initialize histogram memory,
*		Create output file
*
*	Arguments
*
*		SinoFileName is the output histogram file name(s) for dynamic
*			else for static, it is the full output histogram file name
*
*		listFname is option. A NULL string implies on-line histogramming
*		otherwise, it is the input listmode filename for off-line histogramming
*
*	Returns:
*		1	MedComp_OK
*		0	MedComp_ERR
*
******************************************************************************/
__declspec(dllexport)
int	HISTConfig(char *SinoFileName, char *ListFname)

{
	int status ;

	printf("Entering HISTConfig ...\n") ;

	status = config(&histo_st, &frame_def_st, &tag_st, SinoFileName, ListFname) ;
	printf("HISTConfig: histo_state = %d\n", GS_histo_state) ;

	if (status == OK) return(MEDCOMP_OK) ;
	else
	{
		pCB->notify(MEDCOMP_ERR, GS_error_message) ;  // error message to MC-BE
		return(MEDCOMP_ERR) ;
	}
}

__declspec(dllexport)
int HISTWait()			// Add by PL, 17-Jan-2002
{

  DWORD sleepTime = 3;

	while (GS_histo_state == HISTO_ACTIVE
		|| GS_histo_state == HISTO_WAIT
		|| GS_histo_state == HISTO_COMPLETE
		|| GS_histo_state == HISTO_STOPPED)
	{
	  printf("HISTWait: GS_histo_state = %d\n", GS_histo_state);
	  Sleep(sleepTime*1000);
	}

	return((int) GS_histo_state);

}

/****************************************************************
*
*	histo_show_attributes API
*
*	- for debugging purpose only
*
*****************************************************************/
__declspec(dllexport)
void histo_show_attributes(struct _histo_st *reply_histo_st)

{

	printf("Entering histo_show_attributes ...\n") ;

	*reply_histo_st = histo_st ;
	show_attributes( histo_st ) ;
}

/***************************************************************
*
*	histo_show_frames API
*
*	- for debugging purpose only
*
****************************************************************/
__declspec(dllexport)
void histo_show_frames()
{
	printf("entering dll-subroutine histo_show_frames ...\n") ;

	show_frames(frame_def_st) ;
}

/**********************************************************************
*
*	histogram()
*
*	This is the Off-line (or Replay Mode) histogram driver that
*	responds to the START API from the Off-line MedComp BE.
*
*	Returns:
*			0	OK
*		or	-1	HISTO_ERROR
*		or	-3	HISTO_MEM_SHORTAGE
*		or	-4	HISTO_LISTFILE_OPEN_ERROR
*
************************************************************************/
int histogram()

{

   int fdin, nbytes_read, total_nbytes_read=0, end_of_file;
   int pass, nscanned, bufcnt=0 ;
   int iobufsize = 4*1024*1024 ;
   int max_em_offset, max_tx_offset ;
   int em_sinm, tx_sinm, acquisition_mode ;
   int nbytes_per_pass ;
   int logical_lower_bound, logical_upper_bound ;

   int SinoBinSizeInBytes=sizeof(short), npasses=1 ;
   int start_lmwd=0 ;
   int store_status;
   long filepos ;

   DWORD	t0, t1, t2, tt0, tt1 ;  // for timing statistics

   printf("Entering subroutine to perform offline histogramming\n") ;
   
   // allocates memory space for reading raw list mode data
   printf("histogram: malloc listmode buffer\n") ;
   histo_st.lm_input_buff = (long *) malloc(iobufsize) ;
   if (histo_st.lm_input_buff == NULL)
   {
   		printf("histogram: memory shortage\n") ;
		return(HISTO_MEM_SHORTAGE) ;
   }

   // opens list mode raw data file for reading
   printf("histogram: open list mode file for reading\n") ;
   _fmode = O_BINARY ;
   fdin = open(histo_st.list_file, O_RDONLY | O_BINARY) ;
   if (fdin == -1)
   {
	  printf("histogram:ERROR: failed to open file '%s' for reading\n", histo_st.list_file);
	  printf("histogram:ERROR: System Message: %s\n", sys_errlist[errno]) ;
      return(HISTO_LISTFILE_OPEN_ERROR) ;
   }

   // get attributes from data object: C-structure "histo_st"
   printf("histogram: copying attributes from C-struct\n") ;
   em_sinm = histo_st.em_sinm ;
   tx_sinm = histo_st.tx_sinm ;
   max_em_offset = histo_st.max_em_offset ;
   max_tx_offset = histo_st.max_tx_offset ;
   acquisition_mode = histo_st.acqm ;
   
   printf("histogram: beginning of scan loop ..\n") ;
   tt0 = GetTickCount() ;
   npasses = histo_st.npasses ;
   for (pass=0 ; pass < npasses ; pass++)	//BEGIN-pass-loop
   {

      end_of_file = 0 ;
      logical_lower_bound = histo_st.start_offsets[pass] ;
      logical_upper_bound = histo_st.end_offsets[pass] ;
	  histo_st.pass = pass ;
	  printf("histogram: pass = %d, lb = %d, ub = %d\n",
			pass, logical_lower_bound, logical_upper_bound) ;
      nbytes_per_pass = (logical_upper_bound - logical_lower_bound)*SinoBinSizeInBytes ;

      printf("\nhistogram: pass %d ..\n", pass+1) ;
      printf("histogram:start_offset, end_offset = %d, %d\n",
   		 logical_lower_bound, logical_upper_bound) ;
      printf("histogram: estimated total number of bytes this pass = %d\n", nbytes_per_pass) ;
      if (pass > 0) memset(histo_st.sino_base, 0, nbytes_per_pass) ;

      filepos=lseek( fdin, (long) (start_lmwd*sizeof(long)), 0) ;
      if (filepos != (long) (start_lmwd*sizeof(long)))
	  {
		 printf("histogram:ERROR:error in lseeking to %d bytes offset\n", start_lmwd*sizeof(long));
		 perror("histogram:ERROR:system error message") ;
		 return(HISTO_ERROR) ;
	  }
      printf("histogram: starting file position to scan list mode file = %d\n", start_lmwd*sizeof(long)) ;

      printf("\n") ;
      while ( !end_of_file )	// BEGIN-while-loop
	  {
		 t0 = GetTickCount() ;
		 nbytes_read = read(fdin, histo_st.lm_input_buff, iobufsize) ;
		 t1 = GetTickCount() ;
		 //printf("buf %d, %d msecs to read %d bytes\n", bufcnt, t1-t0, nbytes_read) ;
		 total_nbytes_read += nbytes_read ;
		 bufcnt++ ;
		 if (nbytes_read <= 0)	// no more list mode data to sort
		 {
			end_of_file = 1 ;
		 }
         else					// sort list mode data stream
		 {
			if (npasses == 1 && em_sinm == NET_TRUES && tx_sinm == NET_TRUES)
			{
			   /*****
               nscanned=histo_buf_net_trues_1pass(
                  histo_st.lm_input_buff,
                  nbytes_read,
				  -1,					// it was set to 0, fix 17-Jan-2002
                  &histo_st,
                  &tag_st,
                  &frame_def_st) ;
				*****/
				nscanned = HISTBuf((char *) histo_st.lm_input_buff, nbytes_read, -1);
			}
			else	// special edition for PETSPECT beta, multi-pass not supported
			{
				return(HISTO_ERROR) ;
			}

			// other histogramming cases will be added as "histo_buf.." subroutines
			// e.g. the cases for :
			//		not enough memory,
			//		separate emission/transmission prompts and randoms,
			//		continuous histogram with overlapping but no interpolation
			//		  and continuous sampling histogramming with overlapping and interpolation
			//		  with and without preallocation of physical memory

			if (nscanned == 0) break ;
		}

		t2 = GetTickCount() ;
		//printf("%d msec to bin into histo\n", t2-t1) ;
		//printf("total time = %d msec\n", t2-t0) ;

      }		//END-while-loop

	  printf("histogram: falling through to end of main loop\n") ;

	  // S Tolbert 16 Sept 02 - Added the AcqPostComplete to trigger the AcqTool GUI an update command
	  printf("histogram: Setting AcqComplete\n") ;
	  GS_MedCompFlag = pCB->Preset ;
	  pCB->acqComplete(GS_MedCompFlag) ;

	  // this is to handle the special case of no timing tag words
	   // in the listmode stream
	    // OR, for the case of no frames explicitly defined

	  if (nscanned != 0)
	  {
		printf("\n\nhistogram: FALLING THROUGH TO EOF\n\n") ;

		/* writing histogram to disk */
		t1 = GetTickCount() ;
		printf("histogram:time to sort list mode file = %d seconds\n", (t1-tt0)/1000) ;
		t0 = GetTickCount() ;
		frame_def_st.frame_switch_time[frame_def_st.cur_frame]
			= (long)tag_st.ellapsed_time ;
		store_status = frame_write(&histo_st, &tag_st, &frame_def_st) ;
		t1 = GetTickCount() ;
		printf("histogram: %d msec to write to disk\n", t1-t0) ;

		// S Tolbert 16 Sept 02 - Added the AcqPostComplete to trigger the AcqTool GUI a stop command
	    printf("histogram: Setting AcqPostComplete\n") ;
	    GS_MedCompFlag = pCB->Preset ;
	    pCB->acqPostComplete(GS_MedCompFlag, MEDCOMP_OK) ;

		tt1 = GetTickCount() ;
		printf("histogram: end of scan loop\n") ;
		printf("histogram: total time = %d msec\n", tt1 - tt0) ;

		printf("histogramming done\n") ;
		printf("histogram: store_status = %d\n", store_status);
		
		close(fdin) ;
		free(histo_st.lm_input_buff) ;
		if (histo_st.online_flag) ResetEvent(hOnlineEvent) ;
		else ResetEvent(hReplayEvent) ;

		if (store_status == OK) GS_histo_state = HISTO_STORE_COMPLETE;
		else GS_histo_state = HISTO_STORE_ERR;
		return(GS_histo_state);
	  }

   }	//END-pass-loop

   int status;
   status = HISTWait();
   close(fdin) ;
   free(histo_st.lm_input_buff) ;
   if (histo_st.online_flag) ResetEvent(hOnlineEvent) ;
   else ResetEvent(hReplayEvent) ;

   return(status) ;

} 	/* end main program */


/******************************************************************************
*
*	histo_buf_nt_1pass
*
*	C subroutine to perform coincidence events histogramming function on a
*	buffer of list-mode data stream.
*
*   this is a highly optimized routine to handle the special but majority case
*   of fully sufficient system memory and net trues events
*
*	As of 25-Aug-1999, the timing tagwords from the Coincidence Processor
*	hardware is still un-finalized. Therefore, instead of using the timing
*	tagwords within the list mode stream, as originally planned and implemented,
*	the listmode module outside of the Histogrammer has to keep track of the
*	preset time using the NT system clock/timer.
*
*	As a consequence, it complicates the logical and structure of this module
*	unnecessarily. BUT, this was what we had to operate on for the 25-Aug-1999
*	Alpha-release to Cologne, Germany.
*
*	Returns:
*			n	a non-negative integer indicating the total number of event
*				words processed in the input buffer
*		or	-4	TAG_END_OF_SCAN
*
*********************************************************************************/

int histo_buf_net_trues_1pass( long *lmbuf,
				int nbytes,
				int	external_preset,
				struct _histo_st *histo_st,
				struct _tag_st *tag_st,
				struct _frame_def_st *frame_def_st)
{
   // @z register long offset_mask = 0x3fffffff ;
   register long offset_mask = 0x1fffffff ;
   register long sino_offset ;
   register long sino_offset2 ;
   register long ewtype ;
   register int event_word_limit;
   register int acqm ;
   register int	em_sinm ;
   register int	tx_sinm ;

   short int *pwin ;
   short int *rwin ;
   short int *tx_rwin;

   int em_limit;
   int tx_limit;

   int	nevents ;
   int	histo_tag_status ;
   int	status ;
   int	i ;
	//char dum[128];
   //printf("entering histo_buf ...\n") ;

   GS_histo_state = HISTO_ACTIVE ;
   nevents = nbytes / sizeof(long) ;
   
   // @z
   if (nevents > 0xfffff) nevents = 0xfffff;

   acqm = histo_st->acqm ;
   em_limit = histo_st->max_em_offset ;
   tx_limit = histo_st->max_tx_offset ;
   em_sinm = histo_st->em_sinm ;
   tx_sinm = histo_st->tx_sinm ;

   // define valid offsets from rebinner output
   if (acqm == EM_ONLY)
	   event_word_limit = em_limit;
   else
	   event_word_limit = em_limit + tx_limit;

   // define the dynamic histogram memory segments pointer to adapt to the
   // new rebinner LUT (Aug2002) & to the new schema to histogram such events
   if (acqm != TX_ONLY)		// there are emission events
   {
	   if (histo_st->emp_offset == -1 && histo_st->emr_offset == -1)
	   {
		   printf("histo_buf: ERROR: using undefined Em pointers\n");
		   return(0);
	   }
	   else
	   {
		   pwin = histo_st->sino_base + histo_st->emp_offset + histo_st->max_em_offset;
	       rwin = histo_st->sino_base + histo_st->emr_offset + histo_st->max_em_offset;
	   }
	   if (histo_st->txr_offset != -1)
		   tx_rwin = histo_st->sino_base + histo_st->txr_offset;
	   else
		   printf("histo_buf: txr_offset pointer UNDEFINED\n");
   }
   else					// there is NO emission event
   {
	   if (histo_st->txp_offset == -1 && histo_st->txr_offset == -1)
	   {
		   printf("histo_buf: ERROR: using undefined Tx pointers\n");
		   return(0);
	   }
	   else
	   {
			pwin = histo_st->sino_base + histo_st->txp_offset;
			rwin = histo_st->sino_base + histo_st->txr_offset;
			tx_rwin = rwin;
	   }
   }
	
   for (i=0 ; i<nevents ; i++)
   {
   	  sino_offset = lmbuf[i] ;
	  //printf("histo_buf: listmode word %d = %x\n", i, lmbuf[i]) ;

	  // takes care of tag-words here
	  if (sino_offset < 0)
	  {
			histo_tag_status=histo_tag( tag_st, (unsigned long ) sino_offset, frame_def_st) ;
			// only handles offline case here, online is handle at end of loop
			if ( !histo_st->online_flag && histo_tag_status == TAG_END_OF_FRAME)
			{
			//	printf("histo_buf: frame changed via internal PRESET\n") ;
				if (frame_def_st->nframes > 1)		// offline dynamic
					status = frame_write(histo_st, tag_st, frame_def_st) ;
				if ( frame_def_st->cur_frame == (frame_def_st->nframes-1))
				{
			//		printf("histo_buf: offline END-OF_SCAN\n") ;
					GS_histo_state = HISTO_COMPLETE ;
					break ;
				}
				frame_def_st->cur_frame++ ;
			}
			continue ;		// process next listmode word
	  }

	  // takes care of event-words here
      ewtype = sino_offset & ~offset_mask ; 	// extract listmode event word type
      sino_offset = sino_offset & offset_mask ; // extract rebinner sinogram offset

      if (acqm != TX_ONLY)
      {
         if (sino_offset < event_word_limit)	// admit only valid events
         {
			sino_offset2 = sino_offset - em_limit;
			if (sino_offset2 < 0)					// emission event
			{
      	      if (ewtype == EM_PROMPT_MASK)
			  {
          		++pwin[sino_offset2] ;
				++GS_em_prompts ;
			  }
		      else if (ewtype == EM_RANDOM_MASK)
			  {
				if (em_sinm == NET_TRUES)
					--rwin[sino_offset2];
				else
					++rwin[sino_offset2];
				++GS_em_randoms;
			  }
			}
		 }
      }

      if (acqm != EM_ONLY)
      {
         if (sino_offset < event_word_limit)	// admit only valid events
         {
			sino_offset2 = sino_offset - em_limit;
			if (sino_offset2 >= 0)				// transmission event (S Tolbert 18 Sept 02 - changed to >=)
			{
   		      // @z if (ewtype == EM_PROMPT_MASK)
				if (ewtype == TX_PROMPT_MASK)
				{

          			++pwin[sino_offset2] ;
					++GS_tx_prompts ;
				}
              // @z else if (ewtype == EM_RANDOM_MASK)
			  else if (ewtype == TX_RANDOM_MASK)
			  {
				if (tx_sinm == NET_TRUES)
					--tx_rwin[sino_offset2];
				else
					++tx_rwin[sino_offset2];
				++GS_tx_randoms;
			  }
			  
			}
		 }
	  }
   }	// end for loop

   // this may be a critical section if STOP or ABORT is triggered; some kind
   // of mutex & semaphore will be great for precise response.
   // In the mean time, it does not matter since if misses here, it will catch
   // the trigger in the next buffer, an extra 0.33 seconds to respond

	//printf("histo_buf: histo-pending-command = %d\n", GS_histo_pending_command) ;
	//printf("histo_buf: external preset flag = %d\n", external_preset) ;
	//printf("histo_buf: histo_tag_status = %d\n", histo_tag_status) ;
	//printf("histo_buf: online-flag = %d\n", histo_st->online_flag) ;

   if (GS_histo_pending_command == HISTO_STOP_PENDING
	   || external_preset == pCB->Stop)
   {
	   //printf("histo_buf: complete, STOP signal pending, stopping ...\n") ;
	   //printf("histo_buf: external preset = %d\n", external_preset) ;
	   GS_histo_state = HISTO_STOPPED ;	// update histogramming state
	   GS_MedCompFlag = pCB->Stop ;
	   pCB->acqComplete(GS_MedCompFlag) ;
	   //printf("histo_buf: set event to spawn to store\n") ;
	   SetEvent(hWriteEvent) ;			// trigger to spawn to store
	   return(TAG_END_OF_SCAN) ;
   }
   else if (GS_histo_pending_command == HISTO_ABORT_PENDING
	   || external_preset == pCB->Abort)
   {
	   //printf("histo_buf: complete, ABORT signal pending, aborting ...\n") ;
	   //printf("histo_buf: external preset = %d\n", external_preset) ;
	   GS_histo_state = HISTO_ABORTED ;
	   GS_MedCompFlag = pCB->Abort ;
	   pCB->acqComplete(GS_MedCompFlag) ;
	   return(TAG_END_OF_SCAN) ;
   }
   else if (external_preset == pCB->Preset)
   {
	   //printf("histo_buf: frame complete on external PRESET\n") ;
	   GS_MedCompFlag = pCB->Preset ;
	   pCB->acqComplete(GS_MedCompFlag) ;
	   SetEvent(hWriteEvent) ;
	   GS_histo_state = HISTO_COMPLETE ;
	   return(TAG_END_OF_SCAN) ;
   }
   else if (GS_histo_state == HISTO_COMPLETE)
   {
	   //printf("histo_buf: frame complete on internal PRESET\n") ;
	   GS_MedCompFlag = pCB->Preset ;
	   pCB->acqComplete(GS_MedCompFlag) ;
	   if (frame_def_st->nframes == 1) SetEvent(hWriteEvent) ;
	   return(TAG_END_OF_SCAN) ;
   }
   else if ( frame_def_st->frame_criteron == FRAME_DEF_IN_NET_TRUES
		&& GS_em_prompts-GS_em_randoms >= frame_def_st->total_net_trues[frame_def_st->cur_frame])
   {
	    //printf("histo_buf: total net trues = %d\n", GS_em_prompts-GS_em_randoms) ;
		//printf("histo_buf: pre-defined count threshold = %d\n", frame_def_st->total_net_trues[frame_def_st->cur_frame]);
		//printf("histo_buf: frame complete on total net trues\n") ;

		// 23-Sept-2001: support stopping by counts
		GS_t1 = GetTickCount() ;
		//printf("HISTBuf:stopping:ellpased time = %d\n", GS_t1-GS_t0) ;
		int frame_number;
		//printf("frame number = %d\n", frame_def_st->cur_frame);
		frame_number = frame_def_st->cur_frame;
		frame_def_st->frame_duration[frame_number] = (GS_t1-GS_t0)/1000 ;

		GS_MedCompFlag = pCB->Preset ;
		pCB->acqComplete(GS_MedCompFlag) ;
		SetEvent(hWriteEvent) ;
		GS_histo_state = HISTO_COMPLETE ;
		return(TAG_END_OF_SCAN) ;
   }
   else
   {
	   //printf("histo_buf: listmode buffer processing done\n") ;
	   GS_histo_state = HISTO_WAIT ;	// histogramming not active, set to wait
	   return(i*sizeof(long)) ;
   }

}

/*****************************************************************************
*
*	histo_tag.c
*
*   Subroutine for master control of all list mode data stream tag words
*
*   Input arguments:
*
*   	  tagword - 32 bit representing and embedding the tagword type and value
*   	  tagtype - specifies the tagword type to look for
*   	  tagvalue - specifies the the tagword value to match
*
*   Updated input arguments :
*
*   	  tag_st - a structure defining all list mode tagword related information
*
*   Returns :
*
*		0 : no actions taken
*		1 : information logging and updating
*		2 : end-of-frame flag
*
*	NOTE (23-Aug-1999	P. Luk)
*		since there has been no consistent list mode stream with tagwords
*		supplied to me yet, this subroutine can only be in prototyping mode
*		and status.
*
*		only timing tagwords and block singles tagwords are supported on an
*		experimental basis, as of August-1999
*
*******************************************************************************/

#define SCAN_TIME_TAG_MASK 0x80000000
#define SCAN_TIME_VALUE_MASK 0x7fffffff
#define SINGLE_TAG_MASK 0xa0000000
#define DETBLK_NUMBER_MASK 0x1ff80000
#define SINGLE_RATE_MASK 0x0007ffff

int histo_tag(	struct _tag_st *tag_st,
				unsigned long tagword,
				struct _frame_def_st *frame_def_st)
{

   __int64 scan_time ;// ZYB
   int	tagtype ;
   int	end_of_frame=0 ;
   int	detblk_number ;
   int	cur_frame ;
   int	nframes ;
   //int	cur_frame_start, cur_frame_duration, cur_frame_stop ;
   // ZYB
   __int64	cur_frame_start, cur_frame_duration, cur_frame_stop ;

   float	uncor_singles_rate ;
   unsigned long	ltemp ;
   _block_singles_st *block_singles ;

   // first of all, need to determine what kind of tag word it is
   ltemp = (unsigned long) tagword >> 29 ;
   if ( ltemp == 4)
	  tagtype = TAG_SCAN_TIME ;
   else if ( ltemp == 5)
   	  tagtype = TAG_SCAN_SINGLE ;
   else
   	  tagtype = TAG_UNKNOWN ;

   // handles timing tag words,
    // also determines frame switching based on ellapsed time-num of time ticks
   if ( tagtype == TAG_SCAN_TIME)
   {
	  ++GS_nticks ;
	  scan_time = GS_nticks ;
      if (scan_time > 0) tag_st->ellapsed_time = scan_time;//ZYB / 1000 ;
   	  cur_frame = frame_def_st->cur_frame ;
   	  nframes = frame_def_st->nframes ;
      if (frame_def_st->frame_criteron == FRAME_DEF_IN_SECONDS && cur_frame < nframes)
      {
		 end_of_frame = 0 ;
		 cur_frame_start = frame_def_st->frame_start_time[cur_frame] ;
		 cur_frame_duration = frame_def_st->frame_duration[cur_frame] ;
		 cur_frame_stop = cur_frame_start + cur_frame_duration ;
		 //if ((int) scan_time >= cur_frame_stop *1000)
		 // ZYB
		 if (scan_time >= cur_frame_stop *1000)
         {
			printf("\nFRAME SWITCHING: time-tag = %d, predefined time = %d\n",
						scan_time, cur_frame_stop) ;
			printf("current frame = %d, nframes = %d\n", cur_frame, nframes) ;
         	end_of_frame=1 ;
            frame_def_st->frame_switch_time[cur_frame] = (long)(scan_time/1000) ;
			printf("frame switch time = %d\n",
				frame_def_st->frame_switch_time[cur_frame]) ;
         }
      }
      if (end_of_frame) return(TAG_END_OF_FRAME) ;
      else return(TAG_TALLIED) ;
   }

   // ..need to add code to change frame via other criteria, e.g. bed position, etc
   //		also need to add code to compute singles and other tagword-related
   // 	computation

   unsigned long usltemp;

   // code to extract and compute detector block singles count rate
   if (tagtype == TAG_SCAN_SINGLE)
   {
   	  //printf("block singles tagword encountered ..\n") ;
	  ++tag_st->nsingles_tags ;
   	  block_singles = &(tag_st->block_singles_st) ;
   	  detblk_number = (tagword & DETBLK_NUMBER_MASK) >> 19 ;
      //printf("detector block number = %x, %d\n", detblk_number, detblk_number) ;
      if (detblk_number >= 0 && detblk_number < block_singles->ndetblks)
      {

		  ++block_singles->nsamples[detblk_number] ;
		  //uncor_singles_rate = (float) (tagword & SINGLE_RATE_MASK) ;
		  usltemp = (unsigned long) tagword & SINGLE_RATE_MASK;
		  //uncor_singles_rate = (float) (usltemp << 2);	// 17May2002, PL
		  uncor_singles_rate = (float) usltemp;	
		  if (pSI->gantry == HRP_SCANNER)
		  {
			block_singles->uncor_rate[detblk_number] = uncor_singles_rate ;
			//printf("hrp: blk = %d, srate = %f, tagw = %x\n", detblk_number, uncor_singles_rate, tagword);
		  }
		  else
		  {
			block_singles->uncor_rate[detblk_number] += uncor_singles_rate ;
		    GS_TotalSinglesRate += (__int64)uncor_singles_rate ;
		  }
		  ++GS_NumSinglesSamples ;
		  return(TAG_TALLIED) ;
      }
   }

   return(TAG_NOACTION) ;
}

/******************************************************************************
*
*	frame_write.c
*
*		C subroutine to write sinogram frame to disk. Performs the following :
*
*      1. creates and writes the emission binary histogram file
*	   2. tally block singles in preparation for ASCII header file creation
*      3. creates and writes the emission ASCII header file
*	   4. Optional, repeat steps 1 and 3 above for transmission, if applicable
*      5. updates the relevant data structure to prepare for next frame
*
*	Returns:
*			0	OK
*		or	-7	HISTO_OUT_FILE_CREATION_ERROR
*		or	-8	HISTO_WRITE_ERROR
*		or	-9	HISTO_HEADER_CREATION_ERROR
*	
*
*	NOTE (23-Aug-1999	P. Luk)
*			since the 8.0 sinogram and the interface and interaction with
*		the client DB have not been discussed and defined, this subroutine
*		is in prototyping status. It will be splitted up into more subroutines
*		as the definition is finalized
*      
*********************************************************************************/

int frame_write(
		 struct _histo_st *histo_st,
         struct _tag_st *tag_st,
         struct _frame_def_st *frame_def_st)

{
   int	frame_number=0 ;
   int	em_fd = 0 ;
   int	tx_fd = 0 ;
   char *start_offset ;
   int	nwritten ;
   int	status ;
   unsigned len ;
   DWORD t0, t1 ;

   printf("Entering subroutine frame_write ...\n") ;
   printf("frame_write: emission sino-filename = %s\n", histo_st->em_filename) ;
   printf("frame_write: transmission sino-filename = %s\n", histo_st->tx_filename) ;
   frame_number = frame_def_st->cur_frame ;
   printf("frame_write: current frame = %d\n", frame_number) ;
   printf("frame start time = %d\n", frame_def_st->frame_start_time[frame_number]);
   printf("frame end time = %d\n", frame_def_st->frame_switch_time[frame_number]);
   printf("frame duration = %d\n", frame_def_st->frame_duration[frame_number]);

   _fmode = O_BINARY ; 

   // STEP 1a: creates the binary file for histogarm data output
   if (histo_st->acqm != TX_ONLY)
		em_fd = creat(histo_st->em_filename, S_IREAD | S_IWRITE | O_BINARY);
   if (histo_st->acqm != EM_ONLY)
		tx_fd = creat(histo_st->tx_filename, S_IREAD | S_IWRITE | O_BINARY);

   if (em_fd == -1)
   {
	  printf("frame_write1:ERROR: failed to create file '%s' for writing\n",
			histo_st->em_filename) ;
	  printf("frame_write1:ERROR: system message : %s\n", sys_errlist[errno]) ;
      return(HISTO_OUT_FILE_CREATION_ERROR) ;
   }
   if (tx_fd == -1)
   {
	  printf("frame_write1:ERROR: failed to create file '%s' for writing\n",
			histo_st->tx_filename) ;
	  printf("frame_write1:ERROR: system message : %s\n", sys_errlist[errno]) ;
	  if (em_fd > 0) close(em_fd) ;
      return(HISTO_OUT_FILE_CREATION_ERROR) ;
   }

   // STEP 1b: writes emission binary data to disk
   if (histo_st->acqm != TX_ONLY)
   {
	  len = (unsigned) (histo_st->max_em_offset*sizeof(short));
      printf("frame_write: writing emission data: %d bytes to disk ...\n", len) ;
      t0 = GetTickCount() ;
	  start_offset = (char *) (histo_st->sino_base + histo_st->emp_offset) ;
	  printf("frame_write: EM sino-offset = %d\n", start_offset) ;
      nwritten = write(em_fd, start_offset, len) ;
      t1 = GetTickCount() ;
      printf("frame_write: time to write %d bytes = %d msec\n", len, t1-t0) ;
      if (nwritten == -1)
	  {
	     printf("frame_write2:ERROR: Failed to write %d bytes to %s\n", len, histo_st->em_filename) ;
	     printf("frame_write2:ERROR: System Message: %s\n", sys_errlist[errno]) ;
	  }
      else if (nwritten != (int) len)
	  {
	     printf("frame_write3:ERROR: %d bytes request, %d bytes written, PROBABLY disk full\n",
				len, nwritten) ;
	     printf("frame_write3:ERROR: System Message: %s\n", sys_errlist[errno]) ;
	  }

	  if (nwritten != (int) len)
	  {
		  close(em_fd);
		  return(HISTO_WRITE_ERROR) ;
	  }
	  if (histo_st->em_sinm == NET_TRUES || histo_st->em_sinm == PROMPTS_ONLY) close(em_fd);

	  if (histo_st->em_sinm == PROMPTS_AND_RANDOMS)
	  {
		len = (unsigned) (histo_st->max_em_offset*sizeof(short));
		printf("frame_write: writing emission randoms: %d bytes to disk ...\n", len) ;
		t0 = GetTickCount() ;
		printf("frame_write: sino-base = %d\n", histo_st->sino_base);
		printf("frame_write: emr_offset = %d\n", histo_st->emr_offset);
		start_offset = (char *) (histo_st->sino_base + histo_st->emr_offset) ;
		printf("frame_write: EM sino-offset = %d\n", start_offset) ;
		nwritten = write(em_fd, start_offset, len) ;
		t1 = GetTickCount() ;
		printf("frame_write: time to write %d bytes = %d msec\n", len, t1-t0) ;
		if (nwritten == -1)
		{
			printf("frame_write2:ERROR: Failed to write %d bytes to %s\n", len, histo_st->em_filename) ;
			printf("frame_write2:ERROR: System Message: %s\n", sys_errlist[errno]) ;
		}
		else if (nwritten != (int) len)
		{
			printf("frame_write3:ERROR: %d bytes request, %d bytes written, PROBABLY disk full\n",
				len, nwritten) ;
			printf("frame_write3:ERROR: System Message: %s\n", sys_errlist[errno]) ;
		}
		close(em_fd);
	  }

	  // return with error code to caller, if system call failed
	  if (nwritten != (int) len) return(HISTO_WRITE_ERROR) ;
   }
  
   // STEP 2: tallying the block singles rate
   int i ;
   _block_singles_st *block_singles ;

   block_singles = &(tag_st->block_singles_st) ;
   printf("total number of detector blocks = %d\n", block_singles->ndetblks) ;
   printf("Total singles samples = %d\n", GS_NumSinglesSamples);

   if (pSI->gantry == HRP_SCANNER)	// for conventional block detector scanners
   {
		printf("Block detector scanners\n");
		GS_TotalSinglesRate = 0;
		for (i=0; i<block_singles->ndetblks; i++)
			GS_TotalSinglesRate += (__int64)block_singles->uncor_rate[i];
		block_singles->total_uncor_rate = (long) GS_TotalSinglesRate;

		printf("total (from all blocks) average (over time) uncorrected singles = %d\n",
			  block_singles->total_uncor_rate);
   }
   else	// for panel detector class of scanners
   {
		  
		printf("Panel detector scanners:\n");
		long total_from_summing_blocks = 0;
		for (i=0; i<block_singles->ndetblks; i++)
		{
			block_singles->uncor_rate[i] /= (float) block_singles->nsamples[i] ;
			total_from_summing_blocks += (long) block_singles->uncor_rate[i];
		}
		printf("total singles rate from summing blocks = %d\n", total_from_summing_blocks);
		// for HRRT, singles are not average over time per sample, they are independent.
		// Also Mark Lenox & Koln would prefer average (over blocks) singles per block
		// rather than total average (over sampling duration) singles from all blocks
		// S Tolbert 06 Sept 02 - prevent division by 0
		long average_singles_rate;
		if (GS_NumSinglesSamples == 0)
			average_singles_rate = 0;
		else
			//average_singles_rate = (long) GS_TotalSinglesRate/GS_NumSinglesSamples;
			average_singles_rate = (long) (GS_TotalSinglesRate/GS_NumSinglesSamples);

		printf("average singles rate over all blocks = %d\n", average_singles_rate);


		block_singles->total_uncor_rate = average_singles_rate;
   }

   // STEP 3: writing emission sinogram header file to disk
   if (histo_st->acqm != TX_ONLY)
   {  
	  //char header_filename[2048] ;	// S Tolber 19 Sept 02 - increase buffer size
	  char header_filename[4096] ;
	  strcpy(header_filename, histo_st->em_filename) ;
	  strcat(header_filename, ".hdr") ;

	  // writes Interfile header for emission data
	  if (pSI->gantry == HRRT_SCANNER)
		  status = write_interfile_header(EM_ONLY, histo_st, tag_st, frame_def_st) ;
	  else
		  status = write_interfile_header2(header_filename, frame_number, EM_ONLY, histo_st, tag_st, frame_def_st) ;

	  if (status != OK)
	  {
		  printf("frame_write: error in creating Interfile file: %s\n", header_filename);
		  return(HISTO_HEADER_CREATION_ERROR);
	  }
   }

   // STEP 4: optionally writes transmission binary data and header to disk
   if (histo_st->acqm != EM_ONLY)
   {
      //printf("frame_write: writing %d bytes to disk ...\n", len) ;
	  len=(unsigned) ( histo_st->max_tx_offset*sizeof(short) );
      printf("frame_write: writing transmission data %d bytes to disk ...\n", len) ;
      t0 = GetTickCount() ;
	  printf("sino-base = %d\n", histo_st->sino_base) ;
	  printf("offset added = %d\n", histo_st->txp_offset * sizeof(short)) ;
	  start_offset = (char *)(histo_st->sino_base + histo_st->txp_offset) ;
	  printf("frame_write: TX sino-offset = %d\n", start_offset) ;
      nwritten = write(tx_fd, start_offset, len) ;
      t1 = GetTickCount() ;
      printf("frame_write: time to write %d bytes = %d msec\n", len, t1-t0) ;
      if (nwritten == -1)
	  {
	     printf("frame_write2:ERROR: Failed to write %d bytes to %s\n", len, histo_st->tx_filename) ;
	     printf("frame_write2:ERROR: System Message: %s\n", sys_errlist[errno]) ;
	  }
      else if (nwritten != (int) len)
	  {
	     printf("frame_write3:ERROR: %d bytes request, %d bytes written, PROBABLY disk full\n",
				len, nwritten) ;
	     printf("frame_write3:ERROR: System Message: %s\n", sys_errlist[errno]) ;
	  }

	  // return with error code to caller, if system call failed
	  if (nwritten != (int) len)
	  {
		  close(tx_fd);
		  return(HISTO_WRITE_ERROR) ;
	  }
	  if (histo_st->tx_sinm == NET_TRUES || histo_st->tx_sinm == PROMPTS_ONLY)
	      close (tx_fd) ;

	  // now writes the transmission randoms array
	  if (histo_st->tx_sinm == PROMPTS_AND_RANDOMS)
	  {
		len=(unsigned) ( histo_st->max_tx_offset*sizeof(short) );
		printf("frame_write: writing transmission data %d bytes to disk ...\n", len) ;
		t0 = GetTickCount() ;
		printf("sino-base = %d\n", histo_st->sino_base) ;
		printf("offset added = %d\n", histo_st->txr_offset * sizeof(short)) ;
		start_offset = (char *)(histo_st->sino_base + histo_st->txr_offset) ;
		printf("frame_write: TX sino-offset = %d\n", start_offset) ;
		nwritten = write(tx_fd, start_offset, len) ;
		t1 = GetTickCount() ;
		printf("frame_write: time to write %d bytes = %d msec\n", len, t1-t0) ;
		if (nwritten == -1)
		{
			printf("frame_write2:ERROR: Failed to write %d bytes to %s\n", len, histo_st->tx_filename) ;
			printf("frame_write2:ERROR: System Message: %s\n", sys_errlist[errno]) ;
		}
		else if (nwritten != (int) len)
		{
			printf("frame_write3:ERROR: %d bytes request, %d bytes written, PROBABLY disk full\n",
				len, nwritten) ;
			printf("frame_write3:ERROR: System Message: %s\n", sys_errlist[errno]) ;
		}
		close(tx_fd);
		// return with error code to caller, if system call failed
		if (nwritten != (int) len) return(HISTO_WRITE_ERROR) ;
	  }

	  //char header_filename[2048] ;	// S Tolbert 19 Sept 02 - increased buffer size
	  char header_filename[4096] ;
	  strcpy(header_filename, histo_st->tx_filename) ;
	  strcat(header_filename, ".hdr") ;

	  // writes Interfile header for transmission data
	  if (pSI->gantry == HRRT_SCANNER)
		  status = write_interfile_header(TX_ONLY, histo_st, tag_st, frame_def_st) ;
	  else
	      status = write_interfile_header2(header_filename, frame_number, TX_ONLY, histo_st, tag_st, frame_def_st) ;
   		
	  if (status != OK)
	  {
		  printf("frame_write: error in creating Interfile file: %s\n", header_filename);
		  return(HISTO_HEADER_CREATION_ERROR);
	  }

   } // end of block of code to write transmission data to disk

   // reset memory for next frame, if necessary
   if (frame_def_st->nframes > 1) memset((char *)histo_st->sino_base, 0, len) ;

   // STEP 5, current frame is done, now prepares for next frame

   printf("frame_write: current pass number = %d\n", histo_st->pass) ;
   printf("frame_write: total number of passes = %d\n", histo_st->npasses) ;
   if (histo_st->pass == histo_st->npasses-1) // conditional for multiple passes
   {
      frame_def_st->total_em_prompts[frame_number] = GS_em_prompts ;
      frame_def_st->total_em_randoms[frame_number] = GS_em_randoms ;
      frame_def_st->total_tx_prompts[frame_number] = GS_tx_prompts ;
      frame_def_st->total_tx_randoms[frame_number] = GS_tx_randoms ;
	  printf("current frame number = %d\n", frame_def_st->cur_frame) ;
	  printf("number of frames = %d\n", frame_def_st->nframes) ;
   	  if (frame_def_st->cur_frame < frame_def_st->nframes-1)
      {
      	GS_em_prompts=0 ;
      	GS_em_randoms=0 ;
      	GS_tx_prompts=0 ;
      	GS_tx_randoms=0 ;
		GS_old_em_prompts=0 ;
		GS_old_em_randoms=0 ;
		GS_old_tx_prompts=0 ;
		GS_old_tx_randoms=0 ;
      }
   }

   // code to reset block singles stuff. PL 22-Jan-99
   
   printf("frame_write: resetting tag_struct for next frame...\n") ;
   printf("frame_write: resetting %d bytes to zero\n", sizeof(*tag_st)) ;
   memset(tag_st, 0, sizeof(*tag_st)) ;
   tag_st->block_singles_st.ndetblks = histo_st->ndetblks ;

   printf("frame_write: frame %d COMPLETED\n\n", frame_number) ;
   
   return(OK) ;
}

/*****************************************************************

  The following are service routines perform the actual tasks
  to support functionality requested by the API calls:

		set_attributes
		set_sinogram_mode
		unconfig
		config
		define_frames

	and for internal debugging purposes

		show_frames
		show_attributes

********************************************************************/


/***********************************************************************
*
*	set_attributes - performs the task requested by HISTSetAttributes API
*
*
*	Returns:
*			1	MedComp_OK
*			0	MedComp_ERR
*
************************************************************************/
int set_attributes(struct _histo_st *histo_st,
						  int np,
						  int nv,
						  int n_em_sinos,
						  int n_tx_sinos)
{
  printf("Entering set_attributes ...\n") ;

	// validating input parameters
     //if (np != 256 && np != 288) return(MEDCOMP_ERR) ;
	 //if (nv != 128 && nv != 288 && nv != 144) return(MEDCOMP_ERR) ;
	if (n_em_sinos < 0 || n_em_sinos > 10000) return(MEDCOMP_ERR) ;
	if (n_tx_sinos < 0 || n_tx_sinos > 10000) return(MEDCOMP_ERR) ;

	printf("set_attributes: sinogram parameters are definable\n") ;

	histo_st->nprojs = np ;
	histo_st->nviews = nv ;
	histo_st->n_em_sinos = n_em_sinos ;
	histo_st->n_tx_sinos = n_tx_sinos ;
	return(MEDCOMP_OK) ;
}


/**********************************************************************
*
*	set_sinogram_mode - performs the task requested by HISTSinoMode API
*
*	Returns:
*			1	MedComp_OK
*			0	MedComp_ERR
*
************************************************************************/
int set_sinogram_mode(struct _histo_st *histo_st,
					   int	em_sinm,
					   int	tx_sinm,
					   int	acquisition_mode)
{
  printf("Entering set_sinogram_mode ...\n") ;

    if (em_sinm != NET_TRUES && em_sinm != PROMPTS_AND_RANDOMS)
		return(MEDCOMP_ERR) ;

    if (tx_sinm != NET_TRUES && tx_sinm != PROMPTS_AND_RANDOMS)
		return(MEDCOMP_ERR) ;

	histo_st->em_sinm = em_sinm ;
	histo_st->tx_sinm = tx_sinm ;
	histo_st->acqm = acquisition_mode;
	return(MEDCOMP_OK) ;
}

/*****************************************************************
*	unconfig - performs the task requested by HISTUnconfig API:
*
*	- frees previously allocated histogram array, if applicable
*	- resets histogrammer control structures
*	- resets global static variables	
*******************************************************************/
void unconfig(struct _histo_st *histo_st,
					struct _frame_def_st *frame_def_st,
					struct _tag_st *tag_st)
{
	printf("Entering unconfig ...\n") ;

	  // frees previously allocated histogram array, if applicable
	  printf("unconfig: histo-sinobase = %d\n", histo_st->sino_base) ;
	  if (histo_st->sino_base != NULL)
	  {
		   // S Tolbert 01 Oct 02 - use histo_deallocate function
		   if (!histo_deallocate((LPVOID) sbuf, ALLOC_win32Virtual))
			  printf("unconfig: ERROR in deallocating histogram memory\n");	
		   else
		      printf("unconfig: histogram memory deallocated\n");
		  //free(histo_st->sino_base) ;
	  }

	  // reset control structures
      memset(histo_st, 0, sizeof(struct _histo_st)) ;
      memset(frame_def_st, 0, sizeof(struct _frame_def_st)) ;
      frame_def_st->nframes = 0 ;
      memset(tag_st, 0, sizeof(struct _tag_st)) ;
	  frame_def_st->cur_frame = 0 ;

	  // resets global static variables for statistics
	   // these are moved to be done at Config
	  //GS_em_prompts = GS_em_randoms = GS_tx_prompts = GS_tx_randoms = 0 ;
	  //GS_old_em_prompts = GS_old_em_randoms = GS_old_tx_prompts = GS_old_tx_randoms = 0;
	  //GS_TotalSinglesRate = 0 ;
	  //GS_NumSinglesSamples = 0 ;
	  //GS_histo_pending_command = HISTO_NO_PENDING_COMMANDS ;
	  //GS_nticks = 0 ;

	  GS_histo_state = HISTO_UNCONFIGURED ;

	  strcpy(GS_error_message, "") ;
}


/******************************************************************************
*	config - performs tasks requested by HISTConfig API
*
*	- check to made sure the execution threads for async response are ready
*	- copy filen name prefix(es) into the histogram C-structure
*	- determine is its online or offline, based on HISTConfig input arguments
*	- derive acquisition mode from basic API definition
*	- assess current memory status of computer to derive usable physical memory
*	- based on the above and the histogram definition, estimate the number of
*		passess required to perform histogramming without extensive page faults
*	- check to ensure if online histogramming, it can be done in one passes;
*		if not, return error
*	- allocate and initialize histogram memory for first pass
*	- update the configuration-derived info into the histogram C-structure
*	- check to ensure there is sufficient disk space for this histogram session
*******************************************************************************/
int config(struct _histo_st *histo_st,
	   struct _frame_def_st *frame_def_st,
	   struct _tag_st *tag_st,
	   char *SinoFileName,
	   char *ListFname)

{
   char	*filenames_delimiter = "+" ;
   int  sinoBinSize = sizeof(short int) ;
   int	nprojs, nviews ;
   int	totalNumSino ;
   int	totalSinoSize ;
   int	acquisition_mode ;
   int	listmodeBufSize = 4*1024*1024 ;
   int	estMemOverHead = 4*1024*1024 ;
   int	netUsableMem ;
   int	totalRequireMem ;
   int	nbytes_per_pass ;
   int	status ;
   int	i ;
   MEMORYSTATUS ms ;

  printf("Entering config ...\n") ;
  printf("sino-filename = %s\n", SinoFileName) ;
  printf("listmode-filename = %s\n", ListFname) ;

	// check for viability - cannot configure without a frame definition
   if (frame_def_st->nframes < 1)
   {
	   printf("cannot configure histogrammer without frame definition\n") ;
	   return(ERR) ;	// need to define a more specific error code later
   }

	// check for viability - ONLY supports static scans, for now
	if (frame_def_st->nframes > 1)
	{
		printf("cannot configure histogrammer for more than one frame\n") ;
		return(ERR) ;	// need to define a more specific error code later
	}

   // check to make sure the Thread for Asynchronous response are ready
   printf("config: hWriteThread = %x\n", hWriteThread) ;
   if (hWriteThread == NULL)
   {
		status = init_frame_write() ;
		if (status == OK) printf("config: Thread to write frames created\n") ;
		else
		{
			printf("config: FAILED to create thread to write frames\n") ;
			return(ERR) ;
		}	
   }

   // copy listmode file name into histogram control structure
   histo_st->list_file = ListFname ;
   
   // determine if it is online or offline histogramming
   if (histo_st->list_file == NULL) histo_st->online_flag = 1 ;
   else histo_st->online_flag = 0 ;

   printf("config: online-flag = %d\n", histo_st->online_flag) ;

   // if off-line histogram, check to make sure Off-line thread is setup
   if (!histo_st->online_flag)
   {
		printf("config: hReplayThread = %d\n", hReplayThread) ;
		if (hReplayThread == NULL)
		{
		   status = init_offline() ;
		   if (status == OK) printf("config: Thread for Offline-histogram created\n") ;
		   else
		   {
			  printf("config: FAILED to create thread for Offline-histo\n") ;
			  return(ERR) ;
		   }
		}
   }

   // derive acquisition mode from basic sinogram parameters
   nprojs = histo_st->nprojs ;
   nviews = histo_st->nviews ;
   histo_st->max_em_offset = nprojs * nviews * histo_st->n_em_sinos ;
   histo_st->max_tx_offset = nprojs * nviews * histo_st->n_tx_sinos ;

   // previously, acquisition mode is derived here. Won't work with the
   // new rebinner LUT anymore. Acquisition mode is now specified
   // explicitly in the API HISTSinoMode
   /****
   if (histo_st->max_tx_offset == 0) acquisition_mode = EM_ONLY ;
   else if (histo_st->max_em_offset == 0) acquisition_mode = TX_ONLY ;
   else acquisition_mode = SIMU_EM_TX ;
   histo_st->acqm = acquisition_mode ;
   ****/
   acquisition_mode = histo_st->acqm;
   printf("config: acquisition mode = %d\n", acquisition_mode) ;

   // if simultaneous mode, parse API input string to extract 2 file names
    // one for emission, one for transmission
   if (acquisition_mode != SIMU_EM_TX)
   {
	   printf("config: copying filename ...\n") ;
	   if (acquisition_mode == EM_ONLY) histo_st->em_filename = SinoFileName ;
	   else histo_st->tx_filename = SinoFileName ;
   }
   else
   {
	   printf("config: parsing simu-em & tx filenames ...\n") ;
	   histo_st->em_filename = strtok(SinoFileName, filenames_delimiter) ;
	   printf("emission file name = %s\n", histo_st->em_filename) ;
	   if (histo_st->em_filename == NULL) return(ERR) ;
	   histo_st->tx_filename = strtok(NULL, filenames_delimiter) ;
	   printf("transmission file name = %s\n", histo_st->tx_filename) ;
	   if (histo_st->tx_filename== NULL)
	   {
		   printf("config: transmission filename not specified in simu-em&tx mode\n") ;
		   return(ERR) ;
	   }
	   if (strcmp(histo_st->em_filename, histo_st->tx_filename) == 0)
	   {
		   printf("config: emission and transmission has identical filenames\n") ;
		   return(ERR) ;
	   }
   }
   printf("config: emission sinogram filename = %s\n", histo_st->em_filename);
   printf("config: transmission sinogram filename = %s\n", histo_st->tx_filename);

   // estimate net usable physical memory for histogram process

   GlobalMemoryStatus(&ms) ;
   printf("config: total system physical memory = %d MB\n", ms.dwTotalPhys/1024/1024) ;
   printf("config: available and usable physical memory = %d\n", ms.dwAvailPhys/1024/1024);
   if (histo_st->usablePhyMem == 0)
	   histo_st->usablePhyMem = ms.dwAvailPhys - listmodeBufSize - estMemOverHead ;
   printf("config: estimated net available memory for histogram in MB = %d\n",
		histo_st->usablePhyMem/1024/1024) ;
   
   // estimate number of passes for histogram process
   int nEmSets;
   netUsableMem = histo_st->usablePhyMem ;

   totalNumSino = 0;
   if (histo_st->acqm != TX_ONLY)
   {
		nEmSets = 1;
		if (histo_st->em_sinm == PROMPTS_AND_RANDOMS) nEmSets = 2;
		totalNumSino = histo_st->n_em_sinos * nEmSets;
		printf("config: total number of emission sinograms = %d\n", totalNumSino);
		//totalNumSino = histo_st->n_em_sinos * (histo_st->em_sinm+1) ;
   }
   int nTxSets;
   if (histo_st->acqm != EM_ONLY)
   {
		nTxSets = 1;
		if (histo_st->tx_sinm == PROMPTS_AND_RANDOMS) nTxSets = 2;
		totalNumSino += histo_st->n_tx_sinos * nTxSets ;
		printf("config: total number Em & Tx sinograms = %d\n", totalNumSino);
		//totalNumSino += histo_st->n_tx_sinos * (histo_st->tx_sinm+1) ;
   }

   totalSinoSize = totalNumSino * nprojs * nviews ;
   printf("config: total number of sinograms = %d\n", totalNumSino) ;
   printf("config: sinogram has %d projections, %d views\n", nprojs, nviews) ;
   printf("config: total sinogram memory required = %d\n", totalSinoSize * sizeof(short)) ;
   printf("config: total estimated overhead = %d\n", listmodeBufSize + estMemOverHead) ;
   totalRequireMem = totalSinoSize * sizeof(short) + listmodeBufSize + estMemOverHead;
   printf("config: total memory required = %d\n", totalRequireMem) ;
   printf("config: total memory required in MB = %d\n",
			totalRequireMem/1024/1024) ;
   printf("config: total physical memory available in MB = %d\n",
			netUsableMem/1024/1024) ;
   histo_st->npasses = totalRequireMem / netUsableMem + 1 ;
   printf("config: total histogram passes = %d\n", histo_st->npasses) ;

   histo_st->start_offsets[0] = 0 ;
   histo_st->end_offsets[0] = totalSinoSize ;

   //compute_bounds(netUsableMem, totalNumSino, nprojs, nviews, sizeof(short),
		//&histo_st->npasses, histo_st->start_offsets, histo_st->end_offsets) ;
   printf("config: number of passes required = %d\n", histo_st->npasses) ;

   // special edition for PetSpect
   if (histo_st->npasses > 1)
   {
	   printf("config: not enough physical memory to support one frame\n") ;
	   return(ERR) ;
   }

   // if online requested, check for viability
   if (histo_st->online_flag)
   {
	 if (histo_st->npasses > 1)
	 {
		strcpy(GS_error_message,
			   "config: physical memory shortage for online scan\n") ;
		printf(GS_error_message) ;
		return(ERR) ;
	 }
	 if (frame_def_st->nframes > 1)
	 {
		 strcpy(GS_error_message,
			    "config: online dynamic is not supported yet\n") ;
		 printf(GS_error_message) ;
		 return(ERR) ;
	 }
   }

   // print histogram process control information in histo-structure
   for (i=0 ; i<histo_st->npasses ; i++)
   {
		printf("config: pass, start-offset, end-offset = %d, %d, %d\n",
			i, histo_st->start_offsets[i], histo_st->end_offsets[i]) ;
   }

   // Allocate memory space for histogram array
   nbytes_per_pass = (histo_st->end_offsets[0] - histo_st->start_offsets[0]) * sizeof(short int) ;
   printf("config: total number of bytes per pass = %d\n", nbytes_per_pass) ;
   printf("config: histo-sinobase = %x\n", histo_st->sino_base) ;
   if (histo_st->sino_base == NULL)
   {
	   //histo_st->sino_base = (short *) malloc(nbytes_per_pass) ;
	   // S Tolbert 01 Oct 02 - use histo_allocate function
	   if (!histo_allocate((DWORD) nbytes_per_pass, ALLOC_win32Virtual))
	   {
		   printf("config: Error in allocating %d bytes of histogram memory\n", nbytes_per_pass);
		   return(ERR) ;	// need a more specific error code
	   }
	   else
		   histo_st->sino_base = sbuf;
   }
   if (histo_st->sino_base == NULL)
   {
	    strcpy(GS_error_message, "config: not enough usable physical memory\n") ;
		printf(GS_error_message) ;
		return(ERR) ;	// need a more specific error code
   }
   
   printf("config: sino-base address allocated by system = %x\n", histo_st->sino_base) ;
   memset(histo_st->sino_base, 0, nbytes_per_pass) ;
   printf("config: histogram memory initialized to zero\n") ;
   
   // update information in histo-structure in preparation for histogramming

   // new scheme to format the allocate histogram memory,
   // for the new rebinner LUT scheme (aug 2002)
   // this is the previous method to format the histogram space.
   // the new rebinner LUT (aug2002) requires a different format
   /****
   histo_st->emp_offset = 0 ;
   if (histo_st->em_sinm == PROMPTS_AND_RANDOMS)
      histo_st->emr_offset = histo_st->emp_offset + histo_st->max_em_offset ;
   else
	   histo_st->emr_offset = histo_st->emp_offset ;
   histo_st->txp_offset = histo_st->emp_offset + histo_st->emr_offset + histo_st->max_em_offset;
   if (histo_st->tx_sinm == PROMPTS_AND_RANDOMS)
	   histo_st->txr_offset = histo_st->txp_offset + histo_st->max_tx_offset ;
   else
	   histo_st->txr_offset = histo_st->txp_offset ;
   ****/

   // here is the new histogram memory formatting, for rebinner LUT Aug2002
   // All offsets, emp, emr, txp, txr:
   // are offseted from histogram base address: histo_st.sinoBase
   long currentHistoOffsetLimit = 0;
   // first, define the prompts (Em & Tx) offsets and set the "don't cares"
   if (histo_st->acqm == EM_ONLY)	// implies no transmission sinograms
   {
	   histo_st->emp_offset = 0;
	   histo_st->txp_offset = -1;	// don't care,
	   histo_st->txr_offset = -1;	 // tx-events shouldn't & won't be binned
	   currentHistoOffsetLimit = histo_st->max_em_offset;
   }
   if (histo_st->acqm == TX_ONLY)	// implies no emission sinograms
   {
	   histo_st->emp_offset = -1;	// don't care,
	   histo_st->emr_offset = -1;	 // em-events shouldn't & won't be binned
	   histo_st->txp_offset = 0;
	   currentHistoOffsetLimit = histo_st->max_tx_offset;
   }
   if (histo_st->acqm == SIMU_EM_TX)	// there are Em & Tx events
   {
	   histo_st->emp_offset = 0;
	   histo_st->txp_offset = histo_st->max_em_offset;
	   currentHistoOffsetLimit = histo_st->max_em_offset + histo_st->max_tx_offset;
   }
   // next define the randoms (Em & Tx) offsets
   if (histo_st->acqm != TX_ONLY)
		if (histo_st->em_sinm == NET_TRUES)
			histo_st->emr_offset = histo_st->emp_offset;
		else
		{
			//currentHistoOffsetLimit += histo_st->max_em_offset; // make room for the extra Em random
			histo_st->emr_offset = currentHistoOffsetLimit;
			currentHistoOffsetLimit += histo_st->max_em_offset;
		}
   if (histo_st->acqm != EM_ONLY)
	   if (histo_st->tx_sinm == NET_TRUES)
			histo_st->txr_offset = histo_st->txp_offset;
	   else
	   {
			histo_st->txr_offset = currentHistoOffsetLimit;
			currentHistoOffsetLimit += histo_st->max_tx_offset; // make romm for the extra Tx randoms
	   }
   // Histogram memory is now formatting according to acqm, em_sinm & tx_sinm
   // print out the results of the formatting - all the offsets defined
   printf("conf: histogram memory formatted\n");
   printf("acqm, em-sinm, tx-sinm = %d, %d, %d\n",
				histo_st->acqm, histo_st->em_sinm, histo_st->tx_sinm);
   printf("Allocate histogram memory base address = %x, %d\n",
				histo_st->sino_base, histo_st->sino_base);
   printf("emp offset = %d, heap address = %d\n",
	   histo_st->emp_offset, histo_st->sino_base+histo_st->emp_offset);
   printf("emr offset = %d, heap address = %d\n",
	   histo_st->emr_offset, histo_st->sino_base+histo_st->emr_offset);
   printf("txp offset = %d, heap address = %d\n",
	   histo_st->txp_offset, histo_st->sino_base+histo_st->txp_offset);
   printf("txr offset = %d, heap address = %d\n",
	   histo_st->txr_offset, histo_st->sino_base+histo_st->txr_offset);
   printf("currentHistoOffsetLimit = %d\n", currentHistoOffsetLimit);


   // resets global static variables for statistics
   GS_em_prompts = GS_em_randoms = GS_tx_prompts = GS_tx_randoms = 0 ;
   GS_old_em_prompts = GS_old_em_randoms = GS_old_tx_prompts = GS_old_tx_randoms = 0;
   GS_TotalSinglesRate = 0 ;
   GS_NumSinglesSamples = 0;
   GS_histo_pending_command = HISTO_NO_PENDING_COMMANDS ;
   GS_nticks = 0 ;

   // set values for block singles tallying

   histo_st->ndetblks = MAX_DETECTOR_BLOCKS ;
   if (pSI->gantry == HRP_SCANNER || pSI->gantry == GNT_P39) 
	   histo_st->ndetblks = header_st.ndetblks ;  // S Tolbert 19 Sept 02
   else if (pSI->gantry == HRRT_SCANNER) 
	   histo_st->ndetblks = 936 ;
   printf("config: total number of detector blocks = %d\n", histo_st->ndetblks) ;
   tag_st->block_singles_st.ndetblks = histo_st->ndetblks ;

   // add code to check disk space, only checks local disk spec, for now
   // For now, only for PETCT scanners, only the emission file disk requirement is checked
   // For P39 and other scanners, either checking disk space for both em/tx or only for tx
   // will have to be implemented.	13-May-2002, PL

   printf("config: checking disk space for this scan ...\n");
   LPCTSTR pdest;
   LPCTSTR pdest_unc;
   BOOL isUNCSpec = 0;
   char *histoFileName;

   histoFileName = histo_st->em_filename;
   if (histo_st->acqm == TX_ONLY) histoFileName = histo_st->tx_filename;

   // check if filename is UNC embedded
   printf("config: checking is em_filename in UNC format ...\n");
   if ( (pdest_unc = strstr(histoFileName, "\\\\")) != NULL)
   {
	   printf("config: em_filename in UNC format, currently config does not check UNC diskspace\n");
	   isUNCSpec = 1;
   }

   printf("config: em-filename spec NOT UNC, go ahead with system check for sufficient disk space\n");
   if ( !isUNCSpec)
   {

	 BOOL ok;
	 BOOL checkDefaultDisk = 0;
	 char diskdrive[128];
	 DWORD	bytesNeeded ;
	 ULARGE_INTEGER freeBytesAvailToCaller;
	 ULARGE_INTEGER totalBytesOnDisk;
	 ULARGE_INTEGER totalFreeBytesOnDisk;

     // check if file name has a local directory path spec
     pdest = strchr(histoFileName, ':') ;

     // if no local disk drive specified, assume it's the current drive running the executable
     if ( pdest == NULL)
	 {
		printf("config: no explicit disk specified, check default disk\n");
		checkDefaultDisk = 1;
	 }

     // now, work on the case with explicit local diskdrive spec
	  // extracting disk drive substring
     if ( pdest != NULL)
	 {
	   int pos;

	   printf("config: checking on disk space\n");
	   pos = pdest - histoFileName;
	   printf("config: delimiter ':' found at position %d\n", pos);
	   strncpy(diskdrive, histoFileName, pos+1);
	   diskdrive[pos+1] = 0;
	   printf("config: diskdrive string = %s\n", diskdrive);
	 }

	 bytesNeeded = (DWORD) totalRequireMem ;
	 printf("config: estaimated total disk space needed = %d bytes\n", bytesNeeded);

	 if (checkDefaultDisk)
		ok = GetDiskFreeSpaceEx(
				NULL,
				//diskdrive,
				&freeBytesAvailToCaller,
				&totalBytesOnDisk,
				&totalFreeBytesOnDisk);
	 else
		ok = GetDiskFreeSpaceEx(
				//NULL,
				diskdrive,
				&freeBytesAvailToCaller,
				&totalBytesOnDisk,
				&totalFreeBytesOnDisk);

	 if (!ok)
	 {
		  printf("config: Error in sinogram disk space estimation via GetDiskFreeSpaceEx\n");
		  return ERR;
	 }
	  
	 printf("config: free disk space available to caller = %I64d bytes\n", freeBytesAvailToCaller);
	 printf("config: total bytes on disk = %I64d\n", totalBytesOnDisk);
	 printf("config: total free bytes on disk = %I64d\n", totalFreeBytesOnDisk);

	 if (freeBytesAvailToCaller.QuadPart > bytesNeeded)
	 {
		 printf("config: total disk space needed = %d bytes, %f MB\n",
			bytesNeeded, (float) bytesNeeded/1024.0/1024.0);
		 printf("config: disk space OK\n") ;
	 }
	 else
	 {
		 printf("config: ERROR: insufficient disk space to hold histogram\n");
		 return ERR;
	 }
   } // end of If (!isUNCSpec) clause. i.e. the case for local disk filespec, implicit & explicit

   // if the procedure goes to this point, configure is OK
   printf("config: SUCCESS\n") ;
   GS_histo_state = HISTO_CONFIGURED ;

   return(OK) ;
}

/****************************************************************************
*	define_frames - performs tasks requested by HISTDefineFrames API
*
*	- check for consistency and compatibility of API input arguments
*	- if OK, check for consistency and compabibility with existing frame def
*	- derive internal histogram frame table information & update frame-def
*		C-structure
*******************************************************************************/
int	define_frames(struct _frame_def_st* frame_def_st,
						int nframes,
						int frame_def_code,
						long preset_value)
{
	int i, j ;
	int current_num_frames ;

	// check input arguments
	current_num_frames = frame_def_st->nframes ;
	if (nframes < 1) return(ERR) ;		// need a more specific error return code
	if (preset_value < 1)
	{
		printf("define_frames: ERROR: illegal frame preset value\n") ;
		return(ERR) ;
	}

	// check global and system compatibility
	if ( (nframes + current_num_frames) > MAX_NFRAMES )
	{
		printf("define_frames:ERROR: adding %d frames will exceed the limit of %d total frames\n",
				nframes, MAX_NFRAMES) ;
		return(ERR) ;
	}
	
	switch (frame_def_code) {
	  case FRAME_DEF_IN_NET_TRUES :
 
		printf("define_frames: defining frames in Total Net Trues Count...\n") ;
		frame_def_st->total_net_trues[0] = preset_value;
		frame_def_st->frame_criteron = FRAME_DEF_IN_NET_TRUES ;
		break ;

	  case FRAME_DEF_IN_SECONDS:

	    for (i=0 ; i<nframes ; i++)
		{
		   printf("define_frames: defining frames in seconds ...\n") ;
		   j = i + current_num_frames ; // dummy variable j is used to index frame table
		   if (j == 0)
		   {
			  frame_def_st->frame_start_time[0] = 0 ;
			  frame_def_st->frame_duration[0] = preset_value ;
		   }
		   else
		   {
			  frame_def_st->frame_start_time[j] =
				  frame_def_st->frame_start_time[j-1] + frame_def_st->frame_duration[j-1] ;
			  frame_def_st->frame_duration[j] = preset_value ;
		   }
		   printf("frame_def: frame %d, frame-start = %d, frame_dur = %d\n",
				j, frame_def_st->frame_start_time[j], frame_def_st->frame_duration[j]) ;
		}
		frame_def_st->frame_criteron = FRAME_DEF_IN_SECONDS ;
		break ;

	  default:
		printf("define_frames: ERROR: Unknown criteria to define frames\n") ;
		break ;
	}

	frame_def_st->nframes += nframes ;
	return(nframes) ;
}

/**************************************************************************
*	show_attributes - respond to histo_show_frames API
*
*		for internal debugging only
*
***************************************************************************/
void show_attributes( struct _histo_st histo_st )

{

   printf("histo-struct:\n") ;
   printf("nprojs = %d, nviews = %d\n", histo_st.nprojs, histo_st.nviews) ;
   printf("acqm = %d\n", histo_st.acqm) ;
   printf("em-sinm = %d, em-nsinos = %d\n", histo_st.em_sinm, histo_st.n_em_sinos) ;
   printf("tx-sinm = %d, tx-nsinos = %d\n", histo_st.tx_sinm, histo_st.n_tx_sinos) ;
   printf("histogram base address = %x\n", histo_st.sino_base) ;
   printf("total number of emission sinogram bins = %d\n", histo_st.max_em_offset) ;
   printf("total number of transmission sinogram bins = %d\n", histo_st.max_tx_offset) ;
   printf("emission prompt and randoms memory offset = %x, %x\n",
			histo_st.emp_offset, histo_st.emr_offset) ;
   printf("difference = %d\n", histo_st.emr_offset - histo_st.emp_offset) ;
   printf("transmission prompt and randoms memory offset = %x, %x\n",
		   histo_st.txp_offset, histo_st.txr_offset) ;
   printf("difference = %d\n", histo_st.txr_offset - histo_st.txp_offset) ;

}

/*********************************************************************
*	show frames - respond to histo_show_frames API
*
*		for internal debugging only
*
************************************************************************/
void show_frames( struct _frame_def_st frame_def_st )

{
   int i ;

   printf("showing frame-def struct ...\n") ;
   printf("number of frames defined = %d\n", frame_def_st.nframes) ;
   for (i=0 ; i<frame_def_st.nframes ; i++)
   {
	   printf("frame number = %d, start time = %d, duration = %d, switch_time = %d\n",
		i, frame_def_st.frame_start_time[i], frame_def_st.frame_duration[i], frame_def_st.frame_switch_time[i]) ;
   }  
}


/**********************************************************************
*
*	The following subroutines handle threads for asynchronous and
*	timer needs:
*
*		spawn_frame_write
*		spawn_histogram
*		init_frame_write
*		init_offline
*		TimerService
*
*************************************************************************/


/************************************************************************* 
*	spawn_frame_write()
*
*		control routine to invoke the DLL subroutine "frame_write"
*		triggered by an event
*
***************************************************************************/
void spawn_frame_write()

{
  int	status ;

    printf("spawn_frame_write: waiting for trigger-event\n") ;
	printf("spawn_frame_write: hWriteThread pseudo-handle = %x\n", hWriteThread) ;

	if (hWriteThread) WaitForSingleObject(hWriteEvent, INFINITE) ;

	printf("spawn_frame_write: triggered by event\n") ;

	status = frame_write(&histo_st,
						 &tag_st,
						 &frame_def_st) ;

	printf("spawn_frame_write: frame_write call return = %d\n", status);
	if (status == OK)
	{
		printf("spawn_frame_write: returning MEDCOMP_OK\n");
		GS_histo_state = HISTO_STORE_COMPLETE ;
		pCB->acqPostComplete(GS_MedCompFlag, MEDCOMP_OK) ;
	}
	else
	{
		printf("spawn_frame_write: returning MEDCOMP_ERR\n");
		GS_histo_state = HISTO_STORE_ERR ;
		pCB->acqPostComplete(GS_MedCompFlag, MEDCOMP_ERR) ;
	}
	if (hWriteThread != NULL) CloseHandle(hWriteThread);
	hWriteThread = NULL ;
}


/***********************************************************************
*
*	spawn_histogram()
*	
*	  control routine to invoke the DLL "histogram() triggered by
*	  an event
*
*************************************************************************/
void spawn_histogram()

{
  int	status ;
  //BOOL	ok ;

    printf("spawn_histogram: waiting for trigger event ...\n") ;
	printf("spawn_histogram: OfflineThread pseudo-handle = %x\n", hReplayThread) ;

	if (hReplayThread) WaitForSingleObject(hReplayEvent, INFINITE) ;
	printf("spawn_histogram: triggered by event\n") ;

	status = histogram() ;
	if (status >= OK) GS_histo_state = status ;		//12-Mar-2002, from ==OK to >=OK
	else GS_histo_state = HISTO_ERROR ;

	if (hReplayThread != NULL) CloseHandle(hReplayThread);
	hReplayThread = NULL ;

	ExitThread((DWORD) 0) ;
}


/******************************************************************
*
*	init_frame_write()
*
*		- create thread to spawn to write a frame to disk
*
**********************************************************************/
int init_frame_write()

{

	hWriteThread = CreateThread(NULL,
							0,
							(LPTHREAD_START_ROUTINE) spawn_frame_write,
							(LPVOID) NULL,
							0,
							&dhWriteThreadId) ;

	printf("init_frame_write: HANDLE for hWriteThread = %x\n", hWriteThread) ;
	if (!hWriteThread)
	{
		printf("init_frame_write: ERROR: Cannot create Thread to spawn histo-write\n") ;
		return(ERR) ;
	}

	return(OK) ;
}

/************************************************************************
*
*	init_offline
*
*		- create thread to spawn to perform offline histogramming
*
**************************************************************************/

int init_offline()
{

	hReplayThread = CreateThread(NULL,
							0,
							(LPTHREAD_START_ROUTINE) spawn_histogram,
							(LPVOID) NULL,
							0,
							&dhReplayThreadId) ;

	printf("init_offline: HANDLE for hReplayThread = %x\n", hReplayThread) ;
	if (!hReplayThread)
	{
		printf("init_offline: ERROR: Cannot create Thread to spawn Offline Histogramming\n") ;
		return(ERR) ;
	}
	printf("init_offline: histogram thread created & initialized\n") ;
	return(OK) ;
}

/****************************************************************************
*
*	TimerService()
*
*		- sets up NT waitable timer to polling and compute statistical info
*
******************************************************************************/
void TimerService()

{
  LARGE_INTEGER wTime ;		// use as input argument for SetWaitableTime
  int TimerIntervalSec = 2;	// set timer interval to 2 seconds
  __int64 wTime100NanoSec ;	// for SetWaitableTimer call, it operates in 100 ns
  DWORD	t0, t1 ;
  DWORD	total_uncor_singles_rate ;
  int	i ;

	printf("Entering TimerService Thread\n") ;

	// Code to set up timer for statistics and count monitoring
	wTime100NanoSec = (__int64) TimerIntervalSec * (__int64)10000000 ; // convert to 100 ns
	wTime100NanoSec = -wTime100NanoSec ;	// convert to relative time
	
	// put into a large integer
	wTime.LowPart = (DWORD) (wTime100NanoSec & 0xffffffff) ;
	wTime.HighPart = (DWORD) (wTime100NanoSec >> 32) ;

	printf("TimeService: polling loop setup and ready, waiting for trigger ...\n") ;

	while(TRUE)
	{

		printf("TimerService: waiting for online event to continue\n") ;
		WaitForSingleObject(hOnlineEvent, INFINITE) ;

		t0 = GetTickCount() ;
		SetWaitableTimer(hTimer, &wTime, 0, NULL, NULL, FALSE) ;
		WaitForSingleObject(hTimer, INFINITE) ;
		t1 = GetTickCount() ;

		printf("Timer went off in %d ms\n", t1 - t0) ;
		printf("timer: prompts = %d, previous-prompts = %d\n",
				GS_em_prompts, GS_old_em_prompts) ;
		printf("timer: randoms = %d, previous randoms = %d\n",
				GS_em_randoms, GS_old_em_randoms) ;
		
		GS_em_prompts_rate = 1000*(GS_em_prompts - GS_old_em_prompts)/(t1-t0);
		GS_em_randoms_rate = 1000*(GS_em_randoms - GS_old_em_randoms)/(t1-t0);
		GS_em_trues_rate = GS_em_prompts_rate - GS_em_randoms_rate ;
		
		
		printf("timer: em-prompts-rate = %d\n", GS_em_prompts_rate) ;
		printf("timer: em-randoms-rate = %d\n", GS_em_randoms_rate) ;
		printf("timer: em-trues-rate = %d\n", GS_em_trues_rate) ;
		
		// 23-Sep-01, compute instantaneous total singles rate just for HR+
		total_uncor_singles_rate = 0;
		for (i=0; i<histo_st.ndetblks; i++)
		{
			total_uncor_singles_rate += (DWORD) tag_st.block_singles_st.uncor_rate[i];
		}
		printf("HR+ computed total uncor-singles rate = %d\n", total_uncor_singles_rate) ;

		// MEDCOMP BE callback
		// put in actual HIST_stats call here ...
		pHS->prompts_rate = GS_em_prompts_rate;
		pHS->random_rate = GS_em_randoms_rate;
		pHS->net_trues_rate = GS_em_trues_rate;
		pHS->total_singles_rate = GS_em_prompts - GS_em_randoms;
		pHS->n_updates++;

		// preparation for next round
		GS_old_em_prompts = GS_em_prompts ;
		GS_old_em_randoms = GS_em_randoms ;
		GS_old_tx_prompts = GS_tx_prompts ;
		GS_old_tx_randoms = GS_tx_randoms ;
	}
}

// module to compute 3D Michelogram segement table
int michelogram(
		int num_det_rings,
		int	span,
		int ring_difference,
		struct _header_st *header_st)

{
	int nsegments ;
	int nplanes ;
	int nsinos ;

	if (num_det_rings <= 0) num_det_rings = 104 ;
	nsegments = (2*ring_difference+1)/span ;
	if (nsegments % 2 == 0) nsegments += 1 ;
	nplanes = 2*num_det_rings-1 ;
	nsinos = nsegments*nplanes-(nsegments-1)*(span*(nsegments-1)/2+1) ;

	// print statements for debugging
	printf("michelogram: nrings, span, rd = %d,%d,%d\n",
		num_det_rings, span, ring_difference) ;
	printf("michelogram: nsegs, nplns, nsinos = %d, %d, %d\n",
		nsegments, nplanes, nsinos) ;

	header_st->nsegments = nsegments ;
	header_st->segment_table[0] = nplanes ;
	if (nsegments == 1) return(OK) ;

	// now handle the 3D cases
	int seg, ntotal, i, j ;

	ntotal = nplanes ;
	seg = nplanes - (span+1) ;
	for (i=1; i<=2; i++) header_st->segment_table[i] = seg ;
	ntotal += 2*seg ;

	for (i=2; i<=(nsegments-1)/2; i++)
	{
		seg -= 2*span ;
		for (j=0; j<=1; j++) header_st->segment_table[2*i-1+j] = seg ;
		ntotal += 2*seg ;		// 14-Mar-2002, fix from seg to 2*seg
	}

	printf("michelogram: total sinos = %d\n", ntotal) ;
	int sum ;
	sum = 0 ;
	for (i=0; i<nsegments; i++)
	{
		printf("michelogram: segment %d, nsinos = %d\n", i, header_st->segment_table[i]) ;
		sum += header_st->segment_table[i] ;
	}
	printf("michelogram: total from segment table sum = %d\n", sum) ;

	return(OK) ;
}

// earlier version of sinogram interfile header creation. Keep here for
// backward compatibility maintenance for HRRT Cologne scanner
int write_interfile_header(
		int scan_type,
		struct _histo_st *histo_st,
		struct _tag_st *tag_st,
		struct _frame_def_st *frame_def_st)

{
char header_fname[2048] ;
int	frame_number ;
FILE *fptr ;

// ZYB
__int64	totalPrompts ;
__int64	totalRandoms ;
int	nEnergyWins=1 ;
int nDetRings ;
int nSinos ;
int sinoMode ;
int nDataType ;
int nDims=3 ;
int nBytesPerPixel=2 ;
long dataOffset[8] ;
double scaleFactor[4] ;
double hrrtPixelSize = 1.21875 ;
double ecamPixelSize = 2.14511 ;
char *pFname ;
char cPetDataType[1024] ;

	// assign value to vars that are scan-type (em or tx) specific
	if (scan_type == EM_ONLY)
	{
		pFname = histo_st->em_filename ;
		totalPrompts = GS_em_prompts ;
		totalRandoms = GS_em_randoms ;
		nSinos = histo_st->n_em_sinos ;
		sinoMode = histo_st->em_sinm ;
		dataOffset[0] = histo_st->emp_offset ;
		dataOffset[1] = histo_st->emr_offset ;
	}
	else if (scan_type == TX_ONLY)
	{
		pFname = histo_st->tx_filename ;
		totalPrompts = GS_tx_prompts ;
		totalRandoms = GS_tx_randoms ;
		nSinos = histo_st->n_tx_sinos ;
		sinoMode = histo_st->tx_sinm ;
		dataOffset[0] = histo_st->txp_offset ;
		dataOffset[1] = histo_st->txr_offset ;
	}

	// assign value to vars that are scanner specific
	if (pSI->gantry == ECAM_SCANNER)
	{
		scaleFactor[0] = ecamPixelSize ;
		scaleFactor[1] = 1.0 ;
		scaleFactor[2] = ecamPixelSize ;
		nDetRings = 84 ;
	}
	else		// else it's HRRT
	{
		scaleFactor[0] = hrrtPixelSize ;
		scaleFactor[1] = 1.0 ;
		scaleFactor[2] = hrrtPixelSize ;
		nDetRings = 104 ;
	}

	// assign value to vars that are scan-protocol specific
	if (pSI->scanType == EM_ONLY)
		strcpy(cPetDataType, "Emission") ;
	else if (pSI->scanType == TX_ONLY)
		strcpy(cPetDataType, "Transmission") ;
	else if (pSI->scanType == SIMU_EM_TX)
		strcpy(cPetDataType, "Simultaneous Emission + Transmission") ;
	else
		strcpy(cPetDataType, "Unknown") ;

	// assign value to vars that are data definition specific
	nDataType = 1 ;
	if (sinoMode == PROMPTS_AND_RANDOMS) nDataType = 2 ;

	  strcpy(header_fname, pFname) ;
   	  strcat(header_fname, ".hdr") ;
	  printf("creating header file %s\n", header_fname) ;
   	  fptr = fopen(header_fname, "w") ;
   	  if (fptr == NULL)
	  {
		 printf("frame_write4:ERROR: cannot create histo-header file '%s'\n", header_fname) ;
		 printf("frame_write4:ERROR: System Message: %s\n", sys_errlist[errno]) ;
      	 return(HISTO_HEADER_CREATION_ERROR) ;
	  }
	  fprintf(fptr, "!INTERFILE\r\n") ;

	  // save scanner model information
	  if (pSI->gantry == ECAM_SCANNER)
	  {
		fprintf(fptr, "!originating system := E.CAM\r\n") ;
	    fprintf(fptr, "%%Horizontal Bed Position (cm) := %f\r\n", pSI->bedInOut) ;
	    fprintf(fptr, "%%Vertical Bed Position (cm) := %f\r\n", pSI->bedHeight) ;
		fprintf(fptr, "%%Number of Detector Pass := %d\r\n", pSI->headPasses) ;
	  }
	  else
	  {
		fprintf(fptr, "!originating system := HRRT\r\n") ;
	  }

	  // save some basic I/O info
	  fprintf(fptr, "!GENERAL DATA (with block singles)\r\n") ;
	  fprintf(fptr, "%%list mode file name := %s\r\n", histo_st->list_file) ;
	  fprintf(fptr, "!name of data file := %s\r\n", pFname) ;

	  // save study date & time info
	  fprintf(fptr, "!GENERAL IMAGE DATA\r\n") ;
	  fprintf(fptr, "!study date (dd:mm:yryr):= %02d:%02d:%4d\r\n",
		  pSI->scanStartTime.wDay, pSI->scanStartTime.wMonth, pSI->scanStartTime.wYear) ;
		// ZYB GMT
	  fprintf(fptr, "!study time (hh:mm:ss):= %02d:%02d:%02d\r\n",
		  pSI->scanStartTime.wHour, pSI->scanStartTime.wMinute, pSI->scanStartTime.wSecond) ;

	  fprintf(fptr, "!PET data type := %s\r\n", cPetDataType) ;
	  fprintf(fptr, "data format := sinogram\r\n") ;
	  fprintf(fptr, "number format := signed integer\r\n") ;
	  fprintf(fptr, "number of bytes per pixel := %d\r\n", nBytesPerPixel) ; 
	  fprintf(fptr, "number of dimensions := %d\r\n", nDims) ;
	  fprintf(fptr, "matrix size [1] := %d\r\n", histo_st->nprojs) ;
	  fprintf(fptr, "matrix size [2] := %d\r\n", histo_st->nviews) ;
	  fprintf(fptr, "matrix size [3] := %d\r\n", nSinos) ;

	  fprintf(fptr, "scale factor (mm/pixel) [1] := %f\r\n", scaleFactor[0]) ;
	  fprintf(fptr, "scale factor [2] := %f\r\n", scaleFactor[1]) ;
	  fprintf(fptr, "scale factor (mm/pixel) [3] := %f\r\n", scaleFactor[2]) ;	
	  
	  fprintf(fptr, "axial compression := %2d\r\n", pSI->span) ;
	  fprintf(fptr, "maximum ring difference := %3d\r\n", pSI->ringDifference) ;
	  fprintf(fptr, "number of rings := %d\r\n", nDetRings) ;
	  fprintf(fptr, "number of energy window := %d\r\n", nEnergyWins) ; 	  
	  fprintf(fptr, "energy window lower level [1] := %d\r\n", pSI->lld) ;
	  fprintf(fptr, "energy window upper level [1] := %d\r\n", pSI->uld) ;

	  if (histo_st->em_sinm == NET_TRUES)
	  {
		 fprintf(fptr, "number of scan data type := %d\r\n", nDataType) ;
		 fprintf(fptr, "scan data type description [1] := corrected prompts\r\n") ;
		 fprintf(fptr, "data offset in bytes [1] := %d\r\n",
				dataOffset[0]*sizeof(short)) ;
	  }
	  else
	  {
		 fprintf(fptr, "number of scan data type := %d\n", nDataType) ;
		 fprintf(fptr, "scan data type description [1] := prompts\r\n") ;
		 fprintf(fptr, "data offset in bytes [1] := %d\r\n",
				dataOffset[0]*sizeof(short)) ;
		 fprintf(fptr, "scan data type description [2] := randoms\r\n") ;
		 fprintf(fptr, "data offset in bytes [2] := %d\r\n",
				dataOffset[1]*sizeof(short)) ;
	  }

	  // save scan statistics
	  fprintf(fptr, "!IMAGE DATA DESCRIPTION\r\n") ;
   	  fprintf(fptr, "total prompts := %I64d\r\n", totalPrompts) ;
   	  fprintf(fptr, "total randoms := %I64d\r\n", totalRandoms) ;
   	  fprintf(fptr, "total net trues := %I64d\r\n", totalPrompts - totalRandoms) ;

	  // save frame timing information
	  frame_number = frame_def_st->cur_frame ;
	  fprintf(fptr, "%%frame number := %d\r\n", frame_def_st->cur_frame) ;
      fprintf(fptr, "image duration := %d\r\n", frame_def_st->frame_duration[frame_number]) ;
      fprintf(fptr, "image relative start time := %d\r\n",
				frame_def_st->frame_start_time[frame_number]);
// ZYB      fprintf(fptr, "%%image duration from timing tags (ms) := %d\r\n", tag_st->ellapsed_time);
	  fprintf(fptr, "%%image duration from timing tags (ms) := %I64d\r\n", tag_st->ellapsed_time);
	  // write out the detector block singles
	  if (pSI->gantry == HRRT_SCANNER)
	  {
		 int i, j, k;

	     fprintf(fptr, "\r\n%%HRRT Detector Block Singles\r\n") ;
// ZYB	     fprintf(fptr, "%%average singles per block = %d\r\n", tag_st->block_singles_st.total_uncor_rate) ;
		 fprintf(fptr, "average singles per block := %d\r\n", tag_st->block_singles_st.total_uncor_rate) ;
		 int hrrt_ncols = 9 ;
		 int hrrt_nrows = 13 ;
		 int hrrt_nheads = 8 ;

		 for (i=0; i<hrrt_nheads; i++)
		   for (j=0; j<hrrt_nrows; j++)
		   {
			  fprintf(fptr, "%%singles head %2d row %2d := ", i, j) ;
		      for (k=0; k<hrrt_ncols; k++)
				fprintf(fptr, "%d ", (long)(tag_st->block_singles_st.uncor_rate[k+j*hrrt_ncols+i*hrrt_ncols*hrrt_nrows]+0.5));
			  fprintf(fptr, "\r\n") ;
		   }
	  }
	  fclose(fptr) ;
	  return(OK) ;
}

// module to create interfile header for sinograms
int write_interfile_header2(
		char *header_fname,
		int frame_number,
		int scan_type,
		struct _histo_st *histo_st,
		struct _tag_st *tag_st,
		struct _frame_def_st *frame_def_st)

{

FILE *fptr ;

__int64	totalPrompts ;
__int64	totalRandoms ;
int	nEnergyWins=1 ;
int nDetRings ;
int nSinos ;
int sinoMode ;
int nDataType ;
int nDims=3 ;
int nBytesPerPixel=2 ;
long dataOffset[8] ;
double scaleFactor[4] ;
double hrrtPixelSize = 1.21875 ;
double ecamPixelSize = 2.14511 ;
char *pFname ;
char cPetDataType[1024] ;

	// test using subroutine michelogram
	int status ;

	// if number of detector rings not specified, assign value from model
	if (histo_st->nrings == 0)
	{
		if (pSI->gantry == ECAM_SCANNER) histo_st->nrings = 96 ;
		if (pSI->gantry == HRRT_SCANNER) histo_st->nrings = 104 ;
		else histo_st->nrings = header_st.nrings ; //12/07/01
	}
	printf("write_interfile2: histo_st.nrings = %d\n", histo_st->nrings) ;
	printf("write_interfile2: pSI.gantry = %d\n", pSI->gantry) ;

	// NOTE: S Tolbert 19 Sept 02 - This span and rd assigment is temporary to deal with the Em/Tx passes
	int span, rd;
	span = pSI->span;
	rd = pSI->ringDifference;
	if (histo_st->acqm == SIMU_EM_TX && scan_type == TX_ONLY) rd = (span-1)/2;

	status = michelogram(histo_st->nrings, span, rd, &header_st) ;
	//status = michelogram(histo_st->nrings, pSI->span, pSI->ringDifference, &header_st) ; // keep is original

	// assign value to vars that are scan-type (em or tx) specific
	if (scan_type == EM_ONLY)
	{
		pFname = histo_st->em_filename ;
		totalPrompts = GS_em_prompts ;
		totalRandoms = GS_em_randoms ;
		nSinos = histo_st->n_em_sinos ;
		sinoMode = histo_st->em_sinm ;
		dataOffset[0] = 0;
		dataOffset[1] = dataOffset[0] + histo_st->max_em_offset;
	}
	else if (scan_type == TX_ONLY)
	{
		pFname = histo_st->tx_filename ;
		totalPrompts = GS_tx_prompts ;
		totalRandoms = GS_tx_randoms ;
		nSinos = histo_st->n_tx_sinos ;
		sinoMode = histo_st->tx_sinm ;
		dataOffset[0] = histo_st->txp_offset ;
		dataOffset[1] = histo_st->txr_offset ;
	}

	// assign value to vars that are scanner specific
	if (pSI->gantry == ECAM_SCANNER)
	{
		scaleFactor[0] = ecamPixelSize ;
		scaleFactor[1] = 1.0 ;
		scaleFactor[2] = ecamPixelSize ;
		nDetRings = 84 ;
	}
	else if (pSI->gantry == HRP_SCANNER || pSI->gantry == GNT_P39)
	{
		scaleFactor[0] = header_st.binsize ;
		scaleFactor[1] = 1.0 ;
		scaleFactor[2] = header_st.plane_separation ;
		nDetRings = header_st.nrings;
	}
	else // else it's HRRT
	{
		scaleFactor[0] = hrrtPixelSize ;
		scaleFactor[1] = 1.0 ;
		scaleFactor[2] = hrrtPixelSize ;
		nDetRings = 104 ;
	}

	// assign value to vars that are scan-protocol specific
	if (pSI->scanType == EM_ONLY)
		strcpy(cPetDataType, "Emission") ;
	else if (pSI->scanType == TX_ONLY)
		strcpy(cPetDataType, "Transmission") ;
	else if (pSI->scanType == SIMU_EM_TX)
		strcpy(cPetDataType, "Simultaneous Emission + Transmission") ;
	else
		strcpy(cPetDataType, "Unknown") ;

	// assign value to vars that are data definition specific
	nDataType = 1 ;
	if (sinoMode == PROMPTS_AND_RANDOMS) nDataType = 2 ;

	  strcpy(header_fname, pFname) ;
   	  strcat(header_fname, ".hdr") ;
	  printf("creating header file %s\n", header_fname) ;
   	  fptr = fopen(header_fname, "w") ;
   	  if (fptr == NULL)
	  {
		 printf("frame_write4:ERROR: cannot create histo-header file '%s'\n", header_fname) ;
		 printf("frame_write4:ERROR: System Message: %s\n", sys_errlist[errno]) ;
      	 return(HISTO_HEADER_CREATION_ERROR) ;
	  }
	  fprintf(fptr, "!INTERFILE :=\r\n") ;
	  // S Tolbert 12 Sept 02 - Added interfile version
	  fprintf(fptr, "%%interfile ver := 0.1\r\n") ;

	  // save scanner model information
	  if (pSI->gantry == ECAM_SCANNER)
	  {
		fprintf(fptr, "!originating system := E.CAM\r\n") ;
	    fprintf(fptr, "%%Horizontal Bed Position (cm) := %f\r\n", pSI->bedInOutStart) ;
	    fprintf(fptr, "%%Vertical Bed Position (cm) := %f\r\n", pSI->bedHeight) ;
		fprintf(fptr, "%%Number of Detector Pass := %d\r\n", pSI->headPasses) ;
	  }
	  else if (pSI->gantry == HRRT_SCANNER)
	  {
		  fprintf(fptr, "!originating system := HRRT\r\n") ;
	  }
	  else
	  {
		  fprintf(fptr, "!originating system := %s\r\n", pSI->gantryModel) ;
	  }

	  // save some basic I/O info
	  fprintf(fptr, "\r\n!GENERAL DATA :=\r\n") ;
	  fprintf(fptr, "%%list mode file name :=\r\n") ;
	  fprintf(fptr, "!name of data file := %s\r\n", pFname) ;

	  // save study date & time info
	  fprintf(fptr, "\r\n!GENERAL IMAGE DATA :=\r\n") ;
	  fprintf(fptr, "!study date (dd:mm:yryr):= %02d:%02d:%4d\r\n",
		  pSI->scanStartTime.wDay, pSI->scanStartTime.wMonth, pSI->scanStartTime.wYear) ;
	  fprintf(fptr, "!study time (hh:mm:ss GMT):= %02d:%02d:%02d\r\n",
		  pSI->scanStartTime.wHour, pSI->scanStartTime.wMinute, pSI->scanStartTime.wSecond) ;
	  fprintf(fptr, "imagedata byte order := LITTLEENDIAN\r\n");			// PCs only use LittleIndian
	  fprintf(fptr, "isotope name :=\r\n") ;
	  fprintf(fptr, "isotope halflife (sec) :=\r\n") ;
	  fprintf(fptr, "isotope branching factor :=\r\n") ;
	  fprintf(fptr, "radiopharmaceutical :=\r\n") ;
	  fprintf(fptr, "patient orientation :=\r\n") ;
	  fprintf(fptr, "patient rotation :=\r\n") ;
	  fprintf(fptr, "!PET data type := %s\r\n", cPetDataType) ;
	  fprintf(fptr, "data format := sinogram\r\n") ;
	  fprintf(fptr, "number format := signed integer\r\n") ;
	  fprintf(fptr, "number of bytes per pixel := %d\r\n", nBytesPerPixel) ; 
	  fprintf(fptr, "number of dimensions := %d\r\n", nDims) ;
	  fprintf(fptr, "matrix size [1] := %d\r\n", histo_st->nprojs) ;
	  fprintf(fptr, "matrix size [2] := %d\r\n", histo_st->nviews) ;
	  fprintf(fptr, "matrix size [3] := %d\r\n", nSinos) ;

	  fprintf(fptr, "scale factor (mm/pixel) [1] := %f\r\n", scaleFactor[0]) ;
	  fprintf(fptr, "scale factor [2] := %f\r\n", scaleFactor[1]) ;
	  fprintf(fptr, "scale factor (mm/pixel) [3] := %f\r\n", scaleFactor[2]) ;	
	  
	  // add to support PetCT: 12/10/01
	  fprintf(fptr, "horizontal bed translation := stepped\r\n");
	  fprintf(fptr, "start horizontal bed position (mm) := %f\r\n", 10.0*pSI->bedInOut);
	  fprintf(fptr, "end horizontal bed position (mm) :=\r\n") ;
	  fprintf(fptr, "start vertical bed position (mm) := %f\r\n", 10.0*pSI->bedHeight) ;

	  fprintf(fptr, "axial compression := %2d\r\n", pSI->span) ;
	  //fprintf(fptr, "maximum ring difference := %3d\r\n", pSI->ringDifference) ; // keep this is original
	  fprintf(fptr, "maximum ring difference := %3d\r\n", rd) ;
	  fprintf(fptr, "number of rings := %d\r\n", nDetRings) ;
	  fprintf(fptr, "number of segments := %d\r\n", header_st.nsegments) ;

	  int segm_sum, i ;
	  char segm_str[4096] ;
	  char temp_str[8] ;

	  strcpy(segm_str, "") ;
	  sprintf(temp_str, "%s", "{") ;
	  strcat(segm_str, temp_str) ;
	  for (i=0;i<header_st.nsegments; i++)
	  {
		  if (i == (header_st.nsegments-1))// do not add ',' on last segment
			sprintf(temp_str, "%d", header_st.segment_table[i]) ;
		  else
		    sprintf(temp_str, "%d,", header_st.segment_table[i]) ;
		  strcat(segm_str, temp_str) ;
	  }
	  sprintf(temp_str, "%s", "}") ;
	  strcat(segm_str, temp_str) ;
	  fprintf(fptr, "segment table := %s\r\n", segm_str) ;
	  
	  segm_sum = 0 ;
	  for (i=0; i<header_st.nsegments; i++)
		  segm_sum += header_st.segment_table[i] ;
	  fprintf(fptr, "total number of sinograms := %d\r\n", segm_sum) ;

	  fprintf(fptr, "number of energy windows := %d\r\n", nEnergyWins) ; 	  
	  fprintf(fptr, "energy window lower level [1] := %d\r\n", pSI->lld) ;
	  fprintf(fptr, "energy window upper level [1] := %d\r\n", pSI->uld) ;

	  if (  (histo_st->em_sinm == NET_TRUES && scan_type == EM_ONLY)
		  ||(histo_st->tx_sinm == NET_TRUES && scan_type == TX_ONLY) )
	  {
		 fprintf(fptr, "number of scan data types := %d\r\n", nDataType) ;
		 fprintf(fptr, "scan data type description [1] := corrected prompts\r\n") ;
		 fprintf(fptr, "data offset in bytes [1] := %d\r\n",
				dataOffset[0]*sizeof(short)) ;
	  }
	  else
	  {
		 fprintf(fptr, "number of scan data type := %d\r\n", nDataType) ;
		 fprintf(fptr, "scan data type description [1] := prompts\r\n") ;
		 fprintf(fptr, "data offset in bytes [1] := %d\r\n",
				dataOffset[0]*sizeof(short)) ;
		 fprintf(fptr, "scan data type description [2] := randoms\r\n") ;
		 fprintf(fptr, "data offset in bytes [2] := %d\r\n",
				dataOffset[1]*sizeof(short)) ;
	  }

	  // save scan statistics
	  fprintf(fptr, "\r\n!IMAGE DATA DESCRIPTION :=\r\n") ;
   	  fprintf(fptr, "total prompts := %I64d\r\n", totalPrompts) ;
   	  fprintf(fptr, "total randoms := %I64d\r\n", totalRandoms) ;
   	  fprintf(fptr, "total net trues := %I64d\r\n", totalPrompts - totalRandoms) ;

	  // save frame timing information
	  frame_number = frame_def_st->cur_frame ;
	  fprintf(fptr, "%%frame number := %d\r\n", frame_def_st->cur_frame) ;

      fprintf(fptr, "image duration (sec) := %d\r\n", frame_def_st->frame_duration[frame_number]) ;
	  //fprintf(fptr, "image duration := %d\r\n", pSI->scanDuration);	//17-Jan-2002

	  fprintf(fptr, "image relative start time (sec) := %d\r\n",
				frame_def_st->frame_start_time[frame_number]);
// ZYB      fprintf(fptr, "%%image duration from timing tags (ms) := %d\r\n", tag_st->ellapsed_time);
      fprintf(fptr, "%%image duration from timing tags (ms) := %I64d\r\n", tag_st->ellapsed_time);

	  // write out the detector block singles
	  if (pSI->gantry == HRRT_SCANNER)
	  {
	     fprintf(fptr, "\r\n%%HRRT Detector Block Singles\r\n") ;
// ZYB	     fprintf(fptr, "%%average singles per block = %d\r\n", tag_st->block_singles_st.total_uncor_rate) ;
		 fprintf(fptr, "average singles per block := %d\r\n", tag_st->block_singles_st.total_uncor_rate) ;
		 int hrrt_ncols = 9 ;
		 int hrrt_nrows = 13 ;
		 int hrrt_nheads = 8 ;
	     int j, k ;
		 for (i=0; i<hrrt_nheads; i++)
		   for (j=0; j<hrrt_nrows; j++)
		   {
			  fprintf(fptr, "%%singles head %2d row %2d := ", i, j) ;
		      for (k=0; k<hrrt_ncols; k++)
				fprintf(fptr, "%d ", (long)(tag_st->block_singles_st.uncor_rate[k+j*hrrt_ncols+i*hrrt_ncols*hrrt_nrows]+0.5));
			  fprintf(fptr, "\r\n") ;
		   }
	  }
	  else
	  {
		  printf("total number of detector blocks = %d\n", histo_st->ndetblks) ;
		  fprintf(fptr, "\r\n%%Detector Block Singles :=\r\n") ;
		  //fprintf(fptr, "%%Total number of buckets := %d\r\n", histo_st->ndetblks) ;
		  fprintf(fptr, "%%Total number of blockrings := %d\r\n", histo_st->ndetblks) ;
		  unsigned long TotalSinglesRate = 0;
		  for (i=0; i<histo_st->ndetblks; i++) TotalSinglesRate += (unsigned long) tag_st->block_singles_st.uncor_rate[i];
		  fprintf(fptr, "%%Total Uncorrected Singles Rate := %d\r\n", TotalSinglesRate) ;
		  //for (i=0; i<histo_st->ndetblks; i++)
		  for (i=1; i<=histo_st->ndetblks; i++)
		    fprintf(fptr, "%%total number of singles per blockring[%d] := %d\r\n", i, (long) (tag_st->block_singles_st.uncor_rate[i]+0.5));
		    //fprintf(fptr, "%%block = %d, singles rate := %d\r\n", i, (long) (tag_st->block_singles_st.uncor_rate[i]+0.5));
	  }

	  fclose(fptr) ;
	  return(OK) ;
}

/****************************************************************************
*
*	histo_allocate()
*
*		- allocates the memory needed to perform histogramming
*
******************************************************************************/
BOOL histo_allocate(DWORD dwNBytes, int allocCode)

{
	if (allocCode == ALLOC_malloc)
	{
		printf("Memory allocation via malloc\n");
		sbuf = (short int *) malloc((size_t) dwNBytes);
		if (sbuf == NULL) return FALSE;
		//else memset((void *) sbuf, 0, (size_t) dwNBytes);
		return(TRUE);
	}

	if (allocCode == ALLOC_new)
	{
		printf("Memory allocation via new()\n");
		sbuf = new short int[dwNBytes/sizeof(short int)];
		if (sbuf == NULL) return FALSE;
		return(TRUE);
	}
	if (allocCode == ALLOC_win32Heap)
	{
		DWORD dwHeapSize;
		DWORD dwNHeapBytes=1000000;

		printf("Allocating %d bytes of histogram memory via HeapAlloc\n", dwNBytes);
/****
		hHeap = GetProcessHeap();
		if (hHeap == NULL)
		{
			dwWin32LastError = GetLastError();
			printf("Win32LastError = %d\n", dwWin32LastError);
			return(-1);
		}
****/
		sbuf = (short int *) HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, dwNBytes);
		if (sbuf == NULL)
		{
			printf("memory shortage\n");
			return(FALSE) ;
		}
		dwHeapSize = HeapSize(hHeap, NULL, (LPVOID) sbuf);
		printf("actual heap size = %d\n", dwHeapSize);
		return(TRUE);
	}

	if (allocCode == ALLOC_win32Virtual)
	{
		DWORD dwAllocationType;
		DWORD dwProtect;

		printf("histo_allocate: subroutine to allocate % bytes histogram memory\n", dwNBytes);
		dwAllocationType = MEM_RESERVE | MEM_TOP_DOWN | MEM_COMMIT;
		dwProtect = PAGE_READWRITE;

		printf("sbuf = %x\n", sbuf);
		sbuf = (short int *) VirtualAlloc(NULL, dwNBytes, dwAllocationType, dwProtect);
		printf("sbuf = %x\n", sbuf);
		if (sbuf == NULL) return(FALSE);
		//else ZeroMemory((PVOID) sbuf, (size_t) dwNBytes);
		return(TRUE);
	}

	return(FALSE);
}

/****************************************************************************
*
*	histo_deallocate()
*
*		- deallocates the memory used to perform histogramming
*
******************************************************************************/
BOOL histo_deallocate(LPVOID lpHistoMem, int allocCode)
{
	if (allocCode == ALLOC_malloc)
	{
		free((void *) sbuf);
		return(TRUE);
	}

	if (allocCode == ALLOC_new)
	{
		delete [] sbuf;
	}

	if (allocCode == ALLOC_win32Heap)
	{
		BOOL bHeapFree;
		bHeapFree = HeapFree(hHeap, NULL, sbuf);
		if (!bHeapFree)
		{
			printf("HeapFree error = %d\n", GetLastError());
			return(FALSE);
		}
		else return(TRUE);
	}

	if (allocCode == ALLOC_win32Virtual)
	{
		BOOL fMemFree;
		DWORD dwFreeType;

		dwFreeType = MEM_RELEASE;
		fMemFree = VirtualFree(lpHistoMem, 0, dwFreeType);
		return(fMemFree);
	}

	return(FALSE);
}
