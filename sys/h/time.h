/*	@(#)time.h	4.2	(ULTRIX)	9/4/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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
 *      Dan Smith, 23-Feb-90                                            *
 * 0008 Added const to various function prototypes. [ANSI 4.12.1]       *
 *									*
 *	Jon Reeves, 07-Dec-1989						*
 * 0007	Namespace protection; also aligned clock_t, time_t with		*
 *	types.h.							*
 *									*
 *	Ken Lesniak, 03-Nov-1989					*
 * 0006	Changed conditionals around timezone and daylight for X/Open	*
 *									*
 *	Jon Reeves, 16-Jun-1989						*
 * 0005	size_t must be *un*signed.					*
 *									*
 *      Martin Hills, 12-Jun-1989					*
 * 0004 Added strftime() for X/Open conformance.			*
 *									*
 *	Jon Reeves, 30-May-1989						*
 * 0003	More POSIX conformance.  Changed include protection name.	*
 *									*
 *	Ken Lesniak, 03-Mar-1989					*
 * 0002	Made changes for POSIX 1003.1 and ANSI X3J11 conformance.	*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Add defintions for System V compatibility.			*
 *									*
 ************************************************************************/

#ifndef _TIME_H_
#define _TIME_H_

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#if !defined(_POSIX_SOURCE)
/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};
#define	DST_NONE	0	/* not on dst */
#define	DST_USA		1	/* USA style dst */
#define	DST_AUST	2	/* Australian style dst */
#define	DST_WET		3	/* Western European dst */
#define	DST_MET		4	/* Middle European dst */
#define	DST_EET		5	/* Eastern European dst */

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)	\
	((tvp)->tv_sec cmp (uvp)->tv_sec || \
	 (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
#define	ITIMER_REAL	0
#define	ITIMER_VIRTUAL	1
#define	ITIMER_PROF	2

struct	itimerval {
	struct	timeval it_interval;	/* timer interval */
	struct	timeval it_value;	/* current value */
};
#endif /* !defined(_POSIX_SOURCE) */

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	long	tm_gmtoff;
	char	*tm_zone;
};

#ifndef NULL
#define NULL 0
#endif

/*
 * The various time related types.
 */
#ifndef	_SIZE_T_
#define	_SIZE_T_
typedef	unsigned int	size_t;
#endif	/* _SIZE_T_ */

#ifndef _TIME_T_
#define _TIME_T_
typedef int time_t;
#endif /* _TIME_T_ */

#ifndef _CLOCK_T_
#define _CLOCK_T_
typedef int clock_t;
#endif /* _CLOCK_T_ */

/*
 * The number per second of the value returned by clock().
 */
#define CLOCKS_PER_SEC 1000000

#if defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
/*
 * The unit of times, et al (POSIX); also defined in limits.h; spacing
 * must match.
 */
#define CLK_TCK 60 
#endif

#ifndef KERNEL
#ifdef __STDC__
/*
 * prototypes
 */
extern char *		asctime( const struct tm *__tm );
extern clock_t		clock( void );
extern char *		ctime( const time_t *__clock ); 
extern double		difftime( time_t __time1, time_t __time0 );
extern struct tm *	gmtime( const time_t *__clock );
extern struct tm *	localtime( const time_t *__clock );
extern time_t		mktime( struct tm *__timeptr );
extern size_t		strftime( char *__s, size_t __maxsize,
				const char *__format, const struct tm *__tm );
extern time_t		time( time_t *__tloc );	

#else
extern	struct tm *gmtime(), *localtime();
extern	char *asctime(), *ctime();
extern	size_t strftime();
extern	void tzset();
extern	clock_t clock();
extern	time_t time(), mktime();
extern	double difftime();
#endif /* __STDC__ */

extern	char *tzname[];
/*
 * Conditionalize definition of timezone and add SYSTEM V definitions
 */
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#if !defined(__SYSTEM_FIVE) && !defined(_XOPEN_SOURCE)
extern	char *timezone();
#else
extern	long timezone;
extern	int daylight;
#endif
#endif
#endif  /* KERNEL */

#endif  /* _TIME_H_ */
