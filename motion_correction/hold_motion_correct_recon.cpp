/*
  Arman:

    const char *recon_pgm = "je_hrrt_osem3d";

    Added normfac_img input as 'K'

    Used to be based on ref_frame_trues (but now is duration): see hold_motion_correct_recon.cpp for comparison
    
======================
    
File: motion_correct_recon.cpp
Authors:
  Merence Sibomana
  Sune H. Keller
  Oline V. Olesen

Purpose: Reconstruct HRRT multi-frame sinograms with motion corrected attenuation
   Method A: Using AIR transformers from motion_qc
   Method B: Using vicra trackfile
     1- Create corrected frame mu-map using AIR transformers from motion_qc or trackfile
     2- Create corrected frame attenuation
     3- Compute scatter using corrected frame mu-map and attenuation
     4- Reconstruct frames using corrected attenuation
 Method A Only:
     5- Realign reconstructed frames using smoothed images
     6- Optionally, specify frames to be realigned.

Method A usage:
   motion_correct_recon -e em_dyn_file  -n norm [-u mu_map][-F start_frame[,end_frame]] [-O] [-t]              
     - Default start_frame is first frame with NEC=0.5M, start_frame transformer used
       for earlier frames
     - Default end_frame is last frame with NEC=0.5M, end_frame transformer used for
       remaining frames
     -O: Overwrite existing images (default is reuse)
     -t: test only
Method B usage:
    motion_correct_recon -e em_l64_file -u mu_map -n norm -Q vicra_frame_info
                          [-c calibration_dir]
     - Reference frame in vicra_frame_info file
     - Default calibration directory c:\cps\calibration_files (win32),
       $HOME/calibration_files (linux) 

 NB : Frame numbers start from 1

Creation date: 14-feb-2010
 Modification dates:
      10-AUG-2010: Use 3D_ATX prefix for images reconstructed using aligned TX
      11-AUG-2010: Bug fixes (default iterations=10, PSF=true)
                   Use -E for Uncorrected_ecat_file
                   Use 0 start for frame numbering everywhere for consistency
      21-AUG-2010: Add -T option for AIR alignment threshold
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <ecatx/matrix.h>
#include <string>
#ifdef WIN32
#include <direct.h>
#include <ecatx/getopt.h>
#ifdef NEW_CODE
#include <AIR/AIR.h>
#endif
#include <io.h>
#define R_OK 4
#define access _access
#define chdir _chdir
#define getcwd _getcwd
#define mkdir _mkdir
#define popen _popen
#define pclose _pclose
#define stat _stat
#define		DIR_SEPARATOR '\\'
#define strcasecmp _stricmp
#else
#include <dirent.h>
#include <unistd.h>
#define		DIR_SEPARATOR '/'
#endif

#define MAX_ITERATIONS 20
#define PIXEL_SIZE 1.21875f
#define IMAGE_SIZE 256
#define NPLANES 207
#define EM_ALIGN_THRESHOLD 1.0f

#include "frame_info.h"
#include "qmatrix.h"

static char cmd_line[2048];
const char *data_dir = "D:\\SCS_SCANS";
static char em_dir[FILENAME_MAX], log_dir[FILENAME_MAX], qc_dir[FILENAME_MAX]; 
static char reconjob_file[FILENAME_MAX], plt_fname[FILENAME_MAX];
static char patient_dir[FILENAME_MAX];
double max_trues=0;

static void usage(const char *pgm)
{
  printf("%s Build %s %s \n", pgm, __DATE__, __TIME__);
  printf("AIR usage: %s  em_dyn_file -u mu_map -n norm [-F start_frame[,end_frame]] \n"
         "       [-E uncorrected_ecat_file] [-r ref_frame] [options]\n",pgm);
  printf("Vicra usage: %s em_l64_file -u mu_map -n norm -M vicra_frame_info_file",pgm);
  printf("Options: \n");
  printf("       -A polaris to scanner coordinates alignement\n");
  printf("       -O Overwrite exiting images (reuse images by default)\n");
  printf("        -T AIR_threshold (percent of max pixel) (default= 21.5)\n");
  printf("                  suggested values: FDG=36\n");
  //printf("       -c time_constant (default from image header, 9.34e-6 if not in header]\n");
  printf("       -c time_constant (default from image header, 6.67e-6 (for Hopkins HRRT) )\n");
  printf("       -q units (default Bq/ml)\n");
  printf("       -C calibration_dir (default C:\\CPS\\calibration_files)\n");
  printf("       -S cluster_submit_dir (default D:\\Recon-Jobs\\Jobs-Submitted)\n");
  printf("       -a e7_sino athr argument (default blank )\n");
  printf("       -I number of iterations (default 10)\n");
  printf("       -P Enable PSF (default yes)\n");
  printf("       -t test only, prints commands in motion_correct_pp.log\n");
  printf("       -v verbose (default off)\n");
  printf("       \nNB:  FRAMES NUMBERS STARTS from 0 **********\n");
  exit(1);
}


static const char *my_current_time()
{
	static char time_str[20];
	time_t now;
	time(&now);
	struct tm *ptm = localtime(&now);
	sprintf(time_str,"%04d%02d%02d%02d%02d%02d", 
    (1900+ptm->tm_year)%100, ptm->tm_mon+1, ptm->tm_mday, 
    ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	return time_str;
}

static float motion_distance(Matrix m)
{
  float x1[4], x2[4];

  x1[0] = (float)(IMAGE_SIZE*PIXEL_SIZE/2);  // center
  x1[1] = (float)(IMAGE_SIZE*PIXEL_SIZE/2)-100.0f; // 10 cm from the center 
  x1[2]= (float)(NPLANES*PIXEL_SIZE/2); // center
  x1[3] = 1.0f;
  mat_apply(m, x1, x2);
  float dx = (x2[0]-x1[0]);
  float dy = (x2[1]-x1[1]);
  float dz = (x2[2]-x1[2]);
  return (float)(sqrt(dx*dx + dy*dy + dz*dz));

}

static void vicra_align_tx_frame(const float *tx_t, const float *em_t, char *out)
{
  Matrix tx_ref=mat_alloc(4,4); // TX to reference transformer
  Matrix em_inv=mat_alloc(4,4); // EM to reference inverse transformer
  Matrix tmp=mat_alloc(4,4);
  memcpy(tx_ref->data, tx_t, 12*sizeof(float));
  //mat_print(tx_ref);
  memcpy(tmp->data, em_t, 12*sizeof(float));
  //mat_print(tmp);
  mat_invert(em_inv, tmp);
  matmpy(tmp, tx_ref, em_inv);
  //mat_print(tmp);
  float *p = tmp->data;
  sprintf(out,"%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f",
    p[0], p[1], p[2], p[3],p[4], p[5], p[6], p[7],p[8], p[9], p[10], p[11]);
  mat_free(tx_ref);
  mat_free(tmp);
  mat_free(em_inv);
}

static void vicra_align_txt(const VicraEntry &ref, const VicraEntry &in, char *out)
{
  Matrix tmp=mat_alloc(4,4);
  vicra_align(ref.q, ref.t, in.q, in.t, tmp);
  float *p = tmp->data;
  sprintf(out,"%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f",
    p[0], p[1], p[2], p[3],p[4], p[5], p[6], p[7],p[8], p[9], p[10], p[11]);
  mat_free(tmp);
}

#ifdef NEW_CODE
static void AIR_align_txt(struct AIR_Air16 &air_16, char *out)
{
  Matrix t_inv=NULL;
  Matrix t = mat_alloc(4,4); // direct transformer
  float *p=t->data;
  for (int j=0;j<3;j++)
  {
    for (int i=0;i<4;i++)
      t->data[i+j*4] = (float)air_16.e[i][j];
  }
  p[3] *= (float)air_16.r.x_size;
  p[7] *= (float)air_16.r.x_size;
  p[11] *= (float)air_16.r.z_size;
  
  sprintf(out,"%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f",
    p[0], p[1], p[2], p[3],p[4], p[5], p[6], p[7],p[8], p[9], p[10], p[11]);
  mat_free(t);
}
#endif

static int add_frame_def(const char *vicra_file, char *out, FILE *log_fp)
{
  char *p=out;
  int duration=0, repeat=0;
  duration=vicra_info.em[0].dt; repeat=1;
  for (unsigned i=1; i<vicra_info.em.size();i++) {
    if (vicra_info.em[i].dt != duration) {
      if (repeat>1) sprintf(p," -d %d*%d", duration, repeat);
      else sprintf(p," -d %d", duration);
      duration = vicra_info.em[i].dt;
      repeat = 1;
      p += strlen(p);
    } else repeat++;
  }
  // remaining  frames
  if (repeat>1) sprintf(p," -d %d*%d", duration, repeat);
  else sprintf(p," -d %d", duration);
  return strlen(out);
}
#ifdef WIN32
static char *em_histogram(const char *in_file, const char *vicra_file,
                          FILE *log_fp, int overwrite, int exec_flag)
{
  // Histogram listmode in data directory
  int n=0;
  char out_file[FILENAME_MAX];
	char drive[_MAX_DRIVE];
	char in_dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
  char *p1=NULL, *p2=NULL;

  // determine sinogram output directory
	_splitpath(in_file, drive, in_dir, fname, ext);
  if ((n=strlen(in_dir))>1) {
    in_dir[n-1] = '\0'; // remove end DIR_SEPARATOR 
    if ((p1=strrchr(in_dir, DIR_SEPARATOR)) != NULL)
      sprintf(patient_dir, "%s%s", data_dir, p1); 
    else
      sprintf(patient_dir, "%s%c%s", data_dir, DIR_SEPARATOR, in_dir);
  } else {
    sprintf(patient_dir, "%s%c%s", data_dir, DIR_SEPARATOR, fname);
  }

    // create sinogram directory if not existing
  if (access(patient_dir,R_OK) != 0) {
#ifndef LINUX
    mkdir(patient_dir);
#else
    sprintf(cmd_line, "mkdir %s", patient_dir);
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) system(cmd_line);
#endif
  }
  // check existing dyn file and return if existing
  sprintf(out_file, "%s%c%s.dyn", patient_dir, DIR_SEPARATOR, fname);
  if (access(out_file,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing existing %s\n",out_file);
    fflush(log_fp);
  } else {
    sprintf(cmd_line,"lmhistogram_u %s -o %s%c%s.s -PR", in_file,
       patient_dir, DIR_SEPARATOR, fname);
    add_frame_def(vicra_file, cmd_line+strlen(cmd_line), log_fp);
    fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
    if (exec_flag) {
      system(cmd_line);
      if (access(out_file,R_OK) != 0) { 
        fprintf(log_fp,"Error creating file %s\n", out_file);  fflush(log_fp);
        return NULL;
      }
    }
  }
  return strdup(out_file);
}
#endif

static void update_frame_info(FILE *log_fp)
{
  Matrix m=NULL;
  unsigned fi = frame_info.size();
  unsigned vi = vicra_info.em.size();
  if (fi==vi) {
    Matrix mv_ref = mat_alloc(4,4); // vicra space reference matrix
    Matrix mv = mat_alloc(4,4); // vicra space matrix
    Matrix ms = mat_alloc(4,4); // scanner space matrix
    const VicraEntry &er = vicra_info.em[vicra_info.ref_frame];
    q2matrix(er.q,er.t,mv_ref);
    printf("****** Reference matrix\n");
    mat_print(mv_ref);
    /*
    printf("%g,%g,%g,%g,%g,%g,%g\n",er.q[0],er.q[1],er.q[2],er.q[3],er.t[0],er.t[1],er.t[2]);
    float *q =  vicra_info.tx.q, *t =  vicra_info.tx.t;
    printf("%g,%g,%g,%g,%g,%g,%g\n",q[0],q[1],q[2],q[3],t[0],t[1],t[2]);
    */
    for (unsigned i=0; i<vi; i++) {
      const VicraEntry &e = vicra_info.em[i];
      q2matrix(e.q,e.t,mv);
#ifdef _DEBUG
      printf("****** Frame %d matrix\n",i);;
      mat_print(mv);
#endif
      vicra_align(mv_ref, mv, ms);
#ifdef _DEBUG
      printf("****** Frame %d combined matrix\n",i);;
      mat_print(ms);
#endif
      memcpy(frame_info[i].transformer, ms->data, 12*sizeof(float));
      frame_info[i].tx_align_flag = e.tx_align;
      frame_info[i].em_align_flag = e.em_align;
    }
    q2matrix( vicra_info.tx.q, vicra_info.tx.t,mv);
    vicra_align(mv_ref, mv, ms);
    memcpy(tx_info.transformer, ms->data, 12*sizeof(float));
    mat_free(mv_ref);
    mat_free(mv);
    mat_free(ms);
  } else {
   fprintf(log_fp,"Frame_info and vicra_info sizes not match = %d,%d\n", fi,vi);
  }
}
#ifdef UNIT_TEST
#include "unit_test.h"
#endif
int main(int argc, char **argv)
{
  const char *recon_pgm = "je_hrrt_osem3d";
  const char *log_file = "motion_correct_recon.log";
  const char *units = "Bq/ml";
  const char *athr=NULL;
  char line[80];
  char *normfac_img="normfac.i";
  
#ifdef WIN32
  const char *calib_dir = "C:\\CPS\\calibration_files";
#else
  const char *calib_dir = NULL;
#endif
  const char *submit_dir="D:\\Recon-Jobs\\Jobs-Submitted";
  const char *calib_file=NULL;
  char *em_file=NULL, *mu_file=NULL, *norm_file=NULL, *vicra_file=NULL, *ext=NULL;
  const char *vicra2scanner=NULL;
  char em_prefix[FILENAME_MAX], mu_prefix[FILENAME_MAX], fname[FILENAME_MAX];
  char im_prefix[FILENAME_MAX], mu_rsl[FILENAME_MAX], at_rsl[FILENAME_MAX];
  char em_na[FILENAME_MAX];
  int ref_frame = -1;
  int frame=0, start_frame=-1, end_frame=-1, num_frames=0;
  float lber=-1.0f, user_time_constant = 0.0f;
  FILE *fp=NULL, *pp=NULL, *log_fp=NULL;
  int c, exec=1, verbose = 0, overwrite=0, recon_flag=1;
  int s=6; // default smoothing 6mm
  int num_iterations=10, psf_flag=1;
  int ecat_reslice_flag = 1;
#ifdef NEW_CODE
  char air_file[FILENAME_MAX];
  struct AIR_Air16 air_16;
#endif
  float AIR_threshold = 21.5f;
  int mu_width=128;
 struct stat mu_st;

  if (argc<2) usage(argv[0]);
  em_file = argv[1];
  memset(em_na, 0, sizeof(em_na));
  
  while ((c = getopt (argc-1, argv+1, "n:F:s:S:u:r:c:q:C:a:I:L:M:A:E:T:K:POtv")) != EOF) {
    switch (c) {
      case 'u':
        mu_file = optarg;
        break;
      case 'n':
        norm_file = optarg;
        break;
      case 'A':
        vicra2scanner = optarg;
        break;
      case 'M':
        vicra_file = optarg;
        ecat_reslice_flag = 0;
        break;
      case 'E':
        ext = strrchr(optarg,'.');
        if (ext != NULL && tolower(ext[1])=='v') 
        { // ecat alignment file
          strcpy(em_na, optarg);
          ext = strrchr(em_na,'.');
          if (ext != NULL) *ext = '\0';
        }
        else 
        {
          printf("Invalid uncorrected_ecat_file : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'T':
        if (sscanf(optarg,"%g",&AIR_threshold) != 1) 
        {
          printf("Invalid AIR threshold : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'a':
        athr = optarg;
        break;
      case 'r':
        if (sscanf(optarg,"%d",&ref_frame) != 1) {
          printf("Invalid reference frame number : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'c' :
        sscanf(optarg,"%g",&user_time_constant);
        break;
      case 'I' :
        if (sscanf(optarg,"%d",&num_iterations) != 1 || 
          (num_iterations<0 || num_iterations>MAX_ITERATIONS))  {
          printf("Invalid number of iterations : %s\n", optarg);
          usage(argv[0]);
        }
        break;
      case 'L' :
        if (sscanf(optarg,"%g",&lber) != 1) {
          printf("Invalid LBER value : %s\n", optarg);
          usage(argv[0]);
        }
        printf("\n lber values is: %g \n", lber); fflush(stdout);
        break;
      case 'K': //Need to make changes later
        normfac_img = optarg;   
        break;
      case 'q':
        units = optarg;
        break;
      case 'O':
        overwrite = 1;
        break;
      case 'P':
        psf_flag = 1;
        break;
      case 'C':
        calib_dir = optarg;
        break;
      case 'S':
        submit_dir = optarg;
        break;
      case 's':
        calib_file = optarg;
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
        
    }
  }
#ifdef UNIT_TEST
  if (/*vicra2scanner!=NULL && */vicra_file!=NULL)
    unit_test(vicra2scanner,vicra_file, stdout);
  return 0;
#endif
  /* Test remove_blanks 
  strcpy(line, "   1,-0.003,-0.008,1.37,0.00316,   1,0.03,-4.42,0.008,-0.03,   1,1.59");
  puts(line);
  remove_blanks(line);
  puts(line);
  */
  if ((ext=strrchr(em_file,'.')) == NULL) {
    printf("%s unknown file type\n", em_file);
    return 1;
  }
  if (access(em_file, R_OK) != 0) {
    printf("%s file not found\n", em_file);
    return 1;
  }

#ifdef WIN32
  if (strcasecmp(ext,".l64") && strcasecmp(ext,".v") && strcasecmp(ext,".dyn")) {
    printf("Invalid em_file %s\n", em_file);
    return 1;
  }
#else // 64bit rebinning only on WIN32
  if (strcasecmp(ext,".v") && strcasecmp(ext,".dyn")) {
    printf("Invalid em_file %s\n", em_file);
    return 1;
  }
#endif

  if (strcasecmp(ext,".v") ) { // mu_map and norm required for recon
    if (norm_file==NULL) usage(argv[0]);
    if (mu_file != NULL) {
      if (stat(mu_file,&mu_st) != 0) {
        printf("%s file not found\n", mu_file);
        return 1;
      }
      if (mu_st.st_size == 256*256*NPLANES*sizeof(float)) {
        mu_width = 256;
      }
    }
    if (access(norm_file, R_OK) != 0) {
      printf("%s file not found\n", norm_file);
      return 1;
    }
  }
  if ((log_fp=fopen(log_file,"wt")) == NULL) {
    perror(log_file);
    return 1;
  }
  if (vicra_file) {
    if (vicra_calibration(vicra2scanner, log_fp)) {
      fprintf(log_fp,"using %s file for coordinates alignment\n", vicra2scanner);
      fflush(log_fp);
    } else {
      fprintf(log_fp,"Error loading %s file for coordinates alignment\n", vicra2scanner);
      fflush(log_fp);
      return 1;
    }
    if (vicra_get_info(vicra_file, log_fp) == 0) return 1;
    if (ref_frame < 0) ref_frame = vicra_info.ref_frame;
  }
#ifdef WIN32
  if (vicra_file && strcasecmp(ext,".l64") == 0) {
   printf("em_histogram %s\n", em_file);
    char *em_dyn_file = em_histogram(em_file, vicra_file, log_fp, overwrite, exec); 
    if (em_dyn_file == NULL) {
      fprintf(log_fp,"Error histogramming file %s\n", em_file);
      return 1;
    }
   em_file = em_dyn_file;
   strcpy(em_prefix, em_file);
  }
  else strcpy(em_prefix,em_file);
#else
  strcpy(em_prefix,em_file);
#endif
  ext=strrchr(em_prefix,'.');
  *ext++ = '\0';
  // Realign only option for ECAT files
  if (*ext == 'v') {
    MatrixFile *mf;
    MatrixData *matdata;
    Image_subheader *imh;
    MatDirNode *node;
    if ((mf=matrix_open(em_file, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE))==NULL){
      fprintf(log_fp,"Error opening file %s\n", em_file);
      return 1;
    }
    num_frames = mf->dirlist->nmats;
    if (ref_frame<0) ref_frame = 1;
    if (start_frame<0) start_frame = ref_frame;
    if (end_frame<0) end_frame = num_frames-1;
    frame_info.resize(num_frames);
    node = mf->dirlist->first;
    for (frame=0; frame<num_frames && node!=NULL; frame++) {
      if ((matdata = matrix_read(mf, node->matnum, MAT_SUB_HEADER)) == NULL) {
        fprintf(log_fp,"Error reading frame %d header\n", frame);
        return 1;
      }
      imh = (Image_subheader*)matdata->shptr;
      frame_info[frame].start_time = imh->frame_start_time/1000;
      frame_info[frame].duration = imh->frame_duration/1000;
      frame_info[frame].randoms = imh->random_rate*frame_info[frame].duration;
      frame_info[frame].trues = (imh->prompt_rate-imh->random_rate)*frame_info[frame].duration;
      
      
      frame_info[frame].MAF_frame = frame;
      if (start_frame<0 && frame_info[frame].duration>=MIN_FRAME_DURATION)  start_frame = frame;
      //      if (ref_frame<0 && frame_info[frame].duration>=DEF_REF_FRAME_DURATION) ref_frame = frame;
      if (frame_info[frame].trues>max_trues){
        max_trues=frame_info[frame].trues;
        ref_frame=frame;
      }
      
      node = node->next;
      free_matrix_data(matdata);
    }
    matrix_close(mf);
    strcpy(im_prefix, em_prefix);
    if (vicra_file) {
      update_frame_info(log_fp);
      start_frame = 0; // standardized 0-N counting
      end_frame = num_frames-1;
    } else {
      if (ref_frame<0) {
        fprintf(log_fp,"Use -r to specify reference, no frame long enough for default\n");
        exit(1);
      }
      if (end_frame<0) end_frame=num_frames-1;
      for (frame=0; frame<num_frames; frame++) {
        if (frame >= start_frame && frame <= end_frame)
          frame_info[frame].em_align_flag = 1;
        else frame_info[frame].em_align_flag = 0;
      }
      frame_info[ref_frame].em_align_flag = 0;
    }
    goto ecat_ready;
  }

  // Reconstruction

  if (lber<0) // i.e. if not already indicated by -L option as input, then it reads from the .n.hdr file
    get_lber(norm_file, lber);

  if (mu_file != NULL) {
    strcpy(mu_prefix,mu_file);
    if ((ext=strstr(mu_prefix,".i")) == NULL) {
      fprintf(log_fp, "Invalid mu_file %s\n", mu_file);
      return 1;
    }
  }
  *ext = '\0';
  if ((fp=fopen(em_file,"rt")) == NULL) {
    fprintf(log_fp,"Error opening file %s\n", em_file);
    return 1;
  }
  fgets(line,sizeof(line), fp);
  sscanf(line,"%d", &num_frames);
  fclose(fp);
  for (frame=0; frame<num_frames; frame++) {
    sprintf(fname,"%s_frame%d.s.hdr",em_prefix,frame);
    get_frame_info(frame, fname, num_frames);
    if (start_frame<0 && frame_info[frame].duration>=MIN_FRAME_DURATION)  start_frame = frame;
    //if (ref_frame<0 && frame_info[frame].duration>= DEF_REF_FRAME_DURATION) ref_frame = frame;
    if (frame_info[frame].trues>max_trues && frame_info[frame].start_time>=600){
      max_trues=frame_info[frame].trues;
      ref_frame=frame;
    }
    
  }

  if (vicra_file) {
    update_frame_info(log_fp);
    start_frame = 0; // standardized 0-N counting
    end_frame = num_frames-1;
  } else {
    if (ref_frame<0) {
      fprintf(log_fp,"Use -r to specify reference, no frame long enough for default\n");
      exit(1);
    }
    if (end_frame<0) end_frame=num_frames-1;
    for (frame=0; frame<num_frames; frame++) {
      if (frame >= start_frame && frame <= end_frame)
        frame_info[frame].em_align_flag = frame_info[frame].tx_align_flag = 1;
      else 
        frame_info[frame].em_align_flag = frame_info[frame].tx_align_flag = 0;
    }
    // No correction for frame of reference
    frame_info[ref_frame].em_align_flag = frame_info[ref_frame].tx_align_flag = 0;
  }

  for (frame=0; frame<num_frames; frame++) {
    printf("\n frame_info[%d].trues: %f", frame, frame_info[frame].trues); fflush(stdout);
    if (frame_info[frame].trues<5000000)
      printf(" Trues < 5M (Potential source of OSEM overestimation bias)"); fflush(stdout);
  }
  
  printf("\n\n\n start_frame: %d, end_frame:%d, ref_frame:%d \n\n\n", start_frame, end_frame, ref_frame); fflush(stdout);
  
  //exit(0);
  
  
  //Create log and qc dirs if not existing
  strcpy(em_dir,em_prefix);
  if ((ext=strrchr(em_dir,DIR_SEPARATOR)) != NULL) {
    *ext++ = '\0';
    sprintf(cmd_line, "cd %s", em_dir);
    fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
    if (exec) chdir(em_dir);
    strcpy(em_prefix, ext); // em_prefix without full path
  }
  else getcwd(em_dir,sizeof(em_dir));
  sprintf(log_dir,"%s%c%s",em_dir,DIR_SEPARATOR,"log");
  sprintf(qc_dir,"%s%c%s",em_dir,DIR_SEPARATOR,"qc");
#ifdef WIN32
  if (access(log_dir,R_OK) != 0) mkdir(log_dir);
  if (access(qc_dir,R_OK) != 0) mkdir(qc_dir);
#else
  if (access(log_dir,R_OK) != 0) {
    sprintf(cmd_line,"mkdir %s", log_dir);
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) system(cmd_line);
  }
  if (access(qc_dir,R_OK) != 0) {
    sprintf(cmd_line,"mkdir %s", qc_dir);
    fprintf(log_fp,"%s\n",cmd_line);
    if (exec) system(cmd_line);
  }
#endif
   
  fflush(log_fp);
  sprintf(fname,"%s_3D_ATX.v",em_prefix);
  if (access(fname,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing existing %s\n",fname);
  } else {
    //if (strlen(em_na) == 0) sprintf(em_na,"%s_na", em_prefix);
    if (strlen(em_na) == 0) sprintf(em_na,"%s", em_prefix);
    //else sprintf(em_na,"%s_na", em_na);
    

    if (mu_file != NULL) {
      // Reconstruct frames using corrected mu_maps
      //Create individual frame mu-maps
      for (frame=0; frame<num_frames; frame++) 
      {
        if (!frame_info[frame].tx_align_flag)
        {
          fprintf(log_fp,"Using original %s\n",mu_file);
          fflush(log_fp);
          continue; 
        }
        sprintf(mu_rsl, "%s_fr%d.i", mu_prefix,frame);
        if (access(mu_rsl,R_OK) == 0 && overwrite==0) {
          fprintf(log_fp,"Reusing existing %s\n",mu_rsl); fflush(log_fp);
        } 
        else
        { 
          if (vicra_file && frame_info[frame].tx_align_flag) 
          {
            sprintf(cmd_line,"volume_reslice -i %s -M ", mu_file);
            vicra_align_txt(vicra_info.em[frame], vicra_info.tx,
              &cmd_line[strlen(cmd_line)]);
            sprintf(&cmd_line[strlen(cmd_line)]," -o %s", mu_rsl);
            fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
            if (exec) system(cmd_line);
          }
          else 
          {
            // Create mu-map frame transfromer by inverting transformer
            // that was computed using motion_qc program from uncorrected images 
            //Create transformed mu_map using inverse transfomer
#ifndef NEW_CODE


            sprintf(cmd_line,"invert_air %s_fr%d.air %s_fr%d.air y",
              em_na,frame,mu_prefix,frame);
            fprintf(log_fp,"%s\n",cmd_line);
            if (exec) system(cmd_line);
            if (ecat_reslice_flag) {            
              sprintf(cmd_line,"ecat_reslice %s_fr%d.air %s -a %s -o -k",
                mu_prefix,frame,mu_rsl, mu_file);

            } else {
              sprintf(cmd_line,"volume_reslice -a %s_fr%d.air -o %s -i %s ",
                mu_prefix,frame,mu_rsl, mu_file);
              
            }
#else
            
            sprintf(air_file,"%s_fr%d.air",em_na,frame);
            if (AIR_read_air16(air_file, &air_16)!= 0) {
              fprintf(log_fp,"Error reading AIR file %s\n", air_file);
              continue;
            }
            sprintf(cmd_line,"volume_reslice -i %s -r -M ", mu_file);
            AIR_align_txt(air_16,  &cmd_line[strlen(cmd_line)]);
            sprintf(&cmd_line[strlen(cmd_line)]," -R  -o %s", mu_rsl);
#endif
            fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);

            printf("\n frame: %d \n ", frame); fflush(stdout);
            printf("%s\n",cmd_line); fflush(stdout);
            
            if (exec) system(cmd_line);
          }
        }

        //if (frame==5)
        // exit(0);

      }
    }



   
    // Reconstruct frames 
    std::vector<std::string> cluster_images;
    for (frame=0; frame<num_frames; frame++)
    {
      if (mu_file != NULL)
      {
        if (!frame_info[frame].tx_align_flag)
        { // no correction
          strcpy(mu_rsl, mu_file);
          sprintf(at_rsl, "%s.a", mu_prefix);
          if (access(at_rsl,R_OK) == 0)  fprintf(log_fp,"Reusing existing %s\n",at_rsl);
          else 
          { 
            sprintf(cmd_line, "e7_fwd_u --model 328 -u %s -w %d --oa %s --span 9 "
            "--mrd 67 --prj ifore --force -l 53,%s", mu_rsl, mu_width, at_rsl, log_dir);
            fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
            if (exec) system(cmd_line);
          }  
        }
        else
        {
          sprintf(mu_rsl, "%s_fr%d.i", mu_prefix,frame);
          sprintf(at_rsl, "%s_fr%d.a", mu_prefix,frame);
          if (access(at_rsl,R_OK) == 0 && overwrite==0)
          {
            fprintf(log_fp,"Reusing existing %s\n",at_rsl);
          }
          else
          { 
            sprintf(cmd_line, "e7_fwd_u --model 328 -u %s -w %d --oa %s --span 9 "
                "--mrd 67 --prj ifore --force -l 53,%s", mu_rsl, mu_width, at_rsl, log_dir);
            fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
            if (exec) system(cmd_line);
          }
        }
        // Compute scatter
        sprintf(fname, "%s_frame%d_sc_ATX.s ", em_prefix,frame);
        if (access(fname,R_OK) == 0 && overwrite==0) {
          fprintf(log_fp,"Reusing existing %s\n",fname);
        } else {
          sprintf(cmd_line, "e7_sino_u -e %s_frame%d.tr.s -u %s  -w %d "
            "-a %s -n %s --force --os %s --os2d --gf --model 328 "
            "--skip 2 --mrd 67 --span 9 --ssf 0.25,2 -l 73,%s -q %s",
            em_prefix, frame, mu_rsl, mu_width, at_rsl, norm_file, fname, log_dir, qc_dir);
          if (lber>0.0f) sprintf(&cmd_line[strlen(cmd_line)]," --lber %g",lber);
          if (athr != NULL) sprintf(&cmd_line[strlen(cmd_line)]," --athr %s",athr);
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec) system(cmd_line);
        
          //Scatter QC
          sprintf(cmd_line, "cd %s", qc_dir);
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec) chdir(qc_dir);
          strcpy(cmd_line,"gnuplot scatter_qc_00.plt");
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec) system(cmd_line);
          sprintf(cmd_line,"rename scatter_qc_00.ps %s_frame%d_sc_ATX_qc.ps ",em_prefix,frame);
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec) system(cmd_line);
          sprintf(cmd_line, "cd %s", em_dir);
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec) chdir(em_dir);
        }
      }// end attenuation and scatter processing
      
      // hrrt_osem3d_v1.2_x64 -p %EMF%.s -n %no% -a %TXF%.a -s %EMF%_sc_ATX.s -o %EMF%.i -W 3 \
      // -d %EMF%.ch -I 10 -S 16 -m 9,67 -B 0,0,0  
      sprintf(fname, "%s_frame%d_3D_ATX.i ", em_prefix,frame);
      if (access(fname,R_OK) == 0 && overwrite==0) {
        fprintf(log_fp,"Reusing existing %s\n",fname); fflush(log_fp);
      }
      else 
      {
        if (access(submit_dir,R_OK) != 0) {   // host reconstruction
          if (mu_file != NULL) {
            sprintf(cmd_line, "%s -p %s_frame%d.s -d %s_frame%d.ch -s %s_frame%d_sc_ATX.s "
              " -o %s -n %s -a %s -W 3 -I %d -S 16 -m 9,67 -T 4 -X 256 -K %s ",
              recon_pgm, em_prefix, frame, em_prefix, frame, em_prefix, frame, fname,
                    norm_file, at_rsl, num_iterations, normfac_img);
          } else {
            sprintf(cmd_line, "%s -p %s_frame%d.s -d %s_frame%d.ch -o %s -n %s "
              "-W 3 -I %d -S 16 -m 9,67 -T 4 -X 256 -K %s ",
              recon_pgm, em_prefix, frame, em_prefix, frame, fname,
                    norm_file, num_iterations, normfac_img);
          }
          if (psf_flag) strcat(cmd_line, " -B 0,0,0");
          fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
          if (exec) system(cmd_line);
        } // end host reconstruction
        else
        {  // cluster reconstruction
          sprintf(reconjob_file,"%s\\recon_job_%s_frame%d.txt", submit_dir,
            my_current_time(), frame);
          if (exec) {
            if ((fp=fopen(reconjob_file,"wt")) != NULL) {
              fprintf(fp,"#!clcrecon\n\n");
              fprintf(fp,"scanner_model          999\n");
              fprintf(fp,"span                   9\n");
              fprintf(fp,"resolution             256\n");
              fprintf(fp,"outputimage            %s%c%s\n\n",
                em_dir,DIR_SEPARATOR, fname);
              fprintf(fp,"norm                   %s\n", norm_file);
              if (mu_file != NULL) {
                fprintf(fp,"atten                  %s%c%s\n",
                  em_dir,DIR_SEPARATOR,at_rsl);
                fprintf(fp,"scatter                %s%c%s_frame%d_sc_ATX.s\n\n",
                  em_dir,DIR_SEPARATOR,em_prefix, frame);
              }
              fprintf(fp,"iterations             %d\n",num_iterations);
              fprintf(fp,"subsets                16\n");
              if (psf_flag) fprintf(fp,"psf\n");				// set PSF FLAG
              fprintf(fp,"weighting              3\n");
              fprintf(fp,"prompt                 %s%c%s_frame%d.s\n",
                em_dir,DIR_SEPARATOR,em_prefix, frame);
              fprintf(fp,"delayed                %s%c%s_frame%d.ch\n",
                em_dir,DIR_SEPARATOR,em_prefix, frame);
              fclose(fp);
              fprintf(log_fp,"%s submitted to cluster\n",reconjob_file);
              fflush(log_fp);
              cluster_images.push_back(fname);
            } else {
              fprintf(log_fp,"Error creating %s cluster job file\n",reconjob_file);
              fflush(log_fp);
            }
          } else {
              fprintf(log_fp,"Submit %s cluster job file\n",reconjob_file);
              fflush(log_fp);
          }
        } // end cluster submission reconstructrion
      } // else
   } // Frame processing

   int wait_count = cluster_images.size();
   while (wait_count>0)
   { // wait 30sec
#ifdef WIN32
     Sleep(30000);
#else
     sleep(30);
#endif
     wait_count=0;
     for (size_t ci=0; ci<cluster_images.size(); ci++)
     {
       if (access(cluster_images[ci].c_str(),R_OK) != 0) wait_count++;
       if (wait_count==1) 
       {
         fprintf(log_fp,"%s waiting %s\n", my_current_time(),
           cluster_images[ci].c_str());
         fflush(log_fp);
       }
     }
   }

   // Conversion to ECAT
   if (user_time_constant <= 0.0f) user_time_constant = deadtime_constant;
#ifdef WIN32
   sprintf(cmd_line,"if2e7 -v -e %g ",  user_time_constant);
   if (access(calib_dir, R_OK) == 0)
     sprintf(cmd_line+strlen(cmd_line)," -u %s -S %s", units, calib_dir);
   sprintf(cmd_line+strlen(cmd_line)," %s_frame0_3D_ATX.i", em_prefix);
#else
   sprintf(cmd_line,"if2e7 -g 0 -u %s -v -e %g -s %s %s_frame0_3D_ATX.i", units, user_time_constant, calib_file, em_prefix);
#endif
   printf("%s\n",cmd_line); fflush(stdout);
   fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
   if (exec) system(cmd_line); 
 } 
 sprintf(fname,"%s_3D_ATX.v",em_prefix);
 sprintf(im_prefix,"%s_3D_ATX", em_prefix);

ecat_ready:
    // Smooth images for AIR alignment
#ifndef NEW_CODE
  if (!vicra_file) {
    sprintf(cmd_line, "gsmooth_ps %s.v %d", im_prefix, s);
    sprintf(fname,"%s_%dmm.v",im_prefix,s);
    if (access(fname,R_OK) == 0 && overwrite==0) {
      fprintf(log_fp,"Reusing existing %s\n",fname); fflush(log_fp);
    } else {
      fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
      if (exec) system(cmd_line);
    }
  }
#endif

  // Create Realigned images
  if (vicra_file) 
  {
    if (vicra_info.maf_flag)  sprintf(fname,"%s_MAF_rsl.v", im_prefix);
    else sprintf(fname,"%s_rsl.v", im_prefix);
  } else sprintf(fname,"%s_rsl.v", im_prefix);
  if (access(fname,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing existing %s\n",fname);
  }
  else 
  {
    for (frame=0; frame<num_frames; frame++)  
    {
      int frame1=frame+1;                  // ECAT 1-N counting
      if (frame_info[frame].em_align_flag==0) {
        // Copy frame
        sprintf(cmd_line,"matcopy -i %s.v,%d,1,1 -o %s,%d,1,1",
          im_prefix, frame1, fname, frame1); // ECAT 1-N counting
        fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
        if (exec) system(cmd_line);
        continue;
      }

      // Align frame
      if (vicra_file)
      {
        sprintf(cmd_line,"volume_reslice -i %s.v,%d,1,1 -o %s,%d,1,1 -M %s",
          im_prefix,  frame1, fname, frame1, transformer_txt(frame_info[frame].transformer));
        fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
        if (exec) system(cmd_line);
      }
      else
      {
#ifndef NEW_CODE
        int thr = (int)(AIR_threshold*32767/100);
        sprintf(cmd_line,"ecat_alignlinear %s_%dmm.v,%d,1,1 %s_%dmm.v,%d,1,1 %s_fr%d.air -m 6 -t1 %d -t2 %d",
          im_prefix, s, ref_frame+1, im_prefix, s, frame1, im_prefix, frame, thr, thr);
        fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
        if (exec) system(cmd_line);
        sprintf(cmd_line,"ecat_reslice %s_fr%d.air %s,%d,1,1 -a %s.v,%d,1,1 -k -o",
          im_prefix, frame, fname, frame1, im_prefix, frame1);
#else
        sprintf(air_file,"%s_fr%d.air",em_na,frame);
        if (AIR_read_air16(air_file, &air_16)!= 0)  {
          fprintf(log_fp,"Error reading AIR file %s\n", air_file);
          continue;
        }
        sprintf(cmd_line,"volume_reslice -i %s.v,%d,1,1 -o %s,%d,1,1 -r -M ",
          im_prefix,  frame1, fname, frame1);
        AIR_align_txt(air_16, &cmd_line[strlen(cmd_line)]);
#endif
        fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
        if (exec) system(cmd_line);
      }
    }
  }

  //Create Motion QC plot
  if (!vicra_file) {
#ifndef NEW_CODE
    sprintf(fname,"%s_motion_qc.dat", em_prefix);
    if ((fp=fopen(fname,"wt")) == NULL) {
      fprintf(log_fp,"Error opening file %s\n", fname);
      return 1;
    }

    for (frame=start_frame; frame<=end_frame; frame++) {
      int x0 = frame_info[frame].start_time;
      int x1 = x0+frame_info[frame].duration-1;
      if (frame_info[frame].em_align_flag == 0)
      {
        fprintf(log_fp,"reference frame %d\n",frame);  fflush(log_fp);
        fprintf(fp,"%d %d 0 0 0 0\n",x0,x0);
        fprintf(fp,"%d %d 0 0 0 0\n",x1,x0);
        continue;
      }
      sprintf(cmd_line,"motion_distance -a %s_fr%d.air", im_prefix, frame);
      fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
      if (exec) {
        if ((pp = popen(cmd_line,"r")) != NULL) {
          fgets(line, sizeof(line),pp);
          fprintf(fp,"%d %s",x0, line);
          fprintf(fp,"%d %s",x1, line);
          pclose(pp);
        } else {
          fprintf(log_fp,"Error opening pipe %s\n", cmd_line);
          return 1;
        }
      }
    }
    fclose(fp);
    sprintf(plt_fname,"%s_motion_qc.plt", im_prefix);
    if ((fp=fopen(plt_fname,"wt")) != NULL) {
      fprintf(fp,"set terminal postscript portrait color 'Helvetica' 8\n");
      fprintf(fp,"set output  '%s_motion_qc.ps'\n", im_prefix);
      fprintf(fp,"set multiplot\n");
      fprintf(fp,"set size 1.0,0.45\n");
      fprintf(fp,"set title '%s Motion QC'\n",im_prefix);
      fprintf(fp,"set origin 0.,0.55\n");
      fprintf(fp,"set grid\n");
      fprintf(fp,"plot '%s' using 1:3 title 'TX' with lines,\\\n", fname);
      fprintf(fp,"     '%s' using 1:4 title 'TY' with lines,\\\n", fname);
      fprintf(fp,"     '%s' using 1:5 title 'TZ' with lines,\\\n", fname);
      fprintf(fp,"     '%s' using 1:6 title 'D' with lines\n", fname);
      fclose(fp);
    }
    sprintf(cmd_line,"gnuplot %s", plt_fname);
    fprintf(log_fp,"%s\n",cmd_line);  fflush(log_fp);
    if (exec) system(cmd_line);
#endif
  }
  else if (vicra_info.maf_flag) { // Join sub-frames
    sprintf(cmd_line,"MAF_join %s -o %s_rsl.v -M %s", fname, im_prefix, vicra_file);
    fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
    if (exec) system(cmd_line);
  }
  fclose(log_fp);
  return 0;
}
