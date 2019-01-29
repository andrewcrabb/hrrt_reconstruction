/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
	10-DEC-2007: Modifications for compatibility with windelays.
  29-JAN-2009: Add clean_segment_info()
               Replace segzoffset2 by m_segzoffset_span9
  07-Apr-2009: Changed filenames from .c to .cpp and removed debugging printf 
  30-Apr-2009: Integrate Peter Bloomfield __linux__ support
  02-JUL-2009: Add Transmission(TX) LUT
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "segment_info.h"

int m_current_span=0;
double m_d_tan_theta=0.0;
double m_d_tan_theta_tx=0.0;
int m_nsegs=0;
int *m_segz0=NULL, *m_segzmax=NULL;
int *m_segzoffset=NULL;
int *m_segzoffset_span9=NULL;
int conversiontable[45]={ // convert span3 to span9
		0	,0	,0	,1	,2	,1	,2	,1	,2	,3	,4	,3	,4	,3	,4	,5	,6	,5	,6	,5	,6	
	   ,7	,8	,7	,8	,7	,8	,9	,10	,9	,10	,9	,10	,11	,12	,11	,12	,11	,12	,13	,14	,13	,14	,13	,14	
	};

/**
 * nrings     : input parameter, number of rings.
 * span       : input parameter, span
 * maxrd      : input parameter, max ring difference.
 * nplanes    : output parameter, number of total planes.
 * segzoffset : output parameter, ?
 */
void init_seginfo( int nrings, int span, int maxrd,int *nplanes,double *d_tan_theta
				  ,int *nsegs
				  ,double crystal_radius,double plane_sep)
{
    int np, sp2, segnzs, segnum;  // np : number of planes, sp2 : ? 
    int i;
	int *segnz;
	int *segzoff;
	int maxseg;

    maxseg =maxrd/span;
    *nsegs  =2*maxseg+1;
    np     =2*nrings-1;
    sp2    =(span+1)/2;  

    m_segz0   = (int*)( malloc( *nsegs*sizeof(int)));
	m_segzmax = (int*) malloc( *nsegs*sizeof(int));
	

    segnz   = (int*) malloc( *nsegs*sizeof(int));
    segzoff = (int*) malloc( *nsegs*sizeof(int));
    
    segnzs   = 0;
    *nplanes = 0;      //init_seginfo2�� ����.

    for (i=0; i<*nsegs; i++){

      segnum = (1-2*(i%2))*(i+1)/2;
      if (i==0) m_segz0[0]=0;
      else m_segz0[i]=sp2+span*((i-1)/2);
      segnz[i]=np-2*m_segz0[i];
      segnzs+=segnz[i];
      if (i==0) segzoff[0]=0;
      else segzoff[i] = segzoff[i-1] + segnz[i-1];
      *nplanes += segnz[i]; //init_seginfo2�� ����.
	  m_segzmax[i]=m_segz0[i]+segnz[i]-1;
	  m_segzoffset_span9[i]=-m_segz0[i]+segzoff[i];
    }
    *d_tan_theta = span*plane_sep/crystal_radius;
	free(m_segz0);
	free(segnz);
	free(segzoff);
	free(m_segzmax);
}

void init_seginfo2( int nrings, int span, int maxrd,double *d_tan_theta
				   ,int *nsegs
				   ,double crystal_radius,double plane_sep)
{
    int np, sp2, segnzs, segnum;
    int i;
	int *segnz;
	int *segzoff;
	int maxseg;

    maxseg=maxrd/span;
    *nsegs=2*maxseg+1;
    np=2*nrings-1;
    sp2=(span+1)/2;
    m_segz0=(int*) malloc( *nsegs*sizeof(int));
    segnz=(int*) malloc( *nsegs*sizeof(int));
    segzoff=(int*) malloc( *nsegs*sizeof(int));
    m_segzmax=(int*) malloc( *nsegs*sizeof(int));
    segnzs=0;
    for (i=0; i<*nsegs; i++)
    {
      segnum=(1-2*(i%2))*(i+1)/2;
      if (i==0) m_segz0[0]=0;
      else m_segz0[i]=sp2+span*((i-1)/2);
      segnz[i]=np-2*m_segz0[i];
      segnzs+=segnz[i];
      if (i==0) segzoff[0]=0;
      else segzoff[i] = segzoff[i-1] + segnz[i-1];
	  m_segzmax[i]=m_segz0[i]+segnz[i]-1;
	  m_segzoffset[i]=-m_segz0[i]+segzoff[i];
    }
    *d_tan_theta = span*plane_sep/crystal_radius;
	printf("maxseg=%d\n",maxseg);
	free(segnz);
	free(segzoff);

}

int init_segment_info(int *nsegs,int *nplanes,double *d_tan_theta
					  ,int maxrd,int span,int NYCRYS,double crystal_radius,double plane_sep){

	int i = 0;
	*nsegs=2*(maxrd/span)+1;
	m_segzoffset_span9=(int *) (calloc(*nsegs,sizeof(int)));	
    *nsegs=2*(maxrd/3)+1;
	m_segzoffset=(int *) (calloc(*nsegs,sizeof(int)));
    init_seginfo( NYCRYS, span, maxrd,nplanes,d_tan_theta
				  ,nsegs
				  ,crystal_radius,plane_sep);
    init_seginfo2(NYCRYS, 3, maxrd,d_tan_theta
				  ,nsegs
				  ,crystal_radius,plane_sep);
	if(span==9){
    for(i=0;i<*nsegs;i++) {
      m_segzoffset[i]=m_segzoffset_span9[conversiontable[i]];
    }
	}
  free(m_segzoffset_span9);
	
	
	return 1;
}

void clean_segment_info()
{
	if (m_segz0!=NULL) free(m_segz0);
	if (m_segzmax!=NULL) free(m_segzmax);
	if (m_segzoffset!=NULL) free(m_segzoffset);
}

