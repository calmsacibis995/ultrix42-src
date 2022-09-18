#ifndef lint
static	char	*sccsid = "@(#)sysconf.c	4.2	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987,1988,1989 by			*
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
 * 	Mark A. Parenti, 22-Mar-1988
 * 002	Modify to use getsysinfo() for child max(max uprocs).
 *
 *	Mark A. Parenti, 11-July-1988
 * 003	Add error checking from POSIX 12.3
 *
 *	Mark A. Parenti, 25-Aug-1988
 * 004	Remove numerous options that were deleted from the final
 *	POSIX standard.
 *
 *	Jon Reeves, 18-Jul-1989
 * 005	Add XOPEN_VERSION, PASS_MAX for X/Open Portability Guide v. 3.
 *
 *	Jon Reeves, 31-Aug-1989
 * 006	Fix limits checking
 *
 *	Jon Reeves, 16-Jan-1990
 * 007	Add time.h for CLK_TCK, with enabling #define
 *
 *	Paul Shaughnessy, 28-Feb-1991
 *	Return getdtablesize() instead of OPEN_MAX. This
 *	is required if the default number of open file
 *	descriptors increases.
 *
 ************************************************************************/

#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <errno.h>
#define _XOPEN_SOURCE /* turn on CLK_TCK */
#include <time.h>

extern int errno;
static long sysopts[] =
	{
		-1,		/* Invalid option */
	 	ARG_MAX,	
		CHILD_MAX,
		CLK_TCK,
		NGROUPS_MAX,
		_POSIX_OPEN_MAX,
		_POSIX_JOB_CONTROL,
		_POSIX_SAVED_IDS,
		_POSIX_VERSION,
		_XOPEN_VERSION,
		PASS_MAX,
	};
		
long
sysconf(name)
	int name;
{
	long	retval;

	if ( (name <= 0) || (name >= sizeof(sysopts)/sizeof(sysopts[0])) ) {
		errno = EINVAL;
		return(-1);
	}
	switch( name ) {
	case _SC_CHILD_MAX:
		if( getsysinfo(GSI_MAX_UPROCS, &retval, sizeof(long), 0, 0, 0) == 0)
			retval = sysopts[name];
		break;
	case _SC_OPEN_MAX:
		if( (retval = getdtablesize()) == 0)
			retval = sysopts[name];
		break;

	default:
		retval =  sysopts[name];	
		break;
	}
	return(retval);	
}
