/*
  hrrt_util.h
  Common utilities for HRRT executables
  ahc 11/2/17
  */

#pragma once

#include <string>
#include <boost/date_time.hpp>

bool parse_interfile_line(const std::string &line, std::string &key, std::string &value);
bool parse_interfile_datetime(const std::string &datestr, const std::string &format, boost::posix_time::ptime &pt);
bool parse_interfile_time(const std::string &timestr, boost::posix_time::ptime &pt);
bool parse_interfile_date(const std::string &datestr, boost::posix_time::ptime &pt);
extern int run_system_command( char *prog, char *args, FILE *log_fp );
extern  bool file_exists (const std::string& name);
std::istream& safeGetline(std::istream& is, std::string& t);
