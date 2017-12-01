/*
  hrrt_util.h
  Common utilities for HRRT executables
  ahc 11/2/17
  */

#ifndef HRRTUtil_h
#define HRRTUtil_h
extern int run_system_command( char *prog, char *args, FILE *log_fp );
extern  bool file_exists (const std::string& name);
#endif
