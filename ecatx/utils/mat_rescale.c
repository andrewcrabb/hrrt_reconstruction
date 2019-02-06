/* mat_rescale.c

  Purpose
          Prepare frames for AIR realignment: rescales all frames so that
          the volume of pixels above the threshold (default=7000)is close 
          to the volume of pixels 0.03 in the mu-map;

  Creation date: 2010-feb-16

  Author: Merence Sibomana <sibomana@gmail.com>
         
  Modification history:
*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef FILENAME_MAX 
#define FILENAME_MAX 256
#endif
#include "ecat_matrix.hpp"
#define HISTOGRAM_SIZE 32768
#define IMAGE_MAX 32766

void matrix_sum(MatrixData *matrix, MatrixData **sum)
{
  ecat_matrix::MatVal mat;
  int i, nvoxels, nblks;
  short *sdata;
  float *fdata;
  
  nvoxels =  matrix->xdim*matrix->ydim*matrix->zdim;
  mat_numdoc(matrix->matnum, &mat);
  printf("%d,%d,%d,%d,%d\n",
	 mat.frame, mat.plane, mat.gate, mat.data, mat.bed);

  sdata = (short*)matrix->data_ptr;
  if (*sum == NULL)
  {
    *sum = (MatrixData*)calloc(1, sizeof(MatrixData));
    memcpy(*sum, matrix,sizeof(MatrixData));
    (*sum)->data_size = nvoxels*sizeof(float);
    nblks = ((*sum)->data_size + MatBLKSIZE-1)/MatBLKSIZE;
    (*sum)->data_ptr = (void *)calloc(nblks,MatBLKSIZE);
    (*sum)->shptr =(void *)calloc(1,MatBLKSIZE);
    memcpy((*sum)->shptr,matrix->shptr,sizeof(ecat_matrix::Image_subheader));
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
void usage (const char *pgm)
{
  printf("%s: Build %s %s\n", pgm, __DATE__, __TIME__);
  printf("purpose : Rescale frames for realignment with AIR software\n\n");
  printf("usage : %s -i in_file -o out_file -u mu_map [-t thr] [-T mu_thr]\n",pgm);
  printf("        Rescales all frames so that the volume of pixels above the \n"
         "        threshold in the frame is close to the volume of pixels above\n"
         "        threshold in the mu-map \n");
  printf("-t thr  Specify threshold in unscaled units (default=7000)\n");
  printf("-T mu_thr  Specify mu-threshold in scaled units(default=0.03)\n");
  exit(1);
}

int main(int argc, char **argv)
{
  MatDirNode *node;
  ecat_matrix::MatrixFile *mptr1=NULL, *mptr2=NULL;
  MatrixData *matrix;
  ecat_matrix::Image_subheader *imh;
  ecat_matrix::MatVal mat;
  char *in_file=NULL, *mu_file=NULL, *out_file=NULL;
  int ftype, frame = -1, matnum=0, start_frame=0, end_frame=0;
  int i, nvoxels;
  float f, new_scale;
  short *sdata, s;
  short *histogram=NULL;
  int count=0, em_thres= 7000;
  float mu_thres = 0.03f;
  float mu_vol_mm3; //mu volume in mm3
  int mu_vol=0, em_vol=0;       //mu volume in emission pixels
  int verbose=0, zeroes=0;
  double avg=0;

  while ((i = getopt (argc, argv, "i:o:u:t:T:vz")) != EOF) {
    switch (i) {
      case 't' :
        if (sscanf(optarg,"%d",&em_thres) != 1){
          printf("Invalid emission threshold %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'T' :
        if (sscanf(optarg,"%f",&mu_thres) != 1){
          printf("Invalid mu threshold %s\n", optarg);
          usage(argv[0]);
        }
        if (mu_thres<0.01f || mu_thres>0.096f) {
          printf("Invalid mu threshold %s: valid value range [0.01, 0.096]\n",
            optarg);
        }
        break;
      case 'i' :
        in_file = optarg;
        break;
      case 'o' :
        out_file = optarg;
        break;
      case 'u' :
        mu_file = optarg;
        break;
      case 'v' :
        verbose = 1;
        break;
      case 'z' :
        zeroes = 1;
        break;
    }
  }

  if (in_file==NULL || out_file==NULL || mu_file == NULL) usage(argv[0]);

  mptr1 = matrix_open(in_file, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (mptr1 == NULL) {
    matrix_perror(in_file);
    return 1;
  }
  ftype = mptr1->mhptr->file_type;
  // if (ftype <0 || ftype >= NumDataSetTypes)
  //   crash("%s : unkown file type\n", in_file);
  printf( "%s file type  : %s\n", in_file, data_set_types_.at(ftype).name);

  mptr2 = matrix_open(mu_file, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (mptr2 == NULL) {
    matrix_perror(mu_file);
    return 1;
  }
  
  // read mu-map and compute object volume
  if ((node = mptr2->dirlist->first) == NULL) matrix=NULL;
  else matrix = matrix_read(mptr2,node->matnum,UnknownMatDataType);
  if (matrix==NULL) {
    printf("error reading mu-map\n");
    return 1;
  }
  mu_vol=0;
  sdata = (short*)matrix->data_ptr;
  s = (short)(0.5f + mu_thres/matrix->scale_factor);
  nvoxels = matrix->xdim*matrix->ydim* matrix->zdim;
  for (i=0; i<nvoxels; i++)
    if (sdata[i]>=s)
    {
      mu_vol++;
    }
  mu_vol_mm3 = mu_vol*(matrix->pixel_size*matrix->y_size*matrix->z_size);
  if (verbose) printf("Mu-map volume %d pixels %g ml\n", mu_vol*4, mu_vol_mm3);
  free_matrix_data(matrix);
  matrix_close(mptr2);

  mptr2 = matrix_create(out_file,ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, mptr1->mhptr);
  if (mptr2 == NULL) {
    matrix_perror(out_file);
    return 1;
  }

  node = mptr1->dirlist->first;

  histogram = (short*)calloc(HISTOGRAM_SIZE,sizeof(short));
  while (node) {
    mat_numdoc(node->matnum, &mat);
    matrix = matrix_read(mptr1,node->matnum,UnknownMatDataType);
    if (!matrix) crash("%d,%d,%d,%d,%d not found\n",
      mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
    if (verbose) printf("Reading %s,%d done\n", in_file, mat.frame);
    mu_vol = (int)(0.5f + mu_vol_mm3/
      (matrix->pixel_size*matrix->y_size*matrix->z_size));
    nvoxels = matrix->xdim*matrix->ydim* matrix->zdim;

    sdata = (short*)matrix->data_ptr;
    memset(histogram, 0, HISTOGRAM_SIZE*sizeof(short));
    for (i=0; i<nvoxels; i++)
      if (sdata[i]>0) histogram[sdata[i]] += 1;

    count=em_vol=0;
    avg = 0;
    for (i=HISTOGRAM_SIZE-1; i>=0 && em_vol<mu_vol; i--) {
      em_vol += histogram[i];
      avg += i*histogram[i];
      count += histogram[i];
    }
    avg /= count;

    // new_scale = (float)(avg*matrix->scale_factor/em_thres);
    // Just make sure that max short data is same in all ftrames
    new_scale = (float)(matrix->data_max/matrix->scale_factor/IMAGE_MAX);
    if (verbose) printf("frame %d volume %d ithres %d scale %g newscale %g\n",
      mat.frame, em_vol, i,matrix->scale_factor,new_scale);
    for (i=0; i<nvoxels; i++) {
      f = sdata[i]*matrix->scale_factor/new_scale;
      if (f>0.0f) {
        if (f>32766.0f) sdata[i] = IMAGE_MAX; // truncate hot pixels
        else  sdata[i] = (short)(0.5f+f);
      } else {
        if (f < -IMAGE_MAX) sdata[i] = -IMAGE_MAX; // truncate cold pixels
        else  sdata[i] = (short)(-0.5f+f);
      }
      if (zeroes)
        if (sdata[i]<em_thres) sdata[i]=0;
    } 
    matrix->scale_factor = 1.0f; //new_scale;
    imh = (ecat_matrix::Image_subheader*)matrix->shptr;
    imh->image_min = find_smin(sdata, nvoxels);
    imh->image_max = find_smax(sdata, nvoxels);
    imh->scale_factor =  1.0f; //new_scale;
    matrix_write(mptr2, node->matnum, matrix);
    if (verbose) printf("Writing %s,%d done\n", out_file, mat.frame);
    node = node->next;
  }
  matrix_close(mptr1);
  matrix_close(mptr2);
  return 1;
}
