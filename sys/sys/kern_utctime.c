#ifndef lint
static	char	*sccsid = "@(#)kern_utctime.c	4.1	(DECdts)	9/4/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1989, 90 by			*
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
 * ---------------------------------------------------------------------
 *
 * 18-Jan-90   Joe Comuzzi
 *     Added functions to return maximum drift rate and resolution.
 *
 * 30-Mar-90   Joe Comuzzi
 *     Moved 64 bit macros to include file, and changed external interface
 *     to nanoseconds, eliminated type-puns in external interface.
 *
 *  6-Apr-90   Joe Comuzzi
 *     Fixed Daylight time bug.
 *
 * 13-Aug-90   Joe Comuzzi
 *     Fixed race in setting maxdriftrecip
 */

#include "../h/param.h"
#include "../h/time.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/cpudata.h"
#include "../h/int64.h"
#include "../h/utctime.h"
#include "../machine/common/cpuconf.h"
#include "../machine/cpu.h"

/* 
 * Get UTC Time of day and adjust clock routines.
 *
 * These routines provide the kernel entry points to get and set
 * utc timestamps, used by DECdts - the time syncronization service.
 */

/*
 * Variables shared with other kernel modules
 */


struct lock_t lk_utctime;
int inaccinfinite = 1;			/* Set in kern_time.c when old
                                           interface is used */

/*
 * Variables defined and used by other kernel modules
 */

extern int tickdelta;			/* Amount to change tick by each 
					   tick - used by kern_clock.c */
extern long timedelta;			/* Amount of time remaining to be
					   adjusted - used by kern_clock.c */
extern long fixtick;                    /* Amount to adjust each second */
extern struct timeval atime, btime;	/* Time value cells */
extern struct timeval *timepick;	/* Pointer to current time cell */
extern struct cpusw *cpup;		/* Pointer to cpu specific info */

static struct int64 diff_base_times = { /* Constant difference */
    0x13814000, 0x01b21dd2		/* between base times */
};

/*
 * Variables used in inaccuracy calculation
 */

static struct int64 inacc;		/* Base inaccuracy */
static long int tickadjcount;		/* Number of ticks for which
					   adjustment will occur */
static struct timeval driftbase;	/* Time at which last base
					   inaccuracy was calculated */
static struct timeval adjbase;		/* Time at which last adjustment
					   was started */
static struct int64 leaptime;		/* Time of next leap second
					   after driftbase + inacc */
static long int currentmdr = 0;		/* Current maxdriftrecip to use */
static long int nextmdr = 0;		/* maxdriftrecip to use after next
					   clock adjustment */

/*
 * Variables which have the current and next TDF info
 */

#define NO_TDF	8192			/* Used as out-of-band value to
                                           indicate not yet initialized */
static int curtdf = NO_TDF;		/* Current time differential factor */
static int nextdf = NO_TDF;		/* Next time differential factor */
static int tdftime = 0x7fffffff;	/* Time of next TDF change */


/*
 * Routine to compute the number of ticks which have occured since
 * the last time an adjustment was started.
 */

static long int baseticks(curtime, basetime)

struct timeval *curtime;	/* Current time */
struct timeval *basetime;	/* Time last adjustment was started */

{
	struct timeval elapsed;	/* Elapsed time since last adjustment was
				   started */

	elapsed.tv_sec = curtime->tv_sec - basetime->tv_sec;
	elapsed.tv_usec = curtime->tv_usec - basetime->tv_usec;

	/* Calculate the number of ticks, if we must approximate
	   round down so we'll overestimate the inaccuracy */

	/* If it won't overflow, compute using micro-seconds */
	if (elapsed.tv_sec < 4000)
		return(((unsigned)elapsed.tv_sec * K_US_PER_SEC +
			elapsed.tv_usec) / (tick + tickdelta));
/* Change the previous lines when kernel is switched to nano-seconds
 *		return(((unsigned)elapsed.tv_sec * (K_NS_PER_SEC / 1000) +
 *			elapsed.tv_nsec / 1000) /
 *		       ((tick + tickdelta + 999) / 1000));
 */
	/* If it won't overflow, compute using milli-seconds */

	if (elapsed.tv_sec < 4000000) {
 		return(((unsigned)elapsed.tv_sec * (K_US_PER_SEC / 1000) +
			elapsed.tv_usec / 1000) /
		       ((tick + tickdelta + 999) / 1000));
/* Change the previous lines when kernel is switched to nano-seconds
 *		return(((unsigned)elapsed.tv_sec * (K_NS_PER_SEC / 1000000) +
 *			elapsed.tv_nsec / 1000000) /
 *		       ((tick + tickdelta + 999999) / 1000000));
 */
	} else {
		/* Compute using seconds */

		struct int64 temp;

		UTC_MUL32P(K_US_PER_SEC / (tick + tickdelta),
/* Change the previous line when kernel is switched to nano-seconds
 *		UTC_MUL32P(K_NS_PER_SEC / (tick + tickdelta), */
		       elapsed.tv_sec, &temp);

		if (temp.hi == 0 && (long)temp.lo > 0)
			return(temp.lo);
	}

	return(0x7fffffff);
}

/*
 * Routine to calculate the drift and resolution contribution
 * to inaccuracy.
 *
 *	Return drift in 100ns units, if infinite (or just too large) return
 *	negative.
 */

static long int driftfactor(curtv, driftbase, mdr)

struct timeval *curtv;		/* Current time */
struct timeval *driftbase;	/* Last time base inaccuracy was
				   calculated */
long int mdr;			/* Value of maxdriftrecip to use */

{
	struct timeval	elapsed;	/* elapsed time since last time
					   base inaccuracy was
					   calculated */
	register long int drift;	/* Contribution to inaccuracy of
					   drift and resolution */

	/* Compute the amount of inaccuracy since the last base time by
	   computing the elapsed time, and scaling by the max drift rate */

	elapsed.tv_sec = curtv->tv_sec - driftbase->tv_sec;
	elapsed.tv_usec = curtv->tv_usec - driftbase->tv_usec +
			  tick + fixtick;

	/* If drift computation won't overflow, compute using micro-seconds */

	if (elapsed.tv_sec < 2000) {
		drift = (elapsed.tv_sec * K_US_PER_SEC +
			 elapsed.tv_usec +
			 mdr - 1) /
			mdr * (K_100NS_PER_SEC / K_US_PER_SEC);
/* Change the previous lines when kernel is switched to nano-seconds
 *		drift = (elapsed.tv_sec * (K_NS_PER_SEC / 1000) +
 *			 (elapsed.tv_nsec + 999) / 1000 +
 *			 mdr - 1) /
 *			mdr * (K_100NS_PER_SEC / (K_NS_PER_SEC / 1000));
 */
	} else {
		/* If drift computation won't overflow compute using
		   milli-seconds */

		if (elapsed.tv_sec < 2000000) {
			drift = ((elapsed.tv_sec * (K_US_PER_SEC / 1000) +
				  (elapsed.tv_usec + 999) / 1000 +
				  mdr - 1) / mdr) *
				 (1000 * (K_100NS_PER_SEC / K_US_PER_SEC));
/* Change the previous lines when kernel is switched to nano-seconds
 *			drift = ((elapsed.tv_sec * (K_NS_PER_SEC / 1000000) +
 *			  	  (elapsed.tv_nsec + 999999) / 1000000 +
 *				  mdr - 1) / mdr) *
 *				 (1000 * (K_100NS_PER_SEC / (K_NS_PER_SEC / 1000)));
 */
		} else {
			return(-1);
		}
	}

	/* Add in resolution */

	drift += (tick + fixtick) * (K_100NS_PER_SEC/K_US_PER_SEC);
/* Change the previous line when kernel is switched to nano-seconds
 *	drift += (tick + fixtick + (K_NS_PER_SEC/K_100NS_PER_SEC) - 1)/
 *               (K_NS_PER_SEC/K_100NS_PER_SEC);
 */

	return(drift);
}

/*
 * Kernel mode get utc timestamp routine
 */

getutc(utc)

struct utc *utc;	/* 128 bit UTC timestamp */

{
	struct int64	atime;		/* Current time */
	struct int64	saveinacc;	/* Last base inaccuracy */
	struct int64	saveleaptime;	/* Time of next potential leap
					   second correction */
	struct timeval	savedriftbase;	/* Time of last base inaccuracy */
	struct timeval	atv;		/* Current time as a timeval */
        int             savetdf;	/* Current time differential factor */
	struct int64	hightime;	/* high end of current time interval */
	struct int64	correc;		/* amount to reduce inaccuracy
					   due to current adjustment */
	int		saveinfinite;	/* Inaccuracy is currently
					   infinite */
	long int	savetickadjcnt;	/* Current number of ticks for
					   which adjustment will occur */
	long int	saveticks;	/* Count of ticks since last
					   adjust started */
	long int	savemdr;	/* Current maxdriftrecip to use
					   for drift calculation */
	long int	drift;		/* Drift contribution to
					   inaccuracy */
	int		s;		/* Temp for saving IPL */

	s = splclock();
	smp_lock(&lk_utctime, LK_RETRY);

	atv = *(timepick);
        savetdf = (atv.tv_sec > tdftime) ? nextdf : curtdf;
	savedriftbase = driftbase;
	saveinacc = inacc;
	saveinfinite = inaccinfinite;
	saveleaptime = leaptime;
	savetickadjcnt = tickadjcount;
	saveticks = baseticks(&atv, &adjbase);
	savemdr = currentmdr;

	smp_unlock(&lk_utctime);
	splx(s);

	/* Convert the time in seconds and micro-seconds to a 64-bit integer
	   number of 100 nanoseconds. */

	UTC_MUL32P(atv.tv_sec, K_100NS_PER_SEC, &atime);
	UTC_ADD32P(atv.tv_usec * (K_100NS_PER_SEC / K_US_PER_SEC), &atime);
/* Change the previous line when kernel is switched to nano-seconds
 *	UTC_ADD32P(atv.tv_nsec / (K_NS_PER_SEC / K_100NS_PER_SEC), &atime);
 */

	/* Compensate for different base times */

	UTC_ADD64(&diff_base_times, &atime);

	/* Compute drift and resolution contribution to inaccuracy */

	drift = driftfactor(&atv, &savedriftbase,
			    (savemdr ? savemdr : cpup->maxdriftrecip));

	/* If not infinite, add in the drift contribution and reduce
	   the inaccuracy by the adjustment amount. Finally check for
	   leap seconds */

	if (drift >= 0 && !saveinfinite) {

		UTC_ADD32P(drift, &saveinacc);

		/* Calculate the amount to reduce inaccuracy from an
		   adjustment */

		if (savetickadjcnt != 0) {
			UTC_MUL32P((saveticks < savetickadjcnt) ?
				    saveticks : savetickadjcnt,
			           tickdelta * (K_100NS_PER_SEC / K_US_PER_SEC),
/* Change the previous line when kernel is switched to nano-seconds
 *			           tickdelta / (K_NS_PER_SEC / K_100NS_PER_SEC),
 */
			       &correc);

			UTC_SUB64(&correc, &saveinacc);
		}

		/* Compute upper end of interval */

		hightime = atime;
		UTC_ADD64(&saveinacc, &hightime);

		/* If upper end of interval is past a potential leap
		   second, add an extra second of inaccuracy */

		if ((hightime.hi > saveleaptime.hi) ||
		    (hightime.hi == saveleaptime.hi &&
		     hightime.lo >= saveleaptime.lo))
			UTC_ADD32P(K_100NS_PER_SEC, &saveinacc);

	}

	/* If inaccuracy has become infinite, just return
	   max value */

	if (drift < 0 || saveinfinite || saveinacc.hi & 0xffff0000) {
		saveinacc.lo = 0xffffffff;
		saveinacc.hi = 0xffff;
	}

	utc->time = atime;
	utc->inacclo = saveinacc.lo;
	utc->inacchi = saveinacc.hi;

	/* If DTS has not yet supplied a time differential factor
           pick up the kernels and use it (this could be off by an hour). */

	if (savetdf == NO_TDF)
	    savetdf = -tz.tz_minuteswest;
	utc->tdf = (savetdf & 0xfff) + K_UTC_VERSION;

}

/*
 * User mode get utc timestamp routine
 *
 *	call the kernel routine and copy the results out.
 */

utc_gettime()
{
	register struct a {
		struct utc *utc;	/* user mode UTC time stamp */
	} *uap = (struct a *)u.u_ap;
	struct utc autc;

	getutc(&autc);

	u.u_error = copyout((caddr_t)&autc, (caddr_t)(uap->utc),
			    sizeof (autc));

}

/*
 * set/adjust utc timestamp routine
 *
 */

utc_adjtime()
{
	register struct a {
		enum adjmode	modeflag;	/* Set/adjust flag
						    0 - Set time
						    1 - Adjust time
						    2 - End adjustment
						    3 = Get clock resolution
						    4 = Get clock max drift
						    5 = Trim clock frequency
						    6 = Get trimmed frequency */
		union adjunion	*argblk;	/* Pointer to argument block
						   for sub-function:
						    0 - *adjargs union member
						    1 - *adjargs union member
						    2 - IGNORED
						    3 - *resolution
						    4 - *maxdriftrecip
						    5 - *trimargs union member
						    6 - *frequency */
							
	} *uap = (struct a *)u.u_ap;
	struct adjargs aargs;			/* Copy of adj arguments */
	struct trimargs targs;			/* Copy of trim arguments */
	/*
	 * For function 0 and 1:
	 *	struct timespec	a_adjustment; 	Amount to adjust or change
	 *	struct timespec	a_comptime;	Time which corresponds to base
	 *					inaccuracy
	 *	struct int64	a_baseinacc;	Base inaccuracy
	 *	struct int64	a_leaptime;	Time of next potential leap
	 *					second
	 *	long int	a_adjrate;	Rate at which to adjust
	 *					    1000 = .1% (.0768% on PMAX)
	 *					    100 = 1% (.999% on PMAX)
	 *					    10 = 10%, etc.
	 *					    Ignored for set time
	 *	long int	a_curtdf;	Current timezone
	 *	long int	a_nextdf;	Next timezone (Daylight time)
	 *	long int	a_tdftime;	Time of next timezone change
	 * For function 3:
	 *	long int	resolution;	Resolution of clock in nanosecs
	 * For function 4:
	 *	long int	maxdrift;	Maximun drift rate of clock
	 * For function 5:
	 *	long int	t_frequency;	New frequency trim of clock
	 *	long int	t_maxdrift;	New maximun drift rate of clock
	 * For function 6:
	 *	long int	frequency;	Current frequency trim of clock
	 */
	struct timeval atv;		/* Temporary time value */
	struct timezone atz;		/* New timezone */
	long resolution;                /* Resolution of clock */
	long frequency;			/* frequency trim of clock */
	long ndelta;			/* New delta value */
	int saveaffinity;		/* Temp for saving affinity */
	int s;				/* Temp for saving IPL */
	int newinfinite = 0;		/* Flag for new inaccinfinite */
	long int n;			/* Random temp */

	/*
	 * Validate arguments
	 */

	switch (uap->modeflag) {

	    case settime:
	    case adjusttime:

		u.u_error = copyin((caddr_t)(uap->argblk),
				   (caddr_t)&aargs, sizeof (aargs));
		if (u.u_error)
			return;
		if (aargs.a_adjrate == 0)
			aargs.a_adjrate = 100;	/* Default to 1% */
		if (aargs.a_adjrate < 0 ||
		    aargs.a_adjrate > tick/2 ) {
			u.u_error = EINVAL;
			return;
		}
		if (aargs.a_adjustment.tv_nsec > K_NS_PER_SEC ||
		    aargs.a_adjustment.tv_nsec < -K_NS_PER_SEC) {
			u.u_error = EINVAL;
			return;
		}
		if (aargs.a_adjustment.tv_nsec < 0) {
			aargs.a_adjustment.tv_nsec += K_NS_PER_SEC;
			aargs.a_adjustment.tv_sec--;
		}
		if (uap->modeflag == adjusttime &&
				     (aargs.a_adjustment.tv_sec > 2000 ||
				      aargs.a_adjustment.tv_sec < -2000)) {
			u.u_error = EINVAL;
			return;
		}
		if (aargs.a_comptime.tv_sec == 0) {
			newinfinite = 1;
		}
		break;

	    case endadjust:

		break;

	    case getresolution:

		resolution = (tick + fixtick) * (K_NS_PER_SEC/K_US_PER_SEC);
/* Change the previous line when kernel is switched to nano-seconds
 *              resolution = tick + fixtick;
 */
		u.u_error = copyout((caddr_t)&resolution, 
				    (caddr_t)(uap->argblk),
				    sizeof (long));
		return;

	    case getdrift:

		if (nextmdr == 0)
			nextmdr = cpup->maxdriftrecip;
		u.u_error = copyout((caddr_t)&nextmdr,
				    (caddr_t)(uap->argblk),
				    sizeof (long));
		return;

	    case setfreq:

		u.u_error = copyin((caddr_t)(uap->argblk),
				   (caddr_t)&targs, sizeof (targs));
		if (u.u_error)
			return;
		/* Maximum drift rate must be less than 1%! */
		if (targs.t_maxdrift <= 100) {
			u.u_error = EINVAL;
			return;
		}
		break;

	    case getfreq:

		frequency = (tick * hz + fixtick) *
			    (K_NS_PER_SEC/K_US_PER_SEC);
/* Change the previous lines when kernel is switched to nano-seconds
 *              frequency = tick * hz + fixtick;
 */

		u.u_error = copyout((caddr_t)&frequency,
				    (caddr_t)(uap->argblk),
				    sizeof (long));
		return;

	    default:

		u.u_error = EINVAL;
		return;

	}

	/*
	 * Validate privleges
	 */

	if(!suser()) return;

	/*
         * The following code must all be locked against clock ticks. This
         * is achieved by switching to the boot cpu and raising IPL to the
         * clock IPL. If timekeeping changes in some grand way, this strategy
         * will need to be changed! Note parts of this code are also locked
         * against utc_gettime by the private lock lk_utctime.
         */

	saveaffinity = switch_affinity(boot_cpu_mask);
	s = splclock();

	switch (uap->modeflag) {
	    
	    case settime:	/* Set time */

		boottime.tv_sec += aargs.a_comptime.tv_sec - time.tv_sec;
		atv = *(timepick);
   		atv.tv_sec += aargs.a_adjustment.tv_sec;
		atv.tv_usec += aargs.a_adjustment.tv_nsec /
                               ( K_NS_PER_SEC / K_US_PER_SEC );
		if (atv.tv_usec >= K_US_PER_SEC) {
			atv.tv_usec -= K_US_PER_SEC;
			atv.tv_sec++;
		}
		if (timepick == &atime) {
			btime = atv;
			smp_lock(&lk_utctime, LK_RETRY);
			timepick = &btime;
		} else {
			atime = atv;
			smp_lock(&lk_utctime, LK_RETRY);
			timepick = &atime;
		}
		tickdelta = 0;
		timedelta = 0;
		time = adjbase = atv;
		driftbase.tv_sec = aargs.a_comptime.tv_sec;
		driftbase.tv_usec = aargs.a_comptime.tv_nsec /
                                    ( K_NS_PER_SEC / K_US_PER_SEC );
		inacc = aargs.a_baseinacc;
		leaptime = aargs.a_leaptime;
		tickadjcount = 0;
		inaccinfinite = newinfinite;
		curtdf = aargs.a_curtdf;
		nextdf = aargs.a_nextdf;
		tdftime = aargs.a_tdftime;
		currentmdr = nextmdr;
		smp_unlock(&lk_utctime);
		resettodr();
		break;
		
	    case adjusttime:	/* Adjust time */

/* When the kernel is switched to nano-seconds, the variables timedelta and
 * tickdelta will probably be changed. A simple change of units will not
 * surfice since then the range will be too small. This code will need to be
 * reworked. (As will all the code that supports adjtime) */

		ndelta = aargs.a_adjustment.tv_sec * K_US_PER_SEC +
			 aargs.a_adjustment.tv_nsec /
                         ( K_NS_PER_SEC / K_US_PER_SEC );

		smp_lock(&lk_utctime, LK_RETRY);
		atv = *(timepick);
		tickdelta = tick / aargs.a_adjrate;
		if (ndelta % tickdelta)
			ndelta = ndelta / tickdelta * tickdelta;
		timedelta = ndelta;
		adjbase = atv;
		driftbase.tv_sec = aargs.a_comptime.tv_sec;
		driftbase.tv_usec = aargs.a_comptime.tv_nsec /
                                    ( K_NS_PER_SEC / K_US_PER_SEC );
		inacc = aargs.a_baseinacc;
		leaptime = aargs.a_leaptime;
		tickadjcount = ((ndelta >= 0) ? ndelta : -ndelta) / tickdelta;
		inaccinfinite = newinfinite;
		curtdf = aargs.a_curtdf;
		nextdf = aargs.a_nextdf;
		tdftime = aargs.a_tdftime;
		currentmdr = nextmdr;
		smp_unlock(&lk_utctime);
		break;

	    case endadjust:	/* End adjustment */

		timedelta = 0;
		n = baseticks(timepick, &adjbase);
		if (n < tickadjcount)
			tickadjcount = n;
		break;

	    case setfreq:	/* Trim frequency of clock */

		/* The units of frequency are nanoseconds/second, we compute
                   a new tick and fixtick so that after hz ticks, the
                   clock has advanced targs.t_frequency nanoseconds! */
		/* For micro-second version we compute using nanoseconds and
                   round to get as close as we can */

		smp_lock(&lk_utctime, LK_RETRY);
		tick = targs.t_frequency / hz /
                        (K_NS_PER_SEC/K_US_PER_SEC);
		fixtick = ((targs.t_frequency -
			    (tick * hz) * (K_NS_PER_SEC/K_US_PER_SEC)) +
			   (K_NS_PER_SEC/K_US_PER_SEC/2)) /
                          (K_NS_PER_SEC/K_US_PER_SEC);
/* When kernel is switch to nanoseconds, replace preceeding two statements
 * with:
 *		tick = targs.t_frequency / hz;
 *		fixtick = targs.t_frequency - tick * hz;
 */
		/* Set the new current and next maxdriftrecip. The nextmdr
		   is set to the new value, unless zero is specified - then
		   it is reverted to the hardware's default. currentmdr is
		   set to the minimum of its current value and the new
		   value. Remember you divide by mdr to get the drift, so
		   we overestimate the drift by using the minimum. */
		if ((nextmdr = targs.t_maxdrift) == 0)
			nextmdr = cpup->maxdriftrecip;
		if (currentmdr == 0)
			currentmdr = cpup->maxdriftrecip;
		if (currentmdr > nextmdr)
			currentmdr = nextmdr;

		smp_unlock(&lk_utctime);
		
	}

	splx(s);
	(void) switch_affinity(saveaffinity);

}

