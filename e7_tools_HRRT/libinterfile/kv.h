/*! \file kv.h
    \brief This class is used to handle a key/value pair from an Interfile
           file.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments
    \date 2004/12/14 added method getUnit()
    \date 2005/01/15 cleanup templates
    \date 2005/02/07 support for int64 values and arrays
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)
 */

#ifndef _KV_H
#define _KV_H

#include <ostream>
#include <string>
#include <vector>
#include "timedate.h"
#include "types.h"
#include <typeinfo>

/*- class definitions -------------------------------------------------------*/

class KV
 { public:
                                                           /*! type of value */
    typedef enum { NONE,                                       /*!< no value */
                   ASCII,                                        /*!< string */
                   ASCII_A,                            /*!< array of strings */
                   ASCII_L,                             /*!< list of strings */
                   ASCII_AL,                  /*!< array of lists of strings */
                   USHRT,                            /*!< unsigned short int */
                   USHRT_L,                 /*!< list of unsigned short ints */
                   USHRT_A,                /*!< array of unsigned short ints */
                   USHRT_AL,      /*!< array of lists of unsigned short ints */
                   ULONG,                             /*!< unsigned long int */
                   LONG,                                /*!< signed long int */
                   ULONG_A,                 /*!< array of unsigned long ints */
                   ULONG_L,                  /*!< list of unsigned long ints */
                   LONGLONG,                       /*!< signed long long int */
                   LONGLONG_A,           /*!< array of signed long long ints */
                   ULONGLONG,                    /*!< unsigned long long int */
                   ULONGLONG_A,        /*!< array of unsigned long long ints */
                   UFLOAT,                               /*!< unsigned float */
                   FLOAT,                                  /*!< signed float */
                   UFLOAT_A,                   /*!< array of unsigned floats */
                   UFLOAT_L,                    /*!< list of unsigned floats */
                   FLOAT_L,                       /*!< list of signed floats */
                   UFLOAT_AL,         /*!< array of lists of unsigned floats */
                   DATE,                                           /*!< date */
                   TIME,                                           /*!< time */
                   SEX,                                          /*!< gender */
                   ORIENTATION,                    /*!< orientation in space */
                   PETDT,                                 /*!< PET data type */
                   NUMFORM,                               /*!< number format */
                   DTDES_A,              /*!< array of datatype descriptions */
                   ENDIAN,                                    /*!< endianess */
                   DATASET_A,                         /*!< array of datasets */
                   DIRECTION,                            /*!< scan direction */
                   SEPTA,                                   /*!< septa state */
                   DETMOTION,                           /*!< detector motion */
                   EXAMTYPE,                           /*!< examination type */
                   ACQCOND,                       /*!< acquisition condition */
                   RELFILENAME,                       /*!< relative filename */
                   SCANNER_TYPE,                           /*!< scanner type */
                   DECAYCORR,              /*!< decay correction information */
                   SL_ORIENTATION,                    /*!< slice orientation */
                   RANDCORR,             /*!< randoms correction information */
                   PROCSTAT                           /*!< processing status */
                 } ttoken_type;
                                                                 /*! genders */
    typedef enum { MALE,                                           /*!< male */
                   FEMALE,                                       /*!< female */
                   UNKNOWN_SEX                                  /*!< unknown */
                 } tsex;
                                          /*! different patient orientations */
    typedef enum { FFP,                                /*!< feet first prone */
                   HFP,                                /*!< head first prone */
                   FFS,                               /*!< feet first supine */
                   HFS,                               /*!< head first supine */
                   FFDR,                     /*!< feet first decubitus right */
                   HFDR,                     /*!< head first decubitus right */
                   FFDL,                      /*!< feet first decubitus left */
                   HFDL                       /*!< head first decubitus left */
                 } tpatient_orientation;
                                           /*! different scanning directions */
    typedef enum { SCAN_IN,                       /*!< bed moves into gantry */
                   SCAN_OUT                     /*!< bed moves out of gantry */
                 } tscan_direction;
                                                /*! different PET data types */
    typedef enum { EMISSION,                              /*!< emission data */
                   TRANSMISSION,                      /*!< transmission data */
                   EMISSION_TRANSMISSION,/*!< emission and transmission data */
                   IMAGE,                                    /*!< image data */
                   NORMALIZATION                     /*!< normalization data */
                 } tpet_data_type;
                                                /*! different number formats */
    typedef enum { SIGNED_INT_NF,                      /*!< signed short int */
                   FLOAT_NF                                       /*!< float */
                 } tnumber_format;
                                        /*! different data type descriptions */
    typedef enum { CORRECTED_PROMPTS,                 /*!< corrected prompts */
                   PROMPTS,                                     /*!< prompts */
                   RANDOMS                                      /*!< randoms */
                 } tdata_type_description;
                                                /*! different endian formats */
    typedef enum { LITTLE_END,                            /*!< little endian */
                   BIG_END                                   /*!< big endian */
                 } tendian;
                                                  /*! different septa states */
    typedef enum { NOSEPTA,                                    /*!< no septa */
                   EXTENDED,                        /*!< septa extended (2d) */
                   RETRACTED                       /*!< septa retracted (3d) */
                 } tsepta;
                                      /*! different kinds of detector motion */
    typedef enum { NOMOTION,                                  /*!< no motion */
                   STEPSHOOT,                     /*!< step and shoot motion */
                   WOBBLE,                                /*!< wobble motion */
                   CONTINUOUS,                         /*!< continous motion */
                   CLAMSHELL                           /*!< clamshell motion */
                 } tdetector_motion;
                                                /*! different types of exams */
    typedef enum { STATIC,                                  /*!< static exam */
                   DYNAMIC,                                /*!< dynamic exam */
                   GATED,                                    /*!< gated exam */
                   WHOLEBODY                             /*!< wholebody exam */
                 } texamtype;
                                        /*! acquisition start/stop condition */
    typedef enum { CND_CNTS,                                     /*!< counts */
                   CND_DENS,                       /*!< density (counts/sec) */
                      /*! relative density difference (change in counts/sec) */
                   CND_RDD,
                   CND_MANU,                                     /*!< manual */
                   CND_OVFL,                              /*!< data overflow */
                   CND_TIME,                                       /*!< time */
                   CND_TRIG                       /*!< physiological trigger */
                 } tacqcondition;
                                                           /*! scanner types */
    typedef enum { CYLINDRICAL_RING,                        /*!< cylindrical */
                   HEX_RING,                             /*!< hexagonal ring */
                   MULTIPLE_PLANAR            /*!< multiple planar detectors */
                 } tscanner_type;
                                                        /*! decay correction */
    typedef enum { NODEC,                           /*!< no decay correction */
                   START,             /*!< corrected towards scan start time */
                   ADMIN          /*!< corrected towards administration time */
                 } tdecay_correction;
                                                       /*! slice orientation */
    typedef enum { TRANSVERSE,                              /*!< transversal */
                   CORONAL,                                     /*!< coronal */
                   SAGITAL                                      /*!< sagital */
                 } tslice_orientation;
                                                      /*! randoms correction */
    typedef enum { NORAN,                                    /*!< no randoms */
                   DLYD,                                        /*!< delayed */
                   RCSING,                           /*!< singles estimation */
                   PROC
                 } trandom_correction;
                                                          /*! process status */
    typedef enum { RECONSTRUCTED                          /*!< reconstructed */
                 } tprocess_status;
                                                  /*! structure for data set */
    typedef struct {                                  /*! acquisition number */
                     unsigned long int acquisition_number;
                     std::string *headerfile,       /*!< name of header file */
                                 *datafile;        /*!< name of dataset file */
                   } tdataset;
   private:
                                            /*! structure for array elements */
    typedef struct { union {                    /*! unsigned short int value */
                             unsigned short int usi;
                             uint64 ulli;  /*!< unsigned long long int value */
                             int64 lli;      /*!< signed long long int value */
                                                 /*! unsigned long int value */
                             unsigned long int uli;
                             float f;                       /*!< float value */
                             tdataset ds;            /*!< data set structure */
                                             /*! data type description value */
                             tdata_type_description dtd;
                             std::string *ascii;           /*!< string value */
                                                   /*! list of string values */
                             std::vector <std::string> *ascii_list;
                                       /*! list of unsigned short int values */
                             std::vector <unsigned short int> *usi_list;
                                                    /*! list of float values */
                             std::vector <float> *f_list;
                           };
                     std::string unit,                     /*!< unit for key */
                                 comment;         /*!< comment of key[index] */
                     unsigned short int index;                    /*!< index */
                   } tarray;
                                                            /*! value of key */
    typedef union { std::string *ascii;               /*!< pointer to string */
                                           /*! vector of unsigned short ints */
                    std::vector <unsigned short int> *usi_l;
                                            /*! vector of unsigned long ints */
                    std::vector <unsigned long int> *uli_l;
                    std::vector <float> *f_l;          /*!< vector of floats */
                                                       /*! vector of strings */
                    std::vector <std::string> *ascii_l;
                                                /*! vector of array elements */
                    std::vector <tarray *> *array;
                    unsigned short int usi;    /*!< unsigned short int value */
                    unsigned long int uli;      /*!< unsigned long int value */
                    signed long int sli;          /*!< signed long int value */
                    uint64 ulli;           /*!< unsigned long long int value */
                    int64 lli;               /*!< signed long long int value */
                    float f;                                /*!< float value */
                    double d;                              /*!< double value */
                    TIMEDATE::tdate date;                    /*!< date value */
                    TIMEDATE::ttime time;                    /*!< time value */
                    tsex sex;                                 /*!< sex value */
                    tpatient_orientation po;  /*!< patient orientation value */
                    tscan_direction dir;           /*!< scan direction value */
                    tpet_data_type pdt;             /*!< pet data type value */
                    tnumber_format nufo;            /*!< number format value */
                    tendian endian;                        /*!< endian value */
                    tsepta septa;                           /*!< septa state */
                    tdetector_motion detmot;      /*!< detector motion value */
                    texamtype examtype;                 /*!< exam type value */
                                        /*! acquisition start/stop condition */
                    tacqcondition acqcond;
                    tscanner_type sct;               /*!< scanner type value */
                    tdecay_correction dec;       /*!< decay correction value */
                    tslice_orientation so;      /*!< slice orientation value */
                    trandom_correction rc;      /*!< random correction value */
                    tprocess_status ps;            /*!< process status value */
                  } tvalue;
    tvalue v;                                              /*!< value of key */
    std::string key,                                        /*!< name of key */
                unit,                                       /*!< unit of key */
                comment,                                 /*!< comment of key */
               /*! name of key that specifies maximum index of list in array */
                max_lindex_key,
                      /*! name of key which specifies maximum index of array */
                max_index_key;
    ttoken_type type;                                     /*!< type of value */
    void checkDate(const std::string) const;          // check if date is valid
                                          // parse unsigned long long int value
    void parseNumber(uint64 * const, const std::string, std::string,
                     const std::string) const;
                                            // parse signed long long int value
    void parseNumber(int64 * const, const std::string, std::string,
                     const std::string) const;
                                                 // parse signed long int value
    void parseNumber(signed long int * const, const signed long int,
                     const signed long int, const std::string,
                     const std::string, const std::string) const;
                                                           // parse float value
    void parseNumber(float * const, const float, const float,
                     const std::string, const std::string,
                     const std::string) const;
                                                // print line including comment
    void printLine(std::ostream * const, std::string,
                   const std::string comment) const;
   public:
    KV(const std::string);                                 // create empty line
                                                     // create key/value object
    KV(const std::string, const std::string, std::string, ttoken_type,
       const std::string, const std::string);
                                        // create key/value object for TIME key
    KV(const std::string, const std::string, std::string, ttoken_type,
       const std::string, const std::string, const std::string);
                                 // create key/value object with list of values
    KV(const std::string, const std::string, const std::string, std::string,
       ttoken_type, const std::string, const std::string);
                                              // create key[index]/value object
    KV(const std::string, const std::string, const unsigned short int,
       const std::string, std::string, ttoken_type, const std::string,
       const std::string);
                                              // create key[index]/value object
    KV(const std::string, const std::string, const unsigned short int,
       const std::string, const std::string, std::string, ttoken_type,
       const std::string, const std::string);
    ~KV();                                                    // destroy object
    KV& operator = (const KV &);                                // '='-operator
                                                   // add array value and index
    void add(const std::string, const unsigned short int, std::string,
             const std::string, const std::string) const;
                                                   // convert value into string
    template <typename T>
     static std::string convertToString(const T);
    static std::string convertToString(const TIMEDATE::tdate);
    static std::string convertToString(const TIMEDATE::ttime);
    static std::string convertToString(const tsex);
    static std::string convertToString(const tpatient_orientation);
    static std::string convertToString(const tscan_direction);
    static std::string convertToString(const tpet_data_type);
    static std::string convertToString(const tnumber_format);
    static std::string convertToString(const tdata_type_description);
    static std::string convertToString(const tendian);
    static std::string convertToString(const tdataset);
    static std::string convertToString(const tsepta);
    static std::string convertToString(const tdetector_motion);
    static std::string convertToString(const texamtype);
    static std::string convertToString(const tacqcondition);
    static std::string convertToString(const tscanner_type);
    static std::string convertToString(const tdecay_correction);
    static std::string convertToString(const tslice_orientation);
    static std::string convertToString(const trandom_correction);
    static std::string convertToString(const tprocess_status);
    std::string getUnit() const;                       // request onit of a key
    template <typename T>
     T getValue() const;                                // request value of key
    template <typename T>          // request value of key with array of values
     T getValue(const unsigned short int) const;
    template <typename T> // request value of key with array of lists of values
     T getValue(const unsigned short int, const unsigned short int) const;
    std::string Key() const;                             // request name of key
                  // request name of key which specifies maximum index of array
    std::string MaxIndexKey() const;
          // request name of key which specifies maximum index of list in array
    std::string MaxLIndexKey() const;
                                               // parse unsigned long int value
    static void parseNumber(unsigned long int * const, const unsigned long int,
                            const std::string, const std::string,
                            const std::string);
                                         // print key, index, value and comment
    void print(std::ostream * const) const;
                                            // remove value from array of lists
    bool removeValue(const unsigned short int, const unsigned short int,
                     const std::string);
                                                     // remove value from array
    bool removeValue(const unsigned short int, const std::string);
    void removeValues();                        // remove all values from array
                                         // perform semantic check of key/value
    void semanticCheck(const std::string, const unsigned short int) const;
                                   // perform semantic check for array of lists
    void semanticCheckAL(const std::string, const KV * const) const;
    template <typename T>                                   // set value of key
     void setValue(const T, const std::string, const std::string,
                   const std::string);
    template <typename T>              // set value of key with array of values
     void setValue(const T, const std::string, const std::string,
                   const unsigned short int, const std::string);
    template <typename T>     // set value of key with array of lists of values
     void setValue(const T, const std::string, const std::string,
                   const unsigned short int, const unsigned short int,
                   const std::string);
    ttoken_type Type() const;                          // request type of value
 };

#ifndef _KV_CPP
#define _KV_TMPL_CPP
#include "kv.cpp"
#endif

#endif
