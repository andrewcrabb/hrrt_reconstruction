/*! \class ECAT7_MATRIX ecat7_matrix_abc.h "ecat7_matrix_abc.h"
    \brief This class builds an abstract base class for all ECAT7 matrix types.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2004/09/22 added Doxygen style comments
 
    This class builds an abstract base class for all ECAT7 matrix types.
 */

#include "ecat7_matrix_abc.h"
#include "ecat_tmpl.h"
#include "ecat7_global.h"
#include "ecat7_utils.h"
#include "exception.h"

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
 */
ECAT7_MATRIX::ECAT7_MATRIX()
 { data=NULL;
   datatype=E7_DATATYPE_UNKNOWN;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.
 */
ECAT7_MATRIX::~ECAT7_MATRIX()
 { DeleteData();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content
 
    Copies dataset and datatype from another object into this one.
 */
ECAT7_MATRIX& ECAT7_MATRIX::operator = (const ECAT7_MATRIX &e7)
 { if (this != &e7)
    { try
      { unsigned long int matrix_size;
        DeleteData();
                                        // copy local variables into new object
        matrix_size=e7.DataSize();
        if (e7.getecat_matrix::MatrixData() != NULL)
         switch (e7.datatype)
          { case E7_DATATYPE_BYTE:
             data=new unsigned char[matrix_size];
             memcpy(data, e7.getecat_matrix::MatrixData(), matrix_size*sizeof(unsigned char));
             datatype=E7_DATATYPE_BYTE;
             break;
            case E7_DATATYPE_FLOAT:
             data=new float[matrix_size];
             memcpy(data, e7.getecat_matrix::MatrixData(), matrix_size*sizeof(float));
             datatype=E7_DATATYPE_FLOAT;
             break;
            case E7_DATATYPE_SHORT:
             data=new signed short int[matrix_size];
             memcpy(data, e7.getecat_matrix::MatrixData(),
                    matrix_size*sizeof(signed short int));
             datatype=E7_DATATYPE_SHORT;
             break;
            default:
             data=NULL;
             datatype=E7_DATATYPE_UNKNOWN;
             break;
          }
      }
      catch (...)
       { DeleteData();
         throw;
       }
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert byte dataset into float dataset.
 */
void ECAT7_MATRIX::Byte2Float()
 { if ((data == NULL) || (datatype != E7_DATATYPE_BYTE)) return;
   float *buffer=NULL;
   try
   { buffer=new float[DataSize()];
     for (unsigned long int xyz=0; xyz < DataSize(); xyz++)
      buffer[xyz]=(float)((unsigned char *)data)[xyz];
     DeleteData();
     data=buffer;
     datatype=E7_DATATYPE_FLOAT;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove dataset from object.  The dataset is removed from the object without deleting it.
 */
void ECAT7_MATRIX::DataDeleted()
 { data=NULL;                                        // removed but not deleted
   datatype=E7_DATATYPE_UNKNOWN;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request size of dataset.
    \return size of dataset
 */
unsigned long int ECAT7_MATRIX::DataSize() const
 { return((unsigned long int)Width()*(unsigned long int)Height()*
          (unsigned long int)Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype of dataset.
    \return datatype of dataset
 */
unsigned short int ECAT7_MATRIX::DataType() const { 
  return(datatype);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header
    \exception REC_INVALID_ECAT7_MATRIXTYPE the requested operation is not
                                            defined for this ECAT7 matrix type

    Request datatype from matrix header. This method has to be overloaded in
    derived classes.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_MATRIX::DataTypeOrig() const
 {                        // this method has to be redefined in derived classes
   throw Exception(REC_INVALID_ECAT7_MATRIXTYPE,
                   "The requested operation is not defined for this ECAT7 "
                   "matrix type.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete dataset.
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files
 */
void ECAT7_MATRIX::DeleteData()
 { if (data == NULL) return;
   switch (datatype)
    { case E7_DATATYPE_BYTE:
       delete[] (unsigned char *)data;
       break;
      case E7_DATATYPE_FLOAT:
       delete[] (float *)data;
       break;
      case E7_DATATYPE_SHORT:
       delete[] (signed short int *)data;
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
     }
   data=NULL;
   datatype=E7_DATATYPE_UNKNOWN;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request depth of dataset.
    \return depth of dataset
    \exception REC_INVALID_ECAT7_MATRIXTYPE the requested operation is not
                                            defined for this ECAT7 matrix type
    Request depth of dataset. Must be overloaded in derived classes.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_MATRIX::Depth() const
 {                        // this method has to be redefined in derived classes
   throw Exception(REC_INVALID_ECAT7_MATRIXTYPE,
                   "The requested operation is not defined for this ECAT7 "
                   "matrix type.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request height of dataset.
    \return height of dataset
    \exception REC_INVALID_ECAT7_MATRIXTYPE the requested operation is not
                                            defined for this ECAT7 matrix type
 
    Request height of dataset. Must be overloaded in derived classes
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_MATRIX::Height() const
 {                        // this method has to be redefined in derived classes
   throw Exception(REC_INVALID_ECAT7_MATRIXTYPE,
                   "The requested operation is not defined for this ECAT7 "
                   "matrix type.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load dataset of matrix into object.
    \param[in] file             handle of file
    \param[in] matrix_records   number of records to read
    \return pointer to dataset
 
    Load dataset of matrix into object.
 */
/*---------------------------------------------------------------------------*/
void *ECAT7_MATRIX::LoadData(std::ifstream * const file,
                             const unsigned long int matrix_records)
 { DeleteData();
   switch (DataTypeOrig())
    { case E7_DATA_TYPE_ByteData:
       data=(void *)loaddata <unsigned char>(file, DataSize(), matrix_records,                                             1);
       datatype=E7_DATATYPE_BYTE;
       break;
      case E7_DATA_TYPE_VAX_Ix2:
      case E7_DATA_TYPE_VAX_Ix4:
      case E7_DATA_TYPE_VAX_Rx4:
      case E7_DATA_TYPE_SunLong:
             break;
      case E7_DATA_TYPE_IeeeFloat:
       data=(void *)loaddata <float>(file, DataSize(), matrix_records, 4);
       datatype=E7_DATATYPE_FLOAT;
       break;
      case E7_DATA_TYPE_SunShort:
       data=(void *)loaddata <signed short int>(file, DataSize(),                                                matrix_records, 2);
       datatype=E7_DATATYPE_SHORT;
             break;
      default:
       break;
    }
   return(data);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of matrix.
    \exception REC_INVALID_ECAT7_MATRIXTYPE the requested operation is not
                                            defined for this ECAT7 matrix type

    Load header of matrix. Must be overloaded in derived classes.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_MATRIX::LoadHeader(std::ifstream * const)
 {                        // this method has to be redefined in derived classes
   throw Exception(REC_INVALID_ECAT7_MATRIXTYPE,
                   "The requested operation is not defined for this ECAT7 "
                   "matrix type.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Move dataset into object.
    \param[in] ptr   pointer to dataset
    \param[in] dt    datatype
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files
 
    Move dataset into object.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_MATRIX::getecat_matrix::MatrixData(void * const ptr, const unsigned short int dt)
 { if ((dt != E7_DATATYPE_BYTE) && (dt != E7_DATATYPE_SHORT) &&
       (dt != E7_DATATYPE_FLOAT))
    throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                    "Data type not supported for ECAT7 files.");
   DeleteData();
   data=ptr;
   datatype=dt;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to dataset.    \return pointer to dataset
 */
void *ECAT7_MATRIX::getecat_matrix::MatrixData() const
 { return(data);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate number of records needed for dataset.
    \return number of records for dataset
 */
unsigned long int ECAT7_MATRIX::NumberOfRecords() const
 { unsigned long int records, factor=1;
   switch (datatype)
    { case E7_DATATYPE_BYTE:  factor=1;
                              break;
      case E7_DATATYPE_FLOAT: factor=4;
                              break;
      case E7_DATATYPE_SHORT: factor=2;
                              break;
      default:                return(0);
    }
   records=(DataSize()*factor)/E7_RECLEN;
   if (((DataSize()*factor) % E7_RECLEN) != 0) records++;
   return(records);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print header information into string list.
    \exception REC_INVALID_ECAT7_MATRIXTYPE the requested operation is not
                                            defined for this ECAT7 matrix type

    Print header information into string list. Must be overloaded in derived classes.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_MATRIX::PrintHeader(std::list <std::string> * const,                               const unsigned short int) const {
                        // this method has to be redefined in derived classes
   throw Exception(REC_INVALID_ECAT7_MATRIXTYPE,  "The requested operation is not defined for this ECAT7 matrix type.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store dataset of matrix.
    \param[in] file   handle of file
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files
 
    Store dataset of matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_MATRIX::SaveData(std::ofstream * const file) const
 { if (data == NULL) return;
   switch (datatype)
    { case E7_DATATYPE_BYTE:
       savedata(file, (unsigned char *)data, DataSize(), 1);
       break;
      case E7_DATATYPE_FLOAT:
       savedata(file, (float *)data, DataSize(), 4);
       break;
      case E7_DATATYPE_SHORT:
       savedata(file, (signed short int *)data, DataSize(), 2);
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of matrix.
    \exception REC_INVALID_ECAT7_MATRIXTYPE requested operation not defined for this ECAT7 matrix type
 */
void ECAT7_MATRIX::SaveHeader(std::ofstream * const) const
 {                        // this method has to be redefined in derived classes
   throw Exception(REC_INVALID_ECAT7_MATRIXTYPE,
                   "The requested operation is not defined for this ECAT7 matrix type.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset from short int to float.
 */
void ECAT7_MATRIX::Short2Float()
 { if ((data == NULL) || (datatype != E7_DATATYPE_SHORT)) return;
   float *buffer=NULL;

   try
   { buffer=new float[DataSize()];
     for (unsigned long int xyz=0; xyz < DataSize(); xyz++)
      buffer[xyz]=(float)((signed short int *)data)[xyz];
     DeleteData();
     data=buffer;
     datatype=E7_DATATYPE_FLOAT;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request width of dataset.
    \return width of dataset
    \exception REC_INVALID_ECAT7_MATRIXTYPE  not defined for this ECAT7 matrix type
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_MATRIX::Width() const {
   throw Exception(REC_INVALID_ECAT7_MATRIXTYPE,
                   "The requested operation is not defined for this ECAT7 matrix type.");
 }
