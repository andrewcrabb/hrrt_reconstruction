#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef FILENAME_MAX /* SunOs 4.1.3 */
#define FILENAME_MAX 256
#endif
#include "ecat_matrix.hpp"

void matrix_sum(MatrixData *matrix, MatrixData **sum)
{
  struct Matval mat;
  int i, nvoxels, nblks;
  short *sdata;
  float *fdata;
  
  nvoxels =  matrix->xdim*matrix->ydim*matrix->zdim;
  mat_numdoc(matrix->matnum, &mat);
  sdata = (short*)matrix->data_ptr;
  if (*sum == NULL)
  {
    *sum = (MatrixData*)calloc(1, sizeof(MatrixData));
    memcpy(*sum, matrix,sizeof(MatrixData));
    (*sum)->data_size = nvoxels*sizeof(float);
    nblks = ((*sum)->data_size + MatBLKSIZE-1)/MatBLKSIZE;
    (*sum)->data_ptr = (void *)calloc(nblks,MatBLKSIZE);
    (*sum)->shptr =(void *)calloc(1,MatBLKSIZE);
    memcpy((*sum)->shptr,matrix->shptr,sizeof(Image_subheader));
    fdata = (float*)(*sum)->data_ptr;
    for (i=0; i<nvoxels; i++)
      fdata[i] = sdata[i]*matrix->scale_factor;
  }
  else 
  {
    fdata = (float*)(*sum)->data_ptr;
    for (i=0; i<nvoxels; i++)
      fdata[i] += sdata[i]*matrix->scale_factor;
  }
}

int main(int argc, char **argv)
{
  MatDirNode *node;
  MatrixFile *mptr, *sum_mptr=NULL;
  MatrixData *matrix, *sum=NULL;
  Image_subheader *imh;
  struct Matval mat;
  char fname[FILENAME_MAX], *ext=NULL;
  int i, ftype, frame = -1, matnum=0, cubic=0, interpolate=0;
  int start_frame=0, end_frame=0, last_frame=0;
  float *fdata=NULL, f;
  short *sdata;
  int nvoxels;
  int frame_count=0, duration=0;


#ifdef _DEBUG
  char dfname[FILENAME_MAX];
  FILE *fp=NULL;
#endif
  
  if (argc < 2) crash("usage : %s file start_frame,end_frame\n",argv[0]);
  if (argc>2) sscanf(argv[2], "%d,%d",&start_frame,&end_frame);
  mptr = matrix_open(argv[1], MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
  if (mptr == NULL) {
    matrix_perror(argv[1]);
    return 0;
  }
  strcpy(fname, argv[1]);
  if ((ext=strrchr(fname,'.')) != NULL)*ext = '\0';
  ftype = mptr->mhptr->file_type;
  // if (ftype <0 || ftype >= NumDataSetTypes)
  //   crash("%s : unkown file type\n",argv[1]);
  printf( "%s file type  : %s\n", argv[1], datasettype_.at(ftype).name);
  if (!mptr) matrix_perror(fname);
  node = mptr->dirlist->first;
  while (node)
  {
    mat_numdoc(node->matnum, &mat);
    if ((start_frame==0 || mat.frame>=start_frame) &&
      (end_frame==0 || mat.frame<=end_frame))
    {
      matrix = matrix_read(mptr,node->matnum,UnknownMatDataType);
      if (!matrix) crash("%d,%d,%d,%d,%d not found\n",
        mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
      printf("Adding %d,%d,%d,%d,%d\n",mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
      matrix_sum(matrix, &sum);
      imh = (Image_subheader*)sum->shptr;
      duration += imh->frame_duration;
      frame_count++;
      free_matrix_data(matrix);
      last_frame = mat.frame;
#ifdef _DEBUG
      nvoxels =  matrix->xdim*matrix->ydim*matrix->zdim;
      imh = (Image_subheader*)matrix->shptr;
      printf("%d,%d,%d\n",mat.frame,imh->frame_start_time/1000, imh->frame_duration/1000);
      sprintf(dfname,"%s_frame%d.i", fname, mat.frame);
      if ((fp=fopen(dfname,"wb")) != NULL)  {
        if (fdata == NULL) fdata = (float*) calloc(nvoxels, sizeof(float));
        sdata = (short*)matrix->data_ptr;
        for (i=0; i<nvoxels; i++) fdata[i] = sdata[i];
        fwrite(fdata, nvoxels, sizeof(float), fp);
        fclose(fp);
      }
#endif
    } 
    else
    {
      printf("Skipping %d,%d,%d,%d,%d\n",mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
    }
    node = node->next;
  }
  if (frame_count<2) return 0;
  if (start_frame==0 && end_frame==0) strcat(fname,"_sum.v");
  else sprintf(&fname[strlen(fname)],"_sum_%dto%d.v", start_frame, last_frame);
  if ((sum_mptr=matrix_create(fname,MAT_OPEN_EXISTING, mptr->mhptr)) != NULL)
  {
    nvoxels =  matrix->xdim*matrix->ydim*matrix->zdim;
    fdata = (float*)sum->data_ptr;
    sdata = (short*)sum->data_ptr;
    for (i=0; i<nvoxels; i++) fdata[i] /= frame_count;
    sum->data_min = find_fmin(fdata, nvoxels);
    sum->data_max = find_fmax(fdata, nvoxels);
    if (fabs(sum->data_min) < fabs(sum->data_max))
      sum->scale_factor = (float)fabs(sum->data_max)/32766;
    else sum->scale_factor = (float)fabs(sum->data_min)/32766;
    f = 1.0f/sum->scale_factor;
    for (i=0; i<nvoxels; i++) sdata[i] = (short)(fdata[i]*f);
    sum->data_size = nvoxels*sizeof(short);
    imh = (Image_subheader*)sum->shptr;
    imh->frame_duration = duration;
    imh->image_min = find_smin(sdata, nvoxels);
    imh->image_max = find_smax(sdata, nvoxels);
    imh->scale_factor = sum->scale_factor;
    matrix_write(sum_mptr, mat_numcod(1,1,1,0,0), sum);
    matrix_close(sum_mptr);
  }
  matrix_close(mptr);
  return 1;
}
