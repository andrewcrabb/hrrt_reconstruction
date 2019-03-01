/*! \file ecat7_matrix_abc.h
    \brief This class builds an abstract base class for all ECAT7 matrix types.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2004/09/22 added Doxygen style comments
 */

#pragma once

#include <fstream>
#include <list>
#include <string>

#include "ecat_matrix.hpp"

/*- class definitions -------------------------------------------------------*/

class ECAT7_MATRIX {
protected:
  void *data;                                     /*!< data part of matrix */
  unsigned short int datatype;            /*!< data type of data in memory */
  // request data type from matrix header
  virtual unsigned short int DataTypeOrig() const;
  virtual unsigned short int Depth() const;  // depth of dataset
  virtual unsigned short int Height() const;   // height of dataset
  virtual unsigned short int Width() const;  // width of dataset
public:
  ECAT7_MATRIX();               // initialize object
  virtual ~ECAT7_MATRIX();         // destroy object
  virtual ECAT7_MATRIX& operator = (const ECAT7_MATRIX &);    // '='-operator
  virtual void Byte2Float();    // convert dataset from ecat_matrix::MatrixDataType::ByteData to ecat_matrix::MatrixDataType::IeeeFloat
  void DataDeleted();            // remove data part
  unsigned long int DataSize() const;  // request size of data part
  unsigned short int DataType() const;      // request data type of data part
  virtual void DeleteData();     // delete data part
  virtual void *LoadData(std::ifstream * const, const unsigned long int);                           // load data part of matrix
  virtual void LoadHeader(std::ifstream * const);    // load header of matrix
  virtual void getecat_matrix::MatrixData(void * const, const unsigned short int);                          // put data part into object
  void *getecat_matrix::MatrixData() const;    // request pointer to data part
  virtual unsigned long int NumberOfRecords() const;   // calculate number of records needed for data part
  virtual void PrintHeader(std::list <std::string> * const, const unsigned short int) const;          // print header information into string list
  virtual void SaveData(std::ofstream * const) const;                          // store data part of matrix
  virtual void SaveHeader(std::ofstream * const) const;                        // store header part of matrix
  virtual void Short2Float();   // convert dataset from ecat_matrix::MatrixDataType::SunShort to ecat_matrix::MatrixDataType::IeeeFloat

  // ahc these methods allow the file-stored data type to differ from those used in memory.
  // ie data_type is a short int in file, but a ecat_matrix::MatrixDataType in memory
  virtual ecat_matrix::MatrixDataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  virtual void set_data_type(ecat_matrix::MatrixDataType t_data_type);   // Write scoped enum to file as short int
};
