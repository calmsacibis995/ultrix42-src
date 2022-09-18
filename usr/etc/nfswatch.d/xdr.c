#ifndef lint
static char *sccsid = "@(#)xdr.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/xdr.c,v 3.0 91/01/23 08:23:39 davy Exp $";
 */

/*
 * xdr.c - XDR routines for decoding NFS packets.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	xdr.c,v $
 * Revision 3.0  91/01/23  08:23:39  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.2  90/08/17  15:47:53  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:21:08  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <errno.h>
#include <stdio.h>

#define NFSSERVER	1

#ifdef sun
#include <sys/vfs.h>
#endif /* sun */
#ifdef ultrix
#include <sys/types.h>
#include <sys/time.h>
#endif /* ultrix */
#include <nfs/nfs.h>

#include "nfswatch.h"

bool_t
xdr_creatargs(xdrs, argp)
register struct nfscreatargs *argp;
register XDR *xdrs;
{
	if (xdr_diropargs(xdrs, &argp->ca_da) &&
	    xdr_sattr(xdrs, &argp->ca_sa))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_diropargs(xdrs, argp)
register struct nfsdiropargs *argp;
register XDR *xdrs;
{
	if (xdr_fhandle(xdrs, &argp->da_fhandle) &&
	    xdr_string(xdrs, &argp->da_name, NFS_MAXNAMLEN)) {
		free(argp->da_name);
		return(TRUE);
	}

	return(FALSE);
}

bool_t
xdr_fhandle(xdrs, argp)
fhandle_t *argp;
register XDR *xdrs;
{
	if (xdr_opaque(xdrs, (caddr_t) argp, NFS_FHSIZE))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_linkargs(xdrs, argp)
register struct nfslinkargs *argp;
register XDR *xdrs;
{
	if (xdr_fhandle(xdrs, &argp->la_from) &&
	    xdr_diropargs(xdrs, &argp->la_to))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_rddirargs(xdrs, argp)
register struct nfsrddirargs *argp;
register XDR *xdrs;
{
	if (xdr_fhandle(xdrs, &argp->rda_fh) &&
	    xdr_u_long(xdrs, &argp->rda_offset) &&
	    xdr_u_long(xdrs, &argp->rda_count))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_readargs(xdrs, argp)
register struct nfsreadargs *argp;
register XDR *xdrs;
{
	if (xdr_fhandle(xdrs, &argp->ra_fhandle) &&
	    xdr_long(xdrs, (long *) &argp->ra_offset) &&
	    xdr_long(xdrs, (long *) &argp->ra_count) &&
	    xdr_long(xdrs, (long *) &argp->ra_totcount))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_rnmargs(xdrs, argp)
register struct nfsrnmargs *argp;
register XDR *xdrs;
{
	if (xdr_diropargs(xdrs, &argp->rna_from) &&
	    xdr_diropargs(xdrs, &argp->rna_to))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_saargs(xdrs, argp)
register struct nfssaargs *argp;
register XDR *xdrs;
{
	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
	    xdr_sattr(xdrs, &argp->saa_sa))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_sattr(xdrs, argp)
register struct nfssattr *argp;
register XDR *xdrs;
{
	if (xdr_u_long(xdrs, &argp->sa_mode) &&
	    xdr_u_long(xdrs, &argp->sa_uid) &&
	    xdr_u_long(xdrs, &argp->sa_gid) &&
	    xdr_u_long(xdrs, &argp->sa_size) &&
	    xdr_timeval(xdrs, &argp->sa_atime) &&
	    xdr_timeval(xdrs, &argp->sa_mtime))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_slargs(xdrs, argp)
register struct nfsslargs *argp;
register XDR *xdrs;
{
	if (xdr_diropargs(xdrs, &argp->sla_from) &&
	    xdr_string(xdrs, &argp->sla_tnm, (u_int) MAXPATHLEN) &&
	    xdr_sattr(xdrs, &argp->sla_sa)) {
		free(argp->sla_tnm);
		return(TRUE);
	}

	return(FALSE);
}

bool_t
xdr_timeval(xdrs, argp)
register struct timeval *argp;
register XDR *xdrs;
{
	if (xdr_long(xdrs, &argp->tv_sec) &&
	    xdr_long(xdrs, &argp->tv_usec))
		return(TRUE);

	return(FALSE);
}

bool_t
xdr_writeargs(xdrs, argp)
register struct nfswriteargs *argp;
register XDR *xdrs;
{
	if (xdr_fhandle(xdrs, &argp->wa_fhandle) &&
	    xdr_long(xdrs, (long *) &argp->wa_begoff) &&
	    xdr_long(xdrs, (long *) &argp->wa_offset) &&
	    xdr_long(xdrs, (long *) &argp->wa_totcount))
		return(TRUE);

	return(FALSE);
}
