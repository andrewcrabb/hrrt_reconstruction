#pragma once

#include <boost::filesystem>

class MatrixFile {
// Adapted from a struct in ecat_matrix.hpp, so make everything public for now.
public:
  // char        *fname ;  /* file path */
  boost::filesystem::path fname;
  Main_header *mhptr ;  /* pointer to main header */
  MatDirList  *dirlist ;  /* directory */
  // FILE        *fptr ;   /* file ptr for I/O calls */
  std::fstream fptr;
  FileFormat  file_format;
  // char        **interfile_header;   // TODO reimplement as Interfile class or vector<interfile::Key, std::string> - see analyze.cpp:200
  std::vector<interfile::Key, std::string> interfile_header;
  void        *analyze_hdr;

  int read_bytes(void *where, int howmany);
  int set_read_position(int pos);
  int set_header(std::string const &str);
};
