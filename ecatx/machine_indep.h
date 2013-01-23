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
#ifndef machine_indep_h
#define machine_indep_h

#include <stdio.h>


#if defined(__cplusplus)
extern "C" {
#endif
void SWAB(void *from, void *to, int length);
int file_data_to_host(char *dptr, int nblks, int dtype);
int read_matrix_data( FILE *fptr, int strtblk, int nblks,
					 char *dptr, int dtype);
int write_matrix_data( FILE *fptr, int strtblk, int nblks,
					 char *dptr, int dtype);
void bufWrite(char* s, char* buf, int* i, int len);
void bufWrite_s(short val, char* buf, int* i);
void bufWrite_i(int val, char* buf, int* i);
void bufWrite_u(unsigned int val, char* buf, int* i);
void bufWrite_f(float val, char* buf, int* i);
void bufRead(char* s, char* buf, int* i, int len);
void bufRead_s(short*, char* buf, int* i);
void bufRead_i(int*, char* buf, int* i);
void bufRead_u(unsigned int*, char* buf, int* i);
void bufRead_f(float*, char* buf, int* i);
#if defined(__cplusplus)
}
#endif /* __cpluplus */

#endif  /* machine_indep_h */
