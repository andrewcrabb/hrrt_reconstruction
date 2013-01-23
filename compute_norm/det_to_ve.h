#ifndef det_to_ve_h
#define det_to_ve_h

#include <complex>
extern void det_to_cmplx(int det, int doi, std::complex<float> &v, float crystal_depth);
extern int det_to_ve(const std::complex<float> &v1, const std::complex<float> &v2);

#endif
