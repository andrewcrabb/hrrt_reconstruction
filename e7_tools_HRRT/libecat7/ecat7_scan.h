/*! \file ecat7_scan.h
    \brief This class implements the SCAN matrix type of imported ECAT6.5
           files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/17 added Doxygen style comments
 */

#ifndef _ECAT7_SCAN_H
#define _ECAT7_SCAN_H

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"

/*- definitions -------------------------------------------------------------*/

class ECAT7_SCAN:public ECAT7_MATRIX
 { public:
                                        /*! header of scan matrix (512 byte) */
    typedef struct { signed short int data_type,    /*!< datatype of dataset */
                                                    /*! number of dimensions */
                                      num_dimensions;
                                          /*! number of bins in a projection */
                     unsigned short int num_r_elements,
                                      /*! number of angles in sinogram plane */
                                        num_angles;
                                                     /*! applied corrections */
                     signed short int corrections_applied;
                                            /*! number of planes in sinogram */
                     unsigned short int num_z_elements;
                                                 /*! maximum ring difference */
                     signed short int ring_difference;
                         /*! resolution in radial direction in cm (bin size) */
                     float x_resolution,
                              /*! resolution in angular direction in radians */
                           y_resolution,
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
                            /*! total singles with loss correction factoring */
                     float cor_singles[16],
                         /*! total singles without loss correction factoring */
                           uncor_singles[16],
                                    /*! mean value of loss corrected singles */
                           tot_avg_cor,
                           tot_avg_uncor;         /*!< mean value of singles */
                                                  /*! total coincidence rate */
                     signed long int total_coin_rate,
                               /*! time offset from first frame time in msec */
                                     frame_start_time,
                                       /*! duration of current frame in msec */
                                     frame_duration;
                      /*! deadtime correction factor applied to the sinogram */
                     float deadtime_correction_factor;
                          /*! physical plane that make up this logical plane */
                     signed short int physical_planes[8],
                                      fill_cti[83],  /*!< CPS reserved space */
                                      fill_user[50];/*!< user reserved space */
                   } tscan_subheader;
   private:
    tscan_subheader sh;                           /*!< header of scan matrix */
   protected:
                                        // request data type from matrix header
    unsigned short int DataTypeOrig() const;
    unsigned short int Depth() const;                       // depth of dataset
    unsigned short int Height() const;                     // height of dataset
    unsigned short int Width() const;                       // width of dataset
   public:
    ECAT7_SCAN();                                          // initialize object
    ECAT7_SCAN& operator = (const ECAT7_SCAN &);                // '='-operator
                                         // cut bins on both sides of sinograms
    void CutBins(const unsigned short int);
                                                  // calculate decay correction
    float DecayCorrection(const float, const bool) const;
    void DeInterleave();                                   // deinterleave data
    void Feet2Head() const;                  // exchange feet and head position
                                         // request pointer to header structure
    ECAT7_SCAN::tscan_subheader *HeaderPtr();
    void Interleave();                                    // interleave dataset
                                                    // load data part of matrix
    void *LoadData(std::ifstream * const, const unsigned long int);
    void LoadHeader(std::ifstream * const);            // load header of matrix
                                   // print header information into string list
    void PrintHeader(std::list <std::string> * const,
                     const unsigned short int) const;
    void Prone2Supine() const;       // change orientation from prone to supine
                         // rotate dataset counterclockwise along scanner axis
    void Rotate(const signed short int) const;
    void SaveHeader(std::ofstream * const) const;// store header part of matrix
    void ScaleMatrix(const float);                              // scale matrix
 };

#endif
