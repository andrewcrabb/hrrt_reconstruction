#ifndef FFT_H_
#define FFT_H_

#ifndef F_PI_DEFINITION
#define F_PI_DEFINITION

#include <cmath>

const double F_PI = 2.0 * asin(1.0);
#endif

int FFTa_real_inverse(float a[], int n, int ntot, int nstep);
int FFTa_real_direct(float a[], int n, int ntot, int nstep);    
int FFT_real_inverse(float x[], int n, int nu, int ntot, int nstep);
int FFT_real_direct(float x[], int n, int nu, int ntot, int nstep);

#endif /*FFT_H_*/
