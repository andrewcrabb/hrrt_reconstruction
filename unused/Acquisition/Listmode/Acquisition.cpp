//*********************************************************************
//
// Acquisition.cpp - CAcquisition class implementation
//
// Revision History :
//	jhr, modified	17mar02	-	Made to work with Systran or PFT
//
//*********************************************************************

/*********************************************************************
HRRT Users community modification history
08jun2009: Added countrate testing code ifdef MS_TESTING
/*********************************************************************/

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <iostream.h>
#include "acs3acq.h"
#include "TfaIoctl.h"
#include "RbIoctl.h"


#ifdef MS_TESTING
static char lm_info_fname[MAX_PATH];
static FILE *fp_info=NULL;
#endif

//#include "PetCTGCIDll.h"

/*********************************************************************
*
*
*	CAcquisition::CAcquisition		- Constructor
*
*	IN	(None)
*	OUT	(None)
*
*	Returns: void
*
**********************************************************************/

CAcquisition::CAcquisition()
{
	// set the initial state
	LmState = UNINITIALIZED ;

	// get the performance counter frequency
	QueryPerformanceFrequency (&m_Frequency) ;
}

/*********************************************************************
*
*
*	CAcquisition::ConvertTime		- Convert seconds into system timer value
*
*	IN	unsigned long seconds		- Time in seconds to convert
*	OUT	LARGE_INTEGER*	timertime	- Where to store the Win32 timer value
*
*	Returns: void
*
**********************************************************************/

void
CAcquisition::ConvertTime(unsigned long seconds, LARGE_INTEGER* timertime)
{
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

/*********************************************************************
*
*
*	CAcquisition::ComputeScanTime	- Computes the time in seconds since
*									  the beginning of the scan
*
*	IN	(None)
*	OUT	(None)
*
*	Returns: int					- Time in Seconds
*
**********************************************************************/

int
CAcquisition::ComputeScanTime ()
{
	LARGE_INTEGER elapsed ;

	// get the current counter value
	QueryPerformanceCounter (&m_LastTimeCount) ;

	// compute the elapsed time
	elapsed.QuadPart = m_LastTimeCount.QuadPart - m_ScanStart.QuadPart ;

	// convert to seconds
	elapsed.QuadPart /= m_Frequency.QuadPart ;

	return ((int)elapsed.LowPart) ;
}

/*********************************************************************
*
*
*	CAcquisition::ResetRebinner		- Reset the hardware rebinner
*
*	IN	int lut						- Rebinner lookup table
*	IN	int bypass					- Indicates bypass or not
*
*	OUT	(None)
*
*	Returns: void
*
**********************************************************************/

void
CAcquisition::ResetRebinner (int lut, int bypass)
{
	int i ;
	DWORD t0 ;
	__int64 RbWiVal ;

	for (i=0;i<4;i++)
	{
		// sequence the reset loop
		if (i == 0) RbWiVal = RbReset ;
		if (i == 1 || i == 2) RbWiVal = RbTag ;
		if (i == 3)
		{
			RbWiVal = RbCTW ;
			if (lut == 1) RbWiVal |= 0x2 ;
			if (LmBypass) RbWiVal |= 0x4 ;
		}

		// insert the 64 bit value
		pReb->WordInsert (RbWiVal) ;

		// wait about 50 msec if reset
		if (i == 0)
		{
			t0 = GetTickCount () ;
			while ((GetTickCount () - t0) <= 50) ;
		}
	}
}

/*********************************************************************
*
*
* CAcquisition::SyncRebinner - This routine makes sure the TFA and
*								Rebinner fibre connection is in sync.
*
*	IN	int lut						- Rebinner lookup table
*	IN	int bypass					- Indicates bypass or not
*
*	OUT	(None)
*
*	Returns: true if successful or false if fail
*
**********************************************************************/

bool
CAcquisition::SyncRebinner (int lut, int bypass)
{
	__int64 RbWiVal, passall, ctw ;
	int i, dr3, ntimes, passall_set = 0x00004000 ;
	bool done = FALSE, status = TRUE ;
	DWORD t0 ;
	//char gciresp[16] ;

	// turn off IPCP
	//GCIpassthru ("GA64J3", gciresp) ;
	//if (gciresp[0] != 'N') return (FALSE) ;
	//cout << "SyncRebinner: GA64J3 responded with " << gciresp << endl ;

	// try 10 times
	ntimes = 10 ;
	passall = (__int64)RbPassall ;
	ctw = (__int64)RbCTW ;

	// validation sequence
	do
	{
		// reset the rebinner
		cout << "SyncRebinner: Entering rebinner reset loop..." << endl ;
		for (i=0;i<4;i++)
		{
			if (i == 0) RbWiVal = RbReset ;
			if (i == 1 || i == 2) RbWiVal = RbTag ;
			if (i == 3)
			{
				RbWiVal = RbCTW ;
				if (lut == 1) RbWiVal |= 0x2 ;
				if (bypass) RbWiVal |= 0x4 ;
			}

			// insert the 64 bit value
			pReb->WordInsert (RbWiVal) ;

			// wait about 50 msec if reset
			if (i == 0)
			{
				t0 = GetTickCount () ;
				while ((GetTickCount () - t0) <= 50) ;
			}
		}

		// Insert a 'vanilla tag word' - latest rebinner sequence 03feb03-jhr
		RbWiVal = RbTag;
		pReb->WordInsert (RbWiVal) ;

		// Now insert a passall control tagword into the TFA
		cout << "SyncRebinner: Sending a PASSALL control tag to TFA..." << endl ;
		pTfa->WordInsert (passall) ;

		// To read the rebinner passall bit we need to select diagnostic register 3
		pReb->WritePort (IOCTL_RB_WRITE_PORT, RB_SR, 0xa000) ;
		pReb->ReadPort (IOCTL_RB_READ_PORT, RB_DR3, &dr3) ;

		// if it set clear it
		if (dr3 & passall_set)
		{
			// Now we'll clear the passall bit
			cout << "SyncRebinner: Sending a CLEAR PASSALL control tag to TFA..." << endl ;
			pTfa->WordInsert (ctw) ;
			pReb->WritePort (IOCTL_RB_WRITE_PORT, RB_SR, 0xa000) ;
			pReb->ReadPort (IOCTL_RB_READ_PORT, RB_DR3, &dr3) ;
			if (!(dr3 & passall_set)) done = TRUE ;
		}
	} while (!done && ntimes--) ;

	// set the rebinner look up table and bypass
	cout << "SyncRebinner: Setting rebinner lookup table..." << endl ;
	pTfa->WordInsert (RbWiVal) ;

	// turn the IPCP back on
	//GCIpassthru ("GA64J2", gciresp) ;
	//cout << "LmSyncRebinner: GA64J2 responded with " << gciresp << endl ;
	//if (gciresp[0] != 'N') status = FALSE ;

	//GCIpassthru ("GA64J1", gciresp) ;
	//if (gciresp[0] != 'N') status = FALSE ;
	//cout << "LmSyncRebinner: GA64J1 responded with " << gciresp << endl ;
	
	cout << "LmSyncRebinner: packet sync = " << done << "  ntimes = " << (10 - ntimes) + 1 << " status = " << status << endl ;
	
	if (!done) return (FALSE) ;
	else return (status) ;
}

/*********************************************************************
*
*
*	CAcquisition::CreateLmFile	- Create a raw listmode file
*
*	IN	(None)
*	OUT	(None)
*
*	Returns: true if success or false if fail
*
**********************************************************************/

bool
CAcquisition::CreateLmFile ()
{
	// create the file with NO_BUFFERING and SEQUENTIAL
	// to get the maximum throughput
	OutputDebugString("Listmode : CreateLMFile is called\n");
	if ((m_hFile = CreateFile (
							LmFile,
							GENERIC_READ | GENERIC_WRITE,
							0,
							NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN,
							NULL
							)) == INVALID_HANDLE_VALUE) return false ;
#ifdef MS_TESTING
	sprintf(lm_info_fname,"%s_info.txt", LmFile);
	fp_info = fopen(lm_info_fname,"wt");
#endif

	return true ;
}

/*********************************************************************
*
*
*	CAcquisition::ReopenLmFile	- This method closes the Listmode file
*								  and reopens it with buffering so we
*								  write the last buffer that may not
*								  be a multiple of the sector size
*
*	IN	(None)
*	OUT	(None)
*
*	Returns: true if success or false if fail
*
**********************************************************************/

bool
CAcquisition::ReopenLmFile ()
{

	DWORD dwSizeLow, dwSizeHigh=NULL;
	long lSizeHigh;
	OutputDebugString("Listmode : ReopenLmFile is called\n");
	// close the current file
	CloseHandle (m_hFile) ;

	// reopen in standard mode
	m_hFile = CreateFile (
						LmFile,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL ) ;

	//if (hLmAcqFile == INVALID_HANDLE_VALUE) ;  we must error here

	// move the file pointer to the end
	//SetFilePointer (m_hFile, 0, NULL, FILE_END) ;
	//ZYB
	dwSizeLow = GetFileSize (m_hFile, &dwSizeHigh) ; 
	lSizeHigh = (long)dwSizeHigh;
	// move the file pointer to the end
	DWORD ret = SetFilePointer (m_hFile, dwSizeLow, &lSizeHigh, FILE_BEGIN) ;

	return (true) ;
}

#ifdef MS_TESTING
#include <time.h>

// Current time formating
static const char *format_time(time_t t)
{
	static char time_str[20];
	struct tm *ptm;
	time_t now;

	if (t==0) time(&now);
	else now = t;
	ptm = localtime(&now);
	sprintf(time_str,"%02d%02d%02d%02d%02d%02d",(1900+ptm->tm_year)%100,
		ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min,ptm->tm_sec);
	return time_str;
}
#endif

/*********************************************************************
*
*
*	CAcquisition::WriteLmFile	- This method write raw listmode data
*								  to the previously opened file.
*
*	IN	LPCVOID address			- Addres of data to write
*	IN	int nbytes				- Number of bytes to write
*
*	OUT	DWORD* bytes_written	- Number of bytes written
*	OUT	int* err				- pointer to store the Win32 error
*								  code if operation fails
*
*	Returns: True if success or false if fail. If a failure occurs
*			 err will contain the Win32 error code.
*
**********************************************************************/


BOOL
CAcquisition::WriteLmFile (LPCVOID address, DWORD nbytes, LPDWORD bytes_written, int* err) // ZYB
//CAcquisition::WriteLmFile (LPCVOID address, int nbytes, LPDWORD bytes_written, int* err)
{
#ifdef MS_TESTING
	static time_t t0=0;
	static int buf_count=0;
	static __int64 total_count=0;
	time_t t;

	time(&t);
	total_count += nbytes;
	buf_count++;
	if (t != t0 && fp_info != NULL) {
		fprintf(fp_info, "%s: %-6d   %I64d\n", format_time(t), buf_count, total_count);
		t0= t;
		fflush(fp_info);
	}
	return true;
#else
	BOOL wfstat;
	//char z[32];

	// ZYB
	*err = 0 ;

	wfstat = WriteFile (m_hFile, address, nbytes, bytes_written, NULL) ;
	if (!wfstat) *err = GetLastError () ;
	// ZYB
	//sprintf(z,"ListMode : Data to Write = %u\n",nbytes);
	//OutputDebugString(z);
	return (wfstat) ;
#endif
}

/*********************************************************************
*
*
*	CAcquisition::CloseLmFile	- Close the listmode file
*
*	IN	(None)
*	OUT	(None)
*
*	Returns: void
*
**********************************************************************/

void
CAcquisition::CloseLmFile ()
{
	CloseHandle (m_hFile) ;
#ifdef MS_TESTING
	if (fp_info != NULL) fclose (fp_info);
#endif
}
