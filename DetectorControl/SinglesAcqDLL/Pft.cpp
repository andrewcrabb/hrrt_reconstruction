// Pft.cpp - CPft class implementation
//

#define _WIN32_WINNT	0x0400

#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include "SinglesAcq.h"
#include "PftIoctl.h"

//++
//
// CPft::Open - Open the Pft device
//
// Inputs : NONE
//
// Outputs : bool, false = fail, true = success
//
//--

bool CPft::Open ()
{
	HANDLE h ;
	
	h = CreateFile (
				"\\\\.\\pft1", 
        		0, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        		NULL, OPEN_EXISTING, 0, NULL
        		) ;

	if (h != INVALID_HANDLE_VALUE)
	{
		CPft::m_hPft = h ;
		return true ;
	}
	else
	{
		m_hPft = NULL ;
		return false ;
	}
}

//++
//
// CPft::Close - Close the Pft device
//
// Inputs : NONE
//
// Outputs : bool, false = fail, true = success
//
//--

bool CPft::Close ()
{
	if (CloseHandle (m_hPft))
	{
		m_hPft = NULL ;
		return true ;
	} else return false ;
}

//++
//
// CPft::RdPftReg - Read a register from PFT address space
//
// Inputs : register to read
//			pointer to variable to receive register value
//
// Outputs : bool, false = fail, true = success
//
//--

bool CPft::RdPftReg (unsigned long reg, unsigned long& val)
{
	if (!(CPft::IoControl (IOCTL_READ_PFT_REG, (void*)&reg, sizeof (unsigned long),
											   (void*)&val, sizeof (unsigned long)))) return false ;
	else return true ;
}

//++
//
// CPft::RdPlxReg - Read a register from PLX address space
//
// Inputs : register to read
//			pointer to variable to receive register value
//
// Outputs : bool, false = fail, true = success
//
//--

bool CPft::RdPlxReg (unsigned long reg, unsigned long& val)
{
	if (!(CPft::IoControl (IOCTL_READ_PLX_REG, (void*)&reg, sizeof (unsigned long),
											   (void*)&val, sizeof (unsigned long)))) return false ;
	else return true ;
}

//++
//
// CPft::WrtPftReg - Write a register in PFT address space
//
// Inputs : register to write
//			value to write
//
// Outputs : bool, false = fail, true = success
//
//--

bool CPft::WrtPftReg (unsigned long offset, unsigned long val)
{
	REGWRITE regwrite ;

	regwrite.offset = offset ;
	regwrite.data = val ;

	if (!(CPft::IoControl (IOCTL_WRT_PFT_REG, (void*)&regwrite, sizeof (REGWRITE), (void*)NULL, 0))) return false ;
	else return true ;
}

//++
//
// CPft::WrtPlxReg - Write a register in PFT address space
//
// Inputs : register to write
//			value to write
//
// Outputs : bool, false = fail, true = success
//
//--

bool CPft::WrtPlxReg (unsigned long offset, unsigned long val)
{
	REGWRITE regwrite ;

	regwrite.offset = offset ;
	regwrite.data = val ;

	if (!(CPft::IoControl (IOCTL_WRT_PLX_REG, (void*)&regwrite, sizeof (REGWRITE), (void*)NULL, 0))) return false ;
	else return true ;
}

//++
//
// CPft::GetBuffer
//
// Inputs : None 
//
// Outputs : Returns unsigned long pointer to user mapped buffer - NULL if failure
//
//--

unsigned long* CPft::GetBuffer ()
{
	unsigned long buffer = 0 ;

	if (!(CPft::IoControl (IOCTL_GET_MAPPED_BUFFER, (void*)NULL, 0,
						  (void*)&buffer, sizeof (unsigned long)))) buffer = (unsigned long)NULL ;

	return ((unsigned long*)buffer) ;
}

//++
//
// CPft::GetLogical
//
// Inputs : None 
//
// Outputs : Returns the logical address of the kernel mode buffer
//
//--

unsigned long CPft::GetLogical ()
{
	unsigned long buffer = 0 ;

	if (!(CPft::IoControl (IOCTL_GET_CB_LOGICAL, (void*)NULL, 0,
						  (void*)&buffer, sizeof (unsigned long)))) buffer = (unsigned long)NULL ;

	return (buffer) ;
}

//++
//
// CPft::ReleaseBuffer
//
// Inputs : None 
//
// Outputs : bool TRUE = success
//
//--

bool CPft::ReleaseBuffer ()
{
	if (!(CPft::IoControl (IOCTL_UNMAP_BUFFER, (void*)NULL, 0, (void*)NULL, 0))) return false ;
	return (true) ;
}

//++
//
//
//	CPft::Stop			- Stop the acquisition
//	
//	IN	(NONE)
//	OUT (NONE)
//
//	Returns:			- TRUE on success or FALSE on failure
//
//--

BOOL
CPft::Stop ()
{
	if (!(CPft::IoControl (IOCTL_UNMAP_BUFFER, (void*)NULL, 0, (void*)NULL, 0))) return false ;
	return (true) ;
}

//++
//
// CPft::IoControl - Issue a DeviceIoControl operation to the device
//
// Inputs : Control Code
//			pointer to input data
//			length of input data
//			pointer to output data
//			length of output data
//
// Outputs : bool, false = fail, true = success
//
//--

BOOL CPft::IoControl (int code, void* in, int inlen, void* out, int outlen)
{
	DWORD nbytes ;

	BOOL IoResult = DeviceIoControl (
							m_hPft,				// handle to PFT
							(DWORD)code,		// I/O controll code
							in,					// pointer to input data
							(DWORD)inlen,		// input length
							out,				// pointer to output
							(DWORD)outlen,		// output buffer size
							&nbytes,			// number of bytes returned
							(LPOVERLAPPED)NULL ) ;	// No overlap

	return IoResult ;
}
