// Tfa.cpp -	This code implements the CTfa class that controls
//				the TFA (TAXI to Fibre Adapter) card for acquisitions.
//
//
// Created:	28feb02	-	jhr
//
//

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <iostream.h>
#include "acs3acq.h"

/*********************************************************************
*
*
*	CTfa::WordInsert	- Put a 64 bit word in the
*						  Word Insertion register of the TFA
*	
*	IN	__int64 WVal	- 64 bit value to insert
*	OUT (NONE)
*
*	Returns:			- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CTfa::WordInsert (__int64 WVal)
{
	int err ;

	// insert the value
	return (IoControl (IOCTL_TFA_WORD_INSERT, &WVal, sizeof(__int64), NULL, 0, &err)) ;
}

