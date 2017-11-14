#if defined(__linux__) && defined(__INTEL_COMPILER)
#include <mathimf.h>
#else
#include <cmath>
#endif
#include "TV_3D.h"

/************************************************************************
Total Variation functional - absolute value of gradient
Edge preserving. Depends on additional very small parameter in order to
make function differentiable
TV functional has nice propery - preserve edge but preserve smooth function
as well
*************************************************************************/
TV_3D::TV_3D(const int size_, const int size_z_, const float e_) 
 { size=size_;
   size_z=size_z_;
   isize=size*size_z;
   size_i=size_z-1;
   size_j=size-1;
   size_k=size-1;
   e=e_;
 }

// Upper limit of second derivative for TV functional
float TV_3D::der_der_U(const int, const float * const) const
 { return(0.0f);
 }

float TV_3D::der_U(const int index, const float * const img) const
 { int i, j, k;
   float term_1, term_2, term_3, term_4;

   i=(index % isize) % size_z;
   j=(index % isize)/size_z;
   k=index/isize;
   // Voxels that are out of array set to be equal to boundary voxel 
   term_1=(i == 0 ? 0.0f : (img[0]-img[-1])/term_u(i-1,j,k, img-1));
   term_2=(j == 0 ? 0.0f : (img[0]-img[-size_z])/term_u(i,j-1,k, img-size_z));
   term_3=(k == 0 ? 0.0f : (img[0]-img[-isize])/term_u(i,j,k-1, img-isize));
   if ((i == size_i) && (j == size_j) && (k == size_k)) term_4=0.0f;
    else term_4=((i == size_i ? 0.0f : img[1]-img[0])+ 
                 (j == size_j ? 0.0f : img[size_z]-img[0])+ 
                 (k == size_k ? 0.0f : img[isize]-img[0]))/
                 term_u(i, j, k, img);
   return(term_1+term_2+term_3-term_4);
 }

float TV_3D::term_u(const int i, const int j, const int k,
                    const float * const img) const
 { return(sqrt((i == size_i ? 0.0f : (img[1]-img[0])*(img[1]-img[0]))+ 
	       (j == size_j ? 0.0f : (img[size_z]-img[0])*
                                     (img[size_z]-img[0]))+
               (k == size_k ? 0.0f : (img[isize]-img[0])*
                                     (img[isize]-img[0]))+e));
 }

float TV_3D::term_U(const int index, const float * const img) const
 { int i, j, k;

   i=(index % isize) % size_z;
   j=(index % isize)/size_z;
   k=index/isize;
   return(term_u(i, j, k, img));
 }
