/*
 * hunt.c
 */
#ifndef lint
static	char	*sccsid = "@(#)hunt.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1983,1985 by                      *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   This software is  derived  from  software  received  from  the     *
 *   University    of   California,   Berkeley,   and   from   Bell     *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to     *
 *   restrictions  under  license  agreements  with  University  of     *
 *   California and with AT&T.                                          *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

/************************************************************************
	Edit History:

	29-OCT-85 : marc
		Rewrite of hunt() to make fixes to support line turnaround.

	TODO:
		Log failure codes of the open's and ioctl's to make
		life easier for the support people.  The format of the
		aculog file should be changed to allow more information
		to be logged - however we must know what programs access 
		aculog which may be affected. - marc

************************************************************************/
#include "tip.h"

extern char *getremote();
extern char *rindex();
extern int  errno;

hunt(name)
	char *name;
{
	register char *cp;
	int (*f)();
	int temp = 0;

	while (cp = getremote(name)) {
		uucplock = rindex(cp, '/')+1;
		if (mlock(uucplock) < 0) {
			delock(uucplock);
			continue;
		}
		/*
		 * Straight through call units, such as the BIZCOMP,
		 * VADIC and the DF, must indicate they're hardwired in
		 *  order to get an open file descriptor placed in FD.
		 * Otherwise, as for a DN-11, the open will have to
		 *  be done in the "open" routine.
		 *
		 * The latter comment is true.  But if we indicate in
		 * /etc/remote that the site is a "dialed up" site
		 * then DU will be set.  Therefore the condition:
		 *              DU && HW
		 * implies that the device is a straight through call unit.
		 *                                      -Larry
		 */
		if (!HW)
			break;
		if (DU) {
			/*
			 * Integral ACU/modem dialers (straight through)
			 */
			if ((FD = open(cp, O_RDWR|O_NDELAY)) < 0) {
				delock(uucplock);
				continue;
			}
			if (ioctl(FD, TIOCSINUSE) < 0) {
				/* This device is already being used - skip */
				close(FD);
				delock(uucplock);
				continue;
			}
			ioctl(FD, TIOCMODEM, &temp);
			ioctl(FD, TIOCNCAR);  /* ignore carrier */
			ioctl(FD, TIOCEXCL, 0);
			ioctl(FD, TIOCHPCL, 0);
			signal(SIGALRM, SIG_DFL);
			return((int)cp);
		}
		else {
			/*
			 *  DN11's  & HW lines
			 */
			if ((FD = open(cp, O_RDWR|O_NDELAY)) < 0) {
				delock(uucplock);
				continue;
			}
			if (ioctl(FD, TIOCSINUSE) < 0){
				/* This device is already being used - skip */
				close(FD);
				delock(uucplock);
				continue;
			}
			/* If MD is set then look at modem signals */
			if(MD)
				ioctl(FD,TIOCMODEM, &temp);
			else
				ioctl(FD, TIOCNMODEM, &temp);
			ioctl(FD, TIOCEXCL, 0);
			ioctl(FD, TIOCHPCL, 0);
			signal(SIGALRM, SIG_DFL);
			return((int)cp);
		}
	}
	/* If we return here then no lines were found available */
	signal(SIGALRM, f);
	return ((int)cp);
}
