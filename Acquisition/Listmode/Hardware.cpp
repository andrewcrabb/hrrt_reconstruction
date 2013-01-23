// Hardware.cpp - This code implements the CHardware class that abstracts
//				  the ACS III hardware driver functionality.
//
//
// Created:	27feb02	-	jhr
//
//

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <iostream.h>
#include "acs3acq.h"
#include "TfaIoctl.h"


/*********************************************************************
*
*
*	CHardware::Open			- Open the specified device
*
*	IN	char* pDeviceName	- Name of device to open
*	OUT	int* err			- place to hold error code on failure	
*
*	Returns:				- HANDLE on success or NULL on failure
*							  On failure err will contain Win32
*							  specific error code
*
**********************************************************************/

HANDLE
CHardware::Open (char* pDeviceName, int* err)
{
	HANDLE hDev ;

	// try to create the device
	hDev = CreateFile (
					pDeviceName, 
        			0, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        			NULL, OPEN_EXISTING, 0, NULL
        			) ;

	// set up the return
	if (hDev == INVALID_HANDLE_VALUE)
	{
		*err = (int)GetLastError () ;
		cout << "LMInit: CHardware::Open - Could not open " << pDeviceName << " error " << *err << endl ;
		return (FALSE) ;
	}

	m_Handle = hDev ;
	return (hDev) ;
}
	
/*********************************************************************
*
*
*	CHardware::ReadPort		- Read a port (Rebinner or TFA)
*
*	IN	long ioctl_code		- I/O control code
*	IN	unsigned long reg	- register to read
*	OUT	int* val			- where to return value	
*
*	Returns:				- tue if suucess or false if fail
*
**********************************************************************/

BOOL
CHardware::ReadPort (long ioctl_code, unsigned long reg, int* val)
{
	int err ;
    unsigned short data = 0;

	if (!IoControl (ioctl_code, &reg, sizeof(reg), &data, sizeof (data), &err)) return (FALSE) ;

	*val = (int)data ;
	return (TRUE) ;
}

/*********************************************************************
*
*
*	CHardware::WritePort	- Write a value to a port (Rebinner or TFA)
*
*	IN	long ioctl_code		- I/O control code
*	IN	unsigned long reg	- register to read
*	IN  unsigned short val	- value to write
*
*	Returns:				- tue if suucess or false if fail
*
**********************************************************************/

BOOL
CHardware::WritePort (long ioctl_code, unsigned long reg, unsigned short val)
{
	int err ;
	TFA_WRITE_PORT wp ;

	wp.PortNumber = reg ;
	wp.Data = val ;

	return (IoControl (ioctl_code, &wp, sizeof(wp), &val, sizeof (unsigned short), &err)) ;
}

/*********************************************************************
*
*
*	CHardware::WordInsert	- Insert a 64 bit word through the word
*							  insertion register
*	
*	IN	HANDLE hDev			- HANDLE to the open device
*	OUT (NONE)
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CHardware::WordInsert (__int64 WVal)
{
	int err ;

	// insert the value
	return (IoControl (IOCTL_WORD_INSERT, &WVal, sizeof(__int64), NULL, 0, &err)) ;
}

/*********************************************************************
*
*
*	CHardware::Close	- Close the device
*	
*	IN	HANDLE hDev		- HANDLE to the open device
*	OUT (NONE)
*
*	Returns:			- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CHardware::Close (HANDLE hDev)
{
	if (!CloseHandle (hDev)) return (FALSE) ;
	else return (TRUE) ;
}

/*********************************************************************
*
*
*	CHardware::IoControl	- Performs a DeviceIoControl call to the specified device
*
*	IN	int code			- I/O Control Code
*	IN	void* in			- pointer to input data
*	IN	int inlen			- length of input data
*	OUT	void* out			- where to store output data
*	IN	outlen				- length of output data
*	OUT int* err			- Win32 error code if failure
*
*	Returns:				- TRUE if successful or FALSE on Failure
*							  err contains error code on failure
*
**********************************************************************/

BOOL
CHardware::IoControl (int code, void* in, int inlen, void* out, int outlen, int* err)
{
	DWORD nbytes ;

	//cout << "IoControl: device handle = " << m_Handle << "..." << endl ;
	if (!m_Handle) return (FALSE) ;

	// make the control call
	BOOL IoResult = DeviceIoControl (
							m_Handle,				// handle to device
							(DWORD)code,			// I/O controll code
							in,						// pointer to input data
							(DWORD)inlen,			// input length
							out,					// pointer to output
							(DWORD)outlen,			// output buffer size
							&nbytes,				// number of bytes returned
							(LPOVERLAPPED)NULL ) ;	// No overlap

	// set up the return
	if (!IoResult)
	{
		*err = (int)GetLastError () ;
		cout << "IoControl: device handle = " << m_Handle << " error = " << *err << endl ;
		return (FALSE) ;
	}

	return (TRUE) ;
}

