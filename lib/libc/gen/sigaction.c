#ifndef lint
static	char	*sccsid = "@(#)sigaction.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 *			Modification History
 *
 * 	Mark A Parenti, 09-Oct-1987
 * 001	Original version for POSIX
 *
 *	Mark A. Parenti, 10-Feb-1988
 * 002  Only set SV_INTERRUPT if act structure is non-null
 *
 ************************************************************************/

#include <signal.h>

/*
 * POSIX version of sigvec
 * 
 */
sigaction(sig, act, oact)
	int sig;
	struct sigaction *act, *oact;
{
	if( act != (struct sigaction *)0)
		act->sa_flags |= SV_INTERRUPT;	/* Always want to be interrupted */
	return( sigvec(sig, act, oact) );

}
