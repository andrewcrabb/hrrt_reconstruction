/*
 rod_dwell.cpp
 Scope: HRRT only
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: Compute the rotating rod dwell
          
 Modification History:
        04/17/08: Option to set rod rotation radius r and center x0,y0			  
 */
#include <math.h>
#include <stdio.h>
#include "inter.h"
#include "rod_dwell.h"
#include "scanner_params.h"

// Normalization rod rotation radius
float rotation_radius = 155.0f;  //mm
float rod_radius=5.0f;          //mm
float rotation_cx=0.0f, rotation_cy=0.0f;  // rotation center offset in mm

inline float xoff(float x) { return x-rotation_cx; }  
inline float yoff(float y) { return y-rotation_cy; }

//float rod_dwell(float x1, float y1,float x2, float y2)
// Return the rod rotation dwell correction for a 2D LOR
float rod_dwell(float x1, float y1,float x2, float y2)
{
	float inner_xi[2], inner_yi[2];  // LOR and inner circle intersection coordinates
	float outer_xi[2], outer_yi[2];  // LOR and outer circle intersection coordinates
  float ri = rotation_radius-rod_radius; // inner radius
  float ro = rotation_radius+rod_radius; // outer radius

	float w1 = inter(xoff(x1),yoff(y1),xoff(x2),yoff(y2),ri,inner_xi,inner_yi);
	float w2 = inter(xoff(x1),yoff(y1),xoff(x2),yoff(y2),ro,outer_xi,outer_yi);
	return (w2-w1); // mm
}

