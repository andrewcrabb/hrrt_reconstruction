/*
 Module geo_compute
 Scope: HRRT only
 Author: Merence Sibomana
 Creation 08/2005
 Purpose: This module gets HRRT geomnetry coefficients and computes the geometric factor to be applied to LOR
          and before fan sum
 Modification History:
*/
#include <math.h>

static int ha[] = [0,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,3,4,4,5];
static int hb[] = [2,3,4,5,6,3,4,5,6,7,4,5,6,7,5,6,7,6,7,7];
static int la[] = [0,0,1,1];
static int lb[] = [0,1,0,1];
static double cvtrad = 180/M_PI;

static double *primary = NULL;
static int na=5; // heads in coincidences

static void local_params()
{
	if (primary==NULL)
	{
		primary = (double*)calloc(8, sizeof(double));
		for (int i=0; i<8; i++) primary[i] = i*M_PI/4;
	}
}

make_gs()
{
	int lc=4; // layer combinations
	int i,j,k,l,m;
	int ii, jj;
	double x1,y1,z1,x2,y2,z2;  //  crystal a and b coordinates before symmetry rotation
	double xx1,yy1,xx2,yy2;  //  crystal a and b actual coordinates
	double theta1, theta2;
	int angle1, angle2;

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
				ii = ha[j]*NLAYERS*NXCRYS*NYCRYS + la[i]*NXCRYS*NYCRYS + k*NYCRYS;
				x1 = crystal_xpos[ii];
				y1 = crystal_ypos[ii];

				for (l=0; l<NXCRYS; l++)
				{
					jj = hb[j]*NLAYERS*NXCRYS*NYCRYS + lb[i]*NXCRYS*NYCRYS + l*NYCRYS;
					x2 = crystal_xpos[jj];
					y2 = crystal_ypos[jj];

					// Crystal a transaxial position
					xx1 = cos(primary[ha[j]])*x1 - sin(primary[ha[j]])*y1;
					yy1 = sin(primary[ha[j]])*x1 + cos(primary[ha[j]])*y1;

					// Crystal b transaxial position
					xx2 = cos(primary[ha[j]])*x2 - sin(primary[ha[j]])*y2;
					yy2 = sin(primary[ha[j]])*x2 + cos(primary[ha[j]])*y2;

					theta1 = atan((xx2-xx1)/abs((yy2-yy1)));
					angle1 = (int)(0.5 + (theta1*cvtrad+nr/2)/na);
					*(gr_elem(k,l,j,i,0)) = gr(angle1,la(i));
					
					//rotate panel of d2 to top to use same equation
					xx1 = cos(primary[hb[j]])*x1 - sin(primary[hb[j]])*y1;
					yy1 = sin(primary[hb[j]])*x1 + cos(primary[hb[j]])*y1;
					xx2 = cos(primary[hb[j]])*x2 - sin(primary[hb[j]])*y2;
					yy2 = sin(primary[hb[j]])*x2 + cos(primary[hb[j]])*y2;
					
					theta2 = atan((xx1-xx2)/abs((yy2-yy1)));
					angle2 = (int)(0.5 + (theta2*cvtrad+nr/2)/na);		
					*(gr_elem(k,l,j,i,1)) = gr(angle2,lb(i));
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
	
	for (i=0; i<lc; i++)   // layer combinations
	{
		for (j=0; j<na; j++) // head in coincidences
		{
			ii = ha[j]*NLAYERS*NXCRYS*NYCRYS + la[i]*NXCRYS*NYCRYS + NXCRYS/2*NYCRYS;
			for (k=0; k<NXCRYS; k++)
			{
				x1 = crystal_xpos[ii];
				y1 = crystal_ypos[ii];

				jj = hb[j]*NLAYERS*NXCRYS*NYCRYS + lb[i]*NXCRYS*NYCRYS + k*NYCRYS;
				z2 = crystal_zpos[jj];
				y2 = crystal_ypos[jj];
				for (l=0; l<NYCRYS; l++)
				{
					for (m=0; m<NYCRYS; m++)
					{
						z1 = crystal_zpos[ii+m];
						y1 = crystal_ypos[ii+m];
						theta1 = atan((z2-z1)/abs(y2-y1));
						angle1 = (int)(0.5+(theta1*cvtrad+nr/2)/na);
						*(ga_elem(k,l,m,j,i)) = gr(angle1,la(i));
					}
				}
			}
		}
	}

}