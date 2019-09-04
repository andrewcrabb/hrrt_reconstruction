/*
File: motion_correct_pp.cpp
Authors:
        Merence Sibomana
        Sune H. Keller
        Marc C. Huisman
        REM Conversion to ECAT

Purpose: 
        1- Convert reconstructed images to quantified ECAT,
        2- Smooth reconstructed frame image.
        3- Choose a reference frame (e.g first with given NEC) or the sum of first
        frames with given NEC 
        4- Use AIR to align all frames or selected frames to  reference.
        5- Use gnuplot to create a postcript plot showing motion of an arbitary
         point (x,y,z)=(0,100,125)mm

Usage:
      motion_qc -e em_dyn_file  -n norm [-F start_frame[,end_frame]]
                [-r ref_frame -R ref_frame_NEC -S ref_frame_sum_NEC] 
                [-x recon_pgm] [-A alignment_method] [-t]              

      - Default reference is sum of first frames with NEC=4.0M (-R 4.0)
      - Default start_frame is first frame with NEC=0.2M
      - Default end_frame is last frame with NEC=0.2M 
      - Default recon_pgm "hrrt_osem3d"
      - Default alignment method "AIR"
      -t : test only

Example: 36 frames scan with frame 16 as reference frame using hrrt_osem3d_v1.2_x64 test program
      motion_qc -e HAL-2009.12.03.12.02.59_em.dyn -n Norm_2009-09-05_CBN_span9.n -r 16 -x hrrt_osem3d_v1.2_x64

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "frame_info.h"

static char cmd_line[2048];

static void usage(const char *pgm)
{
  printf("%s Build %s %s \n", pgm, __DATE__, __TIME__);
  printf("usage: %s -i em_dyn_file  [-r ref_frame -R ref_frame_NEC -S ref_frame_sum_NEC] "
                 "[-e time_constant] [-u units] [-S calibration_dir] [options]\n", pgm);
  printf("     -e time_constant (default from image header, 9.34e-6 if not in header]\n");
  printf("     -u units (default Bq/ml)\n");
  printf("     -S calibration_dir (default C:\\CPS\\calibration_files)\n");
  printf("     -t test only, prints commands in motion_correct_pp.log\n");
  printf("     -v verbose (default off)\n");
  exit(1);
}


int main(int argc, char **argv)
{
  const char *recon_pgm="hrrt_osem3d";
  const char *align_pgm="AIR";
  const char *log_file = "motion_correct_pp.log";
  const char *units = "Bq/ml";
  char line[80];
  const char *calib_dir = NULL;
  char *em_dyn_file=NULL, *ext=NULL;
  char em_prefix[FILENAME_MAX], fname[FILENAME_MAX];
  char plt_fname[FILENAME_MAX];
  float ref_frame_NEC=4.0E6f, sum_frame_NEC=0.0f, min_frame_NEC=2.0E5f;
  int ref_frame=-1;
  int frame=0, start_frame=-1, end_frame=-1, num_frames=0;
  int s=6; // gaussian smoothing in mm
  int c;
  FILE *fp=NULL, *pp=NULL, *log_fp=NULL;
  int exec=1, verbose = 0;
  float user_time_constant = 0.0f;
  int overwrite=0;

  while ((c = getopt (argc, argv, "i:e:u:r:R:S:C:Otv")) != EOF) {
    switch (c) {
      case 'i':
        em_dyn_file = optarg;
        break;
      case 'e' :
        sscanf(optarg,"%g",&user_time_constant);
        break;
      case 'u':
        units = optarg;
        break;
      case 'O':
        overwrite = 1;
        break;
      case 'r':
        if (sscanf(optarg,"%d",&ref_frame) != 1) {
          printf("Invalid reference frame number : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'R':
        if (sscanf(optarg,"%g",&ref_frame_NEC) != 1) {
          printf("Invalid reference frame NEC : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'S':
        if (sscanf(optarg,"%g",&sum_frame_NEC) != 1) {
          printf("Invalid reference frame NEC : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'C':
        calib_dir = optarg;
        break;
      case 't' :
        exec = 0;
        break;
      case 'v' :
        verbose = 1;
        break;
    }
  }
  if (em_dyn_file==NULL ) usage(argv[0]);
  if ((log_fp=fopen(log_file,"wt")) == NULL) {
    perror(log_file);
    return 1;
  }
  strcpy(em_prefix,em_dyn_file);
  if ((ext=strstr(em_prefix,".dyn")) == NULL) {
    fprintf(log_fp, "Invalid em_dyn_file %s\n", em_dyn_file);
    usage(argv[0]);
  }
  *ext = '\0';
  if ((fp=fopen(em_dyn_file,"rt")) == NULL) {
    fprintf(log_fp,"Error opening file %s\n", em_dyn_file);
    return 1;
  }
  fgets(line,sizeof(line), fp);
  sscanf(line,"%d", &num_frames);
  fclose(fp);
  for (frame=0; frame<num_frames; frame++) {
    sprintf(fname,"%s_frame%d.s.hdr",em_prefix,frame);
    get_frame_info(frame, fname,num_frames);
  }
  if (start_frame<0) 
    for (frame=0; frame<num_frames && start_frame<0; frame++) {
      if (frame_info[frame].NEC > min_frame_NEC)
        start_frame = frame+1; // ECAT 1-N counting
    }
  if (end_frame<0) end_frame=num_frames;

  if (ref_frame<0) {
    if (sum_frame_NEC>0) {
      double sum=0.0;
      for (frame=0; frame<num_frames && sum<sum_frame_NEC; frame++)
        sum += frame_info[frame].NEC;
      ref_frame = frame+1; // ECAT 1-N counting
    } else {
      for (frame=0; frame<num_frames && ref_frame<0; frame++) {
        if (frame_info[frame].NEC > ref_frame_NEC) {
          ref_frame = frame+1; // ECAT 1-N counting
        }
      }
    }
  }

  if (user_time_constant <= 0.0f) user_time_constant = deadtime_constant;
 // hrrt_osem3d_v1.2_x64 -t %em%.dyn -n %no% -X 128 -o %em%_na.v -W 0 -I 4 -S 8 -N -L osem3d.log
  sprintf(cmd_line,"if2e7 -v -e %g %em%_frame%d.i", 
    user_time_constant, em_prefix, ref_fame);
  sprintf(fname,"%s.v",em_prefix);
  if (access(fname,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing existing %s\n",fname);
  } else {
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) system(cmd_line);
  }

  // gsmooth_u  %em%_na.v 6
  sprintf(cmd_line, "gsmooth_u %s.v %d", em_prefix, s);
  sprintf(fname,"%s_%dmm.v",em_prefix,s);
  if (access(fname,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing existing %s\n",fname);
  } else {
    fprintf(log_fp,"%s\n",cmd_line);
   if (exec) system(cmd_line);
  }

  // Copy frames
  for (frame=1; frame<start_frame; frame++) {
    sprintf(cmd_line,"matcopy -i %s.v,%d,1,1 -o %s_rsl.v,%d,1,1",
      em_prefix, frame, em_prefix, frame);
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) system(cmd_line);
  }
 // AIR alignment
  for (frame=start_frame; frame<=end_frame; frame++) {
    if (frame == ref_frame) { // create unity transformer
      //ecat_make_air -s %s_%dmm.v,%d,1,1 -r %s_%dmm.v,%d,1,1 -t 0,0,0,0,0,0 -o %s_na_fr%d.air
      sprintf(cmd_line,"ecat_make_air -s %s_%dmm.v,%d,1,1 -r %s_%dmm.v,%d,1,1 -t 0,0,0,0,0,0 -o %s_fr%d.air",
         em_prefix, s, frame, em_prefix, s, frame, em_prefix, frame);
    } else {
      // ecat_alignlinear  %im_smo%.v,%ref%,1,1 %im_smo%.v,%f%,1,1 %em%_na_fr%f%.air -m 6 
      sprintf(cmd_line,"ecat_alignlinear %s_%dmm.v,%d,1,1 %s_%dmm.v,%d,1,1 %s_fr%d.air -m 6",
        em_prefix, s, ref_frame, em_prefix, s, frame, em_prefix, frame);
    }
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) system(cmd_line);
    //  ecat_reslice %em%_fr%f%.air %em%_rsl.v,%f%,1,1 -a %em%.v,%f%,1,1 -k -o
    if (frame == ref_frame) {
      sprintf(cmd_line,"matcopy -i %s.v,%d,1,1 -o %s_rsl.v,%d,1,1",
        em_prefix, frame, em_prefix, frame);
    } else {
      sprintf(cmd_line,"ecat_reslice %s_fr%d.air %s_rsl.v,%d,1,1 -a %s.v,%d,1,1 -k -o",
        em_prefix, frame, em_prefix, frame, em_prefix, frame);
    }
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) system(cmd_line);
  }

  //Create Motion QC plot
  sprintf(fname,"%s_motion_qc.dat", em_prefix);
  if ((fp=fopen(fname,"wt")) == NULL) {
    fprintf(log_fp,"Error opening file %s\n", fname);
    return 1;
  }

  for (frame=start_frame; frame<=end_frame; frame++) {
    sprintf(cmd_line,"motion_distance -a %s_fr%d.air", em_prefix, frame);
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) {
      if ((pp = popen(cmd_line,"rt")) != NULL) {
        fgets(line, sizeof(line),pp);
        fprintf(fp,"%d %s",frame_info[frame-1].start_time, line);
        pclose(pp);
      } else {
        fprintf(log_fp,"Error opening pipe %s\n", cmd_line);
        return 1;
      }
    }
  }
  fclose(fp);
  sprintf(plt_fname,"%s_motion_qc.plt", em_prefix);
  if ((fp=fopen(plt_fname,"wt")) != NULL) {
    fprintf(fp,"set terminal postscript portrait color 'Helvetica' 8\n");
    fprintf(fp,"set output  '%s_motion_qc.ps'\n", em_prefix);
    fprintf(fp,"set multiplot\n");
    fprintf(fp,"set size 1.0,0.45\n");
    fprintf(fp,"set title '%s Motion QC'\n",em_prefix);
    fprintf(fp,"set origin 0.,0.55\n");
    fprintf(fp,"set grid\n");
    fprintf(fp,"plot '%s' using 1:3 title 'TX' with lines,\\\n", fname);
    fprintf(fp,"     '%s' using 1:4 title 'TY' with lines,\\\n", fname);
    fprintf(fp,"     '%s' using 1:5 title 'TZ' with lines,\\\n", fname);
    fprintf(fp,"     '%s' using 1:6 title 'D' with lines\n", fname);
    fclose(fp);
  }
  sprintf(cmd_line,"pgnuplot %s", plt_fname);
  fprintf(log_fp,"%s\n",cmd_line);
  if (exec) system(cmd_line);
  fclose(log_fp);
  return 0;
}