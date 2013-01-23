#include "Gauss_3D.h"

/*****************************************************************************
Gauss functional. Convex. Smooth noise and edges. 
Do not depend on any additional parameter. 
******************************************************************************/

Gauss_3D::Gauss_3D(const int size_, const int size_z_)
 { size=size_;
   size_z=size_z_;
   isize=size*size_z;
   size_i=size_z-1;
   size_j=size-1;
   size_k=size-1;
 }

float Gauss_3D::der_U(const int index, const float * const img) const
 { int i, j, k;

   i=(index % isize) % size_z;
   j=(index % isize)/size_z;
   k=index/isize;
   // Voxels that are out of array set to be equal to boundary voxel 
   return(6**img-(k == 0      ? *img : *(img-isize))- 
                 (k == size_k ? *img : *(img+isize))- 
                 (j == 0      ? *img : *(img-size_z))- 
                 (j == size_j ? *img : *(img+size_z))- 
                 (i == 0      ? *img : *(img-1))- 
                 (i == size_i ? *img : *(img+1)));
 }

float Gauss_3D::der_der_U(const int index, const float * const)
 { int i, j, k;

   i=(index % isize) % size_z;
   j=(index % isize)/size_z;
   k=index/isize;
   return((k == 0      ? 0 : 1)+
          (k == size_k ? 0 : 1)+ 
          (j == 0      ? 0 : 1)+ 
          (j == size_j ? 0 : 1)+ 
          (i == 0      ? 0 : 1)+ 
          (i == size_i ? 0 : 1));
 }

float Gauss_3D::term_U(const int index, const float * const img) const
 { int i, j, k;

   i=(index % isize) % size_z;
   j=(index % isize)/size_z;
   k=index/isize;
   return(((i == size_i ? 0 : (*(img+1)-*img)*(*(img+1)-*img))+
           (j == size_j ? 0 : (*(img+size_z)-*img)*(*(img+size_z)-*img))+
           (k == size_k ? 0 : (*(img+isize)-*img)*(*(img+isize)-*img)))/2.0);
 }

