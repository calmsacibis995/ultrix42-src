#ifndef lint
static char *sccsid = "@(#)vm_sched.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983,86,88 by			*
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
 *	Modification History
 * 06 Mar 90 -- sekhar(for Joe Amato)
 *	Fixed an smp race condition when swapping a process in.
 *
 * 17 Oct 89 -- jaa
 *	fix deficit calc
 *
 *  24 Jul 89 -- jmartin
 *	Add the function swap_volunteers, which is triggered from
 *	ptexpand.
 *
 *  13-Jul-89   gg
 *	Prevent selecting SNOVM set process from getting swapped out
 *	(vfork'd parent with no resources should not be swapped out )
 *
 *  12-Jun-89	gg
 *	Fix Missing parenthesis in swappable macro.
 *
 *  26-Jan-89	jaw
 *	SMP clean up 
 *
 * 14 Nov 88 -- jmartin
 *	Fix for a divide-by-zero panic:  if the text associated with a
 *	process has a zero value for x_ccount (number of loaded
 *	processes using text), do not consider the process for swapping.
 *
 * 31 Aug 88 -- jmartin
 *	Lock text chains while collecting statistics.
 *
 * 25 Jul 88 -- jmartin
 *	Test swappable() while holding lk_rq so that state doesn't change.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *	
 * 07 Jun 88 - miche
 *	SMP procqs:  re-arranged swapper code to fit more easily with
 *	the FORALLPROC work.
 *
 * 15 Dec 86 -- depp
 *	Added fix to shared memory handling, in regards to a process' SM 
 *	page table swapping space.  
 *
 * 29 Oct 86 -- jaw  fix MP swapper bug.  doing remrq without checking if
 *		     process is on the slave.
 *	
 * 11 Sep 86 -- koehler
 *	gnode name change
 *
 * 10-Jul-86 -- tresvik
 *	moved lotsfree, desfree and minfree to param.c so that
 *	the defaults could be overriden
 *
 * 02 Apr 86 -- depp
 *	Changed method of calculating "lotsfree" so that paging will not
 *	be turned on when there is plenty of memory.  This was a problem
 *	in large memory configurations.
 *
 * 16 Jul 85 -- jrs
 *	Added run queue locking
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V.
 *
 * 30 Sept 85 -- depp
 *	Added checks for memory locking 
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/seg.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/kernel.h"
#include "../h/cpudata.h"

#ifdef mips
#include "../h/fixpoint.h"
#endif mips

int	maxslp = MAXSLP;
int	saferss = SAFERSS;

/*
 * The following parameters control operation of the page replacement
 * algorithm.  They are initialized to 0, and then computed at boot time
 * based on the size of the system.  If they are patched non-zero in
 * a loaded vmunix they are left alone and may thus be changed per system
 * using adb on the loaded system.
 */
int	maxpgio = 0;
int	slowscan = 0;
int	fastscan = 0;
int	klin = KLIN;
int	klseql = KLSEQL;
int	klsdist = KLSDIST;
int	kltxt = KLTXT;
int	klout = KLOUT;
int	multprog = -1;		/* so we don't count process 2 */

#ifdef vax
extern int minfree;
extern int desfree;
extern int lotsfree;
#endif vax
#ifdef mips
int     minfree = 0;
int     desfree = 0;
int	lotsfree= 0;
#endif mips

#ifdef vax
double	avenrun[3];		/* load average, of runnable procs */
#endif vax
#ifdef mips
fix     avenrun[3];             /* load average, of runnable procs */
#endif

/*
 * Setup the paging constants for the clock algorithm.
 * Called after the system is initialized and the amount of memory
 * and number of paging devices is known.
 *
 * Threshold constants are defined in ../machine/vmparam.h.
 */
setupclock()
{

	/*
	 * Setup thresholds for paging:
	 *	lotsfree	is threshold where paging daemon turns on
	 *	desfree		is amount of memory desired free.  if less
	 *			than this for extended period, do swapping
	 *	minfree		is minimal amount of free memory which is
	 *			tolerable.
	 */
	if (lotsfree == 0)
		lotsfree = LOTSFREE / NBPG;
	if (desfree == 0) {
		desfree = DESFREE / NBPG;
		if (desfree > LOOPPAGES / DESFREEFRACT)
			desfree = LOOPPAGES / DESFREEFRACT;
	}
	if (minfree == 0) {
		minfree = MINFREE / NBPG;
		if (minfree > desfree / MINFREEFRACT)
			minfree = desfree / MINFREEFRACT;
	}
	/*
	 * Maxpgio thresholds how much paging is acceptable.
	 * This figures that 2/3 busy on an arm is all that is
	 * tolerable for paging.  We assume one operation per disk rev.
	 */
	if (maxpgio == 0)
		maxpgio = (DISKRPM * 2) / 3;

	/*
	 * Clock to scan using max of ~~10% of processor time for sampling,
	 *     this estimated to allow maximum of 200 samples per second.
	 * This yields a ``fastscan'' of roughly (with CLSIZE=2):
	 *	<=1m	2m	3m	4m	8m
	 * 	5s	10s	15s	20s	40s
	 */
/*
	if (nswdev == 1 && physmem*NBPG > LOTSOFMEM*1024*(1024-16))
		printf("WARNING: should run interleaved swap with >= %dMb\n",
		    LOTSOFMEM);
*/
	if (fastscan == 0)
		fastscan = (LOOPPAGES/CLSIZE) / 200;
	if (fastscan < 5)
		fastscan = 5;
	if (nswdev >= 2)
		maxpgio = (maxpgio * 3) / 2;

	/*
	 * Set slow scan time to 1/2 the fast scan time.
	 */
	if (slowscan == 0)
		slowscan = 2 * fastscan;
}

/*
 * The main loop of the scheduling (swapping) process.
 *
 * The basic idea is:
 *	see if anyone wants to be swapped in;
 *	swap out processes until there is room;
 *	swap him in;
 *	repeat.
 * If the paging rate is too high, or the average free memory
 * is very low, then we do not consider swapping anyone in,
 * but rather look for someone to swap out.
 *
 * The runout flag is set whenever someone is swapped out.
 * Sched sleeps on it awaiting work.
 *
 * Sched sleeps on runin whenever it cannot find enough
 * core (by swapping out or otherwise) to fit the
 * selected swapped process.  It is awakened when the
 * core situation changes and in any case once per second.
 *
 * sched DOESN'T ACCOUNT FOR PAGE TABLE SIZE IN CALCULATIONS.
 */

#define	swappable(p) \
	((((p)->p_vm&(SLOCK|SULOCK|SPAGE|SKEEP|SPHYSIO|SNOVM))==0) && \
	 (((p)->p_type&(SSYS|SWEXIT))==0) && ((p)->p_sched&SLOAD))

#define swapreqst(p) (((p)->p_stat == SSLEEP) && ((p)->p_vm & SSWAP))

/* insure non-zero */
#define	nz(x)	(x != 0 ? x : 1)

#define	NBIG	4
#define	MAXNBIG	10
int	nbig = NBIG;

struct bigp {
	struct	proc *bp_proc;
	int	bp_pri;
	struct	bigp *bp_link;
} bigp[MAXNBIG], bplist;

extern struct proc *hs();

sched()
{
	register struct proc *rp, *p;
	int cpuindx,outpri, inpri, rppri;
	int sleeper, desperate, deservin, needs, divisor;
	register struct bigp *bp;
	int gives;

loop:
	wantin = 0;
	deservin = 0;
	sleeper = 0;
	p = 0;

	if (swapself)
		swap_volunteers();
	/*
	 * See if paging system is overloaded; if so swap someone out.
	 * Conditions for hard outswap are:
	 *	if need kernel map (mix it up).
	 * or
	 *	1. if there are at least 2 runnable processes (on the average)
	 * and	2. the paging rate is excessive or memory is now VERY low.
	 * and	3. the short (5-second) and longer (30-second) average
	 *	   memory is less than desirable.
	 */
	if (kmapwnt ||
#ifdef vax
	    (avenrun[0] >= 2 && imax(avefree, avefree30) < desfree &&
#endif vax
#ifdef mips
            (avenrun[0] >= TO_FIX(2) && imax(avefree, avefree30) < desfree &&
#endif mips
	    (rate.v_pgin + rate.v_pgout > maxpgio || avefree < minfree))) {
		desperate = 1;
		goto hardswap;
	}
	desperate = 0;
	/*
	 * Not desperate for core,
	 * look for someone who deserves to be brought in.
	 */
	outpri = -20000;
	FORALLPROC(
		   splhigh();
		   smp_lock(&lk_rq, LK_RETRY);

		   switch(pp->p_stat) {
	/* pp is defined in FORALLPROC */
	case SRUN:
		if ((pp->p_sched&SLOAD) == 0) {
			rppri = pp->p_time -
			    pp->p_swrss / nz((maxpgio/2) * (klin * CLSIZE)) +
			    pp->p_slptime - (pp->p_nice-NZERO)*8;
			if (rppri > outpri) {
				if (pp->p_poip) {
					smp_unlock(&lk_rq);
					spl0();
					NEXTPROC;
				}
				smp_unlock(&lk_rq);
				smp_lock(&lk_text, LK_RETRY);
				if (pp->p_textp && pp->p_textp->x_poip) {
					smp_unlock(&lk_text);
					spl0();
					NEXTPROC;
				}
				smp_unlock(&lk_text);
				p = pp;
				outpri = rppri;
				spl0();
				NEXTPROC;
			}
		}
	        smp_unlock(&lk_rq);
	        spl0();
		NEXTPROC;

	case SSLEEP:
	case SSTOP:
		if ((freemem < desfree || pp->p_rssize == 0) &&
		    pp->p_slptime > maxslp &&
		   (!pp->p_textp || (pp->p_textp->x_flag&(XLOCK|XNOSW))==0) &&
		    swappable(pp)) {
			/*
			 * Kick out deadwood.
			 */

			pp->p_sched &= ~SLOAD;

			smp_unlock(&lk_rq);
			(void) spl0();
			(void) swapout(pp);
#ifdef vax
			goto loop;
#endif vax
#ifdef mips
			NEXTPROC;
#endif mips
		} 
	        smp_unlock(&lk_rq);
	        spl0();
		NEXTPROC;
      default:
	        smp_unlock(&lk_rq);
	        spl0();
	        NEXTPROC;
	} ) /* end FORALLPROC */

	if (p)
		XPRINTF(XPR_VM,"found one p = 0x%x, pid = %d\n",p,p->p_pid,0,0);

	/*
	 * No one wants in, so nothing to do.
	 */
	if (outpri == -20000) {
		(void) splhigh();
		if (wantin) {
			wantin = 0;
			sleep((caddr_t)&lbolt, PSWP);
		} else {
			runout++;
			sleep((caddr_t)&runout, PSWP);
		}
		(void) spl0();
		goto loop;
	}
	/*
	 * Decide how deserving this guy is.  If he is deserving
	 * we will be willing to work harder to bring him in.
	 * Needs is an estimate of how much core he will need.
	 * If he has been out for a while, then we will
	 * bring him in with 1/2 the core he will need, otherwise
	 * we are conservative.
	 */
	deservin = 0;
	divisor = 1;
	if (outpri > maxslp/2) {
		deservin = 1;
		divisor = 2;
	}
	needs = p->p_swrss;
	if (p->p_textp && p->p_textp->x_ccount == 0)
		needs += p->p_textp->x_swrss;
	needs = imin(needs, lotsfree);
	if (freemem - deficit > needs / divisor) {
		deficit += needs;

		(void) splhigh();
		smp_lock(&lk_rq,LK_RETRY);
		if ((p->p_stat & SRUN) != SRUN || (p->p_sched & SLOAD) != 0) {
			smp_unlock(&lk_rq);
			(void) spl0();
			goto loop;
		}
		smp_unlock(&lk_rq);
		(void) spl0();
		if (swapin(p))
			goto loop;
		deficit -= imin(needs, deficit);
	}

hardswap:
	sleeper = (p = hs(p)) ? p->p_slptime : 0;
	if (!sleeper) {
		p = NULL;
		inpri = -1000;
		for (bp = bplist.bp_link; bp; bp = bp->bp_link) {
			rp = bp->bp_proc;
			rppri = rp->p_time+rp->p_nice-NZERO;
			if (rppri >= inpri) {
				p = rp;
				inpri = rppri;
			}
		}
	}

	/*
	 * If we found a long-time sleeper, or we are desperate and
	 * found anyone to swap out, or if someone deserves to come
	 * in and we didn't find a sleeper, but found someone who
	 * has been in core for a reasonable length of time, then
	 * we kick the poor luser out.
	 */
	if (sleeper || desperate && p || deservin && inpri > maxslp) {

		(void) splhigh();
		smp_lock(&lk_rq, LK_RETRY);
		if(p->p_stat != SRUN && p->p_stat != SSLEEP &&
		   p->p_stat != SSTOP && ((p->p_sched & SLOAD) == 0)) {
			smp_unlock(&lk_rq);
			(void) spl0();
			goto loop;
		}
		if (p->p_stat == SRUN) {
			for (cpuindx = lowcpu; cpuindx <= highcpu; cpuindx++) {
				if ((CPUDATA(cpuindx)) && 
				    p == CPUDATA(cpuindx)->cpu_proc &&
				    CPUDATA(cpuindx)->cpu_noproc == 0) {
					smp_unlock(&lk_rq);
					(void) spl0();
					goto doneit;
				}
			}
			if (!swappable(p)) {
				smp_unlock(&lk_rq);
				(void) spl0();
				goto doneit;
			} else	
				remrq(p);
		}
 		p->p_sched &= ~SLOAD;
		smp_unlock(&lk_rq);
		(void) spl0();
		if (desperate) {
			/*
			 * Want to give this space to the rest of
			 * the processes in core so give them a chance
			 * by increasing the deficit.
			 */
			gives = p->p_rssize;
			if (p->p_textp)
				gives += p->p_textp->x_rssize / p->p_textp->x_ccount;
			gives = imin(gives, lotsfree);
			deficit += gives;
		} else
			gives = 0;	/* someone else taketh away */
		if (swapout(p) == 0)
			deficit -= imin(gives, deficit);
		goto loop;
	}
	/*
	 * Want to swap someone in, but can't
	 * so wait on runin.
	 */
doneit:

	(void) splhigh();
	runin++;
	sleep((caddr_t)&runin, PSWP);
	(void) spl0();
	goto loop;
}

struct proc *
hs(p)
	struct proc *p;
{
	register struct proc *inp;
	int biggot, sleeper, rppri;
	register struct bigp *nbp, *mbp;

	/*
	 * Need resources (kernel map or memory), swap someone out.
	 * Select the nbig largest jobs, then the oldest of these
	 * is ``most likely to get booted.''
	 */
	inp = p;
	sleeper = 0;
	if (nbig > MAXNBIG)
		nbig = MAXNBIG;
	if (nbig < 1)
		nbig = 1;
	biggot = 0;
	bplist.bp_link = 0;
	FORALLPROC (	/* pp is defined in FORALLPROC */
		struct text *xp;

		if (!swappable(pp))
			NEXTPROC;
		if (pp == inp)
			NEXTPROC;
		xp = pp->p_textp;
		if (xp && ((xp->x_flag&(XLOCK|XNOSW)) || (xp->x_ccount == 0)))
			NEXTPROC;
		if (pp->p_slptime > maxslp &&
		    (pp->p_stat==SSLEEP&&pp->p_pri>PZERO||pp->p_stat==SSTOP)) {
			if (sleeper < pp->p_slptime) {
				p = pp;
				sleeper = pp->p_slptime;
			}
		} else if (!sleeper && (pp->p_stat==SRUN||pp->p_stat==SSLEEP)) {
			rppri = pp->p_rssize;
			if (xp)
				rppri += xp->x_rssize/xp->x_ccount;
			if (biggot < nbig)
				nbp = &bigp[biggot++];
			else {
				nbp = bplist.bp_link;
				if (nbp->bp_pri > rppri)
					NEXTPROC;
				bplist.bp_link = nbp->bp_link;
			}
			for (mbp = &bplist; mbp->bp_link; mbp = mbp->bp_link)
				if (rppri < mbp->bp_link->bp_pri)
					break;
			nbp->bp_link = mbp->bp_link;
			mbp->bp_link = nbp;
			nbp->bp_pri = rppri;
			nbp->bp_proc = pp;
		}
	) /* end FORALLPROC */
	if (sleeper)
		return(p);
	else	return((struct proc *)0);
}

swap_volunteers()
{
	swapself = 0;
	FORALLPROC (
		if (!swapreqst(pp))
		    	NEXTPROC;
#ifndef vax
		(void) splhigh();
#else vax
		(void) spl6();
#endif vax
		smp_lock(&lk_rq, LK_RETRY);
		pp->p_sched &= ~SLOAD;
		smp_unlock(&lk_rq);
		(void) spl0();
		(void)swapout(pp);
	    ) ; /* END FORALLPROC */
	wakeup((caddr_t)&swapself);
}

vmmeter()
{
	register unsigned *cp, *rp, *sp;
	register int n;

	deficit -= imin(deficit, imax(deficit / 10, 
			klin * CLSIZE * maxpgio / 4));
	ave(avefree, freemem, 5);
	ave(avefree30, freemem, 30);
	/* v_pgin is maintained by clock.c */
	cp = &cnt.v_first; rp = &rate.v_first; sp = &sum.v_first;
	while (cp <= &cnt.v_last) {
		ave(*rp, *cp, 5);
		*sp += *cp;
		*cp = 0;
		rp++, cp++, sp++;
	}
	if (time.tv_sec % 5 == 0) {
		vmtotal();
		rate.v_swpin = cnt.v_swpin;
		sum.v_swpin += cnt.v_swpin;
		cnt.v_swpin = 0;
		rate.v_swpout = cnt.v_swpout;
		sum.v_swpout += cnt.v_swpout;
		cnt.v_swpout = 0;
	}
	if (avefree < minfree && runout || proc[0].p_slptime > maxslp/2) {
		runout = 0;
		runin = 0;
		wakeup((caddr_t)&runin);
		wakeup((caddr_t)&runout);
	}
}

#ifdef vax
#define	RATETOSCHEDPAGING	4		/* hz that is */
#endif vax

/*
 * Schedule rate for paging.
 * Rate is linear interpolation between
 * slowscan with lotsfree and fastscan when out of memory.
 */
schedpaging()
{
	register int vavail, scanrate;

	nscan = desscan = 0;
	vavail = freemem - deficit;
	if (vavail < 0)
		vavail = 0;
	if (freemem < lotsfree) {
		scanrate =
			(slowscan * vavail + fastscan * (lotsfree - vavail)) /
				nz(lotsfree);
		desscan = ((LOOPPAGES / CLSIZE) / nz(scanrate)) /
				RATETOSCHEDPAGING;
		wakeup((caddr_t)&proc[2]);
	}
	timeout(schedpaging, (caddr_t)0, hz / RATETOSCHEDPAGING);
}

int loadav_debug = 0;
vmtotal()
{
	register struct proc *p;
	register struct text *xp;
	int nrun = 0;

	total.t_vmtxt = 0;
	total.t_avmtxt = 0;
	total.t_rmtxt = 0;
	total.t_armtxt = 0;
	for (xp = text; xp < textNTEXT; xp++) {
		int s;
		s = splimp();
		smp_lock(&lk_text, LK_RETRY);
		if (xp->x_gptr) {
			total.t_vmtxt += xp->x_size;
			total.t_rmtxt += xp->x_rssize;
			for (p = xp->x_caddr; p; p = p->p_xlink)
			switch (p->p_stat) {

			case SSTOP:
			case SSLEEP:
				if (p->p_slptime >= maxslp)
					continue;
				/* fall into... */

			case SRUN:
			case SIDL:
				total.t_avmtxt += xp->x_size;
				total.t_armtxt += xp->x_rssize;
				goto next;
			}
next:
			;
		}
		smp_unlock(&lk_text);
		(void)splx(s);
	}
	total.t_vm = 0;
	total.t_avm = 0;
	total.t_rm = 0;
	total.t_arm = 0;
	total.t_rq = 0;
	total.t_dw = 0;
	total.t_pw = 0;
	total.t_sl = 0;
	total.t_sw = 0;
	FORALLPROC(
		if (pp->p_type & SSYS)
			NEXTPROC;
		if (pp->p_stat) {
			total.t_vm += pp->p_dsize + pp->p_ssize;
			total.t_rm += pp->p_rssize;
			switch (pp->p_stat) {

			case SSLEEP:
			case SSTOP:
				if (pp->p_pri <= PZERO) {
					nrun++;
#ifdef SMP_DEBUG
if (loadav_debug) {
	cprintf("nrun %d pp 0x%x slot %d pri %d pid %d state %d\n",
	nrun, pp, (pp-proc)%148, pp->p_pri, pp->p_pid, pp->p_stat);
}
#endif SMP_DEBUG
				}
				if (pp->p_vm & SPAGE)
					total.t_pw++;
				else if (pp->p_sched & SLOAD) {
					if (pp->p_pri <= PZERO)
						total.t_dw++;
					else if (pp->p_slptime < maxslp)
						total.t_sl++;
				} else if (pp->p_slptime < maxslp)
					total.t_sw++;
				if (pp->p_slptime < maxslp)
					goto active;
				break;

			case SRUN:
			case SIDL:
				nrun++;
#ifdef SMP_DEBUG
if (loadav_debug) {
	cprintf("nrun %d pp 0x%x pid %d state %d\n",
	nrun, pp, pp->p_pid, pp->p_stat);
}
#endif SMP_DEBUG
				if (pp->p_sched & SLOAD)
					total.t_rq++;
				else
					total.t_sw++;
active:
				total.t_avm += pp->p_dsize + pp->p_ssize;
				total.t_arm += pp->p_rssize;
				break;
			}
		}
	) /* end FORALLPROC */
#ifdef SMP_DEBUG
loadav_debug = 0;
#endif SMP_DEBUG
	total.t_vm += total.t_vmtxt;
	total.t_avm += total.t_avmtxt;
	total.t_rm += total.t_rmtxt;
	total.t_arm += total.t_armtxt;
	total.t_free = avefree;
	loadav(avenrun, nrun);
}

/*
 * Constants for averages over 1, 5, and 15 minutes
 * when sampling at 5 second intervals.
 */
#ifdef vax
double	cexp[3] = {
	0.9200444146293232,	/* exp(-1/12) */
	0.9834714538216174,	/* exp(-1/60) */
	0.9944598480048967,	/* exp(-1/180) */
};
#endif vax
#ifdef mips
#if (FBITS != 8)
# include "Error: need to redefine cexp[] decay constants."
#endif FBITS
fix     cexp[3] = {
        236,            /* (1<<8)*exp(-1/12) */
        252,            /* (1<<8)*exp(-1/60) */
        255,            /* (1<<8)*exp(-1/180) */
};
fix     one_cexp[3] = {
        20,             /* (1<<8)*(1-exp(-1/12)) */
        4,              /* (1<<8)*(1-exp(-1/60)) */
        1,              /* (1<<8)*(1-exp(-1/180)) */
};
#endif mips

/*
 * Compute a tenex style load average of a quantity on
 * 1, 5 and 15 minute intervals.
 */
loadav(avg, n)
#ifdef vax
	register double *avg;
#endif vax
#ifdef mips
        register fix *avg;
#endif mips
	int n;
{
	register int i;

	for (i = 0; i < 3; i++)
#ifdef vax
		avg[i] = cexp[i] * avg[i] + n * (1.0 - cexp[i]);
#endif vax
#ifdef mips
                avg[i] = MUL_2FIX(cexp[i], avg[i]) + n * one_cexp[i];
#endif mips
}
