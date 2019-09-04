/*! \class ECAT7_MAINHEADER ecat7_mainheader.h "ecat7_mainheader.h"
    \brief This class implements the main header of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class deals with main headers and provides methods to load, save and
    print these.
 */

#include <iostream>
#include <cstdio>
#include <ctime>
#include "ecat7_mainheader.h"
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
ECAT7_MAINHEADER::ECAT7_MAINHEADER()
 { memset(&mh, 0, sizeof(tmain_header7));                    // header is empty
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content

    This operator copies the main header.
 */
/*---------------------------------------------------------------------------*/
ECAT7_MAINHEADER& ECAT7_MAINHEADER::operator = (const ECAT7_MAINHEADER &e7)
 {                                   // copy header information into new object
   if (this != &e7) memcpy(&mh, &e7.mh, sizeof(tmain_header7));
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
    Request pointer to header data structure.
 */
/*---------------------------------------------------------------------------*/
ECAT7_MAINHEADER::tmain_header7 *ECAT7_MAINHEADER::HeaderPtr()
 { return(&mh);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load main header of ECAT7 file.
    \param[in] file   handle of file

    Load main header of ECAT7 file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_MAINHEADER::LoadMainHeader(std::ifstream * const file) { 
  DataChanger *dc = NULL;

   try
   { unsigned short int i;                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer
                                                   // retrieve data from buffer
     dc->Value(mh.magic_number, 14);
     dc->Value(mh.original_file_name, 32);
     dc->Value(&mh.sw_version);
     dc->Value(&mh.system_type);
     dc->Value(&mh.file_type);
     dc->Value(mh.serial_number, 10);
     dc->Value(&mh.scan_start_time);
     dc->Value(mh.isotope_name, 8);
     dc->Value(&mh.isotope_halflife);
     dc->Value(mh.radiopharmaceutical, 32);
     dc->Value(&mh.gantry_tilt);
     dc->Value(&mh.gantry_rotation);
     dc->Value(&mh.bed_elevation);
     dc->Value(&mh.intrinsic_tilt);
     dc->Value(&mh.wobble_speed);
     dc->Value(&mh.transm_source_type);
     dc->Value(&mh.distance_scanned);
     dc->Value(&mh.transaxial_fov);
     dc->Value(&mh.angular_compression);
     dc->Value(&mh.coin_samp_mode);
     dc->Value(&mh.axial_samp_mode);
     dc->Value(&mh.ecat_calibration_factor);
     dc->Value(&mh.calibration_units);       // ahc calibration_units used to be an int.  Now it's a scoped enum index to a map.
     dc->Value(&mh.calibration_units_label);
     dc->Value(&mh.compression_code);
     dc->Value(mh.study_type, 12);
     dc->Value(mh.patient_id, 16);
     dc->Value(mh.patient_name, 32);
     dc->Value(&mh.patient_sex);
     dc->Value(&mh.patient_dexterity);
     dc->Value(&mh.patient_age);
     dc->Value(&mh.patient_height);
     dc->Value(&mh.patient_weight);
     dc->Value(&mh.patient_birth_date);
     dc->Value(mh.physician_name, 32);
     dc->Value(mh.operator_name, 32);
     dc->Value(mh.study_description, 32);
     dc->Value(&mh.acquisition_type);
     dc->Value(&mh.patient_orientation);
     dc->Value(mh.facility_name, 20);
     dc->Value(&mh.num_planes);
     dc->Value(&mh.num_frames);
     dc->Value(&mh.num_gates);
     dc->Value(&mh.num_bed_pos);
     dc->Value(&mh.init_bed_position);
     for (i=0; i < 15; i++) dc->Value(&mh.bed_position[i]);
     dc->Value(&mh.plane_separation);
     dc->Value(&mh.lwr_sctr_thres);
     dc->Value(&mh.lwr_true_thres);
     dc->Value(&mh.upr_true_thres);
     dc->Value(mh.user_process_code, 10);
     dc->Value(&mh.acquisition_mode);
     dc->Value(&mh.bin_size);
     dc->Value(&mh.branching_fraction);
     dc->Value(&mh.dose_start_time);
     dc->Value(&mh.dosage);
     dc->Value(&mh.well_counter_corr_factor);
     dc->Value(mh.data_units, 32);
     dc->Value(&mh.septa_state);
     for (i=0; i < 6; i++) dc->Value(&mh.fill_cti[i]);
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

    Print header information into string list.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_MAINHEADER::PrintMainHeader(
                                      std::list <std::string> * const sl) const
 { unsigned short int i;
   time_t t;
   std::string ori[9]={ "Feet First Prone", "Head First Prone",
                    "Feet First Supine", "Head First Supine",
                    "Feet First Decubitus Right", "Head First Decubitus Right",
                    "Feet First Decubitus Left", "Head First Decubitus Left",
                    "Unknown Orientation" }, s;

   sl->push_back("***************** Main-Header *****************");
   sl->push_back(" Magic number:             "+
                 std::string((const char *)mh.magic_number));
   sl->push_back(" Original File Name:       "+
                 std::string((const char *)mh.original_file_name));
   sl->push_back(" Software Version:         "+toString(mh.sw_version));
   sl->push_back(" System type:              "+toString(mh.system_type));
   s=" File type:                "+toString(mh.file_type)+" (";
   // ahc
   s += ecat_matrix::data_set_types.at(mh.file_type).name;
   // switch (mh.file_type)
   //  { case E7_FILE_TYPE_Sinogram:
   //     s+="Sinogram";
   //     break;
   //    case E7_FILE_TYPE_Image16:
   //     s+="Image 16";
   //     break;
   //    case E7_FILE_TYPE_AttenuationCorrection:
   //     s+="Attenuation Correction";
   //     break;
   //    case E7_FILE_TYPE_Normalization:
   //     s+="Normalization";
   //     break;
   //    case E7_FILE_TYPE_PolarMap:
   //     s+="Polar Map";
   //     break;
   //    case E7_FILE_TYPE_Volume8:
   //     s+="Volume 8";
   //     break;
   //    case E7_FILE_TYPE_Volume16:
   //     s+="Volume 16";
   //     break;
   //    case E7_FILE_TYPE_Projection8:
   //     s+="Projection 8";
   //     break;
   //    case E7_FILE_TYPE_Projection16:
   //     s+="Projection 16";
   //     break;
   //    case E7_FILE_TYPE_Image8:
   //     s+="Image 8";
   //     break;
   //    case E7_FILE_TYPE_3D_Sinogram16:
   //     s+="3D Sinogram 16";
   //     break;
   //    case E7_FILE_TYPE_3D_Sinogram8:
   //     s+="3D Sinogram 8";
   //     break;
   //    case E7_FILE_TYPE_3D_Normalization:
   //     s+="3D Normalization";
   //     break;
   //    case E7_FILE_TYPE_3D_SinogramFloat:
   //     s+="3D Sinogram float";
   //     break;
   //    case E7_FILE_TYPE_Interfile:
   //     s+="Interfile";
   //     break;
   //    case E7_FILE_TYPE_unknown:
   //    default:
   //     s+="unknown";
   //     break;
   //  }
   sl->push_back(s+")");
   sl->push_back(" Serial Number:            "+                 std::string((const char *)mh.serial_number));
         t = mh.scan_start_time;
         { s=asctime(gmtime(&t));
           s.erase(s.length()-1, 1);
         }
   sl->push_back(" Scan Start Time:          "+s);
   sl->push_back(" Isotope Code:             "+                 std::string((const char *)mh.isotope_name));
   sl->push_back(" Isotope Half-life:        "+                 toString(mh.isotope_halflife)+" sec.");
   sl->push_back(" Radio-pharmaceutical:     "+                 std::string((const char *)mh.radiopharmaceutical));
   sl->push_back(" Gantry Tilt:              "+toString(mh.gantry_tilt)+                 " deg.");
   sl->push_back(" Gantry Rotation:          "+toString(mh.gantry_rotation)+                 " deg.");
   sl->push_back(" Bed Elevation:            "+toString(mh.bed_elevation)+                 " cm");
   sl->push_back(" Intrinsic Tilt:           "+toString(mh.intrinsic_tilt)+                 " deg.");
   if (mh.wobble_speed == 0)
    sl->push_back(" Wobble Speed:             not wobbled");
    else sl->push_back(" Wobble Speed:             "+toString(mh.wobble_speed)+                       " rpm");
   s=" Transmission Source Type: "+toString(mh.transm_source_type)+" (";
   switch (mh.transm_source_type)
    { case E7_TRANSM_SOURCE_TYPE_NoSrc: s+="None";         break;
      case E7_TRANSM_SOURCE_TYPE_Ring:  s+="Ring Source";  break;
      case E7_TRANSM_SOURCE_TYPE_Rod:   s+="Rod Source";   break;
      case E7_TRANSM_SOURCE_TYPE_Point: s+="Point Source"; break;
      default:                          s+="unknown";      break;
    }
   sl->push_back(s+")");
   sl->push_back(" Distance Scanned:         "+toString(mh.distance_scanned)+                 " cm");
   sl->push_back(" Transaxial FOV:           "+toString(mh.transaxial_fov)+                 " cm");
   s=" Angular Compression:      "+toString(mh.angular_compression)+" (";
   switch (mh.angular_compression)
    { case E7_ANGULAR_COMPRESSION_NoMash: s+="no mash";   break;
      case E7_ANGULAR_COMPRESSION_Mash2:  s+="mash of 2"; break;
      case E7_ANGULAR_COMPRESSION_Mash4:  s+="mash of 4"; break;
      default:                            s+="unknown";   break;
    }
   sl->push_back(s+")");
   s=" Coincidence Sample Mode:  "+toString(mh.coin_samp_mode)+" (";
   switch (mh.coin_samp_mode)
    { case E7_COIN_SAMP_MODE_NetTrues:
       s+="NetTrues";
       break;
      case E7_COIN_SAMP_MODE_PromptsDelayed:
       s+="Prompts And Delayed";
       break;
      case E7_COIN_SAMP_MODE_PromptsDelayedMultiples:
       s+="Prompts, Delayed and Multiples";
       break;
      case E7_COIN_SAMP_MODE_TruesTOF:
       s+="NetTrues, time-of-flight";
       break;
      default:
       s+="unknown";
       break;
    }
   sl->push_back(s+")");
   s=" Axial Sample Mode:        " + toString(mh.axial_samp_mode) + " (";
   switch (mh.axial_samp_mode)
    { case E7_AXIAL_SAMP_MODE_Normal: s+="Normal";  break;
      case E7_AXIAL_SAMP_MODE_2X:     s+="2X";      break;
      case E7_AXIAL_SAMP_MODE_3X:     s+="3X";      break;
      default:                        s+="unknown"; break;
    }
   sl->push_back(s+")");
   s=" Calibration Factor:       " + toString(mh.ecat_calibration_factor);
   sl->push_back(s);
   // s=" Calibration Units:        " + toString(mh.calibration_units) + " (";
   // switch (mh.calibration_units)
   //  { case E7_CALIBRATION_UNITS_Uncalibrated: s+="uncalibrated"; break;
   //    case E7_CALIBRATION_UNITS_Calibrated:   s+="calibrated";   break;
   //    default:                                s+="unknown";      break;
   //  }
   // sl->push_back(s+")");
   sl->push_back(fmt::format("Calibration Units:        {} ({})", to_underlying(mh.calibration_units), ecat_matrix::calibration_status_[mh.calibration_units].status));

   s=" Calibration Units Label:  " + toString(mh.calibration_units_label) + " (";
   switch (mh.calibration_units_label)
    { case E7_CALIBRATION_UNITS_LABEL_BloodFlow: s+="blood flow"; break;
      case E7_CALIBRATION_UNITS_LABEL_LMRGLU:    s+="lmrglu";     break;
      default:                                   s+="unknown";    break;
    }
   sl->push_back(s+")");
   s=" Compression Code:         " + toString(mh.compression_code) + " (";
   switch (mh.compression_code)
    { case E7_COMPRESSION_CODE_CompNone: s+="none";    break;
      default:                           s+="unknown"; break;
    }
   sl->push_back(s+")");
   sl->push_back(" Study Name:               "+                 std::string((const char *)mh.study_type));
   sl->push_back(" Patient ID:               "+                 std::string((const char *)mh.patient_id));
   sl->push_back("         Name:             "+                 std::string((const char *)mh.patient_name));
   s="         Sex:              ";
   if (mh.patient_sex == 0) 
    s+="0";
    else 
      s+=mh.patient_sex;
   s+=" (";
   switch (mh.patient_sex)
    { case E7_PATIENT_SEX_SexMale:    s+="male";    break;
      case E7_PATIENT_SEX_SexFemale:  s+="female";  break;
      case E7_PATIENT_SEX_SexUnknown:
      default:                        s+="unknown"; break;
    }
   sl->push_back(s+")");
   s="         Dexterity:        ";
   if (mh.patient_dexterity == 0) s+="0";
    else s+=mh.patient_dexterity;
   s+=" (";
   switch (mh.patient_dexterity)
    { case E7_PATIENT_DEXTERITY_DextRT:      s+="right";   break;
      case E7_PATIENT_DEXTERITY_DextLF:      s+="left";    break;
      case E7_PATIENT_DEXTERITY_DextUnknown:
      default:                               s+="unknown"; break;
    }
   sl->push_back(s+")");
   sl->push_back("         Age:              "+toString(mh.patient_age));
   sl->push_back("         Height:           "+toString(mh.patient_height)+
                 " cm");
   sl->push_back("         Weight:           "+toString(mh.patient_weight)+
                 " kg");
         { t = mh.scan_start_time;
           s=asctime(gmtime(&t));
           s.erase(s.length()-1, 1);
         }
   sl->push_back("         Birth Date:       "+s);
   sl->push_back(" Physician Name:           "+
                 std::string((const char *)mh.physician_name));
   sl->push_back(" Operator Name:            "+
                 std::string((const char *)mh.operator_name));
   sl->push_back(" Comment:                  "+
                 (std::string)(const char *)mh.study_description);
   s=" Acquisition Type:         "+toString(mh.acquisition_type)+" (";
   switch (mh.acquisition_type)
    { case E7_ACQUISITION_TYPE_Undef:
       s+="Undefined";
       break;
      case E7_ACQUISITION_TYPE_Blank:
       s+="Blank";
       break;
      case E7_ACQUISITION_TYPE_Transmission:
       s+="Transmission";
       break;
      case E7_ACQUISITION_TYPE_StaticEmission:
       s+="Static emission";
       break;
      case E7_ACQUISITION_TYPE_DynamicEmission:
       s+="Dynamic emission";
       break;
      case E7_ACQUISITION_TYPE_GatedEmission:
       s+="Gated emission";
       break;
      case E7_ACQUISITION_TYPE_TransmissionRectilinear:
       s+="Transmission rectilinear";
       break;
      case E7_ACQUISITION_TYPE_EmissionRectilinear:
       s+="Emission rectilinear";
       break;
      default:
       s+="unknown";
       break;
    }
   sl->push_back(s+")");
   sl->push_back(" Patient Orientation:      "+
                 toString(mh.patient_orientation)+" ("+
                 ori[mh.patient_orientation]+")");
   sl->push_back(" Facility Name:            "+
                 std::string((const char *)mh.facility_name));
   sl->push_back(" Number of Planes:         "+toString(mh.num_planes));
   sl->push_back(" Number of Frames:         "+toString(mh.num_frames));
   sl->push_back(" Number of Gates:          "+toString(mh.num_gates));
   sl->push_back(" Number of Bed Offsets:    "+toString(mh.num_bed_pos));
   sl->push_back(" Initial Bed Position:     "+toString(mh.init_bed_position)+
                 " cm");
   for (i=0; i < 15; i++)
    sl->push_back(" Bed offset ("+toString(i, 2)+"):          "+
                  toString(mh.bed_position[i])+" cm");
   sl->push_back(" Plane Separation:         "+toString(mh.plane_separation)+
                 " cm");
   sl->push_back(" Lower Scatter Threshold:  "+toString(mh.lwr_sctr_thres)+
                 " KeV");
   sl->push_back(" Lower Trues Threshold:    "+toString(mh.lwr_true_thres)+
                 " KeV");
   sl->push_back(" Upper Trues Threshold:    "+toString(mh.upr_true_thres)+
                 " KeV");
   sl->push_back(" User Process Code:        "+
                 std::string((const char *)mh.user_process_code));
   s=" Acquisition Mode:         "+toString(mh.acquisition_mode)+" (";
   switch (mh.acquisition_mode)
    { case E7_ACQUISITION_MODE_Normal:
       s+="Normal";
       break;
      case E7_ACQUISITION_MODE_Windowed:
       s+="Windowed";
       break;
      case E7_ACQUISITION_MODE_WindowedNonwindowed:
       s+="Windowed & Nonwindowed";
       break;
      case E7_ACQUISITION_MODE_DualEnergy:
       s+="Dual Energy";
       break;
      case E7_ACQUISITION_MODE_UpperEnergy:
       s+="Upper Energy";
       break;
      case E7_ACQUISITION_MODE_EmissionTransmission:
       s+="Emission and Transmission";
       break;
      default:
       s+="unknown";
       break;
    }
   sl->push_back(s+")");
   sl->push_back(" Bin Size:                 "+toString(mh.bin_size)+" cm");
   sl->push_back(" Branching Fraction:       "+
                 toString(mh.branching_fraction));
         { t = mh.scan_start_time;
           s=asctime(gmtime(&t));
           s.erase(s.length()-1, 1);
         }
   sl->push_back(" Dose Start Time:          "+s);
   sl->push_back(" Dosage:                   "+toString(mh.dosage)+
                 " bequerels/cc");
   sl->push_back(" Well Counter Factor:      "+
                 toString(mh.well_counter_corr_factor));
   sl->push_back(" Data Units:               "+
                 std::string((const char *)mh.data_units));
   s=" Septa Position:           "+toString(mh.septa_state)+" (";
   switch (mh.septa_state)
    { case E7_SEPTA_STATE_Extended:  s+="Septa Extended";     break;
      case E7_SEPTA_STATE_Retracted: s+="Septa Retracted";    break;
      case E7_SEPTA_STATE_NoSepta:   s+="No Septa Installed"; break;
      default:                       s+="unknown";            break;
    }
   sl->push_back(s+")");
   /* for (i=0; i < 5; i++)
       sl->push_back(" fill ("+toString(i)+"):                 "+
                     toString(mh.fill_cti[i]));
    */
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store main header in ECAT7 file.
    \param[in] file   handle of file

    Store main header in ECAT7 file.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_MAINHEADER::SaveMainHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;

     dc = new DataChanger(E7_RECLEN);
                                                        // put data into buffer
     dc->Value(14, mh.magic_number);
     dc->Value(32, mh.original_file_name);
     dc->Value(mh.sw_version);
     dc->Value(mh.system_type);
     dc->Value(mh.file_type);
     dc->Value(10, mh.serial_number);
     dc->Value(mh.scan_start_time);
     dc->Value(8, mh.isotope_name);
     dc->Value(mh.isotope_halflife);
     dc->Value(32, mh.radiopharmaceutical);
     dc->Value(mh.gantry_tilt);
     dc->Value(mh.gantry_rotation);
     dc->Value(mh.bed_elevation);
     dc->Value(mh.intrinsic_tilt);
     dc->Value(mh.wobble_speed);
     dc->Value(mh.transm_source_type);
     dc->Value(mh.distance_scanned);
     dc->Value(mh.transaxial_fov);
     dc->Value(mh.angular_compression);
     dc->Value(mh.coin_samp_mode);
     dc->Value(mh.axial_samp_mode);
     dc->Value(mh.ecat_calibration_factor);
     dc->Value(mh.calibration_units);
     dc->Value(mh.calibration_units_label);
     dc->Value(mh.compression_code);
     dc->Value(12, mh.study_type);
     dc->Value(16, mh.patient_id);
     dc->Value(32, mh.patient_name);
     dc->Value(mh.patient_sex);
     dc->Value(mh.patient_dexterity);
     dc->Value(mh.patient_age);
     dc->Value(mh.patient_height);
     dc->Value(mh.patient_weight);
     dc->Value(mh.patient_birth_date);
     dc->Value(32, mh.physician_name);
     dc->Value(32, mh.operator_name);
     dc->Value(32, mh.study_description);
     dc->Value(mh.acquisition_type);
     dc->Value(mh.patient_orientation);
     dc->Value(20, mh.facility_name);
     dc->Value(mh.num_planes);
     dc->Value(mh.num_frames);
     dc->Value(mh.num_gates);
     dc->Value(mh.num_bed_pos);
     dc->Value(mh.init_bed_position);
     for (i=0; i < 15; i++) dc->Value(mh.bed_position[i]);
     dc->Value(mh.plane_separation);
     dc->Value(mh.lwr_sctr_thres);
     dc->Value(mh.lwr_true_thres);
     dc->Value(mh.upr_true_thres);
     dc->Value(10, mh.user_process_code);
     dc->Value(mh.acquisition_mode);
     dc->Value(mh.bin_size);
     dc->Value(mh.branching_fraction);
     dc->Value(mh.dose_start_time);
     dc->Value(mh.dosage);
     dc->Value(mh.well_counter_corr_factor);
     dc->Value(32, mh.data_units);
     dc->Value(mh.septa_state);
     for (i=0; i < 6; i++) dc->Value(mh.fill_cti[i]);
     dc->SaveBuffer(file);                                      // store buffer
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }
