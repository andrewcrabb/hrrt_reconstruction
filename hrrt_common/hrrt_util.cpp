/*
  hrrt_util.cpp
  Common utilities for HRRT executables
  ahc 11/2/17
  */

#include <cstdio>
#include <iostream>
#include <cstddef>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <boost/xpressive/xpressive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "hrrt_util.hpp"
#include "spdlog/spdlog.h"
#include "my_spdlog.hpp"

namespace bt = boost::posix_time;
namespace bg = boost::gregorian;
namespace bx = boost::xpressive;

using std::cout;
using std::endl;
using std::string;

/**
 * @brief      Parse Interfile line into its key and value
 *             !study date (dd:mm:yryr) := 18:09:2017
 *
 * @param[in]  line  Line from Interfile header
 * @param[out] key   Key of Interfile line
 * @param[out] value Value of Interfile line
 *
 * @return     false on success, else true
 */
bool parse_interfile_line(const string &line, string &key, string &value) {
  auto logger = spdlog::get("CHeader");
  bool ret = true;
  bx::smatch match;
  // using namespace boost::xpressive;
  bx::sregex reg = bx::sregex::compile("^\\s*(?P<key>.+)\\s+:=\\s+(?P<value>.+)\\s*$");
  if (bx::regex_match(line, match, reg)) {
    key   = match["key"];
    value = match["value"];
    ret = false;
    LOG_DEBUG(logger, "key {} value {} ret {}", key, value, "false");
  }
  LOG_ERROR(logger, "Could not parse line: '{}'", line);
  return ret;
}

bool file_exists (const std::string& name) {
  bool ret = false;
  std::ifstream f(name.c_str());
  ret = f.good();
  std::cerr << "*** hrrt_util::file_exists: " << name << " returning " << ret << std::endl << std::flush;
  return ret;
}


// prog:    Program to run
// args:    Argument string to program
// outfile: Optional output file to check for
// Check for existence of given program, then run with given args.
// Returns: 0 on success, else 1

int run_system_command( char *prog, char *args, FILE *log_fp ) {
  char command_line[4096];
  int ret = 0;
  char *ptr = NULL;
  char ma_pattern_name[FILENAME_MAX];
  
  ma_pattern_name[0] = '\0';
  if ((ptr=getenv("HOME")) != NULL) {
    sprintf(ma_pattern_name, "%s/.ma_pattern.dat", ptr);
    if (!access(ma_pattern_name, R_OK)) {
      fprintf(stderr, "*** ma_pattern file exists: '%s'\n", ma_pattern_name);      
      if (remove(ma_pattern_name) != 0) {
        fprintf(stderr, "*** ERROR: Removing ma_pattern file: '%s'\n", ma_pattern_name);
      } else {
        fprintf(stderr, "*** Removed ma_pattern file OK: '%s'\n", ma_pattern_name);
      }
    } else {
      fprintf(stderr, "*** ma_pattern file '%s' does not exist\n", ma_pattern_name);
    }
  } else {
    fprintf(stderr, "*** Could not determine HOME envt var: Cannot remove ma_pattern.dat file\n");
  }
  fflush(stderr);

  ret = access(prog, X_OK);
  if (ret) {
    std::cerr << "Error: run_system_command(" << prog << "): File does not exist" << std::endl << std::flush;    
  } else {
    std::cerr << "*** run_system_command('" << prog << "', '" << args << "')" << std::endl << std::flush;
    sprintf(command_line, "%s %s", prog, args);
    fprintf(log_fp, "%s\n", command_line);
    ret = system(command_line);
  }
  std::cerr << "run_system_command('" << prog << "', '" << args << "') returning " << ret << std::endl << std::flush;
  return(ret);
}

// getline() handling cross-platform line endings.  
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
// 
// The characters in the stream are read one-by-one using a std::streambuf.
// That is faster than reading them one-by-one using the std::istream.
// Code that uses streambuf this way must be guarded by a sentry object.
// The sentry object performs various tasks,
// such as thread synchronization and updating the stream state.

std::istream& safeGetline(std::istream& is, std::string& t) {
    t.clear();
    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case std::streambuf::traits_type::eof():
            // Also handle the case when the last line has no line ending
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;
        default:
            t += (char)c;
        }
    }

    return is;
}
