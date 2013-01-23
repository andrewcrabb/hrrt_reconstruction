#include "ecat2air.h"

bool_t xdr_key_info(xdrs, objp)
XDR *xdrs;
struct key_info *objp;
{
	if (!xdr_int(xdrs,&objp->bits) )return FALSE;
	if (!xdr_int(xdrs,&objp->x_dim) )return FALSE;
	if (!xdr_int(xdrs,&objp->y_dim) )return FALSE;
	if (!xdr_int(xdrs,&objp->z_dim) )return FALSE;
	if (!xdr_double(xdrs,&objp->x_size) )return FALSE;
	if (!xdr_double(xdrs,&objp->y_size) )return FALSE;
	if (!xdr_double(xdrs,&objp->z_size) )return FALSE;
	return TRUE;
}

bool_t xdr_air(xdrs, objp)
XDR *xdrs;
struct air16 *objp;
{
	int i,j;
	union { u_short s[2]; u_int u; } tmp;

	for (i=0; i<4; i++)
		for (j=0; j<4; j++)
			if (!xdr_double(xdrs,&objp->e[i][j])) return FALSE;
	if (!xdr_opaque(xdrs,objp->s_file,sizeof(objp->s_file))) return FALSE;
	if (!xdr_key_info(xdrs,&objp->s)) return FALSE;
	if (!xdr_opaque(xdrs,objp->r_file,sizeof(objp->r_file))) return FALSE;
	if (!xdr_key_info(xdrs,&objp->r)) return FALSE;
	if (!xdr_opaque(xdrs,objp->comment,sizeof(objp->comment))) return FALSE;
	if (!xdr_u_int(xdrs,&objp->s_hash) )return FALSE;
	if (!xdr_u_int(xdrs,&objp->r_hash) )return FALSE;
	tmp.u = 0;
	if (xdrs->x_op == XDR_ENCODE) {	
		tmp.s[0] = objp->s_volume;
		tmp.s[1] =  objp->r_volume;
		if (!xdr_u_int(xdrs,&tmp.u) )return FALSE;
	} else {
		if (!xdr_u_int(xdrs,&tmp.u) )return FALSE;
		objp->s_volume = tmp.s[0];
		objp->r_volume = tmp.s[1];
	}
	if (!xdr_opaque(xdrs,objp->reserved,sizeof(objp->reserved))) return FALSE;
	return TRUE;
}
