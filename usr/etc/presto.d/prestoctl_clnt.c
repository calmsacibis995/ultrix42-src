#ifndef lint
static	char	*sccsid = "@(#)prestoctl_clnt.c	4.1	(ULTRIX)	10/8/90";
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
#include <sys/time.h>
#ifdef DEBUG
#include "prestoioctl.h"
#else
#include <sys/prestoioctl.h>
#endif
#include "prestoctl.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

presto_modstat *
prestoctl_getstate_3(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static presto_modstat res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, PRESTOCTL_GETSTATE, xdr_void, argp, xdr_presto_modstat, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

presto_modstat *
prestoctl_setbytes_3(argp, clnt)
	u_int *argp;
	CLIENT *clnt;
{
	static presto_modstat res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, PRESTOCTL_SETBYTES, xdr_u_int, argp, xdr_presto_modstat, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

presto_modstat *
prestoctl_toggle_3(argp, clnt)
	bool_t *argp;
	CLIENT *clnt;
{
	static presto_modstat res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, PRESTOCTL_TOGGLE, xdr_bool, argp, xdr_presto_modstat, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

presto_get_fs_status *
prestoctl_get_fs_status_3(argp, clnt)
	char **argp;
	CLIENT *clnt;
{
	static presto_get_fs_status res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, PRESTOCTL_GET_FS_STATUS, xdr_wrapstring, argp, xdr_presto_get_fs_status, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}
