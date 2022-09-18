#ifndef lint
static	char	*sccsid = "@(#)alarm.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
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
 *  15 Aug 89 -- gg							*
 *	Make alarm call to conform to posix. For large values		* 
 *	silently round it down to the maximum value.			*
 *	alarm() function should return unsigned int in POSIX mode	*
 *									*
 *  08 Feb 88 -- jmartin						*
 *	If SYSTEM_FIVE round up any non-zero fractional time to the	*
 *	next second.  (case ICA-9591)					*
 *									*
 *	David L Ballenger, 30-Mar-1985					*
 * 0001	Add defintions for System V compatibility.			*
 *									*
 ************************************************************************/

/*	alarm.c	4.1	83/06/10	*/

/*
 * Backwards compatible alarm.
 */
#include <sys/time.h>

#if !defined SYSTEM_FIVE && !defined POSIX
#define SECONDS int
#else   
#define SECONDS unsigned
#endif  

SECONDS
alarm(secs)
	SECONDS secs;
{
	struct itimerval it, oitv;
	register struct itimerval *itp = &it;

	timerclear(&itp->it_interval);
	if(secs > 100000000)
		secs = 100000000; /*silently truncate to the highest value */
	itp->it_value.tv_sec = secs;
	itp->it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return (-1);
#ifndef SYSTEM_FIVE
	return (oitv.it_value.tv_sec);
#else   SYSTEM_FIVE
	return (unsigned)oitv.it_value.tv_sec +
			(oitv.it_value.tv_usec ? 1 : 0);
#endif  SYSTEM_FIVE
}
