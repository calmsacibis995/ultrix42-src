#ifndef lint
static	char	*sccsid = "@(#)prestoctl_svc.c	4.1	(ULTRIX)	10/8/90";
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

#include <stdio.h>
#include <rpc/rpc.h>
#include <syslog.h>
#include <sys/file.h>
#ifdef DEBUG
#include "prestoioctl.h"
#else
#include <sys/prestoioctl.h>
#endif
#include "prestoctl.h"

static void prestoctlprog_3();

call_svc(presto_fd)
	int presto_fd;
{
	int i, pid;
	register SVCXPRT *transp;

	/*
	 * put me in the background
	 */
	pid = fork();
	if (pid < 0) {
		syslog(LOG_ERR, "Cannot fork: %m");
		exit(1);
	}
	if (pid)
		exit(0);
	/*
	 * Close existing fds and detach from controlling terminal
	 */
	i = getdtablesize();
	while (--i >= 0)
		if (i != presto_fd)
			(void)close(i);
	(void)open("/dev/null", O_RDONLY);
	(void)open("/dev/null", O_WRONLY);
	(void)dup(1);	
	i = open("/dev/tty", O_RDWR);
	if (i >= 0) {
		ioctl(i, TIOCNOTTY, (char *)0);
		(void)close(i);
	}

	(void) pmap_unset(PRESTOCTLPROG, PRESTOCTLVERS);

	/* 
	 *  Create UDP transport
	 */
	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		syslog (LOG_ERR, "cannot create UDP service");
		exit(1);
	}
	if (!svc_register(transp, PRESTOCTLPROG, PRESTOCTLVERS, prestoctlprog_3, IPPROTO_UDP)) {
		syslog (LOG_ERR, "unable to register (PRESTOCTLPROG, PRESTOCTLVERS, UDP)");
		exit(1);
	}

	/*
	 * Create TCP transport
	 */
	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		syslog (LOG_ERR, "cannot create TCP service");
		exit(1);
	}
	if (!svc_register(transp, PRESTOCTLPROG, PRESTOCTLVERS, prestoctlprog_3, IPPROTO_TCP)) {
		syslog (LOG_ERR, "unable to register (PRESTOCTLPROG, PRESTOCTLVERS, TCP)");
		exit(1);
	}

	svc_run();
	syslog (LOG_ERR, "svc_run returned");
	exit(1);
	/* NOTREACHED */
}

static void
prestoctlprog_3(rqstp, transp)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		u_int prestoctl_setbytes_3_arg;
		bool_t prestoctl_toggle_3_arg;
		char *prestoctl_get_fs_status_3_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply(transp, xdr_void, (char *)NULL);
		return;

	case PRESTOCTL_GETSTATE:
		xdr_argument = xdr_void;
		xdr_result = xdr_presto_modstat;
		local = (char *(*)()) prestoctl_getstate_3;
		break;

	case PRESTOCTL_SETBYTES:
		xdr_argument = xdr_u_int;
		xdr_result = xdr_presto_modstat;
		local = (char *(*)()) prestoctl_setbytes_3;
		break;

	case PRESTOCTL_TOGGLE:
		xdr_argument = xdr_bool;
		xdr_result = xdr_presto_modstat;
		local = (char *(*)()) prestoctl_toggle_3;
		break;

	case PRESTOCTL_GET_FS_STATUS:
		xdr_argument = xdr_wrapstring;
		xdr_result = xdr_presto_get_fs_status;
		local = (char *(*)()) prestoctl_get_fs_status_3;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	bzero((char *)&argument, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, &argument)) {
		syslog (LOG_ERR, "unable to free arguments");
		exit(1);
	}
	return;
}
