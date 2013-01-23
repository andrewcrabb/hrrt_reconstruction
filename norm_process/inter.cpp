/*
 Module inter
 Scope: General
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module provides a function to find intersections of a ray  wrt a circle center=0,0, radius = r
 Modification History:

*/

#include <math.h>
#include "inter.h"
#include "scanner_params.h"

// float inter(float x1, float y1, float x2, float y2, float r,
//find the intersections of a ray wrt a circle center=0,0, radius = r
// (x1,y1) and (x2,y2) are LOR detectors coordinates,
// the detectors are different and distance between them is greater than 0
//
float inter(float x1, float y1, float x2, float y2, float r,
			float *xi, float *yi)
{
	// algorithm 1: secant line, Rhoad et al, 1984, p.429
	double dx = x2-x1;
	double dy = y2-y1;
	double m = x1*y2-x2*y1;
	double d2 = dx*dx + dy*dy;
	double mr2 = sq(r)*d2 - sq(m);
	if (mr2>0)
	{
		int sgn = dy<0? -1 : 1;
		double abs_dy = dy<0? -dy:dy;
		xi[0] = (float)((m*dy-sgn*dx*sqrt(mr2))/d2);
		yi[0] = (float)((-m*dx - abs_dy*sqrt(mr2))/d2);
		xi[1] = (float)((m*dy+sgn*dx*sqrt(mr2))/d2);
		yi[1] = (float)((-m*dx + abs_dy*sqrt(mr2))/d2);
		return (float)sqrt(sq(xi[0]-xi[1])+sq(yi[0]-yi[1]));
	}
	return 0.0f;
}

