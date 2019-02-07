# pragma once

#include <ecatx/ecat_matrix.hpp>
int matrix_resize(ecat_matrix::MatrixData *mat, float pixel_size, int interp_flag,
	int align_flag=1);
// align x to 4 bytes boundary when align_flag set
// int matrix_flip(ecat_matrix::MatrixData *data, int x_flip, int y_flip, int z_flip);
