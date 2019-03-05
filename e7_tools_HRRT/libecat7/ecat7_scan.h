/* \file ecat7_scan.hpp
    \brief This class implements the SCAN matrix type of imported ECAT6.5           files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/17 added Doxygen style comments
*/

#pragma once

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"

/*- definitions -------------------------------------------------------------*/

// header of scan matrix (512 byte)
class ECAT7_SCAN: public ECAT7_MATRIX{
public:
  typedef struct {
    signed short int data_type,    // datatype of dataset
           num_dimensions;       // number of dimensions
    unsigned short int num_r_elements,             // number of bins in a projection
             num_angles;         // number of angles in sinogram plane
    signed short int corrections_applied;        // applied corrections
    unsigned short int num_z_elements;               // number of planes in sinogram
    signed short int ring_difference;    // maximum ring difference
    float x_resolution,         // resolution in radial direction in cm (bin size)
          y_resolution, // resolution in angular direction in radians
          z_resolution,      // resolution in z direction in cm (plane separation)
          w_resolution;       // not used
    signed short int fill_gate[6]; // reserved for gating
    signed long int gate_duration,      // gate duration in msec
           r_wave_offset,      // time from start of first gate in msec
           num_accepted_beats;     // number of accepted beats for this gate
    float scale_factor; // scale factor from int to float
    signed short int scan_min,             // minimum value if data is in integer format
           scan_max;             // maximum value if data is in integer format
    signed long int prompts,         // number of prompts
           delayed,         // number of randoms
           multiples,     // number of multiples
           net_trues;         // number of trues
    float cor_singles[16],            // total singles with loss correction factoring
          uncor_singles[16],         // total singles without loss correction factoring
          tot_avg_cor,       // mean value of loss corrected singles
          tot_avg_uncor;         // mean value of singles
    signed long int total_coin_rate,     // total coincidence rate
           frame_start_time,          // duration of current frame in msec
           frame_duration;
    float deadtime_correction_factor;      // deadtime correction factor applied to the sinogram
    signed short int physical_planes[8],          // physical plane that make up this logical plane
           fill_cti[83],  // CPS reserved space
           fill_user[50];// user reserved space
  } tscan_subheader;
private:
  tscan_subheader scan_subheader_;           // header of scan matrix
protected:
  unsigned short int DataTypeOrig() const;       // request data type from matrix header
  unsigned short int Depth() const;       // depth of dataset
  unsigned short int Height() const;     // height of dataset
  unsigned short int Width() const;       // width of dataset
public:
  ECAT7_SCAN();             // initialize object
  ECAT7_SCAN& operator = (const ECAT7_SCAN &);                // '='-operator
  void CutBins(const unsigned short int);            // cut bins on both sides of sinograms
  float DecayCorrection(const float, const bool) const;     // calculate decay correction
  void DeInterleave();      // deinterleave data
  void Feet2Head() const;  // exchange feet and head position
  ECAT7_SCAN::tscan_subheader *HeaderPtr();            // request pointer to header structure
  void Interleave();       // interleave dataset
  void *LoadData(std::ifstream * const, const unsigned long int);       // load data part of matrix
  void LoadHeader(std::ifstream * const);            // load header of matrix
  void PrintHeader(std::list <std::string> * const,     const unsigned short int) const;      // print header information into string list
  void Prone2Supine() const;       // change orientation from prone to supine
  void Rotate(const signed short int) const;         // rotate dataset counterclockwise along scanner axis
  void SaveHeader(std::ofstream * const) const;// store header part of matrix
  void ScaleMatrix(const float); // scale matrix

  // ahc this must be implemented in every class derived from ECAT7_MATRIX
  ecat_matrix::MatrixDataType get_data_type(void);               // Read data_type as short int from file, return as scoped enum
  void set_data_type(ecat_matrix::MatrixDataType t_data_type);   // Write scoped enum to file as short int
};

