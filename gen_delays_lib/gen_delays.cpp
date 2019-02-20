/* $Id: gen_delays.c,v 1.3 2007/01/08 06:04:55 cvsuser Exp $  */
/* Authors: Inki Hong, Dsaint31, Merence Sibomana, Peter Bloomfield
   Creation 08/2004
   Modification history: Merence Sibomana
   10-DEC-2007: Modifications for compatibility with windelays.
   02-DEC-2008: Bug fix in hrrt_rebinner_lut_path
   07-Apr-2009: Changed filenames from .c to .cpp and removed debugging printf
   30-Apr-2009: Integrate Peter Bloomfield __linux__ support
   11-May-2009: Add span and max ring difference parameters
   02-JUL-2009: Add Transmission(TX) LUT
   21-JUL-2010: Bug fix argument parsing errors after -v (or other single parameter)
   Use scan duration argument only when valid (>0)
   2/6/13 ahc: Made hrrt_rebinner.lut a required command line argument.
               Took out awful code accepting any hrrt_rebinner found anywhere as the one to use.
*/
#include <cstdlib>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
// #include <malloc.h>
#include <string.h>
#include <time.h>
#include <xmmintrin.h>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/program_options.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h" //support for stdout logging
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging

#define _MAX_PATH 256
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "hrrt_util.hpp"
#include "gen_delays.h"
#include "segment_info.h"
#include "geometry_info.h"
#include "lor_sinogram_map.h"

namespace bf = boost::filesystem;
namespace bx = boost::xpressive;
namespace po = boost::program_options;

typedef struct {
  int mp;
  float **delaydata;
  float *csings;
} COMPUTE_DELAYS;

std::shared_ptr<spdlog::logger> g_logger;
boost::filesystem::path g_logfile;
// boost::filesystem::path g_rebinner_lut_file;
boost::filesystem::path g_coincidence_histogram_file;
boost::filesystem::path g_crystal_singles_file;
boost::filesystem::path g_delayed_coincidence_file;
boost::filesystem::path g_output_csings_file;
int g_num_elems = GeometryInfo::NUM_ELEMS;
int g_num_views = GeometryInfo::NUM_VIEWS;
int g_span = 9;
int g_max_ringdiff = GeometryInfo::MAX_RINGDIFF;
float g_pitch = 0.0f;
float g_diam  = 0.0f;
float g_thick = 0.0f;
float g_tau   = 6.0e-9f;
// float tau = 6.0e-9f;
float g_ftime = 1.0f;

static std::string progid = "$Id: gen_delays.c,v 1.3 2007/01/08 06:04:55 cvsuser Exp $";

// static void usage(const char *prog) {
//   printf("%s - generate delayed coincidence data from crystal singles\n", prog);
//   printf("usage: gen_delays -h coincidence_histogram -O delayed_coincidence_file -t count_time <other switches>\n");
//   printf("    -v              : verbose\n");
//   printf("    -C csingles     : old method specifying precomputed crystal singles data\n");
//   printf("    -p ne,nv        : set sinogram size (ne=elements, nv=views) [256,288]\n");
//   printf("    -s span,maxrd   : set span and maxrd values [9,67]\n");
//   printf("    -g p,d,t        : adjust physical parameters (pitch, diameter, thickmess)\n");
//   printf("    -k              : use Koln HRRT geometry (default to normal)\n");
//   printf("    -T tau          : time window parameter [6.0 10-9 sec]\n");
//   // ahc
//   // printf("  * -r rebinner_file: Full path of 'hrrt_rebinner.lut' file (required)\n");
//   exit(1);
// }

int compute_delays(int mp, float **delays_data, float *csings) {
  std::array<float, 104> dz2;

  // int ahead = GeometryInfo::HRRT_MPAIRS[mp][0];
  int bhead = GeometryInfo::HRRT_MPAIRS[mp][1];
  float tauftime = g_tau * g_ftime;

  for (int alayer = 0; alayer < GeometryInfo::NDOIS; alayer++) {
    for (int ay = 0; ay < GeometryInfo::NYCRYS; ay++) {
      //  if (ay%10==0) printf("Generating delays for MP %d [H%d,H%d] %d\t%d \r", mp, ahead, bhead,alayer,ay);
      float cay = lor_sinogram::m_c_zpos2[ay];
      int bs = 1000;
      int be = -1000;

      //get rd,dz2[by],bs,be
      for (int by = 0; by < GeometryInfo::NYCRYS; by++) {
        dz2[by] = lor_sinogram::m_c_zpos[by] - lor_sinogram::m_c_zpos[ay]; // z diff. between det A and det B
        int rd = ay - by;
        if (rd < 0)
          rd = by - ay;
        if (rd < GeometryInfo::maxrd_ + 6) {  // dsaint31 : why 6??
          if (bs > by)
            bs = by; //start ring # of detB
          if (be < by)
            be = by; //end   ring # of detB
        }
      }

      for (int ax = 0; ax < GeometryInfo::NXCRYS; ax++) {
        int axx = ax + GeometryInfo::NXCRYS * alayer;
        float aw = csings[alayer * GeometryInfo::NUM_CRYSTALS_X_Y_HEADS + ay * GeometryInfo::NUM_CRYSTALS_X_Y_HEADS + ax];
        float awtauftime = aw * tauftime; // 2 * tau * singles at detA
        if (awtauftime == 0)
          continue;

        for (int blayer = 0; blayer < 2; blayer++) {
          int bxx = GeometryInfo::NXCRYS * blayer;
          for (int bx = 0; bx < GeometryInfo::NXCRYS; bx++, bxx++) {
            if (lor_sinogram::solution_[mp][axx][bxx].nsino == -1)
              continue;
            lor_sinogram::SOL *sol = &lor_sinogram::solution_[mp][axx][bxx];

            //dsaint31
            float *dptr = delays_data[sol->nsino];//result bin location

            int headxcrys = GeometryInfo::NHEADS * GeometryInfo::NXCRYS;
            int nbd  = blayer * GeometryInfo::NUM_CRYSTALS_X_Y_HEADS + bhead * GeometryInfo::NXCRYS + bx + headxcrys * bs;
            for (int by = bs; by <= be; by++, nbd += headxcrys) {
              int plane = (int)(cay + sol->z * dz2[by]);

              float seg = (float)(0.5 + dz2[by] * sol->d);
              int segnum = (int)seg;
              if (seg < 0)
                segnum = 1 - (segnum << 1);
              else
                segnum = segnum << 1;

              //dsaint31
              if (lor_sinogram::m_segplane[segnum][plane] != -1)
                dptr[lor_sinogram::m_segplane[segnum][plane]] += csings[nbd] * awtauftime;
              //if (lor_sinogram::m_segplane[segnum][plane]!=-1)
              //  delays_data[lor_sinogram::m_segplane[segnum][plane]][current_view][current_proj] +=csings[nbd]*awtauftime;
            }
          }
        }
      }
    }
  }
  return 1;
}



// Compute the delayed crystal coincidence rates for the specified crystal singles rates

template <std::size_t SIZE>
void compute_drate(float *t_srate, float t_tau, std::array<float, SIZE> &t_drate) {
  std::array<float, GeometryInfo::NHEADS> hsum;

  // First we compute the total singles rates for each of the heads...

  for (int head = 0; head < GeometryInfo::NHEADS; head++) {
    hsum[head] = 0.0;
    for (int layer = 0; layer < GeometryInfo::NDOIS; layer++)
      for (int cx = 0; cx < GeometryInfo::NXCRYS; cx++)
        for (int cy = 0; cy < GeometryInfo::NYCRYS; cy++) {
          int i = GeometryInfo::NXCRYS * head + cx + GeometryInfo::NXCRYS * GeometryInfo::NHEADS * cy + GeometryInfo::NUM_CRYSTALS_X_Y_HEADS * layer;
          hsum[head] += t_srate[i];
        }
  }

  // Now we compute the delayed coincidence rate for each crystal...

  for (int head = 0; head < GeometryInfo::NHEADS; head++) {
    int head_xcrys = GeometryInfo::NXCRYS * head;
    float ohead_sum = 0.0;
    for (int j = 0; j < 5; j++) {
      int ohead = (head + j + 2) % GeometryInfo::NHEADS;
      ohead_sum += hsum[ohead];
    }
    for (int layer = 0; layer < GeometryInfo::NDOIS; layer++) {
      for (int cx = 0; cx < GeometryInfo::NXCRYS; cx++) {
        for (int cy = 0; cy < GeometryInfo::NYCRYS; cy++) {
          int i = head_xcrys + cx + GeometryInfo::NXCRYS * GeometryInfo::NHEADS * cy + GeometryInfo::NUM_CRYSTALS_X_Y_HEADS * layer;
          t_drate[i] = t_srate[i] * t_tau * ohead_sum;
        }
      }
    }
  }
}

void estimate_srate(std::array<float, GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS> const &drate, float tau, float *srate) {
  float hsum[GeometryInfo::NHEADS];

  // First we compute the total singles rates for each of the heads...

  for (int head = 0; head < GeometryInfo::NHEADS; head++) {
    hsum[head] = 0.0;
    for (int layer = 0; layer < GeometryInfo::NDOIS; layer++) {
      for (int cx = 0; cx < GeometryInfo::NXCRYS; cx++) {
        for (int cy = 0; cy < GeometryInfo::NYCRYS; cy++) {
          int i = GeometryInfo::NXCRYS * head + cx + GeometryInfo::NXCRYS * GeometryInfo::NHEADS * cy + GeometryInfo::NUM_CRYSTALS_X_Y_HEADS * layer;
          hsum[head] += srate[i];
        }
      }
    }
  }

  // Now we update the estimated singles rate for each crystal...

  for (int head = 0; head < GeometryInfo::NHEADS; head++) {
    float ohead_sum = 0.0;
    for (int j = 0; j < 5; j++) {
      int ohead = (head + j + 2) % GeometryInfo::NHEADS;
      ohead_sum += hsum[ohead];
    }
    for (int layer = 0; layer < GeometryInfo::NDOIS; layer++)
      for (int cx = 0; cx < GeometryInfo::NXCRYS; cx++)
        for (int cy = 0; cy < GeometryInfo::NYCRYS; cy++) {
          int i = GeometryInfo::NXCRYS * head + cx + GeometryInfo::NXCRYS * GeometryInfo::NHEADS * cy + GeometryInfo::NUM_CRYSTALS_X_Y_HEADS * layer;
          srate[i] = (srate[i] + drate[i] / (tau * ohead_sum)) / 2.0f;
        }
  }
}

template<std::size_t SIZE>
void compute_initial_srate(std::array<float, SIZE> const &drate, float tau, float *srate) {
  float dtotal = 0.0;

  for (int i = 0; i < GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS; i++)
    dtotal += drate[i];
  float stotal = (float)(8.0 * sqrt(dtotal / (40. * tau)));
  for (int i = 0; i < GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS; i++)
    srate[i] = drate[i] * stotal / dtotal;
}

template<std::size_t SIZE>
int errtotal(int *ich, std::array<float, SIZE> const &srate, int nvals, float dt) {
  int errsum = 0;

  for (int i = 0; i < nvals; i++) {
    int err = (int)(ich[i] - (0.5 + srate[i] * dt));
    errsum += err * err;
  }
  return (errsum);
}

int compute_csings_from_drates(int *dcoins, float tau, float dt, float *srates) {
  constexpr int num_crystals = GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS;
  std::array<float, num_crystals> drates;
  std::array<float, num_crystals> xrates;

  for (int i = 0; i < num_crystals; i++)
    drates[i] = dcoins[i] / dt;
  compute_initial_srate(drates, tau, srates);
  int iter;
  for (iter = 0; iter < 100; iter++) {
    compute_drate(srates, tau, xrates);  // compute the expected delayed rate
    int err = errtotal(dcoins, xrates, num_crystals, dt);       // compute the error
    if (iter && (err == 0))
      break; // only stop when we converge
    estimate_srate(drates, tau, srates); // update the estimate
  }
  return (iter);
}

void *pt_compute_delays(void *ptarg) {
  COMPUTE_DELAYS *arg = (COMPUTE_DELAYS *) ptarg;
  //  printf("%x\t%x\t%d\n",delays_data,arg->delaydata,arg->mp);
  compute_delays(arg->mp, arg->delaydata, arg->csings);
  pthread_exit(NULL ) ;
}

long int time_diff(struct timeval const &start, struct timeval const &stop) {
  long int start_msec = start.tv_sec * 1000 + (int)(double)start.tv_usec / 1000.0;
  long int stop_msec  = stop.tv_sec  * 1000 + (int)(double)stop.tv_usec  / 1000.0;
  long int diff_msec  = stop_msec - start_msec;
  return diff_msec;
}

// TODO reimplement this using std::shared_ptr<float> crystal_singles when I understand them better...

int read_crystal_singles_file(float *csings) {
  if (!g_crystal_singles_file.empty()) {
    std::ifstream instream;
    instream.open(g_crystal_singles_file.string(), std::ifstream::in | std::ifstream::binary);
    if (!instream.is_open()) {
      g_logger->error("Cound not open crystal singles file: {}", g_crystal_singles_file);
      return 1;
    }
    int nbytes = GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS * sizeof(float);
    instream.read((char *)csings, nbytes);
    if (instream.fail()) {
      g_logger->error("Cound not read crystal singles file: {}", g_crystal_singles_file);
      return 1;
    }
    instream.close();
  }
  return 0;
}

// TODO reimplement this as shared_ptr and ifstream.  Though other routines call gen_delays with a FILE*

int read_coincidence_sinogram_file(int *t_coincidence_sinogram, FILE *t_coincidence_file_ptr) {
  FILE *fptr;
  if (t_coincidence_file_ptr == NULL)
    fptr = fopen(g_coincidence_histogram_file.c_str(), "rb");
  else
    fptr = t_coincidence_file_ptr;

  if (!fptr) {
    if (t_coincidence_file_ptr) {
      g_logger->error("Can't open supplied sinogram FILE");
    } else {
      g_logger->error("Can't open coincidence histogram file {}", g_coincidence_histogram_file);
    }
    return (1);
  }
  // std::vector<int> coincidence_sinogram(GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS * 2); // prompt followed by delayed
  // if (!coincidence_sinogram)
  //   printf("calloc failure for coincidence_sinogram array\n");
  int n = (int)fread(t_coincidence_sinogram, sizeof(int), GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS * 2, fptr);
  if (fptr != t_coincidence_file_ptr)
    fclose(fptr);
  if (n != 2 * GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS)  {
    g_logger->error("Not enough data in coinsfile '{}' (only {} of {})", g_coincidence_histogram_file, n, GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS * 2);
    return (1);
  }
  return 0;
}

// in_inline : 0:file save and alone | 1: file save and inline | 2: file unsave and inline

int gen_delays(int is_inline,
               float scan_duration,
               float ***result,
               FILE *t_coincidence_file_ptr,
               char *p_delays_file,
               int t_span,
               int t_maxrd
               // boost::filesystem const &t_rebinner_lut_file
              )
{
  struct timeval t0, t1, t2, t3;
  // FILE *fptr;
  // char *rebinner_lut_file_ptr = NULL;   // TODO Take this out once all moved to bf
  // bf::path rebinner_lut_file;
  // char *csingles_file = NULL;
  // char *delays_file = NULL;
  // char *coins_file = NULL;
  // char *output_csings_file = NULL;
  // char *optarg;
  float **delays_data;
  // int threadnum ;
  // unsigned int threadID;

  COMPUTE_DELAYS comdelay[4];

  // 1  1  1  1  1   2   2   2   2   2      // machine number for MPI version
  // 2  1  0  1  2   2   1   0   1   2      // amount of load
  int mporder[2][10] = {{1, 2, 3, 4, 5,  11, 12, 13, 14, 18}, // for the load balancing
    {6, 7, 8, 9, 10, 15, 16, 17, 19, 20}
  };
  // const char *rebinner_lut_file=NULL;

  // delays_file = p_delays_file;
  if (p_delays_file)
      g_delayed_coincidence_file = boost::filesystem::path(p_delays_file);
  if (scan_duration > 0)
    g_ftime = scan_duration;
  GeometryInfo::maxrd_ = t_maxrd;

  // Process command line arguments.
  if (is_inline == 0) {
    //   case 'r' : rebinner_lut_file_ptr = optarg;  // hrrt_rebinner.lut
    //   case 'h':   coins_file = optarg; break; // coincidence histogram (int 72,8,104,4)
    //   case 'p':   sscanf(optarg, "%d,%d", &nprojs, &nviews); break; // -p nprojs,nviews - set sinogram size
    //   case 's':   sscanf(optarg, "%d,%d", &span, &GeometryInfo::maxrd_); break;    // -s span,maxrd - set 3D parameters
    //   case 'g':   sscanf(optarg, "%f,%f,%f", &pitch, &diam, &thick); break;    // -g pitch,diam,thick
    //   case 'C':   csingles_file = optarg; break;  // -C crystal singles file
    //   case 'O':   delays_file = optarg; break;  // -O delays_file
    //   case 'S':   output_csings_file = optarg; break; // -S save_csings_file
    //   case 'T':   sscanf(optarg, "%f", &tau); break;
    //   case 't':   sscanf(optarg, "%f", &g_ftime); break;
  } else {
    // inline mode.
    // coins_file  = (char *)"memory_mode";
    g_coincidence_histogram_file = boost::filesystem::path("memory_mode");
  }

  // delays and coins files are required anyway
  // if (is_inline < 2 && !delays_file) {
  //   LOG_INFO( "Output File must be specified with -O <filename>\n");
  //   exit(1);
  // }
  // if (!csingles_file && !coins_file && t_coincidence_file_ptr == NULL)
  //   LOG_INFO( "Input Crystal Singles or Coincidence Histogram file must be specified with -h or -C <filename>\n");

  gettimeofday(&t0, NULL ) ;
  GeometryInfo::head_crystal_depth_.fill(1.0f);
  GeometryInfo::init_geometry_hrrt(g_pitch, g_diam, g_thick);
  int nplanes = 0;
  SegmentInfo::init_segment_info(&SegmentInfo::m_nsegs, &nplanes, &SegmentInfo::m_d_tan_theta, GeometryInfo::maxrd_, t_span, GeometryInfo::NYCRYS, GeometryInfo::crystal_radius_, GeometryInfo::plane_sep_);

  // LOG_ERROR("Using Rebinner LUT file %s\n", rebinner_lut_file);
  lor_sinogram::init_lut_sol(SegmentInfo::m_segzoffset);


  //-------------------------------------------------------
  // delayed true output value init.

  if (is_inline == 2 && result == NULL) {
    result = (float ***) malloc(nplanes * sizeof(float**));
    for (int i = 0; i < nplanes; i++) {
      result[i] = (float **) malloc(GeometryInfo::nviews_ * sizeof(float*));
      for (int j = 0; j < GeometryInfo::nviews_; j++) {
        result[i][j] = (float *) malloc(GeometryInfo::nprojs_ * sizeof(float));
      }
    }
  }
  if (result != NULL) {
    for (int i = 0; i < nplanes; i++)
      for (int j = 0; j < GeometryInfo::nviews_; j++)
        memset(&result[i][j][0], 0, GeometryInfo::nprojs_ * sizeof(float));
  }

  int nsino = g_num_elems * g_num_views;
  delays_data = (float**) calloc(nsino, sizeof(float*));
  for (int i = 0; i < nsino; i++) {
    delays_data[i] = (float *) calloc (nplanes, sizeof(float));
    if (delays_data[i] == NULL) {
      printf("error allocation delays_data\n");
      exit(1);
    }
    memset(delays_data[i], 0, nplanes * sizeof(float));
  }
  if (!delays_data)
    printf("malloc failed for delays_data\n");

  //-------------------------------------------------------
  // make singles. (load or estimate)

  int num_crystals = GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS;
  float *csings = (float*) calloc(num_crystals, sizeof(float));
  // TODO implement this as below.  Requires knowing how to read from a std::ifstream to a std::shared_ptr<float>
  // std::shared_ptr<float> crystal_singles(new float[num_crystals], std::default_delete<int[]>());
  if (read_crystal_singles_file(csings))
    exit(1);

  if (!g_coincidence_histogram_file.empty() || t_coincidence_file_ptr != NULL ) {
    // TODO reimplement this with shared_ptr and ifstream
    int *coincidence_sinogram = (int*) calloc(num_crystals * 2, sizeof(int)); // prompt followed by delayed
    if (read_coincidence_sinogram_file(coincidence_sinogram, t_coincidence_file_ptr))
      exit(1);

    gettimeofday(&t2, NULL);
    // we only used the delayed coincidence_sinogram (coincidence_sinogram+num_crystals)vvvvvv
    int niter = compute_csings_from_drates(coincidence_sinogram + num_crystals, g_tau, g_ftime, csings);
    gettimeofday(&t3, NULL);
    printf("csings computed from drates in %d iterations (%ld msec)\n", niter, (((t3.tv_sec * 1000 ) + (int)((double)t3.tv_usec / 1000.0 ) ) - ((t2.tv_sec * 1000 ) + (int)((double)t2.tv_usec / 1000.0 ))));
    free(coincidence_sinogram);
    if (!g_output_csings_file.empty())
      // write_output_csings_file(csings);
      if (hrrt_util::write_binary_file<float>(csings, num_crystals, g_output_csings_file, "Singles from delayed coincidence histogram"))
        exit(1);
  }

  //-------------------------------------------------------
  // calculate delayed true

  comdelay[0].delaydata = delays_data; //result;
  comdelay[1].delaydata = delays_data; //result;
  comdelay[0].csings = csings;
  comdelay[1].csings = csings;
  comdelay[2].delaydata = delays_data; //result;
  comdelay[3].delaydata = delays_data; //result;
  comdelay[2].csings = csings;
  comdelay[3].csings = csings;

  pthread_t threads[ 4 ] ;
  pthread_attr_t attr;
  for (int mp = 0; mp < 5; mp++) {
    gettimeofday(&t1, NULL );
    comdelay[0].mp = mporder[0][mp];
    comdelay[1].mp = mporder[1][mp];
    comdelay[2].mp = mporder[0][mp + 5];
    comdelay[3].mp = mporder[1][mp + 5];
    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (int threadnum = 0 ; threadnum < 4 ; threadnum += 1 ) {
      pthread_create(&threads[ threadnum ], &attr, &pt_compute_delays, &comdelay[ threadnum ] ) ;
    }
    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr ) ;
    for (int threadnum = 0 ; threadnum < 4 ; threadnum += 1 ) {
      pthread_join(threads[ threadnum ] , NULL ) ;
    }
    gettimeofday(&t2, NULL);
    LOG_INFO( "%d\t%ld msec   \n", mp + 1          , time_diff(t1, t2));
    fflush(stdout);
  }
  gettimeofday(&t1, NULL);
  g_logger->info("Smooth Delays computed in {ld} msec", time_diff(t0, t1));

  free(lor_sinogram::solution_[0]);
  for (int i = 1; i < 21; i++) {
    for (int j = 0; j < GeometryInfo::NXCRYS * GeometryInfo::NDOIS; j++) {
      //LOG_INFO("%d:%d\n",i,j);
      if (lor_sinogram::solution_[i][j] != NULL) free(lor_sinogram::solution_[i][j]);
    }
    //LOG_INFO("%d:%d\n",i,j);
    free(lor_sinogram::solution_[i]);
  }

  free(lor_sinogram::solution_);
  free(csings);
  free(lor_sinogram::m_c_zpos);
  free(lor_sinogram::m_c_zpos2);

  for (int i = 0; i < 63; i++) {
    free(lor_sinogram::m_segplane[i]);
  }

  if (lor_sinogram::m_segplane != NULL)
    free(lor_sinogram::m_segplane);

  //-------------------------------------------------------
  // file write
  float *dtmp  = (float *) calloc(nsino, sizeof(float));
  for (int i = 0; i < nplanes; i++) {
    for (int n = 0; n < nsino; n++) {
      dtmp[n] = delays_data[n][i];
    }
    if (is_inline < 2) {
      if (hrrt_util::write_binary_file<float>(dtmp, nsino, g_delayed_coincidence_file, "Delayed coincidence"))
        exit(1);
      // write_delays_file(dtmp, nsino);
    } else {
      int n = 0;
      for (int j = 0; j < GeometryInfo::nviews_; j++) {
        memcpy(result[i][j], &dtmp[n], sizeof(float) * GeometryInfo::nprojs_); // = dtmp[n];
        n = n + GeometryInfo::nprojs_;
      }
    }
  }

  for (int n = 0; n < nsino; n++) {
    free(delays_data[n]);
  }
  free(delays_data);
  free(dtmp);

  gettimeofday(&t3, NULL);
  int dtime3 = (((t3.tv_sec * 1000 ) + (int)((double)t3.tv_usec / 1000.0 ) ) - ((t2.tv_sec * 1000 ) + (int)((double)t2.tv_usec / 1000.0 ) ) ) ;
  int dtime4 = (((t3.tv_sec * 1000 ) + (int)((double)t3.tv_usec / 1000.0 ) ) - ((t0.tv_sec * 1000 ) + (int)((double)t0.tv_usec / 1000.0 ) ) ) ;
  printf("...stored to disk in %d msec.\n", dtime3);
  printf("Total time %d msec.\n", dtime4);
  return 1;
}

void init_logging(void) {
  if (g_logfile.empty()) {
    g_logfile = fmt::format("{}_gen_delays.log", hrrt_util::time_string());
  }
  g_logger = spdlog::basic_logger_mt("basic_logger", g_logfile.string());
}


// void on_rebinner(std::string const &instring) {
//   g_rebinner_lut_file = boost::filesystem::path(instring);
// }

void on_coincidence(std::string const &instring) {
  g_coincidence_histogram_file = boost::filesystem::path(instring);
}

void on_csingles(std::string const &instring) {
  g_crystal_singles_file = boost::filesystem::path(instring);
}

void on_delay(std::string const &instring) {
  g_delayed_coincidence_file = boost::filesystem::path(instring);
}

void on_outsing(std::string const &instring) {
  g_output_csings_file = boost::filesystem::path(instring);
}

/**
 * @brief      Parse sino size string
 * @param      instring  'elements/projections,views' (ints)
 */

void on_sino_size(std::string const &instring) {
  bx::sregex re_size = bx::sregex::compile("(?P<nelements>[0-9]+)\\,(<?P<nviews>[0-9]+)");
  bx::smatch match;

  if (bx::regex_match(instring, match, re_size)) {
    g_num_elems = boost::lexical_cast<int>(match["nelements"]);
    g_num_views = boost::lexical_cast<int>(match["nviews"]);
  } else {
    g_logger->error("Invalid sino size string: {}", instring);
    exit(1);
  }
}

/**
 * @brief      Parse span string
 * @param      instring  'span,maxrd' (ints)
 */

void on_span(std::string const &instring) {
  bx::sregex re_span = bx::sregex::compile("(?P<span>[0-9]+)\\,(<?P<ringdiff>[0-9]+)");
  bx::smatch match;

  if (bx::regex_match(instring, match, re_span)) {
    g_span         = boost::lexical_cast<int>(match["span"]);
    g_max_ringdiff = boost::lexical_cast<int>(match["ringdiff"]);
  } else {
    g_logger->error("Invalid span,ringdiff string: {}", instring);
    exit(1);
  }
}

/**
 * @brief      Parse input geometry
 * @param      instring  'pitch,diameter,thickness' (floats)
 */

void on_geometry(std::string const &instring) {
  bx::sregex re_geom = bx::sregex::compile("(?P<pitch>[0-9]+\\.[0-9]+)\\,(<?P<diam>[0-9]+\\.[0-9]+)\\,(<?P<thick>[0-9]+\\.[0-9]+)");
  bx::smatch match;

  if (bx::regex_match(instring, match, re_geom)) {
    g_pitch = boost::lexical_cast<float>(match["pitch"]);
    g_diam  = boost::lexical_cast<float>(match["diam"]);
    g_thick = boost::lexical_cast<float>(match["thick"]);
  } else {
    g_logger->error("Invalid pitch,diam,thick string: {}", instring);
    exit(1);
  }
}

void parse_args(int argc, char **argv) {
  try {
    po::options_description desc("Options");
    desc.add_options()
      ("help,H", "Show options")  // Existing code used 'h' for coincidence file
      ("time,t"        , po::value<float>(&g_ftime)->required()                         , "Count time")
      // ("rebinner,r"    , po::value<std::string>()->notifier(on_rebinner)->required()    , "Full path of rebinner LUT file")
      ("coins,h"       , po::value<std::string>()->notifier(on_coincidence)->required() , "Coincidence histogram file")
      ("delays,O"      , po::value<std::string>()->notifier(on_delay)->required()       , "Delayed coincidence file")
      ("span,s"        , po::value<std::string>()->notifier(on_span)                    , fmt::format("span,maxrd ({},{})", 9, GeometryInfo::MAX_RINGDIFF).c_str())
      ("sino_size,p"   , po::value<std::string>()->notifier(on_sino_size)               , fmt::format("sinogram size 'nelements,nviews' ({},{})", GeometryInfo::NUM_ELEMS, GeometryInfo::NUM_VIEWS).c_str())
      ("geometry,g"    , po::value<std::string>()->notifier(on_geometry)                , "geometry 'pitch,diam,thick'")
      ("csingles,C"    , po::value<std::string>()->notifier(on_csingles)                , "Crystal singles file; old method specifying precomputed crystal singles data")
      ("outsing,S"     , po::value<std::string>()->notifier(on_outsing)                 , "Output crystal singles file")
      ("tau,T"         , po::value<float>(&g_tau)                                       , "time window parameter (6.0 10-9 sec)" )
    ;
    po::positional_options_description pos_opts;
    pos_opts.add("infile", 1);

    po::variables_map vm;
    po::store(
      po::command_line_parser(argc, argv)
      .options(desc)
      .style(po::command_line_style::unix_style | po::command_line_style::allow_long_disguise)
      .positional(pos_opts)
      .run(), vm
    );
    po::notify(vm);
    if (vm.count("help"))
      std::cout << desc << '\n';
  }
  catch (const po::error &ex) {
    std::cerr << ex.what() << std::endl;
  }
}


int main(int argc, char* argv[]) {
  init_logging();
  parse_args(argc, argv);
  // gen_delays is expecting to see all the globals.  Bring it in this file or pass them explicitly.
  gen_delays(0, 0.0f, NULL, NULL, NULL, g_span, g_max_ringdiff);
  return 0;
}

