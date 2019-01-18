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
#include "geometry_info.h"


int maxrd_ = GeometryInfo::MAX_RINGDIFF;
int nsino = 0;
//geometry_info.h
double m_binsize;
std::vector<double> m_sin_head(GeometryInfo::NHEADS);
std::vector<double> m_cos_head(GeometryInfo::NHEADS);
double m_crystal_radius;
double m_plane_sep, m_crystal_x_pitch, m_crystal_y_pitch;
double *m_crystal_xpos = NULL;
double *m_crystal_ypos = NULL;
double *m_crystal_zpos = NULL;
int    m_nprojs;
int    m_nviews;
// float *head_crystal_depth = NULL;  This is defined in geomety_info.h

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

LR_Type GeometryInfo::to_lrtype(int intval, std::vector<LR_Type> goodvals) {
  LR_Type ret = MAX_LR_TYPE;
  LR_Type in_lr = static_cast<LR_Type>(intval);
  if ((in_lr >= 0) && (in_lr < LR_Type::MAX_LR_TYPE)) {
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

LR_Type GeometryInfo::to_lrtype(int intval) {
  std::vector<LR_Type> v;
  return(to_lrtype(intval, v));
}

// Define and initialize variables declared in header
LR_Type LR_type = LR_Type::LR_0;
std::vector<float> head_crystal_depth_(GeometryInfo::NHEADS);
std::vector<float> m_crystal_xpos;
std::vector<float> m_crystal_ypos;
std::vector<float> m_crystal_zpos;


const std::map<LR_Type, LR_Geom> GeometryInfo::lr_geometries_ = {
  {LR_Type::LR_0 , {256, 288, GeometryInfo::PITCH / 2.0, GeometryInfo::PITCH / 2.0}},
  {LR_Type::LR_20, {160, 144, 0.2f                     , GeometryInfo::PITCH      }},
  {LR_Type::LR_24, {128, 144, GeometryInfo::PITCH      , GeometryInfo::PITCH      }}
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
      xpos[i] = (float)(GeometryInfo::TX_RADIUS * sint);
      ypos[i] = (float)(GeometryInfo::TX_RADIUS * cost);
    }
  }
  i = head * 72 + detx;
  location[0] = xpos[i];
  location[1] = ypos[i];
  location[2] = (float)(dety * GeometryInfo::PITCH);
}

void calc_det_to_phy( int head, int layer, int detx, int dety, float location[3]) {
  double sint, cost, x, y, z, xpos, ypos;
  int bcrys, blk;

  if (layer == 7) {calc_txsrc_position( head, detx, dety, location); return;}
  sint = m_sin_head[head];
  cost = m_cos_head[head];
  bcrys = detx % 8;
  blk = detx / 8;
  x = blk * (GeometryInfo::BSIZE + GeometryInfo::BGAP) + bcrys * (GeometryInfo::CSIZE + GeometryInfo::CGAP) - GeometryInfo::XHSIZE / 2.0 + GeometryInfo::CSIZE / 2.0; // +GeometryInfo::CGAP/2.0 //dsaint31
//    y = m_crystal_radius+1.0f*(1-layer)+0.5f;
  y = m_crystal_radius + head_crystal_depth_[head] * (1 - layer) + 0.5 * head_crystal_depth_[head];

  bcrys = dety % 8;
  blk = dety / 8;
  z = blk * (GeometryInfo::BSIZE + GeometryInfo::BGAP) + bcrys * (GeometryInfo::CSIZE + GeometryInfo::CGAP); // - GeometryInfo::CGAP-GeometryInfo::CSIZE/2.0 //dsaint31

  xpos = x * cost + y * sint;
  ypos = -x * sint + y * cost;

  location[0] = (float)xpos; // X location
  location[1] = (float)ypos; // Y location
  location[2] = (float)z;    // Z location
}

// Oh ffs np and mv are not used anywhere...
// void init_geometry_hrrt ( int np, int nv, float cpitch, float diam, float thick) {
// Move methods into the namespace as they are processed by the Great Rewrite of 2018-19

void GeometryInfo::init_geometry_hrrt (float cpitch, float diam, float thick) {
  float pitch  = (cpitch > 0.0) ? cpitch : GeometryInfo::PITCH;
  float rdiam  = (diam > 0.0)   ? diam   : GeometryInfo::RDIAM;
  float lthick = (thick > 0.0)  ? thick  : GeometryInfo::LTHICK;

  for (int i = 0; i < GeometryInfo::NHEADS; i++) {
    m_sin_head[i] = sin(i * 2.0 * M_PI / GeometryInfo::NHEADS);
    m_cos_head[i] = cos(i * 2.0 * M_PI / GeometryInfo::NHEADS);
  }
  m_crystal_radius = rdiam / 2.0;
  m_crystal_x_pitch = m_crystal_y_pitch = pitch;
  m_nprojs = GeometryInfo::lr_geometries_[GeometryInfo::LR_type].nprojs;
  m_nviews = GeometryInfo::lr_geometries_[GeometryInfo::LR_type].nviews;
  switch (GeometryInfo::LR_type) {
  case LR_Type::LR_0:
    m_binsize   = pitch / 2.0;
    m_plane_sep = pitch / 2.0;
    break;
  case LR_Type::LR_20:
    m_binsize   = 0.2f; // 2mm
    m_plane_sep = pitch;
    break;
  case LR_Type::LR_24:
    m_binsize   = pitch;
    m_plane_sep = pitch;
    break;
  }

  m_crystal_xpos.reserve(GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS);
  m_crystal_ypos.reserve(GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS);
  m_crystal_zpos.reserve(GeometryInfo::NUM_CRYSTALS_X_Y_HEADS_DOIS);
  head_crystal_depth_.assign(GeometryInfo::NHEADS, lthick);
  printf("  layer_thickness = %0.4f cm\n", lthick);

  float pos[3];
  int i;
  for (int head = 0; head < GeometryInfo::NHEADS; head++) {
    for (int layer = 0; layer < GeometryInfo::NDOIS; layer++) {
      for (int xcrys = 0; xcrys < GeometryInfo::NXCRYS; xcrys++) {
        for (int ycrys = 0; ycrys < GeometryInfo::NYCRYS; ycrys++, i++) {
          calc_det_to_phy( head, layer, xcrys, ycrys, pos);
          m_crystal_xpos[i] = pos[0];
          m_crystal_ypos[i] = pos[1];
          m_crystal_zpos[i] = pos[2];
        }
      }
    }
  }

  printf("Geometry configured for HRRT (%d heads of radius %0.2f cm.)\n", GeometryInfo::NHEADS, m_crystal_radius);
  printf("  crystal x,y pitches (cm) = %0.4f, %0.4f\n", pitch, pitch);
  printf("  x_head_center = %0.4f cm\n", pitch * (GeometryInfo::NXCRYS - 1) / 2.0);
}

void GeometryInfo::init_geometry_hrrt(void) {
  GeometryInfo::init_geometry_hrrt(0.0f, 0.0f, 0.0f);
}

void det_to_phy( int head, int layer, int xcrys, int ycrys, float pos[3]) {
  int i;
  if (layer == 7) {
    calc_det_to_phy( head, layer, xcrys, ycrys, pos);
    return;
  }
  i = head * GeometryInfo::NDOIS * GeometryInfo::NUM_CRYSTALS_X_Y + layer * GeometryInfo::NUM_CRYSTALS_X_Y + xcrys * GeometryInfo::NYCRYS + ycrys;
  pos[0] = (float)m_crystal_xpos[i];
  pos[1] = (float)m_crystal_ypos[i];
  pos[2] = (float)m_crystal_zpos[i];
}
