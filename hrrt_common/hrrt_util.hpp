/*
  hrrt_util.h
  Common utilities for HRRT executables
  ahc 11/2/17
  */

#pragma once

#include <string>
#include <boost/date_time.hpp>

namespace hrrt_util {
// Utility file-open
int open_istream(std::ifstream &ifstr, const bf::path &name, std::ios_base::openmode t_mode = std::ios::in );
int open_ostream(std::ofstream &ofstr, const bf::path &name, std::ios_base::openmode t_mode = std::ios::out );
bool parse_interfile_line(const std::string &line, std::string &key, std::string &value);
extern int run_system_command( char *prog, char *args, FILE *log_fp );
extern  bool file_exists (const std::string& name);
std::istream& safeGetline(std::istream& is, std::string& t);
}
