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
#include "analyze.hpp"
#include "interfile.hpp"
#include "machine_indep.hpp"
#include "my_spdlog.hpp"
#include "hrrt_util.hpp"

// ahc
#include <unistd.h>

// #define END_OF_KEYS static_cast<int>(interfile::Key::END_OF_INTERFILE) + 1

static struct dsr hdr;
int analyze_orientation = 0; /* default is spm neurologist convention */

static int analyze_read_hdr(const char *fname) {
  FILE *fp;
  char tmp[80];

  if ((fp = fopen(fname, "rb")) == NULL)
    return 0;
  // if (fread(&hdr,sizeof(struct dsr),1,fp) < 0) return 0;
  if (fread(&hdr, sizeof(struct dsr), 1, fp) < sizeof(struct dsr))
    return 0;
  fclose(fp);
  if (hdr.hk.sizeof_hdr == sizeof(struct dsr)) {
    hdr.hk.sizeof_hdr = hdr.hk.sizeof_hdr;
    hdr.hk.extents = hdr.hk.extents;
    swab((char*)hdr.dime.dim, tmp, 4 * sizeof(short));
    memcpy(hdr.dime.dim, tmp, 4 * sizeof(short));
    hdr.dime.datatype = hdr.dime.datatype;
    hdr.dime.bitpix = hdr.dime.bitpix;
    swab((char*)hdr.dime.pixdim, tmp, 4 * sizeof(float));
    hrrt_util::swaw((short*)tmp, (short*)hdr.dime.pixdim, 4 * sizeof(float) / 2);
    swab((char*)&hdr.dime.funused1, tmp, sizeof(float));
    hrrt_util::swaw((short*)tmp, (short*)(&hdr.dime.funused1), sizeof(float) / 2);
    hdr.dime.glmax = hdr.dime.glmax;
    hdr.dime.glmin = hdr.dime.glmin;
  }
  return 1;
}

static int _is_analyze(const char* fname) {
  FILE *fp;
  int sizeof_hdr = 0;
  int magic_number = sizeof(struct dsr);

  if ( (fp = fopen(fname, "rb")) == NULL)
    return 0;
  fread(&sizeof_hdr, sizeof(int), 1, fp);
  fclose(fp);
  if (sizeof_hdr == magic_number || sizeof_hdr == magic_number )
    return 1;
  return 0;
}

char* is_analyze(const char* fname) {
  char *p, *hdr_fname = NULL, *img_fname = NULL;
  int elem_size = 0, nvoxels;

  if (_is_analyze(fname)) { /* access by .hdr */
    hdr_fname = strdup(fname);
    img_fname = static_cast<char *>(malloc(strlen(fname) + 4));
    strcpy(img_fname, fname);
    if ( (p = strrchr(img_fname, '.')) != NULL)
      * p++ = '\0';
    strcat(img_fname, ".img");
  } else {          /* access by .img */
    // if ( (p=strstr(fname,".img")) == NULL)
    if ( strstr(fname, ".img") == NULL)
      return NULL;      /* not a .img file */
    img_fname = strdup(fname);
    hdr_fname = static_cast<char *>(malloc(strlen(fname) + 4));
    strcpy(hdr_fname, fname);
    if ( (p = strrchr(hdr_fname, '.')) != NULL)
      * p++ = '\0';
    strcat(hdr_fname, ".hdr");
  }

  struct stat st;
  if (stat(img_fname, &st) >= 0 && _is_analyze(hdr_fname) && analyze_read_hdr(hdr_fname)) {
    short *dim = hdr.dime.dim;
    nvoxels = dim[1] * dim[2] * dim[3] * dim[4];
    switch (hdr.dime.datatype) {
    case 2:   /* byte */
      elem_size = 1;
      break;
    case 4:   /* short */
      elem_size = 2;
      break;
    case 8:   /* int */
      elem_size = 4;
      break;
    case 16:  /* float */
      elem_size = 4;
      break;
    }
    if (elem_size == 0) {
      LOG_ERROR( "unkown ANALYZE data type : {}", hdr.dime.datatype);
    } else {
      if (nvoxels * elem_size == st.st_size) {
        if (strcmp(hdr_fname, fname) != 0)
          LOG_INFO( "using {} header for {} data file", hdr_fname, img_fname);
        free(img_fname);
        return hdr_fname;
      } else {
        LOG_ERROR("{} size does not match {} info", img_fname, hdr_fname);
      }
    }
  }
  free(img_fname);
  free(hdr_fname);
  return NULL;
}

static char *itoa(int i, char *buf, int radix) {
  sprintf(buf , "%d", i);
  return buf;
}

static char *ftoa(float f, char *buf) {
  sprintf(buf, "%g", f);
  return buf;
}

int analyze_open(ecat_matrix::MatrixFile *mptr) {
  ecat_matrix::Main_header *mh;
  ecat_matrix::MatDir matdir;
  char *data_file, *extension, patient_id[20];
  char image_extrema[128];
  short  *dim, spm_origin[5];
  int elem_size, data_offset = 0, data_size, nblks, frame;
  char buf[40];

  ecat_matrix::matrix_errno = static_cast<ecat_matrix::MatrixError>(0);
  if (!analyze_read_hdr(mptr->fname)) return 0;
  strncpy(patient_id, hdr.hist.patient_id, 10);
  patient_id[10] = '\0';
  mh = mptr->mhptr;
  sprintf(mh->magic_number, "%d", (int)(sizeof(struct dsr)));
  mh->sw_version = 70;
  mh->file_type = ecat_matrix::DataSetType::InterfileImage;
  mptr->analyze_hdr = (char*)calloc(1, sizeof(struct dsr));
  memcpy(mptr->analyze_hdr, &hdr, sizeof(hdr));
  // mptr->interfile_header = (char**)calloc(END_OF_KEYS, sizeof(char*));
  mptr->interfile_header = (char**)calloc(interfile::used_keys.size(), sizeof(char*));
  data_file = static_cast<char *>(malloc(strlen(mptr->fname) + 4));
  strcpy(data_file, mptr->fname);
  if ((extension = strrchr(data_file, '/')) == NULL)
    extension = strrchr(data_file, '.');
  else extension = strrchr(extension, '.');
  if (extension != NULL) strcpy(extension, ".img");
  else strcat(data_file, ".img");
  mptr->interfile_header[static_cast<int>(interfile::Key::NAME_OF_DATA_FILE)] = data_file;
  if ( (mptr->fptr = fopen(data_file, "rb")) == NULL) {
    free(mptr->interfile_header);
    mptr->interfile_header = NULL;
    return 0;
  }

  /* All strings are duplicates, they are freed by free_interfile_header */
  // ahc all these casts can go away when interfile_header is reimplemneted as a map
  switch (hdr.dime.datatype) {
  case 2 :
    elem_size = 1;
    mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_FORMAT)] = strdup("unsigned integer");
    mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_OF_BYTES_PER_PIXEL)] = strdup("1");
    break;
  case 4 :
    elem_size = 2;
    if (hdr.dime.glmax > 32767)
      mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_FORMAT)] = strdup("unsigned integer");
    else mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_FORMAT)] = strdup("signed integer");
    sprintf(image_extrema, "%d %d", hdr.dime.glmin, hdr.dime.glmax);
    mptr->interfile_header[static_cast<int>(interfile::Key::IMAGE_EXTREMA)] = strdup(image_extrema);
    mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_OF_BYTES_PER_PIXEL)] = strdup("2");
    break;
  case 8 :
    elem_size = 4;
    mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_FORMAT)] = strdup("signed integer");
    mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_OF_BYTES_PER_PIXEL)] = strdup("4");
    break;
  case 16 :
    elem_size = 4;
    mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_FORMAT)] = strdup("short float");
    mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_OF_BYTES_PER_PIXEL)] = strdup("4");
    break;
  default :
    free(mptr->interfile_header);
    mptr->interfile_header = NULL;
    ecat_matrix::matrix_errno = ecat_matrix::MatrixError::UNKNOWN_FILE_TYPE;
    return 0;
  }
  if (hdr.hk.sizeof_hdr == sizeof(struct dsr))
    mptr->interfile_header[static_cast<int>(interfile::Key::IMAGEDATA_BYTE_ORDER)] = strdup("bigendian");
  else mptr->interfile_header[static_cast<int>(interfile::Key::IMAGEDATA_BYTE_ORDER)] = strdup("littleendian");
  mh->num_planes = hdr.dime.dim[3];
  mh->num_frames = hdr.dime.dim[4];
  mh->num_gates = 1;
  strcpy(mh->patient_id, patient_id);
  if (strlen(patient_id))
    mptr->interfile_header[static_cast<int>(interfile::Key::PATIENT_ID)] = strdup(patient_id);
  mptr->interfile_header[static_cast<int>(interfile::Key::NUMBER_OF_DIMENSIONS)] = strdup("3");
  /* check spm origin */
  dim = hdr.dime.dim;
  if (hdr.hk.sizeof_hdr == sizeof(struct dsr))
    swab(hdr.hist.originator, (char*)spm_origin, 10);
  else memcpy(spm_origin, hdr.hist.originator, 10);
  if (spm_origin[0] > 1 && spm_origin[1] > 1 && spm_origin[2] > 1 &&
      spm_origin[0] < dim[1] && spm_origin[1] < dim[2] && spm_origin[2] < dim[3]) {
    mptr->interfile_header[static_cast<int>(interfile::Key::ATLAS_ORIGIN_1)] = strdup(itoa(spm_origin[0], buf, 10));
    mptr->interfile_header[static_cast<int>(interfile::Key::ATLAS_ORIGIN_2)] = strdup(itoa(spm_origin[1], buf, 10));
    mptr->interfile_header[static_cast<int>(interfile::Key::ATLAS_ORIGIN_3)] = strdup(itoa(spm_origin[2], buf, 10));
  }
  mptr->interfile_header[static_cast<int>(interfile::Key::MATRIX_SIZE_1)] = strdup(itoa(dim[1], buf, 10));
  mptr->interfile_header[static_cast<int>(interfile::Key::MATRIX_SIZE_2)] = strdup(itoa(dim[2], buf, 10));
  mptr->interfile_header[static_cast<int>(interfile::Key::MATRIX_SIZE_3)] = strdup(itoa(dim[3], buf, 10));
  mptr->interfile_header[static_cast<int>(interfile::Key::MAXIMUM_PIXEL_COUNT)] = strdup(itoa(hdr.dime.glmax, buf, 10));
  if (analyze_orientation == 0)       /* Neurology convention */
    mptr->interfile_header[static_cast<int>(interfile::Key::MATRIX_INITIAL_ELEMENT_1)] = strdup("right");
  else                                /* Radiology convention */
    mptr->interfile_header[static_cast<int>(interfile::Key::MATRIX_INITIAL_ELEMENT_1)] = strdup("left");
  mptr->interfile_header[static_cast<int>(interfile::Key::MATRIX_INITIAL_ELEMENT_2)] = strdup("posterior");
  mptr->interfile_header[static_cast<int>(interfile::Key::MATRIX_INITIAL_ELEMENT_3)] = strdup("inferior");
  mptr->interfile_header[static_cast<int>(interfile::Key::SCALE_FACTOR_1)] = strdup(ftoa(hdr.dime.pixdim[1], buf));
  mptr->interfile_header[static_cast<int>(interfile::Key::SCALE_FACTOR_2)] = strdup(ftoa(hdr.dime.pixdim[2], buf));
  mptr->interfile_header[static_cast<int>(interfile::Key::SCALE_FACTOR_3)] = strdup(ftoa(hdr.dime.pixdim[3], buf));
  if (hdr.dime.funused1 > 0.0)
    mptr->interfile_header[static_cast<int>(interfile::Key::QUANTIFICATION_UNITS)] = strdup(ftoa(hdr.dime.funused1, buf));
  mh->plane_separation = hdr.dime.pixdim[3] / 10; /* convert mm to cm */
  data_size = hdr.dime.dim[1] * hdr.dime.dim[2] * hdr.dime.dim[3] * elem_size;
  mptr->dirlist = (ecat_matrix::MatDirList *) calloc(1, sizeof(ecat_matrix::MatDirList)) ;
  nblks = (data_size + ecat_matrix::MatBLKSIZE - 1) / ecat_matrix::MatBLKSIZE;
  for (frame = 1; frame <= mh->num_frames; frame++) {
    matdir.matnum = ecat_matrix::mat_numcod(frame, 1, 1, 0, 0);
    matdir.strtblk = data_offset / ecat_matrix::MatBLKSIZE;
    matdir.endblk = matdir.strtblk + nblks;
    insert_mdir(&matdir, mptr->dirlist) ;
    matdir.strtblk += nblks;
  }
  return 1;
}

int analyze_read(ecat_matrix::MatrixFile *mptr, int matnum, ecat_matrix::MatrixData *t_mdata, ecat_matrix::MatrixDataType dtype) {
  ecat_matrix::MatVal matval;

  mat_numdoc(matnum, &matval);
  t_mdata->matnum = matnum;
  t_mdata->pixel_size = t_mdata->y_size = t_mdata->z_size = 1.0;
  t_mdata->scale_factor = 1.0;

  struct image_dimension *hdim = &(((struct dsr *)mptr->analyze_hdr)->dime);
  ecat_matrix::Image_subheader *imagesub = (ecat_matrix::Image_subheader*)t_mdata->shptr;
  memset(imagesub, 0, sizeof(ecat_matrix::Image_subheader));
  imagesub->num_dimensions = 3;
  imagesub->x_pixel_size = t_mdata->pixel_size = hdim->pixdim[1] * 0.1f;
  imagesub->y_pixel_size = t_mdata->y_size = hdim->pixdim[2] * 0.1f;
  imagesub->z_pixel_size = t_mdata->z_size = hdim->pixdim[3] * 0.1f;
  imagesub->scale_factor = t_mdata->scale_factor = (hdim->funused1 > 0.0) ? hdim->funused1 : 1.0f;
  imagesub->x_dimension = t_mdata->xdim = hdim->dim[1];
  imagesub->y_dimension = t_mdata->ydim = hdim->dim[2];
  imagesub->z_dimension = t_mdata->zdim = hdim->dim[3];
  imagesub->data_type = t_mdata->data_type = ecat_matrix::MatrixDataType::SunShort;
  if (dtype == ecat_matrix::MatrixDataType::MAT_SUB_HEADER)
    return ecat_matrix::ECATX_OK;

  int  data_offset = 0;
  int elem_size = 2;
  size_t npixels = t_mdata->xdim * t_mdata->ydim;
  size_t nvoxels = npixels * t_mdata->zdim;
  t_mdata->data_size = nvoxels * elem_size;
  int nblks = (t_mdata->data_size + ecat_matrix::MatBLKSIZE - 1) / ecat_matrix::MatBLKSIZE;
  t_mdata->data_ptr = (void *) calloc(nblks, ecat_matrix::MatBLKSIZE);
  if (matval.frame > 1) data_offset = (matval.frame - 1) * t_mdata->data_size;
  if (data_offset > 0) {
    if (fseek(mptr->fptr, data_offset, SEEK_SET) != 0)
      LOG_EXIT("Error skeeping to offset {}", data_offset);
  }
  int z_flip = 1, y_flip = 1, x_flip = 1;
  void *plane, *line;
  for (int z = 0; z < t_mdata->zdim; z++) {
    if (z_flip)
      plane = static_cast<uint8_t *>(t_mdata->data_ptr) + (t_mdata->zdim - z - 1) * elem_size * npixels;
    else
      plane = static_cast<uint8_t *>(t_mdata->data_ptr) + z * elem_size * npixels;
    if (fread(plane, elem_size, npixels, mptr->fptr) < npixels) {
      free(t_mdata->data_ptr);
      t_mdata->data_ptr = NULL;
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::READ_ERROR;
      return ecat_matrix::ECATX_ERROR;
    }
    if (y_flip)
      interfile::flip_y(plane, t_mdata->data_type, t_mdata->xdim, t_mdata->ydim);
    if (x_flip) {
      for (int y = 0; y < t_mdata->ydim; y++) {
        // line = plane + y * t_mdata->xdim * elem_size;
        line = static_cast<uint8_t *>(plane) + y * t_mdata->xdim * elem_size;
        interfile::flip_x(line, t_mdata->data_type, t_mdata->xdim);
      }
    }
  }
// find_data_extrema(t_mdata);    /*don't trust in header extrema*/
  return ecat_matrix::ECATX_OK;
}
