//------------------------------------------------------------------------
// File			:	ErrorEventSupport.h
//					Copyright 2002 by CPS
// Description	:	Defines the ErrorEventSupport support class used to
//					fire (log) error messages in the ARS system. 
//	
// Author		:	Rick K. Camp
//
// Date			:	10-May-2002
//
// Author		Date			Update
//------------------------------------------------------------------------
// R Camp		10-May-02		Created
//------------------------------------------------------------------------

#ifndef ERROREVENTSUPPORT_CLASS_H
#define ERROREVENTSUPPORT_CLASS_H

#include "ErrorEvents.h"					// ErrorEvent class (COM+ event)

#ifdef ERREVTSUP_EXPORTS
#define ERREVTSUP_API __declspec(dllexport)
#else
#define ERREVTSUP_API __declspec(dllimport)
#endif

#define HRESULT_CODE(hr) ((hr) & 0xFFFF)	// Parses actual HRESULT codes
#define NO_SUBSCRIBER_FOR_EVENT		514
#define RPC_SERVER_UNAVAILABLE		1722
#define CLASS_NOT_REGISTERED		340
#define CANNOT_INVOKE_SUBSCRIBER	0x40202

#define NUM_APPLICATION_NAMES		19		// No. of valid application names
#define	LOGFILE_SPEC_SIZE			256		// Logfile spec buffer size
#define	MAX_APPL_NAME_SIZE			32
#define INTERNAL_ERR_STR_SIZE		200
#define MAX_RAW_DATA_SIZE			2048	// "Raw data array" size for NewErrorEvent()

// Application error groupings MUST be added here in order to define
// error messages for the groups (in the ArsEventMsgDefsApp app).
// The ErrorEventSupport "Fire" method validates that the specified
// error information includes a valid error message group name.
static char* validApplicationNames[NUM_APPLICATION_NAMES] =
{
	"ARS_LIS",						// Listmode app
	"ARS_HST",						// Histogram app
	"ARS_CP",						// Coincidence processor
	"ARS_GAN",						// Gantry control
	"ARS_MTR",						// Motor control
	"ARS_REC",						// Reconstruction
	"ARS_PHS",						// Patient handling system
	"ARS_LHS",						// Listmode Histogram Server app
	"ARS_CHL",						// Chiller control for Gantry
	"ARS_DSP",						// Display control for Gantry
	"ARS_SIM",						// GCI simulator
	"ARS_LCK",						// ACS Lock Manager
	"ARS_DAL",						// DAL server (detector abstraction layer)
	"ARS_PDS",						// Panel Detector Setup server
	"ARS_DSU",						// Detector Setup Utilities server
	"ARS_SAQ",						// Singles Acquisition server
	"ARS_DSG",						// Detector Setup GUI
	"ARS_DUG",						// Detector Utilities GUI
	"ARS_QCG"						// Detector Quality Check GUI
};

//-----------------------------------------------------------------------
// CErrorEventSupport class definition:
//    This class supports the publishing of "ErrorEvent" COM+ events.
//-----------------------------------------------------------------------

class ERREVTSUP_API CErrorEventSupport
{

private:
	bool m_InitCOM;							// "Init COM" flag from user
	bool m_InitCOMSecurity;					// "Init COM Security" flag
	bool m_InitComplete;					// Set by constructor method
	HRESULT m_InitHResult;					// Stored HRESULT during init
	FILE* mp_ErrEvtSupFile;					// For debug log file
	IErrorEvents* m_ErrEvt;					// ErrorEvents interface obj
	char m_logFileSpec[LOGFILE_SPEC_SIZE];	// Log file specification text

	void RptInternalError( char* InternalErrStr );
	void SetLogfileSpec( void );

public:

	CErrorEventSupport();				// Constructor (no args)
	CErrorEventSupport( bool InitCOM );
	CErrorEventSupport( bool InitCOM, bool InitComSecurity );
	~CErrorEventSupport();				// Destructor

	bool Init();						// For COM init, debug file setup
	bool InitComplete( HRESULT hr );	// Simply returns init status

	HRESULT Fire( 
		unsigned char* cApplicationName,// ARS application name
		int iErrorCode,					// Actual error code being reported
		unsigned char* cAddlText,		// Additional text for error msg
		long lRawDataSize,				// No. of bytes in RawData arg
		unsigned char* cRawData,		// Raw binary data array
		char bLogIt );					// If TRUE, write error to database

};

#endif

