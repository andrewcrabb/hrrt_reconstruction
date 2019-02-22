/*
   * EcatX software is a set of routines and utility programs
   * to access ECAT(TM) data.
   *
   * Copyright (C) 2008 Merence Sibomana
   *
   * Author: Merence Sibomana <sibomana@gmail.com>
   *
   * ECAT(TM) is a registered trademark of CTI, Inc.
   * This software has been written from documentation on the ECAT
   * data format provided by CTI to customers. CTI or its legal successors
   * should not be held responsible for the accuracy of this software.
   * CTI, hereby disclaims all copyright interest in this software.
   * In no event CTI shall be liable for any claim, or any special indirect or 
   * consequential damage whatsoever resulting from the use of this software.
   *
   * This is a free software; you can redistribute it and/or
   * modify it under the terms of the GNU Lesser General Public License
   * as published by the Free Software Foundation; either version 2
   * of the License, or any later version.
   *
   * This software is distributed in the hope that it will be useful,
   * but  WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU Lesser General Public License for more details.

   * You should have received a copy of the GNU Lesser General Public License
   * along with this software; if not, write to the Free Software
   * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include "matpkg.h"

#define MATPKG_SIZE 4

static void sincos(double theta, double *sintheta, double *costheta)
{
        *sintheta = sin(theta);
        *costheta = cos(theta);
}

void matmpy(Matrix a, const Matrix b, const Matrix c)
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

void mat_print(const Matrix a)
{
	int i,j;

	for (j=0;j<a->nrows;j++)
	{ for (i=0;i<a->ncols;i++)
	    LOG_INFO("{}",a->data[i+j*a->ncols]);
	}
}

void mat_apply(Matrix a, float *x1, float *x2)
{
  int i,j;
 
	for (j=0;j<a->nrows;j++)
	{ x2[j] = 0;
    for (i=0;i<a->ncols;i++)
	    x2[j] += (float)(x1[i]*a->data[i+j*a->ncols]);
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

void mat_copy(Matrix a, const Matrix b)
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

int mat_cofactor(Matrix out, int col, int row, const Matrix in)
{
int i,j,k,l;

  if ((out->ncols != in->ncols-1) || (out->nrows != in->nrows-1)) return 0;
  for (j=0, l=0; j<in->nrows; j++) {
    if (j==row) continue;
    for (i=0, k=0; i<in->ncols; i++) {
      if (i==col) continue;
      out->data[k+l*out->nrows] = in->data[i+j*in->nrows];
      k++;
    }
    l++;
  }
  return 1;
}

double mat_determinant(const Matrix a)
{
  int i,j, sign;
  Matrix cf;
  double d=0;
  if (a->nrows != a->ncols) return 0;
  if (a->ncols==2) 
    return (a->data[0]*a->data[2] - a->data[1]*a->data[3]); 
  cf = mat_alloc(a->nrows-1,  a->ncols-1);
  for (j=0; j<a->nrows; j++) {
    for (i=0; i<a->ncols; i++) {
      if (((i+j)%2) == 0) sign = 1;
      else sign = -1;
      d += a->data[i+j*a->nrows] *sign * mat_determinant(cf);
    }
  }
  mat_free(cf);
  return d;
}


void mat_invert(Matrix out, Matrix const m)
{
  /* 
  Code extract from GLT, license below 
  */ 
 /*
 GLT OpenGL C++ Toolkit (LGPL)
 Copyright (C) 2000-2002 Nigel Stewart  

 Email: nigels@nigels.com   
  WWW:   http://www.nigels.com/glt/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
  /* NB. OpenGL Matrices are COLUMN major. */
#define MAT(m,r,c) (m)->data[(c)*4+(r)]

/* Here's some shorthand converting standard (row,column) to index. */
#define m11 MAT(m,0,0)
#define m12 MAT(m,0,1)
#define m13 MAT(m,0,2)
#define m14 MAT(m,0,3)
#define m21 MAT(m,1,0)
#define m22 MAT(m,1,1)
#define m23 MAT(m,1,2)
#define m24 MAT(m,1,3)
#define m31 MAT(m,2,0)
#define m32 MAT(m,2,1)
#define m33 MAT(m,2,2)
#define m34 MAT(m,2,3)
#define m41 MAT(m,3,0)
#define m42 MAT(m,3,1)
#define m43 MAT(m,3,2)
#define m44 MAT(m,3,3)

   int i;
   double det;
   double d12, d13, d23, d24, d34, d41;
   double tmp[16]; /* Allow out == in. */

   /* Inverse = adjoint / det. (See linear algebra texts.)*/

   /* pre-compute 2x2 dets for last two rows when computing */
   /* cofactors of first two rows. */
   d12 = (m31*m42-m41*m32);
   d13 = (m31*m43-m41*m33);
   d23 = (m32*m43-m42*m33);
   d24 = (m32*m44-m42*m34);
   d34 = (m33*m44-m43*m34);
   d41 = (m34*m41-m44*m31);

   tmp[0] =  (m22 * d34 - m23 * d24 + m24 * d23);
   tmp[1] = -(m21 * d34 + m23 * d41 + m24 * d13);
   tmp[2] =  (m21 * d24 + m22 * d41 + m24 * d12);
   tmp[3] = -(m21 * d23 - m22 * d13 + m23 * d12);

   /* Compute determinant as early as possible using these cofactors. */
   det = m11 * tmp[0] + m12 * tmp[1] + m13 * tmp[2] + m14 * tmp[3];

   /* Run singularity test. */
   if (det == 0.0) {
      /* LOG_INFO("{}matrix: Warning: Singular matrix.\n"); */
      mat_unity(out);
   }
   else {
      double invDet = 1.0 / det;
      /* Compute rest of inverse. */
      tmp[0] *= invDet;
      tmp[1] *= invDet;
      tmp[2] *= invDet;
      tmp[3] *= invDet;

      tmp[4] = -(m12 * d34 - m13 * d24 + m14 * d23) * invDet;
      tmp[5] =  (m11 * d34 + m13 * d41 + m14 * d13) * invDet;
      tmp[6] = -(m11 * d24 + m12 * d41 + m14 * d12) * invDet;
      tmp[7] =  (m11 * d23 - m12 * d13 + m13 * d12) * invDet;

      /* Pre-compute 2x2 dets for first two rows when computing */
      /* cofactors of last two rows. */
      d12 = m11*m22-m21*m12;
      d13 = m11*m23-m21*m13;
      d23 = m12*m23-m22*m13;
      d24 = m12*m24-m22*m14;
      d34 = m13*m24-m23*m14;
      d41 = m14*m21-m24*m11;

      tmp[8] =  (m42 * d34 - m43 * d24 + m44 * d23) * invDet;
      tmp[9] = -(m41 * d34 + m43 * d41 + m44 * d13) * invDet;
      tmp[10] =  (m41 * d24 + m42 * d41 + m44 * d12) * invDet;
      tmp[11] = -(m41 * d23 - m42 * d13 + m43 * d12) * invDet;
      tmp[12] = -(m32 * d34 - m33 * d24 + m34 * d23) * invDet;
      tmp[13] =  (m31 * d34 + m33 * d41 + m34 * d13) * invDet;
      tmp[14] = -(m31 * d24 + m32 * d41 + m34 * d12) * invDet;
      tmp[15] =  (m31 * d23 - m32 * d13 + m33 * d12) * invDet;

      /*memcpy(out, tmp, 16*sizeof(double)); */
      for (i=0; i<16; i++) out->data[i] = (float)tmp[i];
   }

#undef m11
#undef m12
#undef m13
#undef m14
#undef m21
#undef m22
#undef m23
#undef m24
#undef m31
#undef m32
#undef m33
#undef m34
#undef m41
#undef m42
#undef m43
#undef m44
#undef MAT
}
void mat_invert2(Matrix out, const Matrix in)
{
  LOG_INFO("{}ert2: Matrix inversion using Derterminant and coofactor, TBD\n");
  exit(1);
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
	LOG_INFO("{}ormer = [\n");
	mat_print(a);
	LOG_INFO("{}}
#endif
