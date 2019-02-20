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

  [-c calibration_dir]
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
#include <ecatx/ecat_matrix.hpp>
#include <string>
#include <dirent.h>
#include <unistd.h>
#define		DIR_SEPARATOR '/'

#define MAX_ITERATIONS 20
#define PIXEL_SIZE 1.21875f
#define IMAGE_SIZE 256
#define NPLANES 207
#define EM_ALIGN_THRESHOLD 1.0f

#include "frame_info.h"
#include "qmatrix.h"

// ahc
#include <iostream>

// Following 3 args sent to run_system_command()
static char program_name[2048]; // Program to run
static char cmd_line[2048];     // Args to program
static char check_file[2048];   // Output file to check for existence

const char *data_dir = "D:\\SCS_SCANS";
static char em_dir[FILENAME_MAX], log_dir[FILENAME_MAX], qc_dir[FILENAME_MAX]; 
static char reconjob_file[FILENAME_MAX], plt_fname[FILENAME_MAX];
static char patient_dir[FILENAME_MAX];
double max_trues=0;
FILE *log_fp=NULL;

// ahc program_path now a required argument.
char program_path[FILENAME_MAX];
char prog_gnuplot[FILENAME_MAX];
char rebinner_lut_file[FILENAME_MAX];
char dir_log[FILENAME_MAX];

// HRRT programs (all must be found in program_path)
static const char *prog_alignlinear = "ecat_alignlinear";
static const char *ecat_make_air    = "ecat_make_air";
static const char *prog_e7_fwd      = "e7_fwd_u";
static const char *prog_e7_sino     = "e7_sino_u";
static const char *prog_gsmooth     = "gsmooth_ps";
static const char *prog_if2e7       = "if2e7";
static const char *prog_invert_air  = "invert_air";
// static const char *prog_lmhistogram = "lmhistogram_u"; // Was originally in this prog.
static const char *prog_lmhistogram = "lmhistogram_mp";
static const char *prog_maf_join    = "MAF_join";
static const char *prog_make_air    = "ecat_make_air";
static const char *prog_matcopy     = "matcopy";
static const char *prog_recon       = "je_hrrt_osem3d";
static const char *prog_reslice     = "ecat_reslice";
static const char *prog_volume_reslice   = "volume_reslice";
static const char *prog_motion_distance = "motion_distance";

// prog:    Program to run
// args:    Argument string to program
// outfile: Optional output file to check for
// Check for existence of given program, then run with given args.
// Returns: 0 on success, else 1

int run_system_command( char *prog, char *args ) {
  char command_line[4096];
  int ret = 0;
  char *ptr = NULL;
  char ma_pattern_name[FILENAME_MAX];
  
  ma_pattern_name[0] = '\0';
  if ((ptr=getenv("HOME")) != NULL) {
    sprintf(ma_pattern_name, "%s/.ma_pattern.dat", ptr);
    if (!access(ma_pattern_name, R_OK)) {
      LOG_ERROR("*** ma_pattern file exists: '%s'\n", ma_pattern_name);      
      if (remove(ma_pattern_name) != 0) {
        LOG_ERROR("*** ERROR: Removing ma_pattern file: '%s'\n", ma_pattern_name);
      } else {
        LOG_ERROR("*** Removed ma_pattern file OK: '%s'\n", ma_pattern_name);
      }
    } else {
      LOG_ERROR("*** ma_pattern file '%s' does not exist\n", ma_pattern_name);
    }
  } else {
    LOG_ERROR("*** Could not determine HOME envt var: Cannot remove ma_pattern.dat file\n");
  }
  fflush(stderr);
       
  ret = access(prog, X_OK);
  if (ret) {
    std::cerr << "Error: run_system_command(" << prog << "): File does not exist" << std::endl << std::flush;    
  } else {
    std::cerr << "*** run_system_command('" << prog << "', '" << args << "')" << std::endl << std::flush;
    sprintf(command_line, "%s %s", prog, args);
    fprintf(log_fp, "%s\n", command_line);
    ret = system(command_line);
  }
  std::cerr << "run_system_command('" << prog << "', '" << args << "') returning " << ret << std::endl << std::flush;
  return(ret);
}

static void usage(const char *pgm){
  printf("%s Build %s %s \n", pgm, __DATE__, __TIME__);
  printf("AIR usage: %s  em_dyn_file -u mu_map -n norm [-F start_frame[,end_frame]] \n"
         "       [-E uncorrected_ecat_file] [-r ref_frame] [options]\n",pgm);
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
  printf("    *   -p program_path: Path to HRRT executables (required)\n");
  printf("    *   -z gnuplot: FQ path of gnuplot program (required)\n");
  printf("    *   -b rebinner_lut_file: FQ path of rebinner LUT program (required)\n");
  printf("        -l logdir: Directory for log file\n");
  printf("       \nNB:  FRAME NUMBERS START from 0 **********\n");
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

int main(int argc, char **argv)
{
  // const char *recon_pgm = "je_hrrt_osem3d";
  const char *log_file = "motion_correct_recon.log";
  const char *units = "Bq/ml";
  const char *athr=NULL;
  char line[80];
  char *normfac_img="normfac.i";
  
  const char *calib_dir = NULL;
  const char *submit_dir="D:\\Recon-Jobs\\Jobs-Submitted";
  const char *calib_file=NULL;
  char *em_file=NULL, *mu_file=NULL, *norm_file=NULL, *ext=NULL;
  char em_prefix[FILENAME_MAX], mu_prefix[FILENAME_MAX], fname[FILENAME_MAX];
  char im_prefix[FILENAME_MAX], mu_rsl[FILENAME_MAX], at_rsl[FILENAME_MAX];
  char em_na[FILENAME_MAX];
  int ref_frame = -1;
  int frame=0, start_frame=-1, end_frame=-1, num_frames=0;
  float lber=-1.0f, user_time_constant = 0.0f;
  FILE *fp=NULL, *pp=NULL;
  int c, exec=1, verbose = 0, overwrite=0, recon_flag=1;
  int default_smoothing=6; // default smoothing 6mm
  int num_iterations=10, psf_flag=1;
  int ecat_reslice_flag = 1;
  int brand_new_final_align = 0;
  
  float AIR_threshold = 21.5f;
  int mu_width=128;
  struct stat mu_st;
  char alignchar;

  // ahc additions
  program_path[0] = '\0';
  prog_gnuplot[0] = '\0';
  rebinner_lut_file[0] = '\0';
  dir_log[0] = '\0';
  log_fp=NULL;  // Made global so run_system_command can log.

  if (argc<2) usage(argv[0]);
  em_file = argv[1];
  memset(em_na, 0, sizeof(em_na));
  
  while ((c = getopt (argc-1, argv+1, "n:F:s:S:u:r:c:q:C:a:I:L:M:A:E:T:K:R:POtvp:z:l:b:")) != EOF) {
    switch (c) {
    case 'u':
      mu_file = optarg;
      break;
    case 'n':
      norm_file = optarg;
      break;
    case 'E':
      ext = strrchr(optarg,'.');
      if (ext != NULL && tolower(ext[1])=='v') {
        // ecat alignment file
        strcpy(em_na, optarg);
        ext = strrchr(em_na,'.');
        if (ext != NULL)
          *ext = '\0';
      } else {
        printf("Invalid uncorrected_ecat_file : %s\n", optarg);
        usage(argv[0]);
      }
      break;
    case 'T':
      if (sscanf(optarg,"%g",&AIR_threshold) != 1) {
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
    case 'R':
      if (sscanf(optarg,"%d",&brand_new_final_align) != 1) {
        printf("Invalid brand_new_final_align number : %s\n", optarg);
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
    case 'b':
      // ahc fully qualified path to rebinner lut file now a required argument.
      strcpy(rebinner_lut_file, optarg);
      break;
    }
  }
  if ((ext=strrchr(em_file,'.')) == NULL) {
    printf("%s unknown file type\n", em_file);
    return 1;
  }
  if (access(em_file, R_OK) != 0) {
    printf("%s file not found\n", em_file);
    return 1;
  }

  if (strcasecmp(ext,".v") && strcasecmp(ext,".dyn")) {
    printf("Invalid em_file %s\n", em_file);
    return 1;
  }


  // ahc test required parameters.
  if (!strlen(program_path)) {
    LOG_ERROR("Error: Missing required parameter 'p' (program path)\n");
    usage(argv[0]);
  }
  if (!strlen(prog_gnuplot)) {
    LOG_ERROR("Error: Missing required parameter 'z' (FQ path of gnuplot program)\n");
    usage(argv[0]);
  }
  if (!strlen(rebinner_lut_file)) {
    LOG_ERROR("Error: Missing required parameter 'b' (FQ path of hrrt_rebinner lut file)\n");
    usage(argv[0]);
  }

  if (strcasecmp(ext,".v") ) {
    // ahc no
    // mu_map and norm required for recon
    if (norm_file==NULL)
      usage(argv[0]);
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
  strcpy(em_prefix,em_file);
  ext=strrchr(em_prefix,'.');
  *ext++ = '\0';
  // Realign only option for ECAT files
  if (*ext == 'v') {
    // ahc no
    ecat_matrix::MatrixFile *mf;
    ecat_matrix::MatrixData *matdata;
    ecat_matrix::Image_subheader *imh;
    ecat_matrix::MatDirNode *node;
    if ((mf=matrix_open(em_file, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE))==NULL){
      fprintf(log_fp,"Error opening file %s\n", em_file);
      return 1;
    }
    num_frames = mf->dirlist->nmats;
    if (ref_frame<0)
      ref_frame = 1;
    if (start_frame<0)
      start_frame = ref_frame;
    if (end_frame<0)
      end_frame = num_frames-1;
    frame_info.resize(num_frames);
    node = mf->dirlist->first;
    for (frame=0; frame<num_frames && node!=NULL; frame++) {
      if ((matdata = matrix_read(mf, node->matnum, ecat_matrix::MatrixDataType::MAT_SUB_HEADER)) == NULL) {
        fprintf(log_fp,"Error reading frame %d header\n", frame);
        return 1;
      }
      imh = (ecat_matrix::Image_subheader*)matdata->shptr;
      frame_info[frame].start_time = imh->frame_start_time/1000;
      frame_info[frame].duration = imh->frame_duration/1000;
      frame_info[frame].randoms = imh->random_rate*frame_info[frame].duration;
      frame_info[frame].trues = (imh->prompt_rate-imh->random_rate)*frame_info[frame].duration;
      
      frame_info[frame].MAF_frame = frame;
      if (start_frame<0 && frame_info[frame].duration>=MIN_FRAME_DURATION)
        start_frame = frame;
      if (frame_info[frame].trues>max_trues){
        max_trues=frame_info[frame].trues;
        ref_frame=frame;
      }
      node = node->next;
      free_matrix_data(matdata);
    }
    matrix_close(mf);
    strcpy(im_prefix, em_prefix);
    if (ref_frame<0) {
      fprintf(log_fp,"Use -r to specify reference, no frame long enough for default\n");
      exit(1);
    }
    if (end_frame<0)
      end_frame=num_frames-1;
    for (frame=0; frame<num_frames; frame++) {
      if (frame >= start_frame && frame <= end_frame)
        frame_info[frame].em_align_flag = 1;
      else
        frame_info[frame].em_align_flag = 0;
    }
    frame_info[ref_frame].em_align_flag = 0;

    goto ecat_ready;
  }  // if ext = 'v'.

  // ahc yes
  // Reconstruction
  // Seems to assume it's a dyn file from here on.

  if (lber<0) // i.e. if not already indicated by -L option as input, then it reads from the .n.hdr file
    // ahc no.  -L calibration_ratio
    get_lber(norm_file, lber);

  if (mu_file != NULL) {
    // ahc yes
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
  // dyn file line 0 holds frame count.
  fgets(line,sizeof(line), fp);
  sscanf(line,"%d", &num_frames);
  fclose(fp);

  // Reference frame is the frame with the most counts starting after 600 seconds.
  for (frame=0; frame<num_frames; frame++) {
    sprintf(fname,"%s_frame%d.s.hdr",em_prefix,frame);
    get_frame_info(frame, fname, num_frames);
    if (start_frame<0 && frame_info[frame].duration>=MIN_FRAME_DURATION)
      start_frame = frame;
    if ((frame_info[frame].trues > max_trues) && (frame_info[frame].start_time >= 600)){
      max_trues=frame_info[frame].trues;
      ref_frame=frame;
    }
  }

  if (ref_frame<0) {
    fprintf(log_fp,"Use -r to specify reference, no frame long enough for default\n");
    exit(1);
  }
  if (end_frame<0)
    end_frame=num_frames-1;
  for (frame=0; frame<num_frames; frame++) {
    if (frame >= start_frame && frame <= end_frame)
      frame_info[frame].em_align_flag = frame_info[frame].tx_align_flag = 1;
    else 
      frame_info[frame].em_align_flag = frame_info[frame].tx_align_flag = 0;
  }
  // No correction for frame of reference
  frame_info[ref_frame].em_align_flag = frame_info[ref_frame].tx_align_flag = 0;

  // Print summary
  printf("%6s %8s %10s %8s\n", "Frame", "Start", "Trues", "Align");
  for (frame=0; frame<num_frames; frame++) {
    // printf("frame_info[%d].trues: %10.1f", frame, frame_info[frame].trues);
    alignchar = (frame == ref_frame) ? 'R' : (frame >= start_frame && frame <= end_frame) ? 'Y' : 'N';
    printf("%6d %8d %8.1f M %8c\n", frame, frame_info[frame].start_time, frame_info[frame].trues / 1000000, alignchar);
    // if (frame_info[frame].trues < 5000000)
    // printf(" ( < 5M: Potential source of OSEM overestimation bias)");
    // printf("\n");
  }
  
  printf("\n start_frame: %d, end_frame:%d, ref_frame:%d \n", start_frame, end_frame, ref_frame);
  fflush(stdout);

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

  if (make_directory(log_dir) or make_directory(qc_dir)) {
    std::cerr << "Error: make_directory(" << log_dir << ", " << qc_dir << ")" << std::endl << std::flush;
    exit(1);
  }
   
  fflush(log_fp);
  sprintf(fname,"%s_3D_ATX.v",em_prefix);
  if (access(fname,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing point 1: existing %s\n",fname);
  } else { // Not reusing 3D_ATX.v
    if (strlen(em_na) == 0)
      sprintf(em_na,"%s", em_prefix);

    if (mu_file != NULL) {
      // Reconstruct frames using corrected mu_maps
      // Create individual frame mu-maps
      for (frame=0; frame<num_frames; frame++) {
        if (!frame_info[frame].tx_align_flag) {
          fprintf(log_fp,"Using original %s\n",mu_file);
          fflush(log_fp);
          continue; 
        }
        sprintf(mu_rsl, "%s_fr%d.i", mu_prefix,frame);
        if (access(mu_rsl,R_OK) == 0 && overwrite==0) {
          fprintf(log_fp,"Reusing point 2: existing %s\n",mu_rsl); fflush(log_fp);
        } else { 
          // Create mu-map frame transfromer by inverting transformer
          // that was computed using motion_qc program from uncorrected images 
          //Create transformed mu_map using inverse transfomer

          sprintf(program_name, "%s/%s", program_path, prog_invert_air);
          sprintf(cmd_line,"%s_fr%d.air %s_fr%d.air y",
                  em_na, frame, mu_prefix, frame);
          if (run_system_command(program_name, cmd_line)) {
            exit(1);
          }
          if (ecat_reslice_flag) {            
            sprintf(program_name, "%s/%s", program_path, prog_reslice);
            sprintf(cmd_line,"%s_fr%d.air %s -a %s -o -k",
                    mu_prefix, frame, mu_rsl, mu_file);
          } else {
            sprintf(program_name, "%s/%s", program_path, prog_volume_reslice);
            sprintf(cmd_line,"-a %s_fr%d.air -o %s -i %s ",
                    mu_prefix, frame, mu_rsl, mu_file);
          }
          // fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          printf("\nFrame: %d \n ", frame); fflush(stdout);
          printf("%s %s\n", program_name, cmd_line); fflush(stdout);
          if (run_system_command(program_name, cmd_line)) {
            exit(1);
          }
        }
      }
    }
   
    // Reconstruct frames 
    for (frame=0; frame<num_frames; frame++)    {
      if (mu_file != NULL)      {
        if (!frame_info[frame].tx_align_flag) {
          // no correction
          strcpy(mu_rsl, mu_file);
          sprintf(at_rsl, "%s_temp_a.a", mu_prefix);
        } else {
          sprintf(mu_rsl, "%s_fr%d.i", mu_prefix,frame);
          sprintf(at_rsl, "%s_fr%d_temp_a.a", mu_prefix,frame);
        }

        if (access(at_rsl,R_OK) == 0 && overwrite==0) {
          fprintf(log_fp,"Reusing point 3: existing %s\n",at_rsl);
        } else { 
          sprintf(program_name, "%s/%s", program_path, prog_e7_fwd);
          sprintf(cmd_line, "--model 328 -u %s -w %d --oa %s --span 9 "
                  "--mrd 67 --prj ifore --force -l 33,%s",
                  mu_rsl, mu_width, at_rsl, log_dir);
          if (run_system_command(program_name, cmd_line)) {
            exit(1);
          }
        } // end attenuation

        // Compute scatter
        sprintf(fname, "%s_frame%d_sc_ATX.s ", em_prefix,frame);
        if (access(fname,R_OK) == 0 && overwrite==0) {
          fprintf(log_fp,"Reusing point 4: existing %s\n",fname);
        } else {
          sprintf(program_name, "%s/%s", program_path, prog_e7_sino);
          sprintf(cmd_line, "-e %s_frame%d.tr.s -u %s  -w %d "
                  "-a %s -n %s --force --os %s --os2d --gf --model 328 "
                  "--skip 2 --mrd 67 --span 9 --ssf 0.25,2 -l 33,%s -q %s",
                  em_prefix, frame, mu_rsl, mu_width, at_rsl, norm_file, fname, log_dir, qc_dir);
          if (lber>0.0f)
            sprintf(&cmd_line[strlen(cmd_line)]," --lber %g",lber);
          if (athr != NULL)
            sprintf(&cmd_line[strlen(cmd_line)]," --athr %s",athr);
          if (run_system_command(program_name, cmd_line)) {
            exit(1);
          }

          sprintf(cmd_line,"cd %s; %s %s/scatter_qc_00.plt", em_dir, prog_gnuplot, em_dir);
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec)
            system(cmd_line);
          sprintf(cmd_line,"rename %s/scatter_qc_00.ps %s/%s_frame%d_sc_qc.ps ", em_dir, em_dir, em_prefix, frame);          
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec)
            system(cmd_line);
          sprintf(cmd_line, "cd %s", em_dir);
          fprintf(log_fp,"%s\n",cmd_line); fflush(log_fp);
          if (exec)
            chdir(em_dir);
        } // end scatter
      } // end attenuation and scatter processing
      
      sprintf(fname, "%s_frame%d_3D_ATX.i ", em_prefix,frame);
      if (access(fname,R_OK) == 0 && overwrite==0) {
        fprintf(log_fp,"Reusing point 5: existing %s\n",fname);
        fflush(log_fp);
      } else {
        // host reconstruction
        sprintf(program_name, "%s/%s", program_path, prog_recon);
        cmd_line[0] = '\0';
        if (mu_file != NULL) {
          sprintf(cmd_line, "-s %s_frame%d_sc_ATX.s -a %s ", em_prefix, frame, at_rsl);
        }
        sprintf(cmd_line, "%s -p %s_frame%d.s -d %s_frame%d.ch  -o %s -n %s -W 3 -I %d -S 16 -m 9,67 -T 2 -X 256 -K %s -r %s", cmd_line, em_prefix, frame, em_prefix, frame, fname, norm_file, num_iterations, normfac_img, rebinner_lut_file);
          
        if (psf_flag)
          strcat(cmd_line, " -B 0,0,0");
        if (run_system_command(program_name, cmd_line)) {
          exit(1);
        }
      } // end reconstruction
    } // Frame processing

    // Conversion to ECAT
    if (user_time_constant <= 0.0f)
      user_time_constant = deadtime_constant;
    sprintf(program_name, "%s/%s", program_path, prog_if2e7);
    sprintf(cmd_line,"-g 0 -u %s -v -e %g -s %s %s_frame0_3D_ATX.i",
            units, user_time_constant, calib_file, em_prefix);
    printf("%s %s\n", program_name, cmd_line); fflush(stdout);
    if (run_system_command(program_name, cmd_line)) {
      exit(1);
    }
  } 
  sprintf(fname,"%s_3D_ATX.v",em_prefix);
  sprintf(im_prefix,"%s_3D_ATX", em_prefix);

 ecat_ready:
  // Smooth images for AIR alignment
  sprintf(fname,"%s_%dmm.v",im_prefix,default_smoothing);
  if (access(fname,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing point 6: existing %s\n",fname); fflush(log_fp);
  } else {
    if (brand_new_final_align) {
      // ahc no
      // Blurring here before motion vector calculation below
      sprintf(program_name, "%s/%s", program_path, prog_gsmooth);
      sprintf(cmd_line, "%s.v %d",
              im_prefix, default_smoothing);
      if (run_system_command(program_name, cmd_line)) {
        exit(1);
      }
    }
  }
  
  // Create Realigned images
  sprintf(fname,"%s_rsl.v", im_prefix);
  if (access(fname,R_OK) == 0 && overwrite==0) {
    fprintf(log_fp,"Reusing point 7: existing %s\n",fname);
  }  else   {
    for (frame=0; frame<num_frames; frame++)      {
      int frame1=frame+1;                  // ECAT 1-N counting
      if (frame_info[frame].em_align_flag==0) {
        // Copy frame
        sprintf(program_name, "%s/%s", program_path, prog_matcopy);
        sprintf(cmd_line,"-i %s.v,%d,1,1 -o %s,%d,1,1",
                im_prefix, frame1, fname, frame1); // ECAT 1-N counting
        if (run_system_command(program_name, cmd_line)) {
          exit(1);
        }
        continue;
      }

      // Align frame
      int thr = (int)(AIR_threshold*32767/100);
      // Unblock gsmooth_ps above if using ecat_alignlinear below
      if (brand_new_final_align){
        // ahc no
        sprintf(program_name, "%s/%s", program_path, prog_alignlinear);
        sprintf(cmd_line,"%s_%dmm.v,%d,1,1 %s_%dmm.v,%d,1,1 %s_fr%d.air -m 6 -t1 %d -t2 %d",
                im_prefix, default_smoothing, ref_frame+1, im_prefix, default_smoothing, frame1, im_prefix, frame, thr, thr);
        fprintf(log_fp,"%s\n",cmd_line);
        fflush(log_fp);
      } else {
        // ahc yes
        // Using previously extracted motion from 128 NA files (more stable)
        sprintf(program_name, "%s/%s", program_path, prog_make_air);
        sprintf(cmd_line,"-s %s.v,%d,1,1 -r %s.v,%d,1,1 -i %s_fr%d.air -o %s_fr%d.air",
                im_prefix, ref_frame+1, im_prefix, frame1, em_na, frame, im_prefix, frame);
      }
      if (run_system_command(program_name, cmd_line)) {
        exit(1);
      }
      sprintf(program_name, "%s/%s", program_path, prog_reslice);
      sprintf(cmd_line,"%s_fr%d.air %s,%d,1,1 -a %s.v,%d,1,1 -k -o",
              im_prefix, frame, fname, frame1, im_prefix, frame1);
      if (run_system_command(program_name, cmd_line)) {
        exit(1);
      }
    }
  }

  //Create Motion QC plot
  sprintf(fname,"%s_motion_qc.dat", em_prefix);
  if ((fp=fopen(fname,"wt")) == NULL) {
    fprintf(log_fp,"Error opening file %s\n", fname);
    return 1;
  }

  for (frame=start_frame; frame<=end_frame; frame++) {
    int x0 = frame_info[frame].start_time;
    int x1 = x0+frame_info[frame].duration-1;
    if (frame_info[frame].em_align_flag == 0) {
      fprintf(log_fp,"reference frame %d\n",frame);  fflush(log_fp);
      fprintf(fp,"%d %d 0 0 0 0\n",x0,x0);
      fprintf(fp,"%d %d 0 0 0 0\n",x1,x0);
      continue;
    }
      
    sprintf(cmd_line,"%s/%s -a %s_fr%d.air",
            program_path, prog_motion_distance,
            im_prefix, frame);
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
  sprintf(cmd_line,"cd %s; %s %s", em_dir, prog_gnuplot, plt_fname);
  fprintf(log_fp,"%s\n",cmd_line);
  fflush(log_fp);
  if (exec)
    system(cmd_line);

  fclose(log_fp);
  return 0;
}
