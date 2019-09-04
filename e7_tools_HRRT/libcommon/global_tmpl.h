/*! \file global_tmpl.h
    \brief This module contains some simple function templates.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/28 added Doxygen style comments
 \date 2005/01/31 added splitStr() function
 */

#pragma once

#include <string>
#include <vector>

/*- exported functions ------------------------------------------------------*/

template <typename T>          // print minimum, maximum and total of a dataset
 void printStat(const std::string, const unsigned short int, T * const,
                const unsigned long int, T *, T *, double *);
template <typename T> signed long int round(const T a);          // round value
