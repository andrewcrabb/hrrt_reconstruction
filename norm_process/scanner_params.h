/*
 Module scanner_param
 Scope: HRRT Only
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module provides constants parameters for the HRRT scanner
 Modification History:
 *   15-AUG-2006: Bug fix: replace 3 by NUM_COIN_HEADS in ga_elem inline method
 *                  to get the correct value for the last 2 opposite heads.
 *    09-APR-2009: Use full sinogram to store rotation dwell because the rod rotation
 *                 center is not necessary aligned with scanner center.
 *    30-Apr-2009: Use gen_delays_lib C++ library for rebinning and remove dead code
*/

# pragma once

#include <stdio.h>
#include <stdlib.h>

#define NLAYERS   2
#define NLAYERS2   NLAYERS*NLAYERS  // number of layer combinations(FF,FB,BF,BB)
#define NXCRYS  72
#define NYCRYS  104
#define NHEADS  8
#define NUM_COIN_HEADS 5    // number of opposite heads in coincidence

extern float *ce; // crystal efficiencies array
extern float *gr; // radial geometric factors array
extern short *cangle; // radial geometric factors array
extern float *ga; // axial geometric factors array

                     // Use full sinogram to store rotation dwell because the rod rotation
                     // center is not necessary aligned with scanner center.
extern float *omega_sino; // LOR correction for rod rotation dwell 
extern float *rdwell_sino; // LOR correction for rod rotation dwell 

#define NHCRYS NXCRYS*NYCRYS  // Number of crystals per head
#define MP_SIZE NXCRYS*NYCRYS*NXCRYS*NYCRYS  // Module Pair size
//module pair size: format is [NXCRYS,NYCRYS,NXCRYS,NYCRYS]
inline int mp_elem(int ax, int ay, int bx, int by)
{
	return (ay*NXCRYS+ax)*NHCRYS + by*NXCRYS+bx;
}

#define NUM_CRYSTALS NXCRYS*NLAYERS*NHEADS*NYCRYS
//Crystal efficiencies size: format is [NXCRYS,NLAYERS,NHEADS,NYCRYS]
inline float *ce_elem(float *data, int ax, int alayer, int ahead, int ay)
{
	int sz2=NXCRYS*NLAYERS;
	int sz3=NXCRYS*NLAYERS*NHEADS;
	int offset = ay*sz3 + ahead*sz2 +alayer*NXCRYS + ax;
	if (offset>= NUM_CRYSTALS)
	{
		printf("internal error: %d,%d,%d,%d = %d\n", ax,alayer,ahead,ax);
		exit(1);
	}
	return data + offset;
}


#define GR_SIZE NXCRYS*NXCRYS*NUM_COIN_HEADS*NLAYERS2*2
// radial geometric factors size: format is [NXCRYS,NXCRYS,NUM_COIN_HEADS,4,2]
// 2 is for each LOR's crystals
//
/*
inline float *gr_elem(int xi, int xj, int hk, int lc, int crystal)
return gr position for cystals xi, xj in the coincidence pair index hk,
layer combination lc
*/
inline float *gr_elem(int xi, int xj, int hk, int lc, int crystal)
{
	int sz2=NXCRYS*NXCRYS;
	int sz3=sz2*NUM_COIN_HEADS;
	int sz4=sz3*NLAYERS2;
  int off = crystal*sz4+lc*sz3 + hk*sz2 +xj*NXCRYS + xi;
  off = GR_SIZE-off;
	return gr + crystal*sz4+lc*sz3 + hk*sz2 +xj*NXCRYS + xi;
}

inline short *cangle_elem(int xi, int xj, int hk, int lc, int crystal)
{
	int sz2=NXCRYS*NXCRYS;
	int sz3=sz2*NUM_COIN_HEADS;
	int sz4=sz3*NLAYERS2;
  int off = crystal*sz4+lc*sz3 + hk*sz2 +xj*NXCRYS + xi;
  off = GR_SIZE-off;
	return cangle + crystal*sz4+lc*sz3 + hk*sz2 +xj*NXCRYS + xi;
}
#define GA_SIZE NXCRYS*NYCRYS*NYCRYS*NUM_COIN_HEADS*NLAYERS2
// axial geometric factors size: format is [NXCRYS,NYCRYS,NYCRYS,NUM_COIN_HEADS,4]
// NXCRYS: incident angles are sampled for each crystal
// NYCRYS,NYCRYS : LOR's crystals axial location
// Could be calculated for 3 opposite heads and obtain the last 2 by symetry. This
// would reduce space required and will be investigated later
//    ex: for head 0, only 2,3,4 would be  calculated; 5 would be obtained from 3
//    and 6 would be  obtained from 2.
// 4: Layers combinations (FF,FB,BF,BB)
//
inline float *ga_elem(int xj, int yi, int yj, int hk, int lc)
{
	int sz2=NXCRYS*NYCRYS;
	int sz3=sz2*NYCRYS;
	int sz4=sz3*NUM_COIN_HEADS;
	return ga + lc*sz4+hk*sz3 + yj*sz2 +yi*NXCRYS + xj;
}

// (ahead,bhead) is the module pair where bhead is greater than ahead

extern float omega_elem(int mp, int ax, int bx);
extern float dwell_elem(int mp, int ax, int bx);

inline float sq(float v) { return v*v; }
inline double sq(double v) { return v*v; }

