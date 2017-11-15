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

#include <stdlib.h>
#include <string.h>
#include "matrix.h"

// ahc
#include <unistd.h>
#include <linux/swab.h>

void 
SWAB(void *from, void *to, int length)
{
	if (ntohs(1) == 1) swab(from, to, (ssize_t)length);
	else memcpy(to,from,length);
}

void 
SWAW (short *from, short *to, int length)
{
	if (ntohs(1) == 1) swaw((short*)from,to,length);
	else memcpy(to,from,length*2);
} 

float
vaxftohf( unsigned short *bufr, int off)
{
	unsigned int sign_exp, high, low, mantissa, ret;
	unsigned u = (bufr[off+1] << 16) + bufr[off];
	
	if (u == 0) return 0.0;	
	sign_exp = u & 0x0000ff80;
	sign_exp = (sign_exp - 0x0100) & 0xff80; 
	high = u & 0x0000007f;
	low  = u & 0xffff0000;
	mantissa = (high << 16) + (low >> 16);
	sign_exp = sign_exp << 16;
	ret = sign_exp + mantissa;
	return *(float*)(&ret);
}

int 
file_data_to_host(char *dptr, int nblks, int dtype)
{
	int i, j;
	char *tmp = NULL;


	matrix_errno = 0;
	matrix_errtxt[0] = '\0';
	if ((tmp = malloc(512)) == NULL) return ECATX_ERROR;
	switch(dtype)
	{
	case ByteData:
		break;
	case VAX_Ix2:
		if (ntohs(1) == 1) 
			for (i=0, j=0; i<nblks; i++, j+=512) {
				swab( dptr+j, tmp, 512);
				memcpy(dptr+j, tmp, 512);
			}
		break;
	case VAX_Ix4:
		if (ntohs(1) == 1)
			for (i=0, j=0; i<nblks; i++, j+=512) {
				swab(dptr+j, tmp, 512);
				swaw((short*)tmp, (short*)(dptr+j), 256);
			}
		break;
	case VAX_Rx4:
	 	if (ntohs(1) == 1) 
			 for (i=0, j=0; i<nblks; i++, j+=512) {
				swab( dptr+j, tmp, 512);
/* remove next line (fix from Yohan.Nuyts@uz.kuleuven.ac.be, 28-oct-97)
				swaw((short*)tmp, (short*)(dptr+j), 256);
*/
			}
		for (i=0; i<nblks*128; i++)
			((float *)dptr)[i] = vaxftohf( (unsigned short *)dptr, i*2) ;
		break;
	case SunShort:
		if (ntohs(1) != 1)
			for (i=0, j=0; i<nblks; i++, j+=512) {
				swab(dptr+j, tmp, 512);
				memcpy(dptr+j, tmp, 512);
			}
		break;
	case SunLong:
	case IeeeFloat:
		if (ntohs(1) != 1) 
			for (i=0, j=0; i<nblks; i++, j+=512) {
				swab(dptr+j, tmp, 512);
				swaw((short*)tmp, (short*)(dptr+j), 256);
			}
		break;
	default:	/* something else...treat as Vax I*2 */
		if (ntohs(1) == 1)
			for (i=0, j=0; i<nblks; i++, j+=512) {
				swab(dptr+j, tmp, 512);
				memcpy(dptr+j, tmp, 512);
			}
		break;
	}
	free(tmp);
	return ECATX_OK;
}

int
read_matrix_data(FILE *fptr, int strtblk, int nblks, char *dptr, int dtype)
{
	int  err;

	err = mat_rblk( fptr, strtblk, dptr, nblks);
	if (err) return -1;
	return file_data_to_host(dptr,nblks,dtype);
}

int 
write_matrix_data(FILE *fptr, int strtblk, int nblks, char *dptr, int dtype)
{
	int error_flag = 0;
	int i, j;
	char *bufr1 = NULL, *bufr2 = NULL;

    /* printf("\n\n\n\n HEY2"); fflush(stdout); */
        
	matrix_errno = 0;
	matrix_errtxt[0] = '\0';
	if ( (bufr1 = malloc(512)) == NULL) return ECATX_ERROR;
	if ( (bufr2 = malloc(512)) == NULL) {
		free(bufr1);
		return ECATX_ERROR;
	}
	switch( dtype)
	{
	case ByteData:
		if ( mat_wblk( fptr, strtblk, dptr, nblks) < 0) error_flag++;
		break;
	case VAX_Ix2: 
	default:	/* something else...treat as Vax I*2 */
		if (ntohs(1) == 1) {
			for (i=0, j=0; i<nblks && !error_flag; i++, j += 512) {
				swab( dptr+j, bufr1, 512);
				memcpy(bufr2, bufr1, 512);
				if ( mat_wblk( fptr, strtblk+i, bufr2, 1) < 0) error_flag++;
			}
		} else if ( mat_wblk( fptr, strtblk, dptr, nblks) < 0) error_flag++;
		break;
	case VAX_Ix4:
	case VAX_Rx4:
        crash("unsupported format\n");
        break;
    case IeeeFloat:
	case SunLong:
		if (ntohs(1) != 1) {
			for (i=0, j=0; i<nblks && !error_flag; i++, j += 512) {
				swab( dptr+j, bufr1, 512);
				swaw( (short*)bufr1, (short*)bufr2, 256);
				if ( mat_wblk( fptr, strtblk+i, bufr2, 1) < 0) error_flag++;
			}
		} else if ( mat_wblk( fptr, strtblk, dptr, nblks) < 0) error_flag++;
		break;
	case SunShort:
		if (ntohs(1) != 1) {
			for (i=0, j=0; i<nblks && !error_flag; i++, j += 512) {
				swab( dptr+j, bufr1, 512);
				memcpy(bufr2, bufr1, 512);
				if ( mat_wblk( fptr, strtblk+i, bufr2, 1) < 0) error_flag++;
			}
		} else if ( mat_wblk( fptr, strtblk, dptr, nblks) < 0) error_flag++;
		break;
	}
	free(bufr1);
	free(bufr2);
	if (error_flag == 0) return ECATX_OK;
	return ECATX_ERROR;
}


/* buf...(...) - functions to handle copying header data to and from a buffer
   in the most type safe way possible; note that i is incremented
   by the size of the item copied so these functions must be
   called in the right order
*/


void 
bufWrite(char *s, char *buf, int *i, int len)
{
   strncpy(&buf[*i], s, len);
    *i += len;
}

void 
bufWrite_s(short val, char *buf, int *i)
{
	union { short s; char b[2]; } tmp;
	tmp.s = val;
	if (ntohs(1) != 1) swab(tmp.b,&buf[*i],2);
    else memcpy(&buf[*i], tmp.b, sizeof(short));
    *i += sizeof(short);
}

void 
bufWrite_i(int val, char *buf, int *i)
{
	union { int i; char b[4]; } tmp;
	union { short s[2]; char b[4]; } tmp1;
	tmp.i = val;
	if (ntohs(1) != 1) {
		swab(tmp.b,tmp1.b,4);
		swaw(tmp1.s,(short*)&buf[*i],2);
	} else memcpy(&buf[*i], tmp.b, sizeof(int));
    *i += sizeof(int);
}

void 
bufWrite_u(unsigned int val, char *buf, int *i)
{
	union { int u; char b[4]; } tmp;
	union { short s[2]; char b[4]; } tmp1;
	tmp.u = val;
	if (ntohs(1) != 1) {
		swab(tmp.b,tmp1.b,4);
		swaw(tmp1.s,(short*)&buf[*i],2);
	} else memcpy(&buf[*i], tmp.b, sizeof(unsigned int));
    *i += sizeof(unsigned int);
}

void
bufWrite_f(float val, char *buf, int *i)
{
	union { float f; char b[4]; } tmp;
	union { short s[2]; char b[4]; } tmp1;
	tmp.f = val;
	if (ntohs(1) != 1) {
		swab(tmp.b,tmp1.b,4);
		swaw(tmp1.s,(short*)&buf[*i],2);
	} else memcpy(&buf[*i], tmp.b, sizeof(float));
    *i += sizeof(float);
}

void 
bufRead(char *s, char *buf, int *i, int len)
{
    strncpy(s, &buf[*i], len);
    *i += len;
}

void
bufRead_s(short *val, char *buf, int *i)
{
	union { short s; unsigned char b[2]; } tmp, tmp1;
	memcpy(tmp.b,&buf[*i],2);
	if (ntohs(1) != 1) {
		swab((char*)tmp.b,(char*)tmp1.b,2);
		*val = tmp1.s;
	} else *val = tmp.s;
    *i += sizeof(short);
}

void 
bufRead_i(int *val, char *buf, int *i)
{
	union {int i; unsigned char b[4]; } tmp, tmp1;
	memcpy(tmp1.b,&buf[*i],4);
	if (ntohs(1) != 1) {
		swab((char*)tmp1.b,(char*)tmp.b,4);
		swaw((short*)tmp.b,(short*)tmp1.b,2);
	}
	*val = tmp1.i;
    *i += sizeof(int);
}

void 
bufRead_u(unsigned int *val, char *buf, int *i)
{
	union {unsigned int u; unsigned char b[4]; } tmp, tmp1;
	memcpy(tmp1.b,&buf[*i],4);
	if (ntohs(1) != 1) {
		swab((char*)tmp1.b,(char*)tmp.b,4);
		swaw((short*)tmp.b,(short*)tmp1.b,2);
	}
	*val = tmp1.u;
    *i += sizeof(unsigned int);
}

void 
bufRead_f(float *val, char *buf, int *i)
{
	union {float f; unsigned char b[2]; } tmp, tmp1;
    memcpy(tmp1.b, &buf[*i], sizeof(float));
	if (ntohs(1) != 1) {
		swab((char*)tmp1.b,(char*)tmp.b,4);
		swaw((short*)tmp.b,(short*)tmp1.b,2);
	}
	*val = tmp1.f;
    *i += sizeof(float);
}
