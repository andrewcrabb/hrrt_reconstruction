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

#include "num_sort.h"
#include <stdlib.h>

int 
compare_short(const void *a, const void *b)
{
	short sa = *(short*)a, sb = *(short*)b;
    if (sa < sb) return(-1);
    else if (sa > sb) return (1);
    else return (0);
}
int compare_int(const void *a, const void *b)
{
	int ia = *(int*)a, ib = *(int*)b;
    if (ia < ib) return(-1);
    else if (ia > ib) return (1);
    else return (0);
}

int sort_short(short *v, int count)

{
    qsort(v, count, sizeof(short), compare_short);
    return 1;
}

int sort_int(int *v, int count)
{
    qsort(v, count, sizeof(int), compare_int);
    return 1;
}
