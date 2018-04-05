/*! \file scatter.h
    \brief This class implements the calculation of a scatter sinogram based on
           an emission sinogram, normalization dataset and u-map.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
    \date 2005/03/29 delete OSEM object after use to reduce swapping
 */

#pragma once

#include <string>
#include <vector>
#include "image_conversion.h"
#include "matrix.h"
#include "sinogram_conversion.h"

/*- class definitions -------------------------------------------------------*/

class Scatter
 { private:
                                        /*! threshold for gap filling (HRRT) */
    static const float gap_fill_threshold;
             /*! number of detector sample points around ring; multiple of 4 */
    static const unsigned short int num_angular_samps;
                            /*! number of subsets for AW-OSEM reconstruction */
    unsigned short int subsets,
                      /*! width/height of images used for scatter simulation */
                       XYSamplesScatter,
                    /*! number of bins in projections for scatter simulation */
                       RhoSamplesScatterEstim,
                    /*! number of angles in sinograms for scatter simulation */
                       ThetaSamplesScatterEstim,
                       loglevel;                  /*!< top level for logging */
    std::string scatter_qc_path;         /*!< path for quality control files */
                 /*! number of first and last planes to skip in each segment */
    unsigned short int skip_outer_planes,
                       RhoSamples,         /*!< number of bins in projection */
                       ThetaSamples, /*!< number of angles in sinogram plane */
                       rebin_r;                 /*!< radial rebinning factor */
    bool scatter_qc;        /*!< store data for quality control of scatter ? */
    float BinSizeRho,                        /*!< bin size of sinogram in mm */
          DeltaZ;                                /*!< plane separation in mm */
    std::vector <unsigned short int> axis_slices;            /*!< axis table */
                                                   // calculate cubic b-splines
    Matrix <float> *bspline(const unsigned short int, const unsigned short int,
                            const unsigned short int, const unsigned short int,
                            const unsigned short int) const;
                                   // create GNUPlot script for quality control
    void GNUPlotScript(const unsigned short int, const unsigned short int,
                       const std::vector <unsigned short int>,
                       const unsigned short int) const;
                         // use linear interpolation to increase with of matrix
    Matrix <float> *interpol(const unsigned short int, const float * const,
                             const unsigned short int, float * const,
                             const unsigned short int,
                             const Matrix <float> * const) const;
                                                   // simulate scatter sinogram
    unsigned short int ScatterEstimation(float * const, float * const,
                                         const float, const float,
                                         const unsigned short int, const float,
                                         const unsigned short int,
                                         const unsigned short int, const float,
                                         const float, const float, const float,
                                         const unsigned short int,
                                         const unsigned short int, const float,
                                         const float, const bool,
                                         const unsigned short int) const;
                      // calculate scale factors for planes of scatter sinogram
    std::vector <float> ScatterScaleFactors(SinogramConversion *,
                                            SinogramConversion *,
                                            SinogramConversion *, const float,
                                            const float,
                                            const unsigned short int,
                                            const unsigned short int,
                                            unsigned short int, const float,
                                            const float,
                                            const unsigned short int) const;
                               // calculate single-slice rebinned sinogram with
                               // attenuation correction and normalization
    SinogramConversion *SingleSliceRebinnedSinogram(SinogramConversion *,
                                                    SinogramConversion *,
                                                    const unsigned short int,
                                                    const unsigned short int);
   public:
                                                           // initialize object
    Scatter(const unsigned short int, const float, const unsigned short int,
            const std::vector <unsigned short int>, const unsigned short int,
            const float, const unsigned short int, const unsigned short int,
            const unsigned short int, const bool, const std::string,
            const unsigned short int);
    ~Scatter();                                               // destroy object
                                                  // perform scatter estimation
    SinogramConversion *estimateScatter(SinogramConversion *,
                                        ImageConversion *,
                                        SinogramConversion *,
                                        const unsigned short int, const float,
                                        const float, const float, const float,
                                        const unsigned short int,
                                        const unsigned short int, const float,
                                        const float,
                                        const unsigned short int,
                                        const float, 
                                        const float,
                                        const bool, const unsigned short int,
                                        const unsigned short int,
                                        unsigned short int, const float,
                                        const float, const bool,
#ifndef USE_OLD_SCATTER_CODE
                                        std::vector<float> &,
#endif
                                        const std::string,
                                        const unsigned short int);
 };

