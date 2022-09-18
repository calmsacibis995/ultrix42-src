#ifndef lint
static char *sccsid = "@(#)clock.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 28-Mar-89 -- Adrian Thoms
 *	Don't reset or set TODR if cpu doesn't have one.
 *
 * 19-May-87 - afd
 *	Don't access ICR register if processor doesn't have one (as
 *	determined by CPU_ICR bit in flags field of cpu switch).
 *
 * 20-Apr-87 -- afd
 *	Removed work-around for bugs in the P1 CVAX chips.
 * 
 * 09-Jun-86	bjg  Set time in inittodr, rather than logging mesg there.
 *
 * 07-May-86 	bjg  Log startup mesg in inittodr, so correct date is 
 *		     logged.
 *
 * 05-Mar-86 -- jaw  VAXBI device and controller config code added.
 *		     todr code put in cpusw.
 *
 * 04-Sep-85 -- jaw
 *	Error in loading toy clock on 1st of the month.
 *
 * 14-Aug-85 -- jaw
 *	Loop error in gettoy.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 25 Mar 85 -- jaw
 * 	day stored in TODR was two days off.
 *
 * 20 Mar 85 -- jaw
 *	Add support for VAX8200 toy clock.  Change base
 * 	stored in Clock chip to be seconds past JAN 1, 1982
 *	GMT.
 *
 * 13 Nov 84 -- rjl
 *	Added support for the MicroVAX-II toy clock.
 *
 * 14 Feb. -- rjl
 *	Changed toy message for MicroVAX I
 *
 * ---------------------------------------------------------------------
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../../machine/common/cpuconf.h"

#include "../machine/mtpr.h"
#include "../machine/clock.h"
#include "../machine/cons.h"
#include "../machine/cpu.h"
#include "../machine/pte.h"
#include "../machine/nexus.h"

#include "../io/uba/ubareg.h"

extern struct cpusw *cpup;	/* pointer to cpusw entry */

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.  Its primary function is to use some file
 * system information in case the hardare clock lost state.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

/*
 * Start the real-time clock.
 */
startrtclock()
{
	if ((cpup->flags & CPU_ICR) != 0)
		mtpr(NICR, -1000000/hz);
	mtpr(ICCS, ICCS_RUN+ICCS_IE+ICCS_TRANS+ICCS_INT+ICCS_ERR);
}

/*
 * Initialze the time of day register, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within six months,
 * and the time of year clock provides the rest.
 */
inittodr(base)
	time_t base;
{
	register u_int todr;
	long deltat;
	int year = YRREF;
	u_int secyr;
	extern long savetime;

	/*
	 * CPU V_VAX has no TODR so simply exit this call.
	 */
	if (cpu == V_VAX)
		return;
	todr = (*cpup->readtodr)();

	if (base < 5*SECYR) {
		printf("WARNING: preposterous time in file system");
		time.tv_sec = 6*SECYR + 186*SECDAY + SECDAY/2;
		resettodr();
		goto check;
	}
	/*
	 * TODRZERO is base used by VMS, which runs on local time.
	 */
	if (todr < TODRZERO) {
		if( cpu == MVAX_I )
			printf("WARNING:");
		else
			printf("WARNING: todr too small");
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
	time.tv_sec = (todr-TODRZERO)/100;
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
	if (deltat < 2*SECDAY) {
		savetime = time.tv_sec; /* defined in kernel.h */
		return;
	}
	printf("WARNING: clock %s %d days",
	    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
check:
	savetime = time.tv_sec;	/* defined in kernel.h */
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 */
resettodr()
{
	
	int year = YRREF;
	u_int secyr;
	u_int yrtime = time.tv_sec;

	/*
	 * do not try and reset TODR if CPU does not have one
	 */
	if (cpu == V_VAX)
		return;

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

	(*cpup->writetodr)(yrtime);

}

/*
 * This routine reads the time of year clock on vax's with toy clock chips.
 *  month:day:hour:min:sec is converted into msec's. 
 */
static	int	dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define	dysize(A) (((A)%4)? 365: 366)

toyread_convert(tm)
struct tm tm;
{
	int i, sec;
	int s,todr;

	/*
	 * sanity check the clock
	 */
	if( tm.tm_sec < 0 || tm.tm_sec >59  ||
		tm.tm_min < 0 || tm.tm_min > 59 ||
		tm.tm_hour < 0 || tm.tm_hour > 23  ||
		tm.tm_mday < 1 || tm.tm_mday > 31 ||
		tm.tm_mon < 1 || tm.tm_mon >12 ||
		tm.tm_year < 82 || tm.tm_year > 83 )
			return 0;
	/*
	 * Added up the seconds since the epoch
	 */


	sec = 0;
	if (tm.tm_year == 83) sec=365;


	for( i=0 ; i < tm.tm_mon-1 ; i++ )
		sec += dmsize[i];


	sec += tm.tm_mday-1;
	sec = 24*sec + tm.tm_hour;
	sec = 60*sec + tm.tm_min;
	sec = 60*sec + tm.tm_sec;
	todr = TODRZERO + (sec*100);
	return (todr);
}

/* general routine to convert time in seconds to month:day:hour:minute:sec.
   This routine is used by vax's with TOY clocks. 
*/
 
toywrite_convert(xtime,tim)
struct tm *xtime; 
u_int	tim;
{
	register int d0, d1;
	long hms, day;
	register int *tp;
	int s;

	/*
	 * break initial number into days
	 */
	hms = tim % 86400;
	day = tim / 86400;
	if (hms<0) {
		hms += 86400;
		day -= 1;
	}
	tp = (int *)xtime;

	/*
	 * generate hours:minutes:seconds
	 */
	*tp++ = hms%60;
	d1 = hms/60;
	*tp++ = d1%60;
	d1 /= 60;
	*tp++ = d1;

	/*
	 * year number
	 */

	if (day/365) {
		xtime->tm_year = 83;
		day = day - 365;
	}

	else {
		xtime->tm_year = 82;
	}

	d1 = 0;
	for (d0=0; day >= d0 ; d0 += dmsize[d1++] );

	d0 = d0 - dmsize[(d1-1)];
	xtime->tm_mday = day - d0 + 1;
	xtime->tm_mon = d1;

}

/*  general routines for VAX's with standard TODR registers */
readtodr()
{
	return(mfpr(TODR));
}


writetodr(yrtime)
u_int yrtime;
{
	mtpr(TODR,TODRZERO + (100 * yrtime));
}



