/**********************************************************************************/
//
// PetCTGCI.cpp -	These are the mid level routines that make up the DLL for the
//					PetCT Gantry Controller Interface..
//
//		Created:	jhr - 03feb01
//		Modified:	jhr - 12sep01 - Added Phase II
//		Modified:	jhr - 07dec01 - Added Phase III
//		Modified:	sft - 26feb02 - Modified op_zap_bucket added check of the 
//									gantry model object. (Fix to PR:2788)
//
/**********************************************************************************/

// The compiler wants this
#define _WIN32_WINNT 0x0400


#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <iostream.h>
#include <fcntl.h>
#include <io.h>
#include "gci.h"
#include "GCIResponseCodes.h"
#include "ServMgrCommon.h"
#include "scanner.h"
#include "ServQCMsgID.h"
#include "gantryModelClass.h"

#define PETCTGCIDLLEXPORT
#include "PetCTGCIdll.h"

#ifndef LOCAL
#define LOCAL static
#endif

// Include LOCAL forward declaration prototypes
#include "GciDecl.h"

// default ACS IP address
#define ACS_IP_ADDR "1.1.1.3"

// pipe name holders
LOCAL char GciClntPipe[64]         = {'\0'} ;
LOCAL char GciClntSinglesPipe[64]  = {'\0'} ;
LOCAL char GciClntRatesPipe[64]    = {'\0'} ;
LOCAL char GciClntAsyncPipe[64]    = {'\0'} ;

// the gantry model
LOCAL CGantryModel* scanner = NULL ;
LOCAL CGantryModel gm ;

// acquisition things
LOCAL CCallback* GCICallback ;			// Notification callback for asynchronous messages

// Asynchronous message thread handle
LOCAL HANDLE GCIAsyncThread = NULL ;	// Thread for asynchronous messages

// GCI thread handles for gantry service tools
LOCAL HANDLE HistThread = NULL ;		// Thread for doing block histogramming
LOCAL HANDLE GSThread = NULL ;			// Thread executing a gantry settings operation

// LOCAL storage
LOCAL HWND hWnd = NULL ;				// Window handle for 'posting messages'
LOCAL HISTARGS HistArgs ;				// structure to histogram arguments
LOCAL GSARGS GSArgs ;					// structure for gantry settings operation

// LOCAL things
LOCAL BOOL GCIbug = FALSE ;				// Debug mode flag
LOCAL BOOL GCIfail = FALSE ;			// Generate a failure in debug mode

LOCAL BOOL inited = FALSE ;

/***********************************************************************************/
//
//
//	Function:	GCIDebug - Sets/Clears debug flags for printouts during debug
//
//	Inputs:		BOOL bug
//				BOOL fail
//
//	Outputs:	None
//
//	Returns:	Void
//
/***********************************************************************************/

void
GCIdebug (BOOL bug, BOOL fail)
{
	if (bug) GCIbug = TRUE ;
	else GCIbug = FALSE ;

	if (fail) GCIfail = TRUE ;
	else GCIfail = FALSE ;

	::MessageBox (NULL, "Inside GCIdebug", NULL, MB_OK) ;
}

/***********************************************************************************/
//
//
//	Function:	GciInit - Store pointer to the passed in gantry model object
//
//	Inputs:		CGantryModel* gm
//
//
//	Outputs:	None
//
//	Returns:	True | False if the pointer has already been initialized
//
/***********************************************************************************/

BOOL
GciInit (CGantryModel* gm)
{
	if (!gm) return FALSE ;
	else scanner = gm ;

	return TRUE ;
}

/***********************************************************************************/
//
//
//	Function:	GciAcqInit - Initializes the GCI for data acquisition
//
//	Inputs:		char* host				- host name (no longer used)
//				CCallback* callback		- pointer to callback for interaction
//
//
//	Outputs:	None
//
//	Returns:	True | False if the thread to handle async messages cannot start
//
/***********************************************************************************/

BOOL
GciAcqInit (char* host, CCallback* callback)
{
	DWORD threadID ;

	// remember callback and hostname
	GCICallback = callback ;

	// start a thread to listen to async messages unless one is running
	if (GCIAsyncThread == NULL)
	{
		if (!(GCIAsyncThread = CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
										  0,
										  (LPTHREAD_START_ROUTINE)GCIAsyncRead,
										  (LPVOID)NULL,
										  0,
										  &threadID))) return (FALSE) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	GciEnergyQuery	- Query the bucket for energy settings
//
//	Inputs:		int len			- (N/A, replaced with model object)
//				int *uld		- pointer to array to return uld settings
//				int *lld		- pointer to array to return lld settings
//				int *scatter	- pointer to array to return scatter settings
//				int index		- if -1 do all buckets otherwise used as a bucket number
//
//
//	Outputs:	uld, lld, scatter
//
//	Returns:	True | False if anything during query fails
//
/***********************************************************************************/

BOOL
GciEnergyQuery (int len, int *uld, int *lld, int *scatter, int index = -1)
{
	int i, j, nbkts, bindex, start_bucket, end_bucket, energy, *aptr ;
	char rc, bcmd, cmd[10], resp[10] ;
	BOOL status = TRUE ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return FALSE ;
	}

	// get the arrays
	nbkts = scanner->buckets() ;
	int* blld = new int[nbkts] ;
	int* buld = new int[nbkts] ;
	int* bse = new int[nbkts] ;
	
	// initialize the energy arrays
	for (i=0;i<nbkts;i++)
	{
		blld[i] = 0 ;
		buld[i] = 0 ;
		bse[i] = 0 ;
	}

	// determine which bucket(s)
	if (index < 0)
	{
		start_bucket = 0 ;
		end_bucket = nbkts ;
	}
	else
	{
		start_bucket = index ;
		end_bucket = index + 1 ;
	}

	// query for the settings
	for (j=0;j<3;j++)
	{
		for (bindex=start_bucket;bindex<end_bucket;bindex++)
		{
			switch (j)
			{
				case 0:
					bcmd = 'Y' ;
					aptr = &bse[bindex] ;
					break ;
				case 1:
					bcmd = 'V' ;
					aptr = &blld[bindex] ;
					break ;
				case 2:
					bcmd = 'U' ;
					aptr = &buld[bindex] ;
					break ;
			}

			// get energy levels
			sprintf (cmd, "%d%c0", bindex, bcmd) ;
			send_command (BKT, cmd, resp) ;
			parse_fesb_response (resp, &rc, &energy) ;
			if ((rc == 'X') || (rc == 'E')) status = FALSE ;
			*aptr = energy ;
		}
	}

	// setup the return
	if (index < 0)
	{
		for (i=0;i<nbkts;i++)
		{
			scatter[i] = bse[i] ;
			lld[i] = blld[i] ;
			uld[i] = buld[i] ;
		}
	}
	else
	{
		*scatter = bse[bindex-1] ;
		*lld = blld[bindex-1] ;
		*uld = buld[bindex-1] ;
	}

	// free everything
	delete [] blld ;
	delete [] buld ;
	delete [] bse ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	GciEnergySetup	- Set the bucket energy levels
//
//	Inputs:		int uld			- uld value
//				int lld			- uld value
//				int scatter		- uld value
//				int index		- if -1 set all buckets otherwise used as a bucket number
//
//
//	Outputs:	None
//
//	Returns:	True | False if anything during set operation fails
//
/***********************************************************************************/

BOOL
GciEnergySetup (int uld, int lld, int scatter, int index = -1)
{
	int i, j, bindex, start_bucket, end_bucket, rval, energy, scatter_read ;
	int sdesc[24], lldesc[24],  uldesc[24], *descptr, current_energy ;
	char rc, bcmd, cmd[10], resp[10] ;
	BOOL status = TRUE, done ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return FALSE ;
	}


	// determine which bucket(s)
	if (index < 0)
	{
		start_bucket = 0 ;
		end_bucket = scanner->buckets() ;
	}
	else
	{
		start_bucket = index ;
		end_bucket = index + 1 ;
	}

	// get the energies
	if (!GciEnergyQuery (24, uldesc, lldesc, sdesc, index)) return (FALSE) ;

	// set the energy levels and check
	for (j=0;j<3;j++)
	{
		switch (j)
		{
			case 0:
				bcmd = 'Y' ;
				if (scatter == 0) scatter = 1 ;
				energy = scatter ;
				descptr = &sdesc[0] ;
				break ;
			case 1:
				bcmd = 'V' ;
				energy = lld ;
				descptr = &lldesc[0] ;
				break ;
			case 2:
				bcmd = 'U' ;
				energy = uld ;
				descptr = &uldesc[0] ;
				break ;
		}

		for (bindex=start_bucket;bindex<end_bucket;bindex++)
		{
			if (index >= 0) current_energy = descptr[0] ;
			else current_energy = descptr[bindex] ;

			// send command and parse response
			for (i=0;i<3;i++)
			{
				// special case for scatter
				if ((bcmd == 'Y') && (energy == 1) && (current_energy == 0)) break ;
				if (energy == current_energy) break ;
				sprintf (cmd, "%d%c%d", bindex, bcmd, energy) ;
				send_command (BKT, cmd, resp) ;
				parse_fesb_response (resp, &rc, &rval) ;
				if ((rc != 'N') && (rc != 'D')) Sleep (ENERGYWAIT) ;
				else break ;
			}
			if (i >= 3) status = FALSE;
		}
	}

	// take into account the scatter disable thing
	if (scatter == 1) scatter_read = 0 ;
	else scatter_read = scatter ;

	// Now wait a little while for things to settle and check
	done = FALSE ;
	i = 0 ;
	while (!done)
	{
		Sleep (ENERGYWAIT*2) ;
		if (!GciEnergyQuery (24, uldesc, lldesc, sdesc, index)) return (FALSE) ;
		done = TRUE ;
		for (bindex=start_bucket;bindex<end_bucket;bindex++)
		{
			if (sdesc[bindex] != scatter_read) done = FALSE ;
			if (lldesc[bindex] != lld) done = FALSE ;
			if (uldesc[bindex] != uld) done = FALSE ;
		}
		if (++i >= 2)
		{
			status = FALSE ;
			break ;
		}
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	GciSingles	- Turn On/Off singles polling
//
//	Inputs:		BOOL flag	- if true turn on, false turn off
//
//	Outputs:	None
//
//	Returns:	True | False if unsuccessful
//
/***********************************************************************************/

BOOL
GciSingles (BOOL flag)
{
	int status ;

	if (flag) status = enable_singles_polling () ;
	else status = disable_singles_polling () ;
	if (status == GCI_SUCCESS) return TRUE ;
	else return FALSE ;
}

/***********************************************************************************/
//
//
//	Function:	GciPollSingles	- Poll for bucket singles
//
//	Inputs:		int* singles	- pointer to singles storage area
//				int* prompts	- pointer to store gantry prompts 
//				int* randoms	- pointer to store gantry randoms
//				int* scatter	- pointer to store gantry scatter
//
//
//	Outputs:	singles, prompts, randoms, scatter
//
//	Returns:	True | False if unsuccessful
//
/***********************************************************************************/

BOOL
GciPollSingles (int* singles, int* prompts, int* randoms, int* scatter)
{
	HANDLE hPipe ;
	char resp[128] ;
	int rval, hdr, bkt, nbkts, s1, s2, s3, s4, nargs ;
	DWORD nbytes ;
	BOOL done, status = TRUE ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return FALSE ;
	}

	// initialize the pipe name... once only
	if (!GciClntSinglesPipe[0]) sprintf (GciClntSinglesPipe, "\\\\%s\\pipe\\GCISinglesPipe", ACS_IP_ADDR) ;

	// only one instance of this pipe is allowed at a time
	// so establish the connection before sending command
	if (!WaitNamedPipe (GciClntSinglesPipe, NMPWAIT_WAIT_FOREVER)) return (FALSE) ;
	if ((hPipe = CreateFile (GciClntSinglesPipe,
							 GENERIC_READ,
							 0,
							 NULL,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_READONLY,
							 NULL)) == INVALID_HANDLE_VALUE) return (FALSE) ;

	// got the pipe so send the command
	send_command (GC, "P0", resp) ;
	if ((parse_gc_response (resp, &rval)) == GCI_ERROR)
	{
		CloseHandle (hPipe) ;
		return (FALSE) ;
	}

	// now wait for the asynchronous information
	nbkts = scanner->buckets() ;
	done = FALSE ;
	while (!done)
	{
		if (ReadFile (hPipe, resp, 128, &nbytes, NULL))
		{
			hdr = get_hdr_code (resp) ;
			switch (hdr)
			{
				case SN_HDR:
					//cout << "Processing... " << resp << endl ;
					nargs = sscanf (&resp[3], "%d,%d,%d,%d,%d", &bkt, &s1, &s2, &s3, &s4) ;
					singles[bkt] = s1 ;
					bkt += nbkts ;
					singles[bkt] = s2 ;
					bkt += nbkts ;
					singles[bkt] = s3 ;
					bkt += nbkts ;
					singles[bkt] = s4 ;
					bkt += nbkts ;
					break ;
				case IP_HDR:
					//cout << "Processing... " << resp << endl ;
					nargs = sscanf (&resp[3], "%d,%d,%d", prompts, randoms, scatter) ;
					done = TRUE ;
					break ;
				default:
					cout << "GciPollSingles: Unknown header... " << resp << endl ;
			}
		}
		else status = FALSE ;
	}

	//cout << "Closing pipe..." << endl ;
	CloseHandle (hPipe) ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	GciPollInstantaneous - Poll gantry for instantaneous rates
//
//	Inputs:		int* corsingles		- pointer to corrected singles storage area
//				int* unccorsingles	- pointer to uncorrected singles storage area
//				int* prompts		- pointer to return prompts 
//				int* randoms		- pointer to return prompts
//				int* scatter		- pointer to return prompts
//
//
//	Outputs:	corsingles, uncorsingles, prompts, randoms, scatter
//
//	Returns:	True | False if unsuccessful
//
/***********************************************************************************/

BOOL
GciPollInstantaneous (int* corsingles, int* uncorsingles, int* prompts, int* randoms, int* scatter)
{
	HANDLE hPipe ;
	char resp[128] ;
	int rval, hdr, nargs ;
	DWORD nbytes ;
	BOOL status = TRUE ;

	// initialize the rate pipe... once only
	if (!GciClntRatesPipe[0]) sprintf (GciClntRatesPipe, "\\\\%s\\pipe\\GCIRatesPipe", ACS_IP_ADDR) ;

	// only one instance of this pipe is allowed at a time
	// so establish the connection before sending command
	if (!WaitNamedPipe (GciClntRatesPipe, NMPWAIT_WAIT_FOREVER)) return (FALSE) ;
	if ((hPipe = CreateFile (GciClntRatesPipe,
							 GENERIC_READ,
							 0,
							 NULL,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_READONLY,
							 NULL)) == INVALID_HANDLE_VALUE) return (FALSE) ;

	// got the pipe so send the command
	send_command (GC, "P1", resp) ;
	if ((parse_gc_response (resp, &rval)) == GCI_ERROR)
	{
		CloseHandle (hPipe) ;
		return (FALSE) ;
	}

	// now wait for the asynchronous information
	if (ReadFile (hPipe, resp, 128, &nbytes, NULL))
	{
		hdr = get_hdr_code (resp) ;
		switch (hdr)
		{
			case IR_HDR:
				nargs = sscanf (&resp[3], "%d,%d,%d,%d,%d", corsingles, uncorsingles, prompts, randoms, scatter) ;
				break ;
			default:
				cout << "GciPollInstantaneous: Unknown header... " << resp << endl ;
		}
	}
	else status = FALSE ;

	CloseHandle (hPipe) ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	GciPing			- ping gantry components for health check
//
//	Inputs:		BOOL& status	- reference to return status
//
//	Outputs:	status
//
//	Returns:	True | False if improper response from gantry components
//
/***********************************************************************************/

BOOL
GciPing (BOOL& status)
{
	char resp[10], rc ;
	char GCcmd [] = "B2" ;
	char CPcmd [] = "64B1" ;
	char BKTcmd[] = "0X4" ;
	int arg, hv ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return (GCI_ERROR) ;
	}
	status = TRUE ;

	// talk to the gantry controller
	send_command (GC, GCcmd, resp) ;
	if ((parse_gc_response (resp, &arg)) == GCI_ERROR) status = FALSE ;

	// talk to the coincidence processor
	send_command (IPCP, CPcmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'N') status = FALSE ;

	// see if the high voltage is up
	if (scanner->bucketOutputChannels() == 1) hv = 1400 ;
	else hv = 950 ;
	send_command (BKT, BKTcmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'H') status = FALSE ;
	if (arg < hv) status = FALSE ;

	// return OK
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	GCIpassthru	- send a passthru command to gantry
//
//	Inputs:		char* cmd	- pointer to command to send
//				char* resp	- pointer to area to store response
//
//	Outputs:	resp
//
//	Returns:	0 (Zero)
//
/***********************************************************************************/

int
GCIpassthru (char *cmd, char *resp)
{
	char tbuf[64] ;

	send_command (PT, cmd, tbuf) ;
	if (!strncmp ("GT", tbuf, 2)) strcpy (resp, &tbuf[3]) ;
	else strcpy (resp, tbuf) ;

	return (0) ;
}

/***********************************************************************************/
//
//
//	Function:	disable_high_voltage	- disables gantry high voltage
//
//	Inputs:		None
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | GCI_ERROR if failed
//
/***********************************************************************************/

int
disable_high_voltage ()
{
	char cmd[] = "PW2", resp[10] ;
	int arg, status ;

	send_command (GC, cmd, resp) ;
	status = parse_gc_response (resp, &arg) ;
	
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	enable_high_voltage	- enables gantry high voltage
//
//	Inputs:		None
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | GCI_ERROR if failed
//
/***********************************************************************************/

int
enable_high_voltage ()
{
	char cmd[] = "PW1", resp[10] ;
	int arg, status ;

	send_command (GC, cmd, resp) ;
	status = parse_gc_response (resp, &arg) ;
	
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	get_high_voltage	- returns high voltage for specified bucket
//
//	Inputs:		int bucket			- bucket to use
//				int* millivolts		- pointer to store returned value in
//
//	Outputs:	millivolts
//
//	Returns:	GCI_SUCCESS | GCI_ERROR if failed
//
/***********************************************************************************/

int
get_high_voltage (int bucket, int *millivolts)
{
	char cmd[10], resp[10], rc ;
	int status ;

	sprintf (cmd, "%dX4", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, millivolts) ;
	if (rc == 'H') status = GCI_SUCCESS ;
	else status = (GCI_ERROR) ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	disable_singles_polling	- sends command to turn off
//										  gantry singles polling
//
//	Inputs:		None
//
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | GCI_ERROR if failed
//
/***********************************************************************************/

int
disable_singles_polling ()
{
	char cmd[] = "P2", resp[10] ;
	int arg, status ;

	// send the command
	send_command (GC, cmd, resp) ;
	status = parse_gc_response (resp, &arg) ;
	
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	enable_singles_polling	- sends command to turn on
//										  gantry singles polling
//
//	Inputs:		None
//
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | GCI_ERROR if failed
//
/***********************************************************************************/

int
enable_singles_polling ()
{
	char cmd[10], resp[10] ;
	int arg, status ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return (GCI_ERROR) ;
	}

	// send the command
	sprintf (cmd, "P%d", scanner->buckets() + 100) ;
	send_command (GC, cmd, resp) ;
	status = parse_gc_response (resp, &arg) ;
	
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	select_block	- sets block to select mode
//
//	Inputs:		int bucket		- bucket number to use
//				int block		- block number to select
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | SETUP_IN_PROGRESS if busy | GCI_ERROR if failed
//
/***********************************************************************************/

int
select_block (int bucket, int block)
{
	char cmd[10], resp[10], rc = '\0' ;
	int arg = -1, status ;

	// send it and parse it
	sprintf (cmd, "%dS%d", bucket, block) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;

	// determine status
	//cout << "select_block : " << bucket << " " << block << " " << rc << " " << arg << endl ;
	if ((rc == 'N') && (arg == 0)) status = GCI_SUCCESS ;
	else if (rc == 'X') status = SETUP_IN_PROGRESS ;
	else if (rc == 'T') status = COMM_TIMEOUT ;
	else status = (GCI_ERROR) ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	calibrate_cfd_binary	- request bucket to calibrate CFD
//
//	Inputs:		int bucket		- bucket number to use
//				int* cal_error	- (N/A no longer used)
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | COMM_TIMEOUT if communications timeout |
//				SETUP_IN_PROGRESS if busy | GCI_ERROR if failed
//
/***********************************************************************************/

int
calibrate_cfd_binary (int bucket, int *cal_error)
{
	char cmd[10], resp[10], rc ;
	int arg, status ;

	// send it and parse it
	sprintf (cmd, "%dO259", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;

	// determine status
	if ((rc == 'N') && (arg == 0)) status = GCI_SUCCESS ;
	else if (rc == 'T') status = COMM_TIMEOUT ;
	else if (rc == 'X') status = SETUP_IN_PROGRESS ;
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	perform_timealign	- request bucket do time alignment
//
//	Inputs:		int bucket		- bucket number to use
//
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | COMM_TIMEOUT if communications timeout |
//				SETUP_IN_PROGRESS if busy | GCI_ERROR if failed
//
/***********************************************************************************/

int
perform_timealign (int bucket)
{
	char cmd[10], resp[10], rc ;
	int arg, status ;

	// send it and parse it
	sprintf (cmd, "%dT16", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;

	// determine status
	if ((rc == 'N') && (arg == 0)) status = GCI_SUCCESS ;
	else if (rc == 'T') status = COMM_TIMEOUT ;
	else if (rc == 'X') status = SETUP_IN_PROGRESS ;
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	perform_xy_offset_adjust	- request bucket do x-y offset adjust
//
//	Inputs:		int bucket					- bucket number to use
//
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | COMM_TIMEOUT if communications timeout |
//				SETUP_IN_PROGRESS if busy | GCI_ERROR if failed
//
/***********************************************************************************/

int
perform_xy_offset_adjust (int bucket)
{
	char cmd[10], resp[10], rc ;
	int arg, status ;

	// send it and parse it
	sprintf (cmd, "%dP10", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;

	// determine status
	if ((rc == 'N') && (arg == 0)) status = GCI_SUCCESS ;
	else if (rc == 'T') status = COMM_TIMEOUT ;
	else if (rc == 'X') status = SETUP_IN_PROGRESS ;
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	autosetup_block	- request bucket full automated setup
//
//	Inputs:		int bucket		- bucket number to use
//
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | COMM_TIMEOUT if communications timeout |
//				SETUP_IN_PROGRESS if busy | GCI_ERROR if failed
//
/***********************************************************************************/

int
autosetup_block (int bucket)
{
	char cmd[10], resp[10], rc ;
	int arg, status ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return GCI_ERROR ;
	}

	// send it and parse it
	sprintf (cmd, "%dJ1", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;

	// determine status
	if ((rc == 'N') && (arg == 0)) status = GCI_SUCCESS ;
	else if (rc == 'T') status = COMM_TIMEOUT ;
	else if (rc == 'X') status = SETUP_IN_PROGRESS ;
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	poll_cfd	- poll bucket CFD status
//
//	Inputs:		int bucket	- bucket number to use
//				int* code	- pointer to store status code
//
//
//	Outputs:	code
//
//	Returns:	SETUP_GOOD | SETUP_WARNING | SETUP_PROGRESSING | SETUP_ABORTED
//				COMM_TIMEOUT if communications timeout | SETUP_UNKNOWN
//
/***********************************************************************************/

int
poll_cfd (int bucket, int *code)
{
	char cmd[10], resp[10], rc ;
	int status ;

	// send the command and parse
	//cout << "poll_cfd : bucket " << bucket << endl ;
	sprintf (cmd, "%dO261", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, code) ;

	// determine the response
	switch (rc)
	{
		case 'S':
			if (*code == 0) status = SETUP_GOOD ;
			else if ((*code > 0) && (*code <= 9)) status = SETUP_WARNING ;
			else status = SETUP_PROGRESSING ;
			break ;
		case 'X':
			status = SETUP_ABORTED ;
			break ;
		case '\0':
			status = COMM_TIMEOUT ;
			break ;
		default:
			status = SETUP_UNKNOWN ;
	}
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	poll_timealign	- poll bucket timealign status
//
//	Inputs:		int bucket	- bucket number to use
//				int* code	- pointer to store status code
//
//
//	Outputs:	code
//
//	Returns:	SETUP_GOOD | SETUP_WARNING | SETUP_PROGRESSING | SETUP_ABORTED
//				COMM_TIMEOUT if communications timeout | SETUP_UNKNOWN
//
/***********************************************************************************/

int
poll_timealign (int bucket, int *code)
{
	char cmd[10], resp[10], rc ;
	int status ;

	// send the command and parse
	sprintf (cmd, "%dT18", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, code) ;

	// determine the response
	switch (rc)
	{
		case 'S':
			if (*code == 0) status = SETUP_GOOD ;
			else if ((*code > 0) && (*code <= 9)) status = SETUP_WARNING ;
			else status = SETUP_PROGRESSING ;
			break ;
		case 'X':
			status = SETUP_ABORTED ;
			break ;
		case '\0':
			status = COMM_TIMEOUT ;
			break ;
		default:
			status = SETUP_UNKNOWN ;
	}
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	poll_xy		- poll bucket x-y setup status
//
//	Inputs:		int bucket	- bucket number to use
//				int* code	- pointer to store status code
//
//
//	Outputs:	code
//
//	Returns:	SETUP_GOOD | SETUP_WARNING | SETUP_PROGRESSING | SETUP_ABORTED
//				COMM_TIMEOUT if communications timeout | SETUP_UNKNOWN
//
/***********************************************************************************/

int
poll_xy (int bucket, int *code)
{
	char cmd[10], resp[10], rc ;
	int status ;

	// send the command and parse
	sprintf (cmd, "%dP12", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, code) ;

	// determine the response
	switch (rc)
	{
		case 'S':
			if (*code == 0) status = SETUP_GOOD ;
			else if ((*code > 0) && (*code <= 9)) status = SETUP_WARNING ;
			else status = SETUP_PROGRESSING ;
			break ;
		case 'X':
			status = SETUP_ABORTED;
			break ;
		case '\0':
			status = COMM_TIMEOUT ;
			break ;
		default:
			status = SETUP_UNKNOWN ;
	}
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	poll_autosetup	- poll bucket x-y setup status
//
//	Inputs:		int bucket		- bucket number to use
//				int* code		- pointer to store status code
//
//
//	Outputs:	code
//
//	Returns:	SETUP_GOOD | SETUP_WARNING | SETUP_PROGRESSING | SETUP_ABORTED
//				COMM_TIMEOUT if communications timeout | SETUP_UNKNOWN
//
/***********************************************************************************/

int
poll_autosetup (int bucket, int *code)
{
	char cmd[10], resp[10], rc ;
	int status ;

	// send the command and parse
	sprintf (cmd, "%dJ0", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, code) ;

	// determine the response
	switch (rc)
	{
		case 'S':
			if (*code == 0) status = SETUP_GOOD ;
			else if ((*code > 0) && (*code <= 9)) status = SETUP_WARNING ;
			else status = SETUP_PROGRESSING ;
			break ;
		case 'X':
			status = SETUP_ABORTED;
			break ;
		case '\0':
			status = COMM_TIMEOUT ;
			break ;
		default:
			status = SETUP_UNKNOWN ;
	}
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	abort_hist	- abort histogram acquisition
//
//	Inputs:		int bucket	- bucket number
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | SETUP_IN_PROGRESS | SETUP_ABORTED |
//				COMM_TIMEOUT if communications timeout
//
/***********************************************************************************/

int
abort_hist (int bucket)
{
	char cmd[10] ;
	char resp[10], rc ;
	int arg, status ;

	// send the command and parse
	sprintf (cmd, "%dH6", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;

	// determine the status
	if ((rc == 'N') && (arg == 0)) status = GCI_SUCCESS ;
	else if ((rc == 'X') && (arg == 0)) status = SETUP_IN_PROGRESS ;
	else if ((rc == 'T') && (arg == 0)) status = COMM_TIMEOUT ;
	else status = (GCI_ERROR) ;
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	abort_cfd	- abort CFD
//
//	Inputs:		int bucket	- bucket number
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | SETUP_IN_PROGRESS | SETUP_ABORTED |
//				COMM_TIMEOUT if communications timeout
//
/***********************************************************************************/

int
abort_cfd (int bucket)
{
	return (abort_hist (bucket)) ;
}

/***********************************************************************************/
//
//
//	Function:	abort_timealign	- abort timealign
//
//	Inputs:		int bucket		- bucket number
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | SETUP_IN_PROGRESS | SETUP_ABORTED |
//				COMM_TIMEOUT if communications timeout
//
/***********************************************************************************/

int
abort_timealign (int bucket)
{
	return (abort_hist (bucket)) ;
}

//
// abort_xy - abort block X-Y setup
//
int
abort_xy (int bucket)
{
	return (abort_hist (bucket)) ;
}

/***********************************************************************************/
//
//
//	Function:	report_hist_status	- report histogram status
//
//	Inputs:		int bucket			- bucket number
//				int* seconds_left	- pointer to return time left
//
//	Outputs:	seconds_left
//
//	Returns:	GCI_SUCCESS | SETUP_IN_PROGRESS | SETUP_UNKNOWN |
//				COMM_TIMEOUT if communications timeout
//
/***********************************************************************************/

int
report_hist_status (int bucket, int *seconds_left)
{
	char cmd[10], resp[10], rc ;
	int status ;

	// send the command and parse
	sprintf (cmd, "%dH0", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, seconds_left) ;

	// determine the response
	switch (rc)
	{
		case 'X':
			if (*seconds_left > 0) status = SETUP_IN_PROGRESS ;
			else status = GCI_SUCCESS ;
			break ;
		case '\0':
			status = COMM_TIMEOUT ;
			break ;
		default:
			status = SETUP_UNKNOWN ;
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	select_hist_position_time	- select histogram mode
//
//	Inputs:		int bucket			- bucket number
//
//	Outputs:	seconds_left
//
//	Returns:	GCI_SUCCESS | SETUP_IN_PROGRESS | INVALID_BLOCK |
//				COMM_TIMEOUT if communications timeout
//
/***********************************************************************************/

int
select_hist_position_time (int bucket)
{
	char cmd[10], resp[10], rc ;
	int arg, status ;

	// send the command and parse
	sprintf (cmd, "%dH1", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;

	// determine the response
	switch (rc)
	{
		case 'N':
			if (arg == 0) status = GCI_SUCCESS ;
			break ;
		case 'X':
			if (arg >= 0) status = SETUP_IN_PROGRESS ;
			break ;
		case '\0':
			status = COMM_TIMEOUT ;
			break ;
		default:
			status = INVALID_BLOCK ;
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	get_tube_gains	- get tube gain settings
//
//	Inputs:		int bucket		- bucket number
//				int* gain0		- pointer to store gain for tube 0
//				int* gain1		- pointer to store gain for tube 1
//				int* gain3		- pointer to store gain for tube 2
//				int* gain3		- pointer to store gain for tube 2
//
//	Outputs:	gain0, gain1, gain2, gain3
//
//	Returns:	GCI_SUCCESS | INVALID_BLOCK |
//				COMM_TIMEOUT if communications timeout
//
/***********************************************************************************/

int
get_tube_gains (int bucket, int *gain0, int *gain1, int *gain2, int *gain3)
{
	char cmd[10], resp[64], rc ;
	int arg[4], status ;

	// determine the response
	sprintf (cmd, "%dP5", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg[0]) ;

	// determine the response
	switch (rc)
	{
		case 'N':
			if (arg[0] == 0) status = GCI_SUCCESS ;
			break ;
		case 'G':
			if (arg[0] >= 0) status = GCI_SUCCESS ;
			{
				status = GCI_SUCCESS ;
				*gain0 = arg[0] ;
				*gain1 = arg[1] ;
				*gain2 = arg[2] ;
				*gain3 = arg[3] ;
			}
			break ;
		case 'T':
		case '\0':
			status = COMM_TIMEOUT ;
			break ;
		default:
			status = INVALID_BLOCK ;
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	ResetBuckets	- do a bucket reset
//
//	Inputs:		int startbkt	- starting bucket number
//				int endbkt		- ending bucket number
//				int resetmode	- reset type selection 0=reboot
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if a specified bucket fails to behave properly
//
/***********************************************************************************/

BOOL
ResetBuckets (int startbkt, int endbkt, int resetmode)
{
	char cmd[10], resp[10] ;
	int bkt ;

	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		sprintf (cmd, "%dZ%d", bkt, resetmode) ;
		send_command (BKT, cmd, resp) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	reset_bucket	- do a 'Z0' bucket reset
//
//	Inputs:		int startbkt	- bucket number
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if a bucket fails to behave properly
//
/***********************************************************************************/

int
reset_bucket (int bucket)
{
	char cmd[10], resp[10] ;

	sprintf (cmd, "%dZ0", bucket) ;
	send_command (BKT, cmd, resp) ;

	return (0) ;
}

/***********************************************************************************/
//
//
//	Function:	reset_bucket	- do a 'Z1' bucket reset (zap)
//
//	Inputs:		int startbkt	- bucket number
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if a bucket fails to behave properly
//
/***********************************************************************************/

int
reset_bucket_all_defaults (int bucket)
{
	char cmd[10], resp[10] ;

	sprintf (cmd, "%dZ1", bucket) ;
	send_command (BKT, cmd, resp) ;

	return (0) ;
}

/***********************************************************************************/
//
//
//	Function:	reset_bucket	- do a 'Z2' bucket reset (zap)
//
//	Inputs:		int startbkt	- bucket number
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if a bucket fails to behave properly
//
/***********************************************************************************/

int
reset_bucket_block_defaults (int bucket)
{
	char cmd[10], resp[10], rc ;
	int arg, status ;

	sprintf (cmd, "%dZ2", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc == 'T') status = GCI_SUCCESS ;
	else status = GCI_ERROR ;

	return (0) ;
}

/***********************************************************************************/
//
//
//	Function:	report_bucket_status	- report status of specified bucket
//
//	Inputs:		int startbkt	- bucket number
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if a bucket fails to behave properly
//
/***********************************************************************************/

int
report_bucket_status (int bucket, int *code)
{
	char cmd[10], resp[10], rc ;
	int status ;

	sprintf (cmd, "%dX0", bucket) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, code) ;

	if ((rc == 'C') || (rc == 'D')) status = CRC_IN_PROGRESS ;
	else if (rc == 'S') status = STATUS_IN_PROGRESS ;

	return (0) ;
}

/***********************************************************************************/
//
//
//	Function:	send_command	- this routine establishes a connection to
//								  the GCI server, sends a command, and gets
//								  response 
//
//	Inputs:		int gbus		- target gantry bus
//				char *cmd		- pointer to command string to be send
//				char *resp		- pointer to area to return repsonse in
//
//	Outputs:	resp
//
//	Returns:	0
//
/***********************************************************************************/

LOCAL int
send_command (int gbus, char* cmd, char* resp)
{
	char gccmd[64] ;
	DWORD nbytes, err ;
	BOOL pstatus ;

	// initialize the pipe name one time
	if (!GciClntPipe[0]) sprintf (GciClntPipe, "\\\\%s\\pipe\\GCIPipe", ACS_IP_ADDR) ;

	// clear command string
	ZeroMemory (gccmd, 64) ;
	
	// build the command depending on the gantry serial bus it goes to
	switch (gbus)
	{
		case GC:
		case PT:
			sprintf (gccmd, "%s", cmd) ;
			break ;
		case BKT:
		case IPCP:
			sprintf (gccmd, "GA%s", cmd) ;
			break ;
	}

	// send the command and get the repsonse
	while (TRUE)
	{
		pstatus = CallNamedPipe (GciClntPipe, gccmd, strlen (gccmd), resp, 128, &nbytes, NMPWAIT_WAIT_FOREVER) ;
		if (pstatus)
		{
			//cout << "DLL send : " << cmd << " Response " << resp << endl ;
			resp[nbytes] = '\0' ;
			break ;
		}
		else if ((err = GetLastError ()) != ERROR_PIPE_BUSY)
		{
			cout << "DLL send : Error " << GetLastError () << endl ;
			resp[0] = '\0' ;
			break ;
		}
	}

	if (GCIbug)
	{
		gccmd [strlen (gccmd) - 1] = '\0' ;
		cout << "DLL send_command: Sent " << gccmd << "  Received " << resp << endl ;
	}

	return (0) ;
}

/***********************************************************************************/
//
//
//	Function:	parse_gc_response	- parses a response from the gantry controller
//
//	Inputs:		char* resp		- response to parse
//				int* arg		- pointer to variable to hold parsed argument
//
//	Outputs:	arg
//
//	Returns:	GCI_SUCCESS | GCI_ERROR
//
/***********************************************************************************/

LOCAL int
parse_gc_response (char* resp, int* arg)
{
	int hdrcode, status ;

	hdrcode = get_hdr_code (resp) ;
	switch (hdrcode)
	{
		// parse GN response from gantry controller
		case GN_HDR:
			sscanf (&resp[2], "%d", arg) ;
			if (*arg >= 0) status = GCI_SUCCESS ;
			else status = GCI_ERROR ;
			break ;
		default:
			status = GCI_ERROR ;
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	parse_fesb_response	- parses a response from the front end serial bus
//
//	Inputs:		char* resp		- response to parse
//				char* rc		- pointer to char to return ascii return character
//				int* arg		- pointer to variable to hold returned argument
//
//	Outputs:	rc, arg
//
//	Returns:	0 for success | -1 if unrecognized header character
//
/***********************************************************************************/

LOCAL int
parse_fesb_response (char* resp, char* rc, int* arg)
{
	int status = 0, hdrcode ;

	//cout << "parse_fesb_response : " << resp << endl ;
	hdrcode = get_hdr_code (resp) ;
	switch (hdrcode)
	{
		// parse GN response from gantry controller
		case GT_HDR:
			*rc = resp[3] ;
			//cout << "GT: " << *rc << endl ;
			// this should be a good response
			switch (*rc)
			{
				case 'N':
				case 'E':
				case 'X':
				case 'D':
				case 'C':
				case 'V':
					sscanf (&resp[5], "%d", &arg[0]) ;
					break ;
				case 'S':
				case 'G':
					sscanf (&resp[5], "%d,%d,%d,%d", &arg[0], &arg[1], &arg[2], &arg[3]) ;
					break ;
				case '\0':
					*rc = 'T' ;
					break ;
				default:
					sscanf (&resp[5], "%d", arg) ;
			}
			// break from GT header
			break ;

		// timeout header
		case TO_HDR:
			*rc = 'T' ;
			break ;

		// main switch hrdcode
		default:
			status = -1 ;
	}

	//cout << "parse_fesb_response : " << *rc << " " << *arg << endl ;
	return (status) ;
}
	
/***********************************************************************************/
//
//
//	Function:	get_hdr_code	- locates index for response header code
//
//	Inputs:		char* resp		- response to parse
//
//	Outputs:	None
//
//	Returns:	index number
//
/***********************************************************************************/

LOCAL int
get_hdr_code (char* resp)
{
	int i ;

	for (i=0;gci_headers[i][0] != '\0';i++)
	{
		if (!strncmp (gci_headers[i], resp, 2)) break;
	}

	return (i) ;
}

/***********************************************************************************/
//
//
//	Function:	GCIAsyncRead	- monitors messages sent asynchronously from gantry
//								  runs as a spearate thread
//
//	Inputs:		None
//
//	Outputs:	None
//
//	Returns:	void
//
/***********************************************************************************/

LOCAL void
GCIAsyncRead ()
{
	HANDLE asyncPipe ;
	char bufr[512] ;
	DWORD nbytes ;
	BOOL fSuccess ;
	int i ;

	// initialize the pipe name... once only
	if (!GciClntAsyncPipe[0]) sprintf (GciClntAsyncPipe, "\\\\%s\\pipe\\GCIAsyncPipe", ACS_IP_ADDR) ;

	if ((asyncPipe = CreateFile (GciClntAsyncPipe,
						    GENERIC_READ,
							0,
							NULL,
							OPEN_EXISTING,
							0,
							NULL)) == INVALID_HANDLE_VALUE)
	{
		cout << "GCIAsyncRead: Could not open Async Pipe... " << endl ;
	}

	// monitor and handle the messages
	while (TRUE)
	{
		// post a read on the pipe
		nbytes = 0 ;
		fSuccess = ReadFile (GciClntAsyncPipe, bufr, 512, &nbytes, NULL) ;
		if (fSuccess && nbytes)
		{
			// find out what it is
			for (i=0;gci_headers[i][0] != '\0';i++)
			{
				if (!strncmp (gci_headers[i], bufr, 2)) break;
			}

			switch (i)
			{
				case SN_HDR:
					break ;
				case IP_HDR:
					break ;
				case IR_HDR:
					break ;
				case ER_HDR:
					break;
				case GM_HDR:
					break ;
				case ES_HDR:
					break ;
				default:
					cout << "GCIAsyncRead: Unknown message received... " << bufr << endl ;
			}
		}
		else
		{
			cout << "GCIAsyncRead: Error reading pipe... " << GetLastError () << endl ;
		}
	}
}

/***********************************************************************************/
//
//
//	Function:	AcquireHist		- Exported routine for GUI to start a block histogram
//
//	Inputs:		int htype		- histogram type
//				int bkt			- bucket number
//				int blk			- block number
//				int acqtime		- acquisition time
//				char* filename	- filename to store histogrammed data to
//
//	Note:		Stores arguments in static HISTARGS structure for worker thread
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if worker thread cannot be spawned
//
/***********************************************************************************/

BOOL
AcquireHist (int htype, int bkt, int blk, int acqtime, char* filename)
{
	DWORD threadID ;

	// Load the data in the LOCAL structure
	HistArgs.htype = htype ;
	HistArgs.bkt = bkt ;
	HistArgs.blk = blk ;
	HistArgs.acqtime = acqtime ;
	strcpy (HistArgs.filename, filename) ;

	// Create the thread that will do the work
	if (!(HistThread = CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
							   0,
							   (LPTHREAD_START_ROUTINE)acquire_histogram_data,
							   (LPVOID)NULL,
							   0,
							   &threadID))) return (FALSE) ;

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	acquire_histogram_data	- worker thread to acquire a block histogram
//
//	Inputs:		None
//
//	Note:		Gets arguments from static HISTARGS structure
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
acquire_histogram_data ()
{
	HANDLE hfile ;
	BOOL status ;
	DWORD err ;
	int time_left, ntimes ;
	tGantryResult *acqComplete, *uploadInProgress ;
	int bkt, blk, htype, acqtime, new_percent ;

	// pick out the arguments
	bkt = HistArgs.bkt ;
	blk = HistArgs.blk ;
	htype = HistArgs.htype ;
	acqtime = HistArgs.acqtime ;

	// try to acquire the histogram
	if (!(status = acquire_histogram (bkt, blk, htype, acqtime))) return (status) ;

	// poll to see if histogram is done
	// calculate the progress bar updates
	ntimes = 0 ;
	while (ntimes++ < (acqtime + 10))
	{
		if ((time_left = poll_histogram(bkt)) == 0)
		{
			status = TRUE ;
			post_status_update (100, WM_STATUSACQUISITION, "") ;
			break ;
		}
		else if (time_left > 0)
		{
			new_percent = (int)(100.0 * (((float)acqtime - (float)time_left) / (float)acqtime)) ;
			post_status_update (new_percent, WM_STATUSACQUISITION, "") ;
		}
		else if (time_left < 0)
		{
			status = FALSE ;
			break ;
		}
		Sleep (2000) ;
	}

	if (uploadInProgress = NewGantryResult ())
	{
		strcpy(uploadInProgress->StatusMsg, "Uploading Data... Please Wait...") ;
		PostWindowMessage(hWnd, WM_ACQUISITIONUPD, (WPARAM)uploadInProgress) ;
	}

	// check the histogram status
	if ((!status) || (ntimes > (acqtime + 10))) return (FALSE) ;

	// now try to create the file
	if ((hfile = CreateFile (HistArgs.filename,
							GENERIC_WRITE,
							FILE_SHARE_READ,
							NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL)) == INVALID_HANDLE_VALUE)
	{
		err = GetLastError () ;
		if (err != ERROR_ALREADY_EXISTS)
		{
			cout << "Error opening file..." << GetLastError () << endl ;
			return (FALSE) ;
		}
	}

	status = process_block_data (hfile, bkt, blk, htype) ;

	CloseHandle (hfile) ;

	if (acqComplete = NewGantryResult ())
	{
		strcpy(acqComplete->StatusMsg, "Acquisition and Upload Complete") ;
		PostWindowMessage(hWnd, WM_ACQUISITIONCOMPLETE, (WPARAM)acqComplete) ;
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	acquire_histogram	- support routine for main worker thread routine
//
//	Inputs:		int bkt				- bucket number
//				int blk				- block number
//				int type			- histogram type
//				int time			- time to acquire
//
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
acquire_histogram (int bkt, int blk, int type, int time)
{
	char cmd[10], resp[10], rc ;
	int hcode, arg ;

	// determine the type acquisition
	switch (type)
	{
		//case POSITION_SCATTER:
		case POS_SCATTER_PROFILE:
			hcode = 1 ;
			break ;
		//case POSITION_PROFILE:
		case POS_PROFILE:
			hcode = 2 ;
			break ;
		//case TUBE_ENERGY:
		case TUBE_ENERGY_PROFILE:
			hcode = 3 ;
			break ;
		//case CRYSTAL_ENERGY:
		case CRYSTAL_ENERGY_PROFILE:
			hcode = 4 ;
			break ;
		//case TIME_SPECTRUM:
		case TIME_HIST_PROFILE:
			hcode = 5 ;
			break ;
		default:
			return (FALSE) ;
	}

	// select the bucket and block
	sprintf (cmd, "%dS%d", bkt, blk) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'N') return (FALSE) ;

	// select histogram type
	sprintf (cmd, "%dH%d", bkt, hcode) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'N') return (FALSE) ;

	// start the histogram
	Sleep (1000) ;
	sprintf (cmd, "%dH%d", bkt, time) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'N') return (FALSE) ;

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	poll_histogram	- polls for histgram status
//
//	Inputs:		int bkt			- bucket number
//
//
//	Outputs:	None
//
//	Returns:	0 | -1 on response failed
//
/***********************************************************************************/

LOCAL int
poll_histogram (int bkt)
{
	char cmd[10], resp[10], rc ;
	int arg ;

	// select the bucket
	sprintf (cmd, "%dH0", bkt) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if ((rc == 'X') && (arg >= 0)) return (arg) ;
	else if (rc == 'N') return (0) ;
	else return (-1) ;
}

/***********************************************************************************/
//
//
//	Function:	process_block_data	- dispatch routine for processing histogram data
//
//	Inputs:		HANDLE file			- handle to open file for storage of data
//				int bkt				- bucket number
//				int blk				- block number
//				int htype			- histogram type
//
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if type is not recognized
//
/***********************************************************************************/

LOCAL BOOL
process_block_data (HANDLE hfile, int bkt, int blk, int htype)
{
	// determine upload info
	switch (htype)
	{
		case POS_SCATTER_PROFILE:
			if (!(get_position_scatter_profile (hfile, bkt))) return (FALSE) ;
			break ;
		case POS_PROFILE:
			if (!(get_position_profile_data (hfile, bkt, blk))) return (FALSE) ;
			break ;
		case TUBE_ENERGY_PROFILE:
			if (!(get_energy_profile (hfile, bkt, TENGSIZE))) return (FALSE) ;
			break ;
		case CRYSTAL_ENERGY_PROFILE:
			if (!(get_crystal_energy_profile (hfile, bkt, blk, CENGSIZE))) return (FALSE) ;
			break ;
		case TIME_HIST_PROFILE:
			if (!(get_time_histogram_profile (hfile, bkt))) return (FALSE) ;
			break ;
		default:
			return (FALSE) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	get_position_scatter_profile	- retrieves the scatter profile
//
//	Inputs:		HANDLE file			- handle to open file for storage of data
//				int bkt				- bucket number
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
get_position_scatter_profile (HANDLE hfile, int bkt)
{
	char *data ;
	int i ;
	int *longdata ;

	// allocate the memory for the upload
	if (!(data = (char*)malloc (POSSCATTERSIZE))) return (FALSE) ;

	// get the data
	if (!(upload_block_data (bkt, data, HISTOADDR, POSSCATTERSIZE)))
	{
		free (data) ;
		return (FALSE) ;
	}

	// setup to access as 32 bit data
	longdata = (int*)data ;

	// store crystal info
	for (i=63;i>=0;i--)
	{
		if (!(write_data (hfile, (int)longdata[i])))
		{
			free (data) ;
			return (FALSE) ;
		}
	}

	// store scatter info
	for (i=127;i>=64;i--)
	{
		if (!(write_data (hfile, (int)longdata[i])))
		{
			free (data) ;
			return (FALSE) ;
		}
	}

	free (data) ;
	
	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	get_position_profile_data	- retrieves position profile data
//
//	Inputs:		HANDLE file			- handle to open file for storage of data
//				int bkt				- bucket number
//				int blk				- block number
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
get_position_profile_data (HANDLE file, int bkt, int blk)
{
	BOOL status = TRUE ;
	char *data, *row, *col, *peakpos ;

	data = row = col = peakpos = NULL ;

	// need 8 k bytes for upload of position profile data
	if (!(data = (char*)malloc (POSPROSIZE))) return (FALSE) ;

	// get the position profile data
	if (!(upload_block_data (bkt, data, HISTOADDR, POSPROSIZE)))
	{
		free_position_profile_memory (data, row, col, peakpos) ;
		return (FALSE) ;
	}

	// store it
	if (!(save_position_profile (file, data, POSPROSIZE)))
	{
		free_position_profile_memory (data, row, col, peakpos) ;
		return (FALSE) ;
	}

	// get the row boundary data - 64 bytes each
	if (!(row = (char*)malloc (XYSIZE))) return (FALSE) ;
	if (!(col = (char*)malloc (XYSIZE)))
	{
		free (row) ;
		return (FALSE) ;
	}
	if (!(peakpos = (char*)malloc (XYSIZE*2)))
	{
		free (row) ;
		free (col) ;
		return (FALSE) ;
	}
	if (!(upload_boundary_data (bkt, blk, row, col, peakpos))) status = FALSE ;

	// now store the infomation to the file
	if (!(save_boundary_peak_data (file, row, col, peakpos))) status = FALSE ;

	free_position_profile_memory (data, row, col, peakpos) ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	get_energy_profile	- retrieves energy profile data
//
//	Inputs:		HANDLE file			- handle to open file for storage of data
//				int bkt				- bucket number
//				int size			- profile size
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
get_energy_profile (HANDLE hfile, int bkt, int size)
{
	char *data ;
	int i, nwords ;
	short *worddata ;

	// allocate the memory for the upload
	if (!(data = (char*)malloc (size))) return (FALSE) ;

	// get the data
	if (!(upload_block_data (bkt, data, HISTOADDR, size)))
	{
		free (data) ;
		return (FALSE) ;
	}

	// setup to access as 16 bit data
	nwords = size / 2 ;
	worddata = (short*)data ;

	// store the data
	for (i=0;i<nwords;i++)
	{
		if (!(write_data (hfile, (int)worddata[i])))
		{
			free (data) ;
			return (FALSE) ;
		}
	}

	free (data) ;
  
	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	get_crystal_energy_profile	- retrieves crystal energy profile data
//
//	Inputs:		HANDLE file			- handle to open file for storage of data
//				int bkt				- bucket number
//				int blk				- block number
//				int size			- profile size
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
get_crystal_energy_profile (HANDLE hfile, int bkt, int blk, int size)
{
	char *data ;
	ULONG address ;
	int i ;
	short *worddata ;

	// first get the energy profile
	if (!(get_energy_profile (hfile, bkt, size))) return (FALSE) ;

	// now the peak energy data
	address = (ULONG)(EEPROMBASE+PKENERGY+(PKENGSIZE*blk)) ;
	if (!(data = (char*) malloc (PKENGSIZE))) return (FALSE) ;
	if (!(upload_block_data (bkt, data, address, PKENGSIZE)))
	{
		free (data) ;
		return (FALSE) ;
	}

	// append the 16 bit peak energy data to the file
	worddata = (short*)data ;
	for (i=0;i<PKENGSIZE/2;i++)
	{
		if (!(write_data (hfile, worddata[i])))
		{
			free (data) ;
			return (FALSE) ;
		}
	}

	free (data) ;

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	get_time_histogram_profile	- retrieves time histogram data
//
//	Inputs:		HANDLE file			- handle to open file for storage of data
//				int bkt				- bucket number
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
get_time_histogram_profile (HANDLE hfile, int bkt)
{
	char *data ;
	int i, *longdata ;

	// allocate the memory for the upload
	if (!(data = (char*)malloc (TIMHISTSIZE))) return (FALSE) ;

	// get the data
	if (!(upload_block_data (bkt, data, HISTOADDR, TIMHISTSIZE)))
	{
		free (data) ;
		return (FALSE) ;
	}

	// setup to access as 32 bit data
	longdata = (int*)data ;

	// store the data
	for (i=0;i<128;i++)
	{
		if (!(write_data (hfile, (int)longdata[i])))
		{
			free (data) ;
			return (FALSE) ;
		}
	}

	free (data) ;
  
	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	free_position_profile_memory	- function to free allocated memory
//												  used for position profile data
//
//	Inputs:		char* data		- pointer to allocated profile data array
//				char* row		- pointer to allocated row boundary information
//				char* col		- pointer to allocated column boundary information
//				char* peakpos	- pointer to allocated peak position information
//
//	Outputs:	None
//
//	Returns:	VOID
//
/***********************************************************************************/

LOCAL VOID
free_position_profile_memory (char* data, char* row, char* col, char* peakpos)
{
	// free memory if allocated
	if (data)	free (data) ;
	if (row)	free (row) ;
	if (col)	free (col) ;
	if (peakpos)free (peakpos) ;
}

/***********************************************************************************/
//
//
//	Function:	upload_block_data	- uploads block histogram data
//
//	Inputs:		int bkt				- bucket number
//				char* buffer		- where to store the data
//				ULONG address		- bucket memory address to retrieve data from
//				int nbytes			- size of data to upload
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
upload_block_data (int bkt, char* buffer, ULONG address, int nbytes)
{
	char cmd[10], resp[64], rc ;
	int arg ;
	BOOL status = TRUE ;

	// set the upload address to the bucket histgram ram
	sprintf (cmd, "%dL%d", bkt, address) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'N') return (FALSE) ;

	// set the number of bytes to upload
	sprintf (cmd, "%dQ%d", bkt, nbytes) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'N') return (FALSE) ;

	// upload record and store
	while (TRUE)
	{
		// get a record
		sprintf (cmd, "%dQ0", bkt) ;
		send_command (BKT, cmd, resp) ;
		if (resp[3] != ':') status = FALSE ;
		
		// is it the final record
		if (strncmp (&resp[3], ":00000001FF", strlen (":00000001FF")) == 0) break ;

		// now parse it and store it
		status = parse_intellec_record (&resp[3], buffer, address) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	upload_boundary_data	- uploads block boundary data
//
//	Inputs:		int bkt				- bucket number
//				int blk				- block number
//				char* row			- where to store row boundary data
//				char* col			- where to store column boundary data
//				char* peakpos		- where to store peak position data
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
upload_boundary_data (int bkt, int blk, char* row, char* col, char* peakpos)
{
	BOOL status = TRUE ;
	ULONG address ;

	// start with row boundary data
	address = (ULONG)(EEPROMBASE+ROWBOUNDARY+(BOUNDSIZE*blk)) ;
	if (!(upload_block_data (bkt, row, address, BOUNDSIZE))) return (FALSE) ;

	// now get the column boundary data
	address = (ULONG)(EEPROMBASE+COLBOUNDARY+(BOUNDSIZE*blk)) ;
	if (!(upload_block_data (bkt, col, address, BOUNDSIZE))) return (FALSE) ;

	// and finally the peak position data
	address = (ULONG)(EEPROMBASE+PKPOSITION+(BOUNDSIZE*2*blk)) ;
	if (!(upload_block_data (bkt, peakpos, address, BOUNDSIZE*2))) return (FALSE) ;

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	parse_intellec_record	- parse a record in Intellec 8 format
//										  and store as binary data
//
//	Inputs:		char* record		- pointer to Intellec 8 record
//				char* data			- where to store translated data
//				ULONG address		- memory address that record should contain
//
//	Outputs:	data
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
parse_intellec_record (char* record, char* data, ULONG address)
{
	BOOL status = TRUE ;
	char str[16] ;
	int rindex, dindex, i, bcnt, hexdata, start ;

	// first determine number of bytes in the record
	str[2] = '\0' ;
	strncpy (str, &record[1], 2) ;
	if (!ascii_to_hex (str, &bcnt, 1)) return (FALSE) ;

	// now pick out the address
	str[4] = '\0' ;
	strncpy (str, &record[3], 4) ;
	if (!ascii_to_hex (str, &start, 2)) return (FALSE) ;

	// check address
	if ((ULONG)start < address) return (FALSE) ;
 
	// now pick out the data
	rindex = 9 ;
	dindex = 0 ;

	// parse rest of record and store to memory
	str[2] = '\0' ;
	for (i=0;i<bcnt;i++)
	{
		strncpy (str, &record[rindex], 2) ;
		if (!ascii_to_hex (str, &hexdata, 1)) status = FALSE ;
		data[(start-address)+dindex] = (char)hexdata ;
		rindex += 2 ;
		dindex++ ;
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	ascii_to_hex	- support routine for Intellec parser that
//								  converts ascii characters into data
//
//	Inputs:		char* astr		- ascii string representing data
//				int* hex		- where to store hex data
//				int size		- size to convert to (byte or word)
//
//	Outputs:	hex
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
ascii_to_hex (char* astr, int* hex, int size)
{
	char achar ;
	int i, nbytes ;
	BOOL status = TRUE ;

	CharLower (astr) ;
	*hex = 0 ;
	nbytes = size * 2 ;
	for (i=0;i<nbytes;i++)
	{
		if (!(((astr[i] >= '0') && (astr[i] <= '9')) || ((astr[i] >= 'a') && (astr[i] <= 'f')))) status = FALSE ;
		achar = astr[i] ;
		if ((achar >= 'a') && (achar <= 'f')) achar += '9' ;
		achar &= 0x0f ;
		*hex = (*hex << 4) | achar ;
	}

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	save_position_profile	- write ascii data to file from
//										 16 bit position profile word binary data
//
//	Inputs:		HANDLE file		- handle of file to store data in
//				char* data		- pointer to data
//				int nbytes		- number of bytes to store
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
save_position_profile (HANDLE file, char* data, int nbytes)
{
	int i, nwords ;
	unsigned short* dataword ;
	BOOL status = TRUE ;

	dataword = (unsigned short*)data ;
	nwords = (nbytes / 2) - 1 ;

    for (i=nwords; i>=0; i--) if (!(write_data (file, (int)dataword[i]))) status = FALSE ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	save_boundary_peak_data	- write boundaries and peak position info
//
//	Inputs:		HANDLE file		- handle of file to store data in
//				char* row		- pointer to row boundary data
//				char* col		- pointer to coloumn boundary data
//				char* peakpos	- pointer to peak position data
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
save_boundary_peak_data (HANDLE file, char* row, char* col, char* peakpos)
{
	int i, j ;

	// first write the row dimemsions
	if (!(write_data (file, XYDIMENSION))) return (FALSE) ;
	if (!(write_data (file, XYDIMENSION))) return (FALSE) ;

	// now write row boundaries
	for (i=0;i<8;i++)
	{
		for (j=7;j>=0;j--) if (!(write_data (file, (int)(63 - row[i*8+j])))) return (FALSE) ;
	}

	// write column dimensions
	if (!(write_data (file, XYDIMENSION))) return (FALSE) ;
	if (!(write_data (file, XYDIMENSION))) return (FALSE) ;

	// write column boundaries
	for (i=0;i<8;i++)
	{
		for (j=6;j>=0;j--)
		{
			if (!(write_data (file, (int)col[i*8+j]))) return (FALSE) ;
		}
		// write upper limit
		if (!(write_data (file, (int)63))) return (FALSE) ;
	}

	// write peak position dimensions
	if (!(write_data (file, XYDIMENSION))) return (FALSE) ;
	if (!(write_data (file, XYDIMENSION))) return (FALSE) ;

	// write the peak position data - x's first then y's
	for (i=0;i<128;i+=2) if (!(write_data (file, (int)(63 - peakpos[i])))) return (FALSE) ;
	for (i=1;i<128;i+=2) if (!(write_data (file, (int)(peakpos[i])))) return (FALSE) ;

	// and return
	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	write_data	- write ascii data to file from 16 bit word binary data
//
//	Inputs:		HANDLE file	- handle of file to store data in
//				char* row	- data to write
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
write_data (HANDLE hfile, int data)
{
	char datastr[32] ;
	DWORD nbytes ;
	BOOL status = TRUE ;

	sprintf (datastr, "%d\n", data) ;
	if (!(WriteFile (hfile, datastr, strlen (datastr), &nbytes, NULL))) status = FALSE ;
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	GCIUtilPassThru	- Does pass through command for gantry utilities
//
//	Inputs:		char* cmd	- pointer to command string
//				char* resp	- where to return the response
//
//	Outputs:	resp
//
//	Returns:	0
//
/***********************************************************************************/

int
GCIUtilPassThru (char *cmd, char *resp)
{
	char tbuf[64] ;
	int bus ;

	if ((cmd[0] >= '0') && (cmd[0] <= '2')) bus = BKT ;
	else if (cmd[0] == '6') bus = IPCP ;
	else bus = GC ;
	
	
	send_command (bus, cmd, tbuf) ;
	if (!strncmp ("GT", tbuf, 2)) strcpy (resp, &tbuf[3]) ;
	else strcpy (resp, tbuf) ;

	return (0) ;
}

/***********************************************************************************/
//
//
//	Function:	ProcessSettingsOp	- exported call for gantry setting tool
//									  spins thread to do the work
//
//	Inputs:		int Op				- operation to perform
//				int startbkt		- starting bucket number
//				int endbkt			- ending bucket number
//				int startblk		- starting block number
//				int endblk			- ending block
//				int arg1			- argument 1, Operation dependent
//				int arg2			- argument 2, Operation dependent
//
//	Note:		Loads static GSARGS structure used by spawned thread
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if thead fails to start
//
/***********************************************************************************/

BOOL
ProcessSettingsOp (int Op, int startbkt, int endbkt, int startblk, int endblk, int arg1, int arg2)
{
	DWORD threadID ;

	// Load the data in the LOCAL structure
	GSArgs.Op = Op ;
	GSArgs.startbkt = startbkt ;
	GSArgs.endbkt = endbkt ;
	GSArgs.startblk = startblk ;
	GSArgs.endblk = endblk ;
	GSArgs.arg1 = arg1 ;
	GSArgs.arg2 = arg2 ;

	// Create the thread that will do the work
	if (!(GSThread = CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
							   0,
							   (LPTHREAD_START_ROUTINE)execute_gantry_operation,
							   (LPVOID)NULL,
							   0,
							   &threadID))) return (FALSE) ;

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	execute_gantry_operation	- dispatcher for gantry settings tool
//											  routines is spawned as separate thread
//
//	Inputs:		None
//
//	Note:		Obtains arguments from static GSARGS structure
//				initialized by parent thread
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if operation code is not recognized
//
/***********************************************************************************/

LOCAL BOOL
execute_gantry_operation ()
{
	int op, startbkt, endbkt, startblk, endblk, arg1, arg2 ;

	// pull out the operation parameters
	op = GSArgs.Op ;
	startbkt = GSArgs.startbkt ;
	endbkt = GSArgs.endbkt ;
	startblk = GSArgs.startblk ;
	endblk = GSArgs.endblk ;
	arg1 = GSArgs.arg1 ;
	arg2 = GSArgs.arg2 ;


	// gantry operation dispatcher
	switch (GSArgs.Op)
	{
		case TUBEGAINS:
			op_tube_gains (startbkt, endbkt, startblk, endblk, arg1, arg2) ;
			break ;
		case BUCKETTEMP:
			op_bucket_temps (startbkt, endbkt) ;
			break ;
		case WATERTEMP:
			op_water_temp () ;
			break ;
		case WATERSHUTDOWN:
			op_water_shutdown (arg1, arg2) ;
			break ;
		case BUCKETSINGLES:
			op_bucket_singles (startbkt, endbkt, (char*)arg1) ;
			break ;
		case HIGHVOLTAGE:
			op_high_voltage (startbkt, endbkt) ;
			break ;
		case LOWVOLTAGE:
			op_low_voltage (startbkt, endbkt) ;
			break ;
		case ULDENERGY:
			op_energy_settings (startbkt, endbkt, 'U') ;
			break ;
		case LLDENERGY:
			op_energy_settings (startbkt, endbkt, 'V') ;
			break ;
		case SCATTERENERGY:
			op_energy_settings (startbkt, endbkt, 'Y') ;
			break ;
		case CFDSETTING:
			op_cfd_settings (startbkt, endbkt, startblk, endblk) ;
			break ;
		case EEPROMVERSION:
			op_prom_version (startbkt, endbkt, 2) ;
			break ;
		case EPROMVERSION:
			op_prom_version (startbkt, endbkt, 1) ;
			break ;
		case BLOCKSINGLES:
			op_block_singles (startbkt, endbkt, startblk, endblk) ;
			break ;
		case HIGHVOLTAGEON:
			op_hv (HIGHVOLTAGEON) ;
			break ;
		case HIGHVOLTAGEOFF:
			op_hv (HIGHVOLTAGEOFF) ;
			break ;
		case ZAPBUCKET:
			op_zap_bucket (startbkt, endbkt) ;
			break ;
		case RESETBUCKET:
			op_reset_bucket (startbkt, endbkt) ;
			break ;
		case BUCKETSTATUS:
			op_bucket_status (startbkt, endbkt) ;
			break ;
		default:
			return (FALSE) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	op_tube_gains		- gets PMT gain settings
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//				int startblk		- starting block number
//				int endblk			- ending block
//				int arg1			- low value used to determine reporting
//				int arg2			- high value used to determine reporting
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_tube_gains (int startbkt, int endbkt, int startblk, int endblk, int arg1, int arg2)
{
	int bkt, blk ;
	char msgstr[64], c ;
	int gain0, gain1, gain2, gain3, status ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		for (blk=startblk;blk<=endblk;blk++)
		{
			select_block (bkt, blk) ;
			if ((status = get_tube_gains (bkt, &gain0, &gain1, &gain2, &gain3)) == GCI_SUCCESS)
			{
				c = ' ' ;
				if ((gain0 < arg1) || (gain0 > arg2)) c = '*' ;
				if ((gain1 < arg1) || (gain0 > arg2)) c = '*' ;
				if ((gain2 < arg1) || (gain0 > arg2)) c = '*' ;
				if ((gain3 < arg1) || (gain0 > arg2)) c = '*' ;
				sprintf (msgstr, "%c%cBucket: %2d  Block: %2d %3d %3d %3d %3d", c, c, bkt, blk, gain0, gain1, gain2, gain3) ;
				post_settings_update (TUBEGAINS, bkt, blk, msgstr) ;
			}
			else if (status == COMM_TIMEOUT)
			{
				post_settings_complete (TUBEGAINS, "Tube Gains Timed Out") ;
				post_settings_complete (TUBEGAINS, "") ;
				return ;
			}

		}
	}
	post_settings_complete (TUBEGAINS, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_bucket_temps		- gets bucket temperatures
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_bucket_temps (int startbkt, int endbkt)
{
	char cmd[10], resp[16], msgstr[64], rc ;
	int arg = -1, bkt ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%dX30", bkt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc == 'T') sprintf (msgstr, "Bucket %2d, Timed Out", bkt) ;
		else sprintf (msgstr, "Bucket %2d, Temperature = %3d degrees C", bkt, arg) ;
		post_settings_update (BUCKETTEMP, bkt, 0, msgstr) ;
	}

	// post the complete message
	post_settings_complete (BUCKETTEMP, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_water_temp	- reads curren water temperature
//
//	Inputs:		None
//
//  Note:		If both input values are 0 (zero) current settings are output
//
//	Outputs:	None
//
//	Returns:	void			- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_water_temp ()
{
	char resp[16], msgstr[64] ;
	int arg ;

	// send it and parse it ;
	send_command (GC, "T2", resp) ;
	parse_gc_response (resp, &arg) ;
	if (resp[0] == 'T') sprintf (msgstr, "Reading water temperature: Timed Out") ;
	else sprintf (msgstr, "Current gantry water temperature sensor: %d degrees C", arg) ;
	post_settings_update (WATERTEMP, 0, 0, msgstr) ;

	// post the complete message
	post_settings_complete (WATERTEMP, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_water_shutdown	- gets or sets the water shutdown limits
//
//	Inputs:		int low				- low shutdown threshold
//				int high			- hogh shutdown threshold
//
//  Note:		If both input values are 0 (zero) current settings are output
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_water_shutdown (int low, int high)
{
	char cmd[16], resp[16], msgstr[64] ;
	int arg ;

	// if both temps are zero read the setting
	if ((low >= 0) && (high >0))
	{
		// set the low
		sprintf (cmd, "K1,%d", low) ;
		send_command (GC, cmd, resp) ;
		if (resp[0] == 'T')
		{
			sprintf (msgstr, "Water Shutdown Timed Out") ;
			post_settings_complete (WATERSHUTDOWN, msgstr) ;
			post_settings_complete (WATERSHUTDOWN, "") ;
			return ;
		}

		// set the high
		sprintf (cmd, "K2,%d", high) ;
		send_command (GC, cmd, resp) ;
		if (resp[0] == 'T')
		{
			sprintf (msgstr, "Water Shutdown Timed Out") ;
			post_settings_complete (WATERSHUTDOWN, msgstr) ;
			post_settings_complete (WATERSHUTDOWN, "") ;
			return ;
		}
	}

	// read the low
	send_command (GC, "J1", resp) ;
	parse_gc_response (resp, &arg) ;
	if (resp[0] == 'T')
	{
		sprintf (msgstr, "Water Shutdown Timed Out") ;
		post_settings_complete (WATERSHUTDOWN, msgstr) ;
		post_settings_complete (WATERSHUTDOWN, "") ;
		return ;
	}
	else sprintf (msgstr, "Current value of water shutdown low: %d", arg) ;
	post_settings_update (WATERSHUTDOWN, 0, 0, msgstr) ;

	// read the high
	send_command (GC, "J2", resp) ;
	parse_gc_response (resp, &arg) ;
	if (resp[0] == 'T')
	{
		sprintf (msgstr, "Water Shutdown Timed Out") ;
		post_settings_complete (WATERSHUTDOWN, msgstr) ;
		post_settings_complete (WATERSHUTDOWN, "") ;
		return ;
	} else sprintf (msgstr, "Current value of water shutdown high: %d", arg) ;
	post_settings_update (WATERSHUTDOWN, 0, 0, msgstr) ;

	// post the complete message
	post_settings_complete (WATERSHUTDOWN, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_bucket_singles	- gets bucket singles and stores to file
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if operation fails to complete
//
/***********************************************************************************/

LOCAL bool
op_bucket_singles (int startbkt, int endbkt, char* fname)
{
	char cmd[10], resp[64], str[64], msgstr[64] ;
	int arg = -1, bkt ;
	HANDLE hfile ;
	bool status ;
	DWORD nbytes ;

	status = TRUE ;
	
	// now try to create the file
	if ((hfile = CreateFile (fname,
							GENERIC_WRITE,
							FILE_SHARE_READ,
							NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL)) == INVALID_HANDLE_VALUE) return (FALSE) ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%dF0", bkt) ;
		send_command (BKT, cmd, resp) ;
		post_settings_update (BUCKETSINGLES, bkt, 0, resp) ;
		sprintf (str, "%s\n", resp) ;
		if (!(WriteFile (hfile, str, strlen (str), &nbytes, NULL))) status = FALSE ;
	}

	CloseHandle (hfile) ;

	// post the complete message
	if (status == FALSE)
	{
		sprintf (msgstr, "Error Writing File %s", fname) ;
		post_settings_complete (BUCKETSINGLES, msgstr) ;
	} else post_settings_complete (BUCKETSINGLES, "") ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	op_high_voltage		- gets bucket high voltage values
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_high_voltage (int startbkt, int endbkt)
{
	char cmd[10], resp[16], msgstr[64], rc ;
	int arg = -1, bkt ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%dX4", bkt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc == 'T') sprintf (msgstr, "Bucket %2d, HV Timed Out", bkt) ;
		else sprintf (msgstr, "Bucket %2d, High Voltage = %4d Volts", bkt, arg) ;
		post_settings_update (HIGHVOLTAGE, bkt, 0, msgstr) ;
	}

	// post the complete message
	post_settings_complete (HIGHVOLTAGE, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_low_voltage		- gets the low voltage reference
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_low_voltage (int startbkt, int endbkt)
{
	char cmd[10], resp[16], msgstr[64], rc ;
	int arg = -1, bkt ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%dX2", bkt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc == 'T') sprintf (msgstr, "Bucket %2d, Reference Voltage Timed Out", bkt) ;
		else sprintf (msgstr, "Bucket %2d, Reference Voltage = %d millivolts", bkt, arg) ;
		post_settings_update (LOWVOLTAGE, bkt, 0, msgstr) ;
	}

	// post the complete message
	post_settings_complete (LOWVOLTAGE, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_energy_settings		- gets the bucket energy settings
//
//	Inputs:		int startbkt			- starting bucket number
//				int endbkt				- ending bucket
//				char decscriptor		- 'U'=uld, 'V'=lld, 'Y'=scatter
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_energy_settings (int startbkt, int endbkt, char desc)
{
	char cmd[10], resp[16], msgstr[64], opstr[20], rc ;
	int arg = -1, bkt, Op ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%d%c0", bkt, desc) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc == 'U')
		{
			Op = ULDENERGY ;
			strcpy (opstr, "ULD") ;
		}
		else if (rc == 'V')
		{
			Op = LLDENERGY ;
			strcpy (opstr, "LLD") ;
		}
		else if (rc == 'S')
		{
			Op = SCATTERENERGY ;
			strcpy (opstr, "Scatter") ;
		}
		else if (rc == 'D')
		{	Op = SCATTERENERGY ;
			strcpy (opstr, "Scatter Disabled") ;
		}
		if (rc == 'T')
		{
			opstr[0] = '\0' ;
			sprintf (msgstr, "Bucket %2d, %s Timed Out", bkt, opstr) ;
		}
		else if (rc == 'D') sprintf (msgstr, "Bucket %2d, %s", bkt, opstr) ;
		else sprintf (msgstr, "Bucket %2d, %s Energy = %d Kev", bkt, opstr, arg) ;
		post_settings_update (Op, bkt, 0, msgstr) ;
	}

	// post the complete message
	post_settings_complete (Op, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_cfd_settings		- gets CFD settings
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//				int startblk		- starting block number
//				int endblk			- ending block
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_cfd_settings (int startbkt, int endbkt, int startblk, int endblk)
{
	int bkt, blk, arg ;
	char cmd[10], resp[64], msgstr[64], rc ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		for (blk=startblk;blk<=endblk;blk++)
		{
			select_block (bkt, blk) ;
			sprintf (cmd, "%dO256", bkt) ;
			send_command (BKT, cmd, resp) ;
			parse_fesb_response (resp, &rc, &arg) ;
			if (rc == 'T') sprintf (msgstr, "Bucket %2d Block %2d CFD Timed Out", bkt, blk) ;
			else sprintf (msgstr, "Bucket %2d Block %2d CFD Setting = %d", bkt, blk, arg) ;
			post_settings_update (CFDSETTING, bkt, blk, msgstr) ;
		}
	}
	post_settings_complete (CFDSETTING, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_prom_version		- gets the firmare version for eprom or eeprom
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//				int prom			- selects device 1=eprom, 2= eeprom
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_prom_version (int startbkt, int endbkt, int prom)
{
	char cmd[10], resp[16], msgstr[64], opstr[16], rc ;
	int arg = -1, bkt, Op ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%dB%d", bkt, prom) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (prom == 1)
		{
			Op = EPROMVERSION ;
			strcpy (opstr, "EPROM") ;
		}
		else if (prom == 2)
		{
			Op = EEPROMVERSION ;
			strcpy (opstr, "EEPROM") ;
		}

		if (rc == 'T') sprintf (msgstr, "Bucket %2d, %s Timed Out", bkt, opstr) ;
		else sprintf (msgstr, "Bucket %2d, %s Version = %d", bkt, opstr, arg) ;
		post_settings_update (Op, bkt, 0, msgstr) ;
	}

	// post the complete message
	post_settings_complete (Op, "PROM Version Report Complete...") ;
}

/***********************************************************************************/
//
//
//	Function:	op_zap_bucket	- does a 'Z1' reset on buckets
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//
//	Outputs:	None
//
//	Returns:	int				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL BOOL
op_zap_bucket (int startbkt, int endbkt)
{
	char cmd[10], resp[64], msg[64] ;
	int arg = -1, bkt, blk, nblks ;
	BOOL status = TRUE;

	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return FALSE ;
	}

	// loop through buckets and blocks
	nblks = scanner->blocks() ;
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		for (blk=0;blk<nblks;blk++)
		{
			sprintf (msg, "Zapping bucket %d block %d", bkt, blk) ;
			post_settings_update (ZAPBUCKET, 0, 0, msg) ;

			// wait until done
			while (TRUE)
			{
				sprintf (cmd, "%dS%d", bkt, blk) ;
				send_command (BKT, cmd, resp) ;
				if (resp[3] == 'N') break ;
				Sleep (1000) ;
			}

			// send it and parse it
			sprintf (cmd, "%dZ1", bkt) ;
			send_command (BKT, cmd, resp) ;
		}
	}

	// post the complete message
	post_settings_complete (ZAPBUCKET, "Zap Bucket Commands Complete...") ;

	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	op_reset_bucket	- does a 'Z0' reset on buckets
//
//	Inputs:		int startbkt		- starting bucket number
//				int endbkt			- ending bucket
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_reset_bucket (int startbkt, int endbkt)
{
	char cmd[10], resp[96] ;
	int arg = -1, bkt, status ;

	// post the resetting message
	sprintf (resp, "Resetting Buckets %d thru %d...", startbkt, endbkt) ;
	post_settings_update (RESETBUCKET, 0, 0, resp) ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%dZ0", bkt) ;
		send_command (BKT, cmd, resp) ;
	}


	// monitor the reset operation
	sprintf (resp, "Monitoring Reset...") ;
	post_settings_update (RESETBUCKET, 0, 0, resp) ;
	status = monitor_bkt_reset (startbkt, endbkt, WM_SETTINGUPD, TRUE) ;

	// post the complete
	sprintf (resp, "Reset buckets operation completed ") ;
	if (status) strcat (resp, "OK") ;
	else strcat (resp, "with error") ;
	post_settings_complete (RESETBUCKET, resp) ;
}

/***********************************************************************************/
//
//
//	Function:	op_block_singles	- gets block singles
//
//	Inputs:		int startbkt		- starting buckt number
//				int endbkt			- ending bucket
//				int startblk		- starting block number
//				int endblk			- ending block
//
//	Outputs:	None
//
//	Returns:	void				- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_block_singles (int startbkt, int endbkt, int startblk, int endblk)
{
	int bkt, blk, singles[3] ;
	char cmd[10], resp[64], msgstr[64], rc ;

	// loop through buckets and blocks
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		for (blk=startblk;blk<=endblk;blk++)
		{
			select_block (bkt, blk) ;
			sprintf (cmd, "%dF4", bkt) ;
			send_command (BKT, cmd, resp) ;
			parse_fesb_response (resp, &rc, &singles[0]) ;
			if (rc == 'T') sprintf (msgstr, "Bucket %2d Block %2d Singles Timed Out", bkt, blk) ;
			else sprintf (msgstr, "Bucket %2d Block %2d Singles = %d, %d", bkt, blk, singles[0], singles[1]) ;
			post_settings_update (BLOCKSINGLES, bkt, blk, msgstr) ;
		}
	}
	post_settings_complete (BLOCKSINGLES, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_hv	- turns on/off high voltage
//
//	Inputs:		int Op	- HIGHVOLTAGEON | HIGHVOLTAGEOFF
//
//	Outputs:	None
//
//	Returns:	void	- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_hv (int Op)
{
	char cmd[10], resp[10], msgstr[64], opstr[16] ;
	int arg ;

	switch (Op)
	{
		case HIGHVOLTAGEON:
			arg = 1 ;
			strcpy (opstr, "ON") ;
			break ;
		case HIGHVOLTAGEOFF:
			arg = 2 ;
			strcpy (opstr, "OFF") ;
			break ;
		default:
			arg = 2 ;
	}

	sprintf (cmd, "PW%d", arg) ;
	send_command (GC, cmd, resp) ;
	parse_gc_response (resp, &arg) ;
	if ((strncmp (resp, "GN", 2) == 0) && (arg == 0))
		sprintf (msgstr, "High Voltage Turned %s", opstr) ;
	else
		sprintf (msgstr, "Warning, High Voltage Command Failed... Check Voltage Manually !!!") ;
	post_settings_update (Op, 0, 0, msgstr) ;
	post_settings_complete (Op, "") ;
}

/***********************************************************************************/
//
//
//	Function:	op_bucket_status	- retrieves bucket status
//
//	Inputs:		int startbkt	- starting bucket number
//				int endbkt		- ending bucket number
//
//	Outputs:	None
//
//	Returns:	void			- shows status by sending window messages
//
/***********************************************************************************/

LOCAL void
op_bucket_status (int startbkt, int endbkt)
{
	char cmd[10], resp[16], msgstr[64], rc ;
	int arg = -1, bkt ;

	// loop through buckets
	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// send it and parse it
		sprintf (cmd, "%dX0", bkt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		sprintf (msgstr, "Bucket %2d Status %c %d", bkt, rc, arg) ;
		if (rc == 'C') strcat (msgstr, " Booting") ;
		if (rc == 'T') strcat (msgstr, " Timed Out") ;
		if (rc == 'D') strcat (msgstr, " Running Diagnostics") ;
		post_settings_update (BUCKETSTATUS, bkt, 0, msgstr) ;
	}

	// post the complete message
	post_settings_complete (BUCKETSTATUS, "") ;
}

/***********************************************************************************/
//
//
//	Function:	LoadBucketDB	- exported routine to initiate either and
//								  upload or download of a bucket 
//
//	Inputs:		int startbkt	- starting bucket number
//				int endbkt		- ending bucket number
//				char* filename	- file to store to or retrieve database records
//				int controller	- controller ID
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE on failure
//
/***********************************************************************************/

int
LoadBucketDB (int startbkt, int endbkt, char* filename, int direction, int controller)
{
	//int arg ;
	//char cmd[10], resp[10], rc ;
	static DBARGS dbargs ;
	DWORD threadID ;
	
	//::MessageBox (NULL, "In LoadBucketDB", "PetCTGCIDll", MB_OK) ;

	// turn singles polling off
	post_status_update (-1, WM_DBLOADUPD, "Turning off singles polling") ;
	disable_singles_polling () ;

	// launch the work
	dbargs.startbkt = startbkt ;
	dbargs.endbkt = endbkt ;
	dbargs.file = filename ;
	dbargs.addr = 0x2000 ;
	dbargs.cnt = 0x1600 ;
	if (direction == BACKUP)
	{	
		CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
					   0,
					   (LPTHREAD_START_ROUTINE)upload_bucket_db,
					   (LPVOID)&dbargs,
					   0,
					   &threadID) ;

	}
	else if (direction == RESTORE)
	{
		CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
					   0,
					   (LPTHREAD_START_ROUTINE)download_bucket_db,
					   (LPVOID)&dbargs,
					   0,
					   &threadID) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	upload_bucket_db	- uploads and saves bucket databases 
//
//	Inputs:		DBARGS* dbargs		- pointer to DBARGS structure
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure occurs while uploading
//
/***********************************************************************************/

LOCAL BOOL
upload_bucket_db (DBARGS* dbargs)
{
	HANDLE hfile ;
	DWORD nbytes ;
	char cmd[10], resp[64], record[64], wmsg[64], rc ;
	int arg, nrecs, reccnt, new_percent, bkt, startbkt, endbkt, recleft, timeleft, percent_mark ;
	unsigned address, upload_cnt ;
	tGantryResult *fwComplete ;
	DWORD err ; 

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL)))
		{
			post_status_update (-1, WM_DBLOADFATAL, "Could not access model object") ;
			return FALSE ;
		}
	}

	// pick up args
	startbkt = dbargs->startbkt ;
	endbkt = dbargs->endbkt ;
	address = dbargs->addr ;
	upload_cnt = dbargs->cnt ;

	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		// Create or open the target file
		hfile = CreateFile (dbargs->file,
							GENERIC_WRITE,
							FILE_SHARE_READ,
							NULL,
							OPEN_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL) ;

		// we need this to determine if the file existed
		err = GetLastError () ;

		// did it fail altogether
		if (hfile == INVALID_HANDLE_VALUE)
		{
			post_status_update (-1, WM_DBLOADFATAL, "Could not open file") ;
			return FALSE ;
		}

		// if the file did exist move the pointer to the end of what is there
		if (err == ERROR_ALREADY_EXISTS) SetFilePointer (hfile, NULL, NULL, FILE_END) ;

		// write a header record to identify bucket number
		sprintf (record, "!%d\n", bkt) ;
		if (!(WriteFile (hfile, record, strlen (record), &nbytes, NULL)))
		{
			CloseHandle (hfile) ;
			post_status_update (-1, WM_DBLOADFATAL, "Could not open file") ;
			return (FALSE) ;
		}

		// set the upload address to the bucket histgram ram
		sprintf (cmd, "%dL%d", bkt, address) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc != 'N') 
		{
			CloseHandle (hfile) ;
			sprintf (wmsg, "Bucket %d failed BACKUP operation", bkt) ;
			post_status_update (-1, WM_DBLOADFATAL, wmsg) ;
			return (FALSE) ;
		}

		// set the number of bytes to upload
		sprintf (cmd, "%dQ%d", bkt, upload_cnt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc != 'N')
		{
			CloseHandle (hfile) ;
			sprintf (wmsg, "Bucket %d failed restore operation", bkt) ;
			post_status_update (-1, WM_DBLOADFATAL, wmsg) ;
			return (FALSE) ;
		}
	
		// upload record and store
		reccnt = 0 ;
		nrecs = scanner->bucketDbSize() / DATASIZE ;
		nrecs = 0x1600 / DATASIZE ;
		recleft = nrecs ;
		percent_mark = 5 ;
		new_percent = 0 ;
		sprintf (wmsg, "Backing up bucket %d", bkt) ;
		post_status_update (-1, WM_DBLOADUPD, wmsg) ;
		timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
		post_status_update (timeleft, WM_DBLOADUPD, "") ;
		post_status_update (new_percent, WM_STATUSDBLOAD, "") ;
		while (TRUE)
		{
			// get a record
			sprintf (cmd, "%dQ0", bkt) ;
			send_command (BKT, cmd, resp) ;
			if (resp[3] != ':')
			{
				CloseHandle (hfile) ;
				sprintf (wmsg, "Bucket %d failed during BACKUP operation", bkt) ;
				post_status_update (-1, WM_DBLOADFATAL, wmsg) ;
				return FALSE ;
			}
		
			// append terminator and store to file
			sprintf (record, "%s\n", &resp[3]) ;
			if (!(WriteFile (hfile, record, strlen (record), &nbytes, NULL)))
			{
				CloseHandle (hfile) ;
				sprintf (wmsg, "Bucket %d failed BACKUP while writing to file", bkt) ;
				post_status_update (-1, WM_DBLOADFATAL, wmsg) ;
				return FALSE ;
			}

			// compute percent done
			recleft = nrecs - reccnt++ ;
			timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
			new_percent = (int)(100.0 * (((float)nrecs - (float)recleft) / (float)nrecs)) ;

			// update the progress bar
			if (new_percent >= percent_mark)
			{
				post_status_update (new_percent, WM_STATUSDBLOAD, "") ;
				post_status_update (timeleft, WM_DBLOADUPD, "") ;
				percent_mark += 5 ;
			}

			// is it the final record
			if (strncmp (&resp[3], ":00000001FF", strlen (":00000001FF")) == 0) break ;
			else recleft++ ;
		}

		// close file
		CloseHandle (hfile) ;

		// post the complete for each bucket
		sprintf (wmsg, "Bucket %d backup complete", bkt) ;
		post_status_update (-1, WM_DBLOADUPD, wmsg) ;

		// update status and progress bar
		post_status_update (100, WM_STATUSDBLOAD, "") ;
	}

	// turn singles back on
	post_status_update (-1, WM_DBLOADUPD, "Turning on singles polling") ;
	enable_singles_polling () ;

	// post the complete
	if (fwComplete = NewGantryResult ())
	{
		strcpy(fwComplete->StatusMsg, "BACKUP operation complete") ;
		PostWindowMessage(hWnd, WM_DBLOADCOMPLETE, (WPARAM)fwComplete) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	download_bucket_db	- downloads bucket databases 
//
//	Inputs:		DBARGS* dbargs		- pointer to DBARGS structure
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure occurs downloading
//
/***********************************************************************************/

LOCAL BOOL
download_bucket_db (DBARGS* dbargs)
{
	FILE *fptr ;
	int fbkt, rlen, nrecs, reccnt, new_percent, percent_mark ;
	int bkt, startbkt, endbkt, restore_cnt, recleft, timeleft ;
	char cmd[10], record[64], resp[64], wmsg[64] ;
	tGantryResult *fwComplete ;
	bool found ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL)))
		{
			post_status_update (-1, WM_DBLOADFATAL, "Could not access model object") ;
			return FALSE ;
		}
	}

	// pick up args
	startbkt = dbargs->startbkt ;
	endbkt = dbargs->endbkt ;
	restore_cnt = dbargs->cnt ;

	// Try to open the target file
	if((fptr = fopen(dbargs->file, "r" )) == NULL )
	{
		post_status_update (-1, WM_DBLOADUPD, "Could not open file") ;
		return FALSE ;
	}

	for (bkt=startbkt;bkt<=endbkt;bkt++)
	{
		found = FALSE ;
		// Locate the bucket record
		while (TRUE)
		{
			// read record
			if(fgets(record, 64, fptr) == NULL)
			{
				sprintf (wmsg, "Could not locate data base for bucket %d", bkt) ;
				post_status_update (-1, WM_DBLOADUPD, wmsg) ;
				fseek (fptr, 0, SEEK_SET) ;
				break ;
			}

			// did we get a hit on the bucket
			if (record[0] == '!')
			{
				fbkt = atoi (&record[1]) ;
				if (fbkt == bkt)
				{	found = true ;
					break ;
				}
			}
		}

		if (found)
		{	
			sprintf (wmsg, "Restoring bucket %d", bkt) ;
			post_status_update (-1, WM_DBLOADUPD, wmsg) ;

			// Set the bucket to download mode
			post_status_update (-1, WM_DBLOADUPD, "Setting bucket to download mode") ;
			sprintf (cmd, "%dW1", bkt) ;
			send_command (BKT, cmd, resp) ;

			if (resp[3] != 'N')
			{
				fclose (fptr) ;
				sprintf (wmsg, "Error setting bucket %d to download mode", bkt) ;
				post_status_update (-1, WM_DBLOADUPD, wmsg) ;
				return (FALSE) ;
			}

			// Now download the database
			reccnt = 0 ;
			nrecs = scanner->bucketDbSize() / DATASIZE ;
			recleft = nrecs ;
			percent_mark = 5 ;
			new_percent = 0 ;
			timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
			post_status_update (timeleft, WM_DBLOADUPD, "") ;
			post_status_update (new_percent, WM_STATUSDBLOAD, "") ;
			while (TRUE)
			{
				// read a record
				fgets(record, 64, fptr) ;

				// Null terminate 
				rlen = strlen (record) ;
				record[rlen] = '\0' ;

				// load the record
				send_command (BKT, record, resp) ;

				// compute percent done
				recleft = nrecs - reccnt++ ;
				timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
				new_percent = (int)(100.0 * (((float)nrecs - (float)recleft) / (float)nrecs)) ;

				// update the progress bar
				if (new_percent >= percent_mark)
				{
					post_status_update (new_percent, WM_STATUSDBLOAD, "") ;
					post_status_update (timeleft, WM_DBLOADUPD, "") ;
					percent_mark += 5 ;
				}
					
				if (strncmp (record, ":00000001FF", strlen (":00000001FF")) == 0) break ;
				else recleft++ ;
			}

			// reset the bucket
			sprintf (wmsg, "Resetting bucket %d", bkt) ;
			post_status_update (-1, WM_DBLOADUPD, wmsg) ;
			ResetBuckets (bkt, bkt, 0) ;
		}
	}

	// allow some settle time
	Sleep (2000) ;

	// monitor the reset... this can take a while
	if (!(monitor_bkt_reset (startbkt, endbkt, WM_DBLOADUPD, TRUE)))
	{
		sprintf (wmsg, "Bucket reset operation failed", bkt) ;
		post_status_update (-1, WM_DBLOADUPD, wmsg) ;
	}

	// update status and progress bar
	post_status_update (100, WM_STATUSDBLOAD, "") ;

	// close the file
	fclose (fptr) ;

	// turn singles back on
	post_status_update (-1, WM_DBLOADUPD, "Turning on singles polling") ;
	enable_singles_polling () ;

	// post the complete
	if (fwComplete = NewGantryResult ())
	{
		strcpy(fwComplete->StatusMsg, "RESTORE operation complete") ;
		PostWindowMessage(hWnd, WM_DBLOADCOMPLETE, (WPARAM)fwComplete) ;
	}
	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	FirmwareVer	- gets the firmware version of specified controller 
//
//	Inputs:		int addr	- controller address
//
//	Outputs:	None
//
//	Returns:	Controller's firmware version
//
/***********************************************************************************/

int
FirmwareVer (int addr)
{
	int arg ;
	char cmd[10], resp[10], rc ;

	// get the firmware version for the return
	switch (addr)
	{
		// gantry controller
		case -1:
			send_command (GC, "B2", resp) ;
			parse_gc_response (resp, &arg) ;
			break ;
		// coincidence processor
		case 64:
			send_command (IPCP, "64B2", resp) ;
			parse_fesb_response (resp, &rc, &arg) ;
			break ;
		// bucket
		default:
			sprintf (cmd, "%dB2", addr) ;
			send_command (BKT, cmd, resp) ;
			parse_fesb_response (resp, &rc, &arg) ;
			break ; 
	}
	return (arg) ;
}


/***********************************************************************************/
//
//
//	Function:	LoadFirmware	- this is the exported routine that spins the
//								  working threads to do firmware downloads 
//
//	Inputs:		int startaddr	- starting controller address
//				int endaddr		- ending controller address
//				char *file		- file containing firmware download records
//				int controller	- controller ID
//
//	Outputs:	None
//
//	Returns:	TRUE
//
/***********************************************************************************/

BOOL
LoadFirmware (int startaddr, int endaddr, char* file, int controller)
{
	static DBARGS dbargs ;

	// fill the argument structure
	dbargs.startbkt = startaddr ;
	dbargs.endbkt = endaddr ;
	dbargs.file = file ;

	// turn off singles polling
	post_status_update (-1, WM_FIRMLOADUPD, "Turning off singles polling") ;
	disable_singles_polling () ;
	
	// go do the work
	if (controller == CTRL_Gantry)
	{
		_beginthread (load_gc_firmware, 0, &dbargs) ;
	}
	else if (controller == CTRL_Bucket)
	{
		_beginthread (load_bkt_firmware, 0, &dbargs) ;
	
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	load_gc_firmware	- this routine is spawned as its own thread
//									  to download bucket firmware
//
//	Inputs:		void* args			- Pointer to structure type DBARGS
//
//	Outputs:	None
//
//	Returns:	void				- status is handle by updating the parent window
//
/***********************************************************************************/

LOCAL void
load_gc_firmware (void* args)
{
	FILE *fptr ;
	int rlen ;
	char resp[10], record[64], wmsg[64] ;
	DBARGS *dbargs ;
	int reccnt, nrecs, recleft ;
	int percent_mark, timeleft, new_percent ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL)))
		{
			post_status_update (-1, WM_FIRMLOADFATAL, "Could not access model object") ;
			return ;
		}
	}


	// Try to open the target file
	dbargs = (DBARGS*)args ;
	if ((fptr = fopen (dbargs->file, "r")) == NULL)
	{
		post_status_update (-1, WM_FIRMLOADFATAL, "Could not open file") ;
		return ;
	}

	// Select the page code
	send_command (GC, "C0", resp) ;
	if (strncmp (resp, "GN 0", 4))
	{
		fclose (fptr) ;
		post_status_update (-1, WM_FIRMLOADFATAL, "Could not select page code") ;
		return ;
	}

	// Set gantry controller to download mode
	post_status_update (-1, WM_FIRMLOADUPD, "Setting gantry controller to download mode") ;
	send_command (GC, "W1", resp) ;
	if (strncmp (resp, "GN 0", 4))
	{
		fclose (fptr) ;
		post_status_update (-1, WM_FIRMLOADFATAL, "Could not set download mode") ;
		return ;
	}

	// Send down the records
	reccnt = 0 ;

	// does not take into account PRT models yet
	//nrecs = ((scanner->maxCodePage() + 1) * DATAPAGESIZE) / DATASIZE ;
	nrecs = DATAPAGESIZE / DATASIZE ;

	recleft = nrecs ;
	percent_mark = 5 ;
	new_percent = 0 ;
	timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
	post_status_update (timeleft, WM_FIRMLOADUPD, "") ;
	post_status_update (new_percent, WM_STATUSFIRMLOAD, "") ;
	while (TRUE)
	{
		// read a record
		record[0] = '\0' ;
		while (record[0] != ':') fgets(record, 64, fptr) ;
		
		// terminate with carriage return instead of new line
		rlen = strlen (record) ;
		record[rlen-1] = '\0' ;

		// load the record
		send_command (GC, record, resp) ;

		// compute update info
		recleft = nrecs - reccnt++ ;
		timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
		new_percent = (int)(100.0 * (((float)nrecs - (float)recleft) / (float)nrecs)) ;

		// update the progress bar
		if (new_percent >= percent_mark)
		{
			post_status_update (new_percent, WM_STATUSFIRMLOAD, "") ;
			post_status_update (timeleft, WM_FIRMLOADUPD, "") ;
			percent_mark += 5 ;
		}

		// change page code if necessary
		if (strncmp (record, ":00000001FF", strlen (":00000001FF")) == 0) break ;
	}

	// close the file
	fclose (fptr) ;

	// Deal with the reset
	post_status_update (-1, WM_FIRMLOADUPD, "Monitoring gantry controller reset") ;
	do {
		Sleep (3000) ;
		send_command (GC, "X0", resp) ;
		sprintf (wmsg, "Gantry controller reset status %s", resp) ;
		post_status_update (-1, WM_FIRMLOADUPD, wmsg) ;
	} while(strncmp(resp, "GN 0", 4) && resp[0] != 'X') ;
		
	// turn on the singles
	post_status_update (-1, WM_FIRMLOADUPD, "Turning on singles polling") ;
	enable_singles_polling () ;

	// post the complete
	post_status_update (-1, WM_FIRMLOADCOMPLETE, "Gantry controller firmware load complete") ;

	return ;
}

/***********************************************************************************/
//
//
//	Function:	load_bkt_firmware	- this routine is spawned as its own thread
//									  to download bucket firmware
//
//	Inputs:		void* args			- Pointer to structure type DBARGS
//
//	Outputs:	None
//
//	Returns:	void				- status is handle by updating the parent window
//
/***********************************************************************************/

LOCAL void
load_bkt_firmware (void* args)
{
	FILE *fptr ;
	int rlen ;
	int page, arg, bkt, startaddr, endaddr ;
	char cmd[10], resp[10], record[64], wmsg[64], rc ;
	int reccnt, nrecs, recleft ;
	int percent_mark, timeleft, new_percent ;
	DBARGS *dbargs ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL)))
		{
			post_status_update (-1, WM_FIRMLOADFATAL, "Could not access model object") ;
			return ;
		}
	}

	// pick up dbargs
	dbargs = (DBARGS*)args ;

	// unpack arguments
	startaddr = dbargs->startbkt ;
	endaddr = dbargs->endbkt ;

	// Try to open the target file
	if ((fptr = fopen (dbargs->file, "r")) == NULL)
	{
		fclose (fptr) ;
		post_status_update (-1, WM_FIRMLOADFATAL, "Could not open file") ;
		return ;
	}

	// Turn off motion polling
	post_status_update (-1, WM_FIRMLOADUPD, "Turning off motion polling") ;
	send_command (GC, "GS2,0", resp) ;
	if (strncmp (resp, "GN 0", 4))
	{
		fclose (fptr) ;
		post_status_update (-1, WM_FIRMLOADFATAL, "Could not turn off motion polling") ;
		return ;
	}

	// Disable front panel updating
	send_command (GC, "GS3", resp) ;
	post_status_update (-1, WM_FIRMLOADUPD, "Disabling front panel update") ;
	if (strncmp (resp, "GN 0", 4))
	{
		fclose (fptr) ;
		post_status_update (-1, WM_FIRMLOADFATAL, "Could not disable front panel update") ;
		send_command (GC, "GS1,0", resp) ;
		return ;
	}

	// Select the page code
	page = 0 ;
	for (bkt=startaddr;bkt<=endaddr;bkt++)
	{
		sprintf (cmd, "%dC%d", bkt, page) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc != 'N' && arg)
		{
			fclose (fptr) ;
			post_status_update (-1, WM_FIRMLOADFATAL, "Could not select page code") ;
			send_command (GC, "GS1,0", resp) ;
			send_command (GC, "GS4", resp) ;
			return ;
		}
		
		// set the bucket to download mode
		sprintf (wmsg, "Setting bucket %d to download mode", bkt) ;
		post_status_update (-1, WM_FIRMLOADUPD, wmsg) ;
 		sprintf (cmd, "%dW1", bkt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc != 'N' && arg)
		{
			fclose (fptr) ;
			post_status_update (-1, WM_FIRMLOADFATAL, "Could not set download mode") ;
			send_command (GC, "GS1,0", resp) ;
			send_command (GC, "GS4", resp) ;
			return ;
		}
	}
			
	// now do the download
	reccnt = 0 ;
	nrecs = ((scanner->maxCodePage() + 1) * DATAPAGESIZE) / DATASIZE ;
	recleft = nrecs ;
	percent_mark = 5 ;
	new_percent = 0 ;
	timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
	post_status_update (timeleft, WM_FIRMLOADUPD, "") ;
	post_status_update (new_percent, WM_STATUSFIRMLOAD, "") ;
	post_status_update (-1, WM_FIRMLOADUPD, "Starting firmware download") ;
	while (TRUE)
	{
		// read a record
		record[0] = '\0' ;
		while (record[0] != ':') fgets(record, 64, fptr) ;
		
		// terminate with carriage return instead of new line
		rlen = strlen (record) ;
		record[rlen-1] = '\0' ;

		// load the record
		send_command (BKT, record, resp) ;

		// compute update info
		recleft = nrecs - reccnt++ ;
		timeleft = (int)(((float)recleft * (float)XFRTIME) / (float)1000) ;
		new_percent = (int)(100.0 * (((float)nrecs - (float)recleft) / (float)nrecs)) ;

		// update the progress bar
		if (new_percent >= percent_mark)
		{
			post_status_update (new_percent, WM_STATUSFIRMLOAD, "") ;
			post_status_update (timeleft, WM_FIRMLOADUPD, "") ;
			percent_mark += 5 ;
		}

		// change page code if necessary
		if (strncmp (record, ":00000001FF", strlen (":00000001FF")) == 0)
		{
			if (++page > 1) break ;
			else
			{
				Sleep (1000) ;
				for (bkt=startaddr;bkt<=endaddr;bkt++)
				{
					sprintf (cmd, "%dC%d", bkt, page) ;
					send_command (BKT, cmd, resp) ;
					parse_fesb_response (resp, &rc, &arg) ;
					if (rc != 'N' && arg)
						post_status_update (-1, WM_FIRMLOADFATAL, "Could not select page code") ;
					sprintf (cmd, "%dW1", bkt) ;
					send_command (BKT, cmd, resp) ;
					parse_fesb_response (resp, &rc, &arg) ;
					if (rc != 'N' && arg)
						post_status_update (-1, WM_FIRMLOADFATAL, "Could not set download mode") ;
				}
			}
		}
	}

	// update status and progress bar
	post_status_update (100, WM_STATUSFIRMLOAD, "") ;

	// close the file
	fclose (fptr) ;

	// send monitoring message
	sprintf (wmsg, "Monitoring bucket reset for buckets %d thru %d", startaddr, endaddr) ;
	post_status_update (-1, WM_FIRMLOADUPD, wmsg) ;

	// allow some settle time
	Sleep (2000) ;

	// monitor the reset... this can take a while
	if (!(monitor_bkt_reset (startaddr, endaddr, WM_FIRMLOADUPD, TRUE)))
	{
		sprintf (wmsg, "Bucket reset operation failed", bkt) ;
		post_status_update (-1, WM_FIRMLOADUPD, wmsg) ;
	}

	// turn on the singles
	post_status_update (-1, WM_FIRMLOADUPD, "Turning on singles polling") ;
	enable_singles_polling () ;

	// re-enable motions polling and panel update
	post_status_update (-1, WM_FIRMLOADUPD, "Turning on motions polling") ;
	send_command (GC, "GS1,0", resp) ;
	post_status_update (-1, WM_FIRMLOADUPD, "Turning on front panel update") ;
	send_command (GC, "GS4", resp) ;

	// post the complete
	post_status_update (-1, WM_FIRMLOADCOMPLETE, "Bucket firmware load complete") ;

	// done
	return ;
}

/***********************************************************************************/
//
//
//	Function:	SetGantryOutputMode	- sets all buckets to single or dual channel mode
//
//	Inputs:		int mode			- output mode 0=single channel mode, 1=dual
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if any bucket fails to set
//
/***********************************************************************************/

BOOL
SetGantryOutputMode (int mode)
{
	int arg, bkt, nbkts, ntries ;
	int bkt_map[GCI_MAXBUCKETS], bkts_done ;
	char cmd[10], resp[10], rc ;
	BOOL done ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return FALSE ;
	}

	// make sure this system needs this
	if (scanner->bucketOutputChannels() == 1) return (TRUE) ;

	// get the number of buckets
	nbkts = scanner->buckets() ;

	// set the buckets and initialize bucket map
	for (bkt=0;bkt<nbkts;bkt++)
	{
		bkt_map[bkt] = 0 ;
		if (!(set_bkt_output_mode (bkt, mode))) return (FALSE) ;
	}

	// wait til they all set
	bkts_done = 0 ;
	ntries = MAX_WAIT_SINGLE_DUAL ;
	while (TRUE)
	{
		done = TRUE ;
		Sleep (DELAY_SINGLE_DUAL) ;
		ntries -= (DELAY_SINGLE_DUAL / 1000) ;
		if (ntries <= 0) return FALSE ;
		for (bkt=0;bkt<nbkts;bkt++)
		{
			sprintf (cmd, "%dS0", bkt) ;
			send_command (BKT, cmd, resp) ;
			parse_fesb_response (resp, &rc, &arg) ;
			if (rc == 'N') bkt_map[bkt] = 1 ;
			{
				for (int i=0;i<nbkts;i++) 
					if (!bkt_map[i]) done = FALSE ;
			}
		}
		if (done) break ;
	}
	return TRUE ;
}

/***********************************************************************************/
//
//
//	Function:	ChkGantryOutputMode	- checks the data output mode of the buckets
//
//	Inputs:		int mode			- mode to check for (single / dual)
//
//	Outputs:	None
//
//	Returns:	Returns TRUE | FALSE if any bucket does not match the mode
//									 or if an error occurs with the response
//
/***********************************************************************************/

BOOL
ChkGantryOutputMode (int mode)
{
	int arg, bkt, nbkts, rval ;
	char cmd[10], resp[10], rc ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return FALSE ;
	}

	if (mode == SINGLE_CHANNEL) rval = 12 ;
	else if (mode == DUAL_CHANNEL) rval = 6 ;

	nbkts = scanner->buckets() ;
	for (bkt=0;bkt<nbkts;bkt++)
	{
		sprintf (cmd, "%dR199", bkt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;
		if (rc != 'D') return FALSE ;
		if (arg != rval) return FALSE ;

	}
	return TRUE ;
}

/***********************************************************************************/
//
//
//	Function:	SetBktOutputMode	- sets a bucket to single or dual channel mode,
//									  monitors setting and verifies the outcome
//
//	Inputs:		int bkt				- bukcet to set
//				int mode			- output mode
//
//	Outputs:	None
//
//	Returns:	GCI_SUCCESS | GCI_ERROR
//
/***********************************************************************************/

int
SetBktOutputMode (int bkt, int mode)
{
	int arg, ntries, status ;
	char cmd[10], resp[10], rc ;

	// initialize the scanner model object if necessary
	if (!scanner)
	{
		scanner = &gm ;
		if (!(scanner->setModel(DEFAULT_SYSTEM_MODEL))) return (GCI_ERROR) ;
	}

	// make sure this system needs this
	if (scanner->bucketOutputChannels() == 1) return (GCI_SUCCESS) ;

	// set the mode
	if (!(set_bkt_output_mode (bkt, mode))) return (GCI_ERROR) ;

	// wait for it to set
	status = GCI_ERROR ;
	ntries = MAX_RETRY_SINGLE_DUAL ;
	while (ntries--) 
	{
		sprintf (cmd, "%dS0", bkt) ;
		send_command (BKT, cmd, resp) ;
		parse_fesb_response (resp, &rc, &arg) ;

		// determine status
		if ((rc == 'N') && (arg == 0))
		{
			status = GCI_SUCCESS ;
			break ;
		}
		else if ((rc == 'S') && (arg == 0))
		{
			status = GCI_SUCCESS ;
			break ;
		}
		else if (rc == 'T')
		{
			status = COMM_TIMEOUT ;
			break ;
		}
		else Sleep (DELAY_SINGLE_DUAL) ;
	}
	return (status) ;
}

/***********************************************************************************/
//
//
//	Function:	GetBktOutputMode	- get the output mode of a bucket
//
//	Inputs:		int bkt				- bukcet to set
//
//	Outputs:	None
//
//	Returns:	SINGLE_CHANNEL_MODE | DUAL_CHANNEL_MODE | GCI_ERROR
//
/***********************************************************************************/

GetBktOutputMode	(int bkt)
{
	char cmd[10], resp[10], rc ;
	int arg, ret ;

	// request the buckets output mode
	sprintf (cmd, "%dR199", bkt) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc == 'T') return (COMM_TIMEOUT) ;
	if (rc != 'D') return (GCI_ERROR) ;
	else
	{
		if (arg == 6) ret =  DUAL_CHANNEL ;
		else if (arg == 12) ret = SINGLE_CHANNEL ;
		else ret = GCI_ERROR ;
	}
	return (ret) ;
}

/***********************************************************************************/
//
//
//	Function:	set_bkt_output_mode	- sets single / dual output mode for a bucket
//
//	Inputs:		int mode			- output mode
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if failure
//
/***********************************************************************************/

LOCAL BOOL
set_bkt_output_mode	(int bkt, int mode)
{
	char cmd[10], resp[10], rc ;
	int arg ;

	// setup correct command
	mode += 200 ;
	
	// set bucket to single channel mode
	sprintf (cmd, "%dR%d", bkt, mode) ;
	send_command (BKT, cmd, resp) ;
	parse_fesb_response (resp, &rc, &arg) ;
	if (rc != 'N') return (FALSE) ;
	return TRUE ;
}

/***********************************************************************************/
//
//
//	Function:	monitor_bkt_reset	- monitors the reset operation of buckets
//
//	Inputs:		int startbkt		- starting bucket number that was reset
//				int endbkt			- ending bucket
//				int message			- message ID to use if updating
//				bool report			- report update flag, true update, false no update
//
//	Outputs:	None
//
//	Returns:	TRUE | FALSE if reset operation failed for any bucket
//
/***********************************************************************************/

LOCAL BOOL
monitor_bkt_reset (int startbkt, int endbkt, int message, bool report)
{
	char cmd[10], resp[16], msgstr[64], rc ;
	int arg = -1, bkt, i, bktdone[24], timeout = 0 ;
	BOOL done ;

	// clear the bucket tracker array
	for (i=0;i<24;i++) bktdone[i] = 0 ;

	// wait for the reset process
	done = FALSE ;
	while (!done)
	{
		// pause for 5 seconds before each sample
		Sleep (5000) ;
		timeout += 5 ;

		// loop through buckets
		for (bkt=startbkt;bkt<=endbkt;bkt++)
		{
			// get the bucket status
			sprintf (cmd, "%dX0", bkt) ;
			send_command (BKT, cmd, resp) ;

			// parse the response
			parse_fesb_response (resp, &rc, &arg) ;

			// build the message based on bucket status
			sprintf (msgstr, "Bucket %2d Status %c %d", bkt, rc, arg) ;
			if (rc == 'C') strcat (msgstr, " Booting") ;
			if (rc == 'T') strcat (msgstr, " Timed Out") ;
			if (rc == 'D') strcat (msgstr, " Running Diagnostics") ;
			if ((rc == 'S') && (arg == 2)) strcat (msgstr, " Reset") ;

			// log if complete... reset timer too
			if ((rc == 'S') && (arg == 0))
			{
				strcat (msgstr, " Reset Complete") ;
				bktdone[bkt] = 1 ;
				timeout = 0 ;
				done = TRUE ;
			}

			// if report is TRUE send the message
			if (report) post_status_update (-1, message, msgstr) ;

			// if any bucket is not at S0 we are not done
			for (i=startbkt;i<=endbkt;i++)
				if (!bktdone[i]) done = FALSE ;
		}

		// check for timeout
		if (timeout >= BKTRESET_TIMEOUT)
		{
			done = FALSE;
			break ;
		}
		if (done) break ;
	}
	return done ;
}

/***********************************************************************************/
//
//
//	Function:	SetWndHandle	- store the window handle to be used for updates
//										  Gantry Settings operation
//
//	Inputs:		HWND handle		- the window handle
//
//	Outputs:	None
//
//	Returns:	TRUE
//
/***********************************************************************************/

BOOL
SetWndHandle (HWND handle)
{
	if (handle)
		hWnd = handle ;
	else
		hWnd = NULL ;

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	post_settings_update	- post an update for a
//										  Gantry Settings operation
//
//	Inputs:		int Op					- settings operation complete code
//				int bkt					- bucket number
//				int blk					- block number
//				char* resp				- response from the settings request
//
//	Outputs:	None
//
//	Returns:	void
//
/***********************************************************************************/

LOCAL void
post_settings_update (int Op, int bkt, int blk, char* resp)
{
	tGantryResult *pgr ;

	if (pgr = NewGantryResult ())
	{
		pgr->OperVal = Op ;
		strcpy(pgr->StatusMsg, resp) ;
		pgr->iBucket = bkt ;
		pgr->iBlock = blk ;
		PostWindowMessage(hWnd, WM_SETTINGUPD, (WPARAM)pgr) ;
	}
}

/***********************************************************************************/
//
//
//	Function:	post_settings_complete	- post a complete for Gantry Settings
//
//	Inputs:		int Op					- settings operation complete code
//				char* message			- message
//
//	Outputs:	None
//
//	Returns:	void
//
/***********************************************************************************/

LOCAL void
post_settings_complete (int Op, char* msg)
{
	tGantryResult *pgr = NewGantryResult () ;

	if (pgr = NewGantryResult ())
	{
		pgr->OperVal = Op ;
		strcpy(pgr->StatusMsg, msg) ;
		PostWindowMessage(hWnd, WM_SETTINGCOMPLETE, (WPARAM)pgr) ;
	}
}

/***********************************************************************************/
//
//
//	Function:	post_status_update	- post a status update to application window
//
//	Inputs:		int progress		- progress percent, -1 if no progress to post
//				int message			- message
//				char* str			- message for window to output
//
//	Outputs:	None
//
//	Returns:	void
//
/***********************************************************************************/

LOCAL void
post_status_update (int progress, int message, char* str)
{	
	// build the window message
	tGantryResult *msg = NewGantryResult () ;
	msg->iProgTime = progress ;
	strcpy (msg->StatusMsg, str) ;

	// send it
	PostWindowMessage (hWnd, message, (WPARAM)msg) ;
}

/***********************************************************************************/
//
//
//	Function:	PostWindowMessage	- post a message to appication window
//
//	Inputs:		HWND handle			- handle to window
//				int message			- message
//				WPARAM wp			- windows parameter structure
//
//	Outputs:	None
//
//	Returns:	TRUE | False if post message fails
//
/***********************************************************************************/

LOCAL BOOL
PostWindowMessage (HWND handle, int message, WPARAM wp)
{
	if (!PostMessage(handle, message, wp ,0))
	{
		ReleaseMsgPointer (wp) ;
		return (FALSE) ;
	}

	return (TRUE) ;
}

/***********************************************************************************/
//
//
//	Function:	tGantryResult	- allocate a structure for UI message update
//
//	Inputs:		None
//
//	Outputs:	None
//
//	Returns:	Pointer to tGantryResult structure
//
/***********************************************************************************/

LOCAL 
tGantryResult* NewGantryResult(void)
{

	tGantryResult* pgr = new tGantryResult ;
	if (!pgr) return (NULL) ;
	return pgr ;
}

/***********************************************************************************/
//
//
//	Function:	ReleaseMsgPointer	- exported routine to release message after the
//									  application is through with the message structure
//
//	Inputs:		None
//
//	Outputs:	None
//
//	Returns:	TRUE
//
/***********************************************************************************/

BOOL
ReleaseMsgPointer (WPARAM wp)
{
	tGantryResult* tMsg = (tGantryResult*) wp ;
	delete tMsg ;
	return (TRUE) ;
}


