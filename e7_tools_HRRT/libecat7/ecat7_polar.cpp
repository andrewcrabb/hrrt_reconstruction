/*! \class ECAT7_POLAR ecat7_polar.h "ecat7_polar.h"
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
#include "ecat7_polar.h"
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
ECAT7_POLAR::ECAT7_POLAR()
 { memset(&ph, 0, sizeof(tpolar_subheader));                 // header is empty
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content

    This operator copies the polar header and uses the copy operator of the
    base class to copy the dataset from another objects into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_POLAR& ECAT7_POLAR::operator = (const ECAT7_POLAR &e7)
 { if (this != &e7)
    {                                // copy header information into new object
      memcpy(&ph, &e7.ph, sizeof(tpolar_subheader));
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
unsigned short int ECAT7_POLAR::DataTypeOrig() const
 { return(ph.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
    Request pointer to header data structure.
 */
/*---------------------------------------------------------------------------*/
ECAT7_POLAR::tpolar_subheader *ECAT7_POLAR::HeaderPtr()
 { return(&ph);
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

    Load header of polar matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_POLAR::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&ph.data_type);
     dc->Value(&ph.polar_map_type);
     dc->Value(&ph.num_rings);
     for (i=0; i < 32; i++) dc->Value(&ph.sectors_per_ring[i]);
     for (i=0; i < 32; i++) dc->Value(&ph.ring_position[i]);
     for (i=0; i < 32; i++) dc->Value(&ph.ring_angle[i]);
     dc->Value(&ph.start_angle);
     for (i=0; i < 3; i++) dc->Value(&ph.long_axis_left[i]);
     for (i=0; i < 3; i++) dc->Value(&ph.long_axis_right[i]);
     dc->Value(&ph.position_data);
     dc->Value(&ph.image_min);
     dc->Value(&ph.image_max);
     dc->Value(&ph.scale_factor);
     dc->Value(&ph.pixel_size);
     dc->Value(&ph.frame_duration);
     dc->Value(&ph.frame_start_time);
     dc->Value(&ph.processing_code);
     dc->Value(&ph.quant_units);
     dc->Value(ph.annotation, 40);
     dc->Value(&ph.gate_duration);
     dc->Value(&ph.r_wave_offset);
     dc->Value(&ph.num_accepted_beats);
     dc->Value(ph.polar_map_protocol, 20);
     dc->Value(ph.database_name, 30);
     for (i=0; i < 27; i++) dc->Value(&ph.fill_cti[i]);
     for (i=0; i < 27; i++) dc->Value(&ph.fill_user[i]);
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
   s=" data_type:                      "+toString(ph.data_type)+" (";
   switch (ph.data_type)
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
   sl->push_back(" polar_map_type:                 "+
                 toString(ph.polar_map_type));
   sl->push_back(" num_rings:                      "+toString(ph.num_rings));
   sl->push_back(" sectors_per_ring:               ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (ph.sectors_per_ring[i*8+j] == 0) break;
         s+=toString(ph.sectors_per_ring[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" ring_position:                  ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (ph.ring_position[i*8+j] == 0) break;
         s+=toString(ph.ring_position[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" ring_angle:                     ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (ph.ring_angle[i*8+j] == 0) break;
         s+=toString(ph.ring_angle[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" start_angle:                    "+toString(ph.start_angle));
   sl->push_back(" long_axis_left:                 "+
                 toString(ph.long_axis_left[0])+" "+
                 toString(ph.long_axis_left[1])+" "+
                 toString(ph.long_axis_left[2]));
   sl->push_back(" long_axis_right:                "+
                 toString(ph.long_axis_right[0])+" "+
                 toString(ph.long_axis_right[1])+" "+
                 toString(ph.long_axis_right[2]));
   sl->push_back(" position data:                  "+
                 toString(ph.position_data));
   sl->push_back(" image min:                      "+toString(ph.image_min));
   sl->push_back(" image max:                      "+toString(ph.image_max));
   sl->push_back(" scale_factor:                   "+
                 toString(ph.scale_factor));
   sl->push_back(" pixel_size:                     "+
                 toString(ph.pixel_size)+" cm^3");
   sl->push_back(" frame_duration:                 "+
                 toString(ph.frame_duration)+" msec.");
   sl->push_back(" frame_start_time:               "+
                 toString(ph.frame_start_time)+" msec.");
   sl->push_back(" processing_code:                "+
                 toString(ph.processing_code));
   if ((j=ph.processing_code) > 0)
    { s="  ( ";
      for (i=0; i < 11; i++)
       if ((j & (1 << i)) != 0) s+=proc_code[i]+" ";
      sl->push_back(s+")");
    }
   s=" quant_units:                    "+toString(ph.quant_units)+" (";
   switch (ph.quant_units)
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
   sl->push_back(" annotation:                     "+
                 (std::string)(const char *)ph.annotation);
   sl->push_back(" gate_duration:                  "+
                 toString(ph.gate_duration));
   sl->push_back(" r_wave_offset:                  "+
                 toString(ph.r_wave_offset));
   sl->push_back(" num_accepted_beats:             "+
                 toString(ph.num_accepted_beats));
   sl->push_back(" polar_map_protocol:             "+
                 (std::string)(const char *)ph.polar_map_protocol);
   sl->push_back(" database_name:                  "+
                 (std::string)(const char *)ph.database_name);
   /*
   for (i=0; i < 27; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(ph.fill_cti[i]));
   for (i=0; i < 27; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(ph.fill_user[i]));
   */
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of polar matrix.
    \param[in] file   handle of file

    Store header of polar matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_POLAR::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
                                                       // fill data into buffer
     dc->Value(ph.data_type);
     dc->Value(ph.polar_map_type);
     dc->Value(ph.num_rings);
     for (i=0; i < 32; i++) dc->Value(ph.sectors_per_ring[i]);
     for (i=0; i < 32; i++) dc->Value(ph.ring_position[i]);
     for (i=0; i < 32; i++) dc->Value(ph.ring_angle[i]);
     dc->Value(ph.start_angle);
     for (i=0; i < 3; i++) dc->Value(ph.long_axis_left[i]);
     for (i=0; i < 3; i++) dc->Value(ph.long_axis_right[i]);
     dc->Value(ph.position_data);
     dc->Value(ph.image_min);
     dc->Value(ph.image_max);
     dc->Value(ph.scale_factor);
     dc->Value(ph.pixel_size);
     dc->Value(ph.frame_duration);
     dc->Value(ph.frame_start_time);
     dc->Value(ph.processing_code);
     dc->Value(ph.quant_units);
     dc->Value(40, ph.annotation);
     dc->Value(ph.gate_duration);
     dc->Value(ph.r_wave_offset);
     dc->Value(ph.num_accepted_beats);
     dc->Value(20, ph.polar_map_protocol);
     dc->Value(30, ph.database_name);
     for (i=0; i < 27; i++) dc->Value(ph.fill_cti[i]);
     for (i=0; i < 27; i++) dc->Value(ph.fill_user[i]);
     dc->SaveBuffer(file);                                      // store header
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }
