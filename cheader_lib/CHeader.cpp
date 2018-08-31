// CHeader.cpp: implementation of the CHeader class.
//
//      Copyright (C) CPS Innovations 2004 All Rights Reserved.
//////////////////////////////////////////////////////////////////////

/*
Modification history:
          01/07/2009 : Warnings cleaning
          24-JUL-2009: Use const char for keywords
          20180830 ahc Convert to standard C++11
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
#include <fmt/format.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;

CHeader::CHeader() {
  //
}

CHeader::~CHeader()
{
  if (hdr_file.is_open())
    hdr_file.close();
}

/**
 * Loads specified filename in memory table.
 * Returns 1 if OK or a negative number (-ERR_CODE) otherwise.
 * Error codes:
 * HdrErrors[-ERR_CODE] contains the text error message.
 */

int CHeader::OpenFile(const string &filename) {
  if (hdr_file.is_open())
    return (E_FILE_ALREADY_OPEN);
  hdr_file.open(filename, std::ifstream::in);
  if (!hdr_file.is_open())
    return (E_COULD_NOT_OPEN_FILE);
  m_FileName.assign(filename);
  return ReadFile();
}


int CHeader::InsertTag(string t_buffer) {
  cout << "CHeader::Inserttag(" << t_buffer << ")" << endl;
  using namespace boost::xpressive;

  smatch match;
  sregex m_reg = sregex::compile("^\\s*(?P<key>.+)\\s+:=\\s+(?P<value>.+)\\s*$");
  if (regex_match(t_buffer, match, m_reg)) {
    tags.push_back({match["key"], match["value"]});
  }

  return 0;
}

int CHeader::ReadFile() {
  string buffer;
  while (safeGetline(hdr_file, buffer))
    InsertTag(buffer);
  return OK;
}

int CHeader::WriteFile(const string &fname) {
  string outname = (fname.length()) ? fname : m_FileName;
  std::ofstream fp;

  fp.open(outname, std::ofstream::out);
  if (!fp.is_open()) {
    return 1;
  }

  fp << "!INTERFILE := \n";
  for (auto &tag : tags) {
    fp << tag.key << " := " << tag.value << endl;
  }
  fp.close();

  return 0;
}

int CHeader::IsFileOpen() {
  return hdr_file.is_open();
}

void CHeader::GetFileName(string &filename) {
  if (IsFileOpen())
    filename = m_FileName;
}

int CHeader::CloseFile() {
  if (IsFileOpen()) {
    hdr_file.close();
    m_FileName.erase();
  }
  return OK;
}

// Locate tag having given key

tag_iterator CHeader::FindTag(const string &key) {
  auto pred = [key](const Tag & tag) { return tag.key == key; };
  return std::find_if(std::begin(tags), std::end(tags), pred);
}

// TODO: Rename these silent overloads.  They invite trouble.

int CHeader::WriteTag(const string &key, double val) {
  string str = fmt::format("{:f}", val);
  return WriteTag(key, str);
}

int CHeader::WriteTag(const string &key, int val) {
  string str = fmt::format("{:d}", val);
  return WriteTag(key, str);
}

int CHeader::WriteTag(const string &key, int64_t val) {
  string str = fmt::format("{:d}", val);
  return WriteTag(key, str);
}

// If tag with given key is found, update its value.
// Else append new tag with given key and value.

int CHeader::WriteTag(const string &key, const string &val) {
  int ret = 0;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags)) {
    tags.push_back({key, val});
    ret = 1;
  } else {
    it->value.assign(val);
  }
  return ret;
}

// Fill in val if tag is found.

int CHeader::Readchar(const string &key, string &val) {
  int ret = OK;
  tag_iterator it = FindTag(key);
  if (it == std::end(tags)) {
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
template <typename T>int CHeader::convertString(const string &str, T &val) {
  int ret = OK;
  cout << "convertString<" << typeid(T).name() << ">(" << str << "): ";
  try {
    val = boost::lexical_cast<T>(str);
  }
  catch (const boost::bad_lexical_cast &e) {
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
int CHeader::ReadTime(const string &tag, boost::posix_time::ptime &time) {
  string line, key, value;
  int ret = 0;
  if (Readchar(tag, line) == OK) {
    if (!parse_interfile_line(line, key, value)) {
      ret = parse_interfile_time(value, time) ? 1 : 0;
    }
  }
  return ret;
}

// Read given tag and return its numeric value
// Return 0 on success, else 1

template <typename T>int CHeader::ReadNum(const string &tag, T &val) {
  string str;
  int result = Readchar(tag, str);
  if (result == OK) {
    result = convertString<T>(str, val);
  }
  return result;

}

int CHeader::Readint(const string &tag, int &val) {
  return ReadNum<int>(tag, val) ? E_NOT_AN_INIT : OK;
}

int CHeader::Readlong(const string &tag, long &val) {
  return ReadNum<long>(tag, val) ? E_NOT_A_LONG : OK;
}

int CHeader::Readfloat(const string &tag, float &val) {
  return ReadNum<float>(tag, val) ? E_NOT_A_FLOAT : OK;
}

int CHeader::Readdouble(const string &tag, double &val) {
  return ReadNum<double>(tag, val) ? E_NOT_A_DOUBLE : OK;
}
