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

/*- class definitions -------------------------------------------------------*/

class ECAT7_MATRIX
 { protected:
    void *data;                                     /*!< data part of matrix */
    unsigned short int datatype;            /*!< data type of data in memory */
                                        // request data type from matrix header
    virtual unsigned short int DataTypeOrig() const;
    virtual unsigned short int Depth() const;               // depth of dataset
    virtual unsigned short int Height() const;             // height of dataset
    virtual unsigned short int Width() const;               // width of dataset
   public:
    ECAT7_MATRIX();                                        // initialize object
    virtual ~ECAT7_MATRIX();                                  // destroy object
    virtual ECAT7_MATRIX& operator = (const ECAT7_MATRIX &);    // '='-operator
    virtual void Byte2Float();    // convert dataset from ecat_matrix::MatrixDataType::ByteData to ecat_matrix::MatrixDataType::IeeeFloat
    void DataDeleted();                                     // remove data part
    unsigned long int DataSize() const;            // request size of data part
    unsigned short int DataType() const;      // request data type of data part
    virtual void DeleteData();                              // delete data part
                                                    // load data part of matrix
    virtual void *LoadData(std::ifstream * const, const unsigned long int);
    virtual void LoadHeader(std::ifstream * const);    // load header of matrix
                                                   // put data part into object
    virtual void getecat_matrix::MatrixData(void * const, const unsigned short int);
    void *getecat_matrix::MatrixData() const;                   // request pointer to data part
                            // calculate number of records needed for data part
    virtual unsigned long int NumberOfRecords() const;
                                   // print header information into string list
    virtual void PrintHeader(std::list <std::string> * const,
                             const unsigned short int) const;
                                                   // store data part of matrix
    virtual void SaveData(std::ofstream * const) const;
                                                 // store header part of matrix
    virtual void SaveHeader(std::ofstream * const) const;
    virtual void Short2Float();   // convert dataset from ecat_matrix::MatrixDataType::SunShort to ecat_matrix::MatrixDataType::IeeeFloat
 };

