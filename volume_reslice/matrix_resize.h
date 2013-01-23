#ifndef matrix_resize_h
#define matrix_resize_h
#include <ecatx/matrix.h>
int matrix_resize(MatrixData *mat, float pixel_size, int interp_flag,
	int align_flag=1);
// align x to 4 bytes boundary when align_flag set
// int matrix_flip(MatrixData *data, int x_flip, int y_flip, int z_flip);
#endif /* matrix_resize_h */
