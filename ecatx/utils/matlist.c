/* @(#)matlist.c  1.2 7/10/92 */
/*
 * Updated by Sibomana@topo.ucl.ac.be for ECAT 7.0 support
 */

#include "ecat_matrix.hpp"

int main( argc, argv) {
  ecat_matrix::MatVal mat;
  char cbufr[256];

  if (argc < 2)
    crash("usage : matlist matrix_file\n");
  ecat_matrix::MatrixFile *mptr = matrix_open( argv[1], ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (!mptr) {
    matrix_perror(argv[1]);
    exit(1);
  }
  printf("file '%s' is of type %d with %d matrices\n", argv[1],  mptr->mhptr->file_type, mptr->dirlist->nmats);
  MatDirNode *node = mptr->dirlist->first;
  while (node) {
    mat_numdoc( node->matnum, &mat);
    sprintf( cbufr, "%d,%d,%d,%d,%d",  mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
    printf("%8.8x %-12s %10d %10d\n", node->matnum, cbufr, node->strtblk, node->endblk);
    node = node->next;
  }
  matrix_close(mptr);
  return (0);
}
