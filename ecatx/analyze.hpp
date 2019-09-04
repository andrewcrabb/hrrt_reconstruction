/*
 *
 * (c) Copyright, 1986-1994
 * Biomedical Imaging Resource
 * Mayo Foundation
 *
 * dbh.h
 *
 *
 * database sub-definitions
 *
 * 20-Nov-1995: Renamed by Sibomana@topo.ucl.ac.be to analyze.h
 */

#pragma once

class Analyze {

struct header_key    {                                      /* off + size*/
  int sizeof_hdr;                   /* 0 + 4     */
  char data_type[10];               /* 4 + 10    */
  char db_name[18];                 /* 14 + 18   */
  int extents;                      /* 32 + 4    */
  short int session_error;          /* 36 + 2    */
  char regular;                     /* 38 + 1    */
  char hkey_un0;                    /* 39 + 1    */
};                  /* total=40  */

struct image_dimension    {                                   /* off + size*/
  short int dim[8];               /* 0 + 16    */
  char vox_units[4];      /* 16 + 4    */
  char cal_units[8];      /* 20 + 4    */
  short int unused1;      /* 24 + 2    */
  short int datatype;     /* 30 + 2    */
  short int bitpix;                     /* 32 + 2    */
  short int dim_un0;                      /* 34 + 2    */
  float pixdim[8];                        /* 36 + 32   */
  // pixdim[]: voxel dimensions: [1] width [2] height [3] interslice 
  float vox_offset;                     /* 68 + 4    */
  float funused1;                         /* 72 + 4    */
  float funused2;                       /* 76 + 4    */
  float funused3;                       /* 80 + 4    */
  float cal_max;                      /* 84 + 4    */
  float cal_min;                      /* 88 + 4    */
  int compressed;                       /* 92 + 4    */
  int verified;                       /* 96 + 4    */
  int glmax, glmin;                   /* 100 + 8   */
};                  /* total=108 */

struct data_history    {                                      /* off + size*/
  char descrip[80];                 /* 0 + 80    */
  char aux_file[24];                /* 80 + 24   */
  char orient;                      /* 104 + 1   */
  char originator[10];              /* 105 + 10  */
  char generated[10];               /* 115 + 10  */
  char scannum[10];                 /* 125 + 10  */
  char patient_id[10];              /* 135 + 10  */
  char exp_date[10];                /* 145 + 10  */
  char exp_time[10];                /* 155 + 10  */
  char hist_un0[3];                 /* 165 + 3   */
  int views;                        /* 168 + 4   */
  int vols_added;                   /* 172 + 4   */
  int start_field;                  /* 176 + 4   */
  int field_skip;                   /* 180 + 4   */
  int omax, omin;                   /* 184 + 8   */
  int smax, smin;                   /* 192 + 8   */
};                          /* total=200 */

struct dsr {                        /* off + size*/
  struct header_key hk;             /* 0 + 40    */
  struct image_dimension dime;      /* 40 + 108  */
  struct data_history hist;         /* 148 + 200 */
};                          /* total=348 */

// Acceptable values for hdr.dime.datatype
// These used to have DT_ prefix

  enum class DataKey {
    NONE          =   0,
    UNKNOWN       =   0,
    BINARY        =   1,
    UNSIGNED_CHAR =   2,
    SIGNED_SHORT  =   4,
    SIGNED_INT    =   8,
    FLOAT         =  16,
    COMPLEX       =  32,
    DOUBLE        =  64,
    RGB           = 128,
    ALL           = 255
  };

  struct DataType {
    int bytes_per_pixel;
    std::string description;
  };

// xxx prefixes are the ones not mentioned in code

  static std::map<DataKey, DataType> data_types_ = {
    {DataKey::NONE         , {0, "xxxnone"}},
    {DataKey::UNKNOWN      , {0, "xxxunknown"}},
    {DataKey::BINARY       , {0, "xxxbinary"}},
    {DataKey::UNSIGNED_CHAR, {1, "unsigned integer"}},
    {DataKey::SIGNED_SHORT , {2, "signed short"}},
    {DataKey::SIGNED_INT   , {4, "signed integer"}},
    {DataKey::FLOAT        , {4, "float"}},
    {DataKey::COMPLEX      , {0, "xxxcomplex"}},
    {DataKey::DOUBLE       , {0, "xxxdouble"}},
    {DataKey::RGB          , {0, "xxxrgb"}},
    {DataKey::ALL          , {0, "xxxall"}},
  };

  typedef struct  {
    float real;
    float imag;
  } COMPLEX;

// ---------- Members ----------

struct dsr header_;
int analyze_orientation;   /*  0: Neurology convention (spm), 1: Radiology convention (AIR) */
}  // class Analyze
