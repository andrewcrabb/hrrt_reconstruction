/*
   * EcatX software is a set of routines and utility programs
   * to access ECAT(TM) data.
   *
   * Copyright (C) 2008 Merence Sibomana
   *
   * Author: Merence Sibomana <sibomana@gmail.com>
   *
   * ECAT(TM) is a registered trademark of CTI, Inc.
   * This software has been written from documentation on the ECAT
   * data format provided by CTI to customers. CTI or its legal successors
   * should not be held responsible for the accuracy of this software.
   * CTI, hereby disclaims all copyright interest in this software.
   * In no event CTI shall be liable for any claim, or any special indirect or 
   * consequential damage whatsoever resulting from the use of this software.
   *
   * This is a free software; you can redistribute it and/or
   * modify it under the terms of the GNU Lesser General Public License
   * as published by the Free Software Foundation; either version 2
   * of the License, or any later version.
   *
   * This software is distributed in the hope that it will be useful,
   * but  WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU Lesser General Public License for more details.

   * You should have received a copy of the GNU Lesser General Public License
   * along with this software; if not, write to the Free Software
   * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "scanner_model.h"
#include <stdio.h>
/*
model number, 
number of direct planes, 
number of elements per projection, 
number of angles or projections, 
number of crystals per ring, 
detector radius in mm, 
plane separation in mm, 
bin size in mm,
intrinsic tilt in degrees (float)

These specs have been confirmed by Mike Casey (26/02/1996)
You can add any future model, from your scanner console using info from ecat7 data_base

from ecat7 console start CAPP by typing capp_restart (idl_send_command deamon) then capp
the following works even in demo_mode (i.e. run_time license)

st = db_getmodelinfo(model_info,model='code')
help,/str,model_info

where 'code' is the CTI model number for the scanner (e.g. 966 for the monster)

Modifications
-------------
December 1996:
intrinsic_tilt sign changed to be compatible with ecat7
change HR tilt value from 12.86 to 13 to be compatible with ecat7
January 1997:
add defAngles in structure to fill attenuation subheader in ecat_osem 
it already accounts for defangularcomp (mash) 1 for models 961, 962 and 966
March 1997:
correct plane separation for 951-953-921-925: 6.75 is 3.375 mm...
May 1997:
correct bin_size for 962-966: 2.25 mm...
November 1997: 
add crystals_per_rings
November 1999:
add HRRT model as 999
September 2002
Add 960 to test span influence on FORE (Defrise simulation package)
Add 963 to test flat data reconstruction from 962

*/
static ScannerModel 
_scanner_model_951 = {"951","ECAT_951",  16, 192, 256,  512, 510.0f, 3.375f,  3.129f,   0.f,1,15}; /* no span in ecat 6*/
static ScannerModel 
_scanner_model_953 = {"953","ECAT_953",  16, 160, 192,  384, 382.5f, 3.375f,  3.129f,  15.f,1,15}; /* no span in ecat 6*/
static ScannerModel
_scanner_model_921 = {"921","EXACT_921", 24, 192, 192,  384, 412.5f, 3.375f,  3.375f,  15.f,7,17}; 
static ScannerModel 
_scanner_model_923 = {"923","EXACT_923", 24, 192, 192,  384, 412.5f, 3.375f,  3.375f,  15.f,7,17};
static ScannerModel
_scanner_model_925 = {"925","ART_925",   24, 192, 192,  384, 412.5f, 3.375f,  3.375f,  15.f,7,17}; 
static ScannerModel 
_scanner_model_960 = {"960","HR sim",    24, 256, 128,  784, 412.0f, 3.125f,  1.65f,    0.f,7,17}; /* simulated - Defrise */
static ScannerModel
_scanner_model_961 = {"961","HR",        24, 336, 196,  784, 412.0f, 3.125f,  1.65f,   13.f,7,17}; 
static ScannerModel 
_scanner_model_962 = {"962","HR+",       32, 288, 144,  576, 412.5f, 2.425f,  2.25f,    0.f,7,17}; 
static ScannerModel 
_scanner_model_963 = {"963","HR+ resamp",32, 256, 128,  576, 412.5f, 2.425f,  2.53125f, 0.f,7,17}; /* resampled - Casey */ 
static ScannerModel 
_scanner_model_966 = {"966","HR++",      48, 288, 144,  576, 412.5f, 2.425f,  2.25f,    0.f,9,40}; /* HR++ */

/* Flat pannels */
static ScannerModel
_scanner_model_998 = {"998","HRRT",      52, 128, 144,  576, 234.5f, 2.4375f, 2.4375f,  0.f,7,38};
static ScannerModel 
_scanner_model_999 = {"999","HRRT",     104, 256, 288, 1152, 234.5f, 1.21875f,1.21875f, 0.f,9,67}; /* also 3,67 */ 

/* User defined model */
static ScannerModel
_scanner_model_ext = {"0","User-defined",0,   0,   0,    0,   0.0f, 0.0f,    0.0f,     0.f,0, 0};  

ScannerModel *
scanner_model(int system_type)
{
	switch(system_type) {
		case 951:
			return (&_scanner_model_951);
		case 953:
			return (&_scanner_model_953);
		case 921:
			return (&_scanner_model_921);
		case 923:
			return (&_scanner_model_923);
		case 925:
			return (&_scanner_model_925);
		case 961:
			return (&_scanner_model_961);
		case 962:
			return (&_scanner_model_962);
		case 966:
			return (&_scanner_model_966);
		case 998:
			return (&_scanner_model_998);
		case 999:
			return (&_scanner_model_999);
		default: 
			{ 	printf(" Model %d unknown, full specs should be given by -M option \n",system_type);
			return (&_scanner_model_ext);
			}
	}
	return 0;
}


