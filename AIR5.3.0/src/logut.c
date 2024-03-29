/* Copyright 2000-2002 Roger P. Woods, M.D. */
/* Modified 7/16/02 */

/*
 * void logut()
 *
 * Solves for z12=frechet derivative in the direction d12
 *
 * x11 and x22 are upper triangular matrices, possibly of different sizes
 *
 * x and a are square and upper triangular
 *
 * storage2 needs to be a pointer to 20 matrices, dimensioned 2x2 (for n<=4)
 *
 */

#include "AIR.h"

AIR_Error AIR_logut(const unsigned int n, double **ar, double **ai, double **xr, double **xi, const double *toproot, const double *botroot, double ***storage2)
{	
	double **x11r=*storage2++; /* 0 */
	double **x11i=*storage2++; /* 1 */
	
	double **x22r=*storage2++; /* 2 */
	double **x22i=*storage2++; /* 3 */

	double **a11r=*storage2++; /* 4 */
	double **a11i=*storage2++; /* 5 */
	
	double **a22r=*storage2++; /* 6 */
	double **a22i=*storage2++; /* 7 */
	
	double **a12r=*storage2++; /* 8 */
	double **a12i=*storage2++; /* 9 */
	
	{
		unsigned int j;
		
		for(j=0;j<n;j++){
		
			unsigned int i;
			
			for (i=0;i<n;i++){
			
				if(i==j){
					AIR_Error errcode=AIR_clog(ar[i][i],ai[i][i],&xr[i][i],&xi[i][i]);
					if(errcode!=0) return errcode;
				}
				else{
					xr[j][i]=0.0;
					xi[j][i]=0.0;
				}
			}
		}
	}

	if(n==2){
		/* First 1x1 block */
		x11r[0][0]=xr[0][0];
		x11i[0][0]=xi[0][0];
		x22r[0][0]=xr[1][1];
		x22i[0][0]=xi[1][1];
		
		a11r[0][0]=ar[0][0];
		a11i[0][0]=ai[0][0];
		a22r[0][0]=ar[1][1];
		a22i[0][0]=ai[1][1];
		
		a12r[0][0]=ar[1][0];
		a12i[0][0]=ai[1][0];

		{
			AIR_Error errcode=AIR_logderivut(1,1,a12r,a12i,x11r,x11i,x22r,x22i,a11r,a11i,a22r,a22i,toproot,botroot,storage2);
			if(errcode!=0) return errcode;
		}

		xr[1][0]=a12r[0][0];
		xi[1][0]=a12i[0][0];
		
		return 0;
	}
	else if(n==3){
		/* First 1x1 block */
		x11r[0][0]=xr[0][0];
		x11i[0][0]=xi[0][0];
		x22r[0][0]=xr[1][1];
		x22i[0][0]=xi[1][1];
		
		a11r[0][0]=ar[0][0];
		a11i[0][0]=ai[0][0];
		a22r[0][0]=ar[1][1];
		a22i[0][0]=ai[1][1];
		
		a12r[0][0]=ar[1][0];
		a12i[0][0]=ai[1][0];

		{
			AIR_Error errcode=AIR_logderivut(1,1,a12r,a12i,x11r,x11i,x22r,x22i,a11r,a11i,a22r,a22i,toproot,botroot,storage2);
			if(errcode!=0) return errcode;
		}

		xr[1][0]=a12r[0][0];
		xi[1][0]=a12i[0][0];
		
		/* 2x1 block */
		x11r[0][0]=xr[0][0];
		x11r[0][1]=xr[0][1];
		x11r[1][0]=xr[1][0];
		x11r[1][1]=xr[1][1];
		
		x11i[0][0]=xi[0][0];
		x11i[0][1]=xi[0][1];
		x11i[1][0]=xi[1][0];
		x11i[1][1]=xi[1][1];
		
		x22r[0][0]=xr[2][2];
		x22i[0][0]=xi[2][2];
		
		a11r[0][0]=ar[0][0];
		a11r[0][1]=ar[0][1];
		a11r[1][0]=ar[1][0];
		a11r[1][1]=ar[1][1];
		
		a11i[0][0]=ai[0][0];
		a11i[0][1]=ai[0][1];
		a11i[1][0]=ai[1][0];
		a11i[1][1]=ai[1][1];
		
		a22r[0][0]=ar[2][2];
		a22i[0][0]=ai[2][2];
		
		a12r[0][0]=ar[2][0];
		a12i[0][0]=ai[2][0];
		a12r[0][1]=ar[2][1];
		a12i[0][1]=ai[2][1];

		{
			AIR_Error errcode=AIR_logderivut(2,1,a12r,a12i,x11r,x11i,x22r,x22i,a11r,a11i,a22r,a22i,toproot,botroot,storage2);
			if(errcode!=0) return errcode;
		}

		xr[2][0]=a12r[0][0];
		xi[2][0]=a12i[0][0];
		xr[2][1]=a12r[0][1];
		xi[2][1]=a12i[0][1];
		
		return 0;
	}
	else if(n==4){
		/* First 1x1 block */
		x11r[0][0]=xr[0][0];
		x11i[0][0]=xi[0][0];
		x22r[0][0]=xr[1][1];
		x22i[0][0]=xi[1][1];
		
		a11r[0][0]=ar[0][0];
		a11i[0][0]=ai[0][0];
		a22r[0][0]=ar[1][1];
		a22i[0][0]=ai[1][1];
		
		a12r[0][0]=ar[1][0];
		a12i[0][0]=ai[1][0];

		{
			AIR_Error errcode=AIR_logderivut(1,1,a12r,a12i,x11r,x11i,x22r,x22i,a11r,a11i,a22r,a22i,toproot,botroot,storage2);
			if(errcode!=0) return errcode;
		}

		xr[1][0]=a12r[0][0];
		xi[1][0]=a12i[0][0];
		
		/* Last 1x1 block*/
		x11r[0][0]=xr[2][2];
		x11i[0][0]=xi[2][2];
		x22r[0][0]=xr[3][3];
		x22i[0][0]=xi[3][3];
		
		a11r[0][0]=ar[2][2];
		a11i[0][0]=ai[2][2];
		a22r[0][0]=ar[3][3];
		a22i[0][0]=ai[3][3];
		
		a12r[0][0]=ar[3][2];
		a12i[0][0]=ai[3][2];

		{
			AIR_Error errcode=AIR_logderivut(1,1,a12r,a12i,x11r,x11i,x22r,x22i,a11r,a11i,a22r,a22i,toproot,botroot,storage2);
			if(errcode!=0) return errcode;
		}

		xr[3][2]=a12r[0][0];
		xi[3][2]=a12i[0][0];
		
		/* 2x2 block */
		x11r[0][0]=xr[0][0];
		x11r[0][1]=xr[0][1];
		x11r[1][0]=xr[1][0];
		x11r[1][1]=xr[1][1];
		
		x11i[0][0]=xi[0][0];
		x11i[0][1]=xi[0][1];
		x11i[1][0]=xi[1][0];
		x11i[1][1]=xi[1][1];
		
		x22r[0][0]=xr[2][2];
		x22r[0][1]=xr[2][3];
		x22r[1][0]=xr[3][2];
		x22r[1][1]=xr[3][3];
		
		x22i[0][0]=xi[2][2];
		x22i[0][1]=xi[2][3];
		x22i[1][0]=xi[3][2];
		x22i[1][1]=xi[3][3];
		
		a11r[0][0]=ar[0][0];
		a11r[0][1]=ar[0][1];
		a11r[1][0]=ar[1][0];
		a11r[1][1]=ar[1][1];
		
		a11i[0][0]=ai[0][0];
		a11i[0][1]=ai[0][1];
		a11i[1][0]=ai[1][0];
		a11i[1][1]=ai[1][1];
		
		a22r[0][0]=ar[2][2];
		a22r[0][1]=ar[2][3];
		a22r[1][0]=ar[3][2];
		a22r[1][1]=ar[3][3];
		
		a22i[0][0]=ai[2][2];
		a22i[0][1]=ai[2][3];
		a22i[1][0]=ai[3][2];
		a22i[1][1]=ai[3][3];
		
		a12r[0][0]=ar[2][0];
		a12r[0][1]=ar[2][1];
		a12r[1][0]=ar[3][0];
		a12r[1][1]=ar[3][1];
		
		a12i[0][0]=ai[2][0];
		a12i[0][1]=ai[2][1];
		a12i[1][0]=ai[3][0];
		a12i[1][1]=ai[3][1];

		{
			AIR_Error errcode=AIR_logderivut(2,2,a12r,a12i,x11r,x11i,x22r,x22i,a11r,a11i,a22r,a22i,toproot,botroot,storage2);
			if(errcode!=0) return errcode;
		}

		xr[2][0]=a12r[0][0];
		xr[2][1]=a12r[0][1];
		xr[3][0]=a12r[1][0];
		xr[3][1]=a12r[1][1];
		
		xi[2][0]=a12i[0][0];
		xi[2][1]=a12i[0][1];
		xi[3][0]=a12i[1][0];
		xi[3][1]=a12i[1][1];
		
		return 0;
	}
	else return AIR_INVALID_UT_ERROR;
}
