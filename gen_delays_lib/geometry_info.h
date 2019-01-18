/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
	10-DEC-2007: Modifications for compatibility with windelays.
  24-MAR-2009: changed filenames extensions to .cpp
               Added short init_geometry_hrrt() with default parameters
  02-JUL-2009: Add Transmission(TX) LUT
*/
#pragma once

#include <istream>
#include <vector>

enum class LR_Type {
  LR_0, 
  LR_20, 
  LR_24,
  MAX_LR_TYPE  // Use this to implement range checks
};

// Allow Boost program_options to parse an LR_Type
// https://stackoverflow.com/questions/5211988/boost-custom-validator-for-enum

extern std::istream& operator>>(std::istream& t_in, LR_Type& t_lr_type);

namespace GeometryInfo {
extern LR_Type LR_type;

const int NUM_ELEMS = 256;
const int NUM_VIEWS = 288;
const int NDOIS   = 2;
const int NXCRYS  = 72;
const int NYCRYS  = 104;
const int NRINGS  = NYCRYS;
const int NHEADS  = 8;
const int NUM_CRYSTALS_X_Y = NXCRYS * NYCRYS;
const int NUM_CRYSTALS_X_Y_HEADS = NUM_CRYSTALS_X_Y * NHEADS;
const int NUM_CRYSTALS_X_Y_HEADS_DOIS = NUM_CRYSTALS_X_Y_HEADS * NDOIS;
const int NUM_CRYSTALS_X_DOIS = NXCRYS * NDOIS;

const int NUM_CRYSTALS_PER_BLOCK = 8;
const int NUM_BLOCKS_PER_BRACKET = 9;
const int NUM_CRYSTALS_PER_BRACKET = NUM_CRYSTALS_PER_BLOCK * NUM_BLOCKS_PER_BRACKET;  // 72
const int NUM_BRACKETS_PER_RING = 8;
const int NUM_CRYSTALS_PER_RING = NUM_CRYSTALS_PER_BRACKET * NUM_BRACKETS_PER_RING;    // 576

const int NBLOCKS = 936;     // NHEADS * 9 * 13
const int MAX_RINGDIFF = 67;

const float  CSIZE  = 0.22;  // Crystal size
const float  CGAP   = 0.02;  // Gap between neighboring crystals
const float  BSIZE  = 8.0 * CSIZE + 7 * CGAP;
const float  BGAP   = 0.05;   // Gap between neighboring blocks
const float  XHSIZE = 9.0 * BSIZE + 8 * BGAP; // Head's x length.

// Crystals and Ring dimensions in cm
const float PITCH     = 0.24375f;    //=cptich
const float RDIAM     = 46.9f;       //=diam
const float LTHICK    = 1.0f;        //=thick
const float TX_RADIUS = 22.357f;
const float CRYSTAL_RADIUS = 23.45;  // cm 

// ahc 1/14/19 
// Moved here from gen_delays.h

const int NMPAIRS = 20;
const std::vector<std::vector<int>> HRRT_MPAIRS{{-1,-1},{0,2},{0,3},{0,4},{0,5},{0,6},
                                                {1,3},{1,4},{1,5},{1,6},{1,7},
                                                {2,4},{2,5},{2,6},{2,7},
                                                {3,5},{3,6},{3,7},
                                                {4,6},{4,7},
                                                {5,7}};

struct LR_Geom {
    int nprojs;
    int nviews;
    double binsize;
    int plane_sep;
};
extern const std::vector<LR_Geom> lr_geometries_;

// Move methods into the namespace as they are processed by the Great Rewrite of 2018-19
extern void init_geometry_hrrt (float cpitch, float diam, float thick);
extern void init_geometry_hrrt(void);

}
extern LR_Type to_lrtype(int i);

// Move member variables into the namespace as they are processed by the Great Rewrite of 2018-19
extern std::vector<double> m_sin_head;
extern std::vector<double> m_cos_head;
}


// namespace GeometryInfo {
//   extern const int NDOIS  ;
//   extern const int NXCRYS ;
//   extern const int NYCRYS ;
//   extern const int NHEADS ;
//   extern const int NBLOCKS;
//   extern const int NUM_CRYSTALS_X_Y;             // Detectors per plane
//   extern const int NUM_CRYSTALS_X_Y_HEADS;       // Detectors per DOI
//   extern const int NUM_CRYSTALS_X_Y_HEADS_DOIS;  // Total detectors
//   extern const int NUM_CRYSTALS_X_DOIS;

//   extern const int NUM_CRYSTALS_PER_BLOCK;   // 8
//   extern const int NUM_BLOCKS_PER_BRACKET;   // 9
//   extern const int NUM_CRYSTALS_PER_BRACKET; // 72
//   extern const int NUM_BRACKETS_PER_RING;    // 8
//   extern const int NUM_CRYSTALS_PER_RING;    // 576

//   extern const float  CSIZE ;   // the cristal size
//   extern const float  CGAP  ;   // the gap between neighboring cristals.
//   extern const float  BSIZE ;
//   extern const float  BGAP  ;   // the gap between neighboring blocks.
//   extern const float  XHSIZE;   // the head's x length.

//   extern const float PITCH ;    //=cptich
//   extern const float RDIAM ;    //=diam
//   extern const float LTHICK;    //=thick
//   extern const float TX_RADIUS;

//   extern LR_Type LR_type;
// };


extern double m_binsize;
extern double m_crystal_radius;
extern double m_plane_sep,  m_crystal_x_pitch, m_crystal_y_pitch;
extern double *m_crystal_xpos;
extern double *m_crystal_ypos;
extern double *m_crystal_zpos;
extern int    m_nprojs;
extern int    m_nviews;
extern int maxrd_, nsino;

// extern float *head_crystal_depth;
extern std::vector<float> head_crystal_depth_;

//void calc_det_to_phy( int head, int layer, int detx, int dety, float location[3]);
extern void det_to_phy( int head, int layer, int xcrys, int ycrys, float pos[3]);

inline int num_projs(LR_Type type) {
  if (type == LR_Type::LR_20) 
    return 160;
  else if (type == LR_Type::LR_24) 
    return 128;
  return 256;  // default LR_0
}

inline int num_views(LR_Type type) {
  return (type == LR_Type::LR_0 ? 288 : 144);
}
