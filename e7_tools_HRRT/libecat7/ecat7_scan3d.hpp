/* \file ecat7_scan3d.hpp
    \brief This class implements the SCAN3D matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/17 added Doxygen style comments
 */

#pragma once

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"

/*- class definitions -------------------------------------------------------*/

class ECAT7_SCAN3D: public ECAT7_MATRIX {
public:
  typedef struct {  // header of scan3d matrix (1024 byte) */
    signed short int data_type;    // datatype of dataset */
    signed short int num_dimensions;    // vier or volume mode ? */
    unsigned short int num_r_elements;    // maximum ring difference */
    unsigned short int  num_angles;    // number of planes per sinogram axis */
    signed short int corrections_applied;     // applied corrections */
    unsigned short int num_z_elements[64];    // number of angles in sinogram plane */
    signed short int ring_difference;    // number of bins in a projection */
    signed short int storage_order;    // number of dimensions */
    signed short int axial_compression;           // span */
    float x_resolution,    // resolution in radial direction in cm (bin size) */
          v_resolution,          // resolution in angular direction in radians */
          z_resolution,          // resolution in z direction in cm (plane separation) */
          w_resolution;                       // not used */
    signed short int fill_gate[6]; // reserved for gating */
    signed long int gate_duration,    // gate duration in msec */
           r_wave_offset,           // time from start of first gate in msec */
           num_accepted_beats;           // number of accepted beats for this gate */
    float scale_factor; // scale factor from int to float */
    signed short int scan_min,    // minimum value if data is in integer format */
           scan_max;           // maximum value if data is in integer format */
    signed long int prompts,         // number of prompts */
           delayed,         // number of randoms */
           multiples,     // number of multiples */
           net_trues;         // number of trues */
    float tot_avg_cor,    // mean value of loss corrected singles */
          tot_avg_uncor;         // mean value of singles */
    signed long int total_coin_rate,    // total coincidence rate */
           frame_start_time,           // time offset from first frame time in msec */
           frame_duration;           // duration of current frame in msec */
    float deadtime_correction_factor;    // deadtime correction factor applied to the sinogram */
    signed short int fill_cti[90],  // CPS reserved space */
           fill_user[50];// user reserved space */
    float uncor_singles[128];    // uncorrected singles from each bucket */
  } tscan3d_subheader;
private:
  tscan3d_subheader scan3d_subheader_;                       // header of scan3d matrix */
protected:
  // request data type from matrix header
  unsigned short int DataTypeOrig() const;
  unsigned short int Depth() const;                       // depth of dataset
  unsigned short int Height() const;                     // height of dataset
  unsigned short int Width() const;                       // width of dataset
public:
  ECAT7_SCAN3D();                                        // initialize object
  ECAT7_SCAN3D& operator = (const ECAT7_SCAN3D &);            // '='-operator
  void CutBins(const unsigned short int);  // cut bins on both sides of sinograms
  float DecayCorrection(const float, const bool) const;  // calculate decay correction
  void DeInterleave();                                   // deinterleave data
  void Feet2Head() const;                  // exchange feet and head position
  ECAT7_SCAN3D::tscan3d_subheader *HeaderPtr();  // request pointer to header structure
  void Interleave();                                    // interleave dataset
  void *LoadData(std::ifstream * const, const unsigned long int);  // load data part of matrix
  void LoadHeader(std::ifstream * const);            // load header of matrix
  unsigned long int NumberOfRecords() const;  // calculate number of records needed for data part
  void PrintHeader(std::list <std::string> * const,                   const unsigned short int) const;  // print header information into string list
  void Prone2Supine() const;       // change orientation from prone to supine
  void Rotate(const signed short int) const;  // rotate dataset counterclockwise along scanner axis
  void SaveHeader(std::ofstream * const) const;// store header part of matrix
  void ScaleMatrix(const float);                              // scale matrix
  void View2Volume() const;     // convert dataset from view to volume format
  void Volume2View() const;     // convert dataset from volume to view format

  // ahc this must be implemented in every class derived from ECAT7_MATRIX
  ecat_matrix::MatrixDataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  void set_data_type(ecat_matrix::MatrixDataType t_data_type);   // Write scoped enum to file as short int
};
