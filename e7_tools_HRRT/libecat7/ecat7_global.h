/*! \file ecat7_global.h
    \brief This file contains constants that are used thoughout the ECAT7
           library.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/10 added Doxygen style comments
 */

#ifndef _ECAT7_GLOBAL_H
#define _ECAT7_GLOBAL_H

#include <string>

/*- constants ---------------------------------------------------------------*/

                                               // extensions of ECAT7 filenames
const std::string ECAT7_SINO1_EXTENSION=".S";   /*!< sinogram file extension */
const std::string ECAT7_SINO2_EXTENSION=".s";   /*!< sinogram file extension */
const std::string ECAT7_ATTN_EXTENSION=".a"; /*!< attenuation file extension */
const std::string ECAT7_IMAG_EXTENSION=".v";       /*!< image file extension */
const std::string ECAT7_NORM1_EXTENSION=".N";       /*!< norm file extension */
const std::string ECAT7_NORM2_EXTENSION=".n";       /*!< norm file extension */

                                      /*! size of a record in the ECAT7 file */
const unsigned long int E7_RECLEN=512;
                                                          // data types in file
const int E7_DATA_TYPE_UnknownMatDataType=0;          /*!< unknown data type */
const int E7_DATA_TYPE_ByteData          =1;                  /*!< byte data */
const int E7_DATA_TYPE_VAX_Ix2           =2;    /*!< 2 byte VAX integer data */
const int E7_DATA_TYPE_VAX_Ix4           =3;    /*!< 4 byte VAX integer data */
const int E7_DATA_TYPE_VAX_Rx4           =4;      /*!< 4 byte VAX float data */
const int E7_DATA_TYPE_IeeeFloat         =5;            /*!< IEEE float data */
const int E7_DATA_TYPE_SunShort          =6;  /*!< big endian 2 byte integer */
const int E7_DATA_TYPE_SunLong           =7;  /*!< big endian 4 byte integer */

                                                        // data types in memory
const int E7_DATATYPE_UNKNOWN=0;                      /*!< unknown data type */
const int E7_DATATYPE_FLOAT  =1;                        /*!< float data type */
const int E7_DATATYPE_SHORT  =2;                /*!< short integer data type */
const int E7_DATATYPE_BYTE   =3;                         /*!< byte data type */

                                                    // orientation of sinograms
const int E7_STORAGE_ORDER_RZTD=0;                /*!< sinogram in view mode */
const int E7_STORAGE_ORDER_RTZD=1;              /*!< sinogram in volume mode */

                                                           // data calibrated ?
const int E7_CALIBRATION_UNITS_Uncalibrated=0;        /*!< data uncalibrated */
const int E7_CALIBRATION_UNITS_Calibrated  =1;          /*!< data calibrated */

                                                       // types of attenuations
const int E7_ATTENUATION_TYPE_AttenNone=0;               /*!< no attenuation */
const int E7_ATTENUATION_TYPE_AttenMeas=1;         /*!< measured attenuation */
const int E7_ATTENUATION_TYPE_AttenCalc=2;       /*!< calculated attenuation */

                                                            // types of filters
const int E7_FILTER_CODE_NoFilter   = 0;                      /*!< no filter */
const int E7_FILTER_CODE_Ramp       = 1;                    /*!< ramp filter */
const int E7_FILTER_CODE_Butterfield= 2;             /*!< butterfield filter */
const int E7_FILTER_CODE_Hann       = 3;                    /*!< hann filter */
const int E7_FILTER_CODE_Hamming    = 4;                 /*!< hamming filter */
const int E7_FILTER_CODE_Parzen     = 5;                  /*!< parzen filter */
const int E7_FILTER_CODE_Shepp      = 6;                   /*!< shepp filter */
const int E7_FILTER_CODE_Butterworth= 7;             /*!< butterworth filter */
const int E7_FILTER_CODE_Gaussian   = 8;                /*!< gaussian filter */
const int E7_FILTER_CODE_Median     = 9;                  /*!< median filter */
const int E7_FILTER_CODE_Boxcar     =10;                  /*!< boxcar filter */

                                                            // types of scatter
const int E7_SCATTER_TYPE_NoScatter           =0;            /*!< no scatter */
const int E7_SCATTER_TYPE_ScatterDeconvolution=1; /*!< scatter deconvolution */
const int E7_SCATTER_TYPE_ScatterSimulated    =2;     /*!< simulated scatter */
                                              /*! dual energy window scatter */
const int E7_SCATTER_TYPE_ScatterDualEnergy   =3;

                                                   // reconstruction algorithms
const int E7_RECON_TYPE_FBPJ         =  0;      /*!< filtered backprojection */
const int E7_RECON_TYPE_PROMIS       =  1;        /*!< promis reconstruction */
const int E7_RECON_TYPE_Ramp3D       =  2;       /*!< 3D ramp reconstruction */
const int E7_RECON_TYPE_FAVOR        =  3;         /*!< FAVOR reconstruction */
const int E7_RECON_TYPE_SSRB         =  4;       /*!< single slice rebinning */
const int E7_RECON_TYPE_Rebinning    =  5;                    /*!< rebinning */
const int E7_RECON_TYPE_FORE         =  6;                         /*!< FORE */
const int E7_RECON_TYPE_USER_MANIP   = 20;             /*!< user manipulated */

                                                               // preprocessing
                                                              /*! normalized */
const int E7_APPLIED_PROC_Normalized                       =    1;
                                         /*! measured attenuation correction */
const int E7_APPLIED_PROC_Measured_Attenuation_Correction  =    2;
                                       /*! calculated attenuation correction */
const int E7_APPLIED_PROC_Calculated_Attenuation_Correction=    4;
                                                /*! smoothing in x-direction */
const int E7_APPLIED_PROC_X_smoothing                      =    8;
                                                /*! smoothing in y-direction */
const int E7_APPLIED_PROC_Y_smoothing                      =   16;
                                                /*! smoothing in z-direction */
const int E7_APPLIED_PROC_Z_smoothing                      =   32;
                                                   /*! 2D scatter correction */
const int E7_APPLIED_PROC_2D_scatter_correction            =   64;
                                                   /*! 3D scatter correction */
const int E7_APPLIED_PROC_3D_scatter_correction            =  128;
                                                          /*! arc correction */
const int E7_APPLIED_PROC_Arc_correction                   =  256;
                                                        /*! decay correction */
const int E7_APPLIED_PROC_Decay_correction                 =  512;
                                                      /*! online compression */
const int E7_APPLIED_PROC_Online_compression               = 1024;
                                                       /*! fourier rebinning */
const int E7_APPLIED_PROC_FORE                             = 2048;
                                                  /*! single-slice rebinning */
const int E7_APPLIED_PROC_SSRB                             = 4096;
                                                               /*! segment 0 */
const int E7_APPLIED_PROC_SEG0                             = 8192;
                                                       /*! randoms smoothing */
const int E7_APPLIED_PROC_Randoms_Smoothing                =16384;

                                                                  // file types
const int E7_FILE_TYPE_unknown              = 0;      /*!< unknown file type */
const int E7_FILE_TYPE_Sinogram             = 1;            /*!< 2d sinogram */
const int E7_FILE_TYPE_Image16              = 2;  /*!< image (short integer) */
const int E7_FILE_TYPE_AttenuationCorrection= 3; /*!< attenuation correction */
const int E7_FILE_TYPE_Normalization        = 4;          /*!< normalization */
const int E7_FILE_TYPE_PolarMap             = 5;              /*!< polar map */
const int E7_FILE_TYPE_Volume8              = 6;         /*!< volume (8 bit) */
const int E7_FILE_TYPE_Volume16             = 7; /*!< volume (short integer) */
const int E7_FILE_TYPE_Projection8          = 8;     /*!< projection (8 bit) */
const int E7_FILE_TYPE_Projection16         = 9; /*!< projection (short int) */
const int E7_FILE_TYPE_Image8               =10;          /*!< image (8 bit) */
const int E7_FILE_TYPE_3D_Sinogram16        =11;/*!< 3d sinogram (short int) */
const int E7_FILE_TYPE_3D_Sinogram8         =12;    /*!< 3d sinogram (8 bit) */
const int E7_FILE_TYPE_3D_Normalization     =13;       /*!< 3d normalization */
const int E7_FILE_TYPE_3D_SinogramFloat     =14;    /*!< 3d sinogram (float) */
const int E7_FILE_TYPE_Interfile            =15;              /*!< interfile */

                                                 // type of transmission source
const int E7_TRANSM_SOURCE_TYPE_NoSrc=0;         /*!< no transmission source */
const int E7_TRANSM_SOURCE_TYPE_Ring =1;       /*!< ring transmission source */
const int E7_TRANSM_SOURCE_TYPE_Rod  =2;        /*!< rod transmission source */
const int E7_TRANSM_SOURCE_TYPE_Point=3;      /*!< point transmission source */

                                                   // coincidence sampling mode
const int E7_COIN_SAMP_MODE_NetTrues               =0;            /*!< trues */
                                                    /*!< prompts and delayed */
const int E7_COIN_SAMP_MODE_PromptsDelayed         =1;
                                         /*!< prompts, delayed and multiples */
const int E7_COIN_SAMP_MODE_PromptsDelayedMultiples=2;
                                              /*! trues with TOF information */
const int E7_COIN_SAMP_MODE_TruesTOF               =3;

                                                                 // mash factor
const int E7_ANGULAR_COMPRESSION_NoMash=0;                   /*!< no mashing */
const int E7_ANGULAR_COMPRESSION_Mash2 =1;                 /*!< mashing of 2 */
const int E7_ANGULAR_COMPRESSION_Mash4 =2;                 /*!< mashing of 4 */

                                                         // axial sampling mode
const int E7_AXIAL_SAMP_MODE_Normal=0;            /*!< normal axial sampling */
const int E7_AXIAL_SAMP_MODE_2X    =1;                /*!< 2x axial sampling */
const int E7_AXIAL_SAMP_MODE_3X    =2;                /*!< 3x axial sampling */

                                                           // calibration units
const int E7_CALIBRATION_UNITS_LABEL_BloodFlow=0;            /*!< blood flow */
const int E7_CALIBRATION_UNITS_LABEL_LMRGLU   =1;           /*!< lmr glucose */

                                                                 // compression
const int E7_COMPRESSION_CODE_CompNone=0;                /*!< no compression */

                                                           // gender of patient
const char E7_PATIENT_SEX_SexMale   ='M';                          /*!< male */
const char E7_PATIENT_SEX_SexFemale ='F';                        /*!< female */
const char E7_PATIENT_SEX_SexUnknown='U';                   /*!< unknown sex */

                                                        // dexterity of patient
const char E7_PATIENT_DEXTERITY_DextRT     ='R';           /*!< right handed */
const char E7_PATIENT_DEXTERITY_DextLF     ='L';            /*!< left handed */
const char E7_PATIENT_DEXTERITY_DextUnknown='U';      /*!< unknown dexterity */

                                                                   // scan type
                                                     /*! undefined scan type */
const int E7_ACQUISITION_TYPE_Undef                  =0;
const int E7_ACQUISITION_TYPE_Blank                  =1;     /*!< blank scan */
                                                       /*! transmission scan */
const int E7_ACQUISITION_TYPE_Transmission           =2;
                                                    /*! static emission scan */
const int E7_ACQUISITION_TYPE_StaticEmission         =3;
                                                   /*! dynamic emission scan */
const int E7_ACQUISITION_TYPE_DynamicEmission        =4;
                                                     /*! gated emission scan */
const int E7_ACQUISITION_TYPE_GatedEmission          =5;
                                         /*! transmission scan (rectilinear) */
const int E7_ACQUISITION_TYPE_TransmissionRectilinear=6;
                                             /*! emission scan (rectilinear) */
const int E7_ACQUISITION_TYPE_EmissionRectilinear    =7;

                                                            // acquisition mode
const int E7_ACQUISITION_MODE_Normal              =0;/*!< normal acquisition */
                                                    /*! windowed acquisition */
const int E7_ACQUISITION_MODE_Windowed            =1;
                                   /*! windowed and non-windowed acquisition */
const int E7_ACQUISITION_MODE_WindowedNonwindowed =2;
                                                 /*! dual energy acquisition */
const int E7_ACQUISITION_MODE_DualEnergy          =3;
                                         /*! upper energy window acquisition */
const int E7_ACQUISITION_MODE_UpperEnergy         =4;
                                  /*! simultaneous emission and transmission */
const int E7_ACQUISITION_MODE_EmissionTransmission=5;

                                                              // septa position
const int E7_SEPTA_STATE_Extended =0;                    /*!< extended septa */
const int E7_SEPTA_STATE_Retracted=1;                   /*!< retracted septa */
const int E7_SEPTA_STATE_NoSepta  =2;                          /*!< no septa */

                                                         // patient orientation
                                                        /*! feet first prone */
const int E7_PATIENT_ORIENTATION_Feet_First_Prone          =0;
                                                        /*! head first prone */
const int E7_PATIENT_ORIENTATION_Head_First_Prone          =1;
                                                       /*! feet first supine */
const int E7_PATIENT_ORIENTATION_Feet_First_Supine         =2;
                                                       /*! head first supine */
const int E7_PATIENT_ORIENTATION_Head_First_Supine         =3;
                                              /*! feet first decubitus right */
const int E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Right=4;
                                              /*! head first decubitus right */
const int E7_PATIENT_ORIENTATION_Head_First_Decubitus_Right=5;
                                               /*! feet first decubitus left */
const int E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Left =6;
                                               /*! head first decubitus left */
const int E7_PATIENT_ORIENTATION_Head_First_Decubitus_Left =7;
                                                     /*! unknown orientation */
const int E7_PATIENT_ORIENTATION_Unknown_Orientation       =8;

                                                        // quantification units
const int E7_QUANT_UNITS_Default=0;                       /*!< default units */
const int E7_QUANT_UNITS_Normalized=1;                       /*!< normalized */
const int E7_QUANT_UNITS_Mean=2;                                   /*!< mean */
const int E7_QUANT_UNITS_StdDevMean=3;               /*!< standard deviation */

#endif
