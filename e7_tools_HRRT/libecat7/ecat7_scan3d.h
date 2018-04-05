/*! \file ecat7_scan3d.h
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

class ECAT7_SCAN3D:public ECAT7_MATRIX
 { public:
                                     /*! header of scan3d matrix (1024 byte) */
    typedef struct { signed short int data_type,    /*!< datatype of dataset */
                                                    /*! number of dimensions */
                                      num_dimensions;
                                          /*! number of bins in a projection */
                     unsigned short int num_r_elements,
                                      /*! number of angles in sinogram plane */
                                        num_angles;
                                                     /*! applied corrections */
                     signed short int corrections_applied;
                                      /*! number of planes per sinogram axis */
                     unsigned short int num_z_elements[64];
                                                 /*! maximum ring difference */
                     signed short int ring_difference,
                                                   /*! vier or volume mode ? */
                                      storage_order,
                                      axial_compression;           /*!< span */
                         /*! resolution in radial direction in cm (bin size) */
                     float x_resolution,
                              /*! resolution in angular direction in radians */
                           v_resolution,
                      /*! resolution in z direction in cm (plane separation) */
                           z_resolution,
                           w_resolution;                       /*!< not used */
                     signed short int fill_gate[6]; /*!< reserved for gating */
                                                   /*! gate duration in msec */
                     signed long int gate_duration,
                                   /*! time from start of first gate in msec */
                                     r_wave_offset,
                                  /*! number of accepted beats for this gate */
                                     num_accepted_beats;
                     float scale_factor; /*!< scale factor from int to float */
                             /*!< minimum value if data is in integer format */
                     signed short int scan_min,
                             /*!< maximum value if data is in integer format */
                                      scan_max;
                     signed long int prompts,         /*!< number of prompts */
                                     delayed,         /*!< number of randoms */
                                     multiples,     /*!< number of multiples */
                                     net_trues;         /*!< number of trues */
                                    /*! mean value of loss corrected singles */
                     float tot_avg_cor,
                           tot_avg_uncor;         /*!< mean value of singles */
                                                  /*! total coincidence rate */
                     signed long int total_coin_rate,
                               /*! time offset from first frame time in msec */
                                     frame_start_time,
                                       /*! duration of current frame in msec */
                                     frame_duration;
                      /*! deadtime correction factor applied to the sinogram */
                     float deadtime_correction_factor;
                     signed short int fill_cti[90],  /*!< CPS reserved space */
                                      fill_user[50];/*!< user reserved space */
                                    /*! uncorrected singles from each bucket */
                     float uncor_singles[128];
                   } tscan3d_subheader;
   private:
    tscan3d_subheader sh;                       /*!< header of scan3d matrix */
   protected:
                                        // request data type from matrix header
    unsigned short int DataTypeOrig() const;
    unsigned short int Depth() const;                       // depth of dataset
    unsigned short int Height() const;                     // height of dataset
    unsigned short int Width() const;                       // width of dataset
   public:
    ECAT7_SCAN3D();                                        // initialize object
    ECAT7_SCAN3D& operator = (const ECAT7_SCAN3D &);            // '='-operator
                                         // cut bins on both sides of sinograms
    void CutBins(const unsigned short int);
                                                  // calculate decay correction
    float DecayCorrection(const float, const bool) const;
    void DeInterleave();                                   // deinterleave data
    void Feet2Head() const;                  // exchange feet and head position
                                         // request pointer to header structure
    ECAT7_SCAN3D::tscan3d_subheader *HeaderPtr();
    void Interleave();                                    // interleave dataset
                                                    // load data part of matrix
    void *LoadData(std::ifstream * const, const unsigned long int);
    void LoadHeader(std::ifstream * const);            // load header of matrix
                            // calculate number of records needed for data part
    unsigned long int NumberOfRecords() const;
                                   // print header information into string list
    void PrintHeader(std::list <std::string> * const,
                     const unsigned short int) const;
    void Prone2Supine() const;       // change orientation from prone to supine
                         // rotate dataset counterclockwise along scanner axis
    void Rotate(const signed short int) const;
    void SaveHeader(std::ofstream * const) const;// store header part of matrix
    void ScaleMatrix(const float);                              // scale matrix
    void View2Volume() const;     // convert dataset from view to volume format
    void Volume2View() const;     // convert dataset from volume to view format
 };
