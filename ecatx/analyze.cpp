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
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>

#include "analyze.hpp"
#include "interfile.hpp"
#include "machine_indep.hpp"
#include "my_spdlog.hpp"
#include "hrrt_util.hpp"

// ahc
#include <unistd.h>

namespace bf = boost::filesystem;

// #define END_OF_KEYS static_cast<int>(interfile::Key::END_OF_INTERFILE) + 1

static struct dsr g_header;
int analyze_orientation = 0; /* default is spm neurologist convention */

static int analyze_read_hdr(const char *fname) {
  FILE *fp;
  char tmp[80];

  if ((fp = fopen(fname, "rb")) == NULL)
    return 0;
  // if (fread(&g_header,sizeof(struct dsr),1,fp) < 0) return 0;
  if (fread(&g_header, sizeof(struct dsr), 1, fp) < sizeof(struct dsr))
    return 0;
  fclose(fp);
  if (g_header.hk.sizeof_hdr == sizeof(struct dsr)) {
    g_header.hk.sizeof_hdr = g_header.hk.sizeof_hdr;
    g_header.hk.extents = g_header.hk.extents;
    swab((char*)g_header.dime.dim, tmp, 4 * sizeof(short));
    memcpy(g_header.dime.dim, tmp, 4 * sizeof(short));
    g_header.dime.datatype = g_header.dime.datatype;
    g_header.dime.bitpix = g_header.dime.bitpix;
    swab((char*)g_header.dime.pixdim, tmp, 4 * sizeof(float));
    hrrt_util::swaw((short*)tmp, (short*)g_header.dime.pixdim, 4 * sizeof(float) / 2);
    swab((char*)&g_header.dime.funused1, tmp, sizeof(float));
    hrrt_util::swaw((short*)tmp, (short*)(&g_header.dime.funused1), sizeof(float) / 2);
    g_header.dime.glmax = g_header.dime.glmax;
    g_header.dime.glmin = g_header.dime.glmin;
  }
  return 1;
}

// static int _is_analyze(const char* fname) {
//   FILE *fp;
//   int sizeof_hdr = 0;
//   int magic_number = sizeof(struct dsr);

//   if ( (fp = fopen(fname, "rb")) == NULL)
//     return 0;
//   fread(&sizeof_hdr, sizeof(int), 1, fp);
//   fclose(fp);
//   if (sizeof_hdr == magic_number || sizeof_hdr == magic_number )
//     return 1;
//   return 0;
// }

// Return 1 if extension "hdr" and first 4 bytes store 0x15c = 348

int is_analyze_hdr(boost::filesystem::path const &t_hdr_file) {
  int is_hdr = (t_hdr_file.extension().string().compare(".hdr")) ? 0 : 1;
  if (is_hdr) {
    std::ifstream instream(t_hdr_file.string(), std::ios::in | std::ios::binary);
    if (instream.good()) {
      int i;
      instream.read(reinterpret_cast<char *>(&i), sizeof(int));
      is_hdr *= (i == sizeof(struct dsr)) ? 1 : 0;
    }
  }
  return is_hdr;
}

// Return 1 if extension "img" and size is equal to that calculated from applicable header.

int is_analyze_img(boost::filesystem::path const &t_img_file) {
  int is_img = (t_hdr_file.extension().string().compare(".img")) ? 0 : 1;
  boost::filesystem::path infile_hdr = t_img_file;
  infile_hdr.replace_extension("hdr");

}

// Given a .img or .hdr file, return 1 if img/hdr pair are both Analyze, else 0.

int is_analyze(boost::filesystem::path const &infile) {
  boost::filesystem::path infile_hdr = infile;
  boost::filesystem::path infile_img = infile;
  infile_hdr = infile_hdr.replace_extension("hdr");
  infile_img = infile_img.replace_extension("img");

  int is_analyze = is_analyze_hdr(infile_hdr) * is_analyze_img(infile_img);
  return is_analyze;
}

char* is_analyze(const char* fname) {
  char *p, *hdr_fname = NULL, *img_fname = NULL;
  int elem_size = 0, nvoxels;

  if (_is_analyze(fname)) { /* access by .g_header */
    hdr_fname = strdup(fname);
    img_fname = static_cast<char *>(malloc(strlen(fname) + 4));
    strcpy(img_fname, fname);
    if ( (p = strrchr(img_fname, '.')) != NULL)
      * p++ = '\0';
    strcat(img_fname, ".img");
  } else {          /* access by .img */
    // if ( (p=strstr(fname,".img")) == NULL)
    if ( strstr(fname, ".img") == NULL)
      return NULL;      /* not a .img file */
    img_fname = strdup(fname);
    hdr_fname = static_cast<char *>(malloc(strlen(fname) + 4));
    strcpy(hdr_fname, fname);
    if ( (p = strrchr(hdr_fname, '.')) != NULL)
      * p++ = '\0';
    strcat(hdr_fname, ".hdr");
  }

  struct stat st;
  if (stat(img_fname, &st) >= 0 && _is_analyze(hdr_fname) && analyze_read_hdr(hdr_fname)) {
    short *dim = g_header.dime.dim;
    nvoxels = dim[1] * dim[2] * dim[3] * dim[4];
    elem_size = analyze::data_types_.at(g_header.dime.datatype).bytes_per_pixel;
    if (elem_size == 0) {
      LOG_ERROR( "unkown ANALYZE data type : {}", g_header.dime.datatype);
    } else {
      if (nvoxels * elem_size == st.st_size) {
        if (strcmp(hdr_fname, fname) != 0)
          LOG_INFO( "using {} header for {} data file", hdr_fname, img_fname);
        free(img_fname);
        return hdr_fname;
      } else {
        LOG_ERROR("{} size does not match {} info", img_fname, hdr_fname);
      }
    }
  }
  free(img_fname);
  free(hdr_fname);
  return NULL;
}

int analyze_open(ecat_matrix::MatrixFile *t_matrix_file) {
  ecat_matrix::matrix_errno = static_cast<ecat_matrix::MatrixError>(0);
  if (!analyze_read_hdr(t_matrix_file->fname)) 
    return 0;
  ecat_matrix::Main_header *main_header = t_matrix_file->mhptr;
  sprintf(main_header->magic_number, "%d", (int)(sizeof(struct dsr)));
  main_header->sw_version = 70;
  main_header->file_type = MatrixData::DataSetType::InterfileImage;
  t_matrix_file->analyze_hdr = (char*)calloc(1, sizeof(struct dsr));  // ahc presence of this also indicates analyze file read.  Why not use file_format?
  memcpy(t_matrix_file->analyze_hdr, &g_header, sizeof(g_header));

  bf::path data_file = t_matrix_file->fname;
  data_file.replace_extension("img");
  t_matrix_file->set_header(interfile::Key::NAME_OF_DATA_FILE, data_file.string());
  hrrt_util::open_istream(t_matrix_file->fptr, data_file);
  if (t_matrix_file->fptr.fail())
    return 0;

  short elem_size;
  switch (g_header.dime.datatype) {
  case analyze::DataKey::UNSIGNED_CHAR :
  case analyze::DataKey::SIGNED_INT :
  case analyze::DataKey::FLOAT :
    process_datatype(g_header.dime.datatype, t_matrix_file, elem_size);
    break;
  case analyze::DataKey::SIGNED_SHORT :
    process_datatype(g_header.dime.datatype, t_matrix_file, elem_size);
    t_matrix_file->set_header(interfile::Key::IMAGE_EXTREMA, fmt::format("{:d} {:d}", g_header.dime.glmin, g_header.dime.glmax));
    break;
  default :
    t_matrix_file->interfile_header.clear();
    ecat_matrix::matrix_errno = ecat_matrix::MatrixError::UNKNOWN_FILE_TYPE;
    return 0;
  }
  std::string endian = g_header.hk.sizeof_hdr == sizeof(struct dsr)) ? "bigendian" : "littleendian";
  t_matrix_file->set_header(interfile::Key::IMAGEDATA_BYTE_ORDER, endian);
  main_header->num_planes = g_header.dime.dim[3];
  main_header->num_frames = g_header.dime.dim[4];
  main_header->num_gates = 1;

  std::string patient_id(g_header.hist.patient_id);
  strcpy(main_header->patient_id, patient_id.c_str());
  if (!patient_id.empty())
    t_matrix_file->set_header(interfile::Key::PATIENT_ID], patient_id);
  t_matrix_file->set_header(interfile::Key::NUMBER_OF_DIMENSIONS, std::to_string(3));
  /* check spm origin */
  short *dim = g_header.dime.dim;
  short  spm_origin[5];
  if (g_header.hk.sizeof_hdr == sizeof(struct dsr))
    swab(g_header.hist.originator, (char*)spm_origin, 10);
  else 
    memcpy(spm_origin, g_header.hist.originator, 10);
  if (spm_origin[0] > 1 && spm_origin[1] > 1 && spm_origin[2] > 1 &&
      spm_origin[0] < dim[1] && spm_origin[1] < dim[2] && spm_origin[2] < dim[3]) {
    t_matrix_file->set_header(interfile::Key::ATLAS_ORIGIN_1, std::to_string(spm_origin[0]));
    t_matrix_file->set_header(interfile::Key::ATLAS_ORIGIN_2, std::to_string(spm_origin[1]));
    t_matrix_file->set_header(interfile::Key::ATLAS_ORIGIN_3, std::to_string(spm_origin[2]));
  }
  t_matrix_file->set_header(interfile::Key::MATRIX_SIZE_1, std::to_string(dim[1]));
  t_matrix_file->set_header(interfile::Key::MATRIX_SIZE_2, std::to_string(dim[2]));
  t_matrix_file->set_header(interfile::Key::MATRIX_SIZE_3, std::to_string(dim[3]));
  t_matrix_file->interfile_header[interfile::Key::MAXIMUM_PIXEL_COUNT] = strdup(itoa(g_header.dime.glmax, buf, 10));
  // 0 = neurology, else radiology convention
  t_matrix_file->set_header(interfile::Key::MATRIX_INITIAL_ELEMENT_1, (analyze_orientation == 0) ? "right" : "left");
  t_matrix_file->set_header(interfile::Key::MATRIX_INITIAL_ELEMENT_2, "posterior");
  t_matrix_file->set_header(interfile::Key::MATRIX_INITIAL_ELEMENT_3, "inferior");
  t_matrix_file->set_header(interfile::Key::SCALE_FACTOR_1, std::to_string(g_header.dime.pixdim[1]));
  t_matrix_file->set_header(interfile::Key::SCALE_FACTOR_2, std::to_string(g_header.dime.pixdim[2]));
  t_matrix_file->set_header(interfile::Key::SCALE_FACTOR_3, std::to_string(g_header.dime.pixdim[3]));
  if (g_header.dime.funused1 > 0.0)
    t_matrix_file->set_header(interfile::Key::QUANTIFICATION_UNITS], std::to_string(g_header.dime.funused1));
  main_header->plane_separation = g_header.dime.pixdim[3] / 10; /* convert mm to cm */
  int data_size = g_header.dime.dim[1] * g_header.dime.dim[2] * g_header.dime.dim[3] * elem_size;
  t_matrix_file->dirlist = (ecat_matrix::MatDirList *) calloc(1, sizeof(ecat_matrix::MatDirList)) ;
  int nblks = (data_size + ecat_matrix::MatBLKSIZE - 1) / ecat_matrix::MatBLKSIZE;
  for (int frame = 1; frame <= main_header->num_frames; frame++) {
    ecat_matrix::MatDir matdir;
    matdir.matnum = ecat_matrix::mat_numcod(frame, 1, 1, 0, 0);
    matdir.strtblk = 0;  // was data_offset / ecat_matrix::MatBLKSIZE;
    matdir.endblk = matdir.strtblk + nblks;
    insert_mdir(&matdir, mptr->dirlist) ;
    matdir.strtblk += nblks;
  }
  return 1;
}

int process_datatype(analyze::DataKey const &t_datakey, ecat_matrix::MatrixFile &t_matrix_file, int &t_elem_size) {
  data_type = analyze::data_types_.at(t_datakey);
  t_elem_size = data_type.bytes_per_pixel;
  t_matrix_file->set_header([interfile::Key::NUMBER_FORMAT], data_type.description);
  t_matrix_file->set_header([interfile::Key::NUMBER_OF_BYTES_PER_PIXEL], fmt.format("{:d}", data_type.bytes_per_pixel));
}


