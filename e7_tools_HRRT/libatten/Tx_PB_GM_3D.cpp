#include <iostream>
#include "Tx_PB_3D.h"
#ifndef _TX_PB_GM_3D_CPP
#define _TX_PB_GM_3D_CPP
#include "Tx_PB_GM_3D.h"
#endif

template <typename T>
Tx_PB_GM_3D <T>::Tx_PB_GM_3D(const int size, const int size_z, const int nang,
                             const int bins, const float bwid,
                             const float delta):
  Tx_PB_3D <T>(size, size_z, nang, bins, bwid), Reg(size, size_z, delta)   
 {
 }

template <typename T>
float Tx_PB_GM_3D <T>::der_der_U(const int i, const float * const image) const
 { return(Reg.der_der_U(i, image));
 }

template <typename T>
float Tx_PB_GM_3D <T>::der_U(const int i, const float * const image) const
 { return(Reg.der_U(i, image));
 }

template <typename T>
float Tx_PB_GM_3D <T>::term_U(const int i, const float * const image) const
 { return(Reg.term_U(i, image));
 }
