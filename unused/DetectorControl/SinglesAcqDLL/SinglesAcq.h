// SinglesAcq.h - Include file singles acquisition
//

#define MAX_FILENAME	128

typedef struct {
	char* fname ;
	int makefile ;
	int preset_time ;
	int verbose ;
	int debug ;
	int passall ;
	int lut ;
	int timeout ;
} ACQ_ARGS ;

// base class for listmode acquisition through Pft
class CPft {
public:
	 CPft(void) {m_hPft = NULL;} ;
	 ~CPft(void) {CloseHandle (m_hPft);} ;

	// public methods
	bool Open () ;														// open the device
	bool Close() ;														// close the device
	bool WrtPftReg (unsigned long offset, unsigned long val) ;			// write to the PFT register space
	bool WrtPlxReg (unsigned long offset, unsigned long val) ;			// write to the PLX register space
	bool RdPftReg  (unsigned long reg, unsigned long & val) ;			// read from the PFT register space
	bool RdPlxReg  (unsigned long reg, unsigned long & val) ;			// read from the PLX register space
	unsigned long* GetBuffer () ;										// get the user mapped buffer
	unsigned long GetLogical () ;										// get the logical address of the buffer
	bool ReleaseBuffer () ;												// release the user mapped buffer
	BOOL Stop () ;														// Stop the acquistion
	BOOL IoControl (int code, void* in, int inlen, void* out, int outlen) ; // perform a DeviceIoControl operation
private:
	HANDLE m_hPft ;														// Handle to write to
} ;

class CRebinner {
public:
	BOOL SetupRebinner (int passall, int lut) ;						// word insertion
private:
} ;

// Listmode Acquisition class
class CSinglesAcq : public CPft {
public:
	int m_quiet;
	int m_debug;
	int m_makefile ;
	CSinglesAcq(void) {} ;
	~CSinglesAcq (void) {} ;

	// acquire singles data
	bool Acquire (ACQ_ARGS* args) ;
	bool StopAcq () ;
	unsigned long ComputeScanTime () ;

	// got to make these public because of the threads
	HANDLE m_hfile ;													// handle to the file
	HANDLE m_presettimer;												// handle to preset timer
	HANDLE m_timeouttimer;												// handle to the timeout timer
	HANDLE m_statstimer ;												// handle to stats poll timer
	HANDLE m_hscancomplete;												// handle to the scan event
	LARGE_INTEGER m_statspolltime ;										// stats poll time
	unsigned long m_timeremaining  ;									// time remaining in scan
	unsigned long m_preset ;											// preset time in seconds
	unsigned long m_timeout ;											// timeout timer in seconds
	bool m_status ;
	char m_fname[MAX_FILENAME] ;										// filename
private:
	// methods
	bool StartTimerCallbackThreads ();									// spin the threads timers will notify
	bool StartPresetTimer();											// start the preset timer
	bool StartStatsPoll();												// start the preset timer
	BOOL StartAcquisition(int passall, int lut);						// start data acquisition
	void ConvertTime (unsigned long seconds, LARGE_INTEGER* timertime);	// convert seconds to system timer time
	bool CreateListmodeFile ();											// create a file if necessary

	// member variables
	LARGE_INTEGER m_timertime ;											// system timer time
	LARGE_INTEGER m_Frequency ;

	// Things needed for stats computations
	DWORD m_StartTime ;
	__int64 m_LastCounts ;
	__int64 m_LastTime ;

	// high resolution timers
	LARGE_INTEGER m_ScanStart ;
	LARGE_INTEGER m_LastTimeCount ;
} ;

// Structure for passing write register through the IoControl
typedef struct {
	unsigned long offset ;
	unsigned long data ;
} REGWRITE ;

