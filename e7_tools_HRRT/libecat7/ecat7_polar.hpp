/* \file ecat7_polar.hpp
    \brief This class implements the polar map matrix type for ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
*/

#pragma once

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"

/*- class definitions -------------------------------------------------------*/
// header of polar map matrix (512 byte)
class ECAT7_POLAR: public ECAT7_MATRIX {
public:

  typedef struct {
    signed short int data_type,    // datatype of dataset
           polar_map_type,   // version of the polar map structure
           num_rings,         // number of rings in polar map
           sectors_per_ring[32];        // number of sectors per ring for upto 32 rings
    float ring_position[32];// fractional distance along the long axis from base to apex
    signed short int ring_angle[32],        // ring angle relative to long axis (90 degrees  along cylinder, decreasing to 0 at the apex)
           start_angle,       // start angle for rings (always 258 degrees, defines polar map's 0)
           long_axis_left[3],    // x,y,z-location of long axis base end (in pixels)
           long_axis_right[3],    // x,y,z-location of long axis apex end (in pixels)
           position_data,            // position data available ?
           image_min,              // minimum pixel value in this polar map
           image_max;// maximum pixel value in this polar map
    float scale_factor, // scale factor from int to float
          pixel_size;// pixel size in cm^3
    signed long int frame_duration,            // duration of frame in msec
           frame_start_time;  // frame start time in msec (offset from first frame)
    signed short int processing_code,  // processing code
           quant_units; // quantification units
    unsigned char annotation[40];          // label for polar map display
    signed long int gate_duration, // gate duration in msec
           r_wave_offset, // R wave offset in msec
           num_accepted_beats;              // number of accepted beats for this gate
    unsigned char polar_map_protocol[20],  // polar map protocol used to generate this polar map
             database_name[30];         // database name used for polar map comparison
    signed short int fill_cti[27],  // CPS reserved space
           fill_user[27];// user reserved space
  } tpolar_subheader;
private:
  tpolar_subheader ph; // header of polar map matrix
protected:
  unsigned short int DataTypeOrig() const;     // request data type from matrix header
public:
  ECAT7_POLAR();      // initialize object
  ECAT7_POLAR& operator = (const ECAT7_POLAR &);              // '='-operator
  ECAT7_POLAR::tpolar_subheader *HeaderPtr();      // request pointer to header structure
  void *LoadData(std::ifstream * const, const unsigned long int);  // load data part of matrix
  void LoadHeader(std::ifstream * const);            // load header of matrix
  void PrintHeader(std::list <std::string> * const,      const unsigned short int) const;// print header information into string list
  void SaveHeader(std::ofstream * const) const;              // store header part of matrix

  // ahc this must be implemented in every class derived from ECAT7_MATRIX
  MatrixData::DataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  void set_data_type(MatrixData::DataType t_data_type);   // Write scoped enum to file as short int
};

