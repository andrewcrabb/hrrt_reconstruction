// acs3acq.h - This include defines the classes to abstract the ACS3 components
//			   to a listmode acquisition environment.
//
//
// created:	21feb02 - jhr
//
//

#ifndef _INCLUDE_ACS3ACQ_H_
#define _INCLUDE_ACS3ACQ_H_

#include "lmioctl.h"
#include "PftIoctl.h"

// I/O Control code for word insertion
#define IOCTL_WORD_INSERT CTL_CODE(		\
					FILE_DEVICE_UNKNOWN,\
					0x803,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

// Status definitions
#define STATUS int							// create STATUS definition
#define SUCCESS 1							// Medcom return convention for success
#define FAIL	0							// Medcom return convention for fail

// Rebinner tag word defines
#define RbReset		0x8000fc0040000001 ;	// Rebinner Reset Control Tag word
#define RbTag		0x8000000040000000 ;	// Rebinner Tag Word
#define RbCTW		0x8000fc0040000000 ;	// Rebinner Control Tag word
#define RbPassall	0x8000fc0040000004 ;	// Rebinner Pass All Control Tag word

// Timer conversion constant define
#define nNsecPerSec	10000000 ;

// Polling time intervals
#define SINGLES_UPDATE_TIME	2				// 2 seconds for singles polling
#define STATS_UPDATE_TIME	2				// 2 seconds for statistics update



// base class for generic hardware interface
class CHardware {
public:
	CHardware (void) { m_Handle = m_pMappedBuffer = NULL ; }
	~CHardware () {} ;

	// deals with the Win32 driver API
	HANDLE Open (char* pDeviceName, int* err) ;
	BOOL Close (HANDLE hDev) ;
	BOOL IoControl (int code, void* in, int inlen, void* out, int outlen, int* err) ;
	BOOL WordInsert (__int64 WVal) ;

	// functions for Rebinner and TFA
	BOOL ReadPort (long ioctl_code, unsigned long reg, int* val) ;
	BOOL WritePort (long ioctl_code, unsigned long reg, unsigned short val) ;

	// virtual functions that can be overridden for Fibre Channel harware
	virtual BOOL Acquire ()				{return (false) ;}
	virtual BOOL Stop ()				{return (false) ;}
	virtual BOOL Reset ()				{return (false) ;}
	virtual BOOL GetMappedBuffer ()		{return (false) ;}
	virtual BOOL UnmapBuffer ()			{return (false) ;}
	virtual BOOL GetInfo (void* Info)	{return (false) ;}
	virtual BOOL SendNotificationHandle (HANDLE* ReadyEvent) {return (false) ;}

	// only for PFT Fibre Channel hardware
	virtual BOOL TagBufferFree ()		{return (false) ;}

	// only for Systran Fibre Channel hardware
	virtual BOOL BufferClearWait (BUFR_CLR_WAIT* bcw, BUFR_CLR_WAIT_RET* bcwr) {return (false) ;}

	// Member variables used through the DLL
	HANDLE m_Handle ;
	PULONG m_pMappedBuffer ;
private:
protected:
} ;

// TFA declaration - just so it will have a name
class CTfa : public CHardware {
public:
private:
} ;

// Rebinner declaration
class CRebinner : public CHardware {
public:
private:
} ;

// PFT declaration - just so it will have a name
class CPft : public CHardware {
public:
	CPft (void) {} ;
	~CPft () {} ;

	// base class virtual function overrides
	BOOL Reset () ;
	BOOL GetMappedBuffer () ;
	BOOL Acquire () ;
	BOOL TagBufferFree () ;
	BOOL GetInfo (void* Info) ;
	BOOL Stop () ;
	BOOL UnmapBuffer () ;
	BOOL SendNotificationHandle (HANDLE* ReadyEvent) ;
} ;

// Systran SLDC declaration
class CSldc : public CHardware {
public:
	CSldc (void) {} ;
	~CSldc () {} ;

	// base class virtual function overrides
	BOOL Reset () ;
	BOOL GetMappedBuffer () ;
	BOOL Acquire () ;
	BOOL GetInfo (void* Info) ;
	BOOL Stop () ;
	BOOL UnmapBuffer () ;
	BOOL BufferClearWait (BUFR_CLR_WAIT* bcw, BUFR_CLR_WAIT_RET* bcwr) ;
} ;

// Acquisition class declaration
class CAcquisition {
public:
	// Constructor
	CAcquisition (void) ;
	// public methods
	// time routines
	void ConvertTime	(unsigned long seconds, LARGE_INTEGER* timertime) ;
	int ComputeScanTime () ;

	// rebinner control routines
	void ResetRebinner	(int lut, int bypass) ;
	bool SyncRebinner	(int lut, int bypass) ;

	// file handling
	bool CreateLmFile	() ;
	bool ReopenLmFile	() ;
	void CloseLmFile	() ;
	BOOL WriteLmFile	(LPCVOID address, DWORD nbytes, DWORD* bytes_written, int* err) ;

	// device object pointers
	CHardware *pFcd ;
	CHardware *pFcd2 ;
	CHardware *pFcd3 ;
	CHardware *pReb ;
	CHardware *pTfa ;

	// timers
	HANDLE hPresetTimer ;
	HANDLE hStatsTimer ;
	HANDLE hSinglesTimer ;

	// synchronization events
	HANDLE hStartAcqEvent ;
	HANDLE hSinglesPollEvent ;

	// Acquisition state
	int LmState ;

	// Storage file for Raw Listmode
	char LmFile[128] ;

	// Scan control members
	unsigned long	LmPreset  ;
	int				LmTime ;
	int				LmOnline ;
	unsigned long	LmBypass ;
	int				LmStopCondition ;
	bool			LastHistoBufr ;

	// Things needed for stats computations
	DWORD m_StartTime ;
	__int64 m_LastCounts ;
	__int64 m_LastTime ;

	// high resolution timers
	LARGE_INTEGER m_ScanStart ;
	LARGE_INTEGER m_LastTimeCount ;

	// indicates Systran card is device
	bool m_Systran ;
private:
	HANDLE m_hFile ;
	LARGE_INTEGER m_Frequency ;
} ;

// Acquisition state definitions
enum State {UNINITIALIZED, INITIALIZED, CONFIGURED, ACQUIRE, STOPPED} ;


#endif _INCLUDE_ACS3ACQ_H_