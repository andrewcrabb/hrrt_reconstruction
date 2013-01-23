#ifndef _PB_TV_3D_H
#define _PB_TV_3D_H

#include "PB_3D.h"
#include "TV_3D.h"

#ifdef WIN32
class __declspec(dllexport) PB_TV_3D:public PB_3D
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
class PB_TV_3D:public PB_3D
#endif
 { private:
    TV_3D Reg;
    float der_U(const int, const float * const) const;
    float term_U(const int, const float * const) const;
   public:
    PB_TV_3D(const int, const int, const int, const int, const float);
 };

#endif
