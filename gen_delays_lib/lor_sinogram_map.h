/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
  10-DEC-2007: Modifications for compatibility with windelays.
  11-Nov-2008: Added LUT for 32bit and 64bit compatibility
  17-Mar-2009: Modified init_sol_lut to load only sinogram map
               if seginfo not provided (segzoffset==NULL).
  24-MAR-2009: changed filenames extensions to .cpp
  02-JUL-2009: Add Transmission(TX) LUT
*/
# pragma once

#include <string>
#include <boost/filesystem.hpp>
namespace bf = boost::filesystem;

namespace lor_sinogram {

typedef struct {
  int    nsino;
  float d;
  float z; //plane
} SOL;

static std::string const LUT_FILENAME_ENVT_VAR;

extern short **m_segplane;
extern float *m_c_zpos;  // "physical z position" of Ycrys index.
extern float *m_c_zpos2; // "plane # "            of Ycrys index.
extern SOL ***solution_;
extern SOL ***solution_tx_[]; // 2 tables: A is the source and B is the source

bf::path get_lut_filename(void);
bool valid_lut_filename(bf::path const &t_path);

extern void init_sol(int *segzoffset);
extern void init_sol_tx(int tx_span);
// init_lut_sol: initialize m_solution, m_c_z_pos, m_c_zpos2, m_segplane
// and load lut_filename. Load m_solution only if segzoffset==NULL
// ahc 1/22/19 now get lut_filename within these methods
extern int    init_lut_sol(int *segzoffset);
extern int    save_lut_sol(bf::path const &lut_filename);
extern int init_lut_sol_tx(void);
extern int save_lut_sol_tx(bf::path const &lut_filename);
}
