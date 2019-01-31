/*! \file exception.h
    \brief This class implements an exception object.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
 */

#pragma once

#include <string>
#include "types.h"
#include "semaphore_al.h"

/*- constants ---------------------------------------------------------------*/

// error ids
/*! file doesn't exist */
typedef unsigned long int const error_id;
error_id REC_FILE_DOESNT_EXIST = 40100;  // file can't be created
error_id REC_FILE_CREATE                           = 40101;  // can't set a value in Interfile main header
error_id REC_ITF_HEADER_ERROR_MAINHEADER_SET       = 40102;  // can't remove key from Interfile main header
error_id REC_ITF_HEADER_ERROR_MAINHEADER_REMOVE    = 40103;  // can't set a value in Interfile norm header
error_id REC_ITF_HEADER_ERROR_NORMHEADER_SET       = 40104;  // can't remove key from Interfile norm header
error_id REC_ITF_HEADER_ERROR_NORMHEADER_REMOVE    = 40105;  // can't set a value in Interfile subheader
error_id REC_ITF_HEADER_ERROR_SUBHEADER_SET        = 40106;  // can't remove key from Interfile subheader
error_id REC_ITF_HEADER_ERROR_SUBHEADER_REMOVE     = 40107;  // the format of the date value in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_DATE_VALUE           = 40108;  // the date is invalid in Interfile header
error_id REC_ITF_HEADER_ERROR_DATE_INVALID         = 40109;  // date im Interfile header is before 1752/09/14
error_id REC_ITF_HEADER_ERROR_GREGORIAN            = 40110;  // format of the time value in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_TIME_VALUE           = 40111;  // time in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_TIME_INVALID         = 40112;  // patient sex in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_SEX_INVALID          = 40113;  // patient orientation in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_ORIENTATION          = 40114;  // scan direction in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_DIRECTION            = 40115;  // PET datatype in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_PET_DATATYPE         = 40116;  // number format in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_NUMBER_FORMAT        = 40117;  // septa value in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_SEPTA_FORMAT         = 40118;  // detector motion value in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_DETMOT_FORMAT        = 40119;  // exam type in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_EXAMTYPE_FORMAT      = 40120;  // acquisition condition value in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_ACQCOND_FORMAT       = 40121;  // data type in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_DATATYPE             = 40122;  // endianess in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_ENDIAN               = 40123;  // key is not a valid Interfile key
error_id REC_ITF_HEADER_ERROR_KEY_INVALID          = 40124;  // can't change the ordering of key in Interfile header
error_id REC_ITF_HEADER_ERROR_KEY_ORDER            = 40125;  // key is missing in Interfile header
error_id REC_ITF_HEADER_ERROR_KEY_MISSING          = 40126;  // key is appearing multiple times in Interfile header
error_id REC_ITF_HEADER_ERROR_MULTIPLE_KEY         = 40127;  // separator between key and value is missing in Interfile header
error_id REC_ITF_HEADER_ERROR_SEPARATOR_MISSING    = 40128;  // format of the unit is invalid in Interfile header
error_id REC_ITF_HEADER_ERROR_UNITFORMAT           = 40129;  // unit of key is wrong in Interfile header
error_id REC_ITF_HEADER_ERROR_UNIT_WRONG           = 40130;  // array index is too large in Interfile header
error_id REC_ITF_HEADER_ERROR_INDEX_TOO_LARGE      = 40131;  // array index is too small in Interfile header
error_id REC_ITF_HEADER_ERROR_INDEX_TOO_SMALL      = 40132;  // value of key is invalid in Interfile header
error_id REC_ITF_HEADER_ERROR_VALUE                = 40133;  // value of key is missing in Interfile header
error_id REC_ITF_HEADER_ERROR_VALUE_MISSING        = 40134;  // a bracket is missing in the value of key in Interfile header
error_id REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING = 40135;  // value of key in Interfile header is too small
error_id REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL      = 40136;  // value of key in Interfile header is too large
error_id REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE      = 40137;  // format of the value in Interfile header is invalid
error_id REC_ITF_HEADER_ERROR_VALUE_FORMAT         = 40138;  // value in Interfile header is out of range
error_id REC_ITF_HEADER_ERROR_VALUE_RANGE          = 40139;  // number of values in key in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG      = 40140;  // type of the value in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_VALUE_TYPE           = 40141;  // can't remove key from Interfile header
error_id REC_ITF_HEADER_ERROR_REMOVE_KEY           = 40142;  // scanner type in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_SCANNERTYPE          = 40143;  // decay correction value in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_DECAYCORR            = 40144;  // slice orientation value in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_SL_ORIENTATION       = 40145;  // randoms correction value in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_RANDCORR             = 40146;  // processing status value in Interfile header is wrong
error_id REC_ITF_HEADER_ERROR_PROCSTAT             = 40147;  // unknown exception occured
error_id REC_UNKNOWN_EXCEPTION                     = 40148;  // file-I/O code doesn't support this data type
error_id REC_PAR_IO_DATATYPE                       = 40149;  // can't start a process
error_id REC_CANT_START_PROCESS                    = 40150;  // client doesn't connect to server
error_id REC_CLIENT_DOESNT_CONNECT                 = 40151;  // file size is too small
error_id REC_FILE_TOO_SMALL                        = 40152;  // buffer overrun
error_id REC_BUFFER_OVERRUN                        = 40153;  // VAX double format is not supported
error_id REC_VAX_DOUBLE                            = 40154;  // unknown ECAT7 matrix type
error_id REC_UNKNOWN_ECAT7_MATRIXTYPE              = 40155;  // ECAT7 matrix header is missing
error_id REC_ECAT7_MATRIXHEADER_MISSING            = 40156;  // file size is wrong
error_id REC_FILESIZE_WRONG                        = 40157;  // invalid ECAT7 matrix type
error_id REC_INVALID_ECAT7_MATRIXTYPE              = 40158;  // unknown ECAT7 data type
error_id REC_UNKNOWN_ECAT7_DATATYPE                = 40159;  // radix of the FFT is too large
error_id REC_FFT_RADIX_TOO_LARGE                   = 40160;  // dimension of the FFT is too small
error_id REC_FFT_DIM_TOO_SMALL                     = 40161;  // dimension of the FFT is not even
error_id REC_FFT_DIM_NOT_EVEN                      = 40162;  // error during FFT initialization
error_id REC_FFT_INIT                              = 40163;  // number of bins in the sinogram is too small for FORE
error_id REC_FORE_BINS_TOO_SMALL                   = 40164;  // number of angles in the sinogram is too small for FORE
error_id REC_FORE_ANGLES_TOO_SMALL                 = 40165;  // access outside of queue
error_id REC_QUEUE_OVERRUN                         = 40166;  // queue is full
error_id REC_QUEUE_FULL                            = 40167;  // priority is out of range
error_id REC_PRIORITY_RANGE                        = 40168;  // matrix number is out of range
error_id REC_MATRIX_NUMBER_RANGE                   = 40169;  // job id is unknown
error_id REC_JOB_ID                                = 40170;  // error during adding of matrices
error_id REC_MATRIX_ADD                            = 40171;  // error during subtraction of matrices
error_id REC_MATRIX_SUB                            = 40172;  // error during multiplication of matrices
error_id REC_MATRIX_MULT                           = 40173;  // wrong index in matrix operation
error_id REC_MATRIX_INDEX                          = 40174;  // matrix identity can't be calculated
error_id REC_MATRIX_IDENT                          = 40175;  // inverse of the matrix can't be calculated
error_id REC_MATRIX_INVERS                         = 40176;  // matrix contains a singularity
error_id REC_MATRIX_SINGULAR                       = 40177;  // stream is empty
error_id REC_STREAM_EMPTY                          = 40178;  // can't initialize socket
error_id REC_SOCKET_ERROR_INIT                     = 40179;  // can't create server side of socket
error_id REC_SOCKET_ERROR_CREATE_S                 = 40180;  // can't create client side of socket
error_id REC_SOCKET_ERROR_CREATE_C                 = 40181;  // can't establish socket connection
error_id REC_SOCKET_ERROR_CONNECT                  = 40182;  // can't change configuration of socket
error_id REC_SOCKET_ERROR_CONF                     = 40183;  // can't bind socket
error_id REC_SOCKET_ERROR_BIND                     = 40184;  // can't get port number of socket
error_id REC_SOCKET_ERROR_GETPORT                  = 40185;  // can't listen for incoming socket connections
error_id REC_SOCKET_ERROR_LISTEN                   = 40186;  // can't accept incoming socket connection
error_id REC_SOCKET_ERROR_ACCEPT                   = 40187;  // socket connection is brocken
error_id REC_SOCKET_ERROR_BROKEN                   = 40188;  // crystal material is unknown
error_id REC_UNKNOWN_CRYSTAL_MATERIAL              = 40189;  // image size is too large
error_id REC_IMAGESIZE_TOO_LARGE                   = 40190;  // too many crystal layers
error_id REC_TOO_MANY_CRYSTAL_LAYERS               = 40191;  // no voxel in u-map is above the threshold
error_id REC_UMAP_ERROR_THRESHOLD                  = 40192;  // error during b-spline interpolation
error_id REC_BSPLINE_ERROR                         = 40193;  // bin size is too large
error_id REC_BINSIZE_TOO_LARGE                     = 40194;  // queue received unknown command from the DCOM server
error_id REC_UNKNOWN_DCOM_COMMAND                  = 40195;  // reconstruction server received unknown command from queue
error_id REC_UNKNOWN_QUEUE_COMMAND                 = 40196;  // queue received unknown command from the reconstruction server
error_id REC_UNKNOWN_RS_COMMAND                    = 40197;  // configuration of the reconstruction software is wrong
error_id REC_CONFIGURATION_ERROR                   = 40198;  // can't terminate queue and reconstruction server
error_id REC_TERMINATE_ERROR                       = 40199;  // matrix number in the ECAT7 file is wrong
error_id REC_ECAT7_MATRIX_NUMBER_WRONG             = 40200;  // norm file is invalid
error_id REC_NORM_FILE_INVALID                     = 40201;  // gantry model number is wrong
error_id REC_GANTRY_MODEL                          = 40202;  // frame number is out of range
error_id REC_OOR_FRAME                             = 40203;  // gate number is out of range
error_id REC_OOR_GATE                              = 40204;  // bed number is out of range
error_id REC_OOR_BED                               = 40205;  // energy window number is out of range
error_id REC_OOR_ENERGY                            = 40206;  // TOF window is out of range
error_id REC_OOR_TOF                               = 40207;  // scan type is out of range
error_id REC_OOR_SCAN_TYPE                         = 40208;  // can't read from file
error_id REC_FILE_READ                             = 40209;  // can't write to file
error_id REC_FILE_WRITE                            = 40210;  //the queue is not connected
error_id REC_QUEUE_NOT_CONNECTED                   = 40211;  // queue is connected
error_id REC_QUEUE_CONNECTED                       = 40212;  // can't access registry key
error_id REC_REGKEY_ACCESS                         = 40213;  // type of the registry key is wrong
error_id REC_REGKEY_TYPE                           = 40214;  // can't access registry subkey
error_id REC_REGSUBKEY_ACCESS                      = 40215;  // can't write registry subkey
error_id REC_REGSUBKEY_WRITE                       = 40216;  // master can't terminate OSEM slave
error_id REC_TERMINATE_SLAVE_ERROR                 = 40217;  // remote execution daemon received unknown command
error_id REC_UNKNOWN_RED_COMMAND                   = 40218;  // processing has been canceled by Syngo
error_id REC_SYNGO_CANCEL                          = 40219;  // offset matrix for OP-OSEM is not initialized
error_id REC_OFFSET_MATRIX                         = 40220;  // no memory block is available
error_id REC_MEMORY_BLOCK                          = 40221;  // path for the swap files is invalid
error_id REC_SWAP_PATH_INVALID                     = 40222;  // home directory is unknown
error_id REC_HOME_UNKNOWN                          = 40223;  // bed was not found
error_id REC_BED_NOT_FOUND                         = 40224;  // IDL library is missing
error_id REC_IDL_LIB                               = 40225;  // symbol can't be found in the IDL library
error_id REC_IDL_SYMBOL                            = 40226;  // can't initialize IDL
error_id REC_IDL_INIT                              = 40227;  // can't run IDL code
error_id REC_IDL_RUN                               = 40228;  // can't execute IDL command
error_id REC_IDL_EXECUTE                           = 40229;  // error in IDL scaling code
error_id REC_IDL_SCALING                           = 40230;  // reconstruction server can't connect to reconstruction modul
error_id REC_E7CONNECT                             = 40231;  // gantry models don't match
error_id REC_GANTRY_MODEL_MATCH                    = 40232;  // gantry model value is unknown
error_id REC_GANTRY_MODEL_UNKNOWN_VALUE            = 40233;  // can't initialize the stopwatch
error_id REC_STOPWATCH_INIT                        = 40234;  // stopwatch is still running
error_id REC_STOPWATCH_RUN                         = 40235;  // can't get CPSDataAccess object
error_id REC_DB_GETDATAACCESS                      = 40236;  // can't initialize CPSDataAccess object
error_id REC_DB_INITDATAACCESS                     = 40237;  // can't get patient from DB
error_id REC_DB_GET_DBPATIENT                      = 40238;  // can't initialize patient interface from DB
error_id REC_DB_DBPATIENT_INTERF                   = 40239;  // can't get study from DB
error_id REC_DB_GET_DBSTUDY                        = 40240;  // can't initialze study interface from DB
error_id REC_DB_DBSTUDY_INTERF                     = 40241;  // can't get series from DB
error_id REC_DB_GET_DBSERIES                       = 40242;  // can't initialize series interface from DB
error_id REC_DB_DBSERIES_INTERF                    = 40243;  // can't get image from DB
error_id REC_DB_GET_DBIMAGE                        = 40244;  // can't initialize image interface from DB
error_id REC_DB_DBIMAGE_INTERF                     = 40245;  // can't get equipment from DB
error_id REC_DB_GET_DBEQUIPMENT                    = 40246;  // can't initialize equipment interface from DB
error_id REC_DB_DBEQUIPMENT_INTERF                 = 40247;  // can't get frame-of-reference from DB
error_id REC_DB_GET_DBFR                           = 40248;  // can't initialize frame-of-reference interface from DB
error_id REC_DB_DBFOR_INTERF                       = 40249;  // can't get PET object from DB
error_id REC_DB_GET_DBPETOBJECT                    = 40250;  // can't initialize PET object interface from DB
error_id REC_DB_DBPETOBJECT_INTERF                 = 40251;  // can't initialize image pixel interface from DB
error_id REC_DB_IMAGEPIXEL_INTERF                  = 40252;  // can't initialize image plane interface from DB
error_id REC_DB_IMAGEPLANE_INTERF                  = 40253;  // can't initialize SOP common interface from DB
error_id REC_DB_SOPCOMMON_INTERF                   = 40254;  // can't initialize PET image interface from DB
error_id REC_DB_PETIMAGE_INTERF                    = 40255;  // can't initialize PET series interface from DB
error_id REC_DB_PETSERIES_INTERF                   = 40256;  // can't initialize PET isotope interface from DB
error_id REC_DB_PETISOTOPE_INTERF                  = 40257;  // can't initialize patient study interface from DB
error_id REC_DB_PATIENTSTUDY_INTERF                = 40258;  // can't initialize patient orientation interface from DB
error_id REC_DB_NMPETPATIENTORIENTATION_INTERF     = 40259;  // can't initialize CT image interface from DB
error_id REC_DB_CTIMAGE_INTERF                     = 40260;  // unknown patient GUID
error_id REC_DB_UNKNOWN_PATGUID                    = 40261;  // unknown study GUID
error_id REC_DB_UNKNOWN_STUDYGUID                  = 40262;  // unknown series GUID
error_id REC_DB_UNKNOWN_SERIESGUID                 = 40263;  // unknown patient ID
error_id REC_DB_UNKNOWN_PATID                      = 40264;  // unknown patient name
error_id REC_DB_UNKNOWN_PATNAME                    = 40265;  // unknown frame of reference
error_id REC_DB_UNKNOWN_FOR                        = 40266;  // unknown equipment
error_id REC_DB_UNKNOWN_EQUIPMENT                  = 40267;  // can't retrieve series from DB
error_id REC_DB_RETRIEVE_SERIES                    = 40268;  // can't retrieve images from DB
error_id REC_DB_RETRIEVE_IMAGES                    = 40269;  // can't retrieve patients from DB
error_id REC_DB_RETRIEVE_PATIENTS                  = 40270;  // can't retrieve studies from DB
error_id REC_DB_RETRIEVE_STUDIES                   = 40271;  // can't retrieve energy window from DB
error_id REC_DB_RETRIEVE_ENERGYWINDOW              = 40272;  // can't retrieve correction flags from DB
error_id REC_DB_RETRIEVE_CORRECTIONFLAGS           = 40273;  // can't retrieve radiopharmaceutical information from DB
error_id REC_DB_RETRIEVE_RADIOINFO                 = 40274;  // can't retrieve image position from DB
error_id REC_DB_RETRIEVE_IMAGEPOSITION             = 40275;  // can't retrieve CARS interface structure from DB
error_id REC_DB_CARS_INTERF                        = 40276;  // can't create equipment in DB
error_id REC_DB_MAKE_EQUIPMENT                     = 40277;  // can't create frame-of-reference in DB
error_id REC_DB_MAKE_FOR                           = 40278;  // can't create image in DB
error_id REC_DB_MAKE_IMAGE                         = 40279;  // can't make patient in DB
error_id REC_DB_MAKE_PATIENT                       = 40280;  // can't create a series in DB
error_id REC_DB_MAKE_SERIES                        = 40281;  // can't create a study in DB
error_id REC_DB_MAKE_STUDY                         = 40282;  // series doesn't contain any image slices
error_id REC_DB_NOIMAGESLICES                      = 40283;  // image modality is not supported
error_id REC_DB_UNSUPPORTED_MODALITY               = 40284;  // out of memory
error_id REC_OUT_OF_MEMORY                         = 40285;  // norm is invalid
error_id REC_INVALID_NORM                          = 40286;  // span of the sinogram is 0
error_id REC_SPAN_RD                               = 40287;  // coincidence sampling mode is unknown
error_id REC_COINSAMPMODE                          = 40288;  // zoom is negative
error_id REC_NEG_ZOOM                              = 40289;  // CRS can't connect to the queue
error_id REC_CRS_QUEUE_CONNECT                     = 40290;  // CRS can't disconnect from the queue
error_id REC_CRS_QUEUE_DISCONNECT                  = 40291;  // violation of nonnegativity constraint in OSEM
error_id REC_NONNEG_CONSTRAINT                     = 40292;  // voxel values in MAP-TR are running out-of-range
error_id REC_UMAP_RECON_OUT_OF_RANGE                        = 40293;
error_id REC_GETHOSTBYNAME_FAILS                            = 40294;  // gethostbyname() fails


/*! information ID for event handle */
const unsigned long int REC_INFO                              = 60000;

/*- class definitions -------------------------------------------------------*/

class Exception {
private :
  std::string err_str;      /*!< description of error */
  unsigned long int err_code;       /*!< error code */
public:
  // store error information in object
  Exception(const unsigned long int, const std::string);
  template <typename T >
  Exception arg(T);  // f ill argument into error string
  unsigned long int errCode() const;  // request error code
  std::string errStr() const;  // request error description
};


