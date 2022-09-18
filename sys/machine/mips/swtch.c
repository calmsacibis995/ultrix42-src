#ifndef lint
static	char	*sccsid = "@(#)swtch.c	4.2	(ULTRIX)	11/9/90";
#endif lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../machine/cpu.h"
#include "../machine/reg.h"
#include "../machine/cpu_board.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#ifdef ultrix
#include "../h/cpudata.h"
#endif ultrix

/*
 *		Modification History
 *
 *
 * 	16-Apr-90 -- jaw
 *	performance fixes for single cpu.
 *
 *	29-Mar-90  -- gmm
 *	Changed splhigh() to splextreme() since now splhigh() same as
 *	splclock(). 
 *
 *	06-Mar-90  -- gmm
 *	Reduce contention on lk_rq (more like VAX now).
 *
 *	06-Feb-90  -- gmm
 *	Initialize cpu_roundrobin to 10 before resuming a process.
 *
 *	08-Dec-89  -- gmm
 *	Use a new routine (unmap_systlbs(p)) to flush only those system page
 *	table ptes for the process. Now flush_tlb() not called during every
 *	context switch.
 *
 *	14-Nov-89  -- gmm
 *	Release lk_rq in the context of the next process all the time. Flush 
 *	tlbs for every context switch if smp defined. Rename idle_swtch() to
 *	idle().
 *
 *	13-Oct-89  -- gmm
 *	SMP changes. Replaced idle() with idle_swtch(). Changes to handle
 *	idle process, tlbpids on a per processor basis etc.
 *
 *	08-Jun-89  -- gmm
 *	Switch over to idle stack before context switching. Also, do not save 
 *	the process context if the process is exiting.
 *
 */

#ifdef USE_IDLE
int	v_zeros = 0;			/* pages zeroed in idle() */
int	v_zero_pg_hits = 0;		/* hits on zeroed pages in vm_page.c */
int	v_zero_pg_misses = 0;		/* misses in vm_page.c */
int	v_zero_pt_hits = 0;		/* hits in vm_pt.c */
int	v_zero_pt_misses = 0;		/* misses in vm_pt.c */
int	zero_end = CMHEAD;		/* index of last zero'ed page */
#endif  USE_IDLE

/*
 * NOTE: This version of setrq/remrq/swtch does NOT use the VAX
 * multiple queue stuff (since we don't have a ffs instruction).
 * The runq is a simple priority ordered doubly-linked list.
 * Therefore qs must be changed from an array of structs to a single
 * struct.
 */

/*
 * setrq must be called at splclock
 */

setrq(p)
register struct proc *p;
{
	register unsigned pri;
	register struct proc *pp;

	if (p->p_rlink)
		panic("setrq p_rlink");
	pri = p->p_pri;
	for (pp = qs.ph_link; pp != (struct proc *)&qs; pp = pp->p_link)
		if (pri < pp->p_pri)
			break;
	
	/* insque */
        pp = pp->p_rlink;
        p->p_link = pp->p_link;
        p->p_rlink = pp;
        pp->p_link->p_rlink = p;
        pp->p_link = p;
	/* insque */
	whichqs |= p->p_affinity; 

}

remrq(p)
register struct proc *p;
{
	/* remque */
	p->p_rlink->p_link = p->p_link;
	p->p_link->p_rlink = p->p_rlink;
	/* remque */

	p->p_rlink = 0;

	/* if queue is now empty then clear out so no other processor 
	   will try to get rq */
	if (qs.ph_link == (struct proc *) &qs) {
		whichqs = 0;
	}
}

/*
 * TODO: recorded for post-mortems?
 */
struct pte	*masterpaddr;
int		cur_tlbpid;			/* tlb pid of current proc */
int		cur_pid;			/* proc pid of current proc */


/*
 * Assign the next available tlbpid to process p.
 * Un-assign all tlbpids and start over if we run out.
 * Remember this process so that we can un-assign tlbpids the next time
 * we run out.
 */
init_tlbpid()
{
	register int i;
	register struct cpudata *pcpu;

	pcpu = CURRENT_CPUDATA;
	pcpu->cpu_next_tlbpid = 0;
	for (i=0; i < TLBHI_NPID; i++) {
		pcpu->cpu_tps[i].tps_procpid = -1;
	}
}


get_tlbpid(p)
 struct proc *p;
{
	register int i;
	register int s;
	register struct cpudata *pcpu;

	s = splextreme();
	pcpu = CURRENT_CPUDATA;
	if (pcpu->cpu_next_tlbpid == TLBHI_NPID) {
		/*
		 * tlbpid overflow
		 */
		 if (smp) {
			for (i=0; i < TLBHI_NPID; i++) {
				   pcpu->cpu_tps[i].tps_procpid = -1;
			}
		} else {
			for (i=0; i < TLBHI_NPID; i++) {
				if(pcpu->cpu_tps[i].tps_procpid != -1){
				   pcpu->cpu_tps[i].tps_owner->p_tlbpid = -1;
				   pcpu->cpu_tps[i].tps_procpid = -1;
				}
			}
		}
		flush_tlb();
		pcpu->cpu_next_tlbpid = 0;
	}
	p->p_tlbpid = pcpu->cpu_next_tlbpid;
#ifdef notdef
	if (p == u.u_procp)
		set_tlbpid(p->p_tlbpid);	/* should re-assign pids */
						/* in the hardwired entries */
#endif notdef
	pcpu->cpu_tps[pcpu->cpu_next_tlbpid].tps_owner = p;
	pcpu->cpu_tps[pcpu->cpu_next_tlbpid].tps_procpid = p->p_pid;
	pcpu->cpu_next_tlbpid += 1;
	cnt.v_tlbpid++;  /* ?? */
	splx(s);
}

release_tlbpid(p)
 register struct proc *p;
{
	register int i;
	register int s;
	register struct cpudata *pcpu;

	s = splextreme(); 

	pcpu = CURRENT_CPUDATA;

	if (pcpu->cpu_tps[p->p_tlbpid].tps_procpid == -1)
		panic("release_tlbpid: not inuse");
	if (pcpu->cpu_tps[p->p_tlbpid].tps_owner != p) 
		panic("release_tlbpid: not owner");


	pcpu->cpu_tps[p->p_tlbpid].tps_procpid = -1;
	if (!smp) p->p_tlbpid = -1;
	(void) splx(s);
}

#ifdef TODO
/*
 * Do we want to bother swapping pids for vfork?
 */
swap_tlbpid(p, q)
 register struct proc *p, *q;
{
	register int tmp;

	if (p->p_tlbpid == -1) {
		if (q->p_tlbpid == -1)
			panic("swap_tlbpid no valid pids");
		if (tps[q->p_tlbpid].tps_procpid == -1)
			panic("swap_tlbpid q not inuse");
		if (tps[q->p_tlbpid].tps_owner != q)
			panic("swap_tlbpid q not owner");
		tps[q->p_tlbpid].tps_owner = p;
	} else {
		if (q->p_tlbpid != -1)
			panic("swap_tlbpid both pids valid");
		if (tps[p->p_tlbpid].tps_procpid == -1)
			panic("swap_tlbpid p not inuse");
		if (tps[p->p_tlbpid].tps_owner != p)
			panic("swap_tlbpid p not owner");
		tps[p->p_tlbpid].tps_owner = q;
	}
	tmp = q->p_tlbpid;
	q->p_tlbpid = p->p_tlbpid;
	p->p_tlbpid = tmp;
}
#endif


extern int smp;
extern struct proc *next_proc();
swtch()
{
	register struct proc *p;
	register int s;
	register struct cpudata *pcpu;
	pcpu = CURRENT_CPUDATA;

	/*
	 * Pass GO, Collect $200
	 */
	astoff();

	/* Save process context only if there is an active  process associated
	   with the processor. exit() sets cpu_noproc to 1 and calls 
	   release_uarea_noreturn() which calls swtch().
	   So, the following check to see if the processor has a process
	   associated with it. */

				    
	if(pcpu->cpu_noproc == 0 ) {   

		if (save()) {
			smp_unlock(&lk_rq);
			/* if the last exiting process has to be relesed in
			   this process's context */
			if (CURRENT_CPUDATA->cpu_exitproc) {
				vrelu(CURRENT_CPUDATA->cpu_exitproc,0);
				vrelpt(CURRENT_CPUDATA->cpu_exitproc);
				proc_exit(CURRENT_CPUDATA->cpu_exitproc);
				CURRENT_CPUDATA->cpu_exitproc = 0; 
			}

			if (CURRENT_CPUDATA->cpu_fpowner == u.u_procp)
				USER_REG(EF_SR) |= SR_CU1;
			else
				USER_REG(EF_SR) &= ~SR_CU1;

			return;
		
		}
		CURRENT_CPUDATA->cpu_noproc = 1;
	}
	if (smp) { 
		if ((pcpu->cpu_stops) || (pcpu->cpu_state & CPU_SIGPARENT))  
			/* if the processor is in a stop state or if 
			   a signal to be sent to the parent */
			start_idleproc();  /* does not return */
	} else	{
		 if  (pcpu->cpu_state & CPU_SIGPARENT) {
			sig_parent(pcpu->cpu_proc);
			pcpu->cpu_state &= ~CPU_SIGPARENT;
		}
	}

	for (p = qs.ph_link; p != (struct proc *)&qs; p = p->p_link) {
		if((!smp) || (p->p_affinity & pcpu->cpu_mask)) {
			
			p->p_rlink->p_link = p->p_link;	/* remque*/
			p->p_link->p_rlink = p->p_rlink;
			p->p_rlink = 0;

			/* if queue is now empty then clear out so no other 
			   processor will try to get rq */
			if (qs.ph_link == (struct proc *) &qs) {
				whichqs = 0;
			}

			if (p->p_wchan || p->p_stat != SRUN) {
				panic("start_proc");
			}
			
			if (smp) {
				if(p->p_cpumask != CURRENT_CPUDATA->cpu_mask) { 
					/* if the process is migrating to another processor */
					get_tlbpid(p);
					pcpu->cpu_tlbcount++;
				} else if (pcpu->cpu_tps[p->p_tlbpid].tps_procpid != p->p_pid) {  
					/* if the tlbpid is invalid on this processor */
					get_tlbpid(p);
				}
				if(pcpu->cpu_tbi_flag) {  /* if TBI needed */
					flush_tlb();
					pcpu->cpu_tbi_flag = 0;
        			} else {
					unmap_systlbs(p);
				}
				p->p_cpumask = pcpu->cpu_mask;
			} else {
				if(p->p_tlbpid == -1) /* for any new process */
					get_tlbpid(p);
		
			}
			pcpu->cpu_paddr = p->p_addr;
			pcpu->cpu_proc = p;
			pcpu->cpu_runrun = 0;
			pcpu->cpu_noproc = 0;
			pcpu->cpu_roundrobin = 25;
			pcpu->cpu_switch++;
			resume(pcbb(p));	/* pcbb should return p */
		}
	}
	start_idleproc();  /* does not return */
}

	
start_idleproc()
{
	register struct proc *p;
	register struct cpudata *pcpu;

	pcpu = CURRENT_CPUDATA;
	if(pcpu->cpu_idleproc == NULL) {
		if(!BOOT_CPU)
			panic("no idle proc set up");
		smp_unlock(&lk_rq); /* since we continue on the same process
				       context, lk_rq will never get unlocked*/
		idle(); 
        }
	if ((pcpu->cpu_idleproc->p_tlbpid == -1) || (pcpu->cpu_tps[pcpu->cpu_idleproc->p_tlbpid].tps_procpid != pcpu->cpu_idleproc->p_pid)) 
		get_tlbpid(pcpu->cpu_idleproc);
	map_to_idleproc(pcpu->cpu_idleproc);
	if(save()) {   /* if idle process running */ 
		/* should be at splhigh() here since spl never lowered since
		   entering swtch for idle process */
		if(pcpu->cpu_state & CPU_SIGPARENT) {
			sig_parent(pcpu->cpu_proc);
			pcpu->cpu_state &= ~CPU_SIGPARENT;
		}
		else  /* sig_parent unlocks lk_rq */
			smp_unlock(&lk_rq); /* release lk_rq only now in the 
					       idle process's context   */
		if (pcpu->cpu_exitproc) {
			vrelu(pcpu->cpu_exitproc,0);
			vrelpt(pcpu->cpu_exitproc);
			proc_exit(pcpu->cpu_exitproc);
			pcpu->cpu_exitproc = 0;
		}
		pcpu->cpu_runrun = 0;
		spl0();
		idle();
	}
	/* start the idle process */
	resume(pcbb(CURRENT_CPUDATA->cpu_idleproc)); 
}

start_proc(p)
struct proc *p;
{
	register struct cpudata *pcpu;

	pcpu = CURRENT_CPUDATA;

	p->p_rlink->p_link = p->p_link;
	p->p_link->p_rlink = p->p_rlink;
	p->p_rlink = 0;

	/* if queue is now empty then clear out so no other processor 
	   will try to get rq */
	if (qs.ph_link == (struct proc *) &qs) {
		whichqs = 0;
	}

	if (p->p_wchan || p->p_stat != SRUN) {
		panic("start_proc");
	}

	if (smp) {
		if(p->p_cpumask != CURRENT_CPUDATA->cpu_mask) { 
			/* if the process is migrating to another processor */
			get_tlbpid(p);
			pcpu->cpu_tlbcount++;
		}
		else if (pcpu->cpu_tps[p->p_tlbpid].tps_procpid != p->p_pid)  
			/* if the tlbpid is invalid on this processor */
			get_tlbpid(p);

		if(pcpu->cpu_tbi_flag) {  /* if TBI needed */
			flush_tlb();
			pcpu->cpu_tbi_flag = 0;
        	} else if(smp)
			unmap_systlbs(p);
		p->p_cpumask = pcpu->cpu_mask;
	} else {
		if(p->p_tlbpid == -1) /* for any new process */
			get_tlbpid(p);
	}
	pcpu->cpu_paddr = p->p_addr;
	pcpu->cpu_proc = p;
	pcpu->cpu_runrun = 0;
	pcpu->cpu_noproc = 0;
	pcpu->cpu_roundrobin = 25;
	pcpu->cpu_switch++;
	resume(pcbb(p));	/* pcbb should return p */

}
/* A secondary processor after doing the initialization, should jump to this
 * routine, waiting for any work to do */
idle()
{
	register struct proc *p;
	register struct cpudata *pcpu;

	pcpu = CURRENT_CPUDATA;
top:

	splclock(); /* whatever spl IPI comes in ?? (ISIS does at IPL 16*/
	while (pcpu->cpu_stops){   /* if the cpu is in a stop state */
		if(!(pcpu->cpu_state & CPU_STOP)) { /* if not 
					already stopped */
		  	if(pcpu->cpu_stops&IPIMSK_STOP) { /* if IPI pending 
							     for stop cpu */
				timeout_affinity_fix();
				if(pcpu->cpu_fpowner) /* save FPU registers */
					checkfp(pcpu->cpu_fpowner,0);
			}
			smp_lock(&lk_rq,LK_RETRY); /* make state changes 
						      visible to other cpus*/
			pcpu->cpu_state &= ~CPU_RUN;
			pcpu->cpu_state |= CPU_STOP;
			smp_unlock(&lk_rq);
		}
		/* since the stopped cpu remains at high IPL, check for
		 * any pending IPI this way. cpu_stops will get reset
		 * through cpu_ip_intr() */
		if(pcpu->cpu_int_req) {
			cpu_ip_intr();
			flush_tlb(); /* flush the tlbs if the pager changed
					the translations */
		}			
	}
	spl0();
	if(!(whichqs & pcpu->cpu_mask)) {
		goto top;
        }
	splclock();
	if (!smp_lock(&lk_rq,LK_ONCE)) {
		spl0();
		goto top;
	}
	for (p = qs.ph_link; p != (struct proc *)&qs; p = p->p_link) 
		if(!smp || (p->p_affinity & pcpu->cpu_mask)) 
			break;

	if(p == (struct proc *)&qs) { /* if no process for this cpu, go back to
					idle loop. */ 
		whichqs &= ~pcpu->cpu_mask; /* whichqs gets
				updated only with lk_rq  held (in swtch() here
				and in setrq().*/
		smp_unlock(&lk_rq);
		spl0();
		goto top;
        }
	start_proc(p);
}

/* unmap all the page table ptes for the process on the new processor */

unmap_systlbs(p)
struct proc *p;
{
	register int i;
	for (i=p->p_textpt; i > 0; i--)
		unmaptlb(0,(btop(p->p_textbr)+(i-1)));
	for (i=p->p_datapt; i > 0; i--)
		unmaptlb(0,(btop(p->p_databr)+(i-1))); 
	for (i=p->p_stakpt; i > 0; i--)
		unmaptlb(0,(btop(p->p_stakbr)+(i-1)));
}


/*
 * generic list struct for use with _insque/_remque
 * any structure that is linked via _insque/_remque should have the
 * list pointers as the first two elements
 */
struct generic_list {
	struct generic_list *link;	/* forward link */
	struct generic_list *rlink;	/* backward link */
};

_insque(ep, pp)
register struct generic_list *ep, *pp;
{
	ep->link = pp->link;
	ep->rlink = pp;
	pp->link->rlink = ep;
	pp->link = ep;
}

_remque(ep)
register struct generic_list *ep;
{
	ep->rlink->link = ep->link;
	ep->link->rlink = ep->rlink;
}
