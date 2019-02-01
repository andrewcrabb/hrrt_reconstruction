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

#include <vector>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#define END_OF_KEYS END_OF_INTERFILE+1
#define MAX_MULTIPLICITY 64
#define LINESIZE 512

// InterfileItem used_keys[] = {
std::vector <InterfileItem> used_keys = {
  {VERSION_OF_KEYS, "version of keys"},
  {IMAGE_MODALITY, "image modality"},
  // Main Header
  {ORIGINAL_INSTITUTION, "original institution"},
  {ORIGINATING_SYSTEM, "originating system"},
  {NAME_OF_DATA_FILE, "name of data file"},
  {DATA_STARTING_BLOCK, "data starting block"},
  {DATA_OFFSET_IN_BYTES, "data offset in bytes"},
  {PATIENT_NAME, "patient name"},
  {PATIENT_ID, "patient id"},
  {PATIENT_DOB, "patient dob"},
  {PATIENT_SEX, "patient sex"},
  {STUDY_ID, "study id"},
  {EXAM_TYPE, "exam type"},
  {DATA_COMPRESSION, "data compression"},
  {DATA_ENCODE, "data encode"},
  {DISPLAY_RANGE, "display range"},
  {IMAGE_EXTREMA, "image extrema"},
  {ATLAS_ORIGIN_1, "atlas origin [1]"},
  {ATLAS_ORIGIN_2, "atlas origin [2]"},
  {ATLAS_ORIGIN_3, "atlas origin [3]"},
  {TYPE_OF_DATA, "type of data"},
  {TOTAL_NUMBER_OF_IMAGES, "total number of images"},
  {STUDY_DATE, "study date"},
  {STUDY_TIME, "study time"},
  {IMAGEDATA_BYTE_ORDER, "imagedata byte order"},
  {NUMBER_OF_WINDOWS, "number of energy windows"},
  // static tomographic images
  {NUMBER_OF_IMAGES, "number of images/energy window"},
  {PROCESS_STATUS, "process status"},
  {NUMBER_OF_DIMENSIONS, "number of dimensions"},
  {MATRIX_SIZE_1, "matrix size [1]"},
  {MATRIX_SIZE_2, "matrix size [2]"},
  {MATRIX_SIZE_3, "matrix size [3]"},
  {NUMBER_FORMAT, "number format"},
  {NUMBER_OF_BYTES_PER_PIXEL, "number of bytes per pixel"},
  {MAXIMUM_PIXEL_COUNT, "maximum pixel count"},
  {MATRIX_INITIAL_ELEMENT_1, "matrix initial element [1]"},
  {MATRIX_INITIAL_ELEMENT_2, "matrix initial element [2]"},
  {MATRIX_INITIAL_ELEMENT_3, "matrix initial element [3]"},
  {SCALE_FACTOR_1, "scaling factor (mm/pixel) [1]"},
  {SCALE_FACTOR_2, "scaling factor (mm/pixel) [2]"},
  {SCALE_FACTOR_3, "scaling factor (mm/pixel) [3]"},
  // ECAT 8 Image Interfile
  {SCALE_FACTOR_1, "scale factor (mm/pixel) [1]"},
  {SCALE_FACTOR_2, "scale factor (mm/pixel) [2]"},
  {SCALE_FACTOR_3, "scale factor (mm/pixel) [3]"},
  // ECAT 8 Sinogram Interfile
  {SCALE_FACTOR_2, "scale factor [2]"},

  {IMAGE_DURATION, "image duration"},
  {IMAGE_START_TIME, "image start time"},
  {IMAGE_NUMBER, "image number"},
  {LABEL, "label"},
  // not standard keys added by Sibomana@topo.ucl.ac.be : it is expressed as scale units; e.g 10e-6 counts/second

  {QUANTIFICATION_UNITS, "quantification units"},
  // scale_factor and units label
  {REAL_EXTREMA, "real extrema"},
  {INTERPOLABILITY, "interpolability"},
  {TRANSFORMER, "transformer"},
  {COLORTAB, "colortab"},
  // Sinograms header support
  {NUM_Z_ELEMENTS, "number of z elements"},
  {STORAGE_ORDER, "storage order"},
  {IMAGE_RELATIVE_START_TIME, "image relative start time"},
  {TOTAL_PROMPTS, "total Prompts"},
  {TOTAL_RANDOMS, "total Randoms"},
  {END_OF_INTERFILE, "end of interfile"},
  {END_OF_KEYS, 0}
};

// static char const *magicNumber = "interfile";
const std::string magicNumber = "!INTERFILE";
static char  line[LINESIZE];

static void byte_order(void * data_ptr, int elem_size, int nblks,
                       char *in_order)
{
  int swap_order = 0;
  char *dptr = NULL, *tmp = NULL;
  int i = 0, j = 0;
  if ( in_order != NULL && (*in_order == 'b' || *in_order == 'B')) {
    if (ntohs(1) != 1) swap_order = 1;  /* big to little endian */
  } else {
    if (ntohs(1) == 1) swap_order = 1;  /* little to big endian */
  }
  if (swap_order) {
    tmp = static_cast<char *>(malloc(MatBLKSIZE));
    dptr = static_cast<char *>(data_ptr);
    if (elem_size == 2) {
      for (i = 0, j = 0; i < nblks; i++, j += MatBLKSIZE) {
        swab(dptr + j, tmp, MatBLKSIZE);
        memcpy(dptr + j, tmp, MatBLKSIZE);
      }
    }
    if (elem_size == 4) {
      for (i = 0, j = 0; i < nblks; i++, j += MatBLKSIZE) {
        swab(dptr + j, tmp, MatBLKSIZE);
        swaw((short*)tmp, (short*)(dptr + j), MatBLKSIZE / 2);
      }
    }
    free(tmp);
  }
}

static int if_multiplicity(char **ifh, int **blk_offsets)
{
  int num_offset = 0;
  char *word;
  static int _offsets[MAX_MULTIPLICITY];
  _offsets[0] = 0;
  *blk_offsets = _offsets;
  if (ifh[DATA_STARTING_BLOCK] == NULL) return 1;
  strcpy(line, ifh[DATA_STARTING_BLOCK]);
  num_offset = 0;
  word = strtok(line, " \t");
  while (word != NULL && num_offset < MAX_MULTIPLICITY &&
         sscanf(word, "%d", &_offsets[num_offset]) == 1)
  {
    num_offset++;
    word = strtok(0, " \t");
  }
  if (num_offset == 0) return 1;
  return num_offset;
}

static void clean_eol(char *line)
{
  int len = strlen(line);
  if (len > LINESIZE - 1) {
    fprintf(stderr, "line too long :\n %s", line);
    exit(1);
  }
  line[len - 1] = ' ';
}

static int _elem_size(int data_type)
{
  switch (data_type) {
  case ByteData :
  case Color_8 :
    return 1;
  case Color_24 :
    return 3;
  case SunShort:
  case VAX_Ix2:
    return 2;
  case SunLong:
    return 4;
  case IeeeFloat:
    return 4;
  default:
    fprintf(stderr, "unkown data type, assume short int\n");
    return 2;
  }
}

static void find_data_extrema(MatrixData *data)
{
  int npixels = data->xdim * data->ydim * data->zdim;
  switch (data->data_type) {
  case ByteData :
  case Color_8 :
    data->data_max = find_bmax((unsigned char *)data->data_ptr, npixels);
    data->data_min = find_bmin((unsigned char *)data->data_ptr, npixels);
    break;
  default :
  case SunShort:
  case VAX_Ix2:
    data->data_max = find_smax((short*)data->data_ptr, npixels);
    data->data_min = find_smin((short*)data->data_ptr, npixels);
    break;
  case SunLong:
    data->data_max = (float)find_imax((int*)data->data_ptr, npixels);
    data->data_min = (float)find_imin((int*)data->data_ptr, npixels);
    break;
  case IeeeFloat:
    data->data_max = find_fmax((float*)data->data_ptr, npixels);
    data->data_min = find_fmin((float*)data->data_ptr, npixels);
    break;
  case Color_24 :  /* get min and max brightness */
    data->data_max = find_bmax((unsigned char *)data->data_ptr, 3 * npixels);
    data->data_min = find_bmin((unsigned char *)data->data_ptr, 3 * npixels);
  }
  data->data_max *=  data->scale_factor;
  data->data_min *=  data->scale_factor;
}

void flip_x(void * line, int data_type, int xdim)
{
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
  case Color_8 :
  case ByteData :
  {
    unsigned char  *b_p0, *b_p1;
    b_p0 = (unsigned char *)line;
    b_p1 = (unsigned char *)_line + xdim - 1;
    for (x = 0; x < xdim; x++) *b_p1-- = *b_p0++;
    memcpy(line, _line, xdim);
    break;
  }
  default :
  case SunShort:
  case VAX_Ix2:
  {
    short *s_p0, *s_p1;
    s_p0 = (short*)line;
    // s_p1 = (short*)(_line + (xdim - 1) * elem_size);
    s_p1 = static_cast<short *>(_line) + (xdim - 1) * elem_size;
    for (x = 0; x < xdim; x++) *s_p1-- = *s_p0++;
    memcpy(line, _line, xdim * elem_size);
    break;
  }
  case SunLong:
  {
    int *i_p0, *i_p1;
    i_p0 = (int*)line;
    i_p1 = static_cast<int *>(_line) + (xdim - 1) * elem_size;
    for (x = 0; x < xdim; x++) *i_p1-- = *i_p0++;
    memcpy(line, _line, xdim * elem_size);
    break;
  }
  case IeeeFloat:
  {
    float *f_p0, *f_p1;
    f_p0 = (float*)line;
    f_p1 = static_cast<float*>(_line) + (xdim - 1) * elem_size;
    for (x = 0; x < xdim; x++) *f_p1-- = *f_p0++;
    memcpy(line, _line, xdim * elem_size);
    break;
  }
  case Color_24:
  {
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

void flip_y(void * plane, int data_type, int xdim, int ydim)
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
  static InterfileItem ret;

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
    for (auto item = used_keys.begin(); item != used_keys.end(); std::advance(item, 1))
      if (item->value.compare(key_str) == 0) {
        ret.key = item->key;
        if (ret.key == TRANSFORMER || ret.key == COLORTAB ) {
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
//       if (*p == '\0') {       /* ECATX_OK */
//         fclose(fp);
//         return strdup(fname);
//       }
//     }
//   }
//   fclose(fp);
//   return 0;
// }

int _is_interfile(const std::string &fname) {
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

// What the flying f does this thing do.  It returns a char *.

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
      if (item->key == NAME_OF_DATA_FILE) {
        /* check short and full name */
        if (strcmp(item->value, img_fname) == 0 || strcmp(item->value, fname) == 0) {
          fclose(fp);
          fprintf(stderr, "using %s header for %s data file\n", hdr_fname, fname);
          free(item->value);
          return hdr_fname;
        }
      }
      free(item->value);
    }
    fclose(fp);
    fprintf(stderr, "using %s header for %s data file\n", hdr_fname, fname);
    fprintf(stderr, "Warning: Adding Missing keyword 'name of data file'\n");
    if ((fp = fopen(hdr_fname, "at")) != NULL) {
      fprintf(fp, "\nname of data file := %s\n", fname);
      fclose(fp);
    } else {
      fprintf(stderr, "Can't open %s header for appending data file\n", hdr_fname);
      free(hdr_fname);
      return NULL;
    }
    return hdr_fname;
  }
  return NULL;
}

int
unmap_interfile_header(char **ifh, MatrixData *mdata)
{
  int  sx, sy, sz = 1, dim = 2, elem_size = 2;
  int big_endian = 0;
  float f;
  char *p;

  if (ifh[NUMBER_OF_DIMENSIONS] != NULL) {
    sscanf(ifh[NUMBER_OF_DIMENSIONS], "%d", &dim);
    if (dim != 2 && dim != 3) {
      matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
      return ECATX_ERROR;
    }
  }
  if ( (p = ifh[IMAGEDATA_BYTE_ORDER]) != NULL && (*p == 'b' || *p == 'B'))
    big_endian = 1;
  sscanf(ifh[NUMBER_OF_BYTES_PER_PIXEL], "%d", &elem_size); /* tested in interfile_open */
  if (ifh[NUMBER_FORMAT] && strstr(ifh[NUMBER_FORMAT], "float")) {
    if (elem_size != 4) {
      matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
      return ECATX_ERROR;
    }
    mdata->data_type = IeeeFloat;
  } else if (ifh[NUMBER_FORMAT] && strstr(ifh[NUMBER_FORMAT], "rgb")) {
    if (elem_size != 3) {
      matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
      return ECATX_ERROR;
    }
    mdata->data_type = Color_24;
  }
  else {  /* integer data type */
    if (elem_size != 1 && elem_size != 2 && elem_size != 4) {
      matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
      return ECATX_ERROR;
    }
    if (elem_size == 1) mdata->data_type = ByteData;
    if (big_endian) {
      if (elem_size == 2) mdata->data_type = SunShort;
      if (elem_size == 4) mdata->data_type = SunLong;
    } else {
      if (elem_size == 2) mdata->data_type = VAX_Ix2;
      if (elem_size == 4) mdata->data_type = VAX_Ix4;
    }
  }
  if (ifh[MATRIX_SIZE_1] == NULL ||
      sscanf(ifh[MATRIX_SIZE_1], "%d", &sx) != 1)
    matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  else if (ifh[MATRIX_SIZE_2] == NULL ||
           sscanf(ifh[MATRIX_SIZE_2], "%d", &sy) != 1)
    matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  else  if (dim == 3)  {
    if (ifh[MATRIX_SIZE_3] == NULL ||
        sscanf(ifh[MATRIX_SIZE_3], "%d", &sz) != 1)
      matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  }
  if (ifh[NUMBER_OF_IMAGES] != NULL) {
    if (sscanf(ifh[NUMBER_OF_IMAGES], "%d", &sz) != 1)
      matrix_errno = ecat_matrix::MatrixError::INVALID_DIMENSION;
  }
  if (matrix_errno) return ECATX_ERROR;
  mdata->xdim = sx;
  mdata->ydim = sy;
  mdata->zdim = sz;
  if (ifh[QUANTIFICATION_UNITS] != NULL)
    sscanf(ifh[QUANTIFICATION_UNITS], "%g", &mdata->scale_factor);
  mdata->data_min = mdata->data_max = 0;
  if (ifh[SCALE_FACTOR_1] && sscanf(ifh[SCALE_FACTOR_1], "%g", &f) == 1)
    mdata->pixel_size = f / 10.0f;  /* mm to cm */
  if (ifh[SCALE_FACTOR_2] && sscanf(ifh[SCALE_FACTOR_2], "%g", &f) == 1)
    mdata->y_size = f / 10.0f;  /* mm to cm */
  if (ifh[SCALE_FACTOR_3] && sscanf(ifh[SCALE_FACTOR_3], "%g", &f) == 1)
    mdata->z_size = f / 10.0f;  /* mm to cm */
  return 1;
}

int free_interfile_header(char **ifh) {
  int i = 0;
  for (i = 0; i < END_OF_KEYS; i++)
    if (ifh[i] != NULL)
      free(ifh[i]);
  free(ifh);
  return 1;
}

int get_block_singles(MatrixFile *mptr, float **pextended_uncor_singles,
                      int *pnum_extended_uncor_singles) {
  int tmp_size = 128, max_size = 1024;
  FILE *fp;
  float *tmp = NULL;
  int block_num, count = 0;
  long block_singles;
  char *key_str, *val_str;
  if ((fp = fopen(mptr->fname, R_MODE)) == NULL) return ECATX_ERROR;
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
      tmp = realloc(tmp, tmp_size * sizeof(float));
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
  return count > 0 ? ECATX_OK : ECATX_ERROR;
}

int interfile_open(MatrixFile *mptr) {
  InterfileItem* item;
  FILE *fp;
  Main_header *mh;
  MatrixData mdata;
  struct MatDir matdir;
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

  memset(&mdata, 0, sizeof(MatrixData));
  now = time(0);
  this_year = 1900 + (localtime(&now))->tm_year;
  if ((fp = fopen(mptr->fname, R_MODE)) == NULL) return ECATX_ERROR;
  mh = mptr->mhptr;
  strcpy(mh->data_units, "none");
  mh->calibration_units = 2; /* Processed */
  mh->calibration_factor = 1.0f;
  mh->calibration_units_label = 0;
  strcpy(mh->magic_number, magicNumber);
  mh->sw_version = 70;
  mptr->interfile_header = (char**)calloc(END_OF_KEYS, sizeof(char*));
  if (mptr->interfile_header == NULL) return ECATX_ERROR;
  mh->num_frames = mh->num_gates = mh->num_bed_pos = 1;
  mh->plane_separation = 1;
  while (!end_of_interfile && (item = get_next_item(fp)) != NULL) {
    if (item->value == NULL) continue;
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
    case END_OF_INTERFILE:
      end_of_interfile = 1;
    }
  }
  fclose(fp);

  if (elem_size != 1 && elem_size != 2 && elem_size != 3 && elem_size != 4) {
    matrix_errno = ecat_matrix::MatrixError::INVALID_DATA_TYPE;
    free_interfile_header(mptr->interfile_header);
    return ECATX_ERROR;
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

  if (mptr->interfile_header[NAME_OF_DATA_FILE] != NULL) {
    strcpy(data_file, mptr->interfile_header[NAME_OF_DATA_FILE]);
    strcpy(data_dir, mptr->fname);
    if ( (p = strrchr(data_dir, DIR_SEPARATOR)) != NULL) * p = '\0';
    else strcpy(data_dir, ".");
    if ( (p = strrchr(data_file, DIR_SEPARATOR)) == NULL) {
      /* build fullpath filename */
      sprintf(data_file, "%s%c%s", data_dir, DIR_SEPARATOR,
              mptr->interfile_header[NAME_OF_DATA_FILE]);
      make_path = 1;
    }
    if ( (mptr->fptr = fopen(data_file, R_MODE)) != NULL) {
      if (make_path)
        mptr->interfile_header[NAME_OF_DATA_FILE] = strdup(data_file);
    } else {
      perror(data_file);
      free_interfile_header(mptr->interfile_header);
      return ECATX_ERROR;
    }
  } else return ECATX_ERROR;

  if (!unmap_interfile_header(mptr->interfile_header, &mdata)) {
    free_interfile_header(mptr->interfile_header);
    return ECATX_ERROR;
  }
  nmats = if_multiplicity(mptr->interfile_header, &blk_offsets);
  data_size = mdata.xdim * mdata.ydim * mdata.zdim * elem_size;
  nblks = (data_size + MatBLKSIZE - 1) / MatBLKSIZE;
  mptr->dirlist = (MatDirList *) calloc(1, sizeof(MatDirList)) ;
  for (i = 0; i < nmats; i++) {
    matdir.matnum = mat_numcod(i + 1, 1, 1, 0, 0);
    matdir.strtblk = blk_offsets[i];
    matdir.endblk = matdir.strtblk + nblks;
    insert_mdir(&matdir, mptr->dirlist) ;
  }
  return ECATX_OK;
}

int
interfile_read(MatrixFile *mptr, int matnum, MatrixData *data, int dtype)
{
  Image_subheader *imagesub = NULL;
  Scan3D_subheader *scan3Dsub = NULL;
  int y, z, image_min = 0, image_max = 0;
  size_t i, npixels, nvoxels;
  int tmp, nblks, elem_size = 2, data_offset = 0;
  void *plane, *line;
  unsigned short u_max, *up = NULL;
  short *sp = NULL;
  int z_flip = 0, y_flip = 0, x_flip = 0;
  float f;
  unsigned time_msec = 0;
  char **ifh, *p, *dup;
  int storage_order = 1;    /* default is sino mode */
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
  if (ifh[MAXIMUM_PIXEL_COUNT])
    sscanf(ifh[MAXIMUM_PIXEL_COUNT], "%d", &image_max);
  if (ifh[IMAGE_EXTREMA])
    sscanf(ifh[IMAGE_EXTREMA], "%d %d", &image_min, &image_max);
  data->data_min = data->scale_factor * image_min;
  data->data_max = data->scale_factor * image_max;
  if (ifh[REAL_EXTREMA])
    sscanf(ifh[REAL_EXTREMA], "%g %g", &data->data_min, &data->data_max);
  sscanf(ifh[NUMBER_OF_BYTES_PER_PIXEL], "%d", &elem_size); /* tested in interfile_open */

  switch (mptr->mhptr->file_type) {
  case ecat_matrix::DataSetType::Byte3dSinogram:
  case ecat_matrix::DataSetType::Short3dSinogram:
  case ecat_matrix::DataSetType::Float3dSinogram:
    scan3Dsub = (Scan3D_subheader*)data->shptr;
    memset(scan3Dsub, 0, sizeof(Scan3D_subheader));
    scan3Dsub->num_dimensions = 3;
    scan3Dsub->num_r_elements = data->xdim;
    scan3Dsub->num_angles = data->ydim;
    if (ifh[STORAGE_ORDER])
      sscanf(ifh[STORAGE_ORDER], "%d", &storage_order);
    scan3Dsub->storage_order = storage_order;
    dup = strdup(mptr->interfile_header[NUM_Z_ELEMENTS]);
    p = strtok(dup, " ");
    while (p) {
      sscanf(p, "%d", &z_elements);
      scan3Dsub->num_z_elements[group++] = z_elements;
      p = strtok(NULL, " ");
    }
    free(dup);
    if (get_block_singles(mptr, &scan3Dsub->extended_uncor_singles, &scan3Dsub->num_extended_uncor_singles) == ECATX_OK) {
      int max_count = scan3Dsub->num_extended_uncor_singles < 128 ? scan3Dsub->num_extended_uncor_singles : 128;
      memcpy(scan3Dsub->uncor_singles, scan3Dsub->extended_uncor_singles, max_count * sizeof(float));
    }
    if (ifh[TOTAL_PROMPTS])
      sscanf(ifh[TOTAL_PROMPTS], "%d", &scan3Dsub->prompts);
    if (ifh[TOTAL_RANDOMS])
      sscanf(ifh[TOTAL_RANDOMS], "%d", &scan3Dsub->delayed);
    if (ifh[IMAGE_DURATION] && sscanf(ifh[IMAGE_DURATION], "%d", (int *)&time_msec) == 1)
      scan3Dsub->frame_duration = time_msec;
    if (ifh[IMAGE_START_TIME] && sscanf(ifh[IMAGE_START_TIME], "%d:%d:%d", &hour, &min, &sec) == 3) {
      scan3Dsub->frame_start_time = sec + 60 * (min + 60 * hour);
    } else if (ifh[IMAGE_RELATIVE_START_TIME] && sscanf(ifh[IMAGE_RELATIVE_START_TIME], "%d", &sec) == 1) {
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
    imagesub = (Image_subheader*)data->shptr;
    memset(imagesub, 0, sizeof(Image_subheader));
    imagesub->num_dimensions = 3;
    imagesub->x_pixel_size = data->pixel_size;
    imagesub->y_pixel_size = data->y_size;
    imagesub->z_pixel_size = data->z_size;
    imagesub->scale_factor = data->scale_factor;
    imagesub->x_dimension = data->xdim;
    imagesub->y_dimension = data->ydim;
    imagesub->z_dimension = data->zdim;
    imagesub->data_type = data->data_type;
    if (ifh[IMAGE_DURATION] && sscanf(ifh[IMAGE_DURATION], "%d", (int *)&time_msec) == 1)
      imagesub->frame_duration = time_msec;
    if (ifh[IMAGE_START_TIME] &&
        sscanf(ifh[IMAGE_START_TIME], "%d:%d:%d", &hour, &min, &sec) == 3)
      imagesub->frame_start_time = sec + 60 * (min + 60 * hour);
  }  // giant switch statement.

  if (ifh[ATLAS_ORIGIN_1] &&
      sscanf(ifh[ATLAS_ORIGIN_1], "%g", &f) == 1) {
    data->x_origin = f * data->pixel_size;
  }
  if (ifh[ATLAS_ORIGIN_2] &&
      sscanf(ifh[ATLAS_ORIGIN_2], "%g", &f) == 1) {
    data->y_origin = f * data->y_size;
  }
  if (ifh[ATLAS_ORIGIN_3] &&
      sscanf(ifh[ATLAS_ORIGIN_3], "%g", &f) == 1) {
    data->z_origin = f  * data->z_size;
  }

  if (dtype == MAT_SUB_HEADER) return ECATX_OK;

  /* compute extrema */
  if (ifh[MATRIX_INITIAL_ELEMENT_3] &&
      *ifh[MATRIX_INITIAL_ELEMENT_3] == 'i') {
    z_flip = 1;
    if (data->z_origin > 0)
      data->z_origin = (data->zdim - 1) * data->z_size - data->z_origin;
    fprintf(stderr,
            "volume z direction is changed to superior->inferior\n");
  }
  if (ifh[MATRIX_INITIAL_ELEMENT_2] &&
      *ifh[MATRIX_INITIAL_ELEMENT_2] == 'p') {
    y_flip = 1;
    if (data->y_origin > 0)
      data->y_origin = (data->ydim - 1) * data->y_size - data->y_origin;
    fprintf(stderr,
            "volume y direction is changed to anterior->posterior\n");
  }
  if (ifh[MATRIX_INITIAL_ELEMENT_1] &&
      *ifh[MATRIX_INITIAL_ELEMENT_1] == 'r') {
    x_flip = 1;
    if (data->x_origin > 0)
      data->x_origin = (data->xdim - 1) * data->pixel_size - data->x_origin;
    fprintf(stderr,
            "volume x direction is changed to left->right\n");
  }
  npixels = data->xdim * data->ydim;
  nvoxels = npixels * data->zdim;
  data->data_size = nvoxels * elem_size;
  nblks = (data->data_size + MatBLKSIZE - 1) / MatBLKSIZE;
  data->data_ptr = (void *) calloc(nblks, MatBLKSIZE);
  if (ifh[DATA_STARTING_BLOCK] &&
      sscanf(ifh[DATA_STARTING_BLOCK], "%d", &data_offset) ) {
    if (data_offset < 0) data_offset = 0;
    else data_offset *= MatBLKSIZE;
  }
  if (data_offset == 0 && ifh[DATA_OFFSET_IN_BYTES] &&
      sscanf(ifh[DATA_OFFSET_IN_BYTES], "%d", &data_offset) ) {
    if (data_offset < 0) data_offset = 0;
  }
  if (data_offset > 0) {
    if (fseek(mptr->fptr, data_offset, SEEK_SET) != 0)
      crash("Error skeeping to offset %d\n", data_offset);
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
      fprintf(stderr, "interfile_read ERROR reading z %3d of %3d, got %5d of %5d bytes in fread, total bytes read = %8d\n", z, data->zdim, num_read, req_size, bytesread);
      free(data->data_ptr);
      data->data_ptr = NULL;
      matrix_errno = ecat_matrix::MatrixError::READ_ERROR;
      return ECATX_ERROR;
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

  byte_order(data->data_ptr, elem_size, nblks, ifh[IMAGEDATA_BYTE_ORDER]);

  if (elem_size == 2 &&
      ifh[NUMBER_FORMAT] && strstr(ifh[NUMBER_FORMAT], "unsigned") ) {
    up = (unsigned short*)data->data_ptr;
    u_max = *up++;
    for (i = 1; i < nvoxels; i++, up++)
      if (u_max < (*up)) u_max = *up;
    if (u_max > 32767) {
      fprintf(stderr, "converting unsigned to signed integer\n");
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
  return ECATX_OK;
}

MatrixData*
interfile_read_slice(FILE *fptr, char **ifh, MatrixData *volume,
                     int slice, int u_flag)
{
  void * line;
  int i, npixels, file_pos, data_size, nblks, elem_size = 2;
  int  y, data_offset = 0;
  int z_flip = 0, y_flip = 0, x_flip = 0;
  /* unsigned short *up=NULL; */
  /* short *sp=NULL; */
  MatrixData *data;

  if (ifh && ifh[MATRIX_INITIAL_ELEMENT_3] &&
      *ifh[MATRIX_INITIAL_ELEMENT_3] == 'i') z_flip = 1;
  if (ifh && ifh[MATRIX_INITIAL_ELEMENT_2] &&
      *ifh[MATRIX_INITIAL_ELEMENT_2] == 'p') y_flip = 1;
  if (ifh && ifh[MATRIX_INITIAL_ELEMENT_1] &&
      *ifh[MATRIX_INITIAL_ELEMENT_1] == 'r') x_flip = 1;
  if (ifh && ifh[DATA_OFFSET_IN_BYTES])
    if (sscanf(ifh[DATA_OFFSET_IN_BYTES], "%d", &data_offset) != 1)
      data_offset = 0;

  /* allocate space for MatrixData structure and initialize */
  data = (MatrixData *) calloc( 1, sizeof(MatrixData)) ;
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
  nblks = (data_size + (MatBLKSIZE - 1)) / MatBLKSIZE;
  if ((data->data_ptr = malloc(nblks * MatBLKSIZE)) == NULL) {
    free_matrix_data(data);
    return NULL;
  }
  fseek(fptr, file_pos, 0); /* jump to location of this slice*/
  if (fread(data->data_ptr, elem_size, npixels, fptr) != npixels) {
    matrix_errno = ecat_matrix::MatrixError::READ_ERROR;
    free_matrix_data(data);
    return NULL;
  }

  byte_order(data->data_ptr, elem_size, nblks, ifh[IMAGEDATA_BYTE_ORDER]);
  if (y_flip)
    flip_y(data->data_ptr, data->data_type, data->xdim, data->ydim);
  if (x_flip) {
    for (y = 0; y < data->ydim; y++) {
      line = data->data_ptr + y * data->xdim * elem_size;
      flip_x(line, data->data_type, data->xdim);
    }
  }
  if (ifh && ifh[NUMBER_FORMAT] && strstr(ifh[NUMBER_FORMAT], "unsigned"))
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

int interfile_write_volume(MatrixFile *mptr, char *image_name, char *header_name,
                           unsigned char  *data_matrix, int size) {
  int count;
  FILE *fp_h, *fp_i;
  char** ifh;
  // InterfileItem* item;
  if ((fp_i = fopen(image_name, W_MODE)) == NULL) return 0;
  count = fwrite(data_matrix, 1, size, fp_i);
  fclose(fp_i);
  if (count != size) {
    return 0;
  }
  if ((fp_h = fopen(header_name, W_MODE)) == NULL) return 0;
  ifh = mptr->interfile_header;
  fprintf(fp_h, "!Interfile :=\n");
  fflush(fp_h);
  // for (item=used_keys; item->value!=NULL; item++){
  for (auto item = used_keys.begin(); item != used_keys.end(); std::advance(item, 1))
    if (ifh[item->key] != 0)
      fprintf(fp_h, "%s := %s\n", item->value, ifh[item->key]);
  fflush(fp_h);
}
fclose(fp_h);

return 1;
}

MatrixData*
interfile_read_scan(MatrixFile *mptr, int matnum, int dtype, int segment)
{
  int i, nprojs, view, nviews, nvoxels;
  MatrixData *data;
  struct MatDir matdir;
  Scan3D_subheader *scan3Dsub;
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

  matrix_errno = ecat_matrix::MatrixError::OK;
  // matrix_errtxt[0] = '\0';
  matrix_errtxt.clear();

  ifh = mptr->interfile_header;
  if (matrix_find(mptr, matnum, &matdir) == ECATX_ERROR)
    return NULL;
  if ((data = (MatrixData *) calloc( 1, sizeof(MatrixData))) != NULL) {
    if ( (data->shptr = (void *) calloc(2, MatBLKSIZE)) == NULL) {
      free(data);
      return NULL;
    }
  } else return NULL;

  if (interfile_read(mptr, matnum, data, MAT_SUB_HEADER) != ECATX_OK) {
    free_matrix_data(data);
    return NULL;
  }
  if (dtype == MAT_SUB_HEADER) return data;
  group = abs(segment);
  file_pos = matdir.strtblk * MatBLKSIZE;
  scan3Dsub = (Scan3D_subheader *)data->shptr;
  z_elements = scan3Dsub->num_z_elements[group];
  if (group > 0) z_elements /= 2;
  data->data_max = scan3Dsub->scan_max * scan3Dsub->scale_factor;
  elem_size = _elem_size(data->data_type);
  nprojs = scan3Dsub->num_r_elements;
  nviews = scan3Dsub->num_angles;
  plane_size = nprojs * nviews * elem_size;
  view_size = nprojs * z_elements * elem_size;
  for (i = 0; i < group; i++)
    file_pos += scan3Dsub->num_z_elements[i] * plane_size;
  if (segment < 0) file_pos += z_elements * plane_size;

  /*  read data in the biggest matrix (group 0) */
  z_fill = scan3Dsub->num_z_elements[0] - z_elements;
  nvoxels = nprojs * nviews * scan3Dsub->num_z_elements[0];
  data->data_size = nvoxels * elem_size;
  nblks = (data->data_size + MatBLKSIZE - 1) / MatBLKSIZE;
  if ((data->data_ptr = (void *)calloc(nblks, MatBLKSIZE)) == NULL ||
      fseek(mptr->fptr, file_pos, 0) == -1) {
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
  byte_order(data->data_ptr, elem_size, nblks, ifh[IMAGEDATA_BYTE_ORDER]);
  find_data_extrema(data);   /*don't trust in header extrema*/
  return data;
}
