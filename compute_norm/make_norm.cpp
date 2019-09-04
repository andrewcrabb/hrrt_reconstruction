/************************************************************************/
/* make_norm : IDL code                                                 */
/************************************************************************/
/*
;+
; NAME:
;	make_norm
;
; PURPOSE:
;
;	Uses the indexes from sino_to_crys to to compute a normalization to
;	best normalize the crystal efficiencies.
;
;
; CATEGORY:
;       Scanner calibration.
;
; CALLING SEQUENCE:
; 		normalization=make_norm(sinogram)
;
; INPUTS:
;		HRRT sinogram plane (nelems x nviews)
;
;
; KEYWORD PARAMETERS:
;
; OUTPUTS:
;		normalization - a nelems x nviews array that corrects a sinogram for crystal efficiencies
;
; HISTORY NOTES:
;	MEC 10/99 - Wrote original
;
;-
function make_norm,sino
;
	common make_norm,dwell,si
	common model, model_name,nelems,nviews,nrings,span,rd,nseg,$
		      bkts_per_ring,blks_per_bkt,crys_per_blk,crys_per_bkt,crys_per_ring,fan_size,$
		      det_dia,blk_pitch,axial_pitch,crys_pitch,crys_depth,dist_to_bkt,$
		      layers,weights,radial_comp,angle_comp,axial_comp,dwell_geom
	common ctrl, qc,white,black,red
	;
	;If the constant data is not defined then make it. The routine runs much faster if this exists. 
	;Type 4 (8) is float (structure)
	;
	if size(dwell,/type) ne 4 or size(si,/type) ne 8 then begin
		print, " make_norm: creating sinogram index and dwell sinogram"
		si=sino_to_crys()
		dwell = dwell_sino()
	endif
	;
	eff=fltarr(crys_per_ring)
	msk = fltarr(nelems,nviews)
	msk(56:nelems-56,*)=1.			; use only central FOV width is 1 buckect diamond
	;msk = fltarr(nelems,nviews)+1.		; Alternative would be to use full sinogram
	msk=msk*sino*dwell
	;
	;Get the crystal sensitivities
	;
	for i=0,crys_per_ring-1 do eff(i)=total(si(i).wts*msk(si(i).indx(0:si(i).npairs-1)))/total(si(i).wts)
	eff=eff*crys_per_ring/total(eff)	; normalize eff vector to crys_per_ring

if(qc) then plot,eff,xstyle=1,background=white,color=black

	if min(eff) lt 0.25 then begin
		print," make_norm: some efficiencies are lower than 0.25 are set to 0."
		eff(where(eff lt 0.25))= 0.
	endif

	ii=where(eff gt 0.)
	eff(ii)=eff(ii)^(-.5)			; 1/sqrt 
	;
	;Then apply them to the dwell to make a normalization.
	;
	msk=dwell
	for i=0,crys_per_ring-1 do msk(si(i).indx(0:si(i).npairs-1))=msk(si(i).indx(0:si(i).npairs-1))*eff(i)
	return,msk
	end
*/

/************************************************************************/
/* make_norm C Implementation :  Merence Sibomana 09/03                 */
/************************************************************************/
#include "norm_globals.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "my_spdlog.hpp"

int make_norm(const float *sino, float *norm, const float *dwell, SinoIndex *sino_index, float *eff) {
	int i=0,j=0, npixels=nviews*nelems;

	// Apply rebinning dwell correction: use only central FOV 
	// width is 1 buckect diamond (144 bins)
	for (int view=0; view<nviews; view++)
	{
		float *pnorm = norm + view*nelems;
		const float *psino = sino + view*nelems;
		const float *pdwell = dwell + view*nelems;
		for (int elem=56; elem<=(nelems-56); elem++)
			pnorm[elem] = psino[elem]*pdwell[elem];
	}
	// Get the crystal sensitivities
	double sum=0.0, sum1=0.0, sum2=0.0;
	int npairs=0, *idx=NULL;
	float *wts=NULL;
	for (i=0; i<crys_per_ring; i++)
	{
		sum1=sum2=0.0;
		npairs = sino_index[i].npairs;
		idx = sino_index[i].idx;
		wts = sino_index[i].wts;
		for (j=0; j<npairs; j++)
		{
			sum1 +=  wts[j]*norm[idx[j]];
			sum2 += wts[j];
		}
		eff[i] = (float)(sum1/sum2);
		sum += eff[i];
	}

	// normalize eff vector to crys_per_ring
	float fact = (float)(crys_per_ring/sum);
	float min_eff = eff[0]*fact;
	for (i=0; i<crys_per_ring; i++)
	{
		eff[i] *= fact;
		if (eff[i] < min_eff) min_eff = eff[i];
	}
	if (min_eff < 0.25) 
		LOG_INFO("make_norm: some efficiencies are lower than 0.25 are set to 0.\n");
	for (i=0; i<crys_per_ring; i++)
	{
		if (eff[i]<0.25f) eff[i] = 0.0f;
		else eff[i] = (float)(1/sqrt(eff[i]));
	}
	
	// Then apply them to the dwell to make a normalization.
	memcpy(norm, dwell, nviews*nelems*sizeof(float));
	unsigned char *mask = (unsigned char*)calloc(nviews*nelems, sizeof(unsigned char));
	for (i=0; i<crys_per_ring; i++)
	{
		npairs = sino_index[i].npairs;
		idx = sino_index[i].idx;
		for (j=0; j<npairs; j++)
		{
			int k = idx[j];
			if (mask[k] == 0) 
			{
				norm[k] *= eff[i];
				mask[k] = 1;
			}
		}
		memset(mask,0,nviews*nelems);
	}
	free(mask);
	return 1;
}
