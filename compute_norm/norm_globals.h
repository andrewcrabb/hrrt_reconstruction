#ifndef Norm_Globals_h
#define Norm_Globals_h

extern const char *model_name;
extern int nelems;
extern int nviews;
extern int nrings;
extern int span;
extern int rd;
extern int nseg;

extern int bkts_per_ring;
extern int blks_per_bkt;
extern float det_dia; 				// Bill Jones uses 469 in the rebinner code ?
extern float blk_pitch; 
extern float crys_pitch;			// use tomographic pitch used for rebinning
extern float crys_depth[];		        // HRRT01 uses 7.5 mm Xtals 
extern float weights[];	    	// LSO-LSO 

extern float axial_pitch;     //use average axial pitch 19.5/8 
extern float dist_to_bkt;		//should be the geometric radius, since DOI is added
extern int radial_comp;					//compression factors for dwell fit
extern int angle_comp;
extern int axial_comp;						//in z.
extern int crys_per_blk;
extern int crys_per_bkt;					//blks_per_bkt*crys_per_blk
extern int crys_per_ring;				// crys_per_bkt*bkts_per_ring
extern int fan_size;		
extern int qc_flag;

struct SinoIndex
{
	int npairs;
	int idx[1024];
	float wts[1024];
};
extern int make_norm(const float *sino, float *norm, const float *dwell,
					   SinoIndex *sino_index, float *plane_eff);
extern void sino_to_crys(SinoIndex *sino_index);
extern int self_norm(float *vsino, int nplns);
extern void plot(const char *basename, const float *y, int size, 
				 const char *title=0);
extern void plot(const char *basename,  const float *y, int count, int size,
				 const char *title=0);
extern void plot(const char *basename,  const float *y, const float *y_fit,int count, int size,
				 const char *title=0);
extern void plot(const char *basename, const float *x, const float *y, int size,
				 const char *title=0);
float *fit_dwell(short *vsino, int nplns);
int dwell_sino(float *dwell, int npixels);


#endif

