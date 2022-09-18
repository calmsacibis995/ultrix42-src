#ifndef lint
static char *sccsid = "@(#)kern_time.c	4.2	(ULTRIX)	9/4/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986, 88 by			*
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
 *
 * 16-Apr-90 -- jaw
 *	performance fixes for single cpu.
 *
 * 18-Jan-90 -- Joe Comuzzi
 *      Set the time inaccuracy to infinite when setting or adjusting
 *      time though the old time syscalls (settimeofday, adjtime). 
 *
 * 02-Jan-90 -- jaw
 *	pick-up dropped change for mips in hzto.
 *
 * 02-Dec-89 -- afd
 *	Fixed the PMAX/3MAX time slipage that was caused due to loosing
 *	64 uSECs every second.
 *
 * 16 Oct 89 -- terry
 *      extend smp lock duration to cover 'time' change in settimeofday
 *      correct else clause in settimeofday to activate atime/btime switching
 *
 * 10-Oct-88 -- jaw
 *	replace switch_to_master with general routine switch_affinity
 *
 * 23 Aug 88 -- miche
 *	note that the psignal call is safe.
 *
 * 28 Dec 87 -- gmm (sccs 6.x)
 *	SMP changes
 *
 * 23 DEC 87 -- jaw
 *	fix system hanging when time changed in multi-user mode.
 *
 * 19 Feb 86 -- bjg
 *	Add check for super user when log time change
 *
 * 20 Jan 86 -- bjg
 *	Added error logging of time change
 *
 * 20 Mar. -- rjl
 *	Fixed settimeofday to set the timezone.
 *
 *
 * ---------------------------------------------------------------------
 */


#include "../machine/reg.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax

#include "../h/param.h"
#include "../h/smp_lock.h"
#include "../h/dir.h"		/* XXX */
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/errlog.h"
#include "../h/cpudata.h"

/* 
 * Time of day and interval timer support.
 *
 * These routines provide the kernel entry points to get and set
 * the time-of-day and per-process interval timers.  Subroutines
 * here provide support for adding and subtracting timeval structures
 * and decrementing interval timers, optionally reloading the interval
 * timers when they expire.
 */
extern int inaccinfinite;
extern struct timeval atime;
extern struct timeval btime;
extern struct timeval *timepick;
extern int fixtick;		/* set in knxxinit for mc146818 clock chip */

gettimeofday()
{
	register struct a {
		struct	timeval *tp;
		struct	timezone *tzp;
	} *uap = (struct a *)u.u_ap;
	struct timeval atv;

	splclock();

	atv = *(timepick); 

	spl0();
	u.u_error = copyout((caddr_t)&atv, (caddr_t)uap->tp, sizeof (atv));
	if (u.u_error)
		return;
	if (uap->tzp == 0)
		return;
	/* SHOULD HAVE PER-PROCESS TIMEZONE */
	u.u_error = copyout((caddr_t)&tz, (caddr_t)uap->tzp, sizeof (tz));
}

settimeofday()
{
	register struct a {
		struct	timeval *tv;
		struct	timezone *tzp;
	} *uap = (struct a *)u.u_ap;
	struct timeval atv;
	struct timezone atz;
	extern struct el_rec *ealloc();
	struct el_rec *elp;
	int saveaffinity,s;

	u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
		sizeof (struct timeval));
	if (u.u_error)
		return;
	if (uap->tzp) {
		u.u_error = copyin((caddr_t)uap->tzp, (caddr_t)&atz,
			sizeof (struct timezone));
		if (u.u_error)
			return;
	}
	if(!suser()) return;

 	saveaffinity = switch_affinity(boot_cpu_mask);
	/* WHAT DO WE DO ABOUT PENDING REAL-TIME TIMEOUTS??? */
	s = splclock(); 
	inaccinfinite = 1;		/* set utc inaccuracy infinite */
	boottime.tv_sec += atv.tv_sec - time.tv_sec;
	if (timepick == &atime) {
		btime = atv;
		smp_lock(&lk_timeout, LK_RETRY);
		timepick = &btime;
	} else {
		atime = atv;
		smp_lock(&lk_timeout, LK_RETRY);
		timepick = &atime;
	}
	time = atv;
        smp_unlock(&lk_timeout);
	if(uap->tzp)
		tz = atz;
	resettodr();
	splx(s);
 	(void) switch_affinity(saveaffinity);


#ifndef RELEASE
#define RELEASE ""
#endif
	elp = ealloc(sizeof(struct el_timchg), EL_PRILOW);
	if (elp != EL_FULL) {
		LSUBID(elp, ELMSGT_TIM, EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
		bcopy((caddr_t)&atv, (caddr_t)&elp->el_body.eltimchg.timchg_time, sizeof(struct timeval));
		bcopy((caddr_t)&atz, (caddr_t)&elp->el_body.eltimchg.timchg_tz, sizeof(struct timezone));

		bcopy("ultrix ", (caddr_t)elp->el_body.eltimchg.timchg_version, 7);
		bcopy((caddr_t)RELEASE, (caddr_t)&elp->el_body.eltimchg.timchg_version[7], 4);
		EVALID(elp);
	}
	
}

extern	int tickadj;			/* "standard" clock skew, us./tick */
int	tickdelta;			/* current clock skew, us. per tick */
long	timedelta;			/* unapplied time correction, us. */
long	bigadj = 1000000;		/* use 10x skew above bigadj us. */

adjtime()
{
	register struct a {
		struct timeval *delta;
		struct timeval *olddelta;
	} *uap = (struct a *)u.u_ap;
	struct timeval atv, oatv;
	register long ndelta;
	int s;

	if (!suser()) 
		return;
	u.u_error = copyin((caddr_t)uap->delta, (caddr_t)&atv,
		sizeof (struct timeval));
	if (u.u_error)
		return;
	ndelta = atv.tv_sec * 1000000 + atv.tv_usec;
	s = splclock();
	smp_lock(&lk_timeout, LK_RETRY);
	inaccinfinite = 1;		/* set utc inaccuracy infinite */
	if (timedelta == 0)
		if (ndelta > bigadj)
			tickdelta = 10 * tickadj;
		else
			tickdelta = tickadj;
	if (ndelta % tickdelta)
		ndelta = ndelta / tickadj * tickadj;

	if (uap->olddelta) {
		oatv.tv_sec = timedelta / 1000000;
		oatv.tv_usec = timedelta % 1000000;
	}
	timedelta = ndelta;
	smp_unlock(&lk_timeout);
	splx(s);

	if (uap->olddelta)
		(void) copyout((caddr_t)&oatv, (caddr_t)uap->olddelta,
			sizeof (struct timeval));
}


/*
 * Get value of an interval timer.  The process virtual and
 * profiling virtual time timers are kept in the u. area, since
 * they can be swapped out.  These are kept internally in the
 * way they are specified externally: in time until they expire.
 *
 * The real time interval timer is kept in the process table slot
 * for the process, and its value (it_value) is kept as an
 * absolute time rather than as a delta, so that it is easy to keep
 * periodic real-time signals from drifting.
 *
 * Virtual time timers are processed in the hardclock() routine of
 * kern_clock.c.  The real time timer is processed by a timeout
 * routine, called from the softclock() routine.  Since a callout
 * may be delayed in real time due to interrupt processing in the system,
 * it is possible for the real time timeout routine (realitexpire, given below),
 * to be delayed in real time past when it is supposed to occur.  It
 * does not suffice, therefore, to reload the real timer .it_value from the
 * real time timers .it_interval.  Rather, we compute the next time in
 * absolute time the timer should go off.
 */
getitimer()
{
	register struct a {
		u_int	which;
		struct	itimerval *itv;
	} *uap = (struct a *)u.u_ap;
	struct itimerval aitv;
	int s;
	struct timeval savetime;

	if (uap->which > 2) {
		u.u_error = EINVAL;
		return;
	}
	s = splclock();
	if (uap->which == ITIMER_REAL) {
		/*
		 * Convert from absoulte to relative time in .it_value
		 * part of real time timer.  If time for real time timer
		 * has passed return 0, else return difference between
		 * current time and time for the timer to go off.
		 */
		smp_lock(&lk_realtimer,LK_RETRY);
		aitv = u.u_procp->p_realtimer;
		smp_unlock(&lk_realtimer);
		if (timerisset(&aitv.it_value)) {
			savetime = *(timepick);
			if (timercmp(&aitv.it_value, &savetime, <))
				timerclear(&aitv.it_value);
			else
				timevalsub(&aitv.it_value, &savetime);
		}
	} else
		aitv = u.u_timer[uap->which];
	splx(s);
	u.u_error = copyout((caddr_t)&aitv, (caddr_t)uap->itv,
	    sizeof (struct itimerval));
}

setitimer()
{
	register struct a {
		u_int	which;
		struct	itimerval *itv, *oitv;
	} *uap = (struct a *)u.u_ap;
	struct itimerval aitv;
	int s;
	register struct proc *p = u.u_procp;
	struct timeval savetime;

	if (uap->which > 2) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyin((caddr_t)uap->itv, (caddr_t)&aitv,
	    sizeof (struct itimerval));
	if (u.u_error)
		return;
	if (uap->oitv) {
		uap->itv = uap->oitv;
		getitimer();
	}
	if (itimerfix(&aitv.it_value) || itimerfix(&aitv.it_interval)) {
		u.u_error = EINVAL;
		return;
	}
	s = splclock();
	if (uap->which == ITIMER_REAL) {
		untimeout(realitexpire, (caddr_t)p);
		if (timerisset(&aitv.it_value)) {
			savetime = *(timepick); 
			timevaladd(&aitv.it_value, &savetime);
			smp_lock(&lk_realtimer,LK_RETRY);
			p->p_realtimer = aitv;
			smp_unlock(&lk_realtimer);
			timeout(realitexpire, (caddr_t)p, mp_hzto(&aitv.it_value,&savetime));

		} else {
			smp_lock(&lk_realtimer,LK_RETRY);
			p->p_realtimer = aitv;
			smp_unlock(&lk_realtimer);
		}
	} else
		u.u_timer[uap->which] = aitv;
	splx(s);
}

/*
 * Real interval timer expired:
 * send process whose timer expired an alarm signal.
 * If time is not set up to reload, then just return.
 * Else compute next time timer should go off which is > current time.
 * This is where delay in processing this timeout causes multiple
 * SIGALRM calls to be compressed into one.
 *
 * The psignal in this routine requires that the caller ensure
 * that 'p' will not disappear until the psignal has completed.
 * This works because interval timers are removed in exit.
 */
realitexpire(p)
	register struct proc *p;
{
	register int s;
	register int diff,i, iterations;
	struct timeval savetime;

	psignal(p, SIGALRM);


	s = splclock();
	savetime = *(timepick); 

	smp_lock(&lk_realtimer,LK_RETRY);
	/* is the interval value set....if not clear out
	   variable in proc structure for alarm time and
	   leave */
	if (!timerisset(&p->p_realtimer.it_interval)) {
		timerclear(&p->p_realtimer.it_value);
		smp_unlock(&lk_realtimer);
		splx(s);
		return;
	}
	/* did time go backward or forwards by more then 10 seconds
	   since last interval? if so then use current time as the
	   base for the next interval. */

	diff = p->p_realtimer.it_value.tv_sec - time.tv_sec;

	if ((diff > 10) || (diff < -10)) {
		p->p_realtimer.it_value = time;   	
	}

	/* schedule the next "alarm" if interval was set.  This
	   code adds the interval into the time the "alarm" was
	   suppose to go off.  It keeps doing this until the time
	   for the next "alarm" is greater then the current time.
	   The above check was made so that we don't hang the system
	   if someone changes system time forward or backwards by
	   a noticeable amount.  Note that since this routine is
	   run at softclock interrupt level, the system time can
	   not be change by a user.  This will not be valid for 
	   SMP.
	   SMP NOTE....  we use "savetime" so that time cannot 
	   change under us.
	*/
	for (;;) {
		timevaladd(&p->p_realtimer.it_value,
		    &p->p_realtimer.it_interval);
		if (timercmp(&p->p_realtimer.it_value, &savetime, >)) {
			smp_unlock(&lk_realtimer);
			timeout(realitexpire, (caddr_t)p,
			    mp_hzto(&p->p_realtimer.it_value,&savetime));
			splx(s);
			return;
		}
	}
}

/*
 * Check that a proposed value to load into the .it_value or
 * .it_interval part of an interval timer is acceptable, and
 * fix it to have at least minimal value (i.e. if it is less
 * than the resolution of the clock, round it up.)
 */
itimerfix(tv)
	struct timeval *tv;
{

	if (tv->tv_sec < 0 || tv->tv_sec > 100000000 ||
	    tv->tv_usec < 0 || tv->tv_usec >= 1000000)
		return (EINVAL);
	if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < tick)
		tv->tv_usec = tick;
	return (0);
}

/*
 * Decrement an interval timer by a specified number
 * of microseconds, which must be less than a second,
 * i.e. < 1000000.  If the timer expires, then reload
 * it.  In this case, carry over (usec - old value) to
 * reducint the value reloaded into the timer so that
 * the timer does not drift.  This routine assumes
 * that it is called in a context where the timers
 * on which it is operating cannot change in value.
 */
itimerdecr(itp, usec)
	register struct itimerval *itp;
	int usec;
{

	if (itp->it_value.tv_usec < usec) {
		if (itp->it_value.tv_sec == 0) {
			/* expired, and already in next interval */
			usec -= itp->it_value.tv_usec;
			goto expire;
		}
		itp->it_value.tv_usec += 1000000;
		itp->it_value.tv_sec--;
	}
	itp->it_value.tv_usec -= usec;
	usec = 0;
	if (timerisset(&itp->it_value))
		return (1);
	/* expired, exactly at end of interval */
expire:
	if (timerisset(&itp->it_interval)) {
		itp->it_value = itp->it_interval;
		itp->it_value.tv_usec -= usec;
		if (itp->it_value.tv_usec < 0) {
			itp->it_value.tv_usec += 1000000;
			itp->it_value.tv_sec--;
		}
	} else
		itp->it_value.tv_usec = 0;		/* sec is already 0 */
	return (0);
}

/*
 * Add and subtract routines for timevals.
 * N.B.: subtract routine doesn't deal with
 * results which are before the beginning,
 * it just gets very confused in this case.
 * Caveat emptor.
 */
timevaladd(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}

timevalsub(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

timevalfix(t1)
	struct timeval *t1;
{

	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}
