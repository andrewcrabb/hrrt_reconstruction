/*! \class ECAT7_SCAN ecat7_scan.h "ecat7_scan.h"
    \brief This class implements the SCAN matrix type of imported ECAT6.5
           files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/17 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class is build on top of the ECAT7_MATRIX abstract base class and the
    ecat7_utils module and adds a few things specific to SCAN matrices, like
    loading, storing and printing the header.
 */

#include <cstdio>
#include "ecat7_scan.h"
#include "data_changer.h"
#include "ecat7_global.h"
#include "ecat7_utils.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
 */

ECAT7_SCAN::ECAT7_SCAN()
 { memset(&scan_subheader_, 0, sizeof(tscan_subheader));                  // header is empty
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content

    This operator copies the scan header and uses the copy operator of the
    base class to copy the dataset from another objects into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_SCAN& ECAT7_SCAN::operator = (const ECAT7_SCAN &e7)
 { if (this != &e7)
    {                                // copy header information into new object
      memcpy(&scan_subheader_, &e7.sh, sizeof(tscan_subheader));
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
void ECAT7_SCAN::CutBins(const unsigned short int cut)
 { utils_CutBins(&data, datatype, scan_subheader_.num_r_elements, scan_subheader_.num_angles,                 scan_subheader_.num_z_elements, cut);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header
 */

unsigned short int ECAT7_SCAN::DataTypeOrig() const
 { return(scan_subheader_.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate decay and frame length correction.
    \param[in] halflife          halflife of isotope in seconds
    \param[in] decay_corrected   decay correction already done ?
    \return factor for decay correction
 */

float ECAT7_SCAN::DecayCorrection(const float halflife,                                  const bool decay_corrected) const
 { return(utils_DecayCorrection(data, datatype,
                                (unsigned long int)scan_subheader_.num_r_elements*
                                (unsigned long int)scan_subheader_.num_angles*
                                (unsigned long int)scan_subheader_.num_z_elements, halflife,
                                decay_corrected, scan_subheader_.frame_start_time,
                                scan_subheader_.frame_duration));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Deinterleave dataset.

    Deinterleave dataset. The number of angular projections will be doubled
    and the number of bins per projection halved.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN::DeInterleave()
 { utils_DeInterleave(data, datatype, scan_subheader_.num_r_elements, scan_subheader_.num_angles,
                      scan_subheader_.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of slices in sinogram.
    \return number of slices in sinogram

 */

unsigned short int ECAT7_SCAN::Depth() const
 { return(scan_subheader_.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between feet first and head first.

    Change orientation between feet first amd head first. The dataset is
    flipped in z-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN::Feet2Head() const
 { unsigned short int nze[2];         // number of slices in different segments

   nze[0]=scan_subheader_.num_z_elements;
   nze[1]=0;                            // these datasets have only one segment
   utils_Feet2Head(data, datatype, scan_subheader_.num_r_elements, scan_subheader_.num_angles, nze);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure

 */

ECAT7_SCAN::tscan_subheader *ECAT7_SCAN::HeaderPtr()
 { return(&scan_subheader_);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of angular projections in sinogram.
    \return number of angular projections in sinogram

 */

unsigned short int ECAT7_SCAN::Height() const
 { return(scan_subheader_.num_angles);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Interleave dataset.

    Interleave dataset. The number of angular projections will be halved
    and the number of bins per projection doubled.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN::Interleave()
 { utils_Interleave(data, datatype, scan_subheader_.num_r_elements, scan_subheader_.num_angles,
                    scan_subheader_.num_z_elements);
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
void *ECAT7_SCAN::LoadData(std::ifstream * const file,
                           const unsigned long int matrix_records)
 { file->seekg(E7_RECLEN, std::ios::cur);                 // skip matrix header
   return(ECAT7_MATRIX::LoadData(file, matrix_records-1));         // load data
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of scan matrix.
    \param[in] file   handle of file
 */

void ECAT7_SCAN::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&scan_subheader_.data_type);
     dc->Value(&scan_subheader_.num_dimensions);
     dc->Value(&scan_subheader_.num_r_elements);
     dc->Value(&scan_subheader_.num_angles);
     dc->Value(&scan_subheader_.corrections_applied);
     dc->Value(&scan_subheader_.num_z_elements);
     dc->Value(&scan_subheader_.ring_difference);
     dc->Value(&scan_subheader_.x_resolution);
     dc->Value(&scan_subheader_.y_resolution);
     dc->Value(&scan_subheader_.z_resolution);
     dc->Value(&scan_subheader_.w_resolution);
     for (i=0; i < 6; i++) dc->Value(&scan_subheader_.fill_gate[i]);
     dc->Value(&scan_subheader_.gate_duration);
     dc->Value(&scan_subheader_.r_wave_offset);
     dc->Value(&scan_subheader_.num_accepted_beats);
     dc->Value(&scan_subheader_.scale_factor);
     dc->Value(&scan_subheader_.scan_min);
     dc->Value(&scan_subheader_.scan_max);
     dc->Value(&scan_subheader_.prompts);
     dc->Value(&scan_subheader_.delayed);
     dc->Value(&scan_subheader_.multiples);
     dc->Value(&scan_subheader_.net_trues);
     for (i=0; i < 16; i++) dc->Value(&scan_subheader_.cor_singles[i]);
     for (i=0; i < 16; i++) dc->Value(&scan_subheader_.uncor_singles[i]);
     dc->Value(&scan_subheader_.tot_avg_cor);
     dc->Value(&scan_subheader_.tot_avg_uncor);
     dc->Value(&scan_subheader_.total_coin_rate);
     dc->Value(&scan_subheader_.frame_start_time);
     dc->Value(&scan_subheader_.frame_duration);
     dc->Value(&scan_subheader_.deadtime_correction_factor);
     for (i=0; i < 8; i++) dc->Value(&scan_subheader_.physical_planes[i]);
     for (i=0; i < 83; i++) dc->Value(&scan_subheader_.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(&scan_subheader_.fill_user[i]);
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
void ECAT7_SCAN::PrintHeader(std::list <std::string> * const sl,
                             const unsigned short int num) const
 { int i, j;
   // std::string applied_proc[11]={ "Normalized",
   //          "Measured-Attenuation-Correction",
   //          "Calculated-Attenuation-Correction", "X-smoothing", "Y-smoothing",
   //          "Z-smoothing", "2D-scatter-correction", "3D-scatter-correction",
   //          "Arc-correction", "Decay-correction", "Online-compression" }, s;
            std::string s;

   sl->push_back("*************** Scan-Matrix (" + toString(num, 2)+
                 ") **************");
   // s=" data_type:                      " + toString(scan_subheader_.data_type)+" (";
   // switch (scan_subheader_.data_type)
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
     sl->push_back(print_header_data_type(scan_subheader_.data_type));
   sl->push_back(" Number of Dimensions:           " + toString(scan_subheader_.num_dimensions));
   sl->push_back(" Dimension 1 (ring elements):    " + toString(scan_subheader_.num_r_elements));
   sl->push_back(" Dimension 2 (angles):           " + toString(scan_subheader_.num_angles));
   sl->push_back(" corrections_applied:            " + toString(scan_subheader_.corrections_applied));
   if ((j=scan_subheader_.corrections_applied) > 0)
    { s="  ( ";
      for (i=0; i < 11; i++)
       if ((j & (1 << i)) != 0) s+=ecat_matrix::applied_proc_.at(i) + " ";
      sl->push_back(s+")");
    }
   sl->push_back(" num_z_elements:                 " + toString(scan_subheader_.num_z_elements));
   sl->push_back(" ring difference:                " + toString(scan_subheader_.ring_difference));
   sl->push_back(" x resolution (sample distance): " + toString(scan_subheader_.x_resolution) + " cm");
   sl->push_back(" y_resolution:                   " + toString(scan_subheader_.y_resolution) + " deg.");
   sl->push_back(" z_resolution:                   " + toString(scan_subheader_.z_resolution) + " cm");
   sl->push_back(" w_resolution:                   " + toString(scan_subheader_.w_resolution));
   /*
   for (i=0; i < 6; i++)
    sl->push_back(" fill (" + toString(i)+"):                      "+
                  toString(scan_subheader_.fill_gate[i]));
   */
   sl->push_back(" Gate duration:                  " + toString(scan_subheader_.gate_duration)+" msec.");
   sl->push_back(" R-wave Offset:                  " + toString(scan_subheader_.r_wave_offset)+" msec.");
   sl->push_back(" num_accepted_beats:             " + toString(scan_subheader_.num_accepted_beats));
   sl->push_back(" Scale factor:                   " + toString(scan_subheader_.scale_factor));
   sl->push_back(" Scan_min:                       " + toString(scan_subheader_.scan_min));
   sl->push_back(" Scan_max:                       " + toString(scan_subheader_.scan_max));
   sl->push_back(" Prompt Events:                  " + toString(scan_subheader_.prompts));
   sl->push_back(" Delayed Events:                 " + toString(scan_subheader_.delayed));
   sl->push_back(" Multiple Events:                " + toString(scan_subheader_.multiples));
   sl->push_back(" Net True Events:                " + toString(scan_subheader_.net_trues));
   sl->push_back(" Corrected Singles:");
   for (i=0; i < 2; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (scan_subheader_.cor_singles[i*8+j] == 0) break;
         s+=toString(scan_subheader_.cor_singles[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" Uncorrected Singles:");
   for (i=0; i < 2; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (scan_subheader_.uncor_singles[i*8+j] == 0) break;
         s+=toString(scan_subheader_.uncor_singles[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" Total Avg. Corrected Singles:   " + toString(scan_subheader_.tot_avg_cor));
   sl->push_back(" Total Avg. Uncorrected Singles: " + toString(scan_subheader_.tot_avg_uncor));
   sl->push_back(" Total Coincidence Rate:         " + toString(scan_subheader_.total_coin_rate));
   sl->push_back(" Frame Start Time:               " + toString(scan_subheader_.frame_start_time)+" msec.");
   sl->push_back(" Frame Duration:                 " + toString(scan_subheader_.frame_duration)+" msec.");
   sl->push_back(" Deadtime Correction Factor:     " + toString(scan_subheader_.deadtime_correction_factor, 5));
   s=" Physical Planes:                ";
   for (i=0; i < 8; i++)
    s+=toString(scan_subheader_.physical_planes[i])+" ";
   sl->push_back(s);
   /*
   for (i=0; i < 83; i++)
    sl->push_back(" fill (" + toString(i, 2)+"):                     "+
                  toString(scan_subheader_.fill_cti[i]));
   for (i=0; i < 50; i++)
    sl->push_back(" fill (" + toString(i, 2)+"):                     "+
                  toString(scan_subheader_.fill_user[i]));
   */
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between prone and supine.

    Change orientation between prone and supine. The dataset is flipped in
    y-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN::Prone2Supine() const
 { utils_Prone2Supine(data, datatype, scan_subheader_.num_r_elements, scan_subheader_.num_angles,
                      scan_subheader_.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rotate sinogram counterclockwise along scanned axis.
    \param[in] rt   number of steps to rotate (each step is 180/#angles
                    degrees)

    Rotate sinogram counterclockwise along scanned axis.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN::Rotate(const signed short int rt) const
 { utils_Rotate(data, datatype, scan_subheader_.num_r_elements, scan_subheader_.num_angles,
                scan_subheader_.num_z_elements, rt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of scan matrix.
    \param[in] file   handle of file

    Store header of scan matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
                                                       // fill data into buffer
     dc->Value(scan_subheader_.data_type);
     dc->Value(scan_subheader_.num_dimensions);
     dc->Value(scan_subheader_.num_r_elements);
     dc->Value(scan_subheader_.num_angles);
     dc->Value(scan_subheader_.corrections_applied);
     dc->Value(scan_subheader_.num_z_elements);
     dc->Value(scan_subheader_.ring_difference);
     dc->Value(scan_subheader_.x_resolution);
     dc->Value(scan_subheader_.y_resolution);
     dc->Value(scan_subheader_.z_resolution);
     dc->Value(scan_subheader_.w_resolution);
     for (i=0; i < 6; i++) dc->Value(scan_subheader_.fill_gate[i]);
     dc->Value(scan_subheader_.gate_duration);
     dc->Value(scan_subheader_.r_wave_offset);
     dc->Value(scan_subheader_.num_accepted_beats);
     dc->Value(scan_subheader_.scale_factor);
     dc->Value(scan_subheader_.scan_min);
     dc->Value(scan_subheader_.scan_max);
     dc->Value(scan_subheader_.prompts);
     dc->Value(scan_subheader_.delayed);
     dc->Value(scan_subheader_.multiples);
     dc->Value(scan_subheader_.net_trues);
     for (i=0; i < 16; i++) dc->Value(scan_subheader_.cor_singles[i]);
     for (i=0; i < 16; i++) dc->Value(scan_subheader_.uncor_singles[i]);
     dc->Value(scan_subheader_.tot_avg_cor);
     dc->Value(scan_subheader_.tot_avg_uncor);
     dc->Value(scan_subheader_.total_coin_rate);
     dc->Value(scan_subheader_.frame_start_time);
     dc->Value(scan_subheader_.frame_duration);
     dc->Value(scan_subheader_.deadtime_correction_factor);
     for (i=0; i < 8; i++) dc->Value(scan_subheader_.physical_planes[i]);
     for (i=0; i < 83; i++) dc->Value(scan_subheader_.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(scan_subheader_.fill_user[i]);
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
 
    Scale sinogram.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN::ScaleMatrix(const float factor)
 { utils_ScaleMatrix(data, datatype, (unsigned long int)scan_subheader_.num_r_elements*
                                     (unsigned long int)scan_subheader_.num_angles*
                                     (unsigned long int)scan_subheader_.num_z_elements,
                     factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bins in a projection.
    \return number of bins in a projection
 
    Request number of bins in a projection.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_SCAN::Width() const
 { return(scan_subheader_.num_r_elements);
 }

// Read data_type as short int from file, return as scoped enum

ecat_matrix::MatrixDataType ECAT7_SCAN::get_data_type(void) {
  return static_cast<ecat_matrix::MatrixDataType>(scan_subheader_.data_type);
}

// Write scoped enum to file as short int

void ECAT7_SCAN::set_data_type(ecat_matrix::MatrixDataType t_data_type) {
  scan_subheader_.data_type = static_cast<signed short int>(t_data_type);
}
