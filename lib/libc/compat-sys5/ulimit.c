#ifndef lint
static	char	*sccsid = "@(#)ulimit.c	4.1	(ULTRIX)	7/3/90";
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
 *	David L Ballenger, 31-May-1985					*
 * 002	Return newlimit if setrlimit() succeeds for cmd 2.		*
 *									*
 *	David L Ballenger, 29-Mar-1985					*
 * 0001	Use definitions from <sys/time.h> and <sys/resource.h> and real	*
 *	interfaces for getrlimit() and  setrlimit().			*
 *									*
 ************************************************************************/

/*
	ulimit -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

#include	<errno.h>
#include	<sys/time.h>
#include	<sys/resource.h>

extern int	getrlimit(), setrlimit();
extern int	errno;

long
ulimit( cmd, newlimit )
	int	cmd;			/* subcommand */
	long	newlimit;		/* desired new limit */
	{
	struct rlimit limit;		/* data being gotten/set */

	switch ( cmd )
		{
	case 1: 			/* get file size limit */
		if ( getrlimit( RLIMIT_FSIZE, &limit ) != 0 )
			return -1L;	/* errno is already set */
		return limit.rlim_max / 512L;

	case 2: 			/* set file size limit */
		limit.rlim_cur = limit.rlim_max = newlimit * 512L;
		if ( setrlimit( RLIMIT_FSIZE, &limit ) != 0 )
			return -1L;
		return newlimit ;

	case 3: 			/* get maximum break value */
		if ( getrlimit( RLIMIT_DATA, &limit ) != 0 )
			return -1L;	/* errno is already set */
		return limit.rlim_max;

	default:
		errno = EINVAL;
		return -1L;
		}
	}
