// matrix_data.cpp
// ahc 20190305
// MatrixData and its related structs and methods
// Moved from ecat_matrix namespace

#include "matrix_data.hpp"

int MatrixData::num_pixels(void) {
	return xdim * ydim * zdim;
}

// static void find_data_extrema(ecat_matrix::MatrixData *t_data){
void MatrixData::find_data_extrema(void) {
  int npixels = t_data->xdim * t_data->ydim * t_data->zdim;
  switch (t_data->data_type) {
  case MatrixData::DataType::ByteData :
  case MatrixData::DataType::Color_8 :
    t_data->data_max = ecat_matrix::find_bmax((unsigned char *)t_data->data_ptr, npixels);
    t_data->data_min = ecat_matrix::find_bmin((unsigned char *)t_data->data_ptr, npixels);
    break;
  default :
  case MatrixData::DataType::SunShort:
  case MatrixData::DataType::VAX_Ix2:
    t_data->data_max = ecat_matrix::find_smax((short*)t_data->data_ptr, npixels);
    t_data->data_min = ecat_matrix::find_smin((short*)t_data->data_ptr, npixels);
    break;
  case MatrixData::DataType::SunLong:
    t_data->data_max = (float)ecat_matrix::find_imax((int*)t_data->data_ptr, npixels);
    t_data->data_min = (float)ecat_matrix::find_imin((int*)t_data->data_ptr, npixels);
    break;
  case MatrixData::DataType::IeeeFloat:
    t_data->data_max = ecat_matrix::find_fmax((float*)t_data->data_ptr, npixels);
    t_data->data_min = ecat_matrix::find_fmin((float*)t_data->data_ptr, npixels);
    break;
  case MatrixData::DataType::Color_24 :  /* get min and max brightness */
    t_data->data_max = ecat_matrix::find_bmax((unsigned char *)t_data->data_ptr, 3 * npixels);
    t_data->data_min = ecat_matrix::find_bmin((unsigned char *)t_data->data_ptr, 3 * npixels);
  }
  t_data->data_max *=  t_data->scale_factor;
  t_data->data_min *=  t_data->scale_factor;
}
