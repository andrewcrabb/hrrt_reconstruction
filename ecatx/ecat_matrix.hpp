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

# pragma once

/*  19-sep-2002:
 * Merge with bug fixes and support for CYGWIN provided by Kris Thielemans@csc.mrc.ac.uk
 *  21-apr-2009: Use same file open modes for all OS (win32,linux,...)
 * 21-sep-2010: Added data rates to image header (Merence sibomana)
 */

// #include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <vector>
#include <string>
#include <map>

// #define R_MODE "rb"
// #define RW_MODE "rb+"
// #define W_MODE "wb+"

// The Great Rewrite of 2018-19

namespace ecat_matrix {

constexpr int MatBLKSIZE     = 512;
constexpr int MatFirstDirBlk = 2;
constexpr int MatMagic       = 0x67452301;
constexpr int V7             = 70;
constexpr int MagicNumLen    = 14;
constexpr int NameLen        = 32;
constexpr int IDLen          = 16;
// constexpr int NumDataMasks   = 12;
constexpr int MAX_FRAMES     = 512;
constexpr int MAX_SLICES     = 1024;
constexpr int MAX_GATES      = 32;
constexpr int MAX_BED_POS    = 32;
constexpr int ECATX_ERROR    = -1;
constexpr int ECATX_OK       = 0;


enum class DataSetType {
  NoData,
  Sinogram,
  PetImage,
  AttenCor,
  Normalization,
  PolarMap,
  ByteVolume,
  PetVolume,
  ByteProjection,
  PetProjection,
  ByteImage,
  Short3dSinogram,
  Byte3dSinogram,
  Norm3d,
  Float3dSinogram,
  InterfileImage,
  NumDataSetTypes
};

struct CodeName {
  std::string code;  // "s"
  std::string name;  // "Sinogram"
};

// Combines former enum DataSetType, char *datasettype, char *dstypecode

static std::map<DataSetType, CodeName> data_set_types_ = {
  {DataSetType::NoData         , {"u" , "Unknown"}},
  {DataSetType::Sinogram       , {"s" , "Sinogram"}},
  {DataSetType::PetImage       , {"i" , "Image-16"}},
  {DataSetType::AttenCor       , {"a" , "Attenuation Correction"}},
  {DataSetType::Normalization  , {"n" , "Normalization"}},
  {DataSetType::PolarMap       , {"pm", "Polar Map"}},
  {DataSetType::ByteVolume     , {"v8", "Volume-8 byte"}},
  {DataSetType::PetVolume      , {"v" , "Volume-16 pet"}},
  {DataSetType::ByteProjection , {"p8", "Projection-8 byte"}},
  {DataSetType::PetProjection  , {"p" , "Projection-16 pet"}},
  {DataSetType::ByteImage      , {"i8", "Image-8 byte"}},
  {DataSetType::Short3dSinogram, {"S" , "3D Sinogram-16 short"}},
  {DataSetType::Byte3dSinogram , {"S8", "3D Sinogram-8 byte"}},
  {DataSetType::Norm3d         , {"N" , "3D Normalization"}},
  {DataSetType::Float3dSinogram, {"FS", "Float3dSinogram"}},
  {DataSetType::InterfileImage , {"I" , "Interfile"}}  // NB the code was missing in the original source code array.
};

enum class MatrixDataType {
  UnknownMatDataType,
  ByteData,
  VAX_Ix2,
  VAX_Ix4,
  VAX_Rx4,
  IeeeFloat,
  SunShort,
  SunLong,
  UShort_BE,
  UShort_LE,
  Color_24,
  Color_8,
  BitData,
  NumMatrixDataTypes,
 MAT_SUB_HEADER = 255  // operation to sub-header only
};

// constexpr int MAT_SUB_HEADER = 255;  // operation to sub-header only
// constexpr MatrixDataType MAT_SUB_HEADER = 255;  // operation to sub-header only

// Combines former MatrixDataType, 

static std::map<MatrixDataType, std::string> matrix_data_types_ = {
  {MatrixDataType::UnknownMatDataType, "UnknownMatDataType"},
  {MatrixDataType::ByteData          , "ByteData"},
  {MatrixDataType::VAX_Ix2           , "VAX_Ix2"},
  {MatrixDataType::VAX_Ix4           , "VAX_Ix4,"},
  {MatrixDataType::VAX_Rx4           , "VAX_Rx4"},
  {MatrixDataType::IeeeFloat         , "IeeeFloat"},
  {MatrixDataType::SunShort          , "SunShort"},
  {MatrixDataType::SunLong           , "SunLong"},
  {MatrixDataType::NumMatrixDataTypes, "NumMatrixDataTypes,"},
  {MatrixDataType::UShort_BE         , "UShort_BE"},
  {MatrixDataType::UShort_LE         , "UShort_LE"},
  {MatrixDataType::Color_24          , "Color_24"},
  {MatrixDataType::Color_8           , "Color_8"},
  {MatrixDataType::BitData           , "BitData"}
};

// Combines former enum CalibrationStatus, char *calstatus[], char *ecfunits[]

enum class CalibrationStatus {
  Uncalibrated, 
  Calibrated,
  Processed
  // NumCalibrationStatus
};

struct CalibrationStruct {
  std::string status;  // Former calstatus
  std::string units;   // Former ecfunits
};

static std::map<CalibrationStatus, CalibrationStruct> calibration_status_ = {
  {CalibrationStatus::Uncalibrated, {"Uncalibrated", "ECAT Counts/Sec"}},
  {CalibrationStatus::Calibrated  , {"Calibrated"  , "Bq/ml"}},
  {CalibrationStatus::Processed   , {"Processed"   , "Processed"}}
};

enum class ScanType {
  UndefScan, BlankScan,
  TransmissionScan, StaticEmission,
  DynamicEmission, GatedEmission,
  TransRectilinear, EmissionRectilinear,
  NumScanTypes
};

static std::map<ScanType, std::array<std::string, 2>> scan_types_ = {
  {ScanType::UndefScan          , {"na", "Not Applicable"}},
  {ScanType::BlankScan          , {"bl", "Blank Scan"}},
  {ScanType::TransmissionScan   , {"tx", "Transmission Scan"}},
  {ScanType::StaticEmission     , {"se", "Static Emission"}},
  {ScanType::DynamicEmission    , {"de", "Dynamic Emission"}},
  {ScanType::GatedEmission      , {"ge", "Gated Emission,"}},
  {ScanType::TransRectilinear   , {"tr", "Transmission Rectilinear"}},
  {ScanType::EmissionRectilinear, {"er", "Emission Rectilinear"}}
};

static std::vector<std::string> typeFilterLabel = {
  "Sinogram", "Attenuation Correction", "Normalization", "Polar Map", "Image", 
  "Volume", "Projection", "3D Sinogram", "Report", "Graph", "ROI", "3D Normalization"
    };


enum class CurrentModels {
  E921,
  E961,
  E953,
  E953B,
  E951,
  E951R,
  E962,
  E925,
  NumEcatModels
};

// Combines former float[] ecfconverter, enum OldDisplayUnits (max NumOldUnits)

enum class OldDisplayUnits {
  TotalCounts,
  UnknownEcfUnits,
  EcatCountsPerSec,
  UCiPerCC,
  LMRGlu,
  LMRGluUmol,
  LMRGluMg,
  NCiPerCC,
  WellCounts,
  BecquerelsPerCC,
  MlPerMinPer100g,
  MlPerMinPer1g,
  NumOldUnits
};

// Values from ecfconverter

static std::map<std::string, float> display_units_ = {
  {"none"              , 0.0},
  {"ECAT Counts/Sec"   , 1.0},
  {"Bq/ml"             , 1.0},
  {"Processed"         , 1.0},
  {"microCi/ml"        , 3.7e4},
  {"micromole/100g/min", 1.0},
  {"mg/100g/min"       , 1.0},
  {"ml/100g/min"       , 1.0},
  {"SUR"               , 37.0},
  {"ml/g"              , 1.0},
  {"1/min"             , 1.0},
  {"pmole/ml"          , 1.0},
  {"nM"                , 1.0}
};

static std::vector<std::string> customDisplayUnits = {
  "none",
  "ECAT Counts/Sec",
  "Bq/ml",
  "Processed",
  "microCi/ml",
  "micromole/100g/min",
  "mg/100g/min",
  "ml/100g/min",
  "SUR",
  "ml/g",
  "1/min",
  "pmole/ml",
  "nM"
};

enum class PatientOrientation {
  FeetFirstProne, HeadFirstProne,
  FeetFirstSupine, HeadFirstSupine,
  FeetFirstRight , HeadFirstRight,
  FeetFirstLeft  , HeadFirstLeft,
  UnknownOrientation, NumOrientations
};

enum class SeptaPos {
  SeptaExtended, SeptaRetracted, NoSeptaInstalled
};

enum class Sino3dOrder {
  ElAxVwRd, ElVwAxRd, NumSino3dOrders
};

enum class MatrixError {
  OK,
  READ_ERROR,
  WRITE_ERROR,
  INVALID_DIRBLK,
  ACS_FILE_NOT_FOUND,
  INTERFILE_OPEN_ERR,
  FILE_TYPE_NOT_MATCH,
  READ_FROM_NILFPTR,
  NOMHD_FILE_OBJECT,
  NIL_SHPTR,
  NIL_DATA_PTR,
  MATRIX_NOT_FOUND,
  UNKNOWN_FILE_TYPE,
  ACS_CREATE_ERR,
  BAD_ATTRIBUTE,
  BAD_FILE_ACCESS_MODE,
  INVALID_DIMENSION,
  NO_SLICES_FOUND,
  INVALID_DATA_TYPE,
  INVALID_MBED_POSITION
};

// Former matrix_errors in matrix_extra

static std::map<MatrixError, std::string> matrix_errors_ = {
  {MatrixError::OK                   , "No Error"},
  {MatrixError::READ_ERROR           , "Read error"},
  {MatrixError::WRITE_ERROR          , "Write error"},
  {MatrixError::INVALID_DIRBLK       , "Invalid directory block"},
  {MatrixError::ACS_FILE_NOT_FOUND   , "ACS file not found"},
  {MatrixError::INTERFILE_OPEN_ERR   , "Interfile open error"},
  {MatrixError::FILE_TYPE_NOT_MATCH  , "File type not match"},
  {MatrixError::READ_FROM_NILFPTR    , "Read from nil filepointer"},
  {MatrixError::NOMHD_FILE_OBJECT    , "No mainheader file object"},
  {MatrixError::NIL_SHPTR            , "Nil subheader pointer"},
  {MatrixError::NIL_DATA_PTR         , "Nil data pointer"},
  {MatrixError::MATRIX_NOT_FOUND     , "Matrix not found"},
  {MatrixError::UNKNOWN_FILE_TYPE    , "Unknown filetype"},
  {MatrixError::ACS_CREATE_ERR       , "ACS create error"},
  {MatrixError::BAD_ATTRIBUTE        , "Bad attribute"},
  {MatrixError::BAD_FILE_ACCESS_MODE , "Bad file access mode"},
  {MatrixError::INVALID_DIMENSION    , "Invalid dimension"},
  {MatrixError::NO_SLICES_FOUND      , "No slices found"},
  {MatrixError::INVALID_DATA_TYPE    , "Invalid data type"},
  {MatrixError::INVALID_MBED_POSITION, "Invalid multibed position"}
};

// TODO get the correct enum values above OnComProc from ecat7_scan3d.cpp which specifies 15 values for sh.corrections_applied where sh is a global (terrible name)

enum class ProcessingCode {
  NotProc    = 0,
  Norm       = 1,
  Atten_Meas = 2,
  Atten_Calc = 4,
  Smooth_X   = 8,
  Smooth_Y   = 16,
  Smooth_Z   = 32,
  Scat2d     = 64,
  Scat3d     = 128,
  ArcPrc     = 256,
  DecayPrc   = 512,
  OnComPrc   = 1024,
  FILLMEIN1  = 2048,
  FILLMEIN2  = 4096,
  FILLMEIN3  = 8192,
  FILLMEIN4  = 16384
};

std::map<ProcessingCode, std::string> applied_proc_ = {
  {ProcessingCode::NotProc    , "None"},
  {ProcessingCode::Norm       , "Normalized",},
  {ProcessingCode::Atten_Meas , "Measured-Attenuation-Correction",},
  {ProcessingCode::Atten_Calc , "Calculated-Attenuation-Correction"},
  {ProcessingCode::Smooth_X   , "X-smoothing",},
  {ProcessingCode::Smooth_Y   , "Y-smoothing",},
  {ProcessingCode::Smooth_Z   , "Z-smoothing"},
  {ProcessingCode::Scat2d     , "2D-scatter-correction",},
  {ProcessingCode::Scat3d     , "3D-scatter-correction",},
  {ProcessingCode::ArcPrc     , "Arc-correction"},
  {ProcessingCode::DecayPrc   , "Decay-correction",},
  {ProcessingCode::OnComPrc   , "Online-compression",},
  {ProcessingCode::FILLMEIN1  , "FORE"},
  {ProcessingCode::FILLMEIN2  , "SSRB",},
  {ProcessingCode::FILLMEIN3  , "Seg0",},
  {ProcessingCode::FILLMEIN4  , "Randoms Smoothing"}
};

// ecat 6.4 compatibility definitions
// matrix data types

enum class MatrixDataType_64 {
  GENERIC,
  BYTE_TYPE,
  VAX_I2,
  VAX_I4,
  VAX_R4,
  SUN_R4,
  SUN_I2,
  SUN_I4
};

// matrix file types

enum class MatrixFileType_64 {
  UNKNOWN_FTYPE,
  SCAN_DATA,
  IMAGE_DATA,
  ATTN_DATA,
  NORM_DATA
};

// end of ecat 6.4 definitions

struct MatDir {
  int matnum;
  int strtblk;
  int endblk;
  int matstat;
};

struct matdir {
  int nmats;
  int nmax;
  MatDir *entry;
};

struct MatVal {
  int frame, plane, gate, data, bed;
};

struct Matlimits {
  int framestart, frameend;
  int planestart, planeend;
  int gatestart , gateend;
  int datastart , dataend;
  int bedstart  , bedend;
};

struct MatDirNode {
  int   matnum;
  int   strtblk;
  int   endblk;
  int   matstat;
  struct matdirnode *next;
};

struct MatDirList {
  int nmats ;
  MatDirNode *first ;
  MatDirNode *last ;
};

struct MatDirBlk {
  int nfree, nextblk, prvblk, nused ;
  MatDir matdir[31] ;
};

// matrix file access modes

enum class MatrixFileAccessMode {
  READ_WRITE,
  READ_ONLY,
  CREATE,
  OPEN_EXISTING,
  CREATE_NEW_FILE
};

// object creation attributes
// None of these are used anywhere.  All had MAT prefix.

enum class MatrixObjectAttribute {
  ATTR_NULL,  // Formerly 'NULL'
  XDIM,
  YDIM,
  ZDIM,
  DATA_TYPE,
  SCALE_FACTOR,
  PIXEL_SIZE,
  Y_SIZE,
  Z_SIZE,
  DATA_MAX,
  DATA_MIN,
  PROTO
};

struct Main_header {
  char magic_number[14];
  char original_file_name[32];
  short sw_version;
  short system_type;
  // short file_type;
  DataSetType file_type;   // ahc this indexes into data_set_types_
  char serial_number[10];
  short align_0;            /* 4 byte alignment purpose */
  unsigned int scan_start_time;
  char isotope_code[8];
  float isotope_halflife;
  char radiopharmaceutical[32];
  float gantry_tilt;
  float gantry_rotation;
  float bed_elevation;
  float intrinsic_tilt;
  short wobble_speed;
  short transm_source_type;  // Currently const int in ecat7_global, make it a namespaced scoped enum
  float distance_scanned;
  float transaxial_fov;
  short angular_compression;
  short coin_samp_mode;
  short axial_samp_mode;
  short align_1;
  float calibration_factor;
  CalibrationStatus calibration_units;
  short calibration_units_label;
  short compression_code;
  char study_name[12];
  char patient_id[16];
  char patient_name[32];
  char patient_sex[1];
  char patient_dexterity[1];
  float patient_age;
  float patient_height;
  float patient_weight;
  int patient_birth_date;
  char physician_name[32];
  char operator_name[32];
  char study_description[32];
  short acquisition_type;  // Currently const int in ecat7_global, make it a namespaced scoped enum
  short patient_orientation;  // Currently const int in ecat7_global, make it a namespaced scoped enum
  char facility_name[20];
  short num_planes;
  short num_frames;
  short num_gates;
  short num_bed_pos;
  float init_bed_position;
  float bed_offset[15];
  float plane_separation;
  short lwr_sctr_thres;
  short lwr_true_thres;
  short upr_true_thres;
  char user_process_code[10];
  short acquisition_mode;  // Currently const int in ecat7_global, make it a namespaced scoped enum
  short align_2;
  float bin_size;
  float branching_fraction;
  unsigned int dose_start_time;
  float dosage;
  float well_counter_factor;
  char data_units[32];
  short septa_state;  // Currently const int in ecat7_global, make it a namespaced scoped enum
  short align_3;
};

struct Scan_subheader {
  MatrixDataType data_type;
  short num_dimensions;
  short num_r_elements;
  short num_angles;
  short corrections_applied;
  short num_z_elements;
  short ring_difference;
  short align_0;
  float x_resolution;
  float y_resolution;
  float z_resolution;
  float w_resolution;
  unsigned int gate_duration;
  int r_wave_offset;
  int num_accepted_beats;
  float scale_factor;
  short scan_min;
  short scan_max;
  int prompts;
  int delayed;
  int multiples;
  int net_trues;
  float cor_singles[16];
  float uncor_singles[16];
  float tot_avg_cor;
  float tot_avg_uncor;
  int total_coin_rate;
  unsigned int frame_start_time;
  unsigned int frame_duration;
  float loss_correction_fctr;
  short phy_planes[8];
};

struct Scan3D_subheader {
  MatrixDataType data_type;
  short num_dimensions;
  short num_r_elements;
  short num_angles;
  short corrections_applied;
  short num_z_elements[64];
  short ring_difference;
  short storage_order;    // Currently const int in ecat7_global, make it a namespaced scoped enum
  short axial_compression;
  float x_resolution;
  float v_resolution;
  float z_resolution;
  float w_resolution;
  unsigned int gate_duration;
  int r_wave_offset;
  int num_accepted_beats;
  float scale_factor;
  short scan_min;
  short scan_max;
  int prompts;
  int delayed;
  int multiples;
  int net_trues;
  float tot_avg_cor;
  float tot_avg_uncor;
  int total_coin_rate;
  unsigned int frame_start_time;
  unsigned int frame_duration;
  float loss_correction_fctr;
  float uncor_singles[128];
  int num_extended_uncor_singles;
  float *extended_uncor_singles;
};

struct Image_subheader {
  MatrixDataType data_type;
  short num_dimensions;
  short x_dimension;
  short y_dimension;
  short z_dimension;
  short align_0;
  float z_offset;
  float x_offset;
  float y_offset;
  float recon_zoom;
  float scale_factor;
  short image_min;
  short image_max;
  float x_pixel_size;
  float y_pixel_size;
  float z_pixel_size;
  unsigned int frame_duration;
  unsigned int frame_start_time;
  short filter_code;  // Currently const int in ecat7_global, make it a namespaced scoped enum
  short align_1;
  float x_resolution;
  float y_resolution;
  float z_resolution;
  float num_r_elements;
  float num_angles;
  float z_rotation_angle;
  float decay_corr_fctr;
  int processing_code;    // Currently const int in ecat7_global, make it a namespaced scoped enum
  unsigned int gate_duration;
  int r_wave_offset;
  int num_accepted_beats;
  float filter_cutoff_frequency;
  float filter_resolution;
  float filter_ramp_slope;
  short filter_order;
  short align_2;
  float filter_scatter_fraction;
  float filter_scatter_slope;
  char annotation[40];
  float mt_1_1;
  float mt_1_2;
  float mt_1_3;
  float mt_2_1;
  float mt_2_2;
  float mt_2_3;
  float mt_3_1;
  float mt_3_2;
  float mt_3_3;
  float rfilter_cutoff;
  float rfilter_resolution;
  short rfilter_code;  
  short rfilter_order;
  float zfilter_cutoff;
  float zfilter_resolution;
  short zfilter_code;
  short zfilter_order;
  float mt_1_4;
  float mt_2_4;
  float mt_3_4;
  short scatter_type;    // Currently const int in ecat7_global, make it a namespaced scoped enum
  short recon_type;  // Currently const int in ecat7_global, make it a namespaced scoped enum
  short recon_views;
  short align_3;
  float prompt_rate; /* total_prompt = prompt_rate*frame_duration */
  float random_rate; /* total_random = random_rate*frame_duration */
  float singles_rate; /* average bucket singles rate */
  float scatter_fraction;
};

struct Norm_subheader {
  MatrixDataType data_type;
  short num_dimensions;
  short num_r_elements;
  short num_angles;
  short num_z_elements;
  short ring_difference;
  float scale_factor;
  float norm_min;
  float norm_max;
  float fov_source_width;
  float norm_quality_factor;
  short norm_quality_factor_code;
  short storage_order;
  short span;
  short z_elements[64];
  short align_0;
};

struct Norm3D_subheader {
  MatrixDataType data_type;
  short num_r_elements;
  short num_transaxial_crystals;
  short num_crystal_rings;
  short crystals_per_ring;
  short num_geo_corr_planes;
  short uld;  // upper level energy discriminator
  short lld;  // lower level energy discriminator
  short scatter_energy;
  short norm_quality_factor_code;
  float norm_quality_factor;
  float ring_dtcor1[32];
  float ring_dtcor2[32];
  float crystal_dtcor[8];
  short span;
  short max_ring_diff;
};

struct Attn_subheader {
  MatrixDataType data_type;
  short num_dimensions;
  short attenuation_type;    // Currently const int in ecat7_global, make it a namespaced scoped enum
  short num_r_elements;
  short num_angles;
  short num_z_elements;
  short ring_difference;
  short align_0;
  float x_resolution;
  float y_resolution;
  float z_resolution;
  float w_resolution;
  float scale_factor;
  float x_offset;
  float y_offset;
  float x_radius;
  float y_radius;
  float tilt_angle;
  float attenuation_coeff;
  float attenuation_min;
  float attenuation_max;
  float skull_thickness;
  short num_additional_atten_coeff;
  short align_1;
  float additional_atten_coeff[8];
  float edge_finding_threshold;
  short storage_order;    // Currently const int in ecat7_global, make it a namespaced scoped enum
  short span;
  short z_elements[64];
};

enum class FileFormat {
  ECAT6, ECAT7, Interfile
};

struct MatrixFile {
  char    *fname ;  /* file path */
  Main_header *mhptr ;  /* pointer to main header */
  MatDirList  *dirlist ;  /* directory */
  FILE    *fptr ;   /* file ptr for I/O calls */
  FileFormat file_format;
  char **interfile_header;   // TODO reimplement as vector<interfile::Key, std::filesystem::path> - see analyze.cpp:200
  void *analyze_hdr;
};

struct MatrixData {
  int   matnum ;  /* matrix number */
  MatrixFile  *matfile ;  /* pointer to parent */
  DataSetType mat_type ;  /* type of matrix? */
  MatrixDataType  data_type ; /* type of data */
  // void *   shptr ;    pointer to sub-header
  // void *   data_ptr ;  /* pointer to data */
  void *shptr ;   /* pointer to sub-header */
  void *data_ptr ;  /* pointer to data */
  int   data_size ; /* size of data in bytes */
  int   xdim;   /* dimensions of data */
  int   ydim;   /* y dimension */
  int   zdim;   /* for volumes */
  float   scale_factor ;  /* valid if data is int? */
  float   intercept;      /* valid if data is USHORT_LE or USHORT_BE */
  float   pixel_size; /* xdim data spacing (cm) */
  float   y_size;   /* ydim data spacing (cm) */
  float   z_size;   /* zdim data spacing (cm) */
  float   data_min; /* min value of data */
  float   data_max; /* max value of data */
  float       x_origin;       /* x origin of data */
  float       y_origin;       /* y origin of data */
  float       z_origin;       /* z origin of data */
  void *dicom_header;     /* DICOM header is stored after matrix data */
  int dicom_header_size;
};

// high level user functions

void SWAB(void *from, void *to, int len);
void SWAW(short *from, short *to, int len);
// ahc replace all u_char with unsigned char
unsigned char find_bmin(const unsigned char*, int size);
unsigned char find_bmax(const unsigned char*, int size);
short find_smin(const short*, int size);
short find_smax(const short*, int size);
int find_imin(const int*, int size);
int find_imax(const int*, int size);
float find_fmin(const float*, int size);
float find_fmax(const float*, int size);
int matspec(const char* specs, char* fname , int* matnum);
char* is_analyze(const char* );
MatrixFile* matrix_create(const char*, MatrixFileAccessMode const mode , Main_header*);
MatrixFile* matrix_open(const char*, MatrixFileAccessMode mode, MatrixFileType_64);
int analyze_open(MatrixFile *mptr);
int analyze_read(MatrixFile *mptr, int matnum, MatrixData  *data, MatrixDataType dtype);
// MatrixData *matrix_read(MatrixFile*, int matnum, int type);
MatrixData *matrix_read(MatrixFile*, int matnum, MatrixDataType type);
MatrixData* matrix_read_slice(MatrixFile*, MatrixData* volume, int slice_num, int segment);
MatrixData* matrix_read_view(MatrixFile*, MatrixData* volume, int view, int segment);
void set_matrix_no_error();
int matrix_write(MatrixFile*, int matnum, MatrixData*);
int mat_numcod(int frame, int plane, int gate, int data, int bed);
int mat_numdoc(int, MatVal*);
void free_matrix_data(MatrixData*);
void matrix_perror(const char*);
int matrix_close(MatrixFile*);
int matrix_find(MatrixFile*, int matnum, MatDir*);
// void crash(const char *fmt, ...);
void crash( const char *fmt, char *a0, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6, char *a7, char *a8, char *a9);
MatrixData *load_volume7(MatrixFile *matrix_file, int frame, int gate, int data, int bedstart, int bedend);
int save_volume7( MatrixFile *mfile, Image_subheader *shptr, float *data_ptr, int frame, int gate, int data, int bed );
int read_host_data(MatrixFile *mptr, int matnum, MatrixData *data, MatrixDataType dtype);
int write_host_data(MatrixFile *mptr, int matnum, const MatrixData *data);
int mh_update(MatrixFile *file);
int convert_float_scan( MatrixData *scan, float *fdata);
int matrix_convert_data(MatrixData *matrix, MatrixDataType dtype);
MatrixData *matrix_read_scan(MatrixFile *mptr, int matnum, MatrixDataType dtype, int segment);

 // low level functions prototypes, don't use

int unmap_main_header(char *buf, Main_header *h);
int unmap_Scan3D_header(char *buf, Scan3D_subheader*);
int unmap_scan_header(char *buf, Scan_subheader*);
int unmap_image_header(char *buf, Image_subheader*);
int unmap_attn_header(char *buf, Attn_subheader*);
int unmap_norm3d_header(char *buf, Norm3D_subheader*);
int unmap_norm_header(char *buf, Norm_subheader*);
int mat_read_main_header(FILE *fptr, Main_header *h);
int mat_read_scan_subheader( FILE *, Main_header*, int blknum, Scan_subheader*);
int mat_read_Scan3D_subheader( FILE *, Main_header*, int blknum, Scan3D_subheader*);
int mat_read_image_subheader(FILE *, Main_header*, int blknum, Image_subheader*);
int mat_read_attn_subheader(FILE *, Main_header*, int blknum, Attn_subheader*);
int mat_read_norm_subheader(FILE *, Main_header*, int blknum, Norm_subheader*);
int mat_read_norm3d_subheader(FILE *, Main_header*, int blknum, Norm3D_subheader*);
int mat_write_main_header(FILE *fptr, Main_header *h);
int mat_write_scan_subheader( FILE *, Main_header*, int blknum, Scan_subheader*);
int mat_write_Scan3D_subheader( FILE *, Main_header*, int blknum, Scan3D_subheader*);
int mat_write_image_subheader(FILE *, Main_header*, int blknum, Image_subheader*);
int mat_write_attn_subheader(FILE *, Main_header*, int blknum, Attn_subheader*);
int mat_write_norm_subheader(FILE *, Main_header*, int blknum, Norm_subheader*);
struct matdir *mat_read_dir(FILE *, Main_header*, char *selector);
int mat_read_matrix_data(FILE *, Main_header*, int blk, int nblks, short* bufr);
int mat_enter(FILE *, Main_header *mhptr, int matnum, int nblks);
int mat_lookup(FILE *fptr, Main_header *mhptr, int matnum,  MatDir *entry);
int mat_rblk(FILE *fptr, int blkno, char *bufr,  int nblks);
int mat_wblk(FILE *fptr, int blkno, char *bufr,  int nblks);
void swaw(short *from, short *to, int length);
int insert_mdir(MatDir *matdir, MatDirList *dirlist);

// int numDisplayUnits;
// replaced with matrix_data_types_
// char* datasettype[NumDataSetTypes];
// char* dstypecode[NumDataSetTypes];
// std::vector<DataSetType> data_set_types_;
// Replaced with scan_types_
// char* scantype[NumScanTypes];
// char* scantypecode[NumScanTypes];
// std::vector <std::string> customDisplayUnits;
// char* customDisplayUnits[];
// float ecfconverter[OldDisplayUnits::NumOldUnits];  // Now in display_units_
// char* calstatus[NumCalibrationStatus];
const std::string sexcode;
const std::string dexteritycode;
// std::vector<std::string> typeFilterLabel;
int ecat_default_version;
MatrixError matrix_errno;
// char ecat_matrix::matrix_errtxt[];
std::string matrix_errtxt;

// extern "C" int numDisplayUnits;
// extern "C" char* datasettype[NumDataSetTypes];
// extern "C" char* dstypecode[NumDataSetTypes];
// extern "C" char* scantype[NumScanTypes];
// extern "C" char* scantypecode[NumScanTypes];
// extern "C" char* customDisplayUnits[];
// extern "C" float ecfconverter[NumOldUnits];
// extern "C" char* calstatus[NumCalibrationStatus];
// extern "C" char* sexcode;
// extern "C" char* dexteritycode;
// extern "C" char* typeFilterLabel[NumDataMasks];
// extern "C" int ecat_default_version;
// extern "C" MatrixError matrix_errno;
// extern "C" char ecat_matrix::matrix_errtxt[];

}  // namespace ecat_matrix
