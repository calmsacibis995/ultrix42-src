
/*
 * vm_sched.c
 */

#ifndef lint
static char *sccsid = "@(#)vm_sched.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983,86 by			*
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
 *	
 * 14 Sep 88 -jaa
 *	Took out swapping from sched() per RR.
 *	Problem lies in pushing the UAREA and pte's
 *	Once solved this MUST go back in (or at least revisited)
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
#include "../h/interlock.h"
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
#ifdef mips
	XPRINTF(XPR_VM,"enter setupclock",0,0,0,0);
#endif mips

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
	(((p)->p_flag&(SSYS|SLOCK|SULOCK|SLOAD|SPAGE|SKEEP|SWEXIT|SPHYSIO))==SLOAD)

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

sched()
{
	register struct proc *rp, *p, *inp;
	int cpuindex,outpri, inpri, rppri;
	int sleeper, desperate, deservin, needs, divisor;
	register struct bigp *bp, *nbp;
	int biggot, gives;

#ifdef mips
	XPRINTF(XPR_VM,"enter sched",0,0,0,0);
#endif mips
loop:
	wantin = 0;
	deservin = 0;
	sleeper = 0;
	p = 0;
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
#ifdef mips
	XPRINTF(XPR_VM,"sched searching for proc",0,0,0,0);
#endif mips
	for (rp = allproc; rp != NULL; rp = rp->p_nxt)
	{
#ifdef mips
	XPRINTF(XPR_VM,"pid = 0x% %d stat = 0x%x\n",rp,rp->p_pid,rp->p_stat,0);
#endif mips
	switch(rp->p_stat) {
	case SRUN:
		if ((rp->p_flag&SLOAD) == 0) {
			rppri = rp->p_time -
			    rp->p_swrss / nz((maxpgio/2) * (klin * CLSIZE)) +
			    rp->p_slptime - (rp->p_nice-NZERO)*8;
			if (rppri > outpri) {
				if (rp->p_poip)
					continue;
				if (rp->p_textp && rp->p_textp->x_poip)
					continue;
				p = rp;
				outpri = rppri;
			}
		}
		continue;

	case SSLEEP:
	case SSTOP:
		if ((freemem < desfree || rp->p_rssize == 0) &&
		    rp->p_slptime > maxslp &&
		   (!rp->p_textp || (rp->p_textp->x_flag&(XLOCK|XNOSW))==0) &&
		    swappable(rp)) {
			/*
			 * Kick out deadwood.
			 */
#ifndef ultrix
#ifdef mips
			(void) splhigh();
#endif mips
#ifdef vax
			(void) spl6();
#endif vax

			lock(LOCK_RQ);
			if (rp->p_stat == SRUN) {

				for (cpuindex = 0; cpuindex < activecpu; cpuindex++) {
					if (rp == cpudata[cpuindex].c_proc &&
				   	   cpudata[cpuindex].c_noproc == 0) {
						break;
					}
				}
				if (cpuindex < activecpu) { 
					unlock(LOCK_RQ);
					(void) spl0();
					continue;
				} else
					remrq(rp);
			}

#endif !ultrix
			rp->p_flag &= ~SLOAD;
#ifndef ultrix
			unlock(LOCK_RQ);
			(void) spl0();
#endif !ultrix
			(void) swapout(rp, rp->p_dsize, rp->p_ssize, 
					   rp->p_smsize);	/* SHMEM */
#ifndef ultrix
			goto loop;
#endif !ultrix

		} 
		continue;
	}
	}
#ifdef mips
	if(p)
		XPRINTF(XPR_VM,"found one p = 0x%x, pid = %d\n",p,p->p_pid,0,0);
#endif mips

	/*
	 * No one wants in, so nothing to do.
	 */
	if (outpri == -20000) {
#ifdef mips
		(void) splhigh();
#endif mips
#ifdef vax
		(void) spl6();
#endif vax
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
		if (swapin(p))
			goto loop;
		deficit -= imin(needs, deficit);
	}

hardswap:
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
	for (rp = allproc; rp != NULL; rp = rp->p_nxt) {
		if (!swappable(rp))
			continue;
		if (rp == inp)
			continue;
		if (rp->p_textp && rp->p_textp->x_flag&(XLOCK|XNOSW))
			continue;
		if (rp->p_slptime > maxslp &&
		    (rp->p_stat==SSLEEP&&rp->p_pri>PZERO||rp->p_stat==SSTOP)) {
			if (sleeper < rp->p_slptime) {
				p = rp;
				sleeper = rp->p_slptime;
			}
		} else if (!sleeper && (rp->p_stat==SRUN||rp->p_stat==SSLEEP)) {
			rppri = rp->p_rssize;
			if (rp->p_textp)
				rppri += rp->p_textp->x_rssize/rp->p_textp->x_ccount;
			if (biggot < nbig)
				nbp = &bigp[biggot++];
			else {
				nbp = bplist.bp_link;
				if (nbp->bp_pri > rppri)
					continue;
				bplist.bp_link = nbp->bp_link;
			}
			for (bp = &bplist; bp->bp_link; bp = bp->bp_link)
				if (rppri < bp->bp_link->bp_pri)
					break;
			nbp->bp_link = bp->bp_link;
			bp->bp_link = nbp;
			nbp->bp_pri = rppri;
			nbp->bp_proc = rp;
		}
	}
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
#ifdef mips
	XPRINTF(XPR_VM,"sched: at sleeper if\n",0,0,0,0);
#endif mips
	if (sleeper || desperate && p || deservin && inpri > maxslp) {
#ifdef mips
		(void) splhigh();
#endif mips
#ifdef vax
		(void) spl6();
#endif vax
#ifndef ultrix
		lock(LOCK_RQ);
		if (p->p_stat == SRUN) {
			for (cpuindex = 0; cpuindex < activecpu; cpuindex++) {
				if (p == cpudata[cpuindex].c_proc &&
				   	   cpudata[cpuindex].c_noproc == 0) {
					break;
				}
			}

			if (cpuindex < activecpu) {
				unlock(LOCK_RQ);
				(void) spl0();
				goto doneit;
			} else	remrq(p);
		}
 		p->p_flag &= ~SLOAD;
		unlock(LOCK_RQ);
#else
 		p->p_flag &= ~SLOAD;
		if (p->p_stat == SRUN) 
			remrq(p);
#endif !ultrix
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
		if (swapout(p, p->p_dsize, p->p_ssize,
					   p->p_smsize)	/* SHMEM */
					   		 == 0)
			deficit -= imin(gives, deficit);
		goto loop;
	}
	/*
	 * Want to swap someone in, but can't
	 * so wait on runin.
	 */
doneit:
#ifdef mips
	XPRINTF(XPR_VM,"sched: at doneit\n",0,0,0,0);
#endif mips
#ifdef mips
	(void) splhigh();
#endif mips
#ifdef vax
	(void) spl6();
#endif vax
	runin++;
#ifdef mips
	XPRINTF(XPR_VM,"sched: sleeping on runin\n",0,0,0,0);
#endif mips
	sleep((caddr_t)&runin, PSWP);
	(void) spl0();
	goto loop;
}

vmmeter()
{
	register unsigned *cp, *rp, *sp;

#ifdef mips
	XPRINTF(XPR_VM,"enter vmmeter",0,0,0,0);
#endif mips
	deficit -= imin(deficit,
	    imax(deficit / 10, ((klin * CLSIZE) / 2) * maxpgio / 2));
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

#ifdef mips
	XPRINTF(XPR_VM,"enter schedpaging",0,0,0,0);
#endif mips
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

vmtotal()
{
	register struct proc *p;
	register struct text *xp;
	int nrun = 0;

#ifdef mips
	XPRINTF(XPR_VM,"enter vmtotal",0,0,0,0);
#endif mips
	total.t_vmtxt = 0;
	total.t_avmtxt = 0;
	total.t_rmtxt = 0;
	total.t_armtxt = 0;
	for (xp = text; xp < textNTEXT; xp++)
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
	total.t_vm = 0;
	total.t_avm = 0;
	total.t_rm = 0;
	total.t_arm = 0;
	total.t_rq = 0;
	total.t_dw = 0;
	total.t_pw = 0;
	total.t_sl = 0;
	total.t_sw = 0;
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_flag & SSYS)
			continue;
		if (p->p_stat) {
			total.t_vm += p->p_dsize + p->p_ssize;
			total.t_rm += p->p_rssize;
			switch (p->p_stat) {

			case SSLEEP:
			case SSTOP:
				if (p->p_pri <= PZERO)
					nrun++;
				if (p->p_flag & SPAGE)
					total.t_pw++;
				else if (p->p_flag & SLOAD) {
					if (p->p_pri <= PZERO)
						total.t_dw++;
					else if (p->p_slptime < maxslp)
						total.t_sl++;
				} else if (p->p_slptime < maxslp)
					total.t_sw++;
				if (p->p_slptime < maxslp)
					goto active;
				break;

			case SRUN:
			case SIDL:
				nrun++;
				if (p->p_flag & SLOAD)
					total.t_rq++;
				else
					total.t_sw++;
active:
				total.t_avm += p->p_dsize + p->p_ssize;
				total.t_arm += p->p_rssize;
				break;
			}
		}
	}
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

#ifdef mips
	XPRINTF(XPR_VM,"enter loadav",0,0,0,0);
#endif mips
	for (i = 0; i < 3; i++)
#ifdef vax
		avg[i] = cexp[i] * avg[i] + n * (1.0 - cexp[i]);
#endif vax
#ifdef mips
                avg[i] = MUL_2FIX(cexp[i], avg[i]) + n * one_cexp[i];
#endif mips
}
