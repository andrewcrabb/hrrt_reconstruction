#include <cstring>
#include <stdlib.h>
#include "polyfitw.h"
#include "matrix.h"

void polyfitw(float *x, float *y, float *w, int n, int m, float *cof) {
	float *b, *z,*rp;
	double tmp=0.0, sum=0.0;
	int i,j,k,p;
	Matrix <float> *CORRM=NULL;

	/*    CORRM = fltarr(M,M)         ;COEFF MATRIX
        B = fltarr(M)           ;WILL CONTAIN SUM Y * X^J
        Z = Replicate(1.0, N)
        xx = float(x)
	*/
	CORRM=new Matrix <float>(m+1, m+1);
	CORRM->identity();
	b = (float *)calloc(m+1,sizeof(float));
	//c = (float *)calloc(m+1,sizeof(float));
	z = (float *)calloc(n,sizeof(float));
	float vf[64];
	memcpy(vf, w, 256);
	for (i=0;i<n;i++)
	{
		z[i] = 1.f;  // set z = 1.0
		tmp += w[i]*y[i];
		sum += w[i];
	}

	/*
		B[0] = TOTAL(Y, DOUBLE = double)
		CORRM[0,0] = TOTAL(W)
	*/

	b[0] = (float)tmp;
	CORRM->set(0,0,(float)sum);

	/*
	FOR P = 1,2*NDEGREE DO BEGIN		;POWER LOOP.
		Z=Z*X							;Z IS NOW X^P
		IF P LT M THEN B[P] = TOTAL(W*Y*Z);B IS SUM Y*X\X^J
		SUM = TOTAL(W*Z)
		FOR J= 0 > (P-NDEGREE), NDEGREE < P DO CORRM[J,P-J] = SUM
	  END			;END OF P LOOP.

	CORRM = INVERT(CORRM, status)	;INVERT MATRIX.

  */
	for(p=1;p<=2*m;p++)
	{
		sum = 0.f; tmp=0.f;
		for (i=0;i<n;i++)
		{
			z[i] = z[i] * x[i];  // set z = 1.0
			tmp += w[i]*y[i]*z[i];
			sum += w[i]*z[i];
		}
		if (p < (m+1))
			b[p] = tmp;
		if(p-m < 0)
			k = 0;
		else
			k = p-m;
		for (j=k;j<=(p-k);j++)
			CORRM->set(j,p-j,sum);
	}

	rp = CORRM->data();
	// invert the matrix
	CORRM->invert();

	for(i=0;i<=m;i++)
	{	
		cof[i] = 0.f;
		for(j=0;j<=m;j++)
			cof[i] +=rp[i*(m+1) + j] * b[j];
	}
	
	//CORRM->~Matrix();
	free(b);
	free(z);
}
