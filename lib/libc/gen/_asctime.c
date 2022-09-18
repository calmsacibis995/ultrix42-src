/************************************************************************
 *									*
 *			Copyright (c) 1987,1988,1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#ifndef lint
static char Sccsid[] = "@(#)_asctime.c	4.1	(ULTRIX)	7/3/90";
#endif

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 001	Wendy Rannenberg, Dec 11, 1989
 *	- Added _last_tz_set logic that does a tzset everytime, since the
 *	  environment may have changed. See also ctime code.
 *
 * 002  Wendy Rannenberg, Dec 11, 1989
 *      - fixed buffer problems for x, X, c options, and return length
 *
 */


#include <sys/time.h>
#include <tzfile.h>
#include <stdlib.h>
#include <i_defs.h>
#include <i_errno.h>
#include <langinfo.h>
#include "tzs.h"

#define MAXBUF	256		/* length of static buffer for assembly	*/

/*
 * _itoa -- local integer to string conversion
 *
 * SYNOPSIS:
 *	static char *
 *	_itoa(i, s, prec)
 *	int i;
 *	char *s;
 *	int prec;
 *
 * DESCRIPTION:
 *	_itoa converts the given int "i" into decimal string representation
 *	using and incrementing the buffer pointed to by ptr. "prec" contains
 *	the number of chracter needed to represent "i". The number will be null
 *	terminated.
 *
 * RETURN:
 *	pointer to first free position in buffer after converted digit.
 *
 * LIMITATIONS:
 *	Will core dump if either prec or s out of range.
 *	Can only handle positive numbers in the range 0..9999
 */
static char *
_itoa(i, s, prec)
int i;
char *s;
int prec;
{
	switch (prec)
	{
		case 4:
			*s++ = i / 1000 + '0';
			i = i - i / 1000 * 1000;
			/*FALL_THROUGH*/
		case 3:
			*s++ = i / 100 + '0';
			i = i - i / 100 * 100;
			/*FALL_THROUGH*/
		case 2:
			*s++ = i / 10 + '0';
			/*FALL_THROUGH*/
		case 1:	
			*s++ = i % 10 + '0';
	}
	*s = '\0';
	return(s);
}

/*
 * jan1 -- get day number of first day of the given year
 *
 * SYNOPSIS
 *	static 
 *	jan1(year)
 *	int year;
 *
 * DESCRIPTION
 *	Jan1 returns the day number (0 = Sunday - 6) of January 1st
 *	in any given year.  It assumes a Julian Calender back to year 1.
 *	this means the result is meaningless for years before 1801.
 * 	
 * RETURNS
 *	Day number in the range 0 to 6 (0 is a sunday)
 *
 * LIMITAIONS
 *	Here centruries are not leap years but every four centuries is.
 *	The result is meaningless before 1801 (Julian calender)
 */

static 
jan1(year)
int year;
{	int y = year - 1;

	/* 
  	 * The Theoretical Julian Epoch was a Sunday
	 *
	 * year + leap years - leap centuries + leap four centuries
 	 */
	return ((year + y/4 - y/100 + y/400) % 7);
}
	

/*
 * _asctime -- convert time to string
 *
 * SYNOPSIS:
 *	_asctime(buf, len, fmt, tp, strtab)
 *	char *buf;
 *	int   len;
 *	char *fmt;
 *	struct tm *tp;
 *	str_tab *strtab;
 *
 * DESCRIPTION:
 *	_asctime is the workhorse of all ctime type routines. It analyses the
 *	structure pointed to by tp and converts it according to the "fmt"
 *	specification, taking the necessary national names from the stringtable
 *	pointed to by strtab.
 *
 * RETURN:
 *	the length of the string in buf or 0 or failure.
 *
 */

static char *weekname[7] = {	
	"Sunday", 	"Monday", 	"Tuesday", 
	"Wednesday", 	"Thursday", 	"Friday", 
	"Saturday"
};

static char *monthname[12] = {
	"January",	"February",	"March",	"April",
	"May",		"June",		"July",		"August",
	"September", 	"October",	"November",	"December"
};

/*
 * a useful macro to allow us to watch len
 */

#define DEC(size) if ((len -= size) < 0) return 0

/* 
 * WR 001
 * External declarations of tz variables for tzset
 */

extern struct state     _tzs;
extern int              _tz_is_set;
extern char             _last_tz_set[LAST_TZ_LEN];

int
_asctime(buf, len, fmt, tp, strtab)
char *buf;
int   len;
char *fmt;
struct tm *tp;

str_tab *strtab;
{
	register char *bp = buf;/* pointer to buffer			 */
	char *cp;		/* temp uses				 */
	char fmtc;		/* current format character		 */
	char nambuf[I_NAML];	/* buffer for database access names	 */
	int t;			/* temporary length			 */
	char *  tz;		/* timezone env. variable */

	DEC(1);			/* allow for nul terminator		 */
	i_errno = 0;

	/*
	 * WR 001
	 * check _tz_is_set or change since last call, if not call tzset
	 */

	if (!_tz_is_set || (tz = getenv("TZ")) && strcmp(tz, _last_tz_set))
		(void) tzset();

	for (*bp = '\0'; (fmtc = *fmt++) != '\0'; *bp = '\0')
	{
		/*
		 * while not in a format copy characters to buffer
		 */
		if (fmtc != '%')
		{
			DEC(1);
			*bp++ = fmtc;
			continue;
		}

		/*
		 * we have encounterd a '%' mark
		 * 	-> handle the ouput conversion
		 */
		fmtc = *fmt++;
		switch (fmtc)
		{
		case '%':	/* %% -> %				*/
			DEC(1);
			*bp++ = fmtc;
			continue;

		case 'n':	/* %n -> \n				*/
			DEC(1);
			*bp++ = '\n';
			continue;

		case 't':	/* %t -> \t				*/
			DEC(1);
			*bp++ = '\t';
			continue;

		case 'D':	/* %D -> mm/dd/yy			*/
			DEC(8);
			bp = _itoa(tp->tm_mon + 1, bp, 2);
			*bp++ = '/';
			bp = _itoa(tp->tm_mday, bp, 2);
			*bp++ = '/';
			bp = _itoa(tp->tm_year, bp, 2);
			continue;

		case 'm':	/* %m -> mm 				*/
			DEC(2);
			bp = _itoa(tp->tm_mon + 1, bp, 2);
			continue;

		case 'd':	/* %d -> dd				*/
			DEC(2);
			bp = _itoa(tp->tm_mday, bp, 2);
			continue;

		case 'y' :	/* %y -> yy				*/
			DEC(2);
			bp = _itoa(tp->tm_year, bp, 2);
			continue;

		case 'T':	/* %T -> HH:MM:SS			*/
			DEC(8);
			bp = _itoa(tp->tm_hour, bp, 2);
			*bp++ = ':';
			bp = _itoa(tp->tm_min, bp, 2);
			*bp++ = ':';
			bp = _itoa(tp->tm_sec, bp, 2);
			continue;

		case 'H':	/* %H -> HH				*/
			DEC(2);
			bp = _itoa(tp->tm_hour, bp, 2);
			continue;

		case 'M':	/* %M -> MM				*/
			DEC(2);
			bp = _itoa(tp->tm_min, bp, 2);
			continue;

		case 'S':	/* %S -> SS				*/
			DEC(2);
			bp = _itoa(tp->tm_sec, bp, 2);
			continue;

		case 'j':	/* %j -> day of year			*/
			DEC(3);
			bp = _itoa(tp->tm_yday + 1, bp, 3);
			continue;

		case 'w':	/* %w -> day of week (0 == sun)		*/
			DEC(1);
			bp = _itoa(tp->tm_wday, bp, 1);
			continue;

		case 'a':	/* %a -> abbreviated day of week string	*/
			/*
			 * assemble identifier for database name fetch
			 * and get string from database if possible,
			 * appending it to the buffer contents.
			 */
			strcpy(nambuf, "ABDAY_");
			_itoa(tp->tm_wday + 1, &nambuf[strlen(nambuf)], 1);

			if ((cp = i_getstr(nambuf, strtab)) == (char *)0)
			{
				DEC(3);
				strncpy(bp, weekname[tp->tm_wday], 3);
				bp += 3;
			}
			else
			{
				t = strlen(cp);
				DEC(t);
				strncpy(bp, cp, t);
				bp += t;
			}
			continue;

		case 'b':	/* %b -> abbreviated name of month str	*/
		case 'h':	/* %h -> abbreviated name of month str	*/
			/*
			 * assemble identifier for database name fetch
			 * and get string from database if possible,
			 * appending it to the buffer contents. Watch for
			 * leading zero problem on names
			 */
			strcpy(nambuf, "ABMON_");
			_itoa(tp->tm_mon + 1, &nambuf[6], tp->tm_mon >= 9 ? 2 : 1);

			if ((cp = i_getstr(nambuf, strtab)) == (char *)0)
			{
				DEC(3);
				strncpy(bp, monthname[tp->tm_mon], 3);
				bp += 3;
			}
			else
			{
				t = strlen(cp);
				DEC(t);
				strncpy(bp, cp, t);
				bp += t;
			}
			continue;

		case 'r':	/* %r -> time in AM/PM format		*/
			{
				int hour;	/* hour modulo 12	*/

				DEC(9);
				hour = tp->tm_hour;
				if ((hour %= 12) == 0)
					hour = 12;

				bp = _itoa(hour, bp, 2);	*bp++ = ':';
				bp = _itoa(tp->tm_min, bp, 2);	*bp++ = ':';
				bp = _itoa(tp->tm_sec, bp, 2);	*bp++ = ' ';
			}
			/*
			 * drop into locale AM PM stuff
			 */
		case 'p':	/* %p -> locale's AM or PM string	*/
			if ((cp = i_getstr((tp->tm_hour < 12) ? AM_STR : PM_STR, strtab)) == (char *)0)
				cp = (tp->tm_hour < 12) ? "AM" : "PM";
			t = strlen(cp);
			DEC(t);
			strncpy(bp, cp, t);
			bp += t;
			continue;

		case 'Z':	/* %Z -> name of timezone		*/
			/*
			 * In both parts of this case we make an assumption
			 * that the users timzone matches the systems idea of 
			 * timezone. This should be OK in practice 
			 */

			t = strlen(cp = tzname[tp->tm_isdst]);
			DEC(t);
			strncpy(bp, cp, t);
			bp += t;
			continue;
		
		case 'A':	/* %A -> full weekday name		*/
			strcpy(nambuf, "DAY_");
			_itoa(tp->tm_wday + 1, &nambuf[4], 1);
			
			if ((cp = i_getstr(nambuf, strtab)) == (char *)0)
				cp = weekname[tp->tm_wday];

			t = strlen(cp);
			DEC(t);
			strncpy(bp, cp, t);
			bp += t;
			continue;

		case 'B':	/* %B -> full month name		*/
			strcpy(nambuf, "MON_");
			_itoa(tp->tm_mon + 1, &nambuf[4], tp->tm_mon >= 9 ? 2 : 1);
			
			if ((cp = i_getstr(nambuf, strtab)) == (char *)0)
				cp = monthname[tp->tm_mon];

			t = strlen(cp);
			DEC(t);
			strncpy(bp, cp, t);
			bp += t;
			continue;

		case 'c':	/* %c -> locale's date and time		*/
			if ((cp = i_getstr(D_T_FMT, strtab)) == (char *)0)
				cp = "%a %b %d %H:%M:%S %Y";
			if ((t = _asctime(bp, len, cp, tp, strtab)) == 0)
				return 0;
			len -= t - 1;
			bp += t;
			continue;

		case 'X':	/* %X -> locale's appropriate time	*/
			if ((cp = i_getstr(T_FMT, strtab)) == (char *)0)
				cp = "%H:%M:%S";
			if ((t = _asctime(bp, len, cp, tp, strtab)) == 0)
				return 0;
			len -= t - 1;
			bp += t;
			continue;

		case 'x':	/* %x -> locale's appropriate date    	*/
			if ((cp = i_getstr(D_FMT, strtab)) == (char *)0)
				cp = "%m/%d/%y";
			if ((t = _asctime(bp, len, cp, tp, strtab)) == 0)
				return 0;
			len -= t - 1;
			bp += t;
			continue;

		case 'Y':	/* %Y -> fully qualified year		*/
			DEC(4);
			bp = _itoa(tp->tm_year + 1900, bp, 4);
			continue;

		case 'I':	/* %I -> hour in twelve hour clock	*/
			{
				int hour;	/* hour modulo 12 */

				DEC(2);
				hour = tp->tm_hour % 12;
				if (hour == 0)
					hour = 12;
				bp = _itoa(hour, bp, 2);
				continue;
			}
		
		case 'W':	/* %W -> weeknum, Monday first day of week */
		case 'U':	/* %w -> weeknum, Sunday first day of week */
			{
				int week;
				int firstday;

				firstday = jan1(tp->tm_year + 1900);

				/* 
				 * if monday relative, subtract one from
				 * starting day modulo 7
				 */
				if (fmtc == 'W')
					if (firstday == 0) 
						firstday = 6;
					else
						firstday--;

				week = (firstday + tp->tm_yday) / 7;
				bp = _itoa(week, bp, 2);
				continue;
			}

		default:	/* unknown format character		*/
			DEC(1);
			*bp++ = fmtc;
			continue;
		}
	}

	/*
	 * assure null termination
	 */
	*bp++ = '\0';
	return(strlen(buf));
}
