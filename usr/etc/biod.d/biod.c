#ifndef lint
static char *sccsid = "@(#)biod.c	4.2	ULTRIX	11/14/90";
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
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/lock.h>

/*
 * This is the NFS asynchronous block I/O daemon
 */

main(argc, argv)
	int argc;
	char *argv[];
{
	extern int errno;
	int pid;
	int count;

	if (argc > 2) {
		usage(argv[0]);
	}

	/*
	 * must be super user
	 */
	if (geteuid() != 0){
		(void) fprintf(stderr, "biod:  must be super user\n");
		(void) fflush(stderr);
		exit(1);
	}

	if (argc == 2) {
		count = atoi(argv[1]);
		if (count < 0) {
			usage(argv[0]);
		}
	} else {
		count = 1;
	}

	{ int tt = open("/dev/tty", O_RDWR);
		if (tt > 0) {
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
	}
	while (count--) {
		pid = fork();
		if (pid == 0) {
			plock(PROCLOCK);
			nfs_biod();	/* never returns */
			fprintf(stderr, "%s: nfs_biod ", argv[0]);
			perror("");
			exit(1);
		}
		if (pid < 0) {
			fprintf(stderr, "%s: cannot fork", argv[0]);
			perror("");
			exit(1);
		}
	}
}

usage(name)
	char	*name;
{

	fprintf(stderr, "usage: %s [<count>]\n", name);
	exit(1);
}
