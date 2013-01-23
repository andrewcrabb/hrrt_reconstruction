/*	
 file_io_processor.h
 Modification history:
	08-May-08: Added default value for scatter or atten when not available (MS)
  12-May-09: Use same code for linux and WIN32
*/
#ifndef file_io_processor_h
#define file_io_processor_h
#include "compile.h"
/* for all sinogram file except for true & prompt */
#define FILEPTR FILE*
int read_flat_float_proj_ori_theta(FILEPTR fp, float *prj,int theta, float val=0.0f);
int read_atten_image(FILEPTR fp, float ***image, unsigned input_type);

/* for true and prompt sinogram */
int read_flat_short_proj_ori_to_float_array_theta(FILEPTR fp, float *prj,int theta);
int read_flat_short_proj_ori_to_short_array_theta(FILEPTR fp, short *prj,int theta);


int write_flat_image(float ***image,char * filename,int index,int verbose );
int read_flat_image (float ***image,char * filename,int index,int verbose);


int    write_norm(float ***norm,char *filename,int isubset,int verbose );
int    read_norm(float ***norm,char * filename,int isubset,int verbose);

/* 2006.02.14 : dsaint31 : not modified yet.... */
int write_flat_3dscan(float ***scan,char *filename,int theta,int verbose);

#endif

