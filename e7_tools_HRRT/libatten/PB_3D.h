#pragma once

#include "Projected_Image_3D.h"

/*****************************************************************************
                           Declarations of
                    Papallel Beam Projected PB_3D class
*****************************************************************************/

class PB_3D:public Projected_Image_3D
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
