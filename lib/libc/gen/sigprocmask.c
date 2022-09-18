#ifndef lint
static	char	*sccsid = "@(#)sigprocmask.c	4.1	(ULTRIX)	7/3/90";
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
 ************************************************************************/

#include <signal.h>
#include <errno.h>

#define	NULL	0
extern	int	errno;

/*
 * POSIX version of sigblock/sigsetmask
 * 
 */
sigprocmask(how,set,oset)
	int how;
	sigset_t *set, *oset;
{
	sigset_t curmask;

	curmask = sigblock(0);
	if(oset != NULL)
		*oset = curmask;

	if(set == NULL)
		return(0);

	switch(how){

	case(SIG_BLOCK):

		(void)sigblock(*set);
		break;

	case(SIG_UNBLOCK):

		curmask &= ~*set;
		(void)sigsetmask(curmask);
		break;

	case(SIG_SETMASK):

		(void)sigsetmask(*set);
		break;

	default:
		errno = EINVAL;
		return(-1);
	}
	return(0);
}
