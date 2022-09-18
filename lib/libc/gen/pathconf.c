#ifndef lint
static	char	*sccsid = "@(#)pathconf.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987, 1988 by			*
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
 *	Jon Reeves, 31-Aug-1989
 * 004	Fix error checking; delete meaningless comment
 *
 *	Mark A. Parenti, 25-Aug-1988
 * 003	Remove numerous options that were deleted from the final
 *	POSIX standard.
 *
 *	Mark A. Parenti, 11-July-1988
 * 002	Add error checking from POSIX 12.3
 *
 * 	Mark A Parenti, 09-Oct-1987
 * 001	Original version for POSIX
 *
 ************************************************************************/

#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

extern int errno;
static long pathopts[] =
	{
		-1,		/* Invalid option */
		LINK_MAX,
		MAX_CANON,
		MAX_INPUT,
		NAME_MAX,
		PATH_MAX,
		PIPE_BUF,
		1,		/* _POSIX_CHOWN_RESTRICTED */
		1,		/* _POSIX_NO_TRUNC	*/
		_POSIX_VDISABLE /* _POSIX_VDISABLE	*/
	};
		
long
pathconf(path, name)
	char *path;
	int name;
{
	if ( (name <= 0) || (name >= sizeof(pathopts)/sizeof(pathopts[0])) ) {
		errno = EINVAL;
		return(-1);
	}
	return( pathopts[name] );
	
}

long
fpathconf(fildes, name)
	int fildes;
	int name;
{
	if ( (name <= 0) || (name >= sizeof(pathopts)/sizeof(pathopts[0])) ) {
		errno = EINVAL;
		return(-1);
	}
	return( pathopts[name] );
	
}
