/*-----------------------------------------------------------------
   What: write_image_header.h

    Why: Header file for write_image_header.cpp

    Who: Merence Sibomana

   When: 3/31/2004

Copyright (C) CPS Innovations 2002-2003-2004 All Rights Reserved.
-------------------------------------------------------------------*/

#ifndef write_ecat_image_h
#define write_ecat_image_h

#include	<stdlib.h>
#include "write_image_header.h"

extern int write_ecat_image(float ***image, char * filename, int frame,
                            ImageHeaderInfo *info, int psf_flag,
                            const char *sw_version, const char *sw_build_id);

#endif
