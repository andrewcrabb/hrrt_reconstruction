#ifndef CONFOSTHREAD_H_
#define CONFOSTHREAD_H_

#include <windows.h>

typedef struct {
  float* sinogram;
  float* norm;
  int nelems;
  int nangles;
  int nplanestoprocess;
  int planestart;
  int nsegs;
  int niter;
  int extention;
  int interpolarea;
  int nanglesbig;
} ConFoSp;

int confos2D_thread (float* sinogram, float* norm, int extention, int niter, int nelems, int nangles, int nplanes, int interpolarea, bool verboseMode, int nthreads);
int confos3D_thread (float* sinogram, float* norm, int extention, int niter, int nelems, int nangles, int* segplanes, int nsegs, int interpolarea, bool verboseMode, int nthreads);
int confos3D_thread2 (float* sinogram, float* norm, int extention, int niter, int nelems, int nangles, int* segplanes, int nsegs, int interpolarea, bool verboseMode, int nthreads);
int createMask(float* mask, int nelems, int nangles);

#endif /*CONFOSTHREAD_H_*/
