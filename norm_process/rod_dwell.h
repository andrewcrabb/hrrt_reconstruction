/*
 rod_dwell.h
 Scope: HRRT only
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: Compute the rotating rod dwell
          
 Modification History:
        04/17/08: Option to set rod rotation radius r and center x0,y0			  
 */

#ifndef rod_dwell_h
#define rod_dwell_h
extern float rotation_radius,  rod_radius; // in mm
// Normalization rod rotation radii, the rod trajectory is a tube with 2 radii,
// where difference between the 2 radii is the rod diameter of 10mm.
extern float rotation_cx,  rotation_cy; // rod rotation center in mm
extern float rod_dwell(float x1, float y1,float x2, float y2);
#endif
