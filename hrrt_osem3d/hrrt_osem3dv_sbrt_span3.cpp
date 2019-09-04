/*
Copyright (C) 2006, Z.H. Cho, Neuroscience Research Institute, Korea, zcho@gachon.ac.kr, http://nri.gachon.ac.kr/english/index.asp
Inki hong, Kore Polytechnic University, Korea, isslhong@kpu.ac.kr

This code is not free software; you cannot redistribute it without permission from Z.H. Cho and Inki Hong.
This code is only for research/educational purpose.
this code is patented and is not supposed to be used in other commercial scanner.
For commercial use, please contact zcho@gachon.ac.kr or isslhong@kpu.ac.kr
*/

/*	
*
*	Subroutines for 3D-OSEM algorithm; called by HRRT_osem3d.c
*
*		dependencies(): parameter handling 
*		back_proj3d():  backprojection over one full subset => back_proj3d_view_n
*		forward_proj3d(): forward projection over one full subset => forward_proj3d_view
*/

/* Modifications history:
   08-MAY-2008: Bug fix for low resolution mode (M. Sibomana)
 - 02-JUL-2009: Add dual logging to console and file
*/
#include "compile.h"

#include "mm_malloc.h"
#include <time.h>

#include "hrrt_osem3d.h"
#include "nr_utils.h"

// ahc
#include <algorithm>

extern int nthreads;

//int group, del, zmin, zmax, iminp, imaxp, iminm, imaxm;

/* 
*	parameters for osem
*/
int subsets = 16;		/* number of subsets  */
int sviews;			/* number of views in one subset; sviews=views/subsets */

/*
*	parameters of 2D-projections 
*/
int xr_pixels = 0;		/* number of Xr pixels; radial;assigned later */
int yr_pixels = 0;		/* number of Yr pixels; same as z_pixels        */
//int xr_off, yr_off;		/* indices where the origin should be */
int th_pixels = 0;		/* number of theta; including +&- ; 2*rings-1 */
int th_min = 0;			/* minimum theta value, normally 0 i.e.2D subset */
/*
*	image parameters; some of them can be modified from command line
*/
int x_pixels = 0;
int y_pixels = 0;
//int x_off, y_off, z_off;
float x_size, y_size;

/* frequently used array and constants  */

int imagesize;             /* number of voxels */
int subsetsize;            /* number of pixels in one subset */
int segmentsize;
int sinosize;              /* number of pixels in one sinogram  */
int *yr_bottom, *yr_top;   /* array length [th_pixels]; range for each theta  */
int *seg_offset;           /* array length [th_pixels]; */
int *segment_offset;       /* array length [th_pixels]; */
int *view_offset;          /* array length [th_pixels]; */
short **norm_zero_mask;
float *sin_theta;          /* array length [th_pixels]; sin(theta)  */
float *cos_theta;          /* array length [th_pixels]; cos(theta)  */
float *sin_psi;            /* array length [views]; sin(view)  */
float *cos_psi;            /* array length [views]; cos(view)  */
//short int **cyl_window;    /* full circular FOV; 1 if inside FOV, otherwise 0 ; [x][y] */
//short int **cyl_window2;   /* restricted circular FOV; 0 if inside FOV, otherwise 1 ; [x][y] */
short int **cylwiny;
/* array to handle end slices  */
int ZShift = 0;
float **vcos;     /* value of cos(theta) at position z_shift; vcos[theta][zshift]; 
				  theta goes from 0 to groupmax-1 (1 --- groupmax), zshift goes from
				  0 to (span-2)  */
float **vsin;
float **rescalearray;
float **tantheta;
int forward_proj3d_view1_thread(float ***image,float ***prj,int view,int theta2,int verbose,
								int xr_start,int xr_end,float ***imagebuf);
int back_proj3d_view_n1thread(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end);

typedef struct {
	unsigned short z_start;
	unsigned short yr_start;
	unsigned short z_end;
	unsigned short z_start_pre;
	unsigned short yr_start_pre;
	unsigned short z_end_pre;
	float coef;
//	float coefsub;
	float coef_pre;
	short num;
} BACKPROJECTION_ACCUMULATOR;

BACKPROJECTION_ACCUMULATOR **blutacc;
char **blutacc_flag;

Backprojection_lookup **blookup;//[256][22];
Backprojection_lookup_oblique ***blookup_oblique;//[256][22];
Backprojection_lookup_oblique *blookup_oblique_ptr;//[256][22];
Backprojection_lookup_oblique_start_end **blookup_oblique_start_end;//[256][22];
int blookup_oblique_maxnum;
int blookup_oblique_totalnum;
Forwardprojection_lookup **flookup;

/* Benchmarking stuff */
//float fForwardProjSecs;
//float fBackProjSecs;

int back_proj3d_view_n1 (float ***image,float ***prjbuf,int view,int theta2,int verbose,int x_start ,int x_end);
int back_proj3d_view_n1_oblique (float ***image,float ***prjbuf,int view,int theta2,int verbose,int x_start ,int x_end);
int rotate3d(float ***ima,float ***out,int view, int xstart, int xend);
int rotate3d2(float ***ima,float ***out,int view, int xstart, int xend);

int start_x__per_thread[16];			// is it related to default subset 16?? CM
int start_x__per_thread_back[16];
int start_x__per_thread_back_8sysmetric[16];

/*
* make vieworder array.
* vieworder[# of subset][# of sviews] = # of view.
*
* ex: subsets = 16
*
* subset                                                                     subset
*   0                                                                          15
* +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
* |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
* | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |
* | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 | 32 | 33 | 34 | 35 | 36 | 37 |
* | .. | .. | .. | .. | .. | .. | .. | .. | .. | .. | .. | .. | .. | .. | .. | .. |
* |282 |283 |284 |285 |286 |287 |288 |289 |280 |281 |282 |283 |284 |285 |286 |287 |
* +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
*/
void calculatevieworder(int **vieworder,int subsets,int th_pixels,int sviews) {

	int i,k;
	for(i=0;i<subsets;i++){
		for(k=0;k<sviews;k++){
			vieworder[i][k]=i+subsets*k;
		}
	}
}

char log_file[_MAX_PATH];
unsigned int log_mode = LOG_TO_CONSOLE;


 // calculate range of planes of each group.
void get_info(int nrings,int span,int rmax,int group,int* del,int* zmin,int* zmax,int* imin,int* imax,int* imin1,int* imax1) {
	int j;

	if (group == 0) {		
		*del = 0;
		*zmin = 0;
		*zmax = 2 * nrings - 2;
		*imin = 0;
		*imax = 2 * nrings - 2;
		*imin1 = *imin;
		*imax1 = *imax;
	}
	if (group == 1) {
		*del = span;
		*zmin = (span + 1) / 2;	
		*zmax = 2 * nrings - 2 - *zmin;
		*imin = 2 * nrings - 1;
		*imax = *imin + (*zmax - *zmin);
		*imin1 = *imax + 1;
		*imax1 = *imin1 + (*zmax - *zmin);
	}
	if (group > 1) {
		*del = group * span;
		*zmin = (span + 1) / 2 + (group - 1) * span;
		*zmax = 2 * nrings - 2 - *zmin;
		*imin = 2 * nrings - 1;
		for (j = 1; j < group; ++j)
			*imin += 2 * (2 * nrings - 1 - 2 * ((span + 1) / 2 + (j - 1) * span));
		*imax = *imin + (*zmax - *zmin);
		*imin1 = *imax + 1;
		*imax1 = *imin1 + (*zmax - *zmin);
	}
}

/*******************************************************************************************************/
/*
*
* @param nprojs  : # of radial samples.
* @param nviews : # of views(angles).
*/
int dependencies(int nprojs,int  nviews,int  verbose) {

	float  tmp_flt, ffov;
	int    x, y, v;
	int    z = 0, mean_rd = 0;
	int    cylflag;
	int 	group, del, zmin, zmax, iminp, imaxp, iminm, imaxm;
	float 	zoomlocal=1.0;
	int    	i;
	int   	*in_fov_pixels_at_x;
	int   	*in_fov_pixels_at_x2;
	int	x_off,y_off,z_off;
	int 	th,th2,yr,yri,zshift;
	float 	zz0;
	float 	tantheta_back[MAXGROUP];
	float  	yr0;
	float  	yr1; // = yri2 + yr2  ,ex:[ yr2 = -1.3 | yr2i = -2.0 | yr2 = 0.7 ]
	float  	yr2;
	float	zz1,zz2;
	int    	yri2;
	int 	zs,ze;

	if(zoom!=1 || newzoom!=1){
		if(zoom!=1) zoomlocal=zoom;
		else if(newzoom!=1) zoomlocal=newzoom;
	} 
	//	zoomlocal=1;
	if (maxdel == 0) maxdel = rings - 1;
	if ((max_g != -1) && (max_g <= groupmax)) groupmax = max_g;
	if (max_g != groupmax) maxdel = groupmax * span + (span - 1) / 2;

	/* nprojs and nviews are specified from command line  */
	if (nprojs != 0) {
		if (nprojs != radial_pixels) {
			if (verbose & 0x0001) LOG_INFO("  Radial resampling from %d to %d pixels, i.e. from %.5f mm to ",radial_pixels, nprojs, sino_sampling);
			sino_sampling *= (float) radial_pixels / (float) nprojs;
			if (verbose & 0x0001) LOG_INFO( " %.5f mm\n", sino_sampling);
		}
		radial_pixels = nprojs;
	}
	if (nviews != 0) views = nviews;

	/************************************/
	/* image parameters                 */
	/* x_pixels , y_pixels,imagesize    */
	/* x_off,y_off,z_off                */
	/* x_size,y_size,z_size             */
	/* IF x_pixels or y_pixels are not set from the command line, THEN assign here */
	if (x_pixels == 0) x_pixels = radial_pixels;
	if (y_pixels == 0) y_pixels = radial_pixels;

	in_fov_pixels_at_x = (int *)malloc(x_pixels*sizeof(int));
	in_fov_pixels_at_x2 = (int *)malloc(x_pixels*sizeof(int));

	/* offsets */	
	x_off = x_pixels / 2;
	y_off = y_pixels / 2;
	z_off = z_pixels / 2;
	if (verbose & 0x0001) LOG_INFO( "  Image offsets: x_off=%d, y_off=%d, z_off=%d\n",x_off, y_off, z_off);

	/* x_size, y_size, z_size */
	x_size = (sino_sampling * radial_pixels) / (zoomlocal * x_pixels);
	y_size = x_size;
	z_size = ring_spacing / 2.0f;
	if (verbose & 0x0001)
		LOG_INFO( "  at zoom = %f, pixel_size is %f zoom=%f newzoom=%f mm\n", zoomlocal,x_size,zoom,newzoom);
	imagesize  = z_pixels * x_pixels  * y_pixels;

	/************************************/
	/* osem_parameter                   */
	/* views, subsets, sviews           */
	if ((views % subsets) != 0) {
		LOG_INFO("  Number of views %d should be multiple of subsets %d \n",views, subsets);
		return 0;
	}
	sviews = views / subsets;

	/************************************	
	* parameters of 2D-projections
	* xr_pixels     : number of Xr pixels; radial;assigned later 
	* yr_pixels     : measured Yr pixels; same as z_pixels		
	* xr_off, yr_off: indices where the origin should be 
	* th_pixels     : number of theta; including +&- ; 2*rings-1 
	* th_min        : minimum value of theta 
	*/
	xr_pixels = radial_pixels;
	yr_pixels = z_pixels;    
	th_pixels = 2 * groupmax + 1;
	th_min    = std::max(2 * groupmin - 1, 0);
	//    	xr_off    = xr_pixels / 2;
	//    	yr_off    = yr_pixels / 2;

	/************************************	
	* scanner parameters
	*/
	rfov = rel_fov * x_size * x_pixels / 2.0f;       /* reconstructed FOV radius */
	ffov = rel_fov * x_size * (x_pixels - 2) / 2.0f; /* final FOV - one pixel smaller - local variable */
	if(rfov > sino_sampling * radial_pixels/2.0 && newzoom!=1){
		rfov=(float) sino_sampling * radial_pixels/2.0f;
	}


	if (verbose & 0x0001) {
		LOG_INFO( "  Additional parameters \n");
		LOG_INFO( "  ----------------------\n");
		LOG_INFO( "  Number of subsets and sviews  : %d , %d \n",subsets, sviews);
		LOG_INFO( "  Measured projections (xr*yr)  : %d  x  %d \n",xr_pixels, yr_pixels);
		LOG_INFO( "  Number of segments            : %d \n",th_pixels);
		LOG_INFO( "  Image size (X*Y*Z)            : %d  x  %d x %d \n",x_pixels, y_pixels, z_pixels);
		//		LOG_INFO( "  Xr offset                     : %d \n", xr_off);
		//		LOG_INFO( "  Yr offset                     : %d \n", yr_off);
		LOG_INFO( "  Maximum ring difference       : %d \n", maxdel);
		LOG_INFO( "  Miminum group [segment]       : %d [%d]\n",groupmin, th_min);
		LOG_INFO( "  Maximum group [segment]       : %d [%d]\n",groupmax, th_pixels);
		LOG_INFO( "  Reconstructed FOV radius      : %f mm (%f %%) \n", rfov,rel_fov);
		LOG_INFO( "  Final FOV radius              : %f mm \n",ffov);
	}

	/* frequently used arrays and constants  */
	ZShift         = span - 1;
	yr_bottom      = (int *)   calloc(sizeof(int), th_pixels*2);
	yr_top         = (int *)   calloc(sizeof(int), th_pixels*2);
	seg_offset     = (int *)   calloc(sizeof(int), th_pixels);
	segment_offset = (int *)   calloc(sizeof(int), th_pixels);
	view_offset    = (int *)   calloc(sizeof(int), views);
	sin_theta      = (float *) calloc(sizeof(float), th_pixels);
	cos_theta      = (float *) calloc(sizeof(float), th_pixels);
	sin_psi        = (float *) calloc(sizeof(float), views);
	cos_psi        = (float *) calloc(sizeof(float), views);
	cylwiny	       = (short int **) matrix_i(0, y_pixels - 1, 0, 1);
	vcos           = (float **) matrixfloat(0, groupmax - 1, 0, ZShift - 1);
	vsin           = (float **) matrixfloat(0, groupmax - 1, 0, ZShift - 1);
	norm_zero_mask = (short **) matrixshort(0,views/2,0,x_pixels/2);

	for(v=0;v<views/2;v++) for(x=0;x<=x_pixels/2;x++) norm_zero_mask[v][x]=1;
	i=0;
	memset( in_fov_pixels_at_x, 0, x_pixels*sizeof(int) );
	memset( in_fov_pixels_at_x2, 0, x_pixels*sizeof(int) );

	/* reconstructed fov and final mask */
	for (y = 0; y < y_pixels; y++){
		cylflag=0;
		cylwiny[y][0] = cylwiny[y][1]=0;
		for (x = 0; x < x_pixels; x++) {
			if (sqrt((x-x_off)*x_size*(x-x_off)*x_size+(y-y_off)*y_size*(y-y_off)*y_size) <= rfov){
				if(cylflag==0) cylwiny[y][0]=x;
				cylflag=1;
				i ++;
				in_fov_pixels_at_x[x]++;
				if(x<=y) in_fov_pixels_at_x2[y]++;
			} else {
				if(cylflag==1) 
					cylwiny[y][1]=x;
				cylflag=0;
			}
		}
		//	LOG_INFO(" Mask boundaries at line y %d\t%d\t%d\n",y,cylwiny[y][0],cylwiny[y][1]);
	}
	x = 0;
	y = 0;
	v = 0;
	z = 0;
	LOG_INFO("  Pixels in rfov [%d]\n",i);
	i = i/nthreads;
	if(i<0) i = 0;

	start_x__per_thread[0] =0;
	for (x=0; x<x_pixels;x++) {
		if (y== nthreads-1 || v<i) {
			v += in_fov_pixels_at_x[x];
		} else {
			y++;
			start_x__per_thread[y] = x-z;
			z =x;
			v = in_fov_pixels_at_x[x];
		}
	}	
	x = 0;
	y = 0;
	v = 0;
	z = 0;
	//	LOG_INFO("[%d]\n",i);
	i = i/2;
	if(i<0) i = 0;

	start_x__per_thread_back[0] =0;
	for (x=0; x<=x_pixels/2;x++) {
		if (y== nthreads-1 || v<i) {
			v += in_fov_pixels_at_x[x];
		} else {
			y++;
			start_x__per_thread_back[y] = x-z;
			z =x;
			v = in_fov_pixels_at_x[x];
		}
	}	
	for(i=0;i<nthreads;i++){
		LOG_INFO("threads {} {} {}",i,start_x__per_thread_back[i],cylwiny[x_pixels/2][0]);
	}
	free(in_fov_pixels_at_x);
	x = y = v = z =0;
	i=0;
	for(x=0;x<=x_pixels/2;x++){
		i+=in_fov_pixels_at_x2[x];
		//		LOG_INFO("%d\t%d\t%d\n",x,i,in_fov_pixels_at_x2[x]);
	}

	i=i/nthreads;
	start_x__per_thread_back_8sysmetric[0] =0;
	for (x=0; x<=x_pixels/2;x++) {
		if (y== nthreads-1 || v<i) {
			v += in_fov_pixels_at_x2[x];
		}
		else {
			y++;
			start_x__per_thread_back_8sysmetric[y] = x-z;
			z =x;
			//			LOG_INFO("%d\t%d\t%d\t%d\n",i,y,start_x__per_thread_back_8sysmetric[y],v);
			v = in_fov_pixels_at_x2[x];
		}
	}	
	//	LOG_INFO("%d\n",i);
	//	for(i=0;i<nthreads;i++) LOG_INFO("%d\t%d\n",i,start_x__per_thread_back_8sysmetric[i]);
	free(in_fov_pixels_at_x2);
	sinosize = radial_pixels * views;
	sin_theta[0] = 0.0;
	cos_theta[0] = 1.0;        

	for (group = 1; group <= groupmax; group++) {
		tmp_flt = group * span * ring_spacing;
		tmp_flt =(float)sqrt(tmp_flt * tmp_flt +(2 * ring_radius) * (2 * ring_radius)); 
		sin_theta[2 * group - 1] = group * span * ring_spacing / tmp_flt;
		cos_theta[2 * group - 1] = (2.0f * ring_radius) / tmp_flt;
		sin_theta[2 * group] = -sin_theta[2 * group - 1];
		cos_theta[2 * group] = cos_theta[2 * group - 1];
		if (verbose & 0x0001) LOG_INFO("  cos_theta[%d] = %f %2.20lf %2.20lf\n", group,cos_theta[2 * group - 1],-sin_theta[2 ]/cos_theta[2 ]*group,-sin_theta[2 * group]/cos_theta[2 * group]);
		/* LX: end of each axial groups has a theta which is different from the mean of that group */
		for (z = 0; z < ZShift; z++) {
			zmin = span * group - (span - 1) / 2;
			mean_rd = zmin + (z + 1) / 2;
			tmp_flt = mean_rd * ring_spacing;
			tmp_flt =(float)sqrt(tmp_flt * tmp_flt +(2 * ring_radius) * (2 * ring_radius)); 
			vcos[group - 1][z] = (2.0f * ring_radius) / tmp_flt;
			vsin[group - 1][z] = mean_rd * ring_spacing / tmp_flt;
		}
	}

	yr_bottom[0] = 0;
	yr_top[0]= z_pixels - 1;
	seg_offset[0] = 0;
	for (group = 1; group <= groupmax; group++) {
		get_info(rings, span, maxdel, group, &del, &zmin, &zmax, &iminp,&imaxp, &iminm, &imaxm);
		yr_bottom[2 * group] = zmin;
		yr_top[2 * group] = zmax;
		seg_offset[2 * group] = iminp * sinosize;	/* group offset in sixels , needed group order is 0,-1,1,-2,2... */
		yr_bottom[2 * group - 1] = yr_bottom[2 * group];
		yr_top[2 * group - 1] = yr_top[2 * group];
		seg_offset[2 * group - 1] = iminm * sinosize;
	}
	segmentsize=0;	// is the total number of planes
	for(v=0;v<th_pixels;v++){
		yr_bottom[th_pixels+v]= yr_bottom[v];
		yr_top[th_pixels+v]   = yr_top[v];
		segment_offset[v]     = segmentsize;
		segmentsize          += yr_top[v]-yr_bottom[v]+1;
	}
	for(v=0;v<views;v++){
		view_offset[v]=v*segmentsize*xr_pixels;
	}

	if (verbose & 0x0001) {
		LOG_INFO("  Toatl number of planes %d \n",segmentsize);
		LOG_INFO("  Range and sixel_offset for each theta (group) \n");
		for (group = th_min; group < th_pixels; group++)
			LOG_INFO("  yr_bottom[%d] = %d; yr_top[%d] = %d offset[%d] = %d \n", group, yr_bottom[group], group, yr_top[group], group,seg_offset[group]);
	}

	sin_psi[0] = 0.0;
	cos_psi[0] = 1.0;
	sin_psi[views / 2] = 1.0;
	cos_psi[views / 2] = 0.0;
	tmp_flt = PI / views;
	for (v = 1; v < views / 2; v++) {
		sin_psi[v] = (float)sin(v * tmp_flt); 			
		cos_psi[v] = (float)sqrt(1. - sin_psi[v] * sin_psi[v]); 
		sin_psi[views - v] = sin_psi[v];
		cos_psi[views - v] = -cos_psi[v];
	}

	subsetsize = sviews   * yr_pixels * xr_pixels;

	/* Calculate normalization factors: Backprojection from estimate to normfac[][][] for each subset */
	vieworder=(int **) calloc(subsets,sizeof(int*));
	if(vieworder==NULL) LOG_EXIT("exiting");
	for(i=0;i<subsets;i++){
		vieworder[i]=(int *) calloc(sviews,sizeof(int));
		if(vieworder[i]==NULL){
			LOG_EXIT("error \n");
		}
	}	
	calculatevieworder(vieworder,subsets,th_pixels,sviews);

	/*---------------------------------------------------------*/
	/* not extern.... */
	/* used in rotation : output of rotation*/
	rescalearray=(float **) matrixfloat(0,groupmax-1,0,z_pixels-1);
	tantheta=(float **) matrixfloat(0,groupmax-1,0,z_pixels-1);
	memset(&tantheta[0][0],0,groupmax*z_pixels*4);
	memset(&rescalearray[0][0],0,groupmax*z_pixels*4);
	flookup=(Forwardprojection_lookup **) _mm_malloc(xr_pixels*sizeof(Forwardprojection_lookup *),16);
	for(i=0;i<xr_pixels;i++){
		flookup[i]=(Forwardprojection_lookup *) _mm_malloc(groupmax*2*ZShift*sizeof(Forwardprojection_lookup),16);
	}
	blookup=(Backprojection_lookup **) _mm_malloc(x_pixels*sizeof(Backprojection_lookup*),16);
	for(x=0;x<x_pixels;x++){
		blookup[x]=(Backprojection_lookup *) _mm_malloc(groupmax*sizeof(Backprojection_lookup),16);
	}

	blutacc=(BACKPROJECTION_ACCUMULATOR **) _mm_malloc(x_pixels*sizeof(BACKPROJECTION_ACCUMULATOR*),16);
	blutacc_flag=(char **) _mm_malloc(x_pixels*sizeof(short *),16);
	for(x=0;x<x_pixels;x++){
		blutacc[x]=(BACKPROJECTION_ACCUMULATOR *) _mm_malloc(groupmax*sizeof(BACKPROJECTION_ACCUMULATOR),16);
		blutacc_flag[x]=(char *) _mm_malloc(groupmax*sizeof(short),16);
	}

	blookup_oblique_start_end=(Backprojection_lookup_oblique_start_end **) _mm_malloc(x_pixels*sizeof(Backprojection_lookup_oblique_start_end*),16);
	for(x=0;x<x_pixels;x++){
		blookup_oblique_start_end[x]=(Backprojection_lookup_oblique_start_end *) _mm_malloc(groupmax*sizeof(Backprojection_lookup_oblique_start_end),16);
	}

	for(th=2,th2=0;th<th_pixels;th+=2,th2++){
		for (yr = yr_bottom[th]; yr <= yr_top[th]; yr++) {				
			tantheta[th2][yr]     = sin_theta[th] / cos_theta[th];
			rescalearray[th2][yr] = y_size / (float)fabs(cos_theta[th]);				
			zshift = yr - yr_bottom[th];
			if (zshift >= ZShift) zshift = yr_top[th] - yr;
			if (zshift < ZShift) {
				tantheta[th2][yr] = vsin[th2][zshift] / vcos[th2][zshift];			
				tantheta[th2][yr] = -tantheta[th2][yr];
				rescalearray[th2][yr] =y_size / (float)fabs(vcos[th2][zshift]);
			}			
			tantheta[th2][yr] = - tantheta[th2][yr]/zoomlocal;///y_size*z_size;
		}
	}

	for(y=0;y<y_pixels;y++){
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			for(yr=yr_bottom[th],yri=0;yr<yr_bottom[th]+ZShift;yr++,yri++){
				zz0=yr+((y-y_off)*tantheta[th2][yr]);
				z=(int)zz0; 
				zz0=zz0-z;
				flookup[y][th2*ZShift*2+yri].zzz0=zz0;
				flookup[y][th2*ZShift*2+yri].zzi=z;
				zz0=yr+((y-y_off)*tantheta[th2][yr]);
			}

			for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++,yri++){
				zz0=yr+((y-y_off)*tantheta[th2][yr]);
				z=(int)zz0; 
				zz0=zz0-z;
				flookup[y][th2*ZShift*2+yri].zzz0=zz0;
				flookup[y][th2*ZShift*2+yri].zzi=z;
				zz0=yr+((y-y_off)*tantheta[th2][yr]);
			}
			//			LOG_INFO("%d\t%d\t",flookup[y][th2*ZShift*2+ZShift].zzi,flookup[y][th2*ZShift*2+yri-1].zzi+1);
			yri2=flookup[y][th2*ZShift*2+ZShift-1].zzi-flookup[y][th2*ZShift*2].zzi+flookup[y][th2*ZShift*2+yri-1].zzi-flookup[y][th2*ZShift*2+ZShift].zzi+2+2;
			blookup_oblique_start_end[y][th2].z1=flookup[y][th2*ZShift*2].zzi;
			blookup_oblique_start_end[y][th2].z2=flookup[y][th2*ZShift*2+ZShift-1].zzi+1;
			blookup_oblique_start_end[y][th2].z3=flookup[y][th2*ZShift*2+ZShift].zzi+1;
			blookup_oblique_start_end[y][th2].z4=flookup[y][th2*ZShift*2+ZShift*2-1].zzi+1;

			blookup_oblique_start_end[y][th2].num=blookup_oblique_start_end[y][th2].z2-blookup_oblique_start_end[y][th2].z1+blookup_oblique_start_end[y][th2].z4-blookup_oblique_start_end[y][th2].z3+2;

		//		LOG_INFO("%d\n",blookup_oblique_start_end[y][th2].num);
	//			LOG_INFO("%d\t%d\t%d\t%d\t%d\t%d\n",y,th,flookup[y][th2*ZShift*2].zzi,flookup[y][th2*ZShift*2+ZShift*2-1].zzi
	//					,flookup[y][th2*ZShift*2+1].zzi,flookup[y][th2*ZShift*2+ZShift*2-2].zzi);
		}
	}

	/* allocation memory for blookup oblique lookup table */
	blookup_oblique_maxnum=0;
	blookup_oblique_totalnum=0;
	for(y=0;y<y_pixels;y++){
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			if(blookup_oblique_maxnum<blookup_oblique_start_end[y][th2].num){
				blookup_oblique_maxnum=blookup_oblique_start_end[y][th2].num;
			}
			blookup_oblique_totalnum+=blookup_oblique_start_end[y][th2].num;
		}
	}
	LOG_INFO("total num {}",blookup_oblique_totalnum);
	blookup_oblique_ptr=(Backprojection_lookup_oblique *) _mm_malloc(blookup_oblique_totalnum*sizeof(Backprojection_lookup_oblique),16);
	blookup_oblique=(Backprojection_lookup_oblique ***) _mm_malloc(y_pixels*sizeof(Backprojection_lookup **),16);
	yri=0;
	for(y=0;y<y_pixels;y++){
		blookup_oblique[y]=(Backprojection_lookup_oblique **) _mm_malloc(groupmax*sizeof(Backprojection_lookup *),16);
		for(group=0;group<groupmax;group++){
			blookup_oblique[y][group]=&blookup_oblique_ptr[yri];
			yri+=blookup_oblique_start_end[y][group].num;
		}
	}
	//	LOG_INFO("done memalloc\n");
	/******************************************************/

	/* calculating backward projection oblique ray's interpolation coefficients */
	for(y=0;y<y_pixels;y++){
		for(group=0;group<groupmax;group++){
			blookup_oblique[y][group][0].coef=flookup[y][group*ZShift*2].zzz0;
			blookup_oblique[y][group][0].yr=yr_bottom[group*2+1]-1;
			blookup_oblique[y][group][0].z=flookup[y][group*ZShift*2].zzi;
			for(zs=1,yr=0,z=blookup_oblique_start_end[y][group].z1+1;z<blookup_oblique_start_end[y][group].z2;z++,zs++){
				//			LOG_INFO("%d\t%d\t%d\t%d\ttotal %d\t%d\n",y,group,blookup_oblique_start_end[y][group].z1+1,blookup_oblique_start_end[y][group].z2,blookup_oblique_totalnum,zs);
				zz1=flookup[y][group*ZShift*2+yr].zzi+flookup[y][group*ZShift*2+yr].zzz0;
				zz2=flookup[y][group*ZShift*2+yr+1].zzi+flookup[y][group*ZShift*2+yr+1].zzz0;
				if(z>=zz1 && z<zz2){
					blookup_oblique[y][group][zs].z=z;
					blookup_oblique[y][group][zs].yr=yr+yr_bottom[group*2+1];
					blookup_oblique[y][group][zs].coef=(zz2-z)/(zz2-zz1);
				} else {
					z--;
					zs--;
					yr++;
				}
			}
			blookup_oblique[y][group][zs].coef=flookup[y][group*ZShift*2+ZShift-1].zzz0;
			blookup_oblique[y][group][zs].yr=yr_bottom[group*2+1]+ZShift-1;
			blookup_oblique[y][group][zs].z=blookup_oblique_start_end[y][group].z2;
			zs++;
			yr=ZShift;
			for(z=blookup_oblique_start_end[y][group].z3;z<blookup_oblique_start_end[y][group].z4;z++,zs++){
				//		LOG_INFO("%d\t%d\t%d\t%d\ttotal %d\t%d\n",y,group,blookup_oblique_start_end[y][group].z3,blookup_oblique_start_end[y][group].z4,blookup_oblique_totalnum,zs);
				zz1=flookup[y][group*ZShift*2+yr].zzi+flookup[y][group*ZShift*2+yr].zzz0;
				zz2=flookup[y][group*ZShift*2+yr+1].zzi+flookup[y][group*ZShift*2+yr+1].zzz0;
				if(z>=zz1 && z<=zz2){
					blookup_oblique[y][group][zs].z=z;
					blookup_oblique[y][group][zs].yr=yr+yr_top[group*2+1]-ZShift*2+1;
					blookup_oblique[y][group][zs].coef=(zz2-z)/(zz2-zz1);
					//			LOG_INFO("%f\t%f\t%d\n",zz1,zz2,z);
				} else {
					z--;
					zs--;
					yr++;
				}
				//			LOG_INFO("%f\t%f\t%d\n",zz1,zz2,z);
			}
			blookup_oblique[y][group][zs].coef=flookup[y][group*ZShift*2+ZShift*2-1].zzz0;
			blookup_oblique[y][group][zs].yr=yr_top[group*2+1];
			blookup_oblique[y][group][zs].z=blookup_oblique_start_end[y][group].z4;
		}
	}
	for(y=0;y<y_pixels;y++){
		for(group=0;group<groupmax;group++){
			//			LOG_INFO("y,group %d\t%d\n",y,group);
			for(yr=0;yr<blookup_oblique_start_end[y][group].num;yr++){
				//				LOG_INFO("back %d\t%d\t%f\n",blookup_oblique[y][group][yr].z,blookup_oblique[y][group][yr].yr,blookup_oblique[y][group][yr].coef);
			}
			for(yr=0;yr<ZShift;yr++){
				//				LOG_INFO("ford %d\t%f\n",yr+yr_bottom[group*2+1],flookup[y][group*ZShift*2+yr].zzi+flookup[y][group*ZShift*2+yr].zzz0);
			}
			for(yr=0;yr<ZShift;yr++){
				//				LOG_INFO("ford %d\t%f\n",yr+yr_top[group*2+1]-ZShift+1,flookup[y][group*ZShift*2+yr+ZShift].zzi+flookup[y][group*ZShift*2+yr+ZShift].zzz0);
			}
		}
	}
	//	for(group=0;group<groupmax;group++){
	//		for(y=0;y<y_pixels;y++){
	//			LOG_INFO("start\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",y,group,blookup_oblique_start_end[y][group].num,				blookup_oblique_start_end[y][group].z1,				blookup_oblique_start_end[y][group].z2,				blookup_oblique_start_end[y][group].z3,				blookup_oblique_start_end[y][group].z4);
	//		}
	//	}
	//	LOG_EXIT("exiting");

	for(th=2,th2=0;th<th_pixels;th+=2,th2++) {		
		tantheta_back[th2]=-sin_theta[th]/cos_theta[th];
	}	
	for (y = 0; y < y_pixels; y++) {
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			yr0=-(y-y_off)*tantheta_back[th2];	
			yr1=yr0;
			yr2=(float)fabs(yr0)-(int) fabs(yr0); 	
			yri2=(int)yr1; 
			if(yr1<0){
				yr2=1-yr2;
				yri2-=1;
			}
			zs = yr_bottom[th]-1-yri2;
			ze = yr_top[th]   -yri2;
			yr = yr_bottom[th]-1;

			if(zs<0) {
				yr=yr-zs;
				zs=0;
			}
			if(ze>z_pixels-1){
				ze=z_pixels-1;
			}
			blookup[y][th2].zsa=zs;
			blookup[y][th2].zea=ze;
			blookup[y][th2].yra=yr;
			blookup[y][th2].yr2a=yr2;
//			LOG_INFO("back\t%d\t%d\t%d\t%d\t%d\t%f\n",y,th2,zs,ze,yr,yr2);
		}
	}


/*	typedef struct {
		short z_start;
		short yr_start;
		short z_end;
		short yr_end;
		float coef;
		short num;
		short z_start_pre;
		short yr_start_pre;
		short z_end_pre;
		short yr_end_pre;
		float coef_pre;
		short num_pre;
	} BACKPROJECTION_ACCUMLATOR;
*/	
	for(th=2,th2=0;th<th_pixels;th+=2,th2++){
		for (y = 0; y < y_pixels; y++) {
				blutacc[y][th2].z_start=blutacc[y][th2].z_end=blutacc[y][th2].yr_start=blutacc[y][th2].coef=0;
				blutacc[y][th2].num=blutacc[y][th2].z_start_pre=blutacc[y][th2].z_end_pre=blutacc[y][th2].yr_start_pre=blutacc[y][th2].coef_pre=0;
				blutacc[y][th2].num=-1;
		}
	}
	for(th=2,th2=0;th<th_pixels;th+=2,th2++){
		zs=0;
		yr=0;
		group=y_pixels/2;
		blutacc_flag[y_pixels/2][th2]=1;
		blutacc[y_pixels/2][th2].z_start=blookup[y_pixels/2][th2].zsa;
		blutacc[y_pixels/2][th2].z_end=blookup[y_pixels/2][th2].zea;
		blutacc[y_pixels/2][th2].yr_start=blookup[y_pixels/2][th2].yra;
		blutacc[y_pixels/2][th2].coef=blookup[y_pixels/2][th2].yr2a;
		blutacc[y_pixels/2][th2].num=0;

		for (y = y_pixels/2+1; y < y_pixels; y++) {
			blutacc_flag[y][th2]=0;
			ze=blookup[y][th2].yra-blookup[y][th2].zsa;
			if(zs==ze){
				yr++;
			}
			else {
				blutacc_flag[y][th2]=yr;
				blutacc[y][th2].z_start=blookup[y][th2].zsa;
				blutacc[y][th2].z_end=blookup[y][th2].zea;
				blutacc[y][th2].yr_start=blookup[y][th2].yra;
				blutacc[y][th2].coef=blookup[y][th2].yr2a;
				blutacc[y][th2].num=yr;
				blutacc[y][th2].z_start_pre=blookup[group][th2].zsa;
				blutacc[y][th2].z_end_pre=blookup[group][th2].zea;
				blutacc[y][th2].yr_start_pre=blookup[group][th2].yra;
				blutacc[y][th2].coef_pre=blookup[group][th2].yr2a;
				group=y;
				yr=0;
				zs=ze;
			}
		}
		zs=0;
		yr=0;
		blutacc_flag[y_pixels/2][th2]=0;
		group=y_pixels/2;
		for (y = y_pixels/2-1; y >=0; y--) {
			blutacc_flag[y][th2]=0;
			ze=blookup[y][th2].yra-blookup[y][th2].zsa;
			if(zs==ze){
				yr++;
			}
			else {
				blutacc_flag[y][th2]=yr;
				blutacc[y][th2].z_start=blookup[y][th2].zsa;
				blutacc[y][th2].z_end=blookup[y][th2].zea;
				blutacc[y][th2].yr_start=blookup[y][th2].yra;
				blutacc[y][th2].coef=blookup[y][th2].yr2a;
				blutacc[y][th2].num=yr;
				blutacc[y][th2].z_start_pre=blookup[group][th2].zsa;
				blutacc[y][th2].z_end_pre=blookup[group][th2].zea;
				blutacc[y][th2].yr_start_pre=blookup[group][th2].yra;
				blutacc[y][th2].coef_pre=blookup[group][th2].yr2a;
				group=y;
				yr=0;
				zs=ze;
			}
		}
	}
/*	for(th=2,th2=0;th<th_pixels;th+=2,th2++){
		for (y = 0; y < y_pixels; y++) {
			LOG_INFO("%d\t%d\t%d\t%d\t%d\t%f\t%d\t%d\t%d\t%f\t%d\n",th2,y,
				blutacc[y][th2].z_start,
				blutacc[y][th2].z_end,
				blutacc[y][th2].yr_start,
				blutacc[y][th2].coef,
				blutacc[y][th2].z_start_pre,
				blutacc[y][th2].z_end_pre,
				blutacc[y][th2].yr_start_pre,
				blutacc[y][th2].coef_pre,
				blutacc[y][th2].num);
		}
	}*/
	return 1;
}/* dependencies()  */

void free_dependencies(){
	int i=0;
	free(yr_bottom  );
	free(yr_top     );
	free(seg_offset );    
	free(segment_offset);
	free(view_offset);    
	free(sin_theta  );    
	free(cos_theta  );   
	free(sin_psi    );    
	free(cos_psi    );  
	free_matrix_i(cylwiny,0, y_pixels - 1, 0, 1);
	free_matrix(vcos,0, groupmax - 1, 0, ZShift - 1);   
	free_matrix(vsin,0, groupmax - 1, 0, ZShift - 1); 
	for(i=0;i<subsets;i++) free(vieworder[i]);
	free(vieworder);
}

/*************
Pthreads implementation. 
Define a structure which holds parameters for 
pt_fproj_3d_view 
and
pt_bkproj_3d_view_n:
**************/


typedef struct {
	int view;
	int xstart;
	int xend;
	float ***ima;
	float ***out;
} RotateArg;

FUNCPTR pt_fproj_3d_view1(void *ptarg) {
	BPFP_ptargs *arg = (BPFP_ptargs *) ptarg;
	forward_proj3d_view1_thread(arg->image, arg->prj,arg->view,arg->theta, arg->verbose, arg->start, arg->end,arg->imagebuf);
//	forward_proj3d_view1_thread(arg->image, arg->prj,arg->view,arg->theta, arg->verbose, arg->start, arg->end);
	return 0;
}
FUNCPTR pt_bkproj_3d_view_n1(void *ptarg) {
	BPFP_ptargs *arg = (BPFP_ptargs *) ptarg;		
	back_proj3d_view_n1(arg->image, arg->prj,arg->view90,arg->view, arg->verbose, arg->start, arg->end);
	return 0;
}
FUNCPTR pt_rotate3d(void *ptarg) {
	RotateArg *arg=(RotateArg *) ptarg;
	rotate3d(arg->ima,arg->out,arg->view,arg->xstart,arg->xend);
	return 0;
}
FUNCPTR pt_rotate3d2(void *ptarg) {
	RotateArg *arg=(RotateArg *) ptarg;
	rotate3d2(arg->ima,arg->out,arg->view,arg->xstart,arg->xend);
	return 0;
}


void rotateimage_bspline(float ***ima,float ***out,int view,int xs,int xe){
	int x,y,z;
	float cosv,sinv;
	int xx,yy;
	int i,j;
	float xx0,yy0,xxx0,yyy0;
	float x1,y1;
	__m128 ***image,***outimage;
	__m128 *im[4][4];
	__m128 *optr;
//	__m128 coef1,coef2,coef3,coef4;
	__m128 *out2;
	float cx[4],cy[4];
	__m128 coef[4][4];
//	static float o[256][256]={0};
	out2=(__m128 *)_alloca((z_pixels_simd)*sizeof(__m128));
	if(out2==NULL){
		LOG_EXIT("exiting");
	}
	
	image=(__m128 ***) ima;
	outimage=(__m128 ***) out;
	cosv=cos_psi[view];
	sinv=sin_psi[view];

	xxx0=-x_pixels/2*cosv+y_pixels/2*sinv+x_pixels/2;
	yyy0=-x_pixels/2*sinv-y_pixels/2*cosv+y_pixels/2;

	for(x=xs;x<xe;x++){
		if(cylwiny[x][0]==0) continue;
		xx0=xxx0-cylwiny[x][0]*sinv+x*cosv;
		yy0=yyy0+cylwiny[x][0]*cosv+x*sinv;
		for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
			//		xx0=(x-x_pixels/2)*cosv-(y-y_pixels/2)*sinv+x_pixels/2;
			//		yy0=(x-x_pixels/2)*sinv+(y-y_pixels/2)*cosv+y_pixels/2;
			//			LOG_INFO("%f\t%f\t%f\t%f\n",xt,yt,xx0,yy0);
			xx=(int)(xx0+0.5); 
			yy=(int)(yy0+0.5); 
				x1=xx0-xx;
			y1=yy0-yy;
			cx[3]=x1*x1*x1/6;
			cx[0]=1/6.0+x1*(x1-1)/2-cx[3];
			cx[2]=x1+cx[0]-2*cx[3];
			cx[1]=1-cx[0]-cx[2]-cx[3];

			cy[3]=y1*y1*y1/6;
			cy[0]=1/6.0+y1*(y1-1)/2-cy[3];
			cy[2]=y1+cy[0]-2*cy[3];
			cy[1]=1-cy[0]-cy[2]-cy[3];

			for(i=0;i<4;i++) for(j=0;j<4;j++) coef[i][j]=_mm_set1_ps(cx[i]*cy[j]);
			//			o[xx][yy]++;
			im[0][0]=image[xx-1][yy-1];
			im[0][1]=image[xx-1][yy];
			im[0][2]=image[xx-1][yy+1];
			im[0][3]=image[xx-1][yy+2];
			im[1][0]=image[xx][yy-1];
			im[1][1]=image[xx][yy];
			im[1][2]=image[xx][yy+1];
			im[1][3]=image[xx][yy+2];
			im[2][0]=image[xx+1][yy-1];
			im[2][1]=image[xx+1][yy];
			im[2][2]=image[xx+1][yy+1];
			im[2][3]=image[xx+1][yy+2];
			im[3][0]=image[xx+2][yy-1];
			im[3][1]=image[xx+2][yy];
			im[3][2]=image[xx+2][yy+1];
			im[3][3]=image[xx+2][yy+2];


			optr=outimage[x][y];


			for(z=0;z<z_pixels_simd;z++){
				//				optr[z]=im1[z]*coef1+im2[z]*coef2+im3[z]*coef3+im4[z]*coef4;
				optr[z]=_mm_add_ps(_mm_mul_ps(im[0][0][z],coef[0][0]),
							_mm_add_ps(_mm_mul_ps(im[0][1][z],coef[0][1]),
							_mm_add_ps(_mm_mul_ps(im[0][2][z],coef[0][2]),
							_mm_add_ps(_mm_mul_ps(im[0][3][z],coef[0][3]),

							_mm_add_ps(_mm_mul_ps(im[1][0][z],coef[1][0]),
							_mm_add_ps(_mm_mul_ps(im[1][1][z],coef[1][1]),
							_mm_add_ps(_mm_mul_ps(im[1][2][z],coef[1][2]),
							_mm_add_ps(_mm_mul_ps(im[1][3][z],coef[1][3]),

							_mm_add_ps(_mm_mul_ps(im[2][0][z],coef[2][0]),
							_mm_add_ps(_mm_mul_ps(im[2][1][z],coef[2][1]),
							_mm_add_ps(_mm_mul_ps(im[2][2][z],coef[2][2]),
							_mm_add_ps(_mm_mul_ps(im[2][3][z],coef[2][3]),

							_mm_add_ps(_mm_mul_ps(im[3][0][z],coef[3][0]),
							_mm_add_ps(_mm_mul_ps(im[3][1][z],coef[3][1]),
							_mm_add_ps(_mm_mul_ps(im[3][2][z],coef[3][2]),
							_mm_mul_ps(im[3][3][z],coef[3][3])
							)))))))))))))));								
	//			out2[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(im1[z],coef1),_mm_mul_ps(im2[z],coef2)),
	//				_mm_add_ps(_mm_mul_ps(im3[z],coef3),_mm_mul_ps(im4[z],coef4)));
			}
//			memcpy(optr,out2,z_pixels_simd*sizeof(__m128));
			xx0-=sinv;
			yy0+=cosv;
		}
	}
}
void rotateimage(float ***ima,float ***out,int view,int xs,int xe){
	unsigned int x,y,z;
	float cosv,sinv;
	int xx,yy;
	float xx0,yy0,xxx0,yyy0;
	float x1,y1;
	__m128 ***image,***outimage;
	__m128 *im1,*im2,*im3,*im4;
	__m128 *optr;
	__m128 coef1,coef2,coef3,coef4;
	__m128 *out2;
//	static float o[256][256]={0};
	out2=(__m128 *)_alloca((z_pixels_simd)*sizeof(__m128));
	if(out2==NULL){
		LOG_EXIT("exiting");
	}
	
	image=(__m128 ***) ima;
	outimage=(__m128 ***) out;
	cosv=cos_psi[view];
	sinv=sin_psi[view];

	xxx0=-x_pixels/2*cosv+y_pixels/2*sinv+x_pixels/2;
	yyy0=-x_pixels/2*sinv-y_pixels/2*cosv+y_pixels/2;
	//LOG_INFO("xs xe %d\t%d\n",xs,xe);
	for(x=xs;x<xe;x++){
		if(cylwiny[x][0]==0) continue;
		xx0=xxx0-cylwiny[x][0]*sinv+x*cosv;
		yy0=yyy0+cylwiny[x][0]*cosv+x*sinv;
		for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
			//		xx0=(x-x_pixels/2)*cosv-(y-y_pixels/2)*sinv+x_pixels/2;
			//		yy0=(x-x_pixels/2)*sinv+(y-y_pixels/2)*cosv+y_pixels/2;
			//			LOG_INFO("%f\t%f\t%f\t%f\n",xt,yt,xx0,yy0);
			xx=(int)xx0; 
			yy=(int)yy0; 
			x1=xx0-xx;
			y1=yy0-yy;
//			o[xx][yy]++;
			im1=image[xx][yy];
			im2=image[xx+1][yy];
			im3=image[xx][yy+1];
			im4=image[xx+1][yy+1];
			optr=outimage[x][y];

			coef1=_mm_set1_ps((1-x1)*(1-y1));
			coef2=_mm_set1_ps(x1*(1-y1));
			coef3=_mm_set1_ps(y1*(1-x1));
			coef4=_mm_set1_ps(y1*x1);

			for(z=0;z<z_pixels_simd;z++){
				//				optr[z]=im1[z]*coef1+im2[z]*coef2+im3[z]*coef3+im4[z]*coef4;
				out2[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(im1[z],coef1),_mm_mul_ps(im2[z],coef2)),
					_mm_add_ps(_mm_mul_ps(im3[z],coef3),_mm_mul_ps(im4[z],coef4)));
			}
			memcpy(optr,out2,z_pixels_simd*sizeof(__m128));
			xx0-=sinv;
			yy0+=cosv;
		}
	}
}
int rotate3d(float ***ima,float ***out,int view, int xstart, int xend) {
//	LOG_INFO("rotate3d %d\t%d\n",xstart,xend);
	rotateimage(ima,out,view,xstart,xend);
	return 1;
}

void rotateimage2_bspline(float ***image,float ***outimage,int view,int xs,int xe){
	int x,y,z;
	float cosv,sinv;
	int xx,yy;
	float xx0,yy0,xxx0,yyy0;
	float x1,y1;
	int ystart,yend;
//	__m128 *im1,*im2,*im3,*im4;
	__m128 *optr;
//	__m128 coef1,coef2,coef3,coef4;
	float cx[4],cy[4];
	__m128 coef[4][4];
	__m128 *im[4][4];
	int i,j;

	cosv=cos_psi[view]/zoom;
	sinv=-sin_psi[view]/zoom;

	xxx0=-x_pixels/2*cosv+y_pixels/2*sinv+x_pixels/2;
	yyy0=-x_pixels/2*sinv-y_pixels/2*cosv+y_pixels/2;
	for(x=xs;x<xe;x++){
		ystart=cylwiny[x][0];
		yend=cylwiny[x][1];
		xx0=xxx0-ystart*sinv+x*cosv;
		yy0=yyy0+ystart*cosv+x*sinv;
		for(y=ystart;y<yend;y++){
			xx=(int)xx0;
			yy=(int)yy0;
			if(xx<0 || xx>x_pixels-2){
				xx0-=sinv;
				yy0+=cosv;
				continue;
			}
			if(yy<0 || yy>x_pixels-2){
				xx0-=sinv;
				yy0+=cosv;
				continue;
			}
			xx=(int)(xx0); 
			yy=(int)(yy0); 
			x1=xx0-xx;
			y1=yy0-yy;
			cx[3]=x1*x1*x1/6;
			cx[0]=1/6.0+x1*(x1-1)/2-cx[3];
			cx[2]=x1+cx[0]-2*cx[3];
			cx[1]=1-cx[0]-cx[2]-cx[3];

			cy[3]=y1*y1*y1/6;
			cy[0]=1/6.0+y1*(y1-1)/2-cy[3];
			cy[2]=y1+cy[0]-2*cy[3];
			cy[1]=1-cy[0]-cy[2]-cy[3];

			for(i=0;i<4;i++) for(j=0;j<4;j++) coef[i][j]=_mm_set1_ps(cx[i]*cy[j]);
			im[0][0]=(__m128 *)image[xx-1][yy-1];
			im[0][1]=(__m128 *)image[xx-1][yy];
			im[0][2]=(__m128 *)image[xx-1][yy+1];
			im[0][3]=(__m128 *)image[xx-1][yy+2];
			im[1][0]=(__m128 *)image[xx][yy-1];
			im[1][1]=(__m128 *)image[xx][yy];
			im[1][2]=(__m128 *)image[xx][yy+1];
			im[1][3]=(__m128 *)image[xx][yy+2];
			im[2][0]=(__m128 *)image[xx+1][yy-1];
			im[2][1]=(__m128 *)image[xx+1][yy];
			im[2][2]=(__m128 *)image[xx+1][yy+1];
			im[2][3]=(__m128 *)image[xx+1][yy+2];
			im[3][0]=(__m128 *)image[xx+2][yy-1];
			im[3][1]=(__m128 *)image[xx+2][yy];
			im[3][2]=(__m128 *)image[xx+2][yy+1];
			im[3][3]=(__m128 *)image[xx+2][yy+2];


			optr=(__m128 *) outimage[x][y];


			for(z=0;z<z_pixels_simd;z++){
		//		LOG_INFO("%d\t%d\t%d\t%d\n",view,x,y,z);
				//				optr[z]=im1[z]*coef1+im2[z]*coef2+im3[z]*coef3+im4[z]*coef4;
				optr[z]=_mm_add_ps(optr[z],
							_mm_add_ps(_mm_mul_ps(im[0][0][z],coef[0][0]),
							_mm_add_ps(_mm_mul_ps(im[0][1][z],coef[0][1]),
							_mm_add_ps(_mm_mul_ps(im[0][2][z],coef[0][2]),
							_mm_add_ps(_mm_mul_ps(im[0][3][z],coef[0][3]),

							_mm_add_ps(_mm_mul_ps(im[1][0][z],coef[1][0]),
							_mm_add_ps(_mm_mul_ps(im[1][1][z],coef[1][1]),
							_mm_add_ps(_mm_mul_ps(im[1][2][z],coef[1][2]),
							_mm_add_ps(_mm_mul_ps(im[1][3][z],coef[1][3]),

							_mm_add_ps(_mm_mul_ps(im[2][0][z],coef[2][0]),
							_mm_add_ps(_mm_mul_ps(im[2][1][z],coef[2][1]),
							_mm_add_ps(_mm_mul_ps(im[2][2][z],coef[2][2]),
							_mm_add_ps(_mm_mul_ps(im[2][3][z],coef[2][3]),

							_mm_add_ps(_mm_mul_ps(im[3][0][z],coef[3][0]),
							_mm_add_ps(_mm_mul_ps(im[3][1][z],coef[3][1]),
							_mm_add_ps(_mm_mul_ps(im[3][2][z],coef[3][2]),
							_mm_mul_ps(im[3][3][z],coef[3][3])
							))))))))))))))));								

				//			out2[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(im1[z],coef1),_mm_mul_ps(im2[z],coef2)),
	//				_mm_add_ps(_mm_mul_ps(im3[z],coef3),_mm_mul_ps(im4[z],coef4)));
			}
			xx0-=sinv;
			yy0+=cosv;
		}
	}
}
void rotateimage2(float ***ima,float ***out,int view,int xs,int xe){
	unsigned int x,y,z;
	float cosv,sinv;
	int xx,yy;
	float xx0,yy0,xxx0,yyy0;
	float x1,y1;
	int ystart,yend;
	__m128 *im1,*im2,*im3,*im4;
	__m128 *optr;
	__m128 coef1,coef2,coef3,coef4;

	cosv=cos_psi[view]/zoom;
	sinv=-sin_psi[view]/zoom;

	xxx0=-x_pixels/2*cosv+y_pixels/2*sinv+x_pixels/2;
	yyy0=-x_pixels/2*sinv-y_pixels/2*cosv+y_pixels/2;
	for(x=xs;x<xe;x++){
		ystart=cylwiny[x][0];
		yend=cylwiny[x][1];
		xx0=xxx0-ystart*sinv+x*cosv;
		yy0=yyy0+ystart*cosv+x*sinv;
		for(y=ystart;y<yend;y++){
			xx=(int)xx0;
			yy=(int)yy0;
			if(xx<0 || xx>x_pixels-2 ||yy<0 || yy>x_pixels-2){
				xx0-=sinv;
				yy0+=cosv;
				continue;
			}
			x1=xx0-xx; /* locate at left */
			y1=yy0-yy; /* locate at bottom */
			im1=(__m128 *)ima[xx][yy];
			im2=(__m128 *)ima[xx+1][yy];
			im3=(__m128 *)ima[xx][yy+1];
			im4=(__m128 *)ima[xx+1][yy+1];
			optr=(__m128 *)out[x][y];
			coef1=_mm_set1_ps((1-x1)*(1-y1));
			coef2=_mm_set1_ps(x1*(1-y1));
			coef3=_mm_set1_ps(y1*(1-x1));
			coef4=_mm_set1_ps(y1*x1);

			for(z=0;z<z_pixels_simd;z++){
				optr[z]=_mm_add_ps(optr[z],_mm_add_ps(_mm_add_ps(_mm_mul_ps(im1[z],coef1),_mm_mul_ps(im2[z],coef2)),
					_mm_add_ps(_mm_mul_ps(im3[z],coef3),_mm_mul_ps(im4[z],coef4))));
			}
			xx0-=sinv;
			yy0+=cosv;
		}
	}
}

int rotate3d2(float ***ima,float ***out,int view, int xstart, int xend) {
	rotateimage2(ima,out,view,xstart,xend);
	return 1;
}

void zoom_proj_2view(float **prj,float zoom)
{
	int i,k;
	float zoomf;
	float *tmpf;
	float xr;
	float xrz;
	float r;
	float *data;
	int xri,xrzi;
	float *normal;

	tmpf=(float *)_alloca(xr_pixels*sizeof(float));
	normal=(float *) alloca(xr_pixels*sizeof(float));


	if(zoom<1){
		zoomf=zoom;

		for(i=0;i<segmentsize*2;i++){
			xr=xrz=0;
			memset(tmpf,0,xr_pixels*sizeof(float));
			data=prj[i];
			xrz=-xr_pixels/2*zoomf+xr_pixels/2;
			for(xri=0;xri<xr_pixels;xri++){
				xrzi=xrz;
				r=xrz-xrzi;
				if(xrzi<xr_pixels-1 && xrzi>=0) tmpf[xri]=data[xrzi]+r*(data[xrzi+1]-data[xrzi]);
				else tmpf[xri]=0;
				xrz+=zoomf;
			}
			memcpy(data,tmpf,xr_pixels*sizeof(float));
		}
	} else {

		zoomf=1/zoom;
		for(k=0;k<xr_pixels;k++) normal[k]=0;
		xrz=-xr_pixels/2*zoomf+xr_pixels/2;
		for(xri=0;xri<xr_pixels;xri++){
			xrzi=xrz;
			r=xrz-xrzi;
			if(xrzi<xr_pixels-2 && xrzi>=0){
				normal[xrzi]+=(1-r);
				normal[xrzi+1]+=r;
			}
			xrz+=zoomf;
		}
		for(k=0;k<xr_pixels;k++) if(normal[k]!=0) normal[k]=1/normal[k];


		for(i=0;i<segmentsize*2;i++){
			xr=xrz=0;
			memset(tmpf,0,xr_pixels*sizeof(float));
			data=prj[i];
			xrz=-xr_pixels/2*zoomf+xr_pixels/2;
			for(xri=0;xri<xr_pixels;xri++){
				xrzi=xrz;
				r=xrz-xrzi;
				if(xrzi<xr_pixels-2 && xrzi>=0){
					tmpf[xrzi]+=data[xri]*(1-r);
					tmpf[xrzi+1]+=data[xri]*r;
				}
				xrz+=zoomf;
			}
			for(k=0;k<xr_pixels;k++) data[k]=tmpf[k]*normal[k];
		}
		//		free(normal);
	}
	//	free(tmpf);
}


/*******************************************************************************************************/
/*
*	forward_proj3d2(ima, prj, view, verbose):
*      forward-project from image "ima" to projection "prj" at angle "view" and "view+90".
*      In specific view, this method process in all theta.(0,+1,-1,....+7,-7) 
*/
int forward_proj3d2(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf)
{
	THREAD_TYPE threads[MAXTHREADS];
	BPFP_ptargs args[MAXTHREADS];

	int thr;
	int xstart,xend;
	long int Clock1,Clock2,Clock3,Clock4,Clock5;
	int z;
	int k;
	int xr,yr;
	float *ptr;
	int y1,y2,yr2;
	int xri;
	unsigned int threadID;
	Clock5=clock();
	Clock3=Clock4=0;

	xstart=0;
	for (thr = 0; thr < nthreads; thr++) {
		if(thr!=nthreads-1) xend=xstart+start_x__per_thread_back[thr+1];
		else xend=x_pixels/2+1;
		args[thr].imagebuf = (float ***)NULL;
		args[thr].image = (float ***)ima;
		args[thr].prj = (float ***)prjbuf;

		args[thr].verbose = numthread;
		args[thr].start = xstart;
		args[thr].end = xend;
		args[thr].view = view;
		args[thr].pthread_id = thr;
		xstart+=start_x__per_thread_back[thr+1];
	}

	Clock2=clock();
	Clock3=(Clock2-Clock5);

	for (thr = 0; thr < nthreads; thr++) {		
		START_THREAD(threads[thr],pt_fproj_3d_view1,args[thr],threadID);
	} // All threads started...now we wait for them to complete

	for (thr = 0; thr < nthreads; thr++) {
		Wait_thread(threads[thr]);
	}

	Clock1=clock()-Clock2;

	/*----------------------------------------------------------------	 
	* set return projection array ( in main, this array is estimate2 )	 
	*/
	Clock2=clock();
	for(xr=1;xr<=xr_pixels/2;xr++){
		if(cylwiny[xr][0]==0) continue;
		xri=xr_pixels-xr;
		ptr=prjbuf[xr][0];
		ptr=prjbuf[xr][0];
		for(z=0,yr=0,yr2=segmentsize;yr<yr_pixels;yr++,z+=4,yr2++){
			prj[yr][xr] =ptr[z];
			prj[yr][xri] =ptr[z+1];
			prj[yr2][xr]=ptr[z+2]; 		
			prj[yr2][xri]=ptr[z+3]; 		
		}		
	}

	for(k=1;k<=th_pixels/2;k++) {
		for(xr=0;xr<=xr_pixels/2;xr++) {
			if(cylwiny[xr][0]==0) continue;
			z=0;
			xri=xr_pixels-xr;
			z=yr_bottom[k*2]*4;
			ptr = prjbuf[xr][k];
			y1=segment_offset[k*2];               
			y2=segment_offset[k*2]+segmentsize;   
			for(yr=yr_bottom[k*2];yr<=yr_top[k*2];yr++,z+=4,y1++,y2++){
				prj[y1][xr]=ptr[z];     //	positive theta,view    
				prj[y1][xri]=ptr[z+1];   //	positive theta,view    
				prj[y2][xr]=ptr[z+2];   // positive theta,view+90 
				prj[y2][xri]=ptr[z+3];   // positive theta,viewi+90 
			}
			z=yr_bottom[k*2]*4;
			y1=segment_offset[k*2-1];             
			y2=segment_offset[k*2-1]+segmentsize;
			ptr = prjbuf[xr_pixels/2+xr][k];

			for(yr=yr_bottom[k*2];yr<=yr_top[k*2];yr++,z+=4,y1++,y2++){
				prj[y1][xr]=ptr[z];     // negative theta,view    
				prj[y1][xri]=ptr[z+1];  // negative theta,view    
				prj[y2][xr]=ptr[z+2];   // negative theta,view+90 
				prj[y2][xri]=ptr[z+3];   // negativetheta,viewi+90 
			}
		}
	}
	if(zoom!=1){
		zoom_proj_2view(prj,zoom);
	}

	Clock4=(clock()-Clock2);
//	LOG_INFO("%d\t%3.2f\trotation time: %3.2f,\tprojection time:\t%3.2f sec\n",view,(0.0+Clock4)/CLOCKS_PER_SEC,(0.0+Clock3)/CLOCKS_PER_SEC,(0.0+Clock1)/CLOCKS_PER_SEC );

	return 1;
} /* end of forward_proj3d2  */

int forward_proj3d_thread1(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf)
{
	long int Clock1,Clock2,Clock3,Clock4,Clock5;
	int z;
	int k;
	int xr,yr;
	float *ptr;
	int y1,y2,yr2;
	int xri;
	Clock5=clock();
//	LOG_INFO("start rotating\n");
//	rotate3d(ima,imagebuf,view,0,x_pixels);

	Clock2=clock();
	Clock3=(Clock2-Clock5);
//	LOG_INFO("start forward\n");
//	forward_proj3d_view1_thread(imagebuf,prjbuf,view,0,0,0,x_pixels/2+1);
	forward_proj3d_view1_thread(ima,prjbuf,view,0,0,0,x_pixels/2+1,imagebuf);

	/*----------------------------------------------------------------	 
	* set return projection array ( in main, this array is estimate2 )	 
	*/
	Clock1=clock()-Clock2;
	Clock2=clock();
	for(xr=1;xr<=xr_pixels/2;xr++){
		if(cylwiny[xr][0]==0) continue;
		xri=xr_pixels-xr;
		ptr=prjbuf[xr][0];
		ptr=prjbuf[xr][0];
		for(z=0,yr=0,yr2=segmentsize;yr<yr_pixels;yr++,z+=4,yr2++){
			prj[yr][xr] =ptr[z];
			prj[yr][xri] =ptr[z+1];
			prj[yr2][xr]=ptr[z+2]; 		
			prj[yr2][xri]=ptr[z+3]; 		
		}		
	}
	for(k=1;k<=th_pixels/2;k++) {
		for(xr=0;xr<=xr_pixels/2;xr++) {
			if(cylwiny[xr][0]==0) continue;
			z=0;
			xri=xr_pixels-xr;
			z=yr_bottom[k*2]*4;
			ptr = prjbuf[xr][k];
			y1=segment_offset[k*2];               
			y2=segment_offset[k*2]+segmentsize;   
			for(yr=yr_bottom[k*2];yr<=yr_top[k*2];yr++,z+=4,y1++,y2++){
				prj[y1][xr]=ptr[z];     //	negative theta,view    
				prj[y1][xri]=ptr[z+1];   //	positive theta,view    
				prj[y2][xr]=ptr[z+2];   // negative theta,view+90 
				prj[y2][xri]=ptr[z+3];   // positive theta,viewi+90 
			}
			z=yr_bottom[k*2]*4;
			y1=segment_offset[k*2-1];             
			y2=segment_offset[k*2-1]+segmentsize;
			ptr = prjbuf[xr_pixels/2+xr][k];

			for(yr=yr_bottom[k*2];yr<=yr_top[k*2];yr++,z+=4,y1++,y2++){
				prj[y1][xr]=ptr[z];     // negative theta,view    
				prj[y1][xri]=ptr[z+1];  // positive theta,view    
				prj[y2][xr]=ptr[z+2];   // negative theta,view+90 
				prj[y2][xri]=ptr[z+3];   // positive theta,viewi+90 
			}
		}
	}

	if(zoom!=1){
		zoom_proj_2view(prj,zoom);
	}

	Clock4=(clock()-Clock5);
//	LOG_INFO("%d\t%3.5f\trotation time: %3.5f,\tprojection time:\t%3.5f sec\n",view,(0.0+Clock4)/CLOCKS_PER_SEC,(0.0+Clock3)/CLOCKS_PER_SEC,(0.0+Clock1)/CLOCKS_PER_SEC );

	return 1;
} /* end of forward_proj3d2  */

/* 
 Mu-Map forward projection to create attenuation map during reconstruction
*/
int proj_atten_view(float ***image, float *prj,int view, int verbose)
{
  LOG_INFO( "proj_atten_view:TBD\n");
  LOG_EXIT("exiting");
}

int back_proj3d2(float **prj,float *** ima,int view,int numthread,float ***imagebuf,float ***prjbuf)
{
	int yr, yr2,xr;
	int y1,y2,y3,y4;
	float rescale;
	THREAD_TYPE threads[MAXTHREADS];
	BPFP_ptargs args[MAXTHREADS];
	int xstart,xend;
	int thr;
	RotateArg roarg[MAXTHREADS];
	long int Clock1,Clock2,Clock3,Clock4,Clock5;
	int k;
	float *ptr;
	__m128 *mptr;
	__m128 *mptr2;
	__m128 coef1;
	unsigned threadID;	
	clock_t cswap1,cswap2;
	int tmp = 0;

	Clock5=clock();
	Clock3=Clock4=0;
	//	LOG_INFO("viewback=%d\n",view);
	xstart=0;
	for (thr = 0; thr < nthreads; thr++) {
		if(thr!=nthreads-1) xend=xstart+start_x__per_thread_back[thr+1];
		else xend=x_pixels/2+1;
		args[thr].image = (float ***)imagebuf;
		args[thr].prj = (float ***)prjbuf;
		args[thr].verbose = numthread;
		args[thr].start = xstart;
		args[thr].end = xend;
		args[thr].view = view;
		args[thr].view90 = 0;
		args[thr].pthread_id = thr;
		xstart+=start_x__per_thread_back[thr+1];
	}

	/* rescale image values */
	for(k=0,yr2=0;k<th_pixels;k++){
		rescale = cos_theta[k] * cos_theta[k] * cos_theta[k] * span * ring_spacing / (2 * ring_radius);
		coef1   =_mm_set_ps1(rescale);
		for(yr=yr_bottom[k];yr<=yr_top[k];yr++,yr2++){
			mptr = (__m128 *)prj[yr2];
			mptr2= (__m128 *)prj[yr2+segmentsize];
			for(xr=0;xr<xr_pixels/4;xr++){
				mptr[xr] =_mm_mul_ps(mptr [xr],coef1);
				mptr2[xr]=_mm_mul_ps(mptr2[xr],coef1);
			}
		}
	}
//memset(&prjbuf[0][0][0],0,(xr_pixels+1)*(th_pixels/2+1)*yr_pixels*16);
	/* set prjxyf from pri */
	cswap1=clock();
	for(xr=1;xr<=xr_pixels/2;xr++){
		tmp=0;
		ptr=prjbuf[xr][0];
		for(yr=0;yr<yr_pixels;yr++,tmp+=4){
			ptr[tmp]  =prj[yr][xr]; /* view    */
			ptr[tmp+2]=prj[yr][xr_pixels-xr]; /* view+90 */
			ptr[tmp+1]=ptr[tmp+3]=0;
		}
	}
	for(k=1;k<=th_pixels/2;k++){
		for(xr=1;xr<=xr_pixels/2;xr++){
			tmp=yr_bottom[k*2]*4;
			ptr=prjbuf[xr][k];
			y1=segment_offset[k*2];
			y2=segment_offset[k*2-1];
			y3=segment_offset[k*2]  +segmentsize;
			y4=segment_offset[k*2-1]+segmentsize;

			for(yr=yr_bottom[k*2];yr<=yr_top[k*2];yr++,tmp+=4,y1++,y2++){
				ptr[tmp]  =prj[y1][xr]; /* negative theta,view    */
				ptr[tmp+1]=prj[y2][xr]; /* positive theta,view    */
				ptr[tmp+2]=prj[y1][xr_pixels-xr]; /* negative theta,view+90 */
				ptr[tmp+3]=prj[y2][xr_pixels-xr]; /* positive theta,view+90 */
			}
//			tmp=yr_bottom[k*2]*4-4;
//				ptr[tmp]=ptr[tmp+1]=ptr[tmp+2]=ptr[tmp+3]=0;
//			tmp=yr_top[k*2]*4+4;
//				ptr[tmp]=ptr[tmp+1]=ptr[tmp+2]=ptr[tmp+3]=0;
		}
	}

	cswap2=clock();
	Clock1=clock();

	/* back projection */
	//	memset(&imagexyzf[0][0][0],0,x_pixels*y_pixels*z_pixels_simd*sizeof(__m128));//(z_pixels+1)*4);
	for (thr = 0; thr < nthreads; thr++) {
		START_THREAD(threads[thr],pt_bkproj_3d_view_n1,args[thr],threadID);
	}			

	for (thr = 0; thr < nthreads; thr++) {
		Wait_thread(threads[thr]);
	}
//memset(&prjbuf[0][0][0],0,(xr_pixels+1)*(th_pixels/2+1)*yr_pixels*16);

	for(xr=1;xr<=xr_pixels/2;xr++){
		tmp=0;
		ptr=prjbuf[xr][0];
		for(yr=segmentsize;yr<yr_pixels+segmentsize;yr++,tmp+=4){
			ptr[tmp]  =prj[yr][xr]; /* view    */
			ptr[tmp+2]=prj[yr][xr_pixels-xr]; /* view+90 */
			ptr[tmp+1]=ptr[tmp+3]=0;
		}
	}
	for(k=1;k<=th_pixels/2;k++){
		for(xr=1;xr<=xr_pixels/2;xr++){
			tmp=yr_bottom[k*2]*4;
			ptr=prjbuf[xr][k];
			y1=segment_offset[k*2]  +segmentsize;
			y2=segment_offset[k*2-1]+segmentsize;
			for(yr=yr_bottom[k*2];yr<=yr_top[k*2];yr++,tmp+=4,y1++,y2++){
				ptr[tmp]  =prj[y1][xr]; /* negative theta,view    */
				ptr[tmp+1]=prj[y2][xr]; /* positive theta,view    */
				ptr[tmp+2]=prj[y1][xr_pixels-xr]; /* negative theta,view+90 */
				ptr[tmp+3]=prj[y2][xr_pixels-xr]; /* positive theta,view+90 */
			}
//						tmp=yr_bottom[k*2]*4-4;
//				ptr[tmp]=ptr[tmp+1]=ptr[tmp+2]=ptr[tmp+3]=0;
//			tmp=yr_top[k*2]*4+4;
//				ptr[tmp]=ptr[tmp+1]=ptr[tmp+2]=ptr[tmp+3]=0;

		}
	}

	for (thr = 0; thr < nthreads; thr++) {
		args[thr].view90=1;
		START_THREAD(threads[thr],pt_bkproj_3d_view_n1,args[thr],threadID);
	}			// All threads started...now we wait for them to complete

	for (thr = 0; thr < nthreads; thr++) {
		Wait_thread(threads[thr]);
	}

	Clock2=clock();
	Clock3=(Clock2-Clock1);

	/*rotate img  */	
	xstart=0;
	for (thr = 0; thr < nthreads; thr++) {
		if(thr!=nthreads-1) xend=xstart+start_x__per_thread[thr+1];
		else xend=x_pixels;
		roarg[thr].view  = view;
		roarg[thr].xstart= xstart;
		roarg[thr].xend	 = xend;
		roarg[thr].ima   = (float ***)imagebuf;
		roarg[thr].out   = (float ***)ima;
		xstart+=start_x__per_thread[thr+1];
	//	LOG_INFO("roarg[%d] %d\t%d\n",thr,roarg[thr].xstart,roarg[thr].xend);
		START_THREAD(threads[thr],pt_rotate3d2,roarg[thr],threadID);
	} // All threads started...now we wait for them to complete

	for (thr = 0; thr < nthreads; thr++) {
		Wait_thread(threads[thr]);
	}
	Clock4=(clock()-Clock2);
	//	LOG_INFO("%d\t%3.2f\trotation time: %3.2f,\tbackprojection time:\t%3.2f sec\n",view,(0.0+clock()-Clock5)/CLOCKS_PER_SEC,(0.0+Clock4)/CLOCKS_PER_SEC,(0.0+Clock3)/CLOCKS_PER_SEC );
	//	LOG_INFO("%d\t%3.2f\trotation time: %3.2f,\tbackprojection time:\t%3.2f sec\n",view,(0.0+cswap2-cswap1)/CLOCKS_PER_SEC,(0.0+Clock4)/CLOCKS_PER_SEC,(0.0+Clock3)/CLOCKS_PER_SEC );
	//	LOG_INFO("%d\t%3.2f\trotation time: %3.2f,\tbackprojection time:\t%3.2f sec\n",view,(0.0+cswap2-cswap1)/CLOCKS_PER_SEC,(0.0+Clock1-Clock5)/CLOCKS_PER_SEC,(0.0+Clock3)/CLOCKS_PER_SEC );
	return 1;
}

int back_proj3d_thread1(float **prj,float *** ima,int view,int numthread,float ***imagebuf,float ***prjbuf)
{
	int yr, yr2,xr,xr2;
	int y1,y2,y3,y4;
	float rescale;
	long int Clock5,Clock3,Clock1;
	int k;
	float *ptr,*ptr2;
	__m128 *mptr;
	__m128 *mptr2;
	__m128 coef1;
	int tmp = 0;

	Clock5=clock();

	/* rescale image values */
	for(k=0,yr2=0;k<th_pixels;k++){
		rescale = cos_theta[k] * cos_theta[k] * cos_theta[k] * span * ring_spacing / (2 * ring_radius);
		coef1   =_mm_set_ps1(rescale);
		for(yr=yr_bottom[k];yr<=yr_top[k];yr++,yr2++){
			mptr = (__m128 *)prj[yr2];
			mptr2= (__m128 *)prj[yr2+segmentsize];
			for(xr=0;xr<xr_pixels/4;xr++){
				mptr[xr] =_mm_mul_ps(mptr [xr],coef1);
				mptr2[xr]=_mm_mul_ps(mptr2[xr],coef1);
			}
		}
	}
//	memset(&prjbuf[0][0][0],0,(xr_pixels+1)*(th_pixels/2+1)*yr_pixels*sizeof(__m128));

//memset(&prjbuf[0][0][0],0,(xr_pixels+1)*(th_pixels/2+1)*yr_pixels*16);
	// set prjxyf from pri 
	for(xr=1;xr<=xr_pixels/2;xr++){
		tmp=0;
		ptr=prjbuf[xr][0];
		y1=0;
		y2=segmentsize;
		xr2=xr_pixels-xr;
		for(y1=0;y1<yr_pixels;y1++,y2++,tmp+=4){
			ptr[tmp]    =prj[y1][xr]; // view    
			ptr[tmp+1]  =prj[y1][xr2]; // view    
			ptr[tmp+2]  =prj[y2][xr]; // view +90 
			ptr[tmp+3]	=prj[y2][xr2]; // view+90 
		}
	}
	for(k=1;k<=th_pixels/2;k++){
		for(xr=1;xr<=xr_pixels/2;xr++){
			tmp=yr_bottom[k*2]*4;
			ptr=	prjbuf[xr][k];
			ptr2=	prjbuf[xr+xr_pixels/2][k];
			y1=segment_offset[k*2];
			y2=segment_offset[k*2-1];
			y3=segment_offset[k*2]  +segmentsize;
			y4=segment_offset[k*2-1]+segmentsize;
			xr2=xr_pixels-xr;
			for(yr=yr_bottom[k*2];yr<=yr_top[k*2];yr++,tmp+=4,y1++,y2++,y3++,y4++){
				ptr[tmp]  =prj[y1][xr]; // negative theta,view    
				ptr[tmp+1]=prj[y1][xr2]; // positive theta,view    
				ptr[tmp+2]=prj[y3][xr]; // negative theta,view+90 
				ptr[tmp+3]=prj[y3][xr2]; // positive theta,view+90 
				ptr2[tmp]  =prj[y2][xr]; // negative theta,view    
				ptr2[tmp+1]=prj[y2][xr2]; // positive theta,view   
				ptr2[tmp+2]=prj[y4][xr]; // negative theta,view+90 
				ptr2[tmp+3]=prj[y4][xr2]; // positive theta,view+90 
			}
			tmp=yr_bottom[k*2]*4-4;
				ptr[tmp]=ptr[tmp+1]=ptr[tmp+2]=ptr[tmp+3]=ptr2[tmp]=ptr2[tmp+1]=ptr2[tmp+2]=ptr2[tmp+3]=0;
			tmp=yr_top[k*2]*4+4;
				ptr[tmp]=ptr[tmp+1]=ptr[tmp+2]=ptr[tmp+3]=ptr2[tmp]=ptr2[tmp+1]=ptr2[tmp+2]=ptr2[tmp+3]=0;

		}
	}
	Clock1=clock()-Clock5;
	back_proj3d_view_n1thread(imagebuf,prjbuf,0,view,0,0,x_pixels/2+1);
	Clock3=clock();
	//	LOG_INFO("%d\t%3.2f\t%3.2f  \n",view,(0.0+Clock3-Clock5)/CLOCKS_PER_SEC,(0.0+Clock1)/CLOCKS_PER_SEC);
	return 1;
}

int	rotate_image_buf(float ****imagebuf,float ***ima,int numthread,int *view)
{
	THREAD_TYPE threads[MAXTHREADS];
	int xstart,xend;
	int thr;
	RotateArg roarg[MAXTHREADS];
	unsigned threadID;	
	int i;
	for(i=0;i<numthread;i++){
		xstart=0;
		for (thr = 0; thr < numthread; thr++) {
			if(thr!=numthread-1) xend=xstart+start_x__per_thread[thr+1];
			else xend=x_pixels;
			roarg[thr].view  = view[i];
			roarg[thr].xstart= xstart;
			roarg[thr].xend	 = xend;
			roarg[thr].ima   = (float ***)imagebuf[i];
			roarg[thr].out   = (float ***)ima;
			xstart+=start_x__per_thread[thr+1];
			START_THREAD(threads[thr],pt_rotate3d2,roarg[thr],threadID);
		} // All threads started...now we wait for them to complete
		for (thr = 0; thr < numthread; thr++) Wait_thread(threads[thr]);
	}
	return 1;
}


int forward_proj3d_view1_thread(float ***image,float ***prj,int view,int theta2,int verbose,
								int xr_start,int xr_end,float ***imagebuf)
{

	int x, y,i,z,yr,yri; /* integer voxel indices               */
	int z2;      /* integer indices for 2D-projections  */
	float zz0;   /* float voxel position in pixel corresponding to the first 
				 one in a sinogram, i.e. xr=0  */
	Forwardprojection_lookup lptr[500];
	Forwardprojection_lookup lptr2[500];
	__m128 **pp1,**pp2;
	__m128 ***image_ptr;
	float *img1,*img2,*img3,*img4;
	float *img5,*img6,*img7,*img8;
	__m128 coef1,coef2;
	int xx,yy;
	float xx0,yy0,x1,y1,cosv,sinv;
	__m128 *rim1;
	__m128 *rim2;
	__m128 *rim3;
	__m128 *rim4;
	__m128 *rim5;
	__m128 *rim6;
	__m128 *rim7;
	__m128 *rim8;
	__m128 *rim;
	__m128 *r1,*r2,*r3,*r4;
	//	__m128 rc1[8],rc2[8],rc3[8],rc4[8];
	__m128 rc1[8];//,rc2[8],rc3[8],rc4[8];
	int    th;
	int    th2;
	__m128 *p1m,*p2m;
	__m128 *pam;
	__m128 *pam2;
	float  *mptr;
	float  *mptr2;
	float *p1mf,*p2mf;
	int img_center=x_pixels/2;

	image_ptr=(__m128 ***) image;
	pam=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	pam2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	rim1=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim2=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim3=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim4=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim5=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim6=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim7=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim8=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	memset(rim1,0,z_pixels_simd*sizeof(__m128));
	memset(rim2,0,z_pixels_simd*sizeof(__m128));
	memset(rim3,0,z_pixels_simd*sizeof(__m128));
	memset(rim4,0,z_pixels_simd*sizeof(__m128));
	memset(rim5,0,z_pixels_simd*sizeof(__m128));
	memset(rim6,0,z_pixels_simd*sizeof(__m128));
	memset(rim7,0,z_pixels_simd*sizeof(__m128));
	memset(rim8,0,z_pixels_simd*sizeof(__m128));
	if(pam==NULL || pam2==NULL){
		LOG_EXIT("exiting");
	}

	mptr=(float *)pam;
	mptr2=(float *)pam2;
	cosv=cos_psi[view];
	sinv=sin_psi[view];
	img1=(float *) rim1;
	img2=(float *) rim2;
	img3=(float *) rim3;
	img4=(float *) rim4;
	img5=(float *) rim5;
	img6=(float *) rim6;
	img7=(float *) rim7;
	img8=(float *) rim8;

	for(x=xr_start;x<xr_end;x++){

		if(x==0) continue;
		pp1=(__m128 **)prj[x];
		pp2=(__m128 **)prj[xr_pixels/2+x];

		if(cylwiny[x][0]==cylwiny[x][1]) continue;

		if(norm_zero_mask[view][x]==0){
						continue;
		}

		for(i=0;i<4;i++){
			if(i==0){	xx=x;yy=img_center; rim=rim1;}
			else if(i==1){		xx=x_pixels-x;yy=img_center;rim=rim2;}
			else if(i==2){			xx=img_center;yy=x;rim=rim3;}
			else {			xx=img_center;yy=x_pixels-x;rim=rim4;}

			xx0=(xx-img_center)*cosv*zoom-(yy-img_center)*sinv+img_center;		yy0=(xx-img_center)*sinv*zoom+(yy-img_center)*cosv+img_center;
			if(xx0<0 || xx0>x_pixels-2 ||yy0<0 || xx0>x_pixels-2) continue;
			xx=(int) xx0; yy=(int) yy0;		x1=xx0-xx;y1=yy0-yy;
			r1=image_ptr[xx][yy];		r2=image_ptr[xx+1][yy];		r3=image_ptr[xx][yy+1];		r4=image_ptr[xx+1][yy+1];
			rc1[0]=_mm_set1_ps((1-x1)*(1-y1));		rc1[1]=_mm_set1_ps(x1*(1-y1));		rc1[2]=_mm_set1_ps(y1*(1-x1));		rc1[3]=_mm_set1_ps(y1*x1);
			for(z=0;z<z_pixels_simd;z++){
				rim[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(r1[z],rc1[0]),_mm_mul_ps(r2[z],rc1[1])),_mm_add_ps(_mm_mul_ps(r3[z],rc1[2]),_mm_mul_ps(r4[z],rc1[3])));
			}
		}

		p1mf=(float *)pp1[0];
		for(z=0,yr=0;z<z_pixels;z++,yr+=4){
			p1mf[yr]=img1[z];
			p1mf[yr+1]=img2[z];
			p1mf[yr+2]=img3[z];
			p1mf[yr+3]=img4[z];
		}
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			p1m=(__m128 *)pp1[th/2];
			p1mf=(float *)p1m;
			p2m=(__m128 *)pp2[th/2];
			p2mf=(float *)p2m;
			for(yr=yr_bottom[th]*4,z=yr_bottom[th];yr<=yr_top[th]*4;yr+=4,z++){
				p1mf[yr]=img1[z];
				p1mf[yr+1]=img2[z];
				p1mf[yr+2]=img3[z];
				p1mf[yr+3]=img4[z];
				p2mf[yr]=img1[z];
				p2mf[yr+1]=img2[z];
				p2mf[yr+2]=img3[z];
				p2mf[yr+3]=img4[z];
			}
		}
		for(y=cylwiny[x][0];y<img_center;y++){

				for(i=0;i<8;i++){
					if(i==0){	xx=x;yy=y;rim=rim1; }								//+theta,view,	+xr
					else if(i==1){		xx=x_pixels-x;yy=y;rim=rim2;}				//+theta,view,	-xr
					else if(i==2){			xx=y_pixels-y;yy=x;rim=rim3;}			//+theta,view+90,+xr
					else if(i==3){			xx=y_pixels-y;yy=x_pixels-x;rim=rim4;}	//+theta,view+90,-xr
					else if(i==4){			xx=x;yy=y_pixels-y;rim=rim5;}			//-theta,view,	+xr
					else if(i==5){			xx=x_pixels-x;yy=y_pixels-y;rim=rim6;}	//-theta,view,	-xr
					else if(i==6){			xx=y;yy=x;rim=rim7;}						//-theta,view+90,	+xr
					else if(i==7){			xx=y;yy=x_pixels-x;rim=rim8;}			//-theta,view+90,	-xr
					xx0=(xx-img_center)*cosv-(yy-img_center)*sinv+img_center;		yy0=(xx-img_center)*sinv+(yy-img_center)*cosv+img_center;
					if(xx0<0 || xx0>x_pixels-2 ||yy0<0 || xx0>x_pixels-2){
						memset(rim,0,z_pixels_simd*sizeof(__m128));
						continue;
					}
					xx=(int) xx0; yy=(int) yy0;		x1=xx0-xx;y1=yy0-yy;

					r1=image_ptr[xx][yy];		r2=image_ptr[xx+1][yy];		r3=image_ptr[xx][yy+1];		r4=image_ptr[xx+1][yy+1];
					rc1[0]=_mm_set1_ps((1-x1)*(1-y1));		rc1[1]=_mm_set1_ps(x1*(1-y1));		rc1[2]=_mm_set1_ps(y1*(1-x1));		rc1[3]=_mm_set1_ps(y1*x1);
			//			for(z=0;z<z_pixels_simd*4;z++){
			//				rim[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(r1[z],rc1[0]),_mm_mul_ps(r2[z],rc1[1])),_mm_add_ps(_mm_mul_ps(r3[z],rc1[2]),_mm_mul_ps(r4[z],rc1[3])));
			//			}
					if(x1!=0 && y1!=0){
						for(z=0;z<z_pixels_simd;z++){
							rim[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(r1[z],rc1[0]),_mm_mul_ps(r2[z],rc1[1])),_mm_add_ps(_mm_mul_ps(r3[z],rc1[2]),_mm_mul_ps(r4[z],rc1[3])));
						}
					} 
					else if(x1==0 && y1!=0){
						for(z=0;z<z_pixels_simd;z++){
							rim[z]=_mm_add_ps(_mm_mul_ps(r1[z],rc1[0]),_mm_mul_ps(r3[z],rc1[2]));
						}
					} else if(x1!=0 && y1==0){
						for(z=0;z<z_pixels_simd;z++){
							rim[z]=_mm_add_ps(_mm_mul_ps(r1[z],rc1[0]),_mm_mul_ps(r2[z],rc1[1]));
						}
					} else {
						memcpy(rim,r1,z_pixels_simd*sizeof(__m128));
					}
				}


			for(z=0,yr=0;z<z_pixels;z++,yr+=4){
				mptr[yr]=img1[z];	
				mptr[yr+1]=img2[z];	
				mptr[yr+2]=img3[z];	
				mptr[yr+3]=img4[z];	
				mptr2[yr]=	img5[z];	
				mptr2[yr+1]=img6[z];	
				mptr2[yr+2]=img7[z];	
				mptr2[yr+3]=img8[z];	
			}
			/* theta = 0 */
			p1m=pp1[0];
			for(z=0;z<z_pixels;z++){
				p1m[z]=_mm_add_ps(p1m[z],_mm_add_ps(pam[z],pam2[z]));
			}
			/* theta = +1,-1, ..... */
			//			lptr=flookup[y];
			memcpy(lptr,flookup[y],groupmax*2*ZShift*sizeof(Forwardprojection_lookup));
			memcpy(lptr2,flookup[y_pixels-y],groupmax*2*ZShift*sizeof(Forwardprojection_lookup));
			yri=0;
			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
				yri=th2*ZShift*2;
				p1m=pp1[th/2];
				p2m=pp2[th/2];
				if(lptr[yri].zzi<0) continue;
				if(lptr2[yri+ZShift*2-1].zzi>z_pixels-1) continue;
				for(yr=yr_bottom[th];yr<yr_bottom[th]+ZShift;yr++,yri++){
					z=lptr[yri].zzi;
					z2=lptr2[yri].zzi;
					p1m[yr]=_mm_add_ps(p1m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam2[z2+1],pam[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam[z+1],pam2[z2]),_mm_set_ps1(lptr[yri].zzz0))));
					p2m[yr]=_mm_add_ps(p2m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam[z2+1],pam2[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam2[z+1],pam[z2]),_mm_set_ps1(lptr[yri].zzz0))));

				}
				zz0=yr+(y-img_center)*tantheta[th2][yr];
				z=(int)zz0; 
				zz0=zz0-z;
				coef1=_mm_set_ps1(zz0);

				zz0=yr+(img_center-y)*tantheta[th2][yr];
				z2=(int)zz0; 
				zz0=zz0-z2;
				coef2=_mm_set_ps1(zz0);
				zz0=yr+(y-img_center)*tantheta[th2][yr];
				z=(int)zz0; 
				zz0=zz0-z;
				for(yr=yr_bottom[th]+ZShift;yr<=yr_top[th]-ZShift;yr++,z++,z2++){
					p1m[yr]=_mm_add_ps(p1m[yr],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(pam2[z2+1],pam[z])),_mm_mul_ps(coef1,_mm_add_ps(pam[z+1],pam2[z2]))));
					p2m[yr]=_mm_add_ps(p2m[yr],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(pam[z2+1],pam2[z])),_mm_mul_ps(coef1,_mm_add_ps(pam2[z+1],pam[z2]))));
				}
				for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++,yri++){
					z=lptr[yri].zzi;
					z2=lptr2[yri].zzi;
					p1m[yr]=_mm_add_ps(p1m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam2[z2+1],pam[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam[z+1],pam2[z2]),_mm_set_ps1(lptr[yri].zzz0))));
					p2m[yr]=_mm_add_ps(p2m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam[z2+1],pam2[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam2[z+1],pam[z2]),_mm_set_ps1(lptr[yri].zzz0))));
				}
			}
		}
		coef1=_mm_set_ps1(y_size);
		p1m=pp1[0];
		for(yr=0;yr<yr_pixels;yr++){
			p1m[yr]=_mm_mul_ps(p1m[yr],coef1);
		}
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			p1m= pp1[th/2];
			p2m= pp2[th/2];
			//			for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
			for(yr=yr_bottom[th];yr<yr_bottom[th]+ZShift;yr++){
				p1m[yr]=_mm_mul_ps(p1m[yr],_mm_set_ps1(rescalearray[th2][yr]));
				p2m[yr]=_mm_mul_ps(p2m[yr],_mm_set_ps1(rescalearray[th2][yr]));
			}
			coef1=_mm_set_ps1(rescalearray[th2][yr]);
			for(yr=yr_bottom[th]+ZShift;yr<=yr_top[th]-ZShift;yr++){
				p1m[yr]=_mm_mul_ps(p1m[yr],coef1);
				p2m[yr]=_mm_mul_ps(p2m[yr],coef1);
			}
			for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++){
				p1m[yr]=_mm_mul_ps(p1m[yr],_mm_set_ps1(rescalearray[th2][yr]));
				p2m[yr]=_mm_mul_ps(p2m[yr],_mm_set_ps1(rescalearray[th2][yr]));
			}
		}
	}
	return 1;
} /* end of forward_proj3d_view() */

int forward_proj3d_view1_thread_2d(float ***image,float ***prj,int view,int theta2,int verbose,
								int xr_start,int xr_end,float ***imagebuf)
{

	int x, y,i,z,yr,yri; /* integer voxel indices               */
	int z2;      /* integer indices for 2D-projections  */
	float zz0;   /* float voxel position in pixel corresponding to the first 
				 one in a sinogram, i.e. xr=0  */
	__m128 **pp1,**pp2;
	__m128 ***image_ptr;
	float *img1,*img2,*img3,*img4;
	float *img5,*img6,*img7,*img8;
	__m128 coef1;
	int xx,yy;
	float xx0,x1,cosv,sinv;
	__m128 *rim1;
	__m128 *rim2;
	__m128 *rim3;
	__m128 *rim4;
	__m128 *rim5;
	__m128 *rim6;
	__m128 *rim7;
	__m128 *rim8;
	__m128 *rim;
	__m128 *r1,*r2,*r4;
	int    th;
	int    th2;
	__m128 *p1m,*p2m;
	__m128 *pam;
	__m128 *pam2;
	float  *mptr;
	float  *mptr2;
	float *p1mf,*p2mf;
	int img_center=x_pixels/2;
	float delta;
	bool integraly=0;
	float cosv2,sinv2;
	float yprime1,yprime2;
	float xx1,xx2;
	int yp1,yp2;
	int xp1,xp2;
	bool interpoly=0;
	float zz1;
	float rescale;
	int z3;
	Forwardprojection_lookup *flookup_local;
	flookup_local=(Forwardprojection_lookup *) _alloca(groupmax*2*ZShift*sizeof(Forwardprojection_lookup));


	image_ptr=(__m128 ***) image;
	pam=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	pam2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	rim1=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim2=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim3=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim4=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim5=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim6=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim7=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	rim8=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	memset(rim1,0,z_pixels_simd*sizeof(__m128));
	memset(rim2,0,z_pixels_simd*sizeof(__m128));
	memset(rim3,0,z_pixels_simd*sizeof(__m128));
	memset(rim4,0,z_pixels_simd*sizeof(__m128));
	memset(rim5,0,z_pixels_simd*sizeof(__m128));
	memset(rim6,0,z_pixels_simd*sizeof(__m128));
	memset(rim7,0,z_pixels_simd*sizeof(__m128));
	memset(rim8,0,z_pixels_simd*sizeof(__m128));
	if(pam==NULL || pam2==NULL){
		LOG_EXIT("exiting");
	}

	mptr=(float *)pam;
	mptr2=(float *)pam2;
	cosv=cos_psi[view];
	sinv=sin_psi[view];
	cosv2=cos_psi[view+views/2];
	sinv2=sin_psi[view+views/2];

	img1=(float *) rim1;
	img2=(float *) rim2;
	img3=(float *) rim3;
	img4=(float *) rim4;
	img5=(float *) rim5;
	img6=(float *) rim6;
	img7=(float *) rim7;
	img8=(float *) rim8;


	if(fabs(cosv)>fabs(sinv)){
		delta=1/cosv;
		integraly=1;
		rescale=1/cosv;
	}	else {
		delta=1/sinv;
		integraly=0;
		rescale=1/sinv;
	}

	//for(y=0;y<y_pixels;y++){
	//	for(th=2,th2=0;th<th_pixels;th+=2,th2++){
	//		for(yr=yr_bottom[th],yri=0;yr<yr_bottom[th]+ZShift;yr++,yri++){
	//			zz0=yr+((y-y_off)*tantheta[th2][yr]);
	//			z=(int)zz0; 
	//			zz0=zz0-z;
	//			flookup[y][th2*ZShift*2+yri].zzz0=zz0;
	//			flookup[y][th2*ZShift*2+yri].zzi=z;
	//		}

	//		for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++,yri++){
	//			zz0=yr+((y-y_off)*tantheta[th2][yr]);
	//			z=(int)zz0; 
	//			zz0=zz0-z;
	//			flookup[y][th2*ZShift*2+yri].zzz0=zz0;
	//			flookup[y][th2*ZShift*2+yri].zzi=z;
	//		}
	//	}
	//}

	for(x=xr_start;x<xr_end;x++){

		if(x==0) continue;
		pp1=(__m128 **)prj[x];
		pp2=(__m128 **)prj[xr_pixels/2+x];

		if(cylwiny[x][0]==cylwiny[x][1]) continue;

		if(norm_zero_mask[view][x]==0){
						continue;
		}
// Symmetries
	//pi	xr	y(or x)	theta	y'	xx(or yy)

	//0		0	0		0		y1'		xx1
	//0		1	1		0		-y1'	-xx1
	//1		0	1		0		y1		xx1
	//1		1	0		0		-y1'	-xx1

	//0		0	0		1		-y1'	xx1
	//0		1	1		1		y1'		-xx1
	//1		0	1		1		-y1		xx1
	//1		1	0		1		y1'		-xx1

	//0		0	1		0		y2'		xx2
	//0		1	0		0		-y2'	-xx2
	//1		0	0		0		y2'		xx2
	//1		1	1		0		-y2		-xx2

	//0		0	1		1		-y2'	xx2
	//0		1	0		1		y2'		-xx2
	//1		0	0		1		-y2'	xx2
	//1		1	1		1		y2		-xx2
		
		xx=x-img_center;
		p1mf=(float *)pp1[0];

		for(z=0,yr=0;z<z_pixels;z++,yr+=4){
			p1mf[yr]=0;
			p1mf[yr+1]=0;
			p1mf[yr+2]=0;
			p1mf[yr+3]=0;
		}
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
				p1m=(__m128 *)pp1[th/2];
				p1mf=(float *)p1m;
				p2m=(__m128 *)pp2[th/2];
				p2mf=(float *)p2m;
				for(yr=yr_bottom[th]*4;yr<=yr_top[th]*4;yr+=4){
				p1mf[yr]=0;
				p1mf[yr+1]=0;
				p1mf[yr+2]=0;
				p1mf[yr+3]=0;
				p2mf[yr]=0;
				p2mf[yr+1]=0;
				p2mf[yr+2]=0;
				p2mf[yr+3]=0;
				
				}
		}

		for(y=1;y<y_pixels-1;y++){
			//		y=img_center;
			//if(y%2==0){
			//	xx=x-img_center;
			//	yy=y-img_center;
			//} else if(x!=img_center){
			//	xx=img_center-x;
			//	yy=y-img_center;
			////	continue;
			//} else {
			//	continue;
			//}
				xx=x-img_center;
				yy=y-img_center;

			if(integraly==1){
				//				y'=(y-x'*sinv)/cosv; x=(x'-y*sinv)/cosv  For pi
				//				y'=(x'*cosv2-x)/sinv2; y=(x'-x*cosv2)/sinv2 For pi+90;
				//		with program notation
				//		--->	yprime1=(y-x*sinv)/cosv; xx=(x-y*sinv)/cosv   For pi
				//		--->	yprime2=(x*cosv2-y)/sinv2; yy=(x-y*cosv2)/sinv2  For pi+90
				yprime1=(yy-xx*sinv)/cosv;
				xx1=(xx-yy*sinv)/cosv+img_center;
				xx2=(xx-yy*sinv)/cosv+img_center;
				//yprime2=(xx*cosv2-yy)/sinv2;
				yprime2=(yy+xx*sinv)/cosv;
				if(((yprime1+img_center) <(cylwiny[x][0])) || ((yprime1+img_center) >=cylwiny[x][1])) {
				//		memset(rim,0,z_pixels_simd*sizeof(__m128));
						continue;				
				}
				
				for(i=0;i<4;i++){
					if(i==0){		interpoly=1; rim=rim1; xx0=xx1;}	// x, pi
					else if(i==1){	interpoly=1; rim=rim2; xx0=x_pixels-xx1;}	// -x, pi
					else if(i==2){	interpoly=0; rim=rim3; xx0=xx2;}	// x , pi+90
					else {			interpoly=0; rim=rim4; xx0=x_pixels-xx2;}	// -x, pi+90


					//				xx0=(xx-img_center)*cosv*zoom-(yy-img_center)*sinv+img_center;		yy0=(xx-img_center)*sinv*zoom+(yy-img_center)*cosv+img_center;
					xx=(int) xx0; x1=xx0-xx;
					yy=y;
					if(i==1|| i==2) yy=y_pixels-y;

				//	if(((yprime1+img_center) <=cylwiny[x][0]) || ((yprime1+img_center) >=cylwiny[x][1])) {
				//		memset(rim,0,z_pixels_simd*sizeof(__m128));
				//		continue;				
				//	}
				
					if(xx<cylwiny[yy][0]-1 || xx>cylwiny[yy][1]){
			//			LOG_INFO("Erorr %d\t%d\t%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\n",view,x,y,xx,i,cylwiny[yy][0],cylwiny[yy][1],cosv,sinv,xx1,yprime1+128);
			//			LOG_INFO("xx1=%f\t%d\t%d\n",(x-img_center-(y-img_center)*sinv)/cosv+img_center,cylwiny[x][0],cylwiny[x][1]);
					//	memset(rim,0,z_pixels_simd*sizeof(__m128));
					//	continue;
					}
					if(interpoly==1){
						r1=image_ptr[xx][yy];	r2=image_ptr[xx+1][yy];
					} else {
						r1=image_ptr[yy][xx];	r2=image_ptr[yy][xx+1];
					}
					if(xx<0 || xx>x_pixels-2 || yy<0 || yy>x_pixels-2){
						memset(rim,0,z_pixels_simd*sizeof(__m128));
						continue;
					}
					coef1=_mm_set_ps1(x1);
					for(z=0;z<z_pixels_simd;z++){
						rim[z]=_mm_add_ps(r1[z],_mm_mul_ps(coef1,_mm_sub_ps(r2[z],r1[z])));
					}
				}
			} else if(integraly==0){
				//				y'=(x'*cosv-x)/sinv; y=(x'-x*cosv)/sinv		For pi
				//				y'=(y-x'*sinv2)/cosv2;  x=(x'-y*sinv2)/cosv2 For pi+90
				//		with program notation
				//		--->	yprime1=(x*cosv-y)/sinv; yy=(x-y*cosv)/sinv   For pi
				//		--->	yprime2=(y-x*sinv2)/cosv2; xx=(x-y*sinv2)/cosv2  For pi+90


				yprime1=(xx*cosv-yy)/sinv;
				xx1=(xx-yy*cosv)/sinv+img_center;

				yprime2=(yy-xx*sinv2)/cosv2;
//				xx2=(xx-yy*sinv2)/cosv2+img_center;
				xx2=(yy*cosv-xx)/sinv+img_center;
				yprime2=(-xx*cosv-yy)/sinv;
				if(((yprime1+img_center) <=cylwiny[x][0]) || ((yprime1+img_center) >=cylwiny[x][1])) {
				//		memset(rim,0,z_pixels_simd*sizeof(__m128));
						continue;				
				}
				for(i=0;i<4;i++){
					//if(i==0){		interpoly=0; rim=rim1;}	// x, theta, pi
					//else if(i==1){	interpoly=0; rim=rim2;}	// -x, theta, pi
					//else if(i==2){	interpoly=1; rim=rim3;}	// x , theta, pi+90
					//else {			interpoly=1; rim=rim4;}	// -x, theta, pi+90
					if(i==0){		interpoly=0; rim=rim1; xx0=xx1;}	// x, pi
					else if(i==1){	interpoly=0; rim=rim2; xx0=x_pixels-xx1;}	// -x, pi
					else if(i==2){	interpoly=1; rim=rim3; xx0=xx2;}	// x , pi+90
					else {			interpoly=1; rim=rim4; xx0=x_pixels-xx2;}	// -x, pi+90

					xx=(int) xx0; x1=xx0-xx; yy=y;
					
					if(i==1||i==3) yy=y_pixels-y;

					if(((yprime1+img_center) <cylwiny[x][0]) || ((yprime1+img_center) >cylwiny[x][1])) {
						memset(rim,0,z_pixels_simd*sizeof(__m128));
						continue;				
					}
					if(xx<cylwiny[yy][0] || xx>cylwiny[yy][1]){
					//	LOG_INFO("Erorr %d\t%d\t%d\t%d\t%d\n",view,x,y,xx,i);
						memset(rim,0,z_pixels_simd*sizeof(__m128));
						continue;
					}

					if(interpoly==1){
						r1=image_ptr[xx][yy];	r2=image_ptr[xx+1][yy];
					} else {
						r1=image_ptr[yy][xx];	r2=image_ptr[yy][xx+1];
					}
					if(xx<0 || xx>x_pixels-2 || xx0<0 || xx0>x_pixels-2){
						memset(rim,0,z_pixels_simd*sizeof(__m128));
						continue;
					}
					coef1=_mm_set_ps1(x1);
					for(z=0;z<z_pixels_simd;z++){
						rim[z]=_mm_add_ps(r1[z],_mm_mul_ps(coef1,_mm_sub_ps(r2[z],r1[z])));
					}

				}

			}

			p1mf=(float *)pp1[0];
			for(z=0,yr=0;z<z_pixels;z++,yr+=4){
				p1mf[yr]+=img1[z];
				p1mf[yr+1]+=img2[z];
				p1mf[yr+2]+=img3[z];
				p1mf[yr+3]+=img4[z];
			}
			//	for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			//		for(yr=yr_bottom[th],yri=0;yr<yr_bottom[th]+ZShift;yr++,yri++){
			//			zz0=yr+((y-y_off)*tantheta[th2][yr]);
			//			z=(int)zz0; 
			//			zz0=zz0-z;
			//			flookup[y][th2*ZShift*2+yri].zzz0=zz0;
			//			flookup[y][th2*ZShift*2+yri].zzi=z;
			//		}

			//		for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++,yri++){
			//			zz0=yr+((y-y_off)*tantheta[th2][yr]);
			//			z=(int)zz0; 
			//			zz0=zz0-z;
			//			flookup[y][th2*ZShift*2+yri].zzz0=zz0;
			//			flookup[y][th2*ZShift*2+yri].zzi=z;
			//		}
			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
				p1m=(__m128 *)pp1[th/2];
				p1mf=(float *)p1m;
				p2m=(__m128 *)pp2[th/2];
				p2mf=(float *)p2m;
				//			for(yr=yr_bottom[th]*4;yr<=yr_top[th]*4;yr+=4){

				for(yr=yr_bottom[th]*4,yri=yr_bottom[th];yr<(yr_bottom[th]+ZShift)*4;yr+=4,yri++){
					zz0=yri + yprime1*tantheta[th2][yri];
					z=(int) zz0;
					zz0=zz0-z;

					zz1=yri - yprime1*tantheta[th2][yri];
					z2=(int) zz1;
					zz1=zz1-z2;
				//	if(z<0 || z>z_pixels) {
				//		LOG_INFO("Erorr z-pixels %d\t%d\t%d\t%d\t%d\t%d\t%d\t%f\t%f\n",view,x,y,z,yri,yr,yr_bottom[th],tantheta[th2][yri],yprime1);
				////		LOG_INFO("Erorr z-pixels %d\n",z);
				//		continue;
				//	}
				//	if(z2<0 || z2>z_pixels) {
				//		LOG_INFO("Erorr2 z-pixels %d\t%d\t%d\t%d\t%d\t%d\t%d\t%f\t%f\n",view,x,y,z2,yri,yr,yr_bottom[th],tantheta[th2][yri],yprime1);
				////		LOG_INFO("Erorr z-pixels %d\n",z);
				//		continue;
				//	}


					//positive theta
					p1mf[yr]+=  img1[z]+zz0*(img1[z+1]-img1[z]);
					p1mf[yr+1]+=img2[z2]+zz1*(img2[z2+1]-img2[z2]);
					p1mf[yr+2]+=img3[z]+zz0*(img3[z+1]-img3[z]);
					p1mf[yr+3]+=img4[z2]+zz1*(img4[z2+1]-img4[z2]);
					//negative theta
					p2mf[yr]+=  img1[z2]+zz1*(img1[z2+1]-img1[z2]);
					p2mf[yr+1]+=img2[z]+zz0*(img2[z+1]-img2[z]);
					p2mf[yr+2]+=img3[z2]+zz1*(img3[z2+1]-img3[z2]);
					p2mf[yr+3]+=img4[z]+zz0*(img4[z+1]-img4[z]);
				}
				zz0=yri + yprime1*tantheta[th2][yri];
				z=(int) zz0;
				zz0=zz0-z;

				zz1=yri - yprime1*tantheta[th2][yri];
				z2=(int) zz1;
				zz1=zz1-z2;
			//	zz1=yri + yprime2*tantheta[th2][yri];
			//	z2=(int) zz1;
			//	zz1=zz1-z;
				for(;yr<(yr_top[th]-ZShift)*4;yr+=4,z++,z2++,yri++){
					//positive theta
					p1mf[yr]+=  img1[z]+zz0*(img1[z+1]-img1[z]);
					p1mf[yr+1]+=img2[z2]+zz1*(img2[z2+1]-img2[z2]);
					p1mf[yr+2]+=img3[z]+zz0*(img3[z+1]-img3[z]);
					p1mf[yr+3]+=img4[z2]+zz1*(img4[z2+1]-img4[z2]);
					//negative theta
					p2mf[yr]+=  img1[z2]+zz1*(img1[z2+1]-img1[z2]);
					p2mf[yr+1]+=img2[z]+zz0*(img2[z+1]-img2[z]);
					p2mf[yr+2]+=img3[z2]+zz1*(img3[z2+1]-img3[z2]);
					p2mf[yr+3]+=img4[z]+zz0*(img4[z+1]-img4[z]);
				}
				for(;yri<=yr_top[th];yr+=4,yri++){
					zz0=yri + yprime1*tantheta[th2][yri];
					z=(int) zz0;
					zz0=zz0-z;

					zz1=yri - yprime1*tantheta[th2][yri];
					z2=(int) zz1;
					zz1=zz1-z2;
					//if(z<0 || z>z_pixels) {
					//	LOG_INFO("Erorr z-pixels %d\t%d\t%d\t%d\t%d\t%d\t%d\n",view,x,y,z,yri,yr,yr_top[th]);
					//}

					//positive theta
					p1mf[yr]+=  img1[z]+zz0*(img1[z+1]-img1[z]);
					p1mf[yr+1]+=img2[z2]+zz1*(img2[z2+1]-img2[z2]);
					p1mf[yr+2]+=img3[z]+zz0*(img3[z+1]-img3[z]);
					p1mf[yr+3]+=img4[z2]+zz1*(img4[z2+1]-img4[z2]);
					//negative theta
					p2mf[yr]+=  img1[z2]+zz1*(img1[z2+1]-img1[z2]);
					p2mf[yr+1]+=img2[z]+zz0*(img2[z+1]-img2[z]);
					p2mf[yr+2]+=img3[z2]+zz1*(img3[z2+1]-img3[z2]);
					p2mf[yr+3]+=img4[z]+zz0*(img4[z+1]-img4[z]);
				}
			}
		}
		coef1=_mm_set_ps1(y_size*rescale);
		p1m=pp1[0];
		for(yr=0;yr<yr_pixels;yr++){
			p1m[yr]=_mm_mul_ps(p1m[yr],coef1);
		}
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			p1m= pp1[th/2];
			p2m= pp2[th/2];
			//			for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
			for(yr=yr_bottom[th];yr<yr_bottom[th]+ZShift;yr++){
				p1m[yr]=_mm_mul_ps(p1m[yr],_mm_set_ps1(rescalearray[th2][yr]*rescale));
				p2m[yr]=_mm_mul_ps(p2m[yr],_mm_set_ps1(rescalearray[th2][yr]*rescale));
			}
			coef1=_mm_set_ps1(rescalearray[th2][yr]*rescale);
			for(yr=yr_bottom[th]+ZShift;yr<=yr_top[th]-ZShift;yr++){
				p1m[yr]=_mm_mul_ps(p1m[yr],coef1);
				p2m[yr]=_mm_mul_ps(p2m[yr],coef1);
			}
			for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++){
				p1m[yr]=_mm_mul_ps(p1m[yr],_mm_set_ps1(rescalearray[th2][yr]*rescale));
				p2m[yr]=_mm_mul_ps(p2m[yr],_mm_set_ps1(rescalearray[th2][yr]*rescale));
			}
		}
	}
	return 1;
} /* end of forward_proj3d_view() */

//int forward_proj3d_view1_thread_pack(float ***image,float ***imagepack,float ***prj,int view,int theta2,int verbose,
//								int xr_start,int xr_end,float ***imagebuf)
//{
//
//	int x, y,i,z,yr,yri; /* integer voxel indices               */
//	int z2;      /* integer indices for 2D-projections  */
//	float zz0;   /* float voxel position in pixel corresponding to the first 
//				 one in a sinogram, i.e. xr=0  */
//	Forwardprojection_lookup lptr[500];
//	Forwardprojection_lookup lptr2[500];
//	__m128 **pp1,**pp2;
//	__m128 ***image_ptr;
//	float *img1,*img2,*img3,*img4;
//	float *img5,*img6,*img7,*img8;
//	__m128 coef1,coef2;
//	int xx,yy;
//	float xx0,yy0,x1,y1,cosv,sinv;
//	__m128 *rim1;
//	__m128 *rim2;
//	__m128 *rim3;
//	__m128 *rim4;
//	__m128 *rim5;
//	__m128 *rim6;
//	__m128 *rim7;
//	__m128 *rim8;
//	__m128 *rim;
//	__m128 *r1,*r2,*r3,*r4;	
//	__m128 *r5,*r6,*r7,*r8;
//	//	__m128 rc1[8],rc2[8],rc3[8],rc4[8];
//	__m128 rc1[8];//,rc2[8],rc3[8],rc4[8];
//	int    th;
//	int    th2;
//	__m128 *p1m,*p2m;
//	__m128 *pam;
//	__m128 *pam2;
//	float  *mptr;
//	float  *mptr2;
//	float *p1mf,*p2mf;
//	int rotate_zero_flag;
//	int rotate_flag1;
//	int rotate_flag2;
//	int img_center=x_pixels/2;
//	int xxp[2],yyp[2];
//	float xxr[8],yyr[8];
//	__m128 ***imagepack_ptr;
//
//	image_ptr=(__m128 ***) image;
//	imagepack_ptr=(__m128 ***) imagepack;
//
//	pam=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	pam2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	rim1=(__m128 *) _alloca(z_pixels_simd*4*sizeof(__m128));
//	rim2=(__m128 *) _alloca(z_pixels_simd*4*sizeof(__m128));
//	rim3=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
//	rim4=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
//	rim5=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
//	rim6=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
//	rim7=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
//	rim8=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
//	memset(rim1,0,z_pixels_simd*4*sizeof(__m128));
//	memset(rim2,0,z_pixels_simd*4*sizeof(__m128));
//	memset(rim3,0,z_pixels_simd*sizeof(__m128));
//	memset(rim4,0,z_pixels_simd*sizeof(__m128));
//	memset(rim5,0,z_pixels_simd*sizeof(__m128));
//	memset(rim6,0,z_pixels_simd*sizeof(__m128));
//	memset(rim7,0,z_pixels_simd*sizeof(__m128));
//	memset(rim8,0,z_pixels_simd*sizeof(__m128));
//	if(pam==NULL || pam2==NULL){
//		LOG_EXIT("exiting");
//	}
//
//	mptr=(float *)pam;
//	mptr2=(float *)pam2;
//	cosv=cos_psi[view];
//	sinv=sin_psi[view];
//	img1=(float *) rim1;
//	img2=(float *) rim2;
//	img3=(float *) rim3;
//	img4=(float *) rim4;
//	img5=(float *) rim5;
//	img6=(float *) rim6;
//	img7=(float *) rim7;
//	img8=(float *) rim8;
//
//	for(x=xr_start;x<xr_end;x++){
//
//		if(x==0) continue;
//		pp1=(__m128 **)prj[x];
//		pp2=(__m128 **)prj[xr_pixels/2+x];
//
//		if(cylwiny[x][0]==cylwiny[x][1]) continue;
//
//		if(norm_zero_mask[view][x]==0){
//						continue;
//		}
//
//		for(i=0;i<4;i++){
//			if(i==0){	xx=x;yy=img_center; rim=rim1;}
//			else if(i==1){		xx=x_pixels-x;yy=img_center;rim=rim2;}
//			else if(i==2){			xx=img_center;yy=x;rim=rim3;}
//			else {			xx=img_center;yy=x_pixels-x;rim=rim4;}
//
//			xx0=(xx-img_center)*cosv*zoom-(yy-img_center)*sinv+img_center;		yy0=(xx-img_center)*sinv*zoom+(yy-img_center)*cosv+img_center;
//			if(xx0<0 || xx0>x_pixels-2 ||yy0<0 || xx0>x_pixels-2) continue;
//			xx=(int) xx0; yy=(int) yy0;		x1=xx0-xx;y1=yy0-yy;
//			r1=image_ptr[xx][yy];		r2=image_ptr[xx+1][yy];		r3=image_ptr[xx][yy+1];		r4=image_ptr[xx+1][yy+1];
//			rc1[0]=_mm_set1_ps((1-x1)*(1-y1));		rc1[1]=_mm_set1_ps(x1*(1-y1));		rc1[2]=_mm_set1_ps(y1*(1-x1));		rc1[3]=_mm_set1_ps(y1*x1);
//			for(z=0;z<z_pixels_simd;z++){
//				rim[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(r1[z],rc1[0]),_mm_mul_ps(r2[z],rc1[1])),_mm_add_ps(_mm_mul_ps(r3[z],rc1[2]),_mm_mul_ps(r4[z],rc1[3])));
//			}
//		}
//
//		p1mf=(float *)pp1[0];
//		for(z=0,yr=0;z<z_pixels;z++,yr+=4){
//			p1mf[yr]=img1[z];
//			p1mf[yr+1]=img2[z];
//			p1mf[yr+2]=img3[z];
//			p1mf[yr+3]=img4[z];
//		}
//		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//			p1m=(__m128 *)pp1[th/2];
//			p1mf=(float *)p1m;
//			p2m=(__m128 *)pp2[th/2];
//			p2mf=(float *)p2m;
//			for(yr=yr_bottom[th]*4,z=yr_bottom[th];yr<=yr_top[th]*4;yr+=4,z++){
//				p1mf[yr]=img1[z];
//				p1mf[yr+1]=img2[z];
//				p1mf[yr+2]=img3[z];
//				p1mf[yr+3]=img4[z];
//				p2mf[yr]=img1[z];
//				p2mf[yr+1]=img2[z];
//				p2mf[yr+2]=img3[z];
//				p2mf[yr+3]=img4[z];
//			}
//		}
//
//		for(y=cylwiny[x][0];y<img_center;y++){
//				rotate_zero_flag=0;
//
//				rotate_flag1=0;
//				rotate_flag2=0;
//				for(i=0;i<2;i++){
//					if(i==0){	xx=x;yy=y;rim=rim1; }								//+theta,view,	+xr
//					else if(i==1){		xx=y;yy=x;rim=rim2;}				//+theta,view,	-xr
//
//					xx0=(xx-img_center)*cosv-(yy-img_center)*sinv+img_center;		yy0=(xx-img_center)*sinv+(yy-img_center)*cosv+img_center;
//					if(xx0<0 || xx0>x_pixels-2 ||yy0<0 || xx0>x_pixels-2){
//						memset(rim,0,z_pixels_simd*4*sizeof(__m128));
//			//			continue;
//					}
//					xx=(int) xx0; yy=(int) yy0;		x1=xx0-xx;y1=yy0-yy;
//					if(xx>=img_center){
//						if(i==0){ rotate_flag1=1;}
//						else	{ rotate_flag2=1;}
//						xxp[i]=yy;
//						yyp[i]=x_pixels-xx;
//
//					} else {
//						if(i==0){ rotate_flag1=0;r1=image_ptr[xx][yy];}
//						else	{ rotate_flag2=0;r2=image_ptr[xx][yy];}
//						xxp[i]=xx;yyp[i]=yy;
//
//					}
//				}
//	//			if(rotate_flag1!=rotate_flag2){
//	//				LOG_INFO("error %d\t%d\t%d\t%d\t%d\n",view,x,y,rotate_flag1,rotate_flag2);
//	//			}
//				for(i=0;i<8;i++){
//					if(i==0){				xx=x         ;yy=y;rim=rim1; }								//+theta,view,	+xr
//					else if(i==1){			xx=x_pixels-x;yy=y_pixels-y;rim=rim2;}				//+theta,view,	-xr
//					else if(i==2){			xx=y_pixels-y;yy=x;rim=rim3;}			//+theta,view+90,+xr
//					else if(i==3){			xx=y		 ;yy=x_pixels-x;rim=rim4;}	//+theta,view+90,-xr
//					else if(i==4){			xx=y		 ;yy=x;rim=rim5;}			//-theta,view,	+xr
//					else if(i==5){			xx=x_pixels-y;yy=y_pixels-x;rim=rim6;}	//-theta,view,	-xr
//					else if(i==6){			xx=x_pixels-x;yy=y;rim=rim7;}						//-theta,view+90,	+xr
//					else if(i==7){			xx=x		 ;yy=x_pixels-y;rim=rim8;}			//-theta,view+90,	-xr
//
//					xx0=(xx-img_center)*cosv-(yy-img_center)*sinv+img_center;		yy0=(xx-img_center)*sinv+(yy-img_center)*cosv+img_center;
//					if(xx0<0 || xx0>x_pixels-2 ||yy0<0 || xx0>x_pixels-2){
//						//						memset(rim,0,z_pixels_simd*sizeof(__m128));
//						//						continue;
//						LOG_INFO("error\n");
//					}
//					xx=(int) xx0; yy=(int) yy0;		x1=xx0-xx;y1=yy0-yy;
//					if(i<4){
//						if(rotate_flag1==0){
//							xxr[i]=x1;yyr[i]=y1;
//						} else {
//							if(i==0){ xxr[2]=x1;yyr[2]=y1;}
//							else if(i==1){ xxr[3]=x1;yyr[3]=y1;}
//							else if(i==2){ xxr[1]=x1;yyr[1]=y1;}
//							else	     { xxr[0]=x1;yyr[0]=y1;}
//						}
//					} else {
//						if(rotate_flag2==0){
//							xxr[i]=x1;yyr[i]=y1;
//						} else {
//							if(i==4)	  {xxr[6]=x1;yyr[6]=y1;}
//							else if(i==5) {xxr[7]=x1;yyr[7]=y1;}
//							else if(i==6) {xxr[5]=x1;yyr[5]=y1;}
//							else	      {xxr[4]=x1;yyr[4]=y1;}
//						}
//					}
//				}
//				rc1[0]=_mm_set_ps((1-xxr[0])*(1-yyr[0]),(1-xxr[1])*(1-yyr[1]),(1-xxr[2])*(1-yyr[2]),(1-xxr[3])*(1-yyr[3]));
//				rc1[1]=_mm_set_ps(xxr[0]*(1-yyr[0]),xxr[1]*(1-yyr[1]),xxr[2]*(1-yyr[2]),xxr[3]*(1-yyr[3]));
//				rc1[2]=_mm_set_ps((1-xxr[0])*yyr[0],(1-xxr[1])*yyr[1],(1-xxr[2])*yyr[2],(1-xxr[3])*yyr[3]);
//				rc1[3]=_mm_set_ps(xxr[0]*yyr[0],xxr[1]*yyr[1],xxr[2]*yyr[2],xxr[3]*yyr[3]);
//
//				rc1[4]=_mm_set_ps((1-xxr[4])*(1-yyr[4]),(1-xxr[5])*(1-yyr[5]),(1-xxr[6])*(1-yyr[6]),(1-xxr[7])*(1-yyr[7]));
//				rc1[5]=_mm_set_ps(xxr[4]*(1-yyr[4]),xxr[5]*(1-yyr[5]),xxr[6]*(1-yyr[6]),xxr[7]*(1-yyr[7]));
//				rc1[6]=_mm_set_ps((1-xxr[4])*yyr[4],(1-xxr[5])*yyr[5],(1-xxr[6])*yyr[6],(1-xxr[7])*yyr[7]);
//				rc1[7]=_mm_set_ps(xxr[4]*yyr[4],xxr[5]*yyr[5],xxr[6]*yyr[6],xxr[7]*yyr[7]);
//
//				r1=imagepack_ptr[xxp[0]][yyp[0]];		r2=imagepack_ptr[xxp[0]+1][yyp[0]];		r3=imagepack_ptr[xxp[0]][yyp[0]+1];		r4=imagepack_ptr[xxp[0]+1][yyp[0]+1];
//				r5=imagepack_ptr[xxp[1]][yyp[1]];		r6=imagepack_ptr[xxp[1]+1][yyp[1]];		r7=imagepack_ptr[xxp[1]][yyp[1]+1];		r8=imagepack_ptr[xxp[1]+1][yyp[1]+1];
//
//				for(z=0;z<z_pixels_simd*4;z++){
//					rim1[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(r1[z],rc1[0]),_mm_mul_ps(r2[z],rc1[1])),_mm_add_ps(_mm_mul_ps(r3[z],rc1[2]),_mm_mul_ps(r4[z],rc1[3])));
//					rim2[z]=_mm_add_ps(_mm_add_ps(_mm_mul_ps(r5[z],rc1[4]),_mm_mul_ps(r6[z],rc1[5])),_mm_add_ps(_mm_mul_ps(r7[z],rc1[6]),_mm_mul_ps(r8[z],rc1[7])));
//				}
//				if(rotate_flag1==0 && rotate_flag2==0){
//					for(z=0,yr=0;z<z_pixels*4;z+=4,yr+=4){
//						mptr[yr]=img1[z];	
//						mptr[yr+1]=img2[z+2];	
//						mptr[yr+2]=img1[z+2];	
//						mptr[yr+3]=img2[z+1];	
//						mptr2[yr]=	img2[z+3];	
//						mptr2[yr+1]=img1[z+1];	
//						mptr2[yr+2]=img2[z];	
//						mptr2[yr+3]=img1[z+3];	
//					}
//				} else if(rotate_flag1==1 && rotate_flag2==0){
//					for(z=0,yr=0;z<z_pixels*4;z+=4,yr+=4){
//						mptr[yr]=img1[z+2];	
//						mptr[yr+1]=img2[z+2];	
//						mptr[yr+2]=img1[z+1];	
//						mptr[yr+3]=img2[z+1];	
//						mptr2[yr]=	img2[z+3];	
//						mptr2[yr+1]=img1[z+3];	
//						mptr2[yr+2]=img2[z];	
//						mptr2[yr+3]=img1[z];	
//					}
//
//				} else if(rotate_flag1==0 && rotate_flag2==1){
//					for(z=0,yr=0;z<z_pixels*4;z+=4,yr+=4){
//						mptr[yr]=img1[z];	
//						mptr[yr+1]=img2[z+1];	
//						mptr[yr+2]=img1[z+2];	
//						mptr[yr+3]=img2[z+3];	
//						mptr2[yr]=	img2[z];	
//						mptr2[yr+1]=img1[z+1];	
//						mptr2[yr+2]=img2[z+2];	
//						mptr2[yr+3]=img1[z+3];
//					}
//				} else {
//					for(z=0,yr=0;z<z_pixels*4;z+=4,yr+=4){
//						mptr[yr]=img1[z+2];		
//						mptr[yr+1]=img2[z+1];	
//						mptr[yr+2]=img1[z+1];	
//						mptr[yr+3]=img2[z+3];	
//						mptr2[yr]=	img2[z];	
//						mptr2[yr+1]=img1[z+3];	
//						mptr2[yr+2]=img2[z+2];	
//						mptr2[yr+3]=img1[z];
//					}
//				}
//
//			/* theta = 0 */
//			p1m=pp1[0];
//			for(z=0;z<z_pixels;z++){
//				p1m[z]=_mm_add_ps(p1m[z],_mm_add_ps(pam[z],pam2[z]));
//			}
//			/* theta = +1,-1, ..... */
//			//			lptr=flookup[y];
//			memcpy(lptr,flookup,groupmax*2*ZShift*sizeof(Forwardprojection_lookup));
//			memcpy(lptr2,flookup[y_pixels-y],groupmax*2*ZShift*sizeof(Forwardprojection_lookup));
//			yri=0;
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				yri=th2*ZShift*2;
//				p1m=pp1[th/2];
//				p2m=pp2[th/2];
//				if(lptr[yri].zzi<0) continue;
//				if(lptr2[yri+ZShift*2-1].zzi>z_pixels-1) continue;
//				for(yr=yr_bottom[th];yr<yr_bottom[th]+ZShift;yr++,yri++){
//					z=lptr[yri].zzi;
//					z2=lptr2[yri].zzi;
//					p1m[yr]=_mm_add_ps(p1m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam2[z2+1],pam[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam[z+1],pam2[z2]),_mm_set_ps1(lptr[yri].zzz0))));
//					p2m[yr]=_mm_add_ps(p2m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam[z2+1],pam2[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam2[z+1],pam[z2]),_mm_set_ps1(lptr[yri].zzz0))));
//				}
//				zz0=yr+(y-img_center)*tantheta[th2][yr];
//				z=(int)zz0; 
//				zz0=zz0-z;
//				coef1=_mm_set_ps1(zz0);
//
//				zz0=yr+(img_center-y)*tantheta[th2][yr];
//				z2=(int)zz0; 
//				zz0=zz0-z2;
//				coef2=_mm_set_ps1(zz0);
//				zz0=yr+(y-img_center)*tantheta[th2][yr];
//				z=(int)zz0; 
//				zz0=zz0-z;
//
//				for(yr=yr_bottom[th]+ZShift;yr<=yr_top[th]-ZShift;yr++,z++,z2++){
//					p1m[yr]=_mm_add_ps(p1m[yr],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(pam2[z2+1],pam[z])),_mm_mul_ps(coef1,_mm_add_ps(pam[z+1],pam2[z2]))));
//					p2m[yr]=_mm_add_ps(p2m[yr],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(pam[z2+1],pam2[z])),_mm_mul_ps(coef1,_mm_add_ps(pam2[z+1],pam[z2]))));
//				}
//				
//				for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++,yri++){
//					z=lptr[yri].zzi;
//					z2=lptr2[yri].zzi;
//					p1m[yr]=_mm_add_ps(p1m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam2[z2+1],pam[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam[z+1],pam2[z2]),_mm_set_ps1(lptr[yri].zzz0))));
//					p2m[yr]=_mm_add_ps(p2m[yr],_mm_add_ps(_mm_mul_ps(_mm_add_ps(pam[z2+1],pam2[z]),_mm_set_ps1(lptr2[yri].zzz0)),_mm_mul_ps(_mm_add_ps(pam2[z+1],pam[z2]),_mm_set_ps1(lptr[yri].zzz0))));
//				}
//			}
//		}
//		coef1=_mm_set_ps1(y_size);
//		p1m=pp1[0];
//		for(yr=0;yr<yr_pixels;yr++){
//			p1m[yr]=_mm_mul_ps(p1m[yr],coef1);
//		}
//		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//			p1m= pp1[th/2];
//			p2m= pp2[th/2];
//			//			for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
//			for(yr=yr_bottom[th];yr<yr_bottom[th]+ZShift;yr++){
//				p1m[yr]=_mm_mul_ps(p1m[yr],_mm_set_ps1(rescalearray[th2][yr]));
//				p2m[yr]=_mm_mul_ps(p2m[yr],_mm_set_ps1(rescalearray[th2][yr]));
//			}
//			coef1=_mm_set_ps1(rescalearray[th2][yr]);
//			for(yr=yr_bottom[th]+ZShift;yr<=yr_top[th]-ZShift;yr++){
//				p1m[yr]=_mm_mul_ps(p1m[yr],coef1);
//				p2m[yr]=_mm_mul_ps(p2m[yr],coef1);
//			}
//			for(yr=yr_top[th]-ZShift+1;yr<=yr_top[th];yr++){
//				p1m[yr]=_mm_mul_ps(p1m[yr],_mm_set_ps1(rescalearray[th2][yr]));
//				p2m[yr]=_mm_mul_ps(p2m[yr],_mm_set_ps1(rescalearray[th2][yr]));
//			}
//		}
//	}
//	return 1;
//} /* end of forward_proj3d_view() */

//int back_proj3d_view_n1_old(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup lptr1[MAXGROUP];
//	Backprojection_lookup lptr2[MAXGROUP];
//	int    th,th2;
//	register int    yr,yr2, z,z2;
//	int    x, y;		/* indice of the 3 dimensions in reconstructed image */
//	int ystart,yend;
//	float  *i1f,*i2f,*i3f,*i4f;
//	__m128 *p1fm;
//	__m128 **pp1;
//	//	float  **pp1;
//	//	int y_off=y_pixels/2;
//	__m128 coef1,coef2;
//	__m128 *i1fmm;
//	__m128 *i2fmm;
//	int zero_mask_flag=0;
//	float *mptr1,*mptr2;
//	int z_start=0,z_end=z_pixels;
//
//	i1fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	i2fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	if(i2fmm==NULL || i1fmm==NULL){
//		LOG_EXIT("exiting");
//	}
//
//	mptr1=(float *) i1fmm;
//	mptr2=(float *) i2fmm;
//	//	LOG_INFO("backview =%d\n",view);
//	for (x = x_start; x < x_end; x++) {
//		if(view_90flag==0) pp1=(__m128 **)prj[x];
//		else	pp1=(__m128 **) prj[x+xr_pixels/2];
//		pp1=(__m128 **) prj[x];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//		zero_mask_flag=0;
//		if(norm_zero_mask[view][x]==0){
//			zero_mask_flag=1;
//		}
//		for(y = ystart; y <y_pixels/2;y++){// yend; y++) {
//			if(zero_mask_flag) goto zero_mask_flag_label;
//			p1fm=pp1[0];
//			if(y==y_pixels/2) continue;
//			memcpy(i1fmm,&p1fm[z_start],(z_pixels_simd*4)*sizeof(__m128));
//			memcpy(i2fmm,&p1fm[z_start],(z_pixels_simd*4)*sizeof(__m128));
//			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
//			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=pp1[th/2];
//				z2=lptr2[th2].zsa;
//				yr2=lptr2[th2].yra;
//				coef2=_mm_set1_ps(lptr2[th2].yr2a);
////				for(z=lptr1[th2].zsa,yr=lptr1[th2].yra,coef1=_mm_set1_ps(lptr1[th2].yr2a);z<=lptr1[th2].zea;z++,yr++){
////					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
////				}
////				for(z=lptr2[th2].zsa,yr=lptr2[th2].yra,coef1=_mm_set1_ps(lptr2[th2].yr2a);z<=lptr2[th2].zea;z++,yr++){
////					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
////				}
//				for(z=lptr1[th2].zsa,yr=lptr1[th2].yra,coef1=_mm_set1_ps(lptr1[th2].yr2a);z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//				}
//			}
//
//zero_mask_flag_label:
//
//			if(view_90flag==0){
//				i1f=image[x][y];
//				i2f=image[x][y_pixels-y];
//				i3f=image[x_pixels-x][y];
//				i4f=image[x_pixels-x][y_pixels-y];
//				if(x!=x_pixels/2){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i3f,0,z_pixels_simd*sizeof(__m128));
//						memset(i4f,0,z_pixels_simd*sizeof(__m128));
//					}
//					else{
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr]+mptr2[yr+1];
//							i2f[z]=mptr1[yr+1]+mptr2[yr];
//							i3f[z]=mptr1[yr+2]+mptr2[yr+3];
//							i4f[z]=mptr1[yr+3]+mptr2[yr+2];
//						}
//					}
//
//				}else {
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//					}
//					else{
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr]+mptr2[yr+1];
//							i2f[z]=mptr1[yr+1]+mptr2[yr];
//						}
//					}
//				}
//			} else if(!zero_mask_flag){
//				i1f=image[y_pixels-y][x];
//				i2f=image[y][x];
//				i3f=image[y_pixels-y][x_pixels-x];
//				i4f=image[y][x_pixels-x];
//				if(x!=x_pixels/2){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]+=mptr1[yr]+mptr2[yr+1];
//						i2f[z]+=mptr1[yr+1]+mptr2[yr];
//						i3f[z]+=mptr1[yr+2]+mptr2[yr+3];
//						i4f[z]+=mptr1[yr+3]+mptr2[yr+2];
//					}
//				}else {
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]+=mptr1[yr]+mptr2[yr+1];
//						i2f[z]+=mptr1[yr+1]+mptr2[yr];
//					}
//				}
//			}
//		}
//
//		y=y_pixels/2;
//		p1fm=(__m128 *)pp1[0];
//		memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//			p1fm=(__m128 *)pp1[th/2];
//			for(z=yr_bottom[th];z<=yr_top[th];z++){
//				i1fmm[z]=_mm_add_ps(i1fmm[z],p1fm[z]);//_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//			}
//		}
//		if(view_90flag==0){
//			i1f=image[x][y];
//			i4f=image[x_pixels-x][y];
//			if(x!=x_pixels/2){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr]+mptr1[yr+1];
//					i4f[z]=mptr1[yr+2]+mptr1[yr+3];
//				}
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr]+mptr1[yr+1];
//				}
//			}
//		} else {
//			i1f=image[y_pixels-y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=x_pixels/2){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]+=mptr1[yr]+mptr1[yr+1];
//					i4f[z]+=mptr1[yr+2]+mptr1[yr+3];
//				}
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]+=mptr1[yr]+mptr1[yr+1];
//				}
//			}
//		}
//	}
//	return 1;
//}
int back_proj3d_view_n1(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
{
	Backprojection_lookup lptr1[MAXGROUP];
	Backprojection_lookup lptr2[MAXGROUP];
	int    th,th2;
	register int    yr,yr2, z,z2;
	int    x, y;		/* indice of the 3 dimensions in reconstructed image */
	int ystart,yend;
	float  *i1f,*i2f,*i3f,*i4f;
	__m128 *p1fm;
	__m128 **pp1;
	//	float  **pp1;
	//	int y_off=y_pixels/2;
	__m128 coef1,coef2;
	__m128 *i1fmm;
	__m128 *i2fmm;
	int zero_mask_flag=0;
	float *mptr1,*mptr2;
	int z_start=0,z_end=z_pixels;

	i1fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	i2fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	if(i2fmm==NULL || i1fmm==NULL){
		LOG_EXIT("exiting");
	}

	mptr1=(float *) i1fmm;
	mptr2=(float *) i2fmm;
	//	LOG_INFO("backview =%d\n",view);
	for (x = x_start; x < x_end; x++) {
		if(view_90flag==0) pp1=(__m128 **)prj[x];
		else	pp1=(__m128 **) prj[x+xr_pixels/2];
		pp1=(__m128 **) prj[x];
		ystart=cylwiny[x][0];
		yend=cylwiny[x][1];
		if(ystart==0 && yend==0) continue;
		zero_mask_flag=0;
		if(norm_zero_mask[view][x]==0){
			zero_mask_flag=1;
		}
		for(y = ystart; y <y_pixels/2;y++){// yend; y++) {
			if(zero_mask_flag) goto zero_mask_flag_label;
			p1fm=pp1[0];
			if(y==y_pixels/2) continue;
			memcpy(i1fmm,&p1fm[z_start],(z_pixels_simd*4)*sizeof(__m128));
			memcpy(i2fmm,&p1fm[z_start],(z_pixels_simd*4)*sizeof(__m128));
			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
				p1fm=pp1[th/2];
				z2=lptr2[th2].zsa;
				yr2=lptr2[th2].yra;
				coef2=_mm_set1_ps(lptr2[th2].yr2a);
//				for(z=lptr1[th2].zsa,yr=lptr1[th2].yra,coef1=_mm_set1_ps(lptr1[th2].yr2a);z<=lptr1[th2].zea;z++,yr++){
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//				}
//				for(z=lptr2[th2].zsa,yr=lptr2[th2].yra,coef1=_mm_set1_ps(lptr2[th2].yr2a);z<=lptr2[th2].zea;z++,yr++){
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//				}
				for(z=lptr1[th2].zsa,yr=lptr1[th2].yra,coef1=_mm_set1_ps(lptr1[th2].yr2a);z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
					i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
				}
			}

zero_mask_flag_label:

			if(view_90flag==0){
				i1f=image[x][y];
				i2f=image[x][y_pixels-y];
				i3f=image[x_pixels-x][y];
				i4f=image[x_pixels-x][y_pixels-y];
				if(x!=x_pixels/2){
					if(zero_mask_flag){
						memset(i1f,0,z_pixels_simd*sizeof(__m128));
						memset(i2f,0,z_pixels_simd*sizeof(__m128));
						memset(i3f,0,z_pixels_simd*sizeof(__m128));
						memset(i4f,0,z_pixels_simd*sizeof(__m128));
					}
					else{
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]=mptr1[yr]+mptr2[yr+1];
							i2f[z]=mptr1[yr+1]+mptr2[yr];
							i3f[z]=mptr1[yr+2]+mptr2[yr+3];
							i4f[z]=mptr1[yr+3]+mptr2[yr+2];
						}
					}

				}else {
					if(zero_mask_flag){
						memset(i1f,0,z_pixels_simd*sizeof(__m128));
						memset(i2f,0,z_pixels_simd*sizeof(__m128));
					}
					else{
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]=mptr1[yr]+mptr2[yr+1];
							i2f[z]=mptr1[yr+1]+mptr2[yr];
						}
					}
				}
			} else if(!zero_mask_flag){
				i1f=image[y_pixels-y][x];
				i2f=image[y][x];
				i3f=image[y_pixels-y][x_pixels-x];
				i4f=image[y][x_pixels-x];
				if(x!=x_pixels/2){
					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
						i1f[z]+=mptr1[yr]+mptr2[yr+1];
						i2f[z]+=mptr1[yr+1]+mptr2[yr];
						i3f[z]+=mptr1[yr+2]+mptr2[yr+3];
						i4f[z]+=mptr1[yr+3]+mptr2[yr+2];
					}
				}else {
					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
						i1f[z]+=mptr1[yr]+mptr2[yr+1];
						i2f[z]+=mptr1[yr+1]+mptr2[yr];
					}
				}
			}
		}

		y=y_pixels/2;
		p1fm=(__m128 *)pp1[0];
		memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
			p1fm=(__m128 *)pp1[th/2];
			for(z=yr_bottom[th];z<=yr_top[th];z++){
				i1fmm[z]=_mm_add_ps(i1fmm[z],p1fm[z]);//_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
			}
		}
		if(view_90flag==0){
			i1f=image[x][y];
			i4f=image[x_pixels-x][y];
			if(x!=x_pixels/2){
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]=mptr1[yr]+mptr1[yr+1];
					i4f[z]=mptr1[yr+2]+mptr1[yr+3];
				}
			} else {
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]=mptr1[yr]+mptr1[yr+1];
				}
			}
		} else {
			i1f=image[y_pixels-y][x];
			i4f=image[y][x_pixels-x];
			if(x!=x_pixels/2){
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]+=mptr1[yr]+mptr1[yr+1];
					i4f[z]+=mptr1[yr+2]+mptr1[yr+3];
				}
			} else {
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]+=mptr1[yr]+mptr1[yr+1];
				}
			}
		}
	}
	return 1;
}
/*
int back_proj3d_view_n1_old(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
{
Backprojection_lookup lptr1[MAXGROUP];
Backprojection_lookup lptr2[MAXGROUP];
int    th,th2;
register int    yr,yr2, z,z2;
int    x, y;		// indice of the 3 dimensions in reconstructed image 
int ystart,yend;
float  *i1f,*i2f,*i3f,*i4f;
__m128 *p1fm;
float  **pp1;
//	int y_off=y_pixels/2;
__m128 coef1,coef2;
__m128 *i1fmm;
__m128 *i2fmm;
int zero_mask_flag=0;
float *mptr1,*mptr2;
int z_start=0,z_end=z_pixels;

i1fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
i2fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
if(i2fmm==NULL || i1fmm==NULL){
LOG_EXIT("exiting");
}

mptr1=(float *) i1fmm;
mptr2=(float *) i2fmm;
//	LOG_INFO("backview =%d\n",view);
for (x = x_start; x < x_end; x++) {
if(view_90flag==0) pp1=prj[x];
else	pp1=prj[x+xr_pixels/2];
pp1=prj[x];
ystart=cylwiny[x][0];
yend=cylwiny[x][1];
if(ystart==0 && yend==0) continue;
zero_mask_flag=0;
if(norm_zero_mask[view][x]==0){
zero_mask_flag=1;
}
for (y = ystart; y <y_pixels/2;y++){// yend; y++) {
if(zero_mask_flag) goto zero_mask_flag_label;
p1fm=(__m128 *)pp1[0];
if(y==y_pixels/2) continue;
memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
memcpy(i2fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
for(th=2,th2=0;th<th_pixels;th+=2,th2++){
p1fm=(__m128 *)pp1[th/2];
z2=lptr2[th2].zsa;
yr2=lptr2[th2].yra;
coef2=_mm_set1_ps(lptr2[th2].yr2a);
for(z=lptr1[th2].zsa,yr=lptr1[th2].yra,coef1=_mm_set1_ps(lptr1[th2].yr2a);z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
}
}

zero_mask_flag_label:

if(view_90flag==0){
i1f=image[x][y];
i2f=image[x][y_pixels-y];
i3f=image[x_pixels-x][y];
i4f=image[x_pixels-x][y_pixels-y];
if(x!=x_pixels/2){
if(zero_mask_flag){
memset(i1f,0,z_pixels_simd*sizeof(__m128));
memset(i2f,0,z_pixels_simd*sizeof(__m128));
memset(i3f,0,z_pixels_simd*sizeof(__m128));
memset(i4f,0,z_pixels_simd*sizeof(__m128));
}
else{
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]=mptr1[yr]+mptr2[yr+1];
i2f[z]=mptr1[yr+1]+mptr2[yr];
i3f[z]=mptr1[yr+2]+mptr2[yr+3];
i4f[z]=mptr1[yr+3]+mptr2[yr+2];
}
}

}else {
if(zero_mask_flag){
memset(i1f,0,z_pixels_simd*sizeof(__m128));
memset(i2f,0,z_pixels_simd*sizeof(__m128));
}
else{
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]=mptr1[yr]+mptr2[yr+1];
i2f[z]=mptr1[yr+1]+mptr2[yr];
}
}
}
} else if(!zero_mask_flag){
i1f=image[y_pixels-y][x];
i2f=image[y][x];
i3f=image[y_pixels-y][x_pixels-x];
i4f=image[y][x_pixels-x];
if(x!=x_pixels/2){
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]+=mptr1[yr]+mptr2[yr+1];
i2f[z]+=mptr1[yr+1]+mptr2[yr];
i3f[z]+=mptr1[yr+2]+mptr2[yr+3];
i4f[z]+=mptr1[yr+3]+mptr2[yr+2];
}
}else {
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]+=mptr1[yr]+mptr2[yr+1];
i2f[z]+=mptr1[yr+1]+mptr2[yr];
}
}
}
}

y=y_pixels/2;
p1fm=(__m128 *)pp1[0];
memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
for(th=2,th2=0;th<th_pixels;th+=2,th2++){
p1fm=(__m128 *)pp1[th/2];
for(z=z_start;z<z_end;z++){
i1fmm[z]=_mm_add_ps(i1fmm[z],p1fm[z]);//_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
}
}
if(view_90flag==0){
i1f=image[x][y];
i4f=image[x_pixels-x][y];
if(x!=x_pixels/2){
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]=mptr1[yr]+mptr1[yr+1];
i4f[z]=mptr1[yr+2]+mptr1[yr+3];
}
} else {
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]=mptr1[yr]+mptr1[yr+1];
}
}
} else {
i1f=image[y_pixels-y][x];
i4f=image[y][x_pixels-x];
if(x!=x_pixels/2){
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]+=mptr1[yr]+mptr1[yr+1];
i4f[z]+=mptr1[yr+2]+mptr1[yr+3];
}
} else {
for(z=z_start,yr=0;z<z_end;z++,yr+=4){
i1f[z]+=mptr1[yr]+mptr1[yr+1];
}
}
}
}
//	LOG_INFO("xstart end %d\t%d\n",x_start,x_end);
return 1;
}

*/
//int back_proj3d_view_n1thread_new2(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup lptr1[MAXGROUP];
//	Backprojection_lookup lptr2[MAXGROUP];
//	int		th,th2;
//	register int    yr, z,z2;
//	int		x, y;		/* indice of the 3 dimensions in reconstructed image */
//	int		ystart,yend;
//	float	*i1f,*i2f,*i3f,*i4f;
//	float	*i5f,*i6f,*i7f,*i8f;
//	float	*p1fm;
//	float	*p2fm;
//	float	**pp1;
//	float	**pp2;
//	int		img_center=x_pixels/2;
//	float	coef1,coef2,coef3;
//	//	__m128	*i1fmm;
//	//	__m128	*i2fmm;
//	int		z_start=0,z_end=z_pixels;
//	int zero_mask_flag=0;
//	float *sum1;
//	float *sum2;
//	float *diff1;
//	float *diff2;
//	float *s1[4],*s2[4],*d1[4],*d2[4];
//	float *p1[4],*p2[4];
//	int numdiff[46];
//	int zdiff[46];
//	float r1[46],tan;
//	int i;
//	__m128 *mptr[8];
//	__m128 *mptr2[8];
//	__m128 *msum1,*msum2,*mdif1,*mdif2,mcoef;
//
//	sum1=(float *)_alloca((z_pixels_simd*16)*sizeof(float));
//	sum2=(float *)_alloca((z_pixels_simd)*16*sizeof(float));
//	diff1=(float *)_alloca((z_pixels_simd*16)*sizeof(float));
//	diff2=(float *)_alloca((z_pixels_simd*16)*sizeof(float));
//
//	if(sum1==NULL || sum2==NULL){
//		LOG_EXIT("exiting");
//	}
////	mptr1=(float *) sum1;
////	mptr2=(float *) sum2;
//	msum1=(__m128 *)sum1;msum2=(__m128 *)sum2;mdif1=(__m128 *)diff1;mdif2=(__m128 *)diff2;
//	for(i=0;i<4;i++){
//		s1[i]=&sum1[i*z_pixels_simd*4];
//		s2[i]=&sum2[i*z_pixels_simd*4];
//		d1[i]=&diff1[i*z_pixels_simd*4];
//		d2[i]=&diff2[i*z_pixels_simd*4];
//	}
//
//	for (x = x_start; x < x_end; x++) {
//		pp1=prj[x];
//		pp2=prj[x+img_center];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//		zero_mask_flag=0;
//		if(norm_zero_mask[view][x]==0){
//			zero_mask_flag=1;
//			y=img_center;
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=img_center){
//				memset(i1f,0,z_pixels*sizeof(float));
//				memset(i2f,0,z_pixels*sizeof(float));
//				memset(i3f,0,z_pixels*sizeof(float));
//				memset(i4f,0,z_pixels*sizeof(float));
//			} else {
//				memset(i1f,0,z_pixels*sizeof(float));
//			}
//		} else {
//			p1fm=pp1[0];
//			p1[0]=&p1fm[0];			p1[1]=&p1fm[z_pixels];			p1[2]=&p1fm[z_pixels*2];			p1[3]=&p1fm[z_pixels*3];
//
//			memcpy(s1[0],p1[0],z_pixels*sizeof(float));
//			memcpy(s1[1],p1[1],z_pixels*sizeof(float));
//			memcpy(s1[2],p1[2],z_pixels*sizeof(float));
//			memcpy(s1[3],p1[3],z_pixels*sizeof(float));
//			memset(diff1,0,z_pixels_simd*4*sizeof(__m128));
//
//
//			for(th=2;th<th_pixels;th+=2){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				p1[0]=&p1fm[0];			p1[1]=&p1fm[z_pixels];			p1[2]=&p1fm[z_pixels*2];			p1[3]=&p1fm[z_pixels*3];
//				p2[0]=&p2fm[0];			p2[1]=&p2fm[z_pixels];			p2[2]=&p2fm[z_pixels*2];			p2[3]=&p2fm[z_pixels*3];
//				for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
//					s1[0][yr]+=p1[0][yr]+p2[0][yr];
//					s1[1][yr]+=p1[1][yr]+p2[1][yr];
//					s1[2][yr]+=p1[2][yr]+p2[2][yr];
//					s1[3][yr]+=p1[3][yr]+p2[3][yr];
//				}
//			}
//			y=img_center;
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=img_center){
//				for(z=z_start;z<z_end;z++){
//					i1f[z]=s1[0][z];
//					i2f[z]=s1[1][z];
//					i3f[z]=s1[2][z];
//					i4f[z]=s1[3][z];
//				}
//			} else {
//				for(z=z_start;z<z_end;z++){
//					i1f[z]=s1[0][z]+s1[2][z];
//				}
//			}
//
//
//			for(th=2;th<th_pixels;th+=2){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				r1[th]=0;
//				r1[th-1]=0;
//				numdiff[th]=0;
//				numdiff[th-1]=0;
//				zdiff[th]=yr_bottom[th]-1;
//				zdiff[th-1]=yr_bottom[th]-1;
//				tan=-sin_theta[th]/cos_theta[th];
//				coef2=tan;
//				p1[0]=&p1fm[0];			p1[1]=&p1fm[z_pixels];			p1[2]=&p1fm[z_pixels*2];			p1[3]=&p1fm[z_pixels*3];
//				p2[0]=&p2fm[0];			p2[1]=&p2fm[z_pixels];			p2[2]=&p2fm[z_pixels*2];			p2[3]=&p2fm[z_pixels*3];
//				for(i=0;i<4;i++){
//					for(yr=yr_bottom[th]-1;yr<=yr_top[th];yr++){
//						d1[i][yr]+=(p2[i][yr+1]+p1[i][yr]-p2[i][yr]-p1[i][yr+1])*coef2;	
//						p1[i][yr]=p1[i][yr+1]-p1[i][yr];
//						p2[i][yr]=p2[i][yr+1]-p2[i][yr];
//					}
//				}
//			}
//			memcpy(sum2,sum1,z_pixels_simd*4*sizeof(__m128));
//			memcpy(diff2,diff1,z_pixels_simd*4*sizeof(__m128));
//
//		}
//		for (y = img_center-1; y >=ystart;y--){// yend; y++) {
//
//			if(zero_mask_flag) goto norm_zero_mask_label;
//			p1fm=pp1[0];
//			if(y==img_center) continue;
//			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
//			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				p1[0]=&p1fm[0];			p1[1]=&p1fm[z_pixels];			p1[2]=&p1fm[z_pixels*2];			p1[3]=&p1fm[z_pixels*3];
//				p2[0]=&p2fm[0];			p2[1]=&p2fm[z_pixels];			p2[2]=&p2fm[z_pixels*2];			p2[3]=&p2fm[z_pixels*3];
//				numdiff[th]++;
//				numdiff[th-1]++;
//				if(zdiff[th]!=lptr1[th2].zsa){
//					z=lptr1[th2].zsa;
//					tan=-sin_theta[th]/cos_theta[th];
//
//					coef2=tan;
//					coef1=(lptr1[th2].yr2a-tan);
//					coef3=(-(r1[th]-1+(numdiff[th]-1)*tan));
//					for(i=0;i<4;i++){
//
//						yr=yr_bottom[th]-1;
//						z=lptr1[th2].zsa;
//
//						d1[i][z]-=p1[i][yr]*coef2;
//						s1[i][z]+=coef1*p1[i][yr];
//						d2[i][z]+=p2[i][yr]*coef2;
//						s2[i][z]+=coef1*p2[i][yr];
//
//						z++;
//						for(;yr<yr_top[th];yr++,z++){
//							d1[i][z]+=coef2*(p1[i][yr]-p1[i][yr+1]);
//							d2[i][z]+=coef2*(p2[i][yr+1]-p2[i][yr]);
//							s1[i][z]+=coef1*p1[i][yr+1]+p1[i][yr]*coef3;
//							s2[i][z]+=coef1*p2[i][yr+1]+p2[i][yr]*coef3;
//						}
//						d1[i][z]+=p1[i][yr]*coef2;
//						s1[i][z]+=p1[i][yr]*coef3;
//						d2[i][z]-=p2[i][yr]*coef2;
//						s2[i][z]+=p2[i][yr]*coef3;
//					}
//					numdiff[th]=0;;
//					r1[th]=lptr1[th2].yr2a;
//					zdiff[th]=lptr1[th2].zsa;
//				}
//				if(zdiff[th-1]!=lptr2[th2].zsa){
//
//					z2=zdiff[th-1];
//					tan=-sin_theta[th]/cos_theta[th];
//
//					coef1=(lptr2[th2].yr2a-1+tan);
//					coef2=(tan);
//					coef3=(-r1[th-1]+(numdiff[th-1]-1)*tan);
//					for(i=0;i<4;i++){					
//						z2=zdiff[th-1];
//						yr=yr_bottom[th]-1;
//						d1[i][z2]-=p2[i][yr]*coef2;
//						d2[i][z2]+=p1[i][yr]*coef2;
//						s1[i][z2]+=p2[i][yr]*coef3;
//						s2[i][z2]+=p1[i][yr]*coef3;
//
//						z2=lptr2[th2].zsa;
//
//						for(;yr<yr_top[th];yr++,z2++){
//							d1[i][z2]+=coef2*(p2[i][yr]-p2[i][yr+1]);
//							d2[i][z2]+=coef2*(p1[i][yr+1]-p1[i][yr]);
//							s1[i][z2]+=p2[i][yr]*coef1+p2[i][yr+1]*coef3;
//							s2[i][z2]+=p1[i][yr]*coef1+p1[i][yr+1]*coef3;
//						}
//
//						d1[i][z2]+=p2[i][yr]*coef2;
//						s1[i][z2]+=coef1*p2[i][yr];
//						d2[i][z2]-=p1[i][yr]*coef2;
//						s2[i][z2]+=coef1*p1[i][yr];
//					}
//					numdiff[th-1]=0;
//					r1[th-1]=lptr2[th2].yr2a;
//					zdiff[th-1]=lptr2[th2].zsa;
//				}
//
//			}
//
//			
//			coef1=0;
//			mcoef=_mm_set_ps1(coef1);
//			for(z=0;z<z_pixels;z++){
//				msum1[z]=_mm_max_ps(mcoef,_mm_sub_ps(msum1[z],mdif1[z]));
//				msum2[z]=_mm_max_ps(mcoef,_mm_add_ps(msum2[z],mdif2[z]));
//			}
//
//norm_zero_mask_label:			;
//
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y_pixels-y][x];
//			i4f=image[y_pixels-y][x_pixels-x];
//			i5f=image[x][y_pixels-y];
//			i6f=image[x_pixels-x][y_pixels-y];
//			i7f=image[y][x];
//			i8f=image[y][x_pixels-x];
//			mptr[0]=(__m128 *) i1f;
//			mptr[1]=(__m128 *) i2f;
//			mptr[2]=(__m128 *) i3f;
//			mptr[3]=(__m128 *) i4f;
//			mptr[4]=(__m128 *) i5f;
//			mptr[5]=(__m128 *) i6f;
//			mptr[6]=(__m128 *) i7f;
//			mptr[7]=(__m128 *) i8f;
//			mptr2[0] =(__m128 *) s1[0];
//			mptr2[1] =(__m128 *) s1[1];
//			mptr2[2] =(__m128 *) s1[2];
//			mptr2[3] =(__m128 *) s1[3];
//			mptr2[4] =(__m128 *) s2[0];
//			mptr2[5] =(__m128 *) s2[1];
//			mptr2[6] =(__m128 *) s2[2];
//			mptr2[7] =(__m128 *) s2[3];
//
//			if(x!=img_center ){
//				if(x==y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						for(z=0;z<z_pixels_simd;z++){
//							mptr[0][z]=_mm_add_ps(mptr2[0][z],mptr2[6][z]);
//							mptr[1][z]=_mm_add_ps(mptr2[1][z],mptr2[2][z]);
//							mptr[4][z]=_mm_add_ps(mptr2[4][z],mptr2[7][z]);
//							mptr[5][z]=_mm_add_ps(mptr2[3][z],mptr2[5][z]);
//						}
//
//					}
//				} else if(x<y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i3f,0,z_pixels_simd*sizeof(__m128));
//						memset(i4f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//						memset(i7f,0,z_pixels_simd*sizeof(__m128));
//						memset(i8f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						memcpy(i1f,s1[0],z_pixels_simd*sizeof(__m128));
//						memcpy(i2f,s1[1],z_pixels_simd*sizeof(__m128));
//						memcpy(i3f,s1[2],z_pixels_simd*sizeof(__m128));
//						memcpy(i4f,s1[3],z_pixels_simd*sizeof(__m128));
//						memcpy(i5f,s2[0],z_pixels_simd*sizeof(__m128));
//						memcpy(i6f,s2[1],z_pixels_simd*sizeof(__m128));
//						memcpy(i7f,s2[2],z_pixels_simd*sizeof(__m128));
//						memcpy(i8f,s2[3],z_pixels_simd*sizeof(__m128));
//					}
//				} else {
//					if(!zero_mask_flag){
////						for(i=0;i<8;i++){
//						for(z=0;z<z_pixels_simd;z++){
////							mptr[i][z]=_mm_add_ps(mptr[i][z],mptr2[i][z]);
//							mptr[0][z]=_mm_add_ps(mptr[0][z],mptr2[0][z]);
//							mptr[1][z]=_mm_add_ps(mptr[1][z],mptr2[1][z]);
//							mptr[2][z]=_mm_add_ps(mptr[2][z],mptr2[2][z]);
//							mptr[3][z]=_mm_add_ps(mptr[3][z],mptr2[3][z]);
//							mptr[4][z]=_mm_add_ps(mptr[4][z],mptr2[4][z]);
//							mptr[5][z]=_mm_add_ps(mptr[5][z],mptr2[5][z]);
//							mptr[6][z]=_mm_add_ps(mptr[6][z],mptr2[6][z]);
//							mptr[7][z]=_mm_add_ps(mptr[7][z],mptr2[7][z]);
////						}
//						}
//					}
//				}
//			} else {
//				if(!zero_mask_flag){
//					for(z=0;z<z_pixels_simd;z++){
//						mptr[0][z]=_mm_add_ps(mptr[0][z],mptr2[0][z]);
//						mptr[2][z]=_mm_add_ps(mptr[2][z],mptr2[2][z]);
//						mptr[4][z]=_mm_add_ps(mptr[4][z],mptr2[4][z]);
//						mptr[6][z]=_mm_add_ps(mptr[6][z],mptr2[6][z]);
//					}
//				}
//			}		
//		}
//
//		coef1=0;
//		for(th=2;th<th_pixels;th+=2){
//			p1fm=pp1[th/2];
//			p2fm=pp2[th/2];
//			p1[0]=&p1fm[0];			p1[1]=&p1fm[z_pixels];			p1[2]=&p1fm[z_pixels*2];			p1[3]=&p1fm[z_pixels*3];
//			p2[0]=&p2fm[0];			p2[1]=&p2fm[z_pixels];			p2[2]=&p2fm[z_pixels*2];			p2[3]=&p2fm[z_pixels*3];
//
//			for(i=0;i<4;i++){
//				p1[i][yr_bottom[th]-1]=coef1;
//				p1[i][yr_top[th]+1]=coef1;
//				p2[i][yr_bottom[th]-1]=coef1;
//				p2[i][yr_top[th]+1]=coef1;
//			}
//		}
//	}
//	return 1;
//}
//
//int back_proj3d_view_n1thread_new_original(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup lptr1[MAXGROUP];
//	Backprojection_lookup lptr2[MAXGROUP];
//	int		th,th2;
//	register int    yr, z,z2;
//	int		x, y;		/* indice of the 3 dimensions in reconstructed image */
//	int		ystart,yend;
//	float	*i1f,*i2f,*i3f,*i4f;
//	float	*i5f,*i6f,*i7f,*i8f;
//	__m128	*p1fm;
//	__m128	*p2fm;
//	__m128	**pp1;
//	__m128	**pp2;
//	int		img_center=x_pixels/2;
//	__m128	coef1,coef2,coef3;
//	//	__m128	*i1fmm;
//	//	__m128	*i2fmm;
//	float	*mptr1,*mptr2;
//	int		z_start=0,z_end=z_pixels;
//	int zero_mask_flag=0;
//	__m128 *sum1;
//	__m128 *sum2;
//	__m128 *diff1;
//	__m128 *diff2;
//	int numdiff[46];
//	int zdiff[46];
//	float r1[46],tan;
//
//	sum1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	sum2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	diff1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	diff2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//
//	if(sum1==NULL || sum2==NULL){
//		LOG_EXIT("exiting");
//	}
//	mptr1=(float *) sum1;
//	mptr2=(float *) sum2;
//
//	for (x = x_start; x < x_end; x++) {
//		pp1=(__m128 **)prj[x];
//		pp2=(__m128 **)prj[x+img_center];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//		zero_mask_flag=0;
//		if(norm_zero_mask[view][x]==0){
//			zero_mask_flag=1;
//			y=img_center;
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=img_center){
//				memset(i1f,0,z_pixels_simd*4*sizeof(float));
//				memset(i2f,0,z_pixels_simd*4*sizeof(float));
//				memset(i3f,0,z_pixels_simd*4*sizeof(float));
//				memset(i4f,0,z_pixels_simd*4*sizeof(float));
//			} else {
//				memset(i1f,0,z_pixels_simd*4*sizeof(float));
//			}
//		} else {
//			memcpy(sum1,pp1[0],z_pixels*sizeof(__m128));
//			memset(diff1,0,z_pixels_simd*4*sizeof(__m128));
//
//			for(th=2;th<th_pixels;th+=2){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
//					sum1[yr]=_mm_add_ps(sum1[yr],_mm_add_ps(p1fm[yr],p2fm[yr]));
//				}
//			}
//			y=img_center;
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=img_center){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr];
//					i2f[z]=mptr1[yr+1];
//					i3f[z]=mptr1[yr+2];
//					i4f[z]=mptr1[yr+3];
//				}
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr]+mptr1[yr+2];
//				}
//			}
//
//			for(th=2;th<th_pixels;th+=2){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				r1[th]=0;
//				r1[th-1]=0;
//				numdiff[th]=0;
//				numdiff[th-1]=0;
//				zdiff[th]=yr_bottom[th]-1;
//				zdiff[th-1]=yr_bottom[th]-1;
//				tan=-sin_theta[th]/cos_theta[th];
//				coef2=_mm_set1_ps(tan);
//				coef1=_mm_set1_ps(1);
//				for(yr=yr_bottom[th]-1;yr<=yr_top[th];yr++){
//					diff1[yr]=_mm_sub_ps(diff1[yr],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2));
//					diff1[yr]=_mm_add_ps(diff1[yr],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2));
//					p1fm[yr]=_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef1);
//					p2fm[yr]=_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef1);
//				}
//			}
//			memcpy(sum2,sum1,z_pixels_simd*4*sizeof(__m128));
//			memcpy(diff2,diff1,z_pixels_simd*4*sizeof(__m128));
//		}
//
//		//		for (y = ystart; y <y_pixels/2;y++){// yend; y++) {
//		for (y = img_center-1; y >=ystart;y--){// yend; y++) {
//
//			if(zero_mask_flag) goto norm_zero_mask_label;
//			p1fm=pp1[0];
//			if(y==img_center) continue;
//			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
//			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				numdiff[th]++;
//				numdiff[th-1]++;
//				if(zdiff[th]!=lptr1[th2].zsa){
//					z=lptr1[th2].zsa;
//					//					z2=zdiff[th];
//					//		r2=lptr1[th2].yr2a;
//					tan=-sin_theta[th]/cos_theta[th];
//
//					coef2=_mm_set_ps1(tan);
//					coef1=_mm_set_ps1(lptr1[th2].yr2a-tan);
//					coef3=_mm_set_ps1(-(r1[th]-1+(numdiff[th]-1)*tan));
//
//					yr=yr_bottom[th]-1;
//					diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef2));
//					sum1[z]=_mm_add_ps(sum1[z],	_mm_mul_ps(coef1,p1fm[yr]));
//					diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef2));
//					sum2[z]=_mm_add_ps(sum2[z],	_mm_mul_ps(coef1,p2fm[yr]));
//					z++;
//
//					for(;yr<yr_top[th];yr++,z++){
//						diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr],p1fm[yr+1])));
//						diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr+1],p2fm[yr])));
//						sum1[z]=_mm_add_ps(sum1[z],_mm_add_ps(_mm_mul_ps(p1fm[yr+1],coef1),_mm_mul_ps(p1fm[yr],coef3)));
//						sum2[z]=_mm_add_ps(sum2[z],_mm_add_ps(_mm_mul_ps(p2fm[yr+1],coef1),_mm_mul_ps(p2fm[yr],coef3)));
//					}
//					diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef2));
//					sum1[z]=_mm_add_ps(sum1[z],_mm_mul_ps(coef3,p1fm[yr]));
//					diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef2));
//					sum2[z]=_mm_add_ps(sum2[z],_mm_mul_ps(coef3,p2fm[yr]));
//
//					numdiff[th]=0;;
//					r1[th]=lptr1[th2].yr2a;
//					zdiff[th]=lptr1[th2].zsa;
//				}
//				if(zdiff[th-1]!=lptr2[th2].zsa){
//
//					z2=zdiff[th-1];
//					tan=-sin_theta[th]/cos_theta[th];
//
//					coef1=_mm_set_ps1(lptr2[th2].yr2a-1+tan);
//					coef2=_mm_set_ps1(tan);
//					coef3=_mm_set_ps1(-r1[th-1]+(numdiff[th-1]-1)*tan);
//
//					yr=yr_bottom[th]-1;
//					diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef2));
//					diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef2));
//					sum1[z2]=_mm_add_ps(sum1[z2],_mm_mul_ps(coef3,p2fm[yr]));
//					sum2[z2]=_mm_add_ps(sum2[z2],_mm_mul_ps(coef3,p1fm[yr]));
//
//					z2=lptr2[th2].zsa;
//
//					for(;yr<yr_top[th];yr++,z2++){
//						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr],p2fm[yr+1])));
//						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr+1],p1fm[yr])));
//						sum1[z2]=_mm_add_ps(sum1[z2],_mm_add_ps(_mm_mul_ps(p2fm[yr],coef1),_mm_mul_ps(p2fm[yr+1],coef3)));
//						sum2[z2]=_mm_add_ps(sum2[z2],_mm_add_ps(_mm_mul_ps(p1fm[yr],coef1),_mm_mul_ps(p1fm[yr+1],coef3)));
//					}
//					diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef2));
//					sum1[z2]=_mm_add_ps(sum1[z2],	_mm_mul_ps(coef1,p2fm[yr]));
//					diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef2));
//					sum2[z2]=_mm_add_ps(sum2[z2],	_mm_mul_ps(coef1,p1fm[yr]));
//
//					numdiff[th-1]=0;
//					r1[th-1]=lptr2[th2].yr2a;
//					zdiff[th-1]=lptr2[th2].zsa;
//				}
//
//			}
//			coef1=_mm_set_ps1(0);
//			for(z=0;z<z_pixels;z++){
////				sum1[z]=_mm_max_ps(coef1,_mm_sub_ps(sum1[z],diff1[z]));
////				sum2[z]=_mm_max_ps(coef1,_mm_add_ps(sum2[z],diff2[z]));
//				sum1[z]=_mm_sub_ps(sum1[z],diff1[z]);
//				sum2[z]=_mm_add_ps(sum2[z],diff2[z]);
//			}
//norm_zero_mask_label:			;
//
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y_pixels-y][x];
//			i4f=image[y_pixels-y][x_pixels-x];
//			i5f=image[x][y_pixels-y];
//			i6f=image[x_pixels-x][y_pixels-y];
//			i7f=image[y][x];
//			i8f=image[y][x_pixels-x];
//
//			if(x!=img_center ){
//				if(x==y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr]+mptr2[yr+2];
//							i2f[z]=mptr1[yr+1]+mptr1[yr+2];
//							i5f[z]=mptr2[yr]+mptr2[yr+3];
//							i6f[z]=mptr1[yr+3]+mptr2[yr+1];
//						}
//					}
//				} else if(x<y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i3f,0,z_pixels_simd*sizeof(__m128));
//						memset(i4f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//						memset(i7f,0,z_pixels_simd*sizeof(__m128));
//						memset(i8f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr];
//							i2f[z]=mptr1[yr+1];
//							i3f[z]=mptr1[yr+2];
//							i4f[z]=mptr1[yr+3];
//							i5f[z]=mptr2[yr];
//							i6f[z]=mptr2[yr+1];
//							i7f[z]=mptr2[yr+2];
//							i8f[z]=mptr2[yr+3];
//						}
//					}
//				} else {
//					if(!zero_mask_flag){
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]+=mptr1[yr];
//							i2f[z]+=mptr1[yr+1];
//							i3f[z]+=mptr1[yr+2];
//							i4f[z]+=mptr1[yr+3];
//							i5f[z]+=mptr2[yr];
//							i6f[z]+=mptr2[yr+1];
//							i7f[z]+=mptr2[yr+2];
//							i8f[z]+=mptr2[yr+3];
//						}
//					}
//				}
//			} else {
//				if(!zero_mask_flag){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]+=mptr1[yr];
//						i3f[z]+=mptr1[yr+2];
//						i5f[z]+=mptr2[yr];
//						i7f[z]+=mptr2[yr+2];
//					}
//				}
//			}
//		}
//		for(th=2;th<th_pixels;th+=2){
//			p1fm=pp1[th/2];
//			p2fm=pp2[th/2];
//			coef1=_mm_set1_ps(0);
//			p1fm[yr_bottom[th]-1]=coef1;
//			p1fm[yr_top[th]+1]=coef1;
//			p2fm[yr_bottom[th]-1]=coef1;
//			p2fm[yr_top[th]+1]=coef1;
//		}
//	}
//
//	return 1;
//}

//int back_proj3d_view_n1thread_new_bug(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup lptr1[MAXGROUP];
//	Backprojection_lookup lptr2[MAXGROUP];
//	int		th,th2;
//	register int    yr, z,z2;
//	int		x, y;		/* indice of the 3 dimensions in reconstructed image */
//	int		ystart,yend;
//	float	*i1f,*i2f,*i3f,*i4f;
//	float	*i5f,*i6f,*i7f,*i8f;
//	__m128	*p1fm;
//	__m128	*p2fm;
//	__m128	**pp1;
//	__m128	**pp2;
//	int		img_center=x_pixels/2;
//	__m128	coef1,coef2,coef3;
//	//	__m128	*i1fmm;
//	//	__m128	*i2fmm;
//	float	*mptr1,*mptr2;
//	int		z_start=0,z_end=z_pixels;
//	int zero_mask_flag=0;
//	__m128 *sum1;
//	__m128 *sum2;
//	__m128 *diff1;
//	__m128 *diff2;
//	int numdiff[46];
//	int zdiff[46];
//	float r1[46],tan;
//
//	sum1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	sum2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	diff1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	diff2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//
//	if(sum1==NULL || sum2==NULL){
//		LOG_EXIT("exiting");
//	}
//	mptr1=(float *) sum1;
//	mptr2=(float *) sum2;
//
//	for (x = x_start; x < x_end; x++) {
//		pp1=(__m128 **)prj[x];
//		pp2=(__m128 **)prj[x+img_center];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//		zero_mask_flag=0;
//		if(norm_zero_mask[view][x]==0){
//			zero_mask_flag=1;
//			y=img_center;
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=img_center){
//				memset(i1f,0,z_pixels_simd*4*sizeof(float));
//				memset(i2f,0,z_pixels_simd*4*sizeof(float));
//				memset(i3f,0,z_pixels_simd*4*sizeof(float));
//				memset(i4f,0,z_pixels_simd*4*sizeof(float));
//			} else {
//				memset(i1f,0,z_pixels_simd*4*sizeof(float));
//			}
//		} else {
//			memcpy(sum1,pp1[0],z_pixels*sizeof(__m128));
//			memcpy(sum2,pp1[0],z_pixels*sizeof(__m128));
//			memset(diff1,0,z_pixels_simd*4*sizeof(__m128));
//			memset(diff2,0,z_pixels_simd*4*sizeof(__m128));
//
//			for(th=2;th<th_pixels;th+=2){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
//					sum1[yr]=_mm_add_ps(sum1[yr],_mm_add_ps(p1fm[yr],p2fm[yr]));
//				}
//			}
//			y=img_center;
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=img_center){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr];
//					i2f[z]=mptr1[yr+1];
//					i3f[z]=mptr1[yr+2];
//					i4f[z]=mptr1[yr+3];
//				}
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr]+mptr1[yr+2];
//				}
//			}
//
//			for(th=2;th<th_pixels;th+=2){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				r1[th]=0;
//				r1[th-1]=0;
//				numdiff[th]=0;
//				numdiff[th-1]=0;
//				zdiff[th]=yr_bottom[th]-1;
//				zdiff[th-1]=yr_bottom[th]-1;
//				tan=-sin_theta[th]/cos_theta[th];
//				coef2=_mm_set1_ps(tan);
//				coef1=_mm_set1_ps(1);
//				for(yr=yr_bottom[th]-1;yr<=yr_top[th];yr++){
//					diff1[yr]=_mm_sub_ps(diff1[yr],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2));
//					diff1[yr]=_mm_add_ps(diff1[yr],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2));
//					p1fm[yr]=_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef1);
//					p2fm[yr]=_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef1);
//				}
//			}
//			memcpy(sum2,sum1,z_pixels_simd*4*sizeof(__m128));
//			memcpy(diff2,diff1,z_pixels_simd*4*sizeof(__m128));
//		}
//	
//		for (y = img_center-1; y >=ystart;y--){// yend; y++) {
//
//			if(zero_mask_flag) goto norm_zero_mask_label;
//			p1fm=pp1[0];
//			if(y==img_center) continue;
//			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
//			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				numdiff[th]++;
//				numdiff[th-1]++;
//				if(zdiff[th]!=lptr1[th2].zsa){
//					z=lptr1[th2].zsa;
//					//					z2=zdiff[th];
//					//		r2=lptr1[th2].yr2a;
//					tan=-sin_theta[th]/cos_theta[th];
//
//					coef2=_mm_set_ps1(tan);
//					coef1=_mm_set_ps1(lptr1[th2].yr2a-tan);
//					coef3=_mm_set_ps1(-(r1[th]-1+(numdiff[th]-1)*tan));
//
//					yr=yr_bottom[th]-1;
//					diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef2));
//					sum1[z]=_mm_add_ps(sum1[z],	_mm_mul_ps(coef1,p1fm[yr]));
//					diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef2));
//					sum2[z]=_mm_add_ps(sum2[z],	_mm_mul_ps(coef1,p2fm[yr]));
//					z++;
//
//					for(;yr<yr_top[th];yr++,z++){
//						diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr],p1fm[yr+1])));
//						diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr+1],p2fm[yr])));
//						sum1[z]=_mm_add_ps(sum1[z],_mm_add_ps(_mm_mul_ps(p1fm[yr+1],coef1),_mm_mul_ps(p1fm[yr],coef3)));
//						sum2[z]=_mm_add_ps(sum2[z],_mm_add_ps(_mm_mul_ps(p2fm[yr+1],coef1),_mm_mul_ps(p2fm[yr],coef3)));
//					}
//					diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef2));
//					sum1[z]=_mm_add_ps(sum1[z],_mm_mul_ps(coef3,p1fm[yr]));
//					diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef2));
//					sum2[z]=_mm_add_ps(sum2[z],_mm_mul_ps(coef3,p2fm[yr]));
//
//					numdiff[th]=0;;
//					r1[th]=lptr1[th2].yr2a;
//					zdiff[th]=lptr1[th2].zsa;
//				}
//				if(zdiff[th-1]!=lptr2[th2].zsa){
//
//					z2=zdiff[th-1];
//					tan=-sin_theta[th]/cos_theta[th];
//
//					coef1=_mm_set_ps1(lptr2[th2].yr2a-1+tan);
//					coef2=_mm_set_ps1(tan);
//					coef3=_mm_set_ps1(-r1[th-1]+(numdiff[th-1]-1)*tan);
//
//					yr=yr_bottom[th]-1;
//					diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef2));
//					diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef2));
//					sum1[z2]=_mm_add_ps(sum1[z2],_mm_mul_ps(coef3,p2fm[yr]));
//					sum2[z2]=_mm_add_ps(sum2[z2],_mm_mul_ps(coef3,p1fm[yr]));
//
//					z2=lptr2[th2].zsa;
//
//					for(;yr<yr_top[th];yr++,z2++){
//						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr],p2fm[yr+1])));
//						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr+1],p1fm[yr])));
//						sum1[z2]=_mm_add_ps(sum1[z2],_mm_add_ps(_mm_mul_ps(p2fm[yr],coef1),_mm_mul_ps(p2fm[yr+1],coef3)));
//						sum2[z2]=_mm_add_ps(sum2[z2],_mm_add_ps(_mm_mul_ps(p1fm[yr],coef1),_mm_mul_ps(p1fm[yr+1],coef3)));
//					}
//					diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef2));
//					sum1[z2]=_mm_add_ps(sum1[z2],	_mm_mul_ps(coef1,p2fm[yr]));
//					diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef2));
//					sum2[z2]=_mm_add_ps(sum2[z2],	_mm_mul_ps(coef1,p1fm[yr]));
//
//					numdiff[th-1]=0;
//					r1[th-1]=lptr2[th2].yr2a;
//					zdiff[th-1]=lptr2[th2].zsa;
//				}
//
//			}
//			coef1=_mm_set_ps1(0);
//			for(z=0;z<z_pixels;z++){
////				sum1[z]=_mm_max_ps(coef1,_mm_sub_ps(sum1[z],diff1[z]));
////				sum2[z]=_mm_max_ps(coef1,_mm_add_ps(sum2[z],diff2[z]));
//				sum1[z]=_mm_sub_ps(sum1[z],diff1[z]);
//				sum2[z]=_mm_add_ps(sum2[z],diff2[z]);
//			}
//norm_zero_mask_label:			;
//
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y_pixels-y][x];
//			i4f=image[y_pixels-y][x_pixels-x];
//			i5f=image[x][y_pixels-y];
//			i6f=image[x_pixels-x][y_pixels-y];
//			i7f=image[y][x];
//			i8f=image[y][x_pixels-x];
//
//			if(x!=img_center ){
//				if(x==y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr]+mptr2[yr+2];
//							i2f[z]=mptr1[yr+1]+mptr1[yr+2];
//							i5f[z]=mptr2[yr]+mptr2[yr+3];
//							i6f[z]=mptr1[yr+3]+mptr2[yr+1];
//						}
//					}
//				} else if(x<y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i3f,0,z_pixels_simd*sizeof(__m128));
//						memset(i4f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//						memset(i7f,0,z_pixels_simd*sizeof(__m128));
//						memset(i8f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr];
//							i2f[z]=mptr1[yr+1];
//							i3f[z]=mptr1[yr+2];
//							i4f[z]=mptr1[yr+3];
//							i5f[z]=mptr2[yr];
//							i6f[z]=mptr2[yr+1];
//							i7f[z]=mptr2[yr+2];
//							i8f[z]=mptr2[yr+3];
//						}
//					}
//				} else {
//					if(!zero_mask_flag){
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]+=mptr1[yr];
//							i2f[z]+=mptr1[yr+1];
//							i3f[z]+=mptr1[yr+2];
//							i4f[z]+=mptr1[yr+3];
//							i5f[z]+=mptr2[yr];
//							i6f[z]+=mptr2[yr+1];
//							i7f[z]+=mptr2[yr+2];
//							i8f[z]+=mptr2[yr+3];
//						}
//					}
//				}
//			} else {
//				if(!zero_mask_flag){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]+=mptr1[yr];
//						i3f[z]+=mptr1[yr+2];
//						i5f[z]+=mptr2[yr];
//						i7f[z]+=mptr2[yr+2];
//					}
//				}
//			}
//		}
//		for(th=2;th<th_pixels;th+=2){
//			p1fm=pp1[th/2];
//			p2fm=pp2[th/2];
//			coef1=_mm_set1_ps(0);
//			p1fm[yr_bottom[th]-1]=coef1;
//			p1fm[yr_top[th]+1]=coef1;
//			p2fm[yr_bottom[th]-1]=coef1;
//			p2fm[yr_top[th]+1]=coef1;
//		}
//	}
//
//	return 1;
//}

int back_proj3d_view_n1thread(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
{
	Backprojection_lookup lptr1[MAXGROUP];
	Backprojection_lookup lptr2[MAXGROUP];
	short		th,th2;
	short    yr, z,z2;
	short		x, y;		/* indice of the 3 dimensions in reconstructed image */
	short		ystart,yend;
	float	*i1f,*i2f,*i3f,*i4f;
	float	*i5f,*i6f,*i7f,*i8f;
	__m128	*p1fm;
	__m128	*p2fm;
	__m128	**pp1;
	__m128	**pp2;
	short		img_center=x_pixels/2;
	__m128	coef1,coef2,coef3;
	float	*mptr1,*mptr2;
	short		z_start=0,z_end=z_pixels;
	short zero_mask_flag=0;
	__m128 *sumt1;
	__m128 *sumt2;
	__m128 *diff1;
	__m128 *diff2;
	__m128 m1;
	short yr2;
	float tan;
//	float coeft1,coeft2,coeft3,coeft4;
	short z3,z4,yr3,yr4;
	__m128 czero;

	czero=_mm_set_ps1(0);

	sumt1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	sumt2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));

	diff1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	diff2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));


	if(sumt1==NULL || sumt2==NULL){
		LOG_EXIT("exiting");
	}

	if(diff1==NULL || diff2==NULL){
		LOG_EXIT("exiting");
	}

	mptr1=(float *) sumt1;
	mptr2=(float *) sumt2;

	for (x = x_start; x < x_end; x++) {
		pp1=(__m128 **)prj[x];
		pp2=(__m128 **)prj[x+img_center];
		ystart=cylwiny[x][0];
		yend=cylwiny[x][1];
		if(ystart==0 && yend==0) continue;
//		ystart=cylwiny[x][0]-1;
//		yend=cylwiny[x][1]+1;
		zero_mask_flag=0;

		i1f=image[x][img_center];
		i2f=image[x_pixels-x][img_center];
		i3f=image[img_center][x];
		i4f=image[img_center][x_pixels-x];

		if(norm_zero_mask[view][x]==0){
			zero_mask_flag=1;
			if(x!=img_center){
				memset(i1f,0,z_pixels_simd*4*sizeof(float));
				memset(i2f,0,z_pixels_simd*4*sizeof(float));
				memset(i3f,0,z_pixels_simd*4*sizeof(float));
				memset(i4f,0,z_pixels_simd*4*sizeof(float));
			} else {
				memset(i1f,0,z_pixels_simd*4*sizeof(float));
			}
			y=img_center-1;
			i1f=image[x][y];
			i2f=image[x_pixels-x][y];
			i3f=image[y_pixels-y][x];
			i4f=image[y_pixels-y][x_pixels-x];
			i5f=image[x][y_pixels-y];
			i6f=image[x_pixels-x][y_pixels-y];
			i7f=image[y][x];
			i8f=image[y][x_pixels-x];
			if(x!=img_center ){
				if(x==y){
					memset(i1f,0,z_pixels_simd*sizeof(__m128));
					memset(i2f,0,z_pixels_simd*sizeof(__m128));
					memset(i5f,0,z_pixels_simd*sizeof(__m128));
					memset(i6f,0,z_pixels_simd*sizeof(__m128));
				} else if(x<y){
					memset(i1f,0,z_pixels_simd*sizeof(__m128));
					memset(i2f,0,z_pixels_simd*sizeof(__m128));
					memset(i3f,0,z_pixels_simd*sizeof(__m128));
					memset(i4f,0,z_pixels_simd*sizeof(__m128));
					memset(i5f,0,z_pixels_simd*sizeof(__m128));
					memset(i6f,0,z_pixels_simd*sizeof(__m128));
					memset(i7f,0,z_pixels_simd*sizeof(__m128));
					memset(i8f,0,z_pixels_simd*sizeof(__m128));
				} 
			}
		} else {

			memcpy(sumt1,pp1[0],z_pixels_simd*4*sizeof(__m128));
			for(th=2;th<th_pixels;th+=2){
				p1fm=pp1[th/2];
				p2fm=pp2[th/2];
				for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
					sumt1[yr]=_mm_add_ps(sumt1[yr],_mm_add_ps(p1fm[yr],p2fm[yr]));
				}
			}

			if(x!=img_center){
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]=mptr1[yr];
					i2f[z]=mptr1[yr+1];
					i3f[z]=mptr1[yr+2];
					i4f[z]=mptr1[yr+3];
				}
			} else {
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]=mptr1[yr]+mptr1[yr+2];
				}
			}			

			y=img_center-1;

			memset(diff1,0,z_pixels_simd*4*sizeof(__m128));
			memset(diff2,0,z_pixels_simd*4*sizeof(__m128));
			memcpy(sumt1,pp1[0],z_pixels_simd*4*sizeof(__m128));
			memcpy(sumt2,pp1[0],z_pixels_simd*4*sizeof(__m128));
			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
				p1fm=pp1[th/2];
				p2fm=pp2[th/2];
				yr=lptr1[th2].yra;
				coef1=_mm_set_ps1(lptr1[th2].yr2a);
				yr2=lptr2[th2].yra;
				z2=lptr2[th2].zsa;
				coef2=_mm_set_ps1(lptr2[th2].yr2a);
				coef3=_mm_set_ps1(0);
				yr=lptr1[th2].yra;

				i1f=(float*)p1fm;
//				LOG_INFO("z=%d\t%d\t%d\t%d\n",th2,lptr1[th2].zsa,lptr1[th2].zea,yr);
				for(z=lptr1[th2].zsa;z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
					sumt1[z]=_mm_add_ps(sumt1[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
					sumt2[z]=_mm_add_ps(sumt2[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
					sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
					sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
				}
				tan=-sin_theta[th]/cos_theta[th];
				coef2=_mm_set1_ps(tan);
				coef1=_mm_set1_ps(1);
			//	p1fm[yr_bottom[th]-1]=czero;
			//	p2fm[yr_bottom[th]-1]=czero;
				for(yr=yr_bottom[th]-1;yr<=yr_top[th];yr++){
					p1fm[yr]=_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2);
					p2fm[yr]=_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2);
					diff1[yr]=_mm_add_ps(diff1[yr],_mm_mul_ps(p1fm[yr],coef1));
					diff2[yr]=_mm_add_ps(diff2[yr],_mm_mul_ps(p2fm[yr],coef1));
//					diff1[yr]=_mm_add_ps(diff1[yr],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2));
//					diff2[yr]=_mm_add_ps(diff2[yr],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2));
				}
				z=blutacc[img_center+1][th2].z_start;
				yr=blutacc[img_center+1][th2].yr_start;
				for(;z<=blutacc[img_center+1][th2].z_end;yr++,z++){
					diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p2fm[yr],coef1));
					diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p1fm[yr],coef1));
//					diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2));
//					diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2));
				}
			}
		//	memcpy(sum1,sumt1,z_pixels_simd*4*sizeof(__m128));
		//	memcpy(sum2,sumt2,z_pixels_simd*4*sizeof(__m128));
			goto norm_zero_mask_label;
		}

		for(y=img_center-2;y>=ystart;y--){
			if(zero_mask_flag==1) goto norm_zero_mask_label;
		//	memcpy(sum1,pp1[0],z_pixels_simd*4*sizeof(__m128));
		//	memcpy(sum2,pp1[0],z_pixels_simd*4*sizeof(__m128));
		//	memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
		//	memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
				p1fm=pp1[th/2];
				p2fm=pp2[th/2];
		//		yr=lptr1[th2].yra;
		//		coef1=_mm_set_ps1(lptr1[th2].yr2a);
		//		yr2=lptr2[th2].yra;
		//		z2=lptr2[th2].zsa;
		//		coef2=_mm_set_ps1(lptr2[th2].yr2a);
		//		for(z=lptr1[th2].zsa;z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
		//			sum1[z]=_mm_add_ps(sum1[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
		//			sum2[z]=_mm_add_ps(sum2[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
		//			sum1[z2]=_mm_add_ps(sum1[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
		//			sum2[z2]=_mm_add_ps(sum2[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
		//		}

				
				if(blutacc_flag[y][th2]!=0 ){
					tan=-sin_theta[th]/cos_theta[th];
					tan=1-tan*(blutacc[y][th2].num)-blutacc[y][th2].coef_pre;
					coef1=_mm_set_ps1(-tan*cos_theta[th]/sin_theta[th]);
//					tan=sin_theta[th]/cos_theta[th];
//					tan=-tan*blutacc[y_pixels-y][th2].num-blutacc[y_pixels-y][th2].coef_pre;
//					coeft2=tan;
//					coef2=_mm_set_ps1(tan);
			//		coeft1=tan;

					z=blutacc[y][th2].z_start_pre;
					z2=blutacc[y_pixels-y][th2].z_start_pre;
					yr=blutacc[y][th2].yr_start_pre;
					yr2=blutacc[y_pixels-y][th2].yr_start_pre;

					z3=blutacc[y][th2].z_start;
					z4=blutacc[y_pixels-y][th2].z_start;
					yr3=blutacc[y][th2].yr_start;
					yr4=blutacc[y_pixels-y][th2].yr_start;

					coef3=_mm_set_ps1(-sin_theta[th]/cos_theta[th]);
					coef3=_mm_set_ps1(1);
			//		coeft2=-sin_theta[th]/cos_theta[th];
			//		coeft3=coeft1/coeft2;
			//		coeft3=coeft3*coeft2;
		//			if(fabs(coeft1+coeft2)>1e-6&&x==126){
			//		if(x==126){
//						LOG_INFO("%d\t%d\t%d\t%e\t%e\t%e\n",x,y,th2,coeft1,coeft2,coeft3);
			//		}
				//	if((z3-z)==-1 && x==126){
			//			LOG_INFO("%d\t%d\t%d\t%d\t%d\t%d\t%d\n",y,th2,z,z2,z3,z4,blutacc[y_pixels-y][th2].z_end);
				//	}
					if((z3-z)!=-1){
						for(;z<=blutacc[y][th2].z_end_pre;z++,z2++,yr++,yr2++){
							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
							sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr2],coef1));
							sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr2],coef1));
							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
							diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
							diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr2],coef3));
							diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr2],coef3));
							diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
						}
						for(;z3<=blutacc[y][th2].z_end;z3++,z4++,yr3++,yr4++){
							sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr3],coef1));
							sumt2[z4]=_mm_add_ps(sumt2[z4],_mm_mul_ps(p1fm[yr4],coef1));
							sumt1[z4]=_mm_add_ps(sumt1[z4],_mm_mul_ps(p2fm[yr4],coef1));
							sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr3],coef1));
							diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr3],coef3));
							diff2[z4]=_mm_sub_ps(diff2[z4],_mm_mul_ps(p1fm[yr4],coef3));
							diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr3],coef3));
							diff1[z4]=_mm_sub_ps(diff1[z4],_mm_mul_ps(p2fm[yr4],coef3));
						}
					} else {
						sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr],coef1));
						sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr],coef1));
						diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr],coef3));
						diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr],coef3));

						sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));
						sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));
						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));
						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));
						z2++;


						for(;z<blutacc[y][th2].z_end_pre;yr++,z++,z2++){//,z3++,z4++){
							m1=_mm_sub_ps(p1fm[yr],p1fm[yr+1]);
							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p1fm[yr],p1fm[yr+1]),coef1));
							sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p1fm[yr],p1fm[yr+1]),coef1));
							diff1[z]=_mm_sub_ps(diff1[z],m1);//_mm_sub_ps(p1fm[yr+1],p1fm[yr]));
							diff2[z2]=_mm_sub_ps(diff2[z2],m1);//_mm_sub_ps(p1fm[yr+1],p1fm[yr]));
							m1=_mm_sub_ps(p2fm[yr],p2fm[yr+1]);
							sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p2fm[yr],p2fm[yr+1]),coef1));
							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p2fm[yr],p2fm[yr+1]),coef1));
							diff2[z]=_mm_sub_ps(diff2[z],m1);//_mm_sub_ps(p2fm[yr+1],p2fm[yr]));
							diff1[z2]=_mm_sub_ps(diff1[z2],m1);//_mm_sub_ps(p2fm[yr+1],p2fm[yr]));


//							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
//							sumt1[z]=_mm_sub_ps(sumt1[z],_mm_mul_ps(p1fm[yr+1],coef1));

//							sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr+1],coef1));
//							sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));

//							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
//							sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr],coef1));

//							sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));
//							sumt2[z4]=_mm_add_ps(sumt2[z4],_mm_mul_ps(p1fm[yr],coef1));



//							sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr+1],coef1));
//							sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));

//							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
//							sumt2[z]=_mm_sub_ps(sumt2[z],_mm_mul_ps(p2fm[yr+1],coef1));


//							sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));
//							sumt1[z4]=_mm_add_ps(sumt1[z4],_mm_mul_ps(p2fm[yr],coef1));

//							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
//							sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr],coef1));

//							diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
//							diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(p1fm[yr+1],coef3));

//							diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr+1],coef3));
//							diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));


//							diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
//							diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr],coef3));

//							diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));
//							diff2[z4]=_mm_sub_ps(diff2[z4],_mm_mul_ps(p1fm[yr],coef3));


	//						m1=_mm_mul_ps(p2fm[yr],coef3);


//							diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
//							diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(p2fm[yr+1],coef3));

//							diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr+1],coef3));
//							diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));

//							diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));
//							diff1[z4]=_mm_sub_ps(diff1[z4],_mm_mul_ps(p2fm[yr],coef3));

//							diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
//							diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr],coef3));
						}
						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
						diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
						diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));

						sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));
						sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));
						diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));
						diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));

					}
/*					for(;z<=blutacc[y][th2].z_end_pre;z++,z2++,yr++,yr2++){
						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
						sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr2],coef1));
						sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr2],coef1));
						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
						diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr2],coef3));
						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr2],coef3));
						diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
//						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef1));
//						sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef2));
//						sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef2));
//						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef1));
//						diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef3));
//						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef3));
//						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef3));
//						diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef3));
					}
//					z=blutacc[y][th2].z_start;
//					z2=blutacc[y_pixels-y][th2].z_start;
//					yr=blutacc[y][th2].yr_start;
//					yr2=blutacc[y_pixels-y][th2].yr_start;
			//		coef1=_mm_set_ps1(blutacc[y][th2].coef+sin_theta[th]/cos_theta[th]);
			//		coeft3=blutacc[y][th2].coef+sin_theta[th]/cos_theta[th];
			//		coef2=_mm_set_ps1(blutacc[y_pixels-y][th2].coef-sin_theta[th]/cos_theta[th]-1);
			//		coeft2=blutacc[y_pixels-y][th2].coef-sin_theta[th]/cos_theta[th]-1;
			//		coef3=_mm_set_ps1(-sin_theta[th]/cos_theta[th]);
			//		if(fabs(coeft1+coeft3)>1e-5){
		//				LOG_INFO("2nd %d\t%d\t%d\t%f\t%f\t%e\n",x,y,th2,coeft1,coeft3,fabs(coeft1+coeft3));
		//			}
					for(;z3<=blutacc[y][th2].z_end;z3++,z4++,yr3++,yr4++){
						sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr3],coef1));
						sumt2[z4]=_mm_add_ps(sumt2[z4],_mm_mul_ps(p1fm[yr4],coef1));
						sumt1[z4]=_mm_add_ps(sumt1[z4],_mm_mul_ps(p2fm[yr4],coef1));
						sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr3],coef1));
						diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr3],coef3));
						diff2[z4]=_mm_sub_ps(diff2[z4],_mm_mul_ps(p1fm[yr4],coef3));
						diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr3],coef3));
						diff1[z4]=_mm_sub_ps(diff1[z4],_mm_mul_ps(p2fm[yr4],coef3));
//						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef1));
//						sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef2));
//						sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef2));
//						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef1));
//						diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef3));
//						diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef3));
//						diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef3));
//						diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef3));
					}*/
				} 
			} 
			for(z=0;z<z_pixels;z++){
				sumt1[z]=_mm_add_ps(sumt1[z],diff1[z]);
				sumt2[z]=_mm_add_ps(sumt2[z],diff2[z]);
			}
	//		for(z=0;z<z_pixels;z++){
	//			sumtt1[z]=_mm_sub_ps(sumt1[z],sum1[z]);
	//			sumtt2[z]=_mm_sub_ps(sumt2[z],sum2[z]);
	//		}
	//		i1f=(float *)sumtt2;
	//		tan=0;
	//		for(z=0;z<z_pixels*4;z+=4){
	//			tan+=fabs(i1f[z]);
	//		}
	//		LOG_INFO("y=%d\t%e\n",y,tan);
	/*		i1f=(float *) sum1;
			i2f=(float *) sumt1;
			i3f=(float *) sumtt1;
			i4f=(float *) diff1;
			i5f=(float *) pp1[0];
			if(y==128 && x==128){
				for(z=0;z<z_pixels*4;z+=4){
					LOG_INFO("%d\t%d\t%e\t%e\t%e\t%e\t%e\n",y,z/4,i1f[z],i2f[z],i3f[z],i4f[z],i5f[z]);
				}
			
			}
*/
norm_zero_mask_label:
			i1f=image[x][y];
			i2f=image[x_pixels-x][y];
			i3f=image[y_pixels-y][x];
			i4f=image[y_pixels-y][x_pixels-x];
			i5f=image[x][y_pixels-y];
			i6f=image[x_pixels-x][y_pixels-y];
			i7f=image[y][x];
			i8f=image[y][x_pixels-x];

			if(x!=img_center ){
				if(x==y){
					if(zero_mask_flag){
						memset(i1f,0,z_pixels_simd*sizeof(__m128));
						memset(i2f,0,z_pixels_simd*sizeof(__m128));
						memset(i5f,0,z_pixels_simd*sizeof(__m128));
						memset(i6f,0,z_pixels_simd*sizeof(__m128));
					} else {
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]=mptr1[yr]+mptr2[yr+2];
							i2f[z]=mptr1[yr+1]+mptr1[yr+2];
							i5f[z]=mptr2[yr]+mptr2[yr+3];
							i6f[z]=mptr1[yr+3]+mptr2[yr+1];
						}
					}
				} else if(x<y){
					if(zero_mask_flag){
						memset(i1f,0,z_pixels_simd*sizeof(__m128));
						memset(i2f,0,z_pixels_simd*sizeof(__m128));
						memset(i3f,0,z_pixels_simd*sizeof(__m128));
						memset(i4f,0,z_pixels_simd*sizeof(__m128));
						memset(i5f,0,z_pixels_simd*sizeof(__m128));
						memset(i6f,0,z_pixels_simd*sizeof(__m128));
						memset(i7f,0,z_pixels_simd*sizeof(__m128));
						memset(i8f,0,z_pixels_simd*sizeof(__m128));
					} else {
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]=mptr1[yr];
							i2f[z]=mptr1[yr+1];
							i3f[z]=mptr1[yr+2];
							i4f[z]=mptr1[yr+3];
							i5f[z]=mptr2[yr];
							i6f[z]=mptr2[yr+1];
							i7f[z]=mptr2[yr+2];
							i8f[z]=mptr2[yr+3];
						}
					}
				} else {
					if(!zero_mask_flag){
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]+=mptr1[yr];
							i2f[z]+=mptr1[yr+1];
							i3f[z]+=mptr1[yr+2];
							i4f[z]+=mptr1[yr+3];
							i5f[z]+=mptr2[yr];
							i6f[z]+=mptr2[yr+1];
							i7f[z]+=mptr2[yr+2];
							i8f[z]+=mptr2[yr+3];
						}
					}
				}
			} else {
				if(!zero_mask_flag){
					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
						i1f[z]+=mptr1[yr];
						i3f[z]+=mptr1[yr+2];
						i5f[z]+=mptr2[yr];
						i7f[z]+=mptr2[yr+2];
					}
				}
			}
		}
		for(th=2;th<th_pixels;th+=2){
			p1fm=pp1[th/2];
			p2fm=pp2[th/2];
			coef1=_mm_set1_ps(0);
			p1fm[yr_bottom[th]-1]=coef1;
			p1fm[yr_top[th]+1]=coef1;
			p2fm[yr_bottom[th]-1]=coef1;
			p2fm[yr_top[th]+1]=coef1;
		}
	}
	return 1;
}

//int back_proj3d_view_n1thread_lor(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup lptr1[MAXGROUP];
//	Backprojection_lookup lptr2[MAXGROUP];
//	short		th,th2;
//	short    yr, z,z2;
//	short		x, y;		/* indice of the 3 dimensions in reconstructed image */
//	short		ystart,yend;
//	float	*i1f,*i2f,*i3f,*i4f;
//	float	*i5f,*i6f,*i7f,*i8f;
//	__m128	*p1fm;
//	__m128	*p2fm;
//	__m128	**pp1;
//	__m128	**pp2;
//	short		img_center=x_pixels/2;
//	__m128	coef1,coef2,coef3;
//	float	*mptr1,*mptr2;
//	short		z_start=0,z_end=z_pixels;
//	short zero_mask_flag=0;
//	__m128 *sumt1;
//	__m128 *sumt2;
//	__m128 *diff1;
//	__m128 *diff2;
//	__m128 m1;
//	short yr2;
//	float tan;
////	float coeft1,coeft2,coeft3,coeft4;
//	short z3,z4,yr3,yr4;
//	float cosv1,sinv1,cosv2,sinv2;
//	float *i1,*i2,*i3,*i4,*i5,*i6,*i7,*i8;	
//	__m128 *i1m,*i2m,*i3m,*i4m,*i5m,*i6m,*i7m,*i8m;
//	__m128 *imptr;
//	__m128 coef4;
//	float rx,ry;
//	float xx,yy;
//	int xi,yi;
//	int i;
//
//	
//
//	cosv1=cos_psi[view];
//	sinv1=sin_psi[view];
//
//	i1=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i2=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i3=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i4=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i5=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i6=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i7=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i8=(float *) _alloca((z_pixels_simd)*sizeof(__m128));
//	i1m=i1;
//	i2m=i2;
//	i3m=i3;
//	i4m=i4;
//	i5m=i5;
//	i6m=i6;
//	i7m=i7;
//	i8m=i8;
//
//	sumt1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	sumt2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//
//	diff1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	diff2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//
//
//	if(sumt1==NULL || sumt2==NULL){
//		LOG_EXIT("exiting");
//	}
//
//	if(diff1==NULL || diff2==NULL){
//		LOG_EXIT("exiting");
//	}
//
//	mptr1=(float *) sumt1;
//	mptr2=(float *) sumt2;
//
//	for (x = x_start; x < x_end; x++) {
//		pp1=(__m128 **)prj[x];
//		pp2=(__m128 **)prj[x+img_center];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//		zero_mask_flag=0;
//
//
//
//		if(norm_zero_mask[view][x]==0){
//			continue;
//			zero_mask_flag=1;
//			if(x!=img_center){
//				memset(i1f,0,z_pixels_simd*4*sizeof(float));
//				memset(i2f,0,z_pixels_simd*4*sizeof(float));
//				memset(i3f,0,z_pixels_simd*4*sizeof(float));
//				memset(i4f,0,z_pixels_simd*4*sizeof(float));
//			} else {
//				memset(i1f,0,z_pixels_simd*4*sizeof(float));
//			}
//			y=img_center-1;
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y_pixels-y][x];
//			i4f=image[y_pixels-y][x_pixels-x];
//			i5f=image[x][y_pixels-y];
//			i6f=image[x_pixels-x][y_pixels-y];
//			i7f=image[y][x];
//			i8f=image[y][x_pixels-x];
//			if(x!=img_center ){
//				if(x==y){
//					memset(i1f,0,z_pixels_simd*sizeof(__m128));
//					memset(i2f,0,z_pixels_simd*sizeof(__m128));
//					memset(i5f,0,z_pixels_simd*sizeof(__m128));
//					memset(i6f,0,z_pixels_simd*sizeof(__m128));
//				} else if(x<y){
//					memset(i1f,0,z_pixels_simd*sizeof(__m128));
//					memset(i2f,0,z_pixels_simd*sizeof(__m128));
//					memset(i3f,0,z_pixels_simd*sizeof(__m128));
//					memset(i4f,0,z_pixels_simd*sizeof(__m128));
//					memset(i5f,0,z_pixels_simd*sizeof(__m128));
//					memset(i6f,0,z_pixels_simd*sizeof(__m128));
//					memset(i7f,0,z_pixels_simd*sizeof(__m128));
//					memset(i8f,0,z_pixels_simd*sizeof(__m128));
//				} 
//			}
//		} else {
//
//			memcpy(sumt1,pp1[0],z_pixels_simd*4*sizeof(__m128));
//			for(th=2;th<th_pixels;th+=2){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
//					sumt1[yr]=_mm_add_ps(sumt1[yr],_mm_add_ps(p1fm[yr],p2fm[yr]));
//				}
//			}
////			i1f=image[x][img_center];
////			i2f=image[x_pixels-x][img_center];
////			i3f=image[img_center][x];
////			i4f=image[img_center][x_pixels-x];
//			if(x!=img_center){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1[z]=mptr1[yr];
//					i2[z]=mptr1[yr+1];
//					i3[z]=mptr1[yr+2];
//					i4[z]=mptr1[yr+3];
//				}
//				for(i=0;i<4;i++){
//					if(i==0){ xi=x-img_center;yi=0;imptr=i1;}
//					else if(i==1){ xi=x_pixels-x-img_center;yi=0;imptr=i2;}
//					else if(i==2){ xi=0;yi=x-img_center;imptr=i3;}
//					else {xi=0;yi=x-img_center;imptr=i4;}
//
//					xx=xi*cosv1-yi*sinv1+img_center;
//					yy=xi*sinv1+yi*cosv1+img_center;
//					xi=xx;				yi=yy;
//					rx=xx-xi;			ry=yy-yi;
//					coef1=_mm_set_ps1((1-rx)*(1-ry));
//					coef2=_mm_set_ps1(rx*(1-ry));
//					coef3=_mm_set_ps1((1-rx)*ry);
//					coef4=_mm_set_ps1(rx*ry);
//					i1f=image[xi][yi];i2f=image[xi+1][yi];i3f=image[xi][yi+1];i4f=image[xi+1][yi+1];
//					for(z=0;z<z_pixels_simd;z++){
//						i1f[z]=_mm_add_ps(i1f[z],_mm_mul_ps(imptr[z],coef1));						
//						i2f[z]=_mm_add_ps(i2f[z],_mm_mul_ps(imptr[z],coef2));						
//						i3f[z]=_mm_add_ps(i3f[z],_mm_mul_ps(imptr[z],coef3));						
//						i4f[z]=_mm_add_ps(i4f[z],_mm_mul_ps(imptr[z],coef4));						
//					}
//				}
//
//
//
//
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1[z]=mptr1[yr]+mptr1[yr+2];
//				}
//				xi=x-img_center;yi=0;imptr=i1;
//
//				xx=xi*cosv1-yi*sinv1+img_center;
//				yy=xi*sinv1+yi*cosv1+img_center;
//				xi=xx;				yi=yy;
//				rx=xx-xi;			ry=yy-yi;
//				coef1=_mm_set_ps1((1-rx)*(1-ry));
//				coef2=_mm_set_ps1(rx*(1-ry));
//				coef3=_mm_set_ps1((1-rx)*ry);
//				coef4=_mm_set_ps1(rx*ry);
//				i1f=image[xi][yi];i2f=image[xi+1][yi];i3f=image[xi][yi+1];i4f=image[xi+1][yi+1];
//				for(z=0;z<z_pixels_simd;z++){
//					i1f[z]=_mm_add_ps(i1f[z],_mm_mul_ps(imptr[z],coef1));						
//					i2f[z]=_mm_add_ps(i2f[z],_mm_mul_ps(imptr[z],coef2));						
//					i3f[z]=_mm_add_ps(i3f[z],_mm_mul_ps(imptr[z],coef3));						
//					i4f[z]=_mm_add_ps(i4f[z],_mm_mul_ps(imptr[z],coef4));						
//				}
//			}		
//			
//
//			y=img_center-1;
//
//			memset(diff1,0,z_pixels_simd*4*sizeof(__m128));
//			memset(diff2,0,z_pixels_simd*4*sizeof(__m128));
//			memcpy(sumt1,pp1[0],z_pixels_simd*4*sizeof(__m128));
//			memcpy(sumt2,pp1[0],z_pixels_simd*4*sizeof(__m128));
//			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
//			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//				yr=lptr1[th2].yra;
//				coef1=_mm_set_ps1(lptr1[th2].yr2a);
//				yr2=lptr2[th2].yra;
//				z2=lptr2[th2].zsa;
//				coef2=_mm_set_ps1(lptr2[th2].yr2a);
//				coef3=_mm_set_ps1(0);
//				yr=lptr1[th2].yra;
//
//				i1f=(float*)p1fm;
////				LOG_INFO("z=%d\t%d\t%d\t%d\n",th2,lptr1[th2].zsa,lptr1[th2].zea,yr);
//				for(z=lptr1[th2].zsa;z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
//					sumt1[z]=_mm_add_ps(sumt1[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					sumt2[z]=_mm_add_ps(sumt2[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//					sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
//					sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//				}
//				tan=-sin_theta[th]/cos_theta[th];
//				coef2=_mm_set1_ps(tan);
//				coef1=_mm_set1_ps(1);
//				for(yr=yr_bottom[th]-1;yr<=yr_top[th];yr++){
//					p1fm[yr]=_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2);
//					p2fm[yr]=_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2);
//					diff1[yr]=_mm_add_ps(diff1[yr],_mm_mul_ps(p1fm[yr],coef1));
//					diff2[yr]=_mm_add_ps(diff2[yr],_mm_mul_ps(p2fm[yr],coef1));
////					diff1[yr]=_mm_add_ps(diff1[yr],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2));
////					diff2[yr]=_mm_add_ps(diff2[yr],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2));
//				}
//				z=blutacc[img_center+1][th2].z_start;
//				yr=blutacc[img_center+1][th2].yr_start;
//				for(;z<=blutacc[img_center+1][th2].z_end;yr++,z++){
//					diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p2fm[yr],coef1));
//					diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p1fm[yr],coef1));
////					diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef2));
////					diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef2));
//				}
//			}
//		//	memcpy(sum1,sumt1,z_pixels_simd*4*sizeof(__m128));
//		//	memcpy(sum2,sumt2,z_pixels_simd*4*sizeof(__m128));
//			goto norm_zero_mask_label;
//		}
//
//		for(y=img_center-2;y>=ystart;y--){
//			if(zero_mask_flag==1) goto norm_zero_mask_label;
//		//	memcpy(sum1,pp1[0],z_pixels_simd*4*sizeof(__m128));
//		//	memcpy(sum2,pp1[0],z_pixels_simd*4*sizeof(__m128));
//		//	memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
//		//	memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//		//		yr=lptr1[th2].yra;
//		//		coef1=_mm_set_ps1(lptr1[th2].yr2a);
//		//		yr2=lptr2[th2].yra;
//		//		z2=lptr2[th2].zsa;
//		//		coef2=_mm_set_ps1(lptr2[th2].yr2a);
//		//		for(z=lptr1[th2].zsa;z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
//		//			sum1[z]=_mm_add_ps(sum1[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//		//			sum2[z]=_mm_add_ps(sum2[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//		//			sum1[z2]=_mm_add_ps(sum1[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
//		//			sum2[z2]=_mm_add_ps(sum2[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//		//		}
//
//				
//				if(blutacc_flag[y][th2]!=0 ){
//					tan=-sin_theta[th]/cos_theta[th];
//					tan=1-tan*(blutacc[y][th2].num)-blutacc[y][th2].coef_pre;
//					coef1=_mm_set_ps1(-tan*cos_theta[th]/sin_theta[th]);
////					tan=sin_theta[th]/cos_theta[th];
////					tan=-tan*blutacc[y_pixels-y][th2].num-blutacc[y_pixels-y][th2].coef_pre;
////					coeft2=tan;
////					coef2=_mm_set_ps1(tan);
//			//		coeft1=tan;
//
//					z=blutacc[y][th2].z_start_pre;
//					z2=blutacc[y_pixels-y][th2].z_start_pre;
//					yr=blutacc[y][th2].yr_start_pre;
//					yr2=blutacc[y_pixels-y][th2].yr_start_pre;
//
//					z3=blutacc[y][th2].z_start;
//					z4=blutacc[y_pixels-y][th2].z_start;
//					yr3=blutacc[y][th2].yr_start;
//					yr4=blutacc[y_pixels-y][th2].yr_start;
//
//					coef3=_mm_set_ps1(-sin_theta[th]/cos_theta[th]);
//					coef3=_mm_set_ps1(1);
//			//		coeft2=-sin_theta[th]/cos_theta[th];
//			//		coeft3=coeft1/coeft2;
//			//		coeft3=coeft3*coeft2;
//		//			if(fabs(coeft1+coeft2)>1e-6&&x==126){
//			//		if(x==126){
////						LOG_INFO("%d\t%d\t%d\t%e\t%e\t%e\n",x,y,th2,coeft1,coeft2,coeft3);
//			//		}
//				//	if((z3-z)==-1 && x==126){
//			//			LOG_INFO("%d\t%d\t%d\t%d\t%d\t%d\t%d\n",y,th2,z,z2,z3,z4,blutacc[y_pixels-y][th2].z_end);
//				//	}
//					if((z3-z)!=-1){
//						for(;z<=blutacc[y][th2].z_end_pre;z++,z2++,yr++,yr2++){
//							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
//							sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr2],coef1));
//							sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr2],coef1));
//							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
//							diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
//							diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr2],coef3));
//							diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr2],coef3));
//							diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
//						}
//						for(;z3<=blutacc[y][th2].z_end;z3++,z4++,yr3++,yr4++){
//							sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr3],coef1));
//							sumt2[z4]=_mm_add_ps(sumt2[z4],_mm_mul_ps(p1fm[yr4],coef1));
//							sumt1[z4]=_mm_add_ps(sumt1[z4],_mm_mul_ps(p2fm[yr4],coef1));
//							sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr3],coef1));
//							diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr3],coef3));
//							diff2[z4]=_mm_sub_ps(diff2[z4],_mm_mul_ps(p1fm[yr4],coef3));
//							diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr3],coef3));
//							diff1[z4]=_mm_sub_ps(diff1[z4],_mm_mul_ps(p2fm[yr4],coef3));
//						}
//					} else {
//						sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr],coef1));
//						sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr],coef1));
//						diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr],coef3));
//						diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr],coef3));
//
//						sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));
//						sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));
//						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));
//						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));
//						z2++;
//
//
//						for(;z<blutacc[y][th2].z_end_pre;yr++,z++,z2++){//,z3++,z4++){
//							m1=_mm_sub_ps(p1fm[yr],p1fm[yr+1]);
//							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p1fm[yr],p1fm[yr+1]),coef1));
//							sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p1fm[yr],p1fm[yr+1]),coef1));
//							diff1[z]=_mm_sub_ps(diff1[z],m1);//_mm_sub_ps(p1fm[yr+1],p1fm[yr]));
//							diff2[z2]=_mm_sub_ps(diff2[z2],m1);//_mm_sub_ps(p1fm[yr+1],p1fm[yr]));
//							m1=_mm_sub_ps(p2fm[yr],p2fm[yr+1]);
//							sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p2fm[yr],p2fm[yr+1]),coef1));
//							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(m1,coef1));//_mm_mul_ps(_mm_sub_ps(p2fm[yr],p2fm[yr+1]),coef1));
//							diff2[z]=_mm_sub_ps(diff2[z],m1);//_mm_sub_ps(p2fm[yr+1],p2fm[yr]));
//							diff1[z2]=_mm_sub_ps(diff1[z2],m1);//_mm_sub_ps(p2fm[yr+1],p2fm[yr]));
//
//
////							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
////							sumt1[z]=_mm_sub_ps(sumt1[z],_mm_mul_ps(p1fm[yr+1],coef1));
//
////							sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr+1],coef1));
////							sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));
//
////							sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
////							sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr],coef1));
//
////							sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));
////							sumt2[z4]=_mm_add_ps(sumt2[z4],_mm_mul_ps(p1fm[yr],coef1));
//
//
//
////							sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr+1],coef1));
////							sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));
//
////							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
////							sumt2[z]=_mm_sub_ps(sumt2[z],_mm_mul_ps(p2fm[yr+1],coef1));
//
//
////							sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));
////							sumt1[z4]=_mm_add_ps(sumt1[z4],_mm_mul_ps(p2fm[yr],coef1));
//
////							sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
////							sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr],coef1));
//
////							diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
////							diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(p1fm[yr+1],coef3));
//
////							diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr+1],coef3));
////							diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));
//
//
////							diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
////							diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr],coef3));
//
////							diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));
////							diff2[z4]=_mm_sub_ps(diff2[z4],_mm_mul_ps(p1fm[yr],coef3));
//
//
//	//						m1=_mm_mul_ps(p2fm[yr],coef3);
//
//
////							diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
////							diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(p2fm[yr+1],coef3));
//
////							diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr+1],coef3));
////							diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));
//
////							diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));
////							diff1[z4]=_mm_sub_ps(diff1[z4],_mm_mul_ps(p2fm[yr],coef3));
//
////							diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
////							diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr],coef3));
//						}
//						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
//						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
//						diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
//						diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
//
//						sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(p1fm[yr],coef1));
//						sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(p2fm[yr],coef1));
//						diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(p1fm[yr],coef3));
//						diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(p2fm[yr],coef3));
//
//					}
///*					for(;z<=blutacc[y][th2].z_end_pre;z++,z2++,yr++,yr2++){
//						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(p1fm[yr],coef1));
//						sumt2[z2]=_mm_sub_ps(sumt2[z2],_mm_mul_ps(p1fm[yr2],coef1));
//						sumt1[z2]=_mm_sub_ps(sumt1[z2],_mm_mul_ps(p2fm[yr2],coef1));
//						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(p2fm[yr],coef1));
//						diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(p1fm[yr],coef3));
//						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(p2fm[yr2],coef3));
//						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(p1fm[yr2],coef3));
//						diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(p2fm[yr],coef3));
////						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef1));
////						sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef2));
////						sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef2));
////						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef1));
////						diff1[z]=_mm_sub_ps(diff1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef3));
////						diff1[z2]=_mm_add_ps(diff1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef3));
////						diff2[z2]=_mm_add_ps(diff2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef3));
////						diff2[z]=_mm_sub_ps(diff2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef3));
//					}
////					z=blutacc[y][th2].z_start;
////					z2=blutacc[y_pixels-y][th2].z_start;
////					yr=blutacc[y][th2].yr_start;
////					yr2=blutacc[y_pixels-y][th2].yr_start;
//			//		coef1=_mm_set_ps1(blutacc[y][th2].coef+sin_theta[th]/cos_theta[th]);
//			//		coeft3=blutacc[y][th2].coef+sin_theta[th]/cos_theta[th];
//			//		coef2=_mm_set_ps1(blutacc[y_pixels-y][th2].coef-sin_theta[th]/cos_theta[th]-1);
//			//		coeft2=blutacc[y_pixels-y][th2].coef-sin_theta[th]/cos_theta[th]-1;
//			//		coef3=_mm_set_ps1(-sin_theta[th]/cos_theta[th]);
//			//		if(fabs(coeft1+coeft3)>1e-5){
//		//				LOG_INFO("2nd %d\t%d\t%d\t%f\t%f\t%e\n",x,y,th2,coeft1,coeft3,fabs(coeft1+coeft3));
//		//			}
//					for(;z3<=blutacc[y][th2].z_end;z3++,z4++,yr3++,yr4++){
//						sumt1[z3]=_mm_sub_ps(sumt1[z3],_mm_mul_ps(p1fm[yr3],coef1));
//						sumt2[z4]=_mm_add_ps(sumt2[z4],_mm_mul_ps(p1fm[yr4],coef1));
//						sumt1[z4]=_mm_add_ps(sumt1[z4],_mm_mul_ps(p2fm[yr4],coef1));
//						sumt2[z3]=_mm_sub_ps(sumt2[z3],_mm_mul_ps(p2fm[yr3],coef1));
//						diff1[z3]=_mm_add_ps(diff1[z3],_mm_mul_ps(p1fm[yr3],coef3));
//						diff2[z4]=_mm_sub_ps(diff2[z4],_mm_mul_ps(p1fm[yr4],coef3));
//						diff2[z3]=_mm_add_ps(diff2[z3],_mm_mul_ps(p2fm[yr3],coef3));
//						diff1[z4]=_mm_sub_ps(diff1[z4],_mm_mul_ps(p2fm[yr4],coef3));
////						sumt1[z]=_mm_add_ps(sumt1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef1));
////						sumt2[z2]=_mm_add_ps(sumt2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef2));
////						sumt1[z2]=_mm_add_ps(sumt1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef2));
////						sumt2[z]=_mm_add_ps(sumt2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef1));
////						diff1[z]=_mm_add_ps(diff1[z],_mm_mul_ps(_mm_sub_ps(p1fm[yr+1],p1fm[yr]),coef3));
////						diff2[z2]=_mm_sub_ps(diff2[z2],_mm_mul_ps(_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]),coef3));
////						diff2[z]=_mm_add_ps(diff2[z],_mm_mul_ps(_mm_sub_ps(p2fm[yr+1],p2fm[yr]),coef3));
////						diff1[z2]=_mm_sub_ps(diff1[z2],_mm_mul_ps(_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]),coef3));
//					}*/
//				} 
//			} 
//			for(z=0;z<z_pixels;z++){
//				sumt1[z]=_mm_add_ps(sumt1[z],diff1[z]);
//				sumt2[z]=_mm_add_ps(sumt2[z],diff2[z]);
//			}
//	//		for(z=0;z<z_pixels;z++){
//	//			sumtt1[z]=_mm_sub_ps(sumt1[z],sum1[z]);
//	//			sumtt2[z]=_mm_sub_ps(sumt2[z],sum2[z]);
//	//		}
//	//		i1f=(float *)sumtt2;
//	//		tan=0;
//	//		for(z=0;z<z_pixels*4;z+=4){
//	//			tan+=fabs(i1f[z]);
//	//		}
//	//		LOG_INFO("y=%d\t%e\n",y,tan);
//	/*		i1f=(float *) sum1;
//			i2f=(float *) sumt1;
//			i3f=(float *) sumtt1;
//			i4f=(float *) diff1;
//			i5f=(float *) pp1[0];
//			if(y==128 && x==128){
//				for(z=0;z<z_pixels*4;z+=4){
//					LOG_INFO("%d\t%d\t%e\t%e\t%e\t%e\t%e\n",y,z/4,i1f[z],i2f[z],i3f[z],i4f[z],i5f[z]);
//				}
//			
//			}
//*/
//norm_zero_mask_label:
//			if(x!=img_center ){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1[z]=mptr1[yr];
//					i2[z]=mptr1[yr+1];
//					i3[z]=mptr1[yr+2];
//					i4[z]=mptr1[yr+3];
//					i5[z]=mptr2[yr];
//					i6[z]=mptr2[yr+1];
//					i7[z]=mptr2[yr+2];
//					i8[z]=mptr2[yr+3];
//				}
//				for(i=0;i<8;i++){
//					if(i==0){imptr=i1;xi=x;yi=y;}
//					else if(i==1){imptr=i2;xi=x_pixels-x;yi=y;}
//					else if(i==2){imptr=i3;xi=y_pixels-y;yi=x;}
//					else if(i==3){imptr=i4;xi=y_pixels-y;yi=x_pixels-x;}
//					else if(i==4){imptr=i5;xi=x;yi=y_pixels-y;}
//					else if(i==5){imptr=i6;xi=x_pixels-x;yi=y_pixels-y;}
//					else if(i==6){imptr=i7;xi=y;yi=x;}
//					else if(i==7){imptr=i8;xi=y;yi=x_pixels-x;}
//
//					xi=xi-img_center;yi=yi-img_center;
//					xx=xi*cosv1-yi*sinv1+img_center;
//					yy=xi*sinv1+yi*cosv1+img_center;
//					xi=xx;				yi=yy;
//					rx=xx-xi;			ry=yy-yi;
//					coef1=_mm_set_ps1((1-rx)*(1-ry));
//					coef2=_mm_set_ps1(rx*(1-ry));
//					coef3=_mm_set_ps1((1-rx)*ry);
//					coef4=_mm_set_ps1(rx*ry);
//					i1f=image[xi][yi];i2f=image[xi+1][yi];i3f=image[xi][yi+1];i4f=image[xi+1][yi+1];
//					for(z=0;z<z_pixels_simd;z++){
//						i1f[z]=_mm_add_ps(i1f[z],_mm_mul_ps(imptr[z],coef1));						
//						i2f[z]=_mm_add_ps(i2f[z],_mm_mul_ps(imptr[z],coef2));						
//						i3f[z]=_mm_add_ps(i3f[z],_mm_mul_ps(imptr[z],coef3));						
//						i4f[z]=_mm_add_ps(i4f[z],_mm_mul_ps(imptr[z],coef4));						
//					}
//				}
//
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1[z]=mptr1[yr];
//					i3[z]=mptr1[yr+2];
//					i5[z]=mptr2[yr];
//					i7[z]=mptr2[yr+2];
//				}
//				for(i=0;i<4;i++){
//					if(i==0){imptr=i1;xi=x;yi=y;}
//					else if(i==1){imptr=i3;xi=y_pixels-y;yi=x;}
//					else if(i==2){imptr=i5;xi=x;yi=y_pixels-y;}
//					else if(i==3){imptr=i7;xi=y;yi=x;}
//
//					xi=xi-img_center;yi=yi-img_center;
//					xx=xi*cosv1-yi*sinv1+img_center;
//					yy=xi*sinv1+yi*cosv1+img_center;
//					xi=xx;				yi=yy;
//					rx=xx-xi;			ry=yy-yi;
//					coef1=_mm_set_ps1((1-rx)*(1-ry));
//					coef2=_mm_set_ps1(rx*(1-ry));
//					coef3=_mm_set_ps1((1-rx)*ry);
//					coef4=_mm_set_ps1(rx*ry);
//					i1f=image[xi][yi];i2f=image[xi+1][yi];i3f=image[xi][yi+1];i4f=image[xi+1][yi+1];
//					for(z=0;z<z_pixels_simd;z++){
//						i1f[z]=_mm_add_ps(i1f[z],_mm_mul_ps(imptr[z],coef1));						
//						i2f[z]=_mm_add_ps(i2f[z],_mm_mul_ps(imptr[z],coef2));						
//						i3f[z]=_mm_add_ps(i3f[z],_mm_mul_ps(imptr[z],coef3));						
//						i4f[z]=_mm_add_ps(i4f[z],_mm_mul_ps(imptr[z],coef4));						
//					}
//				}
//
//			}
//		}
//
//		for(th=2;th<th_pixels;th+=2){
//			p1fm=pp1[th/2];
//			p2fm=pp2[th/2];
//			coef1=_mm_set1_ps(0);
//			p1fm[yr_bottom[th]-1]=coef1;
//			p1fm[yr_top[th]+1]=coef1;
//			p2fm[yr_bottom[th]-1]=coef1;
//			p2fm[yr_top[th]+1]=coef1;
//		}
//	}
//	return 1;
//}
//
int back_proj3d_view_n1thread_old(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
{
	Backprojection_lookup lptr1[MAXGROUP];
	Backprojection_lookup lptr2[MAXGROUP];
	int		th,th2;
	register int    yr, z,z2;
	int		x, y;		/* indice of the 3 dimensions in reconstructed image */
	int		ystart,yend;
	float	*i1f,*i2f,*i3f,*i4f;
	float	*i5f,*i6f,*i7f,*i8f;
	__m128	*p1fm;
	__m128	*p2fm;
	__m128	**pp1;
	__m128	**pp2;
	int		img_center=x_pixels/2;
	__m128	coef1,coef2;
	//	__m128	*i1fmm;
	//	__m128	*i2fmm;
	float	*mptr1,*mptr2;
	int		z_start=0,z_end=z_pixels;
	int zero_mask_flag=0;
	__m128 *sum1;
	__m128 *sum2;
	__m128 *diff1;
	__m128 *diff2;
//	int numdiff[46];
//	int zdiff[46]
	int yr2;
	//float r1[46],
//	float tan;

	sum1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	sum2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	diff1=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
	diff2=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));

	if(sum1==NULL || sum2==NULL){
		LOG_EXIT("exiting");
	}
	mptr1=(float *) sum1;
	mptr2=(float *) sum2;

	for (x = x_start; x < x_end; x++) {
		pp1=(__m128 **)prj[x];
		pp2=(__m128 **)prj[x+img_center];
		ystart=cylwiny[x][0];
		yend=cylwiny[x][1];
		if(ystart==0 && yend==0) continue;
		zero_mask_flag=0;

		i1f=image[x][img_center];
		i2f=image[x_pixels-x][img_center];
		i3f=image[img_center][x];
		i4f=image[img_center][x_pixels-x];

		if(norm_zero_mask[view][x]==0){
			zero_mask_flag=1;
			if(x!=img_center){
				memset(i1f,0,z_pixels_simd*4*sizeof(float));
				memset(i2f,0,z_pixels_simd*4*sizeof(float));
				memset(i3f,0,z_pixels_simd*4*sizeof(float));
				memset(i4f,0,z_pixels_simd*4*sizeof(float));
			} else {
				memset(i1f,0,z_pixels_simd*4*sizeof(float));
			}
		} else {
			memcpy(sum1,pp1[0],z_pixels_simd*4*sizeof(__m128));
			for(th=2;th<th_pixels;th+=2){
				p1fm=pp1[th/2];
				p2fm=pp2[th/2];
				for(yr=yr_bottom[th];yr<=yr_top[th];yr++){
					sum1[yr]=_mm_add_ps(sum1[yr],_mm_add_ps(p1fm[yr],p2fm[yr]));
				}
			}

			if(x!=img_center){
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]=mptr1[yr];
					i2f[z]=mptr1[yr+1];
					i3f[z]=mptr1[yr+2];
					i4f[z]=mptr1[yr+3];
				}
			} else {
				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
					i1f[z]=mptr1[yr]+mptr1[yr+2];
				}
			}
		}

		for(y=img_center-1;y>=ystart;y--){
			if(zero_mask_flag==1) goto norm_zero_mask_label;
			memcpy(sum1,pp1[0],z_pixels_simd*4*sizeof(__m128));
			memcpy(sum2,pp1[0],z_pixels_simd*4*sizeof(__m128));
			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
				p1fm=pp1[th/2];
				p2fm=pp2[th/2];
				yr=lptr1[th2].yra;
				coef1=_mm_set_ps1(lptr1[th2].yr2a);
				yr2=lptr2[th2].yra;
				z2=lptr2[th2].zsa;
				coef2=_mm_set_ps1(lptr2[th2].yr2a);
				for(z=lptr1[th2].zsa;z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
					sum1[z]=_mm_add_ps(sum1[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
					sum2[z]=_mm_add_ps(sum2[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
					sum1[z2]=_mm_add_ps(sum1[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
					sum2[z2]=_mm_add_ps(sum2[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
				}
			}
norm_zero_mask_label:
			i1f=image[x][y];
			i2f=image[x_pixels-x][y];
			i3f=image[y_pixels-y][x];
			i4f=image[y_pixels-y][x_pixels-x];
			i5f=image[x][y_pixels-y];
			i6f=image[x_pixels-x][y_pixels-y];
			i7f=image[y][x];
			i8f=image[y][x_pixels-x];

			if(x!=img_center ){
				if(x==y){
					if(zero_mask_flag){
						memset(i1f,0,z_pixels_simd*sizeof(__m128));
						memset(i2f,0,z_pixels_simd*sizeof(__m128));
						memset(i5f,0,z_pixels_simd*sizeof(__m128));
						memset(i6f,0,z_pixels_simd*sizeof(__m128));
					} else {
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]=mptr1[yr]+mptr2[yr+2];
							i2f[z]=mptr1[yr+1]+mptr1[yr+2];
							i5f[z]=mptr2[yr]+mptr2[yr+3];
							i6f[z]=mptr1[yr+3]+mptr2[yr+1];
						}
					}
				} else if(x<y){
					if(zero_mask_flag){
						memset(i1f,0,z_pixels_simd*sizeof(__m128));
						memset(i2f,0,z_pixels_simd*sizeof(__m128));
						memset(i3f,0,z_pixels_simd*sizeof(__m128));
						memset(i4f,0,z_pixels_simd*sizeof(__m128));
						memset(i5f,0,z_pixels_simd*sizeof(__m128));
						memset(i6f,0,z_pixels_simd*sizeof(__m128));
						memset(i7f,0,z_pixels_simd*sizeof(__m128));
						memset(i8f,0,z_pixels_simd*sizeof(__m128));
					} else {
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]=mptr1[yr];
							i2f[z]=mptr1[yr+1];
							i3f[z]=mptr1[yr+2];
							i4f[z]=mptr1[yr+3];
							i5f[z]=mptr2[yr];
							i6f[z]=mptr2[yr+1];
							i7f[z]=mptr2[yr+2];
							i8f[z]=mptr2[yr+3];
						}
					}
				} else {
					if(!zero_mask_flag){
						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
							i1f[z]+=mptr1[yr];
							i2f[z]+=mptr1[yr+1];
							i3f[z]+=mptr1[yr+2];
							i4f[z]+=mptr1[yr+3];
							i5f[z]+=mptr2[yr];
							i6f[z]+=mptr2[yr+1];
							i7f[z]+=mptr2[yr+2];
							i8f[z]+=mptr2[yr+3];
						}
					}
				}
			} else {
				if(!zero_mask_flag){
					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
						i1f[z]+=mptr1[yr];
						i3f[z]+=mptr1[yr+2];
						i5f[z]+=mptr2[yr];
						i7f[z]+=mptr2[yr+2];
					}
				}
			}
		}
		for(th=2;th<th_pixels;th+=2){
			p1fm=pp1[th/2];
			p2fm=pp2[th/2];
			coef1=_mm_set1_ps(0);
			p1fm[yr_bottom[th]-1]=coef1;
			p1fm[yr_top[th]+1]=coef1;
			p2fm[yr_bottom[th]-1]=coef1;
			p2fm[yr_top[th]+1]=coef1;
		}
	}
	return 1;
}

//int back_proj3d_view_n1thread_old_bug(float ***image,float ***prj, int view_90flag,int view,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup lptr1[MAXGROUP];
//	Backprojection_lookup lptr2[MAXGROUP];
//	int		th,th2;
//	register int    yr,yr2, z;
//	int		x, y;		// indice of the 3 dimensions in reconstructed image 
//	int		ystart,yend;
//	float	*i1f,*i2f,*i3f,*i4f;
//	float	*i5f,*i6f,*i7f,*i8f;
//	__m128	*p1fm;
//	__m128	*p2fm;
//	__m128	**pp1;
//	__m128	**pp2;
//	int		y_off=y_pixels/2;
//	__m128	coef1,coef2;
//	__m128	*i1fmm;
//	__m128	*i2fmm;
//	float	*mptr1,*mptr2;
//	int		z_start=0,z_end=z_pixels;
//	int zero_mask_flag=0;
//	int z2;
//
//	i1fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	i2fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	if(i2fmm==NULL || i1fmm==NULL){
//		LOG_EXIT("exiting");
//	}
//	mptr1=(float *) i1fmm;
//	mptr2=(float *) i2fmm;
//	for (x = x_start; x < x_end; x++) {
//		pp1=(__m128 **)prj[x];
//		pp2=(__m128 **)prj[x+xr_pixels/2];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//		zero_mask_flag=0;
//		if(norm_zero_mask[view][x]==0){
//			zero_mask_flag=1;
//		}
////		zero_mask_flag=0;
//
//		for (y = ystart; y <y_pixels/2;y++){// yend; y++) {
//			if(zero_mask_flag) goto norm_zero_mask_label;
//			p1fm=pp1[0];
//			if(y==y_pixels/2) continue;
//			memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//			memcpy(i2fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
////			memset(i1fmm,0,z_pixels_simd*4*sizeof(__m128));
////			memset(i2fmm,0,z_pixels_simd*4*sizeof(__m128));
//			memcpy(lptr1,blookup[y],sizeof(Backprojection_lookup)*groupmax);
//			memcpy(lptr2,blookup[y_pixels-y],sizeof(Backprojection_lookup)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=pp1[th/2];
//				p2fm=pp2[th/2];
//
//				yr2=lptr2[th2].yra;
//				coef2=_mm_set1_ps(lptr2[th2].yr2a);
//				z=lptr1[th2].zsa;
//				yr=lptr1[th2].yra;
//				coef1=_mm_set1_ps(lptr1[th2].yr2a);
//				if(lptr2[th2].zsa>=lptr1[th2].zea){
//					z2=lptr2[th2].zsa;
//					for(z=lptr1[th2].zsa,yr=lptr1[th2].yra;z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
//						i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//						i1fmm[z2]=_mm_add_ps(i1fmm[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
//						i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//						i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//					}
//					continue;
//				}
//				for(;z<lptr2[th2].zsa;z++,yr++){
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//				}
//
//				for(;z<=lptr1[th2].zea;z++,yr++,yr2++){
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(p1fm[yr],p2fm[yr2+1])),_mm_mul_ps(coef1,_mm_add_ps(p2fm[yr2],p1fm[yr+1]))));
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(p2fm[yr],p1fm[yr2+1])),_mm_mul_ps(coef1,_mm_add_ps(p1fm[yr2],p2fm[yr+1]))));
//				}
//				for(;z<=lptr2[th2].zea;z++,yr2++){
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
//				}
//			}
//norm_zero_mask_label:	;		
//
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y_pixels-y][x];
//			i4f=image[y_pixels-y][x_pixels-x];
//			i5f=image[x][y_pixels-y];
//			i6f=image[x_pixels-x][y_pixels-y];
//			i7f=image[y][x];
//			i8f=image[y][x_pixels-x];
//			if(x!=x_pixels/2 ){
//				if(x==y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr]+mptr2[yr+2];
//							i2f[z]=mptr1[yr+1]+mptr1[yr+2];
//							i5f[z]=mptr2[yr]+mptr2[yr+3];
//							i6f[z]=mptr1[yr+3]+mptr2[yr+1];
//						}
//					}
//				} else if(x<y){
//					if(zero_mask_flag){
//						memset(i1f,0,z_pixels_simd*sizeof(__m128));
//						memset(i2f,0,z_pixels_simd*sizeof(__m128));
//						memset(i3f,0,z_pixels_simd*sizeof(__m128));
//						memset(i4f,0,z_pixels_simd*sizeof(__m128));
//						memset(i5f,0,z_pixels_simd*sizeof(__m128));
//						memset(i6f,0,z_pixels_simd*sizeof(__m128));
//						memset(i7f,0,z_pixels_simd*sizeof(__m128));
//						memset(i8f,0,z_pixels_simd*sizeof(__m128));
//					} else {
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//							i1f[z]=mptr1[yr];
//							i2f[z]=mptr1[yr+1];
//							i3f[z]=mptr1[yr+2];
//							i4f[z]=mptr1[yr+3];
//							i5f[z]=mptr2[yr];
//							i6f[z]=mptr2[yr+1];
//							i7f[z]=mptr2[yr+2];
//							i8f[z]=mptr2[yr+3];
//						}
//					}
//				} else {
//					if(!zero_mask_flag){
//						for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						//	LOG_INFO("xyz %d\t%d\t%d\t%d\n",view,x,y,z);
//							i1f[z]+=mptr1[yr];
//							i2f[z]+=mptr1[yr+1];
//							i3f[z]+=mptr1[yr+2];
//							i4f[z]+=mptr1[yr+3];
//							i5f[z]+=mptr2[yr];
//							i6f[z]+=mptr2[yr+1];
//							i7f[z]+=mptr2[yr+2];
//							i8f[z]+=mptr2[yr+3];
//						}
//					}
//				}
//			} else {
//				if(!zero_mask_flag){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]+=mptr1[yr];
//						i3f[z]+=mptr1[yr+2];
//						i5f[z]+=mptr2[yr];
//						i7f[z]+=mptr2[yr+2];
//					}
//				}
//			}
//		}
//
//		y=y_pixels/2;
//		//	pp1=(__m128 **) prj[x];
//		//	pp2=(__m128 **) prj[x+xr_pixels/2];
//		p1fm=pp1[0];
//		p2fm=pp2[0];
//
//
//		memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//			p1fm=pp1[th/2];
//			p2fm=pp2[th/2];
//			for(z=yr_bottom[th];z<=yr_top[th];z++){
//				i1fmm[z]=_mm_add_ps(i1fmm[z],p1fm[z]);
//				i1fmm[z]=_mm_add_ps(i1fmm[z],p2fm[z]);
//			}
//		}
//		i1f=image[x][y];
//		i2f=image[x_pixels-x][y];
//		i3f=image[y][x];
//		i4f=image[y][x_pixels-x];
//		if(x!=x_pixels/2){
//			for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//				i1f[z]=mptr1[yr];
//				i2f[z]=mptr1[yr+1];
//				i3f[z]=mptr1[yr+2];
//				i4f[z]=mptr1[yr+3];
//			}
//		} else {
//			for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//				i1f[z]=mptr1[yr]+mptr1[yr+2];
//			}
//		}
//	}
//	return 1;
//}
//
//
//int back_proj3d_view_n1_oblique(float ***image,float ***prj, int view_90flag,int theta2,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup_oblique *lptr1;
//	Backprojection_lookup_oblique *lptr2;
//	Backprojection_lookup_oblique_start_end lut_start_end1[MAXGROUP];
//	Backprojection_lookup_oblique_start_end lut_start_end2[MAXGROUP];
//	int    th,th2;
//	int    yr,yr2;
//	int    x, y, z,z2,zi1,zi2;		// indice of the 3 dimensions in reconstructed image 
//	int ystart,yend;
//	float  *i1f,*i2f,*i3f,*i4f;
//	__m128 *p1fm;
//	float  **pp1;
//	int y_off=y_pixels/2;
//	__m128 coef1,coef2;
//	__m128 *i1fmm;
//	__m128 *i2fmm;
//
//	float *mptr1,*mptr2;
//	int z_start=0,z_end=z_pixels;
//
//	i1fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	i2fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	lptr1=(Backprojection_lookup_oblique *)_alloca((blookup_oblique_maxnum)*sizeof(Backprojection_lookup_oblique));
//	lptr2=(Backprojection_lookup_oblique *)_alloca((blookup_oblique_maxnum)*sizeof(Backprojection_lookup_oblique));
//	if(i2fmm==NULL || i1fmm==NULL || lptr1==NULL || lptr2==NULL){
//		LOG_EXIT("exiting");
//	}
//
//	mptr1=(float *) i1fmm;
//	mptr2=(float *) i2fmm;
//
//	for (x = x_start; x < x_end; x++) {
//		if(view_90flag==0) pp1=prj[x];
//		else	pp1=prj[x+xr_pixels/2];
//		pp1=prj[x];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//		for (y = ystart; y <y_pixels/2;y++){// yend; y++) {
//			p1fm=(__m128 *)pp1[0];
//			if(y==y_pixels/2) continue;
//			memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//			memcpy(i2fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//			memcpy(lut_start_end1,blookup_oblique_start_end[y],sizeof(Backprojection_lookup_oblique_start_end)*groupmax);
//			memcpy(lut_start_end2,blookup_oblique_start_end[y_pixels-y],sizeof(Backprojection_lookup_oblique_start_end)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				memcpy(lptr1,blookup_oblique[y][th2],sizeof(Backprojection_lookup_oblique)*lut_start_end1[th2].num);
//				memcpy(lptr2,blookup_oblique[y_pixels-y][th2],sizeof(Backprojection_lookup_oblique)*lut_start_end2[th2].num);
//				p1fm=(__m128 *)pp1[th/2];
//				for(zi1=0,z=lut_start_end1[th2].z1;z<lut_start_end1[th2].z2;z++,zi1++){
//					coef1=_mm_set1_ps(1-lptr1[zi1].coef);
//					yr=lptr1[zi1].yr;
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					//					i1fmm[z]+=p1fm[lptr1[yr2].yr]*lptr1[yr].coef+p1fm[lptr1[yr].yr+1]*(1-lptr1[yr].coef);
//				}
//				for(zi2=0,z=lut_start_end2[th2].z1;z<lut_start_end2[th2].z2;z++,zi2++){
//					coef1=_mm_set1_ps(1-lptr2[zi2].coef);
//					yr=lptr2[zi2].yr;
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					//					i1fmm[z]+=p1fm[lptr1[yr2].yr]*lptr1[yr].coef+p1fm[lptr1[yr].yr+1]*(1-lptr1[yr].coef);
//				}
//				z=lut_start_end1[th2].z2;
//				z2=lut_start_end2[th2].z2;
//				coef1=_mm_set1_ps(1-lptr1[zi1].coef);
//				coef2=_mm_set1_ps(1-lptr2[zi2].coef);
//				yr=lptr1[zi1].yr;
//				yr2=lptr2[zi2].yr;
//				zi1++;
//				zi2++;
//				for(;z<lut_start_end1[th2].z3;z++,z2++,yr++,yr2++){
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//				}
//				for(z=lut_start_end1[th2].z3;z<lut_start_end1[th2].z4;z++,zi1++){
//					coef1=_mm_set1_ps(1-lptr1[zi1].coef);
//					yr=lptr1[zi1].yr;
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//				}
//				for(z=lut_start_end2[th2].z3;z<lut_start_end2[th2].z4;z++,zi2++){
//					coef1=_mm_set1_ps(1-lptr2[zi2].coef);
//					yr=lptr2[zi2].yr;
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//				}
//				/*
//				z2=lptr2[th2].zsa;
//				yr2=lptr2[th2].yra;
//				coef2=_mm_set1_ps(lptr2[th2].yr2a);
//				for(z=lptr1[th2].zsa,yr=lptr1[th2].yra,coef1=_mm_set1_ps(lptr1[th2].yr2a);z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
//				i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//				i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//				}
//				*/			}
//
//
//			if(view_90flag==0){
//				i1f=image[x][y];
//				i2f=image[x][y_pixels-y];
//				i3f=image[x_pixels-x][y];
//				i4f=image[x_pixels-x][y_pixels-y];
//				if(x!=x_pixels/2){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]=mptr1[yr]+mptr2[yr+1];
//						i2f[z]=mptr1[yr+1]+mptr2[yr];
//						i3f[z]=mptr1[yr+2]+mptr2[yr+3];
//						i4f[z]=mptr1[yr+3]+mptr2[yr+2];
//					}
//				}else {
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]=mptr1[yr]+mptr2[yr+1];
//						i2f[z]=mptr1[yr+1]+mptr2[yr];
//					}
//				}
//			} else {
//				i1f=image[y_pixels-y][x];
//				i2f=image[y][x];
//				i3f=image[y_pixels-y][x_pixels-x];
//				i4f=image[y][x_pixels-x];
//				if(x!=x_pixels/2){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]+=mptr1[yr]+mptr2[yr+1];
//						i2f[z]+=mptr1[yr+1]+mptr2[yr];
//						i3f[z]+=mptr1[yr+2]+mptr2[yr+3];
//						i4f[z]+=mptr1[yr+3]+mptr2[yr+2];
//					}
//				}else {
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						//i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_shuffle_ps(i2fmm[z],i2fmm[z],_MM_SHUFFLE(2, 3, 0, 1)));
//						i1f[z]+=mptr1[yr]+mptr2[yr+1];
//						i2f[z]+=mptr1[yr+1]+mptr2[yr];
//					}
//				}
//			}
//		}
//
//		y=y_pixels/2;
//		p1fm=(__m128 *)pp1[0];
//		memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//			p1fm=(__m128 *)pp1[th/2];
//			for(z=z_start;z<z_end;z++){
//				i1fmm[z]=_mm_add_ps(i1fmm[z],p1fm[z]);//_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//			}
//		}
//		if(view_90flag==0){
//			i1f=image[x][y];
//			i4f=image[x_pixels-x][y];
//			if(x!=x_pixels/2){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr]+mptr1[yr+1];
//					i4f[z]=mptr1[yr+2]+mptr1[yr+3];
//				}
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]=mptr1[yr]+mptr1[yr+1];
//				}
//			}
//		} else {
//			i1f=image[y_pixels-y][x];
//			i4f=image[y][x_pixels-x];
//			if(x!=x_pixels/2){
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]+=mptr1[yr]+mptr1[yr+1];
//					i4f[z]+=mptr1[yr+2]+mptr1[yr+3];
//				}
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]+=mptr1[yr]+mptr1[yr+1];
//				}
//			}
//		}
//	}
//	return 1;
//}
//
//int back_proj3d_view_n1_thread_oblique(float ***image,float ***prj, int view_90flag,int theta2,int verbose,int x_start,int x_end)
//{
//	Backprojection_lookup_oblique *lptr1;
//	Backprojection_lookup_oblique *lptr2;
//	Backprojection_lookup_oblique_start_end lut_start_end1[MAXGROUP];
//	Backprojection_lookup_oblique_start_end lut_start_end2[MAXGROUP];
//	int		th,th2;
//	int		yr,yr2;
//	int		x, y, z,z2,zi1,zi2;		/* indice of the 3 dimensions in reconstructed image */
//	int		ystart,yend;
//	float	*i1f,*i2f,*i3f,*i4f;
//	float	*i5f,*i6f,*i7f,*i8f;
//	__m128	*p1fm;
//	__m128	*p2fm;
//	float	**pp1;
//	float	**pp2;
//	int		y_off=y_pixels/2;
//	__m128	coef1,coef2;
//	__m128	*i1fmm;
//	__m128	*i2fmm;
//	float	*mptr1,*mptr2;
//	int		z_start=0,z_end=z_pixels;
//
//	i1fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	i2fmm=(__m128 *)_alloca((z_pixels_simd*4)*sizeof(__m128));
//	lptr1=(Backprojection_lookup_oblique *)_alloca((blookup_oblique_maxnum)*sizeof(Backprojection_lookup_oblique));
//	lptr2=(Backprojection_lookup_oblique *)_alloca((blookup_oblique_maxnum)*sizeof(Backprojection_lookup_oblique));
//	if(i2fmm==NULL || i1fmm==NULL || lptr1==NULL || lptr2==NULL){
//		LOG_EXIT("exiting");
//	}
//	mptr1=(float *) i1fmm;
//	mptr2=(float *) i2fmm;
//	//	LOG_INFO("start thread %d\n",view_90flag);
//	for (x = x_start; x < x_end; x++) {
//		pp1=prj[x];
//		pp2=prj[x+xr_pixels/2];
//		ystart=cylwiny[x][0];
//		yend=cylwiny[x][1];
//		if(ystart==0 && yend==0) continue;
//
//		for (y = ystart; y <y_pixels/2;y++){// yend; y++) {
//			p1fm=(__m128 *)pp1[0];
//			if(y==y_pixels/2) continue;
//			memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//			memcpy(i2fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//			memcpy(lut_start_end1,blookup_oblique_start_end[y],sizeof(Backprojection_lookup_oblique_start_end)*groupmax);
//			memcpy(lut_start_end2,blookup_oblique_start_end[y_pixels-y],sizeof(Backprojection_lookup_oblique_start_end)*groupmax);
//			for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//				p1fm=(__m128 *)pp1[th/2];
//				p2fm=(__m128 *)pp2[th/2];
//				memcpy(lptr1,blookup_oblique[y][th2],sizeof(Backprojection_lookup_oblique)*lut_start_end1[th2].num);
//				memcpy(lptr2,blookup_oblique[y_pixels-y][th2],sizeof(Backprojection_lookup_oblique)*lut_start_end2[th2].num);
//
//				for(zi1=0,z=lut_start_end1[th2].z1;z<lut_start_end1[th2].z2;z++,zi1++){
//					coef1=_mm_set1_ps(1-lptr1[zi1].coef);
//					yr=lptr1[zi1].yr;
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//					//					i1fmm[z]+=p1fm[lptr1[yr2].yr]*lptr1[yr].coef+p1fm[lptr1[yr].yr+1]*(1-lptr1[yr].coef);
//				}
//				for(zi2=0,z=lut_start_end2[th2].z1;z<lut_start_end2[th2].z2;z++,zi2++){
//					coef1=_mm_set1_ps(1-lptr2[zi2].coef);
//					yr=lptr2[zi2].yr;
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					//					i1fmm[z]+=p1fm[lptr1[yr2].yr]*lptr1[yr].coef+p1fm[lptr1[yr].yr+1]*(1-lptr1[yr].coef);
//				}
//				//		LOG_INFO("done\n");
//				z=lut_start_end1[th2].z2;
//				z2=lut_start_end2[th2].z2;
//				coef1=_mm_set1_ps(1-lptr1[zi1].coef);
//				coef2=_mm_set1_ps(1-lptr2[zi2].coef);
//				yr=lptr1[zi1].yr;
//				yr2=lptr2[zi2].yr;
//				zi1++;
//				zi2++;
//				for(;z<lut_start_end1[th2].z3;z++,yr++){
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//					//	i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]);
//					//	i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]);
//					//	i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(p1fm[yr],p2fm[yr2+1])),_mm_mul_ps(coef1,_mm_add_ps(p2fm[yr2],p1fm[yr+1]))));
//				}
//
//				for(;z2<lut_start_end2[th2].z3;z2++,yr2++){
//					i1fmm[z2]=_mm_add_ps(i1fmm[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
//					i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef2,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//					//	i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(p2fm[yr],p1fm[yr2+1])),_mm_mul_ps(coef1,_mm_add_ps(p1fm[yr2],p2fm[yr+1]))));
//				}
//
//				for(z=lut_start_end1[th2].z3;z<lut_start_end1[th2].z4;z++,zi1++){
//					coef1=_mm_set1_ps(1-lptr1[zi1].coef);
//					yr=lptr1[zi1].yr;
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//					//	i1fmm[z]+=p1fm[lptr1[yr2].yr]*lptr1[yr].coef+p1fm[lptr1[yr].yr+1]*(1-lptr1[yr].coef);
//				}
//				for(z=lut_start_end2[th2].z3;z<lut_start_end2[th2].z4;z++,zi2++){
//					coef1=_mm_set1_ps(1-lptr2[zi2].coef);
//					yr=lptr2[zi2].yr;
//					i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//					i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//					//	i1fmm[z]+=p1fm[lptr1[yr2].yr]*lptr1[yr].coef+p1fm[lptr1[yr].yr+1]*(1-lptr1[yr].coef);
//				}
//
//
//				/*			z2=lptr2[th2].zsa;
//				yr2=lptr2[th2].yra;
//				coef2=_mm_set1_ps(lptr2[th2].yr2a);
//				z=lptr1[th2].zsa;
//				yr=lptr1[th2].yra;
//				coef1=_mm_set1_ps(lptr1[th2].yr2a);
//				for(;z<lptr2[th2].zsa;z++,yr++){
//				i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//				i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(p2fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr+1],p2fm[yr]))));
//				}
//				for(;z<=lptr1[th2].zea;z++,yr++,z2++,yr2++){
//				i1fmm[z]=_mm_add_ps(i1fmm[z],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(p1fm[yr],p2fm[yr2+1])),_mm_mul_ps(coef1,_mm_add_ps(p2fm[yr2],p1fm[yr+1]))));
//				i2fmm[z]=_mm_add_ps(i2fmm[z],_mm_add_ps(_mm_mul_ps(coef2,_mm_add_ps(p2fm[yr],p1fm[yr2+1])),_mm_mul_ps(coef1,_mm_add_ps(p1fm[yr2],p2fm[yr+1]))));
//				}
//				coef1=_mm_set1_ps(lptr2[th2].yr2a);
//				for(;z2<=lptr2[th2].zea;z2++,yr2++){
//				i2fmm[z2]=_mm_add_ps(i2fmm[z2],_mm_add_ps(p1fm[yr2],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr2+1],p1fm[yr2]))));
//				i1fmm[z2]=_mm_add_ps(i1fmm[z2],_mm_add_ps(p2fm[yr2],_mm_mul_ps(coef1,_mm_sub_ps(p2fm[yr2+1],p2fm[yr2]))));
//				}
//				*/
//			}
//
//
//			i1f=image[x][y];
//			i2f=image[x_pixels-x][y];
//			i3f=image[y_pixels-y][x];
//			i4f=image[y_pixels-y][x_pixels-x];
//			i5f=image[x][y_pixels-y];
//			i6f=image[x_pixels-x][y_pixels-y];
//			i7f=image[y][x];
//			i8f=image[y][x_pixels-x];
//			if(x!=x_pixels/2 ){
//				if(x==y){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]=mptr1[yr]+mptr2[yr+2];
//						i2f[z]=mptr1[yr+1]+mptr1[yr+2];
//						i5f[z]=mptr2[yr]+mptr2[yr+3];
//						i6f[z]=mptr1[yr+3]+mptr2[yr+1];
//					}
//				} else if(x<y){
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]=mptr1[yr];
//						i2f[z]=mptr1[yr+1];
//						i3f[z]=mptr1[yr+2];
//						i4f[z]=mptr1[yr+3];
//						i5f[z]=mptr2[yr];
//						i6f[z]=mptr2[yr+1];
//						i7f[z]=mptr2[yr+2];
//						i8f[z]=mptr2[yr+3];
//					}
//				} else {
//					for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//						i1f[z]+=mptr1[yr];
//						i2f[z]+=mptr1[yr+1];
//						i3f[z]+=mptr1[yr+2];
//						i4f[z]+=mptr1[yr+3];
//						i5f[z]+=mptr2[yr];
//						i6f[z]+=mptr2[yr+1];
//						i7f[z]+=mptr2[yr+2];
//						i8f[z]+=mptr2[yr+3];
//					}
//				}
//			} else {
//				for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//					i1f[z]+=mptr1[yr];
//					i3f[z]+=mptr1[yr+2];
//					i5f[z]+=mptr2[yr];
//					i7f[z]+=mptr2[yr+2];
//				}
//			}
//		}
//		y=y_pixels/2;
//		pp1=prj[x];
//		pp2=prj[x+xr_pixels/2];
//		p1fm=(__m128 *)pp1[0];
//		p2fm=(__m128 *)pp2[0];
//
//
//		memcpy(i1fmm,&p1fm[z_start],z_pixels_simd*4*sizeof(__m128));
//		for(th=2,th2=0;th<th_pixels;th+=2,th2++){
//			p1fm=(__m128 *)pp1[th/2];
//			p2fm=(__m128 *)pp2[th/2];
//			for(z=z_start;z<z_end;z++){
//				i1fmm[z]=_mm_add_ps(i1fmm[z],p1fm[z]);//_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//				i1fmm[z]=_mm_add_ps(i1fmm[z],p2fm[z]);//_mm_add_ps(p1fm[yr],_mm_mul_ps(coef1,_mm_sub_ps(p1fm[yr+1],p1fm[yr]))));
//			}
//		}
//		i1f=image[x][y];
//		i2f=image[x_pixels-x][y];
//		i3f=image[y][x];
//		i4f=image[y][x_pixels-x];
//		if(x!=x_pixels/2){
//			for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//				i1f[z]=mptr1[yr];
//				i2f[z]=mptr1[yr+1];
//				i3f[z]=mptr1[yr+2];
//				i4f[z]=mptr1[yr+3];
//			}
//		} else {
//			for(z=z_start,yr=0;z<z_end;z++,yr+=4){
//				i1f[z]=mptr1[yr]+mptr1[yr+2];
//			}
//		}
//	}
//	//	LOG_INFO("done\n");
//	return 1;
//}
//
