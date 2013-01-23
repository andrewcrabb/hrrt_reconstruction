/************************************************************************/
/* det_to_ve                                                            */
/************************************************************************/
/*
;+
; NAME:
;	det_to_ve
;
; PURPOSE:
;	Returns the sinogram address (view and element) for a crystal pair
;	in the HRRT. The procedure is to find the exact angle and offset of
;	each LOR, then to position it in the sinogram bin closest to the
;	exact value.
;
; CATEGORY:
;	Scanner calibration.
;
; CALLING SEQUENCE:
; 	address = det_to_ve(det1,det1,doi1,doi2)
;
; INPUTS:
;	det1	- A pair of vectors containing the crystals comprising LOR's.
;	det2
;
;       doi1	- A pair of vectors containing the depth information for the
;	doi2	  crystal vectors above. 1 for patient layer, 0 for pmt layer.
;
; KEYWORD PARAMETERS:
;
; OUTPUTS:
;	address - a vector of sinogram addresses for a 256 x 288 sinogram.
;
; HISTORY NOTES:
;	MEC 2/99 - Wrote original
;
;-
*/

/************************************************************************/
/* det_to_cmplx IDL code                                                */
/************************************************************************/
/*
; det_to_cmplx converts a crystal number and layer bit to a vector pointing
; from the origin to the crystal. Complex numbers are used to simplify the
; calculations as idl implements them directly.
;
function det_to_cmplx,det,doi
	;
	common model, model_name,nelems,nviews,nrings,span,rd,nseg,$
		      bkts_per_ring,blks_per_bkt,crys_per_blk,crys_per_bkt,crys_per_ring,fan_size,$
		      det_dia,blk_pitch,axial_pitch,crys_pitch,crys_depth,dist_to_bkt,$
		      layers,weights,radial_comp,angle_comp,axial_comp,dwell_geom
	;
	; Find vector to crystal
	;
	bkt = det / crys_per_bkt
	blk = (det / crys_per_blk) mod (blks_per_bkt)
	crys = det mod crys_per_blk
	;
	;Vector to center of detector
	; x postive to right, y postive down, angle postive clock-wise
	;
	lyr = float(doi eq 0) ; Converts 1 for patient side to 0 for patient side
	v1 = (dist_to_bkt+crys_depth*(lyr+0.5))*(complex(1.,1.)/sqrt(2))^bkt*complex(0,-1)
	v2 = (blk - float(blks_per_bkt-1)/2.)*blk_pitch
	v2 = v2 + (crys - float(crys_per_blk -1)/2.)*crys_pitch
	v2 = v2 * (complex(1.,1.)/sqrt(2))^bkt
	v1 = v1 + v2
	return, v1
	end
*/

/************************************************************************/
/* det_to_cmplx C++ implementation :  Merence Sibomana 09/03             */
/************************************************************************/
#define SQRT_2 0.707107f
#define M_PI 3.14159265358979323846
#include <complex>
#include <vector>
#include <math.h>
#include "norm_globals.h"

using namespace std;

static float quad_real[8] = {1.0f, SQRT_2, 0.0f, -SQRT_2, -1.0f, -SQRT_2, 0.0f, SQRT_2};
static float quad_imag[8] = {0.0f, SQRT_2, 1.0f, SQRT_2, 0.0f, -SQRT_2, -1.0f, -SQRT_2};
void det_to_cmplx(int det, int doi, complex<float> &v, float crystal_depth)
{
	int bkt = det / crys_per_bkt;
	int blk = (det / crys_per_blk) % (blks_per_bkt);
	int crys = det % crys_per_blk;
	float lyr = doi==0? 1.0f : 0.0f;
	complex<float> c1(quad_real[bkt], quad_imag[bkt]);
	complex<float> c2(0,-1);
	complex<float> c3 = c1*c2;
	float fact = (dist_to_bkt+crystal_depth*(lyr+0.5f));
	v =  fact * c3;
	fact = (blk - 0.5f*(blks_per_bkt-1))*blk_pitch;
	fact = fact + (crys - 0.5f*(crys_per_blk -1))*crys_pitch;
	c2 = fact * c1;
	v = v+c2;
}

//
//	Function det_to_ve returns indexes to the sinogram bins for the LOR's described
//	by the two vectors det1 and det2. Vectors doi1 and doi2 represent the depth layer.
//	1 for patient layer, 0 for pmt layer.
//

/************************************************************************/
/* det_to_ve IDL code                                                   */
/************************************************************************/
/*
 function det_to_ve,det1,det2,doi1,doi2
	;
	common model, model_name,nelems,nviews,nrings,span,rd,nseg,$
		      bkts_per_ring,blks_per_bkt,crys_per_blk,crys_per_bkt,crys_per_ring,fan_size,$
		      det_dia,blk_pitch,axial_pitch,crys_pitch,crys_depth,dist_to_bkt,$
		      layers,weights,radial_comp,angle_comp,axial_comp
	;
	; Find complex vector to detector
	;
	p1 = det_to_cmplx(det1,doi1)
	p2 = det_to_cmplx(det2,doi2)
	;
	;Find line from p2 to p1
	;
	lor = p1-p2
	;
	;Find vector from origin and perpindicular to lor
	;
	ii = where(abs(lor) ne 0)
	lor(ii) = lor(ii)*complex(0,1)/abs(lor(ii)) 	;Find unit vector orthogonal to LOR
	offs = float(p1*conj(lor)) 			;vector orthogonal to p1 from origin to LOR
	;
	;Find angle with respect to origin.
	;
	angle = atan(imaginary(lor),float(lor))
	;
	;Convert to sinogram addresses : uses tomographic (crystal) pitch from rebinner ?
	;
	view = round(angle*nviews/!pi)
	elem = round(2.*offs/crys_pitch)		; binsize is crys_pitch/2.
	indx = where(view lt 0 or view eq nviews ,n)
	if n gt 0 then begin
		elem(indx)=-elem(indx)
		view(indx)=view(indx)+nviews
	endif
	;
	;Return indexes to valid sinogram bins
	;
	elem=elem+nelems/2
	view=view mod nviews
	indx = where(elem ge 0 and elem le nelems-1)
	return, elem(indx)+nelems*view(indx)
end
*/
inline int my_round(float f) 
{
	return (f>0.0f? (int)(0.5 + f) : (int)(-0.5+f));
}

int det_to_ve(const complex<float> &v1,  const complex<float> &v2)
{
	 complex<float> c01(0.0f, 1.0f),lor;
	 float angle=0.0f, offs=0.0f, dview = nviews/M_PI, delem = 2.0f/crys_pitch;
	 int view=0, elem=0, mid_elem = nelems/2;
	 lor = v1-v2;
	 float r = abs(lor);
	 if (r > 0.0f) lor = lor*c01/r;
	 offs = real(v1*conj(lor));
	 angle = (float)atan2(imag(lor), real(lor));
	 view = my_round(angle*dview);  //binsize is crys_pitch/2.
	 elem = my_round(offs*delem);  //binsize is crys_pitch/2.
	 if (view<0 || view >= nviews)
	 {
		 elem = -elem;
		 view = view + nviews;
	 }
	//
	// Return indexes to valid sinogram bins
	//
	 elem += mid_elem;
	 view = view%nviews;
	 if (elem>=0 && elem<nelems)
		 return (elem+nelems*view);
	 return -1;
}
