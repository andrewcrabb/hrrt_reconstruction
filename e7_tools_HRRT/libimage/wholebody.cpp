/*! \class Wholebody wholebody.h "wholebody.h"
    \brief This class assembles images to wholebody images.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/10/28 initial version
    \date 2004/08/02 beds shipped with PET only system have different
                     coordinate system than PET/CT beds

    This class assembles images to wholebody images. The orientation of the
    assembled image is always head first.
 */

#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <string.h>
#ifndef _WHOLEBODY_CPP
#define _WHOLEBODY_CPP
#include "wholebody.h"
#endif
#include "ecat_tmpl.h"
#include "gm.h"
#include "logging.h"
#include "vecmath.h"

/*- methods -----------------------------------------------------------------*/

#ifndef _WHOLEBODY_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Initialize wholebody image.
    \param[in] _data              pointer to image data
    \param[in] _width             width of image
    \param[in] _height            height of image
    \param[in] _depth             depth of image
    \param[in] _slice_thickness   thickness of slices in mm
    \param[in] _bedpos            horizontal bed position of image in mm

    Initialize wholebody image. The data is copied into the object and is
    afterwards still owned by the caller.
 */
/*---------------------------------------------------------------------------*/
Wholebody::Wholebody(float * const _data, const unsigned short int _width,
                     const unsigned short int _height,
                     const unsigned short int _depth,
                     const float _slice_thickness, const float _bedpos)
 { data=NULL;
   try
   { width=_width;
     height=_height;
     depth=_depth;
     bedpos=_bedpos;
     slice_thickness=_slice_thickness;
     slice_size=(unsigned long int)width*(unsigned long int)height;
                                                       // copy data into object
     data=new float[slice_size*(unsigned long int)depth];
     memcpy(data, _data, slice_size*(unsigned long int)depth*sizeof(float));
   }
   catch (...)
    { if (data != NULL) delete[] data;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy the instance of this object.

    Release the used memory and destroy the instance of this class.
 */
/*---------------------------------------------------------------------------*/
Wholebody::~Wholebody()
 { if (data != NULL) delete[] data;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Append image to stored image.
    \param[in] _data          pointer to image data
    \param[in] _depth         depth of image
    \param[in] _bedpos        horizontal position of image in mm
    \param[in] feet_first     image has feet first orientation
    \param[in] bed_moves_in   did bed move in during scanning ?
    \param[in] loglevel       level for logging

    Append image to stored image. The wholebody image is always in head first
    orientation.
 */
/*---------------------------------------------------------------------------*/
void Wholebody::addBed(float * const _data, const unsigned short int _depth,
                       float _bedpos, const bool feet_first,
                       const bool bed_moves_in,
                       const unsigned short int loglevel)
 { float *new_data=NULL;

   try
   { unsigned short int new_depth;
     signed short int overlap;
     float *dp, *dps, a1, a2, a3, a4, b1, b2;

     if (feet_first)                 // transform from feet first to head first
      { feet2head(_data, width, height, _depth);
        // if (!bed_moves_in) _bedpos-=(float)_depth*slice_thickness;
      }
                                      // calculate amount of overlapping planes
     a1=bedpos;
     if (!feet_first) a2=a1-depth*slice_thickness;
      else { a1+=_depth*slice_thickness;
             a2=a1-depth*slice_thickness;
           }

     a3=_bedpos;
     a4=a3-_depth*slice_thickness;
     //   std::cerr << a1 << " " << a2 << " " << a3 << " " << a4 << std::endl;
     if (((a3 > a2) && (a3 > a1)) || ((a3 < a2) && (a3 < a1))) b1=a4;
      else b1=a3;
     if (((a1 > a3) && (a1 > a4)) || ((a1 < a3) && (a1 < a4))) b2=a2;
      else b2=a1;
     //   std::cerr << b1 << " " << b2 << std::endl;
     if (bed_moves_in)
      overlap=(signed short int)((b1-b2)/slice_thickness+0.5f);
      else overlap=(unsigned short int)((b2-b1)/slice_thickness+0.5f);
     Logging::flog()->logMsg("overlapping planes=#1", loglevel)->arg(overlap);
     new_depth=depth+_depth-overlap;
     new_data=new float[slice_size*(unsigned long int)new_depth];
     memset(new_data, 0,
            slice_size*(unsigned long int)new_depth*sizeof(float));
                                        // copy new image before existing image
     if (feet_first == bed_moves_in)
      { memcpy(new_data, _data,
               (unsigned long int)_depth*slice_size*sizeof(float));
        if (overlap > 0)
         memcpy(new_data+(unsigned long int)_depth*slice_size,
                data+(unsigned long int)overlap*slice_size,
                (unsigned long int)(depth-overlap)*slice_size*sizeof(float));
         else memcpy(new_data+(unsigned long int)(_depth-overlap)*slice_size,
                     data,
                     (unsigned long int)depth*slice_size*sizeof(float));
        dp=new_data+(unsigned long int)(_depth-overlap)*slice_size;
        dps=data;
        bedpos=_bedpos;
      }
      else {                             // copy new image after existing image
             memcpy(new_data, data,
                    slice_size*(unsigned long int)depth*sizeof(float));
             if (overlap > 0)
              memcpy(new_data+(unsigned long int)depth*slice_size,
                     _data+(unsigned long int)overlap*slice_size,
                     (unsigned long int)(_depth-overlap)*slice_size*
                     sizeof(float));
              else memcpy(new_data+
                          (unsigned long int)(depth-overlap)*slice_size,
                          _data,
                          (unsigned long int)_depth*slice_size*sizeof(float));
             dp=new_data+(unsigned long int)(depth-overlap)*slice_size;
             dps=_data;
           }
                                   // calculate overlap area of assembled image
     for (unsigned short int z=0; z < overlap; z++,
          dp+=slice_size,
          dps+=slice_size)
      { float c;
                                                        // apply ramp to images
        c=(float)(z+1)/(overlap+1);
        vecMulAdd(dp, 1.0f-c, dps, c, dp, slice_size);
      }
     delete[] data;
     data=new_data;
     new_data=NULL;
     depth=new_depth;
                 // transform image back to feet first orientation if necessary
     if (feet_first) feet2head(_data, width, height, _depth);
   }
   catch (...)
    { if (new_data != NULL) delete[] new_data;
      throw;
    }
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Transform image from feet first to head first orientation.
    \param[in,out] dptr   pointer to image data
    \param[in]     w      width of image
    \param[in]     h      height of image
    \param[in]     d      depth of image

    Transform image from feet first to head first orientation. The image is
    mirrored in z- and x-direction.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void Wholebody::feet2head(T * const dptr, const unsigned short int w,
                          const unsigned short int h,
                          const unsigned short int d)
 { if (d < 2) return;
   T *dp;

   ::feet2head(dptr, w, h, d);
   dp=dptr;
   for (unsigned long int zy=0;
        zy < (unsigned long int)d*(unsigned long int)h;
        zy++,
        dp+=w)
    for (unsigned short int x=0; x < w/2; x++)
     std::swap(dp[x], dp[w-1-x]);
 }

#ifndef _WHOLEBODY_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Remove the ownership over the data from this object.
    \param[in] _depth   depth of wholebody image
    \return pointer to wholebody data

    Remove the ownership over the data from this object. The ownership goes
    over to the caller. The data is not released.
 */
/*---------------------------------------------------------------------------*/
float *Wholebody::getWholebody(unsigned short int * const _depth)
 { float *dp;

   dp=data;
   data=NULL;
   *_depth=depth;
   return(dp);
 }
#endif
