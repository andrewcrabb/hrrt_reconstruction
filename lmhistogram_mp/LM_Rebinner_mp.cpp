/*
  Component       : HRRT

  Name            : LM_Rebinner.cpp - Listmode Reader thread interface
  Authors         : Merence Sibomana
  Language        : C++
  Platform        : Microsoft Windows.
  Description     :
  * void lm_rebinner(void *arg=NULL): 64-bit listmode rebinner thread
  *  The argument is not used and may be NULL
  *  The thread has 2 states:
  *  1. The producer thread (lm64_reader) is busy; waits for ready packets and sorts them.
  *     It returns if no packet has been ready after MAX_TRY*TIME_SLICE.
  *  2. Sorts all remaining packets.

  Copyright (C) CPS Innovations 2004 All Rights Reserved.

  Creation Date   : 04/25/2004
  Modification history:
  12/11/2004: Added max_rebinners variable for initial number of rebinners
  12/21/2004: The first frame may be done before all rebinner are intialized,
  Use Sleep and check for L32 container initialization rather than
  lm64_consumer_wait which may timeout because of first frame writing.
  20-MAY-2009: Use a single fast LUT rebinner
  04-AUG-2009: Bug fix segment computation was wrong (segment -1 empty)
  1/22/19 rebinner lut file now got from envt var
*/
#include "LM_Rebinner_mp.hpp"
#include "histogram_mp.hpp"
#include "gantryinfo.hpp"
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
#include <gen_delays_lib/gen_delays.h>
#include <boost/filesystem.hpp>

// ahc
#include <string.h>

using namespace std;
using namespace cps;
int model_number = MODEL_HRRT;
// std::string LM_Rebinner::rebinner_lut_file;

int tx_span = 21;
static int em_span = 9;

// boost::filesystem::path LM_Rebinner::rebinner_lut_file;

/*
 * Gets  configuration values from GantryModel and calls init_sort3d_hrrt with read or default values.
 * When span=0 (TX mode), span is set to the value from gm328.ini (key rebTxLUTMode0Span) or 21 if key not found,
 * maxrd is set to (span-1)/2.
 * Returns 1 if OK and 0 if gm328.ini not found or if a key is not found.
 */
// int LM_Rebinner::init_rebinner(int &t_span, int &t_max_ringdiff, const boost::filesystem::path &t_lut_file) {
int LM_Rebinner::init_rebinner(int &t_span, int &t_max_ringdiff) {
  int i = 0,  uniform_flag = -1;
  // int *head_type = (int*)calloc(GeometryInfo::NHEADS, sizeof(int));
  std::vector<int> head_type(GeometryInfo::NHEADS);
  float radius = GeometryInfo::CRYSTAL_RADIUS;
  float def_depth = 1.0f; // default crystal depth
  int ret = 1, tx_flag = 0;
  int nplanes = 0;

  em_span = t_span;
  // head_crystal_depth = (float*)calloc(GeometryInfo::NHEADS, sizeof(float));
  if (GantryInfo::load(model_number) > 0) {
    //    GantryInfo::get("interactionDepth", def_depth);
    GantryInfo::get("crystalRadius", radius);
    for (i = 0; i < GeometryInfo::NHEADS; i++) {
      if (!GantryInfo::get("headInfo[%d].type", i, head_type[i]))
        break;

      HeadType current_head_type = static_cast<HeadType>(head_type[i]);
      if (current_head_type != HeadType::LSO_LSO && current_head_type != HeadType::LSO_GSO && current_head_type != HeadType::LSO_LYSO) {
        cerr << "Invalid head " << i << " type = " << head_type[i] << endl;
        ret = 0;
        break;
      }
      if (i == 1) {
        uniform_flag = (head_type[0] == head_type[1]) ? 1 : 0;
      } else if (head_type[i] != head_type[0]) {
        uniform_flag = 0;
      }
    }
    if (i == GeometryInfo::NHEADS) { // all heads defines
      for (i = 0; i < GeometryInfo::NHEADS; i++) {
        if (static_cast<HeadType>(head_type[i]) == HeadType::LSO_LYSO)
          head_crystal_depth_[i] = def_depth;
        else
          head_crystal_depth_[i] = 0.75; //HeadType::LSO_LSO or HeadType::LSO_GSO
      }
      if (uniform_flag)
        cout << "Layer thickness for all heads = " << head_crystal_depth_[0] << endl;
      else {
        cout << "Layer thickness per head = ";
        for (i = 0; i < GeometryInfo::NHEADS; i++)
          cout << " " << head_crystal_depth_[i];
        cout << endl;
      }
    } else {
      // for (i = 0; i < GeometryInfo::NHEADS; i++) head_crystal_depth_[i] = def_depth;
      head_crystal_depth_.assign(GeometryInfo::NHEADS, def_depth);
      cout << "Layer thickness for all heads = " << def_depth << endl;
    }

    if (t_span == 0) {
      // Transmission mode
      tx_flag = 1;
      if (GantryInfo::get("rebTxLUTMode0Span", t_span)) {
        cout << "Tramsission mode: using span " << t_span << " from configuration file" << endl;
      } else {
        t_span = 21;
        cout << "Tramsission mode: using default span " << t_span << endl;
      }
      tx_span = t_span;
      if (GeometryInfo::LR_type > LR_Type::LR_0 ) {
        t_span = t_span / 2 + 1; // 21==>11; 9==>5
        cout << "Low Resolution mode: span changed to " << t_span << endl;
      }
      t_max_ringdiff = (t_span - 1) / 2;
    }
  } else {
    cerr << "Unable to open gm328.ini configuration file, using default values:" << endl;
    cerr << "Layer thickness for all heads = " << def_depth << "cm" << endl;
    cerr << "Crystal radius = " << radius << "cm" << endl;
    t_span = 21;
    cout << "Tramsission mode: using default span " << t_span << endl;
    ret = 0;
  }

  init_geometry_hrrt();
  SegmentInfo::init_segment_info(&SegmentInfo::m_nsegs, &nplanes, &SegmentInfo::m_d_tan_theta, maxrd_, t_span, GeometryInfo::NYCRYS, m_crystal_radius, m_plane_sep);
  // LM_Rebinner::rebinner_lut_file = t_lut_file;

  if (!tx_flag)
    lor_sinogram::init_lut_sol(SegmentInfo::m_segzoffset);
  else
    lor_sinogram::init_lut_sol_tx();
  SegmentInfo::m_d_tan_theta = (float)(tx_span * m_plane_sep / m_crystal_radius);
  return ret;
}

int rebin_event_tx( int mp, int alayer, int ax, int ay, int blayer, int bx, int by) {
  // double dx, dy, dz, d;
  float deta[3], detb[3]; // , tan_theta, z;
  // int plane, seg, 
  int addr = -1, sino_addr;
  // int axx, bxx;

  int ahead = GeometryInfo::HRRT_MPAIRS[mp][0];
  int bhead = GeometryInfo::HRRT_MPAIRS[mp][1];
  det_to_phy( ahead, alayer, ax, ay, deta);
  det_to_phy( bhead, blayer, bx, by, detb);
  double dz = detb[2] - deta[2];
  double dy = deta[1] - detb[1];
  double dx = deta[0] - detb[0];
  double d = sqrt(dx * dx + dy * dy);
  float tan_theta = (float)(dz / d);
  float z = (float)(deta[2] + (deta[0] * dx + deta[1] * dy) * dz / (d * d));
  int seg =  ((int)(tan_theta / SegmentInfo::m_d_tan_theta + 1024.5)) - 1024;
  int plane = (int)(0.5 + z / m_plane_sep);

  int axx = ax + GeometryInfo::NXCRYS * alayer;
  int bxx = bx + GeometryInfo::NXCRYS * blayer;
  if (blayer == 7) {
    sino_addr = lor_sinogram::solution_tx_[0][mp][axx][bx].nsino; // b is TX source
    z = lor_sinogram::solution_tx_[0][mp][axx][bx].z;
    d = lor_sinogram::solution_tx_[0][mp][axx][bx].d;
  } else {
    sino_addr = lor_sinogram::solution_tx_[1][mp][bxx][ax].nsino; // a is TX source
    z = lor_sinogram::solution_tx_[1][mp][bxx][ax].z;
    d = lor_sinogram::solution_tx_[1][mp][bxx][ax].d;
  }
  if (sino_addr >= 0) {
    if (seg == 0) {
      addr = plane * m_nprojs * m_nviews + sino_addr;
    }
  }
  return addr;
}

int rebin_event( int mp, int alayer, int ax, int ay, int blayer, int bx, int by) {
  float cay, dz2, d, z;
  int plane, segnum, addr, sino_addr, offset;
  int axx, bxx;
  float seg;

  axx = ax + GeometryInfo::NXCRYS * alayer;
  bxx = bx + GeometryInfo::NXCRYS * blayer;
  if ((sino_addr = lor_sinogram::solution_[mp][axx][bxx].nsino) == -1)
    return -1;
  cay = lor_sinogram::m_c_zpos2[ay];
  dz2 = lor_sinogram::m_c_zpos[by] - lor_sinogram::m_c_zpos[ay];
  z = lor_sinogram::solution_[mp][axx][bxx].z;
  d = lor_sinogram::solution_[mp][axx][bxx].d;
  plane = (int)(cay + z * dz2);
  seg = (float)(0.5 + dz2 * d);
  segnum = (int)seg;
  // int rd = abs(by - ay);
  if (seg < 0)
    segnum = 1 - (segnum << 1);
  else
    segnum = segnum << 1;
  if (segnum >= SegmentInfo::m_nsegs)
    return -1;
  if (lor_sinogram::m_segplane[segnum][plane] != -1) {
    offset = SegmentInfo::m_segzoffset[segnum];
    addr = (plane + offset) * m_nprojs * m_nviews + sino_addr;
  } else {
    addr = -1;
  }
  return addr;
}
