//==========================================================================
// File			:	ServMgrCommon.h
//					Copyright 1999 by CTI
// Description	:	Contains the defines, window message ids, and common 
//					information used by the Service Manager dialogs
//	
// Author		:	Selene F. Tolbert
//
// Date			:	19 April 2001
//
// Author		Date			Update
// S Tolbert	19 April 01		Created
// S Tolbert	15 August 01	Added guards and renamed file from AnalyzerDefines
//								to ServMgrCommon
// S Tolbert	17 August 01	Eliminated window message id's and moved them
//								to the ServQCMsgID.h file
// S Tolbert	05 Sept 01		Corrected the naming of the IDL file (IDLDefines.cmn)
// S Tolbert	08 Oct 01		Added INVALID_STATE in the AnalyzerInfo enums to 
//								handle failure in scanner acquisition data and set
//								the GUI in a disabled state. Only exit allowed.
// S Tolbert	23 Oct 01		Added #include of the the Export.h file to include 
//								the Callable/External IDL exports used by the Settings
//								Manager (Bucket Singles Operation) and the ServMgrView
//								(Sinogram Mananger/Viewer) in order to prevent multiple
//								inlcusions of the header file.
// S Tolbert	14 Feb 02		Added Retain defines for the IDL widget to prevent it from
//								blackout. 
//==========================================================================

// Set guards to prevent multiple header file inclusion
//
#if !defined(INCLUDE_SERVMGR_COMMONDEF_H)
#define INCLUDE_SERVMGR_COMMONDEF_H

// Include the Callable/External IDL Export File
#include "export.h"				


//// Analyzer (Histogram) Dialog ////

// NOTE: These defines are being used by the positionprofile.pro IDL procedure
// representation for the position profile filtering  (See IDLDefines.cmn)
#define NO_OP		50
#define PP_PEAKS	60
#define PP_BOUNDRY	70
#define PP_FILTER	80

// Backing store of the draw widget (1=window system, 2=idl)
#define RETAIN_1	 1		
#define RETAIN_2	 2		

// NOTE: These enums are being used by the printhistogram.pro IDL procedure
// enum representation for the Histogram Types (See IDLDefines.cmn)
enum AnalyzerInfo
{
	POS_PROFILE = 0,			// Histogram - Position Profile = 0 
	POS_PROFILE_PEAKS,			// Histogram - Position Profile Peaks = 1
	POS_PROFILE_BOUNDRY,		// Histogram - Position Profile Boundry = 2
	POS_PROFILE_FILTER,			// Histogram - Position Profile Filter = 3
	POS_SCATTER_PROFILE,		// Histogram - Position Scatter Profile = 4 
	CRYSTAL_ENERGY_PROFILE,		// Histogram - Crystal Energy Profile = 5
	TUBE_ENERGY_PROFILE,  		// Histogram - Tube Energy Profile = 6
	TIME_HIST_PROFILE,			// Histogram - Time Histogram Profile = 7
	HIST_ACQUIRE,				// Action - Histogram Acquire (Acquire button) = 8
	HIST_PRINT,					// Action - Print histogram from IDL DrawWidget (Print button) = 9
	HIST_CLEAR,					// Action - Clear histogram from IDL DrawWidget (Clear button) = 10
	STOP_HIST,					// Action - Stop histogram Acquisition (Stop button) = 11
	FETCH_MORE_HIST,			// Action - Fetch more histogram data (Fetch More button) = 12
	INVALID_STATE				// Invalid State wrong scanner type, total buckets or total blocks			
};

// structure that holds the firmware controllers obtained from the gantry model object 
typedef struct tControllerModelInfo
{
	int		iContrlID;			// controller ID
	int		iContrlQty;			// controller quantity
	int		iStartAddr;			// controller(s) starting address (if -1 = Not Defined Address)
	char	ContrlDesc[90];		// controller text description
	char	ContrlFirmFile[90];	// controller firmware file name
}tControllerModelInfo;

#endif //!defined(INCLUDE_SERVMGR_COMMONDEF_H)
