#ifndef lint
static char *sccsid = "@(#)date.c	4.1	ULTRIX 7/2/90";
#endif

/************************************************************************
 *									*
 *		     Copyright (c) 1985,86,87,88,89 by		        *
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

/* ---------------------------------------------------------------------
 * Modification History 
 *
 * 05 Jun, 1989 -- lesniak
 *	Changed to not use obsolute timezone function to get time
 *	zone name.
 *
 * 11 Apr, 1989 -- gray
 *      Added formatting capabilities and the -c option to be 
 *      Posix compliant, P1003.2 draft 8. 
 *      Used getopt to get options.  DJG#1
 *
 * 20 Apr, 1984 -- rjl
 *	Added date formating capabilities from system V and support
 *	for the setting of the timezone. This feature relies on a kernel
 *	fix.
 *
 * 10 Apr 1985 -- jrs
 *	Use append mode when updating wtmp file
 *
 * 07 Aug 1986 -- prs
 *	Added parsing of negative number in the minutes west of 
 *	Greenwich field.
 *
 * ---------------------------------------------------------------------
 */

/*
 * Date - print and set date with formating capabilites.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/file.h>
#include <utmp.h>

#define WTMP		"/usr/adm/wtmp"
#define OPTIONS		"cu"

extern int	optind;
extern char	*optarg;

struct	timeval tv;
struct	timezone tz;
char	datestamp[256];
char	*ap, *ep, *sp, *cp, buf[200];
int	uflag = 0, cflag = 0;

static	int	dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static char *usage = "usage: date [-c | -u] [+format] [[yy[mm[dd]]]hhmm[.ss][-tttt][z]]\n";


#define	MONTH	itoa(tp->tm_mon+1,cp,2)
#define	DAY	itoa(tp->tm_mday,cp,2)
#define	YEAR	itoa(tp->tm_year,cp,2)
#define	HOUR	itoa(tp->tm_hour,cp,2)
#define	MINUTE	itoa(tp->tm_min,cp,2)
#define	SECOND	itoa(tp->tm_sec,cp,2)
#define	JULIAN	itoa(tp->tm_yday+1,cp,3)
#define	WEEKDAY	itoa(tp->tm_wday,cp,1)
#define	MODHOUR	itoa(h,cp,2)
#define ZONE	itoa(tz.tz_minuteswest,cp,4)
#define	dysize(A) (((A)%4)? 365: 366)

char	month[12][3] = {
	"Jan","Feb","Mar","Apr",
	"May","Jun","Jul","Aug",
	"Sep","Oct","Nov","Dec"
};

char	days[7][3] = {
	"Sun","Mon","Tue","Wed",
	"Thu","Fri","Sat"
};

char	*itoa();

struct utmp wtmp[2] = {
	{ "|", "", "", 0 },
	{ "{", "", "", 0 }
};

char  	*dst_type = "nuawme";
char	*ctime();
char	*asctime();
struct	tm *localtime();
struct	tm *gmtime();
struct	tm *tp;

main(argc, argv)

int argc;
char *argv[];

{
    register char *tzn, c;
    int wf, rc, i, h;
    char	*ucttime();
    
    rc = 0;
    gettimeofday(&tv, &tz);
    while ((c = getopt (argc, argv, OPTIONS)) != EOF) {
        switch (c) {
    	case 'u':
    	    uflag ++;
    	    tp = gmtime(&tv.tv_sec);
    	    break;
    	case 'c':   /* We are not worrying about leap seconds */
    	    cflag ++;
    	    tp = gmtime(&tv.tv_sec);
    	    break;
    	case '?':
    	default:
    	    printf (usage);
    	    exit(1);
        }
    	
    }
    if (uflag && cflag) {
    	printf(usage);
    	exit (1);
    }
    if (optind < argc ) {
    	ap = argv[optind];
	if (*ap == '+') { /* exits after processing format */
	    if (!uflag && !cflag) 
		tp = localtime(&tv.tv_sec);
	    ap++;
	    if (strftime(datestamp, 256, ap, tp) != 0) { /* DJG#1 */
		printf ("%s\n", datestamp);
		exit (0);
	    }
	    else {
		fprintf (stderr, "%s: Could not format date\n", argv[0]);
		exit(1);
	    }
	}
    	wtmp[0].ut_time = tv.tv_sec;
    	if (gtime()) {
    		printf(usage);
    		exit(1);
    	}
    	/* convert to GMT assuming local time */
    	if (uflag == 0) {
    		tv.tv_sec += (long)tz.tz_minuteswest*60;
    		/* now fix up local daylight time */
    		if (localtime(&tv.tv_sec)->tm_isdst)
    			tv.tv_sec -= 60*60;
    	}
    	tv.tv_sec = tv.tv_sec;
    	if (settimeofday(&tv, &tz) < 0) {
    		rc++;
    		perror("Failed to set date");
    	} else if ((wf = open(WTMP, O_WRONLY|O_APPEND)) >= 0) {
    		time(&wtmp[1].ut_time);
    		write(wf, (char *)wtmp, sizeof(wtmp));
    		close(wf);
    	}
    }
    if (rc == 0)
    	time(&tv.tv_sec);
    if (uflag) {
    	ap = asctime(gmtime(&tv.tv_sec));
    	tzn = "GMT";
    } else if (cflag) {
    	ap = asctime(gmtime(&tv.tv_sec));
    	tzn = "UCT";
    } else {
    	tp = localtime(&tv.tv_sec);
    	ap = asctime(tp);
	tzn = tp->tm_zone;
    }
    printf("%.20s", ap);
    if (tzn)
    	printf("%s", tzn);
    printf("%s", ap+19);
    	exit(rc);
}

/*
 * Parse the date setting string.
 *
 *	Format  [[[yy]MM]dd]hhmm[.ss][-tttt][z]
 *	Where:	yy year
 *		MM month
 *		dd day
 *		hh hour
 *		mm min
 *		ss sec
 *		tttt timezone (minutes west of greenwich mean
 *		z    {n=none,u=usa,a=austrailian,w=western europe
 *			     m=middle europe,e=eastern europe}
 */
gtime()
{
	register int i, year, month;
	int day, hour, mins, secs, itz;
	int east_greenwich = 0;		/* Set to 1 if "west of Greenwich" */
	struct tm *L;
	char x;

	ep = ap;
	while(*ep) ep++;
	sp = ap;
	/*
	 * Reverse the string.
	 */
	while(sp < ep) {
		x = *sp;
		*sp++ = *--ep;
		*ep = x;
	}
	sp = ap;
	gettimeofday(&tv, 0);
	L = localtime(&tv.tv_sec);
	/*
	 * Set the type of daylight savings time if present.
	 */
	switch ( *sp++ ) {
	case 'n':
		tz.tz_dsttime = DST_NONE;
		break;
	case 'u':
		tz.tz_dsttime = DST_USA;
		break;
	case 'a':
		tz.tz_dsttime = DST_AUST;
		break;
	case 'w':
		tz.tz_dsttime = DST_WET;
		break;
	case 'm':
		tz.tz_dsttime = DST_MET;
		break;
	case 'e':
		tz.tz_dsttime = DST_EET;
		break;
	default:
		sp--;
		break;
	}
	/*
	 * Look for the dash as the fifth char to determine
	 * if we should try to parse a timezone.
	 */
	if ( sp[4] == '-' ) {
		if ( sp[5] == '-' )
			/*
			 * If a negative number in the minutes west field,
			 * set the boolean var east_greenwich to 1.
			 */
			east_greenwich = 1;
		itz = gp(-1);
		itz += gp(-1) * 100;
		sp++;
		if( itz >= 0 && itz < 1440 )
			tz.tz_minuteswest = itz;
		else
			return (1);
		if (east_greenwich) {
			/*
			 * If minuteswest was meant to be negative,
			 * set it to be.
			 */
			tz.tz_minuteswest = 0 - tz.tz_minuteswest;
			sp++;
		}
	}
	/*
	 * Do the seconds.  If the seconds is not followed by a period
	 * then we really got mins.
	 */
	secs = gp(-1);
	if (*sp != '.') {
		mins = secs;
		secs = 0;
	} else {
		sp++;
		mins = gp(-1);
	}
	hour = gp(-1);
	/*
	 * Try for day month and year and use the system values as a default
	 * if the string is exhausted.
	 */ 
	day = gp(L->tm_mday);
	month = gp(L->tm_mon+1);
	year = gp(L->tm_year);
	if (*sp)
		return (1);
	/*
	 * Do a little sanity check
	 */
	if (month < 1 || month > 12 ||
	    day < 1 || day > 31 ||
	    mins < 0 || mins > 59 ||
	    secs < 0 || secs > 59)
		return (1);
	if (hour == 24) {
		hour = 0;
		day++;
	}
	if (hour < 0 || hour > 23)
		return (1);
	tv.tv_sec = 0;
	/*
	 * Added up the seconds since the epoch
	 */
	year += 1900;
	for (i = 1970; i < year; i++)
		tv.tv_sec += dysize(i);
	/* 
	 * Leap year 
	 */
	if (dysize(year) == 366 && month >= 3)
		tv.tv_sec++;
	/*
	 * Do the current year
	 */
	while (--month)
		tv.tv_sec += dmsize[month-1];
	tv.tv_sec += day-1;
	tv.tv_sec = 24*tv.tv_sec + hour;
	tv.tv_sec = 60*tv.tv_sec + mins;
	tv.tv_sec = 60*tv.tv_sec + secs;
	return (0);
}

/*
 *	Return the next two decimal digits.
 *	This routine returns it's argument if the source string is
 *	depleted.
 */
gp(dfault)
{
	register int c, d;

	if (*sp == 0)
		return (dfault);
	/*
	 * Get the digits and convert from ascii to decimal.
	 */
	c = (*sp++) - '0';
	d = (*sp ? (*sp++) - '0' : 0);
	/*
	 * Range check the digits
	 */
	if (c < 0 || c > 9 || d < 0 || d > 9)
		return (-1);
	return (c+10*d);
}

/*
 * Special version of integer to ascii conversion for prnt_format.
 */
char *
itoa(i,ptr,dig)
register  int	i;
register  int	dig;
register  char	*ptr;
{
	switch(dig)	{
		case 4: 
			*ptr++ = i/1000 + '0';
			i = i - i / 1000 * 1000;
		case 3:
			*ptr++ = i/100 + '0';
			i = i - i / 100 * 100;
		case 2:
			*ptr++ = i / 10 + '0';
		case 1:	
			*ptr++ = i % 10 + '0';
	}
	return(ptr);
}
/*
 * Print the date using format strings
 * This routine uses a format similar to printf, however there is
 * no attempt at compatibility.
 *
 * Note: This routine exits when done.
 */
prnt_date( ap )
char *ap;
{
	char c;
	int i, h;
	int hflag = 0;

	ap++;
	cp = buf;
	while(c = *ap++) {
		if(c == '%')
		switch(*ap++) {
		case '%':
			*cp++ = '%';
			continue;
		case 'n':
			*cp++ = '\n';
			continue;
		case 't':
			*cp++ = '\t';
			continue;
		case 'm' :
			cp = MONTH;
			continue;
		case 'd':
			cp = DAY;
			continue;
		case 'y' :
			cp = YEAR;
			continue;
		case 'D':
			cp = MONTH;
			*cp++ = '/';
			cp = DAY;
			*cp++ = '/';
			cp = YEAR;
			continue;
		case 'H':
			cp = HOUR;
			continue;
		case 'M':
			cp = MINUTE;
			continue;
		case 'S':
			cp = SECOND;
			continue;
		case 'T':
			cp = HOUR;
			*cp++ = ':';
			cp = MINUTE;
			*cp++ = ':';
			cp = SECOND;
			continue;
		case 'j':
			cp = JULIAN;
			continue;
		case 'w':
			cp = WEEKDAY;
			continue;
		case 'r':
			if((h = tp->tm_hour) >= 12)
				hflag++;
			if((h %= 12) == 0)
				h = 12;
			cp = MODHOUR;
			*cp++ = ':';
			cp = MINUTE;
			*cp++ = ':';
			cp = SECOND;
			*cp++ = ' ';
			if(hflag)
				*cp++ = 'P';
			else *cp++ = 'A';
			*cp++ = 'M';
			continue;
		case 'h':
			for(i=0; i<3; i++)
				*cp++ = month[tp->tm_mon][i];
			continue;
		case 'a':
			for(i=0; i<3; i++)
				*cp++ = days[tp->tm_wday][i];
			continue;
		case 'z':
			cp = ZONE;
			continue;
		case 's':
			*cp++ = dst_type[ tz.tz_dsttime ];
			continue;
		default:
			(void) fprintf(stderr, "date: bad format character - %c\n", *--ap);
			exit(2);
		}	/* endsw */
		*cp++ = c;
	}	/* endwh */

	*cp = '\n';
	(void) write(1,buf,(cp - &buf[0]) + 1);
	exit(0);
}

