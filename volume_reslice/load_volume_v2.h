# pragma once

#include "ecatx/matrix.h"
class CProgressCtrl;
//static char sccsid[] = "@(#)load_volume_v2.h	1.3 UCL-TOPO 00/05/29"
ecat_matrix::MatrixData *load_volume_v2(/*CProgressCtrl *progress_ctrl, */ecat_matrix::MatrixFile *matrix_file,
						   int matnum, float pixel_size, int interp);
