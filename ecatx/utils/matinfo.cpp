#include <stdio.h>
#include "my_spdlog.hpp"

#ifndef FILENAME_MAX /* SunOs 4.1.3 */
#define FILENAME_MAX 256
#endif
#include "load_volume.h"
#define MIN_SINGLE_RATE 100.0f
#define MAX_SINGLE_RATE 100000.0f

static char line1[256], line2[256], line3[256], line4[256];
matrix_info(mh, matrix)
ecat_matrix::Main_header *mh;
ecat_matrix::MatrixData *matrix;
{
  int x,y,z,i, nvoxels;
  char *units;
  ecat_matrix::MatVal mat;
  unsigned char  *b_data;
  short *s_data;
  float *f_data;
  double total=0, mean=0;
  int data_unit = mh->calibration_units_label;
  float ecf = mh->calibration_factor;
  double mx=0, my=0,mz=0;
	ecat_matrix::Image_subheader *imh=NULL;
  
  mat_numdoc(matrix->matnum, &mat);
  LOG_INFO("Matrix := {},{},{},{},{}", mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
  nvoxels = matrix->xdim*matrix->ydim*matrix->zdim ;
  switch(matrix->data_type) {
  case MatrixData::DataType::ByteData:
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
  case MatrixData::DataType::IeeeFloat :
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
  mx = mx / total * matrix->pixel_size * 10;
  my = my / total * matrix->y_size * 10;
  mz = mz / total * matrix->z_size * 10;
  LOG_INFO("Dimensions := {}x{}x{}\n",matrix->xdim,matrix->ydim,matrix->zdim);
  if (mh->calibration_units == Uncalibrated || ecf <= 1.0 || data_unit > customDisplayUnits.size())
    data_unit = 0;
  units = (data_unit) ? customDisplayUnits[data_unit] : "";
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
  LOG_INFO("{}\n{}\n{}\n{}\n",line1, line2, line3, line4);
  LOG_INFO("center of mass (x,y,z) mm := {},{},{}\n",mx-1,my-1,mz-1);
	if (mh->sw_version > 72 && mh->file_type == MatrixData::DataSetType::PetVolume) 
  { // print data rates
		imh = (ecat_matrix::Image_subheader*)matrix->shptr;
		if (imh->singles_rate>MIN_SINGLE_RATE && imh->singles_rate < MAX_SINGLE_RATE)
		{ // valid Extended ECAT file
			int frame_duration_sec;
			frame_duration_sec = imh->frame_duration/1000;
			LOG_INFO("Image duration       := {}", frame_duration_sec);
			LOG_INFO("Total prompts        := {}",(__int64)((double)imh->prompt_rate * frame_duration_sec));
			LOG_INFO("Total randoms        := {}",(__int64)((double)imh->random_rate * frame_duration_sec));
			LOG_INFO("Average singles rate := {}", imh->singles_rate);
			if (imh->scatter_fraction>0.0f)
				LOG_INFO("scatter fraction     := {}", imh->scatter_fraction);
		}
	}
}

main(int argc, char **argv) {
  // ecat_matrix::MatDirNode *node;
  // ecat_matrix::MatrixFile *mptr;
  // ecat_matrix::MatrixData *matrix;
  // ecat_matrix::MatVal mat;
  char fname[FILENAME_MAX];
  // int ftype, 
  int frame = -1, matnum=0, cubic=0, interpolate=0;

   my_spdlog::init_logging(argv[0]);
  
  if (argc < 2) {
    LOG_ERROR("{}: Build {} {}", argv[0], __DATE__, __TIME__);
    LOG_EXIT("usage : {} matspec",argv[0]);
  }
  matspec( argv[1], fname, &matnum);
    ecat_matrix::MatrixFile *mptr = matrix_open(fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (mptr == NULL) {
    LOG_ERROR(fname);
    return 0;
  }
  DataSetType ftype = mptr->mhptr->file_type;
  // if (ftype <0 || ftype >= NumDataSetTypes)
  //   LOG_EXIT("%s : unkown file type\n",fname);
  LOG_INFO( "{} file type  : {}", fname, data_set_types_.at(ftype).name);
  if (!mptr) 
    LOG_ERROR(fname);
    ecat_matrix::MatVal mat;
  if (matnum) {
		mat_numdoc(matnum, &mat);
      ecat_matrix::MatrixData *matrix = matrix_read(mptr,matnum, UnknownMatDataType);
    if (!matrix) 
      LOG_EXIT("{},{},{},{},{} not found",		       mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
    matrix_info(mptr->mhptr,matrix);
  } 
	else {
      ecat_matrix::MatDirNode *node = mptr->dirlist->first;
    while (node) {
			mat_numdoc(node->matnum, &mat);
			if (ftype == PetImage) {
				if (frame != mat.frame) {
					frame = mat.frame;
					matrix = load_volume(mptr,frame,cubic,interpolate);
					matrix_info(mptr->mhptr,matrix);
				}
			} else {
				matrix = matrix_read(mptr,node->matnum,UnknownMatDataType);
				if (!matrix) 
          LOG_EXIT("{},{},{},{},{} not found\n",
					mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
				matrix_info(mptr->mhptr,matrix);
			}
			node = node->next;
		}
  }
  matrix_close(mptr);
  return 1;
}