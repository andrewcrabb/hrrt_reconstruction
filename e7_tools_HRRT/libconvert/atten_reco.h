/*! \file atten_reco.h
    \brief This class implements the calculation of a u-map from blank and
           transmission.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Vladimir Panin (vladimir.panin@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added deadtime correction
    \date 2004/10/01 added Doxygen style comments
    \date 2004/10/01 use memory controller object
 */

#pragma once

#include <string>
#include <vector>
#include "types.h"

/*- class definitions -------------------------------------------------------*/

class AttenReco
 { private:
                               /*! number of projections in a sinogram plane */
    unsigned short int ThetaSamples,
                       RhoSamples,           /*!< number of bins in sinogram */
                       XYSamples,                 /*!< width/height of u-map */
                       planes,             /*!< number of planes in sinogram */
                              /*! number of projections in rebinned sinogram */
                       ThetaSamplesUMap,
                                     /*! number of bins in rebinned sinogram */
                       RhoSamplesUMap,
                       planesUMap,/*!< number of planes in rebinned sinogram */
                         /*! radial and azimuthal  zoom factor for rebinning */
                       sino_rt_zoom,
                       sino_z_zoom,           /*!< zoom factor for rebinning */
                       loglevel,                      /*!< level for logging */
                    /*! radial rebinning factor that was applied to sinogram */
                       rebin_r,
                 /*! azimuthal rebinning factor that was applied to sinogram */
                       rebin_t;
                                         /*! plane size in rebinned sinogram */
    unsigned long int sinoUMap_plane_size,
                      sinoUMap_size,          /*!< size of rebinned sinogram */
                      umap_plane_size,              /*!< plane size in u-map */
                      umap_size;                          /*!< size of u-map */
    float BinSizeRho;                              /*!< bin size in sinogram */
                                       // calculate deadtime correction factors
    std::vector <float> dead_time_correction(const uint64, const std::string,
                                             const unsigned short int,
                                             const unsigned short int) const;
                                           // calculate log(blank/transmission)
    float *calcRatio(const float * const, const float * const,
                     const float, unsigned short int * const) const;
                                   // create GNUPlot script for quality control
    void GNUPlotScript(const std::string, const unsigned short int) const;
                                       // normalize blank and transmission scan
    void normalizeBlankTx(float * const, float * const) const;
                                                              // rebin sinogram
    float *rebin_sinogram(const float * const,
                          unsigned short int * const) const;
                                            // rebin sinogram and add up angles
    std::vector <float> rebin_sinogram_add_angles(const float *) const;
                              // calculate sum of counts measured under the bed
    float sumCountsUnderBed(float * const) const;
   public:
                                                           // initialize object
    AttenReco(const unsigned short int, const unsigned short int,
              const float, const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int,
              const unsigned short int);
                                                             // calculate u-map
    void reconstruct(float * const, float * const, const unsigned short int,
                     const unsigned short int, const float, const float,
                     const float, const unsigned short int, const bool,
                     const float, const unsigned short int, float * const,
                     float * const, float * const, const float, const bool,
                     const bool, const std::string, const bool, const float,
                     const unsigned short int, const uint64, const std::string,
                     const unsigned short int, unsigned short int * const,
                     const unsigned short int) const;
 };

