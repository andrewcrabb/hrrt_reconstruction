/*! \class ECAT7_IMAGE ecat7_image.h "ecat7_image.h"
    \brief This class implements the IMAGE matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/19 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class is build on top of the ECAT7_MATRIX abstract base class and the
    ecat7_utils module and adds a few things specific to IMAGE matrices, like
    loading, storing and printing the header.
 */

#include <cstdio>
#include "ecat7_image.h"
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
ECAT7_IMAGE::ECAT7_IMAGE()
 { memset(&ih, 0, sizeof(timage_subheader));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content
 
    This operator copies the image header and uses the copy operator of the
    base class to copy the dataset from another object into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_IMAGE& ECAT7_IMAGE::operator = (const ECAT7_IMAGE &e7)
 { if (this != &e7)
    {                                // copy header information into new object
      memcpy(&ih, &e7.ih, sizeof(timage_subheader));
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
unsigned short int ECAT7_IMAGE::DataTypeOrig() const
 { return(ih.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of slices in image.
    \return number of slices in image
 
    Request number of slices in image.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_IMAGE::Depth() const
 { return(ih.z_dimension);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between feet first and head first.

    Change orientation between feet first amd head first. The dataset is
    flipped in z-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_IMAGE::Feet2Head() const
 { unsigned short int nze[2];         // number of slices in different segments

   nze[0]=ih.z_dimension;
   nze[1]=0;                            // these datasets have only one segment
   utils_Feet2Head(data, datatype, ih.x_dimension, ih.y_dimension, nze);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
    Request pointer to header data structure.
 */
/*---------------------------------------------------------------------------*/
ECAT7_IMAGE::timage_subheader *ECAT7_IMAGE::HeaderPtr()
 { return(&ih);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request height of image.
    \return height of image
 
    Request height of image.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_IMAGE::Height() const
 { return(ih.y_dimension);
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
void *ECAT7_IMAGE::LoadData(std::ifstream * const file,
                            const unsigned long int matrix_records)
 { file->seekg(E7_RECLEN, std::ios::cur);                 // skip matrix header
   return(ECAT7_MATRIX::LoadData(file, matrix_records-1));         // load data
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of image matrix.
    \param[in] file   handle of file

    Load header of image matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_IMAGE::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc=new DataChanger(E7_RECLEN, false, true);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(&ih.data_type);
     dc->Value(&ih.num_dimensions);
     dc->Value(&ih.x_dimension);
     dc->Value(&ih.y_dimension);
     dc->Value(&ih.z_dimension);
     dc->Value(&ih.x_offset);
     dc->Value(&ih.y_offset);
     dc->Value(&ih.z_offset);
     dc->Value(&ih.recon_zoom);
     dc->Value(&ih.scale_factor);
     dc->Value(&ih.image_min);
     dc->Value(&ih.image_max);
     dc->Value(&ih.x_pixel_size);
     dc->Value(&ih.y_pixel_size);
     dc->Value(&ih.z_pixel_size);
     dc->Value(&ih.frame_duration);
     dc->Value(&ih.frame_start_time);
     dc->Value(&ih.filter_code);
     dc->Value(&ih.x_resolution);
     dc->Value(&ih.y_resolution);
     dc->Value(&ih.z_resolution);
     dc->Value(&ih.num_r_elements);
     dc->Value(&ih.num_angles);
     dc->Value(&ih.z_rotation_angle);
     dc->Value(&ih.decay_corr_fctr);
     dc->Value(&ih.processing_code);
     dc->Value(&ih.gate_duration);
     dc->Value(&ih.r_wave_offset);
     dc->Value(&ih.num_accepted_beats);
     dc->Value(&ih.filter_cutoff_frequency);
     dc->Value(&ih.filter_resolution);
     dc->Value(&ih.filter_ramp_slope);
     dc->Value(&ih.filter_order);
     dc->Value(&ih.filter_scatter_fraction);
     dc->Value(&ih.filter_scatter_slope);
     dc->Value(ih.annotation, 40);
     dc->Value(&ih.mt_1_1);
     dc->Value(&ih.mt_1_2);
     dc->Value(&ih.mt_1_3);
     dc->Value(&ih.mt_2_1);
     dc->Value(&ih.mt_2_2);
     dc->Value(&ih.mt_2_3);
     dc->Value(&ih.mt_3_1);
     dc->Value(&ih.mt_3_2);
     dc->Value(&ih.mt_3_3);
     dc->Value(&ih.rfilter_cutoff);
     dc->Value(&ih.rfilter_resolution);
     dc->Value(&ih.rfilter_code);
     dc->Value(&ih.rfilter_order);
     dc->Value(&ih.zfilter_cutoff);
     dc->Value(&ih.zfilter_resolution);
     dc->Value(&ih.zfilter_code);
     dc->Value(&ih.zfilter_order);
     dc->Value(&ih.mt_1_4);
     dc->Value(&ih.mt_2_4);
     dc->Value(&ih.mt_3_4);
     dc->Value(&ih.scatter_type);
     dc->Value(&ih.recon_type);
     dc->Value(&ih.recon_views);
     for (i=0; i < 87; i++) dc->Value(&ih.fill_cti[i]);
     for (i=0; i < 48; i++) dc->Value(&ih.fill_user[i]);
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
void ECAT7_IMAGE::PrintHeader(std::list <std::string> * const sl,
                              const unsigned short int num) const
 { DataChanger *dc=NULL;

   try
   { int i, j;
     // std::string applied_proc[15]={ "Normalized",
     //         "Measured-Attenuation-Correction",
     //         "Calculated-Attenuation-Correction", "X-smoothing", "Y-smoothing",
     //         "Z-smoothing", "2D-scatter-correction", "3D-scatter-correction",
     //         "Arc-correction", "Decay-correction", "Online-compression",
     //         "FORE", "SSRB", "Seg0", "Randoms Smoothing" }, s;
             std::string s;

     sl->push_back("************** Image-Matrix ("+toString(num, 2)+
                   ") **************");
     s=" data_type:                      "+toString(ih.data_type)+" (";
     switch (ih.data_type)
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
                   toString(ih.num_dimensions));
     sl->push_back(" x-Dimension:                    "+
                   toString(ih.x_dimension));
     sl->push_back(" y-Dimension:                    "+
                   toString(ih.y_dimension));
     sl->push_back(" z-Dimension:                    "+
                   toString(ih.z_dimension));
     sl->push_back(" x-offset:                       "+toString(ih.x_offset)+
                   " cm");
     sl->push_back(" y-offset:                       "+toString(ih.y_offset)+
                   " cm");
     sl->push_back(" z-offset:                       "+toString(ih.z_offset)+
                   " cm");
     sl->push_back(" recon_zoom:                     "+
                   toString(ih.recon_zoom));
     sl->push_back(" scale_factor:                   "+
                   toString(ih.scale_factor));
     sl->push_back(" image_min:                      "+toString(ih.image_min));
     sl->push_back(" image_max:                      "+toString(ih.image_max));
     sl->push_back(" x_pixel_size:                   "+
                   toString(ih.x_pixel_size)+" cm");
     sl->push_back(" y_pixel_size:                   "+
                   toString(ih.y_pixel_size)+" cm");
     sl->push_back(" z_pixel_size:                   "+
                   toString(ih.z_pixel_size)+" cm");
     sl->push_back(" frame_duration:                 "+
                   toString(ih.frame_duration)+" msec.");
     sl->push_back(" frame_start_time:               "+
                   toString(ih.frame_start_time)+" msec.");
     s=" filter_code:                    "+toString(ih.filter_code)+" (";
     switch (ih.filter_code)
      { case E7_FILTER_CODE_NoFilter:    s+="no filter";   break;
        case E7_FILTER_CODE_Ramp:        s+="ramp";        break;
        case E7_FILTER_CODE_Butterfield: s+="Butterfield"; break;
        case E7_FILTER_CODE_Hann:        s+="Hann";        break;
        case E7_FILTER_CODE_Hamming:     s+="Hamming";     break;
        case E7_FILTER_CODE_Parzen:      s+="Parzen";      break;
        case E7_FILTER_CODE_Shepp:       s+="Shepp";       break;
        case E7_FILTER_CODE_Butterworth: s+="Butterworth"; break;
        case E7_FILTER_CODE_Gaussian:    s+="Gaussian";    break;
        case E7_FILTER_CODE_Median:      s+="Median";      break;
        case E7_FILTER_CODE_Boxcar:      s+="Boxcar";      break;
        default:                         s+="unknown";     break;
      }
     sl->push_back(s+")");
     sl->push_back(" x_resolution:                   "+
                   toString(ih.x_resolution)+" cm");
     sl->push_back(" y_resolution:                   "+
                   toString(ih.y_resolution)+" cm");
     sl->push_back(" y_resolution:                   "+
                   toString(ih.z_resolution)+" cm");
     sl->push_back(" num_r_elements:                 "+
                   toString(ih.num_r_elements));
     sl->push_back(" num_angles:                     "+
                   toString(ih.num_angles));
     sl->push_back(" z_rotation_angle:               "+
                   toString(ih.z_rotation_angle)+" deg.");
     sl->push_back(" decay_corr_fctr:                "+
                   toString(ih.decay_corr_fctr));
     sl->push_back(" processing_code:                "+
                   toString(ih.processing_code));
     if ((j=ih.processing_code) > 0)
      { s="  ( ";
        for (i=0; i < 15; i++)
         if ((j & (1 << i)) != 0) s+=ecat_matrix::applied_proc_.at(i) + " ";
        sl->push_back(s+")");
      }
     sl->push_back(" gate duration:                  " + toString(ih.gate_duration)+" msec.");
     sl->push_back(" r_wave_offset:                  " + toString(ih.r_wave_offset)+" msec.");
     sl->push_back(" num_accepted_beats:             " + toString(ih.num_accepted_beats));
     sl->push_back(" filter_cutoff_frequency:        " + toString(ih.filter_cutoff_frequency));
     sl->push_back(" filter_resolution:              " + toString(ih.filter_resolution));
     sl->push_back(" filter_ramp_slope:              " + toString(ih.filter_ramp_slope));
     sl->push_back(" filter_order:                   " + toString(ih.filter_order));
     sl->push_back(" filter_scatter_fraction:        " + toString(ih.filter_scatter_fraction));
     sl->push_back(" filter_scatter_slope:           " + toString(ih.filter_scatter_slope));
     sl->push_back(" annotation:                     " + (std::string)(const char *)ih.annotation);
     sl->push_back(" mt_1_1:                         " + toString(ih.mt_1_1));
     sl->push_back(" mt_1_2:                         " + toString(ih.mt_1_2));
     sl->push_back(" mt_1_3:                         " + toString(ih.mt_1_3));
     sl->push_back(" mt_2_1:                         " + toString(ih.mt_2_1));
     sl->push_back(" mt_2_2:                         " + toString(ih.mt_2_2));
     sl->push_back(" mt_2_3:                         " + toString(ih.mt_2_3));
     sl->push_back(" mt_3_1:                         " + toString(ih.mt_3_1));
     sl->push_back(" mt_3_2:                         " + toString(ih.mt_3_2));
     sl->push_back(" mt_3_3:                         " + toString(ih.mt_3_3));
     sl->push_back(" rfilter_cutoff:                 " + toString(ih.rfilter_cutoff));
     sl->push_back(" rfilter_resolution:             " + toString(ih.rfilter_resolution));
     sl->push_back(" rfilter_code:                   " + toString(ih.rfilter_code));
     sl->push_back(" rfilter_order:                  " + toString(ih.rfilter_order));
     sl->push_back(" zfilter_cutoff:                 " + toString(ih.zfilter_cutoff));
     sl->push_back(" zfilter_resolution:             " + toString(ih.zfilter_resolution));
     sl->push_back(" zfilter_code:                   " + toString(ih.zfilter_code));
     sl->push_back(" zfilter_order:                  " + toString(ih.zfilter_order));
     sl->push_back(" mt_1_4:                         " + toString(ih.mt_1_4));
     sl->push_back(" mt_2_4:                         " + toString(ih.mt_2_4));
     sl->push_back(" mt_3_4:                         " + toString(ih.mt_3_4));
     s=" scatter_type:                   "+toString(ih.scatter_type)+" (";
     switch (ih.scatter_type)
       { case E7_SCATTER_TYPE_NoScatter:
          s+="None";
          break;
         case E7_SCATTER_TYPE_ScatterDeconvolution:
          s+="Deconvolution";
          break;
         case E7_SCATTER_TYPE_ScatterSimulated:
          s+="Simulated";
          break;
         case E7_SCATTER_TYPE_ScatterDualEnergy:
          s+="Dual Energy";
          break;
         default:
          s+="unknown";
          break;
       }
     sl->push_back(s+")");
     s=" recon_type:                     "+toString(ih.recon_type)+" (";
     switch (ih.recon_type)
      { case E7_RECON_TYPE_FBPJ:
         s+="Filtered backprojection";
         break;
        case E7_RECON_TYPE_PROMIS:
         s+="Forward Projection 3D (PROMIS)";
         break;
        case E7_RECON_TYPE_Ramp3D:
         s+="Ramp 3D";
         break;
        case E7_RECON_TYPE_FAVOR:
         s+="FAVOR 3D";
         break;
        case E7_RECON_TYPE_SSRB:
         s+="SSRB";
         break;
        case E7_RECON_TYPE_Rebinning:
         s+="Multi-slice rebinning";
         break;
        case E7_RECON_TYPE_FORE:
         s+="FORE";
         break;
        case E7_RECON_TYPE_USER_MANIP:
         s+="User Manipulated";
         break;
        default:
         s+="unknown";
         break;
      }
     sl->push_back(s+")");
     sl->push_back(" recon_views:                    "+
                   toString(ih.recon_views));
     /*
     for (i=0; i < 87; i++)
      sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                    toString(sh.fill_cti[i]));
     for (i=0; i < 48; i++)
      sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                    toString(sh.fill_user[i]));
     */
   }
   catch  (...)
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation between prone and supine.

    Change orientation between prone and supine. The dataset is flipped in
    y-direction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_IMAGE::Prone2Supine() const
 { utils_Prone2Supine(data, datatype, ih.x_dimension, ih.y_dimension,
                      ih.z_dimension);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of image matrix.
    \param[in] file   handle of file

    Store header of image matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_IMAGE::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                      // DataChanger is used to store data system independently
     dc=new DataChanger(E7_RECLEN, false, true);
                                                       // fill data into buffer
     dc->Value(ih.data_type);
     dc->Value(ih.num_dimensions);
     dc->Value(ih.x_dimension);
     dc->Value(ih.y_dimension);
     dc->Value(ih.z_dimension);
     dc->Value(ih.x_offset);
     dc->Value(ih.y_offset);
     dc->Value(ih.z_offset);
     dc->Value(ih.recon_zoom);
     dc->Value(ih.scale_factor);
     dc->Value(ih.image_min);
     dc->Value(ih.image_max);
     dc->Value(ih.x_pixel_size);
     dc->Value(ih.y_pixel_size);
     dc->Value(ih.z_pixel_size);
     dc->Value(ih.frame_duration);
     dc->Value(ih.frame_start_time);
     dc->Value(ih.filter_code);
     dc->Value(ih.x_resolution);
     dc->Value(ih.y_resolution);
     dc->Value(ih.z_resolution);
     dc->Value(ih.num_r_elements);
     dc->Value(ih.num_angles);
     dc->Value(ih.z_rotation_angle);
     dc->Value(ih.decay_corr_fctr);
     dc->Value(ih.processing_code);
     dc->Value(ih.gate_duration);
     dc->Value(ih.r_wave_offset);
     dc->Value(ih.num_accepted_beats);
     dc->Value(ih.filter_cutoff_frequency);
     dc->Value(ih.filter_resolution);
     dc->Value(ih.filter_ramp_slope);
     dc->Value(ih.filter_order);
     dc->Value(ih.filter_scatter_fraction);
     dc->Value(ih.filter_scatter_slope);
     dc->Value(40, ih.annotation);
     dc->Value(ih.mt_1_1);
     dc->Value(ih.mt_1_2);
     dc->Value(ih.mt_1_3);
     dc->Value(ih.mt_2_1);
     dc->Value(ih.mt_2_2);
     dc->Value(ih.mt_2_3);
     dc->Value(ih.mt_3_1);
     dc->Value(ih.mt_3_2);
     dc->Value(ih.mt_3_3);
     dc->Value(ih.rfilter_cutoff);
     dc->Value(ih.rfilter_resolution);
     dc->Value(ih.rfilter_code);
     dc->Value(ih.rfilter_order);
     dc->Value(ih.zfilter_cutoff);
     dc->Value(ih.zfilter_resolution);
     dc->Value(ih.zfilter_code);
     dc->Value(ih.zfilter_order);
     dc->Value(ih.mt_1_4);
     dc->Value(ih.mt_2_4);
     dc->Value(ih.mt_3_4);
     dc->Value(ih.scatter_type);
     dc->Value(ih.recon_type);
     dc->Value(ih.recon_views);
     for (i=0; i < 87; i++) dc->Value(ih.fill_cti[i]);
     for (i=0; i < 48; i++) dc->Value(ih.fill_user[i]);
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
void ECAT7_IMAGE::ScaleMatrix(const float factor)
 { utils_ScaleMatrix(data, datatype, (unsigned long int)ih.x_dimension*
                                     (unsigned long int)ih.y_dimension*
                                     (unsigned long int)ih.z_dimension,
                     factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request width of image.
    \return width of image
 
    Request width of image.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_IMAGE::Width() const
 { return(ih.x_dimension);
 }
