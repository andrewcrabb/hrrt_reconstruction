/* Author : Merence Sibomana <Sibomana@topo.ucl.ac.be> */
/* created: 25-apr-1996 */
/*
 * int ecat2air
 *
 * Returns:
 *  1 if save was successful
 *  0 if save was unsuccessful
 *
 */
/* Modification history: Merence Sibomana <sibomana@gmail.com> 
   11-DEC-2009: Port to 5.2 and replace XDR by XML 
   10-FEB-2010: Added function ecat_AIR_load_probr

*/

#include "ecat2air.h"
#include <unistd.h>
#include <ecatx/ecat_matrix.hpp>
#include <ecatx/load_volume.h>
#include <math.h>

typedef struct _MatrixExtrema {
	char *specs;
	int data_type;
	float data_min;
	float data_max;
	struct _MatrixExtrema *next;
} MatrixExtrema;

static MatrixExtrema *matrix_extrema=NULL;


static void flip_x(void * line, int data_type, int xdim)
{
	static void * _line=NULL;
	static int line_size = 0;
	int x=0;
	int elem_size = (data_type==ByteData)? 1 : 2;

  if (data_type == IeeeFloat) elem_size = 4;
	if (line_size == 0) {
		line_size = xdim*elem_size;
		_line = (void *)malloc(line_size);
	} else if (xdim*elem_size > line_size) {
		line_size = xdim*elem_size;
		_line = (void *)realloc(_line, line_size);
	}
	switch(data_type) {
		case ByteData :
		{
			unsigned char  *b_p0, *b_p1;
			b_p0 = (unsigned char *)line;
			b_p1 = (unsigned char *)_line + xdim - 1;
			for (x=0; x<xdim; x++) *b_p1-- = *b_p0++;
			memcpy(line,_line,xdim);
			break;
		} 
		default :
		{
			short *s_p0, *s_p1;
			s_p0 = (short*)line;
			s_p1 = (short*)(_line + (xdim-1)*elem_size);
			for (x=0; x<xdim; x++) *s_p1-- = *s_p0++;
			memcpy(line,_line,xdim*sizeof(short));
			break;
		}
	}
}

static void flip_y(void * plane, int data_type, int xdim, int ydim)
{
	static void * _plane=NULL;
	static int plane_size = 0;
	void *p0, *p1;
	int elem_size = (data_type==ByteData)? 1 : 2;
	int y=0;

  if (data_type == IeeeFloat) elem_size = 4;
	if (plane_size == 0) {
		plane_size = xdim*ydim*elem_size;
		_plane = (void *)malloc(plane_size);
	} else if (xdim*ydim*elem_size > plane_size) {
		plane_size = xdim*ydim*elem_size;
		_plane = (void *)realloc(_plane, plane_size);
	}
	p0 = plane;
	p1 = _plane + (ydim-1)*xdim*elem_size;
	for (y=0; y<ydim; y++) {
		memcpy(p1,p0,xdim*elem_size);
		p0 += xdim*elem_size;
		p1 -= xdim*elem_size;
	}
	memcpy(plane,_plane,xdim*ydim*elem_size);
}

void  matrix_flip(MatrixData *data, int x_flip, int y_flip, int z_flip)
{
  int y=0,z=0;
  void *plane_in=NULL, *plane_out=NULL, *tmp=NULL, *line=NULL; 
  int elem_size = (data->data_type==ByteData)? 1 : 2;
  int npixels;

  if (data->data_type == IeeeFloat) elem_size = 4;
	npixels = data->xdim * data->ydim;
	plane_in = data->data_ptr;
	if(z_flip){
	  plane_out = data->data_ptr + (data->zdim-1)*elem_size*npixels;
	  tmp = (void *) malloc(npixels*elem_size);
	  while (plane_in<plane_out) {
		memcpy(tmp,plane_out,npixels*elem_size);
		memcpy(plane_out,plane_in,npixels*elem_size);
		memcpy(plane_in,tmp,npixels*elem_size);
		plane_in += npixels*elem_size;
		plane_out -= npixels*elem_size;
	  }
	  free(tmp);
	}
	plane_in = data->data_ptr;
	for (z = 0; z < data->zdim; z++) {
		if (y_flip) flip_y(plane_in,data->data_type,data->xdim,data->ydim);
		if( x_flip){
		  for (y = 0; y < data->ydim; y++) {
			line = plane_in + y*data->xdim*elem_size;
			flip_x(line,data->data_type,data->xdim);
		  }
		}
		plane_in += npixels*elem_size;
	}
}

static AIR_Pixels ***matrix2air(MatrixData* matrix, struct AIR_Key_info *stats)
{
	int i, nvoxels;
	unsigned char  *bdata=NULL;
	short *sdata=NULL;
  float *fdata=NULL;
	float a,b,f,v, v0,v1;
	int binaryok=stats->bits;
	AIR_Pixels ***image, *image_data;

	if (sizeof(AIR_Pixels) != 1 && sizeof(AIR_Pixels) != 2) {
		printf("ecat2air not configured for %i bits/pixel\n",
			8*sizeof(AIR_Pixels));
		return 0;
	}
	switch (matrix->data_type) {
		case BitData : stats->bits = 1; break;
		case ByteData : stats->bits = 8; break;
		default :	stats->bits = 16; 	/* assume 16 */
	}
	stats->x_dim = matrix->xdim;
	stats->y_dim = matrix->ydim;
	stats->z_dim = matrix->zdim;
	stats->x_size = matrix->pixel_size*10.;	/* in mm*/
	stats->y_size = matrix->y_size*10.;
	stats->z_size = matrix->z_size*10.;
	nvoxels = matrix->xdim*matrix->ydim*matrix->zdim;

/*
 * Deal with binary images by converting if allowed and refusing if not allowed
*/
	if(stats->bits==1){
		if(!binaryok){ 
			printf("ecat matrix is binary and was therefore not converted\n");
			return 0;
		}
	}
	
#ifdef OLD_AIR
	/*Issue warning if voxel size is out of predefined range*/
	if(stats->x_size>PIXEL_MAX_SIZE||stats->x_size<PIXEL_MIN_SIZE||
		stats->y_size>PIXEL_MAX_SIZE||stats->y_size<PIXEL_MIN_SIZE||
		stats->z_size>PIXEL_MAX_SIZE||stats->z_size<PIXEL_MIN_SIZE) {
		printf("WARNING: matrix voxel dimensions %.4f x %.4f x %.4f\n",
			stats->x_size,stats->y_size,stats->z_size);
	}
#endif
  /*Allocate memory for the image*/
	image = AIR_create_vol3(stats->x_dim,stats->y_dim,stats->z_dim);
	if (image==NULL) {
		printf("ecat2air : unable to allocate memory for %dx%dx%d voxels\n",
			stats->x_dim,stats->y_dim,stats->z_dim);
		return 0;
	}

	/*fill the image*/
	image_data = image[0][0];

	v0 = matrix->data_min/matrix->scale_factor;
	v1 = matrix->data_max/matrix->scale_factor;
/* change scale : f(v) = a*v+b where f(v0) = 0 and f(v1) = MAX_POSS_VALUE
   a = (f(v1) - f(v0))/(v1-v0) 
   b = f(v0) - a*v0
*/
	a = (float)(AIR_CONFIG_MAX_POSS_VALUE)/(v1-v0);
	b = -a*v0;
	
	switch (matrix->data_type) {
		case BitData:
		case ByteData:
			bdata = (unsigned char *)matrix->data_ptr;
			for (i=0; i<nvoxels; i++) {
				v = *bdata++;
				f = a*v+b;
				*image_data++ = (int)(f+0.5);
			}
			break;
    case IeeeFloat:
			fdata = (float*)matrix->data_ptr;  
      // compute foat to short scale factor
      if (fabs(matrix->data_max) > fabs(matrix->data_min)) {
        f = (float)(fabs(matrix->data_max)/32767);
      } else if (fabs(matrix->data_min)>0) {
        f = (float)(fabs(matrix->data_min)/32767);
      } else f = 1.0f;  

      f = 1.0f/f; // inverse scale factor
			for (i=0; i<nvoxels; i++) {
				image_data[i] = (AIR_Pixels)(32767 + fdata[i] * f);
			}
			break;
		default :
			sdata = (short*)matrix->data_ptr;
			if (sizeof(AIR_Pixels) == 2) {
				for (i=0; i<nvoxels; i++) {
					v = (float)(32768 + sdata[i]);
					image_data[i] = (AIR_Pixels)(v);
				}
			} else {
				for (i=0; i<nvoxels; i++) {
					v = sdata[i];
					f = a*v+b;
					image_data[i] = (int)(f+0.5);
				}
			}
	}
	return image;	
}

static int matnum;
static char fname[256];
int matrix_exists(const char *specs)
{
    int ret = 0;
    ecat_matrix::MatrixFile *file;
    MatDirNode *node;

    matnum = 0;
    ret = matspec(specs,fname,&matnum);
	if (access(fname,F_OK) < 0) { /* try adding .img extension */
		fprintf(stderr,"%s not found\n",fname);
		strcat(fname,".img");
		if (access(fname,F_OK) < 0) {
			fprintf(stderr,"%s not found\n",fname);
			return 0;
		} else fprintf(stderr,"using %s\n",fname);
	}
		
    file = matrix_open(fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
    if (file == NULL) {
        matrix_perror(fname);
        return 0;
    }
    if (ret==0)  /* no matrix specified, use first */
        if (file->dirlist->nmats) matnum = file->dirlist->first->matnum;
    for (node = file->dirlist->first; node!=NULL; node=node->next)
        if (node->matnum == matnum) break;
    ret = node!=NULL? 1:0;
    matrix_close(file);
    return ret;
}


AIR_Pixels ***ecat2air(const char *specs, struct AIR_Key_info *stats,
                       const AIR_Boolean binary_ok, AIR_Error *errcode)
{
  ecat_matrix::MatrixFile* file;
  MatrixData *volume;
  ecat_matrix::MatVal mat;
  AIR_Pixels ***pixels = NULL;
  int cubic=0,interpolate=0;
  MatrixExtrema *extrema;
  
  *errcode = 0;
  /* Fills in fname */
  if (!matrix_exists(specs))
    return NULL;
  file = matrix_open(fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  mat_numdoc(matnum, &mat);

  // printf("air_ecat/ecat2air.c: fname:%s, matnum:%d \n", fname, matnum); fflush(stdout);
  if ( (volume = matrix_read(file,matnum, GENERIC)) != NULL) {
    //if ( (volume = load_volume(file,mat.frame, /*cubic*/1, /*interp*/1)) != NULL) {
    if (matrix_extrema == NULL) {
      matrix_extrema = (MatrixExtrema*)calloc(1,sizeof(MatrixExtrema));
      extrema = matrix_extrema;
    } else {
      extrema = matrix_extrema;
      while (extrema->next) extrema = extrema->next;
      extrema->next = (MatrixExtrema*)calloc(1,sizeof(MatrixExtrema));
      extrema = extrema->next;
    }
    extrema->specs = strdup(specs);
    extrema->data_min = volume->data_min/volume->scale_factor;
    extrema->data_max = volume->data_max/volume->scale_factor;
    extrema->data_type = volume->data_type;
    matrix_flip(volume,0,1,1); 		/* radiology convention */
    pixels = matrix2air(volume,stats);
    free_matrix_data(volume);
  } else {
    printf("air_ecat/ecat2air.c: volume %s,%d,*,*,*,* not found\n",fname,mat.frame);
    /* ahc addition 3/4/13 */
    *errcode = AIR_READ_IMAGE_FILE_ERROR;
  }

  matrix_close(file);
  return pixels;
}

/*
int ecat_AIR_map_value(const char *specs, int value, int *error)
{
  float a,f,v0,v1;

  MatrixExtrema *extrema = matrix_extrema;
  *error = 0;
	while (extrema && strcmp(extrema->specs,specs)!=0)
		extrema = extrema->next;
	if (extrema) {
		if (extrema->data_type != ByteData)
			return (int)(value+32767);
		v0 = extrema->data_min;
		v1 = extrema->data_max;
    	a = (float)(AIR_CONFIG_MAX_POSS_VALUE)/(v1-v0);
    	f = a*v0 + 2*value;
    	return (int)(f+0.5);
	}
  *error = 1;
	return value;
}
*/
