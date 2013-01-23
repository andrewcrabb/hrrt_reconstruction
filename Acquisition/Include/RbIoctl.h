// File Name:
//		RbIoctl.h
//
// Contents:
//		This file contains the defined IOCTL codes
//		and the Port Offset values to access the
//		Rebinner card.
//
// Revision  History:
//		Author: J. Reed 17mar98  Rev. 1.0.
//


//
// The IOCTL codes for reading and writing to the ports
//
#ifndef RbIoctl_h
#define RbIoctl_h
#define IOCTL_RB_READ_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x801,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_RB_WRITE_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x802,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_RB_WORD_INSERT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x803,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)


//
// Define the Port Offsets. The Rebinner has 16 bit word access ports.
//
#define RB_PORT_BASE 0					// Base offset at zero

#define RB_ISPPR	(RB_PORT_BASE + 0x00)	// ISP Programming Register
#define RB_CR		(RB_PORT_BASE + 0x02)	// Control Register
#define RB_TMCR0	(RB_PORT_BASE + 0x04)	// Time Marker Counter Register 0
#define RB_TMCR1	(RB_PORT_BASE + 0x06)	// Time Marker Counter Register 1
#define RB_WIR0		(RB_PORT_BASE + 0x08)	// Word Insert Register 0
#define RB_WIR1		(RB_PORT_BASE + 0x0a)	// Word Insert Register 1
#define RB_WIR2		(RB_PORT_BASE + 0x0c)	// Word Insert Register 2
#define RB_WIR3		(RB_PORT_BASE + 0x0e)	// Word Insert Register 3
#define RB_DR0		(RB_PORT_BASE + 0x10)	// Diagnostic Register 0
#define RB_DR1		(RB_PORT_BASE + 0x12)	// Diagnostic Register 1
#define RB_DR2		(RB_PORT_BASE + 0x14)	// Diagnostic Register 2
#define RB_DR3		(RB_PORT_BASE + 0x16)	// Diagnostic Register 3
#define RB_SR		(RB_PORT_BASE + 0x18)	// Select Register
#define RB_FDR		(RB_PORT_BASE + 0x1a)	// Flash Data Register
#define RB_FAR0		(RB_PORT_BASE + 0x1c)	// Flash Address Register 0
#define RB_FAR1		(RB_PORT_BASE + 0x1e)	// Flash Address REgister 1

//
// Define a structure to use with the Write Port Ioctl
//
typedef struct _RB_WRITE_PORT {
	ULONG PortNumber ;		// Port number to write to
	USHORT Data ;			// Data to write
} RB_WRITE_PORT, *PRB_WRITE_PORT ;
     
#endif


