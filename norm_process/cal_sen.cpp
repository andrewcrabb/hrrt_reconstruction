/*
 Module cal_sen
 Scope: General
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module computes iteratively crystal efficiencies from fan sums 
 Modification History:
 31-JUL-2007: Make max_iterations and min_rmse variable
 30-Apr-2009: Use gen_delays_lib C++ library for rebinning and remove dead code
              Integrate Peter Bloomfield _LINUX support

*/
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "cal_sen.h"
#include "scanner_params.h"
int max_iterations=30;
float min_rmse=10e-9f;

static double head_sum(float *ce, int head)
{
	int x, y, layer;
	double sum = 0;
	for (layer=0; layer<NLAYERS; layer++)
		for (x=0; x<NXCRYS; x++)
			for (y=0; y<NYCRYS; y++)
				sum += *ce_elem(ce,x,layer,head,y);
	return sum;
}


int cal_sen(float *fan_sum, float *cry_eff, FILE *log_fp)
{
	int ax, ay, ahead, alayer, bhead;
	int i=0, it=0;
	double rmse=0.0, max_rmse=0.0;
	double sum=0;
	float *old_ce = (float*)calloc(NUM_CRYSTALS, sizeof(float));

	// Initialize all efficiencies to 1;
	for (i=0; i<NUM_CRYSTALS; i++) cry_eff[i] = old_ce[i] = 1.0f;

	// Iterations
	for (it=0; it<max_iterations; it++)
	{
		for (ahead=0; ahead<NHEADS; ahead++)
		{
			sum = 0;
			for (int chead=0; chead<NUM_COIN_HEADS; chead++)
			{
				bhead = (ahead+chead+2)%NHEADS;
				sum += head_sum(cry_eff, bhead);
			}
			for (alayer=0; alayer<NLAYERS; alayer++)
			{	
				for (ay=0; ay<NYCRYS; ay++)
				{
					for (ax=0; ax<NXCRYS; ax++)
					{
						float *pce = ce_elem(cry_eff, ax, alayer, ahead, ay);
						float *pfs = ce_elem(fan_sum, ax, alayer, ahead, ay);
						*pce = (float)((*pfs)/sum);
					}
				}
			}
			if (it>0) fprintf(log_fp, "%d=%g ", ahead, sum);
		}
		fprintf(log_fp, "\n");

		//
		// compute deviation
		sum = max_rmse = sq(cry_eff[0]-old_ce[0]);
		for (i=1; i<NUM_CRYSTALS; i++)
        {
            double d = sq(cry_eff[i]-old_ce[i]);
            if (d>max_rmse) max_rmse = d;
            sum += d;
        }
		sum /= NUM_CRYSTALS;
		rmse = sqrt(sum);
        max_rmse = sqrt(max_rmse);
		fprintf(log_fp, "cal sen: Iteration %d RMSE %g (max %g)\n", it, rmse,max_rmse);
        fflush(log_fp);

		// save current iteration
		memcpy(old_ce, cry_eff, NUM_CRYSTALS*sizeof(float));
		if (rmse<min_rmse) 
		{
			fprintf(log_fp, "Algorithm has converged at iteration %d\n",it+1);
			break;
		}
	}
	free(old_ce);
    if (it==max_iterations) 
    {
	    fprintf(log_fp, "Algorithm didn't converged in %d iterations %d\n",
            max_iterations);
        // return 0;
    }

	// calculate mean and scale crystal efficiencies to 1.0
	double mean_ce = cry_eff[0];
	for (i=1; i<NUM_CRYSTALS; i++) mean_ce += cry_eff[i];
	mean_ce /= NUM_CRYSTALS;
	for (i=0; i<NUM_CRYSTALS; i++) cry_eff[i] = (float)(cry_eff[i]/mean_ce);
	return 1;
}
