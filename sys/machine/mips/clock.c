#ifndef lint
static char *sccsid = "@(#)clock.c	4.1      (ULTRIX)  8/9/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Clock routines.
 * Modification History: clock.c
 *
 * 27-Mar-90	Randall Brown
 *	Created file to handle generic clock routines.  These routines 
 *	in turn call to system specific clock routines.
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/clock.h"

extern int cpu;
extern struct cpusw *cpup;

#define	TEST_PERIOD	500

config_delay()
{
	DELAY(TEST_PERIOD);
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by reading the interval count
 * register to determine the time remaining to the next clock tick.
 * We must compensate for wraparound which is not yet reflected in the time
 * (which happens when the counter hits 0 and wraps after the splhigh(),
 * but before the counter latch command).  Also check that this time is
 * no less than any previously-reported time, which could happen around
 * the time of a clock adjustment.  Just for fun, we guarantee that
 * the time will be greater than the value obtained by a previous call.
 */
microtime(tvp)
	register struct timeval *tvp;
{
	static struct timeval lasttime;
	int	s = splclock();

	tvp -> tv_sec = time.tv_sec;
	tvp -> tv_usec = time.tv_usec + (1000000/hz) + tick;
	while (tvp -> tv_usec > 1000000) {
		tvp -> tv_sec++;
		tvp -> tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx (s);
}

/*
 * Initialize the time of day register, based on the time base which is
 * from a filesystem.  Base provides the time to within six months.
 */
inittodr(base)
	time_t base;
{
	register u_int todr;
	long deltat;
	int year = YRREF;
	int checktodr();
	extern long savetime;   /* used to save the boot time for errlog */

	/*
	 * Once a day check for clock rollover
	 */
	timeout(checktodr, 0, SECDAY*hz);

	todr = read_todclk();	

	/*
	 * NOTE: 
	 * On pmax & 3max, read_todclk returns seconds from the
	 *    beginning of the year + (1 << 26).
	 * On Mipsfair or Isis (with 10mS VAX time of day clock),
 	 *    read_todclk returns the number of 10mS ticks from the
	 *    beginning of the year + (1 << 28).
	 */
	if (base < 5*SECYR) {
		printf("WARNING: preposterous time in file system");
		time.tv_sec = 6*SECYR + 186*SECDAY + SECDAY/2;
		resettodr();
		goto check;
	}
	/*
	 * cpup->todrzero is base used to detected loss of power to TODCLK
	 */
	if (todr < cpup->todrzero) {
		printf("WARNING: lost battery backup clock");
		time.tv_sec = base;
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		resettodr();
		goto check;
	}

	/*
	 * Sneak to within 6 month of the time in the filesystem,
	 * by starting with the time of the year suggested by the TODR,
	 * and advancing through succesive years.  Adding the number of
	 * seconds in the current year takes us to the end of the current year
	 * and then around into the next year to the same position.
	 */

	time.tv_sec = (todr - cpup->todrzero) / cpup->rdclk_divider;

	while (time.tv_sec < base-SECYR/2) {
		if (LEAPYEAR(year))
			time.tv_sec += SECDAY;
		year++;
		time.tv_sec += SECYR;
	}

	/*
	 * See if we gained/lost two or more days;
	 * if so, assume something is amiss.
	 */
	deltat = time.tv_sec - base;
	if (deltat < 0)
		deltat = -deltat;
	if (deltat >= 2*SECDAY) {
		printf("WARNING: clock %s %d days",
		time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
	}
	resettodr();
	savetime = time.tv_sec;
	return;
check:
	savetime = time.tv_sec;
	printf(" -- CHECK AND RESET THE DATE!\n");
	return;
}

/*
 * checktodr -- check for clock rollover and reset if necessary
 */
checktodr()
{
	timeout(checktodr, 0, SECDAY*hz);	/* Check again tomorrow */
	if ((read_todclk()/cpup->rdclk_divider) > SECYR+(2*SECDAY))
	    resettodr();
}
	

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * cpup->todrzero + 100*(SECYR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 */
resettodr()
{
	int year = YRREF;
	u_int secyr;
	u_int yrtime;
	int s;
	
	s = splclock();
	yrtime = time.tv_sec;
	splx(s);

	/*
	 * Whittle the time down to an offset in the current year,
	 * by subtracting off whole years as long as possible.
	 */
	for (;;) {
		secyr = SECYR;
		if (LEAPYEAR(year))
			secyr += SECDAY;
		if (yrtime < secyr)
			break;
		yrtime -= secyr;
		year++;
	}
	write_todclk(yrtime);
}

