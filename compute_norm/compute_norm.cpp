/*[ compute_norm.cpp
------------------------------------------------

   Name            : compute_norm.cpp - C++ implementation of original IDL compute_norm_s9
                   : and compute_norm_s3 utilities described below.
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 09/23/2003
   Modification history:
          08/23/2004 : User request: Write program version on stdout rather than stderr to include
                       the program ID in processing history.
          09/13/2004 : Threshold computed normalization 3 border bins to 1.0
          11/15/2004 : Threshold computed normalization specified (defaul=3) border bins to 1.0
          04/18/2005 : Divide norm by cosine(theta) rather than multiply to be compatible
                       with Forward Projection in OSEM-3D
                                        04/19/2005 : Removed hard coded build date (DSW)
               Copyright (C) CPS Innovations 2003 All Rights Reserved.

---------------------------------------------------------------------*

/
/*] END */

/*
;+
; NAME:
; compute_norm_s9
;   compute_norm_s3
;
; PURPOSE:
; This program computes a normalization matrix from the sinogram of a rotated
; rod source. The computed matrix can be multiplied with a sinogram to correct
; the sinogram for differences in efficiences between LOR's
;
; CATEGORY:
;       Scanner calibration.
;
; CALLING SEQUENCE:
;   compute_norm_s9,sinogram
;   compute_norm_s3,sinogram
;
; INPUTS:
; sinogram - a 3 dimensional integer 16 matrix containing the sinogram
;
; KEYWORD PARAMETERS:
;
; OUTPUTS:
; sinogram - a 3 dimensional float matrix containing the normalization
;
; HISTORY NOTES:
; MEC 2/99 - Wrote original
; CJM 10/02 - common information in model
;
;-
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "norm_globals.h"
#include "gantryinfo.h"
#include "my_spdlog.hpp"

using namespace cps;
static int model_number = 328;

#define NORM_HISTO_FILE "norm_histogram.dat"

#define MAX_INPUT_FILES 10
#define LINESIZE 1024
#define DIR_SEPARATOR '/'

static char line[LINESIZE];

const char *pgm_id = "V1.1 ";

// Error messages
static const char *malloc_error = "Memory allocation error";
static const char *read_error = "File read error";
static const char *write_error = "File write error";
static const char *fit_error = "Fit dwell failure";

// scanner_model_999 = {"999", 104, 256, 288, 1152, 234.5, 1.21875,1.21875, 0.};
int nelems = 256;
int nviews = 288;
int nrings = 104;
int span = 0;           // TBD : 3 or 9 wrt file size
int rd = GeometryInfo::MAX_RINGDIFF;
int nseg = 0;     // TBD: (2*rd+1)/span;

/*********************** Note ***************************************/
/*
 *  The term bucket (bkt), normaly used for ring scanners should be
 * replaced by head for panel detector scanner like the HRRT
 */
/********************************************************************/

int bkts_per_ring = 8;    // #heads
int blks_per_bkt = 9;   // Transaxial #blocks per head
float det_dia = 469.0f;   // Bill Jones uses 469 in the rebinner code
// default value updated from gm328.ini

typedef enum { LSO_LSO = 0, LSO_NAI = 1, LSO_ONLY = 2, LSO_GSO = 3, LSO_LYSO = 4 } HeadType;
float crys_depth[] = {10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0}; // in mm: All heads LSO/LYSO
// default value updated from gm328.ini

float blk_pitch = 19.5f;    // Block size in mm
float crys_pitch = 2.4003f;     // use tomographic pitch used for rebinning
float weights[] = {.8f, .2f}; // Layers statistics weights for LSO-LSO

float axial_pitch = 0.0f;        // use average axial pitch 19.5/8
float dist_to_bkt = 0.0f;    //det_dia/2.0f; should be the geometric radius, since DOI is added

int radial_comp = 4, angle_comp = 8, axial_comp = 9;
//compression factors for dwell fit

int crys_per_blk = 8;     // #crystals per block
int crys_per_bkt = 72;      // blks_per_bkt*crys_per_blk
int crys_per_ring = 576;    // crys_per_bkt*bkts_per_ring
int fan_size = 360;       // 5*crys_per_head:
// 1 head (bucket)is in coincidence with 5 opposite

int qc_flag = 1;

int border_nbin = 3;

static const char *fname(const char *path)  {
  const char *pos = strrchr(path, DIR_SEPARATOR);
  if (pos != NULL)
    pos = pos + 1;
  else
    pos = path;
  return pos;
}

/**
 * write_header:
 * Creates the header file "out_file.hdr" with the date, time and duration of each input file.
 */
static void write_header(char **in_data_fnames, int nfiles, char *out_data_fname) {
  char in_hdr_fname[FILENAME_MAX], out_hdr_fname[FILENAME_MAX];
  FILE *in_fp = NULL, *out_fp = NULL;
  sprintf(out_hdr_fname, "%s.hdr", out_data_fname);
  if ((out_fp = fopen(out_hdr_fname, "wb")) == NULL)
    LOG_ERROR(out_hdr_fname);
  if (out_fp == NULL)
    return;
  fprintf( out_fp, "!INTERFILE\n");
  fprintf( out_fp, "!originating system := HRRT\n");
  fprintf( out_fp, "name of data file := %s\n", fname(out_data_fname));
  for (int file_num = 0; file_num < nfiles; file_num++)
  {
    sprintf(in_hdr_fname, "%s.hdr", in_data_fnames[file_num]);
    if ((in_fp = fopen(in_hdr_fname, "rb")) == NULL)
    {
      LOG_ERROR(in_hdr_fname);
      continue;
    }
    fprintf(out_fp, ";input file := %s\n", in_data_fnames[file_num]);
    while (fgets(line, LINESIZE, in_fp) != NULL )
    {
      if (strstr(line, "study date") != NULL ||
          strstr(line, "image duration") != NULL ||
          strstr(line, "study time") != NULL ||
          strstr(line, "LM rebinner method") != NULL) fprintf(out_fp, "%s", line);
    }
    fclose(in_fp);
  }
  fprintf( out_fp, "data format := normalization\n");
  fprintf( out_fp, "number format := float\n");
  fprintf( out_fp, "number of dimensions := 3\n");
  fprintf( out_fp, "number of bytes per pixel := 4\n");
  fprintf( out_fp, "matrix size [1] := %d\n", nelems);
  fprintf( out_fp, "matrix size [2] := %d\n", nviews);
  int num_sinos = nseg * (2 * nrings - 1) - (nseg - 1) * (span * (nseg - 1) / 2 + 1);
  fprintf( out_fp, "matrix size [3] := %d\n", num_sinos);
  fprintf( out_fp, "scaling factor (mm/pixel) [1] := 1.218750\n");
  fprintf( out_fp, "scaling factor [2] := 1\n");
  fprintf( out_fp, "scaling factor (mm/pixel) [3] := 1.218750\n");
  fclose(out_fp);
}

/**
 * compute_norm :
 * Gets the size of the input(s) file(s) to determine the sinogram span.
 * Multiple input files must have the same size.
 * Logs error and exit if it can't open any input file or the files have not the same size.
 * Reads the segment 0 and calls fit_dwell to compute the parameters.
 * For each segment: computes the normalization using the fitted parameters,
 * updates the normalization histogram and writes the normalization segment.
 * The segment is the sum of corresponding segments when many input files are given.
 * Calls write_header to create the normalization header.
 * Logs error and exit when an error is encountered: (bad file size, file I/O error, memory exhausted, fit fail)
 */
static void compute_norm(char **in_files, int nfiles, char *out_file)
{
  struct stat st;
  FILE *in_fp[MAX_INPUT_FILES], *out_fp = NULL;
  int nplns0 = (2 * nrings - 1);  // number of direct planes
  int file_num = 0, pl = 0, nplns = nplns0;
  short *r1 = NULL, *r2 = NULL;
  short *sino = NULL, *tmp_sino = NULL;
  float *norm = NULL, *norm_view = NULL, *tmp_view = NULL;
  float *x_array = NULL, *dwell_array = NULL;
  int *z_array = NULL;
  int *sensitivity = NULL;
  int *histo = NULL, histo_size = 400; // norm values histogram array and size
  float *a_array = NULL; // parameters return from fit_dwell: float array[4,nplns0]
  float *ap = NULL, *dwellp = NULL;
  int seg = 0, iseg = 0;
  int err_flag = 0;
  FILE *dwell_fp = NULL;
  float *rdwell_c = NULL; // rotation dwell correction
  //
  //Check files size
  //
  nseg = (2 * rd + 1) / 3;
  int ntotal_3 = nseg * nplns0 - (nseg - 1) * (3 * (nseg - 1) / 2 + 1);
  nseg = (2 * rd + 1) / 9;
  int ntotal_9 = nseg * nplns0 - (nseg - 1) * (9 * (nseg - 1) / 2 + 1);
  if (stat(in_files[0], &st) == -1)
  {
    LOG_ERROR(in_files[0]);
    exit(1);
  }
  size_t data_size = st.st_size;
  if (data_size == (size_t)(nelems * nviews * ntotal_3 * sizeof(short)))
    span = 3;
  else if (data_size == (size_t)(nelems * nviews * ntotal_9 * sizeof(short)))
    span = 9;
  else
    LOG_EXIT("{} size= {} : not span3 or span9 size", in_files[0], (int)data_size);
  for (file_num = 1; file_num < nfiles; file_num++) {
    if (stat(in_files[file_num], &st) == -1) {
      LOG_EXIT(in_files[file_num]);
    }
    if (data_size != (size_t)st.st_size)     {
      LOG_EXIT("Input files should have the same size");
    }
    LOG_INFO("{}: size OK", in_files[file_num]);
  }
  nseg = (2 * rd + 1) / span;
  for (file_num = 0; file_num < nfiles; file_num++) {
    if ((in_fp[file_num] = fopen(in_files[file_num], "rb")) == NULL) {
      LOG_EXIT(in_files[file_num]);
    }
    LOG_INFO("{}: open OK", in_files[file_num]);
  }
  if ((out_fp = fopen(out_file, "wb")) == NULL) {
    LOG_EXIT(out_file);
  }

  //
  // Axial sensitivity stuff
  //
  int dmax = (span + 1) / 2;    // max number of axial LOR in a single plane
  int i = 0, j = 0, nrings2 = nrings * nrings;
  try {
    if ((r1 = (short*)calloc(nrings2, sizeof(short))) == NULL) throw(malloc_error);
    if ((r2 = (short*)calloc(nrings2, sizeof(short))) == NULL) throw(malloc_error);
    for (i = 0; i < nrings2; i++) {
      r1[i] = i % nrings;
      r2[i] = (i - r1[i]) / nrings;
    }
    int zoff = 0; //offset to the first plane of the segment
    //
    // Get segment 0
    //
    // IDL: st = assoc(ilun,intarr(nelems,nviews,nplns),0)
    int npixels = nelems * nviews;
    int nvoxels = nelems * nviews * nplns0;
    if ((sino = (short*)calloc(nvoxels, sizeof(short))) == NULL)
      throw(malloc_error);
    if (nfiles > 1) {
      if ((tmp_sino = (short*)calloc(nvoxels, sizeof(short))) == NULL)
        throw(malloc_error);
    }
    if ((norm = (float*)calloc(nvoxels, sizeof(float))) == NULL)
      throw(malloc_error);
    int seg = 0, iseg = 0, sign = 1, phi = 0;
    size_t offset = 0;
//    float norm_max = span==9? 20.0f : 10.0f;
//    why higherf threshold? span norm 9 values are lower than span 3
    float norm_max = 10.0f;
    float norm_min = 0.025f;
    LOG_INFO("keep only normalization between {} and {}", norm_min, norm_max);
    // We will need to correct for the trajectory of the rod,
    // so fit it an save parameters in array "a"
    if (fread(sino, npixels * sizeof(short), nplns, in_fp[0]) != (size_t)nplns)
      throw(read_error);
    for (file_num = 1; file_num < nfiles; file_num++) {
      if (fread(tmp_sino, npixels * sizeof(short), nplns, in_fp[file_num]) == (size_t)nplns) {
        for (i = 0; i < npixels * nplns; i++) sino[i] += tmp_sino[i];
      }
      else throw (read_error);
    }

    LOG_INFO("Fitting dwell ...");
    if ((a_array = fit_dwell(sino, nplns0)) == NULL)
      throw (fit_error);
    // output rotation dwell data
    if ((dwell_fp = fopen("rotation_dwell.txt", "wt")) != NULL)   {
      fprintf(dwell_fp, "plane, activity,rotation_radius(pixel),offset_amplitude(pixel),offset_direction(rad)\n");
      for (ap = a_array, pl = 0; pl < nplns0; pl++, ap += 4)
        fprintf(dwell_fp, "%d,%g,%g,%g,%g\n", pl, ap[0], ap[1], ap[2], ap[3]);
      fclose(dwell_fp);
    }
    LOG_INFO("done");

    // open rotation dwell sinogram file
    dwell_fp = fopen("rotation_dwell.s", "wb");

    if ((x_array     = (float*)calloc(nelems * nplns0, sizeof(float))) == NULL) throw(malloc_error);
    if ((z_array     = (int*)calloc(nelems * nplns0, sizeof(int))) == NULL) throw(malloc_error);
    if ((dwell_array = (float*)calloc(nelems * nplns0, sizeof(float))) == NULL) throw(malloc_error);
    if ((sensitivity = (int*)calloc(nplns0, sizeof(int))) == NULL) throw(malloc_error);
    if ((norm_view   = (float*)calloc(nelems * nplns0, sizeof(float))) == NULL) throw(malloc_error);
    if ((tmp_view    = (float*)calloc(nelems * nplns0, sizeof(float))) == NULL) throw(malloc_error);
    if ((rdwell_c    = (float*)calloc(nelems, sizeof(float))) == NULL) throw(malloc_error);
    if ((histo       = (int*)calloc(histo_size, sizeof(int))) == NULL) throw(malloc_error);
    float thres = 0.0f;
    // Now compute the normalization.
    while (iseg < nseg)    {
      seg = (iseg + 1) / 2;
      phi = span * seg * sign;
      // For each segment other than 0, find the number of planes in the segment
      // and read the segment.
      if (seg != 0 )       {
        nplns = 2 * nrings - span * (2 * seg - 1) - 2; //oblique segments are shorter
        if (fread(sino, npixels * sizeof(short), nplns, in_fp[0]) != (size_t)nplns)
          throw(read_error);
        for (file_num = 1; file_num < nfiles; file_num++)        {
          if (fread(tmp_sino, npixels * sizeof(short), nplns, in_fp[file_num]) == (size_t)nplns)           {
            for (i = 0; i < npixels * nplns; i++)
              sino[i] += tmp_sino[i];
          }
          else
            throw (read_error);
        }
        zoff =  (span * (2 * seg - 1) + 1) / 2;
      }
      // IDL: x=(findgen(nelems,nplns) mod nelems)-nelems/2     ; view (r,z), r centered
      // IDL: z=lindgen(nelems,nplns)/nelems
      // IDL: if (seg ne 0) z = z+zoff
      float x_center = -0.5f * nelems;
      i = 0;
      for (pl = 0; pl < nplns; pl++)
      {
        for (j = 0; j < nelems; j++)
        {
          x_array[i] = j + x_center;
          z_array[i] = zoff + nplns; // zoff==0 if seg=0
        }
      }
      //
      // axial sensitivity profile
      //
      // IDL: for pl=0,2*(nrings-1) do sensitivity(pl)=total((abs(r1-r2-phi) lt dmax) and ((r1+r2) eq pl))
      // IDL: norm_view = 1./( (intarr(nelems)+1) # sensitivity[zoff:zoff+nplns-1] )
      for (pl = 0; pl < nplns0; pl++)
      {
        int total = 0;
        for (i = 0; i < nrings2; i++)
        {
          if ((r1[i] + r2[i]) == pl)
          {
            int d = abs(r1[i] - r2[i] - phi);
            if (d < dmax) total++;
          }
          sensitivity[pl] = total;
        }
      }
      float *pn = norm_view;
      for (pl = 0; pl < nplns; pl++)
      {
        float factor = 1.0f / sensitivity[zoff + pl];
        for (i = 0; i < nelems; i++) *pn++ = factor;
      }
      // reset unused cells
      memset(norm_view + nplns * nelems, 0, (nplns0 - nplns)*nelems * sizeof(float));
      //
      // Find angle of LOR with respect to z axis
      //
      //    histo = indgen(400)
      double phi_r = atan(seg * span * axial_pitch / det_dia); // in radian
      LOG_INFO("Segment {:3d} @ phi {:6.4f} deg  NumPlanes: {} Offset: {}",  seg * sign, phi_r * 180 / M_PI, nplns, (int)offset);
      double angle = 0.0, d_angle = M_PI / nviews;
      int view_size = nelems * nplns;

      for (int view = 0; view < nviews; view++, angle += d_angle)      {
        // Make the dwell correction for the entire projection (view)
        float dwell = 0.0f, dwell_max = 0.0f;
        ap = a_array;
        dwellp = dwell_array;
        for (pl = 0; pl < nplns; pl++)        {
          float a1 = ap[1], a2 = ap[2], a3 = ap[3];
          float a1_sq = a1 * a1;
          float x = -0.5f * nelems;
          for (j = 0; j < nelems; j++, x++)          {
            double v2 = x  - a2 * cos(angle - a3);
            if ((dwell = a1_sq - (v2 * v2)) > 0.0f)            {
              dwell = (float)sqrt(dwell);
              if (dwell > dwell_max)
                dwell_max = dwell;
            }
            else
              dwell = 0.0f;
            *dwellp++ = dwell;
          }
          ap += 4;
        }

        if (seg == 0 && dwell_fp != NULL)         {
          // write segment 0 dwell correction sinogram
          for (pl = 0; pl < nplns; pl++)          {
            dwellp = dwell_array + pl * nelems;
            for (j = 0; j < nelems; j++)            {
              if (dwellp[j] > 0)
                rdwell_c[j] = 1.0f / dwellp[j];
              else
                rdwell_c[j] = 1.0f;
            }
            fwrite(rdwell_c, nelems, sizeof(float), dwell_fp);
          }
        }

        thres = 0.01f * dwell_max;
        ap = a_array;
        dwellp = dwell_array;
        for (pl = 0; pl < nplns; pl++)        {
          for (j = 0; j < nelems; j++)           {
            dwell = *dwellp;
            if (dwell < thres)
              dwell = thres;
            dwell = ap[0] / dwell;
            *dwellp++ = dwell;
          }
          ap += 4;
        }

        //
        // Preserve the zeros
        // Remove the axial sensitivity to calculate mask
        //
        // IDL: tmp_view = sinogram(*,view,*) * norm_view
        float *ptmp = tmp_view;
        double avg = 0;
        pn = norm_view;
        for (pl = 0; pl < nplns; pl++)
        {
          short *viewp = sino + npixels * pl + view * nelems;
          for (j = 0; j < nelems; j++)
          {
            *ptmp = (*viewp++) * (*pn++);
            avg += *ptmp;
            ptmp++;
          }
        }
        avg = avg / view_size;
        thres = (float)(0.05 * avg);
        // IDL: mask = (tmp_view gt (0.05*total(tmp_view)/(nelems*nplns)))
        // IDL: dwell = dwell*mask
        for (i = 0; i < view_size; i++)
          if (tmp_view[i] <= thres) dwell_array[i] = 0.0f;
        //
        // Then invert the data correcting for the angle through the cylinder
        // traced by the rod.
        // restore the axial sensitivity before histogramming
        //
        // IDL: tmp_view = cos(phi_r)*(dwell/(tmp_view > 1.))*norm_view
        double cos_phi = cos(phi_r);
        // To be compatible with Forward Projection in OSEM-3D,
        // Oblique projections are 1/cos(phi_r) larger and the normalized sinogram
        // of the rotated rod should follow that rule. The cosine factor should go in the
        // denominator rather than in numerator.
        for (i = 0; i < view_size; i++)
        {
          if (tmp_view[i] > 1.0f)
            tmp_view[i] = (float)((dwell_array[i] / tmp_view[i]) * norm_view[i]) / cos_phi;
          else tmp_view[i] = (float)(dwell_array[i] * norm_view[i]) / cos_phi;
        }
        //;*************************************************
        // IDL: h=histogram(tmp_view,min=0,max=20,bins=0.05)
        // IDL: xh=findgen(n_elements(h))*0.05
        // IDL: histo = histo+h
        for (i = 0, ptmp = tmp_view ; i < view_size; i++, ptmp++)
        {
          if (*ptmp > 0)
          {
            if ((j = (int)((*ptmp) * 20)) < histo_size) histo[j] += 1;
          }
        }

        // ;*************************************************
        // ;set all normalization coefficients higher/lower than norm_max/norm_min to 0.
        // IDL: w = where(tmp_view gt norm_max,nw)
        // IDL: if nw ne 0 then tmp_view(w) = 0.0
        // IDL: w = where(tmp_view lt norm_min,nw)
        // IDL: if nw ne 0 then tmp_view(w) = 0.0
        // sinogram(*,view,*) = tmp_view
        for (i = 0, ptmp = tmp_view ; i < view_size; i++, ptmp++)
        {
          j = i % nelems;
          if (j < border_nbin || nelems - j - 1 < border_nbin)
          {
            if ((*ptmp) > 1)
            {
              *ptmp = 1.0f;
            }
          }
          else
          {
            if ((*ptmp) < norm_min || (*ptmp) > norm_max)
              *ptmp = 0.0f;
          }
        }
        for (pl = 0; pl < nplns; pl++)
        {
          float *src = tmp_view + nelems * pl;
          float *dest = norm + npixels * pl + view * nelems;
          memcpy(dest, src, nelems * sizeof(float));
        }
      }
      if (fwrite(norm, npixels * sizeof(float), nplns, out_fp) != (size_t)nplns) throw(write_error);
      iseg = iseg + 1;
      sign *= -1;
      offset = ftell(in_fp[0]);
    }
  }

  catch (const char *err_message)
  {
    if (iseg > 0) fprintf(stderr, "segment %d: %s\n", seg, err_message);
    else fprintf(stderr, "%s\n", err_message);
    err_flag++;
  }
  if (!err_flag)
  {
    histo[0] = 0;  // background
    //
    // save normalization histogram in gnuplot format
    //
    if ((out_fp = fopen(NORM_HISTO_FILE, "wb")) != NULL)
    {
      fprintf(out_fp, "#HRRT normalization histogram\n");

      for (i = 0; i < histo_size; i++)
        fprintf(out_fp, "%g %d\n", 0.05f * i, histo[i]);
      fclose(out_fp);
    }
  }
  if (r1 != NULL) free(r1);
  if (r2 != NULL) free(r2);
  if (a_array != NULL) free(a_array);
  if (x_array != NULL) free(x_array);
  if (z_array != NULL) free(z_array);
  if (dwell_array != NULL) free(dwell_array);
  if (sensitivity != NULL) free(sensitivity);
  if (norm_view != NULL) free(norm_view);
  if (tmp_view != NULL) free(tmp_view);
  if (norm != NULL) free(norm);
  if (sino != NULL) free(sino);
  if (tmp_sino != NULL) free(tmp_sino);
  for (file_num = 0; file_num < nfiles; file_num++) fclose(in_fp[file_num]);
  if (dwell_fp != NULL) fclose(dwell_fp);
  return;
}


static void usage(char* pgm_name)
{
  LOG_INFO("%s %s Build %s %s\n", pgm_name, pgm_id, __DATE__, __TIME__);
  LOG_INFO("usage1: {} scan_file [norm_file] [-t num_bins] [-q]", pgm_name);
  LOG_INFO("usage2: {} scan_file1 [scan_file2 ... scan_filen] -o norm_file [-t num_bins][-q]", pgm_name);
  LOG_INFO("       -t num_bins sets specified radial border bins to 1 (default=3 bins)");
  LOG_INFO("       -q (quiet) option sets the quality check flag off");
  LOG_INFO("       span (3 or 9) is automatically determined w.r.t file size");
  exit(1);
}

int main(int argc, char **argv) {
  char out_file[FILENAME_MAX];
  char working_dir[FILENAME_MAX];
  char *input_files[MAX_INPUT_FILES];
  int input_count = 0;
  float ftmp = 0;
  int head_type = 0, i = 0, nheads = bkts_per_ring;
  float def_depth = 7.5f; // default LSO_GSO
  int uniform_flag = 1;

  my_spdlog::init_logging(argv[0]);

  out_file[0] = '\0';
  while ((input_count + 1) < argc && argv[input_count + 1][0] != '-')  {
    input_files[input_count] = argv[input_count + 1];
    input_count++;
    if (input_count == MAX_INPUT_FILES)
    {
      LOG_EXIT("Number of input files limited to {}", MAX_INPUT_FILES);
    }
  }

  if (input_count == 0)
    usage(argv[0]);
  int pos = input_count + 1;
  while (pos < argc)  {
    if (strncmp(argv[pos], "-o", 2) == 0)    {
      if (strlen(argv[pos]) > 2)      {
        strcpy(out_file, argv[pos] + 2); // -onormfile
      }      else       {
        if (++pos < argc)
          strcpy(out_file, argv[pos]);
        else
          usage(argv[0]);
      }

    }
    else if (strncmp(argv[pos], "-t", 2) == 0)    {
      if (strlen(argv[pos]) > 2)      {
        sscanf(argv[pos] + 2, "%d", &border_nbin); // -tnum_bins
      }      else       {
        if (++pos < argc)
          sscanf(argv[pos], "%d", &border_nbin);
        else
          usage(argv[0]);
      }
    }
    {
      if (strcmp(argv[pos], "-q") == 0) qc_flag = 0;
    }
    pos++;
  }

  if (out_file[0] == '\0')  {
    if (input_count != 1) usage(argv[0]); // no default
    strcpy(out_file, input_files[0]);
    char *p = strrchr(out_file, '.');
    if (p != NULL) strcpy(p, ".n");
    else strcat(out_file, ".n");
  }
  strcpy(working_dir, out_file);
  char *p = strrchr(working_dir, '\\');
  if (p != NULL)
  {
    *p = '\0';
    if (chdir(working_dir) != 0)
    {
      LOG_ERROR(working_dir);
      return 1;
    }
  }

  //
  // Update default parameters
  //
  if (GantryInfo::load(model_number) > 0) {
//    if (GantryInfo::get("interactionDepth", ftmp)) def_depth = 10*ftmp;
    if (GantryInfo::get("crystalRadius", ftmp))
    {
      det_dia = 2 * 10 * ftmp ; // in mm
      LOG_INFO("Crystal radius : {}", det_dia);
    }
    LOG_INFO("Crystal Depths (mm) :");
    for (i = 0; i < nheads; i++)    {
      if (!GantryInfo::get("headInfo[%d].type", i, head_type)) 
        break;
      if (head_type != LSO_LSO && head_type != LSO_GSO && head_type != LSO_LYSO) {
        LOG_ERROR("Invalid head {} type : {}", i, head_type);
        break;
      }
      if (head_type == LSO_LYSO) 
        crys_depth[i] = 10.0;
      else
       crys_depth[i] = 7.5; //LSO_LSO or LSO_GSO
      LOG_INFO(" {}", crys_depth[i]);
    }
    for (i = 1; i < nheads && uniform_flag; i++)
      if (crys_depth[i] != crys_depth[i - 1]) uniform_flag = 0;
    if (!uniform_flag)    {
      LOG_INFO("Using {} mm for all heads", def_depth);
      for (i = 0; i < nheads; i++) 
        crys_depth[i] = def_depth;
    }
  }

  //
  // initialize derived global parameters
  //
  axial_pitch = blk_pitch / crys_per_blk;
  dist_to_bkt = det_dia / 2.0f;
  crys_per_bkt = blks_per_bkt * crys_per_blk;
  crys_per_ring = crys_per_bkt * bkts_per_ring;
  fan_size = (bkts_per_ring - 3) * crys_per_bkt; // 1 head (bucket)is in coincidence with 5 opposite


  compute_norm(input_files, input_count, out_file);
  write_header(input_files, input_count, out_file);
  if (!qc_flag) return 0;
  //
  // generate gnuplot file
  //
  FILE *qc_fp = fopen("compute_norm_qc.plt", "wb");
  if (qc_fp == NULL)
  {
    LOG_ERROR("compute_norm_qc.plt");
    return 1;
  }
  fprintf(qc_fp, "set terminal postscript landscape color \"Helvetica\" 8\n");
  fprintf(qc_fp, "set output \"compute_norm_qc.ps\"\n");
  int pln = 0, nplns = (2 * nrings - 1) / axial_comp;
  //
  // Crystal efficiencies plot
  //
  float xoffset = 0.0f, yoffset = 0.8f;
  float xsize = 0.2f, ysize = 0.2f;
  fprintf(qc_fp, "set xrange [0:%d]\n", crys_per_ring - 1);
  fprintf(qc_fp, "set multiplot\n");
  fprintf(qc_fp, "set size %g,%g\n", xsize, ysize);
  for (pln = 1; pln <= nplns; pln++)
  {
    fprintf(qc_fp, "set title \"Plane %d\"\n", pln);
    fprintf(qc_fp, "set origin %g,%g\n", xoffset, yoffset);
    fprintf(qc_fp, "plot \"crys_eff.dat\" using %d notitle with lines\n\n", pln);
    xoffset += xsize;
    if (xoffset >= 1.0f) {
      xoffset = 0.0f;
      yoffset -= ysize;
    }
  }
  fprintf(qc_fp, "reset\n");
  //
  // view 0 profile and fit plot
  //
  xoffset = 0.0f, yoffset = 0.8f;
  fprintf(qc_fp, "set multiplot\n");
  fprintf(qc_fp, "set size %g,%g\n", xsize, ysize);
  fprintf(qc_fp, "set xrange [0:%d]\n", nelems / radial_comp - 1);
  fprintf(qc_fp, "set autoscale y\n");
  for (pln = 1; pln <= nplns; pln++)
  {
    fprintf(qc_fp, "set title \"Plane %d\"\n", pln);
    fprintf(qc_fp, "set origin %g,%g\n", xoffset, yoffset);
    if (pln == 1)
    {
      fprintf(qc_fp, "plot \"fit_sino_dwell.dat\" using %d title \"Profile\" with lines,\\\n", 2 * pln - 1);
      fprintf(qc_fp, "     \"fit_sino_dwell.dat\" using %d title \"Fit\" with lines\n\n", 2 * pln);
    }
    else
    {
      fprintf(qc_fp, "plot \"fit_sino_dwell.dat\" using %d notitle with lines,\\\n", 2 * pln - 1);
      fprintf(qc_fp, "     \"fit_sino_dwell.dat\" using %d notitle with lines\n\n", 2 * pln);
    }
    xoffset += xsize;
    if (xoffset >= 1.0f) {
      xoffset = 0.0f;
      yoffset -= ysize;
    }
  }
  fprintf(qc_fp, "reset\n");
  //
  // source activity, radius, offset amplitude and direction plot
  //
  xoffset = 0.0f, yoffset = 0.6f;
  fprintf(qc_fp, "set multiplot\n");
  xsize = ysize = 0.4f;
  fprintf(qc_fp, "set size %g,%g\n", xsize, ysize);
  fprintf(qc_fp, "set title \"Source Activity\"\n");
  fprintf(qc_fp, "set origin %g,%g\n", xoffset, yoffset);
  fprintf(qc_fp, "plot \"activity.dat\" using 1 notitle with lines,\\\n");
  fprintf(qc_fp, "     \"activity.dat\" using 2 notitle with lines\n\n");

  xoffset += xsize;
  fprintf(qc_fp, "set title \"radius\"\n");
  fprintf(qc_fp, "set yrange [0:150]\n");
  fprintf(qc_fp, "set origin %g,%g\n", xoffset, yoffset);
  fprintf(qc_fp, "plot \"radius.dat\" using 1 notitle with lines,\\\n");
  fprintf(qc_fp, "     \"radius.dat\" using 2 notitle with lines\n\n");

  xoffset = 0.0f;
  yoffset -= ysize;
  fprintf(qc_fp, "set title \"Offset amplitude\"\n");
  fprintf(qc_fp, "set origin %g,%g\n", xoffset, yoffset);
  fprintf(qc_fp, "set autoscale y\n");
  fprintf(qc_fp, "plot \"offset_amplitude.dat\" using 1 notitle with lines,\\\n");
  fprintf(qc_fp, "     \"offset_amplitude.dat\" using 2 notitle with lines\n\n");

  xoffset += xsize;
  fprintf(qc_fp, "set title \"Offset direction\"\n");
  fprintf(qc_fp, "set origin %g,%g\n", xoffset, yoffset);
  fprintf(qc_fp, "plot \"offset_direction.dat\" using 1 notitle with lines,\\\n");
  fprintf(qc_fp, "     \"offset_direction.dat\" using 2 notitle with lines\n\n");

  //
  // Normalization histogram
  //
  fprintf(qc_fp, "set nomultiplot\n");
  xsize = ysize = 0.8f;
  xoffset = 0.1f, yoffset = 0.1f;
  fprintf(qc_fp, "set size %g,%g\n", xsize, ysize);
  fprintf(qc_fp, "set title \"Normalization histogram\"\n");
  fprintf(qc_fp, "set origin %g,%g\n", xoffset, yoffset);
  fprintf(qc_fp, "set xrange [0:2.5]\n");
  fprintf(qc_fp, "plot \"norm_histogram.dat\" notitle with lines\n");
  fclose(qc_fp);
  return 0;
}

