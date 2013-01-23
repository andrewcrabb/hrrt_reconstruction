/************************************************************************/
/* self_norm : IDL Code                                                 */
/************************************************************************/
/*
; NAME:
;	self_norm
;
; PURPOSE:
;	Corrects for sensitivity variations in HRRT detector heads on a
;	diamond by diamond basis.
;
; CATEGORY:
;       Scanner calibration.
;
; CALLING SEQUENCE:
; 	self_norm,sinogram
;
; INPUTS:
;	sinogram - a 2 or 3 dimensional matrix containing the sinogram stored
;		   WARNING - DATA WILL BE MODIFIED!!
;
;
; KEYWORD PARAMETERS:
;
; OUTPUTS:
;	sinogram - the normalized sinogram
;
; HISTORY NOTES:
;	MEC 2/99 - Wrote original
;-

pro self_norm,sino
	common ctrl, qc
;
	ssize = size(sino,/dimensions)
	nplns = ssize(2)
	if(qc) then begin 
		window,/free,xsize=768,ysize=768,title='Crystal efficiencies after self-normalization'	
		!p.multi=[0,5,5]
	endif
	for pln=0,nplns -1 do begin
		print,' Self-normalizing plane ',pln
		sino(*,*,pln)=sino(*,*,pln)*make_norm(sino(*,*,pln))
	endfor
	return
	end
*/

/************************************************************************/
/* self_norm C Implementation :  Merence Sibomana 09/03                 */
/************************************************************************/
#include "norm_globals.h"
#include <string.h>
#include <sys/malloc.h>
#include <stdlib.h>
#include <stdio.h>

int self_norm(float *vsino, int nplns)
{
	int ret=1;
	int npixels = nelems*nviews;
	float *dwell = (float*)calloc(npixels, sizeof(float));
	if (!dwell_sino(dwell, npixels))
	{
		free(dwell);
		return 0;
	}
	SinoIndex	*sino_index = (SinoIndex*)calloc(crys_per_ring, sizeof(SinoIndex)); 
	float *psino = vsino;
	float *norm = (float*)calloc(npixels, sizeof(float));
	float *eff = (float*)calloc(crys_per_ring*nplns, sizeof(float));
	float *pn = norm;
	sino_to_crys(sino_index);
	for (int pln=0; pln<nplns; pln++, psino += npixels)
	{
		if (qc_flag) printf("Self-normalizing plane %d\n",pln+1);
		if (make_norm(psino, norm, dwell, sino_index, eff+crys_per_ring*pln))
		{
			for (int i=0; i<npixels; i++) psino[i] *= norm[i];
			memset(norm, 0, npixels*sizeof(float)); // reset for next plane
		}
		else
		{
			ret = 0;
			break;
		}
	}
	if(qc_flag) 
		plot("crys_eff.dat", eff, nplns, crys_per_ring,  "Crystal Efficiencies");
	free(sino_index);
	free(dwell);
	free(eff);
	free(norm);
	return ret;
}
