/*! \class ECAT7_NORM3D ecat7_norm3d.hpp "ecat7_norm3d.hpp"
    \brief This class implements the NORM3D matrix type for ECAT7 files.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 1997/11/11 initial version
    \date 2005/01/18 added Doxygen style comments

    This class is build on top of the ECAT7_MATRIX abstract base class and the
    ecat7_utils module and adds a few things specific to NORM3D matrices, like
    loading, storing and printing the header and data.
 */

#include <cstdio>
#include "ecat7_norm3d.hpp"
#include "data_changer.h"
#include "ecat7_global.h"
#include "fastmath.h"
#include "str_tmpl.h"
#include <string.h>

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object and fill header data structure with 0s.
 */
/*---------------------------------------------------------------------------*/
ECAT7_NORM3D::ECAT7_NORM3D()
 { memset(&nh, 0, sizeof(tnorm3d_subheader));                // header is empty
   plane_corr=NULL;
   intfcorr=NULL;
   crystal_eff=NULL;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
ECAT7_NORM3D::~ECAT7_NORM3D()
 { DeleteData();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] e7   object to copy
    \return this object with new content

    This operator copies the norm3d header and uses the copy operator of the
    base class to copy the dataset from another objects into this one.
 */
/*---------------------------------------------------------------------------*/
ECAT7_NORM3D& ECAT7_NORM3D::operator = (const ECAT7_NORM3D &e7)
 { if (this != &e7)
    { try
      { DeleteData();
                                        // copy local variables into new object
        plane_corr=new float[nh.num_r_elements*nh.num_geo_corr_planes];
        memcpy(plane_corr, e7.plane_corr,
               nh.num_r_elements*nh.num_geo_corr_planes*sizeof(float));

        intfcorr=new float[nh.num_transaxial_crystals*nh.num_r_elements];
        memcpy(intfcorr, e7.intfcorr,
               nh.num_transaxial_crystals*nh.num_r_elements*sizeof(float));

        crystal_eff=new float[nh.crystals_per_ring*nh.num_crystal_rings];
        memcpy(crystal_eff, e7.crystal_eff,
               nh.crystals_per_ring*nh.num_crystal_rings*sizeof(float));

        ECAT7_MATRIX:: operator = (e7);       // call copy operator from parent
      }
      catch (...)
       { DeleteData();
         throw;
       }
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to data for crystal efficiency correction.
    \return pointer to data for crystal efficiency correction

    Request pointer to data for crystal efficiency correction.
 */
/*---------------------------------------------------------------------------*/
float *ECAT7_NORM3D::CrystalEffPtr() const
 { return(crystal_eff);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set pointer to data for crystal efficiency correction.
    \param[in] ptr   pointer to data for crystal efficiency correction

    Set pointer to data for crystal efficiency correction. A local copy of the
    data is created.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::CrystalEffPtr(float * const ptr)
 { try
   { if (crystal_eff != NULL) delete[] crystal_eff;
     crystal_eff=new float[nh.crystals_per_ring*nh.num_crystal_rings];
     memcpy(crystal_eff, ptr,
            nh.crystals_per_ring*nh.num_crystal_rings*sizeof(float));
   }
   catch (...)
    { if (crystal_eff != NULL) { delete[] crystal_eff;
                                 crystal_eff=NULL;
                               }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request datatype from matrix header.
    \return datatype from matrix header

    Request datatype from matrix header.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ECAT7_NORM3D::DataTypeOrig() const
 { return(nh.data_type);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete data part.

    Delete data part.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::DeleteData()
 { if (plane_corr != NULL) { delete[] plane_corr;
                             plane_corr=NULL;
                           }
   if (intfcorr != NULL) { delete[] intfcorr;
                           intfcorr=NULL;
                         }
   if (crystal_eff != NULL) { delete[] crystal_eff;
                              crystal_eff=NULL;
                            }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to header data structure.
    \return pointer to header data structure
 
    Request pointer to header data structure.
 */
/*---------------------------------------------------------------------------*/
ECAT7_NORM3D::tnorm3d_subheader *ECAT7_NORM3D::HeaderPtr()
 { return(&nh);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to data for crystal interference correction.
    \return pointer to data for crystal interference correction

    Request pointer to data for crystal interference correction.
 */
/*---------------------------------------------------------------------------*/
float *ECAT7_NORM3D::IntfCorrPtr() const
 { return(intfcorr);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set pointer to data for crystal interference correction.
    \param[in] ptr   pointer to data for crystal interference correction

    Set pointer to data for crystal interference correction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::IntfCorrPtr(float * const ptr)
 { try
   { if (intfcorr != NULL) delete[] intfcorr;
     intfcorr=new float[nh.num_transaxial_crystals*nh.num_r_elements];
     memcpy(intfcorr, ptr,
            nh.num_transaxial_crystals*nh.num_r_elements*sizeof(float));
   }
   catch (...)
    { if (intfcorr != NULL) { delete[] intfcorr;
                              intfcorr=NULL;
                            }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load dataset of matrix into object.
    \param[in] file             handle of file
    \param[in] matrix_records   number of records to read
    \return pointer to dataset
 
    Load dataset of matrix into object. The header of the matrix, which is two
    records long, is skipped.
 */
/*---------------------------------------------------------------------------*/
void *ECAT7_NORM3D::LoadData(std::ifstream * const file,                             const unsigned long int) { 
  DataChanger *dc=NULL;

   file->seekg(E7_RECLEN, std::ios::cur);                 // skip matrix header
   //   matrix_records--;
   try   { 
    unsigned long int i, size;
     DeleteData();                                           // delete old data
     size=nh.num_r_elements*nh.num_geo_corr_planes;                                    // read data for plane geometric correction
     plane_corr=new float[size];
     dc = new DataChanger(size * sizeof(float));                       // DataChanger is used to read data system independently
     dc->LoadBuffer(file);                             // load data into buffer
     for (i=0; i < size; i++)                                                    // retrieve data from buffer
      dc->Value(&plane_corr[i]);
     delete dc;
     dc=NULL;
     size=nh.num_transaxial_crystals*nh.num_r_elements;                               // read data for crystal interference correction
     intfcorr=new float[size];
     dc = new DataChanger(size * sizeof(float));                       // DataChanger is used to read data system independently
     dc->LoadBuffer(file);                             // load data into buffer

     for (i=0; i < size; i++)                                                    // retrieve data from buffer
      dc->Value(&intfcorr[i]);
     delete dc;
     dc=NULL;
     size=nh.crystals_per_ring*nh.num_crystal_rings;                                 // read data for crystal efficiency correction
     crystal_eff=new float[size];
     dc = new DataChanger(size * sizeof(float));                       // DataChanger is used to read data system independently
     dc->LoadBuffer(file);                             // load data into buffer

     for (i=0; i < size; i++)                                                   // retrieve data from buffer
      dc->Value(&crystal_eff[i]);
     delete dc;
     dc=NULL;
   }   catch (...)    { 
    if (dc != NULL) delete dc;
      DeleteData();
      throw;
    }
   return(NULL);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load header of norm3d matrix.
    \param[in] file   handle of file

    Load header of norm3d matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::LoadHeader(std::ifstream * const file)
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                       // DataChanger is used to read data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->LoadBuffer(file);                             // load data into buffer    
     dc->Value(&nh.data_type);                                               // retrieve data from buffer
     dc->Value(&nh.num_r_elements);
     dc->Value(&nh.num_transaxial_crystals);
     dc->Value(&nh.num_crystal_rings);
     dc->Value(&nh.crystals_per_ring);
     dc->Value(&nh.num_geo_corr_planes);
     dc->Value(&nh.uld);
     dc->Value(&nh.lld);
     dc->Value(&nh.scatter_energy);
     dc->Value(&nh.norm_quality_factor);
     dc->Value(&nh.norm_quality_factor_code);
     for (i=0; i < 32; i++) dc->Value(&nh.ring_dtcor1[i]);
     for (i=0; i < 32; i++) dc->Value(&nh.ring_dtcor2[i]);
     for (i=0; i < 8; i++)  dc->Value(&nh.crystal_dtcor[i]);
     dc->Value(&nh.span);
     dc->Value(&nh.max_ring_diff);
     for (i=0; i < 48; i++) dc->Value(&nh.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(&nh.fill_user[i]);
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate number of records needed for matrix.
    \return number of records

    Calculate number of records needed for matrix. Take into account that the
    norm3d header requires an additional record.
 */
/*---------------------------------------------------------------------------*/
unsigned long int ECAT7_NORM3D::NumberOfRecords() const
 { unsigned long int size=0, records;
   if (plane_corr != NULL)                               // count size of plane geometric correction data
    size+=nh.num_r_elements*nh.num_geo_corr_planes;
   if (intfcorr != NULL)                           // count size of crystal interference correction data
    size+=nh.num_transaxial_crystals*nh.num_r_elements;
   if (crystal_eff != NULL)                             // count size of crystal efficiency correction data
    size+=nh.crystals_per_ring*nh.num_crystal_rings;
   records=(size*sizeof(float))/E7_RECLEN;
   if (((size*sizeof(float)) % E7_RECLEN) != 0) 
    records++;
   return(records);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to data for plane geometric correction.
    \return pointer to data for plane geometric correction

    Request pointer to data for plane geometric correction.
 */
/*---------------------------------------------------------------------------*/
float *ECAT7_NORM3D::PlaneCorrPtr() const
 { return(plane_corr);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set pointer to data for plane geometric correction.
    \param[in] ptr   pointer to data for plane geometric correction

    Set pointer to data for plane geometric correction.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::PlaneCorrPtr(float * const ptr)
 { try
   { if (plane_corr != NULL) delete[] plane_corr;
     plane_corr=new float[nh.num_r_elements*nh.num_geo_corr_planes];
     memcpy(plane_corr, ptr,
            nh.num_r_elements*nh.num_geo_corr_planes*sizeof(float));
   }
   catch (...)
    { if (plane_corr != NULL) { delete[] plane_corr;
                                plane_corr=NULL;
                              }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print header information into string list.
    \param[in] sl    string list
    \param[in] num   matrix number

    Print header information into string list.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::PrintHeader(std::list <std::string> * const sl,
                               const unsigned short int num) const
 { unsigned short int i, j;
   std::string s;

   sl->push_back("************* 3D-Norm-Matrix ("+toString(num, 2)+                 ") *************");
   // s=" data_type:                      "+toString(nh.data_type)+" (";
   // switch (nh.data_type)
   //  { case E7_DATA_TYPE_UnknownMatDataType:
   //     s+="UnknownMatDataType";
   //     break;
   //    case E7_DATA_TYPE_ByteData:
   //     s+="ByteData";
   //     break;
   //    case E7_DATA_TYPE_VAX_Ix2:
   //     s+="VAX_Ix2";
   //     break;
   //    case E7_DATA_TYPE_VAX_Ix4:
   //     s+="VAX_Ix4";
   //     break;
   //    case E7_DATA_TYPE_VAX_Rx4:
   //     s+="VAX_Rx4";
   //     break;
   //    case E7_DATA_TYPE_IeeeFloat:
   //     s+="IeeeFloat";
   //     break;
   //    case E7_DATA_TYPE_SunShort:
   //     s+="SunShort";
   //     break;
   //    case E7_DATA_TYPE_SunLong:
   //     s+="SunLong";
   //     break;
   //    default:
   //     s+="unknown";
   //     break;
   //  }
   // sl->push_back(s+")");
     sl->push_back(print_header_data_type(nh.data_type));
   sl->push_back(" ring elements:                  " + toString(nh.num_r_elements));
   sl->push_back(" transaxial crystals:            " + toString(nh.num_transaxial_crystals));
   sl->push_back(" crystal rings:                  " + toString(nh.num_crystal_rings));
   sl->push_back(" crystal per ring:               " + toString(nh.crystals_per_ring));
   sl->push_back(" geometric correction planes:    " + toString(nh.num_geo_corr_planes));
   sl->push_back(" upper energy limit:             "+toString(nh.uld));
   sl->push_back(" lower energy limit:             "+toString(nh.lld));
   sl->push_back(" scatter energy:                 " + toString(nh.scatter_energy));
   sl->push_back(" norm quality factor:            " + toString(nh.norm_quality_factor));
   sl->push_back(" norm quality factor code:       " + toString(nh.norm_quality_factor_code));
   sl->push_back(" ring dead time corr. coeff. 1:  ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       s+=toString(nh.ring_dtcor1[i*8+j], 8)+" ";
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" ring dead time corr. coeff. 2:  ");
   for (i=0; i < 4; i++)
    { s=std::string();
      for (j=0; j < 8; j++)
       s+=toString(nh.ring_dtcor2[i*8+j], 8)+" ";
      if (s.length() > 0) sl->push_back(s);
    }
   sl->push_back(" crystal dead time corr.:        ");
   s=std::string();
   for (i=0; i < 8; i++)
    s+=toString(nh.crystal_dtcor[i], 8)+" ";
   if (s.length() > 0) sl->push_back(s);
   sl->push_back(" span:                           "+toString(nh.span));
   sl->push_back(" maximum ring difference:        "+
                 toString(nh.max_ring_diff));
   /*
   for (i=0; i < 48; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(nh.fill_cti[i]));
   for (i=0; i < 50; i++)
    sl->push_back(" fill ("+toString(i, 2)+"):                     "+
                  toString(nh.fill_user[i]));
   */
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of norm3d matrix.
    \param[in] file   handle of file

    Store header of norm3d matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::SaveData(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned long int i, size, fsize=0;
                                   // write data for plane geometric correction
     if (plane_corr != NULL)
      { size=nh.num_r_elements*nh.num_geo_corr_planes;
        dc = new DataChanger(size * sizeof(float));
        for (i=0; i < size; i++) dc->Value(plane_corr[i]);
        dc->SaveBuffer(file);
        delete dc;
        dc=NULL;
        fsize+=size*sizeof(float);
      }
                              // write data for crystal interference correction
     if (intfcorr != NULL)
      { size=nh.num_transaxial_crystals*nh.num_r_elements;
        dc = new DataChanger(size * sizeof(float));
        for (i=0; i < size; i++) dc->Value(intfcorr[i]);
        dc->SaveBuffer(file);
        delete dc;
        dc=NULL;
        fsize+=size*sizeof(float);
      }
                                // write data for crystal efficiency correction
     if (crystal_eff != NULL)
      { size=nh.crystals_per_ring*nh.num_crystal_rings;
        dc = new DataChanger(size * sizeof(float));
        for (i=0; i < size; i++) dc->Value(crystal_eff[i]);
        dc->SaveBuffer(file);
        delete dc;
        dc=NULL;
        fsize+=size*sizeof(float);
      }
     fsize=fsize % 512;
     dc = new DataChanger(fsize * sizeof(char));
     dc->SaveBuffer(file);
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Store header of norm3d matrix.
    \param[in] file   handle of file

    Store header of norm3d matrix.
 */
/*---------------------------------------------------------------------------*/
void ECAT7_NORM3D::SaveHeader(std::ofstream * const file) const
 { DataChanger *dc=NULL;

   try
   { unsigned short int i;
                      // DataChanger is used to store data system independently
     dc = new DataChanger(E7_RECLEN);
     dc->Value(nh.data_type);
     dc->Value(nh.num_r_elements);
     dc->Value(nh.num_transaxial_crystals);
     dc->Value(nh.num_crystal_rings);
     dc->Value(nh.crystals_per_ring);
     dc->Value(nh.num_geo_corr_planes);
     dc->Value(nh.uld);
     dc->Value(nh.lld);
     dc->Value(nh.scatter_energy);
     dc->Value(nh.norm_quality_factor);
     dc->Value(nh.norm_quality_factor_code);
     for (i=0; i < 32; i++) dc->Value(nh.ring_dtcor1[i]);
     for (i=0; i < 32; i++) dc->Value(nh.ring_dtcor2[i]);
     for (i=0; i < 8; i++) dc->Value(nh.crystal_dtcor[i]);
     dc->Value(nh.span);
     dc->Value(nh.max_ring_diff);
     for (i=0; i < 48; i++) dc->Value(nh.fill_cti[i]);
     for (i=0; i < 50; i++) dc->Value(nh.fill_user[i]);
     dc->SaveBuffer(file);                                      // store header
     delete dc;
     dc=NULL;
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      throw;
    }
 }
