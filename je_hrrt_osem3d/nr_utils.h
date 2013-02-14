#ifndef nr_utils_h
#define nr_utils_h

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <xmmintrin.h>
 
#define	NR_END 1
#define	FREE_ARG char*

//typedef struct __declspec(intrin_type) __declspec(align(16)) __m128 {
//   float m128_f32[4];
//} __m128;

void 	nrerror(char error_text[]);
float	*vector(int nl, int nh);
float   **matrixfloat(int nrl, int nrh, int ncl, int nch);
__m128  **matrixm128(int nrl, int nrh, int ncl, int nch);
char    ***matrix3dchar(int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
float   ***matrix3d(int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
__m128  ***matrix3dm128(int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
__m128	***matrix3dm128_check(int nrl, int nrh, int ncl, int nch, int ndl, int ndh);

short int  **matrix_i( int nrl, int nrh, int ncl, int nch  );
void	free_vector(float *v, int nl, int nh);
void	free_matrix(float **m, int nrl, int nrh, int ncl, int nch);
void	free_matrix3d(float ***t, int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
void	free_matrix3dchar(char ***t, int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
void	free_matrix3dm128(float ***t, int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
void	free_matrix3dshortm128(short ***t, int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
void	free_matrix_i(short int **m, int nrl, int nrh, int ncl, int nch);
void	free_matrixm128(float **m, int nrl, int nrh, int ncl, int nch);
void	free_matrix3dshort(short ***ptr, int nrl, int nrh, int ncl, int nch, int ndl, int ndh);
#endif /* nr_utils_h */
