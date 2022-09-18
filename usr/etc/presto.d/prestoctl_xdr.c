#ifndef lint
static	char	*sccsid = "@(#)prestoctl_xdr.c	4.1	(ULTRIX)	10/8/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by			*
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
 *	Copyright (c) 1990 Legato Systems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 *  23 Aug 90 -- chet
 *	Added this file; it was derived from Legato sources.
 *
 */

#include <rpc/rpc.h>
#ifdef DEBUG
#include "prestoioctl.h"
#else
#include <sys/prestoioctl.h>
#endif
#include "prestoctl.h"

bool_t
xdr_battery(xdrs, objp)
	XDR *xdrs;
	battery *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_prstates(xdrs, objp)
	XDR *xdrs;
	prstates *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_io(xdrs, objp)
	XDR *xdrs;
	io *objp;
{
	if (!xdr_u_int(xdrs, &objp->total)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->hitclean)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->hitdirty)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pass)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->alloc)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_presto_status(xdrs, objp)
	XDR *xdrs;
	presto_status *objp;
{
	if (!xdr_prstates(xdrs, &objp->pr_state)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_battcnt)) {
		return (FALSE);
	}
	if (!xdr_vector(xdrs, (char *)objp->pr_batt, MAX_BATTERIES, sizeof(battery), xdr_battery)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_maxsize)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_cursize)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_ndirty)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_nclean)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_ninval)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_nactive)) {
		return (FALSE);
	}
	if (!xdr_io(xdrs, &objp->pr_rdstats)) {
		return (FALSE);
	}
	if (!xdr_io(xdrs, &objp->pr_wrstats)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->pr_seconds)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_presto_modstat(xdrs, objp)
	XDR *xdrs;
	presto_modstat *objp;
{
	if (!xdr_bool(xdrs, &objp->ps_status)) {
		return (FALSE);
	}
	switch (objp->ps_status) {
	case FALSE:
		if (!xdr_string(xdrs, &objp->presto_modstat_u.ps_errmsg, ~0)) {
			return (FALSE);
		}
		break;
	case TRUE:
		if (!xdr_presto_status(xdrs, &objp->presto_modstat_u.ps_new)) {
			return (FALSE);
		}
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_presto_fs_status(xdrs, objp)
	XDR *xdrs;
	presto_fs_status *objp;
{
	if (!xdr_bool(xdrs, &objp->pfs_prestoized)) {
		return (FALSE);
	}
	if (!xdr_prstates(xdrs, &objp->pfs_state)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->pfs_enabled)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->pfs_bounceio)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->pfs_unused)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_presto_get_fs_status(xdrs, objp)
	XDR *xdrs;
	presto_get_fs_status *objp;
{
	if (!xdr_bool(xdrs, &objp->succeeded)) {
		return (FALSE);
	}
	switch (objp->succeeded) {
	case FALSE:
		if (!xdr_string(xdrs, &objp->presto_get_fs_status_u.errmsg, ~0)) {
			return (FALSE);
		}
		break;
	case TRUE:
		if (!xdr_presto_fs_status(xdrs, &objp->presto_get_fs_status_u.status)) {
			return (FALSE);
		}
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}
