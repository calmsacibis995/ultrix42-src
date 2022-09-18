#ifndef lint
static	char	*sccsid = "@(#)df112.c	4.1	(ULTRIX)	7/17/90";
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
#include <sys/file.h>

jmp_buf df112Sjbuf;
df112timeout();


df112_dialer(num, acu)
	char *num, *acu;
{
	register int f = FD;
	struct sgttyb buf;
	int rw = 2;
	char c = '\0';
	int st = TIOCM_ST;	/* secondary Transmit flag */

	ioctl(f, TIOCHPCL, 0);		/* make sure it hangs up when done */
	if (setjmp(df112Sjbuf)) {
		ioctl(FD,TIOCNCAR); /* so writes wont hang if carrier never came up*/
		printf("connection timed out\r\n");
		df_disconnect();
		return (0);
	}
	if (boolean(value(VERBOSE)))
		printf("\ndialing...");
	fflush(stdout);
#ifdef TIOCMSET
	ioctl(f, TIOCGETP, &buf);
	if (buf.sg_ospeed != B1200) {	/* must dial at 1200 baud */
		buf.sg_ospeed = buf.sg_ispeed = B1200;
		ioctl(f, TIOCSETP, &buf);
		ioctl(f, TIOCMBIC, &st); /* clear ST for 300 baud */
	} else
		ioctl(f, TIOCMBIS, &st); /* set ST for 1200 baud */
#endif
	signal(SIGALRM, df112timeout);
	alarm(5 * strlen(num) + 10);
	ioctl(f, TIOCFLUSH, &rw);
 	/*cntrl A = burst mode, P = pulse dial */
	write(f, "\01", 1);
	write(f, "P", 1);
	write(f, num, strlen(num));
	write(f, "#", 1);
#ifdef ONDELAY
	/*
	 * At this point, wait for carrier; the ACU told
	 * us things were cool now let's see if this
	 * really is the case.
	 */
/*
	Should wait for "Attached" to be returned by modem but
	But the dmf32 discards incoming characters until carrier comes up.
	In order to make this work with all supported interfaces
	we just wait for carrier to come up.
*/
	ioctl(f, TIOCCAR);
	alarm(60);	/* Larry wants a better number */
	ioctl(f, TIOCWONLINE);	/* suspend, waiting for CD */
	alarm(0);
	return(1);
#else
	return (1); /* fail */
#endif
}

df112_disconnect()
{
	int rw = FREAD|FWRITE;

/*
	write(FD, "\002", 1);
*/
	sleep(1);
	ioctl(FD, TIOCFLUSH, &rw);
}


df112_abort()
{

	df112_disconnect();
}


static
df112timeout()
{

	longjmp(df112Sjbuf, 1);
}
