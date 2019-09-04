/*! \file ecat7_mainheader.h
    \brief This class implements the main header of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
 */

#pragma once

#include <fstream>
#include <list>
#include <string>

/*- class definitions -------------------------------------------------------*/

class ECAT7_MAINHEADER {
public:
  /*! main header of ECAT7 file (512 byte) */
  typedef struct {                /*! UNIX file type identification number */
    unsigned char    magic_number[14];
    unsigned char    original_file_name[32]; /*! scan file's creation name */
    signed short int sw_version;  /*! software version number */
    signed short int system_type;        /*!< scanner model */
    signed short     intfile_type;              /*!< file type */
    unsigned char    serial_number[10];  /*! serial number of the gantry */
    signed long int  scan_start_time;  /*! scan start time in sec since 1.1.1970 */
    unsigned char    isotope_name[8];        /*!< isotope name */
    float            isotope_halflife;    /*!< isotope halflife in sec */
    unsigned char    radiopharmaceutical[32];  /*! radiopharmaceutical */
    float            gantry_tilt          /*!< gantry tilt in degrees */
    float            gantry_rotation;  /*!< gantry rotation in degrees */
    float            bed_elevation;  /*! bed elevation in cm from lowest point */
    float            intrinsic_tilt;    /*!< intrinsic tilt in degrees */
    signed short int wobble_speed /*!< wobble speed in rpm */
    signed short int transm_source_type;  /*! transmission source type */
    float            distance_scanned;     /*!< distance scanned in cm */
    float            transaxial_fov;  /*! diameter of transaxial view in cm */
    signed short int angular_compression;  /*!< mash factor */
    signed short int coin_samp_mode;  /*! coincidence sampling mode */
    signed short int axial_samp_mode;  /*! axial sampling mode */
    float            ecat_calibration_factor;  /*! ECAT calibration factor */
    signed short int calibration_units;  /*! calibration units */
    signed short int calibration_units_label;  /*! label of calibration units */
    signed short int compression_code; /*!< compression code */
    unsigned char    study_type[12];     /*!< study descriptor */
    unsigned char    patient_id[16];           /*!< patient ID */
    unsigned char    patient_name[32];       /*!< patient name */
    unsigned char    patient_sex;             /*!< patient sex */
    unsigned char    patient_dexterity; /*!< patient dexterity */
    float            patient_age;            /*!< patient age in years */
    float            patient_height;         /*!< patient height in cm */
    float            patient_weight;         /*!< patient weight in kg */
    signed long int  patient_birth_date;  /*! patient birth date in sec since 1.1.1970 */
    unsigned char    physician_name[32];   /*!< physician name */
    unsigned char    operator_name[32];     /*!< operator name */
    unsigned char    study_description[32];  /*!< study description */
    signed short int acquisition_type;/*!< acquisition type */
    signed short int patient_orientation;  /*! patient orientation */
    unsigned char    facility_name[20];     /*!< facility name */
    signed short int num_planes;      /*!< number of planes */
    signed short int num_frames;      /*!< number of frames */
    signed short int num_gates;        /*!< number of gates */
    signed short int num_bed_pos;  /*! number of bed positions */
    float            init_bed_position;  /*! position of first bed in cm */
    float            bed_position[15];        /*!< bed positions in cm */
    float            plane_separation;     /*!< plane separation in cm */
    signed short int lwr_sctr_thres;  /*! lower threshold for scatter in keV */
    signed short int lwr_true_thres;  /*! lower threshold for trues in keV */
    signed short int upr_true_thres;  /*! upper threshold for trues in keV */
    unsigned char    user_process_code[10];  /*! user defined data processing code */
    signed short int acquisition_mode;/*!< acquisition mode */
    float            bin_size;                     /*!< bin size in cm */
    float            branching_fraction;  /*! branching fraction for positrons */
    signed long int  dose_start_time;  /*! injection time in sec since 1.1.1970 */
    float            dosage;   /*!< dosage in Bq/ccm at injection time */
    float            well_counter_corr_factor;        /*!< not defined */
    unsigned char    data_units[32];          /*!< not defined */
    signed short int septa_state;       /*!< septa position */
    signed short int fill_cti[6];   /*!< CPS reserved space */
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

