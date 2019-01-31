#include <stdlib.h>
#include "polyfit.h"
#include "math_matrix.h"
void polyfit(float *x, float *y, int n, int m, float *cof)
{
	float *b, *z,*rp, tmp = 0.0, sum=0.0;
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
	for (i=0;i<n;i++)
	{
		z[i] = 1.f;  // set z = 1.0
		tmp += y[i];
	}

	/*
		B[0] = TOTAL(Y, DOUBLE = double)
		CORRM[0,0] = N
	*/

	b[0] = tmp;
	CORRM->set(0,0,n);

	/*
	FOR P = 1,2*NDEGREE DO BEGIN ;POWER LOOP.
		Z=Z*XX			;Z IS NOW X^P
                ;B IS SUM Y*X\X^J
		IF P LT M THEN B[P] = TOTAL(Y*Z)
		SUM = TOTAL(Z)
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
			tmp+=y[i]*z[i];
			sum += z[i];
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
