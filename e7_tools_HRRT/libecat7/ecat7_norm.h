/*! \file ecat7_norm.h
    \brief This class implements the NORM matrix type for ECAT7 files.
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

class ECAT7_NORM: public ECAT7_MATRIX{
public:
  typedef struct {  /*! header of norm matrix (512 byte) */
    signed short int data_type,    /*!< datatype of dataset */
           num_dimensions;  /*! maximum ring difference */
    unsigned short int num_r_elements,  /*! number of sinogram planes */
             num_angles,  /*! number of angles in sinogram plane */
             num_z_elements;  /*! number of bins in projection */
    signed short int ring_difference;  /*! number of dimensions */
    float scale_factor,     /*!< normalization scale factor */
          norm_min,  /*! width of normalization source in cm */
          norm_max,  /*! maximum value for the normalization data */
          fov_source_width,  /*! minimum value for the normalization data */
          norm_quality_factor;     /*!< norm quality factor */
    signed short int norm_quality_factor_code,
           storage_order,/*!< view or volume mode */
           span,                        /*!< span */
           z_elements[64],           /*! number of planes per sinogram axis */
           fill_cti[123], /*!< CPS reserved space */
           fill_user[50];/*!< user reserved space */
  } tnorm_subheader;
private:
  tnorm_subheader nh;                           /*!< header of norm matrix */
protected:
  unsigned short int DataTypeOrig() const;  // request data type from matrix header
  unsigned short int Depth() const;                       // depth of dataset
  unsigned short int Height() const;                     // height of dataset
  unsigned short int Width() const;                       // width of dataset
public:
  ECAT7_NORM();                                          // initialize object
  ECAT7_NORM& operator = (const ECAT7_NORM &);                // '='-operator
  ECAT7_NORM::tnorm_subheader *HeaderPtr();  // request pointer to header structure
  void *LoadData(std::ifstream * const, const unsigned long int);  // load data part of matrix
  void LoadHeader(std::ifstream * const);            // load header of matrix
  void PrintHeader(std::list <std::string> * const,                   const unsigned short int) const;  // print header information into string list
  void SaveHeader(std::ofstream * const) const;// store header part of matrix

  // ahc this must be implemented in every class derived from ECAT7_MATRIX
  ecat_matrix::MatrixDataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  void set_data_type(ecat_matrix::MatrixDataType t_data_type);   // Write scoped enum to file as short int
};
