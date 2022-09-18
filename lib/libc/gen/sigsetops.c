#ifndef lint
static	char	*sccsid = "@(#)sigsetops.c	4.1	(ULTRIX)	7/3/90";
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
 *	Jon Reeves, 04-Nov-88
 * 003	Change all bits to the clearer/more portable ~0 (from -1).
 *
 *	Mark A. Parenti, 14-Jul-1988
 * 002	Change siginitset() to sigemptyset().
 *	Turn on/off ALL signals, not just POSIX signals.
 *
 * 	Mark A Parenti, 09-Oct-1987
 * 001	Original version for POSIX
 *
 ************************************************************************/

#include <errno.h>
#include <signal.h>


extern int errno;
/*
 * The following are the POSIX sigsetops functions.  These functions
 * primitives manipulate signal sets.  They operate on data objects
 * addressable by the application, not on any set of signals known to the
 * system, such as the set block from delivery to a process.
 */

/*
 * Initialize signal set such that all POSIX signals are excluded.
 */
sigemptyset(set)
	sigset_t *set;
{
	*set = 0;
	return(0);
}
/*
 * Initialize signal set such that all POSIX signals are included.
 */
sigfillset(set)
	sigset_t *set;
{
	*set = ~0;
	return(0);
}

/*
 * Add signal to signal set
 */
int
sigaddset(set, signo)
	sigset_t *set;
	int	 signo;
{
	if (signo <= 0 || signo > NSIG) {
		errno = EINVAL;
		return(-1);
	}
	*set |= sigmask(signo);
	return(0);
}
/*
 * Delete signal from signal set
 */
int
sigdelset(set, signo)
	sigset_t *set;
	int	 signo;
{
	if (signo <= 0 || signo > NSIG) {
		errno = EINVAL;
		return(-1);
	}
	*set &= ~sigmask(signo);
	return(0);
}
/*
 * Test if signal is member of sigset
 */
int
sigismember(set, signo)
	sigset_t *set;
	int	 signo;
{
	if (signo <= 0 || signo > NSIG) {
		errno = EINVAL;
		return(-1);
	}
	if (*set & sigmask(signo))
		return(1);
	else
		return(0);
}
