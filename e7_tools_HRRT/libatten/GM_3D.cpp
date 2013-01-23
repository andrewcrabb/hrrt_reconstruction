#include "GM_3D.h"

/*****************************************************************************
GM functional. Non convex. Edge preserving. 
Strongly depends on additional parameter (delta). 
This parameter defines edge trashhold.
Gradient above this parameter is edge and will be preserved.
Gradient below this parameter is noise and will be smoothed out. 
******************************************************************************/

GM_3D::GM_3D(const int size_, const int size_z_, const float delta_)
 { size=size_;
   size_z=size_z_;
   isize=size*size_z;
   size_i=size_z-1;
   size_j=size-1;
   size_k=size-1;
   delta_2=delta_*delta_;
 }

float GM_3D::der_der_U(const int, const float * const) const
 { return(1.0f/delta_2);
 }

float GM_3D::der_U(const int index, const float * const img) const
 { int i, j, k;

   i=(index % isize) % size_z;
   j=(index % isize)/size_z;
   k=index/isize;
   return((i == 0      ? 0.0f : term_u(*(img)-*(img-1)))+
	  (i == size_i ? 0.0f : term_u(*(img)-*(img+1)))+
	  (j == 0      ? 0.0f : term_u(*(img)-*(img-size_z)))+
	  (j == size_j ? 0.0f : term_u(*(img)-*(img+size_z)))+
	  (k == 0      ? 0.0f : term_u(*(img)-*(img-isize)))+
	  (k == size_k ? 0.0f : term_u(*(img)-*(img+isize))));
 }

float GM_3D::term_u(const float r) const
 { return((r/delta_2)/((1.0f+r*r/delta_2)*(1.0f+r*r/delta_2))) ;
 }

float GM_3D::term_U(const int, const float * const) const
 { return(0.0f);
 }
