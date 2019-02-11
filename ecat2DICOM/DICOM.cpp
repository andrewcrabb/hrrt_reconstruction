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

 Modification History
 05-APR-2008: Support DICOM10 ONLY
 05-AUG-2008: Add DICOM_dump()
*/
#include <stdlib.h>
#include <stdio.h>
#include<limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "DICOM.h"
#include <string>
#include "my_spdlog.hpp"

int sequence_number = 1;
extern int verbose;

static char *DICOM10MAGIC = "DICM";
static int DICOM10OFFSET = 128;

static void swaw(short *from, short *to, int len) {
  for (int i = 0; i < len; i += 2) {
    to[i] = from[i + 1];
    to[i + 1] = from[i];
  }
}

static void DICOM_data2host(void *buf, void *dest,
                            size_t size, size_t num_items)
{
  memcpy(dest, buf, size * num_items);
  return;
  char *tmp = (char*)malloc(size * num_items);
  switch (size)
  {
  case 2 :  /* short ==> swab */
    _swab((char*)buf, tmp , num_items * 2);
    memcpy((char*)dest, tmp, num_items * 2);
    break;
  case 4 :
    _swab((char*)buf, tmp, num_items * 4);
    swaw((short*)tmp, (short*)dest, num_items * 2);
    break;
  }
  free(tmp);
}

inline unsigned S2B2(const char *s)
{
  const unsigned char  *us = (const unsigned char *)s;
  return ((us[1] << 8) + us[0]);
}

inline const char *B2S2(unsigned u)
{
  static char tmp[4];
  tmp[0] = u & 0x7f;
  tmp[1] = (u >> 8)  & 0x7f;
  tmp[2] = '\0';
  return tmp;
}

// DICOM part 10 data types obtained from public domain
// NIH ImageJ source code.
const char *DICOM10_type(unsigned type)
{
  static map <unsigned, string> dcm_type;
  string s;
  map <unsigned, string>::iterator i;
  if (dcm_type.size() == 0) {
    dcm_type.insert(make_pair(S2B2("AE"), (s = "AE")));
    dcm_type.insert(make_pair(S2B2("AS"), (s = "AS")));
    dcm_type.insert(make_pair(S2B2("AT"), (s = "AT")));
    dcm_type.insert(make_pair(S2B2("CS"), (s = "CS")));
    dcm_type.insert(make_pair(S2B2("DA"), (s = "DA")));
    dcm_type.insert(make_pair(S2B2("DS"), (s = "DS")));
    dcm_type.insert(make_pair(S2B2("DT"), (s = "DT")));
    dcm_type.insert(make_pair(S2B2("FD"), (s = "FD")));
    dcm_type.insert(make_pair(S2B2("FL"), (s = "FL")));
    dcm_type.insert(make_pair(S2B2("IS"), (s = "IS")));
    dcm_type.insert(make_pair(S2B2("LO"), (s = "LO")));
    dcm_type.insert(make_pair(S2B2("LT"), (s = "LT")));
    dcm_type.insert(make_pair(S2B2("PN"), (s = "PN")));
    dcm_type.insert(make_pair(S2B2("SH"), (s = "SL")));
    dcm_type.insert(make_pair(S2B2("SS"), (s = "SS")));
    dcm_type.insert(make_pair(S2B2("ST"), (s = "ST")));
    dcm_type.insert(make_pair(S2B2("TM"), (s = "TM")));
    dcm_type.insert(make_pair(S2B2("UI"), (s = "UI")));
    dcm_type.insert(make_pair(S2B2("UL"), (s = "UL")));
    dcm_type.insert(make_pair(S2B2("US"), (s = "US")));
    dcm_type.insert(make_pair(S2B2("UT"), (s = "UT")));
    dcm_type.insert(make_pair(S2B2("OB"), (s = "OB")));
    dcm_type.insert(make_pair(S2B2("OW"), (s = "OW")));
    dcm_type.insert(make_pair(S2B2("SQ"), (s = "SQ")));
    dcm_type.insert(make_pair(S2B2("UN"), (s = "UN")));
    dcm_type.insert(make_pair(S2B2("QQ"), (s = "QQ")));
  }
  if ((i = dcm_type.find(type)) != dcm_type.end())
    return i->second.c_str();
  return "UNKOWN";
}

static int DICOM_scan_elem(unsigned char  *buf, int count, unsigned offset,
                           unsigned &group, unsigned &elem, DICOMElem &dcm_elem)
{
  unsigned short us;
  unsigned int len;
  int elem_spec_len = 8;
  if ((int)(count - offset) <= elem_spec_len)
    return 0;
  DICOM_data2host(buf + offset, &us, 2, 1); group = us;
  DICOM_data2host(buf + offset + 2, &us, 2, 1); elem = us;
  DICOM_data2host(buf + offset + 4, &len, 4, 1);
  dcm_elem.len = len;
  dcm_elem.type = 0;  // undetermined
  dcm_elem.offset = offset + elem_spec_len;
  if (group == 0xFFFE && elem == 0xE00D) { // end of sequence
    return  DICOM_scan_elem(buf, count, offset + elem_spec_len,
                            group, elem, dcm_elem);
  }
  return 1;
}

static int DICOM10_scan_elem(unsigned char  *buf, int count, unsigned offset,
                             unsigned &group, unsigned &elem, DICOMElem &dcm_elem)
{
  unsigned short us;
  int elem_spec_len = 8;
  if (offset > count || ((int)(count - offset) <= elem_spec_len))
    return 0;
  DICOM_data2host(buf + offset, &us, 2, 1); group = us;
  DICOM_data2host(buf + offset + 2, &us, 2, 1); elem = us;
  DICOM_data2host(buf + offset + 4, &us, 2, 1); dcm_elem.type = us;
  DICOM_data2host(buf + offset + 6, &us, 2, 1); dcm_elem.len = us;
  dcm_elem.offset = offset + elem_spec_len;
  if ((dcm_elem.type == S2B2("OB") || dcm_elem.type == S2B2("OW") || dcm_elem.type == S2B2("SQ") ||
       dcm_elem.type == S2B2("UN")) && dcm_elem.len == 0) { // used next 32 bytes
    unsigned ul;
    DICOM_data2host(buf + offset + 8, &ul, 4, 1);
    dcm_elem.len = ul;
    dcm_elem.offset += 4;
  }
  if (group == 0xFFFE && elem == 0xE00D) { // end of sequence
    return  DICOM_scan_elem(buf, count, offset + elem_spec_len,
                            group, elem, dcm_elem);
  }
  return 1;
}

static int DICOM_scan(unsigned char  *buf, int count, DICOMMap &dcm_map, int DICOM10_flag)
{
  int success;
  unsigned offset = 0, group, elem;
  DICOMElem dcm_elem;
  DICOMMap::iterator i;
  DICOMGroupMap new_group;
  DICOMGroupMap::iterator j;
  if (dcm_map.size() > 0) dcm_map.clear();
  if (DICOM10_flag) {
    offset = DICOM10OFFSET + strlen(DICOM10MAGIC);
    success = DICOM10_scan_elem(buf, count, offset, group, elem, dcm_elem);
  } else {
    success = DICOM_scan_elem(buf, count, offset, group, elem, dcm_elem);
  }
  while (success) {
    if ((i = dcm_map.find(group)) == dcm_map.end()) {
      new_group.insert(make_pair(elem, dcm_elem));
      dcm_map.insert(make_pair(group, new_group));
      new_group.clear();
    } else {
      i->second.insert(make_pair(elem, dcm_elem));
    }
    offset = dcm_elem.offset + dcm_elem.len;
    if (DICOM10_flag)
      success = DICOM10_scan_elem(buf, count, offset, group, elem, dcm_elem);
    else success = DICOM_scan_elem(buf, count, offset, group, elem, dcm_elem);
  }
  return dcm_map.size();
}

int DICOM_get_elem(unsigned group, unsigned elem,  DICOMMap &dcm_map, DICOMElem &dcm_elem)
{
  DICOMMap::iterator i = dcm_map.find(group);
  if (i != dcm_map.end()) {
    DICOMGroupMap::iterator j =  i->second.find(elem);
    if (j != i->second.end()) {
      dcm_elem = j->second;
      return 1;
    }
  }
  return 0;
}

int DICOM_open(const char *filename, unsigned char  *&buf, int &bufsize,
               DICOMMap &dcm_map, int &DICOM10_flag)
{
  struct stat st;
  int count = 0;
  FILE *fp;

  if (!filename) return 0;
  if (strlen(filename) == 0) return 0;
  if (stat(filename, &st) == -1) {
    LOG_ERROR(filename);
    return 0;
  }
  if (st.st_size == 0) {
    LOG_ERROR("{} is empty", filename);
    return 0;
  }
  if (buf == NULL) {
    if ((buf = (unsigned char *)calloc(1, st.st_size)) == NULL) {
      LOG_ERROR(filename);
      return 0;
    }
    bufsize = st.st_size;
  } else if (bufsize < st.st_size) {
    if ((buf = (unsigned char *)realloc(buf, st.st_size)) == NULL) {
      LOG_ERROR(filename);
      return 0;
    }
    bufsize = st.st_size;
  }
  if ((fp = fopen(filename, "rb")) == NULL) {
    LOG_ERROR(filename);
    return 0;
  }
  if ((count = fread(buf, 1, st.st_size, fp)) != st.st_size) {
    if (count < 0) LOG_ERROR(filename);
    else
      LOG_ERROR("Read fail : only {} of {}  bytes",
                count, st.st_size);
  }
  fclose(fp);
  if (count != st.st_size) return 0;
  if (strncasecmp((char*)buf + DICOM10OFFSET, DICOM10MAGIC, strlen(DICOM10MAGIC))
      == 0)  DICOM10_flag = 1;
  else return 0;    // DICOM10_flag=0; ONLY DICOM10 is supported
  return DICOM_scan(buf, count, dcm_map, DICOM10_flag);
}

void DICOM_dump(DICOMMap &dcm_map, const unsigned char *buf, const char *filename)
{
  DICOMMap::iterator i;
  DICOMGroupMap::iterator j;
  FILE *fp = NULL;
  if (filename != NULL) {
    if ((fp = fopen(filename, "wt")) == NULL) {
      LOG_ERROR(filename);
      return;
    }
  } else fp = stdout;
  for (i = dcm_map.begin(); i != dcm_map.end(); i++) {
    for (j = i->second.begin(); j != i->second.end(); j++) {
      fprintf(fp, "%x,%x \t %d \t %d \t %s", i->first, j->first,
              j->second.offset, j->second.len, DICOM10_type(j->second.type));
      if (j->second.type == S2B2("UL")) fprintf(fp, " %d\n", *(int*)(buf + j->second.offset));
      if (j->second.type == S2B2("ST")) fprintf(fp, " %s\n", (char*)(buf + j->second.offset));
      if (j->second.type == S2B2("SL")) fprintf(fp, " %s\n", (char*)(buf + j->second.offset));
      if (j->second.type == S2B2("CS")) fprintf(fp, " %s\n", (char*)(buf + j->second.offset));
      if (j->second.type == S2B2("SS")) fprintf(fp, " %s\n", (char*)(buf + j->second.offset));
      if (j->second.type == S2B2("LO")) fprintf(fp, " %s\n", (char*)(buf + j->second.offset));
      fprintf(fp, "\n");
    }
  }
  if (filename != NULL) fclose(fp);
}

#ifdef TEST
int main(int argc, char **argv) {
  DICOMMap dcm_map;
  DICOMMap::iterator i;
  DICOMGroupMap::iterator j;
  unsigned char  *buf = NULL;
  int buf_size = 0, DICOM10_flag = 0;

  my_spdlog::init_logging(argv[0]);
  if (argc < 2)
    return 1;
  if (DICOM_open(argv[1], buf, buf_size, dcm_map, DICOM10_flag) > 0) {
    if (DICOM10_flag)
      LOG_INFO("group,elem \t pos \t len \t type\n");
    else
      LOG_INFO("group,elem \t pos \t len\n");
    for (i = dcm_map.begin(); i != dcm_map.end(); i++) {
      for (j = i->second.begin(); j != i->second.end(); j++) {
        if (DICOM10_flag) {
          LOG_INFO("{},{} \t {} \t {} \t {}", i->first, j->first, j->second.offset, j->second.len, DICOM10_type(j->second.type));
          if (j->second.type == S2B2("UL")) LOG_INFO(" {}", *(int*)(buf + j->second.offset));
          if (j->second.type == S2B2("ST")) LOG_INFO(" {}", (char*)(buf + j->second.offset));
          if (j->second.type == S2B2("SL")) LOG_INFO(" {}", (char*)(buf + j->second.offset));
          if (j->second.type == S2B2("CS")) LOG_INFO(" {}", (char*)(buf + j->second.offset));
        } else {
          printf("{}, {} \t {} \t {} \t {}", i->first, j->first,      j->second.offset, j->second.len);
        }
      }
    }
    return 1;
  }
  return 0;
}
#endif // TEST
