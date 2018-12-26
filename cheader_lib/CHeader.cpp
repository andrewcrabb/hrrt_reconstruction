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

#include <boost/xpressive/xpressive.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <fmt/format.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;

namespace bf = boost::filesystem;
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
// 'extern' approach: http://bit.ly/2Sn8eSS gives 'error: storage class specified for ‘HDR_AXIAL_COMPRESSION’'
// Omitting the 'extern' in the hpp removes the error.  
//   .hpp:   extern string const HDR_AXIAL_COMPRESSION           ;
//   .cpp:   string const CHeader::HDR_AXIAL_COMPRESSION  = "axial compression";
// 'static' approach: http://bit.ly/2Va3Kkm

string const CHeader::HDR_AXIAL_COMPRESSION  = "axial compression";
string const CHeader::HDR_AVERAGE_SINGLES_PER_BLOCK   = "average singles per block";
string const CHeader::HDR_BRANCHING_FACTOR            = "branching factor";
string const CHeader::HDR_DATA_FORMAT                 = "data format";
string const CHeader::HDR_DEAD_TIME_CORRECTION_FACTOR = "Dead time correction factor";
string const CHeader::HDR_DECAY_CORRECTION_FACTOR     = "decay correction factor";
string const CHeader::HDR_DECAY_CORRECTION_FACTOR_2   = "decay correction factor2";
string const CHeader::HDR_DOSAGE_STRENGTH             = "Dosage Strength";
string const CHeader::HDR_DOSE_ASSAY_DATE             = "dose_assay_date (dd:mm:yryr)";
string const CHeader::HDR_DOSE_ASSAY_TIME             = "dose_assay_time (hh:mm:ss)";
string const CHeader::HDR_DOSE_TYPE                   = "Dose type";
string const CHeader::HDR_ENERGY_WINDOW_LOWER_LEVEL   = "energy window lower level[1]";
string const CHeader::HDR_ENERGY_WINDOW_UPPER_LEVEL   = "energy window upper level[1]";
string const CHeader::HDR_FRAME                       = "frame";
string const CHeader::HDR_FRAME_DEFINITION            = "Frame definition";
string const CHeader::HDR_HISTOGRAMMER_REVISION       = "!histogrammer revision";
string const CHeader::HDR_IMAGE_DURATION              = "image duration";
string const CHeader::HDR_IMAGE_RELATIVE_START_TIME   = "image relative start time";
string const CHeader::HDR_INTERFILE                   = "!INTERFILE";
string const CHeader::HDR_ISOTOPE_HALFLIFE            = "isotope halflife";
string const CHeader::HDR_LM_REBINNER_METHOD          = "!LM rebinner method";
string const CHeader::HDR_LMHISTOGRAM_BUILD_ID        = "!lmhistogram build ID";
string const CHeader::HDR_LMHISTOGRAM_VERSION         = "!lmhistogram version";
string const CHeader::HDR_MATRIX_SIZE_1               = "matrix size [1]";
string const CHeader::HDR_MATRIX_SIZE_2               = "matrix size [2]";
string const CHeader::HDR_MATRIX_SIZE_3               = "matrix size [3]";
string const CHeader::HDR_MAXIMUM_RING_DIFFERENCE     = "maximum ring difference";
string const CHeader::HDR_NAME_OF_DATA_FILE           = "!name of data file";
string const CHeader::HDR_NAME_OF_TRUE_DATA_FILE      = "!name of true data file";
string const CHeader::HDR_NUMBER_FORMAT               = "number format";
string const CHeader::HDR_NUMBER_OF_BYTES_PER_PIXEL   = "number of bytes per pixel";
string const CHeader::HDR_NUMBER_OF_DIMENSIONS        = "number of dimensions";
string const CHeader::HDR_ORIGINATING_SYSTEM          = "!originating system";
string const CHeader::HDR_PATIENT_DOB                 = "Patient DOB";
string const CHeader::HDR_PATIENT_ID                  = "Patient ID";
string const CHeader::HDR_PATIENT_NAME                = "Patient name";
string const CHeader::HDR_PATIENT_SEX                 = "Patient sex";
string const CHeader::HDR_PET_DATA_TYPE               = "!PET data type";
string const CHeader::HDR_PROGRAM_BUILD_ID            = ";program build ID";
string const CHeader::HDR_SCALING_FACTOR_1            = "scaling factor (mm/pixel) [1]";
string const CHeader::HDR_SCALING_FACTOR_2            = "scaling factor [2]";
string const CHeader::HDR_SCALING_FACTOR_3            = "scaling factor (mm/pixel) [3]";
string const CHeader::HDR_SINOGRAM_DATA_TYPE          = "Sinogram data type";
string const CHeader::HDR_SOFTWARE_VERSION            = ";software version";
string const CHeader::HDR_STUDY_DATE                  = "!study date (dd:mm:yryr)";
string const CHeader::HDR_STUDY_TIME                  = "!study time (hh:mm:ss)";
string const CHeader::HDR_TOTAL_NET_TRUES             = "Total Net Trues";
string const CHeader::HDR_TOTAL_PROMPTS               = "Total Prompts";
string const CHeader::HDR_TOTAL_RANDOMS               = "Total Randoms";

// Test data
Tag const CHeader::VALID_CHAR(HDR_ORIGINATING_SYSTEM, "HRRT");
Tag const CHeader::VALID_DOUBLE(HDR_ISOTOPE_HALFLIFE, "100.0");
Tag const CHeader::VALID_FLOAT(HDR_ISOTOPE_HALFLIFE , "100.0");
Tag const CHeader::VALID_INT(HDR_IMAGE_DURATION     , "5400");

string Tag::sayit(void) const {
  string str = fmt::format("{} = {}", key, value);
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
    return (CHeaderError::E_FILE_ALREADY_OPEN);
  hdr_file_.open(filename, std::ifstream::in);
  if (!hdr_file_.is_open())
    return (CHeaderError::E_COULD_NOT_OPEN_FILE);
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
 * @return     CHeaderError::E_OK, or one of the CHeaderError::E_foo error values
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
  return CHeaderError::E_OK;
}

CHeaderError CHeader::ReadFile() {
  string buffer;
  while (safeGetline(hdr_file_, buffer)) {
    InsertTag(buffer);
  }
  return CHeaderError::E_OK;
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
    return CHeaderError::E_COULD_NOT_OPEN_FILE;
  }
  fp.open(outname, std::ofstream::out);
  if (!fp.is_open()) {
    LOG_ERROR(logger_, "Could not open file: {}", fname);
    return CHeaderError::E_COULD_NOT_OPEN_FILE;
  }

  fp << "!INTERFILE\n";
  for (auto &tag : tags_) {
    fp << tag.key << " := " << tag.value << endl;
  }
  if (fp.fail()) {
    LOG_ERROR(logger_, "fp.fail(): {}", fname);
    return CHeaderError::E_COULD_NOT_OPEN_FILE;
  }
  fp.close();
  LOG_DEBUG(logger_, "returning {}", to_underlying(CHeaderError::E_OK));

  return CHeaderError::E_OK;
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
    return CHeaderError::E_OK;
  }
  return CHeaderError::E_COULD_NOT_OPEN_FILE;
}

CHeaderError CHeader::CloseFile() {
  if (IsFileOpen()) {
    hdr_file_.close();
    m_FileName_.erase();
    return CHeaderError::E_OK;
  }
  return CHeaderError::E_COULD_NOT_OPEN_FILE;
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
  CHeaderError ret = CHeaderError::E_OK;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags_)) {
    tags_.push_back({key, val});
    ret = CHeaderError::E_TAG_APPENDED;  // Not an error but consistent with other methods.
  } else {
    it->value.assign(val);
  }
  return ret;
}

// Fill in val if tag is found.

CHeaderError CHeader::Readchar(string const &key, string &val) {
  CHeaderError ret = CHeaderError::E_OK;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags_)) {
    ret = CHeaderError::E_TAG_NOT_FOUND;
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
  CHeaderError ret = CHeaderError::E_OK;
  try {
    val = boost::lexical_cast<T>(str);
  }
  catch (boost::bad_lexical_cast const &e) {
    LOG_ERROR(logger_, "Lexical cast: {}",  e.what());
    ret = CHeaderError::E_BAD_LEXICAL_CAST;
  }
  return ret;
}

/**
 * @brief      Read time from element with given tag
 * @param[in]  tag   Tag to read
 * @param      time  Time stored in the tag
 * @return     0 on success, else 1
 */
CHeaderError CHeader::ReadTime(std::string const &tag, boost::posix_time::ptime &time) {
  string line, key, value;
  CHeaderError ret = CHeaderError::E_OK;
  if (Readchar(tag, line) == CHeaderError::E_OK) {
    if (!parse_interfile_line(line, key, value)) {
      ret = parse_interfile_time(value, time) ? CHeaderError::E_NOT_A_TIME : CHeaderError::E_OK;
    }
  }
  return ret;
}

// Open file, read given value, close file.

template <typename T>int CHeader::ReadHeaderNum(string &filename, string &tag, T &val) {
int result;
if (OpenFile(filename) == CHeaderError::E_OK) {
  result = ReadNum<T>(tag, val);
  CloseFile();
}
return result;
}


// Read given tag and return its numeric value
// Return 0 on success, else 1

template <typename T>CHeaderError CHeader::ReadNum(string const &tag, T &val) {
  string str;
  CHeaderError result = Readchar(tag, str);
  if (result == CHeaderError::E_OK) {
    result = convertString<T>(str, val);
    if (result != CHeaderError::E_OK) {
      if (std::is_same<T, int>::value) {
        result = CHeaderError::E_NOT_AN_INT;
      } else if (std::is_same<T, long>::value) {
        result = CHeaderError::E_NOT_A_LONG;
      } else if (std::is_same<T, float>::value) {
        result = CHeaderError::E_NOT_A_FLOAT;
      } else if (std::is_same<T, double>::value) {
        result = CHeaderError::E_NOT_A_DOUBLE;
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
