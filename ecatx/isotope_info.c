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
 	/* Isotope information */

#include  "isotope_info.h"
#include <string.h>
#include <strings.h>

	/* ALWAYS add new isotopes to the END of the following array to
	   ensure backward compatibility */

static struct IsotopeInfoItem _isotope_info[] = {
	{ "Br-75",  0.755f, 5880.0f},                 /* 98.0 min */
	{ "C-11",  0.9976f, 1224.0f},                 /* 20.4 min */
	{ "Cu-62",  0.980f, 583.8f},                  /* 9.73 min */
	{ "Cu-64", 0.184f, 46080.0f},                   /* 12.8 hours */
	{ "F-18", 0.967f, 6588.0f},                   /* 1.83 hours */
	{ "Fe-52", 0.57f, 298800.0f},                 /* 83.0 hours */
	{ "Ga-68", 0.891f, 4098.0f},                  /* 68.3 min */
	{ "Ge-68",  0.891f, 23760000.0f},             /* 275.0 days */
	{ "N-13",  0.9981f, 598.2f},                  /* 9.97 min */
	{ "O-14",  1.0f, 70.91f},                     /* 70.91 sec */
	{ "O-15", 0.9990f, 123.0f},                   /* 123.0 sec */
	{ "Rb-82", 0.950f, 78.0f},                    /* 78.0 sec */
	{ "Na-22",  0.9055f, 82080000.0f},            /* 950 days */
	{ "Zn-62",  0.152f, 33480.0f},                /* 9.3 hours */
	{ "Br-76",  0.57f, 58320.0f},                 /* 16.2 hours */
	{ "K-38", 1.0f, 458.16f},                     /* 7.636 min  */
	{ NULL, 0.0f, 0.0f}
};


struct IsotopeInfoItem *get_isotope_info(char *code)
{
  struct IsotopeInfoItem *p = _isotope_info;
  while (p->name != NULL)
  {
    if (strcasecmp(p->name, code)== 0) return p;
    p++;
  }
  return NULL;
}
