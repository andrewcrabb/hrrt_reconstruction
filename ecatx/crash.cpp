/* @(#)crash.c	1.1 6/7/90 */

#include <stdio.h>
#include <stdlib.h>

void crash( const char *fmt, char *a0, char *a1, char *a2, char *a3, char *a4,
           char *a5, char *a6, char *a7, char *a8, char *a9)
{
	fprintf( stderr, fmt, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9);
	exit(1);
}
