/*             
hrrt_sinocor.c
*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <xmmintrin.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <pmmintrin.h>
#include <sys/sysinfo.h>
#include <sys/fcntl.h>
#include <unistd.h>
// O_DIRECT defined here because not found otherwise CM + MS
#define O_DIRECT         040000 /* direct disk access hint */	
#define		DIR_SEPARATOR '/'

#include "scanner_model.h"
#include "hrrt_osem3d.h"
#include "nr_utils.h"
#include "file_io_processor.h"
#include "simd_operation.h"
#include "mm_malloc.h"
#include "gen_delays.h"
#include "write_image_header.h"


#define NHEADS 8
#define HEAD_XSIZE 72
#define HEAD_YSIZE 104

void alloc_tmprprj2();
void alloc_tmprprj1();
void free_tmpprj1();
void free_tmpprj2();

const char *sw_version = "HRRT_U 1.1";
char sw_build_id[80];

/************* For clustering computing *************/
//int mynode;
//int totalnode;
/*****************************************************/

static time_t t0,t1,t2,tb1,tb2,tf1,tf2;        
unsigned long ClockNormfacTheta; /* !sv */
int nthreads = 0;                /* !sv # threads to use in backproj and forwardproj */


/* Definition of scanner parameters */
int   rings=0;
float ring_radius=0.;
float ring_spacing=0.;
float z_size=0.;
float sino_sampling=0.;
float fov=0.;
float rfov=0.;
float tilt=0.;
int   z_pixels=0;
int   z_pixels_simd;
int   z_min=6, z_max=200;    /* z_min and z_max */
//int   z_min=0, z_max=206;    /* z_min and z_max */
int   iModel=999;
/* Definition of scan parameters */
int   radial_pixels=0;
int   views=0;
int   maxdel=0;          /* default max ring difference */
int   span=0;             /* default span */
int   groupmin=0;
int   groupmax=0;
int   defelements=0;
int   defangles=0;

float zoom = 1.0;
float newzoom= 1.0;

int   verbose = 0x0000;//1111;
float rel_fov = (float) 0.95;

/**********************************************/
// IT IS CAME FROM MAIN();
	FILEPTR tsinofp=(FILEPTR)0;
	FILEPTR psinofp=(FILEPTR)0;
	FILEPTR dsinofp=(FILEPTR)0;
	FILEPTR ssinofp=(FILEPTR)0;
	FILEPTR normfp=(FILEPTR)0;
	FILEPTR attenfp=(FILEPTR)0;
	char *true_file=NULL, *prompt_file=NULL, *delayed_file=NULL, *singles_file=NULL;
	char *out_scan_file=NULL,*out_scan3D_file, *out_img_file=NULL;
	char *in_img_file=NULL, *scat_scan_file=NULL,*atten_file=NULL, *norm_file=NULL;
  char ra_smo_file[MAX_PATH];

	bool s2DFlag=0;     /* unscaled 2d segment scatter and scale factors flag */
	bool chFlag=0;      /* smoothed randoms flag, true for .ch file */
	bool muFlag=0;      /* input image is not a mu-map */
  bool precorFlag=0;  /* pre-corrected true sinogram input flag */
	char *imodeldefname;
	int def_elements,def_angles;
  float scan_duration=0.0f;

  bool calnormfacFlag=1;
	char  normfac_img[MAX_PATH];
  ScannerModel  *model_info=NULL;    

/*******************************************/


extern __m128 ***prjinkim128;

short    int *order;       /* array to store the subset order; [subsets] */
float    ***oscan;         /* 3D array to store corrected output scan [views][yr_pixels][xr_pixels] */
short	 ***tmp_for_short_sino_read;//= (short *) malloc(yr_pixels*sizeof(short)*views*xr_pixels);

/*for file processing */
float ***delayedprj3=NULL;  /* 3D array to store whole proj, [theta*segmentsize][views][xr_pixels] */
float ***imageprj3=NULL;    /* 3D array to store whole proj, [theta*segmentsize][views][xr_pixels] */
float ***imageprj=NULL;      /* 3D array to point whole proj, [views/2][segmentsize*2][xr_pixels] */

/*for recon. processing */
float    **estimate;      /* 2D array to store estimated proj (one view), [#of plane][xr_pixels] */
float    **estimate_swap;      /* 2D array to store estimated proj (one view), [#of plane][xr_pixels] */

short    ***largeprjshort; /* 3D array to point whole short proj, [views/2][segmentsize*2][xr_pixels] */
float    ***largeprjfloat; /* 3D array to point whole float proj, [views/2][segmentsize*2][xr_pixels] */

float    ***image;         /* 3D array [z_pixels][x_pixels][y_pixels] to store final image */
float    ***correction;    /* 3D array [z_pixels][x_pixels][y_pixels] to store correction image */

float    ***gradient;
float    sum_normfac=0;
float    sum_image=0;


float    **unitprj;

float	***prjxyf;
float	***imagexyzf;
float	****prjxyf_thread;
float	****imagexyzf_thread;
float   ****normfac_infile;
float   ****normfac_infile_negml;
float   ***estimate_thread;
char    ***imagemask;
int globalview;


int    max_g=-1;
int    min_g=-1;
float image_max = 1.E4;			/* could be a parameter */

int **vieworder;

int npskip=3;       /* number of skipped end segment planes */
int x_rebin = 1;
int v_rebin = 1;

pthread_t threads[16]; // !sv

int back_proj3d_thread1(float ** prj,float *** ima,int view,int numthread,float ***imagebuf,float ***prjbuf);
int forward_proj3d_thread1(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf);	

static void usage() {
  fprintf (stdout,"\nhrrt_sinocor %s Build %s\n\n", sw_version, sw_build_id);
	fprintf (stdout,"usage :\n  hrrt_sinocor -p prompt -d random_smoothed|coinc_histogram  \n");
	fprintf (stdout,"  -i initial_image -n norm -[a atten]  [-s scatter|scatter] -o corrected_sino \n"); 
	fprintf (stdout,"  -g max_group,min_group \n"); 
	fprintf (stdout,"where :\n");
	fprintf (stdout,"  -p  3D_flat_integer_scan (prompt)\n");
	fprintf (stdout,"  -d  3D_flat_float_scan smoothed randoms or coinc_histogram (.ch)\n");
	fprintf (stdout,"  -n  3D_flat_normalisation \n");
	fprintf (stdout,"  -a  3D_flat_attenuation \n");
	fprintf (stdout,"  -i  initial image in flat format forward-projected for gap-filling \n");
  fprintf (stdout,"  -s  input scatter scan in flat 3D format or unscalled 2D followed by scaling factors\n");
	fprintf (stdout,"  -o  output corrected and gap-filled sinogram(flat format) \n");
	fprintf (stdout,"  -g  max group, min group, [groupmax,0] \n");
	fprintf (stdout,"  -m  span,Rd (m-o-gram parameters of all input files) [9,67] \n");
	fprintf (stderr,"  -M  scanner model number [%d] \n",iModel);
  fprintf (stderr,"      HRRT Low Resoltion models: 9991 (2mm), 9992 (2.4375mm)\n");
	fprintf (stderr,"      if iModel==0, needs file name which have Optional parameters \n");
	fprintf (stdout,"  -T  number of threads [# of CPU-core - 2 for linux] \n");    
	fprintf (stdout,"  -L logging_filename[,log_mode] [default is screen]\n");
  fprintf (stdout,"     log_mode:  1=console,2=file,3=file&console\n");    
	fprintf (stdout,"  -v  verbose_level [none = 0]");
	fprintf (stdout,"      1: Scanner info             4: Normalization info \n"); 
	fprintf (stdout,"      8: Reconstruction info     16: I/O info           \n");
	fprintf (stdout,"     32: Processing info         64: Memory allocation info\n");
	exit(1);
}
#define FOPEN_ERROR -1
int file_open(char *fn,char *mode){
	int ret;
	if(mode[0]=='r' && mode[1]=='b'){
		ret= open(fn,O_RDONLY|O_DIRECT); //|O_DIRECT);
		return ret;
	}
}

void file_close(int fp){
	close(fp);
}

/* get_scan_duration  
read: sinogram header and sets scan_duration global variable.
Returns 1 on success and 0 on failure.
*/
bool get_scan_duration(const char *sino_fname)
{
  char hdr_fname[MAX_PATH];
  char line[256], *p=NULL;
  bool found = false;
  FILE *fp;
  sprintf(hdr_fname, "%s.hdr", sino_fname);
  if ((fp=fopen(hdr_fname,"rt")) != NULL) {
    while (!found && fgets(line, sizeof(line), fp) != NULL)
      if ((p=strstr(line,"image duration")) != NULL) {
        if ((p=strstr(p,":=")) != NULL) 
          if (sscanf(p+2,"%g",&scan_duration) == 1) found = true;
      }
      fclose(fp);
  }
  return found;
}

void LogMessage( char *fmt, ... )
{
	//variable argument list
	int nc = 0;	va_list args;
	char printout[_MAX_PATH];
	
	//log entry
	FILE *log_fp=NULL; 
  time_t ltime;
	char LogEntry[_MAX_PATH];

	//format the incoming string + args
	va_start(args, fmt);
	nc = vsprintf(printout, fmt, args);
	va_end(args);

	//prepend the time and date
	time(&ltime);			strcpy(LogEntry,ctime(&ltime));
	LogEntry[24] = '\t';	strcpy(&LogEntry[25],printout);

	//make the log entry
  if (log_file != NULL) log_fp = fopen(log_file,"a+");
  if(log_fp != NULL) {
	  fprintf(log_fp,"%s",LogEntry);
	  fclose(log_fp);
  }
  else fprintf(stdout,"%s",LogEntry);
}

/***********************************************************************
apply_scale_factors routine extracts sinograms from unscaled 2D segment
for a given segment (ISSRB) and apply scale factor to the segment data.
*/
void apply_scale_factors(const float *in, const float *scale_factors,
                         int theta, float *out)
{
  int i = 0, j=xr_pixels*views*yr_bottom[theta];
  for(int yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++) {
    float sf = scale_factors[segment_offset[theta]+yr];
    for(int v=0;v<views;v++) 
      for(int xr=0;xr<xr_pixels;xr++)
        out[i++] = in[j++] * sf;
  }
}

FUNCPTR pt_back_proj3d_thread1(void *ptarg) 
{
	FPBP_thread *arg= (FPBP_thread *) ptarg;
  back_proj3d_thread1(arg->prj,arg->ima,arg->view,arg->numthread,arg->imagebuf,arg->prjbuf);
	return 0;
}

/*
* required memory
*   - span9
*         256(resolution) : about 1157M
*         128(resolution) : about  548M
*
* --------------------------------------
* imageprj3 and imageprj
*
*   views = 288 (covers 180 degree).
*
*   for file format                        for processing recon.
*   imageprj3                              imageprj
*   +----------+---------------+------+    +--------------+----------+------+
*   | yr=0     | 0      degree | xr   |    | 0 degree     | yr=0     | xr   |
*   |          | 0.625  degree | xr   |    |              | yr=1     | xr   |
*   |          | 1.25   degree | xr   |    |              | yr=2     | xr   |
*   |          | ...           | xr   |    |              | ...      | xr   |
*   |          | 179.375degree | xr   |    |              | yr=2208  | xr   |
*   | yr=1     | 0      degree | xr   |    | (0+90 degree)| yr=2209  | xr   | <-'yr=0' data of the '0+90 degree'
*   |          | 0.625  degree | xr   |    |              | yr=2210  | xr   | <-'yr=1' data of the '0+90 degree'
*   |          | 1.25   degree | xr   |    |              | yr=2211  | xr   | <-'yr=2' data of the '0+90 degree'
*   |          | ...           | xr   |    |              | ...      | xr   |
*   |          | 179.375degree | xr   |    |              | yr=4417  | xr   | <-'yr=2208' data of the '179.375 degree'
*   | ...      | ...           | ...  |    | ...          | ...      | ...  |
*   | yr=2207  | 0      degree | xr   |    | 89.375 degree| yr=0     | xr   |
*   |          | 0.625  degree | xr   |    |              | yr=1     | xr   |
*   |          | ...           | ...  |    |              | ...      | xr   |
*   |          | 179.375degree | xr   |    |              | yr=2208  | xr   |
*   | yr=2208  | 0      degree | xr   |    |(89.375+90)deg| yr=2209  | xr   | <-'yr=0' data of the '179.375 degree'
*   |          | ...           | ...  |    |              | ...      | ...  |
*   |          | 179.375degree | xr   |    |              | yr=4417  | xr   | <-'yr=2208' data of the '179.375 degree'
*   +----------+---------------+------+    +--------------+----------+------+
*/
void allocation_memory() {
	int i,j;

	imageprj3=(float ***) calloc(segmentsize,sizeof(float **));
	for(i=0;i<segmentsize;i++){
		imageprj3[i]=(float **) calloc(views,sizeof(float*));
		for(j=0;j<views;j++){
			imageprj3[i][j]=(float *) _mm_malloc(xr_pixels*sizeof(float),16);
		}
	}

	if( imageprj3 == NULL ) crash1 ("  Error in allocation memory for imageprj3 !\n");
	else if (verbose & 0x0040) fprintf(stdout,"  Allocate imageprj3      : %d kbytes \n", 
    views/2*segmentsize*2*xr_pixels/256);

  if (chFlag)
  {
    delayedprj3 =(float ***) calloc(segmentsize,sizeof(float **));
    for(i=0;i<segmentsize;i++){
      delayedprj3[i]=(float **) calloc(views,sizeof(float*));
      for(j=0;j<views;j++){
        delayedprj3[i][j]=(float *) _mm_malloc(xr_pixels*sizeof(float),16);
      }
    }
    if( delayedprj3 == NULL ) crash1 ("  Error in allocation memory for delayedprj3 !\n");
    else if (verbose & 0x0040) fprintf(stdout,"  Allocate delayedprj3      : %d kbytes \n",
      views/2*segmentsize*2*xr_pixels/256);
  }

	// 1-b. projection type = for processing recon.
	// 310.7M   / 150.5M
	estimate = (float **) matrixm128(0,segmentsize*2-1,0,xr_pixels/4-1);  //for two views.
	estimate_thread = (float ***) matrix3dm128(0,nthreads-1,0,segmentsize*2-1,0,xr_pixels/4-1);  //for two views.
	imageprj=(float ***) calloc(views/2,sizeof(float **));                 //for all views.
	for(i=0;i<views/2;i++){
		imageprj[i]=(float **) calloc(segmentsize*2,sizeof(float *));
	}
	for(i=0;i<views/2;i++){
    for(j=0;j<segmentsize;j++){
			imageprj[i][j] = imageprj3[j][i];
			imageprj[i][j+segmentsize] = imageprj3[j][i+views/2];
		}
  }
}

typedef struct {
	float **est;
	float **prj;
	short **prjshort;
	float **prjfloat;		// was short and should be float MS+CM
	int   size;
	int   plane_start;
	int   plane_end;
	int   view;
} UPDATE_STRUCT;


float *** alloc_imagexyzf_thread()
{
	int x,y;
	float ***image;
	image=(float ***) _mm_malloc( (size_t) ( x_pixels*sizeof(float **) ),16 );

	for(x=0;x<x_pixels;x++){
			image[x]=(float **) _mm_malloc( (size_t) ( y_pixels*sizeof(float *) ),16 );
		memset(image[x],0,y_pixels*sizeof(float *));
	}
	for(x=0;x<x_pixels;x++){
		for(y=0;y<y_pixels;y++){
			image[x][y]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*sizeof(__m128) ),16 );
			memset(image[x][y],0,z_pixels_simd*sizeof(__m128));
		}
	}

/*	for(x=0;x<x_pixels;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0]-2;y<cylwiny[x][1]+2;y++){
			image[x][y]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*sizeof(__m128) ),64 );
			memset(image[x][y],0,z_pixels_simd*sizeof(__m128));
		}
	}
	for(x=0;x<x_pixels;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0]-2;y<cylwiny[x][1]+2;y++){
			if(image[y][x]==NULL){
				image[y][x]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*sizeof(__m128) ),64 );
				memset(image[y][x],0,z_pixels_simd*sizeof(__m128));
			}
		}
	}
*/
	return image;
}


float *** alloc_imagepack()
{
	int x,y;
	float ***image;
	image=(float ***) _mm_malloc( (size_t) ( x_pixels*sizeof(float **) ),16 );

	for(x=0;x<=x_pixels/2;x++){
			image[x]=(float **) _mm_malloc( (size_t) ( y_pixels*sizeof(float *) ),16 );
			memset(image[x],0,y_pixels*sizeof(float *));
	}

	for(x=0;x<=x_pixels/2;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0]-2;y<=y_pixels/2;y++){
			image[x][y]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*4*sizeof(__m128) ),64 );
			memset(image[x][y],0,z_pixels_simd*4*sizeof(__m128));
		}
	}
	for(x=0;x<x_pixels;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0]-2;y<=y_pixels/2;y++){
			if(image[y][x]==NULL){
				image[y][x]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*4*sizeof(__m128) ),64 );
				memset(image[y][x],0,z_pixels_simd*4*sizeof(__m128));
			}
		}
	}
	return image;
}

float *** alloc_imagexyzf_thread2()
{
	int x,y;
	float ***image;
	float *ptr;
	image=(float ***) _mm_malloc( (size_t) ( x_pixels*sizeof(float **) ),16 );

	for(x=0;x<x_pixels;x++){
			image[x]=(float **) _mm_malloc( (size_t) ( y_pixels*sizeof(float *) ),16 );
		memset(image[x],0,y_pixels*sizeof(float *));
	}

	for(x=0;x<x_pixels/2;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0]-2;y<y_pixels/2;y++){
//			image[x][y]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*sizeof(__m128) ),16 );
			ptr=(float *)_mm_malloc((size_t)(z_pixels_simd*4*sizeof(__m128)),16);
			memset(ptr,0,z_pixels_simd*sizeof(__m128)*4);
			image[x][y]=&ptr[0];
			image[x][y_pixels-y]=&ptr[z_pixels_simd*4];
			image[x_pixels-x][y]=&ptr[z_pixels_simd*8];
			image[x_pixels-x][y_pixels-y]=&ptr[z_pixels_simd*12];
		}
	}
	for(x=0;x<x_pixels/2;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0]-2;y<y_pixels/2;y++){
			if(image[y][x]==NULL){
				ptr=(float *)_mm_malloc((size_t)(z_pixels_simd*4*sizeof(__m128)),16);
				memset(ptr,0,z_pixels_simd*sizeof(__m128)*4);
				image[y][x]=&ptr[0];
				image[y][x_pixels-x]=&ptr[z_pixels_simd*4];
				image[y_pixels-y][x]=&ptr[z_pixels_simd*8];
				image[y_pixels-y][x_pixels-x]=&ptr[z_pixels_simd*12];
			}
		}
	}
	x=x_pixels/2;
	if(cylwiny[x][0]==0 && cylwiny[x][1]==0){
	} else {
		for(y=cylwiny[x][0]-2;y<cylwiny[x][1]+2;y++){
			image[x][y]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*sizeof(__m128) ),16 );
			memset(image[x][y],0,z_pixels_simd*sizeof(__m128));
		}
	}

	x=x_pixels/2;
	if(cylwiny[x][0]==0 && cylwiny[x][1]==0){
	} else {
		for(y=cylwiny[x][0]-2;y<cylwiny[x][1]+2;y++){
			if(image[y][x]==NULL){
				image[y][x]=(float *) _mm_malloc( (size_t) ( z_pixels_simd*sizeof(__m128) ),16 );
				memset(image[y][x],0,z_pixels_simd*sizeof(__m128));
			}
		}
	}

	return image;
}
float *** alloc_prjxyf_thread()
{
	float ***prj;
	int xr,theta;
//	prj=(float ***) matrix3dm128(0,xr_pixels,0,th_pixels/2,0,yr_pixels-1);
	prj=(float ***) _mm_malloc((size_t)(xr_pixels+1)*sizeof(float **),16);
	for(xr=0;xr<xr_pixels+1;xr++){
		prj[xr]=(float **) _mm_malloc((size_t)(th_pixels/2+1)*sizeof(float *),16);
		theta=0;
		prj[xr][0]=(float *) _mm_malloc((size_t)(yr_pixels)*sizeof(__m128),16);
		memset(prj[xr][0],0,(yr_pixels)*sizeof(__m128));
		for(theta=1;theta<th_pixels/2+1;theta++){
//			prj[xr][theta]=(float *) _mm_malloc((size_t)(yr_pixels+1)*sizeof(__m128),16);
			prj[xr][theta]=(float *) _mm_malloc((size_t)(yr_top[theta*2]-yr_bottom[theta*2]+13)*sizeof(__m128),16);
			memset(prj[xr][theta],0,(yr_top[theta*2]-yr_bottom[theta*2]+13)*sizeof(__m128));
			prj[xr][theta]=prj[xr][theta]-yr_bottom[theta*2]*4+4;
		}
	}
	return prj;
}

void alloc_thread_buf()
{
	int i;
//	imagepack=(float ***) matrix3dm128(0,x_pixels/2,0,y_pixels,0,z_pixels_simd*4-1);
//	memset(&imagepack[0][0][0],0,(x_pixels/2+1)*(y_pixels/2+1)*z_pixels_simd*4*sizeof(__m128));
	imagexyzf_thread=(float ****) malloc(nthreads*sizeof(float ***));
	prjxyf_thread=(float ****) malloc(nthreads*sizeof(float ***));
	for(i=0;i<nthreads;i++){
		imagexyzf_thread[i]= (float ***) alloc_imagexyzf_thread();
//		imagexyzf_thread[i]=matrix3dm128(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd*4);
	if( imagexyzf_thread[i] == NULL ) crash1("  Error allocating thread recon buffer !\n");
		prjxyf_thread[i]=(float ***) alloc_prjxyf_thread();
//		prjxyf_thread[i]=(float ***) matrix3dm128(0,xr_pixels,0,th_pixels/2,0,yr_pixels);
		if( prjxyf_thread[i] == NULL ) crash1("  Error allocating thread recon buffer2 !\n");
	}
}
void free_prjxyf_thread(float ***prj)
{
	int xr,theta;
	for(xr=0;xr<xr_pixels+1;xr++){
		_mm_free(prj[xr][0]);
		for(theta=1;theta<th_pixels/2+1;theta++){
			prj[xr][theta]=prj[xr][theta]+yr_bottom[theta*2]*4-4;
			_mm_free(prj[xr][theta]);
		}
		_mm_free(prj[xr]);
	}
	_mm_free(prj);
}

void free_imagexyzf_thread2(float ***image)
{
	int x,y;

	for(x=0;x<x_pixels/2;x++){
		for(y=0;y<y_pixels/2;y++){
			if(image[x][y]!=NULL) _mm_free(image[x][y]);
		}
	}
	LogMessage("free done\n");
	x=x_pixels/2;
	for(y=0;y<y_pixels;y++){
		if(image[x][y]!=NULL) _mm_free(image[x][y]);
	}

	x=x_pixels/2;
	for(y=0;y<y_pixels;y++){
		if(image[y][x]!=NULL) _mm_free(image[y][x]);
	}

	for(x=0;x<x_pixels;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		if(image[x]!=NULL) _mm_free(image[x]);
	}
	_mm_free(image);
}
void free_imagexyzf_thread(float ***image)
{
	int x,y;

	for(x=0;x<x_pixels;x++){
		for(y=0;y<y_pixels;y++){
			if(image[x][y]!=NULL) _mm_free(image[x][y]);
		}
	}

	for(x=0;x<x_pixels;x++){
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		if(image[x]!=NULL) _mm_free(image[x]);
	}
	_mm_free(image);
}
void free_thread_buf()
{
	int i;
//	free_matrix3dm128(imagepack, 0,x_pixels/2,0,y_pixels,0,z_pixels_simd*4-1);
	for(i=0;i<nthreads;i++){
//		if(imagexyzf_thread[i]!=NULL) free_matrix3dm128(imagexyzf_thread[i], 0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd*4);
//		if(prjxyf_thread[i]!=NULL) free_matrix3dm128(prjxyf_thread[i], 0, xr_pixels, 0, th_pixels/2 ,0 ,yr_pixels);
		if(imagexyzf_thread[i]!=NULL) free_imagexyzf_thread(imagexyzf_thread[i]);
		if(prjxyf_thread[i]!=NULL) free_prjxyf_thread(prjxyf_thread[i]);
	}
	free(imagexyzf_thread);
	free(prjxyf_thread);
}

void alloc_imagexyzf()
{
	imagexyzf =(float ***) matrix3dm128(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd-1);//test
	if( imagexyzf == NULL ) crash1("  Memory allocation failure for imagexyzf !\n");
	else if (verbose & 0x0040) fprintf(stdout,"  Allocate imagexyzf       : %d kbytes \n", imagesize/256);  
	memset(&imagexyzf[0][0][0],0,x_pixels*y_pixels*z_pixels_simd*sizeof(__m128));
}

void alloc_prjxyf()
{
	prjxyf=(float ***) matrix3dm128(0,xr_pixels,0,th_pixels/2,0,yr_pixels-1);
	if( prjxyf == NULL ) crash1("  Memory allocation failure for prjxyf !\n");
	else if (verbose & 0x0040) fprintf(stdout,"  Allocate prjxyf       : %d kbytes \n", xr_pixels*(th_pixels/2+1)*yr_pixels*16/1024);    
}

void free_imagexyzf()
{
	free_matrix3dm128(imagexyzf, 0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free imagexyzf             : %d kbytes \n", imagesize/256);
}

void free_prjxyf()
{
	free_matrix3dm128(prjxyf,0,xr_pixels,0,th_pixels/2,0,yr_pixels-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free prjxyf             : %d kbytes \n", xr_pixels*(th_pixels/2+1)*yr_pixels*16/1024);
}
typedef struct {
	int nthreads;
	char *true_file;
	char *prompt_file;
	char *delayed_file;
	char *norm_file;
	char *atten_file;
	char *out_scan3D_file;
	char *in_img_file;
	bool muFlag;
	char *scat_scan_file;
	int max_g;
	int min_g;
	float rel_fov;
	int verbose;
	int span;
	int maxdel;
	int iModel;
	char *imodeldefname;
} GLOBAL_OSEM3DPAR;
GLOBAL_OSEM3DPAR *osem3dpar;

void InitParameter()
{
	GLOBAL_OSEM3DPAR *p=osem3dpar;

	p->nthreads=nthreads;
	p->true_file=true_file;
	p->prompt_file=prompt_file;
	p->delayed_file=delayed_file;
	p->norm_file=norm_file;
	p->atten_file=atten_file;
	p->out_scan3D_file=out_scan3D_file;
	p->in_img_file=in_img_file;
	p->muFlag=muFlag;
	p->scat_scan_file=scat_scan_file;
	p->max_g=max_g;
	p->min_g=min_g;
	p->verbose=verbose;
	p->span=span;
	p->maxdel=maxdel;
	p->iModel=iModel;
	p->imodeldefname=imodeldefname;
}

void PutParameter()
{
	GLOBAL_OSEM3DPAR *p=osem3dpar;

	nthreads=p->nthreads;
	true_file=p->true_file;
	prompt_file=p->prompt_file;
	delayed_file=p->delayed_file;
	norm_file=p->norm_file;
	atten_file=p->atten_file;
	out_scan3D_file=p->out_scan3D_file;
	in_img_file=p->in_img_file;
	muFlag=p->muFlag;
	scat_scan_file=p->scat_scan_file;
	max_g=p->max_g;
	min_g=p->min_g;
	verbose=p->verbose;
	span=p->span;
	maxdel=p->maxdel;
	iModel=p->iModel;
	imodeldefname=p->imodeldefname;
}

void GetParameter(int argc,char **argv)
{
	int i;
  unsigned int n=0;
	char *optarg;
	char c;
  FILE *log_fp=NULL;
	
  log_file[0] = '\0'; // no log_file
	InitParameter();
  sprintf(sw_build_id,"%s %s", __DATE__, __TIME__);
	for(i=0;i<argc;i++) {

		if(argv[i][0]!='-') continue;
		c=argv[i][1];
		if(argv[i][2]==0){
			if(i<argc-1 ){
				if(argv[i+1][0]!='-') i++;
			}
			optarg=argv[i];
		}    
		else {
			optarg=(char *) &argv[i][2];
		}
		switch (c) {
			 case 'T' :
				 if (sscanf(optarg,"%d",&osem3dpar->nthreads) != 1)
					 crash2("error decoding -T %s\n",optarg);
				 break;
			 case 'p' :
				 osem3dpar->prompt_file = optarg;
				 break;
       case 'F': // gapfill corrected sinogram
         osem3dpar->true_file = optarg;
				 precorFlag=1; 
         break;
			 case 'd' :
				 osem3dpar->delayed_file = optarg;
				 break;
			 case 'n':
				 osem3dpar->norm_file = optarg;
				 break;
			 case 'a':
				 osem3dpar->atten_file = optarg;
				 break;
			 case 'o':
				 osem3dpar->out_scan3D_file = optarg;
				 break;
			 case 'i':
				 osem3dpar->in_img_file = optarg;
				 break;
			 case 'L':
				 if (sscanf(optarg,"%s,%d",log_file,&n) == 2) log_mode = n;
         else log_mode = LOG_TO_FILE;
         if (log_mode==0 || log_mode>(LOG_TO_CONSOLE|LOG_TO_FILE))
           log_mode = LOG_TO_FILE; // default
         if ((log_fp=fopen(log_file,"wt")) == NULL)
           crash2("error creating log file %s\n",log_file);
         fclose(log_fp);
			 break;
			 case 's':
				 osem3dpar->scat_scan_file = optarg;
				 break;
			 case 'g' :
				 if (sscanf(optarg,"%d,%d",&osem3dpar->max_g,&osem3dpar->min_g) < 1)        
					 crash2("error decoding -g %s\n",optarg);
				 break;
			 case 'v':
				 if (sscanf(optarg,"%d",&osem3dpar->verbose) != 1)
					 crash2("error decoding -v %s\n",optarg);
				 break;
			 case 'm' :
				 if (sscanf(optarg,"%d,%d",&osem3dpar->span,&osem3dpar->maxdel) != 2)
					 crash2("error decoding -m %s\n",optarg);
				 break;
			 case 'M':
				 if (sscanf(optarg,"%d",&osem3dpar->iModel) != 1)
					 crash2("error decoding -M %s\n",optarg);
				 if(osem3dpar->iModel==0) {
					 i++;
					 osem3dpar->imodeldefname=argv[i];
				 }
				 break;
			 default:
         LogMessage("\n*****Unknown option '%c' *********\n\n", c);
				 usage();
		}  
	}/* end of while  */
}

void CheckParameterAndPrint()
{
	/* check mandatory arguments & conflicting arguments, calculate dependency-parameters */

	if (in_img_file==NULL || out_scan3D_file==NULL) usage();
  if(true_file==NULL && (prompt_file==NULL || delayed_file==NULL)) usage();
  if (norm_file==NULL) usage();
  LogMessage("  Input image          : %s\n",in_img_file);
	if (true_file!=NULL) 
    LogMessage("  Input true scan      : %s\n",true_file);
	else {
		LogMessage("  Input prompt scan    : %s\n",prompt_file);
		LogMessage("  Input delayed scan   : %s\n",delayed_file);
	}
  LogMessage("  Normalization        : %s\n",norm_file);
  if (atten_file != NULL)
    LogMessage("  Attenuation          : %s\n",atten_file);
  if (scat_scan_file!=NULL)
    LogMessage("  Input scatter scan   : %s\n",scat_scan_file);
  LogMessage("  Output corrected 3D scan: %s\n",out_scan3D_file);
}

void GetScannerParameter()
{
	FILE *tmpfp;

  if(iModel!=0){
    LogMessage("Model %d \n", iModel);
		model_info    = scanner_model(iModel);     /* assumed code for HRRT */
	} else {
    LogMessage("Model %s \n", imodeldefname);	
    model_info=(ScannerModel *) malloc(sizeof(ScannerModel));
		tmpfp=fopen(imodeldefname,"rt");
		if(tmpfp==NULL){
			fprintf(stderr,"Error open %s, iModel=0, please try again with right scanner definition file\n",imodeldefname);
			exit(1);
		}

		fscanf(tmpfp,"%s",imodeldefname);
		//		fscanf(tmpfp,"%s",model_info->number);
		fscanf(tmpfp,"%s",imodeldefname);
		//		fscanf(tmpfp,"%s",model_info->model_name);
		model_info->model_name=imodeldefname;
		fscanf(tmpfp,"%d",&model_info->dirPlanes);
		fscanf(tmpfp,"%d",&model_info->defElements);
		fscanf(tmpfp,"%d",&model_info->defAngles);
		fscanf(tmpfp,"%d",&model_info->crystals_per_ring);
		fscanf(tmpfp,"%f",&model_info->crystalRad);
		fscanf(tmpfp,"%f",&model_info->planesep);
		fscanf(tmpfp,"%f",&model_info->binsize);
		fscanf(tmpfp,"%f",&model_info->intrTilt);
		fscanf(tmpfp,"%d",&model_info->span);
		fscanf(tmpfp,"%d",&model_info->maxdel);
		fclose(tmpfp);
	}

	def_elements  = model_info->defElements;          /* should be 256 */
	def_angles    = model_info->defAngles;            /* should be 288 */
	rings         = model_info->dirPlanes;            /* should be 104 */
	ring_radius   = model_info->crystalRad;
	z_size        = model_info->planesep;             /* plane separation in mm */
	sino_sampling = model_info->binsize;              /* bin_size in mm */


	z_pixels      = rings*2-1;                        /* 104 direct planes => 207 planes */
	//  simd operations on 4 pixels at a time
	if(z_pixels%4==0) z_pixels_simd=z_pixels/4;
	else              z_pixels_simd=z_pixels/4+1;     // when remainder is 1..3 pixels 

	ring_spacing  = 2.0f*z_size;                      /* ring spacing in mm */    
	if(span==0) span=model_info->span;
	if(maxdel==0) maxdel=model_info->maxdel;

	groupmax      = (maxdel - (span-1)/2)/span;       /* should be 7 */
	radial_pixels = def_elements;                     /* global variable */
	views         = def_angles;                       /* global variable */
	fov           = sino_sampling*radial_pixels;
	rfov          = sino_sampling*radial_pixels/2.0f;

	if (z_max ==0) z_max= z_pixels-z_min;		/* symmetrize truncation */

	/* print relevant parameters */
	if (verbose & 0x0001) {
		fprintf(stdout,"  Reconstruction using %1d thread(s)\n", nthreads);
		fprintf(stdout,"  Scanner parameters\n");
		fprintf(stdout,"  -------------------\n");
		fprintf(stdout,"  Number of rings in scanner %d\n",rings);
		fprintf(stdout,"  Number of 2D slices %d\n",z_pixels);
		fprintf(stdout,"  Ring radius %f mm\n",ring_radius);
		fprintf(stdout,"  Ring spacing %f mm\n",ring_spacing);
		fprintf(stdout,"  FOV diameter %f mm\n",fov);
		fprintf(stdout,"  3D Scan parameters\n");
		fprintf(stdout,"  ------------------\n");
		fprintf(stdout,"  Original number of radial elements %d\n",radial_pixels);
		fprintf(stdout,"  Original number of angular elements %d\n",views);
		fprintf(stdout,"  Default number of radial elements %d\n",def_elements);
		fprintf(stdout,"  Default number of angular elements %d\n",def_angles);
		fprintf(stdout,"  Sampling distance %f mm\n",sino_sampling);
		fprintf(stdout,"  Maximum Ring Difference = %d  Span = %d Maximum Group = %d\n",maxdel,span,groupmax);
	}
	/* update variables according to requested parameters */
	if (max_g >=0) {
		groupmax = max_g;
		if(verbose) fprintf(stdout,"  New maximum group %d\n",groupmax);
	} 
	if (min_g >=0) {
		groupmin = min_g;
		if(verbose) fprintf(stdout,"  New minimum group %d\n",groupmin);
	}
}

void CheckFileValidate()
{
  if (true_file!=NULL) {            
		tsinofp = file_open(true_file,"rb");
		if(tsinofp == FOPEN_ERROR) crash2("  Error: cannot open input true sinogram file %s\n",true_file);
	} 
	else {            
		psinofp = file_open(prompt_file,"rb");
		if(psinofp == FOPEN_ERROR) LogMessage("  Error: cannot open input prompt sinogram file '%s'\n",prompt_file);
		if(psinofp == FOPEN_ERROR) crash2("  Error: cannot open input prompt sinogram file %s\n",prompt_file);
		dsinofp = file_open(delayed_file,"rb");
		if(dsinofp == FOPEN_ERROR) LogMessage("  Error: cannot open input delayed sinogram file %s\n",delayed_file);
		if(dsinofp == FOPEN_ERROR) crash2("  Error: cannot open input delayed sinogram file %s\n",delayed_file);
	}

  normfp = file_open(norm_file,"rb");
  if(normfp == FOPEN_ERROR) LogMessage("  Error: cannot open normalization file %s\n",norm_file);
  if(normfp == FOPEN_ERROR) crash2("  Error: cannot open normalization file %s\n",norm_file);

  if (atten_file != NULL) {            
		attenfp = file_open(atten_file,"rb");
		if(attenfp == FOPEN_ERROR) LogMessage("  Error: cannot open attenuation file %s\n",atten_file);    /* attenfp could be null */
		if(attenfp == FOPEN_ERROR) crash2("  Error: cannot open attenuation file %s\n",atten_file);    /* attenfp could be null */
	}

	if(scat_scan_file!=NULL) {            
		ssinofp = file_open(scat_scan_file,"rb");
		if(ssinofp == FOPEN_ERROR) LogMessage("  Error: cannot open input scatter sinogram file %s\n",scat_scan_file);
		if(ssinofp == FOPEN_ERROR) crash2("  Error: cannot open input scatter sinogram file %s\n",scat_scan_file);
	}

  /* check initial image file existence */
  if (access(in_img_file,R_OK) != 0)
    crash2("  Main(): error initial image file %s not found\n", in_img_file );
}

void Output_CorrectedScan()
{
	clock_t ClockA;
	int x,y,z, npixels;
	FILE *tmpfp;
	float **ptr2d;
  short *stmp=NULL; // measured prompts or trues
  float *emc=NULL, *emp=NULL; // corrected sinogram and image projection
  float *ra=NULL;  // smoothed randoms
  float *sc=NULL, *at=NULL, *no=NULL;
  float *sc2d=NULL, *scale_factors=NULL;  // unscaled 2D scatter and scale factors
  int plane=0;
	int yr,v,yr2,i,theta;
  float eps = 1.0e-6f;

  time(&t1);
  ClockA=clock();
  npixels = xr_pixels*views;
  if ((stmp=(short*)calloc(npixels, sizeof(short))) == NULL ||
    (emp=(float*)calloc(npixels, sizeof(float))) == NULL ||
    (emc=(float*)calloc(npixels, sizeof(float))) == NULL)
    crash1("  Main(): memory allocation failure for true 2D sinogram !\n");
  if (tsinofp == NULL)
  {
    if ((ra=(float*)calloc(npixels, sizeof(float))) == NULL)
      crash1("  Main(): memory allocation failure for smoothed randoms 2D sinogram !\n");
  }
  if ((no=(float*)calloc(npixels, sizeof(float))) == NULL)
    crash1("  Main(): memory allocation failure for norm 2D sinogram !\n");
  if (attenfp != NULL)
  {
    if ((at=(float*)calloc(npixels, sizeof(float))) == NULL)
    crash1("  Main(): memory allocation failure for atten 2D sinogram !\n");
  }
  if (ssinofp != NULL)
  {
    if ((sc=(float*)calloc(npixels, sizeof(float))) == NULL)
    crash1("  Main(): memory allocation failure for scatter 2D sinogram !\n");
  }
  if (s2DFlag) { // pre-load unscaled scatter and scale factors
    sc2d = (float*)calloc(npixels*z_pixels, sizeof(float));
    scale_factors = (float*)calloc(segmentsize, sizeof(float));
    if (sc2d==NULL || scale_factors==NULL) 
      crash1("  Output_CorrectedScan: memory allocation error!\n ");
    if( fread(sc2d, sizeof(float), npixels*z_pixels, ssinofp) != 
      npixels*z_pixels)  
      crash1("  Output_CorrectedScan: error reading unscaled 2D scatter!\n");
    if (fread(scale_factors, sizeof(float), segmentsize, ssinofp) != segmentsize)
      crash1("  Prepare_osem3dw0: error reading scatter scale factors!\n");
  }

  image = (float ***) matrix3dm128(0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd-1);  
  if( image == NULL ) crash1("  Main(): matrix3dm128 error! memory allocation failure for image !\n");
  else if (verbose & 0x0008) fprintf(stdout,"  Allocate initial image : %d kbytes \n", imagesize/256);
  if ((tmpfp=fopen(in_img_file,"rb"))==NULL) 
				crash2("  Main(): error in read_flat_image() when reading the initial image from %s \n", in_img_file );
  if ((ptr2d=matrixfloat(0,x_pixels-1,0,y_pixels-1)) == NULL)
    crash1("  Main(): matrixfloat error! memory allocation failure for image !\n");
  for(z=0;z<z_pixels;z++){
    fread(&ptr2d[0][0],x_pixels*y_pixels,sizeof(float),tmpfp);
    for(x=0;x<x_pixels;x++){
      for(y=0;y<y_pixels;y++){
        image[x][y][z]=ptr2d[y][x];
      }
    }
  }
  free_matrix(ptr2d,0,x_pixels-1,0,y_pixels-1);
  fclose(tmpfp);

  alloc_imagexyzf();
  alloc_prjxyf();
  printf("%d %d %d\n",z_pixels,x_pixels,y_pixels);

  //memset(&imageprj3[0][0][0],0,xr_pixels*views*segmentsize*sizeof(float));
  for(yr=0;yr<segmentsize;yr++){
    for(v=0;v<views;v++){
      memset(imageprj3[yr][v],0,xr_pixels*sizeof(float));
    }
  }

  // forward projection done only for 95% FOV, initialize estimate memory
   for(yr2=0;yr2<segmentsize*2;yr2++) 
     memset(estimate[yr2],0,xr_pixels*sizeof(float));
  // forward projection calculates 2 views at one time(v, views/2+v)
  for(v=0;v<views/2;v++)
  {
    printf("  Progress %d %d %%\r",v,(int)((v+0.0)/views*2.0*100));
    if( !forward_proj3d2(image, estimate, v,nthreads,imagexyzf,prjxyf))
      crash1("  Main(): error occurs in forward_proj3d!\n");
    for(yr2=0;yr2<segmentsize*2;yr2++) 
      memcpy(imageprj[v][yr2],estimate[yr2],xr_pixels*sizeof(float));
  }

	time(&t2);
	fprintf(stdout,"  Osem3d forward projection done in %d sec\n",t2-t1);
	tmpfp=fopen(out_scan3D_file,"wb");

	// 	write line by line in sinogram mode 
	yr2=0;
  if ((ptr2d=matrixfloat(0,x_pixels-1,0,y_pixels-1)) == NULL)
    crash1("  Main(): matrixfloat error! memory allocation failure for image !\n");
  plane=0;
	for(theta=0;theta<th_pixels;theta++)
  {
	  //			yr2=segment_offset[theta];
		// printf("yr2=%d\n",yr2);
		for( yr=yr_bottom[theta]; yr<=yr_top[theta]; yr++, yr2++, plane++)
    {
			for( v=0; v<views; v++ )
         memcpy(emp+v*radial_pixels, &imageprj3[yr2][v][0], 
                                     radial_pixels*sizeof(float));
      if (!precorFlag)
      {
        if (tsinofp != NULL)
        { // Read true sinogram plane and convert to float
          if (fread( stmp, npixels*sizeof(short), 1, tsinofp) != 1 )
          crash1("   True sinogram read error\n");
          for (i=0; i<npixels; i++) emc[i] = (float)stmp[i];
        } 
        else
        {  // Read prompt sinogram and substract smoothed randoms
          if (fread(stmp, npixels*sizeof(short), 1, psinofp) != 1 )
            crash1("   Prompt sinogram read error at plane\n");
          if (!chFlag)
          { // read smoothed random from file
            if (fread(ra, npixels*sizeof(float), 1, dsinofp) != 1 )
              crash1("   random smoothed read error at plane\n");
          }
          else
          { // copy smoothed randoms from memory
            for( v=0; v<views; v++ )
              memcpy(ra+v*radial_pixels, &delayedprj3[yr2][v][0],
                                         radial_pixels*sizeof(float));
          }
          for (i=0; i<npixels; i++) emc[i] = (float)stmp[i]-ra[i];
        }
         // Read norm and normalize trues
        if (fread(no, npixels*sizeof(float), 1, normfp) != 1 )
          crash1("   normalization read error at plane\n");
        for (i=0; i<npixels; i++) emc[i] *= no[i];
         if (ssinofp != NULL)
        { // Read scatter sinogram plane and convert normalized trues
          if (s2DFlag)
          {
            float *scptr = sc2d + yr*npixels;
            for (i=0; i<npixels; i++) sc[i] = scptr[i]*scale_factors[plane];
          }
          else
          {
            if (fread( sc, npixels*sizeof(float), 1, ssinofp) != 1 )
              crash1("   Scatter sinogram read error at plane\n");
          }
          for (i=0; i<npixels; i++) emc[i] -= sc[i];
        } 

         if (attenfp != NULL)
         { // Read atten sinogram plane and convert scatter-corrected trues
           if (fread( at, npixels*sizeof(float), 1, attenfp) != 1 )
           crash1("   Attenuation sinogram read error\n");
           for (i=0; i<npixels; i++) emc[i] *= at[i];
         } 
      }
      else
      {
        // Read pre-corrected true sinogram plane
        if (fread( emc, npixels*sizeof(float), 1, tsinofp) != 1 )
          crash1("   pre-corrected true sinogram read error\n");
         // Read norm and replace gaps (norm=0) by image projection to fill the gaps
        if (fread(no, npixels*sizeof(float), 1, normfp) != 1 )
          crash1("   normalization read error at plane\n");
      }  
      for (i=0; i<npixels; i++)
        if (no[i]< eps) emc[i] = emp[i];
      if (fwrite(emc, npixels*sizeof(float), 1, tmpfp) != 1 )
         crash1("  Corrected sinogram write error\n");
        
       printf("   plane %d done\r", yr2);
     }  // end yr
   }  // end theta
	fclose(tmpfp);

  // Write header
  ImageHeaderInfo *Info = (ImageHeaderInfo *) calloc(1,sizeof(ImageHeaderInfo));

	Info->datatype	= 3;		//3=float
  strcpy(Info->imagefile, out_scan3D_file);
	if( true_file	 != NULL ) strcpy( Info->truefile    ,true_file    );
	if( prompt_file	 != NULL ) strcpy( Info->promptfile  ,prompt_file  );
	if( delayed_file != NULL ) strcpy( Info->delayedfile ,delayed_file );
	if( norm_file	 != NULL ) strcpy( Info->normfile    ,norm_file	   );
	if( atten_file	 != NULL ) strcpy( Info->attenfile   ,atten_file   );
	if( scat_scan_file != NULL ) strcpy( Info->scatterfile ,scat_scan_file );
	write_image_header(Info, 0, sw_version, sw_build_id, 1);

	free_imagexyzf();
	free_prjxyf();
  printf("\ndone\n");
}

void GetSystemInformation()
{
	struct sysinfo sinfo;
	LogMessage(" Hardware information: \n");  
	sysinfo(&sinfo);
	printf("  Total RAM: %u\n", sinfo.totalram); 
	printf("  Free RAM: %u\n", sinfo.freeram);
	// To get n_processors we need to get the result of this command "grep processor /proc/cpuinfo | wc -l"
	if(nthreads==0) nthreads=2; // for now
}

int main(int argc, char* argv[])
{
	int nprojs=0,nviews=0;
	unsigned long ClockB=0,ClockF=0,ClockL=0,ClockA=0;
  struct stat st;

	osem3dpar=(GLOBAL_OSEM3DPAR *) malloc(sizeof(GLOBAL_OSEM3DPAR));
	time(&t0);
	t2 = t0;
	x_pixels = y_pixels =0;
	GetSystemInformation();
//	InitParameter();
	GetParameter(argc,argv);

	PutParameter();
  LogMessage("CheckParameterAndPrint \n");
	CheckParameterAndPrint();

  CheckFileValidate();
//	InitParameter();
	GetScannerParameter();

	/* calculate depended variables and some frequently used arrays and constants  - see HRRT_osem3d_sbrt.c */
	if( !dependencies(nprojs, nviews, verbose) ) crash1("  Main(): Error occur in dependencies() !");
  if (ssinofp != (FILEPTR)0) {
    int segmentsize_9 = 2209;
    /* Get scatter file format : 3D or unscaled 2D and scale factors*/
    if (stat(osem3dpar->scat_scan_file, &st) == 0) {
      if (st.st_size == (xr_pixels*views*z_pixels + segmentsize_9)*sizeof(float)) {
        s2DFlag = true;
      } else {
        if (st.st_size != (xr_pixels*views*segmentsize)*sizeof(float)) {
          fprintf(stdout,"%s: invalid file size %d, expected %d\n", scat_scan_file,
            st.st_size, (xr_pixels*views*segmentsize)*sizeof(float));
          exit(1);
        }
      }
    }
    else crash2("%s: Error getting file size\n", scat_scan_file);
  }

  /* Get smoothed  file format : 3D or .ch file*/
  if (dsinofp!= (FILEPTR)0) {
    int ch_size = NHEADS*HEAD_XSIZE*HEAD_YSIZE*4;  // 4 combinations Back/front
    if (stat(osem3dpar->delayed_file, &st) == 0) {
      if (st.st_size == ch_size*sizeof(int)) {
        // ch format: compute smooth randoms
        // compute smoothed randoms into file
        if (!get_scan_duration(prompt_file))
          crash2("Error: scan duration not found in sinogram %s header\n",prompt_file);
        chFlag = true;                // inline random in prepare_osem3dw3
       LogMessage("Inline  randoms from %s, scan duration %g\n", 
          delayed_file, scan_duration);
      } else {
        if (st.st_size != (xr_pixels*views*segmentsize)*sizeof(float)) {
        fprintf(stdout,"%s: invalid file size %d, expected %d\n", delayed_file,
          st.st_size, (xr_pixels*views*segmentsize)*sizeof(float));
        exit(1);
        }
      }
    }  else crash2("%s: Error getting file size\n", delayed_file);
  }

  allocation_memory();

  if (chFlag)
       gen_delays(0, NULL, 2, scan_duration, delayedprj3, dsinofp, NULL,span, maxdel);

  Output_CorrectedScan();
	return 0;
}   /* main()  */
