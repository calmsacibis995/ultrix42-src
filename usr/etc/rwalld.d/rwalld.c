#ifndef lint
static	char	*sccsid = "@(#)rwalld.c	4.1	(ULTRIX)	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984,86 by			*
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
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Modification History:
 *
 * 09-Jun-88	Mark Parenti
 *	Changed signal handlers to void.
 */

#include <rpcsvc/rwall.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>

void rwalld();
void	reapchildren();

main()
{
	register SVCXPRT *transp;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	int readfds;

	(void) signal(SIGCHLD, reapchildren);
#ifndef INETD
	{
	/*
	 * Remove this chunk of code if we ever run under inetd.
	 * Also remove the abort() routine if running under inetd.
	 */
	int pid, s, t, nfds;
#ifndef DEBUG
		pid = fork();
		if (pid < 0) {
			perror("rwalld: fork");
			exit(1);
		}
		if (pid > 0)
			exit(0);
		nfds = getdtablesize();
		for (t = 0; t < nfds; t++)
			close(t);
		open("/", 0);
		dup2(0, 1);
		dup2(0, 2);
#endif DEBUG
		if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			perror("rwalld: socket");
			exit(1);
		}
		if (bind(s, &addr, sizeof(addr)) < 0) {
			perror("rwalld bind");
			exit(1);
		}
		if (getsockname(s, &addr, &len) != 0) {
			perror("rwalld: getsockname");
			exit(1);
		}
		pmap_unset(WALLPROG, WALLVERS);
		pmap_set(WALLPROG, WALLVERS, IPPROTO_UDP, ntohs(addr.sin_port));
		if (dup2(s, 0) < 0) {
			perror("rwalld: dup2");
			exit(1);
		}
/* End chunk to remove if running under inetd. */
	}
#endif INETD

	if ((transp = svcudp_create(RPC_ANYSOCK)) == NULL) {
		fprintf(stderr, "rwalld: couldn't create an RPC server\n");
		abort();
	}
	pmap_unset(WALLPROG, WALLVERS);
	if (!svc_register(transp, WALLPROG, WALLVERS, rwalld, IPPROTO_UDP)) {
		fprintf(stderr, "rwalld: couldn't register with service\n");
		abort();
	}

	svc_run();
	fprintf(stderr, "rwalld: svc_run shouldn't have returned\n");
	abort();
}

void
rwalld(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
int pid;
	FILE *fp, *popen();
	char *msg;

	msg = NULL;
	switch (rqstp->rq_proc) {
		case 0:
			if (svc_sendreply(transp, xdr_void, 0)  == FALSE) 
				fprintf(stderr, "rwalld: svc_sendreply");
			break;

		case WALLPROC_WALL:
			if (!svc_getargs(transp, xdr_wrapstring, &msg)) {
				svcerr_decode(transp);
				break;
			}
			if (svc_sendreply(transp, xdr_void, 0)  == FALSE)
				fprintf(stderr, "rwalld: svc_sendreply2");
			
			if(fork() == 0) {
				fp = popen("/bin/wall", "w");
				fprintf(fp, "%s", msg);
				pclose(fp);
				exit(0);
			}
			break;

		default: 
			svcerr_noproc(transp);
			break;
	}
	return;
}

void
reapchildren()
{

	while (wait3((struct wait *)0, WNOHANG, (struct rusage *)0) > 0)
		;
}
