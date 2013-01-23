#ifndef SinoInfo_h
#define SinoInfo_h

// a class for exchanging sinogram related information between LM and HIST
class CSinoInfo {
public:
	UINT fileType;	// 0: undefined, 1: LM, 2: SINO
	SYSTEMTIME scanStartTime;
	SYSTEMTIME scanStopTime;
	ULONG presetTime;	// in sec. Intended scan duration
	ULONG scanDuration; // in sec. Calculated from starTime/stopTime
	UINT gantry;		// 0: HRRT, 1: E.CAM, 2: PETCT
	char gantryModel[32];	// e.g., 1062, 1023, biograph
	UINT scanType;		// 0: em  1: tx  2: em/tx
	UINT span;
	UINT ringDifference;
	UINT lld;
	UINT uld;
	UINT headPasses;
	float bedInOut;		// in cm. outdated.
	float bedInOutStart;	// in cm
	float bedInOutStop;	// in cm
	float bedHeight; // in cm
};

enum EN_Gantry {
		GNT_HRRT = 0, 
		GNT_ECAM, 
		GNT_PETCT, 
		GNT_P39
};



#endif
