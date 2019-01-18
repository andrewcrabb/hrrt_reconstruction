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

#include <boost/filesystem.hpp>
namespace bf = boost::filesystem;

typedef struct {
	int    nsino;
	float d;
	float z; //plane 
} SOL;

extern short **m_segplane;
extern float *m_c_zpos;  // "physical z position" of Ycrys index.
extern float *m_c_zpos2; // "plane # "            of Ycrys index.
extern SOL ***m_solution;
extern SOL ***m_solution_tx[]; // 2 tables: A is the source and B is the source

extern void init_sol(int *segzoffset);
extern void init_sol_tx(int tx_span);
// init_lut_sol: initialize m_solution, m_c_z_pos, m_c_zpos2, m_segplane
// and load lut_filename. Load m_solution only if segzoffset==NULL
extern int    init_lut_sol(const boost::filesystem::path &lut_filename, int *segzoffset);
extern int    save_lut_sol(const boost::filesystem::path &lut_filename);
extern int init_lut_sol_tx(const boost::filesystem::path &lut_filename);
extern int save_lut_sol_tx(const boost::filesystem::path &lut_filename);

