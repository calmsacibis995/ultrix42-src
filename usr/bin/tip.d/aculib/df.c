#ifndef lint
static	char	*sccsid = "@(#)df.c	4.1	(ULTRIX)	7/17/90";
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
 * Dial the DF02-AC or DF03-AC
 */

#include "tip.h"

jmp_buf dfSjbuf;

df02_dialer(num, acu)
	char *num, *acu;
{

	return (df_dialer(num, acu, 0));
}

df03_dialer(num, acu)
	char *num, *acu;
{

	return (df_dialer(num, acu, 1));
}

df_dialer(num, acu, df03)
	char *num, *acu;
	int df03;
{
	extern int dftimeout();
	register int f = FD;
	struct sgttyb buf;
	int speed = 0, rw = 2;
	char c = '\0';

	ioctl(f, TIOCHPCL, 0);		/* make sure it hangs up when done */
	if (setjmp(dfSjbuf)) {
		ioctl(FD,TIOCNCAR); /* so writes wont hang if carrier never came up*/
		printf("connection timed out\r\n");
		df_disconnect();
		return (0);
	}
	if (boolean(value(VERBOSE)))
		printf("\ndialing...");
	fflush(stdout);
#ifdef TIOCMSET
	if (df03) {
		int st = TIOCM_ST;	/* secondary Transmit flag */

		ioctl(f, TIOCGETP, &buf);
		if (buf.sg_ospeed != B1200) {	/* must dial at 1200 baud */
			speed = buf.sg_ospeed;
			buf.sg_ospeed = buf.sg_ispeed = B1200;
			ioctl(f, TIOCSETP, &buf);
			ioctl(f, TIOCMBIC, &st); /* clear ST for 300 baud */
		} else
			ioctl(f, TIOCMBIS, &st); /* set ST for 1200 baud */
	}
#endif
	signal(SIGALRM, dftimeout);
	alarm(5 * strlen(num) + 10);
	ioctl(f, TIOCFLUSH, &rw);
	write(f, "\001", 1);
	sleep(1);
	write(f, "\002", 1);
	write(f, num, strlen(num));
	read(f, &c, 1);
#ifdef TIOCMSET
	if (df03 && speed) {
		buf.sg_ispeed = buf.sg_ospeed = speed;
		ioctl(f, TIOCSETP, &buf);
	}
#endif
#ifdef ONDELAY
	/*
	 * At this point, wait for carrier; the ACU told
	 * us things were cool now let's see if this
	 * really is the case.
	 */
/*
	To make at least one dialer work with a dmf32.
	The dmf32 discards incoming characters until carrier comes up.
	if (c != 'A')
		return(0);
*/
	ioctl(f, TIOCCAR);
	alarm(40);	/* Larry wants a better number */
	ioctl(f, TIOCWONLINE);	/* suspend, waiting for CD */
	alarm(0);
	return(1);
#else
	return (c == 'A');
#endif
}

df_disconnect()
{
	int rw = 2;

	write(FD, "\001", 1);
	sleep(1);
	ioctl(FD, TIOCFLUSH, &rw);
}


df_abort()
{

	df_disconnect();
}


dftimeout()
{

	longjmp(dfSjbuf, 1);
}
