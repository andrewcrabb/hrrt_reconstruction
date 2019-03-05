/*
  hrrt_util.h
  Common utilities for HRRT executables
  ahc 11/2/17
  */

#pragma once

#include <string>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

namespace hrrt_util {

  // DateFormat / date_formats_ define date formats for parsing into Boost.

  enum class DateFormat {
    ecat_date,     // 18:09:2017
    ecat_time,     // 15:26:00
    ecat_datetime
  };

  static std::map<DateFormat, std::string> date_formats_ = {
    {DateFormat::ecat_date    , "%d:%m:%Y"},          // !study date (dd:mm:yryr) := 18:09:2017
    {DateFormat::ecat_time    , "%H:%M:%S"},          // !study time (hh:mm:ss) := 15:26:00
    {DateFormat::ecat_datetime, "%d:%m:%Y %H:%M:%S"}  //
  };

// string const ECAT_DATE_FORMAT = "%d:%m:%Y";  
// string const ECAT_TIME_FORMAT = "%H:%M:%S";  
// string const ECAT_DATETIME_FORMAT = "%d:%m:%Y %H:%M:%S";  

extern int nthreads_;

void swaw( short *from, short *to, int length);
// Utility file-open
int open_istream(std::ifstream &ifstr, const boost::filesystem::path &name, std::ios_base::openmode t_mode = std::ios::in );
int open_ostream(std::ofstream &ofstr, const boost::filesystem::path &name, std::ios_base::openmode t_mode = std::ios::out );
void GetSystemInformation(void);
template <typename T> int write_binary_file(T *t_data, int t_num_elems, boost::filesystem::path const &outpath, std::string const &msg);
// bool parse_interfile_line(const std::string &line, std::string &key, std::string &value);
extern int run_system_command( char *prog, char *args, FILE *log_fp );
extern  bool file_exists (const std::string& name);
std::string time_string(void);
std::istream& safeGetline(std::istream& is, std::string& t);
  // Class methods
  bool parse_interfile_datetime(const std::string &datestr, DateFormat t_format, boost::posix_time::ptime &pt);
  bool parse_interfile_time(const std::string &timestr, boost::posix_time::ptime &pt);
  bool parse_interfile_date(const std::string &datestr, boost::posix_time::ptime &pt);
}
