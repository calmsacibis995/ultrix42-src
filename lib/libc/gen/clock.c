#ifndef lint
static	char	*sccsid = "@(#)clock.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987, 1989 by		*
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
 *	Ken Lesniak, 21-Jul-1989					*
 * 0004	Return 0 on first call to clock to satisify X/Open.		*
 *									*
 *	Ken Lesniak, 01-Jun-1989					*
 * 0003	Re-wrote to use getrusage() and conform to ANSI: Overflow	*
 *	checking is now done, and system call failures return -1	*
 *	instead of 0.							*
 *									*
 *	jaw - 09-DEC-1987						*
 *	On large machines, no time will have accumulated on the 	*
 *	process when first call so "first" will not get set up 		*
 *	properly.  The fix is to set first to -1 and allow 0 to be	*
 * 	a valid amount of time.						*
 *									*
 *	David L Ballenger, 16-Apr-1985					*
 * 0001	Define HZ here instead of getting it from <sys/param.h>.  The	*
 *	real HZ value for the system but this corresponds to the usage	*
 *	in the times call.  This routine should be cleaned up to use	*
 *	getrusage() instead of times().					* 
 *									*
 ************************************************************************/

/*LINTLIBRARY*/

#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

/* The following variables are used to hold the amount of CPU time */
/* used up to the point of the first call to clock() */

static long		first_min = -1;
static long		first_sec = 0;
static long		first_usec = 0;

/* The following macros, define the maximum value that each time */
/* component can have, before overflowing, when converted to a */
/* clock_t value. */

#define min_MAX (LONG_MAX / (1000000 * 60))
#define sec_MAX ((LONG_MAX / 1000000) % 60)
#define usec_MAX (LONG_MAX % 1000000)

/*
 * clock() returns the amount of CPU time (in microseconds) used since
 * the first call to clock().
 *
 * If the CPU time is not available (i.e. a system call failure), or
 * its value cannot be represented (i.e. the value is too big for a
 * clock_t value), (clock_t)-1 is returned.
 *
 * In order to do the overflow checking, the time values are divided
 * into components, with enough extra bits, to guarantee that
 * the components will not overflow during the calculations. Please
 * be careful when making changes, so that no overflow occurs
 * during the calculations.
 */

clock_t clock()
    {
	struct rusage	ru;
	register long	min, sec, usec;

	/* Get the amount of time this process has used */

	if (getrusage(RUSAGE_SELF, &ru) != 0)
	    return (clock_t)-1;
	min = (ru.ru_utime.tv_sec / 60) + (ru.ru_stime.tv_sec / 60);
	sec = (ru.ru_utime.tv_sec % 60) + (ru.ru_stime.tv_sec % 60);
	usec = ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;

	/* Add on the amount of time used by any finished children */

	if (getrusage(RUSAGE_CHILDREN, &ru) != 0)
	    return (clock_t)-1;
	min += (ru.ru_utime.tv_sec / 60) + (ru.ru_stime.tv_sec / 60);
	sec += (ru.ru_utime.tv_sec % 60) + (ru.ru_stime.tv_sec % 60);
	usec += ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;

	/* Get the amount of time used since the first call to clock() */
	/* The constants are a trick to keep the components from going */
	/* negative, which would complicate the normalization. The */
	/* constants were chosen so that the value will not need any */
	/* additional correction after it is normalized */

	usec += 1000000 - first_usec;
	sec += 59 - first_sec;
	min += -1 - first_min;

	/* Normalize the time components so we can check for overflow */

	sec += usec / 1000000;
	usec %= 1000000;
	min += sec / 60;
	sec %= 60;

	/* If this was the first call to clock(), compensate for */
	/* the initial value of first_min, and save the time for */
	/* subsequent calls */

	if (first_min < 0) {
	    first_min = --min;
	    first_sec = sec;
	    first_usec = usec;

	    return (clock_t)0;
	};
		
	/* Verify that the time components can be combined into */
	/* a valid clock_t value */

	if (min > min_MAX || (min == min_MAX && (sec > sec_MAX ||
	    (sec == sec_MAX && usec >= usec_MAX)))
	)
	    return (clock_t)-1;

	/* Build the clock_t value and return it */

	return (clock_t)((((min * 60) + sec) * 1000000) + usec);
    }
