#ifndef _TX_PB_TV_3D_CPP
#define _TX_PB_TV_3D_CPP
#include "Tx_PB_TV_3D.h"
#endif

template <class T>
Tx_PB_TV_3D <T>::Tx_PB_TV_3D(const int size, const int size_z, const int nang,
                             const int bins, const float bwid):
 Tx_PB_3D <T>(size, size_z, nang, bins, bwid),
 Reg(size, size_z, pow(0.0001, 2))   
 {
 }

template <class T>
float Tx_PB_TV_3D <T>::der_der_U(const int i, const float * const image) const
 { return(Reg.der_der_U(i, image));
 }

template <class T>
float Tx_PB_TV_3D <T>::der_U(const int i, const float * const image) const
 { return(Reg.der_U(i, image));
 }

template <class T>
float Tx_PB_TV_3D <T>::term_U(const int i, const float * const image) const
 { return(Reg.term_U(i, image));
 }
