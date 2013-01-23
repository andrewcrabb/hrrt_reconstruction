/*! \class KV kv.h "kv.h"
    \brief This class is used to handle a key/value pair from an Interfile
           file.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments
    \date 2004/12/14 added method getUnit()
    \date 2005/01/15 cleanup templates
    \date 2005/02/07 support for int64 values and arrays

    This class is an abstraction layer for one line of an Interfile header. A
    line consists of key, unit, index, value and comment. The class contains
    code to parse these elements and fill them into the internal data
    structure, print the line and change parts of the line. For every key of
    an Interfile header one instantiation of this object is created. In case of
    an array key, this object stores all values of this array.
 */

#include <iostream>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <errno.h>
#ifndef _KV_CPP
#define _KV_CPP
#include "kv.h"
#endif
#include "exception.h"
#include "str_tmpl.h"

#ifdef WIN32
#undef max                        // some comedian in Seattle has defined max()
#undef min                        // and min()
#endif

/*- methods -----------------------------------------------------------------*/

#ifndef _KV_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Create object for empty line.
    \param[in] _comment   comment for empty line

    Create object for a line that is empty, except of an optional comment.
 */
/*---------------------------------------------------------------------------*/
KV::KV(const std::string _comment)
 { key=std::string();
   unit=std::string();
   type=KV::NONE;
   comment=_comment;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create object for key-value pair and parse value.
    \param[in] _key       name of key
    \param[in] _unit      unit of key
    \param[in] value      value of key
    \param[in] _type      type of key
    \param[in] comment    comment of key
    \param[in] filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE           the key must not have a
                                                    value
    \exception REC_ITF_HEADER_ERROR_VALUE_MISSING   the value for the key is
                                                    missing
    \exception REC_ITF_HEADER_ERROR_DATE_VALUE      the format of the date-
                                                    value is wrong
    \exception REC_ITF_HEADER_ERROR_SEX_INVALID     the value for the gender is
                                                    invalid
    \exception REC_ITF_HEADER_ERROR_ORIENTATION     the value for the patient
                                                    orientation is invalid
    \exception REC_ITF_HEADER_ERROR_DIRECTION       the value for the scan
                                                    direction is invalid
    \exception REC_ITF_HEADER_ERROR_PET_DATATYPE    the value for the PET
                                                    datatype is invalid
    \exception REC_ITF_HEADER_ERROR_NUMBER_FORMAT   the value for the number
                                                    format is invalid
    \exception REC_ITF_HEADER_ERROR_ENDIAN          the value for the
                                                    endianess is invalid
    \exception REC_ITF_HEADER_ERROR_SEPTA_FORMAT    the value for the septa
                                                    state is invalid
    \exception REC_ITF_HEADER_ERROR_DETMOT_FORMAT   the value for the detector
                                                    motion format is invalid
    \exception REC_ITF_HEADER_ERROR_EXAMTYPE_FORMAT the value for the exam type
                                                    is invalid
    \exception REC_ITF_HEADER_ERROR_ACQCOND_FORMAT  the value for the
                                                    acquisition condition is
                                                    invalid
    \exception REC_ITF_HEADER_ERROR_SCANNERTYPE     the value for the scanner
                                                    type is invalid
    \exception REC_ITF_HEADER_ERROR_DECAYCORR       the value for the decay
                                                    correction type is invalid
    \exception REC_ITF_HEADER_ERROR_SL_ORIENTATION  the value for the slice
                                                    orientation is invalid
    \exception REC_ITF_HEADER_ERROR_RANDCORR        the value for the randoms
                                                    correction type is invalid
    \exception REC_ITF_HEADER_ERROR_PROCSTAT        the value for the
                                                    processing state is invalid

    Create object for key-value pair and parse value.
 */
/*---------------------------------------------------------------------------*/
KV::KV(const std::string _key, const std::string _unit, std::string value,
       ttoken_type _type, const std::string _comment,
       const std::string filename)
 { v.ascii=NULL;
   try
   { std::string::size_type p;
     unsigned long int uli;

     key=_key;
     unit=_unit;
     type=_type;
     comment=_comment;
                                                                 // parse value
     switch (type)
      { case KV::NONE:                                      // key has no value
         if (!value.empty())
          throw Exception(REC_ITF_HEADER_ERROR_VALUE,
                          "Parsing error in the file '#1'. The key '#2' must "
                          "not have a value.").arg(filename).arg(key);
         break;
        case KV::ASCII:                                 // key has string value
        case KV::RELFILENAME:
         if (value.empty()) { v.ascii=NULL;
                              return;
                            }
         v.ascii=new std::string(value);
         break;
        case KV::USHRT:                     // key has unsigned short int value
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, value, filename);
         v.usi=(unsigned short int)uli;
         break;
        case KV::ULONG:                      // key has unsigned long int value
         parseNumber(&v.uli, std::numeric_limits <unsigned long int>::max(),
                     key, value, filename);
         break;
        case KV::LONG:                         // key has signed long int value
         parseNumber(&v.sli, std::numeric_limits <signed long int>::min(),
                     std::numeric_limits <signed long int>::max(), key, value,
                     filename);
         break;
        case KV::LONGLONG:                // key has signed long long int value
         parseNumber(&v.lli, key, value, filename);
         break;
        case KV::ULONGLONG:             // key has unsigned long long int value
         parseNumber(&v.ulli, key, value, filename);
         break;
        case KV::UFLOAT:       // key has unsigned float (positive float) value
         parseNumber(&v.f, 0.0, std::numeric_limits <float>::max(), key, value,
                     filename);
         break;
        case KV::FLOAT:                                  // key has float value
         parseNumber(&v.f, -std::numeric_limits <float>::max(),
                     std::numeric_limits <float>::max(), key, value, filename);
         break;
        case KV::DATE:                                    // key has date value
         if (value.empty())
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_MISSING,
                          "Parsing error in the file '#1'. The value for the "
                          "key '#2' is missing.").arg(filename).arg(key);
         if ((p=value.find(':')) == std::string::npos)
          throw Exception(REC_ITF_HEADER_ERROR_DATE_VALUE,
                          "Parsing error in the file '#1'. The value of the "
                          "key '#2' must have the format 'yyyy:mm:dd'"
                          ".").arg(filename).arg(key);
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, value.substr(0, p), filename);
         v.date.year=(unsigned short int)uli;
         value.erase(0, p+1);
         if ((p=value.find(':')) == std::string::npos)
          throw Exception(REC_ITF_HEADER_ERROR_DATE_VALUE,
                          "Parsing error in the file '#1'. The value of the "
                          "key '#2' must have the format 'yyyy:mm:dd'"
                          ".").arg(filename).arg(key);
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, value.substr(0, p), filename);
         v.date.month=(unsigned short int)uli;
         value.erase(0, p+1);
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, value.substr(0, p), filename);
         v.date.day=(unsigned short int)uli;
         checkDate(filename);                       // check if date is correct
         break;
        case KV::SEX:                                      // key has sex value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "m") v.sex=MALE;
          else if (value == "f") v.sex=FEMALE;
          else if (value == "unknown") v.sex=UNKNOWN_SEX;
          else throw Exception(REC_ITF_HEADER_ERROR_SEX_INVALID,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"m\", \"f\" and "
                               "\"unknown\".").arg(filename).arg(key);
         break;
        case KV::ORIENTATION:              // key has patient orientation value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "ffp") v.po=FFP;
          else if (value == "hfp") v.po=HFP;
          else if (value == "ffs") v.po=FFS;
          else if (value == "hfs") v.po=HFS;
          else if (value == "ffdr") v.po=FFDR;
          else if (value == "hfdr") v.po=HFDR;
          else if (value == "ffdl") v.po=FFDL;
          else if (value == "hfdl") v.po=HFDL;
          else throw Exception(REC_ITF_HEADER_ERROR_ORIENTATION,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"ffp\", \"hfp\", \"ffs\", \"hfs\", \"ffdr\", "
                               "\"hfdr\", \"ffdl\" and \"hfdl\""
                               ".").arg(filename).arg(key);
         break;
        case KV::DIRECTION:                          // key has direction value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "in") v.dir=SCAN_IN;
          else if (value == "out") v.dir=SCAN_OUT;
          else throw Exception(REC_ITF_HEADER_ERROR_DIRECTION,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"in\" and \"out\".").arg(filename).arg(key);
         break;
        case KV::PETDT:                          // key has PET data type value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "emission") v.pdt=EMISSION;
          else if (value == "transmission") v.pdt=TRANSMISSION;
          else if (value == "simultaneous emission + transmission")
                v.pdt=EMISSION_TRANSMISSION;
          else if (value == "image") v.pdt=IMAGE;
          else if (value == "normalization") v.pdt=NORMALIZATION;
          else throw Exception(REC_ITF_HEADER_ERROR_PET_DATATYPE,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"emission\", \"transmission\", "
                               "\"simultaneous emission + transmission\" and "
                               "\"image\".").arg(filename).arg(key);
         break;
        case KV::NUMFORM:                        // key has number format value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "signed integer") v.nufo=SIGNED_INT_NF;
          else if (value == "float") v.nufo=FLOAT_NF;
          else throw Exception(REC_ITF_HEADER_ERROR_NUMBER_FORMAT,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"signed integer\" and \"float\""
                               ".").arg(filename).arg(key);
         break;
        case KV::ENDIAN:                                // key has endian value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "littleendian") v.endian=LITTLE_END;
          else if (value == "bigendian") v.endian=BIG_END;
          else throw Exception(REC_ITF_HEADER_ERROR_ENDIAN,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"littleendian\" and \"bigendian\""
                               ".").arg(filename).arg(key);
         break;
        case KV::SEPTA:                            // key has septa state value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "none") v.septa=NOSEPTA;
          else if (value == "extended") v.septa=EXTENDED;
          else if (value == "retracted") v.septa=RETRACTED;
          else throw Exception(REC_ITF_HEADER_ERROR_SEPTA_FORMAT,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"none\", \"extended\" and \"retracted\""
                               ".").arg(filename).arg(key);
         break;
        case KV::DETMOTION:                    // key has detector motion value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "none") v.detmot=NOMOTION;
          else if (value == "step and shoot") v.detmot=STEPSHOOT;
          else if (value == "wobble") v.detmot=WOBBLE;
          else if (value == "continuous") v.detmot=CONTINUOUS;
          else if (value == "clamshell") v.detmot=CLAMSHELL;
          else throw Exception(REC_ITF_HEADER_ERROR_DETMOT_FORMAT,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"none\", \"step and shoot\", \"wobble\", "
                               "\"continuous\" and \"clamshell\""
                               ".").arg(filename).arg(key);
         break;
        case KV::EXAMTYPE:                           // key has exam type value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "static") v.examtype=STATIC;
          else if (value == "dynamic") v.examtype=DYNAMIC;
          else if (value == "gated") v.examtype=GATED;
          else if (value == "wholebody") v.examtype=WHOLEBODY;
          else throw Exception(REC_ITF_HEADER_ERROR_EXAMTYPE_FORMAT,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"static\", \"dynamic\", \"gated\" and "
                               "\"wholebody\".").arg(filename).arg(key);
         break;
        case KV::ACQCOND:                            // key has exam type value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "cnts") v.acqcond=CND_CNTS;
          else if (value == "dens") v.acqcond=CND_DENS;
          else if (value == "rdd") v.acqcond=CND_RDD;
          else if (value == "manu") v.acqcond=CND_MANU;
          else if (value == "ovfl") v.acqcond=CND_OVFL;
          else if (value == "time") v.acqcond=CND_TIME;
          else if (value == "trig") v.acqcond=CND_TRIG;
          else throw Exception(REC_ITF_HEADER_ERROR_ACQCOND_FORMAT,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"cnts\", \"dens\", \"rdd\", \"manu\", "
                               "\"ovfl\", \"time\" and \"trig\""
                               ".").arg(filename).arg(key);
         break;
        case KV::SCANNER_TYPE:                    // key has scanner type value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "cylindrical ring") v.sct=CYLINDRICAL_RING;
          else if (value == "hexagonal ring") v.sct=HEX_RING;
          else if (value == "multiple planar") v.sct=MULTIPLE_PLANAR;
          else throw Exception(REC_ITF_HEADER_ERROR_SCANNERTYPE,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"cylindrical ring\", \"hexagonal ring\" and "
                               "\"multiple planar\""
                               ".").arg(filename).arg(key);
         break;
        case KV::DECAYCORR:                   // key has decay correction value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "none") v.dec=NODEC;
          else if (value == "start") v.dec=START;
          else if (value == "admin") v.dec=ADMIN;
          else throw Exception(REC_ITF_HEADER_ERROR_DECAYCORR,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"none\", \"start\" and \"admin\""
                               ".").arg(filename).arg(key);
         break;
        case KV::SL_ORIENTATION:             // key has slice orientation value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "transverse") v.so=TRANSVERSE;
          else if (value == "coronal") v.so=CORONAL;
          else if (value == "sagital") v.so=SAGITAL;
          else throw Exception(REC_ITF_HEADER_ERROR_SL_ORIENTATION,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"transverse\", \"coronal\" and \"sagital\""
                               ".").arg(filename).arg(key);
         break;
        case KV::RANDCORR:                   // key has random correction value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "none") v.rc=NORAN;
          else if (value == "dlyd") v.rc=DLYD;
          else if (value == "sing") v.rc=RCSING;
          else if (value == "proc") v.rc=PROC;
          else throw Exception(REC_ITF_HEADER_ERROR_RANDCORR,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"none\", \"dlyd\", \"sing\" and \"proc\""
                               ".").arg(filename).arg(key);
         break;
        case KV::PROCSTAT:                   // key has random correction value
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "reconstructed") v.ps=RECONSTRUCTED;
          else throw Exception(REC_ITF_HEADER_ERROR_PROCSTAT,
                               "Parsing error in the file '#1'. The value for "
                               "the key '#2' is invalid. Allowed values are "
                               "\"reconstructed\".").arg(filename).arg(key);
         break;
        default:                                                // can't happen
         break;
      }
   }
   catch (...)
    { switch (type)
       { case KV::ASCII:
         case KV::RELFILENAME:
          if (v.ascii != NULL) { delete v.ascii;
                                 v.ascii=NULL;
                               }
           break;
          default:
           break;
       }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create object for key-value pair where the value has an offset.
    \param[in] _key       name of key
    \param[in] _unit      unit of key
    \param[in] value      value of key
    \param[in] _type      type of key
    \param[in] offset     offset for value
    \param[in] comment    comment of key
    \param[in] filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_MISSING the value is missing
    \exception REC_ITF_HEADER_ERROR_TIME_VALUE    the format of the value is
                                                  wrong
    \exception REC_ITF_HEADER_ERROR_TIME_INVALID  the value is invalid

    Create object for key-value pair. The value has an offset, like the GMT
    offset in a time-value. The constructor also parses the value.
 */
/*---------------------------------------------------------------------------*/
KV::KV(const std::string _key, const std::string _unit, std::string value,
       ttoken_type _type, const std::string offset, const std::string _comment,
       const std::string filename)
 { std::string::size_type p;
   unsigned long int uli;
   signed long int sli;

   key=_key;
   unit=_unit;
   type=_type;
   comment=_comment;
                                                                 // parse value
     switch (type)
      { case KV::TIME:                                    // key has time value
         if (value.empty())
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_MISSING,
                          "Parsing error in the file '#1'. The value for the "
                          "key '#2' is missing.").arg(filename).arg(key);
         if ((p=value.find(':')) == std::string::npos)
          throw Exception(REC_ITF_HEADER_ERROR_TIME_VALUE,
                          "Parsing error in the file '#1'. The value of the "
                          "key '#2' must have the format 'hh:mm:ss'"
                          ".").arg(filename).arg(key);
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, value.substr(0, p), filename);
         if (uli > 23)
          throw Exception(REC_ITF_HEADER_ERROR_TIME_INVALID,
                          "Parsing error in the file '#1'. The time in the key"
                          " '#2' is invalid.").arg(filename).arg(key);
         v.time.hour=(unsigned short int)uli;
         value.erase(0, p+1);
         if ((p=value.find(':')) == std::string::npos)
          throw Exception(REC_ITF_HEADER_ERROR_TIME_VALUE,
                          "Parsing error in the file '#1'. The value of the "
                          "key '#2' must have the format 'hh:mm:ss'"
                          ".").arg(filename).arg(key);
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, value.substr(0, p), filename);
         if (uli > 59)
          throw Exception(REC_ITF_HEADER_ERROR_TIME_INVALID,
                          "Parsing error in the file '#1'. The time in the key"
                          " '#2' is invalid.").arg(filename).arg(key);
         v.time.minute=(unsigned short int)uli;
         value.erase(0, p+1);
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, value.substr(0, p), filename);
         if (uli > 59)
          throw Exception(REC_ITF_HEADER_ERROR_TIME_INVALID,
                          "Parsing error in the file '#1'. The time in the key"
                          " '#2' is invalid.").arg(filename).arg(key);
         v.time.second=(unsigned short int)uli;
         p=offset.find(':');
         if ((offset.substr(0, 3) != "gmt") || (p == std::string::npos))
          throw Exception(REC_ITF_HEADER_ERROR_TIME_INVALID,
                          "Parsing error in the file '#1'. The time in the key"
                          " '#2' is invalid.").arg(filename).arg(key);
         parseNumber(&sli, std::numeric_limits <signed short int>::min(),
                     std::numeric_limits <unsigned short int>::max(), key,
                     offset.substr(3, p-3), filename);
         if ((sli < -23) || (sli > 23))
          throw Exception(REC_ITF_HEADER_ERROR_TIME_INVALID,
                          "Parsing error in the file '#1'. The time in the key"
                          " '#2' is invalid.").arg(filename).arg(key);
         v.time.gmt_offset_h=(signed short int)sli;
         parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                     key, offset.substr(p+1, 2), filename);
         if (uli > 59)
          throw Exception(REC_ITF_HEADER_ERROR_TIME_INVALID,
                          "Parsing error in the file '#1'. The time in the key"
                          " '#2' is invalid.").arg(filename).arg(key);
         v.time.gmt_offset_m=(unsigned short int)uli;
         break;
        default:                                                // can't happen
         break;
      }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create object for key-"list-of-values" pair.
    \param[in] _key             name of key
    \param[in] _unit            unit of key
    \param[in] _max_index_key   name of key which specifies maximum index of
                                the list
    \param[in] value            value of key
    \param[in] _type            type of key
    \param[in] comment          comment of key
    \param[in] filename         name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING the closing bracket in
                                                         the value is missing
    \exception REC_ITF_HEADER_ERROR_VALUE_FORMAT         the format of the
                                                         value is wrong

    Create object for key-"list of values" pair. The value has the format
    "{v1,v2,v3,...,vn}".
 */
/*---------------------------------------------------------------------------*/
KV::KV(const std::string _key, const std::string _unit,
       const std::string _max_index_key, std::string value, ttoken_type _type,
       const std::string _comment, const std::string filename)
 { v.usi_l=NULL;
   v.f_l=NULL;
   try
   { std::string::size_type p;
     unsigned long int uli;
     float uf;

     key=_key;
     unit=_unit;
     type=_type;
     comment=_comment;
     max_index_key=_max_index_key;
                                                                 // parse value
     switch (type)
      { case KV::ASCII_L:                      // key has list of string values
         v.ascii_l=NULL;
         if ((p=value.find('{')) != std::string::npos)
           { v.ascii_l=new std::vector <std::string>;
             value.erase(0, p+1);
             while ((p=value.find(',')) != std::string::npos)
              { v.ascii_l->push_back(value.substr(0, p));
                value.erase(0, p+1);
              }
             if ((p=value.find('}')) != std::string::npos)
              v.ascii_l->push_back(value.substr(0, p));
              else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                   "Parsing error in the file '#1'. The "
                                   "closing bracket in the value of the key "
                                   "'#2' is missing.").arg(filename).arg(key);
           }
           else if (!value.empty())
                 throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                 "Parsing error in the file '#1'. The value "
                                 "for the key '#2' has the wrong format"
                                 ".").arg(filename).arg(key);
         break;
        case KV::USHRT_L:          // key has list of unsigned short int values
         v.usi_l=NULL;
         if ((p=value.find('{')) != std::string::npos)
           { v.usi_l=new std::vector <unsigned short int>;
             value.erase(0, p+1);
             while ((p=value.find(',')) != std::string::npos)
              { parseNumber(&uli,
                            std::numeric_limits <unsigned short int>::max(),
                            key, value.substr(0, p), filename);
                v.usi_l->push_back((unsigned short int)uli);
                value.erase(0, p+1);
              }
             if ((p=value.find('}')) != std::string::npos)
              { parseNumber(&uli,
                            std::numeric_limits <unsigned short int>::max(),
                            key, value.substr(0, p), filename);
                v.usi_l->push_back((unsigned short int)uli);
              }
              else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                   "Parsing error in the file '#1'. The "
                                   "closing bracket in the value of the key "
                                   "'#2' is missing.").arg(filename).arg(key);
           }
           else if (!value.empty())
                 throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                 "Parsing error in the file '#1'. The value "
                                 "for the key '#2' has the wrong format"
                                 ".").arg(filename).arg(key);
         break;
        case KV::ULONG_L:                 // key has list of unsigned long ints
         v.uli_l=NULL;
         if ((p=value.find('{')) != std::string::npos)
           { v.uli_l=new std::vector <unsigned long int>;
             value.erase(0, p+1);
             while ((p=value.find(',')) != std::string::npos)
              { parseNumber(&uli,
                            std::numeric_limits <unsigned long int>::max(),
                            key, value.substr(0, p), filename);
                v.uli_l->push_back(uli);
                value.erase(0, p+1);
              }
             if ((p=value.find('}')) != std::string::npos)
              { parseNumber(&uli,
                            std::numeric_limits <unsigned long int>::max(),
                            key, value.substr(0, p), filename);
                v.uli_l->push_back(uli);
              }
              else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                   "Parsing error in the file '#1'. The "
                                   "closing bracket in the value of the key "
                                   "'#2' is missing.").arg(filename).arg(key);
           }
           else if (!value.empty())
                 throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                 "Parsing error in the file '#1'. The value "
                                 "for the key '#2' has the wrong format"
                                 ".").arg(filename).arg(key);
         break;
        case KV::UFLOAT_L:                   // key has list of unsigned floats
         v.f_l=NULL;
         if ((p=value.find('{')) != std::string::npos)
           { v.f_l=new std::vector <float>;
             value.erase(0, p+1);
             while ((p=value.find(',')) != std::string::npos)
              { parseNumber(&uf, 0.0f, std::numeric_limits <float>::max(), key,
                            value.substr(0, p), filename);
                v.f_l->push_back(uf);
                value.erase(0, p+1);
              }
             if ((p=value.find('}')) != std::string::npos)
              { parseNumber(&uf, 0.0f, std::numeric_limits <float>::max(), key,
                            value.substr(0, p), filename);
                v.f_l->push_back(uf);
              }
              else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                   "Parsing error in the file '#1'. The "
                                   "closing bracket in the value of the key "
                                   "'#2' is missing.").arg(filename).arg(key);
           }
           else if (!value.empty())
                 throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                 "Parsing error in the file '#1'. The value "
                                 "for the key '#2' has the wrong format"
                                 ".").arg(filename).arg(key);
         break;
        case KV::FLOAT_L:                      // key has list of signed floats
         v.f_l=NULL;
         if ((p=value.find('{')) != std::string::npos)
           { v.f_l=new std::vector <float>;
             value.erase(0, p+1);
             while ((p=value.find(',')) != std::string::npos)
              { parseNumber(&uf, -std::numeric_limits <float>::max(),
                            std::numeric_limits <float>::max(), key,
                            value.substr(0, p), filename);
                v.f_l->push_back(uf);
                value.erase(0, p+1);
              }
             if ((p=value.find('}')) != std::string::npos)
              { parseNumber(&uf, -std::numeric_limits <float>::max(),
                            std::numeric_limits <float>::max(), key,
                            value.substr(0, p), filename);
                v.f_l->push_back(uf);
              }
              else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                   "Parsing error in the file '#1'. The "
                                   "closing bracket in the value of the key "
                                   "'#2' is missing.").arg(filename).arg(key);
           }
           else if (!value.empty())
                 throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                 "Parsing error in the file '#1'. The value "
                                 "for the key '#2' has the wrong format"
                                 ".").arg(filename).arg(key);
         break;
        default:                                                // can't happen
         break;
      }
   }
   catch (...)
    { switch (type)
       { case KV::ASCII_L:
          if (v.ascii_l != NULL) { delete v.ascii_l;
                                   v.ascii_l=NULL;
                                 }
          break;
         case KV::USHRT_L:
          if (v.usi_l != NULL) { delete v.usi_l;
                                 v.usi_l=NULL;
                               }
          break;
         case KV::ULONG_L:
          if (v.uli_l != NULL) { delete v.uli_l;
                                 v.uli_l=NULL;
                               }
          break;
         case KV::UFLOAT_L:
         case KV::FLOAT_L:
          if (v.f_l != NULL) { delete v.f_l;
                               v.f_l=NULL;
                             }
          break;
         default:
          break;
       }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create object for key-value pair where the key is an array.
    \param[in] _key             name of key
    \param[in] _unit            unit of key
    \param[in] index            index of array
    \param[in] _max_index_key   name of key which specifies maximum index of
                                the array
    \param[in] value            value of key
    \param[in] _type            type of key
    \param[in] comment          comment of key
    \param[in] filename         name of Interfile header

    Create object for key-value pair where the key is an array.
 */
/*---------------------------------------------------------------------------*/
KV::KV(const std::string _key, const std::string _unit,
       const unsigned short int index, const std::string _max_index_key,
       std::string value, ttoken_type _type, const std::string _comment,
       const std::string filename)
 { v.array=NULL;
   try
   { std::string __unit;

     key=_key;
     __unit=_unit;
     type=_type;
     comment=_comment;
     max_index_key=_max_index_key;
     switch (type)                                // key has an array of values
      { case KV::ASCII_A:
        case KV::USHRT_A:
        case KV::ULONG_A:
        case KV::ULONGLONG_A:
        case KV::LONGLONG_A:
        case KV::UFLOAT_A:
        case KV::DATASET_A:
        case KV::DTDES_A:
         v.array=new std::vector <tarray *>;     // create vector of key values
         break;
        default:
         break;
      }
     add(__unit, index, value, comment, filename);        // add value to array
   }
   catch (...)
    { switch (_type)
       { case KV::ASCII_A:
         case KV::USHRT_A:
         case KV::ULONG_A:
         case KV::ULONGLONG_A:
         case KV::LONGLONG_A:
         case KV::UFLOAT_A:
         case KV::DATASET_A:
         case KV::DTDES_A:
          if (v.array != NULL) { delete v.array;
                                 v.array=NULL;
                               }
          break;
         default:
          break;
       }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create object for key-"list-of-values" pair where the key is an
           array.
    \param[in] _key              name of key
    \param[in] _unit             unit of key
    \param[in] index             index of array
    \param[in] _max_lindex_key   name of key which specifies maximum index of
                                 the list
    \param[in] _max_index_key    name of key which specifies maximum index of
                                 the array
    \param[in] value             value of key
    \param[in] _type             type of key
    \param[in] comment           comment of key
    \param[in] filename          name of Interfile header

    Create object for key-"list-of-values" pair where the key is an array.
 */
/*---------------------------------------------------------------------------*/
KV::KV(const std::string _key, const std::string _unit,
       const unsigned short int index, const std::string _max_lindex_key,
       const std::string _max_index_key, std::string value, ttoken_type _type,
       const std::string _comment, const std::string filename)
 { v.array=NULL;
   try
   { std::string __unit;

     key=_key;
     __unit=_unit;
     type=_type;
     comment=_comment;
     max_lindex_key=_max_lindex_key;
     max_index_key=_max_index_key;
     switch (type)                                // key has an array of values
      { case KV::ASCII_AL:
        case KV::USHRT_AL:
        case KV::UFLOAT_AL:
         v.array=new std::vector <tarray *>;     // create vector of key values
         break;
        default:
         break;
      }
     add(__unit, index, value, comment, filename);        // add value to array
   }
   catch (...)
    { switch (_type)
       { case KV::ASCII_AL:
         case KV::USHRT_AL:
         case KV::UFLOAT_AL:
          if (v.array != NULL) { delete v.array;
                                 v.array=NULL;
                               }
          break;
         default:
          break;
       }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete key-value object.

    Delete key-value object.
 */
/*---------------------------------------------------------------------------*/
KV::~KV()
 { switch (type)
    { case KV::ASCII:                                          // delete string
      case KV::RELFILENAME:
       if (v.ascii != NULL) delete v.ascii;
       break;
      case KV::ASCII_L:                              // delete vector of values
       if (v.ascii_l != NULL) delete v.ascii_l;
       break;
      case KV::USHRT_L:                              // delete vector of values
       if (v.usi_l != NULL) delete v.usi_l;
       break;
      case KV::ULONG_L:                              // delete vector of values
       if (v.uli_l != NULL) delete v.uli_l;
       break;
      case KV::UFLOAT_L:                             // delete vector of values
      case KV::FLOAT_L:
       if (v.f_l != NULL) delete v.f_l;
       break;
      case KV::ASCII_A:
      case KV::ASCII_AL:
      case KV::USHRT_A:
      case KV::USHRT_AL:
      case KV::ULONG_A:
      case KV::ULONGLONG_A:
      case KV::LONGLONG_A:
      case KV::UFLOAT_A:
      case KV::UFLOAT_AL:
      case KV::DATASET_A:
      case KV::DTDES_A:
       if (v.array != NULL)                          // delete vector of values
        { std::vector <tarray *>::iterator it;

          while (v.array->size() > 0)
           { it=v.array->begin();
             if (type == KV::ASCII_A) delete (*it)->ascii;
              else if (type == KV::ASCII_AL) delete (*it)->ascii_list;
              else if (type == KV::USHRT_AL) delete (*it)->usi_list;
              else if (type == KV::UFLOAT_AL) delete (*it)->f_list;
              else if (type == KV::DATASET_A)
                    { if ((*it)->ds.headerfile != NULL)
                       delete (*it)->ds.headerfile;
                      if ((*it)->ds.datafile != NULL)
                       delete (*it)->ds.datafile;
                    }
             delete *it;
             v.array->erase(it);
           }
          delete v.array;
        }
       break;
      default:
       break;
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy operator.
    \param[in] kv   object to copy
    \return copy of object

    Copy operator.
 */
/*---------------------------------------------------------------------------*/
KV& KV::operator = (const KV &kv)
 { if (this != &kv)
    { std::vector <tarray *>::iterator it;
      switch (kv.type)
       { case KV::ASCII:
         case KV::RELFILENAME:
          if (kv.v.ascii != NULL) v.ascii=new std::string(*(kv.v.ascii));
           else v.ascii=NULL;
          break;
         case KV::USHRT:
          v.usi=kv.v.usi;
          break;
         case KV::ULONG:
          v.uli=kv.v.uli;
          break;
         case KV::LONG:
          v.sli=kv.v.sli;
          break;
         case KV::ULONGLONG:
          v.ulli=kv.v.ulli;
          break;
         case KV::LONGLONG:
          v.lli=kv.v.lli;
          break;
         case KV::UFLOAT:
         case KV::FLOAT:
          v.f=kv.v.f;
          break;
         case KV::DATE:
          v.date=kv.v.date;
          break;
         case KV::TIME:
          v.time=kv.v.time;
          break;
         case KV::SEX:
          v.sex=kv.v.sex;
          break;
         case KV::ORIENTATION:
          v.po=kv.v.po;
          break;
         case KV::DIRECTION:
          v.dir=kv.v.dir;
          break;
         case KV::PETDT:
          v.pdt=kv.v.pdt;
          break;
         case KV::NUMFORM:
          v.nufo=kv.v.nufo;
          break;
         case KV::ENDIAN:
          v.endian=kv.v.endian;
          break;
         case KV::SEPTA:
          v.septa=kv.v.septa;
          break;
         case KV::DETMOTION:
          v.detmot=kv.v.detmot;
          break;
         case KV::EXAMTYPE:
          v.examtype=kv.v.examtype;
          break;
         case KV::ACQCOND:
          v.acqcond=kv.v.acqcond;
          break;
         case KV::SCANNER_TYPE:
          v.sct=kv.v.sct;
          break;
         case KV::DECAYCORR:
          v.dec=kv.v.dec;
          break;
         case KV::SL_ORIENTATION:
          v.so=kv.v.so;
          break;
         case KV::RANDCORR:
          v.rc=kv.v.rc;
          break;
         case KV::PROCSTAT:
          v.ps=kv.v.ps;
          break;
         case KV::ASCII_L:
          if (kv.v.ascii_l != NULL)
           { v.ascii_l=new std::vector <std::string>();
             *(v.ascii_l)=*(kv.v.ascii_l);
           }
           else v.ascii_l=NULL;
          break;
         case KV::USHRT_L:
          if (kv.v.usi_l != NULL)
           { v.usi_l=new std::vector <unsigned short int>();
             *(v.usi_l)=*(kv.v.usi_l);
           }
           else v.usi_l=NULL;
          break;
         case KV::ULONG_L:
          if (kv.v.uli_l != NULL)
           { v.uli_l=new std::vector <unsigned long int>();
             *(v.uli_l)=*(kv.v.uli_l);
           }
           else v.uli_l=NULL;
          break;
         case KV::UFLOAT_L:
         case KV::FLOAT_L:
          if (kv.v.f_l != NULL)
           { v.f_l=new std::vector <float>();
             *(v.f_l)=*(kv.v.f_l);
           }
           else v.f_l=NULL;
          break;
         case USHRT_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->usi=(*it)->usi;
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case ULONG_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->uli=(*it)->uli;
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case ULONGLONG_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->ulli=(*it)->ulli;
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case LONGLONG_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->lli=(*it)->lli;
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case UFLOAT_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->f=(*it)->f;
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case DTDES_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->dtd=(*it)->dtd;
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case DATASET_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->ds.headerfile=new std::string(*((*it)->ds.headerfile));
                ta->ds.datafile=new std::string(*((*it)->ds.datafile));
                ta->ds.acquisition_number=(*it)->ds.acquisition_number;
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case ASCII_A:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->ascii=new std::string(*((*it)->ascii));
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case ASCII_AL:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->ascii_list=new std::vector <std::string>();
                *(ta->ascii_list)=*((*it)->ascii_list);
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case USHRT_AL:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->usi_list=new std::vector <unsigned short int>();
                *(ta->usi_list)=*((*it)->usi_list);
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case UFLOAT_AL:
          if (kv.v.array != NULL)
           { v.array=new std::vector <tarray *>();
             for (it=kv.v.array->begin(); it != kv.v.array->end(); ++it)
              { tarray *ta;

                ta=new tarray;
                ta->f_list=new std::vector <float>();
                *(ta->f_list)=*((*it)->f_list);
                ta->unit=(*it)->unit;
                ta->index=(*it)->index;
                ta->comment=(*it)->comment;
                v.array->push_back(ta);
              }
           }
           else v.array=NULL;
          break;
         case NONE:
          break;
       }
      key=kv.key;
      unit=kv.unit;
      comment=kv.comment;
      max_lindex_key=kv.max_lindex_key;
      max_index_key=kv.max_index_key;
      type=kv.type;
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add new value and index to existing key.
    \param[in] _unit      unit of key
    \param[in] index      index of key
    \param[in] value      value of key
    \param[in] _comment   comment of key
    \param[in] filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING the closing bracket in
                                                         the value is missing
    \exception REC_ITF_HEADER_ERROR_VALUE_FORMAT         the value for the key
                                                         has the wrong format
    \exception REC_ITF_HEADER_ERROR_DATATYPE             the value for the
                                                         datatype is invalid

    Add new value and index to existing key. The key is an array of values
    or an array of lists of values.
 */
/*---------------------------------------------------------------------------*/
void KV::add(const std::string _unit, const unsigned short int index,
             std::string value, const std::string _comment,
             const std::string filename) const
 { tarray *a=NULL;
   try
   { std::string::size_type p;
     std::vector <tarray *>::iterator it;

     if (v.array == NULL) return;
     a=new tarray;
     a->unit=_unit;
     a->index=index;
     a->comment=_comment;
     switch (type)
      { case KV::ASCII_A:                                       // parse string
         a->ascii=new std::string(value);
         break;
        case KV::ASCII_AL:                             // parse list of strings
         if ((p=value.find('{')) != std::string::npos)
          { a->ascii_list=new std::vector <std::string>;
            value.erase(0, p+1);
            while ((p=value.find(',')) != std::string::npos)
             { a->ascii_list->push_back(value.substr(0, p));
               value.erase(0, p+1);
             }
            if ((p=value.find('}')) != std::string::npos)
             a->ascii_list->push_back(value.substr(0, p));
             else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                  "Parsing error in the file '#1'. The "
                                  "closing bracket in the value of the key "
                                  "'#2' is missing"
                           ".").arg(filename).arg(key+"["+toString(index)+"]");
          }
          else if (!value.empty())
                throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                "Parsing error in the file '#1'. The value for"
                                " the key '#2' has the wrong format"
                                ".").arg(filename).arg(key);
         break;
        case KV::USHRT_A:                     // parse unsigned short int value
         { unsigned long int uli;

           parseNumber(&uli, std::numeric_limits <unsigned short int>::max(),
                       key+"["+toString(index)+"]", value, filename);
           a->usi=(unsigned short int)uli;
         }
         break;
        case KV::USHRT_AL:           // parse list of unsigned short int values
         { unsigned long int uli;

           if ((p=value.find('{')) != std::string::npos)
            { a->usi_list=new std::vector <unsigned short int>;
              value.erase(0, p+1);
              while ((p=value.find(',')) != std::string::npos)
               { parseNumber(&uli,
                             std::numeric_limits <unsigned short int>::max(),
                             key+"["+toString(index)+"]", value.substr(0, p),
                             filename);
                 a->usi_list->push_back((unsigned short int)uli);
                 value.erase(0, p+1);
               }
              if ((p=value.find('}')) != std::string::npos)
               { parseNumber(&uli,
                             std::numeric_limits <unsigned short int>::max(),
                             key, value.substr(0, p), filename);
                 a->usi_list->push_back((unsigned short int)uli);
               }
               else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                    "Parsing error in the file '#1'. The "
                                    "closing bracket in the value of the key "
                                    "'#2' is missing"
                           ".").arg(filename).arg(key+"["+toString(index)+"]");
            }
            else if (!value.empty())
                  throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                "Parsing error in the file '#1'. The value for"
                                " the key '#2' has the wrong format"
                                ".").arg(filename).arg(key);
         }
         break;
        case KV::ULONG_A:                      // parse unsigned long int value
         parseNumber(&a->uli, std::numeric_limits <unsigned long int>::max(),
                     key+"["+toString(index)+"]", value, filename);
         break;
        case KV::ULONGLONG_A:             // parse unsigned long long int value
         parseNumber(&a->ulli, key+"["+toString(index)+"]", value, filename);
         break;
        case KV::LONGLONG_A:                // parse signed long long int value
         parseNumber(&a->lli, key+"["+toString(index)+"]", value, filename);
         break;
        case KV::UFLOAT_A:                        // parse unsigned float value
         parseNumber(&a->f, 0.0, std::numeric_limits <float>::max(),
                     key+"["+toString(index)+"]", value, filename);
         break;
        case KV::UFLOAT_AL:              // parse list of unsigned float values
         { float uf;

           if ((p=value.find('{')) != std::string::npos)
            { a->f_list=new std::vector <float>;
              value.erase(0, p+1);
              while ((p=value.find(',')) != std::string::npos)
               { parseNumber(&uf, 0.0f, std::numeric_limits <float>::max(),
                             key+"["+toString(index)+"]", value.substr(0, p),
                             filename);
                 a->f_list->push_back(uf);
                 value.erase(0, p+1);
               }
              if ((p=value.find('}')) != std::string::npos)
               { parseNumber(&uf, 0.0f, std::numeric_limits <float>::max(),
                             key, value.substr(0, p), filename);
                 a->f_list->push_back(uf);
               }
               else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                    "Parsing error in the file '#1'. The "
                                    "closing bracket in the value of the key "
                                    "'#2' is missing"
                           ".").arg(filename).arg(key+"["+toString(index)+"]");
            }
            else if (!value.empty())
                  throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                "Parsing error in the file '#1'. The value for"
                                " the key '#2' has the wrong format"
                                ".").arg(filename).arg(key);
         }
         break;
        case KV::DATASET_A:                         // parse data set structure
         a->ds.headerfile=NULL;
         a->ds.datafile=NULL;
         if ((p=value.find('{')) != std::string::npos)
          { value.erase(0, p+1);
            if ((p=value.find(',')) == std::string::npos)
             throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                             "Parsing error in the file '#1'. The value for"
                             " the key '#2' has the wrong format"
                             ".").arg(filename).arg(key);
            parseNumber(&a->ds.acquisition_number,
                        std::numeric_limits <unsigned long int>::max(),
                        key+"["+toString(index)+"]", value.substr(0, p),
                        filename);
            value.erase(0, p+1);
            if (value.at(0) == ' ') value.erase(0, 1);
            if ((p=value.find(',')) == std::string::npos)
             throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                             "Parsing error in the file '#1'. The value for"
                             " the key '#2' has the wrong format"
                             ".").arg(filename).arg(key);
            a->ds.headerfile=new std::string(value.substr(0, p));
            value.erase(0, p+1);
            if (value.at(0) == ' ') value.erase(0, 1);
            if ((p=value.find('}')) != std::string::npos)
             a->ds.datafile=new std::string(value.substr(0, p));
             else throw Exception(REC_ITF_HEADER_ERROR_VALUEBRACKET_MISSING,
                                  "Parsing error in the file '#1'. The "
                                  "closing bracket in the value of the key "
                                  "'#2' is missing"
                           ".").arg(filename).arg(key+"["+toString(index)+"]");
          }
          else if (!value.empty())
                throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                                "Parsing error in the file '#1'. The value for"
                                " the key '#2' has the wrong format"
                                ".").arg(filename).arg(key);
         break;
        case KV::DTDES_A:                        // parse data type description
         std::transform(value.begin(), value.end(), value.begin(), tolower);
         if (value == "corrected prompts") a->dtd=CORRECTED_PROMPTS;
          else if (value == "prompts") a->dtd=PROMPTS;
          else if (value == "randoms") a->dtd=RANDOMS;
          else throw Exception(REC_ITF_HEADER_ERROR_DATATYPE,
                           "Parsing error in the file '#1'. The value for the "
                           "key '#2' is invalid. Allowed values are "
                           "\"corrected prompts\", \"prompts\" and \"randoms\""
                           ".").arg(filename).arg(key+"["+toString(index)+"]");
         break;
        default:
         if (a != NULL) { delete a;
                          a=NULL;
                        }
         return;
      }
                                              // sort value into existing array
     for (it=v.array->begin(); it != v.array->end(); ++it)
      if ((*it)->index > index) { v.array->insert(it, a);
                                  return;
                                }
     v.array->push_back(a);                     // append value at end of array
   }
   catch (...)
    { if (a != NULL) delete a;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Check if a date is valid.
    \param[in] filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_GREGORIAN    the date is before 1752/09/14
    \exception REC_ITF_HEADER_ERROR_DATE_INVALID the date is invalid

    Check if a date is valid. This code works only with Gregorian dates, i.e.
    1752/09/14 or later.
 */
/*---------------------------------------------------------------------------*/
void KV::checkDate(const std::string filename) const
 { const unsigned short int monthDays[]=
                              {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
   bool leapyear;
                         // is date before introduction of Gregorian calender ?
   if ((v.date.year < 1752) ||
       ((v.date.year == 1752) &&
        ((v.date.month < 9) || ((v.date.month == 9) && (v.date.day < 14)))))
    throw Exception(REC_ITF_HEADER_ERROR_GREGORIAN,
                    "Parsing error in the file '#1'. The date in the key '#2' "
                    "is invalid. The Gregorian calender started at 1752/09/14"
                    ".").arg(filename).arg(key);
   if ((v.date.month < 1) || (v.date.month > 12))           // is month valid ?
    throw Exception(REC_ITF_HEADER_ERROR_DATE_INVALID,
                    "Parsing error in the file '#1'. The date in the key '#2' "
                    "is invalid.").arg(filename).arg(key);
   leapyear=(v.date.year % 4 == 0) && (v.date.year % 100 != 0) ||
            (v.date.year % 400 == 0);                     // is it a leapyear ?
                                            // 29th of february in a leapyear ?
   if ((v.date.day == 29) && (v.date.month == 2) && leapyear) return;
   if ((v.date.day < 1) ||
       (v.date.day > monthDays[v.date.month-1]))              // is day valid ?
    throw Exception(REC_ITF_HEADER_ERROR_DATE_INVALID,
                    "Parsing error in the file '#1'. The date in the key '#2' "
                    "is invalid.").arg(filename).arg(key);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Convert value into string.
    \param[in] value   value
    \return string

    Convert value into a string.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
std::string KV::convertToString(const T value)
 { std::string str;

   str=toString(value);
   if ((typeid(T) == typeid(float)) &&
       ((float)*(float *)&value ==
        (float)(unsigned long int)*(unsigned long int *)&value))
    return(str+".0");
   return(str);
 }

#ifndef _KV_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Convert date-value into string.
    \param[in] date   date-value
    \return string

    Convert date-value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const TIMEDATE::tdate date)
 { return(toStringZero(date.year, 4)+":"+toStringZero(date.month, 2)+":"+
          toStringZero(date.day, 2));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert time-value into string.
    \param[in] time   time-value
    \return string

    Convert time-value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const TIMEDATE::ttime t)
 { return(toStringZero(t.hour, 2)+":"+toStringZero(t.minute, 2)+":"+
          toStringZero(t.second, 2));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert sex-value into string.
    \param[in] sex   sex-value
    \return string

    Convert sex-value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tsex sex)
 { switch (sex)
    { case MALE:
       return("m");
      case FEMALE:
       return("f");
      case UNKNOWN_SEX:
      default:
       return("unknown");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert patient-orientation-value into string.
    \param[in] po   patient orientation
    \return string

    Convert patient-orientation-value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tpatient_orientation po)
 { switch (po)
    { case FFP:
       return("FFP");
      case HFP:
       return("HFP");
      case FFS:
       return("FFS");
      case HFS:
       return("HFS");
      case FFDR:
       return("FFDR");
      case HFDR:
       return("HFDR");
      case FFDL:
       return("FFDL");
      case HFDL:
      default:
       return("HFDL");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert scan-direction-value into string.
    \param[in] dir   scan-direction
    \return string

    Convert scan-direction-value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tscan_direction dir)
 { switch (dir)
    { case SCAN_IN:
       return("in");
      case SCAN_OUT:
      default:
       return("out");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert PET data type value into string.
    \param[in] pdt   PET data type value
    \return string

    Convert PET data type value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tpet_data_type pdt)
 { switch (pdt)
    { case EMISSION:
       return("emission");
      case TRANSMISSION:
       return("transmission");
      case EMISSION_TRANSMISSION:
       return("simultaneous emission + transmission");
      case IMAGE:
       return("image");
      case NORMALIZATION:
      default:
       return("normalization");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert number-format-value into string.
    \param[in] nufo   number-format-value
    \return string

    Convert number-format-value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tnumber_format nufo)
 { switch (nufo)
    { case SIGNED_INT_NF:
       return("signed integer");
      case FLOAT_NF:
      default:
       return("float");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert data type description value into string.
    \param[in] dtd   data type description value
    \return string

    Convert data type description value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tdata_type_description dtd)
 { switch (dtd)
    { case CORRECTED_PROMPTS:
       return("corrected prompts");
      case PROMPTS:
       return("prompts");
      case RANDOMS:
      default:
       return("randoms");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert endianess value into string.
    \param[in] endian   endianess value
    \return string

    Convert endianess value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tendian endian)
 { switch (endian)
    { case LITTLE_END:
       return("LITTLEENDIAN");
      case BIG_END:
      default:
       return("BIGENDIAN");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset value into string.
    \param[in] ds   dataset value
    \return string

    Convert dataset value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tdataset ds)
 { std::string s;

   s=convertToString(ds.acquisition_number)+",";
   if (ds.headerfile != NULL) s+=*(ds.headerfile);
   s+=",";
   if (ds.datafile != NULL) s+=*(ds.datafile);
   return(s);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert septa state value into string.
    \param[in] septa   septa state value
    \return string

    Convert septa state value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tsepta septa)
 { switch (septa)
    { case NOSEPTA:
       return("none");
      case EXTENDED:
       return("extended");
      case RETRACTED:
      default:
       return("retracted");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert detector motion value into string.
    \param[in] detmot   detector motion value
    \return string

    Convert detector motion value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tdetector_motion detmot)
 { switch (detmot)
    { case NOMOTION:
       return("none");
      case STEPSHOOT:
       return("step and shoot");
      case WOBBLE:
       return("wobble");
      case CONTINUOUS:
       return("continuous");
      case CLAMSHELL:
      default:
       return("clamshell");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert exam type value into string.
    \param[in] examtype   exam type value
    \return string

    Convert exam type value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const texamtype examtype)
 { switch (examtype)
    { case STATIC:
       return("static");
      case DYNAMIC:
       return("dynamic");
      case GATED:
       return("gated");
      case WHOLEBODY:
      default:
       return("wholebody");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert acquisition condition value into string.
    \param[in] acqcond   acquisition condition value
    \return string

    Convert acquisition conditiion value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tacqcondition acqcond)
 { switch (acqcond)
    { case CND_CNTS:
       return("cnts");
      case CND_DENS:
       return("dens");
      case CND_RDD:
       return("rdd");
      case CND_MANU:
       return("manu");
      case CND_OVFL:
       return("ovfl");
      case CND_TIME:
       return("time");
      case CND_TRIG:
      default:
       return("trig");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert scanner type value into string.
    \param[in] sct   scanner type value
    \return string

    Convert scanner type value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tscanner_type sct)
 { switch (sct)
    { case CYLINDRICAL_RING:
       return("cylindrical ring");
      case HEX_RING:
       return("hexagonal ring");
      case MULTIPLE_PLANAR:
      default:
       return("multiple planar");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert decay correction information into string.
    \param[in] dec   decay correction information
    \return string

    Convert scanner type value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tdecay_correction dec)
 { switch (dec)
    { case NODEC:
       return("none");
      case START:
       return("start");
      case ADMIN:
      default:
       return("admin");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert slice orientation value into string.
    \param[in] so   slice orientation
    \return string

    Convert slice orientation value into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tslice_orientation so)
 { switch (so)
    { case TRANSVERSE:
       return("transverse");
      case CORONAL:
       return("coronal");
      case SAGITAL:
      default:
       return("sagital");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert randoms correction information into string.
    \param[in] rc   randoms correction information
    \return string

    Convert randoms correction information into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const trandom_correction rc)
 { switch (rc)
    { case NORAN:
       return("none");
      case DLYD:
       return("dlyd");
      case RCSING:
       return("sing");
      case PROC:
      default:
       return("proc");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert process status information into string.
    \param[in] ps   convert process status
    \return string

    Convert process status information into string.
 */
/*---------------------------------------------------------------------------*/
std::string KV::convertToString(const tprocess_status ps)
 { switch (ps)
    { case RECONSTRUCTED:
      default:
       return("reconstructed");
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request unit of key.
    \return unit of key

    Request unit of key.
 */
/*---------------------------------------------------------------------------*/
std::string KV::getUnit() const
 { return(unit);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Request value of key.
    \return value of key

    Request value of key. The value is returned as a parameter and as return
    value.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
T KV::getValue() const
 { T vnull=T();
 
   switch (type)
    { case KV::ASCII:
      case KV::RELFILENAME:
       if (typeid(T) != typeid(std::string)) return(vnull);
       if (v.ascii == NULL) return(vnull);
       return(*(T *)v.ascii);
      case KV::USHRT:
       if (typeid(T) != typeid(unsigned short int)) return(vnull);
       return(*(T *)&v.usi);
      case KV::ULONG:
       if (typeid(T) != typeid(unsigned long int)) return(vnull);
       return(*(T *)&v.uli);
      case KV::LONG:
       if (typeid(T) != typeid(signed long int)) return(vnull);
       return(*(T *)&v.sli);
      case KV::ULONGLONG:
       if (typeid(T) != typeid(uint64)) return(vnull);
       return(*(T *)&v.ulli);
      case KV::LONGLONG:
       if (typeid(T) != typeid(int64)) return(vnull);
       return(*(T *)&v.lli);
      case KV::UFLOAT:
      case KV::FLOAT:
       if (typeid(T) != typeid(float)) return(vnull);
       return(*(T *)&v.f);
      case KV::DATE:
       if (typeid(T) != typeid(TIMEDATE::tdate)) return(vnull);
       return(*(T *)&v.date);
      case KV::TIME:
       if (typeid(T) != typeid(TIMEDATE::ttime)) return(vnull);
       return(*(T *)&v.time);
      case KV::SEX:
       if (typeid(T) != typeid(tsex)) return(vnull);
       return(*(T *)&v.sex);
      case KV::ORIENTATION:
       if (typeid(T) != typeid(tpatient_orientation)) return(vnull);
       return(*(T *)&v.po);
      case KV::DIRECTION:
       if (typeid(T) != typeid(tscan_direction)) return(vnull);
       return(*(T *)&v.dir);
      case KV::PETDT:
       if (typeid(T) != typeid(tpet_data_type)) return(vnull);
       return(*(T *)&v.pdt);
      case KV::NUMFORM:
       if (typeid(T) != typeid(tnumber_format)) return(vnull);
       return(*(T *)&v.nufo);
      case KV::ENDIAN:
       if (typeid(T) != typeid(tendian)) return(vnull);
       return(*(T *)&v.endian);
      case KV::SEPTA:
       if (typeid(T) != typeid(tsepta)) return(vnull);
       return(*(T *)&v.septa);
      case KV::DETMOTION:
       if (typeid(T) != typeid(tdetector_motion)) return(vnull);
       return(*(T *)&v.detmot);
      case KV::EXAMTYPE:
       if (typeid(T) != typeid(texamtype)) return(vnull);
       return(*(T *)&v.examtype);
      case KV::ACQCOND:
       if (typeid(T) != typeid(tacqcondition)) return(vnull);
       return(*(T *)&v.acqcond);
      case KV::SCANNER_TYPE:
       if (typeid(T) != typeid(tscanner_type)) return(vnull);
       return(*(T *)&v.sct);
      case KV::DECAYCORR:
       if (typeid(T) != typeid(tdecay_correction)) return(vnull);
       return(*(T *)&v.dec);
      case KV::SL_ORIENTATION:
       if (typeid(T) != typeid(tslice_orientation)) return(vnull);
       return(*(T *)&v.so);
      case KV::RANDCORR:
       if (typeid(T) != typeid(trandom_correction)) return(vnull);
       return(*(T *)&v.rc);
      case KV::PROCSTAT:
       if (typeid(T) != typeid(tprocess_status)) return(vnull);
       return(*(T *)&v.ps);
      default:
       return(vnull);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request value of key with array or list of values.
    \param[in] a_idx   index of array
    \return value of key

    Request value of key with array or list of values. The value is returned as
    a parameter and as return value.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
T KV::getValue(const unsigned short int a_idx) const
 { T vnull=T();
 
   switch (type)
    { case KV::ASCII_A:
       if (typeid(T) != typeid(std::string)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)v.array->at(a_idx)->ascii);
       return(vnull);
      case KV::ASCII_L:
       if (typeid(T) != typeid(std::string)) return(vnull);
       if (v.ascii_l != NULL)
        if (v.ascii_l->size() > a_idx) return(*(T *)&(v.ascii_l->at(a_idx)));
       return(vnull);
      case KV::USHRT_L:
       if (typeid(T) != typeid(unsigned short int)) return(vnull);
       if (v.usi_l != NULL)
        if (v.usi_l->size() > a_idx) return(*(T *)&(v.usi_l->at(a_idx)));
       return(vnull);
      case KV::USHRT_A:
       if (typeid(T) != typeid(unsigned short int)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)&(v.array->at(a_idx)->usi));
       return(vnull);
      case KV::ULONG_A:
       if (typeid(T) != typeid(unsigned long int)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)&(v.array->at(a_idx)->uli));
       return(vnull);
      case KV::ULONG_L:
       if (typeid(T) != typeid(unsigned long int)) return(vnull);
       if (v.uli_l != NULL)
        if (v.uli_l->size() > a_idx) return(*(T *)&(v.uli_l->at(a_idx)));
       return(vnull);
      case KV::ULONGLONG_A:
       if (typeid(T) != typeid(uint64)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)&(v.array->at(a_idx)->ulli));
       return(vnull);
      case KV::LONGLONG_A:
       if (typeid(T) != typeid(int64)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)&(v.array->at(a_idx)->lli));
       return(vnull);
      case KV::UFLOAT_L:
      case KV::FLOAT_L:
       if (typeid(T) != typeid(float)) return(vnull);
       if (v.f_l != NULL)
        if (v.f_l->size() > a_idx) return(*(T *)&(v.f_l->at(a_idx)));
         else { float mx;

                mx=std::numeric_limits <float>::max();
                return(*(T *)&mx);
              }
       return(vnull);
      case KV::UFLOAT_A:
       if (typeid(T) != typeid(float)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)&(v.array->at(a_idx)->f));
       return(vnull);
      case KV::DATASET_A:
       if (typeid(T) != typeid(tdataset)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)&(v.array->at(a_idx)->ds));
       return(vnull);
      case KV::DTDES_A:
       if (typeid(T) != typeid(tdata_type_description)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx) return(*(T *)&(v.array->at(a_idx)->dtd));
       return(vnull);
      default:                                                  // can't happen
       return(vnull);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request value of key with array of list of values.
    \param[in] a_idx   index of array
    \param[in] l_idx   index of list
    \return value of key

    Request value of key with array of list of values. The value is returned as
    a parameter and as return value.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
T KV::getValue(const unsigned short int a_idx,
               const unsigned short int l_idx) const
 { T vnull=T();
 
   switch (type)
    { case KV::ASCII_AL:
       if (typeid(T) != typeid(std::string)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx)
         if (v.array->at(a_idx)->ascii_list != NULL)
          if (v.array->at(a_idx)->ascii_list->size() > l_idx)
           return(*(T *)&(v.array->at(a_idx)->ascii_list->at(l_idx)));
       return(vnull);
      case KV::USHRT_AL:
       if (typeid(T) != typeid(unsigned short int)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx)
         if (v.array->at(a_idx)->usi_list != NULL)
          if (v.array->at(a_idx)->usi_list->size() > l_idx)
           return(*(T *)&(v.array->at(a_idx)->usi_list->at(l_idx)));
       return(vnull);
      case KV::UFLOAT_AL:
       if (typeid(T) != typeid(float)) return(vnull);
       if (v.array != NULL)
        if (v.array->size() > a_idx)
         if (v.array->at(a_idx)->f_list != NULL)
          if (v.array->at(a_idx)->f_list->size() > l_idx)
           return(*(T *)&(v.array->at(a_idx)->f_list->at(l_idx)));
       return(vnull);
      default:
       return(vnull);
    }
 }

#ifndef _KV_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Request name of key.
    \return name of key

    Request name of key.
 */
/*---------------------------------------------------------------------------*/
std::string KV::Key() const
 { return(key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request name of key which specifies maximum index of array.
    \return name of key

    Request name of key which specifies maximum index of array.
 */
/*---------------------------------------------------------------------------*/
std::string KV::MaxIndexKey() const
 { return(max_index_key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request name of key which specifies maximum index of list in array.
    \return name of key

    Request name of key which specifies maximum index of list in array.
 */
/*---------------------------------------------------------------------------*/
std::string KV::MaxLIndexKey() const
 { return(max_lindex_key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse unsigned long long int value from string.
    \param[out] val        unsigned long long int value
    \param[in]  _key       name of key
    \param[in]  value      string with value
    \param[in]  filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL value too small
    \exception REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE value too large
    \exception REC_ITF_HEADER_ERROR_VALUE_FORMAT    format of value is wrong

    Parse unsigned long long int value from string.
 */
/*---------------------------------------------------------------------------*/
void KV::parseNumber(uint64 * const val, const std::string _key,
                     std::string value, const std::string filename) const
 { uint64 old;

   *val=0;
   if (value.empty()) return;
   if (value.at(0) == '-')
    throw Exception(REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL,
                    "Parsing error in the file '#1'. The value for the key "
                    "'#2' is too small.").arg(filename).arg(_key);
   while ((value.at(0) >= '0') && (value.at(0) <= '9'))
    { old=*val;
      *val=(*val*10)+(value.at(0)-'0');
      if (*val/10 != old)                                         // overflow ?
       throw Exception(REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE,
                       "Parsing error in the file '#1'. The value for the key "
                       "'#2' is too large.").arg(filename).arg(_key);
      value.erase(0, 1);
      if (value.empty()) return;
    }
   throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                   "Parsing error in the file '#1'. The value for the key '#2'"
                   " has the wrong format.").arg(filename).arg(_key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse signed long long int value from string.
    \param[out] val        signed long long int value
    \param[in]  _key       name of key
    \param[in]  value      string with value
    \param[in]  filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL value too small
    \exception REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE value too large
    \exception REC_ITF_HEADER_ERROR_VALUE_FORMAT    format of value is wrong

    Parse signed long long int value from string.
 */
/*---------------------------------------------------------------------------*/
void KV::parseNumber(int64 * const val, const std::string _key,
                     std::string value, const std::string filename) const
 { int64 old;
   bool negative=false;

   *val=0;
   if (value.empty()) return;
   if (value.at(0) == '-') { negative=true;
                             value.erase(0, 1);
                           }
   while ((value.at(0) >= '0') && (value.at(0) <= '9'))
    { old=*val;
      *val=(*val*10)+(value.at(0)-'0');
      if (*val/10 != old)                                         // overflow ?
       if (negative)
        throw Exception(REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL,
                       "Parsing error in the file '#1'. The value for the key "
                       "'#2' is too small.").arg(filename).arg(_key);
        else throw Exception(REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE,
                       "Parsing error in the file '#1'. The value for the key "
                       "'#2' is too large.").arg(filename).arg(_key);
      value.erase(0, 1);
      if (value.empty())
       { if (negative) *val=-*val;
         return;
       }
    }
   throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                   "Parsing error in the file '#1'. The value for the key '#2'"
                   " has the wrong format.").arg(filename).arg(_key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse unsigned long int value from string.
    \param[out] val        unsigned long int value
    \param[in]  maxv       maximum allowed value
    \param[in]  key        name of key
    \param[in]  value      string with value
    \param[in]  filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE value too large
    \exception REC_ITF_HEADER_ERROR_VALUE_FORMAT    format of value is wrong

    Parse unsigned long int value from string.
 */
/*---------------------------------------------------------------------------*/
void KV::parseNumber(unsigned long int * const val,
                     const unsigned long int maxv, const std::string key,
                     const std::string value, const std::string filename)
 { char *endptr;

   *val=strtoul(value.c_str(), &endptr, 10);
   if (*endptr != '\0')
    throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                    "Parsing error in the file '#1'. The value for the key "
                    "'#2' has the wrong format.").arg(filename).arg(key);
   if (((*val == std::numeric_limits <unsigned long int>::max()) &&
        (errno == ERANGE)) || (*val > maxv))
    throw Exception(REC_ITF_HEADER_ERROR_VALUE_TOO_LARGE,
                    "Parsing error in the file '#1'. The value for the key "
                    "'#2' is too large.").arg(filename).arg(key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse signed long int value from string.
    \param[out] val        signed long int value
    \param[in]  minv       minimum value allowed
    \param[in]  maxv       maximum value allowed
    \param[in]  _key       name of key
    \param[in]  value      string with value
    \param[in]  filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_RANGE  value too large or too small
    \exception REC_ITF_HEADER_ERROR_VALUE_FORMAT format of value is wrong

    Parse signed long int value from string.
 */
/*---------------------------------------------------------------------------*/
void KV::parseNumber(signed long int * const val, const signed long int minv,
                     const signed long int maxv, const std::string _key,
                     const std::string value, const std::string filename) const
 { char *endptr;

   *val=strtol(value.c_str(), &endptr, 10);
   if (*endptr != '\0')
    throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                    "Parsing error in the file '#1'. The value for the key "
                    "'#2' has the wrong format.").arg(filename).arg(_key);
   if ((((*val == std::numeric_limits <signed long int>::max()) ||
         (*val == std::numeric_limits <signed long int>::min())) &&
        (errno == ERANGE)) || (*val > maxv) || (*val < minv))
    throw Exception(REC_ITF_HEADER_ERROR_VALUE_RANGE,
                    "Parsing error in the file '#1'. The value for the key "
                    "'#2' is too large or too small.").arg(filename).arg(_key);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse float value from string.
    \param[out] val        float value
    \param[in]  minv       minimum value allowed
    \param[in]  maxv       maximum value allowed
    \param[in]  _key       name of key
    \param[in]  value      string with value
    \param[in]  filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_RANGE  value too large or too small
    \exception REC_ITF_HEADER_ERROR_VALUE_FORMAT format of value is wrong

    Parse float value from string.
 */
/*---------------------------------------------------------------------------*/
void KV::parseNumber(float * const val, const float minv, const float maxv,
                     const std::string _key, const std::string value,
                     const std::string filename) const
 { char *endptr;
   double d;

   d=strtod(value.c_str(), &endptr);
   if (*endptr != '\0')
    throw Exception(REC_ITF_HEADER_ERROR_VALUE_FORMAT,
                    "Parsing error in the file '#1'. The value for the key "
                    "'#2' has the wrong format.").arg(filename).arg(_key);
   if ((((d == std::numeric_limits <double>::max()) ||
         (d == -std::numeric_limits <double>::max())) &&
        (errno == ERANGE)) || (d > maxv) || (d < minv))
    throw Exception(REC_ITF_HEADER_ERROR_VALUE_RANGE,
                    "Parsing error in the file '#1'. The value for the key "
                    "'#2' is too large or too small.").arg(filename).arg(_key);
   *val=(float)d;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print key with unit, index, value and comment.
    \param[in,out] stream in which data is printed

    Print key with unit, index, value and comment.
 */
/*---------------------------------------------------------------------------*/
void KV::print(std::ostream *ostr) const
 { std::string s;

   if (key.empty()) s=std::string();
    else { s=key;
           if (type == KV::TIME)
            { s+=" ("+unit+" GMT";
              if ((v.time.gmt_offset_h < 0) || (v.time.gmt_offset_m < 0))
               s+="-";
               else s+="+";
              if (abs(v.time.gmt_offset_h) < 10) s+="0";
              s+=convertToString(abs(v.time.gmt_offset_h))+":";
              if (abs(v.time.gmt_offset_m) < 10) s+="0";
              s+=convertToString(abs(v.time.gmt_offset_m))+")";
            }
            else if (!unit.empty()) s+=" ("+unit+")";
           s+=":=";
         }
   switch (type)
    { case KV::ASCII:
      case KV::RELFILENAME:
       if (v.ascii != NULL) s+=*(v.ascii);
       break;
      case KV::ASCII_L:
       if (v.ascii_l != NULL)
        if (v.ascii_l->size() > 0)
         { std::vector <std::string>::const_iterator it;

           s+="{";
           for (it=v.ascii_l->begin(); it != v.ascii_l->end(); ++it)
            s+=convertToString(*it)+",";
           s=s.substr(0, s.length()-1)+"}";
         }
       break;
      case KV::ASCII_AL:
       if (v.array != NULL)
        { std::vector <tarray *>::const_iterator it1;

          for (it1=v.array->begin(); it1 != v.array->end(); ++it1)
           { std::vector <std::string>::const_iterator it2;

             s=key;
             if (!(*it1)->unit.empty()) s+=" ("+(*it1)->unit+") ";
             s+="["+toString((*it1)->index)+"]:=";
             if ((*it1)->ascii_list != NULL)
              if ((*it1)->ascii_list->size() > 0)
               { s+="{";
                 for (it2=(*it1)->ascii_list->begin();
                      it2 != (*it1)->ascii_list->end(); ++it2)
                  s+=*it2+",";
                 s=s.substr(0, s.length()-1)+"}";
               }
             printLine(ostr, s, (*it1)->comment);
           }
        }
       return;
      case KV::USHRT:
       s+=convertToString(v.usi);
       break;
      case KV::USHRT_L:
       if (v.usi_l != NULL)
        if (v.usi_l->size() > 0)
         { std::vector <unsigned short int>::const_iterator it;

           s+="{";
           for (it=v.usi_l->begin(); it != v.usi_l->end(); ++it)
            s+=convertToString(*it)+",";
           s=s.substr(0, s.length()-1)+"}";
         }
       break;
      case KV::USHRT_AL:
       if (v.array != NULL)
        { std::vector <tarray *>::const_iterator it1;

          for (it1=v.array->begin(); it1 != v.array->end(); ++it1)
           { std::vector <unsigned short int>::const_iterator it2;

             s=key;
             if (!(*it1)->unit.empty()) s+=" ("+(*it1)->unit+") ";
             s+="["+toString((*it1)->index)+"]:=";
             if ((*it1)->usi_list != NULL)
              if ((*it1)->usi_list->size() > 0)
               { s+="{";
                 for (it2=(*it1)->usi_list->begin();
                      it2 != (*it1)->usi_list->end(); ++it2)
                  s+=toString(*it2)+",";
                 s=s.substr(0, s.length()-1)+"}";
               }
             printLine(ostr, s, (*it1)->comment);
           }
        }
       return;
      case KV::ULONG:
       s+=convertToString(v.uli);
       break;
      case KV::LONG:
       s+=convertToString(v.sli);
       break;
      case KV::ULONG_L:
       if (v.uli_l != NULL)
        if (v.uli_l->size() > 0)
         { std::vector <unsigned long int>::const_iterator it;

           s+="{";
           for (it=v.uli_l->begin(); it != v.uli_l->end(); ++it)
            s+=convertToString(*it)+",";
           s=s.substr(0, s.length()-1)+"}";
         }
       break;
      case KV::ULONGLONG:
       s+=convertToString(v.ulli);
       break;
      case KV::LONGLONG:
       s+=convertToString(v.lli);
       break;
      case KV::UFLOAT:
      case KV::FLOAT:
       s+=convertToString(v.f);
       break;
      case KV::UFLOAT_L:
      case KV::FLOAT_L:
       if (v.f_l != NULL)
        if (v.f_l->size() > 0)
         { std::vector <float>::const_iterator it;

           s+="{";
           for (it=v.f_l->begin(); it != v.f_l->end(); ++it)
            s+=convertToString(*it)+",";
           s=s.substr(0, s.length()-1)+"}";
         }
       break;
      case KV::UFLOAT_AL:
       if (v.array != NULL)
        { std::vector <tarray *>::const_iterator it1;

          for (it1=v.array->begin(); it1 != v.array->end(); ++it1)
           { std::vector <float>::const_iterator it2;

             s=key;
             if (!(*it1)->unit.empty()) s+=" ("+(*it1)->unit+") ";
             s+="["+toString((*it1)->index)+"]:=";
             if ((*it1)->f_list != NULL)
              if ((*it1)->f_list->size() > 0)
               { s+="{";
                 for (it2=(*it1)->f_list->begin();
                      it2 != (*it1)->f_list->end(); ++it2)
                  s+=toString(*it2)+",";
                 s=s.substr(0, s.length()-1)+"}";
               }
             printLine(ostr, s, (*it1)->comment);
           }
        }
       return;
      case KV::DATE:
       s+=convertToString(v.date);
       break;
      case KV::TIME:
       s+=convertToString(v.time);
       break;
      case KV::SEX:
       s+=convertToString(v.sex);
       break;
      case KV::ORIENTATION:
       s+=convertToString(v.po);
       break;
      case KV::DIRECTION:
       s+=convertToString(v.dir);
       break;
      case KV::PETDT:
       s+=convertToString(v.pdt);
       break;
      case KV::NUMFORM:
       s+=convertToString(v.nufo);
       break;
      case KV::ENDIAN:
       s+=convertToString(v.endian);
       break;
      case KV::SEPTA:
       s+=convertToString(v.septa);
       break;
      case KV::DETMOTION:
       s+=convertToString(v.detmot);
       break;
      case KV::EXAMTYPE:
       s+=convertToString(v.examtype);
       break;
      case KV::ACQCOND:
       s+=convertToString(v.acqcond);
       break;
      case KV::SCANNER_TYPE:
       s+=convertToString(v.sct);
       break;
      case KV::DECAYCORR:
       s+=convertToString(v.dec);
       break;
      case KV::SL_ORIENTATION:
       s+=convertToString(v.so);
       break;
      case KV::RANDCORR:
       s+=convertToString(v.rc);
       break;
      case KV::PROCSTAT:
       s+=convertToString(v.ps);
       break;
      case KV::ASCII_A:
      case KV::USHRT_A:
      case KV::ULONG_A:
      case KV::ULONGLONG_A:
      case KV::LONGLONG_A:
      case KV::UFLOAT_A:
      case KV::DATASET_A:
      case KV::DTDES_A:
       if (v.array != NULL)
        { std::vector <tarray *>::const_iterator it;

          if (v.array != NULL)
           for (it=v.array->begin(); it != v.array->end(); ++it)
            { s=key;
              if (!(*it)->unit.empty()) s+=" ("+(*it)->unit+") ";
              s+="["+toString((*it)->index)+"]:=";
              switch (type)
               { case KV::ASCII_A:
                  s+=*(*it)->ascii;
                  break;
                 case KV::USHRT_A:
                  s+=convertToString((*it)->usi);
                  break;
                 case KV::ULONG_A:
                  s+=convertToString((*it)->uli);
                  break;
                 case KV::ULONGLONG_A:
                  s+=convertToString((*it)->ulli);
                  break;
                 case KV::LONGLONG_A:
                  s+=convertToString((*it)->lli);
                  break;
                 case KV::UFLOAT_A:
                  s+=convertToString((*it)->f);
                  break;
                 case KV::DATASET_A:
                  s+="{"+convertToString((*it)->ds)+"}";
                  break;
                 case KV::DTDES_A:
                  s+=convertToString((*it)->dtd);
                  break;
                 default:
                  break;
               }
              printLine(ostr, s, (*it)->comment);
            }
        }
       return;
      default:
       break;
    }
   printLine(ostr, s, comment);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Attach comment to string and print to stream.
    \param[in,out] ostr       stream for output
    \param[in]     s          string
    \param[in]     _comment   comment

    Attach comment to string and print to stream.
 */
/*---------------------------------------------------------------------------*/
void KV::printLine(std::ostream * const ostr, std::string s,
                   const std::string _comment) const
 { if (!comment.empty())
    { while (s.length()+_comment.length() < 76) s+=" ";
      s+=" ; "+_comment;
    }
   *ostr << s << "\n";
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove list value from array of lists.
    \param[in] a_idx      index of array
    \param[in] l_idx      index of list
    \param[in] filename   name of Interfile header
    \return array empty ?
    \exception REC_ITF_HEADER_ERROR_REMOVE_KEY can't remove the key

    Remove list value from array of lists. If the list is empty, the array
    entry is removed. If the array is empty, the key is removed.
 */
/*---------------------------------------------------------------------------*/
bool KV::removeValue(const unsigned short int a_idx,
                     const unsigned short int l_idx,
                     const std::string filename)
 { std::vector <tarray *>::iterator it;
   unsigned short int idx=0;

   switch (type)
    { case KV::ASCII_AL:
       { std::vector <std::string>::iterator it2;

         if (v.array != NULL)
          for (it=v.array->begin(); it != v.array->end(); ++it)
           if ((*it)->index == a_idx+1)
            { if ((*it)->ascii_list != NULL)
              for (it2=(*it)->ascii_list->begin();
                   it2 != (*it)->ascii_list->end();
                   ++it2,
                   idx++)
               if (idx == l_idx)
                { (*it)->ascii_list->erase(it2);
                  if ((*it)->ascii_list->size() == 0)
                   { delete (*it)->ascii_list;
                     delete *it;
                     v.array->erase(it);
                     if (v.array->size() == 0) { delete v.array;
                                                 v.array=NULL;
                                                 return(true);
                                               }
                     return(false);
                   }
                }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2][#3] from the file '#4'"
                       ".").arg(key).arg(a_idx).arg(l_idx).arg(filename);
      case KV::USHRT_AL:
       { std::vector <unsigned short int>::iterator it2;

         if (v.array != NULL)
          for (it=v.array->begin(); it != v.array->end(); ++it)
           if ((*it)->index == a_idx+1)
            { if ((*it)->usi_list != NULL)
              for (it2=(*it)->usi_list->begin();
                   it2 != (*it)->usi_list->end();
                   ++it2,
                   idx++)
               if (idx == l_idx)
                { (*it)->usi_list->erase(it2);
                  if ((*it)->usi_list->size() == 0)
                   { delete (*it)->usi_list;
                     delete *it;
                     v.array->erase(it);
                     if (v.array->size() == 0) { delete v.array;
                                                 v.array=NULL;
                                                 return(true);
                                               }
                     return(false);
                   }
                }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2][#3] from the file '#4'"
                       ".").arg(key).arg(a_idx).arg(l_idx).arg(filename);
      case KV::UFLOAT_AL:
       { std::vector <float>::iterator it2;

         if (v.array != NULL)
          for (it=v.array->begin(); it != v.array->end(); ++it)
           if ((*it)->index == a_idx+1)
            { if ((*it)->f_list != NULL)
              for (it2=(*it)->f_list->begin();
                   it2 != (*it)->f_list->end();
                   ++it2,
                   idx++)
               if (idx == l_idx)
                { (*it)->f_list->erase(it2);
                  if ((*it)->f_list->size() == 0)
                   { delete (*it)->f_list;
                     delete *it;
                     v.array->erase(it);
                     if (v.array->size() == 0) { delete v.array;
                                                 v.array=NULL;
                                                 return(true);
                                               }
                     return(false);
                   }
                }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2][#3] from the file '#4'"
                       ".").arg(key).arg(a_idx).arg(l_idx).arg(filename);
      default:
       break;
    }
   return(false);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove value from array.
    \param[in] a_idx      index of array
    \param[in] filename   name of Interfile header
    \return array empty ?
    \exception REC_ITF_HEADER_ERROR_REMOVE_KEY can't remove the key

    Remove value from array. If the value is a list, the whole list is removed.
    If the array is empty, the key is removed.
 */
/*---------------------------------------------------------------------------*/
bool KV::removeValue(const unsigned short int a_idx,
                     const std::string filename)
 { switch (type)
    { case KV::ASCII_L:
       { std::vector <std::string>::iterator it;
         unsigned short int idx=0;

         if (v.ascii_l != NULL)
          for (it=v.ascii_l->begin(); it != v.ascii_l->end(); ++it,
               idx++)
           if (idx == a_idx)
            { v.ascii_l->erase(it);
              if (v.ascii_l->size() == 0) { delete v.ascii_l;
                                            v.ascii_l=NULL;
                                            return(true);
                                          }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2] from the file '#3'"
                       ".").arg(key).arg(a_idx).arg(filename);
      case KV::USHRT_L:
       { std::vector <unsigned short int>::iterator it;
         unsigned short int idx=0;

         if (v.usi_l != NULL)
          for (it=v.usi_l->begin(); it != v.usi_l->end(); ++it,
               idx++)
           if (idx == a_idx)
            { v.usi_l->erase(it);
              if (v.usi_l->size() == 0) { delete v.usi_l;
                                          v.usi_l=NULL;
                                          return(true);
                                        }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2] from the file '#3'"
                       ".").arg(key).arg(a_idx).arg(filename);
      case KV::ASCII_A:
      case KV::ASCII_AL:
      case KV::USHRT_A:
      case KV::USHRT_AL:
      case KV::ULONG_A:
      case KV::ULONGLONG_A:
      case KV::LONGLONG_A:
      case KV::UFLOAT_A:
      case KV::UFLOAT_AL:
      case KV::DATASET_A:
      case KV::DTDES_A:
       { std::vector <tarray *>::iterator it;

         if (v.array != NULL)
          for (it=v.array->begin(); it != v.array->end(); ++it)
           if ((*it)->index == a_idx+1)
            { if (type == KV::ASCII_A) delete (*it)->ascii;
               else if (type == KV::ASCII_AL) delete (*it)->ascii_list;
               else if (type == KV::USHRT_AL) delete (*it)->usi_list;
               else if (type == KV::UFLOAT_AL) delete (*it)->f_list;
               else if (type == KV::DATASET_A) { delete (*it)->ds.headerfile;
                                                 delete (*it)->ds.datafile;
                                               }
              delete *it;
              v.array->erase(it);
              if (v.array->size() == 0) { delete v.array;
                                          v.array=NULL;
                                          return(true);
                                        }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2] from the file '#3'"
                       ".").arg(key).arg(a_idx).arg(filename);
      case KV::ULONG_L:
       { std::vector <unsigned long int>::iterator it;
         unsigned short int idx=0;

         if (v.uli_l != NULL)
          for (it=v.uli_l->begin(); it != v.uli_l->end(); ++it,
               idx++)
           if (idx == a_idx)
            { v.uli_l->erase(it);
              if (v.uli_l->size() == 0) { delete v.uli_l;
                                          v.uli_l=NULL;
                                          return(true);
                                        }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2] from the file '#3'"
                       ".").arg(key).arg(a_idx).arg(filename);
      case KV::UFLOAT_L:
      case KV::FLOAT_L:
       { std::vector <float>::iterator it;
         unsigned short int idx=0;

         if (v.f_l != NULL)
          for (it=v.f_l->begin(); it != v.f_l->end(); ++it,
               idx++)
           if (idx == a_idx)
            { v.f_l->erase(it);
              if (v.f_l->size() == 0) { delete v.f_l;
                                        v.f_l=NULL;
                                        return(true);
                                      }
              return(false);
            }
       }
       throw Exception(REC_ITF_HEADER_ERROR_REMOVE_KEY,
                       "Can't remove the key #1[#2] from the file '#3'"
                       ".").arg(key).arg(a_idx).arg(filename);
      default:
       break;
    }
   return(false);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove all array values.

    Remove all array values.
 */
/*---------------------------------------------------------------------------*/
void KV::removeValues()
 { switch (type)
    { case KV::ASCII_L:
       if (v.ascii_l != NULL) { delete v.ascii_l;
                                v.ascii_l=NULL;
                                return;
                              }
       return;
      case KV::USHRT_L:
       if (v.usi_l != NULL) { delete v.usi_l;
                              v.usi_l=NULL;
                              return;
                            }
       return;
      case KV::ASCII_A:
      case KV::ASCII_AL:
      case KV::USHRT_A:
      case KV::USHRT_AL:
      case KV::ULONG_A:
      case KV::ULONGLONG_A:
      case KV::LONGLONG_A:
      case KV::UFLOAT_A:
      case KV::UFLOAT_AL:
      case KV::DATASET_A:
       if (v.array != NULL)
        { std::vector <tarray *>::iterator it;

          while (v.array->size() > 0)
           { it=v.array->begin();
             if (type == KV::ASCII_A) delete (*it)->ascii;
              else if (type == KV::ASCII_AL) delete (*it)->ascii_list;
              else if (type == KV::USHRT_AL) delete (*it)->usi_list;
              else if (type == KV::UFLOAT_AL) delete (*it)->f_list;
              else if (type == KV::DATASET_A) { delete (*it)->ds.headerfile;
                                                delete (*it)->ds.datafile;
                                              }
             delete *it;
             v.array->erase(it);
           }
          delete v.array;
          v.array=NULL;
        }
       return;
      case KV::ULONG_L:
       if (v.uli_l != NULL) { delete v.uli_l;
                              v.uli_l=NULL;
                              return;
                            }
       return;
      case KV::UFLOAT_L:
      case KV::FLOAT_L:
       if (v.f_l != NULL) { delete v.f_l;
                            v.f_l=NULL;
                            return;
                          }
       return;
      default:
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform semantic checks of key/value data.
    \param[in] filename    name of Interfile header
    \param[in] max_index   maximum index of array
    \exception REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG wrong number of values in
                                                    key
    \exception REC_ITF_HEADER_ERROR_MULTIPLE_KEY    key appears more than once
    \exception REC_ITF_HEADER_ERROR_INDEX_TOO_LARGE array index larger than
                                                    allowed


    Perform semantic checks of key/value data.
 */
/*---------------------------------------------------------------------------*/
void KV::semanticCheck(const std::string filename,
                       const unsigned short int max_index) const
 { switch (type)
    { case KV::ASCII_L:     // does list contain the correct number of values ?
       if (max_index < std::numeric_limits <unsigned short int>::max())
        if (v.ascii_l != NULL)
         if (v.ascii_l->size() != max_index)
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG,
                          "Parsing error in the file '#1'. The key '#2' "
                          "contains the wrong number of values"
                          ".").arg(filename).arg(key);
       break;
      case KV::USHRT_L:     // does list contain the correct number of values ?
       if (max_index < std::numeric_limits <unsigned short int>::max())
        if (v.usi_l != NULL)
         if (v.usi_l->size() != max_index)
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG,
                          "Parsing error in the file '#1'. The key '#2' "
                          "contains the wrong number of values"
                          ".").arg(filename).arg(key);
       break;
      case KV::ASCII_A:
      case KV::ASCII_AL:
      case KV::USHRT_A:
      case KV::USHRT_AL:
      case KV::ULONG_A:
      case KV::ULONGLONG_A:
      case KV::LONGLONG_A:
      case KV::UFLOAT_A:
      case KV::UFLOAT_AL:
      case KV::DATASET_A:
       if (v.array != NULL)
        if (v.array->size() > 0)
         { std::vector <tarray *>::const_iterator it_a1, it_a2;
                                        // appears array index more than once ?
           for (it_a1=v.array->begin(); it_a1 != v.array->end()-1; it_a1++)
            for (it_a2=it_a1+1; it_a2 != v.array->end(); it_a2++)
             if ((*it_a1)->index == (*it_a2)->index)
              throw Exception(REC_ITF_HEADER_ERROR_MULTIPLE_KEY,
                              "Parsing error in the file '#1'. The key '#2'"
                              " appears more than once"
                 ".").arg(filename).arg(key+"["+toString((*it_a1)->index)+"]");
                                                    // is array index too big ?
           for (it_a1=v.array->begin(); it_a1 != v.array->end(); it_a1++)
            if ((*it_a1)->index > max_index)
             throw Exception(REC_ITF_HEADER_ERROR_INDEX_TOO_LARGE,
                            "Parsing error in the file '#1'. The index of the "
                            "key '#2[#3]' is larger than specified in the key "
                            "'#4'.").arg(filename).arg(key).
                            arg((*it_a1)->index).arg(max_index_key);
         }
       break;
      case KV::ULONG_L:     // does list contain the correct number of values ?
       if (max_index < std::numeric_limits <unsigned short int>::max())
        if (v.uli_l != NULL)
         if (v.uli_l->size() != max_index)
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG,
                          "Parsing error in the file '#1'. The key '#2' "
                          "contains the wrong number of values"
                          ".").arg(filename).arg(key);
       break;
      case KV::UFLOAT_L:    // does list contain the correct number of values ?
      case KV::FLOAT_L:
       if (max_index < std::numeric_limits <unsigned short int>::max())
        if (v.f_l != NULL)
         if (v.f_l->size() != max_index)
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG,
                          "Parsing error in the file '#1'. The key '#2' "
                          "contains the wrong number of values"
                          ".").arg(filename).arg(key);
       break;
      default:
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform semantic checks of key/value data.
    \param[in] filename    name of Interfile header
    \param[in] kv          key-value pair with length-of-lists-information
    \exception REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG wrong number of values in
                                                    key

    Perform semantic checks of key/value data with arrays of lists.
 */
/*---------------------------------------------------------------------------*/
void KV::semanticCheckAL(const std::string filename, const KV * const kv) const
 { switch (type)
    { case KV::ASCII_AL:
       if (v.array != NULL)
        if (v.array->size() > 0)
         { std::vector <tarray *>::const_iterator it_a1;
                                         // is number of values in list wrong ?
           for (it_a1=v.array->begin(); it_a1 != v.array->end(); it_a1++)
            { unsigned short int max_num;

              max_num=kv->getValue<unsigned short int>((*it_a1)->index-1);
              if ((*it_a1)->ascii_list != NULL)
               if (max_num != (*it_a1)->ascii_list->size())
                throw Exception(REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG,
                            "Parsing error in the file '#1'. The key '#2[#3]' "
                            "contains the wrong number of values"
                            ".").arg(filename).arg(key).arg((*it_a1)->index);

            }
         }
       break;
      case KV::USHRT_AL:
       if (v.array != NULL)
        if (v.array->size() > 0)
         { std::vector <tarray *>::const_iterator it_a1;
                                         // is number of values in list wrong ?
           for (it_a1=v.array->begin(); it_a1 != v.array->end(); it_a1++)
            { unsigned short int max_num;

              max_num=kv->getValue<unsigned short int>((*it_a1)->index-1);
              if ((*it_a1)->usi_list != NULL)
               if (max_num != (*it_a1)->usi_list->size())
                throw Exception(REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG,
                            "Parsing error in the file '#1'. The key '#2[#3]' "
                            "contains the wrong number of values"
                            ".").arg(filename).arg(key).arg((*it_a1)->index);

            }
         }
       break;
      case KV::UFLOAT_AL:
       if (v.array != NULL)
        if (v.array->size() > 0)
         { std::vector <tarray *>::const_iterator it_a1;
                                         // is number of values in list wrong ?
           for (it_a1=v.array->begin(); it_a1 != v.array->end(); it_a1++)
            { unsigned short int max_num;

              max_num=kv->getValue<unsigned short int>((*it_a1)->index-1);
              if ((*it_a1)->f_list != NULL)
               if (max_num != (*it_a1)->f_list->size())
                throw Exception(REC_ITF_HEADER_ERROR_VALUE_NUM_WRONG,
                            "Parsing error in the file '#1'. The key '#2[#3]' "
                            "contains the wrong number of values"
                            ".").arg(filename).arg(key).arg((*it_a1)->index);

            }
         }
       break;
      default:
       break;
    }
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Set value of key.
    \param[in] value      value of key
    \param[in] _unit      unit of key
    \param[in] _comment   comment of key
    \param[in] filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_TYPE      value with unexpected type
    \exception REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL value too small
    \exception REC_ITF_HEADER_ERROR_DATE_INVALID    date is invalid

    Set value of key.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void KV::setValue(const T value, const std::string _unit,
                  const std::string _comment, const std::string filename)
 { try
   { comment=_comment;
     unit=_unit;
     switch (type)
      { case KV::ASCII:
        case KV::RELFILENAME:
         if (typeid(T) != typeid(std::string))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         if (v.ascii != NULL) delete v.ascii;
         v.ascii=new std::string(*(std::string *)&value);
         break;
        case KV::USHRT:
         if (typeid(T) != typeid(unsigned short int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.usi=*(unsigned short int *)&value;
         break;
        case KV::ULONG:
         if (typeid(T) != typeid(unsigned long int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.uli=*(unsigned long int *)&value;
         break;
        case KV::LONG:
         if (typeid(T) != typeid(signed long int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.sli=*(signed long int *)&value;
         break;
        case KV::ULONGLONG:
         if (typeid(T) != typeid(uint64))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.ulli=*(uint64 *)&value;
         break;
        case KV::LONGLONG:
         if (typeid(T) != typeid(int64))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.lli=*(int64 *)&value;
         break;
        case KV::UFLOAT:
         if (typeid(T) != typeid(float))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         if (*(float *)&value < 0.0f)
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL,
                          "Parsing error in the file '#1'. The value for the "
                          "key '#2' is too small.").arg(filename).arg(key);
         v.f=*(float *)&value;
         break;
        case KV::FLOAT:
         if (typeid(T) != typeid(float))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.f=*(float *)&value;
         break;
        case KV::DATE:
         if (typeid(T) != typeid(TIMEDATE::tdate))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.date=*(TIMEDATE::tdate *)&value;
         checkDate(filename);
         break;
        case KV::TIME:
         if (typeid(T) != typeid(TIMEDATE::ttime))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.time=*(TIMEDATE::ttime *)&value;
         if ((v.time.hour > 23) || (v.time.minute > 59) ||
             (v.time.second > 59) || (v.time.gmt_offset_h < -23) ||
             (v.time.gmt_offset_h > 23) || (v.time.gmt_offset_m > 59) ||
             (v.time.gmt_offset_m < -59))
          throw Exception(REC_ITF_HEADER_ERROR_DATE_INVALID,
                          "Parsing error in the file '#1'. The date in the key"
                          " '#2' is invalid.").arg(filename).arg(key);
         break;
        case KV::SEX:
         if (typeid(T) != typeid(tsex))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.sex=*(tsex *)&value;
         break;
        case KV::ORIENTATION:
         if (typeid(T) != typeid(tpatient_orientation))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.po=*(tpatient_orientation *)&value;
         break;
        case KV::DIRECTION:
         if (typeid(T) != typeid(tscan_direction))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.dir=*(tscan_direction *)&value;
         break;
        case KV::PETDT:
         if (typeid(T) != typeid(tpet_data_type))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.pdt=*(tpet_data_type *)&value;
         break;
        case KV::NUMFORM:
         if (typeid(T) != typeid(tnumber_format))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.nufo=*(tnumber_format *)&value;
         break;
        case KV::ENDIAN:
         if (typeid(T) != typeid(tendian))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.endian=*(tendian *)&value;
         break;
        case KV::SEPTA:
         if (typeid(T) != typeid(tsepta))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.septa=*(tsepta *)&value;
         break;
        case KV::DETMOTION:
         if (typeid(T) != typeid(tdetector_motion))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.detmot=*(tdetector_motion *)&value;
         break;
        case KV::EXAMTYPE:
         if (typeid(T) != typeid(texamtype))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.examtype=*(texamtype *)&value;
         break;
        case KV::ACQCOND:
         if (typeid(T) != typeid(tacqcondition))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.acqcond=*(tacqcondition *)&value;
         break;
        case KV::SCANNER_TYPE:
         if (typeid(T) != typeid(tscanner_type))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.sct=*(tscanner_type *)&value;
         break;
        case KV::DECAYCORR:
         if (typeid(T) != typeid(tdecay_correction))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.dec=*(tdecay_correction *)&value;
         break;
        case KV::SL_ORIENTATION:
         if (typeid(T) != typeid(tslice_orientation))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.so=*(tslice_orientation *)&value;
         break;
        case KV::RANDCORR:
         if (typeid(T) != typeid(trandom_correction))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.rc=*(trandom_correction *)&value;
         break;
        case KV::PROCSTAT:
         if (typeid(T) != typeid(tprocess_status))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         v.ps=*(tprocess_status *)&value;
         break;
        default:                                                // can't happen
         break;
      }
   }
   catch (...)
    { if ((v.ascii != NULL) &&
          ((type == KV::ASCII) || (type == KV::RELFILENAME)))
       { delete v.ascii;
         v.ascii=NULL;
       }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set value of key with array of values or list of values.
    \param[in] value      value of key
    \param[in] _unit      unit of key
    \param[in] _comment   comment of key
    \param[in] a_idx      index of array
    \param[in] filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_TYPE      value with unexpected type
    \exception REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL value too small

    Set value of key with array of values or list of values.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void KV::setValue(const T value, const std::string _unit,
                  const std::string _comment, const unsigned short int a_idx,
                  const std::string filename)
 { tarray *a=NULL;
   try
   { switch (type)
      { case KV::ASCII_L:
         if (typeid(T) != typeid(std::string))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         comment=_comment;
         unit=_unit;
         if (v.ascii_l == NULL) v.ascii_l=new std::vector <std::string>;
                                                          // list long enough ?
         if (v.ascii_l->size() <= a_idx) v.ascii_l->resize(a_idx+1);
         v.ascii_l->at(a_idx)=*(std::string *)&value;
         break;
        case KV::ASCII_A:
         if (typeid(T) != typeid(std::string))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                if (v.array->at(a_index)->ascii != NULL)
                 delete v.array->at(a_index)->ascii;
                v.array->at(a_index)->ascii=
                                       new std::string(*(std::string *)&value);
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->ascii=new std::string(*(std::string *)&value);
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::USHRT_L:
         if (typeid(T) != typeid(unsigned short int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         comment=_comment;
         unit=_unit;
         if (v.usi_l == NULL) v.usi_l=new std::vector <unsigned short int>;
                                                          // list long enough ?
         if (v.usi_l->size() <= a_idx) v.usi_l->resize(a_idx+1);
         v.usi_l->at(a_idx)=*(unsigned short int *)&value;
         break;
        case KV::USHRT_A:
         if (typeid(T) != typeid(unsigned short int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
              v.array->at(a_index)->unit=_unit;
                v.array->at(a_index)->usi=*(unsigned short int *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->usi=*(unsigned short int *)&value;
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::ULONG_A:
         if (typeid(T) != typeid(unsigned long int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                v.array->at(a_index)->uli=*(unsigned long int *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->uli=*(unsigned long int *)&value;
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::ULONG_L:
         if (typeid(T) != typeid(unsigned long int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         comment=_comment;
         unit=_unit;
         if (v.uli_l == NULL) v.uli_l=new std::vector <unsigned long int>;
                                                          // list long enough ?
         if (v.uli_l->size() <= a_idx) v.uli_l->resize(a_idx+1);
         v.uli_l->at(a_idx)=*(unsigned long int *)&value;
         break;
        case KV::ULONGLONG_A:
         if (typeid(T) != typeid(uint64))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                v.array->at(a_index)->ulli=*(uint64 *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->ulli=*(uint64 *)&value;
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::LONGLONG_A:
         if (typeid(T) != typeid(int64))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                v.array->at(a_index)->lli=*(int64 *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->lli=*(int64 *)&value;
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::UFLOAT_L:
        case KV::FLOAT_L:
         if (typeid(T) != typeid(float))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                          ".").arg(filename).arg(key);
         comment=_comment;
         unit=_unit;
         if (v.f_l == NULL) v.f_l=new std::vector <float>;
                                                          // list long enough ?
         if (v.f_l->size() <= a_idx) v.f_l->resize(a_idx+1);
         v.f_l->at(a_idx)=*(float *)&value;
         break;
        case KV::UFLOAT_A:
         if (typeid(T) != typeid(float))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;

            if (*(float *)&value < 0.0f)
             throw Exception(REC_ITF_HEADER_ERROR_VALUE_TOO_SMALL,
                             "Parsing error in the file '#1'. The value for "
                             "the key '#2' is too small"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                v.array->at(a_index)->f=*(float *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->f=*(float *)&value;
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::DATASET_A:
         if (typeid(T) != typeid(tdataset))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                v.array->at(a_index)->ds.acquisition_number=
                                      (*(tdataset *)&value).acquisition_number;
                if ((*(tdataset *)&value).headerfile != NULL)
                 v.array->at(a_index)->ds.headerfile=
                            new std::string(*(*(tdataset *)&value).headerfile);
                 else v.array->at(a_index)->ds.headerfile=NULL;
                if ((*(tdataset *)&value).datafile != NULL)
                 v.array->at(a_index)->ds.datafile=
                              new std::string(*(*(tdataset *)&value).datafile);
                 else v.array->at(a_index)->ds.datafile=NULL;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->ds.acquisition_number=(*(tdataset *)&value).acquisition_number;
            if ((*(tdataset *)&value).headerfile != NULL)
             a->ds.headerfile=
                            new std::string(*(*(tdataset *)&value).headerfile);
             else a->ds.headerfile=NULL;
            if ((*(tdataset *)&value).datafile != NULL)
             a->ds.datafile=new std::string(*(*(tdataset *)&value).datafile);
             else a->ds.datafile=NULL;
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::DTDES_A:
         if (typeid(T) != typeid(tdata_type_description))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                         "Parsing error in the file '#1'. Value with "
                         "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          { std::vector <tarray *>::iterator it;
            unsigned short int a_index=0;
                                                          // search array index
            for (it=v.array->begin(); it != v.array->end(); it++,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                v.array->at(a_index)->dtd=*(tdata_type_description *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->dtd=*(tdata_type_description *)&value;
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        default:                                                // can't happen
         break;
      }
   }
   catch (...)
    { if (a != NULL) delete a;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set value of key with array of lists of values.
    \param[in] value      value of key
    \param[in] _unit      unit of key
    \param[in] _comment   comment of key
    \param[in] a_idx      index of array
    \param[in] l_idx      index of list element
    \param[in] filename   name of Interfile header
    \exception REC_ITF_HEADER_ERROR_VALUE_TYPE value with unexpected type

    Set value of key with array of list of values.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void KV::setValue(const T value, const std::string _unit,
                  const std::string _comment, const unsigned short int a_idx,
                  const unsigned short int l_idx, const std::string filename)
 { tarray *a=NULL;

   try
   { std::vector <tarray *>::iterator it;
     unsigned short int a_index=0;

     switch (type)
      { case KV::ASCII_AL:
         if (typeid(T) != typeid(std::string))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                          "Parsing error in the file '#1'. Value with "
                          "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          {                                               // search array index
            for (it=v.array->begin(); it != v.array->end(); ++it,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                if (v.array->at(a_index)->ascii_list == NULL)
                 v.array->at(a_index)->ascii_list=
                                                 new std::vector <std::string>;
                                                          // list long enough ?
                if (v.array->at(a_index)->ascii_list->size() <= l_idx)
                 v.array->at(a_index)->ascii_list->resize(l_idx+1);
                                                        // put string into list
                v.array->at(a_index)->ascii_list->at(l_idx)=
                                                        *(std::string *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->ascii_list=new std::vector <std::string>;
            a->ascii_list->resize(l_idx);                       // enlarge list
                                                          // add string to list
            a->ascii_list->push_back(*(std::string *)&value);
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::USHRT_AL:
         if (typeid(T) != typeid(unsigned short int))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                         "Parsing error in the file '#1'. Value with "
                         "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          {                                               // search array index
            for (it=v.array->begin(); it != v.array->end(); it++,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                if (v.array->at(a_index)->usi_list == NULL)
                 v.array->at(a_index)->usi_list=
                                          new std::vector <unsigned short int>;
                                                          // list long enough ?
                if (v.array->at(a_index)->usi_list->size() <= l_idx)
                 v.array->at(a_index)->usi_list->resize(l_idx+1);
                                            // put unsigned short int into list
                v.array->at(a_index)->usi_list->at(l_idx)=
                                                 *(unsigned short int *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->usi_list=new std::vector <unsigned short int>;
            a->usi_list->resize(l_idx);                         // enlarge list
                                              // add unsigned short int to list
            a->usi_list->push_back(*(unsigned short int *)&value);
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        case KV::UFLOAT_AL:
         if (typeid(T) != typeid(float))
          throw Exception(REC_ITF_HEADER_ERROR_VALUE_TYPE,
                         "Parsing error in the file '#1'. Value with "
                         "unexpected type in the key '#2'"
                         ".").arg(filename).arg(key+"["+toString(a_idx+1)+"]");
         if (v.array != NULL)
          {                                               // search array index
            for (it=v.array->begin(); it != v.array->end(); it++,
                 a_index++)
             if ((*it)->index == a_idx+1)                        // index found
              { v.array->at(a_index)->comment=_comment;
                v.array->at(a_index)->unit=_unit;
                if (v.array->at(a_index)->f_list == NULL)
                 v.array->at(a_index)->f_list=new std::vector <float>;
                                                          // list long enough ?
                if (v.array->at(a_index)->f_list->size() <= l_idx)
                 v.array->at(a_index)->f_list->resize(l_idx+1);
                                                         // put float into list
                v.array->at(a_index)->f_list->at(l_idx)=*(float *)&value;
                return;
              }
              else if ((*it)->index > a_idx+1) break;
                                                             // create new list
            a=new tarray;
            a->index=a_idx+1;
            a->comment=_comment;
            a->unit=_unit;
            a->f_list=new std::vector <float>;
            a->f_list->resize(l_idx);                           // enlarge list
            a->f_list->push_back(*(float *)&value);        // add float to list
                        // add new list to array or insert (indizes are sorted)
            if (a_index == v.array->size()) v.array->push_back(a);
             else v.array->insert(it, a);
            a=NULL;
          }
         break;
        default:
         break;
      }
   }
   catch (...)
    { if (a != NULL) delete a;
      throw;
    }
 }

#ifndef _KV_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Request type of value.
    \return type of value

    Request type of value.
 */
/*---------------------------------------------------------------------------*/
KV::ttoken_type KV::Type() const
 { return(type);
 }
#endif
