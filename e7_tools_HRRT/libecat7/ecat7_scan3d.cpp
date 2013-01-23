/*! \class ECAT7_SCAN3D ecat7_scan3d.h "ecat7_scan3d.h"
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
#include "ecat7_scan3d.h"
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
 { memset(&sh, 0, sizeof(tscan3d_subheader));                // header is empty
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
      memcpy(&sh, &e7.sh, sizeof(tscan3d_subheader));
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
void ECAT7_SCAN3D::CutBins(const unsigned short int cut)
 { utils_CutBins(&data, datatype, sh.num_r_elements, sh.num_angles, Depth(),
                 cut);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header

    Request datatype from matrix header.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_SCAN3D::DataTypeOrig() const
 { return(sh.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate decay and frame length correction.
    \param[in] halflife          halflife of isotope in seconds
    \param[in] decay_corrected   decay correction already done ?
    \return factor for decay correction

    Calculate decay and frame length correction.
 */
/*---------------------------------------------------------------------------*/
float ECAT7_SCAN3D::DecayCorrection(const float halflife,
                                    const bool decay_corrected) const
 { return(utils_DecayCorrection(data, datatype,
                                (unsigned long int)sh.num_r_elements*
                                (unsigned long int)sh.num_angles*
                                (unsigned long int)Depth(), halflife,
                                decay_corrected, sh.frame_start_time,
                                sh.frame_duration));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Deinterleave dataset.

    Deinterleave dataset. The number of angular projections will be doubled
    and the number of bins per projection halved.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::DeInterleave()
 { utils_DeInterleave(data, datatype, sh.num_r_elements, sh.num_angles,
                      Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of slices in sinogram.
    \return number of slices in sinogram
 
    Request number of slices in sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_SCAN3D::Depth() const
 { unsigned short int sum, i;

   for (sum=0,
        i=0; i < 64; i++)
    if (sh.num_z_elements[i] == 0) break;
     else sum+=sh.num_z_elements[i];
   return(sum);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between feet first and head first.

    Change orientation between feet first amd head first. The dataset is
    flipped in z-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Feet2Head() const
 { utils_Feet2Head(data, datatype, sh.num_r_elements, sh.num_angles,
                   (unsigned short int *)sh.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
    Request pointer to header data structure.
 */
/*---------------------------------------------------------------------------*/
ECAT7_SCAN3D::tscan3d_subheader *ECAT7_SCAN3D::HeaderPtr()
 { return(&sh);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of angular projections in sinogram.
    \return number of angular projections in sinogram
 
    Request number of angular projections in sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_SCAN3D::Height() const
 { return(sh.num_angles);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Interleave dataset.

    Interleave dataset. The number of angular projections will be halved
    and the number of bins per projection doubled.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Interleave()
 { utils_Interleave(data, datatype, sh.num_r_elements, sh.num_angles, Depth());
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
void *ECAT7_SCAN3D::LoadData(std::ifstream * const file,
                             const unsigned long int matrix_records)
 { file->seekg(2*E7_RECLEN, std::ios::cur);               // skip matrix header
   return(ECAT7_MATRIX::LoadData(file, matrix_records-2));         // load data
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of scan3d matrix.
    \param[in] file   handle of file

    Load header of scan3d matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc=new DataChanger(2*E7_RECLEN, false, true);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&sh.data_type);
     dc->Value(&sh.num_dimensions);
     dc->Value(&sh.num_r_elements);
     dc->Value(&sh.num_angles);
     dc->Value(&sh.corrections_applied);
     for (i=0; i < 64; i++) dc->Value(&sh.num_z_elements[i]);
     dc->Value(&sh.ring_difference);
     dc->Value(&sh.storage_order);
     dc->Value(&sh.axial_compression);
     dc->Value(&sh.x_resolution);
     dc->Value(&sh.v_resolution);
     dc->Value(&sh.z_resolution);
     dc->Value(&sh.w_resolution);
     for (i=0; i < 6; i++) dc->Value(&sh.fill_gate[i]);
     dc->Value(&sh.gate_duration);
     dc->Value(&sh.r_wave_offset);
     dc->Value(&sh.num_accepted_beats);
     dc->Value(&sh.scale_factor);
     dc->Value(&sh.scan_min);
     dc->Value(&sh.scan_max);
     dc->Value(&sh.prompts);
     dc->Value(&sh.delayed);
     dc->Value(&sh.multiples);
     dc->Value(&sh.net_trues);
     dc->Value(&sh.tot_avg_cor);
     dc->Value(&sh.tot_avg_uncor);
     dc->Value(&sh.total_coin_rate);
     dc->Value(&sh.frame_start_time);
     dc->Value(&sh.frame_duration);
     dc->Value(&sh.deadtime_correction_factor);
     for (i=0; i < 90; i++) dc->Value(&sh.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(&sh.fill_user[i]);
     for (i=0; i < 128; i++) dc->Value(&sh.uncor_singles[i]);
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

    Print header information into string list.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::PrintHeader(std::list <std::string> * const sl,
                               const unsigned short int num) const
 { int i, j;
   std::string applied_proc[15]={ "Normalized",
             "Measured-Attenuation-Correction",
             "Calculated-Attenuation-Correction", "X-smoothing", "Y-smoothing",
             "Z-smoothing", "2D-scatter-correction", "3D-scatter-correction",
             "Arc-correction", "Decay-correction", "Online-compression",
             "FORE", "SSRB", "SEG0", "Random-smoothing" }, s;

   sl->push_back("************* 3D-Scan-Matrix ("+toString(num)+
                 ") *************");
   s=" data_type:                      "+toString(sh.data_type)+" (";
   switch (sh.data_type)
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
   sl->push_back(" Number of Dimensions:           "+
                 toString(sh.num_dimensions));
   sl->push_back(" Dimension 1 (ring elements):    "+
                 toString(sh.num_r_elements));
   sl->push_back(" Dimension 2 (angles):           "+toString(sh.num_angles));
   sl->push_back(" corrections_applied:            "+
                 toString(sh.corrections_applied));
   if ((j=sh.corrections_applied) > 0)
    { s="  ( ";
      for (i=0; i < 15; i++)
       if ((j & (1 << i)) != 0) s+=applied_proc[i]+" ";
      sl->push_back(s+")");
    }
   s=" num_z_elements:                 "+toString(sh.num_z_elements[0]);
   for (i=1; sh.num_z_elements[i] != 0; i++)
    s+="+"+toString(sh.num_z_elements[i]);
   sl->push_back(s);
   sl->push_back(" ring difference:                "+
                 toString(sh.ring_difference));
   s=" storage order:                  "+toString(sh.storage_order)+" (";
   switch (sh.storage_order)
    { case E7_STORAGE_ORDER_RTZD:  s+="r, theta, z, d"; break;
      case E7_STORAGE_ORDER_RZTD:  s+="r, z, theta, d"; break;
      default:                     s+="none";           break;
    }
   sl->push_back(s+")");
   sl->push_back(" axial compression (span):       "+
                 toString(sh.axial_compression));
   sl->push_back(" x resolution (sample distance): "+toString(sh.x_resolution)+
                 " cm");
   sl->push_back(" v_resolution:                   "+toString(sh.v_resolution)+
                 " deg.");
   sl->push_back(" z_resolution:                   "+toString(sh.z_resolution)+
                 " cm");
   sl->push_back(" w_resolution:                   "+
                 toString(sh.w_resolution));
   /*
   for (i=0; i < 6; i++)
    sl->push_back(" fill ("+toString(i)+"):                      "+
                  toString(sh.fill_gate[i]));
   */
   sl->push_back(" Gate duration:                  "+
                 toString(sh.gate_duration)+" msec.");
   sl->push_back(" R-wave Offset:                  "+
                 toString(sh.r_wave_offset)+" msec.");
   sl->push_back(" num_accepted_beats:             "+
                 toString(sh.num_accepted_beats));
   sl->push_back(" Scale factor:                   "+
                 toString(sh.scale_factor));
   sl->push_back(" Scan_min:                       "+toString(sh.scan_min));
   sl->push_back(" Scan_max:                       "+toString(sh.scan_max));
   sl->push_back(" Prompt Events:                  "+toString(sh.prompts));
   sl->push_back(" Delayed Events:                 "+toString(sh.delayed));
   sl->push_back(" Multiple Events:                "+toString(sh.multiples));
   sl->push_back(" Net True Events:                "+toString(sh.net_trues));
   sl->push_back(" Total Avg. Corrected Singles:   "+toString(sh.tot_avg_cor));
   sl->push_back(" Total Avg. Uncorrected Singles: "+
                 toString(sh.tot_avg_uncor));
   sl->push_back(" Total Coincidence Rate:         "+
                 toString(sh.total_coin_rate));
   sl->push_back(" Frame Start Time:               "+
                 toString(sh.frame_start_time)+" msec.");
   sl->push_back(" Frame Duration:                 "+
                 toString(sh.frame_duration)+" msec.");
   sl->push_back(" Deadtime Correction Factor:     "+
                 toString(sh.deadtime_correction_factor));
   /*
   for (i=0; i < 90; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(sh.fill_cti[i]));
   for (i=0; i < 50; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(sh.fill_user[i]));
   */
   sl->push_back(" Uncorrected Singles:");
   for (i=0; i < 16; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       { if (sh.uncor_singles[i*8+j] == 0) break;
         s+=toString(sh.uncor_singles[i*8+j], 8)+" ";
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
 { utils_Prone2Supine(data, datatype, sh.num_r_elements, sh.num_angles,
                      Depth());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rotate sinogram counterclockwise along scanned axis.
    \param[in] rt   number of steps to rotate (each step is 180/#angles
                    degrees)

    Rotate sinogram counterclockwise along scanned axis.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Rotate(const signed short int rt) const
 { utils_Rotate(data, datatype, sh.num_r_elements, sh.num_angles, Depth(), rt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of scan3d matrix.
    \param[in] file   handle of file

    Store header of scan3d matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc=new DataChanger(2*E7_RECLEN, false, true);
                                                       // fill data into buffer
     dc->Value(sh.data_type);
     dc->Value(sh.num_dimensions);
     dc->Value(sh.num_r_elements);
     dc->Value(sh.num_angles);
     dc->Value(sh.corrections_applied);
     for (i=0; i < 64; i++) dc->Value(sh.num_z_elements[i]);
     dc->Value(sh.ring_difference);
     dc->Value(sh.storage_order);
     dc->Value(sh.axial_compression);
     dc->Value(sh.x_resolution);
     dc->Value(sh.v_resolution);
     dc->Value(sh.z_resolution);
     dc->Value(sh.w_resolution);
     for (i=0; i < 6; i++) dc->Value(sh.fill_gate[i]);
     dc->Value(sh.gate_duration);
     dc->Value(sh.r_wave_offset);
     dc->Value(sh.num_accepted_beats);
     dc->Value(sh.scale_factor);
     dc->Value(sh.scan_min);
     dc->Value(sh.scan_max);
     dc->Value(sh.prompts);
     dc->Value(sh.delayed);
     dc->Value(sh.multiples);
     dc->Value(sh.net_trues);
     dc->Value(sh.tot_avg_cor);
     dc->Value(sh.tot_avg_uncor);
     dc->Value(sh.total_coin_rate);
     dc->Value(sh.frame_start_time);
     dc->Value(sh.frame_duration);
     dc->Value(sh.deadtime_correction_factor);
     for (i=0; i < 90; i++) dc->Value(sh.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(sh.fill_user[i]);
     for (i=0; i < 128; i++) dc->Value(sh.uncor_singles[i]);
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
 { utils_ScaleMatrix(data, datatype, (unsigned long int)sh.num_r_elements*
                                     (unsigned long int)sh.num_angles*
                                     (unsigned long int)Depth(), factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert sinogram from view to volume format (rztd to rtzd).

    Convert sinogram from view to volume format (rztd to rtzd).
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::View2Volume() const
 { utils_View2Volume(data, datatype, sh.num_r_elements, sh.num_angles,
                     (unsigned short int *)sh.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert sinogram from volume to view format (rtzd to rztd).

    Convert sinogram from volume to view format (rtzd to rztd).
 */
/*---------------------------------------------------------------------------*/
void ECAT7_SCAN3D::Volume2View() const
 { utils_Volume2View(data, datatype, sh.num_r_elements, sh.num_angles,
                     (unsigned short int *)sh.num_z_elements);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bins in a projection.
    \return number of bins in a projection
 
    Request number of bins in a projection.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_SCAN3D::Width() const
 { return(sh.num_r_elements);
 }
