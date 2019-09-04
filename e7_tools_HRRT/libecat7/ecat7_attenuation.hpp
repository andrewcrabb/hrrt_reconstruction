/* \file ecat7_attenuation.hpp
    \brief This class implements the ATTENUATION matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2004/09/22 added Doxygen style comments
 */

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"
#include "ecat_matrix.hpp"

/*- class definitions -------------------------------------------------------*/

class ECAT7_ATTENUATION: public ECAT7_MATRIX {
public:
  // header of attenuation matrix (512 byte)
  typedef struct {
    signed short int data_type;    // datatype of dataset
    signed short int num_dimensions;    // maximum ring difference of sinogram
    signed short int attenuation_type;// attenuation type
    unsigned short int num_r_elements,             /* number of planes in sinogram */
             num_angles,             /* number of angles in a projection */
             num_z_elements;    // number of bins in a projection
    signed short int ring_difference;           // number of dimensions
    float x_resolution,// resolution in x-dimension in cm
          y_resolution,// resolution in y-dimension in cm
          z_resolution,// resolution in z-dimension in cm
          w_resolution,               // to be determined
          scale_factor,                   // scale factor
          x_offset,    // data storage order rtzd or rztd (volume/view)
          y_offset,    // additional attenuation coefficients
          x_radius,     // ellipse radius in x-axis in cm
          y_radius,     // ellipse radius in y-axis in cm
          tilt_angle,    // number of attenuation coefficients other than the u-value above
          attenuation_coeff,           // u-value in 1/cm
          attenuation_min,          // maximum value in attenuation data
          attenuation_max,          // minimum value in attenuation data
          skull_thickness;       // skull thickness in cm
    signed short int num_additional_atten_coeff;          // tilt angle of the ellipse in degrees
    float additional_atten_coeff[8],          // ellipse offset in y-axis from center in cm
          edge_finding_threshold;          // threshold value used by the automatic edge-detection routine (fraction of maximum)        
    signed short int storage_order,          // ellipse offset in x-axis from center in cm
           span,             /*< span of sinogram */
           z_elements[64],           // number of planes for each segment
           fill_unused[86],           // unused
           fill_user[50];             // unused
  } tattenuation_subheader;
private:
  // tattenuation_subheader ah;             // header of attenuation matrix
  tattenuation_subheader attenuation_subheader_;             // header of attenuation matrix
protected:
  // request data type from matrix header
  unsigned short int DataTypeOrig() const;
  unsigned short int Depth() const;                       // depth of dataset
  unsigned short int Height() const;                     // height of dataset
  unsigned short int Width() const;                       // width of dataset
public:
  ECAT7_ATTENUATION();                                   // initialize object
  ECAT7_ATTENUATION& operator = (const ECAT7_ATTENUATION &);
  void CutBins(const unsigned short int);  // cut bins on both sides of sinograms
  void DeInterleave();                                   // deinterleave data
  void Feet2Head() const;                  // exchange feet and head position
  ECAT7_ATTENUATION::tattenuation_subheader *HeaderPtr();  // request pointer to header structure
  void Interleave();                                    // interleave dataset
  void *LoadData(std::ifstream * const, const unsigned long int);  // load data part of matrix
  void LoadHeader(std::ifstream * const);            // load header of matrix
  void PrintHeader(std::list <std::string> * const, const unsigned short int) const;  // print header information into string list
  void Prone2Supine() const;       // change orientation from prone to supine
  void Rotate(const signed short int) const;  // rotate dataset counterclockwise along scanner axis
  void SaveHeader(std::ofstream * const) const;// store header part of matrix
  void ScaleMatrix(const float);                              // scale matrix
  void Tra2Emi() const;       // convert transmission data into emission data
  void View2Volume() const;     // convert dataset from view to volume format
  void Volume2View() const;     // convert dataset from volume to view format

  // ahc this must be implemented in every class derived from ECAT7_MATRIX
  MatrixData::DataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  void set_data_type(MatrixData::DataType t_data_type);   // Write scoped enum to file as short int
};

