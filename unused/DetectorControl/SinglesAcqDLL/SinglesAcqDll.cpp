// SinglesAcqDLL.cpp - Code that contains main DLL export routine
//					   and acquisition control for collecting singles data
//
// Revision History:
//
// created:	jhr - 12dec02
//
//

// The compiler needs this
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <winbase.h>
#include <winioctl.h>
#include <iostream.h>
#include <stdio.h>
#include "SinglesAcq.h"
#include "PftIoctl.h"

#define SINGLESACQDLLAPI __declspec(dllexport)
#include "SinglesAcqDLL.h"

#define LOCAL static

// Some LOCAL definitions
LOCAL int Makefile = false ;
LOCAL int Verbose = false ;
LOCAL int Debug = false ;
LOCAL int RebLut = 0 ;
LOCAL char fname[256] ;

// The SinglesAcq object instantiation
CSinglesAcq sacq ;

//++
//
// SetSAcqEnv - Setup the environment for the DLL to run in
//				Debug, Verbose Mode, etc...
//
// Inputs : int PresetTime
//			int RebinnerMode
//			int TimeOut
//
// Outputs : bool, false = fail, true = success
//
//--

void
SetSAcqEnv (char* Filename, int verbose, int debug, int lut)
{
	if (Filename)
	{
		strcpy (fname, Filename) ;
		Makefile = true ;
	}
	else
	{
		fname[0] = '\0' ;
		Makefile = false ;
	}

	Verbose = verbose ;
	Debug = debug ;
	RebLut = lut ;
}

//++
//
// AcquireSingles - Acquire panel detector singles
//
// Inputs : int PresetTime
//			int RebinnerMode
//			int TimeOut
//
// Outputs : bool, false = fail, true = success
//
//--

int
AcquireSingles (int PresetTime, int RebinnerMode, int TimeOut)
{
	// The acquisition args
	ACQ_ARGS args ;
	
	// Load the args structure
	args.fname = fname ;
	args.makefile = Makefile ;
	args.preset_time = PresetTime ;
	args.verbose = Verbose ;
	args.debug = Debug ;
	args.passall = RebinnerMode ;
	args.lut = RebLut ;
	args.timeout = TimeOut ;

	// Go do the acquisition
	return (sacq.Acquire (&args)) ;
}

//++
//
// StopAcq - Stop the singles acquisition
//
// Inputs : None
//
// Outputs: None
//
// Return : false = fail, true = success
//
//--

int
StopAcq ()
{
	return (sacq.StopAcq ()) ;
}



