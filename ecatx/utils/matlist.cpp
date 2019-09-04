/* @(#)matlist.c  1.2 7/10/92 */
/*
 * Updated by Sibomana@topo.ucl.ac.be for ECAT 7.0 support
 */

#include "ecat_matrix.hpp"
#include "my_spdlog.hpp"

int main( argc, argv) {
  ecat_matrix::MatVal mat;
  char cbufr[256];

  if (argc < 2)
    LOG_EXIT("usage : matlist matrix_file");
  ecat_matrix::MatrixFile *mptr = matrix_open( argv[1], ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (!mptr) {
    LOG_EXIT(argv[1]);
    exit(1);
  }
  LOG_INFO("file {} is of type {} with {} matrices", argv[1],  mptr->mhptr->file_type, mptr->dirlist->nmats);
  ecat_matrix::MatDirNode *node = mptr->dirlist->first;
  while (node) {
    mat_numdoc( node->matnum, &mat);
    sprintf( cbufr, "%d,%d,%d,%d,%d",  mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
    LOG_INFO("{:8.8x} {:-12s} {:10d} {:10d}", node->matnum, cbufr, node->strtblk, node->endblk);
    node = node->next;
  }
  matrix_close(mptr);
  return (0);
}
