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
#include <ctype.h>
#include "interfile.hpp"
#include "machine_indep.hpp"
#include "my_spdlog.hpp"
#include "hrrt_util.hpp"

#include <vector>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>

// #define END_OF_KEYS END_OF_INTERFILE+1
#define MAX_MULTIPLICITY 64
#define LINESIZE 512
#define DIR_SEPARATOR '/'
#define R_MODE "rb"

namespace interfile {

// InterfileItem used_keys[] = {
std::vector <InterfileItem> used_keys = {
// std::map <Key, std::string> used_keys = {
  {Key::VERSION_OF_KEYS     , "version of keys"},
  {Key::IMAGE_MODALITY      , "image modality"},
  // Main Header
  {Key::ORIGINAL_INSTITUTION, "original institution"},
  {Key::ORIGINATING_SYSTEM  , "originating system"},
  {Key::NAME_OF_DATA_FILE   , "name of data file"},
  {Key::DATA_STARTING_BLOCK , "data starting block"},
  {Key::DATA_OFFSET_IN_BYTES, "data offset in bytes"},
  {Key::PATIENT_NAME        , "patient name"},
  {Key::PATIENT_ID          , "patient id"},
  {Key::PATIENT_DOB         , "patient dob"},
  {Key::PATIENT_SEX         , "patient sex"},
  {Key::STUDY_ID            , "study id"},
  {Key::EXAM_TYPE           , "exam type"},
  {Key::DATA_COMPRESSION    , "data compression"},
  {Key::DATA_ENCODE         , "data encode"},
  {Key::DISPLAY_RANGE       , "display range"},
  {Key::IMAGE_EXTREMA       , "image extrema"},
  {Key::ATLAS_ORIGIN_1      , "atlas origin [1]"},
  {Key::ATLAS_ORIGIN_2      , "atlas origin [2]"},
  {Key::ATLAS_ORIGIN_3      , "atlas origin [3]"},
  {Key::TYPE_OF_DATA        , "type of data"},
  {Key::TOTAL_NUMBER_OF_IMAGES, "total number of images"},
  {Key::STUDY_DATE, "study date"},
  {Key::STUDY_TIME, "study time"},
  {Key::IMAGEDATA_BYTE_ORDER, "imagedata byte order"},
  {Key::NUMBER_OF_WINDOWS, "number of energy windows"},
  // static tomographic images
  {Key::NUMBER_OF_IMAGES, "number of images/energy window"},
  {Key::PROCESS_STATUS, "process status"},
  {Key::NUMBER_OF_DIMENSIONS, "number of dimensions"},
  {Key::MATRIX_SIZE_1, "matrix size [1]"},
  {Key::MATRIX_SIZE_2, "matrix size [2]"},
  {Key::MATRIX_SIZE_3, "matrix size [3]"},
  {Key::NUMBER_FORMAT, "number format"},
  {Key::NUMBER_OF_BYTES_PER_PIXEL, "number of bytes per pixel"},
  {Key::MAXIMUM_PIXEL_COUNT, "maximum pixel count"},
  {Key::MATRIX_INITIAL_ELEMENT_1, "matrix initial element [1]"},
  {Key::MATRIX_INITIAL_ELEMENT_2, "matrix initial element [2]"},
  {Key::MATRIX_INITIAL_ELEMENT_3, "matrix initial element [3]"},
  {Key::SCALE_FACTOR_1, "scaling factor (mm/pixel) [1]"},
  {Key::SCALE_FACTOR_2, "scaling factor (mm/pixel) [2]"},
  {Key::SCALE_FACTOR_3, "scaling factor (mm/pixel) [3]"},
  // ECAT 8 Image Interfile
  {Key::SCALE_FACTOR_1, "scale factor (mm/pixel) [1]"},
  {Key::SCALE_FACTOR_2, "scale factor (mm/pixel) [2]"},
  {Key::SCALE_FACTOR_3, "scale factor (mm/pixel) [3]"},
  // ECAT 8 Sinogram Interfile
  {Key::SCALE_FACTOR_2, "scale factor [2]"},
  {Key::IMAGE_DURATION, "image duration"},
  {Key::IMAGE_START_TIME, "image start time"},
  {Key::IMAGE_NUMBER, "image number"},
  {Key::LABEL, "label"},
  // not standard keys added by Sibomana@topo.ucl.ac.be : it is expressed as scale units; e.g 10e-6 counts/second
  {Key::QUANTIFICATION_UNITS, "quantification units"},
  // scale_factor and units label
  {Key::REAL_EXTREMA, "real extrema"},
  {Key::INTERPOLABILITY, "interpolability"},
  {Key::TRANSFORMER, "transformer"},
  {Key::COLORTAB, "colortab"},
  // Sinograms header support
  {Key::NUM_Z_ELEMENTS, "number of z elements"},
  {Key::STORAGE_ORDER, "storage order"},
  {Key::IMAGE_RELATIVE_START_TIME, "image relative start time"},
  {Key::TOTAL_PROMPTS, "total Prompts"},
  {Key::TOTAL_RANDOMS, "total Randoms"},
  // {Key::END_OF_INTERFILE, "end of interfile"},
  // {Key::END_OF_KEYS, 0}
};

// static char const *magicNumber = "interfile";
const std::string magicNumber = "!INTERFILE";
static char  line[LINESIZE];

static void byte_order(void * data_ptr, int elem_size, int nblks,                       char *in_order){
  int swap_order = 0;
  char *dptr = NULL, *tmp = NULL;
  int i = 0, j = 0;
  if ( in_order != NULL && (*in_order == 'b' || *in_order == 'B')) {
    if (ntohs(1) != 1) 
    swap_order = 1;  /* big to little endian */
  } else {
    if (ntohs(1) == 1) 
    swap_order = 1;  /* little to big endian */
  }
  if (swap_order) {
    tmp = static_cast<char *>(malloc(ecat_matrix::MatBLKSIZE));
    dptr = static_cast<char *>(data_ptr);
    if (elem_size == 2) {
      for (i = 0, j = 0; i < nblks; i++, j += ecat_matrix::MatBLKSIZE) {
        swab(dptr + j, tmp, ecat_matrix::MatBLKSIZE);
        memcpy(dptr + j, tmp, ecat_matrix::MatBLKSIZE);
      }
    }
    if (elem_size == 4) {
      for (i = 0, j = 0; i < nblks; i++, j += ecat_matrix::MatBLKSIZE) {
        swab(dptr + j, tmp, ecat_matrix::MatBLKSIZE);
        hrrt_util::swaw((short*)tmp, (short*)(dptr + j), ecat_matrix::MatBLKSIZE / 2);
      }
    }
    free(tmp);
  }
}

static int if_multiplicity(char **ifh, int **blk_offsets){
  int num_offset = 0;
  char *word;
  static int _offsets[MAX_MULTIPLICITY];
  _offsets[0] = 0;
  *blk_offsets = _offsets;
  if (ifh[to_underlying(Key::DATA_STARTING_BLOCK)] == NULL) 
    return 1;
  strcpy(line, ifh[to_underlying(Key::DATA_STARTING_BLOCK)]);
  num_offset = 0;
  word = strtok(line, " \t");
  while (word != NULL && num_offset < MAX_MULTIPLICITY &&         sscanf(word, "%d", &_offsets[num_offset]) == 1)  {
    num_offset++;
    word = strtok(0, " \t");
  }
  if (num_offset == 0) 
    return 1;
  return num_offset;
}

static void clean_eol(char *line){
  int len = strlen(line);
  if (len > LINESIZE - 1) {
    LOG_EXIT( "line too long :{}", line);
  }
  line[len - 1] = ' ';
}

static int _elem_size(ecat_matrix::MatrixDataType data_type){
  return ecat_matrix::matrix_data_types_.at(data_type).length;
  // switch (data_type) {
  // case ecat_matrix::MatrixDataType::ByteData :
  // case ecat_matrix::MatrixDataType::Color_8 :
  //   return 1;
  // case ecat_matrix::MatrixDataType::Color_24 :
  //   return 3;
  // case ecat_matrix::MatrixDataType::SunShort:
  // case ecat_matrix::MatrixDataType::VAX_Ix2:
  //   return 2;
  // case ecat_matrix::MatrixDataType::SunLong:
  //   return 4;
  // case ecat_matrix::MatrixDataType::IeeeFloat:
  //   return 4;
  // default:
  //   LOG_ERROR( "unkown data type, assume short int");
  //   return 2;
  // }
}

static void find_data_extrema(ecat_matrix::MatrixData *data){
  int npixels = data->xdim * data->ydim * data->zdim;
  switch (data->data_type) {
  case ecat_matrix::MatrixDataType::ByteData :
  case ecat_matrix::MatrixDataType::Color_8 :
    data->data_max = ecat_matrix::find_bmax((unsigned char *)data->data_ptr, npixels);
    data->data_min = ecat_matrix::find_bmin((unsigned char *)data->data_ptr, npixels);
    break;
  default :
  case ecat_matrix::MatrixDataType::SunShort:
  // case ecat_matrix::MatrixDataType::VAX_Ix2:
  //   data->data_max = ecat_matrix::find_smax((short*)data->data_ptr, npixels);
  //   data->data_min = ecat_matrix::find_smin((short*)data->data_ptr, npixels);
  //   break;
  case ecat_matrix::MatrixDataType::SunLong:
    data->data_max = (float)ecat_matrix::find_imax((int*)data->data_ptr, npixels);
    data->data_min = (float)ecat_matrix::find_imin((int*)data->data_ptr, npixels);
    break;
  case ecat_matrix::MatrixDataType::IeeeFloat:
    data->data_max = ecat_matrix::find_fmax((float*)data->data_ptr, npixels);
    data->data_min = ecat_matrix::find_fmin((float*)data->data_ptr, npixels);
    break;
  case ecat_matrix::MatrixDataType::Color_24 :  /* get min and max brightness */
    data->data_max = ecat_matrix::find_bmax((unsigned char *)data->data_ptr, 3 * npixels);
    data->data_min = ecat_matrix::find_bmin((unsigned char *)data->data_ptr, 3 * npixels);
  }
  data->data_max *=  data->scale_factor;
  data->data_min *=  data->scale_factor;
}

  void flip_x(void * line, ecat_matrix::MatrixDataType data_type, int xdim) {
    static void * _line = NULL;
    static int line_size = 0;
    int x = 0;
    int elem_size = _elem_size(data_type);

    if (line_size == 0) {
      line_size = xdim * elem_size;
      _line = (void *)malloc(line_size);
    } else if (xdim * elem_size > line_size) {
      line_size = xdim * elem_size;
      _line = (void *)realloc(_line, line_size);
    }
    switch (data_type) {
    case ecat_matrix::MatrixDataType::Color_8 :
    case ecat_matrix::MatrixDataType::ByteData :  {
      unsigned char  *b_p0, *b_p1;
      b_p0 = (unsigned char *)line;
      b_p1 = (unsigned char *)_line + xdim - 1;
      for (x = 0; x < xdim; x++) *b_p1-- = *b_p0++;
      memcpy(line, _line, xdim);
      break;
    }
    default :
    case ecat_matrix::MatrixDataType::SunShort:
    // case ecat_matrix::MatrixDataType::VAX_Ix2:  {
    //   short *s_p0, *s_p1;
    //   s_p0 = (short*)line;
    //   // s_p1 = (short*)(_line + (xdim - 1) * elem_size);
    //   s_p1 = static_cast<short *>(_line) + (xdim - 1) * elem_size;
    //   for (x = 0; x < xdim; x++) *s_p1-- = *s_p0++;
    //   memcpy(line, _line, xdim * elem_size);
    //   break;
    // }
    case ecat_matrix::MatrixDataType::SunLong:  {
      int *i_p0, *i_p1;
      i_p0 = (int*)line;
      i_p1 = static_cast<int *>(_line) + (xdim - 1) * elem_size;
      for (x = 0; x < xdim; x++) *i_p1-- = *i_p0++;
      memcpy(line, _line, xdim * elem_size);
      break;
    }
    case ecat_matrix::MatrixDataType::IeeeFloat:  {
      float *f_p0, *f_p1;
      f_p0 = (float*)line;
      f_p1 = static_cast<float*>(_line) + (xdim - 1) * elem_size;
      for (x = 0; x < xdim; x++) *f_p1-- = *f_p0++;
      memcpy(line, _line, xdim * elem_size);
      break;
    }
    case ecat_matrix::MatrixDataType::Color_24:  {
      unsigned char  *p0, *p1;
      p0 = (unsigned char *)line;
      p1 = static_cast<unsigned char *>(_line) + (xdim - 1) * elem_size;
      for (x = 0; x < xdim; x++)
      {
        *p1++ = *p0++;  /* red */
        *p1++ = *p0++;  /* green */
        *p1++ = *p0++;  /* blue */
        p1 -= 6;    /* go to previous pixel */
      }
      memcpy(line, _line, xdim * elem_size);
    }
    }
  }

  void flip_y(void * plane, ecat_matrix::MatrixDataType data_type, int xdim, int ydim)
{
  static void * _plane = NULL;
  static int plane_size = 0;
  // void *p0, *p1;
  uint8_t *p0, *p1;
  int elem_size = _elem_size(data_type);
  int y = 0;

  if (plane_size == 0) {
    plane_size = xdim * ydim * elem_size;
    _plane = (void *)malloc(plane_size);
  } else if (xdim * ydim * elem_size > plane_size) {
    plane_size = xdim * ydim * elem_size;
    _plane = (void *)realloc(_plane, plane_size);
  }
  p0 = static_cast<uint8_t *>(plane);
  p1 = static_cast<uint8_t *>(_plane) + (ydim - 1) * xdim * elem_size;
  for (y = 0; y < ydim; y++) {
    memcpy(p1, p0, xdim * elem_size);
    p0 += xdim * elem_size;
    p1 -= xdim * elem_size;
  }
  memcpy(plane, _plane, xdim * ydim * elem_size);
}

static char* get_list(FILE *fp, char *str)
{
  int end_of_table = 0;
  char *p, *ret, **lines;
  float r, g, b;
  int i, line_count = 0, cc = 0;

  if ( (p = strchr(str, '<')) != NULL) p++;
  else return NULL;
  lines = (char**) calloc(256, sizeof(char*));
  if (sscanf(p, "%g %g %g", &r, &g, &b) == 3) { /*valid entry */
    cc += strlen(p);
    lines[line_count++] = strdup(p);
  }
  while (!end_of_table && fgets(line, LINESIZE - 1, fp) != NULL) {
    clean_eol(line);
    if ((p = strchr(line, ';')) != NULL) * p = ' ';
    if ((p = strchr(line, '>')) != NULL) {  /* end of table */
      *p = '\0';
      end_of_table = 1;
    }
    if (sscanf(line, "%g %g %g", &r, &g, &b) == 3) { /*valid entry */
      cc += strlen(line);
      lines[line_count++] = strdup(line);
    }
  }
  ret = static_cast<char *>(malloc(cc + 1));
  strcpy(ret, lines[0]); free(lines[0]);
  for (i = 1; i < line_count; i++) {
    strcat(ret, lines[i]); free(lines[i]);
  }
  free(lines);
  return ret;
}

static InterfileItem  *get_next_item(FILE *fp) {
  char *key_str, *val_str, *end;
  // InterfileItem* item;

  while (fgets(line, LINESIZE - 1, fp) != NULL) {
    clean_eol(line);
    key_str = line;
    while (*key_str && (isspace(*key_str) || *key_str == '!'))
      key_str++;
    if (*key_str == ';') {
      if (key_str[1] != '%')
        continue;
      else
        key_str += 2;
    }
    if ( (val_str = strchr(key_str, ':')) == NULL )
      continue;  // invalid line; skip
    *val_str++ = '\0';
    if (*val_str == '=')
      val_str++;

    // clean up key_str and val_str
    end = key_str + strlen(key_str) - 1;
    while (isspace(*end))
      *end-- = '\0';
    while (isspace(*val_str))
      val_str++;
    end = val_str + strlen(val_str) - 1;
    while (isspace(*end))
      *end-- = '\0';
    for (end = key_str; *end != '\0'; end++)
      *end = tolower(*end);

// find key
    // for (item=used_keys; item->value!=NULL; item++)
    // for (auto item = used_keys.begin(); item != used_keys.end(); std::advance(item, 1))
    //   if (item->value.compare(key_str) == 0) {
    static InterfileItem ret;
    std::string key_string(key_str);
    // std::map<Key, std::string>::iterator item = used_keys.find(key_str);
    std::vector<InterfileItem>::iterator item = find_if(used_keys.begin(), used_keys.end(), [&key_string] (const InterfileItem& item) { return item.value == key_string; } );
    if (item != used_keys.end()) {
        ret.key = item->key;
        if (ret.key == Key::TRANSFORMER || ret.key == Key::COLORTAB ) {
          ret.value = get_list(fp, val_str);
        } else {
          if (strlen(val_str) > 0)
            ret.value = val_str;
        }
        return &ret;
      }
  }
  return NULL;
}

// char* _is_interfile_old(const char* fname) {
//   int c;
//   FILE *fp;
//   char *p = magicNumber;

//   if ( (fp = fopen(fname, R_MODE)) == NULL) return 0;

// /* skip spaces */
//   while ( (c = fgetc(fp)) != EOF)
//     if (!isspace(c)) break;

// /*  check magic */
//   if (c != EOF) {
//     if (c != '!' && *p++ != tolower(c)) {
//       fclose(fp);
//       return 0;
//     }
//     while ( (c = fgetc(fp)) != EOF) {
//       if (*p++ != tolower(c)) break;
//       if (*p == '\0') {       /* ecat_matrix::ECATX_OK */
//         fclose(fp);
//         return strdup(fname);
//       }
//     }
//   }
//   fclose(fp);
//   return 0;
// }

int _is_interfile(std::string const &fname) {
  std::ifstream fs(fname);
  std::string line;
  int ret = 0;
  while (std::getline(fs, line)) {
    boost::trim(line);
    boost::to_upper(line);
    if (line.compare(magicNumber) == 0) {
      ret = 1;
      break;
    }
  }
  return ret;
}

// What the flying f does this thing do.  It returns a char * but only called once.

char* is_interfile(const char* fname) {
  char *hdr_fname = NULL, *ext = NULL;
  const char *img_fname = NULL;
  InterfileItem* item;
  FILE *fp;

  if (_is_interfile(fname))
    return strdup(fname);
  /* assume data_fname and check header */
  if ( (img_fname = strrchr(fname, DIR_SEPARATOR)) == NULL)
    img_fname = fname;
  else
    img_fname++;
  hdr_fname = static_cast<char *>(malloc(strlen(fname) + 5)); /* +5 : strlen(.hdr)+1 */
  sprintf(hdr_fname, "%s.hdr", fname);
  if (access(hdr_fname, F_OK) != 0) { /* try .hdr extension */
    strcpy(hdr_fname, fname);
    if ((ext = strrchr(hdr_fname, '.')) == NULL) {
      free(hdr_fname);
      return NULL;
    }
    strcpy(ext, ".hdr");
    if (access(hdr_fname, F_OK) != 0) { /* try .h33 extension */
      strcpy(ext, ".h33");
      if (access(hdr_fname, F_OK) != 0) {
        free(hdr_fname);
        return NULL;
      }
    }
  }
  if (_is_interfile(hdr_fname) && (fp = fopen(hdr_fname, R_MODE)) != NULL) {
    while ((item = get_next_item(fp)) != NULL) {
      if (item->value.length() == 0)
        continue;
      if (item->key == Key::NAME_OF_DATA_FILE) {
        /* check short and full name */
        // if (strcmp(item->value, img_fname) == 0 || strcmp(item->value, fname) == 0) {
        if ((item->value.compare(img_fname) == 0) || item->value.compare(fname) == 0) {
          fclose(fp);
          LOG_ERROR( "using {} header for {} data file", hdr_fname, fname);
          // free(item->value);
          return hdr_fname;
        }
      }
      // free(item->value);
    }
    fclose(fp);
    LOG_ERROR( "using {} header for {} data file", hdr_fname, fname);
    LOG_ERROR( "Warning: Adding Missing keyword 'name of data file'");
    if ((fp = fopen(hdr_fname, "at")) != NULL) {
      fprintf(fp, fmt::format("name of data file := {}", fname).c_str());
      fclose(fp);
    } else {
      LOG_ERROR( "Can't open {} header for appending data file", hdr_fname);
      free(hdr_fname);
      return NULL;
    }
    return hdr_fname;
  }
  return NULL;
}

int unmap_interfile_header(char **ifh, ecat_matrix::MatrixData *mdata) {
  int  sx, sy, sz = 1, dim = 2, elem_size = 2;
  int big_endian = 0;
  float f;
  char *p;

  if (ifh[to_underlying(Key::NUMBER_OF_DIMENSIONS)] != NULL) {
    sscanf(ifh[to_underlying(Key::NUMBER_OF_DIMENSIONS)], "%d", &dim);
    if (dim != 2 && dim != 3) {
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
      return ecat_matrix::ECATX_ERROR;
    }
  }
  if ( (p = ifh[to_underlying(Key::IMAGEDATA_BYTE_ORDER)]) != NULL && (*p == 'b' || *p == 'B'))
    big_endian = 1;
  sscanf(ifh[to_underlying(Key::NUMBER_OF_BYTES_PER_PIXEL)], "%d", &elem_size); /* tested in interfile_open */
  if (ifh[to_underlying(Key::NUMBER_FORMAT)] && strstr(ifh[to_underlying(Key::NUMBER_FORMAT)], "float")) {
    if (elem_size != 4) {
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
      return ecat_matrix::ECATX_ERROR;
    }
    mdata->data_type = ecat_matrix::MatrixDataType::IeeeFloat;
  } else if (ifh[to_underlying(Key::NUMBER_FORMAT)] && strstr(ifh[to_underlying(Key::NUMBER_FORMAT)], "rgb")) {
    if (elem_size != 3) {
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
      return ecat_matrix::ECATX_ERROR;
    }
    mdata->data_type = ecat_matrix::MatrixDataType::Color_24;
  } else {  /* integer data type */
    if (elem_size != 1 && elem_size != 2 && elem_size != 4) {
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
      return ecat_matrix::ECATX_ERROR;
    }
    if (elem_size == 1) mdata->data_type = ecat_matrix::MatrixDataType::ByteData;
    if (big_endian) {
      if (elem_size == 2) mdata->data_type = ecat_matrix::MatrixDataType::SunShort;
      if (elem_size == 4) mdata->data_type = ecat_matrix::MatrixDataType::SunLong;
    } else {
      if (elem_size == 2) mdata->data_type = ecat_matrix::MatrixDataType::VAX_Ix2;
      if (elem_size == 4) mdata->data_type = ecat_matrix::MatrixDataType::VAX_Ix4;
    }
  }
  if (ifh[to_underlying(Key::MATRIX_SIZE_1)] == NULL ||
      sscanf(ifh[to_underlying(Key::MATRIX_SIZE_1)], "%d", &sx) != 1)
    ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  else if (ifh[to_underlying(Key::MATRIX_SIZE_2)] == NULL ||
           sscanf(ifh[to_underlying(Key::MATRIX_SIZE_2)], "%d", &sy) != 1)
    ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  else  if (dim == 3)  {
    if (ifh[to_underlying(Key::MATRIX_SIZE_3)] == NULL || sscanf(ifh[to_underlying(Key::MATRIX_SIZE_3)], "%d", &sz) != 1)
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  }
  if (ifh[to_underlying(Key::NUMBER_OF_IMAGES)] != NULL) {
    if (sscanf(ifh[to_underlying(Key::NUMBER_OF_IMAGES)], "%d", &sz) != 1)
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  }
  if (ecat_matrix::matrix_errno != ecat_matrix::MatrixError::OK) 
    return ecat_matrix::ECATX_ERROR;
  mdata->xdim = sx;
  mdata->ydim = sy;
  mdata->zdim = sz;
  if (ifh[to_underlying(Key::QUANTIFICATION_UNITS)] != NULL)
    sscanf(ifh[to_underlying(Key::QUANTIFICATION_UNITS)], "%g", &mdata->scale_factor);
  mdata->data_min = mdata->data_max = 0;
  if (ifh[to_underlying(Key::SCALE_FACTOR_1)] && sscanf(ifh[to_underlying(Key::SCALE_FACTOR_1)], "%g", &f) == 1)
    mdata->pixel_size = f / 10.0f;  /* mm to cm */
  if (ifh[to_underlying(Key::SCALE_FACTOR_2)] && sscanf(ifh[to_underlying(Key::SCALE_FACTOR_2)], "%g", &f) == 1)
    mdata->y_size = f / 10.0f;  /* mm to cm */
  if (ifh[to_underlying(Key::SCALE_FACTOR_3)] && sscanf(ifh[to_underlying(Key::SCALE_FACTOR_3)], "%g", &f) == 1)
    mdata->z_size = f / 10.0f;  /* mm to cm */
  return 1;
}

int free_interfile_header(char **ifh) {
  // int i = 0;
  // for (i = 0; i < END_OF_KEYS; i++)
  for (size_t i = 0; i < used_keys.size(); i++)
    if (ifh[i] != NULL)
      free(ifh[i]);
  // for (auto &item : used_keys)
  //   if (item != NULL)
  //     free(item);
  free(ifh);
  return 1;
}

int get_block_singles(ecat_matrix::MatrixFile *mptr, float **pextended_uncor_singles, int *pnum_extended_uncor_singles) {
  int tmp_size = 128, max_size = 1024;
  FILE *fp;
  float *tmp = NULL;
  int block_num, count = 0;
  long block_singles;
  char *key_str, *val_str;
  if ((fp = fopen(mptr->fname, R_MODE)) == NULL) return ecat_matrix::ECATX_ERROR;
  tmp = (float*)calloc(tmp_size, sizeof(float));
  while (fgets(line, LINESIZE - 1, fp) != NULL) {
    clean_eol(line);
    key_str = line;
    while (*key_str && (isspace(*key_str) || *key_str == '!')) key_str++;
    if (*key_str == ';') {
      if (key_str[1] != '%') continue;  /* comment line */
      else key_str += 2;         /* My extension */
    }
    if ( (val_str = strchr(key_str, ':')) == NULL )  continue; /* invalid line */
    *val_str++ = '\0';
    if (*val_str == '=') val_str++;
    if (sscanf(key_str, "block singles %d", &block_num) != 1 ||
        sscanf(val_str, "%ld", &block_singles) != 1) continue;
    if (block_num >= max_size) continue;
    while (block_num >= tmp_size) {
      tmp_size *= 2;
      tmp = static_cast<float *>(realloc(tmp, tmp_size * sizeof(float)));
    }
    tmp[block_num] = (float)block_singles;
    if (count <= block_num)
      count = block_num + 1; /* numbered 0 ... N */
  }
  if (count > 0) {
    if (*pextended_uncor_singles) free(*pextended_uncor_singles);
    *pextended_uncor_singles = (float*)malloc(count * sizeof(float));
    memcpy(*pextended_uncor_singles, tmp, count * sizeof(float));
    *pnum_extended_uncor_singles = count;
  }
  free(tmp);
  return count > 0 ? ecat_matrix::ECATX_OK : ecat_matrix::ECATX_ERROR;
}

int interfile_open(ecat_matrix::MatrixFile *mptr) {
  InterfileItem* item;
  FILE *fp;
  ecat_matrix::Main_header *mh;
  ecat_matrix::MatrixData mdata;
  ecat_matrix::MatDir matdir;
  time_t now, t;
  struct tm tm;
  char *p, dup[80], data_dir[FILENAME_MAX], data_file[FILENAME_MAX];
  char *year, *month, *day, *hour, *minute, *second;
  /* char *block_singles = NULL; */
  int this_year;
  int i, elem_size = 2, data_size;
  float scale_factor;
  int end_of_interfile = 0;
  int nmats, nblks, *blk_offsets;
  int make_path = 0;

  memset(&mdata, 0, sizeof(ecat_matrix::MatrixData));
  now = time(0);
  this_year = 1900 + (localtime(&now))->tm_year;
  if ((fp = fopen(mptr->fname, R_MODE)) == NULL) return ecat_matrix::ECATX_ERROR;
  mh = mptr->mhptr;
  strcpy(mh->data_units, "none");
  // mh->calibration_units = 2; /* Processed */
  mh->calibration_units = ecat_matrix::CalibrationStatus::Processed;
  mh->calibration_factor = 1.0f;
  mh->calibration_units_label = 0;
  strcpy(mh->magic_number, magicNumber);
  mh->sw_version = 70;
  // mptr->interfile_header = (char**)calloc(END_OF_KEYS, sizeof(char*));
  mptr->interfile_header = (char**)calloc(used_keys.size(), sizeof(char*));
  if (mptr->interfile_header == NULL) 
    return ecat_matrix::ECATX_ERROR;
  mh->num_frames = mh->num_gates = mh->num_bed_pos = 1;
  mh->plane_separation = 1;
  while (!end_of_interfile && (item = get_next_item(fp)) != NULL) {
    if (item->value == NULL) 
      continue;
    mptr->interfile_header[item->key] = item->value;
    switch (item->key) {
    case ORIGINATING_SYSTEM:
      mh->system_type = atoi(item->value);
      break;
    case QUANTIFICATION_UNITS:
      mh->calibration_units_label = 0;  /* for multiple keys */
      if (sscanf(item->value, "%g %s", &scale_factor, mh->data_units) == 2) {
        auto it = std::find(customDisplayUnits.begin(), customDisplayUnits.end(), mh->data_units);
        if (it != customDisplayUnits.end()) {
          int index = it - customDisplayUnits.begin();
          mh->calibration_units_label = index;
        }
        // for (i = 0; i < numDisplayUnits; i++)
        //   if (strcmp(mh->data_units, customDisplayUnits[i]) == 0)
        //     mh->calibration_units_label = i;
      }
      break;
    case EXAM_TYPE:
      strncpy(mh->radiopharmaceutical, item->value, sizeof(mh->facility_name));
      mh->radiopharmaceutical[sizeof(mh->facility_name) - 1] = 0;
      break;
    case ORIGINAL_INSTITUTION:
      strncpy(mh->facility_name, item->value, sizeof(mh->facility_name));
      mh->facility_name[sizeof(mh->facility_name) - 1] = '\0';
      break;
    case PATIENT_NAME:
      strncpy(mh->patient_name, item->value, sizeof(mh->patient_name));
      mh->patient_name[sizeof(mh->patient_name) - 1] = '\0';
      break;
    case PATIENT_ID:
      strncpy(mh->patient_id, item->value, sizeof(mh->patient_id));
      mh->patient_id[sizeof(mh->patient_id) - 1] = '\0';
      break;
    case PATIENT_DOB:
      strcpy(dup, item->value);
      if ( (year = strtok(dup, ":")) == NULL) break;
      mh->patient_age = (float)(this_year - atoi(year));
      if ( (month = strtok(NULL, ":")) == NULL) break;
      if ( (day = strtok(NULL, ":")) == NULL) break;
      memset(&tm, 0, sizeof(tm));
      tm.tm_year = atoi(year) - 1900;
      tm.tm_mon = atoi(month) - 1;
      tm.tm_mday = atoi(day);
      mh->patient_birth_date = (int)mktime(&tm);
      break;
    case PATIENT_SEX:
      mh->patient_sex[0] = item->value[0];
      break;
    case STUDY_ID:
      strncpy(mh->study_name, item->value, 12);
      mh->study_name[11] = '\0';
    case STUDY_DATE:
      strcpy(dup, item->value);
      if ( (year = strtok(dup, ":")) == NULL) break;
      if ( (month = strtok(NULL, ":")) == NULL) break;
      if ( (day = strtok(NULL, ":")) == NULL) break;
      memset(&tm, 0, sizeof(tm));
      tm.tm_year = atoi(year) - 1900;
      tm.tm_mon = atoi(month) - 1;
      tm.tm_mday = atoi(day);
      mh->scan_start_time = (int)mktime(&tm);
      break;

    case STUDY_TIME:
      strcpy(dup, item->value);
      if ( (hour = strtok(dup, ":")) == NULL) break;
      if ( (minute = strtok(NULL, ":")) == NULL) break;
      if ( (second = strtok(NULL, ":")) == NULL) break;
      t = mh->scan_start_time;
      memcpy(&tm, localtime(&t), sizeof(tm));
      tm.tm_hour = atoi(hour);
      tm.tm_min = atoi(minute);
      tm.tm_sec = atoi(second);
      mh->scan_start_time = (int)mktime(&tm);
      break;
    case NUMBER_OF_IMAGES :
    case MATRIX_SIZE_3 :
      mh->num_planes = atoi(item->value);
      break;
    case SCALE_FACTOR_3:
      mh->plane_separation = (float)atof(item->value) / 10.f; /* mm to cm */
      break;
    case NUMBER_OF_BYTES_PER_PIXEL:
      elem_size = atoi(item->value);
      break;
    // case END_OF_INTERFILE:
    //   end_of_interfile = 1;
    }
  }
  fclose(fp);

  if (elem_size != 1 && elem_size != 2 && elem_size != 3 && elem_size != 4) {
    ecat_matrix::matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
    free_interfile_header(mptr->interfile_header);
    return ecat_matrix::ECATX_ERROR;
  }
  if (mptr->interfile_header[NUM_Z_ELEMENTS] != NULL) {
    switch (elem_size) {
    case 1 : mptr->mhptr->file_type = ecat_matrix::DataSetType::Byte3dSinogram; break;
    case 2 : mptr->mhptr->file_type = ecat_matrix::DataSetType::Short3dSinogram; break;
    case 4 : mptr->mhptr->file_type = ecat_matrix::DataSetType::Float3dSinogram; break;
    }
  } else {
    mh->file_type = ecat_matrix::DataSetType::PetVolume;
  }

  if (mptr->interfile_header[to_underlying(Key::NAME_OF_DATA_FILE)] != NULL) {
    strcpy(data_file, mptr->interfile_header[to_underlying(Key::NAME_OF_DATA_FILE)]);
    strcpy(data_dir, mptr->fname);
    if ( (p = strrchr(data_dir, DIR_SEPARATOR)) != NULL) * p = '\0';
    else strcpy(data_dir, ".");
    if ( (p = strrchr(data_file, DIR_SEPARATOR)) == NULL) {
      /* build fullpath filename */
      sprintf(data_file, "%s%c%s", data_dir, DIR_SEPARATOR,
              mptr->interfile_header[to_underlying(Key::NAME_OF_DATA_FILE)]);
      make_path = 1;
    }
    if ( (mptr->fptr = fopen(data_file, R_MODE)) != NULL) {
      if (make_path)
        mptr->interfile_header[to_underlying(Key::NAME_OF_DATA_FILE)] = strdup(data_file);
    } else {
      LOG_ERROR(data_file);
      free_interfile_header(mptr->interfile_header);
      return ecat_matrix::ECATX_ERROR;
    }
  } else return ecat_matrix::ECATX_ERROR;

  if (unmap_interfile_header(mptr->interfile_header, &mdata) == ecat_matrix::ECATX_ERROR) {
    free_interfile_header(mptr->interfile_header);
    return ecat_matrix::ECATX_ERROR;
  }
  nmats = if_multiplicity(mptr->interfile_header, &blk_offsets);
  data_size = mdata.xdim * mdata.ydim * mdata.zdim * elem_size;
  nblks = (data_size + ecat_matrix::MatBLKSIZE - 1) / ecat_matrix::MatBLKSIZE;
  mptr->dirlist = (ecat_matrix::MatDirList *) calloc(1, sizeof(ecat_matrix::MatDirList)) ;
  for (i = 0; i < nmats; i++) {
    matdir.matnum = ecat_matrix::mat_numcod(i + 1, 1, 1, 0, 0);
    matdir.strtblk = blk_offsets[i];
    matdir.endblk = matdir.strtblk + nblks;
    insert_mdir(&matdir, mptr->dirlist) ;
  }
  return ecat_matrix::ECATX_OK;
}

int interfile_read(ecat_matrix::MatrixFile *mptr, int matnum, ecat_matrix::MatrixData *data, ecat_matrix::MatrixDataType_64 dtype) {
  int y, z;
  size_t i, npixels, nvoxels;
  int tmp, nblks, elem_size = 2, data_offset = 0;
  void *plane, *line;
  unsigned short u_max, *up = NULL;
  short *sp = NULL;
  int z_flip = 0, y_flip = 0, x_flip = 0;
  float f;
  unsigned time_msec = 0;
  char **ifh, *p, *dup;
  int group = 0;
  int z_elements;
  int hour, min, sec;
  int bytesread = 0;
  int num_read;

  data->matnum = matnum;
  data->pixel_size = data->y_size = data->z_size = 1.0;
  data->scale_factor = 1.0;
  ifh = mptr->interfile_header;
  unmap_interfile_header(ifh, data);
  int image_min = 0, image_max = 0;
  if (ifh[to_underlying(Key::MAXIMUM_PIXEL_COUNT])
    sscanf(ifh[to_underlying(Key::MAXIMUM_PIXEL_COUNT], "%d", &image_max);
  if (ifh[to_underlying(Key::IMAGE_EXTREMA])
    sscanf(ifh[to_underlying(Key::IMAGE_EXTREMA], "%d %d", &image_min, &image_max);
  data->data_min = data->scale_factor * image_min;
  data->data_max = data->scale_factor * image_max;
  if (ifh[to_underlying(Key::REAL_EXTREMA])
    sscanf(ifh[to_underlying(Key::REAL_EXTREMA], "%g %g", &data->data_min, &data->data_max);
  sscanf(ifh[to_underlying(Key::NUMBER_OF_BYTES_PER_PIXEL)], "%d", &elem_size); /* tested in interfile_open */

  switch (mptr->mhptr->file_type) {
  case ecat_matrix::DataSetType::Byte3dSinogram:
  case ecat_matrix::DataSetType::Short3dSinogram:
  case ecat_matrix::DataSetType::Float3dSinogram:
    ecat_matrix::Scan3D_subheader *scan3Dsub = (ecat_matrix::Scan3D_subheader*)data->shptr;
    memset(scan3Dsub, 0, sizeof(ecat_matrix::Scan3D_subheader));
    scan3Dsub->num_dimensions = 3;
    scan3Dsub->num_r_elements = data->xdim;
    scan3Dsub->num_angles = data->ydim;
    int storage_order = 1;    /* default is sino mode */
    if (ifh[to_underlying(Key::STORAGE_ORDER])
      sscanf(ifh[to_underlying(Key::STORAGE_ORDER], "%d", &storage_order);
    scan3Dsub->storage_order = storage_order;
    dup = strdup(mptr->interfile_header[NUM_Z_ELEMENTS]);
    p = strtok(dup, " ");
    while (p) {
      sscanf(p, "%d", &z_elements);
      scan3Dsub->num_z_elements[group++] = z_elements;
      p = strtok(NULL, " ");
    }
    free(dup);
    if (get_block_singles(mptr, &scan3Dsub->extended_uncor_singles, &scan3Dsub->num_extended_uncor_singles) == ecat_matrix::ECATX_OK) {
      int max_count = scan3Dsub->num_extended_uncor_singles < 128 ? scan3Dsub->num_extended_uncor_singles : 128;
      memcpy(scan3Dsub->uncor_singles, scan3Dsub->extended_uncor_singles, max_count * sizeof(float));
    }
    if (ifh[to_underlying(Key::TOTAL_PROMPTS])
      sscanf(ifh[to_underlying(Key::TOTAL_PROMPTS], "%d", &scan3Dsub->prompts);
    if (ifh[to_underlying(Key::TOTAL_RANDOMS])
      sscanf(ifh[to_underlying(Key::TOTAL_RANDOMS], "%d", &scan3Dsub->delayed);
    if (ifh[to_underlying(Key::IMAGE_DURATION] && sscanf(ifh[to_underlying(Key::IMAGE_DURATION], "%d", (int *)&time_msec) == 1)
      scan3Dsub->frame_duration = time_msec;
    if (ifh[to_underlying(Key::IMAGE_START_TIME] && sscanf(ifh[to_underlying(Key::IMAGE_START_TIME], "%d:%d:%d", &hour, &min, &sec) == 3) {
      scan3Dsub->frame_start_time = sec + 60 * (min + 60 * hour);
    } else if (ifh[to_underlying(Key::IMAGE_RELATIVE_START_TIME] && sscanf(ifh[to_underlying(Key::IMAGE_RELATIVE_START_TIME], "%d", &sec) == 1) {
      scan3Dsub->frame_start_time =  sec;
    }
    scan3Dsub->scale_factor = data->scale_factor;
    scan3Dsub->x_resolution = data->pixel_size;
    scan3Dsub->z_resolution = data->z_size;;
    scan3Dsub->data_type = data->data_type;
    if (data->data_max > 0.0)
      scan3Dsub->scan_max = (int)(data->scale_factor / data->data_max);
    break;
  default:
    // wtf imagesub is not used?
    ecat_matrix::Image_subheader *imagesub = (ecat_matrix::Image_subheader*)data->shptr;
    memset(imagesub, 0, sizeof(ecat_matrix::Image_subheader));
    imagesub->num_dimensions = 3;
    imagesub->x_pixel_size = data->pixel_size;
    imagesub->y_pixel_size = data->y_size;
    imagesub->z_pixel_size = data->z_size;
    imagesub->scale_factor = data->scale_factor;
    imagesub->x_dimension = data->xdim;
    imagesub->y_dimension = data->ydim;
    imagesub->z_dimension = data->zdim;
    imagesub->data_type = data->data_type;
    if (ifh[to_underlying(Key::IMAGE_DURATION] && sscanf(ifh[to_underlying(Key::IMAGE_DURATION], "%d", (int *)&time_msec) == 1)
      imagesub->frame_duration = time_msec;
    if (ifh[to_underlying(Key::IMAGE_START_TIME] &&
        sscanf(ifh[to_underlying(Key::IMAGE_START_TIME], "%d:%d:%d", &hour, &min, &sec) == 3)
      imagesub->frame_start_time = sec + 60 * (min + 60 * hour);
  }  // giant switch statement.

  if (ifh[to_underlying(Key::ATLAS_ORIGIN_1] && sscanf(ifh[to_underlying(Key::ATLAS_ORIGIN_1], "%g", &f) == 1) {
    data->x_origin = f * data->pixel_size;
  }
  if (ifh[to_underlying(Key::ATLAS_ORIGIN_2] && sscanf(ifh[to_underlying(Key::ATLAS_ORIGIN_2], "%g", &f) == 1) {
    data->y_origin = f * data->y_size;
  }
  if (ifh[to_underlying(Key::ATLAS_ORIGIN_3] && sscanf(ifh[to_underlying(Key::ATLAS_ORIGIN_3], "%g", &f) == 1) {
    data->z_origin = f  * data->z_size;
  }

  if (dtype == ecat_matrix::MatrixDataType::MAT_SUB_HEADER)
    return ecat_matrix::ECATX_OK;

  /* compute extrema */
  if (ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_3] && *ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_3] == 'i') {
    z_flip = 1;
    if (data->z_origin > 0)
      data->z_origin = (data->zdim - 1) * data->z_size - data->z_origin;
    LOG_ERROR( "volume z direction is changed to superior->inferior");
  }
  if (ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_2] &&
      *ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_2] == 'p') {
    y_flip = 1;
    if (data->y_origin > 0)
      data->y_origin = (data->ydim - 1) * data->y_size - data->y_origin;
    LOG_ERROR( "volume y direction is changed to anterior->posterior");
  }
  if (ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_1] && *ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_1] == 'r') {
    x_flip = 1;
    if (data->x_origin > 0)
      data->x_origin = (data->xdim - 1) * data->pixel_size - data->x_origin;
    LOG_ERROR( "volume x direction is changed to left->right");
  }
  npixels = data->xdim * data->ydim;
  nvoxels = npixels * data->zdim;
  data->data_size = nvoxels * elem_size;
  nblks = (data->data_size + ecat_matrix::MatBLKSIZE - 1) / ecat_matrix::MatBLKSIZE;
  data->data_ptr = (void *) calloc(nblks, ecat_matrix::MatBLKSIZE);
  if (ifh[to_underlying(Key::DATA_STARTING_BLOCK] && sscanf(ifh[to_underlying(Key::DATA_STARTING_BLOCK], "%d", &data_offset) ) {
    if (data_offset < 0)
      data_offset = 0;
    else
      data_offset *= ecat_matrix::MatBLKSIZE;
  }
  if (data_offset == 0 && ifh[to_underlying(Key::DATA_OFFSET_IN_BYTES] && sscanf(ifh[to_underlying(Key::DATA_OFFSET_IN_BYTES], "%d", &data_offset) ) {
    if (data_offset < 0)
      data_offset = 0;
  }
  if (data_offset > 0) {
    if (fseek(mptr->fptr, data_offset, SEEK_SET) != 0)
      LOG_EXIT("Error skeeping to offset {}", data_offset);
  }
  for (z = 0; z < data->zdim; z++) {
    if (z_flip)
      plane = data->data_ptr + (data->zdim - z - 1) * elem_size * npixels;
    else
      plane = data->data_ptr + z * elem_size * npixels;
    num_read = fread(plane, elem_size, npixels, mptr->fptr);
    bytesread += (elem_size * npixels);
    if (num_read < npixels) {
      int req_size = elem_size * npixels;
      LOG_ERROR( "interfile_read ERROR reading z {} of {}, got {} of {} bytes in fread, total bytes read = {}", z, data->zdim, num_read, req_size, bytesread);
      free(data->data_ptr);
      data->data_ptr = NULL;
      ecat_matrix::matrix_errno = ecat_matrix::MatrixError::READ_ERROR;
      return ecat_matrix::ECATX_ERROR;
    }
    if (y_flip)
      flip_y(plane, data->data_type, data->xdim, data->ydim);
    if (x_flip) {
      for (y = 0; y < data->ydim; y++) {
        line = plane + y * data->xdim * elem_size;
        flip_x(line, data->data_type, data->xdim);
      }
    }
  }

  byte_order(data->data_ptr, elem_size, nblks, ifh[to_underlying(Key::IMAGEDATA_BYTE_ORDER)]);

  if (elem_size == 2 &&
      ifh[to_underlying(Key::NUMBER_FORMAT)] && strstr(ifh[to_underlying(Key::NUMBER_FORMAT)], "unsigned") ) {
    up = (unsigned short*)data->data_ptr;
    u_max = *up++;
    for (i = 1; i < nvoxels; i++, up++)
      if (u_max < (*up)) u_max = *up;
    if (u_max > 32767) {
      LOG_ERROR( "converting unsigned to signed integer");
      sp = (short*)data->data_ptr;
      up = (unsigned short*)data->data_ptr;
      for (i = 0; i < nvoxels; i++) {
        tmp = *up++;
        *sp++ = tmp / 2;
      }
      data->scale_factor *= 2;
    }
  }
  find_data_extrema(data);    /*don't trust in header extrema*/
  return ecat_matrix::ECATX_OK;
}

ecat_matrix::MatrixData*
interfile_read_slice(FILE *fptr, char **ifh, ecat_matrix::MatrixData *volume,
                     int slice, int u_flag)
{
  void * line;
  int i, npixels, file_pos, data_size, nblks, elem_size = 2;
  int  y, data_offset = 0;
  int z_flip = 0, y_flip = 0, x_flip = 0;
  /* unsigned short *up=NULL; */
  /* short *sp=NULL; */
  ecat_matrix::MatrixData *data;

  if (ifh && ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_3] &&
      *ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_3] == 'i') z_flip = 1;
  if (ifh && ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_2] &&
      *ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_2] == 'p') y_flip = 1;
  if (ifh && ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_1] &&
      *ifh[to_underlying(Key::MATRIX_INITIAL_ELEMENT_1] == 'r') x_flip = 1;
  if (ifh && ifh[to_underlying(Key::DATA_OFFSET_IN_BYTES])
    if (sscanf(ifh[to_underlying(Key::DATA_OFFSET_IN_BYTES], "%d", &data_offset) != 1)
      data_offset = 0;

  /* allocate space for ecat_matrix::MatrixData structure and initialize */
  data = (ecat_matrix::MatrixData *) calloc( 1, sizeof(ecat_matrix::MatrixData)) ;
  if (!data) return NULL;
  *data = *volume;
  data->zdim = 1;
  data->shptr = NULL;
  npixels = data->xdim * data->ydim;
  file_pos = data_offset;
  elem_size = _elem_size(data->data_type);
  data_size = data->data_size = npixels * elem_size;
  if (z_flip == 0) file_pos += slice * data_size;
  else file_pos += (volume->zdim - slice - 1) * data_size;
  nblks = (data_size + (ecat_matrix::MatBLKSIZE - 1)) / ecat_matrix::MatBLKSIZE;
  if ((data->data_ptr = malloc(nblks * ecat_matrix::MatBLKSIZE)) == NULL) {
    free_matrix_data(data);
    return NULL;
  }
  fseek(fptr, file_pos, 0); /* jump to location of this slice*/
  if (fread(data->data_ptr, elem_size, npixels, fptr) != npixels) {
    ecat_matrix::matrix_errno = ecat_matrix::MatrixError::READ_ERROR;
    free_matrix_data(data);
    return NULL;
  }

  byte_order(data->data_ptr, elem_size, nblks, ifh[to_underlying(Key::IMAGEDATA_BYTE_ORDER)]);
  if (y_flip)
    flip_y(data->data_ptr, data->data_type, data->xdim, data->ydim);
  if (x_flip) {
    for (y = 0; y < data->ydim; y++) {
      line = data->data_ptr + y * data->xdim * elem_size;
      flip_x(line, data->data_type, data->xdim);
    }
  }
  if (ifh && ifh[to_underlying(Key::NUMBER_FORMAT)] && strstr(ifh[to_underlying(Key::NUMBER_FORMAT)], "unsigned"))
    u_flag = 1;
  if (u_flag && elem_size == 2) {
    short* sp = (short*)data->data_ptr;
    unsigned short* up = (unsigned short*)data->data_ptr;
    for (i = 0; i < npixels; i++) *sp++ = (*up++) / 2;
    data->scale_factor *= 2;
  }
  find_data_extrema(data);   /*don't trust in header extrema*/
  return data;
}

int interfile_write_volume(ecat_matrix::MatrixFile const &mptr, string const image_name, char *header_name, unsigned char  *data_matrix, int size) {
  int count;
  FILE *fp_h, *fp_i;
  char** ifh;
  // InterfileItem* item;
  if ((fp_i = fopen(image_name, W_MODE)) == NULL) 
    return 0;
  count = fwrite(data_matrix, 1, size, fp_i);
  fclose(fp_i);
  if (count != size)
    return 0;
  if ((fp_h = fopen(header_name, W_MODE)) == NULL) 
    return 0;
  ifh = mptr->interfile_header;
  fprintf(fp_h, "!Interfile :=\n");
  fflush(fp_h);

  // for (auto item = used_keys.begin(); item != used_keys.end(); std::advance(item, 1))
  for (auto item : used_keys) 
    if (ifh[to_underlying(Key::item->key] != 0)
      fprintf(fp_h, "%s := %s\n", item->value, ifh[to_underlying(Key::item->key]);
  fflush(fp_h);
}
fclose(fp_h);

return 1;
}

ecat_matrix::MatrixData *interfile_read_scan(ecat_matrix::MatrixFile *mptr, int matnum, int dtype, int segment) {
  int i, nprojs, view, nviews, nvoxels;
  ecat_matrix::MatrixData *data;
  ecat_matrix::MatDir matdir;
  ecat_matrix::Scan3D_subheader *scan3Dsub;
  int group, z_elements;
  int elem_size = 2, view_size, plane_size, nblks;
  unsigned file_pos = 0, z_fill = 0;
  int error_flag = 0;
  void * data_pos = NULL;
  char **ifh;

  /* Scan3D and Atten storage:
    storage_order = 0 : (((projs x z_elements)) x num_angles) x Ringdiffs
    storage_order != 0 : (((projs x num_angles)) x z_elements)) x Ringdiffs
  */

  ecat_matrix::matrix_errno = ecat_matrix::MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();

  ifh = mptr->interfile_header;
  if (matrix_find(mptr, matnum, &matdir) == ecat_matrix::ECATX_ERROR)
    return NULL;
  if ((data = (ecat_matrix::MatrixData *) calloc( 1, sizeof(ecat_matrix::MatrixData))) != NULL) {
    if ( (data->shptr = (void *) calloc(2, ecat_matrix::MatBLKSIZE)) == NULL) {
      free(data);
      return NULL;
    }
  } else return NULL;

  if (interfile_read(mptr, matnum, data, ecat_matrix::MatrixDataType::MAT_SUB_HEADER) != ecat_matrix::ECATX_OK) {
    free_matrix_data(data);
    return NULL;
  }
  if (dtype == ecat_matrix::MatrixDataType::MAT_SUB_HEADER) 
    return data;
  group = abs(segment);
  file_pos = matdir.strtblk * ecat_matrix::MatBLKSIZE;
  scan3Dsub = (ecat_matrix::Scan3D_subheader *)data->shptr;
  z_elements = scan3Dsub->num_z_elements[group];
  if (group > 0) 
    z_elements /= 2;
  data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
  elem_size = _elem_size(data->data_type);
  nprojs = scan3Dsub->num_r_elements;
  nviews = scan3Dsub->num_angles;
  plane_size = nprojs * nviews * elem_size;
  view_size = nprojs * z_elements * elem_size;
  for (i = 0; i < group; i++)
    file_pos += scan3Dsub->num_z_elements[i] * plane_size;
  if (segment < 0) 
    file_pos += z_elements * plane_size;

  /*  read data in the biggest matrix (group 0) */
  z_fill = scan3Dsub->num_z_elements[0] - z_elements;
  nvoxels = nprojs * nviews * scan3Dsub->num_z_elements[0];
  data->data_size = nvoxels * elem_size;
  nblks = (data->data_size + ecat_matrix::MatBLKSIZE - 1) / ecat_matrix::MatBLKSIZE;
  if ((data->data_ptr = (void *)calloc(nblks, ecat_matrix::MatBLKSIZE)) == NULL || fseek(mptr->fptr, file_pos, 0) == -1) {
    free_matrix_data(data);
    return NULL;
  }
  if (scan3Dsub->storage_order == 0) { /* view mode */
    data->z_size = data->pixel_size;
    data->y_size = mptr->mhptr->plane_separation;
    data->ydim = scan3Dsub->num_z_elements[0];
    data->zdim = scan3Dsub->num_angles;
    data_pos = data->data_ptr + (nprojs * z_fill * elem_size) / 2;
    for (view = 0; view < nviews && !error_flag;  view++) {
      if (fread(data_pos, view_size, 1, mptr->fptr) != 1) error_flag++;
      data_pos += view_size + nprojs * z_fill * elem_size;
    }
  } else {
    data->y_size = data->pixel_size;
    data->z_size = mptr->mhptr->plane_separation;
    data->ydim = scan3Dsub->num_angles;
    /* nplanes = z_elements; */
    data->zdim = scan3Dsub->num_z_elements[0];
    data_pos = data->data_ptr + (z_fill * plane_size) / 2;
    if (fread(data_pos, plane_size, z_elements, mptr->fptr) != z_elements)
      error_flag++;
  }
  byte_order(data->data_ptr, elem_size, nblks, ifh[to_underlying(Key::IMAGEDATA_BYTE_ORDER)]);
  find_data_extrema(data);   /*don't trust in header extrema*/
  return data;
}

} // namespace interfile