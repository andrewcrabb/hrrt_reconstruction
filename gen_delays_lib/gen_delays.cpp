/* $Id: gen_delays.c,v 1.3 2007/01/08 06:04:55 cvsuser Exp $  */
/* Authors: Inki Hong, Dsaint31, Merence Sibomana, Peter Bloomfield
   Creation 08/2004
   Modification history: Merence Sibomana
   10-DEC-2007: Modifications for compatibility with windelays.
   02-DEC-2008: Bug fix in hrrt_rebinner_lut_path
   07-Apr-2009: Changed filenames from .c to .cpp and removed debugging printf 
   30-Apr-2009: Integrate Peter Bloomfield __linux__ support
   11-May-2009: Add span and max ring difference parameters
   02-JUL-2009: Add Transmission(TX) LUT
   21-JUL-2010: Bug fix argument parsing errors after -v (or other single parameter)
   Use scan duration argument only when valid (>0)
   2/6/13 ahc: Made hrrt_rebinner.lut a required command line argument.
               Took out awful code accepting any hrrt_rebinner found anywhere as the one to use.
*/
#include <cstdlib>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
// #include <malloc.h>
#include <string.h>
#include <time.h>
#include <xmmintrin.h>

#define _MAX_PATH 256
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "gen_delays.h"
#include "segment_info.h"
#include "geometry_info.h"
#include "lor_sinogram_map.h"

// #define LUT_FILENAME "hrrt_rebinner.lut"

float tau=6.0e-9f;
float ftime=1.0f;

int *bin_number_to_nview;
int *bin_number_to_nproj;

//dsaint31 float *head_lthick = NULL;
//dsaint31 float *head_irad   = NULL;

typedef struct {
  int mp;
  float **delaydata;
  float *csings;
} COMPUTE_DELAYS;


static char progid[]="$Id: gen_delays.c,v 1.3 2007/01/08 06:04:55 cvsuser Exp $";


float koln_lthick[8]   = {0.75f, 0.75f, 1.0f, 0.75f, 0.75f, 0.75f, 1.0f, 0.75f};

static void usage(const char *prog)
{
  printf("%s - generate delayed coincidence data from crystal singles\n", prog);
  printf("usage: gen_delays -h coincidence_histogram -O delayed_coincidence_file -t count_time <other switches>\n");
  printf("    -v              : verbose\n");
  printf("    -C csingles     : old method specifying precomputed crystal singles data\n");
  printf("    -p ne,nv        : set sinogram size (ne=elements, nv=views) [256,288]\n");
  printf("    -s span,maxrd   : set span and maxrd values [9,67]\n");
  printf("    -g p,d,t        : adjust physical parameters (pitch, diameter, thickmess)\n");
  printf("    -k              : use Koln HRRT geometry (default to normal)\n");
  printf("    -T tau          : time window parameter [6.0 10-9 sec]\n");
  // ahc
  printf("  * -r rebinner_file: Full path of 'hrrt_rebinner.lut' file (required)\n");
  exit(1);
}

int compute_delays( int mp,float **delays_data,float *csings)
{
  int ax, ay, bx, by, ahead, bhead;
  int alayer, blayer, rd;
  int axx,bxx,nbd;
  int headxcrys=NHEADS*NXCRYS;
  float aw;
  float cay;
  float awtauftime;
  float *dptr;
  int current_view;
  int current_proj;
  int num=0;
  int bs,be;
  SOL *sol;
  int segnum,plane;
  float dz2[104],seg;
  float tauftime;
	
  ahead = hrrt_mpairs[mp][0];
  bhead = hrrt_mpairs[mp][1];
  tauftime=tau*ftime;
  //fprintf(stdout,"2 Generating delays for MP %d [H%d,H%d]\n", mp, ahead, bhead);
  fflush(stdout);

  for (alayer=0; alayer<NDOIS; alayer++) {
    for (ay=0; ay<NYCRYS; ay++) {
      //	if (ay%10==0) printf("Generating delays for MP %d [H%d,H%d] %d\t%d \r", mp, ahead, bhead,alayer,ay);
      cay=m_c_zpos2[ay];
      bs=1000;be=-1000;

      //get rd,dz2[by],bs,be
      for (by=0;by<NYCRYS;by++) {
	dz2[by]=m_c_zpos[by]-m_c_zpos[ay]; // z diff. between det A and det B
        rd = ay-by; if (rd<0) rd=by-ay; 
	if (rd < maxrd+6) {  // dsaint31 : why 6??
	  if (bs>by) bs=by; //start ring # of detB
	  if (be<by) be=by; //end   ring # of detB
	}
      }

      for (ax=0; ax<NXCRYS; ax++) {
				
	axx=ax+NXCRYS*alayer;
	aw=csings[alayer*NHEADS*NXCRYS*NYCRYS+ay*NHEADS*NXCRYS+ahead*NXCRYS+ax];
	awtauftime=aw*tauftime; // 2 * tau * singles at detA
	if (awtauftime==0) continue;

	for (blayer=0; blayer<2; blayer++) {
	  //nbl=NXCRYS*blayer;
	  //nbl;
	  bxx=NXCRYS*blayer;
	  for (bx=0; bx<NXCRYS; bx++,bxx++) {
	    if (m_solution[mp][axx][bxx].nsino==-1) continue;
	    sol=&m_solution[mp][axx][bxx];
						
	    //dsaint31
	    dptr = delays_data[sol->nsino];//result bin location
	    current_view = bin_number_to_nview[sol->nsino];
	    current_proj = bin_number_to_nproj[sol->nsino];

	    nbd  = blayer*NHEADS*NXCRYS*NYCRYS+bhead*NXCRYS+bx+headxcrys*bs;
	    for (by=bs; by<=be; by++,nbd+=headxcrys) {

	      plane        = (int)(cay+sol->z*dz2[by]);

	      seg = (float)(0.5+dz2[by] * sol->d);
              segnum = (int)seg;
	      if (seg<0) segnum=1-(segnum<<1);
	      else      segnum=segnum<<1;

	      //dsaint31
	      if (m_segplane[segnum][plane]!=-1) dptr[m_segplane[segnum][plane]] +=csings[nbd]*awtauftime;
	      //if (m_segplane[segnum][plane]!=-1) 
	      //	delays_data[m_segplane[segnum][plane]][current_view][current_proj] +=csings[nbd]*awtauftime;
	    }
	  }
	}
      }
    }
  }
  return 1;
}



// Compute the delayed crystal coincidence rates for the specified crystal singles rates

void compute_drate( float *srate, float tau, float *drate)
{
  float hsum[NHEADS], ohead_sum;
  int head, layer, cx, cy, i, j, ohead;

  // First we compute the total singles rates for each of the heads...

  for (head=0; head<NHEADS; head++) {
    hsum[head]=0.0;
    for (layer=0; layer<NDOIS; layer++)
      for (cx=0; cx<NXCRYS; cx++)
	for (cy=0; cy<NYCRYS; cy++) {
	  i=NXCRYS*head+cx+NXCRYS*NHEADS*cy+NXCRYS*NYCRYS*NHEADS*layer;
	  hsum[head] += srate[i];
	}
  }

  // Now we compute the delayed coincidence rate for each crystal...

  for (head=0; head<NHEADS; head++) {
    ohead_sum=0.0;
    for (j=0; j<5; j++) {
      ohead = (head+j+2)%NHEADS;
      ohead_sum += hsum[ohead];
    }
    for (layer=0; layer<NDOIS; layer++)
      for (cx=0; cx<NXCRYS; cx++)
	for (cy=0; cy<NYCRYS; cy++) {
	  i=NXCRYS*head+cx+NXCRYS*NHEADS*cy+NXCRYS*NYCRYS*NHEADS*layer;
	  drate[i] = srate[i]*tau*ohead_sum;
	}
  }
}

void estimate_srate( float *drate, float tau, float *srate)
{
  float hsum[NHEADS], ohead_sum;
  int head, layer, cx, cy, i, j, ohead;

  // First we compute the total singles rates for each of the heads...

  for (head=0; head<NHEADS; head++) {
    hsum[head]=0.0;
    for (layer=0; layer<NDOIS; layer++)
      for (cx=0; cx<NXCRYS; cx++)
	for (cy=0; cy<NYCRYS; cy++) {
	  i=NXCRYS*head+cx+NXCRYS*NHEADS*cy+NXCRYS*NYCRYS*NHEADS*layer;
	  hsum[head] += srate[i];
	}
  }

  // Now we update the estimated singles rate for each crystal...

  for (head=0; head<NHEADS; head++) {
    ohead_sum=0.0;
    for (j=0; j<5; j++) {
      ohead = (head+j+2)%NHEADS;
      ohead_sum += hsum[ohead];
    }
    for (layer=0; layer<NDOIS; layer++)
      for (cx=0; cx<NXCRYS; cx++)
	for (cy=0; cy<NYCRYS; cy++) {
	  i=NXCRYS*head+cx+NXCRYS*NHEADS*cy+NXCRYS*NYCRYS*NHEADS*layer;
	  srate[i]=(srate[i]+drate[i]/(tau*ohead_sum))/2.0f;
	  //                    if (drate[i] == 0.0) srate[i]=0.0;  // dead crystal - no delayed coins
	}
  }
}

void compute_initial_srate( float *drate, float tau, float *srate)
{
  float dtotal=0.0, stotal;
  int i;

  for (i=0; i<NHEADS*NXCRYS*NYCRYS*NDOIS; i++) dtotal += drate[i];
  stotal=(float)(8.0*sqrt(dtotal/(40.*tau)));
  for (i=0; i<NHEADS*NXCRYS*NYCRYS*NDOIS; i++) srate[i] = drate[i] * stotal / dtotal;
}

int errtotal( int *ich, float *srate, int nvals, float dt)
{
  int i;
  int errsum=0, err;

  for (i=0; i<nvals; i++) {
    err = (int)(ich[i]-(0.5+srate[i]*dt));
    errsum += err*err;
  }
  return(errsum);
}

int compute_csings_from_drates( int ncrys, int *dcoins, float tau, float dt, float *srates)
{
  int iter, i;
  float *xrates, *drates;
  int err, erbest;
    
  drates = (float*) calloc( ncrys, sizeof(float));
  xrates = (float*) calloc( ncrys, sizeof(float));
  for (i=0; i<ncrys; i++) drates[i] = dcoins[i]/dt;
  compute_initial_srate( drates, tau, srates);
  for (iter=0; iter<100; iter++) {
    compute_drate( srates, tau, xrates);  // compute the expected delayed rate
    err = errtotal( dcoins, xrates, ncrys, dt);       // compute the error
    //        if (!quiet) printf("%3d %i\n", iter, err);
    //        if (iter && ((err >= erbest) || (err==0))) break;    // time to stop?
    if (iter && (err==0)) break; // only stop when we converge
    erbest = err;
    estimate_srate( drates, tau, srates); // update the estimate
  }
  free(drates);
  free(xrates);
  return(iter);
}
     
void *pt_compute_delays(void *ptarg)
{
  COMPUTE_DELAYS *arg = (COMPUTE_DELAYS *) ptarg;
  //	printf("%x\t%x\t%d\n",delays_data,arg->delaydata,arg->mp);
  compute_delays(arg->mp,arg->delaydata,arg->csings);
  pthread_exit( NULL ) ;
}

/**
 * in_inline : 0:file save and alone | 1: file save and inline | 2: file unsave and inline
 */
int gen_delays(int argc, char **argv,int is_inline, float scan_duration,
               float ***result,FILE *p_coins_file,char *p_delays_file, 
               int _span, int _maxrd,
               // My addition ahc: Rebinner LUT file now a required argument.
               char *p_rebinner_lut_file
               ) 
{
  int i, n, mp,j;
  struct timeval t0, t1, t2, t3;
  int dtime1, dtime2, dtime3, dtime4;
  FILE *fptr;
  // ahc
  char *rebinner_lut_file = NULL;
  char *csingles_file=NULL;
  char *delays_file=NULL;
  char *coins_file=NULL;
  char *output_csings_file=NULL;
  char *optarg;
  int c;
  int nprojs=256;
  int nviews=288;
  int nvals;
  float pitch=0.0;
  float diam=0.0;
  float thick=0.0;
  float *dtmp;
  int *coins=NULL;
  float **delays_data;
  int niter=0;
  int span=_span;
  int nplanes=0;

  int quiet=1;
  int kflag=0;	
  float *csings=NULL;

  int threadnum ;
  pthread_t threads[ 4 ] ;
  pthread_attr_t attr;
  unsigned int threadID;

  COMPUTE_DELAYS comdelay[4];
	
  // 1  1  1  1  1   2   2   2   2   2		  // machine number for MPI version
  // 2  1  0  1  2   2   1   0   1   2		  // amount of load
  int mporder[2][10]={{1, 2, 3, 4, 5,  11, 12, 13, 14, 18}, // for the load balancing 
		      {6, 7, 8, 9, 10, 15, 16, 17, 19, 20}};
  // const char *rebinner_lut_file=NULL;
   
  delays_file = p_delays_file;
  if (scan_duration>0) ftime = scan_duration;
  maxrd = _maxrd;

  // Process command line arguments.
  if (is_inline == 0) {
    for (i=0; i < argc; i++) {
      if (argv[i][0] != '-') 
	continue;
      c = argv[i][1];
      if (argv[i][2] == 0) {
        if (i < argc - 1 ) {
          if (argv[i+1][0] != '-') 
	    i++;
	}
	optarg=argv[i];
      }	else {
	optarg=(char *) &argv[i][2];
      }

      switch(c) {
	// ahc
      case 'r' : rebinner_lut_file = optarg;	// hrrt_rebinner.lut
      case 'v':   quiet = 0; break;   // -v don't be quiet any longer
      case 'h':   coins_file = optarg; break; // coincidence histogram (int 72,8,104,4)
      case 'p':   sscanf( optarg, "%d,%d", &nprojs, &nviews); break; // -p nprojs,nviews - set sinogram size
      case 's':   sscanf( optarg, "%d,%d", &span, &maxrd); break;    // -s span,maxrd - set 3D parameters
      case 'g':   sscanf( optarg, "%f,%f,%f", &pitch, &diam, &thick); break;    // -g pitch,diam,thick
      case 'C':   csingles_file = optarg; break;  // -C crystal singles file
      case 'k':   kflag = 1; break;          // user Koln geometry
      case 'O':   delays_file = optarg; break;  // -O delays_file
      case 'S':   output_csings_file = optarg; break; // -S save_csings_file
      case 'T':   sscanf( optarg, "%f", &tau); break;
      case 't':   sscanf( optarg, "%f", &ftime); break;
      }
    }
    if ( coins_file == NULL || delays_file == NULL || rebinner_lut_file == NULL ) 
      usage("gen_delays");		
  } else {
    // inline mode.
    coins_file  = (char *)"memory_mode";
    // ahc this value is overridden by argv if called as main
    if (strlen(p_rebinner_lut_file)) {
      rebinner_lut_file = p_rebinner_lut_file;
    } else {
      fprintf(stderr, "gen_delays.cpp:main(): Rebinner file must be specified\n");
      exit(1);
    }
  }
  if (is_inline < 2 && !delays_file) {
    fprintf(stdout,"Output File must be specified with -O <filename>\n");		
    exit(1);
  }
  if (!csingles_file && !coins_file && p_coins_file==NULL)
    fprintf(stdout,"Input Crystal Singles or Coincidence Histogram file must be specified with -h or -C <filename>\n");
		
  gettimeofday( &t0, NULL ) ;
  
  head_crystal_depth = (float*)calloc(NHEADS, sizeof(float));
  for (i=0; i<NHEADS;i++) 
    if (kflag) head_crystal_depth[i] = koln_lthick[i];
    else head_crystal_depth[i] = 1.0f;

  init_geometry_hrrt( nprojs, nviews, pitch, diam, thick);
  init_segment_info(&m_nsegs,&nplanes,&m_d_tan_theta,maxrd,span,NYCRYS,m_crystal_radius,m_plane_sep);


  // ahc hrrt_rebinner.lut now a required command line argument.
  // if ((rebinner_lut_file=hrrt_rebinner_lut_path())==NULL)
  //   {
  //     fprintf(stdout,"Rebinner LUT file not found\n");
  //     exit(1);
  //   }
  // fprintf(stdout,"Using Rebinner LUT file %s\n", rebinner_lut_file);
  // init_lut_sol(rebinner_lut_file, m_segzoffset);
  fprintf(stderr,"Using Rebinner LUT file %s\n", rebinner_lut_file);
  init_lut_sol(rebinner_lut_file, m_segzoffset);

  clean_segment_info();
	
  free(m_crystal_xpos);
  free(m_crystal_ypos);
  free(m_crystal_zpos);
  free(head_crystal_depth);

  //-------------------------------------------------------
  // delayed true output value init.
  nsino=nprojs*nviews;
  //printf("nplanes= %d %d\n",nplanes,m_nsegs);
  bin_number_to_nview  = (int *) calloc(nsino,sizeof(int));
  bin_number_to_nproj = (int *) calloc(nsino,sizeof(int));
  for (j=0,n=0;j<m_nviews;j++) {
    for (c=0;c<m_nprojs;c++) {
      //dsaint31
      bin_number_to_nview[n] = j;
      bin_number_to_nproj[n]= c;			
      n++;
    }
  }

  if (is_inline==2 && result == NULL) {
    result = (float ***) malloc(nplanes*sizeof(float**));
    for (i=0;i<nplanes;i++) {
      result[i]=(float **) malloc(m_nviews*sizeof(float*));
      for (j=0;j<m_nviews;j++) {
	result[i][j] = (float *) malloc(m_nprojs*sizeof(float));
      }
    }
  }
  if (result != NULL) {
    for (i=0;i<nplanes;i++)
      for (j=0;j<m_nviews;j++)
	memset(&result[i][j][0],0,m_nprojs*sizeof(float));
  }

  delays_data = (float**) calloc( nsino, sizeof(float*));
  for (i=0;i<nsino;i++) {
    delays_data[i]=(float *) calloc (nplanes,sizeof(float));		
    if (delays_data[i]==NULL) {
      printf("error allocation delays_data\n");
      exit(1);
    }
    memset(delays_data[i],0,nplanes*sizeof(float));
  }
  if (!delays_data) printf("malloc failed for delays_data\n");
	
  //-------------------------------------------------------
  // make singles. ( load or estimate)
  nvals  = NDOIS*NHEADS*NXCRYS*NYCRYS; // # of detecters
  
  csings = (float*) calloc( nvals, sizeof(float));
  if (!csings) printf("malloc failed for csings data\n");
  if (csingles_file) {
    fptr = fopen( csingles_file, "rb");
    if (!fptr) printf("Can't open crystal singles file '%s'\n", csingles_file);
    n = (int)fread( csings, sizeof(float), nvals, fptr);
    fclose( fptr);
    if (n != nvals) printf("Only read %d of %d values from csings file '%s'\n", n, nvals, csingles_file);
  }
  if (coins_file || p_coins_file!=NULL ) {
    if (p_coins_file==NULL) fptr = fopen( coins_file,"rb");
    else fptr = p_coins_file;

    if (!fptr) { 
      printf("Can't open coincidence histogram file '%s'\n", coins_file);
      exit(1);
    }
    coins=(int*) calloc( nvals*2, sizeof(int)); // prompt followed by delayed
    if (!coins) printf("calloc failure for coins array\n");
    n = (int)fread( coins, sizeof(int), nvals*2, fptr);
    if (fptr != p_coins_file) fclose(fptr);
    if (n != 2*nvals) printf("Not enough data in coinsfile '%s' (only %d of %d)\n", coins_file, n, nvals*2);
    gettimeofday( &t2, NULL);
    // we only used the delayed coins (coins+nvals)vvvvvv
    niter = compute_csings_from_drates( nvals, coins+nvals, tau, ftime, csings);
    gettimeofday( &t3, NULL);
    printf("csings computed from drates in %d iterations (%ld msec)\n", niter, ( ( ( t3.tv_sec * 1000 ) + ( int )( (double)t3.tv_usec / 1000.0 ) ) - ( ( t2.tv_sec * 1000 ) + ( int )( (double)t2.tv_usec / 1000.0 ) ) ) );
    free(coins);
    if (output_csings_file) {
      fptr = fopen( output_csings_file, "wb");
      if (!fptr) printf("ERROR - unable to save computed singles data to '%s'...continuing\n",
			output_csings_file);
      else {
        fwrite( csings, sizeof(float), nvals, fptr);
        fclose(fptr);
        printf("Computed Singles (from Delayed Coincidence Histogram) stored in '%s'\n",
	       output_csings_file);
      }
    }
  }

  //-------------------------------------------------------
  // check if the delay_file is writable.
  if (is_inline<2) {
    fptr = fopen( delays_file, "wb");
    if (!fptr) {
      fprintf(stdout,"Can't create delayed coincidence output file '%s'\n", delays_file);
      exit(1);
    }
  }

  //-------------------------------------------------------
  // calculate delayed true
	
  comdelay[0].delaydata=delays_data;//result;
  comdelay[1].delaydata=delays_data;//result;
  comdelay[0].csings=csings;
  comdelay[1].csings=csings;
  comdelay[2].delaydata=delays_data;//result;
  comdelay[3].delaydata=delays_data;//result;
  comdelay[2].csings=csings;
  comdelay[3].csings=csings;

  for (mp=0; mp<5; mp++) {
    gettimeofday( &t1, NULL );
    comdelay[0].mp=mporder[0][mp];
    comdelay[1].mp=mporder[1][mp];
    comdelay[2].mp=mporder[0][mp+5];
    comdelay[3].mp=mporder[1][mp+5];
    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for ( threadnum = 0 ; threadnum < 4 ; threadnum += 1 )
      {
	pthread_create( &threads[ threadnum ], &attr, &pt_compute_delays, &comdelay[ threadnum ] ) ;
      }
    /* Free attribute and wait for the other threads */
    pthread_attr_destroy( &attr ) ;
    for ( threadnum = 0 ; threadnum < 4 ; threadnum += 1 )
      {
	pthread_join( threads[ threadnum ] , NULL ) ;
      }
    gettimeofday( &t2, NULL);
    fprintf(stdout,"%d\t%ld msec   \n",mp+1             , ( ( ( t2.tv_sec * 1000 ) + ( int )( (double)t2.tv_usec / 1000.0 ) ) - ( ( t1.tv_sec * 1000 ) + ( int )( (double)t1.tv_usec / 1000.0 ) ) ) );
    fflush(stdout);
  }
  gettimeofday( &t1, NULL);
  fprintf(stdout,"Smooth Delays computed in %ld msec.\n", ( ( ( t1.tv_sec * 1000 ) + ( int )( (double)t1.tv_usec / 1000.0 ) ) - ( ( t0.tv_sec * 1000 ) + ( int )( (double)t0.tv_usec / 1000.0 ) ) ) );
  gettimeofday( &t2, NULL );
  fprintf(stdout,"...reduced in %ld msec   \n",mp+1     , ( ( ( t2.tv_sec * 1000 ) + ( int )( (double)t2.tv_usec / 1000.0 ) ) - ( ( t1.tv_sec * 1000 ) + ( int )( (double)t1.tv_usec / 1000.0 ) ) ) );
  fflush(stdout);	
	
  free(m_solution[0]);
  for (i=1;i<21;i++) {
    for (j=0;j<NXCRYS*NDOIS;j++) {
      //fprintf(stdout,"%d:%d\n",i,j);
      if (m_solution[i][j] !=NULL) free(m_solution[i][j]);
    }
    //fprintf(stdout,"%d:%d\n",i,j);
    free(m_solution[i]);
  }
	
  free(m_solution);
  free(csings);
  free(m_c_zpos);
  free(m_c_zpos2);
	
  for (i=0;i<63;i++) {
    free(m_segplane[i]);
  }
	
  if (m_segplane!=NULL) free(m_segplane);
	

  //-------------------------------------------------------
  // file write
  dtmp  = (float *) calloc( nsino, sizeof(float));		
  for (i=0;i<nplanes;i++) {
    for (n=0;n<nsino;n++) {
      dtmp[n]=delays_data[n][i];
    }
    if (is_inline <2) {
      fwrite( dtmp, sizeof(float), nsino, fptr);
    }
    else {
      for (j=0,n=0;j<m_nviews;j++) {								
	memcpy(result[i][j],&dtmp[n],sizeof(float)*m_nprojs);// = dtmp[n];
	n=n+m_nprojs;				
      }	
    }
  }
		
  free(bin_number_to_nview);
  free(bin_number_to_nproj);

  for (n=0;n<nsino;n++) {
    free(delays_data[n]);
  }
  free(delays_data);
  free(dtmp);
  if (is_inline<2) {
    fclose( fptr);
  }
  
  gettimeofday( &t3, NULL);
  dtime1 = ( ( ( t1.tv_sec * 1000 ) + ( int )( (double)t1.tv_usec / 1000.0 ) ) - ( ( t0.tv_sec * 1000 ) + ( int )( (double)t0.tv_usec / 1000.0 ) ) ) ;
  dtime2 = ( ( ( t2.tv_sec * 1000 ) + ( int )( (double)t2.tv_usec / 1000.0 ) ) - ( ( t1.tv_sec * 1000 ) + ( int )( (double)t1.tv_usec / 1000.0 ) ) ) ;
  dtime3 = ( ( ( t3.tv_sec * 1000 ) + ( int )( (double)t3.tv_usec / 1000.0 ) ) - ( ( t2.tv_sec * 1000 ) + ( int )( (double)t2.tv_usec / 1000.0 ) ) ) ;
  dtime4 = ( ( ( t3.tv_sec * 1000 ) + ( int )( (double)t3.tv_usec / 1000.0 ) ) - ( ( t0.tv_sec * 1000 ) + ( int )( (double)t0.tv_usec / 1000.0 ) ) ) ;
  printf("...stored to disk in %d msec.\n", dtime3);
  printf("Total time %d msec.\n", dtime4);
  return 1;
}

