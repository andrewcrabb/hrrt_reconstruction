/*! \file ecat7_image.hpp
    \brief This class implements the IMAGE matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/19 added Doxygen style comments
 */

#pragma once

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"
#include "ecat_matrix.hpp"

/*- definitions -------------------------------------------------------------*/

class ECAT7_IMAGE: public ECAT7_MATRIX{
public:
  /*! header of image matrix (512 byte) */
  typedef struct {
    signed short int data_type,    // datatype of dataset
           num_dimensions,           // number of dimensions
           x_dimension,       // width of image
           y_dimension,      // height of image
           z_dimension;       // depth of image
    float x_offset,                     // x-offset in cm
          y_offset,                     // y-offset in cm
          z_offset,                     // z-offset in cm
          recon_zoom,       // reconstruction zoom factor
          scale_factor;    // quantification scale factor
    signed short int image_min,    // image minimum voxel value
           image_max;           // image maximum voxel value
    float x_pixel_size,              // voxel width in cm
          y_pixel_size,             // voxel height in cm
          z_pixel_size;              // voxel depth in cm
    signed long int frame_duration,    // frame duration in msec
           frame_start_time;           // frame start time relative to first frame in msec
    signed short int filter_code;          // filter code
    float x_resolution,// resolution in x-dimension in cm
          y_resolution,// resolution in y-dimension in cm
          z_resolution,// resolution in z-dimension in cm
          num_r_elements, // number of bins in projection
          num_angles,          // number of angles in sinogram plane
          z_rotation_angle,          // rotation in the x/y-plane in degrees (right-hand coordinate system)
          decay_corr_fctr;     // decay correction factor
    signed long int processing_code,   // processing code
           gate_duration,           // gate duration in msec
           r_wave_offset,       // R wave offset
           num_accepted_beats;           // number of accepted beats for this gate
    float filter_cutoff_frequency,   // cut-off frequency
          filter_resolution,                  // not used
          filter_ramp_slope;                  // not used
    signed short int filter_order;            // not used
    float filter_scatter_fraction,            // not used
          filter_scatter_slope;               // not used
    unsigned char annotation[40];     // image annotation
    float mt_1_1,         // transformation matrix m(1,1)
          mt_1_2,         // transformation matrix m(1,2)
          mt_1_3,         // transformation matrix m(1,3)
          mt_2_1,         // transformation matrix m(2,1)
          mt_2_2,         // transformation matrix m(2,2)
          mt_2_3,         // transformation matrix m(2,3)
          mt_3_1,         // transformation matrix m(3,1)
          mt_3_2,         // transformation matrix m(3,2)
          mt_3_3,         // transformation matrix m(3,3)
          rfilter_cutoff,        // radial filter cut-off
          rfilter_resolution; // radial filter resolution
    signed short int rfilter_code,  // radial filter code
           rfilter_order;// radial filter order
    float zfilter_cutoff,             // z-filter cut-off
          zfilter_resolution;      // z-filter resolution
    signed short int zfilter_code,       // z-filter code
           zfilter_order;     // z-filter order
    float mt_1_4,         // transformation matrix m(1,4)
          mt_2_4,         // transformation matrix m(2,4)
          mt_3_4;         // transformation matrix m(3,4)
    signed short int scatter_type,    // scatter correction type
           recon_type, // reconstruction method
           recon_views,           // number of views used in reconstruction
           fill_cti[87],  // CPS reserved space
           fill_user[48];// user reserved space
  } timage_subheader;
private:
  // ECAT7_IMAGE::timage_subheader ih;            // header of image matrix
  ECAT7_IMAGE::timage_subheader image_subheader_;            // header of image matrix
protected:
  unsigned short int DataTypeOrig() const;  // request data type from matrix header
  unsigned short int Depth() const;                       // depth of dataset
  unsigned short int Height() const;                     // height of dataset
  unsigned short int Width() const;                       // width of dataset
public:
  ECAT7_IMAGE();                                         // initialize object
  ECAT7_IMAGE& operator = (const ECAT7_IMAGE &);              // '='-operator
  void Feet2Head() const;                  // exchange feet and head position
  ECAT7_IMAGE::timage_subheader *HeaderPtr();  // request pointer to header structure
  void *LoadData(std::ifstream * const, const unsigned long int);  // load data part of matrix
  void LoadHeader(std::ifstream * const);            // load header of matrix
  void PrintHeader(std::list <std::string> * const,                   const unsigned short int) const;  // print header information into string list
  void Prone2Supine() const;       // change orientation from prone to supine
  void SaveHeader(std::ofstream * const) const;// store header part of matrix
  void ScaleMatrix(const float);                              // scale matrix

  // ahc this must be implemented in every class derived from ECAT7_MATRIX
  ecat_matrix::MatrixDataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  void set_data_type(ecat_matrix::MatrixDataType t_data_type);   // Write scoped enum to file as short int
};
