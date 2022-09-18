#ifndef lint
static char *sccsid = "@(#)tty_hc.c	4.1	(ULTRIX)	7/2/90";
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

#include "hc.h"

#if NHC > 0
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/hc.h"

/*
 * Line discipline for Berkeley network.
 *
 * This supplies single lines to a user level program
 * with a minimum of fuss.  Lines are newline terminated.
 *
 * This discipline requires that tty device drivers call
 * the line specific l_ioctl routine from their ioctl routines,
 * assigning the result to cmd so that we can refuse most tty specific
 * ioctls which are unsafe because we have ambushed the
 * teletype input queues, overlaying them with other information.
 */

/*
 * Open as networked discipline.  Called when discipline changed
 * with ioctl, this assigns a buffer to the line for input, and
 * changing the interpretation of the information in the tty structure.
 */
/*ARGSUSED*/


hcopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct buf *bp;

	if (tp->t_line == HCLDISC)
		return (EBUSY);	/* sometimes the network opens /dev/tty */
	bp = geteblk(HCBUFSIZ);
	ttyflush(tp, FREAD|FWRITE);
	tp->h_bufp = bp;
	tp->h_base = tp->h_out = tp->h_in = (char *)bp->b_un.b_addr;
	tp->h_top = tp->h_base + HCBUFSIZ;
	tp->h_inbuf = 0;
	tp->h_read = HCBUFSIZ;
#ifdef HCDEBUG
if (hcdebug)
	printf("tty_hc(%o).open h_read=%d, base=%o, top=%o, in=%o, out=%o, inbuf=%d\n",
		tp, tp->h_read, tp->h_base, tp->h_top, tp->h_in, tp->h_out, tp->h_inbuf);
#endif
	return (0);
}

/*
 * Break down... called when discipline changed or from device
 * close routine.
 */
hcclose(tp)
	register struct tty *tp;
{
	register int s;

	s = spltty();
	wakeup((caddr_t)&tp->t_rawq);
	tp->t_line = 0;		/* paranoid: avoid races */
	if (tp->h_bufp) {
		brelse(tp->h_bufp);
		tp->h_bufp = 0;
	} else
		printf("hcclose: no buf\n");
#ifdef HCDEBUG
	if (hcdebug)
		printf("tty_hc(%o).close\n", tp);
#endif
	splx(s);
}

/*
 * Read from a network line.
 * Characters have been buffered in a system buffer and are
 * now dumped back to the user in one fell swoop, and with a
 * minimum of fuss.  
 */
hcread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	register int s;
	int error;
	register len;
	register rest;

	if (tp->t_line != HCLDISC)
		return (-1);
	if ((tp->t_state&TS_CARR_ON)==0)
		return (-1);
	tp->h_read = uio->uio_iov->iov_len;
#ifdef HCDEBUG
	if (hcdebug)
		printf("tty_hc(%o).hcread before: h_read=%d\n", tp, tp->h_read);
#endif
	if(tp->h_read > HCBUFSIZ)
		return(-1);
	s = spltty();
	while (tp->h_read > tp->h_inbuf && tp->t_line == HCLDISC)
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	splx(s);
#ifdef HCDEBUG
	if (hcdebug)
		printf("tty_hc(%o).hcread after: h_inbuf=%d\n", tp, tp->h_inbuf);
#endif
	len = (int)(tp->h_top - tp->h_out);   
	if (len < tp->h_read) {
		/* data is wrapped around top of buffer */
#ifdef HCDEBUG
		if (hcdebug)
			printf("tty_hc(%o).hcread.1: h_out=%o, len=%d\n", tp, tp->h_out, len);
#endif
		error = uiomove(tp->h_out, len, UIO_READ, uio);
		tp->h_out = tp->h_base;
		rest = tp->h_read-len;
		error |= uiomove(tp->h_out, rest, UIO_READ, uio);
		tp->h_out += rest;
#ifdef HCDEBUG
		if (hcdebug)
			printf("tty_hc(%o).hcread.2: h_out=%o, rest=%d\n", tp, tp->h_out, rest);
#endif
	}
	else {
		/* data is not wrapped around top of buffer */
#ifdef HCDEBUG
		if (hcdebug)
			printf("tty_hc(%o).hcread.3: h_out=%o, len=%d\n", tp, tp->h_out, len);
#endif
		error = uiomove(tp->h_out, tp->h_read, UIO_READ, uio);
		tp->h_out += tp->h_read;
		if (tp->h_out >= tp->h_top) tp->h_out = tp->h_base;
	}
	s = spltty();
	tp->h_inbuf -= tp->h_read;
	splx(s);
	tp->h_read = HCBUFSIZ;
	return (error);
}

/*
 * Low level character input routine.
 * Stuff the character in the buffer, and wake up the top
 * half when the desired number of characters are read in (h_read) 
 * or if the buffer is (ick!) full.
 *
 * This rutine should be expanded in-line in the receiver
 * interrupt routine of the dh-11 to make it run as fast as possible.
 */
hcinput(c, tp)
register c;
register struct tty *tp;
{

	if ((tp->t_line != HCLDISC) || (tp->h_inbuf >= HCBUFSIZ))
		return;
	if (tp->h_in >= tp->h_top)
		tp->h_in = tp->h_base;
	*tp->h_in++ = c & 0377;
	if (++tp->h_inbuf >= HCBUFSIZ || tp->h_read <= tp->h_inbuf) {
		wakeup((caddr_t)&tp->t_rawq);
	}
}

/*
 * This routine is called whenever a ioctl is about to be performed
 * and gets a chance to reject the ioctl.  We reject all teletype
 * oriented ioctl's except those which set the discipline, and
 * those which get parameters (gtty and get special characters).
 */
/*ARGSUSED*/
hcioctl(tp, cmd, data, flag)
	struct tty *tp;
	caddr_t data;
{

	if ((cmd>>8) != 't')
		return (-1);
#ifdef HCDEBUG
	if (hcdebug)
		printf("tty_hc(%o).hcioctl cmd=%d\n", tp, cmd);
#endif
	switch (cmd) {

	case TIOCSETD:
	case TIOCGETD:
	case TIOCGETP:
	case TIOCGETC:
		return (-1);
	}
	return (ENOTTY);
}
#endif
