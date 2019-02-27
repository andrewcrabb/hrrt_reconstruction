/*! \file ecat7.h
    \brief This class implements the ECAT7 file format.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/19 added Doxygen style documentation
 */

#pragma once

#include <fstream>
#include <string>
#include <vector>
#include "ecat7_directory.h"
#include "ecat7_global.h"
#include "ecat7_mainheader.h"
#include "ecat7_matrix_abc.h"

/*- definitions -------------------------------------------------------------*/

class ECAT7 {
private:
  std::string in_filename_;                       /*!< filename for loading */
  std::string  out_filename_;                      /*!< filename for storing */
  ECAT7_MAINHEADER *e7_main_header_;         /*!< main header of ECAT7 file */
  ECAT7_DIRECTORY *e7_directory_;              /*!< directory of ECAT7 file */
  std::vector <ECAT7_MATRIX *> e7_matrix_; /*! array of matrices from ECAT7 file */
  void CheckFormat(std::ifstream * const);               // check file format
  void DeleteDirectory();               // delete directory
public:
  ECAT7();                             // initialize object
  ~ECAT7();                               // destroy object
  ECAT7& operator = (const ECAT7 &);        // '='-operator

  void AppendMatrix(const unsigned short int) const;  // append matrix to ECAT7 file
  void AppendMatrix(const unsigned short int, const std::string);
  void Byte2Float(const unsigned short int) const;                // convert dataset from ecat_matrix::MatrixDataType::ByteData to ecat_matrix::MatrixDataType::IeeeFloat
  void CreateAttnMatrices(const unsigned short int);                               // create Attenuation matrices
  void CreateDirectoryStruct();         // create directory
  void CreateImageMatrices(const unsigned short int);// create Image matrices
  void CreateMainHeader();            // create main header
  void CreateNormMatrices(const unsigned short int);  // create Norm matrices
  void CreateNorm3DMatrices(const unsigned short int);                // delete data part of matrix and free memory
  void CreatePolarMatrices(const unsigned short int);                            // request data type of data part
  void CreateScanMatrices(const unsigned short int);  // create Scan matrices
  void CreateScan3DMatrices(const unsigned short int);                  // remove data from object without deleting
  void DataDeleted(const unsigned short int) const;                                    // create Scan3D matrices
  unsigned short int DataType(const unsigned short int) const;                                 // create Polar map matrices
  void DeleteData(const unsigned short int) const;                                    // create Norm3D matrices
  void DeleteMatrices();                 // delete matrices
  void DeleteMatrix(const unsigned short int);               // delete matrix
  void DeleteMainHeader();            // delete main header
  ECAT7_DIRECTORY *Directory() const;  // request pointer to directory object
  void LoadData(const unsigned short int);        // load data part of matrix
  void LoadFile(const std::string);             // load headers of ECAT7 file
  ECAT7_MAINHEADER *MainHeader() const;                       // change values in Attenuation header
  ECAT7_MATRIX *Matrix(const unsigned short int) const;                            // update subheader in ECAT7 file
  void *getecat_matrix::MatrixData(const unsigned short int) const;                          // update main header in ECAT7 file
  void getecat_matrix::MatrixData(void * const, const unsigned short int, const unsigned short int) const;               // convert dataset from ecat_matrix::MatrixDataType::SunShort to ecat_matrix::MatrixDataType::IeeeFloat
  unsigned short int NumberOfMatrices() const;  // request number of matrices
  void PrintHeader(std::list <std::string> * const) const;       // store main header and empty directory as ECAT7 file
  void SaveFile(const std::string);                 // print header information into string list
  void Short2Float(const unsigned short int) const;                        // put matrix data into matrix object
  void UpdateMainHeader(const std::string);             // request pointer to data part of matrix object
  void UpdateSubheader(const std::string, const unsigned short int);  

  template <typename T> *get_matrix(int t_matrix_index);
                          // request pointer to matrix object

  void               Attn_data_type(const signed short int, const unsigned short int) const;                    // request pointer to main header object
  void               Attn_num_dimensions(const signed short int, const unsigned short int) const;
  void               Attn_attenuation_type(const signed short int, const unsigned short int) const;
  void               Attn_num_r_elements(const unsigned short int, const unsigned short int) const;
  void               Attn_num_angles(const unsigned short int, const unsigned short int) const;
  void               Attn_num_z_elements(const unsigned short int, const unsigned short int) const;
  void               Attn_ring_difference(const signed short int, const unsigned short int) const;
  void               Attn_x_resolution(const float, const unsigned short int) const;
  void               Attn_y_resolution(const float, const unsigned short int) const;
  void               Attn_z_resolution(const float, const unsigned short int) const;
  void               Attn_w_resolution(const float, const unsigned short int) const;
  void               Attn_scale_factor(const float, const unsigned short int) const;
  void               Attn_x_offset(const float, const unsigned short int) const;
  void               Attn_y_offset(const float, const unsigned short int) const;
  void               Attn_x_radius(const float, const unsigned short int) const;
  void               Attn_y_radius(const float, const unsigned short int) const;
  void               Attn_tilt_angle(const float, const unsigned short int) const;
  void               Attn_attenuation_coeff(const float, const unsigned short int) const;
  void               Attn_attenuation_min(const float, const unsigned short int) const;
  void               Attn_attenuation_max(const float, const unsigned short int) const;
  void               Attn_skull_thickness(const float, const unsigned short int) const;
  void               Attn_num_additional_atten_coeff(const signed short int, const unsigned short int) const;
  void               Attn_additional_atten_coeff(const float, const unsigned short int, const unsigned short int) const;
  void               Attn_edge_finding_threshold(const float, const unsigned short int) const;
  void               Attn_storage_order(const signed short int, const unsigned short int) const;
  void               Attn_span(const signed short int, const unsigned short int) const;
  void               Attn_z_elements(const signed short int, const unsigned short int, unsigned short int) const;
  void               Attn_fill_unused(const signed short int, const unsigned short int, unsigned short int) const;
  void               Attn_fill_user(const signed short int, const unsigned short int, unsigned short int) const;

  signed short int   Attn_data_type(const unsigned short int) const;                    // request values from Attenuation header
  signed short int   Attn_num_dimensions(const unsigned short int) const;
  signed short int   Attn_attenuation_type(const unsigned short int) const;
  unsigned short int Attn_num_r_elements(const unsigned short int) const;
  unsigned short int Attn_num_angles(const unsigned short int) const;
  unsigned short int Attn_num_z_elements(const unsigned short int) const;
  signed short int   Attn_ring_difference(const unsigned short int) const;
  float              Attn_x_resolution(const unsigned short int) const;
  float              Attn_y_resolution(const unsigned short int) const;
  float              Attn_z_resolution(const unsigned short int) const;
  float              Attn_w_resolution(const unsigned short int) const;
  float              Attn_scale_factor(const unsigned short int) const;
  float              Attn_x_offset(const unsigned short int) const;
  float              Attn_y_offset(const unsigned short int) const;
  float              Attn_x_radius(const unsigned short int) const;
  float              Attn_y_radius(const unsigned short int) const;
  float              Attn_tilt_angle(const unsigned short int) const;
  float              Attn_attenuation_coeff(const unsigned short int) const;
  float              Attn_attenuation_min(const unsigned short int) const;
  float              Attn_attenuation_max(const unsigned short int) const;
  float              Attn_skull_thickness(const unsigned short int) const;
  signed short int   Attn_num_additional_atten_coeff(                                               const unsigned short int) const;
  float              Attn_additional_atten_coeff(const unsigned short int, const unsigned short int) const;
  float              Attn_edge_finding_threshold(const unsigned short int) const;
  signed short int   Attn_storage_order(const unsigned short int) const;
  signed short int   Attn_span(const unsigned short int) const;
  signed short int   Attn_z_elements(const unsigned short int, const unsigned short int) const;
  signed short int   Attn_fill_unused(const unsigned short int, const unsigned short int) const;
  signed short int   Attn_fill_user(const unsigned short int, const unsigned short int) const;

  void            Dir_bed(const signed long int, const unsigned short int) const;                                // change values in directory
  void            Dir_frame(const signed long int, const unsigned short int) const;
  void            Dir_gate(const signed long int, const unsigned short int) const;
  void            Dir_plane(const signed long int, const unsigned short int) const;
  void            Dir_data(const signed long int, const unsigned short int) const;

  signed long int Dir_bed(const unsigned short int) const;                             // request values from directory
  signed long int Dir_frame(const unsigned short int) const;
  signed long int Dir_gate(const unsigned short int) const;
  signed long int Dir_plane(const unsigned short int) const;
  signed long int Dir_data(const unsigned short int) const;

  void Image_data_type(const signed short int, const unsigned short int) const;                             // change values in Image header
  void Image_num_dimensions(const signed short int, const unsigned short int) const;
  void Image_x_dim(const signed short int, const unsigned short int) const;
  void Image_y_dim(const signed short int, const unsigned short int) const;
  void Image_z_dim(const signed short int, const unsigned short int) const;
  void Image_x_offset(const float, const unsigned short int) const;
  void Image_y_offset(const float, const unsigned short int) const;
  void Image_z_offset(const float, const unsigned short int) const;
  void Image_recon_zoom(const float, const unsigned short int) const;
  void Image_scale_factor(const float, const unsigned short int) const;
  void Image_image_min(const signed short int, const unsigned short int) const;
  void Image_image_max(const signed short int, const unsigned short int) const;
  void Image_x_pixel_size(const float, const unsigned short int) const;
  void Image_y_pixel_size(const float, const unsigned short int) const;
  void Image_z_pixel_size(const float, const unsigned short int) const;
  void Image_frame_duration(const signed long int, const unsigned short int) const;
  void Image_frame_start_time(const signed long int, const unsigned short int) const;
  void Image_filter_code(const signed short int, const unsigned short int) const;
  void Image_x_resolution(const float, const unsigned short int) const;
  void Image_y_resolution(const float, const unsigned short int) const;
  void Image_z_resolution(const float, const unsigned short int) const;
  void Image_num_r_elements(const float, const unsigned short int) const;
  void Image_num_angles(const float, const unsigned short int) const;
  void Image_z_rotation_angle(const float, const unsigned short int) const;
  void Image_decay_corr_fctr(const float, const unsigned short int) const;
  void Image_processing_code(const signed long int, const unsigned short int) const;
  void Image_gate_duration(const signed long int, const unsigned short int) const;
  void Image_r_wave_offset(const signed long int, const unsigned short int) const;
  void Image_num_accepted_beats(const signed long int, const unsigned short int) const;
  void Image_filter_cutoff_frequency(const float, const unsigned short int) const;
  void Image_filter_resolution(const float, const unsigned short int) const;
  void Image_filter_ramp_slope(const float, const unsigned short int) const;
  void Image_filter_order(const signed short int, const unsigned short int) const;
  void Image_filter_scatter_fraction(const float, const unsigned short int) const;
  void Image_filter_scatter_slope(const float, const unsigned short int) const;
  void Image_annotation(unsigned char * const, const unsigned short int) const;
  void Image_mt_1_1(const float, const unsigned short int) const;
  void Image_mt_1_2(const float, const unsigned short int) const;
  void Image_mt_1_3(const float, const unsigned short int) const;
  void Image_mt_2_1(const float, const unsigned short int) const;
  void Image_mt_2_2(const float, const unsigned short int) const;
  void Image_mt_2_3(const float, const unsigned short int) const;
  void Image_mt_3_1(const float, const unsigned short int) const;
  void Image_mt_3_2(const float, const unsigned short int) const;
  void Image_mt_3_3(const float, const unsigned short int) const;
  void Image_rfilter_cutoff(const float, const unsigned short int) const;
  void Image_rfilter_resolution(const float, const unsigned short int) const;
  void Image_rfilter_code(const signed short int, const unsigned short int) const;
  void Image_rfilter_order(const signed short int, const unsigned short int) const;
  void Image_zfilter_cutoff(const float, const unsigned short int) const;
  void Image_zfilter_resolution(const float, const unsigned short int) const;
  void Image_zfilter_code(const signed short int, const unsigned short int) const;
  void Image_zfilter_order(const signed short int, const unsigned short int) const;
  void Image_mt_1_4(const float, const unsigned short int) const;
  void Image_mt_2_4(const float, const unsigned short int) const;
  void Image_mt_3_4(const float, const unsigned short int) const;
  void Image_scatter_type(const signed short int, const unsigned short int) const;
  void Image_recon_type(const signed short int, const unsigned short int) const;
  void Image_recon_views(const signed short int, const unsigned short int) const;
  void Image_fill_cti(const signed short int, const unsigned short int, const unsigned short int) const;
  void Image_fill_user(const signed short int, const unsigned short int, const unsigned short int) const;

  signed short int Image_data_type(const unsigned short int) const;                          // request values from Image header
  signed short int Image_num_dimensions(const unsigned short int) const;
  signed short int Image_x_dim(const unsigned short int) const;
  signed short int Image_y_dim(const unsigned short int) const;
  signed short int Image_z_dim(const unsigned short int) const;
  float            Image_x_offset(const unsigned short int) const;
  float            Image_y_offset(const unsigned short int) const;
  float            Image_z_offset(const unsigned short int) const;
  float            Image_recon_zoom(const unsigned short int) const;
  float            Image_scale_factor(const unsigned short int) const;
  signed short int Image_image_min(const unsigned short int) const;
  signed short int Image_image_max(const unsigned short int) const;
  float            Image_x_pixel_size(const unsigned short int) const;
  float            Image_y_pixel_size(const unsigned short int) const;
  float            Image_z_pixel_size(const unsigned short int) const;
  signed long int  Image_frame_duration(const unsigned short int) const;
  signed long int  Image_frame_start_time(const unsigned short int) const;
  signed short int Image_filter_code(const unsigned short int) const;
  float            Image_x_resolution(const unsigned short int) const;
  float            Image_y_resolution(const unsigned short int) const;
  float            Image_z_resolution(const unsigned short int) const;
  float            Image_num_r_elements(const unsigned short int) const;
  float            Image_num_angles(const unsigned short int) const;
  float            Image_z_rotation_angle(const unsigned short int) const;
  float            Image_decay_corr_fctr(const unsigned short int) const;
  signed long int  Image_processing_code(const unsigned short int) const;
  signed long int  Image_gate_duration(const unsigned short int) const;
  signed long int  Image_r_wave_offset(const unsigned short int) const;
  signed long int  Image_num_accepted_beats(const unsigned short int) const;
  float            Image_filter_cutoff_frequency(const unsigned short int) const;
  float            Image_filter_resolution(const unsigned short int) const;
  float            Image_filter_ramp_slope(const unsigned short int) const;
  signed short int Image_filter_order(const unsigned short int) const;
  float            Image_filter_scatter_fraction(const unsigned short int) const;
  float            Image_filter_scatter_slope(const unsigned short int) const;
  unsigned char   *Image_annotation(const unsigned short int) const;
  float            Image_mt_1_1(const unsigned short int) const;
  float            Image_mt_1_2(const unsigned short int) const;
  float            Image_mt_1_3(const unsigned short int) const;
  float            Image_mt_2_1(const unsigned short int) const;
  float            Image_mt_2_2(const unsigned short int) const;
  float            Image_mt_2_3(const unsigned short int) const;
  float            Image_mt_3_1(const unsigned short int) const;
  float            Image_mt_3_2(const unsigned short int) const;
  float            Image_mt_3_3(const unsigned short int) const;
  float            Image_rfilter_cutoff(const unsigned short int) const;
  float            Image_rfilter_resolution(const unsigned short int) const;
  signed short int Image_rfilter_code(const unsigned short int) const;
  signed short int Image_rfilter_order(const unsigned short int) const;
  float            Image_zfilter_cutoff(const unsigned short int) const;
  float            Image_zfilter_resolution(const unsigned short int) const;
  signed short int Image_zfilter_code(const unsigned short int) const;
  signed short int Image_zfilter_order(const unsigned short int) const;
  float            Image_mt_1_4(const unsigned short int) const;
  float            Image_mt_2_4(const unsigned short int) const;
  float            Image_mt_3_4(const unsigned short int) const;
  signed short int Image_scatter_type(const unsigned short int) const;
  signed short int Image_recon_type(const unsigned short int) const;
  signed short int Image_recon_views(const unsigned short int) const;
  signed short int Image_fill_cti(const unsigned short int, const unsigned short int) const;
  signed short int Image_fill_user(const unsigned short int, const unsigned short int) const;
  void Main_magic_number(unsigned char * const) const;                              // change values in main header
  void Main_original_file_name(unsigned char * const) const;
  void Main_sw_version(const signed short int) const;
  void Main_system_type(const signed short int) const;
  void Main_file_type(const signed short int) const;
  void Main_serial_number(unsigned char * const) const;
  void Main_scan_start_time(const signed long int) const;
  void Main_isotope_name(unsigned char * const) const;
  void Main_isotope_halflife(const float) const;
  void Main_radiopharmaceutical(unsigned char * const) const;
  void Main_gantry_tilt(const float) const;
  void Main_gantry_rotation(const float) const;
  void Main_bed_elevation(const float) const;
  void Main_intrinsic_tilt(const float) const;
  void Main_wobble_speed(const signed short int) const;
  void Main_transm_source_type(const signed short int) const;
  void Main_distance_scanned(const float) const;
  void Main_transaxial_fov(const float) const;
  void Main_angular_compression(const signed short int) const;
  void Main_coin_samp_mode(const signed short int) const;
  void Main_axial_samp_mode(const signed short int) const;
  void Main_ecat_calibration_factor(const float) const;
  void Main_calibration_units(const signed short int) const;
  void Main_calibration_units_label(const signed short int) const;
  void Main_compression_code(const signed short int) const;
  void Main_study_type(unsigned char * const) const;
  void Main_patient_id(unsigned char * const) const;
  void Main_patient_name(unsigned char * const) const;
  void Main_patient_sex(const unsigned char) const;
  void Main_patient_dexterity(const unsigned char) const;
  void Main_patient_age(const float) const;
  void Main_patient_height(const float) const;
  void Main_patient_weight(const float) const;
  void Main_patient_birth_date(const signed long int) const;
  void Main_physician_name(unsigned char * const) const;
  void Main_operator_name(unsigned char * const) const;
  void Main_study_description(unsigned char * const) const;
  void Main_acquisition_type(const signed short int) const;
  void Main_patient_orientation(const signed short int) const;
  void Main_facility_name(unsigned char * const) const;
  void Main_num_planes(const signed short int) const;
  void Main_num_frames(const signed short int) const;
  void Main_num_gates(const signed short int) const;
  void Main_num_bed_pos(const signed short int) const;
  void Main_init_bed_position(const float) const;
  void Main_bed_position(const float, const unsigned short int) const;
  void Main_plane_separation(const float) const;
  void Main_lwr_sctr_thres(const signed short int) const;
  void Main_lwr_true_thres(const signed short int) const;
  void Main_upr_true_thres(const signed short int) const;
  void Main_user_process_code(unsigned char * const) const;
  void Main_acquisition_mode(const signed short int) const;
  void Main_bin_size(const float) const;
  void Main_branching_fraction(const float) const;
  void Main_dose_start_time(const signed long int) const;
  void Main_dosage(const float) const;
  void Main_well_counter_corr_factor(const float) const;
  void Main_data_units(unsigned char * const) const;
  void Main_septa_state(const signed short int) const;
  void Main_fill_cti(const signed short int, const unsigned short int) const;
  unsigned char   *Main_magic_number() const;                           // request values from main header
  unsigned char   *Main_original_file_name() const;
  signed short int Main_sw_version() const;
  signed short int Main_system_type() const;
  signed short int Main_file_type() const;
  unsigned char   *Main_serial_number() const;
  signed long int  Main_scan_start_time() const;
  unsigned char   *Main_isotope_name() const;
  float            Main_isotope_halflife() const;
  unsigned char   *Main_radiopharmaceutical() const;
  float            Main_gantry_tilt() const;
  float            Main_gantry_rotation() const;
  float            Main_bed_elevation() const;
  float            Main_intrinsic_tilt() const;
  signed short int Main_wobble_speed() const;
  signed short int Main_transm_source_type() const;
  float            Main_distance_scanned() const;
  float            Main_transaxial_fov() const;
  signed short int Main_angular_compression() const;
  signed short int Main_coin_samp_mode() const;
  signed short int Main_axial_samp_mode() const;
  float            Main_ecat_calibration_factor() const;
  signed short int Main_calibration_units() const;
  signed short int Main_calibration_units_label() const;
  signed short int Main_compression_code() const;
  unsigned char   *Main_study_type() const;
  unsigned char   *Main_patient_id() const;
  unsigned char   *Main_patient_name() const;
  unsigned char    Main_patient_sex() const;
  unsigned char    Main_patient_dexterity() const;
  float            Main_patient_age() const;
  float            Main_patient_height() const;
  float            Main_patient_weight() const;
  signed long int  Main_patient_birth_date() const;
  unsigned char   *Main_physician_name() const;
  unsigned char   *Main_operator_name() const;
  unsigned char   *Main_study_description() const;
  signed short int Main_acquisition_type() const;
  signed short int Main_patient_orientation() const;
  unsigned char   *Main_facility_name() const;
  signed short int Main_num_planes() const;
  signed short int Main_num_frames() const;
  signed short int Main_num_gates() const;
  signed short int Main_num_bed_pos() const;
  float            Main_init_bed_position() const;
  float            Main_bed_position(const unsigned short int) const;
  float            Main_plane_separation() const;
  signed short int Main_lwr_sctr_thres() const;
  signed short int Main_lwr_true_thres() const;
  signed short int Main_upr_true_thres() const;
  unsigned char   *Main_user_process_code() const;
  signed short int Main_acquisition_mode() const;
  float            Main_bin_size() const;
  float            Main_branching_fraction() const;
  signed long int  Main_dose_start_time() const;
  float            Main_dosage() const;
  float            Main_well_counter_corr_factor() const;
  unsigned char   *Main_data_units() const;
  signed short int Main_septa_state() const;
  signed short int Main_fill_cti(const unsigned short int) const;

  void Norm_data_type(const signed short int, const unsigned short int) const;
  void Norm_num_dimensions(const signed short int, const unsigned short int) const;
  void Norm_num_r_elements(const signed short int, const unsigned short int) const;
  void Norm_num_angles(const signed short int, const unsigned short int) const;
  void Norm_num_z_elements(const signed short int, const unsigned short int) const;
  void Norm_crystals_per_ring(const signed short int, const unsigned short int) const;
  void Norm_ring_difference(const signed short int, const unsigned short int) const;
  void Norm_scale_factor(const float, const unsigned short int) const;
  void Norm_norm_min(const float, const unsigned short int) const;
  void Norm_norm_max(const float, const unsigned short int) const;
  void Norm_fov_source_width(const float, const unsigned short int) const;
  void Norm_norm_quality_factor(const float, const unsigned short int) const;
  void Norm_norm_quality_factor_code(const signed short int, const unsigned short int) const;
  void Norm_storage_order(const signed short int, const unsigned short int) const;
  void Norm_span(const signed short int, const unsigned short int) const;
  void Norm_z_elements(const signed short int, const unsigned short int, const unsigned short int) const;
  void Norm_fill_cti(const signed short int, const unsigned short int, const unsigned short int) const;
  void Norm_fill_user(const signed short int, const unsigned short int, const unsigned short int) const;

  signed short int Norm_data_type(const unsigned short int) const;                           // request values from Norm header
  signed short int Norm_num_dimensions(const unsigned short int) const;
  signed short int Norm_num_r_elements(const unsigned short int) const;
  signed short int Norm_num_angles(const unsigned short int) const;
  signed short int Norm_num_z_elements(const unsigned short int) const;
  signed short int Norm_crystals_per_ring(const unsigned short int) const;
  signed short int Norm_ring_difference(const unsigned short int) const;
  float            Norm_scale_factor(const unsigned short int) const;
  float            Norm_norm_min(const unsigned short int) const;
  float            Norm_norm_max(const unsigned short int) const;
  float            Norm_fov_source_width(const unsigned short int) const;
  float            Norm_norm_quality_factor(const unsigned short int) const;
  signed short int Norm_norm_quality_factor_code(                                               const unsigned short int) const;
  signed short int Norm_storage_order(const unsigned short int) const;
  signed short int Norm_span(const unsigned short int) const;
  signed short int Norm_z_elements(const unsigned short int, const unsigned short int) const;
  signed short int Norm_fill_cti(const unsigned short int, const unsigned short int) const;
  signed short int Norm_fill_user(const unsigned short int, const unsigned short int) const;

  void Norm3D_data_type(const signed short int, const unsigned short int) const;                            // change values in Norm3D header
  void Norm3D_num_r_elements(const signed short int, const unsigned short int) const;
  void Norm3D_num_transaxial_crystals(const signed short int, const unsigned short int) const;
  void Norm3D_num_crystal_rings(const signed short int, const unsigned short int) const;
  void Norm3D_crystals_per_ring(const signed short int, const unsigned short int) const;
  void Norm3D_num_geo_corr_planes(const signed short int, const unsigned short int) const;
  void Norm3D_uld(const signed short int, const unsigned short int) const;
  void Norm3D_lld(const signed short int, const unsigned short int) const;
  void Norm3D_scatter_energy(const signed short int, const unsigned short int) const;
  void Norm3D_norm_quality_factor(const float, const unsigned short int) const;
  void Norm3D_norm_quality_factor_code(const signed short int, const unsigned short int) const;
  void Norm3D_ring_dtcor1(const float, const unsigned short int, const unsigned short int) const;
  void Norm3D_ring_dtcor2(const float, const unsigned short int, const unsigned short int) const;
  void Norm3D_crystal_dtcor(const float, const unsigned short int, const unsigned short int) const;
  void Norm3D_span(const signed short int, const unsigned short int) const;
  void Norm3D_max_ring_diff(const signed short int, const unsigned short int) const;
  void Norm3D_fill_cti(const signed short int, const unsigned short int, const unsigned short int) const;
  void Norm3D_fill_user(const signed short int, const unsigned short int, const unsigned short int) const;

  signed short int Norm3D_data_type(const unsigned short int) const;                         // request values from Norm3D header
  signed short int Norm3D_num_r_elements(const unsigned short int) const;
  signed short int Norm3D_num_transaxial_crystals(                                               const unsigned short int) const;
  signed short int Norm3D_num_crystal_rings(const unsigned short int) const;
  signed short int Norm3D_crystals_per_ring(const unsigned short int) const;
  signed short int Norm3D_num_geo_corr_planes(                                               const unsigned short int) const;
  signed short int Norm3D_uld(const unsigned short int) const;
  signed short int Norm3D_lld(const unsigned short int) const;
  signed short int Norm3D_scatter_energy(const unsigned short int) const;
  float            Norm3D_norm_quality_factor(const unsigned short int) const;
  signed short int Norm3D_norm_quality_factor_code(                                               const unsigned short int) const;
  float            Norm3D_ring_dtcor1(const unsigned short int, const unsigned short int) const;
  float            Norm3D_ring_dtcor2(const unsigned short int, const unsigned short int) const;
  float            Norm3D_crystal_dtcor(const unsigned short int, const unsigned short int) const;
  signed short int Norm3D_span(const unsigned short int) const;
  signed short int Norm3D_max_ring_diff(const unsigned short int) const;
  signed short int Norm3D_fill_cti(const unsigned short int, const unsigned short int) const;
  signed short int Norm3D_fill_user(const unsigned short int, const unsigned short int) const;

  void Polar_data_type(const signed short int, const unsigned short int) const;                         // change values in Polar map header
  void Polar_polar_map_type(const signed short int, const unsigned short int) const;
  void Polar_num_rings(const signed short int, const unsigned short int) const;
  void Polar_sectors_per_ring(const signed short int, const unsigned short int, const unsigned short int) const;
  void Polar_ring_position(const float, const unsigned short int, const unsigned short int) const;
  void Polar_ring_angle(const signed short int, const unsigned short int, const unsigned short int) const;
  void Polar_start_angle(const signed short int, const unsigned short int) const;
  void Polar_long_axis_left(const signed short int, const unsigned short int, const unsigned short int) const;
  void Polar_long_axis_right(const signed short int, const unsigned short int, const unsigned short int) const;
  void Polar_position_data(const signed short int, const unsigned short int) const;
  void Polar_image_min(const signed short int, const unsigned short int) const;
  void Polar_image_max(const signed short int, const unsigned short int) const;
  void Polar_scale_factor(const float, const unsigned short int) const;
  void Polar_pixel_size(const float, const unsigned short int) const;
  void Polar_frame_duration(const signed long int, const unsigned short int) const;
  void Polar_frame_start_time(const signed long int, const unsigned short int) const;
  void Polar_processing_code(const signed short int, const unsigned short int) const;
  void Polar_quant_units(const signed short int, const unsigned short int) const;
  void Polar_annotation(unsigned char * const, const unsigned short int) const;
  void Polar_gate_duration(const signed long int, const unsigned short int) const;
  void Polar_r_wave_offset(const signed long int, const unsigned short int) const;
  void Polar_num_accepted_beats(const signed long int, const unsigned short int) const;
  void Polar_polar_map_protocol(unsigned char * const, const unsigned short int) const;
  void Polar_database_name(unsigned char * const, const unsigned short int) const;
  void Polar_fill_cti(const signed short int, const unsigned short int, const unsigned short int) const;
  void Polar_fill_user(const signed short int, const unsigned short int, const unsigned short int) const;

  signed short int Polar_data_type(const unsigned short int) const;                      // request values from Polar map header
  signed short int Polar_polar_map_type(const unsigned short int) const;
  signed short int Polar_num_rings(const unsigned short int) const;
  signed short int Polar_sectors_per_ring(const unsigned short int, const unsigned short int) const;
  float            Polar_ring_position(const unsigned short int, const unsigned short int) const;
  signed short int Polar_ring_angle(const unsigned short int, const unsigned short int) const;
  signed short int Polar_start_angle(const unsigned short int) const;
  signed short int Polar_long_axis_left(const unsigned short int, const unsigned short int) const;
  signed short int Polar_long_axis_right(const unsigned short int, const unsigned short int) const;
  signed short int Polar_position_data(const unsigned short int) const;
  signed short int Polar_image_min(const unsigned short int) const;
  signed short int Polar_image_max(const unsigned short int) const;
  float            Polar_scale_factor(const unsigned short int) const;
  float            Polar_pixel_size(const unsigned short int) const;
  signed long int  Polar_frame_duration(const unsigned short int) const;
  signed long int  Polar_frame_start_time(const unsigned short int) const;
  signed short int Polar_processing_code(const unsigned short int) const;
  signed short int Polar_quant_units(const unsigned short int) const;
  unsigned char   *Polar_annotation(const unsigned short int) const;
  signed long int  Polar_gate_duration(const unsigned short int) const;
  signed long int  Polar_r_wave_offset(const unsigned short int) const;
  signed long int  Polar_num_accepted_beats(const unsigned short int) const;
  unsigned char   *Polar_polar_map_protocol(const unsigned short int) const;
  unsigned char   *Polar_database_name(const unsigned short int) const;
  signed short int Polar_fill_cti(const unsigned short int, const unsigned short int) const;
  signed short int Polar_fill_user(const unsigned short int, const unsigned short int) const;

  void Scan_data_type(const signed short int, const unsigned short int) const;                              // change values in Scan header
  void Scan_num_dimensions(const signed short int, const unsigned short int) const;
  void Scan_num_r_elements(const unsigned short int, const unsigned short int) const;
  void Scan_num_angles(const unsigned short int, const unsigned short int) const;
  void Scan_corrections_applied(const signed short int, const unsigned short int) const;
  void Scan_num_z_elements(const unsigned short int, const unsigned short int) const;
  void Scan_ring_difference(const signed short int, const unsigned short int) const;
  void Scan_x_resolution(const float, const unsigned short int) const;
  void Scan_y_resolution(const float, const unsigned short int) const;
  void Scan_z_resolution(const float, const unsigned short int) const;
  void Scan_w_resolution(const float, const unsigned short int) const;
  void Scan_fill_gate(const signed short int, const unsigned short int, const unsigned short int) const;
  void Scan_gate_duration(const signed long int, const unsigned short int) const;
  void Scan_r_wave_offset(const signed long int, const unsigned short int) const;
  void Scan_num_accepted_beats(const signed long int, const unsigned short int) const;
  void Scan_scale_factor(const float, const unsigned short int) const;
  void Scan_scan_min(const signed short int, const unsigned short int) const;
  void Scan_scan_max(const signed short int, const unsigned short int) const;
  void Scan_prompts(const signed long int, const unsigned short int) const;
  void Scan_delayed(const signed long int, const unsigned short int) const;
  void Scan_multiples(const signed long int, const unsigned short int) const;
  void Scan_net_trues(const signed long int, const unsigned short int) const;
  void Scan_cor_singles(const float, const unsigned short int, unsigned short int) const;
  void Scan_uncor_singles(const float, const unsigned short int, unsigned short int) const;
  void Scan_tot_avg_cor(const float, const unsigned short int) const;
  void Scan_tot_avg_uncor(const float, const unsigned short int) const;
  void Scan_total_coin_rate(const signed long int, const unsigned short int) const;
  void Scan_frame_start_time(const signed long int, const unsigned short int) const;
  void Scan_frame_duration(const signed long int, const unsigned short int) const;
  void Scan_deadtime_correction_factor(const float, const unsigned short int) const;
  void Scan_physical_planes(const signed short int, const unsigned short int, const unsigned short int) const;
  void Scan_fill_cti(const signed short int, const unsigned short int, const unsigned short int) const;
  void Scan_fill_user(const signed short int, const unsigned short int, const unsigned short int) const;

  signed short int   Scan_data_type(const unsigned short int) const;                           // request values from Scan header
  signed short int   Scan_num_dimensions(const unsigned short int) const;
  unsigned short int Scan_num_r_elements(const unsigned short int) const;
  unsigned short int Scan_num_angles(const unsigned short int) const;
  signed short int   Scan_corrections_applied(const unsigned short int) const;
  unsigned short int Scan_num_z_elements(const unsigned short int) const;
  signed short int   Scan_ring_difference(const unsigned short int) const;
  float              Scan_x_resolution(const unsigned short int) const;
  float              Scan_y_resolution(const unsigned short int) const;
  float              Scan_z_resolution(const unsigned short int) const;
  float              Scan_w_resolution(const unsigned short int) const;
  signed short int   Scan_fill_gate(const unsigned short int, const unsigned short int) const;
  signed long int    Scan_gate_duration(const unsigned short int) const;
  signed long int    Scan_r_wave_offset(const unsigned short int) const;
  signed long int    Scan_num_accepted_beats(const unsigned short int) const;
  float              Scan_scale_factor(const unsigned short int) const;
  signed short int   Scan_scan_min(const unsigned short int) const;
  signed short int   Scan_scan_max(const unsigned short int) const;
  signed long int    Scan_prompts(const unsigned short int) const;
  signed long int    Scan_delayed(const unsigned short int) const;
  signed long int    Scan_multiples(const unsigned short int) const;
  signed long int    Scan_net_trues(const unsigned short int) const;
  float              Scan_cor_singles(const unsigned short int, const unsigned short int) const;
  float              Scan_uncor_singles(const unsigned short int, const unsigned short int) const;
  float              Scan_tot_avg_cor(const unsigned short int) const;
  float              Scan_tot_avg_uncor(const unsigned short int) const;
  signed long int    Scan_total_coin_rate(const unsigned short int) const;
  signed long int    Scan_frame_start_time(const unsigned short int) const;
  signed long int    Scan_frame_duration(const unsigned short int) const;
  float              Scan_deadtime_correction_factor(const unsigned short int) const;
  signed short int   Scan_physical_planes(const unsigned short int, const unsigned short int) const;
  signed short int   Scan_fill_cti(const unsigned short int, const unsigned short int) const;
  signed short int   Scan_fill_user(const unsigned short int, const unsigned short int) const;

  void Scan3D_data_type(const signed short int, const unsigned short int) const;                            // change values in Scan3D header
  void Scan3D_num_dimensions(const signed short int, const unsigned short int) const;
  void Scan3D_num_r_elements(const unsigned short int, const unsigned short int) const;
  void Scan3D_num_angles(const unsigned short int, const unsigned short int) const;
  void Scan3D_corrections_applied(const signed short int, const unsigned short int) const;
  void Scan3D_num_z_elements(const unsigned short int, const unsigned short int, const unsigned short int) const;
  void Scan3D_ring_difference(const signed short int, const unsigned short int) const;
  void Scan3D_storage_order(const signed short int, const unsigned short int) const;
  void Scan3D_axial_compression(const signed short int, const unsigned short int) const;
  void Scan3D_x_resolution(const float, const unsigned short int) const;
  void Scan3D_v_resolution(const float, const unsigned short int) const;
  void Scan3D_z_resolution(const float, const unsigned short int) const;
  void Scan3D_w_resolution(const float, const unsigned short int) const;
  void Scan3D_fill_gate(const signed short int, const unsigned short int, const unsigned short int) const;
  void Scan3D_gate_duration(const signed long int, const unsigned short int) const;
  void Scan3D_r_wave_offset(const signed long int, const unsigned short int) const;
  void Scan3D_num_accepted_beats(const signed long int, const unsigned short int) const;
  void Scan3D_scale_factor(const float, const unsigned short int) const;
  void Scan3D_scan_min(const signed short int, const unsigned short int) const;
  void Scan3D_scan_max(const signed short int, const unsigned short int) const;
  void Scan3D_prompts(const signed long int, const unsigned short int) const;
  void Scan3D_delayed(const signed long int, const unsigned short int) const;
  void Scan3D_multiples(const signed long int, const unsigned short int) const;
  void Scan3D_net_trues(const signed long int, const unsigned short int) const;
  void Scan3D_tot_avg_cor(const float, const unsigned short int) const;
  void Scan3D_tot_avg_uncor(const float, const unsigned short int) const;
  void Scan3D_total_coin_rate(const signed long int, const unsigned short int) const;
  void Scan3D_frame_start_time(const signed long int, const unsigned short int) const;
  void Scan3D_frame_duration(const signed long int, const unsigned short int) const;
  void Scan3D_deadtime_correction_factor(const float, const unsigned short int) const;
  void Scan3D_fill_cti(const signed short int, const unsigned short int, const unsigned short int) const;
  void Scan3D_fill_user(const signed short int, const unsigned short int, const unsigned short int) const;
  void Scan3D_uncor_singles(const float, const unsigned short int, const unsigned short int) const;

  signed short int   Scan3D_data_type(const unsigned short int) const;                         // request values from Scan3D header
  signed short int   Scan3D_num_dimensions(const unsigned short int) const;
  unsigned short int Scan3D_num_r_elements(const unsigned short int) const;
  unsigned short int Scan3D_num_angles(const unsigned short int) const;
  signed short int   Scan3D_corrections_applied(                                               const unsigned short int) const;
  unsigned short int Scan3D_num_z_elements(const unsigned short int, const unsigned short int) const;
  signed short int   Scan3D_ring_difference(const unsigned short int) const;
  signed short int   Scan3D_storage_order(const unsigned short int) const;
  signed short int   Scan3D_axial_compression(const unsigned short int) const;
  float              Scan3D_x_resolution(const unsigned short int) const;
  float              Scan3D_v_resolution(const unsigned short int) const;
  float              Scan3D_z_resolution(const unsigned short int) const;
  float              Scan3D_w_resolution(const unsigned short int) const;
  signed short int   Scan3D_fill_gate(const unsigned short int, const unsigned short int) const;
  signed long int    Scan3D_gate_duration(const unsigned short int) const;
  signed long int    Scan3D_r_wave_offset(const unsigned short int) const;
  signed long int    Scan3D_num_accepted_beats(const unsigned short int) const;
  float              Scan3D_scale_factor(const unsigned short int) const;
  signed short int   Scan3D_scan_min(const unsigned short int) const;
  signed short int   Scan3D_scan_max(const unsigned short int) const;
  signed long int    Scan3D_prompts(const unsigned short int) const;
  signed long int    Scan3D_delayed(const unsigned short int) const;
  signed long int    Scan3D_multiples(const unsigned short int) const;
  signed long int    Scan3D_net_trues(const unsigned short int) const;
  float              Scan3D_tot_avg_cor(const unsigned short int) const;
  float              Scan3D_tot_avg_uncor(const unsigned short int) const;
  signed long int    Scan3D_total_coin_rate(const unsigned short int) const;
  signed long int    Scan3D_frame_start_time(const unsigned short int) const;
  signed long int    Scan3D_frame_duration(const unsigned short int) const;
  float              Scan3D_deadtime_correction_factor(const unsigned short int) const;
  signed short int   Scan3D_fill_cti(const unsigned short int, const unsigned short int) const;
  signed short int   Scan3D_fill_user(const unsigned short int, const unsigned short int) const;
  float              Scan3D_uncor_singles(const unsigned short int, const unsigned short int) const;
};
