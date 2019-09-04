/*
Copyright (C) 2006, Z.H. Cho, Neuroscience Research Institute, Korea, zcho@gachon.ac.kr
Inki hong, Kore Polytechnic University, Korea, isslhong@kpu.ac.kr

This code is not free software; you cannot redistribute it without permission from Z.H. Cho and Inki Hong.
This code is only for research/educational purpose.
this code is patented and is not supposed to be used in other commercial scanner.
For commercial use, please contact zcho@gachon.ac.kr or isslhong@kpu.ac.kr
*/
/*	
 Modification history:
	08-May-08: Added default value for scatter or atten when not available (MS)
  05-Jun-08: Write image plane per plane because
             writing volume for cluster nodes doesn't work (MS)
  12-May-09: Use same code for linux and WIN32
  09-DEC-2009: Restore -X 128 option (MS)
*/

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "scanner_model.h"
#include "hrrt_osem3d.h"
#include "nr_utils.h"
/* Rebin original sinogram to reconstruction size */
template <typename T> 
void sino_rebin(T *tmp_buf, T *out_buf, int planes, int norm_flag)
{
  T *src=NULL, *dst=NULL;
  int i,v,xr;
  int r_size=xr_pixels*x_rebin, v_size=views*v_rebin;
  for (int plane=0; plane<planes;plane++) {
    // rebin plane
    T *pdata1 = tmp_buf + plane*v_size*r_size;
    // rebin views
    for (v=0; v<views; v++) {
      dst = pdata1 + v*v_rebin*r_size;
      for (int vr=0; vr<v_rebin; vr++) {
        if (!norm_flag) {
          if (vr==0) continue;
          src = dst + vr*r_size;
          for (i=0; i<r_size; i++) dst[i] += src[i];
        } else {
          src = dst + vr*r_size;
          if (vr == 0) {
            for (i=0; i<r_size; i++) 
              if (src[i]>(T)0) dst[i] = (T)1/src[i];
          } else {
            for (i=0; i<r_size; i++)
              if (src[i]>(T)0) dst[i] += (T)1/src[i];
          }
        }
      }
    }
    // rebin projections
    T *pdata2 = out_buf + plane*xr_pixels*views;
    pdata1 = tmp_buf + plane*v_size*r_size;
    for (v=0; v<views; v++) {
      dst = pdata2 + v*xr_pixels;
      src = pdata1 + v*v_rebin*r_size;
      for (i=0; i<xr_pixels; i++, dst++) {
        *dst = *src++;
        for (xr=1; xr<x_rebin; xr++) *dst += *src++;
      }
    }
  }
  if (norm_flag) {
    int count = planes*xr_pixels*views;
    for (i=0; i<count; i++)
      if (out_buf[i]>(T)0) out_buf[i] = (T)1/out_buf[i];
  }
}

template <typename T> 
void sino_rebin_float(T *tmp_buf, float *out_buf, int planes)
{
  T *src=NULL;
  int i,v;
  int r_size=xr_pixels*x_rebin, v_size=views*v_rebin;
  for (int plane=0; plane<planes;plane++) {
    // rebin plane
    T *pdata1 = tmp_buf + plane*v_size*r_size;
    // rebin views
    for (v=0; v<views; v++) {
      T *vdst = pdata1 + v*v_rebin*r_size;;
      for (int vr=1; vr<v_rebin; vr++) {
        T *vsrc = vdst + vr*r_size;
        for (i=0; i<r_size; i++) vdst[i] += vsrc[i];
      }
    }
    // rebin projections
    float *pdata2 = out_buf + plane*xr_pixels*views;
    pdata1 = tmp_buf + plane*v_size*r_size;
    for (v=0; v<views; v++) {
      float *dst = pdata2 + v*xr_pixels;
      T *src = pdata1 + v*v_rebin*r_size;
      for (i=0; i<xr_pixels; i++, dst++) {
        *dst = 0.0f;
        for (int xr=0; xr<x_rebin; xr++, src++) *dst += (float)(*src);
      }
    }
  }
}

/****************************************************************************/
/* Reading routine for all sinogram file except for true & prompt when short*/

int read_flat_float_proj_ori_theta(FILE *fp, float *prj,int theta, float def_val, int norm_flag) {        

	long offset;
	int i, rebin=x_rebin*v_rebin;
	int plane, nplanes, file_plane_size;

	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;
	if (fp == NULL){
		int ds_size = xr_pixels*views*(yr_top[theta]-yr_bottom[theta]+1);
		for (i=0; i<ds_size; i++) prj[i] = def_val;
		return 1;
	}
	offset = (seg_offset[theta])*sizeof(float)*rebin;
  file_plane_size = views*xr_pixels*rebin;
  nplanes = yr_top[theta]-yr_bottom[theta]+1;

	//    if (verbose & 0x0010) LOG_INFO(" Read flat float projection at theta = %d\t offset = %d\n",theta,offset);
	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		LOG_ERROR("  Read flat float projection: fseek problem \n");
		return 0;
	}
  if (rebin == 1) { // no rebinning
    for(plane=0;plane<nplanes;plane++) {
      fread(prj+plane*file_plane_size,sizeof(float),file_plane_size,fp);
      if (norm_flag && (plane<npskip || plane+npskip>=nplanes)) { // kill end segments
        memset(prj+plane*file_plane_size, 0, sizeof(float)*file_plane_size);
      }
    }
  } else {
    float *buf=NULL; 
    unsigned char *mask=NULL, *mask_r=NULL;
    if (norm_flag) 
      if ((mask = (unsigned char*)calloc(file_plane_size, sizeof(unsigned char))) == NULL) {
        LOG_ERROR("  Read flat float projection: memory allocation failure \n");
        return 0;
      }
    if ((buf=(float*)calloc(file_plane_size, sizeof(float))) == NULL) {
      LOG_ERROR("  Read flat float projection: memory allocation failure \n");
      return 0;
    }
    for (plane=0;plane<nplanes;plane++) {
      fread(buf,sizeof(float),file_plane_size,fp);
      if (norm_flag && (plane<npskip || plane+npskip>=nplanes)) { // kill end segments
        memset(prj+plane*views*xr_pixels, 0, sizeof(float)*views*xr_pixels);
        continue;
      }
      if (norm_flag) 
        for (i=0;i<file_plane_size;i++)
          if (buf[i] > 0) mask[i] = 1;
      sino_rebin(buf, prj+plane*views*xr_pixels, 1, norm_flag);
    }
    free(buf);
    if (norm_flag) {
      if ((mask_r = (unsigned char*)calloc(views*xr_pixels, sizeof(unsigned char))) == NULL) {
        LOG_ERROR("  Read flat float projection: memory allocation failure \n");
        return 0;
      }
      sino_rebin(mask, mask_r, 1, 0);
      for (plane=0;plane<nplanes;plane++) {
        float *pdata = prj+plane*views*xr_pixels;
        for (i=0;i<views*xr_pixels;i++)
          if (mask_r[i] < rebin) pdata[i]=0.0f; // keep gaps out
      }
      free(mask);
      free(mask_r);
    }
  }
	return 1;
}

/* 
* Read routine for true and prompt sinogram 
*/
int read_flat_short_proj_ori_to_float_array_theta(FILE *fp, float *prj,int theta)
{
	short *ds_tmp;
	int   ds_size;
	long offset;
	int i;
	long curptr;

	//ds_size = views*xr_pixels*sizeof(short);
	ds_size=views*v_rebin*xr_pixels*x_rebin*(yr_top[theta]-yr_bottom[theta]+1);
  if ((ds_tmp = (short*)malloc(ds_size*sizeof(short)))==NULL) {
    LOG_ERROR("  Read flat short projection: memory allocation failure \n");
    return 0;
  }
	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;
	offset = (seg_offset[theta])*sizeof(short)*x_rebin*v_rebin;

	//    if (verbose & 0x0010) LOG_INFO(" Read short projection 2 float at theta = %d\t offset =%d\n",theta,offset);

	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		LOG_ERROR("  Read flat short projection 2 float: fseek problem \n");
		return 0;
	}
	curptr=0;
	fread(ds_tmp,sizeof(short),ds_size,fp);
	if (x_rebin*v_rebin==1) for(i=0;i<ds_size;i++) prj[i]=ds_tmp[i];
  else sino_rebin_float(ds_tmp, prj, yr_top[theta]-yr_bottom[theta]+1);
	//for(i=0;i<(yr_top[theta]-yr_bottom[theta]+1);i++, curptr+=views*xr_pixels){
	//	fread(ds_tmp, sizeof(short)*xr_pixels,views,fp);
	//	ptr=&prj[curptr];
	//	for(j=0;j<views*xr_pixels;j++) ptr[j]=ds_tmp[j];
	//}
	free(ds_tmp);
	return 1;
}

/**
* for prompt
*/
int read_flat_short_proj_ori_to_short_array_theta(FILE *fp, short *prj,int theta)
{        
	long offset;
	int i=0;
	int curptr;

	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;
	offset = (seg_offset[theta])*sizeof(short)*x_rebin*v_rebin;
	//    if (verbose & 0x0010) LOG_INFO(" Read short projection 2 short at theta = %d\t offset = %d\n",theta,offset);
	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		LOG_EXIT("  Read flat short projection 2 short: fseek problem \n");
	}
	curptr=0;
  if (x_rebin*v_rebin == 1) {
    for(i=0;i<yr_top[theta]-yr_bottom[theta]+1;i++,curptr+=views *xr_pixels)
      fread(&prj[curptr], sizeof(short) *xr_pixels,views,fp);
  } else {
    int count=xr_pixels*x_rebin*views*v_rebin;
    short *buf = (short*)calloc(count, sizeof(short));
    if (buf==NULL) {
      LOG_ERROR("  Read flat short projection: memory allocation failure \n");
      return 0;
    }
    for(i=0;i<yr_top[theta]-yr_bottom[theta]+1;i++) {
      fread(buf, sizeof(short), count,fp);
      sino_rebin(buf, &prj[curptr], 1, 0);
      curptr += views *xr_pixels;
    }
    free(buf);
  }

	return 1;
}
int read_flat_int_proj_ori_to_float_array_theta(FILE *fp, float *prj,int theta)
{
	int *ds_tmp;
	int   ds_size;
	long offset;
	int i,j;
	long curptr;
	float *ptr;

	ds_size = views*v_rebin*xr_pixels*x_rebin;
	ds_tmp = (int*)calloc(ds_size, sizeof(int));

	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;


	offset = (seg_offset[theta])*sizeof(int)*x_rebin*v_rebin;
	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		LOG_INFO("  Read Flat float Projection: fseek problem \n");
		return 0;
	}
	printf("short %d\t%d\n",theta,offset);
	curptr=0;
	for(i=0;i<(yr_top[theta]-yr_bottom[theta]+1);i++, curptr+=views*xr_pixels){
		fread(ds_tmp, sizeof(int)*xr_pixels,views,fp);
		ptr=&prj[curptr];
    if (x_rebin*v_rebin == 1)
      for(j=0;j<views*xr_pixels;j++) ptr[j]= (float)ds_tmp[j];///100.0;
    else sino_rebin_float(ds_tmp, ptr, 1);
	}

	free(ds_tmp);
	return 1;
}


/***************************************************************************/
/*	write for flat image file. 					   */
int write_flat_image(float ***image,char * filename,int index,int verbose ){

	FILE *f;
	int  offset, count, z;
  float *plane;

	offset = index * x_pixels*y_pixels*(z_pixels);

	//if(verbose & 0x0010) 
		LOG_INFO("  ... writing flat image at index %d (offset %d)\n", index, offset );

	if( index==0 ) 
		f = fopen(filename,"wb"); 
	else 
		f = fopen(filename,"ab+");

	if( f==NULL ) { 
		LOG_ERROR("  write_flat_image(): can't open file %s \n", filename);  
		return 0; 
	}

  // write volume plane by plane, whole volume
  // image may be too large for writing on a network file
  count = x_pixels*y_pixels;
  plane = &image[0][0][0];
  for (z=0, plane = &image[0][0][0] ; z<z_pixels; z++, plane += count)
    if( fwrite(plane, sizeof(float), count, f) != count ) {
      LOG_ERROR("  write_flat_image(): write error %d bytes\n", count*sizeof(float));
      fclose(f);
      return  0;  
    }
	fclose(f);
	return 1;
}

int    read_flat_image(float ***image,char * filename,int index,int verbose)
{
	FILE    *f;
	int    offset;
	offset = index * x_pixels*y_pixels*(z_pixels);
	if(verbose & 0x0010) LOG_INFO("  ... reading flat image at index %d (offset %d) \n", index, offset );

	f = fopen(filename,"rb");
	if(f == NULL) { LOG_INFO("  read_flat_image(): can't open file %s !\n",filename ); return 0; }

	if( fseek( f,  offset*sizeof(float),  SEEK_SET ) !=0 ) {    
		LOG_ERROR("  read_flat_image(): fseek error at index=%d \n", index)  ;
		fclose(f);
		return 0;
	}
	if( fread( &image[0][0][0],sizeof(float), x_pixels*y_pixels*(z_pixels), f) != 1 ){
		LOG_ERROR("  read_flat_image(): write error at  index=%d \n",  index );
		fclose(f);
		return  0;    
	}
	fclose(f);
	return 1;
}

/**************************************************************************************************************/
int    write_norm(float ***norm,char *filename,int isubset,int verbose ) {
	FILE    *f;
	int    offset;
	int i;
//	int x,y;
//	float a[256][256];

	//	data are now aligned with multiple 4 pixels in z (e.g. 207-> 208 ; 153->156)
	offset =  isubset * x_pixels*y_pixels*(z_pixels_simd*4);
	if (verbose & 0x0010) LOG_INFO("  ... writing norm subset %d at offset %d \n", isubset, offset*4 );
	if( isubset==0 ) 
		f = fopen(filename,"wb"); 
	else 
		f = fopen(filename,"ab+");
	if( f==NULL ) { 
		LOG_ERROR("  write_norm(): can't open file %s \n", filename);  return 0; 
	}
	for(i=0;i<x_pixels;i++){
		if( fwrite( &norm[i][0][0],sizeof(float)*(z_pixels_simd)*4,y_pixels, f) != y_pixels){
			LOG_ERROR("  write_norm(): write error at  isubset=%d  \n",  isubset );
			fclose(f);
			return  0;    
		}
	}
	fclose(f);
/*	f=fopen("normflip.i","ab");
	for(i=0;i<z_pixels;i++){
		for(y=0;y<x_pixels;y++){
			for(x=0;x<x_pixels;x++){
				a[y][x]=norm[y][x][i];
			}
		}
		fwrite(a,4,256*256,f);
	}
	fclose(f);
*/	return 1;
}

/**************************************************************************************************************/
int    read_norm(float ***norm,char * filename,int isubset,int verbose) {
	FILE    *f;
	int    offset;
	int i;
	int curptr;

	offset = isubset * x_pixels*y_pixels*(z_pixels_simd*4);
	if(verbose & 0x0010) LOG_INFO("  ... reading norm for subset %d at offset %d \n", isubset, offset );
	f = fopen(filename,"rb");
	if( f == NULL ) { 
		LOG_INFO("  read_norm(): can't open file %s !\n",filename ); 
		return 0; 
	}
	if( fseek( f,  offset*sizeof(float),  SEEK_SET ) !=0 ) {    
		LOG_INFO("  read_norm(): fseek error at isubset=%d \n", isubset)  ;
		fclose(f);
		return 0;
	}
	curptr=0;
	for(i=0;i<x_pixels;i++){
		if( fread( &norm[i][0][0],sizeof(float)*(z_pixels_simd)*4,y_pixels, f) != y_pixels ){    
			LOG_INFO("  read_norm(): read error at  isubset=%d \n",  isubset );
			fclose(f);
			return  0;    
		}
	}
	fclose(f);
	return 1;
}

int write_flat_3dscan(float ***scan,char *filename,int theta,int verbose) {

	FILE     *f;
	int yr,v;
	if(verbose & 0x0010) LOG_INFO("  Writing flat 3D scan for theta = %d \n",theta);
	if( theta == 0 ) 
		f = fopen(filename,"wb"); 
	else 
		f = fopen(filename,"ab+");
	if( f==NULL ) { LOG_INFO("  write_flat_3d_scan(): can't open file %s \n", filename);  return 0; }

	/* write line by line in sinogram mode from data stored in view mode */
	for( yr=yr_bottom[theta]; yr<=yr_top[theta]; yr++ ){
		for( v=0; v<sviews; v++ ) {
			if( fwrite( &scan[v][yr][0], radial_pixels*sizeof(float), 1, f) != 1 )
			{ LOG_INFO("  write_flat_3dscan(): write error at plane %d view %d \n", yr,v); fclose(f); return  0;}
		}
	}
	fclose(f);
	return 1;
}

/* 
 Read attenuation mu-map image and expand tfrom 128x128 to 256x256 if necessary
*/
int read_atten_image(FILE *fp, float ***image, unsigned input_type)
{
  LOG_INFO( "read_atten_image:TBD\n");
  exit(1);
}
