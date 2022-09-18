#ifndef lint
static	char	*sccsid = "@(#)setlinebuf.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/************************************************************************
 *			Modification History				*
 *									*
 *	David L Ballenger, 25-Jun-1985					*
 * 001	Move setlinebuf() into it's own file, and call setvbuf() to set *
 *	up the buffering information.					*
 *									*
 *	Based on:  setbuffer.c	4.2 (Berkeley) 2/27/83 			*
 *									*
 ************************************************************************/


#include	<stdio.h>

/*
 * set line buffering for either stdout or stderr
 */
#ifdef mips
int
#endif mips
#ifdef vax
void
#endif vax
setlinebuf(iop)
	FILE *iop;
{
	static char _sebuf[BUFSIZ];
	extern char _sobuf[];

	/* Make sure we have either stdout or stderr
	 */
	if (iop != stdout && iop != stderr)
		return;

	/* Flush any output.
	 */
	fflush(iop);

	/* Turn on line buffering.
	 */
	setvbuf( iop, (iop == stderr) ? _sebuf : _sobuf, _IOLBF, BUFSIZ);
}
