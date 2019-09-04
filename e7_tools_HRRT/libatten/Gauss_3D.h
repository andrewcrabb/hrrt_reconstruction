#pragma once

class Gauss_3D
 { private:
    int size,
        size_z,
        isize,
        size_i,
        size_j,
        size_k;
   public:
    Gauss_3D(const int, const int); 
    float der_U(const int, const float * const) const;
    /* My change ahc the following function was not declared const */
    float der_der_U(const int, const float * const) const;
    float term_U(const int, const float * const) const;
 };
