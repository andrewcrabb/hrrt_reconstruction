/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
	10-DEC-2007: Modifications for compatibility with windelays.
  24-MAR-2009: changed filenames extensions to .cpp
               Added short init_geometry_hrrt() with default parameters
  02-JUL-2009: Add Transmission(TX) LUT
*/
#pragma once

enum class LR_Type {
  LR_0, 
  LR_20, 
  LR_24
};

namespace GeometryInfo {
  extern const int NDOIS  ;
  extern const int NXCRYS ;
  extern const int NYCRYS ;
  extern const int NHEADS ;
  extern const int NBLOCKS;
  extern const int NUM_CRYSTALS_X_Y;             // Detectors per plane
  extern const int NUM_CRYSTALS_X_Y_HEADS;       // Detectors per DOI
  extern const int NUM_CRYSTALS_X_Y_HEADS_DOIS;  // Total detectors
  extern const int NUM_CRYSTALS_X_DOIS;

  extern const int NUM_CRYSTALS_PER_BLOCK;   // 8
  extern const int NUM_BLOCKS_PER_BRACKET;   // 9
  extern const int NUM_CRYSTALS_PER_BRACKET; // 72
  extern const int NUM_BRACKETS_PER_RING;    // 8
  extern const int NUM_CRYSTALS_PER_RING;    // 576

  extern const float  CSIZE ;   // the cristal size
  extern const float  CGAP  ;   // the gap between neighboring cristals.
  extern const float  BSIZE ;
  extern const float  BGAP  ;   // the gap between neighboring blocks.
  extern const float  XHSIZE;   // the head's x length.

  extern const float PITCH ;    //=cptich
  extern const float RDIAM ;    //=diam
  extern const float LTHICK;    //=thick
  extern const float TX_RADIUS;

  extern LR_Type LR_type;
};


extern double m_binsize;
extern double *m_sin_head;
extern double *m_cos_head;
extern double m_crystal_radius;
extern double m_plane_sep,  m_crystal_x_pitch, m_crystal_y_pitch;
extern double *m_crystal_xpos;
extern double *m_crystal_ypos;
extern double *m_crystal_zpos;
extern int    m_nprojs;
extern int    m_nviews;
extern int maxrd_, nsino;
extern float *head_crystal_depth;


extern void init_geometry_hrrt ( int np, int nv, float cpitch, float diam, float thick);
//void calc_det_to_phy( int head, int layer, int detx, int dety, float location[3]);
extern void det_to_phy( int head, int layer, int xcrys, int ycrys, float pos[3]);
inline void init_geometry_hrrt() {
  init_geometry_hrrt(0,0,0.0f,0.0f,0.0f);
}

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
