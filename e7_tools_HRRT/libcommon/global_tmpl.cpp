/*! \file global_tmpl.cpp
    \brief This module contains some simple function templates.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/28 added Doxygen style comments
 \date 2005/01/31 added splitStr() function

    This module contains some simple function templates.
 */

#include <iostream>
#include "global_tmpl.h"
#include "fastmath.h"
#include "logging.h"

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Print minimum, maximum and total of a dataset.
    \param[in]  logstr     string for logging
    \param[in]  loglevel   level for logging
    \param[in]  data       dataset
    \param[in]  size       size of dataset
    \param[out] minv       minimum value
    \param[out] maxv       maximum value
    \param[out] sum        sum of values

    Print minimum, maximum and total of a dataset.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void printStat(const std::string logstr, const unsigned short int loglevel,
               T * const data, const unsigned long int size, T *minv,
               T *maxv, double *sum)
 { double _sum=0.0;
   T _minv, _maxv;

   _minv=data[0];
   _maxv=data[0];
   for (unsigned long int i=1; i < size; i++)
    { if (data[i] < _minv) _minv=data[i];
       else if (data[i] > _maxv) _maxv=data[i];
      _sum+=(double)data[i];
    }
   Logging::flog()->logMsg(logstr+": min=#1  max=#2  total=#3", loglevel)->
    arg(_minv)->arg(_maxv)->arg(_sum);
   if (minv != NULL) *minv=_minv;
   if (maxv != NULL) *maxv=_maxv;
   if (sum != NULL) *sum=_sum;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Round a value to the nearest integer.
    \param[in] a   value
    \return rounded value

    Round a value to the nearest integer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
inline signed long int round(const T a) { 
  if (a >= 0) 
    return((signed long int)(a+0.5));
  return((signed long int)(a-0.5));
 }

