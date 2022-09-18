#ifndef lint
static char *sccsid = "@(#)sh.time.c	4.2  (ULTRIX)        8/13/90";
#endif
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sh.time.c,v 1.5 87/04/15 17:41:19 dce Exp $ */

#include "sh.h"

/*
 * C Shell - routines handling process timing and niceing
 *
 * Modification History
 *
 * 002 - Bob Fontaine	- Fri Jun 22 09:53:01 EDT 1990
 *	Changed call to internal printf function to csh_printf to avoid
 *	confusion with stdio library routine.
 *
 * 001 - Gary A. Gaudet - Tue Jan  2 12:05:05 EST 1990
 *	straightened a loop
 *	added comments
 */

settimes()
{
	struct rusage ruch;

/*
 *	get the current time
 */
	(void) gettimeofday(&time0, (struct timezone *)0);
/*
 *	get information on resources used by this process
 */
	(void) getrusage(RUSAGE_SELF, &ru0);
/*
 *	get information on resources used by this process's children
 */
	(void) getrusage(RUSAGE_CHILDREN, &ruch);
/*
 *	add children's resource usage to this process's resource usage
 */
	ruadd(&ru0, &ruch);
}

/*
 * dotime is only called if it is truly a builtin function and not a
 * prefix to another command
 */
dotime()
{
	struct timeval timedol;
	struct rusage ru1, ruch;

	(void) getrusage(RUSAGE_SELF, &ru1);
	(void) getrusage(RUSAGE_CHILDREN, &ruch);
	ruadd(&ru1, &ruch);
	(void) gettimeofday(&timedol, (struct timezone *)0);
	prusage(&ru0, &ru1, &timedol, &time0);
}

/*
 * donice is only called when it on the line by itself or with a +- value
 */
donice(v)
	register char **v;
{
	register char *cp;
	int nval;

	v++, cp = *v++;
	if (cp == 0)
		nval = 4;
	else if (*v == 0 && any(cp[0], "+-"))
		nval = getn(cp);
	(void) setpriority(PRIO_PROCESS, 0, nval);
}

/*
 * ruadd() - adds ru2's resources usage to ru's resource usage
 */
ruadd(ru, ru2)
	register struct rusage *ru, *ru2;
{
/*
 *	add ru2's user time to ru's user time
 */
	tvadd(&ru->ru_utime, &ru2->ru_utime);
/*
 *	add ru2's system time to ru's system time
 */
	tvadd(&ru->ru_stime, &ru2->ru_stime);
/*
 *	set ru's resident set size to the larger of ru's or ru2's
 */
	if (ru2->ru_maxrss > ru->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
/*
 * add the following ru2's resource usage to ru's resource usage
 */
	ru->ru_ixrss += ru2->ru_ixrss;	/* 001 - GAG */
	ru->ru_ismrss += ru2->ru_ismrss;
	ru->ru_idrss += ru2->ru_idrss;
	ru->ru_isrss += ru2->ru_isrss;
	ru->ru_minflt += ru2->ru_minflt;
	ru->ru_majflt += ru2->ru_majflt;
	ru->ru_nswap += ru2->ru_nswap;
	ru->ru_inblock += ru2->ru_inblock;
	ru->ru_oublock += ru2->ru_oublock;
	ru->ru_msgsnd += ru2->ru_msgsnd;
	ru->ru_msgrcv += ru2->ru_msgrcv;
	ru->ru_nsignals += ru2->ru_nsignals;
	ru->ru_nvcsw += ru2->ru_nvcsw;
	ru->ru_nivcsw += ru2->ru_nivcsw;
}

prusage(r0, r1, e, b)
	register struct rusage *r0, *r1;
	struct timeval *e, *b;
{
	static int kb_per_page = 0;
	register time_t t =
	    (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
	    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
	    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
	    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
	register char *cp;
	register int i;
	register struct varent *vp = adrof("time");
	int ms =
	    (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

	/*
	 * Get number of kilobytes per page. Must be an integer.
	 */

	if (kb_per_page == 0) {
		kb_per_page = getpagesize()/1024;
	}

	cp = "%Uu %Ss %E %P %X+%Dk %I+%Oio %Fpf+%Ww";
	if (vp && vp->vec[0] && vp->vec[1])
		cp = vp->vec[1];
	for (; *cp; cp++)
	if (*cp != '%')
		putchar(*cp);
	else if (cp[1]) switch(*++cp) {

	case 'U':
		pdeltat(&r1->ru_utime, &r0->ru_utime);
		break;

	case 'S':
		pdeltat(&r1->ru_stime, &r0->ru_stime);
		break;

	case 'E':
		psecs((long)(ms / 100));
		break;

	case 'P':
		/*
		 * ms = total time
		 * t  = system + user time
		 *
		 * Due to the fact that these are stored with different
		 * units and are rounded differently, t can be bigger
		 * than ms, which would cause a number bigger than 100%.
		 * In this case, or when ms is 0, we just print 100%.
		 */

		if (t > ms || ms == 0) {
			csh_printf("100%%");			/* 002 RNF */
		} else {
			csh_printf("%d%%", (int) (t*100 / ms)); /* 002 RNF */
		}
		break;

	case 'W':
		i = r1->ru_nswap - r0->ru_nswap;
		csh_printf("%d", i);				/* 002 RNF */
		break;

	case 'X':
		csh_printf("%d", t == 0 ? 0 :			/* 002 RNF */
		    ((r1->ru_ixrss-r0->ru_ixrss) * kb_per_page)/t);
		break;

	case 'D':
		csh_printf("%d", t == 0 ? 0 :			/* 002 RNF */
		    ((r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss)) *
		    kb_per_page)/t);
		break;

	case 'K':
		csh_printf("%d", t == 0 ? 0 :			/* 002 RNF */
		    (((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
		    (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss)) * kb_per_page)/t);
		break;

	case 'M':
		csh_printf("%d", (r1->ru_maxrss * kb_per_page)/2);/* 002 RNF */
		break;

	case 'F':
		csh_printf("%d", r1->ru_majflt-r0->ru_majflt);	/* 002 RNF */
		break;

	case 'R':
		csh_printf("%d", r1->ru_minflt-r0->ru_minflt);	/* 002 RNF */
		break;

	case 'I':
		csh_printf("%d", r1->ru_inblock-r0->ru_inblock);/* 002 RNF */
		break;

	case 'O':
		csh_printf("%d", r1->ru_oublock-r0->ru_oublock);/* 002 RNF */
		break;
	}
	putchar('\n');
}

pdeltat(t1, t0)
	struct timeval *t1, *t0;
{
	struct timeval td;

	tvsub(&td, t1, t0);
	csh_printf("%d.%01d", td.tv_sec, td.tv_usec/100000);	/* 002 RNF */
}

/*
 * tvadd() - add t0's time to tsum's time
 */
tvadd(tsum, t0)
	struct timeval *tsum, *t0;
{
/*
 *	add t0's seconds to tsum's seconds
 */

	tsum->tv_sec += t0->tv_sec;
/*
 *	add t0's microseconds to tsum's microseconds
 */
	tsum->tv_usec += t0->tv_usec;
/*
 *	check to see if the number of tsum's microseconds is greater
 *	than a second and if so, adjust tsum's microseconds and second
 */
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}
