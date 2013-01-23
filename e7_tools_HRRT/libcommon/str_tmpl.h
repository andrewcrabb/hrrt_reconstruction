/*! \file str_tmpl.h
    \brief This module provides utilities to convert numbers into C++ strings.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments
 */

#ifndef _STR_TMPL_H
#define _STR_TMPL_H

#include <string>

/*- exported functions ------------------------------------------------------*/

                                                   // convert value into string
template <typename T> std::string toString(const T);
                                     // convert value into string of fixed size
template <typename T> std::string toString(const T, const unsigned short int);
                                     // convert integer into hexadecimal string
std::string toStringHex(const unsigned long int);
                  // convert value into string of fixed size with leading zeros
template <typename T> std::string toStringZero(const T,
                                               const unsigned short int);
    // convert integer into hexadecimal string of fixed size with leading zeros
std::string toStringZeroHex(const unsigned long int, const unsigned short int);

#ifndef _STR_CPP
#define _STR_TMPL_CPP
#include "str_tmpl.cpp"
#endif

#endif
