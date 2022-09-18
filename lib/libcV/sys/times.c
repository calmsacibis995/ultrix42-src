#ifndef lint
static	char	*sccsid = "@(#)times.c	4.1	(ULTRIX)	7/3/90";
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
 * 0001	Use definitions from <sys/time.h> and <sys/resource.h> and real	*
 *	interface for getrusage().					*
 *									*
 ************************************************************************/
/*
	times -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/times.h>
#include	<sys/resource.h>

extern int	getrusage();
extern long	time();

long
times( buffer )
	register struct tms	*buffer;	/* where to put info */
	{
	struct rusage ru;

	if ( getrusage( RUSAGE_SELF, &ru ) != 0 )	/* self */
		return -1L;			/* a mystery */
	buffer->tms_utime = ru.ru_utime.tv_sec * 60L
			  + ru.ru_utime.tv_usec * 60L / 1000000L;
	buffer->tms_stime = ru.ru_stime.tv_sec * 60L
			  + ru.ru_stime.tv_usec * 60L / 1000000L;

	if ( getrusage( RUSAGE_CHILDREN, &ru ) != 0 )	/* children */
		return -1L;			/* another mystery */
	buffer->tms_cutime = ru.ru_utime.tv_sec * 60L
			   + ru.ru_utime.tv_usec * 60L / 1000000L;
	buffer->tms_cstime = ru.ru_stime.tv_sec * 60L
			   + ru.ru_stime.tv_usec * 60L / 1000000L;

	return 60L * time( (long *)0 );
	}
