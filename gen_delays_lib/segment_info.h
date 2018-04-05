/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
	10-DEC-2007: Modifications for compatibility with windelays.
  29-JAN-2009: Add clean_segment_info()
               Replace segzoffset2 by m_segzoffset_span9
  24-MAR-2009: changed filenames extensions to .cpp
  02-JUL-2009: Add Transmission(TX) LUT
*/
# pragma once

extern double m_d_tan_theta, m_d_tan_theta_tx;
extern  int m_nsegs, *m_segz0, *m_segzmax, *m_segzoffset, *m_segzoffset_span9, 
        conversiontable[];


extern int init_segment_info(int *nsegs,int *nplanes,double *d_tan_theta,
                             int maxrd,int span, int nycrys,
                             double crystal_radius,double plane_sep);
extern void clean_segment_info();
