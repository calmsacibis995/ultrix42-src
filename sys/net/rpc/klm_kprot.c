#ifndef lint
static	char	*sccsid = "@(#)klm_kprot.c	4.1	7/2/90";
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

/*
 * XDR routines for Kernel<->Lock-Manager communication
 *
 * Generated from ../rpcsvc/klm_prot.x
 */

#include "../rpc/rpc.h"
#include "../rpcsvc/klm_prot.h"

#define NULL 0


bool_t
xdr_klm_stats(xdrs,objp)
	XDR *xdrs;
	klm_stats *objp;
{
	if (! xdr_enum(xdrs, (enum_t *) objp)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_lock(xdrs,objp)
	XDR *xdrs;
	klm_lock *objp;
{
	if (! xdr_string(xdrs, &objp->server_name, LM_MAXSTRLEN)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->fh)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->pid)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_offset)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_len)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_holder(xdrs,objp)
	XDR *xdrs;
	klm_holder *objp;
{
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->svid)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_offset)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_len)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_stat(xdrs,objp)
	XDR *xdrs;
#ifdef KERNEL
	klm_testrply *objp;
#else
	klm_stat *objp;
#endif
{
	if (! xdr_klm_stats(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_testrply(xdrs,objp)
	XDR *xdrs;
	klm_testrply *objp;
{
	static struct xdr_discrim choices[] = {
		{ (int) klm_granted, xdr_void },
		{ (int) klm_denied, xdr_klm_holder },
		{ (int) klm_denied_nolocks, xdr_void },
		{ (int) klm_working, xdr_void },
		{ __dontcare__, NULL }
	};

	if (! xdr_union(xdrs, (enum_t *) &objp->stat, 
		(char *) &objp->klm_testrply_u, choices, NULL_xdrproc_t)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_lockargs(xdrs,objp)
	XDR *xdrs;
	klm_lockargs *objp;
{
	if (! xdr_bool(xdrs, &objp->block)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_klm_lock(xdrs, &objp->alock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_testargs(xdrs,objp)
	XDR *xdrs;
#ifdef KERNEL
	klm_lockargs *objp;
#else
	klm_testargs *objp;
#endif
{
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_klm_lock(xdrs, &objp->alock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_klm_unlockargs(xdrs,objp)
	XDR *xdrs;
#ifdef KERNEL
	klm_lockargs *objp;
#else
	klm_unlockargs *objp;
#endif
{
	if (! xdr_klm_lock(xdrs, &objp->alock)) {
		return(FALSE);
	}
	return(TRUE);
}


