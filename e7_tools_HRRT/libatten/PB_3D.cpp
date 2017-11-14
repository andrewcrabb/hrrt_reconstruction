#if defined(__linux__) && defined(__INTEL_COMPILER)
#include <mathimf.h>
#else
#include <cmath>
#endif
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "const.h"
#include "PB_3D.h"
#include <string.h>
/*****************************************************************************
                           Declarations of
                    Concrete Papallel Projected PB_3D class

*****************************************************************************/

PB_3D::PB_3D(const int size, const int size_z, const int nang, const int bins,
             const float bwid):
 Projected_Image_3D(size, size_z, nang, bins, bwid)     
 {
 }

void PB_3D::blf(float *p, const int, const float sintheta,
                const float costheta, float * const b) const
 { int i, j, k, ki, kj, ii, jj, iout, jout, k_z;
   float xl, yl, dxl, dyl, z, *p_b;
                                  // Start from the lowest position of detector
   z=-AXIS*bwid;
   for (k=0; k < bins; k++)
    { z+=bwid; 
      entry_point(costheta, sintheta, z, &xl, &yl, &dxl, &dyl, &ki, &kj, &i,
                  &j, &ii, &jj, &iout, &jout);
                                              // superimpose all the pixel data
      if (((ii >= 1) && (ii <= msize)) || ((jj >= 1) && (jj <= msize)))
       { p_b=&b[(i-1)*slice+(j-1)*i_msize_offset];
         while ((i != iout) && (j != jout))
          if (xl < yl) { for (k_z=0; k_z < slice; k_z++)
                          p_b[k_z]+=xl*p[k_z];
                         i+=ki;
                         p_b+=ki*slice;
                         yl-=xl;
                         xl=dxl;
                       }
           else { for (k_z=0; k_z < slice; k_z++)
                   p_b[k_z]+=yl*p[k_z];
                  j+=kj;
                  p_b+=kj*i_msize_offset;
                  xl-=yl;
                  yl=dyl;
                }
       }
      p+=slice;
    }
 }

/****************************************************************************/
void PB_3D::change_COR(const float offset)
 { AXIS+=offset;
 }

/****************************************************************************/
void PB_3D::plf(float *p, const int, const float sintheta,
                const float costheta) const
 { int i, j, k, ki, kj, ii, jj, iout, jout, k_z;
   float xl, yl, dxl, dyl, z, *p_img;
                                  // Start from the lowest position of detector
   z=-AXIS*bwid;
   for (k=0; k < bins; k++)
    { memset(p, 0, slice*sizeof(float));
      z+=bwid;
      entry_point(costheta, sintheta, z, &xl, &yl, &dxl, &dyl, &ki, &kj, &i,
                  &j, &ii, &jj, &iout, &jout);
                                              // superimpose all the pixel data
      if (((ii >= 1) && (ii <= msize)) || ((jj>=1) && (jj<=msize)))
       { p_img=&img[(i-1)*slice+(j-1)*i_msize_offset];
         while ((i != iout) && (j != jout))
          if (xl < yl) { for (k_z=0; k_z < slice; k_z++)
                          p[k_z]+=xl*p_img[k_z];
                         i+=ki;
                         p_img+=ki*slice;
                         yl-=xl;
                         xl=dxl;
                       }
           else { for (k_z=0; k_z < slice; k_z++)
                   p[k_z]+=yl*p_img[k_z];
                  j+=kj;
                  p_img+=kj*i_msize_offset;
                  xl-=yl;
                  yl=dyl;
                }
       }
      p+=slice;
    }
 }

void PB_3D::plf(float *p, const int, const float sintheta,
                const float costheta, float * const b) const
 { int i, j, k, ki, kj, ii, jj, iout, jout, k_z;
   float xl, yl, dxl, dyl, z, *p_b;
                                  // Start from the lowest position of detector
   z=-AXIS*bwid;
   for (k=0; k < bins; k++)
    { memset(p, 0, slice*sizeof(float));
      z+=bwid;
      entry_point(costheta, sintheta, z, &xl, &yl, &dxl, &dyl, &ki, &kj, &i,
                  &j, &ii, &jj, &iout, &jout);
                                              // superimpose all the pixel data
      if (((ii >= 1) && (ii <= msize)) || ((jj >= 1) && (jj <= msize)))
       { p_b=&b[(i-1)*slice+(j-1)*i_msize_offset];
         while ((i != iout) && (j != jout))
          if (xl < yl) { for (k_z=0; k_z < slice; k_z++)
                          p[k_z]+=xl*p_b[k_z];
                         i+=ki;
                         p_b+=ki*slice;
                         yl-=xl;
                         xl=dxl;
                       }
           else { for (k_z=0; k_z < slice; k_z++)
                   p[k_z]+=yl*p_b[k_z];
                  j+=kj;
                  p_b+=kj*i_msize_offset;
                  xl-=yl;
                  yl=dyl;
                }
       }
      p+=slice;
    }
 }
