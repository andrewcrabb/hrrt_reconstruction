// lmioctl.h - include file to define structures and
//				 ioctl codes for the Systran SLDC driver
//				 along with other constants
//

//
// IO Control structures
//

#ifndef _LMIOCTL_H_
#define _LMIOCTL_H_


// for retrieving acquisition info
typedef struct _LM_ACQ_INFO {
	ULONG acq_status;			// acquisition status
	LARGE_INTEGER total_events; // current total events
	LARGE_INTEGER total_time;	// total time in 100 nsec intervals
	ULONG nfifo_overflows;
	ULONG nbufrs_missed;
} LM_ACQ_INFO, *PLM_ACQ_INFO;


typedef struct _LM_INFO_resp {
	int acq_status;
	LARGE_INTEGER counts;
	int cps;
	int time_remaining;
	__int64 total_prompts;
	__int64 total_randoms;
	ULONG nfifo_overflows;
	ULONG nbufrs_missed;
	BOOLEAN histo_write_pending;
} LM_INFO_resp, *PLM_INFO_resp;

// for writing values to the V3 chip (for testing and debugging only)
typedef struct _V3_POKE {
	ULONG offset;				// offset into register space
	ULONG val;					// value to write
	ULONG size;					// size 1, 2, or 4 bytes
} V3_POKE, *PV3_POKE;

// for reading values from the V3 chip (for testing and debugging only)
typedef struct _V3_PEEK {
	ULONG offset;				// offset into register space
	ULONG size;					// size 1, 2, or 4 bytes
} V3_PEEK, *PV3_PEEK;

// structure to pass parameters for the IOCTL_SLDC_BUFR_CLR_WAIT ioctl 
typedef struct _BUFR_CLR_WAIT {
	ULONG wait_bnum;			// buffer number to wait for completion
	ULONG clr_bnum;				// buffer number to tag as clear
} BUFR_CLR_WAIT, *PBUFR_CLR_WAIT;

// returned structure from the IOCTL_SLDC_BUFR_CLR_WAIT ioctl
typedef struct _BUFR_CLR_WAIT_RET {
	ULONG acq_status;			// acquisition status
	ULONG nevents;				// number of events in buffer
} BUFR_CLR_WAIT_RET, *PBUFR_CLR_WAIT_RET;

//
// define some IO Control Codes and structures
//

// Io code for reading the PCI space
#define FX_GET_PCI_SPACE 801

#define IOCTL_GET_PCI_SPACE CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,	\
				FX_GET_PCI_SPACE,		\
				METHOD_BUFFERED,		\
				FILE_ANY_ACCESS )

// Io code to return the slot number of the device
#define FX_GET_PCI_SLOT  802

#define IOCTL_GET_PCI_SLOT CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,	\
				FX_GET_PCI_SLOT,		\
				METHOD_BUFFERED,		\
				FILE_ANY_ACCESS )

// Io code to get mapped address of locked buffer
#define FX_GET_MAPPED_BUFFER 803

#define IOCTL_SLDC_GET_MAPPED_BUFFER CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,		\
				FX_GET_MAPPED_BUFFER,		\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )

// Io code to initialize the Fx SLDC card
#define FX_SLDC_INIT 804

#define IOCTL_SLDC_INIT CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,\
				FX_SLDC_INIT,		\
				METHOD_BUFFERED,	\
				FILE_ANY_ACCESS )

// Io code to get the CSR from SLDC card
#define FX_SLDC_GET_CSR 805

#define IOCTL_SLDC_GET_CSR CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,	\
				FX_SLDC_GET_CSR,		\
				METHOD_BUFFERED,		\
				FILE_ANY_ACCESS )


// Io code to read data to buffer 0
#define FX_SLDC_READ_BUFR_0 806

#define IOCTL_SLDC_READ_BUFR_0 CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,		\
				FX_SLDC_READ_BUFR_0,		\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )


// Io code to read data to buffer 1 (no longer used - use read buffer 0 to start acq.)
#define FX_SLDC_READ_BUFR_1 807

#define IOCTL_SLDC_READ_BUFR_1 CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,		\
				FX_SLDC_READ_BUFR_1,		\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )


// Io code to reset SLDC receiver
#define FX_SLDC_RECV_RESET 808

#define IOCTL_SLDC_RECV_RESET CTL_CODE(		\
				FILE_DEVICE_UNKNOWN,		\
				FX_SLDC_RECV_RESET,			\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )

// Io code to and tag a buffer as clear then wait for a buffer to fill
#define FX_BUFR_WAIT_CLR 809

#define IOCTL_SLDC_BUFR_CLR_WAIT CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,		\
				FX_BUFR_WAIT_CLR,			\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )


// Io code to halt acquisition
#define FX_SLDC_HALT 810

#define IOCTL_SLDC_HALT CTL_CODE(		\
				FILE_DEVICE_UNKNOWN,	\
				FX_SLDC_HALT,			\
				METHOD_BUFFERED,		\
				FILE_ANY_ACCESS )


// Io code to halt acquisition
#define FX_SLDC_UNMAP_BUFFER 811

#define IOCTL_SLDC_UNMAP_BUFFER CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,		\
				FX_SLDC_UNMAP_BUFFER,		\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )


#define FX_SLDC_LM_ACQ_INFO 812

#define IOCTL_SLDC_LM_ACQ_INFO CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,		\
				FX_SLDC_LM_ACQ_INFO,		\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )

#define FX_SLDC_RESET_ERROR_CNT 813

#define IOCTL_SLDC_RESET_ERROR_CNT CTL_CODE(\
				FILE_DEVICE_UNKNOWN,		\
				FX_SLDC_RESET_ERROR_CNT,	\
				METHOD_BUFFERED,			\
				FILE_ANY_ACCESS )

// Test IO control codes
// These are test IOCTL's for deugging
// IOCTL codes will start at 1000
#define FX_SLDC_GEN_INTR 1000

#define IOCTL_SLDC_GEN_INTR CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,	\
				FX_SLDC_GEN_INTR,		\
				METHOD_BUFFERED,		\
				FILE_ANY_ACCESS )


#define FX_SLDC_V3_POKE 1001

#define IOCTL_SLDC_V3_POKE CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,	\
				FX_SLDC_V3_POKE,		\
				METHOD_BUFFERED,		\
				FILE_ANY_ACCESS )


#define FX_SLDC_V3_PEEK 1002

#define IOCTL_SLDC_V3_PEEK CTL_CODE(	\
				FILE_DEVICE_UNKNOWN,	\
				FX_SLDC_V3_PEEK,		\
				METHOD_BUFFERED,		\
				FILE_ANY_ACCESS )

//
// Acquisition status definitions
//

#define ACQ_STOPPED		0x0
#define ACQ_ACTIVE		0x1
#define ACQ_PAUSED		0x2

#endif _LMIOCTL_H_

