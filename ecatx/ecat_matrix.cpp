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

/*  Modifications history
 * 21-sep-2010: Added data rates to image header (Merence sibomana)
 */

#include  "ecat_matrix.hpp"
#include  "machine_indep.h"

#include    <string>
#include    <malloc.h>

namespace ecat_matrix {

#define TRUE 1
#define FALSE 0

const std::string magicNumber = "MATRIX";

// // char* datasettype[NumDataSetTypes] =
//   std::vector<std::string> datasettype_ = {
//     "Unknown", "Sinogram", "Image-16", "Attenuation Correction",
//     "Normalization", "Polar Map", "Volume-8", "Volume-16", "Projection-8",
//     "Projection-16", "Image-8", "3D Sinogram-16", "3D Sinogram-8",
//     "3D Normalization", "Float3dSinogram", "Interfile"
//   };

// // char* dstypecode[NumDataSetTypes] =
//   std::vector<std::string> dstypecode_ = {
//     "u","s","i","a","n","pm","v8","v","p8","p","i8","S","S8","N", "FS"
//   };

// char* scantype[NumScanTypes] = {
//     "Not Applicable", "Blank Scan", "Transmission Scan",
//     "Static Emission Scan", "Dynamic Emission", "Gated Emission",
//     "Transmission Rectilinear", "Emission Rectilinear"
//     };

// char* scantypecode[NumScanTypes] =
//     { "", "bl", "tx", "se", "de", "ge", "tr", "er"};

// int numDisplayUnits = 13;
// char* customDisplayUnits[] =

/* for modelled images :
 * set Main_header calibration_units to 2(Processed) and
 * FDG : calibration_units_label to 5("micromole/100g/min") or 6("mg/100g/min")
 * FLOW : calibration_units_label to 7("ml/100g/min")
 *
 * for other custom units :
 * set Main_header calibration_units_label to 0 and data_units to "my units"
 */

// float ecfconverter[NumOldUnits] = {1.0, 1.0, 1.0, 3.7e4, 1.0, 1.0, 1.0, 37.0, 1.0, 1.0, 1.0, 1.0};
// char* calstatus[NumCalibrationStatus] = {"Uncalibrated", "Calibrated", "Processed"};
// char* ecfunits[NumCalibrationStatus] =   {"ECAT Counts/Sec", "Bq/ml", "Processed"};

const std::string sexcode = "MFU";
const std::string dexteritycode = "RLUA";

FILE *mat_open( char *fname, char *fmode) {
  FILE  *mat_file_ptr = NULL;

  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();
  mat_file_ptr = fopen(fname, fmode);
  return (mat_file_ptr);

}

int mat_close(FILE *t_file_ptr) {
  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();
  return fclose( t_file_ptr);
}

int mat_rblk(FILE *t_file_ptr,  int blkno, char *bufr, int nblks) {
  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();

  if ( fseek( t_file_ptr, (blkno - 1)* MatBLKSIZE, 0) )
    return ( ecat_matrix::ECATX_ERROR );

  int err = fread( bufr, 1, nblks * MatBLKSIZE, t_file_ptr);
  if ( err == ecat_matrix::ECATX_ERROR ) {
    return ( ecat_matrix::ECATX_ERROR );
  }
  /*  some applications write pixel instead of block count
   ==> check if byte count less than (nblks-1) (M. Sibomana 23-oct-1997) */
  else if ( err == 0 || err < (nblks - 1)* MatBLKSIZE ) {
    matrix_errno = MatrixError::READ_ERROR;
    return ( ecat_matrix::ECATX_ERROR );
  }
  return ( 0 );
}

// void swaw( short *from, short *to, int length) {
//   short int temp;

//   for (int i = 0; i < length; i += 2) {
//     temp = from[i + 1];
//     to[i + 1] = from[i];
//     to[i] = temp;
//   }
// }

int mat_numcod( int frame, int plane, int gate, int data, int bed) {
  return ((frame) | ((bed & 0xF) << 12) | ((plane & 0xFF) << 16) | (((plane & 0x300) >> 8) << 9) |
          ((gate & 0x3F) << 24) | ((data & 0x3) << 30) | ((data & 0x4) << 9));
}

int mat_numdoc( int matnum, MatVal *matval) {
  matval->frame = matnum & 0x1FF;
  matval->plane = ((matnum >> 16) & 0xFF) + (((matnum >> 9) & 0x3) << 8);
  matval->gate  = (matnum >> 24) & 0x3F;
  matval->data  = ((matnum >> 9) & 0x4) | (matnum >> 30) & 0x3;
  matval->bed   = (matnum >> 12) & 0xF;
  return 1;
}

int mat_lookup(FILE *t_file_ptr, Main_header *mhptr, int matnum, MatDir *entry) {

  int blk, i;
  int nfree, nxtblk, prvblk, nused, matnbr, strtblk, endblk, matstat;
  int dirbufr[ MatBLKSIZE / 4];

  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();
  blk = MatFirstDirBlk;
  while (1) {
    if ( read_matrix_data(t_file_ptr, blk, 1, (char*)dirbufr, SunLong) == ecat_matrix::ECATX_ERROR )
      return ( ecat_matrix::ECATX_ERROR );
    nfree  = dirbufr[0];
    nxtblk = dirbufr[1];
    prvblk = dirbufr[2];
    nused  = dirbufr[3];
    for (i = 4; i < MatBLKSIZE / 4; i += 4) {
      matnbr  = dirbufr[i];
      strtblk = dirbufr[i + 1];
      endblk  = dirbufr[i + 2];
      matstat = dirbufr[i + 3];
      if (matnum == matnbr) {
        entry->matnum  = matnbr;
        entry->strtblk = strtblk;
        entry->endblk  = endblk;
        entry->matstat = matstat;
        return (1);
      }
    }
    blk = nxtblk;
    if (blk == MatFirstDirBlk)
      break;
  }
  return (0);
}

int unmap_main_header( char *bufr, Main_header *header) {
  int i = MagicNumLen;      /* skip magic number */
  int j = 0;
  unsigned one_week = 7 * 24 * 3600;  /* 7 x 24 hours in seconds */

  bufRead(header->original_file_name, bufr, &i, NameLen);
  header->original_file_name[NameLen - 1] = '\0';
  bufRead_s(&header->sw_version , bufr, &i);
  bufRead_s(&header->system_type, bufr, &i);
  bufRead_s(&header->file_type  , bufr, &i);
  bufRead(header->serial_number , bufr, &i, 10);
  header->serial_number[9] = '\0';
  bufRead_u(&header->scan_start_time, bufr, &i);
  bufRead(header->isotope_code, bufr, &i, 8);
  header->isotope_code[7] = '\0';
  bufRead_f(&header->isotope_halflife, bufr, &i);
  bufRead(header->radiopharmaceutical, bufr, &i, NameLen);
  header->radiopharmaceutical[NameLen - 1] = '\0';
  bufRead_f(&header->gantry_tilt            , bufr, &i);
  bufRead_f(&header->gantry_rotation        , bufr, &i);
  bufRead_f(&header->bed_elevation          , bufr, &i);
  bufRead_f(&header->intrinsic_tilt         , bufr, &i);
  bufRead_s(&header->wobble_speed           , bufr, &i);
  bufRead_s(&header->transm_source_type     , bufr, &i);
  bufRead_f(&header->distance_scanned       , bufr, &i);
  bufRead_f(&header->transaxial_fov         , bufr, &i);
  bufRead_s(&header->angular_compression    , bufr, &i);
  bufRead_s(&header->coin_samp_mode         , bufr, &i);
  bufRead_s(&header->axial_samp_mode        , bufr, &i);
  bufRead_f(&header->calibration_factor     , bufr, &i);
  bufRead_s(&header->calibration_units      , bufr, &i);
  bufRead_s(&header->calibration_units_label, bufr, &i);
  bufRead_s(&header->compression_code       , bufr, &i);
  bufRead(header->study_name                , bufr, &i, 12);
  header->study_name[11] = '\0';
  bufRead(header->patient_id, bufr, &i, IDLen);
  header->patient_id[IDLen - 1] = '\0';
  bufRead(header->patient_name, bufr, &i, NameLen);
  header->patient_name[NameLen - 1] = '\0';
  bufRead(header->patient_sex, bufr, &i, 1);
  bufRead(header->patient_dexterity, bufr, &i, 1);
  bufRead_f(&header->patient_age       , bufr, &i);
  bufRead_f(&header->patient_height    , bufr, &i);
  bufRead_f(&header->patient_weight    , bufr, &i);
  bufRead_i(&header->patient_birth_date, bufr, &i);
  bufRead(header->physician_name       , bufr, &i, NameLen);
  header->physician_name[NameLen - 1] = '\0';
  bufRead(header->operator_name, bufr, &i, NameLen);
  header->operator_name[NameLen - 1] = '\0';
  bufRead(header->study_description, bufr, &i, NameLen);
  header->study_description[NameLen - 1] = '\0';
  bufRead_s(&header->acquisition_type, bufr, &i);
  bufRead_s(&header->patient_orientation, bufr, &i);
  bufRead(header->facility_name, bufr, &i, 20);
  header->facility_name[19] = '\0';
  bufRead_s(&header->num_planes       , bufr, &i);
  bufRead_s(&header->num_frames       , bufr, &i);
  bufRead_s(&header->num_gates        , bufr, &i);
  bufRead_s(&header->num_bed_pos      , bufr, &i);
  bufRead_f(&header->init_bed_position, bufr, &i);
  for (j = 0; j < 15; j++)
    bufRead_f(&header->bed_offset[j], bufr, &i);
    bufRead_f(&header->plane_separation, bufr, &i);
    bufRead_s(&header->lwr_sctr_thres, bufr, &i);
    bufRead_s(&header->lwr_true_thres, bufr, &i);
    bufRead_s(&header->upr_true_thres, bufr, &i);
    bufRead(header->user_process_code, bufr, &i, 10);
  header->user_process_code[9] = '\0';
  bufRead_s(&header->acquisition_mode  , bufr, &i);
  bufRead_f(&header->bin_size          , bufr, &i);
  bufRead_f(&header->branching_fraction, bufr, &i);
  bufRead_u(&header->dose_start_time   , bufr, &i);
  if (header->dose_start_time < one_week)
  { /* assume 7.0 encoding */
    // fprintf(stderr,"converting V7.0 dose start time encoding\n");
    header->dose_start_time = header->scan_start_time -
                              header->dose_start_time;
  }
  bufRead_f(&header->dosage, bufr, &i);
  bufRead_f(&header->well_counter_factor, bufr, &i);
  bufRead(header->data_units, bufr, &i, 32);
  header->data_units[31] = '\0';
  bufRead_s(&header->septa_state, bufr, &i);
  return 0;
}

int mat_read_main_header(FILE *t_file_ptr, Main_header *header) {
  int             i = 0;
  char            bufr[ MatBLKSIZE];

  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();

  /* check magic number */
  if (mat_rblk(t_file_ptr, 1, bufr, 1) == ecat_matrix::ECATX_ERROR)
    return (ecat_matrix::ECATX_ERROR);
  bufRead(header->magic_number, bufr, &i, MagicNumLen);
  header->magic_number[MagicNumLen - 1] = '\0';
  return unmap_main_header(bufr, header);
}

int mat_read_matrix_data(FILE *t_file_ptr, Main_header *mhptr, int blk, int nblks, short *bufr) {
  return read_matrix_data(t_file_ptr, blk, nblks, (char *) bufr, MatrixData::DataType::SunShort);
}

int unmap_scan_header(char *buf, Scan_subheader *header) {
  int i = 0;

  bufRead_s(&header->data_type          , buf, &i);
  bufRead_s(&header->num_dimensions     , buf, &i);
  bufRead_s(&header->num_r_elements     , buf, &i);
  bufRead_s(&header->num_angles         , buf, &i);
  bufRead_s(&header->corrections_applied, buf, &i);
  bufRead_s(&header->num_z_elements     , buf, &i);
  bufRead_s(&header->ring_difference    , buf, &i);
  bufRead_f(&header->x_resolution       , buf, &i);
  bufRead_f(&header->y_resolution       , buf, &i);
  bufRead_f(&header->z_resolution       , buf, &i);
  bufRead_f(&header->w_resolution       , buf, &i);
  i += 6 * sizeof(short);   /* space reserved for future gating info */
  bufRead_u(&header->gate_duration     , buf, &i);
  bufRead_i(&header->r_wave_offset     , buf, &i);
  bufRead_i(&header->num_accepted_beats, buf, &i);
  bufRead_f(&header->scale_factor      , buf, &i);
  bufRead_s(&header->scan_min          , buf, &i);
  bufRead_s(&header->scan_max          , buf, &i);
  bufRead_i(&header->prompts           , buf, &i);
  bufRead_i(&header->delayed           , buf, &i);
  bufRead_i(&header->multiples         , buf, &i);
  bufRead_i(&header->net_trues         , buf, &i);
  for (int j = 0; j < 16; j++)
    bufRead_f(&header->cor_singles[j], buf, &i);
  for (int j = 0; j < 16; j++)
    bufRead_f(&header->uncor_singles[j], buf, &i);
    bufRead_f(&header->tot_avg_cor, buf, &i);
    bufRead_f(&header->tot_avg_uncor, buf, &i);
    bufRead_i(&header->total_coin_rate, buf, &i);
    bufRead_u(&header->frame_start_time, buf, &i);
    bufRead_u(&header->frame_duration, buf, &i);
    bufRead_f(&header->loss_correction_fctr, buf, &i);
  for (j = 0; j < 8; j++)
    bufRead_s(&header->phy_planes[j], buf, &i);
  return 0;
}

int unmap_Scan3D_header(char *buf, Scan3D_subheader *header) {
  int i = 0;
  int j;
  bufRead_s(&header->data_type          , buf, &i);
  bufRead_s(&header->num_dimensions     , buf, &i);
  bufRead_s(&header->num_r_elements     , buf, &i);
  bufRead_s(&header->num_angles         , buf, &i);
  bufRead_s(&header->corrections_applied, buf, &i);
  for (j = 0; j < 64; j++)
    bufRead_s(&header->num_z_elements[j], buf, &i);
  bufRead_s(&header->ring_difference  , buf, &i);
  bufRead_s(&header->storage_order    , buf, &i);
  bufRead_s(&header->axial_compression, buf, &i);
  bufRead_f(&header->x_resolution     , buf, &i);
  bufRead_f(&header->v_resolution     , buf, &i);
  bufRead_f(&header->z_resolution     , buf, &i);
  bufRead_f(&header->w_resolution     , buf, &i);
  i += 6 * sizeof(short);  /* space reserved for future gating info */
  bufRead_u(&header->gate_duration       , buf, &i);
  bufRead_i(&header->r_wave_offset       , buf, &i);
  bufRead_i(&header->num_accepted_beats  , buf, &i);
  bufRead_f(&header->scale_factor        , buf, &i);
  bufRead_s(&header->scan_min            , buf, &i);
  bufRead_s(&header->scan_max            , buf, &i);
  bufRead_i(&header->prompts             , buf, &i);
  bufRead_i(&header->delayed             , buf, &i);
  bufRead_i(&header->multiples           , buf, &i);
  bufRead_i(&header->net_trues           , buf, &i);
  bufRead_f(&header->tot_avg_cor         , buf, &i);
  bufRead_f(&header->tot_avg_uncor       , buf, &i);
  bufRead_i(&header->total_coin_rate     , buf, &i);
  bufRead_u(&header->frame_start_time    , buf, &i);
  bufRead_u(&header->frame_duration      , buf, &i);
  bufRead_f(&header->loss_correction_fctr, buf, &i);
  i += 90 * sizeof(short);  /* CTI reserved space */
  i += 50 * sizeof(short);  /* user reserved space */
  for (j = 0; j < 128; j++)
    bufRead_f(&header->uncor_singles[j], buf, &i);
  return 0;
}

int map_Scan3D_header(char *buf, Scan3D_subheader *header) {
  int i = 0;
  int j;
  bufWrite_s(header->data_type          , buf, &i);
  bufWrite_s(header->num_dimensions     , buf, &i);
  bufWrite_s(header->num_r_elements     , buf, &i);
  bufWrite_s(header->num_angles         , buf, &i);
  bufWrite_s(header->corrections_applied, buf, &i);
  for (j = 0; j < 64; j++)
    bufWrite_s(header->num_z_elements[j], buf, &i);
  bufWrite_s(header->ring_difference  , buf, &i);
  bufWrite_s(header->storage_order    , buf, &i);
  bufWrite_s(header->axial_compression, buf, &i);
  bufWrite_f(header->x_resolution     , buf, &i);
  bufWrite_f(header->v_resolution     , buf, &i);
  bufWrite_f(header->z_resolution     , buf, &i);
  bufWrite_f(header->w_resolution     , buf, &i);
  i += 6 * sizeof(short);  /* space reserved for future gating info */
  bufWrite_u(header->gate_duration       , buf, &i);
  bufWrite_i(header->r_wave_offset       , buf, &i);
  bufWrite_i(header->num_accepted_beats  , buf, &i);
  bufWrite_f(header->scale_factor        , buf, &i);
  bufWrite_s(header->scan_min            , buf, &i);
  bufWrite_s(header->scan_max            , buf, &i);
  bufWrite_i(header->prompts             , buf, &i);
  bufWrite_i(header->delayed             , buf, &i);
  bufWrite_i(header->multiples           , buf, &i);
  bufWrite_i(header->net_trues           , buf, &i);
  bufWrite_f(header->tot_avg_cor         , buf, &i);
  bufWrite_f(header->tot_avg_uncor       , buf, &i);
  bufWrite_i(header->total_coin_rate     , buf, &i);
  bufWrite_u(header->frame_start_time    , buf, &i);
  bufWrite_u(header->frame_duration      , buf, &i);
  bufWrite_f(header->loss_correction_fctr, buf, &i);
  i += 90 * sizeof(short);  /* CTI reserved space */
  i += 50 * sizeof(short);  /* user reserved space */
  for (j = 0; j < 128; j++)
    bufWrite_f(header->uncor_singles[j], buf, &i);
  return 0;
}

int mat_read_Scan3D_subheader( FILE *t_file_ptr, Main_header *mhptr, int blknum,   Scan3D_subheader *header) {
  char buf[2 * MatBLKSIZE];
  if ( mat_rblk( t_file_ptr, blknum, buf, 2) == ecat_matrix::ECATX_ERROR )
    return ( ecat_matrix::ECATX_ERROR );
  return unmap_Scan3D_header(buf, header);
}

int mat_read_scan_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,   Scan_subheader *header) {
  char            buf[ MatBLKSIZE];

  if (mat_rblk(t_file_ptr, blknum, buf, 1) == ecat_matrix::ECATX_ERROR)
    return (ecat_matrix::ECATX_ERROR);
  return unmap_scan_header(buf, header);
}

int unmap_image_header(char *buf, Image_subheader *header) {
  int i = 0;

  bufRead_s(&header->data_type              , buf, &i);
  bufRead_s(&header->num_dimensions         , buf, &i);
  bufRead_s(&header->x_dimension            , buf, &i);
  bufRead_s(&header->y_dimension            , buf, &i);
  bufRead_s(&header->z_dimension            , buf, &i);
  bufRead_f(&header->z_offset               , buf, &i);
  bufRead_f(&header->x_offset               , buf, &i);
  bufRead_f(&header->y_offset               , buf, &i);
  bufRead_f(&header->recon_zoom             , buf, &i);
  bufRead_f(&header->scale_factor           , buf, &i);
  bufRead_s(&header->image_min              , buf, &i);
  bufRead_s(&header->image_max              , buf, &i);
  bufRead_f(&header->x_pixel_size           , buf, &i);
  bufRead_f(&header->y_pixel_size           , buf, &i);
  bufRead_f(&header->z_pixel_size           , buf, &i);
  bufRead_u(&header->frame_duration         , buf, &i);
  bufRead_u(&header->frame_start_time       , buf, &i);
  bufRead_s(&header->filter_code            , buf, &i);
  bufRead_f(&header->x_resolution           , buf, &i);
  bufRead_f(&header->y_resolution           , buf, &i);
  bufRead_f(&header->z_resolution           , buf, &i);
  bufRead_f(&header->num_r_elements         , buf, &i);
  bufRead_f(&header->num_angles             , buf, &i);
  bufRead_f(&header->z_rotation_angle       , buf, &i);
  bufRead_f(&header->decay_corr_fctr        , buf, &i);
  bufRead_i(&header->processing_code        , buf, &i);
  bufRead_u(&header->gate_duration          , buf, &i);
  bufRead_i(&header->r_wave_offset          , buf, &i);
  bufRead_i(&header->num_accepted_beats     , buf, &i);
  bufRead_f(&header->filter_cutoff_frequency, buf, &i);
  bufRead_f(&header->filter_resolution      , buf, &i);
  bufRead_f(&header->filter_ramp_slope      , buf, &i);
  bufRead_s(&header->filter_order           , buf, &i);
  bufRead_f(&header->filter_scatter_fraction, buf, &i);
  bufRead_f(&header->filter_scatter_slope   , buf, &i);
  bufRead(header->annotation                , buf, &i, 40);
  bufRead_f(&header->mt_1_1                 , buf, &i);
  bufRead_f(&header->mt_1_2                 , buf, &i);
  bufRead_f(&header->mt_1_3                 , buf, &i);
  bufRead_f(&header->mt_2_1                 , buf, &i);
  bufRead_f(&header->mt_2_2                 , buf, &i);
  bufRead_f(&header->mt_2_3                 , buf, &i);
  bufRead_f(&header->mt_3_1                 , buf, &i);
  bufRead_f(&header->mt_3_2                 , buf, &i);
  bufRead_f(&header->mt_3_3                 , buf, &i);
  bufRead_f(&header->rfilter_cutoff         , buf, &i);
  bufRead_f(&header->rfilter_resolution     , buf, &i);
  bufRead_s(&header->rfilter_code           , buf, &i);
  bufRead_s(&header->rfilter_order          , buf, &i);
  bufRead_f(&header->zfilter_cutoff         , buf, &i);
  bufRead_f(&header->zfilter_resolution     , buf, &i);
  bufRead_s(&header->zfilter_code           , buf, &i);
  bufRead_s(&header->zfilter_order          , buf, &i);
  bufRead_f(&header->mt_1_4                 , buf, &i);
  bufRead_f(&header->mt_2_4                 , buf, &i);
  bufRead_f(&header->mt_3_4                 , buf, &i);
  bufRead_s(&header->scatter_type           , buf, &i);
  bufRead_s(&header->recon_type             , buf, &i);
  bufRead_s(&header->recon_views            , buf, &i);
  bufRead_f(&header->prompt_rate            , buf, &i);
  bufRead_f(&header->random_rate            , buf, &i);
  bufRead_f(&header->singles_rate           , buf, &i);
  bufRead_f(&header->scatter_fraction       , buf, &i);
  return 0;
}

int mat_read_image_subheader( FILE *t_file_ptr, Main_header *mhptr, int blknum, Image_subheader *header) {
  char buf[ MatBLKSIZE];

  if ( mat_rblk( t_file_ptr, blknum, buf, 1) == ecat_matrix::ECATX_ERROR ) return ( ecat_matrix::ECATX_ERROR );
  return unmap_image_header(buf, header);
}

struct matdir *mat_read_dir( FILE *t_file_ptr, Main_header *mhptr, char *selector) {
  int i, n, blk, nxtblk, ndblks, bufr[128];
  struct matdir *dir;

  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();
  blk = MatFirstDirBlk;
  nxtblk = 0;
  for (ndblks = 0; nxtblk != MatFirstDirBlk; ndblks++)  {
    read_matrix_data(t_file_ptr, blk, 1, (char*)bufr, SunLong);
    nxtblk = bufr[1];
    blk = nxtblk;
  }
  dir = (struct matdir*) malloc( sizeof(struct matdir));
  dir->nmats = 0;
  dir->nmax = 31 * ndblks;
  dir->entry = (MatDir *) malloc(31 * ndblks * sizeof(MatDir));
  for (n = 0, nxtblk = 0, blk = MatFirstDirBlk; nxtblk != MatFirstDirBlk; blk = nxtblk) {
    read_matrix_data(t_file_ptr, blk, 1, (char*)bufr, SunLong);

    nxtblk = bufr[1];
    for (i = 4; i < MatBLKSIZE / 4; n++)    {
      dir->entry[n].matnum = bufr[i++];
      dir->entry[n].strtblk = bufr[i++];
      dir->entry[n].endblk = bufr[i++];
      dir->entry[n].matstat = bufr[i++];
      if (dir->entry[n].matnum != 0)
        dir->nmats++;
    }
  }
  return dir;
}


int mat_wblk(FILE *t_file_ptr, int blkno, char *bufr, int nblks) {
  int             err;

  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();

  /* seek to position in file */

  err = fseek(t_file_ptr, (blkno - 1) *  MatBLKSIZE, 0);
  if (err)
    return (ecat_matrix::ECATX_ERROR);

  /* write matrix data */
  err = fwrite(bufr, 1, nblks *  MatBLKSIZE, t_file_ptr);
  if (err == -1)
    return (ecat_matrix::ECATX_ERROR);
  if (err != nblks *  MatBLKSIZE) {
    matrix_errno = MatrixError::WRITE_ERROR;
    return (ecat_matrix::ECATX_ERROR);
  }
  return (0);
}

FILE *mat_create( char *fname, Main_header *mhead) {
  FILE *mat_file_ptr;
  int bufr[ MatBLKSIZE / sizeof(int)];
  int ret;

  mat_file_ptr = mat_open( fname, W_MODE);
  if (!mat_file_ptr)
    return ( NULL );
  ret = mat_write_main_header( mat_file_ptr, mhead );
  if ( ret != 0 ) {
    mat_close( mat_file_ptr);
    return ( NULL );
  }
  memset(bufr, 0, MatBLKSIZE);
  bufr[0] = 31;
  bufr[1] = 2;
  ret = write_matrix_data(mat_file_ptr, MatFirstDirBlk, 1, (char*)bufr, SunLong);
  if ( ret != 0 ) {
    mat_close( mat_file_ptr);
    return ( NULL );
  }
  return (mat_file_ptr);
}

int mat_enter(FILE *t_file_ptr, Main_header *mhptr, int matnum, int nblks) {
  int             dirblk, dirbufr[128], i, nxtblk, busy, oldsize;
  short           sw_version = mhptr->sw_version;

  matrix_errno = MatrixError::OK;
  ecat_matrix::matrix_errtxt.clear();
  dirblk = MatFirstDirBlk;
  if ( fseek(t_file_ptr, 0, 0) )
    return ( ecat_matrix::ECATX_ERROR );
  if (read_matrix_data(t_file_ptr, dirblk, 1, (char *) dirbufr, SunLong) == ecat_matrix::ECATX_ERROR)
    return (ecat_matrix::ECATX_ERROR);
  busy = 1;
  while (busy) {
    nxtblk = dirblk + 1;
    for (i = 4; i < 128; i += 4) {
      if (dirbufr[i] == 0) {
        busy = 0;
        break;
      } else if (dirbufr[i] == matnum) {
        oldsize = dirbufr[i + 2] - dirbufr[i + 1] + 1;
        if (oldsize < nblks) {
          dirbufr[i] = 0xFFFFFFFF;
          write_matrix_data(t_file_ptr, dirblk, 1, (char*)dirbufr, SunLong);
          nxtblk = dirbufr[i + 2] + 1;
        } else {
          nxtblk = dirbufr[i + 1];
          dirbufr[0]++;
          dirbufr[3]--;
          busy = 0;
          break;
        }
      } else {
        nxtblk = dirbufr[i + 2] + 1;
      }
    }
    if (!busy)
      break;
    if (dirbufr[1] != MatFirstDirBlk) {
      dirblk = dirbufr[1];
      if (read_matrix_data(t_file_ptr, dirblk, 1, (char *) dirbufr, SunLong) == ecat_matrix::ECATX_ERROR)
        return (ecat_matrix::ECATX_ERROR);
    } else {
      dirbufr[1] = nxtblk;
      if (write_matrix_data(t_file_ptr, dirblk, 1, (char *) dirbufr, SunLong) == ecat_matrix::ECATX_ERROR)
        return (ecat_matrix::ECATX_ERROR);
      dirbufr[0] = 31;
      dirbufr[1] = MatFirstDirBlk;
      dirbufr[2] = dirblk;
      dirbufr[3] = 0;
      dirblk = nxtblk;
      for (i = 4; i < 128; i++)
        dirbufr[i] = 0;
    }
  }
  dirbufr[i] = matnum;
  dirbufr[i + 1] = nxtblk;
  dirbufr[i + 2] = nxtblk + nblks;
  dirbufr[i + 3] = 1;
  dirbufr[0]--;
  dirbufr[3]++;
  if (write_matrix_data(t_file_ptr, dirblk, 1, (char*)dirbufr, SunLong) == ecat_matrix::ECATX_ERROR)
    return (ecat_matrix::ECATX_ERROR);
  if ( fseek(t_file_ptr, 0, 0) ) 
    return ( ecat_matrix::ECATX_ERROR );
  return (nxtblk);
}

int mat_write_data( FILE *t_file_ptr, int blk, int nbytes, char *data, int dtype) {
  int nblks;

  nblks = (511 + nbytes) / 512;
  return write_matrix_data( t_file_ptr, blk, nblks, data, dtype);
}

int mat_read_data( FILE *t_file_ptr, int blk, int nbytes, char *data, int dtype) {
  int nblks;

  nblks = (511 + nbytes) / 512;
  return read_matrix_data( t_file_ptr, blk, nblks, data, dtype);
}

int map_main_header(char *bufr, Main_header *header) {
  int i = 0, j = 0;
  char mn[20];
  /* set magic number */
  sprintf(mn, "%s%d%s", magicNumber, header->sw_version, data_set_types_.at(header->file_type).name);
  bufWrite(mn, bufr, &i, 14);

  /* copy buffer into struct */
  bufWrite(header->original_file_name, bufr, &i, NameLen);
  bufWrite_s(header->sw_version, bufr, &i);
  bufWrite_s(header->system_type, bufr, &i);
  bufWrite_s(header->file_type, bufr, &i);
  bufWrite(header->serial_number, bufr, &i, 10);
  bufWrite_u(header->scan_start_time, bufr, &i);
  bufWrite(header->isotope_code, bufr, &i, 8);
  bufWrite_f(header->isotope_halflife, bufr, &i);
  bufWrite(header->radiopharmaceutical, bufr, &i, NameLen);
  bufWrite_f(header->gantry_tilt, bufr, &i);
  bufWrite_f(header->gantry_rotation, bufr, &i);
  bufWrite_f(header->bed_elevation, bufr, &i);
  bufWrite_f(header->intrinsic_tilt, bufr, &i);
  bufWrite_s(header->wobble_speed, bufr, &i);
  bufWrite_s(header->transm_source_type, bufr, &i);
  bufWrite_f(header->distance_scanned, bufr, &i);
  bufWrite_f(header->transaxial_fov, bufr, &i);
  bufWrite_s(header->angular_compression, bufr, &i);
  bufWrite_s(header->coin_samp_mode, bufr, &i);
  bufWrite_s(header->axial_samp_mode, bufr, &i);
  bufWrite_f(header->calibration_factor, bufr, &i);
  bufWrite_s(header->calibration_units, bufr, &i);
  bufWrite_s(header->calibration_units_label, bufr, &i);
  bufWrite_s(header->compression_code, bufr, &i);
  bufWrite(header->study_name, bufr, &i, 12);
  bufWrite(header->patient_id, bufr, &i, IDLen);
  bufWrite(header->patient_name, bufr, &i, NameLen);
  bufWrite(header->patient_sex, bufr, &i, 1);
  bufWrite(header->patient_dexterity, bufr, &i, 1);
  bufWrite_f(header->patient_age, bufr, &i);
  bufWrite_f(header->patient_height, bufr, &i);
  bufWrite_f(header->patient_weight, bufr, &i);
  bufWrite_i(header->patient_birth_date, bufr, &i);
  bufWrite(header->physician_name, bufr, &i, NameLen);
  bufWrite(header->operator_name, bufr, &i, NameLen);
  bufWrite(header->study_description, bufr, &i, NameLen);
  bufWrite_s(header->acquisition_type, bufr, &i);
  bufWrite_s(header->patient_orientation, bufr, &i);
  bufWrite(header->facility_name, bufr, &i, 20);
  bufWrite_s(header->num_planes, bufr, &i);
  bufWrite_s(header->num_frames, bufr, &i);
  bufWrite_s(header->num_gates, bufr, &i);
  bufWrite_s(header->num_bed_pos, bufr, &i);
  bufWrite_f(header->init_bed_position, bufr, &i);
  for (j = 0; j < 15; j++)
    bufWrite_f(header->bed_offset[j], bufr, &i);
  bufWrite_f(header->plane_separation, bufr, &i);
  bufWrite_s(header->lwr_sctr_thres, bufr, &i);
  bufWrite_s(header->lwr_true_thres, bufr, &i);
  bufWrite_s(header->upr_true_thres, bufr, &i);
  bufWrite(header->user_process_code, bufr, &i, 10);
  bufWrite_s(header->acquisition_mode, bufr, &i);
  bufWrite_f(header->bin_size, bufr, &i);
  bufWrite_f(header->branching_fraction, bufr, &i);
  bufWrite_u(header->dose_start_time, bufr, &i);
  bufWrite_f(header->dosage, bufr, &i);
  bufWrite_f(header->well_counter_factor, bufr, &i);
  bufWrite(header->data_units, bufr, &i, 32);
  bufWrite_s(header->septa_state, bufr, &i);
  return 1;
}

int mat_write_main_header(FILE *t_file_ptr, Main_header *header) {
  char            bufr[ MatBLKSIZE];


  map_main_header(bufr, header);
  return mat_wblk(t_file_ptr, 1, bufr, 1);  /* write main header at block 1 */
}

int map_image_header(char *buf, Image_subheader *header) {
  int i = 0;
  bufWrite_s(header->data_type              , buf, &i);
  bufWrite_s(header->num_dimensions         , buf, &i);
  bufWrite_s(header->x_dimension            , buf, &i);
  bufWrite_s(header->y_dimension            , buf, &i);
  bufWrite_s(header->z_dimension            , buf, &i);
  bufWrite_f(header->z_offset               , buf, &i);
  bufWrite_f(header->x_offset               , buf, &i);
  bufWrite_f(header->y_offset               , buf, &i);
  bufWrite_f(header->recon_zoom             , buf, &i);
  bufWrite_f(header->scale_factor           , buf, &i);
  bufWrite_s(header->image_min              , buf, &i);
  bufWrite_s(header->image_max              , buf, &i);
  bufWrite_f(header->x_pixel_size           , buf, &i);
  bufWrite_f(header->y_pixel_size           , buf, &i);
  bufWrite_f(header->z_pixel_size           , buf, &i);
  bufWrite_u(header->frame_duration         , buf, &i);
  bufWrite_u(header->frame_start_time       , buf, &i);
  bufWrite_s(header->filter_code            , buf, &i);
  bufWrite_f(header->x_resolution           , buf, &i);
  bufWrite_f(header->y_resolution           , buf, &i);
  bufWrite_f(header->z_resolution           , buf, &i);
  bufWrite_f(header->num_r_elements         , buf, &i);
  bufWrite_f(header->num_angles             , buf, &i);
  bufWrite_f(header->z_rotation_angle       , buf, &i);
  bufWrite_f(header->decay_corr_fctr        , buf, &i);
  bufWrite_i(header->processing_code        , buf, &i);
  bufWrite_u(header->gate_duration          , buf, &i);
  bufWrite_i(header->r_wave_offset          , buf, &i);
  bufWrite_i(header->num_accepted_beats     , buf, &i);
  bufWrite_f(header->filter_cutoff_frequency, buf, &i);
  bufWrite_f(header->filter_resolution      , buf, &i);
  bufWrite_f(header->filter_ramp_slope      , buf, &i);
  bufWrite_s(header->filter_order           , buf, &i);
  bufWrite_f(header->filter_scatter_fraction, buf, &i);
  bufWrite_f(header->filter_scatter_slope   , buf, &i);
  bufWrite(header->annotation               , buf, &i, 40);
  bufWrite_f(header->mt_1_1                 , buf, &i);
  bufWrite_f(header->mt_1_2                 , buf, &i);
  bufWrite_f(header->mt_1_3                 , buf, &i);
  bufWrite_f(header->mt_2_1                 , buf, &i);
  bufWrite_f(header->mt_2_2                 , buf, &i);
  bufWrite_f(header->mt_2_3                 , buf, &i);
  bufWrite_f(header->mt_3_1                 , buf, &i);
  bufWrite_f(header->mt_3_2                 , buf, &i);
  bufWrite_f(header->mt_3_3                 , buf, &i);
  bufWrite_f(header->rfilter_cutoff         , buf, &i);
  bufWrite_f(header->rfilter_resolution     , buf, &i);
  bufWrite_s(header->rfilter_code           , buf, &i);
  bufWrite_s(header->rfilter_order          , buf, &i);
  bufWrite_f(header->zfilter_cutoff         , buf, &i);
  bufWrite_f(header->zfilter_resolution     , buf, &i);
  bufWrite_s(header->zfilter_code           , buf, &i);
  bufWrite_s(header->zfilter_order          , buf, &i);
  bufWrite_f(header->mt_1_4                 , buf, &i);
  bufWrite_f(header->mt_2_4                 , buf, &i);
  bufWrite_f(header->mt_3_4                 , buf, &i);
  bufWrite_s(header->scatter_type           , buf, &i);
  bufWrite_s(header->recon_type             , buf, &i);
  bufWrite_s(header->recon_views            , buf, &i);
  bufWrite_f(header->prompt_rate            , buf, &i);
  bufWrite_f(header->random_rate            , buf, &i);
  bufWrite_f(header->singles_rate           , buf, &i);
  bufWrite_f(header->scatter_fraction       , buf, &i);
  return 1;
}

int mat_write_image_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,    Image_subheader *header) {
  char            buf[ MatBLKSIZE];

  map_image_header(buf, header);
  return mat_wblk(t_file_ptr, blknum, buf, 1);
}

int map_scan_header(char *buf, Scan_subheader *header) {
  int i = 0, j = 0;
  bufWrite_s(header->data_type          , buf, &i);
  bufWrite_s(header->num_dimensions     , buf, &i);
  bufWrite_s(header->num_r_elements     , buf, &i);
  bufWrite_s(header->num_angles         , buf, &i);
  bufWrite_s(header->corrections_applied, buf, &i);
  bufWrite_s(header->num_z_elements     , buf, &i);
  bufWrite_s(header->ring_difference    , buf, &i);
  bufWrite_f(header->x_resolution       , buf, &i);
  bufWrite_f(header->y_resolution       , buf, &i);
  bufWrite_f(header->z_resolution       , buf, &i);
  bufWrite_f(header->w_resolution       , buf, &i);
  i += 6 * sizeof(short);   /* space reserved for future gating info */
  bufWrite_u(header->gate_duration     , buf, &i);
  bufWrite_i(header->r_wave_offset     , buf, &i);
  bufWrite_i(header->num_accepted_beats, buf, &i);
  bufWrite_f(header->scale_factor      , buf, &i);
  bufWrite_s(header->scan_min          , buf, &i);
  bufWrite_s(header->scan_max          , buf, &i);
  bufWrite_i(header->prompts           , buf, &i);
  bufWrite_i(header->delayed           , buf, &i);
  bufWrite_i(header->multiples         , buf, &i);
  bufWrite_i(header->net_trues         , buf, &i);
  for (j = 0; j < 16; j++)
    bufWrite_f(header->cor_singles[j], buf, &i);
  for (j = 0; j < 16; j++)
    bufWrite_f(header->uncor_singles[j], buf, &i);
  bufWrite_f(header->tot_avg_cor         , buf, &i);
  bufWrite_f(header->tot_avg_uncor       , buf, &i);
  bufWrite_i(header->total_coin_rate     , buf, &i);
  bufWrite_u(header->frame_start_time    , buf, &i);
  bufWrite_u(header->frame_duration      , buf, &i);
  bufWrite_f(header->loss_correction_fctr, buf, &i);
  for (j = 0; j < 8; j++)
    bufWrite_s(header->phy_planes[j], buf, &i);
  return 1;
}

int mat_write_scan_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,   Scan_subheader *header) {
  char            buf[ MatBLKSIZE];

  map_scan_header(buf, header);
  return mat_wblk(t_file_ptr, blknum, buf, 1);
}

int map_attn_header(char *buf, Attn_subheader *header) {
  int i = 0, j = 0;
  bufWrite_s(header->data_type                 , buf, &i);
  bufWrite_s(header->num_dimensions            , buf, &i);
  bufWrite_s(header->attenuation_type          , buf, &i);
  bufWrite_s(header->num_r_elements            , buf, &i);
  bufWrite_s(header->num_angles                , buf, &i);
  bufWrite_s(header->num_z_elements            , buf, &i);
  bufWrite_s(header->ring_difference           , buf, &i);
  bufWrite_f(header->x_resolution              , buf, &i);
  bufWrite_f(header->y_resolution              , buf, &i);
  bufWrite_f(header->z_resolution              , buf, &i);
  bufWrite_f(header->w_resolution              , buf, &i);
  bufWrite_f(header->scale_factor              , buf, &i);
  bufWrite_f(header->x_offset                  , buf, &i);
  bufWrite_f(header->y_offset                  , buf, &i);
  bufWrite_f(header->x_radius                  , buf, &i);
  bufWrite_f(header->y_radius                  , buf, &i);
  bufWrite_f(header->tilt_angle                , buf, &i);
  bufWrite_f(header->attenuation_coeff         , buf, &i);
  bufWrite_f(header->attenuation_min           , buf, &i);
  bufWrite_f(header->attenuation_max           , buf, &i);
  bufWrite_f(header->skull_thickness           , buf, &i);
  bufWrite_s(header->num_additional_atten_coeff, buf, &i);
  for (j = 0; j < 8; j++)
    bufWrite_f(header->additional_atten_coeff[j], buf, &i);
  bufWrite_f(header->edge_finding_threshold, buf, &i);
  bufWrite_s(header->storage_order, buf, &i);
  bufWrite_s(header->span, buf, &i);
  for (j = 0; j < 64; j++)
    bufWrite_s(header->z_elements[j], buf, &i);

  return 1;
}

 int mat_write_attn_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,   Attn_subheader *header) {
  char            buf[ MatBLKSIZE];

  map_attn_header(buf, header);
  return mat_wblk(t_file_ptr, blknum, buf, 1);
}

int map_norm_header(char *buf, Norm_subheader *header) {
  int i = 0, j = 0;
  bufWrite_s(header->data_type               , buf, &i);
  bufWrite_s(header->num_dimensions          , buf, &i);
  bufWrite_s(header->num_r_elements          , buf, &i);
  bufWrite_s(header->num_angles              , buf, &i);
  bufWrite_s(header->num_z_elements          , buf, &i);
  bufWrite_s(header->ring_difference         , buf, &i);
  bufWrite_f(header->scale_factor            , buf, &i);
  bufWrite_f(header->norm_min                , buf, &i);
  bufWrite_f(header->norm_max                , buf, &i);
  bufWrite_f(header->fov_source_width        , buf, &i);
  bufWrite_f(header->norm_quality_factor     , buf, &i);
  bufWrite_s(header->norm_quality_factor_code, buf, &i);
  bufWrite_s(header->storage_order           , buf, &i);
  bufWrite_s(header->span                    , buf, &i);
  for (j = 0; j < 64; j++)
    bufWrite_s(header->z_elements[j], buf, &i);
  return 1;
}

int mat_write_norm_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,    Norm_subheader *header) {
  char            buf[ MatBLKSIZE];

  map_norm_header(buf, header);
  return mat_wblk(t_file_ptr, blknum, buf, 1);
}

int mat_write_Scan3D_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,     Scan3D_subheader *header) {
  char buf[2 * MatBLKSIZE];
  map_Scan3D_header(buf, header);
  return mat_wblk( t_file_ptr, blknum, buf, 2);
}

int unmap_attn_header(char *buf, Attn_subheader *header)  {
  int i = 0, j = 0;
  bufRead_s(&header->data_type                 , buf, &i);
  bufRead_s(&header->num_dimensions            , buf, &i);
  bufRead_s(&header->attenuation_type          , buf, &i);
  bufRead_s(&header->num_r_elements            , buf, &i);
  bufRead_s(&header->num_angles                , buf, &i);
  bufRead_s(&header->num_z_elements            , buf, &i);
  bufRead_s(&header->ring_difference           , buf, &i);
  bufRead_f(&header->x_resolution              , buf, &i);
  bufRead_f(&header->y_resolution              , buf, &i);
  bufRead_f(&header->z_resolution              , buf, &i);
  bufRead_f(&header->w_resolution              , buf, &i);
  bufRead_f(&header->scale_factor              , buf, &i);
  bufRead_f(&header->x_offset                  , buf, &i);
  bufRead_f(&header->y_offset                  , buf, &i);
  bufRead_f(&header->x_radius                  , buf, &i);
  bufRead_f(&header->y_radius                  , buf, &i);
  bufRead_f(&header->tilt_angle                , buf, &i);
  bufRead_f(&header->attenuation_coeff         , buf, &i);
  bufRead_f(&header->attenuation_min           , buf, &i);
  bufRead_f(&header->attenuation_max           , buf, &i);
  bufRead_f(&header->skull_thickness           , buf, &i);
  bufRead_s(&header->num_additional_atten_coeff, buf, &i);
  for (j = 0; j < 8; j++)
    bufRead_f(&header->additional_atten_coeff[j], buf, &i);
  bufRead_f(&header->edge_finding_threshold, buf, &i);
  bufRead_s(&header->storage_order, buf, &i);
  bufRead_s(&header->span, buf, &i);
  for (j = 0; j < 64; j++)
    bufRead_s(&header->z_elements[j], buf, &i);

  return 0;
}

int mat_read_attn_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,   Attn_subheader *header) {
  char            buf[ MatBLKSIZE];

  if (mat_rblk(t_file_ptr, blknum, buf, 1) == ecat_matrix::ECATX_ERROR)
    return (ecat_matrix::ECATX_ERROR);
  return unmap_attn_header(buf, header);
}

int unmap_norm_header(char *buf, Norm_subheader *header) {
  int i = 0, j = 0;
  bufRead_s(&header->data_type               , buf, &i);
  bufRead_s(&header->num_dimensions          , buf, &i);
  bufRead_s(&header->num_r_elements          , buf, &i);
  bufRead_s(&header->num_angles              , buf, &i);
  bufRead_s(&header->num_z_elements          , buf, &i);
  bufRead_s(&header->ring_difference         , buf, &i);
  bufRead_f(&header->scale_factor            , buf, &i);
  bufRead_f(&header->norm_min                , buf, &i);
  bufRead_f(&header->norm_max                , buf, &i);
  bufRead_f(&header->fov_source_width        , buf, &i);
  bufRead_f(&header->norm_quality_factor     , buf, &i);
  bufRead_s(&header->norm_quality_factor_code, buf, &i);
  bufRead_s(&header->storage_order           , buf, &i);
  bufRead_s(&header->span                    , buf, &i);
  for (j = 0; j < 64; j++)
    bufRead_s(&header->z_elements[j], buf, &i);
  return 0;
}

int unmap_norm3d_header(char *buf, Norm3D_subheader *header) {
  int i = 0, j = 0;
  memset(header, 0, sizeof(Norm3D_subheader));
  bufRead_s(&header->data_type               , buf, &i);
  bufRead_s(&header->num_r_elements          , buf, &i);
  bufRead_s(&header->num_transaxial_crystals , buf, &i);
  bufRead_s(&header->num_crystal_rings       , buf, &i);
  bufRead_s(&header->crystals_per_ring       , buf, &i);
  bufRead_s(&header->num_geo_corr_planes     , buf, &i);
  bufRead_s(&header->uld                     , buf, &i);
  bufRead_s(&header->lld                     , buf, &i);
  bufRead_s(&header->scatter_energy          , buf, &i);
  bufRead_s(&header->norm_quality_factor_code, buf, &i);
  bufRead_f(&header->norm_quality_factor     , buf, &i);
  for (j = 0; j < 32; j++)
    bufRead_f(&header->ring_dtcor1[j], buf, &i);
  for (j = 0; j < 32; j++)
    bufRead_f(&header->ring_dtcor2[j], buf, &i);
  for (j = 0; j < 8; j++)
    bufRead_f(&header->crystal_dtcor[j], buf, &i);
  bufRead_s(&header->span, buf, &i);
  bufRead_s(&header->max_ring_diff, buf, &i);
  return 0;
}

int
mat_read_norm_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,  Norm_subheader *header) {
  char            buf[ MatBLKSIZE];
  if (mat_rblk(t_file_ptr, blknum, buf, 1) == ecat_matrix::ECATX_ERROR)
    return (ecat_matrix::ECATX_ERROR);
  return unmap_norm_header(buf, header);
}

int mat_read_norm3d_subheader(FILE *t_file_ptr, Main_header *mhptr, int blknum,     Norm3D_subheader *header) {
  char buf[ MatBLKSIZE];
  if ( mat_rblk( t_file_ptr, blknum, buf, 1) == ecat_matrix::ECATX_ERROR ) return ( ecat_matrix::ECATX_ERROR );
  return unmap_norm3d_header(buf, header);
  return 0;
}

int matspec(const char *str, char *fname, int *matnum) {
  char           *cp;
  int             mf = 0, mp = 1, mg = 1, ms = 0, mb = 0;

  strcpy(fname, str);
  cp = (char *) strchr(fname, ',');
  if (cp) {
    *cp++ = '\0';
    sscanf(cp, "%d,%d,%d,%d,%d", &mf, &mp, &mg, &ms, &mb);
    *matnum = mat_numcod(mf, mp, mg, ms, mb);
    return 1;
  }
  return 0;
}

} // namespace ecat_matrix