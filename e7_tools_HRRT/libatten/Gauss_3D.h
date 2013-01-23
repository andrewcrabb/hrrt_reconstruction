#ifndef _GAUSS_3D_H
#define _GAUSS_3D_H

#ifdef WIN32
class __declspec(dllexport) Gauss_3D
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
class Gauss_3D
#endif
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
    float der_der_U(const int, const float * const);
    float term_U(const int, const float * const) const;
 };

#endif
