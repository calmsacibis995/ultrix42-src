#ifndef lint
static char *sccsid = "@(#)setbuffred.c	4.1	(ULTRIX)	7/2/90";
#endif lint


/************************************************************************
 *									*
 *			Copyright (c) 1988, 1990 by			*
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
 *
 * 02/07/90 GWS	replaced sgttyb / tty(4)-style terminal I/O with termio(4)-
 *		 style terminal I/O.  The replacement code is the formerly
 *		 conditionalized "USG" code.
 *
 */

/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

static short    baud_convert[] =
{
	0, 50, 75, 110, 135, 150, 200, 300, 600, 1200,
	1800, 2400, 4800, 9600, 19200, 38400
};

/*
 * Force output to be buffered.
 * Also figures out the baud rate.
 * Grouped here because they are machine dependent.
 */
_setbuffered(fd)
FILE *fd;
{
	char *sobuf;
	char *calloc();
	SGTTY   sg;

	sobuf = calloc(1, BUFSIZ);
	setbuf(fd, sobuf);

	ioctl (fileno (fd), TCGETA, &sg);
	SP->baud = sg.c_cflag&CBAUD ? baud_convert[sg.c_cflag&CBAUD] : 1200;
}
