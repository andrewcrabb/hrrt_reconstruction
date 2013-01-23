/*
 QMatrix.h : Quaternion to Matrix Conversion
Authors:
  Merence Sibomana
  Sune H. Keller
  Oline V. Olesen
Creation date: 31-mar-2010
*/
#ifndef QMatrix_h
#define QMatrix_h
#include <stdio.h>
#include <ecatx/matpkg.h>
extern int q2matrix(const float *q, const float *t, Matrix m);
extern int vicra_align(Matrix ref, Matrix in, Matrix out);
extern int vicra_align(const float *ref_q, const float *ref_t, 
                       const float *in_q, const float *in_t, Matrix out);
// Read vicra to scanner calibration or set it to identify (NULL argument)
// Default calibration is from Copenhagen Rigshopitalet HRRT scanner
extern int vicra_calibration(const char *filename, FILE *log_fp);
#endif