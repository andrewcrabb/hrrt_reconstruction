//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Tools for working with position profile data (finding crystal locations)
// 
// Name:			position_profile_tools.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	July 23, 2002
// 
// Description:		This component is the position profile tools used by detector setup and 
//					detector utilities to find crystal locations.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include "DHI_Constants.h"
#include "position_profile_tools.h"
#include "remove_dnl.h"

static long m_MasterFound;

void Log_Message(char* Message);

/////////////////////////////////////////////////////////////////////////////
// Routine:		SetMasterFlag
// Purpose:		master flag controls size of area searched for peaks
// Arguments:	MasterFound	-	long = state flag
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void SetMasterFlag(long MasterFound)
{
	m_MasterFound = MasterFound;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Standard_CRM_Pattern
// Purpose:		generates a standard CRM pattern for the specfied number of blocks
//				and crystal dimensions
// Arguments:	NumBlocks	-	long = number of blocks
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
//				*Position	-	long = output array of standard positions
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Standard_CRM_Pattern(long NumBlocks, long XCrystals, long YCrystals, long *Position)
{
	// local variables
	long x = 0;
	long y = 0;
	long vx = 0;
	long vy = 0;
	long Block = 0;
	long index = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long VertsBlock = 0;

	long PosX[33];
	long PosY[33];

	double Space_X = 0;
	double Space_Y = 0;

	// number of verticies per block
	Verts_X = (2 * XCrystals) + 1;
	Verts_Y = (2 * YCrystals) + 1;
	VertsBlock = Verts_X * Verts_Y;

	// Determine Spacing (leave 24 pixels on each side for crosstalk)
	Space_X = (double)(PP_SIZE_X - 24 - 24) / (double) (Verts_X - 1);
	Space_Y = (double)(PP_SIZE_Y - 24 - 24) / (double) (Verts_Y - 1);

	// fill in one dimensional values
	for (x = 0 ; x < Verts_X ; x++) PosX[x] = (long)(((double)x * Space_X) + 24.5);
	for (y = 0 ; y < Verts_Y ; y++) PosY[y] = (long)(((double)y * Space_Y) + 24.5);

	// Fill in for all blocks
	for (Block = 0 ; Block < NumBlocks ; Block++) {

		// default position for each vertex
		for (y = 0 ; y < Verts_Y ; y++) for (x = 0 ; x < Verts_X ; x++) {
			index = (Block * VertsBlock * 2) + (y * Verts_X * 2) + (x * 2);
			Position[index+0] = PosX[x];
			Position[index+1] = PosY[y];
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Fill_CRM_Verticies
// Purpose:		fills in the verticies between peak positions
// Arguments:	Edge		-	long = distance from edge crystals to extend
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
//				*Vert		-	long = output array of standard positions
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Fill_CRM_Verticies(long Edge, long XCrystals, long YCrystals, long *Vert)
{
	// local variables
	long i = 0;
	long x = 0;
	long y = 0;
	long index = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long VertsBlock = 0;

	// number of verticies per block
	Verts_X = (2 * XCrystals) + 1;
	Verts_Y = (2 * YCrystals) + 1;
	VertsBlock = Verts_X * Verts_Y;

	// restrict peak positions
	for (y = 1 ; y < (Verts_Y-1) ; y += 2) for (x = 1 ; x < (Verts_X-1) ; x += 2) {
		index = (y * Verts_X * 2) + (x * 2);
		if (Vert[index+0] < 10) Vert[index+0] = 10;
		if (Vert[index+0] > (PP_SIZE_X-11)) Vert[index+0] = PP_SIZE_X - 11;
		if (Vert[index+1] < 10) Vert[index+1] = 10;
		if (Vert[index+1] > (PP_SIZE_Y-11)) Vert[index+1] = PP_SIZE_Y - 11;
	}

	// fill in all middle vertices
	for (y = 1 ; y < (Verts_Y-1) ; y++) for (x = 1 ; x < (Verts_X-1) ; x++) {

		// if peak position, it is not processed
		if (((x % 2) == 1) && ((y % 2) == 1)) continue;

		// index into array
		index = (y * Verts_X * 2) + (x * 2);

		// on the same row as peaks and between columns
		if (((y % 2) == 1) && ((x % 2) == 0)) {
			Vert[index+0] = (Vert[index-2] + Vert[index+2]) / 2;
			Vert[index+1] = (Vert[index-1] + Vert[index+3]) / 2;

		// on the same column as peaks and between rows
		} else if (((y % 2) == 0) && ((x % 2) == 1)) {
			Vert[index+0] = (Vert[index-(2*Verts_X)+0] + Vert[index+(2*Verts_X)+0]) / 2;
			Vert[index+1] = (Vert[index-(2*Verts_X)+1] + Vert[index+(2*Verts_X)+1]) / 2;

		// between rows and columns
		} else if (((y % 2) == 0) && ((x % 2) == 0)) {
			Vert[index+0] = long(((double)(Vert[index-(2*Verts_X)-2] + Vert[index-(2*Verts_X)+2] + 
											   Vert[index+(2*Verts_X)-2] + Vert[index+(2*Verts_X)+2]) / 4.0) + 0.5);
			Vert[index+1] = long(((double)(Vert[index-(2*Verts_X)-1] + Vert[index-(2*Verts_X)+3] + 
											   Vert[index+(2*Verts_X)-1] + Vert[index+(2*Verts_X)+3]) / 4.0) + 0.5);
		}
	}

	// left and right edges
	for (y = 1 ; y < (Verts_Y-1) ; y++) {

		// left
		index = y * Verts_X * 2;
		Vert[index+0] = Vert[index+2] - Edge;
		Vert[index+1] = Vert[index+3];
		if (Vert[index+0] < 5) Vert[index+0] = 5;

		// right
		index = (y * Verts_X * 2) + (Verts_X * 2) - 2;
		Vert[index+0] = Vert[index-2] + Edge;
		Vert[index+1] = Vert[index-1];
		if (Vert[index+0] > PP_SIZE_X-6) Vert[index+0] = PP_SIZE_X - 6;
	}

	// top and bottom
	for (x = 1 ; x < (Verts_X-1) ; x++) {

		// top
		index = ((Verts_Y - 1) * Verts_X * 2) + (x * 2);
		Vert[index+0] = Vert[index-(2*Verts_X)+0];
		Vert[index+1] = Vert[index-(2*Verts_X)+1] + Edge;
		if (Vert[index+1] > PP_SIZE_Y - 6) Vert[index+1] = PP_SIZE_Y - 6;

		// bottom
		index = x * 2;
		Vert[index+0] = Vert[index+(2*Verts_X)+0];
		Vert[index+1] = Vert[index+(2*Verts_X)+1] - Edge;
		if (Vert[index+1] < 5) Vert[index+1] = 5;
	}

	// bottom left corner
	index = 0;
	Vert[index+0] = (Vert[index+2] + (3*Vert[index+(2*Verts_X)+0])) / 4;
	Vert[index+1] = ((3*Vert[index+3]) + Vert[index+(2*Verts_X)+1]) / 4;

	// bottom right corner
	index = 2 * Verts_X - 2;
	Vert[index+0] = (Vert[index-2] + (3*Vert[index+(2*Verts_X)+0])) / 4;
	Vert[index+1] = ((3*Vert[index-1]) + Vert[index+(2*Verts_X)+1]) / 4;

	// top left corner
	index = (Verts_Y - 1) * Verts_X * 2;
	Vert[index+0] = (Vert[index+2] + (3*Vert[index-(2*Verts_X)+0])) / 4;
	Vert[index+1] = ((3*Vert[index+3]) + Vert[index-(2*Verts_X)+1]) / 4;

	// top right corner
	index = (VertsBlock * 2) - 2;
	Vert[index+0] = (Vert[index-2] + (3*Vert[index-(2*Verts_X)+0])) / 4;
	Vert[index+1] = ((3*Vert[index-1]) + Vert[index-(2*Verts_X)+1]) / 4;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Smooth_2d
// Purpose:		smoothing of 2 dimensional data
// Arguments:	CorrectDNL	-	long = flag whether or not to applay dnl correction
//				range		-	long = window size
//				passes		-	long = number of smoothing passes
//				*in			-	long = input array
//				*out		-	double = output array
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Smooth_2d(long CorrectDNL, long range, long passes, long in[PP_SIZE], double out[PP_SIZE])
{
	long i = 0;
	long j = 0;
	long ii = 0;
	long jj = 0;
	long x = 0;
	long x1 = 0;
	long x2 = 0;
	long y = 0;
	long y1 = 0;
	long y2 = 0;
	long index = 0;
	long pass = 0;
	long rng = 0;

	double sum;

	double before[PP_SIZE];

	// range must be an odd number
	rng = ((range % 2) == 1) ? range : (range+1);

	// transfer input data to the before array
	if (1 == CorrectDNL) {
		remove_dnl(in, before);
	} else {
		for (i = 0 ; i < PP_SIZE ; i++) before[i] = (double) in[i];
	}

	// loop the requested number of passes
	for (pass = 0 ; pass < passes ; pass++) {

		// for each element
		for (j = 0 ; j < PP_SIZE_Y ; j++) for (i = 0 ; i < PP_SIZE_X ; i++) {

			// sum neighborhood
			x1 = i - (rng / 2);
			x2 = i + (rng / 2);
			y1 = j - (rng / 2);
			y2 = j + (rng / 2);

			// loop through rows of neighborhood
			sum = 0.0;
			for (jj = y1 ; jj <= y2 ; jj++) {

				// if out of range, use closest value
				y = (jj >= PP_SIZE_Y) ? (PP_SIZE_Y-1) : ((jj < 0) ? 0 : jj);

				// loop through columns of neighborhood
				for (ii = x1 ; ii <= x2 ; ii++) {

					// if out of range, use closest value
					x = (ii >= PP_SIZE_X) ? (PP_SIZE_X-1) : ((ii < 0) ? 0 : ii);

					// calculate array position
					index = (y * PP_SIZE_X) + x;

					// add value to sum
					sum += before[index];
				}
			}

			// calculate array position
			index = (j * PP_SIZE_X) + i;

			// transfer to output array
			out[index] = sum / (rng * rng);
		}

		// transfer back to the before array for the next pass
		for (j = 0 ; j < 256 ; j++) before[j] = out[j];
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Find_Region
// Purpose:		returns the region within a polygon
// Arguments:	line_x		-	long = array of x coordinates
//				line_y		-	long = array of y coordinates
//				*Region		-	long = output array region indicies
// Returns:		long Number of pixels in the region
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Find_Region(long *line_x, long *line_y, long *Region)
{
	// local variables
	long i = 0;
	long x = 0;
	long y = 0;
	long index = 0;
	long Num = 0;
	long min_x = PP_SIZE_X;
	long max_x = -1;
	long min_y = PP_SIZE_Y;
	long max_y = -1;
	long avg_x = 0;
	long avg_y = 0;
	long Last = -1;
	long Last_Side = 0;
	long Curr_Side = 0;
	long quad = 0;
	long in_quad = 0;

	long quad_x[5];
	long quad_y[5];

	// error if first and last points are not the same
	if ((line_x[0] != line_x[8]) || (line_y[0] != line_y[8])) return -1;

	// determine the local area and center
	for (i = 0 ; i < 8 ; i++) {
		if (line_x[i] < min_x) min_x = line_x[i];
		if (line_x[i] > max_x) max_x = line_x[i];
		if (line_y[i] < min_y) min_y = line_y[i];
		if (line_y[i] > max_y) max_y = line_y[i];
		avg_x += line_x[i];
		avg_y += line_y[i];
	}

	// center position
	quad_x[2] = avg_x / 8;
	quad_y[2] = avg_y / 8;

	// loop through the local area
	for (y = min_y ; y <= max_y ; y++) for (x = min_x ; x <= max_x ; x++) {

		// try 4 different quadrants
		for (in_quad = 0, quad = 0 ; (quad < 4) && (in_quad == 0) ; quad++) {

			// fill in the quad values
			quad_x[0] = line_x[quad*2];
			quad_y[0] = line_y[quad*2];
			quad_x[1] = line_x[(quad*2)+1];
			quad_y[1] = line_y[(quad*2)+1];
			quad_x[3] = line_x[((quad*2)+7)%8];
			quad_y[3] = line_y[((quad*2)+7)%8];
			quad_x[4] = quad_x[0];
			quad_y[4] = quad_y[0];

			// find a pair that are not the same point
			for (Last = -1, i = 0 ; i < 4 ; i++) if ((quad_x[i] != quad_x[i+1]) || (quad_y[i] != quad_y[i+1])) Last = i;

			// all the same point
			if (Last == -1) {

				// if current point
				if ((x == quad_x[0]) && (y == quad_y[0])) in_quad = 1;

			// quad occupies space
			} else {

				// determine which side of the last segment the point is on
				Last_Side = Side(x, y, quad_x[Last], quad_y[Last], quad_x[Last+1], quad_y[Last+1]);

				// check if it is on the same side of all segments
				for (Curr_Side = Last_Side, i = 0 ; (Curr_Side == Last_Side) && (i < 4) ; i++) {

					// if the two points are the same point, then don't check
					if ((quad_x[i] != quad_x[i+1]) || (quad_y[i] != quad_y[i+1])) {
						Curr_Side = Side(x, y, quad_x[i], quad_y[i], quad_x[i+1], quad_y[i+1]);
					}
				}

				// on the same side of all segments
				if (Curr_Side == Last_Side) in_quad = 1;
			}

			// if it all checked out, then it is in the polygon
			if (in_quad == 1) {
				index = (y * PP_SIZE_X) + x;
				Region[Num++] = index;
			}
		}
	}

	// return the number of pixels in the region
	return Num;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		region_high
// Purpose:		returns the position with the highest value within the region
// Arguments:	data		-	double = position profile
//				NumPixels	-	long = number of pixels in region
//				*Region		-	long = input array region indicies
// Returns:		long position with highest value in the region
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long region_high(double data[PP_SIZE], long NumPixels, long *Region)
{
	// local variables
	long i;
	long index = -1;

	double high = -1.0;

	// find the highest value
	for (i = 0 ; i < NumPixels ; i++) {
		if (data[Region[i]] > high) {
			index = Region[i];
			high = data[index];
		}
	}

	// return location of largest value
	return index;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		region_sum
// Purpose:		returns the total of all values within the region
// Arguments:	data		-	double = position profile
//				NumPixels	-	long = number of pixels in region
//				*Region		-	long = input array region indicies
// Returns:		double total of all values in the region
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

double region_sum(double data[PP_SIZE], long NumPixels, long *Region)
{
	// local variables
	long i;

	double sum = 0.0;

	// add the next value
	for (i = 0 ; i < NumPixels ; i++) sum += data[Region[i]];

	// return location of largest value
	return sum;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Shifted_Peaks
// Purpose:		returns the position with the highest value within the region
// Arguments:	pp			-	double = position profile
//				Verts		-	long = input verticies
//				MiddleX		-	long = position to assign to middle point X
//				MiddleY		-	long = position to assign to middle point Y
//				vx			-	long = middle point X
//				vy			-	long = middle point Y
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
//				*NewVerts	-	long = output verticies
// Returns:		double fit statistic
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

double Shifted_Peaks(double *pp, long *Verts, long MiddleX, long MiddleY, long vx, long vy, long XCrystals, long YCrystals, long *NewVerts)
{
	// local variables
	long i = 0;
	long index = 0;
	long shift_x = 0;
	long shift_y = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long VertsBlock = 0;
	long cx = 0;
	long cy = 0;
	long Next = 0;
	long nx = 0;
	long ny = 0;
	long NumPixels = 0;

	long line_x[9];
	long line_y[9];
	long Region[65536];
	long Flag[256];
	long WorkVerts[2178];				// 1089 = 33 * 33, 33 = (16 * 2) + 1, 16 = Maximum Crystals in row or column

	double Run[256];

	double Mean = 0.0;
	double Fit = 0.0;

	// number of verticies per block
	Verts_X = (2 * XCrystals) + 1;
	Verts_Y = (2 * YCrystals) + 1;
	VertsBlock = Verts_X * Verts_Y;

	// clear the flags for when a peak is established
	for (i = 0 ; i < 256 ; i++) Flag[i] = 0;

	// determine shift
	index = (vy * (2 * Verts_X)) + (2 * vx);
	shift_x = MiddleX - Verts[index+0];
	shift_y = MiddleY - Verts[index+1];

	// transfer the verticies while shifting them
	for (i = 0 ; i < VertsBlock ; i++) {
		index = i * 2;
		NewVerts[index+0] = Verts[index+0] + shift_x;
		NewVerts[index+1] = Verts[index+1] + shift_y;
		WorkVerts[index+0] = NewVerts[index+0];
		WorkVerts[index+1] = NewVerts[index+1];
	}

	// for its immediate neighbors adjust them to highest point in the regions shifted from the original point
	Peak_Shift(pp, vx-2, vy, 0, 0, XCrystals, YCrystals, NewVerts);
	Peak_Shift(pp, vx+2, vy, 0, 0, XCrystals, YCrystals, NewVerts);
	Peak_Shift(pp, vx, vy-2, 0, 0, XCrystals, YCrystals, NewVerts);
	Peak_Shift(pp, vx, vy+2, 0, 0, XCrystals, YCrystals, NewVerts);

	// set the flag for the original 5
	cx = (vx - 1) / 2;
	cy = (vy - 1) / 2;
	index = (cy * XCrystals) + cx;
	Flag[index] = 1;
	Flag[index-1] = 1;
	Flag[index+1] = 1;
	Flag[index-XCrystals] = 1;
	Flag[index+XCrystals] = 1;

	// find the remaining crystals with the most number of found crystals and set it to its high point
	while ((Next = Next_Neighbor(Flag, XCrystals, YCrystals, WorkVerts, NewVerts, &shift_x, &shift_y)) != -1) {
		nx = ((Next % XCrystals) * 2) + 1;
		ny = ((Next / XCrystals) * 2) + 1;
		Peak_Shift(pp, nx, ny, shift_x, shift_y, XCrystals, YCrystals, NewVerts);
		Flag[Next] = 1;
	}

	// get new verticies
	Fill_CRM_Verticies(4, XCrystals, YCrystals, NewVerts);

	// number of counts for each crystal
	for (ny = 1 ; ny < Verts_Y ; ny += 2) for (nx = 1 ; nx < Verts_X ; nx += 2) {

		// get the border line
		Build_Line(nx, ny, XCrystals, YCrystals, NewVerts, line_x, line_y);

		// Get the region
		NumPixels = Find_Region(line_x, line_y, Region);

		// add the counts in the region to the sum
		Run[(((ny-1)/2)*XCrystals)+((nx-1)/2)] = region_sum(pp, NumPixels, Region);
	}

	// calculate the mean crystal sum
	for (i = 0 ; i < (XCrystals*YCrystals) ; i++) Mean += Run[i];
	Mean /= (double)(XCrystals * YCrystals);

	// Calculate the variance to use as the fit value
	for (i = 0 ; i < (XCrystals*YCrystals) ; i++) Fit += fabs(Mean - Run[i]);
	Fit /= (double)(XCrystals * YCrystals);

	return Fit;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Peak_Shift
// Purpose:		returns the position with the highest value within the region
// Arguments:	pp			-	double = position profile
//				vx			-	long = middle point X
//				vy			-	long = middle point Y
//				Shift_X		-	long = shift in X direction
//				Shift_Y		-	long = shift in Y direction
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
//				*Verts	-	long = output verticies
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Peak_Shift(double *pp, long vx, long vy, long Shift_X, long Shift_Y, long XCrystals, long YCrystals, long *Verts)
{
	// local variables
	long i = 0;
	long index = 0;
	long NumPixels = 0;
	long high = 0;
	long high_x = 0;
	long high_y = 0;
	long Verts_X = 0;
	long Verts_Y = 0;

	long line_x[9];
	long line_y[9];
	long Region[65536];

	// number of verticies
	Verts_X = (XCrystals * 2) + 1;
	Verts_Y = (YCrystals * 2) + 1;

	// get the border line
	Build_Line(vx, vy, XCrystals, YCrystals, Verts, line_x, line_y);

	// apply the shift to the line
	for (i = 0 ; i < 9 ; i++) {
		line_x[i] += Shift_X;
		if (line_x[i] < 1) line_x[i] = 1;
		if (line_x[i] > (PP_SIZE_X-2)) line_x[i] = PP_SIZE_X - 2;
		line_y[i] += Shift_Y;
		if (line_y[i] < 1) line_y[i] = 1;
		if (line_y[i] > (PP_SIZE_Y-2)) line_y[i] = PP_SIZE_Y - 2;
	}


	// Get the region
	NumPixels = Find_Region(line_x, line_y, Region);

	// Highest point in region
	high = region_high(pp, NumPixels, Region);
	high_x = high % PP_SIZE_X;
	high_y = high / PP_SIZE_X;

	// insert new values
	index = ((vy * Verts_X) + vx) * 2;
	Verts[index+0] = high_x;
	Verts[index+1] = high_y;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Next_Neighbor
// Purpose:		returns the unresolved peak with the most good neighbors
// Arguments:	Flag		-	long = array denoting whether or not a peak has been resolved
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
//				Verts		-	long = input verticies
//				NewVerts	-	long = output verticies
//				shift_x		-	long = amount of shift in X direction
//				shift_y		-	long = amount of shift in Y direction
// Returns:		long unresolved peak with the most good neighbors
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Next_Neighbor(long *Flag, long XCrystals, long YCrystals, long *Verts, long *NewVerts, long *shift_x, long *shift_y)
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x = 0;
	long y = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long Most = -1;
	long CurrentNeighbors = 0;
	long MostNeighbors = 0;
	long Num = 0;

	double Center_X = 0;
	double Center_Y = 0;
	double Dist = 0.0;
	double Closest = 9999.999;

	// number of verticies per block
	Verts_X = (2 * XCrystals) + 1;
	Verts_Y = (2 * YCrystals) + 1;

	// loop through calculating center of found crystals
	for (y = 0 ; y < YCrystals ; y++) for (x = 0 ; x < XCrystals ; x++) {
		index = (y * XCrystals) + x;
		if (Flag[index] != 0) {
			Center_X += (double) x;
			Center_Y += (double) y;
			Num++;
		}
	}

	// averages
	Center_X /= (double) Num;
	Center_Y /= (double) Num;


	// loop through looking for largest number of neighbors, keeping track of occurrence closest to center of already found pixels
	for (y = 0 ; y < YCrystals ; y++) for (x = 0 ; x < XCrystals ; x++) {

		// number of good neighbors for current  crystal
		CurrentNeighbors = Number_Neighbors(Flag, x, y, XCrystals, YCrystals);

		// crystal distance from center
		Dist = sqrt(pow(fabs((double) x - Center_X), 2.0) + pow(fabs((double) y - Center_Y), 2.0));

		// if this crystal has more good neighbors than previously found
		// or has the same number of good neighbors, but is closer, then it is our choice
		if ((CurrentNeighbors > MostNeighbors) || ((CurrentNeighbors == MostNeighbors) && (CurrentNeighbors != 0) && (Dist < Closest))) {
			Most = (y * XCrystals) + x;
			MostNeighbors = CurrentNeighbors;
			Closest = Dist;
		}
	}

	// if one was found, then determine the amount of shift to start with
	if (Most != -1) {

		// initialize shift
		*shift_x = 0;
		*shift_y = 0;

		// x/y coordinates where the most neighbors were found
		x = Most % XCrystals;
		y = Most / XCrystals;

		// loop through neighbors
		for (j = (y-1) ; j <= (y+1) ; j++) {

			// if beyond edge, then skip
			if ((j < 0) || (j >= YCrystals)) continue;

			// loop through columns
			for (i = (x-1) ; i <= (x+1) ; i++) {

				// if beyond edge, then skip
				if ((i < 0) || (i >= XCrystals)) continue;

				// if set add to the shift amounts
				if (Flag[(j*XCrystals)+i] == 1) {
					index = (((j * 2) + 1) * Verts_X * 2) + (((i * 2) + 1) * 2);
					*shift_x += NewVerts[index+0] - Verts[index+0];
					*shift_y += NewVerts[index+1] - Verts[index+1];
				}
			}
		}

		// average the shift
		*shift_x = (long) (((double) *shift_x / (double) MostNeighbors) + 0.5);
		*shift_y = (long) (((double) *shift_y / (double) MostNeighbors) + 0.5);
	}

	// return the remaining entry with the most neighbors
	return Most;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Number_Neighbors
// Purpose:		returns the number of neighbors for a peak
// Arguments:	Flag		-	long = array denoting whether or not a peak has been resolved
//				x			-	long = x number of crystal
//				y			-	long = y number of crystal
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
// Returns:		long number of good neighbors
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Number_Neighbors(long *Flag, long x, long y, long XCrystals, long YCrystals)
{
	// local variables
	long i = 0;
	long j = 0;
	long Num = 0;

	// do not consider if it is set
	if (Flag[(y*XCrystals)+x] == 1) return 0;

	// loop through neighbors counting
	for (j = (y-1) ; j <= (y+1) ; j++) {

		// if beyond edge, then skip
		if ((j < 0) || (j >= YCrystals)) continue;

		// loop through columns
		for (i = (x-1) ; i <= (x+1) ; i++) {

			// if beyond edge, then skip
			if ((i < 0) || (i >= XCrystals)) continue;

			// if set then increment number of "Good" neighbors
			if (Flag[(j*XCrystals)+i] == 1) Num++;
		}
	}

	// return the number of neighbors found
	return Num;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Build_Line
// Purpose:		returns the number of neighbors for a peak
// Arguments:	vx			-	long = center point x of region
//				vy			-	long = center point y of region
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
//				Verts		-	long = verticies
//				*line_x		-	long = x coordinates of line around current region
//				*line_y		-	long = y coordinates of line around current region
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Build_Line(long vx, long vy, long XCrystals, long YCrystals, long *Verts, long *line_x, long *line_y)
{
	// local variables
	long i = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long index = 0;
	long min_x = 1;
	long max_x = PP_SIZE_X - 2;
	long min_y = 1;
	long max_y = PP_SIZE_Y - 2;

	// number of verticies per block
	Verts_X = (2 * XCrystals) + 1;
	Verts_Y = (2 * YCrystals) + 1;

	// index to current pixel
	index = (vy * Verts_X * 2) + (vx * 2);

	// if the master was found, only go half the distance out
	if (m_MasterFound == 1) {
		line_x[0] = (Verts[index+0] + Verts[index-(2 * Verts_X)-2]) / 2;
		line_y[0] = (Verts[index+1] + Verts[index-(2 * Verts_X)-1]) / 2;
		line_x[1] = (Verts[index+0] + Verts[index-(2 * Verts_X)-0]) / 2;
		line_y[1] = (Verts[index+1] + Verts[index-(2 * Verts_X)+1]) / 2;
		line_x[2] = (Verts[index+0] + Verts[index-(2 * Verts_X)+2]) / 2;
		line_y[2] = (Verts[index+1] + Verts[index-(2 * Verts_X)+3]) / 2;
		line_x[3] = (Verts[index+0] + Verts[index+2]) / 2;
		line_y[3] = (Verts[index+1] + Verts[index+3]) / 2;
		line_x[4] = (Verts[index+0] + Verts[index+(2 * Verts_X)+2]) / 2;
		line_y[4] = (Verts[index+1] + Verts[index+(2 * Verts_X)+3]) / 2;
		line_x[5] = (Verts[index+0] + Verts[index+(2 * Verts_X)-0]) / 2;
		line_y[5] = (Verts[index+1] + Verts[index+(2 * Verts_X)+1]) / 2;
		line_x[6] = (Verts[index+0] + Verts[index+(2 * Verts_X)-2]) / 2;
		line_y[6] = (Verts[index+1] + Verts[index+(2 * Verts_X)-1]) / 2;
		line_x[7] = (Verts[index+0] + Verts[index-2]) / 2;
		line_y[7] = (Verts[index+1] + Verts[index-1]) / 2;

	// otherwise, go the full distance
	} else {
		line_x[0] = Verts[index-(2 * Verts_X)-2];
		line_y[0] = Verts[index-(2 * Verts_X)-1];
		line_x[1] = Verts[index-(2 * Verts_X)-0];
		line_y[1] = Verts[index-(2 * Verts_X)+1];
		line_x[2] = Verts[index-(2 * Verts_X)+2];
		line_y[2] = Verts[index-(2 * Verts_X)+3];
		line_x[3] = Verts[index+2];
		line_y[3] = Verts[index+3];
		line_x[4] = Verts[index+(2 * Verts_X)+2];
		line_y[4] = Verts[index+(2 * Verts_X)+3];
		line_x[5] = Verts[index+(2 * Verts_X)-0];
		line_y[5] = Verts[index+(2 * Verts_X)+1];
		line_x[6] = Verts[index+(2 * Verts_X)-2];
		line_y[6] = Verts[index+(2 * Verts_X)-1];
		line_x[7] = Verts[index-2];
		line_y[7] = Verts[index-1];
	}
	line_x[8] = line_x[0];
	line_y[8] = line_y[0];

	// prevent bad values
	for (i = 0 ; i < 9 ; i++) {
		if (line_x[i] < min_x) line_x[i] = min_x;
		if (line_x[i] > max_x) line_x[i] = max_x;
		if (line_y[i] < min_y) line_y[i] = min_y;
		if (line_y[i] > max_y) line_y[i] = max_y;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Build_CRM
// Purpose:		returns a crystal region map from verticies
// Arguments:	Verts		-	long = verticies
//				XCrystals	-	long = number of crystals in x dimension per block
//				YCrystals	-	long = number of crystals in x dimension per block
//				CRM			-	unsigned char = crystal region map
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void Build_CRM(long *Verts, long XCrystals, long YCrystals, unsigned char *CRM)
{
	// local variables
	long i = 0;
	long x = 0;
	long y = 0;
	long Verts_X = 0;
	long Verts_Y = 0;
	long NumPixels = 0;
	long Crystal = 0;
	long SaveMasterFound = 0;

	long line_x[9];
	long line_y[9];
	long Region[65536];

	// save the master found flag and then turn it off (we want the full are for the CRM)
	SaveMasterFound = m_MasterFound;
	m_MasterFound = 0;

	// number of verticies per block
	Verts_X = (2 * XCrystals) + 1;
	Verts_Y = (2 * YCrystals) + 1;

	// default everything to crosstalk
	for (i = 0 ; i < PP_SIZE ; i++) CRM[i] = 255;

	// loop through the crystals
	for (y = 0 ; y < YCrystals ; y++) for (x = 0 ; x < XCrystals ; x++) {

		// Crystal Number
		Crystal = (y * XCrystals) + x;

		// get the border line
		Build_Line((2 * x) + 1, (2 * y) + 1, XCrystals, YCrystals, Verts, line_x, line_y);

		// Get the region
		NumPixels = Find_Region(line_x, line_y, Region);

		// fill in crystal number
		for (i = 0 ; i < NumPixels ; i++) CRM[Region[i]] = (unsigned char) Crystal;
	}

	// restore master found flag
	m_MasterFound = SaveMasterFound;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Side
// Purpose:		given a line from x1, y1 to x2, y2, which side does x,y fall on?
// Arguments:	x		-	long = x coordinate
//				y		-	long = y coordinate
//				x1		-	long = x coordinate position 1
//				y1		-	long = y coordinate position 1
//				x2		-	long = x coordinate position 2
//				y2		-	long = y coordinate position 2
// Returns:		long 0 left, 1 right
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

long Side(long x, long y, long x1, long y1, long x2, long y2)
{
	// local variables
	long dx = 0;
	long dy = 0;

	double slope = 0.0;
	double intercept = 0.0;
	double y_calculated = 0.0;

	// x and y differences
	dx = x2 - x1;
	dy = y2 - y1;

	// horizontal line
	if (dx == 0) {

		// line up
		if (dy >= 0) {

			// right is right
			if (x > x1) {
				return 1;

			// left is left
			} else {
				return 0;
			}

		// line down
		} else {

			// right is left
			if (x >= x1) {
				return 0;

			// left is right
			} else {
				return 1;
			}
		}

	// other lines we can calculate the slope and itercept
	} else {

		// calculate slope and intercept
		slope = (double) dy / (double) dx;
		intercept = (double) y1 - (slope * (double) x1);

		// calculate y from x using slope and intercept
		y_calculated = (slope * (double) x) + intercept;

		// if it is on the line, then it is to the left
		if ((double) y == y_calculated) {
			return 0;

		// below the line
		} else if ((double) y < y_calculated) {

			// line goes from left to right - below is right
			if (dx > 0) {
				return 1;

			// line is right to left - below is left
			} else {
				return 0;
			}

		// above the line
		} else {

			// line goes from left to right - above is left
			if (dx > 0) {
				return 0;

			// line is right to left - above is right
			} else {
				return 1;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		hist_equal
// Purpose:		apply histogram equalization to a data set
// Arguments:	NumElements	-	long = number of elements in data set
//				In			-	long = input array
//				Out			-	long = output array
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void hist_equal(long DirectScale, long NumElements, long *In, long *Out)
{
	// local variables
	long i = 0;
	long j = 0;
	long NumValues = 0;
	long Temp = 0;
	long Offset = 0;
	long First = 0;
	long MaxValue = 0;

	long *Ptr = NULL;

	long Count[65536];
	long Index[65536];
	long Value[65536];

	double Range = 256.0;
	double Accum = 0.0;
	double Remaining = 0.0;

	// histogram equalization
	if (0 == DirectScale) {

		// determine the values
		for (i = 0 ; i < NumElements ; i++) {

			// pointer to current value
			Ptr = &In[i];

			// search the existing list
			for (j = 0 ; (j < NumValues) && (Value[j] != *Ptr) ; j++);

			// if it was not in the list, add it to the end
			if (j == NumValues) {
				Value[j] = *Ptr;
				Count[j] = 1;
				NumValues++;

			// increment the number of times it occurred
			} else {
				Count[j]++;
			}
		}

		// sort the list
		for (i = 0 ; i < (NumValues-1) ; i++) for (j = (i+1) ; j < NumValues ; j++) {

			// if the later value is less than the first, then swap them
			if (Value[j] < Value[i]) {
				Temp = Value[j];
				Value[j] = Value[i];
				Value[i] = Temp;
				Temp = Count[j];
				Count[j] = Count[i];
				Count[i] = Temp;
			}
		}

		// if zero is the first value, then it gets index and the rest divide the rest
		if (Value[0] == 0) {
			Index[0] = 0;
			First = 1;
			Remaining = (double)(NumElements - Count[0]);
			Range = 255.0;

		// otherwise process all elements
		} else {
			Remaining = (double) NumElements;
		}

		// fill in the greyscale index for each value
		for (i = First ; i < NumValues ; i++) {

			// Calculate the new value
			Index[i] = (long) ((Accum / Remaining) * Range);

			// add current counts to the accumulator
			Accum += (double) Count[i];
		}

		// apply to outside value
		for (i = 0 ; i < NumElements ; i++) {
			Ptr = &In[i];
			for (j = 0 ; *Ptr != Value[j] ; j++);
			Out[i] = Index[j];
		}

	// direct scaling
	} else {

		// determine the maximum value
		for (MaxValue = 0, i = 0 ; i < NumElements ; i++) if (In[i] > MaxValue) MaxValue = In[i];

		// scale data
		for (i = 0 ; i < NumElements ; i++) Out[i] = (In[i] * 256) / (MaxValue + 1);
	}
}