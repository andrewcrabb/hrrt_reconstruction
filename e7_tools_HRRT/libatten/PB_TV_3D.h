#pragma once

#include "PB_3D.h"
#include "TV_3D.h"

class PB_TV_3D:public PB_3D
 { private:
    TV_3D Reg;
    float der_U(const int, const float * const) const;
    float term_U(const int, const float * const) const;
   public:
    PB_TV_3D(const int, const int, const int, const int, const float);
 };
