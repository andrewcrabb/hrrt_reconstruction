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

#include	<stdlib.h>
#include	<math.h>
#include	<fcntl.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"interfile.h"
#include	"machine_indep.h"
#include	"matrix.h"
#include	"num_sort.h"


#if defined(_WIN32) && !defined(__CYGWIN__)
#include <io.h>
#define stat _stat
#define access _access
#define F_OK 0
#define fseek64 _lseeki64
#define fileno _fileno
#else
#include	<unistd.h>
#endif


#define TRUE 1
#define FALSE 0

MatrixErrorCode matrix_errno;
char		matrix_errtxt[132];

char* matrix_errors[] =
	{
		"No Error",
		"Read error",
		"Write error",
		"Invalid directory block",
		"ACS file not found",
		"Interfile open error",
		"File type not match",
		"Read from nil filepointer",
		"No mainheader file object",
		"Nil subheader pointer",
		"Nil data pointer",
		"Matrix not found",
		"Unknown filetype",
		"ACS create error",
		"Bad attribute",
		"Bad file access mode",
		"Invalid dimension",
		"No slices found",
		"Invalid data type",
		"Invalid multibed position"
	};

static int
matrix_freelist(MatDirList *matdirlist)
{
  MatDirNode	*node, *next ;

	if (matdirlist == NULL) return ECATX_OK;
	if (matdirlist->first != NULL)
	{
	  node = matdirlist->first ;
	  do
	  {
		next = node->next ;
		free(node) ;
		node = next ;
	  }
	  while(next != NULL) ;
	}
	free(matdirlist) ;
	return ECATX_OK;
}

static void 
free_matrix_file(MatrixFile *mptr)
{
	if (mptr == NULL) return;
	if (mptr->mhptr != NULL) free(mptr->mhptr) ;
	if (mptr->dirlist != NULL) matrix_freelist(mptr->dirlist) ;
	if (mptr->fptr) fclose(mptr->fptr);
	if (mptr->fname) free(mptr->fname);
	free(mptr);
}

is_acs(fname)

  char	*fname ;

{
 	if (strstr(fname, "/sd") == fname)
		return(TRUE) ; 
	else
	   return(FALSE) ;
}

int matrix_convert_data(MatrixData *matrix, MatrixDataType dtype)
{
  float minval,maxval;
  float *fdata, scalef;
  short *sdata, smax=32766;  /* not 32767 to avoid rounding problems */
  int i, nvoxels, nblks;
  if (matrix->data_type != IeeeFloat || dtype != SunShort)
    return ECATX_OK ;	/* dummy for now */
  
  fdata = (float*)matrix->data_ptr;
  nvoxels = matrix->xdim*matrix->ydim*matrix->zdim;
  nblks = (nvoxels*sizeof(short) + MatBLKSIZE-1)/MatBLKSIZE;
  if ((sdata=(short*)calloc(nblks, MatBLKSIZE)) == NULL) return 0;
  matrix->data_size = nvoxels*sizeof(short);
  matrix->data_type = SunShort;
  scalef = matrix->scale_factor;
  minval = maxval = fdata[0];
  for (i=1; i<nvoxels; i++)
  {
    if (fdata[i]>maxval) maxval = fdata[i];
    else if (fdata[i]<minval) minval = fdata[i];
  }
  minval *= matrix->scale_factor;
  maxval *= matrix->scale_factor;
  if (fabs(minval) < fabs(maxval)) scalef = (float)(fabs(maxval)/smax);
  else scalef = (float)(fabs(minval)/smax);
  for (i=0; i<nvoxels; i++)
    sdata[i] = (short)(0.5+fdata[i]/scalef);
  free(fdata);
  matrix->data_ptr = (caddr_t)sdata;
  matrix->scale_factor = scalef;
  matrix->data_min = minval;
  matrix->data_max = maxval;
  return 1;
}



static int matrix_write_slice(mptr,matnum,data,plane)
MatrixFile *mptr ;
MatrixData *data ;
int   matnum, plane;
{
	MatrixData *slice;
	Image_subheader *imh;
	struct Matval val;
	int i, npixels, nblks, s_matnum;
	short *sdata;
	u_char* bdata;
	int	ret;

	switch(mptr->mhptr->file_type) {
		case PetImage :
			if (data->data_type ==  ByteData) {
/*				fprintf(stderr,"Only short data type supported in V6\n");*/
				matrix_errno = MAT_INVALID_DATA_TYPE ;
				return ECATX_ERROR;
			}
			mat_numdoc(matnum,&val);
			slice = (MatrixData*)malloc(sizeof(MatrixData));
			if( !slice ) return(ECATX_ERROR) ;

			imh = (Image_subheader*)calloc(1,MatBLKSIZE);
			if( !imh ) {
				free( slice );
				return(ECATX_ERROR) ;
			}
			memcpy(slice,data,sizeof(MatrixData));
			memcpy(imh,data->shptr,sizeof(Image_subheader));
			slice->shptr = (caddr_t)imh;
			slice->zdim = imh->z_dimension = 1;
			npixels = slice->xdim*slice->ydim;
			nblks = (npixels*2 + MatBLKSIZE-1)/MatBLKSIZE;
			slice->data_ptr = (caddr_t)calloc(nblks,MatBLKSIZE);
			if( !slice->data_ptr ) {
				free( slice );
				free( imh );
				return(ECATX_ERROR) ;
			}
			slice->data_size = nblks*MatBLKSIZE;
			if (data->data_type ==  ByteData) {
				bdata = (u_char*)(data->data_ptr+(plane-1)*npixels);
				imh->image_min = find_bmin(bdata,npixels);
				imh->image_max = find_bmax(bdata,npixels);
				sdata = (short*)slice->data_ptr;
				for (i=0; i<npixels; i++)  sdata[i] = bdata[i];
			} else {
				sdata = (short*)(data->data_ptr+(plane-1)*npixels*2);
				imh->image_min = find_smin(sdata,npixels);
				imh->image_max = find_smax(sdata,npixels);
				memcpy(slice->data_ptr,sdata,npixels*2);
			}
			s_matnum = mat_numcod(val.frame,plane,val.gate,val.data,val.bed);
			ret = matrix_write(mptr,s_matnum,slice);
			free_matrix_data(slice);
			return( ret );
		case PetVolume :
		case ByteVolume :
		case InterfileImage:
/*			fprintf(stderr,
			"matrix_slice_write : Main_header file_type should be PetImage\n");
*/
			matrix_errno = MAT_FILE_TYPE_NOT_MATCH;
			return ECATX_ERROR;
		default:
/*			fprintf(stderr,"V7 to V6 conversion only supported for images\n");*/
			matrix_errno = MAT_FILE_TYPE_NOT_MATCH;
			return ECATX_ERROR;
		}
}

u_char 
find_bmax(const u_char *bdata, int nvals)
{
	int i;
	u_char bmax = bdata[0];
	for (i=1; i<nvals; i++)
	  if (bdata[i] > bmax) bmax = bdata[i];
	return bmax;
}

u_char 
find_bmin(const u_char *bdata, int nvals)
{
	int i;
	u_char bmin = bdata[0];
	for (i=1; i<nvals; i++)
	  if (bdata[i] < bmin) bmin = bdata[i];
	return bmin;
}

short 
find_smax( const short *sdata, int nvals)
{
	int i;
	short smax = sdata[0];
	for (i=1; i<nvals; i++)
	  if (sdata[i] > smax) smax = sdata[i];
	return smax;
}

short
find_smin( const short *sdata, int nvals)
{
	int i;
	short  smin = sdata[0];
	for (i=1; i<nvals; i++)
	  if (sdata[i] < smin) smin = sdata[i];
	return smin;
}

int 
find_imax( const int *idata, int nvals)
{
	int i, imax=idata[0];
	for (i=1; i<nvals; i++)
	  if (idata[i]>imax) imax = idata[i];
	return imax;
}

int
find_imin( const int *idata, int nvals)
{
	int i, imin=idata[0];
	for (i=1; i<nvals; i++)
	  if (idata[i]<imin) imin = idata[i];
	return imin;
}

float 
find_fmin( const float *fdata, int nvals)
{
	int i;
	float fmin = fdata[0];
	for (i=1; i<nvals; i++)
	  if (fdata[i]<fmin) fmin = fdata[i];
	return fmin;
}

float
find_fmax( const float *fdata, int nvals)
{
	int i;
	float fmax = fdata[0];
	for (i=1; i<nvals; i++)
	  if (fdata[i]>fmax) fmax = fdata[i];
	return fmax;
}


int
read_host_data(MatrixFile *mptr, int matnum, MatrixData *data, int dtype) 
{
  struct MatDir matdir;
  int	 nblks, data_size ;
  Scan_subheader *scansub ;
  Scan3D_subheader *scan3Dsub ;
  Image_subheader *imagesub ;
  Attn_subheader *attnsub ;
  Norm_subheader *normsub ;
  Norm3D_subheader *norm3d;
  int sx,sy,sz;
  int elem_size= 2;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';

	data->matnum = matnum;
	data->matfile = mptr;
	data->mat_type = mptr->mhptr->file_type;

  if (mptr->analyze_hdr)  /* read interfile */
		return analyze_read(mptr, matnum, data, dtype);

  if (mptr->interfile_header)  /* read interfile */
		return interfile_read(mptr, matnum, data, dtype);

	if (matrix_find(mptr,matnum,&matdir) == ECATX_ERROR)
	{
	   matrix_errno = MAT_MATRIX_NOT_FOUND ;
	   return(ECATX_ERROR) ;
	}
	nblks = matdir.endblk - matdir.strtblk ;
	data_size = data->data_size = 512*nblks;
	if (dtype != MAT_SUB_HEADER)
	{
	  data->data_ptr = (caddr_t) calloc(1, data_size) ;
	  if (data->data_ptr == NULL)
	  {
		 return(ECATX_ERROR) ;
	  }
	} 
	switch(mptr->mhptr->file_type)
	{
	   case Sinogram :
		scansub = (Scan_subheader *) data->shptr ;
        mat_read_scan_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk,
			scansub) ;
		data->data_type = scansub->data_type ;
		data->xdim = scansub->num_r_elements ;
		data->ydim = scansub->num_angles ;
		data->zdim = scansub->num_z_elements ;
		data->scale_factor = scansub->scale_factor ;
		data->pixel_size = scansub->x_resolution;
		data->data_max = scansub->scan_max * scansub->scale_factor ;
		if (dtype == MAT_SUB_HEADER) break;
		read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks,
		  data->data_ptr, scansub->data_type) ;
		break ;
	   case Short3dSinogram :
	   case Float3dSinogram :
		scan3Dsub = (Scan3D_subheader *) data->shptr ;
		mat_read_Scan3D_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk,
			scan3Dsub) ;
		data->data_type = scan3Dsub->data_type ;
		data->scale_factor = scan3Dsub->scale_factor ;
		data->pixel_size = scan3Dsub->x_resolution;
		data->y_size = data->pixel_size;
		data->z_size = scan3Dsub->z_resolution;
		data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor ;
		sx = data->xdim = scan3Dsub->num_r_elements ;
		sz = data->ydim = scan3Dsub->num_angles ;
		sy = data->zdim = scan3Dsub->num_z_elements[0] ;
		if (dtype == MAT_SUB_HEADER) break;

		read_matrix_data(mptr->fptr, matdir.strtblk+2, nblks-1,
		  		data->data_ptr, scan3Dsub->data_type) ;
		if (scan3Dsub->data_type == SunShort) {
			scan3Dsub->scan_max = find_smax((short*)data->data_ptr,sx*sy*sz);
			scan3Dsub->scan_min = find_smin((short*)data->data_ptr,sx*sy*sz);
			data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
			data->data_min = scan3Dsub->scan_min * scan3Dsub->scale_factor;

		} else {
			data->data_max = find_fmax((float*)data->data_ptr,sx*sy*sz);
			data->data_min = find_fmin((float*)data->data_ptr,sx*sy*sz);
		}
		break ;
	   case ByteVolume :
	   case PetImage :
	   case PetVolume :
	   case InterfileImage:
		imagesub = (Image_subheader *) data->shptr ;
		mat_read_image_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk,
			imagesub);
		data->data_type = imagesub->data_type ;
		sx = data->xdim = imagesub->x_dimension ;
		sy = data->ydim = imagesub->y_dimension ;
		data->scale_factor = imagesub->scale_factor ;
		if (data->data_type==ByteData || data->data_type==Color_8)
			elem_size =1 ;
		else elem_size = 2;
		sz = data->zdim = imagesub->z_dimension ;
		if( sx*sy*elem_size == 0 ) {
			matrix_errno = MAT_INVALID_DIMENSION;
			return( ECATX_ERROR );
		}
		if (sz > data_size/(sx*sy*elem_size))
	      sz = data->zdim = imagesub->z_dimension = data_size/(sx*sy*elem_size);
/* fix inconsistent file types */
		if (data->zdim > 1) {
			if (data->data_type == ByteData) data->mat_type = ByteVolume;
			else data->mat_type = PetVolume;
		} else data->mat_type = PetImage;

		data->pixel_size = imagesub->x_pixel_size;
		data->y_size = imagesub->y_pixel_size;
				/* if imagesub->y_pixel_size not filled assume square pixels */
		if (data->y_size <= 0) data->y_size = imagesub->x_pixel_size;
		data->z_size = imagesub->z_pixel_size;
				/* if imagesub->z_pixel_size not filled use palne separation */
		if (data->z_size <= 0) data->z_size = mptr->mhptr->plane_separation;
		data->data_max = imagesub->image_max * imagesub->scale_factor;
		/* KT added next 3 lines */
		data->z_origin = imagesub->z_offset;
		data->y_origin = imagesub->y_offset;
		data->x_origin = imagesub->x_offset;
		if (dtype == MAT_SUB_HEADER) break;
		read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks,
		  data->data_ptr, imagesub->data_type) ;
		if (imagesub->data_type == ByteData) {
			imagesub->image_max = find_bmax((u_char*)data->data_ptr,sx*sy*sz);
			imagesub->image_min = find_bmin((u_char*)data->data_ptr,sx*sy*sz);
		} else {
			imagesub->image_max = find_smax((short*)data->data_ptr,sx*sy*sz);
			imagesub->image_min = find_smin((short*)data->data_ptr,sx*sy*sz);
		}
		data->data_max = imagesub->image_max * imagesub->scale_factor;
		data->data_min = imagesub->image_min * imagesub->scale_factor;
		break ;
	   case AttenCor :
		attnsub = (Attn_subheader *) data->shptr ;
		mat_read_attn_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk,attnsub);
		data->data_type = attnsub->data_type ;
		sx = data->xdim = attnsub->num_r_elements ;
		sz = data->ydim = attnsub->num_angles ;
		sy = data->zdim = attnsub->z_elements[0] ;
		data->scale_factor = attnsub->scale_factor;
		data->pixel_size = attnsub->x_resolution;
		if (dtype == MAT_SUB_HEADER) break;
		read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks,
			data->data_ptr, attnsub->data_type);
		data->data_max = find_fmax((float*)data->data_ptr,sx*sy*sz);
		break ;
	   case Normalization :
		normsub = (Norm_subheader *) data->shptr ;
		mat_read_norm_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk,normsub);
		data->data_type = normsub->data_type ;
		data->xdim = normsub->num_r_elements ;
		data->ydim = normsub->num_angles ;
		data->zdim = normsub->num_z_elements ;
		data->scale_factor = normsub->scale_factor ;
		if (dtype == MAT_SUB_HEADER) break;
		read_matrix_data(mptr->fptr, matdir.strtblk+1, nblks,
		  data->data_ptr, normsub->data_type) ;
		data->data_max = data->scale_factor * 
		  find_fmax((float*)data->data_ptr, data->xdim * data->ydim);
		break ;
	   case Norm3d :
		norm3d = (Norm3D_subheader *) data->shptr ;
		mat_read_norm3d_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk,norm3d);
		data->data_type = norm3d->data_type ;
		data->xdim = norm3d->num_r_elements;	/* 336 */
		data->ydim = norm3d->crystals_per_ring;	/* 784 */
		data->zdim = norm3d->num_crystal_rings;	/* 24 */
		data->scale_factor = 1.0;
		if (dtype == MAT_SUB_HEADER) break;
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
		matrix_errno = MAT_UNKNOWN_FILE_TYPE ;
		return(ECATX_ERROR) ;
		break ;
	}
	return(ECATX_OK) ;
}


int matrix_write(MatrixFile *mptr, int matnum, MatrixData *data)
{
  struct MatDir matdir, dir_entry ;
  Scan3D_subheader *scan3Dsub = NULL;
  Scan_subheader *scansub ;
  Image_subheader *imagesub ;
  Attn_subheader *attnsub ;
  Norm_subheader *normsub ;
  int	status, blkno, nblks ;


   
	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';
	status = ECATX_OK ;
	nblks = (data->data_size+511)/512;

        
	/* 3D sinograms subheader use one more block */
    if (mptr->mhptr->file_type == Short3dSinogram  ||
        mptr->mhptr->file_type == Float3dSinogram) nblks += 1;

	if (matrix_find(mptr, matnum, &matdir) == ECATX_ERROR)
	{
	   blkno = mat_enter(mptr->fptr, mptr->mhptr, matnum, nblks) ;
	   if( blkno == ECATX_ERROR ) return( ECATX_ERROR );
	   dir_entry.matnum = matnum ;
	   dir_entry.strtblk = blkno ;
	   dir_entry.endblk = dir_entry.strtblk + nblks - 1 ;
	   dir_entry.matstat = 1 ;
	   insert_mdir(&dir_entry, mptr->dirlist) ;
	   matdir = dir_entry ;
	}

	switch(mptr->mhptr->file_type)
	{
       case Float3dSinogram :
       case Short3dSinogram :
        scan3Dsub = (Scan3D_subheader*) data->shptr;
        if (mat_write_Scan3D_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk,
            scan3Dsub) == ECATX_ERROR) return ECATX_ERROR;
          if (write_matrix_data(mptr->fptr, matdir.strtblk+2,
            nblks, data->data_ptr, scan3Dsub->data_type) == ECATX_ERROR) return ECATX_ERROR;
          break;
	   case Sinogram :
		  scansub = (Scan_subheader *) data->shptr ;
		  if( mat_write_scan_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, scansub) == ECATX_ERROR )
        return( ECATX_ERROR );
		  status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, scansub->data_type) ;
		  if( status == ECATX_ERROR ) return( ECATX_ERROR );
		  break ;
	   case ByteVolume :
	   case PetImage :
	   case PetVolume :
	   case InterfileImage:


		  imagesub = (Image_subheader *) data->shptr ;
	   	  if (imagesub == NULL) {
                imagesub = (Image_subheader *) calloc(1, MatBLKSIZE);
                data->shptr = (caddr_t)imagesub;
		  }							/* use MatrixData info */
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

                  if( mat_write_image_subheader(mptr->fptr,mptr->mhptr,matdir.strtblk, imagesub) == ECATX_ERROR )
        return( ECATX_ERROR );
		  status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, imagesub->data_type) ;
		  if( status == ECATX_ERROR ) return( ECATX_ERROR );
		  break ;
	   case AttenCor :
		  attnsub = (Attn_subheader *) data->shptr ;
		  if( mat_write_attn_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, attnsub) == ECATX_ERROR )
        return( ECATX_ERROR );
		  status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, attnsub->data_type);
		  if( status == ECATX_ERROR ) return( ECATX_ERROR );
		  break ;
	   case Normalization :
		  normsub = (Norm_subheader *) data->shptr ;
		  if( mat_write_norm_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, normsub) == ECATX_ERROR )
        return( ECATX_ERROR );
		  status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, normsub->data_type) ;
		  if( status == ECATX_ERROR ) return( ECATX_ERROR );
		  break ;
	   default :	/* default treated as sinogram */
		  scansub = (Scan_subheader *) data->shptr ;
		  if( mat_write_scan_subheader(mptr->fptr,mptr->mhptr, matdir.strtblk, scansub) == ECATX_ERROR )
        return( ECATX_ERROR );

		  status = write_matrix_data(mptr->fptr, matdir.strtblk+1, nblks, data->data_ptr, scansub->data_type) ;
		  if( status == ECATX_ERROR ) return( ECATX_ERROR );
		  break ;
	}
	return(status) ;
}

int
matrix_find(MatrixFile *matfile, int matnum, struct MatDir *matdir)
{
  MatDirNode	*node ;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';
	if (matfile == NULL) return(ECATX_ERROR) ;
	if (matfile->dirlist == NULL) return(ECATX_ERROR) ;	
	node = matfile->dirlist->first ;
	while (node != NULL)
	{
	   if (node->matnum == matnum)
	   {
		matdir->matnum = node->matnum ;
		matdir->strtblk = node->strtblk ;
		matdir->endblk = node->endblk ;
		matdir->matstat = node->matstat ;
		break ;
	   }
	   node = node->next ;
	}
	if (node != NULL) return(ECATX_OK) ;
	else return(ECATX_ERROR) ;
}
	


int 
insert_mdir(struct MatDir	*matdir, MatDirList	*dirlist)
{
  MatDirNode	*node ;
 
	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';
	if (dirlist == NULL)
	{
		dirlist = (MatDirList *) malloc(sizeof(MatDirList)) ;
		if (dirlist == NULL) return(ECATX_ERROR) ;
		dirlist->nmats = 0 ;
		dirlist->first = NULL ;
		dirlist->last = NULL ;
	}
	node = (MatDirNode *) malloc(sizeof(MatDirNode)) ;
	if (node == NULL) return(ECATX_ERROR) ;

	node->matnum = matdir->matnum ;
	node->strtblk = matdir->strtblk ;
	node->endblk = matdir->endblk ;
	node->matstat = matdir->matstat;
	node->next = NULL ;

	if (dirlist->first == NULL)	/* if list was empty, add first node */
	{
	   dirlist->first = node ;
	   dirlist->last = node ;
	   dirlist->nmats = 1 ;
	}
	else
	{
	   (dirlist->last)->next = node ;
	   dirlist->last = node ;
	   ++(dirlist->nmats) ;
	}
	return ECATX_OK;
}

void 
free_matrix_data(MatrixData	*data)
{
	if (data != NULL)
	{
	   if (data->data_ptr != NULL) free(data->data_ptr) ;
	   if (data->shptr != NULL) free(data->shptr) ;
	   free(data) ;
	}

}

void matrix_perror( const char* s)

{
	if (matrix_errno)
		fprintf( stderr, "%s: %s\n", s, matrix_errors[matrix_errno]);
	else perror(s);
}

Main_header *
matrix_init_main_header( char *fname, DataSetType ftype, Main_header *mh_proto)
{
	Main_header *mhptr;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';
	mhptr = (Main_header*) calloc( 1, sizeof(Main_header));
	if( !mhptr ) return( NULL );
	if (mh_proto)
	  memcpy(mhptr, mh_proto, sizeof(Main_header));
	mhptr->file_type = ftype;
	strncpy( mhptr->original_file_name, fname, 20);
	return mhptr;
}

void 
matrix_free( MatrixData *matrix)
{
	if (matrix->shptr) free( matrix->shptr);
	if (matrix->data_ptr) free( matrix->data_ptr);
	free( matrix);
}

int 
convert_float_scan( MatrixData *scan, float *fdata)
{
	int i, nvals, tot;
	float fmax, scale;
	short int *sdata;
	Scan_subheader *ssh;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';
	if (scan->data_ptr)
	  free(scan->data_ptr);
	nvals = scan->xdim*scan->ydim;
	sdata = (short int*) malloc( nvals*sizeof(short int));
	if (sdata == NULL) return ECATX_ERROR;
	scan->data_ptr = (caddr_t) sdata;
	scan->data_size = nvals*sizeof(short int);
	fmax = (float)(fabs(*fdata));
	for (i=0; i<nvals; i++)
	  if (fabs(fdata[i]) > fmax) fmax = (float)fabs(fdata[i]);
	scale = 1.0f;
	if (fmax > 0.0f) scale = 32767.0f/fmax;
	tot = 0;
	for (i=0; i<nvals; i++)
	{
	  sdata[i] = (short)(scale*fdata[i] + 0.5f);
	  tot += sdata[i];
	}
	scan->scale_factor = 1.0f/scale;
	ssh = (Scan_subheader*) scan->shptr;
	ssh->scan_min = 0;
	ssh->scan_max = (short)(fmax*scale);
	ssh->num_r_elements = scan->xdim;
	ssh->num_angles = scan->ydim;
	ssh->net_trues = tot;
	ssh->scale_factor = 1.0f/scale;
		ssh->x_resolution = scan->pixel_size;
	return ECATX_OK;
}

MatrixData *
matrix_read_scan(MatrixFile *mptr, int matnum, int dtype, int segment)
{
	int i, nblks, plane_size;
	int sx, sy, sz;
	MatrixData *data;
	struct MatDir matdir;
	Scan3D_subheader *scan3Dsub;
	Attn_subheader *attnsub;
	int status, group;
	unsigned z_elements;
#ifdef _WIN32
  __int64 file_pos=0;
#else 
  long file_pos=0;
#endif

/* Scan3D and Atten storage:
	storage_order = 0 : (((projs x z_elements)) x num_angles) x Ringdiffs
	storage_order != 0 : (((projs x num_angles)) x z_elements)) x Ringdiffs
    
*/

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';

  if (matrix_find(mptr,matnum,&matdir) == ECATX_ERROR)
    return NULL;

	if ((data = (MatrixData *) calloc( 1, sizeof(MatrixData))) != NULL) {
		if ( (data->shptr = (caddr_t) calloc(2, MatBLKSIZE)) == NULL) {
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
	case Float3dSinogram :
	case Short3dSinogram :
		scan3Dsub = (Scan3D_subheader *)data->shptr;
        status = mat_read_Scan3D_subheader(mptr->fptr, mptr->mhptr,
				matdir.strtblk, scan3Dsub);
		if (status == ECATX_ERROR) {
			free_matrix_data(data);	
			return NULL;
		}
		file_pos = (matdir.strtblk+1)*MatBLKSIZE;
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
		if (mptr->mhptr->file_type == Float3dSinogram) plane_size *= 2;
		data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
		if (dtype == MAT_SUB_HEADER) break;
		for (i=0; i<group; i++)
			file_pos += scan3Dsub->num_z_elements[i]*plane_size;
		if (segment < 0) file_pos += z_elements*plane_size;
		data->data_size = z_elements*plane_size;
		nblks = (data->data_size+511)/512;


                                    
	  	if ((data->data_ptr = (caddr_t)calloc(nblks,512)) == NULL ||
#ifdef _WIN32
        fseek64(fileno(mptr->fptr),file_pos,0) == -1 ||
#else
            /* fseeko64(mptr->fptr,file_pos,0) == -1 || */
            /* ahc */
            fseeko(mptr->fptr,file_pos,0) == -1 ||
#endif
			fread(data->data_ptr, plane_size, z_elements, mptr->fptr)
				!= z_elements ||
			file_data_to_host(data->data_ptr,nblks,data->data_type) == ECATX_ERROR) {
				free_matrix_data(data);
				return NULL;
		}
		if (mptr->mhptr->file_type == Short3dSinogram) {
			scan3Dsub->scan_max = find_smax((short*)data->data_ptr,sx*sy*sz);
			scan3Dsub->scan_min = find_smin((short*)data->data_ptr,sx*sy*sz);
			data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
			data->data_min = scan3Dsub->scan_min * scan3Dsub->scale_factor;

		} else {
			data->data_max = find_fmax((float*)data->data_ptr,sx*sy*sz);
			data->data_min = find_fmin((float*)data->data_ptr,sx*sy*sz);
		}
		break;
	case AttenCor :
		attnsub = (Attn_subheader *) data->shptr;
		if (mat_read_attn_subheader(mptr->fptr, mptr->mhptr, matdir.strtblk,
			attnsub) == ECATX_ERROR) {
				free_matrix_data(data);
				return NULL;
		}
		file_pos = matdir.strtblk*MatBLKSIZE;
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
        if (dtype == MAT_SUB_HEADER) break;
        plane_size = sx*attnsub->num_angles*sizeof(float);
        for (i=0; i<group; i++)
            file_pos += attnsub->z_elements[i]*plane_size;
        if (segment < 0) file_pos += z_elements*plane_size;
		if (dtype == MAT_SUB_HEADER) break;
		data->data_size = z_elements*plane_size;
		nblks = (data->data_size+511)/512;


                if ((data->data_ptr = (caddr_t)calloc(nblks,512)) == NULL ||
#ifdef _WIN32
        fseek64(fileno(mptr->fptr),file_pos,0) == -1 ||
#else
                    /* ahc */
                    /*         fseeko64(mptr->fptr,file_pos,0) == -1 || */
        fseeko(mptr->fptr,file_pos,0) == -1 ||
#endif
			fread(data->data_ptr,plane_size,z_elements,mptr->fptr) !=
				z_elements ||
			file_data_to_host(data->data_ptr,nblks,data->data_type) == ECATX_ERROR) {
				free_matrix_data(data);
        		return NULL;
    	}
		data->data_max = find_fmax((float*)data->data_ptr,sx*sy*sz);
		break;
	default:
		matrix_errno = MAT_FILE_TYPE_NOT_MATCH;
		return NULL;
	}
	return data;
}
int 
mh_update(MatrixFile *file)
{
	MatDirNode     *node;
	struct Matval   val;
	int            *frames, *planes, *gates, *beds;
	int             num_frames = 0, num_planes = 0, num_gates = 0,
	                num_beds = 0;
	Main_header    *mh = file->mhptr;
	MatDirList     *dir_list = file->dirlist;
	int             mod = 0, nmats;					/* correction  7/12/99 MS */
	if (file->dirlist == NULL || file->dirlist->nmats == 0)
		return ECATX_OK;
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
	return ECATX_OK;
}

MatDirBlk *
mat_rdirblk(MatrixFile *file, int blknum)
{
	MatDirBlk      *matdirblk;
	int             i, j, err, ndirs;
	int             dirbufr[MatBLKSIZE / 4];
	FILE           *fptr = file->fptr;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';

	matdirblk = (MatDirBlk *) malloc(MatBLKSIZE);
	if (matdirblk == NULL)
		return (NULL);
    err = read_matrix_data(fptr, blknum, 1, (char *) dirbufr, SunLong);
	if (err == ECATX_ERROR) {
		free(matdirblk);
		return (NULL);
	}
	matdirblk->nfree = dirbufr[0];
	matdirblk->nextblk = dirbufr[1];
	matdirblk->prvblk = dirbufr[2];
	matdirblk->nused = dirbufr[3];

	if (matdirblk->nused > 31) {
		matrix_errno = MAT_INVALID_DIRBLK;
		free(matdirblk);
		return (NULL);
	}
	ndirs = (MatBLKSIZE / 4 - 4) / 4;
	for (i = 0; i < ndirs; i++) {
		matdirblk->matdir[i].matnum = 0;
		matdirblk->matdir[i].strtblk = 0;
		matdirblk->matdir[i].endblk = 0;
		matdirblk->matdir[i].matstat = 0;
	}

	for (i = 0; i < matdirblk->nused; i++) {
		j = i + 1;
		matdirblk->matdir[i].matnum = dirbufr[j * 4 + 0];
		matdirblk->matdir[i].strtblk = dirbufr[j * 4 + 1];
		matdirblk->matdir[i].endblk = dirbufr[j * 4 + 2];
		matdirblk->matdir[i].matstat = dirbufr[j * 4 + 3];
	}
	return (matdirblk);
}

MatDirList *
mat_read_directory(MatrixFile *mptr)
{
	struct MatDir   matdir;
	MatDirList     *dirlist;
	MatDirBlk      *matdirblk;
	int             i, blknum;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';

	dirlist = (MatDirList *) calloc(1, sizeof(MatDirList));
	if (dirlist == NULL) return (NULL);

	blknum = MatFirstDirBlk;
	do {
		matdirblk = mat_rdirblk(mptr, blknum);
		if (matdirblk == NULL) {
			free(dirlist);
			return (NULL);
		}
		for (i = 0; i < matdirblk->nused; i++) {
			matdir.matnum = matdirblk->matdir[i].matnum;
			matdir.strtblk = matdirblk->matdir[i].strtblk;
			matdir.endblk = matdirblk->matdir[i].endblk;
			matdir.matstat = matdirblk->matdir[i].matstat;
			insert_mdir(&matdir, dirlist);
		}
		blknum = matdirblk->nextblk;
		free(matdirblk);
	}
	while (blknum != MatFirstDirBlk);
	return (dirlist);
}

MatrixFile *
matrix_open(const char* fname, int fmode, int mtype)
{
#ifndef _WIN32
  int status;
#endif
  MatrixFile *mptr ;
  char *omode;

	if (fmode == MAT_READ_ONLY) omode = R_MODE;
	else {
		if (access(fname,F_OK) == 0) omode = RW_MODE;
		else omode = W_MODE;
	}

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';

	/* allocate space for MatrixFile and main header data structures */
	if ( (mptr = (MatrixFile *)calloc(1,sizeof(MatrixFile))) == NULL) {
		 return(NULL) ;
	}

	if ( (mptr->mhptr = (Main_header *)calloc(1,sizeof(Main_header))) == NULL) {
		 free( mptr);
		 return(NULL) ;
	}

  if (fmode == MAT_READ_ONLY) { /* check if interfile or analyze format */
			if ((mptr->fname=is_interfile(fname)) != NULL) {
				if (interfile_open(mptr) == ECATX_ERROR) {
					/* matrix_errno set by interfile_open */
					free_matrix_file(mptr);
					return (NULL);
				}
				return mptr;
			}
			if ((mptr->fname=is_analyze(fname)) != NULL) {
				if (analyze_open(mptr) == ECATX_ERROR) {
					/* matrix_errno is set by analyze_open */
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
  if (mat_read_main_header(mptr->fptr, mptr->mhptr) == ECATX_ERROR) {
    matrix_errno = MAT_NOMHD_FILE_OBJECT ;
    free_matrix_file(mptr);
    return(NULL);
  }

	/*
	   if the matrix type doesn't match the requested type, that's
	   an error. Specify matrix type NoData to open any type.
	*/
	if (mtype != NoData && mptr->mhptr->file_type!=NoData)
    if (mtype != mptr->mhptr->file_type) {
		matrix_errno = MAT_FILE_TYPE_NOT_MATCH ;
		free_matrix_file(mptr);
		return (NULL);
	}

	/* read and store the matrix file directory.  */
	mptr->dirlist = mat_read_directory(mptr);
	if( mptr->dirlist == NULL ) {
		free_matrix_file( mptr );
		return( NULL );
	}

	if( !strncmp(mptr->mhptr->magic_number, "MATRIX", strlen("MATRIX")) ) {
		mptr->file_format = ECAT7;
	} else {
		mptr->file_format = ECAT6;
	}

	if( matrix_errno == ECATX_OK ) return( mptr ) ;
	free_matrix_file( mptr);
	return(NULL) ;
}

MatrixFile *
matrix_create(const char *fname, int fmode, Main_header * proto_mhptr)
{
	MatrixFile     *mptr = NULL;
	FILE           *fptr, *mat_create();

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';
	switch (fmode) {
	case MAT_READ_WRITE:
	case MAT_OPEN_EXISTING:
		mptr = matrix_open(fname, MAT_READ_WRITE, proto_mhptr->file_type);
		if (mptr) break;
		/*
		 * if (matrix_errno != MAT_NFS_FILE_NOT_FOUND) break; if we
		 * got an NFS_FILE_NOT_FOUND error, then try to create the
		 * matrix file.
		 */
	case MAT_CREATE:
	case MAT_CREATE_NEW_FILE:
		matrix_errno = MAT_OK;
		matrix_errtxt[0] = '\0';
		fptr = mat_create(fname, proto_mhptr);
		if (!fptr) return( NULL );

		mptr = (MatrixFile *) calloc(1, sizeof(MatrixFile));
		if (!mptr) {
			fclose( fptr );
			return( NULL );
		}

		mptr->fptr = fptr;

		mptr->fname = (char *) malloc(strlen(fname) + 1);
		if (!mptr->fname) {
			free( mptr );
			fclose( fptr );
			return( NULL );
		}

		strcpy(mptr->fname, fname);
		mptr->mhptr = (Main_header *) malloc(sizeof(Main_header));
		if (!mptr->mhptr) {
			free( mptr->fname );
			free( mptr );
			fclose( fptr );
			return( NULL );
		}

		memcpy(mptr->mhptr, proto_mhptr, sizeof(Main_header));
		mptr->dirlist = mat_read_directory(mptr);
		if (!mptr->dirlist) {
			free( mptr->fname );
			free( mptr->mhptr );
			free( mptr );
			fclose( fptr );
			return( NULL );
		}
		break;
	default:
		matrix_errno = MAT_BAD_FILE_ACCESS_MODE;
		return( NULL );
		break;
	}
	return mptr;
}

int
matrix_close(MatrixFile *mptr)
{
	int status = ECATX_OK;
	matrix_errno = MAT_OK;
	if (mptr->fname) strcpy(matrix_errtxt,mptr->fname);
	else matrix_errtxt[0] = '\0';
	if (mptr == NULL) return status;
	if (mptr->mhptr != NULL) free(mptr->mhptr) ;
	if (mptr->dirlist != NULL) matrix_freelist(mptr->dirlist) ;
	if (mptr->fptr) status = fclose(mptr->fptr);
	if (mptr->fname) free(mptr->fname);
	free(mptr);
	return status;
}

MatrixData *
matrix_read(MatrixFile *mptr, int matnum, int dtype)
{
	MatrixData     *data;

	matrix_errno = ECATX_OK;
	matrix_errtxt[0] = '\0';
	if (mptr == NULL)
		matrix_errno = MAT_READ_FROM_NILFPTR;
	else if (mptr->mhptr == NULL)
		matrix_errno = MAT_NOMHD_FILE_OBJECT;
	if (matrix_errno != ECATX_OK)
		return (NULL);
	/* allocate space for MatrixData structure and initialize */
	data = (MatrixData *) calloc(1, sizeof(MatrixData));
	if (!data)
		return (NULL);

	/* allocate space for subheader and initialize */
	data->shptr = (caddr_t) calloc(2, MatBLKSIZE);
	if (!data->shptr) {
		free(data);
		return (NULL);
	}
	if (read_host_data(mptr, matnum, data, dtype) != ECATX_OK) {
		free_matrix_data(data);
		data = NULL;
  } else if (data->data_ptr != NULL) {
    if (data->mat_type == PetVolume && data->data_type == IeeeFloat)
      matrix_convert_data(data, SunShort);
    if (dtype != NoData && data->data_type != dtype)
      matrix_convert_data(data, dtype);
  }
	return (data);
}
