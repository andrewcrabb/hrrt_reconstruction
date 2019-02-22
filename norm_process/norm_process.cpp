/*
  norm_process
  Scope: HRRT only
  Derived from Windelays code
  Authors: Merence Sibomana, Peter Bloomfield, Claude Comtat
  Windelays Authors: Larry Byars, Judson Jones
  Creation 08/2005
  Purpose: norm_process creates a component norm (crystal efficiencies and geometric factors)
  and an expanded norm sinogram (span3 or span9) from a rotating rod listmode file

  Modification History:
  06/29/06: Add option to exclude LOR with border crystals
  Provide an option to create an expanded span3 or span9 norm sinogram from component norm.
  08/23/06: Compute gr and ga arrays instead of using pre-computed files
  08/12/07: Add fansum input and number of iterations options
  09/02/07: Use same code as lmhistogram to compute projection in CNThread()
  01/15/08: Remove test rd<maxrd_+6 b/c invalid for LR mode
  04/17/08: Option to set rod rotation radius r and center x0, y0
  05/26/08: Truncate high rotation dwell values to exclude corresponding LORs
  from fan sums to avoid rod misalignments as suggested by R. Carson
  09/10/08: Changes for 64bit support
  Change .ce and .fs filename to match the listmode file name
  09/26/08: Make normalization compatible with lmhistogram
  11/13/08: Set maximum ring difference in norm header
  04/30/09: Use gen_delays_lib C++ library for rebinning and remove dead code
  Add an option to use a measured rotation_dwell and omega sino
  Integrate Peter Bloomfield __linux__ support
  28-May-2009: Added new geometric profiles format using 4 linear segments cosine
  16-Sep-2009: Check input arguments and exit if arguments are invalid
  16-Oct-2009: Added fix_back_layer_ce() to solve ring artifact
  23-Nov-2009: Bug fix: potential crash in compute_fan_sum if end_of_file reached
  17-Jun-2010: Bug fixes in normalization header, e.g "number format := normalization "
*/

#define MAX_THREADS 8
#define _FILE_OFFSET_BITS 64

#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h" //support for stdout logging
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include <fmt/format.h>
#include <fmt/ostream.h>

#define _MAX_PATH 256
#define _MAX_DRIVE 0
#define _MAX_DIR 256
#define _MAX_EXT 256
#include <unistd.h>
#include <pthread.h>
#define _stricmp strcasecmp
#define _splitpath splitpath

#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
#include <gen_delays_lib/gen_delays_lib.hpp>

#include "cal_sen.h"
#include "fan_sum.h"
#include "get_gs.h"
#include "CHeader.hpp"
#include "MarkTime.h"
#include "rod_dwell.h"
#include "scanner_params.h"
#include "rotation_dwell_sino.h"
#include "histogram_mp.hpp"

extern float inter(float x1, float y1, float x2, float y2, float r, float *xi, float *yi);

//***** #define LOG_ERROR( args...) {LOG_ERROR( args); exit(1);}

int mpi_myrank = 0;
int mpi_nprocs = 0;
int rank0;

int cflag = 0;
int kflag = 0;
int span  = 9;
int verbose = 0;

int border = 0;

float *norm_data = NULL;
float **norm_data2 = NULL;
float *ce = NULL; // crystal efficiencies array
float *gr = NULL; // radial geometric factors array
short *cangle = NULL; // radial geometric factors array
float *ga = NULL; // axial geometric factors array

float *omega_sino = NULL; // NPROJS*NVIEWS array
float *rdwell_sino = NULL; // NPROJS*NVIEWS array
float omega2[NYCRYS];     // addittional solid angle

#define LOR_WEIGHT_LOW_THRESH 1E-5  // LOR Weight (ea*eb*gra*grb*gaa*gab) low threshold
// used to ignore too weak LORs

static float max_rotation_dwell = 100.0f;

// Normalization lower and higher limits
#define NORM_MIN 0.025f
#define NORM_MAX 10.0f

#define EV_BUFSIZE 1024*1024 // Events buffer size
#define XBLKS NXCRYS/8
#define YBLKS NYCRYS/8

static int mp_data_offset = 100;  // mp data saved as signed integer with offset

static double span9_d_tan_theta = 0;
static unsigned corrections = 0x1f ; // all corrections

int duration = 0;
int current_time = 0;
/*===============================*/

void compute_drate( float *srate, float tau, float *drate);
void estimate_srate( float *drate, float tau, float *srate);
void compute_initial_srate( float *drate, float tau, float *srate);
int errtotal( int *ich, float *srate, int nvals, float dt);
int compute_csings_from_drates( int ncrys, int *dcoins, float tau, float dt, float *srates);
/*===============================*/

int NumberOfProcessors;
int NumberOfThreads;

std::string g_logfile;
// extern std::shared_ptr<spdlog::logger> g_logger;


static const char *prog_id = "norm_process";
static const char *sw_version = "HRRT_U 1.1";

static float koln_lthick[8]   = {7.5, 7.5, 10.0, 7.5, 7.5, 7.5, 10.0, 7.5};
static float koln_irad  [8]   = {3.75, 3.75, 5.0, 3.75, 3.75, 3.75, 5.0, 3.75};

float normal_lthick[8] = {10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0};
float normal_irad  [8] = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0};

float *head_lthick = NULL;
float *head_irad   = NULL;

int mp_num(int ahead, int bhead) {
  // for (int i = 1; i < = GeometryInfo::NMPAIRS; i++)
  //   if (GeometryInfo::HRRT_MPAIRS[i][0] = = ahead && GeometryInfo::HRRT_MPAIRS[i][1] = = bhead) return i;
  // return 0;
  for (auto pair : GeometryInfo::NMPAIRS)
    if (pair[0] = = ahead && pair[1] = = bhead) 
      return i;
  return 0;
}


static char build_id[80];

static void usage(char *prog) {
  printf("%s: Version %s Build %s", prog, sw_version, build_id);
  printf("%s - process component-based normalization to sinogram format", prog);
  printf("usage: %s [lm_file|fs_file|ce_file]  -o norm_file [options]", prog);
  printf("    -v          : verbose");
  printf("    -L mode     : set Low Resolution mode (1: binsize=2mm, nbins = 160, nviews = 144, 2:binsize=2.4375)");
  printf("    -s span, rd  : set span and maxrd_ values [9, 67]");
  printf("    -R rdwell_file  : Rotation dwell correction sinogram (default 155, 0, 0)");
  printf("    -I max_iterations  : set maximum number of iterations (default=30)");
  printf("    -T max_rotation_dwell  : set maximum rotation dwell for LOR fansum(default = 100)");
  printf("    -M min_rmse : Minimum RMSE for convergence criteria (default = 10e-9)");
  printf("    -g geo_fname: radial and axial geometry filename (default %%GMINI%%\\gr_ga.dat");
  printf("    -d duration : Only process the specified scan duration for listmode input");
  printf("    -b      : ignore LOR with border crystals ");
  printf("    -c correction_bits: Obliqueness = 0x1, RotationDwell = 0x2, SolidAngle = 0x4, GR = 0x8, GA = 0x10");
  printf("                         Default=all corrections; example \"-c 7\" to exclude GR and GA");
  printf("    -k          : use Koln HRRT geometry (default to normal)");
  printf("    -l layer    : 0=back layer, 1=front layer, 2=all layers (default=2)");
  exit(1);
}

float omega_elem(int mp, int ax, int bx) {
  float omega = 0.0f;
  int idx = lor_sinogram::solution_[mp][NXCRYS + ax][NXCRYS + bx].nsino; //coincident head index
  if (idx >= 0) {
    omega = omega_sino[idx];
    if (abs(GeometryInfo::HRRT_MPAIRS[mp][0] - GeometryInfo::HRRT_MPAIRS[mp][1]) == 4) {
      // add omega2 for opposite MP
      // assume ax order 0...71 and bx order 71...0
      //omega *= omega2[abs(NXCRYS-bx-1-ax)];
    }
  }
  return omega;
}

float dwell_elem(int mp, int ax, int bx) {
  int idx = lor_sinogram::solution_[mp][NXCRYS + ax][NXCRYS + bx].nsino; //coincident head index
  if (idx >= 0) return rdwell_sino[idx];
  else return 0.0f;
}

static short *mp_read(const char *diamond_file) {
  int i = 0, count = 0;
  FILE *fptr = NULL;
  signed char *c_data = (signed char*)calloc(MP_SIZE, 1);
  short *mp_data = (short*)calloc(MP_SIZE, sizeof(short));
  if (c_data == NULL || mp_data == NULL) {
    LOG_EXIT("memory allocation");
  }

  if ((fptr = fopen( diamond_file, "rb")) != NULL) {
    //    printf("Reading %s", diamond_file);
    // read first part
    count = fread(c_data, sizeof(char), MP_SIZE, fptr);
    if (count != MP_SIZE) {
      LOG_EXIT("Error reading {}", diamond_file);
    }
    for (i = 0; i < count; i++)
      mp_data[i] = c_data[i] + mp_data_offset;
    int nbuffers = 1;
    //read extensions
    while ((count = fread(c_data, sizeof(char), MP_SIZE, fptr)) > 0)  {
      for (i = 0; i < count; i++)
        mp_data[i] += c_data[i] + mp_data_offset;
      nbuffers++;
    }
    int mp_min = mp_data[0];
    int mp_max = mp_data[0];
    double tot = mp_data[0];
    for (i = 1; i < MP_SIZE; i++) {
      if (mp_data[i] > mp_max)
        mp_max = mp_data[i];
      else if (mp_data[i] < mp_min)
        mp_min = mp_data[i];
      tot += mp_data[i];
    }
    LOG_INFO("Reading {} done; buffers = {} total, max, min= {}, {}, {}", diamond_file, nbuffers, tot, mp_max, mp_min);
  } else {
    LOG_EXIT(diamond_file);
  }
  free(c_data);
  return mp_data;
}

static short *mp_reads(const char *diamond_file) {
  int i = 0, count = 0;
  FILE *fptr = NULL;
  short *mp_data = (short*)calloc(MP_SIZE, sizeof(short));
  if ((fptr = fopen( diamond_file, "rb")) != NULL) {
    //    printf("Reading %s", diamond_file);
    // read first part
    count = fread(mp_data, sizeof(short), MP_SIZE, fptr);
    if (count != MP_SIZE) {
      LOG_EXIT("Error reading diamond_file {}", diamond_file);
    }
    int mp_min = mp_data[0];
    int mp_max = mp_data[0];
    double tot = mp_data[0];
    for (i = 1; i < MP_SIZE; i++) {
      if (mp_data[i] > mp_max)
        mp_max = mp_data[i];
      else if (mp_data[i] < mp_min)
        mp_min = mp_data[i];
      tot += mp_data[i];
    }
    LOG_INFO("Reading {} done; total, max, min= {}, {}, {}", diamond_file, tot, mp_max, mp_min);
  } else {
    LOG_EXIT(diamond_file);
  }
  return mp_data;
}

/*
  void init_corrections() computes solid angles for all LORs in transaxial dimension (i.e for a single ring)
  Only LORs with head0 are calculated, other LORs values are obtained by rotation
*/
void init_corrections() {
  int ax, bx, idx;
  int axx, bxx, alayer, blayer;
  float deta[3], detb[3], rd;
  float rdiam = (float)GeometryInfo::crystal_radius_ * 2.0f; //cm
  float inner_xi[2], inner_yi[2];  // LOR and inner circle intersection coordinates
  float outer_xi[2], outer_yi[2];  // LOR and outer circle intersection coordinates
  double a_sq, b_sq, c_sq, d_sq;             // distance sqwared between interection and detectors


  // create default dwell sino
  if (rdwell_sino == NULL) {
    LOG_INFO("using default rotation dwell");
    if ((rdwell_sino = (float*)calloc(GeometryInfo::nprojs_ * GeometryInfo::nviews_, sizeof(float))) == NULL) {
      LOG_EXIT("Can't allocate memory for dwell correction array");
    }
    for (int mp = 1; mp <= GeometryInfo::NMPAIRS; mp++) {
      for (ax = 0; ax < NXCRYS; ax++) {
        det_to_phy(GeometryInfo::HRRT_MPAIRS[mp][0], 1, ax, 0, deta);  // head 0, ring 0, front layer crystal position
        for (bx = 0; bx < NXCRYS; bx++) {
          det_to_phy(GeometryInfo::HRRT_MPAIRS[mp][1], 1, bx, 0, detb);  // ring 0, front layer crystal position
          rd = rod_dwell(deta[0], deta[1], detb[0], detb[1]);
          rd *= 10.0f; //cm to  mm
          for (alayer = 0; alayer < NLAYERS; alayer++)
            for (blayer = 0; blayer < NLAYERS; blayer++) {
              axx = NXCRYS * alayer + ax;
              bxx = NXCRYS * blayer + bx;
              idx = lor_sinogram::solution_[mp][axx][bxx].nsino; // front layer crystals
              if (idx >= 0)
                rdwell_sino[idx] = rd;
            }
        }
      }
    }
  } else {
    LOG_INFO("using measured rotation dwell");
  }

  // Create default omega
  if (omega_sino == NULL) {
    LOG_INFO("using default solid angle dwell");
    if ((omega_sino = (float*)calloc(GeometryInfo::nprojs_ * GeometryInfo::nviews_, sizeof(float))) == NULL) {
      LOG_EXIT("Can't allocate memory for LOR correction arrays");
    }

    float d0_sq = rdiam * rdiam, so = 0.0f;
    for (int mp = 1; mp <= GeometryInfo::NMPAIRS; mp++) {
      for (ax = 0; ax < NXCRYS; ax++) {
        det_to_phy(GeometryInfo::HRRT_MPAIRS[mp][0], 1, ax, 0, deta);  // head 0, ring 0, front layer crystal position
        deta[0] -= rotation_cx;
        deta[1] -= rotation_cy;
        for (bx = 0; bx < NXCRYS; bx++) {
          det_to_phy(GeometryInfo::HRRT_MPAIRS[mp][1], 1, bx, 0, detb);  // ring 0, front layer crystal position
          detb[0] -= rotation_cx;
          detb[1] -= rotation_cy;
          if (inter(deta[0], deta[1], detb[0], detb[1], rotation_radius + rod_radius, outer_xi, outer_yi) > 0) {
            if (inter(deta[0], deta[1], detb[0], detb[1], rotation_radius - rod_radius, inner_xi, inner_yi) > 0) {
              // LOR intersects both inner and outer rod diameter
              // use intersections with outer circle
              // Should use the mid-point between inner and outer intersections
              // Jeih-San@NIH reported that the difference was not significant
              a_sq = sq(deta[0] - outer_xi[0]) + sq(deta[1] - outer_yi[0]); //deta outer interscetion 0
              b_sq = sq(detb[0] - outer_xi[0]) + sq(detb[1] - outer_yi[0]); //detb outer interscetion 0
              c_sq = sq(deta[0] - outer_xi[1]) + sq(deta[1] - outer_yi[1]); //deta outer interscetion 1
              d_sq = sq(detb[0] - outer_xi[1]) + sq(detb[1] - outer_yi[1]); //detb outer interscetion 1
              so = (float)(0.5 * (sqrt(a_sq + b_sq) + sqrt(c_sq + d_sq)) / rdiam);
            } else {
              // LOR intersects only outer rod diameter
              // use  middle point
              a_sq = sq(deta[0] - 0.5 * (outer_xi[0] + outer_xi[1])) + sq(deta[1] - 0.5 * (outer_yi[0] + outer_yi[1]));
              b_sq = sq(detb[0] - 0.5 * (outer_xi[0] + outer_xi[1])) + sq(detb[1] - 0.5 * (outer_yi[0] + outer_yi[1]));
              so = (float)sqrt(a_sq + b_sq) / rdiam;
            }
          } else {
            so = 0.0;// LOR not intesecting rod
          }
          for (alayer = 0; alayer < NLAYERS; alayer++)
            for (blayer = 0; blayer < NLAYERS; blayer++) {
              axx = NXCRYS * alayer + ax;
              bxx = NXCRYS * blayer + bx;
              idx = lor_sinogram::solution_[mp][axx][bxx].nsino;
              if (idx >= 0)
                omega_sino[idx] = so * so;
            }
        }
      }
    }
  } else {
    LOG_INFO("using measured solid angle dwell");
  }

}

void init_omega2() {
  int N = 15;
  float a = 6.5;
  float scale = 1.0f / 9.0f; // gaussian function was determined from span9 segments
  float nfactor = 0.123f; // normalization factor
  for (int i = 0; i < 104; i++) {
    double x = a * (i * scale) / (N / 2);
    omega2[i] = 1.0f + (float)(nfactor * exp(-(x * x) / 2));
    //printf("gaussian(%d=%g)=%g", i, x, omega2[i]);
  }

}

struct CNArgs {
  int mp;
  int ahead;
  int bhead;
  int alayer;
  int blayer;
  int TNumber;
};

struct FS_MP_Args {
  const char *basename;
  int mp;
  int lc;
  int TNumber;
  float *out;
};

static struct CNArgs CN_args[MAX_THREADS];
static struct FS_MP_Args FS_MP_args[MAX_THREADS];
static struct FS_L64_Args FS_L64_args[MAX_THREADS];

/*
  Back layer crystal efficiencies for border analog boards.
  Affected crystals are x position 23, 24, 25, 47, 48, 49 corresponding to
  last crystal in blocks 2 and 3, and first 2 crystals in blocks 5 and 6
  The crystal efficiencies of these crystals are scaled to obtain the
  same average as crystals in similar positions to correct for this effects.
  1.Crystals x = 23, 47 are scaled to the the same average of other last block
  crystals (i, e: 7, 15, 31, 39, 55, 63, 71)
  2.Crystals x = 24, 48 are scaled to the the same average of other first block
  crystals (i, e: 0, 8, 16, 32, 40, 56, 64).
  3. Crystals x = 22, 46 are scaled to the the same average of other second block
  crystals (i, e: 6, 14, 30, 38, 54, 62, 70).

*/
static float sum[NLAYERS][NHEADS][NXCRYS];
/*********************/


/*---------------------------*/
/* The Compute Delays Thread */

void *CNThread2( void *arglist ) {
  struct CNArgs *args = (struct CNArgs *) arglist;
  int ax, ay, bx, by, mp, ahead, bhead, axx, bxx;
  int alayer, blayer, rd;
  int lc = 0;  // layer combination
  float ae, be; // crystals a, b efficiencies
  float gra = 1.0, grb = 1.0; // radial geometric a, b
  float ga = 1.0, ga_sq = 1.0; // axial geometric a, b and squared ga
  float omega = 1.0;    //solid angle correction
  int bys, bye; // crystal b y start and end
  float dz2[NYCRYS], seg, cay;
  int segnum, plane;
  lor_sinogram::SOL *sol;
  float *dptr = NULL;

  char TimeStr[ 32 ] ;
  memset( TimeStr, 0, 32 ) ;
  sprintf( TimeStr, "CNThread_%d", args->mp );
  StartTimer( TimeStr ) ;

  /*----*/
  mp = args->mp;
  ahead = GeometryInfo::HRRT_MPAIRS[mp][0];
  bhead = GeometryInfo::HRRT_MPAIRS[mp][1];

  //  printf("processing mp %d\t%d\t%d", args->mp, ahead, bhead);
  LOG_INFO("CNThread[{}]: ahead {} bhead {}", mp, ahead, bhead);

  for (alayer = 0; alayer < NLAYERS; alayer++) {
    // skip if not requested layer
    if (args->alayer < NLAYERS && alayer != args->alayer)
      continue;
    // loop and crystal a y
    for (ay = 0; ay < NYCRYS; ay++) {
      //  if (ay%10= = 0) printf("Generating delays for MP %d [H%d, H%d] %d\t%d \r",
      // mp, ahead, bhead, alayer, ay);
      cay = lor_sinogram::m_c_zpos2[ay];
      bys = 1000;
      bye = -1000;

      // precalculate segment info: get rd, dz2[by], bs, be
      for (by = 0; by < NYCRYS; by++) {
        dz2[by] = lor_sinogram::m_c_zpos[by] - lor_sinogram::m_c_zpos[ay]; // z diff. between det A and det B
        rd = ay - by;
        if (rd < 0)
          rd = by - ay;
        if (rd < maxrd_ + 6) {
          // dsaint31 : why 6??
          if (bys > by)
            bys = by; //start ring # of detB
          if (bye < by)
            bye = by; //end   ring # of detB
        }
      }

      // loop and crystal a x, ignore x border crystals
      axx = NXCRYS * alayer + border;
      for (ax = border; ax < NXCRYS - border; ax++, axx++) {
        ae =  * (ce_elem(ce, ax, alayer ? 0 : 1, ahead, ay)); // inverted layers

        // loop and crystal b layer
        for (blayer = 0; blayer < NLAYERS; blayer++) {
          // skip if not requested layer
          if (args->blayer < NLAYERS && blayer != args->blayer)
            continue;
          switch (alayer * 2 + blayer)
          {
          case 3: lc = 0; break; // FF
          case 2: lc = 1; break; // FB
          case 1: lc = 2; break; // BF
          case 0: lc = 3; break; // BB
          }

          // loop and crystal b x, ignore x border crystals
          bxx = NXCRYS * blayer + border;
          for (bx = border; bx < NXCRYS - border; bx++, bxx++) {
            if (lor_sinogram::solution_[mp][axx][bxx].nsino == -1) continue;
            sol = &lor_sinogram::solution_[mp][axx][bxx];
            dptr = norm_data2[sol->nsino];//result bin location
            if (corrections & GR_C) {
              gra =  * (gr_elem(ax, bx, bhead - ahead - 2, lc, 0));
              grb =  * (gr_elem(ax, bx, bhead - ahead - 2, lc, 1));
            }
            omega = omega_elem(mp, ax,  bx);

            // loop and crystal b y
            for (by = bys; by <= bye; by++) {
              be =  * (ce_elem(ce, bx, blayer ? 0 : 1, bhead, by)); // inverted layers
              if (corrections & GA_C) {
                ga =  * (ga_elem(bx, ay, by, bhead - ahead - 2, lc));
                ga_sq = ga * ga;
              }
              plane        = (int)(cay + sol->z * dz2[by]);
              seg = (float)(0.5 + dz2[by] * sol->d);
              segnum = (int)seg;
              if (seg < 0)
                segnum = 1 - (segnum << 1);
              else
                segnum = segnum << 1;
              if (lor_sinogram::m_segplane[segnum][plane] != -1 && omega > eps)
                dptr[lor_sinogram::m_segplane[segnum][plane]] += ae * be * gra * grb * ga_sq / omega;
              //                 dptr[lor_sinogram::m_segplane[segnum][plane]] += ae*be*gra*grb/omega;
            } // end by
          } // end bx
        }  // end blayer
      } // end ay
    } // end ax
  } // end alayer
  pthread_exit( NULL ) ;
  return 0 ;
}

/*---------------------------*/
/* The Compute Fan Sum from listmode Thread*/
#ifdef __linux__
void *FS_L64_Thread( void *arglist )
#else
unsigned int __stdcall FS_L64_Thread( void *arglist )
#endif
{
  struct FS_L64_Args *args = (struct FS_L64_Args *) arglist;
  fan_sum(args->in, args->count, args->out, corrections);
#ifdef __linux__
  pthread_exit( NULL ) ;
#else
  return 0 ;
#endif
}

/*
 * int check_end_of_frame(L64EventPacket &packet)
 * Locate end of frame and stop producing packets until all packets
 * are sorted and CH files are writen
 */

int check_end_of_frame(struct FS_L64_Args &src) {
  unsigned int ew1, ew2, type, tag;
  int pos = (src.count - 1) * 2;
  const unsigned *in_buf = src.in;
  int frame_end = duration * 1000; // in msec
  int last_time = 0;
  while (pos >= 0) {
    ew1 = in_buf[pos];
    ew2 = in_buf[pos + 1];
    type = HISTOGRAM_MP::ewtypes[(((ew2 & 0xc0000000) >> 30) | ((ew1 & 0xc0000000) >> 28))];
    if (type == 2) {
      // tag word
      tag = (ew1 & 0xffff) | ((ew2 & 0xffff) << 16);
      if ((tag & 0xE0000000) == 0x80000000) {
        current_time = (tag & 0x3fffffff); //msec
        current_time /= 1000; // in sec
        LOG_INFO("Time in sec : {}", current_time);
        if (current_time != last_time) {
          last_time = current_time;
          LOG_INFO("Time in sec : {}", current_time);
        }
        if (current_time > duration)
          return pos;
      }
    }
    pos -= 2;
  }
  return 0;
}

/*
  void compute_fan_sum(const char *l64_file, float *fan_sum)
  creates and starts Fan Sum thread (FS_L64_Thread) for 1M Events buffers.
  Thread use separate memory buffers and the thread buffers are added at the end into
  the fan_sum.
*/
static void compute_fan_sum(const char *l64_file, float *fan_sum) {
  // Threads output on different memory space because on different crystals
  int thread = 0, end_of_file = 0, end_of_frame = 0;
  int nthreads = 0; // Number of current threads may be less than NumberOfThreads
  int num_buf = 0;
#ifdef __linux__
  pthread_t ThreadHandles[ MAX_THREADS ] ;
  pthread_attr_t  attr ;
#else
  HANDLE ThreadHandles[MAX_THREADS];
#endif

  FILE *fp = fopen(l64_file, "rb");
  if (fp == NULL) {
    LOG_EXIT("Can't open listmode file {}", l64_file);
  }
  // Initialize thread arguments
  for (thread = 0; thread < NumberOfThreads; thread++) {
    // initialize allocated memory to 0 using calloc
    FS_L64_args[thread].in = (unsigned*)calloc(EV_BUFSIZE, 2 * sizeof(unsigned));
    FS_L64_args[thread].out = (float*)calloc(NUM_CRYSTALS, sizeof(float));
    if (FS_L64_args[thread].in == NULL || FS_L64_args[thread].out == NULL) {
      LOG_EXIT("Thread buffers memory allocation failed");
    }
    FS_L64_args[thread].count  = 0;
    FS_MP_args[thread].TNumber = thread;
  }

  //Feed the threads

  while (!end_of_file && !end_of_frame) {
#ifdef __linux__
    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif
    for (thread = 0; thread < NumberOfThreads && !end_of_file; thread++ )
    {
      FS_L64_args[thread].count = fread(FS_L64_args[thread].in, 2 * sizeof(unsigned), EV_BUFSIZE, fp);
      if (FS_L64_args[thread].count  <=  0) {
        end_of_file = 1;
        break;
      }
      if (num_buf++ > NumberOfThreads && duration > 0) {
        end_of_frame = check_end_of_frame(FS_L64_args[thread]);
        if (end_of_frame > 0)
          FS_L64_args[thread].count = end_of_frame;
      }
#ifdef __linux__
      pthread_create( &ThreadHandles[thread], &attr, &FS_L64_Thread, &FS_L64_args[thread] ) ;
      usleep( 1000 ) ;
#else
      ThreadHandles[thread] = (HANDLE)
                              _beginthreadex( NULL, 0, FS_L64_Thread, &FS_L64_args[thread], 0, NULL );
      Sleep(1);
#endif
    }
    nthreads = thread; //may be less than NumberOfThreads if end_of_frame of end_of_file
    //Wait for threads to finish
#ifdef __linux__
    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);
    for ( thread = 0 ; thread < nthreads ; thread++ )
    {
      pthread_join( ThreadHandles[thread] , NULL ) ;
    }
#else
    WaitForMultipleObjects(
      nthreads, ThreadHandles, TRUE, INFINITE );
#endif
  }

  // Sum buffers into fansum and clear memory
  for (thread = 0; thread < NumberOfThreads; thread++) {
    float *cur = FS_L64_args[thread].out;
    for (int i = 0; i < NUM_CRYSTALS; i++)
      fan_sum[i] += cur[i];
    free(FS_L64_args[thread].out);
    free(FS_L64_args[thread].in);
  }
}

//
// Some sinogram bins not filled in a direct sinogram may be used
// in oblique sinograms. These bins are filled with neighbor values
//
static void fill_sino(float *sino) {
  int nprojs = GeometryInfo::nprojs_, nviews = GeometryInfo::nviews_;
  float *temp = (float*)calloc(nprojs * nviews, sizeof(float));
  memcpy(temp, sino, nprojs * nviews * sizeof(float));
  for (int view = 1; view < nviews - 1; view++) {
    float *p = temp + view * nprojs + 1;
    float *dest = sino + view * nprojs + 1;
    for (int bin = 1; bin < nprojs - 1; bin++, p++, dest++)
    {
      if (*dest = = 0) {
        float neigb_sum = ( * (p - 1)) + ( * (p + 1)) + ( * (p - nprojs)) + ( * (p + nprojs));
        if (neigb_sum > 0) {
          if (( * (p - 1)) != 0)
            *dest =  * (p - 1);
          else if (( * (p + 1)) != 0)
            *dest =  * (p + 1);
          else if (( * (p - nprojs)) != 0)
            *dest =  * (p - nprojs);
          else if (( * (p + nprojs)) != 0)
            *dest =  * (p + nprojs);
        }
      }
    }
  }
  free(temp);
}

void create_omega_sino(float * &sino) {
  if (sino == NULL)
    sino = (float*)calloc(GeometryInfo::nprojs_ * GeometryInfo::nviews_, sizeof(float));
  else
    memset(sino, 0, GeometryInfo::nprojs_ * GeometryInfo::nviews_ * sizeof(float));

  for (int mp = 1; mp <= GeometryInfo::NMPAIRS; mp++) {
    for (int ax = 0; ax < NXCRYS; ax++) {
      int axx = NXCRYS + ax;   // front layer
      for (int bx = 0; bx < NXCRYS; bx++) {
        int bxx = NXCRYS + bx; // front layer
        int bin = lor_sinogram::solution_[mp][axx][bxx].nsino;
        if (bin >= 0)
          sino[bin] =  omega_elem(mp, ax, bx);
      }
    }
  }
  fill_sino(sino);
  FILE *fp = fopen("omega.s", "wb");
  if (fp != NULL) {
    fwrite(sino, GeometryInfo::nprojs_ * GeometryInfo::nviews_, sizeof(float), fp);
    fclose(fp);
  }
}

void create_dwell_sino(float * &sino) {
  if (sino == NULL) sino = (float*)calloc(GeometryInfo::nprojs_ * GeometryInfo::nviews_, sizeof(float));
  else memset(sino, 0, GeometryInfo::nprojs_ * GeometryInfo::nviews_ * sizeof(float));
  for (int mp = 1; mp <= GeometryInfo::NMPAIRS; mp++)  {
    for (int ax = 0; ax < NXCRYS; ax++) {
      int axx = NXCRYS + ax;   // front layer
      for (int bx = 0; bx < NXCRYS; bx++) {
        int bxx = NXCRYS + bx;   // front layer
        int bin = lor_sinogram::solution_[mp][axx][bxx].nsino;
        if (bin >= 0)
          sino[bin] =  dwell_elem(mp, ax, bx);
      }
    }
  }
  fill_sino(sino);
  FILE *fp = fopen("rotation_dwell.s", "wb");
  if (fp != NULL) {
    fwrite(sino, GeometryInfo::nprojs_ * GeometryInfo::nviews_, sizeof(float), fp);
    fclose(fp);
  }
}
#define GRGR
void create_gr_sino(int layer)
{
  char fname[80];
  int bin, view;
  float r, phi;
  float deta[3], detb[3];
  double dx, dy, d;
  float pi = (float)M_PI;
  float gra, grb;
  int nprojs = GeometryInfo::nprojs_, nviews = GeometryInfo::nviews_;

  float *sino = (float*)calloc(nprojs * nviews, sizeof(float));

  int lc = (layer = = 0) ? 3 : 0; // BB:FF

  for (int mp = 0; mp < GeometryInfo::NMPAIRS; mp++) {
    int ahead = GeometryInfo::HRRT_MPAIRS[mp + 1][0];
    int bhead = GeometryInfo::HRRT_MPAIRS[mp + 1][1];
    for (int ax = 0; ax < NXCRYS; ax++) {
      det_to_phy(ahead, 1, ax, layer, deta);  // ring 0, front layer crystal position
      for (int bx = 0; bx < NXCRYS; bx++) {
        det_to_phy(bhead, 1, bx, layer, detb);  // ring 0, front layer crystal position
        dy  = deta[1] - detb[1];
        dx  = deta[0] - detb[0];
        d   = sqrt( dx * dx + dy * dy);
        r   = (float)((deta[1] * detb[0] - deta[0] * detb[1]) / d);
        phi = (float)atan2( dx, dy);
        if (phi < 0.0) {
          phi += pi;
          r *= -1.0;
        }
        if (phi == pi) {
          phi = 0.0;
          r *= -1.0;
        }
        bin = (int)(nprojs / 2 + (r / GeometryInfo::binsize_ + 0.5)); // pro[0] is r
        view = (int)(nviews * phi / M_PI);          // pro[1] is phi
        if (bin >= 0 && bin < nprojs && view >= 0 && view < nviews) {
#ifdef GRGR
          gra =  * (gr_elem(ax, bx, bhead - ahead - 2, lc, 0));
          grb =  * (gr_elem(ax, bx, bhead - ahead - 2, lc, 1));
          sino[bin + nprojs * view] =  gra * grb;
#else
          gra =  * (cangle_elem(ax, bx, bhead - ahead - 2, lc, 0)) * 0.01f;
          grb =  * (cangle_elem(ax, bx, bhead - ahead - 2, lc, 1)) * 0.01f;
          //          if (gra*grb != 0) sino[bin + nprojs*view] =  fabs(gra/grb);
          sino[bin + nprojs * view] =  fabs(grb);
#endif
        }
      }
    }
  }
#ifdef GRGR
  sprintf(fname, "grgr_layer%d.n", layer);
#else
  //  sprintf(fname, "cacb_layer%d.n", layer);
  sprintf(fname, "cb_layer%d.n", layer);
#endif
  FILE *fp = fopen(fname, "wb");
  if (fp != NULL) {
    fwrite(sino, nprojs * nviews, sizeof(float), fp);
    fclose(fp);
  }
  fclose(fp);
  free(sino);
}

#ifdef __linux__
void splitpath(const char* path, char* drv, char* dir, char* name, char* ext) {
  /*
   * Copyright 2000, 2004 Martin Fuchs
   *
   * This library is free software; you can redistribute it and/or
   * modify it under the terms of the GNU Lesser General Public
   * License as published by the Free Software Foundation; either
   * version 2.1 of the License, or (at your option) any later version.
   *
   * This library is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   * Lesser General Public License for more details.
   *
   * You should have received a copy of the GNU Lesser General Public
   * License along with this library; if not, write to the Free Software
   * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
   */

  const char* end; /* end of processed string */
  const char* p;   /* search pointer */
  const char* s;   /* copy pointer */

  /* extract drive name */
  if (path[0] && path[1] = = ':') {
    if (drv) {
      *drv++ = *path++;
      *drv++ = *path++;
      *drv = '\0';
    }
  } else if (drv)
    *drv = '\0';

  /* search for end of string or stream separator */
  for (end = path; *end && *end != ':'; )
    end++;

  /* search for begin of file extension */
  for (p = end; p > path && *--p != '\\' && *p != '/'; )
    if (*p == '.') {
      end = p;
      break;
    }

  if (ext)
    for (s = end; (*ext = *s++); )
      ext++;

  /* search for end of directory name */
  for (p = end; p > path; )
    if (*--p == '\\' || *p == '/') {
      p++;
      break;
    }

  if (name) {
    for (s = p; s < end; )
      *name++ = *s++;

    *name = '\0';
  }

  if (dir) {
    for (s = path; s < p; )
      *dir++ = *s++;

    *dir = '\0';
  }
}
#endif

/*
  void fix_back_layer_ce()
  Sets back layer crystals X=21, 22, 24, 25 and X=45, 46, 48, 49 to average front / (front+back) ratio
  Set back layer crystals X=23 and X=47 to 80% of average front / (fron+back) ratio
*/

void fix_back_layer_ce()
{
  float *blk_ratio[NHEADS];
  int  head, x, y, k, xblk, yblk;
  float be, fe, r;  // back and front layer efficiencies

  blk_ratio[0] = (float*)calloc(XBLKS * YBLKS * NHEADS, sizeof(float));
  for (head = 1; head < NHEADS; head++)
    blk_ratio[head] = blk_ratio[head - 1] + XBLKS * YBLKS;

  for (head = 0; head < NHEADS; head++)
  {
    // compute block F, B ratio
    for (x = 0; x < NXCRYS; x++)
    {
      xblk = x / 8;
      for (y = 0; y < NYCRYS; y++)
      {
        yblk = y / 8;
        fe = *ce_elem(ce, x, 0, head, y);
        be = *ce_elem(ce, x, 1, head, y);
        blk_ratio[head][yblk * XBLKS + xblk] += fe / (be + fe);
      }
    }
    for (k = 0; k < YBLKS * XBLKS; k++)  blk_ratio[head][k] /= 64;
    // Update biased crystal efficiencies
    for (y = 0; y < NYCRYS; y++)
    {
      yblk = y / 8;
      for (x = 0; x <= NXCRYS; x++)
      {
        if ( (x >= 21 && x <= 25) || (x >= 45 && x <= 49))
        {
          xblk = x / 8;
          r =  blk_ratio[head][yblk * XBLKS + xblk];
          fe = *ce_elem(ce, x, 0, head, y);
          be = *ce_elem(ce, x, 1, head, y);
          *ce_elem(ce, x, 1, head, y) = (1.0f - r) * (fe + be);
          if (x = = 23 || x = = 47) * ce_elem(ce, x, 1, head, y) *= .8f;
        }
      }
    }
  }
}
/*------------------------------------------------------------------------*/
/*                              MAIN ROUTINE                              */
/*------------------------------------------------------------------------*/

int main(int argc, char **argv) {
  int nrings = NYCRYS;
  FILE *fptr = NULL;
  // , *log_fp = NULL;
  char *lm_file = NULL, *ce_file = NULL, *fs_file = NULL;
  char *geom_fname = NULL;
  char norm_file[_MAX_PATH], base_name[_MAX_PATH];
  int c, count = 0;

  char fname[_MAX_PATH], ofname[_MAX_PATH];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char ext[_MAX_EXT];

  //***** extern char *optarg;
  //***** extern int optind, opterr;

  int npixels = 0, nvoxels = 0, plane = 0, y = 0;
  int nprojs = 256, nviews = 288, nplanes = 0;
  short segnum = 0, nsegs = 0, *seg_planes = NULL;

  float *psino = NULL, *tmp_sino = NULL;
  float *cosphi = NULL;
  double sum = 0;
  float fval = 0.0f, avg = 0.0f;

  float pitch = 0.0f;
  float diam  = 0.0f;
  float thick = 0.0f;
  float scale = 1.0f;
  float calibration_scale = 1.0f;
  // scale span3, span9 and LR so that the same calibration can be used.
  int *coins = NULL;

  char *nprocstring = NULL;
  CHeader cheader;
  int layer = NLAYERS;
  char rdwell_path[_MAX_PATH], *omega_file = NULL;

  init_logging(argv[0]);

  //***** MPI_Init( &argc, &argv);
  //***** MPI_Comm_rank( MPI_COMM_WORLD, &mpi_myrank);
  //***** MPI_Comm_size( MPI_COMM_WORLD, &mpi_nprocs);

  mpi_myrank = 0;
  mpi_nprocs = 1;
  rank0 = (mpi_myrank == 0);

  sprintf(build_id, "%s %s", __DATE__, __TIME__);

  /*--------------------------------*/
  /* Process command-line arguments */

  if ( argc < 2 )
    usage(argv[0]);

  memset(rdwell_path, 0, sizeof(rdwell_path));
  // memset(rebinner_lut_file, 0, sizeof(rebinner_lut_file));
  _splitpath(argv[1], drive, dir, fname, ext);
  if (_stricmp(ext, ".l64") == 0) {
    lm_file = argv[1];
    if (strlen(drive) + strlen(dir) > 0)
      sprintf(rdwell_path, "%s%s", drive, dir);
    else
      strcpy(rdwell_path, ".");
  } else {
    if (_stricmp(ext, ".fs") == 0)
      fs_file = argv[1];
    else if (_stricmp(ext, ".ce") == 0)
      ce_file = argv[1];
    else
      usage(argv[0]);
  }

#ifndef CALSEN_V1
  sprintf(norm_file, "%s%s%s.n", drive, dir, fname); // default output filename
#else
  sprintf(norm_file, "%s%s%s_v2.n", drive, dir, fname); // default output filename
#endif
  if (argc > 2)
    while ((c = getopt( argc - 1, argv + 1, "v:p:s:g:G:o:O:k:c:d:L:l:R:T:br:")) != -1)
    {
      switch (c) {
      // -v set verbose mode
      case 'v':   verbose = 1; break;
      // -p nprojs, nviews - set sinogram size
      case 'p':
        if (sscanf( optarg, "%d, %d", &nprojs, &nviews) != 2)
          LOG_EXIT("invalid -p arguments");
        break;
      // -s span, maxrd_ - set 3D parameters
      case 's':
        if (sscanf( optarg, "%d, %d", &span, &maxrd_)  != 2)
          LOG_EXIT("invalid -s arguments");
        break;
      // -g geom_fname
      case 'g':   geom_fname = optarg; break;
      // -G pitch, diam, thick
      case 'G':
        if (sscanf( optarg, "%f, %f, %f", &pitch, &diam, &thick) != 3)
          LOG_EXIT("invalid -G arguments");
        break;
      // -o norm output file
      case 'o':  strcpy(norm_file, optarg); break;
      // specify the applied corrections
      case 'b': border = 1; break;
      case 'c':
        if (sscanf(optarg, "%d", &corrections) != 1) LOG_EXIT("invalid -c argument");
        break;
      // user Koln geometry
      case 'k':   kflag = 1; break;
      // -I max_iterations
      case 'I':
        if (sscanf(optarg, "%d", &max_iterations) != 1) LOG_EXIT("invalid -I argument");
        break;
      // -I max_iterations
      case 'M':
        if (sscanf(optarg, "%f", &min_rmse) != 1) LOG_EXIT("invalid -M argument");
        break;
      case 'L':   // low resolution
        if (sscanf( optarg, "%d", &GeometryInfo::lr_type_) != 1) LOG_EXIT("invalid -L argument");
        if (GeometryInfo::lr_type_ != LR_20 && GeometryInfo::lr_type_ != LR_24) {
          LOG_ERROR("Invalid LR mode {}", GeometryInfo::lr_type_);
          usage(argv[0]);
        }
        span = 7;
        maxrd_ = 38;
        nviews = 144;
        nrings = NYCRYS / 2;
        if (GeometryInfo::lr_type_ == LR_20) 
          nprojs = 160;
        else 
          nprojs = 128;
        break;
      case 'l':
        if (sscanf(optarg, "%d", &layer) != 1) 
          LOG_EXIT("invalid -l argument");
        if (layer < 0 || layer > NLAYERS) {
          LOG_ERROR("Invalid layer {}", layer);
          usage(argv[0]);
        }
        break;
      case 'd':
        if (sscanf(optarg, "%d", &duration) != 1) 
          LOG_ERROR("invalid -d argument");
        exit(1);
      case 'O':
        omega_file = optarg;
        break;
      case 'R':
        /*
          printf("Default Rotation radius=%g mm, center=(%g, %g)mm",
          rotation_radius, rotation_cx, rotation_cy);
          if (sscanf(optarg, "%f, %f, %f", &rotation_radius, &rotation_cx,
          &rotation_cy) != 3) usage(argv[0]);
          printf("Rotation radius=%g mm, center=(%g, %g)mm",
          rotation_radius, rotation_cx, rotation_cy);
        */
        strcpy(rdwell_path, optarg);
        break;
      case 'T':
        if (sscanf(optarg, "%f", &max_rotation_dwell) != 1) LOG_EXIT("invalid -T argument");
        break;
      // ahc
      // hrrt_rebinner.lut file now a required argument.
      // case 'r':
      //   strcpy(rebinner_lut_file, optarg);

        break;
      }
    }

  /* Program build date & time --> log file */
  LOG_INFO("{} build {} {}", argv[0], __DATE__, __TIME__ );
  if (ce_file != NULL) 
    LOG_INFO( "input crystal efficiencies file {}", ce_file);
  else 
    LOG_INFO( "input listmode file {}", lm_file);
  LOG_INFO( "output normalization file {}", norm_file);
  if (duration > 0) 
    LOG_INFO( "Used scan duration {}", duration);

  /*--------------------------------*/
  /* determine number of processors */

#ifdef __linux__
  NumberOfProcessors = 2;
#else
  nprocstring = getenv("NUMBER_OF_PROCESSORS");

  if ( nprocstring == NULL ) {
    LOG_INFO("Cannot determine the number of processors. Assuming 1.");
    NumberOfProcessors = 1;
  }
  else {
    if ( sscanf(nprocstring, "%d", &NumberOfProcessors) != 1 ) {
      LOG_ERROR("Error converting environment variable. Assuming nprocs = 1");
      NumberOfProcessors = 1;
    } else {
      LOG_INFO("Running on {} processors", NumberOfProcessors );
    }
  }
#endif

  NumberOfThreads = NumberOfProcessors;
  if ( NumberOfThreads > MAX_THREADS ) NumberOfThreads = MAX_THREADS;


  //InitializeCriticalSectionAndSpinCount
  StartTimer("Total Time");

  /*---------------------*/
  /* initialize geometry */

  LOG_INFO("Initializing geometry");

  head_lthick = normal_lthick;
  head_irad = normal_irad;

  GeometryInfo::init_geometry_hrrt(pitch, diam, thick);
  // convert mm to cm
  rotation_radius *= 0.1f; rod_radius *= 0.1f;
  rotation_cx *= 0.1f;
  rotation_cy *= 0.1f;

  SegmentInfo::init_segment_info(&SegmentInfo::m_nsegs, &nplanes, &m_d_tan_theta, maxrd_, span, NYCRYS,
                    GeometryInfo::crystal_radius_, GeometryInfo::plane_sep_);
  // if (strlen(rebinner_lut_file) == 0) {
  //   LOG_INFO( "Error: Rebinner LUT file not defined (opt 'r')");
  //   exit(1);
  // }
  // LOG_INFO( "Using Rebinner LUT file %s ", rebinner_lut_file);
  // lor_sinogram::init_lut_sol(rebinner_lut_file, SegmentInfo::m_segzoffset);
  lor_sinogram::init_lut_sol(SegmentInfo::m_segzoffset);

  npixels = nprojs * nviews;
  nvoxels = npixels * nplanes;

  // compute planes per segment
  if (span == 9) {
    nsegs = SegmentInfo::m_nsegs / 3;
    seg_planes = (short*)calloc(nsegs, sizeof(short));
    seg_planes[0] = 2 * NYCRYS - 1;
    for (int i = 1, int segnum = 3; i < nsegs; i += 2, segnum += 6) {
      seg_planes[i] = seg_planes[i + 1] = SegmentInfo::m_segzmax[segnum] - SegmentInfo::m_segz0[segnum] + 1;
    }
  } else {
    nsegs = SegmentInfo::m_nsegs;
    seg_planes = (short*)calloc(nsegs, sizeof(short));
    seg_planes[0] = 2 * NYCRYS - 1;
    for (int i = 1; i < nsegs; i += 2)  {
      seg_planes[i] = seg_planes[i + 1] = SegmentInfo::m_segzmax[i] - SegmentInfo::m_segz0[i] + 1;
    }
  }
  cosphi = (float*)calloc(nsegs, sizeof(float));
  for (int segnum = 0; segnum < nsegs; segnum++) {
    int seg = (segnum + 1) / 2;
    double phi_r = atan(seg * span * GeometryInfo::crystal_y_pitch_ / (2 * GeometryInfo::crystal_radius_)); // in radian
    cosphi[segnum] = (float)(1 / cos(phi_r));
  }

  //Read rotation Dwell Sino
  if (strlen(rdwell_path)) {
    LOG_INFO( "Using directory {} for rotation dwell parameter files", rdwell_path);
    if ((rdwell_sino = rotation_dwell_sino(rdwell_path, log_fp)) == NULL) {
      LOG_ERROR("Error creating rotation dwell sino from {}", rdwell_path);
      exit(1);
    }
  }

  //Read solid angle Sino
  if (omega_file != NULL) {
    if ((fptr = fopen(omega_file, "rb")) == NULL) {
      LOG_ERROR(omega_file);
      exit(1);
    }
    if ((omega_sino = (float*) calloc( npixels, sizeof(float))) == NULL)  {
      LOG_ERROR("memory allocation failed");
      exit(1);
    }
    if ((fread(omega_sino, sizeof(float), npixels, fptr)) != npixels) {
      LOG_ERROR("Error reading {}", omega_file);
      exit(1);
    }
  }

  init_omega2();
  init_corrections();

  // Truncate high dwell values
  for (int i = 0; i < npixels; i++) {
    if (rdwell_sino[i] > max_rotation_dwell)
      rdwell_sino[i] = max_rotation_dwell;
  }

#ifdef DEBUG
  FILE *fp;
  float *debug_sino = NULL;
  if (rdwell_sino && (fp = fopen("in2_rotation_dwell.s", "wb")) != NULL)
  {
    fwrite(rdwell_sino, 256 * 288, sizeof(float), fp);
    fclose(fp);
  }

  create_dwell_sino(debug_sino);
  create_omega_sino(debug_sino);
#endif

  LOG_INFO("Initializing geometry completed");

  /*-----------------*/
  /* allocate memory */
  LOG_INFO("Allocating memory");
  LOG_INFO("Allocating memory for crystal efficiencies ({} bytes)", NUM_CRYSTALS * sizeof(float));
  if ((ce = (float*) calloc( NUM_CRYSTALS, sizeof(float))) == NULL) {
    LOG_ERROR("memory allocation failed");
    exit(1);
  }

  LOG_INFO("Allocating memory for radial geometric factors ... (%d bytes)", GR_SIZE * sizeof(float));
  cangle = (short*) calloc( GR_SIZE, sizeof(short));
  if ((gr = (float*) calloc( GR_SIZE, sizeof(float))) == NULL) {
    LOG_ERROR( "memory allocation failed");
    exit(1);
  }
  LOG_INFO("Allocating memory for axial geometric factors ... (%d bytes)", GA_SIZE * sizeof(float));
  if ((ga = (float*) calloc( GA_SIZE, sizeof(float))) == NULL) {
    LOG_ERROR( "memory allocation failed");
    exit(1);
  }

  LOG_INFO("Allocating memory completed.");

  if (!get_gs(geom_fname, log_fp)) exit(1);

#ifdef DEBUG
  create_gr_sino(0);   // back layer
  create_gr_sino(1);   // front layer
#endif
  /*-----------------*/
  /* read input data */

  StartTimer("Read Data");

  if (ce_file != NULL) {
    // Crystal efficiencies input

    // Open original listmode header file
    _splitpath(ce_file, drive, dir, fname, ext);
    sprintf(fname, "%s%s%s.l64.hdr", drive, dir, fname);
    if (cheader.OpenFile(fname) != -1) cheader.CloseFile();

    LOG_INFO( "Reading Crystal efficiencies file {}", ce_file);
    if ((fptr = fopen( ce_file, "rb")) == NULL) {
      LOG_ERROR( "Can't open crystal efficiencies file {}", ce_file);
      exit(1);
    }
    int n = fread( ce, sizeof(float), NUM_CRYSTALS, fptr);
    fclose( fptr);
    if ( n != NUM_CRYSTALS ) {
      LOG_ERROR( "Only read {} of {} values from efficiencies file {}", n, NUM_CRYSTALS, ce_file);
      exit(1);
    }
  } else {
    // 64-bit lismode input
    float *fsum = (float*)calloc(NUM_CRYSTALS, sizeof(float));
    if (fsum == NULL) {
      LOG_ERROR("memory allocation");
      exit(1);
    }

    // Create .ce and .fs file using listmode file
    _splitpath(lm_file, drive, dir, fname, ext);
    _splitpath(norm_file, drive, dir, ofname, ext);
    compute_fan_sum(lm_file, fsum);

    sprintf(base_name, "%s%s%s", drive, dir, fname);
    sprintf(fname, "%s.fs", base_name);
    if ((fptr = fopen(fname, "wb+")) != NULL) {
      fwrite(fsum, NUM_CRYSTALS, sizeof(float), fptr);
      fclose(fptr);
    } else {
      LOG_ERROR( "Can't create crystal efficiencies output file {}", fname);
    }

    sprintf(fname, "%s.hdr", lm_file);
    // Copy listmode header to output directory
    if (cheader.OpenFile(fname) != -1) {
      LOG_INFO( "Copy {} to {}{}", fname, drive, dir);
      sprintf(fname, "%s.l64.hdr", base_name);
      cheader.CloseFile();
      cheader.WriteFile(fname);
    }

    // calculate crystal efficienies from fansum and save to file
    // Crystal efficiencies computed for the border of the heads are unreliable
    // They should be ingored when reconstructing computing the norm sinogram

    if (!cal_sen(fsum, ce, log_fp))
      exit(1);
    sprintf(fname, "%s.ce", base_name);
    if ((fptr = fopen(fname, "wb+")) != NULL) {
      fwrite(ce, NUM_CRYSTALS, sizeof(float), fptr);
      fclose(fptr);
    } else {
      LOG_ERROR( "Can't create crystal efficiencies output file {}", fname);
    }

    //free allocated memory for fansum
    free(fsum);
  }

  StopTimer("Read Data");

  fix_back_layer_ce();

  if (GeometryInfo::lr_type_ == 0) {
    calibration_scale = 9.0f / span; // 3 for span 3 and 1.0 for span9
    LOG_INFO( "Normalization span = {}, max_rd = {}, calibration scale = {}", span, maxrd_, calibration_scale);
  } else {
    calibration_scale = (float)((9.0f / 14.0f) * (0.5f * GeometryInfo::crystal_x_pitch_ / GeometryInfo::binsize_) / 4);
    // 14: High resolution equivalent span
    // 4: 2 times fewer angles and planes
    LOG_INFO( "LR normalization: span = {}, max_rd = {}, calibration scale = {}", span, maxrd_, calibration_scale);
    LOG_INFO( "                  nprojs = {}, nviews = {}, bin_size {} mm", nprojs, nviews, GeometryInfo::binsize_);
  }
  fflush(log_fp);

#ifndef __linux__
  StopTimer("Total Time");
  PrintTimerList();
  StartTimer("Total Time");
#endif

  /*----------------*/
  /* Compute normalization sinogram */

  //initialization and memory allocation
  StartTimer("Compute Normalization");
  LOG_INFO("Allocating memory for normalization ({} bytes)", nvoxels * sizeof(float));
  // Use calloc to initialize the values to 0
  if ((norm_data = (float *) calloc( nvoxels, sizeof(float) )) == NULL) {
    LOG_ERROR( "memory allocation failed");
    exit(1);
  }

  // Use calloc to initialize the values to 0
  if ((norm_data2 = (float **) calloc( npixels, sizeof(float*) )) == NULL) {
    LOG_ERROR( "memory allocation failed");
    exit(1);
  }
  for (int i = 0; i < npixels; i++)
    norm_data2[i] =  norm_data + i * nplanes;

  if ((tmp_sino = (float *) calloc(npixels, sizeof(float) )) == NULL) {
    LOG_ERROR( "memory allocation failed");
    exit(1);
  }

  // compute the sinogram using all LORs
  // A timer is set for computing each Module Pair
  unsigned int threadID;
  //create array of handles to threads

#ifdef __linux__
  pthread_t ThreadHandles[ GeometryInfo::NMPAIRS ] ;
  pthread_attr_t  attr ;
#else
  HANDLE *threadhandles = (HANDLE *) malloc( GeometryInfo::NMPAIRS * sizeof( HANDLE ) );
  if ( threadhandles == NULL )  
    LOG_ERROR("BP: ERROR malloc threadhandles");
#endif

  struct CNArgs *CNargs = (struct CNArgs *) malloc( GeometryInfo::NMPAIRS * sizeof( struct CNArgs) );;
  if ( CNargs == NULL )
    LOG_ERROR("BP: ERROR malloc CNargs");

#ifdef __linux__
  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif
  for (int mp = 0; mp < GeometryInfo::NMPAIRS; mp++) {
    CNargs[mp].alayer = CNargs[mp].blayer = layer;
    CNargs[mp].mp = mp + 1;
    CNargs[mp].TNumber = mp;
#ifdef __linux__
    pthread_create( &ThreadHandles[mp], &attr, &CNThread2, &CNargs[mp] ) ;
#else
    threadhandles[mp] = (HANDLE) _beginthreadex( NULL, 0, CNThread2, &CNargs[mp], 0, &threadID );
#endif
  }
#ifdef __linux__
  /* Free attribute and wait for the other threads */
  pthread_attr_destroy(&attr);
#endif
  for (int mp = 0; mp < GeometryInfo::NMPAIRS; mp++) {
#ifdef __linux__
    pthread_join( ThreadHandles[mp] , NULL ) ;
#else
    WaitForSingleObject( threadhandles[mp], INFINITE );
#endif
  }

  StopTimer("Compute Normalization");
#ifndef __linux__
  free( threadhandles );
  PrintTimerList();
#endif

  // Computing sinogram complete

  // compute norm factors histogram min = 0, max = 20, bins = 0.001
  // Normalize norm factors so that the average in segment 0 is 1.0
  StartTimer("Normalization Rescale");

  // scale by calibration_scale and invert sinogram
  for (int i = 0, psino = norm_data; i < nvoxels; i++, psino++)
    if (*psino > 0.0f) *psino = 1.0f / (*psino);

  // Find seg0 average
  sum = 0;
  count = 0;
  for (int i = 0; i < npixels; i++, psino++) {
    for (int plane = 0, psino = norm_data2[i]; plane < seg_planes[0]; plane++, psino++) {
      // remove outliers before rescaling
      if (*psino > 0.0f && *psino < NORM_MAX) {
        sum += *psino;
        count++;
      }
    }
  }

  // scale norm
  if (count > 0) {
    float norm_avg = (float)(sum / count);
    float norm_min = NORM_MIN * calibration_scale;
    float norm_max = NORM_MAX * calibration_scale;
    LOG_INFO( "segment 0 normalization average(excluding gaps): {}", norm_avg);
    scale = calibration_scale / norm_avg;
    LOG_INFO( "Rescale Norm with {} ({}/{}) and threshold to min = {}, max = {}",
            scale, calibration_scale, norm_avg, norm_min, norm_max);
    for (int i = 0, psino = norm_data; i < nvoxels; i++) {
      if (*psino > 0.0f) {
        norm_data[i] *= scale;
        if (norm_data[i] < norm_min || norm_data[i] > norm_max) 
          norm_data[i] = 0.0;
      }
    }
  }

  // Apply inverse cosine correction to be compatible with Forward Projection in OSEM-3D
  // and omega2 solid angle correction
  for (int i = 0; i < npixels; i++) {
    psino = norm_data2[i];
    for (int segnum = 0; segnum < nsegs; segnum++) {
      int mid_rd = (segnum / 2) * span; // mid-ring difference
      scale = cosphi[segnum]; // Don't use yet, * omega2[mid_rd];
      for (int plane = 0; plane < seg_planes[segnum]; plane++, psino++)
        *psino *= scale;
    }
  }

#ifndef __linux__
  StopTimer("Normalization Rescale");
  StopTimer("Total Time");
  PrintTimerList();
  StartTimer("Total Time");
#endif
  /*---------------*/
  /* Write Results */

#ifndef VTUNE_RUN

  LOG_INFO("Writing to disk");

  StartTimer("Disk Write");

  FILE *fptr = fopen(norm_file, "wb");
  if (!fptr) {
    LOG_ERROR( "Can't create normalization output file '%s'", fname);
    exit(1);
  } else {
    int num_sinos = 0;
    // first and last 2 planes in segment 0 are all 0, set the diamonds to 1
    // using a central plane
    plane = 0;
    for (int segnum = 0; segnum < nsegs; segnum++) {
      LOG_INFO( " writing segment {}: nplanes={}", segnum, seg_planes[segnum]);
      num_sinos +=  seg_planes[segnum];
      for (int segplane = 0; segplane < seg_planes[segnum]; segplane++, plane++) {
        for (int i = 0; i < npixels; i++) 
          tmp_sino[i] = norm_data2[i][plane];
        if ((int n = fwrite(tmp_sino, sizeof(float), npixels, fptr)) != npixels) {
          LOG_ERROR( "Write to disk error for '{}' segment {} plane {}", norm_file, segnum, plane + 1);
        }
      }
    }

    fclose( fptr);
    free(norm_data);
    free(norm_data2);
    free(tmp_sino);
    free(cosphi);

    // Update original listmode header and write normalization header
    cheader.WriteChar(CHeader::NAME_OF_DATA_FILE        , norm_file);
    cheader.WriteChar(CHeader::DATA_FORMAT              , "normalization");
    cheader.WriteChar(CHeader::SOFTWARE_VERSION         , sw_version);
    cheader.WriteChar(CHeader::PROGRAM_BUILD_ID         , build_id);
    cheader.WriteInt(CHeader::AXIAL_COMPRESSION™       , span);
    cheader.WriteInt(CHeader::MAXIMUM_RING_DIFFERENCE  , maxrd_);
    cheader.WriteChar(CHeader::DATA_FORMAT              , "normalization");
    if (duration > 0)
      cheader.WriteInt(CHeader::IMAGE_DURATION, duration);
    cheader.WriteChar(CHeader::NUMBER_FORMAT            , "float");
    cheader.WriteInt(CHeader::NUMBER_OF_DIMENSIONS     , 3);
    cheader.WriteInt(CHeader::NUMBER_OF_BYTES_PER_PIXEL, 4);
    cheader.WriteInt(CHeader::MATRIX_SIZE_1            , nprojs);
    cheader.WriteInt(CHeader::MATRIX_SIZE_2            , nviews);
    cheader.WriteInt(CHeader::MATRIX_SIZE_3            , num_sinos);
    cheader.WriteFloat(CHeader::SCALING_FACTOR_1         , 1.218750f);
    cheader.WriteInt(CHeader::SCALING_FACTOR_2         , 1);
    cheader.WriteFloat(CHeader::SCALING_FACTOR_3         , 1.218750f);
    sprintf(fname, "%s.cheader", norm_file);
    LOG_INFO( "Write header {}", fname);
    cheader.WriteFile(fname);

    LOG_INFO( "Write to disk completed");
  }

  StopTimer("Disk Write");

#endif

  /*-----*/
  /* End */

#ifndef __linux__
  StopTimer("Total Time");
  PrintTimerList();
#endif
  fclose( log_fp ) ;
  exit(0) ;
}

