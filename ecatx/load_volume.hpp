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
#pragma once
/*
 * The function
 * ecat_matrix::MatrixData *load_volume(ecat_matrix::MatrixFile*, int frame, int cubic, int interp)
 * loads a frame from a ecat_matrix::MatrixFile. The frames slices may be stored as separate
 * matrices (ECAT V6x files) or as a single volume data.
 * if the cubic flag is non zero, the function returns a volume with cubic
 * voxel.
 * if the interp is set cubic voxels are made by linear interpolation in the
 * z-direction and by nearest voxel pixel otherwise. This flag is not used
 * when the cubic flag is not set.
 * 
 */
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "my_spdlog.hpp"

#include "ecat_matrix.hpp"

typedef struct _Tslice {
    int matnum;
    float zloc;
} Tslice;

typedef int (*qsort_func)(const void *, const void *);

static int compare_zloc(const Tslice *a, const Tslice *b) {
    if (a->zloc < b->zloc) 
      return(-1);
    else if (a->zloc > b->zloc) 
      return (1);
    else 
      return (0);
}

static int slice_sort(Tslice *slices, int count){
  qsort(slices, count, sizeof(Tslice), (qsort_func)compare_zloc);
  return 1;
}


static int load_slices(ecat_matrix::MatrixFile *matrix_file, ecat_matrix::MatrixData *volume, Tslice *slice, int nslices, int cubic, int interp) {
  int i, j, k, sz;
  ecat_matrix::MatrixData *s1, *s2;
  ecat_matrix::Image_subheader *imh=NULL;
  float fval;
  int ival;
  short *vp=NULL, *p1, *p2;
  unsigned char  *b_vp=NULL, *b_p1, *b_p2;
  int npixels, nvoxels;
  char cbufr[256];
  float  zsep,zloc, w1, w2, scalef = volume->scale_factor;


  zsep = matrix_file->mhptr->plane_separation;
  slice_sort( slice, nslices);
  if (cubic) sz = (int)(1+(0.5+slice[nslices-1].zloc/volume->pixel_size));
  else sz = nslices;
  volume->zdim = sz;
  npixels = volume->xdim*volume->ydim;
  nvoxels = npixels*volume->zdim;
  imh = (ecat_matrix::Image_subheader*)volume->shptr;
  switch (volume->data_type) {
  case MatrixData::DataType::ByteData : 
    volume->data_ptr = (void *)calloc(nvoxels,sizeof(unsigned char ));
    b_vp = (unsigned char *)volume->data_ptr;
    break;
  case MatrixData::DataType::VAX_Ix2:
  case MatrixData::DataType::SunShort:
  default:
    volume->data_ptr = (void *)calloc(nvoxels,sizeof(short));
    vp = (short*)volume->data_ptr;
  }
            /* set position to center */
  if (!vp && !b_vp)  {
    LOG_ERROR("malloc failure for {} voxels", nvoxels);
    return 0;
  }
  if (!cubic) {
    for (i=0; i<nslices; i++)      {
      s1 = matrix_read( matrix_file, slice[i].matnum, UnknownMatDataType);
      switch (volume->data_type) {
      case MatrixData::DataType::ByteData : 
        b_p1 = (unsigned char *) s1->data_ptr;
        w1 = s1->scale_factor/scalef;
        for (k=0; k<npixels; k++, b_vp++ ) {
          ival = (int)(w1*(*b_p1++));
          if (ival < 0) *b_vp = 0;
          else if (ival > 255) *b_vp = 255;
          else *b_vp = (unsigned char )(ival);
        }
        break;
      case MatrixData::DataType::VAX_Ix2:
      case MatrixData::DataType::SunShort:
      default:
        p1 = (short int*) s1->data_ptr;
        w1 = s1->scale_factor/scalef;
        for (k=0; k<npixels; k++, vp++) {
          ival = (int)(w1*(*p1++));
          if (ival < -32768) *vp = -32768;
          else if (ival > 32767) *vp = 32767;
          else *vp = (short)(ival);
        }
      }
      free_matrix_data( s1);
      }
    return 1;
  }
  j = 1;
  s1 = matrix_read( matrix_file, slice[0].matnum, UnknownMatDataType);
  s2 = matrix_read( matrix_file, slice[1].matnum, UnknownMatDataType);
  for (i=0; i<sz; i++)
  {
    zloc = i*volume->pixel_size;
    LOG_INFO("Computing slice {} ({:.2f} cm)", i+1, zloc);
    while (zloc > slice[j].zloc)    {
      free_matrix_data( s1);
      s1 = s2;
      if (j < nslices-1)
        s2 = matrix_read(matrix_file, slice[++j].matnum,
           UnknownMatDataType);
      else {    /*  plane overflow */
        j++;
        slice[j].zloc = slice[j-1].zloc+zsep;
        s2 = NULL;
        break;
      }
    }
    if (!s2) break;
    w2 = (zloc-slice[j-1].zloc)/(slice[j].zloc-slice[j-1].zloc);
    if (!interp) w2 = (int)(w2+0.5);
/* speed could be improved if not interp */
    w1 = 1.0 - w2;
    w1 = w1*s1->scale_factor;
    w2 = w2*s2->scale_factor;
    switch (volume->data_type) {
    case MatrixData::DataType::ByteData : 
       b_p1 = (unsigned char *)s1->data_ptr;
       b_p2 = (unsigned char *)s2->data_ptr;
      for (k=0; k<npixels; k++, b_vp++) {
        fval = w1*(*b_p1++)+w2*(*b_p2++);
        ival = (int)(fval/scalef);
        if (ival < 0) *b_vp = 0;
        else if (ival > 255) *b_vp = 255;
        else *b_vp = (unsigned char )(ival);
      }
      break;
    case MatrixData::DataType::VAX_Ix2:
    case MatrixData::DataType::SunShort:
    default:
      p1 = (short int*) s1->data_ptr;
      p2 = (short int*) s2->data_ptr;
      for (k=0; k<npixels; k++, vp++) {
        fval = w1*(*p1++)+w2*(*p2++);
        ival = (int)(fval/scalef);
        if (ival < -32768) *vp = -32768;
        else if (ival > 32767) *vp = 32767;
        else *vp = (short)(ival);
      }
      break;
    }
  }
  free_matrix_data( s1);
  if (s2 && s2 != s1) free_matrix_data(s2);
  return 1;
}

static int load_v_slices(ecat_matrix::MatrixFile *matrix_file, ecat_matrix::MatrixData *volume,
Tslice *slice, int interp) 
{
  ecat_matrix::MatrixData *v_slices;
  short *vp, *s1_data, *s2_data, *s_p1, *s_p2;
  unsigned char  *b_vp, *b1_data, *b2_data, *b_p1, *b_p2;
  float *f1_data, *f2_data, *f_p1, *f_p2;
  float fval;
  float zloc, w1, w2, zsep,scalef;
  unsigned i, j, k, sz;
  int npixels, nvoxels, nslices;
  char cbufr[256];

  v_slices = matrix_read( matrix_file, slice[0].matnum, UnknownMatDataType);
  nslices = v_slices->zdim;
              /* update extrema */
  volume->scale_factor = v_slices->scale_factor;
  volume->data_max = v_slices->data_max;
  volume->data_max = v_slices->data_min;
  if (volume->shptr != NULL) 
    memcpy(volume->shptr,v_slices->shptr,sizeof(ecat_matrix::Image_subheader));

  zsep = volume->z_size;
  scalef = volume->scale_factor;
  for (j=1; j<nslices; j++) slice[j].zloc = slice[0].zloc+zsep*j;
  slice_sort( slice, nslices);
  sz = volume->zdim = (int)(1+(0.5+slice[nslices-1].zloc/volume->pixel_size));
            /* set position to center */
  npixels = volume->xdim*volume->ydim;
  nvoxels = npixels*sz;
  if (volume->data_type == MatrixData::DataType::ByteData) 
    volume->data_ptr = (void *)calloc(nvoxels,1);
  else 
    volume->data_ptr = (void *)calloc(nvoxels,sizeof(short));
  if (!volume->data_ptr)  {
    LOG_ERROR("malloc failure for {} voxels", nvoxels);
    return 0;
  }
  switch(v_slices->data_type) {
  case MatrixData::DataType::ByteData:
    b_vp = (unsigned char *)volume->data_ptr;
    j = 1;
    b1_data = (unsigned char *)v_slices->data_ptr;
    b2_data = (unsigned char *)v_slices->data_ptr+npixels;
    for (i=0; i<sz; i++)
    {
      zloc = i*volume->pixel_size;
      LOG_INFO("Computing slice {} ({})", i+1, zloc);
      while (zloc > slice[j].zloc)      {
        b1_data = b2_data;
        if (j < nslices-1) {
          j++;
          b2_data = (unsigned char *)v_slices->data_ptr+npixels*j;
        } else {  /*  plane overflow */
          j++;
          slice[j].zloc = slice[j-1].zloc+zsep;
          b2_data = NULL;
          break;
        }
      }
      if (!b2_data) break;    /* plane overflow */
      w2 = (zloc-slice[j-1].zloc)/(slice[j].zloc-slice[j-1].zloc);
      if (!interp) w2 = (int)(w2+0.5);
      w1 = 1.0 - w2;
      w1 *= scalef;
      w2 *= scalef;
      b_p1 = b1_data; b_p2 = b2_data;
      for (k=0; k<npixels; k++) {
        fval = w1*(*b_p1++)+w2*(*b_p2++);
        *b_vp++ = (unsigned char )(fval/scalef);
      }
    }
    break;

  case MatrixData::DataType::IeeeFloat:
    volume->data_max = v_slices->data_max;
    volume->scale_factor = scalef = v_slices->data_max/32768;
    vp = (short*)volume->data_ptr;
    j = 1;
    f1_data = (float*)v_slices->data_ptr;
    f2_data = (float*)v_slices->data_ptr+npixels;
    for (i=0; i<sz; i++) {
      zloc = i*volume->pixel_size;
      LOG_INFO("Computing slice {} ({}) ({})", i+1, j, zloc);
      while (zloc > slice[j].zloc) {
        f1_data = f2_data;
        if (j < nslices-1) {
          j++;
          f2_data = (float*)v_slices->data_ptr+npixels*j;
        } else {  /*  plane overflow */
          j++;
          slice[j].zloc = slice[j-1].zloc+zsep;
          f2_data = NULL;
          break;
        }
      }
      if (!f2_data) break;    /* plane overflow */
      w2 = (zloc-slice[j-1].zloc)/(slice[j].zloc-slice[j-1].zloc);
      if (!interp) w2 = (int)(w2+0.5);
      w1 = 1.0 - w2;
      f_p1 = f1_data; f_p2 = f2_data;
      for (k=0; k<npixels; k++) {
        fval = w1*(*f_p1++)+w2*(*f_p2++);
        *vp++ = (short)(fval/scalef);
      }
    }
    break;
  default:
    vp = (short*)volume->data_ptr;
    j = 1;
    s1_data = (short*)v_slices->data_ptr;
    s2_data = (short*)v_slices->data_ptr+npixels;
    for (i=0; i<sz; i++) {
      zloc = i*volume->pixel_size;
      LOG_INFO("Computing slice {} ({})", i+1, zloc);
      while (zloc > slice[j].zloc) {
        s1_data = s2_data;
        if (j < nslices-1) {
          j++;
          s2_data = (short*)v_slices->data_ptr+npixels*j;
        } else {  /*  plane overflow */
          j++;
          slice[j].zloc = slice[j-1].zloc+zsep;
          s2_data = NULL;
          break;
        }
      }
      if (!s2_data) break;    /* plane overflow */
      w2 = (zloc-slice[j-1].zloc)/(slice[j].zloc-slice[j-1].zloc);
      if (!interp) w2 = (int)(w2+0.5);
      w1 = 1.0 - w2;
      w1 *= scalef;
      w2 *= scalef;
      s_p1 = s1_data; s_p2 = s2_data;
      for (k=0; k<npixels; k++) {
        fval = w1*(*s_p1++)+w2*(*s_p2++);
        *vp++ = (short)(fval/scalef);
      }
    }
    break;
  }
  free_matrix_data( v_slices);
  return 1;
}

static ecat_matrix::MatrixData *load_volume(ecat_matrix::MatrixFile *matrix_file,int frame, int cubic, int interp) {
  int i=0, ret=0;
  ecat_matrix::MatrixData *mat;
  int matnum;
  float zsep,maxval;
  ecat_matrix::Main_header *mh;
  ecat_matrix::Image_subheader *imh = NULL;
  int nmats, plane, bed, nslices=0;
  float bed_pos[MAX_BED_POS];
  ecat_matrix::MatDirNode *entry;
  ecat_matrix::MatVal matval;
  Tslice slice[MAX_SLICES];
  ecat_matrix::MatrixData *volume;
  int nvoxels;

  volume = (ecat_matrix::MatrixData*)calloc(1,sizeof(ecat_matrix::MatrixData));
  mh = matrix_file->mhptr;
  volume->mat_type = (DataSetType)mh->file_type;
  if (volume->mat_type != MatrixData::DataSetType::Short3dSinogram) 
    imh = (ecat_matrix::Image_subheader*)calloc(1,sizeof(ecat_matrix::Image_subheader));
  memset(bed_pos,0,MAX_BED_POS*sizeof(float));

/* BED OFFSETS CODING ERROR IN ECAT FILES, position 1 not filled */
  for (i=2; i<mh->num_bed_pos; i++)
    bed_pos[i] = mh->bed_offset[i-2];
  if (mh->num_bed_pos>2) bed_pos[1] = bed_pos[2]/2;

  zsep = mh->plane_separation;
  nmats = matrix_file->dirlist->nmats;
  entry = matrix_file->dirlist->first;
  maxval = 0.0;
  for (i=0; i<nmats; i++,entry=entry->next) {
    matnum = entry->matnum;
    mat_numdoc( matnum, &matval);
    plane = matval.plane;
    bed = matval.bed;
    if (matval.frame != frame) continue;
    mat = matrix_read( matrix_file, matnum, MatrixData::DataType::MAT_SUB_HEADER);
    if (mat == NULL)
     LOG_EXIT(matrix_file->fname);
    memcpy(volume, mat, sizeof(ecat_matrix::MatrixData));
    if (imh) memcpy(imh,mat->shptr,sizeof(ecat_matrix::Image_subheader));
    slice[nslices].matnum = matnum;
    slice[nslices].zloc = bed_pos[bed]+(plane-1)*zsep;
    if (volume->data_max > maxval) maxval = volume->data_max;
    nslices++;
    free_matrix_data(mat);
  }
  if (nslices == 0)
  {
    fprintf( stderr, "ERROR...No slices selected\n");
    free_matrix_data(volume);
    return 0;
  }
  volume->data_max = maxval;
  if (volume->data_max > 0) {
    if (volume->data_type == MatrixData::DataType::ByteData) 
      volume->scale_factor = maxval / 256;
    else 
      volume->scale_factor = maxval / 32768;
  } else { 
    volume->scale_factor = 1.0;
  }
  if (imh) {
    imh->scale_factor = volume->scale_factor;
    volume->shptr = (void *)imh;
  }
  if (nslices > 1) {
    ret = load_slices(matrix_file,volume,slice,nslices, cubic, interp);
  } else {
    if (cubic) { 
      ret = load_v_slices(matrix_file,volume,slice, interp);
    } else {
      free_matrix_data(volume);
      return matrix_read(matrix_file, slice[0].matnum,UnknownMatDataType);
    }
  }
  if (!ret) {
    free_matrix_data(volume);
    return 0;
  }
  volume->y_size = volume->pixel_size;
  if (cubic) volume->z_size = volume->pixel_size;
  else volume->z_size = zsep;
  volume->x_origin = 0.5 * volume->xdim * volume->pixel_size;
  volume->y_origin = 0.5 * volume->ydim * volume->y_size;
  volume->z_origin = 0.5 * volume->zdim * volume->z_size;
  nvoxels = volume->xdim * volume->ydim * volume->zdim;
  if (imh) {
    imh->num_dimensions = 3;
    imh->z_dimension = volume->zdim;
    imh->y_pixel_size = volume->y_size;
    imh->z_pixel_size = volume->z_size;
    // hrrt_util::Extrema<unsigned char> extrema;
    if (volume->data_type == MatrixData::DataType::ByteData) {
      // imh->image_min = find_bmin((unsigned char *)volume->data_ptr,nvoxels);
      // imh->image_max = find_bmax((unsigned char *)volume->data_ptr,nvoxels);
      hrrt_util::Extrema<unsigned char> extrema = hrrt_util::find_extrema<unsigned char>(static_cast<unsigned char *>(volume->data_ptr), nvoxels);
      imh->image_min = extrema.min;
      imh->image_max = extrema.max;
    } else {
      // imh->image_min = find_smin((short*)volume->data_ptr,nvoxels);
      // imh->image_max = find_smax((short*)volume->data_ptr,nvoxels);
      hrrt_util::Extrema<short> extrema = hrrt_util::find_extrema<short>(static_cast<short *>(volume->data_ptr), nvoxels);
      imh->image_min = extrema.min;
      imh->image_max = extrema.max;
    }
    // imh->image_min = extrema.min;
    // imh->image_max = extrema.max;
    volume->data_min = imh->image_min * volume->scale_factor;
    volume->data_max = imh->image_max * volume->scale_factor;
  }
  volume->data_size = nvoxels;
  if (volume->data_type != MatrixData::DataType::ByteData) volume->data_size *= sizeof(short);
  volume->matnum = slice[0].matnum;
  return volume;
}

