#ifndef lint
static char *sccsid = "@(#)mountxdr.c	4.2	ULTRIX	9/4/90";
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
#include <sys/time.h>
#include <sys/errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>

#define xdr_dev_t xdr_short

xdr_fhstatus(xdrs, fhsp)
	XDR *xdrs;
	struct fhstatus *fhsp;
{
	if (!xdr_int(xdrs, &fhsp->fhs_status))
		return FALSE;
	if (fhsp->fhs_status == 0) {
		if (!xdr_fhandle(xdrs, &fhsp->fhs_fh))
			return FALSE;
	}
}

xdr_fhandle(xdrs, fhp)
	XDR *xdrs;
	fhandle_t *fhp;
{
	if (xdr_opaque(xdrs, fhp, NFS_FHSIZE)) {
		return (TRUE);
	}
	return (FALSE);
}

bool_t
xdr_path(xdrs, pathp)
	XDR *xdrs;
	char **pathp;
{
	if (xdr_string(xdrs, pathp, 1024)) {
		return(TRUE);
	}
	return(FALSE);
}

/* 
 * body of a mountlist
 */
bool_t
xdr_mountbody(xdrs, mlp)
	XDR *xdrs;
	struct mountlist *mlp;
{
	if (!xdr_path(xdrs, &mlp->ml_name))
		return FALSE;
	if (!xdr_path(xdrs, &mlp->ml_path))
		return FALSE;
	return(TRUE);
}

bool_t
xdr_mountlist(xdrs, mlp)
	register XDR *xdrs;
	register struct mountlist **mlp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	int more_elements;
	register int freeing = (xdrs->x_op == XDR_FREE);
	register struct mountlist **nxt;

	while (TRUE) {
		more_elements = (*mlp != NULL);
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
		if (! more_elements)
			return (TRUE);  /* we are done */
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the nxt object
		 * before we free the current object ...
		 */
		if (freeing)
			nxt = &((*mlp)->ml_nxt); 
		if (! xdr_reference(xdrs, mlp, sizeof(struct mountlist),
		    xdr_mountbody))
			return (FALSE);
		mlp = (freeing) ? nxt : &((*mlp)->ml_nxt);
	}
}

/*
 * Strange but true: the boolean that tells if another element
 * in the list is present has already been checked.  We handle the
 * body of this element then check on the next element.  YUK.
 */
bool_t
xdr_groups(xdrs, gr)
	register XDR *xdrs;
	register struct groups *gr;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	int more_elements;

	if (! xdr_path(xdrs, &(gr->g_name)))
		return (FALSE);
	more_elements = (gr->g_next != NULL);
	if (! xdr_bool(xdrs, &more_elements))
		return (FALSE);
	if (! more_elements) {
		gr->g_next = NULL;
		return (TRUE);  /* we are done */
	}
	return (xdr_reference(xdrs, &(gr->g_next), sizeof(struct groups),
	    xdr_groups));
}

/* 
 * body of a exportlist
 */
bool_t
xdr_exportbody(xdrs, ex)
	XDR *xdrs;
	struct exports *ex;
{
	int more_elements;

	if (!xdr_path(xdrs, &ex->ex_name))
		return FALSE;
	more_elements = (ex->ex_groups != NULL);
	if (! xdr_bool(xdrs, &more_elements))
		return (FALSE);
	if (! more_elements) {
		ex->ex_groups = NULL;
		return (TRUE);  /* we are done */
	}
	if (! xdr_reference(xdrs, &(ex->ex_groups), sizeof(struct groups),
	    xdr_groups))
		return (FALSE);
	return(TRUE);
}


/*
 * Encodes the export list structure "exports" on the
 * wire as:
 * bool_t eol;
 * if (!eol) {
 * 	char *name;
 *	struct groups *groups;
 * }
 * where groups look like:
 * if (!eog) {
 *	char *gname;
 * }
 */
bool_t
xdr_exports(xdrs, exp)
	register XDR *xdrs;
	register struct exports **exp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	int more_elements;
	register int freeing = (xdrs->x_op == XDR_FREE);
	register struct exports **nxt;

	while (TRUE) {
		more_elements = (*exp != NULL);
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
		if (! more_elements)
			return (TRUE);  /* we are done */
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the nxt object
		 * before we free the current object ...
		 */
		if (freeing)
			nxt = &((*exp)->ex_next); 
		if (! xdr_reference(xdrs, exp, sizeof(struct exports),
		    xdr_exportbody))
			return (FALSE);
		exp = (freeing) ? nxt : &((*exp)->ex_next);
	}
}
