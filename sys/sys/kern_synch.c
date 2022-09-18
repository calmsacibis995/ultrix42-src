#ifndef lint
static	char	*sccsid = "@(#)kern_synch.c	4.4	(ULTRIX)	12/20/90";
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/kern_synch.c
 *
 *
 * 19-Dec-90 -- jaw
 *	clear out "slptime" in setrun for mips...
 *
 * 16-Apr-90 -- jaw
 *	performance fixes for single cpu.
 *
 * 06-Mar-90 gmm
 *	Prevent lowering a process's priority to be interruptible (same is
 *	being done in VAX) in mips under some conditions.
 *
 * 03-Mar-90 jaw
 *	fix missed wakeup if stop signal is recieved as a process
 *	is being put to sleep.
 *
 * 09-Nov-89 jaw
 *	allow spin locks to be compiled out.
 *
 * 10-Oct-89 jaw 
 *	fix bug in schedcpu where a priority of a sleeping process below
 * 	PZERO could get changed.
 *
 * 9-Oct-89 jaw 
 *	fix to priority computation in schedcpu that was dropped in PMAX
 *	merge.  
 *
 * 20-Jul-89 jaw
 *	fix hang when vfork gets stop signal.
 *
 * 12-Jun-89 bp
 *	Added check in schedcpu to detect kernel memory allocator
 *	map shortage.  A true condition results in high water mark
 *	trimming in an attempt to free allocator map resource.
 *
 *  11-May-89 jaw
 *    fix uninitialized variable in schedcpu.
 *
 *  26-Jan-89	jaw
 *	SMP clean up 
 *
 * 10-Oct-88 -- jaw
 *	replace switch_to_master with general routine switch_affinity
 *
 * 28-Jul-88 -- miche
 *	protect remaining scheduling fields in the process structure
 *
 * 10-Jun-88 -- jaw 
 * 	add parameter to ISSIG for SMP.... this makes going to stop
 *	state atomic.
 *
 * 07 Jun 88 -- miche
 *	SMP procqs:  use FORALLPROC to find processes
 *
 * 27 Jan 88 -- gmm
 *	Changed intrcpu() to intrpt_cpu() to conform to the new IPI interface
 ********* SMP changes above ******************
 *
 * 30-Aug-88	jaw
 *	add proc field p_master to fix ASMP bug.
 *
 * 08 Jul 87 -- depp
 *      Added PCATCH to sleep().  This flag will indicate to sleep that
 *      if a signal is received during sleep, that control must be returned
 *      to the calling routine (error indication), so that routine may
 *      cleanup.  Semaphores and message queues are the primary 
 *      recipient of this feature.  No longjmp is performed in this case.
 *
 * 21 Jan 87 -- jaw
 *	performance fixes to syscall.
 *
 * 11-Sep-86 -- koehler
 *	added debug code to check for missed wakeups
 *
 * 15-Apr-86 -- jf
 *	Add support for system process sleep and wakeup
 *
 * 02-Apr-86 -- jrs
 *	Clean up low pri cpu determination and improve single cpu case
 *
 * 18 Mar 86 -- jrs
 *	Clean up cpu determination and preemption
 *
 * 12-Feb-86 -- pmk 
 *	Change spl4 to spl1 in sleep rundown check.
 *	Move rundown check in sleep to before panic check.
 *
 * 16 Jul 85 -- jrs
 *	Add run queue locking and multicpu sched code
 *
 * 20-Jan-86 -- pmk
 *	Add check in sleep for rundown - if set don't swtch
 *	fix recursive panic
 *
 * 25 Oct 84 -- jrs
 *	Add changes for proc queue lists
 *	Derived from 4.2BSD, labeled:
 *		kern_synch.c 6.2	84/05/22
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/gnode.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/smp_lock.h"
#include "../h/cpudata.h"
#include "../h/kmalloc.h"

extern int smp;
#ifdef vax
#include "../machine/mtpr.h"	/* XXX */
#endif
extern int runrun;;
#ifdef vax
double avenrun[3];
#endif vax
#ifdef mips
fix avenrun[3];
#endif mips

/* fraction for digital decay to forget 90% of usage in 5*loadav sec */
#ifdef vax
#define	filter(loadav) ((2 * (loadav)) / (2 * (loadav) + 1))
#endif vax
#ifdef mips
#define	filter(loadav) DIV_2FIX(MUL_2FIX(TO_FIX(2), (loadav)), \
				MUL_2FIX(TO_FIX(2), (loadav) + TO_FIX(1)))
#endif mips

#ifdef vax
double	ccpu = 0.95122942450071400909;		/* exp(-1/20) */
#endif vax

#ifdef mips
#if (FBITS != 8)
# include "Error: need to redefine ccpu decay constant."
#else
fix	ccpu		= 244;		/* (1<<8)*exp(-1/20) */
fix	one_ccpu	= 12;		/* (1<<8)*(1 - exp(-1/20)) */
#endif FBITS != 8
#endif mips


/*
 * Recompute process priorities, once a second
 * SMP note:  this routine runs only on one processor, and we don't
 * care enough about the accuracy of these statistics to lock
 */
schedcpu()
{
#ifdef vax
	register double ccpu1 = (1.0 - ccpu) / (double)hz;
#endif vax
	register int s, a;
	register struct cpudata *pcpu;
	int pri;
#ifdef vax
	float scale = filter(avenrun[0]);
#endif vax
#ifdef mips
	extern fix avenrun[];
	fix scale = filter(avenrun[0]);
#endif mips

	wakeup((caddr_t)&lbolt);

	FORALLPROC(
		if (pp->p_time != 127)
			pp->p_time++;
		if (pp->p_stat==SSLEEP || pp->p_stat==SSTOP)
			if (pp->p_slptime != 127)
				pp->p_slptime++;

		/*
                 * If the process has slept the entire second,
                 * stop recalculating its priority until it wakes up.
                 */
		if (pp->p_slptime > 1) {
#ifdef vax
			pp->p_pctcpu *= ccpu;
#endif vax
#ifdef mips
			pp->p_pctcpu = MUL_2FIX(pp->p_pctcpu, ccpu);
#endif mips
                        NEXTPROC;
                }

		if (pp->p_sched&SLOAD)
#ifdef vax
			pp->p_pctcpu = ccpu * pp->p_pctcpu + ccpu1 * pp->p_cpticks;
#endif vax
#ifdef mips
			pp->p_pctcpu = MUL_2FIX(pp->p_pctcpu, ccpu) +
				(one_ccpu * pp->p_cpticks) / hz;
#endif mips
		pp->p_cpticks = 0;
#ifdef vax
		a = (int) (scale * (pp->p_cpu & 0377)) + pp->p_nice - NZERO;
#endif vax
#ifdef mips
		a = FIX_TO_INT(scale * (pp->p_cpu & 0377)) + pp->p_nice - NZERO;
#endif mips
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		pp->p_cpu = a;

		pri = (a & 0377)/4;
		pri += PUSER + 2*(pp->p_nice - NZERO);
		if (pp->p_rssize > pp->p_maxrss && freemem < desfree)
			pri += 2*4;	/* effectively, nice(4) */
		if (pri > 127)
			pri = 127;
		pp->p_usrpri = pri;

		/* we double do this check to avoid getting the lk_rq lock
		 * unless we really need it to reduce contention
		 */
		if (pp->p_pri >= PUSER && pp->p_stat == SRUN) {
			s = splhigh();
#ifdef vax
#define	PPQ	(128 / NQS)
#else mips
#define PPQ	1
#endif mips
			smp_lock(&lk_rq,LK_RETRY);
			if (pp->p_pri >= PUSER &&
			    pp->p_stat == SRUN) {
				if ((pp->p_sched & SLOAD) &&
			    	   (pp->p_pri / PPQ) != (pp->p_usrpri / PPQ)) {
					for (a = lowcpu; a <= highcpu ; a++) {
						pcpu = CPUDATA(a);
						if (pcpu &&
						    pp == pcpu->cpu_proc &&
						    pcpu->cpu_noproc == 0){
							pp->p_pri=pp->p_usrpri;
							break;
						}
					}
					if (a > highcpu) {
						remrq(pp);
						pp->p_pri = pp->p_usrpri;
						setrq(pp);
					}
			
				} else {
						pp->p_pri = pp->p_usrpri;
				}
			}

			smp_unlock(&lk_rq);
			splx(s);
		}
	) /*end FORALLPROC */

	if (kmemneedmap) {
#ifdef	KMEM_DEBUG
		kmemd_gs.gs_lowmap++;
#endif	KMEM_DEBUG
		if (km_hwmscan(KM_HWMHARD))  km_intr_cpus();
	}

	vmmeter();
	if (runin!=0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}
	if (bclnlist != NULL)
		wakeup((caddr_t)&proc[2]);
	timeout(schedcpu, (caddr_t)0, hz);
}
#ifdef mips
/*
 * Recalculate the priority of a process after it has slept for a while.
 */
updatepri(p)
	register struct proc *p;
{
	register int a = p->p_cpu & 0377;
#ifdef vax
	float scale = filter(avenrun[0]);
#endif vax
#ifdef mips
	fix scale = filter(avenrun[0]);
#endif mips

	p->p_slptime--;		/* the first time was done in schedcpu */
	while (a && --p->p_slptime)
#ifdef vax
		a = (int) (scale * a) /* + p->p_nice */;
#endif vax
#ifdef mips
		a = FIX_TO_INT (scale * a) /* + p->p_nice */;
#endif mips
	if (a < 0)
		a = 0;
	if (a > 255)
		a = 255;
	p->p_cpu = a;
	(void) setpri(p);
}
#endif mips

#define SQSIZE 0100	/* Must be power of 2 */
#define HASH(x)	(( (int) x >> 5) & (SQSIZE-1))
struct {
struct proc *slp_forward;
struct proc *slp_back;
}slpque[SQSIZE];

slpque_init() {
	int i;

	for(i=0; i< SQSIZE; i++) {
		slpque[i].slp_forward= (struct proc *)&slpque[i];
		slpque[i].slp_back = (struct proc *) &slpque[i];
	}
}

/*
 * Give up the processor till a wakeup occurs
 * on chan, at which time the process
 * enters the scheduling queue at priority pri.
 * The most important effect of pri is that when
 * pri<=PZERO a signal cannot disturb the sleep;
 * if pri>PZERO signals will be processed.
 * Callers of this routine must be prepared for
 * premature return, and check that the reason for
 * sleeping has gone away.
 *
 * PCATCH is a SYSTEM Vism that is used to avoid the longjmp if
 * the calling routine requires notification for cleanup.  SYSTEM V
 * IPC requires this, particularly semaphores.
 */
sleep(chan, pri)
	caddr_t chan;
	int pri;
{
	return(sleep_unlock(chan,pri,0));
}

sleep_unlock(chan, pri,l)
	caddr_t chan;
	register struct lock_t *l;
	register int pri;
{
	register struct proc *rp;
#ifdef mips
	register struct proc *slq;
#endif mips
	register int s;
	register struct cpudata *pcpu;
	register int pcatch = pri & PCATCH;
	
	pri &= PMASK;
	rp = u.u_procp;

	/*
	 * the lock held can be lk_rq out of smp_lock in
	 * the case of wait locks which are lost: holding
	 * lk_rq ensures that we do not miss wakeups
	 * There is a nesting here: smp_lock,
	 *	sleep_unlock (issig), smp_unlock, wakeup_type.
	 */
	s = splhigh();


	/*
	 * MBH:  why all this silliness with spl levels?
	 */
	if (rundown != 0) {		/* set in boot - don't swtch */
		if(l) {
			smp_unlock(l);		
		}
#ifdef vax
		(void) spl1();
#else
		(void) spl0();
#endif vax
	    	splx(s);
	    	goto out;
	}

	if (chan==0 || rp->p_stat != SRUN) {
		panic("sleep");
	}

	if (smp) {

		if (pri > PZERO) {		/* check for signals? */
			/*
			 * This is coordination with the psignal routine:
		 	 * we need to atomically check for no signals and
			 * get to the sleep state, or we can miss a kill
			 * signal forever going to sleep.  
			 */
			smp_lock(&lk_signal,LK_RETRY);
		}

		/* get rq lock if we don't have it */
		if (&lk_rq != l) {
			smp_lock(&lk_rq, LK_RETRY);
			if (l) smp_unlock(l);
		}
	} else if (l) smp_unlock(l);

	/* put process onto the sleep queue.  This is done before checking
	   for signals to handle the case where a stop signal is taken,
	   a wakeup takes place and then a sigcont happens.  If the process
	   was put on the sleep queue after signals, it would be possible to
	   miss a wakeup */
	u.u_ru.ru_nvcsw++;
	rp->p_slptime = 0;
	rp->p_wchan = chan;
#ifdef vax
	insque(rp,slpque[HASH(chan)].slp_back);
#endif vax
#ifdef mips
	slq = slpque[HASH(chan)].slp_back;
	rp->p_link = slq->p_link;
	rp->p_rlink = slq;
	slq->p_link->p_rlink = rp;
	slq->p_link = rp;
#endif mips 
	rp->p_pri = pri;

	if (pri > PZERO) {
		/* can't do sleep_unlock that is interruptable holding
		  a high priority lock */
		if (rp->p_sig && (rp->p_trace&STRC ||
		    (rp->p_sig &~ (rp->p_sigignore | rp->p_sigmask)))) {
			if (issig_subr(1)) {
				smp_unlock(&lk_signal);
				/* the running process must take a signal.  
				   need to take the process back off the 
				   sleep queue */
				if (rp->p_wchan)  
					unsleep(rp);
				rp->p_stat = SRUN;
				smp_unlock(&lk_rq);
				(void) spl0();
				goto psig;
			}
			/* wakeup has happened */
			if (rp->p_wchan ==0) {
				smp_unlock(&lk_signal);
				smp_unlock(&lk_rq);
				goto out;
			}	
		}
		smp_unlock(&lk_signal);
	} 
	pcpu = CURRENT_CPUDATA;
	if (pcpu->cpu_hlock) {
		if (smp) {
			/*
	 		 * The run queue lock is first on the list:
	 		 * Save the second on down 
	 		 */
			pcpu->cpu_proc->p_hlock = pcpu->cpu_hlock->l_plock;
			pcpu->cpu_hlock->l_plock=0;
		} else {
			pcpu->cpu_proc->p_hlock = pcpu->cpu_hlock;
			pcpu->cpu_hlock=0;
		}	
		rp->p_stat = SSLEEP;
		swtch();
		pcpu = CURRENT_CPUDATA;
		pcpu->cpu_hlock = pcpu->cpu_proc->p_hlock;
		pcpu->cpu_proc->p_hlock=0;
	} else {
		rp->p_stat = SSLEEP;
		swtch();
	}
	if (pri > PZERO && ISSIG(rp,0)) {
		(void) spl0();
		goto psig;
	}
	(void) spl0();

out:
	splx(s);
	return(0);

	/*
	 * If priority was low (>PZERO) and
	 * there has been a signal, execute non-local goto through
	 * u.u_qsave, aborting the system call in progress (see trap.c)
	 * (or finishing a tsleep, see below)
	 */
psig:
	splx(s);
	if(pcatch) return(1);
	longjmp(&u.u_qsave);
	/*NOTREACHED*/
}

/*
 * Remove a process from its wait queue
 */
unsleep(p)
	register struct proc *p;
{
	register int s;

/* if (smp_debug) lsert(&lk_rq,"unsleep"); */

	if (p->p_wchan) {
#ifdef vax
		remque(p);
#endif vax
#ifdef mips
		p->p_rlink->p_link = p->p_link;
		p->p_link->p_rlink = p->p_rlink;
#endif mips
		p->p_rlink = 0;
		p->p_wchan = 0;
	}
}
wakeup(chan) 
caddr_t chan;
{
	wakeup_type(chan,WAKE_ALL);
}


/*
 * Wake up all processes sleeping on chan.
 */
wakeup_type(chan,wake_type)
	register caddr_t chan;
	int wake_type;
{
	register struct proc *p,*q;
	int s, had_rq = 0;
	int checkrunout=0;
	int retval;
	if (wake_type == WAKE_ONE)
		retval = 0;
	else	retval = 1;

	if (smp && ((had_rq = smp_owner(&lk_rq))== 0)) {
		s = splhigh();
		smp_lock(&lk_rq,LK_RETRY);
	} else{
		s = splhigh();
	}

	for (p= slpque[HASH(chan)].slp_forward; p != (struct proc *)&slpque[HASH(chan)] ; p=q) {
		q = p->p_link;

		if ( p->p_stat != SSLEEP && p->p_stat != SSTOP) {
			panic("wakeup");
		}
		if (p->p_wchan==chan) {
			p->p_wchan = 0;
#ifdef vax
			remque(p);
#endif vax
#ifdef mips
			p->p_rlink->p_link = p->p_link; /* inline remque */
			p->p_link->p_rlink = p->p_rlink;
#endif mips
			p->p_rlink=0;
#ifdef vax
			p->p_slptime = 0;
#endif vax
			if (p->p_stat == SSLEEP) {
#ifdef mips
				if (p->p_slptime > 1)
					updatepri(p);
				p->p_slptime = 0;
#endif mips

				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				p->p_stat = SRUN;
				if (p->p_sched & SLOAD) {
					CURRENT_CPUDATA->cpu_runrun++;
					aston();
					setrq(p);
				}
				/*
				 * Since curpri is a usrpri,
				 * p->p_pri is always better than curpri.
				 */
				if ((p->p_sched&SLOAD) == 0) {
					checkrunout =1;
				}
				if (wake_type==WAKE_ONE) {
					retval = 1;
					break;
				}
			}
#ifdef mips
			p->p_slptime = 0;
#endif mips
		} 
	}
	if (checkrunout) {
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
		wantin++;
	}
	if (smp && (had_rq == 0)) {
		smp_unlock(&lk_rq);
		splx(s);
	} else {
		splx(s);
	}
	return(retval);
}

/*
 * Initialize the (doubly-linked) run queues
 * to be empty.
 */
rqinit()
{
	register int i;
	lockinit(&lk_rq,&lock_rq_d);
	lockinit(&lk_waitchk,&lock_waitchk_d);
	lockinit(&lk_procqs,&lock_procqs_d);
	lockinit(&lk_signal, &lock_signal_d);
	lockinit(&lk_pid, &lock_pid_d);
#ifdef vax
	for (i = 0; i < NQS; i++)
		qs[i].ph_link = qs[i].ph_rlink = (struct proc *)&qs[i];
#endif vax
#ifdef mips
	/*
	 * Single run queue implementation.
	 */
	qs.ph_link = qs.ph_rlink = (struct proc *)&qs;
#endif mips

	/*
	 * Since we only have bitmap for 512*32=16K processes, shrink
	 * back to this number if more are requested.  To change this
	 * requires modifications in proc.h to MAX_PROC_INDEX.
	 */
	if (max_proc_index > MAX_PROC_INDEX) {
		max_proc_index = MAX_PROC_INDEX;
		printf("WARNING:  %d processes requested, but only %d allocated\n",
			nproc, MAX_PROC_INDEX * 32);
		nproc = MAX_PROC_INDEX * 32;
	}
}

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */
setrun(p)
	register struct proc *p;
{
	register int s;
/* if (smp_debug) lsert(&lk_rq, "setrun"); */

	switch (p->p_stat) {

	case 0:
	case SWAIT:
	case SRUN:
	case SZOMB:
	default:
		panic("setrun");

	case SSTOP:
	case SSLEEP:
		unsleep(p);		/* e.g. when sending signals */
		break;

	case SIDL:
		break;
	}
	
#ifdef mips
	if (p->p_slptime > 1)
		updatepri(p);
#endif mips

	p->p_slptime = 0;  /* sleep time must be set to zero so that
			      priority calculations will continue in 
			      schedcpu */

	p->p_stat = SRUN;
	if (p->p_sched & SLOAD) {
		setrq(p);
		if (p->p_pri < CURRENT_CPUDATA->cpu_proc->p_pri) {
			aston();
			CURRENT_CPUDATA->cpu_runrun++;
		}	
	}
	if ((p->p_sched&SLOAD) == 0) {
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
		wantin++;
	}
}

/*
 * Set user priority.
 * The rescheduling flag (runrun)
 * is set if the priority is better
 * than the currently running process.
 */
setpri(pp)
	register struct proc *pp;
{
	register int p;
	int cpident;

	p = (pp->p_cpu & 0377)/4;
	p += PUSER + 2*(pp->p_nice - NZERO);
	if (pp->p_rssize > pp->p_maxrss && freemem < desfree)
		p += 2*4;	/* effectively, nice(4) */
	if (p > 127)
		p = 127;
	pp->p_usrpri = p;
	return (p);
}

pinglow(pri,p)
	register int	pri;
	struct proc *p;
{
	register struct cpudata *pcpu;
	register int index, lowpri_cpu; 
	register int lowpri = -1;

	for (index = lowcpu ; index <= highcpu; index++) {
	
		pcpu = CPUDATA(index);
		if (pcpu == 0 ) continue;

		if (((1<<index) & p->p_affinity) == 0 ) continue;
		if (pcpu->cpu_noproc != 0) {
			lowpri = 127;
			lowpri_cpu = index;
			return;
		}
		if (pcpu->cpu_proc->p_usrpri >= lowpri) {
			lowpri = pcpu->cpu_proc->p_usrpri;
			lowpri_cpu = index;
		}
	}
	if (lowpri > pri) {
		CPUDATA(lowpri_cpu)->cpu_runrun=1;
		if (lowpri_cpu == CURRENT_CPUDATA->cpu_num) {
			aston();
		} else {
			intrpt_cpu(lowpri_cpu,IPI_SCHED);
		}	
	}
}
