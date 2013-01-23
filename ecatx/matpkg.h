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

#ifndef matpkg_h_defined
#define matpkg_h_defined
#include <math.h>

typedef struct matrix
	{
	  int ncols, nrows;
	  float *data;
	}
*Matrix;

typedef struct vol3d
	{
	  int xdim, ydim, zdim;
	  float voxel_size;
	  float *data;
	}
*Vol3d;

typedef struct stack3d
	{
	  int xdim, ydim, zdim;
	  float xy_size, z_size;
	  float *data;
	}
*Stack3d;

typedef struct view2d
	{
	  int xdim, ydim;
	  float x_pixel_size, y_pixel_size;
	  float *data;
	}
*View2d;

#if defined(__cplusplus)
extern "C" {
#endif
void matmpy(Matrix out, const Matrix in1, const Matrix in2);
void mat_print(const Matrix);
void mat_unity(Matrix);
Matrix mat_alloc(int ncols, int nrows);
void mat_copy(Matrix a, const Matrix b);
void rotate(Matrix a,float rx, float ry, float rz);
void translate(Matrix a,float tx, float ty, float tz);
void scale(Matrix a,float sx, float sy, float sz);
void mat_apply(Matrix a, float *x1, float *x2);
void mat_invert(Matrix out, Matrix const in);
int mat_cofactor(Matrix out, int col, int row, const Matrix in);
double mat_determinant(const Matrix a);
void mat_invert2(Matrix out, const Matrix in);
void mat_free(Matrix);
Vol3d make3d_volume();
Stack3d make3d_stack();
#if defined(__cplusplus)
}
#endif

#endif /* matpkg_h_defined */

