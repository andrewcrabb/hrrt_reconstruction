/*! \class ECAT7_SCAN3D ecat7_scan3d.hpp "ecat7_scan3d.hpp"
    \brief This class implements the SCAN3D matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/17 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class is build on top of the ECAT7_MATRIX abstract base class and the
    ecat7_utils module and adds a few things specific to SCAN3D matrices, like
    loading, storing and printing the header.
 */

#include <cstdio>
#include "ecat7_scan3d.hpp"
#include "data_changer.h"
#include "ecat7_global.h"
#include "ecat7_utils.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object and fill header data structure with 0s.
 */
/*---------------------------------------------------------------------------*/
ECAT7_SCAN3D::ECAT7_SCAN3D()
 { memset(&scan3d_subheader_, 0, sizeof(tscan3d_subheader));                // header is empty
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content

    This operator copies the scan3d header and uses the copy operator of the
    base class to copy the dataset from another objects into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_SCAN3D& ECAT7_SCAN3D::operator = (const ECAT7_SCAN3D &e7)
 { if (this != &e7)
    {                                // copy header information into new object
      memcpy(&scan3d_subheader_, &e7.scan3d_subheader_, sizeof(tscan3d_subheader));
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
void ECAT7_SCAN3D::CutBins(const unsigned short int cut) { 
  utils_CutBins(&data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles, Depth(), cut);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header

 */
unsigned short int ECAT7_SCAN3D::DataTypeOrig() const {
 return(scan3d_subheader_.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate decay and frame length correction.
    \param[in] halflife          halflife of isotope in seconds
    \param[in] decay_corrected   decay correction already done ?
    \return factor for decay correction

 */
float ECAT7_SCAN3D::DecayCorrection(const float halflife,                                    const bool decay_corrected) const
 { return(utils_DecayCorrection(data, datatype,
                                (unsigned long int)scan3d_subheader_.num_r_elements*
                                (unsigned long int)scan3d_subheader_.num_angles*
                                (unsigned long int)Depth(), halflife,
                                decay_corrected, scan3d_subheader_.frame_start_time,
                                scan3d_subheader_.frame_duration));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Deinterleave dataset.

    Deinterleave dataset. The number of angular projections will be doubled
    and the number of bins per projection halved.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::DeInterleave() { 
  utils_DeInterleave(data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles,                      Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of slices in sinogram.
    \return number of slices in sinogram
 
 */
unsigned short int ECAT7_SCAN3D::Depth() const {
 unsigned short int sum, i;

   for (sum=0,
        i=0; i < 64; i++)
    if (scan3d_subheader_.num_z_elements[i] == 0) break;
     else sum+=scan3d_subheader_.num_z_elements[i];
   return(sum);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between feet first and head first.

    Change orientation between feet first amd head first. The dataset is
    flipped in z-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Feet2Head() const { 
  utils_Feet2Head(data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles,                   (unsigned short int *)scan3d_subheader_.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
 */
ECAT7_SCAN3D::tscan3d_subheader *ECAT7_SCAN3D::HeaderPtr() {
 return(&scan3d_subheader_);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of angular projections in sinogram.
    \return number of angular projections in sinogram
 
 */
unsigned short int ECAT7_SCAN3D::Height() const {
 return(scan3d_subheader_.num_angles);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Interleave dataset.

    Interleave dataset. The number of angular projections will be halved
    and the number of bins per projection doubled.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Interleave() { 
  utils_Interleave(data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles, Depth());
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
void *ECAT7_SCAN3D::LoadData(std::ifstream * const file,                             const unsigned long int matrix_records) { 
file->seekg(2*E7_RECLEN, std::ios::cur);               // skip matrix header
   return(ECAT7_MATRIX::LoadData(file, matrix_records-2));         // load data
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of scan3d matrix.
    \param[in] file   handle of file

 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::LoadHeader(std::ifstream * const file) { 
  DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(2 * E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&scan3d_subheader_.data_type);
     dc->Value(&scan3d_subheader_.num_dimensions);
     dc->Value(&scan3d_subheader_.num_r_elements);
     dc->Value(&scan3d_subheader_.num_angles);
     dc->Value(&scan3d_subheader_.corrections_applied);
     for (i=0; i < 64; i++) dc->Value(&scan3d_subheader_.num_z_elements[i]);
     dc->Value(&scan3d_subheader_.ring_difference);
     dc->Value(&scan3d_subheader_.storage_order);
     dc->Value(&scan3d_subheader_.axial_compression);
     dc->Value(&scan3d_subheader_.x_resolution);
     dc->Value(&scan3d_subheader_.v_resolution);
     dc->Value(&scan3d_subheader_.z_resolution);
     dc->Value(&scan3d_subheader_.w_resolution);
     for (i=0; i < 6; i++) dc->Value(&scan3d_subheader_.fill_gate[i]);
     dc->Value(&scan3d_subheader_.gate_duration);
     dc->Value(&scan3d_subheader_.r_wave_offset);
     dc->Value(&scan3d_subheader_.num_accepted_beats);
     dc->Value(&scan3d_subheader_.scale_factor);
     dc->Value(&scan3d_subheader_.scan_min);
     dc->Value(&scan3d_subheader_.scan_max);
     dc->Value(&scan3d_subheader_.prompts);
     dc->Value(&scan3d_subheader_.delayed);
     dc->Value(&scan3d_subheader_.multiples);
     dc->Value(&scan3d_subheader_.net_trues);
     dc->Value(&scan3d_subheader_.tot_avg_cor);
     dc->Value(&scan3d_subheader_.tot_avg_uncor);
     dc->Value(&scan3d_subheader_.total_coin_rate);
     dc->Value(&scan3d_subheader_.frame_start_time);
     dc->Value(&scan3d_subheader_.frame_duration);
     dc->Value(&scan3d_subheader_.deadtime_correction_factor);
     for (i=0; i < 90; i++) dc->Value(&scan3d_subheader_.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(&scan3d_subheader_.fill_user[i]);
     for (i=0; i < 128; i++) dc->Value(&scan3d_subheader_.uncor_singles[i]);
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate number of records needed for matrix.
    \return number of records

    Calculate number of records needed for matrix. Take into account that the
    scan3d header requires an additional record.
 */
/*---------------------------------------------------------------------------*/
unsigned long int ECAT7_SCAN3D::NumberOfRecords() const
 {                                      // scan3d header uses additional record
   return(ECAT7_MATRIX::NumberOfRecords()+1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print header information into string list.
    \param[in] sl    string list
    \param[in] num   matrix number

 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::PrintHeader(std::list <std::string> * const sl,                               const unsigned short int num) const
 { int i, j;
   // std::string applied_proc[15]={ "Normalized",
   //           "Measured-Attenuation-Correction",
   //           "Calculated-Attenuation-Correction", "X-smoothing", "Y-smoothing",
   //           "Z-smoothing", "2D-scatter-correction", "3D-scatter-correction",
   //           "Arc-correction", "Decay-correction", "Online-compression",
   //           "FORE", "SSRB", "SEG0", "Random-smoothing" }, s;
  std::string s;

   sl->push_back("************* 3D-Scan-Matrix (" + toString(num)+                 ") *************");
   // s=" data_type:                      " + toString(scan3d_subheader_.data_type)+" (";
   // switch (scan3d_subheader_.data_type)
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
     sl->push_back(print_header_data_type(scan3d_subheader_.data_type));
   sl->push_back(" Number of Dimensions:           " + toString(scan3d_subheader_.num_dimensions));
   sl->push_back(" Dimension 1 (ring elements):    " + toString(scan3d_subheader_.num_r_elements));
   sl->push_back(" Dimension 2 (angles):           " + toString(scan3d_subheader_.num_angles));
   sl->push_back(" corrections_applied:            " +                 toString(scan3d_subheader_.corrections_applied));
   if ((j=scan3d_subheader_.corrections_applied) > 0)
    { s="  ( ";
      for (i=0; i < 15; i++)
       if ((j & (1 << i)) != 0) s+=ecat_matrix::applied_proc_.at(i) + " ";
      sl->push_back(s+")");
    }
   s=" num_z_elements:                 " + toString(scan3d_subheader_.num_z_elements[0]);
   for (i=1; scan3d_subheader_.num_z_elements[i] != 0; i++)
    s+="+" + toString(scan3d_subheader_.num_z_elements[i]);
   sl->push_back(s);
   sl->push_back(" ring difference:                " + toString(scan3d_subheader_.ring_difference));
   s=" storage order:                  " + toString(scan3d_subheader_.storage_order)+" (";
   switch (scan3d_subheader_.storage_order)
    { case E7_STORAGE_ORDER_RTZD:  s+="r, theta, z, d"; break;
      case E7_STORAGE_ORDER_RZTD:  s+="r, z, theta, d"; break;
      default:                     s+="none";           break;
    }
   sl->push_back(s+")");
   sl->push_back(" axial compression (span):       " + toString(scan3d_subheader_.axial_compression));
   sl->push_back(" x resolution (sample distance): " + toString(scan3d_subheader_.x_resolution)+ " cm");
   sl->push_back(" v_resolution:                   " + toString(scan3d_subheader_.v_resolution)+ " deg.");
   sl->push_back(" z_resolution:                   " + toString(scan3d_subheader_.z_resolution)+ " cm");
   sl->push_back(" w_resolution:                   " + toString(scan3d_subheader_.w_resolution));
   /*
   for (i=0; i < 6; i++)
    sl->push_back(" fill (" + toString(i)+"):                      "+
                  toString(scan3d_subheader_.fill_gate[i]));
   */
   sl->push_back(" Gate duration:                  " + toString(scan3d_subheader_.gate_duration)+" msec.");
   sl->push_back(" R-wave Offset:                  " + toString(scan3d_subheader_.r_wave_offset)+" msec.");
   sl->push_back(" num_accepted_beats:             " + toString(scan3d_subheader_.num_accepted_beats));
   sl->push_back(" Scale factor:                   " + toString(scan3d_subheader_.scale_factor));
   sl->push_back(" Scan_min:                       " + toString(scan3d_subheader_.scan_min));
   sl->push_back(" Scan_max:                       " + toString(scan3d_subheader_.scan_max));
   sl->push_back(" Prompt Events:                  " + toString(scan3d_subheader_.prompts));
   sl->push_back(" Delayed Events:                 " + toString(scan3d_subheader_.delayed));
   sl->push_back(" Multiple Events:                " + toString(scan3d_subheader_.multiples));
   sl->push_back(" Net True Events:                " + toString(scan3d_subheader_.net_trues));
   sl->push_back(" Total Avg. Corrected Singles:   " + toString(scan3d_subheader_.tot_avg_cor));
   sl->push_back(" Total Avg. Uncorrected Singles: " + toString(scan3d_subheader_.tot_avg_uncor));
   sl->push_back(" Total Coincidence Rate:         " + toString(scan3d_subheader_.total_coin_rate));
   sl->push_back(" Frame Start Time:               " + toString(scan3d_subheader_.frame_start_time)+" msec.");
   sl->push_back(" Frame Duration:                 " + toString(scan3d_subheader_.frame_duration)+" msec.");
   sl->push_back(" Deadtime Correction Factor:     " + toString(scan3d_subheader_.deadtime_correction_factor));
   /*
   for (i=0; i < 90; i++)
    sl->push_back(" fill (" + toString(i, 2)+"):                     "+
                  toString(scan3d_subheader_.fill_cti[i]));
   for (i=0; i < 50; i++)
    sl->push_back(" fill (" + toString(i, 2)+"):                     "+
                  toString(scan3d_subheader_.fill_user[i]));
   */
   sl->push_back(" Uncorrected Singles:");
   for (i=0; i < 16; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (scan3d_subheader_.uncor_singles[i*8+j] == 0) break;
         s+=toString(scan3d_subheader_.uncor_singles[i*8+j], 8)+" ";
       }
      if (s.length() > 0) sl->push_back(s);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between prone and supine.

    Change orientation between prone and supine. The dataset is flipped in
    y-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Prone2Supine() const
 { utils_Prone2Supine(data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles,
                      Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rotate sinogram counterclockwise along scanned axis.
    \param[in] rt   number of steps to rotate (each step is 180/#angles
                    degrees)

 */
void ECAT7_SCAN3D::Rotate(const signed short int rt) const {
 utils_Rotate(data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles, Depth(), rt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of scan3d matrix.
    \param[in] file   handle of file

 */
void ECAT7_SCAN3D::SaveHeader(std::ofstream * const file) const {
 DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(2 * E7_RECLEN);
     dc->Value(scan3d_subheader_.data_type);
     dc->Value(scan3d_subheader_.num_dimensions);
     dc->Value(scan3d_subheader_.num_r_elements);
     dc->Value(scan3d_subheader_.num_angles);
     dc->Value(scan3d_subheader_.corrections_applied);
     for (i=0; i < 64; i++) dc->Value(scan3d_subheader_.num_z_elements[i]);
     dc->Value(scan3d_subheader_.ring_difference);
     dc->Value(scan3d_subheader_.storage_order);
     dc->Value(scan3d_subheader_.axial_compression);
     dc->Value(scan3d_subheader_.x_resolution);
     dc->Value(scan3d_subheader_.v_resolution);
     dc->Value(scan3d_subheader_.z_resolution);
     dc->Value(scan3d_subheader_.w_resolution);
     for (i=0; i < 6; i++) dc->Value(scan3d_subheader_.fill_gate[i]);
     dc->Value(scan3d_subheader_.gate_duration);
     dc->Value(scan3d_subheader_.r_wave_offset);
     dc->Value(scan3d_subheader_.num_accepted_beats);
     dc->Value(scan3d_subheader_.scale_factor);
     dc->Value(scan3d_subheader_.scan_min);
     dc->Value(scan3d_subheader_.scan_max);
     dc->Value(scan3d_subheader_.prompts);
     dc->Value(scan3d_subheader_.delayed);
     dc->Value(scan3d_subheader_.multiples);
     dc->Value(scan3d_subheader_.net_trues);
     dc->Value(scan3d_subheader_.tot_avg_cor);
     dc->Value(scan3d_subheader_.tot_avg_uncor);
     dc->Value(scan3d_subheader_.total_coin_rate);
     dc->Value(scan3d_subheader_.frame_start_time);
     dc->Value(scan3d_subheader_.frame_duration);
     dc->Value(scan3d_subheader_.deadtime_correction_factor);
     for (i=0; i < 90; i++) dc->Value(scan3d_subheader_.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(scan3d_subheader_.fill_user[i]);
     for (i=0; i < 128; i++) dc->Value(scan3d_subheader_.uncor_singles[i]);
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
void ECAT7_SCAN3D::ScaleMatrix(const float factor)
 { utils_ScaleMatrix(data, datatype, (unsigned long int)scan3d_subheader_.num_r_elements*
                                     (unsigned long int)scan3d_subheader_.num_angles*
                                     (unsigned long int)Depth(), factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert sinogram from view to volume format (rztd to rtzd).

    Convert sinogram from view to volume format (rztd to rtzd).
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::View2Volume() const
 { utils_View2Volume(data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles,
                     (unsigned short int *)scan3d_subheader_.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert sinogram from volume to view format (rtzd to rztd).

    Convert sinogram from volume to view format (rtzd to rztd).
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Volume2View() const
 { utils_Volume2View(data, datatype, scan3d_subheader_.num_r_elements, scan3d_subheader_.num_angles,
                     (unsigned short int *)scan3d_subheader_.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bins in a projection.
    \return number of bins in a projection
 
    Request number of bins in a projection.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_SCAN3D::Width() const { 
  return(scan3d_subheader_.num_r_elements);
 }

// Read data_type as short int from file, return as scoped enum

MatrixData::DataType ECAT7_SCAN3D::get_data_type(void) {
  return static_cast<MatrixData::DataType>(scan3d_subheader_.data_type);
}

// Write scoped enum to file as short int

void ECAT7_ATTENUATION::set_data_type(MatrixData::DataType t_data_type) {
  scan3d_subheader_.data_type = static_cast<signed short int>(t_data_type);
}
