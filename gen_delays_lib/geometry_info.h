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
#include <array>
#include <map>

// Allow Boost program_options to parse an LR_Type
// https://stackoverflow.com/questions/5211988/boost-custom-validator-for-enum


namespace GeometryInfo {
enum class LR_Type {
  LR_0, 
  LR_20, 
  LR_24,
  MAX_LR_TYPE  // Use this to implement range checks
};
extern std::istream& operator>>(std::istream& t_in, LR_Type& t_lr_type);
extern LR_Type lr_type_;

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
    int   nprojs;
    int   nviews;
    float binsize;
    float plane_sep;
};
extern const std::map<LR_Type, LR_Geom> lr_geometries_;

// Move methods into the namespace as they are processed by the Great Rewrite of 2018-19
extern void init_geometry_hrrt (float cpitch, float diam, float thick);
extern void init_geometry_hrrt(void);
extern LR_Type to_lrtype(int i);

// Move member variables into the namespace as they are processed by the Great Rewrite of 2018-19
extern std::vector<double> sin_head_;
extern std::vector<double> cos_head_;

extern double binsize_;
extern double crystal_radius_;
extern double plane_sep_, crystal_x_pitch_, crystal_y_pitch_;

extern std::array<double, NUM_CRYSTALS_X_Y_HEADS_DOIS> crystal_xpos_;
extern std::array<double, NUM_CRYSTALS_X_Y_HEADS_DOIS> crystal_ypos_;
extern std::array<double, NUM_CRYSTALS_X_Y_HEADS_DOIS> crystal_zpos_;

extern int    nprojs_;
extern int    nviews_;
extern int    maxrd_, nsino;

// extern float *head_crystal_depth;
extern std::array<float, NHEADS> head_crystal_depth_;

//void calc_det_to_phy( int head, int layer, int detx, int dety, float location[3]);
extern void det_to_phy( int head, int layer, int xcrys, int ycrys, float pos[3]);
int num_views(LR_Type type);
int num_projs(LR_Type type);

} // namespace GeometryInfo



