// hrrt_rebinner_lut.cpp : Defines the entry point for the console application.
/* Authors:  Merence Sibomana
  Creation 11/2008
  Modification history:
  22-MAY-2009: Added options -k to support Cologne gantry and
               -L for low tresolution mode

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
#include <unistd.h>

static void usage() {
  printf("\nhrrt_rebinner_lut build %s %s\n\n", __DATE__, __TIME__);
  printf("usage: hrrt_rebinner_lut -o em_lut_out_file [-t tx_lut_out_file] [-k] [-L mode] \n");
  printf("    -t          : Transmission LUT output\n");
  printf("    -L mode     : set Low Resolution mode (1: binsize=2mm, nbins=160, nviews=144, 2:binsize=2.4375)\n");
  exit(1);
}

void main(int argc, char* argv[]) {
  int nprojs = 256;
  int nviews = 288;
  float pitch = 0.0;
  float diam = 0.0;
  float thick = 0.0;
  int span = 9, maxrd = GeometryInfo::MAX_RINGDIFF;
  int nplanes = 0;
  int i = 0, kflag = 0, type = 0, tx_flag = 0;
  char *lut_fname = NULL;

  if (argc == 1) usage();
  while ((i = getopt( argc, argv, "o:t:L:k")) != -1) {
    switch (i) {
    case 'o':
      lut_fname = optarg;
      break;
    case 't':
      lut_fname = optarg;
      tx_flag = 1;
      span = 21;
      maxrd = 10;
      break;
    case 'L':
      if (sscanf( optarg, "%d", &type) == 1) {
        switch (type) {
        case LR_20:
          GeometryInfo::LR_type = LR_20;
          break;
        case LR_24:
          GeometryInfo::LR_type = LR_24;
          break;
        }
      }
      if (GeometryInfo::LR_type != LR_20 && GeometryInfo::LR_type != LR_24) {
        fprintf(stderr, "Invalid LR mode %d\n", type);
        usage();
      }
      break;
    default:
      usage();
    }
  }
  // head_crystal_depth = (float*)calloc(NHEADS, sizeof(float));
  // for (i=0; i<NHEADS;i++)
  //   if (kflag) head_crystal_depth[i] = koln_lthick[i];
  //   else head_crystal_depth[i] = 1.0f;
  for (auto &head_depth : head_crystal_depth_)
    head_depth = 1.0f;

  init_geometry_hrrt( nprojs, nviews, pitch, diam, thick);
  SegmentInfo::init_segment_info(&SegmentInfo::m_nsegs, &nplanes, &SegmentInfo::m_d_tan_theta, maxrd, span, NYCRYS, m_crystal_radius, m_plane_sep);
  // short *sino = (short*)calloc(m_nprojs*m_nviews, sizeof(short));
  if (!tx_flag) {
    init_sol(SegmentInfo::m_segzoffset);
    save_lut_sol(lut_fname);
  } else {
    init_sol_tx(span);
    save_lut_sol_tx(lut_fname);
  }
  exit(0);
}