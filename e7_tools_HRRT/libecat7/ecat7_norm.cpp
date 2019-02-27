/*! \class ECAT7_NORM ecat7_norm.h "ecat7_norm.h"
    \brief This class implements the NORM matrix type for ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class is build on top of the ECAT7_MATRIX abstract base class and the
    ecat7_utils module and adds a few things specific to NORM matrices, like
    loading, storing and printing the header.
 */

#include <cstdio>
#include "ecat7_norm.h"
#include "data_changer.h"
#include "ecat7_global.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object and fill header data structure with 0s.
 */
/*---------------------------------------------------------------------------*/
ECAT7_NORM::ECAT7_NORM()
 { memset(&nh, 0, sizeof(tnorm_subheader));                  // header is empty
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content

    This operator copies the norm header and uses the copy operator of the
    base class to copy the dataset from another objects into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_NORM& ECAT7_NORM::operator = (const ECAT7_NORM &e7)
 { if (this != &e7)
    {                                // copy header information into new object
      memcpy(&nh, &e7.nh, sizeof(tnorm_subheader));
      ECAT7_MATRIX:: operator = (e7);         // call copy operator from parent
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header

    Request datatype from matrix header.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_NORM::DataTypeOrig() const
 { return(nh.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of slices in sinogram.
    \return number of slices in sinogram
 
    Request number of slices in sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_NORM::Depth() const
 { return(nh.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
    Request pointer to header data structure.
 */
/*---------------------------------------------------------------------------*/
ECAT7_NORM::tnorm_subheader *ECAT7_NORM::HeaderPtr()
 { return(&nh);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of angular projections in sinogram.
    \return number of angular projections in sinogram
 
    Request number of angular projections in sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_NORM::Height() const
 { return(nh.num_angles);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load dataset of matrix into object.
    \param[in] file             handle of file
    \param[in] matrix_records   number of records to read
    \return pointer to dataset
 
    Load dataset of matrix into object. The header of the matrix, which is two
    records long, is skipped.
 */
/*---------------------------------------------------------------------------*/
void *ECAT7_NORM::LoadData(std::ifstream * const file,
                           const unsigned long int matrix_records)
 { file->seekg(E7_RECLEN, std::ios::cur);                 // skip matrix header
   return(ECAT7_MATRIX::LoadData(file, matrix_records-1));         // load data
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of norm matrix.
    \param[in] file   handle of file

    Load header of norm matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                      // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&nh.data_type);
     dc->Value(&nh.num_dimensions);
     dc->Value(&nh.num_r_elements);
     dc->Value(&nh.num_angles);
     dc->Value(&nh.num_z_elements);
     dc->Value(&nh.ring_difference);
     dc->Value(&nh.scale_factor);
     dc->Value(&nh.norm_min);
     dc->Value(&nh.norm_max);
     dc->Value(&nh.fov_source_width);
     dc->Value(&nh.norm_quality_factor);
     dc->Value(&nh.norm_quality_factor_code);
     dc->Value(&nh.storage_order);
     dc->Value(&nh.span);
     for (i=0; i < 64; i++) dc->Value(&nh.z_elements[i]);
     for (i=0; i < 123; i++) dc->Value(&nh.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(&nh.fill_user[i]);
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print header information into string list.
    \param[in] sl    string list
    \param[in] num   matrix number

    Print header information into string list.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM::PrintHeader(std::list <std::string> * const sl,
                             const unsigned short int num) const
 { unsigned short int i;
   std::string s;

   sl->push_back("************** Norm-Matrix ("+toString(num, 2)+
                 ") **************");
   s=" data_type:                      "+toString(nh.data_type)+" (";
   switch (nh.data_type)
    { case E7_DATA_TYPE_UnknownMatDataType:
       s+="UnknownMatDataType";
       break;
      case E7_DATA_TYPE_ByteData:
       s+="ByteData";
       break;
      case E7_DATA_TYPE_VAX_Ix2:
       s+="VAX_Ix2";
       break;
      case E7_DATA_TYPE_VAX_Ix4:
       s+="VAX_Ix4";
       break;
      case E7_DATA_TYPE_VAX_Rx4:
       s+="VAX_Rx4";
       break;
      case E7_DATA_TYPE_IeeeFloat:
       s+="IeeeFloat";
       break;
      case E7_DATA_TYPE_SunShort:
       s+="SunShort";
       break;
      case E7_DATA_TYPE_SunLong:
       s+="SunLong";
       break;
      default:
       s+="unknown";
       break;
    }
   sl->push_back(s+")");
   sl->push_back(" num_dimensions:                 " + toString(nh.num_dimensions));
   sl->push_back(" num_r_elements:                 " + toString(nh.num_r_elements));
   sl->push_back(" num_angles:                     " + toString(nh.num_angles));
   sl->push_back(" num_z_elements:                 " + toString(nh.num_z_elements));
   sl->push_back(" ring_difference:                " + toString(nh.ring_difference));
   sl->push_back(" scale_factor:                   " + toString(nh.scale_factor));
   sl->push_back(" norm_min:                       " + toString(nh.norm_min));
   sl->push_back(" norm_max:                       " + toString(nh.norm_max));
   sl->push_back(" fov_source_width:               " + toString(nh.fov_source_width)+" cm");
   sl->push_back(" norm quality factor:            " + toString(nh.norm_quality_factor));
   sl->push_back(" norm quality factor code:       " + toString(nh.norm_quality_factor_code));
   s=" storage order:                  "+toString(nh.storage_order)+" (";
   switch (nh.storage_order)
    { case E7_STORAGE_ORDER_RTZD:  s+="r, theta, z, d"; break;
      case E7_STORAGE_ORDER_RZTD:  s+="r, z, theta, d"; break;
      default:                     s+="none";           break;
    }
   sl->push_back(s);
   sl->push_back(" span:                           "+toString(nh.span));
   sl->push_back(" z_elements:                     ");
   s=std::string();
   for (i=0; i < 64; i++)
    { if (nh.z_elements[i] == 0) break;
      s+=toString(nh.z_elements[i], 4);
    }
   if (s.length() > 0) sl->push_back(s);
   /*
   for (i=0; i < 123; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(nh.fill_cti[i]));
   for (i=0; i < 50; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(nh.fill_user[i]));
   */
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of norm matrix.
    \param[in] file   handle of file

    Store header of norm matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                      // DataChanger is used to store data system independently
     dc = new DataChanger(E7_RECLEN);
                                                       // fill data into buffer
     dc->Value(nh.data_type);
     dc->Value(nh.num_dimensions);
     dc->Value(nh.num_r_elements);
     dc->Value(nh.num_angles);
     dc->Value(nh.num_z_elements);
     dc->Value(nh.ring_difference);
     dc->Value(nh.scale_factor);
     dc->Value(nh.norm_min);
     dc->Value(nh.norm_max);
     dc->Value(nh.fov_source_width);
     dc->Value(nh.norm_quality_factor);
     dc->Value(nh.norm_quality_factor_code);
     dc->Value(nh.storage_order);
     dc->Value(nh.span);
     for (i=0; i < 64; i++) dc->Value(nh.z_elements[i]);
     for (i=0; i < 123; i++) dc->Value(nh.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(nh.fill_user[i]);
     dc->SaveBuffer(file);                                      // store header
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bins in a projection.
    \return number of bins in a projection
 
    Request number of bins in a projection.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_NORM::Width() const
 { return(nh.num_r_elements);
 }
