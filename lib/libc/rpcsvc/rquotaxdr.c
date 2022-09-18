#ifndef lint
static char *sccsid = "@(#)rquotaxdr.c	4.1	ULTRIX	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/param.h>
#include <sys/quota.h>
#include <rpcsvc/rquota.h>


bool_t
xdr_getquota_args(xdrs, gq_argsp)
	XDR *xdrs;
	struct getquota_args *gq_argsp;
{
	extern bool_t xdr_path();

	return (xdr_path(xdrs, &gq_argsp->gqa_pathp) &&
	    xdr_int(xdrs, &gq_argsp->gqa_uid));
}

struct xdr_discrim gqr_arms[2] = {
	{ (int)Q_OK, xdr_rquota },
	{ __dontcare__, NULL }
};

bool_t
xdr_getquota_rslt(xdrs, gq_rsltp)
	XDR *xdrs;
	struct getquota_rslt *gq_rsltp;
{

	return (xdr_union(xdrs,
	    &gq_rsltp->gqr_status, &gq_rsltp->gqr_rquota,
	    gqr_arms, xdr_void));
}

bool_t
xdr_rquota(xdrs, rqp)
	XDR *xdrs;
	struct rquota *rqp;
{

	return (xdr_int(xdrs, &rqp->rq_bsize) &&
	    xdr_bool(xdrs, &rqp->rq_active) &&
	    xdr_u_long(xdrs, &rqp->rq_bhardlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_bsoftlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_curblocks) &&
	    xdr_u_long(xdrs, &rqp->rq_fhardlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_fsoftlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_curfiles) &&
	    xdr_u_long(xdrs, &rqp->rq_btimeleft) &&
	    xdr_u_long(xdrs, &rqp->rq_ftimeleft) );
}
