/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
	10-DEC-2007: Modifications for compatibility with windelays.
  24-MAR-2009: changed filenames extensions to .cpp
               Added short init_geometry_hrrt() with default parameters
  02-JUL-2009: Add Transmission(TX) LUT
*/
#pragma once

#define NDOIS   2
#define NXCRYS  72
#define NYCRYS  104
#define NHEADS  8
#define NBLOCKS 936//117*8 //NHEADS*9*13

#define CSIZE  0.22  // the cristal size
#define CGAP   0.02  // the gap between neighboring cristals.
#define BSIZE  (8*CSIZE+7*CGAP)
#define BGAP   0.05   // the gap between neighboring blocks.
#define XHSIZE (9*BSIZE+8*BGAP) // the head's x length.

typedef enum {LR_0, LR_20, LR_24} LR_Type;

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
extern int maxrd, nsino;
extern float *head_crystal_depth;
extern LR_Type LR_type;

extern void init_geometry_hrrt ( int np, int nv, float cpitch, float diam, float thick);
//void calc_det_to_phy( int head, int layer, int detx, int dety, float location[3]);
extern void det_to_phy( int head, int layer, int xcrys, int ycrys, float pos[3]);
inline void init_geometry_hrrt() 
{
  init_geometry_hrrt(0,0,0.0f,0.0f,0.0f);
}

inline int num_projs(LR_Type type)
{
  if (type==LR_20) return 160;
  else if (type==LR_24) return 128;
  return 256;  // default LR_0
}
inline int num_views(LR_Type type)
{
  return (type==LR_0? 288:144);
}
