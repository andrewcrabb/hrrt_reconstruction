/*! \file fastmath.h
    \brief This file contains some definitions for 32 bit trigonometric
           functions.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/12/01 initial version
    \date 2004/05/04 added Doxygen style comments

    The Intel compiler is able create code that uses special 32 bit
    trigonometric functions, which are faster but less accurate than their
    64 bit counterparts. This file provides definitions for other compilers to
    emulate these function calls by 64 bit function calls.
 */

#ifndef _FAST_MATH_H
#define _FAST_MATH_H

#if defined(__linux__) && defined(__INTEL_COMPILER)
                            // Intel's math library contains fast trigonometric
                            // functions with 32 bit precision
#include <mathimf.h>
#else
#if !defined(__linux__) || !defined(__INTEL_COMPILER)
#include <cmath>
#endif

                                     // MacOS X and Solaris use GNU C++ instead
#if defined(__MACOSX__) || defined(__SOLARIS__) || defined(__linux__)
                        // define trigonometric functions with 32 bit precision
                        // based on 64 bit precision functions
#define acosf(a)    (float)acos((double)a)                   /*!< arc cosine */
#define asinf(a)    (float)asin((double)a)                     /*!< arc sine */
#define atanf(a)    (float)atan((double)a)                  /*!< arc tangent */
#define ceilf(a)    (float)ceil((double)a)                         /*!< ceil */
#define cosf(a)     (float)cos((double)a)                        /*!< cosine */
#define expf(a)     (float)exp((double)a)                   /*!< exponential */
#define fabsf(a)    (float)fabs((double)a)                     /*!< absolute */
#define logf(a)     (float)log((double)a)                     /*!< logarithm */
#define powf(a, b)  (float)pow((double)a, (double)b)              /*!< power */
#define sinf(a)     (float)sin((double)a)                          /*!< sine */
#define sqrtf(a)    (float)sqrt((double)a)                  /*!< square root */
#define tanf(a)     (float)tan((float)a)                        /*!< tangent */
#endif
#endif

#endif
