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
#include "my_spdlog.hpp"

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
Tag const CHeader::VALID_DATE(CHeader::STUDY_DATE        , "30:12:2009");  // DD:MM:YYYY
Tag const CHeader::VALID_TIME(CHeader::STUDY_TIME        , "15:50:00");    // HH:MM:SS

string Tag::sayit(void) const {
  string str = fmt::format("'{}' with value '{}'", key, value);
  return str;
}

CHeader::CHeader() {
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
  LOG_DEBUG(filename);
  if (hdr_file_.is_open())
    return (CHeaderError::FILE_ALREADY_OPEN);
  hdr_file_.open(filename, std::ifstream::in);
  if (!hdr_file_.is_open())
    return (CHeaderError::COULD_NOT_OPEN_FILE);
  m_FileName_.assign(filename);
  return ReadFile();
}

CHeaderError CHeader::OpenFile(bf::path &filename) {
  LOG_DEBUG(filename.string());
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
    LOG_TRACE("push[{}, {}]: length {}", key, value, tags_.size() );
  } else {
    LOG_DEBUG("No match: '{}'", t_buffer);
  }
  return CHeaderError::OK;
}

CHeaderError CHeader::ReadFile() {
  string buffer;
  while (hrrt_util::safeGetline(hdr_file_, buffer)) {
    InsertTag(buffer);
  }
  return CHeaderError::OK;
}

CHeaderError CHeader::WriteFile(bf::path const &fname) {
  LOG_DEBUG("bf::path {}", fname.string());
  string s(fname.string());
  return WriteFile(s);  
}

CHeaderError CHeader::WriteFile(string &fname) {
  string outname = (fname.length()) ? fname : m_FileName_;
  LOG_DEBUG("tag count {}, string outname {}", tags_.size(), outname);
  std::ofstream fp;

  if (tags_.size() == 0) {
    LOG_ERROR("tags.size = 0: {}", fname);
    return CHeaderError::COULD_NOT_OPEN_FILE;
  }
  fp.open(outname, std::ofstream::out);
  if (!fp.is_open()) {
    LOG_ERROR("Could not open file: {}", fname);
    return CHeaderError::COULD_NOT_OPEN_FILE;
  }

  fp << "!INTERFILE\n";
  for (auto &tag : tags_) {
    fp << tag.key << " := " << tag.value << endl;
  }
  if (fp.fail()) {
    LOG_ERROR("fp.fail(): {}", fname);
    return CHeaderError::COULD_NOT_OPEN_FILE;
  }
  fp.close();
  LOG_DEBUG("returning {}", to_underlying(CHeaderError::OK));

  return CHeaderError::OK;
}

int CHeader::NumTags(void) const {
  return tags_.size();
}

int CHeader::IsFileOpen() const {
  return hdr_file_.is_open();
}

CHeaderError CHeader::GetFileName(string &filename) const {
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
// Const and non-const variants depending on whether the found tag will be modified.
// Use of 'auto' here doesn't work as we have to specify const-ness.
// Could try this to eliminate duplication:
// https://stackoverflow.com/questions/856542/elegant-solution-to-duplicate-const-and-non-const-getters

const_tag_iterator CHeader::FindTag(string const &key) const {
  auto pred = [key](Tag const & tag) { return tag.key == key; };
  // return std::find_if(std::begin(tags_), std::end(tags_), pred);

  const_tag_iterator ibegin = tags_.begin();
  const_tag_iterator iend = tags_.end();
  // auto ibegin = tags_.begin();
  // auto iend = tags_.end();
  auto found = std::find_if(ibegin, iend, pred);
  // tag_iterator tfound = found;
  return found;
}

tag_iterator CHeader::FindTag(string const &key) {
  auto pred = [key](Tag const & tag) { return tag.key == key; };

  // auto ibegin = tags_.begin();
  // auto iend = tags_.end();
  tag_iterator ibegin = tags_.begin();
  tag_iterator iend = tags_.end();
  // auto found = std::find_if(ibegin, iend, pred);
  tag_iterator found = std::find_if(ibegin, iend, pred);
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
    // it->value.assign(val);
    it->value = val;
  }
  return ret;
}

// Fill in val if tag is found.
// Can't make this const as it calls ReadTag (see its doc)

CHeaderError CHeader::ReadChar(string const &key, string &val) const {
  CHeaderError ret = CHeaderError::OK;
  const_tag_iterator it = FindTag(key);
  if (it == std::end(tags_)) {
    string file_name;
    GetFileName(file_name);
    LOG_ERROR("Tag not found: '{}' in {}", key, file_name);
    ret = CHeaderError::TAG_NOT_FOUND;
  } else {
    val = it->value;
  }
  LOG_TRACE("val {} ret {}", val, to_underlying(ret) );
  return ret;
}

/**
 * @brief      Convert string buffer to numeric value of type T
 * @param[in]  str   string to convert
 * @param      val   numeric return value
 * @tparam     T     Numeric type to convert to: int, float, int64_t
 * @return     0 on success, else 1
 */
template <typename T>CHeaderError CHeader::convertString(string &str, T &val) const {
  CHeaderError ret = CHeaderError::OK;
  try {
    val = boost::lexical_cast<T>(str);
  }
  catch (boost::bad_lexical_cast const &e) {
    LOG_ERROR("Lexical cast: {}",  e.what());
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
CHeaderError CHeader::ReadDate(std::string const &t_tag, std::unique_ptr<DateTime> &t_date_time) const {
  return (ReadDateTime(t_tag, DateTime::Format::ecat_date, t_date_time) == CHeaderError::OK) ? CHeaderError::OK : CHeaderError::NOT_A_DATE;
}

/**
 * @brief      Read time from element with given tag
 * @param[in]  tag   Tag to read
 * @param      time  Time stored in the tag
 * @return     0 on success, else 1
 */
CHeaderError CHeader::ReadTime(std::string const &t_tag, std::unique_ptr<DateTime> &t_date_time) const {
  return (ReadDateTime(t_tag, DateTime::Format::ecat_time, t_date_time) == CHeaderError::OK) ? CHeaderError::OK : CHeaderError::NOT_A_TIME;
}

/**
 * @param t_tag
 * @param t_format
 * @param t_pt
 * @return CHeaderError 
 */
CHeaderError CHeader::ReadDateTime(string const &t_tag, DateTime::Format t_format, std::unique_ptr<DateTime> &t_date_time) const {
  string value;
  CHeaderError ret = CHeaderError::OK;
  if ((ret = ReadChar(t_tag, value)) == CHeaderError::OK) {
    // ret = ParseInterfileDatetime(value, t_format, t_pt) ? CHeaderError::ERROR : CHeaderError::OK;
    t_date_time = DateTime::Create(value, t_format);
    std::string date_time_str = (t_date_time) ? t_date_time->ToString(t_format) : "Not a DateTime";
    LOG_DEBUG("t_tag {} value {} format {} t_date_time {}", t_tag, value, DateTime::FormatString(t_format), date_time_str);
    ret = t_date_time ? ret : CHeaderError::ERROR;
  }
  return ret;
}

CHeaderError CHeader::WriteDateTime(string const &t_tag, DateTime::Format t_format, string const &t_date_time_str) {
  CHeaderError ret = CHeaderError::ERROR;

 std::unique_ptr<DateTime> date_time = DateTime::Create(t_date_time_str, t_format);
 if (date_time)
   ret = WriteDateTime(t_tag, t_format, date_time);
  return ret;
}

// Write a date or time to given tag in given format.
// Note that date is checked: All times should have a valid date.

CHeaderError CHeader::WriteDateTime(string const &t_tag, DateTime::Format t_format, std::unique_ptr<DateTime> &t_date_time) {
  CHeaderError ret = CHeaderError::ERROR;
  if (t_date_time->IsValid()) {
    std::string date_time_str = t_date_time->ToString(t_format);
    ret = WriteChar(t_tag, date_time_str);
  }
  return ret;
}

CHeaderError CHeader::WriteDate(string const &t_tag, string const &t_date_time_str) {
  return WriteDateTime(t_tag, DateTime::Format::ecat_date, t_date_time_str);
}

CHeaderError CHeader::WriteDate(string const &t_tag, std::unique_ptr<DateTime> t_date_time) {
  return WriteDateTime(t_tag, DateTime::Format::ecat_date, t_date_time);
}

CHeaderError CHeader::WriteTime(string const &t_tag, string const &t_date_time_str) {
  return WriteDateTime(t_tag, DateTime::Format::ecat_time, t_date_time_str);
}

CHeaderError CHeader::WriteTime(string const &t_tag, std::unique_ptr<DateTime> t_date_time) {
  return WriteDateTime(t_tag, DateTime::Format::ecat_time, t_date_time);
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

template <typename T>CHeaderError CHeader::ReadNum(string const &tag, T &val) const {
  string str;
  CHeaderError result = ReadChar(tag, str);
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

CHeaderError CHeader::ReadInt(string const &tag, int &val) const {
  return ReadNum<int>(tag, val);
}

CHeaderError CHeader::ReadLong(string const &tag, long &val) const {
  return ReadNum<long>(tag, val);
}

CHeaderError CHeader::ReadFloat(string const &tag, float &val) const {
  return ReadNum<float>(tag, val);
}

CHeaderError CHeader::ReadDouble(string const &tag, double &val) const {
  return ReadNum<double>(tag, val);
}

