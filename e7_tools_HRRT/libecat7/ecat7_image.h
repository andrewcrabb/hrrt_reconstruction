/*! \file ecat7_image.h
    \brief This class implements the IMAGE matrix type of ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/19 added Doxygen style comments
 */

#ifndef _ECAT7_IMAGE_H
#define _ECAT7_IMAGE_H

#include <fstream>
#include <list>
#include <string>
#include "ecat7_matrix_abc.h"
 
/*- definitions -------------------------------------------------------------*/

class ECAT7_IMAGE:public ECAT7_MATRIX
 { public:
                                       /*! header of image matrix (512 byte) */
    typedef struct { signed short int data_type,    /*!< datatype of dataset */
                                                    /*! number of dimensions */
                                      num_dimensions,
                                      x_dimension,       /*!< width of image */
                                      y_dimension,      /*!< height of image */
                                      z_dimension;       /*!< depth of image */
                     float x_offset,                     /*!< x-offset in cm */
                           y_offset,                     /*!< y-offset in cm */
                           z_offset,                     /*!< z-offset in cm */
                           recon_zoom,       /*!< reconstruction zoom factor */
                           scale_factor;    /*!< quantification scale factor */
                                               /*! image minimum voxel value */
                     signed short int image_min,
                                               /*! image maximum voxel value */
                                      image_max;
                     float x_pixel_size,              /*!< voxel width in cm */
                           y_pixel_size,             /*!< voxel height in cm */
                           z_pixel_size;              /*!< voxel depth in cm */
                                                  /*! frame duration in msec */
                     signed long int frame_duration,
                        /*! frame start time relative to first frame in msec */
                                     frame_start_time;
                     signed short int filter_code;          /*!< filter code */
                     float x_resolution,/*!< resolution in x-dimension in cm */
                           y_resolution,/*!< resolution in y-dimension in cm */
                           z_resolution,/*!< resolution in z-dimension in cm */
                           num_r_elements, /*!< number of bins in projection */
                                      /*! number of angles in sinogram plane */
                           num_angles,
                                   /*! rotation in the x/y-plane in degrees
                                              (right-hand coordinate system) */
                           z_rotation_angle,
                           decay_corr_fctr;     /*!< decay correction factor */
                     signed long int processing_code,   /*!< processing code */
                                                   /*! gate duration in msec */
                                     gate_duration,
                                     r_wave_offset,       /*!< R wave offset */
                                  /*! number of accepted beats for this gate */
                                     num_accepted_beats;
                     float filter_cutoff_frequency,   /*!< cut-off frequency */
                           filter_resolution,                  /*!< not used */
                           filter_ramp_slope;                  /*!< not used */
                     signed short int filter_order;            /*!< not used */
                     float filter_scatter_fraction,            /*!< not used */
                           filter_scatter_slope;               /*!< not used */
                     unsigned char annotation[40];     /*!< image annotation */
                     float mt_1_1,         /*!< transformation matrix m(1,1) */
                           mt_1_2,         /*!< transformation matrix m(1,2) */
                           mt_1_3,         /*!< transformation matrix m(1,3) */
                           mt_2_1,         /*!< transformation matrix m(2,1) */
                           mt_2_2,         /*!< transformation matrix m(2,2) */
                           mt_2_3,         /*!< transformation matrix m(2,3) */
                           mt_3_1,         /*!< transformation matrix m(3,1) */
                           mt_3_2,         /*!< transformation matrix m(3,2) */
                           mt_3_3,         /*!< transformation matrix m(3,3) */
                           rfilter_cutoff,        /*!< radial filter cut-off */
                           rfilter_resolution; /*!< radial filter resolution */
                     signed short int rfilter_code,  /*!< radial filter code */
                                      rfilter_order;/*!< radial filter order */
                     float zfilter_cutoff,             /*!< z-filter cut-off */
                           zfilter_resolution;      /*!< z-filter resolution */
                     signed short int zfilter_code,       /*!< z-filter code */
                                      zfilter_order;     /*!< z-filter order */
                     float mt_1_4,         /*!< transformation matrix m(1,4) */
                           mt_2_4,         /*!< transformation matrix m(2,4) */
                           mt_3_4;         /*!< transformation matrix m(3,4) */
                                                 /*! scatter correction type */
                     signed short int scatter_type,
                                      recon_type, /*!< reconstruction method */
                                 /*!< number of views used in reconstruction */
                                      recon_views,
                                      fill_cti[87],  /*!< CPS reserved space */
                                      fill_user[48];/*!< user reserved space */
                   } timage_subheader;
   private:
    ECAT7_IMAGE::timage_subheader ih;            /*!< header of image matrix */
   protected:
                                        // request data type from matrix header
    unsigned short int DataTypeOrig() const;
    unsigned short int Depth() const;                       // depth of dataset
    unsigned short int Height() const;                     // height of dataset
    unsigned short int Width() const;                       // width of dataset
   public:
    ECAT7_IMAGE();                                         // initialize object
    ECAT7_IMAGE& operator = (const ECAT7_IMAGE &);              // '='-operator
    void Feet2Head() const;                  // exchange feet and head position
                                         // request pointer to header structure
    ECAT7_IMAGE::timage_subheader *HeaderPtr();
                                                    // load data part of matrix
    void *LoadData(std::ifstream * const, const unsigned long int);
    void LoadHeader(std::ifstream * const);            // load header of matrix
                                   // print header information into string list
    void PrintHeader(std::list <std::string> * const,
                     const unsigned short int) const;
    void Prone2Supine() const;       // change orientation from prone to supine
    void SaveHeader(std::ofstream * const) const;// store header part of matrix
    void ScaleMatrix(const float);                              // scale matrix
 };

#endif
