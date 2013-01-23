/*! \file global_tmpl.h
    \brief This module contains some simple function templates.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/28 added Doxygen style comments
 \date 2005/01/31 added splitStr() function
 */

#ifndef _GLOBAL_TMPL_H
#define _GLOBAL_TMPL_H

#include <string>
#include <vector>

/*- exported functions ------------------------------------------------------*/

#ifdef WIN32
#undef max
#undef min

//namespace std
// {
     template <typename T> const T& mymax(const T&, const T&);      // maximum of two values
     template <typename T> const T& mymin(const T&, const T&);      // minimum of two values
// }
#endif
template <typename T>          // print minimum, maximum and total of a dataset
 void printStat(const std::string, const unsigned short int, T * const,
                const unsigned long int, T *, T *, double *);
template <typename T> signed long int round(const T a);          // round value
#ifdef WIN32
template <typename T>        // split ',' separated string into array of values
 std::vector <T> splitStr(std::string);
#endif

#ifndef _GLOBAL_TMPL_CPP
#define _GLOBAL_TMPL2_CPP
#include "global_tmpl.cpp"
#endif

#endif
