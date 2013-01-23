/*! \file swap_tmpl.h
    \brief This module provides functions to swap between little and big
           endian.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
 */

#ifndef _SWAP_TMPL_H
#define _SWAP_TMPL_H

/*- exported functions ------------------------------------------------------*/

                                              // is this a big endian machine ?
bool BigEndianMachine(void);

template <typename T> void Swap(T *);                              // swap data

#ifndef _SWAP_TMPL_CPP
#define _SWAP_TMPL2_CPP
#include "swap_tmpl.cpp"
#endif

#endif
