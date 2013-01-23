/* static char sccsid[] = "@(#)matpkg.c	1.1 UCL-TOPO 98/08/11"; */

/*
 * ANSI version created by Sibomana@topo.ucl.ac.be
 * Date : 11/15/96
 * Reason : compilation problems on Solaris2.5
 */

#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include "matpkg.h"

#ifdef WIN32
#define M_PI            3.14159265358979323846

static void sincos(double theta, double *sintheta, double *costheta)
{
        *sintheta = sin(theta);
        *costheta = cos(theta);
}
#endif

void matmpy(Matrix a,Matrix b,Matrix c)
{	int i,j,k;
	float t;

	for (i=0; i<b->ncols; i++)
	  for (j=0; j<c->nrows; j++)
	  { t = 0.0;
	    for (k=0; k<b->nrows; k++)
		t += b->data[i+k*b->ncols]*c->data[k+j*c->ncols];
	    a->data[i+j*a->ncols]=t;
	  }
}

void mat_print(Matrix a)
{
	int i,j;

	for (j=0;j<a->nrows;j++)
	{ for (i=0;i<a->ncols;i++)
	    printf("%13.6g ",a->data[i+j*a->ncols]);
	  printf("\n");
	}
}

void mat_apply(Matrix a, float *x1, float *x2)
{
  int i,j;

 
	for (j=0;j<a->nrows;j++)
	{ x2[j] = 0;
    for (i=0;i<a->ncols;i++)
	    x2[j] += x1[i]*a->data[i+j*a->ncols];
	}
}

void mat_unity(Matrix a)
{
	int i,j;

	for (j=0; j<a->nrows; j++)
	  for (i=0; i<a->ncols; i++)
	    a->data[i+j*a->ncols]=(i==j)?1.0F:0.0F;
}

Matrix mat_alloc(int ncols,int nrows)
{
	Matrix t;

	t=(Matrix)malloc(sizeof(struct matrix));
	t->ncols=ncols;
	t->nrows=nrows;
	t->data = (float*)malloc(ncols*nrows*sizeof(float));
	mat_unity(t);
	return t;
}

void mat_copy(Matrix a,Matrix b)
{
	int i;

	for (i=0; i<a->ncols*a->nrows; i++)
	  a->data[i] = b->data[i];
}

void rotate(Matrix a,float rx,float ry,float rz)
{
	Matrix t,b;
	double sint,cost;

	t=mat_alloc(4,4);
	b=mat_alloc(4,4);
	mat_unity(b);
	sincos(rx*M_PI/180.,&sint,&cost);
	b->data[5]=(float)cost;
	b->data[6] = (float)-sint;
	b->data[9]=(float)sint;
	b->data[10]=(float)cost;
	matmpy(t,a,b);
	mat_unity(b);
	sincos(ry*M_PI/180.F,&sint,&cost);
	b->data[0]=(float)cost;
	b->data[2]=(float)sint;
	b->data[8] = (float)-sint;
	b->data[10]=(float)cost;
	matmpy(a,t,b);
	mat_unity(b);
	sincos(rz*M_PI/180,&sint,&cost);
	b->data[0]=(float)cost;
	b->data[1] = (float)-sint;
	b->data[4]=(float)sint;
	b->data[5]=(float)cost;
	matmpy(t,a,b);
	mat_copy(a,t);
	mat_free(t);
	mat_free(b);
}

void translate(Matrix a,float dx,float dy,float dz)
{
	Matrix b,t;

	b=mat_alloc(4,4);
	t=mat_alloc(4,4);
	mat_copy(b,a);
	mat_unity(t);
	t->data[3]=dx;
	t->data[7]=dy;
	t->data[11]=dz;
	matmpy(a,b,t);
	mat_free(b);
	mat_free(t);
}

void scale(Matrix a,float sx,float sy,float sz)
{
	Matrix b,t;

	b=mat_alloc(4,4);
	t=mat_alloc(4,4);
	mat_copy(b,a);
	mat_unity(t);
	t->data[0]=sx;
	t->data[5]=sy;
	t->data[10]=sz;
	matmpy(a,b,t);
	mat_free(b);
	mat_free(t);
}

void mat_free(Matrix a)
{
	free(a->data);
	free(a);
}

#ifdef TEST
main()
{
	Matrix a, x;

	a = mat_alloc(4,4);
	mat_unity(a);

	translate(a,-.5F,-.5F,-.5F);
	mat_print(a);
	rotate(a,5.,-10.0F,15.0F);
	mat_print(a);
	scale(a,.25F,.25F,.25F);
	mat_print(a);
	printf(" Transformer = [\n");
	mat_print(a);
	printf("]\n");
}
#endif
