/*! \file rebin_xy.h
    \brief This class implements the value-preserving rebinning of 2d images.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/12/20 support image zoom
 */

#pragma once

/*- class definitions -------------------------------------------------------*/

class RebinXY
 { private:
    float factor_x,               /*!< ratio between new and old voxel width */
          factor_y,              /*!< ratio between new and old voxel height */
          scale_factor;         /*!< scale factor for voxels after rebinning */
    unsigned short int old_width,                    /*!< old width of image */
                       old_height,                  /*!< old height of image */
                       new_width,                    /*!< new width of image */
                       new_height;                  /*!< new height of image */
    signed short int offsx,   /* horizontal offset of new image in old image */
                     offsy;     /* vertical offset of new image in old image */
   public:
                                                           // initialize object
    RebinXY(const unsigned short int, const float, const unsigned short int,
            const float, const unsigned short int, const float,
            const unsigned short int, const float, const bool);
                               // rebin image and allocate memory for new image
    float *calcRebinXY(float * const, const unsigned short int) const;
                             // rebin image in memory that was allocated before
    float *calcRebinXY(float * const, float * const,
                       const unsigned short int) const;
 };

