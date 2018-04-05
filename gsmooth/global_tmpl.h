//** file: global_tmpl.h
//** date: 2002/11/04

//** author: Frank Kehren
//** © CPS Innovations

//** This module contains simple function templates that are used at different
//** places in the program.

#pragma once

/*- exported functions ------------------------------------------------------*/

                                          // convert emission into transmission
template <typename T> void Emi2Tra(T * const, const unsigned long int);
template <typename T> signed long int round(const T a);          // round value
                                          // convert transmission into emission
template <typename T> void Tra2Emi(T * const, const unsigned long int);
