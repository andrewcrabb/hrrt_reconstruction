//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		remove differential non-linearity
// 
// Name:			remove_dnl.cpp
//
// Routine:			remove_dnl
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	July 8, 2003
// 
// Description:		This component removes artifacts introduced by analog to digital conversion
//					usually in position profile data
//
// Arguments:		in	- long = input array
//					out	- double = output array
//
// Returns:			void
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "remove_dnl.h"
#include "find_peak_1d.h"

void remove_dnl(long in[PP_SIZE], double out[PP_SIZE])
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;

	long Line_X[PP_SIZE_X];
	long Line_Y[PP_SIZE_Y];

	double Filter_X[PP_SIZE_X];
	double Filter_Y[PP_SIZE_Y];

	// initialize
	for (j = 0 ; j < PP_SIZE_X ; j++) Line_Y[j] = 0;
	for (i = 0 ; i < PP_SIZE_X ; i++) Line_X[i] = 0;

	// one dimensionalize the data
	for (j = 0 ; j < PP_SIZE_X ; j++) for (i = 0 ; i < PP_SIZE_X ; i++) {
		index = (j * PP_SIZE_X) + i;
		Line_X[i] += in[index];
		Line_Y[j] += in[index];
	}

	// smooth the one dimension data
	Smooth(5, 2, Line_X, Filter_X);
	Smooth(5, 2, Line_Y, Filter_Y);

	// Fill in gaps in original lines
	for (j = 0 ; j < PP_SIZE_Y ; j++) if (Line_Y[j] < 1) Line_Y[j] = 1;
	for (i = 0 ; i < PP_SIZE_X ; i++) if (Line_X[i] < 1) Line_X[i] = 1;

	// finish filter
	for (j = 0 ; j < PP_SIZE_Y ; j++) Filter_Y[j] /= (double) Line_Y[j];
	for (i = 0 ; i < PP_SIZE_X ; i++) Filter_X[i] /= (double) Line_X[i];

	// apply filter
	for (j = 0 ; j < PP_SIZE_X ; j++) for (i = 0 ; i < PP_SIZE_X ; i++) {
		index = (j * PP_SIZE_X) + i;
		out[index] = (double) in[index] * Filter_Y[j] * Filter_X[i];
	}
}