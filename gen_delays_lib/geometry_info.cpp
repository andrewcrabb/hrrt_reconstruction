/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
  10-DEC-2007: Modifications for compatibility with windelays.
  07-Apr-2009: Changed filenames from .c to .cpp and removed debugging printf
  09-Apr-2009: Bug fix in init_geometry_hrrt (initialize i=0)
  30-Apr-2009: Integrate Peter Bloomfield __linux__ support
  02-JUL-2009: Add Transmission(TX) LUT
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <array>
#include <algorithm>
#include <map>
#include <fmt/format.h>

#include "geometry_info.h"

int nsino = 0;

namespace GeometryInfo {

double binsize_;
std::vector<double> sin_head_(NHEADS);
std::vector<double> cos_head_(NHEADS);
double crystal_radius_;
// double *crystal_xpos_ = NULL;
// double *crystal_ypos_ = NULL;
// double *crystal_zpos_ = NULL;
int    nprojs_;
int    nviews_;
int maxrd_ = MAX_RINGDIFF;
double plane_sep_, crystal_x_pitch_, crystal_y_pitch_;

std::istream& operator>>(std::istream& t_in, LR_Type& t_lr_type) {
    std::string token;
    t_in >> token;
    int ival = stoi(token);
    if ((ival >= (int)LR_Type::LR_0) && (ival <= (int)LR_Type::LR_24))
      t_lr_type = (LR_Type)stoi(token);
    // else 
        // t_in.setstate(std::ios_base::failbit);
    return t_in;
}

// Return int converted to LR_Type, if valid.  Else raise.

LR_Type to_lrtype(int intval, std::vector<LR_Type> goodvals) {
  LR_Type ret = LR_Type::MAX_LR_TYPE;
  LR_Type in_lr = static_cast<LR_Type>(intval);
  if ((in_lr >= LR_Type::LR_0) && (in_lr < LR_Type::MAX_LR_TYPE)) {
    if (goodvals.size() > 0) {
      if(std::find(goodvals.begin(), goodvals.end(), in_lr) != goodvals.end()) {
        ret = in_lr;
      }
    }
  }
  if (ret == LR_Type::MAX_LR_TYPE) {
    throw std::invalid_argument(fmt::format("Invalid LR_Type value: {}", intval));
  }
  return ret;
}

LR_Type to_lrtype(int intval) {
  std::vector<LR_Type> v;
  return(to_lrtype(intval, v));
}

// Define and initialize variables declared in header
LR_Type LR_type = LR_Type::LR_0;
std::array<float, NHEADS> head_crystal_depth_;
std::array<double, NUM_CRYSTALS_X_Y_HEADS_DOIS> crystal_xpos_;
std::array<double, NUM_CRYSTALS_X_Y_HEADS_DOIS> crystal_ypos_;
std::array<double, NUM_CRYSTALS_X_Y_HEADS_DOIS> crystal_zpos_;

const std::map<LR_Type, LR_Geom> lr_geometries_ = {
  {LR_Type::LR_0 , {256, 288, PITCH / 2.0, PITCH / 2.0}},
  {LR_Type::LR_20, {160, 144, 0.2f                     , PITCH      }},
  {LR_Type::LR_24, {128, 144, PITCH      , PITCH      }}
  };


/**********************************************************
/   routine det_to_phy converts from detector index to
/   physical location based on externally defined geometry.
function hrrt_pos, head, l, i, j
    dist_to_head    = 23.45 cm
    blk_pitch       = 1.95  cm
    xblks_per_head  = 9
    yblks_per_head  = 13
    crys_per_blk    = 8
    layer_thickness = 0.10
    xcrys_per_head  = xblks_per_head*crys_per_blk
    ycrys_per_head  = yblks_per_head*crys_per_blk
    crys_pitch      = blk_pitch/crys_per_blk
    head_angle      = head*!pi/4.
    sint = sin(head_angle)
    cost = cos(head_angle)
    x = crys_pitch*(i-(xcrys_per_head-1)/2.)
    y = dist_to_head+l*layer_thickness
    z = j*crys_pitch
    return,[x*cost+y*sint,-x*sint+y*cost,z]
end
**********************************************************/

void calc_txsrc_position( int head, int detx, int dety, float location[3]) {
//  The transmission source position is determined from a position tag word which
//  contains the head, and crystal x,y location of the source. This is converted
//  into X,Y,Z position coordinates here.

  float angle;
  int offset = -36, i;
  double sint, cost;
  static float *xpos = NULL;
  static float *ypos = NULL;

  if (xpos == NULL) {
    xpos = (float*) calloc( 576, sizeof(float));
    ypos = (float*) calloc( 576, sizeof(float));
    for (i = 0; i < 576; i++) {
      angle = (float)(M_PI * (i + offset) / 288.0);
      sint = sin(angle);
      cost = cos(angle);
      xpos[i] = (float)(TX_RADIUS * sint);
      ypos[i] = (float)(TX_RADIUS * cost);
    }
  }
  i = head * 72 + detx;
  location[0] = xpos[i];
  location[1] = ypos[i];
  location[2] = (float)(dety * PITCH);
}

void calc_det_to_phy( int head, int layer, int detx, int dety, float location[3]) {
  double sint, cost, x, y, z, xpos, ypos;
  int bcrys, blk;

  if (layer == 7) {
    calc_txsrc_position( head, detx, dety, location);
    return;
  }
  sint = sin_head_[head];
  cost = cos_head_[head];
  bcrys = detx % 8;
  blk = detx / 8;
  x = blk * (BSIZE + BGAP) + bcrys * (CSIZE + CGAP) - XHSIZE / 2.0 + CSIZE / 2.0; // +CGAP/2.0 //dsaint31
//    y = crystal_radius_+1.0f*(1-layer)+0.5f;
  y = crystal_radius_ + head_crystal_depth_[head] * (1 - layer) + 0.5 * head_crystal_depth_[head];

  bcrys = dety % 8;
  blk = dety / 8;
  z = blk * (BSIZE + BGAP) + bcrys * (CSIZE + CGAP); // - CGAP-CSIZE/2.0 //dsaint31

  xpos = x * cost + y * sint;
  ypos = -x * sint + y * cost;

  location[0] = (float)xpos; // X location
  location[1] = (float)ypos; // Y location
  location[2] = (float)z;    // Z location
}

// Oh ffs np and mv are not used anywhere...
// void init_geometry_hrrt ( int np, int nv, float cpitch, float diam, float thick) {
// Move methods into the namespace as they are processed by the Great Rewrite of 2018-19

void init_geometry_hrrt (float cpitch, float diam, float thick) {
  float pitch  = (cpitch > 0.0) ? cpitch : PITCH;
  float rdiam  = (diam > 0.0)   ? diam   : RDIAM;
  float lthick = (thick > 0.0)  ? thick  : LTHICK;

  for (int i = 0; i < NHEADS; i++) {
    sin_head_[i] = sin(i * 2.0 * M_PI / NHEADS);
    cos_head_[i] = cos(i * 2.0 * M_PI / NHEADS);
  }
  crystal_radius_ = rdiam / 2.0;
  crystal_x_pitch_ = crystal_y_pitch_ = pitch;
  nprojs_ = lr_geometries_[LR_type].nprojs;
  nviews_ = lr_geometries_[LR_type].nviews;
  switch (LR_type) {
  case LR_Type::LR_0:
    binsize_   = pitch / 2.0;
    plane_sep_ = pitch / 2.0;
    break;
  case LR_Type::LR_20:
    binsize_   = 0.2f; // 2mm
    plane_sep_ = pitch;
    break;
  case LR_Type::LR_24:
    binsize_   = pitch;
    plane_sep_ = pitch;
    break;
  }

  head_crystal_depth_.assign(NHEADS, lthick);
  printf("  layer_thickness = %0.4f cm\n", lthick);

  float pos[3];
  int i;
  for (int head = 0; head < NHEADS; head++) {
    for (int layer = 0; layer < NDOIS; layer++) {
      for (int xcrys = 0; xcrys < NXCRYS; xcrys++) {
        for (int ycrys = 0; ycrys < NYCRYS; ycrys++, i++) {
          calc_det_to_phy( head, layer, xcrys, ycrys, pos);
          crystal_xpos_[i] = pos[0];
          crystal_ypos_[i] = pos[1];
          crystal_zpos_[i] = pos[2];
        }
      }
    }
  }

  printf("Geometry configured for HRRT (%d heads of radius %0.2f cm.)\n", NHEADS, crystal_radius_);
  printf("  crystal x,y pitches (cm) = %0.4f, %0.4f\n", pitch, pitch);
  printf("  x_head_center = %0.4f cm\n", pitch * (NXCRYS - 1) / 2.0);
}

void init_geometry_hrrt(void) {
  init_geometry_hrrt(0.0f, 0.0f, 0.0f);
}

int num_projs(LR_Type type) {
  if (type == LR_Type::LR_20) 
    return 160;
  else if (type == LR_Type::LR_24) 
    return 128;
  return 256;  // default LR_0
}

int num_views(LR_Type type) {
  return (type == LR_Type::LR_0 ? 288 : 144);
}

void det_to_phy( int head, int layer, int xcrys, int ycrys, float pos[3]) {
  if (layer == 7) {
    calc_det_to_phy( head, layer, xcrys, ycrys, pos);
    return;
  }
  int i = head * NDOIS * NUM_CRYSTALS_X_Y + layer * NUM_CRYSTALS_X_Y + xcrys * NYCRYS + ycrys;
  pos[0] = (float)crystal_xpos_[i];
  pos[1] = (float)crystal_ypos_[i];
  pos[2] = (float)crystal_zpos_[i];
}

}
