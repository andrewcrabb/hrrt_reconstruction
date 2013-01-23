/*! \file convert.h
    \brief This module provides functions to convert sinograms into images and
           vice versa.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/04/01 initial version
    \date 2004/05/06 no arc-correction for acf from PET/CT gantry
    \date 2004/05/28 call IDL scatter code
    \date 2004/10/01 added Doxygen style comments
    \date 2004/12/17 added support for 2395
    \date 2005/01/04 added progress reporting to get_umap_acf() and
                     eau2scatter()
    \date 2005/03/08 fixed loading of u-map if GM is not initialized
    \date 2005/03/11 added PSF-AW-OSEM
 */

#ifndef _CONVERT_H
#define _CONVERT_H

#include <string>
#include "image_conversion.h"
#include "parser.h"
#include "sinogram_conversion.h"
#include "types.h"

/*- exported functions ------------------------------------------------------*/

               /*! functions to convert images into sinograms and vice versa */
namespace CONVERT
{
                                 // calculate u-map from blank and transmission
ImageConversion *bt2umap(SinogramConversion *, SinogramConversion *,
                         const unsigned short int, const unsigned short int,
                         const unsigned short int, const unsigned short int,
                         const unsigned short int, const unsigned short int,
                         const float, const float, const float,
                         const unsigned short int, const bool, const float,
                         const unsigned short int, float * const,
                         float * const, float * const, const float, const bool,
                         const float, const float, const float,
                         const unsigned short int, const unsigned short int,
                         const bool, const bool, const std::string,
                         const unsigned short int, const std::string,
                         const unsigned short int, float * const,
                         const unsigned short int, const unsigned short int);
                   // calculate scatter from normalized emission, acf and u-map
SinogramConversion *eau2scatter(SinogramConversion *, SinogramConversion *,
                                ImageConversion *, const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int, const float,
                                const float, const float,
                                const unsigned short int,
                                const unsigned short int, const bool,
                                const std::string, const bool,
                                const std::string, const unsigned short int,
                                bool,
#ifndef USE_OLD_SCATTER_CODE
                                std::vector<float> &,
#endif
                                const unsigned short int,
                                const unsigned short int);
                                                 // get u-map and acf from file
void get_umap_acf(Parser::tparam * const, ImageConversion **,
                  SinogramConversion **, const bool, const bool,
                  unsigned short int, unsigned short int,
                  const unsigned short int, const unsigned short int,
                  const unsigned short int, const unsigned short int,
                  const unsigned short int);
                                                 // convert image into sinogram
SinogramConversion *image2sinogram(ImageConversion *, Parser::tparam * const,
                                   const unsigned short int, const std::string,
                                   const unsigned short int,
                                   const unsigned short int);
                                                 // convert sinogram into image
ImageConversion *sinogram2image(SinogramConversion *, SinogramConversion **,
                                SinogramConversion **, SinogramConversion **,
                                const unsigned short int, const float,
                                const ALGO::talgorithm,
                                const unsigned short int,
                                const unsigned short int, const float,
                                const bool, const unsigned short int,
                                const unsigned short int, float * const,
                                std::string, const unsigned short int,
                                const unsigned short int);
}

#endif
