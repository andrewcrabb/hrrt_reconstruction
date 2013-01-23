/************************************************************************/
/* sino_to_crys : IDL code                                              */
/************************************************************************/
/*
;+
; NAME:
;	sino_to_crys
;
; PURPOSE:
;	Computes indexes for each crystal to each LOR.
;
; CATEGORY:
;       Scanner calibration.
;
; CALLING SEQUENCE:
; 	stc = sino_to_crys()
;
; INPUTS:
;		none
;
;
; KEYWORD PARAMETERS:
;
; OUTPUTS:
;		stc - An crys_per_ring element array of structures containing pointers to each lor
;
; HISTORY NOTES:
;	MEC 10/99 - Wrote original
;
;-
function sino_to_crys
;
	common model, model_name,nelems,nviews,nrings,span,rd,nseg,$
		      bkts_per_ring,blks_per_bkt,crys_per_blk,crys_per_bkt,crys_per_ring,fan_size,$
		      det_dia,blk_pitch,axial_pitch,crys_pitch,crys_depth,dist_to_bkt,$
		      layers,weights,radial_comp,angle_comp,axial_comp,dwell_geom
;
	sino_index = replicate({npairs:0.,indx:lonarr(1024),wts:fltarr(1024)},crys_per_ring)
	;
	;This vector defines the number of layers per head. 1 for double layer, 0 for single
	;
 	for ca = 0,crys_per_ring-1 do begin
		idx = lonarr(1)
		wts = fltarr(1)
		crys_a = replicate(ca,fan_size)
		crys_b = (indgen(fan_size) + ca + 2*crys_per_bkt) mod crys_per_ring
		for d1 = 0, layers(ca / crys_per_bkt) do begin
			doi_a = (1 - d1) * layers(ca / crys_per_bkt)
			for d2 = 0, 1 do begin
				doi_b = replicate(1 - d2,fan_size) * layers(crys_b/crys_per_bkt)
				ii = where( (layers(crys_b/crys_per_bkt) eq 1) $
					or (layers(crys_b/crys_per_bkt) eq 0 and d2 eq 0))

				jj = det_to_ve(crys_a(ii),crys_b(ii),doi_a,doi_b(ii))
				idx=[idx,jj]
				wts=[wts,replicate(weights(d2),size(jj,/n_elements))]
			endfor
		endfor
		sino_index(ca).npairs=size(idx,/n_elements)-1
		sino_index(ca).indx = idx(1:*)
		sino_index(ca).wts = wts(1:*)
	endfor
	return,sino_index
	end

*/

/************************************************************************/
/* sino_to_crys C Implementation :  Merence Sibomana 09/03              */
/************************************************************************/

#include "norm_globals.h"
#include <stdio.h>
#include "det_to_ve.h"
using namespace std;

static short layers[] = {1,1,1,1,1,1,1,1};        // All heads have 2 layers
void sino_to_crys(SinoIndex *sino_index)
{
	int i=0;
	int ca=0, cb=0, d1=0, d2=0, doi_a=0, doi_b=0;
	complex<float> v1, v2;
	for (ca=0; ca<crys_per_ring; ca++)
	{
		int npairs = 0;
		int *idx = sino_index[ca].idx;
		float *wts = sino_index[ca].wts;
		int head_a = ca/crys_per_bkt;
		for (d1=0; d1<=layers[head_a]; d1++)
		{
			doi_a = (1 - d1) * layers[head_a];
			det_to_cmplx(ca, doi_a, v1, crys_depth[head_a]);
			for (d2=0; d2<=1; d2++)
			{
				for (i=0; i<fan_size; i++)
				{
					cb = (i +  ca + 2*crys_per_bkt)%crys_per_ring;
					int head_b = cb/crys_per_bkt;
					doi_b = (1-d2)* layers[head_b];
					if ((layers[head_b] == 1) || (layers[head_b]==0 && d2==0))
					{
						det_to_cmplx(cb, doi_b, v2, crys_depth[head_b]);
						int pos = det_to_ve(v1, v2);
						if (pos>=0)
						{
							idx[npairs] = pos;
							wts[npairs] = weights[d2];
							npairs++;
						}

					}
				}
			}
		}
		sino_index[ca].npairs = npairs;
	}

}
