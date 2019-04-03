/*! \class ECAT7_ATTENUATION ecat7_attenuation.h "ecat7_attenuation.h"
    \brief This class implements the ATTENUATION matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2004/09/22 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)
 
    This class is build on top of the ECAT7_MATRIX abstract base class and the
    ecat7_utils module and adds a few things specific to ATTENUATION matrices,
    like loading, storing and printing the header.
 */

#include <cstdio>
#include "ecat7_attenuation.hpp"
#include "data_changer.h"
#include "ecat7_global.h"
#include "ecat7_utils.h"
#include "fastmath.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

 */
ECAT7_ATTENUATION::ECAT7_ATTENUATION()
 { memset(&attenuation_subheader_, 0, sizeof(tattenuation_subheader));           // header is empty
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content
 
    This operator copies the attenuation header and uses the copy operator of
    the base class to copy the dataset from another object into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_ATTENUATION& ECAT7_ATTENUATION::operator = (const ECAT7_ATTENUATION &e7)
 { if (this != &e7)
    {                                // copy header information into new object
      memcpy(&attenuation_subheader_, &e7.attenuation_subheader_, sizeof(tattenuation_subheader));
      ECAT7_MATRIX:: operator = (e7);         // call copy operator from parent
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Cut bins on both sides of projections.
    \param[in] cut   number of bins to cut on each side
 
    The given number of bins is cut off on each side of a projection.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::CutBins(const unsigned short int cut) { 
  utils_CutBins(&data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles, Depth(), cut);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header

 */
unsigned short int ECAT7_ATTENUATION::DataTypeOrig() const
 { return(attenuation_subheader_.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Deinterleave dataset.

    Deinterleave dataset. The number of angular projections will be doubled
    and the number of bins per projection halved.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::DeInterleave() { 
  utils_DeInterleave(data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles, Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of slices in sinogram.
    \return number of slices in sinogram
 
 */
unsigned short int ECAT7_ATTENUATION::Depth() const
 { unsigned short int sum, i;

   for (sum=0,
        i=0; i < 64; i++)                         // calculate number of slices
    if (attenuation_subheader_.z_elements[i] == 0) break;
     else sum+=attenuation_subheader_.z_elements[i];
   return(sum);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between feet first and head first.

    Change orientation between feet first amd head first. The dataset is
    flipped in z-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::Feet2Head() const
 { utils_Feet2Head(data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles,
                   (unsigned short int *)attenuation_subheader_.z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
    Request pointer to header data structure.
 */
/*---------------------------------------------------------------------------*/
ECAT7_ATTENUATION::tattenuation_subheader *ECAT7_ATTENUATION::HeaderPtr()
 { return(&attenuation_subheader_);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of angular projections in sinogram.
    \return number of angular projections in sinogram
 */
unsigned short int ECAT7_ATTENUATION::Height() const
 { return(attenuation_subheader_.num_angles);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Interleave dataset.
    Interleave dataset. The number of angular projections will be halved
    and the number of bins per projection doubled.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::Interleave()
 { utils_Interleave(data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles, Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load dataset of matrix into object.
    \param[in] file             handle of file
    \param[in] matrix_records   number of records to read
    \return pointer to dataset
 
    Load dataset of matrix into object. The header of the matrix, which is one
    record long, is skipped.
 */
/*---------------------------------------------------------------------------*/
void *ECAT7_ATTENUATION::LoadData(std::ifstream * const file,
                                  const unsigned long int matrix_records)
 { file->seekg(E7_RECLEN, std::ios::cur);                 // skip matrix header
   return(ECAT7_MATRIX::LoadData(file, matrix_records-1));         // load data
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of attenuation matrix.
    \param[in] file   handle of file
 */
void ECAT7_ATTENUATION::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     // dc = new DataChanger(E7_RECLEN);
     dc=new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&attenuation_subheader_.data_type);
     dc->Value(&attenuation_subheader_.num_dimensions);
     dc->Value(&attenuation_subheader_.attenuation_type);
     dc->Value(&attenuation_subheader_.num_r_elements);
     dc->Value(&attenuation_subheader_.num_angles);
     dc->Value(&attenuation_subheader_.num_z_elements);
     dc->Value(&attenuation_subheader_.ring_difference);
     dc->Value(&attenuation_subheader_.x_resolution);
     dc->Value(&attenuation_subheader_.y_resolution);
     dc->Value(&attenuation_subheader_.z_resolution);
     dc->Value(&attenuation_subheader_.w_resolution);
     dc->Value(&attenuation_subheader_.scale_factor);
     dc->Value(&attenuation_subheader_.x_offset);
     dc->Value(&attenuation_subheader_.y_offset);
     dc->Value(&attenuation_subheader_.x_radius);
     dc->Value(&attenuation_subheader_.y_radius);
     dc->Value(&attenuation_subheader_.tilt_angle);
     dc->Value(&attenuation_subheader_.attenuation_coeff);
     dc->Value(&attenuation_subheader_.attenuation_min);
     dc->Value(&attenuation_subheader_.attenuation_max);
     dc->Value(&attenuation_subheader_.skull_thickness);
     dc->Value(&attenuation_subheader_.num_additional_atten_coeff);
     for (i=0; i < 8; i++) dc->Value(&attenuation_subheader_.additional_atten_coeff[i]);
     dc->Value(&attenuation_subheader_.edge_finding_threshold);
     dc->Value(&attenuation_subheader_.storage_order);
     dc->Value(&attenuation_subheader_.span);
     for (i=0; i < 64; i++) dc->Value(&attenuation_subheader_.z_elements[i]);
     for (i=0; i < 86; i++) dc->Value(&attenuation_subheader_.fill_unused[i]);
     for (i=0; i < 50; i++) dc->Value(&attenuation_subheader_.fill_user[i]);
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

 */
void ECAT7_ATTENUATION::PrintHeader(std::list <std::string> * const sl, const unsigned short int num) const { 
  unsigned short int i;
   std::string s;

   sl->push_back("******* Attenuation-Matrix-Header ("+toString(num, 2)+                 ") ********");
   // s=" data_type:                   "+toString(attenuation_subheader_.data_type)+" (";
   //  switch (attenuation_subheader_.data_type)
   //   { case E7_DATA_TYPE_UnknownMatDataType:
   //      s+="UnknownMatDataType";
   //      break;
   //     case E7_DATA_TYPE_ByteData:
   //      s+="ByteData";
   //      break;
   //     case E7_DATA_TYPE_VAX_Ix2:
   //      s+="VAX_Ix2";
   //      break;
   //     case E7_DATA_TYPE_VAX_Ix4:
   //      s+="VAX_Ix4";
   //      break;
   //     case E7_DATA_TYPE_VAX_Rx4:
   //      s+="VAX_Rx4";
   //      break;
   //     case E7_DATA_TYPE_IeeeFloat:
   //      s+="IeeeFloat";
   //      break;
   //     case E7_DATA_TYPE_SunShort:
   //      s+="SunShort";
   //      break;
   //     case E7_DATA_TYPE_SunLong:
   //      s+="SunLong";
   //      break;
   //     default:
   //      s+="unknown";
   //      break;
   //   }
   // sl->push_back(s+")");
     sl->push_back(print_header_data_type(attenuation_subheader_.data_type));
   sl->push_back(" Number of Dimensions:        "+toString(attenuation_subheader_.num_dimensions));
   s=" Attenuation-type:            "+toString(attenuation_subheader_.attenuation_type)+" (";
   switch (attenuation_subheader_.attenuation_type)
    { case E7_ATTENUATION_TYPE_AttenNone: s+="none";       break;
      case E7_ATTENUATION_TYPE_AttenMeas: s+="measured";   break;
      case E7_ATTENUATION_TYPE_AttenCalc: s+="calculated"; break;
      default:                            s+="unknown";    break;
    }
   sl->push_back(s+")");
   sl->push_back(" Dimension 1 (ring elements): "+toString(attenuation_subheader_.num_r_elements));
   sl->push_back(" Dimension 2 (angles):        "+toString(attenuation_subheader_.num_angles));
   sl->push_back(" num_z_elements:              "+toString(attenuation_subheader_.num_z_elements));
   sl->push_back(" ring_difference:             "+                 toString(attenuation_subheader_.ring_difference));
   sl->push_back(" x_resolution:                "+toString(attenuation_subheader_.x_resolution)+                 " cm");
   sl->push_back(" y_resolution:                "+toString(attenuation_subheader_.y_resolution)+                 " cm");
   sl->push_back(" z_resolution:                "+toString(attenuation_subheader_.z_resolution)+                 " cm");
   sl->push_back(" w_resolution:                "+toString(attenuation_subheader_.w_resolution));
   sl->push_back(" scale_factor:                "+toString(attenuation_subheader_.scale_factor));
   sl->push_back(" x_offset:                    "+toString(attenuation_subheader_.x_offset)+" cm");
   sl->push_back(" y_offset:                    "+toString(attenuation_subheader_.y_offset)+" cm");
   sl->push_back(" x_radius:                    "+toString(attenuation_subheader_.x_radius)+" cm");
   sl->push_back(" y_radius:                    "+toString(attenuation_subheader_.y_radius)+" cm");
   sl->push_back(" tilt_angle:                  "+toString(attenuation_subheader_.tilt_angle)+
                 " deg.");
   sl->push_back(" Attenuation Coefficient:     "+                 toString(attenuation_subheader_.attenuation_coeff)+" 1/cm");
   sl->push_back(" attenuation_min:             "+                 toString(attenuation_subheader_.attenuation_min));
   sl->push_back(" attenuation_max:             "+                 toString(attenuation_subheader_.attenuation_max));
   sl->push_back(" skull_thickness:             "+                 toString(attenuation_subheader_.skull_thickness)+" cm");
   sl->push_back(" num_additional_atten_coeff:  "+                 toString(attenuation_subheader_.num_additional_atten_coeff));
   for (i=0; i < 8; i++)
    sl->push_back(" additional_atten_coeff["+toString(i)+"]:   "+                  toString(attenuation_subheader_.additional_atten_coeff[i]));
   sl->push_back(" edge_finding_threshold:      "+                 toString(attenuation_subheader_.edge_finding_threshold));
   s=" storage order:               "+toString(attenuation_subheader_.storage_order)+" (";
   switch (attenuation_subheader_.storage_order)
    { case E7_STORAGE_ORDER_RTZD:  s+="r, theta, z, d"; break;
      case E7_STORAGE_ORDER_RZTD:  s+="r, z, theta, d"; break;
      default:                     s+="none";           break;
    }
   sl->push_back(s+")");
   sl->push_back(" span:                        "+toString(attenuation_subheader_.span));
   s=" z_elements:                  "+toString(attenuation_subheader_.z_elements[0]);
   for (i=1; attenuation_subheader_.z_elements[i] != 0; i++) s+="+"+toString(attenuation_subheader_.z_elements[i]);
   sl->push_back(s);
   /*
   for (i=0; i < 86; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                 "+
                  toString(attenuation_subheader_.fill_cti[i]));
   for (i=0; i < 50; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                 "+
                  toString(attenuation_subheader_.fill_user[i]));
   */
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between prone and supine.

    Change orientation between prone and supine. The dataset is flipped in
    y-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::Prone2Supine() const
 { utils_Prone2Supine(data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles,
                      Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rotate sinogram counterclockwise along scanned axis.
    \param[in] rt   number of steps to rotate (each step is 180/#angles degrees)

    Rotate sinogram counterclockwise along scanned axis.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::Rotate(const signed short int rt) const
 { utils_Rotate(data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles, Depth(), rt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of attenuation matrix.
    \param[in] file   handle of file

    Store header of attenuation matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                      // DataChanger is used to store data system independently
     dc = new DataChanger(E7_RECLEN);
                                                       // fill data into buffer
     dc->Value(attenuation_subheader_.data_type);
     dc->Value(attenuation_subheader_.num_dimensions);
     dc->Value(attenuation_subheader_.attenuation_type);
     dc->Value(attenuation_subheader_.num_r_elements);
     dc->Value(attenuation_subheader_.num_angles);
     dc->Value(attenuation_subheader_.num_z_elements);
     dc->Value(attenuation_subheader_.ring_difference);
     dc->Value(attenuation_subheader_.x_resolution);
     dc->Value(attenuation_subheader_.y_resolution);
     dc->Value(attenuation_subheader_.z_resolution);
     dc->Value(attenuation_subheader_.w_resolution);
     dc->Value(attenuation_subheader_.scale_factor);
     dc->Value(attenuation_subheader_.x_offset);
     dc->Value(attenuation_subheader_.y_offset);
     dc->Value(attenuation_subheader_.x_radius);
     dc->Value(attenuation_subheader_.y_radius);
     dc->Value(attenuation_subheader_.tilt_angle);
     dc->Value(attenuation_subheader_.attenuation_coeff);
     dc->Value(attenuation_subheader_.attenuation_min);
     dc->Value(attenuation_subheader_.attenuation_max);
     dc->Value(attenuation_subheader_.skull_thickness);
     dc->Value(attenuation_subheader_.num_additional_atten_coeff);
     for (i=0; i < 8; i++) dc->Value(attenuation_subheader_.additional_atten_coeff[i]);
     dc->Value(attenuation_subheader_.edge_finding_threshold);
     dc->Value(attenuation_subheader_.storage_order);
     dc->Value(attenuation_subheader_.span);
     for (i=0; i < 64; i++) dc->Value(attenuation_subheader_.z_elements[i]);
     for (i=0; i < 86; i++) dc->Value(attenuation_subheader_.fill_unused[i]);
     for (i=0; i < 50; i++) dc->Value(attenuation_subheader_.fill_user[i]);
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
/*! \brief Scale sinogram.
    \param[in] factor   scaling factor
 
 */
void ECAT7_ATTENUATION::ScaleMatrix(const float factor)
 { utils_ScaleMatrix(data, datatype, (unsigned long int)attenuation_subheader_.num_r_elements*
                                     (unsigned long int)attenuation_subheader_.num_angles*
                                     (unsigned long int)Depth(), factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert transmission data into emission data.

    Convert transmission data into emission data by taking the logarithm.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_ATTENUATION::Tra2Emi() const
 { if ((datatype != E7_DATATYPE_FLOAT) || (data == NULL)) return;
   float *fp;

   fp=(float *)data;
   for (unsigned long int xyz=0;
        xyz < (unsigned long int)attenuation_subheader_.num_r_elements*
              (unsigned long int)attenuation_subheader_.num_angles*
              (unsigned long int)Depth();
        xyz++)
    if (fp[xyz] > 1.0f) fp[xyz]=logf(fp[xyz]);
     else fp[xyz]=0.0f;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert sinogram from view to volume format (rztd to rtzd).

 */
void ECAT7_ATTENUATION::View2Volume() const
 { utils_View2Volume(data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles,
                     (unsigned short int *)attenuation_subheader_.z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert sinogram from volume to view format (rtzd to rztd).

 */
void ECAT7_ATTENUATION::Volume2View() const
 { utils_Volume2View(data, datatype, attenuation_subheader_.num_r_elements, attenuation_subheader_.num_angles,
                     (unsigned short int *)attenuation_subheader_.z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bins in a projection.
    \return number of bins in a projection
 
 */
unsigned short int ECAT7_ATTENUATION::Width() const
 { return(attenuation_subheader_.num_r_elements);
 }

// attenuation_subheader_c

// Read data_type as short int from file, return as scoped enum

MatrixData::DataType ECAT7_ATTENUATION::get_data_type(void) {
  return static_cast<MatrixData::DataType>(attenuation_subheader_.data_type);
}

// Write scoped enum to file as short int

void ECAT7_ATTENUATION::set_data_type(MatrixData::DataType t_data_type) {
  attenuation_subheader_.data_type = static_cast<signed short int>(t_data_type);
}
