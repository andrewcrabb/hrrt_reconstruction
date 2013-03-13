//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		convert string input to an integer and validate against a range
// 
// Name:			Validate.cpp
//
// Routine:			Validate
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	February 12, 2003
// 
// Description:		This component converts string input to an integer and validates against a range
//
// Arguments:		Str		-	char = input string
//					Min		-	long = minimal allowable value
//					Max		-	long = maxmmum allowable value
//					pValue	-	long = return integer
//
// Returns:			long status
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include "DHI_Constants.h"
#include "Validate.h"

long Validate(char Str[MAX_STR_LEN], long Min, long Max, long *pValue)
{
	// local variables
	long i = 0;
	long Len = 0;

	// get length of String
	Len = strlen(Str);

	// if NULL string, then not valid
	if (Len == 0) return 1;

	// make sure all the characters are digits (48 is '0' and 57 is '9')
	for (i = 0 ; i < Len ; i++) {
		if ((Str[i] < 48) || (Str[i] > 57)) return 2;
	}

	// convert
	*pValue = atol(Str);

	// compare against range
	if ((*pValue < Min) || (*pValue > Max)) return 3;

	// success
	return 0;
}