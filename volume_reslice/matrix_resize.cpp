/* static char sccsid[] = "@(#)matrix_resize.c	1.2 UCL-TOPO 99/08/16"; */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "matrix_resize.h"

// T is unsigned int
static unsigned interpolate_rgb(int v0, int v1, float t)
{
	int r0, g0, b0, r1, b1, g1, r, g, b;
	unsigned v;
	r0 = 0xff & (v0) ;
	g0 = 0xff & (v0 >> 8) ;
	b0 = 0xff & (v0 >> 16) ;
	r1 = 0xff & (v1) ;
	g1 = 0xff & (v1 >> 8) ;
	b1 = 0xff & (v1 >> 16) ;
	r = (int)((1-t)*r0 + t * r1);
	g = (int)((1-t)*g0 + t * g1);
	b = (int)((1-t)*b0 + t * b1);
	v = r + (g<<8) + (b<<16);
	return v;
}

typedef unsigned (*interpolate_func)(int v0, int v1, float t);

template <typename T>
int matrix_resize_(T *src, MatrixData* slice, float pixel_size,
int interpolate, int align, interpolate_func func=0)
{
	int i1, i2, j, k;
	int sx1, size1, sx2;
	int sx1a, sx2a;
	float x1, x2; 
	T y1;
	float t, u;
	T *ya, *y;
	T *p0, *p1, *dest;
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
	if (align) size1 = ((sx1+3)/4)*4;
	else size1 = sx1;
	slice->pixel_size = slice->y_size = pixel_size;
	if (src == NULL) {
		slice->xdim = size1;
		slice->ydim = sx2;
		return 1;
	}

/* linear interpolation in x dimension  or align to 4 bytes boundary*/
	ya = src;
	if (sx1a != sx1) {
		y = (T*)calloc(size1*sx2a,sizeof(T));
		for (i1=0; i1<sx1; i1++) {
            x1 = dx1 * (i1+0.5F);  // i1+0.5 = destination mid pixel
            j = (int) (x1 - 0.5); // j = source previous mid pixel
            t = x1 -(j+0.5F);
			if (j>0) p0 = ya+j;
			else p0 = ya;  // left fill with first pixel
 			if (j+1 < sx1a) p1 = ya+j+1;
			else p1 = p0;			// right fill with last pixel
			dest = y+i1;
			if (interpolate) { 		/*	linear interpolation */
				for (i2=0; i2<sx2a; i2++) {
					if (func == 0)
						*dest = (T)((1-t)*(*p0) + t*(*p1));
					else *dest = (T)((*func)((int)(*p0),(int)(*p1),t));
					p0 += sx1a;
					p1 += sx1a;
					dest += size1;
				}
			} else {			/* nearest neighbour */
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
		slice->xdim = size1;
		slice->data_ptr = (void *)y;
		free(ya);
		ya = y;
	} else if (sx1<size1) {		/* Align to 4 bytes boundary */
		y = (T*)calloc(size1*sx2a,sizeof(T));
		for (i2=0; i2<sx2a; i2++) 
			memcpy(y+i2*size1, src+i2*sx1, sx1*sizeof(T));
		slice->xdim = size1;
		slice->data_ptr = (void *)y;
		free(ya);
		ya = y;
	}

/* linear interpolation in y dimension */
	if (sx2a != sx2) {
		y = (T*)calloc(size1*sx2,sizeof(T));
		for (i2=0; i2<sx2; i2++) {
            x2 = dx2 * (i2 + 0.5F); // i2+0.5 = destination mid pixel
            k = (int)(x2-0.5);     // k+0.5 = source previous mid pixel
            u = x2 -(k+0.5F);
            if (k> 0) p0 = ya + k*size1;
            else p0 = ya;       // fill up with first line
			if (k+1<sx2a) p1 = ya + (k+1)*size1;
			else p1 = p0;		//  Fill down with last line
			dest = y + i2*size1;
			if (interpolate) {
				for (i1=0; i1<sx1; i1++) {
					if (func == 0)
						*dest++ = (T)((1-u)*(*p0++) + u*(*p1++));
					else *dest = (T)((*func)((int)(*p0++),(int)(*p1++),t));
				}
			} else {           // nearest neighbour
				if (u<0.5) memcpy(dest,p0,sx1*sizeof(T));
                else memcpy(dest,p1,sx1*sizeof(T));
			}
		}
		slice->ydim = sx2;
		slice->data_ptr = (void *)y;
		free(ya);
	}
	if (sx1<size1) {	/* fill line with last value */
		for (i2=0; i2<sx2; i2++) {
			y1 = y[i2*size1+sx1-1];
			dest = y + i2*size1+sx1;
			for (i1=sx1; i1<size1; i1++) *dest++ = y1;
		}
		slice->xdim = size1;
	}
	slice->data_size = slice->xdim*slice->ydim*sizeof(T);
	return 1;
}

static MatrixData *rgb_split_data(MatrixData *src , int segment)
{
    int nvoxels = src->xdim*src->ydim*src->zdim;
    MatrixData *dest = (MatrixData*)calloc(1,sizeof(MatrixData));
    memcpy(dest,src,sizeof(MatrixData));
    dest->data_ptr = (void *)calloc(1, nvoxels);
	memcpy(dest->data_ptr, src->data_ptr + segment*nvoxels, nvoxels);
    dest->data_type = ByteData;
	return dest;
}

static int rgb_create_data(MatrixData *ret,
MatrixData *r, MatrixData *g , MatrixData *b)
{
    ret->data_type = Color_24;
    ret->xdim = r->xdim;
    ret->ydim = r->ydim;
    ret->zdim = r->zdim;
    ret->pixel_size = r->pixel_size;
    ret->y_size = r->y_size;
    ret->z_size = r->z_size;
    ret->scale_factor = 1;
    int nvoxels = r->xdim*r->ydim*r->zdim;
	free(ret->data_ptr);
	ret->data_ptr = (void *)calloc(3,nvoxels);
	memcpy(ret->data_ptr, r->data_ptr, nvoxels);
	memcpy(ret->data_ptr+nvoxels, g->data_ptr, nvoxels);
	memcpy(ret->data_ptr+2*nvoxels, b->data_ptr, nvoxels);
	return 1;
}

int  matrix_resize(MatrixData *mat, float pixel_size, int interp_flag,
					int align)
{
	int interpolate = interp_flag;
	if (mat->data_type == Color_8) interpolate = 0;
    switch(mat->data_type)
	{
	case ByteData:
	case Color_8:
		return matrix_resize_((unsigned char *)mat->data_ptr, mat, pixel_size,
			interpolate, align);
	case SunShort:
	case VAX_Ix2:
		return matrix_resize_((short*)mat->data_ptr, mat, pixel_size,
			interpolate, align);
	case Color_24:
		{
			MatrixData *red_slice = rgb_split_data(mat,0);
			MatrixData *green_slice = rgb_split_data(mat,1);
			MatrixData *blue_slice = rgb_split_data(mat,2);
			matrix_resize_((unsigned char *)red_slice->data_ptr,
				red_slice, pixel_size, interpolate, align);
			matrix_resize_((unsigned char *)green_slice->data_ptr,
				green_slice, pixel_size, interpolate, align);
			matrix_resize_((unsigned char *)blue_slice->data_ptr,
				blue_slice, pixel_size, interpolate, align);
			rgb_create_data(mat, red_slice,green_slice,blue_slice);
			free_matrix_data(red_slice);
			free_matrix_data(green_slice);
			free_matrix_data(blue_slice);
			return 1;
		}
	case IeeeFloat:
		return matrix_resize_((float*)mat->data_ptr, mat, pixel_size,
			interpolate, align);
	default:
		return 0;
	}
}
