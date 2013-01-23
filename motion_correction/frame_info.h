/* File frame_info.h
  Author: Merence Sibomana
Creation date: 14-feb-2010
*/
#ifndef frame_info_h
#define frame_info_h

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifndef WIN32
typedef signed long long __int64;
#endif

#define MIN_FRAME_DURATION 30  // Default alignement start frame len in sec
#define DEF_REF_FRAME_DURATION 300 // Default reference frame len in sec

typedef struct _FrameInfo {
  int start_time; 
  int duration; 
  double trues;
  double randoms;
  int MAF_frame; // MAF destination frame number
  int em_align_flag;
  int tx_align_flag;
  float transformer[12]; // affine transformer to reference frame
} FrameInfo;

static int remove_blanks(char *txt)
{  // remove spaces: initialize i to first blank and j to replacement
  int i=0, j=0, n = strlen(txt);
  for (i=0; i<n; i++)
    if (isspace(txt[i])) break; // find first space
  for (j=i+1; j<n ;j++)
    if (!isspace(txt[j])) break; // find first replacement
  while (j<n) {
    if (!isspace(txt[j])) txt[i++] = txt[j];
    j++;
  }
  txt[i] = '\0'; 
  return i;
}

static const char * transformer_txt(float *t)
{
  static char txt[256];
  sprintf(txt,"%4.3g",t[0]);
  for (int i=1; i<12; i++)  sprintf(txt+strlen(txt),",%4.3g",t[i]);
  remove_blanks(txt);
  return txt;
}

typedef struct _VicraEntry {
  int fr;   // frame number 
  int   st, dt; // frame start time and duration
  float tx_m;        //Motion from TX [mm]
  float em_m;        //Motion from TX [mm]
  float q[4];  // quaternion vector
  float t[3];  // translation vector
  float rmse;
  short tx_align, em_align; // alignment flags
} VicraEntry;

static struct _VicraInfo {
  int ref_frame, maf_flag;
  std::vector<VicraEntry> em;
  VicraEntry tx;
} vicra_info;

#define INTERFILE_LINE_SIZE 256

//static float deadtime_constant = 9.34e-006f;
static float deadtime_constant = 6.67e-6f;  // Value for Hopkins HRRT

static std::vector<FrameInfo> frame_info;
static FrameInfo tx_info;

static int get_frame_info(int frame, const char *fname, int num_frames)
{
  char line[INTERFILE_LINE_SIZE];
  double d;
  FrameInfo fi;
  const char *key=NULL, *value=NULL;
  FILE *fp=fopen(fname,"rt");
  if (fp==NULL) {
    perror(fname);
    return 0;
  }
  if (frame_info.size() < (size_t)num_frames) 
    frame_info.resize((size_t)num_frames);
  memset(&fi,0,sizeof(fi));
  while (fgets(line,sizeof(line),fp) != NULL) {
    if ((key=strstr(line, "Total Net Trues")) != NULL) {
      if ((value = strstr(line, ":=")) == NULL) {
        printf("invalid line %s in file %s\n", line, fname);
        return 0;
      }
      if (sscanf(value+2,"%lg", &d) == 1) fi.trues = d;
    } else if ((key=strstr(line, "Total Randoms")) != NULL) {
      if ((value = strstr(line, ":=")) == NULL) {
        printf("invalid line %s in file %s\n", line, fname);
        return 0;
      }
      if (sscanf(value+2,"%lg", &d) == 1) fi.randoms =  d;
    } else if ((key=strstr(line, "image relative start time")) != NULL) {
      if ((value = strstr(line, ":=")) == NULL) {
        printf("invalid line %s in file %s\n", line, fname);
        return 0;
      }
      sscanf(value+2,"%d", &fi.start_time);
    } else if ((key=strstr(line, "image duration")) != NULL) {
        if ((value = strstr(line, ":=")) == NULL) {
        printf("invalid line %s in file %s\n", line, fname);
        return 0;
      }
      sscanf(value+2,"%d", &fi.duration);
    }
    if (frame==0) {
      if ((key=strstr(line, "Dead time constant ")) != NULL) {
        if ((value = strstr(line, ":=")) == NULL) {
          printf("invalid line %s in file %s\n", line, fname);
          return 0;
        }
        sscanf(value+2,"%g", &deadtime_constant);
      }
    }
  }
//  fi.NEC = fi.trues*fi.trues/(fi.trues+fi.randoms);
  fi.MAF_frame = frame;
  fi.tx_align_flag = 1;
  frame_info[frame] = fi;
  fclose(fp);
  return 1;
}

static int get_lber(const char *norm_file, float &lber)
{
  char line[INTERFILE_LINE_SIZE];
  char hdr_file[FILENAME_MAX];
  const char *key=NULL, *value=NULL;

  strcpy(hdr_file, norm_file); strcat(hdr_file,".hdr");
  FILE *fp=fopen(hdr_file,"rt");
  if (fp==NULL) {
    perror(hdr_file);
    return 0;
  }
  while (fgets(line,sizeof(line),fp) != NULL) {
    if ((key=strstr(line, "LBER")) != NULL) {
      if ((value = strstr(line, ":=")) == NULL) {
        printf("invalid line %s in file %s\n", line, hdr_file);
        return 0;
      }
      if (sscanf(value+2,"%g", &lber) == 1) {
        fclose(fp);
        return 1;
      }
    }
  }
  fclose(fp);
  return 0;
}

// short version format:
// "frame" "time" "duration" "qw" "qx" "qy" "qz" "tx" "ty" "tz" "Err"
// 0 0 15 0.95 0.15 0.2 -0.16 65.02 5.97 -199.08 0.06
// 1 15 15 0.95 0.15 0.2 -0.16 65.03 6.01 -199.04 0.07
// 2 30 15 0.95 0.15 0.2 -0.16 65.28 6.16 -198.64 0.14

static unsigned vicra_get_info(const char *vicra_file, FILE *log_fp)
{
  unsigned i, j;
  float st=0.0f, dt=0.0f; // read input times as float
  int tx_al=0, em_al=0; // tmp variables for tx_align and em_align flags
  const char *key=NULL;
  char line[256], *p=NULL;
  VicraEntry e;
  FILE *fp=NULL;
  int ref_frame = -1;
  float foo=0.0f;

  if (vicra_info.em.size()) return vicra_info.em.size();  // already loaded

  e.em_m = e.tx_m = 0.0f;
  e.em_align = e.tx_align = 0; // will be set later
  vicra_info.ref_frame = -1; // undefined
  vicra_info.maf_flag = 0; // default
  if ((fp=fopen(vicra_file,"rt"))==NULL) {
    fprintf(log_fp,"Error opening file %s\n", vicra_file);
    return 0;
  }
  if (fgets(line, sizeof(line), fp) != NULL) {
    // Decode header
    while (sscanf(line, "%d %d", &i, &j) != 2) {
      if ((key=strstr(line, "reference frame")) != NULL) {
        if ((p=strrchr(line,'=')) != NULL) 
          sscanf(p+1,"%d",&ref_frame);
      }
      if (fgets(line, sizeof(line), fp) == NULL) {
        fprintf(log_fp,"Error : invalid vicra info file %s, too short\n", vicra_file);
        fclose(fp);
        return 0;
      }
    }
    
    // Decode EM and TX
    // Test short format
    int wc = sscanf(line, "%d %g %g %g %g %g %g %g %g %g %g %g", &e.fr,&st,&dt,
      &e.q[0],&e.q[1],&e.q[2],&e.q[3],&e.t[0],&e.t[1],&e.t[2],&e.rmse, &foo);
    if (wc!=11) wc=15; // assume long format
    while (strlen(line)>0)
    {
      if (wc==11)  // short format
        i = sscanf(line, "%d %g %g %g %g %g %g %g %g %g %g", &e.fr,&st,&dt,
                   &e.q[0],&e.q[1],&e.q[2],&e.q[3],&e.t[0],&e.t[1],&e.t[2],&e.rmse);
      else
        i = sscanf(line, "%d %g %g %g %d %g %d %g %g %g %g %g %g %g %g", &e.fr,&st,&dt,&e.tx_m,&tx_al,
                 &e.em_m, &em_al, &e.q[0],&e.q[1],&e.q[2],&e.q[3],&e.t[0],&e.t[1],&e.t[2],&e.rmse);
      if (i!= wc) {
        fprintf(log_fp,"Error decoding line: %s\n", line);
        vicra_info.em.clear();
        fclose(fp);
        return 0;
      }
      if (dt<1.0f) {
        fprintf(log_fp,"Invalid frame duration %g,  line: %s\n", dt, line);
        return 0;
      }
      e.dt = (int)dt;
      e.st = (int)st;
      if (wc==15)
      {
        e.em_align = em_al==1? 1: 0;
        e.tx_align = tx_al==1? 1: 0;
      }
      if (e.fr>0 && (i=vicra_info.em.size())>0) { // Fix duration
        int d =  e.st - vicra_info.em[i-1].st;
        vicra_info.em[i-1].dt = d;
      }
      if (e.fr<0 && vicra_info.em.size()>0) memcpy(&vicra_info.tx, &e, sizeof(e));
      else {
        if (!vicra_info.maf_flag && e.fr != vicra_info.em.size()) {
          fprintf(log_fp,"set MAF flag line: %s\n", line);
          vicra_info.maf_flag = 1;
        }
        if (vicra_info.ref_frame<0 && e.fr==ref_frame) {
          vicra_info.ref_frame=vicra_info.em.size();
          e.em_align = 0;
        } else {
          e.em_align = 1;   // force alignment of all other frames 
        }
        vicra_info.em.push_back(e);
      }
      if (fgets(line, sizeof(line), fp) == NULL) line[0] = '\0'; // end of file
    }
  }
  fclose(fp);
  return vicra_info.em.size();
}


#endif
