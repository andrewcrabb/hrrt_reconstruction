/*
 Module fan_sum
 Scope: HRRT only
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module provides 2 fansum functions that apply geometric and dwell corrections to a list of LOR events
          and update the crystal fan sum
 Modification History:
        06/29/06: Move the cosine initialization code duplicated in the 2 fansum functions
		              into init_cosine function.
        08/06/08: Bug fix: cosine correction was wrong (multiplied instead of divided)
                  in event fansum function.
        04/30/09: Use gen_delays_lib C++ library for rebinning and remove dead code
                  Integrate Peter Bloomfield __linux__ support
*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "fan_sum.h"
#include "scanner_params.h"
#include <gen_delays_lib/gen_delays.h>
#include <gen_delays_lib/geometry_info.h>

#define MIN_COSINE 0.999
#define FANSUM_RD 10
#define FANSUM_UD NYCRYS-FANSUM_RD-1
#define FANSUM_SPAN 2*FANSUM_RD+1
static float *cosine=NULL;

static void init_cosine(unsigned corrections)
{
	int i;
	if (cosine == NULL) 
	{
		cosine = (float*)calloc(NYCRYS, sizeof(float));
		if (corrections&OBLIQUE_C)
		{
			
			for (i=0; i<NYCRYS; i++) 
			{
				int bcrys =  i%8;
				int blk   =  i/8;
				float dz     =  (float)(blk*(BSIZE+BGAP)+bcrys*(CSIZE+CGAP));
				cosine[i] = (float)(cos(atan(dz/(2*m_crystal_radius))));
			}
		}
		else
		{
			for (i=0; i<NYCRYS; i++) 
				 cosine[i] = 1.0f;
		}
	}
}

//
// int fan_sum(unsigned *events, float *sum, unsigned corrections)
//  fan_sum applies geometric and dwell corrections to event buffer
//  The function updates the fan sum with LOR events in a given buffer after 
//  applying geometric and dwell corrections. 
//
int fan_sum(unsigned *event, int count, float *sum, unsigned corrections)
{
	//
	// apply rotation dwell, solid angle, oblique and geometric radial corrections
	//
	float gra=1.0f, grb=1.0f, gaa=1.0;
	float so=1.0f, live=1.0f;
	int ax=0, ay=0, bx=0, by=0, alayer, blayer, lc;
	int dy;   //ay,by distance
	int ahead, bhead;
	float cor=0.0f, ccor=1.0f; //  correction factor
	unsigned ew1=0, ew2=0, type=0;
	int i=0, mp=0, pos=0;

	// Obliqueness computed as function of ring difference
	// For more accuracy, it should be computed for each LOR
	if (cosine == NULL) init_cosine(corrections);

	while (pos<count-1)
	{
		ew1 = event[pos];
		ew2 = event[pos+1];
		type = ewtypes[(((ew2&0xc0000000)>>30)|((ew1&0xc0000000)>>28))];
		switch(type)
		{
		case 3: // not in sync
			pos += 1;
			break;
		case 2:	// tag word
		   pos += 2;
		   break;
		default: // event: 0=prompt, 1=delayed
		   pos += 2;
		   mp = ((ew1&0x00070000)>>16) | ((ew2&0x00070000)>>13);
		   ax = (ew1&0xff);
		   ay = ((ew1&0xff00)>>8);
		   bx = (ew2&0xff);
		   by = ((ew2&0xff00)>>8);
		   alayer = (ew1&0x01C00000)>>22;
		   blayer = (ew2&0x01C00000)>>22;
		   // Jeih-San layer numbering inverted
		   alayer = alayer? 0: 1; 
		   blayer = blayer? 0: 1;
		   lc = alayer*2+blayer;
		   ahead = hrrt_mpairs[mp][0];
		   bhead = hrrt_mpairs[mp][1];
		   if (mp>0 && mp<=nmpairs)
		   {
			   cor = type? -1.0f:1.0f;
			   if (corrections&GR_C) //geometric radial 
			   {
				   gra = *(gr_elem(ax, bx, bhead-ahead-2, lc, 0));
				   grb = *(gr_elem(ax, bx, bhead-ahead-2, lc, 1));
			   }
			   if (corrections&GA_C) gaa = *(ga_elem(bx, ay, by, bhead-ahead-2, lc)); //geometric axial
			   if (corrections&OMEGA_C) so = omega_elem(mp, ax, bx); // solid angle
			   if (corrections&RDWELL_C) live = dwell_elem(mp, ax, bx); // rotation dwell

			   dy = abs(by-ay);
			   if (live>eps )
			   {
				   cor =  so/(live*gra*grb*gaa*gaa*cosine[dy]);
           if (type == DELAYED_EVENT) cor *= -1;        // substract for delayed event
				   *ce_elem(sum, ax, alayer, ahead, ay) += cor;
				   *ce_elem(sum, bx, blayer, bhead, by) += cor;
			   }
		   }
		   break;
		}
	}
	return 1;
}
