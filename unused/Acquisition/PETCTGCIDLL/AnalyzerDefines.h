//==========================================================================
// File			:	AnalyzerDefines.h
//					Copyright 1999 by CTI
// Description	:	Contains the Histogram Type information used by the 
//					Analyzer class (Analyzer.h, Analyzer.cpp)
//	
// Author		:	Selene F. Tolbert
//
// Date			:	19 April 2001
//
// Author		Date		Update
// S Tolbert	19 April	Created
//
//==========================================================================

// NOTE: These defines are being used by the positionprofile.pro IDL procedure
// representation for the position profile filtering  (See IDLDefines.pro)
#define NO_OP		50
#define PP_PEAKS	60
#define PP_BOUNDRY	70
#define PP_FILTER	80

// NOTE: These enums are being used by the printhistogram.pro IDL procedure
// enum representation for the Histogram Types (See IDLDefines.pro)
enum AnalyzerInfo
{
	POS_PROFILE = 0,			// Histogram - Position Profile = 0 
	POS_PROFILE_PEAKS,			// Histogram - Position Profile Peaks = 1
	POS_PROFILE_BOUNDRY,		// Histogram - Position Profile Boundry = 2
	POS_SCATTER_PROFILE,		// Histogram - Position Scatter Profile = 3 
	CRYSTAL_ENERGY_PROFILE,		// Histogram - Crystal Energy Profile = 4
	TUBE_ENERGY_PROFILE,  		// Histogram - Tube Energy Profile = 5
	TIME_HIST_PROFILE,			// Histogram - Time Histogram Profile = 6
	HIST_ACQUIRE,				// Action - Histogram Acquire (Acquire button) = 7
	HIST_PRINT,					// Action - Print histogram from IDL DrawWidget (Print button) = 8
	HIST_CLEAR,					// Action - Clear histogram from IDL DrawWidget (Clear button) = 9
	STOP_HIST,					// Action - Stop histogram Acquisition (Stop button) = 10
	FETCH_MORE_HIST				// Action - Fetch more histogram data (Fetch More button) = 11
};
