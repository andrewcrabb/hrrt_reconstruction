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
#include "Errors.hpp"
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

int CHeader::OpenFile(string const &filename) {
  LOG_DEBUG(logger_, filename);
  if (hdr_file_.is_open())
    return (E_FILE_ALREADY_OPEN);
  hdr_file_.open(filename, std::ifstream::in);
  if (!hdr_file_.is_open())
    return (E_COULD_NOT_OPEN_FILE);
  m_FileName_.assign(filename);
  return ReadFile();
}

int CHeader::OpenFile(bf::path &filename) {
  LOG_DEBUG(logger_, filename.string());
  return OpenFile(filename.string());
}

/**
 * @brief      Add to tags_ a new Tag from pattern "key := value" in buffer
 * @param[in]  t_buffer  Line from input file
 * @return     E_OK, or one of the E_foo error values
 */

int CHeader::InsertTag(string t_buffer) {
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
  return 0;
}

int CHeader::ReadFile() {
  LOG_DEBUG(logger_, "ReadFile");
  string buffer;
  while (safeGetline(hdr_file_, buffer)) {
    InsertTag(buffer);
  }
  return E_OK;
}

int CHeader::WriteFile(bf::path const &fname) {
  LOG_DEBUG(logger_, "fname {}", fname.string());
  string s(fname.string());
  WriteFile(s);  
}

int CHeader::WriteFile(string &fname) {
  string outname = (fname.length()) ? fname : m_FileName_;
  LOG_DEBUG(logger_, "tag count {}, outname {}", tags_.size(), outname);
  std::ofstream fp;

  if (tags_.size() == 0)
    return E_COULD_NOT_OPEN_FILE;
  fp.open(outname, std::ofstream::out);
  if (!fp.is_open()) {
    return E_COULD_NOT_OPEN_FILE;
  }

  fp << "!INTERFILE := \n";
  for (auto &tag : tags_) {
    fp << tag.key << " := " << tag.value << endl;
  }
  if (fp.fail()) {
    return E_COULD_NOT_OPEN_FILE;
  }
  fp.close();

  return E_OK;
}

int CHeader::NumTags(void) {
  return tags_.size();
}

int CHeader::IsFileOpen() {
  return hdr_file_.is_open();
}

void CHeader::GetFileName(string &filename) {
  if (IsFileOpen())
    filename = m_FileName_;
}

int CHeader::CloseFile() {
  if (IsFileOpen()) {
    hdr_file_.close();
    m_FileName_.erase();
  }
  return E_OK;
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

// TODO: Rename these silent overloads.  They invite trouble.

int CHeader::WriteTag(string &key, double val) {
  string str = fmt::format("{:f}", val);
  return WriteTag(key, str);
}

int CHeader::WriteTag(string &key, int val) {
  string str = fmt::format("{:d}", val);
  return WriteTag(key, str);
}

int CHeader::WriteTag(string &key, int64_t val) {
  string str = fmt::format("{:d}", val);
  return WriteTag(key, str);
}

int CHeader::WriteTag(string &key, bf::path const &val) {
  WriteTag(key, val.string());  
}

int CHeader::WriteTag(string &key, char const *val) {
  string str(val);
  WriteTag(key, str);
}

// If tag with given key is found, update its value.
// Else append new tag with given key and value.

int CHeader::WriteTag(string &key, string &val) {
  int ret = 0;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags_)) {
    tags_.push_back({key, val});
    ret = 1;
  } else {
    it->value.assign(val);
  }
  return ret;
}

// Fill in val if tag is found.

int CHeader::Readchar(string const &key, string &val) {
  int ret = E_OK;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags_)) {
    ret = E_TAG_NOT_FOUND;
  } else {
    val = it->value;
  }
  return ret;
}

/**
 * @brief      Convert string buffer to numeric value of type T
 * @param[in]  str   string to convert
 * @param      val   numeric return value
 * @tparam     T     Numeric type to convert to: int, float, int64_t
 * @return     0 on success, else 1
 */
template <typename T>int CHeader::convertString(string &str, T &val) {
  int ret = E_OK;
  cout << "convertString<" << typeid(T).name() << ">(" << str << "): ";
  try {
    val = boost::lexical_cast<T>(str);
  }
  catch (boost::bad_lexical_cast const &e) {
    std::cerr << e.what() << endl;
    ret = 1;
  }
  return ret;
}

/**
 * @brief      Read time from element with given tag
 * @param[in]  tag   Tag to read
 * @param      time  Time stored in the tag
 * @return     0 on success, else 1
 */
int CHeader::ReadTime(string &tag, boost::posix_time::ptime &time) {
  string line, key, value;
  int ret = 0;
  if (Readchar(tag, line) == E_OK) {
    if (!parse_interfile_line(line, key, value)) {
      ret = parse_interfile_time(value, time) ? 1 : 0;
    }
  }
  return ret;
}

// Open file, read given value, close file.

template <typename T>int CHeader::ReadHeaderNum(string &filename, string &tag, T &val) {
  int result;
  if (OpenFile(filename) == E_OK) {
    result = ReadNum<T>(tag, val);
    CloseFile();
  }
  return result;
}


// Read given tag and return its numeric value
// Return 0 on success, else 1

template <typename T>int CHeader::ReadNum(string &tag, T &val) {
  string str;
  int result = Readchar(tag, str);
  if (result == E_OK) {
    result = convertString<T>(str, val);
  }
  return result;
}

int CHeader::Readint(string &tag, int &val) {
  return ReadNum<int>(tag, val) ? E_NOT_AN_INIT : E_OK;
}

int CHeader::Readlong(string &tag, long &val) {
  return ReadNum<long>(tag, val) ? E_NOT_A_LONG : E_OK;
}

int CHeader::Readfloat(string &tag, float &val) {
  return ReadNum<float>(tag, val) ? E_NOT_A_FLOAT : E_OK;
}

int CHeader::Readdouble(string &tag, double &val) {
  return ReadNum<double>(tag, val) ? E_NOT_A_DOUBLE : E_OK;
}
