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

#include <stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<fcntl.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"interfile.hpp"
#include	"machine_indep.h"
#include	"ecat_matrix.hpp"
#include	"num_sort.h"
#include	<unistd.h>

// ahc
#include <stdbool.h>
#include <vector>
#include <string>

#define TRUE 1
#define FALSE 0

// MatrixError g_matrix_error;
// char    ecat_matrix::matrix_errtxt[132];
std::string ecat_matrix::matrix_errtxt;

// Moved to ecat_matrix::matrix_errors_ struct
// std::vector <std::string> matrix_errors =

// std::vector <std::string> matrix_errors =
//   {
//     "No Error",
//     "Read error",
//     "Write error",
//     "Invalid directory block",
//     "ACS file not found",
//     "Interfile open error",
//     "File type not match",
//     "Read from nil filepointer",
//     "No mainheader file object",
//     "Nil subheader pointer",
//     "Nil data pointer",
//     "Matrix not found",
//     "Unknown filetype",
//     "ACS create error",
//     "Bad attribute",
//     "Bad file access mode",
//     "Invalid dimension",
//     "No slices found",
//     "Invalid data type",
//     "Invalid multibed position"
//   };

static int matrix_freelist(ecat_matrix::MatDirList *matdirlist) {
  ecat_matrix::MatDirNode	*node, *next ;

  if (matdirlist == NULL)
    return ecat_matrix::ECATX_OK;
  if (matdirlist->first != NULL)	{
	  node = matdirlist->first ;
	  do {
          next = node->next ;
          free(node) ;
          node = next ;
        }
	  while(next != NULL) ;
	}
  free(matdirlist) ;
  return ecat_matrix::ECATX_OK;
}

static void free_matrix_file(ecat_matrix::MatrixFile *mptr) {
  if (mptr == NULL)
    return;
  if (mptr->mhptr != NULL)
    free(mptr->mhptr) ;
  if (mptr->dirlist != NULL)
    matrix_freelist(mptr->dirlist) ;
  if (mptr->fptr)
    fclose(mptr->fptr);
  if (mptr->fname)
    free(mptr->fname);
  free(mptr);
}

bool is_acs(char *fname) {
  if (strstr(fname, "/sd") == fname)
    return(TRUE) ; 
  else
    return(FALSE) ;
}

int matrix_convert_data(ecat_matrix::MatrixData *matrix, MatrixData::DataType dtype) {
  float minval,maxval;
  float *fdata, scalef;
  short *sdata, smax=32766;  /* not 32767 to avoid rounding problems */
  int i, nvoxels, nblks;
  if (matrix->data_type != MatrixData::DataType::IeeeFloat || dtype != MatrixData::DataType::SunShort)
    return ecat_matrix::ECATX_OK ;	/* dummy for now */
  
  fdata = (float*)matrix->data_ptr;
  nvoxels = matrix->xdim*matrix->ydim*matrix->zdim;
  nblks = (nvoxels*sizeof(short) + ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;
  if ((sdata=(short*)calloc(nblks, ecat_matrix::MatBLKSIZE)) == NULL)
    return 0;
  matrix->data_size = nvoxels*sizeof(short);
  matrix->data_type = MatrixData::DataType::SunShort;
  scalef = matrix->scale_factor;
  minval = maxval = fdata[0];
  for (i=1; i<nvoxels; i++)    {
      if (fdata[i]>maxval)
        maxval = fdata[i];
      else
        if (fdata[i]<minval)
          minval = fdata[i];
  }
  minval *= matrix->scale_factor;
  maxval *= matrix->scale_factor;
  if (fabs(minval) < fabs(maxval))
    scalef = (float)(fabs(maxval)/smax);
  else
    scalef = (float)(fabs(minval)/smax);
  for (i=0; i<nvoxels; i++)
    sdata[i] = (short)(0.5+fdata[i]/scalef);
  free(fdata);
  matrix->data_ptr = (void *)sdata;
  matrix->scale_factor = scalef;
  matrix->data_min = minval;
  matrix->data_max = maxval;
  return 1;
}



static int matrix_write_slice(ecat_matrix::MatrixFile *mptr, int matnum, ecat_matrix::MatrixData *data, int plane) {
  ecat_matrix::MatrixData *slice;
  ecat_matrix::Image_subheader *imh;
  ecat_matrix::MatVal val;
  int i, npixels, nblks, s_matnum;
  short *sdata;
  unsigned char * bdata;
  int	ret;

  switch(mptr->mhptr->file_type) {
  case MatrixData::DataSetType::PetImage :
    if (data->data_type ==  MatrixData::DataType::ByteData) {
      /*				fprintf(stderr,"Only short data type supported in V6\n");*/
      // g_matrix_error = ecat_matrix::MatrixError::INVALID_DATA_TYPE ;
      LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::INVALID_DATA_TYPE));
      return ecat_matrix::ECATX_ERROR;
    }
    mat_numdoc(matnum,&val);
    slice = (ecat_matrix::MatrixData*)malloc(sizeof(ecat_matrix::MatrixData));
    if (!slice )
      return(ecat_matrix::ECATX_ERROR) ;

    imh = (ecat_matrix::Image_subheader*)calloc(1,ecat_matrix::MatBLKSIZE);
    if (!imh ) {
      free( slice );
      return(ecat_matrix::ECATX_ERROR) ;
    }
    memcpy(slice,data,sizeof(ecat_matrix::MatrixData));
    memcpy(imh,data->shptr, sizeof(ecat_matrix::Image_subheader));
    slice->shptr = (void *)imh;
    slice->zdim = imh->z_dimension = 1;
    npixels = slice->xdim*slice->ydim;
    nblks = (npixels*2 + ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;
    slice->data_ptr = (void *)calloc(nblks,ecat_matrix::MatBLKSIZE);
    if (!slice->data_ptr ) {
      free( slice );
      free( imh );
      return(ecat_matrix::ECATX_ERROR) ;
    }
    slice->data_size = nblks*ecat_matrix::MatBLKSIZE;
    if (data->data_type ==  MatrixData::DataType::ByteData) {
      bdata = (unsigned char *)(data->data_ptr + (plane - 1) * npixels);
      // imh->image_min = find_bmin(bdata,npixels);
      // imh->image_max = find_bmax(bdata,npixels);
      hrrt_util::Extrema<unsigned char> extrema = hrrt_util::find_extrema<unsigned char>(bdata, npixels);
      imh->image_min = extrema.min;
      imh->image_max = extrema.max;
      sdata = (short *)slice->data_ptr;
      for (i=0; i<npixels; i++)  
        sdata[i] = bdata[i];
    } else {
      sdata = (short *)(data->data_ptr + (plane - 1) * npixels * 2);
      // imh->image_min = find_smin(sdata,npixels);
      // imh->image_max = find_smax(sdata,npixels);
      hrrt_util::Extrema<short> extrema = hrrt_util::find_extrema<short>(bdata, npixels);
      imh->image_min = extrema.min;
      imh->image_max = extrema.max;
      memcpy(slice->data_ptr, sdata, npixels * 2);
    }
    s_matnum = ecat_matrix::mat_numcod(val.frame,plane,val.gate,val.data,val.bed);
    ret = matrix_write(mptr, s_matnum,slice);
    free_matrix_data(slice);
    return( ret );
  case MatrixData::DataSetType::PetVolume :
  case MatrixData::DataSetType::ByteVolume :
  case MatrixData::DataSetType::InterfileImage:
    /*			fprintf(stderr,
                "matrix_slice_write : ecat_matrix::Main_header file_type should be PetImage\n");
    */
    // g_matrix_error = ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH;
        LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH));

    return ecat_matrix::ECATX_ERROR;
  default:
    /*			fprintf(stderr,"V7 to V6 conversion only supported for images\n");*/
    // g_matrix_error = ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH;
      LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH));
    return ecat_matrix::ECATX_ERROR;
  }
}

// find_bmax, find_bmin, find_smax, find_smin, find_imax, find_imin, find_fmin, find_fmax moved to <T> hrrt_util::find_extrema()

// unsigned char  find_bmax(const unsigned char  *bdata, int nvals) {
//   int i;
//   unsigned char  bmax = bdata[0];
//   for (i=1; i<nvals; i++)
//     if (bdata[i] > bmax) bmax = bdata[i];
//   return bmax;
// }

// unsigned char  find_bmin(const unsigned char  *bdata, int nvals) {
//   int i;
//   unsigned char  bmin = bdata[0];
//   for (i=1; i<nvals; i++)
//     if (bdata[i] < bmin) 
//       bmin = bdata[i];
//   return bmin;
// }

// short find_smax( const short *sdata, int nvals) {
//   int i;
//   short smax = sdata[0];
//   for (i=1; i<nvals; i++)
//     if (sdata[i] > smax) 
//       smax = sdata[i];
//   return smax;
// }

// short find_smin( const short *sdata, int nvals) {
//   int i;
//   short  smin = sdata[0];
//   for (i=1; i<nvals; i++)
//     if (sdata[i] < smin) 
//       smin = sdata[i];
//   return smin;
// }

// int find_imax( const int *idata, int nvals) {
//   int i, imax=idata[0];
//   for (i=1; i<nvals; i++)
//     if (idata[i]>imax) 
//       imax = idata[i];
//   return imax;
// }

// int find_imin( const int *idata, int nvals) {
//   int i, imin=idata[0];
//   for (i=1; i<nvals; i++)
//     if (idata[i]<imin) 
//       imin = idata[i];
//   return imin;
// }

// float find_fmin( const float *fdata, int nvals) {
//   int i;
//   float fmin = fdata[0];
//   for (i=1; i<nvals; i++)
//     if (fdata[i]<fmin) fmin = fdata[i];
//   return fmin;
// }

// float find_fmax( const float *fdata, int nvals) {
//   int i;
//   float fmax = fdata[0];
//   for (i=1; i<nvals; i++)
//     if (fdata[i]>fmax) fmax = fdata[i];
//   return fmax;
// }

int read_host_data(ecat_matrix::MatrixFile *mptr, int matnum, ecat_matrix::MatrixData *data, MatrixData::DataType dtype) {
  ecat_matrix::MatDir matdir;
  int	 nblks, data_size ;
  ecat_matrix::Scan3D_subheader *scan3Dsub ;
  int sx,sy,sz;
  int elem_size= 2;

  set_matrix_no_error();
  data->matnum = matnum;
  data->matfile = mptr;
  data->mat_type = mptr->mhptr->file_type;

  // ahc why not use file_format to indicate the input file type?
  if (mptr->analyze_hdr)  /* read interfile */
    return analyze_read(mptr, matnum, data, dtype);

  if (!mptr->interfile_header.empty())  /* read interfile */
    return interfile_read(mptr, matnum, data, dtype);

  if (matrix_find(mptr,matnum,&matdir) == ecat_matrix::ECATX_ERROR) {
      // g_matrix_error = ecat_matrix::MatrixError::MATRIX_NOT_FOUND ;
          LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::MATRIX_NOT_FOUND));

      return(ecat_matrix::ECATX_ERROR) ;
	}
  nblks = matdir.endblk - matdir.strtblk ;
  data_size = data->data_size = 512*nblks;
  if (dtype != MatrixData::DataType::MAT_SUB_HEADER) {
	  data->data_ptr = (void *) calloc(1, data_size) ;
	  if (data->data_ptr == NULL) {
          return(ecat_matrix::ECATX_ERROR) ;
        }
	} 
  switch(mptr->mhptr->file_type) {
    case MatrixData::DataSetType::Sinogram :
      ecat_matrix::Scan_subheader *scansub (ecat_matrix::Scan_subheader *) data->shptr ;
      mat_read_scan_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk,                              scansub) ;
      data->data_type = scansub->data_type ;
      data->xdim = scansub->num_r_elements ;
      data->ydim = scansub->num_angles ;
      data->zdim = scansub->num_z_elements ;
      data->scale_factor = scansub->scale_factor ;
      data->pixel_size = scansub->x_resolution;
      data->data_max = scansub->scan_max * scansub->scale_factor ;
      if (dtype == MatrixData::DataType::MAT_SUB_HEADER) 
        break;
      read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks,                       data->data_ptr, scansub->data_type) ;
      break ;
    case MatrixData::DataSetType::Short3dSinogram :
    case MatrixData::DataSetType::Float3dSinogram :
      ecat_matrix::Scan3D_subheader *scan3Dsub = (ecat_matrix::Scan3D_subheader *) data->shptr ;
      mat_read_Scan3D_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk,                                scan3Dsub) ;
      data->data_type = scan3Dsub->data_type ;
      data->scale_factor = scan3Dsub->scale_factor ;
      data->pixel_size = scan3Dsub->x_resolution;
      data->y_size = data->pixel_size;
      data->z_size = scan3Dsub->z_resolution;
      data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor ;
      sx = data->xdim = scan3Dsub->num_r_elements ;
      sz = data->ydim = scan3Dsub->num_angles ;
      sy = data->zdim = scan3Dsub->num_z_elements[0] ;
      if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
        break;

      read_matrix_data(mptr->fptr, matdir.strtblk+2, nblks-1,                       data->data_ptr, scan3Dsub->data_type) ;
      if (scan3Dsub->data_type == MatrixData::DataType::SunShort) {
        scan3Dsub->scan_max = find_smax((short*)data->data_ptr, sx * sy * sz);
        scan3Dsub->scan_min = find_smin((short*)data->data_ptr, sx * sy * sz);
        data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
        data->data_min = scan3Dsub->scan_min * scan3Dsub->scale_factor;
      } else {
        data->data_max = find_fmax((float*)data->data_ptr, sx * sy * sz);
        data->data_min = find_fmin((float*)data->data_ptr, sx * sy * sz);
      }
      break ;
    case MatrixData::DataSetType::ByteVolume :
    case MatrixData::DataSetType::PetImage :
    case MatrixData::DataSetType::PetVolume :
    case MatrixData::DataSetType::InterfileImage:
        ecat_matrix::Image_subheader *imagesub = (ecat_matrix::Image_subheader *) data->shptr ;
      mat_read_image_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk, imagesub);
      data->data_type = imagesub->data_type ;
      sx = data->xdim = imagesub->x_dimension ;
      sy = data->ydim = imagesub->y_dimension ;
      data->scale_factor = imagesub->scale_factor ;
      if (data->data_type==MatrixData::DataType::ByteData || data->data_type==MatrixData::DataType::Color_8)
        elem_size =1 ;
      else 
        elem_size = 2;
      sz = data->zdim = imagesub->z_dimension ;
      if (sx*sy*elem_size == 0 ) {
        // g_matrix_error = ecat_matrix::MatrixError::INVALID_DIMENSION;
              LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::INVALID_DIMENSION));
        return( ecat_matrix::ECATX_ERROR );
      }
      if (sz > data_size/(sx*sy*elem_size))
        sz = data->zdim = imagesub->z_dimension = data_size/(sx*sy*elem_size);
      /* fix inconsistent file types */
      if (data->zdim > 1) {
        if (data->data_type == MatrixData::DataType::ByteData) {
          data->mat_type = MatrixData::DataSetType::ByteVolume;
        } else {
          data->mat_type = MatrixData::DataSetType::PetVolume;
        }
      } else {
        data->mat_type = MatrixData::DataSetType::PetImage;
      }

      data->pixel_size = imagesub->x_pixel_size;
      data->y_size = imagesub->y_pixel_size;
      /* if imagesub->y_pixel_size not filled assume square pixels */
      if (data->y_size <= 0)
        data->y_size = imagesub->x_pixel_size;
      data->z_size = imagesub->z_pixel_size;
      /* if imagesub->z_pixel_size not filled use palne separation */
      if (data->z_size <= 0)
        data->z_size = mptr->mhptr->plane_separation;
      data->data_max = imagesub->image_max * imagesub->scale_factor;
      /* KT added next 3 lines */
      data->z_origin = imagesub->z_offset;
      data->y_origin = imagesub->y_offset;
      data->x_origin = imagesub->x_offset;
      if (dtype == MatrixData::DataType::MAT_SUB_HEADER) 
        break;
      read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, imagesub->data_type) ;
      int npix = sx * sy * sz;
      if (imagesub->data_type == MatrixData::DataType::ByteData) {
        // imagesub->image_max = find_bmax((unsigned char *)data->data_ptr, sx * sy * sz);
        // imagesub->image_min = find_bmin((unsigned char *)data->data_ptr, sx * sy * sz);
      hrrt_util::Extrema<unsigned char> extrema = hrrt_util::find_extrema<unsigned char>(static_cast<unsigned char *>(data->data_ptr), npix);
      imagesub->image_min = extrema.min;
      imagesub->image_max = extrema.max;
      } else {
        // imagesub->image_max = find_smax((short*)data->data_ptr, sx * sy * sz);
        // imagesub->image_min = find_smin((short*)data->data_ptr, sx * sy * sz);
      hrrt_util::Extrema<short> extrema = hrrt_util::find_extrema<short>(static_cast<short *>(data->data_ptr), npix);
      imagesub->image_min = extrema.min;
      imagesub->image_max = extrema.max;
      }
      data->data_max = imagesub->image_max * imagesub->scale_factor;
      data->data_min = imagesub->image_min * imagesub->scale_factor;
      break ;
    case MatrixData::DataSetType::AttenCor :
        ecat_matrix::Attn_subheader *attnsub = (ecat_matrix::Attn_subheader *) data->shptr ;
      mat_read_attn_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk,attnsub);
      data->data_type = attnsub->data_type ;
      sx = data->xdim = attnsub->num_r_elements ;
      sz = data->ydim = attnsub->num_angles ;
      sy = data->zdim = attnsub->z_elements[0] ;
      data->scale_factor = attnsub->scale_factor;
      data->pixel_size = attnsub->x_resolution;
      if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
        break;
      read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, attnsub->data_type);
      // data->data_max = find_fmax((float*)data->data_ptr, sx * sy * sz);
      hrrt_util::Extrema<float> extrema = hrrt_util::find_extrema<float>(static_cast<float *>(data->data_ptr), sx * sy * sz);
      data->data_max = extrema.max;

      break ;
    case MatrixData::DataSetType::Normalization :
        ecat_matrix::Norm_subheader *normsub = (ecat_matrix::Norm_subheader *) data->shptr ;
      mat_read_norm_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk,normsub);
      data->data_type = normsub->data_type ;
      data->xdim = normsub->num_r_elements ;
      data->ydim = normsub->num_angles ;
      data->zdim = normsub->num_z_elements ;
      data->scale_factor = normsub->scale_factor ;
      if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
        break;
      read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, normsub->data_type) ;
      // data->data_max = data->scale_factor * find_fmax((float*)data->data_ptr, data->xdim * data->ydim);
      hrrt_util::Extrema<float> extrema = hrrt_util::find_extrema<float>(static_cast<float *>(data->data_ptr), data->xdim * data->ydim);
      data->data_max = data->scale_factor * extrema.max;

      break ;
    case MatrixData::DataSetType::Norm3d :
      ecat_matrix::Norm3D_subheader *norm3d = (ecat_matrix::Norm3D_subheader *) data->shptr ;
      mat_read_norm3d_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk,norm3d);
      data->data_type = norm3d->data_type ;
      data->xdim = norm3d->num_r_elements;	/* 336 */
      data->ydim = norm3d->crystals_per_ring;	/* 784 */
      data->zdim = norm3d->num_crystal_rings;	/* 24 */
      data->scale_factor = 1.0;
      if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
        break;
      /*
		336*(1+7) + 24*784
		336*(1+7) =
		radial elements (plane geometry + Crystal Interference) Corrections
		24*784 = cristals efficiencies
      */
      read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks,
                       data->data_ptr, norm3d->data_type) ;
      break ;
    default :
      // g_matrix_error = ecat_matrix::MatrixError::UNKNOWN_FILE_TYPE ;
      LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::UNKNOWN_FILE_TYPE));

      return(ecat_matrix::ECATX_ERROR) ;
      break ;
	}
  return(ecat_matrix::ECATX_OK) ;
}

void set_matrix_no_error() {
  // g_matrix_error = ecat_matrix::MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();
}

int matrix_write(ecat_matrix::MatrixFile *mptr, int matnum, ecat_matrix::MatrixData *data) {
  ecat_matrix::MatDir matdir, dir_entry ;
  ecat_matrix::Scan3D_subheader *scan3Dsub = NULL;
  ecat_matrix::Scan_subheader *scansub ;
  ecat_matrix::Image_subheader *imagesub ;
  ecat_matrix::Attn_subheader *attnsub ;
  ecat_matrix::Norm_subheader *normsub ;
  int	status, blkno, nblks ;

  set_matrix_no_error();
  status = ecat_matrix::ECATX_OK ;
  nblks = (data->data_size+511)/512;
        
  /* 3D sinograms subheader use one more block */
  if (mptr->mhptr->file_type == Short3dSinogram  ||      mptr->mhptr->file_type == Float3dSinogram) 
    nblks += 1;

  if (matrix_find(mptr, matnum, &matdir) == ecat_matrix::ECATX_ERROR)	{
      blkno = mat_enter(mptr->fptr, mptr->mhptr, matnum, nblks) ;
      if (blkno == ecat_matrix::ECATX_ERROR )
        return( ecat_matrix::ECATX_ERROR );
      dir_entry.matnum = matnum ;
      dir_entry.strtblk = blkno ;
      dir_entry.endblk = dir_entry.strtblk + nblks - 1 ;
      dir_entry.matstat = 1 ;
      insert_mdir(&dir_entry, mptr->dirlist) ;
      matdir = dir_entry ;
	}

  switch(mptr->mhptr->file_type) {
    case MatrixData::DataSetType::Float3dSinogram :
    case MatrixData::DataSetType::Short3dSinogram :
      scan3Dsub = (ecat_matrix::Scan3D_subheader*) data->shptr;
      if (mat_write_Scan3D_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk, scan3Dsub) == ecat_matrix::ECATX_ERROR) 
        return ecat_matrix::ECATX_ERROR;
      if (write_matrix_data(mptr->fptr, matdir.strtblk+2, nblks, data->data_ptr, scan3Dsub->data_type) == ecat_matrix::ECATX_ERROR) 
        return ecat_matrix::ECATX_ERROR;
      break;
    case MatrixData::DataSetType::Sinogram :
      scansub = (ecat_matrix::Scan_subheader *) data->shptr ;
      if (mat_write_scan_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, scansub) == ecat_matrix::ECATX_ERROR )
        return( ecat_matrix::ECATX_ERROR );
      status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, scansub->data_type) ;
      if (status == ecat_matrix::ECATX_ERROR )
        return( ecat_matrix::ECATX_ERROR );
      break ;
    case MatrixData::DataSetType::ByteVolume :
    case MatrixData::DataSetType::PetImage :
    case MatrixData::DataSetType::PetVolume :
    case MatrixData::DataSetType::InterfileImage:
      imagesub = (ecat_matrix::Image_subheader *) data->shptr ;
      if (imagesub == NULL) {
        imagesub = (ecat_matrix::Image_subheader *) calloc(1, ecat_matrix::MatBLKSIZE);
        data->shptr = (void *)imagesub;
      }							/* use ecat_matrix::MatrixData info */
      imagesub->x_pixel_size = data->pixel_size;
      imagesub->y_pixel_size = data->y_size;
      imagesub->z_pixel_size = data->z_size;
      imagesub->num_dimensions = 3;
      imagesub->x_dimension = data->xdim;
      imagesub->y_dimension = data->ydim;
      imagesub->z_dimension = data->zdim;
      imagesub->image_max = (int)(data->data_max/data->scale_factor);
      imagesub->image_min = (int)(data->data_min/data->scale_factor);
      imagesub->scale_factor = data->scale_factor;
      imagesub->data_type = data->data_type;

      if (mat_write_image_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk, imagesub) == ecat_matrix::ECATX_ERROR )
        return( ecat_matrix::ECATX_ERROR );
      status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, imagesub->data_type) ;
      if (status == ecat_matrix::ECATX_ERROR ) 
        return( ecat_matrix::ECATX_ERROR );
      break ;
    case MatrixData::DataSetType::AttenCor :
      attnsub = (ecat_matrix::Attn_subheader *) data->shptr ;
      if (mat_write_attn_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, attnsub) == ecat_matrix::ECATX_ERROR )
        return( ecat_matrix::ECATX_ERROR );
      status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, attnsub->data_type);
      if (status == ecat_matrix::ECATX_ERROR ) 
        return( ecat_matrix::ECATX_ERROR );
      break ;
    case MatrixData::DataSetType::Normalization :
      normsub = (ecat_matrix::Norm_subheader *) data->shptr ;
      if (mat_write_norm_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, normsub) == ecat_matrix::ECATX_ERROR )
        return( ecat_matrix::ECATX_ERROR );
      status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, normsub->data_type) ;
      if (status == ecat_matrix::ECATX_ERROR ) 
        return( ecat_matrix::ECATX_ERROR );
      break ;
    default :	/* default treated as sinogram */
      scansub = (ecat_matrix::Scan_subheader *) data->shptr ;
      if (mat_write_scan_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, scansub) == ecat_matrix::ECATX_ERROR )
        return( ecat_matrix::ECATX_ERROR );

      status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, scansub->data_type) ;
      if (status == ecat_matrix::ECATX_ERROR ) 
        return( ecat_matrix::ECATX_ERROR );
      break ;
	}
  return(status) ;
}

int matrix_find(ecat_matrix::MatrixFile *matfile, int matnum, ecat_matrix::MatDir *matdir) {
  ecat_matrix::MatDirNode	*node ;

  set_matrix_no_error();
  if (matfile == NULL)
    return(ecat_matrix::ECATX_ERROR) ;
  if (matfile->dirlist == NULL)
    return(ecat_matrix::ECATX_ERROR) ;	
  node = matfile->dirlist->first ;
  while (node != NULL)	{
      if (node->matnum == matnum)        {
          matdir->matnum = node->matnum ;
          matdir->strtblk = node->strtblk ;
          matdir->endblk = node->endblk ;
          matdir->matstat = node->matstat ;
          break ;
        }
      node = node->next ;
	}
  if (node != NULL)
    return(ecat_matrix::ECATX_OK) ;
  else return(ecat_matrix::ECATX_ERROR) ;
}
	


int insert_mdir(ecat_matrix::MatDir	*matdir, ecat_matrix::MatDirList	*dirlist) { 
  set_matrix_no_error();
  if (dirlist == NULL)	{
      dirlist = (ecat_matrix::MatDirList *) malloc(sizeof(ecat_matrix::MatDirList)) ;
      if (dirlist == NULL)
        return(ecat_matrix::ECATX_ERROR) ;
      dirlist->nmats = 0 ;
      dirlist->first = NULL ;
      dirlist->last = NULL ;
	}
    ecat_matrix::MatDirNode  *node = (ecat_matrix::MatDirNode *) malloc(sizeof(ecat_matrix::MatDirNode)) ;
  if (node == NULL)
    return(ecat_matrix::ECATX_ERROR) ;

  node->matnum = matdir->matnum ;
  node->strtblk = matdir->strtblk ;
  node->endblk = matdir->endblk ;
  node->matstat = matdir->matstat;
  node->next = NULL ;

  if (dirlist->first == NULL)	{ /* if list was empty, add first node */
      dirlist->first = node ;
      dirlist->last = node ;
      dirlist->nmats = 1 ;
	} else {
      (dirlist->last)->next = node ;
      dirlist->last = node ;
      ++(dirlist->nmats) ;
	}
  return ecat_matrix::ECATX_OK;
}

void free_matrix_data(ecat_matrix::MatrixData	*data) {
  if (data != NULL)	{
      if (data->data_ptr != NULL) 
        free(data->data_ptr) ;
      if (data->shptr != NULL) 
        free(data->shptr) ;
      free(data) ;
	}
}

void matrix_perror( const char* s) {
  LOG_ERROR("XXX should not be calling matrix_perror: {} XXX", s)
  // if (g_matrix_error)
  //   fprintf( stderr, "%s: %s\n", s, ecat_matrix::matrix_errors_[g_matrix_error]);
  // else 
  //   perror(s);
}

ecat_matrix::Main_header *matrix_init_main_header( char *fname, DataSetType ftype, ecat_matrix::Main_header *mh_proto) {
  ecat_matrix::Main_header *mhptr;

  set_matrix_no_error();
  mhptr = (ecat_matrix::Main_header*) calloc( 1, sizeof(ecat_matrix::Main_header));
  if (!mhptr )
    return( NULL );
  if (mh_proto)
    memcpy(mhptr, mh_proto, sizeof(ecat_matrix::Main_header));
  mhptr->file_type = ftype;
  strncpy( mhptr->original_file_name, fname, 20);
  return mhptr;
}

void matrix_free( ecat_matrix::MatrixData *matrix) {
  if (matrix->shptr) 
    free( matrix->shptr);
  if (matrix->data_ptr) 
    free( matrix->data_ptr);
  free( matrix);
}

int convert_float_scan( ecat_matrix::MatrixData *scan, float *fdata) {
  int i, nvals, tot;
  float fmax, scale;
  short int *sdata;
  ecat_matrix::Scan_subheader *ssh;

  set_matrix_no_error();
  if (scan->data_ptr)
    free(scan->data_ptr);
  nvals = scan->xdim*scan->ydim;
  sdata = (short int*) malloc( nvals*sizeof(short int));
  if (sdata == NULL)
    return ecat_matrix::ECATX_ERROR;
  scan->data_ptr = (void *) sdata;
  scan->data_size = nvals*sizeof(short int);
  fmax = (float)(fabs(*fdata));
  for (i=0; i<nvals; i++)
    if (fabs(fdata[i]) > fmax) fmax = (float)fabs(fdata[i]);
  scale = 1.0f;
  if (fmax > 0.0f) 
    scale = 32767.0f/fmax;
  tot = 0;
  for (i=0; i<nvals; i++)	{
	  sdata[i] = (short)(scale*fdata[i] + 0.5f);
	  tot += sdata[i];
	}
  scan->scale_factor = 1.0f/scale;
  ssh = (ecat_matrix::Scan_subheader*) scan->shptr;
  ssh->scan_min = 0;
  ssh->scan_max = (short)(fmax*scale);
  ssh->num_r_elements = scan->xdim;
  ssh->num_angles = scan->ydim;
  ssh->net_trues = tot;
  ssh->scale_factor = 1.0f/scale;
  ssh->x_resolution = scan->pixel_size;
  return ecat_matrix::ECATX_OK;
}

ecat_matrix::MatrixData *ecat_matrix::read_scan(ecat_matrix::MatrixFile *mptr, int matnum, MatrixData::DataType dtype, int segment) {
  int i, nblks, plane_size;
  int sx, sy, sz;
  ecat_matrix::MatrixData *data;
  ecat_matrix::MatDir matdir;
  ecat_matrix::Scan3D_subheader *scan3Dsub;
  ecat_matrix::Attn_subheader *attnsub;
  int status, group;
  unsigned z_elements;
  long file_pos=0;

  /* Scan3D and Atten storage:
     storage_order = 0 : (((projs x z_elements)) x num_angles) x Ringdiffs
nnnn     storage_order != 0 : (((projs x num_angles)) x z_elements)) x Ringdiffs
    
  */

  set_matrix_no_error();
  if (matrix_find(mptr,matnum,&matdir) == ecat_matrix::ECATX_ERROR)
    return NULL;

  if ((data = (ecat_matrix::MatrixData *) calloc( 1, sizeof(ecat_matrix::MatrixData))) != NULL) {
    if ( (data->shptr = (void *) calloc(2, ecat_matrix::MatBLKSIZE)) == NULL) {
      free(data);
      return NULL;
    }
  } else return NULL;

  data->matnum = matnum;
  data->matfile = mptr;
  data->mat_type = mptr->mhptr->file_type;
  nblks = matdir.endblk - matdir.strtblk;
  group=abs(segment);
  switch(mptr->mhptr->file_type) {
  case MatrixData::DataSetType::Float3dSinogram :
  case MatrixData::DataSetType::Short3dSinogram :
    scan3Dsub = (ecat_matrix::Scan3D_subheader *)data->shptr;
    status = mat_read_Scan3D_subheader(mptr->fptr, mptr->mhptr,
                                       matdir.strtblk, scan3Dsub);
    if (status == ecat_matrix::ECATX_ERROR) {
      free_matrix_data(data);	
      return NULL;
    }
    file_pos = (matdir.strtblk+1)*ecat_matrix::MatBLKSIZE;
    data->data_type = scan3Dsub->data_type;
    data->scale_factor = scan3Dsub->scale_factor;
    data->pixel_size = scan3Dsub->x_resolution;
    sx = data->xdim = scan3Dsub->num_r_elements;
    z_elements = scan3Dsub->num_z_elements[group];
    if (group > 0) z_elements /= 2;
    if (scan3Dsub->storage_order == 0) {
      data->z_size = data->pixel_size;
      data->y_size = mptr->mhptr->plane_separation;
      sy = data->ydim = z_elements;
      sz = data->zdim = scan3Dsub->num_angles;
      plane_size = sx*sz*sizeof(short);
    } else {
      data->y_size = data->pixel_size;
      data->z_size = mptr->mhptr->plane_separation;
      sy = data->ydim = scan3Dsub->num_angles;
      sz = data->zdim = z_elements;
      plane_size = sx*sy*sizeof(short);
    }
    if (mptr->mhptr->file_type == MatrixData::DataSetType::Float3dSinogram) plane_size *= 2;
    data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
    if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
      break;
    for (i=0; i<group; i++)
      file_pos += scan3Dsub->num_z_elements[i]*plane_size;
    if (segment < 0) file_pos += z_elements*plane_size;
    data->data_size = z_elements*plane_size;
    nblks = (data->data_size+511)/512;
                                    
    if ((data->data_ptr = (void *)calloc(nblks,512)) == NULL ||
        fseek(mptr->fptr,file_pos,0) == -1 ||
        /* ahc all Linux fseek now 64 bits */
        /* fseeko(mptr->fptr,file_pos,0) == -1 || */

        fread(data->data_ptr, plane_size, z_elements, mptr->fptr)
        != z_elements ||
        file_data_to_host(data->data_ptr,nblks,data->data_type) == ecat_matrix::ECATX_ERROR) {
      free_matrix_data(data);
      return NULL;
    }
    if (mptr->mhptr->file_type == MatrixData::DataSetType::Short3dSinogram) {
      // scan3Dsub->scan_max = find_smax((short*)data->data_ptr, sx * sy * sz);
      // scan3Dsub->scan_min = find_smin((short*)data->data_ptr, sx * sy * sz);

      hrrt_util::Extrema<short> extrema = hrrt_util::find_extrema<short>(static_cast<short *>(data->data_ptr), sx * sy * sz);
      scan3Dsub->scan_max = extrema.min;
      scan3Dsub->scan_max = extrema.max;

      data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
      data->data_min = scan3Dsub->scan_min * scan3Dsub->scale_factor;

    } else {
      // data->data_max = find_fmax((float*)data->data_ptr, sx * sy * sz);
      // data->data_min = find_fmin((float*)data->data_ptr, sx * sy * sz);
      hrrt_util::Extrema<float> extrema = hrrt_util::find_extrema<float>(static_cast<float *>(data->data_ptr), sx * sy * sz);
      data->data_min = extrema.min;
      data->data_max = extrema.max;
    }
    break;
  case MatrixData::DataSetType::AttenCor :
    attnsub = (ecat_matrix::Attn_subheader *) data->shptr;
    if (mat_read_attn_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk,
                                attnsub) == ecat_matrix::ECATX_ERROR) {
      free_matrix_data(data);
      return NULL;
    }
    file_pos = matdir.strtblk*ecat_matrix::MatBLKSIZE;
    data->data_type = attnsub->data_type;
    data->scale_factor = attnsub->scale_factor;
    data->pixel_size = attnsub->x_resolution;
    sx = data->xdim = attnsub->num_r_elements;
    z_elements = attnsub->z_elements[group];
    if (group > 0) z_elements /= 2;
    if (attnsub->storage_order == 0) {
      data->y_size = mptr->mhptr->plane_separation;
      data->z_size = data->pixel_size;
      sy = data->ydim = z_elements;
      sz = data->zdim = attnsub->num_angles;
    } else {
      data->z_size = mptr->mhptr->plane_separation;
      data->y_size = data->pixel_size;
      sy = data->ydim = attnsub->num_angles;
      sz = data->zdim = z_elements;
    }
    data->data_max = attnsub->attenuation_max;
    if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
      break;
    plane_size = sx*attnsub->num_angles*sizeof(float);
    for (i=0; i<group; i++)
      file_pos += attnsub->z_elements[i]*plane_size;
    if (segment < 0) file_pos += z_elements*plane_size;
    if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
      break;
    data->data_size = z_elements*plane_size;
    nblks = (data->data_size+511)/512;


    if ((data->data_ptr = (void *)calloc(nblks,512)) == NULL ||
        /* ahc */
        /*         fseeko64(mptr->fptr,file_pos,0) == -1 || */
        fseeko(mptr->fptr,file_pos,0) == -1 ||

        fread(data->data_ptr,plane_size,z_elements,mptr->fptr) !=
        z_elements ||
        file_data_to_host(data->data_ptr,nblks,data->data_type) == ecat_matrix::ECATX_ERROR) {
      free_matrix_data(data);
      return NULL;
    }
    // data->data_max = find_fmax((float*)data->data_ptr, sx * sy * sz);
      hrrt_util::Extrema<float> extrema = hrrt_util::find_extrema<float>(static_cast<float *>(data->data_ptr), sx * sy * sz);
      data->data_max = extrema.max;
    break;
  default:
    // g_matrix_error = ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH;
        LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH));

    return NULL;
  }
  return data;
}

int mh_update(ecat_matrix::MatrixFile *file) {
  ecat_matrix::MatDirNode     *node;
  ecat_matrix::MatVal   val;
  int            *frames, *planes, *gates, *beds;
  int             num_frames = 0, num_planes = 0, num_gates = 0,
    num_beds = 0;
  ecat_matrix::Main_header    *mh = file->mhptr;
  ecat_matrix::MatDirList     *dir_list = file->dirlist;
  int             mod = 0, nmats;					/* correction  7/12/99 MS */
  if (file->dirlist == NULL || file->dirlist->nmats == 0)
    return ecat_matrix::ECATX_OK;
  frames = planes = gates = beds = NULL;
  nmats = file->dirlist->nmats;
  frames = (int *) calloc(nmats, sizeof(int));
  planes = (int *) calloc(nmats, sizeof(int));
  gates = (int *) calloc(nmats, sizeof(int));
  beds = (int *) calloc(nmats, sizeof(int));
  node = file->dirlist->first;
  while (node != NULL) {
    mat_numdoc(node->matnum, &val);
    if (bsearch(&val.frame, frames, num_frames, sizeof(int), compare_int) ==
        NULL) {
      frames[num_frames++] = val.frame;
      sort_int(frames, num_frames);
    }
    if (bsearch(&val.plane, planes, num_planes, sizeof(int), compare_int) ==
        NULL) {
      planes[num_planes++] = val.plane;
      sort_int(planes, num_planes);
    }
    if (bsearch(&val.gate, gates, num_gates, sizeof(int), compare_int) ==
        NULL) {
      gates[num_gates++] = val.gate;
      sort_int(gates, num_gates);
    }
    if (bsearch(&val.bed, beds, num_beds, sizeof(int), compare_int) ==
        NULL) {
      beds[num_beds++] = val.bed;
      sort_int(beds, num_beds);
    }
    node = node->next;
  }
  free(frames);
  free(planes);
  free(gates);
  free(beds);
  num_beds--;		/* ??? CTI convention ==> nombre d'offsets */
  if (mh->num_frames != num_frames) {
    mh->num_frames = num_frames;
    mod++;
  }
  /*	if (mh->file_type == PetImage && mh->num_planes != num_planes) {
		mh->num_planes = num_planes;
		mod++;
        }
  */
  if (mh->num_gates != num_gates) {
    mh->num_gates = num_gates;
    mod++;
  }
  if (mh->num_bed_pos != num_beds) {
    mh->num_bed_pos = num_beds;
    mod++;
  }
  if (mod > 0)
    return mat_write_main_header(file->fptr, mh);
  return ecat_matrix::ECATX_OK;
}

MatDirBlk *mat_rdirblk(ecat_matrix::MatrixFile *t_infile, int blknum) {
  MatDirBlk      *matdirblk;
  int             i, j, err, ndirs;
  int             dirbufr[ecat_matrix::MatBLKSIZE / 4];
  // FILE           *fptr = t_infile->fptr;

  set_matrix_no_error();
  matdirblk = (MatDirBlk *) malloc(ecat_matrix::MatBLKSIZE);
  if (matdirblk == NULL)
    return (NULL);
  err = read_matrix_data(t_infile->fptr, blknum, 1, (char *) dirbufr, SunLong);
  if (err == ecat_matrix::ECATX_ERROR) {
    free(matdirblk);
    return (NULL);
  }
  matdirblk->nfree = dirbufr[0];
  matdirblk->nextblk = dirbufr[1];
  matdirblk->prvblk = dirbufr[2];
  matdirblk->nused = dirbufr[3];

  if (matdirblk->nused > 31) {
    // g_matrix_error = ecat_matrix::MatrixError::INVALID_DIRBLK;
          LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::INVALID_DIRBLK));

    free(matdirblk);
    return (NULL);
  }
  ndirs = (ecat_matrix::MatBLKSIZE / 4 - 4) / 4;
  for (i = 0; i < ndirs; i++) {
    matdirblk->matdir[i].matnum  = 0;
    matdirblk->matdir[i].strtblk = 0;
    matdirblk->matdir[i].endblk  = 0;
    matdirblk->matdir[i].matstat = 0;
  }

  for (i = 0; i < matdirblk->nused; i++) {
    j = i + 1;
    matdirblk->matdir[i].matnum  = dirbufr[j * 4 + 0];
    matdirblk->matdir[i].strtblk = dirbufr[j * 4 + 1];
    matdirblk->matdir[i].endblk  = dirbufr[j * 4 + 2];
    matdirblk->matdir[i].matstat = dirbufr[j * 4 + 3];
  }
  return (matdirblk);
}

ecat_matrix::MatDirList *
mat_read_directory(ecat_matrix::MatrixFile *mptr)
{
  ecat_matrix::MatDir   matdir;
  ecat_matrix::MatDirList     *dirlist;
  MatDirBlk      *matdirblk;
  int             i, blknum;

  set_matrix_no_error();
  dirlist = (ecat_matrix::MatDirList *) calloc(1, sizeof(ecat_matrix::MatDirList));
  if (dirlist == NULL)
    return (NULL);

  blknum = MatFirstDirBlk;
  do {
    matdirblk = mat_rdirblk(mptr, blknum);
    if (matdirblk == NULL) {
      free(dirlist);
      return (NULL);
    }
    for (i = 0; i < matdirblk->nused; i++) {
      matdir.matnum  = matdirblk->matdir[i].matnum;
      matdir.strtblk = matdirblk->matdir[i].strtblk;
      matdir.endblk  = matdirblk->matdir[i].endblk;
      matdir.matstat = matdirblk->matdir[i].matstat;
      insert_mdir(&matdir, dirlist);
    }
    blknum = matdirblk->nextblk;
    free(matdirblk);
  }
  while (blknum != MatFirstDirBlk);
  return (dirlist);
}

ecat_matrix::MatrixFile *matrix_open(const char* fname, ecat_matrix::MatrixFileAccessMode fmode, ecat_matrix::MatrixFileType_64 mtype) {
  int status;
  ecat_matrix::MatrixFile *mptr = NULL;
  char *omode;

  if (fmode == ecat_matrix::MatrixFileAccessMode::READ_ONLY) {
    omode = R_MODE;
  } else {
    if (access(fname,F_OK) == 0) 
      omode = RW_MODE;
    else 
      omode = W_MODE;
  }

  set_matrix_no_error();
  /* allocate space for ecat_matrix::MatrixFile and main header data structures */
  if ( (mptr = (ecat_matrix::MatrixFile *)calloc(1,sizeof(ecat_matrix::MatrixFile))) == NULL) {
    return(NULL) ;
  }
  if ( (mptr->mhptr = (ecat_matrix::Main_header *)calloc(1,sizeof(ecat_matrix::Main_header))) == NULL) {
    free( mptr);
    return(NULL) ;
  }
  if (fmode == ecat_matrix::MatrixFileAccessMode::READ_ONLY) { /* check if interfile or analyze format */
    if ((mptr->fname = is_interfile(fname)) != NULL) {
      if (interfile_open(mptr) == ecat_matrix::ECATX_ERROR) {
        free_matrix_file(mptr);
        return (NULL);
      }
      return mptr;
    }
    if ((mptr->fname=is_analyze(fname)) != NULL) {
      if (analyze_open(mptr) == ecat_matrix::ECATX_ERROR) {
        free_matrix_file(mptr);
        return (NULL);
      }
      return mptr;
    }
  }

  /* assume CTI/ECAT format */
  if ((mptr->fptr = fopen(fname, omode)) == NULL) {
    free_matrix_file(mptr);
    return (NULL);
  }
  mptr->fname = strdup(fname);
  if (mat_read_main_header(mptr->fptr, mptr->mhptr) == ecat_matrix::ECATX_ERROR) {
    // g_matrix_error = ecat_matrix::MatrixError::NOMHD_FILE_OBJECT ;
    LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::NOMHD_FILE_OBJECT));
    free_matrix_file(mptr);
    return(NULL);
  }

  /*
    if the matrix type doesn't match the requested type, that's
    an error. Specify matrix type NoData to open any type.
  */
  if (mtype != MatrixData::DataSetType::NoData && mptr->mhptr->file_type!= MatrixData::DataSetType::NoData)
    if (mtype != mptr->mhptr->file_type) {
      // g_matrix_error = ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH ;
      LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::FILE_TYPE_NOT_MATCH));
      free_matrix_file(mptr);
      return (NULL);
	}

  /* read and store the matrix file directory.  */
  mptr->dirlist = mat_read_directory(mptr);
  if (mptr->dirlist == NULL ) {
    free_matrix_file( mptr );
    return( NULL );
  }

  if (!strncmp(mptr->mhptr->magic_number, "MATRIX", strlen("MATRIX")) ) {
    mptr->file_format = ecat_matrix::FileFormat::ECAT7;
  } else {
    mptr->file_format = ecat_matrix::FileFormat::ECAT6;
  }

  // wtf was this.  Return mptr if no error, else return null?
  // if (g_matrix_error == ecat_matrix::ECATX_OK ) 
    return( mptr ) ;
  // free_matrix_file( mptr);
  // return(NULL) ;
}

ecat_matrix::MatrixFile *matrix_create(const char *fname, MatrixFileAccessMode const fmode, ecat_matrix::Main_header * proto_mhptr) {
  ecat_matrix::MatrixFile     *mptr = NULL;
  FILE           *outfile_ptr, *mat_create();

  set_matrix_no_error();
  switch (fmode) {
  case ecat_matrix::MatrixFileAccessMode::READ_WRITE:
  case ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING:
    mptr = matrix_open(fname, ecat_matrix::MatrixFileAccessMode::READ_WRITE, proto_mhptr->file_type);
    if (mptr) 
      break;
  case ecat_matrix::MatrixFileAccessMode::CREATE:
  case ecat_matrix::MatrixFileAccessMode::CREATE_NEW_FILE:
    set_matrix_no_error();
    outfile_ptr = mat_create(fname, proto_mhptr);
    if (!outfile_ptr) 
      return( NULL );

    mptr = (ecat_matrix::MatrixFile *) calloc(1, sizeof(ecat_matrix::MatrixFile));
    if (!mptr) {
      fclose( outfile_ptr );
      return( NULL );
    }
    mptr->fptr = outfile_ptr;
    mptr->fname = (char *) malloc(strlen(fname) + 1);
    if (!mptr->fname) {
      free( mptr );
      fclose( outfile_ptr );
      return( NULL );
    }
    strcpy(mptr->fname, fname);
    mptr->mhptr = (ecat_matrix::Main_header *) malloc(sizeof(ecat_matrix::Main_header));
    if (!mptr->mhptr) {
      free( mptr->fname );
      free( mptr );
      fclose( outfile_ptr );
      return( NULL );
    }

    memcpy(mptr->mhptr, proto_mhptr, sizeof(ecat_matrix::Main_header));
    mptr->dirlist = mat_read_directory(mptr);
    if (!mptr->dirlist) {
      free( mptr->fname );
      free( mptr->mhptr );
      free( mptr );
      fclose( outfile_ptr );
      return( NULL );
    }
    break;
  default:
    // g_matrix_error = ecat_matrix::MatrixError::BAD_FILE_ACCESS_MODE;
      LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::BAD_FILE_ACCESS_MODE));
    return( NULL );
    break;
  }
  return mptr;
}

int matrix_close(ecat_matrix::MatrixFile *mptr) {
  int status = ecat_matrix::ECATX_OK;
  // g_matrix_error = ecat_matrix::MatrixError::OK;
  if (mptr->fname) 
    strcpy(ecat_matrix::matrix_errtxt,mptr->fname);
  else 
    ecat_matrix::matrix_errtxt[0] = '\0';
  if (mptr == NULL) 
    return status;
  if (mptr->mhptr != NULL) 
    free(mptr->mhptr) ;
  if (mptr->dirlist != NULL) 
    matrix_freelist(mptr->dirlist) ;
  if (mptr->fptr) 
    status = fclose(mptr->fptr);
  if (mptr->fname) 
    free(mptr->fname);
  free(mptr);
  return status;
}

ecat_matrix::MatrixData *matrix_read(ecat_matrix::MatrixFile *mptr, int matnum, MatrixData::DataType dtype) {
  ecat_matrix::MatrixData     *data;

  // g_matrix_error = ecat_matrix::ECATX_OK;
  // ecat_matrix::matrix_errtxt.clear();
  if (mptr == NULL) {
    // g_matrix_error = ecat_matrix::MatrixError::READ_FROM_NILFPTR;
    LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::READ_FROM_NILFPTR));
    return NULL;
  }
  if (mptr->mhptr == NULL) {
    // g_matrix_error = ecat_matrix::MatrixError::NOMHD_FILE_OBJECT;
    LOG_ERROR(ecat_matrix::matrix_errors_.at(ecat_matrix::MatrixError::NOMHD_FILE_OBJECT));
    return NULL;
  }
  /* allocate space for ecat_matrix::MatrixData structure and initialize */
  data = (ecat_matrix::MatrixData *) calloc(1, sizeof(ecat_matrix::MatrixData));
  if (!data)
    LOG_ERROR("Unable to allocate memory");
    return (NULL);
  }

  /* allocate space for subheader and initialize */
  data->shptr = (void *) calloc(2, ecat_matrix::MatBLKSIZE);
  if (!data->shptr) {
    free(data);
    LOG_ERROR("Unable to allocate memory");
    return (NULL);
  }
  if (read_host_data(mptr, matnum, data, dtype) != ecat_matrix::ECATX_OK) {
    free_matrix_data(data);
    data = NULL;
  } else if (data->data_ptr != NULL) {
    if (data->mat_type == MatrixData::DataSetType::PetVolume && data->data_type == MatrixData::DataType::IeeeFloat)
      matrix_convert_data(data, MatrixData::DataType::SunShort);
    if (dtype != NoData && data->data_type != dtype)
      matrix_convert_data(data, dtype);
  }
  return (data);
}

// ahc this used to be in analyze.cpp

int analyze_read(ecat_matrix::MatrixFile *mptr, int matnum, MatrixData *t_mdata, MatrixData::DataType dtype) {
  ecat_matrix::MatVal matval;

  mat_numdoc(matnum, &matval);
  t_mdata->matnum = matnum;
  t_mdata->pixel_size = t_mdata->y_size = t_mdata->z_size = 1.0;
  t_mdata->scale_factor = 1.0;

  struct image_dimension *hdim = &(((struct dsr *)mptr->analyze_hdr)->dime);
  ecat_matrix::Image_subheader *imagesub = (ecat_matrix::Image_subheader*)t_mdata->shptr;
  memset(imagesub, 0, sizeof(ecat_matrix::Image_subheader));
  imagesub->num_dimensions = 3;
  imagesub->x_pixel_size = t_mdata->pixel_size   = hdim->pixdim[1] * 0.1f;
  imagesub->y_pixel_size = t_mdata->y_size       = hdim->pixdim[2] * 0.1f;
  imagesub->z_pixel_size = t_mdata->z_size       = hdim->pixdim[3] * 0.1f;
  imagesub->scale_factor = t_mdata->scale_factor = (hdim->funused1 > 0.0) ? hdim->funused1 : 1.0f;
  imagesub->x_dimension  = t_mdata->xdim         = hdim->dim[1];
  imagesub->y_dimension  = t_mdata->ydim         = hdim->dim[2];
  imagesub->z_dimension  = t_mdata->zdim         = hdim->dim[3];
  imagesub->data_type = t_mdata->data_type = MatrixData::DataType::SunShort;
  if (dtype == MatrixData::DataType::MAT_SUB_HEADER)
    return ecat_matrix::ECATX_OK;

  int  data_offset = 0;
  int elem_size = 2;
  size_t npixels = t_mdata->xdim * t_mdata->ydim;
  size_t nvoxels = npixels * t_mdata->zdim;
  t_mdata->data_size = nvoxels * elem_size;
  int nblks = (t_mdata->data_size + ecat_matrix::MatBLKSIZE - 1) / ecat_matrix::MatBLKSIZE;
  t_mdata->data_ptr = (void *) calloc(nblks, ecat_matrix::MatBLKSIZE);
  if (matval.frame > 1) 
    data_offset = (matval.frame - 1) * t_mdata->data_size;
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