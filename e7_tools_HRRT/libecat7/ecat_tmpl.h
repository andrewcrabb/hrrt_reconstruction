/*! \file ecat_tmpl.h
    \brief This module implements templates that operate on ECAT7 datasets.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/10 added Doxygen style comments
 */

#ifndef _ECAT_TMPL_H
#define _ECAT_TMPL_H

#include <fstream>

/*- exported functions ------------------------------------------------------*/

                                     // cut bins on both sides of the sinograms
template <typename T> void cutbins(T **, const unsigned short int,
                                   const unsigned short int,
                                   const unsigned short int,
                                   const unsigned short int);
                                                      // deinterleave sinograms
template <typename T> void deinterleave(T * const, const unsigned short int,
                                        const unsigned short int,
                                        const unsigned short int);
                           // mirror dataset to exchange feet and head position
template <typename T> void feet2head(T * const, const unsigned short int,
                                     const unsigned short int,
                                     const unsigned short int);
                                                        // interleave sinograms
template <typename T> void interleave(T * const, const unsigned short int,
                                      const unsigned short int,
                                      const unsigned short int);
                                                  // load data part of a matrix
template <typename T> T *loaddata(std::ifstream * const,
                                  const unsigned long int,
                                  const unsigned long int,
                                  const unsigned short int);
                                     // change orientation from prone to supine
template <typename T> void prone2supine(T * const, const unsigned short int,
                                        const unsigned short int,
                                        const unsigned short int);
                                       // rotate dataset along the scanner axis
template <typename T> void rotate(T * const, const unsigned short int,
                                  const unsigned short int,
                                  const unsigned short int,
                                  const signed short int);
                                                 // store data part of a matrix
template <typename T> void savedata(std::ofstream * const, T * const,
                                    const unsigned long int,
                                    const unsigned short int);
                                                               // scale dataset
template <typename T> void scaledata(T * const, const unsigned long int,
                                     const float);
                                          // convert from view to volume format
template <typename T> void view2volume(T * const, const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int);
                                          // convert from volume to view format
template <typename T> void volume2view(T * const, const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int);

#ifndef _ECAT_TMPL_CPP
#include "ecat_tmpl.cpp"
#endif

#endif
