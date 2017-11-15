#include <stdio.h>
#ifndef FILENAME_MAX /* SunOs 4.1.3 */
#define FILENAME_MAX 256
#endif
#include "load_volume.h"
#define MIN_SINGLE_RATE 100.0f
#define MAX_SINGLE_RATE 100000.0f

static char line1[256], line2[256], line3[256], line4[256];
matrix_info(mh, matrix)
Main_header *mh;
MatrixData *matrix;
{
  int x,y,z,i, nvoxels;
  char *units;
  struct Matval mat;
  unsigned char  *b_data;
  short *s_data;
  float *f_data;
  double total=0, mean=0;
  int data_unit = mh->calibration_units_label;
  float ecf = mh->calibration_factor;
  double mx=0, my=0,mz=0;
	Image_subheader *imh=NULL;
  
  mat_numdoc(matrix->matnum, &mat);
  printf("\n\n *** Matrix := %d,%d,%d,%d,%d\n",
	 mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
  nvoxels = matrix->xdim*matrix->ydim*matrix->zdim ;
  switch(matrix->data_type) {
  case ByteData:
    b_data = (unsigned char *)matrix->data_ptr;
    for (i=0, z=1; z<=matrix->zdim; z++)  {
      for (y=1; y<=matrix->ydim; y++) {
        for (x=1; x<=matrix->xdim; x++, i++) {
          total += b_data[i];
          mx += x*b_data[i];
          my += y*b_data[i];
          mz += z*b_data[i];
        }
      }
    }
    break;
  case IeeeFloat :
    f_data = (float*)matrix->data_ptr;
    for (i=0; i<nvoxels; i++) total += *f_data++;
    for (i=0, z=1; z<=matrix->zdim; z++)  {
      for (y=1; y<=matrix->ydim; y++) {
        for (x=1; x<=matrix->xdim; x++, i++) {
          total += f_data[i];
          mx += x*f_data[i];
          my += y*f_data[i];
          mz += z*f_data[i];
        }
      }
    }
   break;
  default:		/* short */
    s_data = (short*)matrix->data_ptr;
    for (i=0, z=1; z<=matrix->zdim; z++)  {
      for (y=1; y<=matrix->ydim; y++) {
        for (x=1; x<=matrix->xdim; x++, i++) {
          total += s_data[i];
          mx += x*s_data[i];
          my += y*s_data[i];
          mz += z*s_data[i];
        }
      }
    }
  }
  mx = (mx/total)*matrix->pixel_size*10;
  my = (my/total)*matrix->y_size*10;
  mz = (mz/total)*matrix->z_size*10;
  printf("Dimensions := %dx%dx%d\n",matrix->xdim,matrix->ydim,matrix->zdim);
  if (mh->calibration_units == Uncalibrated || ecf <= 1.0 ||
      data_unit>numDisplayUnits)  data_unit = 0;
  if (data_unit) units = customDisplayUnits[data_unit];
  else units = "";
  total *= matrix->scale_factor;
  mean = total/nvoxels;
  sprintf(line1, "Minimum := %g %s",matrix->data_min, units);
  sprintf(line2, "Maximum := %g %s", matrix->data_max, units);
  sprintf(line3, "Mean    := %g %s", mean, units);
  sprintf(line4, "Total   := %g %s", total, units);
  if (data_unit == 1) { /* convert Ecat counts to Bq/ml */
    units = customDisplayUnits[2];
    sprintf(line1+strlen(line1), ", %g Bq/ml", ecf*matrix->data_min);
    sprintf(line2+strlen(line2), ", %g Bq/ml", ecf*matrix->data_max);
    sprintf(line3+strlen(line3), ", %g Bq/ml", ecf*mean);
    sprintf(line4+strlen(line4), ", %g Bq/ml", ecf*total,units);
  }
  printf("%s\n%s\n%s\n%s\n",line1, line2, line3, line4);
  printf("center of mass (x,y,z) mm := %g,%g,%g\n",mx-1,my-1,mz-1);
	if (mh->sw_version>72 && mh->file_type == PetVolume) 
  { // print data rates
		imh = (Image_subheader*)matrix->shptr;
		if (imh->singles_rate>MIN_SINGLE_RATE && imh->singles_rate < MAX_SINGLE_RATE)
		{ // valid Extended ECAT file
			int frame_duration_sec;
			frame_duration_sec = imh->frame_duration/1000;
			printf("Image duration        := %d\n", frame_duration_sec);
			printf("Total prompts        := %I64d\n",(__int64)((double)imh->prompt_rate*frame_duration_sec));
			printf("Total randoms        := %I64d\n",(__int64)((double)imh->random_rate*frame_duration_sec));
			printf("Average singles rate := %g\n",imh->singles_rate);
			if (imh->scatter_fraction>0.0f)
				printf("scatter fraction     := %g\n",imh->scatter_fraction);
		}
	}
}

main(argc, argv)
     int argc;
     char **argv;
{
  MatDirNode *node;
  MatrixFile *mptr;
  MatrixData *matrix;
  struct Matval mat;
  char fname[FILENAME_MAX];
  int ftype, frame = -1, matnum=0, cubic=0, interpolate=0;
  
  if (argc < 2) {
    printf("%s: Build %s %s\n", argv[0], __DATE__, __TIME__);
    crash("usage : %s matspec\n",argv[0]);
  }
  matspec( argv[1], fname, &matnum);
  mptr = matrix_open(fname, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
  if (mptr == NULL) {
    matrix_perror(fname);
    return 0;
  }
  ftype = mptr->mhptr->file_type;
  if (ftype <0 || ftype >= NumDataSetTypes)
    crash("%s : unkown file type\n",fname);
  printf( "%s file type  : %s\n", fname, datasettype[ftype]);
  if (!mptr) matrix_perror(fname);
  if (matnum) {
		mat_numdoc(matnum, &mat);
    matrix = matrix_read(mptr,matnum, UnknownMatDataType);
    if (!matrix) crash("%d,%d,%d,%d,%d not found\n",
		       mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
    matrix_info(mptr->mhptr,matrix);
  } 
	else {
    node = mptr->dirlist->first;
    while (node)
		{
			mat_numdoc(node->matnum, &mat);
			if (ftype == PetImage) {
				if (frame != mat.frame) {
					frame = mat.frame;
					matrix = load_volume(mptr,frame,cubic,interpolate);
					matrix_info(mptr->mhptr,matrix);
				}
			} else {
				matrix = matrix_read(mptr,node->matnum,UnknownMatDataType);
				if (!matrix) crash("%d,%d,%d,%d,%d not found\n",
					mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
				matrix_info(mptr->mhptr,matrix);
			}
			node = node->next;
		}
  }
  matrix_close(mptr);
  return 1;
}