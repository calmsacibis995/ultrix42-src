#ifndef lint
static	char	*sccsid = "@(#)biz22.c	4.1		7/17/90";
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

#include "tip.h"

#define DISCONNECT_CMD	"\20\04"	/* disconnection string */

static	int sigALRM();
static	int timeout = 0;
static	jmp_buf timeoutbuf;

/*
 * Dial up on a BIZCOMP Model 1022 with either
 * 	tone dialing (mod = "V")
 *	pulse dialing (mod = "W")
 */
static int
biz_dialer(num, mod)
	char *num, *mod;
{
	register int connected = 0;
	char cbuf[40];
	void (*f)();

	if (boolean(value(VERBOSE)))
		printf("\nstarting call...");
	/*
	 * Disable auto-answer and configure for tone/pulse
	 *  dialing
	 */
	if (cmd("\02K\r")) {
		printf("can't initialize bizcomp...");
		return (0);
	}
	strcpy(cbuf, "\02.\r");
	cbuf[1] = *mod;
	if (cmd(cbuf)) {
		printf("can't set dialing mode...");
		return (0);
	}
	strcpy(cbuf, "\02D");
	strcat(cbuf, num);
	strcat(cbuf, "\r");
	write(FD, cbuf, strlen(cbuf));
	if (!detect("7\r")) {
		printf("can't get dial tone...");
		return (0);
	}
	if (boolean(value(VERBOSE)))
		printf("ringing...");
	/*
	 * The reply from the BIZCOMP should be:
	 *	2 \r or 7 \r	failure
	 *	1 \r		success
	 */
	connected = detect("1\r");
#ifdef ACULOG
	if (timeout) {
		char line[80];

		sprintf(line, "%d second dial timeout",
			number(value(DIALTIMEOUT)));
		logent(value(HOST), num, "biz1022", line);
	}
#endif
	if (timeout)
		biz22_disconnect();	/* insurance */
#ifdef ONDELAY
	/*
	 * At this point, wait for carrier; the ACU told
	 * us things were cool now let's see if this
	 * really is the case.
	 */
	if (!connected)
		return(0);	/* oops */
	ioctl(FD, TIOCCAR);
	f = signal(SIGALRM, sigALRM);
	if (setjmp(timeoutbuf)) {
		ioctl(FD,TIOCNCAR); /* so writes wont hang if carrier never came up*/
		biz22_abort();
		signal(SIGALRM, f);
		return (0);
	}
	alarm(40);	/* Larry wants a better number */
	ioctl(FD, TIOCWONLINE);	/* suspend, waiting for CD */
	alarm(0);
	signal(SIGALRM, f);
	return(1);
#else
	return (connected);
#endif
}

biz22w_dialer(num, acu)
	char *num, *acu;
{

	return (biz_dialer(num, "W"));
}

biz22f_dialer(num, acu)
	char *num, *acu;
{

	return (biz_dialer(num, "V"));
}

biz22_disconnect()
{
	int rw = 2;

	write(FD, DISCONNECT_CMD, 4);
	sleep(2);
	ioctl(FD, TIOCFLUSH, &rw);
}

biz22_abort()
{

	write(FD, "\02", 1);
}

static int
sigALRM()
{

	timeout = 1;
	longjmp(timeoutbuf, 1);
}

static int
cmd(s)
	register char *s;
{
	char c;
	void (*f)();

	write(FD, s, strlen(s));
	f = signal(SIGALRM, sigALRM);
	if (setjmp(timeoutbuf)) {
		ioctl(FD,TIOCNCAR); /* so writes wont hang if carrier never came up*/
		biz22_abort();
		signal(SIGALRM, f);
		return (1);
	}
	alarm(number(value(DIALTIMEOUT)));
	read(FD, &c, 1);
	alarm(0);
	signal(SIGALRM, f);
	c &= 0177;
	return (c != '\r');
}

static int
detect(s)
	register char *s;
{
	char c;
	void (*f)();

	f = signal(SIGALRM, sigALRM);
	timeout = 0;
	while (*s) {
		if (setjmp(timeoutbuf)) {
			biz22_abort();
			break;
		}
		alarm(number(value(DIALTIMEOUT)));
		read(FD, &c, 1);
		alarm(0);
		c &= 0177;
		if (c != *s++)
			return (0);
	}
	signal(SIGALRM, f);
	return (timeout == 0);
}
