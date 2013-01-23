//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Command Line interface to detector abstraction layer (dal.exe)
// 
// Name:			dal.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	May 31, 2002
// 
// Description:		This component provides the interface to the detector abstraction
//					layer and thus the scanner from the command line.
//
// Commands:
//			pp              Put indicated head into position profile mode
//			                dal pp [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			en              Put indicated head into crystal energy mode
//			                dal en [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			te              Put indicated head into tube energy mode
//			                dal te [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			shape           Put indicated head into shape discrimination mode
//			                dal shape [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			run             Put indicated head into run mode
//			                dal run [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			trans           Put indicated head into transmission mode
//			                dal trans [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			spect           Put indicated head into spect mode
//			                dal spect [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			ti              Put indicated head into crystal time mode
//			                dal ti [-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]
//			high_voltage    Turn High Voltage Off/On
//			                dal high_voltage on/off
//			point_source    Turn Point Source Off/On for indicated head
//			                dal point_source on/off [-h head]
//			coinc           Put Coincidence Processor into Coincidence Mode
//			                dal coinc on/off (transmission sources - not HRRT) modulepair (P39 Only) [-g]
//			singles         Retrieve Singles for each block on indicated head
//			                dal singles [-h head]
//			mget            Retrieve analog card settings for indicated head
//			                dal mget [-h head][-conf configuration][-ch block]
//			temp            Retrieve Temperatures
//			                dal temp
//			zap             Reset (Zap) indicated RAM values on indicated head
//			                dal zap all/asic/time/energy [-h head][-conf configuration][-ch block]
//			xy              Determine X, Y (& Energy) Offsets for indicated block(s) on indicated head(s)
//			                dal xy [-h head][-conf configuration][-ch block]
//			cfd_del         Determine CFD Delay for indicated block(s) on indicated head(s)
//			                dal cfd_del [-h head][-conf configuration][-ch block]
//			set             Set indicated analog setting(s) for indicated head
//			                dal set string value [-h head][-conf configuration][-ch block]
//			upload          Upload file from head
//			                dal upload head_filename [-h head][-f override_local_filename]
//			download        Download file to head
//			                dal download local_filename [-h head][-f override_head_filename]
//			load            Load indicated RAM on indicated head
//			                dal load energy/time/asic [-h head][-conf configuration][-fast/-slow][-f download_filename]
//			pass            Execute firmware command directly on head
//			                dal pass "command" [-h head]
//			passthru        Put Coincidence Processor in Pass-Thru Mode for indicated head(s)
//			                dal passthru [-h head]
//			time            Put Coincidence Processor in Time Histogram Mode
//			                dal time on/off (transmission sources) modulepair (P39 Only) [-g]
//			reset           Reset indicated head(s)
//			                dal reset [-h head][-g]
//			dos             Execute operating system command directly on head
//			                dal dos "command" [-h head]
//			force_unlock    Force Server to release lock code
//			                dal force_unlock
//			init_scan       Initialize Detectors and Coincidence Processor to be Ready to start scan
//			                dal init_scan on/off (transmission sources) modulepair (P39 Only) [-g] [-lld <value>] [-uld <value>]
//			start           Indicate to Coincidence Processor to start scan
//			                dal start
//			halt            Indicate to Coincidence Processor to halt scan
//			                dal halt
//			stats           Get Statistics
//			                dal stats
//			progress        Poll For Current Percent Complete
//			                dal progress [-h head]
//			wait            Wait Until Percent Complete is 100%
//			                dal wait [-h head]
//			trajectory      HRRT Transmission Source Trajectory
//			                dal trajectory speed step
//			control         Singles and Time Tagword Insertion Control
//			                dal control singles_tagword_On/Off time_tagword_On/Off
//			set_temp        Set Temperature Limits (All Detector Electronics)
//			                dal set_temp low high
//			voltage         Display DHI Voltages
//			                dal voltage
//			hrrt_voltage    Display High Voltage for DHI on HRRT
//			                dal hrrt_voltage
//			window          Set Coincidence Timing Window
//			                dal window 6ns/10ns/open
//			tag             Put Coincidence Processor in Tagword only mode
//			                dal tag
//			insert          Insert tagword(s) into data stream at coincidence processor
//			                dal insert <hex_tagword> || <-f filename [-a]>
//			check           Report The Mode The Head or Coincidence Processor is in
//			                dal check [-h head]
//			ck_src          Report Whether the Point Source is extended or retracted
//			                dal ck_src [-h head]
//			health          Is Detector System Healthy?
//			                dal health
//			init_pet        Initialize Detectors and Coincidence Processor to be Read to start a PET scan
//			                dal init_pet Em|EmTx|Tx [-lld <value>] [-uld <value>]
//			count           Count Rates
//			                dal count
//			ring            Ring Singles
//			                dal ring
//			kill            Kill Server
//			                dal kill
//			log             Turn Logging On/Off
//			                dal log on/off
//			ping            Retrieve BuildID from indicated head
//			                dal ping [-h head]
//			lock            Lock ACS Servers Returning LockCode
//			                dal lock [-f filename]
//			unlock          Unlock ACS Servers With LockCode Provided
//			                dal unlock LockCode | -f filename
//			voltage_temperature     Hardware Temperatures and Voltages
//			                dal voltage_temperature
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

// dal.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"
#include "iostream.h"
#include "DHI_Constants.h"
#include "DHICom.h"
#include "DHICom_i.c"
#include "LockMgr.h"
#include "LockMgr_i.c"
#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

#include "gantryModelClass.h"
#include "isotopeInfo.h"

#define NUM_ACTIONS 53
#define	OVERRIDE_ANY_LOCKS	0xFF	// For OverrideFlag in LockACS method
#define RPC_FAILED 0x800706BE		// code returned when killing server

CErrorEventSupport* pErrEvtSup;

/////////////////////////////////////////////////////////////////////////////
// Routine:		flagval
// Purpose:		determines whether or not a particular flag was set ont the command line
// Arguments:	search		-	char = flag to search for
//				argc		-	int = number of command line arguments
//				argv		-	char = array of command line arguments
//				buffer		-	char = command line argument following found flag (output)
// Returns:		long argument where flag was found
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long flagval(char* search, int argc, char* argv[], char buffer[MAX_STR_LEN])
{
	// initialize
	long success = 0;

	// clear the buffer
	strcpy(buffer, "");

	// loop through the command line arguments looking for search string
	for (long i = 1 ; (i < argc) && (success == 0) ; i++) {

		// check argument
		if (strcmp(argv[i], search) == 0) {

			// copy the next argument
			if ((i+1) < argc) strcpy(buffer, argv[i+1]);

			// indicate that the argument was found
			success = i;
		}
	}

	// return whether or not the value was found
	return success;
}


/////////////////////////////////////////////////////////////////////////////
// Routine:	hextobin
// Purpose:	convert hexadecimal string used for file upload to byte equivalent
// Arguments:	hex			- char = string of hexadecimal characters (in ascii)
//				data		- unsigned char = binary equivalint
// Returns:		long number of bytes
// History:		15 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long hextobin(char *hex, unsigned char *data)
{
   long i = 0;
   long index = 0;
   long upper = 0;
   long lower = 0;
   long length = 0;
   char digit[MAX_STR_LEN];
   char *hexvals = "0123456789abcdefABCDEF";
   char local[2048];

   // determine number of bytes
   length = strlen(hex) / 2;

   // if an odd number of characters, add a 0 before it to make it even and increase the length by 1
   if ((2 * length) != (long) strlen(hex)) {
	   length++;
	   sprintf(local, "0%s", hex);
   } else {
	   sprintf(local, "%s", hex);
   }

   // convert each hexadecimal pair to byte integer
   for (i = 0; i < length; i++) {

	   // index into input hex string
	   index = 2 * i;

	   // convert the left hand hex digit
	   strncpy(digit, &local[index], 1);
	   digit[1] = 0;
	   upper = strcspn(hexvals, digit);
	   if (upper > 15) upper -= 6;

	   // convert the right hand hex digit
	   strncpy(digit, &local[index+1], 1);
	   digit[1] = 0;
	   lower = strcspn(hexvals, digit);
	   if (lower > 15) lower -= 6;

	   // combine the two into one output byte
	   data[i] = (unsigned char) ((upper << 4) | lower);
   }

   return length;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:	Add_Error
// Purpose:	send an error message to the event handler
//			add it to the log file and store it for recall
// Arguments:	What		-	char = what was the error
//				Why			-	long = pertinent integer information
// Returns:		long status
// History:		17 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Add_Error(char *What, long Why)
{
	// local variable
	long RawDataSize = 0;

	unsigned char GroupName[10] = "ARS_DAL";
	unsigned char Message[MAX_STR_LEN];
	unsigned char EmptyStr[10] = "";

	HRESULT hr = S_OK;

	// build the message
	sprintf((char *) Message, "Command Line: %s = %d (decimal) %X (hexadecimal)", What, Why, Why);

	// print error
	cout << Message << endl;

	// fire message to event handler
	if (pErrEvtSup != NULL) {
		hr = pErrEvtSup->Fire(GroupName, DAL_GENERIC_DEVELOPER_ERR, Message, RawDataSize, EmptyStr, TRUE);
		if (FAILED(hr)) {
			sprintf((char *) Message, "Error Firing Event to Event Handler.  HRESULT = %X", hr);
			cout << Message << endl;
		}
	}

	// return error code
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_DAL_Ptr
// Purpose:		retrieve a pointer to the Detector Abstraction Layer COM server (DHICOM)
// Arguments:	none
// Returns:		IAbstract_DHI* pointer to DHICOM
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

IAbstract_DHI* Get_DAL_Ptr()
{
	// local variables
	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	IAbstract_DHI *Ptr = NULL;
	
	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// get server
	if (getenv("DHICOM_COMPUTER") == NULL) {
		sprintf(Server, "P39ACS");
	} else {
		sprintf(Server, "%s", getenv("DHICOM_COMPUTER"));
	}
	for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
	si.pwszName = Server_Name;

	// identify interface
	iid = IID_IAbstract_DHI;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	// get instance
	hr = ::CoCreateInstanceEx(CLSID_Abstract_DHI, NULL, CLSCTX_SERVER, &si, 1, mqi);
	if (FAILED(hr)) Add_Error("Failed ::CoCreateInstanceEx(CLSID_Abstract_DHI, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (IAbstract_DHI*) mqi[0].pItf;

	return Ptr;
}


/////////////////////////////////////////////////////////////////////////////
// Routine:		Get_Lock_Ptr
// Purpose:		retrieve a pointer to the Lock Manager COM server (LockMgr)
// Arguments:	none
// Returns:		ILockClass* pointer to LockMgr
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

ILockClass* Get_Lock_Ptr()
{
	// local variables
	long i = 0;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	ILockClass *Ptr = NULL;
	
	HRESULT hr = S_OK;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// get server
	if (getenv("LOCKMGR_COMPUTER") == NULL) {
		sprintf(Server, "P39ACS");
	} else {
		sprintf(Server, "%s", getenv("LOCKMGR_COMPUTER"));
	}
	for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
	si.pwszName = Server_Name;

	// identify interface
	iid = IID_ILockClass;
	mqi[0].pIID = &iid;
	mqi[0].pItf = NULL;
	mqi[0].hr = 0;

	// attach to the lock server
	hr = ::CoCreateInstanceEx(CLSID_LockClass, NULL, CLSCTX_SERVER, &si, 1, mqi);
	if (FAILED(hr)) Add_Error("Failed ::CoCreateInstanceEx(CLSID_LockClass, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
	if (SUCCEEDED(hr)) Ptr = (ILockClass*) mqi[0].pItf;

	return Ptr;
}

void main(int argc, char* argv[])
{
	// settings information
	long Singles[MAX_BLOCKS];
	long Settings[MAX_BLOCKS * MAX_SETTINGS];

	// local variables
	byte Force = 0;

	long CorrectedSingles = 0;
	long UncorrectedSingles = 0;
	long Prompts = 0;
	long Randoms = 0;
	long Scatters = 0;
	long NumFailed = 0;
	long NextOut = 0;
	long NumRings = 0;
	long Next = 0;
	long i = 0;
	long j = 0;
	long Status = 0;
	long num_action = -1;
	long head = 0;
	long layer = LAYER_ALL;
	long channel = -1;
	long singles_type = 1;
	long config = 0;
	long lld = 1;
	long uld = 850;
	long ascii = 0;
	long Block = 0;
	long col = 0;
	long row = 0;
	long Locked = 0;
	long ModulePair = 0;
	long Percent = 0;
	long speed = 0;
	long memvalid = 0;
	long step = 0;
	long Filesize = 0;
	long ok = 1;
	long wait = 1;
	long OffOn = 0;
	long OnOff = 0;
	long code = 0;
	long mode = 0;
	long index = 0;
	long bytes = 0;
	long Arguement = 0;
	long HeadSet = 0;
	long Head = 0;
	long block = 0;
	long first_block = 0;
	long last_block = 0;
	long Reset = 0;
	long ScanType = -1;
	long HeadCount = 0;
	long LockBypass = 0;
	long NumTemps = 0;
	long ResetWait = 0;
	long print_help = 0;
	long HighVoltage[80];
	long NeedsLock[NUM_ACTIONS] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
								   1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 
								   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
								   0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 
								   1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 
								   1, 0, 0};
	long SetOrder[MAX_SETTINGS];
	long PrintOrder[MAX_SETTINGS];
	long Stats[19];
	long *Failed;
	long *RingSingles;
	long HeadSelect[MAX_HEADS];
	long SourceSelect[MAX_HEADS];
	unsigned long CheckSum = 0;
	unsigned long check = 0;
	ULONGLONG *Tagword = NULL;

	__int64 LockCode = -1;

	float low = 0.0;
	float high = 100.0;
	float cptemp = 0.0;
	float Temperature[35];
	float Voltage[64];

	FILE *fp = NULL;

	char *ptr;
	char buffer[MAX_STR_LEN];
	char filename[MAX_STR_LEN] = "display";
	char *Action[NUM_ACTIONS] = {"pp", "en", "te", "shape", "run", "trans", "spect", "ti", "high_voltage", "point_source",
								 "coinc", "singles", "mget", "temp", "zap", "xy", "cfd_del", "set", "upload", "download", 
								 "load", "pass", "passthru", "time", "reset", "dos", "force_unlock", "init_scan", "start", "halt", 
								 "stats", "progress", "wait", "trajectory", "control", "set_temp", "voltage", "hrrt_voltage", "window", "tag", 
								 "insert", "check", "ck_src", "health", "init_pet", "count", "ring", "kill", "log", "ping",
								 "lock", "unlock", "voltage_temperature"};
	char *Description[NUM_ACTIONS] = {"\t\tPut indicated head into position profile mode", 
									  "\t\tPut indicated head into crystal energy mode",
									  "\t\tPut indicated head into tube energy mode",
									  "\t\tPut indicated head into shape discrimination mode",
									  "\t\tPut indicated head into run mode",
									  "\t\tPut indicated head into transmission mode",
									  "\t\tPut indicated head into spect mode",
									  "\t\tPut indicated head into crystal time mode",
									  "\tTurn High Voltage Off/On",
									  "\tTurn Point Source Off/On for indicated head", 
									  "\t\tPut Coincidence Processor into Coincidence Mode",
									  "\t\tRetrieve Singles for each block on indicated head",
									  "\t\tRetrieve analog card settings for indicated head", 
									  "\t\tRetrieve Temperatures", 
									  "\t\tReset (Zap) indicated RAM values on indicated head",
									  "\t\tDetermine X, Y (& Energy) Offsets for indicated block(s) on indicated head(s)", 
									  "\t\tDetermine CFD Delay for indicated block(s) on indicated head(s)", 
									  "\t\tSet indicated analog setting(s) for indicated head", 
									  "\t\tUpload file from head", 
									  "\tDownload file to head",
									  "\t\tLoad indicated RAM on indicated head", 
									  "\t\tExecute firmware command directly on head",
									  "\tPut Coincidence Processor in Pass-Thru Mode for indicated head(s)", 
									  "\t\tPut Coincidence Processor in Time Histogram Mode",
									  "\t\tReset indicated head(s)",
									  "\t\tExecute operating system command directly on head",
									  "\tForce Server to release lock code",
									  "\tInitialize Detectors and Coincidence Processor to be Ready to start scan",
									  "\t\tIndicate to Coincidence Processor to start scan",
									  "\t\tIndicate to Coincidence Processor to halt scan",
									  "\t\tGet Statistics",
									  "\tPoll For Current Percent Complete", 
									  "\t\tWait Until Percent Complete is 100%", 
									  "\tHRRT Transmission Source Trajectory", 
									  "\t\tSingles and Time Tagword Insertion Control", 
									  "\tSet Temperature Limits (All Detector Electronics)", 
									  "\t\tDisplay DHI Voltages", 
									  "\tDisplay High Voltage for DHI on HRRT", 
									  "\t\tSet Coincidence Timing Window", 
									  "\t\tPut Coincidence Processor in Tagword only mode", 
									  "\t\tInsert tagword(s) into data stream at coincidence processor", 
									  "\t\tReport The Mode The Head or Coincidence Processor is in", 
									  "\t\tReport Whether the Point Source is extended or retracted", 
									  "\t\tIs Detector System Healthy?", 
									  "\tInitialize Detectors and Coincidence Processor to be Read to start a PET scan", 
									  "\t\tCount Rates",
									  "\t\tRing Singles",
									  "\t\tKill Server", 
									  "\t\tTurn Logging On/Off",
									  "\t\tRetrieve BuildID from indicated head",
									  "\t\tLock ACS Servers Returning LockCode",
									  "\t\tUnlock ACS Servers With LockCode Provided",
									  "\tHardware Temperatures and Voltages"};
	char *format[NUM_ACTIONS] = {"[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "[-fast/-slow][-lld value][-uld value][-h head][-conf configuration][-ch block][-g]", 
								 "on/off",
								 "on/off [-h head]",
								 "on/off (transmission sources - not HRRT) modulepair (P39 Only) [-g]",
								 "[-h head]",
								 "[-h head][-conf configuration][-ch block]",
								 "",
								 "all/asic/time/energy [-h head][-conf configuration][-ch block]",
								 "[-h head][-conf configuration][-ch block]",
								 "[-h head][-conf configuration][-ch block]",
								 "string value [-h head][-conf configuration][-ch block]", 
								 "head_filename [-h head][-f override_local_filename]",
								 "local_filename [-h head][-f override_head_filename]",
								 "energy/time/asic [-h head][-conf configuration][-fast/-slow][-f download_filename]",
								 "\"command\" [-h head]",
								 "[-h head]",
								 "on/off (transmission sources) modulepair (P39 Only) [-g]",
								 "[-h head][-g]", 
								 "\"command\" [-h head]",
								 "",
								 "on/off (transmission sources) modulepair (P39 Only) [-g] [-lld <value>] [-uld <value>]",
								 "",
								 "",
								 "",
								 "[-h head]",
								 "[-h head]", 
								 "speed step",
								 "singles_tagword_On/Off time_tagword_On/Off",
								 "low high",
								 "",
								 "",
								 "6ns/10ns/open",
								 "", 
								 "<hex_tagword> || <-f filename [-a]>", 
								 "[-h head]",
								 "[-h head]",
								 "", 
								 "Em|EmTx|Tx [-lld <value>] [-uld <value>]",
								 "",
								 "",
								 "",
								 "on/off",
								 "[-h head]",
								 "[-f filename]",
								 "LockCode | -f filename",
								 ""};
	char *ZapMode[NUM_ZAP_TYPES] = {"all", "asic", "time", "energy"};
	char *LoadStr[NUM_RAM_TYPES] = {"energy", "time", "asic"};
	char *type_str[NUM_RAM_TYPES] = {"pk", "tm", "analog"};
	char *SetStr[MAX_SETTINGS] = {"pmta", "pmtb", "pmtc", "pmtd", "cfd", "cfd_del", "x_offset", "y_offset", "e_offset", 
									"mode", "setup_eng", "sd", "slow_low", "slow_high", "fast_low", "fast_high", 
									"cfd_setup", "tdc_gain", "tdc_offset", "tdc_setup"};
	char *ResetStr[6] = {"reboot", "master", "prompts", "randoms", "fifo", "glink"};
	char *CPStr[NUM_CP_MODES] = {"COINCIDENCE", "PASS-THRU", "TAGWORD", "TEST PATTERN", "TIME HISTOGRAM"};
	char *DHIModeStr[NUM_DHI_MODES] = {"Position Profile", "Crystal Energy", "Tube Energy", "Shape", "Run", "Transmission", "SPECT", "Crystal Time"};
	char *LayerStr[NUM_LAYER_TYPES] = {"fast", "slow", "all"};
	char *ScanTypes[3] = {"em", "emtx", "tx"};
	char str[MAX_STR_LEN];
	char Str[MAX_STR_LEN];

	unsigned char HeadFile[MAX_STR_LEN];
	unsigned char command[MAX_STR_LEN];
	unsigned char response[MAX_STR_LEN];
	unsigned char ErrorString[MAX_STR_LEN];
	unsigned char data[MAX_STR_LEN];
	unsigned char *filebuf = NULL;

	HRESULT hr = S_OK;

	IAbstract_DHI* pDHI = NULL;
	ILockClass* pLock = NULL;

	// Initialize system parameters
	long NumHeads = 6;
	long NumSettings = 14;
	long NumBlocks = 84;
	long EmissionX = 10;
	long EmissionY = 7;
	long ScannerType = SCANNER_P39;

	// initialize head arrays
	long HeadPresent[MAX_HEADS];
	long PointSource[MAX_HEADS];

	// instantiate a Gantry Object Model object
	CGantryModel model;
	pHead HeadData;

	// structures
	HeadInfo *Info;
	PhysicalInfo *HW_Info;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// translate all arguments to lower case
	for (i = 1 ; i < argc ; i++) _strlwr(argv[i]);

	// Initialize COM
	hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		cout << "Failed ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)" << endl;
		exit(1);
	}

	// Initialize security
	hr = CoInitializeSecurity(NULL,		// Allow access to everyone
								-1,		// Use default authentication service
							  NULL,		// Use default authorization service
							  NULL,		// Reserved
			RPC_C_AUTHN_LEVEL_NONE,		// Authentication Level=None
	   RPC_C_IMP_LEVEL_IMPERSONATE,		// Impersonation Level= Impersonate
							  NULL,		// Reserved
						 EOAC_NONE,		// Capabilities Flag
							  NULL);	// Reserved
	if (FAILED(hr)) {
		cout << "Failed CoInitializeSecurity" << endl;
		exit(1);
	}

	// instantiate an ErrorEventSupport object
	pErrEvtSup = new CErrorEventSupport(false, false);

	// if error support not establisted, note it in log
	if (pErrEvtSup == NULL) {
		cout << "Failed to Create ErrorEventSupport" << endl;

	// if it was not fully istantiated, note it in log and release it
	} else if (pErrEvtSup->InitComplete(hr) == false) {
		delete pErrEvtSup;
		pErrEvtSup = NULL;
	}

	// set gantry model to model number held in register if none held, use the 393
	if (!model.setModel(DEFAULT_SYSTEM_MODEL)) if (!model.setModel(393)) exit(1);

	// set scanner type from gantry model
	switch (model.modelNumber()) {

	// PETSPECT
	case 311:
		ScannerType = SCANNER_PETSPECT;
		break;

	// HRRT
	case 328:
		ScannerType = SCANNER_HRRT;
		break;

	// P39 family (391 - 396)
	case 391:
	case 392:
	case 393:
	case 394:
	case 395:
	case 396:
		ScannerType = SCANNER_P39;
		break;

	// P39 Phase IIA Electronics family (2391 - 2396)
	case 2391:
	case 2392:
	case 2393:
	case 2394:
	case 2395:
	case 2396:
		ScannerType = SCANNER_P39_IIA;
		break;

	// not a panel detector - return error
	default:
		Add_Error("Unknown Scanner, Model", model.modelNumber());
		exit(1);
		break;
	}

	// number of blocks
	NumBlocks = model.blocks();
	if (model.isHeadRotated()) {
		EmissionX = model.axialBlocks();
		EmissionY = model.transBlocks();
	} else {
		EmissionX = model.transBlocks();
		EmissionY = model.axialBlocks();
	}

	// length of time to wait after head reset
	ResetWait = model.resetWaitDuration() * 1000;

	// number of settings
	NumSettings = model.nAnalogSettings();
	for (i = 0 ; i < MAX_SETTINGS ; i++) SetOrder[i] = -1;
	SetOrder[PMTA] = model.pmtaSetting();
	SetOrder[PMTB] = model.pmtbSetting();
	SetOrder[PMTC] = model.pmtcSetting();
	SetOrder[PMTD] = model.pmtdSetting();
	SetOrder[CFD] = model.cfdSetting();
	SetOrder[CFD_DELAY] = model.cfdDelaySetting();
	SetOrder[X_OFFSET] = model.xOffsetSetting();
	SetOrder[Y_OFFSET] = model.yOffsetSetting();
	SetOrder[E_OFFSET] = model.energyOffsetSetting();
	SetOrder[DHI_MODE] = model.dhiModeSetting();
	SetOrder[SETUP_ENG] = model.setupEnergySetting();
	SetOrder[SHAPE] = model.shapeThresholdSetting();
	SetOrder[SLOW_LOW] = model.slowLowSetting();
	SetOrder[SLOW_HIGH] = model.slowHighSetting();
	SetOrder[FAST_LOW] = model.fastLowSetting();
	SetOrder[FAST_HIGH] = model.fastHighSetting();
	SetOrder[CFD_SETUP] = model.cfdSetupSetting();
	SetOrder[TDC_GAIN] = model.tdcGainSetting();
	SetOrder[TDC_OFFSET] = model.tdcOffsetSetting();
	SetOrder[TDC_SETUP] = model.tdcSetupSetting();

	// output order of settings (initialized to memory order)
	for (i = 0 ; i < NumSettings ; i++) PrintOrder[i] = i;
	if (SetOrder[PMTA] != -1) PrintOrder[NextOut++] = SetOrder[PMTA];
	if (SetOrder[PMTB] != -1) PrintOrder[NextOut++] = SetOrder[PMTB];
	if (SetOrder[PMTC] != -1) PrintOrder[NextOut++] = SetOrder[PMTC];
	if (SetOrder[PMTD] != -1) PrintOrder[NextOut++] = SetOrder[PMTD];
	if (SetOrder[CFD] != -1) PrintOrder[NextOut++] = SetOrder[CFD];
	if (SetOrder[CFD_DELAY] != -1) PrintOrder[NextOut++] = SetOrder[CFD_DELAY];
	if (SetOrder[X_OFFSET] != -1) PrintOrder[NextOut++] = SetOrder[X_OFFSET];
	if (SetOrder[Y_OFFSET] != -1) PrintOrder[NextOut++] = SetOrder[Y_OFFSET];
	if (SetOrder[E_OFFSET] != -1) PrintOrder[NextOut++] = SetOrder[E_OFFSET];
	if (SetOrder[SHAPE] != -1) PrintOrder[NextOut++] = SetOrder[SHAPE];
	if (SetOrder[DHI_MODE] != -1) PrintOrder[NextOut++] = SetOrder[DHI_MODE];
	if (SetOrder[CFD_SETUP] != -1) PrintOrder[NextOut++] = SetOrder[CFD_SETUP];
	if (SetOrder[TDC_GAIN] != -1) PrintOrder[NextOut++] = SetOrder[TDC_GAIN];
	if (SetOrder[TDC_OFFSET] != -1) PrintOrder[NextOut++] = SetOrder[TDC_OFFSET];
	if (SetOrder[TDC_SETUP] != -1) PrintOrder[NextOut++] = SetOrder[TDC_SETUP];
	if (SetOrder[SETUP_ENG] != -1) PrintOrder[NextOut++] = SetOrder[SETUP_ENG];
	if (SetOrder[SLOW_LOW] != -1) PrintOrder[NextOut++] = SetOrder[SLOW_LOW];
	if (SetOrder[SLOW_HIGH] != -1) PrintOrder[NextOut++] = SetOrder[SLOW_HIGH];
	if (SetOrder[FAST_LOW] != -1) PrintOrder[NextOut++] = SetOrder[FAST_LOW];
	if (SetOrder[FAST_HIGH] != -1) PrintOrder[NextOut++] = SetOrder[FAST_HIGH];

	// initialize head arrays
	for (i = 0 ; i < MAX_HEADS ; i++) {
		HeadPresent[i] = 0;
		PointSource[i] = 0;
		HeadSelect[i] = 0;
		SourceSelect[i] = 0;
	}

	// get the data for the next head
	HeadData = model.headInfo();

	// set head values
	for (NumHeads = 0, i = 0 ; i < model.nHeads() ; i++) {

		// keeping track of the highest head number
		if (HeadData[i].address >= NumHeads) NumHeads = HeadData[i].address + 1;

		// indicate that the head is present
		HeadPresent[HeadData[i].address] = 1;

		// indicate if transmission sources are hooked up to this head
		if (HeadData[i].pointSrcData) PointSource[HeadData[i].address] = 1;
	}

	// if command not present, then display help
	if (argc == 1) print_help = 1;
	if ((argc == 2) && (strcmp(argv[1], "help") == 0)) print_help = 1;
	if (flagval("-help", argc, argv, buffer) > 0) print_help = 1;
	if (print_help == 1) {
		for (i = 0 ; i < NUM_ACTIONS ; i++) {
			cout << Action[i] << Description[i] << endl;
			cout << "\t\t" << argv[0] << " " << Action[i] << " " << format[i] << endl;
		}
		exit(0);
	}

	// process flags
	if (flagval("-a",argc,argv,buffer) > 0) ascii = 1; 
	if (flagval("-f",argc,argv,buffer) > 0) strcpy(filename,buffer);
	if (flagval("-g",argc,argv,buffer) > 0) wait = 0;
	if (flagval("-c",argc,argv,buffer) > 0) singles_type = 0;
	if (flagval("-l",argc,argv,buffer) > 0) LockBypass = 1;
	if (flagval("-ch",argc,argv,buffer) > 0) channel = atol(buffer);
	if (flagval("-lld",argc,argv,buffer) > 0) lld = atoi(buffer);
	if (flagval("-uld",argc,argv,buffer) > 0) uld = atoi(buffer);
	if (flagval("-conf",argc,argv,buffer) > 0) config = atoi(buffer);
	if (flagval("-fast",argc,argv,buffer) > 0) layer = LAYER_FAST;
	if (flagval("-slow",argc,argv,buffer) > 0) layer = LAYER_SLOW;

	// validate flags
	if ((ScannerType == SCANNER_P39_IIA) && (layer != LAYER_ALL)) {
		Add_Error("Invalid Layer, Layer", layer);
		exit(1);
	}

	// determine action code
	for (i = 0 ; (i < NUM_ACTIONS) && (num_action < 0) ; i++) {
		if (strcmp(Action[i], argv[1]) == 0) num_action = i;
	}

	// unrecognized command
	if (num_action == -1) {
		Add_Error("Command Not Recognized, Action Number", num_action);
		exit(1);
	}

	// when switching to pass-thru mode, we can do multiple heads and multiple point sources
	if (num_action == 22) {

		// process any head selections
		while (((Arguement = flagval("-h",argc,argv,buffer)) > 0) && (Status == 0)) {

			// set flag indicating that the flag has been specifically set by the user
			HeadSet = 1;

			// remove that arguement from the rest of the list
			sprintf(argv[Arguement], "-x");

			// assign head
			Head = atol(buffer);

			// if all heads, then switch on by scanner
			// also, Rotation is not valid for PETSPECT & HRRT so make sure it is off
			if (Head == -1) {
				for (i = 0 ; i < MAX_HEADS ; i++) {
					HeadSelect[i] = HeadPresent[i];
				}

			// otherwise, just the one head
			} else {

				if ((Head < 0) || (Head >= MAX_HEADS)) {
					Status = Add_Error("Invalid Argument (Out of Range), Head", Head);
				} else if (HeadPresent[Head] != 1) {
					Status = Add_Error("Invalid Argument (Not Present), Head", Head);
				} else {
					HeadSelect[Head] = 1;
				}
			}
		}

		// process any point source selections
		while (((Arguement = flagval("-p",argc,argv,buffer)) > 0) && (Status == 0)) {

			// set flag indicating that the flag has been specifically set by the user
			HeadSet = 1;

			// remove that arguement from the rest of the list
			sprintf(argv[Arguement], "-x");

			// assign head
			Head = atol(buffer);

			// if all heads, then switch on by scanner
			// also, Rotation is not valid for PETSPECT & HRRT so make sure it is off
			if (Head == -1) {
				for (i = 0 ; i < MAX_HEADS ; i++) {
					SourceSelect[i] = PointSource[i];
				}

			// otherwise, just the one head
			} else {

				if ((Head < 0) || (Head >= MAX_HEADS)) {
					Status = Add_Error("Invalid Argument (Out of Range), Head For PointSource", Head);
				} else if (PointSource[Head] == 0) {
					Status = Add_Error("Invalid Argument (Not Present), Point Source on Head", Head);
				} else {
					SourceSelect[Head] = 1;
				}
			}
		}

	// other modes, just translate the one head
	} else {
		if (flagval("-h",argc,argv,buffer) > 0) head = atol(buffer);
	}

	// Create an object.
	if (SUCCEEDED(hr)) {
		if ((strcmp(argv[1], "lock") != 0) && (strcmp(argv[1], "unlock") != 0) && (strcmp(argv[1], "force_unlock") != 0)) {

			// get the DAL pointer
			pDHI = Get_DAL_Ptr();
			if (pDHI == NULL) Status = Add_Error("Failed To Get DAL Pointer", 0);
		}
	}

	// if successful, continue
	if ((Status == 0) && (SUCCEEDED(hr))) {

		// force_unlock
		if (num_action == 26) Force = OVERRIDE_ANY_LOCKS;

		// Lock if needed
		if ((Status == 0) && (NeedsLock[num_action] == 1) && (LockBypass == 0)) {

			// get lock pointer
			pLock = Get_Lock_Ptr();

			// if successful in getting pointer
			if (pLock != NULL) {

				// lock servers
				hr = pLock->LockACS(&LockCode, Force);
				if (FAILED(hr)) {
					Status = Add_Error("Method Failed (pLock->LockACS), HRESULT", hr);
					pLock->Release();
				} else if (hr == S_FALSE) {
					Status = Add_Error("ACS Resources Already Locked, HRESULT", hr);
					pLock->Release();
				} else {
					Locked = 1;
				}
			} else {
				Status = -1;
			}
		}

		// if file indicated then open it (for singles and mget)
		if ((Status == 0) && (strcmp(filename, "display") != 0) && 
			((strcmp(argv[1], "singles") == 0) || (strcmp(argv[1], "mget") == 0))) {
			if ((fp = fopen(filename, "w")) == NULL) {
				sprintf(Str, "Could Not Open File: %s", filename);
				Add_Error(Str, 0);
			}
		}

		// if successful so far
		if (Status == 0) {

			// act on command 
			switch (num_action) {

			// mode change
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:

				// issue command to server
				hr = pDHI->Set_Head_Mode(num_action, config, head, layer, channel, lld, uld, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Set_Head_Mode), HRESULT", hr);

				// poll for progress
				while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
					hr = pDHI->Progress(head, &Percent, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
					if (SUCCEEDED(hr) && (Status == 0)) {
						cout << "\rProgress: " << Percent << "%" << flush;
						if (Percent == 100) {
							cout << endl;
						} else {
							Sleep(1000);
						}
					}
				}

				// if successful, print the mode
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Mode: " << argv[1] << endl;
				break;

			// high voltage switch
			case 8:

				// must have at least 3 arguments
				if (argc >= 3) {

					// determine switch state
					if (strcmp(argv[2], "on") == 0) {
						OffOn = 1;
					} else if (strcmp(argv[2], "off") == 0) {
						OffOn = 0;
					} else {
						sprintf(Str, "Unrecognized Keyword: %s - Turning High Voltage Off", argv[2]);
						OffOn = 0;
						Add_Error(Str, OffOn);
					}

					// issue command to server
					hr = pDHI->High_Voltage(OffOn, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->High_Voltage), HRESULT", hr);

					// if successful, print the action
					if (SUCCEEDED(hr) && (Status == 0)) cout << "High Voltage: " << OffOn << endl;

				// otherwise, error message
				} else {
					cout << argv[0] << " high_voltage Off/On" << endl;
				}
				break;

			// point source switch
			case 9:

				// must have at least 3 arguments
				if (argc >= 3) {

					// determine switch state
					if (strcmp(argv[2], "on") == 0) {
						OffOn = 1;
					} else if (strcmp(argv[2], "off") == 0) {
						OffOn = 0;
					} else {
						sprintf(Str, "Unrecognized Keyword: %s - Turning Point Source Off", argv[2]);
						OffOn = 0;
						Add_Error(Str, OffOn);
					}

					// issue command to server
					hr = pDHI->Point_Source(head, OffOn, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Point_Source), HRESULT", hr);

					// if successful, print the action
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Point Source: " << OffOn << endl;

				// otherwise, error message
				} else {
					cout << argv[0] << " point_source Off/On" << endl;
				}
				break;

			// Coincidence Mode
			case 10:

				// P39 and PETSPECT require on/off for transmission source
				if ((ScannerType == SCANNER_P39) || (ScannerType == SCANNER_P39_IIA) || (ScannerType == SCANNER_PETSPECT)) {

					// check for first argument
					if (argc >= 3) {

						// determine switch state
						if (strcmp(argv[2], "on") == 0) {
							OffOn = 1;
						} else if (strcmp(argv[2], "off") == 0) {
							OffOn = 0;
						} else {
							sprintf(Str, "Transmission Must Be Off/On - Received %s", argv[2]);
							Status = Add_Error(Str, 0);
							ok = 0;
						}

					// improper arguments
					} else {
						cout << argv[0] << " " << argv[1] << " off/on" << endl;
						ok = 0;
					}
				}

				// P39 must have module pair
				if ((ok == 1) && ((ScannerType == SCANNER_P39) || (ScannerType == SCANNER_P39_IIA))) {

					// check for second argument
					if (argc >= 4) {

						// extract module pair
						bytes = hextobin(argv[3], data);
						for (i = 0 ; i < bytes ; i++) ModulePair += ((long) data[i]) << (8 * ((bytes-1) - i));

					// improper arguments
					} else {
						cout << argv[0] << " " << argv[1] << " " << argv[2] << " <ModulePair>" << endl;
						ok = 0;
					}
				}

				// if all arguments were correct
				if (ok == 1) {
					hr = pDHI->Coincidence_Mode(OffOn, ModulePair, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Coincidence_Mode), HRESULT", hr);
				}

				// poll for progress
				while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
					hr = pDHI->Progress(64, &Percent, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
					if (SUCCEEDED(hr) && (Status == 0)) {
						cout << "\rProgress: " << Percent << "%" << flush;
						if (Percent == 100) {
							cout << endl;
						} else {
							Sleep(1000);
						}
					}
				}


				// if successful, print the action
				if (SUCCEEDED(hr) && (Status == 0) && (ok == 1)) cout << "Coincidence Processor: Coincidence" << endl;
				break;

			// Singles
			case 11:

				// issue command to server
				hr = pDHI->Singles(head, singles_type, Singles, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Singles), HRESULT", hr);

				// if successful, print the singles
				if (SUCCEEDED(hr) && (Status == 0)) {

					// for each block on the row, print the block singles separated by tabs
					for (Block = 0 ; Block < NumBlocks ; Block++) {

						// break out row/col
						row = Block / EmissionX;
						col = Block % EmissionX;

						// start of row, print row number
						if (col == 0) {
							cout << row;
							if (fp != NULL) fprintf(fp, "%d", row);
						}

						// print singles
						cout << "\t" << Singles[Block];
						if (fp != NULL) fprintf(fp, "\t%d", Singles[Block]);

						// end of row, print newline
						if ((col == (EmissionX-1)) || (Block == (NumBlocks-1))) {
							cout << endl;
							if (fp != NULL) fprintf(fp, "\n");
						}
					}
				}
				break;

			// mget (analog settings)
			case 12:

				// issue command to server
				hr = pDHI->Get_Analog_Settings(config, head, Settings, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Get_Analog_Settings), HRESULT", hr);

				// if successful, print the settings
				if (SUCCEEDED(hr) && (Status == 0)) {

					// range of blocks
					first_block = (channel == -1) ? 0 : channel;
					last_block = (channel == -1) ? (NumBlocks-1) : channel;

					// loop through specified blocks
					for (block = first_block ; block <= last_block ; block++) {

						// block number
						cout << block;
						if (fp != NULL) fprintf(fp, "%d", block);

						// each setting separated by tabs
						for (index = 0 ; index < NumSettings ; index++) {
							cout << "\t" << Settings[(block*NumSettings)+PrintOrder[index]];
							if (fp != NULL) fprintf(fp, "\t%d", Settings[(block*NumSettings)+PrintOrder[index]]);
						}

						// carriage return at end of line
						cout << endl;
						if (fp != NULL) fprintf(fp, "\n");
					}
				}
				break;

			// temp (report temperatures)
			case 13:

				// issue command to server
				hr = pDHI->Report_Temperature(Temperature, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Report_Temperature), HRESULT", hr);

				// if successful, print temperatures
				if (SUCCEEDED(hr) && (Status == 0)) {
					cout << "Acceptable Range: " << Temperature[1] << " <--> " << Temperature[0] << endl;
					cout << "Coincidence Processor: " << Temperature[2] << endl;
					for (i = 0 ; i < MAX_HEADS ; i++) if (HeadPresent[i] != 0) {
						cout << "Head " << i << " DHI: " << Temperature[(i*2)+3] << "\tLTI: " << Temperature[(i*2)+4] << endl;
					}
				}
				break;

			// zap (reset to defaults)
			case 14:

				// must have at least 3 arguments
				if (argc >= 3) { 

					// determine mode
					for (mode = -1, index = 0 ; (index < NUM_ZAP_TYPES) && (mode == -1) ; index++) {
						if (strcmp(ZapMode[index], argv[2]) == 0) mode = index;
					}

					// if mode was recognized
					if (mode != -1) {

						// issue command to server
						hr = pDHI->Zap(mode, config, head, channel, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Zap), HRESULT", hr);

						// if successful, print temperatures
						if (SUCCEEDED(hr) && (Status == 0)) cout << "Zap Successful" << endl;

					// unrecognized argument
					} else {
						cout << argv[0] << " zap " << argv[2] << " unknown" << endl;
					}
				} else {
					cout << argv[0] << " zap RAM_Type" << endl;
				}
				break;

			// determine xy & energy offsets
			case 15:

				// issue command to server
				hr = pDHI->Determine_Offsets(config, head, channel, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Determine_Offsets), HRESULT", hr);

				// if successful, print it
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Offsets Determined" << endl;
				break;

			// determine cfd delay
			case 16:

				// issue command to server
				hr = pDHI->Determine_Delay(config, head, channel, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Determine_Delay), HRESULT", hr);

				// if successful, print it
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Delays Determined" << endl;
				break;

			// set analog value
			case 17:

				// must have at least 4 arguments
				if (argc >= 4) {

					// determine which setting is to be set
					for (code = -1, i = 0 ; (i < (MAX_SETTINGS+2)) && (code == -1) ; i++) {
						if (SetOrder[i] >= 0) {
							if (strcmp(SetStr[i], argv[2]) == 0) code = SetOrder[i];
						}
					}

					// check for group settings (if the code has not been found yet
					if ((code == -1) && (strcmp("pmt*", argv[2]) == 0)) code = MAX_SETTINGS;
					if ((code == -1) && (strcmp("gain", argv[2]) == 0)) code = MAX_SETTINGS + 1;

					// if it is a valid code for this scanner
					if (code >= 0) {

						// fetch the analog settings
						hr = pDHI->Get_Analog_Settings(config, head, Settings, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Get_Analog_Settings), HRESULT", hr);

						// if successful in fetching the settings
						if (SUCCEEDED(hr) && (Status == 0)) {

							// range of blocks
							first_block = (channel == -1) ? 0 : channel;
							last_block = (channel == -1) ? (NumBlocks-1) : channel;

							// loop through specified blocks
							for (block = first_block ; block <= last_block ; block++) {

								// single value
								if (code < MAX_SETTINGS) {
									Settings[(block*NumSettings)+code] = atol(argv[3]);

								// set all pmts to same value
								} else if (code == MAX_SETTINGS) {
									Settings[(block*NumSettings)+PMTA] = atol(argv[3]);
									Settings[(block*NumSettings)+PMTB] = atol(argv[3]);
									Settings[(block*NumSettings)+PMTC] = atol(argv[3]);
									Settings[(block*NumSettings)+PMTD] = atol(argv[3]);

								// set all pmts to separate values
								} else {
									if (argc >= 7) {
										Settings[(block*NumSettings)+PMTA] = atol(argv[3]);
										Settings[(block*NumSettings)+PMTB] = atol(argv[4]);
										Settings[(block*NumSettings)+PMTC] = atol(argv[5]);
										Settings[(block*NumSettings)+PMTD] = atol(argv[6]);
									} else {
										cout << argv[0] << " set gain value1 value2 value3 value4" << endl;
									}
								}
							}

							// send the settings back to the head
							hr = pDHI->Set_Analog_Settings(config, head, Settings, &Status);
							if (FAILED(hr)) Add_Error("Failed Method (pDHI->Set_Analog_Settings), HRESULT", hr);

							// print success message
							if (SUCCEEDED(hr) && (Status == 0)) cout << "Value Set" << endl;
						}

					// otherwise print error message
					} else {
						cout << "setting not recognized for this scanner" << endl;
					}

				// otherwise print error message
				} else {
					cout << argv[0] << " set setting value" << endl;
				}
				break;

			// upload
			case 18:

				// issue command to server
				if (argc >= 3) {

					// if the local file name is not overridden, then set it to the same as name on head
					if (strcmp(filename, "display") == 0) sprintf(filename, "%s", argv[2]);

					// upload file
					hr = pDHI->File_Upload(head, (unsigned char *) argv[2], &Filesize, &CheckSum, &filebuf, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->File_Upload), HRESULT", hr);
				} else {
					cout << "No file name specified" << endl;
				}

				// if successful, write the file
				if (SUCCEEDED(hr) && (Status == 0)) {

					// compare the checksum
					for (check = 0, i = 0 ; i < Filesize ; i++) check += (unsigned long) filebuf[i];
					if (check != CheckSum) {
						cout << "File Upload Checksum Did Not Match" << endl;

					// success, write file
					} else {
						if ((fp = fopen(filename, "wb")) == NULL) {
							cout << "Error Opening File " << filename << endl;
						} else {

							// write data and close file
							fwrite((void *) filebuf, sizeof(unsigned char), Filesize, fp);
							fclose(fp);

							// message
							cout << "File Uploaded" << endl;
						}
					}

					// release the memory
					::CoTaskMemFree((void *) filebuf);
				}
				break;

			// download
			case 19:

				// open the file 
				if (argc >= 3) {

					// if the head file name is not overridden, then set it to the same as local file
					if (strcmp(filename, "display") == 0) {
						ptr = argv[2];
						while (strstr(ptr, "\\") != NULL) {
							ptr = strstr(ptr, "\\");
							ptr = &ptr[1];
						}
						sprintf(filename, "%s", ptr);
					}

					// open the local file
					if ((fp = fopen(argv[2], "rb")) != NULL) {

						// get the size of the file
						fseek(fp, 0, SEEK_END);
						Filesize = ftell(fp);
						fseek(fp, 0, SEEK_SET);

						// allocate memory
						filebuf = (unsigned char *) ::CoTaskMemAlloc((ULONG) Filesize);
						if (filebuf != NULL) {

							// read the data and close file
							bytes = fread((void *) filebuf, sizeof(unsigned char), Filesize, fp);
							fclose(fp);

							// compute the checksum
							for (CheckSum = 0, i = 0 ; i < Filesize ; i++) CheckSum += (unsigned long) filebuf[i];

							// issue command to server
							hr = pDHI->File_Download(head, Filesize, CheckSum, (unsigned char *) filename, &filebuf, &Status);
							if (FAILED(hr)) Add_Error("Failed Method (pDHI->File_Download), HRESULT", hr);

							// release the memory
							::CoTaskMemFree((void *) filebuf);

							// if successful, indicate success
							if (SUCCEEDED(hr) && (Status == 0)) cout << "File  Downloaded" << endl;

						// memory allocation error
						} else {
							cout << "Memory Allocation Problem" << endl;
						}

					// file open error
					} else {
						cout << "Error Opening File: " << filename << endl;
					}
				} else {
					cout << "File Name Not Specified" << endl;
				}
				break;

			// load RAM
			case 20:

				// determine which RAM is being loaded
				code = -1;
				if (argc >= 3) {
					for (i = 0 ; (i < NUM_RAM_TYPES) && (code == -1) ; i++) if (strcmp(LoadStr[i], argv[2]) == 0) code = i;
				}

				// if it is a valid code for this scanner
				if (code >= 0)  {

					// if a filename is specified, then the file is to be downloaded first
					if (strcmp(filename, "display") != 0) {

						// if downloading analog settings
						if (code == RAM_ANALOG_SETTINGS) {
							sprintf((char *) HeadFile, "%d_%s.txt", config, type_str[code]);
						} else if (layer == LAYER_FAST) {
							sprintf((char *) HeadFile, "%d_%sfast.txt", config, type_str[code]);
						} else if (layer == LAYER_SLOW) {
							sprintf((char *) HeadFile, "%d_%sslow.txt", config, type_str[code]);
						} else if (layer == LAYER_ALL) {
							sprintf((char *) HeadFile, "%d_%sboth.txt", config, type_str[code]);
							if (ScannerType != SCANNER_P39_IIA) {
								ok = 0;
								cout << "layer invalid for download";
							}
						} else {
							ok = 0;
							cout << "layer invalid for download";
						}

						// if still downloading
						if (ok == 1) {

							// open the file
							if ((fp = fopen(filename, "rb")) != NULL) {

								// get the size of the file
								fseek(fp, 0, SEEK_END);
								Filesize = ftell(fp);
								fseek(fp, 0, SEEK_SET);

								// allocate memory
								filebuf = (unsigned char *) ::CoTaskMemAlloc((ULONG) Filesize);
								if (filebuf != NULL) {

									// read the data and close file
									bytes = fread((void *) filebuf, sizeof(unsigned char), Filesize, fp);
									fclose(fp);

									// compute the checksum
									for (CheckSum = 0, i = 0 ; i < Filesize ; i++) CheckSum += (unsigned long) filebuf[i];

									// issue command to server
									hr = pDHI->File_Download(head, Filesize, CheckSum, HeadFile, &filebuf, &Status);
									if (FAILED(hr)) Add_Error("Failed Method (pDHI->File_Download), HRESULT", hr);

									// release the memory
									::CoTaskMemFree((void *) filebuf);

									// if download not successful, then don't do load
									if (FAILED(hr) || (Status != 0)) ok = 0;

								// memory allocation error
								} else {
									cout << "Memory Allocation Problem" << endl;
									ok = 0;
								}
							} else {
								cout << "Problem Opening File" << endl;
								ok = 0;
							}
						}
					}

					// if load is still to be done
					if (ok == 1) {

						// issue command to server
						hr = pDHI->Load_RAM(config, head, code, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Load_RAM), HRESULT", hr);

						// if successful, print it
						if (SUCCEEDED(hr)) {

							// good status means it loaded successfully
							if (Status == 0) {
								cout << "Loaded" << endl;

							// not loaded
							} else {
								cout << "Not Loaded - Possibly Files For All Layers Not Present" << endl;
							}
						}
					}

				// otherwise print error message
				} else {
					cout << "load arguments incorrect" << endl;
				}
				break;

			// pass command
			case 21:

				// argument must exist
				if (argc >= 3) {

					// convert command over to correct string type and size
					sprintf((char *) command, "%s", argv[2]);

					// issue command to server
					hr = pDHI->Pass_Command(head, command, response, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Pass_Command), HRESULT", hr);

					// if successful, print response
					if (SUCCEEDED(hr) && (Status == 0)) cout << "[" << response << "]" << endl;

				} else {
					cout << "command missing" << endl;
				}
				break;

			// Pass-Thru Mode
			case 22:

				// issue command to server
				hr = pDHI->PassThru_Mode(HeadSelect, SourceSelect, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->PassThru_Mode), HRESULT", hr);

				// poll for progress
				while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
					hr = pDHI->Progress(64, &Percent, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
					if (SUCCEEDED(hr) && (Status == 0)) {
						cout << "\rProgress: " << Percent << "%" << flush;
						if (Percent == 100) {
							cout << endl;
						} else {
							Sleep(1000);
						}
					}
				}

				// if successful, print the action
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Coincidence Processor: Pass-Thru" << endl;
				break;

			// Time Mode
			case 23:

				// must have at least 3 arguments
				switch (ScannerType) {

				case SCANNER_P39:
				case SCANNER_P39_IIA:
					if (argc >= 4) {

						// determine switch state
						if (strcmp(argv[2], "on") == 0) {
							OffOn = 1;
						} else if (strcmp(argv[2], "off") == 0) {
							OffOn = 0;
						} else {
							sprintf(Str, "Transmission Must Be Off/On - Received %s", argv[2]);
							Status = Add_Error(Str, 0);
						}

						// extract module pair
						bytes = hextobin(argv[3], data);
						for (i = 0 ; i < bytes ; i++) ModulePair += ((long) data[i]) << (8 * ((bytes-1) - i));

						// issue command to server
						if (Status == 0) {
							hr = pDHI->Time_Mode(OffOn, ModulePair, &Status);
							if (FAILED(hr)) Add_Error("Failed Method (pDHI->Time_Mode), HRESULT", hr);
						}
					} else {
						cout << argv[0] << " time transmission_Off/On ModulePair" << endl;
					}
					break;

				default:
					if (argc >= 3) {

						// determine switch state
						if (strcmp(argv[2], "on") == 0) {
							OffOn = 1;
						} else if (strcmp(argv[2], "off") == 0) {
							OffOn = 0;
						} else {
							sprintf(Str, "Transmission Must Be Off/On - Received %s", argv[2]);
							Status = Add_Error(Str, 0);
						}

						// issue command to server
						if (Status == 0) {
							hr = pDHI->Time_Mode(OffOn, ModulePair, &Status);
							if (FAILED(hr)) Add_Error("Failed Method (pDHI->Time_Mode), HRESULT", hr);
						}
					} else {
						cout << argv[0] << " time transmission_Off/On" << endl;
					}
					break;
				}

				// poll for progress
				while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
					hr = pDHI->Progress(64, &Percent, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
					if (SUCCEEDED(hr) && (Status == 0)) {
						cout << "\rProgress: " << Percent << "%" << flush;
						if (Percent == 100) {
							cout << endl;
						} else {
							Sleep(1000);
						}
					}
				}

				// if successful, print the action
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Coincidence Processor: Timing" << endl;
				break;

			// Reset
			case 24:

				// command different for coincidence processor
				if (head == 64) {

					// find reset mode
					mode = -1;
					if (argc >= 3) {
						for (i = 0 ; (mode == -1) && (i < 6) ; i++) if (strcmp(argv[2], ResetStr[i]) == 0) mode = i;
					} else {
						cout << "Reset Mode Not Specified" << endl;
					}

					// issue command if found
					if (mode != -1) {
						Reset = 1 << mode;
						if ((mode == 0) && ((ScannerType == SCANNER_P39) || (ScannerType == SCANNER_P39_IIA))) cout << "Rebooting Will Take 3.5 Minutes" << endl;
						hr = pDHI->Reset_CP(Reset, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Reset_CP), HRESULT", hr);

						// if wait for completion then start off with sleep before starting to poll
						if (wait == 1) {
							cout << "\rProgress: 0%" << flush;
							if (mode == 0) if ((ScannerType != SCANNER_P39) && (ScannerType != SCANNER_P39_IIA)) Sleep(10000);
						}

						// poll for progress
						Percent = 0;
						while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
							hr = pDHI->Progress(head, &Percent, &Status);
							if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
							if (SUCCEEDED(hr) && (Status == 0)) {
								cout << "\rProgress: " << Percent << "%" << flush;
								if (Percent == 100) {
									cout << endl;
								} else {
									Sleep(1000);
								}
							}
						}

					// otherwise display error
					} else {
						cout << "Reset Mode Not Recognized For Coincidence Processor" << endl;
					}

				// otherwise issue command to head
				} else {
					hr = pDHI->Reset_Head(head, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Reset_Head), HRESULT", hr);
					if (hr != S_OK) Status = -1;

					// if wait for completion then start off with some sleep
					if ((wait == 1) && (Status == 0)) {
						cout << "\rProgress: 0%" << flush;
						Sleep(ResetWait);
					}

					// poll for progress
					while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
						hr = pDHI->Progress(head, &Percent, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
						if (SUCCEEDED(hr) && (Status == 0)) {
							cout << "\rProgress: " << Percent << "%" << flush;
							if (Percent == 100) {
								cout << endl;
							} else {
								Sleep(1000);
							}
						}
					}
				}

				// if successful, print the action
				if (SUCCEEDED(hr) && (Status == 0)) cout << head << " Reset" << endl;
				break;

			// DOS
			case 25:

				// argument must exist
				if (argc >= 3) {

					// convert command over to correct string type and size
					sprintf((char *) command, "%s", argv[2]);

					// issue command to server
					hr = pDHI->OS_Command(head, command, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->OS_Command), HRESULT", hr);

					// if successful, print response
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Completion OK" << endl;

				} else {
					cout << "command missing" << endl;
				}
				break;

			// force unlock - place holder
			case 26:
				cout << "Unlocking Server" << endl;
				break;

			// Initialize Scan
			case 27:

				// P39 and PETSPECT require on/off for transmission source
				if ((ScannerType == SCANNER_P39) || (ScannerType == SCANNER_P39_IIA) || (ScannerType == SCANNER_PETSPECT)) {

					// check for first argument
					if (argc >= 3) {

						// determine switch state
						if (strcmp(argv[2], "on") == 0) {
							OffOn = 1;
						} else if (strcmp(argv[2], "off") == 0) {
							OffOn = 0;
						} else {
							sprintf(Str, "Transmission Must Be Off/On - Received %s", argv[2]);
							Add_Error(Str, 0);
							ok = 0;
						}
					// improper arguments
					} else {
						cout << argv[0] << " " << argv[1] << " off/on" << endl;
						ok = 0;
					}
				}

				// P39 must have module pair
				if ((ScannerType == SCANNER_P39) || (ScannerType == SCANNER_P39_IIA)) {

					// check for second argument
					if (argc >= 4) {

						// extract module pair
						bytes = hextobin(argv[3], data);
						for (i = 0 ; i < bytes ; i++) ModulePair += ((long) data[i]) << (8 * ((bytes-1) - i));

					// improper arguments
					} else {
						cout << argv[0] << " " << argv[1] << " " << argv[2] << " <ModulePair>" << endl;
						ok = 0;
					}
				}

				// issue command to server
				if (ok == 1) {
					hr = pDHI->Initialize_Scan(OffOn, ModulePair, config, layer, lld, uld, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Initialize_Scan), HRESULT", hr);
				}

				// poll for progress
				while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
					hr = pDHI->Progress(-2, &Percent, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
					if (SUCCEEDED(hr) && (Status == 0)) {
						cout << "\rProgress: " << Percent << "%" << flush;
						if (Percent == 100) {
							cout << endl;
						} else {
							Sleep(1000);
						}
					}
				}

				// if successful, print the action
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Scan Initialized" << endl;
				break;

			// Start Scan
			case 28:

				// issue command to server
				hr = pDHI->Start_Scan(&Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Start_Scan), HRESULT", hr);

				// if successful, print the action
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Scan Started" << endl;
				break;

			// Stop Scan
			case 29:

				// issue command to server
				hr = pDHI->Stop_Scan(&Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Stop_Scan), HRESULT", hr);

				// if successful, print the action
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Scan Stopped" << endl;
				break;

			// stats (get statistics)
			case 30:

				// issue command to server
				hr = pDHI->Get_Statistics(Stats, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Get_Statistics), HRESULT", hr);

				// if successful, print statistics
				if (SUCCEEDED(hr) && (Status == 0)) {
					if (Stats[2] == CP_MODE_PASSTHRU) {
						cout << "Counts: " << Stats[0] << endl;
					} else {
						cout << "Prompts: " << Stats[0] << endl;
					}
					cout << "Randoms: " << Stats[1] << endl;
					if (Stats[2] == -1) {
						cout << "Mode: Initialized - No Mode" << endl;
					} else {
						cout << "Mode: " << CPStr[Stats[2]] << endl;
					}
					for (index = 3 ; index < 19 ; index++) if (HeadPresent[index-3] != 0) {
						cout << "Head " << index-3 << ": " << Stats[index] << endl;
					}
				}
				break;

			// progress
			case 31:

				// issue command to server
				hr = pDHI->Progress(head, &Percent, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);

				// if successful, print progress percentage
				if (SUCCEEDED(hr) && (Status == 0)) cout << "\rProgress: " << Percent << "%" << endl;
				break;

			// wait
			case 32:

				// poll for progress
				while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr)) {
					hr = pDHI->Progress(head, &Percent, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
					if (SUCCEEDED(hr) && (Status == 0)) {
						cout << "\rProgress: " << Percent << "%" << flush;
						if (Percent == 100) {
							cout << endl;
						} else {
							Sleep(1000);
						}
					}
				}
				break;

			// transmission trajectory
			case 33:

				// must have sufficient arguments
				if (argc >= 4) {

					// translate arguments
					speed = atol(argv[2]);
					step = atol(argv[3]);

					// issue command to server
					hr = pDHI->Transmission_Trajectory(speed, step, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Transmission_Trajectory), HRESULT", hr);

					// if successful, report it
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Trajectory " << speed << " " << step << endl;

				// otherwise, error message
				} else {
					cout << argv[0] << " trajectory speed step" << endl;
				}
				break;

			// tag control
			case 34:

				// must have at least 3 arguments
				if (argc >= 4) {

					// determine first switch state
					if (strcmp(argv[2], "on") == 0) {
						OffOn = 1;
					} else if (strcmp(argv[2], "off") == 0) {
						OffOn = 0;
					} else {
						sprintf(Str, "Singles Tags Must Be Off/On - Received %s", argv[2]);
						Status = Add_Error(Str, 0);
					}

					// determine second switch state
					if (Status == 0) {
						if (strcmp(argv[3], "on") == 0) {
							OnOff = 1;
						} else if (strcmp(argv[3], "off") == 0) {
							OffOn = 0;
						} else {
							sprintf(Str, "Time Tags Must Be Off/On - Received %s", argv[3]);
							Status = Add_Error(Str, 0);
						}
					}

					// issue command to server
					if (Status == 0) {
						hr = pDHI->Tag_Control(OffOn, OnOff, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Tag_Control), HRESULT", hr);
					}

					// if successful, print the action
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Singles: " << OffOn << " Time: " << OnOff << endl;

				// otherwise, error message
				} else {
					cout << argv[0] << " tag_control singles_Off/On time_Off/On" << endl;
				}
				break;

			// set temperature limits
			case 35:

				// must have at least 3 arguments
				if (argc >= 4) {

					// convert arguments to floating point
					low = (float) atof(argv[2]);
					high = (float) atof(argv[3]);

					// issue command to server
					hr = pDHI->Set_Temperature_Limits(low, high, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Set_Temperature_Limits), HRESULT", hr);

					// if successful, print the action
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Low: " << low << " High: " << high << endl;

				// otherwise, error message
				} else {
					cout << argv[0] << " set_temp low high" << endl;
				}
				break;

			// report voltages
			case 36:

				// issue command to server
				hr = pDHI->Report_Voltage(Voltage, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Report_Voltage), HRESULT", hr);

				// if successful, print voltages
				if (SUCCEEDED(hr) && (Status == 0)) {
					for (head = 0 ; head < MAX_HEADS ; head++) if (HeadPresent[head] != 0) {
						cout << "Head " << head;
						for (index = 0 ; index < 4 ; index++) cout << "\t" << Voltage[(head*4)+index];
						cout << endl;
					}
				}
				break;

			// report HRRT high voltages
			case 37:

				// issue command to server
				hr = pDHI->Report_HRRT_High_Voltage(HighVoltage, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Report_HRRT_High_Voltage), HRESULT", hr);

				// if successful, print voltages
				if (SUCCEEDED(hr) && (Status == 0)) {
					for (head = 0 ; head < MAX_HEADS ; head++) if (HeadPresent[head] != 0) {
						cout << "Head " << head;
						for (index = 0 ; index < 5 ; index++) cout << "\t" << HighVoltage[(head*5)+index];
						cout << endl;
					}
				}
				break;

			// set coincidence timing window
			case 38:

				// must have 3 arguments
				if (argc >= 3) {

					// convert time window to long integer
					code = atol(argv[2]);

					// issue command to server
					hr = pDHI->Time_Window(code, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Time_Window), HRESULT", hr);

					// poll for progress
					while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
						hr = pDHI->Progress(64, &Percent, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
						if (SUCCEEDED(hr) && (Status == 0)) {
							cout << "\rProgress: " << Percent << "%" << flush;
							if (Percent == 100) {
								cout << endl;
							} else {
								Sleep(1000);
							}
						}
					}

					// if successful, report it
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Coincidence Timing Window: " << argv[2] << endl;

				// otherwise print error message
				} else {
					cout << argv[0] << " " << argv[1] << " must specify window" << endl;
				}
				break;

			// Put Coincidence Processor in Tagword only mode
			case 39:

				// issue command to server
				hr = pDHI->TagWord_Mode(&Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->TagWord_Mode), HRESULT", hr);

				// poll for progress
				while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
					hr = pDHI->Progress(64, &Percent, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
					if (SUCCEEDED(hr) && (Status == 0)) {
						cout << "\rProgress: " << Percent << "%" << flush;
						if (Percent == 100) {
							cout << endl;
						} else {
							Sleep(1000);
						}
					}
				}

				// if successful, report it
				if (SUCCEEDED(hr) && (Status == 0)) cout << "Coincidence Processor: Tagword Only Mode" << endl;
				break;

			// Tagword Insertion
			case 40:

				// must have argument
				if (argc >= 3) {

					// check for file data
					if (strcmp(filename, "display") != 0) {

						// if ascii file
						if (ascii == 1) {

							// open file
							if ((fp = fopen(filename, "rt")) != NULL) {

								// Allocate Memory
								Filesize = 8;
								Tagword = (ULONGLONG *) ::CoTaskMemAlloc((ULONG) Filesize);
								if (Tagword != NULL) {

									// for each line in the file
									while (SUCCEEDED(hr) && (fscanf(fp, "%s", str) == 1)) {

										// convert to binary
										bytes = hextobin(str, data);
										*Tagword = 0;
										for (i = 0 ; i < bytes ; i++) *Tagword += ((ULONGLONG) data[i]) << (8 * ((bytes-1) - i));

										// issue command to server
										hr = pDHI->Tag_Insert(Filesize/8, &Tagword, &Status);
										if (FAILED(hr)) Add_Error("Failed Method (pDHI->Tag_Insert), HRESULT", hr);
									}

									// release the memory
									::CoTaskMemFree((void *) Tagword);

									// invalidate the memory
									memvalid = 0;

								// problem allocating memory
								} else cout << "Error Allocating Memory" << endl;

							// problem opening file
							} else cout << "Error Opening File: " << filename << endl;


						// binary file
						} else {

							// open file
							if ((fp = fopen(filename, "rb")) != NULL) {

								// get the size of the file
								fseek(fp, 0, SEEK_END);
								Filesize = ftell(fp);
								fseek(fp, 0, SEEK_SET);

								// allocate memory
								Tagword = (ULONGLONG *) ::CoTaskMemAlloc((ULONG) Filesize);
								if (Tagword != NULL) {

									// read the data and close file
									memvalid = 1;
									bytes = fread((void *) Tagword, sizeof(ULONGLONG), Filesize/8, fp);
									fclose(fp);

								// problem allocating memory
								} else cout << "Error Allocating Memory" << endl;

							// problem opening file
							} else cout << "Error Opening File: " << filename << endl;
						}

					// otherwise, from command line
					} else {

						// Allocate Memory
						Filesize = 8;
						Tagword = (ULONGLONG *) ::CoTaskMemAlloc((ULONG) Filesize);
						if (Tagword != NULL) {

							// convert argument from hexadecimal
							memvalid = 1;
							bytes = hextobin(argv[2], data);
							*Tagword = 0;
							for (i = 0 ; i < bytes ; i++) *Tagword += ((ULONGLONG) data[i]) << (8 * ((bytes-1) - i));

						// problem allocating memory
						} else cout << "Error Allocating Memory" << endl;
					}

					// valid memory to insert
					if (memvalid == 1) {

						// issue command to server
						hr = pDHI->Tag_Insert(Filesize/8, &Tagword, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Tag_Insert), HRESULT", hr);

						// release the memory
						::CoTaskMemFree((void *) Tagword);
					}

					// if successful, report it
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Tagword(s) Inserted" << endl;

				// otherwise indicate no data
				} else {
					cout << "Tag_Insert: No Tagword Specified" << endl;
				}
				break;

			// Check the mode of the head or Coincidence Processor
			case 41:

				// different methods based on head number
				switch (head) {

					// coincidence processor
				case 64:

					// issue command to server
					hr = pDHI->Check_CP_Mode(&mode, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Check_CP_Mode), HRESULT", hr);

					// if successful, display
					if (SUCCEEDED(hr) && (Status == 0)) {
						if (mode == -1) {
							cout << "Coincidence Processor Mode: Initialized - No Mode" << endl;
						} else {
							cout << "Coincidence Processor Mode: " << CPStr[mode] << endl;
						}
					}
					break;

				// all heads
				case -1:

					// issue command to server
					hr = pDHI->Head_Status(&HeadCount, &Info, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Head_Status), HRESULT", hr);

					// successful, display
					if (hr == S_OK) {

						// Show Results
						for (i = 0 ; i < HeadCount ; i++) {

							// head number
							cout << "Head: " << Info[i].Address << " Mode: ";

							// if the head is not in a mode, then that is all we care about
							if (Info[i].Mode == -1) {
								cout << "Off" << endl;
							} else {
								cout << DHIModeStr[Info[i].Mode] << " Configuration: " << Info[i].Config << " Layer: " << LayerStr[Info[i].Layer] <<
									" LLD: " << Info[i].LLD << " ULD: " << Info[i].ULD << " SLD: " << Info[i].SLD << endl;
							}
						}
					}
					break;

				// individual heads
				default:

					// issue command to server
					hr = pDHI->Check_Head_Mode(head, &mode, &config, &layer, &lld, &uld, &Status);
					if (FAILED(hr)) Add_Error("Failed Method (pDHI->Check_Head_Mode), HRESULT", hr);

					// if successful, display
					if (SUCCEEDED(hr) && (Status == 0)) {

						// head number
						cout << "Head " << head << " Mode: ";

						// display mode
						if (mode == -1) {
							cout << "off" << endl;
						} else {
							cout << DHIModeStr[mode] << " Configuration: " << config << " Layer: " << LayerStr[layer] << " LLD: " << lld << " ULD: " << uld << endl;
						}
					}
					break;
				}

				break;

			// Check whether the point source is extended or retracted
			case 42:

				// issue command to server
				hr = pDHI->Point_Source_Status(head, &OnOff, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Point_Source_Status), HRESULT", hr);

				// if successful, display
				if (SUCCEEDED(hr) && (Status == 0)) {

					// head number
					cout << "Head " << head << " Point Source: ";

					// display mode
					if (OnOff == 0) {
						cout << "RETRACTED" << endl;
					} else {
						cout << "EXTENDED" << endl;
					}
				}
				break;

			// Check health of detector system
			case 43:

				// issue command to server
				hr = pDHI->Health_Check(&NumFailed, &Failed, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Check_CP_Mode), HRESULT", hr);

				// if healthy
				if (hr == S_OK) {

					// print message
					cout << "Detector System Healthy" << endl;

				// show what parts aren't healthy
				} else {

					// bad result
					if (FAILED(hr)) {

						// problem with abstraction layer
						cout << "Problem with Detector Abstraction Layer" << endl;

					// print details
					} else {

						// coincidence processor would be first
						for (Next = 0 ; Next < NumFailed ; Next++) {
							if (Failed[Next] == 64) {
								cout << "Coincidence Processor Not Accessible" << endl;
							} else {
								cout << "Head " << Failed[Next] << " Not Accessible" << endl;
							}
						}

						// release the memory
						::CoTaskMemFree((void*) *Failed);
					}
				}
				break;

			// put scanner into mode for pet
			case 44:

				// must have at least 3 arguements
				if (argc >= 3) {

					// determine the scan type
					for (i = 0, ScanType = -1 ; (i < 3) && (ScanType == -1) ; i++) {
						if (strcmp(ScanTypes[i], argv[2]) == 0) ScanType = i;
					}

					// issue command to server
					if (ScanType != -1) {
						hr = pDHI->Initialize_PET(ScanType, lld, uld, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Initialize_PET), HRESULT", hr);

					// unrecognized scan type
					} else {
						sprintf(Str, "Unrecognized Scan Type: %s", argv[2]);
						Status = Add_Error(Str, ScanType);
					}

					// poll for progress
					while ((Percent < 100) && (Status == 0) && SUCCEEDED(hr) && (wait == 1)) {
						hr = pDHI->Progress(-2, &Percent, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->Progress), HRESULT", hr);
						if (SUCCEEDED(hr) && (Status == 0)) {
							cout << "\rProgress: " << Percent << "%" << flush;
							if (Percent == 100) {
								cout << endl;
							} else {
								Sleep(1000);
							}
						}
					}

					// if successful, print the action
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Scan Initialized" << endl;

				// insufficient arguments
				} else {
					cout << argv[0] << " " << argv[1] << " Em|EmTx|Tx" << endl;
				}
				break;

			// count
			case 45:

				// call method
				hr = pDHI->CountRate(&CorrectedSingles, &UncorrectedSingles, &Prompts, &Randoms, &Scatters, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->CountRate), HRESULT", hr);

				// if successfull
				if (hr == S_OK) {
					cout << "Corrected Singles:\t" << CorrectedSingles << endl;
					cout << "Uncorrected Singles:\t" << UncorrectedSingles << endl;
					cout << "Prompts:\t\t" << Prompts << endl;
					cout << "Randoms:\t\t" << Randoms << endl;
					cout << "Scatters:\t\t" << Scatters << endl;
				}
				break;

			// Ring Singles
			case 46:

				// call method
				hr = pDHI->Ring_Singles(&NumRings, &RingSingles, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Ring_Singles), HRESULT", hr);

				// if successfull, loop through rings
				if (hr == S_OK) for (i = 0 ; i < NumRings ; i++) cout << "Ring: " << i << " = " << RingSingles[i] << endl;
				break;

			// kill server
			case 47:

				// issue command to server
				hr = pDHI->KillSelf();
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->KillSelf), HRESULT", hr);

				// success in this case is actually a failure code
				if (hr == RPC_FAILED) {
					cout << "Detector Abstraction Layer COM Server Killed" << endl;
					hr = S_OK;
				} else {
					cout << "Received Unexpected Return Code: " << hr << endl;
				}

				break;

			// log control
			case 48:

				// must have at least 3 arguments
				if (argc >= 3) {

					// determine switch state
					if (strcmp(argv[2], "on") == 0) {
						OffOn = 1;
					} else if (strcmp(argv[2], "off") == 0) {
						OffOn = 0;
					} else {
						sprintf(Str, "Log Must Be Off/On - Received %s", argv[2]);
						Status = Add_Error(Str, 0);
					}

					// issue command to server
					if (Status == 0) {
						hr = pDHI->SetLog(OffOn, &Status);
						if (FAILED(hr)) Add_Error("Failed Method (pDHI->SetLog), HRESULT", hr);
					}

					// if successful, print the action
					if (SUCCEEDED(hr) && (Status == 0)) cout << "Log: " << OffOn << endl;

				// otherwise, error message
				} else {
					cout << argv[0] << " log Off/On" << endl;
				}
				break;

			// ping
			case 49:

				// issue command to server
				hr = pDHI->Ping(head, response, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Ping), HRESULT", hr);

				// if successful, print the build id
				if (SUCCEEDED(hr) && (Status == 0)) cout << "BuildID: " << response << endl;
				break;

			// lock
			case 50:

				// if it is to be written to a file
				if (strcmp(filename, "display") != 0) {

					// open the file
					if ((fp = fopen(filename, "wb")) != NULL) {

						// write
						if (fwrite((void *) &LockCode, 8, 1, fp) != 1) Status = -1;

						// close
						fclose(fp);
					} else {
						Status = -1;
					}
				}

				// if succeeded in writing file
				if (Status == 0) {

					// print the lockcode
					sprintf(buffer, "Lock Code = %I64d", LockCode);
					cout << buffer << endl;

					// we don't want the cleanup code to unlock the servers
					Locked = 0;

					// now we have to release the pointer ourself
					hr = pLock->Release();
				} else {
					cout << "Error Writing File" << endl;
				}
				break;

			// unlock
			case 51:

				// if it is to be read from a file
				if (strcmp(filename, "display") != 0) {

					// open the file
					if ((fp = fopen(filename, "rb")) != NULL) {

						// read
						if (fread((void *) &LockCode, 8, 1, fp) != 1) {
							Status = -1;
							cout << "Error Reading File" << endl;
						}

						// close
						fclose(fp);
					} else {
						cout << "Error Opening File" << endl;
						Status = -1;
					}

				// otherwise, it must be an arguement
				} else {

					// convert arguement
					if (argc >= 3) {
						LockCode = _atoi64(argv[2]);
					} else {
						Status = -1;
						cout << argv[0] << " unlock LockCode | -f <filename>" << endl;
					}
				}

				// if succeeded in retrieving lock Code
				if (Status == 0) {

					// attach to the lock server
					pLock = Get_Lock_Ptr();
					if (pLock != NULL) {
						Locked = 1;
					} else {
						cout << "Failed To Get LockMgr Pointer" << endl;
					}
				}
				break;

			// Hardware Temperature and Voltage
			case 52:

				// issue command to server
				hr = pDHI->Report_Temperature_Voltage(&cptemp, &low, &high, &NumTemps, &HeadCount, &HW_Info, &Status);
				if (FAILED(hr)) Add_Error("Failed Method (pDHI->Report_Temperature_Voltage), HRESULT", hr);

				// successful, display
				if (hr == S_OK) {

					// coincindence processor information
					cout << endl;
					cout << "Coincidence Processor Temperature: " << cptemp << endl;
					cout << "Temperature Range: " << low << " - " << high << endl << endl;

					// head information
					for (i = 0 ; i < HeadCount ; i++) {

						// head number
						cout << "Head: " << HW_Info[i].Address << endl;
						
						// voltages
						cout << "High Voltage: " << HW_Info[i].HighVoltage << endl;
						cout << "+5 Volts: " << HW_Info[i].Plus5Volts << endl;
						cout << "-5 Volts: " << HW_Info[i].Minus5Volts << endl;
						cout << "Other Voltage: " << HW_Info[i].OtherVoltage << endl;

						// lti temperature
						if (ScannerType != SCANNER_HRRT) cout << "LTI Temperature: " << HW_Info[i].LTI_Temperature << endl;

						// dhi temperatures
						cout << "DHI Temperatures:";
						for (j = 0 ; j < NumTemps ; j++) cout << " " << HW_Info[i].DHI_Temperature[j];

						// end line
						cout << endl << endl;

						// add a little serial line dealy
						Sleep(100);
					}
				}
				break;

			// action not recognized
			default:
				cout << "Command " << argv[1] << " not recognized" << endl;
				break;
			}
		}

		// if server command failed
		if (FAILED(hr)) {
			cout << argv[1] << " Method Failure" << endl;
			Status = -1;
		} else if (Status > 0) {
			hr = pDHI->Error_Lookup(Status, &code, ErrorString);
			if (FAILED(hr)) Add_Error("Failed Method (pDHI->Check_CP_Mode), HRESULT", hr);
			cout << ErrorString << endl;
			Status = -1;
		}

		// if locked
		if (Locked == 1) {

			// unlock
			hr = pLock->UnlockACS(LockCode);
			if (FAILED(hr)) {
				Add_Error("Method Failed (pLock->LockACS), HRESULT", hr);
				Status = -1;
			} else if (hr == S_FALSE) {
				Add_Error("Invalid Lock ID", 0);
			} else {
				Locked = 0;
				if (num_action == 51) cout << "Servers Unlocked" << endl;
			}

			// release the lock server
			pLock->Release();
		}

		// close the file if it was opened
		if (fp != NULL) fclose(fp);

		// Release the server
		if (pDHI != NULL) pDHI->Release();
	}

	// Release COM
	::CoUninitialize();

	// proper exit code
	if ((Status != 0) || (hr != S_OK)) {
		exit(1);
	} else {
		exit(0);
	}
}
