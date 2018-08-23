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
#include <sys/types.h>
#include <stdlib.h>
#include "matrix.h"
#include "matrix_utils.h"

#if defined(__cplusplus)
extern "C" {
#endif
int  matrix_resize(MatrixData *mat, float pixel_size, int interp_flag);
int matrix_resize_1( MatrixData* slice, float pixel_size, int interpolate);
int matrix_resize_2( MatrixData* slice, float pixel_size, int interpolate);
int matrix_resize_4( MatrixData* slice, float pixel_size, int interpolate);
#if defined(__cplusplus)
}
#endif
#if defined(MATRIX_RESIZE_1)
#define ELEM_TYPE unsigned char
int matrix_resize_1( MatrixData* slice, float pixel_size, int interpolate)
#endif
#if defined(MATRIX_RESIZE_2)
#define ELEM_TYPE short
int matrix_resize_2( MatrixData* slice, float pixel_size, int interpolate)
#endif
#if defined(MATRIX_RESIZE_4)
#define ELEM_TYPE float
int matrix_resize_4( MatrixData* slice, float pixel_size, int interpolate)
#endif
#if defined(MATRIX_RESIZE_1) || defined(MATRIX_RESIZE_2) || defined(MATRIX_RESIZE_4)
{
	int i1, i2, j, k;
	int sx1, size1, sx2;
	int sx1a, sx2a;
	float x1, x2; 
	ELEM_TYPE y1;
	float t, u;
	ELEM_TYPE *ya, *y;
	ELEM_TYPE *p0, *p1, *dest;
	float dx1, dx2;

/*
 * !!!!!!!!!!!!!! AVOID DIVISIONS FOR SPEED, USE MULTIPLY, EVEN FOR INTEGERS 
 */

/* resize input slice to display voxel size,
 * align lines to 4 bytes border ==> line size = size1 ((sx+3)/4)*4
 */
 
	sx1 = (int)(0.5 + slice->xdim*slice->pixel_size/pixel_size);
	sx2 = (int)(0.5 + slice->ydim*slice->y_size/pixel_size);
	dx1 = pixel_size/slice->pixel_size;
	dx2 = pixel_size/slice->y_size;
	sx1a = slice->xdim; sx2a = slice->ydim;
	size1 = ((sx1+3)/4)*4;
/*
 * This is a general purpose function, 4 byte alignment must be done
 * at display refresh (Sibomana@topo.ucl.ac.be 10-aug-1998) ==> size1 = sx1;
 */
	size1 = sx1;
	slice->pixel_size = slice->y_size = pixel_size;
	if (slice->data_ptr == NULL) {
		slice->xdim = size1;
		slice->ydim = sx2;
		return 1;
	}

/* linear interpolation in x dimension */
	if (sx1a != sx1) {
		ya = (ELEM_TYPE*)slice->data_ptr;
		y = (ELEM_TYPE*)calloc(size1*sx2a,sizeof(ELEM_TYPE));
		for (i1=0; i1<sx1; i1++) {
			x1 = dx1 * i1;
			j = (int) x1;
			t = (x1 -j);
			p0 = ya+j;
 			if (j+1 < sx1a) p1 = ya+j+1;
			else p1 = p0;						/*  overflow, use last column */
			dest = y+i1;
			if (interpolate) { 					/*	linear interpolation */
				for (i2=0; i2<sx2a; i2++) {
					*dest = (ELEM_TYPE)((1-t)*(*p0) + t*(*p1));
					p0 += sx1a;
					p1 += sx1a;
					dest += size1;
				}
			} else {								/* nearest neighbour */
				if (t<0.5) {
					for (i2=0; i2<sx2a; i2++) {
						*dest = *p0;
						p0 += sx1a;
						dest += size1;
					}
				} else {
					for (i2=0; i2<sx2a; i2++) {
						*dest = *p1;
						p1 += sx1a;
						dest += size1;
					}
				}
			}
		}
		slice->xdim = sx1;
		slice->data_ptr = (void *)y;
		free(ya);
	}
/* linear interpolation in y dimension */
	if (sx2a != sx2) {
		ya = (ELEM_TYPE*)slice->data_ptr;
		y = (ELEM_TYPE*)calloc(size1*sx2,sizeof(ELEM_TYPE));
		for (i2=0; i2<sx2; i2++) {
			x2 = dx2 * i2;
			k = (int)x2;
			u = (x2 - k);
			p0 = ya + k*size1;
			if (k+1<sx2a) p1 = ya + (k+1)*size1;
			else p1 = p0;						/*  overflow, use last line */
			dest = y + i2*size1;
			if (interpolate) {
				for (i1=0; i1<sx1; i1++) {
					*dest++ = (ELEM_TYPE)((1-u)*(*p0++) + u*(*p1++));
				}
			} else {
				for (i1=0; i1<sx1; i1++)
					*dest++ = *p0++;
			}
		}
		slice->ydim = sx2;
		slice->data_ptr = (void *)y;
		free(ya);
	}
	if (sx1<size1) {			/* fill line with last value */
		for (i2=0; i2<sx2; i2++) {
			y1 = y[i2*size1+sx1-1];
			dest = y + i2*size1+sx1;
			for (i1=sx1; i1<size1; i1++) *dest++ = y1;
		}
		slice->xdim = size1;
	}
	return 1;
}
#endif
