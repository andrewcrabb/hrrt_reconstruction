/*! \file getopt_wrapper.h
    \brief This module rebuilds functions to parse the command line.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/24 added Doxygen style comments
 */

#ifndef _GETOPT_WRAPPER_H
#define _GETOPT_WRAPPER_H

#if defined(__LINUX__) || defined(__MACOSX__)
#include <getopt.h>
#endif

#if defined(WIN32) || defined(__SOLARIS__)
                                             /*! parameters for long options */
struct option { const char *name;                      /*!< name of argument */
                int has_arg;       /*!< has argument additional parameters ? */
                int *flag;                                       /*!< unused */
                int val;                                         /*!< unused */
              };

/*- exported variables ------------------------------------------------------*/

extern char *optarg;                         /*!< argument of current option */
extern int optind;                                     /*!< argument counter */

/*- exported functions ------------------------------------------------------*/

                                     // get next short option from command line
extern int getopt(int argc, char *argv[], const char *);
                                      // get next long option from command line
extern int getopt_long(int, char *[], const char *, const struct option *,
                       int *);
#endif

#endif
