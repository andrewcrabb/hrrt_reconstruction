/*! \file gm_check.cpp
    \brief Check the values of the gantry model.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/08/05 initial version

    This program prints all the values of the gantry model object, that are
    used by any of the E7-Tools.
 */

#include <stdio.h>
#include "e7_common.h"
#include "getopt_wrapper.h"
#include "gm.h"
#include "logging.h"
#include "timestamp.h"

/*---------------------------------------------------------------------------*/
/*! \brief Print usage information.

    Print usage information.
 */
/*---------------------------------------------------------------------------*/
void Usage()
 { std::cout << "\n                                                  ";
#ifdef WIN32
   std::cout << "(c)";
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
   std::cout << "  " << (char)169;
#endif
   std::cout << " 2004-2005 CPS Innovations\n\n"
           "gm_check - check the values of the gantry model\n\n"
           "Usage:\n\n"
           " gm_check model|-v|-h\n\n"
           "  model   gantry model number\n"
           "  -v      print version information\n"
           "  -h      print this information\n" << std::endl;
 }

/*---------------------------------------------------------------------------*/
/*! \brief main function of program to print the values of the gantry model
    \param[in] argc   number of arguments
    \param[in] argv   command line switches
    \return 0 success
    \return 1 unknown failure
    \return error code

    main function of program to print the values of the gantry model. Possible
    command line switches are:
    \verbatim
gm_check - check the values of the gantry model

Usage:

 gm_check model|-v|-h

  model   gantry model number
  -v      print version information
  -h      print this information
    \endverbatim
 */
/*---------------------------------------------------------------------------*/
int main(int argc, char **argv)
 { std::ios::sync_with_stdio(false);
                                                  // parse command line options
   { char c;
     bool no_error=true;

     if (argc > 1)
      while ((c=getopt(argc, argv, "hv")) >= 0)
       switch (c)
        { case 'h':
           Usage();
           return(EXIT_SUCCESS);
          case 'v':
           timestamp(__FILE__, "2004-2005");
           return(EXIT_SUCCESS);
        }
      else { Usage();
             return(EXIT_SUCCESS);
           }
     if (!no_error) return(EXIT_FAILURE);
   }
   Logging::flog()->init("gm_check", 7, std::string(), Logging::LOG_SCREEN);
   GM::gm()->init(std::string(argv[1]));
   GM::gm()->printUsedGMvalues();
   GM::close();
   Logging::close();
   return(EXIT_SUCCESS);
 }
