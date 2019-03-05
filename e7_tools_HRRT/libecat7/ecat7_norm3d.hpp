/* \file ecat7_norm3d.hpp
    \brief This class implements the NORM3D matrix type for ECAT7 files.
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

class ECAT7_NORM3D: public ECAT7_MATRIX { // header of norm3d matrix (512 byte)
public:
  typedef struct {
    signed short int data_type,    // datatype of dataset

           num_r_elements,                                            // number of bins in projection
           num_transaxial_crystals,                                 // number of transaxial crystals per block
           num_crystal_rings,                                                 // number of crystal rings
           crystals_per_ring,                                             // number of crystals per ring
           num_geo_corr_planes,                  // number of rows in the plane geometric correction array
           uld,    // upper energy limit in keV
           lld,    // lower energy limit in keV
           scatter_energy;                                                // scatter energy threshold
    float norm_quality_factor;     // norm quality factor
    signed short int norm_quality_factor_code;                                                             // not defined
    float ring_dtcor1[32],
          ring_dtcor2[32],                      // second "per ring" dead time correction coefficient
          crystal_dtcor[8];                    // dead time correction factors for transaxial crystals
    signed short int span,                        // span
           max_ring_diff,                                                // maximum ring difference
           fill_cti[48],  // CPS reserved space
           fill_user[50];// user reserved space
  } tnorm3d_subheader;
private:
  tnorm3d_subheader nh;                       // header of norm3d matrix
  float *plane_corr,              // data for plane geometric correction
        *intfcorr,           // data for crystal interference correction
        *crystal_eff;          // data for crystal efficiency correction
protected:
  unsigned short int DataTypeOrig() const;                                        // request data type from matrix header
public:
  ECAT7_NORM3D();                                        // initialize object
  virtual ~ECAT7_NORM3D();                                      // destructor
  ECAT7_NORM3D& operator = (const ECAT7_NORM3D &);            // '='-operator
  float *CrystalEffPtr() const;                           // pointer to data for crystal efficiency correction
  void CrystalEffPtr(float * const);
  void DeleteData();                                      // delete data part
  ECAT7_NORM3D::tnorm3d_subheader *HeaderPtr();                                         // request pointer to header structure
  float *IntfCorrPtr() const;                         // pointer to data for crystal interference correction
  void IntfCorrPtr(float * const);
  void *LoadData(std::ifstream * const, const unsigned long int);                                                    // load data part of matrix
  void LoadHeader(std::ifstream * const);            // load header of matrix
  unsigned long int NumberOfRecords() const;                            // calculate number of records needed for data part
  float *PlaneCorrPtr() const;                              // pointer to data for plane geometric correction
  void PlaneCorrPtr(float * const);
  void PrintHeader(std::list <std::string> * const,                   const unsigned short int) const;                                   // print header information into string list
  void SaveData(std::ofstream * const) const;    // store data part of matrix
  void SaveHeader(std::ofstream * const) const;// store header part of matrix

  // ahc this must be implemented in every class derived from ECAT7_MATRIX
  ecat_matrix::MatrixDataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  void set_data_type(ecat_matrix::MatrixDataType t_data_type);   // Write scoped enum to file as short int
};

