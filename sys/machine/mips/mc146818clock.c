#ifndef lint
static char *sccsid = "@(#)mc146818clock.c	4.3      (ULTRIX)  11/9/90";
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
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Clock routines.
 * Modification History: mc146818clock.c
 *
 *  30-Apr-1990 Randall Brown
 *	Moved all generic clock handling routines to clock.c
 *
 *  29-Mar-1990 gmm
 *	Changed splhigh() to splextreme() since now splhigh() same as
 *	splclock().
 *
 *  06-Mar-1990 afd
 *	Create mc146818startclocks() to re-enable clock interrrupts.
 *	Needed so drivers can do timeouts during autoconfiguration.
 *
 *  4-Dec-1989 afd
 *	In mc146818stopclocks() don't "clobber" (zero) rega and regb
 *	just turn off interrupt enable in regb.  This fixes the bug of
 *	loosing time on reboots (during autoconfig).
 *
 *  26-Oct-1989 afd
 *	Fixed the cpu checks around the format of todr to explicitely
 *	check for workstations that use the mc146818-clock chip,
 *	the default case is a VAX 10mS clock.
 *
 *  15-May-1989 Kong
 *	Added variable todrzero which is set to 1<<26 for pmax, 
 *	and 1<<28 for mipsfair and isis.  The variable is initialised
 *	by inittodr.
 *
 *  07-Apr-1989 - afd
 *	Renamed mc146818 clock specific clock routines to mc146818whatever.
 *	"startclocks", "ackclock", and "stopclocks" are now in machdep.c
 *	and called thru the cpu switch.
 *  28-Feb-1989 Kong
 *	Changed routines "startrtclock" to "kn01startrtclock",
 *	"stopclocks" to "kn01stopclocks", and "ackrtclock" to "kn01ackrtclock"
 *	"startrtclock", "ackrtclock", and "stopclocks" are now in
 *	machdep.c
 *
 *  16-Jan-1989	Kong
 *	Changed name of these routines read_todclk and write_todclk
 *	to kn01read_todclk and kn01write_todclk and vector them through
 *	the system switch table.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/clock.h"
#include "../machine/mc146818clock.h"
#include "../machine/entrypt.h"

char *rt_clock_addr;	/* assigned in processor specific knxxinit routines */

/*
 * Disable clocks 
 */
mc146818stopclocks()
{
	register volatile struct rt_clock *rt =(struct rt_clock *)rt_clock_addr;
	rt->rt_regb = rt->rt_regb & (~ RTB_PIE);

	mc146818ackrtclock();
	return(0);
}

/*
 * Enable clock interrupts
 */
mc146818startclocks()
{
	register volatile struct rt_clock *rt =(struct rt_clock *)rt_clock_addr;
	rt->rt_regb = rt->rt_regb | (RTB_PIE);

	mc146818ackrtclock();
	return(0);
}

/*
 * Clear cpu board TIM0 acknowledge register
 */
mc146818ackrtclock()
{
	register volatile struct rt_clock *rt =(struct rt_clock *)rt_clock_addr;
	register int c;
	c = rt->rt_regc;
	return(0);
}

/*
 * This is an attempt to use the brain-damaged 146818 real-time clock chip.
 * We always set the chip for a leap year (1972) and use the chip only to
 * calculate seconds from the first of the year.
 */
int month_days[12] = {
	31,	/* January */
	29,	/* February */
	31,	/* March */
	30,	/* April */
	31,	/* May */
	30,	/* June */
	31,	/* July */
	31,	/* August */
	30,	/* September */
	31,	/* October */
	30,	/* November */
	31	/* December */
};

mc146818read_todclk()
{
	register volatile struct rt_clock *rt =(struct rt_clock *)rt_clock_addr;
	register u_int month, day, year, hours, mins, secs;
	int i, s;
	extern struct cpusw *cpup;

	/*
	 * check if either the chip or the prom has noticed that
	 * battery backup has been lost
	 */
	if ((rt->rt_regd & RTD_VRT) == 0) {
		printf("lost battery backup on clock\n");
		return(0);
	}
	if ((rt->rt_mem[RT_MEMX(NVSTATE_ADDR)] & NVTOD_VALID) == 0) {
		printf("battery backup clock not valid\n");
		return(0);
	}
	/*
	 * If UIP (update in progress) is set, the 146818 should not be read.
	 * Furthermore, the 146818 can not be read more than 1/4 of the
	 * available bus cycles and not more than 50 contiguous references
	 * can be to the chip.  Therefore the DELAY(10).
	 *
	 * If UIP is not set, then we have 244us to read the state of the
	 * 146818, hence the spls.
	 */
	s = splextreme();
	while (rt->rt_rega & RTA_UIP) {
		splx(s);
		DELAY(10);
		s = splextreme();
	}
	secs = rt->rt_secs;
	mins = rt->rt_mins;
	hours = rt->rt_hours;
	day = rt->rt_daym;
	month = rt->rt_month;
	year = rt->rt_year;
	splx(s);

	if (year != BASEYEAR && year != BASEYEAR+1) {
		printf("battery backup clock set to invalid year\n");
		return(0);
	}

	/*
	 * Sum up seconds from beginning of year
	 */
	secs += mins * SECMIN;
	secs += hours * SECHOUR;
	secs += (day-1) * SECDAY;
	for (i = 0; i < month-1; i++)
		 secs += month_days[i] * SECDAY;
	year -= BASEYEAR;
	secs += year * (SECYR + SECDAY);
	secs += cpup->todrzero;
	return(secs);
}

mc146818write_todclk(year_secs)
	register long year_secs;
{
	register volatile struct rt_clock *rt =(struct rt_clock *)rt_clock_addr;
	register int month, day, hours, mins, secs;
	int dummy;

	/*
	 * Break seconds in year into month, day, hours, minutes, seconds
	 */
	for (month = 0;
	   year_secs >= month_days[month]*SECDAY;
	   year_secs -= month_days[month++]*SECDAY)
		continue;
	month++;

	for (day = 1; year_secs >= SECDAY; day++, year_secs -= SECDAY)
		continue;

	for (hours = 0;
	    year_secs >= SECHOUR;
	    hours++, year_secs -= SECHOUR)
		continue;

	for (mins = 0;
	    year_secs >= SECMIN;
	    mins++, year_secs -= SECMIN)
		continue;
	secs = year_secs;
	
	/*
	 * 146818 is brain damaged and may not be initialized to
	 * certain dates, check for them and avoid them by backing
	 * up a couple of seconds.
	 */
	if (day >= 28 && day <= 30 && hours == 23 && mins == 59
	    && secs >= 58 && secs <= 59)
		secs = 57;
	
	dummy = rt->rt_regd;	/* set VRT */
	rt->rt_regb = RTB_SET|RTB_DMBINARY|RTB_24HR;
	rt->rt_rega = RTA_DVRESET;
	rt->rt_secs = secs;
	rt->rt_mins = mins;
	rt->rt_hours = hours;
	rt->rt_daym = day;
	rt->rt_month = month;
	rt->rt_year = BASEYEAR;
	rt->rt_rega = RTA_DV32K|RTA_4ms;
	rt->rt_regb = RTB_DMBINARY|RTB_24HR|RTB_PIE;
	rt->rt_mem[RT_MEMX(NVSTATE_ADDR)] |= NVTOD_VALID;
/*
	cprintf("setting battery backup clock to %d/%d/%d %d:%d:%d\n",
	    month, day, BASEYEAR, hours, mins, secs);
	cprintf("nv state = 0x%x\n", rt->rt_mem[RT_MEMX(NVSTATE_ADDR)]);
*/
}
