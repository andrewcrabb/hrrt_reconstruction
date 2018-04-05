//** file: global_tmpl.cpp
//** date: 2002/11/04

//** author: Frank Kehren
//** © CPS Innovations

#include <iostream>
// #include <mathimf.h>
#include <cmath>

#include "global_tmpl.h"


/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Emi2Tra: convert emission into transmission by applying exp-function      */
/*  data   pointer to sinogram                                               */
/*  size   size of dataset                                                   */
/*---------------------------------------------------------------------------*/
template <typename T>
void Emi2Tra(T * const data, const unsigned long int size)
 { for (unsigned long int i=0; i < size; i++)
    if (data[i] < 0) data[i]=1.0;                    // cut-off negative values
     else data[i]=(T)exp(data[i]);
 }

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
