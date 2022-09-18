#ifndef lint
static	char	*sccsid = "@(#)kill.c	4.1	(ULTRIX)	7/3/90";
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
 *	David L Ballenger, 29-Mar-1985					*
 * 0001	Use real killpg() interface.					*
 *									*
 ************************************************************************/

/*
	kill -- system call emulation for 4.2BSD

	last edit:	02-Jan-1985	D A Gwyn
*/

#include	<errno.h>
#include	<sys/signal.h>

extern int	_kill(), killpg(), getpid();

int
kill( pid, sig )
	register int	pid;		/* process ID or special code */
	register int	sig;		/* signal to be sent */
	{
	register int	retval;		/* function return value */

	if ( sig < 0 /* || sig > NSIG */
	  || sig == SIGKILL && pid == 1	/* undocumented kernel check */
	   )	{
		errno = EINVAL;		/* kernel would've allowed it */
		return -1;
		}

	if ( pid < -1 )
		return killpg( -pid, sig );
	else	if ( (retval = _kill( pid, sig )) == 0 && pid == -1 )
			return _kill( getpid(), sig );
		else
			return retval;

	/* The case PID == -1 and UID != 0 is not handled right, since
	   it would require testing the UID of all running processes. */
	}
