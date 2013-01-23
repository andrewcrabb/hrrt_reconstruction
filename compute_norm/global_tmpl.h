//** file: global_tmpl.h
//** date: 2002/11/04

//** author: Frank Kehren
//** © CPS Innovations

//** This module contains simple function templates that are used at different
//** places in the program.

#ifndef _GLOBAL_TMPL_H
#define _GLOBAL_TMPL_H

/*- exported functions ------------------------------------------------------*/

                                          // convert emission into transmission
template <typename T> void Emi2Tra(T * const, const unsigned long int);
#ifdef WIN32
template <typename T> T maxi(const T, const T);        // maximum of two values
template <typename T> T mini(const T, const T);        // minimum of two values
#endif
template <typename T> signed long int round(const T a);          // round value
                                          // convert transmission into emission
template <typename T> void Tra2Emi(T * const, const unsigned long int);

#ifndef _GLOBAL_TMPL_CPP
#define _GLOBAL_TMPL2_CPP
#include "global_tmpl.cpp"
#endif

#endif
