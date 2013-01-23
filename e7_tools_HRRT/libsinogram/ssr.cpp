/*! \class SSR ssr.h "ssr.h"
    \brief This class implements the single-slice rebinning.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/09/21 added Doxygen style comments
 
    This class implements the single-slice rebinning. The plane offset for each
    segment is calculated and the segments are added up. Some border planes of
    the segments can be skipped, e.g. if the statistics in these planes is too
    low.
 */

#include <iostream>
#include <string>
#include <cstring>
#include "ssr.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "vecmath.h"

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _RhoSamples         number of bins in projection
    \param[in] _ThetaSamples       number of angles in sinogram
    \param[in] _axis_slices        number of planes for axes of 3d sinogram
    \param[in] skip_outer_planes   number of planes to skip
    \param[in] loglevel            level for logging
 
    Initialize object and allocate memory for resulting 2d sinogram.
 */
/*---------------------------------------------------------------------------*/
SSR::SSR(const unsigned short int _RhoSamples,
         const unsigned short int _ThetaSamples,
         const std::vector <unsigned short int> _axis_slices,
         const unsigned short int skip_outer_planes,
         const unsigned short int loglevel)
 { std::string str;
   unsigned short int axes_slices;

   Logging::flog()->logMsg("initialize SSR", 2);
   RhoSamples=_RhoSamples;
   ThetaSamples=_ThetaSamples;
   axis_slices=_axis_slices;
   Logging::flog()->logMsg("RhoSamples=#1", 3)->arg(RhoSamples);
   Logging::flog()->logMsg("ThetaSamples=#1", 3)->arg(ThetaSamples);
   Logging::flog()->logMsg("axes=#1", 3)->arg(axis_slices.size());
   str=toString(axis_slices[0]);
   axes_slices=axis_slices[0];
   for (unsigned short int i=0; i < axis_slices.size()-1; i++)
    { str+=","+toString(axis_slices[i+1]);
      axes_slices+=axis_slices[i+1];
    }
   Logging::flog()->logMsg("axis table=#1 {#2}", 3)->arg(axes_slices)->
    arg(str);
                                            // memory for normalization factors
   axpro.assign(axis_slices[0], 0);
   sino_plane_size=(unsigned long int)RhoSamples*
                   (unsigned long int)ThetaSamples;
                                             // memory for rebinned 2d sinogram
   sino2d=MemCtrl::mc()->createFloat(sino_plane_size*
                                     (unsigned long int)axis_slices[0],
                                     &sino2d_idx, "ssr", loglevel);
   memset(sino2d, 0, sino_plane_size*(unsigned long int)axis_slices[0]*
                     sizeof(float));
                          // number of planes to skip at each side of a segment
   skip=skip_outer_planes;
   Logging::flog()->logMsg("skip #1 planes at each side of a segment", 3)->
    arg(skip);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Normalize rebinned 2d sinogram.
    \return memory controller index of rebinned sinogram
 
    Normalize rebinned 2d sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SSR::Normalize()
 { float *s2d;
   unsigned short int p;

   for (s2d=sino2d,
        p=0; p < axis_slices[0]; p++,
        s2d+=sino_plane_size)
    if (axpro[p] > 0)
     vecMulScalar(s2d, 1.0f/(float)axpro[p], s2d, sino_plane_size);
   MemCtrl::mc()->put(sino2d_idx);
   return(sino2d_idx);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rebin axis of 3d sinogram into 2d sinogram.
    \param[in] sino3d_axis   pointer to axis of 3d sinogram
    \param[in] axis          number of axis
 
    Rebin axis of 3d sinogram into 2d sinogram.
 */
/*---------------------------------------------------------------------------*/
void SSR::Rebinning(float * const sino3d_axis,
                    const unsigned short int axis)
 { unsigned short int p, pl, planes;
   float *s2d;

   if (axis >= axis_slices.size()) return;
   if (axis == 0) { pl=skip;
                    planes=axis_slices[0]-2*skip;
                  }
    else { pl=(axis_slices[0]-axis_slices[axis]/2)/2+skip;
           planes=axis_slices[axis]/2-2*skip;
         }
   Logging::flog()->logMsg("ssr #1 planes", 3)->arg(planes);
   for (s2d=sino2d+(unsigned long int)pl*sino_plane_size,
        p=0; p < planes; p++,
        s2d+=sino_plane_size)
    { vecAdd(s2d, sino3d_axis+(unsigned long int)(skip+p)*sino_plane_size,
             s2d, sino_plane_size);
      axpro[pl+p]++;
      if (axis == 0) continue;
      vecAdd(s2d,
             sino3d_axis+(unsigned long int)(planes+3*skip+p)*sino_plane_size,
             s2d, sino_plane_size);
      axpro[pl+p]++;
    }
 }
