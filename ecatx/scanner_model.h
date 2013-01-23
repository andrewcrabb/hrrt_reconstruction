/*
   * EcatX software is a set of routines and utility programs
   * to access ECAT(TM) data.
   *
   * Copyright (C) 1996-2008 Merence Sibomana
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
   * modify it under the terms of the GNU General Public License
   * as published by the Free Software Foundation; either version 2
   * of the License, or any later version.
   *
   * This software is distributed in the hope that it will be useful,
   * but  WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.

   * You should have received a copy of the GNU General Public License
   * along with this software; if not, write to the Free Software
   * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef ScannerModel_h
#define ScannerModel_h

typedef struct _ScannerModel {
	char *number;  		/* model number as an ascii string */
	char *model_name;	/* model name as an ascii string  sept 2002   */	
	int dirPlanes;		/* number of direct planes */
	int defElements;	/* default number of elements */
	int defAngles;		/* default number of angles */
	int crystals_per_ring;	/* number of crystal */
	float crystalRad;	/* detector radius in mm*/
	float planesep;		/* plane separation in mm*/
	float binsize;		/* bin size in mm (spacing of transaxial elements) */
	float intrTilt;		/* intrinsic tilt (not an integer please)*/
	int  span;		/* default span 		sept 2002	*/
	int  maxdel;		/* default ring difference 	sept 2002	*/
} ScannerModel;

#if defined(__cplusplus)
extern "C" {
#endif
ScannerModel *scanner_model(int);

#endif /* ScannerModel_h */
