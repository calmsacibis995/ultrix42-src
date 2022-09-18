#ifndef lint
static	char	*sccsid = "@(#)stime.c	4.1	(ULTRIX)	7/3/90";
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
 *	David L Ballenger, 29-Mar-1985					*
 * 0001	Use definitions from <sys/time.h> and real interfaces for	*
 *	gettimeofday() and settimeofday().				*
 *									*
 ************************************************************************/


/*
	stime -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/
#include <sys/time.h>
extern int	gettimeofday(), settimeofday();

int
stime( tp )
	long	*tp;			/* -> time to be set */
	{
	struct timeval	tv ;
	struct timezone	tz ;

	if ( gettimeofday( &tv, &tz ) != 0 )
		return -1;		/* "can't happen" */

	tv.tv_sec = *tp;
	tv.tv_usec = 0L;

	return settimeofday( &tv, &tz );
	}
