/*
Modification History
    \author Merence Sibomana  - HRRT users community (sibomana@gmail.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)
*/
#if defined(__LINUX__) && defined(__INTEL_COMPILER)
#include <mathimf.h>
#else
#include <cmath>
#endif
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "Projected_Image_3D.h"
#include "const.h"
#include "exception.h"
#ifdef WIN32
#include "global_tmpl.h"
#endif
#include <string.h>
/*****************************************************************************
                              Methods of
                    Abstract Projected_Image_3D class

*****************************************************************************/

Projected_Image_3D::Projected_Image_3D(const int msize_, const int slice_,
                                       const int nang_, const int bins_,
                                       const float bwid_) 
 { img=NULL;
   proj=NULL;
   try
   { msize=msize_;
     slice=slice_;
     nang=nang_;
     bins=bins_;
     bwid=bwid_;
     isize=slice*msize*msize;
     psize=slice*bins*nang;
     i_msize_offset=slice*msize;
     p_nang_offset=slice*bins;
     img=new float[isize];
     proj=new float[psize];
     H=msize*0.5f;                                           // Center of image
     AXIS=(((float)bins+1.0f)*0.5f);         // Center of the detector plus 0.5
   }
   catch (...)
    { if (img != NULL) { delete[] img;
                         img=NULL;
                       }
      if (proj != NULL) { delete[] proj;
                          proj=NULL;
                        }
      throw;
    }
 }
                  
Projected_Image_3D::~Projected_Image_3D()
 { if (img != NULL) delete[] img;
   if (proj != NULL) delete[] proj;
 }

// Generation of subsets
 float Projected_Image_3D::BinFrac(int D) const
 { float V=0.0f, F=0.5f;

   while (D != 0)
    { if (D % 2 != 0) V+=F;
      D/=2;
      F/=2.0f;
    }
   return(V);
 }

// Copy operations
// Copy image and projection into external world. Rearange indexes
void Projected_Image_3D::copy_image(float * const external) const
 { int i, j, k, i_input_offset;
   float *ip;

   i_input_offset=msize*msize;
   ip=img;
   for (j=0; j < msize; j++)
    for (i=0; i < msize; i++)
     for (k=0; k < slice; k++)
      external[k*i_input_offset+j*msize+i]=*ip++;
 }

void Projected_Image_3D::copy_proj(float * const external) const
 { int k, j, m, p_input_offset;
   float *pp;

   p_input_offset=bins*nang;
   pp=proj;
   for (m=0; m < nang; m++)
    for (j=0; j < bins; j++)
     for (k=0; k < slice; k++)
      external[k*p_input_offset+m*bins+j]=*pp++;
 }

float Projected_Image_3D::der_U(const int, const float * const) const
 { return(0.0f);
 }

/****************************************************************************/
void Projected_Image_3D::entry_point(float c, float s, const float z,
                                     float * const xl, float * const yl,
                                     float * const dxl, float * const dyl,
                                     int * const ki, int * const kj,
                                     int * const i, int * const j,
                                     int * const ii, int * const jj,
                                     int * const iout, int * const jout) const
 { int i1st, j1st;
   float xoff, yoff, x, y, dx, dy;
                            // Remove possible instability at k*pi/4, k=1,3,5,7
   if (fabs(c) == fabs(s))
    { c+=EPS;
      if (s > 0) s=(float)(sqrt(1.0-c*c));
       else s=-(float)(sqrt(1.0-c*c));
    }
                                              // set the edge judging variables
   if (c < 0.0f) { *ki=1;
                   i1st=1;
                   *iout=msize+1;
                   xoff=0.0f;
                 }
    else { *ki=-1;
           i1st=msize;
           *iout=0;
           xoff=1.0f;
         }
   if (s < 0.0f) { *kj=1;
                   j1st=1;
                   *jout=msize+1;
                   yoff=0.0f;
                 }
    else { *kj=-1;
           j1st=msize;
           *jout=0;
           yoff=1.0f;
         }
                                      // calculate dxl, dyl and (x,0), (y,0) at
                                      // the outside edges of the pixel array 
   if ((s < EPS) && (s > -EPS)) s=-*kj*EPS;
   if ((c < EPS) && (c > -EPS)) c=-*ki*EPS;
   dx=1.0f/c;
   dy=1.0f/s;
   // use correct center of rotation, as in emission (VP, 03/11/04)
   //   x=(-*kj*H*c-z)*dy+H;     // original equations
   //   y=(-*ki*H*s+z)*dx+H;   
   //   x=(-*kj*(H+*kj*0.25f)*c-z)*dy+H+0.25f; // correct equations ?
   //   y=(-*ki*(H+*ki*0.25f)*s+z)*dx+H+0.25f;
   x=(-*kj*(H+*kj*0.5f)*c-z)*dy+H+0.5f;    // best result regarding slope
   y=(-*ki*(H+*ki*0.5f)*s+z)*dx+H+0.5f;

   *dxl=dx;
   *dyl=dy;
   if (*dxl < 0.0f) *dxl=-*dxl;
   if (*dyl < 0.0f) *dyl=-*dyl;
   *ii=(int)(x+1.0f);
   *jj=(int)(y+1.0f);
   if ((*jj >= 1) && (*jj <= msize))
    { *j=*jj;
      *i=i1st;
      *yl=(*j-y-yoff)**dyl;
      if (*yl < 0.0f) *yl=-*yl;
      *xl=*dxl;
    }
   if ((*ii >= 1) && (*ii <= msize))
    { *i=*ii;
      *j=j1st;
      *xl=(*i-x-xoff)**dxl;
      if (*xl < 0.0f) *xl=-*xl;
      *yl=*dyl;
    }
 }

void Projected_Image_3D::generate_subsets(int ** const subsets,
                                          const int angles,
                                          const int subset_size) const
 { int *A=NULL;

   try
   { int Aleft, D, Snum=0, k=0, Aj, Si;
     float Ai;

     A=new int[angles];
     memset(A, 0, angles*sizeof(int));
     Aleft=angles;
     while (Aleft > subset_size)
      { D=Snum;
        Ai=angles/subset_size*BinFrac(D);
        for (Si=0; Si < subset_size; Si++)
         { Aj=(int)((Ai+ceil((float)angles/subset_size)*Si)-
                    ((int)(Ai+ceil((float)angles/subset_size)*Si)/angles)*
                    angles);
           while (A[Aj] != 0) Aj=(Aj+1) % angles;
           A[Aj]=1;
           subsets[k][Si]=Aj;
         }
        Aleft-=subset_size;
        Snum++; 
        k++;
      }
                      // now calculate last (possibly nonstandard sized) subset
     D=Snum;
     Ai=angles/subset_size*BinFrac(Snum);
     for (Si=0; Si < Aleft; Si++)
      { Aj=(int)((Ai+ceil((float)angles/subset_size)*Si)-
                 ((int)(Ai+ceil((float)angles/subset_size)*Si)/angles)*angles);
        while (A[Aj] != 0) Aj=(Aj+1) % angles;
        A[Aj]=1;
        subsets[k][Si]=Aj;
      }
     delete[] A;
     A=NULL;
   }
   catch (...)
    { if (A != NULL) delete[] A;
      throw;
    }
 }

void Projected_Image_3D::make_backprojection(const int rotation,
                                             const int div_twopi,
                                             const float angle_shift) const
 { float *p=NULL;

   try
   { int m;
     float theta;
  
     p=new float[p_nang_offset];
     memset(img, 0, isize*sizeof(float));
     for (m=0; m < nang; m++)
      { memcpy(p, proj+m*p_nang_offset, p_nang_offset*sizeof(float));
        theta=(float)(m*rotation*2)*M_PI/(float)(div_twopi*nang)+angle_shift;
        blf(p, m, sin(theta), cos(theta), img);
      }
     delete[] p;
   }
   catch (...)
    { if (p != NULL) delete[] p;
      throw;
    }
 }

/****************************************************************************/
// Projection
void Projected_Image_3D::make_projection(const int rotation,
                                         const int div_twopi,
                                         const float angle_shift) const
 { float *p=NULL;

   try
   { int m; 
     float theta, *p;

     p=new float[p_nang_offset];
     for (m=0; m < nang; m++)
      { theta=(float)(m*rotation*2)*M_PI/(float)(div_twopi*nang)+angle_shift;
        plf(p, m, sin(theta), cos(theta), img);
        memcpy(proj+m*p_nang_offset, p, p_nang_offset*sizeof(float));
      }  
     delete[] p;
   }
   catch (...)
    { if (p != NULL) delete[] p;
      throw;
    }
 }

void Projected_Image_3D::max_min_ML_EM() const
 { float max_value=img[0];
   float min_value=img[0];

   for (int i=0; i < isize; i++)
    { max_value=std::max(max_value, img[i]);
      min_value=std::min(min_value, img[i]);
    }
   //std::cerr << min_value << " " << max_value << std::endl;
   if ((min_value < 0.0f) || (max_value > 2.0f))
    throw Exception(REC_UMAP_RECON_OUT_OF_RANGE,
                    "The voxel values in the u-map are running out-of-range "
                    "during reconstruction.");
 }

// Code contains redundancy:
// Backprojection of mask/one is performed for each iteration
// It is slower, but it saves memory
void Projected_Image_3D::OS_EM(const int niter, const int subs,
                               const int rotation, const int div_twopi,
                               const float angle_shift, const float beta,
                               const unsigned short int *mask,
                               const bool initial,
                               const unsigned short int loglevel) const
 { float *p1=NULL, *cdel=NULL, *del=NULL;
   int **subsets=NULL;

   try
   { int i, j, ms, m, iter, subset_size;
     float ang, sinphi, cosphi, back_Gibbs;
     bool mask_enforced=false;

     p1=new float[p_nang_offset];
                                     // Backprojections of one for each subsets
     del=new float[isize];
     cdel=new float[isize];
     if (!initial)
      for (i=0; i < isize; i++) 
       img[i]=0.01f;
     if (mask != NULL) mask_enforced=true;
                                                            // Generate subsets
     subset_size=nang/subs;
     subsets=new int *[subs];
     for (ms=0; ms < subs; ms++)
      subsets[ms]=new int[subset_size];
     generate_subsets(subsets, nang, subset_size);
                                       // OS-EM algorithm starts iterating here
     for (iter=0; iter < niter; iter++)
      { Logging::flog()->logMsg("iteration: #1", loglevel)->arg(iter+1);
        for (ms = 0; ms < subs; ms++)
         { memset(del, 0, isize*sizeof(float));
           memset(cdel, 0, isize*sizeof(float));
           for (j=0; j < subset_size; j++)
            { int offset;

              m=subsets[ms][j];
              offset=m*p_nang_offset;
              ang=(float)(m*rotation*2)*M_PI/(float)(div_twopi*nang)+
                  angle_shift;
              sinphi=sin(ang);
              cosphi=cos(ang);
                                        // project img[] onto p1[] at angle ang
              plf(p1, m, sinphi, cosphi);
              for (i=0; i < p_nang_offset; i++)
               { if (p1[i] == 0.0f) p1[i]=1.0f;
                  else p1[i]=proj[offset+i]/p1[i];
                 if (mask_enforced) p1[i]*=mask[offset+i];
               }
                                                 // backproject p1[] into del[]
              blf(p1, m, sinphi, cosphi, del);
              for (i=0; i < p_nang_offset; i++)
               if (mask_enforced) p1[i]=mask[offset+i];
                else p1[i]=1.0f;
              blf(p1, m, sinphi, cosphi, cdel);
            }
                                                                // Update image
           if (beta != 0.0f)
            for (i=0; i < isize; i++)
             if ((back_Gibbs=cdel[i]+beta*der_U(i, img)) < 0.0f)
              throw Exception(REC_NONNEG_CONSTRAINT,
                              "Violation of nonnegativity constraint in OSEM "
                              "reconstruction.");
              else img[i]*=del[i]/back_Gibbs;
            else for (i=0; i < isize; i++) 
                  if (cdel[i] != 0.0f) img[i]*=del[i]/cdel[i];
           max_min_ML_EM();
         }
      }
     delete[] p1;
     p1=NULL;
     delete[] cdel;
     cdel=NULL;
     for (ms=0; ms < subs; ms++)
      { delete[] subsets[ms];
        subsets[ms]=NULL;
      }
     delete[] subsets;
     subsets=NULL;
     delete[] del;
     del=NULL;
   }
   catch (...)
    { if (p1 != NULL) delete[] p1;
      if (cdel != NULL) delete[] cdel;
      if (del != NULL) delete[] del;
      if (subsets != NULL)
       { for (unsigned short int ms=0; ms < subs; ms++)
          if (subsets[ms] != NULL) delete[] subsets[ms];
         delete[] subsets;
       }
      throw;
    }
 }

/****************************************************************************/
// Read operations
// Read image and projection from external word and rearange indexes
void Projected_Image_3D::read_image(const float * const external) const
 { int i, j, k, i_input_offset;
   float *ip;

   i_input_offset=msize*msize;
   ip=img;
   for (j=0; j < msize; j++)
    for (i=0; i < msize; i++)
     for (k=0; k < slice; k++)
      *ip++=external[k*i_input_offset+j*msize+i];
 }

void Projected_Image_3D::read_proj(const float * const external) const
 { int k, j, m, p_input_offset;
   float *pp;

   p_input_offset=bins*nang;
   pp=proj;
   for (m=0; m < nang; m++)
    for (j=0; j < bins; j++)
     for (k=0; k < slice; k++)
      *pp++=external[k*p_input_offset+m*bins+j];
 }

float Projected_Image_3D::term_U(const int, const float * const) const
 { return(0.0f);
 }
