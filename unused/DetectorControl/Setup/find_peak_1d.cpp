//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Peak Finding Tools
// 
// Name:			find_peak_1d.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	July 21, 2002
// 
// Description:		This component is the peak finding tools used by detector setup and 
//					detector utilities.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// Routine:		Smooth
// Purpose:		Smoothing of 1 dimensional data
// Arguments:	range	-	long = window size
//				passes	-	long = number of smoothing passes
//				in		-	long = input array
//				out		-	double = output array
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Smooth(long range, long passes, long in[256], double out[256])
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long start = 0;
	long stop = 0;
	long rng = 0;

	double sum;

	double before[256];

	// range must be an odd number
	rng = ((range % 2) == 1) ? range : (range+1);

	// transfer input data to the before array
	for (i = 0 ; i < 256 ; i++) {
		before[i] = (double) in[i];
		out[i] = (double) in[i];
	}

	// loop the requested number of passes
	for (i = 0 ; i < passes ; i++) {

		// for each element
		for (j = 0 ; j < 256 ; j++) {

			// sum neighborhood
			start = j - (rng / 2);
			stop = j + (rng / 2);
			for (k = start, sum = 0.0 ; k <= stop ; k++) {
				if (k <= 0) {
					sum += before[0];
				} else if (k >= 255) {
					sum += before[255];
				} else {
					sum += before[k];
				}
			}

			// transfer to output array
			out[j] = sum / rng;
		}

		// transfer back to the before array for the next pass
		for (j = 0 ; j < 256 ; j++) before[j] = out[j];
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Find_Peak_1d
// Purpose:		find the peaks of 1 dimensional data
// Arguments:	dData	-	double = array of input data
//				*lPeaks	-	long = array of peaks positions
// Returns:		long Number of peaks
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Find_Peak_1d (double dData[256], long *lPeaks)

{
	long i;
	long j;
	long num_peaks = 0;

	// check if the first point is a peak
	if (dData[0] > dData[1]) {
		lPeaks[num_peaks] = 0;
		num_peaks++;
	}

	// find the peaks in the middle of the data
	for (i = 1 ; i < 255 ; i++) {
		if ((dData[i] > dData[i-1]) && (dData[i] > dData[i+1])) {
			lPeaks[num_peaks] = i;
			num_peaks++;
		}
	}

	// check if the last point is a peak
	if (dData[255] > dData[254]) {
		lPeaks[num_peaks] = 255;
		num_peaks++;
	}

	// if no peaks found, then find high point
	if (num_peaks == 0) {
		long high_point = 0;
		for (i = 0 ; i < 256 ; i++) if (dData[i] > dData[high_point]) high_point = i;
		lPeaks[num_peaks] = high_point;
		num_peaks++;
	}

	// if more than one peak found, sort for highest first
	long temp = 0;
	if (num_peaks > 1) {

		//bubble sort
		for (i = 0 ; i < (num_peaks-1) ; i++) for (j = i + 1 ; j < num_peaks ; j++) {
			if (dData[lPeaks[j]] > dData[lPeaks[i]]) {
				temp = lPeaks[i];
				lPeaks[i] = lPeaks[j];
				lPeaks[j] = temp;
			}
		}
	}

	// return the high point(s)
	return num_peaks;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Find_Valley_1d
// Purpose:		find the Valleys of 1 dimensional data
// Arguments:	dData		-	double = array of input data
//				*lValleys	-	long = array of valley positions
// Returns:		long Number of valleys
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Find_Valley_1d (double dData[256], long *lValleys)

{
	long i;
	long j;
	long num_valleys = 0;

	// check if the first point is a valley
	if (dData[0] < dData[1]) {
		lValleys[num_valleys] = 0;
		num_valleys++;
	}

	// find the valleys in the middle of the data
	for (i = 1 ; i < 255 ; i++) {
		if ((dData[i] < dData[i-1]) && (dData[i] < dData[i+1])) {
			lValleys[num_valleys] = i;
			num_valleys++;
		}
	}

	// check if the last point is a valley
	if (dData[255] < dData[254]) {
		lValleys[num_valleys] = 255;
		num_valleys++;
	}

	// if no valleys found, then find low point
	if (num_valleys == 0) {
		long low_point = 0;
		for (i = 0 ; i < 256 ; i++) if (dData[i] < dData[low_point]) low_point = i;
		lValleys[num_valleys] = low_point;
		num_valleys++;
	}

	// if more than one valley found, sort for lowest first
	long temp = 0;
	if (num_valleys > 1) {

		//bubble sort
		for (i = 0 ; i < (num_valleys-1) ; i++) for (j = i + 1 ; j < num_valleys ; j++) {
			if (dData[lValleys[j]] < dData[lValleys[i]]) {
				temp = lValleys[i];
				lValleys[i] = lValleys[j];
				lValleys[j] = temp;
			}
		}
	}

	// return the lowest point(s)
	return num_valleys;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Peak_Most_Counts
// Purpose:		find the hightes point in one dimensional data
// Arguments:	dData	-	double = array of input data
// Returns:		long Peak position
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Peak_Most_Counts (double dData[256])
{
	long i = 0;
	long j = 0;
	long Peak = 10;

	double Sum = 0.0;
	double Most = 0;

	// loop through bins
	for (i = 10 ; i < 246 ; i++) {

		// sum +/- 10 bins
		for (Sum = 0.0, j = (i - 10) ; j <= (i + 10) ; j++) Sum += dData[j];

		// if the sum is higher that the previous high sum, then use it
		if (Sum > Most) {
			Most = Sum;
			Peak = i;
		}
	}

	// return point with most counts
	return Peak;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Determine_Gain
// Purpose:		Determine the new gain value from current and master tube energy data
// Arguments:	OldGain	-	long = gain at which current data was taken
//				Peak	-	long = minimum allowable current peak
//				Current	-	double = current tube energy
//				Master	-	double = master tube energy
// Returns:		long New Gain
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Determine_Gain(long OldGain, long Peak, double Current[256], double Master[256])
{
	// local variables
	long i = 0;
	long index = 0;
	long NewGain = 0;
	long Shift = 0;

	double HighCurrent = 0.0;
	double HighMaster = 0.0;
	double Ratio = 0.0;
	double CurrentEvents = 0.0;
	double MinArea = 0.0;
	double Area = 0.0;
	double BestShift = 0.0;

	double Work[256];

	// don't let peak be below 100;
	if (Peak < 100) Peak = 100;

	// find the high values
	for (i = 0 ; i < 256 ; i++) {
		if (Current[i] > HighCurrent) HighCurrent = Current[i];
		if (Master[i] > HighMaster) HighMaster = Master[i];
	}

	// if no current data, then bring it up 25%
	if (HighCurrent == 0.0) {
		NewGain = (OldGain * 125) / 100;
		if (NewGain > 255) NewGain = 255;
		return NewGain;
	}

	// determine ratio
	Ratio = HighMaster / HighCurrent;

	// transfer scaled data to working array
	for (i = 0 ; i < 256 ; i++) {
		Work[i] = Current[i] * Ratio;
		CurrentEvents += Work[i];
	}

	// loop through all possibilities
	for (MinArea = (2*CurrentEvents), Shift = -128 ; Shift < 127 ; Shift++) {

		// determine the area of the difference of the two histograms
		for (Area = 0, i = 0 ; i < 256 ; i++) Area += fabs(Master[i] - Current[(i-Shift+256) % 256]);

		// note shift closest to shape of master
		if (Area < MinArea) {
			MinArea = Area;
			BestShift = (double) Shift;
		}
	}

	// apply best shift to gain setting
	Ratio = 1.0 + ((BestShift / (double) Peak) * 0.4);
	NewGain = (long) (((double) OldGain * Ratio) + 0.5);

	return NewGain;
}

////////////////////////////////////////////////////////////////////////////////
// Name:	peak_analyze
// Purpose:	finds the centroid and fwhm of 1 data
// Arguments:	data		-	long = Input array
//				length		-	long = number of elements in array
//				*centroid	-	double = centroid of data
// Returns:		double fwhm
// History:		18 Sep 03	T Gremillion	History Started
//				29 Sep 03	T Gremillion	Moved from DSMain.cpp to find_peak_1d.cpp
////////////////////////////////////////////////////////////////////////////////

double peak_analyze(long *data, long length, double *centroid)
{
	long i;
	long max_val = -1;
	long max_bin = -1;

	double half_max;
	double left_loc = 0.0;
	double right_loc = 0.0;
	double phi;
	double r;

	// locate the peak
	for (i = 0; i < length; i++) {
		if (data[i] > max_val) {
			max_val = data[i];
			max_bin = i;
		}
	}

	// half of maximum value
	half_max = max_val / 2.0;

	// locate the left location of half-max
	for (i = max_bin - 1; i > 0; i--) {
		if (data[i] < half_max) {
			phi = atan((data[i+1] - data[i]) / 1.0);
			r = (half_max - data[i]) / sin(phi);
			left_loc = i + (r * cos(phi));
			break;
		}
	}

	// locate the right location of half-max
	right_loc = (double)(length-1);
	for (i = max_bin + 1; i < length; i++) {
		if (data[i] < half_max)	{
			phi = atan((data[i-1] - data[i]) / 1.0);
			r = (half_max - data[i]) / sin(phi);
			right_loc = i - (r * cos(phi));
			break;
		}
	}

	// centroid is the average of the left and right half-max locations
	*centroid = (right_loc + left_loc) / 2.0;

	// return the full width half max
	return (right_loc - left_loc);
}
