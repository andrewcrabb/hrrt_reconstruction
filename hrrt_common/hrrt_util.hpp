// hrrt_util.h
// Common utilities for HRRT executables
// ahc 11/2/17

#pragma once

#include <string>
#include <boost/filesystem.hpp>

namespace hrrt_util {

// Replacement for matrix_extra:: find_bmax, find_bmin, find_smax, find_smin, find_imax, find_imin, find_fmin, find_fmax

template <typename T> struct Extrema {
  T min;
  T max;
};

// template <typename T> struct ExtremaTestData {
// 	T *data;
// 	int length;
// 	T min;
// 	T max;
// };

// Compatibility function handling T* - intend to migrate to std::vector<T>
template <typename T> Extrema<T> find_extrema(T *t_values, int t_num_values);
template <typename T> bool test_find_extrema(std::vector<T>  &t_data);

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
std::string const BoolToString(bool b);
}
