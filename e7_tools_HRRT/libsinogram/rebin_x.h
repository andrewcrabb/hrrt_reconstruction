/*! \file rebin_x.h
    \brief This template implements a rebinning in x-direction by linear
           interpolation.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
    \date 2005/02/25 use vectors
 */

#pragma once

#include <vector>

/*- template definitions ----------------------------------------------------*/

template <typename T> class RebinX
 { public:
                                     /*! different methods of handling edges */
    typedef enum { EDGE_NONE,                        /*!< don't handle edges */
                   EDGE_ZERO,                         /*!< set edges to zero */
                   EDGE_TRUNCATE    /*!< cut edges and replace by neighbours */
                 } tedge_handling;
                                                          /*! arc correction */
    typedef enum { NO_ARC,                            /*!< no arc correction */
                   ARC2UNIFORM,/*!< convert arc sampling to uniform sampling */
                   UNIFORM2ARC /*!< convert uniform sampling to arc sampling */
                 } tarc_correction;
   private:
                                           /*! mask for linear interpolation */
    typedef struct {                           /*! first index of neighbours */
                     unsigned short int first_index;
                     std::vector <double> weight; /*!< weights of neighbours */
                   } tmask;
    unsigned long int new_slice_size,            /*!< size of rebinned slice */
                      old_slice_size;                     /*!< size of slice */
    unsigned short int width,                            /*!< width of slice */
                       new_width,               /*!< width of rebinned slice */
                       height,                          /*!< height of slice */
                       edge_i1,                /*!< coordinate for left edge */
                       edge_i2;               /*!< coordinate for right edge */
    tedge_handling edge_handling;              /*!< method for edge handling */
    std::vector <tmask> masks;             /*!< table of weights and indices */
   public:
                                   // initialize tables for rebinning algorithm
    RebinX(const unsigned short int, const unsigned short int,
           const unsigned short int, const tarc_correction,
           const unsigned short int, const T, const T, const T, const bool,
           const bool, const tedge_handling);
                                         // rebinning of dataset in x-direction
    void calcRebinX(T * const, T * const, const unsigned short int) const;
 };

