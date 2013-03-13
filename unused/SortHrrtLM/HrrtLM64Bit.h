/* $Id: HrrtLM64Bit.h,v 1.3 2009/09/30 17:12:40 peter Exp $ */

/*
$Log: HrrtLM64Bit.h,v $
Revision 1.3  2009/09/30 17:12:40  peter
Added references for routines DeleteBlkLM and Copy_LM_Hdr

Revision 1.2  2009/03/16 17:35:08  peter
Added definitions for the generation of prompt and delated coincidence maps

Revision 1.1  2005/09/19 13:54:06  peter
Initial revision

*/

	/*	Global definitions			*/
#define		MAX_STRING_LENGTH	1024
#define		NUMEVENTSTOREAD		1024*1024*2

#define		MAXSEGNUM       60
#define		NUM_LAYERS	2					/* # crystal layers */
#define		NUM_CRYSTALS	8					/* # crystals/detector in radial and axial */
#define		RAD_DETECTORS   9                                       /* # detectors - radial */
#define		AX_DETECTORS    13                                      /* # detectors - axial */
#define		NHEADS          8                                       /* number of heads */
                /* HRRT with 10 mm. layers of both LSO & LYSO */
#define FXD	( ( double )1.0 )               /* first (FOV side) crystal depth - cm */
#define SXD	( ( double )1.0 )               /* second (PMT side) crystal depth - cm */
#define HEADY	104                                     /* head axial crystal span */
#define SINOW	256                                     /* radial width of 2-D sinogram */
#define SINOH	288                                     /* angular height of 2-D sinogram */

	/* CPS specific declarations */
		/* Function, variable and global definitions */
#ifndef _LUT_
extern int init_sort3d_hrrt(int span, int maxrd, int nv, int np, float diam, float crys_depth);
extern int sort3d_event_addr(int mp, int alayer, int ax, int ay, int blayer, int bx, int by);
extern int nmpairs, nheads, nxcrys, nycrys, nlayers;
extern float *head_crystal_depth;
extern double binsize, plane_sep;
void calc_det_to_phy( int head, int layer, int detx, int dety, float location[3]);
void phy_to_pro( float deta[3], float detb[3], float projection[4]);
void Init_Geometry_HRRT ( int np, int nv, float cpitch, float diam, float thick);
void Init_Seginfo( int nrings,  int span, int maxrd) ;
static int hrrt_mpairs[][2]={{-1,-1},{0,2},{0,3},{0,4},{0,5},{0,6},
                               {1,3},{1,4},{1,5},{1,6},{1,7},
                                     {2,4},{2,5},{2,6},{2,7},
                                           {3,5},{3,6},{3,7},
                                                 {4,6},{4,7},
                                                       {5,7}};
int nmpairs;
#endif
inline void crash(char *args) { fprintf(stderr,args); exit(1);}
inline void crash1(char *args, int i) { fprintf(stderr,args, i); exit(1);}
/*
external values:
    sin_head[NHEADS] contains sin of head position angle
    cos_head[NHEADS] contains cos of head position angle
    crystal_x_pitch is distance between crystal columns
    crystal_y_pitch is distance between crystal rows
    x_head_center is distance from edge of head to center
    crystal_radius is distance from center to crystal
    layer_thickness is thickness of each layer
*/
static int seg_plane_map[135*207];
double *sin_head;
double *cos_head;
double crystal_x_pitch;
double crystal_y_pitch;
double x_head_center;
double crystal_radius;
int nheads;
int nlayers;
int nxcrys;
int nycrys;
int (*mpairs)[2];
int nprojs, nviews;
int nplanes;
double binsize;
double d_tan_theta;
double plane_sep;
static int maxseg, nsegs, span1_flag=0;
static int *segz0, *segnz, *segzoff;
static int *span9_segz0=NULL, *span9_segnz=NULL, *span9_segzoff=NULL;
static double span9_d_tan_theta=0;
float *crystal_xpos=NULL;
float *crystal_ypos=NULL;
float *crystal_zpos=NULL;
float *Head_Crystal_Depth=NULL;

#define CSIZE 0.22
#define CGAP 0.02
#define BSIZE (8*CSIZE+7*CGAP)
#define BGAP 0.05
#define XHSIZE (9*BSIZE+8*BGAP)

extern int pro_to_addr(float[]);

/*
	Default structure for CPS Interfile Information
		Enable updating of scan and patient information in
		V3.3 of interfile --> STIR reconstructions
*/
struct InterFileInfo {

	char	StudyDate[ MAX_STRING_LENGTH ] ,
		StudyTime[ MAX_STRING_LENGTH ] ;

	int	AxialCompression ,
		MaximumRingDifference ,
		ULD ,
		LLD ,
		StudyDuration ;

	char	PatientName[ MAX_STRING_LENGTH ] ,
		PatientDOB[ MAX_STRING_LENGTH ] ,
		PatientID[ MAX_STRING_LENGTH ] ,
		PatientSex[ MAX_STRING_LENGTH ] ,
		PatientDexterity[ MAX_STRING_LENGTH ] ,
		PatientHeight[ MAX_STRING_LENGTH ] ,
		PatientWeight[ MAX_STRING_LENGTH ] ,
		Injectate[ MAX_STRING_LENGTH ] ,
		Activity[ MAX_STRING_LENGTH ] ;

} InterFileInfo ;

/*
	Default structure for Frame Information
*/
struct FrameInfo {

	char	OriginalFile[ MAX_STRING_LENGTH ] ;

	int	FrameNum ,
		TotalNumFrame ;

	long	Prompt ,
		Delayed ;

	float	FrameStartTime ,
		FrameEndTime ;

	int	NumTimeTags ;

	int	NumBin ,
		NumView ,
		NumSino ,
		Span ,
		RingDiff ;
		
	long	nElements ;

} FrameInfo ;


	/* Timing Information */
struct AcqTime {
	long	TagTime ,
		ms ;
} AcqTime ;

	/* Singles Information */
struct SglInfo {
	float	Sgls[936][3] , /* [*][0] = Total Singles [*][1] = # Polled [*][2] = Average */
		TotSgls[ 3 ] ; /* [0] = Total Singles [1] = # Polled [2] = Average */
} SglInfo ;
	
	/* Motion Information */
struct MotionInfo {
	float	Q0 ,
		Qx ,
		Qy ,
		Qz ,
		Tx ,
		Ty ,
		Tz ,
		Err ;
} MotionInfo ;

/*
	Default structure for HRRT 64-bit list mode
*/
struct EventInfo {
	unsigned int	Event1 ,
			Event2 ;
		int		AX ,
				BX ,
				AY ,
				BY ,
				XE ,
				AE ,
				BE ,
				AI ,
				BI ,
				TF ,
				Res1 ,
				Res2 ,
				Event ,
				PS0 ,
				PS1 ,
				HeadA ,
				HeadB ,
				SinogramAddress ,
				Status ;
}  ;

struct ListMode_64bit {
		int			Tag_64 ;
		int			LM_Type ;	/* 1 - Event, 2 - Time, 4 - Singles , 8 - Motion */
		struct EventInfo	EventInfo ;
		struct AcqTime		AcqTime ;
		struct SglInfo		SglInfo ;
		struct MotionInfo	MotionInfo ;
	};

struct LM_64bit {
		/* Event Information */
  struct EventInfo EventInfo;
  struct ListMode_64bit lm_64bit ;
} ;

/*		Type Definitions		*/
typedef unsigned char BYTE ;
typedef signed char SBYTE ;



/*	Routine declaration	*/
int UpDateLM() ;
int UpDateLM32bit() ;
int DeleteBlkLM() ;
int ListModeAcqTo() ;
int ListMode32BitAcqTo() ;
int ExtractInterFileInfo() ;
int SinoInfo() ;
int WriteDataToDisk() ;
int WriteCoinMapToDisk() ;
float GetDTC() ;
int Copy_LM_Hdr() ;

#ifdef _LUT_
	#include "gen_delays_lib/lor_sinogram_map.h"
	#include "gen_delays_lib/segment_info.h"
	#include "gen_delays_lib/geometry_info.h"
	#include "gen_delays_lib/gen_delays.h"
	int Init_Rebinner(int, int);
	int em_span=9;
	int tx_span=21;
  extern int rebin_event( int mp, int alayer, int ax, int ay, int blayer, int bx, 
    int by);
  extern int rebin_event_tx( int mp, int alayer, int ax, int ay, int blayer, int bx, 
    int by);
#endif