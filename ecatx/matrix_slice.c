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
#include	<fcntl.h>
#include	<string.h>
#include	"DICOM.h"
#include	"machine_indep.h"
#include	"matrix.h"
#include	"interfile.h"

#define MY_ERROR	 -1

static MatrixData *matrix_read_v2(MatrixFile *file, int matnum, int type)
{
	char buf[80];
	int i, npixels, b;
	float intercept=0.0, slope=1.0;
	int magic_size;
	char *magic_number=NULL;
	short *data=NULL;
	MatrixData *matrix = NULL;
    Image_subheader *imh = NULL;
	magic_number = file->mhptr->magic_number;
	magic_size = sizeof(file->mhptr->magic_number);
	matrix = matrix_read(file, matnum, type);
	if (matrix==NULL || matrix->dicom_header == NULL) return matrix;
    imh = (Image_subheader*)matrix->shptr;
	if (!dicom_scan_string(0x0008, 0x0060, buf, matrix->dicom_header,
          	matrix->dicom_header_size)) return matrix;
	strncpy(magic_number+magic_size-4,buf,2);
	if (strncmp(buf,"CT",2) != 0)  return matrix;
	/* convert units to hounsfield */
    if (dicom_scan_string(0x0028, 0x1052, buf, matrix->dicom_header,
          	matrix->dicom_header_size)) sscanf(buf, "%g", &intercept); 
   	if (dicom_scan_string(0x0028, 0x1053, buf, matrix->dicom_header,
          	matrix->dicom_header_size)) sscanf(buf, "%g", &slope); 
	matrix->scale_factor *= slope;
	imh->scale_factor = matrix->scale_factor;
	b = (int)(intercept/slope);
	if (b != 0 && matrix->data_type!=ByteData)
	{
		npixels = matrix->xdim*matrix->ydim;
		data = (short*)matrix->data_ptr;
		if (data != NULL)
			for (i=0; i<npixels; i++) data[i] += b;
		matrix->data_min += intercept;
		matrix->data_max += intercept;
		imh->image_min = (int)(matrix->data_min/matrix->scale_factor);
		imh->image_max = (int)(matrix->data_max/matrix->scale_factor);
	}
	return matrix;
}
MatrixData *matrix_read_slice(MatrixFile *mptr, MatrixData *volume,
							  int slice, int segment)
{
	int	i, group=0, slice_count=0, z_elements;
	int view, num_views, num_projs;
	int file_pos, nblks, npixels, data_size ;
	float *fdata;
	Scan3D_subheader *scan3Dsub ;
	struct MatDir matdir;
	struct Matval val;
	Attn_subheader *attnsub ;
	Image_subheader *imagesub;
	Scan_subheader *scansub ;
	int y, line_size, line_blks;
	int u_flag = 0;
	char  *line, *p;
	MatrixData *data = NULL;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';

	if (mptr->interfile_header != NULL) 
		return interfile_read_slice(mptr->fptr, mptr->interfile_header,
			volume, slice, u_flag);

	if (matrix_find(mptr,volume->matnum,&matdir) == MY_ERROR) {
		matrix_errno = MAT_MATRIX_NOT_FOUND;
		return NULL;
	}

	/* allocate space for MatrixData structure and initialize */
	data = (MatrixData *) calloc( 1, sizeof(MatrixData)) ;
	if (!data) return NULL;
	memcpy(data,volume,sizeof(MatrixData));
	data->zdim = 1;
	data->shptr = NULL;
	data->dicom_header = NULL;
	npixels = data->xdim*data->ydim;
	group = abs(segment);

	switch(volume->mat_type) {
	case PetImage:
		free_matrix_data(data);
		mat_numdoc(volume->matnum, &val);
		return matrix_read_v2(mptr,
			mat_numcod(val.frame,slice+1,val.gate,val.data,val.bed),
			volume->mat_type);
	case Sinogram:
		free_matrix_data(data);
		mat_numdoc(volume->matnum, &val);
		return matrix_read(mptr,
			mat_numcod(val.frame,slice+1,val.gate,val.data,val.bed),
			volume->mat_type);
	case Short3dSinogram :
	case Float3dSinogram :
		scan3Dsub = (Scan3D_subheader*)volume->shptr;
		if (scan3Dsub->data_type == SunShort) {
			data_size = npixels*sizeof(short);
			line_size = data->xdim*sizeof(short);
		} else {
			data_size = npixels*sizeof(float);
			line_size = data->xdim*sizeof(float);
		}
		file_pos = (matdir.strtblk+1)*MatBLKSIZE;
		z_elements = scan3Dsub->num_z_elements[group];
		if (group > 0) z_elements /= 2;
		if (slice >= z_elements) {
			free_matrix_data(data);
			return NULL;
		}
		for (i=0; i<group; i++)
			file_pos += scan3Dsub->num_z_elements[i]*data_size;
		if (segment < 0) file_pos += z_elements*data_size;

		nblks = (data_size+(MatBLKSIZE-1))/MatBLKSIZE;
		data->data_size = data_size;
		data->data_ptr = (void *) calloc(nblks, MatBLKSIZE) ;
		if (!data->data_ptr) {
			free_matrix_data(data);
			return NULL;
		}

		line_blks = (line_size+511)/512;
		if ((line = calloc(line_blks,512)) == NULL) {
			free_matrix_data(data);
			return NULL;
		}
		data->zdim = 1;
		p = data->data_ptr;
		if (scan3Dsub->storage_order==0) {	/* view mode */
			file_pos += slice*line_size;
			for (y=0; y<data->ydim; y++) {	/* for each planar view fixed theta */
				{
						/* skip to planar view and read */
					if (fseek(mptr->fptr,file_pos,0) != 0 ||
						fread(line,line_size,1,mptr->fptr) != 1) {
						free_matrix_data(data);
						return NULL;
					}
				}
				file_data_to_host(line,line_blks,scan3Dsub->data_type);
				memcpy(p,line,line_size);
				p += line_size;
				file_pos += line_size*z_elements;
			}
		} else {			/* sinogram mode */
			file_pos += slice*data_size;
			fseek(mptr->fptr,file_pos,0);
			fread(data->data_ptr,data_size,1,mptr->fptr);
			file_data_to_host(data->data_ptr,nblks,scan3Dsub->data_type);

		}
		scansub = (Scan_subheader*)calloc(sizeof(Scan_subheader),1);
		if (scan3Dsub->data_type == SunShort) {
      scansub->scan_max = find_smax((short*)data->data_ptr, npixels);
			data->data_max = scansub->scan_max;
    }
    else {
      data->data_max = find_fmax((float*)data->data_ptr, npixels);
      data->data_max = 0; /* not applicable */
    }
		data->shptr = (void *)scansub;
		scansub->num_z_elements = 1;
		scansub->num_angles = scan3Dsub->num_angles;
		scansub->num_r_elements = scan3Dsub->num_r_elements;
		scansub->x_resolution = scan3Dsub->x_resolution;
		free(line);
		return data;
	case AttenCor:
		attnsub = (Attn_subheader*)volume->shptr;
		num_projs = attnsub->num_r_elements;
		num_views =  attnsub->num_angles;
		data->scale_factor = attnsub->scale_factor;
		data_size = num_projs*num_views*sizeof(float);
		file_pos = matdir.strtblk*MatBLKSIZE;
		z_elements = attnsub->z_elements[group];
		if (group > 0) z_elements /= 2;
		if (slice >= z_elements) {
			free_matrix_data(data);
			return NULL;
		}
		for (i=0; i<group; i++) 
			file_pos += attnsub->z_elements[i]*data_size;
		if (segment < 0) file_pos += z_elements*data_size;

		nblks = (data_size+(MatBLKSIZE-1))/MatBLKSIZE;
		data->data_size = data_size;
		if ((data->data_ptr = calloc(nblks, MatBLKSIZE)) == NULL) {
			free_matrix_data(data);
			return NULL;
		}
		fdata = (float*)data->data_ptr;
		if (attnsub->storage_order==0) {
			line_size = num_projs*sizeof(float);
			file_pos += slice*line_size;
			nblks = (line_size+511)/512;
			if ((line = malloc(nblks*512)) == NULL) {
				free_matrix_data(data);
				return NULL;
			}
			for (view=0; view<num_views; view++) {	/* for each planar view  */
				/* skip to planar view and read */
				if (fseek(mptr->fptr,file_pos,0) != 0 ||
					fread(line,sizeof(float)*num_projs,1,mptr->fptr) != 1) {
					free_matrix_data(data);
					return NULL;
				}
				file_data_to_host(line,nblks,IeeeFloat);
				memcpy(fdata+view*num_projs,line,line_size);
				file_pos += line_size*z_elements;
			}
			free(line);
		} else {
			file_pos += slice*npixels*sizeof(float);
			if (fseek(mptr->fptr,file_pos,0) != 0 ||
				fread(fdata,sizeof(float)*npixels,1,mptr->fptr) != 1) {
				free_matrix_data(data);
				return NULL;
			}	
			file_data_to_host((char*)fdata,nblks,IeeeFloat);
		}
		data->xdim = num_projs;
		data->ydim = num_views;
		data->zdim = 1;
		data->data_max = find_fmax(fdata,num_projs*num_views);
		return data;
	case ByteVolume:
	case PetVolume:
	case ByteProjection:
	case PetProjection:
		file_pos = matdir.strtblk*MatBLKSIZE;
		if (data->data_type == ByteData) data_size = npixels;
		else data_size = npixels*sizeof(short);
		file_pos += slice*data_size;
		nblks = (data_size+(MatBLKSIZE-1))/MatBLKSIZE;
		data->data_size = data_size;
		if ((data->data_ptr = calloc(nblks,MatBLKSIZE)) == NULL) {
			free_matrix_data(data);
			return NULL;
		}
		imagesub = (Image_subheader *) malloc(sizeof(Image_subheader));
		if (imagesub == NULL ||
			fseek(mptr->fptr,file_pos,0) != 0 ||
			fread(data->data_ptr,data_size,1,mptr->fptr) != 1) {
				free_matrix_data(data);
				return NULL;
		}
		file_data_to_host(data->data_ptr,nblks,data->data_type);
		memcpy(imagesub,volume->shptr,sizeof(Image_subheader));
		imagesub->z_dimension = 1;
		data->shptr = (void *)imagesub;
		if (data->data_type==ByteData)
			imagesub->image_max = find_bmax((unsigned char *)data->data_ptr,npixels);
		else imagesub->image_max = find_smax((short*)data->data_ptr,npixels);
		data->data_max = imagesub->image_max * data->scale_factor;
		return data;
	default:
		matrix_errno = MAT_UNKNOWN_FILE_TYPE;
		free_matrix_data(data);
		return NULL;
	}
}


MatrixData *matrix_read_view(MatrixFile *mptr, MatrixData *volume,
							 int view, int segment)
{
	int  i, num_views, num_projs, group=0,  num_planes;
	int storage_order, file_pos, nblks, plane_size, view_size;
	Scan3D_subheader *scan3Dsub ;
	struct MatDir matdir;
	Attn_subheader *attnsub ;
	int z, line_size, line_blks;
	short *z_elements;
	char  *line, *p;
	MatrixData *data = NULL;

	matrix_errno = MAT_OK;
	matrix_errtxt[0] = '\0';


	if (matrix_find(mptr,volume->matnum,&matdir) == MY_ERROR) {
		matrix_errno = MAT_MATRIX_NOT_FOUND;
		return NULL;
	}

	switch(volume->mat_type) {
	case Short3dSinogram :
	case Float3dSinogram :
		scan3Dsub = (Scan3D_subheader*)volume->shptr;
		file_pos = (matdir.strtblk+1)*MatBLKSIZE;
		z_elements = scan3Dsub->num_z_elements;
		num_views =  scan3Dsub->num_angles;
		num_projs = scan3Dsub->num_r_elements;
		storage_order = scan3Dsub->storage_order;
		break;

	case AttenCor:
		attnsub = (Attn_subheader*)volume->shptr;
		num_projs = attnsub->num_r_elements;
		num_views =  attnsub->num_angles;
		file_pos = matdir.strtblk*MatBLKSIZE;
		z_elements = attnsub->z_elements;
		storage_order = attnsub->storage_order;
		break;

	default:
		matrix_errno = MAT_BAD_FILE_ACCESS_MODE;
		return NULL;
	}

	if (view<0 && view >= num_views) return NULL;

	group = abs(segment);
	num_planes = z_elements[group];
	if (group > 0) num_planes /= 2;

	/* allocate space for MatrixData structure and initialize */
	data = (MatrixData *) calloc( 1, sizeof(MatrixData)) ;
	if (!data) return NULL;
	memcpy(data,volume,sizeof(MatrixData));
	data->shptr = NULL;

	if (data->data_type == SunShort)
		line_size = num_projs*sizeof(short);
	else line_size = num_projs*sizeof(float);
	plane_size = line_size*num_views;
	view_size = line_size*num_planes;
	for (i=0; i<group; i++)
		file_pos += z_elements[i]*plane_size;
	if (segment < 0) file_pos += num_planes*plane_size;

	nblks = (view_size+(MatBLKSIZE-1))/MatBLKSIZE;
	data->data_size = view_size;
	data->data_ptr = (void *) calloc(nblks, MatBLKSIZE) ;

	line_blks = (line_size+511)/512;
	if ((line = calloc(line_blks,512)) == NULL) {
		free_matrix_data(data);
		return NULL;
	}
	p = data->data_ptr;
	if (storage_order==1) {	/* sino mode */
		file_pos += view*line_size;
		for (z=0; z<num_planes; z++) {	/* for each slice */
			/* skip to planar view and read */
			if (fseek(mptr->fptr,file_pos,0) != 0 ||
				fread(line,line_size,1,mptr->fptr) != 1)
			{
				free_matrix_data(data);
				return NULL;
			}
			file_data_to_host(line,line_blks,data->data_type);
			memcpy(p,line,line_size);
			p += line_size;
			file_pos += line_size*num_views;
		}
	} else {			/* view mode */
		file_pos += view*view_size;
		fseek(mptr->fptr,file_pos,0);
		fread(data->data_ptr,view_size,1,mptr->fptr);
		file_data_to_host(data->data_ptr,nblks,data->data_type);
	}
	data->xdim = num_projs;
	data->ydim = num_planes;
	data->zdim = 1;
	if (data->data_type == SunShort)
		data->data_max = find_smax((short*)data->data_ptr,view_size/2);
	else data->data_max = find_fmax((float*)data->data_ptr, view_size/4);
	free(line);
	return data;
}
