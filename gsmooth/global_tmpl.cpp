//** file: global_tmpl.cpp
//** date: 2002/11/04

//** author: Frank Kehren
//** © CPS Innovations

#include <iostream>
#if defined(__linux__) && defined(__INTEL_COMPILER)
#include <mathimf.h>
#else
#include <cmath>
#endif
#ifndef _GLOBAL_TMPL_CPP
#define _GLOBAL_TMPL_CPP
#include "global_tmpl.h"
#endif

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Emi2Tra: convert emission into transmission by applying exp-function      */
/*  data   pointer to sinogram                                               */
/*  size   size of dataset                                                   */
/*---------------------------------------------------------------------------*/
template <typename T>
void Emi2Tra(T * const data, const unsigned long int size)
 { for (unsigned long int i=0; i < size; i++)
    if (data[i] < T()) data[i]=1;                   // cut-off negative values
     else data[i]=(T)exp(data[i]);
 }

#ifdef WIN32
/*---------------------------------------------------------------------------*/
/* maxi: return maximum of two values                                        */
/*  a   first value                                                          */
/*  b   second value                                                         */
/* return: maximum                                                           */
/*---------------------------------------------------------------------------*/
template <typename T>
inline T maxi(const T a, const T b)
 { if (a > b) return(a);
   return(b);
 }

/*---------------------------------------------------------------------------*/
/* mini: return minimum of two values                                        */
/*  a   first value                                                          */
/*  b   second value                                                         */
/* return: minimum                                                           */
/*---------------------------------------------------------------------------*/
template <typename T>
inline T mini(const T a, const T b)
 { if (a < b) return(a);
   return(b);
 }
#endif

/*---------------------------------------------------------------------------*/
/* round: round a value to the nearest integer                               */
/*  a   value                                                                */
/* return: rounded value                                                     */
/*---------------------------------------------------------------------------*/
template <typename T>
inline signed long int round(const T a)
 { if (a >= 0) return((signed long int)(a+0.5));
   return((signed long int)(a-0.5));
 }

/*---------------------------------------------------------------------------*/
/* Tra2Emi: convert transmission into emission by applying log-function      */
/*  data   pointer to sinogram                                               */
/*  size   size of dataset                                                   */
/*---------------------------------------------------------------------------*/
template <typename T>
void Tra2Emi(T * const data, const unsigned long int size)
 { for (unsigned long int i=0; i < size; i++)
    if (data[i] > 1.0) data[i]=(T)log(data[i]);
     else data[i]=T();                         // don't produce negative values
 }
