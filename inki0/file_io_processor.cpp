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
*/

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "scanner_model.h"
#include "hrrt_osem3d.h"
#include "nr_utils.h"

/****************************************************************************/
/* Reading routine for all sinogram file except for true & prompt when short*/

int read_flat_float_proj_ori_theta(FILE *fp, float *prj,int theta, float def_val) {        

	long offset;
	int i;
	long curptr;

	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;
	if (fp == NULL){
		int ds_size = xr_pixels*views*(yr_top[theta]-yr_bottom[theta]+1);
		for (i=0; i<ds_size; i++) prj[i] = def_val;
		return 1;
	}
	offset = (seg_offset[theta])*sizeof(float);

	//    if (verbose & 0x0010) fprintf(stdout," Read flat float projection at theta = %d\t offset = %d\n",theta,offset);
	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		fprintf(stderr,"  Read flat float projection: fseek problem \n");
		return 0;
	}
	curptr=0;
	//for(i=0;i<=yr_top[theta]-yr_bottom[theta];i++,curptr+=views *xr_pixels){
	//	fread(&prj[curptr], sizeof(float) *xr_pixels,views,fp);
	//}	  
	fread(prj,sizeof(float)*xr_pixels*views,yr_top[theta]-yr_bottom[theta]+1,fp);
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
	ds_size=views*xr_pixels*(yr_top[theta]-yr_bottom[theta]+1);
	ds_tmp = (short*)malloc(ds_size*sizeof(short));
	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;
	offset = (seg_offset[theta])*sizeof(short);

	//    if (verbose & 0x0010) fprintf(stdout," Read short projection 2 float at theta = %d\t offset =%d\n",theta,offset);

	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		fprintf(stderr,"  Read flat short projection 2 float: fseek problem \n");
		return 0;
	}
	curptr=0;
	fread(ds_tmp,sizeof(short),ds_size,fp);
	for(i=0;i<ds_size;i++) prj[i]=ds_tmp[i];
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
	int i;
	int curptr;

	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;
	offset = (seg_offset[theta])*sizeof(short);
	//    if (verbose & 0x0010) fprintf(stdout," Read short projection 2 short at theta = %d\t offset = %d\n",theta,offset);
	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		fprintf(stdout,"  Read flat short projection 2 short: fseek problem \n");
		exit(1);
	}
	curptr=0;
	for(i=0;i<yr_top[theta]-yr_bottom[theta]+1;i++,curptr+=views *xr_pixels){
		fread(&prj[curptr], sizeof(short) *xr_pixels,views,fp);
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

	ds_size = views*xr_pixels*sizeof(int);
	ds_tmp = (int*)malloc(ds_size);

	if(theta%2==1) theta++;
	else if(theta==0) theta=0;
	else if(theta%2==0) theta--;


	offset = (seg_offset[theta])*sizeof(int);
	if( (fseek(fp,offset,SEEK_SET)) != 0) {
		fprintf(stdout,"  Read Flat float Projection: fseek problem \n");
		return 0;
	}
	printf("short %d\t%d\n",theta,offset);
	curptr=0;
	for(i=0;i<(yr_top[theta]-yr_bottom[theta]+1);i++, curptr+=views*xr_pixels){
		fread(ds_tmp, sizeof(int)*xr_pixels,views,fp);
		ptr=&prj[curptr];
		for(j=0;j<views*xr_pixels;j++) ptr[j]= (float)ds_tmp[j];///100.0;
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
		fprintf(stdout,"  ... writing flat image at index %d (offset %d)\n", index, offset );

	if( index==0 ) 
		f = fopen(filename,"wb"); 
	else 
		f = fopen(filename,"ab+");

	if( f==NULL ) { 
		fprintf(stderr,"  write_flat_image(): can't open file %s \n", filename);  
		return 0; 
	}

  // write volume plane by plane, whole volume
  // image may be too large for writing on a network file
  count = x_pixels*y_pixels;
  plane = &image[0][0][0];
  for (z=0, plane = &image[0][0][0] ; z<z_pixels; z++, plane += count)
    if( fwrite(plane, sizeof(float), count, f) != count ) {
      fprintf(stderr,"  write_flat_image(): write error %d bytes\n", count*sizeof(float));
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
	if(verbose & 0x0010) fprintf(stdout,"  ... reading flat image at index %d (offset %d) \n", index, offset );

	f = fopen(filename,"rb");
	if(f == NULL) { fprintf(stdout,"  read_flat_image(): can't open file %s !\n",filename ); return 0; }

	if( fseek( f,  offset*sizeof(float),  SEEK_SET ) !=0 ) {    
		fprintf(stderr,"  read_flat_image(): fseek error at index=%d \n", index)  ;
		fclose(f);
		return 0;
	}
	if( fread( &image[0][0][0],sizeof(float), x_pixels*y_pixels*(z_pixels), f) != 1 ){
		fprintf(stderr,"  read_flat_image(): write error at  index=%d \n",  index );
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
	if (verbose & 0x0010) fprintf(stdout,"  ... writing norm subset %d at offset %d \n", isubset, offset*4 );
	if( isubset==0 ) 
		f = fopen(filename,"wb"); 
	else 
		f = fopen(filename,"ab+");
	if( f==NULL ) { 
		fprintf(stderr,"  write_norm(): can't open file %s \n", filename);  return 0; 
	}
	for(i=0;i<x_pixels;i++){
		if( fwrite( &norm[i][0][0],sizeof(float)*(z_pixels_simd)*4,y_pixels, f) != y_pixels){
			fprintf(stderr,"  write_norm(): write error at  isubset=%d  \n",  isubset );
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
	if(verbose & 0x0010) fprintf(stdout,"  ... reading norm for subset %d at offset %d \n", isubset, offset );
	f = fopen(filename,"rb");
	if( f == NULL ) { 
		fprintf(stdout,"  read_norm(): can't open file %s !\n",filename ); 
		return 0; 
	}
	if( fseek( f,  offset*sizeof(float),  SEEK_SET ) !=0 ) {    
		fprintf(stdout,"  read_norm(): fseek error at isubset=%d \n", isubset)  ;
		fclose(f);
		return 0;
	}
	curptr=0;
	for(i=0;i<x_pixels;i++){
		if( fread( &norm[i][0][0],sizeof(float)*(z_pixels_simd)*4,y_pixels, f) != y_pixels ){    
			fprintf(stdout,"  read_norm(): read error at  isubset=%d \n",  isubset );
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
	if(verbose & 0x0010) fprintf(stdout,"  Writing flat 3D scan for theta = %d \n",theta);
	if( theta == 0 ) 
		f = fopen(filename,"wb"); 
	else 
		f = fopen(filename,"ab+");
	if( f==NULL ) { fprintf(stdout,"  write_flat_3d_scan(): can't open file %s \n", filename);  return 0; }

	/* write line by line in sinogram mode from data stored in view mode */
	for( yr=yr_bottom[theta]; yr<=yr_top[theta]; yr++ ){
		for( v=0; v<sviews; v++ ) {
			if( fwrite( &scan[v][yr][0], radial_pixels*sizeof(float), 1, f) != 1 )
			{ fprintf(stdout,"  write_flat_3dscan(): write error at plane %d view %d \n", yr,v); fclose(f); return  0;}
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
  fprintf(stdout, "read_atten_image:TBD\n");
  exit(1);
}