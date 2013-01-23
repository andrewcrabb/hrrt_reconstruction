/*
 Module fan_sum
 Scope: HRRT only
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module provides 2 fansum functions that apply geometric and dwell corrections to a list of LOR events
          and update the crystal fan sum
 Modification History:
        04/30/09: Use gen_delays_lib C++ library for rebinning and remove dead code
                  Integrate Peter Bloomfield _LINUX support

*/
#ifndef fan_sum_h
#define fan_sum_h
#define OBLIQUE_C 0x1
#define RDWELL_C  0x2
#define OMEGA_C   0x4
#define GR_C      0x8
#define GA_C      0x10

struct FS_L64_Args
{
	unsigned *in;
  unsigned *x_histogram;
  unsigned *y_histogram;
	int count;
	int TNumber;
	float *out;
};
//  fan_sum function to apply geometric and dwell corrections to a module pair data into sum
extern int fan_sum(unsigned *events, int count, float *sum, unsigned corrections);
//  fan_sum function to apply geometric and dwell corrections to events stream into sum
typedef enum {PROMPT_EVENT=0, DELAYED_EVENT=1, TAG_EVENT=2, SYNC_EVENT=3} EventType;
static unsigned int ewtypes[16] = {3,3,1,0,3,3,2,2,3,3,3,3,3,3,3,3};
extern float *fan_sum_weight;
static float  eps = 1.0e-4f;
#endif

