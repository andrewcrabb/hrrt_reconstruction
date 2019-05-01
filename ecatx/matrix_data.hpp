// matrix_data.hpp
// Content taken from monolithic ecat_matrix.hpp
// Global changes: 
//   ecat_matrix::MatrixData         MatrixData
//   ecat_matrix::MatrixDataType     MatrixData::DataType
//   ecat_matrix::MatrixDataType_64  MatrixData::DataType_64
//   ecat_matrix::matrix_data_types_ MatrixData::data_types_
//   ecat_matrix::MatrixDataSetType  MatrixData::DataSetType
//   ecat_matrix::data_set_types_    MatrixData::data_set_types_

#pragma once

#include <map>

// predeclare classes rather than include ecat_matrix.hpp
struct MatrixFile;

class MatrixData {

public:
  // enum class MatrixDataType {
  enum class DataType {
    UnknownMatDataType,
    ByteData,
    VAX_Ix2,  // Can't comment these out as existing code assigns raw ints to this variable.
    VAX_Ix4,  // Can't comment these out as existing code assigns raw ints to this variable.
    VAX_Rx4,  // Can't comment these out as existing code assigns raw ints to this variable.
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

// ecat 6.4 compatibility definitions
// matrix data types

// enum class MatrixDataType_64 {
enum class DataType_64 {
  GENERIC,
  BYTE_TYPE,
  VAX_I2,  // Can't comment these out as existing code assigns raw ints to this variable.
  VAX_I4,  // Can't comment these out as existing code assigns raw ints to this variable.
  VAX_R4,  // Can't comment these out as existing code assigns raw ints to this variable.
  SUN_R4,
  SUN_I2,
  SUN_I4
};

  // struct MatrixDataProperty {
  struct DataProperty {
    std::string name;
    int length;
  };

// Combines former MatrixDataType,

  // static std::map<MatrixDataType, MatrixDataProperty> matrix_data_types_ = {

  // Can't initialize static nonliteral types in header.
  static std::map<DataType, DataProperty> data_types_;
  // static constexpr std::map<DataType, DataProperty> data_types_ = {
  //   {DataType::UnknownMatDataType, {"UnknownMatDataType" , 0}},
  //   {DataType::ByteData          , {"ByteData"           , 1}},
  //   {DataType::VAX_Ix2           , {"VAX_Ix2"            , 2}},
  //   {DataType::VAX_Ix4           , {"VAX_Ix4,"           , 4}},
  //   {DataType::VAX_Rx4           , {"VAX_Rx4"            , 4}},
  //   {DataType::IeeeFloat         , {"IeeeFloat"          , 4}},
  //   {DataType::SunShort          , {"SunShort"           , 2}},  // big endian
  //   {DataType::SunLong           , {"SunLong"            , 4}},  // big endian
  //   {DataType::UShort_BE         , {"UShort_BE"          , 2}},
  //   {DataType::UShort_LE         , {"UShort_LE"          , 2}},
  //   {DataType::Color_24          , {"Color_24"           , 3}},
  //   {DataType::Color_8           , {"Color_8"            , 1}},
  //   {DataType::BitData           , {"BitData"            , 1}}
  // };

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

std::map<DataSetType, CodeName> data_set_types_ = {
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


  // Members
  int   matnum ;  /* matrix number */
  MatrixFile  *matfile ;  /* pointer to parent */
  DataSetType mat_type ;  /* type of matrix? */
  DataType  data_type ; /* type of data */
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

  // Methods
  ~MatrixData(void);
  void find_data_extrema(void);
  int num_pixels(void);
  int init_data_ptr(int num_bytes);
};
