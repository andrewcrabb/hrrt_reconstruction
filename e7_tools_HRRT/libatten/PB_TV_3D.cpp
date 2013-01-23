#if defined(__LINUX__) && defined(__INTEL_COMPILER)
#include <mathimf.h>
#else
#include <cmath>
#endif
#include "PB_TV_3D.h"

PB_TV_3D::PB_TV_3D(const int size, const int size_z, const int nang,
                   const int bins, const float bwid):
 PB_3D(size, size_z, nang, bins, bwid), Reg(size, size_z, pow(0.0001, 2))   
 {
 }

float PB_TV_3D::der_U(const int i, const float * const image) const
 { return(Reg.der_U(i, image));
 }


float PB_TV_3D::term_U(const int i, const float * const image) const
 { return(Reg.term_U(i, image));
 }
