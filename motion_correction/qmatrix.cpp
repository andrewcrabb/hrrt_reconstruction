/*
 QMatrix.cpp : Quaternion to Matrix Conversion
Authors:
  Merence Sibomana
  Sune H. Keller
  Oline V. Olesen
Creation date: 31-mar-2010
*/
#include "qmatrix.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/*
static float vicra2scanner_calibration[16] = {
   -0.024f, -1.0f,  0.005f,  159.68f,
   -1.0f,   0.024f, -0.011f, 276.76f,
   0.011f, -0.005f, -1.0f,  -142.9f,
    0.0f, 0.0f, 0.0f, 1.0f};
*/
static float vicra2scanner_calibration[16] = {
   -0.0183f, -0.9998f, 0.0048f, 158.3019f,
   -0.9997f, 0.0182f, -0.0173f, 275.6558f,
    0.0172f, -0.0051f, -0.9998f, -144.2073f,
    0.0f, 0.0f, 0.0f, 1.0f};

int vicra_calibration(const char *filename, FILE *log_fp)
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
            fprintf(log_fp, "Error decoding '%s'\n", line);
            err_flag++;
          }
        } else {
           fprintf(log_fp, "File '%s' has not enough data\n", filename);
          err_flag++;
        }
        j++;
      }
      fclose(fp);
    }
  }
  mat_print(m);
  if (!err_flag)
    memcpy(vicra2scanner_calibration, m->data, 16*sizeof(float));
  mat_free(m);
  return err_flag? 0:1;
}

int  q2matrix(const float *q, const float *t, Matrix out)
{  
      /*
    matrix = [(qw*qw+qx*qx-qy*qy-qz*qz) 2*(qx*qy-qw*qz)    2*(qx*qz+qw*qy) Tx 
           2*(qy*qx+qw*qz)    (qw*qw-qx*qx+qy*qy-qz*qz) 2*(qy*qz-qw*qx) Ty 
           2*(qz*qx-qw*qy)    2*(qz*qy+qw*qx) (qw*qw-qx*qx-qy*qy+qz*qz) Tz 
             0                     0                    0               1];
     */
  float *m = out->data;
  if (out->ncols!=4 || out->nrows!=4) return 0;
  m[0]=(q[0]*q[0]+q[1]*q[1]-q[2]*q[2]-q[3]*q[3]); m[1]= 2*(q[1]*q[2]-q[0]*q[3]); m[2]=2*(q[1]*q[3]+q[0]*q[2]);
  m[4]=2*(q[2]*q[1]+q[0]*q[3]); m[5]=(q[0]*q[0]-q[1]*q[1]+q[2]*q[2]-q[3]*q[3]); m[6]=2*(q[2]*q[3]-q[0]*q[1]);
  m[8]=2*(q[3]*q[1]-q[0]*q[2]); m[9]= 2*(q[3]*q[2]+q[0]*q[1]); m[10]=(q[0]*q[0]-q[1]*q[1]-q[2]*q[2]+q[3]*q[3]);
  m[12]=m[13]=m[14]=0.0f;
  m[3]=t[0]; m[7]=t[1]; m[11]=t[2]; m[15]=1.0f;
  return 1;
}


static Matrix v2s=NULL, v2s_inv=NULL; // vicra to scanner coordinates transformer and inverse
static Matrix t_tmp=NULL;               // tmp transformer
static Matrix ref_inv=NULL, t_inv=NULL;          // Reference and current transformer inverse
static Matrix tc=NULL;                // Combined transformer 

int vicra_align(Matrix ref, Matrix t, Matrix out)
{

  if (v2s == NULL) {
    v2s = mat_alloc(4,4);
    v2s_inv = mat_alloc(4,4);
//    t_inv = mat_alloc(4,4);
    ref_inv = mat_alloc(4,4);
    tc = mat_alloc(4,4);
    t_tmp = mat_alloc(4,4);
    memcpy(v2s->data, vicra2scanner_calibration, 16*sizeof(float));
    mat_invert(v2s_inv, v2s);
  }

  // alignment transformer tc= v2s.t.v2s_inv.ref_inv.t.v2s
  mat_invert(ref_inv, ref);
  // mat_print(ref_inv);
  matmpy(tc, t, v2s);          // tc=v2s.t
  // mat_print(tc);
  mat_copy(t_tmp,tc); matmpy(tc, ref_inv, t_tmp);  // a=a.ref_inv 
  // mat_print(tc);
  mat_copy(t_tmp,tc); matmpy(tc, v2s_inv, t_tmp);  // a=a.v2s_inv  
  mat_copy(out,tc);
  mat_print(out);
  return 1;
}

int vicra_align(const float *ref_q, const float *ref_t, 
                const float *in_q, const float *in_t, Matrix out)
{
  Matrix ref = mat_alloc(4,4); 
  Matrix in = mat_alloc(4,4);
  q2matrix(ref_q, ref_t, ref);
  q2matrix(in_q, in_t, in);
  int ret = vicra_align(ref, in,  out);
  mat_free(ref);
  mat_free(in);
  return ret;
}