/*-----------------------------------------------------------------
   What: write_image_header.h

    Why: Header file for write_image_header.cpp

    Who: Merence Sibomana

   When: 3/31/2004

Copyright (C) CPS Innovations 2002-2003-2004 All Rights Reserved.
-------------------------------------------------------------------*/

#ifndef write_image_header_h
#define write_image_header_h

#include	<stdlib.h>

// !sv
#ifdef IS__linux__
#define _MAX_PATH 256
#endif

typedef struct {
	char imagefile  [_MAX_PATH];	// image file must exist 
	char truefile   [_MAX_PATH];	// NULL if not used 
	char promptfile [_MAX_PATH];	// NULL if not used
	char delayedfile[_MAX_PATH];	// NULL if not used
	char normfile   [_MAX_PATH];	// NULL if not used
	char attenfile  [_MAX_PATH];	// NULL if not used 
	char scatterfile[_MAX_PATH];	// NULL if not used 
	char BuildTime  [_MAX_PATH];	// build time
	int nx, ny,nz;					// all values positives 
	float dx, dy, dz;				// pixel size 
	int datatype;					// 1=byte, 2=short(integer2), 3=float, other=ERROR 
	int recontype; 					// CPSCC-OSEM3D-(something) 1=UW, 2=ANW, 3=OP 
	int iterations;					// number of iterations
	int subsets;					// number of subsets
}ImageHeaderInfo;

extern int write_image_header(ImageHeaderInfo *info, int psf_flag, 
                              const char *sw_version, const char *sw_build_id,
                              int sino_flag=0);


#endif
