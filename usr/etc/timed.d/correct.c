#ifndef lint
static	char	*sccsid = "@(#)correct.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *static char sccsid[] = "@(#)correct.c	2.3 (Berkeley) 5/28/86";
 */


#include "globals.h"
#include <protocols/timed.h>

#ifdef MEASURE
extern FILE *fp;
#endif

/* 
 * `correct' sends to the slaves the corrections for their clocks
 */

correct(avdelta)
long avdelta;
{
	int i;
	int corr;
	struct timeval adjlocal;
	struct tsp msgs;
	struct timeval mstotvround();
	struct tsp *answer, *acksend();

#ifdef MEASURE
	for(i=0; i<slvcount; i++) {
		if (hp[i].delta == HOSTDOWN)
			fprintf(fp, "%s\t", "down");
		else { 
			fprintf(fp, "%d\t", hp[i].delta);
		}
	}
	fprintf(fp, "\n");
#endif
        if (!External_source)
            {

              corr = avdelta - hp[0].delta;
              adjlocal = mstotvround(&corr);
              adjclock(&adjlocal);
#ifdef MEASURE
              fprintf(fp, "%d\t", corr);
#endif
            }

	for(i=1; i<slvcount; i++) {
		if (hp[i].delta != HOSTDOWN)  {
			corr = avdelta - hp[i].delta;
			msgs.tsp_time = mstotvround(&corr);
			msgs.tsp_type = (u_char)TSP_ADJTIME;
			(void)strcpy(msgs.tsp_name, hostname);
			answer = acksend(&msgs, &hp[i].addr, hp[i].name,
			    TSP_ACK, (struct netinfo *)NULL);
			if (answer == NULL) {
				hp[i].delta = HOSTDOWN;
#ifdef MEASURE
				fprintf(fp, "%s\t", "down");
			} else {
				fprintf(fp, "%d\t", corr);
#endif
			}
		} else {
#ifdef MEASURE
			fprintf(fp, "%s\t", "down");
#endif
		}
	}
#ifdef MEASURE
	fprintf(fp, "\n");
#endif
}

/* 
 * `mstotvround' rounds up the value of the argument to the 
 * nearest multiple of five, and converts it into a timeval 
 */
 
struct timeval mstotvround(x)
int *x;
{
	int temp;
	struct timeval adj;

	temp = *x % 5;
	if (temp >= 3)
		*x = *x-temp+5;
	else {
		if (temp <= -3)
			*x = *x - temp -5;
		else 
			*x = *x-temp;
	}
	adj.tv_sec = *x/1000;
	adj.tv_usec = (*x-adj.tv_sec*1000)*1000;
	if (adj.tv_usec < 0) {
		adj.tv_usec += 1000000;
		adj.tv_sec--;
	}
	return(adj);
}

adjclock(corr)
struct timeval *corr;
{
	struct timeval now;

	if (timerisset(corr)) {
		if (corr->tv_sec < MAXADJ && corr->tv_sec > - MAXADJ) {
			(void)adjtime(corr, (struct timeval *)0);
		} else {
			syslog(LOG_WARNING,
			    "clock correction too large to adjust (%d sec)",
			    corr->tv_sec);
			(void) gettimeofday(&now, (struct timezone *)0);
			timevaladd(&now, corr);
			if (settimeofday(&now, (struct timezone *)0) < 0)
				syslog(LOG_ERR, "can't set time");
		}
	}
}

timevaladd(tv1, tv2)
	register struct timeval *tv1, *tv2;
{
	
	tv1->tv_sec += tv2->tv_sec;
	tv1->tv_usec += tv2->tv_usec;
	if (tv1->tv_usec >= 1000000) {
		tv1->tv_sec++;
		tv1->tv_usec -= 1000000;
	}
	if (tv1->tv_usec < 0) {
		tv1->tv_sec--;
		tv1->tv_usec += 1000000;
	}
}
