/* Copyright 1995-2002 Roger P. Woods, M.D. */
/* Modified: 7/24/02 */

/*
 * This defines a new average standard space that is the
 *  average of the reslice spaces of a set of .air files that
 *  share a common standard space.
 * 
 * Routine iterates until absolute values of all means are less than tiny, then
 *  iterates until failure to improve further.
 *
 * Storage2 needs to be allocated matrix3(2,2,20)
 * and storage4 needs to be allocated matrix3(4,4,13)
 *
 * Returns:
 *	0 if successful
 *	errcode if unsuccessful						
 */

#include "AIR.h"
#include <float.h>

AIR_Error AIR_comtloger(const AIR_Boolean affinesafe, double ***en, const unsigned int count, double **t, const double tiny, const unsigned int maxiters, double *accuracy, const double *toproot, const double *botroot, double ***storage2, double ***storage4)

{

	double **e=*storage4++;
	double **temp=*storage4++;
	double **temp3=*storage4++;

	/*Do the averaging*/
	{
		unsigned int j;

		for(j=0;j<4;j++){

			unsigned int i;

			for(i=0;i<4;i++){
				/* Start with first file as the estimated average */
				temp[j][i]=t[j][i]=en[0][j][i];
			}
		}
	}
	{
		unsigned int its;
		double fitold=DBL_MAX;
		AIR_Boolean small2=FALSE;
		unsigned int which=0;

		for(its=0;its<maxiters;its++){

			unsigned int ipvt[4];
			
			/*Minimize log*/

			/* Define estimated average as standard space */

			if(AIR_dgefa(t,4,ipvt)!=4) return AIR_SINGULAR_COMLOGER_ERROR;
			{
				unsigned int j;

				for(j=0;j<4;j++){

					unsigned int i;

					for(i=0;i<4;i++){
						temp3[j][i]=0.0;
					}
				}
			}

			{
				unsigned int k;

				for(k=0;k<count;k++){
				
					{
						unsigned int j;
						
						for(j=0;j<4;j++){
						
							unsigned int i;
						
							for(i=0;i<4;i++){
								
								e[j][i]=en[k][j][i];
							}
							AIR_dgesl(t,4,ipvt,e[j],FALSE);
							
						}
					}
					/*Find log of e*/
					{
						AIR_Error errcode;
						
						if(affinesafe){
							errcode=AIR_eloger5(e,toproot,botroot,storage2,storage4);
						}
						else{
							errcode=AIR_eloger4(4,e,toproot,botroot,storage2,storage4);
						}
						if(errcode!=0){
							which++;
							if(which<count){
								{
									unsigned int j;

									for(j=0;j<4;j++){

										unsigned int i;

										for(i=0;i<4;i++){
											/* Restart with next file as the estimated average */
											temp[j][i]=t[j][i]=en[which][j][i];
											temp3[j][i]=0.0;
										}
									}
								}
								if(AIR_dgefa(t,4,ipvt)!=4) return AIR_SINGULAR_COMLOGER_ERROR;
								k=0;
								its=0;
								continue;
							}
							else return errcode;
						}
					}
					{
						unsigned int j;

						for(j=0;j<4;j++){

							unsigned int i;

							for(i=0;i<4;i++){
								temp3[j][i]+=e[j][i];
							}
						}
					}
				}
			}
			{
				unsigned int j;

				for(j=0;j<4;j++){

					unsigned int i;

					for(i=0;i<4;i++){
						temp3[j][i]/=count;
						e[j][i]=temp3[j][i];
					}
				}
			}
			
			/*Find exp e*/
			{
				AIR_Error errcode=AIR_eexper4(4,e,toproot,botroot,storage2,storage4);

				if(errcode!=0) return errcode;
			}

			AIR_mulmat(temp,4,4,e,4,t);	// WARNING: temp, e, and t array structure assumptions are made

			/*Reset for next iteration*/
			{
				unsigned int j;

				for(j=0;j<4;j++){

					unsigned int i;

					for(i=0;i<4;i++){
						temp[j][i]=t[j][i];
					}
				}
			}

			if(!small2){
				small2=TRUE;
				{
					unsigned int j;

					for(j=0;j<4;j++){

						unsigned int i;

						for(i=0;i<4;i++){
							if(fabs(temp3[j][i])>tiny) small2=FALSE;
						}
					}
				}
			}
			else{
				double fitnew=0.0;
				{
					unsigned int j;

					for(j=0;j<4;j++){

						unsigned int i;

						for(i=0;i<4;i++){
							double tempvalue=fabs(temp3[j][i]);
							fitnew+=tempvalue*tempvalue;
						}
					}
				}
				if(fitnew>=fitold) break;
				fitold=fitnew;
			}
		} /* End of loop */
	}

	*accuracy=0.0;
	{
		unsigned int j;

		for(j=0;j<4;j++){

			unsigned int i;

			for(i=0;i<4;i++){
				if(fabs(temp3[j][i])>*accuracy) *accuracy=fabs(temp3[j][i]);
			}
		}
	}

	return 0;
}
