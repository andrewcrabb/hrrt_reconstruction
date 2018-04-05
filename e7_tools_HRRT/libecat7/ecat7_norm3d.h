/*! \file ecat7_norm3d.h
    \brief This class implements the NORM3D matrix type for ECAT7 files.
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

class ECAT7_NORM3D:public ECAT7_MATRIX
 { public:
                                      /*! header of norm3d matrix (512 byte) */
    typedef struct { signed short int data_type,    /*!< datatype of dataset */
                                            /*! number of bins in projection */
                                      num_r_elements,
                                 /*! number of transaxial crystals per block */
                                      num_transaxial_crystals,
                                                 /*! number of crystal rings */
                                      num_crystal_rings,
                                             /*! number of crystals per ring */
                                      crystals_per_ring,
                  /*! number of rows in the plane geometric correction array */
                                      num_geo_corr_planes,
                                      uld,    /*!< upper energy limit in keV */
                                      lld,    /*!< lower energy limit in keV */
                                                /*! scatter energy threshold */
                                      scatter_energy;
                     float norm_quality_factor;     /*!< norm quality factor */
                                                             /*! not defined */
                     signed short int norm_quality_factor_code;
                       /*! first "per ring" dead time correction coefficient */
                     float ring_dtcor1[32],
                      /*! second "per ring" dead time correction coefficient */
                           ring_dtcor2[32],
                    /*! dead time correction factors for transaxial crystals */
                           crystal_dtcor[8];
                     signed short int span,                        /*!< span */
                                                /*!< maximum ring difference */
                                      max_ring_diff,
                                      fill_cti[48],  /*!< CPS reserved space */
                                      fill_user[50];/*!< user reserved space */
                   } tnorm3d_subheader;
   private:
    tnorm3d_subheader nh;                       /*!< header of norm3d matrix */
    float *plane_corr,              /*!< data for plane geometric correction */
          *intfcorr,           /*!< data for crystal interference correction */
          *crystal_eff;          /*!< data for crystal efficiency correction */
   protected:
                                        // request data type from matrix header
    unsigned short int DataTypeOrig() const;
   public:
    ECAT7_NORM3D();                                        // initialize object
    virtual ~ECAT7_NORM3D();                                      // destructor
    ECAT7_NORM3D& operator = (const ECAT7_NORM3D &);            // '='-operator
                           // pointer to data for crystal efficiency correction
    float *CrystalEffPtr() const;
    void CrystalEffPtr(float * const);
    void DeleteData();                                      // delete data part
                                         // request pointer to header structure
    ECAT7_NORM3D::tnorm3d_subheader *HeaderPtr();
                         // pointer to data for crystal interference correction
    float *IntfCorrPtr() const;
    void IntfCorrPtr(float * const);
                                                    // load data part of matrix
    void *LoadData(std::ifstream * const, const unsigned long int);
    void LoadHeader(std::ifstream * const);            // load header of matrix
                            // calculate number of records needed for data part
    unsigned long int NumberOfRecords() const;
                              // pointer to data for plane geometric correction
    float *PlaneCorrPtr() const;
    void PlaneCorrPtr(float * const);
                                   // print header information into string list
    void PrintHeader(std::list <std::string> * const,
                     const unsigned short int) const;
    void SaveData(std::ofstream * const) const;    // store data part of matrix
    void SaveHeader(std::ofstream * const) const;// store header part of matrix
 };

