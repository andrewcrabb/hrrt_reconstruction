/*! \file ecat7_mainheader.h
    \brief This class implements the main header of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
 */

#ifndef _ECAT7_MAINHEADER_H
#define _ECAT7_MAINHEADER_H

#include <fstream>
#include <list>
#include <string>

/*- class definitions -------------------------------------------------------*/

class ECAT7_MAINHEADER
 { public:
                                    /*! main header of ECAT7 file (512 byte) */
    typedef struct {                /*! UNIX file type identification number */
                     unsigned char magic_number[14],
                                               /*! scan file's creation name */
                                   original_file_name[32];
                                                 /*! software version number */
                     signed short int sw_version,
                                      system_type,        /*!< scanner model */
                                      file_type;              /*!< file type */
                                             /*! serial number of the gantry */
                     unsigned char serial_number[10];
                                   /*! scan start time in sec since 1.1.1970 */
                     signed long int scan_start_time;
                     unsigned char isotope_name[8];        /*!< isotope name */
                     float isotope_halflife;    /*!< isotope halflife in sec */
                                                     /*! radiopharmaceutical */
                     unsigned char radiopharmaceutical[32];
                     float gantry_tilt,          /*!< gantry tilt in degrees */
                           gantry_rotation,  /*!< gantry rotation in degrees */
                                   /*! bed elevation in cm from lowest point */
                           bed_elevation,
                           intrinsic_tilt;    /*!< intrinsic tilt in degrees */
                     signed short int wobble_speed, /*!< wobble speed in rpm */
                                                /*! transmission source type */
                                      transm_source_type;
                     float distance_scanned,     /*!< distance scanned in cm */
                                       /*! diameter of transaxial view in cm */
                           transaxial_fov;
                     signed short int angular_compression,  /*!< mash factor */
                                               /*! coincidence sampling mode */
                                      coin_samp_mode,
                                                     /*! axial sampling mode */
                                      axial_samp_mode;
                                                 /*! ECAT calibration factor */
                     float ecat_calibration_factor;
                                                       /*! calibration units */
                     signed short int calibration_units,
                                              /*! label of calibration units */
                                      calibration_units_label,
                                      compression_code;/*!< compression code */
                     unsigned char study_type[12],     /*!< study descriptor */
                                   patient_id[16],           /*!< patient ID */
                                   patient_name[32],       /*!< patient name */
                                   patient_sex,             /*!< patient sex */
                                   patient_dexterity; /*!< patient dexterity */
                     float patient_age,            /*!< patient age in years */
                           patient_height,         /*!< patient height in cm */
                           patient_weight;         /*!< patient weight in kg */
                                /*! patient birth date in sec since 1.1.1970 */
                     signed long int patient_birth_date;
                     unsigned char physician_name[32],   /*!< physician name */
                                   operator_name[32],     /*!< operator name */
                                                      /*!< study description */
                                   study_description[32];
                     signed short int acquisition_type,/*!< acquisition type */
                                                     /*! patient orientation */
                                      patient_orientation;
                     unsigned char facility_name[20];     /*!< facility name */
                     signed short int num_planes,      /*!< number of planes */
                                      num_frames,      /*!< number of frames */
                                      num_gates,        /*!< number of gates */
                                                 /*! number of bed positions */
                                      num_bed_pos;
                                             /*! position of first bed in cm */
                     float init_bed_position,
                           bed_position[15],        /*!< bed positions in cm */
                           plane_separation;     /*!< plane separation in cm */
                                      /*! lower threshold for scatter in keV */
                     signed short int lwr_sctr_thres,
                                        /*! lower threshold for trues in keV */
                                      lwr_true_thres,
                                        /*! upper threshold for trues in keV */
                                      upr_true_thres;
                                       /*! user defined data processing code */
                     unsigned char user_process_code[10];
                     signed short int acquisition_mode;/*!< acquisition mode */
                     float bin_size,                     /*!< bin size in cm */
                                        /*! branching fraction for positrons */
                           branching_fraction;
                                    /*! injection time in sec since 1.1.1970 */
                     signed long int dose_start_time;
                     float dosage,   /*!< dosage in Bq/ccm at injection time */
                           well_counter_corr_factor;        /*!< not defined */
                     unsigned char data_units[32];          /*!< not defined */
                     signed short int septa_state,       /*!< septa position */
                                      fill_cti[6];   /*!< CPS reserved space */
                   } tmain_header7;
   private:
    tmain_header7 mh;             /*!< data structure for header information */
   public:
    ECAT7_MAINHEADER();                                    // initialize object
    ECAT7_MAINHEADER& operator = (const ECAT7_MAINHEADER &);    // '='-operator
                                    // request pointer to header data structure
    ECAT7_MAINHEADER::tmain_header7 *HeaderPtr();
    void LoadMainHeader(std::ifstream * const);             // load main header
                              // print main header information into string list
    void PrintMainHeader(std::list <std::string> * const) const;
    void SaveMainHeader(std::ofstream * const) const;      // store main header
 };

#endif
