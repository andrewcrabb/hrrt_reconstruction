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
/*
   Modification History
   05-APR-2008: Support DICOM10 ONLY
   05-AUG-2008: Add DICOM_dump()
*/

#ifndef DICOM_h
#define DICOM_h
#include <stdio.h>
#include <sys/types.h>
#include <map>
#include <vector>

using namespace std;

typedef struct _DICOMElem {
  unsigned type, offset, len;
} DICOMElem;

typedef map<unsigned, DICOMElem> DICOMGroupMap;
typedef map<unsigned,DICOMGroupMap> DICOMMap;
extern int DICOM_open(const char *filename, unsigned char *&buf, int &bufsize,
					  DICOMMap &dcm_map, int &DICOM10_flag);
extern int DICOM_get_elem(unsigned group, unsigned elem, DICOMMap &dcm_map,
						  DICOMElem &dcm_elem);
extern void DICOM_dump(DICOMMap &dcm_map, const unsigned char *buf, const char *filename);
#endif
