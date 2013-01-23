//*********************************************************************
//
// Listmode.cpp - Listmode DLL for ACS III acquisitions that supports
//				  acquisitions with Systran or PFT cards
//
// Revision History :
//	jhr, modified	11mar02	-	Rewritten to work with Systran or PFT
//	jhr, created	22aug99
//
//*********************************************************************

// The compiler needs this
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <winbase.h>
#include <winioctl.h>
#include <iostream.h>
#include <stdio.h>
#include "acs3acq.h"
#include "lmioctl.h"
#include "RbIoctl.h"
#include "TfaIoctl.h"
#include "PftIoctl.h"
#include "LmHistBeUtils.h"
#include "LmHistRebinnerInfo.h"
#include "LmHistStats.h"
#include "scanner.h"
#include "PetCTGCIDll.h"
#include "gantryModelClass.h"

#define LISTMODEDLLAPI __declspec(dllexport)
#include "ListmodeDLL.h"

// Things used everywhere

// First the pointers to passed in initialization objects
LM_stats* LmStats ;				// Pointer to store online stats from
CCallback* LmCallbacks ;		// Pointer to callback object for async communication
CSinoInfo* LmSinoInfo ;			// Pointer to sinogram info object

// ZYB
int Exit_Wait_Falg = FALSE;

// The Acquisition class that contains entried needed by all the DLL
CAcquisition Acq ;

// Forward declaration
#define LOCAL static
LOCAL BOOL GetOsVersion (OSVERSIONINFOEX* osvi) ;

// default sector size
#define SECTOR_SIZE 512

// for debug
#define DP 1


/*********************************************************************
*
*
*	LMInit			- Exported routine to initilize Listmode environment
*
*	IN	LM_Stats*	- Pointer to store stats information in
*	IN	CCallBack*	- Pointer to AIM callbacks
*	IN	LSinoInfo*	- Pointer to singoram information
*	OUT	(None)	
*
*	Returns: Status	- SUCCESS or FAIL
*
**********************************************************************/

STATUS
LMInit (LM_stats* lmstats, CCallback* callbacks, CSinoInfo* sinoinfo)
{
	CHardware *Fcd ;
	char Pft[] =	"\\\\.\\pft1" ;
	char Sldc[] =	"\\\\.\\sldc1" ;
	int err ;

	// Use a Systran or PFT, whichever is there
	// Try the Systran first
	Fcd = new CSldc ;
	if (!Fcd->Open (Sldc, &err))
	{
		if (DP) cout << "LMInit: Systran failed to open..." << endl ;
		// If Systran fails try PFT
		delete Fcd ;
		Fcd = new CPft ;
		if (!Fcd->Open (Pft, &err))
		{
			if (DP) cout << "LMInit: Pft failed to open..." << endl ;
			return (FAIL) ;
		}
		else
		{
			Acq.pFcd = Fcd ;
			Acq.m_Systran = false ;
			if (DP) cout << "LMInit: Pft handle " << Acq.pFcd->m_Handle << " ..." << endl ;
		}
	}
	else
	{
		// If Systran open the other two device handles we need
		Acq.pFcd = Fcd ;
		Acq.pFcd2 = new CSldc ;
		Acq.pFcd3 = new CSldc ;
		Acq.pFcd2->Open (Sldc, &err) ;
		Acq.pFcd3->Open (Sldc, &err) ;
		Acq.m_Systran = true ;
		if (DP) cout << "LMInit: Systran handle " << Acq.pFcd->m_Handle << " ..." << endl ;
		if (DP) cout << "LMInit: Systran handle " << Acq.pFcd2->m_Handle << " ..." << endl ;
		if (DP) cout << "LMInit: Systran handle " << Acq.pFcd3->m_Handle << " ..." << endl ;
	}

	// Store the Init parameter's pointers
	LmStats = lmstats ;
	LmCallbacks = callbacks ;
	LmSinoInfo = sinoinfo ;

	// Open the rebinner and TFA
	Acq.pReb = new CRebinner ;
	Acq.pTfa = new CTfa ;
	Acq.pReb->Open ("\\\\.\\rb1", &err) ;
	Acq.pTfa->Open ("\\\\.\\tfa1", &err) ;

	// Create the timers
	Acq.hPresetTimer = CreateWaitableTimer ((LPSECURITY_ATTRIBUTES)NULL, (BOOL)FALSE, (LPCTSTR)NULL) ;
	Acq.hStatsTimer = CreateWaitableTimer ((LPSECURITY_ATTRIBUTES)NULL, (BOOL)FALSE, (LPCTSTR)NULL) ;
	Acq.hSinglesTimer = CreateWaitableTimer ((LPSECURITY_ATTRIBUTES)NULL, (BOOL)FALSE, (LPCTSTR)NULL) ;

	// Create the synchronization events
	Acq.hStartAcqEvent = CreateEvent ((LPSECURITY_ATTRIBUTES)NULL, FALSE, FALSE, (LPTSTR)NULL) ;
	Acq.hSinglesPollEvent = CreateEvent ((LPSECURITY_ATTRIBUTES)NULL, FALSE, FALSE, (LPTSTR)NULL) ;

	Acq.LmState = INITIALIZED ;

	// And return
	if (DP) cout << "LMInit: Completed successfully..." << endl ;
	return (SUCCESS) ;
}

/*********************************************************************
*
*
*	LMConfig			- Exported routine to configure a raw or
*						  online listmode acquisition
*
*	IN	char* filename	- Pointer to store stats information in
*	IN	ulong preset	- Preset time in seconds
*	IN	int time		- Indicates preset type
*	IN	int online		- Indicates online or raw listmode
*	IN	ulong bypass	- Indicates 64 bit raw or 32 bit mode
*	IN	Rebinner_info&	- Rebinner info
*
*	OUT	(None)
*
*	Returns:	STATUS	- SUCCESS or FAIL	
**********************************************************************/

STATUS
LMConfig (
	char* fname,
	ULONG preset,
	int time,
	int online,
	ULONG bypass,
	Rebinner_info& RebinnerInfo)
{
	char str[64] ;

	cout << "CTI_LM : LMConfig entered state = " << Acq.LmState << endl ;
	sprintf (str, "LmState = %x", Acq.LmState) ;
	//MessageBox (NULL, str, NULL, MB_OK) ;

	// handle the acquisition state
	if ((Acq.LmState == UNINITIALIZED) || (Acq.LmState == ACQUIRE)) return (FAIL) ;

	// initialize the scan control members
	strcpy (Acq.LmFile, fname) ;
	Acq.LmPreset = preset  ;
	Acq.LmTime = time ;
	Acq.LmOnline = online ;
	Acq.LmBypass = bypass ;

	// NOTE: check to see if this still belongs
	// if (online) Acq.LmPreset += 3 ;

	// initialize the stats object
	LmStats->n_updates = 0 ;
	LmStats->time_elapsed = 0 ;
	LmStats->time_remaining = 0 ;
	LmStats->total_stream_events.QuadPart = 0 ;
	LmStats->total_event_rate = 0 ;

	// change. 11/16/00 - old stuff - check to see if we can take out
	// if (LmBypass >= 1000)
	// 	LmBypass -= 1000;	// recover the intended value
	// if (bypass >= 1000)
	// {
		// bypass>=1000 is a special flag to bypass the rebinner init for
		// multi-bed scan. If the rebinner configuration is the same for all
		// the beds, rebinner card only needs to be initialized and configured
		// for the first bed.
	// 	cout << "CTI_LM : LMConfig... Bypass rebinner setup" << endl ;
	// } 
	// else
	// {

	// reset the rebinner
	Acq.ResetRebinner (RebinnerInfo.lut, (int)bypass) ;

	// now sync rebinner and TFA if PetCT
	if (LmSinoInfo->gantry == 2)
	{
		if (!Acq.SyncRebinner(RebinnerInfo.lut, bypass))
		{
			cout << "CTI_LM : LMConfig... Rebinner sync failed" << endl ;
			return (FAIL) ;
		}
	}

	cout << "CTI_LM : LMConfig... Rebinner setup" << endl ;

	Acq.LastHistoBufr = FALSE ;
	Acq.LmStopCondition = LmCallbacks->Preset ;
	Acq.LmState = CONFIGURED ;

	// reset the Fibre Channel device
	Acq.pFcd->Reset () ;

	cout << "CTI_LM : LMConfig... Configured" << endl ;

	return (SUCCESS) ;
}
 
/*********************************************************************
*
*
*	LMStart				- Exported routine to start acquisition
*
*	IN	(None)
*	OUT	(NONE)
*
*	Returns:	STATUS	- SUCCESS or FAIL	
**********************************************************************/

STATUS
LMStart ()
{
	LARGE_INTEGER timertime ;
	void LmAcqControl (), LmAcqPreset (), LmStatsUpdate (), LmSinglesPoll () ;

	// manage the state machine
	if (Acq.LmState != CONFIGURED) return (FAIL) ;

	cout << "CTI_LM : LMStart entered" << endl ;

	// map the FC device memory to user space
	if (DP) cout << "LMStart: Calling Map Buffers..." << endl ;
	if (!(Acq.pFcd->GetMappedBuffer ())) return (FAIL) ;
	if (DP) cout << "LMStart: Memory buffers mapped..." << endl ;

	// start the control threads
	if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) LmAcqPreset, NULL, 0, NULL))) return false ;
	if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) LmStatsUpdate, NULL, 0, NULL))) return false ;
	if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) LmAcqControl, NULL, 0, NULL))) return false ;
	// if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) LmSinglesPoll, NULL, 0, NULL))) return false ;

	// if we are in raw listmode create the file
	if (!Acq.LmOnline) Acq.CreateLmFile () ;
	if (DP) cout << "LMStart: File Created..." << endl ;
	
	// this is for PetSPECT for rotational compensation
	if (LmSinoInfo->gantry == 1)
	{
		Acq.LmPreset = (ULONG)((double)Acq.LmPreset * (double)1.02) ;
		cout << "CTI_LM : LmPreset relaxed to "<< Acq.LmPreset << endl ;
	}

	// reset the Fibre Channel device
	Acq.pFcd->Reset () ;

	cout << "CTI_LM : LMStart... Fibre Channel ready" << endl ;

	// store somethings away for for computations
	//GetSystemTime (&LmSinoInfo->scanStartTime) ;
	// ZYB
	GetLocalTime(&LmSinoInfo->scanStartTime) ;
	//char sttimea[128], sttimeb[128];
	//sprintf(sttimea,"%02d:%02d:%02d\n",LmSinoInfo->scanStartTime.wHour, LmSinoInfo->scanStartTime.wMinute, LmSinoInfo->scanStartTime.wSecond);
	//sprintf(sttimes,"%02d:%02d:%02d\n",pSI->scanStartTime.wHour, LmSinoInfo->scanStartTime.wMinute, LmSinoInfo->scanStartTime.wSecond);

	//OutputDebugString();
	Acq.m_StartTime = GetTickCount () ;
	Acq.m_LastCounts = 0 ;

	// start the Fibre Channel device acquiring
	if (DP) cout << "LMStart: Starting Fibre Channel Device..." << endl ;
	if (!Acq.pFcd->Acquire ())
	{
		SetEvent (Acq.hStartAcqEvent) ;
		Acq.pFcd->Stop () ;
		Sleep (5000) ;
		ResetEvent (Acq.hStartAcqEvent) ;
		return (FAIL) ;
	}

	cout << "CTI_LM : LMStart... Acquisition started" << endl ;
	
	// set the event to trigger the control thread
	if (DP) cout << "LMStart: Setting the event..." << endl ;
	SetEvent (Acq.hStartAcqEvent) ;
	QueryPerformanceCounter (&Acq.m_ScanStart) ;
	Acq.m_LastTimeCount.QuadPart = Acq.m_ScanStart.QuadPart ;

	// start the preset timer...
	Acq.ConvertTime (Acq.LmPreset, &timertime) ;
	SetWaitableTimer (Acq.hPresetTimer, &timertime, 0, NULL, NULL, FALSE) ;

	// and the stats polling timer...
	Acq.ConvertTime (STATS_UPDATE_TIME, &timertime) ;
	SetWaitableTimer (Acq.hStatsTimer, &timertime, 0, NULL, NULL, FALSE) ;

	// and the singles polling timer
	Acq.ConvertTime (SINGLES_UPDATE_TIME, &timertime) ;
	SetWaitableTimer (Acq.hSinglesTimer, &timertime, 0, NULL, NULL, FALSE) ;


	if (DP) cout << "LMStart: Timers started..." << endl ;
	// check to see if we can delete the rest of this
	// start the preset timer if we are not no doing online
	//if (!LmOnline)
	//{
	//	SetWaitableTimer (hPresetTimer, &lmPresetTime, 0, NULL, NULL, FALSE) ;
	//	cout << "CTI_LM : LMStart... Preset timer started" << endl ;
	//}

	// change the acquisition state
	Acq.LmState = ACQUIRE ;

	cout << "CTI_LM : LMStart... Start successful" << endl ;

	// that's it
	return (SUCCESS) ;
}
/*********************************************************************
*
*
*	LMHalt				- Exported routine to stop an acquisition
*
*	IN	(None)
*	OUT	(NONE)
*					Ziad Burbar Oct 4th 2002
*	Returns:	STATUS	- SUCCESS	
**********************************************************************/

STATUS
LMHalt ()
{
	// Set the flag to TRUE soi we can exit the waitforsingleobject loop
	Exit_Wait_Falg = TRUE;
	return (SUCCESS) ;
}


/*********************************************************************
*
*
*	LMStop				- Exported routine to stop an acquisition
*
*	IN	(None)
*	OUT	(NONE)
*
*	Returns:	STATUS	- SUCCESS	
**********************************************************************/

STATUS
LMStop ()
{
	LARGE_INTEGER wTime ;

	// make a zero time value
	wTime.LowPart = (DWORD)0 ;
	wTime.HighPart = (DWORD)0 ;

	// set the stop condition that came from above
	Acq.LmStopCondition = LmCallbacks->Stop ;

	// trigger the preset by setting of the timer
	SetWaitableTimer (Acq.hPresetTimer, &wTime, 0, NULL, NULL, FALSE) ;

	return (SUCCESS) ;
}

/*********************************************************************
*
*
*	LMTriggerTimer	- Exported routine to manually preset the scan
*					  Behaves exactly the same as LMStop except the
*					  call specifies the stop condition is treated
*					  as a true preset
*
*	IN	(None)
*	OUT	(NONE)
*
*	Returns:	STATUS	- SUCCESS	
**********************************************************************/

STATUS
LMTriggerTimer ()
{
	LARGE_INTEGER wTime ;

	// make a zero time value
	wTime.LowPart = (DWORD)0 ;
	wTime.HighPart = (DWORD)0 ;

	// set the stop condition that came from above
	Acq.LmStopCondition = LmCallbacks->Preset ;

	// trigger the preset by setting of the timer
	SetWaitableTimer (Acq.hPresetTimer, &wTime, 0, NULL, NULL, FALSE) ;

	return (SUCCESS) ;
}

/*********************************************************************
*
*
*	LMAbort		- Exported routine to abort a scan
*				  Same as stop except for the
*				  'StopCondition'
*
*	IN	(None)
*	OUT	(NONE)
*
*	Returns:	STATUS	- SUCCESS	
**********************************************************************/

STATUS
LMAbort ()
{
	LMStop () ;

	Acq.LmStopCondition = LmCallbacks->Abort ;

	return (SUCCESS) ;
}

/*********************************************************************
*
*
*	LmAcqControl	- Thread to control the listmode acquisition
*
*	IN	(None)																										x
*	OUT	(None)	
*
*	Returns:	void
*
*	Note: This routine has to take into account which Fibre Channel
*		  device it is using, either the Systran or PFT. Separate
*		  paths are implemented depending on the device
*
**********************************************************************/

// import the histogramming routine
__declspec (dllimport) int HISTBuf (char *, int, int) ;

//
// HISTBuf - for testing purposes without the Histgramming DLL
//

//int
//HISTBuf (char *, int, int)
//{
//	return (0) ;
//}

void
LmAcqControl ()
{
	BUFR_CLR_WAIT bcw ;
	BUFR_CLR_WAIT_RET bcwr ;
	HANDLE ReadyEvent ;
	LMSTATS lmstats ;
	ULONG bnum, acq_status, *bufr0, *bufr1, *sbufr ,*inkibuf;
	DWORD nbytes, bytes_written ;
	int err, remainder, histRet, stopFlag ;
	BOOL acq_not_triggered = TRUE ;
	OSVERSIONINFOEX osvi ;
	char z[128];
	__int64 z_events=0;

	// if using a PFT we have to deal with things differently
	if (!Acq.m_Systran)
	{
		if (DP) cout << "CTI_LM AcqControl: Pft Inside PFT acquisition..." << endl ;
		
		// Decide how we have to deal with the event (NT4 or W2K/XP)
		GetOsVersion (&osvi) ;

		// Is it NT
		if (osvi.dwMajorVersion <= 4)
		{
			// open the named notification event
			if (DP) cout << "CTI_LM AcqControl: Opening sync event..." << endl ;
			if (!(ReadyEvent = OpenEvent (SYNCHRONIZE, FALSE, "LmBufrRdyEvent")))
			{
				cout << "AcqControl: Could not open event..." << endl ;
				return ;
			}

			if (DP) cout << "CTI_LM AcqControl: ReadyEvent opened..." << endl ;
		}
		else if (osvi.dwMajorVersion == 5)
		{
			// It is 2K or XP so create the event
			ReadyEvent = CreateEvent (NULL, TRUE, FALSE, "LmBufrReady") ;
			if (!ReadyEvent)
			{
				cout << "AcqControl: Error creating event " << GetLastError () << endl ;
				return ;
			}

			// Send it to the driver
			Acq.pFcd->SendNotificationHandle (&ReadyEvent) ;
		}

		// if we got here the acquisition is active
		if (DP) cout << "CTI_LM AcqControl: Setting status to active..." << endl ;
		acq_status = ACQ_ACTIVE ;			// S Tolbert Commented

		// assign the buffers
		bufr0 = Acq.pFcd->m_pMappedBuffer ;
		bufr1 = bufr0 + 0x100000 ;
		sbufr = bufr0 ;
		bnum = 0 ;
		

		if (DP) cout << "CTI_LM AcqControl: Buffers assigned..." << endl ;

		// now we play ping pong with the buffers

		while (acq_status != ACQ_STOPPED)
		{
			if (DP) cout << "CTI_LM AcqControl: Waiting for a buffer..." << endl ;

			// wait on the buffer to fill
			// ZYB
			if (WaitForSingleObject (ReadyEvent, 500) == WAIT_TIMEOUT)
				if (!Exit_Wait_Falg) 
				{
					//sprintf(z,"ListMode : lmstats.events = %u\n",lmstats.events);
					//OutputDebugString(z);					
					continue;
				}
						
			Exit_Wait_Falg = FALSE;
			// ZYB
			//OutputDebugString("ListMode: After WaitForSingleObject\n");

			// bingo, let's see what happened
			Acq.pFcd->GetInfo (&lmstats) ;
			if (DP) cout << "CTI_LM AcqControl: Got it... status = " << lmstats.status << " lmstats.events = " << lmstats.events << endl ;

			// remember the acquisition status and number of bytes
			acq_status = lmstats.status ;
			nbytes = lmstats.events * 4 ;
			z_events+=lmstats.events;

			//sprintf(z,"ListMode : nbytes = %u, acq_status = %u\n",nbytes, acq_status);
			//OutputDebugString(z);
			

			if (!Acq.LmOnline)
			{
				if (DP) cout << "CTI_LM AcqControl: Inside offline loop..." << endl ;
				// if this is the last buffer close file and reopen in normal
				// mode to handle non multiples of sector size writes
				if (acq_status != ACQ_ACTIVE)
					Acq.ReopenLmFile () ;

				if (acq_status == ACQ_STOPPED)
				{
					LmCallbacks->acqPostComplete(Acq.LmStopCondition, SUCCESS);
				}

				// write the buffer to disk
				// cout << "AcqControl: Writing buffer " << bnum << " to disk..." << endl ;
				inkibuf=sbufr;
				if(sbufr==bufr0) sbufr=bufr1;
				else if(sbufr==bufr1) sbufr=bufr0;
				else OutputDebugString("Swap buffer errror\n");
				//bnum++ ;
				//if (bnum & 1) sbufr = bufr1 ;
				//else sbufr = bufr0 ;

				// go tell the driver the buffer is free
				Acq.pFcd->TagBufferFree () ;
				
				Acq.WriteLmFile ((LPCVOID)inkibuf, nbytes, (LPDWORD)&bytes_written, &err) ;
				if (err !=0)
				{
					sprintf(z,"ListMode : Write Error = %d\n",err);
					OutputDebugString(z);
				}
			}
			
			//Acq.pFcd->TagBufferFree () ;

			// toggle the buffer
			//bnum++ ;
			//if (bnum & 1) sbufr = bufr1 ;
			//else sbufr = bufr0 ;
		}

		// clean up if we are done

		// Print out the datarate...
		sprintf(z,"DMAevents = %I64d\n, Written events = %I64d\n",pLMStats.total_events, z_events);
		OutputDebugString(z);

		CloseHandle (ReadyEvent) ;
		if (DP) cout << "CTI_LM AcqControl: Pft Acquisition loop done..." << endl ;
	}
	else // this is the Systran portion
	{
		if (DP) cout << "AcqControl: Inside Systran acquisition..." << endl ;
		while (acq_not_triggered)
		{
			if (DP) cout << "AcqControl: Acquisition triggered..." << endl ;
			// wait here for the Systran start event
			WaitForSingleObjectEx (Acq.hStartAcqEvent, INFINITE, TRUE) ;

			// change triggered state
			acq_not_triggered = FALSE ;

			// if we got here the acquisition is active
			acq_status = ACQ_ACTIVE ;

			// initialize the buffer keepers
			bnum = 0 ;
			bcw.wait_bnum = 0 ;
			bcw.clr_bnum = (ULONG)-1 ;
			sbufr = Acq.pFcd->m_pMappedBuffer ;
			bufr0 = sbufr ;
			bufr1 = bufr0 + (unsigned long)0x100000 ;

			// the control of the acquisition occurs in this loop
			while (acq_status != ACQ_STOPPED)
			{
				cout << "CTI_LM : LmAcqControl... Acquire loop entered" << endl ;
				// wait here for the buffer to fill
				if (Acq.pFcd2->BufferClearWait (&bcw, &bcwr))
				{
					// pick up the info about the buffer
					nbytes =  bcwr.nevents * 4 ;
					acq_status = bcwr.acq_status ;

					// determine the full buffer
					if (bnum & 1) sbufr = bufr1 ;
					else sbufr = bufr0 ;

					// if we are not online store the data to the listmode file
					if (!Acq.LmOnline)
					{
						// check the buffer size
						remainder = nbytes % SECTOR_SIZE;

						// if it is not a multiple of sector ssize reopen
						if (remainder) Acq.ReopenLmFile () ;

						// set the appropriate stop condition
						if ((acq_status == ACQ_STOPPED) && (!Acq.LmOnline)) 
							LmCallbacks->acqComplete (Acq.LmStopCondition) ;

						// write the buffer to the file
						Acq.WriteLmFile ((LPCVOID)sbufr, nbytes, (LPDWORD)&bytes_written, &err) ;

						// if (!wfStat) ;  we must error here

						cout << "CTI_LM : LmAcqControl... File Write " << nbytes << " nbytes" << endl ;
						cout << "CTI_LM : LmAcqControl... Bytes written " << bytes_written << endl ;

						// if we stopped without abort post a complete
						if ((acq_status == ACQ_STOPPED) && (Acq.LmStopCondition != LmCallbacks->Abort))
						{
							if (!Acq.LmOnline && Acq.LmStopCondition != LmCallbacks->Abort)
							{
								if (err) LmCallbacks->acqPostComplete (Acq.LmStopCondition, FAIL) ;
								else LmCallbacks->acqPostComplete (Acq.LmStopCondition, SUCCESS) ; ;

								cout << "CTI_LM : LmAcqControl... Sending acqPostComplete" << endl ;
							}
						}
					}
					else	// online
					{
						// set the appropriate stop condition if done
						if (DP) cout << "AcqControl: Setting stop condition..." << endl ;
						if (acq_status == ACQ_STOPPED) stopFlag = Acq.LmStopCondition ;
						else stopFlag = -1 ;
					
						// histogram it unless we are done
						if (!Acq.LastHistoBufr)
						{
							cout << "CTI_LM : LmAcqControl... Inside histogram portion" << endl ;
							histRet = HISTBuf ((char *)sbufr, (int)nbytes, stopFlag) ;
							cout << "CTI_LM : LmAcqControl... histRet " << histRet << endl ;

							if (!histRet)
							{
								cout << "CTI_LM : LmAcqControl... Inside histo halt" << endl ;
								// histogrammer says we need to stop
								CancelWaitableTimer (Acq.hPresetTimer) ;

								// stop the acquisition
								cout << "CTI_LM : LmAcqControl... Inside histo halt sending stop" << endl ;
								Acq.pFcd->Stop () ;
								Acq.LastHistoBufr = TRUE ;
							}
						}
					}

					// keep up with the buffer swapping
					bnum++;
					bcw.wait_bnum = bnum & 1 ;
					bcw.clr_bnum = ~bcw.wait_bnum & 1 ;
				}
				else ; // fatal error here if buffer clear wait fails
			} // end single loop acquisition inside while
		}
	}

	if (DP) cout << "AcqControl: Cleaning up..." << endl ;

	// unmap the buffer
	if (DP) cout << "AcqControl: Unmapping buffer..." << endl ;
	Acq.pFcd->UnmapBuffer () ;

	// update the state
	Acq.LmState = STOPPED ;

	// close the listmode acquisition file
	Acq.CloseLmFile () ;
}

/*********************************************************************
*
*
*	LmAcqPreset		- Routine to handle the stop when the preset
*					  timer expires. Runs as separate thread.
*
*	IN	(None)
*	OUT	(None)
*
*	Returns:		- void	
**********************************************************************/

void
LmAcqPreset ()
{
	while (TRUE)
	{
		// wait here for the preset timer event
		WaitForSingleObjectEx (Acq.hPresetTimer, INFINITE, TRUE) ;

		cout << "CTI_LM : LmAcqPreset... Preset entered" << endl ;

		// stop the Fibre Channel acquisition
		// if it is a Systran we have to use a a different device
		if (Acq.m_Systran) Acq.pFcd2->Stop () ;
		else
		{
			// ZYB
			OutputDebugString("LmAcqPreset: Stopping Pft...\n");
			Exit_Wait_Falg = TRUE;
			cout << "LmAcqPreset: Stopping Pft..." << endl ;
			Acq.pFcd->Stop () ;
		}
	}
}

// uncomment if linking with PetCTGCIDll
//BOOL GciPollSingles (int* singles, int* prompts, int* randoms, int* scatter)
//{
//	return (TRUE) ;
//}

// import the singles poll routine
__declspec (dllimport) BOOL GciPollSingles (int* singles, int* prompts, int* randoms, int* scatter) ;

//
// GCIPollSingles - for testing purposes without the PetCTGCI DLL
//

/*********************************************************************
*
*
*	LmSinglesPoll	- Routine to handle the singles polling during
*					  acquisition. Runs as separate thread.
*
*	IN	(None)
*	OUT	(None)
*
*	Returns:		- void	
**********************************************************************/

void
LmSinglesPoll ()
{
	CGantryModel scanner ;
	LM_ACQ_INFO SldcInfo ;
	LMSTATS PftInfo ;
	LARGE_INTEGER timertime ;
	int prompts, randoms, scatter, blk, tag32 ;
	int adjusted_singles, nbkts, nentries, acq_status ;
	unsigned short *tword ;
	unsigned __int64 w1, w2, reb_tag64, tag64 = 0x8000000040000000 ;
	

	// set the model number and init GCI dll with it
	scanner.setModel(LmSinoInfo->gantryModel) ;

	// allocate the array to store singles
	nbkts = scanner.buckets() ;
	nentries = nbkts * scanner.axialBlocks() ;
	int* singles = new int[nentries] ;


	while (TRUE)
	{
		// wait here til we are signaled by the timer
		WaitForSingleObjectEx (Acq.hSinglesTimer, INFINITE, TRUE) ;		

		// go get the singles
		GciPollSingles (singles, &prompts, &randoms, &scatter) ;
		//if (DP)
		//{
		//	for (blk=0;blk<nentries;blk++)
		//		cout << "LmSinglesPoll: block " << blk << " = " << singles[blk] << endl ;
		//}

		// format and word insert the singles into Rebinner
		tword = (unsigned short*)&tag32 ;
		for (blk=0;blk<nentries;blk++)
		{
			adjusted_singles = singles[blk] >> 2 ;
			tag32 = 0xa0000000 | (blk << 19) | adjusted_singles ;
			//printf ("LmSinglesPoll: tag32 = %08X  singles[%d] = %08X\n", tag32, blk, singles[blk]) ;
			w1 = (__int64)tword[1] ;
			w2 = (__int64)tword[0] ;
			//printf ("LmSinglesPoll: w1 = %I64X   w2 = %I64X\n", w1, w2) ;
			reb_tag64 = tag64 | (w1 << 32) | w2  ;

			//printf ("LmSinglesPoll: Inserting %I64X for block %d\n", reb_tag64, blk) ;
			Acq.pReb->WordInsert (reb_tag64) ;
		}

		// get the acquisition status
		// this is Fibre Channel device specific
		if (Acq.m_Systran)
		{
			cout << "LmSinglesPoll: Systran info..." << endl ;
			Acq.pFcd3->GetInfo (&SldcInfo) ;
			acq_status = SldcInfo.acq_status ;
		}
		else
		{
			cout << "LmSinglesPoll: Pft info..." << endl ;
			Acq.pFcd->GetInfo (&PftInfo) ;
			acq_status = PftInfo.status ;
		}

		// restart the timer if we are still going
		if (acq_status == ACQ_ACTIVE)
		{
			Acq.ConvertTime (SINGLES_UPDATE_TIME, &timertime) ;
			SetWaitableTimer (Acq.hSinglesTimer, &timertime, 0, NULL, NULL, FALSE) ;
		}
	}
	delete [] singles;
}

/*********************************************************************
*
*
*	LmStatsUpdate	- Routine to handle the statistics updates during
*					  acquisition. Runs as separate thread.
*
*	IN	(None)
*	OUT	(None)
*
*	Returns:		- void	
**********************************************************************/

void
LmStatsUpdate ()
{
	LM_ACQ_INFO SldcInfo ;
	LMSTATS PftInfo ;
	LARGE_INTEGER timertime ;
	float deltaTime ;
	__int64 deltaEvents ;
	int acq_status ;
	char str[1024] ;

	while (TRUE)
	{
		// wait here for the timer to go off
		WaitForSingleObjectEx (Acq.hStatsTimer, INFINITE, TRUE) ;
		
		// get the info from the Fibre Channel device
		// this is device specific
		if (Acq.m_Systran)
		{
			Acq.pFcd3->GetInfo (&SldcInfo) ;
			LmStats->time_remaining = (ULONG)(Acq.LmPreset - (ULONG)((GetTickCount () - Acq.m_StartTime) / 1000)) ;
			if (LmStats->time_remaining < 0) LmStats->time_remaining = 0 ;
			LmStats->time_elapsed = (ULONG)Acq.LmPreset - LmStats->time_remaining ;

			deltaTime = ((float)(SldcInfo.total_time.QuadPart - Acq.m_LastTime)) / (float)10000000.0 ;
			Acq.m_LastTime = SldcInfo.total_time.QuadPart ;
			deltaEvents = SldcInfo.total_events.QuadPart - Acq.m_LastCounts ;
			Acq.m_LastCounts = SldcInfo.total_events.QuadPart ;
			LmStats->total_event_rate = (ULONG)((float)deltaEvents / deltaTime) ;
			if (LmStats->total_event_rate < 0) LmStats->total_event_rate = 0 ;

			
			LmStats->total_stream_events = SldcInfo.total_events ;
			char strbuf[128];
	//		cout << "total_events=" << _i64toa(SldcInfo.total_events.QuadPart, strbuf, 10)
	//		cout << "total_events.HighPart=" << SldcInfo.total_events.HighPart
	//			<< " total_events.LowPart=" << SldcInfo.total_events.LowPart
	//			<< endl;
			sprintf(strbuf, "total_events=%I64d", SldcInfo.total_events.QuadPart);
	//		cout << strbuf << endl;

			LmStats->n_updates++ ;
			acq_status = SldcInfo.acq_status ;
		}
		else
		{
			// get the driver info so we can compute statistics
			Acq.pFcd->GetInfo (&PftInfo) ;

			// determine the elapsed time
			LmStats->time_elapsed = Acq.ComputeScanTime () ;

			// compute the remaining time
			LmStats->time_remaining = Acq.LmPreset - LmStats->time_elapsed ;
			if (LmStats->time_remaining < 0) LmStats->time_remaining = 0 ;

			LmStats->total_event_rate = PftInfo.event_rate ;
			
			// compute the event rate
			//deltaTime = (float)LmStats->time_elapsed ;
			//deltaEvents = PftInfo.total_events - Acq.m_LastCounts ;
			//deltaEvents = PftInfo.total_events ;
			//Acq.m_LastCounts = PftInfo.total_events ;
			//LmStats->total_event_rate = (ULONG)((float)deltaEvents / deltaTime) ;
			if (LmStats->total_event_rate < 0) LmStats->total_event_rate = 0 ;

			sprintf (str, "elapsed time %d total events %I64d  deltaEvents %I64d  deltaTime %f",
							LmStats->time_elapsed, PftInfo.total_events, deltaEvents, deltaTime) ;
			//MessageBox (NULL, str, NULL, MB_OK) ;
			
			// count it
			LmStats->n_updates++ ;

			// get the acquisition status
			acq_status = PftInfo.status ;
		}

		// restart timer if still acquiring
		if (acq_status == ACQ_ACTIVE)
		{
			Acq.ConvertTime (STATS_UPDATE_TIME, &timertime) ;
			SetWaitableTimer (Acq.hStatsTimer, &timertime, 0, NULL, NULL, FALSE) ;
		} else LmStats->time_remaining = 0 ;
	}
}

/*********************************************************************
*
*
*	GetOsVersion	- This routine gets the major version of
*					  the OS so we know how to deal with the
*					  notification event.
*
*	Inputs:			- Pointer to OSVERSIONINFOEX to fill in
*	Outputs:		- Filled in OSVERSIONINFOEX
*
*
*	Returns:		- TRUE on success, FALSE on failure	
**********************************************************************/


LOCAL BOOL
GetOsVersion (
	OSVERSIONINFOEX* osvi
	)
{
   BOOL bOsVersionInfoEx;
 
   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.
   ZeroMemory(osvi, sizeof(OSVERSIONINFOEX));
   osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) osvi)) )
   {
      // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
      osvi->dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) osvi) ) 
         return FALSE;
   }

   return (TRUE) ;
}
