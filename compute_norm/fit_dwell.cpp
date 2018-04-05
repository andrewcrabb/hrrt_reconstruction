/*[ fit_dwell.cpp
------------------------------------------------

   Name            : fit_dwell.cpp - C++ implementation of original IDL fit_dwell utility
                   : described below.
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 09/25/2003

   Description     :  see below
               Copyright (C) CPS Innovations 2003 All Rights Reserved.

---------------------------------------------------------------------*
//*
;+
; NAME:
;	fit_dwell
;
; PURPOSE:
;	This program fits the dwell function to a 3D sinogram. The dwell function comes from
;	the trajectory of a rod source enscribing a circular path in the field of view. In a plane,
;	the dwell function is A0/sqrt(R^2 - (x-t*cos(angle - phi))^2). A0 is the intensity, R
;	is the radius of rotation and t*cos(angle - phi) is the angle dependant offset from the
;	center of the scanner.
;
; CATEGORY:
;	Scanner calibration.
;
; CALLING SEQUENCE:
; 	parms = fit_dwell(sinogram)
;
; INPUTS:
;	sinogram - 	a 3 dimensional float matrix containing the sinogram. This sinogram
;				should be partially normalized by the self_norm proceedure.
;
; KEYWORD PARAMETERS:
;
; OUTPUTS:
;	parms - 	a four by nplanes array containing the parameters of the fit for each plane.
;				parms = [A0,R,t,phi] (see description above)
;
; HISTORY NOTES:
;	MEC 2/99 - Wrote original
;
;-
;
;Fits each angle of the sinogram, then forces the offsets to conform to t*cos(theta-phi)
;
*/

#include "gapfill.h"
#include "polyfitw.h"
#include "norm_globals.h"
#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static  void sino_rebin(float *sino, float *rsino, int nelemsr, int nviewsr)
{
	int i=0, j=0, k=0;
	float *src = sino, *dest = rsino;
	for (i=0; i<nviews;  i++)
	{
		dest = rsino + (i/angle_comp)*nelemsr;
		for (j=0; j<nelems; j += radial_comp)
		{
			for (k=0;k<radial_comp; k++, src++) *dest += *src;
			dest++;
		}
	}
	int npixels = nelemsr*nviewsr;
	float w = 1.0f/(angle_comp*radial_comp);
	for (i=0; i<npixels;  i++)	rsino[i] *= w;
}

static  void axial_rebin(short *sino, float *rsino, int nplns)
{
	int pl=0, i=0,npixels = nelems*nviews;
	short *src = sino;
	float *dest = rsino;
	for (pl=0; pl<nplns;  pl++)
	{
		dest = rsino + (pl/axial_comp)*npixels;
		for (i=0; i<npixels;  i++)	*dest++ += *src++;
	}
	int nvoxels = npixels*(nplns/axial_comp);
	for (i=0; i<nvoxels;  i++)	rsino[i] /= axial_comp;
}

static void fit_sino_dwell(float *sino, double *pret,
						   float *qc_profile, float *qc_profile_fit)
{

	int nelemsr = nelems/radial_comp;
	int nviewsr = nviews/angle_comp;
	float psize   = crys_pitch/2.0f;
	//
	//These restrict the fit to reasonable values
	//
	float max_rod_rad = (175.0f/psize)/radial_comp;
	float max_offset = (10.0f/psize)/radial_comp;

	float coef[3];
	double a=0, r=0, t=0;
	double asum=0.0, rsum=0.0, usum=0.0, vsum=0.0;
	//
	// Get a good estimate of the average profile of all angles.
	//
	// IDL: mask = rebin(float(sino gt 0),nelemsr,nviewsr)
	// IDL: rsino = rebin(sino,nelemsr,nviewsr)
	// IDL: x=findgen(nelemsr)
	// IDL: xc=x-nelemsr/2		; centered radial rebinned coordinate
	// IDL: theta=findgen(nviewsr)*!pi/nviewsr

	int mid_nelemsr = nelemsr/2;
	int i=0, npixels = nelemsr*nviewsr;
	float *rsino = (float*)calloc(npixels, sizeof(float));
	sino_rebin(sino, rsino, nelemsr, nviewsr);

	//
	// Find the fit to each view
	// If curvefit fails to converge, it defaults to the average fit
	//
	float *x = (float*)calloc(nelemsr, sizeof(float));
	for (i=0; i<nelemsr; i++) x[i] = (float)i;
	float *profile2 = (float*)calloc(nelemsr, sizeof(float));
	float *inv_profile2 = (float*)calloc(nelemsr, sizeof(float));

	for (int view = 0; view<nviewsr; view++)
	{
		// IDL: profile = rsino(*,view)/(mask(*,view)>1)	
		// IDL: c = polyfitw(x,1/(profile^2>1),profile^2,2)
		// IDL: a = 1/sqrt(-c(2))
		// IDL: t = ((0.5*a^2*c(1)) > (nelemsr/2-max_offset)) < (nelemsr/2+max_offset)
		// IDL: r = (sqrt(c(0)*a^2+t^2) > (t+.1)) < max_rod_rad

		float *profile = rsino +view*nelemsr; 
		// mask division ignored: mask>1 is a all 1 matrix since mask is a binary matrix
		for (i=0; i<nelemsr; i++) 
		{
			profile2[i] = profile[i]*profile[i];
			if (profile2[i]>1) inv_profile2[i] = 1.0f/profile2[i];
			else inv_profile2[i] = 1.0f;
		}

		polyfitw(x,inv_profile2,profile2,nelemsr,2,coef);
		a = 1/sqrt(-coef[2]);
		t = 0.5*a*a*coef[1];
		if (t < (nelemsr/2 -max_offset)) t = nelemsr/2 - max_offset;
		else if (t > (nelemsr/2 + max_offset)) t = nelemsr/2 + max_offset;
		r = sqrt(coef[0]*a*a + t*t);
		if (r < (t+0.1f)) r = t+0.1f;
		else if (r > max_rod_rad) r = max_rod_rad;


		// IDL: if( view eq 0) then begin
		// IDL:		plot,x,profile,xstyle=1,background=white,color=black
		// IDL: 	oplot,x,sqrt((1./poly(x,c))>1.),color=red
		// IDL: endif

		if (view == 0 && qc_profile!=NULL && qc_profile_fit!=NULL)
		{
			memcpy(qc_profile, profile, nelemsr*sizeof(float));
			for (i=0; i<nelemsr; i++) 
			{
				qc_profile_fit[i] = 1.0f/(coef[0] + coef[1]*x[i] + coef[2]*x[i]*x[i]);
				if (qc_profile_fit[i]<1.0f) qc_profile_fit[i] = 1.0f;
				qc_profile_fit[i] = (float)sqrt(qc_profile_fit[i]);
			}
			// Set first and last bin to 1 if too high
			i=0;
			if (qc_profile_fit[i]>1.0  && 
				qc_profile_fit[i]/qc_profile_fit[i+1] > 2*profile[i]/profile[i+1])
				qc_profile_fit[i] = 1.0;
			i=nelemsr-1;
			if (qc_profile_fit[i]>1.0  && 
				qc_profile_fit[i]/qc_profile_fit[i-1] > 2*profile[i]/profile[i-1])
				qc_profile_fit[i] = 1.0;
		}

		// IDL: parms(*,view) = [a,r,t]
		asum += a;
		rsum += r;
		double theta = view*M_PI/nviewsr;
		usum += (t-mid_nelemsr)*cos(theta);
		vsum += (t-mid_nelemsr)*sin(theta);
	}
	free(profile2);
	free(inv_profile2);
	free(rsino);
	
	//
	// Assume constant rod strength and constant radius over the plane
	//
	// IDL: a0 = total(parms(0,*))*radial_comp/nviewsr
	// IDL: r  = ((total(parms(1,*))/nviewsr) < max_rod_rad)*radial_comp

	a = asum*radial_comp/nviewsr;
	r = rsum/nviewsr;
	if (r < max_rod_rad) r = r*radial_comp;
	else r = max_rod_rad*radial_comp;
	
	//
	// Fit the offset to u*cos(phi)+v*sin(phi) offset to center of sinogram.
	//
	// IDL: u=total((parms(2,*)-nelemsr/2)*cos(theta))*2/nviewsr
	// IDL: v=total((parms(2,*)-nelemsr/2)*sin(theta))*2/nviewsr
	// IDL: t = (sqrt(u^2+v^2) < max_offset) * radial_comp
	// IDL: phi = atan(v,u)
	// IDL: return,[a0,r,t,phi]

	double u = usum*2/nviewsr;
	double v = vsum*2/nviewsr;
	t = sqrt(u*u + v*v);
	if (t<max_offset) t = t*radial_comp;
	else t = max_offset*radial_comp;
	double phi = atan2(v,u);
	pret[0] = a;
	pret[1] = r;
	pret[2] = t;
	pret[3] = phi;

}

//
// Fit for all planes in the sinogram
//
static int oplot(const char *filename, double *parms1, double *parms2, int nplns,
				  int offset, const char *title)
{
	double *p1=parms1+offset, *p2=parms2+offset;
	FILE *fp = fopen(filename,"wb");
	if (fp == NULL) 
	{
		fprintf(stderr,"%s : error creating file\n", filename);
		return 0;
	}
	fprintf(fp,"# %s\n", title);
	for (int pln=0; pln<nplns; pln++)
	{
		fprintf(fp,"%lg\t%lg\n", *p1, *p2);
		p1 += 4; p2 += 4;
	}
	fclose(fp);
	return 1;
}

float *fit_dwell(short *vsino, int full_nplns)
{

	// IDL: ssize = size(sino,/dimensions)
	// IDL: nplns = ssize(2)/axial_comp
	// IDL: parms = fltarr(4,nplns)
	int pln=0, nplns = full_nplns/axial_comp;
	
	//
	// First we'll compress in the axial direction then take out
	// the bucket dependent sensitivities.
	//
	if (qc_flag) printf("Rebinning sinogram in z direction from %d to %d\n", full_nplns, nplns);

	// IDL: c_sino = rebin(sino,nelems,nviews,nplns)
	// IDL: wts = total(total(c_sino,1),1)		; rather then weigths (nplns values)
	int i=0,j=0, npixels = nelems*nviews;
	float *sino=0;
	float *c_sino = (float*)calloc(nplns*npixels, sizeof(float));
	float *wts = (float*)calloc(nplns, sizeof(float));
	axial_rebin(vsino, c_sino, full_nplns);
	for (pln=0; pln<nplns; pln++)
	{
		sino = c_sino + pln*npixels;
		for (i=0; i<npixels; i++) wts[pln] += sino[i];
	}

	//	
	// Self-normalize plane: remove most of known effect i.e.  
	// crystal efficiencies and geometric dwell to parallel beam rebinning
	// to be able to see (i.e. fit) the rod dwell function
	//

	if (!self_norm(c_sino, nplns))
	{
		free(c_sino);
		free(wts);
		return NULL;
	}

	//
	// Then we fit the dwell function to each plane
	//
	// if(qc) then begin 
	//	  window,/free,xsize=768,ysize=768,title='HRRT fit rod dwell'	
	//	  !p.multi=[0,5,5]
	//	  print,' Now fit rod dwell (transform rotating rod into a plane source) ... '
	//  endif
	//for pln=0, nplns-1 do begin
	//	tmp = c_sino(*,*,pln)
	//	gap_interp,tmp,tmp gt 0,indxx
	//	parms(*,pln) = fit_sino_dwell(tmp)
	//	if(qc) then print,' Plane =',pln,' a0 =',parms(0,pln),' r =',parms(1,pln),' t =',parms(2,pln),' phi =',parms(3,pln)
	//endfor

	//
	// average all plans to get gap template
	//
	float *gap_template = (float*)calloc(npixels, sizeof(float));
	for (pln=0; pln<nplns; pln++)
	{
		sino = c_sino + npixels*pln;
		for (i=0; i<npixels; i++)
			gap_template[i] += sino[i];
	}
	for (i=0; i<npixels; i++)
		gap_template[i] = (float)(floor(gap_template[i]/nplns));

	GapFill gp(nelems,nviews,0.0f);
	sino = c_sino;
	double *vparms = (double*)calloc(4*nplns, sizeof(double));
	double *parms = vparms;
	int nelemsr = nelems/radial_comp;
	float *profile = NULL;
	float *profile_fit = NULL;
	if (qc_flag) 
	{
		profile = (float*)calloc(nelemsr*nplns, sizeof(float));
		profile_fit = (float*)calloc(nelemsr*nplns, sizeof(float));
	}
	for (pln=0; pln<nplns; pln++)
	{
		gp.calcGapFill(gap_template,sino);
		if (qc_flag)
		{
			fit_sino_dwell(sino, parms, profile+pln*nelemsr, profile_fit+pln*nelemsr);
			printf("Plane =%d a0 =%g r =%g t=%g phi =%g\n",
			pln,parms[0], parms[1], parms[2], parms[3]);
		}
		else fit_sino_dwell(sino, parms, NULL,NULL);
		sino += npixels;
		parms += 4;
	}
	free(gap_template);
	if (qc_flag)
	{
		plot("fit_sino_dwell.dat", profile, profile_fit, nplns, nelemsr, "Sino dwell profile and fit");
		free(profile); free(profile_fit);
	}

	/* IDL
	!p.multi=[0,1,1]
	;
	fparms=parms		; a copy
	z=findgen(nplns)
	;
	;Assume the rod is uniform activity along it's length
	;
	fparms(0,*) = polyfitw(z,parms(0,*),wts,0)
	;
	;Assume the rod is straight. The radius should be a straight line.
	;
	parms(1,*) = poly(z,polyfitw(z,parms(1,*),wts,1))
	;
	;Assume that the center of rotation is a straight line
	;Straighten it in x then in y
	;
	tx=parms(2,*)*cos(parms(3,*))
	ty=parms(2,*)*sin(parms(3,*))
	ftx = poly(z,polyfitw(z,tx,wts,1))
	fty = poly(z,polyfitw(z,ty,wts,1))
	fparms(2,*) = sqrt(ftx^2+fty^2)
	fparms(3,*) = atan(fty,ftx)
	*/
	float *xv = (float*)calloc(nplns, sizeof(float));
	float *yv = (float*)calloc(nplns, sizeof(float));
	float *ftx = (float*)calloc(nplns, sizeof(float));
	float *fty = (float*)calloc(nplns, sizeof(float));
	double *fparms = (double*)calloc(4*nplns, sizeof(double));
	float coef[3];
	for (pln=0,parms = vparms; pln<nplns; pln++, parms += 4) 
	{
		xv[pln] = (float)pln;
		yv[pln] = (float)parms[0];
	}
	polyfitw(xv,yv,wts,nplns,0,coef);
	for (pln=0, parms = fparms; pln<nplns; pln++, parms += 4) 
		parms[0] = coef[0];
	for (pln=0,parms = vparms; pln<nplns; pln++, parms += 4)
		yv[pln] = (float)parms[1];
	polyfitw(xv,yv,wts,nplns,1,coef);
	for (pln=0, parms = fparms; pln<nplns; pln++, parms += 4) 
		parms[1] = coef[0] + coef[1]*pln;

	for (pln=0,parms = vparms; pln<nplns; pln++, parms += 4) 
		yv[pln] = (float)(parms[2]*cos(parms[3]));
	polyfitw(xv,yv,wts,nplns,1,coef);
	for (pln=0; pln<nplns; pln++) 
		ftx[pln] = coef[0] + coef[1]*pln;

	for (pln=0,parms = vparms; pln<nplns; pln++, parms += 4) 
		yv[pln] = (float)(parms[2]*sin(parms[3]));
	polyfitw(xv,yv,wts,nplns,1,coef);
	for (pln=0; pln<nplns; pln++) 
		fty[pln] = coef[0] + coef[1]*pln;

	for (pln=0, parms = fparms; pln<nplns; pln++, parms += 4) 
	{
		double fx = ftx[pln], fy = fty[pln];
		parms[2] = sqrt(fx*fx + fy*fy);
		parms[3] = atan2(fy,fx);
	}

	free(xv); free(yv);
	free(ftx); free(fty);
	
	/* IDL
	;
	; QC display
	;
	if(qc) then begin
		white='ffffff'XL
		black='000000'XL
		red='0000FF'XL
		window,/free,xsize=512,ysize=512,title='HRRT fit dwell'	
		!p.multi=[0,2,2]
		plot,z,parms(0,*),xstyle=1,xtitle='plane',title='Source activity',background=white,color=black
		oplot,z,fparms(0,*),color=red
		plot,z,parms(1,*),xstyle=1,xtitle='plane',title='Radius',background=white,color=black
		oplot,z,fparms(1,*),color=red
		plot,z,parms(2,*),xstyle=1,xtitle='plane',title='Offset amplitude',background=white,color=black
		oplot,z,fparms(2,*),color=red
		plot,z,parms(3,*),xstyle=1,xtitle='plane',title='Offset direction',background=white,color=black
		oplot,z,fparms(3,*),color=red
		!p.multi=[0,1,1]
		print,' Fitted dwell parameters ..'
		print,' Activity - Radius - Offset amplitude - Offset direction'
		print,fparms
	end
	;
	*/
	if (qc_flag)
	{
		oplot("activity.dat", vparms, fparms, nplns, 0, "Source activity");
		oplot("radius.dat", vparms, fparms, nplns, 1, "Source radius");
		oplot("offset_amplitude.dat", vparms, fparms, nplns, 2, "Offset amplitude");
		oplot("offset_direction.dat", vparms, fparms, nplns, 3, "Offset direction");
	}
	free(c_sino); free(wts); free(vparms);
	
	// IDL: return,rebin(fparms,4,nplns*axial_comp)
	// end
	float *ret = (float*)calloc(4*nplns*axial_comp, sizeof(float));
	parms = fparms;
	float *dest = ret;
	if (qc_flag) 
	{
		printf("Fitted dwell parameters ..\n");
		printf("Activity     - Radius - Off amplitude - Off direction\n");
	}

	for (pln=0; pln<nplns; pln++)
	{
		for (j=0; j<4; j++) dest[j] = (float)parms[j];
		if (qc_flag) 
		{
			for (int j=0; j<4; j++) printf("%10.4f ", dest[j]);
			printf("\n");
		}
		for (i=1; i<axial_comp; i++)
			memcpy(dest+4*i, dest, 4*sizeof(float));
		dest += 4*axial_comp;
		parms += 4;
	}
	free(fparms);
	return ret;
}

