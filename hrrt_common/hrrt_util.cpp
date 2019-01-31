/*
  hrrt_util.cpp
  Common utilities for HRRT executables
  ahc 11/2/17
  */

#include <iostream>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fstream>

#include "hrrt_util.hpp"
#include "spdlog/spdlog.h"
#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

namespace hrrt_util {

int open_ostream(std::ofstream &t_outs, const bf::path &t_path, std::ios_base::openmode t_mode) {
  t_outs.open(t_path.string(), t_mode);
  if (!t_outs.is_open()) {
    spdlog::get("HRRT")->error("Could not open output file {}", t_path.string());
    return 1;
  }
  return 0;
}

int open_istream(std::ifstream &t_ins, const bf::path &t_path, std::ios_base::openmode t_mode) {
  t_ins.open(t_path.string(), t_mode);
  if (!t_ins.is_open()) {
    spdlog::get("HRRT")->error("Could not open input file {}", t_path.string());
    return 1;
  }
  return 0;
}

template <typename T>
int write_binary_file(T *t_data, int t_num_elems, bf::path const &outpath, std::string const &msg) {
  std::ofstream outstream;
  std::shared_ptr<spdlog::logger> logger = spdlog::get("HRRT");  // Must define logger named HRRT in your main application
  if (open_ostream(outstream, outpath, std::ios::out | std::ios::binary))
    return 1;
  outstream.write(reinterpret_cast<char *>(t_data), t_num_elems * sizeof(T));
  if (!outstream.good()) {
    logger->error("Error {} {} elements stored in {}", msg, t_num_elems, outpath.string());
    outstream.close();
    return 1;
  }
  logger->info("{} {} elements stored in {}", msg, t_num_elems, outpath.string());
  outstream.close();
  return 0;
}

bool file_exists (const std::string& name) {
  bool ret = false;
  std::ifstream f(name.c_str());
  ret = f.good();
  std::cerr << "*** hrrt_util::file_exists: " << name << " returning " << ret << std::endl << std::flush;
  return ret;
}

std::string time_string(void) {
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, 80, "%y%m%d_%H%M%S", timeinfo);
  std::string s(buffer);
  return s;
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

// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf

std::istream& safeGetline(std::istream& is, std::string& t)
{
    t.clear();

    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.

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
}

// Required for symbols to be present in the library file.  Otherwise, won't link.
// https://stackoverflow.com/questions/1022623/c-shared-library-with-templates-undefined-symbols-error

// template <typename T> int write_binary_file(T *t_data, int t_num_elems, boost::filesystem::path const &outpath, std::string const &msg);
// template <float *> int write_binary_file(float *t_data, int t_num_elems, boost::filesystem::path const &outpath, std::string const &msg);
template int write_binary_file<float>(float *t_data, int t_num_elems, boost::filesystem::path const &outpath, std::string const &msg);

}