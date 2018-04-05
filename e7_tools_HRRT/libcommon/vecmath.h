/*! \file vecmath.h
    \brief This module provides functions for vector operations.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/04/01 initial version
    \date 2004/04/29 added vecAddScalar function
    \date 2004/09/25 added functions for complex vectors
 */

#pragma once

/*- exported functions ------------------------------------------------------*/

                                                         /*! add two vectors */
inline void vecAdd(const float * const, const float * const, float * const,
                   const unsigned long int);
                                                 /*! add two complex vectors */
inline void vecAddComplex(const float * const, const float * const,
                          const float * const, const float * const,
                          float * const, float * const,
                          const unsigned long int);
                                             /*! add complex and real vector */
inline void vecAddComplexReal(const float * const, const float * const,
                              const float * const, float * const,
                              float * const, const unsigned long int);
                                 /*! add two vectors and multiply with third */
inline void vecAddMult(const float * const, const float * const,
                       const float * const, float * const,
                       const unsigned long int);
                                                    /*! add scalar to vector */
inline void vecAddScalar(const float * const, const float, float * const,
                         const unsigned long int);
                                             /* convolute signal with filter */
inline void vecConvolution(const float * const, const unsigned long int,
                           const unsigned long int, const float * const,
                           const unsigned long int, float * const);
               /*! copy interleaved complex vector into spllt complex vector */
inline void vecCopyIntlCplxSplCplx(const float * const, float * const,
                                   float * const, const unsigned long int);
               /*! copy split complex vector into interleaved complex vector */
inline void vecCopySplCplxIntlCplx(const float * const, const float * const,
                                   float * const, const unsigned long int);
                              /*! calculate absolute value of complex vector */
inline void vecCplxAbs(const float * const, const float * const,
                       float * const, const unsigned long int);
                    /*! calculate complex vector conjugate, multiply and add */
inline void vecCplxConjugateMultiplyAdd(const float * const,
                                        const float * const,
                                        const float * const,
                                        const float * const,
                                        const float * const,
                                        const float * const, float * const,
                                        float * const,
                                        const unsigned long int);
                                                      /*! divide two vectors */
inline void vecDiv(const float * const, const float * const, float * const,
                   const unsigned long int);
                                                   /*! calculate dot product */
inline float vecDotProd(const float * const, const float * const,
                        const unsigned long int);
                                           /*! calculate complex dot product */
inline void vecDotProdComplex(const float * const, const float * const,
                              const float * const, const float * const,
                              float * const, float * const,
                              const unsigned long int, const bool);
                  /*! calculate dot product between complex and real vector  */
inline void vecDotProdComplexReal(const float * const, const float * const,
                                  const float * const, float * const,
                                  float * const, const unsigned long int);
                                                    /*! multiply two vectors */
inline void vecMul(const float * const, const float * const, float * const,
                   const unsigned long int);
                           /*! multiply two vector with scalars and add them */
inline void vecMulAdd(const float * const, const float, const float * const,
                      const float, float * const, const unsigned long int);
                                            /*! multiply two complex vectors */
inline void vecMulComplex(const float * const, const float * const,
                          const float * const, const float * const,
                          float * const, float * const,
                          const unsigned long int, const bool);
                                        /*! multiply complex and real vector */
inline void vecMulComplexReal(const float * const, const float * const,
                              const float * const, float * const,
                              float * const, const unsigned long int);
                                             /*! multiply vector with scalar */
inline void vecMulScalar(const float * const, const float, float * const,
                         const unsigned long int);
                           /*! multiply vector with scalar and add to result */
inline void vecMulScalarAdd(const float * const, const float, float * const,
                            const unsigned long int);
                                           /*! add vector elements to scalar */
inline double vecScalarAdd(const float * const, const unsigned long int);
                                 /*! calculate scaled sum of vector elements */
inline float vecScaledSum(const float * const, const float,
                          const unsigned long int);
                              /*! calculate signed square of vector elements */
inline void vecSignedSquare(const float * const, float * const,
                            const unsigned long int);
                                     /*! calculate square of vector elements */
inline void vecSquare(const float * const, float * const,
                      const unsigned long int);
                                                        /*! subtract vectors */
inline void vecSub(const float * const, const float * const, float * const,
                   const unsigned long int);
                                            /*! subtract two complex vectors */
inline void vecSubComplex(const float * const, const float * const,
                          const float * const, const float * const,
                          float * const, float * const,
                          const unsigned long int);
                                        /*! subtract complex and real vector */
inline void vecSubComplexReal(const float * const, const float * const,
                              const float * const, float * const,
                              float * const, const unsigned long int);
