// rotation_dwell_sino.cpp : Defines the entry point for the console application.
//

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
#include <gen_delays_lib/gen_delays_lib.hpp>
#include "rotation_dwell_sino.h"

#define MAX_LEN 256
#define NUM_PLANES 207
#define NPROJS 256
#define NVIEWS 288
#define AXIAL_REBIN 9
#define DWELL_MAX 100.0f

#define ROD_DIAMETER 10.0f // mm
// Conversion ftom Direct norm dwell to CBN dwell convention
// DN dwell in sino center = 1/128
// CBN dwell in sino center = 2*ROD_DIAMETER
float DN2CBN_scale = 2*ROD_DIAMETER*128.0f; 

static const char *r_file = "radius.dat";
static const char *off_file = "offset_amplitude.dat";
static const char *offd_file = "offset_direction.dat";

#define _MAX_PATH 256
#define SEPARATOR '/'

struct RotationParam
{
  float  r, off, offd; //radius, rotation offset and direction  
} param[NUM_PLANES/AXIAL_REBIN];

static const char *fpath(const char *f, const char *d)
{
  static char s[_MAX_PATH];
  sprintf(s,"%s%c%s", d,SEPARATOR,f);
  return s;
}

float *rotation_dwell_sino(const char* path, FILE *log_fp)
{
  int proj, view, plane; 
  float raw, r, r2, off, offd;
  float angle = 0.0, d_angle = (float)(M_PI/NVIEWS);
  float *sino=NULL, *dwellp = NULL, dwell=0;
  FILE *rin=NULL, *offin=NULL, *offdin=NULL;
  char line[MAX_LEN];
  FILE *log = (log_fp!=NULL)? log_fp:stderr;
  int err_flag=0;

  if (path==NULL || strlen(path)==0) return NULL; 

  if ((rin=fopen(fpath(r_file,path),"rt")) == NULL) perror(r_file);
  else if ((offin=fopen(fpath(off_file,path),"rt")) == NULL) perror(off_file);
  else if ((offdin=fopen(fpath(offd_file,path),"rt")) == NULL) perror(offd_file);
  if (rin!=NULL && offin!=NULL && offdin!=NULL )
  {
    fprintf(log,"reading %s, %s, %s\n", r_file, off_file, offd_file);
    // skip first line header
    fgets(line,sizeof(line), rin);
    fgets(line,sizeof(line), offin);
    fgets(line,sizeof(line), offdin);
    for (plane=0; plane<NUM_PLANES/AXIAL_REBIN && err_flag==0; plane++ )
    {
      if (fgets(line, sizeof(line), rin) == NULL ||
        sscanf(line, "%g\t%g", &raw, &param[plane].r) != 2)  // raw and fitted value
      {
        fprintf(log, "Error reading %s plane %d\n", r_file, plane);
        err_flag++;
      }
      if (fgets(line, sizeof(line), offin) == NULL ||
        sscanf(line, "%g\t%g", &raw, &param[plane].off) != 2)  // raw and fitted value
      {
        fprintf(log, "Error reading %s plane %d\n", off_file, plane);
        err_flag++;
      }
      if (fgets(line, sizeof(line), offdin) == NULL ||
        sscanf(line, "%g\t%g", &raw, &param[plane].offd) != 2)  // raw and fitted value
      {
        fprintf(log, "Error reading %s plane %d\n", offd_file, plane);
        err_flag++;
      }
    }
  } else err_flag++;
  if (rin != NULL) fclose(rin);
  if (offin != NULL) fclose(offin);
  if (offdin != NULL)fclose(offdin);

  if (err_flag) return NULL;
  if ((sino = (float*) calloc( NPROJS*NVIEWS, sizeof(float))) == NULL)
  {
    fprintf(log,"rotation_dwell_sino: memory allocation failed\n");
    return NULL;
   } 

  int mid_plane=NUM_PLANES/AXIAL_REBIN/2;
  r = param[mid_plane].r; r2 = r*r;
  off = param[mid_plane].off;
  offd = param[mid_plane].offd;
  dwellp = sino;
  for (view=0; view<NVIEWS; view++, angle += d_angle)
  {
    int x = -NPROJS/2;
    for (proj=0; proj<NPROJS; proj++, x++, dwellp++)
    {
      float rcor = x  - off*cos(angle-offd);
      if ((dwell = (r2 - (rcor*rcor))) > 0.0f)
      {
        *dwellp = DN2CBN_scale/(float)sqrt(dwell);
      }
      else *dwellp = 0.0f;
    }	
  }
  // Apply threshold
  for (int i=0; i<NPROJS*NVIEWS; i++)
    if (sino[i] > DWELL_MAX) sino[i]=DWELL_MAX;

	return sino;
}
