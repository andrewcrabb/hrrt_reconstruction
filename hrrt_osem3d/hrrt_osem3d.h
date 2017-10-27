/*	
 *	HRRT_osem3d.h
 */
/*	
 Modification history:
- 15-MAY-2008: Added file logging (MS)
- 11-SEP-2008: Added PSF flag
- 02-JUL-2009: Add dual logging to console and file
- 09-DEC-2009: Restore -X 128 option (MS)
*/

#ifndef         hrrt_osem3d_h
#define         hrrt_osem3d_h
#include "compile.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "scanner_model.h"
#ifdef IS_WIN32
	#include <windows.h>
	#include <process.h>
	#include <io.h>
	#include <xmmintrin.h>
	#include <emmintrin.h>
#else
	#include <pthread.h>
	#include <pmmintrin.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
	
#endif
#define   degree_to_rad .0174532888f
#define   PI 3.14159265358979323846f
#define	  MAXTHREADS 32
#define   MAXGROUP  100
/* 
 *	specific for ECAT 7 
 */ 
extern int    span;
extern int    maxdel;
extern int    groupmax;
extern int    groupmin;
extern int    defelements;
extern int    defangles;
extern float  fov;

/*
	multiframe version
*/

typedef struct {
	float ***image;
	float ***imagebuf;
	float ***prj;
	int view90,view, theta, verbose;
	int start, end;
	int pthread_id;
} BPFP_ptargs;

typedef struct {
	float zzz0;
	int zzi;
} Forwardprojection_lookup;	

typedef struct {
	int zsa;
	int zea;
	int yra;
	float yr2a;
} Backprojection_lookup;

typedef struct {
	short z;
	short yr;
	float coef;
} Backprojection_lookup_oblique;

typedef struct {
	int   num;
	short z1;
	short z2;
	short z3;
	short z4;
} Backprojection_lookup_oblique_start_end;

enum PsfShape {UnknownShape,GaussShape,ExpShape};
typedef struct //psf_1d
	{
        int         type        ; /* PSF shape */
		float           fwhm1       ; 
		float           fwhm2       ;
		float           ratio       ;
		int             kernel_size ; 
		float           pixel_size  ;
		float           *kernel     ;
		ScannerModel    *model      ;
	} Psf1D;

typedef struct {
	float ***ima;
	float ***out;
	float ***buf;
	int thr;
	int start;
	int end;
	Psf1D *psf;
} ConvolArg;

// Attenuation input file may be one of: 3D attenation, compressed mu-map(128x128)
// or full mu-map(256x256)
typedef  enum {Atten3D, Atten3D_Span9,RebinnedMuMap, FullMuMap} AttenInputType; 

extern int start_x__per_thread[16];

extern float **rescalearray;
extern float **tantheta;
extern int ZShift;
extern Backprojection_lookup **blookup;//[256][22];
extern Forwardprojection_lookup **flookup;
extern short int **cylwiny;
extern int start_x__per_thread[16];			// is it related to default subset 16?? CM
extern int start_x__per_thread_back[16];
extern int start_x__per_thread_back_8sysmetric[16];
/*
 *   MT version
 */
extern int nthreads;

/*
 *	scanner parameters; defined in osem3d_sbrt.c 
 */
extern int   rings;        /* number of crystal rings */
extern float ring_radius;  /* in mm */
extern float ring_spacing; /* in mm  */
extern float rfov;         /* radius of FOV in mm */

/*
 *	sinogram parameters ; defined in osem3d_sbrt.c 
 */
extern int   v_rebin;       /* view rebinning */
extern int   views;         /* number of angles: def_angles/v_rebin  */
extern int   radial_pixels; /* number of radial samples */
extern float sino_sampling; /* sinogram sampling; in mm */
extern int   r_off;


/*
 *	 parameters for OSEM ; defined in osem3d_sbrt.c 
 */
extern int sviews;          /* number of views in one subset */
extern int subsets;         /* number of subsets  */
extern int **vieworder;     /* size [subsets][views] */

/*
 *	parameters of 2D-projections ; defined in osem3d_sbrt.c 
 */
extern	int	x_rebin  ;	      /* radial rebinning */
extern	int	xr_pixels  ;			/* number of Xr pixels: def_elements/x_rebin */
extern	int	yr_pixels ;			/* measured Yr pixels; same as z_pixels	*/	
//extern	int	xr_off, yr_off;		/* indices where the origin should be */
extern	int	th_pixels ;			/* number of theta; including +&- ; 2*rings-1 */
extern	int	th_min ;			/* minimum value of theta */

/* 
 *	image parameters 
 */
extern	int	  x_pixels;     /* def_elements/x_rebin */
extern	int	  y_pixels;
extern	int	  z_pixels;
extern  int   z_pixels_simd;
extern	int	  imagesize;              	/* number of voxels */
//extern	int	  x_off, y_off, z_off;
extern	float x_size, y_size, z_size;

/* frequently used array and constants  */

//extern short int  **cyl_window;  /* FOV map; 1 if inside FOV, otherwise 0 ; [x][y] */
//extern short int  **cyl_window2; /* FFOV map; 0 if inside FOV, otherwise 1 ; [x][y] */
extern short int **cylwiny;

extern	int	sinosize;           /* number of pixels in one sinogram  */
extern	float 	*sin_theta;	    /* array length [th_pixels]; sin(theta)  */
extern	float 	*cos_theta;	    /* array length [th_pixels]; cos(theta)  */ 
extern	int	*yr_bottom;         /* array length [th_pixels];   */
extern	int	*yr_top;            /* array length [th_pixels];   */
extern	int	*seg_offset;        /* array length [th_pixels];  by pixel... */
extern  int   	*segment_offset;    /* array length [th_pixels];  by plane... */
extern  int    	segmentsize;        /* maybe 2209 (span9) */
extern  int   	*view_offset;       /* array length [views]; used by largeprj array(<- [view][segmentsize][xr_sizes] */
extern	float 	*sin_psi;           /* array length [views]; sin(view)  */
extern 	float 	*cos_psi;           /* array length [views]; cos(view)  */
extern	int	subsetsize;         /* number of pixels in one subset  */
extern  int     z_min, z_max;       /* z_min and z_max */
extern  int     npskip;             /* number of skipped end segment planes */
extern float zoom;
extern float newzoom;
extern int   max_g;              /* max # of groups. */
extern float rel_fov;            /* relative fov : 0.95 (95%)  */

/* benchmarking stuff sv */
//extern float fForwardProjSecs;
//extern float fBackProjSecs;

#ifdef IS_WIN32
#else
//	#define max(a,b) ((a)>(b)?(a):(b))
//	#define min(a,b) ((a)<(b)?(a):(b))
#endif

/* prototype of routines */
//#if defined(__STDC__)
#if 0
	void free_dependencies();
	extern	int dependencies(int nprojs, int nviews, int verbose);
	extern	int forward_proj3d( float ***ima, float ***prj, int sub, int theta, int verbose);
	extern	int back_proj3d( float ***prj, float ***ima, int sub, int theta, int verbose);
	int crash1(char *fmt);
	int crash2(char *fmt, char *a0);
	int crash3(char *fmt, int a0);
	int forward_proj3d2(float ***ima,float ** prj,int view,int verbose,float ***imagebuf,float **prjbuf);	
	int back_proj3d2   (float ** prj,float *** ima,int view,int verbose,float ***imagebuf,float **prjbuf);
	// following methods will be deprecated.
	int forward_proj3d(float ***ima,float *** prj,int sub,int view,int verbose);	
	int back_proj3d(float ***prj,float *** ima,int sub,int view,int verbose);
#else
	void free_dependencies();
	int dependencies(int nprojs,int  nviews,int  verbose);		
	int crash1(const char *fmt);
	int crash2(const char *fmt, char *a0);
	int crash3(const char *fmt, int a0);
	int forward_proj3d2(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf);	
	int forward_proj3d_thread1(float ***ima,float ** prj,int view,int numthread,float ***imagebuf,float ***prjbuf);	
	int back_proj3d2(float **prj,float ***ima,int view,int numthread,float ***imagebuf,float ***prjbuf);
	int back_proj3d_thread1(float ** prj,float *** ima,int view,int numthread,float ***imagebuf,float ***prjbuf);
	short	**matrixshort( int nrl, int nrh, int ncl, int nch  );
	float	***matrixfloatptr(int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
	//int forward_proj3d2(float ***ima,float ** prj,int sub,int view,int verbose);
	//int back_proj3d2(float **prj,float *** ima,int sub,int view,int verbose);

	// following methods will be deprecated.
	int forward_proj3d(float ***ima,float *** prj,int sub,int view,int verbose);	
	int back_proj3d(float ***prj,float *** ima,int sub,int view,int verbose);
	int rotate_image_buf(float ****imagebuf,float ***ima,int numthread,int *view);
	void init_psf1D(float blur_fwhm1,float blur_fwhm2,float blur_ratio,Psf1D *psf);
	void convolve3d(float ***image,float ***out,float ***buf);
	void mulprojectionscalar(float ***prj1,float coef,int theta);
  int proj_atten_view(float ***image, float *prj,int view, int verbose=0);
  void LogMessage( const char *fmt, ... );
	//int write_image_header(struct ImageHeaderInfo *info, int psf_flag);
#endif  /* __STDC__ */



typedef struct {
	float **prj;
	float ***ima;
	float ***imagepack;
	float ***imagebuf;
	float ***prjbuf;
	float **volumeprj;
	short **volumeprjshort;
	float **volumeprjfloat;
	int weighting;
	int view;
	int numthread;
} FPBP_thread;


typedef struct {
	float ***prj;
	float ***normfac;   /* img type array. normfac  */ 
	char  *normfac_img; /* normfac file name string */
	int   isubset;
	int   verbose;
	int   view;
	//FILE  *fp;
	int   fp;
} File_structure;

typedef struct {
	int v;
	int view;
	int ths;
	int the;
} Theta_start_end;

#ifdef IS_WIN32
extern File_structure fstruct[16];
#define THREAD_TYPE HANDLE
extern HANDLE threads[16];
#define FUNCPTR unsigned __stdcall 
#define START_THREAD(th,func,arg,id) th = (HANDLE)_beginthreadex( NULL, 0, &func, &arg, 0, &id )
#define Wait_thread(threads) {WaitForSingleObject(threads, INFINITE );CloseHandle(threads);}

#else
// !sv File_structure fstruct[16];
#define THREAD_TYPE pthread_t
extern pthread_t threads[16];
#define FUNCPTR void*
#define START_THREAD(th,func,arg,id) pthread_create(&th,NULL,&func, &arg)
#define Wait_thread(threads) pthread_join(threads,NULL)
#endif

#define LOG_TO_CONSOLE 0x1
#define LOG_TO_FILE 0x2

extern short **norm_zero_mask;
extern bool CompressFlag;
extern unsigned char ***compressmask;
extern float ***compress_prj;
extern short **compress_num;
extern int *segment_offset;

extern Psf1D *psfx;
extern Psf1D *psfy;
extern Psf1D *psfz;
extern char log_file[];
extern unsigned int log_mode;

#endif	/* HRRT_osem3d_h  */

