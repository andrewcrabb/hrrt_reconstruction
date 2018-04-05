/*! \file ecat7_polar.h
    \brief This class implements the polar map matrix type for ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
 */

#pragma once

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"

/*- class definitions -------------------------------------------------------*/

class ECAT7_POLAR:public ECAT7_MATRIX
 { public:
                                   /*! header of polar map matrix (512 byte) */
    typedef struct { signed short int data_type,    /*!< datatype of dataset */
                                      /*! version of the polar map structure */
                                      polar_map_type,
                                            /*! number of rings in polar map */
                                      num_rings,
                            /*! number of sectors per ring for upto 32 rings */
                                      sectors_per_ring[32];
               /*! fractional distance along the long axis from base to apex */
                     float ring_position[32];
                            /*! ring angle relative to long axis (90 degrees
                                along cylinder, decreasing to 0 at the apex) */
                     signed short int ring_angle[32],
       /*! start angle for rings (always 258 degrees, defines polar map's 0) */
                                      start_angle,
                        /*! x,y,z-location of long axis base end (in pixels) */
                                      long_axis_left[3],
                        /*! x,y,z-location of long axis apex end (in pixels) */
                                      long_axis_right[3],
                                               /*! position data available ? */
                                      position_data,
                                   /*! minimum pixel value in this polar map */
                                      image_min,
                                   /*! maximum pixel value in this polar map */
                                      image_max;
                     float scale_factor, /*!< scale factor from int to float */
                           pixel_size;               /*!< pixel size in cm^3 */
                                               /*! duration of frame in msec */
                     signed long int frame_duration,
                      /*! frame start time in msec (offset from first frame) */
                                     frame_start_time;
                     signed short int processing_code,  /*!< processing code */
                                      quant_units; /*!< quantification units */
                                             /*! label for polar map display */
                     unsigned char annotation[40];
                                                   /*! gate duration in msec */
                     signed long int gate_duration,
                                                   /*! R wave offset in msec */
                                     r_wave_offset,
                                  /*! number of accepted beats for this gate */
                                     num_accepted_beats;
                      /*! polar map protocol used to generate this polar map */
                     unsigned char polar_map_protocol[20],
                             /*! database name used for polar map comparison */
                                   database_name[30];
                     signed short int fill_cti[27],  /*!< CPS reserved space */
                                      fill_user[27];/*!< user reserved space */
                   } tpolar_subheader;
  private:
    tpolar_subheader ph;                     /*!< header of polar map matrix */
   protected:
                                        // request data type from matrix header
    unsigned short int DataTypeOrig() const;
   public:
    ECAT7_POLAR();                                         // initialize object
    ECAT7_POLAR& operator = (const ECAT7_POLAR &);              // '='-operator
                                         // request pointer to header structure
    ECAT7_POLAR::tpolar_subheader *HeaderPtr();
                                                    // load data part of matrix
    void *LoadData(std::ifstream * const, const unsigned long int);
    void LoadHeader(std::ifstream * const);            // load header of matrix
                                   // print header information into string list
    void PrintHeader(std::list <std::string> * const,
                     const unsigned short int) const;
                                                 // store header part of matrix
    void SaveHeader(std::ofstream * const) const;
 };

