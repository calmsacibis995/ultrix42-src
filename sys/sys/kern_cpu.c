#ifndef lint
static	char	*sccsid = "@(#)kern_cpu.c	4.5	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * Modification History: /sys/sys/kern_cpu.c
 *
 * 11-Apr-91	dlh
 * 	startcpu():
 * 		Need to account for the case where startcpu is called
 * 		interactively.  In this case the vpmask, vpfree and
 * 		vptotal variables may need to be updated.
 * 	
 * 	stopcpu():
 * 		Need to account for the case where stopcpu is called
 * 		interactively.  In this case, the vpmask, vpfree and
 * 		vptotal variables may need to be updated.  Also, print
 * 		out a warning (to the console and the error logger) if
 * 		one or more of the CPUs being stopped has an attached
 * 		VP and the boot CPU does not have a VP.  This will
 * 		cause vector processes to 'hang'.  Processes 'hung'
 * 		this way will be re-scheduled when the vector capable
 * 		secondary CPU is re-started.
 *
 * 18-Dec-90 Jim Paradis
 *	Idle vector processor before stopping CPU (VAX only)
 *
 *	DATE -- dlh
 *	added vector processor support code
 * 	17-Jul-90 -- jaw
 *	fix for slave hold hang.
 *
 * 	16-Apr-90 -- jaw/gmm
 *	move kstackflg to cpudata structure.
 *
 *	14-Feb-90 jaw
 *	missing spl call in subr_cprintf.
 *
 *	11-Dec-89 jaw
 *	Add ISIS icache flush code.
 *
 *	08-Dec-89 gmm
 *	Changed mtpr() to aston() to be portable to all architectures. 
 *
 *	08-Dec-89 jaw
 *	Fix for cprintf's crashing firefox.  Reschedule printf with 
 *	timeout, so driver is not called from high priority interrupt.
 *
 *	14-Nov-89  gmm
 *	Move idle process startup from startcpu() to mips specific machdep.c.
 *	Cleaned up the code and logic for mips secondary processor startup.
 *
 *	09-Nov-89  jaw
 *	change references from maxcpu to smp.	also change to timeout.
 *
 *	13-Oct-89  gmm
 *	SMP support for MIPS. New logic for startcpu() for mips
 *
 *	11-Oct-89  jaw
 *	pick of PTE invalidating code for kmalloc bucket scrubber.
 *
 *	24-Jul-89 map (Mark A. Parenti)
 *	Include kdb.h if KDEBUG flag is defined.
 *
 * 	20-Jul-89 jaw
 *	move some non-vax specific code for ip interrupts to sys area.
 *
 *	08-Jun-89	gmm
 *	Initialize cpu_noproc to 0 for the boot cpu in init_boot_cpu(). This
 *	helps to avoid saving process context on exit of the process in
 *	swtch().
 *
 *	24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 *	23 May 89  -- darrell
 *	Changed startcpu and stopcpu to use the new cpusw.  Added cpusw
 *	to MIPS pool, and removed the "#ifdef vax" around the cpusw
 *	accesses.
 *
 *	11 may 89  -- jaw
 *	valid cpu number must be checked against maxcpu instead of MAXCPU.
 *
 *	 8 May 89  -- Giles Atkinson
 *	Add call to lmf_start_cpu()
 *
 *	Apr-14-89  -- gmm
 *	Changes to startcpu() and stopcpu() to behave like regular system
 *	calls for error numbers and return values
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/cpudata.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/cpu.h"
#include "../h/kmalloc.h"
#include "../h/conf.h"
#ifdef vax
#include "../machine/mtpr.h"
#ifdef KDEBUG
#include "../machine/kdb/kdb.h"
#endif KDEBUG
extern int cold;
#endif vax
#ifdef mips
#include "../h/vmmac.h"
#include "../machine/pte.h"
#endif mips
#ifdef	vax
#include "../machine/vectors.h"
#endif


extern int cpu;
extern struct cpusw *cpup;	/* pointer to cpusw entry */

startcpu() {
	register struct a {
		int cpu_num;
	} *uap = (struct a *)u.u_ap;

	int saveaffinity;
#if defined(__vax)
	struct vpdata *vpd;
#endif /* __vax */

	/* range check cpunum */
	if (suser()) {

		if (smp && (uap->cpu_num < MAXCPU) && (uap->cpu_num >= 0)) {
			saveaffinity = switch_affinity(boot_cpu_mask);
			start_one_cpu(uap->cpu_num); 
	 		(void) switch_affinity(saveaffinity);
			if (u.u_error == 0) {		/* Success */
			    lmf_start_cpu();
#if defined(__vax)
			    vpd = CPUDATA(uap->cpu_num)->cpu_vpdata;
			    if (vpd && vpd->vpd_state) {
				/* if this routine is called as part of system 
				 * startup, then this field will still be set 
				 * to VPD_ABSENT (0), and the following code 
				 * will not be executed.  But, if this routine 
				 * is called as the result of a startcpu 
				 * system call, and there is an attached 
				 * vector processor, then the vpd_state will 
				 * be set to a non-zero value.  If there is an 
				 * attached vector processor, then the VP 
				 * variables need to be updated.
				 */
				/* don't just inc vptotal:  in some cases 
				   startcpu seems to be called more than 
				   once on Mariah
				JIM ?? does this test of vpmask need to be
					smp locked?
				 */
				if ( ! (vpmask & 
					CPUDATA(uap->cpu_num)->cpu_mask) ) {
					adawi (1, &vptotal);
				}
					
				set_bit_atomic (uap->cpu_num, &vpmask);
				set_bit_atomic (uap->cpu_num, &vpfree);
			    }
#endif /* __vax */
			}
		} else {
		  		u.u_error = EINVAL;
		}
	}
}

start_one_cpu(cpunum) 
int cpunum;
{
	struct cpudata *pcpu;
	int ret_val;
	int timeout;
	
	if (pcpu =CPUDATA(cpunum)){
		/* check if already running */
		if (pcpu->cpu_state & CPU_RUN) { 
		  	u.u_error = EBUSY;
		  	return;
		}

		/* if cpu stopped then try to start it */
		if (pcpu->cpu_state & CPU_STOP) {
			intrpt_cpu(cpunum,IPI_START);
		}	

		/* wait up to 1/10 of a second for it to declare it 
		   is in the run state */	
		timeout=10;
		while(timeout-->0) {
			DELAY(10000); /* 1/100 of sec */
			/* if running then success */
			if (pcpu->cpu_state&CPU_RUN) return;
		}	
	} 

	/* cpu must be in console mode or might not be present at all.  
	   Use the cpu dependent start routine to attempt to start it.
	   The machdep routine will return 1 if success and 0 if failure */

	if ( (ret_val =(*cpup->startcpu)(cpunum)) < 0 )
		panic("No start cpu routine configured\n");
	if(ret_val == 0) 
	  	u.u_error = ENODEV;
	return;
}

stop_cpu_now() {
	spl7();
	/* if we have paniced the system then don't change
	   any state. */
	if (panicstr == 0){
		/* BUG... need to check run queue */
		
		/* can't leave any thing on the timeout queue that can
		   only be run on the cpu that is being stopped. */
		timeout_affinity_fix();
	}
	if ((*cpup->stopcpu)() < 0 )
		panic("No stop cpu routine configured\n");
}
stopcpu() {
	register struct a {
		int cpu_num;
	} *uap = (struct a *)u.u_ap;

	register int cpu_num;

	if (suser()) {
		cpu_num = uap->cpu_num;
		if (cpu_num == boot_cpu_num) {
		  	u.u_error = EACCES;
			return;
		}
		if ((cpu_num >= MAXCPU) || (cpu_num < 0)) {
		  	u.u_error = EINVAL;
			return;
		}
		if (CPUDATA(cpu_num) && (CPUDATA(cpu_num)->cpu_state 
				& CPU_RUN)) {
#if defined(__vax)
			if (CPUDATA(cpu_num)->cpu_mask & vpmask) {
				/* this CPU has an attached vector processor; 
				 * therefore the VP variables need to be 
				 * updated
				 */
				unsigned clear_mask;
				clear_mask = ~(CPUDATA(cpu_num)->cpu_mask);
				clear_bit_atomic (cpu_num, &vpmask);
				clear_bit_atomic (cpu_num, &vpfree);
				adawi ((-1), &vptotal);
				if (vptotal == 0) {
				    printf ("stopping last vector processor\n");
				}
			}
#endif /* __vax */
			intrpt_cpu(cpu_num,IPI_STOP);
		} else
		  	u.u_error = EBUSY;
	}

}

get_cpudata(cpu_num)
int cpu_num;
{
	struct cpudata *pcpu;
	char *istack;

	if ( CPUDATA(cpu_num) == 0 ) {

#ifdef mips
		if(init_idleproc(cpu_num) == 0)
			return(0);
#endif mips
		if (cpu_num > highcpu) highcpu = cpu_num;
		if (cpu_num < lowcpu) lowcpu = cpu_num;

		KM_ALLOC(pcpu, struct cpudata  *, sizeof(struct cpudata),
			 KM_DEVBUF,KM_CLEAR|KM_CALL);
		if(pcpu == NULL)
			panic("getcpudata: KM_ALLOC could not allocate cpudata");			
			
		pcpu->cpu_num = cpu_num;
		pcpu->cpu_mask = 1<< pcpu->cpu_num;
 		pcpu->cpu_noproc = 1;
		pcpu->cpu_l_ctrace = &pcpu->cpu_l_trace[CPU_LK_TRACES-1];
		pcpu->cpu_bufin = 0;
		pcpu->cpu_bufout = 0;

		KM_ALLOC(pcpu->cpu_buf, char  *, CPUDATA_BUFSIZE,
			 KM_DEVBUF,KM_CLEAR|KM_CALL);
		
#ifndef mips
		KM_ALLOC(istack,char  *, NISP*512,
			 KM_DEVBUF,KM_CLEAR|KM_CALL);

		pcpu->cpu_istack = istack + (NISP*512);
		cpudata[pcpu->cpu_num] = pcpu;
#else
		/* WARNING: Assumes that the cpudata gets allocated within the
		   first 256K memory. We want the K0 address since there 
		   should be no TLB miss when accessing the cpudata structure 
		   in the exception path*/

		cpudata[pcpu->cpu_num] = (struct cpudata *)PHYS_TO_K0(svtophy(pcpu));
		pcpu->cpu_newpc = (caddr_t)1;   /* cpu_newpc is used in VAX in 
				  longjmp. In mips, this field is being used to
				  flag if running on kernel stack or not */
#endif mips

		return(1); /* success, MIPS uses the return value */
	}
	return(0);
}


extern int nblkdev;
extern int nchrdev;
#ifdef vax
extern char eintstack; /* base of interrupt stack on vax */
#endif vax

#ifdef mips
char	boot_idle_uarea[NBPG*2]; /* 2 pages created to get page alighment */
struct	user *boot_idle_up;
#endif mips

init_boot_cpu() {

	struct cpudata *pcpu;
	int j;
	struct bdevsw *bdevp;
	struct cdevsw *cdevp;

#ifdef mips
	boot_idle_up = (struct user *)((int)boot_idle_uarea+PGOFSET & ~PGOFSET);
	tlbwired(TLBWIREDBASE, 0, UADDR,
	 K0_TO_PHYS(boot_idle_up)>>(PGSHIFT-PTE_PFNSHIFT) | PG_M | PG_V | PG_G);
	u.u_pcb.pcb_cpuptr = &boot_cpudata;  /* put the 
			  cpudata pointer in the  dummy u of boot cpu */
#endif mips
	cpudata[cpuident()] = &boot_cpudata;	
	pcpu = &boot_cpudata;	
#ifdef mips
	pcpu->cpu_newpc = (caddr_t)1;   /* cpu_newpc is used in VAX in longjmp.
				  In mips, this field is being used to flag if
				  running on kernel stack or not */
#endif mips
	pcpu->cpu_num = cpuident();
	pcpu->cpu_mask = 1<<pcpu->cpu_num;
	pcpu->cpu_l_ctrace = &pcpu->cpu_l_trace[CPU_LK_TRACES-1];
	pcpu->cpu_noproc = 1;
	pcpu->cpu_state = CPU_RUN|CPU_BOOT;
	pcpu->cpu_bufin = 0;
	pcpu->cpu_bufout = 0;

#ifdef vax
	mtpr(ESP,&boot_cpudata);
#endif vax
	boot_cpu_num = pcpu->cpu_num;
	boot_cpu_mask = 1<<boot_cpu_num;
#ifdef vax
	pcpu->cpu_istack = &eintstack;
#endif vax

	cpu_avail = 1;	/* one processor now available */
	lowcpu = boot_cpu_num;
	highcpu = boot_cpu_num;

	lockinit(&lk_printf, &lock_printf_d);

	/* set-up affinity mask for devices that are not already set */
	for(j=0; j<nblkdev; j++)
		if (bdevsw[j].d_affinity == 0)
			bdevsw[j].d_affinity = boot_cpu_mask;

	for(j=0; j<nchrdev; j++)
		if (cdevsw[j].d_affinity == 0)
			cdevsw[j].d_affinity = boot_cpu_mask;

	/* allow for lock tracing */
	smp_enable_trace = 1;

	/* Initialize SMP locks */
	kern_lock_init();

}

int cprintf_sched=0;

/*
 * Routine subr_cprintf prints out all characters posted from secondary 
 * cpu's and prints them to the console.  This routine is scheduled by
 * the boot cpu in the interprocessor interrupt routine.  Multiple calls
 * are pervented by the global "cprintf_sched".  This variable is clear
 * only after all characters are printed.
 */

subr_cprintf()
{
	register int s,i;
	register struct cpudata *pcpu;
	struct cpudata *char_to_print();

	s=spl7();	
	smp_lock(&lk_printf,LK_RETRY);

	/* check for characters ..if none exit */
	while (pcpu = char_to_print()) {
		smp_unlock(&lk_printf);
		splx(s);
		/* print out all character for this cpu */
		while(pcpu->cpu_bufin != pcpu->cpu_bufout) {
			cnputc(pcpu->cpu_buf[pcpu->cpu_bufout]);
			s=spl7();
			smp_lock(&lk_printf, LK_RETRY);
			/* check for wrap around in buffer */
			if (pcpu->cpu_bufout == (CPUDATA_BUFSIZE-1))
				pcpu->cpu_bufout = 0;
			else
				pcpu->cpu_bufout++;
			smp_unlock(&lk_printf);
			splx(s);
		}
		s=spl7();
		smp_lock(&lk_printf,LK_RETRY);		
	}
	/* clear sched flag */
	cprintf_sched=0;
	smp_unlock(&lk_printf);
	splx(s);
}

struct cpudata *char_to_print() {
	register int i;
	struct cpudata *pcpu;

	for (i=1; i< MAXCPU; i++) {
		pcpu=CPUDATA(i);
		if (pcpu == 0) continue;
		if (pcpu->cpu_bufin != pcpu->cpu_bufout)
			return(pcpu);
	}
	return(0);
}

cpu_ip_intr() 
{
	register int i,s;
	register struct cpudata *pcpu, *current_cpu;
	char out_char;
	unsigned int ipi_mask;
/* NOTE: IP interrupts are cause by "cpu_int_req" being sent or
         by console communication (ie: CCA on VAX6200). */


	current_cpu = CURRENT_CPUDATA;
	current_cpu->cpu_ip_intr++;
	ipi_mask = current_cpu->cpu_int_req;

	if(ipi_mask & IPIMSK_PANIC) {
	        clear_bit_atomic(IPI_PANIC,(caddr_t)&current_cpu->cpu_int_req);
		if(BOOT_CPU)
			panic("secondary cpu requested");
		else {
			(void)spl7();
#if defined(__vax)
			/* If this is a vector-capable CPU, then wait
			 * for the attached vector processor to go idle.
			 * This is a no-op on non-vector-capable VAXen
			 */
			vp_idle();
#endif /* __vax */
			stop_secondary_cpu();
			/* no return */
		}
	}
	if(ipi_mask & IPIMSK_PRINT) {
	        clear_bit_atomic(IPI_PRINT,(caddr_t)&current_cpu->cpu_int_req);
		if(BOOT_CPU) {
			if (!cprintf_sched) { 
				cprintf_sched++;
				timeout(subr_cprintf,0,0);
			}
		}
	}

	if(ipi_mask & IPIMSK_SCHED) {  /* reschedule the process */
	        clear_bit_atomic(IPI_SCHED,(caddr_t)&current_cpu->cpu_int_req);
		current_cpu->cpu_runrun++;
		aston();
	}

	if (ipi_mask & IPIMSK_KMEMTBFL) { /* Flush kmem dead pte(s) */
	        clear_bit_atomic(IPI_KMEMTBFL,
			(caddr_t)&current_cpu->cpu_int_req);
		(void) km_scan_pte();
	}

	if(ipi_mask & IPIMSK_TBFLUSH) {  /* TB flush request */
	        clear_bit_atomic(IPI_TBFLUSH,(caddr_t)&current_cpu->cpu_int_req);
#ifdef vax
		/* Quiesce the vector processor, if necessary */
		VPSYNC ();
		mtpr(TBIA,0);
#endif vax
	}

	if(ipi_mask & IPIMSK_CPUHOLD) {  /* TB flush request */
		s=splhigh();
	        smp_lock(&lk_rq,LK_RETRY);
		if (slavehold)
			hold_cpu(IPI_CPUHOLD);
		else	free_cpu(IPI_CPUHOLD);
	        smp_unlock(&lk_rq);
		splx(s);
	}
	if(ipi_mask & IPIMSK_STOP) {  /* stop secondary cpu */
		s=splhigh();
	        smp_lock(&lk_rq,LK_RETRY);
		hold_cpu(IPI_STOP);
	        smp_unlock(&lk_rq);
		splx(s);
	}

	if(ipi_mask & IPIMSK_START) {  /* stop secondary cpu */
		s=splhigh();
	        smp_lock(&lk_rq,LK_RETRY);
	        clear_bit_atomic(IPI_START,(caddr_t)&current_cpu->cpu_int_req);
		free_cpu(IPI_STOP);
	        smp_unlock(&lk_rq);
		splx(s);
	}

#ifdef vax
#ifdef KDEBUG
	if(ipi_mask & IPIMSK_KDB) {
	    if(!BOOT_CPU) {
		s=splhigh();
	        smp_lock(&lk_rq,LK_RETRY);
		if(kdb_intr_req == KDB_ENTER) 
			hold_cpu(IPI_KDB);
		else if(kdb_intr_req == KDB_LEAVE) 
			free_cpu(IPI_KDB);
	        smp_unlock(&lk_rq);
		splx(s);
	    }
	}
#endif KDEBUG

	if ((cpu == VAX_6200) || (cpu == VAX_6400))
		if ((BOOT_CPU) && (cold==0) ) cca_check_input();
#endif vax
#ifdef mips

	if (ipi_mask & IPIMSK_ICACHE) { /* flush icache*/
		clear_bit_atomic(IPI_ICACHE,(caddr_t)&current_cpu->cpu_int_req);
		consume_icache_clears();
	}
#endif mips

}

#ifdef mips
produce_icache_clears(pf)
	int pf;
{
	int index;
	unsigned mask;
	int cpident = CURRENT_CPUDATA->cpu_num;
	int cpindex;
	struct cpudata *pcpu;

        pf = pf % (MAXCACHE/NBPG);      
        index = pf/(NBBY*NBPW);
        mask = pf%(NBBY*NBPW);


	for (cpindex = lowcpu; cpindex <= highcpu; cpindex++)
		if (cpindex != cpident && (pcpu=CPUDATA(cpindex)))

			set_bit_atomic(mask,&pcpu->cpu_ic_dirt[index]);

	for (cpindex = lowcpu; cpindex <= highcpu; cpindex++)
		if (cpindex != cpident && (pcpu=CPUDATA(cpindex)))
			intrpt_cpu(cpindex, IPI_ICACHE);
}

consume_icache_clears() /* at a low interrupt level */
{
	int i, pf;
	unsigned *local = &CURRENT_CPUDATA->cpu_ic_dirt[0];

	for (pf=0, i=0; i<ICACHE_PG_MAP_SZ; i++, pf+=NBBY*NBPW) {
		int j;
		unsigned mask;

		if ((mask=local[i]) == 0)
			continue;
		for (j=0; j<NBBY*NBPW; j++) {


			if (mask & (1<<j)) {
				clean_icache(PHYS_TO_K0(ptob(pf+j)), NBPG);
				clear_bit_atomic(j,&local[i]);
			}
			/*
			 * We could be interrupted here, and this bit could
			 * then be set from the global to the local mask
			 * just before we clear it.  It doesn't matter as
			 * we did not run in user mode the whole time.
			 */
		}
	}
	/* Lower IPL */
}

#endif mips

/*
 * called out of intrcpu on behalf of self.
 * real purpose is to minimize the number of
 * items to be checked in switch for stopping
 */
hold_cpu(cause)
	unsigned int cause;
{
	register struct cpudata *current_cpu;
	int mask = 1 << cause;
	current_cpu = CURRENT_CPUDATA;

	if (mask & (IPIMSK_KDB | IPIMSK_CPUHOLD | IPIMSK_STOP ) == 0)
		panic("hold_cpu: invalid cause");
	if (BOOT_CPU)
		panic("hold_cpu: on primary");

#ifdef vax
	/* Quiesce the vector processor (if any) */
	VPSYNC();
#endif vax

	clear_bit_atomic(cause,(caddr_t)&current_cpu->cpu_int_req);
	current_cpu->cpu_stops |= mask;
	current_cpu->cpu_runrun++;
	aston();
}

/*
 * called on behalf of self
 */
free_cpu(cause)
	unsigned int cause;
{
	register struct cpudata *current_cpu;
	int mask = 1 << cause;

	current_cpu = CURRENT_CPUDATA;

	if (mask & (IPIMSK_KDB | IPIMSK_CPUHOLD | IPIMSK_STOP ) == 0)
		panic("free_cpu: invalid cause");

	clear_bit_atomic(cause,(caddr_t)&current_cpu->cpu_int_req);
	current_cpu->cpu_stops &= ~mask;

	/* if no reason to stay stopped, then don't */
	if (current_cpu->cpu_stops == 0) {	
		/* take out rq lock when changing states */
		current_cpu->cpu_state &= ~CPU_STOP;
		current_cpu->cpu_state |= CPU_RUN;
	}
}



intrpt_cpu(cpu_no,int_req)
{
	struct cpudata *pcpu;

	pcpu = CPUDATA(cpu_no);
	if (pcpu == 0 ) panic("intrpt_cpu: invalid cpu");
	set_bit_atomic(int_req,(caddr_t)&pcpu->cpu_int_req);
	intrcpu(cpu_no);
	return(0);
}
