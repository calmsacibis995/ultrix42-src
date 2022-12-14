
#ifndef lint
static	char	*sccsid = "@(#)ventel.c	4.1		7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
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
 * Routines for calling up on a Ventel Modem
 * The Ventel is expected to be strapped for "no echo".
 */
#include "tip.h"

#define	MAXRETRY	5

static	int sigALRM();
static	int timeout = 0;
static	jmp_buf timeoutbuf;

ven_dialer(num, acu)
	register char *num;
	char *acu;
{
	register char *cp;
	register int connected = 0;
#ifdef ACULOG
	char line[80];
#endif
	/*
	 * Get in synch with a couple of carriage returns
	 */
	if (!vensync(FD)) {
		printf("can't synchronize with ventel\n");
#ifdef ACULOG
		logent(value(HOST), num, "ventel", "can't synch up");
#endif
		return (0);
	}
	if (boolean(value(VERBOSE)))
		printf("\ndialing...");
	fflush(stdout);
	ioctl(FD, TIOCHPCL, 0);
	echo("#k$\r$\n$D$I$A$L$:$ ");
	for (cp = num; *cp; cp++) {
		sleep(1);
		write(FD, cp, 1);
	}
	echo("\r$\n");
	if (gobble('\n'))
		connected = gobble('!');
	ioctl(FD, TIOCFLUSH);
#ifdef ACULOG
	if (timeout) {
		sprintf(line, "%d second dial timeout",
			number(value(DIALTIMEOUT)));
		logent(value(HOST), num, "ventel", line);
	}
#endif
	if (timeout)
		ven_disconnect();	/* insurance */
	return (connected);
}

ven_disconnect()
{

	close(FD);
}

ven_abort()
{

	write(FD, "\03", 1);
	close(FD);
}

static int
echo(s)
	register char *s;
{
	char c;

	while (c = *s++) switch (c) {

	case '$':
		read(FD, &c, 1);
		s++;
		break;

	case '#':
		c = *s++;
		write(FD, &c, 1);
		break;

	default:
		write(FD, &c, 1);
		read(FD, &c, 1);
	}
}

static int
sigALRM()
{

	printf("\07timeout waiting for reply\n");
	timeout = 1;
	longjmp(timeoutbuf, 1);
}

static int
gobble(match)
	register char match;
{
	char c;
	int (*f)();

	signal(SIGALRM, sigALRM);
	timeout = 0;
	do {
		if (setjmp(timeoutbuf)) {
			ioctl(FD,TIOCNCAR); /* so writes wont hang if carrier never came up*/
			signal(SIGALRM, f);
			return (0);
		}
		alarm(number(value(DIALTIMEOUT)));
		read(FD, &c, 1);
		alarm(0);
		c &= 0177;
#ifdef notdef
		if (boolean(value(VERBOSE)))
			putchar(c);
#endif
	} while (c != '\n' && c != match);
#ifdef ONDELAY
	/*
	 * At this point, wait for carrier; the ACU told
	 * us things were cool now let's see if this
	 * really is the case.
	 */
	if (c != match)
		return(0);	/* oops */
	ioctl(FD, TIOCCAR);
	alarm(40);	/* Larry wants a better number */
	ioctl(FD, TIOCWONLINE);	/* suspend, waiting for CD */
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	return(1);
#else
	signal(SIGALRM, SIG_DFL);
	return (c == match);
#endif
}

#define min(a,b)	((a)>(b)?(b):(a))
/*
 * This convoluted piece of code attempts to get
 * the ventel in sync.  If you don't have FIONREAD
 * there are gory ways to simulate this.
 */
static int
vensync(fd)
{
	int already = 0, nread;
	char buf[60];

	/*
	 * Toggle DTR to force anyone off that might have left
	 * the modem connected, and insure a consistent state
	 * to start from.
	 *
	 * If you don't have the ioctl calls to diddle directly
	 * with DTR, you can always try setting the baud rate to 0.
	 */
	ioctl(FD, TIOCCDTR, 0);
	sleep(2);
	ioctl(FD, TIOCSDTR, 0);
	while (already < MAXRETRY) {
		/*
		 * After reseting the modem, send it two \r's to
		 * autobaud on. Make sure to delay between them
		 * so the modem can frame the incoming characters.
		 */
		write(fd, "\r", 1);
		sleep(1);
		write(fd, "\r", 1);
		sleep(3);
		if (ioctl(fd, FIONREAD, (caddr_t)&nread) < 0) {
			perror("tip: ioctl");
			continue;
		}
		while (nread > 0) {
			read(fd, buf, min(nread, 60));
			if ((buf[nread - 1] & 0177) == '$')
				return (1);
			nread -= min(nread, 60);
		}
		sleep(1);
		already++;
	}
	return (0);
}
