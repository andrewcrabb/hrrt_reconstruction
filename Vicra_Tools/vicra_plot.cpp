/*
vicra_plot : create a postscript plot from vicra tracking data using gnuplot
Author: Merence Sibomana <sibomana@gmail.cm>
Creation date: 12-feb-2010
Modification history:
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "listmode_table.h"
#include <ecatx/matpkg.h>
#ifdef WIN32
#include <direct.h>
#include <windows.h>
#include <ecatx/getopt.h>
#define GNUPLOT "pgnuplot"
#define SEPARATOR '\\'
#define chdir _chdir
#define strdup _strdup
#define unlink _unlink
#else
#include <unistd.h>
#define GNUPLUT "gnuplot"
#define FILENAME_MAX 256
#define SEPARATOR '/'
#endif
#define MAX_FRAMES 256
#define XDIM 256
#define NPLANES 207
#define PIXEL_SIZE 1.21875f
#define PLANE_SEP 1.21875f
#include <vector>
#include <list>
using namespace std;
#define MOTION_DISTANCE 1

//Tools,Port 1,Frame,Face,State,Q0,Qx,Qy,Qz,Tx,Ty,Tz,Error,Port 2,Frame,Face,State,Q0,Qx,Qy,Qz,Tx,Ty,Tz,Error
static char line[1024];
typedef struct _TrackingEntry { int time; float q[4]; float t[4]; float e; } TrackingEntry;

inline float distance2 (float *x1, float *x2)
{
  return (float)
    ((x2[0]-x1[0])*(x2[0]-x1[0]) + 
    (x2[0]-x1[0])*(x2[0]-x1[0]) + 
    (x2[0]-x1[0])*(x2[0]-x1[0]));
}

int  find_entry(vector<TrackingEntry> &tstack, int time, float x, float y, float z, float e)
{
  int  ret=0;
  if (tstack.size() == 0){
    return -1;
  }
  float t[3], d2, tmp;
  t[0]=x; t[1]=y; t[2]=z;
  d2 = distance2(tstack[0].t, t);
  for (size_t i=1; i<tstack.size(); i++) {
    if (tstack[i].e < e && (tmp=distance2(tstack[i].t, t)) < d2) {
      ret = (int )i;
      d2 = tmp;
    }
  }
  return ret;
}

#ifdef MOTION_DISTANCE
static void q2m(float *q, float *t, float *m)
{  
      /*
    matrix = [(qw*qw+qx*qx-qy*qy-qz*qz) 2*(qx*qy-qw*qz)    2*(qx*qz+qw*qy) Tx 
           2*(qy*qx+qw*qz)    (qw*qw-qx*qx+qy*qy-qz*qz) 2*(qy*qz-qw*qx) Ty 
           2*(qz*qx-qw*qy)    2*(qz*qy+qw*qx) (qw*qw-qx*qx-qy*qy+qz*qz) Tz 
             0                     0                    0               1];
     */
  m[0]=(q[0]*q[0]+q[1]*q[1]-q[2]*q[2]-q[3]*q[3]); m[1]= 2*(q[1]*q[2]-q[0]*q[3]); m[2]=2*(q[1]*q[3]+q[0]*q[2]);
  m[4]=2*(q[2]*q[1]+q[0]*q[3]); m[5]=(q[0]*q[0]-q[1]*q[1]+q[2]*q[2]-q[3]*q[3]); m[6]=2*(q[2]*q[3]-q[0]*q[1]);
  m[8]=2*(q[3]*q[1]-q[0]*q[2]); m[9]= 2*(q[3]*q[2]+q[0]*q[1]); m[10]=(q[0]*q[0]-q[1]*q[1]-q[2]*q[2]+q[3]*q[3]);
  m[12]=m[13]=m[14]=0.0f;
  m[3]=t[0]; m[7]=t[1]; m[11]=t[2]; m[15]=1.0;
}

static float vicra2hrrt[16] = {
   -0.024f, -1.0f,  0.005f,  159.68f,
   -1.0f,   0.024f, -0.011f, 276.76f,
   0.011f, -0.005f, -1.0f,  -142.9f,
    0.0f, 0.0f, 0.0f, 1.0f};

static Matrix v2s=NULL, v2s_inv=NULL; // vicra to scanner coordinates transformer and inverse
static Matrix ref=NULL;               // Reference transformer
static Matrix tv=NULL, tv_inv=NULL;   // Current transformer and inverse
static Matrix tc=NULL;   // Combined transformer 

static float cx=XDIM*PIXEL_SIZE*0.5f;
static float cz=NPLANES*PLANE_SEP*0.5f;

int vicra_calibration(const char *filename)
{
  int err_flag=0, ret=0;
  char line[256];
  FILE *fp;
  Matrix m = mat_alloc(4,4);
  if (filename!=NULL) {
    if ((fp=fopen(filename,"rt")) != NULL) {
      int i=0, j=0;
      while(j<m->nrows-1 && !err_flag) {
        if ((fgets(line,sizeof(line), fp)) != NULL) {
          if (line[0] == '#') continue;
           // clean trailing spaces
          i=strlen(line)-1;
          while (i>0 && isspace(line[i])) line[i--]='\0';
          float *p = m->data + j*m->ncols;
          if ((ret=sscanf(line,"%g %g %g %g", p, p+1, p+2, p+3)) != 4) {
            printf("Error decoding '%s'\n", line);
            err_flag++;
          }
        } else {
          printf("File '%s' has not enough data\n", filename);
          err_flag++;
        }
        j++;
      }
      fclose(fp);
    }
  }
  mat_print(m);
  if (!err_flag)
    memcpy(vicra2hrrt, m->data, 16*sizeof(float));
  mat_free(m);
  return err_flag? 0:1;
}

static int motion_distance(TrackingEntry v, float *d)
{
  float x1[4], x2[4];
  x1[0] = cx;  // center
  x1[1] = cx-100.0f; // 10 cm from the center 
  x1[2] = cz; // center
  x1[3] = 1.0f;

  q2m(v.q, v.t, tv->data);
  mat_invert(tv_inv, tv);
  // alignment transformer t= v2s_inv.tv_inv.ref.v2s
  matmpy(tc, v2s_inv, tv_inv);          // a=v2s_inv.tv_inv
 //     mat_print(v2s_inv);
 //     mat_print(tv_inv);
 //  mat_print(tc);
  mat_copy(tv,tc); matmpy(tc, tv, ref);  // a=a.ref   
 // mat_print(tc);
  mat_copy(tv,tc); matmpy(tc, tv, v2s);  // a=a.v2s   
 // mat_print(tc);
  mat_apply(tc,x1,x2);
  d[0] = (x2[0]-x1[0]);
  d[1] = (x2[1]-x1[1]);
  d[2] = (x2[2]-x1[2]);
  d[3] = (float)(sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]));
  return 1;
}
#endif

int decode(int *frame,TrackingEntry &v)
{
  int i=0;
  char *p=strchr(line,','); // Tool
  if (p==NULL) return 0;
  if ((p=strchr(p+1,',')) == NULL) return 0; // port
  if (sscanf(p+1,"%d",frame) != 1) return 0;
  for (i=0; i<3; i++) {
    if ((p=strchr(p+1,',')) == NULL) return 0; // skip, Face,State,
  }
  if ((i=sscanf(p+1,"%g,%g,%g,%g",&v.q[0],&v.q[1],&v.q[2],&v.q[3])) != 4) return 0;
  for (i=0; i<4; i++) {
    if ((p=strchr(p+1,',')) == NULL) return 0; // skip, Q0,Qx,Qy,Qz
  }
  if ((i=sscanf(p+1,"%g,%g,%g,%g",&v.t[0],&v.t[1],&v.t[2],&v.e)) != 4) return 0;
  return 1;
}

int filecreation_time(char *fname, SYSTEMTIME *pst, float duration)
{
  FILETIME creation_t, modif_t, access_t, lt;
  HANDLE h;

  h = CreateFile (fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	if (h != INVALID_HANDLE_VALUE)  {
    if (GetFileTime(h, &creation_t, &access_t, &modif_t)!= NULL) {
      if (duration>0) 
        FileTimeToLocalFileTime(&modif_t, &lt); // use modif_t - duration
      else FileTimeToLocalFileTime(&creation_t, &lt); // use creation time
      if (FileTimeToSystemTime(&lt, pst))  {
        if (duration>0)  {
          int sec = (int)duration;
          int h = sec/3600;
          int m = (sec%3600)/60;
          pst->wHour -= sec/3600;
          pst->wMinute -= (sec%3600)/60;
          pst->wSecond -= sec%60;
          pst->wMilliseconds -= (int)(1000*(duration-sec));
        }
        printf("%04d/%02d/%02d %02d:%02d:%02d.%03d\n", 
          pst->wYear,pst->wMonth,pst->wDay,
          pst->wHour,pst->wMinute,pst->wSecond, pst->wMilliseconds);
        CloseHandle(h);
        return 1;
      }  else fprintf(stderr,"Error converting creation time\n");
    } else fprintf(stderr, "%s: Error GetFileTime()\n", fname);
    CloseHandle(h);
  } 
  return 0;
}
static void usage(const char *pgm)
{
  printf("%s: Build %s %s\n", pgm, __DATE__, __TIME__);
  printf("usage: %s vicra_tracking_file [-c calibration_file] [-a existing_file.plt] [-s step] [-f]\n", pgm);
  printf("      -A  Polaris to Scanner Alignment file (default internal)");
  printf("      -a  add gnuplot commands to existing_file.plt \n");
  printf("      -s  step in seconds (default=5)\n");
  printf("      -f  frame mode (default=no)\n");
  exit(1);
}

unsigned get_lm_frames( const char *frame_def, unsigned *duration)
{
  int nframes = 0, count=0;
  int multi_spec[2];
  char *dup = strdup(frame_def);
  char *p = strtok(dup,", ");
  while (p) {
    if (sscanf(p,"%d*%d", &multi_spec[0], &multi_spec[1]) == 2) {
      if (multi_spec[0]>0) {
        printf("%d sec frame  repeated %d\n",  multi_spec[0], multi_spec[1]);
        for (int count=0; count<multi_spec[1] && nframes<MAX_FRAMES; count++)
          duration[nframes++] = multi_spec[0];
      } else {
        printf("%s: Invalid frame specification '%s'\n",  frame_def, p);
        break;
      }
    }	else {
      if (sscanf(p,"%d", &count) == 1 && count>0) duration[nframes++] = count;
      else {
        printf("%s: Invalid frame specification '%s'\n",  frame_def, p);
        break;
      }
    }
    p = strtok(0,", ");
  }
  printf("number of frames: %d\n", nframes);
  free(dup);
  return nframes;
}

int main(int argc, char ** argv)
{
  char dat_file[FILENAME_MAX], frame_info[FILENAME_MAX];
  char plt_file[FILENAME_MAX], ps_file[FILENAME_MAX];
  char dir[FILENAME_MAX], em_fname[FILENAME_MAX];
  char cmd[FILENAME_MAX], *fname=NULL, *ext=NULL;
  const char *cal_file = NULL;
  FILE *fp1=NULL, *fp2=NULL;
  int frame;
  TrackingEntry v0, v, vs;
  float d[4];  // dx,dy,dz,d 
  int t0, frame_sec, start_frame, end_frame;
  const CTableEntry *em_entry=NULL;
  int em_time=0;
  float tx0, ty0, tz0; // average values at t0
  float tx_sec, ty_sec, tz_sec, error_sec; // average per sec values
  int c, count, line_number;
  int step = 5, freq=60;  // average frame every second vicra frequency is 60hz
  SYSTEMTIME st;
  vector<TrackingEntry> tvector, tstack;
  char *ds=NULL, *ts=NULL; // date and time stamps
  char *add_file=NULL;
  int i, frame_mode=1;
  unsigned j, num_frames=0, duration[MAX_FRAMES];

  if (argc < 2) usage(argv[0]);
  while ((c = getopt (argc-1, argv+1, "a:s:A:f")) != EOF) {
    switch (c) {
      case 'a':
        add_file = optarg;
        break;
      case 'A':
        cal_file = optarg;
        break;
      case 's' :
        sscanf(optarg,"%d",&step);
        break;
      case 'f' :
        frame_mode = 1;
        break;
    }
  }

  if (frame_mode) step = 1; // ignore step
  strcpy(dir, argv[1]);
  if ((fname = strrchr(dir,SEPARATOR)) == NULL) {
    fname = argv[1];
    strcpy(dir,".");
  } else *fname++ = '\0';
    
  if (cal_file !=NULL) {
    if (!vicra_calibration(cal_file)) return 1;
  }
  strcpy(dat_file,argv[1]);
  if ((ext=strrchr(dat_file,'.')) != NULL) strcpy(ext,"_plt.dat");
  else strcat(dat_file,"_plt.dat");
  strcpy(frame_info,dat_file);
  strcpy(frame_info+strlen(frame_info)-4,"_frame_info.txt");
  strcpy(plt_file, argv[1]);
  if ((ext=strrchr(plt_file,'.')) != NULL) strcpy(ext,".plt");
  else strcat(plt_file,".plt");
  strcpy(ps_file, argv[1]);
  if ((ext=strrchr(ps_file,'.')) != NULL) strcpy(ext,".ps");
  else strcat(ps_file,".ps");
  if ((fp1 = fopen(argv[1],"rt")) == NULL) {
    perror(argv[1]);
    return 1;
  }
  fgets(line,sizeof(line),fp1); // header
  line_number = 1;
  if ((ds=strstr(line, ":=")) != NULL) {
    ds += 2;
    if ((ts=strstr(line, ",")) != NULL)   *ts++ = '\0';
    ds = strdup(ds);
    ts = strdup(ts);
    fgets(line,sizeof(line),fp1); // header
    line_number++;
  }
  fgets(line,sizeof(line),fp1); // first data
  line_number++;

  v0.t[3] = v.t[3] =1.0f;
  if (decode(&frame,v0) == 0) {
    printf("Error decoding line %d: %s\n", line_number, line);
    return(1);
  }
  start_frame = frame;
  t0 = frame_sec = frame/freq;
  tx_sec=tx0= v0.t[0]; ty_sec=ty0=v0.t[1]; tz_sec=tz0=v0.t[2]; error_sec = v0.e;
  count = 1;
  while (fgets(line,sizeof(line),fp1) != NULL) {
    line_number++;
    if (decode(&frame,v) == 0) {
      printf("Error decoding line %d: %s\n", line_number, line);
      return(1);
    }
    if (frame_sec/step != frame/freq/step) { // out current frame
      if ((i=find_entry(tstack, frame_sec-t0, tx_sec/count-tx0,ty_sec/count-ty0,
        tz_sec/count-tz0,error_sec/count))<0) {
          printf("Internal error decoding line %d: %s\n", line_number, line);
          return 1;
      }
      vs = tstack[i];
      vs.time = frame_sec-t0;
/*    
      vs.t[0]=tx_sec/count-tx0;
      vs.t[1]=ty_sec/count-ty0;
      vs.t[2]=tz_sec/count-tz0;
      vs.e=error_sec/count;
*/
      tvector.push_back(vs);
      count=0;
      tstack.clear();
      tx_sec = ty_sec = tz_sec = error_sec = 0.0f;
      frame_sec = frame/freq;
    } 
    tx_sec += v.t[0]; ty_sec += v.t[1]; tz_sec += v.t[2]; error_sec += v.e;
    tstack.push_back(v);
    count++;
  }
  end_frame = frame;
  fclose(fp1); 
  memset(&st,0,sizeof(st));
  if (listmode_table_load(dir)) {
    time_t tt;
    
    if (ts != NULL) {
      int d,M,y,h,m,s;
      char *msec = strchr(ts,'.');
      if (msec!=NULL)  *msec++ = '\0'; // millisec not used
      sscanf(ds,"%d:%d:%d",&d,&M,&y);
      sscanf(ts,"%d:%d:%d",&h,&m,&s);
      tt = time_encode(y,M,d,h,m,s);
    } else {
      filecreation_time(fname, &st, (end_frame-start_frame)/60.0f);
      tt = time_encode(st.wYear,st.wMonth,st.wDay,
          st.wHour,st.wMinute,st.wSecond);
    }
    if ((em_entry = listmode_find(tt, frame_sec-t0)) != NULL) 
      em_time = (int)(em_entry->t-tt);
 }

  if ((fp2=fopen(dat_file,"wt")) == NULL) {
    perror(dat_file);
    return 1;
  }

  v2s = mat_alloc(4,4);
  v2s_inv = mat_alloc(4,4);
  ref = mat_alloc(4,4);
  tv = mat_alloc(4,4);
  tv_inv = mat_alloc(4,4); // vicra 2 scanner coordinates alignment
  tc = mat_alloc(4,4);     // combined transformer
  memcpy(v2s->data, vicra2hrrt, sizeof(vicra2hrrt));
  mat_invert(v2s_inv, v2s);
  q2m(v0.q, v0.t, ref->data);

  fprintf(fp2,"Time(sec),Tx,Ty,Tz,Error, dx, dy, dz, d\n");
  int n = tvector.size();
  memcpy(v0.t, tvector[2500].t, 4*sizeof(float));
  for (i=0; i<tvector.size(); i++) {
    v = tvector[i];
    motion_distance(v, d);
//    if (fabs(v.e) > 0.6 || d[3]>20) continue; 
    if (fabs(v.e) > 0.6 || d[3]>200) continue; 
    fprintf(fp2, "%d %g %g %g %g ", v.time-em_time, 
      v.t[0]-v0.t[0],v.t[1]-v0.t[1],v.t[2]-v0.t[2],v.e);
    fprintf(fp2, "%g %g %g %g\n", d[0],d[1],d[2],d[3]);
  }
  fclose(fp2);

  if (frame_mode && em_entry != NULL) {
    if ((num_frames=get_lm_frames(em_entry->frame_def, duration))>0) {
      int tstart=0, tend=0;
      i=j=0;
      v = tvector[i];
      if ((fp2=fopen(frame_info,"wt")) == NULL) {
        perror(frame_info);
        return 1;
      }
//      fprintf(fp2,"Time(sec),Tx,Ty,Tz,Error\n");
      fprintf(fp2,"Track file = %s\n", argv[1]);
      if (cal_file != NULL) fprintf(fp2,"calibration file = %s\n", cal_file);
      fprintf(fp2,"listmode file = %s\n", em_entry->fname);
      fprintf(fp2,"\n");
      fprintf(fp2,"Frame, start, duration, qw , qx , qy , qz , Tx , Ty , Tz, RMSE[mm]\n");
      while (v.t<0 && i+1<tvector.size()) v = tvector[++i];
      for (j=0; j<num_frames && i<tvector.size(); j++) {
        tend=tstart+duration[j];
        memset(&vs, 0, sizeof(vs));
        count = 0;
        while (v.time < tend && i<tvector.size()) {
          v = tvector[i++];
          if (fabs(v.e) > 0.6) continue; 
          vs.t[0] += v.t[0]; vs.t[1] += v.t[1]; vs.t[2] += v.t[2]; vs.e += v.e;
          count++;
        }
        if (count==0) break;
        vs.t[0] /= count; vs.t[1] /= count; vs.t[2] /= count; vs.e /= count;
        fprintf(fp2, "%d %d %d %g %g %g %g %g %g %g %g\n", j, tstart, duration[j],
          vs.q[0], vs.q[1], vs.q[2], vs.q[3], vs.t[0], vs.t[2], vs.t[2],vs.e);
//        fprintf(fp2, "%d, %d, %d\n", j, tstart+em_time,tend-tstart);
        tstart = tend;
      }
      fclose(fp2);
    }
  }

  if ((fp2=fopen(plt_file,"wt")) == NULL) {
    perror(plt_file);
    return 1;
  }
  if (add_file) {
    if ((fp1=fopen(add_file,"rt")) == NULL) {
      perror(add_file);
      return 1;
    }
    while (fgets(line,sizeof(line), fp1) != NULL)
      fputs(line, fp2);
    fclose(fp1);
    fprintf(fp2,"set size 1.0,0.45\n");
    fprintf(fp2,"set origin 0.,0.05\n");
  } else {
    fprintf(fp2,"set terminal postscript portrait color \"Helvetica\" 8\n");
    fprintf(fp2,"set output '%s'\n", ps_file);
    fprintf(fp2,"set multiplot\n");
    fprintf(fp2,"set size 1.0,0.45\n");
    fprintf(fp2,"set origin 0.,0.55\n");
    fprintf(fp2,"set grid\n");
  }
  fprintf(fp2,"set key left top\n");
  if (em_entry != NULL) {
    strcpy(em_fname, em_entry->fname);
    if ((ext=strstr(em_fname,".hdr")) != NULL) *ext = '\0';
    fprintf(fp2,"set title 'EM %s - Tracking %s'\n", em_fname, fname);
  } else {
    fprintf(fp2,"set title 'Tracking %s'\n", fname);
  }
  fprintf(fp2,"set xrange [%d:%d]\n", -((em_time+99)/100)*100,
    (frame_sec-t0-em_time+99)/100*100);
  fprintf(fp2,"plot '%s' using 1:2 title 'TX' with lines,\\\n", dat_file);
  fprintf(fp2,"     '%s' using 1:3 title 'TY' with lines,\\\n", dat_file);
  fprintf(fp2,"     '%s' using 1:4 title 'TZ' with lines,\\\n", dat_file);
  fprintf(fp2,"     '%s' using 1:5 title 'Error' with lines\n", dat_file);
 
  fprintf(fp2,"set title 'Displacement of a point 10cm from FOV center'\n");
  fprintf(fp2,"set origin 0.,0.\n");
  fprintf(fp2,"plot '%s' using 1:6 title 'dX' with lines,\\\n", dat_file);
  fprintf(fp2,"     '%s' using 1:7 title 'dY' with lines,\\\n", dat_file);
  fprintf(fp2,"     '%s' using 1:8 title 'dZ' with lines,\\\n", dat_file);
  fprintf(fp2,"     '%s' using 1:9 title 'd' with lines\n", dat_file);

  fclose(fp2);
  sprintf(cmd,"%s %s",GNUPLOT,plt_file);
  printf("%s\n",cmd);
  system(cmd);
  //unlink(dat_file);
  //unlink(plt_file);
}