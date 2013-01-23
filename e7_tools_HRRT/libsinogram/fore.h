/*! \file fore.h
    \brief This class implements the fourier rebinning to convert 3d sinograms
           into a stack of 2d sinograms.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2003/12/09 fixed bug in calculation of current ring difference for
                     oblique segments
    \date 2003/12/30 fixed bug in logging of axial angle
    \date 2005/01/27 added Doxygen style comments
    \date 2005/02/25 use vectors and memory controller
 */

#ifndef _FORE_H
#define _FORE_H

#include <vector>
#include "fore_fft.h"

/*- class definitions -------------------------------------------------------*/

class FORE
 { private:
    unsigned short int W_Limit,      /*!< FORE limit in the radial direction */
                       K_Limit,     /*!< FORE limit in the angular direction */
                       A_Limit,              /*!< SSRB limit in oblique axes */
                       RhoSamples,           /*!< number of bins in sinogram */
                       ThetaSamples,       /*!< number of angles in sinogram */
                       span,                           /*!< span of sinogram */
                       RhoSamples_d2p1,/*!< number of bins in FT of sinogram */
                       loglevel,                  /*!< top level for logging */
                                /*! index of buffer for FT of sinogram plane */
                       sino3d_buffer_idx,
                                   /*! index of buffer for FT of 3d sinogram */
                       sino_fft_idx,
                                   /*! index of buffer for rebinned sinogram */
                       fore_vol_idx,
                                /*! index of buffer for normalization matrix */
                       fore_norm_idx;
                              /*! array containing number of planes per axis */
    std::vector <unsigned short int> axis_slices,
                           /*! first projection to rebin for each view angle */
                                     First_Proj;
    unsigned long int fft_plane_size,   /*!< size of plane in FT of sinogram */
                      sino_plane_size;        /*!< size of plane in sinogram */
    float plane_separation,                      /*!< plane separation in mm */
          ring_radius,                                /*!< ring radius in mm */
          delta_phi,                     /*!< angular step of axes (radians) */
          x_res_z_res,                  /*!< bin size to plane spacing ratio */
          *fore_vol,                                  /*!< rebinned sinogram */
          *fore_norm,                              /*!< normalization matrix */
          *sino3d_buffer,                          /*!< FT of sinogram plane */
          *sino_fft;                                  /*!< FT of 3d sinogram */
    std::vector <float> frac0, frac1;  /*!< buffers for linear interpolation */
    std::vector <signed short int> del_Z;          /*!< buffer for z offsets */
    FORE_FFT *fore_fft;         /*!< object for calculatin FFTs of sinograms */
                         // calculate constants for rebinning of specified axis
    void gen_rebin_map(const unsigned short int);
                     // calculate constants for rebinning independent from axis
    void gen_rebin_map();
                                  // calculate reversed ordered conjugate of FT
    void obl_sym_fix(float *) const;
                                                        // rebin "direct" plane
    void rebin_2Dplane(const unsigned short int, const float *, float *) const;
                                           // normalize FT of rebinned sinogram
    void rebin_norm2D(float * const, const float * const) const;
                                                       // rebin "oblique" plane
    void rebin_plane(const unsigned short int, const unsigned short int,
                     float *, float *, float *) const;
                                           // scale sinogram axis for phi-angle
    void scaling(float * const, const unsigned short int) const;
   public:
                                                        // initialize algorithm
    FORE(const unsigned short int, const float, const unsigned short int,
         const std::vector <unsigned short int>,
         const unsigned short int, const float, const float,
         const unsigned short int, const unsigned short int,
         const unsigned short int, const unsigned short int);
    ~FORE();                                                  // destroy object
                                        // normalize and iFFT rebinned sinogram
    void Normalize(const std::string, unsigned short int * const,
                   const unsigned short int) const;
                                                                // perform FORE
    void Rebinning(float *, const unsigned short int,
                   const unsigned short int);
 };

#endif
