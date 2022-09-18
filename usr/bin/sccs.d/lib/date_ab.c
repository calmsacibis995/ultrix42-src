#ifndef lint
static	char	*sccsid = "@(#)date_ab.c	4.1	(ULTRIX)	7/17/90";
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
 * 003	Ken Lesniak, 12-May-1989					*
 *	Changed the tzset function defined here since it conflicts with *
 *	the new function of the same name in libc.			*
 *									*
 * 	Barb Glover, 15-Feb-1987					*
 *	Add dysize macro; no longer routine in libc/gen/ctime.c		*
 *									*
 *	David L Ballenger, 26-Apr-1985					*
 * 001	Fix problems with local definition of timezone conflicting	*
 *	with the definition in <time.h>.  Some more cleanup can be	*
 *	done to resolve the SysV/ULTRIX differences for the various	*
 *	time/date routines.						*
 *									*
 ************************************************************************/


# include	<sys/types.h>
# include	<macros.h>
# include	<time.h>
#define dysize(y) ((y)%4 == 0 ? 366 : 365)

static long	time_zone = -1;

/*
	Function to convert date in the form "yymmddhhmmss" to
	standard UNIX time (seconds since Jan. 1, 1970 GMT).
	Units left off of the right are replaced by their
	maximum possible values.

	The function corrects properly for leap year,
	daylight savings time, offset from Greenwich time, etc.

	Function returns -1 if bad time is given (i.e., "730229").
*/
char *Datep;


date_ab(adt,bdt)
char *adt;
long *bdt;
{
	int y, t, d, h, m, s, i;
	long tim;
#ifdef notdef	/* sysIII vs. 4.1cBSD discrepancy */
	extern int *localtime();
#endif notdef


	sccs_tzset();
	Datep = adt;

	if((y=g2()) == -2) y = 99;
	if(y<70 || y>99) return(-1);

	if((t=g2()) == -2) t = 12;
	if(t<1 || t>12) return(-1);

	if((d=g2()) == -2) d = mosize(y,t);
	if(d<1 || d>mosize(y,t)) return(-1);

	if((h=g2()) == -2) h = 23;
	if(h<0 || h>23) return(-1);

	if((m=g2()) == -2) m = 59;
	if(m<0 || m>59) return(-1);

	if((s=g2()) == -2) s = 59;
	if(s<0 || s>59) return(-1);

	tim = 0L;
	y += 1900;
	for(i=1970; i<y; i++)
		tim += dysize(i);
	while(--t)
		tim += mosize(y,t);
	tim += d - 1;
	tim *= 24;
	tim += h;
	tim *= 60;
	tim += m;
	tim *= 60;
	tim += s;
	tim += time_zone;			/* GMT correction */
#ifdef notdef		/* sysIII vs. 4.1cBSD discrepancy */
	if(localtime(&tim)[8])
#else notdef
	if(localtime(&tim)->tm_isdst)
#endif notdef
		tim += -1*60*60;		/* daylight savings */
	*bdt = tim;
	return(0);
}


mosize(y,t)
int y, t;
{
	extern int dmsize[];

	if(t==2 && dysize(y)==366) return(29);
	return(dmsize[t-1]);
}


g2()
{
	register int c;
	register char *p;

	for (p = Datep; *p; p++)
		if (numeric(*p))
			break;
	if (*p) {
		c = (*p++ - '0') * 10;
		if (*p)
			c += (*p++ - '0');
		else
			c = -1;
	}
	else
		c = -2;
	Datep = p;
	return(c);
}

/***********************************************************************
 *
 * "time_zone" name clash between systemIII and 4.1cBSD
 *
 * The name "timezone" is a function in 4.1cBSD.  In systemIII timezone
 * is a global variable whose value is established with the `tzset'
 * function.  This is not a complete implementation of `tzset'.
 *
 * The array dmsize is declared "static" in 4.1BSD ctime.c
 * In systemIII it is a global name.
 *							27Aug82 jmcg
 *
 * New declaration of localtime in 4.1cBSD.
 *							3Jul83 jmcg
 */

static	int	dmsize[12] =
{
	31,	/* Jan */
	28,	/* Feb */
	31,	/* Mar */
	30,	/* Apr */
	31,	/* May */
	30,	/* Jun */
	31,	/* Jul */
	31,	/* Aug */
	30,	/* Sep */
	31,	/* Oct */
	30,	/* Nov */
	31	/* Dec */
};

sccs_tzset()
	{
	struct timeval	timebuf;
	struct timezone tz;

	gettimeofday( &timebuf, &tz);
	time_zone = 60 * tz.tz_minuteswest;
	}
