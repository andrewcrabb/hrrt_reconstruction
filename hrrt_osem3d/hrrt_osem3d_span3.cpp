/*             
hrrt_osem3d.c
Authors:
  Christian Michel (CM)
  Merence Sibomana (MS)
  Stefan Vollmar (sv)
  Inki Hong  (Inki)
  Ziad Burbar (ZB)
  Peter M. Bloomfield (PMB)

Version 1 - April 15, 2000 - C. Michel
- Available weighting methods: 0=UWO3D, 1=AWO3D, 2=ANWO3D

Version 2 - April 21-28, 2000 - After Koln visit...
Change all fopen to add "b" for binary
option -M (method) is now option -W (weighting method)
input scan (-i) is either -t or (-p and -d)
New options....
-f fov: relative fov in %,z_min,z_max (.95,5,200)
-7 write images in Ecat7 format default is flat
-z zoom 
-i initial image in flat format
-s scatter image in flat format
Allows 2 acquisition modes: -t = true or -p = prompt and -d = delayed 
- Add Shifted Poisson Method 4=SPO3D: requires data acquired in true mode and singles in a separate file.
when data acquired in P&D mode, OPO3D would be preferable....

Version 3 - May 10, 2000 
- correct bug in scatter subtraction of initial image estimate
this correction should be done only at iteration 2 
but scatter image is still requested at iteration 3, so add a control Flag (-C)
- Add Ordinary Poisson Method 3=OPO3D (as discussed with M. Defrise)
when data are acquired in  P&D mode...
- The calculation of randoms from singles should still be implemented...
- May 15: at flat write, bug corrected, seek not necessary with open in ab+

Version 4 - May 29, 2000
- output corrected scan for zoomed reconstruction of large objects.
- segment 0 is enough to perform analytical 2D (zoomed and shifted) recon

Version 5 - June 29, 2000
- add m(ogram) option to control span and Rd of input data.
- July 10, correct bug: move "oFlag=1" under -c option (rather then -o option) 

Version 6 - Nov 22, 2000 (sv)
- introduced multi-threaded execution in forward_proj3d and back_proj3d
additional parameter -T controls #threads (usually =#processors),
- added additional benchmarking information, esp. for time spent
in forward and back projection

Version 7 - Dec 28, 2000 (sv)
- added parameter -h to output result of 3D forward projection;
in order to produce a synthetic sinogram, just specify
"-I 0 -i <image_flat> -h <sino>"

Version 8 - Jan 06, 2001 (sv)
- made <nthreads> a global variable; multi-threaded versions of
forward and backproj use same parameter list as original version, so going
from original to optimized vesion just requires a global replace
that adds "_MT" to function name (forward_3D_MT, back_proj3d_MT)

Version 9 - February 2002 : catch up with P39 code
- Allows no normalization, no attenuation for simulated data (and set them to 1), then input scan is float
- When FORE'd data input scan is float - decorrect for atten and use AW scheme (recon group 0 (2D) with OSEM 3D 
- Restore circular FOV and apply to normalization images ....
- Modify OP-scheme 
read 3D scatter scan - rather than image -  made using ISSR from 2D scatter
random are normalized than smoothed outside this code
scatter is produced by SSS (normalized by default)then z-scaled, scatter is not corrected for attenuation... 
- Further decrease the saved image by one pixel (trimFlag, see -f option)  ...
- Remove write_cti_image (-7 never used) 
- Remove SP-OSEM scheme. when delayed acquired, use OP-OSEM. SP-OSEM would require to build delayed scan from singles...
- Add group_min concept to look at image from specific group range
- Remove maxdel/rmax unused in in get_info
- Change stderr to stdout so that logfile can be created with "| tee recon.log"
- Correct rounding error in bkproj.(yr00/yr000 calculation)
- 14 March correct error when decoding -f  option: 1 argument was missing
- correct error when decoding -P (no argument required - remove :) 
- use multithreaded BP/FP code assumes 2 threads is the default
Version 10 - April 2004
- Check if all schemes work with P and separate Delayed since OP scheme is slow
- We assume that smoothed (or unsmoothed) delayed are NOT normalized,
- However scatter is coming from CW code and is normalized
- Allow processing without scatter for all schemes (also in OP case)
*/
/*
Version Inki
- started from version 10 and merged part of P_39 code up to version 14
Version Inki2
- complete version for Linux case 
- add verbosity levels 2: spit out sensitivity and prepared sinogram for debugging purpose...
-                     32: spit out operations performed on input data for sensitivity and data preparation
- check all error messages and fprintf to be clear
- cleanup un-used code
- add -U flag to create 3D attenuation (assumes input mu_map is in cm-1)
- in file_io_processor adapt pagesize for opteron
- look at lines  with CM / MS / ZB  for other changes
Version Inki3 - 13 Oct 2006
- add norm_zero_flag for gap processing (do not calculate gap of sinogram)
- implement new backprojection methods called as "inki's accumulating methods
- remove rotation function in forward-projection, but using instant rotation inside of projector function (twice calculation,but better cache optimization)
Version Inki4 - 13 Oct 2006
- implement compressed reconstruction methods for back & forward projector
- 08-MAY-2008: Bug fix and set default to normfac in memory for x64 OS(M. Sibomana)
- 09-MAY-2008: Added support for unscaled 2D segment and scale factors for scatter input
- 12-MAY-2008: Added default value for scatter or atten when not available (MS)
               Added normfac_path() to create the normfac in the study directory (MS)
- 14-MAY-2008: Added support for .ch file for delayed input (MS)
- 15-MAY-2008: Added file logging (MS)
- 31-July-2008 : Added HRRT Low Resolution 2mm bin size
- 26-AUG-2008: Corrected logging message for PSF 
- 25-SEP-2008: (MS)
              Add -D option to specify the directory where normfac.i and random smoothed
              files are created.
              Make -O option compatible with CPS cluster reconstruction output image filenaming 
              Delete random smoothed file after reconstruction is complete
- 08-OCT-2008: Add sw_version and sw_build_id (MS)
- 13-Nov-2008: Provide pgme path to gendelay call for LUT location
- 14-Nov-2008: fix bug for '-Z' option (inki)
- 02-DEC-2008: Bug fix in searching hrrt_rebinner.lut path (library gen_delay_lib)
- 02-DEC-2008: Change sw_version from 1.0 to 1.0.1, print unknown option
- 07-MAY-2009: Bug fix: changed image_max in GLOBAL_OSEM3DPAR from int fo float (PMB)
               Add span 3 support (creating random smoothed)
- 22-MAY-2009: Add span 3 scatter expansion from 2D scatter and span9 scale factors
- 02-JUL-2009: Add dual logging to console and file
- 31-Aug-2009: Apply block masks to prompts
- 23-Nov-2009: Bug fix in CalculateNormfac()
- 09-DEC-2009: Restore -X 128 option (MS)
               Enable -k option to skip segment end planes
*/


/*
Copyright (C) 2006, Z.H. Cho, Neuroscience Research Institute, Korea, zcho@gachon.ac.kr, http://nri.gachon.ac.kr/english/index.asp
Inki hong, Korea Polytechnic University, Korea, isslhong@kpu.ac.kr

This code is not free software; you cannot redistribute it without permission from Z.H. Cho and Inki Hong.
This code is only for research/educational purpose.
this code is patented and is not supposed to be used in other commercial scanner.
For commercial use, please contact zcho@gachon.ac.kr or isslhong@kpu.ac.kr
*/


// IS_WIN32 and IS__linux__ will be defined by compilation switches
//#define IS_WIN32
#include "compile.h" 	

#ifdef IS_WIN32
#else
#define _REENTRANT
#define _POSIX_SOURCE
#define _P __P
#include <pthread.h>
#include <pmmintrin.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <xmmintrin.h>
#include <fcntl.h>
/* ahc */
#include <linux/types.h>
#include <linux/stat.h>
#include <algorithm>
#ifdef IS_WIN32
#include <windows.h>
#include <process.h>
#include <io.h>
#define stat _stat
#define access _access
#define unlink _unlink
#define		DIR_SEPARATOR '\\'
#define R_OK 4
#else
#include <pthread.h>
#include <pmmintrin.h>
/* ahc */
/* Not available on Mac - host_info has some similar features. */
/* #include <sysinfo.h> */
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
// O_DIRECT defined here because not found otherwise CM + MS
// #define O_DIRECT         040000 /* direct disk access hint */	
#define		DIR_SEPARATOR '/'
#endif

#include "write_image_header.h"
#include "scanner_model.h"
#include "hrrt_osem3d.h"
#include "nr_utils.h"
#include "file_io_processor.h"
#include "simd_operation.h"
#include "mm_malloc.h"
#include <gen_delays_lib/gen_delays.h>
#include <gen_delays_lib/segment_info.h>
#ifndef NO_ECAT_SUPPORT
#include "write_ecat_image.h"
#endif
#define Wnormfact  0
#define Rnormfact  1
#define Normt    2
#define Attent    3
#define Tsinot    4
#define Psinot    5
#define Dsinot    6
#define Scattt    7
#define Cal1    8
#define Cal2    9
#define Cal3    10
#define Cal4    11
#define Cal5    12
#define Cal6    13
#define Cal7    14
#define Cal8    15

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
int segmentsize_s9=2209;  /* scatter always in span 9 */
/* Definition of scan parameters */
int   radial_pixels=0;
int   views=0;
int   maxdel=0;          /* default max ring difference */
int   span=0;             /* default span */
int   groupmin=0;
int   groupmax=0;
int   defelements=0;
int   defangles=0;
#ifdef WIN_X64
int   normfac_in_file_flag=0;
#else
int   normfac_in_file_flag=-1;
#endif
float zoom = 1.0;
float newzoom= 1.0;
int   verbose = 0x0000;//1111;
float rel_fov = (float) 0.95;

/**********************************************/
// IT IS CAME FROM MAIN();
	FILEPTR normfacfp;
	FILEPTR tsinofp;
	FILEPTR psinofp;
	FILEPTR dsinofp;
	FILEPTR ssinofp;
	FILEPTR normfp;
	FILEPTR attenfp;
	char *true_file=NULL, *prompt_file=NULL, *delayed_file=NULL, *singles_file=NULL;
	char *out_scan_file=NULL,*out_scan3D_file, *out_img_file=NULL;
	char *in_img_file=NULL, *scat_scan_file=NULL,*atten_file=NULL, *norm_file=NULL;
  char ra_smo_file[_MAX_PATH];
  char *normfac_dir = NULL;

	int    weighting=2;
	int  iterations=1;
	bool noattenFlag=1; /* not use atten.  file */
	bool sFlag=0;       /* not use scatter file */
	bool s2DFlag=0;     /* unscaled 2d segment scatter and scale factors flag */
  AttenInputType attenType=Atten3D; /* input file type for attenuation correction */
	bool iFlag=0;       /* not use initial img. */
	bool oFlag=0;       /* not does output scan(2D)     */
	bool o3Flag=0;      /* not does output scan(3D)     */
	bool tFlag=0;       /* not use true sinogram file   */
	bool pFlag=0;       /* not use prompt sinogram file */
	bool chFlag=0;      /* smoothed randoms flag, true for .ch file */
	bool nFlag=0;       /* not use normalize file       */
	bool inflipFlag=0;  /* controls flipping of input image */
	bool outflipFlag=0; /* controls flipping of output image */
	bool saveFlag=0;    /* controls saving image at every iteration */
	bool trimFlag=0;    /* trimming reconstructed FOV by 1 pixel */
	int positFlag=0;
	bool muFlag=0;      /* input image is not a mu-map */
	int npskip=3;       /* number of skipped end segment planes */
	char *imodeldefname;
	int def_elements,def_angles;
  float scan_duration=0.0f;

  bool calnormfacFlag=1;
	char  normfac_img[_MAX_PATH];
  ScannerModel  *model_info=NULL;  

/*******************************************/


int x_rebin = 1;
int v_rebin = 1;
static int num_em_frames=0;
static char em_file_prefix[_MAX_PATH], em_file_postfix[8];

bool mapemflag=0;
extern __m128 ***prjinkim128;

short    int *order=NULL;       /* array to store the subset order; [subsets] */
float    ***oscan=NULL;         /* 3D array to store corrected output scan [views][yr_pixels][xr_pixels] */
short	 ***tmp_for_short_sino_read=NULL;//= (short *) malloc(yr_pixels*sizeof(short)*views*xr_pixels);

/*for file processing */
float    ***tmp_prj1=NULL;      /* 3D array to store temporary proj (one theta), [yr_pixels][views][xr_pixels] */
float    ***tmp_prj2=NULL;      /* 3D array to store temporary proj (one theta), [yr_pixels][views][xr_pixels] */
float    ***largeprj3=NULL;     /* 3D array to store whole proj, [theta*segmentsize][views][xr_pixels] */
short    ***largeprjshort3=NULL;/* 3D array to store whole short proj, [theta*segmentsize][views][xr_pixels] */
float    ***largeprjfloat3=NULL;/* 3D array to store whole float proj, [theta*segmentsize][views][xr_pixels] */

/*for atten. processing */
float    ***atten_image=NULL;   /* 3D array to store attenuation image (mu-map), [z_pixels][x_pixels][y_pixels] */
float    **mu_prj=NULL;         /* 3D array to store temporary mu proj (2 views v,v+90), [xr_pixels][2*segmentsize] */

/*for recon. processing */
float    **estimate=NULL;      /* 2D array to store estimated proj (one view), [#of plane][xr_pixels] */
float    **estimate_swap=NULL;      /* 2D array to store estimated proj (one view), [#of plane][xr_pixels] */

float    ***largeprj=NULL;      /* 3D array to point whole proj, [views/2][segmentsize*2][xr_pixels] */
short    ***largeprjshort=NULL; /* 3D array to point whole short proj, [views/2][segmentsize*2][xr_pixels] */
float    ***largeprjfloat=NULL; /* 3D array to point whole float proj, [views/2][segmentsize*2][xr_pixels] */
float    ***negml_proj_PT=NULL;
float	 ***negml_proj_AN=NULL;
float	 ***negml_proj_ANSR=NULL;

float    ***negml_proj_PT_order=NULL;
float	 ***negml_proj_AN_order=NULL;
float	 ***negml_proj_ANSR_order=NULL;

float    ***largeprj_swap=NULL;
short    ***largeprjshort_swap=NULL;
float    ***largeprjfloat_swap=NULL;

float    ***image=NULL;         /* 3D array [z_pixels][x_pixels][y_pixels] to store final image */
float    ***correction=NULL;    /* 3D array [z_pixels][x_pixels][y_pixels] to store correction image */
float    ***normfac=NULL;       /* 3D array [z_pixels][x_pixels][y_pixels] to store normalization factors */
float    ***normfac_logml=NULL;       /* 3D array [z_pixels][x_pixels][y_pixels] to store log_ml normalization factors */
float    ***normfac_negml=NULL;
float    ***imagepack=NULL;
float    ***image_psf=NULL;

float    ***gradient=NULL;
float    ***laplacian=NULL;
float    sum_normfac=0;
float    sum_image=0;


float    **unitprj=NULL;

float	***prjxyf=NULL;
float	***imagexyzf=NULL;
float	****prjxyf_thread=NULL;
float	****imagexyzf_thread=NULL;
float   ****normfac_infile=NULL;
float   ****normfac_infile_negml=NULL;
float   ***estimate_thread=NULL;
char    ***imagemask=NULL;
int globalview;


int    max_g=-1;
int    min_g=-1;
float epsilon1 = 5.E-2f;			/* inverse gives maximum of sensitivity image  */
float epsilon2 = 1.E-2f;			/* controls missing data for sensitivity image */
float epsilon3 = 1.E-10f;		/* controls image update 1e-10 on cluster */
float image_max = 1.E4f;			/* could be a parameter */

int floatFlag=0;			/* means that input scan is in float format */
bool precorFlag=0;			/* means that input scan is fully precorrected */
int **vieworder=NULL;

bool CompressFlag=0;		    /* for Compression recon methods */
unsigned char ***compressmask=NULL;
short **compress_num=NULL;
float ***compress_prj=NULL;

int ecat_flag=0;

typedef struct {
	int nthreads;
	char *true_file;
	bool tFlag;
	int floatFlag;
	bool precorFlag;
	bool CompressFlag;
	bool inflipFlag;
	bool outflipFlag;
	char *prompt_file;
	bool pFlag;
	char *delayed_file;
	char *norm_file;
	bool nFlag;
	char *atten_file;
	bool noattenFlag;
	char *out_img_file;
	char *out_scan3D_file;
	bool o3Flag;
	bool saveFlag;
	char *in_img_file;
	bool iFlag;
	bool muFlag;
	char *scat_scan_file;
	bool sFlag;
	int max_g;
	int min_g;
	int iterations;
	int subsets;
	int weighting;
	float zoom;
	float newzoom;
	float rel_fov;
	bool trimFlag;
	int z_min;
	int z_max;
	int verbose;
	int span;
	int maxdel;
	int positFlag;
	float image_max;	// PMB changed to float from int
	float epsilon1;
	float epsilon2;
	float epsilon3;
	int npskip;
	int normfac_in_file_flag;
	int iModel;
	char *imodeldefname;
	bool calnormfacFlag;
	
	// claude comtate PSF modeling parameters
	float blur_fwhm1;
	float blur_fwhm2;
	float blur_ratio;
	bool blur;
	bool logmlflag;
	bool mapemflag;

} GLOBAL_OSEM3DPAR;
GLOBAL_OSEM3DPAR *osem3dpar;

#ifdef WIN32
File_structure fstruct[16];
HANDLE threads[16];
#else
pthread_t threads[16]; // !sv
#endif
	// claude comtate PSF modeling parameters
float blur_fwhm1;
float blur_fwhm2;
float blur_ratio;
bool blur=0;
Psf1D *psfx=NULL;
Psf1D *psfy=NULL;
Psf1D *psfz=NULL;


FUNCPTR pt_osem3d_proj_thread1(void *ptarg) ;
int back_proj3d_thread1(float ** prj,float *** ima,int view,int numthread,float ***imagebuf,float ***prjbuf);
int forward_proj3d_thread1(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf);	
//int back_proj3d_thread1_multiframe(float ** prj,float *** ima,int view,int numthread,float ***imagebuf,float ***prjbuf);
//int forward_proj3d_thread1_multiframe(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf);	
//int back_proj3d_thread1_compress(float ** prj,float *** ima,int view,int numthread,float ***imagebuf,float ***prjbuf);
//int forward_proj3d_thread1_compress(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf);	
void update_estimate_w012(float **estimate,float **prj,int nplanes,int xr_pixels,int nthreads);
void update_estimate_w3(float **estimate,float **prj,short **prjshort,float **prjfloat,int nplanes,int xr_pixels,int nthreads);
void update_estimate_w3_swap(float **estimate,float **prj,short **prjshort,float **prjfloat,int nplanes,int xr_pixels,int nthreads);
void update_estimate_w3_swap2(float **estimate,float **prj,short **prjshort,float **prjfloat,int nplanes,int xr_pixels,int nthreads);
void update_estimate_negml(float **estimate,int view,int nplanes,int xr_pixels,int nthreads);
void update_estimate_logml(float **estimate,int view,int nplanes,int xr_pixels,int nthreads);
void update_estimate_logml_ratio(float **estimate,int view,int nplanes,int xr_pixels,int nthreads);


static void usage() {
	fprintf (stdout,"usage :\n  hrrt_osem3d -t 3D_scan (or -p 3D_scan -d 3D_scan|coinc_histogram)  \n");
	fprintf (stdout,"  -i initial_image [-n 3D_norm] -[a 3D_atten]  [-s 3D_scatter] -o image (or -O image) -F in_flip,out_flip \n"); 
	fprintf (stdout,"  [-h output corrected 3D scan or attenuation (see -U) ] [-L logfile]\n");
	fprintf (stdout,"  -g max_group,min_group -S number of subsets  -I number of iterations -W weighting method \n"); 
	fprintf (stderr,"	 -e epsilon1,epsilon2,epsilon3 -u image max -k number of planes -w \n"); 
	fprintf (stdout,"  -z zoom -Z zoom \n");
	fprintf (stdout,"  -f rel_fov,trimFlag,zmin,zmax -m span,Rd -T nthreads -v verbose_level]\n");
	fprintf (stdout,"where :\n");
	fprintf (stdout,"  -t  3D_flat_integer_scan (true)\n");
	fprintf (stdout,"  -p  3D_flat_integer_scan (prompt)\n");
	fprintf (stdout,"  -d  3D_flat_float_scan (smoothed un-normalized delayed)) or coinc_histogram (.ch)\n");
	fprintf (stdout,"  -D  Working directory for normfac.i (when applicable)\n");
	fprintf (stderr,"  -P  Flag: input scan is pre-corrected (i.e. float rather than integer format) [not set] \n");
	fprintf (stderr,"  -Q  Flag: input scan is float but is not pre-corrected [not set]\n");
	fprintf (stderr,"  -R  Flag: input scan is 4-bytes integer [not set]\n");
	fprintf (stdout,"  -F  input_flip, output_flip flag, 0/1  no/yes, [0,0] \n");
	fprintf (stdout,"  -N  normfac.i will be in memory if there is enough memory. If it is slower with this option, don't use it\n");
	fprintf (stdout,"  -n  3D_flat_normalisation \n");
	fprintf (stdout,"  -a  3D_flat_attenuation \n");
	fprintf (stdout,"  -i  initial image in flat format default is 1 in FOV specified by -f \n");
	fprintf (stdout,"  -U  initial image is a mu_map in cm-1 \n");
	fprintf (stdout,"  -s  input scatter scan (already normalized) in flat format\n");
	fprintf (stdout,"  -o  output image (flat format) \n");
	fprintf (stdout,"  -O  output image and save each iteration\n");
	//  fprintf (stdout,"  -c  output corrected 2D scan (flat format) \n");
	fprintf (stdout,"  -h  output corrected 3D scan or attenuation (flat format) \n");
	fprintf (stdout,"  -g  max group, min group, [groupmax,0] \n");
	fprintf (stdout,"  -S  number of subsets; [16] \n");
	fprintf (stdout,"  -I  number of iterations; [1]  \n");
	fprintf (stdout,"  -W  weighting method (0=UWO3D, 1=AWO3D, 2=ANWO3D, 3=OPO3D) [2]\n");
	fprintf (stderr,"  -w  Flag: positivity in image space [sinogram space]\n");
	fprintf (stderr,"  -e  epsilon[3] %e,%e,%e\n",epsilon1,epsilon2,epsilon3);
	fprintf (stderr,"  -u  image upper threshold [%f]\n",image_max);
	fprintf (stderr,"  -k  kill end segments with specified planes in image space [0]\n");
	fprintf (stdout,"  -X  image dimension [128]\n");
	fprintf (stdout,"  -z  zoom [1.0] \n");
	fprintf (stdout,"  -Z  zoom [1.0] zoom in projection space \n");
	fprintf (stdout,"  -f  fov as tomographic_fov_in_%%,trimFlag,z_min,z_max [0.95,0,5,201] \n");
	fprintf (stdout,"  -m  span,Rd (m-o-gram parameters of all input files) [9,67] \n");
	fprintf (stderr,"  -M  scanner model number [%d] \n",iModel);
  fprintf (stderr,"      HRRT Low Resoltion models: 9991 (2mm), 9992 (2.4375mm)\n");
	fprintf (stderr,"      if iModel==0, needs file name which have Optional parameters \n");
	fprintf (stdout,"  -L logging_filename[,log_mode] [default is screen]\n");
  fprintf (stdout,"     log_mode:  1=console,2=file,3=file&console\n");    
	fprintf (stdout,"  -T  number of threads [# of CPU-core - 2 for linux] \n");    
	fprintf (stdout,"  -v  verbose_level [none = 0]");
	fprintf (stdout,"      1: Scanner info             4: Normalization info \n"); 
	fprintf (stdout,"      8: Reconstruction info     16: I/O info           \n");
	fprintf (stdout,"     32: Processing info         64: Memory allocation info\n");
	//  fprintf (stdout,"   Version inki - April 5, 2006\n");
	//    fprintf (stdout,"   Version Inki (April 5, 2006) + SMS-MI modifs (CM+MS) - May 2006\n");
	fprintf (stdout,"   Version Inki (Aug. 30, 2006) + SMS-MI modifs (CM+MS) - May 2006\n");
	exit(1);
}

#define FOPEN_ERROR NULL
FILEPTR file_open(const char *fn,char *mode){
	return fopen(fn,mode);
}

void file_close(FILEPTR fp){
	fclose(fp);
}

#ifdef IS_WIN32
void normfac_path(const char *src_filename, char *dest_path)
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char ImageName[_MAX_FNAME];
	char ext[_MAX_EXT];

	if (src_filename != NULL) {
		_splitpath(src_filename, drive, dir, ImageName, ext );
		_makepath(dest_path, drive,dir,"normfac","i");
	}
	else strcpy(dest_path,"normfac.i");
}
# else
void normfac_path(const char *src_filename, char *dest_path)
{
	strcpy(dest_path,"normfac.i");
}
#endif

/* get_scan_duration  
read: sinogram header and sets scan_duration global variable.
Returns 1 on success and 0 on failure.
*/
bool get_scan_duration(const char *sino_fname)
{
  char hdr_fname[_MAX_PATH];
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

static int get_num_frames(const char *em_file)
{
  int frame=0;
  char *ext;
  strcpy(em_file_prefix, em_file);
  if ((ext=strstr(em_file_prefix,".dyn")) == NULL) return 0;
  *ext = '\0';
  strcpy(em_file_postfix,".tr");
  // allocate strings for frame dependent sinogram and image file names
  true_file = (char*)calloc(_MAX_PATH, 1);
  out_img_file = (char*)calloc(_MAX_PATH, 1);
  sprintf(true_file,"%s_frame%d%s.s",em_file_prefix,frame,em_file_postfix);
  if (access(true_file,R_OK) != 0) {// try without postfix
    em_file_postfix[0] = '\0'; 
    sprintf(true_file,"%s_frame%d%s.s",em_file_prefix,frame,em_file_postfix);
  }
  while (access(true_file,R_OK) == 0) {
    frame++;
    sprintf(true_file,"%s_frame%d%s.s",em_file_prefix,frame,em_file_postfix);
  }
  if (frame>0) // allocate frame dependent image file name
    out_img_file = (char*)calloc(_MAX_PATH, 1);
  else  // restore sinogram file name
    strcpy(true_file, em_file);
  return frame;
}


/**************************************************************************************************************/
static int sequence(int sets,short int *order) {

	int i,j,k;
	int *done; 
	int ibest, dbest;
	int d;

	done = (int *)calloc( sizeof(int), sets);
	if( done==NULL ) { 
		fprintf(stdout,"  sequence(): memory allocation failure for done[%d] \n", sets); 
		return -1; 
	}
	for(i=0;i<sets;++i) done[i] = 0;
	order[0] = 0; /*start with arbitrary subset*/
	done[0] = 1;

	for(i=1;i<sets;++i) {		
		/* fprintf(stdout," for subset i = %d ,", i);
		/* check all remaining subsets and take the one with maximum distance to previous subset */
		ibest = 0; dbest = 0; j = 0;
		while(j < sets) {    /*next unprocessed subset*/
			while((done[j] == 1) && (j < sets) ) j += 1;
			if( j<sets )
				if(done[j] == 0){ /* we have found a yet unprocessed subset*/
					/*find distance to previous subset*/
					k = order[i-1];
					d = abs(j-k);
					if((sets-abs(j-k)) < d) 
						d = sets-abs(j-k);
					if(d > dbest) { /*check if this is the largest distance found so far*/
						dbest = d;    
						ibest = j;
					}
					j += 1;
				}
		}
		order[i] = ibest;
		done[ibest] = 1;
	}
	fprintf(stdout,"  Subset order : ");
	for(i=0;i<sets;++i) 
		fprintf(stdout,"  %d  ",order[i]);
	fprintf(stdout,"\n");
	free(done); 
	return 1;
}

/**************************************************************************************************************/
/*    data[x][y] --> data[y][x]   */
static  int  flip_xy(float *data,int x_dim,int y_dim){
	int     x,y;
	float    **tmp; 
	tmp = (float **)matrixfloat(0,y_pixels-1,0,x_pixels-1);
	if( tmp == NULL ) return 0;
	for(y=0;y<y_dim;y++)
		for(x=0;x<x_dim;x++) 
			tmp[y][x] = data[x*y_dim+y]; 
	memcpy(data, &tmp[0][0], x_dim*y_dim*sizeof(float) );
	free_matrix(tmp,0,y_pixels-1,0,x_pixels-1);
	return 1;
}
/*    data[z][x][y] --> data'[z][y][x]   */
static  int  flip_xyz(float ***datain,float *** dataout,int x_dim,int y_dim,int z_dim) {
	int     x,y,z;
	for(z=0; z<z_dim; z++)
		for(y=0;y<y_dim;y++)
			for(x=0;x<x_dim;x++)
				dataout[z][y][x] = datain[z][x][y]; 
	return 1;
}


void make_compress_mask(float ***prj)
{
	int v,xr,yr,yr2,i,j,theta;
	unsigned char *ucmask;
	float *ptr;
	short *sptr;
	int zerocount=0;
	int totalcount=0;
	unsigned long c1,c2;
	c1=clock();

	for(v=0;v<views;v++){
		for(xr=0;xr<xr_pixels;xr++){
			compress_num[v][xr]=0;	
		}
	}
	memset(&compressmask[0][0][0],0,views*xr_pixels*segmentsize/8);
	for(theta=0;theta<th_pixels;theta++){
		yr2=segment_offset[theta];
		for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
			for(v=0;v<views;v++){
				ptr=prj[yr2][v];
				sptr=compress_num[v];
				ucmask=compressmask[v][yr2];
				for(xr=0,i=0;xr<xr_pixels;xr+=8,i++){
					for(j=0;j<8;j++){
						if(ptr[xr+j]>0){
							ucmask[i]|=(0x01)<<j;
							totalcount++;
							sptr[xr+j]++;
						} 
					}
				}
			}
		}
	}
	c2=clock();
	zerocount=xr_pixels*views*segmentsize-totalcount;
	printf("zeorcount = %d\t%d\t%d\t%f\t%f\n",zerocount,totalcount,zerocount+totalcount,(zerocount+0.0)/(zerocount+totalcount),(c2-c1+0.0)/CLOCKS_PER_SEC);
}

void make_compress_mask_short(short ***prj)
{
	int v,xr,yr,yr2,i,j,theta;
	unsigned char *ucmask;
	short *ptr;
	short *sptr;
	int zerocount=0;
	int totalcount=0;
	unsigned long c1,c2;
	c1=clock();

	for(v=0;v<views;v++){
		for(xr=0;xr<xr_pixels;xr++){
			compress_num[v][xr]=0;	
		}
	}
	memset(&compressmask[0][0][0],0,views*xr_pixels*segmentsize/8);
	for(theta=0;theta<th_pixels;theta++){
		yr2=segment_offset[theta];
		for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
			for(v=0;v<views;v++){
				ptr=prj[yr2][v];
				sptr=compress_num[v];
				ucmask=compressmask[v][yr2];
				for(xr=0,i=0;xr<xr_pixels;xr+=8,i++){
					for(j=0;j<8;j++){
						if(ptr[xr+j]>0){
							ucmask[i]|=(0x01)<<j;
							totalcount++;
							sptr[xr+j]++;
						} 
					}
				}
			}
		}
	}
//	for(xr=0;xr<xr_pixels;xr++) printf("view=16\t%d\t%d\n",xr,compress_num[0][xr]);
	c2=clock();
	zerocount=xr_pixels*views*segmentsize-totalcount;
	printf("zeorcount = %d\t%d\t%d\t%f\t%f\n",zerocount,totalcount,zerocount+totalcount,(zerocount+0.0)/(zerocount+totalcount),(c2-c1+0.0)/CLOCKS_PER_SEC);
}

void compress_sinogram_and_alloc_mem(float ***input,float ***out)
{
	int i,j,theta;
	int yr,yr2,v,xr;
//	short *sptr;
	unsigned char *ucmask,uc;
	float *ptr;
	long int num=0;
	int *xrnum;
//	float tmp[512];
	float **outptr;
	long int c1,c2;
//	int xri;
	c1=clock();

	for(v=0;v<views;v++){
		for(xr=0;xr<xr_pixels;xr++){
			num+=compress_num[v][xr];
		}
	}
	printf("totalcount=%ld\n",num);
	xrnum=(int *) calloc(xr_pixels,sizeof(int));
	out=(float ***) matrixfloatptr(0,views-1,0,xr_pixels-1,0,0);
	ptr=(float *) calloc(num,sizeof(float));
	num=0;
	for(v=0;v<views;v++){
		for(xr=0;xr<xr_pixels;xr++){
			out[v][xr]=&ptr[num];
	//		printf("num=%d %d %ld\t%d\n",v,xr,num,compress_num[v][xr]);
			num+=compress_num[v][xr];
		}
	}
	printf("num=%d\n",num);

	for(v=0;v<views;v++){
		memset(&xrnum[0],0,xr_pixels*sizeof(int));
		outptr=out[v];
		for(theta=0;theta<th_pixels;theta++){
			yr2=segment_offset[theta];
			for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
				ptr=input[yr2][v];
				ptr=largeprj3[yr2][v];
				ucmask=compressmask[v][yr2];
				for(xr=0,j=0;xr<xr_pixels;xr+=8,j++){
					uc=ucmask[j];
					for(i=0;i<8;i++){
						if(uc&0x01){
							outptr[xr+i][xrnum[xr+i]]=ptr[xr+i];
							xrnum[xr+i]++;
						}
						uc=uc>>1;
					}
				}
			}
		}
		for(xr=0;xr<xr_pixels;xr++){
			if(xrnum[xr]!=compress_num[v][xr]) printf("error %d %d %d %d\n",v,xr,xrnum[xr],compress_num[v][xr]);
		}
	}
	c2=clock();
	printf("time to compress=\t%f\n",(c2-c1+0.0)/CLOCKS_PER_SEC);
	free(xrnum);
}
/*********************************************************************************************/
/*
* make out array as binary mask from normalization (UW case).
*/
void norm_UW(float ***out,int theta){

	int v,yr,xr;
	float *norm1,*est1;
	int yr2;

	yr2=segment_offset[theta];
	for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
		for(v=0;v<views;v++){ // see norm_ANW
			norm1 = &out[yr2][v][0];
			est1 = &out[yr2][v][0];
			for( xr=0; xr<xr_pixels; xr++ ) if(norm1[xr] > 0.0f) est1[xr] = 1.0f; // else  est1[xr] = 0.f; missing data
		}
	}
}
/*
* out[yr][view][xr] = 1/std::max(A[yr][view][xr],1) (AW case)
*/
void norm_AW(float ***out,float ***atten,int theta) {
	int v,yr,xr;
	float *norm1,*atten1,*est1;
	int yr2;

	yr2=segment_offset[theta];
	for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
		for(v=0;v<views;v++){ // see norm_ANW
			norm1  = &out[yr2][v][0];
			atten1 = &atten[yr][v][0];
			est1   = &out[yr2][v][0];
			for( xr=0; xr<xr_pixels; xr++ ) {
				if(norm1[xr] > 0.0f) est1[xr] = 1.0f/std::max(atten1[xr],1.0f);
				else  est1[xr] = 0.0f; // missing data
			}
		}
	}
}

/*
* out[yr][view][xr] = 1/[N[yr][view][xr]*max(A[yr][view][xr],1)] (ANW case)
*/
void norm_ANW(float ***out,float ***atten,int theta) {
	int v,yr,xr;
	float *norm1,*atten1,*est1;
	float den;
	int yr2;

	yr2=segment_offset[theta];
	for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
		for(v=0;v<views;v++){
			norm1 = &out[yr2][v][0];
			atten1 = &atten[yr][v][0];
			est1  = &out[yr2][v][0];
			for( xr=0; xr<xr_pixels; xr++ ) {
				if (atten1[xr] < 1.0f) atten1[xr]= 1.0f;  /* atten should be > 1. */
				if (norm1[xr] > 0.0f) {
					den = norm1[xr]*atten1[xr];       /* AN */
					if(den > epsilon2 ){
						est1[xr] = 1.0f/den;
					//	est1[xr]=log(1+est1[xr]);
					}
					else  est1[xr] = 0.0f;
				} else est1[xr] = norm1[xr]; // when norm is 0, 1/AN should be 0
			}
		}
	}
}

/**
* make normfac memory array
* result out <= 1/out[][][] will be BKP'ed to get normfac (sensitivity) images.
*/
void prepare_norm(float ***out,int nFlag,int weighting,FILEPTR normfp,FILEPTR attenfp)
{
	int v;
	int xr,yr2,yr;
	int theta;   
	int zerocount=0;
	int totalcount=0;
	int mask[288][288];
	memset(mask,0,views*xr_pixels*4);
	
  if (attenType == Atten3D_Span9) alloc_tmprprj2();

	/* set out(=largeprj3) with normalization file. */
	if (verbose & 0x0020) fprintf(stdout,"  Prepare normalization (sensitivity) data\n");
  if(nFlag) {
		printf("  Read normalization (should show the gaps or missing data) \n");
    for(theta=0;theta<th_pixels;theta++) {
	//		if( !read_flat_float_proj_ori_theta(normfp, &out[segment_offset[theta]][0][0], theta) )        
	//			crash3("  Prepare_norm: error reading normalization at theta  = %d \n ", theta);
			if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj1[0][0][0], theta, 1.0f, 1) )        
				crash3("  Prepare_norm: error reading normalization at theta  = %d \n ", theta);
      yr2=segment_offset[theta];
      for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
        for(v=0;v<views;v++){
          memcpy(&out[yr2][v][0],&tmp_prj1[yr][v][0],xr_pixels*sizeof(float));
        }
      }
		}
  } 
  else {
		if (verbose & 0x0020) fprintf(stdout,"  Assume normalization is 1 everywhere\n");
		for(yr2=0;yr2<segmentsize;yr2++)
			for(v=0;v<views;v++)
				for(xr=0;xr<xr_pixels;xr++)               
					out[yr2][v][xr]=1.0f; // memset should be faster
	}
	
  if(newzoom==1) {// fix bug related with -Z option  // 14-NOV-2008 inki
		for(yr2=0;yr2<segmentsize;yr2++){
			for(v=0;v<views;v++){
				for(xr=0;xr<xr_pixels;xr++){
					if(out[yr2][v][xr]==0) zerocount++;
					else {
						mask[v][xr]=1;
					}
					totalcount++;
				}
			}
		}
  } else {
		for(v=0;v<views;v++){
			for(xr=0;xr<xr_pixels;xr++){
				mask[v][xr]=1;
				totalcount++;
			}
		}
	}

	printf("zero %d %d %f\n",zerocount,totalcount,(zerocount+0.0)/totalcount);
	zerocount=totalcount=0;
	//		fp=fopen("zeromask.raw","wb");
	//		fwrite(mask,256*288,4,fp);
	//		fclose(fp);
  for(v=0;v<views;v++) {
		for(xr=0;xr<xr_pixels;xr++){
			if(mask[v][xr]==0) zerocount++;
			totalcount++;
		}
	}

	printf("zero2 %d %d %f\n",zerocount,totalcount,(zerocount+0.0)/totalcount);
	zerocount=totalcount=0;
  for(v=0;v<views/2;v++) {
		for(xr=1;xr<=xr_pixels/2;xr++){
			if(mask[v][xr]==0 &&mask[v+views/2][xr]==0 &&mask[v][xr_pixels-xr]==0 &&mask[v+views/2][xr_pixels-xr]==0 ){
				zerocount++;
				norm_zero_mask[v][xr]=0;
			} else {
				norm_zero_mask[v][xr]=1;
			}
			totalcount++;
		}
	}

	printf("zero2 %d %d %f\n",zerocount,totalcount,(zerocount+0.0)/totalcount);

	//now, out[yr][view][xr] = normalization projection data.
  if(weighting==0 || weighting==5) { // UW case
		if(nFlag){ // normalization may contain mask containing gaps
			if (verbose & 0x0020) fprintf(stdout,"  Invert normalization as mask\n");
			for(theta=0;theta<th_pixels;theta++) norm_UW(out,theta); // UW works if theta<th_pixels (was theta) CM+ZB
		}  // else the inverse of 1 is one 
	} 

  else if(weighting==1 || weighting==6) {// AW case
   int s3_offset;
   int i1,i2, v1, v2;
    if (verbose & 0x0020) fprintf(stdout,"  Read attenuation and do 1/A\n");
    switch(attenType)
    {
    case Atten3D: // Atten size is prompt size
      for(theta=0;theta<th_pixels;theta++){
        if( !read_flat_float_proj_ori_theta(attenfp, &tmp_prj1[0][0][0],  theta, 1.0f))   
          crash3("  Prepare_norm: error reading attenuation at theta  =%d !\n ", theta);
        norm_AW(out,tmp_prj1,theta);
      }
      break;

    case Atten3D_Span9: // span9 atten for span3 recon
      theta=0;
      fseek(attenfp,0, SEEK_SET); // reset file position
      while (theta<th_pixels) {
        int  nplanes = yr_top[theta]-yr_bottom[theta]+1;
        // read span9 segment 0 or negative segment
        if (fread(&tmp_prj1[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp) != nplanes)
			    crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta);
        if (theta>0)  {  // read positive segment
          if (fread(&tmp_prj2[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp)!= nplanes) 
            crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta+1);
        }
        if (theta==0) {  // compute next 3 segments using  current data          // compute next 3 segments using  current data
          norm_AW(out, tmp_prj1, theta);
          s3_offset = yr_bottom[theta+1] - yr_bottom[theta];
				  norm_AW(out,tmp_prj1+s3_offset,theta+1);
			    norm_AW(out,tmp_prj1+s3_offset,theta+2);
          theta += 3;
        } else { // compute next 6 segments using  current negative and positive theta segment
          norm_AW(out,tmp_prj1,theta);
          norm_AW(out,tmp_prj2,theta+1);
          s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
          norm_AW(out,&tmp_prj1[s3_offset],theta+2);
          norm_AW(out,&tmp_prj2[s3_offset],theta+3);
          s3_offset = yr_bottom[theta+4] - yr_bottom[theta];
          norm_AW(out,&tmp_prj1[s3_offset],theta+4);
          norm_AW(out,&tmp_prj2[s3_offset],theta+5);
          theta += 6;
        }
      }
      break;

    case FullMuMap:
    case RebinnedMuMap:
        read_atten_image(attenfp, atten_image, attenType);
        if (verbose & 0x0020) fprintf(stdout,"  FWDProj mu-map\n");
        for(v1=0, v2=views/2;v1<views/2;v1++, v2++){
          proj_atten_view(atten_image, &mu_prj[0][0],  v1);
          for (i1=0,i2=segmentsize; i1<segmentsize; i1++,i2++){
            for(xr=0;xr<xr_pixels;xr++){
              //v1 view  
              if (out[i1][v1][xr]>0) 
                out[i1][v1][xr] /= std::max(mu_prj[i1][xr], 1.0f);
              else out[i1][v1][xr] = 0.0f; // missing data
              //v2 view (v1+views/2)
              if (out[i1][v2][xr]>0) 
                out[i1][v2][xr] /= std::max(mu_prj[i2][xr], 1.0f);
              else out[i1][v2][xr] = 0.0f; // missing data
			      }
          }
        }
        break;
    }
  }

  else if(weighting==2 || weighting==3 || weighting==4 || weighting==7 || weighting==8|| weighting==9) {
    int i1,i2, v1, v2;
    int s3_offset=0;
		// ANW or OP 
    if (verbose & 0x0020) fprintf(stdout,"  Read attenuation and do 1/(AN)\n");
    switch(attenType)
    {
    case Atten3D:// Atten size is prompt size
		  for(theta=0;theta<th_pixels;theta++){
			  if( !read_flat_float_proj_ori_theta(attenfp, &tmp_prj1[0][0][0], theta, 1.0f))        
				  crash3("  Prepare_norm: error reading attenuation at theta  =%d !\n ", theta);
			  norm_ANW(out,tmp_prj1,theta);
      }
      break;
    case Atten3D_Span9:// span9 atten for span3 recon
      theta=0;
      fseek(attenfp,0, SEEK_SET); // reset file position
      while (theta<th_pixels) {
        int  nplanes = yr_top[theta]-yr_bottom[theta]+1;
        // read span9 segment 0 or negative segment
        if (fread(&tmp_prj1[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp) != nplanes)
			    crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta);
        if (theta>0)  {  // read positive segment
          if (fread(&tmp_prj2[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp)!= nplanes) 
            crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta+1);
        }
        if (theta==0) {  // compute next 3 segments using  current data          // compute next 3 segments using  current data
          norm_ANW(out, tmp_prj1, theta);
          s3_offset = yr_bottom[theta+1] - yr_bottom[theta];
				  norm_ANW(out,&tmp_prj1[s3_offset],theta+1);
			    norm_ANW(out,&tmp_prj1[s3_offset],theta+2);
          theta += 3;
        } else { // compute next 6 segments using  current negative and positive theta segment
          norm_ANW(out,tmp_prj1,theta);
          norm_ANW(out,tmp_prj2,theta+1);
          s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
          norm_ANW(out,&tmp_prj1[s3_offset],theta+2);
          norm_ANW(out,&tmp_prj2[s3_offset],theta+3);
          s3_offset = yr_bottom[theta+4] - yr_bottom[theta];
          norm_ANW(out,&tmp_prj1[s3_offset],theta+4);
          norm_ANW(out,&tmp_prj2[s3_offset],theta+5);
          theta += 6;
        }
      }
      break;
    case FullMuMap:
    case RebinnedMuMap:
      read_atten_image(attenfp, atten_image, attenType);
      if (verbose & 0x0020) fprintf(stdout,"  FWDProj mu-map\n");
      for(v1=0, v2=views/2;v1<views/2;v1++, v2++){
        proj_atten_view(atten_image, &mu_prj[0][0],  v1);
        for (i1=0,i2=segmentsize; i1<segmentsize; i1++,i2++){
          for(xr=0;xr<xr_pixels;xr++){
            //v1 view  
            if (out[i1][v1][xr]>0) 
              out[i1][v1][xr] /= std::max(mu_prj[i1][xr], 1.0f);
            else out[i1][v1][xr] = 0.0f; // missing data
            //v2 view (v1+views/2)
            if (out[i1][v2][xr]>0) 
              out[i1][v2][xr] /= std::max(mu_prj[i2][xr], 1.0f);
            else out[i1][v2][xr] = 0.0f; // missing data
          }
  	    }
      }     
    }
  }
}

/******************************************************** 
* read sinogram
*
* tFlag = 1 : read true sinogram(short int)
* tFlag = 0 : read prompt and delayed_true.
*             largeprj3 = prompt - delayed_true
*
*/
void readsinogram(float ***largeprj3,FILEPTR tsinofp, FILEPTR psinofp,FILEPTR dsinofp,int tFlag,int floatFlag) 
{
	int theta;
	int yr,yr2,v;
	if(tFlag==1){
		if (verbose & 0x0020) {
			fprintf(stdout,"  Reading true sinogram ");
			if (floatFlag == 0) fprintf(stdout,"  (short)\n");
			else fprintf(stdout,"  (float)\n");
		}
		for(theta=0;theta<th_pixels;theta++){
			if(floatFlag==0){			// short
				if( !read_flat_short_proj_ori_to_float_array_theta(tsinofp, &tmp_prj1[0][0][0],  theta))        
					crash3(" Main(): error occurs in read_flat_short_proj_ori_to_float_array_theta at theta  =%d !\n ", theta);
			} else if(floatFlag==1){	// float data
				if( !read_flat_float_proj_ori_theta(tsinofp, &tmp_prj1[0][0][0],  theta, 0.0f))
					crash3(" Main(): error occurs in read_flat_float_proj_ori_to_float_array_theta at theta  =%d !\n ", theta);
			} else if(floatFlag==2){	// 4byte int
			  if( 1 ) // !sv !read_flat_int_proj_ori_to_float_array_theta(tsinofp, &tmp_prj1[0][0][0],  theta)) 
					crash3(" Main(): error occurs in read_flat_int_proj_ori_to_float_array_theta at theta  =%d !\n ", theta);
			}
			yr2=segment_offset[theta];
			for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
				for(v=0;v<views;v++){
					memcpy(&largeprj3[yr2][v][0],tmp_prj1[yr][v],xr_pixels*sizeof(float));
				}
			}
		}
	}
	if(tFlag==0){
		for(theta=0;theta<th_pixels;theta++){
			if (verbose & 0x0020) {
				fprintf(stdout,"  Reading prompt sinogram %d ",theta);
				if (floatFlag == 0) fprintf(stdout,"  (short)\n");
				else if(floatFlag==1) fprintf(stdout,"  (float)\n");
				else if(floatFlag==2) fprintf(stdout,"  (int)\n");
			}
			if(floatFlag==0){	// short
				if( !read_flat_short_proj_ori_to_float_array_theta(psinofp, &tmp_prj1[0][0][0],  theta))
					crash3("  Readsinogram: error reading short sinogram at theta  =%d !\n ", theta);
			} else if(floatFlag==1){	// float
				if( !read_flat_float_proj_ori_theta(psinofp, &tmp_prj1[0][0][0],  theta, 0.0f))
					crash3("  Readsinogram: error reading float sinogram at theta  =%d !\n ", theta);
			}else if(floatFlag==2){		// 4byte int
			  if( 1 ) // !sv !read_flat_int_proj_ori_to_float_array_theta(psinofp, &tmp_prj1[0][0][0],  theta))
					crash3("  Readsinogram: error reading int sinogram at theta  =%d !\n ", theta);
			}
			yr2=segment_offset[theta];
			for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
				for(v=0;v<views;v++){
					memcpy(&largeprj3[yr2][v][0],tmp_prj1[yr][v],xr_pixels*sizeof(float));
				}
			}
		}
		if (verbose & 0x0020) fprintf(stdout,"  Reading and subtracting delayed sinogram\n");
		for(theta=0;theta<th_pixels;theta++){
			if( !read_flat_float_proj_ori_theta(dsinofp, &tmp_prj1[0][0][0],  theta, 0.0f))        
				crash3("  Readsinogram: error reading float delayed at theta  =%d !\n ", theta);
			subprojection(largeprj3,tmp_prj1,theta); // subtract smoothed random
		}
	}

}

/***********************************************************************
convert_s9_s3 converts span9 scatter scale factors to span 3
*/

int convert_s9_s3(float **pscale_factors)
{
  int i=0, j=0; // span3, span9 indices
  int plane, nplanes, theta=0;
  int s3_offset=0;
  float *sf_s9 = *pscale_factors;
  if (span == 9) return 0;
  *pscale_factors = (float*)calloc(segmentsize, sizeof(float));
  float *sf_s3 = *pscale_factors;
  while(theta<th_pixels){
    // current span3 theta corresponds to span 9 theta/3
    nplanes = yr_top[theta]-yr_bottom[theta]+1;
    for(plane=0; plane<nplanes; plane++, i++, j++)
      sf_s3[i] = sf_s9[j]/3.0f;
    // copy theta to theta+1
    s3_offset = yr_bottom[theta+1] - yr_bottom[theta];
    memcpy(sf_s3+i, sf_s3+i-nplanes+s3_offset, (nplanes-2*s3_offset)*sizeof(float));
    nplanes -= 2*s3_offset;
    i += nplanes;
    // copy theta+1 to theta+2
    s3_offset = yr_bottom[theta+2] - yr_bottom[theta+1];
    memcpy(sf_s3+i, sf_s3+i-nplanes+s3_offset, (nplanes-2*s3_offset)*sizeof(float));
    nplanes -= 2*s3_offset;
    i += nplanes;
    theta += 3;
  }
  free(sf_s9);
  return 1;
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


/***********************************************************************
* this function implements ''max[ corr(t),0 ]'' used with UW-OSEM
* 
* corr(t) = [{p(t)-d(t)}N(t) - s(t)] * a(t)
* true = p(t)-d(t)
*
* if precorFlag =1 , data is fully precorrected, there is nothing to do. - CM
*/
void prepare_osem3dw0(int nFlag,int noattenFlag,int tFlag,int floatFlag,int sFlag,int weighting,
					  FILEPTR normfp,FILEPTR attenfp,FILEPTR tsinofp,FILEPTR psinofp,FILEPTR dsinofp,FILEPTR ssinofp)
{
	int v;
	int xr,yr2,yr;
	int theta;

  if (verbose & 0x0020) fprintf(stdout,"  Prepare_osem3dw0 (unweighted case) \n");
  if(precorFlag==0) {
		if (verbose & 0x0020) fprintf(stdout,"  Read or build true from prompt & delayed \n");
		readsinogram(largeprj3,tsinofp,psinofp,dsinofp,tFlag,floatFlag);		
		//	     (shortTrue * norm - scatter ) * attenuation
    if( sFlag==1 ) {
      float *sc2d=NULL, *scale_factors=NULL;
			if (verbose & 0x0020) fprintf(stdout,"  True - scatter/norm\n");
      if (s2DFlag) { // pre-load unscaled scatter and scale factors
        sc2d = (float*)calloc(xr_pixels*views*z_pixels, sizeof(float));
        scale_factors = (float*)calloc(segmentsize, sizeof(float));
        if (sc2d==NULL || scale_factors==NULL) 
          crash1("  Prepare_osem3dw0: memory allocation error!\n ");
        if( fread(sc2d, sizeof(float), xr_pixels*views*z_pixels, ssinofp) != 
          xr_pixels*views*z_pixels)  
          crash1("  Prepare_osem3dw0: error reading unscaled 2D scatter!\n");
        if (fread(scale_factors, sizeof(float), segmentsize_s9, ssinofp) != segmentsize_s9)
          crash1("  Prepare_osem3dw0: error reading scatter scale factors!\n");
        convert_s9_s3(&scale_factors);
      }
			for(theta=0;theta<th_pixels;theta++){
				if(nFlag==1){
					if (verbose & 0x0020) fprintf(stdout,"  Reading normalization at theta %d\n",theta);
					if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj1[0][0][0],  theta, 1.0f, 1))        
						crash3("  Prepare_osem3dw0: error reading normalization at theta  =%d !\n ", theta);
					if (verbose & 0x0020) fprintf(stdout,"  Normalize scattered trues at theta %d\n",theta);
			     mulprojection(largeprj3,tmp_prj1,theta);
				}
				if (verbose & 0x0020) fprintf(stdout,"  Reading scatter at theta %d\n",theta);
				if (!s2DFlag) {
				  if( !read_flat_float_proj_ori_theta(ssinofp, &tmp_prj2[0][0][0],  theta))  
					crash3("  Prepare_osem3dw0: error reading scatter at theta  =%d !\n ", theta);
				} else apply_scale_factors(sc2d, scale_factors, theta, &tmp_prj2[0][0][0]);
				if (verbose & 0x0020) fprintf(stdout,"  Substract scatter at theta %d\n",theta);
        subprojection(largeprj3,tmp_prj1,theta);
			}
      if (s2DFlag) { 
        free(sc2d);
        free(scale_factors);
      }
    } else {
		  if(nFlag==1){
			  if (verbose & 0x0020) fprintf(stdout,"  Multiply true by normalization \n");
			  for(theta=0;theta<th_pixels;theta++){
				  if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj1[0][0][0],  theta, 1.0f, 1))        
					  crash3("  Prepare_osem3dw0: error reading normalization at theta  =%d !\n ", theta);
				  mulprojection(largeprj3,tmp_prj1,theta);
			  }
		  }
    }
		if(noattenFlag==0){
      int i1,i2, v1, v2;
      int s3_offset=0;
      if (verbose & 0x0020) fprintf(stdout,"  Multiply result by attenuation \n");
      switch(attenType)
      {
      case Atten3D:
			  for(theta=0;theta<th_pixels;theta++){
				  if( !read_flat_float_proj_ori_theta(attenfp, &tmp_prj1[0][0][0],  theta, 1.0f))        
					  crash3("  Prepare_osem3dw0: error reading attenuation at theta  =%d !\n ", theta);
				  mulprojection(largeprj3,tmp_prj1,theta);
			  }
        break;
      case Atten3D_Span9:
        theta=0;
        fseek(attenfp,0, SEEK_SET); // reset file position
	      while(theta<th_pixels){
          int  nplanes = yr_top[theta]-yr_bottom[theta]+1;
          // read span9 segment 0 or negative segment
          if (fread(&tmp_prj1[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp) != nplanes)
			      crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta);
          if (theta>0)  {  // read positive segment
            if (fread(&tmp_prj2[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp)!= nplanes) 
              crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta+1);
          }
          if (theta==0) {  // compute next 3 segments using  current data          // compute next 3 segments using  current data
            mulprojection(largeprj3, tmp_prj1, theta);
            s3_offset = yr_bottom[theta+1] - yr_bottom[theta];
				    mulprojection(largeprj3,tmp_prj1+s3_offset,theta+1);
            s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
			      mulprojection(largeprj3,tmp_prj1+s3_offset,theta+2);
            theta += 3;
          } else { // compute next 6 segments using  current negative and positive theta segment
            mulprojection(largeprj3,tmp_prj1,theta);
            mulprojection(largeprj3,tmp_prj2,theta+1);
            s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
            mulprojection(largeprj3,&tmp_prj1[s3_offset],theta+2);
            mulprojection(largeprj3,&tmp_prj2[s3_offset],theta+3);
            s3_offset = yr_bottom[theta+4] - yr_bottom[theta];
            mulprojection(largeprj3,&tmp_prj1[s3_offset],theta+4);
            mulprojection(largeprj3,&tmp_prj2[s3_offset],theta+5);
            theta += 6;
          }
        }
        break;
      case RebinnedMuMap:
      case FullMuMap:
        read_atten_image(attenfp, atten_image, attenType);
		    if (verbose & 0x0020) fprintf(stdout,"  FWDProj mu-map\n");
        for(v1=0, v2=views/2;v1<views/2;v1++, v2++){
          proj_atten_view(atten_image, &mu_prj[0][0],  v1);
          for (i1=0,i2=segmentsize; i1<segmentsize; i1++,i2++){
            for(xr=0;xr<xr_pixels;xr++){
              largeprj3[i1][v1][xr] *= mu_prj[i1][xr];
              largeprj3[i1][v2][xr] *= mu_prj[i2][xr];
            }
			    }
        }
        break;
      }
		}
  }
	else if(precorFlag==1) {
		//assumed fully precorrected.
		if (verbose & 0x0020) fprintf(stdout,"  Reading fully precorrected sinogram \n");
		for(theta=0;theta<th_pixels;theta++){
			//			if( !read_flat_float_proj_ori_theta(tsinofp, &largeprj3[segment_offset[theta]][0][0],  theta))
			if( !read_flat_float_proj_ori_theta(tsinofp, &tmp_prj1[0][0][0],  theta, 0.0f))
				crash3("  Prepare_osem3dw0: error reading precorrected sino at theta  =%d !\n ", theta);
			yr2=segment_offset[theta];
			for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
				for(v=0;v<views;v++){
					memcpy(&largeprj3[yr2][v][0],tmp_prj1[yr][v],xr_pixels*sizeof(float));
				}
			}
		}
	}
	//  Implements max[ corr(t),0 ] positivity in projection space.
	if (verbose & 0x0020) fprintf(stdout,"  Set negative to 0 \n");
	if (weighting==0) setnegative2zero(largeprj); // CM: why test weigthing, should be 0 ??
}

/*
*
* this function implements ''max[ corr(t),0 ]'' with AW-OSEM
* 
* corr(t) = [{p(t)-d(t)}N(t) - s(t)]
* true = p(t)-d(t)
*
* if precorrFlag =1 , data is fully precorrected. 
* corr(t) = [fc(t)/a(t)]  fully corrected data should be decorrected for attenuation - CM
*/
void prepare_osem3dw1(int nFlag,int noattenFlag,int tFlag,int floatFlag,int sFlag,int weighting,
					  FILEPTR normfp,FILEPTR attenfp,FILEPTR tsinofp,FILEPTR psinofp,FILEPTR dsinofp,FILEPTR ssinofp)
{
	int v;
	int xr,yr2,yr;
	int theta;
	if (verbose & 0x0020) fprintf(stdout,"  Prepare_osem3dw1 (attenuation weighted case) \n");
	if(precorFlag==0){
		if (verbose & 0x0020)fprintf(stdout,"  Read or build true from prompt & delayed \n");
		readsinogram(largeprj3,tsinofp,psinofp,dsinofp,tFlag,floatFlag);
		//	scatter processing
		if(sFlag==1){
      float *sc2d=NULL, *scale_factors=NULL;
			if(nFlag) {
				if (verbose & 0x0020) fprintf(stdout,"  subtract un-normalized scatter \n");
			} else {
				if (verbose & 0x0020) fprintf(stdout,"  subtract scatter (already unnormalized) \n");
			}
      if (s2DFlag) { // pre-load unscaled scatter and scale factors
        sc2d = (float*)calloc(xr_pixels*views*z_pixels, sizeof(float));
        scale_factors = (float*)calloc(segmentsize, sizeof(float));
        if (sc2d==NULL || scale_factors==NULL) 
          crash1("  Prepare_osem3dw1: memory allocation error!\n");
        if( fread(sc2d, sizeof(float), xr_pixels*views*z_pixels, ssinofp) != 
          xr_pixels*views*z_pixels)  
          crash1("  Prepare_osem3dw1: error reading unscaled scatter!\n");
        if (fread(scale_factors, sizeof(float), segmentsize_s9, ssinofp)!= segmentsize_s9)
          crash1("  Prepare_osem3dw1: error reading scatter scale factors!\n");
        convert_s9_s3(&scale_factors);

      }
			for(theta=0;theta<th_pixels;theta++){
				if (!s2DFlag) {
          if( !read_flat_float_proj_ori_theta(ssinofp, &tmp_prj1[0][0][0],  theta, 0.0f))  
            crash3("  Prepare_osem3dw1: error reading scatter at theta  =%d !\n ", theta);
        } else apply_scale_factors(sc2d, scale_factors, theta, &tmp_prj1[0][0][0]);
				if(nFlag==1){
					if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj2[0][0][0],  theta, 1.0f, 1))
						crash3("  Prepare_osem3dw1: error reading normalization at theta  =%d !\n ", theta);
					yr2=segment_offset[theta];
					for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
						for(v=0;v<views;v++){
							for(xr=0;xr<xr_pixels;xr++){
								if(tmp_prj2[yr][v][xr]>0.0f) 
									// 				   e = t - s/n
									largeprj3[yr2][v][xr]=largeprj3[yr2][v][xr]-tmp_prj1[yr][v][xr]/tmp_prj2[yr][v][xr];
								else largeprj3[yr2][v][xr]=0;
							}
						}
					}
				} else if(nFlag==0){ 
					//		    e = t - s (scatter already unnormalized)
					subprojection(largeprj3,tmp_prj1,theta);
				}
			}
      if (s2DFlag) { 
        free(sc2d);
        free(scale_factors);
      }
		}
		if (nFlag ==1) { // CM if exists
			//     	  normalization processing
			if (verbose & 0x0020) fprintf(stdout,"  Normalize result \n");
			for(theta=0;theta<th_pixels;theta++){
				if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj1[0][0][0],  theta, 1.0f, 1))        
					crash3("  Prepare_oseme3dw1: error reading normalization at theta  =%d !\n ", theta);            
				//           e= t*n-s 
				mulprojection(largeprj3,tmp_prj1,theta);
			}
		}
	}    
	else if(precorFlag==1){
    int s3_offset;
    int i1,i2, v1, v2;
		//load fully precorrected sinogram
		if (verbose & 0x0004) fprintf(stdout,"  Reading precorrected sinogram\n");
		for(theta=0;theta<th_pixels;theta++){
			if( !read_flat_float_proj_ori_theta(tsinofp, &largeprj3[segment_offset[theta]][0][0],  theta, 0.0f))        
				crash3("  Prepare_osem3dw1: error reading precorrected true at theta  =%d !\n ", theta);
		}
		//      normalization processing => CM: we should divide precorrected data with attenuation not normalization 
		if (verbose & 0x0020) fprintf(stdout,"  Divide precorrected true by attenuation\n");
    switch (attenType)
    {
    case Atten3D:
      for(theta=0;theta<th_pixels;theta++){
			  if( !read_flat_float_proj_ori_theta(attenfp, &tmp_prj1[0][0][0],  theta, 1.0f))        
				  crash3(" Prepare_osem3dw1: error reading attenuation at theta  =%d !\n ", theta);
        divideprojection(largeprj3, tmp_prj1, theta);
      }
      break;
    case Atten3D_Span9:
      theta=0;
      fseek(attenfp,0, SEEK_SET); // reset file position
	    while(theta<th_pixels){
        int  nplanes = yr_top[theta]-yr_bottom[theta]+1;
        // read span9 segment 0 or negative segment
        if (fread(&tmp_prj1[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp) != nplanes)
			    crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta);
        if (theta>0)  {  // read positive segment
          if (fread(&tmp_prj2[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp)!= nplanes) 
            crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta+1);
        }
        if (theta==0) {  // compute next 3 segments using  current data          // compute next 3 segments using  current data
          divideprojection(largeprj3, tmp_prj1, theta);
          s3_offset = yr_bottom[theta+1] - yr_bottom[theta];
				  divideprojection(largeprj3,tmp_prj1+s3_offset,theta+1);
          s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
			    divideprojection(largeprj3,tmp_prj1+s3_offset,theta+2);
          theta += 3;
        } else { // compute next 6 segments using  current negative and positive theta segment
          divideprojection(largeprj3,tmp_prj1,theta);
          divideprojection(largeprj3,tmp_prj2,theta+1);
          s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
          divideprojection(largeprj3,&tmp_prj1[s3_offset],theta+2);
          divideprojection(largeprj3,&tmp_prj2[s3_offset],theta+3);
          s3_offset = yr_bottom[theta+4] - yr_bottom[theta];
          divideprojection(largeprj3,&tmp_prj1[s3_offset],theta+4);
          divideprojection(largeprj3,&tmp_prj2[s3_offset],theta+5);
          theta += 6;
        }
      }
      break;
    case RebinnedMuMap:
    case FullMuMap:
      read_atten_image(attenfp, atten_image, attenType);
      if (verbose & 0x0020) fprintf(stdout,"  FWDProj mu-map\n");
      for(v1=0,v2=views/2; v1<views/2; v1++,v2++){
        proj_atten_view(atten_image, &mu_prj[0][0],  v1);
        for (i1=0,i2=segmentsize; i1<segmentsize; i1++,i2++){
          for(xr=0;xr<xr_pixels;xr++){
							  //	v1:		  e = (t-s*a)/n
            if(mu_prj[i1][xr]>0) 
              largeprj3[i1][v1][xr]=largeprj3[i1][v1][xr]/mu_prj[i1][xr];
            else largeprj3[i1][v1][xr]=0;
							  //	v1+views/2:		  e = (t-s*a)/n
            if(mu_prj[i2][xr]>0) 
              largeprj3[i1][v2][xr]=largeprj3[i1][v2][xr]/mu_prj[i2][xr];
            else largeprj3[i1][v2][xr]=0;
          }
        }
      }
      break;
    }
	}
	if (verbose & 0x0020) fprintf(stdout,"  Set negative to 0 \n");
	if(weighting==1) setnegative2zero(largeprj); 
}
/*
*
* this function implements ''max[ corr(t),0 ]'' with ANW-OSEM
* 
* corr(t) = {p(t)-d(t)} - s(t)/N(t)
* true = p(t)-d(t)
*
* if precorFlag =1 , data is fully precorrected.
* corr(t) = fc(t)/{a(t)*n(t)}   - CM
*/
void prepare_osem3dw2(int nFlag,int noattenFlag,int tFlag,int floatFlag,int sFlag,int weighting,
					  FILEPTR normfp,FILEPTR attenfp,FILEPTR tsinofp,FILEPTR psinofp,FILEPTR dsinofp,FILEPTR ssinofp)
{
	int v;
	int xr,yr2,yr;
	int theta;

	if (verbose & 0x0020) fprintf(stdout,"  Prepare_osem3dw2 (attenuation normalization weighted case) \n");
	if(precorFlag==0) {
		if (verbose & 0x0020) fprintf(stdout,"  Read or build true sinogram from prompt and delayed \n");
		readsinogram(largeprj3,tsinofp,psinofp,dsinofp,tFlag,floatFlag);
		if(sFlag==1){
			//	  e = t - s/n
      float *sc2d=NULL, *scale_factors=NULL;
			if (verbose & 0x0020) fprintf(stdout,"  Subtract un-normalized scatter\n");
      if (s2DFlag) { // pre-load unscaled scatter and scale factors
        sc2d = (float*)calloc(xr_pixels*views*z_pixels, sizeof(float));
        scale_factors = (float*)calloc(segmentsize, sizeof(float));
        if (sc2d==NULL || scale_factors==NULL) 
          crash1("  Prepare_osem3dw2: memory allocation error!\n");
        if( fread(sc2d, sizeof(float), xr_pixels*views*z_pixels, ssinofp) != 
          xr_pixels*views*z_pixels)  
          crash1("  Prepare_osem3dw3: error reading unscaled 2D scatter!\n");
        if (fread(scale_factors, sizeof(float), segmentsize_s9, ssinofp)!= segmentsize_s9)
          crash1("  Prepare_osem3dw3: error reading scatter scale factors!\n");
        convert_s9_s3(&scale_factors);
      }
			for(theta=0;theta<th_pixels;theta++){
        if (!s2DFlag) {
          if( !read_flat_float_proj_ori_theta(ssinofp, &tmp_prj1[0][0][0],  theta, 0.0f))        
					crash3("  Prepare_osem3dw2: error reading scatter at theta  =%d !\n ", theta);
        } else apply_scale_factors(sc2d, scale_factors, theta, &tmp_prj1[0][0][0]);
				if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj2[0][0][0],  theta, 1.0f, 1))        
					crash3("  Prepare_osem3dw2: error reading normalization at theta  =%d !\n ", theta);
				yr2=segment_offset[theta];
				for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
					for(v=0;v<views;v++){
						for(xr=0;xr<xr_pixels;xr++){
							if(tmp_prj2[yr][v][xr]>0.0f) 
								largeprj3[yr2][v][xr]=largeprj3[yr2][v][xr]-tmp_prj1[yr][v][xr]/tmp_prj2[yr][v][xr];
							else largeprj3[yr2][v][xr]=0.0f;
						}
					}
				}
			}
      if (s2DFlag) { 
        free(sc2d);
        free(scale_factors);
      }
		}  
	}
	if(precorFlag==1){
    int s3_offset;
    int i1,i2, v1, v2;
		//	read fully precorrected sinogram
		if (verbose & 0x0020) fprintf(stdout,"  Reading precorrected sinogram\n");
		for(theta=0;theta<th_pixels;theta++){
			if( !read_flat_float_proj_ori_theta(tsinofp, &largeprj3[segment_offset[theta]][0][0],  theta, 0.0f))        
				crash3("  Prepare_osem3dw2: error reading corrcted sinogram at theta  =%d !\n ", theta);
		}
		// 	e=t/(a*n)
		if (verbose & 0x0020) fprintf(stdout,"  Divide by normalization\n");
    for(theta=0;theta<th_pixels;theta++){
			if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj1[0][0][0],  theta, 1.0f, 1))        
				crash3("  Prepare_osem3dw2: error reading normalization at theta  =%d !\n ", theta);
			yr2=segment_offset[theta];
			for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
				for(v=0;v<views;v++){
					for(xr=0;xr<xr_pixels;xr++){
						if(tmp_prj1[yr][v][xr]>0) 
							largeprj3[yr2][v][xr]/=tmp_prj1[yr][v][xr];
						else largeprj3[yr2][v][xr]=0;
					}
				}
			}
		}
		if (verbose & 0x0020) fprintf(stdout,"  Divide by attenuation\n");
    switch (attenType)
    {
    case Atten3D:
      for(theta=0;theta<th_pixels;theta++){
			  if( !read_flat_float_proj_ori_theta(attenfp, &tmp_prj1[0][0][0],  theta, 1.0f))        
				  crash3("  Prepare_osem3dw2: error reading attenuation at theta  =%d !\n ", theta);
        divideprojection(largeprj3, tmp_prj1, theta);
		  }
      break;
    case Atten3D_Span9:
      theta=0;
      fseek(attenfp,0, SEEK_SET); // reset file position
	    while(theta<th_pixels){
        int  nplanes = yr_top[theta]-yr_bottom[theta]+1;
        // read span9 segment 0 or negative segment
        if (fread(&tmp_prj1[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp) != nplanes)
			    crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta);
        if (theta>0)  {  // read positive segment
          if (fread(&tmp_prj2[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp)!= nplanes) 
            crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta+1);
        }
        if (theta==0) {  // compute next 3 segments using  current data          // compute next 3 segments using  current data
          divideprojection(largeprj3, tmp_prj1, theta);
          s3_offset = yr_bottom[theta+1] - yr_bottom[theta];
				  divideprojection(largeprj3,tmp_prj1+s3_offset,theta+1);
          s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
			    divideprojection(largeprj3,tmp_prj1+s3_offset,theta+2);
          theta += 3;
        } else { // compute next 6 segments using  current negative and positive theta segment
          divideprojection(largeprj3,tmp_prj1,theta);
          divideprojection(largeprj3,tmp_prj2,theta+1);
          s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
          divideprojection(largeprj3,&tmp_prj1[s3_offset],theta+2);
          divideprojection(largeprj3,&tmp_prj2[s3_offset],theta+3);
          s3_offset = yr_bottom[theta+4] - yr_bottom[theta];
          divideprojection(largeprj3,&tmp_prj1[s3_offset],theta+4);
          divideprojection(largeprj3,&tmp_prj2[s3_offset],theta+5);
          theta += 6;
        }
      }
      break;

    case FullMuMap:
    case RebinnedMuMap:
      read_atten_image(attenfp, atten_image, attenType);
      for(v1=0, v2=views/2;v1<views/2;v1++, v2++){
        proj_atten_view(atten_image, &mu_prj[0][0],  v1);
        for (i1=0,i2=segmentsize; i1<segmentsize; i1++,i2++){
          for(xr=0;xr<xr_pixels;xr++){
            // v1
						  if(mu_prj[i1][xr]>0) 
							  largeprj3[i1][v1][xr]/=mu_prj[i1][xr];
						  else largeprj3[i1][v1][xr]=0;
              //v1 + views/2
						  if(mu_prj[i2][xr]>0) 
							  largeprj3[i1][v2][xr]/=mu_prj[i2][xr];
						  else largeprj3[i1][v2][xr]=0;
          }
			  }
		  }
      break;
    }
  }
	if (verbose & 0x0020) fprintf(stdout,"  Set negative to 0 \n");
	if(weighting==2) setnegative2zero(largeprj);
}

/**
* this function implements 
*   		calculating {d(t)*N(t)+s(t)}*a(t) ---> largeprj3
* and		loading prompt ---> largeprjshort3
* with OP-OSEM 
* 
*/

void prepare_osem3dw3(int nFlag,int noattenFlag,int tFlag,int floatFlag,int sFlag,int weighting,
					  FILEPTR normfp,FILEPTR attenfp,FILEPTR tsinofp,FILEPTR psinofp,FILEPTR dsinofp,FILEPTR ssinofp)
{
	int theta;
	int zerocount=0;
	int totalcount=0;
	int yr,yr2,v;
  int s3_offset;
  int i1,i2, v1, v2, xr;

	if (verbose & 0x0020) {
		fprintf(stdout, "  Prepare_osem3dw3 \n   Reading prompt sinogram ");
		if(floatFlag ==0) fprintf(stdout, "  (short) \n");
		else fprintf(stdout, "  (float) \n");
	}

	for(theta=0;theta<th_pixels;theta++){	
		if(floatFlag==0){

//			if( !read_flat_short_proj_ori_to_short_array_theta(psinofp, &largeprjshort3[segment_offset[theta]][0][0],  theta))        
//				crash3("  Prepare_osem3dw3: error reading (short) prompt sinogram at theta  =%d !\n ", theta);
			if( !read_flat_short_proj_ori_to_short_array_theta(psinofp,&tmp_for_short_sino_read[0][0][0],theta))        
				crash3("  Prepare_osem3dw3: error reading (short) prompt sinogram at theta  =%d !\n ", theta);
			yr2=segment_offset[theta];
			for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
				for(v=0;v<views;v++){
					memcpy(&largeprjshort3[yr2][v][0],tmp_for_short_sino_read[yr][v],xr_pixels*sizeof(short));
				}
			}
		} 
		else {
			if( !read_flat_float_proj_ori_theta(psinofp, &largeprjfloat3[segment_offset[theta]][0][0],  theta, 0.0f))        
				crash3("  Prepare_osem3dw3: error reading (float) prompt sinogram at theta  =%d !\n ", theta);

		}
	}
//	printf("zero ratio of sinogram %f %d %d\n",((float)zerocount)/totalcount,zerocount,totalcount);

	// if (verbose & 0x0020)
  if (!chFlag) {
    fprintf(stderr,"  Read delayed \n");
	  for(theta=0;theta<th_pixels;theta++){
		  if( !read_flat_float_proj_ori_theta(dsinofp, &tmp_prj1[0][0][0],  theta, 0.0f))        
			  crash3("  Prepare_osem3dw3: error reading delayed at theta  =%d !\n ", theta);
		  yr2=segment_offset[theta];
		  for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
			  for(v=0;v<views;v++){
				  memcpy(&largeprj3[yr2][v][0],tmp_prj1[yr][v],xr_pixels*sizeof(float));
			  }
      }
    }
  } else { // compute random smoothing from .ch file into largeprj3
    fprintf(stderr,"computing smooth randoms from %s to memory and scan duration %g\n", 
          delayed_file, scan_duration);
    gen_delays(0, NULL, 2, scan_duration, largeprj3, dsinofp, NULL,
                osem3dpar->span, osem3dpar->maxdel);
  }

	//    d(t)*N(t) and apply norm mask to prompts
 // if (verbose & 0x0020)
  LogMessage("  Read normalization and normalize delayed\n");	
  LogMessage("  Apply normalization mask to prompt sinogram\n");
	for(theta=0;theta<th_pixels;theta++){
		if( !read_flat_float_proj_ori_theta(normfp, &tmp_prj1[0][0][0],  theta, 1.0f, 1))        
			crash3("  Prepare_osem3dw3: error reading normalization at theta  =%d !\n ", theta);
		mulprojection(largeprj3,tmp_prj1,theta);
    yr2=segment_offset[theta];
    for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
      for(v=0;v<views;v++){
        for(xr=0;xr<xr_pixels;xr++){
          if(tmp_prj1[yr][v][xr]==0.0f) 
          {
            if (floatFlag==0) largeprjshort3[yr2][v][xr]=0;
            else largeprjfloat3[yr2][v][xr]=0.0f;
          }
        }
      }
    }
  }


	//    d(t)*N(t)+s(t)
  if(sFlag==1) {
    float *sc2d=NULL, *scale_factors=NULL;
		if (verbose & 0x0020) fprintf(stderr,"  Add scatter (assumed normalized) \n");
 
    if (s2DFlag) { // pre-load unscaled scatter and scale factors
      sc2d = (float*)calloc(xr_pixels*views*z_pixels, sizeof(float));
      scale_factors = (float*)calloc(segmentsize, sizeof(float));
      if (sc2d==NULL || scale_factors==NULL) 
        crash1("  Prepare_osem3dw3: memory allocation error!\n");
      if( fread(sc2d, sizeof(float), xr_pixels*views*z_pixels, ssinofp) != 
        xr_pixels*views*z_pixels)  
        crash1("  Prepare_osem3dw3: error reading unscaled 2D scatter!\n");
      if (fread(scale_factors, sizeof(float), segmentsize_s9, ssinofp)!= segmentsize_s9)
        crash1("  Prepare_osem3dw3: error reading scatter scale factors!\n");
       convert_s9_s3(&scale_factors);
    }
		for(theta=0;theta<th_pixels;theta++){
      if (!s2DFlag) {
        if( !read_flat_float_proj_ori_theta(ssinofp, &tmp_prj1[0][0][0],  theta, 0.0f))        
				  crash3("  Prepare_osem3dw3: error reading scatter at theta  =%d !\n ", theta);
      } else apply_scale_factors(sc2d, scale_factors, theta, &tmp_prj1[0][0][0]);
			addprojection(largeprj3,tmp_prj1,theta);
		}
    if (s2DFlag) { 
      free(sc2d);
      free(scale_factors);
    }
	}

  //     (d(t)*N(t)+s(t))*A(t)
	if (verbose & 0x0020) fprintf(stderr,"  Multiply result by attenuation\n");
  switch (attenType)
  {
  case Atten3D:
	  for(theta=0;theta<th_pixels;theta++){
		  if( !read_flat_float_proj_ori_theta(attenfp, &tmp_prj1[0][0][0],  theta, 1.0f))        
			  crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta);
		  mulprojection(largeprj3,tmp_prj1,theta);
    }
    break;
  case Atten3D_Span9:
    theta=0;
    fseek(attenfp,0, SEEK_SET); // reset file position
	  while(theta<th_pixels){
      int  nplanes = yr_top[theta]-yr_bottom[theta]+1;
      // read span9 segment 0 or negative segment
      if (fread(&tmp_prj1[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp) != nplanes)
			  crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta);
      if (theta>0)  {  // read positive segment
        if (fread(&tmp_prj2[0][0][0],sizeof(float)*xr_pixels*views,nplanes,attenfp)!= nplanes) 
          crash3("  Prepare_osem3dw3: error reading attenuation at theta  =%d !\n ", theta+1);
      }
      if (theta==0) {  // compute next 3 segments using  current data
        mulprojection(largeprj3,tmp_prj1,theta);
        s3_offset = yr_bottom[theta+1] - yr_bottom[theta];
        mulprojection(largeprj3,&tmp_prj1[s3_offset],theta+1);
        mulprojection(largeprj3,&tmp_prj1[s3_offset],theta+2);
        theta += 3;
      } else {
        // compute next 6 segments using  current negative and positive theta segment
        mulprojection(largeprj3,tmp_prj1,theta);
        mulprojection(largeprj3,tmp_prj2,theta+1);
        s3_offset = yr_bottom[theta+2] - yr_bottom[theta];
        mulprojection(largeprj3,&tmp_prj1[s3_offset],theta+2);
        mulprojection(largeprj3,&tmp_prj2[s3_offset],theta+3);
        s3_offset = yr_bottom[theta+4] - yr_bottom[theta];
        mulprojection(largeprj3,&tmp_prj1[s3_offset],theta+4);
        mulprojection(largeprj3,&tmp_prj2[s3_offset],theta+5);
        theta += 6;
      }
    }
    break;
  case FullMuMap:
  case RebinnedMuMap:
    read_atten_image(attenfp, atten_image, attenType);
    for(v1=0, v2=views/2;v1<views/2;v1++, v2++){
      proj_atten_view(atten_image, &mu_prj[0][0],  v1);
      for (i1=0,i2=segmentsize; i1<segmentsize; i1++,i2++){
        for(xr=0;xr<xr_pixels;xr++){
          largeprj3[i1][v1][xr] *= mu_prj[i1][xr];
          largeprj3[i1][v2][xr] *= mu_prj[i2][xr];
        }
      }
    }
    break;
  }
}


/*-----------------------------------------------*/
/* for threads which are used to write normfac.i */

FUNCPTR pt_write_norm(void *ptarg) 
{
	File_structure *arg = (File_structure *) ptarg;
	if(normfac_in_file_flag==1){
		;
	} else if(!write_norm(arg->normfac, arg->normfac_img, arg->isubset,arg->verbose) )
		crash3("  Error occurs in write_norm at subset %d !\n", arg->isubset );
	return 0;
}

FUNCPTR pt_read_norm(void *ptarg)
{
	File_structure *arg = (File_structure *) ptarg;
	if(normfac_in_file_flag==1){
		;
		//	memcpy(&(arg->normfac[0][0][0]),&(normfac_infile[arg->isubset][0][0][0]),x_pixels*y_pixels*z_pixels_simd*sizeof(__m128));
	} else	{
		if(normfac_in_file_flag==2){
			if(!read_norm(normfac_infile[arg->isubset], arg->normfac_img, arg->isubset,arg->verbose) )
				crash3("  Error occurs in read_norm at subset %d !\n", arg->isubset );
		} else if(normfac_in_file_flag==-1){
			if(!read_norm(arg->normfac, arg->normfac_img, arg->isubset,arg->verbose) )
				crash3("  Error occurs in read_norm at subset %d !\n", arg->isubset );
		}
	}
	return 0;
}

FUNCPTR pt_normfac_calculate(void *ptarg)
{
	int x,y;
	unsigned short z;
	int xs,xe;
	int thr=*((int *) ptarg);
	float *ptr1,*ptr2;
	__m128 mmsum;
	__m128 *mptr;

	mmsum=_mm_set_ps1(0);


	if(thr==0){
		xs=0;
		xe=x_pixels/2;
	} else {
		xs=x_pixels/2;
		xe=x_pixels;
	}
	for( x=xs; x<xe;x++) {
		//for using SIMD, we use zpixels+1.
		//z_pixels=207, so 207+1=208%4=0. :)
		//memset(&correction[x][0][0],0,y_pixels*z_pixels_simd*sizeof(__m128));
		//for( z=0; z<z_pixels; z++ ) normfac[x][y][z] =  0.0;						
		for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) { 
			ptr1=correction[x][y];
			ptr2=normfac[x][y];
			mptr=(__m128 *) ptr1;
			if(mapemflag){
				memcpy(ptr1,ptr2,z_pixels_simd*4*4);
				for(z=0;z<z_pixels_simd;z++){
					mmsum=_mm_add_ps(mmsum,mptr[z]);
				}
				continue;
			} else {
			for( z=0; z<z_pixels; z++ ) { 
				if( fabs(normfac[x][y][z])> epsilon1 )  ptr1[z] = (1.0f/normfac[x][y][z]);
				else 
					ptr1[z] = 0.;
			}
			}
		}    
	}
	ptr1=(float *) &mmsum;
	sum_normfac=ptr1[0]+ptr1[1]+ptr1[2]+ptr1[3];
	return 0;
}


FUNCPTR pt_back_proj3d_thread1(void *ptarg) 
{
	FPBP_thread *arg= (FPBP_thread *) ptarg;
	

	if(CompressFlag==0){
		back_proj3d_thread1(arg->prj,arg->ima,arg->view,arg->numthread,arg->imagebuf,arg->prjbuf);
	} else {
		back_proj3d_thread1(arg->prj,arg->ima,arg->view,arg->numthread,arg->imagebuf,arg->prjbuf);
	}
	return 0;
}

FUNCPTR pt_osem3d_proj_thread1(void *ptarg) 
{
	FPBP_thread *arg= (FPBP_thread *) ptarg;
	int weighting=arg->weighting;

	if(CompressFlag==0){
		forward_proj3d_thread1(arg->ima,arg->prj,arg->view,arg->numthread,arg->imagebuf,arg->prjbuf);
//		forward_proj3d_thread1_swap_pack(arg->ima,arg->imagepack,arg->prj,arg->view,arg->numthread,arg->imagebuf,arg->prjbuf);
			if (weighting ==0 || weighting ==1 || weighting ==2 || weighting ==5 || weighting ==6 || weighting ==7 ) { 
				update_estimate_w012(arg->prj,arg->volumeprj,segmentsize*2,xr_pixels,1);
			}	        
			if ((weighting == 3 || weighting==8)) {  
				if(floatFlag==0) update_estimate_w3(arg->prj,arg->volumeprj,arg->volumeprjshort,arg->volumeprjfloat,segmentsize*2,xr_pixels,1);
				if(floatFlag==1) update_estimate_w3(arg->prj,arg->volumeprj,NULL,arg->volumeprjfloat,segmentsize*2,xr_pixels,1);
			}
		back_proj3d_thread1(arg->prj,arg->ima, arg->view, 1,arg->imagebuf,arg->prjbuf); 
	} else {
		//forward_proj3d_thread1(arg->ima,arg->prj,arg->view,arg->numthread,arg->imagebuf,arg->prjbuf);
		//if (weighting ==0 || weighting ==1 || weighting ==2 || weighting ==5 || weighting ==6 || weighting ==7 ) { 
		//	update_estimate_w012(arg->prj,arg->volumeprj,segmentsize*2,xr_pixels,1);
		//}	        
		//if ((weighting == 3 || weighting==8)) {  
		//	if(floatFlag==0) update_estimate_w3(arg->prj,arg->volumeprj,arg->volumeprjshort,NULL,segmentsize*2,xr_pixels,1);
		//	if(floatFlag==1) update_estimate_w3(arg->prj,arg->volumeprj,NULL,arg->volumeprjfloat,segmentsize*2,xr_pixels,1);
		//}
		//back_proj3d_thread1_compress(arg->prj,arg->ima, arg->view, 1,arg->imagebuf,arg->prjbuf); 
	}
	return 0;
}

void calculate_derivative_mapem(float ***image,float ***gradient,float ***laplacian)
{
	int x,y,z;
  float omega[3][3][3]={{{ 0.8165f ,1.0f ,0.8165f},{ 1.0f ,1.4142f ,1.0f },   { 0.8165f ,1.0f ,0.8165f} },
                        {{ 1.f ,1.4142f ,1.0f },   { 1.4142f ,0.0f ,1.4142f}, { 1.0f ,1.4142f ,1.0f} },
						            {{ 0.8165f ,1.0f,0.8165f}, { 1.0f ,1.4142f ,1.0f},    { 0.8165f ,1.0f ,0.8165f} }};
	float normtotal=0;

	for(x=0;x<3;x++){
		for(y=0;y<3;y++){
			for(z=0;z<3;z++){
				normtotal+=omega[x][y][z];
			}
		}
	}

	for(x=0;x<x_pixels;x++){
		for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
			
			for(z=1;z<z_pixels-1;z++){
				gradient[x][y][z]=0;
				gradient[x][y][z]+=1.4142f*(image[x+1][y][z]+image[x][y+1][z]+image[x][y][z+1]+image[x-1][y][z]+image[x][y-1][z]+image[x][y][z-1])
					+(image[x+1][y+1][z]+image[x-1][y+1][z]+image[x+1][y-1][z]+image[x-1][y-1][z]
				     +image[x+1][y][z+1]+image[x-1][y][z+1]+image[x][y-1][z+1]+image[x][y+1][z+1]
				     +image[x+1][y][z-1]+image[x-1][y][z-1]+image[x][y-1][z-1]+image[x][y+1][z-1])
					 +0.8165f*(image[x+1][y+1][z+1]+image[x-1][y+1][z+1]+image[x+1][y-1][z+1]+image[x-1][y-1][z+1]
							 +image[x+1][y+1][z-1]+image[x-1][y+1][z-1]+image[x+1][y-1][z-1]+image[x-1][y-1][z-1]);
				 gradient[x][y][z]/=normtotal;
				 gradient[x][y][z]=image[x][y][z]-gradient[x][y][z];

			}
		}
	}
}

void pt_cal_updateimage_mapem(void *ptarg)
{
BPFP_ptargs *arg;
	__m128 coef;
	__m128 czero;
	int x,y,z;
	__m128 *mptr1,*mptr2,*mptr3,*mptr4;
	__m128 amax,amin;
	float *nptr1,*nptr2,*nptr3,*nptr4;
	float eps=0.0003f;
	__m128 mmeps;

	if(sum_image!=0) eps*=sum_normfac/sum_image;
	else eps=0;
	mmeps=_mm_set_ps1(eps);
	amax=_mm_set_ps1(100.00);
	amin=_mm_set_ps1(0.1f);
	if(sum_image!=0) printf("sumnormfac %f\t%f\t%f\n",sum_normfac,sum_image,eps);
	czero=_mm_set_ps1(0);

	arg=(BPFP_ptargs*) ptarg;
	coef=_mm_set_ps1(image_max);
	for(x=arg->start;x<arg->end;x++){
		for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) {
			mptr1=(__m128 *)image[x][y];
			mptr2=(__m128 *)correction[x][y];
			mptr3=(__m128 *)normfac[x][y];
			mptr4=(__m128 *)gradient[x][y];
			nptr1=(float *)mptr1;
			nptr2=(float *)mptr2;
			nptr3=(float *)mptr3;
			nptr4=(float *)mptr4;
			for(z=0;z<z_pixels_simd;z++){
				mptr1[z]=_mm_mul_ps(mptr1[z],_mm_div_ps(_mm_sub_ps(mptr2[z],_mm_mul_ps(mmeps,_mm_sub_ps(mptr4[z],mptr1[z]))),_mm_add_ps(mptr3[z],_mm_mul_ps(mmeps,mptr1[z]))));
//				mptr1[z]=_mm_add_ps(mptr1[z],_mm_mul_ps(mptr1[z],_mm_mul_ps(mptr2[z],mptr3[z])));
//				mptr1[z]=_mm_min_ps(coef,_mm_mul_ps(mptr1[z],_mm_mul_ps(mptr2[z],mptr3[z])));
				mptr2[z]=czero;
			}

		}
	}
}

FUNCPTR pt_cal_updateimage(void *ptarg)
{
	BPFP_ptargs *arg;
	__m128 coef;
	__m128 czero;
	int x,y,z;
	__m128 *mptr1,*mptr2,*mptr3;
	__m128 amax,amin;
	float *nptr1,*nptr2,*nptr3;


	if(mapemflag){
		pt_cal_updateimage_mapem(ptarg);
		return 0;
	}
	amax=_mm_set_ps1(100.00);
	amin=_mm_set_ps1(0.01f);

	czero=_mm_set_ps1(0);

	arg=(BPFP_ptargs*) ptarg;
	coef=_mm_set_ps1(image_max);
	for(x=arg->start;x<arg->end;x++){
		for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) {
			mptr1=(__m128 *)image[x][y];
			mptr2=(__m128 *)correction[x][y];
			mptr3=(__m128 *)normfac[x][y];
			nptr1=(float *)mptr1;
			nptr2=(float *)mptr2;
			nptr3=(float *)mptr3;
			for(z=0;z<z_pixels_simd;z++){
//				mptr1[z]=_mm_add_ps(mptr1[z],_mm_mul_ps(mptr1[z],_mm_mul_ps(mptr2[z],mptr3[z])));
//				mptr1[z]=_mm_min_ps(coef,_mm_mul_ps(mptr1[z],_mm_mul_ps(mptr2[z],mptr3[z])));
				mptr1[z]=_mm_min_ps(coef,_mm_mul_ps(mptr1[z],_mm_mul_ps(_mm_min_ps(amax,_mm_max_ps(amin,mptr2[z])),mptr3[z])));
				mptr2[z]=czero;
			}

		}
	}
//	printf("startend %d\t%d\n",arg->start,arg->end);
	return 0;
}


/*
* required memory
*   - span9
*         256(resolution) : about 1157M
*         128(resolution) : about  548M
*
* --------------------------------------
* largeprj3 and largeprj
*
*   views = 288 (covers 180 degree).
*
*   for file format                        for processing recon.
*   largeprj3                              largeprj
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
void allocation_memory(int pFlag,int  sFlag,int weighting) {
	int i,j;
	//short *
	tmp_for_short_sino_read=(short ***) matrix3dm128(0,yr_pixels-1,0,views-1,0,xr_pixels/8-1);
	// 1. projection type

	//	largeprj3 = (float ***) matrix3dm128(0,segmentsize-1,0,views-1,0,xr_pixels/4-1); //for all theta.	

	largeprj3=(float ***) calloc(segmentsize,sizeof(float **));
	for(i=0;i<segmentsize;i++){
		largeprj3[i]=(float **) calloc(views,sizeof(float*));
		for(j=0;j<views;j++){
			largeprj3[i][j]=(float *) _mm_malloc(xr_pixels*sizeof(float),16);
		}
	}

	if(mapemflag){
		gradient=(float ***) matrix3dm128(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd-1);  //for two views.
		laplacian=(float ***) matrix3dm128(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd-1);  //for two views.
	}

	if( largeprj3 == NULL ) crash1 ("  Error in allocation memory for largeprj3 !\n");
	else if (verbose & 0x0040) fprintf(stdout,"  Allocate largeprj3      : %d kbytes \n", views/2*segmentsize*2*xr_pixels/256);
	if(weighting==3|| weighting==8|| weighting==4|| weighting==9){
		if(floatFlag==0){
			largeprjshort3=(short ***) calloc(segmentsize,sizeof(short **));
			for(i=0;i<segmentsize;i++){
				largeprjshort3[i]=(short **) calloc(views,sizeof(short*));
				for(j=0;j<views;j++){
					largeprjshort3[i][j]=(short *) _mm_malloc(xr_pixels*sizeof(short),16);
				}
			}
		} else { //MS+CM next was short and should be float
			largeprjfloat3 = (float ***) matrix3dm128(0,segmentsize-1,0,views-1,0,xr_pixels/4-1);
			if( largeprjfloat3 == NULL ) crash1 ("  Error in allocation memory  for largeprjfloat3 !\n");
			else if (verbose & 0x0040) fprintf(stdout,"  Allocate largeprjfloat3      : %d kbytes \n", views/2*segmentsize*2*xr_pixels/256);
		}
	}
	// 1-b. projection type = for processing recon.
	// 4.31M    /   2.16M
	// 621.28M  / 311M
	// 310.7M   / 150.5M
	estimate = (float **) matrixm128(0,segmentsize*2-1,0,xr_pixels/4-1);  //for two views.
	estimate_thread = (float ***) matrix3dm128(0,nthreads-1,0,segmentsize*2-1,0,xr_pixels/4-1);  //for two views.

	largeprj=(float ***) calloc(views/2,sizeof(float **));                 //for all views.
	for(i=0;i<views/2;i++){
		largeprj[i]=(float **) calloc(segmentsize*2,sizeof(float *));
	}
	for(i=0;i<views/2;i++){
		for(j=0;j<segmentsize;j++){
			largeprj[i][j]=largeprj3[j][i];
			largeprj[i][j+segmentsize]=largeprj3[j][i+views/2];
		}
	}
	if(weighting==3|| weighting==8|| weighting==4|| weighting==9){
		if(floatFlag==0){
			largeprjshort=(short ***) calloc(views/2,sizeof(short **));
			for(i=0;i<views/2;i++){
				largeprjshort[i]=(short **) calloc(segmentsize*2,sizeof(short *));
			}
			for(i=0;i<views/2;i++){
				for(j=0;j<segmentsize;j++){
					largeprjshort[i][j]=largeprjshort3[j][i];
					largeprjshort[i][j+segmentsize]=largeprjshort3[j][i+views/2];
				}
			}
		} else {
			largeprjfloat=(float ***) calloc(views/2,sizeof(float **));
			for(i=0;i<views/2;i++){
				largeprjfloat[i]=(float **) calloc(segmentsize*2,sizeof(float *));
			}
			for(i=0;i<views/2;i++){
				for(j=0;j<segmentsize;j++){
					largeprjfloat[i][j]=largeprjfloat3[j][i];
					largeprjfloat[i][j+segmentsize]=largeprjfloat3[j][i+views/2];
				}
			}
		}
	}

	if(CompressFlag==1){
		compressmask=(unsigned char ***) matrix3dchar(0,views-1,0,segmentsize-1,0,xr_pixels/8-1);
		compress_num=(short **) matrixshort(0,views-1,0,xr_pixels-1);
		if(compressmask==NULL){
			printf("compressmask allocation error\n");
		}
		if(compress_num==NULL){
			printf("compress_num allocation error\n");
		}
	}
	// 2. img type memory array.
	//    51.75M x 2 / 12.94M x 2
}


void test_free(int weighting){

	int i =0;
	int j =0;
	free_matrix3dm128(largeprj3, 0,segmentsize-1,0,views-1   , 0,xr_pixels/4-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free largeprj3     : %d kbytes \n", views/2*segmentsize*2*xr_pixels/256);
	if(weighting==3|| weighting==8|| weighting==4|| weighting==9){
		free_matrix3dm128((float ***)largeprjshort3, 0,segmentsize-1,0,views-1,0,xr_pixels/8-1);        
		if (verbose & 0x0040) fprintf(stdout,"  Free largeprjshort3      : %d kbytes \n", views/2*segmentsize*2*xr_pixels/512);
	}
	free_matrixm128(estimate,0,segmentsize-1,0,xr_pixels/4-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free estimate      : %d kbytes \n", segmentsize*2*xr_pixels/512);
	for(i=0;i<views/2;i++){
		for(j=0;j<segmentsize*2;j++){
			//free(largeprj[i][j]);
		}
		free(largeprj[i]);
	}
	free(largeprj);
	if(weighting==3|| weighting==8|| weighting==4|| weighting==9){
		for(i=0;i<views/2;i++){
			for(j=0;j<segmentsize*2;j++){
				free(largeprjshort[i][j]);
			}
			free(largeprjshort[i]);//=(short **) calloc(segmentsize*2,sizeof(short *));
		}
		free(largeprjshort);
	}
}

void zoom_proj_zoomup(float ***largeprj3,float zoom)
{
	int i,j;
	float zoomf;
	float *tmpf;
	float xr;
	float xrz;
	float r;
	float *data;
	int xri,xrzi;

	tmpf=(float *) calloc(xr_pixels,sizeof(float));
	if(zoom<=1){
		printf("  Zoom should be greater than 1.\n");
		return;
	}
	zoomf=1/zoom;
	for(i=0;i<segmentsize;i++){
		for(j=0;j<views;j++){
			xr=xrz=0;
			memset(tmpf,0,xr_pixels*sizeof(float));
			data=largeprj3[i][j];
			xrz=-xr_pixels/2*zoomf+xr_pixels/2;
			for(xri=0;xri<xr_pixels;xri++){
				xrzi=(int)xrz;
				r=xrz-xrzi;
				if(xrzi<xr_pixels-2 && xrzi>=0){
					tmpf[xri]=data[xrzi]+r*(data[xrzi+1]-data[xrzi]);
				} else tmpf[xri]=0;
				xrz+=zoomf;
			}
			memcpy(data,tmpf,xr_pixels*sizeof(float));
		}
	}
	free(tmpf);
}

void zoom_proj_reduce(float ***largeprj3,float zoom)
{
	int i,j,k;
	float zoomf;
	float *tmpf;
	float xr;
	float xrz;
	float r;
	float *data;
	int xri,xrzi;
	float *normal;

	tmpf=(float *) calloc(xr_pixels,sizeof(float));
	normal=(float *) calloc(xr_pixels,sizeof(float));
	if(zoom>=1){
		printf("  Zoom should be smaller than 1\n");
		return;
	}
	zoomf=zoom;

	for(k=0;k<xr_pixels;k++) normal[k]=0;
	xrz=-xr_pixels/2*zoomf+xr_pixels/2;
	for(xri=0;xri<xr_pixels;xri++){
		xrzi=(int)xrz;
		r=xrz-xrzi;
		if(xrzi<xr_pixels-2 && xrzi>=0){
			normal[xrzi]+=(1-r);
			normal[xrzi+1]+=r;
		}
		xrz+=zoomf;
	}
	for(k=0;k<xr_pixels;k++) if(normal[k]!=0) normal[k]=1/normal[k];

	for(i=0;i<segmentsize;i++){
		for(j=0;j<views;j++){
			xr=xrz=0;
			memset(tmpf,0,xr_pixels*sizeof(float));
			data=largeprj3[i][j];
			xrz=-xr_pixels/2*zoomf+xr_pixels/2;
			for(xri=0;xri<xr_pixels;xri++){
				xrzi=(int)xrz;
				r=xrz-xrzi;
				if(xrzi<xr_pixels-2 && xrzi>=0){
					tmpf[xrzi]+=data[xri]*(1-r);
					tmpf[xrzi+1]+=data[xri]*r;
				}
				xrz+=zoomf;
			}
			for(k=0;k<xr_pixels;k++) data[k]=tmpf[k]*normal[k];
		}
	}
	free(tmpf);
	free(normal);
}

void zoom_proj_short_zoomup(short ***largeprj3,float zoom)
{
	int i,j,k;
	float zoomf;
	float *tmpf;
	float xr;
	float xrz;
	float r;
	short *data;
	int xri,xrzi;
	tmpf=(float *) calloc(xr_pixels,sizeof(float));
	if(zoom<=1){
		printf(" Zoom should be greater than 1\n");
		return;
	}
	zoomf=1/zoom;

	for(i=0;i<segmentsize;i++){
		for(j=0;j<views;j++){
			xr=xrz=0;
			memset(tmpf,0,xr_pixels*sizeof(float));
			data=largeprj3[i][j];
			xrz=-xr_pixels/2*zoomf+xr_pixels/2;
			for(xri=0;xri<xr_pixels;xri++){
				xrzi=(int)xrz;
				r=xrz-xrzi;
				if(xrzi<xr_pixels-2 && xrzi>=0){
					tmpf[xri]=data[xrzi]+r*(data[xrzi+1]-data[xrzi]);
				}
				else tmpf[xri]=0;
				xrz+=zoomf;
			}
			for(k=0;k<xr_pixels;k++) data[k]=(int) (tmpf[k]+0.5);
		}
	}
	free(tmpf);
}

void zoom_proj_short_reduce(short ***largeprj3,float zoom)
{
	int i,j,k;
	float zoomf;
	float *tmpf;
	float xr;
	float xrz;
	float r;
	short *data;
	int xri,xrzi;
	float *normal;
	tmpf=(float *) calloc(xr_pixels,sizeof(float));
	normal=(float *) calloc(xr_pixels,sizeof(float));
	if(zoom>=1){
		printf(" Zoom should be smaller than 1\n");
		return;
	}
	zoomf=zoom;

	for(k=0;k<xr_pixels;k++) normal[k]=0;
	xrz=-xr_pixels/2*zoomf+xr_pixels/2;
	for(xri=0;xri<xr_pixels;xri++){
		xrzi=(int)xrz;
		r=xrz-xrzi;
		if(xrzi<xr_pixels-2 && xrzi>=0){
			normal[xrzi]+=(1-r);
			normal[xrzi+1]+=r;
		}
		xrz+=zoomf;
	}
	for(k=0;k<xr_pixels;k++) if(normal[k]!=0) normal[k]=1/normal[k];


	for(i=0;i<segmentsize;i++){
		for(j=0;j<views;j++){
			xr=xrz=0;
			memset(tmpf,0,xr_pixels*sizeof(float));
			data=largeprj3[i][j];
			xrz=-xr_pixels/2*zoomf+xr_pixels/2;
			for(xri=0;xri<xr_pixels;xri++){
				xrzi=(int)xrz;
				r=xrz-xrzi;
				if(xrzi<xr_pixels-2 && xrzi>=0){
					tmpf[xrzi]+=data[xri]*(1-r);
					tmpf[xrzi+1]+=data[xri]*r;
				}
				xrz+=zoomf;
			}
			for(k=0;k<xr_pixels;k++) data[k]=(int) (tmpf[k]*normal[k]+0.5);
		}
	}
	free(tmpf);
	free(normal);
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

FUNCPTR pt_update_est_w012(void *ptarg) 
{
	UPDATE_STRUCT *arg=(UPDATE_STRUCT *) ptarg;
	float **est=arg->est;
	float **prj=arg->prj;
	int start=arg->plane_start;
	int end=arg->plane_end;
	int size=arg->size;
	int i;
	int xr;
	float *ptr1,*ptr2;
	int totalcount=0;
	int zerocount=0;

	for(i=start;i<end;i++){
		ptr1 = est[i];
		ptr2 = prj[i];
		for( xr=0; xr<size; xr++ ) {
//			if(ptr1[xr] > epsilon3) ptr1[xr] = (ptr2[xr]-ptr1[xr])/ptr1[xr];
			if(ptr1[xr] > epsilon3) ptr1[xr] = ptr2[xr]/ptr1[xr];
			else  {
				if(ptr2[xr]==0) ptr1[xr] = 0.0;
				else ptr1[xr]=0.0;
			}
		}
	}
	return 0;
}


void update_estimate_w012(float **estimate,float **prj,int nplanes,int xr_pixels,int nthreads)
{
	int thr,inc;
	THREAD_TYPE threads[16];
	UPDATE_STRUCT updatestruct[16];
	unsigned int threadID;

	inc=nplanes/nthreads;
	for(thr=0;thr<nthreads;thr++){
		updatestruct[thr].est=estimate;
		updatestruct[thr].prj=prj;
		updatestruct[thr].size=xr_pixels;
		updatestruct[thr].plane_start=thr*inc;
		if(thr!=nthreads-1) updatestruct[thr].plane_end=(thr+1)*inc;
		else updatestruct[thr].plane_end=nplanes;
		START_THREAD(threads[thr],pt_update_est_w012,updatestruct[thr],threadID);
	}
	for (thr = 0; thr < nthreads; thr++) {
		Wait_thread( threads[thr]);
	}
}

FUNCPTR pt_update_est_w3(void *ptarg) 
{
	UPDATE_STRUCT *arg=(UPDATE_STRUCT *) ptarg;
	float **est=arg->est;
	float **prj=arg->prj;
	short **prjshort=arg->prjshort;
	int start=arg->plane_start;
	float **prjfloat=arg->prjfloat;
	int end=arg->plane_end;
	int size=arg->size;
	int i;
	int xr;
	int view = arg->view;
	float *ptr1,*ptr2,*ptr4;
	short *ptr3;
	float den;
	int totalcount=0;
	int zerocount=0;

	if(floatFlag==0){
		for(i=start;i<end;i++){
			ptr1 = est[i];
			ptr2 = prj[i];
			ptr3 = prjshort[i];
			ptr4 = prjfloat[i];
			for( xr=0; xr<size; xr++ ) {
				den = ptr1[xr] + ptr2[xr];
				if(ptr3[xr]==0) ptr1[xr]=0;
				else if(den > epsilon3) ptr1[xr] = ptr3[xr]/den;
				else	ptr1[xr] = 1.0;
			}
		}
	} else {
		for(i=start;i<end;i++){
			ptr1 = est[i];
			ptr2 = prj[i];
			ptr4 = prjfloat[i];
			for( xr=0; xr<size; xr++ ) {
				den = ptr1[xr] + ptr2[xr];
				if(den > epsilon3) ptr1[xr] = ptr4[xr]/den;
				else ptr1[xr] = 1.0;
			}
		}
	}

	return 0;
}




void update_estimate_w3(float **estimate,float **prj,short **prjshort,float **prjfloat,int nplanes,int xr_pixel,int nthreads)
{
	int thr,inc;
	THREAD_TYPE threads[16];
	UPDATE_STRUCT updatestruct[16];
	unsigned int threadID;

	inc=nplanes/nthreads;
	for(thr=0;thr<nthreads;thr++){
		updatestruct[thr].est=estimate;
		updatestruct[thr].prj=prj;
		updatestruct[thr].prjshort=prjshort;
		updatestruct[thr].prjfloat=prjfloat;
		updatestruct[thr].size=xr_pixels;
		updatestruct[thr].plane_start=thr*inc;
		updatestruct[thr].view=xr_pixel;

		if(thr!=nthreads-1) updatestruct[thr].plane_end=(thr+1)*inc;
		else updatestruct[thr].plane_end=nplanes;
		START_THREAD(threads[thr],pt_update_est_w3,updatestruct[thr],threadID);
	}
	for (thr = 0; thr < nthreads; thr++) {
		Wait_thread( threads[thr]);
	}
}


void alloc_tmprprj1()
{
	tmp_prj1  = (float ***) matrix3dm128(0,yr_pixels-1,0,views-1,0,xr_pixels/4-1); // for one theta.
	if(tmp_prj1==NULL){
		fprintf(stderr,"  Error allocating tmpprj1\n");
		exit(1);
	}
	if (verbose & 0x0040) fprintf(stdout,"  Allocate tmp_prj1        :  %d kbytes\n",(yr_pixels*views*xr_pixels/4)/256);
}

void alloc_tmprprj2()
{
	tmp_prj2  = (float ***) matrix3dm128(0,yr_pixels-1,0,views-1,0,xr_pixels/4-1); // for one theta.
	if(tmp_prj2==NULL){
		fprintf(stderr,"  Error allocating tmpprj2\n"); 
		exit(1);
	}
	if (verbose & 0x0040) fprintf(stdout,"  Allocate tmp_prj2         : %d kbytes\n",(yr_pixels*views*xr_pixels/4)/256);
}

void free_tmpprj1()
{
	free_matrix3dm128(tmp_prj1,0,yr_pixels-1,0,views-1,0,xr_pixels/4-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free tmp_prj1\n");
}

void free_tmpprj2()
{
	free_matrix3dm128(tmp_prj2,0,yr_pixels-1,0,views-1,0,xr_pixels/4-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free tmp_prj2\n");
}

void alloc_correction()
{
	correction= (float ***) matrix3dm128(0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd-1);
	if( correction == NULL ) crash1("  Error allocating correction !\n");
	else if (verbose & 0x0040) fprintf(stdout,"  Allocate correction    : %d kbytes \n", imagesize/256);
}

void alloc_normfac()
{
	normfac = (float ***) matrix3dm128(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd-1); 
	if( normfac == NULL ) crash1("  Error allocating normfac !\n");
	else if (verbose & 0x0040) fprintf(stdout,"  Allocate normfac       : %d kbytes \n", imagesize/256);    
}

void alloc_imagemask()
{
	imagemask = (char ***) matrix3dchar(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd*4-1); 
	if( imagemask == NULL ) crash1("  Error allocating imagemask !\n");
	else if (verbose & 0x0040) fprintf(stdout,"  Allocate imagemask      : %d kbytes \n", imagesize/1024);    
}

void free_imagemask()
{
	free_matrix3dchar(imagemask, 0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd*4-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free imagemask          : %d kbytes \n", imagesize/1024);    
}
void free_correction()
{
	free_matrix3dm128(correction, 0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free correction          : %d kbytes \n", imagesize/256);    
}

void free_normfac()
{
	free_matrix3dm128(normfac, 0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd-1);
	if (verbose & 0x0040) fprintf(stdout,"  Free normfac             : %d kbytes \n", imagesize/256);
}

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

//void pack_image(float ***image,float ***pack)
void pack_image()
{
	int x,y,z,z4;
	int xi,yi;
	float *optr,*ptr1,*ptr2,*ptr3,*ptr4;;
	for(x=0;x<=x_pixels/2;x++){
		xi=x_pixels-x;
		if(cylwiny[x][0]==0 && cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0];y<=y_pixels/2;y++){
			yi=y_pixels-y;
			ptr1=image[x][y];
			ptr2=image[xi][yi];
			ptr3=image[yi][x];
			ptr4=image[y][xi];
			optr=imagepack[x][y];
			for(z=0,z4=0;z<z_pixels;z++,z4+=4){
				optr[z4]=ptr1[z];
				optr[z4+1]=ptr2[z];
				optr[z4+2]=ptr3[z];
				optr[z4+3]=ptr4[z];
			}
		}
	}
}


void alloc_thread_buf()
{
	int i;
//	imagepack=(float ***) matrix3dm128(0,x_pixels/2,0,y_pixels,0,z_pixels_simd*4-1);
	imagepack= (float ***) alloc_imagepack();
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
	printf("free done\n");
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

void alloc_normfac_infile(int weighting,int span)
{
	int i,j;
	if(normfac_in_file_flag==-1) return ;

	normfac_infile=(float ****) malloc(subsets*sizeof(float *));
	for(i=0;i<subsets;i++){
		normfac_infile[i]=(float ***) matrix3dm128_check(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd-1);//x_pixels*y_pixels*z_pixels_simd*4*sizeof(float));
		if(normfac_infile[i]==NULL){
			normfac_in_file_flag=-1;
		}
	}

	if(normfac_in_file_flag==-1 && i>0){
		for(j=0;j<i;j++){
			free_matrix3dm128(normfac_infile[j],0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd-1);
		}
		free(normfac_infile);
	} else {
		normfac_in_file_flag=1;
	}

}

//void swap_xr_yr(float ***largeprj3,float ***largeprj_swap)
void swap_xr_yr()
{
	int yr,v,yr2,xr;
	float ***ptr;
	float *ptr11,*ptr12;
	float **ptr2;

	float **tmpbuf;

	tmpbuf=(float **) matrixfloat(0,segmentsize*2-1,0,xr_pixels-1);

	printf("start\n");

	ptr=(float ***) calloc(views/2,sizeof(float **));
	for(v=0;v<views/2;v++){
		ptr[v]=(float **) calloc(xr_pixels,sizeof(float *));
		for(xr=0;xr<xr_pixels;xr++){
			ptr[v][xr]=(float *) _mm_malloc(segmentsize*2*sizeof(float),16);
			if(ptr[v][xr]==NULL) {
				printf("error swapping memory %d\t%d\n",v,xr);
				exit(1);
			}
		}
		ptr2=ptr[v];
		for(yr=0,yr2=segmentsize;yr<segmentsize;yr++,yr2++){
			memcpy(&tmpbuf[yr][0],largeprj3[yr][v],xr_pixels*sizeof(float));
			memcpy(&tmpbuf[yr2][0],largeprj3[yr][v+views/2],xr_pixels*sizeof(float));
		}
		for(yr=0,yr2=segmentsize;yr<segmentsize;yr++,yr2++){
			ptr11=tmpbuf[yr];//largeprj3[yr][v];
			ptr12=tmpbuf[yr2];//largeprj3[yr][v+views/2];
			for(xr=0;xr<xr_pixels;xr++){
				ptr2[xr][yr]=ptr11[xr];
				ptr2[xr][yr2]=ptr12[xr];
			}
		}
		for(yr=0;yr<segmentsize;yr++){
			_mm_free(largeprj3[yr][v]);
			_mm_free(largeprj3[yr][v+views/2]);
		}
	}
	free_matrix(tmpbuf,0,segmentsize*2-1,0,xr_pixels/4-1);

	largeprj_swap=ptr;
//	for(v=0;v<views;v++){
//		for(yr=0;yr<segmentsize;yr++){
//			if(largeprj3[yr][v]!=NULL) printf("error free %d\t%d\n",yr,v);
//		}
//	}

//	printf("done\n");
//	exit(1);
}
//void swap_xr_yr_short(short ***largeprjshort3,short ***largeprjshort_swap2)
void swap_xr_yr_short()
{
	int yr,v,yr2,xr;
	short***ptr;
	short *ptr11,*ptr12;
	short **ptr2;

	printf("start\n");

	ptr=(short***) calloc(views/2,sizeof(short**));
	for(v=0;v<views/2;v++){
		ptr[v]=(short**) calloc(xr_pixels,sizeof(short *));
		for(xr=0;xr<xr_pixels;xr++){
			ptr[v][xr]=(short *) _mm_malloc(segmentsize*2*sizeof(short),16);
			if(ptr[v][xr]==NULL) {
				printf("error swapping memory %d\t%d\n",v,xr);
				exit(1);
			}
		}
		ptr2=ptr[v];
		for(yr=0,yr2=segmentsize;yr<segmentsize;yr++,yr2++){
			ptr11=largeprjshort3[yr][v];
			ptr12=largeprjshort3[yr][v+views/2];
			for(xr=0;xr<xr_pixels;xr++){
				ptr2[xr][yr]=ptr11[xr];
				ptr2[xr][yr2]=ptr12[xr];
			}
		}
		for(yr=0;yr<segmentsize;yr++){
			_mm_free(largeprjshort3[yr][v]);
			_mm_free(largeprjshort3[yr][v+views/2]);
		}
	}
	largeprjshort_swap=ptr;
}


void InitParameter()
{
	GLOBAL_OSEM3DPAR *p=osem3dpar;

	p->nthreads=nthreads;
	p->true_file=true_file;
	p->tFlag=tFlag;
	p->floatFlag=floatFlag;
	p->precorFlag=precorFlag;
	p->CompressFlag=CompressFlag;
	p->inflipFlag=inflipFlag;
	p->prompt_file=prompt_file;
	p->pFlag=pFlag;
	p->delayed_file=delayed_file;
	p->norm_file=norm_file;
	p->nFlag=nFlag;
	p->atten_file=atten_file;
	p->noattenFlag=noattenFlag;
	p->out_img_file=out_img_file;
	p->out_scan3D_file=out_scan3D_file;
	p->o3Flag=o3Flag;
	p->saveFlag=saveFlag;
	p->in_img_file=in_img_file;
	p->iFlag=iFlag;
	p->muFlag=muFlag;
	p->scat_scan_file=scat_scan_file;
	p->sFlag=sFlag;
	p->max_g=max_g;
	p->min_g=min_g;
	p->iterations=iterations;
	p->subsets=subsets;
	p->weighting=weighting;
	p->zoom=zoom;
	p->newzoom=newzoom;
	p->rel_fov=rel_fov;
	p->trimFlag=trimFlag;
	p->z_min=z_min;
	p->z_max=z_max;
	p->verbose=verbose;
	p->span=span;
	p->maxdel=maxdel;
	p->positFlag=positFlag;
	p->image_max=image_max;
	p->epsilon1=epsilon1;
	p->epsilon2=epsilon2;
	p->epsilon3=epsilon3;
	p->npskip=npskip;
	p->normfac_in_file_flag=normfac_in_file_flag;
	p->iModel=iModel;
	p->imodeldefname=imodeldefname;
	p->calnormfacFlag=calnormfacFlag;
	p->blur_fwhm1=blur_fwhm1;
	p->blur_fwhm2=blur_fwhm2;
	p->blur_ratio=blur_ratio;
	p->blur=blur;
	p->mapemflag=mapemflag;
}

void PutParameter()
{
	GLOBAL_OSEM3DPAR *p=osem3dpar;

	nthreads=p->nthreads;
	if (num_em_frames == 0) true_file=p->true_file;
	tFlag=p->tFlag;
	floatFlag=p->floatFlag;
	precorFlag=p->precorFlag;
	CompressFlag=p->CompressFlag;
	inflipFlag=p->inflipFlag;
	prompt_file=p->prompt_file;
	pFlag=p->pFlag;
	delayed_file=p->delayed_file;
	norm_file=p->norm_file;
	nFlag=p->nFlag;
	atten_file=p->atten_file;
	noattenFlag=p->noattenFlag;
	if (num_em_frames == 0) out_img_file=p->out_img_file;
	out_scan3D_file=p->out_scan3D_file;
	o3Flag=p->o3Flag;
	saveFlag=p->saveFlag;
	in_img_file=p->in_img_file;
	iFlag=p->iFlag;
	muFlag=p->muFlag;
	scat_scan_file=p->scat_scan_file;
	sFlag=p->sFlag;
	max_g=p->max_g;
	min_g=p->min_g;
	iterations=p->iterations;
	subsets=p->subsets;
	weighting=p->weighting;
	zoom=p->zoom;
	newzoom=p->newzoom;
	rel_fov=p->rel_fov;
	trimFlag=p->trimFlag;
	z_min=p->z_min;
	z_max=p->z_max;
	verbose=p->verbose;
	span=p->span;
	maxdel=p->maxdel;
	positFlag=p->positFlag;
	image_max=p->image_max;
	epsilon1=p->epsilon1;
	epsilon2=p->epsilon2;
	epsilon3=p->epsilon3;
	npskip=p->npskip;
	normfac_in_file_flag=p->normfac_in_file_flag;
	iModel=p->iModel;
	imodeldefname=p->imodeldefname;
	calnormfacFlag=p->calnormfacFlag;
	blur_fwhm1=p->blur_fwhm1;
	blur_fwhm2=p->blur_fwhm2;
	blur_ratio=p->blur_ratio;
	blur=p->blur;
	mapemflag=p->mapemflag;

}

void GetParameter(int argc,char **argv)
{
	int i;
  unsigned int n=0;
	char *optarg, *sep=NULL;
	char c;
  FILE *log_fp=NULL;
	
	InitParameter();

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
			 case 't' :
				 osem3dpar->true_file = optarg;
				 osem3dpar->tFlag=1;
				 break;
			 case 'P' :
				 osem3dpar->floatFlag=1;
				 osem3dpar->precorFlag=1; 
				 break;
			 case 'C' :
				 osem3dpar->CompressFlag=1;
				 break;
			 case 'Q' :
				 osem3dpar->floatFlag=1; 
				 break;
			 case 'R' :
				 osem3dpar->floatFlag=2; 
				 break;
			 case 'F' :
				 if (sscanf(optarg,"%d,%d",&osem3dpar->inflipFlag,&osem3dpar->outflipFlag) != 2)
					 crash2("error decoding -F %s\n",optarg);
				 break;
			 case 'p' :
				 osem3dpar->prompt_file = optarg;
				 osem3dpar->pFlag=1;
				 break;
			 case 'd' :
				 osem3dpar->delayed_file = optarg;
				 osem3dpar->pFlag=1;
				 break;
			 case 'D' :
				 normfac_dir = optarg;
				 break;
			 case 'n':
				 osem3dpar->norm_file = optarg;
				 osem3dpar->nFlag=1;
				 break;
			 case 'a':
				 osem3dpar->atten_file = optarg;
				 osem3dpar->noattenFlag=0;
				 break;
			 case 'o':
				 osem3dpar->out_img_file = optarg;
				 break;
			 case 'h':
				 osem3dpar->out_scan3D_file = optarg;
				 osem3dpar->o3Flag=1;
				 break;
			 case 'O':
				 osem3dpar->out_img_file = optarg;
				 osem3dpar->saveFlag = 1;
				 break;
			 case 'i':
				 osem3dpar->in_img_file = optarg;
				 osem3dpar->iFlag=1;
				 break;
			 case 'L':
         strcpy(log_file,optarg);
         if ((sep=strchr(log_file,',')) != NULL) *sep++='\0';
         if (sep!=NULL && sscanf(sep,"%d",&n) == 1) log_mode = n;
         else log_mode = LOG_TO_FILE;
         if (log_mode==0 || log_mode>(LOG_TO_CONSOLE|LOG_TO_FILE))
           log_mode = LOG_TO_FILE; // default
          if ((log_fp=fopen(log_file,"wt")) == NULL)
           crash2("error creating log file %s\n",log_file);
         fclose(log_fp);
				 break;
			 case 'U' :
				 osem3dpar->muFlag=1;
				 break;
			 case 's':
				 osem3dpar->scat_scan_file = optarg;
				 osem3dpar->sFlag=1;
				 break;
			 case 'g' :
				 if (sscanf(optarg,"%d,%d",&osem3dpar->max_g,&osem3dpar->min_g) < 1)        
					 crash2("error decoding -g %s\n",optarg);
				 break;
			 case 'I' :
				 if (sscanf(optarg,"%d",&osem3dpar->iterations) != 1)
					 crash2("error decoding -I %s\n",optarg);
				 break;
			 case 'S' :
				 if (sscanf(optarg,"%d",&osem3dpar->subsets) != 1)
					 crash2("error decoding -S %s\n",optarg);
				 break;
			 case 'W' :
				 if (sscanf(optarg,"%d",&osem3dpar->weighting) != 1)
					 crash2("error decoding -W %s\n",optarg);
				 break;
			 case 'z' :
				 if (sscanf(optarg,"%f",&osem3dpar->zoom) != 1)
					 crash2("error decoding -z %s\n",optarg);
				 break;
			 case 'Z' :
				 if (sscanf(optarg,"%f",&osem3dpar->newzoom) != 1)
					 crash2("error decoding -z %s\n",optarg);
				 break;
			 case 'f' :
				 if (sscanf(optarg,"%f,%d,%d,%d",&osem3dpar->rel_fov,&osem3dpar->trimFlag,&osem3dpar->z_min,&osem3dpar->z_max) != 4)
					 crash2("error decoding -f %s\n",optarg);
				 break;
			 case 'v':
				 if (sscanf(optarg,"%d",&osem3dpar->verbose) != 1)
					 crash2("error decoding -v %s\n",optarg);
				 break;
			 case 'm' :
				 if (sscanf(optarg,"%d,%d",&osem3dpar->span,&osem3dpar->maxdel) != 2)
					 crash2("error decoding -m %s\n",optarg);
				 break;
			 case 'w' :
				 if (sscanf(optarg,"%d",&osem3dpar->positFlag) != 1){
					 osem3dpar->positFlag=1;
//					 crash2("error decoding -w %s\n",optarg);
				 } else {
					 if(osem3dpar->positFlag>3 ||osem3dpar->positFlag<0 ) {
						 crash2("error decoding -w %s\n",optarg);
					 } 
				 }
				 break;
			 case 'u' :
				 if (sscanf(optarg,"%f",&osem3dpar->image_max) != 1)
					 crash2("error decoding -u %s\n",optarg);
				 break;
			 case 'e' :
				 if (sscanf(optarg,"%f,%f,%f",&osem3dpar->epsilon1,&osem3dpar->epsilon2,&osem3dpar->epsilon3) != 3)
					 crash2("error decoding -e %s\n",optarg);
				 break;
			 case 'k' :
				 if (sscanf(optarg,"%d",&osem3dpar->npskip) != 1)
					 crash2("error decoding -k %s\n",optarg);
				 break;
			 case 'N' :
				 osem3dpar->normfac_in_file_flag=0;
				 break;
			 case 'M':
				 if (sscanf(optarg,"%d",&osem3dpar->iModel) != 1)
					 crash2("error decoding -M %s\n",optarg);
				 if(osem3dpar->iModel==0) {
					 i++;
					 osem3dpar->imodeldefname=argv[i];
				 }
				 break;
			 case 'A':
				 osem3dpar->mapemflag=1;
				 break;
			case 'B':
				if (sscanf(optarg,"%f,%f,%f",&osem3dpar->blur_fwhm1,&osem3dpar->blur_fwhm2,&osem3dpar->blur_ratio) != 3)
					crash2("error decoding -B %s\n",optarg);
				osem3dpar->blur=1;
				break;
      case 'X':
				if (sscanf(optarg,"%d",&x_pixels) != 1)
					crash2("error decoding -X %s\n",optarg);
        y_pixels = x_pixels;
        break;
			 default:
         printf("\n*****Unknown option '%c' *********\n\n", c);
				 usage();
		}  
	}/* end of while  */

#ifndef NO_ECAT_SUPPORT
  if (osem3dpar->out_img_file != NULL) {
    char *ext = strrchr(osem3dpar->out_img_file,'.');
    if (ext != NULL) 
      if (tolower(ext[1]) == 'v') ecat_flag = 1;
  }
#endif
  if (osem3dpar->tFlag && strstr(osem3dpar->true_file,".dyn") != NULL)
    num_em_frames = get_num_frames(osem3dpar->true_file);
  if (ecat_flag && num_em_frames>0) { // restore output file
    strcpy(out_img_file, osem3dpar->out_img_file);
  }
}

void CheckParameterAndPrint()
{
	/* check mandatory arguments & conflicting arguments, calculate dependency-parameters */
	fprintf(stdout,"OSEM3D weighted reconstruction for parallel projections in flat file format\n");

	if (out_img_file==NULL && num_em_frames<2) /* automatic image filenames for multiframe recon */
		if ( ! ( oFlag || o3Flag || iterations == 0 ) ) 
			usage(); /* !sv  might be OK if forward proj only */

	if (iFlag) { 
		fprintf                (stdout,"  Input image          : %s\n",in_img_file);
		if (inflipFlag) fprintf(stdout,"  Input image will be flipped x-y \n");
	}
	else                fprintf(stdout,"  Starting from a uniform image \n");

	if (sFlag)          LogMessage("  Input scatter scan   : %s\n",scat_scan_file);

	if (tFlag)          fprintf(stdout,"  Input true scan      : %s\n",true_file);
	if (pFlag) {
		LogMessage("  Input prompt scan    : %s\n",prompt_file);
		if (delayed_file) LogMessage("  Input delayed scan   : %s\n",delayed_file);
    else {
      fprintf(stdout,"\n\n***********  Input delayed scan  file is missing****\n\n");
      usage();
    }
	}

	if(floatFlag)       LogMessage("  Input scan is in float format \n");

	if(nFlag)           LogMessage("  Normalization        : %s\n",norm_file);
	else                LogMessage("  Assuming uniform normalization \n");

	if(noattenFlag)     LogMessage("  Assuming attenuation is 1...\n");
	else                LogMessage("  Attenuation          : %s\n",atten_file);

	LogMessage("  Output image         : %s\n",out_img_file);

	if (outflipFlag)    LogMessage("  Output image will be flipped x-y \n");
	//  if (oFlag)          fprintf(stdout,"  Output corrected 2D scan: %s\n",out_scan_file);
	if (o3Flag)         LogMessage("  Output corrected 3D scan: %s\n",out_scan3D_file);

	switch(weighting) {
	  case 0:
		  LogMessage("  Method               : UWO3D\n");
		  break;
	  case 1:
		  LogMessage("  Method               : AWO3D\n");
		  break;
	  case 2:
		  LogMessage("  Method               : ANWO3D\n");
		  break;
	  case 3:
		  LogMessage("  Method               : OPO3D\n");
		  if (tFlag) crash1("  ... it requires data acquired in Prompt and separate Delayed mode\n");
		  break;
	}

	if (positFlag==1) {
		LogMessage("  Positivity constraint in image space (maybe useful for UW-AW-ANW schemes only) \n");
		weighting += 5;
	} else {
		LogMessage("  Positivity constraint in sinogram space\n");
	}
	LogMessage("  Max of sensitivity image      : %e\n",1./epsilon1);
	LogMessage("  Threshold for missing data    : %e\n",epsilon2);
	LogMessage("  Threshold for image update    : %e\n",epsilon3);
	LogMessage("  Maximum of image      	: %f\n",image_max);

	/* verbosity */
	if (verbose > 0) {
		fprintf(stdout,"  Verbosity level : %d \n", verbose);
		if (verbose & 0x0001) fprintf(stdout,"   - Scanner info requested \n");
		if (verbose & 0x0002) fprintf(stdout,"   - Write numerator of update equation and sensitivity \n");
		if (verbose & 0x0004) fprintf(stdout,"   - Normalisation factor info requested \n");
		if (verbose & 0x0008) fprintf(stdout,"   - Reconstruction info requested \n");
		if (verbose & 0x0010) fprintf(stdout,"   - I/O info requested \n");
		if (verbose & 0x0020) fprintf(stdout,"   - Processing info requested \n");
		if (verbose & 0x0040) fprintf(stdout,"   - Memory allocation info requested \n");
	}
	if (blur) LogMessage("  Reconstruction with %d subsets and %d iteration(s) with PSF\n",
                        subsets, iterations );
  else LogMessage("  Reconstruction with %d subsets and %d iteration(s)\n",
                        subsets, iterations );
	if(saveFlag) LogMessage("  ... an image will be saved after each iteration ! \n");
}

void GetScannerParameter()
{
	FILE *tmpfp;
	/* sets mosts global variables from ecat_model and 3D scan header*/
	/* set data from scanner_model */
	//    model_info    = scanner_model(iModel);           /* assumed code for HRRT */
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
  
  // set rebinning flag if reduced image size requested
  if (x_pixels != 0 && x_pixels<def_elements)
    x_rebin = v_rebin = def_angles/x_pixels;

	sino_sampling = model_info->binsize*x_rebin;              /* bin_size in mm */


	z_pixels      = rings*2-1;                        /* 104 direct planes => 207 planes */
	//  simd operations on 4 pixels at a time
	if(z_pixels%4==0) z_pixels_simd=z_pixels/4;
	else              z_pixels_simd=z_pixels/4+1;     // when remainder is 1..3 pixels 

	ring_spacing  = 2.0f*z_size;                      /* ring spacing in mm */    
	if(span==0) span=model_info->span;
	if(maxdel==0) maxdel=model_info->maxdel;

	groupmax      = (maxdel - (span-1)/2)/span;       /* should be 7 */
	radial_pixels = def_elements/x_rebin;                     /* global variable */
	views         = def_angles/v_rebin;                       /* global variable */
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
	LogMessage("  Triming %d planes at segment edges \n",npskip);
	if (npskip > z_min) z_min = npskip;
	if (z_max  > z_pixels-npskip) z_max = z_pixels - npskip; 
	LogMessage("  z_min max : %d %d npskip %d\n",z_min,z_max,npskip);

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
	FILE *tmpfp;

	if ( iterations > 0 ){// !sv  we need only image to generate fully corrected 3D scans by forward projection 
		/* open input files  */
		if (tFlag) {            
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

		if(nFlag) {            
			normfp = file_open(norm_file,"rb");
			if(normfp == FOPEN_ERROR) LogMessage("  Error: cannot open normalization file %s\n",norm_file);
			if(normfp == FOPEN_ERROR) crash2("  Error: cannot open normalization file %s\n",norm_file);
		}

		if (noattenFlag == 0) {            
			attenfp = file_open(atten_file,"rb");
			if(attenfp == FOPEN_ERROR) LogMessage("  Error: cannot open attenuation file %s\n",atten_file);    /* attenfp could be null */
			if(attenfp == FOPEN_ERROR) crash2("  Error: cannot open attenuation file %s\n",atten_file);    /* attenfp could be null */
		}

		if(sFlag) {            
			ssinofp = file_open(scat_scan_file,"rb");
			if(ssinofp == FOPEN_ERROR) LogMessage("  Error: cannot open input scatter sinogram file %s\n",scat_scan_file);
			if(ssinofp == FOPEN_ERROR) crash2("  Error: cannot open input scatter sinogram file %s\n",scat_scan_file);
		}
	}
	/* check initial image file existence */
	if (iFlag) {
		tmpfp=fopen(in_img_file,"rb");
		if(tmpfp==NULL) crash2("  Main(): error when opening the initial image from %s \n", in_img_file );
		else fclose(tmpfp);
	}
  if((normfacfp = file_open(normfac_img,"rb")) == FOPEN_ERROR) {
     calnormfacFlag=1;
  }
	else {
		calnormfacFlag=0;
		file_close(normfacfp);
	}
}
void Output_CorrectedScan()
{
	clock_t ClockA;
	int x,y,z;
	FILE *tmpfp;
	float **tmpfptr2d;
	int yr,v,yr2,theta;


	if(o3Flag){
			alloc_tmprprj1();
			alloc_tmprprj2();

	//	prepare_osem3dw3(nFlag,noattenFlag,tFlag,floatFlag,sFlag,weighting,normfp,attenfp,tsinofp,psinofp,dsinofp,ssinofp);
	//	make_compress_mask_short(largeprjshort3);

		time(&t1);
		ClockA=clock();
		if(iterations==0){
			image = (float ***) matrix3dm128(0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd-1);        
			if( image == NULL ) crash1("  Main(): error! memory allocation failure for image !\n");
			else if (verbose & 0x0008) fprintf(stdout,"  Allocate initial image : %d kbytes \n", imagesize/256);
		}
		if (iFlag && iterations==0) {
			tmpfp=fopen(in_img_file,"rb");
			if(tmpfp==NULL) 
				crash2("  Main(): error in read_flat_image() when reading the initial image from %s \n", in_img_file );
			tmpfptr2d=matrixfloat(0,x_pixels-1,0,y_pixels-1);
			for(z=0;z<z_pixels;z++){
				fread(&tmpfptr2d[0][0],x_pixels*y_pixels,sizeof(float),tmpfp);
				if(inflipFlag){
					/* Flip x<->y (in place, since first time) */
					for(x=0;x<x_pixels;x++){
						for(y=0;y<y_pixels;y++){
							image[y][x][z]=tmpfptr2d[x][y];
						}
					}
				} else {
					for(x=0;x<x_pixels;x++){
						for(y=0;y<y_pixels;y++){
							image[x][y][z]=tmpfptr2d[y][x];
						}
					}
				}
			}

			free_matrix(tmpfptr2d,0,x_pixels-1,0,y_pixels-1);
			fclose(tmpfp);
		} else if(iterations==0){ // no image => forward project the cylindrical mask
			for( x=0; x<x_pixels; x++ ){ 
				memset(&image[x][0][0],0,y_pixels*z_pixels_simd*sizeof(__m128));
				for( y=cylwiny[x][0]; y<cylwiny[x][1];y++){ 
					for( z=z_min; z<z_max; z++ ) image[x][y][z] =  1.0;
				}
			}
		}
		alloc_imagexyzf();
		alloc_prjxyf();
		printf("%d %d %d\n",z_pixels,x_pixels,y_pixels);
//		memset(&largeprj3[0][0][0],0,xr_pixels*views*segmentsize*sizeof(float));
		for(yr=0;yr<segmentsize;yr++){
			for(v=0;v<views;v++){
				memset(largeprj3[yr][v],0,xr_pixels*sizeof(float));
			}
		}
		for(v=0;v<views/2;v++){
			printf("  Progress %d %d %%\r",v,(int)((v+0.0)/views*2.0*100));
//			if( !forward_proj3d2_compress(image, estimate, v,nthreads,imagexyzf,prjxyf))
			if( !forward_proj3d2(image, estimate, v,nthreads,imagexyzf,prjxyf))
//				if(!forward_proj3d_thread1_compress(image,estimate,v,1,imagexyzf,prjxyf))
				crash1("  Main(): error occurs in forward_proj3d!\n");
			for(yr2=0;yr2<segmentsize*2;yr2++) memcpy(largeprj[v][yr2],estimate[yr2],xr_pixels*4);
		}

		time(&t2);
		fprintf(stdout,"  Osem3d forward projection done in %d %2.5f sec\n",t2-t1,(clock()-ClockA+0.0)/CLOCKS_PER_SEC);
		tmpfp=fopen(out_scan3D_file,"wb");
		if (muFlag) {
			float *p;
			int xr;
			fprintf(stdout,"  Calculating attenuation correction mu in cm-1 and FP in mm\n");
			p = &largeprj3[0][0][0];
//			for (i=0; i<xr_pixels*views*segmentsize; i++, p++) *p = exp(*p/10.);
			for(yr=0;yr<segmentsize;yr++){
				for(v=0;v<views;v++){
					for(xr=0;xr<xr_pixels;xr++){
						largeprj3[yr][v][xr]=exp(largeprj3[yr][v][xr]/10);
					}
				}
			}
		} 
		// 	write line by line in sinogram mode 
		yr2=0;
		for(theta=0;theta<th_pixels;theta++){
			//			yr2=segment_offset[theta];
			printf("yr2=%d\n",yr2);
			for( yr=yr_bottom[theta]; yr<=yr_top[theta]; yr++, yr2++) {
				for( v=0; v<views; v++ ) {
					if(fwrite( &largeprj3[yr2][v][0], radial_pixels*sizeof(float), 1, tmpfp) != 1 ){
						fprintf(stdout,"   write error at plane %d view %d \n", yr2,v);
					}
				}
				//			for( v=0; v<views/2; v++ ) {
				//				if(fwrite( &largeprj3[yr2][v][radial_pixels], radial_pixels*sizeof(float), 1, tmpfp) != 1 ){
				//					fprintf(stdout,"   write error at plane %d view %d \n", yr2,v);
				//				}
				//			}
			}
		}
		fclose(tmpfp);
		free_imagexyzf();
		free_prjxyf();
	}
	printf("done\n");
}

void GetSystemInformation()
{
#ifdef IS_WIN32 
	SYSTEM_INFO siSysInfo;
#else
    /* ahc */
	/* struct sysinfo sinfo; */
#endif
	printf(" Hardware information: \n");  
#ifdef IS_WIN32
	GetSystemInfo(&siSysInfo); 
	printf("  OEM ID: %u\n", siSysInfo.dwOemId);
	printf("  Number of processors: %u\n", siSysInfo.dwNumberOfProcessors); 
	printf("  Page size: %u\n", siSysInfo.dwPageSize); 
	printf("  Processor type: %u\n", siSysInfo.dwProcessorType); 
	printf("  Minimum application address: %lx\n", 
		siSysInfo.lpMinimumApplicationAddress); 
	printf("  Maximum application address: %lx\n", 
		siSysInfo.lpMaximumApplicationAddress); 
	if(nthreads==0) nthreads=siSysInfo.dwNumberOfProcessors;
#else
    /* ahc */
    /*
	sysinfo(&sinfo);
	printf("  Total RAM: %u\n", sinfo.totalram); 
	printf("  Free RAM: %u\n", sinfo.freeram);
	// To get n_processors we need to get the result of this command "grep processor /proc/cpuinfo | wc -l"
    */
	if ( nthreads == 0 )
      nthreads = 2; // for now
#endif
}
void PrepareNormfac()
{
	FILE *tmpfp;
	int theta,yr2,yr,v;

	time(&t1);
	alloc_tmprprj1();
	prepare_norm(largeprj3,nFlag,weighting,normfp,attenfp);
	free_tmpprj1();
	if(newzoom!=1) {
		if(newzoom<1) zoom_proj_reduce(largeprj3,newzoom);
		else  zoom_proj_zoomup(largeprj3,newzoom);
	}
	time(&t0);
	if (verbose & 0x0004)printf(" Preparation done in %d sec\n",t0-t1);

	if (verbose & 0x0002) {
		tmpfp=fopen("sensitivity.scn","wb");
		fprintf(stdout,"  Writing sensitivity.scn \n");
		// write line by line in sinogram mode 
		for(theta=0;theta<th_pixels;theta++){
			yr2=segment_offset[theta];
			for( yr=yr_bottom[theta]; yr<=yr_top[theta]; yr++, yr2++) {
				for( v=0; v<views; v++ ) {
					if( fwrite( &largeprj3[yr2][v][0], radial_pixels*sizeof(float), 1, tmpfp) != 1 )
					{ fprintf(stdout,"   write error at plane %d view %d \n", yr2,v);}
				}
			}
		}
		fclose(tmpfp);    
	}
}


void CalculateNormfac()
{
	int v,thr,isubset;
	int view;
	File_structure normstructure;
	FPBP_thread bparg[MAXTHREADS];
	unsigned int threadID;
	int calnormfac[MAXTHREADS];
	clock_t ClockB;

	if(verbose & 0x0004) {
		fprintf(stdout,"  Calculating Normalization (sensitivity) image.... \n");
		fprintf(stdout,"  ... preparation will take a minute.. depends on HDD speed\n");
	}

	// CM debugging : largeprj3 contains sensitivity data
	alloc_thread_buf();
	alloc_normfac();

	if(blur){
		if(image_psf==NULL) image_psf= (float ***) alloc_imagexyzf_thread();
	}

	normstructure.normfac     = correction;
	normstructure.normfac_img = normfac_img;
	normstructure.verbose     = verbose;

	for( isubset=0; isubset<subsets; isubset++ ){  
		if(verbose & 0x0004) fprintf(stdout,"  subset %d \n", isubset );
		memset( &normfac[0][0][0], 0, x_pixels*y_pixels*z_pixels_simd*sizeof(__m128));//(z_pixels+1)*sizeof(float) );
		ClockNormfacTheta = clock ();
		if(normfac_in_file_flag!=-1) correction=normfac_infile[isubset];

		//must use 16 subsets, because of using 90 degree symmetry in this loop.
		// we should find a way to test if the 90 deg projection is in same subset.

		for(v=0;v<sviews/2-nthreads+1;v+=nthreads){
			for(thr=0;thr<nthreads;thr++){
				view=vieworder[isubset][v+thr];
				bparg[thr].prj=largeprj[view];//estimate_thread[thr];
				bparg[thr].ima=normfac;
				bparg[thr].view=view;
				bparg[thr].numthread=1;
				bparg[thr].imagebuf=imagexyzf_thread[thr];
				bparg[thr].prjbuf=prjxyf_thread[thr];
				//					for(yr2=0;yr2<segmentsize*2;yr2++) memcpy(estimate_thread[thr][yr2],largeprj[view][yr2],xr_pixels*4);
//				START_THREAD(threads[Cal1+thr],pt_back_proj3d_thread1,bparg[thr],threadID);
				 START_THREAD(threads[Cal1+thr],pt_back_proj3d_thread1,bparg[thr],threadID);
			}
			for(thr=0;thr<nthreads;thr++)	Wait_thread( threads[Cal1+thr]);
			ClockB=clock();
			rotate_image_buf(imagexyzf_thread,normfac,nthreads,&vieworder[isubset][v]);
			if (verbose & 0x0008) fprintf(stdout," Rotation done in %3.3f\n",(clock()-ClockB+0.0)/CLOCKS_PER_SEC);
		}

		ClockB=clock();
		if(((sviews/2)%nthreads)!=0){
			for(v=sviews/2-(sviews/2)%nthreads;v<sviews/2;v++){
				view=vieworder[isubset][v];
				//			for(yr2=0;yr2<segmentsize*2;yr2++) memcpy(estimate_thread[0][yr2],largeprj[view][yr2],xr_pixels*4);
				//			if( !back_proj3d2(estimate_thread[0],normfac, view, nthreads,imagexyzf_thread[0],prjxyf_thread[0])) 
				if( !back_proj3d2(largeprj[view],normfac, view, nthreads,imagexyzf_thread[0],prjxyf_thread[0])) 
					crash3("  Main(): error occurs in back_proj3d, for subset %d \n ", isubset); 
			}
		}

		if (verbose &0x0008) fprintf(stdout," Residual parts done in %3.4f sec\n",(clock()-ClockB+0.0)/CLOCKS_PER_SEC);

		if(verbose & 0x0004) fprintf(stdout,"calculating normalization with isubset = %1d/%1d done in %1.4fs.\n",
			isubset, subsets, 1.0 * ( clock () - ClockNormfacTheta  ) / CLOCKS_PER_SEC );
		if(normfac_in_file_flag==-1){
      if(isubset!=0) Wait_thread( threads[Wnormfact]);
    }

		if(blur){
//			for(x=0;x<x_pixels;x++){
//				for(y=cylwiny[x][0];y<cylwiny[x][1];y++) memcpy(image_psf[x][y],normfac[x][y],z_pixels_simd*16);
//			convolve3d(normfac,image_psf,imagexyzf_thread[0]);
			convolve3d(normfac,normfac,imagexyzf_thread[0]);
//			convolve3d(image_psf,,imagexyzf_thread[0]);
		}
		for(thr=0;thr<2;thr++){
			calnormfac[thr]=thr;
			START_THREAD(threads[Cal1+thr],pt_normfac_calculate,calnormfac[thr],threadID);
		}

		for(thr=0;thr<2;thr++)Wait_thread( threads[Cal1+thr]);

		normstructure.isubset=isubset;
		if(normfac_in_file_flag==-1){
			START_THREAD(threads[Wnormfact],pt_write_norm,normstructure,threadID);
			if(isubset==subsets-1)    Wait_thread( threads[Wnormfact]);
		}
		if (verbose & 0x0004) fprintf(stdout," Normfac %d/%d done in %3.2f sec\n",isubset,subsets,(clock()-ClockNormfacTheta+0.0)/CLOCKS_PER_SEC);
	} /* end subset */

	free_normfac();

	if(normfac_in_file_flag==-1) free_correction();
	free_thread_buf();

	time(&t1);
	// if (verbose & 0x0004)
	LogMessage("  Normalization (sensitivity) factors calculated in %d sec\n",t1-t0);
}

/**************************************************************************************************************/
/* 
Main program starts 

each subset is 3.6 Mb => 14.6 Mb in memory 
each image (128x128x207) is 13 Mb => 39 Mb in memory - would be 156 Mb for (256x256*207)
there is one normalization image per subset, so norm_fac.img is 13 Mb * 16 = 208 Mb in 128 (and 832 Mb in 256)

*/ 
void PrepareOsem3dFiles()
{
	FILE *tmpfp;
	int theta;
	int yr2;
	int yr;
	int v;
	/*---------------------------------------------------------------*/
	/* Start iterations; one iteration goes through the whole data set    */

	time(&t0);
	// if (verbose & 0x0008) 
	LogMessage("  Prepare components of update equation from input data ...\n");
	alloc_tmprprj1();
	alloc_tmprprj2();

		if(weighting==0 || weighting==5) 
			prepare_osem3dw0(nFlag,noattenFlag,tFlag,floatFlag,sFlag,weighting,normfp,attenfp,tsinofp,psinofp,dsinofp,ssinofp);
		else if(weighting==1 || weighting==6) 
			prepare_osem3dw1(nFlag,noattenFlag,tFlag,floatFlag,sFlag,weighting,normfp,attenfp,tsinofp,psinofp,dsinofp,ssinofp);
		else if(weighting==2 || weighting==7) 
			prepare_osem3dw2(nFlag,noattenFlag,tFlag,floatFlag,sFlag,weighting,normfp,attenfp,tsinofp,psinofp,dsinofp,ssinofp);
		else if(weighting==3 || weighting==8) 
			prepare_osem3dw3(nFlag,noattenFlag,tFlag,floatFlag,sFlag,weighting,normfp,attenfp,tsinofp,psinofp,dsinofp,ssinofp);
	time(&t1);

	// if (verbose & 0x0008)
	LogMessage("  Preparation done in %d sec\n",t1-t0);
	if(newzoom!=1) { // apply zoom on numerator or prompt and shift in update equation 
		if(newzoom<1) zoom_proj_reduce(largeprj3,newzoom);
		else	      zoom_proj_zoomup(largeprj3,newzoom);
		if(weighting==3 && newzoom<1)      zoom_proj_short_reduce(largeprjshort3,newzoom);
		else if(weighting==3 && newzoom>1) zoom_proj_short_zoomup(largeprjshort3,newzoom);
	}

	free_tmpprj1();
	free_tmpprj2();
#ifdef WIN32
  // MS: this lines crashes on LINUX, perhaps due to bug hidden somewhere else
//	free_matrix3dshort(tmp_for_short_sino_read,0,yr_pixels-1,0,views-1,0,xr_pixels-1);
  if (tmp_for_short_sino_read != NULL) {
    free_matrix3dshortm128(tmp_for_short_sino_read,0,yr_pixels-1,0,views-1,0,xr_pixels/8-1);
    tmp_for_short_sino_read = NULL;
  }

#endif
	// CM debugging : largeprj3 contains numerator of update equation
	printf("verbose %x\n",verbose);
	if (verbose & 0x0002) {
		tmpfp=fopen("largeprj3.scn","wb");
		fprintf(stdout,"  Writing largeprj3.scn \n");
		// write line by line in sinogram mode 
		for(theta=0;theta<th_pixels;theta++){
			yr2=segment_offset[theta];
			for( yr=yr_bottom[theta]; yr<=yr_top[theta]; yr++, yr2++) {
				for( v=0; v<views; v++ ) {
					if( fwrite( &largeprj3[yr2][v][0], radial_pixels*sizeof(float), 1, tmpfp) != 1 )
					{ fprintf(stdout,"   write error at plane %d view %d \n", yr2,v);}
				}
			}
		}
		fclose(tmpfp);    
	}

	if(CompressFlag==1){
		if(weighting!=3) make_compress_mask(largeprj3);
		else if(weighting==3) make_compress_mask_short(largeprjshort3);
		compress_sinogram_and_alloc_mem(largeprj3,compress_prj);
		printf("compress mask flag setted!\n");
//		free_matrixm128(largeprj3,0,segmentsize-1,0,views-1,0,xr_pixels/4-1);
	}


	if ( iterations > 0 ){ // close input files  
		if (tFlag) file_close(tsinofp);
		else{
			file_close(psinofp);
			file_close(dsinofp);
		}
		if(nFlag) file_close(normfp);
		if (noattenFlag == 0) file_close(attenfp);
		if(sFlag) file_close(ssinofp);            
	}
		/*  read initial image or initialize it in a cylindrical window */
	/*---------------------------------------------------------------
	* initialize image 
	* image size : 51.75M(256) 12.94M(128)
	*/	

//	image = (float ***) matrix3dm128(0,x_pixels-1, 0,y_pixels-1, 0,z_pixels_simd-1);     
}
void Init_Image()
{
	FILE *tmpfp;
	float **tmpfptr2d;
	int x,y,z;

//	image= (float ***) alloc_imagexyzf_thread();
	image= (float ***) matrix3dm128(0,x_pixels-1,0,y_pixels-1,0,z_pixels_simd-1);
	if(image_psf==NULL) image_psf= (float ***) alloc_imagexyzf_thread();
	if( image == NULL ) crash1("  Main(): error! memory allocation failure for image !\n");
	else if (verbose & 0x0008) fprintf(stdout,"  Allocate initial image : %d kbytes \n", imagesize/256);

	if (iFlag) {
		tmpfp=fopen(in_img_file,"rb");
		if(tmpfp==NULL) crash2("  Main(): error when opening initial image from %s \n", in_img_file );
		tmpfptr2d=matrixfloat(0,x_pixels-1,0,y_pixels-1);

		for(z=0;z<z_pixels;z++){
			fread(&tmpfptr2d[0][0],x_pixels*y_pixels,sizeof(float),tmpfp);
			if(inflipFlag){
				/* Flip x<->y (in place, since first time) */
				for(x=0;x<x_pixels;x++){
					for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
						image[x][y][z]=tmpfptr2d[x][y];
					}
				}
			} else {
				for(x=0;x<x_pixels;x++){
					for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
						image[y][x][z]=tmpfptr2d[x][y];
					}
				}
			}
		}
		free_matrix(tmpfptr2d,0,x_pixels-1,0,y_pixels-1);
		fclose(tmpfp);
	} 
	else { 
		for( x=0; x<x_pixels; x++ ){ 
			memset(&image[x][0][0],0,y_pixels*z_pixels_simd*sizeof(__m128));
			if(cylwiny[x][0]==cylwiny[x][1]) continue;
			for( y=cylwiny[x][0]; y<cylwiny[x][1];y++){ 
				for( z=z_min; z<z_max; z++ ) image[x][y][z] =  1.0;
			}
		}
	}
}

// Re-initialize image for next frame reconstruction
void ResetImage()
{
		for( int x=0; x<x_pixels; x++ ){ 
			memset(&image[x][0][0],0,y_pixels*z_pixels_simd*sizeof(__m128));
			if(cylwiny[x][0]==cylwiny[x][1]) continue;
			for( int y=cylwiny[x][0]; y<cylwiny[x][1];y++){ 
				for( int z=z_min; z<z_max; z++ ) image[x][y][z] =  1.0f;
			}
		}
}

void SaveImage(char *out_img_file,int iter, int frame)
{
	float ***tmpimage;
	int x,y,z;
	ImageHeaderInfo *Info;

	tmpimage = (float ***) matrix3d(0,z_pixels-1, 0,y_pixels-1, 0,x_pixels-1);        
	if( tmpimage == NULL ) crash1("  Main(): memory allocation failure for tmp image !\n");
	else if (verbose & 0x0008) fprintf(stdout,"  Allocate tmpimage     : %d kbytes \n", imagesize/256);
	memset(&tmpimage[0][0][0],0, x_pixels*y_pixels*z_pixels*sizeof(float));

	if (outflipFlag) { 
		for(x=0;x<x_pixels;x++) for(y=cylwiny[x][0];y<cylwiny[x][1];y++) for(z=0;z<z_pixels;z++) tmpimage[z][y][x]=image[y][x][z];
		fprintf(stdout,"  Intermediate image has been flipped \n");
	} 
	else {
		for(x=0;x<x_pixels;x++) 
			for(y=cylwiny[x][0];y<cylwiny[x][1];y++)
				for(z=0;z<z_pixels;z++) 
					tmpimage[z][y][x]=image[x][y][z];
	}

  if (!ecat_flag) {
    if( !write_flat_image(tmpimage, out_img_file, 0, verbose) )
      crash1("  Main(): error occurs in write_flat_image !\n");
  }


	Info = (ImageHeaderInfo *) calloc(1,sizeof(ImageHeaderInfo));

	Info->recontype	= weighting;//0=UW, 1=AW, 2=ANW, 3=OP
	Info->datatype	= 3;		//3=float
	Info->iterations	= iter;		//3=float

	Info->nx = x_pixels;	//image size
	Info->ny = y_pixels;	//image size
	Info->nz = z_pixels;	//image size

	Info->dx = x_size;	//pixel size
	Info->dy = y_size;	//pixel size
	Info->dz = z_size;	//pixel size

	Info->truefile   [0] = Info->promptfile [0] = '\0';
	Info->delayedfile[0] = Info->normfile   [0] = '\0';
	Info->attenfile  [0] = Info->scatterfile[0] = '\0';
	strcpy(Info->imagefile, out_img_file);
	if( true_file	 != NULL ) strcpy( Info->truefile    ,true_file    );
	if( prompt_file	 != NULL ) strcpy( Info->promptfile  ,prompt_file  );
	if( delayed_file != NULL ) strcpy( Info->delayedfile ,delayed_file );
	if( norm_file	 != NULL ) strcpy( Info->normfile    ,norm_file	   );
	if( atten_file	 != NULL ) strcpy( Info->attenfile   ,atten_file   );
	if( scat_scan_file != NULL ) strcpy( Info->scatterfile ,scat_scan_file );

	Info->subsets = subsets;
	//	strcpy(Info->BuildTime,BuildTime());

#ifndef NO_ECAT_SUPPORT
  if (!ecat_flag) {
    write_image_header(Info, blur, sw_version, sw_build_id);
  } else
      if( !write_ecat_image(tmpimage, out_img_file, frame, Info, blur, sw_version, sw_build_id) )
        crash1("  Main(): error occurs in write_ecat_image !\n");
#else
    if (!write_image_header(Info, blur, sw_version, sw_build_id))
       crash1("  Main(): error occurs in write_flat_image !\n");
#endif

	/***********************************/
	/* free memory before reprojection */
	free_matrix3d(tmpimage, 0,z_pixels-1,0,y_pixels-1,0,x_pixels-1 );
	if (verbose & 0x0008) fprintf(stdout,"  Free tmpimage            : %d kbytes \n", imagesize/256);

}

void CalculateOsem3d(int frame)
{
  char *iter_file=NULL, *pext=NULL;
	int iter;
	int i,isubset;
	unsigned int threadID;
	int v,thr,view,count,x,y,z;
	float *fptr1,*fptr2,*fptr3;
	clock_t ClockA;
	clock_t ccomm;

	FPBP_thread bparg[MAXTHREADS];
	BPFP_ptargs normarg[MAXTHREADS];
	File_structure normstructure;


  if (frame == 0) {
    order = (short int *) calloc( subsets, sizeof(short int) );
    if( !sequence(subsets, order) ) crash1("  Main(): error occurs in sequence() !\n");

    if(normfac_in_file_flag==-1){
      alloc_normfac();
    }
    alloc_correction();
    alloc_thread_buf();

    if(positFlag==1){
      alloc_imagemask();
      memset(&imagemask[0][0][0],0,x_pixels*y_pixels*z_pixels_simd*4);
    }
  }

//	swap_xr_yr(largeprj3,largeprj_swap);
//	swap_xr_yr();
//	if(weighting==3 || weighting==8){
//		swap_xr_yr_short(largeprjshort3,largeprjshort_swap);
//	}
	normstructure.normfac     = normfac;
	normstructure.normfac_img = normfac_img;
	normstructure.verbose     = verbose;

  if (saveFlag) {
    iter_file = (char*)calloc(_MAX_PATH,1);
    strcpy(iter_file, out_img_file);
    if ((pext = strrchr(iter_file,'.')) == NULL) {
      // no extension: pointer to end of string
      pext = iter_file + strlen(iter_file);
    }
  }
	for( iter=0; iter<iterations; iter++ ){  
		if( verbose & 0x0008) LogMessage("  Starting iteration %d \n", iter+1);
		if ( saveFlag ) {         /* form the filename for intermediate images */
			if (iter+1<10) sprintf(pext,"i0%d.i",iter+1);
      else sprintf(pext,"i%d.i",iter+1);
			LogMessage("  Image will be saved in %s \n", iter_file);
		}
		time(&t0);
		ClockA=clock();
		if(normfac_in_file_flag==2){
			if(iter!=0) normfac_in_file_flag=1;
		}
		for( i=0; i<subsets; i++ ){
			normstructure.normfac     = normfac;
			normstructure.normfac_img = normfac_img;
			normstructure.verbose     = verbose;
			//read normfac.i file!!
			isubset = order[i];
			normstructure.isubset=isubset;
			if(normfac_in_file_flag==-1 || normfac_in_file_flag==2){
				START_THREAD(threads[Rnormfact],pt_read_norm,normstructure,threadID);
			} 
			ClockNormfacTheta = clock ();                /* reuse previous timing variable */

			if(blur) convolve3d(image,image_psf,imagexyzf_thread[0]);

			for(v=0;v<sviews/2-nthreads+1;v+=nthreads){
				for(thr=0;thr<nthreads;thr++){
					
					view=vieworder[isubset][v+thr];
					bparg[thr].prj=estimate_thread[thr];
					if(blur) bparg[thr].ima=image_psf;
					else  bparg[thr].ima=image;
					bparg[thr].imagepack=imagepack;
					bparg[thr].view=view;
					bparg[thr].numthread=1;
					bparg[thr].imagebuf=imagexyzf_thread[thr];
					bparg[thr].prjbuf=prjxyf_thread[thr];
					bparg[thr].volumeprj=largeprj[view];
					
						if (pFlag && (weighting == 3 || weighting==8) && floatFlag==0){
							bparg[thr].volumeprjshort=largeprjshort[view];
						}
						else if (pFlag && (weighting == 3 || weighting==8) && floatFlag==1)	bparg[thr].volumeprjfloat=largeprjfloat[view];
					bparg[thr].weighting=weighting;
					START_THREAD(threads[Cal1+thr],pt_osem3d_proj_thread1,bparg[thr],threadID);
				}
				for(thr=0;thr<nthreads;thr++) Wait_thread( threads[Cal1+thr]);
				rotate_image_buf(imagexyzf_thread,correction,nthreads,&vieworder[isubset][v]);
			} 		
			if(((sviews/2)%nthreads)!=0){
				for(v=sviews/2-(sviews/2)%nthreads;v<sviews/2;v++){
					view=vieworder[isubset][v];
					if(blur){
						if( !forward_proj3d2(image_psf, estimate_thread[0], view,verbose,imagexyzf_thread[0],prjxyf_thread[0]))
							crash3("  Main(): error occurs in forward_proj3d at subset %d !\n",isubset );        
					} else {
						if( !forward_proj3d2(image, estimate_thread[0], view,verbose,imagexyzf_thread[0],prjxyf_thread[0]))
							crash3("  Main(): error occurs in forward_proj3d at subset %d !\n",isubset );        
					}
					// UW, AW, ANW scheme
					if (weighting ==0 || weighting ==1 || weighting ==2 || weighting ==5 || weighting ==6 || weighting ==7 ) { 
						update_estimate_w012(estimate_thread[0],largeprj[view],segmentsize*2,xr_pixels,nthreads);
					}  else if (pFlag && (weighting == 3 || weighting==8)) {  
						if(floatFlag==0) update_estimate_w3(estimate_thread[0],largeprj[view],largeprjshort[view],NULL,segmentsize*2,xr_pixels,nthreads);
						else if(floatFlag==1) update_estimate_w3(estimate_thread[0],largeprj[view],NULL,largeprjfloat[view],segmentsize*2,xr_pixels,nthreads);
					}
					if( !back_proj3d2( estimate_thread[0], correction, view,verbose,imagexyzf_thread[0],prjxyf_thread[0])) 
						crash3("  Main(): error occurs in back_proj3d for subset %d \n ", isubset);
				}
			}
			if (verbose & 0x0008) fprintf(stdout," Calculating isubset = %1d/%1d in %1.2fs.\n",isubset, subsets, 1.0 * ( clock () - ClockNormfacTheta  ) / CLOCKS_PER_SEC );        

			if(normfac_in_file_flag==-1){		Wait_thread(threads[Rnormfact]);}
			else normfac=normfac_infile[order[i]];

			if(positFlag==0 || positFlag==2){
				count=0;
				x=0;
				y=0;
				
				if(blur) convolve3d(correction,correction,imagexyzf_thread[0]);
				if(mapemflag){
					calculate_derivative_mapem(image,gradient,laplacian);
				}
				
				for(thr=0;thr<nthreads;thr++){
					x+=start_x__per_thread[thr];
					y+=start_x__per_thread[thr+1];
					normarg[thr].image=image;
					normarg[thr].imagebuf=correction;
					normarg[thr].prj=normfac;
					normarg[thr].start=x;
					normarg[thr].end=y;
					if(thr==nthreads-1) normarg[thr].end=x_pixels;
					START_THREAD(threads[Cal1+thr],pt_cal_updateimage,normarg[thr],threadID);
				} 
				for(thr=0;thr<nthreads;thr++) Wait_thread( threads[Cal1+thr]);
			} else {
				count=0;
				for( x=0; x<x_pixels; x++ ) {
					for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) {
						fptr1=image[x][y];
						fptr2=correction[x][y];
						fptr3=normfac[x][y];
						if(iter>6){
							for( z=0; z<z_pixels; z++ ){
								if(fptr2[z]>0.0) fptr1[z]*=fptr2[z]*fptr3[z];
								if( fptr1[z]> image_max) {
									count++;
									fptr1[z] = image_max;		/* truncate - CJM Sept 2002 */
								}
							}
						} else {
							for( z=0; z<z_pixels; z++ ){
								if(fptr2[z]>0.0){
									fptr1[z]*=fptr2[z]*fptr3[z];
									imagemask[x][y][z]=1;
								}
								if( fptr1[z]> image_max) {
									count++;
									fptr1[z] = image_max;		/* truncate - CJM Sept 2002 */
								}
							}
						}
					}
				}
			}
			ccomm=clock();

			if (count != 0) if (verbose & 0x0008) fprintf(stderr,"  Number of truncated pixels for subset (%d) = %d\n",i,count);
			if (verbose & 0x0008) fprintf(stdout," Reconstructing isubset = %1d/%1d in %1.2fs.\n",
				isubset, subsets, 1.0 * ( clock () - ClockNormfacTheta  ) / CLOCKS_PER_SEC );  
//			goto inki_temp;
		}  /* end subset loop */

		ClockA=clock()-ClockA;
		time(&t1);
		LogMessage("  Iteration %d in %3.3f sec\n",iter+1, (ClockA+0.0)/CLOCKS_PER_SEC);
		t0 = t1;

		sum_image=0;
		for( x=0; x<x_pixels; x++ ) {
			for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) {
				fptr1=(float *) image[x][y];
				for( z=z_min; z<z_max; z++){
					sum_image+=fptr1[z];
					if(fptr1[z]<1e-16) fptr1[z]=0;
				}
			}
		}
		if(iter<1 && positFlag==1){
			count=0;
			for( x=0; x<x_pixels; x++ ) {
				for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) {
					fptr1=image[x][y];
					for( z=0; z<z_pixels; z++){
						fptr1[z]*=((float) imagemask[x][y][z]);
						if(imagemask[x][y][z]==0) count++;
						imagemask[x][y][z]=0;		
					}
				}
			}
			// printf("positive\n");
			if (count != 0) LogMessage("  Number of un-updated pixels = %d\n",count);
		}

    //Use old output file name convention when intermediate image are requested
		/* save intermediate image as long as it is not the last iteration   */
		if( saveFlag && (iter!=(iterations-1)) ) {    
			/* we need a temporary image, keep original untouched */
			free_correction();
			if(normfac_in_file_flag==-1) free_normfac();

			SaveImage(iter_file,iter+1,0);
			
			alloc_correction();
			if(normfac_in_file_flag==-1) alloc_normfac();
		} 
	}  /* end iteration loop */
  
  if (num_em_frames<2) {
    free_correction();
    free_normfac();
    free_thread_buf();
    free(order);
  }
  
  /* save last image in old filename conventions   */
  if ( saveFlag ) 
  {
			SaveImage(iter_file, iterations,0);
      free(iter_file);
  }
}


int main(int argc, char* argv[])
{
	int nprojs=0,nviews=0;
	ImageHeaderInfo*  Info=NULL;
//	__m128 coef;

	unsigned long ClockB=0,ClockF=0,ClockL=0,ClockA=0;
//	__m128 *mptr1,*mptr2,*mptr3;
  struct stat st;
  char user_name[80];


  sprintf(sw_build_id,"%s %s", __DATE__, __TIME__);
  printf("\nhrrt_osem3d sw_version %s Build %s\n\n", sw_version, sw_build_id);
	osem3dpar=(GLOBAL_OSEM3DPAR *) malloc(sizeof(GLOBAL_OSEM3DPAR));
	time(&t0);
	t2 = t0;
	x_pixels = y_pixels =0;
	
	GetSystemInformation();

	InitParameter();
	GetParameter(argc,argv);
  if (log_file) LogMessage("hrrt_osem3d sw_version %s Build %s\n", sw_version, sw_build_id);

	PutParameter();
	CheckParameterAndPrint();

  if (osem3dpar->normfac_in_file_flag!=0) {
    if (normfac_dir!=NULL) { 
      // normfac in specicied directory, check if directory is writable
      if (access(normfac_dir, 0x06) != 0) {
        LogMessage("Can't write in normfac directory %s\n", normfac_dir);
        crash2("Error: cannot accessing normfac directory %s\n", normfac_dir);
      }
      sprintf(normfac_img, "%s%cnormfac.i", normfac_dir,DIR_SEPARATOR);
    }
    else { // normfac in patient directory
      if (osem3dpar->atten_file!=NULL)
        normfac_path(osem3dpar->atten_file,normfac_img);
      else if (osem3dpar->prompt_file) 
        normfac_path(osem3dpar->prompt_file,normfac_img);
      else normfac_path(osem3dpar->true_file,normfac_img);
    }
  }

  if (num_em_frames > 0) {  // set frame 0 file names
    sprintf(true_file,"%s_frame0%s.s",em_file_prefix,em_file_postfix);
    if (!ecat_flag)
      sprintf(out_img_file,"%s_frame0%s.i",em_file_prefix,em_file_postfix);
  }

#ifdef WIN32
  DWORD len=sizeof(user_name);
  GetUserName(user_name,&len);
  LogMessage("Username : %s \n", user_name);
#endif

  memset(ra_smo_file,0,sizeof(ra_smo_file));
  CheckFileValidate();
	InitParameter();
	GetScannerParameter();

	/* calculate depended variables and some frequently used arrays and constants  - see HRRT_osem3d_sbrt.c */
	if( !dependencies(nprojs, nviews, verbose) ) crash1("  Main(): Error occur in dependencies() !");

  
  /* Get scatter file format : 3D or unscaled 2D and scale factors*/
  if (sFlag) {
    if (stat(osem3dpar->scat_scan_file, &st) == 0) {
      if (st.st_size == (xr_pixels*views*z_pixels + segmentsize_s9)*sizeof(float)) {
        s2DFlag = true;
      } else {
        if (st.st_size != (xr_pixels*views*segmentsize)*sizeof(float)) {
          if (max_g<0 && min_g<0)
          { // check file size
            LogMessage("%s: invalid file size %d, expected %d\n", scat_scan_file,
              st.st_size, (xr_pixels*views*segmentsize)*sizeof(float));
            exit(1);
          }
        }
      }
    }
    else crash2("%s: Error getting file size\n", scat_scan_file);
  }

  /* Get smoothed  file format : 3D or .ch file*/
  if (pFlag) {
    int ch_size = NHEADS*HEAD_XSIZE*HEAD_YSIZE*4;  // 4 combinations Back/front
    if (stat(osem3dpar->delayed_file, &st) == 0) {
      if (st.st_size == ch_size*sizeof(int)) {
        // ch format: compute smooth randoms
        // compute smoothed randoms into file
        if (!get_scan_duration(prompt_file))
          crash2("Error: scan duration not found in sinogram %s header\n",prompt_file);
       
#ifdef WIN_X64
        chFlag = true;                // inline random in prepare_osem3dw3
       LogMessage("Inline  randoms from %s, scan duration %g\n", 
          delayed_file, scan_duration);
#else  
        char *basename, *ext;
       // create random smoothed file in same directory as normfac.i
       if (normfac_dir != NULL) 
       {
         if ((basename=strrchr(delayed_file,DIR_SEPARATOR)) == NULL)
           basename=delayed_file;
         else basename += 1;
         sprintf(ra_smo_file,"%s%c%s", normfac_dir, DIR_SEPARATOR,basename);
       } else strcpy(ra_smo_file, delayed_file); 
       if ((ext=strrchr(ra_smo_file, '.')) != NULL) strcpy(ext,"_ra_smo.s");
       else strcat(ra_smo_file,"_ra_smo.s");

       LogMessage("computing  %s , scan duration %g\n", 
          ra_smo_file, scan_duration);

       // provide program path for LUT location
        gen_delays(0,NULL,1, scan_duration, NULL, dsinofp, ra_smo_file,
                    osem3dpar->span, osem3dpar->maxdel);
        // close ch file and open created smoothed random file
        fclose(dsinofp);
        dsinofp = file_open(ra_smo_file,"rb");
        if(dsinofp == FOPEN_ERROR) 
          crash2("  Error: cannot open input delayed sinogram file %s\n",ra_smo_file);
#endif     
      } else {
        if (st.st_size != (xr_pixels*views*segmentsize)*sizeof(float)) {
          if (max_g<0 && min_g<0)
          { // check file size
            LogMessage("%s: invalid file size %d, expected %d\n", delayed_file,
              st.st_size, (xr_pixels*views*segmentsize)*sizeof(float));
            exit(1);
          }
        }
      }
    }  else crash2("%s: Error getting file size\n", delayed_file);
  }
  
  /* Get Attenuation file format : mu-map or 3D atten*/
  if (noattenFlag == 0) {      
    if (stat(osem3dpar->atten_file, &st) == 0) {
       LogMessage("%s: attenuation file size %d\n",osem3dpar->atten_file,  st.st_size);
       LogMessage("expected file size %dx%dx%d(%d)\n", xr_pixels,views,segmentsize, segmentsize_s9);
      if (st.st_size == (xr_pixels*views*segmentsize)*sizeof(float)) 
        attenType = Atten3D;
      else if (st.st_size == (xr_pixels*views*segmentsize_s9)*sizeof(float)) 
        attenType = Atten3D_Span9;
      else if (st.st_size = (xr_pixels*xr_pixels*z_pixels)*sizeof(float)) 
          attenType = FullMuMap;
      else if (st.st_size = (xr_pixels*xr_pixels*z_pixels)*sizeof(float)/4)
        attenType = RebinnedMuMap;
      else 
      {
        LogMessage("%s: Invalid attenuation file size %d\n",osem3dpar->atten_file,  st.st_size);
        LogMessage("Supported attenuation inputs are: 3D attenuation, 128x128 or 256x256 mu-map\n");
        exit(1);
      }
    }
    else
    {
      LogMessage("%s: Error getting file size\n", osem3dpar->atten_file);
      exit(1);
    }
    LogMessage("%s: attenuation file type %d\n",osem3dpar->atten_file, attenType);
  }

  /* Start processing */
	LogMessage("  Reconstruction with %d subsets and %d iteration(s) \n", subsets, iterations );
	if(saveFlag) fprintf(stdout,"  ... an image will be saved after each iteration ! \n");

	time(&t1);
	t0 = t1;


	allocation_memory(pFlag, sFlag, weighting);

	/*-----------------------------------------------------------------------*/
	/* !sv special trick to reproject any 3D image into parallel projections */
	if ( (oFlag || o3Flag ) && ( iterations == 0 ) ) 
	{
		fprintf(stdout,"\n  Special case: NO iteration and output 2D or 3D sinogram.\n" );
		fprintf(stdout,"  ... proceeding with final forward projection(s).\n" );
		Output_CorrectedScan();
		exit(EXIT_SUCCESS);
	}

	if(blur){
		printf("fwhm1 %f %f %f\n",blur_fwhm1,blur_fwhm2,blur_ratio);
		psfx=(Psf1D *) malloc(sizeof(Psf1D));
		psfy=(Psf1D *) malloc(sizeof(Psf1D));
		psfz=(Psf1D *) malloc(sizeof(Psf1D));
		psfx->kernel = NULL;
		psfy->kernel = NULL;
		psfz->kernel = NULL;
		psfx->fwhm1 = psfy->fwhm1 = psfz->fwhm1 = blur_fwhm1;
		psfx->fwhm2 = psfy->fwhm2 = psfz->fwhm2 = blur_fwhm2;
		psfx->ratio = psfy->ratio = psfz->ratio = blur_ratio;
		psfx->model = psfy->model = psfz->model = model_info;
		psfx->type  = psfy->type  = psfz->type  = GaussShape;
		psfx->pixel_size = x_size;
		psfy->pixel_size = y_size;
		psfz->pixel_size = z_size;

		init_psf1D(blur_fwhm1,blur_fwhm2,blur_ratio,psfx);
		init_psf1D(blur_fwhm1,blur_fwhm2,blur_ratio,psfy);
		init_psf1D(blur_fwhm1,blur_fwhm2,blur_ratio,psfz);
	}

	if (calnormfacFlag) {
		PrepareNormfac();
		if(normfac_in_file_flag!=-1) alloc_normfac_infile(weighting,span);
		if(normfac_in_file_flag==-1) alloc_correction();
		CalculateNormfac();
	} else {
		if(normfac_in_file_flag!=-1)alloc_normfac_infile(weighting,span);
		if(normfac_in_file_flag==1) normfac_in_file_flag=2;
		LogMessage("  *** WARNING *** Reuse normfac (sensitivity) previously created ; Care should be taken that it contains the correct information \n");
	}

	LogMessage("  Osem3d %s reconstruction started\n", out_img_file);
  // reconstruct single or first frame
	PrepareOsem3dFiles();
  time(&t0);
  Init_Image();
	CalculateOsem3d(0);
  // Save image if not already done as intermediate image
	if (!saveFlag) SaveImage(out_img_file,iterations, 0);
	LogMessage("  Osem3d %s reconstruction done in %d sec\n", out_img_file, t1-t2);
  // clean ra_smo_file
#ifndef WIN_X64
  if (strlen(ra_smo_file) && access(ra_smo_file,0) == 0) unlink(ra_smo_file);
#endif

   // reconstruct remaining frames
  for (int em_frame=1; em_frame<num_em_frames; em_frame++) {
    sprintf(true_file,"%s_frame%d%s.s",em_file_prefix,em_frame,em_file_postfix); 
    if (!ecat_flag)
      sprintf(out_img_file,"%s_frame%d%s.i",em_file_prefix,em_frame,em_file_postfix);

    if ((tsinofp = file_open(true_file,"rb")) == FOPEN_ERROR) 
      crash2("  Error: cannot open input true sinogram file %s\n",true_file);
    
    if(nFlag) {  // reopen normalization          
      if ((normfp = file_open(norm_file,"rb")) == FOPEN_ERROR) 
        crash2("  Error: cannot open normalization file %s\n",norm_file);
    }

    if (ecat_flag) 
      LogMessage("  Osem3d %s,%d reconstruction started\n", out_img_file,em_frame+1);
    else LogMessage("  Osem3d %s reconstruction started\n", out_img_file);

    PrepareOsem3dFiles();
    time(&t0);
    Init_Image();
    CalculateOsem3d(em_frame);
    // Save image if not already done as intermediate image
    if (!saveFlag) SaveImage(out_img_file,iterations, em_frame);
    if (ecat_flag)
      LogMessage("  Osem3d %s,%d done in %d sec\n",out_img_file, em_frame+1, t1-t2);
    else LogMessage("  Osem3d %s  done in %d sec\n",out_img_file,t1-t2);
    // clean ra_smo_file
#ifndef WIN_X64
    if (strlen(ra_smo_file) && access(ra_smo_file,0) == 0) unlink(ra_smo_file);
#endif
  }

	return 0;
}   /* main()  */
