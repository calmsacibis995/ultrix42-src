#ifndef lint
static char *sccsid = "@(#)rusersxdr.c	4.1      (ULTRIX)        7/3/90";
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
/*	Change Log
 *	04/13/89	jhw	added SUN 4.0 changes
 */

#include <rpc/rpc.h>
#include <utmp.h>
#include <rpcsvc/rusers.h>

rusers(host, up)
	char *host;
	struct utmpidlearr *up;
{
	return (callrpc(host, RUSERSPROG, RUSERSVERS_IDLE, RUSERSPROC_NAMES,
	    xdr_void, (char *) NULL, xdr_utmpidlearr, (char *) up));
}

rnusers(host)
	char *host;
{
	int nusers;
	
	if (callrpc(host, RUSERSPROG, RUSERSVERS_ORIG, RUSERSPROC_NUM,
	    xdr_void, (char *) NULL, xdr_u_long, (char *) &nusers) != 0)
		return (-1);
	else
		return (nusers);
}

xdr_utmp(xdrsp, up)
	XDR *xdrsp;
	struct utmp *up;
{
	int len;
	char *p;

	len = sizeof(up->ut_line);
	p = up->ut_line;
	if (xdr_bytes(xdrsp, &p, &len, len) == FALSE)
		return (0);
	len = sizeof(up->ut_name);
	p = up->ut_name;
	if (xdr_bytes(xdrsp, &p, &len, len) == FALSE)
		return (0);
	len = sizeof(up->ut_host);
	p = up->ut_host;
	if (xdr_bytes(xdrsp, &p, &len, len) == FALSE)
		return (0);
	if (xdr_long(xdrsp, &up->ut_time) == FALSE)
		return (0);
	return (1);
}

xdr_utmpidle(xdrsp, ui)
	XDR *xdrsp;
	struct utmpidle *ui;
{
	if (xdr_utmp(xdrsp, &ui->ui_utmp) == FALSE)
		return (0);
	if (xdr_u_int(xdrsp, &ui->ui_idle) == FALSE)
		return (0);
	return (1);
}

xdr_utmpptr(xdrsp, up)
	XDR *xdrsp;
	struct utmp **up;
{
	if (xdr_reference(xdrsp, (char **) up, sizeof (struct utmp), xdr_utmp) == FALSE)
		return (0);
	return (1);
}

xdr_utmpidleptr(xdrsp, up)
	XDR *xdrsp;
	struct utmpidle **up;
{
	if (xdr_reference(xdrsp, (char **) up, sizeof (struct utmpidle), xdr_utmpidle)
	    == FALSE)
		return (0);
	return (1);
}

xdr_utmparr(xdrsp, up)
	XDR *xdrsp;
	struct utmparr *up;
{
	return (xdr_array(xdrsp, (char **) &up->uta_arr, &(up->uta_cnt),
	    MAXUSERS, sizeof(struct utmp *), xdr_utmpptr));
}

xdr_utmpidlearr(xdrsp, up)
	XDR *xdrsp;
	struct utmpidlearr *up;
{
	return (xdr_array(xdrsp, (char **) &up->uia_arr, &(up->uia_cnt),
	    MAXUSERS, sizeof(struct utmpidle *), xdr_utmpidleptr));
}
