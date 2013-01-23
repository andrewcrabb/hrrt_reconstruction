#ifndef ecatx_getopt_h
#define ecatx_getopt_h

#if defined(unix) || defined(__CYGWIN__)
#include <unistd.h>
#else
#if defined(__cplusplus)
extern "C" int getopt(int argc, char **argv, char *opts);
extern "C" int	opterr, optind, optopt;
extern "C" char	*optarg;
#else
extern int getopt(int argc, char **argv, char *opts);
extern int	opterr, optind, optopt;
extern char	*optarg;
#endif

#endif /* unix */

#endif /*ecatx_getopt_h */
