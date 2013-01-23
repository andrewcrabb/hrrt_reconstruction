#ifndef load_volume_v2_h
#define load_volume_v2_h
#include "ecatx/matrix.h"
class CProgressCtrl;
//static char sccsid[] = "@(#)load_volume_v2.h	1.3 UCL-TOPO 00/05/29"
MatrixData *load_volume_v2(/*CProgressCtrl *progress_ctrl, */MatrixFile *matrix_file,
						   int matnum, float pixel_size, int interp);
#endif
