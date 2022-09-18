#ifndef lint
static char *sccsid = "@(#)rstatxdr.c	4.1      (ULTRIX)        7/3/90";
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
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
/* 	Change Log
 *	04/13/89	jhw	added SUN 4.0 changes
 */

#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>

rstat(host, statp)
	char *host;
	struct statstime *statp;
{
	return (callrpc(host, RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
	    xdr_void, (char *) NULL, xdr_statstime, (char *) statp));
}

havedisk(host)
	char *host;
{
	long have;
	
	if (callrpc(host, RSTATPROG, RSTATVERS_SWTCH, RSTATPROC_HAVEDISK,
	    xdr_void, (char *) NULL, xdr_long, (char *) &have) != 0)
		return (-1);
	else
		return (have);
}

xdr_stats(xdrs, statp)
	XDR *xdrs;
	struct stats *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, (int *) &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, (int *) &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	return (1);
}

xdr_statsswtch(xdrs, statp)		/* version 2 */
	XDR *xdrs;
	struct statsswtch *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, (int *) &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, (int *) &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < 3; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	return (1);
}

xdr_statstime(xdrs, statp)		/* version 3 */
	XDR *xdrs;
	struct statstime *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, (int *) &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, (int *) &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < 3; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	if (xdr_timeval(xdrs, &statp->curtime) == 0)
		return (0);
	/* 
	 * Many implementations of this protocol incorrectly left out
	 * if_opackets.  This value is included here for compatibility 
	 * with these buggy implementations.
	 */
	(void) xdr_int (xdrs, &statp->if_opackets);
	return (1);
}

xdr_timeval(xdrs, tvp)
	XDR *xdrs;
	struct timeval *tvp;
{
	if (xdr_long(xdrs, &tvp->tv_sec) == 0)
		return (0);
	if (xdr_long(xdrs, &tvp->tv_usec) == 0)
		return (0);
	return (1);
}
