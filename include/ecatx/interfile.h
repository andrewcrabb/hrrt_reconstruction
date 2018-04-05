/*
   * EcatX software is a set of routines and utility programs
   * to access ECAT(TM) data.
   *
   * Copyright (C) 1996-2008 Merence Sibomana
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
   * modify it under the terms of the GNU General Public License
   * as published by the Free Software Foundation; either version 2
   * of the License, or any later version.
   *
   * This software is distributed in the hope that it will be useful,
   * but  WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.

   * You should have received a copy of the GNU General Public License
   * along with this software; if not, write to the Free Software
   * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
# pragma once

#include "matrix.h"

typedef enum {
	VERSION_OF_KEYS,
	IMAGE_MODALITY,
	ORIGINAL_INSTITUTION,
	ORIGINATING_SYSTEM,
	NAME_OF_DATA_FILE,
	DATA_STARTING_BLOCK,
	DATA_OFFSET_IN_BYTES,
	PATIENT_NAME,
	PATIENT_ID,
	PATIENT_DOB,		/* date format is YYYY:MM:DD */
	PATIENT_SEX,
	STUDY_ID,
	EXAM_TYPE,
	DATA_COMPRESSION,
	DATA_ENCODE,
	TYPE_OF_DATA,
	TOTAL_NUMBER_OF_IMAGES,
	STUDY_DATE,
	STUDY_TIME,		/* Time Format is hh:mm:ss */
	IMAGEDATA_BYTE_ORDER,
	NUMBER_OF_WINDOWS,	/* Number of energy windows */
	NUMBER_OF_IMAGES,	/* Number of images/energy window */
	PROCESS_STATUS,
	NUMBER_OF_DIMENSIONS,
	MATRIX_SIZE_1,
	MATRIX_SIZE_2,
	MATRIX_SIZE_3,
	NUMBER_FORMAT,
	NUMBER_OF_BYTES_PER_PIXEL,
	SCALE_FACTOR_1,
	SCALE_FACTOR_2,
	SCALE_FACTOR_3,
	IMAGE_DURATION,
	IMAGE_START_TIME,
	IMAGE_NUMBER,
	LABEL,
	MAXIMUM_PIXEL_COUNT,
	TOTAL_COUNTS,
	/*
	 My Extensions
	*/
	QUANTIFICATION_UNITS,	/* scale_factor units; eg 10e-3 counts/seconds */
	COLORTAB,
	DISPLAY_RANGE,
	IMAGE_EXTREMA,
	REAL_EXTREMA,
	INTERPOLABILITY,
	MATRIX_INITIAL_ELEMENT_1,
	MATRIX_INITIAL_ELEMENT_2,
	MATRIX_INITIAL_ELEMENT_3,
	ATLAS_ORIGIN_1,
	ATLAS_ORIGIN_2,
	ATLAS_ORIGIN_3,
	TRANSFORMER,
	/*
	 Sinograms Support
	*/
	NUM_Z_ELEMENTS,   /* 3D Elements number of planes (array)
					      Number of grous = 2*(#num_z_elements)-1 */
	STORAGE_ORDER,
	IMAGE_RELATIVE_START_TIME,
	TOTAL_PROMPTS,
	TOTAL_RANDOMS,

	END_OF_INTERFILE
} InterfileKeys;

typedef enum {
	STATIC,
	DYNAMIC,
	GATED,
	TOMOGRAPHIC,
	CURVE,
	ROI,
	OTHER,
	/* My Externsion */
	MULTIBED,
	CLICHE			/* with a fixed colormap */
}	TypeOfData;

typedef enum {
	UNSIGNED_INTEGER,
	SIGNED_INTEGER,
	SHORT_FLOAT,
	LONG_FLOAT,
	/* My Extension */
	COLOR_PIXEL
} NumberFormat;

typedef struct _InterfileItem {
	int key;
	char* value;
} InterfileItem;



extern "C" {
	int interfile_write_volume(MatrixFile* mptr, char *image_name, char *header_name,
	                           unsigned char* data_matrix, int size);
	char *is_interfile(const char*);
	int interfile_open(MatrixFile*);
	MatrixData *interfile_read_slice(FILE*, char** ifh, MatrixData*, int slice,
	                                 int u_flag);
	int interfile_read(MatrixFile *mptr, int matnum, MatrixData  *data, int dtype);
	MatrixData *interfile_read_scan(MatrixFile *mptr, int matnum,
	                                int dtype, int segment);
	int free_interfile_header(char** ifh);
	void flip_x(void * line, int data_type, int xdim);
	void flip_y(void * plane, int data_type, int xdim, int ydim);
}

extern "C" InterfileItem used_keys[];
