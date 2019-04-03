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

#define MATRIX_RESIZE_1
#include "matrix_resize.h"

int  matrix_resize(ecat_matrix::MatrixData *mat, float pixel_size, int interp_flag) {
	int interpolate = interp_flag;
//	if (mat->data_type == ColorData)  interpolate = 0;
  switch (mat->data_type)
  {
  case MatrixData::DataType::ByteData:
//  case ColorData:
		return matrix_resize_1(mat, pixel_size, interpolate);
  case MatrixData::DataType::SunShort:
	case MatrixData::DataType::VAX_Ix2:
    return matrix_resize_2(mat, pixel_size, interpolate);
  case MatrixData::DataType::IeeeFloat:
    return matrix_resize_4(mat, pixel_size, interpolate);
  }
  return 0;
}
