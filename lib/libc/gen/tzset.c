/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char	*sccsid = "@(#)tzset.c	4.1	(ULTRIX)	7/3/90";
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
 * 005	Jon Reeves, 16-Jan-1990
 *	Fix handling of DST with negative offset, mktime w/o rules
 *
 * 004	Jon Reeves, 20-Nov-1989
 *	Added missing set of timezone/daylight; added _last_tz_set
 *
 * 003	Ken Lesniak, 03-Nov-1989
 *	Conditionalized for X/Open
 *
 * 002	Ken Lesniak, 17-Jul-1989
 *	Moved tz_is_set flag here from ctime.c and set flag in tzset()
 *
 * 001	Ken Lesniak, 11-Apr-1989
 *	Moved here from ctime.c.
 *	Added support for POSIX 1003.1 compliancy.
 *
 *	Based on Berkeley ctime.c 1.2
 ************************************************************************/

#undef	_POSIX_SOURCE /* namespace protection is deadly to this file */
#include <sys/param.h>
#include <sys/time.h>
#include <tzfile.h>
#include <string.h>
#include <stdlib.h>
#include "tzs.h"

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

struct state		_tzs;
int			_tz_is_set = FALSE;
char			_last_tz_set[LAST_TZ_LEN]='\0';

/*
 * For both system V and X/Open, the variables timezone and daylight
 * are defined as macros. This is because for X/Open, the names
 * must not be in the ANSI/POSIX name space. There is a special assembler
 * module with duplicate definitions for the names for those users
 * that really want to use them.
 */

#ifdef SYSTEM_FIVE

/* Ideally, the initial values of these should be unimportant, because */
/* a program should call tzset() first before accessing them. But, */
/* since they have always initialized this way, I'm sure there are */
/* programs that exploit that fact. */

#define TIMEZONE timezone
#define DAYLIGHT daylight

time_t			TIMEZONE = 5*60*60;
int			DAYLIGHT = 1;
char			*tzname[2] = {"EST", "EDT"};

#else /* SYSTEM_FIVE */
#ifdef _XOPEN_SOURCE

#define TIMEZONE __timezone
#define DAYLIGHT __daylight

time_t			TIMEZONE;
int			DAYLIGHT;

#endif /* _XOPEN_SOURCE */
char			*tzname[2] = {"GMT", "GMT"};
#endif /* SYSTEM_FIVE */

static long
detzcode(codep)
char *	codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

/*
 * Attempt to load time conversion information from a tzfile formatted file.
 */

static
tzload(name)
register char *	name;
{
	register int	i;
	register int	fid;
	int		charcnt;
	int		typecnt;

	if (name == 0 && (name = TZDEFAULT) == 0)
		return -1;
	{
		register char *	p;
		register int	doaccess;
		char		fullname[MAXPATHLEN];

		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == 0)
				return -1;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, name);
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			while (*name != '\0')
				if (*name++ == '.')
					doaccess = TRUE;
			name = fullname;
		}
		if (doaccess && access(name, 4) != 0)
			return -1;
		if ((fid = open(name, 0)) == -1)
			return -1;
	}
	{
		register char *			p;
		register struct tzhead *	tzhp;
		char				buf[sizeof _tzs];

		i = read(fid, buf, sizeof buf);
		if (close(fid) != 0 || i < sizeof *tzhp)
			return -1;
		tzhp = (struct tzhead *) buf;
		_tzs.timecnt = (int) detzcode(tzhp->tzh_timecnt);
		typecnt = (int) detzcode(tzhp->tzh_typecnt);
		charcnt = (int) detzcode(tzhp->tzh_charcnt);
		if (_tzs.timecnt > TZ_MAX_TIMES ||
			typecnt == 0 ||
			typecnt > TZ_MAX_TYPES ||
			charcnt > TZ_MAX_CHARS)
				return -1;
		if (i < sizeof *tzhp +
			_tzs.timecnt * (4 + sizeof (char)) +
			typecnt * (4 + 2 * sizeof (char)) +
			charcnt * sizeof (char))
				return -1;
		p = buf + sizeof *tzhp;
		for (i = 0; i < _tzs.timecnt; ++i) {
			_tzs.ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < _tzs.timecnt; ++i)
			_tzs.types[i] = (unsigned char) *p++;
		for (i = 0; i < typecnt; ++i) {
			register struct ttinfo *	ttisp;

			ttisp = &_tzs.ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < charcnt; ++i)
			_tzs.chars[i] = *p++;
		_tzs.chars[i] = '\0';	/* ensure '\0' at end */
	}
	/*
	** Check that all the local time type indices are valid.
	*/
	for (i = 0; i < _tzs.timecnt; ++i)
		if (_tzs.types[i] >= typecnt)
			return -1;
	/*
	** Check that all abbreviation indices are valid.
	*/
	for (i = 0; i < typecnt; ++i)
		if (_tzs.ttis[i].tt_abbrind >= charcnt)
			return -1;
	/*
	** Set tzname elements to initial values.
	*/
	tzname[0] = tzname[1] = _tzs.chars;
#if defined(SYSTEM_FIVE) || defined(_XOPEN_SOURCE)
	TIMEZONE = -_tzs.ttis[0].tt_gmtoff;
	DAYLIGHT = 0;
#endif
	for (i = 1; i < typecnt; ++i) {
		register struct ttinfo *	ttisp;

		ttisp = &_tzs.ttis[i];
		if (ttisp->tt_isdst) {
			tzname[1] = &_tzs.chars[ttisp->tt_abbrind];
#if defined(SYSTEM_FIVE) || defined(_XOPEN_SOURCE)
			DAYLIGHT = 1;
#endif
		} else {
			tzname[0] = &_tzs.chars[ttisp->tt_abbrind];
#if defined(SYSTEM_FIVE) || defined(_XOPEN_SOURCE)
			TIMEZONE = -ttisp->tt_gmtoff;
#endif
		}
	}
	return 0;
}

/*
 * Set time conversion information based on GMT.
 */

static
tzsetgmt()
{
	_tzs.timecnt = 0;
	_tzs.ttis[0].tt_gmtoff = 0;
	_tzs.ttis[0].tt_isdst = 0;
	_tzs.ttis[0].tt_abbrind = 0;
	tzname[0] = tzname[1] = strcpy(_tzs.chars, "GMT");
#if defined(SYSTEM_FIVE) || defined(_XOPEN_SOURCE)
	TIMEZONE = 0;
	DAYLIGHT = 0;
#endif
}

/*
 * The following tables are used for setting the time conversion
 * information in a manner which is backwards compatible with older
 * versions of BSD and System V ctime.c.
 *
 * Please, DO NOT FIX THE TABLES. They may be wrong, but the must match
 * the behavior of the old routines.
 *
 * Now, they are only used if the tzfile formatted files cannot be
 * accessed, as is the case if the program is run on a system which
 * has not yet been upgraded.
 */

struct dstab {				/* describe change to and from dst */
	int	dayyr;			/* year being described */
	int	daylb;			/* begin: first Sunday after change */
	int	dayle;			/* end: first Sunday after chagne */
};

struct dayrule {
	int		dst_type;	/* number obtained from system */
	int		dst_hrs;	/* hours to add when dst on */
	struct	dstab *	dst_rules;	/* one of the above */
	enum {STH, NTH}	dst_hemi;	/* southern, northern hemisphere */
};

static struct dstab usdaytab[] = {
	0,	96,	303,	/* all other years: beg Apr - end Oct */
	1974,	5,	333,	/* 1974: Jan 6 - last Sun. in Nov */
	1975,	58,	303,	/* 1975: Last Sun. in Feb - last Sun in Oct */
	1976,	119,	303,	/* 1976: end Apr - end Oct */
	1977,	119,	303,	/* 1977: end Apr - end Oct */
	1978,	119,	303,	/* 1978: end Apr - end Oct */
	1979,	119,	303,	/* 1979: end Apr - end Oct */
	1980,	119,	303,	/* 1980: end Apr - end Oct */
	1981,	119,	303,	/* 1981: end Apr - end Oct */
	1982,	119,	303,	/* 1982: end Apr - end Oct */
	1983,	119,	303,	/* 1983: end Apr - end Oct */
	1984,	119,	303,	/* 1984: end Apr - end Oct */
	1985,	119,	303,	/* 1985: end Apr - end Oct */
	1986,	119,	303,	/* 1986: end Apr - end Oct */
};

#ifdef SYSTEM_FIVE

static struct dayrule dayrules[] ={
	DST_USA,	1,	usdaytab,	NTH,
	-1
};

#else

static struct dstab ausdaytab[] = {
	0,	65,	303,	/* others: -> Mar 7, Oct 31 -> */
	1970,	0,	400,	/* 1970: no daylight saving at all */
	1971,	0,	303,	/* 1971: daylight saving from Oct 31 */
	1972,	58,	303,	/* 1972: Jan 1 -> Feb 27 & Oct 31 -> dec 31 */
};

/*
 * The European tables ... based on hearsay
 * Believed correct for:
 *	WE:	Great Britain, Ireland, Portugal
 *	ME:	Belgium, Luxembourg, Netherlands, Denmark, Norway,
 *		Austria, Poland, Czechoslovakia, Sweden, Switzerland,
 *		DDR, DBR, France, Spain, Hungary, Italy, Jugoslavia
 * Eastern European dst is unknown, we'll make it ME until someone speaks up.
 *	EE:	Bulgaria, Finland, Greece, Rumania, Turkey, Western Russia
 */
static struct dstab wedaytab[] = {
	0,	86,	303,	/* others: end March - end Oct */
	1983,	86,	303,	/* 1983: end March - end Oct */
	1984,	86,	303,	/* 1984: end March - end Oct */
	1985,	86,	303,	/* 1985: end March - end Oct */
};

static struct dstab medaytab[] = {
	0,	86,	272,	/* others: saving end March - end Sep */
	1983,	86,	272,	/* 1983: end March - end Sep */
	1984,	86,	272,	/* 1984: end March - end Sep */
	1985,	86,	272,	/* 1985: end March - end Sep */
};

static struct dayrule dayrules[] = {
	DST_USA,	1,	usdaytab,	NTH,
	DST_AUST,	1,	ausdaytab,	STH,
	DST_WET,	1,	wedaytab,	NTH,
	DST_MET,	1,	medaytab,	NTH,
	DST_EET,	1,	medaytab,	NTH,
	-1,
};

#endif

/*
 * This routine sets the time conversion information in a way which produces
 * compatible behavior of localtime() with older versions.
 */

static
tzset_compat(dr, sw, std, dst)
struct dayrule *dr;
int sw;		/* seconds west of gmt */
char *std;	/* standard time zone name */
char *dst;	/* daylight savings time zone name */
{
	register int	tc;
	register time_t	ytt;
	register int	tday;
	register int	wday;
	int		y;
	int		ly;
	struct dstab	*dp, *dp0, *dpyr;
	int		i;
	int		chars;
	int		type;

	_tzs.timecnt = 0;

	/* Copy "std" designation */

	tzname[0] = tzname[1] = _tzs.chars;
	chars = 0;
	while (_tzs.chars[chars++] = *std++)
		;

	/* Fill in the time type information */

	_tzs.ttis[0].tt_gmtoff = -sw;
	_tzs.ttis[0].tt_isdst = 0;
	_tzs.ttis[0].tt_abbrind = 0;

	/* If "dst" is specified, create the transtion time table */

	if (dr->dst_type >= 0) {

		/* Fill in the time type information */

		_tzs.ttis[1].tt_gmtoff = (dr->dst_hrs * SECS_PER_HOUR) - sw;
		_tzs.ttis[1].tt_abbrind = chars;
		_tzs.ttis[1].tt_isdst = 1;

		/* Copy the "dst" designation */

		tzname[1] += chars;
		while (_tzs.chars[chars++] = *dst++)
			;

		/* Now for every year that is representable by a time_t */
		/* value, create a pair of transition times. One for the */
		/* start and one for the end of summer time. The transition */
		/* times are used by localtime() in determining whether */
		/* standard or summer time is in effect. */

		/* The following *_year symbols, represent the minimum */
		/* and maximum whole years representable by the time_t */
		/* type. These definitions assume that time_t is a */
		/* signed long */

#define t_min_year (time_t)0x8017e880 /* Minimum year in time_t format */
#define min_year 1902		/* Minimum year */
#define max_year 2037		/* Maximum year */
#define feb28_day0 (31 + 28 - 1) /* Julian day (0 based) of Feb 28th */

		tc = 0;
		ytt = t_min_year;
		dp0 = dr->dst_rules;
		dpyr = dp0 + 1;

		/* Create a pair of transition times for each year */

		for (y = min_year; y < max_year; y++) {
			ly = isleap(y);

			/* Point to the begin and end days for this year */

			dp = (y == dpyr->dayyr ? dpyr++ : dp0);
			tday = dp->daylb;

			for (i = 0; i < 2; i++) {

				/* Compute the transition time */

				if (ly && tday >= feb28_day0)
					tday++;
				wday = ((ytt / SECS_PER_DAY)
					+ EPOCH_WDAY + tday)
					% DAYS_PER_WEEK;
				if (wday < 0)
					wday += DAYS_PER_WEEK;
				tday -= wday;

				/* Store the transition time and type index */

				type = i;
				if (dr->dst_hemi != STH)
				    type ^= 1;

				_tzs.ats[tc] = ytt
					+ (tday * SECS_PER_DAY)
					+ (2 * SECS_PER_HOUR)
					- _tzs.ttis[type ^ 1].tt_gmtoff;
				_tzs.types[tc] = type;
				tc++;

				tday = dp->dayle;
			};
			ytt += DAYS_PER_NYEAR * SECS_PER_DAY;
			if (ly)
				ytt += SECS_PER_DAY;
		};
		_tzs.timecnt = tc;
	};			
}

/*
 * Set the time conversion information based on the kernel's idea
 * of local time. Changes to and from daylight savings time are
 * handled in a backwards compatible fashion.
 */

static
tzsetkernel()
{
	time_t		sw;	/* seconds west */
	register struct dayrule *dr;
	struct timeval	tv;
	struct timezone	tz;
	char		*_tztab();

	/* Determine the seconds west of GMT and the dst rules */

	if (gettimeofday(&tv, &tz))
	    return -1;
	sw = tz.tz_minuteswest * 60;
	dr = dayrules;
	while (dr->dst_type >= 0 && dr->dst_type != tz.tz_dsttime)
		dr++;

	/* Create the transition time using the old rules */

	tzset_compat(dr, sw, _tztab(tz.tz_minuteswest, 0),
	    _tztab(tz.tz_minuteswest, 1));

#if defined(SYSTEM_FIVE) || defined(_XOPEN_SOURCE)
	TIMEZONE = tz.tz_minuteswest * 60;
	DAYLIGHT = tz.tz_dsttime;
#endif

	return 0;
}

/*
 * Non-System V specific routines
 */

#ifndef SYSTEM_FIVE

/*
 * utoi() converts an unsigned string of 1 or more digts, and performs
 * range checking according to supplied limits. A pointer to the first
 * non-digit is returned, unless there is an error, in which case a
 * null pointer is returned.
 */

static char *
utoi(p, ip, minv, maxv)
register char	*p;
int		*ip;
int		minv;
int		maxv;
{
	register int	i;

	/* Make sure we've got at least one digit */

	if (*p < '0' || *p > '9')
		return (char *)0;

	/* Convert until first non-digit */

	i = 0;
	do
		i = (i * 10) + *p++ - '0';
	while (*p >= '0' && *p <= '9');

	/* Make sure the number is in range */

	if (i < minv || i > maxv)
		return (char *)0;

	/* Return the number and updated pointer */

	*ip = i;
	return p;
}
	
/*
 * tzsetenv() sets the time conversion information according to the TZ
 * environment variable. The format, as specified by POSIX 1003.1, is:
 *
 *	stdoffset[dst[offset][,start[/time],end[/time]]]
 *
 * A status is returned to indicate if the string was correct. A zero
 * value indicates it was correct; non-zero indicates it was incorrect.
 */

static
tzsetenv(p)
register char *	p;
{
	register int	tti;	/* transistion time info index */
	int		val;
	int		i;
	static int	secs_per_hms[3] = {SECS_PER_HOUR, SECS_PER_MIN, 1};

	_tzs.timecnt = tti = 0;
	tzname[1] = "";

	/* Get std and dst zone names */

	{
	    register int	ai = 0;	/* abbreviation index */
	    int			len;
	    int			gmtoff;
	    char		sign = '+';
	    int			negate_gmt = 0;

	    do {

		/* Copy time zone abbreviation string */

		tzname[tti] = &_tzs.chars[ai];
		_tzs.ttis[tti].tt_abbrind = ai;

		len = 0;
		while ((*p && *p < '+') || (*p > '-' && *p < '0') || *p > '9') {
			_tzs.chars[ai++] = *p++;
			if (ai == TZ_MAX_CHARS)
				return -1;
			len++;
		};
		_tzs.chars[ai++] = '\0';
		if (len < 3)
			return -1;

		/* Get the offset's sign, if there is one */

		if (*p == '+' || *p == '-')
			sign = *p++;

		/* Get the offset if there is one, or should be */

		if (sign || (*p >= '0' && *p <= '9')) {
			gmtoff = 0;
			if (sign != '-') negate_gmt = 1;
			for (i = 0;;) {
				if (!(p = utoi(p, &val, 0, 59)))
				    return -1;
				gmtoff += val * secs_per_hms[i];
				i++;
	
				if (*p != ':' || i == 3)
				    break;
				p++;
			};
		} else

			/* We only get here if the offset for dst is */
			/* being defaulted. Since the offset for std */
			/* must be specified, gmtoff is guaranteed to */
			/* contain the std offset */

			gmtoff += SECS_PER_HOUR;

		/* Compensate for backwards '-' flag */
		if (negate_gmt)
			gmtoff = -gmtoff;
		negate_gmt = 0;
		sign = 0;

		_tzs.ttis[tti].tt_gmtoff = gmtoff;

		/* Indicate if this zone is in daylight savings time */

		_tzs.ttis[tti].tt_isdst = tti;

		/* Set up table bookkeeping for mktime() without rule */

		_tzs.types[tti] = tti;

	    } while (++tti < 2 && *p != ',' && *p != '\0');
	};

	_tzs.timecnt = tti;

	/* Process the optional rule */

	if (tti == 2 && *p == ',') {
		int		hms[2];
		int		m[2], n[2], d[2];
		int		jday[2];
		char		day_spec[2];
		int		j;
		register time_t	ytt;
		int		y;
		int		tc;
		int		ly;
		register int	tday;	/* transition day */
		register int	wday;
		static int	jday_of_month[2][MONS_PER_YEAR] = {
			0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
			0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
		};

		/* First thing we do is parse the rule */

		for (i = 0; i < 2; i++) {

			/* Check for mandatory ',' */

			if (*p++ != ',')
				return -1;

			/* Get the day specifier */

			if ((day_spec[i] = *p) == 'M') {

				/* Mm.n.d (nth day (d) of month (m)) */

				p++;

				if (!(p  = utoi(p, &m[i], 1, 12)) || *p++ != '.')
					return -1;
				if (!(p = utoi(p, &n[i], 1, 5)) || *p++ != '.')
					return -1;
				if (!(p = utoi(p, &d[i], 0, 6)))
					return -1;
			} else {

				/* Jn or n (Julian day) */

				if (*p == 'J')
					p++;
				if (!(p = utoi(p, &jday[i], day_spec[i] == 'J', 365)))
					return -1;
			};

			/* Get the optional time specifier */

			if (*p == '/') {
				p++;

				hms[i] = 0;
				for (j = 0;;) {
					if (!(p = utoi(p, &val, 0, 59)))
					    return -1;
					hms[i] += val * secs_per_hms[j];
					j++;

					if (*p != ':' || j == 3)
					    break;
					p++;
				};
			} else
				hms[i] = 2 * SECS_PER_HOUR;
		};

		/* Now for every year that is representable by a time_t */
		/* value, create a pair of transition times. One for the */
		/* start and one for the end of summer time. The transition */
		/* times are used by localtime() in determining whether */
		/* standard or summer time is in effect */

		/* The following *_year symbols, represent the minimum */
		/* and maximum whole years representable by the time_t */
		/* type. These definitions assume that time_t is a */
		/* signed long */

#define t_min_year (time_t)0x8017e880 /* Minimum year in time_t format */
#define min_year 1902		/* Minimum year */
#define max_year 2037		/* Maximum year */
#define feb28_day (31 + 28)	/* Julian day (1 based) of Feb 28th */

		tc = 0;
		ytt = t_min_year;

		/* Create a pair of transition times for each year */

		for (y = min_year; y <= max_year; y++) {
			ly = isleap(y);
			for (i = 0; i < 2; i++) {

				/* Determine the transition time according */
				/* to its specification */

				if (day_spec[i] == 'M') {

					/* day of month */

					tday = jday_of_month[ly][m[i]-1];
					wday = (d[i] - ((ytt / SECS_PER_DAY)
						+ EPOCH_WDAY + tday))
						% DAYS_PER_WEEK;
					if (wday < 0)
						wday += DAYS_PER_WEEK;
					tday += wday;
					tday += (n[i] - 1) * DAYS_PER_WEEK;
					if (tday >= jday_of_month[ly][m[i]])
						tday -= DAYS_PER_WEEK;
				} else {

					/* day of year */

					tday = jday[i];
					if (day_spec[i] == 'J' && (!ly || tday < feb28_day))
					    tday--;
				};

				/* Store the transition time and type index */

				_tzs.ats[tc] = ytt + hms[i] + (tday * SECS_PER_DAY) - _tzs.ttis[i].tt_gmtoff;
				_tzs.types[tc] = i ^ 1;
				tc++;
			};
			ytt += DAYS_PER_NYEAR * SECS_PER_DAY;
			if (ly)
				ytt += SECS_PER_DAY;
		};
		_tzs.timecnt = tc;
	};

#if	defined(_XOPEN_SOURCE)
	TIMEZONE = -_tzs.ttis[0].tt_gmtoff;
	DAYLIGHT = (tti>1);
#endif

	/* If not at end of string, then there was something unexpected */

	return *p != '\0';
}

#endif

/*
 * System V specific routines.
 */

#ifdef SYSTEM_FIVE

/*
 * tzsetenv() sets the time zone information according to the TZ
 * environment variable using the System V format of stdoffsetdst.
 * For compatiblity, the transition times are computed using the same
 * brain damaged rules as System V. This routine always succeeds, but
 * the results may be bogus if the format of the variable is incorrect.
 */

static
tzsetenv(p)
register char *	p;
{
	register int	n;
	char		sign;
	char		*std = "123";
	char		*dst = "123";
	struct dayrule	*dr = dayrules;

	/* Copy the "std" designation */

	n = 0;
	do
		std[n] = *p ? *p++ : ' ';
	while (++n < 3);

	/* Get the offset, which may be preceded by a minus sign */

	if (sign = *p == '-')
		p++;
	n = 0;
	while (*p >= '0' && *p <= '9')
		n = (n * 10) + *p++ - '0';

	/* Compute the time zone offset */

	if (sign)
		n = -n;
	TIMEZONE = ((long)(n * 60)) * 60;

	/* If "dst" is specified, get it */

	if ((DAYLIGHT = *p) != '\0') {

		/* Get the "dst" designation */

		n = 0;
		do
			dst[n] = *p ? *p++ : ' ';
		while (++n < 3);
	} else
		dr++;

	/* Create the transition time using the old rules */

	tzset_compat(dr, timezone, std, dst);

	return 0;
}

#endif

/*
 * tzset() is used to initialize the time conversion information
 * used by localtime() and mktime().
 */

void
tzset()
{
	register char *	name;
	register char * tzpath;

#ifdef SYSTEM_FIVE
#define tzpath_of(n) (n)
#define wants_gmt(n) (n && !*n)
#define envar(n) (n)
#else
#define tzpath_of(n) ((n && *n == ':') ? &n[1] : (char *)0)
#define wants_gmt(n) (n && *n == ':' && !n[1])
#define envar(n) (n && !tzpath)
#endif /* SYSTEM_FIVE */

	_tz_is_set = TRUE;
	if (name = getenv("TZ")) 
		strncpy(_last_tz_set, name, sizeof(_last_tz_set));
	else
		_last_tz_set[0] = '\0';
	if (!wants_gmt(name)) {			/* did not request GMT */
	    tzpath = tzpath_of(name);
	    if (tzpath && !tzload(tzpath))	/* requested path worked */
		return;
	    if (envar(name) && !tzsetenv(name))	/* environment string worked */
		return;
	    if (!tzload((char *)0))		/* default name worked */
		return;
	    if (!tzsetkernel())			/* kernel guess worked */
		return;
	}
	tzsetgmt();				/* GMT is default */
}

static struct zone {
	int	offset;
	char	*stdzone;
	char	*dlzone;
} zonetab[] = {
	-1*60,	"MET",	"MET DST",	/* Middle European */
	-2*60,	"EET",	"EET DST",	/* Eastern European */
	4*60,	"AST",	"ADT",		/* Atlantic */
	5*60,	"EST",	"EDT",		/* Eastern */
	6*60,	"CST",	"CDT",		/* Central */
	7*60,	"MST",	"MDT",		/* Mountain */
	8*60,	"PST",	"PDT",		/* Pacific */
#ifdef notdef
	/* there's no way to distinguish this from WET */
	0,	"GMT",	0,		/* Greenwich */
#endif
	0*60,	"WET",	"WET DST",	/* Western European */
	-10*60,	"EST",	"EST",		/* Aust: Eastern */
     -10*60+30,	"CST",	"CST",		/* Aust: Central */
	-8*60,	"WST",	0,		/* Aust: Western */
	-1
};

/*
 * _tztab --
 *	check static tables or create a new zone name; broken out from
 *	timezone() so that we can make a guess as to what the zone is if
 *	the standard tables aren't in place in /etc. 
 *	DO NOT USE THIS ROUTINE OUTSIDE OF THE STANDARD LIBRARY.
 */
char *
_tztab(zone,dst)
	register int	zone;
	int	dst;
{
	register struct zone	*zp;
	register char	*cp;
	static char	czone[] = "GMT+nn:nn";	/* space for zone name */

	/* search static tables */

	for (zp = zonetab; zp->offset != -1;++zp)
		if (zp->offset == zone) {
			if (dst && zp->dlzone)
				return(zp->dlzone);
			if (!dst && zp->stdzone)
				return(zp->stdzone);
		}

	/* create one */

	cp = &czone[3];
	if (zone < 0) {
		zone = -zone;
		*cp++ = '+';
	}
	else
		*cp++ = '-';

	if ((*cp = zone / 600) != 0) {
		*cp++ += '0';
		zone %= 600;
	};
	*cp++ = (zone / 60) + '0';
	zone %= 60;
	*cp++ = ':';
	*cp++ = (zone / 10) + '0';
	*cp++ = (zone % 10) + '0';
	*cp = '\0';

	return(czone);
}
