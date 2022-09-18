#ifndef lint
static	char	*sccsid = "@(#)nice.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *	G Gopal, 28-Sep-1989
 * 0002 In posix mode return the new nice value.
 *
 *	David L Ballenger, 30-Mar-1985					*
 * 0001	Add defintions for System V compatibility.			*
 *									*
 ************************************************************************/

/*	nice.c	4.1	83/05/30	*/

#include <sys/exec.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

/*
 * Backwards compatible nice.
 */
nice(incr)
	int incr;
{
	int prio, retval;
	short progenv;
	extern int errno;

	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (prio == -1 && errno)
		return (-1);

	prio += incr ; /* Assumed new priority */

	if( getsysinfo(GSI_PROG_ENV, &progenv, sizeof(int), 0, 0, 0) < 1 )
		progenv = A_BSD;
#ifndef SYSTEM_FIVE
	/*
	 * On ULTRIX just return value from setpriority
	 */ 
	retval = setpriority(PRIO_PROCESS, 0, prio);
	if((progenv == A_POSIX) && (retval == 0)) /* success */
		return(getpriority(PRIO_PROCESS,0));	
	return (retval);

#else   SYSTEM_FIVE
	/*
	 * For System V  make sure the assumed priority is in the
	 * correct range, and return it if set priority succeeds
	 */
	if (prio < -20)
		prio = -20;
	else if (prio > 19)
		prio = 19;
	return(setpriority(PRIO_PROCESS, 0, prio) == 0 ? prio : -1);
#endif SYSTEM_FIVE
}
