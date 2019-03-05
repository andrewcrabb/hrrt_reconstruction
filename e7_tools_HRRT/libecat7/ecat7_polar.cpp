/*! \class ECAT7_POLAR ecat7_polar.hpp "ecat7_polar.hpp"
    \brief This class implements the polar map matrix type for ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class is build on top of the ECAT7_MATRIX abstract base class and the
    ecat7_utils module and adds a few things specific to polar matrices, like
    loading, storing and printing the header.
 */

#include <cstdio>
#include "ecat7_polar.hpp"
#include "data_changer.h"
#include "ecat7_global.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
 */
ECAT7_POLAR::ECAT7_POLAR()
 { memset(&polar_subheader_, 0, sizeof(tpolar_subheader));                 // header is empty
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content

    copies the polar header and uses copy operator of base class to copy dataset from another objects into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_POLAR& ECAT7_POLAR::operator = (const ECAT7_POLAR &e7)
 { if (this != &e7)
    {                                // copy header information into new object
      memcpy(&polar_subheader_, &e7.polar_subheader_, sizeof(tpolar_subheader));
      ECAT7_MATRIX:: operator = (e7);         // call copy operator from parent
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header

 */
unsigned short int ECAT7_POLAR::DataTypeOrig() const
 { return(polar_subheader_.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
 */
ECAT7_POLAR::tpolar_subheader *ECAT7_POLAR::HeaderPtr()
 { return(&polar_subheader_);
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
void *ECAT7_POLAR::LoadData(std::ifstream * const file,
                            const unsigned long int matrix_records)
 { file->seekg(E7_RECLEN, std::ios::cur);                 // skip matrix header
   return(ECAT7_MATRIX::LoadData(file, matrix_records-1));         // load data
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of polar matrix.
    \param[in] file   handle of file

 */
void ECAT7_POLAR::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&polar_subheader_.data_type);
     dc->Value(&polar_subheader_.polar_map_type);
     dc->Value(&polar_subheader_.num_rings);
     for (i=0; i < 32; i++) dc->Value(&polar_subheader_.sectors_per_ring[i]);
     for (i=0; i < 32; i++) dc->Value(&polar_subheader_.ring_position[i]);
     for (i=0; i < 32; i++) dc->Value(&polar_subheader_.ring_angle[i]);
     dc->Value(&polar_subheader_.start_angle);
     for (i=0; i < 3; i++) dc->Value(&polar_subheader_.long_axis_left[i]);
     for (i=0; i < 3; i++) dc->Value(&polar_subheader_.long_axis_right[i]);
     dc->Value(&polar_subheader_.position_data);
     dc->Value(&polar_subheader_.image_min);
     dc->Value(&polar_subheader_.image_max);
     dc->Value(&polar_subheader_.scale_factor);
     dc->Value(&polar_subheader_.pixel_size);
     dc->Value(&polar_subheader_.frame_duration);
     dc->Value(&polar_subheader_.frame_start_time);
     dc->Value(&polar_subheader_.processing_code);
     dc->Value(&polar_subheader_.quant_units);
     dc->Value(polar_subheader_.annotation, 40);
     dc->Value(&polar_subheader_.gate_duration);
     dc->Value(&polar_subheader_.r_wave_offset);
     dc->Value(&polar_subheader_.num_accepted_beats);
     dc->Value(polar_subheader_.polar_map_protocol, 20);
     dc->Value(polar_subheader_.database_name, 30);
     for (i=0; i < 27; i++) dc->Value(&polar_subheader_.fill_cti[i]);
     for (i=0; i < 27; i++) dc->Value(&polar_subheader_.fill_user[i]);
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
void ECAT7_POLAR::PrintHeader(std::list <std::string> * const sl,
                              const unsigned short int num) const
 { int i, j;
   std::string proc_code[10]={ "Volumetric", "Threshold Applied", "Summed Map",
                               "Subtracted Map", "Product of two maps",
                               "Ratio of two maps", "Bias", "Multiplier",
                               "Transform",
                               "Polar Map calculational protocol used" }, s;

   sl->push_back("************ Polar-Map-Matrix ("+toString(num, 2)+
                 ") ************");
   // s=" data_type:                      "+toString(polar_subheader_.data_type)+" (";
   // switch (polar_subheader_.data_type)
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
     sl->push_back(print_header_data_type(polar_subheader_.data_type));
   sl->push_back(" polar_map_type:                 "+                 toString(polar_subheader_.polar_map_type));
   sl->push_back(" num_rings:                      "+toString(polar_subheader_.num_rings));
   sl->push_back(" sectors_per_ring:               ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (polar_subheader_.sectors_per_ring[i*8+j] == 0) break;
         s+=toString(polar_subheader_.sectors_per_ring[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" ring_position:                  ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (polar_subheader_.ring_position[i*8+j] == 0) break;
         s+=toString(polar_subheader_.ring_position[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" ring_angle:                     ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (polar_subheader_.ring_angle[i*8+j] == 0) break;
         s+=toString(polar_subheader_.ring_angle[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" start_angle:                    "+toString(polar_subheader_.start_angle));
   sl->push_back(" long_axis_left:                 "+                 toString(polar_subheader_.long_axis_left[0])+" "+
                 toString(polar_subheader_.long_axis_left[1])+" "+                 toString(polar_subheader_.long_axis_left[2]));
   sl->push_back(" long_axis_right:                "+                 toString(polar_subheader_.long_axis_right[0])+" "+
                 toString(polar_subheader_.long_axis_right[1])+" "+                 toString(polar_subheader_.long_axis_right[2]));
   sl->push_back(" position data:                  "+                 toString(polar_subheader_.position_data));
   sl->push_back(" image min:                      "+toString(polar_subheader_.image_min));
   sl->push_back(" image max:                      "+toString(polar_subheader_.image_max));
   sl->push_back(" scale_factor:                   "+                 toString(polar_subheader_.scale_factor));
   sl->push_back(" pixel_size:                     "+                 toString(polar_subheader_.pixel_size)+" cm^3");
   sl->push_back(" frame_duration:                 "+                 toString(polar_subheader_.frame_duration)+" msec.");
   sl->push_back(" frame_start_time:               "+                 toString(polar_subheader_.frame_start_time)+" msec.");
   sl->push_back(" processing_code:                "+                 toString(polar_subheader_.processing_code));
   if ((j=polar_subheader_.processing_code) > 0)
    { s="  ( ";
      for (i=0; i < 11; i++)
       if ((j & (1 << i)) != 0) s+=proc_code[i]+" ";
      sl->push_back(s+")");
    }
   s=" quant_units:                    "+toString(polar_subheader_.quant_units)+" (";
   switch (polar_subheader_.quant_units)
    { case E7_QUANT_UNITS_Default:
       s+="default";
       break;
      case E7_QUANT_UNITS_Normalized:
       s+="normalized";
       break;
      case E7_QUANT_UNITS_Mean:
       s+="mean";
       break;
      case E7_QUANT_UNITS_StdDevMean:
       s+="std. deviation from mean";
       break;
    }
   sl->push_back(s+")");
   sl->push_back(" annotation:                     "+                 (std::string)(const char *)polar_subheader_.annotation);
   sl->push_back(" gate_duration:                  "+                 toString(polar_subheader_.gate_duration));
   sl->push_back(" r_wave_offset:                  "+                 toString(polar_subheader_.r_wave_offset));
   sl->push_back(" num_accepted_beats:             "+                 toString(polar_subheader_.num_accepted_beats));
   sl->push_back(" polar_map_protocol:             "+                 (std::string)(const char *)polar_subheader_.polar_map_protocol);
   sl->push_back(" database_name:                  "+                 (std::string)(const char *)polar_subheader_.database_name);
   /*
   for (i=0; i < 27; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(polar_subheader_.fill_cti[i]));
   for (i=0; i < 27; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(polar_subheader_.fill_user[i]));
   */
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of polar matrix.
    \param[in] file   handle of file

 */
void ECAT7_POLAR::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
                                                       // fill data into buffer
     dc->Value(polar_subheader_.data_type);
     dc->Value(polar_subheader_.polar_map_type);
     dc->Value(polar_subheader_.num_rings);
     for (i=0; i < 32; i++) dc->Value(polar_subheader_.sectors_per_ring[i]);
     for (i=0; i < 32; i++) dc->Value(polar_subheader_.ring_position[i]);
     for (i=0; i < 32; i++) dc->Value(polar_subheader_.ring_angle[i]);
     dc->Value(polar_subheader_.start_angle);
     for (i=0; i < 3; i++) dc->Value(polar_subheader_.long_axis_left[i]);
     for (i=0; i < 3; i++) dc->Value(polar_subheader_.long_axis_right[i]);
     dc->Value(polar_subheader_.position_data);
     dc->Value(polar_subheader_.image_min);
     dc->Value(polar_subheader_.image_max);
     dc->Value(polar_subheader_.scale_factor);
     dc->Value(polar_subheader_.pixel_size);
     dc->Value(polar_subheader_.frame_duration);
     dc->Value(polar_subheader_.frame_start_time);
     dc->Value(polar_subheader_.processing_code);
     dc->Value(polar_subheader_.quant_units);
     dc->Value(40, polar_subheader_.annotation);
     dc->Value(polar_subheader_.gate_duration);
     dc->Value(polar_subheader_.r_wave_offset);
     dc->Value(polar_subheader_.num_accepted_beats);
     dc->Value(20, polar_subheader_.polar_map_protocol);
     dc->Value(30, polar_subheader_.database_name);
     for (i=0; i < 27; i++) dc->Value(polar_subheader_.fill_cti[i]);
     for (i=0; i < 27; i++) dc->Value(polar_subheader_.fill_user[i]);
     dc->SaveBuffer(file);                                      // store header
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

// Read data_type as short int from file, return as scoped enum

ecat_matrix::MatrixDataType ECAT7_POLAR::get_data_type(void) {
  return static_cast<ecat_matrix::MatrixDataType>(polar_subheader_.data_type);
}

// Write scoped enum to file as short int

void ECAT7_POLAR::set_data_type(ecat_matrix::MatrixDataType t_data_type) {
  polar_subheader_.data_type = static_cast<signed short int>(t_data_type);
}
