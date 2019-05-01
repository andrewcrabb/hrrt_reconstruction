// matrix_data.cpp
// ahc 20190305
// MatrixData and its related structs and methods
// Moved from ecat_matrix namespace

#include "matrix_data.hpp"

// Can't initialize static nonliteral types in header.
data_types_ = {
    {MatrixData::DataType::UnknownMatDataType, {"UnknownMatDataType" , 0}},
    {MatrixData::DataType::ByteData          , {"ByteData"           , 1}},
    {MatrixData::DataType::VAX_Ix2           , {"VAX_Ix2"            , 2}},
    {MatrixData::DataType::VAX_Ix4           , {"VAX_Ix4,"           , 4}},
    {MatrixData::DataType::VAX_Rx4           , {"VAX_Rx4"            , 4}},
    {MatrixData::DataType::IeeeFloat         , {"IeeeFloat"          , 4}},
    {MatrixData::DataType::SunShort          , {"SunShort"           , 2}},  // big endian
    {MatrixData::DataType::SunLong           , {"SunLong"            , 4}},  // big endian
    {MatrixData::DataType::UShort_BE         , {"UShort_BE"          , 2}},
    {MatrixData::DataType::UShort_LE         , {"UShort_LE"          , 2}},
    {MatrixData::DataType::Color_24          , {"Color_24"           , 3}},
    {MatrixData::DataType::Color_8           , {"Color_8"            , 1}},
    {MatrixData::DataType::BitData           , {"BitData"            , 1}}
  };

MatrixData::~MatrixData() {
      if (data_ptr != NULL) 
        delete[] data_ptr;
      // Started using new char[] in interfile.cpp:1152
      if (shptr != NULL) 
        delete[] shptr;
      free(data) ;
}

int MatrixData::num_pixels(void) {
	return xdim * ydim * zdim;
}

// static void find_data_extrema(ecat_matrix::MatrixData *t_data){
void MatrixData::find_data_extrema(void) {
  int npixels = t_data->xdim * t_data->ydim * t_data->zdim;
  switch (t_data->data_type) {
  case MatrixData::DataType::ByteData :
  case MatrixData::DataType::Color_8 :
    // t_data->data_max = ecat_matrix::find_bmax((unsigned char *)t_data->data_ptr, npixels);
    // t_data->data_min = ecat_matrix::find_bmin((unsigned char *)t_data->data_ptr, npixels);
      hrrt_util::Extrema<unsigned char> extrema = hrrt_util::find_extrema<unsigned char>(static_cast<unsigned char *>(t_data->data_ptr), npixels);
      t_data->data_min = extrema.min;
      t_data->data_max = extrema.max;
    break;
  default :
  case MatrixData::DataType::SunShort:
  case MatrixData::DataType::VAX_Ix2:
    // t_data->data_max = ecat_matrix::find_smax((short*)t_data->data_ptr, npixels);
    // t_data->data_min = ecat_matrix::find_smin((short*)t_data->data_ptr, npixels);
      hrrt_util::Extrema<short> extrema = hrrt_util::find_extrema<short>(static_cast<short *>(t_data->data_ptr), npixels);
      t_data->data_min = extrema.min;
      t_data->data_max = extrema.max;
    break;
  case MatrixData::DataType::SunLong:
    // t_data->data_max = (float)ecat_matrix::find_imax((int*)t_data->data_ptr, npixels);
    // t_data->data_min = (float)ecat_matrix::find_imin((int*)t_data->data_ptr, npixels);
      hrrt_util::Extrema<int> extrema = hrrt_util::find_extrema<int>(static_cast<int *>(t_data->data_ptr), npixels);
      t_data->data_min = static_cast<float>(extrema.min);
      t_data->data_max = static_cast<float>(extrema.max);
    break;
  case MatrixData::DataType::IeeeFloat:
    // t_data->data_max = ecat_matrix::find_fmax((float*)t_data->data_ptr, npixels);
    // t_data->data_min = ecat_matrix::find_fmin((float*)t_data->data_ptr, npixels);
      hrrt_util::Extrema<float> extrema = hrrt_util::find_extrema<float>(static_cast<float *>(t_data->data_ptr), npixels);
      t_data->data_min = extrema.min;
      t_data->data_max = extrema.max;
    break;
  case MatrixData::DataType::Color_24 :  /* get min and max brightness */
    // t_data->data_max = ecat_matrix::find_bmax((unsigned char *)t_data->data_ptr, 3 * npixels);
    // t_data->data_min = ecat_matrix::find_bmin((unsigned char *)t_data->data_ptr, 3 * npixels);
      hrrt_util::Extrema<unsigned char> extrema = hrrt_util::find_extrema<unsigned char>(static_cast<unsigned char *>(t_data->data_ptr), 3 * npixels);
      t_data->data_min = extrema.min;
      t_data->data_max = extrema.max;
  }
  t_data->data_max *=  t_data->scale_factor;
  t_data->data_min *=  t_data->scale_factor;
}

def init_data_ptr(int num_bytes) {
  data_ptr = new char[num_bytes];
}
