#ifndef GAPFILL_H_
#define GAPFILL_H_

int linearinterpolation_angles(float* sinogram2D, float* normsino2D, int nelems, int nangles, int nplanes, bool verboseMode);
int linearinterpolation_bins(float* sinogram2D, float* normsino2D, int nelems, int nangles, int nplanes, bool verboseMode);
int bilinearinterpolation(float* sinogram2D, float* normsino2D, int nelems, int nangles, int nplanes, bool verboseMode);
int useestimateddata(float* sinogram, float* estimate, float* normsino, int nelems, int nangles, int nplanes, bool verboseMode);

#endif
