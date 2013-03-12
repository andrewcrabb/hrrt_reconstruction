//==========================================================================
// File			:	ServQCMsgID.h
//					Copyright 1999 by CTI
// Description	:	Contains window message ids used in the Service/QC GUI
//	
// Author		:	Selene F. Tolbert
//
// Date			:	24 January 2001
//
// Author		Date		Update
// S Tolbert	24 Jan 01	Created
// S Tolbert	11 June 01	Added iLogMe integer field to the gantry struct
//							to notify in the WM_STATUSUPD callback to log the
//							message being passed or to not log it.
// S Tolbert	14 June 01	Added iAllowLock integer field to the gantry struct
//							to only allow the User Interface to lock the 
//							gantry operations that resulted with a SO or S123
//							status.
// S Tolbert	15 Aug 01	Added iProgTime integer field to the gantry struct
//							to notify in the GIU of updates to the progress bar(s).
// S Tolbert	16 Aug 01	Added more Window Message ID for the rest of the 
//							service tools.
// S Tolbert	29 Aug 01	Added more Window Message IDs for the Settings Mgr.
// S Tolbert	14 Sept 01	Added more Window Message IDs to represent the Status
//							updates and status fatal of operations.
// S Tolbert	14 Sept 01	Added SettingsOperation enums for the Gantry Settings
//							operations.
// S Tolbert	17 Sept 01	Added Bucket Status to the SettingsOperation enums
// S Tolbert	02 Oct 01	Eliminated the NBLOCKS and NBUCKETS defines
// S Tolbert	04 Oct 01	Added the INVALIDSTATE Gantry Setup Operation.  This
//							states reperesents the wrong acquisition of Scanner type,
//							total number of blocks or total number of buckets information.
// S Tolbert	08 Oct 01	Added the ScannerType enum (HRPLUS, ECCEL)
// S Tolbert	19 Nov 01	Added more Window Message IDs for the Database Mgr.
// S Tolbert	20 Nov 01	Added enum representation for the bucket database operations.
// S Tolbert	26 Nov 01	Added defined MAXCONTROLLERS to represent the maximum number
//							of controllers per system used by the Gantry Firmware Manager.
// S Tolbert	10 Dec 01	Added enum values for the HRPLUS_MODEL and ACCEL_MODEL used 
//							when obtaining the gantry object model information.
// S Tolbert	12 Dec 01	Eliminated enum valued for the HRPLUS_MODEL and ACCEL_MODEL. 
//							No longer used by the Service applications.
//==========================================================================

// Set guards to prevent multiple header file inclusion
//
#if !defined(INCLUDE_SERVQCMSGID_H)
#define INCLUDE_SERVQCMSGID_H


//////////////////////////////////////////////////////////////////////////////
// Window Messages
// callback window events
//////////////////////////////////////////////////////////////////////////////

//// FSE Gantry Setup Manager ////
#define	WM_COLORGRID			(WM_USER+1)		// to update the cell colors in grid/matrix
#define WM_STATUSUPD			(WM_USER+2)		// to update the status (message) bar
#define WM_STATUSFATAL			(WM_USER+3)		// notify user with status fatal (error) information (Scheduler thread is dead)
#define WM_ABORTCOMPLETE		(WM_USER+4)		// notify user of completion/processed abort signal
#define WM_SETUPCOMPLETE		(WM_USER+5)		// notify user of completion/processed Gantry Setup
#define WM_SETUPSTART			(WM_USER+6)		// notify user of start Gantry Setup
#define WM_STATRESTART			(WM_USER+7)		// notify user of restart of Gantry Setup (Scheduler thread is alive)
#define WM_ELAPSETIME			(WM_USER+8)		// notify user of elapse time

//// Analyzer (Histogram) Manager ////
#define	WM_STATUSACQUISITION	(WM_USER+9)		// to update the progress status bar for the data acquistion
#define WM_ACQUISITIONCOMPLETE	(WM_USER+10)	// notify user of completion/processed data acquisition
#define WM_ACQUISITIONSTOP		(WM_USER+11)	// notify user of completion/procesed stop signal
#define WM_ACQUISITIONUPD		(WM_USER+12)	// notify user with status update information 
#define WM_ACQUISITIONFATAL		(WM_USER+13)	// notify user with status fatal (error) information

/// Firmware Manager ///
#define WM_STATUSFIRMLOAD		(WM_USER+14)	// to update the progress status bar for the firmware load
#define WM_FIRMLOADSTOP			(WM_USER+15)	// notify user of completion/processed stop signal
#define WM_FIRMLOADCOMPLETE		(WM_USER+16)	// notify user of completion/processed firmware load
#define WM_FIRMLOADUPD			(WM_USER+17)	// notify user with status update information
#define WM_FIRMLOADFATAL		(WM_USER+18)	// notify user with status fatal (error) information

/// Settings Manager ///
#define WM_SETTINGSTOP			(WM_USER+19)	// notify user of completion/processed stop signal
#define WM_SETTINGCOMPLETE		(WM_USER+20)	// notify user of completion/processed settings
#define WM_SETTINGUPD			(WM_USER+21)	// notify user with status update information
#define WM_SETTINGFATAL			(WM_USER+22)	// notify user with status fatal (error) information

/// Utility/Utilities Manager ///
#define WM_UTILITYUPD			(WM_USER+23)	// notify user with Response to Command sent by user
#define WM_UTILITYFATAL			(WM_USER+24)	// notify user with status fatal (error) information

/// Database Manager ///
#define WM_STATUSDBLOAD			(WM_USER+25)	// to update the progress status bar for the bucket database load
#define WM_DBLOADSTOP			(WM_USER+26)	// notify user of completion/processed stop signal
#define WM_DBLOADCOMPLETE		(WM_USER+27)	// notify user of completion/processed firmware load
#define WM_DBLOADUPD			(WM_USER+28)	// notify user with status update information
#define WM_DBLOADFATAL			(WM_USER+29)	// notify user with status fatal (error) information


//////////////////////////////////////////////////////////////////////////////
// Defines and Structures
//////////////////////////////////////////////////////////////////////////////

#define BUFSIZE				1024		// size of the char array with the histogram filename
#define MAXCONTROLLERS		10			// maximum number of controllers (Firmware Download Mgr).

//// FSE Gantry Setup Manager ////
//// defines for operations string representation
#define CFD_STR				"C"
#define CFD_STR_LCK			"CL"		// locked CFD operation (used by GUI)
#define XY_STR				"X"
#define XY_STR_LCK			"XL"		// locked XY operation (used by GUI)
#define	BLOCK_STR			"B"
#define BLOCK_STR_LCK		"BL"		// locked Block/Setup operation	(used by GUI)
#define TIMEALIGN_STR		"T"
#define TIMEALIGN_STR_LCK	"TL"		// lock Time Alignment operation (used by GUI)
#define	ELECTRONIC_STR		"E"
#define ALL_STR				"A"
#define LOCKBLOCK_STR		"L"
#define UNLOCKBLOCK_STR		"UB"
#define PRINTMSG_STR		"PM"
#define READMSG_STR			"RM"
#define NOOP_STR			"NO"


//// FSE Gantry Setup Manager ////
//// enum representation for the cell coloring of the grid/matrix
enum FSEGantryColorMap
{
	FAIL = 0,					// Fail		= 0 (red)
	S0,							// S0		= 1 (light green)
	S123,						// S1,2,3	= 2 (yellow)
	S1XX,						// S1XX		= 3 (light blue)
	S2XX,						// S2XX		= 4 (blue)
	S3XX,						// S3XX		= 5 (magenta)
	S4XX,						// S4XX		= 6 (purple)
	S5XX,						// S5XX		= 7 (green)
	NONE						// None		= 8 (No change is color grid)
};

//// FSE Gantry Setup Manager ////
//// enum definition for the operations allowed in FSE Gantry Setup
enum FSEGantryOperation			// Declare enum type Operation
{
	CFD = 0,					// CFD = 0 
	TIMEALIGN,					// Time Alignment = 1
	ELECTRONIC,					// Electronic = 2
	XY,							// XY = 3
	BLOCK,						// Block = 4
	ALL,						// All {C, T, X, B} = 5
	LOCKBLOCKS,					// Lock Good Blocks = 6
	UNLOCKBLOCKS,				// Unlock Good Blocks = 7
	CLEARBLOCKS,				// Clear Blocks = 8
	READMSG,					// Read Messages (from blocks) = 9
	PRINTMSG,					// Print Messages (from blocks) = 10
	NOOP,						// No operation = 11
	STARTGANTRYSETUP,			// Start Gantry Setup = 12
	COMPLETEGANTRYSETUP,		// Complete Gantry Setup = 13
	EXITGANTRYSETUP,			// Exit Gantry Setup = 14
	ABORTGANTRYSETUP,			// Abort Gantry Setup = 15
	DISPALL,					// Display all operations grip = 16
	DISPCFD,					// Display CFD operation grip = 17
	DISPTIMEALIGN,				// Display Time Alignment operation grid = 18
	DISPXY,						// Display XY operation grip = 19
	DISPBLOCK,					// Display Block operation grid = 20
	EDITMODE,					// Edit Mode, display grids are disabled and operations enabled
	DISPLAYMODE,				// Display Mode, display grids are enabled and operations disabled
	INVALIDSTATE				// Invalid State wrong scanner type, total buckets or total blocks
};

//// Gantry Settings Manager ////
//// enum definition for the operations allowed in Gantry Settings
enum SettingsOperation			// Declare enum type Operation
{
	TUBEGAINS = 0,				// Tube Gains = 0 
	BUCKETTEMP,					// Bucket Temperature = 1
	RESETBUCKET,				// Reset Bucket = 2
	ZAPBUCKET,					// ZAP Bucket = 3
	BUCKETSINGLES,				// Bucket Singles = 4
	BLOCKSINGLES,				// Block Singles = 5
	WATERTEMP,					// Water Temperature = 6
	WATERSHUTDOWN,				// Water Shutdown = 7
	HIGHVOLTAGE,				// High Voltage = 8
	HIGHVOLTAGEON,				// High Voltage ON = 9
	HIGHVOLTAGEOFF,				// High Voltage OFF = 10 
	LOWVOLTAGE,					// Low Voltage = 11
	ULDENERGY,					// Upper Level Descriminator Energy = 12
	LLDENERGY,					// Low Level Descriminator Energy = 13
	SCATTERENERGY,				// Scatter Energy = 14
	CFDSETTING,					// CFD Setting = 15
	EEPROMVERSION,				// EEPROM Version = 16
	EPROMVERSION,				// EPROM Version = 17
	BUCKETSTATUS				// Bucket Status = 18
};

//// Bucket Database Manager ////
//// enum representation for the bucket database operations
enum BucketDBOperation
{
	RESTORE = 0,				// Restore (File->Gantry) = 0
	BACKUP,						// Backup (Gantry->File) = 1
	NODBOP						// No operation = 2
};


//// ALL Service QC Managers ////
//// structure that contains the information sent by the Service/QC Dll
//// to the Service/QC GUI via PostMessage(...). The Service/QC GUI gets 
//// the relevant information from the structure and displays it to the user
typedef struct tGantryResult 
{
	int		OperVal;			// int/enum value assigned to the operation (see Operation enums)
	int		iBucket;			// bucket number
	int		iBlock;				// block number
	int		iErrNum;			// error number
	int		iElapseTime;		// elapse time 
	int		iColor;				// int/enum color value to color the cell in grid/matrix (see GantryColorMap enums)
	char	StatusMsg[90];		// informational message to display the user
	char	OperChar[2];		// operation character (i.e. "C\0" for CFD, "X\0" for XY, etc.)
	char	OperDesc[20];		// operation description (i.e. TimeAlignment, CFD, etc.)
	char	CompTime[30];		// operation completion time
	int		iLogMe;				// log the message into the logfile 0=don't log otherwise log
	int		iAllowLock;			// allow locking of cell 1=allow lock otherwise do not allow cell locking
	int		iProgTime;			// progress time to update progress bars in GUI
} tGantryResult;

typedef tGantryResult *ptGantryResult;


#endif //!defined(INCLUDE_SERVQCMSGID_H)

