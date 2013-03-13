//*********************************************************************
//
// SinglesAcq.cpp - DLL for ACS III acquisitions of singles data using the PFT
//
// Revision History :
//
//	tgg, modified	01Dec03 -	removed commented out code
//	tgg, modified	05Nov03 -	added this header and modified to run under
//								windows NT, 2K or XP following example of listmode.dll
//	jhr, created	31Jan03 -	created
//*********************************************************************
//

// The compiler needs this
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <winbase.h>
#include <winioctl.h>
#include <iostream.h>
#include <stdio.h>
#include <time.h>
#include "SinglesAcq.h"
#include "PftIoctl.h"

// Log file for debugging stop problem - not in product code
FILE *fp = NULL ;
struct tm *newtime ;
time_t aclock ;

// Forward declaration
#define LOCAL static
LOCAL BOOL GetOsVersion		(OSVERSIONINFOEX* osvi) ;
			
//++
//
// CSinglesAcq::Acquire - Acquire listmode data and store to a file
//
// Inputs : char* file
//			unsigned long preset
//
// Outputs : bool, false = fail, true = success
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//										replaced m_quite with m_quiet
//
//--

bool
CSinglesAcq::Acquire (ACQ_ARGS* args)
{
	// preset the class member status
	m_status = true ;

	// set output mode
	m_quiet = !(args->verbose) ;
	m_debug = args->debug ;

	// store the file name and preset time
	strcpy (m_fname, args->fname) ;
	m_makefile = args->makefile ;
	m_preset = args->preset_time ;
	m_timeout = args->timeout ;

	// try to create the file
	if (m_makefile)
	{
		if (!CreateListmodeFile ()) return false ;
	}

	// open the device
	if (!CPft::Open()) return false ;

	// start the acquisition
	if (!StartAcquisition(args->passall, args->lut)) return false ;

	// create a synchronization event so we can hang around until things are done
	m_hscancomplete = CreateEvent ((LPSECURITY_ATTRIBUTES)NULL, FALSE, FALSE, (LPTSTR)NULL) ;
	if (m_hscancomplete == NULL) return false ;

	// hang around til the scan is done
	WaitForSingleObjectEx (m_hscancomplete, INFINITE, TRUE) ;

	// clean up
	if (m_makefile) CloseHandle (m_hfile) ;
	CloseHandle (m_hscancomplete) ;

	// and exit
	return (m_status) ;
}

//++
//
// CSinglesAcq::StartAcquisition - Start the PFT acquiring
//
// Inputs : NONE
//
// Outputs : true if OK else false
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//
//--

BOOL
CSinglesAcq::StartAcquisition(int passall, int lut)
{
	BOOL ret ;
	CRebinner Rebinner ;

	// set up the rebinner - don't worry about the return value since there may not be a rebinner in the system
	Rebinner.SetupRebinner (passall, lut);

	// create the acquisition control, preset timer, and statistics polling threads
	if (!StartTimerCallbackThreads ()) return false ;

	// start the preset timer
	if (!StartPresetTimer ()) return false ;

	// start the statistics poller
	if (!StartStatsPoll ()) return false ;

	// start the pft acquiring data
	ret = CPft::IoControl (IOCTL_PFT_ACQUIRE, (void*) NULL, 0, (void*) NULL, 0) ;
	
	// remember the start time and the high resolution counter freq
	QueryPerformanceCounter (&m_ScanStart) ;
	QueryPerformanceFrequency (&m_Frequency) ;

	return (ret) ;
}

//++
//
// CSinglesAcq::StopAcq - Stop the singles acquisition
//
// Inputs : None
//
// Outputs : bool, false = fail, true = success
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//
//--

bool
CSinglesAcq::StopAcq ()
{
	int rcode ;
	
	// first cancel the timers
	CancelWaitableTimer (m_presettimer) ;
	CancelWaitableTimer (m_timeouttimer) ;

	// now stop the acquisition
	rcode = CPft::IoControl (IOCTL_PFT_STOP, (void*)NULL, 0, (void*)NULL, 0) ;
	return ((bool)rcode) ;
}

//++
//
// CSinglesAcq::StartTimerCallbackThreads - Spin the threads to do the acquisition
//
// Inputs : NONE
//
// Outputs : true if OK else false
//
//--

bool
CSinglesAcq::StartTimerCallbackThreads ()
{
	DWORD dwThreadId ;
	void PresetWait (CSinglesAcq*), TimeoutWait (CSinglesAcq*) ;
	void AcqControl (CSinglesAcq*), PollStats (CSinglesAcq*) ;

	// Create the preset, timeout, and polling timers
	m_statstimer = CreateWaitableTimer ((LPSECURITY_ATTRIBUTES)NULL, (BOOL)FALSE, (LPCTSTR)NULL) ;
	m_presettimer = CreateWaitableTimer ((LPSECURITY_ATTRIBUTES)NULL, (BOOL)FALSE, (LPCTSTR)NULL) ;
	m_timeouttimer = CreateWaitableTimer ((LPSECURITY_ATTRIBUTES)NULL, (BOOL)FALSE, (LPCTSTR)NULL) ;

	// ok ??
	if ((m_statstimer == NULL) || (m_presettimer == NULL) || (m_timeouttimer == NULL)) return false ;

	// Presettime hread routine can't be part of the class but we can sneak "this" pointer to it
	if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) PresetWait, (LPVOID)this, 0, &dwThreadId))) return false ;

	// Timeout hread routine can't be part of the class but we can sneak "this" pointer to it
	if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) TimeoutWait, (LPVOID)this, 0, &dwThreadId))) return false ;

	// Poll statistics hread routine can't be part of the class but we can sneak "this" pointer to it
	if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) PollStats, (LPVOID)this, 0, &dwThreadId))) return false ;

	// Spin a thread to handle the buffer full notifications and storage
	if (!(CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) AcqControl, (LPVOID)this, 0, &dwThreadId))) return false ;

	return true ;
}

//++
//
// CSinglesAcq::StartStatsPoll - Start the acquisition statistics poll timer
//
// Inputs : NONE
//
// Outputs : true if OK else false
//
//--

bool
CSinglesAcq::StartStatsPoll ()
{
	// convert to timer time
	ConvertTime (2, &m_statspolltime) ;

	// start the periodic timer to go off every second
	SetWaitableTimer (m_statstimer, &m_statspolltime, 0, NULL, NULL, FALSE) ;

	return true ;
}

//++
//
// CSinglesAcq::StartPresetTimer - Start the acquisition preset timer
//
// Inputs : NONE
//
// Outputs : true if OK else false
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//
//--

bool
CSinglesAcq::StartPresetTimer ()
{
	// start the preset timer unless 0 which indicates no preset
	if (m_preset)
	{
		// convert to timer time
		ConvertTime (m_preset, &m_timertime) ;

		// start the timer
		SetWaitableTimer (m_presettimer, &m_timertime, 0, NULL, NULL, FALSE) ;
	}

	// start the timeout timer unless 0 which indicates no timeout
	if (m_timeout)
	{
		// convert to timer time
		ConvertTime (m_timeout, &m_timertime) ;

		// start the timer
		SetWaitableTimer (m_timeouttimer, &m_timertime, 0, NULL, NULL, FALSE) ;
	}

	return true ;
}

//++
//
// CSinglesAcq::ConvertTime - Convert seconds into system timer value
//
// Inputs : NONE
//
// Outputs : NONE
//
//--

void
CSinglesAcq::ConvertTime(unsigned long seconds, LARGE_INTEGER* timertime)
{
	const ULONG nNsecPerSec = 10000000 ;
	__int64 Nsecs ;

	// compute the number of 100 nsec ticks
	Nsecs = (__int64)seconds * (__int64)nNsecPerSec ;

	// negate it so it will be absolute time
	Nsecs = -Nsecs ;

	// stuff it in the LARGE_INTEGER
	timertime->LowPart = (DWORD)(Nsecs & 0xffffffff) ;
	timertime->HighPart = (DWORD)(Nsecs >> 32) ;

	return ;
}

//++
//
//
//	CSinglesAcq::ComputeScanTime	- Computes the time in seconds since
//									  the beginning of the scan
//
//	IN	(None)
//
//
//	Returns: unsigned long			- Time in Seconds
//
//--

unsigned long
CSinglesAcq::ComputeScanTime ()
{
	LARGE_INTEGER elapsed ;

	// get the current counter value
	QueryPerformanceCounter (&m_LastTimeCount) ;

	// compute the elapsed time
	elapsed.QuadPart = m_LastTimeCount.QuadPart - m_ScanStart.QuadPart ;

	// convert to seconds
	elapsed.QuadPart /= m_Frequency.QuadPart ;

	return ((unsigned long)elapsed.LowPart) ;
}

//++
//
// CSinglesAcq::CreateListmodeFile - Create the listmode file
//
// Inputs : NONE
//
// Outputs : true if OK else false
//
//--

bool
CSinglesAcq::CreateListmodeFile ()
{
	m_hfile = CreateFile (
					m_fname,
					GENERIC_READ | GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN,
					NULL
					) ;

	if (m_hfile == INVALID_HANDLE_VALUE) return false ;
	
	return true ;
}

//++
//
// AcqControl - Thread to control the double buffer listmode acquisition.
//				Because this is the entry point for a thread it cannot be
//				part of the class. It must be a global function
//
// Inputs : 'this' pointer to the CSinglesAcq object
//
// Outputs : NONE
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//										replaced m_quite with m_quiet
//
//--

// prototype for processing routine (in SinglesHistogram.dll)
__declspec(dllimport) long Histogram_Buffer (unsigned char* sbufr, long nbytes, long flag) ;

void
AcqControl (CSinglesAcq* op)
{
	HANDLE ReadyEvent ;
	PULONG kbufr, bufr0, bufr1, sbufr ;
	LMSTATS lmstats ;
	DWORD nbytes, bytes_written, wfstat ;
	BOOL iostat ;
	int acq_status, bnum ;
	bool nomoreflag = false ;
	long histo_stat ;
	OSVERSIONINFOEX osvi ;
	
	// Decide how we have to deal with the event (NT4 or W2K/XP)
	GetOsVersion (&osvi) ;

	// Is it NT
	if (osvi.dwMajorVersion <= 4)
	{
		// open the named notification event
		if (!(ReadyEvent = OpenEvent (SYNCHRONIZE, FALSE, "LmBufrRdyEvent"))) return ;

		// Make sure the event is cleared
		ResetEvent (ReadyEvent) ;
	}

	// create the event for Windows 2K or XP
	else if (osvi.dwMajorVersion == 5)
	{

		// Create the notification event
		ReadyEvent = CreateEvent (NULL,
								  TRUE,
								  FALSE,
								  "LmBufrReady"
								  ) ;

		if (!ReadyEvent) return ;

		// Send the event to the driver so we are hooked up for notification
		iostat = op->IoControl (IOCTL_PFT_SET_NOTIFICATION_EVENT, (HANDLE*)&ReadyEvent, sizeof (HANDLE), (void*) NULL, 0) ;
		if (!iostat)
		{
			CloseHandle (ReadyEvent) ;
			return ;
		}
	}

	// map the kernel mode buffer into this processes memory space
	op->IoControl (IOCTL_GET_MAPPED_BUFFER, (void*) NULL, 0, (void*) &kbufr, sizeof (PULONG)) ;

	// if we got this far the acquisition is active
	acq_status = ACQ_ACTIVE ;
	bufr0 = kbufr ;
	bufr1 = kbufr + 0x100000 ;
	sbufr = bufr0 ;
	bnum = 0 ;

	// now we go back and forth with the driver until the scan is done
	if (!op->m_quiet) cout << "AcqControl: Entering while..." << endl ;

	// loop until scan is done
	while (acq_status != ACQ_STOPPED)
	{
		// wait on the buffer to be acquired
		if (op->m_debug) cout << "AcqControl: Waiting for event..." << endl ;
		WaitForSingleObject (ReadyEvent, INFINITE) ;

		// bingo, let's see what happened
		op->IoControl (IOCTL_GET_CB_INFO, (void*) &lmstats, 0, (void*) &lmstats, sizeof (LMSTATS)) ;
		if (op->m_debug) cout << "AcqControl: Got it... status = " << lmstats.status << " lmstats.events = " << lmstats.events << endl ;

		// remember the acquisition status
		acq_status = lmstats.status ;

		// if this is the last buffer close file and reopen in normal
		// mode to handle non multiples of sector size writes
		if (acq_status != ACQ_ACTIVE)
		{
			if (op->m_debug) cout << "AcqControl: Closing file and reopening..." << endl ;

			// set the nomoreflag
			nomoreflag = true ;

			// if we are using a file
			if (op->m_makefile)
			{
				// close it
				CloseHandle (op->m_hfile) ;

				// reopen it
				op->m_hfile = CreateFile (
									op->m_fname,
									GENERIC_READ | GENERIC_WRITE,
									0,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL ) ;

				// set file pointer to end of file
				SetFilePointer (op->m_hfile, 0, NULL, FILE_END) ;
			}
		}

		if (op->m_makefile)
		{
			// write the buffer to disk if we are using a file
			if (op->m_debug) cout << "AcqControl: Writing buffer " << bnum << " to disk..." << endl ;
			nbytes = lmstats.events * 4 ;
			wfstat = WriteFile (op->m_hfile, (LPCVOID)sbufr, nbytes, &bytes_written, NULL) ;
		}
		else
		{
			// send it to the singles histogrammer
			nbytes = lmstats.events * 4 ;
			histo_stat = Histogram_Buffer ((unsigned char*)sbufr, (long)nbytes, (long)nomoreflag) ;
			if (histo_stat)
			{
				if (op->m_debug) cout << "SinglesAcq: AcqControl... Inside histogrammer halt" << endl ;

				// histogrammer says we need to stop, cancel timers
				CancelWaitableTimer (op->m_presettimer) ;
				CancelWaitableTimer (op->m_timeouttimer) ;

				// stop the acquisition
				if (op->m_debug) cout << "SinglesAcq: AcqControl... Stopping in histogrammer halt" << endl ;
				op->Stop () ;
				break ;
			}
		}

		// go tell the driver the buffer is free
		op->IoControl (IOCTL_PFT_TAG_BUFFER_FREE, (void*) NULL, 0, (void*) NULL, sizeof (LMSTATS)) ;
		if (op->m_debug) cout << "AcqControl: Mapped buffer freed..." << endl ;

		// toggle the buffer
		bnum++ ;
		if (bnum & 1) sbufr = bufr1 ;
		else sbufr = bufr0 ;

		// make sure we have the latest acquisition status
		op->IoControl (IOCTL_GET_CB_INFO, (void*) &lmstats, 0, (void*) &lmstats, sizeof (LMSTATS)) ;
		if (op->m_debug) cout << "AcqControl: Got it... status = " << lmstats.status << " lmstats.events = " << lmstats.events << endl ;

		// remember the acquisition status
		acq_status = lmstats.status ;
	}

	if (op->m_debug) cout << "AcqControl: Acquire loop finished..." << endl ;

	// release the user mapped buffer and indicate that the scan is complete
	op->ReleaseBuffer () ;
	CloseHandle (ReadyEvent) ;
	SetEvent (op->m_hscancomplete) ;
}

//++
//
// PresetWait - Thread to wait for the timer preset signal and issue stop.
//				Because this is the entry point for a thread it cannot be
//				part of the class. It must be a global function
//
// Inputs : 'this' pointer to the CSinglesAcq object
//
// Outputs : NONE
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//			 T Gremillion	02 Dec 03	removed extraneous loop and break
//
//--

void
PresetWait (CSinglesAcq* op)
{
	// wait here for the timer to expire
	WaitForSingleObjectEx (op->m_presettimer, INFINITE, TRUE) ;

	// stop the acquisition
	op->IoControl (IOCTL_PFT_STOP, (void*) NULL, 0, (void*) NULL, 0) ;
}

//++
//
// TimeoutWait - Thread to wait for the timeout signal and issue stop.
//				 Because this is the entry point for a thread it cannot be
//				 part of the class. It must be a global function
//
// Inputs : 'this' pointer to the CSinglesAcq object
//
// Outputs : NONE
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//			 T Gremillion	02 Dec 03	removed extraneous loop and break
//
//--

void
TimeoutWait (CSinglesAcq* op)
{
	// wait here for the timer to expire
	WaitForSingleObjectEx (op->m_timeouttimer, INFINITE, TRUE) ;

	// stop the acquisition
	op->IoControl (IOCTL_PFT_STOP, (void*) NULL, 0, (void*) NULL, 0) ;
}

//++
//
// PollStats -  Thread to wait for the poll timer signal and get acquisition stats.
//				Because this is the entry point for a thread it cannot be
//				part of the class. It must be a global function
//
// Inputs : 'this' pointer to the CSinglesAcq object
//
// Outputs : NONE
//
// Modified: T Gremillion	01 Dec 03	removed commented out code
//										replaced m_quite with m_quiet
//
//--

void
PollStats (CSinglesAcq* op)
{
	int status = ACQ_ACTIVE;
	LMSTATS lmstats ;
	unsigned long timeremaining ;

	while (status == ACQ_ACTIVE)
	{
		// wait here for the timer to expire
		WaitForSingleObjectEx (op->m_statstimer, INFINITE, TRUE) ;

		// Get the info
		op->IoControl (IOCTL_GET_CB_INFO, (void*) &lmstats, 0, (void*) &lmstats, sizeof (LMSTATS)) ;

		// store acquisition status
		status = lmstats.status ;

		// compute time remaining
		timeremaining = op->m_preset - (op->ComputeScanTime ()) ;

		// cancel the timeout timer if we have any counts
		if (lmstats.total_events > (__int64)0) CancelWaitableTimer (op->m_timeouttimer) ;


		// printout for debug
		if (!op->m_quiet) cout << "status = " << lmstats.status << " " ;
		if (!op->m_quiet) cout << "time_remaining = " << timeremaining << " seconds" << endl ;
		if (!op->m_quiet) printf ("total_events = %I64d \n", lmstats.total_events) ;
		if (!op->m_quiet) cout << "event_rate = " << lmstats.event_rate << endl ;
		if (!op->m_quiet) cout << "events = " << lmstats.events << endl ;

		// restart the poll timer
		SetWaitableTimer (op->m_statstimer, &op->m_statspolltime, 0, NULL, NULL, FALSE) ;
	}

	// Cancel the timer and clean up
	CancelWaitableTimer (op->m_statstimer) ;
	timeremaining = 0 ;

	// print again
	if (!op->m_quiet) cout << "PollStats: Exited polling loop ..." << endl ;
	if (!op->m_quiet) cout << "status = " << lmstats.status << endl ;
	if (!op->m_quiet) printf ("total_events = %I64d\n", lmstats.total_events) ;
	if (!op->m_quiet) printf ("event_rate = %I64d\n", lmstats.event_rate) ;
	if (!op->m_quiet) cout << "time_remaining = " << timeremaining << endl ;
	if (!op->m_quiet) cout << "events = " << lmstats.events << endl ;
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
*	History:		- 11/05/03 TGG - copied from listmode.cpp
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

