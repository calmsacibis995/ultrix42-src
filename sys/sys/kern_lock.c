#ifndef lint
static	char	*sccsid = "@(#)kern_lock.c	4.4	(ULTRIX)	2/21/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1988 by			*
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

/*	Modification history
 *
 * 20-Feb-91 jaw
 *	fix for mp locking in unix domain sockets.
 *
 * 18-Dec-90 Jim Paradis
 *	Idle vector processor before stopping CPU (VAX only)
 *
 * 18-Jan-90 Joe Comuzzi
 *      Initialize utctime lock
 *
 * 03-Mar-90 jaw
 *	primitive change to optimize mips.
 *
 * 02-Jan-90 jaw
 *	move slpri calculation inside lk_rq lock.
 *
 * 02-Jan-90 gmm
 *	More generic work around for the lock timeout problem for mips
 *
 * 08-Dec-89 gmm
 *	Try getting lock once more after max_seconds_wait. If it succeeds, log
 *	an error in error log and continue.
 *
 * 09-Nov-89 jaw
 *	move smp parameters to /sys/conf/machine/param.c.  handle
 *	spin count rollover.
 *
 * 20-Jul-89 jaw
 *	changes to lock structure, cleanup debug code.
 *
 *  9-Jun-89	Larry Scott
 *	Add initialisation of audit locks.
 *
 *  8-May-89	Giles Atkinson
 *	Add initialisation of LMF data lock.
 *
 * 24-Apr-89 -- jaw 
 *	fix race condition in process tracing between "ptrace" and child
 *	exiting.
 *
 * 12-Jan-89 - jaw
 *	hierpos changed to make 0 highest priority
 *
 */
#include "../h/types.h"
#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/cpu.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/smp_lock.h"
#include "../h/cpudata.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
/*
 * This routine checks to see if we own the lock "l".
 */
smp_owner(l) 
	register struct lock_t *l;
{

	register struct lock_t *sl;

	/* get head of held locks link list */
	sl = CURRENT_CPUDATA->cpu_hlock;

	while (sl != l) {
			
		if (sl == 0) 
			/* we do not own the lock */
			return(LK_FALSE);

		/* traverse list */
		sl = sl->l_plock;

	}
	/* we own the lock */
	return(LK_TRUE);
}


check_lock(l,pcall)
caddr_t pcall;
struct lock_t *l;
{
	struct cpudata *pcpu;
	int ipl;

	pcpu=CURRENT_CPUDATA;

	if (((l->l_type) != LK_WAIT) 
		&& ((l->l_type)  != LK_SPIN)) {
		panic("smp_lock_long: invalid lock type");
	}
	if(pcpu->cpu_hlock) {
		if (l->l_hierpos > pcpu->cpu_hlock->l_hierpos) { 
			panic("smp_lock_long: lock position messup");
		}
	}
#ifdef vax
	if ((l->l_ipl) > (ipl= mfpr(IPL)) ) {
#endif vax
#ifdef mips
	if ((l->l_ipl) > (ipl=whatspl(getspl()) )) {
#endif mips
		smp_enable_trace=0;
		printf("lock = %x, l_ipl = %x, ipl = %x callpc = %x\n",
			l, l->l_ipl, ipl, pcall);
		panic("smp_lock_long: wrong spl");
	}
}

extern int max_spin_count;
extern int max_seconds_wait;
extern int max_sleep_count;

smp_lock_long(l,flag,pcall)
	register struct lock_t *l;
	register int flag;
	caddr_t pcall;  	/* this must be the third argument so
				   asm below works properly */
{
	register struct lock_trace *lk_cpu_trace;
	register struct cpudata *pcpu;
	int slpri;
	int s;
	register int spin_cnt=0;
	int ipl;
	int starttime;
	int timediff;
#ifdef vax
	/* the next ASM will change the caller from the JSB routine
	   to the caller of the smp_lock */
	asm("movl 12(ap),16(fp)");
#endif vax
	pcpu = CURRENT_CPUDATA;
	/* always check to see if we are owner.  This isolates
	   a problem where a processes trys to get a sleep lock
	   twice.  If this check isn't here then the process 
	   sleeps forever... */
	if (smp_owner(l) == LK_TRUE) {
		panic("smp_lock_long: lock owner");
	}
	if (smp_debug) 
		check_lock(l,pcall);

	starttime = (int) pcpu->cpu_cptime[CP_SYS];


    while(LK_TRUE) {
	if (panicstr && ((pcpu->cpu_state & CPU_PANIC) == 0)) {
		spl7();
		if  (BOOT_CPU){ 
			panic("secondary cpu requested ");
		}
		else  {
#if defined(__vax)
			/* If this is a vector-capable CPU, then wait
			 * for the attached vector processor to go idle.
			 This is a no-op on non-vector-capable VAXen
			 */
			vp_idle();
#endif /* __vax */
			stop_secondary_cpu();		
		}
	} 
	/* test lock */
	if (l->l_lock==0)
		if (setlock(l)) 
			break; /* won the lock!  */
	
	/* if LK_ONCE...return */
	if ((flag & LK_RETRY) == 0){
		return (LK_LOST);
	}

	if ((l->l_type) == LK_WAIT) {

		s = spl6();
		smp_lock(&lk_rq, LK_RETRY);
		l->l_wanted++;
		/* two setlocks are done to workaround hardware
		   problem in 8840. (JAW) */
		if (smp && (setlock(l) || setlock(l))) {
			l->l_wanted--;
			smp_unlock(&lk_rq);
			splx(s);
			break;
		}
		/* caculate priority to sleep at.  Must be an 
		   non-interruptable sleep, so insure we are 
		   at or below PZERO */
		slpri =(pcpu->cpu_proc->p_pri > PZERO ?
			PZERO:pcpu->cpu_proc->p_pri);
		sleep_unlock(l,slpri ,&lk_rq);
		pcpu = CURRENT_CPUDATA;
		splx(s);
		/* fall through to top of loop */

	} 

	/* if debug is off then we will check the lock ordering..etc.. 
	   only after we lose the lock once. */
	if (spin_cnt == 0) 
		check_lock(l,pcall);

	spin_cnt++;

#ifdef mips
	if(!(spin_cnt%(max_spin_count/10)))
	      if (setlock(l)) {
		      mprintf("smp_lock: got lock in second try: l = %X, pcall = %X, l_pc = %X\n",l,pcall,l->l_pc);
		      break;
	      }
#endif mips
	if ((l->l_type) == LK_SPIN) {

		/* non-smp machines shouldnot spin ! */
		if (smp==0) 
		     panic("smp_lock_long: non-smp spin on spinlock\n"); 

		/* for spin locks above clock level the cpu must
		  not look at the system time to do timeout since
		  it can not move forward */
#ifdef vax
		if  (mfpr(IPL) >= SPLCLOCK) {
#endif vax
#ifdef mips
		if  ((whatspl(getspl())) >= SPLCLOCK) {
#endif mips
			if (spin_cnt > max_spin_count) 
			      panic("smp_lock_long: beyond spin count\n");
		} else {
			timediff = pcpu->cpu_cptime[CP_SYS] - starttime;

			if ( timediff < 0)
				timediff = -timediff;

			if (timediff  > (max_seconds_wait * hz)) {
			      if (setlock(l)) {
				      mprintf("smp_lock: got lock on second try: l = %X, pcall = %X, l_pc = %X\n",l,pcall,l->l_pc);
				      break;
			      }
		     	      panic("smp_lock_long: beyond time wait \n"); 
		        }
		}
	} else 
		/* for sleep locks we don't want to sleep/wakeup too
		   many times. */
		if (spin_cnt > max_sleep_count) 
		      panic("smp_lock_long: beyond sleep count\n");


    } /* end big while loop */

        /* bump stats */
	if (spin_cnt){ 
		if (debug_lock_trace && (l == debug_lock_trace->traced_lock)) {
			register struct debug_entry *ptc;

			ptc = &debug_lock_trace->element[debug_lock_trace->current_lock_trace];
			ptc->spinct = spin_cnt;
			ptc->calling_pc = pcall;
			ptc->holding_pc = l->l_pc;
			if (++debug_lock_trace->current_lock_trace >= 
				debug_lock_trace->debug_lock_entries)
				debug_lock_trace->current_lock_trace = 0;
			
		}
		l->l_lost++;
		l->l_spin += spin_cnt;
	}
	l->l_won++;

	if (smp_debug && smp_enable_trace) {
		s=spl7();

		if (++pcpu->cpu_l_ctrace >= 
			&pcpu->cpu_l_trace[CPU_LK_TRACES])
			pcpu->cpu_l_ctrace = &pcpu->cpu_l_trace[0];
		lk_cpu_trace = pcpu->cpu_l_ctrace;
		splx(s);

		/* save off PID if valid */
		if (pcpu->cpu_noproc ==0) {
			lk_cpu_trace->tr_pid = pcpu->cpu_proc->p_pid;
		} else {
			lk_cpu_trace->tr_pid = -1;
		}

#ifdef vax
		/* save off psl */
		lk_cpu_trace->tr_psl = movpsl();
#endif vax

		/* save off calling pc */
		lk_cpu_trace->tr_pc =  pcall;

		/* save off lock address */
		lk_cpu_trace->tr_lock= l;

	}

	/* add lock to lock chain */
	l->l_plock = pcpu->cpu_hlock;
	pcpu->cpu_hlock = l;

	/* save pc lock asserted */
	l->l_pc = pcall;

	return(LK_WON);
}

/*
 *	note!!!! this is also done in lock.s to make if fast!
 */
smp_unlock_long(l)
	struct lock_t *l;
{
	int s;
	register struct cpudata *pcpu;
	register struct lock_t *sl, **pl;

#ifdef vax
	/* the next ASM will change the caller from the JSB routine
	   to the caller of the smp_unlock */
	asm("movl 8(ap),16(fp)");
#endif

	pcpu = CURRENT_CPUDATA;

	switch (l->l_type) {

		case LK_SPIN:
		case LK_WAIT:

			/* delete lock from lock chain */
			pl = &(pcpu->cpu_hlock);
			sl = *pl;
			while (sl) {
				if (l == sl) {
					*pl = sl->l_plock;
					sl->l_plock = (struct lock_t *)NULL;
					break;
				}
				pl = &sl->l_plock;
				sl = *pl;
			}
		
			if (!sl) {
				panic("smp_unlock_long: not lock owner");
			}
			break;

		default:
			panic("smp_unlock_long: invalid lock address");

	}

	if (smp) clearlock(l);
	else l->l_lock=0;

	if (l->l_wanted && (l->l_type == LK_WAIT)) {
		int had_rq = 0;
		/*
		 * we can already have the lk_rq at this point
		 * if we are called from sleep_unlock or issig
		 */
		if ((had_rq = smp_owner(&lk_rq)) == 0) {
			s=spl6();
			smp_lock(&lk_rq, LK_RETRY);
		}
		if (l->l_wanted > 0) {
			l->l_wanted--;
			/*
			 * we MUST wake someone up now
			 * the ONE flag tells wakeup to wake
			 * only a single process
			 */
			if (wakeup_type(l,WAKE_ONE)==0) 
				panic("smp_unlock_long: no process woken\n");
		}
		if (had_rq == 0) {
			smp_unlock(&lk_rq);
			splx(s);
		}
	}
}

/*
 * Initialize a lock structure with type and hierarchical position.
 * If debugging is being done, point c_trace to last element of array.
 * 
 * Returns:	TRUE if initialized
 *		FALSE otherwise
 */
lockinit(l,lockdata)
	struct lock_t *l;
	register struct lock_data *lockdata;
{
	if (smp || (lockdata->l_type==LK_WAIT))
		l->l_call= 1;
	l->l_ipl = lockdata->l_ipl;
	l->l_type = lockdata->l_type;
	l->l_hierpos = lockdata->l_hierpos;
	l->l_wanted = 0;
	l->l_plock = 0;
	l->l_lost = 0;
	l->l_spin = 0;
	l->l_won = 0;
}

print_locks_held(num_cpu)
int num_cpu;
{
	register struct lock_t *l;
	caddr_t pc;
	register struct cpudata *pcpu;

	pcpu =CPUDATA(num_cpu); 
	if (pcpu == 0 ) return;

	l= pcpu->cpu_hlock;

	cprintf("\n\nlocks held by cpu %d \n",num_cpu);
	while(l) {
		cprintf("l= %x, l_type= %x, l_hierpos= %x l_ipl= %x\n	l_wanted= %x l_pc %x\n",
			l,l->l_type,l->l_hierpos,l->l_ipl,l->l_wanted,l->l_pc);
		l=l->l_plock;
	}

}

print_lock_trace(num_cpu) 
register int num_cpu;

{
	register int i;
	register struct lock_trace *t;
	register struct cpudata *pcpu;

	pcpu = CPUDATA(num_cpu);

	if ((pcpu ==0) || (smp_debug ==0))return;

	t = pcpu->cpu_l_ctrace;
	

	cprintf("\n\nlock trace for cpu %d \n",num_cpu);

	for (i=0; i < CPU_LK_TRACES; i++ ) {
		if (t < &pcpu->cpu_l_trace[0]) 
			t=(struct lock_trace *) &pcpu->cpu_l_trace[CPU_LK_TRACES-1];
		cprintf(" l = %x , pc = %x, psl = %x, pid = %d \n",
			t->tr_lock,t->tr_pc,t->tr_psl,t->tr_pid);
			
		--t;
	}
}

/*
 *  this routine prints locks held by processes not currently 
 *  active.
 */
print_process_locks() {
	int i;
	struct lock_t *l;
	cprintf("\n\nprint locks held by non-active processes\n");
	/* look at all procs */
	for(i=0; i< nproc; i++) {

		/* check if lock held */
		if (l = proc[i].p_hlock) {
			DELAY(400000);
			cprintf("proc %x, pid %d holds the following locks:\n",
				  &proc[i],proc[i].p_pid);

			/* print out lock list */
			l = proc[i].p_hlock;
			while(l) {
				cprintf("lock= %x, pc=%x \n",l,l->l_pc);
				l = l->l_plock;
			}
			printf("\n");
		}
			
	}
	cprintf("done\n");
}



kern_lock_init() {

	int i;
        extern struct lock_t lk_lmf;
	extern struct lock_t lk_utctime;
	extern struct lock_data lock_utctime_d;
	extern struct lock_t lk_so_disconnect;
	extern struct lock_data lock_so_disconnect_d;


	/* initialize error logger */
	init_errlog();

	/* initialize lock to synchronize both sides of unix domain socket */
	lockinit(&lk_so_disconnect, &lock_so_disconnect_d);

	/* initialize lock that keeps slave processors from starting. */
	lockinit(&lk_printf,&lock_printf_d);

	/* initialize locks that are optionally complied into system 
	   routine lives in kern_lock_data.c */
	options_lock_init();

	slpque_init();
	lockinit(&lk_debug,&lock_debug_d);	
	lockinit(&lk_realtimer,&lock_realtimer_d);

	/* initialize timeout queue lock */
	lockinit(&lk_timeout,&lock_timeout_d);
 
	/* initialize utctime lock */
        lockinit(&lk_utctime,&lock_utctime_d);

	rm_lock_init();  /* initialize locking for resource maps. */
	lockinit(&lk_select,&lock_select_d);

	/* Network locks.  */
	lockinit(&lk_rtentry, &lock_rtentry_d);
	lockinit(&lk_net_mgt, &lock_net_mgt_d);
	lockinit(&lk_ifnet, &lock_ifnet_d);	
	/* 3.31.89.us  Added lk_tcpiss to control tcp_iss and
	 * tcp_slow_active.  */
	lockinit(&lk_tcpiss, &lock_tcpiss_d);

	/* audit locks */
	lockinit(&lk_auditmask, &lock_auditmask_d);
	lockinit(&lk_audbuf, &lock_audbuf_d);
	lockinit(&lk_audlog, &lock_audlog_d);

	/* Miscellaneous locks */
	lockinit(&lk_lmf, &lock_lmf_d);
}

lsert(l, str)
	struct lock_t *l;
	char *str;
{

	if (smp_owner(l) == LK_TRUE)
		return;
	
	smp_enable_trace=0;
	cprintf("%x not owned from %s\n", l, str);
	panic("lsert");
}

swtch_check()
{
	/* make sure no spin locks are held going into
	 * swtch except for lk_rq:  lk_rq must be held
	 */
	register struct lock_t *l;

	l= CURRENT_CPUDATA->cpu_hlock;

	if (l == NULL) {
		return;
	}

	do {
		if ((l != &lk_rq) && ((l->l_type) == LK_SPIN)){
			panic("swtch holds not just rq");
		}
		l = l->l_plock;
	} while(l);
}

sleep_check() {
	register struct lock_t *l;					
#ifdef vax
	if (movpsl() & PSL_IS)						
		panic("could sleep on interrupt stack");	
#endif vax

	for (l = CURRENT_CPUDATA->cpu_hlock; l; l = l->l_plock)		
		if (l->l_type == LK_SPIN)				
			panic("could sleep holding spin lock"); 
}
