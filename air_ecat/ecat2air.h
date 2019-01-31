#pragma once

/* created on 30-apr-1996 by Merence Sibomana <Sibomana@topo.ucl.ac.be>
 *
 * 1. ecat2air
 * The function ecat2air loads an ecat volume into memory following the
 * AIR2.0 conventions. The ecat volume is specified by the filename and
 * matrix number i.e filename,frame,plane,gate.data,bed
 * (p02000_fdg.img,1,1,1,0,0 for example).
 * Note that the plane number is not significant, all the planes of the
 * frame are loaded if the volume is stored slice by slice as in ECAT
 * version 6 images.
 *
 * 2. air2ecat
 * This function is called by the reslicer to produce a resliced volume
 * in ECAT version 7 format and keep all the header informations.
 * The header information is copied from the volume specified by the
 * orig_specs argument.
 *
 * Modification history: Merence Sibomana <sibomana@gmail.com> 
 * 10-FEB-2010: Added function ecat_AIR_load_probr
*/

// ahc 11/15/17 This was including hrrt_open_2011/include/AIR/AIR.h
// But this was an old version of AIR.h from 2002
// Current AIR 5.3.0 is 2011, so set -I in CMake to AIR src dir.
// #include <AIR/AIR.h>
#include "AIR.h"

#include <ecatx/ecat_matrix.hpp>

#define AIR_ECAT 
#if defined(__cplusplus)
extern "C" {
/*
 * high level user functions
 */
#endif
AIR_Pixels ***ecat2air(const char *specs, struct AIR_Key_info *stats, const AIR_Boolean,  AIR_Error *);
int air2ecat(AIR_Pixels ***pixels, struct AIR_Key_info *stats,
	const char *ecat_specs, int ow, const char* comment, const char *orig_specs);
float ecat_AIR_open_header(const char *, /*@out@*/ struct AIR_Fptrs *, /*@out@*/ struct AIR_Key_info *, int *);
AIR_Pixels ecat_AIR_map_value(const char *filename, long int value, AIR_Error *errcode);
void ecat_AIR_close_header(struct AIR_Fptrs *);
void matrix_flip(MatrixData *matrix,int x_flip, int y_flip, int z_flip);
struct AIR_Air16 *xmldecode_air16(const char *filename);
int xmlencode_air16(const char *outfile, int permission, double **e, int zooming,
                 struct AIR_Air16 *air);
                 AIR_Pixels AIR_map_value(const char *, long int, /*@out@*/ AIR_Error *);

AIR_Error ecat_AIR_do_alignlinear(const char *, const unsigned int, const char *, const AIR_Boolean,
                                  const char *, long int, const float, const float, const float, 
                                  const unsigned int, const AIR_Boolean, /*@null@*/ const char *,
                                  const char *, long int, const float, const float, const float,
                                  const unsigned int, const AIR_Boolean, /*@null@*/ const char *,
                                  /*@null@*/ const char *, /*@null@*/ const char *, /*@null@*/ const char *,
                                  const AIR_Boolean, /*@null@*/ const char *, const AIR_Boolean,
                                  const unsigned int, const unsigned int, const unsigned int, const float,
                                  const unsigned int, const unsigned int, const AIR_Boolean, const AIR_Boolean,
                                  const AIR_Boolean, const unsigned int, const AIR_Boolean);
AIR_Error ecat_AIR_load_probr(const char *specs, const AIR_Boolean decompressable_read);
#if defined(__cplusplus)
}
#endif

#define AIR_load ecat2air
#define AIR_save air2ecat
#define AIR_map_value ecat_AIR_map_value
#define AIR_do_alignlinear ecat_AIR_do_alignlinear
#define AIR_open_header ecat_AIR_open_header
#define AIR_close_header ecat_AIR_close_header
#define AIR_load_probr ecat_AIR_load_probr

