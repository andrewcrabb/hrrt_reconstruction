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

