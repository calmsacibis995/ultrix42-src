#ifndef lint
static char *sccsid = "@(#)mesg.c	4.1      ULTRIX  7/17/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * Modification History
 * 06 May 89 -- dal
 *	Took 4.3BSD version with some modifications.
 */

/*
char copyright[] =
"@(#) Copyright (c) 1987 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)mesg.c	4.4 (Berkeley) 11/24/87";
*/

/*
 * mesg -- set current tty to accept or
 *	forbid write permission.
 *
 *	mesg [y] [n]
 *		y allow messages
 *		n forbid messages
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define	TTY_WRITE	020

static char *tty;

main(argc, argv)
	int	argc;
	char	**argv;
{
	struct stat sbuf;
	char *ttyname();

	if (!(tty = ttyname(2))) {
		fputs("mesg: not a device in /dev.\n", stderr);
		exit(-1);
	}
	if (stat(tty, &sbuf) < 0) {
		perror("mesg");
		exit(-1);
	}
	if (argc < 2) {
		if (sbuf.st_mode & TTY_WRITE) {
			fputs("is y\n", stderr);
			exit(0);
		}
		fputs("is n\n", stderr);
		exit(1);
	}
	switch(*argv[1]) {
	case 'y':
		newmode(sbuf.st_mode | TTY_WRITE);
		exit(0);
	case 'n':
		newmode(sbuf.st_mode & ~022);
		exit(1);
	default:
		fputs("usage: mesg [y] [n]\n", stderr);
	}
	exit(-1);
}

static
newmode(m)
	u_short m;
{
	if (chmod(tty, m) < 0) {
		perror("mesg");
		exit(-1);
	}
}
