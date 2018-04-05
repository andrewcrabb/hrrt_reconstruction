# pragma once

/*
 * static char sccsid[] = "%W% UCL-TOPO %E%";
 *
 *  Author : Merence Sibomana (Sibomana@topo.ucl.ac.be)
 *
 *	  Positron Emission Tomography Laboratory
 *	  Universite Catholique de Louvain
 *	  Ch. du Cyclotron, 2
 *	  B-1348 Louvain-la-Neuve
 *		  Belgium
 *
 *
 *  This program may be used free of charge by the members
 *  of all academic and/or scientific institutions.
 *	   ANY OTHER USE IS PROHIBITED.
 *  It may also be included in any package
 *	  -  provided that this text is not removed from
 *	  the source code and that reference to this original
 *	  work is done ;
 *	  - provided that this package is itself available
 *	  for use, free of charge, to the members of all
 *	  academic and/or scientific institutions.
 *  Nor the author, nor the Universite Catholique de Louvain, in any
 *  way guarantee that this program will fullfill any particular
 *  requirement, nor even that its use will always be harmless.
 *
 *  Creation date : 28-Apr-00
 *  Modification history:
 *  29-May-00 : Reslice in global area
 *  16-May-01 : Use mid pixel for interpolation
 *  14-Dec-06 : Bug fix : booundary check (Yan Shikui)
 */


#include "ecatx/matrix.h"
#include "matpkg.h"
#include <stdlib.h>
#include "matrix_resize.h"

#define V_ADD(x,y) for(j=0 ; j < 4 ; j++) (*(x+j)) += (*(y+j))
#define V_ADD2(x,y,m) for(j=0 ; j < 4 ; j++) (*(x+j)) += ((*(y+j)) * (m))
#define N_BLKS(n) (((n)+MatBLKSIZE-1)/MatBLKSIZE)

inline int round(float x) { return x > 0 ? int(x+0.5) : -int(-x+0.5); }
                                                         
template <class T> class IsotropicSlicer
{
  const MatrixData* volume;
  int interpolate;
  T *data;
  Matrix  transformer;
  int transf_x_slice(T *dest, float x_pos, int dest_yd, int dest_zd,
                     float pixel_size) const;
  int transf_y_slice(T *dest, float y_pos, int dest_xd, int dest_zd,
                     float pixel_size) const;
  int transf_z_slice(T *dest, float z_pos, int dest_xd, int dest_yd,
                     float pixel_size) const;
  MatrixData *create_slice(T *dptr, int sx, int sy, float dx, float dy,
                     float dz) const;
  MatrixData *create_comp_slice(float *dptr, int sx, int sy,
				float dx, float dy, float dz) const;
public :
  IsotropicSlicer<T>(const MatrixData* v, Matrix t=0, int interp=0);
 MatrixData *sagittal(float x_pos, float area_y, float area_z,
                      float pixel_size) const;
  MatrixData *coronal(float y_pos, float area_y, float area_z,
                      float pixel_size) const;
  MatrixData *transverse(float z_pos, float area_x, float area_y,
                      float pixel_size) const;
  MatrixData* x_projection(float x0, float x1, int mode) const;
  MatrixData* y_projection(float y0, float y1, int mode) const;
  MatrixData* slice(unsigned dim, float pos,
                    float area_x, float area_y, float area_z,
                    float pixel_size) const;

  MatrixData* projection(unsigned dim, float l, float r, float pixel_size,
		int mode=0) const;
  MatrixData* average(float z0, float thickness, float pixel_size) const;
};

template <class T>
IsotropicSlicer<T>::IsotropicSlicer(const MatrixData* v, Matrix t, int interp)
{
  volume = v;
  data = (T*)volume->data_ptr;
  if (t) transformer = t;
  //else transformer = mat_alloc(4,4); // identity
  interpolate = interp;
}

template <class T>
MatrixData* IsotropicSlicer<T>::create_slice(T *dptr, int sx, int sy, 
				       float dx, float dy, float dz) const
{
  MatrixData* slice = (MatrixData*)calloc(1,sizeof(MatrixData));
  slice->data_type = volume->data_type;
  slice->zdim = 1;
  if (dptr) slice->data_ptr = (void *)dptr;
  else
  {
    slice->data_ptr = (void *)calloc(sx*sy, sizeof(T));
    slice->data_type = ByteData;
  }
  slice->xdim = sx;
  slice->ydim = sy;
  slice->pixel_size = dx;
  slice->y_size = dy;
  slice->z_size = dz;
  slice->scale_factor = volume->scale_factor;
  slice->data_min =  volume->data_min;
  slice->data_max =  volume->data_max;
  slice->intercept = volume->intercept;
  slice->data_size = sx*sy*sizeof(T);
  return(slice);
}

template <class T>
MatrixData* IsotropicSlicer<T>::create_comp_slice(float *dptr, int sx, int sy, 
				       float dx, float dy, float dz) const
{
  MatrixData* slice = (MatrixData*)calloc(1,sizeof(MatrixData));
  slice->zdim = 1;
  slice->data_type = IeeeFloat;
  slice->data_ptr = (void *)dptr;
  slice->xdim = sx;
  slice->ydim = sy;
  slice->pixel_size = dx;
  slice->y_size = dy;
  slice->z_size = dz; 
  slice->scale_factor = volume->scale_factor;
  slice->data_min =  volume->data_min;
  slice->data_max =  volume->data_max;
  slice->data_size = sx*sy*sizeof(T);
  return(slice);
}

template <class T>
int IsotropicSlicer<T>::transf_x_slice(T *dest, float x_pos,
int dest_yd, int dest_zd, float pixel_size) const
{
  float   dx[4], dy[4], dz[4],pt[4], rsdy[4], qx, qy, qz, w ;
  int     i, j, dest_n, page, ind, xctr=0, yctr=0, zctr=0, xind, yind, zind ;
  int xind1,yind1,zind1;  // [x|y|z]ind + 1 if [x|y|z]ind>=0 otherwise 0
  int xd,yd,zd;
  int is_float=0;
  float *M=transformer->data, zoom = transformer->data[0];
  T  *src=data;

  if ((T)(0.5) == 0.5) is_float=1;
  xd = volume->xdim; yd = volume->ydim; zd = volume->zdim;
  dest_n = dest_yd*dest_zd;
  page = xd*yd;
  
  for(i=0 ; i < 4 ; i++)
    {
      *(dx+i) = pixel_size/volume->pixel_size*(*(M+i*4)) ;
      *(dy+i) = pixel_size/volume->y_size * (*(M+i*4+1)) ;
      *(dz+i) = pixel_size/volume->z_size * (*(M+i*4+2)) ;
      *(pt+i) = (i == 3) ? 1.0F : 0.0F ;
    }
  
  pt[0] = *(M+3);
  pt[1] = *(M+7);
  pt[2] = *(M+11);
  
  V_ADD2(pt,dx,x_pos/pixel_size); /* move to x_pos */
  memcpy(rsdy,pt,sizeof(pt));  /* save y==0 position */
  for(i=0 ; i < dest_n ; i++) {
      if((0 <= *pt) && (*pt < xd) && (0 <= *(pt+1)) &&
		  (*(pt+1) < yd) && (0 <= *(pt+2)) && (*(pt+2) < zd)) {
		  xind = (int)pt[0];
		  yind = (int)pt[1];
		  zind = (int)pt[2];
		  xind1 = xind+1;
		  yind1 = yind+1;
		  zind1 = zind+1;
		  qx       = pt[0]-xind;
		  qy       = pt[1]-yind;
		  qz       = pt[2]-zind;
		  if (interpolate==0) {
			  if (qx>=0.5 && xind1<xd) xind = xind1;
			  if (qy>=0.5 && yind1<yd) yind = yind1;
			  if (qz>=0.5 && zind1<zd) zind = zind1;
			  ind     = zind*page+yind*xd+xind;
			  *(dest+i) = *(src+ind);
		  } else {
			  ind     = zind*page+yind*xd+xind;
			  w = (1.0F-qx)*(1.0F-qy)*(1.0F-qz)*(*(src+ind));
			  if(xind1 < xd)
				  w += qx*(1.0F-qy)*(1.0F-qz)*(*(src+ind+1));
			  if((xind1 < xd) && (zind1 < zd))
				  w += qx*(1.0F-qy)*qz*(*(src+ind+1+page));
			  if(zind1 < zd)
				  w += (1.0F-qx)*(1.0F-qy)*qz*(*(src+ind+page));
			  if(yind1 < yd)
				  w += (1.0F-qx)*qy*(1.0F-qz)*(*(src+ind+xd));
			  if((xind1 < xd) && (yind1 < yd))
				  w += qx*qy*(1.0F-qz)*(*(src+ind+1+xd));
			  if((xind1 < xd) && (yind1 < yd) && (zind1 < zd))
				  w += qx*qy*qz*(*(src+ind+1+xd+page));
			  if((yind1 < yd) && (zind1 < zd))
				  w += (1.0F-qx)*qy*qz*(*(src+ind+xd+page));
			  if (is_float) *(dest+i) = (T)w;
			  else *(dest+i) = (T)(w>0 ? 0.5+w : -0.5+w);
		  }
	  } else {
		  *(dest+i)  = 0;
	  }
      /* next  line */
	  yctr++ ;
	  if(yctr == dest_yd) {
		  yctr = 0 ;
		  memcpy(pt,rsdy,sizeof(pt)); /* goto y==0 position */
		  V_ADD(pt,dz) ;			 /* goto next z */
		  memcpy(rsdy,pt,sizeof(pt)); /* save y==0 position */
		  zctr++ ;
	  } else {
		  V_ADD(pt,dy);
	  }
  }
  return 1;       
}

template <class T>
int IsotropicSlicer<T>::transf_y_slice(T *dest, float y_pos, int dest_xd,
                                    int dest_zd, float pixel_size) const
{
  float  dx[4], dy[4], dz[4],pt[4], rsdx[4], qx, qy, qz, w ;
  int     i, j, dest_n, page, ind, xctr=0, yctr=0, zctr=0, xind, yind, zind ;
  int xind1,yind1,zind1;  // [x|y|z]ind + 1 if [x|y|z]ind>=0 otherwise 0
  int xd,yd,zd;
  float *M=transformer->data;
  T  *src=data;
  
  int is_float=0;
  if ((T)(0.5) == 0.5) is_float=1;
  xd = volume->xdim; yd = volume->ydim; zd = volume->zdim;
  dest_n = dest_xd*dest_zd;
  page = xd*yd;
  
  for(i=0 ; i < 4 ; i++) {
      *(dx+i) = pixel_size/volume->pixel_size*(*(M+i*4)) ;
      *(dy+i) = pixel_size/volume->y_size * (*(M+i*4+1)) ;
      *(dz+i) = pixel_size/volume->z_size * (*(M+i*4+2)) ;
      *(pt+i) = (i == 3) ? 1.0F : 0.0F ;
  }

  pt[0] = *(M+3);
  pt[1] = *(M+7);
  pt[2] = *(M+11);
  
  V_ADD2(pt,dy,y_pos/pixel_size); /* move to y_pos */
  memcpy(rsdx,pt,sizeof(pt));     /* save x==0 position */
  
  for(i=0 ; i < dest_n ; i++) {
      if((0 <= *pt) && (*pt < xd) && (0 <= *(pt+1)) &&
		  (*(pt+1) < yd) && (0 <= *(pt+2)) && (*(pt+2) < zd)) {
		  xind = (int)pt[0];
		  yind = (int)pt[1];
		  zind = (int)pt[2];
		  xind1 = xind+1;
		  yind1 = yind+1;
		  zind1 = zind+1;
		  qx       = pt[0]-xind;
		  qy       = pt[1]-yind;
		  qz       = pt[2]-zind;
		  if (interpolate==0) {
			  if (qx>=0.5 && xind1<xd) xind = xind1;
			  if (qy>=0.5 && yind1<yd) yind = yind1;
			  if (qz>=0.5 && zind1<zd) zind = zind1;
			  ind     = zind*page+yind*xd+xind;
			  *(dest+i) = *(src+ind);
		  } else {
			  ind     = zind*page+yind*xd+xind;
			  w = (1.0F-qx)*(1.0F-qy)*(1.0F-qz)*(*(src+ind));
			  if(xind1 < xd)
				  w += qx*(1.0F-qy)*(1.0F-qz)*(*(src+ind+1));
			  if((xind1 < xd) && (zind1 < zd))
				  w += qx*(1.0F-qy)*qz*(*(src+ind+1+page));
			  if(zind1 < zd)
				  w += (1.0F-qx)*(1.0F-qy)*qz*(*(src+ind+page));
			  if(yind1 < yd)
				  w += (1.0F-qx)*qy*(1.0F-qz)*(*(src+ind+xd));
			  if((xind1 < xd) && (yind1 < yd))
				  w += qx*qy*(1.0F-qz)*(*(src+ind+1+xd));
			  if((xind1 < xd) && (yind1 < yd) && (zind1 < zd))
				  w += qx*qy*qz*(*(src+ind+1+xd+page));
			  if((yind1 < yd) && (zind1 < zd))
				  w += (1.0F-qx)*qy*qz*(*(src+ind+xd+page));
			  if (is_float) *(dest+i) = (T)w;
			  else *(dest+i) = (T)(w>0 ? 0.5+w : -0.5+w);
		  }
	  } else *(dest+i) = 0 ;
      xctr++ ;
      if(xctr == dest_xd) {
		  xctr =0 ;
		  memcpy(pt,rsdx,sizeof(pt));     /* move to x==0 position */
		  V_ADD(pt,dz) ;                  /* goto next z */
		  memcpy(rsdx,pt,sizeof(pt));     /* save x==0 position */
	  } else {
		  V_ADD(pt,dx);
	  }
  }
  return 1;          
}

template <class T>
int IsotropicSlicer<T>::transf_z_slice(T *dest, float z_pos,
int dest_xd, int dest_yd, float pixel_size) const
{
//extern int debug_level;
  float   dx[4], dy[4], dz[4],pt[4], rsdx[4], qx, qy, qz, w ;
  int     i, j, dest_n, page, ind, xctr=0, yctr=0, zctr=0, xind, yind, zind ;
  int xind1,yind1,zind1;  // [x|y|z]ind + 1 if [x|y|z]ind>=0 otherwise 0
  int xd,yd,zd;
  float *M, zoom;
  const T   *src;
  
  int is_float=0;
  if ((T)(0.5) == 0.5) is_float=1;
  M = transformer->data; zoom = transformer->data[0];
  xd = volume->xdim; yd = volume->ydim; zd = volume->zdim;
  src = data;
  dest_n = dest_xd*dest_yd;
  page = xd*yd;
  for(i=0 ; i < 4 ; i++)
    {
      *(dx+i) = pixel_size/volume->pixel_size*(*(M+i*4)) ;
      *(dy+i) = pixel_size/volume->y_size * (*(M+i*4+1)) ;
      *(dz+i) = pixel_size/volume->z_size * (*(M+i*4+2)) ;
      *(pt+i) = (i == 3) ? 1.0F : 0.0F ;
    }

  pt[0] = *(M+3);
  pt[1] = *(M+7);
  pt[2] = *(M+11);
  
  V_ADD2(pt,dz,z_pos/pixel_size); /* move to z_pos */
  //if (debug_level>9)
  //  fprintf(stderr,"move to (0,0,%g) = (%g,%g,%g)\n",
//            z_pos, pt[0], pt[1], pt[2]);
  memcpy(rsdx,pt,sizeof(pt)); /* save x==0 position */
  for(i=0 ; i < dest_n ; i++) {
	  if((0 <= *pt) && (*pt < xd) && (0 <= *(pt+1)) &&
		  (*(pt+1) < yd) && (0 <= *(pt+2)) && (*(pt+2) < zd)) {
		  xind = (int)pt[0];
		  yind = (int)pt[1];
		  zind = (int)pt[2];
		  xind1 = xind+1;
		  yind1 = yind+1;
		  zind1 = zind+1;
		  qx       = pt[0]-xind;
		  qy       = pt[1]-yind;
		  qz       = pt[2]-zind;
		  if (interpolate==0) {
			  if (qx>=0.5 && xind1<xd) xind = xind1;
			  if (qy>=0.5 && yind1<yd) yind = yind1;
			  if (qz>=0.5 && zind1<zd) zind = zind1;
			  ind     = zind*page+yind*xd+xind;
			  *(dest+i) = *(src+ind);
		  } else { 
			  ind     = zind*page+yind*xd+xind;
			  w = (1.0F-qx)*(1.0F-qy)*(1.0F-qz)*(*(src+ind));
			  if(xind1 < xd)
				  w += qx*(1.0F-qy)*(1.0F-qz)*(*(src+ind+1));
			  if((xind1 < xd) && (zind1 < zd))
				  w += qx*(1.0F-qy)*qz*(*(src+ind+1+page));
			  if(zind1 < zd)
				  w += (1.0F-qx)*(1.0F-qy)*qz*(*(src+ind+page));
			  if(yind1 < yd)
				  w += (1.0F-qx)*qy*(1.0F-qz)*(*(src+ind+xd));
			  if((xind1 < xd) && (yind1 < yd))
				  w += qx*qy*(1.0F-qz)*(*(src+ind+1+xd));
			  if((xind1 < xd) && (yind1 < yd) && (zind1 < zd))
				  w += qx*qy*qz*(*(src+ind+1+xd+page)) ;
			  if((yind1 < yd) && (zind1 < zd))
				  w += (1.0F-qx)*qy*qz*(*(src+ind+xd+page));
			  if (is_float) *(dest+i) = (T)w;
			  else *(dest+i) = (T)(w>0 ? 0.5+w : -0.5+w);
		  }
    } else {
      *(dest+i)  = -20000;
    }
    T v = *(dest+i);
	  xctr++ ; 
	  if(xctr == dest_xd) {
		  xctr = 0;
		  memcpy(pt,rsdx,sizeof(pt));
		  V_ADD(pt,dy) ;
		  memcpy(rsdx,pt,sizeof(pt)); /* save x==0 position */
	  } else {
		  V_ADD(pt,dx) ;
	  }
  }
  return 1;          
}


template <class T>
MatrixData *IsotropicSlicer<T>::transverse(float z, float area_x,
                                          float area_y, float pixel_size) const
{
//extern int debug_level;
  T *dest = 0;
  float dz = volume->z_size;
  int sx = (int)(ceil(volume->xdim*volume->pixel_size/pixel_size));
  int sy = (int)(ceil(volume->ydim*volume->y_size/pixel_size));
  if (area_x>0) sx = (int)(ceil(area_x/pixel_size));
  if (area_y>0) sy = (int)(ceil(area_y/pixel_size));

  //////////////////////////////////////////////////
  //12-14-2006: Boundary check on image size.
  if (sx>volume->xdim)
	  sx = volume->xdim;
  if (sy>volume->ydim)
	  sy = volume->ydim;
  //////////////////////////////////////////////////

  if (!data) return create_slice(dest,sx,sy,pixel_size,pixel_size,dz);
  dest =  (T*)calloc(sx*sy, sizeof(T));
  for (int i=0; i<sx*sy; i++) dest[i] = (T)(-1);
  transf_z_slice(dest, z+0.5F*dz, sx, sy, pixel_size); // use mid slice
  return create_slice(dest,sx,sy,pixel_size,pixel_size,dz);
}

template <class T>
MatrixData* IsotropicSlicer<T>::sagittal(float x, float area_y,
                                         float area_z, float pixel_size) const
{
  T *dest=0;
  float dx = volume->pixel_size;
  int sy = (int)(ceil(volume->ydim*volume->y_size/pixel_size));
  int sz = (int)(ceil(volume->zdim*volume->z_size/pixel_size));
  if (area_y>0) sy = (int)(ceil(area_y/pixel_size));
  if (area_z>0) sz = (int)(ceil(area_z/pixel_size));

  //////////////////////////////////////////////////
  //12-14-2006: Boundary check on image size.
  if (sy>volume->ydim)
	  sy = volume->ydim;
  if (sz>volume->zdim)
	  sz = volume->zdim;
  //////////////////////////////////////////////////

  if (!data)
     return create_slice(dest,sy,sz,pixel_size,pixel_size,dx);
  dest = (T*)calloc(sy*sz, sizeof(T));
  transf_x_slice(dest,x+0.5F*dx,sy,sz, pixel_size); // use mid slice
  return create_slice(dest,sy,sz,pixel_size,pixel_size,dx);
}

template <class T>
MatrixData* IsotropicSlicer<T>::x_projection(float x0 , float x1, int mode ) const
{
  float *dest=0, *dest_line=0, *curp=0;
  int  yi, zi, xi;
  int xi0 = round(x0), xi1 = round(x1);
  int sx = volume->xdim, sy = volume->ydim, sz = volume->zdim;
  float dy = volume->y_size, dz = volume->z_size;
  
  if (!data || !interpolate) return create_comp_slice(dest,sy,sz,dy,dz,x1-x0);
  int  npixels = sy*sz;
  dest = (float*)calloc(npixels,sizeof(float));
  float fact = 1.0F;
  if (mode == MEAN_PROJ && xi1>xi0) fact = 1.0F/(xi1-xi0);
  for (zi=0; zi<sz; zi++)
  {
     dest_line = dest+zi*sy;
     T *plane = data + sx*sy*zi;
     for (yi=0; yi<sy; yi++)
     {
        T *line = plane+sx*yi;
        curp = &dest_line[yi];
        if (mode == MEAN_PROJ) {
	  *curp = 0;
	  for (xi=xi0; xi<xi1; xi++) *curp += line[xi];
	  *curp *= fact;
	}
        else
        {
           *curp = line[xi0];
           for (xi=xi0+1; xi<xi1; xi++) 
             if (*curp < line[xi]) *curp = line[xi];
         }
     }
  }
  return create_comp_slice(dest,sy,sz,dy,dz,x1-x0);
}

template <class T>
MatrixData* IsotropicSlicer<T>::coronal(float y, float area_x, float area_z,
                                        float pixel_size) const
{
  T *dest=0;
  int sx = (int)(ceil(volume->xdim*volume->pixel_size/pixel_size));
  int sz = (int)(ceil(volume->zdim*volume->z_size/pixel_size));
  float dy = volume->y_size;
  if (area_x>0) sx = (int)(ceil(area_x/pixel_size));
  if (area_z>0) sz = (int)(ceil(area_z/pixel_size));

  //////////////////////////////////////////////////
  //12-14-2006: Boundary check on image size.
  if (sx>volume->xdim)
	  sx = volume->xdim;
  if (sz>volume->zdim)
	  sz = volume->zdim;
  //////////////////////////////////////////////////

  if (!data) 
    return create_slice(dest,sx,sz,pixel_size, pixel_size,dy);
  dest = (T*)calloc(sx*sz, sizeof(T));
  transf_y_slice(dest,y+0.5F*dy,sx,sz, pixel_size);  // use mid slice
  return create_slice(dest,sx,sz,pixel_size,pixel_size,dy);
}

template <class T>
MatrixData* IsotropicSlicer<T>::y_projection(float y0, float y1, int mode) const
{
  float *dest=0, *dest_line=0;
  int xi, yi, zi;
  int yi0=round(y0), yi1=round(y1);
  int sx = volume->xdim, sy = volume->ydim, sz = volume->zdim;
  float dx = volume->pixel_size, dz = volume->z_size;
  
  if (!data || volume->data_type == Color_8 || volume->data_type==Color_24)
    return create_comp_slice(dest,sx,sz,dx,dz,y1-y0);
  int npixels = sx*sz;
  dest = (float*)calloc(npixels,sizeof(float));
  float fact = 1.0F;
  if (mode == MEAN_PROJ && yi1>yi0) fact = 1.0F/(yi1-yi0);
  for ( zi=0; zi<sz; zi++)
    {
      dest_line = dest+sx*zi;
      T *plane = data + sx*sy*zi;
      T* line = plane+sx*yi0;
      if (mode == MAX_PROJ) {
	for (xi=0; xi<sx; xi++) dest_line[xi] = line[xi];
	for (yi=yi0+1; yi<yi1; yi++) {
	  line = plane+sx*yi;
	  for (xi=0; xi<sx; xi++) 
	    if (dest_line[xi] < line[xi]) dest_line[xi] = line[xi];
	}
      } else {
	for (yi=yi0; yi<yi1; yi++) {
	  line = plane+sx*yi;
	  for (xi=0; xi<sx; xi++) dest_line[xi] += line[xi];
	}
	for (xi=0; xi<sx; xi++) dest_line[xi] *= fact;
      }
    }
  return create_comp_slice(dest,sx,sz,dx,dz,y1-y0);
}

template <class T>
MatrixData* IsotropicSlicer<T>::projection(unsigned dimension,
					float l, float h, float pixel_size,
					int mode) const {
  T *dest=0;
  MatrixData *ret=NULL;
  float dx = volume->pixel_size, dy = volume->y_size;
  switch(dimension) {
  case Dimension_X :
    ret =  x_projection(l, h, mode);
	break;
  case Dimension_Y :
    ret =  y_projection(l, h, mode);
	break;
  case Dimension_Z :
    ret =  create_slice(dest,volume->xdim,volume->ydim,dx,dy,l-h);
  default :
	return NULL;
  }
  matrix_resize(ret,pixel_size,interpolate);
  return ret;
}

template <class T>
MatrixData* IsotropicSlicer<T>::slice(unsigned dimension, float pos, float area_x,
                                   float area_y, float area_z, float pixel_size) const
{
  T *dest=0;
  switch(dimension) {
  case Dimension_X :
    return sagittal(pos, area_y, area_z, pixel_size);
  case Dimension_Y :
    return coronal(pos, area_x, area_z, pixel_size);
  case Dimension_Z :
    return  transverse(pos, area_x, area_y, pixel_size);
  }
  return NULL;
}
template <class T>
MatrixData* IsotropicSlicer<T>::average(float z0, float thickness,
                                        float pixel_size) const
{
  int i, z;
  float *dest=0;
  T *tmp = NULL, *src=NULL;
  float dx = volume->pixel_size, dy = volume->y_size;
  float half_width = 0.5*thickness;

  int xdim = (int)(0.5+volume->xdim*dx/pixel_size);
  int ydim = (int)(0.5+volume->ydim*dy/pixel_size);
  if (!data || volume->data_type == Color_8 || volume->data_type==Color_24)
    return create_comp_slice(dest,xdim,ydim,pixel_size,pixel_size,thickness);
  int b = round(max((float)0,z0-half_width));
  int t = round(min((float)volume->zdim-1,z0+half_width));
  int npixels = xdim*ydim;
  src = tmp = (T*)calloc(npixels,sizeof(T));
  dest = (float*)calloc(npixels,sizeof(float));
  for (z=b; z<=t; z++)
  {
     transf_x_slice(src,z,xdim,ydim,pixel_size);
     for (i=0; i<npixels; i++) dest[i] += src[i];
  }
  free(tmp);
  return create_comp_slice(dest,xdim,ydim,pixel_size,pixel_size,thickness);
}
