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
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <memory.h>
#include <strings.h>
#include <unistd.h>
#include "DICOM.h"

int sequence_number = 1;
extern int verbose;

static char *DICOM10MAGIC = "DICM";
static int DICOM10OFFSET = 128;       

// static void swaw(short *from, short *to, int len) {
//   int i=0;
//   for (i=0;i<len; i+=2) {
//     to[i] = from[i+1];
//     to[i+1] = from[i];
//   }
// }

static void DICOM_data2host(void *buf, void *dest,
                            size_t size, size_t num_items)
{
  char *tmp=NULL;
  if (size<2) {
    memcpy(dest,buf,size*num_items);
    return;
  }
  tmp = (char*)malloc(size*num_items);
  switch(size)
    {
    case 2 :	/* short ==> swab */
      swab((char*)buf,tmp ,num_items*2);
      memcpy((char*)dest, tmp, num_items*2);
      break;
    case 4 :
      swab((char*)buf,tmp,num_items*4);
      hrrt_util::swaw((short*)tmp,(short*)dest,num_items*2);
      break;
    }
  free(tmp);
}

static  unsigned S2B2(const char *s)
{
  const unsigned char  *us = (const unsigned char *)s;
  return ((us[1] << 8) + us[0]); 
}

static  const char *B2S2(unsigned u)
{
  static char tmp[4];
  tmp[0] = u & 0x7f;
  tmp[1] = (u >> 8)  & 0x7f;
  tmp[2] = '\0';
  return tmp;
}

static void DICOM_blank(char *data)
{
  char *p = NULL;
  for (p = data; *p != '\0'; p++)
    if (*p == '^') *p = ' ';
}

static int DICOM_scan_elem(unsigned char  *buf, unsigned count, unsigned offset,
                           unsigned *group, unsigned *elem, DICOMElem *dcm_elem)
{
  unsigned short us;
  unsigned int len;
  unsigned elem_spec_len = 8;
  if ((count-offset)<= elem_spec_len) return 0;
  DICOM_data2host(buf+offset,&us,2,1); *group = us;
  DICOM_data2host(buf+offset+2,&us,2,1); *elem = us;
  DICOM_data2host(buf+offset+4,&len,4,1);
  dcm_elem->len = len;
  dcm_elem->type = 0;  /* undetermined */
  dcm_elem->offset = offset+elem_spec_len;
  if (*group==0xFFFE && *elem==0xE00D) { /* end of sequence */
    return  DICOM_scan_elem(buf, count, offset+elem_spec_len,
                            group, elem, dcm_elem);
  }
  return 1;
}

static int DICOM10_scan_elem(unsigned char  *buf, unsigned count, unsigned offset,
                             unsigned *group, unsigned *elem, DICOMElem *dcm_elem)
{
  unsigned short us;
  unsigned ul;
  unsigned elem_spec_len = 8;
  if ((count-offset) <= elem_spec_len)
    return 0;
  DICOM_data2host(buf+offset, &us,2,1); *group = us;
  DICOM_data2host(buf+offset+2, &us,2,1); *elem = us;
  DICOM_data2host(buf+offset+4, &us,2,1); dcm_elem->type = us;
  DICOM_data2host(buf+offset+6, &us, 2,1); dcm_elem->len = us;
  dcm_elem->offset = offset+elem_spec_len;
  if ((dcm_elem->type==S2B2("OB") || dcm_elem->type==S2B2("OW") || dcm_elem->type==S2B2("SQ") ||
       dcm_elem->type==S2B2("UN")) && dcm_elem->len==0) { /* used next 32 bytes */
    DICOM_data2host(buf+offset+8,&ul, 4,1);
    dcm_elem->len = ul;
    dcm_elem->offset += 4;
  }
  if (*group==0xFFFE && *elem==0xE00D) { /* end of sequence */
    return  DICOM_scan_elem(buf, count, offset+elem_spec_len,
                            group, elem, dcm_elem);
  }
  return 1;
}

 
static int DICOM_get_elem(unsigned my_group, unsigned my_elem,  unsigned char  *buf, int count,
                          DICOMElem *dcm_elem)
{
  unsigned offset=0, group, elem;
  int success=0, DICOM10_flag=0;
  if (strncasecmp((char*)buf+DICOM10OFFSET,DICOM10MAGIC,strlen(DICOM10MAGIC))
      == 0)  DICOM10_flag=1;
  if (DICOM10_flag) {
    offset = DICOM10OFFSET+strlen(DICOM10MAGIC);
    success = DICOM10_scan_elem(buf, count, offset, &group, &elem, dcm_elem);
  } else {
    success = DICOM_scan_elem(buf, count, offset, &group, &elem, dcm_elem); 
  }
  while (success) {
    if (group>my_group)
      return 0;
    if (group==my_group &&  elem==my_elem)
      return 1;
    offset = dcm_elem->offset + dcm_elem->len;
    if (DICOM10_flag)
      success = DICOM10_scan_elem(buf, count, offset, &group, &elem, dcm_elem);
    else
      success = DICOM_scan_elem(buf, count, offset, &group, &elem, dcm_elem);
  }
  return 0;
}


int dicom_scan_string(int a, int b, char* data, void* header, int header_size)
{
  DICOMElem  dcm_elem;
  if (DICOM_get_elem(a,b,(unsigned char *)header,header_size, &dcm_elem)) {
    memcpy(data, (char*)header+dcm_elem.offset, dcm_elem.len);
    data[dcm_elem.len] = '\0';
    DICOM_blank(data);
    return 1;
  }
  return 0;
}

int dicom_scan_int32(int a, int b, int *value, void* header, int header_size)
{
  DICOMElem  dcm_elem;
  if (DICOM_get_elem(a,b,(unsigned char *)header,header_size, &dcm_elem)) {
    memcpy(value, (char*)header+dcm_elem.offset, 4);
    return 1;
  }
  return 0;
}

int dicom_scan_int16(int a, int b, short *value, void* header, int header_size)
{
  DICOMElem  dcm_elem;
  if (DICOM_get_elem(a,b,(unsigned char *)header,header_size, &dcm_elem)) {
    memcpy(value, (char*)header+dcm_elem.offset, 4);
    return 1;
  }
  return 0;
}
