// MAF_join.cpp : Defines the entry point for the console application.
//
/*[ MAF_join
------------------------------------------------

   Component       : <Add your component name here>

   Name            : MAF_join.cpp - Join sub-frames created by MAF motion correction method
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 15/04/2010
   Modification history:

   Description     :  Read MAF description file, cop.
					  Supports ECAT7 and Interfile formats

---------------------------------------------------------------------*/

/* ahc */
#define unix 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "ecatx/matrix.h"
#ifndef unix
/* #include <io.h> */
/* #include <direct.h> */
#include <getopt.h>
#define access _access
#define my_mkdir _mkdir
#define F_OK 0x0
#define W_OK 0x2
#define R_OK 0x4
#define RW_OK 0x6
#define SEPARATOR '\\'
#define strcasecmp stricmp
#define strncasecmp stricmp
#else
#define SEPARATOR '/'
#include <unistd.h>
#endif
#include "frame_info.h"

#define MIN_SUB_FRAME_WEIGHT 0.1f // exclude subframes with duration less than 10% of total frame
#define MIN_SUB_FRAME_DURATION 40 // keep subframes with duration longer than 40sec 
inline int keep_sub_frame(int sf_duration, int f_duration)
{
  // Keep all frames, to be replaced by LOR repositioning in lmhistogram
  return 1;
  float w = (float)sf_duration/(float)f_duration;
  if (w>MIN_SUB_FRAME_WEIGHT || sf_duration>MIN_SUB_FRAME_DURATION) return 1;
  else return 0;
}

static void usage(const char *pgm)
{
    fprintf(stderr, "\nMAF_join Build %s %s\n\n",__DATE__,__TIME__);
		fprintf(stderr, "Usage: MAF_Join input_ecat_file -o output_ecat_file -M Vicra_MAF_file [-l log_file]\n");
		fprintf(stderr, "        Read MAF file, and copy single frames or join sub-frames \n");
		fprintf(stderr, "        weighted by sub-frame durations, excluding frames of weight<10%%\n");
		fprintf(stderr, "        Default log file (MAF_join.log) \n");
		exit(1);
}

int main(int argc, char **argv)
{
  char txt[80];
  int c, frame, verbose=0;
  int frame_duration=0;
	char *in_fname=NULL, *out_fname=NULL, *MAF_file=NULL;
	MatrixFile *in = NULL, *out=NULL;
  Main_header proto;
	char *p = NULL;
  const char *log_file = "MAF_join.log";
  std::vector<int> sub_frames;
  MatrixData *mat=NULL;
  Image_subheader *imh=NULL;
  float *fdata=NULL, w=1.0f, fmin=0.0f, fmax=0.0f;
  short *sdata = NULL;
  unsigned i=0,j=1;
  int k=0, npixels=0, nvoxels=0;
  FILE *log_fp=NULL;

	if (argc<2) usage(argv[0]);
  in_fname = argv[1];
  while ((c = getopt (argc-1, argv+1, "M:o:l:v")) != EOF) {
    switch(c) {
      case 'M': MAF_file=optarg; break;
      case 'o': out_fname = optarg; break;
      case 'l': log_file = optarg; break;
      case 'v': verbose = 1; break;
    }
  }
	if (MAF_file==NULL || out_fname==NULL) usage(argv[0]);
  if ((log_fp=fopen(log_file,"wt")) == NULL) {
    fprintf(stderr, "Error opening %s, log on stderr\n", log_file);
    log_fp = stderr;
  }
  if (vicra_get_info(MAF_file, log_fp) == 0) exit(1);
  
  if ((in = matrix_open(in_fname,ecat_matrix::MatrixFileAccessMode::READ_ONLY,ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE)) == NULL) {
    fprintf(log_fp, "Error opening %s\n", in_fname);
    exit(1);
  }

  if (in->dirlist->nmats != vicra_info.em.size() ) {
    fprintf(log_fp, "Error: number of frames in %s and %s are different; %d:%lu\n", 
      in_fname, MAF_file, in->dirlist->nmats, vicra_info.em.size());
    exit(1);
  }
  memcpy(&proto, in->mhptr, sizeof(Main_header));
  if ((out = matrix_create(out_fname, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, &proto)) ==NULL) {
    fprintf(log_fp,"Error creating %s\n", out_fname);
    exit(1);
  }

  while (i<vicra_info.em.size()) {
    frame_duration =  vicra_info.em[i].dt;
    frame = vicra_info.em[i].fr;
    while (i+j < vicra_info.em.size()) {
      if (vicra_info.em[i+j].fr != vicra_info.em[i].fr) break;
      sub_frames.push_back(i+j);
      frame_duration += vicra_info.em[i+j].dt;
      j++;
    }
    if ((mat=matrix_read(in, mat_numcod(i+1,1,1,0,0), GENERIC)) == NULL) {
      fprintf(log_fp,"Error reading %s frame %u\n", in_fname, i+1);
      exit(1);
    }
    if (sub_frames.size() == 0) { // single frame
      fprintf(log_fp,"copy frame %d\n", frame);
      if (matrix_write(out, mat_numcod(frame+1,1,1,0,0), mat) != 0) {
			  fprintf(log_fp,"Error writing %s frame %d\n", out_fname, frame);
        exit(1);
      }
    } else {
      float wt= 0.0f; // total weight excluding very short subframes (w<0.1)
      int eflag=0;
      npixels = mat->xdim*mat->ydim;
      nvoxels = npixels*mat->zdim;
      sdata = (short*)mat->data_ptr;
      if (fdata == NULL) fdata = (float*)calloc(nvoxels, sizeof(float));
      else memset(fdata,0,nvoxels*sizeof(float));
      sprintf(txt,"join %d(%d/%d)", i, vicra_info.em[i].dt,frame_duration);
      for (j=1; j<=sub_frames.size(); j++) {
        sprintf(txt+strlen(txt),",%d(%d/%d)", sub_frames[j-1],
          vicra_info.em[i+j].dt,frame_duration);
      }
      fprintf(log_fp, "%s %d\n", txt, frame);
      if (keep_sub_frame(vicra_info.em[i].dt, frame_duration)) {
        w = mat->scale_factor*vicra_info.em[i].dt/frame_duration;
        for (k=0; k<nvoxels; k++) fdata[k] = w*sdata[k];
        wt += (float)vicra_info.em[i].dt/frame_duration;;
      } else {
        eflag++;
        fprintf(log_fp,"excluding subframe %d (%d/%d)\n", i, 
          vicra_info.em[i].dt,frame_duration);
      }
      for (j=1; j<=sub_frames.size(); j++) {
        if (keep_sub_frame(vicra_info.em[i+j].dt, frame_duration)) {
          MatrixData *smat =matrix_read(in, mat_numcod(i+j+1,1,1,0,0), GENERIC);
          if (smat == NULL) {
            fprintf(log_fp, "Error reading %s frame %u\n", in_fname, i+j+1);
            exit(1);
          }
          w = smat->scale_factor*vicra_info.em[i+j].dt/frame_duration;
          sdata = (short*)smat->data_ptr;
          for (k=0; k<nvoxels; k++) fdata[k] += w*sdata[k];
          free_matrix_data(smat);
          wt += (float)vicra_info.em[i+j].dt/frame_duration;
        } else {
          eflag++;
          fprintf(log_fp,"excluding subframe %d (%d/%d)\n", i+j, 
          vicra_info.em[i+j].dt, frame_duration);
        }
      }
      if (eflag) {
        if (wt<MIN_SUB_FRAME_WEIGHT) { 
          fprintf(log_fp, "Error frame %d: no valid sub frame, total weight=%g\n", 
            frame+1, wt);
          exit(1);
        }
        w = 1.0f/wt;
        for (k=0; k<nvoxels; k++) fdata[k] = w*fdata[k];
      }

      mat->data_min = find_fmin(fdata, nvoxels);
      mat->data_max = find_fmax(fdata, nvoxels);
      if (fabs(mat->data_max)> fabs(mat->data_min)) {
        mat->scale_factor = fabs(mat->data_max)/32766.0f; // stay away of 32767 limit
      } else { 
        mat->scale_factor = fabs(mat->data_min)/32766.0f; // stay away of limit
      }
      if (mat->scale_factor == 0.0f) mat->scale_factor = 1.0f;
      w = 1.0f/mat->scale_factor;
      sdata = (short*)mat->data_ptr;
      for (k=0; k<nvoxels; k++) sdata[k] = (int) (w*fdata[k]);
      imh = (Image_subheader*)mat->shptr;
      imh->image_max = (int)(w*mat->data_max);
      imh->image_min = (int)(w*mat->data_min);
      imh->scale_factor = mat->scale_factor;
      imh->frame_duration = frame_duration;

      if (matrix_write(out, mat_numcod(frame+1,1,1,0,0), mat) != 0) {
			  fprintf(log_fp, "Error writing %s frame %d\n", out_fname, frame+1);
        exit(1);
      }
    }
    i += sub_frames.size()+1;
    j = 1;
    sub_frames.clear();
    free_matrix_data(mat);
  }
  matrix_close(in);
  mh_update(out); // update number of frames
  matrix_close(out);
  exit(0);
  return 0;
}
