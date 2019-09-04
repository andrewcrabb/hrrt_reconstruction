/*! \file interfile.h
    \brief This class is used to read, write, create and modify Interfile
           headers.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments
    \date 2004/12/14 added main header keys for normalization code
    \date 2004/12/14 changed unit of tracer activity from MBq to mCi
    \date 2004/12/14 return unit of value when retrieving value
    \date 2004/12/14 changed unit of UTC keys to "sec"
    \date 2005/01/15 cleanup templates
    \date 2005/02/08 support negative number of trues
 */

#pragma once

#include <string>
#include <vector>
#include "kv.h"
#include "types.h"

using string;

/*- class definitions -------------------------------------------------------*/

class Interfile{
private:
  /*! where is a key used ? */
  typedef enum { SH,                              /*!< in sinogram headers */
                 IH,                                 /*!< in image headers */
                 NH,                                  /*!< in norm headers */
                 SIH,                   /*!< in sinogram and image headers */
                 ALL                                   /*!< in all headers */
               } tused_where;
  /*! structure for element of parser table */
  typedef struct {
    string key,                       /*!< name of key */
        max_lindex_key,  /*! name of key that specifies maximum index of list in array */
        max_index_key;   /*! name of key that specifies maximum index of array */
    tused_where where;   /*!< in what headers is key used ? */
    std::vector <string> units;  /*! allowed units for this key */
    KV::ttoken_type type;                /*!< type of value */
    bool need_value;                 /*!< key needs value ? */
  } ttoken;
  std::vector <ttoken *> smh,   /*!< parser table for sinogram main header */
      ssh,     /*!< parser table for sinogram subheader */
      nh;             /*!< parser table for norm header */
  /*! key/value table from parsed sinogram main header */
  std::vector <KV *> kv_smh,
      /*! key/value table from parsed sinogram subheader */
      kv_ssh,
      kv_nh;   /*!< key/value table from parsed norm header */
  string main_headerfile,      /*!< name of sinogram main header file */
      main_headerpath,      /*!< path of sinogram main header file */
      subheaderfile,          /*!< name of sinogram subheader file */
      subheaderpath,          /*!< path of sinogram subheader file */
      norm_headerfile,               /*!< name of norm header file */
      norm_headerpath;               /*!< path of norm header file */
  unsigned short int last_pos[2];/*!< remember positions of last used keys */
  /*! function number for determination of evaluation order */
  unsigned short int eo;
  bool fpf;                           /*!< evalutation order of parameters */
  // add a blank line to key/value list
  void blankLine(const string, std::vector <KV *> *);
  // check if unit of key is valid
  void checkUnit(std::vector <ttoken *>, const string, string,
                 string, const string) const;
  // delete key/value pair
  void deleteKV(std::vector <KV *> *, KV *, const string);
  void deleteKVList(std::vector <KV *> *);           // delete key/value list
  void deleteParserTable(std::vector <ttoken *> *);    // delete parser table
  // helper function to determine the evaluation order of parameters
  unsigned short int eo_fct(unsigned short int);
  // determine evaluation order of parameters
  bool evaluationOrder(unsigned short int, unsigned short int) const;
  // get value of key from header
  template <typename T>
  T getValue(const string, std::vector <KV *>, const string,
             const string);
  template <typename T>
  T getValue(const string, const unsigned short int,
             std::vector <KV *>, const string, const string);
  // create entry in parser table
  ttoken *initKV(const tused_where, const string, string,
                 const KV::ttoken_type, const bool) const;
  ttoken *initKV(const tused_where, const string,
                 const KV::ttoken_type, const bool) const;
  // create entry in parse table for key[index]
  ttoken *initKV(const tused_where, const string, string,
                 const KV::ttoken_type, const string, const bool) const;
  ttoken *initKV(const tused_where, const string, string,
                 const KV::ttoken_type, const string, const string,
                 const bool) const;
  ttoken *initKV(const tused_where, const string, const KV::ttoken_type,
                 const string, const bool) const;
  ttoken *initKV(const tused_where, const string, const KV::ttoken_type,
                 const string, const string, const bool) const;
  void initMain();   // create parser table for sinogram or image main header
  void initNorm();                     // create parser table for norm header
  void initSub();      // create parser table for sinogram or image subheader
  // put key that was searched pre-last before key that was searched last
  void keyResort(std::vector <KV *> *, bool) const;
  // save header
  void saveHeader(const string, std::vector <ttoken *>,
                  std::vector <KV *>);
  // search key in key/value list
  KV *searchKey(const string, std::vector <KV *>, const string);
  // parse key/value list for semantic errors
  void semanticParser(const string, std::vector <ttoken *> sh,
                      std::vector <KV *>);
  // add separator key to key/value list
  void separatorKey(const string, const string,
                    std::vector <KV *> *);
  // add key/value pair to header
  template <typename T>
  void setValue(const string, const T, string, const string,
                const string, const unsigned long int,
                const string, std::vector <KV *> *,
                std::vector <ttoken *>);
  template <typename T>
  void setValue(const string, const T, const string,
                const string, const unsigned long int,
                const string, std::vector <KV *> *,
                std::vector <ttoken *>);
  template <typename T>
  void setValue(const string, const T, const unsigned short int,
                const string, const string, const string,
                const unsigned long int, const string,
                std::vector <KV *> *, std::vector <ttoken *>);
  template <typename T>
  void setValue(const string, const T, const unsigned short int,
                const string, const string,
                const unsigned long int, const string,
                std::vector <KV *> *, std::vector <ttoken *>, const bool);
  template <typename T>
  void setValue(const string, const T, const unsigned short int,
                const unsigned short int, const string,
                const string, const unsigned long int,
                const string, std::vector <KV *> *,
                std::vector <ttoken *>);
  // read file and parse for syntax errors
  void syntaxParser(const string, std::vector <ttoken *>,
                    std::vector <KV *> *);
  // create entry in key/value list for parsed line
  void tokenize(string, string, string, const string,
                const string, std::vector <ttoken *>,
                std::vector <KV *> *, const string) const;
public:
  // export the data types from the KV object (just for convenience)
  typedef KV::tsex tsex;                            /*!< gender of patient */
  /*! patient orientation */
  typedef KV::tpatient_orientation tpatient_orientation;
  typedef KV::tscan_direction tscan_direction;         /*!< scan direction */
  typedef KV::tpet_data_type tpet_data_type;            /*!< PET data type */
  typedef KV::tnumber_format tnumber_format;            /*!< number format */
  /*! data type description */
  typedef KV::tdata_type_description tdata_type_description;
  typedef KV::tendian tendian;                              /*!< endianess */
  typedef KV::tdataset tdataset;  /*!< dataset name and acquisition number */
  typedef KV::tsepta tsepta;                              /*!< septa state */
  typedef KV::tdetector_motion tdetector_motion;      /*!< detector motion */
  typedef KV::texamtype texamtype;                          /*!< exam type */
  typedef KV::tacqcondition tacqcondition;      /*!< acquisition condition */
  typedef KV::tscanner_type tscanner_type;               /*!< scanner type */
  typedef KV::tslice_orientation tslice_orientation;/*!< slice orientation */
  /*! decay correction information */
  typedef KV::tdecay_correction tdecay_correction;
  /*! randoms correction information */
  typedef KV::trandom_correction trandom_correction;
  typedef KV::tprocess_status tprocess_status;      /*!< processing status */
  /*! types of scans */
  typedef enum { EMISSION_TYPE,                         /*!< emission scan */
                 TRANSMISSION_TYPE                  /*!< transmission scan */
               } tscan_type;
  /*! sinogram modes */
  typedef enum { TRUES_MODE,                                    /*!< trues */
                 PROMPTS_MODE,                                /*!< prompts */
                 RANDOMS_MODE                                 /*!< randoms */
               } tsino_mode;
  // interfile file extensions
  // main header extension
  static const string INTERFILE_MAINHDR_EXTENSION,
         // emission subheader extension
         INTERFILE_SSUBHDR_EXTENSION,
         // emission sinogram data file extension
         INTERFILE_SDATA_EXTENSION,
         // transmission sinogram subheader extension
         INTERFILE_ASUBHDR_EXTENSION,
         // transmission sinogram data file extension
         INTERFILE_ADATA_EXTENSION,
         INTERFILE_NHDR_EXTENSION, // norm header extension
         // norm data file extension
         INTERFILE_NDATA_EXTENSION,
         INTERFILE_IHDR_EXTENSION,// image header extension
         // image data file extension
         INTERFILE_IDATA_EXTENSION;
  Interfile();                                               // create object
  ~Interfile();                                             // destroy object
  Interfile& operator = (const Interfile &);                  // '='-operator
  // encode acquisition Code
  unsigned long int acquisitionCode(const tscan_type,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const tsino_mode,
                                    const unsigned short int) const;
  // decode acquisition code
  void acquisitionCode(unsigned long int, tscan_type * const,
                       unsigned short int * const,
                       unsigned short int * const,
                       unsigned short int * const,
                       unsigned short int * const, tsino_mode * const,
                       unsigned short int * const) const;
  void convertToImageMainHeader();  // remove all non image main header token
  void convertToImageSubheader();     // remove all non image subheader token
  void convertToSinoMainHeader();// remove all non sinogram main header token
  void convertToSinoSubheader();   // remove all non sinogram subheader token
  void createMainHeader(const string);             // create main header
  void createNormHeader(const string);             // create norm header
  void createSubheader(const string);                // create subheader
  void loadMainHeader(const string);                 // load main header
  void loadNormHeader(const string);        // load normalization header
  void loadSubheader(const string);                    // load subheader
  // convert date and time into UTC
  static void time2UTC(TIMEDATE::tdate const, TIMEDATE::ttime const,
                       time_t * const);
  // convert UTC into date and time
  static void timeUTC2local(time_t, TIMEDATE::tdate * const,
                            TIMEDATE::ttime * const);
  // add blank line to key/value list of main header
  unsigned long int Main_blank_line(const string comment = string());
  // put key of first parameter after key of second paramater
  template <typename T, typename U>
  void MainKeyAfterKey(T, U);
  // put key of first parameter before key of second paramater
  template <typename T, typename U>
  void MainKeyBeforeKey(T, U);
  // add blank line to key/value list of main header
  unsigned long int Norm_blank_line(const string comment = string());
  // put key of first parameter after key of second paramater
  template <typename T, typename U>
  void NormKeyAfterKey(T, U);
  // put key of first parameter before key of second paramater
  template <typename T, typename U>
  void NormKeyBeforeKey(T, U);
  void saveMainHeader(const string);                 // save main header
  void saveMainHeader();
  void saveNormHeader(const string);                 // save norm header
  void saveNormHeader();
  void saveSubheader(const string);                    // save subheader
  void saveSubheader();
  // add blank line to key/value list of subheader
  unsigned long int Sub_blank_line(const string comment = string());
  // put key of first parameter after key of second paramater
  template <typename T, typename U>
  void SubKeyAfterKey(T, U);
  // put key of first parameter before key of second paramater
  template <typename T, typename U>
  void SubKeyBeforeKey(T, U);
  // request values from main header
  string Main_comment();
  string Main_originating_system();
  string Main_CPS_data_type();
  string Main_CPS_version_number();
  string Main_gantry_serial_number();
  string Main_original_institution();
  string Main_data_description();
  string Main_patient_name();
  string Main_patient_ID();
  TIMEDATE::tdate Main_patient_DOB(string * const);
  tsex Main_patient_sex();
  float Main_patient_height(string * const);
  float Main_patient_weight(string * const);
  string Main_study_ID();
  texamtype Main_exam_type();
  TIMEDATE::tdate Main_study_date(string * const);
  TIMEDATE::ttime Main_study_time(string * const);
  unsigned long int Main_study_UTC(string * const);
  string Main_isotope_name();
  float Main_isotope_gamma_halflife(string * const);
  float Main_isotope_branching_factor();
  string Main_radiopharmaceutical();
  TIMEDATE::tdate Main_tracer_injection_date(string * const);
  TIMEDATE::ttime Main_tracer_injection_time(string * const);
  unsigned long int Main_tracer_injection_UTC(string * const);
  signed long int Main_relative_time_of_tracer_injection(
    string * const);
  float Main_tracer_activity_at_time_of_injection(string * const);
  float Main_injected_volume(string * const);
  tpatient_orientation Main_patient_orientation();
  tpatient_orientation Main_patient_scan_orientation();
  tscan_direction Main_scan_direction();
  float Main_coincidence_window_width(string * const);
  tsepta Main_septa_state();
  float Main_gantry_tilt_angle(string * const);
  float Main_gantry_slew(string * const);
  tdetector_motion Main_type_of_detector_motion();
  unsigned short int Main_number_of_time_frames();
  unsigned short int Main_number_of_horizontal_bed_offsets();
  unsigned short int Main_number_of_time_windows();
  unsigned short int Main_number_of_energy_windows();
  unsigned short int Main_energy_window_lower_level(
    const unsigned short int,
    string * const);
  unsigned short int Main_energy_window_upper_level(
    const unsigned short int,
    string * const);
  unsigned short int Main_number_of_emission_data_types();
  tdata_type_description Main_emission_data_type_description(
    const unsigned short int);
  unsigned short int Main_number_of_transmission_data_types();
  tdata_type_description Main_transmission_data_type_description(
    const unsigned short int);
  unsigned short int Main_number_of_TOF_time_bins();
  float Main_TOF_bin_width(string * const);
  float Main_TOF_gaussian_fwhm(string * const);
  float Main_scanner_quantification_factor(string * const);
  TIMEDATE::tdate Main_calibration_date(string * const);
  TIMEDATE::ttime Main_calibration_time(string * const);
  unsigned long int Main_calibration_UTC(string * const);
  float Main_dead_time_quantification_factor(const unsigned short int);

  unsigned short int Main_total_number_of_data_sets();
  tdataset Main_data_set(const unsigned short int);
  tscanner_type Main_PET_scanner_type();
  float Main_transaxial_FOV_diameter(string * const);
  unsigned short int Main_number_of_rings();
  float Main_distance_between_rings(string * const);
  float Main_bin_size(string * const);
  string Main_source_model();
  string Main_source_serial();
  string Main_source_shape();
  unsigned short int Main_source_radii();
  float Main_source_radius(const unsigned short int, string * const);
  float Main_source_length(string * const);
  unsigned short int Main_source_densities();
  float Main_source_mu(const unsigned short int, string * const);
  unsigned short int Main_source_ratios();
  float Main_source_ratio(const unsigned short int);
  // set values in main header
  unsigned long int Main_INTERFILE(const string comment = string());
  unsigned long int Main_comment(const string,
                                 const string comment = string());
  unsigned long int Main_originating_system(const string,
      const string comment = string());
  unsigned long int Main_CPS_data_type(const string,
                                       const string comment = string());
  unsigned long int Main_CPS_version_number(const string,
      const string comment = string());
  unsigned long int Main_gantry_serial_number(const string,
      const string comment = string());
  unsigned long int Main_GENERAL_DATA(
    const string comment = string());
  unsigned long int Main_original_institution(const string,
      const string comment = string());
  unsigned long int Main_data_description(const string,
                                          const string comment = string());
  unsigned long int Main_patient_name(const string,
                                      const string comment = string());
  unsigned long int Main_patient_ID(const string,
                                    const string comment = string());
  unsigned long int Main_patient_DOB(const TIMEDATE::tdate,
                                     const string,
                                     const string comment = string());
  unsigned long int Main_patient_sex(const tsex,
                                     const string comment = string());
  unsigned long int Main_patient_height(const float, const string,
                                        const string comment = string());
  unsigned long int Main_patient_weight(const float, const string,
                                        const string comment = string());
  unsigned long int Main_study_ID(const string,
                                  const string comment = string());
  unsigned long int Main_exam_type(const texamtype,
                                   const string comment = string());
  unsigned long int Main_study_date(const TIMEDATE::tdate, const string,
                                    const string comment = string());
  unsigned long int Main_study_time(const TIMEDATE::ttime, const string,
                                    const string comment = string());
  unsigned long int Main_study_UTC(const unsigned long int,
                                   const string,
                                   const string comment = string());
  unsigned long int Main_isotope_name(const string,
                                      const string comment = string());
  unsigned long int Main_isotope_gamma_halflife(const float,
      const string,
      const string comment = string());
  unsigned long int Main_isotope_branching_factor(const float,
      const string comment = string());
  unsigned long int Main_radiopharmaceutical(const string,
      const string comment = string());
  unsigned long int Main_tracer_injection_date(const TIMEDATE::tdate,
      const string,
      const string comment = string());
  unsigned long int Main_tracer_injection_time(const TIMEDATE::ttime,
      const string,
      const string comment = string());
  unsigned long int Main_tracer_injection_UTC(const unsigned long int,
      const string,
      const string comment = string());
  unsigned long int Main_relative_time_of_tracer_injection(
    const signed long int, const string,
    const string comment = string());
  unsigned long int Main_tracer_activity_at_time_of_injection(const float,
      const string,
      const string comment = string());
  unsigned long int Main_injected_volume(const float, const string,
                                         const string comment = string());
  unsigned long int Main_patient_orientation(const tpatient_orientation,
      const string comment = string());
  unsigned long int Main_patient_scan_orientation(const tpatient_orientation,
      const string comment = string());
  unsigned long int Main_scan_direction(const tscan_direction,
                                        const string comment = string());
  unsigned long int Main_coincidence_window_width(const float,
      const string,
      const string comment = string());
  unsigned long int Main_septa_state(const tsepta,
                                     const string comment = string());
  unsigned long int Main_gantry_tilt_angle(const float, const string,
      const string comment = string());
  unsigned long int Main_gantry_slew(const float, const string,
                                     const string comment = string());
  unsigned long int Main_type_of_detector_motion(const tdetector_motion,
      const string comment = string());
  unsigned long int Main_DATA_MATRIX_DESCRIPTION(
    const string comment = string());
  unsigned long int Main_number_of_time_frames(const unsigned short int,
      const string comment = string());
  unsigned long int Main_number_of_horizontal_bed_offsets(
    const unsigned short int,
    const string comment = string());
  unsigned long int Main_number_of_time_windows(const unsigned short int,
      const string comment = string());
  unsigned long int Main_number_of_energy_windows(const unsigned short int,
      const string comment = string());
  unsigned long int Main_energy_window_lower_level(const unsigned short int,
      const unsigned short int,
      const string,
      const string comment = string());
  unsigned long int Main_energy_window_upper_level(const unsigned short int,
      const unsigned short int,
      const string,
      const string comment = string());
  unsigned long int Main_number_of_emission_data_types(
    const unsigned short int,
    const string comment = string());
  unsigned long int Main_emission_data_type_description(
    const tdata_type_description,
    const unsigned short int,
    const string comment = string());
  unsigned long int Main_number_of_transmission_data_types(
    const unsigned short int,
    const string comment = string());
  unsigned long int Main_transmission_data_type_description(
    const tdata_type_description,
    const unsigned short int,
    const string comment = string());
  unsigned long int Main_number_of_TOF_time_bins(const unsigned short int,
      const string comment = string());
  unsigned long int Main_TOF_bin_width(const float, const string,
                                       const string comment = string());
  unsigned long int Main_TOF_gaussian_fwhm(const float, const string,
      const string comment = string());
  unsigned long int Main_GLOBAL_SCANNER_CALIBRATION_FACTOR(
    const string comment = string());
  unsigned long int Main_scanner_quantification_factor(const float,
      const string,
      const string comment = string());
  unsigned long int Main_calibration_date(const TIMEDATE::tdate,
                                          const string,
                                          const string comment = string());
  unsigned long int Main_calibration_time(const TIMEDATE::ttime,
                                          const string,
                                          const string comment = string());
  unsigned long int Main_calibration_UTC(const unsigned long int,
                                         const string,
                                         const string comment = string());
  unsigned long int Main_dead_time_quantification_factor(const float,
      const unsigned short int,
      const string comment = string());
  unsigned long int Main_DATA_SET_DESCRIPTION(
    const string comment = string());
  unsigned long int Main_total_number_of_data_sets(const unsigned short int,
      const string comment = string());
  unsigned long int Main_data_set(const tdataset, const unsigned short int,
                                  const string comment = string());
  unsigned long int Main_SUPPLEMENTARY_ATTRIBUTES(
    const string comment = string());
  unsigned long int Main_PET_scanner_type(const tscanner_type,
                                          const string comment = string());
  unsigned long int Main_transaxial_FOV_diameter(const float,
      const string,
      const string comment = string());
  unsigned long int Main_number_of_rings(const unsigned short int,
                                         const string comment = string());
  unsigned long int Main_distance_between_rings(const float,
      const string,
      const string comment = string());
  unsigned long int Main_bin_size(const float, const string,
                                  const string comment = string());
  unsigned long int Main_source_model(const string,
                                      const string comment = string());
  unsigned long int Main_source_serial(const string,
                                       const string comment = string());
  unsigned long int Main_source_shape(const string,
                                      const string comment = string());
  unsigned long int Main_source_radii(const unsigned short int,
                                      const string comment = string());
  unsigned long int Main_source_radius(const float, const unsigned short int,
                                       const string,
                                       const string comment = string());
  unsigned long int Main_source_length(const float, const string,
                                       const string comment = string());
  unsigned long int Main_source_densities(const unsigned short int,
                                          const string comment = string());
  unsigned long int Main_source_mu(const float, const unsigned short int,
                                   const string,
                                   const string comment = string());
  unsigned long int Main_source_ratios(const unsigned short int,
                                       const string comment = string());
  unsigned long int Main_source_ratio(const float, const unsigned short int,
                                      const string comment = string());
  // empty array
  unsigned long int Main_energy_window_lower_level_remove(
    const unsigned short int);
  unsigned long int Main_energy_window_lower_level_remove();
  unsigned long int Main_energy_window_upper_level_remove(
    const unsigned short int);
  unsigned long int Main_energy_window_upper_level_remove();
  unsigned long int Main_emission_data_type_description_remove(
    const unsigned short int);
  unsigned long int Main_emission_data_type_description_remove();
  unsigned long int Main_transmission_data_type_description_remove(
    const unsigned short int);
  unsigned long int Main_transmission_data_type_description_remove();
  unsigned long int Main_data_set_remove(const unsigned short int);
  unsigned long int Main_data_set_remove();
  unsigned long int Main_source_radius_remove(const unsigned short int);
  unsigned long int Main_source_radius_remove();
  unsigned long int Main_source_mu_remove(const unsigned short int);
  unsigned long int Main_source_mu_remove();
  unsigned long int Main_source_ratio_remove(const unsigned short int);
  unsigned long int Main_source_ratio_remove();
  // request values from norm header
  string Norm_comment();
  string Norm_originating_system();
  string Norm_CPS_data_type();
  string Norm_CPS_version_number();
  string Norm_gantry_serial_number();
  string Norm_data_description();
  string Norm_name_of_data_file();
  TIMEDATE::tdate Norm_expiration_date(string * const);
  TIMEDATE::ttime Norm_expiration_time(string * const);
  unsigned long int Norm_expiration_UTC(string * const);
  TIMEDATE::tdate Norm_study_date(string * const);
  TIMEDATE::ttime Norm_study_time(string * const);
  unsigned long int Norm_study_UTC(string * const);
  tendian Norm_image_data_byte_order();
  tpet_data_type Norm_PET_data_type();
  string Norm_data_format();
  tnumber_format Norm_number_format();
  unsigned short int Norm_number_of_bytes_per_pixel();
  unsigned short int Norm_number_of_normalization_scans();
  string Norm_normalization_scan(const unsigned short int);
  string Norm_isotope_name(const unsigned short int);
  string Norm_radiopharmaceutical(const unsigned short int);
  uint64 Norm_total_prompts(const unsigned short int);
  uint64 Norm_total_randoms(const unsigned short int);
  int64 Norm_total_net_trues(const unsigned short int);
  unsigned long int Norm_image_duration(const unsigned short int,
                                        string * const);
  unsigned short int Norm_number_of_bucket_rings(const unsigned short int);
  uint64 Norm_total_uncorrected_singles(const unsigned short int);
  unsigned short int Norm_number_of_normalization_components();
  string Norm_normalization_component(const unsigned short int);
  unsigned short int Norm_number_of_dimensions(const unsigned short int);
  unsigned short int Norm_matrix_size(const unsigned short int,
                                      const unsigned short int);
  string Norm_matrix_axis_label(const unsigned short int,
                                     const unsigned short int);
  string Norm_matrix_axis_unit(const unsigned short int,
                                    const unsigned short int);
  float Norm_scale_factor(const unsigned short int,
                          const unsigned short int);
  uint64 Norm_data_offset_in_bytes(const unsigned short int);
  unsigned short int Norm_axial_compression();
  unsigned short int Norm_maximum_ring_difference();
  unsigned short int Norm_number_of_rings();
  unsigned short int Norm_number_of_energy_windows();
  unsigned short int Norm_energy_window_lower_level(
    const unsigned short int,
    string * const);
  unsigned short int Norm_energy_window_upper_level(
    const unsigned short int,
    string * const);
  float Norm_scanner_quantification_factor(string * const);
  TIMEDATE::tdate Norm_calibration_date(string * const);
  TIMEDATE::ttime Norm_calibration_time(string * const);
  unsigned long int Norm_calibration_UTC(string * const);
  float Norm_dead_time_quantification_factor(const unsigned short int);
  unsigned short int Norm_total_number_of_data_sets();
  tdataset Norm_data_set(const unsigned short int);
  // set values in norm header
  unsigned long int Norm_INTERFILE(const string comment = string());
  unsigned long int Norm_comment(const string,
                                 const string comment = string());
  unsigned long int Norm_originating_system(const string,
      const string comment = string());
  unsigned long int Norm_CPS_data_type(const string,
                                       const string comment = string());
  unsigned long int Norm_CPS_version_number(const string,
      const string comment = string());
  unsigned long int Norm_gantry_serial_number(const string,
      const string comment = string());
  unsigned long int Norm_GENERAL_DATA(
    const string comment = string());
  unsigned long int Norm_data_description(const string,
                                          const string comment = string());
  unsigned long int Norm_name_of_data_file(const string,
      const string comment = string());
  unsigned long int Norm_expiration_date(const TIMEDATE::tdate,
                                         const string,
                                         const string comment = string());
  unsigned long int Norm_expiration_time(const TIMEDATE::ttime,
                                         const string,
                                         const string comment = string());
  unsigned long int Norm_expiration_UTC(const unsigned long int,
                                        const string,
                                        const string comment = string());
  unsigned long int Norm_GENERAL_IMAGE_DATA(
    const string comment = string());
  unsigned long int Norm_study_date(const TIMEDATE::tdate, const string,
                                    const string comment = string());
  unsigned long int Norm_study_time(const TIMEDATE::ttime, const string,
                                    const string comment = string());
  unsigned long int Norm_study_UTC(const unsigned long int,
                                   const string,
                                   const string comment = string());
  unsigned long int Norm_image_data_byte_order(const tendian,
      const string comment = string());
  unsigned long int Norm_PET_data_type(const tpet_data_type,
                                       const string comment = string());
  unsigned long int Norm_data_format(const string,
                                     const string comment = string());
  unsigned long int Norm_number_format(const tnumber_format,
                                       const string comment = string());
  unsigned long int Norm_number_of_bytes_per_pixel(const unsigned short int,
      const string comment = string());
  unsigned long int Norm_RAW_NORMALIZATION_SCANS_DESCRIPTION(
    const string comment = string());
  unsigned long int Norm_number_of_normalization_scans(
    const unsigned short int,
    const string comment = string());
  unsigned long int Norm_normalization_scan(const string,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_isotope_name(const string,
                                      const unsigned short int,
                                      const string comment = string());
  unsigned long int Norm_radiopharmaceutical(const string,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_total_prompts(const uint64,
                                       const unsigned short int,
                                       const string comment = string());
  unsigned long int Norm_total_randoms(const uint64,
                                       const unsigned short int,
                                       const string comment = string());
  unsigned long int Norm_total_net_trues(const int64,
                                         const unsigned short int,
                                         const string comment = string());
  unsigned long int Norm_image_duration(const unsigned long int,
                                        const unsigned short int,
                                        const string,
                                        const string comment = string());
  unsigned long int Norm_number_of_bucket_rings(const unsigned short int,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_total_uncorrected_singles(
    const uint64, const unsigned short int,
    const string comment = string());
  unsigned long int Norm_NORMALIZATION_COMPONENTS_DESCRIPTION(
    const string comment = string());
  unsigned long int Norm_number_of_normalization_components(
    const unsigned short int,
    const string comment = string());
  unsigned long int Norm_normalization_component(const string,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_number_of_dimensions(const unsigned short int,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_matrix_size(const unsigned short int,
                                     const unsigned short int,
                                     const unsigned short int,
                                     const string comment = string());
  unsigned long int Norm_matrix_axis_label(const string,
      const unsigned short int,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_matrix_axis_unit(const string,
                                          const unsigned short int,
                                          const unsigned short int,
                                          const string comment = string());
  unsigned long int Norm_scale_factor(const float, const unsigned short int,
                                      const unsigned short int,
                                      const string comment = string());
  unsigned long int Norm_data_offset_in_bytes(const uint64,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_axial_compression(const unsigned short int,
      const string comment = string());
  unsigned long int Norm_maximum_ring_difference(const unsigned short int,
      const string comment = string());
  unsigned long int Norm_number_of_rings(const unsigned short int,
                                         const string comment = string());
  unsigned long int Norm_number_of_energy_windows(const unsigned short int,
      const string comment = string());
  unsigned long int Norm_energy_window_lower_level(const unsigned short int,
      const unsigned short int,
      const string,
      const string comment = string());
  unsigned long int Norm_energy_window_upper_level(const unsigned short int,
      const unsigned short int,
      const string,
      const string comment = string());
  unsigned long int Norm_GLOBAL_SCANNER_CALIBRATION_FACTOR(
    const string comment = string());
  unsigned long int Norm_scanner_quantification_factor(const float,
      const string,
      const string comment = string());
  unsigned long int Norm_calibration_date(const TIMEDATE::tdate,
                                          const string,
                                          const string comment = string());
  unsigned long int Norm_calibration_time(const TIMEDATE::ttime,
                                          const string,
                                          const string comment = string());
  unsigned long int Norm_calibration_UTC(const unsigned long int,
                                         const string,
                                         const string comment = string());
  unsigned long int Norm_dead_time_quantification_factor(const float,
      const unsigned short int,
      const string comment = string());
  unsigned long int Norm_DATA_SET_DESCRIPTION(
    const string comment = string());
  unsigned long int Norm_total_number_of_data_sets(const unsigned short int,
      const string comment = string());
  unsigned long int Norm_data_set(const tdataset, const unsigned short int,
                                  const string comment = string());
  // empty array
  unsigned long int Norm_normalization_scan_remove(const unsigned short int);
  unsigned long int Norm_normalization_scan_remove();
  unsigned long int Norm_isotope_name_remove(const unsigned short int);
  unsigned long int Norm_isotope_name_remove();
  unsigned long int Norm_radiopharmaceutical_remove(
    const unsigned short int);
  unsigned long int Norm_radiopharmaceutical_remove();
  unsigned long int Norm_total_prompts_remove(const unsigned short int);
  unsigned long int Norm_total_prompts_remove();
  unsigned long int Norm_total_randoms_remove(const unsigned short int);
  unsigned long int Norm_total_randoms_remove();
  unsigned long int Norm_total_net_trues_remove(const unsigned short int);
  unsigned long int Norm_total_net_trues_remove();
  unsigned long int Norm_image_duration_remove(const unsigned short int);
  unsigned long int Norm_image_duration_remove();
  unsigned long int Norm_number_of_bucket_rings_remove(
    const unsigned short int);
  unsigned long int Norm_number_of_bucket_rings_remove();
  unsigned long int Norm_total_uncorrected_singles_remove(
    const unsigned short int);
  unsigned long int Norm_total_uncorrected_singles_remove();
  unsigned long int Norm_normalization_component_remove(
    const unsigned short int);
  unsigned long int Norm_normalization_component_remove();
  unsigned long int Norm_number_of_dimensions_remove(
    const unsigned short int);
  unsigned long int Norm_number_of_dimensions_remove();
  unsigned long int Norm_matrix_size_remove(const unsigned short int,
      const unsigned short int);
  unsigned long int Norm_matrix_size_remove(const unsigned short int);
  unsigned long int Norm_matrix_size_remove();
  unsigned long int Norm_matrix_axis_label_remove(const unsigned short int,
      const unsigned short int);
  unsigned long int Norm_matrix_axis_label_remove(const unsigned short int);
  unsigned long int Norm_matrix_axis_label_remove();
  unsigned long int Norm_matrix_axis_unit_remove(const unsigned short int,
      const unsigned short int);
  unsigned long int Norm_matrix_axis_unit_remove(const unsigned short int);
  unsigned long int Norm_matrix_axis_unit_remove();
  unsigned long int Norm_scale_factor_remove(const unsigned short int,
      const unsigned short int);
  unsigned long int Norm_scale_factor_remove(const unsigned short int);
  unsigned long int Norm_scale_factor_remove();
  unsigned long int Norm_data_offset_in_bytes_remove(
    const unsigned short int);
  unsigned long int Norm_data_offset_in_bytes_remove();
  unsigned long int Norm_energy_window_lower_level_remove(
    const unsigned short int);
  unsigned long int Norm_energy_window_lower_level_remove();
  unsigned long int Norm_energy_window_upper_level_remove(
    const unsigned short int);
  unsigned long int Norm_energy_window_upper_level_remove();
  unsigned long int Norm_data_set_remove(const unsigned short int);
  unsigned long int Norm_data_set_remove();
  // request values from subheader
  string Sub_comment();
  string Sub_originating_system();
  string Sub_CPS_data_type();
  string Sub_CPS_version_number();
  string Sub_name_of_listmode_file();
  string Sub_name_of_sinogram_file();
  string Sub_name_of_data_file();
  string Sub_type_of_data();
  TIMEDATE::tdate Sub_study_date(string * const);
  TIMEDATE::ttime Sub_study_time(string * const);
  unsigned long int Sub_study_UTC(string * const);
  tendian Sub_image_data_byte_order();
  tpet_data_type Sub_PET_data_type();
  string Sub_data_format();
  tnumber_format Sub_number_format();
  unsigned short int Sub_number_of_bytes_per_pixel();
  float Sub_image_scale_factor();
  unsigned short int Sub_number_of_dimensions();
  string Sub_matrix_axis_label(const unsigned short int);
  unsigned short int Sub_matrix_size(const unsigned short int);
  float Sub_scale_factor(const unsigned short int, string * const);
  float Sub_start_horizontal_bed_position(string * const);
  float Sub_start_vertical_bed_position(string * const);
  float Sub_reconstruction_diameter(string * const);
  unsigned short int Sub_reconstruction_bins();
  unsigned short int Sub_reconstruction_views();
  Interfile::tprocess_status Sub_process_status();
  string Sub_quantification_units();
  Interfile::tdecay_correction Sub_decay_correction();
  float Sub_decay_correction_factor();
  float Sub_scatter_fraction_factor();
  float Sub_dead_time_factor(const unsigned short int);
  Interfile::tslice_orientation Sub_slice_orientation();
  string Sub_method_of_reconstruction();
  unsigned short int Sub_number_of_iterations();
  unsigned short int Sub_number_of_subsets();
  string Sub_stopping_criteria();
  string Sub_filter_name();
  float Sub_xy_filter(string * const);
  float Sub_z_filter(string * const);
  float Sub_image_zoom();
  float Sub_x_offset(string * const);
  float Sub_y_offset(string * const);
  unsigned short int Sub_number_of_scan_data_types();
  tdata_type_description Sub_scan_data_type_description(
    const unsigned short int);
  unsigned long int Sub_data_offset_in_bytes(const unsigned short int);
  unsigned short int Sub_angular_compression();
  unsigned short int Sub_axial_compression();
  unsigned short int Sub_maximum_ring_difference();
  unsigned short int Sub_number_of_segments();
  unsigned short int Sub_segment_table(const unsigned short int);
  string Sub_applied_corrections(const unsigned short int);
  string Sub_method_of_attenuation_correction();
  string Sub_method_of_scatter_correction();
  Interfile::trandom_correction Sub_method_of_random_correction();
  unsigned short int Sub_total_number_of_data_sets();
  tacqcondition Sub_acquisition_start_condition();
  tacqcondition Sub_acquisition_termination_condition();
  unsigned long int Sub_image_duration(string * const);
  unsigned long int Sub_image_relative_start_time(string * const);
  uint64 Sub_total_prompts();
  uint64 Sub_total_randoms();
  int64 Sub_total_net_trues();
  uint64 Sub_total_uncorrected_singles();
  unsigned short int Sub_number_of_bucket_rings();
  uint64 Sub_bucket_ring_singles(const unsigned short int);
  unsigned short int Sub_number_of_projections();
  unsigned short int Sub_number_of_views();
  // set values in subheader
  unsigned long int Sub_INTERFILE(const string comment = string());
  unsigned long int Sub_comment(const string,
                                const string comment = string());
  unsigned long int Sub_originating_system(const string,
      const string comment = string());
  unsigned long int Sub_CPS_data_type(const string,
                                      const string comment = string());
  unsigned long int Sub_CPS_version_number(const string,
      const string comment = string());
  unsigned long int Sub_GENERAL_DATA(
    const string comment = string());
  unsigned long int Sub_name_of_listmode_file(const string,
      const string comment = string());
  unsigned long int Sub_name_of_sinogram_file(const string,
      const string comment = string());
  unsigned long int Sub_name_of_data_file(const string,
                                          const string comment = string());
  unsigned long int Sub_GENERAL_IMAGE_DATA(
    const string comment = string());
  unsigned long int Sub_type_of_data(const string,
                                     const string comment = string());
  unsigned long int Sub_study_date(const TIMEDATE::tdate, const string,
                                   const string comment = string());
  unsigned long int Sub_study_time(const TIMEDATE::ttime, const string,
                                   const string comment = string());
  unsigned long int Sub_study_UTC(const unsigned long int,
                                  const string,
                                  const string comment = string());
  unsigned long int Sub_image_data_byte_order(const tendian,
      const string comment = string());
  unsigned long int Sub_PET_data_type(const tpet_data_type,
                                      const string comment = string());
  unsigned long int Sub_data_format(const string,
                                    const string comment = string());
  unsigned long int Sub_number_format(const tnumber_format,
                                      const string comment = string());
  unsigned long int Sub_number_of_bytes_per_pixel(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_image_scale_factor(const float,
      const string comment = string());
  unsigned long int Sub_number_of_dimensions(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_matrix_axis_label(const string,
                                          const unsigned short int,
                                          const string comment = string());
  unsigned long int Sub_matrix_size(const unsigned short int,
                                    const unsigned short int,
                                    const string comment = string());
  unsigned long int Sub_scale_factor(const float, const unsigned short int,
                                     const string,
                                     const string comment = string());
  unsigned long int Sub_start_horizontal_bed_position(const float,
      const string,
      const string comment = string());
  unsigned long int Sub_start_vertical_bed_position(const float,
      const string,
      const string comment = string());
  unsigned long int Sub_reconstruction_diameter(const float,
      const string,
      const string comment = string());
  unsigned long int Sub_reconstruction_bins(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_reconstruction_views(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_process_status(const tprocess_status,
                                       const string comment = string());
  unsigned long int Sub_quantification_units(const string,
      const string comment = string());
  unsigned long int Sub_decay_correction(const Interfile::tdecay_correction,
                                         const string comment = string());
  unsigned long int Sub_decay_correction_factor(const float,
      const string comment = string());
  unsigned long int Sub_scatter_fraction_factor(const float,
      const string comment = string());
  unsigned long int Sub_dead_time_factor(const float,
                                         const unsigned short int,
                                         const string comment = string());
  unsigned long int Sub_slice_orientation(
    const Interfile::tslice_orientation,
    const string comment = string());
  unsigned long int Sub_method_of_reconstruction(const string,
      const string comment = string());
  unsigned long int Sub_number_of_iterations(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_number_of_subsets(const unsigned short int,
                                          const string comment = string());
  unsigned long int Sub_stopping_criteria(const string,
                                          const string comment = string());
  unsigned long int Sub_filter_name(const string,
                                    const string comment = string());
  unsigned long int Sub_xy_filter(const float, const string,
                                  const string comment = string());
  unsigned long int Sub_z_filter(const float, const string,
                                 const string comment = string());
  unsigned long int Sub_image_zoom(const float,
                                   const string comment = string());
  unsigned long int Sub_x_offset(const float, const string,
                                 const string comment = string());
  unsigned long int Sub_y_offset(const float, const string,
                                 const string comment = string());
  unsigned long int Sub_number_of_scan_data_types(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_scan_data_type_description(
    const tdata_type_description,
    const unsigned short int,
    const string comment = string());
  unsigned long int Sub_data_offset_in_bytes(const unsigned long int,
      const unsigned short int,
      const string comment = string());
  unsigned long int Sub_angular_compression(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_axial_compression(const unsigned short int,
                                          const string comment = string());
  unsigned long int Sub_maximum_ring_difference(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_number_of_segments(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_segment_table(const unsigned short int,
                                      const unsigned short int,
                                      const string comment = string());
  unsigned long int Sub_applied_corrections(const string,
      const unsigned short int,
      const string comment = string());
  unsigned long int Sub_method_of_attenuation_correction(const string,
      const string comment = string());
  unsigned long int Sub_method_of_scatter_correction(const string,
      const string comment = string());
  unsigned long int Sub_method_of_random_correction(
    const Interfile::trandom_correction,
    const string comment = string());
  unsigned long int Sub_IMAGE_DATA_DESCRIPTION(
    const string comment = string());
  unsigned long int Sub_total_number_of_data_sets(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_acquisition_start_condition(const tacqcondition,
      const string comment = string());
  unsigned long int Sub_acquisition_termination_condition(
    const tacqcondition,
    const string comment = string());
  unsigned long int Sub_image_duration(const unsigned long int,
                                       const string,
                                       const string comment = string());
  unsigned long int Sub_image_relative_start_time(const unsigned long int,
      const string,
      const string comment = string());
  unsigned long int Sub_total_prompts(const uint64,
                                      const string comment = string());
  unsigned long int Sub_total_randoms(const uint64,
                                      const string comment = string());
  unsigned long int Sub_total_net_trues(const int64,
                                        const string comment = string());
  unsigned long int Sub_total_uncorrected_singles(const uint64,
      const string comment = string());
  unsigned long int Sub_number_of_bucket_rings(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_bucket_ring_singles(const uint64,
      const unsigned short int,
      const string comment = string());
  unsigned long int Sub_SUPPLEMENTARY_ATTRIBUTES(
    const string comment = string());
  unsigned long int Sub_number_of_projections(const unsigned short int,
      const string comment = string());
  unsigned long int Sub_number_of_views(const unsigned short int,
                                        const string comment = string());
  // empty array
  unsigned long int Sub_applied_corrections_remove(const unsigned short int);
  unsigned long int Sub_applied_corrections_remove();
  unsigned long int Sub_matrix_axis_label_remove(const unsigned short int);
  unsigned long int Sub_matrix_axis_label_remove();
  unsigned long int Sub_matrix_size_remove(const unsigned short int);
  unsigned long int Sub_matrix_size_remove();
  unsigned long int Sub_scale_factor_remove(const unsigned short int);
  unsigned long int Sub_scale_factor_remove();
  unsigned long int Sub_scan_data_type_description_remove(
    const unsigned short int);
  unsigned long int Sub_scan_data_type_description_remove();
  unsigned long int Sub_data_offset_in_bytes_remove(
    const unsigned short int);
  unsigned long int Sub_data_offset_in_bytes_remove();
  unsigned long int Sub_segment_table_remove(const unsigned short int);
  unsigned long int Sub_segment_table_remove();
  unsigned long int Sub_bucket_ring_singles_remove(const unsigned short int);
  unsigned long int Sub_bucket_ring_singles_remove();
};
