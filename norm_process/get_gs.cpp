/*
  Module geo_compute
  Scope: HRRT only
  Author: Merence Sibomana
  Creation 08/2005
  Purpose: This module reads HRRT geometry coefficients and computes the geometric factor to be applied to LOR
  and before fan sum
  Modification History:
  31-JUL-2008: Added include "gr_ga.h" for default hardcoded values
  13-Nov-2008: Use linear interpolation for geometric factors
  09-Apr-2009: Use last geometric factor for angle higher than NUM_ANGLES
  Some values are sligthly higher than 28 degrees
  30-Apr-2009: Use gen_delays_lib C++ library for rebinning and remove dead code
  Integrate Peter Bloomfield _LINUX support
  28-May-2009: Added new geometric profiles format using 4 linear segments cosine
  16-Oct-2009: Changed geometric file windows directory to c:\cps\users_sw
*/
#include <math.h>
#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif
#define NUM_ANGLES 70
#define NUM_DISCRETE_ANGLES 14
#define LINE_SIZE 80
#ifdef WIN32
#define SEPARATOR '\\'
#define INSTALL_DIR "c:\\cps\\users_sw"
#else
#define SEPARATOR '/'
#define _MAX_PATH 256
#endif
#include "get_gs.h"
#include "gr_ga.h"
#include <string.h>
#include "scanner_params.h"
#include <gen_delays_lib/geometry_info.h>
#include <string>
#include <vector>

static int ha[] = {0,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,3,4,4,5};
static int hb[] = {2,3,4,5,6,3,4,5,6,7,4,5,6,7,5,6,7,6,7,7};
static int la[] = {0,0,1,1};
static int lb[] = {0,1,0,1};
static double cvtrad = 180.0/M_PI;
static float gra[NUM_ANGLES][NLAYERS];  // back layers crystals geometric factors
static const char *default_geom_fname = "geom_cor.dat";

static double *primary = NULL;
static int na=5; // heads in coincidences
static int analytic_flag=0;

/*
  pro get_gs_4_fansum

  s = get_scanner_params()

  get_hrrt_dtct2, xd, yd, zd

  pai = 3.14159265
  cvtrad = 180./pai
  primary = 1.*[0,45,90,135,180,225,270,315]/cvtrad

  ;from -70 to 70 every 10)
  nr = 70-(-70)
  na = 5	; #heads in coincidence

  ; Module pairs
  ha = [0,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,3,4,4,5] 
  hb = [2,3,4,5,6,3,4,5,6,7,4,5,6,7,5,6,7,6,7,7] 

  ;Layers combination
  la = [0,0,1,1]
  lb = [0,1,0,1]
  set = ['00','01','10','11']
*/

static void init_params()
{
  if (primary==NULL)
    {
      primary = (double*)calloc(8, sizeof(double));
      for (int i=0; i<8; i++) primary[i] = i*M_PI/4;
    }
}
typedef struct _LineFit { float a, b; } LineFit;
static LineFit backLayerFit[] = { {-5.46f,23.4f}, {1.07f, 1.83f}, {1.19f, -1.12f}, {1.32f, -6.92f} };
static LineFit frontLayerFit[] = { {-2.26f,15.0f}, {0.62f, 0.82f}, {0.72f, -2.08f}, {1.08f, -18.85f} };
static float breakPoint[] = {0.0f, 5.0f,22.5f, 45.0f}; 
static float geom_cor(double theta, int layer)
{
  double x=fabs(theta); // absolute theta in degrees
  double y=0.0;    // acos(g) in degrees
  int idx=0;
  if (x >= breakPoint[1])
    {
      if (x >= breakPoint[3]) idx=3;
      else if (x >= breakPoint[2]) idx=2;
      else idx = 1;
    }
  if (layer == 0) y = frontLayerFit[idx].a*x + frontLayerFit[idx].b;
  else y = backLayerFit[idx].a*x + backLayerFit[idx].b;
  if (y<0.0f) y=0.0f;
  return (float)(cos(y/cvtrad));
}

int get_gs(const char *geom_fname, FILE *log_fp)
{
  int lc=4; // layer combinations
  int i,j,k,l,m;
  int ii, jj;
  double x1,y1,z1,x2,y2,z2;  //  crystal a and b coordinates before symmetry rotation
  double xx1,yy1,xx2,yy2;  //  crystal a and b actual coordinates
  double theta1, theta2;
  float f;
  int angle1, angle2;
  FILE *fp=NULL;
  char path[_MAX_PATH], line[LINE_SIZE];
  std::vector<std::string> lines;
  std::vector<float> discrete_gr;
  int lai=0, lbi=0;   // inverted layer convention
  // Read file
  /*
    gs = fltarr(72,72,5,4,2)
    ;one for each crystal

    openr,1,'tx_geo.dat'
    buf = fltarr(28,8,9,2)
    readf,1,buf
    close,1
    gr = reform(buf(*,0,0,*))
    ga = reform(buf(*,0,0,*))

    ;calculate transaxial incident angle wrt e1 & e2
    print,systime(),' Calculating transaxial geometric'
  */
  if (geom_fname == NULL)
    {	// default geometry file
#ifdef WIN32
      sprintf(path,"%s%c%s", INSTALL_DIR, SEPARATOR, default_geom_fname);
#else
      char *gmini_dir = getenv("GMINI");
      if (gmini_dir == NULL) 
	{
	  fprintf(log_fp, "GMINI environment variable not set: ", path);
	}
      else
	{
	  sprintf(path,"%s%c%s", gmini_dir, SEPARATOR, default_geom_fname);
	}
#endif
    } 
  else strcpy(path, geom_fname);

  if ((fp=fopen(path,"r")) != NULL) {
      fprintf(log_fp, "Using file %s for radial and axial geometric factors\n", path);
      while (fgets(line, sizeof(line), fp) != NULL) {
	  if (sscanf(line,"%g", &f) == 1)
	    lines.push_back(line); // no empty lines
	}
      fclose(fp);
  } else {
    fprintf(log_fp, "file %s not found\n", path);
  }
  if (lines.size() == 4) 
    analytic_flag = 1;
  if (analytic_flag) {
      fprintf(log_fp, "Using %s parameter file for radial and axial geometric factors\n", path); 
      for (k=0; k<4; k++) {
	  if (sscanf(lines[k].c_str(), "%g %g,%g %g,%g",
		     &breakPoint[k], &(frontLayerFit[k].a), &(frontLayerFit[k].b),
		     &(backLayerFit[k].a), &(backLayerFit[k].b)) != 5) {
	      fprintf(log_fp, "Error decoding parameters in %s\n", lines[k].c_str());
	      return 0;
	    }
	}
      for (i=0; i<NUM_ANGLES; i++)
	for (l=0; l<NLAYERS; l++) {
	    gra[i][l] = geom_cor(i,l);
	  }
    } else {
      fprintf(log_fp, "Using default radial and axial geometric factors\n");
      // use only positive part because symetric 
      angle1 = 0;  // angle
      for (i=0; i<NUM_DISCRETE_ANGLES; i++)
	{
	  j = NUM_DISCRETE_ANGLES+i;
	  for (l=0; l<NLAYERS; l++)
	    {
	      if (i>0)
		{  // fill previous value
		  f = (def_gr_ga[j*NLAYERS+l]-def_gr_ga[(j-1)*NLAYERS+l])/(NUM_ANGLES/NUM_DISCRETE_ANGLES);

		  for (k=0; k<NUM_ANGLES/NUM_DISCRETE_ANGLES; k++)
		    {
		      gra[angle1+k][l] = def_gr_ga[(j-1)*NLAYERS+l] + f*(k+1);
		    }
		} 
	      else gra[angle1][l] = def_gr_ga[(j-1)*NLAYERS+l];
	    }
	  if (i==0) angle1++;
	  else angle1 += NUM_ANGLES/NUM_DISCRETE_ANGLES;
	}
      // fill last values with previous slope
      for (l=0; l<NLAYERS; l++)
	{
	  for (k=1; k<NUM_ANGLES/NUM_DISCRETE_ANGLES; k++)
	    {
	      gra[angle1+k-1][l] = gra[angle1-1][l] + f*(k);
	    }
	}
    }
  fflush(log_fp);
  //	return 1;
  init_params();
	
  // make gr

  /* IDL code
     for i = 0,3 do begin
     for j = 0,4 do begin
     for k = 0,71 do begin
     for l = 0,71 do begin
     x1 = xd(k,la(i),ha(j),0)		; Transaxial [72,2,8,104]
     y1 = yd(k,la(i),ha(j),0)
     x2 = xd(l,lb(i),hb(j),0)
     y2 = yd(l,lb(i),hb(j),0)
     ; note that rotate head no useful here since ha(j) is head 0 when j=0-4
     xx1 = cos(primary(ha(j)))*x1 - sin(primary(ha(j)))*y1
     yy1 = sin(primary(ha(j)))*x1 + cos(primary(ha(j)))*y1
     xx2 = cos(primary(ha(j)))*x2 - sin(primary(ha(j)))*y2
     yy2 = sin(primary(ha(j)))*x2 + cos(primary(ha(j)))*y2
     theta1 = atan((xx2-xx1)/abs((yy2-yy1)))
     angle1 = fix((theta1*cvtrad+nr/2)/na)
     gs(k,l,j,i,0) = gr(angle1,la(i))			; CM add layer
     ;rotate panel of d2 to top to use same equation
     xx1 = cos(primary(hb(j)))*x1 - sin(primary(hb(j)))*y1
     yy1 = sin(primary(hb(j)))*x1 + cos(primary(hb(j)))*y1
     xx2 = cos(primary(hb(j)))*x2 - sin(primary(hb(j)))*y2
     yy2 = sin(primary(hb(j)))*x2 + cos(primary(hb(j)))*y2
     theta2 = atan((xx1-xx2)/abs((yy2-yy1)))
     angle2 = fix((theta2*cvtrad+nr/2)/na)			; could be round?			
     gs(k,l,j,i,1) = gr(angle2,lb(i))			; CM add layer
     endfor
     endfor
     endfor
     endfor
     openw,1,'gr_4_fansum_FB.dat'
     writeu,1,gs
     close,1
  */

  for (i=0; i<lc; i++)   // layer combinations
    {
      for (j=0; j<na; j++) // head in coincidences
	{
	  for (k=0; k<NXCRYS; k++)
	    {
	      lai =  la[i]==0?1:0;
	      ii = ha[j]*NLAYERS*NXCRYS*NYCRYS + lai*NXCRYS*NYCRYS + k*NYCRYS;
	      x1 = m_crystal_xpos[ii];
	      y1 = m_crystal_ypos[ii];

	      for (l=0; l<NXCRYS; l++)
		{
		  lbi =  lb[i]==0?1:0;
		  jj = hb[j]*NLAYERS*NXCRYS*NYCRYS + lbi*NXCRYS*NYCRYS + l*NYCRYS;
		  x2 = m_crystal_xpos[jj];
		  y2 = m_crystal_ypos[jj];

		  // Crystal a transaxial position
		  xx1 = cos(primary[ha[j]])*x1 - sin(primary[ha[j]])*y1;
		  yy1 = sin(primary[ha[j]])*x1 + cos(primary[ha[j]])*y1;

		  // Crystal b transaxial position
		  xx2 = cos(primary[ha[j]])*x2 - sin(primary[ha[j]])*y2;
		  yy2 = sin(primary[ha[j]])*x2 + cos(primary[ha[j]])*y2;

		  theta1 = atan((xx2-xx1)/fabs((yy2-yy1)));
		  //				angle1 = (int)(0.5 + fabs(theta1*cvtrad));
		  angle1 = (int)(fabs(theta1*cvtrad));
		  *(cangle_elem(k,l,j,i,0)) = (short)(angle1);
		  if (angle1<NUM_ANGLES) *(gr_elem(k,l,j,i,0)) = gra[angle1][la[i]];
		  else *(gr_elem(k,l,j,i,0)) = gra[NUM_ANGLES-1][la[i]];
		  //rotate panel of d2 to top to use same equation
		  xx1 = cos(primary[hb[j]])*x1 - sin(primary[hb[j]])*y1;
		  yy1 = sin(primary[hb[j]])*x1 + cos(primary[hb[j]])*y1;
		  xx2 = cos(primary[hb[j]])*x2 - sin(primary[hb[j]])*y2;
		  yy2 = sin(primary[hb[j]])*x2 + cos(primary[hb[j]])*y2;
					
		  theta2 = atan((xx1-xx2)/fabs((yy2-yy1)));
		  //					angle2 = (int)(0.5+fabs((theta2*cvtrad));		
		  angle2 = (int)(fabs(theta2*cvtrad));	
		  *(cangle_elem(k,l,j,i,1)) = (short)(angle2);
		  if (angle2 < NUM_ANGLES) *(gr_elem(k,l,j,i,1)) = gra[angle2][lb[i]];
		  else *(gr_elem(k,l,j,i,1)) = gra[NUM_ANGLES-1][lb[i]];
		}
	    }
	}
    }

  // make ga
  /* IDL code
     gs = fltarr(72,104,104,5,4)	; CM consider all 5 opposite heads
     ;incident angle are the same for each crystal
     for i = 0,3 do begin		; layer-layer
     ; for j = 0,2 do begin	; consider 3 heads (mirror symmetry)
     for j = 0,4 do begin		; CM consider all 5 opposite heads
     for k = 0,71 do begin	; all tx in head b
     for l = 0,103 do begin	; all axial in head a
     for m = 0,103 do begin	; all axial in head b
     z1 = zd(36,la(i),ha(j),m)
     y1 = yd(36,la(i),ha(j),m)
     z2 = zd(k,lb(i),hb(j),l)
     y2 = yd(k,lb(i),hb(j),l)
     theta1 = atan((z2-z1)/abs(y2-y1))
     angle1 = fix((theta1*cvtrad+nr/2)/na)	; could be round?
     gs(k,l,m,j,i) = gr(angle1,la(i))	; CM add layer
     endfor
     endfor
     endfor
     endfor
     endfor
     openw,1,'ga_4_fansum_FB.dat'
     writeu,1,gs
     close,1
  */

  int cx = NXCRYS/2;  // center crystal x index
  int found=0;
  for (i=0; i<lc; i++)   // layer combinations
    {
      for (j=0; j<na; j++) // head in coincidences
	{
	  lai =  la[i]==0?1:0;
	  ii = ha[j]*NLAYERS*NXCRYS*NYCRYS + lai*NXCRYS*NYCRYS + cx*NYCRYS;
	  for (k=0; k<NXCRYS; k++)
	    {
	      lbi =  lb[i]==0?1:0;
	      jj = hb[j]*NLAYERS*NXCRYS*NYCRYS + lbi*NXCRYS*NYCRYS + k*NYCRYS;
	      for (l=0; l<NYCRYS; l++)
		{
		  z2 = m_crystal_zpos[jj+l];
		  y2 = m_crystal_ypos[jj+l];
		  for (m=0; m<NYCRYS; m++)
		    {
		      z1 = m_crystal_zpos[ii+m];
		      y1 = m_crystal_ypos[ii+m];
		      theta1 = atan((z2-z1)/fabs(y2-y1));
		      // angle1 = (int)(0.5+fabs(theta1*cvtrad));
		      angle1 = (int)(fabs(theta1*cvtrad));
		      if (angle1 < NUM_ANGLES) *(ga_elem(k,l,m,j,i)) = gra[angle1][la[i]];
		      else *(ga_elem(k,l,m,j,i)) = gra[NUM_ANGLES-1][la[i]];
		    }
		}
	    }
	}
    }

#ifdef DEBUG
  fp = fopen("gr_log.txt", "wt");
  for (i=0; i<NUM_ANGLES; i++)
    fprintf(fp,"%d %g %g\n", i,  gra[i][0],  gra[i][1]);
  fclose(fp);
#endif
  return 1;
}
