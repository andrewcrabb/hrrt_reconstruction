/*! \file swap_tmpl.cpp
    \brief This module provides functions to swap between little and big
           endian.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
*/

#ifndef _SWAP_TMPL_CPP
#define _SWAP_TMPL_CPP
#include "swap_tmpl.h"
#include <algorithm>
#endif

/*- exported functions ------------------------------------------------------*/

#ifndef _SWAP_TMPL2_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Is this a big endian machine ?
    \return big endian machine

    Is this a big endian machine ?
 */
/*---------------------------------------------------------------------------*/
bool BigEndianMachine(void)
 { char *p;
   short int swaptest=0x1234;

   p=(char *)&swaptest;                              // pointer to value 0x1234
   return(*p == 0x12);            // first byte in memory is 0x12 => big endian
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Swap value between big and little endian.
    \param[in,out] value   value to swap

    Swap value between big and little endian.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Swap(T *value)
 { char *cp;
   unsigned short int len;

   cp=(char *)value;
   len=sizeof(T)/2;
   for (unsigned short int i=0; i < len; i++)
    std::swap(cp[i], cp[sizeof(T)-1-i]);
 }
