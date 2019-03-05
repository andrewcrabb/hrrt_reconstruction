/*! \class ECAT7 ecat7.h "ecat7.h"
    \brief This class implements the ECAT7 file format.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/19 added Doxygen style documentation
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class implements the ECAT7 file format. Data and headers are kept
    independently. When loading an ECAT7 file, all the headers are loaded, but
    none of the datasets. These are loaded explicitly to allow the user to keep
    only the data matrices in memory that are needed.
 */

#include <iostream>
#include <sys/stat.h>
#include "ecat7.h"
#include "ecat7_attenuation.hpp"
#include "ecat7_image.hpp"
#include "ecat7_norm.h"
#include "ecat7_norm3d.hpp"
#include "ecat7_polar.hpp"
#include "ecat7_scan.h"
#include "ecat7_scan3d.hpp"
#include "exception.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object and fill header data structure with 0s.
 */
/*---------------------------------------------------------------------------*/
ECAT7::ECAT7()
 { e7_matrix_.clear();                                     // no matrices stored
   e7_main_header_ = NULL;                                       // no main header
   e7_directory_ = NULL;                                           // no directory
   in_filename_ = std::string();
   out_filename_ = std::string();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
ECAT7::~ECAT7()
 { DeleteMatrices();                           // delete matrices and directory
   DeleteMainHeader();                                    // delete main header
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content
 
    This operator copies the ecat7 main header and directory and uses the copy
    operators of the ECAT7 matrix classes to copy the datasets from another
    object into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7& ECAT7::operator = (const ECAT7 &e7)
 { if (this != &e7)
    {                                                   // copy local variables
      try
      { in_filename_ = e7.in_filename_;
        out_filename_ = e7.out_filename_;
        DeleteMainHeader();
        e7_main_header_ =new ECAT7_MAINHEADER();              // copy main header
        *e7_main_header_ = *e7.e7_main_header_;
        DeleteMatrices();
        e7_directory_ =new ECAT7_DIRECTORY();                   // copy directory
        *e7_directory_ = *e7.e7_directory_;
                                                               // copy matrices
        for (unsigned short int mnr=0; mnr < e7.NumberOfMatrices(); mnr++)
         switch (e7_main_header_->HeaderPtr()->file_type)
          { case E7_FILE_TYPE_Sinogram:
             { ECAT7_SCAN *mb;

               mb=new ECAT7_SCAN();
               *mb= *(ECAT7_SCAN *)e7.Matrix(mnr);
               e7_matrix_.push_back(mb);
             }
             break;
            case E7_FILE_TYPE_AttenuationCorrection:
             { ECAT7_ATTENUATION *mb;

               mb=new ECAT7_ATTENUATION();
               *mb= *(ECAT7_ATTENUATION *)e7.Matrix(mnr);
               e7_matrix_.push_back(mb);
             }
             break;
            case E7_FILE_TYPE_Normalization:
             { ECAT7_NORM *mb;

               mb=new ECAT7_NORM();
               *mb= *(ECAT7_NORM *)e7.Matrix(mnr);
               e7_matrix_.push_back(mb);
             }
             break;
            case E7_FILE_TYPE_PolarMap:
             { ECAT7_POLAR *mb;

               mb=new ECAT7_POLAR();
               *mb= *(ECAT7_POLAR *)e7.Matrix(mnr);
               e7_matrix_.push_back(mb);
             }
             break;
            case E7_FILE_TYPE_Image8:
            case E7_FILE_TYPE_Image16:
            case E7_FILE_TYPE_Volume8:
            case E7_FILE_TYPE_Volume16:
             { ECAT7_IMAGE *mb;

               mb=new ECAT7_IMAGE();
               *mb= *(ECAT7_IMAGE *)e7.Matrix(mnr);
               e7_matrix_.push_back(mb);
             }
             break;
            case E7_FILE_TYPE_3D_Sinogram8:
            case E7_FILE_TYPE_3D_Sinogram16:
            case E7_FILE_TYPE_3D_SinogramFloat:
             { ECAT7_SCAN3D *mb;

               mb=new ECAT7_SCAN3D();
               *mb= *(ECAT7_SCAN3D *)e7.Matrix(mnr);
               e7_matrix_.push_back(mb);
             }
             break;
            case E7_FILE_TYPE_3D_Normalization:
             { ECAT7_NORM3D *mb;

               mb=new ECAT7_NORM3D();
               *mb= *(ECAT7_NORM3D *)e7.Matrix(mnr);
               e7_matrix_.push_back(mb);
             }
             break;
            default:
             throw Exception(REC_UNKNOWN_ECAT7_MATRIXTYPE,
                             "Unknown ECAT7 matrix type '#1'"
                             ".").arg(e7_main_header_->HeaderPtr()->file_type);
             break;
          }
      }
      catch (...)
       { DeleteMainHeader();
         DeleteMatrices();
         throw;
       }
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Append matrix at end of ECAT7 file.
    \param[in] num   matrix number
    \exception REC_ECAT7_MATRIXHEADER_MISSING ECAT7 matrix header is missing

    Append matrix (header and data) at end of ECAT7 file. The directory is
    updated accordingly.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::AppendMatrix(const unsigned short int num) const
 { std::ofstream *file=NULL;
                                                         // check matrix header
   if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num) ||
       (e7_directory_ == NULL))
    throw Exception(REC_ECAT7_MATRIXHEADER_MISSING,
                    "ECAT7 matrix header is missing.");
   try
   {                                            // register matrix in directory
     e7_directory_->AppendEntry(out_filename_, e7_matrix_[num]->NumberOfRecords(),
                               num);
     file=new std::ofstream(out_filename_.c_str(),
                            std::ios::app|std::ios::binary);
     file->seekp(0, std::ios::end);             // append matrix at end of file
     e7_matrix_[num]->SaveHeader(file);                   // store matrix header
     e7_matrix_[num]->SaveData(file);                       // store matrix data
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Append matrix to ECAT7 file.
    \param[in] num        matrix number
    \param[in] filename   name of ECAT7 file

    Append matrix to ECAT7 file and set name of output file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::AppendMatrix(const unsigned short int num,
                         const std::string filename)
 { out_filename_ =filename;
   AppendMatrix(num);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset from ecat_matrix::MatrixDataType::ByteData to ecat_matrix::MatrixDataType::IeeeFloat.
    \param[in] num   matrix number

    Convert dataset from ecat_matrix::MatrixDataType::ByteData to ecat_matrix::MatrixDataType::IeeeFloat.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::Byte2Float(const unsigned short int num) const
 { if (e7_matrix_.size() > num) e7_matrix_[num]->Byte2Float();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check format of ECAT7 file.
    \param[in] file   handle of ECAT7 file
    \exception REC_FILE_DOESNT_EXIST the file doesn't exist

    Check format of ECAT7 file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CheckFormat(std::ifstream * const file)
 { if (!*file)
    throw Exception(REC_FILE_DOESNT_EXIST,
                    "The file '#1' doesn't exist.").arg(in_filename_);
#if 0
                                    // some file sizes are not multiples of 512
                                    // therefore this test is not performed
   signed long int size;
                                                         // determine file size
   file->seekg(0, std::ios::end);
   size=file->tellg();
   file->seekg(0);
   if ((size < (signed long int)E7_RECLEN) || ((size % E7_RECLEN) != 0))
    throw Exception(REC_FILESIZE_WRONG,
                    "The size of the file '#1' is wrong.").arg(in_filename_);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create attenuation matrices.
    \param[in] num   number of new matrices

    Create attenuation matrices or add matrices to the existing ones. The
    directory is updated accordingly. If the ECAT7 file contains matrices which
    are not attenuation matrices, they are deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateAttnMatrices(const unsigned short int num)
 { try
   {                                         // create main header if necessary
     if (e7_main_header_ == NULL) e7_main_header_ =new ECAT7_MAINHEADER();
                                               // create directory if necessary
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
                // delete existing matrices if they are no attenuation matrices
     if (e7_main_header_->HeaderPtr()->file_type !=
         E7_FILE_TYPE_AttenuationCorrection)
      { DeleteMatrices();
        Main_file_type(E7_FILE_TYPE_AttenuationCorrection);
        CreateDirectoryStruct();
      }
                                                 // create attenuation matrices
     for (unsigned short int i=0; i < num; i++)
      e7_matrix_.push_back(new ECAT7_ATTENUATION());
     e7_directory_->AddEntries(num);                 // create directory entries
   }
   catch (...)
    { DeleteMainHeader();
      DeleteMatrices();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create directory.

    Create directory. An existing directory is deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateDirectoryStruct()
 { try
   { DeleteDirectory();
     e7_directory_ =new ECAT7_DIRECTORY();
   }
   catch (...)
    { DeleteDirectory();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create image matrices.
    \param[in] num   number of new matrices

    Create image matrices or add matrices to the existing ones. The directory
    is updated accordingly. If the ECAT7 file contains matrices which are not
    image matrices, they are deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateImageMatrices(const unsigned short int num)
 { try
   { signed short int ft;
                                             // create main header if necessary
     if (e7_main_header_ == NULL) e7_main_header_ =new ECAT7_MAINHEADER();
                                               // create directory if necessary
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
                      // delete existing matrices if they are no image matrices
     ft= e7_main_header_->HeaderPtr()->file_type;
     if ((ft != E7_FILE_TYPE_Image8) && (ft != E7_FILE_TYPE_Image16) &&
         (ft != E7_FILE_TYPE_Volume8) && (ft != E7_FILE_TYPE_Volume16))
      { DeleteMatrices();
        Main_file_type(E7_FILE_TYPE_Volume16);
        CreateDirectoryStruct();
      }
     for (unsigned short int i=0; i < num; i++)
      e7_matrix_.push_back(new ECAT7_IMAGE());          // create image matrices
     e7_directory_->AddEntries(num);                 // create directory entries
   }
   catch (...)
    { DeleteMainHeader();
      DeleteMatrices();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create main header.

    Create main header. An existing main header is deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateMainHeader()
 { try
   { DeleteMainHeader();
     e7_main_header_ =new ECAT7_MAINHEADER();
   }
   catch (...)
    { DeleteMainHeader();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create norm matrices.
    \param[in] num   number of new matrices

    Create norm matrices or add matrices to the existing ones. The directory
    is updated accordingly. If the ECAT7 file contains matrices which are not
    norm matrices, they are deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateNormMatrices(const unsigned short int num)
 { try
   {                                         // create main header if necessary
     if (e7_main_header_ == NULL) e7_main_header_ =new ECAT7_MAINHEADER();
                                               // create directory if necessary
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
                       // delete existing matrices if they are no norm matrices
     if (e7_main_header_->HeaderPtr()->file_type != E7_FILE_TYPE_Normalization)
      { DeleteMatrices();
        Main_file_type(E7_FILE_TYPE_Normalization);
        CreateDirectoryStruct();
      }
     for (unsigned short int i=0; i < num; i++)
      e7_matrix_.push_back(new ECAT7_NORM());            // create norm matrices
     e7_directory_->AddEntries(num);                 // create directory entries
   }
   catch (...)
    { DeleteMainHeader();
      DeleteMatrices();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create norm3d matrices.
    \param[in] num   number of new matrices

    Create norm3d matrices or add matrices to the existing ones. The directory
    is updated accordingly. If the ECAT7 file contains matrices which are not
    norm3d matrices, they are deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateNorm3DMatrices(const unsigned short int num)
 { try
   {                                         // create main header if necessary
     if (e7_main_header_ == NULL) e7_main_header_ =new ECAT7_MAINHEADER();
                                               // create directory if necessary
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
                     // delete existing matrices if they are no norm3d matrices
     if (e7_main_header_->HeaderPtr()->file_type !=
         E7_FILE_TYPE_3D_Normalization)
      { DeleteMatrices();
        Main_file_type(E7_FILE_TYPE_3D_Normalization);
        CreateDirectoryStruct();
      }
     for (unsigned short int i=0; i < num; i++)
      e7_matrix_.push_back(new ECAT7_NORM3D());        // create norm3d matrices
     e7_directory_->AddEntries(num);                 // create directory entries
   }
   catch (...)
    { DeleteMainHeader();
      DeleteMatrices();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create polar matrices.
    \param[in] num   number of new matrices

    Create polar matrices or add matrices to the existing ones. The directory
    is updated accordingly. If the ECAT7 file contains matrices which are not
    polar matrices, they are deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreatePolarMatrices(const unsigned short int num)
 { try
   {                                         // create main header if necessary
     if (e7_main_header_ == NULL) e7_main_header_ =new ECAT7_MAINHEADER();
                                               // create directory if necessary
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
                  // delete existing matrices if they are no polar map matrices
     if (e7_main_header_->HeaderPtr()->file_type != E7_FILE_TYPE_PolarMap)
      { DeleteMatrices();
        Main_file_type(E7_FILE_TYPE_PolarMap);
        CreateDirectoryStruct();
      }
     for (unsigned short int i=0; i < num; i++)
      e7_matrix_.push_back(new ECAT7_POLAR());      // create polar map matrices
     e7_directory_->AddEntries(num);                 // create directory entries
   }
   catch (...)
    { DeleteMainHeader();
      DeleteMatrices();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create scan matrices.
    \param[in] num   number of new matrices

    Create scan matrices or add matrices to the existing ones. The directory
    is updated accordingly. If the ECAT7 file contains matrices which are not
    scan matrices, they are deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateScanMatrices(const unsigned short int num)
 { try
   {                                         // create main header if necessary
     if (e7_main_header_ == NULL) e7_main_header_ =new ECAT7_MAINHEADER();
                                               // create directory if necessary
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
                       // delete existing matrices if they are no scan matrices
     if (e7_main_header_->HeaderPtr()->file_type != E7_FILE_TYPE_Sinogram)
      { DeleteMatrices();
        Main_file_type(E7_FILE_TYPE_Sinogram);
        CreateDirectoryStruct();
      }
     for (unsigned short int i=0; i < num; i++)
      e7_matrix_.push_back(new ECAT7_SCAN());            // create scan matrices
     e7_directory_->AddEntries(num);                 // create directory entries
   }
   catch (...)
    { DeleteMainHeader();
      DeleteMatrices();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create scan3d matrices.
    \param[in] num   number of new matrices

    Create scan3d matrices or add matrices to the existing ones. The directory
    is updated accordingly. If the ECAT7 file contains matrices which are not
    scan3d matrices, they are deleted.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::CreateScan3DMatrices(const unsigned short int num)
 { try
   { signed short int ft;
                                             // create main header if necessary
     if (e7_main_header_ == NULL) e7_main_header_ =new ECAT7_MAINHEADER();
                                               // create directory if necessary
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
                     // delete existing matrices if they are no scan3d matrices
     ft= e7_main_header_->HeaderPtr()->file_type;
     if ((ft != E7_FILE_TYPE_3D_Sinogram8) &&
         (ft != E7_FILE_TYPE_3D_Sinogram16) &&
         (ft != E7_FILE_TYPE_3D_SinogramFloat))
      { DeleteMatrices();
        Main_file_type(E7_FILE_TYPE_3D_SinogramFloat);
        CreateDirectoryStruct();
      }
     for (unsigned short int i=0; i < num; i++)
      e7_matrix_.push_back(new ECAT7_SCAN3D());        // create scan3d matrices
     e7_directory_->AddEntries(num);                 // create directory entries
   }
   catch (...)
    { DeleteMainHeader();
      DeleteMatrices();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove data from object without deleting.
    \param[in] num   matrix number

    Remove data from object without deleting.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::DataDeleted(const unsigned short int num) const
 { if (e7_matrix_.size() > num) e7_matrix_[num]->DataDeleted();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request data type of data part.
    \param[in] num   matrix number
    \return data type

    Request data type of data part.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7::DataType(const unsigned short int num) const
 { if (e7_matrix_.size() <= num) return(E7_DATATYPE_UNKNOWN);
   return(e7_matrix_[num]->DataType());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete data part of matrix and free memory.
    \param[in] num   matrix number

    Delete data part of matrix and free memory.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::DeleteData(const unsigned short int num) const
 { if (e7_matrix_.size() > num) e7_matrix_[num]->DeleteData();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete directory object.

    Delete directory object.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::DeleteDirectory()
 { if (e7_directory_ != NULL) { delete e7_directory_;
                               e7_directory_ = NULL;
                             }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete matrix objects.

    Delete matrix objects.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::DeleteMatrices()
 { std::vector <ECAT7_MATRIX *>::iterator ma;

   while (e7_matrix_.size() > 0)
    { ma= e7_matrix_.begin();
      delete *ma;                                       // delete matrix object
      e7_matrix_.erase(ma);                // remove pointer to matrix from list
    }
   DeleteDirectory();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete matrix object.
    \param[in] num   matrix number

    Delete matrix object.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::DeleteMatrix(const unsigned short int num)
 { if (e7_matrix_.size() <= num) return;
    std::vector <ECAT7_MATRIX *>::iterator ma;
                                                // search matrix object in list
    ma= e7_matrix_.begin();
    for (unsigned short int i=0; i < num; i++) ma++;
    delete *ma;                                         // delete matrix object
    e7_matrix_.erase(ma);                  // remove pointer to matrix from list
    e7_directory_->DeleteEntry(num);                   // delete directory entry
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete main header object.

    Delete main header object.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::DeleteMainHeader()
 { if (e7_main_header_ != NULL) { delete e7_main_header_;
                                 e7_main_header_ = NULL;
                               }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to directory object.
    \return pointer to directory object

    Request pointer to directory object.
 */
/*---------------------------------------------------------------------------*/
ECAT7_DIRECTORY *ECAT7::Directory() const
 { return(e7_directory_);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load data part of one matrix.
    \param[in] num   matrix number
    \exception REC_ECAT7_MATRIXHEADER_MISSING ECAT7 matrix header is missing
    \exception REC_FILE_DOESNT_EXIST the file doesn't exist
    \exception REC_FILE_TOO_SMALL file doesn't contain enough data

    Load data part of one matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::LoadData(const unsigned short int num)
 { std::ifstream *file=NULL;

   if ((e7_directory_ == NULL) || (e7_matrix_.size() <= num))
    throw Exception(REC_ECAT7_MATRIXHEADER_MISSING,
                    "ECAT7 matrix header is missing.");
   try
   { file=new std::ifstream(in_filename_.c_str(),
                            std::ios::in|std::ios::binary);
     if (!*file)
      throw Exception(REC_FILE_DOESNT_EXIST,
                      "The file '#1' doesn't exist.").arg(in_filename_);
     e7_directory_->SeekMatrixStart(file, num);
     e7_matrix_[num]->LoadData(file, e7_directory_->MatrixRecords(num));
     file->close();
     delete file;
     file=NULL;
   }
   catch (const Exception r)                               // handle exceptions
    { if (file != NULL) { file->close();
                          delete file;
                        }
      if (r.errCode() == REC_FILE_TOO_SMALL)
       throw Exception(REC_FILE_TOO_SMALL,
                       "File '#1' doesn't contain enough data.").
              arg(in_filename_);
      throw;
    }
   catch (...)
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load headers of ECAT7 file.
    \param[in] ifilename   name of ECAT7 file
    \exception REC_FILE_DOESNT_EXIST the file doesn't exist
    \exception REC_UNKNOWN_ECAT7_MATRIXTYPE unknown ECAT7 matrix type

    Load headers of ECAT7 file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::LoadFile(const std::string ifilename)
 { std::ifstream *file=NULL;

   try
   { in_filename_ =ifilename;
     file=new std::ifstream(in_filename_.c_str(),
                            std::ios::in|std::ios::binary);
     if (!*file)
      throw Exception(REC_FILE_DOESNT_EXIST,
                      "The file '#1' doesn't exist.").arg(in_filename_);
     CheckFormat(file);
                                                            // load main header
     DeleteMainHeader();
     e7_main_header_ =new ECAT7_MAINHEADER();
     e7_main_header_->LoadMainHeader(file);
                                                              // load directory
     DeleteMatrices();
     e7_directory_ =new ECAT7_DIRECTORY();
     e7_directory_->LoadDirectory(file);
                                                               // load matrices
     for (unsigned short int i=0; i < e7_directory_->NumberOfMatrices(); i++)
      { e7_directory_->SeekMatrixStart(file, i);
        switch (e7_main_header_->HeaderPtr()->file_type)
         { case E7_FILE_TYPE_Sinogram:
            e7_matrix_.push_back(new ECAT7_SCAN());
            break;
           case E7_FILE_TYPE_AttenuationCorrection:
            e7_matrix_.push_back(new ECAT7_ATTENUATION());
            break;
           case E7_FILE_TYPE_Normalization:
            e7_matrix_.push_back(new ECAT7_NORM());
            break;
           case E7_FILE_TYPE_PolarMap:
            e7_matrix_.push_back(new ECAT7_POLAR());
            break;
           case E7_FILE_TYPE_Image8:
           case E7_FILE_TYPE_Image16:
           case E7_FILE_TYPE_Volume8:
           case E7_FILE_TYPE_Volume16:
            e7_matrix_.push_back(new ECAT7_IMAGE());
            break;
           case E7_FILE_TYPE_3D_Sinogram8:
           case E7_FILE_TYPE_3D_Sinogram16:
           case E7_FILE_TYPE_3D_SinogramFloat:
            e7_matrix_.push_back(new ECAT7_SCAN3D());
            break;
           case E7_FILE_TYPE_3D_Normalization:
            e7_matrix_.push_back(new ECAT7_NORM3D());
            break;
           default:
            throw Exception(REC_UNKNOWN_ECAT7_MATRIXTYPE,
                            "Unknown ECAT7 matrix type '#1'.").
                   arg(e7_main_header_->HeaderPtr()->file_type);
            break;
         }
        e7_matrix_[e7_matrix_.size()-1]->LoadHeader(file);
      }
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (file != NULL) { file->close();
                          delete file;
                        }
      DeleteMatrices();
      DeleteMainHeader();
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to main header object.
    \return pointer to main header object
 */
ECAT7_MAINHEADER *ECAT7::MainHeader() const
 { return(e7_main_header_);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to matrix object.
    \param[in] num   matrix number
    \return pointer to matrix object
 */
ECAT7_MATRIX *ECAT7::Matrix(const unsigned short int num) const
 { if (e7_matrix_.size() <= num) return(NULL);
   return(e7_matrix_[num]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to data part of matrix object.
    \param[in] num   matrix number
    \return pointer to data part of matrix object
 */
void *ECAT7::getecat_matrix::MatrixData(const unsigned short int num) const
 { if (e7_matrix_.size() <= num) return(NULL);
   return(e7_matrix_[num]->getecat_matrix::MatrixData());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Put matrix data into matrix object.
    \param[in] data   pointer to matrix data
    \param[in] dt     data type
    \param[in] num    matrix number
    \exception REC_ECAT7_MATRIXHEADER_MISSING ECAT7 matrix header is missing
 */
void ECAT7::getecat_matrix::MatrixData(void * const data, const unsigned short int dt,
                       const unsigned short int num) const
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num))
    throw Exception(REC_ECAT7_MATRIXHEADER_MISSING,
                    "ECAT7 matrix header is missing.");
   e7_matrix_[num]->getecat_matrix::MatrixData(data, dt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of matrices.
    \return number of matrices
 */
unsigned short int ECAT7::NumberOfMatrices() const
 { if (e7_directory_ != NULL) return(e7_directory_->NumberOfMatrices());
   return(0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print header information into string list.
    \param[in] sl   string list
 */
// ahc a pointer to a std::list wtf
void ECAT7::PrintHeader(std::list <std::string> * const sl) const {
  if ((e7_main_header_ != NULL) && (sl != NULL))    {
    e7_main_header_->PrintMainHeader(sl);
    if (e7_directory_ != NULL)       {
      // std::vector <ECAT7_MATRIX *>::const_iterator ma;
      // unsigned short int mnr = 0;
      e7_directory_->PrintDirectory(sl);
      // for (ma = e7_matrix_.begin(); ma != e7_matrix_.end(); ma++)
      int matrix_index = 0;
      for (auto matrix_element : e7_matrix_)
        (*matrix_element)->PrintHeader(sl, matrix_index++);
    }
  }
}

/*---------------------------------------------------------------------------*/
/*! \brief Store main header and empty directory as ECAT7 file.
    \param[in] fname   name of ECAT7 file
 */
void ECAT7::SaveFile(const std::string fname)
 { std::ofstream *file=NULL;

   if (e7_main_header_ == NULL) return;
   out_filename_ =fname;
   try
   { file=new std::ofstream(out_filename_.c_str(),
                            std::ios::out|std::ios::binary);
     e7_main_header_->SaveMainHeader(file);                 // store main header
     if (e7_directory_ == NULL) e7_directory_ =new ECAT7_DIRECTORY();
     e7_directory_->CreateDirBlock(file, 0, 2);        // create empty directory
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset from ecat_matrix::MatrixDataType::SunShort to ecat_matrix::MatrixDataType::IeeeFloat.
    \param[in] num   matrix number

    Convert dataset from ecat_matrix::MatrixDataType::SunShort to ecat_matrix::MatrixDataType::IeeeFloat.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::Short2Float(const unsigned short int num) const
 { if (e7_matrix_.size() > num) e7_matrix_[num]->Short2Float();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Update main header in ECAT7 file.
    \param[in] filename   name of file

    Update main header in ECAT7 file. The file may already exist.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::UpdateMainHeader(const std::string filename)
 { std::ofstream *file=NULL;

   try
   { if (e7_main_header_ == NULL) return;
     out_filename_ =filename;
     file=new std::ofstream(out_filename_.c_str(),
                            std::ios::out|std::ios::in|std::ios::binary);
     e7_main_header_->SaveMainHeader(file);                 // store main header
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Update subheader in ECAT7 file.
    \param[in] filename   name of file
    \param[in] num        number of matrix

    Update subheader in ECAT7 file. The file may already exist.
 */
/*---------------------------------------------------------------------------*/
void ECAT7::UpdateSubheader(const std::string filename,
                            const unsigned short int num)
 { if (e7_matrix_.size() <= num) return;
   std::ofstream *file=NULL;

   try
   { out_filename_ =filename;
     file=new std::ofstream(out_filename_.c_str(),
                            std::ios::out|std::ios::in|std::ios::binary);
     e7_directory_->SeekMatrixStart(file, num);
     e7_matrix_[num]->SaveHeader(file);                   // store matrix header
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

// --------------------------------- ahc template methods ------------------------------------------
// ahc template classes to explicitly implement methods created by macros below
// The existing code uses eg the #define Attn(var,type) to create setter and getter methods.
// But they require that the data type be the same in the file as it is in memory.
// To used scoped enums (ie data_type member for Attn_x_y method on ECAT7_ATTENUATION subclass):
//   comment out the call to the macro eg:
//     Attn(data_type, signed short int)
//   define scoped setter and getter in this file:
//     ecat_matrix::MatrixDataType ECAT7::Attn_data_type(int t_matrix_index)
//     void ECAT7::Attn_data_type(ecat_matrix::MatrixDataType t_data_type, int t_matrix_index)
//   In the derived class define setter/getter which are called by above methods:
//     ecat_matrix::MatrixDataType ECAT7_ATTENUATION::get_data_type(void)
//     void ECAT7_ATTENUATION::set_data_type(ecat_matrix::MatrixDataType t_data_type)

// Return true if e7_main_header_->HeaderPtr()->file_type is correct for called class.

template <typename T> bool ECAT7::valid_main_header_file_type(void) {
  bool valid = false;
  std::type_info calling_type = typeid(T);

  if (e7_main_header_ ) {
    switch (e7_main_header_->HeaderPtr()->file_type) {
    case E7_FILE_TYPE_Sinogram:
      valid = (calling_type == typeid(ECAT7_SCAN));
      break;
    case E7_FILE_TYPE_AttenuationCorrection:
      valid = (calling_type == typeid(ECAT7_ATTENUATION));
      break;
    case E7_FILE_TYPE_Normalization:
      valid = (calling_type == typeid(ECAT7_NORM));
      break;
    case E7_FILE_TYPE_PolarMap:
      valid = (calling_type == typeid(ECAT7_POLAR));
      break;
    case E7_FILE_TYPE_Image8:
    case E7_FILE_TYPE_Image16:
    case E7_FILE_TYPE_Volume8:
    case E7_FILE_TYPE_Volume16:
      valid = (calling_type == typeid(ECAT7_IMAGE));
      break;
    case E7_FILE_TYPE_3D_Sinogram8:
    case E7_FILE_TYPE_3D_Sinogram16:
    case E7_FILE_TYPE_3D_SinogramFloat:
      valid = (calling_type == typeid(ECAT7_SCAN3D));
      break;
    case E7_FILE_TYPE_3D_Normalization:
      valid = (calling_type == typeid(ECAT7_NORM3D));
      break;
    default:
      LOG_EXIT("Unknown file type {} for class {}", e7_main_header_->HeaderPtr()->file_type, typeid(T).name());
    }
  } else {
    LOG_EXIT("e7_main_header_ NULL");
  }
  return valid;
}

// Return ECAT7_MATRIX-derived object at given index, checking class.  Returns null on error.

template <typename T>T *ECAT7::get_matrix(int t_matrix_index) {
  ECAT7_MATRIX *matrix_element = nullptr;
  if (valid_main_header_file_type<T>())
    matrix_element = e7_matrix_.at(t_matrix_index);  // Exception if out of range
  // std::type_info called_type  = typeid(matrix_element);
  // std::type_info calling_type = typeid(T);
  // if (called_type != calling_type)
  //   LOG_EXIT("Calling type {} on type {}", calling_type.name(), called_type.name());
  return static_cast<T *>(matrix_element);
}

template <typename T> ecat_matrix::MatrixDataType ECAT7::get_matrix_object_data_type(int t_matrix_index) {
  T *matrix_object = get_matrix<T>(t_matrix_index);
    data_type = (matrix_object) ? matrix_object->HeaderPtr()->get_data_type() : ecat_matrix::MatrixDataType::UnknownMatDataType;
  return data_type;
}

template <typename T> void ECAT7::set_matrix_object_data_type(ecat_matrix::MatrixDataType t_data_type, int t_matrix_index ) {
  T *matrix_object = get_matrix<T>(t_matrix_index);
  if (matrix_object)
    matrix_object->HeaderPtr->set_data_type(t_data_type);
}

// --------------------------------- ahc end of template methods ------------------------------------------

// The source code for the manipulation of header values is generated by
// the parser from the following definitions:

/*---------------------------------------------------------------------------*/
/* Attn: request/change value from attenuation header                               */
#define Attn(var, type)\
type ECAT7::Attn_##var(const unsigned short int num) const {\
 if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
   return(0);\
 if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_AttenuationCorrection)\
    return(((ECAT7_ATTENUATION *)e7_matrix_[num])->HeaderPtr()->var);\
 return(0);\
 }\
\
void ECAT7::Attn_##var(const type var, const unsigned short int num) const {\
 if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
  return;\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_AttenuationCorrection)\
    ((ECAT7_ATTENUATION *)e7_matrix_[num])->HeaderPtr()->var = var;\
 }

       // generate code for the following header values of attenuation matrices
// ahc This God-awful code generates same-named setters/getters - 2 methods per macro invocation.
// Indentation and colouring are wrong because these are all #define calls!  FFS.
// Attn(data_type, signed short int)
Attn(num_dimensions, signed short int)
Attn(attenuation_type, signed short int)
Attn(num_r_elements, unsigned short int)
Attn(num_angles, unsigned short int)
Attn(num_z_elements, unsigned short int)
Attn(ring_difference, signed short int)
Attn(x_resolution, float)
Attn(y_resolution, float)
Attn(z_resolution, float)
Attn(w_resolution, float)
Attn(scale_factor, float)
Attn(x_offset, float)
Attn(y_offset, float)
Attn(x_radius, float)
Attn(y_radius, float)
Attn(tilt_angle, float)
Attn(attenuation_coeff, float)
Attn(attenuation_min, float)
Attn(attenuation_max, float)
Attn(skull_thickness, float)
Attn(num_additional_atten_coeff, signed short int)
AttnArray(additional_atten_coeff, float, 8)
Attn(edge_finding_threshold, float)
Attn(storage_order, signed short int)
Attn(span, signed short int)

// ahc these two methods must be written in this file for each derived class.

// Get data_type for ECAT7_ATTENUATION object. Return e7_matrix[indx]->HeaderPtr->data_type as scoped enum for given class.

ecat_matrix::MatrixDataType ECAT7::Attn_data_type(int t_matrix_index) {
  return get_matrix_object_data_type<ECAT7_ATTENUATION>(t_matrix_index);
}

// Set data_type for ECAT7_ATTENUATION object

void ECAT7::Attn_data_type(ecat_matrix::MatrixDataType t_data_type, int t_matrix_index) {
  set_matrix_object_data_type<ECAT7_ATTENUATION>(t_data_type, t_matrix_index);
}

/*---------------------------------------------------------------------------*/
/* AttnArray: request/change value from array in attenuation header                 */
#define AttnArray(var, type, maxidx)\
type ECAT7::Attn_##var(const unsigned short int idx, const unsigned short int num) const {\
  if ((idx >= maxidx) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return(0);\
  if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_AttenuationCorrection)\
    return(((ECAT7_ATTENUATION *)e7_matrix_[num])->HeaderPtr()->var[idx]);\
  return(0);\
 }\
\
void ECAT7::Attn_##var(const type var, const unsigned short int idx, const unsigned short int num) const {\
  if ((idx >= maxidx) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return;\
  if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_AttenuationCorrection)\
    ((ECAT7_ATTENUATION *)e7_matrix_[num])->HeaderPtr()->var[idx]=var;\
 }

AttnArray(z_elements, signed short int, 64)
AttnArray(fill_unused, signed short int, 86)
AttnArray(fill_user, signed short int, 50)

/*---------------------------------------------------------------------------*/
/* Dir: request/change value from directory                                         */
#define Dir(t_var, t_type)\
t_type ECAT7::Dir_##t_var(const unsigned short int idx) const {\
  if (e7_directory_ == NULL)\
    return(0);\
  if (idx >= e7_directory_->NumberOfMatrices())\
    return(0);\
   return(e7_directory_->HeaderPtr()[idx].t_var);
 }\
\
void ECAT7::Dir_##t_var(const t_type t_var, const unsigned short int idx) const\
 { if (e7_directory_ == NULL) return;\
   if (idx >= e7_directory_->NumberOfMatrices()) return;\
   e7_directory_->HeaderPtr()[idx].t_var=t_var;                 /* change value */ \
 }
                       // generate code for the following values of directories
Dir(bed, signed long int)
Dir(frame, signed long int)
Dir(gate, signed long int)
Dir(plane, signed long int)
Dir(data, signed long int)

/*---------------------------------------------------------------------------*/
/* Image: request/change value from image header                                    */
#define Image(t_var, t_type)\
t_type ECAT7::Image_##t_var(const unsigned short int num) const {\
 signed short int ft;\
   if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return(0);\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_Image8) || (ft == E7_FILE_TYPE_Image16) || (ft == E7_FILE_TYPE_Volume8) || (ft == E7_FILE_TYPE_Volume16))\
    return(((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->t_var);\
   return(0);\
 }\
\
void ECAT7::Image_##t_var(const t_type t_var, const unsigned short int num) const {\
 signed short int ft;\
   if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return;\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_Image8) || (ft == E7_FILE_TYPE_Image16) || (ft == E7_FILE_TYPE_Volume8) || (ft == E7_FILE_TYPE_Volume16))\
    ((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->t_var=t_var;
 }

/*---------------------------------------------------------------------------*/
/* ImageArray: request/change value from array in image header                      */
#define ImageArray(t_var, t_type, maxidx)\
t_type ECAT7::Image_##t_var(const unsigned short int idx, const unsigned short int num) const {\
 signed short int ft;\
   if ((idx >= maxidx) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return(0);\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_Image8) || (ft == E7_FILE_TYPE_Image16) || (ft == E7_FILE_TYPE_Volume8) || (ft == E7_FILE_TYPE_Volume16))\
    return(((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->t_var[idx]);\
   return(0);\
 }\
\
void ECAT7::Image_##t_var(const t_type t_var, const unsigned short int idx, const unsigned short int num) const {\
 signed short int ft;\
   if ((idx >= maxidx) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return;\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_Image8) || (ft == E7_FILE_TYPE_Image16) || (ft == E7_FILE_TYPE_Volume8) || (ft == E7_FILE_TYPE_Volume16))\
    ((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->t_var[idx]=t_var;\
 }

/*---------------------------------------------------------------------------*/
/* ImageStr: request/change string from image header                                */
#define ImageStr(var, len)\
unsigned char *ECAT7::Image_##var(const unsigned short int num) const {\
  signed short int ft;\
  if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return(NULL);\
   ft = e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_Image8) || (ft == E7_FILE_TYPE_Image16) || (ft == E7_FILE_TYPE_Volume8) || (ft == E7_FILE_TYPE_Volume16)) {\
     unsigned char *str=NULL;\
      try {\
       str=new unsigned char[strlen((char *)((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->var)+1];\
        return((unsigned char *)strcpy((char *)str, (char *)((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->var));\
      }\
      catch (...) {\
        if (str != NULL)\
          delete[] str;\
        throw;\
       }\
    }\
   return(NULL);\
 }\
\
void ECAT7::Image_##var(unsigned char * const var, const unsigned short int num) const {\
 signed short int ft;\
   if ((var == NULL) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return;\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_Image8) || (ft == E7_FILE_TYPE_Image16) || (ft == E7_FILE_TYPE_Volume8) || (ft == E7_FILE_TYPE_Volume16)) {\
      strncpy((char *)((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->var, (char *)var, len-1);\
      ((ECAT7_IMAGE *)e7_matrix_[num])->HeaderPtr()->var[len-1]=0;\
    }\
 }
             // generate code for the following header values of image matrices
// Image(data_type, signed short int)
// Image(data_type, ecat_matrix::MatrixDataType)
Image(num_dimensions, signed short int)
Image(x_dimension, signed short int)
Image(y_dimension, signed short int)
Image(z_dimension, signed short int)
Image(x_offset, float)
Image(y_offset, float)
Image(z_offset, float)
Image(recon_zoom, float)
Image(scale_factor, float)
Image(image_min, signed short int)
Image(image_max, signed short int)
Image(x_pixel_size, float)
Image(y_pixel_size, float)
Image(z_pixel_size, float)
Image(frame_duration, signed long int)
Image(frame_start_time, signed long int)
Image(filter_code, signed short int)
Image(x_resolution, float)
Image(y_resolution, float)
Image(z_resolution, float)
Image(num_r_elements, float)
Image(num_angles, float)
Image(z_rotation_angle, float)
Image(decay_corr_fctr, float)
Image(processing_code, signed long int)
Image(gate_duration, signed long int)
Image(r_wave_offset, signed long int)
Image(num_accepted_beats, signed long int)
Image(filter_cutoff_frequency, float)
Image(filter_resolution, float)
Image(filter_ramp_slope, float)
Image(filter_order, signed short int)
Image(filter_scatter_fraction, float)
Image(filter_scatter_slope, float)
ImageStr(annotation, 40)
Image(mt_1_1, float)
Image(mt_1_2, float)
Image(mt_1_3, float)
Image(mt_2_1, float)
Image(mt_2_2, float)
Image(mt_2_3, float)
Image(mt_3_1, float)
Image(mt_3_2, float)
Image(mt_3_3, float)
Image(rfilter_cutoff, float)
Image(rfilter_resolution, float)
Image(rfilter_code, signed short int)
Image(rfilter_order, signed short int)
Image(zfilter_cutoff, float)
Image(zfilter_resolution, float)
Image(zfilter_code, signed short int)
Image(zfilter_order, signed short int)
Image(mt_1_4, float)
Image(mt_2_4, float)
Image(mt_3_4, float)
Image(scatter_type, signed short int)
Image(recon_type, signed short int)
Image(recon_views, signed short int)
ImageArray(fill_cti, signed short int, 87)
ImageArray(fill_user, signed short int, 48)

// Explicit definition of methods generated by macro Image(data_type)

ecat_matrix::MatrixDataType ECAT7::Image_data_type(int t_matrix_index) {
  return get_matrix_object_data_type<ECAT7_IMAGE>(t_matrix_index);
}

// Set data_type for ECAT7_IMAGE object

void ECAT7::Image_data_type(ecat_matrix::MatrixDataType t_data_type, int t_matrix_index) {
  set_matrix_object_data_type<ECAT7_IMAGE>(t_data_type, t_matrix_index);
}

/*---------------------------------------------------------------------------*/
/* Main: request/change value from main header                                      */
#define Main(var, t_type)\
t_type ECAT7::Main_##var() const\
 { if (e7_main_header_ == NULL) return(0);\
   return(e7_main_header_->HeaderPtr()->var);
 }\
\
void ECAT7::Main_##var(const t_type var) const\
 { if (e7_main_header_ != NULL)\
    e7_main_header_->HeaderPtr()->var=var;
 }

/*---------------------------------------------------------------------------*/
/* MainArray: request/change value from array in main header                        */
#define MainArray(var, t_type, maxidx)\
t_type ECAT7::Main_##var(const unsigned short int idx) const\
 { if ((idx >= maxidx) || (e7_main_header_ == NULL)) return(0);\
   return(e7_main_header_->HeaderPtr()->var[idx]);
 }\
\
void ECAT7::Main_##var(const t_type var, const unsigned short int idx) const\
 { if ((idx < maxidx) && (e7_main_header_ != NULL))\
    e7_main_header_->HeaderPtr()->var[idx]=var;
 }

/*---------------------------------------------------------------------------*/
/* MainStr: request/change string from main header                                  */
#define MainStr(var, len)\
unsigned char *ECAT7::Main_##var() const {\
 if (e7_main_header_ == NULL)\
  return(NULL);\
  unsigned char *str=NULL;\
   try {\
    str=new unsigned char[strlen((char *)e7_main_header_->HeaderPtr()->var)+1];\
     return((unsigned char *)strcpy((char *)str,(char *)e7_main_header_->HeaderPtr()->var));\
   }\
   catch (...)\
    { if (str != NULL) delete[] str;\
      throw;\
    }\
 }\
\
void ECAT7::Main_##var(unsigned char * const var) const {\
 if ((var != NULL) &&  (e7_main_header_ != NULL)) {\
      strncpy((char *)e7_main_header_->HeaderPtr()->var, (char *)var, len-1);\
      e7_main_header_->HeaderPtr()->var[len-1]=0;\
    }\
 }

MainStr(magic_number, 14)
MainStr(original_file_name, 32)
Main(sw_version, signed short int)
Main(system_type, signed short int)
Main(file_type, signed short int)
MainStr(serial_number, 10)
Main(scan_start_time, signed long int)
MainStr(isotope_name, 8)
Main(isotope_halflife, float)
MainStr(radiopharmaceutical, 32)
Main(gantry_tilt, float)
Main(gantry_rotation, float)
Main(bed_elevation, float)
Main(intrinsic_tilt, float)
Main(wobble_speed, signed short int)
Main(transm_source_type, signed short int)
Main(distance_scanned, float)
Main(transaxial_fov, float)
Main(angular_compression, signed short int)
Main(coin_samp_mode, signed short int)
Main(axial_samp_mode, signed short int)
Main(ecat_calibration_factor, float)
Main(calibration_units, signed short int)
Main(calibration_units_label, signed short int)
Main(compression_code, signed short int)
MainStr(study_type, 12)
MainStr(patient_id, 16)
MainStr(patient_name, 32)
Main(patient_sex, unsigned char)
Main(patient_dexterity, unsigned char)
Main(patient_age, float)
Main(patient_height, float)
Main(patient_weight, float)
Main(patient_birth_date, signed long int)
MainStr(physician_name, 32)
MainStr(operator_name, 32)
MainStr(study_description, 32)
Main(acquisition_type, signed short int)
Main(patient_orientation, signed short int)
MainStr(facility_name, 20)
Main(num_planes, signed short int)
Main(num_frames, signed short int)
Main(num_gates, signed short int)
Main(num_bed_pos, signed short int)
Main(init_bed_position, float)
MainArray(bed_position, float, 15)
Main(plane_separation, float)
Main(lwr_sctr_thres, signed short int)
Main(lwr_true_thres, signed short int)
Main(upr_true_thres, signed short int)
MainStr(user_process_code, 10)
Main(acquisition_mode, signed short int)
Main(bin_size, float)
Main(branching_fraction, float)
Main(dose_start_time, signed long int)
Main(dosage, float)
Main(well_counter_corr_factor, float)
MainStr(data_units, 32)
Main(septa_state, signed short int)
MainArray(fill_cti, signed short int, 6)

/*---------------------------------------------------------------------------*/
/* Norm: request value from norm header                                      */
/*       change value in norm header                                         */
/*---------------------------------------------------------------------------*/
#define Norm(var, type)\
type ECAT7::Norm_##var(const unsigned short int num) const {\
  if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return(0);\
  if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Normalization)\
    return(((ECAT7_NORM *)e7_matrix_[num])->HeaderPtr()->var);\
   return(0);\
 }\
\
void ECAT7::Norm_##var(const type var, const unsigned short int num) const {\
  if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return;\
  if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Normalization)\
    ((ECAT7_NORM *)e7_matrix_[num])->HeaderPtr()->var=var;
 }

/*---------------------------------------------------------------------------*/
/* NormArray: request value from array in norm header                        */
/*            change value in array in norm header                           */
/*---------------------------------------------------------------------------*/
#define NormArray(var, type, maxidx)\
type ECAT7::Norm_##var(const unsigned short int idx, const unsigned short int num) const {\
  if ((idx >= maxidx) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return(0);\
  if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Normalization)\
    return(((ECAT7_NORM *)e7_matrix_[num])->HeaderPtr()->var[idx]);\
   return(0);\
 }\
\
void ECAT7::Norm_##var(const type var, const unsigned short int idx, const unsigned short int num) const {\
  if ((idx >= maxidx) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return;\
  if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Normalization)\
    ((ECAT7_NORM *)e7_matrix_[num])->HeaderPtr()->var[idx]=var;\
 }

// Norm(data_type, signed short int)
Norm(data_type, ecat_matrix::MatrixDataType)
Norm(num_dimensions, signed short int)
Norm(num_r_elements, signed short int)
Norm(num_angles, signed short int)
Norm(num_z_elements, signed short int)
Norm(ring_difference, signed short int)
Norm(scale_factor, float)
Norm(norm_min, float)
Norm(norm_max, float)
Norm(fov_source_width, float)
Norm(norm_quality_factor, float)
Norm(norm_quality_factor_code, signed short int)
Norm(storage_order, signed short int)
Norm(span, signed short int)
NormArray(z_elements, signed short int, 64)
NormArray(fill_cti, signed short int, 123)
NormArray(fill_user, signed short int, 50)

/*---------------------------------------------------------------------------*/
/* Norm3D: request value from norm3d header                                  */
/*         change value in norm3d header                                     */
/*---------------------------------------------------------------------------*/
#define Norm3D(var, type)\
type ECAT7::Norm3D_##var(const unsigned short int num) const\
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return(0);\
   if (e7_main_header_->HeaderPtr()->file_type ==\
       E7_FILE_TYPE_3D_Normalization)\
                                                          /* request value */ \
    return(((ECAT7_NORM3D *)e7_matrix_[num])->HeaderPtr()->var);\
   return(0);\
 }\
\
void ECAT7::Norm3D_##var(const type var, const unsigned short int num) const\
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return;\
   if (e7_main_header_->HeaderPtr()->file_type ==\
       E7_FILE_TYPE_3D_Normalization)\
                                                           /* change value */ \
    ((ECAT7_NORM3D *)e7_matrix_[num])->HeaderPtr()->var=var;\
 }

/*---------------------------------------------------------------------------*/
/* Norm3DArray: request value from array in norm3d header                    */
/*              change value in array in norm3d header                       */
/*---------------------------------------------------------------------------*/
#define Norm3DArray(var, type, maxidx)\
type ECAT7::Norm3D_##var(const unsigned short int idx,\
                         const unsigned short int num) const\
 { if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return(0);\
   if (e7_main_header_->HeaderPtr()->file_type ==\
       E7_FILE_TYPE_3D_Normalization)\
                                                          /* request value */ \
    return(((ECAT7_NORM3D *)e7_matrix_[num])->HeaderPtr()->var[idx]);\
   return(0);\
 }\
\
void ECAT7::Norm3D_##var(const type var, const unsigned short int idx,\
                         const unsigned short int num) const\
 { if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return;\
   if (e7_main_header_->HeaderPtr()->file_type ==\
       E7_FILE_TYPE_3D_Normalization)\
                                                           /* change value */ \
    ((ECAT7_NORM3D *)e7_matrix_[num])->HeaderPtr()->var[idx]=var;\
 }
            // generate code for the following header values of norm3d matrices
// Norm3D(data_type, signed short int)
Norm3D(data_type, ecat_matrix::MatrixDataType)
Norm3D(num_r_elements, signed short int)
Norm3D(num_transaxial_crystals, signed short int)
Norm3D(num_crystal_rings, signed short int)
Norm3D(crystals_per_ring, signed short int)
Norm3D(num_geo_corr_planes, signed short int)
Norm3D(uld, signed short int)
Norm3D(lld, signed short int)
Norm3D(scatter_energy, signed short int)
Norm3D(norm_quality_factor, float)
Norm3D(norm_quality_factor_code, signed short int)
Norm3DArray(ring_dtcor1, float, 32)
Norm3DArray(ring_dtcor2, float, 32)
Norm3DArray(crystal_dtcor, float, 8)
Norm3D(span, signed short int)
Norm3D(max_ring_diff, signed short int)
Norm3DArray(fill_cti, signed short int, 48)
Norm3DArray(fill_user, signed short int, 50)

/*---------------------------------------------------------------------------*/
/* Polar: request value from polar map header                                */
/*        change value in polar map header                                   */
/*---------------------------------------------------------------------------*/
#define Polar(var, type)\
type ECAT7::Polar_##var(const unsigned short int num) const\
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return(0);\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_PolarMap)\
                                                          /* request value */ \
    return(((ECAT7_POLAR *)e7_matrix_[num])->HeaderPtr()->var);\
   return(0);\
 }\
\
void ECAT7::Polar_##var(const type var, const unsigned short int num) const\
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return;\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_PolarMap)\
    ((ECAT7_POLAR *)e7_matrix_[num])->HeaderPtr()->var=var; /* change value */ \
 }

/*---------------------------------------------------------------------------*/
/* PolarArray: request value from array in polar map header                  */
/*             change value in array in polar map header                     */
/*---------------------------------------------------------------------------*/
#define PolarArray(var, type, maxidx)\
type ECAT7::Polar_##var(const unsigned short int idx,\
                        const unsigned short int num) const\
 { if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return(0);\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_PolarMap)\
                                                          /* request value */ \
    return(((ECAT7_POLAR *)e7_matrix_[num])->HeaderPtr()->var[idx]);\
   return(0);\
 }\
\
void ECAT7::Polar_##var(const type var, const unsigned short int idx,\
                        const unsigned short int num) const\
 { if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return;\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_PolarMap)\
                                                           /* change value */ \
    ((ECAT7_POLAR *)e7_matrix_[num])->HeaderPtr()->var[idx]=var;\
 }

/*---------------------------------------------------------------------------*/
/* PolarStr: request string from polar map header                            */
/*           change string in polar map header                               */
/*---------------------------------------------------------------------------*/
#define PolarStr(var, len)\
unsigned char *ECAT7::Polar_##var(const unsigned short int num) const\
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return(NULL);\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_PolarMap)\
    { unsigned char *str=NULL;\
 \
      try\
      { str=new unsigned char[strlen((char *)((ECAT7_POLAR *)e7_matrix_[num])->\
                                             HeaderPtr()->var)+1];\
                                                         /* request value */ \
        return((unsigned char *)strcpy((char *)str,\
                                     (char *)((ECAT7_POLAR *)e7_matrix_[num])->\
                                             HeaderPtr()->var));\
      }\
      catch (...)\
       { if (str != NULL) delete[] str;\
         throw;\
       }\
    }\
   return(NULL);\
 }\
\
void ECAT7::Polar_##var(unsigned char * const var,\
                        const unsigned short int num) const\
 { if ((var == NULL) || (e7_main_header_ == NULL) || (e7_matrix_.size() <= num))\
    return;\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_PolarMap)\
    {                                                      /* change value */ \
      strncpy((char *)((ECAT7_POLAR *)e7_matrix_[num])->HeaderPtr()->var,\
              (char *)var, len-1);\
      ((ECAT7_POLAR *)e7_matrix_[num])->HeaderPtr()->var[len-1]=0;\
    }\
 }
         // generate code for the following header values of polar map matrices
// Polar(data_type, signed short int)
Polar(data_type, ecat_matrix::MatrixDataType)
Polar(polar_map_type, signed short int)
Polar(num_rings, signed short int)
PolarArray(sectors_per_ring, signed short int, 32)
PolarArray(ring_position, float, 32)
PolarArray(ring_angle, signed short int, 32)
Polar(start_angle, signed short int)
PolarArray(long_axis_left, signed short int, 3)
PolarArray(long_axis_right, signed short int, 3)
Polar(position_data, signed short int)
Polar(image_min, signed short int)
Polar(image_max, signed short int)
Polar(scale_factor, float)
Polar(pixel_size, float)
Polar(frame_duration, signed long int)
Polar(frame_start_time, signed long int)
Polar(processing_code, signed short int)
Polar(quant_units, signed short int)
PolarStr(annotation, 40)
Polar(gate_duration, signed long int)
Polar(r_wave_offset, signed long int)
Polar(num_accepted_beats, signed long int)
PolarStr(polar_map_protocol, 20)
PolarStr(database_name, 30)
PolarArray(fill_cti, signed short int, 27)
PolarArray(fill_user, signed short int, 27)

/*---------------------------------------------------------------------------*/
/* Scan: request value from scan header                                      */
/*       change value in scan header                                         */
/*---------------------------------------------------------------------------*/
#define Scan(var, type)\
type ECAT7::Scan_##var(const unsigned short int num) const\
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return(0);\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Sinogram)\
                                                          /* request value */ \
    return(((ECAT7_SCAN *)e7_matrix_[num])->HeaderPtr()->var);\
   return(0);\
 }\
\
void ECAT7::Scan_##var(const type var, const unsigned short int num) const\
 { if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return;\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Sinogram)\
    ((ECAT7_SCAN *)e7_matrix_[num])->HeaderPtr()->var=var;  /* change value */ \
 }

/*---------------------------------------------------------------------------*/
/* Scan: request value from array in scan header                             */
/*       change value in array in scan header                                */
/*---------------------------------------------------------------------------*/
#define ScanArray(var, type, maxidx)\
type ECAT7::Scan_##var(const unsigned short int idx,\
                       const unsigned short int num) const\
 { if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return(0);\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Sinogram)\
                                                          /* request value */ \
    return(((ECAT7_SCAN *)e7_matrix_[num])->HeaderPtr()->var[idx]);\
   return(0);\
 }\
\
void ECAT7::Scan_##var(const type var, const unsigned short int idx,\
                       const unsigned short int num) const\
 { if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return;\
   if (e7_main_header_->HeaderPtr()->file_type == E7_FILE_TYPE_Sinogram)\
                                                           /* change value */ \
    ((ECAT7_SCAN *)e7_matrix_[num])->HeaderPtr()->var[idx]=var;\
 }
              // generate code for the following header values of scan matrices
// Scan(data_type, signed short int)
Scan(data_type, ecat_matrix::MatrixDataType)
Scan(num_dimensions, signed short int)
Scan(num_r_elements, unsigned short int)
Scan(num_angles, unsigned short int)
Scan(corrections_applied, signed short int)
Scan(num_z_elements, unsigned short int)
Scan(ring_difference, signed short int)
Scan(x_resolution, float)
Scan(y_resolution, float)
Scan(z_resolution, float)
Scan(w_resolution, float)
ScanArray(fill_gate, signed short int, 6)
Scan(gate_duration, signed long int)
Scan(r_wave_offset, signed long int)
Scan(num_accepted_beats, signed long int)
Scan(scale_factor, float)
Scan(scan_min, signed short int)
Scan(scan_max, signed short int)
Scan(prompts, signed long int)
Scan(delayed, signed long int)
Scan(multiples, signed long int)
Scan(net_trues, signed long int)
ScanArray(cor_singles, float, 16)
ScanArray(uncor_singles, float, 16)
Scan(tot_avg_cor, float)
Scan(tot_avg_uncor, float)
Scan(total_coin_rate, signed long int)
Scan(frame_start_time, signed long int)
Scan(frame_duration, signed long int)
Scan(deadtime_correction_factor, float)
ScanArray(physical_planes, signed short int, 8)
ScanArray(fill_cti, signed short int, 83)
ScanArray(fill_user, signed short int, 50)

/*---------------------------------------------------------------------------*/
/* Scan3D: request value from scan3d header                                  */
/*         change value in scan3d header                                     */
/*---------------------------------------------------------------------------*/
#define Scan3D(var, type)\
type ECAT7::Scan3D_##var(const unsigned short int num) const\
 { signed short int ft;\
   \
   if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return(0);\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_3D_Sinogram8) ||\
       (ft == E7_FILE_TYPE_3D_Sinogram16) ||\
       (ft == E7_FILE_TYPE_3D_SinogramFloat))\
                                                          /* request value */ \
    return(((ECAT7_SCAN3D *)e7_matrix_[num])->HeaderPtr()->var);\
   return(0);\
 }\
\
void ECAT7::Scan3D_##var(const type var, const unsigned short int num) const\
 { signed short int ft;\
   \
   if ((e7_main_header_ == NULL) || (e7_matrix_.size() <= num)) return;\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_3D_Sinogram8) ||\
       (ft == E7_FILE_TYPE_3D_Sinogram16) ||\
       (ft == E7_FILE_TYPE_3D_SinogramFloat))\
    ((ECAT7_SCAN3D *)e7_matrix_[num])->HeaderPtr()->var=var;/* change value */ \
 }

/*---------------------------------------------------------------------------*/
/* Scan3D: request value from array in scan3d header                         */
/*         change value in array in scan3d header                            */
/*---------------------------------------------------------------------------*/
#define Scan3DArray(var, type, maxidx)\
type ECAT7::Scan3D_##var(const unsigned short int idx,\
                         const unsigned short int num) const\
 { signed short int ft;\
   \
   if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return(0);\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_3D_Sinogram8) ||\
       (ft == E7_FILE_TYPE_3D_Sinogram16) ||\
       (ft == E7_FILE_TYPE_3D_SinogramFloat))\
                                                          /* request value */ \
    return(((ECAT7_SCAN3D *)e7_matrix_[num])->HeaderPtr()->var[idx]);\
   return(0);\
 }\
\
void ECAT7::Scan3D_##var(const type var, const unsigned short int idx,\
                         const unsigned short int num) const\
 { signed short int ft;\
   \
   if ((idx >= maxidx) || (e7_main_header_ == NULL) ||\
       (e7_matrix_.size() <= num)) return;\
   ft= e7_main_header_->HeaderPtr()->file_type;\
   if ((ft == E7_FILE_TYPE_3D_Sinogram8) ||\
       (ft == E7_FILE_TYPE_3D_Sinogram16) ||\
       (ft == E7_FILE_TYPE_3D_SinogramFloat))\
                                                           /* change value */ \
    ((ECAT7_SCAN3D *)e7_matrix_[num])->HeaderPtr()->var[idx]=var;\
 }
            // generate code for the following header values of scan3d matrices
// Scan3D(data_type, signed short int)
Scan3D(data_type, ecat_matrix::MatrixDataType)
Scan3D(num_dimensions, signed short int)
Scan3D(num_r_elements, unsigned short int)
Scan3D(num_angles, unsigned short int)
Scan3D(corrections_applied, signed short int)
Scan3DArray(num_z_elements, unsigned short int, 64)
Scan3D(ring_difference, signed short int)
Scan3D(storage_order, signed short int)
Scan3D(axial_compression, signed short int)
Scan3D(x_resolution, float)
Scan3D(v_resolution, float)
Scan3D(z_resolution, float)
Scan3D(w_resolution, float)
Scan3DArray(fill_gate, signed short int, 6)
Scan3D(gate_duration, signed long int)
Scan3D(r_wave_offset, signed long int)
Scan3D(num_accepted_beats, signed long int)
Scan3D(scale_factor, float)
Scan3D(scan_min, signed short int)
Scan3D(scan_max, signed short int)
Scan3D(prompts, signed long int)
Scan3D(delayed, signed long int)
Scan3D(multiples, signed long int)
Scan3D(net_trues, signed long int)
Scan3D(tot_avg_cor, float)
Scan3D(tot_avg_uncor, float)
Scan3D(total_coin_rate, signed long int)
Scan3D(frame_start_time, signed long int)
Scan3D(frame_duration, signed long int)
Scan3D(deadtime_correction_factor, float)
Scan3DArray(fill_cti, signed short int, 90)
Scan3DArray(fill_user, signed short int, 50)
Scan3DArray(uncor_singles, float, 128)

// ahc utility routines

// Return a string describing the data type field, for use in print_header() in derived classes

std::string ECAT7::print_header_data_type(ecat_matrix::MatrixDataType t_data_type) {
  return fmt::format(" data_type:                      {} ({})", to_underlying(t_data_type), ecat_matrix::matrix_data_types_.at(t_data_type).name);
}

     // s=
     // switch (image_subheader_.data_type)
     //  { case E7_DATA_TYPE_UnknownMatDataType:
     //     s+="UnknownMatDataType";
     //     break;
     //    case E7_DATA_TYPE_ByteData:
     //     s+="ByteData";
     //     break;
     //    case E7_DATA_TYPE_VAX_Ix2:
     //     s+="VAX_Ix2";
     //     break;
     //    case E7_DATA_TYPE_VAX_Ix4:
     //     s+="VAX_Ix4";
     //     break;
     //    case E7_DATA_TYPE_VAX_Rx4:
     //     s+="VAX_Rx4";
     //     break;
     //    case E7_DATA_TYPE_IeeeFloat:
     //     s+="IeeeFloat";
     //     break;
     //    case E7_DATA_TYPE_SunShort:
     //     s+="SunShort";
     //     break;
     //    case E7_DATA_TYPE_SunLong:
     //     s+="SunLong";
     //     break;
     //    default:
     //     s+="unknown";
     //     break;
     //  }
     // sl->push_back(s+")");


