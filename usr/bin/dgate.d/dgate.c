#ifndef lint
static char *Sccsid="@(#)dgate.c	4.1	(ULTRIX)	7/17/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

#include <sys/types.h>
#include <signal.h>
#include <sgtty.h>
#include <stdio.h>
#ifdef	pdp11
#define	signal	sigset
#endif	pdp11
#include "dgate.h"

#define CTLQ	021
#define CTLS	023
#define OFF	-1

struct	tchars	otchars;
struct	ltchars oltchars;
struct	tchars	tchars = { OFF, OFF, CTLQ, CTLS, OFF, OFF };
struct	ltchars	ltchars = { OFF, OFF, OFF, OFF, OFF, OFF };
struct	sgttyb	osgb, sgb;
int	cpid;

extern char dgated[];

main(argc, argv, envp)
int	argc;
char	**argv;
char	*envp[];
{
	char	*namebuf[256];
	char	**tp;
	char	gate_way[64];
	char	gate_accnt[64];
	int	cleanup();
	char 	*quote();
	if (argc < 2) {
		fprintf (stderr, "usage: %s hostname\n", argv[0]);
		exit(1);
	}
	
	getgateway(gate_way, gate_accnt);

	tp = namebuf;
	*tp++ = RSH;
	*tp++ = gate_way;
	*tp++ = "-l";
	*tp++ = gate_accnt;
	*tp++ = dgated;

	argv++;
	while(--argc)
		*tp++ = quote(*argv++);
	*tp = (char *) 0;

	if(isatty(0) != 1) {
		fprintf (stderr, "%s: stdin must be a tty\n", argv[0]);
		exit(1);
	}

	setterm();
	switch(cpid = fork()) {
		case 0 :
			execve (RSH, namebuf, envp);
			fprintf (stderr, "%s: exec of rsh failed\r\n", argv[0]);
			exit(1);
		case -1 :
			fprintf (stderr, "%s: can't fork\n\r", argv[0]);
			resetterm();
			exit(1);
	}
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGCHLD, cleanup);
	signal(SIGTSTP, SIG_IGN);

	while (wait((int *) 0) != -1)
		;
	cleanup(0);
}

setterm() {
	ioctl(0, TIOCGETP, &osgb);
	ioctl(0, TIOCGETC, &otchars);
	ioctl(0, TIOCGLTC, &oltchars);
	sgb.sg_ispeed = osgb.sg_ispeed;
	sgb.sg_ospeed = osgb.sg_ospeed;
	sgb.sg_erase = sgb.sg_kill = OFF;
	sgb.sg_flags = (XTABS | CBREAK) & ~ECHO;
	ioctl(0, TIOCFLUSH, (char *) 0);
	ioctl(0, TIOCSETP, &sgb);
	ioctl(0, TIOCSETC, &tchars);
	ioctl(0, TIOCSLTC, &ltchars);
}

resetterm () {
	ioctl(0, TIOCSETP, &osgb);
	ioctl(0, TIOCSLTC, &oltchars);
	ioctl(0, TIOCSETC, &otchars);
}

cleanup(how)
int how;
{
	resetterm();

	if (how) {
		if (how != SIGCHLD) {
			kill(cpid, SIGKILL);
			fprintf (stderr, "\007Lost connection\n");
		}
		wait((int *) 0);	
	}
	fprintf (stderr, "\n");
	exit(how & ~SIGCHLD);
}
