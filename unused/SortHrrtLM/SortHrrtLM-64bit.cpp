/* $Id: SortHrrtLM-64bit.c,v 1.17 2009/09/30 17:23:44 peter Exp $ */
/*
Program:	SORTHRRTLM-64BIT
To Compile:
		make SortHrrtM-64bit
To Use:
		SortHrrtM-64bit -h
Author:
		P.M.Bloomfield CAMH, Merence Sibomana & Larry G Byars CPS
Date:
		16-September-2005
Note:
History:
$Log: SortHrrtLM-64bit.c,v $
Revision 1.17  2009/09/30 17:23:44  peter
Added commented out hooks to lut rebinner method for sorting

Revision 1.16  2009/08/06 16:09:04  peter
Added code to zero CoinMapPr and CoinMapDel memory prior to frame 1. Already cleared for subsequent frames.

Revision 1.15  2009/08/06 15:51:55  peter
Changed the declarations for CoinMapPr and CoinMapDel to 'int', long on 64-bit machine is different to 32-bit machine

Revision 1.14  2009/08/05 17:40:47  peter
Modified subroutine WriteDataToDisk() to create either float or short int sinograms.

Revision 1.13  2009/04/02 14:10:53  peter
Changed the code so that both prompt and delayed sinograms are sorted in memory, and dependent on the switch
-m any combination of sinograms can be written to disk.

Revision 1.12  2009/03/18 18:49:26  peter
Added code to build and write to disk coincidence map, used by gen_delays

Revision 1.11  2009/03/16 17:35:36  peter
Addded code to generate prompt and delated coincidence maps; need to check that match lmhistogram.exe maps

Revision 1.10  2006/05/25 20:03:31  peter
Added include library stdlib.h for gcc 4.1

Revision 1.9  2006/04/11 19:24:22  peter
Close sinogram header on completion of write

Revision 1.8  2005/09/28 15:09:15  peter
Cleared up single processing and information printed to screen

Revision 1.7  2005/09/27 18:53:24  peter
Corrected author list in help listings.

Revision 1.6  2005/09/27 18:50:46  peter
Corrected program name in help listing etc.

Revision 1.5  2005/09/27 15:54:11  peter
Added additional keywords 'quantification units' and 'applied corrections' to the out sinogram interfile header

Revision 1.4  2005/09/20 14:55:55  peter
Changed code so that the singles calculation is consistent to that from Merence and sorting data on the acquisition console

Revision 1.3  2005/09/19 15:32:21  peter
Tidied code, and checked byte stripping against CPS

Revision 1.2  2005/09/16 18:57:48  peter
Added in CPS code for 64-bit listmode sort, compiles executes and generates correct looking sinograms.

Revision 1.1  2005/09/16 14:03:34  peter
Initial revision
*/
/*	SCCS I.D. String */
static char sccsid[]="$Id: SortHrrtLM-64bit.c,v 1.17 2009/09/30 17:23:44 peter Exp $ Peter M. Bloomfield { CAMH } + Merence Sibomana & Larry G Byars { CPS }" ;
static char version[]="$Id: SortHrrtLM-64bit.c,v 1.17 2009/09/30 17:23:44 peter Exp $" ;

/*	Include Header Definition Files		*/
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#include <string.h>
#else
#include <io.h>
#include <direct.h>
#include <string.h>
#endif
#include <math.h>

#ifdef WIN32
#define stat _stat
#define access _access
#define unlink _unlink
#define strcasecmp _stricmp
#define F_OK 0
#define M_PI 3.14159265358979323846

extern int getopt(int argc, char *argv[], char *opstring);
extern char *optarg;
extern int   optind;
extern int   opterr;

inline float rintf(float x)
{
return (float)floor(x+.5);
}

#endif 

/*	HRRT Specific Header Definition		*/
#include "HrrtLM64Bit.h"

/*****************************************************************

	Routines for run-time error handling

*****************************************************************/
void errtxt(char *error_text)
{
	fprintf(stderr,"\n... Run time error ...\n");
	fprintf(stderr,"%s %s\n"," ", error_text);
	fprintf(stderr,"... Exiting to system ...\n");
	exit(1);
}

/*****************************************************************

	Routine to display program argument list

*****************************************************************/
void ShowHrrtLM_Help(/*	Arguments NOT Used	*/)
{
	printf( "\n" ) ;
	printf( "/******************************************************************************************************/\n" ) ;
	printf( "\n\n\tProgram Name/Version:\t%s\n", version ) ;
	printf( "\tAuthor:\t\t\tP.M.Bloomfield, Merence Sibomana & Larry G Byars CPS\n" ) ;
	printf( "\tOrganisation:\t\tCAMH & CPS\n" ) ;
	printf( "\n\tUsage:\n" ) ;
	printf( "\tSortHrrtLM-64bit -lsftdmcvh\n" ) ;
	printf( "\n\twhere:\n" ) ;
	printf( "\t\t-l <ListMode Filename>\n" ) ;
	printf( "\t\t-s <Sinogram Filename>\n" ) ;
	printf( "\t\t-f Frame Definition Filename\n" ) ;
	printf( "\t\t-t Span\n" ) ;
	printf( "\t\t-d Ring Difference\n" ) ;
	printf( "\t\t-m <Sinogram Mode [1]>\n" ) ;
	printf( "\t\t-c <Write Coincidence Map --> Disk [0]>\n" ) ;
	printf( "\t\t-v <Verbose Flag [0]>\n" ) ;
	printf( "\t\t-h <This List>\n" ) ;
	printf( "\n" ) ;
	printf( "\tNotes:\n" ) ;
	printf( "\n\t1\tThe -m flag defines the sinogram mode\n" ) ;
	printf( "\t\t\t1 ==> Trues Sinogram\n" ) ;
	printf( "\t\t\t3 ==> Prompts & Trues Sinogram\n" ) ;
	printf( "\t\t\t7 ==> Prompts, Delayed & Trues  Sinogram\n" ) ;
	printf( "\n" ) ;
	printf( "\n" ) ;
	printf( "/******************************************************************************************************/\n" ) ;
	exit( 0 ) ;
}

#ifndef _LUT_
int sort3d_event_addr( int mp, int alayer, int ax, int ay, int blayer, int bx, int by )
{
	float	deta_pos[ 3 ] ,
		detb_pos[ 3 ] ,
		pro[ 4 ] ;

	int	ahead ,
		bhead ;

	ahead = mpairs[ mp ][ 0 ] ;
	bhead = mpairs[ mp ][ 1 ] ;
	calc_det_to_phy( ahead, alayer, ax, ay, deta_pos ) ;
	calc_det_to_phy( bhead, blayer, bx, by, detb_pos ) ;
	phy_to_pro( deta_pos, detb_pos, pro ) ;
	return( pro_to_addr( pro ) ) ;
}

void calc_det_to_phy( int head, int layer, int detx, int dety, float location[ 3 ] )
{
	double	sint ,
		cost ,
		x ,
		y ,
		z ,
		xpos ,
		ypos ;

	int	bcrys ,
		blk ;

		//if (layer==7) {calc_txsrc_position( head, detx, dety, location); return;}
	sint = sin_head[ head ] ;
	cost = cos_head[ head ] ;
	bcrys = detx % 8 ;
	blk = detx / 8 ;
	x = blk * ( BSIZE + BGAP ) + bcrys * ( CSIZE + CGAP ) - XHSIZE / 2 + CSIZE / 2 ;
		//    y = crystal_radius+koln_lthick[head]*(1-layer)+koln_irad[head];
	y = crystal_radius + Head_Crystal_Depth[ head ] * ( 1 - layer ) + 0.5 * Head_Crystal_Depth[ head ] ;
	bcrys = dety % 8 ;
	blk = dety / 8 ;
	z = blk * ( BSIZE + BGAP ) + bcrys * ( CSIZE + CGAP ) ;
	xpos = x * cost + y * sint ;
	ypos = -x * sint + y * cost ;
	location[ 0 ] = ( float )xpos;		// X location
	location[ 1 ] = ( float )ypos ;		// Y location
	location[ 2 ] = ( float )z ;		// Z location
}

#endif
//  This procedure converts the line between 2 detector physical
//  coordinates (deta and detb are 3 value arrays, x, y, z in cm)
//  and LOR projection coordinates (4 value array with values of
//  r, phi, z, tan_theta in appropriate units (cm, radians, cm, value).
void phy_to_pro( float deta[ 3 ], float detb[ 3 ], float projection[ 4 ] )
{
	double	dx ,
		dy ,
		dz ,
		d ;

	float	r ,
		phi ,
		tan_theta ,
		z ,
		pi = ( float )M_PI ;

	dz = detb[ 2 ] - deta[ 2 ] ;
	dy = deta[ 1 ] - detb[ 1 ] ;
	dx = deta[ 0 ] - detb[ 0 ] ;
	d = sqrt( dx*dx + dy*dy ) ;
	r = ( float )( ( deta[ 1 ] * detb[ 0 ] - deta[ 0 ] * detb[ 1 ] ) / d ) ;
		//    if (dx == 0.0) phi = 0.0; else
	phi = ( float )( atan2( dx, dy ) ) ;
	tan_theta = ( float )( dz / d ) ;
	z = ( float )( deta[ 2 ] + ( deta[ 0 ] * dx + deta[ 1 ] * dy ) * dz / ( d * d ) ) ;
	if ( phi < 0.0 )
	{
		phi += pi ;
		r *= -1.0 ;
		tan_theta *= -1.0 ;
	}
	if ( phi == pi )
	{
		phi=0.0 ;
		r *= -1.0 ;
		tan_theta *= -1.0 ;
	}
	projection[ 0 ] = r ;
	projection[ 1 ] = phi ;
	projection[ 2 ] = z ;
	projection[ 3 ] = tan_theta ;
}

int pro_to_addr( float pro[ 4 ] )
{
	int	bin ,
		view ,
		plane ,
		segment ,
		segnum ,
		addr = -1 ,
		segment9 ,
		segnum9 ;

	float	r ,
		phi ,
		z ,
		tan_theta ;

	r = pro[ 0 ] ;
	phi = pro[ 1 ] ;
	z = pro[ 2 ] ;
	tan_theta = pro[ 3 ] ;
	bin = ( int )( nprojs / 2 + r / binsize + 0.5 ) ;
	view = ( int )( nviews * phi / M_PI ) ;
	plane = ( int )( 0.5 + z / plane_sep ) ;
		// segment = (int)(floor(z/d_tan_theta+0.5));
		// avoid floor by 1024 shift to get int of number positive and substract 1024
	segment = ( ( int )( pro[ 3 ] / d_tan_theta + 1024.5 ) ) - 1024 ;
	segnum = 2 * abs( segment ) ;
	if ( segment < 0 ) segnum-- ;
	if ( ( bin < 0 ) || ( bin > nprojs - 1 ) ) return( -1 ) ;
	if ( segnum > nsegs - 1 ) return( -1 ) ;
	if ( plane < segz0[ segnum ] ) return( -1 ) ;
	if ( plane > segz0[ segnum ] +segnz[ segnum ] - 1 ) return( -1 ) ;
	if ( !span1_flag )
	{
		if ( span9_segz0 == NULL )
			plane = ( plane - segz0[ segnum ] ) + segzoff[ segnum ] ;
		else
		{ // span 9
			segment9 = ( (int )( tan_theta / span9_d_tan_theta + 1024.5 ) ) - 1024 ; 
			segnum9 = 2 * abs( segment9 ) ;
			if ( segment9 < 0 ) segnum9-- ;
			plane = ( plane - span9_segz0[ segnum9 ] ) + span9_segzoff[ segnum9 ] ;
		}
	}
	else 
	{	seg_plane_map[ segnum * 207 + plane ]++ ;
		plane = ( plane - segz0[ segnum ] - 1 ) / 2 + segzoff[ segnum ] ;
	}
	addr = bin + nprojs * view + nprojs * nviews * plane ;
	return( addr ) ;
}

#ifndef _LUT_
int init_sort3d_hrrt( int span, int maxrd, int nv, int np, float diam, float crys_depth )
{

    Init_Geometry_HRRT( np, nv, 0.0, diam, crys_depth ) ;
    Init_Seginfo( nycrys, span, maxrd ) ;
    return( nplanes ) ;
}

void Init_Geometry_HRRT ( int np, int nv, float cpitch, float diam, float thick )
{
	int	i ;

	float	pitch = 0.24375f ,
		rdiam = 46.9f ,
		lthick = 1.0f ,
		tx_radius = 22.357f ,   //Transmission Source Radius (cm)
		pos[ 3 ] ;

	int	head ,
		layer ,
		xcrys ,
		ycrys ,
		ncrystals ;

	nheads = 8 ;
	nxcrys = 72 ;
	nlayers = 2 ;
	nycrys = 104 ;
	nprojs = np ;
	nviews = nv ;
	if ( cpitch > 0.0 ) pitch = cpitch ;
	if ( diam > 0.0 ) rdiam = diam ;
	if ( thick > 0.0 ) lthick = thick ;
	sin_head = ( double* )calloc( nheads, sizeof( double ) ) ;
	cos_head = ( double* )calloc( nheads, sizeof( double ) ) ;
	for (i=0; i<nheads; i++)
	{
		sin_head[ i ] = sin( 2.0 * M_PI * i / nheads ) ;
		cos_head[ i ] = cos( 2.0 * M_PI * i / nheads ) ;
		//        printf("Head %d sin = %f, cos = %f\n", i, sin_head[i], cos_head[i]);
	}
	crystal_x_pitch = pitch ;
	crystal_y_pitch = pitch ;
	x_head_center = pitch * ( nxcrys - 1 ) / 2.0 ;
	crystal_radius = rdiam / 2.0 ;
	binsize = pitch / 2.0 ;
	plane_sep = pitch / 2.0 ;
	mpairs = hrrt_mpairs ;
	nmpairs = 20 ;

	ncrystals = nheads * nlayers * nxcrys * nycrys ;
	if ( ( crystal_xpos = ( float* )calloc( ncrystals, sizeof( float ) ) ) == NULL )
	{
		crash( "Can't allocate memory for crystal_xpos array\n" ) ;
	}
	if ( ( crystal_ypos = ( float* )calloc( ncrystals, sizeof( float ) ) ) == NULL )
	{
		crash( "Can't allocate memory for crystal_ypos array\n" ) ;
	}
	if ( ( crystal_zpos = ( float* )calloc( ncrystals, sizeof( float ) ) ) == NULL )
	{
		crash( "Can't allocate memory for crystal_zpos array\n" ) ;
	}
	if ( Head_Crystal_Depth == NULL )
	{
		Head_Crystal_Depth = ( float* )calloc( nheads, sizeof( float ) ) ;
		for ( i = 0 ; i < nheads ; i++ ) Head_Crystal_Depth[ i ] = lthick ;
			//printf("  layer_thickness = %0.4f cm\n", lthick);
	}
	i = 0 ;
	for ( head = 0 ; head < nheads ; head++ )
	{
		for ( layer = 0 ; layer < nlayers ; layer++ )
		{
			for ( xcrys = 0 ; xcrys < nxcrys ; xcrys++ )
			{
				for ( ycrys = 0 ; ycrys < nycrys ; ycrys++ , i++ )
				{
					calc_det_to_phy( head, layer, xcrys, ycrys, pos ) ;
					crystal_xpos[ i ] = pos[ 0 ] ;
					crystal_ypos[ i ] = pos[ 1 ] ;
					crystal_zpos[ i ] = pos[ 2 ] ;
				}
			}
		}
	}
	printf( "\n\t64-Bit Listmode :\n\t\tGeometry configured for HRRT (%d heads of radius %0.2f cm.)\n", nheads, crystal_radius ) ;
	printf( "\t\t\tCrystal x,y pitches (cm) = %0.4f, %0.4f\n", crystal_x_pitch, crystal_y_pitch ) ;
	printf( "\t\t\tHead center [x] = %0.4f cm\n", x_head_center ) ;
	printf( "\t\t\tLayer thickness = %0.4f cm\n", lthick ) ;
}

/*
 *	void Init_Seginfo( int nrings,  int span, int maxrd)
 * Initialize segments information arrays: 
 * segz0[segment] : axial position (Z) of the first sinogram in the segment
 * segnz[segment] : number of sinograms in the segment
 * segzoff[segment] : position of the first sinogram WRT all segment sinogram
 *                  for span 1, only non empty sinograms (segnz[segment]+1)/2) are stored
 */
void Init_Seginfo( int nrings,  int span, int maxrd )
{
	int	i = 0 ,
		np = 2 * nrings - 1 ,
		sp2 = ( span + 1 ) / 2 ,
		segnzs = 0 ,
		segnum = 0 ,
		sign = 1 ;

	if (span<1) crash1("Invalid span %d\n", span);
	if (span > 1) span1_flag = 0;
	else span1_flag = 1;
	maxseg = maxrd / span ;
	nsegs = 2 * maxseg + 1 ;

	if (span9_segz0==NULL)
	{
		printf( "\n\tSegment Definition :\n" ) ;
		printf( "\t\t\t\t# Segments : %d with Maximum Segment Index : %d\n", nsegs, maxseg ) ;
		printf( "\t\tSegment\t\tOrder\t\t z0\t\t  nZ\t\t Zoff\n" ) ;
		printf( "\t\t=======\t\t=====\t\t===\t\t ===\t\t=====\n" ) ;
	}
	segz0 = ( int* )malloc( nsegs * sizeof( int ) ) ;
	segnz = ( int* )malloc( nsegs * sizeof( int ) ) ;
	segzoff = ( int* )malloc( nsegs * sizeof( int ) ) ;
	segz0[ 0 ] = 0 ;
	segnz[ 0 ] = np ;
	segzoff[ 0 ] = 0 ;

	if ( !span1_flag ) nplanes = np ;
	else nplanes = ( np + 1 ) / 2 ;		//only not empty planes
	if ( span9_segz0 == NULL ) printf("\t\t%4d\t\t%3d\t\t%3d\t\t%4d\t\t%5d\n", i, segnum, segz0[ i ], segnz[ i ], segzoff[ i ] ) ;
	for ( i = 1 ; i < nsegs ; i++ )
	{
		sign *= -1 ;
		segnum = sign * ( i + 1 ) / 2 ;
		segz0[ i ] = sp2 + span * ( ( i - 1 ) / 2 ) ;
		segnz[ i ] = np - 2 * segz0[ i ] ;
		segnzs += segnz[ i ] ;
		if ( !span1_flag )segzoff[ i ] = segzoff[ i - 1 ] + segnz[ i - 1 ] ;
		else segzoff[ i ] = segzoff[ i - 1 ] + ( segnz[ i - 1 ] + 1 ) / 2 ;	//only not empty planes
		if ( span9_segz0 == NULL ) printf("\t\t%4d\t\t%3d\t\t%3d\t\t%4d\t\t%5d\n", i, segnum, segz0[ i ], segnz[ i ], segzoff[ i ] ) ;
		if ( !span1_flag ) nplanes += segnz[ i ] ;
		else nplanes  += ( segnz[ i ] + 1 ) / 2 ;				//only not empty planes
	}
	d_tan_theta = span * plane_sep / crystal_radius ;
	if ( span9_segz0 == NULL )
	{
		printf("\t\t\t\tTotal # Sinograms : %d\n", nplanes);
		printf("\t\t\t\t\td_tan_theta = %f\n", d_tan_theta);
	}
	if ( span == 9 )
	{
		span9_d_tan_theta = d_tan_theta ;
		span9_segz0= segz0 ;
		span9_segnz=segnz ;
		span9_segzoff=segzoff ;
		Init_Seginfo( nrings, 3, maxrd ) ;
	}
}
#endif

int SinoInfo( int NumDetRings, int Span, int RingDiff, int *NumSino, int *NumSegments, 
             short *NumSinoPerSeg )
/*
int		NumDetRings ,
		Span ,
		RingDiff ,
		*NumSino ,
		*NumSegments ;

short int	*NumSinoPerSeg ;
*/
{
	char	line[ MAX_STRING_LENGTH ] ;

	int	i1 ,
		SegmentNumber ,
		Sign ;

/* Ensure # Detector Rings, Span and Ring Difference are Consistent */
		/* Ensure that the Ring Difference is Less than the Number of Crystal Rings */
	if ( RingDiff >= NumDetRings )
	{
		sprintf( line, "\n\tRing Difference :%3d MUST Be Less than Number of Detector Rings :%3d\n",
											RingDiff, NumDetRings ) ;
		printf( line ) ;
		return( 1 ) ;
	}
		/* Ensure that the Span Number is Odd */
	if ( !( Span % 2 ) )	 /* If Odd */
	{
		sprintf( line, "\n\tSpan :%3d MUST Be Odd\n", Span ) ;
		printf( line ) ;
		return( 1 ) ;
	}
		/* Ensure Consistency Between Span, Ring Difference and Segment Number */
	for ( SegmentNumber = 0 ; SegmentNumber < MAXSEGNUM ; SegmentNumber++ )
	{
		if ( RingDiff ==  ( ( ( ( ( SegmentNumber * 2 ) + 1 ) * Span ) - 1 ) / 2 ) )
		{
			*NumSegments = SegmentNumber ;
			sprintf( line, "\t\tSupplied span :%4d and ring difference :%3d ---> segment # +/- %d\n",
											Span, RingDiff, *NumSegments ) ;
			printf( line ) ;
			SegmentNumber = MAXSEGNUM ;
		}
	}
		/* If Inconsistent - Report It */
	if ( SegmentNumber == MAXSEGNUM )
	{
		printf( "\n\tRing Difference = ( ( ( ( SegmentNumber * 2 ) + 1 ) * Span ) - 1 ) / 2\n" ) ;
		sprintf( line, "\n\tInconsistency Between Span :%3d and Ring Difference :%3d - See Above Expression\n",
													Span, RingDiff ) ;
		printf( line ) ;
		return( 1 ) ;
	}
		/* Total # Sinograms Defined by # Detector Rings and Ring Difference */
	*NumSino = ( ( ( 2 * ( NumDetRings - 1 ) ) + 1 ) * RingDiff ) - ( RingDiff * RingDiff ) +
											( ( NumDetRings - 1 ) + 1 ) ;
	printf("\t\t%d sinograms defined for a span of %2d, ring difference of %2d and %2d detector rings\n",
										*NumSino, Span, RingDiff, NumDetRings ) ;

/* Number of Planes in Sinogram */
	for ( *NumSino = 0 , SegmentNumber = 0 ; SegmentNumber <= *NumSegments ; SegmentNumber++ )
	{
		if ( SegmentNumber == 0 )
		{
			*( NumSinoPerSeg + SegmentNumber ) = ( 2 * NumDetRings ) - 1 ;
			*NumSino = ( 2 * NumDetRings ) - 1 ;
		}
		else
		{
			for ( i1 = 0 ; i1 <= 1 ; i1++ )
			{
				Sign = 1 ;
				if ( i1 == 0 )
				{
					Sign = -1 ;
				}
				*( NumSinoPerSeg + SegmentNumber ) += ( 2 * NumDetRings ) -
								( ( 2 * abs( Sign * SegmentNumber ) - 1 ) * Span ) - 2 ;
				*NumSino += ( 2 * NumDetRings ) -
								( ( 2 * abs( Sign * SegmentNumber ) - 1 ) * Span ) - 2 ;
			}
		}
	}
	printf( "\t\t\tTotal # sinograms in sorted file :%5d\n", *NumSino ) ;

/* Return Correct Status */
	return( 0 ) ;
}

int UpdateLm(ListMode_64bit *Event64Bit, int Verbose )
/* struct ListMode_64bit	*Event64Bit ;
int	Verbose ;
*/
{
	int	BlockAddr ;

	float	BlockSgls ;

		/* Tag_64 Bit */
	Event64Bit->Tag_64 = ( Event64Bit->EventInfo.Event1 >> 30 ) & 0x1 ;
	if ( Verbose == 1 )
	{
		printf( "Event64Bit->Tag_64 : %d\n", Event64Bit->Tag_64 ) ;
		printf( "\t Event64Bit->EventInfo.Event1 : %8x\n", Event64Bit->EventInfo.Event1 ) ;
		printf( "\t Event64Bit->EventInfo.Event2 : %8x\n", Event64Bit->EventInfo.Event2 ) ;
	}
		/* Sync Bits */
	switch( Event64Bit->Tag_64 )
	{
		case 0:	/* Event Word */
			Event64Bit->EventInfo.AX = Event64Bit->EventInfo.Event1 & 0xFF ;
			Event64Bit->EventInfo.BX = Event64Bit->EventInfo.Event2 & 0xFF ;

			Event64Bit->EventInfo.AY = ( Event64Bit->EventInfo.Event1 & 0x0000FF00 ) >> 8 ;
			Event64Bit->EventInfo.BY = ( Event64Bit->EventInfo.Event2 & 0x0000FF00 ) >> 8 ;

			Event64Bit->EventInfo.XE = ( ( Event64Bit->EventInfo.Event1 & 0x00070000 ) >> 16 ) | ( ( Event64Bit->EventInfo.Event2 & 0x00070000 ) >> 13 ) ;

			Event64Bit->EventInfo.AE = ( Event64Bit->EventInfo.Event1 & 0x00380000 ) >> 19 ;
			Event64Bit->EventInfo.BE = ( Event64Bit->EventInfo.Event2 & 0x00380000 ) >> 19 ;

			Event64Bit->EventInfo.AI = ( ( Event64Bit->EventInfo.Event1 >> 22 ) & 0x1 ) | ( ( ( Event64Bit->EventInfo.Event1 >> 24 ) & 0x1 ) << 2 ) ;
			Event64Bit->EventInfo.BI = ( ( Event64Bit->EventInfo.Event2 >> 22 ) & 0x1 ) | ( ( ( Event64Bit->EventInfo.Event2 >> 24 ) & 0x1 ) << 2 ) ;

			Event64Bit->EventInfo.TF = ( ( Event64Bit->EventInfo.Event1 & 0x0E000000 ) >> 25 ) + ( ( Event64Bit->EventInfo.Event2 & 0x0E000000 ) >> 22 ) ;

			Event64Bit->EventInfo.Res1 = ( Event64Bit->EventInfo.Event1 & 0x70000000 ) >> 28 ;
			Event64Bit->EventInfo.Res2 = ( Event64Bit->EventInfo.Event2 & 0x70000000 ) >> 28 ;

			Event64Bit->EventInfo.Event = ( Event64Bit->EventInfo.Event2 & 0x40000000 ) >> 30 ;

			Event64Bit->EventInfo.PS0 = ( Event64Bit->EventInfo.Event1 & 0x80000000 ) >> 31 ;
			Event64Bit->EventInfo.PS1 = ( Event64Bit->EventInfo.Event2 & 0x80000000 ) >> 31 ;

			Event64Bit->EventInfo.HeadA = mpairs[ Event64Bit->EventInfo.XE ][ 0 ] ;
			Event64Bit->EventInfo.HeadB = mpairs[ Event64Bit->EventInfo.XE ][ 1 ] ;
#ifdef _LUT_
	// LUT
// address = rebin_event( mpe, doia, ax, ay, doib, bx, by /*, type*/) ;
Event64Bit->EventInfo.SinogramAddress = rebin_event( Event64Bit->EventInfo.XE, Event64Bit->EventInfo.AI, Event64Bit->EventInfo.AX, Event64Bit->EventInfo.AY, Event64Bit->EventInfo.BI, Event64Bit->EventInfo.BX, Event64Bit->EventInfo.BY /*, type*/) ;

#else
	// Geometry & trigonometry - Pre 64-bit architecture
Event64Bit->EventInfo.SinogramAddress = sort3d_event_addr( Event64Bit->EventInfo.XE, Event64Bit->EventInfo.AI, Event64Bit->EventInfo.AX, Event64Bit->EventInfo.AY, Event64Bit->EventInfo.BI, Event64Bit->EventInfo.BX, Event64Bit->EventInfo.BY);

#endif

			Event64Bit->EventInfo.Status = 1 ;
			if ( Event64Bit->EventInfo.SinogramAddress == -1 )
			{
				Event64Bit->EventInfo.Status = 0 ;
			}
			if ( Verbose == 1 )
			{
				printf( "\t             Event64Bit->EventInfo.AX : %3d\n", Event64Bit->EventInfo.AX ) ;
				printf( "\t             Event64Bit->EventInfo.BX : %3d\n", Event64Bit->EventInfo.BX ) ;
				printf( "\t             Event64Bit->EventInfo.AY : %3d\n", Event64Bit->EventInfo.AY ) ;
				printf( "\t             Event64Bit->EventInfo.BY : %3d\n", Event64Bit->EventInfo.BY ) ;
				printf( "\t             Event64Bit->EventInfo.XE : %3d\n", Event64Bit->EventInfo.XE ) ;
				printf( "\t             Event64Bit->EventInfo.AE : %3d\n", Event64Bit->EventInfo.AE ) ;
				printf( "\t             Event64Bit->EventInfo.BE : %3d\n", Event64Bit->EventInfo.BE ) ;
				printf( "\t             Event64Bit->EventInfo.AI : %3d\n", Event64Bit->EventInfo.AI ) ;
				printf( "\t             Event64Bit->EventInfo.BI : %3d\n", Event64Bit->EventInfo.BI ) ;
				printf( "\t             Event64Bit->EventInfo.TF : %3d\n", Event64Bit->EventInfo.TF ) ;
				printf( "\t           Event64Bit->EventInfo.Res1 : %3d\n", Event64Bit->EventInfo.Res1 ) ;
				printf( "\t           Event64Bit->EventInfo.Res2 : %3d\n", Event64Bit->EventInfo.Res2 ) ;
				printf( "\t          Event64Bit->EventInfo.Event : %s\n",  Event64Bit->EventInfo.Event ? "Prompt" : "Delayed" ) ;
				printf( "\t            Event64Bit->EventInfo.PS0 : %3d\n", Event64Bit->EventInfo.PS0 ) ;
				printf( "\t            Event64Bit->EventInfo.PS1 : %3d\n", Event64Bit->EventInfo.PS1 ) ;
				printf( "\t          Event64Bit->EventInfo.HeadA : %3d\n", Event64Bit->EventInfo.HeadA ) ;
				printf( "\t          Event64Bit->EventInfo.HeadB : %3d\n", Event64Bit->EventInfo.HeadB ) ;
				printf( "\tEvent64Bit->EventInfo.SinogramAddress : %12d\n", Event64Bit->EventInfo.SinogramAddress ) ;
				printf( "\t Event64Bit->EventInfo.Status : %3d\n\n", Event64Bit->EventInfo.Status ) ;
			}
				/* Event Type */
			Event64Bit->LM_Type = 1 ;
			break ;
		case 1: /* Time TAG, Single TAG, Motion TAG */
			switch ( ( Event64Bit->EventInfo.Event2 & 0x0000E000 ) >> 13 )
			{
				case 4:		/* Time TAG */
					Event64Bit->AcqTime.TagTime = ( ( Event64Bit->EventInfo.Event1 & 0x0000FFFF ) ) + ( ( Event64Bit->EventInfo.Event2 & 0x00001FFF ) << 16 ) ;
					Event64Bit->AcqTime.ms += 1 ;
					if ( Verbose == 2 )
					{
						printf( "\t  Event64Bit->AcqTime.TagTime : %d\t[Acq. Time {Event64Bit->AcqTime.ms} : %d ]\n\n", Event64Bit->AcqTime.TagTime, Event64Bit->AcqTime.ms ) ;
					}
						/* Event Type */
					Event64Bit->LM_Type = 2 ;
					break ;
				case 5:		/* Singles TAG */
						/*
							Decode singles information
								1. Crystal block address : 0-935
									#heads * #CrysBlocksAxial * #CrysBlocksRadial
										==> 8 * 13 * 9 = 936
								2. Singles rate recorded for that address
						*/
					BlockAddr = ( Event64Bit->EventInfo.Event2 & 0x00001FF8 ) >> 3 ;
					BlockSgls = ( float )( ( Event64Bit->EventInfo.Event1 & 0x0000FFFF ) + ( ( Event64Bit->EventInfo.Event2 & 0x00000007 ) << 16 ) ) ;
						/*
							Bucket singles
						*/
					Event64Bit->SglInfo.Sgls[ BlockAddr ][ 0 ] += BlockSgls ;
					Event64Bit->SglInfo.Sgls[ BlockAddr ][ 1 ] += 1. ;
					Event64Bit->SglInfo.Sgls[ BlockAddr ][ 2 ] = Event64Bit->SglInfo.Sgls[ BlockAddr ][ 0 ] / Event64Bit->SglInfo.Sgls[ BlockAddr ][ 1 ] ;
						/*
							Total singles
						*/
					Event64Bit->SglInfo.TotSgls[ 0 ] += BlockSgls ;
					Event64Bit->SglInfo.TotSgls[ 1 ] += 1. ;
					Event64Bit->SglInfo.TotSgls[ 2 ] = Event64Bit->SglInfo.TotSgls[ 0 ] / Event64Bit->SglInfo.TotSgls[ 1 ] ;
					if ( Verbose == 3 )
					{
						printf( "\t Bucket : %3d - Total : %.0f\t# Poll : %.0f\tMean : %.1f\t[System - Total : %.0f # Poll : %.0f Mean : %.1f]\n\n", BlockAddr, Event64Bit->SglInfo.Sgls[ BlockAddr ][ 0 ], Event64Bit->SglInfo.Sgls[ BlockAddr ][ 1 ], Event64Bit->SglInfo.Sgls[ BlockAddr ][ 2 ], Event64Bit->SglInfo.TotSgls[ 0 ], Event64Bit->SglInfo.TotSgls[ 1 ], Event64Bit->SglInfo.TotSgls[ 2 ] ) ;
					}
						/* Event Type */
					Event64Bit->LM_Type = 4 ;
					break ;
			}
			break ;
		default:
					/*
						It should not get here!
						If it does, yikes, 'we have a problem Houston'!!
					*/
			return( 1 ) ;
			break ;
	}
	/* Return ---> routine */
	return( 0 ) ;
}

#ifdef _LUT_
/*
 * Gets  configuration values from GantryModel and calls init_sort3d_hrrt with read or default values.
 * When span=0 (TX mode), span is set to the value from gm328.ini (key rebTxLUTMode0Span) or 21 if key not found,
 * max_rd is set to (span-1)/2.
 * Returns 1 if OK and 0 if gm328.ini not found or if a key is not found.
 */
int Init_Rebinner( int span, int max_rd)
//int init_rebinner( int *span, int *max_rd)
{
	int i=0,  uniform_flag=-1;
	// int *head_type = (int*)calloc(NHEADS, sizeof(int));
	float radius=23.45f;
	float def_depth = 1.0f; // default crystal depth
	int ret=1, tx_flag=0;
	int nplanes=0;
  const char *rebinner_lut_file;


	mpairs = hrrt_mpairs ;
	nmpairs = 20 ;
  em_span = span;
	head_crystal_depth = (float*)calloc(NHEADS, sizeof(float));
	//if (GantryInfo::load(model_number)>0) 
	//{
			// GantryInfo::get("interactionDepth", def_depth);
			// GantryInfo::get("crystalRadius", radius);
			radius = 23.4500 ;
			//for (i=0; i<NHEADS; i++)
			//{
			//	if (!GantryInfo::get("headInfo[%d].type", i, head_type[i])) break;
			//	if (head_type[i] != LSO_LSO && head_type[i] != LSO_GSO && head_type[i] != LSO_LYSO) {
			//		cerr << "Invalid head " << i << " type = " << head_type[i] << endl;
			//		ret=0;
			//		break;
			//	}
			//	if (i==1) {
			//		if (head_type[0]==head_type[1]) uniform_flag=1;
			//		else uniform_flag = 0;
			//	} else if (head_type[i]!=head_type[0]) uniform_flag = 0;
			//		
			//}
			uniform_flag=1;
			//if (i==NHEADS) { // all heads defines
				for (i=0; i<NHEADS; i++)
				{
					//if (head_type[i] == LSO_LYSO) head_crystal_depth[i] = def_depth;
					//else head_crystal_depth[i] = 0.75; //LSO_LSO or LSO_GSO
					head_crystal_depth[i] = def_depth;
				}
			//	if (uniform_flag) cout << "Layer thickness for all heads = " << head_crystal_depth[0] << endl;
			//	else {
			//		cout << "Layer thickness per head = ";
			//		for (i=0; i<NHEADS; i++) 
			//			cout << " " <<head_crystal_depth[i];
			//		cout << endl;
			//	}
			//}
			//else 
			//{
			//	for (i=0; i<NHEADS; i++) head_crystal_depth[i] = def_depth;
			//	cout << "Layer thickness for all heads = " << def_depth << endl;
			//}
			// free(head_type);
			if (span==0)
			{ // Transmission mode
				tx_flag = 1;
				span = 21 ;
				//if (GantryInfo::get("rebTxLUTMode0Span", span))
				//{
				//	cout << "Tramsission mode: using span " << span << " from configuration file" << endl; 
				//}
				//else
				//{
				//	span = 21;
				//	cout << "Tramsission mode: using default span " << span << endl; 
				//}
				tx_span = span;
				//if (LR_type >0 )
				//{
				//	span = span/2+1;  // 21==>11; 9==>5
				//	cout << "Low Resolution mode: span changed to " << span << endl; 
				//}
				max_rd = (span-1)/2;
			}
	//}
	//else
	//{
	//	cerr << "Unable to open gm328.ini configuration file, using default values:" << endl;
	//	cerr << "Layer thickness for all heads = " << def_depth << "cm" << endl;
	//	cerr << "Crystal radius = " << radius << "cm" << endl; 
	//	span = 21;
	//	cout << "Tramsission mode: using default span " << span << endl; 
	//	ret=0;
	//}
	// Init_Geometry_HRRT();
       #ifdef _LUT_
               init_geometry_hrrt();
       #else
               Init_Geometry_HRRT(SINOW, SINOH, 0.0, 2*radius, FXD);
       #endif

#ifndef _LUT_
	Init_Geometry_HRRT(SINOW, SINOH, 0.0, 2*radius, FXD);
#endif
	init_segment_info(&m_nsegs,&nplanes,&m_d_tan_theta,maxrd,span,NYCRYS,m_crystal_radius,m_plane_sep);
	if ((rebinner_lut_file=hrrt_rebinner_lut_path(tx_flag))==NULL)
	{
		fprintf(stdout,"Rebinner LUT file not found\n");		
		exit(1);
	}
	if (!tx_flag) init_lut_sol(rebinner_lut_file, m_segzoffset);
	else init_lut_sol_tx(rebinner_lut_file);
	m_d_tan_theta= (float)(tx_span*m_plane_sep/m_crystal_radius);
	return ret;
}

int rebin_event_tx( int mp, int alayer, int ax, int ay, int blayer, int bx, 
                      int by)
{
  double dx,dy,dz,d;
  float deta[3], detb[3], tan_theta, z;
  int plane, seg, addr=-1, sino_addr;
  int axx, bxx;

  int ahead = hrrt_mpairs[mp][0];
  int bhead = hrrt_mpairs[mp][1];
  det_to_phy( ahead, alayer, ax, ay, deta);
  det_to_phy( bhead, blayer, bx, by, detb);
  dz = detb[2]-deta[2];
  dy = deta[1]-detb[1];
  dx = deta[0]-detb[0];
  d = sqrt(dx*dx + dy*dy);
  tan_theta = (float)(dz / d);
  z = (float)(deta[2]+(deta[0]*dx+deta[1]*dy)*dz/(d*d));
  seg =  ((int)(tan_theta/m_d_tan_theta+1024.5))-1024;  
  plane = (int)(0.5+z/m_plane_sep);

  axx=ax+NXCRYS*alayer;
  bxx=bx+NXCRYS*blayer;
  if (blayer == 7) 
  {
    sino_addr=m_solution_tx[0][mp][axx][bx].nsino; // b is TX source
    z = m_solution_tx[0][mp][axx][bx].z;
    d = m_solution_tx[0][mp][axx][bx].d;
  }
  else
  {
    sino_addr=m_solution_tx[1][mp][bxx][ax].nsino; // a is TX source
    z = m_solution_tx[1][mp][bxx][ax].z;
    d = m_solution_tx[1][mp][bxx][ax].d;
  }
  if (sino_addr>=0)
  {
    if (seg == 0) 
    {
       addr = plane*m_nprojs*m_nviews + sino_addr;
    }
  }
  return addr;
}

int rebin_event( int mp, int alayer, int ax, int ay, int blayer, int bx, 
                      int by/*, unsigned delayed_flag*/)
{
  float cay, dz2, d, z;
  int plane, segnum, addr, sino_addr, offset;
  int axx, bxx;
  float seg;

  axx=ax+NXCRYS*alayer;
  bxx=bx+NXCRYS*blayer;
  if ((sino_addr=m_solution[mp][axx][bxx].nsino)==-1) return -1;
  cay = m_c_zpos2[ay];
  dz2 = m_c_zpos[by]-m_c_zpos[ay];
  z = m_solution[mp][axx][bxx].z;
  d = m_solution[mp][axx][bxx].d;
  plane = (int)(cay+z*dz2);
  seg = (float)(0.5+dz2*d);
  segnum = (int)seg;
  int rd = abs(by-ay);
  if(seg<0) segnum=1-(segnum<<1);
  else      segnum=segnum<<1;
  if (segnum >= m_nsegs) return -1;
  if(m_segplane[segnum][plane]!=-1)
  {
    offset = m_segzoffset[segnum];
    addr = (plane+offset)*m_nprojs*m_nviews + sino_addr;
  }
  else addr = -1;
  return addr;
}

#endif

/*
	Subroutine to determine the acquisition start time from the supplied
	64-bit listmode data file
*/
int ListModeAcqTo( FILE *Ptr, long *To, long *OffSet, int verbose )
/* 
FILE	*Ptr ;

long	*To ,
	*OffSet ;

int	verbose ;
*/
{
	// char			Info[ MAX_STRING_LENGTH ] ;

	int			StartTAG = 0 ,
				FirstTAG = 0 ;

	unsigned int		*Raw64BitData ;

	long			NumEventsRead ,
				EventNum ,
				TimeTo ,
				ByteOffSet = 0 ;

	struct ListMode_64bit	*Decode64BitLM ;

/* Ensure file pointer at start listmode file */
	if ( fseek( Ptr, 0, SEEK_SET ) )
	{
			/* Return error status to calling routine */
		return( 0 ) ;
	}

/* Initialise variables */
	*OffSet = 0 ;
	*To = -1 ;

        
        
/* Allocate memory for structure */
	Raw64BitData = ( unsigned int * ) malloc( NUMEVENTSTOREAD * sizeof( unsigned int ) ) ;
	Decode64BitLM = ( struct  ListMode_64bit * ) calloc( 1, sizeof( struct ListMode_64bit  ) ) ;

/* Search scan start time */
	while ( ! StartTAG )
	{
printf("\n\n\n What is up?!?"); fflush(stdout);
		NumEventsRead = fread( Raw64BitData, sizeof( unsigned int ), NUMEVENTSTOREAD, Ptr ) ;

		if ( NumEventsRead )
		{

			for ( EventNum = 0 ; EventNum < NumEventsRead ; EventNum += 2, *OffSet += 8 )
			{

				Decode64BitLM->EventInfo.Event1 = *( Raw64BitData + EventNum ) ;
				Decode64BitLM->EventInfo.Event2 = *( Raw64BitData + EventNum + 1 ) ;

				if ( !( ( !( ( Decode64BitLM->EventInfo.Event1 >> 31 ) & 0x1 ) ) && ( ( Decode64BitLM->EventInfo.Event2 >> 31) & 0x1 ) ) )
				{

					errtxt( "\n\tError Syncing 64-bit packets\n" ) ;

				}

					/* Extract Information <---> 64-bit Event Stream */
				if ( UpdateLm( Decode64BitLM, verbose ) )
				{

					errtxt( "\n\tError extracting listmode information from event stream\n" ) ;

				}

				switch( FirstTAG )
				{
					case 0:
					if ( Decode64BitLM->LM_Type == 2 )
					{
							/* Log Initial Time Tag */
						TimeTo = Decode64BitLM->AcqTime.TagTime ;
							/* Reset flag */
						FirstTAG = 1 ;
					}
					break ;
					case 1:
					switch ( Decode64BitLM->LM_Type )
					{
						case 1:		/* Event - prompt or random */
							break ;
						case 2:		/* Time information */
							if ( ( Decode64BitLM->AcqTime.TagTime - TimeTo ) < 0 )
							{
/*
printf( "\t\t\t{ Decode64BitLM->AcqTime.TagTime : %d\tTimeTo : %d\tDecode64BitLM->AcqTime.TagTime - TimeTo : %d }\n",Decode64BitLM->AcqTime.TagTime, TimeTo, Decode64BitLM->AcqTime.TagTime - TimeTo ) ; fflush( stdout ) ;
*/
								printf( "\t\t\t{\n\t\t\t\tLocated scan start . . .\n\t\t\t\t\tCurrent Time Tag :%6d\tLast Time TAG :%10d\tDifference :%10d\n\t\t\t}\n",Decode64BitLM->AcqTime.TagTime, TimeTo, Decode64BitLM->AcqTime.TagTime - TimeTo ) ; fflush( stdout ) ;
									/* Located acquisition start */
								*To = Decode64BitLM->AcqTime.TagTime ;
									/* File pointer offset - Start Time TAG */
								*OffSet -= 8 ; /* Account for increment in for loop */
									/* End loop */
								EventNum = NumEventsRead ;
									/* End While */
								StartTAG = 1 ;
							}
							else
							{
								TimeTo = Decode64BitLM->AcqTime.TagTime ;
							}
							break ;
						case 4:		/* Singles */
							break ;
						case 8:		/* Motion */
							break ;
					}
					break ;
				}
			}
		}
	}

/* Free allocated memory */
	free( Decode64BitLM ) ;
	free( Raw64BitData ) ;

/* Return success status to calling routine */
	return( 1 ) ;

}

/*
	Subroutine to calculate the deadtime from the supplied average singles/block
*/
float GetDTC( int LLD, float SinPerBlock)
{
	double Tau;
	if (LLD == 250)
		Tau = 6.9378e-6;
	if (LLD > 250  && LLD <300)
		Tau = (((double)LLD - 250.0)/50.0) * (7.0677e-6 - 6.9378e-6) + 6.9378e-6;
	if (LLD == 300)
		Tau = 7.0677e-6;
	if (LLD > 300  && LLD <350)
		Tau = (((double)LLD - 300.0)/50.0) * (7.7004e-6 - 7.0677e-6 ) + 7.0677e-6;
	if (LLD == 350)
		Tau = 7.7004e-6;
	if (LLD > 350  && LLD <400)
		Tau = (((double)LLD - 350.0)/50.0) * (8.94e-6 - 7.7004e-6) + 7.7004e-6;
	if (LLD >= 400)
		Tau = 8.94e-6;
	return((float)exp(Tau*(double)SinPerBlock));
}

int ExtractInterFileInfo( struct FrameInfo *FrInfo, struct InterFileInfo *InterFileInfo )
{

	FILE		*InHdr_FilePtr ;

	char		InHdrFileName[ MAX_STRING_LENGTH ] ,
			Info[ MAX_STRING_LENGTH ] ;

//	int		Var ,
	int		Day ,
			Month ,
			Year ;

	/* Build interfile header file name */
		/* 1st Format - Input listmode header filename */
	memset( InHdrFileName, 0, MAX_STRING_LENGTH ) ;
	strncat( InHdrFileName, FrInfo->OriginalFile, strlen( FrInfo->OriginalFile ) - strlen( strstr( FrInfo->OriginalFile, ".l64" ) ) ) ;
	strcat( InHdrFileName, ".l.hdr" ) ;

/* Check if input sinogram header file exists */
	if ( access( InHdrFileName, F_OK ) != 0 )
	{
			/* 2nd Format - Input listmode header filename */
		memset( InHdrFileName, 0, MAX_STRING_LENGTH ) ;
		strcat( InHdrFileName, FrInfo->OriginalFile ) ;
		strcat( InHdrFileName, ".hdr" ) ;
			/* Check if input sinogram header file exists */
		if ( access( InHdrFileName, F_OK ) != 0 )
		{
				/* Error message - return to calling routine */
			printf( "\n\t\tHeader file [%s] for input listmode file does not exist !!!\n\n", InHdrFileName ) ;
		}
	}

	/* Open input listmode header file */
	if ( !( InHdr_FilePtr = fopen( InHdrFileName, "r" ) ) )
	{
			/* Error message - return to calling routine */
		printf( "\n\tError opening header file %s\n", InHdrFileName ) ;
	}

/* Copy input file header to output file header */
	while ( fgets( Info, MAX_STRING_LENGTH, InHdr_FilePtr ) )
	{
		if ( strstr( Info, "study date (dd:mm:yryr)" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %s", InterFileInfo->StudyDate ) ;
				/* Convert date from day:month:year to year:month:day */
			sscanf( InterFileInfo->StudyDate, "%d:%d:%d", &Day, &Month, &Year ) ;
			sprintf( InterFileInfo->StudyDate, "%d:%d:%d", Year, Month, Day ) ;
			/*
				printf( "%s\n", InterFileInfo->StudyDate ) ;
			*/
		}
		//else if ( strstr( Info, "study time (hh:mm:ss GMT)" ) )
		else if ( strstr( Info, "study time (hh:mm:ss)" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %s", InterFileInfo->StudyTime ) ;
			/*
				printf( "%s\n", InterFileInfo->StudyTime ) ;
			*/
		}
		else if ( strstr( Info, "axial compression" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %d", &InterFileInfo->AxialCompression ) ;
			/*
				printf( "%d\n", InterFileInfo->AxialCompression ) ;
			*/
		}
		else if ( strstr( Info, "maximum ring difference" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %d", &InterFileInfo->MaximumRingDifference ) ;
			/*
				printf( "%d\n", InterFileInfo->MaximumRingDifference ) ;
			*/
		}
		else if ( strstr( Info, "energy window lower level" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %d", &InterFileInfo->LLD ) ;
			/*
				printf( "%d\n", InterFileInfo->LLD ) ;
			*/
		}
		else if ( strstr( Info, "energy window upper level" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %d", &InterFileInfo->ULD ) ;
			/*
				printf( "%d\n", InterFileInfo->ULD ) ;
			*/
		}
		else if ( strstr( Info, "image duration" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %d", &InterFileInfo->StudyDuration ) ;
			/*
				printf( "%d\n", InterFileInfo->StudyDuration ) ;
			*/
		}
		else if ( strstr( Info, "Patient name" ) )
		{
			strcpy( InterFileInfo->PatientName, strstr( strstr( Info, ":=" ), " " ) ) ;
			/*
				printf( "%s", InterFileInfo->PatientName ) ;
			*/
		}
		else if ( strstr( Info, "Patient DOB" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %s", InterFileInfo->PatientDOB ) ;
				/* Convert date from day/month/year to year:month:day */
			sscanf( InterFileInfo->PatientDOB, "%d/%d/%d", &Day, &Month, &Year ) ;
			sprintf( InterFileInfo->PatientDOB, "%d:%d:%d", Year, Month, Day ) ;
			/*
				printf( "%s\n", InterFileInfo->PatientDOB ) ;
			*/
		}
		else if ( strstr( Info, "Patient ID" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %s", InterFileInfo->PatientID ) ;
			/*
				printf( "%s\n", InterFileInfo->PatientID ) ;
			*/
		}
		else if ( strstr( Info, "Patient sex" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %s", InterFileInfo->PatientSex ) ;
			/*
				printf( "%s\n", InterFileInfo->PatientSex ) ;
			*/
		}
		else if ( strstr( Info, "Dose type" ) )
		{
			sscanf( strstr( Info, ":=" ), ":= %s", InterFileInfo->Injectate ) ;
			/*
				printf( "%s\n", InterFileInfo->Injectate ) ;
			*/
		}
		else if ( strstr( Info, "Dosage Strength" ) )
		{
			strcpy( InterFileInfo->Activity, strstr( strstr( Info, ":=" ), " " ) ) ;
			/*
				printf( "%s", InterFileInfo->Activity ) ;
			*/
		}
	}
		/* Close open file */
	fclose( InHdr_FilePtr ) ;
		/* Return 'successful' completion to calling routine */
	return( 1 ) ;
}

int WriteDataToDisk( char *FileName, short *Data, char *DataType,
                    struct ListMode_64bit *Event64Bit, struct FrameInfo *FrInfo,
                      struct InterFileInfo *InterFileInfo )
{
	FILE		*FilePtr ;

	char		SinoFileName[ MAX_STRING_LENGTH ] ,
			HdrFileName[ MAX_STRING_LENGTH ] ,
			SifFileName[ MAX_STRING_LENGTH ] ,
			Descriptor[ MAX_STRING_LENGTH ] ,
			SegmentInfo[ MAX_STRING_LENGTH ] ;

	int		i1 ,
			nSegments ,
			nSeg ,
			Segment ,
			Seg0Planes = 207 ,
			minRd ;

	float		*Sinogram_Flt ;

	long int	sum ;

		/* Sinogram & Header Filename */
	memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
	sprintf( Descriptor, "_fr%d", FrInfo->FrameNum ) ;
			/* Sinogram */
	memset( SinoFileName, 0, MAX_STRING_LENGTH ) ;
	strncat( SinoFileName, FileName, strlen( FileName ) - 2 ) ;
	strcat( SinoFileName, Descriptor ) ; strcat( SinoFileName, ".s" ) ;

			/* Header */
	memset( HdrFileName, 0, MAX_STRING_LENGTH ) ;
	strncat( HdrFileName, FileName, strlen( FileName ) - 2 ) ;
	strcat( HdrFileName, Descriptor ) ; strcat( HdrFileName, ".s.hdr" ) ;

			/* Scan information file */
	memset( SifFileName, 0, MAX_STRING_LENGTH ) ;
	strncat( SifFileName, FileName, strlen( FileName ) - 2 ) ;
	strcat( SifFileName, ".sif" ) ;

		/* Information --> Screen */
			/* Report frame counts ---> screen */
	printf( "\t\t\t\tPrompt+Delayed :%12d\tPrompt :%12d\tDelayed :%12d\t\tTrues :%12d\n", FrInfo->Prompt+FrInfo->Delayed, FrInfo->Prompt, FrInfo->Delayed, FrInfo->Prompt-FrInfo->Delayed ) ;
	for ( sum = 0 , i1 = 0 ; i1 < FrInfo->nElements ; i1++ ) sum = sum + *(Data + i1) ;
	printf( "\t\t\t\t\t\t\t\t\t\t\t\t\t      {Sinogram Total :%12d}", sum ) ;
	if ( sum != FrInfo->Prompt-FrInfo->Delayed ) printf( "***" ) ;
	printf( "\n" ) ;

/* Write Sif Information --> */
		/* If creating file from scratch */
        if ( FrInfo->FrameNum == 1 )
        {
			/* Open Sif File - (WRITE Only) */
		if ( !( FilePtr = fopen( SifFileName, "w" ) ) )
		{
			printf( "\n\tError Opening Supplied File %s\n", SifFileName ) ;
			return( 0 ) ;
		}
			/* 	Write scan information	*/
		fprintf( FilePtr, "%s %s %d 4 1 %s %s\n", InterFileInfo->StudyDate, InterFileInfo->StudyTime,
						FrInfo->TotalNumFrame, FileName, InterFileInfo->Injectate ) ;
			/* Close open file */
		fclose( FilePtr ) ;
        }
		/* Open Sif File - (APPEND Only) */
	if ( !( FilePtr = fopen( SifFileName, "a" ) ) )
	{
		printf( "\n\tError Opening Supplied File %s\n", SifFileName ) ;
		return( 0 ) ;
	}
		/* 	Write scan information	*/
	fprintf( FilePtr, "%9.3f %9.3f %12d %12d\n", FrInfo->FrameStartTime , FrInfo->FrameEndTime, FrInfo->Prompt, FrInfo->Delayed ) ;
		/* Close open file */
	fclose( FilePtr ) ;

/* Write sinogram data ---> disk */
		/* Open Sinogram File - (WRITE Only) */
	if ( !( FilePtr = fopen( SinoFileName, "w" ) ) )
	{
		printf( "\n\tError Opening Supplied File %s\n", SinoFileName ) ;
		return( 0 ) ;
	}
		/* 	Write data	*/
	if ( ! strcasecmp( DataType, "float" )  )
	{
			/* Allocate memory */
		Sinogram_Flt = ( float * )malloc( FrInfo->nElements * sizeof( float ) ) ;
			/* Convert integer sinogram ---> float */
		for ( i1 = 0 ; i1 < FrInfo->nElements ; i1++ ) *( Sinogram_Flt + i1 ) = ( float )*(Data + i1) ;
		if ( fwrite( Sinogram_Flt, sizeof( float ), FrInfo->nElements, FilePtr ) == 0 )
		{
			printf( "\n\tError Writing Sinogram Data ---> Supplied File %s\n", SinoFileName ) ;
			return( 0 ) ;
		}
			/* Free allocated memory */
		free( Sinogram_Flt ) ;
	}
	else
	{
		if ( fwrite( Data, sizeof( short int ), FrInfo->nElements, FilePtr ) == 0 )
		{
			printf( "\n\tError Writing Sinogram Data ---> Supplied File %s\n", SinoFileName ) ;
			return( 0 ) ;
		}
	}
		/* Close open file */
	fclose( FilePtr ) ;

/* Write header information ---> disk */
		/* Open header File - (WRITE Only) */
	if ( !( FilePtr = fopen( HdrFileName, "w" ) ) )
	{
		printf( "\n\tError Opening Supplied File %s\n", HdrFileName ) ;
		return( 0 ) ;
	}
		/* Write information */
	fprintf( FilePtr, "!INTERFILE :=\n" ) ;
	fprintf( FilePtr, "!originating system := HRRT\n" ) ;
	fprintf( FilePtr, "!GENERAL DATA :=\n" ) ;
	fprintf( FilePtr, "original institution := CAMH\n" ) ;
	fprintf( FilePtr, "!name of data file := %s\n", SinoFileName ) ;
	fprintf( FilePtr, "patient name := %s", InterFileInfo->PatientName ) ;
	fprintf( FilePtr, "!patient ID := %s\n", InterFileInfo->PatientID ) ;
	fprintf( FilePtr, "patient dob := %s\n", InterFileInfo->PatientDOB ) ;
	fprintf( FilePtr, "patient sex := %s\n", InterFileInfo->PatientSex ) ;
	fprintf( FilePtr, "patient dexterity := %s\n", InterFileInfo->PatientDexterity ) ;
	fprintf( FilePtr, "patient height (cm) := %s\n", InterFileInfo->PatientHeight ) ;
	fprintf( FilePtr, "patient weight (kg) := %s\n", InterFileInfo->PatientWeight ) ;
	fprintf( FilePtr, "!GENERAL IMAGE DATA := \n" ) ;
	fprintf( FilePtr, "!type of data := PET\n" ) ;
	fprintf( FilePtr, "study date := %s\n", InterFileInfo->StudyDate ) ;
	fprintf( FilePtr, "study time := %s\n", InterFileInfo->StudyTime ) ;
	fprintf( FilePtr, "radiopharmaceutical := %s\n", InterFileInfo->Injectate ) ;
	fprintf( FilePtr, "tracer activity at time of injection (MBq) := %s", InterFileInfo->Activity ) ;
	fprintf( FilePtr, "imagedata byte order := LITTLEENDIAN\n" ) ;
	fprintf( FilePtr, "!PET STUDY (General) :=\n" ) ;
	fprintf( FilePtr, "quantification units := None\n" ) ;
	fprintf( FilePtr, "decay corrected := n\n" ) ;
	fprintf( FilePtr, "process status := Acquired\n" ) ;
	fprintf( FilePtr, "!PET data type := emission\n" ) ;
	fprintf( FilePtr, "data format := sinogram\n" ) ;
	if ( ! strcasecmp( DataType, "float" )  )
	{
		fprintf( FilePtr, "!number format := float\n" ) ;
		fprintf( FilePtr, "!number of bytes per pixel := 4\n" ) ;
	}
	else
	{
		fprintf( FilePtr, "!number format := signed integer\n" ) ;
		fprintf( FilePtr, "!number of bytes per pixel := 2\n" ) ;
	}
	fprintf( FilePtr, "!number of dimensions := %d\n", 4 ) ;
	fprintf( FilePtr, "matrix axis label [1] := %s\n", "tangential coordinate" ) ;
	fprintf( FilePtr, "matrix size [1] := %d\n", FrInfo->NumBin ) ;
	fprintf( FilePtr, "matrix axis label [2] := %s\n", "view" ) ;
	fprintf( FilePtr, "matrix size [2] := %d\n", FrInfo->NumView ) ;
	fprintf( FilePtr, "matrix axis label [3] := axial coordinate\n" ) ;

		/* Segment 0 --> 207 Planes */
			/* Clear memory */
	memset( SegmentInfo, 0, MAX_STRING_LENGTH ) ;
	memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
		/* Fill segment 0 value */
	sprintf( Descriptor, "{ %d", Seg0Planes ) ;
	strcat( SegmentInfo, Descriptor ) ;
		/* Fill other segments */
	nSegments = ( 2 * FrInfo->RingDiff + 1 ) / FrInfo->Span ;
	for ( nSeg = 2 ; nSeg <= nSegments ; nSeg++ )
	{
			/* Add string to list */
		memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
		sprintf( Descriptor, ", %d", ( Seg0Planes - ( FrInfo->Span * ( 2 * abs( ( int )( nSeg / 2 * pow( -1., ( double )( nSeg % 2 ) ) ) ) - 1 ) + 1 ) ) ) ;
		strcat( SegmentInfo, Descriptor ) ;
	}
	strcat( SegmentInfo, " }" ) ;
	fprintf( FilePtr, "matrix size [3] := %s\n", SegmentInfo ) ;
	fprintf( FilePtr, "matrix axis label [4] := %s\n", "segment" ) ;
	fprintf( FilePtr, "matrix size [4] := %d\n", nSegments ) ;
		/* Minimum ring difference per segment */
	minRd = -( FrInfo->Span - 1 ) / 2 ;
	memset( SegmentInfo, 0, MAX_STRING_LENGTH ) ;
	memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
	sprintf( Descriptor, "{ %d", minRd ) ;
	strcat( SegmentInfo, Descriptor ) ;
	for ( nSeg = 0 ; nSeg < ( nSegments - 1 ) / 2 ; nSeg++ )
	{
			/* Increment ring difference by span */
		minRd += FrInfo->Span ;
			/* Add to string list */
		memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
		sprintf( Descriptor, ", -%d, %d", minRd, minRd ) ;
		strcat( SegmentInfo, Descriptor ) ;
	}
	strcat( SegmentInfo, " }" ) ;
	fprintf( FilePtr, "minimum ring difference per segment := %s\n" , SegmentInfo ) ;
		/* Maximum ring difference per segment */
	minRd = ( FrInfo->Span - 1 ) / 2 ;
	memset( SegmentInfo, 0, MAX_STRING_LENGTH ) ;
	memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
	sprintf( Descriptor, "{ %d", minRd ) ;
	strcat( SegmentInfo, Descriptor ) ;
	for ( nSeg = 0 ; nSeg < ( nSegments - 1 ) / 2 ; nSeg++ )
	{
			/* Increment ring difference by span */
		minRd += FrInfo->Span ;
			/* Add to string list */
		memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
		sprintf( Descriptor, ", -%d, %d", minRd, minRd ) ;
		strcat( SegmentInfo, Descriptor ) ;
	}
	strcat( SegmentInfo, " }" ) ;
	fprintf( FilePtr, "maximum ring difference per segment := %s\n" , SegmentInfo ) ;
	fprintf( FilePtr, "number of time frames := 1\n", FrInfo->FrameNum ) ;
	fprintf( FilePtr, "number of energy windows := 1\n" ) ;
	fprintf( FilePtr, "energy window lower level [1] := %d\n", InterFileInfo->LLD ) ;
	fprintf( FilePtr, "energy window upper level [1] := %d\n", InterFileInfo->ULD ) ;
	fprintf( FilePtr, "transaxial FOV diameter (cm) := 46.9\n" ) ;
	fprintf( FilePtr, "number of rings := 104\n" ) ;
	fprintf( FilePtr, "number of detectors per ring := 576\n" ) ;
	fprintf( FilePtr, "distance between rings (cm) := 0.24375\n" ) ;
	fprintf( FilePtr, "gantry tilt angle (degrees) := 0\n" ) ;
	fprintf( FilePtr, "gantry rotation angle (degrees) := 0\n" ) ;
	fprintf( FilePtr, "bin size (cm) := 0.121875\n" ) ;
	fprintf( FilePtr, "septa state := none\n" ) ;
	fprintf( FilePtr, "applied correction := {none}\n" ) ;
	fprintf( FilePtr, "!IMAGE DATA DESCRIPTION:=\n" ) ;
	fprintf( FilePtr, "!image duration (sec)[%d] := %.3f\n", FrInfo->FrameNum, FrInfo->FrameEndTime - FrInfo->FrameStartTime ) ;
	fprintf( FilePtr, "!image relative start time (sec)[%d] := %.3f\n", FrInfo->FrameNum, FrInfo->FrameStartTime ) ;
	fprintf( FilePtr, "total prompts[%d][1][1] := %d\n", FrInfo->FrameNum, FrInfo->Prompt ) ;
	fprintf( FilePtr, "total delayed[%d][1][1] := %d\n", FrInfo->FrameNum, FrInfo->Delayed ) ;
	fprintf( FilePtr, "total trues[%d][1][1] := %d\n", FrInfo->FrameNum, FrInfo->Prompt - FrInfo->Delayed ) ;
	fprintf( FilePtr, "total singles[%d][1][1] := %d\n", FrInfo->FrameNum, ( int )rintf( Event64Bit->SglInfo.TotSgls[ 2 ] * RAD_DETECTORS * AX_DETECTORS * NHEADS ) ) ;
		/* CPS additional */
	fprintf( FilePtr, "frame := %d\n", FrInfo->FrameNum ) ;
	fprintf( FilePtr, "axial compression := %d\n", InterFileInfo->AxialCompression ) ;
	fprintf( FilePtr, "maximum ring difference := %d\n", InterFileInfo->MaximumRingDifference ) ;
	fprintf( FilePtr, "image duration := %.3f\n", FrInfo->FrameEndTime - FrInfo->FrameStartTime ) ;
	fprintf( FilePtr, "Normalization file name and path :=\n" ) ;
	fprintf( FilePtr, "Blank file name and path :=\n" ) ;
	fprintf( FilePtr, "image relative start time := %.3f\n", FrInfo->FrameStartTime ) ;
	fprintf( FilePtr, "frame := %d\n", FrInfo->FrameNum ) ;
	fprintf( FilePtr, "Total Prompts := %d\n", FrInfo->Prompt ) ;
	fprintf( FilePtr, "Total Randoms := %d\n", FrInfo->Delayed ) ;
	fprintf( FilePtr, "Total Net Trues := %d\n", FrInfo->Prompt - FrInfo->Delayed ) ;
		/* 	Write block singles	*/
	for ( i1 = 0 ; i1 < RAD_DETECTORS * AX_DETECTORS * NHEADS ; i1++ )
	{
		fprintf( FilePtr, "block singles%4d := %d\n", i1 , ( int )rintf( Event64Bit->SglInfo.Sgls[ i1 ][ 2 ] ) ) ;
	}
	fprintf( FilePtr, "average singles per block := %d\n", ( int )( Event64Bit->SglInfo.TotSgls[ 2 ] ) ) ;
	fprintf( FilePtr, "Dead time correction factor := %f\n", GetDTC( 350, ( float )( ( int )Event64Bit->SglInfo.TotSgls[ 2 ] ) ) ) ;
	fprintf( FilePtr, "!END OF INTERFILE :=\n" ) ;
		/* Close open file */
	fclose( FilePtr ) ;
		/* Return - success */
	return( 1 ) ;
}

int WriteCoinMapToDisk( char *FileName, int *C_Map_Pr, int *C_Map_Del, 
                       struct ListMode_64bit *Event64Bit, struct FrameInfo *FrInfo,
                       struct InterFileInfo *InterFileInfo )

{
	FILE		*FilePtr ;

	char		CoinMapFileName[ MAX_STRING_LENGTH ] ,
			CoinMapHdrFileName[ MAX_STRING_LENGTH ] ,
			Descriptor[ MAX_STRING_LENGTH ] ;

	int		i1 ,
			nSegments ,
			nSeg ,
			Segment ,
			Seg0Planes = 207 ,
			minRd ;

	long int	sum ;

		/* Sinogram & Header Filename */
	memset( Descriptor, 0, MAX_STRING_LENGTH ) ;
	sprintf( Descriptor, "_fr%d", FrInfo->FrameNum ) ;
			/* Coincidence Map */
	memset( CoinMapFileName, 0, MAX_STRING_LENGTH ) ;
	strncat( CoinMapFileName, FileName, strlen( FileName ) - 2 ) ;
	strcat( CoinMapFileName, Descriptor ) ; strcat( CoinMapFileName, ".ch" ) ;
			/* Coincidence Map Header */
	memset( CoinMapHdrFileName, 0, MAX_STRING_LENGTH ) ;
	strncat( CoinMapHdrFileName, FileName, strlen( FileName ) - 2 ) ;
	strcat( CoinMapHdrFileName, Descriptor ) ; strcat( CoinMapHdrFileName, ".ch.hdr" ) ;

/* Write coincidence map ---> disk */
		/* Open coincidence map - (WRITE Only) */
	if ( !( FilePtr = fopen( CoinMapFileName, "w" ) ) )
	{
		printf( "\n\tError Opening Supplied Coincidence Map Filename %s\n", CoinMapFileName ) ;
		return( 0 ) ;
	}
		/* 	Write data	*/
			/* Prompt */
	if ( fwrite( C_Map_Pr, sizeof( int ), NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ), FilePtr ) == 0 )
	{
		printf( "\n\tError Writing coincidence map prompt ---> Supplied File %s\n", CoinMapFileName ) ;
		return( 0 ) ;
	}
			/* Random */
	if ( fwrite( C_Map_Del, sizeof( int ), NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ), FilePtr ) == 0 )
	{
		printf( "\n\tError Writing coincidence map delayed ---> Supplied File %s\n", CoinMapFileName ) ;
		return( 0 ) ;
	}
		/* Close open file */
	fclose( FilePtr ) ;

		/* Information --> Screen */
	printf( "\t\t\t& Coincidence Map\n" ) ;
	printf( "\n\n" ) ;

/* Write header information ---> disk */
		/* Open header File - (WRITE Only) */
	if ( !( FilePtr = fopen( CoinMapHdrFileName, "w" ) ) )
	{
		printf( "\n\tError Opening Supplied Coincidence Map Headrr Filename %s\n", CoinMapHdrFileName ) ;
		return( 0 ) ;
	}
		/* Write information */
	fprintf( FilePtr, "!INTERFILE :=\n" ) ;
	fprintf( FilePtr, "!originating system := HRRT\n" ) ;
	fprintf( FilePtr, "!GENERAL DATA :=\n" ) ;
	fprintf( FilePtr, "original institution := CAMH\n" ) ;
	fprintf( FilePtr, "!name of data file := %s\n", CoinMapFileName ) ;
	fprintf( FilePtr, "patient name := %s", InterFileInfo->PatientName ) ;
	fprintf( FilePtr, "!patient ID := %s\n", InterFileInfo->PatientID ) ;
	fprintf( FilePtr, "!GENERAL IMAGE DATA := \n" ) ;
	fprintf( FilePtr, "!type of data := PET\n" ) ;
	fprintf( FilePtr, "study date := %s\n", InterFileInfo->StudyDate ) ;
	fprintf( FilePtr, "study time := %s\n", InterFileInfo->StudyTime ) ;
	fprintf( FilePtr, "imagedata byte order := LITTLEENDIAN\n" ) ;
	fprintf( FilePtr, "!PET STUDY (General) :=\n" ) ;
	fprintf( FilePtr, "quantification units := None\n" ) ;
	fprintf( FilePtr, "decay corrected := n\n" ) ;
	fprintf( FilePtr, "!number format := long\n" ) ;
	fprintf( FilePtr, "!number of bytes per pixel := 4\n" ) ;
	fprintf( FilePtr, "frame := %d\n", FrInfo->FrameNum ) ;
	fprintf( FilePtr, "!image duration (sec)[%d] := %.3f\n", FrInfo->FrameNum, FrInfo->FrameEndTime - FrInfo->FrameStartTime ) ;
	fprintf( FilePtr, "!END OF INTERFILE :=\n" ) ;
		/* Close open file */
	fclose( FilePtr ) ;

		/* Return - success */
	return( 1 ) ;
}

int main( int argc, char **argv )
{		/* Start of MAIN routine */
        extern char		*optarg ;

        extern int		optind,
				opterr ;

	FILE			*LM_FilePtr ,
				*FilePtr ;

	char			*ListModeFileName ,
				*SinoGramFileName ,
				*FrameDefFileName ,
				Info[ MAX_STRING_LENGTH ] ;

	int			which_option ,
				Verbose = 0 ,
				Write_Coin_Map = 0 ,
				i1 ,
				Span ,
				RingDiff ,
				ScanMode = 1 ,
				NumSegments ,
				TotalNumFrame = 0 ,
				NumFrame ,
				NumErrSync = 0 ;

	short int		*SinogramsPerSegment ,
				*SinogramPr ,
				*SinogramDel ;

	unsigned int		*LmData_64Bit ;

	int			*CoinMapPr ,
				*CoinMapDel ;

	long			*FrameTimes ,
				*FrameTimesPtr ,
				AcqTo ,
				AcqTime ,
				ByteOffSet ,
				NumEventsRead ,
				TotEventNum ,
				RandomCount ,
				PromptCount ,
				EventNum ,
				CurrPtrPos ;

	float			FrameDuration ,
				AcqDuration = 0. ;

	struct ListMode_64bit	*LM_64Bit ;

	struct FrameInfo	*FrameInfo ;

	struct InterFileInfo	*InterFileInfo ;

/* Check number of arguments. */
	if(argc == 1)
	{
		ShowHrrtLM_Help() ;
	}
	else
	{
		while ( ( which_option = getopt(argc, argv, "l:s:f:t:d:m:c:v:h") ) != -1)
		{
			switch ( which_option )
			{
				case 'h':
					ShowHrrtLM_Help() ;
					break ;
				case 'l':
					ListModeFileName = ( optarg ) ;
					break ;
				case 's':
					SinoGramFileName = ( optarg ) ;
					break ;
				case 'f':
					FrameDefFileName = ( optarg ) ;
					break ;
				case 't':	/* Required Span for Output Sinogram */
					Span = atoi( optarg ) ;
					break ;
				case 'd':	/* Maximum Ring Difference - Acceptance 'Angle' */
					RingDiff = atoi( optarg ) ;
					break ;
				case 'm':	/* Scan Mode { 0 ==> Trues; 3 ==> Prompts & Trues; 7 ==> Prompts, Delayed & Trues } */
					ScanMode = atoi( optarg ) ;
					break ;
				case 'c':	/* Write coincidence map to disk */
					Write_Coin_Map = atoi( optarg ) ;
					break ;
				case 'v':
					Verbose = atoi( optarg ) ;
					break ;
			}
		}
	}

/* Display run-time variables --> screen */
	printf( "\n\tSorting 64-bit list mode ---> sinogram :\n" ) ;
	printf( "\t\tListmode Filename : %s\n", ListModeFileName ) ;
	printf( "\t\tGeneric Sinogram Filename : %s\n", SinoGramFileName ) ;
	printf( "\t\tFrame Definition Filename : %s\n", FrameDefFileName ) ;
	printf( "\n\tTomograph Description :\n\t\tHRRT " ) ;
	if ( FXD == 1.0 )
	{
		printf( "[New]\n" ) ;
	} else {
		printf( "[Prototype]\n" ) ;
	}
	printf( "\t\tCrystal Depth LSO:LYSO - %.2f:%.2f\n", FXD, SXD ) ;

/* Allocate memory for structure - dynamic */
	LmData_64Bit = ( unsigned int * ) malloc( NUMEVENTSTOREAD * sizeof( unsigned int ) ) ;
	LM_64Bit = ( struct  ListMode_64bit * ) calloc( 1, sizeof( struct ListMode_64bit  ) ) ;
	FrameInfo = ( struct  FrameInfo * ) calloc( 1, sizeof( struct FrameInfo  ) ) ;
	SinogramsPerSegment = ( short int * ) malloc( 64 * sizeof( short int ) ) ;
	InterFileInfo = ( struct InterFileInfo * ) calloc( 1, sizeof( struct InterFileInfo ) ) ;

	/* Generate Lookup Table for PLane Definitions in Michelogram from supplied Span, Ring Difference etc. */

#ifdef _LUT_
	int span_bak=Span, rd_bak=RingDiff;
	printf( "Before init_rebinner - Span : %d\tRing Diff : %d\n", Span, RingDiff ) ;
	Init_Rebinner( Span, RingDiff);
	printf( "After init_rebinner - Span : %d\tRing Diff : %d\n", Span, RingDiff ) ;
	//errtxt( "Bye\n" ) ;
#else
	EventNum = init_sort3d_hrrt( Span, RingDiff, SINOH, SINOW, 46.9, FXD ) ;
#endif

/* Determine # sinograms and segments */
	printf("\n\tPlane Definition : \n") ;
	memset( SinogramsPerSegment, 0, 64 * sizeof( short int ) ) ;
	if ( SinoInfo( HEADY, Span, RingDiff, &FrameInfo->NumSino, &NumSegments, SinogramsPerSegment ) )
	{
		errtxt("\n\tInconsistency : Check the Number of Detector Rings, the Span or the Ring Difference\n" ) ;
	}
		/* Set structure */
	FrameInfo->NumBin = SINOW ;
	FrameInfo->NumView = SINOH ;
	FrameInfo->nElements = FrameInfo->NumSino * FrameInfo->NumBin * FrameInfo->NumView ;
	FrameInfo->Span = Span ;
	FrameInfo->RingDiff = RingDiff ;
	memset( FrameInfo->OriginalFile, 0, MAX_STRING_LENGTH ) ; strcpy( FrameInfo->OriginalFile, ListModeFileName ) ;

/*
	Open Supplied Frame Definition File - ( READ Only )
		Read Frame Definitions ---> Memory
*/
	if ( !( FilePtr = fopen( FrameDefFileName, "r" ) ) )
	{
		sprintf( Info, "\n\tError Opening Supplied File %s - Does it Exist?\n", FrameDefFileName ) ;
		errtxt( Info ) ;
	}
			/* Determine Number of Frame Defined */
	if ( fseek( FilePtr, 0, 0 ) == -1 )
	{
		sprintf( Info, "\n\tAccessing Frame Definition File %s\n", FrameDefFileName ) ;
		errtxt( Info ) ;
	}
	while ( fscanf( FilePtr, "%d %*f", &NumFrame ) != EOF )
	{
		TotalNumFrame += NumFrame ;
	}
	FrameInfo->TotalNumFrame = TotalNumFrame ;
		/* Allocate Memory */
	FrameTimes = ( long int * ) malloc( TotalNumFrame * sizeof( long int ) ) ;
		/* Reset FilePointer */
	if ( fseek( FilePtr, 0, 0 ) == -1 )
	{
		sprintf( Info, "\n\tAccessing Frame Definition File %s\n", FrameDefFileName ) ;
		errtxt( Info ) ;
	}
	printf( "\n\tFrame Definition : \n" ) ;
	printf( "\t\t\t# Frames\tFrame Duration (s)\n" ) ;
	printf( "\t\t\t========\t==================\n" ) ;
	FrameTimesPtr = FrameTimes ;
	while ( fscanf( FilePtr, "%d %f", &NumFrame, &FrameDuration) != EOF )
	{
		printf( "\t\t\t%5d\t\t%12.3f\n", NumFrame, FrameDuration ) ;
		for ( i1 = 0 ; i1 < NumFrame ; i1++ )
		{
			*FrameTimesPtr++ = ( int )( FrameDuration * 1000. ) ;
			AcqDuration += FrameDuration ;
		}
	}
		/* Close File */
	fclose( FilePtr ) ;
	if ( FrameInfo->TotalNumFrame > 1 )
	{
		printf("\t\tTotal : %d frames defined\n", TotalNumFrame ) ;
	}
	else
	{
		printf("\t\tTotal : %d frame defined\n", TotalNumFrame ) ;
	}
	printf( "\t\t\tAcquisition duration : %.3f second ( %.2f minute(s) )\n", AcqDuration, AcqDuration / 60. ) ;

/* Open List Mode File - ( READ Only ) */
	if ( !( LM_FilePtr = fopen( ListModeFileName, "r" ) ) )
	{
		sprintf( Info, "\n\tError Opening Supplied File %s - Does it Exist?\n", ListModeFileName ) ;
		errtxt( Info ) ;
	}

/* Determine acquistion start time */
	printf( "\n\tAcquisition start information . . .\n" ) ;
	if ( ! ListModeAcqTo( LM_FilePtr, &AcqTo, &ByteOffSet, Verbose ) )
	{
			/* Close open file(s) */
		fclose( LM_FilePtr ) ;
			/* Report error - exit to system */
		sprintf( Info, "\n\tError determining scan start time in 32-bit listmode file %s\n", ListModeFileName ) ;
		errtxt( Info ) ;
	}
	printf( "\t\tAcquisition Start Time : %.3f s\n", ( float )AcqTo / 1000.0 ) ;
	printf( "\t\tFrame 1 duration : %.3f s\n", ( float )*FrameTimes / 1000.0 ) ;
	printf( "\t\tByte Offset : %d\n", ByteOffSet ) ;
	fflush( stdout ) ;

/* Point listmode file pointer to acquisition start */
	if ( fseek( LM_FilePtr, ByteOffSet, SEEK_SET ) )
	{
			/* Close open file(s) */
		fclose( LM_FilePtr ) ;
			/* Report error - exit to system */
		sprintf( Info, "\n\tError setting file pointer to byte offset %d file %s\n", ByteOffSet, ListModeFileName ) ;
		errtxt( Info ) ;
	}

/* Extract scan information from CPS interfile header */
	ExtractInterFileInfo( FrameInfo, InterFileInfo ) ;
		/* Correct settings */
	InterFileInfo->AxialCompression = Span ;
	InterFileInfo->MaximumRingDifference = RingDiff ;

/* Sort LM Data */
		/* Initialise flags for sinogram type */
	printf( "\n\tSinogram mode . . .\n" ) ;
	switch ( ScanMode )
	{
		case 1:	/* True Sinogram */
			printf( "\t\tTrue Sinogram\n" ) ;
		break ;
		case 2:	/* Prompt Sinogram */
			printf( "\t\tPrompt Sinogram\n" ) ;
		break ;
		case 3:	/* Prompt & True Sinogram */
			printf( "\t\tPrompt & True Sinograms\n" ) ;
		break ;
		case 4:	/* Delayed Sinogram */
			printf( "\t\tDelayed Sinogram\n" ) ;
		break ;
		case '5':
			printf( "\t\tDelayed & True Sinograms\n" ) ;
			break ;
		case 6:	/* Prompt & Delayed Sinogram */
			printf( "\t\tPrompt & Delayed Sinograms\n" ) ;
			break ;
		case 7:	/* Prompt, Delayed & True Sinograms */
			printf( "\t\tPrompt, Delayed & True Sinograms\n" ) ;
			break ;
		default:
				/* Report error - exit to system */
			sprintf( Info, "\n\tUnknown Scan Mode %d\n", ScanMode ) ;
			errtxt( Info ) ;
			break ;
	}
	printf( "\n\tSorting 64-bit listmode data . . .\n" ) ; fflush( stdout ) ;
		/* Allocate memory ---> sinogram */
	SinogramPr = ( short int * )malloc( FrameInfo->nElements * sizeof( short int ) ) ;
	SinogramDel = ( short int * )malloc( FrameInfo->nElements * sizeof( short int ) ) ;
	CoinMapPr = ( int * )malloc( NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) * sizeof( int ) ) ;
	CoinMapDel = ( int * )malloc( NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) * sizeof( int ) ) ;
		/* Initialise frame times */
	FrameTimesPtr = FrameTimes ;
	AcqTime = *FrameTimesPtr++ ;
		/* Initialise frame 1 information */
	FrameInfo->FrameNum = 1 ;
	FrameInfo->FrameStartTime = ( float )AcqTo / 1000.0 ;
	FrameInfo->NumTimeTags = 0 ;
	FrameInfo->Prompt = 0 ;
	FrameInfo->Delayed = 0 ;
		/* Clear Coin. Map memory */
        memset( CoinMapPr, 0, NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * (NUM_CRYSTALS * AX_DETECTORS ) * sizeof( int ) ) ;
        memset( CoinMapDel, 0, NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) * sizeof( int ) ) ;
		/* Clear singles memory */
	for ( EventNum = 0 ; EventNum < RAD_DETECTORS * AX_DETECTORS * NHEADS ; EventNum += 1 )
	{
		LM_64Bit->SglInfo.Sgls[ EventNum ][ 0 ] = 0. ;
		LM_64Bit->SglInfo.Sgls[ EventNum ][ 1 ] = 0. ;
		LM_64Bit->SglInfo.Sgls[ EventNum ][ 2 ] = 0. ;
	}
	//memset( LM_64Bit->SglInfo.Sgls, 0, RAD_DETECTORS * AX_DETECTORS * NHEADS * 3 * sizeof( float ) ) ;
	LM_64Bit->SglInfo.TotSgls[ 0 ] = LM_64Bit->SglInfo.TotSgls[ 1 ] = LM_64Bit->SglInfo.TotSgls[ 2 ] = 0 ;
		/* Go sort */
	TotEventNum = 0 ;
	while ( FrameInfo->FrameNum <= TotalNumFrame )
	{
			/* Read data */
		NumEventsRead = fread( LmData_64Bit, sizeof( unsigned int ), NUMEVENTSTOREAD, LM_FilePtr ) ;
			/* Current file pointer position - byte offset from start of file */
		CurrPtrPos = 0 ;
			/* If data read - process events */
		if ( NumEventsRead )
		{
				/* Process list mode data */
			for ( EventNum = 0 ; EventNum < NumEventsRead ; EventNum += 2 )
			{
				TotEventNum += 1 ;

				LM_64Bit->EventInfo.Event1 = *( LmData_64Bit + EventNum ) ;
				LM_64Bit->EventInfo.Event2 = *( LmData_64Bit + EventNum + 1 ) ;

				if ( !( ( !( ( LM_64Bit->EventInfo.Event1 >> 31 ) & 0x1 ) ) && ( ( LM_64Bit->EventInfo.Event2 >> 31) & 0x1 ) ) )
				{
					NumErrSync += 1 ;
					printf( "\n\t\tError Syncing 32-bit packets [ # %d ] Event # %d\t{ PS0 - %d\tPS1 - %d }\n", NumErrSync, TotEventNum, ( LM_64Bit->EventInfo.Event1 >> 31 ) & 0x1, ( LM_64Bit->EventInfo.Event2 >> 31) & 0x1 ) ;
					fflush( stdout ) ;
						/* Punch out event - attempt to re-sync */
					EventNum += 1 ;
						/*
							Reduce # Events to Process
								Punching out a event (above) means odd number of 32-bit events
								to process which is problematic for 64-bit list mode
								To make even, reduce number of events to process by 1
						*/
					NumEventsRead -= 1 ;
						/*
							Reduce CurrPtrPos by 4 bytes
								Reducing number of events read means need to re-read last 32-bit component
								Reset file pointer by 4-bytes, which enables the re-reading of omitted 32-bits
									at start of 64-bit list
						*/
					CurrPtrPos -= 4 ;
				} else {
						/* Extract Information <---> 64-bit Event Stream */
					if ( UpdateLm( LM_64Bit, Verbose ) )
					{
						errtxt( "\n\tError extracting listmode information from event stream\n" ) ;
					}

					switch ( LM_64Bit->LM_Type )
					{
						case 1:	/* Event - prompt or random */
								/*
									Build coincidence map; prompt & delayed both layers
									Process all events defined within time frame of the current acquisition frame
								*/
							if ( LM_64Bit->EventInfo.Event )
							{		// Prompt
								*( CoinMapPr + ( LM_64Bit->EventInfo.AI * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) ) + ( LM_64Bit->EventInfo.HeadA * ( NUM_CRYSTALS * RAD_DETECTORS ) ) + LM_64Bit->EventInfo.AX + ( LM_64Bit->EventInfo.AY * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) ) ) += 1 ;
								*( CoinMapPr + ( LM_64Bit->EventInfo.BI * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) ) + ( LM_64Bit->EventInfo.HeadB * ( NUM_CRYSTALS * RAD_DETECTORS ) ) + LM_64Bit->EventInfo.BX + ( LM_64Bit->EventInfo.BY * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) ) ) += 1 ;
							}
							else
							{		// Delayed
								*( CoinMapDel + ( LM_64Bit->EventInfo.AI * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) ) + ( LM_64Bit->EventInfo.HeadA * ( NUM_CRYSTALS * RAD_DETECTORS ) ) + LM_64Bit->EventInfo.AX + ( LM_64Bit->EventInfo.AY * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) ) ) += 1 ;
								*( CoinMapDel + ( LM_64Bit->EventInfo.BI * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) ) + ( LM_64Bit->EventInfo.HeadB * ( NUM_CRYSTALS * RAD_DETECTORS ) ) + LM_64Bit->EventInfo.BX + ( LM_64Bit->EventInfo.BY * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) ) ) += 1 ;
							}
								/*
									Process valid event
										Valid event - within singorams defined by span & ring difference 
								*/
							if ( LM_64Bit->EventInfo.Status )
							{
								if ( LM_64Bit->EventInfo.Event )
								{		// Prompt
									*( SinogramPr + LM_64Bit->EventInfo.SinogramAddress ) += 1 ;
									FrameInfo->Prompt += 1 ;
								}
								else
								{		// Delayed
									*( SinogramDel + LM_64Bit->EventInfo.SinogramAddress ) += 1 ;
									FrameInfo->Delayed += 1 ;
								}
							}
							break ;
						case 2:	/* Time information */
							FrameInfo->NumTimeTags += LM_64Bit->AcqTime.ms ;
								/* If frame ended - write data --> disk */
							if ( LM_64Bit->AcqTime.TagTime >= AcqTime )
							{
										/* Information --> Screen */
								printf( "\t\tFrame %d - Complete\t ---> Disk\n", FrameInfo->FrameNum ) ;
								/*
									printf("Event Time [Event Stream] : %d Accumulated Time [# Time Tags] : %d\n", LM_64Bit->AcqTime.TagTime, FrameInfo->NumTimeTags );
								*/
									/* Frame end time equals start time plus frame duration */
								FrameInfo->FrameEndTime = ( float )LM_64Bit->AcqTime.TagTime / 1000.0 ;

									/* Write sinogram and singles ---> disk */
								PromptCount = FrameInfo->Prompt ;
								RandomCount = FrameInfo->Delayed ;
										// Prompt
								if ( ( ScanMode & 0x2 ) >> 1  )
								{
									printf( "\t\t\tPrompt\n" ) ;
										/* Prompt Filename */
									memset( Info, 0, MAX_STRING_LENGTH ) ;
									strncat( Info, SinoGramFileName, strlen( SinoGramFileName ) - strlen( strstr( SinoGramFileName, "." ) ) ) ;
									strcat ( Info, "_Pr.s" ) ;
										/* Set sinogram counts */
									FrameInfo->Prompt = PromptCount ;
									FrameInfo->Delayed = 0 ;
										/* Write sinogram data ---> Disk */
									if ( ! WriteDataToDisk( Info, SinogramPr, "short int", LM_64Bit, FrameInfo, InterFileInfo ) )
									{
										sprintf( Info, "\n\tError Writing Frame %d Data ---> Disk File {%s}\n", FrameInfo->FrameNum, SinoGramFileName ) ;
										errtxt( Info ) ;
									}
								}
										// True
								if ( ScanMode & 0x1  )
								{
									printf( "\t\t\tTrue {prompt-delay}\n" ) ;
										/* True = Prompt - Random */
									for ( i1 = 0 ; i1 < FrameInfo->nElements ; i1 += 1 )
									{
										*( SinogramPr + i1 ) = *( SinogramPr + i1 ) - *( SinogramDel + i1 ); 
									}
										/* Set sinogram counts */
									FrameInfo->Prompt = PromptCount ;
									FrameInfo->Delayed = RandomCount ;
										/* Write sinogram data ---> Disk */
									if ( ! WriteDataToDisk( SinoGramFileName, SinogramPr, "short int", LM_64Bit, FrameInfo, InterFileInfo ) )
									{
										sprintf( Info, "\n\tError Writing Frame %d Data ---> Disk File {%s}\n", FrameInfo->FrameNum, SinoGramFileName ) ;
										errtxt( Info ) ;
									}
								}
										// Delayed
								if ( ( ScanMode & 0x4 ) >> 1  )
								{
									printf( "\t\t\tDelay\n" ) ;
										/* Delayed Filename */
									memset( Info, 0, MAX_STRING_LENGTH ) ;
									strncat( Info, SinoGramFileName, strlen( SinoGramFileName ) - strlen( strstr( SinoGramFileName, "." ) ) ) ;
										/* Set sinogram counts */
									FrameInfo->Prompt = 0 ;
									FrameInfo->Delayed = RandomCount ;
										/* Write sinogram data ---> Disk */
									strcat ( Info, "_Del.s" ) ;
									if ( ! WriteDataToDisk( Info, SinogramDel, "float", LM_64Bit, FrameInfo, InterFileInfo ) )
									{
										sprintf( Info, "\n\tError Writing Frame %d Data ---> Disk File {%s}\n", FrameInfo->FrameNum, SinoGramFileName ) ;
										errtxt( Info ) ;
									}
								}
									/* Write Coincidence Map */
								if ( Write_Coin_Map )
								{
									if ( ! WriteCoinMapToDisk( SinoGramFileName, CoinMapPr, CoinMapDel, LM_64Bit, FrameInfo, InterFileInfo ) )
									{
										sprintf( Info, "\n\tError Writing Frame %d Data ---> Disk File {%s}\n", FrameInfo->FrameNum, SinoGramFileName ) ;
										errtxt( Info ) ;
									}
								}
								else
								{
									printf( "\n" ) ;
								}
								/*
printf("StartTime : %.3f\tFrameEndTime : %.3f\tLM_32Bit->AcqTime.TagTime : %d\n", FrameInfo->FrameStartTime, FrameInfo->FrameEndTime, LM_64Bit->AcqTime.TagTime ) ;
								*/
									/* Reset frame start time */
								FrameInfo->FrameStartTime = FrameInfo->FrameEndTime ;
									/* Increment frame counter */
								AcqTime += *FrameTimesPtr++ ;
									/* Increment Current frame number */
								FrameInfo->FrameNum += 1 ;
									/* If last frame -- jump out of event processing loop */
								if ( FrameInfo->FrameNum > TotalNumFrame )
								{
										/* Process no more events */
									EventNum = NumEventsRead ;
								} else {
										/* Clear allocated memory */
									memset( SinogramPr, 0, FrameInfo->nElements * sizeof( short int ) ) ;
									memset( SinogramDel, 0, FrameInfo->nElements * sizeof( short int ) ) ;
									memset( CoinMapPr, 0, NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) * sizeof( int ) ) ;
									memset( CoinMapDel, 0, NUM_LAYERS * NHEADS * ( NUM_CRYSTALS * RAD_DETECTORS ) * ( NUM_CRYSTALS * AX_DETECTORS ) * sizeof( int ) ) ;
										/* Reset Counters */
									LM_64Bit->AcqTime.ms = 0 ;
									FrameInfo->NumTimeTags = 0 ;
									FrameInfo->Prompt = 0 ;
									FrameInfo->Delayed = 0 ;
										/* Reset block singles counters */
									memset( LM_64Bit->SglInfo.Sgls, 0, RAD_DETECTORS * AX_DETECTORS * NHEADS * 3 * sizeof( float ) ) ;
									LM_64Bit->SglInfo.TotSgls[ 0 ] = LM_64Bit->SglInfo.TotSgls[ 1 ] = LM_64Bit->SglInfo.TotSgls[ 2 ] = 0 ;
								}
							}
							break ;
						case 4:	/* Singles */
							break ;
						case 8:	/* Motion */
							break ;
					}
				}
			}
				/* Reset FilePtr - correct position for read */
			if ( CurrPtrPos < 0 )
			{
				//printf( "\tCurrPtrPos : %d\n", CurrPtrPos ) ;
				if ( fseek( LM_FilePtr, CurrPtrPos, SEEK_CUR ) )
				{
					sprintf( Info, "\n\tError moving file ptr by %d bytes from current position %.1f\n", CurrPtrPos, ( float )ftell( LM_FilePtr ) ) ;
					errtxt( Info ) ;
				}
			}
		}
		else
		{
				/* No more data */
			printf( "\t\t\tNo More 64-Bit List Mode Data ---> EOF!!\n" ) ;
				/* Stop processing any further frames */
			FrameInfo->FrameNum = TotalNumFrame + 1 ;
		}
	}

		/* */
	printf( "\n\tFinished\n" ) ;

/* Close Open File */
	fclose( LM_FilePtr ) ;

/* Free Allocated Memory */
	free( InterFileInfo ) ;
	free( CoinMapPr ) ;
	free( CoinMapDel ) ;
	free( SinogramDel ) ;
	free( SinogramPr ) ;
	free( FrameTimes ) ;
	free( SinogramsPerSegment ) ;
	free( FrameInfo ) ;
	free( LM_64Bit ) ;
	free( LmData_64Bit ) ;

}		/* End of Main Routine */
