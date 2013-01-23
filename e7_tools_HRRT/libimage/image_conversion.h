/*! \file image_conversion.h
    \brief This class provides methods to convert an image.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/03/23 initial version
    \date 2004/09/10 added trim() method
    \date 2004/09/10 fix memory leaks in resampleXY() and resampleZ()
    \date 2004/11/12 added CT2UMAP() method
    \date 2005/01/04 added progress reporting to CT2UMAP()
 */

#ifndef _IMAGE_CONVERSION_H
#define _IMAGE_CONVERSION_H

#include <vector>
#include "image_io.h"

/*- class definitions -------------------------------------------------------*/

class ImageConversion:public ImageIO
 { public:
                                                       // initialize the object
    ImageConversion(const unsigned short int, const float,
                    const unsigned short int, const float,
                    const unsigned short int, const unsigned short int,
                    const unsigned short int, const float, const float,
                    const float, const float, const float);
                                             // convert CT image into PET u-map
    void CT2UMAP(const std::vector <float>, const float, const float,
                 const float, const float, const float,
                 const unsigned short int, const unsigned short int);
                                              // cut-off values above threshold
    void cutAbove(const float, const float, const unsigned short int) const;
                                              // cut-off values below threshold
    void cutBelow(const float, const float, const unsigned short int) const;
                                           // decay and frame-length correction
    float decayCorrect(const bool, const bool, const unsigned short int);
                                  // gaussian smoothing in x/y- and z-direction
    void gaussFilter(const float, const float, const unsigned short int,
                     const unsigned short int);
                                                // apply circular mask to image
    void maskXY(float * const, const unsigned short int) const;
                                                // print statistics about image
    void printStat(const unsigned short int) const;
                                             // resample image in x/y direction
    void resampleXY(const unsigned short int, const float, const bool,
                    const unsigned short int);
                                               // resample image in z-direction
    void resampleZ(const unsigned short int, const unsigned short int,
                   const unsigned short int);
                                                                  // trim image
    void trim(const unsigned short int, const std::string,
              const unsigned short int);
 };

#endif
