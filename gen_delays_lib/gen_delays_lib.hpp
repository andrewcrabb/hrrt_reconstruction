/* Authors: Inki Hong, Dsaint31, Merence Sibomana (MS)
  Creation 08/2004
  Modification history: Merence Sibomana
  10-DEC-2007: Modifications for compatibility with windelays.
  24-MAR-2009: changed filenames extensions to .cpp
  11-MAY-2009: Add span and max ring difference parameters
  02-JUL-2009: Add Transmission(TX) LUT
  2/14/13 ahc: Made Rebinner LUT file a required argument.
*/
# pragma once

#include <cstdio>

#include "geometry_info.h"

int gen_delays(int is_inline,
               float scan_duration,
               float ***result,
               FILE *p_coins_file,
               char *p_delays_file,
               int span = 9,
               int t_maxrd = GeometryInfo::MAX_RINGDIFF
               // boost::filesystem::path const &p_rebinner_lut_file
              );
// const char *hrrt_rebinner_lut_path(int tx_flag = 0);

// gen_delays_lib is used in gen_delays and lmhistogram_mp executables.
// In the hpp containing main() of these, define these globals, which may be set at command line (gen_delays) or use defaults (lmhistogram_mp)

extern int g_num_elems;
extern int g_num_views;
extern int g_span;
extern int g_max_ringdiff;
extern float g_pitch;
extern float g_diam;
extern float g_thick;
extern float g_tau;
extern float g_ftime;
