/*! \file sinogram_conversion.h
    \brief This class provides methods to convert a sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/03/19 initial version
    \date 2004/03/29 added model based gap filling
    \date 2004/03/30 added axial arc-correction
    \date 2004/04/01 added Doxygen style comments
    \date 2004/04/22 reduce memory requirements in divide, multiply, subtract
                     subtract2D and fillGaps
    \date 2004/04/22 increase performance of FORE and SSRB for TOF data
    \date 2004/04/29 added method to add Poisson noise
    \date 2004/05/06 don't allow radial arc correction on corrected data
    \date 2004/06/08 delete segment table of randoms dataset after subtraction
    \date 2004/07/15 added axial un-arc correction
    \date 2004/09/09 calculate correct axis size after trimming
    \date 2004/09/10 ignore norm border in gap filling
    \date 2004/09/16 added "multiplyRandoms" method
    \date 2004/09/21 added randoms smoothing for TOF data
    \date 2004/09/21 fixed model based gap filling if sinogram is not full size
    \date 2004/09/21 fixed addTOFbins for prompts and randoms data
    \date 2004/09/22 allow to switch off radial and axial arc corrections
    \date 2004/11/11 divide by zero in divide() method changed
    \date 2005/01/04 added progress reporting to sino3D2D()
 */

#ifndef _SINOGRAM_CONVERSION_H
#define _SINOGRAM_CONVERSION_H

#include <vector>
#include "sinogram_io.h"

class SinogramConversion:public SinogramIO
 { private:
                           /*! threshold for normalization based gap-filling */
    static const float gap_fill_threshold;
                                                               // mask-out LORs
    void maskOutLORs(float *, const std::vector <bool> mask,
                     const std::vector <unsigned short int>) const;
                                               // rotate sinogram around z-axis
    template <typename T>
     void rotate(T * const, const unsigned short int, const unsigned short int,
                 const unsigned short int, const signed short int) const;
   public:
             /*! rebinning methods to convert 3d sinograms into 2d sinograms */
    typedef enum { NO_REB,                                 /*!< no rebinning */
                   FORE_REB,                          /*!< fourier rebinning */
                   SSRB_REB,                     /*!< single-slice rebinning */
                   SEG0_REB                          /*!< use only segment 0 */
                 } trebin_method;
   /*! rebinning methods to convert 2d sinograms or images into 3d sinograms */
    typedef enum { iNO_REB,                                /*!< no rebinning */
                   iFORE_REB,     /*!< inverse fourier rebinning of sinogram */
                   iSSRB_REB,/*!< inverse single-slice rebinning of sinogram */
                   FWDPJ                 /*!< 3d forward-projection of image */
                 } tirebin_method;
                                                                // constructors
    SinogramConversion(const SinogramIO::tscaninfo, const bool);
    SinogramConversion(const unsigned short int, const float,
                       const unsigned short int, const unsigned short int,
                       const unsigned short int, const float,
                       const unsigned short int, const float, const bool,
                       const unsigned short int, const unsigned short int,
                       const float, const float, const float, const float,
                       const float, const unsigned short int, const bool);
                                               // add Poisson noise to sinogram
    void addPoissonNoise(const double, const double, const unsigned short int);
    void addTOFbins(const unsigned short int);    // add-up time-of-flight bins
                                                        // axial arc-correction
    void axialArcCorrection(const bool, const bool, const unsigned short int,
                            const unsigned short int);
                                                 // smooth and subtract randoms
    void calculateTrues(const bool, const unsigned short int,
                        const unsigned short int);
                                           // apply lower threshold to sinogram
    void cutBelow(const float, const float, const unsigned short int);
                                          // decay- and frame-length correction
    void decayCorrect(bool, bool, const unsigned short int);
                                                            // divide sinograms
    void divide(SinogramConversion * const, const bool,
                const unsigned short int, const bool, const unsigned short int,
                const unsigned short int);
                                          // convert emission into transmission
    void emi2Tra(const unsigned short int);
                                              // fill gaps base on gantry model
    void fillGaps(const unsigned short int, const unsigned short int);
                                            // fill gaps based on normalization
    void fillGaps(SinogramConversion *, const unsigned short int, const bool,
                  const unsigned short int, const unsigned short int);
                                                            // mask border bins
    void maskBorders(const unsigned short int, const unsigned short int);
                                                    // mask-out detector blocks
    void maskOutBlocks(const std::vector <unsigned short int>,
                       const unsigned short int);
                                                          // multiply sinograms
    void multiply(SinogramConversion * const, const bool,
                  const unsigned short int, const unsigned short int,
                  const unsigned short int);
                                     // multiply randoms sinogram with sinogram
    void multiplyRandoms(SinogramConversion * const, const float,
                         const unsigned short int);
                                             // print statistics about sinogram
    void printStat(const unsigned short int) const;
                                                       // radial arc-correction
    void radialArcCorrection(const bool, const bool, const unsigned short int,
                             const unsigned short int);
                            // rebin sinogram in radial and azimuthal direction
    void resampleRT(const unsigned short int, const unsigned short int,
                    const unsigned short int, const unsigned short int);
                                            // rebin 2d sinogram in z-direction
    void resampleZ(const unsigned short int, const unsigned short int,
                   const std::string, const unsigned short int);
                                                  // resort time-of-flight data
    void shuffleTOFdata(const bool, const unsigned short int);
                                        // convert 2d sinogram into 3d sinogram
    void sino2D3D(SinogramConversion::tirebin_method, const unsigned short int,
                  const unsigned short int, const std::string,
                  const unsigned short int, const unsigned short int);
                                        // convert 3d sinogram into 2d sinogram
    void sino3D2D(SinogramConversion::trebin_method, const unsigned short int,
                  const unsigned short int, const unsigned short int,
                  const std::string, const unsigned short int,
                  const unsigned short int, const unsigned short int);
                                                     // smooth randoms sinogram
    void smoothRandoms(const unsigned short int, const unsigned short int);
                                                           // subtract sinogram
    void subtract(SinogramConversion * const, const bool,
                  const unsigned short int, const unsigned short int);
                                                        // subtract 2d sinogram
    void subtract2D(SinogramConversion * const, const bool,
                    const unsigned short int, double * const, double * const,
                    const unsigned short int);
                                     // convert transmission into emission data
    void tra2Emi(const unsigned short int);
                                                               // trim sinogram
    void trim(const unsigned short int, const std::string,
              const unsigned short int);
                                                     // tilt or untilt sinogram
    void untilt(const bool, const unsigned short int);
 };

#endif
