/*! \file ecat7_utils.h
    \brief This module implements functions that operate on ECAT7 datasets.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/17 added Doxygen style comments
 */

#pragma once

/*- exported functions ------------------------------------------------------*/

                                     // cut bins on both sides of the sinograms
void utils_CutBins(void **, const unsigned short int,
                   const unsigned short int, const unsigned short int,
                   const unsigned short int, const unsigned short int);
                                                  // calculate decay correction
float utils_DecayCorrection(void * const, const unsigned short int,
                            const unsigned long int, const float, const bool,
                            const unsigned long int, const unsigned long int);
                                                      // deinterleave sinograms
void utils_DeInterleave(void * const, const unsigned short int,
                        const unsigned short int, const unsigned short int,
                        const unsigned short int);
                           // mirror dataset to exchange feet and head position
void utils_Feet2Head(void * const , const unsigned short int,
                     const unsigned short int, const unsigned short int,
                     const unsigned short int * const);
                                                        // interleave sinograms
void utils_Interleave(void * const, const unsigned short int,
                      const unsigned short int, const unsigned short int,
                      const unsigned short int);
                                     // change orientation from prone to supine
void utils_Prone2Supine(void * const, const unsigned short int,
                        const unsigned short int, const unsigned short int,
                        const unsigned short int);
                                       // rotate dataset along the scanner axis
void utils_Rotate(void * const, const unsigned short int,
                  const unsigned short int, const unsigned short int,
                  const unsigned short int, const signed short int);
                                                               // scale dataset
void utils_ScaleMatrix(void * const, const unsigned short int,
                       const unsigned long int, const float);
                                          // convert from view to volume format
void utils_View2Volume(void * const, const unsigned short int,
                       const unsigned short int, const unsigned short int,
                       const unsigned short int * const);
                                          // convert from volume to view format
void utils_Volume2View(void * const, const unsigned short int,
                       const unsigned short int, const unsigned short int,
                       const unsigned short int * const);
