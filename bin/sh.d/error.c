#ifndef lint
static char sccsid[] = "@(#)error.c	4.2 (ULTRIX) 8/13/90";
/* Original ID:  "@(#)error.c	4.2 8/11/83" */
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * ~~~~~~~~~~~~~~~~~~~~
 * 02	David Lindner Tue Jun 12 14:12:55 EDT 1990
 *	- Added check for traps in done routine.
 *
 * 01 	David Lindner Mon Jul 24 14:20:37 EDT 1989
 *    	- Fixed badfilename bug with error message, and added comment 
 *    	- header.
 *
 */

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	error handling	======== */

exitset()
{
	assnum(&exitadr,exitval);
}

sigchk()
{
	/* Find out if it is time to go away.
	 * `trapnote' is set to SIGSET when fault is seen and
	 * no trap has been set.
	 */
	IF trapnote&SIGSET
	THEN	exitsh(SIGFAIL);
	FI
}

failed(s1,s2)
	STRING	s1, s2;
{
	prp2(); prs(s1); 	/* DJL 01 */
	IF s2
	THEN	prs(colon); prs(s2);
	FI
	newline(); exitsh(ERROR);
}

error(s)
	STRING	s;
{
	failed(s,NIL);
}

exitsh(xno)
	INT	xno;
{
	/* Arrive here from `FATAL' errors
	 *  a) exit command,
	 *  b) default trap,
	 *  c) fault with no trap set.
	 *
	 * Action is to return to command level or exit.
	 */
	exitval=xno;
	IF (flags & (forked|errflg|ttyflg)) != ttyflg
	THEN	done();
	ELSE	clearup();
		longjmp(errshell,1);
	FI
}

done()
{
	REG STRING	t;
	if (t=trapcom[0])
	{
		trapcom[0]=0; /*should free but not long */
		execexp(t,0);
	}
	else
		chktrap();	/* DJL 02 */

	rmtemp(0);
	exit(exitval);
}

rmtemp(base)
	IOPTR		base;
{
	WHILE iotemp>base
	DO  unlink(iotemp->ioname);
	    iotemp=iotemp->iolst;
	OD
}
