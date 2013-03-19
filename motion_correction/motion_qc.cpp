/*

  Arman:
  Added -R option; allowing ecat_reslice performance (default 1/one).
  
File: motion_qc.cpp
Authors:
  Merence Sibomana
  Sune H. Keller
  Oline V. Olesen

Purpose : Create a motion curve showing  the displacement of an arbitary
         point (x,y,z)=(0,-100,125)mm -- forhead point
  Method A: Registration software method
  1- Reconstruct HRRT multi-frame sinograms without attenuation correction,
  2- Smooth reconstructed frame image.
  3- Choose a reference frame (e.g first with given trues) 
  4- Use AIR software to align all frames or selected frames to  reference.
  5- Use gnuplot to create a postcript plot showing motion of an arbitary
         point (x,y,z)=(0,-100,125)mm -- forhead point

  Method B: Polaris vicra tracking method
  1- Choose a reference frame (e.g first with given trues) 
  2- Read timestamped tracking file (using vicra_file_ts program) to create
     average 4x4 transformer matrix (Oline V. Olesen et al, IEEE TNS 00572-2009)
     Vicra to Scanner Calibration parameters are saved in .vcal files located
     in specified directory (default c:\cps\users_sw)
  3- Use gnuplot to create a postcript plot showing motion of an arbitary
         point (x,y,z)=(0,-100,125)mm -- forhead point

Purpose A usage:
  motion_qc -e em_dyn_file  -n norm [-r ref_frame] [-O] [-t]              
´    - Default reference is first frame with 5min duration
     - Default start_frame is first frame with 30sec duration
     - Default end_frame is last frame
     -O: Overwrite existing images (default is reuse)
     -t: test only
  Example: 36 frames scan with frame 16 as reference frame 
  motion_qc -e HAL-2009.12.03.12.02.59_em.dyn -n Norm_2009-09-05_CBN_span9.n -r 16 

Purpose B usage:
  motion_qc -e em_dyn_file  -T trackfile [-r ref_frame] [-c calibration_dir]
     - Default reference is first frame 1
     - Default calibration directory c:\cps\users_sw\calibration (win32),
       $HOME/calibration (linux) 

 Creation date: 
      04-DEC-2009
 Modification dates:
      20-MAY-2010: Add scatter scaling for motion QC 
      10-AUG-2010: Use 0 starting numbers for .air file names 
      11-AUG-2010: Use 0 start for frame numbering everywhere for consistency
      21-AUG-2010: Add -T option for AIR alignment threshold
      23-NOV-2010: Use hrrt_osem3d_u for fast 128x128 reconstruction
                   Bug fix: default reference frame set to 0 if no 5min or more frame found
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frame_info.h"
#include <ecatx/matrix.h>
#include <unistd.h>
#define		DIR_SEPARATOR '/'

#include <vector>
// ahc
#include <iostream>
#include <sys/stat.h>

static char cmd_line[2048];
static int ref_frame=-1, start_frame=-1, end_frame=-1, num_frames=0;
static int exec=1, overwrite=0, verbose = 0;
static char em_prefix[FILENAME_MAX];
static char im_prefix[FILENAME_MAX];
static char mu_file[FILENAME_MAX];
static const char *norm_file=NULL;
static float lber=-1.0f;
static FILE *log_fp=NULL;
const char *athr=NULL;

double max_trues=0;
static int ecat_reslice_flag = 1;

// ahc program_path now a required argument.
char program_path[FILENAME_MAX];
char prog_gnuplot[FILENAME_MAX];
char dir_log[FILENAME_MAX];

// HRRT programs (all must be found in program_path)
static const char *prog_e7_fwd = "e7_fwd_u";
static const char *prog_e7_sino = "e7_sino_u";
static const char *prog_gsmooth = "gsmooth_ps";
static const char *prog_matcopy = "matcopy";
static const char *prog_alignlinear = "ecat_alignlinear";
static const char *prog_reslice = "ecat_reslice";
static const char *prog_lmhistogram = "lmhistogram_u";

static void usage(const char *pgm)
{
  printf("%s Build %s %s \n", pgm, __DATE__, __TIME__);
  printf("usage: %s em_file  -n norm [-u mu-map] [-F start_frame[,end_frame]]"
         "      [-r ref_frame  [-x recon_pgm] [options]\n", pgm);
  printf("       em_file.[l64|dyn|.v]: histogram if .l64, reconstruction 128x128 if .dyn \n");
  printf("Options: \n");
  printf("        -a e7_sino athr argument (default blank )\n");
  printf("        -O Overwrite exiting images (reuse images by default)\n");
  printf("        -g fwhm : FWHM in mm used for gaussian smoothing of alignment images\n");
  printf("        -T AIR_threshold (percent of max pixel) (default= 21.5)\n");
  printf("                  suggested values: FDG=36\n");
  printf("        -t test only, prints commands in motion_qc.log\n");
  printf("        -v verbose (default off)\n");
  printf("        -R ecat_reslice_flag (default 1 i.e. on)\n");
  printf("    *   -p program_path: Path to HRRT executables (required)\n");
  printf("    *   -z gnuplot: FQ path of gnuplot program (required)\n");
  printf("    *   -l logdir: Directory for log file\n");
  printf("       \nNB:  FRAMES NUMBERS STARTS from 0 **********\n");
  printf("       \nNB:  Only AIR alignment method implmented **\n");
  exit(1);
}

// Ensure directory is exists and can be written.
// Creates directory if it does not exist.
// Returns: 0 if exists on exit and can be written, else 1.
// This is a good candidate for the Boost::Filesystem library.

static int make_directory(char *path) {
  int ret;
  char buff[1000];
  
  // See if it exists.
  ret = access(path, W_OK);
  if (ret != 0)
    ret = mkdir(path, 0755);
  if (ret) {
    sprintf(buff, "Error creating %s\n", path);
    perror(buff);
  }

  std::cerr << "make_directory(" << path << ") returning " << ret << std::endl << std::flush;
  return(ret);
}

static int compute_scatter()
{
  char em_dir[FILENAME_MAX], log_dir[FILENAME_MAX], qc_dir[FILENAME_MAX];
  char at_file[FILENAME_MAX], sc_file[FILENAME_MAX];
  char *ext=NULL;

  //Create log and qc dirs if not existing
  strcpy(em_dir,em_prefix);
  if ((ext=strrchr(em_dir,DIR_SEPARATOR)) != NULL)
    *ext = '\0';
  else
    getcwd(em_dir,sizeof(em_dir));
  sprintf(log_dir,"%s%c%s",em_dir,DIR_SEPARATOR,"log");
  sprintf(qc_dir,"%s%c%s",em_dir,DIR_SEPARATOR,"qc");

  if (make_directory(log_dir)) {
    fprintf(log_fp, "Error creating %s\n", log_dir);
    return(1);
  }
  if (make_directory(qc_dir)) {
    fprintf(log_fp, "Error creating %s\n", qc_dir);
    return(1);
  }
  
  strcpy(at_file,mu_file);
  if ((ext=strrchr(at_file,'.')) != NULL)
    if (strcasecmp(ext,".i") == 0)
      *ext = '\0';
  strcat(at_file,".a");
  if (access(at_file,R_OK) == 0  && overwrite==0) {
    fprintf(log_fp,"Reusing existing %s\n",at_file);
  } else {
    sprintf(cmd_line, "%s/%s --model 328 -u %s -w 128 --oa %s --span 9 --mrd 67 --prj ifore --force -l 53,%s", \
            program_path, prog_e7_fwd, mu_file, at_file, log_dir);
    fprintf(log_fp,"%s\n",cmd_line);
    fflush(log_fp);
    if (exec)
      system(cmd_line);
  }
    // Compute scatter

  if (lber<0)
    // i.e. if not already indicated by -L option as input, then it reads from the .n.hdr file
    get_lber(norm_file, lber);

  for (int frame=0; frame<num_frames; frame++) {
    sprintf(sc_file, "%s_frame%d_sc.s ", em_prefix,frame);
    if (access(sc_file,R_OK) == 0 && overwrite==0) {
      fprintf(log_fp,"Reusing existing %s\n",sc_file);
    } else {
      sprintf(cmd_line, "%s/%s -e %s_frame%d.tr.s -u %s  -w 128 "
        "-a %s -n %s --force --os %s --os2d --gf --model 328 "
        "--skip 2 --mrd 67 --span 9 --ssf 0.25,2 -l 73,%s -q %s",
              program_path, prog_e7_sino, em_prefix, frame, mu_file, at_file, norm_file, sc_file, log_dir, qc_dir);
      if (lber>0.0f)
        sprintf(&cmd_line[strlen(cmd_line)]," --lber %g",lber);
      if (athr != NULL)
        sprintf(&cmd_line[strlen(cmd_line)]," --athr %s",athr);
      fprintf(log_fp,"%s\n",cmd_line);
      fflush(log_fp);
      if (exec)
        system(cmd_line);

      //Scatter QC
      /*
      sprintf(cmd_line, "cd %s", qc_dir);
      fprintf(log_fp,"%s\n",cmd_line);
      fflush(log_fp);
      if (exec)
        chdir(qc_dir);
      */

      sprintf(cmd_line,"cd %s; %s %s/scatter_qc_00.plt", em_dir, prog_gnuplot, em_dir);
      fprintf(log_fp,"%s\n",cmd_line);
      fflush(log_fp);
      if (exec)
        system(cmd_line);

      sprintf(cmd_line,"rename %s/scatter_qc_00.ps %s/%s_frame%d_sc_qc.ps ", em_dir, em_dir, em_prefix, frame);
      fprintf(log_fp,"%s\n",cmd_line);
      fflush(log_fp);
      if (exec)
        system(cmd_line);

      /*
      sprintf(cmd_line, "cd %s", em_dir);
      fprintf(log_fp,"%s\n",cmd_line);
      fflush(log_fp);
      if (exec)
        chdir(em_dir);
      */
    }
  }
  return(0);
}

static void AIR_motion_qc(const char *ecat_file, float threshold)
{
  char plt_fname[FILENAME_MAX], dat_fname[FILENAME_MAX];
  int frame = 0;
  char line[40];
  FILE *qc_dat_file=NULL, *qc_plt_file=NULL, *pp=NULL;

  int thr = (int)(threshold*32767/100);

  for (frame=0; frame<num_frames; frame++) {
      int frame1=frame+1;

      if (ecat_reslice_flag) {
        if (frame_info[frame].em_align_flag==0) {
          // Copy frame
          sprintf(cmd_line,"%s/%s -i %s.v,%d,1,1 -o %s_rsl.v,%d,1,1",
		  program_path, prog_matcopy, \
                  im_prefix, frame1, im_prefix, frame1); // ECAT 1-N counting
          fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
          if (exec)
            system(cmd_line);
          continue;
        }
      }
      
      sprintf(cmd_line,"%s/%s %s,%d,1,1 %s,%d,1,1 %s_fr%d.air -m 6 -t1 %d -t2 %d",
	      program_path, prog_alignlinear, \
              ecat_file, ref_frame+1, ecat_file, frame+1, // ECAT 1-N counting
              em_prefix, frame,                           // Standardized 0-N counting
              thr, thr);
      fprintf(log_fp,"%s\n",cmd_line);
      if (exec)
        system(cmd_line);
      
      if (ecat_reslice_flag) {
        sprintf(cmd_line,"%s/%s %s_fr%d.air %s_rsl.v,%d,1,1 -a %s.v,%d,1,1 -k -o",
		program_path, prog_reslice, \
                im_prefix, frame, im_prefix, frame1, im_prefix, frame1);
        fprintf(log_fp,"%s\n",cmd_line);
        if (exec)
          system(cmd_line);
      }
    }
  //Create Motion QC plot
  if (exec) {
    sprintf(dat_fname,"%s_motion_qc.dat", em_prefix);
    if ((qc_dat_file=fopen(dat_fname,"wt")) == NULL) {
      fprintf(log_fp,"Error opening file %s\n", dat_fname);
      exit(1);
    }
  }
  
  for (frame=0; frame<num_frames; frame++) {
    int x0 = frame_info[frame].start_time;
    int x1 = x0+frame_info[frame].duration-1;
    if (frame>=start_frame && frame<num_frames && frame != ref_frame) {
      // motion_distance output is like '0 0.694 0.733 -0.224 1.03'
      sprintf(cmd_line,"motion_distance -a %s_fr%d.air", em_prefix, frame);
      fprintf(log_fp,"%s\n",cmd_line);
      if (exec) {
        if ((pp = popen(cmd_line,"r")) != NULL) {
          fgets(line, sizeof(line),pp);
          fprintf(qc_dat_file,"%d %s",x0, line);
          fprintf(qc_dat_file,"%d %s",x1, line);
          pclose(pp);
        } else {
          fprintf(log_fp,"Error opening pipe %s\n", cmd_line);
          exit(1);
        }
      }
    } else {
      if (exec) {
          fprintf(qc_dat_file,"%d %d 0 0 0 0\n",x0, x0);
          fprintf(qc_dat_file,"%d %d 0 0 0 0\n",x1, x0);
      }
    }
  }
  if (exec)
    fclose(qc_dat_file); // close .dat file
  sprintf(plt_fname,"%s_motion_qc.plt", em_prefix);
  if (exec) {
    if ((qc_plt_file=fopen(plt_fname,"wt")) != NULL) {
      fprintf(qc_plt_file,"set terminal postscript portrait color 'Helvetica' 8\n");
      fprintf(qc_plt_file,"set output  '%s_motion_qc.ps'\n", em_prefix);
      fprintf(qc_plt_file,"set multiplot\n");
      fprintf(qc_plt_file,"set size 1.0,0.45\n");
      fprintf(qc_plt_file,"set title '%s Motion QC'\n",em_prefix);
      fprintf(qc_plt_file,"set origin 0.,0.55\n");
      fprintf(qc_plt_file,"set grid\n");
      fprintf(qc_plt_file,"plot '%s' using 1:3 title 'TX' with lines,\\\n", dat_fname);
      fprintf(qc_plt_file,"     '%s' using 1:4 title 'TY' with lines,\\\n", dat_fname);
      fprintf(qc_plt_file,"     '%s' using 1:5 title 'TZ' with lines,\\\n", dat_fname);
      fprintf(qc_plt_file,"     '%s' using 1:6 title 'D' with lines\n", dat_fname);
      fclose(qc_plt_file);
    }
  }
  sprintf(cmd_line,"%s %s", prog_gnuplot, plt_fname);
  fprintf(log_fp,"%s\n",cmd_line);
  if (exec)
    system(cmd_line);
}
static int sc_motion_qc()
{
  size_t nprojs=256, nviews=288, nplanes=207;
  char sc_prefix[FILENAME_MAX], fname[FILENAME_MAX], plt_fname[FILENAME_MAX];
  std::vector< std::vector<float> > scale_factors;
  FILE *fp;
  int frame=0, plane=0, found_frames=0;;

  if (num_frames<2) return 0;
  scale_factors.resize(num_frames);
  strcpy(sc_prefix, em_prefix);
  // clean prefix
  int len = strlen(sc_prefix);
  while (len>2 && strncasecmp(&sc_prefix[len-2],"EM", 2)!=0)
    sc_prefix[--len] = '\0';
 
  size_t sino_size = nprojs*nviews*nplanes*sizeof(float);
  for (frame=0; frame<num_frames; frame++) {
    sprintf(fname, "%s_frame%d_sc.s ", sc_prefix,frame);
    if ((fp=fopen(fname,"rb")) != NULL) {
      fprintf(log_fp,"Reading %s scale factors\n", fname);
      if (fseek(fp,sino_size,SEEK_SET) == 0) {
        scale_factors[frame].resize(nplanes);
        fread(&scale_factors[frame][0],nplanes,sizeof(float),fp);
        found_frames++;
      }
      fclose(fp);
    } else fprintf(log_fp,"%s not found\n", fname);
  }
  if (found_frames<2) return 0;
  sprintf(fname,"%s_sc_qc.dat", sc_prefix);
  fprintf(log_fp,"Create %s\n", fname);
  if (exec) {
    if ((fp=fopen(fname,"wt")) != NULL) {
      for (plane=0; plane<(int)nplanes; plane++) {
        if (scale_factors[0].size() != nplanes) 
          fprintf(fp,"%d 1.0", plane);
        else
          fprintf(fp,"%d %g",plane, scale_factors[0][plane]);

        for (frame=0; frame<num_frames; frame++) {
          if (scale_factors[frame].size() != nplanes)
            fprintf(fp," 1.0");
          else
            fprintf(fp," %g",scale_factors[frame][plane]);
        }
        fprintf(fp,"\n");
      }
      fclose(fp);
    } else
      fprintf(log_fp,"Create %s failed\n", fname);
  }
  sprintf(plt_fname,"%s_sc_qc.plt", sc_prefix);
  fprintf(log_fp,"Create %s\n", plt_fname);
  if (exec) {
    int num_plots = (num_frames+4)/5; // 5 frames per plot
    int num_pages = (num_plots+5)/6;  // 2x3 plots per page
    frame = 0;
    if ((fp=fopen(plt_fname,"wt")) != NULL) {
      fprintf(fp,"set terminal postscript landscape color 'Helvetica' 8\n");
      fprintf(fp,"set output  '%s_sc_qc.ps'\n", sc_prefix);
      for (int page=0; page<num_pages && frame<num_frames; page++) {
        fprintf(fp,"set multiplot\n");
        fprintf(fp,"set size .45,0.3\n");
        fprintf(fp,"set grid\n");
        for (int plot=0; plot<6 && frame<num_frames; plot++) {
          int x = plot%2, y=plot/2;
          fprintf(fp,"set title 'multiframe scatter QC, frames %d:%d'\n",frame+1,frame+5);
          fprintf(fp,"set origin %g,%g\n", 0.45*x, 1.0f-0.32*(y+1));
          int plot_frames=0;
          for (int pframe=0; pframe<5 && frame<num_frames; pframe++, frame++) {
            if (scale_factors[frame].size() != nplanes)
              continue;
            if (plot_frames>0)
              fprintf(fp,",\\\n");
            if (pframe== 0)
              fprintf(fp,"plot '%s' using 1:%d title 'f%d' with lines", fname, frame+2, frame+1);
            else 
              fprintf(fp,"     '%s' using 1:%d title 'f%d' with lines", fname, frame+2, frame+1);
            plot_frames++;
          }
          fprintf(fp,"\n");
        }
        fprintf(fp,"set nomultiplot\n");
        fprintf(fp,"reset\n");
      }
      fclose(fp);
    }
  }
  sprintf(cmd_line,"%s %s", prog_gnuplot, plt_fname);
  fprintf(log_fp,"%s\n",cmd_line);
  if (exec)
    system(cmd_line);
  return 1;
}

int main(int argc, char **argv)
{
  // ahc made this a required arg
  // char *recon_pgm="hrrt_osem3d_u", *align_pgm="AIR";
  char *recon_pgm = NULL;
  char log_file[FILENAME_MAX];
  const char *data_dir = "D:\\SCS_SCANS";
  char *em_file=NULL, *ext=NULL;
  char em_dyn_file[FILENAME_MAX];
  char em_ecat_file[FILENAME_MAX], fname[FILENAME_MAX];
  char patient_dir[FILENAME_MAX];
  char line[80];
  int frame=0;

  int fwhm=6; // gaussian smoothing in mm
  int c=0;
  FILE *fp=NULL, *pp=NULL;
  char *p1=NULL, *p2=NULL;
  float AIR_threshold=21.5f;

  program_path[0] = '\0';
  prog_gnuplot[0] = '\0';
  dir_log[0] = '\0';

  if (argc<2) usage(argv[0]);
  em_file = argv[1];
  mu_file[0] = '\0';
  while ((c = getopt (argc-1, argv+1, "n:r:F:x:D:u:a:L:g:T:R:Otvp:z:l:")) != EOF) {
    switch (c) {
      case 'n':
        norm_file = optarg;
        break;
      case 'u':
        strcpy(mu_file, optarg);
        break;
      case 'a':
        athr = optarg;
        break;
      case 'L' :
        if (sscanf(optarg,"%g",&lber) != 1) {
          printf("Invalid LBER value : %s\n", optarg);
          usage(argv[0]);
        } 
        break;
      case 'x':
        recon_pgm = optarg;
        break;
      case 'D':
        data_dir = optarg;
        break;
      case 'O':
        overwrite = 1;
        break;
      case 'R':
        if (sscanf(optarg,"%d",&ecat_reslice_flag) != 1) {
          printf("Invalid ecat_reslice_flag : %s, should be integer number\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'g':
        if (sscanf(optarg,"%d",&fwhm) != 1) {
          printf("Invalid FWHM : %s, should be integer number\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'r':
        if (sscanf(optarg,"%d",&ref_frame) != 1) {
          printf("Invalid reference frame number : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'F':
        if (sscanf(optarg,"%d,%d",&start_frame, &end_frame) < 1) {
          printf("Invalid reference frame range : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 't' :
        exec = 0;
        break;
      case 'v' :
        verbose = 1;
        break;
      case 'T':
        if (sscanf(optarg,"%g",&AIR_threshold) != 1) {
          printf("Invalid AIR threshold : %s\n", optarg);
          usage(argv[0]);
        }
        break;
    case 'p':
      // ahc program_path now a required argument.
      strcpy(program_path, optarg);
      break;
    case 'z':
      // ahc fully qualified path to gnuplot now a required argument.
      strcpy(prog_gnuplot, optarg);
      break;
    case 'l':
      // ahc optional path for log file.
      strcpy(dir_log, optarg);
      break;
    }
  }

  // Check required arguments present.
  if (!strlen(program_path)) {
    fprintf(stderr, "Error: Missing required parameter '-p' (program path)\n");
    usage(argv[0]);
  }
  if (!strlen(prog_gnuplot)) {
    fprintf(stderr, "Error: Missing required parameter '-z' (FQ path of gnuplot)\n");
    usage(argv[0]);
  }
  if (!strlen(recon_pgm)) {
    fprintf(stderr, "Error: Missing required parameter '-x' (FQ path of recon program)\n");
    usage(argv[0]);
  }
  
  strcpy(em_prefix,em_file);
  if ((ext=strrchr(em_prefix,'.')) == NULL) {
    printf( "%s unknown file type\n", em_file);
    exit(1);
  }
  *ext++ = '\0';

  strcpy(im_prefix,em_prefix);  
  printf("\n im_prefix:%s \n", im_prefix);
  fflush(stdout);
  memset(em_ecat_file,0,sizeof(em_ecat_file));
  memset(em_dyn_file,0,sizeof(em_dyn_file));
    
  if (strcasecmp(ext,"v") == 0) {  // ahc yes
    // ecat reconstructed images file
    MatrixFile *mf=NULL;
    MatrixData *matdata;
    Image_subheader *imh;
    MatDirNode *node;
    int suggested_ref_frame=-1; // first 5min frame

    // Create log file in patient dir
    if (strlen(dir_log)) {
      sprintf(log_file, "%s/motion_qc.log", dir_log);
    } else {
      sprintf(log_file, "%s_motion_qc.log", em_prefix);
    }

    if ((log_fp=fopen(log_file,"wt")) == NULL) {
      perror(log_file);
      exit(1);
    }
    strcpy(em_ecat_file, em_file);

    
    if ((mf=matrix_open(em_file,MAT_READ_ONLY,MAT_UNKNOWN_FTYPE)) == NULL) {
      fprintf(log_fp, "Error opening %s\n", em_file);
      exit(1);
    }
    num_frames = mf->mhptr->num_frames;
    node = mf->dirlist->first;
    frame_info.resize(num_frames);

    for (frame=0; frame<num_frames && node!=NULL; frame++) {
      if ((matdata = matrix_read(mf, node->matnum, MAT_SUB_HEADER)) == NULL) {
        fprintf(log_fp,"Error reading frame %d header\n", frame);
        exit(1);
      }
      imh = (Image_subheader*)matdata->shptr;
      frame_info[frame].start_time = imh->frame_start_time/1000;
      frame_info[frame].duration = imh->frame_duration/1000;
      frame_info[frame].randoms = imh->random_rate*frame_info[frame].duration;
      frame_info[frame].trues = (imh->prompt_rate-imh->random_rate)*frame_info[frame].duration;

      printf("\n frame_info[%d].trues: %f (start_time: %d)", frame, frame_info[frame].trues,frame_info[frame].start_time);
      fflush(stdout);

      if (frame_info[frame].trues<5000000)
          printf(" Trues < 5M (Potential source of OSEM overestimation bias)");
      fflush(stdout);
                     
      frame_info[frame].MAF_frame = frame;
      if (start_frame<0 && frame_info[frame].duration>=MIN_FRAME_DURATION)
        start_frame = frame;

      if (frame_info[frame].trues>max_trues && frame_info[frame].start_time>=600) {
        max_trues=frame_info[frame].trues;
        ref_frame=frame;
      }
      
      node = node->next;
      free_matrix_data(matdata);
    }
    
    matrix_close(mf);
    if (end_frame<0)
      end_frame = num_frames-1; // last frame
    if (ref_frame<0) {
      fprintf(log_fp,"Use -r to specify reference, no frame long enough for default\n");
      fflush(log_fp);
      printf("Use -r to specify reference, no frame long enough for default\n");
      fflush(stdout);
      exit(1);
    }

    for (frame=0; frame<num_frames; frame++) {
      if (frame >= start_frame && frame <= end_frame)
        frame_info[frame].em_align_flag = 1;
      else
        frame_info[frame].em_align_flag = 0;
    }
    frame_info[ref_frame].em_align_flag = 0;
  } else {
    // Extension is not 'v'
    if (strcasecmp(ext,"dyn") == 0) { 
      // Create log_file in patient dir 
      sprintf(log_file, "%s_motion_qc.log", em_prefix);
      if ((log_fp=fopen(log_file,"wt")) == NULL) {
        perror(log_file);
        exit(1);
      }
      if (norm_file==NULL) {
        fprintf(log_fp, "Normalization required for %s reconstruction\n", em_file);
        exit(1);
      }
      strcpy(em_dyn_file, em_file);
    } else {
      // dyn
      if (strcasecmp(ext,"l64") == 0) {
        // Histogram listmode in data directory
        strcpy(fname, em_file);
        if ((p2=strrchr(fname, DIR_SEPARATOR)) != NULL) {
          *p2 = '\0';
          p1 = strrchr(fname, DIR_SEPARATOR);
        }
        if (p1!= NULL) {
          sprintf(patient_dir, "%s%c%s", data_dir, DIR_SEPARATOR, p1+1);
        } else {
          strcpy(patient_dir,data_dir);
        }
        
        // create patient directory
        if (access(patient_dir,R_OK) != 0) {
          if (make_directory(patient_dir)) {
            return(1);
          }
        }
        if (p2 != NULL)
          sprintf(em_prefix, "%s%c%s", patient_dir, DIR_SEPARATOR, p2+1);
        else
          sprintf(em_prefix, "%s%c%s", patient_dir, DIR_SEPARATOR, fname);
        ext=strrchr(em_prefix, '.');
        *ext = '\0';

        // Create log_file in patient dir 
        sprintf(log_file, "%s_motion_qc.log", em_prefix);
        printf("%s\n", log_file);
        if ((log_fp=fopen(log_file,"wt")) == NULL) {
          perror(log_file);
          exit(1);
        }
        if (norm_file==NULL) {
          fprintf(log_fp, "Normalization required for %s reconstruction\n", em_file);
          exit(1);
        }
        sprintf(em_dyn_file,"%s.dyn", em_prefix);
        sprintf(cmd_line,"%s/%s %s -o %s.s -PR", \
		program_path, prog_lmhistogram, \
		em_file, em_prefix);
        printf("%s\n",cmd_line);
        if (access(em_dyn_file,R_OK) == 0 && overwrite==0) {
          fprintf(log_fp,"Reusing existing %s\n",em_dyn_file);
        } else {
          fprintf(log_fp,"%s\n",cmd_line);
          if (exec)
            system(cmd_line);
          if (access(em_dyn_file,R_OK) != 0) {
            fprintf(log_fp,"Error creating file %s\n", em_dyn_file);
            exit(1);
          }
        }
      } else {
        fprintf(log_fp, "Invalid emission input file %s\n", em_file);
        usage(argv[0]);
      }
    } // else dyn
  } // else v

  // Reconstruct ecat images if l64 or dyn input
  if (em_ecat_file[0] == '\0') {
    // ahc no.
    if ((fp=fopen(em_dyn_file,"rt")) == NULL) {
      fprintf(log_fp,"Error opening file %s\n", em_dyn_file);
     exit(1);
    }
    // read number from .dyn file
    fgets(line,sizeof(line), fp);
    sscanf(line,"%d", &num_frames);

    printf("\n\n\n num_frames:%d", num_frames);
    fflush(stdout);
    
    fclose(fp);
    // Read frame info from sinogram headers
    for (frame=0; frame<num_frames; frame++) {
      sprintf(fname,"%s_frame%d.s.hdr",em_prefix,frame);
      if (!get_frame_info(frame, fname, num_frames))
        exit(1);
      // Set alignment start and end frame if not specified
      if (start_frame<0 && frame_info[frame].duration>=MIN_FRAME_DURATION)
        start_frame = frame; // standardized 0-N counting
    // Set reference frame if not specified

      if (frame_info[frame].trues>max_trues && frame_info[frame].start_time>=600) {
        max_trues=frame_info[frame].trues;
        ref_frame=frame;
      }
    }

    // Fill em_ecat_file and start reconstruction
    if (recon_pgm != NULL) {
      //sprintf(em_ecat_file,"%s_na.v", em_prefix);
      sprintf(em_ecat_file,"%s.v", em_prefix);
      sprintf(cmd_line,"%s -t %s -n %s -X 128 -o %s -W 2 -I 4 -S 8 -N -T 1",
        recon_pgm, em_dyn_file, norm_file, em_ecat_file);
      if (access(em_ecat_file,R_OK) == 0 && overwrite==0) {
        fprintf(log_fp,"Reusing existing %s\n",em_ecat_file);
      } else {
        fprintf(log_fp,"%s\n",cmd_line);
        if (exec) 
	  system(cmd_line);
      }
    } else {
      // can't create image files, create scatter QC
      em_ecat_file[0] = '\0';
    }
  }  // if (not em_ecat_file)
    
  printf("\n start_frame: %d, end_frame:%d, ref_frame:%d \n", start_frame, end_frame, ref_frame);
  fflush(stdout);
  
  // scatter scaling motion QC
  if (strlen(mu_file)>0) {
    if (compute_scatter()) {
      fprintf(log_fp,"Error in compute_scatter()\n");
      exit(1);
    }
    sc_motion_qc();
  }

  // Reconstructed images motion QC
  if (strlen(em_ecat_file)>0) {
    // Smooth reconstructed images with 6mm
    sprintf(fname,"%s_%dmm.v",em_prefix, fwhm);
    sprintf(cmd_line, "%s/%s %s %d %s", program_path, prog_gsmooth, em_ecat_file, fwhm, fname);
    printf("\nfname: %s\n", fname);
    fflush(stdout);
    
    if (access(fname,R_OK) == 0 && overwrite==0) {
      fprintf(log_fp,"Reusing existing %s\n",fname);
    } else {
      fprintf(log_fp,"%s\n",cmd_line);
      fflush(log_fp);
      printf("%s\n",cmd_line);
      fflush(stdout);
      if (exec)
        system(cmd_line);
    }
     // Use AIR to compute motion transformers on smoothed images
    AIR_motion_qc(fname, AIR_threshold);
  }

  fclose(log_fp);
  exit(0);
  return 0;
}

