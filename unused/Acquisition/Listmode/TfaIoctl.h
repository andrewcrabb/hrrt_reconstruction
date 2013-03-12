// File Name:
//		TfaIoctl.h
//
// Contents:
//		This file contains the defined IOCTL codes
//		and the Port Offset values to access the
//		TFA card.
//
// Revision  History:
//		Author: J. Reed 12nov99  Rev. 1.0.
//


//
// The IOCTL codes for reading and writing to the ports
//
#define IOCTL_TFA_READ_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x801,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_TFA_WRITE_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x802,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_TFA_WORD_INSERT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x803,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)


//
// Define the Port Offsets. The TFA has 16 bit word access ports.
//
#define TFA_PORT_BASE 0					// Base offset at zero

#define TFA_ISPPR	(TFA_BASE_PORT + 0x00)	// ISP Programming Register
#define TFA_CR		(TFA_PORT_BASE + 0x02)	// Control Register
#define TFA_WIR0	(TFA_PORT_BASE + 0x04)	// Word Insert Register 0
#define TFA_WIR1	(TFA_PORT_BASE + 0x06)	// Word Insert Register 1
#define TFA_WIR2	(TFA_PORT_BASE + 0x08)	// Word Insert Register 2
#define TFA_WIR3	(TFA_BASE_PORT + 0x0a)	// Word Insert Register 3
#define TFA_AETCL	(TFA_BASE_PORT + 0x0c)	// A Elapsed Time Counter Low 16 Bits
#define TFA_AETCH	(TFA_BASE_PORT + 0x0e)	// A Elapsed Time Counter High 4 bits
#define TFA_BETCL	(TFA_BASE_PORT + 0x10)	// B Elapsed Time Counter Low 16 Bits
#define TFA_BECTH	(TFA_BASE_PORT + 0x12)	// B Elapsed Time Counter High 4 bits
#define TFA_APCL	(TFA_BASE_PORT + 0x14)	// A Prompt Counter Low 16 bits
#define TFA_BPCL	(TFA_BASE_PORT + 0x16)	// B Prompt Counter Low 16 bits
#define TFA_BAPCH	(TFA_BASE_PORT + 0x18)	// B & A Prompt Counter High bits
#define TFA_ADCL	(TFA_BASE_PORT + 0x1a)	// A Delayed Counter Low 16 bits
#define TFA_BDCL	(TFA_BASE_PORT + 0x1c)	// B Delayed Counter Low 16 bits
#define TFA_BADCH	(TFA_BASE_PORT + 0x1e)	// B & A Delayed Counter High bits

//
// Define a structure to use with the Write Port Ioctl
//
typedef struct _TFA_WRITE_PORT {
	ULONG PortNumber ;		// Port number to write to
	USHORT Data ;			// Data to write
} TFA_WRITE_PORT, *PTFA_WRITE_PORT ;
     



