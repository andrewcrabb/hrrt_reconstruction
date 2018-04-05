/*! \file ssr.h
    \brief This class implements the single-slice rebinning.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/09/21 added Doxygen style comments
 */

#pragma once

#include <vector>

/*- class definitions -------------------------------------------------------*/

class SSR
 { private:
                                 /*! number of planes for axes of 3d sinogram */
    std::vector <unsigned short int> axis_slices,
                                     axpro;    /*!< factors for normalization */
    unsigned short int RhoSamples,         /*!< number of bins in projections */
                       ThetaSamples,        /*!< number of angles in sinogram */
                        /*! number of planes to skip at each end of a segment */
                       skip,
                       sino2d_idx;           /*!< index for memory controller */
    unsigned long int sino_plane_size;          /*!< size of a sinogram plane */
    float *sino2d;                                  /*!< rebinned 2d sinogram */
   public:
                                                           // initialize object
    SSR(const unsigned short int, const unsigned short int,
        const std::vector <unsigned short int>, const unsigned short int,
        const unsigned short int);
    unsigned short int Normalize();           // normalize rebinned 2d sinogram
                                                // rebin segment of 3d sinogram
    void Rebinning(float * const, const unsigned short int);
 };

