/*! \file swap_tmpl.h
    \brief This module provides functions to swap between little and big
           endian.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
 */

#pragma once

/*- exported functions ------------------------------------------------------*/

                                              // is this a big endian machine ?
bool BigEndianMachine(void);

template <typename T> void Swap(T *);                              // swap data
