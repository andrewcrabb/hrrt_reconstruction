#ifndef _PB_3D_H
#define _PB_3D_H

#include "Projected_Image_3D.h"

/*****************************************************************************
                           Declarations of
                    Papallel Beam Projected PB_3D class

*****************************************************************************/

#ifdef WIN32
class __declspec(dllexport) PB_3D:public Projected_Image_3D
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
class PB_3D:public Projected_Image_3D
#endif
 { protected:
    void blf(float *, const int, const float, const float,
             float * const) const;
    void plf(float *, const int, const float, const float) const;
    void plf(float *, const int, const float, const float,
             float * const) const;
   public:
    PB_3D(const int, const int, const int, const int, const float);
    void change_COR(const float);
 };

#endif
