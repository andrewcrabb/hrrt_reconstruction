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
#include <sys/sysinfo.h>

#include "hrrt_util.hpp"
#include "my_spdlog.hpp"
#include <boost/filesystem.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace bf = boost::filesystem;
namespace bt = boost::posix_time;
namespace bx = boost::xpressive;

namespace hrrt_util {

  int nthreads_;

void swaw( short *from, short *to, int length) {
  short int temp;

  for (int i = 0; i < length; i += 2) {
    temp = from[i + 1];
    to[i + 1] = from[i];
    to[i] = temp;
  }
}

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
      LOG_ERROR("*** ma_pattern file exists: '%s'\n", ma_pattern_name);      
      if (remove(ma_pattern_name) != 0) {
        LOG_ERROR("*** ERROR: Removing ma_pattern file: '%s'\n", ma_pattern_name);
      } else {
        LOG_ERROR("*** Removed ma_pattern file OK: '%s'\n", ma_pattern_name);
      }
    } else {
      LOG_ERROR("*** ma_pattern file '%s' does not exist\n", ma_pattern_name);
    }
  } else {
    LOG_ERROR("*** Could not determine HOME envt var: Cannot remove ma_pattern.dat file\n");
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

void GetSystemInformation() {
  struct sysinfo sinfo;
  LOG_INFO(" Hardware information: ");  
  sysinfo(&sinfo);
  LOG_INFO("  Total RAM: {}", sinfo.totalram); 
  LOG_INFO("  Free RAM: {}", sinfo.freeram);
  // To get n_processors we need to get the result of this command "grep processor /proc/cpuinfo | wc -l"
  if(nthreads_ == 0) 
    nthreads_ = 2; // for now
}

// Replacement for matrix_extra:: find_bmax, find_bmin, find_smax, find_smin, find_imax, find_imin, find_fmin, find_fmax

template <typename T> struct Extrema {
  T min;
  T max;
};

template <typename T> Extrema<T> find_extrema(T *t_values, int t_num_values) {
  T min = t_values[0];
  T max = t_values[0];

  for (int i = 0; i < t_num_values; i++) {
    T test_val = t_values[i];
    if (test_val < min)
      min = test_val;
    if (test_val > max)
      max = test_val;
  }
  Extrema<T> ret{min, max};
  return ret;
}

// unsigned char  find_bmax(const unsigned char  *bdata, int nvals) {
//   int i;
//   unsigned char  bmax = bdata[0];
//   for (i=1; i<nvals; i++)
//     if (bdata[i] > bmax) bmax = bdata[i];
//   return bmax;
// }


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
bool parse_interfile_datetime(const std::string &t_datestr, DateFormat t_format, bt::ptime &t_pt) {
  std::string format_string = date_formats_[t_format];
  bt::time_input_facet *facet = new bt::time_input_facet(format_string);
  const std::locale loc(std::locale::classic(), facet);

  LOG_DEBUG("t_datestr {} format_string {}", t_datestr, format_string);
  std::istringstream iss(t_datestr);
  iss.imbue(loc);

  iss >> t_pt;
  bool ret = t_pt.is_not_a_date_time();
  std::string timestr("FILLMEIN");
  // LOG_DEBUG("t_datestr {} posix_time {} returning {}", t_datestr, bt::to_simple_string(t_pt), ret ? "true" : "false");
  LOG_DEBUG("t_datestr {} posix_time {} returning {}", t_datestr, timestr, ret ? "true" : "false");
  return ret;
}

bool parse_interfile_date(std::string const &datestr, bt::ptime &pt) {
  return parse_interfile_datetime(datestr, DateFormat::ecat_date, pt);
}

bool parse_interfile_time(std::string const &timestr, bt::ptime &pt) {
  return parse_interfile_datetime(timestr, DateFormat::ecat_time, pt);
}

// Convert ptime to time/date string in given format
// TODO: This doesn't do any error checking

CHeaderError CHeader::PTimeToString(bt::ptime const &t_ptime, hrrt_util::DateFormat t_format, string &t_datetime) {
  bt::time_facet *tfacet = new bt::time_facet();
  tfacet->format(t_format.c_str());
  std::ostringstream oss;
  oss.imbue(std::locale(oss.getloc(), tfacet));
  oss << t_ptime;
  t_datetime = oss.str();
  LOG_TRACE("ptime {} format {} returning {}", bt::to_iso_string(t_ptime), t_format, t_datetime);
  return CHeaderError::OK;
}

// Convert datetime string to a ptime
// Can't convert a time by itself: must be a datetime

CHeaderError CHeader::StringToPTime(string const &t_datestr, string const &t_format, bt::ptime &t_datetime) {
  bt::time_input_facet *tfacet = new bt::time_input_facet();
  tfacet->format(t_format.c_str());
  std::istringstream iss(t_datestr);
  iss.imbue(std::locale(iss.getloc(), tfacet));
  // bt::ptime the_datetime;
  iss >> t_datetime;
  CHeaderError ret = CHeaderError::OK;
  if (t_datetime.is_not_a_date_time()) {
    ret = CHeaderError::INVALID_DATE;
    LOG_ERROR("datestr {} format {} ptime {}", iss.str(), t_format, bt::to_iso_string(t_datetime) );
  } else {
    LOG_TRACE("datestr {} format {} ptime {}", iss.str(), t_format, bt::to_iso_string(t_datetime) );
    // t_datetime = t_datetime;
  }
  return ret;
}

// Check that given date is between 1900 and 2100.

CHeaderError CHeader::ValidDate(bt::ptime const &t_datetime) {
  static bt::ptime low_time(boost::gregorian::date(1900,01,01), bt::time_duration(0,0,0));
  static bt::ptime high_time(boost::gregorian::date(2099,12,31), bt::time_duration(24,0,0));

  CHeaderError ret = ((t_datetime < low_time) || (t_datetime > high_time)) ? CHeaderError::INVALID_DATE : CHeaderError::OK;
  if (ret != CHeaderError::OK) {
    string str;
    PTimeToString(t_datetime, ECAT_DATE_FORMAT, str);
    LOG_DEBUG("{}: {}", CHdrErrorString[ret], str);
  }
  return ret;
}

}