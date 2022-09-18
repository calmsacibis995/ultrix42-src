#ifndef lint
static char *sccsid = "@(#)kern_clock.c	4.5    ULTRIX  3/6/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,88 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/kern_clock.c
 *
 * 06-Mar-91 -- jaw
 *	fix bug in fixtick code for mips
 *
 * 17-May-90 -- gmm
 *	fix a bug where if a timeout was requested immediately (through
 *	select()), it can get delayed depending on the nature of the timeout
 *	queue.
 *
 * 30-Mar-90 -- jaw
 *	allow soft interrupt to be set any time.  Needed for MSI disk
 *	performance reasons.
 *
 * 14-Feb-90 -- jaw
 *	fix race condition of untimeout in mp system.
 *
 * 12-Jan-90 -- jaw
 *	fix to "fix affintity" routine.
 *
 * 02-Jan-90 -- jaw
 *	pick-up dropped change for mips in hzto.
 *
 * 11-Dec-89 -- burns
 *	Put back the ability of hardclock to call softclock directly
 *	now that the real bug is fixed in trap.c (see 27-Sep-89)
 *
 * 02-Dec-89 -- afd
 *	Fixed the PMAX/3MAX time slipage that was caused due to loosing
 *	64 uSECs every second.
 *
 * 14-Nov-89 -- sekhar
 * 	Fixes to turn profiling off when scale set to 1.
 *
 * 09-Nov-89 -- jaw
 *  	change timeout queue handling.
 *
 * 16-Oct-89 -- Adrian Thoms
 *    Move lk_timeout lock out of VVAX path to enable boot
 *    Fix to avoid cumulative error on timeout q for VVAX
 *
 * 27-Sep-89 -- burns
 *	Stopped hardclock from ever calling softclock directly for mips
 *	based systems. The vax hardware has built in protection that allows
 *	the direct call. On mips based systems we run into problems of
 *	trying to service the same interrupt twice.
 *
 * 21-Sep-89 -- Pete Keilty
 *	The kernel subroutine scheduler no longer sets the IPL from the
 *	kschedblk structure. The routine being called now does the IPL
 * 	synchronizing.
 *
 * 	Removed ipl passing from ksched routine. Also changed timeout so
 *	ipl is not put in callout structure and not used in softclock.
 *	Removed AILtimeout routine no longer needed.
 *
 * 20-Jul-89 jaw
 *	debug code cleanup and fix to roundrobin.
 *
 * 30-May-89	darrell
 *	Added include of ../../machine/common/cpuconf.h -- cpu types
 *	were moved there.
 *
 * 5-May-89 -- Adrian Thoms
 *	VVAX support: split machine elapsed time from real elapsed time
 *	Modified to cope with random time increments of less than a tick
 *	up to 10 seconds.
 *
 *  26-Jan-89	jaw
 *	SMP clean up 
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 28-Jul-88 -- miche
 *	protect remaining scheduling fields in the process structure.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 ********* SMP changes above ********
 *
 * 17-Feb-89 -- map (Mark Parenti)
 *	Add #ifdefs around splhigh() and use spl7() in vax mode.
 *	This matches V3.0 behavior as spl7() does not equal splhigh()
 *	on the VAX.
 *
 * 08-Sep-88 -- afd (Al Delorey)
 *	Check for cache parity errors & keep a counter in hardclock()
 *
 * 28-Sep-87 -- Ali Rafieymehr
 *	Fixed a bug (protected the xstack). Also fixed a bug which was causing
 *	the screen to blink.
 *
 * 09-Sep-87 -- JAW
 *	Performance fix to gather stat.  2% loss in performance.
 *
 * 25-Jun-87 -- Brian Stevens
 *	Put in a timer for X server.
 *
 * 21-Apr-87 -- Brian Stevens
 *	Fix bugs associated with shutting down the kernel based X server.
 *
 * 16-Apr-87 -- Fred Canter (mostly for Brian Stevens)
 *	Multi-head GPX support.
 *	Move stack(s) to xos_data.
 *
 * 19-Mar-87 -- Fred Canter (for Brian Stevens)
 *	Integrate scheduling X in the kernel off of the clock.
 *
 * 16-Jul-86 -- Todd M. Katz
 *	Add a kernel subroutine scheduler.  This mechanism consists of:
 *		1. ksched - A kernel subroutine scheduling routine.
 *		2. unksched - A kernel subroutine unscheduling routine.
 *		3. Changes to softclock().  When softclock() is invoked
 *		   all currently scheduled kernel subroutines are
 *		   asynchronously invoked prior to processing of due
 *		   periodic events on the timeout queue.
 *
 * 15-Jul-86 -- rich
 *	Added hooks for the adjusttime system call to hard clock and a
 * 	minor change to bumptime.
 *
 * 02-Apr-86 -- jrs
 *	Clean up so that single cpu tick updates run cleaner
 *
 * 18-Mar-86 -- jrs
 *	Clean up cpu determination and preemption
 *
 * 03-Mar-86 -- jrs
 *	Change gatherstats to record disk usage only when master cycles.
 *	cpu utilization is gathered as aggregate average of all processors.
 *	This allows for possible count of "missed" cycles externally.
 *
 * 23 Jul 85 -- jrs
 *	Add multicpu scheduling
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 16-Apr-85 -- lp
 *	Changed BUMPTIME to a macro. Added ipl 7 interrupt queues
 *	for interrupt burdened devices.
 *
 * 11 Mar 85 -- depp
 *	Added System V shared memory support.
 *
 * 24 Oct 84 -- jrs
 *	Added changes to limit soft clock calling
 *	Derived from 4.2BSD, labeled:
 *		kern_clock.c 6.7	84/05/22
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/callout.h"
#include "../h/dir.h"
#include "../h/ksched.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/smp_lock.h"
#include "../h/cpudata.h"
#include "../h/dk.h"

#if defined(vax)
# include "../machine/mtpr.h"
# include "../machine/clock.h"
#endif vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"

#ifdef vax
#ifdef GPROF
#include "../h/gprof.h"
#endif GPROF
#endif vax

#ifdef mips
#ifdef PROFILING
#include "../h/prof.h"
#include "../h/kernel.h"
#endif PROFILING
#endif mips

#include "../h/ipc.h"
#include "../h/shm.h"
extern struct sminfo sminfo;
extern struct pte Sysmap[];

/*
 * Bump a timeval by a small number of usec's. (macro)
 */

#define BUMPTIME(t, usec) {			\
	register struct timeval *tp = (t); 	\
	tp->tv_usec += (usec); 			\
	if (tp->tv_usec >= 1000000) { 		\
		tp->tv_usec -= 1000000; 	\
		(tp)->tv_sec++; 		\
	} 					\
}

/*
 * DECR_NONNEG(x, d)
 *	expression macro which decrements x by d but does not allow
 *	x to be negative, any remainder is left in d.
 *	The macro takes the value of x after the operation is performed
 *	Think of it as a smart x-=d instruction.
 */
#define DECR_NONNEG(x, d)	(( (int)((x)=(x)-(d)) < 0) ?		\
				 ((d)=(-(x)), (x)=0) : ((d)=0, (x)))


/*
 * Clock handling routines.
 *
 * This code is written to operate with two timers which run
 * independently of each other. The main clock, running at hz
 * times per second, is used to do scheduling and timeout calculations.
 * The second timer does resource utilization estimation statistically
 * based on the state of the machine phz times a second. Both functions
 * can be performed by a single clock (ie hz == phz), however the
 * statistics will be much more prone to errors. Ideally a machine
 * would have separate clocks measuring time spent in user state, system
 * state, interrupt state, and idle state. These clocks would allow a non-
 * approximate measure of resource utilization.
 */

/*
 * TODO:
 *	time of day, system/user timing, timeouts, profiling on separate timers
 *	allocate more timeout table slots when table overflows.
 */
struct lock_t lk_timeout;

/*
 * The hz hardware interval timer.
 * We update the events relating to real time.
 * If this timer is also being used to gather statistics,
 * we run through the statistics gathering routine as well.
 */

/*ARGSUSED*/

extern int elwakeup;
struct timeval atime;
struct timeval btime;
struct timeval *timepick;
int fixtick = 0;		/* set in knxxinit for mc146818 clock chip */
int fixcnt = 0;			/* for systems with mc146818 clock chip this
				   counts the ticks till we need to correct
				   the time with "fixtick" */

#ifdef vax
long saved_usecs;             /* We get interrupts on odd usecs on VVAX */
#endif

#ifdef vax
/*ARGSUSED*/
hardclock(pc, ps)
	caddr_t pc;
	int ps;
#endif vax
#ifdef mips
extern int cpecount;		/* count of cache parity errors */

hardclock(intr_frame)
	u_int *intr_frame;
#endif mips
{
	register struct callout *p1;
	register struct proc *p;
	register int s, cpstate;
	int needsoft = 0;
	register struct cpudata *pcpu;
	int time_update;
	extern int tickdelta;
	extern long timedelta;
	int i;
	register int decr;	/* Number of ticks elapsed */
#ifdef vax
	register long walltick;	/* number of micro-seconds since last intr. */
				/* (equal to tick unless VVAX) */
	long seconds_diff = 0;	/* Diff in time.tv_sec since last intr. */
	int time_has_changed = 0; /* Time has been stomped (VVAX) */
	struct timeval *newtime; /* The time from VVAX */
	struct timeval *get_vvax_time(); /* Get the time from VVAX */
#endif

#ifdef mips

	ackrtclock();
	if (intr_frame[EF_SR] & SR_PE) {
		cpecount++;
		clearcpe();
	}
#endif mips

	pcpu=CURRENT_CPUDATA;
	if (BOOT_CPU) {
                /*
                 * Time is kept in microsecs. The value of tick is
                 * computed in microsecs as 1000000/hz.  If `hz' does
                 * not divide evenly into a million, then fixtick is set
                 * to the remainder (the amount to catch up each second)
                 * in the knxxinit() routine.
                 */
                if (fixtick) {
                        fixcnt++;
                        if (fixcnt >= hz) {
				/* fix-up of usec (if overflow) will be done
				   below when we add in the current tick.  If
				   fix-up is done here, "timepick" will not
				   be updated. */
				time.tv_usec += fixtick;
                                fixcnt = 0;
                        }
                }
#ifdef vax
		if (cpu == V_VAX) {
			newtime = get_vvax_time();

			/* If time has increased by more than 10 seconds
			 * or has gone back then we assume that time has
			 * been changed under our feet
			 */
			seconds_diff = newtime->tv_sec - time.tv_sec;
			if (seconds_diff < 0 || seconds_diff > 10) {
				/* this check needed because walltick
				 * calculation may wrap if time has been changed
				 */
				time_has_changed++;
			} else {
				walltick = (seconds_diff * 1000000 +
					    (newtime->tv_usec - time.tv_usec));
				/* We make an intelligent guess as to
				 * whether the underlying time has been changed */
				if (walltick < 0 || walltick > 10000000) {
					time_has_changed++;
					walltick = tick;
				} else {
					walltick += saved_usecs;
				}
			}
			/* decr will sometime be 0 but that's OK */
			decr = walltick / tick;
			saved_usecs = walltick % tick;
		} else
#endif
		{
			decr = 1;
			smp_lock(&lk_timeout,LK_RETRY);
		}


			/*
			 * Update real-time timeout queue.
			 * At front of queue are some number of events which are ``due''.
			 * The time to these is <= 0 and if negative represents the
			 * number of ticks which have passed since it was supposed to happen.
			 * The rest of the q elements (times > 0) are events yet to happen,
			 * where the time for each is given as a delta from the previous.
			 * Decrementing just the first of these serves to decrement the time
			 * to all events.
			 */
			if (decr) {
				for (i=lowcpu; i<= highcpu; i++) {
					struct cpudata *c;
					
					if (!(c=CPUDATA(i))) continue;
					p1=c->cpu_call_fwd;
					while(p1) {
					     if (p1->c_time > 0) {
					     	p1->c_time -= decr;
					     	if (p1->c_time <= 0)
							c->cpu_call_pending =1;
						break;
					     } 
					     else if (p1->c_time == 0)
						     c->cpu_call_pending = 1;
					     p1=p1->c_next;
					}
				}	
			}
			if(timedelta == 0) {
				time_update = tick;
			} else {
				if (timedelta < 0) {
					time_update = tick - tickdelta;
					timedelta += tickdelta;
				} else {
					time_update = tick + tickdelta;
					timedelta -= tickdelta;
				}
			}
#ifdef vax
		if (cpu == V_VAX) {
			/* V_VAX is always a uni-processor */
			atime = time = *newtime;
		} else
#endif
		{

			time.tv_usec += time_update;
			if (time.tv_usec >= 1000000) {
				time.tv_usec -= 1000000;
				time.tv_sec++;
				if (timepick== &atime) {
					btime = time;
					smp_unlock(&lk_timeout);
					timepick = &btime;
				} else {
					atime = time;
					smp_unlock(&lk_timeout);
					timepick = &atime;
				}

			} else {

				timepick->tv_usec = time.tv_usec; 
				smp_unlock(&lk_timeout);
			}

		}
		/* schedule errlog deamon... */
		if (elwakeup == 1) {
			needsoft++;
		}

	}
	if (pcpu->cpu_call_pending) needsoft++;

	/*
	 * Charge the time out based on the mode the cpu is in.
	 * Here again we fudge for the lack of proper interval timers
	 * assuming that the current state has been around at least
	 * one tick.
	 */
#ifdef vax
	if (USERMODE(ps))
#endif vax
#ifdef mips
	if (USERMODE(intr_frame[EF_SR]))
#endif mips
	{
		if (u.u_prof.pr_scale > 1) {
			u.u_oweupc |= SOWEUPC;
			aston();
		}

		/*
		 * CPU was in user state.  Increment
		 * user time counter, and process process-virtual time
		 * interval timer.
		 */
		BUMPTIME(&u.u_ru.ru_utime, tick);
		if (timerisset(&u.u_timer[ITIMER_VIRTUAL].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_VIRTUAL], tick) == 0) {
		    	pcpu->cpu_state |= CPU_SIGVTALRM;
			needsoft++;
		}
		if (u.u_procp->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
	} else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.  If no process is running
		 * then this is a system tick if we were running
		 * at a non-zero IPL (in a driver).  If a process is running,
		 * then we charge it with system time even if we were
		 * at a non-zero IPL, since the system often runs
		 * this way during processing of system calls.
		 * This is approximate, but the lack of true interval
		 * timers makes doing anything else difficult.
		 */
		cpstate = CP_SYS;
		if (pcpu->cpu_noproc) {
#ifdef vax
			if (BASEPRI(ps))
#endif vax
#ifdef mips
			if (BASEPRI(intr_frame[EF_SR]))
#endif mips
				cpstate = CP_IDLE;
		} else {
			BUMPTIME(&u.u_ru.ru_stime, tick);
		}
	}

	/*
	 * If the cpu is currently scheduled to a process, then
	 * charge it with resource utilization for a tick, updating
	 * statistics which run in (user+system) virtual time,
	 * such as the cpu time limit and profiling timers.
	 * This assumes that the current process has been running
	 * the entire last tick.
	 */
	if (pcpu->cpu_noproc == 0) {
		p = u.u_procp;
		if ((u.u_ru.ru_utime.tv_sec+u.u_ru.ru_stime.tv_sec+1) >
		    u.u_rlimit[RLIMIT_CPU].rlim_cur) {
		    	pcpu->cpu_state |= CPU_SIGXCPU;
			needsoft++;
			if (u.u_rlimit[RLIMIT_CPU].rlim_cur <
			    u.u_rlimit[RLIMIT_CPU].rlim_max)
				u.u_rlimit[RLIMIT_CPU].rlim_cur += 5;
		}

		if (timerisset(&u.u_timer[ITIMER_PROF].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_PROF], tick) == 0) {
		    	pcpu->cpu_state |= CPU_SIGPROF;
			needsoft++;
		}
		s = p->p_rssize;
		u.u_ru.ru_idrss += s; u.u_ru.ru_isrss += 0;	/* XXX */
		if (p->p_textp) {
			register int xrss = p->p_textp->x_rssize;

			s += xrss;
			u.u_ru.ru_ixrss += xrss;
		}


		/* begin SHMEM */
		if ((p->p_smcount > 0) && !(p->p_vm & SKEEP)) {
			register struct smem *sp;
			register int i;

			if(u.u_procp->p_sm == (struct p_sm *) NULL) 
				panic("hardclock: p_sm");
			for (i = 0; i < p->p_smcount; i++)
				if (sp = p->p_sm[i].sm_p){
					s += sp->sm_rssize;
					u.u_ru.ru_ismrss +=
							sp->sm_rssize;
				}
		}
		/* end SHMEM */

		if (s > u.u_ru.ru_maxrss)
			u.u_ru.ru_maxrss = s;

		/*
		 * We adjust the priority of the current process.
		 * The priority of a process gets worse as it accumulates
		 * CPU time.  The cpu usage estimator (p_cpu) is increased here
		 * and the formula for computing priorities (in kern_synch.c)
		 * will compute a different value each time the p_cpu increases
		 * by 4.  The cpu usage estimator ramps up quite quickly when
		 * the process is running (linearly), and decays away 
		 * exponentially, at a rate which is proportionally slower 
		 * when the system is busy.  The basic principal is that the 
		 * system will 90% forget that a process used a lot of CPU 
		 * time in 5*loadav seconds.  This causes the system to favor 
		 * processes which haven't run much recently, and to 
		 * round-robin among other processes.
	 	 */
		p->p_cpticks++;
		if (++p->p_cpu == 0)
			p->p_cpu--;
		if ((p->p_cpu&3) == 0) {
			(void) setpri(p);
			if (p->p_pri >= PUSER)
				p->p_pri = p->p_usrpri;
		}
	}

	if (!pcpu->cpu_noproc) {
		if ((--pcpu->cpu_roundrobin <= 0)) { 
			if (whichqs) {
				pcpu->cpu_runrun=1;
				aston();  /* simulate roundrobin */
			} else
				pcpu->cpu_roundrobin=5; 
 		}
	}

	/*
	 * If the alternate clock has not made itself known then
	 * we must gather the statistics.
	 */
	if (phz == 0) {
#ifdef mips
#ifdef XPRBUG
		extern unsigned clkticks;

		clkticks++;
#endif XPRBUG
#endif mips

#ifdef vax
		if (!(USERMODE(ps)))
#endif vax
#ifdef mips
		if (!(USERMODE(intr_frame[EF_SR])))
#endif mips
		{

#if defined(GPROF) || defined(PROFILING)
#ifdef vax
			s = pc - s_lowpc;
			if (profiling < 2 && s < s_textsize)
				kcount[s /(HISTFRACTION * sizeof (*kcount))]++;
#endif vax
#ifdef mips
			s = intr_frame[EF_EPC] - (u_int)s_lowpc;
 			if (profiling < 2 && s < s_textsize && phz)
 			{
 				char *k = (char *)kcount;
 				(*(u_int *)(k + ((s >> 3) << 2)))++;
 			}
#endif mips
#endif GPROF || PROFILING
		}
		pcpu->cpu_cptime[cpstate]++;
	}
	
	if (needsoft) {
#ifdef vax
		if (BASEPRI(ps))
#endif vax
#ifdef mips
		if (BASEPRI(intr_frame[EF_SR]))
#endif mips
		{
			/*
			 * Save the overhead of a software interrupt;
			 * it will happen as soon as we return, so do it now.
			 */
			(void) splsoftclock();
#ifdef vax
			softclock(pc, ps);
#endif vax
#ifdef mips
			softclock(intr_frame);
#endif mips
		} else
			setsoftclock();
	}
}

int	dk_ndrive = DK_NDRIVE;
/*
 * Gather statistics on resource utilization.
 *
 * We make a gross assumption: that the system has been in the
 * state it is in (user state, kernel state, interrupt state,
 * or idle state) for the entire last time interval, and
 * update statistics accordingly.
 */
/*ARGSUSED*/

#ifdef notdef
#ifdef vax
/*ARGSUSED*/
gatherstats(pc, ps)
	caddr_t pc;
	int ps;
#endif vax
#ifdef mips
gatherstats(intr_frame)
	u_int *intr_frame;
#endif mips
{
	register int cpstate, cpndx;
	unsigned int s;
#ifdef mips
#ifdef XPRBUG
	extern unsigned clkticks;

	clkticks++;
#endif XPRBUG

#endif mips

	/*
	 * Determine what state the cpu is in.
	 */
	pcpu = CURRENT_CPUDATA;
#ifdef vax
	if (USERMODE(ps))
#endif vax
#ifdef mips
	if (USERMODE(intr_frame[EF_SR]))
#endif mips
	{
		/*
		 * CPU was in user state.
		 */
		if (u.u_procp->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
	} else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.
		 */
		cpstate = CP_SYS;
#ifdef vax
		if (cpudata[cpndx].c_noproc && BASEPRI(ps))
#endif vax
#ifdef mips
		if (cpudata[cpndx].c_noproc && BASEPRI(intr_frame[EF_SR]))
#endif mips
			cpstate = CP_IDLE;
#if defined(GPROF) || defined(PROFILING)
#ifdef vax
		s = pc - s_lowpc;
		if (profiling < 2 && s < s_textsize)
			kcount[s / (HISTFRACTION * sizeof (*kcount))]++;
#endif vax
#ifdef mips
		s = intr_frame[EF_EPC] - (u_int)s_lowpc;
 		if (profiling < 2 && s < s_textsize && phz)
 			{
 			char *k = (char *)kcount;
 			(*(u_int *)(k + ((s >> 3) << 2)))++;
 			}
#endif mips
#endif GPROF || PROFILING
	}

	pcpu->cpu_cptime[cpstate]++;
}
#endif

/*
 * Software priority level clock interrupt.
 * Run periodic events from timeout queue.
 * Asynchronously invoke all current scheduled kernel subroutines.
 */
/*ARGSUSED*/
#ifdef vax
softclock(pc, ps)
	caddr_t pc;
	int ps;
#endif vax
#ifdef mips
softclock(intr_frame)
	u_int *intr_frame;
#endif mips
{
	register struct cpudata *pcpu=CURRENT_CPUDATA;
	register int s;

#ifdef mips
	acksoftclock();
#endif mips
	s = splhigh();
	/* if queue not empty and this processor not already running
	   softclock then field timeout requests */
	if ((pcpu->cpu_call_pending) &&
		!(pcpu->cpu_state & CPU_TIMEOUT)) {
		smp_lock(&lk_timeout, LK_RETRY);
		pcpu->cpu_call_pending=0;
		/* set flag say this processor is doing a timeout.  This
		   is used so we do execute this code twice on the same 
		   processor */
		pcpu->cpu_state |= CPU_TIMEOUT;	
		while (pcpu->cpu_call_fwd && pcpu->cpu_call_fwd->c_time==0) {
			register struct callout *p1;
			p1=pcpu->cpu_call_fwd;

			/* set flag saying this timeout is running.  This
			   flags tells untimeout that it cannot delete this
			   entry */
			smp_unlock(&lk_timeout);
			splx(s);
			(*p1->c_func)(p1->c_arg, p1->c_time);
			splhigh();
			smp_lock(&lk_timeout,LK_RETRY);
			
						

			/* remove the entry and discard it */
			pcpu->cpu_call_donot_requeue = 0;
			pcpu->cpu_call_fwd=p1->c_next;
			p1->c_next = callfree;
			callfree = p1;
		}
		/* we are done handling timeouts so allow this processor
		   to enter this code agin */
		pcpu->cpu_state &= ~CPU_TIMEOUT;  		
		smp_unlock(&lk_timeout);
	}
	splx(s);
	/*
	 * If trapped user-mode and profiling, give it
	 * a profiling tick.
	 */

	if (pcpu->cpu_state & CPU_SIGPROF) {
		pcpu->cpu_state &= ~ CPU_SIGPROF;
		psignal(u.u_procp,SIGPROF);
	}

	if (pcpu->cpu_state & CPU_SIGXCPU) {
		pcpu->cpu_state &= ~ CPU_SIGXCPU;
		psignal(u.u_procp,SIGXCPU);
	}

	if (pcpu->cpu_state & CPU_SIGVTALRM) {
		pcpu->cpu_state &= ~ CPU_SIGVTALRM;
		psignal(u.u_procp,SIGVTALRM);
	}
#ifdef vax
	if (USERMODE(ps))
#endif vax
#ifdef mips
	if (USERMODE(intr_frame[EF_SR]))
#endif mips
	{
		register struct proc *p = u.u_procp;
		/*
		 * Check to see if process has accumulated
		 * more than 10 minutes of user time.  If so
		 * reduce priority to give others a chance.
		 */
		if (p->p_uid && p->p_nice == NZERO &&
		    u.u_ru.ru_utime.tv_sec > 10 * 60) {
			int s = spl6();
			smp_lock(&lk_rq, LK_RETRY);
			p->p_nice = NZERO+4;
			(void) setpri(p);
			p->p_pri = p->p_usrpri;
			smp_unlock(&lk_rq);
			splx(s);
		}
	}
	/* kick errorlogger if needed */
	if (BOOT_CPU && elwakeup==1) eldaemon(); 

}

/*
 * Bump a timeval by a small number of usec's.
 */
bumptime(tp, usec)
	register struct timeval *tp;
	int usec;
{

	tp->tv_usec += usec;
	if (tp->tv_usec >= 1000000) {
		tp->tv_usec -= 1000000;
		tp->tv_sec++;
	}
}


void ksched(f)
    register struct kschedblk *f;
{
	register struct cpudata *pcpu=CURRENT_CPUDATA;
	timeout(f->func, f->arg, 0);
	pcpu->cpu_call_pending = 1;
	setsoftclock();
}
void unksched(f)
    register struct kschedblk *f;
{
	untimeout(f->func, f->arg);
}

/*
 * Arrange that (*fun)(arg) is called in t/hz seconds.
 */
timeout(fun, arg, t)
	int (*fun)();
	caddr_t arg;
	register int t;
{
	register struct callout *p1, *p2, *pnew;
	register struct cpudata *pcpu;
	register int s;
	int t2;

	s = splhigh();
	if (t<0) t=1;

	pcpu = CURRENT_CPUDATA;

	smp_lock(&lk_timeout, LK_RETRY);

	/* if this processor is in timeout then we could be trying to 
	   requeue the same timeout routine.  This is a special case in the
	   fact that another processor could have done an untimeout.  After
	   the untimeout we should not requeue the timeout routine.  This
	   was the case on a single processore. */

	if (pcpu->cpu_state & CPU_TIMEOUT) {
		/* if this cpu is doing a timeout that another processor
		   did an untimeout for then do not requeue it */
		p1 = pcpu->cpu_call_fwd;
		if ((pcpu->cpu_call_donot_requeue & CALLOUT_DONOT_REQUEUE) &&
		    (p1->c_func == fun) &&
		    (p1->c_arg	== arg)) {

			smp_unlock(&lk_timeout);
			splx(s);
			return;
		}
	}	  
	pnew = callfree;
	if (pnew == NULL)
		panic("timeout table overflow");
	callfree = pnew->c_next;
	pnew->c_arg = arg;
	pnew->c_func = fun;
	for (p1 = (struct callout *)&pcpu->cpu_call_fwd; 
	    (p2 = p1->c_next) && p2->c_time <= t; 
	     p1 = p2) {
		if (p2->c_time > 0)
		    t -= p2->c_time;
	}
	p1->c_next = pnew;
	pnew->c_next = p2;
	pnew->c_time = t;
	
	if (p2)
		p2->c_time -= t;

	smp_unlock(&lk_timeout);
	splx(s);
}

/*
 * untimeout is called to remove a function timeout call
 * from the callout structure.
 */
untimeout(fun, arg)
	int (*fun)();
	caddr_t arg;
{
	register struct callout *p1, *p2;
	register int s;
	register struct cpudata *pcpu;
	register int cpunum;

	s = splhigh();
	smp_lock(&lk_timeout, LK_RETRY);

	/* look at each cpu queue to see if it is there.  If the timeout
	   is in progress, then set a flag in the callout struct to signal
	   not to "requeue" this timeout */
	for(cpunum = lowcpu; cpunum <=highcpu; cpunum++) {
		pcpu =CPUDATA(cpunum);
		if (pcpu) {
			p1 = (struct callout *)&pcpu->cpu_call_fwd;
			while(p1->c_next) {
				p2=p1->c_next;
				if(p2->c_func == fun &&
				   p2->c_arg == arg) {
				   	/* if timeout is in progress then
					   set the donot requeue flag else
					   remove it and free the structure*/
				   	if ((pcpu->cpu_call_fwd == p2) &&
					     (pcpu->cpu_state & CPU_TIMEOUT)) {
/* The timeout is current running on "cpunum" at this point.  There are
   two cases.  If the cpu doing the untimeout is not the cpu that is 
   running the timeout, then set the CALLOUT_DONOT_REQUEUE flag.  This is
   to prevent the timeout routine from rescheduling itself.  This case can
   only happen on MP boxes.

   The second case is that the cpu doing the untimeout is the same one that
   is running the timeout routine.  In this case we do nothing so the the
   timeout routine can reschedule itself if it wants too.  Code example
   is:
   	timeout_routine() {

		untimeout(timeout_routine, ...);
		timeout(timeout_routine, ...);
	}

   This case is unlikely, it worked prior to SMP and was done in one
   driver that I know of.
   
   JAW

*/						
						if (CURRENT_CPUDATA != pcpu)
							pcpu->cpu_call_donot_requeue |= CALLOUT_DONOT_REQUEUE;
					} else {

						if (p2->c_next && p2->c_time > 0)
                                			p2->c_next->c_time += p2->c_time;

						p1->c_next = p2->c_next;

						p2->c_next=callfree;
						callfree=p2;
						p2=p1;
						smp_unlock(&lk_timeout);
						splx(s);
						return;
					}
				}
				p1=p2;
			}
		}
	}
	smp_unlock(&lk_timeout);
	splx(s);
}

/* this routine is used when shutting down a processor to
   change affinity to any timeout that can only execute on the
   current processor.
 */
timeout_affinity_fix()
{
	register struct callout *p1, *pnew, *p2;
	register int cpu_num;
	register struct cpudata *pcpu;
	int t;
	int s;

	pcpu = CURRENT_CPUDATA;

	cpu_num = pcpu->cpu_num;

	s = splhigh();
	smp_lock(&lk_timeout, LK_RETRY);

	/* requeue all of the entries on the cpu callout list to the 
	  boot cpu */
	while(pcpu->cpu_call_fwd) {
		pnew = pcpu->cpu_call_fwd;
		pcpu->cpu_call_fwd=pnew->c_next;

		/* add time from entry we deleted to next element */
		if (pcpu->cpu_call_fwd)
			pcpu->cpu_call_fwd->c_time += pnew->c_time;

		t = pnew->c_time;

		for (p1 = (struct callout *)&boot_cpudata.cpu_call_fwd; (p2 = p1->c_next) && p2->c_time <= t; p1 = p2)
			if (p2->c_time > 0)
				t -= p2->c_time;
		p1->c_next = pnew;
		pnew->c_next = p2;
		pnew->c_time = t;
	
		if (p2)
			p2->c_time -= t;

	}

	smp_unlock(&lk_timeout);
	splx(s);
}
hzto(tv) 
	struct timeval *tv;
{

	struct timeval mytime;

	register int s = splclock();	/* read the time deterministically */
	mytime.tv_sec  = time.tv_sec;
	mytime.tv_usec = time.tv_usec;
	splx(s);

	return(mp_hzto(tv,&mytime));
}

mp_hzto(tv,mytime)
	struct timeval *tv;
	struct timeval *mytime;
{
	register long ticks = 0x7fffffff;
	register long sec;

	/*
	 * If ticks will fit in 32 bit arithmetic,
	 * then compute ticks.  Otherwise just return huge number of ticks...
	 *
	 * Maximum value for any timeout is 0x7fffffff ticks
	 * which is 97 days with hz=256 and 248 days with hz=100
	 *
	 */
	if ((tv->tv_sec - mytime->tv_sec) < 0x7fffffff / hz)
		ticks = times_to_ticks(tv,mytime);
	/*
	 * Account for clock rounding error when there are 256 ticks per
	 * second.  "fixunits" is the number of "fixtick" units (64 uSEC units)
	 * we will be off by.  We then calculate the number of uSECs
	 * (fixtick * fixunits) that we will be off, divide total uSECs
	 * by "tick" (the number of uSECs in a tick) to get the
	 * number of ticks to add in.
	 */
	if (fixtick) {
		int fixunits;
		fixunits = ticks / hz;
		ticks = ticks + ((fixtick * fixunits) / tick);
		if (ticks < 0)
			ticks = 0x7fffffff;
	}
	return (ticks);
}

/* return the number of clock ticks between two time values */
/* t1 > t2 (as the time flies...) */
/* this is tricky as the tick value is strange on non-vaxen... */
/* return a positive number of ticks or zero */

times_to_ticks(t1,t2)
register struct timeval *t1,*t2;
{
	register int ret =  (t1->tv_sec  - t2->tv_sec)  * hz  +
			(t1->tv_usec - t2->tv_usec) / tick;
	if (ret > 0) return(ret);
	return(0);
}

profil()
{
	register struct a {
		short	*bufbase;
		unsigned bufsize;
		unsigned pcoffset;
		unsigned pcscale;
	} *uap = (struct a *)u.u_ap;
	register struct uprof *upp = &u.u_prof;

	upp->pr_base = uap->bufbase;
	upp->pr_size = uap->bufsize;
	upp->pr_off = uap->pcoffset;
	upp->pr_scale = uap->pcscale;
}

opause()
{

	for (;;)
		sleep((caddr_t)&u, PSLEP);
}


#ifdef vax
/*
 * Arrange that (*fun)(args) is called on @ ipl 7.
 *   (called on as end of interrupt routine)
 */
chrqueue(fun, arg1, arg2)
	register int (*fun)();
	register int arg1;
	register int arg2;
{
	register struct chrout *pnew;
	int s = splhigh();

	if (!BOOT_CPU) {
		panic("not bootcpu\n");
	}

	pnew = chrfree;
	chrfree = pnew->c_next;
	if(chrfree == chrcur)
		panic("Character queue overflow");
	pnew->c_arg = arg1;
	pnew->d_arg = arg2;
	pnew->c_func = fun;
	setintqueue();	/* Schedule interrupt queue at IPL 7 */
	splx(s);
}

intqueue()
{

	if (!BOOT_CPU) {
		panic("not bootcpu\n");
	}
		if(chrcur != chrfree)
		for(;;) {
			register struct chrout *c1;
			register int s;
			s = splhigh();
			c1 = chrcur;
			if (c1 == chrfree) {
				splx(s);
				break;
			}
			chrcur = c1->c_next;
			splx(s);
			(*c1->c_func)(c1->c_arg, c1->d_arg);
		}
}
#endif

#ifdef mips
uniqtime(tv)
	register struct timeval *tv;
{
	static struct timeval last;
	static int uniq;
	register volatile struct timeval *tptr = &time;

	while (last.tv_usec != tptr->tv_usec || last.tv_sec != tptr->tv_sec) {
		last = *tptr;
		uniq = 0;
	}
	*tv = last;
	tv->tv_usec += uniq++;

}
#endif mips
