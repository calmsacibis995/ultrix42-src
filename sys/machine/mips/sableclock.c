/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Sable clock routines.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"

#include "../machine/cpu.h"
#include "../machine/sableclock.h"

/*
 * Start the real-time clock (really the programable interval clock!).
 */

startrtclock()
{
	struct sable_clock *scp = SABLE_CLOCK_BASE;

	splhigh();
	scp->sc_sched_hz = hz;
	scp->sc_command |= SC_SCHED_START;
	spl0();
}

stopclocks()
{
	struct sable_clock *scp = SABLE_CLOCK_BASE;

	splhigh();
	scp->sc_command &= ~(SC_SCHED_START|SC_PROF_START);
	spl0();
}

ackrtclock()
{
	struct sable_clock *scp = SABLE_CLOCK_BASE;

	scp->sc_command |= SC_SCHED_ACK;
}

/*
 * Initialize the time of day register, based on the time base which is
 * from a filesystem.  Base provides the time to within six months.
 * For this sable implimentation, we use the todr as is and ignore
 * the time from the filesystem.
 */
/*ARGSUSED*/
inittodr(base)
	time_t base;
{
	struct sable_clock *scp = SABLE_CLOCK_BASE;

	time.tv_sec = scp->sc_todr;
	time.tv_usec = 0;
	/* TODO: warn if filesystem date is bogus? */
}

/*
 * For sable do nothing, we cannot reset the hosts clock!
 */
resettodr()
{
}

#ifdef STATCLOCK
int	phz = 0;

/*
 * STATCLOCK is used as an independent time base for statistics gathering.
 */
startstatclock()
{
	struct sable_clock *scp = SABLE_CLOCK_BASE;

	splhigh();
	scp->sc_prof_hz = phz;
	scp->sc_command |= SC_PROF_START;
	phz = 500;	/* pseudo-hz */
	spl0();
}
#endif STATCLOCK

/*
 * Acknowledge the profiling clock tick
 */
ackstatclock()
{
	struct sable_clock *scp = SABLE_CLOCK_BASE;

	scp->sc_command |= SC_PROF_ACK;
}

microtime(tvp)
	register struct timeval *tvp;
{
	*tvp = time;
}
