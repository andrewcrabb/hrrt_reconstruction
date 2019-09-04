// gsmooth.cpp : Defines the entry point for the console application.
//
/*[ gsmooth
------------------------------------------------

   Component       : <Add your component name here>

   Name            : gsmooth.cpp - gaussian smoothing
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 10/10/2003
   Modification history:
        04-Feb-2009: Add ECAT multiframe support
        01-apr-2010: Add multi_movie_mode to create frames on same scale 
        25-may-2010: Use SIMD for gaussian smoothing 

   Description     :  applies a 3D gaussian smoothing on an image.
					  Supports ECAT7 and Interfile formats

               Copyright (C) CPS Innovations 2003 All Rights Reserved.

---------------------------------------------------------------------*/
// Uses ARS Filter class:
// XYSamples       width/height of image
// DeltaXY         width/height of image voxel in mm
// ZSamples        depth of image
// image           pointer to image
// num_log_cpus    number of CPUs to use
// gauss_fwhm_xy   filter width in mm FWHM
// gauss_fwhm_z    filter width in mm FWHM


#include "image_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include "my_spdlog.hpp"

static float gauss_fwhm_xy = 2.0;
static float gauss_fwhm_z = 2.0;
static int num_log_cpus=1;

#include "ecatx/matrix.h"

#include <io.h>
#include <direct.h>
#define access _access
#define my_mkdir _mkdir
#define F_OK 0x0
#define W_OK 0x2
#define R_OK 0x4
#define RW_OK 0x6

#include <unistd.h>

int main(int argc, char **argv) {

	char *in_fname=NULL, out_fname[FILENAME_MAX], ext[FILENAME_MAX];
	char in_hdr_fname[FILENAME_MAX];
	ecat_matrix::MatrixFile *in = NULL, *out=NULL;
	char *p=NULL;
  int multi_frame_movie_mode = 0, simd_mode=1;
  int ecat_flag=1, cubic_flag=0;

	if (argc<2)	{
    LOG_ERROR("\n%s Build %s %s\n\n",argv[0],__DATE__,__TIME__);
		LOG_ERROR("Usage: gsmooth input [FWHM [output [k|m|s]]]\n");
		LOG_ERROR("        Applies a 3D gaussian smoothing with specified FWHM in mm\n");
		LOG_ERROR("        Valid FWHM values are between 1.0 and 10.0, default=2.0\n");
		LOG_ERROR("        Default output is input_xmm.extension where x is the FWHM\n");
		LOG_ERROR("        Examples of extensions: .i,.v,.img,.hdr,.h33\n");
		LOG_ERROR("        Supports ECAT(static&multi-frame) and Interfile formats\n");
    LOG_ERROR("        m: multi-frame movie mode to create frames with same scale\n");
    LOG_ERROR("        s: use single arithmetic instead SIMD default\n");
    LOG_ERROR("        i: Use isotropic voxelsize output instead of original voxelsize\n");
		return 1;
	}
	in_fname = argv[1];
	if (argc>2)
	{
		if (sscanf(argv[2],"%g",&gauss_fwhm_xy) != 1 ||
			(gauss_fwhm_xy<1.0 || gauss_fwhm_xy>10.0) )
		{
			LOG_ERROR("Invalid  FWHM value: %s, must be between 1.0 and 10.0\n",
				argv[2]);
			return 1;
		}
		gauss_fwhm_z = gauss_fwhm_xy;
	}
	if (argc > 3) {
    strcpy(out_fname, argv[3]);
	} else {
		strcpy(out_fname, in_fname);
		if ((p = strrchr(out_fname,'.')) != NULL)	{
			sprintf(ext,"_%dmm",(int)gauss_fwhm_xy);
			strcat(ext,p);
			*p = '\0';
			strcat(out_fname, ext);
		}
	}
	
  if (argc > 4) {
    if (tolower(argv[4][0]) == 'm') 
      multi_frame_movie_mode = 1;
    else if (tolower(argv[4][0]) == 's') 
      simd_mode = 0;
    else if (tolower(argv[4][0]) == 'i') 
      cubic_flag = 1;
  }
  strcpy(in_hdr_fname, in_fname);
	strcat(in_hdr_fname,".hdr");
  clock_t c1=clock();
	if (access(in_hdr_fname, F_OK) == 0) {
		if ((in = matrix_open(in_hdr_fname,ecat_matrix::MatrixFileAccessMode::READ_ONLY,ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE)) == NULL)
			LOG_EXIT("Error opening %s\n", in_hdr_fname);
	} else {
		if ((in = matrix_open(in_fname,ecat_matrix::MatrixFileAccessMode::READ_ONLY,ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE)) == NULL)
			LOG_EXIT("Error opening %s\n", in_fname);
		 int pos = strlen(in_fname) - 6;
		if (pos>0 && strcasecmp(in_fname+pos, ".i.hdr")== 0) {
			strncpy(out_fname,in_fname, pos);
			sprintf(&out_fname[pos], "_%dmm.i",(int)gauss_fwhm_xy);
		}
	}
	if (in->dirlist->nmats == 0)
    LOG_EXIT("Error: %s is empty\n", in_fname);
  clock_t c2=clock();  
  // printf("matrix_open: \t%f sec\n",(c2-c1+0.0)/CLOCKS_PER_SEC);   
  ecat_matrix::MatDirNode *node = in->dirlist->first;
  int frame=0;
  while (node) {
    ecat_matrix::MatVal mat;
    int matnum = node->matnum;
    mat_numdoc(matnum, &mat);
    if (in->dirlist->nmats > 1) 
      LOG_INFO("Smoothing frame %d\n", mat.frame);
    c1=clock();
    ecat_matrix::MatrixData *volume = matrix_read(in, matnum,GENERIC);
    c2=clock();  
    // printf("matrix_read: \t%f sec\n",(c2-c1+0.0)/CLOCKS_PER_SEC);  
    if (volume == NULL) LOG_EXIT("error loading %s frame %d\n", in_fname, mat.frame);
    if (volume->xdim != volume->ydim) 
      LOG_EXIT("%s : unsupported different X and Y dimensions (%d,%d)\n",
		in_fname,
		volume->xdim, volume->ydim);
    float xfov = volume->xdim*volume->pixel_size, yfov=volume->ydim*volume->y_size;
    if (fabs(xfov - yfov) > 0.1)
      LOG_EXIT("%s : unsupported non-squared image FOV (%g,%g)\n",
      in_fname, xfov, yfov);
    
    int sx=volume->xdim, sy=volume->xdim, sz=volume->zdim;
    // convert voxelsize from cm to mm
    float dx = 10.0f*volume->pixel_size, dz=10.0f*volume->z_size;
    int i=0, nvoxels = sx*sy*sz;
    short *sdata = (short*)volume->data_ptr;
    unsigned short *udata = (unsigned short*)volume->data_ptr;
    unsigned char *bdata = (unsigned char*)volume->data_ptr;
    float *image = NULL;
    float scalef = volume->scale_factor;
    switch(volume->data_type) {
    case MatrixData::DataType::SunShort:
    case MatrixData::DataType::VAX_Ix2:
      image = (float*)calloc(nvoxels, sizeof(float));
      for (i=0; i<nvoxels; i++)
        image[i] = sdata[i]*scalef;
      break;
    case MatrixData::DataType::ByteData:
      image = (float*)calloc(nvoxels, sizeof(float));
      for (i=0; i<nvoxels; i++)
        image[i] = bdata[i]*scalef;
      break;
    case MatrixData::DataType::UShort_BE:
    case MatrixData::DataType::UShort_LE:
      image = (float*)calloc(nvoxels, sizeof(float));
      for (i=0; i<nvoxels; i++)
        image[i] = udata[i]*scalef;
      break;
    case MatrixData::DataType::IeeeFloat:
      image = (float*)volume->data_ptr;
      break;
    default:
      LOG_EXIT("%s : unsupported image data type (%d)\n",
        in_fname, volume->data_type);
    }
    
    if (volume->data_type != MatrixData::DataType::IeeeFloat) {
      free(volume->data_ptr);
      volume->data_ptr = NULL;
    }
    
    if (simd_mode) {  // z-padding
      int sz_padded = ((volume->zdim+3)/4)*4;
      float *old = image;
      c1=clock();
      image = (float*)calloc(sx*sy*sz_padded, sizeof(float));
      memcpy(image, old, sx*sy*sz*sizeof(float));
      for (i=sz; i<sz_padded; i++) 
        memcpy(&image[i*sx*sy], &old[sx*sy*(sz-1)], sx*sy*sizeof(float));
      free(old);
      sz = sz_padded;
      c2 =clock();
      // printf("Z padding \t%f sec\n",(c2-c1+0.0)/CLOCKS_PER_SEC);   
    }
    float *image2=NULL;
    ImageFilter *filter=NULL;
    tfilter f;
    // filtering in x/y-direction
    f.name=GAUSS;
    f.restype=FWHM;
    f.resolution=gauss_fwhm_xy;               // filter-width in mm FWHM
    filter=new ImageFilter(sx, sz, f, dx, 1.0f, true, 1);
    
    c1=clock();
    if (simd_mode) filter->calcFilter_ps(image);
    else filter->calcFilter(image, num_log_cpus);
    c2 =clock();
    // printf("XY filter: \t%f sec\n",(c2-c1+0.0)/CLOCKS_PER_SEC);   

    delete filter;
    filter=NULL;
                                       // filtering in z-direction
    f.name=GAUSS;
    f.restype=FWHM;
    f.resolution=gauss_fwhm_z;         // filter-width in mm FWHM
    filter=new ImageFilter(sx, sz, f, dz, 1.0f, false, 1);
    c1=clock();
    if (simd_mode) filter->calcFilter_ps(image);
    else filter->calcFilter(image, num_log_cpus);
    c2 =clock();
    // printf("Total Z filter: \t%f sec\n",(c2-c1+0.0)/CLOCKS_PER_SEC);   
    delete filter;
    filter=NULL;
    
    if (simd_mode)
      sz = volume->zdim; // restore zsize

    if (cubic_flag) {  // simple implementation for HRRT 128x128x207
      if (volume->xdim==128 && volume->zdim==207) {
        for (i=2; i<sz; i += 2) 
          memcpy(&image[(i/2)*sx*sy], &image[i*sx*sy], sx*sy*sizeof(float));
        volume->zdim = sz = sz/2+1;
        volume->z_size = volume->pixel_size;
        nvoxels = sx*sy*sz;
        volume->data_size = nvoxels*sizeof(short);
      }
    }

    c1 =clock();
	  char **interfile_hdr = in->interfile_header;
    if (in->analyze_hdr==NULL && interfile_hdr!=NULL) {
     // Interfile format: use float
      volume->data_type = MatrixData::DataType::IeeeFloat;
    } else {   // ECAT format: convert to Short
      ecat_matrix::Image_subheader* imh = (ecat_matrix::Image_subheader*)volume->shptr;
         // update z dimension in case it changed
      imh->z_dimension = volume->zdim;
      imh->z_pixel_size = volume->z_size;
      in->mhptr->num_planes = volume->zdim;

      volume->data_type = MatrixData::DataType::SunShort;
      imh->data_type = volume->data_type;
      float fmin = find_fmin(image, nvoxels);
      float fmax = find_fmax(image, nvoxels);
      if (multi_frame_movie_mode) {
        scalef = (float)fabs(fmax)/32767;
        for (i=0;i<nvoxels;i++) 
          if (image[i]<0) image[i]=0.0f;
      } else {
        if (fabs(fmin) > fmax) scalef = (float)fabs(fmin)/32767;
        else scalef = (float)fabs(fmax)/32767;
      }
      volume->scale_factor = scalef;
		  volume->data_max = fmax;
		  volume->data_min = fmin;
		  float fact = 1.0f/scalef; // multiply faster than divide
		  imh->image_max = (short)(fmax*fact);
		  imh->image_min = (short)(fmin*fact);
      if (multi_frame_movie_mode) {
		    imh->scale_factor = volume->scale_factor = 1.0f;
        volume->data_max = imh->image_max;
        volume->data_min = imh->image_min;
        LOG_INFO("Image extrema : {}, {}", volume->data_min, volume->data_max);
      } else {
        imh->scale_factor = volume->scale_factor;
      }
		  sdata = (short*)calloc(nvoxels, sizeof(short));
		  for (i=0;i<nvoxels;i++) sdata[i] = (short)(fact*image[i]);
		  volume->data_ptr = (void *)sdata;
		  free(image);
	  }
  	
	  if (in->analyze_hdr==NULL && interfile_hdr != NULL) {
		  if ((p = strrchr(out_fname,'.')) != NULL) {
			  char data_file[FILENAME_MAX];
			  strcpy(p+1,"i");
			  strcpy(data_file, out_fname);
			  FILE *fp = fopen(out_fname,"wb");
			  if (fp==NULL) LOG_EXIT("Error creating %s\n", out_fname);
			  int data_size = volume->data_type == MatrixData::DataType::IeeeFloat? 
				  nvoxels*sizeof(float) : nvoxels*sizeof(short);
			  if (fwrite(image,data_size,1,fp) != 1)
				  LOG_EXIT("Error writing %s\n", out_fname);
			  fclose(fp);
			  strcpy(p+1,"i.hdr");
			  FILE *in_hdr = fopen(in->fname,"rb");
			  if (fp==NULL)
          LOG_EXIT("Error opening input header %s\n", in->fname);
			  if ((fp = fopen(out_fname,"wb")) == NULL)
				  LOG_EXIT("Error creating %s\n", out_fname);
			  const char *endianess = "BIGENDIAN";
  #ifdef unix
			  if (ntohs(1)!=1) endianess = "LITTLEENDIAN";
  #endif
			  char line[256];
			  while (fgets(line,256,in_hdr) != NULL)
			  {
				  if (strstr(line,"name of data file") != NULL )
				  {
					  if ((p=strrchr(data_file,SEPARATOR)) == NULL)
						  fprintf(fp,"name of data file := %s\n", data_file);
					  else fprintf(fp,"name of data file := %s\n", p+1);
				  }
				  else if (strstr(line,"imagedata byte order") != NULL)
          {
						  fprintf(fp,"imagedata byte order := %s\n", endianess);
          }
          else if (strstr(line,"matrix size [3]") != NULL) 
          {
             fprintf(fp,"matrix size [3] := %d\n", volume->zdim);
          }
          else if (strstr(line,"scaling factor (mm/pixel) [3]") != NULL) 
          {
             fprintf(fp,"scaling factor (mm/pixel) [3] := %d\n", volume->z_size);
          }
          else fputs(line, fp);
			  }
			  fprintf(fp,";gaussian 3D post-smoothing := %g mm\n",
				  gauss_fwhm_xy);
			  if (volume->data_type != MatrixData::DataType::IeeeFloat)
				  fprintf(fp,";%%quantification units := %g mm\n", 
				  volume->scale_factor);
			  fclose(in_hdr);
			  fclose(fp);
		  }
	  }
	  else
	  {
		  // ECAT 7 format
		  ecat_matrix::Main_header proto;
		  memcpy(&proto, in->mhptr, sizeof(ecat_matrix::Main_header));
      if (in->analyze_hdr)
      {
        proto.file_type = MatrixData::DataSetType::PetVolume;
      }
      if (multi_frame_movie_mode)  proto.calibration_factor = 1.0f;
      if (frame==0) 
      {
        out = matrix_create(out_fname, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, &proto);
		     if (out==NULL) LOG_EXIT("Error creating %s\n", out_fname);
      }
		  if (matrix_write(out, volume->matnum, volume) != 0)
			  LOG_EXIT("Error writing %s\n", out_fname);
    }
	  free_matrix_data(volume);
    node = node->next;
    c2 =clock();
    // printf("matrix_write: \t%f sec\n",(c2-c1+0.0)/CLOCKS_PER_SEC);   
    frame++;
  }
  matrix_close(in);
  if (out != NULL) matrix_close(out);
	return 0;
}
