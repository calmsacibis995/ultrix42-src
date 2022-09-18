#ifndef lint
static	char	*sccsid = "@(#)nfsd.c	4.2	(ULTRIX)	11/14/90";
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
 *	Modification History:
 *
 * 23 Nov 88 -- chet
 *	Added umask(0) for default file creation mask.
 *
 *	09-Jun-88	Suzanne Logcher
 *		Added Chet's change to check if mountd is already
 *		running.  This fix was previously made to v2.3.
 *
 *	09-Jun-88	Mark Parenti
 *		Changed signal handlers to void.
 *
 */
/* NFS server */

#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/lock.h>
#include <nfs/nfs.h>
#include <stdio.h>
#include <signal.h>
#include <rpcsvc/mount.h>

char host[NFS_MAXNAMLEN+1];

void
catch()
{
}

main(argc, argv)
char	*argv[];
{
	register int sock;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	char *dir = "/";
	int nservers = 1;
	int pid, t;
	int err;

	if (argc > 2) {
		(void) fprintf(stderr, "usage: %s [servers]\n", argv[0]);
		exit(1);
	}
	if (argc == 2) {
		nservers = atoi(argv[1]);
	}

	if (geteuid() != 0){
		(void) fprintf(stderr, "nfsd:  must be super user\n");
		exit(1);
	}

	/*
	 * Set current and root dir to server root
	 */
	if (chroot(dir) < 0) {
		perror(dir);
		exit(1);
	}
	if (chdir(dir) < 0) {
		perror(dir);
		exit(1);
	}

	/* clear default file creation mask */
	umask(0);

	{ int tt = open("/dev/tty", O_RDWR);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	  }
	}

	addr.sin_addr.S_un.S_addr = 0;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NFS_PORT);
	if ( ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	    || (bind(sock, &addr, len) != 0)
	    || (getsockname(sock, &addr, &len) != 0) ) {
		(void) close(sock);
		(void) fprintf(stderr, "%s: server(s) already active\n",
		       argv[0]);
		exit(1);
	}

	/* "Cannot happen" unless no file descriptors are passed... */
	if (sock == 0) {
		(void) fprintf(stderr, "%s: botched I/O descriptors\n",
		       argv[0]);
		exit(1);
	}

	if (gethostname(host, NFS_MAXNAMLEN) < 0) {
		perror ("%s: gethostname", argv[0]);
		exit (1);
	}

	if (err = callrpc(host, MOUNTPROG, MOUNTVERS, NULLPROC,
		xdr_void, 0, xdr_void, 0)) {
		(void) fprintf(stderr, "%s: can't find mountd: ", argv[0]);
		clnt_perrno(err);
		(void) fprintf(stderr, "\n");
		exit(1);
	}

	/* register with the portmapper */
	pmap_unset(NFS_PROGRAM, NFS_VERSION);
	pmap_set(NFS_PROGRAM, NFS_VERSION, IPPROTO_UDP, NFS_PORT);
	while (--nservers >= 0) {
		if (!fork()) {
			server(sock);
			exit(0);
		}
	}
	exit(0);
}

/*
 *  Start an NFS server.  This routine never returns unless the nfs_svc
 *  call fails or the daemon is killed.
 */

int
server(sock)
	int sock;
{
	int s;

	for (s = 0; s < 10; s++) {
		if (s != sock)
			(void) close(s);
	}

	(void) open("/", O_RDONLY);
	if (sock != 1)
		(void) dup2(0, 1);
	if (sock != 2)
		(void) dup2(0, 2);
	signal(SIGTERM, catch);
	plock(PROCLOCK);
	nfs_svc(sock);
}
