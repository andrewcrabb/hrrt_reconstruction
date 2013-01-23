/*! \file ecat7_norm.h
    \brief This class implements the NORM matrix type for ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments
 */

#ifndef _ECAT7_NORM_H
#define _ECAT7_NORM_H

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"

/*- class definitions -------------------------------------------------------*/

class ECAT7_NORM:public ECAT7_MATRIX
 { public:
                                        /*! header of norm matrix (512 byte) */
    typedef struct { signed short int data_type,    /*!< datatype of dataset */
                                                    /*! number of dimensions */
                                      num_dimensions;
                                            /*! number of bins in projection */
                     unsigned short int num_r_elements,
                                      /*! number of angles in sinogram plane */
                                        num_angles,
                                               /*! number of sinogram planes */
                                        num_z_elements;
                                                 /*! maximum ring difference */
                     signed short int ring_difference;
                     float scale_factor,     /*!< normalization scale factor */
                                /*! minimum value for the normalization data */
                           norm_min,
                                /*! maximum value for the normalization data */
                           norm_max,
                                     /*! width of normalization source in cm */
                           fov_source_width,
                           norm_quality_factor;     /*!< norm quality factor */
                                                             /*! not defined */
                     signed short int norm_quality_factor_code,
                                      storage_order,/*!< view or volume mode */
                                      span,                        /*!< span */
                                      /*! number of planes per sinogram axis */
                                      z_elements[64],
                                      fill_cti[123], /*!< CPS reserved space */
                                      fill_user[50];/*!< user reserved space */
                   } tnorm_subheader;
   private:
    tnorm_subheader nh;                           /*!< header of norm matrix */
   protected:
                                        // request data type from matrix header
    unsigned short int DataTypeOrig() const;
    unsigned short int Depth() const;                       // depth of dataset
    unsigned short int Height() const;                     // height of dataset
    unsigned short int Width() const;                       // width of dataset
   public:
    ECAT7_NORM();                                          // initialize object
    ECAT7_NORM& operator = (const ECAT7_NORM &);                // '='-operator
                                         // request pointer to header structure
    ECAT7_NORM::tnorm_subheader *HeaderPtr();
                                                    // load data part of matrix
    void *LoadData(std::ifstream * const, const unsigned long int);
    void LoadHeader(std::ifstream * const);            // load header of matrix
                                   // print header information into string list
    void PrintHeader(std::list <std::string> * const,
                     const unsigned short int) const;
    void SaveHeader(std::ofstream * const) const;// store header part of matrix
 };

#endif
