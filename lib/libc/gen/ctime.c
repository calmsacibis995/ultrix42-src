/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char	*sccsid = "@(#)ctime.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * 004	Jon Reeves, 20-Nov-1989
 *	Added _last_tz_set logic: effectively do tzset every time, not
 *	just first, since environment may have changed.  Still leaves a
 *	little acceleration in place.  Believe user's tm_isdst for
 *	mktime.
 *
 * 003	Ken Lesniak, 31-Aug-1989
 *	Modified mktime to correctly handle all times not representable
 *
 * 002	Ken Lesniak, 17-Jul-1989
 *	Moved tz_is_set flag to tzset.c and actually set flag in tzset()
 *
 * 001	Ken Lesniak, 20-Mar-1989
 *	Added support for POSIX 1003.1 compliancy.
 *	Added mktime and difftime routines for ANSI X3J11.
 *	Moved tzset* routines to tzset.c.
 *
 *	Based on Berkeley ctime.c 1.2
 ************************************************************************/

#include <sys/time.h>
#include <tzfile.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "tzs.h"

char *
ctime(t)
time_t *t;
{
	return(asctime(localtime(t)));
}

static char *
itoa2(cp, n)
register char *cp;
int	n;
{
	if (n >= 10)
		*cp++ = (n / 10) % 10 + '0';
	else
		*cp++ = ' ';
	*cp++ = n % 10 + '0';
	return cp;
}

char *
asctime(t)
register struct tm *	t;
{
	static char	wday_name[DAYS_PER_WEEK][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static char	mon_name[MONS_PER_YEAR][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char	result[26];
	register char	*cp;
	register char	*ncp;
	register int	year;

	cp = result;
	ncp = wday_name[t->tm_wday];
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;

	*cp++ = ' ';
	ncp = mon_name[t->tm_mon];
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;

	*cp++ = ' ';
	cp = itoa2(cp, t->tm_mday);

	*cp++ = ' ';
	cp = itoa2(cp, t->tm_hour + 100);

	*cp++ = ':';
	cp = itoa2(cp, t->tm_min + 100);

	*cp++ = ':';
	cp = itoa2(cp, t->tm_sec + 100);

	*cp++ = ' ';
	year = TM_YEAR_BASE + t->tm_year;
	cp = itoa2(cp, year / 100);
	cp = itoa2(cp, year);

	*cp++ = '\n';
	*cp++ = '\0';

	return result;
}

struct tm *		offtime();

extern struct state	_tzs;

extern int		_tz_is_set;
extern char 		_last_tz_set[LAST_TZ_LEN];

struct tm *
localtime(timep)
time_t *	timep;
{
	register struct ttinfo *	ttisp;
	register struct tm *		tmp;
	register int			i;
	time_t				t;
	char *				tz;

	if (!_tz_is_set || (tz=getenv("TZ")) && strcmp(tz, _last_tz_set))
		(void) tzset();

	t = *timep;
	if (_tzs.timecnt == 0 || t < _tzs.ats[0]) {
		i = 0;
		while (_tzs.ttis[i].tt_isdst)
			if (++i >= _tzs.timecnt) {
				i = 0;
				break;
			}
	} else {
		for (i = 1; i < _tzs.timecnt; ++i)
			if (t < _tzs.ats[i])
				break;
		i = _tzs.types[i - 1];
	}
	ttisp = &_tzs.ttis[i];
	/*
	** To get (wrong) behavior that's compatible with System V Release 2.0
	** you'd replace the statement below with
	**	tmp = offtime((time_t) (t + ttisp->tt_gmtoff), 0L);
	*/
	tmp = offtime(&t, ttisp->tt_gmtoff);
	tmp->tm_isdst = ttisp->tt_isdst;
	tzname[tmp->tm_isdst] = &_tzs.chars[ttisp->tt_abbrind];
	tmp->tm_zone = &_tzs.chars[ttisp->tt_abbrind];
	return tmp;
}

struct tm *
gmtime(clock)
time_t *	clock;
{
	register struct tm *	tmp;

	tmp = offtime(clock, 0L);
	tzname[0] = "GMT";
	tmp->tm_zone = "GMT";		/* UCT ? */
	return tmp;
}

static int	mon_lengths[2][MONS_PER_YEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

struct tm *
offtime(clock, offset)
time_t *	clock;
long		offset;
{
	register struct tm *	tmp;
	register long		days;
	register long		rem;
	register int		y;
	register int		yleap;
	register int *		ip;
	static struct tm	tm;

	tmp = &tm;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	rem += offset;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = isleap(y);
			if (days < (long) year_lengths[yleap])
				break;
			++y;
			days -= (long) year_lengths[yleap];
		}
	else do {
		--y;
		yleap = isleap(y);
		days += (long) year_lengths[yleap];
	} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days -= (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
	tmp->tm_zone = "";
	tmp->tm_gmtoff = offset;
	return tmp;
}


double difftime(time1, time0)
	time_t		time1;
	time_t		time0;
    {
	return (double)time1 - (double)time0;
    }


/*
 * The following types and routines are provided so that mktime() can
 * use quadword math as it combines the broken-down time into
 * a time_t value. mktime() does not impose any bounds on the broken-down
 * time components, without the quadword math, all sorts of overflows
 * are possible.
 *
 * The routines are not general purpose quadword routines. The types
 * of the operands are restricted, to allow shortcuts in the algorthims.
 *
 * I know this is ugly, I can't think of any other way to allow mktime
 * to handle all broken-down times that can represent a valid time_t.
 */

typedef union {
	unsigned long	lw[2];
	unsigned short	w[4];
    } quadword;

typedef union {
	unsigned long	lw;
	unsigned short	w[2];
    } longword;

#define ULONG unsigned long

/* quadword(qw) = quadword(qw) + long(op) */

void qadd(qw, op)
	register quadword *qw;
	long		op;
    {
	longword	ac;
	longword	sext;
	longword	lw_op;

	sext.lw = 0;
	if (op < 0)
	    sext.lw = (unsigned long)-1;

	lw_op.lw = (ULONG)op;

	qw->w[0] = (ac.lw = (ULONG)qw->w[0] + lw_op.w[0]);
	qw->w[1] = (ac.lw = (ULONG)qw->w[1] + lw_op.w[1] + (ULONG)ac.w[1]);
	qw->w[2] = (ac.lw = (ULONG)qw->w[2] + (ULONG)sext.w[0] + (ULONG)ac.w[1]);
	qw->w[3] += (ULONG)sext.w[1] + (ULONG)ac.w[1];
    };

/* quadword(qw) = (quadword(qw) * unsigned short(mop)) + long(aop) */

void qmuladd(qw, mop, aop)
	register quadword *qw;
	unsigned short	mop;
	long		aop;
    {
	longword	ac;

	qw->w[0] = (ac.lw = (ULONG)qw->w[0] * (ULONG)mop);
	qw->w[1] = (ac.lw = ((ULONG)qw->w[1] * (ULONG)mop) + ac.w[1]);
	qw->w[2] = (ac.lw = ((ULONG)qw->w[2] * (ULONG)mop) + ac.w[1]);
	qw->w[3] = (qw->w[3] * mop) + ac.w[1];

	qadd(qw, aop);
    }

time_t mktime(tp)
	register struct tm *tp;
    {
	time_t		t;
	quadword	tq;
	int		y;
	int		m;
	int		ly;
	int		isdst;
	int		i;
	struct ttinfo	*ttis;
	char		*tz;

#define mktime_max_year (INT_MAX + (INT_MIN / MONS_PER_YEAR) + \
	(INT_MIN / DAYS_PER_NYEAR) + \
	(INT_MIN / (HOURS_PER_DAY * DAYS_PER_NYEAR)) + \
	(INT_MIN / (MINS_PER_HOUR * HOURS_PER_DAY * DAYS_PER_NYEAR)) + \
	(INT_MIN / (SECS_PER_MIN * MINS_PER_HOUR * HOURS_PER_DAY * DAYS_PER_NYEAR)) + \
	TM_YEAR_BASE)
#define mktime_min_year (INT_MIN + (INT_MAX / MONS_PER_YEAR) + \
	(INT_MAX / DAYS_PER_NYEAR) + \
	(INT_MAX / (HOURS_PER_DAY * DAYS_PER_NYEAR)) + \
	(INT_MAX / (MINS_PER_HOUR * HOURS_PER_DAY * DAYS_PER_NYEAR)) + \
	(INT_MAX / (SECS_PER_MIN * MINS_PER_HOUR * HOURS_PER_DAY * DAYS_PER_NYEAR)) + \
	TM_YEAR_BASE)

	/* Make sure the year is in bounds to avoid overflowing the */
	/* month-year normalization */

	if (tp->tm_year < mktime_min_year || tp->tm_year > mktime_max_year)
	    return (time_t)-1;

	/* Get normalized month and year */

	y = tp->tm_year + TM_YEAR_BASE + (tp->tm_mon / MONS_PER_YEAR);
	if ((m = tp->tm_mon % MONS_PER_YEAR) < 0) {
	    m += MONS_PER_YEAR;
	    y--;
	};
	ly = isleap(y);

	/* Determine the number of days since the epoch */

	tq.lw[0] = tq.lw[1] = -1;
	qadd(&tq, tp->tm_mday);
	while (m != 0)
	    qadd(&tq, mon_lengths[ly][--m]);

	if (y >= EPOCH_YEAR) {
	    while (y != EPOCH_YEAR) {
		y--;
		qadd(&tq, year_lengths[isleap(y)]);
	    };
	} else {
	    do {
		qadd(&tq, -year_lengths[isleap(y)]);
		y++;
	    } while (y != EPOCH_YEAR);
	};
	
	/* Now convert everything to seconds */

	qmuladd(&tq, HOURS_PER_DAY, tp->tm_hour);
	qmuladd(&tq, MINS_PER_HOUR, tp->tm_min);
	qmuladd(&tq, SECS_PER_MIN, tp->tm_sec);

	/* Make sure the resulting time can be represented by a time_t */

	if (tq.lw[1] != 0 && tq.lw[1] != (unsigned long)-1)
	    return (time_t)-1;

	t = (time_t)tq.lw[0];

	/* Determine the GMT offset for the time */

	if (!_tz_is_set || (tz=getenv("TZ")) && strcmp(tz, _last_tz_set))
	    (void)tzset();

	ttis = &_tzs.ttis[0];
	if (tp->tm_isdst < 0) {
	    if (_tzs.timecnt != 0 && t >= _tzs.ats[0] + ttis->tt_gmtoff) {
		for (i = 1; i < _tzs.timecnt; i++) {
		    ttis = &_tzs.ttis[_tzs.types[i - 1]];
		    if (t < _tzs.ats[i - 1] + ttis->tt_gmtoff)
			return (time_t)-1;
		    if (t < _tzs.ats[i] + ttis->tt_gmtoff)
			break;
		};
	    };
	} else {
	    /* User specified DST, normalize to 0 or 1 (yes, '=' is right) */
	    if (isdst = tp->tm_isdst)
		isdst = 1;

	    /* Search for an appropriate DST setting; fall out if no DST */
	    for (i = 0; i < _tzs.timecnt; i++) {
		ttis = &_tzs.ttis[_tzs.types[i]];
		if (ttis->tt_isdst == isdst)
		    break;
	    }

	    /* Asked for DST (or standard), but none defined */
	    if (ttis->tt_isdst != isdst)
		return (time_t)-1;
	}

	/* Adjust for time zone and daylight savings time */

	t -= ttis->tt_gmtoff;

	/* Normalize the broken down time and fill in the unspecified fields */

	(void)memcpy(tp, localtime(&t), sizeof(struct tm));

	/* Return the time */

	return t;
    }
