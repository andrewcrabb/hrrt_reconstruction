/*
 Module geo_compute
 Scope: HRRT only
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module reads HRRT geometry coefficients and computes the geometric factor to be applied to LOR
          and before fan sum
 Modification History:
 28-May-2009: Added new geometric profiles format using 4 linear segments cosine
*/
#ifndef get_gs_h
#define get_gs_h
#include <stdio.h>
extern int get_gs(const char *geom_fname, FILE *log_fp);
#endif
