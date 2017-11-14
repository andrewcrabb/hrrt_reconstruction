// Created by Vladimir Panin
// Tomographic reconstruction algorithms for parallel or fan beam 3D

// One important notice

// Data are orginized in such way that z-direction of image or projection come
// as first array index


/*****************************************************************************
                           Declarations of
                    Abstract Projected_Imag_3De class

*****************************************************************************/

#ifndef _PROJECTED_IMAGE_3D
#define _PROJECTED_IMAGE_3D

#include <cstdlib>

#ifdef WIN32
class __declspec(dllexport) Projected_Image_3D
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
class Projected_Image_3D
#endif
 { protected:
                                                               // Basic members
    int msize,
        slice,
        nang,
        bins;
    float bwid,               // bin width for detector in terms of pixel width
          *proj,
          *img;
                                     // Often used derivatives of basic members
    int isize,
        psize,
        i_msize_offset,
        p_nang_offset;
    float AXIS,
          H,
          BWID;
    float BinFrac(int) const;
       // Entry point into image array for a ray-driven projector/backprojector
    void entry_point(float, float, const float, float * const, float * const,
                     float * const, float * const, int * const, int * const,
                     int * const, int * const, int * const, int * const,
                     int * const, int * const) const;
    void generate_subsets(int ** const, const int, const int) const;
    void max_min_ML_EM() const;
                                                       // Derivatives of Priors
    virtual float der_U(const int, const float * const) const;
    virtual float term_U(const int, const float * const) const;
                                                     // Projector/Backprojector
    virtual void plf(float *, const int, const float, const float) const=0;
    virtual void plf(float *, const int, const float, const float,
                     float * const) const=0;
    virtual void blf(float *, const int, const float, const float,
                     float * const) const=0;
   public:
    Projected_Image_3D(int, int, int, int, float);
    virtual ~Projected_Image_3D();
                                                              // Copy operation
    void copy_image(float * const) const;
    void copy_proj(float * const) const;
                                                                  // Projection
    void make_projection(const int, const int, const float) const;
    void make_backprojection(const int, const int, const float) const;
                                                              // Reconstruction
                                                             // OS-EM algorithm
    void OS_EM(const int, const int, const int, const int, const float,
               const float, const unsigned short int *, const bool,
               const unsigned short int) const;
                                                              // Read operators
    void read_image(const float * const) const; 
    void read_proj(const float * const) const;
 };

#endif
