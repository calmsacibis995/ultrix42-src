#ifndef lint
static	char	*sccsid = "@(#)implogd.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,1988 by			*
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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
#ifndef lint
char copyright[] =
" Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "implogd.c	5.3 (Berkeley) 6/25/86";
#endif not lint
*/
#include <sgtty.h>

#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#ifdef 43BSD
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif 43BSD
#include <sys/file.h>

#include <netinet/in.h>
#include <netimp/if_imp.h>

#define	LOGFILE	"/usr/adm/implog"

u_char	request[1024];
int	marktime();
int	options;
extern	int errno;
int	log;

/*
 * Socket address, internet style, with
 * unused space taken by timestamp and packet
 * size.
 */
struct sockstamp {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	time_t	sin_time;
	int	sin_len;
};

main(argc, argv)
	char *argv[];
{
	int i, s;
	time_t t;
	struct sockstamp from;

	argc--, argv++;
#ifdef 43BSD
	openlog("implogd", LOG_PID | LOG_ODELAY, LOG_DAEMON);
#else
	openlog("implogd", LOG_PID);
#endif
	if (argc > 0 && !strcmp(argv[0], "-d"))
		options |= SO_DEBUG;
	log = open(LOGFILE, O_CREAT|O_WRONLY|O_APPEND, 0644);
	if (log < 0) {
		syslog(LOG_ERR, "%s: %m\n", LOGFILE);
		perror("implogd: open");
		exit(1);
	}
	from.sin_time = time(0);
	from.sin_len = sizeof (time_t);
	write(log, (char *)&from, sizeof (from));
	if ((s = socket(AF_IMPLINK, SOCK_RAW, 0)) < 0) {
		syslog(LOG_ERR, "socket: %m\n");
		perror("implogd: socket");
		exit(5);
	}
#ifndef DEBUG
	if (fork())
		exit(0);
	for (i = 0; i < 10; i++)
		if (i != log && i != s)
			(void) close(i);
	(void) open("/", 0);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
	{ int tt = open("/dev/tty", 2);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	  }
	}
#endif
	for (;;) {
		int fromlen = sizeof (from), len;

		len = recvfrom(s, request, sizeof (request), 0,
			&from, &fromlen);
		if (len < 0) {
			syslog(LOG_ERR, "recvfrom: %m\n");
			perror("implogd: recvfrom");
			continue;
		}
		if (len == 0 || len > IMPMTU)	/* sanity */
			continue;
		from.sin_len = len;
		from.sin_time = time(0);
		write(log, (char *)&from, sizeof (from));
		write(log, request, len);
	}
	/*NOTREACHED*/
}
