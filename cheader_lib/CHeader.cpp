// CHeader.cpp: implementation of the CHeader class.
//
//      Copyright (C) CPS Innovations 2004 All Rights Reserved.
//////////////////////////////////////////////////////////////////////

/*
Modification history:
          01/07/2009 : Warnings cleaning
          24-JUL-2009: Use const char for keywords
          20180830 ahc Convert to standard C++11, String, Vector, iterators, spdlog and East Const
*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstdint>
#include <cstring>
#include <ctype.h>
#include <iostream>


#include "CHeader.hpp"
#include "hrrt_util.hpp"

#include <boost/date_time.hpp>
#include <boost/date_time/date_facet.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/xpressive/xpressive.hpp>

#include <fmt/format.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace bf = boost::filesystem;
namespace bt = boost::posix_time;
namespace bx = boost::xpressive;

// Cast CHeaderError to its underlying type (int) for LOG_foo macros.  http://bit.ly/2PSzLJY
// NOTE this is a c++14 construct.
    // template <typename E> constexpr auto CHeader::to_underlying(E e);
    // Cast CHeaderError to its underlying type (int) for LOG_foo macros.
template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}
using ::to_underlying;

// Define const strings only once.  Declared as externs in .hpp 
// 'extern' approach: http://bit.ly/2Sn8eSS gives 'error: storage class specified for ‘AXIAL_COMPRESSION’'
// Omitting the 'extern' in the hpp removes the error.  
//   .hpp:   extern string const CHeader::AXIAL_COMPRESSION           ;
//   .cpp:   string const CHeader::AXIAL_COMPRESSION  = "axial compression";
// 'static' approach: http://bit.ly/2Va3Kkm

string const CHeader::AXIAL_COMPRESSION  = "axial compression";
string const CHeader::AVERAGE_SINGLES_PER_BLOCK   = "average singles per block";
string const CHeader::BRANCHING_FACTOR            = "branching factor";
string const CHeader::DATA_FORMAT                 = "data format";
string const CHeader::DEAD_TIME_CORRECTION_FACTOR = "Dead time correction factor";
string const CHeader::DECAY_CORRECTION_FACTOR     = "decay correction factor";
string const CHeader::DECAY_CORRECTION_FACTOR_2   = "decay correction factor2";
string const CHeader::DOSAGE_STRENGTH             = "Dosage Strength";
string const CHeader::DOSE_ASSAY_DATE             = "dose_assay_date (dd:mm:yryr)";
string const CHeader::DOSE_ASSAY_TIME             = "dose_assay_time (hh:mm:ss)";
string const CHeader::DOSE_TYPE                   = "Dose type";
string const CHeader::ENERGY_WINDOW_LOWER_LEVEL   = "energy window lower level[1]";
string const CHeader::ENERGY_WINDOW_UPPER_LEVEL   = "energy window upper level[1]";
string const CHeader::FRAME                       = "frame";
string const CHeader::FRAME_DEFINITION            = "Frame definition";
string const CHeader::HISTOGRAMMER_REVISION       = "!histogrammer revision";
string const CHeader::IMAGE_DURATION              = "image duration";
string const CHeader::IMAGE_RELATIVE_START_TIME   = "image relative start time";
string const CHeader::INTERFILE                   = "!INTERFILE";
string const CHeader::ISOTOPE_HALFLIFE            = "isotope halflife";
string const CHeader::LM_REBINNER_METHOD          = "!LM rebinner method";
string const CHeader::LMHISTOGRAM_BUILD_ID        = "!lmhistogram build ID";
string const CHeader::LMHISTOGRAM_VERSION         = "!lmhistogram version";
string const CHeader::MATRIX_SIZE_1               = "matrix size [1]";
string const CHeader::MATRIX_SIZE_2               = "matrix size [2]";
string const CHeader::MATRIX_SIZE_3               = "matrix size [3]";
string const CHeader::MAXIMUM_RING_DIFFERENCE     = "maximum ring difference";
string const CHeader::NAME_OF_DATA_FILE           = "!name of data file";
string const CHeader::NAME_OF_TRUE_DATA_FILE      = "!name of true data file";
string const CHeader::NUMBER_FORMAT               = "number format";
string const CHeader::NUMBER_OF_BYTES_PER_PIXEL   = "number of bytes per pixel";
string const CHeader::NUMBER_OF_DIMENSIONS        = "number of dimensions";
string const CHeader::ORIGINATING_SYSTEM          = "!originating system";
string const CHeader::PATIENT_DOB                 = "Patient DOB";
string const CHeader::PATIENT_ID                  = "Patient ID";
string const CHeader::PATIENT_NAME                = "Patient name";
string const CHeader::PATIENT_SEX                 = "Patient sex";
string const CHeader::PET_DATA_TYPE               = "!PET data type";
string const CHeader::PROGRAM_BUILD_ID            = ";program build ID";
string const CHeader::SCALING_FACTOR_1            = "scaling factor (mm/pixel) [1]";
string const CHeader::SCALING_FACTOR_2            = "scaling factor [2]";
string const CHeader::SCALING_FACTOR_3            = "scaling factor (mm/pixel) [3]";
string const CHeader::SINOGRAM_DATA_TYPE          = "Sinogram data type";
string const CHeader::SOFTWARE_VERSION            = ";software version";
string const CHeader::STUDY_DATE                  = "!study date (dd:mm:yryr)";
string const CHeader::STUDY_TIME                  = "!study time (hh:mm:ss)";
string const CHeader::TOTAL_NET_TRUES             = "Total Net Trues";
string const CHeader::TOTAL_PROMPTS               = "Total Prompts";
string const CHeader::TOTAL_RANDOMS               = "Total Randoms";

// Test data matching contents of test_EM.l64.hdr
Tag const CHeader::VALID_CHAR(CHeader::ORIGINATING_SYSTEM, "HRRT");
Tag const CHeader::VALID_DOUBLE(CHeader::BRANCHING_FACTOR, "0.0");
Tag const CHeader::VALID_FLOAT(CHeader::ISOTOPE_HALFLIFE , "100.0");
Tag const CHeader::VALID_INT(CHeader::IMAGE_DURATION     , "5400");
Tag const CHeader::VALID_DATE(CHeader::STUDY_DATE        , "03:12:2009");  // DD:MM:YYYY
Tag const CHeader::VALID_TIME(CHeader::STUDY_TIME        , "12:03:00");    // HH:MM:SS

string const ECAT_DATE_FORMAT = "%d:%m:%Y";  // !study date (dd:mm:yryr) := 18:09:2017
string const ECAT_TIME_FORMAT = "%H:%M:%S";  // !study time (hh:mm:ss) := 15:26:00

string Tag::sayit(void) const {
  string str = fmt::format("'{}' with value '{}'", key, value);
  return str;
}

CHeader::CHeader() {
  logger_ = spdlog::get("CHeader");
  if (!logger_) {
    std::cerr << "Error: No logger 'CHeader'" << std::endl;
  }
}

CHeader::~CHeader() {
  if (hdr_file_.is_open())
    hdr_file_.close();
}

/**
 * Loads specified filename in memory table.
 * Returns 1 if OK or a negative number (-ERR_CODE) otherwise.
 * Error codes:
 * HdrErrors[-ERR_CODE] contains the text error message.
 */

CHeaderError CHeader::OpenFile(string const &filename) {
  LOG_DEBUG(logger_, filename);
  if (hdr_file_.is_open())
    return (CHeaderError::FILE_ALREADY_OPEN);
  hdr_file_.open(filename, std::ifstream::in);
  if (!hdr_file_.is_open())
    return (CHeaderError::COULD_NOT_OPEN_FILE);
  m_FileName_.assign(filename);
  return ReadFile();
}

CHeaderError CHeader::OpenFile(bf::path &filename) {
  LOG_DEBUG(logger_, filename.string());
  return OpenFile(filename.string());
}

/**
 * @brief      Add to tags_ a new Tag from pattern "key := value" in buffer
 * @param[in]  t_buffer  Line from input file
 * @return     CHeaderError::OK, or one of the CHeaderError::foo error values
 */

CHeaderError CHeader::InsertTag(string t_buffer) {
  using namespace boost::xpressive;

  smatch match;
  sregex m_reg = sregex::compile("^\\s*(?P<key>.+)\\s+:=\\s+(?P<value>.+)?\\s*$");
  if (regex_match(t_buffer, match, m_reg)) {
    std::string key = match["key"];
    std::string value = match["value"];
    tags_.push_back({key, value});
    LOG_TRACE(logger_, "push[{}, {}]: length {}", key, value, tags_.size() );
  } else {
    LOG_DEBUG(logger_, "No match: '{}'", t_buffer);
  }
  return CHeaderError::OK;
}

CHeaderError CHeader::ReadFile() {
  string buffer;
  while (safeGetline(hdr_file_, buffer)) {
    InsertTag(buffer);
  }
  return CHeaderError::OK;
}

CHeaderError CHeader::WriteFile(bf::path const &fname) {
  LOG_DEBUG(logger_, "bf::path {}", fname.string());
  string s(fname.string());
  return WriteFile(s);  
}

CHeaderError CHeader::WriteFile(string &fname) {
  string outname = (fname.length()) ? fname : m_FileName_;
  LOG_DEBUG(logger_, "tag count {}, string outname {}", tags_.size(), outname);
  std::ofstream fp;

  if (tags_.size() == 0) {
    LOG_ERROR(logger_, "tags.size = 0: {}", fname);
    return CHeaderError::COULD_NOT_OPEN_FILE;
  }
  fp.open(outname, std::ofstream::out);
  if (!fp.is_open()) {
    LOG_ERROR(logger_, "Could not open file: {}", fname);
    return CHeaderError::COULD_NOT_OPEN_FILE;
  }

  fp << "!INTERFILE\n";
  for (auto &tag : tags_) {
    fp << tag.key << " := " << tag.value << endl;
  }
  if (fp.fail()) {
    LOG_ERROR(logger_, "fp.fail(): {}", fname);
    return CHeaderError::COULD_NOT_OPEN_FILE;
  }
  fp.close();
  LOG_DEBUG(logger_, "returning {}", to_underlying(CHeaderError::OK));

  return CHeaderError::OK;
}

int CHeader::NumTags(void) {
  return tags_.size();
}

int CHeader::IsFileOpen() {
  return hdr_file_.is_open();
}

CHeaderError CHeader::GetFileName(string &filename) {
  if (IsFileOpen()) {
    filename = m_FileName_;
    return CHeaderError::OK;
  }
  return CHeaderError::COULD_NOT_OPEN_FILE;
}

CHeaderError CHeader::CloseFile() {
  if (IsFileOpen()) {
    hdr_file_.close();
    m_FileName_.erase();
    return CHeaderError::OK;
  }
  return CHeaderError::COULD_NOT_OPEN_FILE;
}

// Locate tag having given key

tag_iterator CHeader::FindTag(string const &key) {
  auto pred = [key](Tag const & tag) { return tag.key == key; };
  // return std::find_if(std::begin(tags_), std::end(tags_), pred);

  // tag_iterator ibegin = tags_.begin();
  // tag_iterator iend = tags_.end();
  auto ibegin = tags_.begin();
  auto iend = tags_.end();
  tag_iterator found = std::find_if(ibegin, iend, pred);
  // return std::find_if(std::begin(tags_), std::end(tags_), pred);
  return found;
}

CHeaderError CHeader::WriteFloat(string const &key, float val) {
  string str = fmt::format("{:f}", val);
  return WriteChar(key, str);
}

CHeaderError CHeader::WriteDouble(string const &key, double val) {
  string str = fmt::format("{:f}", val);
  return WriteChar(key, str);
}

CHeaderError CHeader::WriteInt(string const &key, int val) {
  string str = fmt::format("{:d}", val);
  return WriteChar(key, str);
}

CHeaderError CHeader::WriteLong(string const &key, int64_t val) {
  string str = fmt::format("{:d}", val);
  return WriteChar(key, str);
}

CHeaderError CHeader::WritePath(string const &key, bf::path const &val) {
  string const str = val.string();
  return WriteChar(key, str);
}

// CHeaderError CHeader::WriteChar(string const &key, char const *val) {
//   string str(val);
//   return WriteChar(key, str);
// }

// If tag with given key is found, update its value.
// Else append new tag with given key and value.

CHeaderError CHeader::WriteChar(string const &key, string const &val) {
  CHeaderError ret = CHeaderError::OK;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags_)) {
    tags_.push_back({key, val});
    ret = CHeaderError::TAG_APPENDED;  // Not an error but consistent with other methods.
  } else {
    it->value.assign(val);
  }
  return ret;
}

// Fill in val if tag is found.

CHeaderError CHeader::Readchar(string const &key, string &val) {
  CHeaderError ret = CHeaderError::OK;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags_)) {
    string file_name;
    GetFileName(file_name);
    LOG_ERROR(logger_, "Tag not found: '{}' in {}", key, file_name);
    ret = CHeaderError::TAG_NOT_FOUND;
  } else {
    val = it->value;
  }
  LOG_TRACE(logger_, "val {} ret {}", val, to_underlying(ret) );
  return ret;
}

/**
 * @brief      Convert string buffer to numeric value of type T
 * @param[in]  str   string to convert
 * @param      val   numeric return value
 * @tparam     T     Numeric type to convert to: int, float, int64_t
 * @return     0 on success, else 1
 */
template <typename T>CHeaderError CHeader::convertString(string &str, T &val) {
  CHeaderError ret = CHeaderError::OK;
  try {
    val = boost::lexical_cast<T>(str);
  }
  catch (boost::bad_lexical_cast const &e) {
    LOG_ERROR(logger_, "Lexical cast: {}",  e.what());
    ret = CHeaderError::BAD_LEXICAL_CAST;
  }
  return ret;
}

/**
 * @brief      Read date from element with given tag
 * @param[in]  tag   Tag to read
 * @param      time  Date stored in the tag
 * @return     0 on success, else 1
 */
CHeaderError CHeader::ReadDate(std::string const &t_tag, bt::ptime &t_date) {
  return (ReadDateTime(t_tag, ECAT_DATE_FORMAT, t_date) == CHeaderError::OK) ? CHeaderError::OK : CHeaderError::NOT_A_DATE;
}

/**
 * @brief      Read time from element with given tag
 * @param[in]  tag   Tag to read
 * @param      time  Time stored in the tag
 * @return     0 on success, else 1
 */
CHeaderError CHeader::ReadTime(std::string const &t_tag, bt::ptime &t_time) {
  return (ReadDateTime(t_tag, ECAT_TIME_FORMAT, t_time) == CHeaderError::OK) ? CHeaderError::OK : CHeaderError::NOT_A_TIME;
}

CHeaderError CHeader::ReadDateTime(string const &t_tag, string const &t_format, bt::ptime &t_pt) {
  string value;
  CHeaderError ret = CHeaderError::OK;
  if ((ret = Readchar(t_tag, value)) == CHeaderError::OK) {
    LOG_DEBUG(logger_, "t_tag {} value {}", t_tag, value);
    ret = parse_interfile_datetime(value, t_format, t_pt) ? CHeaderError::ERROR : CHeaderError::OK;
  }
  return ret;
}

template <typename T>CHeaderError CHeader::WriteDate(string const &t_tag, T const &t_datetime) {
  return(WriteDateTime(t_tag, ECAT_DATE_FORMAT, t_datetime));
}

template <typename T>CHeaderError CHeader::WriteTime(string const &t_tag, T const &t_datetime) {
  return(WriteDateTime(t_tag, ECAT_TIME_FORMAT, t_datetime));
}

// t_datetime can be a boost::posix_time::ptime or a std::string
// Am I doing this correctly?  Seems wrong to switch on type of T.

template <typename T>CHeaderError CHeader::WriteDateTime(string const &t_tag, string const &t_format, T const &t_datetime) {
  bt::ptime datetime;
  if (std::is_same<T, std::string>::value) {
    std::string datetime_str = t_datetime;
    LOG_TRACE(logger_, "Converting string to ptime: {}", datetime_str);
  bt::time_facet *ofacet = new bt::time_facet();
  ofacet->format(ECAT_DATE_FORMAT.c_str());

UP TO HERE.  THE IDEA IS TO ALLOW A CALL TO WRITEDATE() OR WRITETIME()
WITH EITHER A POSIX_TIME OR A STD::STRING.  
WRITEDATETIME() SHOULD ALWAYS RECEIVE A POSIX_TIME, SO IS NOT TEMPLATED
WRITEDATE() AND WRITETIME() SHOULD CONVERT STRING TO POSIX_TIME IF NEEDED, AND ARE TEMPLATED
FINISH THIS METHOD WITH CODE FROM BOOST_DATE1.CPP

    std::istringstream iss(datetime_str);
    iss.imbue(std::locale(iss.getloc(), ))
  } else if (std::is_same<T, bt::ptime>::value) {
    datetime = t_datetime;
  } else {
    raise();
  }


  bt::time_facet *dtfacet = new bt::time_facet();
  dtfacet->format(t_format.c_str());
  std::ostringstream datetime_osstr;
  datetime_osstr.imbue(std::locale(datetime_osstr.getloc(), dtfacet));
  datetime_osstr << datetime;
  LOG_DEBUG(logger_, "datetime_osstr time: {}", datetime_osstr.str());
  CHeaderError ret = WriteChar(t_tag, datetime_osstr.str());
  return ret;
}

// Open file, read given value, close file.

template <typename T>int CHeader::ReadHeaderNum(string &filename, string &tag, T &val) {
int result;
if (OpenFile(filename) == CHeaderError::OK) {
  result = ReadNum<T>(tag, val);
  CloseFile();
}
return result;
}


// Read given tag and return its numeric value
// Return 0 on success, else 1
// Am I doing this correctly?  Seems wrong to switch on T's type.

template <typename T>CHeaderError CHeader::ReadNum(string const &tag, T &val) {
  string str;
  CHeaderError result = Readchar(tag, str);
  if (result == CHeaderError::OK) {
    result = convertString<T>(str, val);
    if (result != CHeaderError::OK) {
      if (std::is_same<T, int>::value) {
        result = CHeaderError::NOT_AN_INT;
      } else if (std::is_same<T, long>::value) {
        result = CHeaderError::NOT_A_LONG;
      } else if (std::is_same<T, float>::value) {
        result = CHeaderError::NOT_A_FLOAT;
      } else if (std::is_same<T, double>::value) {
        result = CHeaderError::NOT_A_DOUBLE;
      }
    }
  }
  return result;
}

CHeaderError CHeader::Readint(string const &tag, int &val) {
  return ReadNum<int>(tag, val);
}

CHeaderError CHeader::Readlong(string const &tag, long &val) {
  return ReadNum<long>(tag, val);
}

CHeaderError CHeader::Readfloat(string const &tag, float &val) {
  return ReadNum<float>(tag, val);
}

CHeaderError CHeader::Readdouble(string const &tag, double &val) {
  return ReadNum<double>(tag, val);
}

// These came from hrrt_util.cpp
// But all CHeader lines are 'HRRT' ECAT-format lines, so they have been moved here to CHeader.

/**
 * @brief      Parse ECAT-format date into Boost ptime
 *
 * @param[in]  str   Date string in format '18:09:2017'
 * @param[in]  pt    bt::ptime object to set to date
 *
 * @return     false on success, else true
 */
bool CHeader::parse_interfile_datetime(const string &t_datestr, const string &t_format, bt::ptime &t_pt) {
  bt::time_input_facet *facet = new bt::time_input_facet(t_format);
  const std::locale loc(std::locale::classic(), facet);
  auto logger = spdlog::get("CHeader");

  LOG_DEBUG(logger, "t_datestr {} t_format {}", t_datestr, t_format);
  std::istringstream iss(t_datestr);
  iss.imbue(loc);
  // iss.exceptions(std::ios_base::failbit);

  iss >> t_pt;
  bool ret = t_pt.is_not_a_date_time();
  std::string timestr("FILLMEIN");
  // LOG_DEBUG(logger, "t_datestr {} posix_time {} returning {}", t_datestr, bt::to_simple_string(t_pt), ret ? "true" : "false");
  LOG_DEBUG(logger, "t_datestr {} posix_time {} returning {}", t_datestr, timestr, ret ? "true" : "false");
  return ret;
}

bool CHeader::parse_interfile_date(const string &datestr, bt::ptime &pt) {
  return parse_interfile_datetime(datestr, ECAT_DATE_FORMAT, pt);
}

bool CHeader::parse_interfile_time(const string &timestr, bt::ptime &pt) {
  return parse_interfile_datetime(timestr, ECAT_TIME_FORMAT, pt);
}
