#ifndef _TX_PB_TV_3D_H
#define _TX_PB_TV_3D_H

#include "TV_3D.h"
#include "Tx_PB_3D.h"

template <typename T> class Tx_PB_TV_3D:public Tx_PB_3D <T>
 { private:
    TV_3D Reg;
    float der_der_U(const int, const float * const) const;
    float der_U(const int, const float * const) const;
    float term_U(const int, const float * const) const;
   public :
    Tx_PB_TV_3D(const int, const int, const int, const int, const float);
 };

#ifndef _TX_PB_TV_3D_CPP
#include "Tx_PB_TV_3D.h"
#endif

#endif
