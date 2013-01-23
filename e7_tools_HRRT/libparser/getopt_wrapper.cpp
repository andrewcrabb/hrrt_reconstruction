/*! \file getopt_wrapper.cpp
    \brief This module rebuilds functions to parse the command line.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/24 added Doxygen style comments

    This module rebuilds functions to parse the command line, for cases where
    these are not part of the system libraries. These functions don't do the
    same error handling as the originals, but should work in the normal
    situations.
 */

#include <iostream>
#include <cstring>
#define _GETOPT_WRAPPER_CPP
#include "getopt_wrapper.h"

#if defined(WIN32) || defined(__SOLARIS__)

/*- global variables --------------------------------------------------------*/

char *optarg;                                /*!< argument of current switch */
int optind=1;                                          /*!< argument counter */
static char zero=0;    /*!< used as argument if switch doesn't have argument */

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Parse next command line switch.
    \param[in] argc        number of arguments
    \param[in] argv        list of arguments
    \param[in] optstring   allowed short switch
    \return short switch as character

    Parse next command line switch. This function is only used if all allowed
    switches consist of one character (short switches). If no more switches can
    be parsed or a switch is invalid, the function returns -1.
 */
/*---------------------------------------------------------------------------*/
int getopt(int argc, char *argv[], const char *optstring)
 { if (optind >= argc) return(-1);                          // no more switches
                                                // is argument a short switch ?
   if ((strlen(argv[optind]) == 2) && (argv[optind][0] == '-'))
    if (strstr(optstring, argv[optind]+1) != NULL)// is switch a valid switch ?
     { if (optind < argc-1)
        { optarg=argv[(++optind)++];              // next parameter is argument
          return((int)argv[optind-2][1]);                      // return switch
        }
       optarg=&zero;                                            // no parameter
       return((int)argv[optind++][1]);                         // return switch
     }
   return(-1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Parse next command line switch.
    \param[in]     argc        number of arguments
    \param[in]     argv        list of arguments
    \param[in]     optstring   allowed short switch
    \param[in]     longopts    list of allowed long options
    \param[in,out] longindex   number of long option found
    \return 0 if a long switch was found, and a character for a short switch

    Parse next command line switch. This function is used if at least one of
    the allowed switches consist of more than one character (long switches).
    If no more switches can be parsed or a switch is invalid, the function
    returns -1.
 */
/*---------------------------------------------------------------------------*/
int getopt_long(int argc, char *argv[], const char *optstring,
                const struct option *longopts, int *longindex)
 { if (optind >= argc) return(-1);                          // no more switches
                                                          // check long options
   *longindex=0;
   while (longopts[*longindex].name != 0)
    {  if (strcmp(longopts[*longindex].name, argv[optind]+2) == 0)
       { if (longopts[*longindex].has_arg) optarg=argv[(++optind)++];
          else optarg=argv[++optind];
         return(0);
       }
      (*longindex)++;
    }
   const char *cp;
                                                // is argument a short switch ?
   if ((strlen(argv[optind]) == 2) && (argv[optind][0] == '-'))
                                                  // is switch a valid switch ?
    if ((cp=strstr(optstring, argv[optind]+1)) != NULL)
     if (cp[1] == ':')
      { optarg=argv[(++optind)++];                // next parameter is argument
        return((int)argv[optind-2][1]);
      }
      else { optarg=&zero;                                      // no parameter
             return((int)argv[optind++][1]);                   // return switch
           }
   return(-2);
 }
#endif
