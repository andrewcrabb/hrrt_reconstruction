#pragma once

class TV_3D
 { private:
    int size,
        size_z,
        isize,
        size_i,
        size_j,
        size_k;
    float e;
    float term_u(const int, const int, const int, const float * const) const;
   public:
    TV_3D(const int, const int, const float);
    float der_der_U(const int, const float * const) const;
    float der_U(const int, const float * const) const;
    float term_U(const int, const float * const) const;
 };
