/*
   * EcatX software is a set of routines and utility programs
   * to access ECAT(TM) data.
   *
   * Copyright (C) 2008 Merence Sibomana
   *
   * Author: Merence Sibomana <sibomana@gmail.com>
   *
   * ECAT(TM) is a registered trademark of CTI, Inc.
   * This software has been written from documentation on the ECAT
   * data format provided by CTI to customers. CTI or its legal successors
   * should not be held responsible for the accuracy of this software.
   * CTI, hereby disclaims all copyright interest in this software.
   * In no event CTI shall be liable for any claim, or any special indirect or 
   * consequential damage whatsoever resulting from the use of this software.
   *
   * This is a free software; you can redistribute it and/or
   * modify it under the terms of the GNU Lesser General Public License
   * as published by the Free Software Foundation; either version 2
   * of the License, or any later version.
   *
   * This software is distributed in the hope that it will be useful,
   * but  WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU Lesser General Public License for more details.

   * You should have received a copy of the GNU Lesser General Public License
   * along with this software; if not, write to the Free Software
   * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "analyze.h"
#include "interfile.h"
#include "machine_indep.h"

// ahc
#include <unistd.h>


#define END_OF_KEYS END_OF_INTERFILE+1
static struct dsr hdr;

int analyze_orientation=0;	/* default is spm neurologist convention */
static int analyze_read_hdr(const char *fname) 
{
	FILE *fp;
	char tmp[80];

	if ((fp = fopen(fname, R_MODE)) == NULL) return 0;
	if (fread(&hdr,sizeof(struct dsr),1,fp) < 0) return 0;
	fclose(fp);
    if (ntohl(hdr.hk.sizeof_hdr) == sizeof(struct dsr)) {
        hdr.hk.sizeof_hdr = ntohl(hdr.hk.sizeof_hdr);
        hdr.hk.extents = ntohl(hdr.hk.extents);
        swab((char*)hdr.dime.dim,tmp,4*sizeof(short));
        memcpy(hdr.dime.dim,tmp,4*sizeof(short));
        hdr.dime.datatype = ntohs(hdr.dime.datatype);
        hdr.dime.bitpix = ntohs(hdr.dime.bitpix);
        swab((char*)hdr.dime.pixdim,tmp,4*sizeof(float));
        swaw((short*)tmp,(short*)hdr.dime.pixdim,4*sizeof(float)/2);
        swab((char*)&hdr.dime.funused1,tmp,sizeof(float));
        swaw((short*)tmp,(short*)(&hdr.dime.funused1),sizeof(float)/2);
        hdr.dime.glmax = ntohl(hdr.dime.glmax);
        hdr.dime.glmin = ntohl(hdr.dime.glmin);
    }
	return 1;
}

static int _is_analyze(const char* fname)
{
	FILE *fp;
	int sizeof_hdr=0;
	int magic_number = sizeof(struct dsr);
	
	if ( (fp = fopen(fname, R_MODE)) == NULL) return 0;
	fread(&sizeof_hdr,sizeof(int),1,fp);
	fclose(fp);
	if (sizeof_hdr == magic_number || ntohl(sizeof_hdr) == magic_number ) return 1;
	return 0;
}

char* is_analyze(const char* fname)
{
	char *p, *hdr_fname=NULL, *img_fname=NULL;
	struct stat st;
	int elem_size = 0, nvoxels;
	short *dim=NULL;

	if (_is_analyze(fname)) {	/* access by .hdr */
		hdr_fname = strdup(fname);
		img_fname = malloc(strlen(fname)+4);
		strcpy(img_fname,fname);
		if ( (p=strrchr(img_fname,'.')) != NULL) *p++ = '\0';
		strcat(img_fname,".img");
	} else {					/* access by .img */
		if ( (p=strstr(fname,".img")) == NULL)
			return NULL;			/* not a .img file */
		img_fname = strdup(fname);
		hdr_fname = malloc(strlen(fname)+4);
		strcpy(hdr_fname,fname);
		if ( (p=strrchr(hdr_fname,'.')) != NULL) *p++ = '\0';
		strcat(hdr_fname,".hdr");
	}

	if (stat(img_fname,&st)>=0 &&
		_is_analyze(hdr_fname) && analyze_read_hdr(hdr_fname)) {
		dim = hdr.dime.dim;
		nvoxels = dim[1]*dim[2]*dim[3]*dim[4];
		switch (hdr.dime.datatype) {
		case 2:		/* byte */
			elem_size = 1;
			break;
		case 4:		/* short */
			elem_size = 2;
			break;
		case 8:		/* int */
			elem_size = 4;
			break;
		case 16:	/* float */
			elem_size = 4;
			break;
		}
		if (elem_size == 0) fprintf(stderr,"unkown ANALYZE data type : %d\n",
			hdr.dime.datatype);
		else {
			if (nvoxels*elem_size == st.st_size) {
				if (strcmp(hdr_fname, fname) != 0)
					fprintf(stderr,"using %s header for %s data file\n",
						hdr_fname, img_fname);
				free(img_fname);
				return hdr_fname;
			} else fprintf(stderr,
				"%s size does not match %s info\n",img_fname,hdr_fname);
		}
	}
	free(img_fname);
	free(hdr_fname);
	return NULL;
}

#ifndef _WIN32      /* unix */
static char *itoa(int i, char *buf, int radix)
{
        sprintf(buf ,"%d",i);
        return buf;
}
#endif

static char *ftoa(float f, char *buf)
{
	sprintf(buf,"%g",f);
	return buf;
}

int analyze_open(MatrixFile *mptr)
{
	Main_header *mh;
	struct MatDir matdir;
	char *data_file, *extension, patient_id[20];
	char image_extrema[128];
	short  *dim, spm_origin[5];
	int elem_size, data_offset=0, data_size, nblks, frame;
	char buf[40];

	matrix_errno = 0;
	if (!analyze_read_hdr(mptr->fname)) return 0;
    strncpy(patient_id,hdr.hist.patient_id,10);
    patient_id[10] ='\0';
	mh = mptr->mhptr;
	sprintf(mh->magic_number,"%d", (int)(sizeof(struct dsr)));
	mh->sw_version = 70;
	mh->file_type = InterfileImage;
  mptr->analyze_hdr = (char*)calloc(1, sizeof(struct dsr));
  memcpy(mptr->analyze_hdr, &hdr, sizeof(hdr));
	mptr->interfile_header = (char**)calloc(END_OF_KEYS,sizeof(char*));
	data_file = malloc(strlen(mptr->fname)+4);
	strcpy(data_file, mptr->fname);
	if ((extension = strrchr(data_file,DIR_SEPARATOR)) == NULL)
		extension = strrchr(data_file,'.');
	else extension = strrchr(extension,'.');
	if (extension != NULL) strcpy(extension,".img");
	else strcat(data_file,".img");
	mptr->interfile_header[NAME_OF_DATA_FILE] = data_file;
	if ( (mptr->fptr = fopen(data_file, R_MODE)) == NULL) {
		free(mptr->interfile_header);
		mptr->interfile_header = NULL;
		return 0;
	}
/* All strings are duplicates, they are freed by free_interfile_header */
	switch(hdr.dime.datatype) {
		case 2 :
			elem_size = 1;
			mptr->interfile_header[NUMBER_FORMAT] = strdup("unsigned integer");
			mptr->interfile_header[NUMBER_OF_BYTES_PER_PIXEL] = strdup("1");
			break;
		case 4 :
			elem_size = 2;
      if (hdr.dime.glmax > 32767)
				mptr->interfile_header[NUMBER_FORMAT] = strdup("unsigned integer");
			else mptr->interfile_header[NUMBER_FORMAT] = strdup("signed integer");
			sprintf(image_extrema,"%d %d",hdr.dime.glmin,hdr.dime.glmax);
			mptr->interfile_header[IMAGE_EXTREMA] = strdup(image_extrema);
			mptr->interfile_header[NUMBER_OF_BYTES_PER_PIXEL] = strdup("2");
			break;
		case 8 :
			elem_size = 4;
			mptr->interfile_header[NUMBER_FORMAT] = strdup("signed integer");
			mptr->interfile_header[NUMBER_OF_BYTES_PER_PIXEL] = strdup("4");
			break;
		case 16 :
			elem_size = 4;
			mptr->interfile_header[NUMBER_FORMAT] = strdup("short float");
			mptr->interfile_header[NUMBER_OF_BYTES_PER_PIXEL] = strdup("4");
			break;
		default :
			free(mptr->interfile_header);
			mptr->interfile_header = NULL;
			matrix_errno = MAT_UNKNOWN_FILE_TYPE;
			return 0;
	}
  if (ntohs(hdr.hk.sizeof_hdr) == sizeof(struct dsr))
    mptr->interfile_header[IMAGEDATA_BYTE_ORDER] = strdup("bigendian");
  else mptr->interfile_header[IMAGEDATA_BYTE_ORDER] = strdup("littleendian");
	mh->num_planes = hdr.dime.dim[3];
	mh->num_frames = hdr.dime.dim[4];
  mh->num_gates = 1;
	strcpy(mh->patient_id,patient_id);
	if (strlen(patient_id))
		mptr->interfile_header[PATIENT_ID] = strdup(patient_id);
	mptr->interfile_header[NUMBER_OF_DIMENSIONS] = strdup("3");
							/* check spm origin */
	dim = hdr.dime.dim;
  if (ntohs(hdr.hk.sizeof_hdr) == sizeof(struct dsr)) 
    swab(hdr.hist.originator,(char*)spm_origin,10);
	else memcpy(spm_origin,hdr.hist.originator,10);
	if (spm_origin[0]>1 && spm_origin[1]>1 && spm_origin[2]>1 &&
		spm_origin[0]<dim[1] && spm_origin[1]<dim[2] && spm_origin[2]<dim[3]) {
		mptr->interfile_header[ATLAS_ORIGIN_1] = strdup(itoa(spm_origin[0],buf,10));
		mptr->interfile_header[ATLAS_ORIGIN_2] = strdup(itoa(spm_origin[1],buf,10));
		mptr->interfile_header[ATLAS_ORIGIN_3] = strdup(itoa(spm_origin[2],buf,10));
	}
	mptr->interfile_header[MATRIX_SIZE_1] = strdup(itoa(dim[1],buf,10));
	mptr->interfile_header[MATRIX_SIZE_2] = strdup(itoa(dim[2],buf,10));
	mptr->interfile_header[MATRIX_SIZE_3] = strdup(itoa(dim[3],buf,10));
	mptr->interfile_header[MAXIMUM_PIXEL_COUNT] = strdup(itoa(hdr.dime.glmax,buf,10));
	if (analyze_orientation == 0)       /* Neurology convention */
		mptr->interfile_header[MATRIX_INITIAL_ELEMENT_1] = strdup("right");
	else                                /* Radiology convention */
		mptr->interfile_header[MATRIX_INITIAL_ELEMENT_1] = strdup("left");
	mptr->interfile_header[MATRIX_INITIAL_ELEMENT_2] = strdup("posterior");
	mptr->interfile_header[MATRIX_INITIAL_ELEMENT_3] = strdup("inferior");
	mptr->interfile_header[SCALE_FACTOR_1] = strdup(ftoa(hdr.dime.pixdim[1],buf));
	mptr->interfile_header[SCALE_FACTOR_2] = strdup(ftoa(hdr.dime.pixdim[2],buf));
	mptr->interfile_header[SCALE_FACTOR_3] = strdup(ftoa(hdr.dime.pixdim[3],buf));
	if (hdr.dime.funused1 > 0.0)
		mptr->interfile_header[QUANTIFICATION_UNITS] = strdup(ftoa(hdr.dime.funused1,buf));
	mh->plane_separation = hdr.dime.pixdim[3]/10;	/* convert mm to cm */
	data_size = hdr.dime.dim[1] * hdr.dime.dim[2] * hdr.dime.dim[3] * elem_size;
	mptr->dirlist = (MatDirList *) calloc(1,sizeof(MatDirList)) ;
	  nblks = (data_size+MatBLKSIZE-1)/MatBLKSIZE;
  for (frame=1; frame <= mh->num_frames; frame++)
  {
	  matdir.matnum = mat_numcod(frame,1,1,0,0);
	  matdir.strtblk = data_offset/MatBLKSIZE;
	  matdir.endblk = matdir.strtblk + nblks;
	  insert_mdir(&matdir, mptr->dirlist) ;
    matdir.strtblk += nblks; 
  }
	return 1;
}

int analyze_read(MatrixFile *mptr, int matnum, MatrixData *data, int dtype) 
{
  Image_subheader *imagesub=NULL;
  int y, z, image_min=0, image_max=0;
  size_t npixels, nvoxels;
  int nblks, elem_size=2, data_offset=0;
  void *plane, *line;
  /* u_short u_max, *up=NULL; */
  short *sp=NULL;
  int z_flip=1, y_flip=1, x_flip=1;
  int group = 0;
  struct Matval matval;
  struct image_dimension *hdim=NULL;

  mat_numdoc(matnum, &matval);
  data->matnum = matnum;
  data->pixel_size=data->y_size=data->z_size=1.0;
  data->scale_factor = 1.0;

  hdim = &(((struct dsr *)mptr->analyze_hdr)->dime);
  imagesub = (Image_subheader*)data->shptr;
  memset(imagesub,0,sizeof(Image_subheader));
	imagesub->num_dimensions = 3;
  imagesub->x_pixel_size = data->pixel_size = hdim->pixdim[1]*0.1f;
  imagesub->y_pixel_size = data->y_size = hdim->pixdim[2]*0.1f;
  imagesub->z_pixel_size = data->z_size = hdim->pixdim[3]*0.1f;
  imagesub->scale_factor = data->scale_factor = 
    (hdim->funused1 > 0.0) ? hdim->funused1: 1.0f;
  imagesub->x_dimension = data->xdim = hdim->dim[1];
  imagesub->y_dimension = data->ydim = hdim->dim[2];
  imagesub->z_dimension = data->zdim = hdim->dim[3];
  imagesub->data_type = data->data_type = SunShort;  
  if (dtype == MAT_SUB_HEADER) return ECATX_OK;


  npixels = data->xdim * data->ydim;
  nvoxels = npixels * data->zdim;
  data->data_size = nvoxels * elem_size;
  nblks = (data->data_size+MatBLKSIZE-1)/MatBLKSIZE;
  data->data_ptr = (void *) calloc(nblks,MatBLKSIZE);
  if (matval.frame>1) data_offset = (matval.frame-1)*data->data_size;
  if (data_offset>0) {
    if (fseek(mptr->fptr, data_offset, SEEK_SET) != 0) 
      crash("Error skeeping to offset %d\n", data_offset);
  }
  for (z = 0; z < data->zdim; z++) {
    if (z_flip) 
      plane = data->data_ptr + (data->zdim-z-1)*elem_size*npixels;
    else plane = data->data_ptr + z*elem_size*npixels;
    if (fread(plane,elem_size,npixels,mptr->fptr) < npixels) {
      free(data->data_ptr);
      data->data_ptr = NULL;
      matrix_errno = MAT_READ_ERROR;
      return ECATX_ERROR;
    }
    if (y_flip) 
      flip_y(plane,data->data_type,data->xdim,data->ydim);
    if (x_flip) {
      for (y = 0; y < data->ydim; y++) {
        line = plane + y*data->xdim*elem_size;
        flip_x(line,data->data_type,data->xdim);
      }
    }
  }
 // find_data_extrema(data);		/*don't trust in header extrema*/
  return ECATX_OK;
}
