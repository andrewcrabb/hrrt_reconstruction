// Created by Vladimir Panin

/*****************************************************************************
Tomographic reconstruction algorithms

Concrete class of reconstruction algoritm of attenuation map 
from Parallel Beam projections 
******************************************************************************/
/*
Modification History
    \author Merence Sibomana  - HRRT users community (sibomana@gmail.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)
*/

#pragma once

#include <vector>
#include "PB_3D.h"

/*****************************************************************************
                           Declarations of
                    Papallel Beam Projected PB_Image class

*****************************************************************************/

template <typename T> class Tx_PB_3D:public PB_3D
 { private:
    T *tx,
      *blank;
    float *norm;
    void calculate_proj_norm() const;
    void IP_grad(float * const, float * const, const float, const int,
                 const float * const, const float * const, const float * const,
                 const float * const) const;
    void IP_grad_compute(float * const, float * const, const float,
                         const float, const float, const bool) const;
    float IP_prior(const float, const int, const float * const,
                   const float * const, const float * const,
                   const float * const) const;
    float IP_prior_compute(const float, const float, const float,
                           const bool) const;
   protected:
    float der_der_U(const int, const float * const) const;
   public:
                                                                // Constructors
    Tx_PB_3D(const int, const int, const int, const int, const float);
    virtual ~Tx_PB_3D();
    void MAPTR(const int, const int, const int, const float, const float,
               const unsigned short int, const float * const,
               const float * const, const float * const, const float,
               std::vector <unsigned long int> *, const int, const float,
               const float, const float, const int, const bool, const bool,
               const float, const unsigned short int);
    void read_projections(const T * const, const T * const, const float);
 };

/*
 PMB
*/
extern float hrrt_tx_scatter_a, hrrt_tx_scatter_b;
extern float hrrt_blank_factor;
