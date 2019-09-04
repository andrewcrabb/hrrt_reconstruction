/*
 Module cal_sen
 Scope: General
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module computes iteratively crystal efficiencies from fan sums 
 Modification History:
 02-aug-07 : change cal_sen() to return 1 on success and 0 on failure

*/

# pragma once

#include <stdio.h>
extern int max_iterations;
extern float min_rmse;

extern int cal_sen(float *in_fan_sum, float *out_cry_eff, FILE *log_fp);
//  cal_sen computes iteratively crystal efficiencies from fan sums 
