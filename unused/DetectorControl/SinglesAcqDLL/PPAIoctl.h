// File Name:
//		PPAIoctl.h
//
// Contents:
//		This file contains the defined IOCTL codes
//		and the Port Offset values to access the
//		ISA registers of the card installed on the PPA.
//
// Revision  History:
//		Author: J. Reed 12aug02  Rev. 1.0.
//


//++
// The IOCTL codes for reading and writing to the Rebinner ports
// This is for backward compatibility with existing software
//--
#define IOCTL_PPA_READ_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x801,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_PPA_WRITE_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x802,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_PPA_WORD_INSERT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x803,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)
//++
// The IOCTL codes for reading and writing to the Rebinner ports
// This is for backward compatibility with existing software
//--
#define IOCTL_RB_READ_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x810,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_RB_WRITE_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x811,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_RB_WORD_INSERT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x812,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)
//++
// The IOCTL codes for reading and writing to the TFA ports
// This is for backward compatibility with existing software
//--
#define IOCTL_TFA_READ_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x820,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_TFA_WRITE_PORT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x821,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

#define IOCTL_TFA_WORD_INSERT CTL_CODE(	\
					FILE_DEVICE_UNKNOWN,\
					0x822,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)


//++
// Define structures to use with the Write Port Ioctl for PPA
//--
typedef struct _PPA_WRITE_PORT {
	ULONG PortNumber ;		// Port number to write to
	USHORT Data ;			// Data to write
} PPA_WRITE_PORT, *PPPA_WRITE_PORT ;

//++
// Define structures to use with the Write Port Ioctl
// Name defined for backward compatability
//--
typedef struct _RB_WRITE_PORT {
	ULONG PortNumber ;		// Port number to write to
	USHORT Data ;			// Data to write
} RB_WRITE_PORT, *PRB_WRITE_PORT ;

//++
// Define structures to use with the Write Port Ioctl
// Name defined for backward compatability
//--
typedef struct _TFA_WRITE_PORT {
	ULONG PortNumber ;		// Port number to write to
	USHORT Data ;			// Data to write
} TFA_WRITE_PORT, *PTFA_WRITE_PORT ;

//++
// Description:
//		Rebinner Port asignments 
//		
//--
typedef struct {
	unsigned short isppr ;	// ISP programming register
	unsigned short csr	;	// Control Status Register
	unsigned short tmcr0 ;	// Time Marker Counter Register 0
	unsigned short tmcr1 ;	// Time Marker Counter Register 1
	unsigned short wir0 ;	// Word Insert Register 0
	unsigned short wir1 ;	// Word Insert Register 1
	unsigned short wir2 ;	// Word Insert Register 2
	unsigned short wir3 ;	// Word Insert Register 3
	unsigned short dr0 ;	// Diagnostic Register 0
	unsigned short dr1 ;	// Diagnostic Register 1
	unsigned short dr2 ;	// Diagnostic Register 2
	unsigned short dr3 ;	// Diagnostic Register 3
	unsigned short sr ;		// Select Register
	unsigned short fdr ;	// Flash Data Register
	unsigned short far0 ;	// Flash Address Register 0
	unsigned short far1 ;	// Flash Address REgister 1
} RB_PORTS, *PRB_PORTS;

//++
// Description:
//		TFA Port asignments 
//		
//--

typedef struct
{
	USHORT isppr ;		// ISP Programming Register
	USHORT cr ;			// Control Register
	USHORT wir0 ;		// Word Insert 0
	USHORT wir1 ;		// Word Insert 1
	USHORT wir2 ;		// Word Insert 2
	USHORT wir3 ;		// Word Insert 3
	USHORT aetcl ;		// A Elapsed Time Counter Low 16 bits
	USHORT aetch ;		// A Elapsed Time Counter High 4 bits
	USHORT betcl ;		// B Elapsed Time Counter Low 16 bits
	USHORT betch ;		// B Elapsed Time Counter High 4 bits
	USHORT apcl ;		// A Prompt Counter Low 16 bits
	USHORT bpcl ;		// B Prompt Counter Low 16 bits
	USHORT bapch ;		// B & A Prompt Counter Upper Bytes
	USHORT adcl ;		// A Delayed Counter Low 16 bits
	USHORT bdcl ;		// B Delayed Counter Low 16 bits
	USHORT badch ;		// B & A Delayed Counter Upper Bytes
} TFA_PORTS, *PTFA_PORTS ;


