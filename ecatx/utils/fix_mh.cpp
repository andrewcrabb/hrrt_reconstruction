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
#include <assert.h>
#include <malloc.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "ecat_matrix.hpp"
#include "isotope_info.h"
#include "my_spdlog.hpp"

static void usage() {
  LOG_ERROR(" "fix_mh %s %s \n", __DATE__, __TIME__);
  LOG_ERROR("
           "usage : \n"
           "fix_mh -i input_file  -c ecf -w patient_weight -h patient_height -s patient_sex\n"
           "       -d dose -D delay -I isotope_name -R radiopharmacentical -t tilt -f n_frames\n"
           "       -p n_planes -A anonymity -v\n");
  LOG_ERROR(" " Updates main header ... \n");
  LOG_ERROR(" " -c ecat_calibration_factor, sets calibration units to Bq/ml\n");
  LOG_ERROR(" " -w patient_weight [kg]\n");
  LOG_ERROR(" " -h patient height [cm]\n");
  LOG_ERROR(" " -d injected_dose [mCi]\n");
  LOG_ERROR(" " -D delay between injection and emission scan (hh:mm:ss)\n");
  LOG_ERROR(" " -t tilt [degrees]\n");
  LOG_ERROR(" " -I Isotope name chosen amongst the following: \n");
  LOG_ERROR(" " Br-75 C-11 Cu-62 Cu-64 F-18 Fe-52 Ga-68 N-13\n");
  LOG_ERROR(" " O-14 O-15 Rb-82 Na-22 Zn-62 Br-76 K-38\n");
  LOG_ERROR(" " -R Radiopharmaceutical: eg. FDG \n");
  LOG_ERROR(" " -f number of frames \n");
  LOG_ERROR(" " -F Facility name \n");
  LOG_ERROR(" " -p number of planes \n");
  LOG_ERROR(" " -B initial_bed_pos\n");
  LOG_ERROR(" " -b reference_file for initial_bed_pos and offsets\n");
  LOG_ERROR(" " -s patient sex :M,F or U\n");
  LOG_ERROR(" " -M Modality(CT|MR|NM|PET)\n");
  LOG_ERROR(" " -N patient_name\n");
  LOG_ERROR(" " -S scan_start_time (dd/mm/yyyy hh:mm:ss)\n");
  LOG_ERROR(" " -A anonymity sets patient_id, patient_name, study_name and original_file_name to provided argument\n");
  LOG_ERROR(" "Version 8 : Nov 07, 2005\n");
  exit(1);
}
int main( argc, argv)
int argc;
char** argv;
{
  char *input_file = NULL;
  char *reference_file = NULL;
  char *patient_sex = NULL;
  char *sex_code = NULL;
  char *scan_time = NULL;
  ecat_matrix::MatrixFile *mptr, *mptr1;
  ecat_matrix::Main_header* mh;
  char facility_name[20];
  float ecf, weight, height, dose, branching_fraction, tilt;
  int i, nframes, nplanes;
  struct IsotopeInfoItem* isotope_info = NULL;
  char* radiopharmaceutical = NULL;
  char *modality = NULL;
  char *patient_name = NULL, *anonymity = NULL;
  int stype = 0, update_stype = 0;
  int hh = 0, mm = 0, ss = 0;
  time_t t;
  int c, j = 0, verbose = 0, calibrate = 0, update_weight = 0, update_height = 0, update_dose = 0, update_delay = 0, update_sex = 0;
  int update_isotope = 0, update_radiopharmaceutical = 0, update_tilt = 0, update_nframes = 0, update_nplanes = 0, update_bed = 0, update_modality = 0;
  int update_bed_pos = 0;
  int count, count1;
  float *bed_offsets, *bed_offsets1, bed_pos = 0;
  extern int optind, opterr;
  extern char *optarg;
  extern int getopt(int argc, char **argv, char *opts);

  memset(facility_name, 0, sizeof(facility_name));

  while ((c = getopt (argc, argv, "i:c:w:h:d:D:t:I:M:N:R:f:F:p:b:s:S:B:A:T:v")) != EOF) {

    switch (c) {
    case 'i' :
      input_file = optarg;
      break;
    case 'c' :
      if (sscanf(optarg, "%g", &ecf) != 1)
        LOG_EXIT("error decoding -c %s\n", optarg);
      calibrate = 1;
      break;
    case 'w' :
      if (sscanf(optarg, "%g", &weight) != 1)
        LOG_EXIT("error decoding -w %s\n", optarg);
      update_weight = 1;
      break;
    case 'h' :
      if (sscanf(optarg, "%g", &height) != 1)
        LOG_EXIT("error decoding -h %s\n", optarg);
      update_height = 1;
      break;
    case 'd' :
      if (sscanf(optarg, "%g", &dose) != 1)
        LOG_EXIT("error decoding -d %s\n", optarg);
      update_dose = 1;
      break;
    case 'D' :
      if (sscanf(optarg, "%d:%d:%d", &hh, &mm, &ss) != 3)
        LOG_EXIT("error decoding -D %s\n", optarg);
      update_delay = 1;
      break;
    case 't' :
      if (sscanf(optarg, "%g", &tilt) != 1)
        LOG_EXIT("error decoding -t %s\n", optarg);
      update_tilt = 1;
      break;
    case 'T' :
      if (sscanf(optarg, "%d", &stype) != 1)
        LOG_EXIT("error decoding -T %s\n", optarg);
      update_stype = 1;
      break;

    case 'I' :
      if ( (isotope_info = get_isotope_info(optarg)) == NULL) {
        LOG_ERROR( "ERROR: illegal isotope name: -i %s\n\n", optarg);
        return 1;
      }
      LOG_INFO("isotope name: {}; isotope halflife: {} sec",   isotope_info->name, isotope_info->halflife);
      update_isotope = 1;
      break;
    case 'M' :
      modality = optarg;
      update_modality = 1;
      break;
    case 'A' :
      anonymity = optarg;
      break;
    case 'N' :
      patient_name = optarg;
      break;
    case 'R' :
      radiopharmaceutical = optarg;
      update_radiopharmaceutical = 1;
      break;
    case 'f' :
      if (sscanf(optarg, "%d", &nframes) != 1)
        LOG_EXIT("error decoding -f {}", optarg);
      update_nframes = 1;
      break;
    case 'F':
      strncpy(facility_name, optarg, sizeof(facility_name) - 1);
      break;
    case 'p' :
      if (sscanf(optarg, "%d", &nplanes) != 1)
        LOG_EXIT("error decoding -p {}", optarg);
      update_nplanes = 1;
      break;
    case 'b' :
      reference_file = optarg;
      update_bed = 1;
      break;
    case 'B' :
      if (sscanf(optarg, "%g", &bed_pos) != 1)
        LOG_EXIT("error decoding -d {}", optarg);
      update_bed_pos = 1;
      break;
    case 'S' :
      scan_time = optarg;
      break;
    case 's' :
      sex_code = optarg;
      update_sex = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      usage();
    }
  }

  if (input_file == NULL) usage();

  LOG_DEBUG(" Input file : %s\n", input_file);

  mptr = matrix_open(input_file, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (!mptr) LOG_EXIT( "%s: can't open file '%s'\n", argv[0], input_file);
  mh = mptr->mhptr;
  if ( calibrate ) {
    if (verbose) {
      LOG_INFO("  old calibration factor : {}"     , mh->calibration_factor);
      LOG_INFO("  old calibration data units : {}" , mh->data_units);
      LOG_INFO("  old calibration units : {}"      , mh->calibration_units);
      LOG_INFO("  old calibration units_label : {}", mh->calibration_units_label);
    }
    mh->calibration_factor = ecf;
    mh->calibration_units = 1;
    mh->calibration_units_label = 1;
    strcpy(mh->data_units, "Bq/ml");
  }
  if ( update_dose ) {
    LOG_DEBUG("  old dose : %g\n", mh->dosage);
    mh->dosage = dose;
  }
  if ( update_weight ) {
    LOG_DEBUG("  old patient weight : %g\n", mh->patient_weight);
    mh->patient_weight = weight;
  }
  if (strlen(facility_name) > 0) {
    LOG_DEBUG("  old facility name : %s\n", mh->facility_name);
    strncpy(mh->facility_name, facility_name, sizeof(mh->facility_name) - 1);
  }

  if ( update_sex ) {
    if (verbose) {
      if (mh->patient_sex[0] == 0 || mh->patient_sex[0] == 'M')
        LOG_INFO("  old patient sex : M");
      else if (mh->patient_sex[0] == 1 || mh->patient_sex[0] == 'F')
        LOG_INFO("  old patient sex : F");
      else  
        LOG_INFO("  old patient sex : U");
    }
    mh->patient_sex[0] = sex_code[0];
  }
  if ( update_height ) {
    LOG_DEBUG("  old patient height : %g\n", mh->patient_height);
    mh->patient_height = height;
  }
  if ( update_tilt ) {
    LOG_DEBUG("  old tilt : %g\n", mh->intrinsic_tilt);
    mh->intrinsic_tilt = tilt;
  }
  if ( update_isotope ) {
    LOG_DEBUG("  old Isotope : %s\n", mh->isotope_code);
    strncpy(mh->isotope_code, isotope_info->name, sizeof(mh->isotope_code) - 1);
    mh->isotope_halflife = isotope_info->halflife;
    mh->branching_fraction = isotope_info->branch_ratio;
  }
  if ( update_modality ) {
    char *modality_pos = mh->magic_number + sizeof(mh->magic_number) - 4;
    LOG_DEBUG("  old modality : %s\n", modality_pos);
    strncpy(modality_pos, modality, 3);
  }
  if (patient_name) {
    LOG_DEBUG("  old patient name : %s\n", mh->patient_name);
    strncpy(mh->patient_name, patient_name, sizeof(mh->patient_name));
  }
  if (anonymity) {
    strncpy(mh->patient_id, anonymity, sizeof(mh->patient_id) - 1);
    strncpy(mh->patient_name, anonymity, sizeof(mh->patient_name) - 1);
    strncpy(mh->study_name, anonymity, sizeof(mh->study_name) - 1);
    strncpy(mh->original_file_name, anonymity,
            sizeof(mh->original_file_name) - 1);
  }
  if ( update_radiopharmaceutical ) {
    LOG_DEBUG("  old radiopharmaceutical : %s\n", mh->radiopharmaceutical);
    strncpy(mh->radiopharmaceutical, radiopharmaceutical, 32);
  }
  if ( update_nframes ) {
    LOG_DEBUG("  old number of frames : %d\n", mh->num_frames);
    mh->num_frames = nframes;
  }
  if ( update_nplanes ) {
    LOG_DEBUG("  old number of planes : %d\n", mh->num_planes);
    mh->num_planes = nplanes;
  }
  if (scan_time) {
    struct tm tm;
    int y, d, M, h, m, s = 0;
    if (sscanf(scan_time, "%d/%d/%d %d:%d:%d", &d, &M, &y, &h, &m, &s) < 5)
      LOG_EXIT ("bad time format : %s\n", scan_time);
    assert(d > 0 && d <= 31);
    assert(M > 0 && M <= 12);
    assert(y > 1999 && y <= 2098);
    assert(h > 0 && h <= 24);
    assert(m >= 0 && m < 60);
    assert(s >= 0 && s < 60);
    tm.tm_year = y - 1900;
    tm.tm_mon = M - 1;    /* 0-11 */
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = m;
    tm.tm_sec = s;
    tm.tm_isdst = -1;
#if defined(sun) && !defined(__SVR4)
    t = timelocal(&tm);
#else
    t = mktime(&tm);
#endif
    mh->dose_start_time = mh->scan_start_time = t;
    LOG_INFO("scan_start_time({}/{}/{} {}:{}:{}) = {}", y, d, M, h, m, s, ctime(&t));
  }

  if ( update_bed ) {
    mptr1 = matrix_open(reference_file, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
    if (!mptr1) 
      LOG_EXIT( "{}: can't open reference file '{}'", argv[0], reference_file);
    count = mptr->mhptr->num_bed_pos;
    bed_offsets = mptr->mhptr->bed_offset;
    count1 = mptr1->mhptr->num_bed_pos;
    bed_offsets1 = mptr1->mhptr->bed_offset;

      LOG_INFO("Initial bed position {} (old) {} (new)", mptr->mhptr->init_bed_position, mptr1->mhptr->init_bed_position);
      for (i = 0; i < count; i++)  
        LOG_DEBUG("Position: {} Old offset {}", i, bed_offsets[i]);
      for (i = 0; i < count1; i++) 
        LOG_DEBUG("Position: {} New offset {}", i, bed_offsets1[i]);

    mptr->mhptr->num_bed_pos = mptr1->mhptr->num_bed_pos;
    mptr->mhptr->init_bed_position = mptr1->mhptr->init_bed_position;
    memcpy(mptr->mhptr->bed_offset, mptr1->mhptr->bed_offset, sizeof(mptr->mhptr->bed_offset));
  }
  if ( update_delay) {
    t = mh->dose_start_time;
    if (verbose) {
      LOG_INFO("  old dose start time : {}", ctime(&t));
      LOG_INFO("  old delay since injection : {} sec", mh->scan_start_time - t);
    }
    mh->dose_start_time = mh->scan_start_time - (hh * 3600 + mm * 60 + ss);
  }
  if ( update_bed_pos ) {
    LOG_DEBUG("  old Initial bed position: %g\n", mh->init_bed_position);
    mh->init_bed_position = bed_pos;
  }
  if ( update_stype ) {
    LOG_DEBUG("  old system type: %g\n", mh->system_type);
    mh->system_type = stype;
  }
  mat_write_main_header(mptr->fptr, mh);
  matrix_close(mptr);
  return (0);
}
