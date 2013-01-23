/*! \file exception.h
    \brief This class implements an exception object.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
 */

#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <string>
#include "types.h"
#include "semaphore_al.h"
#ifdef WIN32
#include "ArsEventMsgDefs.h"
#endif

/*- constants ---------------------------------------------------------------*/

#if !defined(WIN32) || defined(ERRCODES)
                                                                   // error ids
                                                      /*! file doesn't exist */
const unsigned long int REC_FILE_DOESNT_EXIST                    =40100,
                                                   /*! file can't be created */
                        REC_FILE_CREATE                          =40101,
                          /*! can't set a value in the Interfile main header */
                        REC_ITF_HEADER_ERROR_MAINHEADER_SET      =40102,
                             /*! can't remove key from Interfile main header */
                        REC_ITF_HEADER_ERROR_MAINHEADER_REMOVE   =40103,
                          /*! can't set a value in the Interfile norm header */
                        REC_ITF_HEADER_ERROR_NORMHEADER_SET      =40104,
                             /*! can't remove key from Interfile norm header */
                        REC_ITF_HEADER_ERROR_NORMHEADER_REMOVE   =40105,
                            /*! can't set a value in the Interfile subheader */
                        REC_ITF_HEADER_ERROR_SUBHEADER_SET       =40106,
                               /*! can't remove key from Interfile subheader */
                        REC_ITF_HEADER_ERROR_SUBHEADER_REMOVE    =40107,
           /*! the format of the date value in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_DATE_VALUE          =40108,
                             /*! the date is invalid in the Interfile header */
                        REC_ITF_HEADER_ERROR_DATE_INVALID        =40109,
                   /*! the date im the Interfile header is before 1752/09/14 */
                        REC_ITF_HEADER_ERROR_GREGORIAN           =40110,
           /*! the format of the time value in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_TIME_VALUE          =40111,
                             /*! the time in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_TIME_INVALID        =40112,
                      /*! the patient sex in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_SEX_INVALID         =40113,
              /*! the patient orientation in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_ORIENTATION         =40114,
                   /*! the scan direction in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_DIRECTION           =40115,
                     /*! the PET datatype in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_PET_DATATYPE        =40116,
                    /*! the number format in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_NUMBER_FORMAT       =40117,
                      /*! the septa value in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_SEPTA_FORMAT        =40118,
            /*! the detector motion value in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_DETMOT_FORMAT       =40119,
                        /*! the exam type in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_EXAMTYPE_FORMAT     =40120,
      /*! the acquisition condition value in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_ACQCOND_FORMAT      =40121,
                        /*! the data type in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_DATATYPE            =40122,
                        /*! the endianess in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_ENDIAN              =40123,
                                    /*! the key is not a valid Interfile key */
                        REC_ITF_HEADER_ERROR_KEY_INVALID         =40124,
            /*! can't change the ordering of the key in the Interfile header */
                        REC_ITF_HEADER_ERROR_KEY_ORDER           =40125,
                              /*! the key is missing in the Interfile header */
                        REC_ITF_HEADER_ERROR_KEY_MISSING         =40126,
             /*! the key is appearing multiple times in the Interfile header */
                        REC_ITF_HEADER_ERROR_MULTIPLE_KEY        =40127,
  /*! the separator between key and value is missing in the Interfile header */
                        REC_ITF_HEADER_ERROR_SEPARATOR_MISSING   =40128,
               /*! the format of the unit is invalid in the Interfile header */
                        REC_ITF_HEADER_ERROR_UNITFORMAT          =40129,
                    /*! the unit of the key is wrong in the Interfile header */
                        REC_ITF_HEADER_ERROR_UNIT_WRONG          =40130,
                    /*! the array index is too large in the Interfile header */
                        REC_ITF_HEADER_ERROR_INDEX_TOO_LARGE     =40131,
                     /* the array index is too small in the Interfile header */
                        REC_ITF_HEADER_ERROR_INDEX_TOO_SMALL     =40132,
                 /*! the value of the key is invalid in the Interfile header */
                        REC_ITF_HEADER_ERROR_VALUE               =40133,
                 /*! the value of the key is missing in the Interfile header */
                        REC_ITF_HEADER_ERROR_VALUE_MISSING       =40134,
    /*! a bracket is missing in the value of the key in the Interfile header */
                        REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING=40135,
               /*! the value of the key in the Interfile header is too small */
                        REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL     =40136,
               /*! the value of the key in the Interfile header is too large */
                        REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE     =40137,
              /*! the format of the value in the Interfile header is invalid */
                        REC_ITF_HEADER_ERROR_VALUE_FORMAT        =40138,
                       /*! the value in the Interfile header is out of range */
                        REC_ITF_HEADER_ERROR_VALUE_RANGE         =40139,
        /*! the number of values in the key in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG     =40140,
                  /*! the type of the value in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_VALUE_TYPE          =40141,
                          /*! can't remove the key from the Interfile header */
                        REC_ITF_HEADER_ERROR_REMOVE_KEY          =40142,
                       /*! the scanner type in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_SCANNERTYPE         =40143,
             /*! the decay correction value in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_DECAYCORR           =40144,
            /*! the slice orientation value in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_SL_ORIENTATION      =40145,
           /*! the randoms correction value in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_RANDCORR            =40146,
            /*! the processing status value in the Interfile header is wrong */
                        REC_ITF_HEADER_ERROR_PROCSTAT            =40147,
                                               /*! unknown exception occured */
                        REC_UNKNOWN_EXCEPTION                    =40148,
                        /*! the file-I/O code doesn't support this data type */
                        REC_PAR_IO_DATATYPE                      =40149,
                                                   /*! can't start a process */
                        REC_CANT_START_PROCESS                =40150,
                                        /*! client doesn't connect to server */
                        REC_CLIENT_DOESNT_CONNECT             =40151,
                                              /*! the file size is too small */
                        REC_FILE_TOO_SMALL                    =40152,
                                                          /*! buffer overrun */
                        REC_BUFFER_OVERRUN                    =40153,
                                      /*! VAX double format is not supported */
                        REC_VAX_DOUBLE                        =40154,
                                               /*! unknown ECAT7 matrix type */
                        REC_UNKNOWN_ECAT7_MATRIXTYPE          =40155,
                                          /*! ECAT7 matrix header is missing */
                        REC_ECAT7_MATRIXHEADER_MISSING        =40156,
                                                  /*! the file size is wrong */
                        REC_FILESIZE_WRONG                    =40157,
                                               /*! invalid ECAT7 matrix type */
                        REC_INVALID_ECAT7_MATRIXTYPE          =40158,
                                                 /*! unknown ECAT7 data type */
                        REC_UNKNOWN_ECAT7_DATATYPE            =40159,
                                       /*! the radix of the FFT is too large */
                        REC_FFT_RADIX_TOO_LARGE               =40160,
                                   /*! the dimension of the FFT is too small */
                        REC_FFT_DIM_TOO_SMALL                 =40161,
                                    /*! the dimension of the FFT is not even */
                        REC_FFT_DIM_NOT_EVEN                  =40162,
                                         /*! error during FFT initialization */
                        REC_FFT_INIT                          =40163,
                /*! the number of bins in the sinogram is too small for FORE */
                        REC_FORE_BINS_TOO_SMALL               =40164,
              /*! the number of angles in the sinogram is too small for FORE */
                        REC_FORE_ANGLES_TOO_SMALL             =40165,
                                                 /*! access outside of queue */
                        REC_QUEUE_OVERRUN                     =40166,
                                                       /*! the queue is full */
                        REC_QUEUE_FULL                        =40167,
                                            /*! the priority is out of range */
                        REC_PRIORITY_RANGE                    =40168,
                                       /*! the matrix number is out of range */
                        REC_MATRIX_NUMBER_RANGE               =40169,
                                                   /*! the job id is unknown */
                        REC_JOB_ID                            =40170,
                                         /*! error during adding of matrices */
                        REC_MATRIX_ADD                        =40171,
                                    /*! error during subtraction of matrices */
                        REC_MATRIX_SUB                        =40172,
                                 /*! error during multiplication of matrices */
                        REC_MATRIX_MULT                       =40173,
                                         /*! wrong index in matrix operation */
                        REC_MATRIX_INDEX                      =40174,
                                     /*! matrix identity can't be calculated */
                        REC_MATRIX_IDENT                      =40175,
                           /*! the inverse of the matrix can't be calculated */
                        REC_MATRIX_INVERS                     =40176,
                                       /*! the matrix contains a singularity */
                        REC_MATRIX_SINGULAR                   =40177,
                                                     /*! the stream is empty */
                        REC_STREAM_EMPTY                      =40178,
                                                 /*! can't initialize socket */
                        REC_SOCKET_ERROR_INIT                 =40179,
                                      /*! can't create server side of socket */
                        REC_SOCKET_ERROR_CREATE_S             =40180,
                                      /*! can't create client side of socket */
                        REC_SOCKET_ERROR_CREATE_C             =40181,
                                       /*! can't establish socket connection */
                        REC_SOCKET_ERROR_CONNECT              =40182,
                                    /*! can't change configuration of socket */
                        REC_SOCKET_ERROR_CONF                 =40183,
                                                       /*! can't bind socket */
                        REC_SOCKET_ERROR_BIND                 =40184,
                                         /*! can't get port number of socket */
                        REC_SOCKET_ERROR_GETPORT              =40185,
                            /*! can't listen for incoming socket connections */
                        REC_SOCKET_ERROR_LISTEN               =40186,
                                 /*! can't accept incoming socket connection */
                        REC_SOCKET_ERROR_ACCEPT               =40187,
                                        /*! the socket connection is brocken */
                        REC_SOCKET_ERROR_BROKEN               =40188,
                                         /*! the crystal material is unknown */
                        REC_UNKNOWN_CRYSTAL_MATERIAL          =40189,
                                             /*! the image size is too large */
                        REC_IMAGESIZE_TOO_LARGE               =40190,
                                                 /*! too many crystal layers */
                        REC_TOO_MANY_CRYSTAL_LAYERS           =40191,
                                /*! no voxel in u-map is above the threshold */
                        REC_UMAP_ERROR_THRESHOLD              =40192,
                                     /*! error during b-spline interpolation */
                        REC_BSPLINE_ERROR                     =40193,
                                               /*! the bin size is too large */
                        REC_BINSIZE_TOO_LARGE                 =40194,
                 /*! the queue received unknown command from the DCOM server */
                        REC_UNKNOWN_DCOM_COMMAND              =40195,
           /*! the reconstruction server received unknown command from queue */
                        REC_UNKNOWN_QUEUE_COMMAND             =40196,
       /*! the queue received unknown command from the reconstruction server */
                        REC_UNKNOWN_RS_COMMAND                =40197,
               /*! the configuration of the reconstruction software is wrong */
                        REC_CONFIGURATION_ERROR               =40198,
                         /*! can't terminate queue and reconstruction server */
                        REC_TERMINATE_ERROR                   =40199,
                            /*! the matrix number in the ECAT7 file is wrong */
                        REC_ECAT7_MATRIX_NUMBER_WRONG         =40200,
                                                /*! the norm file is invalid */
                        REC_NORM_FILE_INVALID                 =40201,
                                        /*! the gantry model number is wrong */
                        REC_GANTRY_MODEL                      =40202,
                                        /*! the frame number is out of range */
                        REC_OOR_FRAME                         =40203,
                                         /*! the gate number is out of range */
                        REC_OOR_GATE                          =40204,
                                          /*! the bed number is out of range */
                        REC_OOR_BED                           =40205,
                                /*! the energy window number is out of range */
                        REC_OOR_ENERGY                        =40206,
                                          /*! the TOF window is out of range */
                        REC_OOR_TOF                           =40207,
                                           /*! the scan type is out of range */
                        REC_OOR_SCAN_TYPE                     =40208,
                                                    /*! can't read from file */
                        REC_FILE_READ                         =40209,
                                                     /*! can't write to file */
                        REC_FILE_WRITE                        =40210,
                                               /*!the queue is not connected */
                        REC_QUEUE_NOT_CONNECTED               =40211,
                                                  /*! the queue is connected */
                        REC_QUEUE_CONNECTED                   =40212,
                                               /*! can't access registry key */
                        REC_REGKEY_ACCESS                     =40213,
                                   /*! the type of the registry key is wrong */
                        REC_REGKEY_TYPE                       =40214,
                                            /*! can't access registry subkey */
                        REC_REGSUBKEY_ACCESS                  =40215,
                                             /*! can't write registry subkey */
                        REC_REGSUBKEY_WRITE                   =40216,
                                       /*! master can't terminate OSEM slave */
                        REC_TERMINATE_SLAVE_ERROR             =40217,
                        /*! remote execution daemon received unknown command */
                        REC_UNKNOWN_RED_COMMAND               =40218,
                               /*! the processing has been canceled by Syngo */
                        REC_SYNGO_CANCEL                      =40219,
                        /*! the offset matrix for OP-OSEM is not initialized */
                        REC_OFFSET_MATRIX                     =40220,
                                            /*! no memory block is available */
                        REC_MEMORY_BLOCK                      =40221,
                                  /*! the path for the swap files is invalid */
                        REC_SWAP_PATH_INVALID                 =40222,
                                           /*! the home directory is unknown */
                        REC_HOME_UNKNOWN                      =40223,
                                                   /*! the bed was not found */
                        REC_BED_NOT_FOUND                     =40224,
                                              /*! the IDL library is missing */
                        REC_IDL_LIB                           =40225,
                            /*! the symbol can't be found in the IDL library */
                        REC_IDL_SYMBOL                        =40226,
                                                    /*! can't initialize IDL */
                        REC_IDL_INIT                          =40227,
                                                      /*! can't run IDL code */
                        REC_IDL_RUN                           =40228,
                                               /*! can't execute IDL command */
                        REC_IDL_EXECUTE                       =40229,
                                               /*! error in IDL scaling code */
                        REC_IDL_SCALING                       =40230,
             /*! reconstruction server can't connect to reconstruction modul */
                        REC_E7CONNECT                         =40231,
                                           /*! the gantry models don't match */
                        REC_GANTRY_MODEL_MATCH                =40232,
                                           /*! gantry model value is unknown */
                        REC_GANTRY_MODEL_UNKNOWN_VALUE        =40233,
                                          /*! can't initialize the stopwatch */
                        REC_STOPWATCH_INIT                    =40234,
                                          /*! the stopwatch is still running */
                        REC_STOPWATCH_RUN                     =40235,
                                          /*! can't get CPSDataAccess object */
                        REC_DB_GETDATAACCESS                  =40236,
                                   /*! can't initialize CPSDataAccess object */
                        REC_DB_INITDATAACCESS                 =40237,
                                               /*! can't get patient from DB */
                        REC_DB_GET_DBPATIENT                  =40238,
                              /*! can't initialize patient interface from DB */
                        REC_DB_DBPATIENT_INTERF               =40239,
                                                 /*! can't get study from DB */
                        REC_DB_GET_DBSTUDY                    =40240,
                                 /*! can't initialze study interface from DB */
                        REC_DB_DBSTUDY_INTERF                 =40241,
                                                /*! can't get series from DB */
                        REC_DB_GET_DBSERIES                   =40242,
                               /*! can't initialize series interface from DB */
                        REC_DB_DBSERIES_INTERF                =40243,
                                                 /*! can't get image from DB */
                        REC_DB_GET_DBIMAGE                    =40244,
                                /*! can't initialize image interface from DB */
                        REC_DB_DBIMAGE_INTERF                 =40245,
                                             /*! can't get equipment from DB */
                        REC_DB_GET_DBEQUIPMENT                =40246,
                            /*! can't initialize equipment interface from DB */
                        REC_DB_DBEQUIPMENT_INTERF             =40247,
                                    /*! can't get frame-of-reference from DB */
                        REC_DB_GET_DBFR                       =40248,
                   /*! can't initialize frame-of-reference interface from DB */
                        REC_DB_DBFOR_INTERF                   =40249,
                                            /*! can't get PET object from DB */
                        REC_DB_GET_DBPETOBJECT                =40250,
                           /*! can't initialize PET object interface from DB */
                        REC_DB_DBPETOBJECT_INTERF             =40251,
                          /*! can't initialize image pixel interface from DB */
                        REC_DB_IMAGEPIXEL_INTERF              =40252,
                          /*! can't initialize image plane interface from DB */
                        REC_DB_IMAGEPLANE_INTERF              =40253,
                           /*! can't initialize SOP common interface from DB */
                        REC_DB_SOPCOMMON_INTERF               =40254,
                            /*! can't initialize PET image interface from DB */
                        REC_DB_PETIMAGE_INTERF                =40255,
                           /*! can't initialize PET series interface from DB */
                        REC_DB_PETSERIES_INTERF               =40256,
                          /*! can't initialize PET isotope interface from DB */
                        REC_DB_PETISOTOPE_INTERF              =40257,
                        /*! can't initialize patient study interface from DB */
                        REC_DB_PATIENTSTUDY_INTERF            =40258,
                  /*! can't initialize patient orientation interface from DB */
                        REC_DB_NMPETPATIENTORIENTATION_INTERF =40259,
                             /*! can't initialize CT image interface from DB */
                        REC_DB_CTIMAGE_INTERF                 =40260,
                                                    /*! unknown patient GUID */
                        REC_DB_UNKNOWN_PATGUID                =40261,
                                                      /*! unknown study GUID */
                        REC_DB_UNKNOWN_STUDYGUID              =40262,
                                                     /*! unknown series GUID */
                        REC_DB_UNKNOWN_SERIESGUID             =40263,
                                                      /*! unknown patient ID */
                        REC_DB_UNKNOWN_PATID                  =40264,
                                                    /*! unknown patient name */
                        REC_DB_UNKNOWN_PATNAME                =40265,
                                              /*! unknown frame of reference */
                        REC_DB_UNKNOWN_FOR                    =40266,
                                                       /*! unknown equipment */
                        REC_DB_UNKNOWN_EQUIPMENT              =40267,
                                           /*! can't retrieve series from DB */
                        REC_DB_RETRIEVE_SERIES                =40268,
                                           /*! can't retrieve images from DB */
                        REC_DB_RETRIEVE_IMAGES                =40269,
                                         /*! can't retrieve patients from DB */
                        REC_DB_RETRIEVE_PATIENTS              =40270,
                                          /*! can't retrieve studies from DB */
                        REC_DB_RETRIEVE_STUDIES               =40271,
                                    /*! can't retrieve energy window from DB */
                        REC_DB_RETRIEVE_ENERGYWINDOW          =40272,
                                 /*! can't retrieve correction flags from DB */
                        REC_DB_RETRIEVE_CORRECTIONFLAGS       =40273,
                  /*! can't retrieve radiopharmaceutical information from DB */
                        REC_DB_RETRIEVE_RADIOINFO             =40274,
                                   /*! can't retrieve image position from DB */
                        REC_DB_RETRIEVE_IMAGEPOSITION         =40275,
                         /*! can't retrieve CARS interface structure from DB */
                        REC_DB_CARS_INTERF                    =40276,
                                            /*! can't create equipment in DB */
                        REC_DB_MAKE_EQUIPMENT                 =40277,
                                    /* can't create frame-of-reference in DB */
                        REC_DB_MAKE_FOR                       =40278,
                                                /*! can't create image in DB */
                        REC_DB_MAKE_IMAGE                     =40279,
                                                /*! can't make patient in DB */
                        REC_DB_MAKE_PATIENT                   =40280,
                                             /*! can't create a series in DB */
                        REC_DB_MAKE_SERIES                    =40281,
                                              /*! can't create a study in DB */
                        REC_DB_MAKE_STUDY                     =40282,
                             /*! the series doesn't contain any image slices */
                        REC_DB_NOIMAGESLICES                  =40283,
                                     /*! the image modality is not supported */
                        REC_DB_UNSUPPORTED_MODALITY           =40284,
                                                           /*! out of memory */
                        REC_OUT_OF_MEMORY                     =40285,
                                                     /*! the norm is invalid */
                        REC_INVALID_NORM                      =40286,
                                           /*! the span of the sinogram is 0 */
                        REC_SPAN_RD                           =40287,
                                    /*! coincidence sampling mode is unknown */
                        REC_COINSAMPMODE                      =40288,
                                                    /*! the zoom is negative */
                        REC_NEG_ZOOM                          =40289,
                                      /*! the CRS can't connect to the queue */
                        REC_CRS_QUEUE_CONNECT                 =40290,
                                 /*! the CRS can't disconnect from the queue */
                        REC_CRS_QUEUE_DISCONNECT              =40291,
                           /*! violation of nonnegativity constraint in OSEM */
                        REC_NONNEG_CONSTRAINT                 =40292,
                         /*! voxel values in MAP-TR are running out-of-range */
                        REC_UMAP_RECON_OUT_OF_RANGE=40293,
                         /*! gethostbyname() fails */
                        REC_GETHOSTBYNAME_FAILS = 40294;
#endif
#ifndef WIN32
                                         /*! information ID for event handle */
const unsigned long int REC_INFO                              =60000;
#endif

/*- class definitions -------------------------------------------------------*/

class Exception
 { private:
    std::string err_str;                           /*!< description of error */
    unsigned long int err_code;                              /*!< error code */
   public:
                                           // store error information in object
    Exception(const unsigned long int, const std::string);
    template <typename T>
     Exception arg(T);                       // fill argument into error string
#ifdef WIN32
    Exception arg(GUID);                         // fill GUID into error string
#endif
    unsigned long int errCode() const;                    // request error code
    std::string errStr() const;                    // request error description
 };

#ifndef _EXCEPTION_CPP
#define _EXCEPTION_TMPL_CPP
#include "exception.cpp"
#endif

#endif
