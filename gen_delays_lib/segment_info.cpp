/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
  10-DEC-2007: Modifications for compatibility with windelays.
  29-JAN-2009: Add clean_segment_info()
               Replace segzoffset2 by SegmentInfo::m_segzoffset_span9
  07-Apr-2009: Changed filenames from .c to .cpp and removed debugging printf
  30-Apr-2009: Integrate Peter Bloomfield __linux__ support
  02-JUL-2009: Add Transmission(TX) LUT
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <array>

#include "segment_info.h"

int m_current_span = 0;
double SegmentInfo::m_d_tan_theta = 0.0;
double SegmentInfo::m_d_tan_theta_tx = 0.0;
int SegmentInfo::m_nsegs = 0;
// int *SegmentInfo::m_segz0=NULL,
// int *SegmentInfo::m_segzmax=NULL;
// int *SegmentInfo::m_segzoffset = NULL;
// int *SegmentInfo::m_segzoffset_span9 = NULL;
std::vector<int> conversiontable = { // convert span3 to span9
  0 , 0  , 0  , 1  , 2  , 1  , 2  , 1  , 2  , 3  , 4  , 3  , 4  , 3  , 4  , 5  , 6  , 5  , 6  , 5  , 6
  , 7 , 8  , 7  , 8  , 7  , 8  , 9  , 10 , 9  , 10 , 9  , 10 , 11 , 12 , 11 , 12 , 11 , 12 , 13 , 14 , 13 , 14 , 13 , 14
};

std::vector<int> SegmentInfo::m_segz0;
std::vector<int> SegmentInfo::m_segzmax;
std::vector<int> SegmentInfo::m_segzoffset_span9;
std::vector<int> SegmentInfo::m_segzoffset;

/**
 * t_nrings     : input parameter, number of rings.
 * t_span       : input parameter, span
 * t_maxrd      : input parameter, max ring difference.
 * t_nplanes    : output parameter, number of total planes.
 * segzoffset : output parameter, ?
 */
void init_seginfo( int t_nrings, int t_span, int t_maxrd, int *t_nplanes, double *d_tan_theta,
                   int *nsegs, double crystal_radius, double plane_sep) {

  int maxseg = t_maxrd / t_span;
  *nsegs = 2 * maxseg + 1;
  std::vector<int> segnz;
  std::vector<int> segzoff;
  segnz.reserve(*nsegs);
  segzoff.reserve(*nsegs);
  SegmentInfo::m_segz0.reserve(*nsegs);
  SegmentInfo::m_segzmax.reserve(*nsegs);
  *t_nplanes = 0;      //init_seginfo2엔 없음.

  int num_planes = 2 * t_nrings - 1;
  int sp2 = (t_span + 1) / 2;
  for (int i = 0; i < *nsegs; i++) {
    SegmentInfo::m_segz0[0] = (i == 0) ? i : sp2 + t_span * ((i - 1) / 2);
    segnz[i] = num_planes - 2 * SegmentInfo::m_segz0[i];
    segzoff[i] = (i == 0) ? i : segzoff[i - 1] + segnz[i - 1];
    *t_nplanes += segnz[i]; //init_seginfo2엔 없음.
    SegmentInfo::m_segzmax[i] = SegmentInfo::m_segz0[i] + segnz[i] - 1;
    SegmentInfo::m_segzoffset_span9[i] = -SegmentInfo::m_segz0[i] + segzoff[i];
  }
  *d_tan_theta = t_span * plane_sep / crystal_radius;
  segnz.clear();
  segzoff.clear();
  SegmentInfo::m_segz0.clear();
  SegmentInfo::m_segzmax.clear();
}

void init_seginfo2( int t_nrings, int t_span, int t_maxrd, double *d_tan_theta
                    , int *nsegs, double crystal_radius, double plane_sep) {
  std::vector<int> segnz;
  std::vector<int> segzoff;
  segnz.reserve(*nsegs);
  segzoff.reserve(*nsegs);
  SegmentInfo::m_segz0.reserve(*nsegs);
  SegmentInfo::m_segzmax.reserve(*nsegs);

  int maxseg = t_maxrd / t_span;
  *nsegs = 2 * maxseg + 1;
  int num_planes = 2 * t_nrings - 1;
  int sp2 = (t_span + 1) / 2;

  for (int i = 0; i < *nsegs; i++) {
    SegmentInfo::m_segz0[i] = (i == 0) ? i : sp2 + t_span * ((i - 1) / 2);
    segnz[i] = num_planes - 2 * SegmentInfo::m_segz0[i];
    segzoff[0] = (i == 0) ? i : segzoff[i - 1] + segnz[i - 1];
    SegmentInfo::m_segzmax[i] = SegmentInfo::m_segz0[i] + segnz[i] - 1;
    SegmentInfo::m_segzoffset[i] = -SegmentInfo::m_segz0[i] + segzoff[i];
  }
  *d_tan_theta = t_span * plane_sep / crystal_radius;
  segnz.clear();
  segzoff.clear();
}

int SegmentInfo::init_segment_info(int *nsegs, int *t_nplanes, double *d_tan_theta
                      , int t_maxrd, int t_span, int t_nycrys, double crystal_radius, double plane_sep) {
  *nsegs = 2 * (t_maxrd / t_span) + 1;
  *nsegs = 2 * (t_maxrd / 3) + 1;
  init_seginfo( t_nycrys, t_span, t_maxrd, t_nplanes, d_tan_theta, nsegs, crystal_radius, plane_sep);
  init_seginfo2(t_nycrys, 3     , t_maxrd           , d_tan_theta, nsegs, crystal_radius, plane_sep);
  if (t_span == 9) {
    for (int i = 0; i < *nsegs; i++) {
      SegmentInfo::m_segzoffset[i] = SegmentInfo::m_segzoffset_span9[SegmentInfo::conversiontable[i]];
    }
  }
  return 1;
}


