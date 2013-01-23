// Rebinner.cpp -	This code implements the CRebinner class that controls
//					the ACS III Rebinner card for acquisitions.
//
//
// Created:	28feb02	-	jhr
// Modified 01Dec03 -	tgg - removed commented out code
//
//

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <iostream.h>
#include "PPAIoctl.h"
#include "SinglesAcq.h"

// I/O Control code for word insertion
#define IOCTL_WORD_INSERT CTL_CODE(		\
					FILE_DEVICE_UNKNOWN,\
					0x803,				\
					METHOD_BUFFERED,	\
					FILE_ANY_ACCESS)

// Rebinner tag word defines
#define RbReset		0x8000fc0040000001		// Rebinner Reset Control Tag word
#define RbTag		0x8000000040000000		// Rebinner Tag Word
#define RbCTW		0x8000fc0040000000		// Rebinner Control Tag word

// Passall and LUT defines
#define LUT1		0x2						// Lookup table 1 value
#define PASSALL		0x4						// 64 bit Pass all value					

/*********************************************************************
*
*
*	CRebinner::SetupRebinner- Resets the reinner and puts a 64 bit
*							  control tag word in the  rebinner to
*							  setup its acquisition mode.
*	
*	IN	bool passall		- 64 passall mode flag
*	IN  int lut				- rebinner lookup table to use
*	OUT (NONE)
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CRebinner::SetupRebinner (int passall, int lut)
{
	DWORD nbytes ;
	HANDLE hReb ;
	DWORD t0 ;
	__int64 RbWiVal ;
	int i ;

	// open the rebinner
	hReb = CreateFile (
				"\\\\.\\rb1", 
        		0, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        		NULL, OPEN_EXISTING, 0, NULL
        		) ;

	// invalid handle, return false
	if (hReb == INVALID_HANDLE_VALUE) return false ;

	for (i=0;i<4;i++)
	{
		// sequence the reset loop
		if (i == 0) RbWiVal = RbReset ;
		if (i == 1 || i == 2) RbWiVal = RbTag ;
		if (i == 3)
		{
			RbWiVal = RbCTW ;
			if (lut == 1) RbWiVal |= LUT1 ;
			if (!passall) RbWiVal |= PASSALL ;
		}

		// insert the value
		if (!(DeviceIoControl (
							hReb,
							IOCTL_WORD_INSERT,
							&RbWiVal,
							sizeof(__int64),
							NULL,
							0,
							&nbytes,
							NULL
							)))
		{
			return false ;
		}

		// wait about 50 msec if reset
		if (i == 0)
		{
			t0 = GetTickCount () ;
			while ((GetTickCount () - t0) <= 50) ;
		}
	}

	// close the rebinner
	CloseHandle (hReb) ;

	return (TRUE) ;
}

