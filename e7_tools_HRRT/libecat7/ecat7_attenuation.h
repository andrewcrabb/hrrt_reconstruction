/*! \file ecat7_attenuation.h
    \brief This class implements the ATTENUATION matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2004/09/22 added Doxygen style comments
 */

#ifndef _ECAT7_ATTENUATION_H
#define _ECAT7_ATTENUATION_H

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"

/*- class definitions -------------------------------------------------------*/

class ECAT7_ATTENUATION:public ECAT7_MATRIX
 { public:
                                 /*! header of attenuation matrix (512 byte) */
    typedef struct { signed short int data_type,    /*!< datatype of dataset */
                                                    /*! number of dimensions */
                                      num_dimensions,
                                      attenuation_type;/*!< attenuation type */
                                          /*! number of bins in a projection */
                     unsigned short int num_r_elements,
                                         /* number of angles in a projection */
                                        num_angles,
                                             /* number of planes in sinogram */
                                        num_z_elements;
                                     /*! maximum ring difference of sinogram */
                     signed short int ring_difference;
                     float x_resolution,/*!< resolution in x-dimension in cm */
                           y_resolution,/*!< resolution in y-dimension in cm */
                           z_resolution,/*!< resolution in z-dimension in cm */
                           w_resolution,               /*!< to be determined */
                           scale_factor,                   /*!< scale factor */
                              /*! ellipse offset in x-axis from center in cm */
                           x_offset,
                              /*! ellipse offset in y-axis from center in cm */
                           y_offset,
                           x_radius,     /*!< ellipse radius in x-axis in cm */
                           y_radius,     /*!< ellipse radius in y-axis in cm */
                                    /*! tilt angle of the ellipse in degrees */
                           tilt_angle,
                           attenuation_coeff,           /*!< u-value in 1/cm */
                                      /*!< minimum value in attenuation data */
                           attenuation_min,
                                      /*!< maximum value in attenuation data */
                           attenuation_max,
                           skull_thickness;       /*!< skull thickness in cm */
         /*! number of attenuation coefficients other than the u-value above */
                     signed short int num_additional_atten_coeff;
                                     /*! additional attenuation coefficients */
                     float additional_atten_coeff[8],
                         /*! the threshold value used by the automatic edge-
                             detection routine (fraction of maximum)         */
                           edge_finding_threshold;
                           /*! data storage order rtzd or rztd (volume/view) */
                     signed short int storage_order,
                                      span,             /*< span of sinogram */
                                       /*! number of planes for each segment */
                                      z_elements[64],
                                      fill_unused[86],           /*!< unused */
                                      fill_user[50];             /*!< unused */
                   } tattenuation_subheader;
   private:
    tattenuation_subheader ah;             /*!< header of attenuation matrix */
   protected:
                                        // request data type from matrix header
    unsigned short int DataTypeOrig() const;
    unsigned short int Depth() const;                       // depth of dataset
    unsigned short int Height() const;                     // height of dataset
    unsigned short int Width() const;                       // width of dataset
   public:
    ECAT7_ATTENUATION();                                   // initialize object
                                                                // '='-operator
    ECAT7_ATTENUATION& operator = (const ECAT7_ATTENUATION &);
                                         // cut bins on both sides of sinograms
    void CutBins(const unsigned short int);
    void DeInterleave();                                   // deinterleave data
    void Feet2Head() const;                  // exchange feet and head position
                                         // request pointer to header structure
    ECAT7_ATTENUATION::tattenuation_subheader *HeaderPtr();
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
    void Tra2Emi() const;       // convert transmission data into emission data
    void View2Volume() const;     // convert dataset from view to volume format
    void Volume2View() const;     // convert dataset from volume to view format
 };

#endif
