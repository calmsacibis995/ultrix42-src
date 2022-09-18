#ifndef lint
static char *sccsid = "@(#)kn02ba.c	4.8      (ULTRIX)  3/7/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Modification History:
 *
 * 7-Mar-91	Randall Brown
 *	Fixed a bug in isolatepar().  Print out more information about
 *	which CPU card is in the system.
 *
 * 7-Mar-91	Jaw
 *	3min spl optimization.
 *
 * 21-Jan-91	Randall Brown 
 *	Modified kn02ba_isolate_memerr() to use tc_memerr_status struct.
 *	Modified kn02ba_isolate_par() to take the blocksize as
 *	a parameter.
 *
 * 06-Dec-90	Randall Brown
 *	Added isolate_memerr() routine for logging of memory errors
 *	that occur during I/O.
 *
 * 15-Oct-90	Randall Brown
 *	Added error handling code.
 *
 * 06-Sep-90 	Randall Brown 
 *	Changed slot_order to config_order.
 *
 * 23-Feb-90	Randall Brown
 *	Created file for support of 3MIN (DS5000_100).
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/types.h"
#include "../h/cpudata.h"
#include "../machine/cpu.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/errlog.h"
#include "../h/cmap.h"
#include "../machine/pte.h"
#include "../../machine/common/cpuconf.h"
#include "../io/tc/tc.h"
#include "../machine/reg.h"
#include "../machine/kn02ba.h"
#include "../machine/mc146818clock.h"

extern 	softclock(), softnet(), hardclock(), fpuintr();

int	kn02ba_stray(), kn02ba_sysintr(), kn02ba_halt();
int	kn02ba_enable_option(), kn02ba_disable_option();
int	kn02ba_clear_errors(), kn02ba_isolate_memerr();

extern u_int printstate;	/* how to print to the console */
extern char *rt_clock_addr;	/* addr of the mc146818clock chip */
extern struct timeval time;	/* the system time */
extern int hz;
extern int tick;
extern int tickadj;
extern int fixtick;		/* set to the number of microsec's that the
				   clock looses per second */


/* The SR reg masks for splxxx usage */
int kn02ba_splm[SPLMSIZE] = {
	KN02BA_SR_IMASK0,		/* 0 SPLNONE			*/
	KN02BA_SR_IMASK1,		/* 1 SPLSOFTC			*/
	KN02BA_SR_IMASK2,		/* 2 SPLNET			*/
	0,				/* 3 NOT_USED 			*/
	0,				/* 4 NOT_USED 			*/
	KN02BA_SR_IMASK5,		/* 5 SPLBIO, SPLIMP, SPLTTY	*/
	KN02BA_SR_IMASK6,		/* 6 SPLCLOCK			*/
	KN02BA_SR_IMASK7,		/* 7 SPLMEM			*/
	KN02BA_SR_IMASK8,		/* 8 SPLFPU			*/
};

/* The mask values for the system interrup mask */
int kn02ba_sim[SPLMSIZE] = {
	SPL0_MASK,			/* 0 SPLNONE			*/
	SPL1_MASK,			/* 1 SPLSOFTC			*/
	SPL2_MASK,			/* 2 SPLNET			*/
	0,				/* 3 NOT_USED 			*/
	0,				/* 4 NOT_USED 			*/
	SPLIO_MASK,			/* 5 SPLBIO, SPLIMP, SPLTTY	*/
	SPLCLOCK_MASK,			/* 6 SPLCLOCK			*/
	SPLMEM_MASK,			/* 7 SPLMEM			*/
	SPLFPU_MASK,			/* 8 SPLFPU			*/
};

u_int kn02ba_slotaddr[TC_IOSLOTS] = {KN02BA_SL_0_ADDR, KN02BA_SL_1_ADDR,
	KN02BA_SL_2_ADDR, KN02BA_SL_3_ADDR, KN02BA_SL_4_ADDR,
	KN02BA_SL_5_ADDR, KN02BA_SL_6_ADDR, KN02BA_SL_7_ADDR};

/*
 * Program the order in which to probe the IO slots on the system.
 * This determines the order in which we assign unit numbers to like devices.
 * It also determines how many slots (and what slot numbers) there are to probe.
 * Terminate the list with a -1.
 * Note: this agrees with the console's idea of unit numbers
 */
int kn02ba_config_order [] = { 3, 4, 5, 0, 1, 2, -1 };

/* ipllevel describes the current interrupt level the processor is */
/* running at.  The values that ipllevel can be are SPLNONE -> SPLEXTREME. */
/* These values are defined in cpu.h */

int ipllevel = SPLEXTREME;

int clk_counter;

int kn02ba_transintvl = KN02BA_TRANSINTVL;/* global var so we can change it */
int kn02ba_translog = 1;		/* is trans logging currently enabled */
struct trans_errcnt kn02ba_trans_errcnt = { 0, 0 };
int kn02ba_tcount[MAXSIMM];		/* # of transient parity errs per simm*/
int kn02ba_scount[MAXSIMM];		/* # of soft errs on each simm */
int kn02ba_hcount[MAXSIMM];		/* # of hard errs on each simm */
int kn02ba_pswarn;			/* set true if we had a ps-warning */
int kn02ba_model_number;

/* Initial value is worst case, real value is determined in kn02ba_init() */
int kn02ba_delay_mult = 25; 


int kn02ba_psintvl = (60 * 1);	/* time delta to check pswarn bit */
                                /* global var so we can change it */

unsigned kn02ba_isolatepar();
int kn02ba_transenable();
int kn02ba_pscheck();
caddr_t vatophys();

kn02ba_init()
{
        int i;
	extern int c0vec_tblsize;
	extern int (*c0vec_tbl[])();
	extern int splm[];
	extern struct cpusw *cpup;
	extern u_int sr_usermask;
	/*
	 * Initialize the interrupt dispatch table c0vec_tbl.
	 * Initialize the spl table for the system we are running on.
	 * Initialize the interrupt masks for the system we are running on.
	 * This must be done very early on at boot time and must
	 * be done before any interrupt is allowed.
	 */
	bcopy(kn02ba_splm, splm, (SPLMSIZE) * sizeof(int));

	/* Initialize the spl dispatch table and the intr dispatch routine */
	spl_init();

	/* disable all DMA's, but don't reset the chips */
	*(u_int *)(PHYS_TO_K1(KN02BA_SSR_ADDR)) = 0xf00;

/* rpbfix: do we want to enable halt interrupt at this point */
	splextreme();
	/*
	 * Set up the system specific value for "hz", the number of clock
	 * interrupts per second; and corresponding tick and tickadj values.
	 */
	hz = cpup->HZ;
	tick = 1000000 / hz;
	tickadj = 240000 / (60 * hz);
	fixtick = 1000000 - (tick * hz);

	/*
	 * Assign the rt_clock_addr for this processor
	 */
	rt_clock_addr = (char *)PHYS_TO_K1(KN02BA_CLOCK_ADDR);

	clk_counter = kn02ba_conf_clk_speed();

	if (clk_counter < 8775) {
	    kn02ba_model_number = 120;
	    kn02ba_delay_mult = 11;
	} else if (clk_counter < 11311) {
	    kn02ba_model_number = 125;
	    kn02ba_delay_mult = 13;
	} else if (clk_counter < 14000) {
	    kn02ba_model_number = 133;
	    kn02ba_delay_mult = 19;
	} else {
	    kn02ba_model_number = 0;
	    kn02ba_delay_mult = 25;
	}

	/*
	 *
	 */
	sr_usermask = (KN02BA_SR_IMASK0 | SR_IEP | SR_KUP );
	sr_usermask &= ~(SR_IEC);

	/* Initialize the TURBOchannel */
	tc_init();

	/* Fill in the TURBOchannel slot addresses */
	for (i = 0; i < TC_IOSLOTS; i++)
	    tc_slotaddr[i] = kn02ba_slotaddr[i];

	/* Fill in the TURBOchannel switch table */
	tc_sw.isolate_memerr = kn02ba_isolate_memerr;
	tc_sw.enable_option = kn02ba_enable_option;
	tc_sw.disable_option = kn02ba_disable_option;
	tc_sw.clear_errors = kn02ba_clear_errors;
	tc_sw.config_order = kn02ba_config_order;

	/*
	 * Fixed 3min IO devices
	 */

	strcpy(tc_slot[KN02BA_SCSI_INDEX].devname,"asc");
	strcpy(tc_slot[KN02BA_SCSI_INDEX].modulename, "PMAZ-BA ");
	tc_slot[KN02BA_SCSI_INDEX].slot = 3;
	tc_slot[KN02BA_SCSI_INDEX].module_width = 1;
	tc_slot[KN02BA_SCSI_INDEX].physaddr = KN02BA_SCSI_ADDR;
	tc_slot[KN02BA_SCSI_INDEX].intr_b4_probe = 0;
	tc_slot[KN02BA_SCSI_INDEX].intr_aft_attach = 1;
	tc_slot[KN02BA_SCSI_INDEX].adpt_config = 0;

	strcpy(tc_slot[KN02BA_LN_INDEX].devname,"ln");
	strcpy(tc_slot[KN02BA_LN_INDEX].modulename, "PMAD-BA ");
	tc_slot[KN02BA_LN_INDEX].slot = 3;
	tc_slot[KN02BA_LN_INDEX].module_width = 1;
	tc_slot[KN02BA_LN_INDEX].physaddr = KN02BA_LN_ADDR;
	tc_slot[KN02BA_LN_INDEX].intr_b4_probe = 0;
	tc_slot[KN02BA_LN_INDEX].intr_aft_attach = 1;
	tc_slot[KN02BA_LN_INDEX].adpt_config = 0;

	strcpy(tc_slot[KN02BA_SCC_INDEX].devname,"scc");
	tc_slot[KN02BA_SCC_INDEX].slot = 3;
	tc_slot[KN02BA_SCC_INDEX].module_width = 1;
	tc_slot[KN02BA_SCC_INDEX].physaddr = KN02BA_SCC_ADDR;
	tc_slot[KN02BA_SCC_INDEX].intr_b4_probe = 0;
	tc_slot[KN02BA_SCC_INDEX].intr_aft_attach = 1;
	tc_slot[KN02BA_SCC_INDEX].adpt_config = 0;

	*(u_int *)PHYS_TO_K1(KN02BA_INTR_REG) = 0;

/* rpbfix: these should go into their respective drivers */
	*(u_int *)(0xbc040160) = 0x3;
	*(u_int *)(0xbc040170) = 0xe;
	*(u_int *)(0xbc040180) = 0x14;
	*(u_int *)(0xbc040190) = 0x16;

	return (0);
}

kn02ba_conf()
{
        extern int cold;
	extern u_int cpu_systype;
	extern int icache_size, dcache_size;

#ifdef ERRLOG_DEBUG
	int kn02ba_errlog_testing();
#endif	

	cold = 1;
	
	/*
	 * Initialize PROM environment entries.
	 */
	hwconf_init();
	
	/* 
	 * Report what system we are on
	 */
	if (kn02ba_model_number != 0)
	    printf("DECstation 5000 Model %d - system rev %d\n", 
		   kn02ba_model_number,  (GETHRDREV(cpu_systype)));
	else
	    printf("DECstation 5000 Model 100 Series, Unknown Clock Speed\n");

	printf("%dKb Instruction Cache, %dKb Data Cache\n", 
	       icache_size/1024, dcache_size/1024);

	coproc_find();
	
	/* Turn off all interrupts in the SIRM */
	*(u_int *)PHYS_TO_K1(KN02BA_SIRM_ADDR) = 0;
	
	/*
	 * Probe the TURBOchannel and find all devices
	 */
	splextreme();
	tc_find();
	
	timeout (kn02ba_pscheck, (caddr_t) 0, kn02ba_psintvl * hz);
	timeout (kn02ba_transenable, (caddr_t) 0, kn02ba_transintvl * hz);

#ifdef ERRLOG_DEBUG
	timeout (kn02ba_errlog_testing, (caddr_t) 0, 5 * hz);
#endif
	cold = 0;
	splnone();

	return (0);	/* tell configure() we have configured */
}

/*
 * Check Power Supply over-heat Warning.
 * If its overheating, warn to shut down the system.
 * If its gone from overheat to OK, cancel the warning.
 */
kn02ba_pscheck()
{
	register u_int sir;		/* a copy of the real csr */
	
	sir = *(u_int *)PHYS_TO_K1(KN02BA_SIR_ADDR);
	
	if (sir & PSWARN) {
	    printf("System Overheating - suggest immediate shutdown and power-off\n");
	    kn02ba_pswarn = 1;
	} else {
	    if (kn02ba_pswarn) {
		printf("System OK - cancel overheat shutdown\n");
		kn02ba_pswarn = 0;
	    }
	}
	timeout (kn02ba_pscheck, (caddr_t) 0, kn02ba_psintvl * hz);
}

/*
 * Enable transient parity memory error logging
 */
kn02ba_transenable()
{
	kn02ba_translog = 1;
	timeout (kn02ba_transenable, (caddr_t) 0, kn02ba_transintvl * hz);
}

/*
 * Routine to handle trap errors: user-mode ibe & dbe, & all kernel mode traps.
 * We try to recover from user-mode errors and panic on kernel mode errors.
 */
kn02ba_trap_error(ep, code, sr, cause, signo)
	register u_int *ep;		/* exception frame ptr */
	register u_int code;		/* trap code (trap type) */
	u_int sr, cause;		/* status and cause regs */
	int *signo;			/* set if we want to kill process */
{
	caddr_t pa;			/* the physical addr of the error */	
	int epc;			/* the EPC of the error */	
	unsigned memreg;		/* memory parity error info */
	int vaddr;			/* virt addr of error */
	register struct proc *p;	/* ptr to current proc struct */
	long currtime;			/* current time value */
	unsigned ssr;
	unsigned sir;
	unsigned sirm;
	
	ssr = *(u_int *)(PHYS_TO_K1(KN02BA_SSR_ADDR));
	sir = *(u_int *)(PHYS_TO_K1(KN02BA_SIR_ADDR));
	sirm = *(u_int *)(PHYS_TO_K1(KN02BA_SIRM_ADDR));
	
	p = u.u_procp;
	if (USERMODE(sr)) {
	    /*
	     * If address of bus error is in physical memory, then its
	     * a parity memory error.  Gather additional info in "memreg",
	     * for the error log & to determine how to recover.
	     * If its a transient error then continue the user process.
	     * If its a hard or soft parity error:
	     *    a) on a private process page, terminate the process
	     *	 (by setting signo = SIGBUS)
	     *    b) on a shared page, crash the system.
	     * TBD: on a non-modified page, re-read the page (page fault),
	     *	and continue the process.
	     * TBD: on a shared page terminate all proc's sharing the page,
	     *	instead of crash system.
	     * TBD: on hard errors map out the page.
	     */
	    pa = vatophys(ep[EF_BADVADDR]);
	    if ( (int)pa != -1 && (btop((int)pa) < physmem) ) {
		memreg = kn02ba_isolatepar(pa, ep[EF_BADVADDR], 4);
		/*
		 * If we get 3 or more in 1 second then disable logging
		 * them for 15 minutes.  The variable "kn02ba_translog"
		 * is set by the kn02ba_transenable routine.
		 */
		if (((memreg >> TYPEOFF) & TRANSPAR) == TRANSPAR) {
		    if (kn02ba_translog) {
			currtime = time.tv_sec;
			if (currtime == kn02ba_trans_errcnt.trans_prev) {
			    kn02ba_translog = 0;
			    mprintf("High rate of transient parity memory errors, logging disabled for 15 minutes\n");
			    kn02ba_trans_errcnt.trans_last = 0;
			    currtime = 0;
			}
			kn02ba_logmempkt(EL_PRIHIGH, ep, memreg, pa);
			kn02ba_trans_errcnt.trans_prev = kn02ba_trans_errcnt.trans_last;
			kn02ba_trans_errcnt.trans_last = currtime;
		    }
		    return(0);
		}
		
		if (SHAREDPG(pa)) {
		    kn02ba_logmempkt(EL_PRISEVERE, ep, memreg, pa);
		    kn02ba_consprint(KN02BA_MEMPKT, ep, memreg, pa, 0, 0, 0);
		    panic("memory parity error in shared page");
		} else {
		    kn02ba_logmempkt(EL_PRIHIGH, ep, memreg, pa);
		    printf("pid %d (%s) was killed on memory parity error\n",
			   p->p_pid, u.u_comm);
		    uprintf("pid %d (%s) was killed on memory parity error\n",
			    p->p_pid, u.u_comm);
		}
	    } else {
		uprintf("pid %d (%s) was killed on bus error\n",
			p->p_pid, u.u_comm);
	    }
	} else {
	    /*
	     * Kernel mode errors.
	     * They all panic, its just a matter of what we log
	     * and what panic message we issue.
	     */
	    switch (code) {
		
	      case EXC_DBE:
	      case EXC_IBE:
		/*
		 * Figure out if its a memory parity error
		 *     or a read bus timeout error
		 */
		pa = vatophys(ep[EF_BADVADDR]);
		if ( (int)pa != -1 && (btop((int)pa) < physmem) ) {
		    /*
		     * Note that we must save anything "interesting"
		     * from the exception frame, since isolatepar()
		     * may cause additional bus errors which will
		     * stomp on the exception frame in locore.
		     */
		    vaddr = ep[EF_BADVADDR];
		    epc = ep[EF_EPC];
		    memreg = kn02ba_isolatepar(pa, vaddr, 4);
		    ep[EF_BADVADDR] = vaddr;
		    ep[EF_EPC] = epc;
		    kn02ba_logmempkt(EL_PRISEVERE, ep, memreg, pa);
		    kn02ba_consprint(KN02BA_MEMPKT, ep, memreg, pa, 0, 0, 0);
		    panic("memory parity error in kernel mode");
		} else {
		    kn02ba_logesrpkt(EL_PRISEVERE, ep, ssr, sir, sirm);
		    kn02ba_consprint(KN02BA_ESRPKT, ep, 0, 0, ssr, sir, sirm);
		    panic("bus timeout");
		}
		break;
	      case EXC_CPU:
		kn02ba_logesrpkt(EL_PRISEVERE, ep, ssr, sir, sirm);
		kn02ba_consprint(KN02BA_ESRPKT, ep, 0, 0, ssr, sir, sirm);
		panic("coprocessor unusable");
		break;
	      case EXC_RADE:
	      case EXC_WADE:
		kn02ba_logesrpkt(EL_PRISEVERE, ep, ssr, sir, sirm);
		kn02ba_consprint(KN02BA_ESRPKT, ep, 0, 0, ssr, sir, sirm);
		panic("unaligned access");
		break;
	      default:
		kn02ba_logesrpkt(EL_PRISEVERE, ep, ssr, sir, sirm);
		kn02ba_consprint(KN02BA_ESRPKT, ep, 0, 0, ssr, sir, sirm);
		panic("trap");
		break;
	    }
	}
	/*
	 * Default user-mode action is to terminate the process
	 */
	*signo = SIGBUS;
	return(0);
}

kn02ba_intr(ep, code, sr, cause)
u_int *ep;
u_int code, sr, cause;
{
	register u_int req, sir, tmp_cause;
        register int nintr = 1, current_ipl;
	extern int nstrays;
	u_int current_sirm = *(u_int *)PHYS_TO_K1(KN02BA_SIRM_ADDR);
	
	CURRENT_CPUDATA->cpu_inisr++;
	
	/* remove any pending intr's that are masked */
	tmp_cause = cause & sr;
	
	req = ffintr(tmp_cause);
	switch (req) {
	  case CR_HALT:
	    /* loop until the halt button is released */
            while (get_cause() & SR_IBIT7) ;
	    kn02ba_halt(ep);
	    break;
	    
	  case CR_FPU:
	    splx(SPLFPU);
	    fpuintr(ep);
	    break;
	    
	  case CR_SYSTEM:
	    sir = *(u_int *)PHYS_TO_K1(KN02BA_SIR_ADDR);
	    
	    /* mask out bits that we don't want */
	    sir &= current_sirm;
	    
	    if (sir & CPU_IO_TIMEOUT) {
		splx(SPLMEM);
		kn02ba_errintr(ep);
	    } else if (sir & TOY_INTR) {
		splx(SPLCLOCK);
		hardclock(ep);
	    } else if (sir & SLU_INTR) {
		splx(SPLIO);
		(*(tc_slot[KN02BA_SCC_INDEX].intr))();
	    } else if (sir & LANCE_INTR) {
		splx(SPLIO);
		(*(tc_slot[KN02BA_LN_INDEX].intr))();
	    } else if (sir & SCSI_INTR) {
		splx(SPLIO);
		(*(tc_slot[KN02BA_SCSI_INDEX].intr))();
	    } else {
		nintr--;
		nstrays++;
		/* rpbfix: downgrade to a printf after debugging */
		cprintf("Stray interrupt from System Interrupt Register");
	    }
	    break;
	    
	  case CR_OPTION_2:
	    splx(SPLIO);
	    (*(tc_slot[2].intr))(tc_slot[2].unit);
	    break;
	    
	  case CR_OPTION_1:
	    splx(SPLIO);
	    (*(tc_slot[1].intr))(tc_slot[1].unit);
	    break;
	    
	  case CR_OPTION_0:
	    splx(SPLIO);
	    (*(tc_slot[0].intr))(tc_slot[0].unit);
	    break;
	    
	  case CR_SOFTNET:
	    splx(SPLNET);
	    softnet(ep);
	    break;
	    
	  case CR_SOFTCLOCK:
	    splx(SPLSOFTC);
	    softclock(ep);
	    break;
	    
	  case CR_NONE:
	    /* rpbfix: downgrade to printf later */
	    nintr--;
	    nstrays++;
	    /*	    panic("no interrupt pending");*/
	    cprintf("no interrupt pending, cause = %x, sr = %x\n", cause, sr);
	    break;
	}
	
	CURRENT_CPUDATA->cpu_intr += nintr;
	CURRENT_CPUDATA->cpu_inisr--;
}

kn02ba_getspl()
{
    return (ipllevel);
}

/* On 3min, the value returned by getspl is the actual ipllevel */
kn02ba_whatspl(ipl)
u_int ipl;
{
    return (ipl);
}

kn02ba_errintr(ep)
    register u_int *ep;		/* exception frame ptr */
{
        register unsigned cpu_addr_err;
	register unsigned ssr;
	register unsigned sir;
	register unsigned sirm;
	struct kn02ba_consinfo_t kn02ba_consinfo, *p;
	
	ssr = *(u_int *)(PHYS_TO_K1(KN02BA_SSR_ADDR));
	sir = *(u_int *)(PHYS_TO_K1(KN02BA_SIR_ADDR));
	sirm = *(u_int *)(PHYS_TO_K1(KN02BA_SIRM_ADDR));
	
	cpu_addr_err = *(u_int *)PHYS_TO_K1(KN02BA_ADDR_ERR);
	ep[EF_BADVADDR] = cpu_addr_err;
	/*
	 * if we are still processing an previous interrupt
	 * then simply crash. we don't queue these interrupts.
	 */
	if (CURRENT_CPUDATA->cpu_wto_event) {
	    kn02ba_logesrpkt(EL_PRISEVERE, ep, ssr, sir, sirm);
	    kn02ba_consprint(KN02BA_ESRPKT, ep, 0, 0, ssr, sir, sirm);
	    *(u_int *)PHYS_TO_K1(KN02BA_INTR_REG) = 0;
	    wbflush();
	    panic("CPU write timeout");
	}
	else {
	    /*
	     *	capture error information in kn02consinfo.
	     *	softnet() interrupt will print this info
	     *	if panicing on the console.
	     */
	    p = &kn02ba_consinfo;
	    /*
	     *	capture log information in kn02log_errinfo.
	     *	softnet() interrupt will log this info
	     *	if panicing in the error log buffer.
	     */
	    p->pkt_type 		= KN02BA_ESRPKT;
	    p->pkt.esrp.cause		= ep[EF_CAUSE];	
	    p->pkt.esrp.epc		= ep[EF_EPC];
	    p->pkt.esrp.status		= ep[EF_SR];	
	    p->pkt.esrp.badva		= ep[EF_BADVADDR];
	    p->pkt.esrp.sp		= ep[EF_SP];	
	    p->pkt.esrp.ssr		= ssr;
	    p->pkt.esrp.sir		= sir;
	    p->pkt.esrp.sirm		= sirm;
	    CURRENT_CPUDATA->cpu_consinfo	= (char *) &kn02ba_consinfo;
	    CURRENT_CPUDATA->cpu_log_errinfo	= (char *) &kn02ba_consinfo;
	    CURRENT_CPUDATA->cpu_wto_pfn    	= btop(cpu_addr_err);
	    CURRENT_CPUDATA->cpu_wto_event 		= 1;
	    *(u_int *)PHYS_TO_K1(KN02BA_INTR_REG) 	= 0;
	    wbflush();
	    setsoftnet();
	}
}

/*
 *
 * Tested from 5 seconds down to 4,000 usecs (4 mSec clock accuracy).
 *
 */
kn02ba_delay(n)
	int n;
{
	register int N = kn02ba_delay_mult*(n); 
	while (--N > 0); 
	return(0);
}

kn02ba_stray()
{
        /* rpbfix: downgrade to printf */
        panic("Received stray interrupt");
}

halt_cnt = 0;

kn02ba_halt(ep)
u_int *ep;
{
    /* print out value of PC, SP, and EP with labels */
    rex_printf("\nPC:\t0x%x\nSP:\t0x%x\nEP:\t0x%x\n\n", ep[EF_EPC], ep[EF_SP], ep);
    rex_halt(0,0);
}

kn02ba_enable_option(index)
    register int index;
{
        register int i;
	
	switch (index) {
	  case KN02BA_SCC_INDEX:
	    for (i = 0; i < SPLIO; i++)
		kn02ba_sim[i] |= (SLU_INTR);
	    break;
	    
	  case KN02BA_LN_INDEX:
	    for (i = 0; i < SPLIO; i++)
		kn02ba_sim[i] |= (LANCE_INTR);
	    break;
	    
	  case KN02BA_SCSI_INDEX:
	    /* rpbfix: reenable when scsi is really turned on */
	    for (i = 0; i < SPLIO; i++)
		kn02ba_sim[i] |= (SCSI_INTR);
	    break;
	    
	  case TC_OPTION_SLOT_0:
	    for (i = 0; i < SPLIO; i++)
		splm[i] |= SR_IBIT3;
	    sr_usermask |= SR_IBIT3;
	    break;
	    
	  case TC_OPTION_SLOT_1:
	    for (i = 0; i < SPLIO; i++)
		splm[i] |= SR_IBIT4;
	    sr_usermask |= SR_IBIT4;
	    break;
	    
	  case TC_OPTION_SLOT_2:
	    for (i = 0; i < SPLIO; i++)
		splm[i] |= SR_IBIT5;
	    sr_usermask |= SR_IBIT5;
	    break;
	    
	  default:
	    /* rpbfix: downgrade to printf */
	    panic("Enable_option call to non existent slot");
	    break;
	}
	
	/* load the registers with the new values */
	splx(ipllevel);
}

kn02ba_disable_option(index)
    register int index;
{
        register int i;

	switch (index) {
	  case KN02BA_SCC_INDEX:
	    for (i = 0; i < SPLIO; i++)
		kn02ba_sim[i] &= ~(SLU_INTR);
	    break;
	    
	  case KN02BA_LN_INDEX:
	    for (i = 0; i < SPLIO; i++)
		kn02ba_sim[i] &= ~(LANCE_INTR);
	    break;
	    
	  case KN02BA_SCSI_INDEX:
	    for (i = 0; i < SPLIO; i++)
		kn02ba_sim[i] &= ~(SCSI_INTR);
	    break;
	    
	  case TC_OPTION_SLOT_0:
	    for (i = 0; i < SPLIO; i++)
		splm[i] &= ~(SR_IBIT3);
	    sr_usermask &= ~(SR_IBIT3);
	    break;
	    
	  case TC_OPTION_SLOT_1:
	    for (i = 0; i < SPLIO; i++)
		splm[i] &= ~(SR_IBIT4);
	    sr_usermask &= ~(SR_IBIT4);
	    break;
	    
	  case TC_OPTION_SLOT_2:
	    for (i = 0; i < SPLIO; i++)
		splm[i] &= ~(SR_IBIT5);
	    sr_usermask &= ~(SR_IBIT5);
	    break;
	    
	  default:
	    /* rpbfix: downgrade to printf */
	    panic("disable_option call to non existent slot");
	    break;
	}
	
	/* load the registers with the new values */
	splx(ipllevel);
}

kn02ba_clear_errors()
{
}

#define KN02BA_LOG_ESRPKT(elrp, cause,epc,sr,badva,sp,ssr,sir,sirm)	\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_cause = cause;	\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_epc = epc;		\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_status = sr;		\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_badva = badva;	\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_sp = sp;		\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_ssr = ssr;		\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_sir = sir;		\
	elrp->el_body.elesr.elesr.el_esrkn02ba.esr_sirm = sirm;		\

/*
 * Log Error & Status Registers to the error log buffer
 */
kn02ba_logesrpkt(priority, ep, ssr, sir, sirm)
	int priority;		/* for pkt priority */
	register u_int *ep;	/* exception frame ptr */
	u_int ssr;
	u_int sir;
	u_int sirm;
{
	struct el_rec *elrp;
	
	elrp = ealloc(sizeof(struct el_esr), priority);
	if (elrp != NULL) {
	    LSUBID(elrp,ELCT_ESR,ELESR_KN02BA,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	    KN02BA_LOG_ESRPKT(elrp, ep[EF_CAUSE], ep[EF_EPC], ep[EF_SR], ep[EF_BADVADDR], 
			      ep[EF_SP], ssr, sir, sirm);
	    
	    EVALID(elrp);
	}
}

/*
 * Log a memory error packet, so uerf can find it as a main memory error.
 */
kn02ba_logmempkt(priority, ep, memreg, pa)
	int priority;		/* pkt priority: panic: severe; else: high */
	register u_int *ep;	/* exception frame ptr */
	unsigned memreg;	/* assorted parity error info */
	int pa;			/* physical addr where memory err occured */
{
	struct el_rec *elrp;
	register struct el_mem *mrp;
	
	elrp = ealloc(EL_MEMSIZE, priority);
	if (elrp != NULL) {
	    LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_KN02BA,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	    mrp = &elrp->el_body.elmem;
	    mrp->elmem_cnt = 1;
	    mrp->elmemerr.cntl = 1;
	    mrp->elmemerr.type = ELMETYP_PAR;
	    mrp->elmemerr.numerr = 1;
	    mrp->elmemerr.regs[0] = memreg;
	    mrp->elmemerr.regs[1] = pa;
	    mrp->elmemerr.regs[2] = ep[EF_EPC];;
	    mrp->elmemerr.regs[3] = ep[EF_BADVADDR];;
	    EVALID(elrp);
	}
}

/*
 * 	Logs error information to the error log buffer.
 *	Exported through the cpu switch.
 */

kn02ba_log_errinfo(p)
struct kn02ba_consinfo_t *p;
{
	struct el_rec *elrp;
	struct kn02ba_consinfo_esr_t *esrp;
	
	switch (p->pkt_type) {
	    
	  case KN02BA_ESRPKT:
	    esrp = &(p->pkt.esrp);
	    elrp = ealloc(sizeof(struct el_esr), EL_PRISEVERE);
	    if (elrp != NULL) {
		LSUBID(elrp,ELCT_ESR,ELESR_KN02BA,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		KN02BA_LOG_ESRPKT(elrp, esrp->cause, esrp->epc, esrp->status, esrp->badva, 
				  esrp->sp, esrp->ssr, esrp->sir, esrp->sirm);
		EVALID(elrp);
	    }
	    break;
	    
	  default: 
	    cprintf("bad pkt type\n");
	    return;
	}
}

/*
 * Print error packet to the console.
 * This is only done when we are about to panic on the error.
 * It calls kn02_print_consinfo to actually print the information.
 *
 */
kn02ba_consprint(pkt, ep, memreg, pa, ssr, sir, sirm)
	int pkt;		/* error pkt: Error & Stat Regs / memory pkt */
	register u_int *ep;	/* exception frame ptr */
	unsigned memreg;	/* For MEMPKT: assorted parity error info */
	unsigned pa;		/* For MEMPKT: physical addr of error */	
	unsigned ssr;
	unsigned sir;
	unsigned sirm;
{
	register int i;
	struct kn02ba_consinfo_t p;
	
	
	p.pkt_type = pkt;
	switch (pkt) {
	  case KN02BA_ESRPKT:
	    p.pkt.esrp.cause	= ep[EF_CAUSE];
	    p.pkt.esrp.epc		= ep[EF_EPC];
	    p.pkt.esrp.status	= ep[EF_SR];
	    p.pkt.esrp.badva	= ep[EF_BADVADDR];
	    p.pkt.esrp.sp		= ep[EF_SP];
	    p.pkt.esrp.ssr		= ssr;
	    p.pkt.esrp.sir		= sir;
	    p.pkt.esrp.sirm		= sirm;
	    break;
	    
	  case KN02BA_MEMPKT:
	    p.pkt.memp.epc		= ep[EF_EPC];
	    p.pkt.memp.badva	= ep[EF_BADVADDR];
	    p.pkt.memp.memreg	= memreg;
	    p.pkt.memp.pa		= pa;
	    break;
	    
	  default:
	    cprintf("bad consprint\n");
	    return;
	}
	kn02ba_print_consinfo(&p);
	
}

/*
 *	This routine is similar to kn02consprint().
 *	This is exported through the cpusw structure. 
 *	
 */

kn02ba_print_consinfo(p)
struct kn02ba_consinfo_t *p;
{
        int simm, byte;
	u_int memreg;
	
	/*
	 * If console is a graphics device,
	 * force printf messages directly to screen.
	 */
	printstate |= PANICPRINT;
	
	switch (p->pkt_type) {
	  case KN02BA_ESRPKT:
	    cprintf("\nException condition\n");
	    cprintf("\tCause reg\t= 0x%x\n", p->pkt.esrp.cause);
	    cprintf("\tException PC\t= 0x%x\n", p->pkt.esrp.epc);
	    cprintf("\tStatus reg\t= 0x%x\n", p->pkt.esrp.status);
	    cprintf("\tBad virt addr\t= 0x%x\n", p->pkt.esrp.badva);
	    cprintf("\tStack ptr\t= 0x%x\n", p->pkt.esrp.sp);
	    cprintf("\tSystem Support reg = 0x%x\n", p->pkt.esrp.ssr);
	    cprintf("\tSystem Interrupt reg = 0x%x\n", p->pkt.esrp.sir);
	    cprintf("\tSystem Interrupt Mask reg = 0x%x\n", p->pkt.esrp.sirm);
	    break;
	    
	  case KN02BA_MEMPKT:
	    memreg = p->pkt.memp.memreg;
	    cprintf("\nMemory Parity Error\n");
	    simm =  (memreg >> SIMMOFF) & 0xf;
	    cprintf("\tSIMM (module number)\t= BANK %d, %s\n", 
		    simm/2, ((simm & 0x1) ? "D16-31" : "D0-15"));
	    if (((memreg >> TYPEOFF) & HARDPAR) == HARDPAR)
		cprintf("\tHard error\t\n");
	    else if (((memreg >> TYPEOFF) & SOFTPAR) == SOFTPAR)
		cprintf("\tSoft error\t\n");
	    else cprintf("\tTransient error\t\n");
	    if (simm & 0x1) {
		/* D16-31(high) simm: high half word */
		if ((memreg >> BYTEOFF) & 0x1)
		    byte = 3;
		else
		    byte = 2;
	    } else {
		/* D0-15(low) simm: low half word */
		if ((memreg >> BYTEOFF) & 0x1)
		    byte = 1;
		else
		    byte = 0;
	    }
	    cprintf("\tByte in error (0-3)\t= %d\n", byte);
	    cprintf("\t%s bit error\n", ((memreg >> DPOFF) & 0x1) ? "Parity" : "Data");
	    cprintf("\tTransient errors for this SIMM\t= %d\n", kn02ba_tcount[simm]);
	    cprintf("\tSoft errors for this SIMM\t= %d\n", kn02ba_scount[simm]);
	    cprintf("\tHard errors for this SIMM\t= %d\n", kn02ba_hcount[simm]);
	    cprintf("\tPhysical address of error\t= 0x%x\n", p->pkt.memp.pa);
	    cprintf("\tException PC\t\t\t= 0x%x\n", p->pkt.memp.epc);
	    cprintf("\tVirtual address of error\t= 0x%x\n", p->pkt.memp.badva);
	    break;
	    
	  default:
	    cprintf("bad print_consinfo \n");
	    break;
	}
}

kn02ba_isolate_memerr(memerr_status)
	struct tc_memerr_status *memerr_status;
{
        unsigned memreg;
	int ep[EF_SIZE/4];

	if (btop((int)memerr_status->pa) >= physmem)
	    return (-1);

	/* zero out these since they are not pertinent 	*/
	/* for this type of error			*/
	ep[EF_EPC] = 0;
	ep[EF_BADVADDR] = 0;
	
	memreg = kn02ba_isolatepar(memerr_status->pa, memerr_status->va, memerr_status->blocksize);

	memerr_status->errtype = TC_MEMERR_NOERROR;

	if (((memreg >> TYPEOFF) & HARDPAR) == HARDPAR)
	    memerr_status->errtype = TC_MEMERR_HARD;
	else if (((memreg >> TYPEOFF) & SOFTPAR) == SOFTPAR)
	    memerr_status->errtype = TC_MEMERR_SOFT;
	else if (((memreg >> TYPEOFF) & TRANSPAR) == TRANSPAR)
	    memerr_status->errtype = TC_MEMERR_TRANS;

	if (memerr_status->log == TC_LOG_MEMERR) {
	    kn02ba_logmempkt(EL_PRISEVERE, ep, memreg, memerr_status->pa);
	    kn02ba_consprint(KN02BA_MEMPKT, ep, memreg, memerr_status->pa, 0, 0, 0);
	}

}

/*
 * Isolate a memory parity error to which SIMM is in error.
 * This routine is machine specific, in that it "knows" how the memory
 * is laid out, i.e. how to convert a physical address to a module number.
 *
 * Block faults from occuring while we isolate the parity error by using
 * "nofault" facility thru the bbadaddr routine.
 */
unsigned
kn02ba_isolatepar(pa, va, blocksize)
	register caddr_t pa;	/* the phys addr to convert to a SIMM */	
	caddr_t va;		/* the virtual addr of the error */	
	int blocksize;		/* the size of the block error occured in */
{
	register int i;		/* loop index */	
	register int *blockaddr;/* address of the beginning of block in error */
	register int *addr;	/* increment thru the block w/ parity error */
	register char *baddr;	/* increment thru the word w/ parity error */
	unsigned memreg;	/* collection of memory error info */
	int low;		/* true if its the D0-15(low) SIMM */
	int simm;		/* which simm had the error */
	register int allzeros;	/* true if parity err occurs on all 0's write */
	register int allones;	/* true if parity err occurs on all 1's write */
	register int oneone;	/* true if parity err occurs on 1 1 write */
	int dp;			/* 0 for data bit, 1 for parity bit */
	int type;		/* error type: transient, soft, hard */
	int byte;		/* 0 for low byte; 1 for high byte in word */
	int bank;
	int banksize;
	int parityerr;
	int blockcnt;


	/* 
	 * Round physical address to beginning of block
	 */
	blockaddr = (int *)(PHYS_TO_K1((int)pa & ~((blocksize << 2) - 1)));
	addr = blockaddr;
	for (blockcnt = 0; blockcnt < blocksize; blockcnt++, addr++) {
	
	    type = 0;
	    dp = 0;
	    /*
	     * Do badaddr probe on addr (a few times),
	     * to see if it was only a transient.
	     */
	    parityerr = 0;
	    for (i = 0; i < 4; i++) {
		if (bbadaddr(addr, 4)) {
		    parityerr = 1;
		    break;
		}
	    }
	    if (!parityerr) {
		/* if no error, try the next word */
		continue;
	    }
	    /*
	     * Isolate the parity error to which SIMM is in error (which byte in
	     * the word) and isolate the type of error: soft or hard, data bit
	     * or parity bit.
	     *
	     * This is done by writing (& reading) each byte in the word first
	     * with all 0's then with all 1's (0xff) then with one 1 (0x1).
	     *
	     * Use k1 address in order not to get TLBMOD exception when writing
	     * shared memory space.
	     */
	    for (i = 0, baddr = (char *)addr; i < 4; i++, baddr += 1) {
		allzeros = 0;
		*baddr = 0x00;
		if (bbadaddr(baddr, 1))
		    allzeros = 1;
		allones = 0;
		*baddr = 0xff;
		if (bbadaddr(baddr, 1))
		    allones = 1;
		oneone = 0;
		*baddr = 0x1;
		if (bbadaddr(baddr, 1))
		    oneone = 1;
		/*
		 * If all 3 reads caused the error then this is the wrong
		 * byte, go on to the next byte
		 */
		if (allzeros && allones && oneone)
		    continue;
		/*
		 * If only one of the allones/allzeros patterns caused a
		 * parity error, then we have a hard data bit stuck to
		 * zero or one.
		 */
		if ((allzeros && !allones && !oneone) ||
		    (allones && !allzeros && !oneone)) {
		    type = HARDPAR;
		    break;
		}
		/*
		 * If only the "oneone" (0x1) pattern caused a parity error,
		 *   then we have a parity bit stuck to zero.
		 * If only the "oneone" (0x1) pattern did NOT cause a parity
		 *   error then we have a parity bit stuck to one.
		 */
		if ((oneone && !allzeros && !allones) ||
		    (allzeros && allones && !oneone)) {
		    type = HARDPAR;
		    dp = 1;
		    break;
		}
		/*
		 * If no parity error on all 3 patterns then we had a soft
		 * parity error in one of the data bits or in the parity bit
		 * of this byte.
		 */
		if (!allzeros && !allones && !oneone) {
		    type = SOFTPAR;
		    break;
		}
	    }
	    /*
	     * If i is 0 or 1, parity error is on the D0-15(low) SIMM.
	     * If i is 2 or 3, parity error is on the D16-31(high) SIMM.
	     * Also record high or low byte position in half-word.
	     */
	    switch (i) {
	      case 0:
		byte = 0;
		low = 1;
		break;
	      case 1:
		byte = 1;
		low = 1;
		break;
	      case 2:
		byte = 0;
		low = 0;
		break;
	      case 3:
	      default:
		byte = 1;
		low = 0;
		break;
	    }
	    /* we found the bad word, now determine which simm */
	    break;
	}
	
	/* if none of the words checked found an error, the error must */
	/* have been a transient parity error. */
	if (!parityerr) {
	    unsigned int mer;

	    type = TRANSPAR;
	    mer = *(u_int *)(PHYS_TO_K1(KN02BA_MEM_ERR));
	    mer &= LAST_BYTE_ERR_MASK;
	    if (mer & 0x800) {
		byte = 1;
		low = 0;
	    } else if (mer & 0x400) {
		byte = 0;
		low = 0;
	    } else if (mer & 0x200) {
		byte = 1;
		low = 1;
	    } else {
		byte = 0;
		low = 1;
	    }
	    /* clear error bits */
	    *(u_int *)(PHYS_TO_K1(KN02BA_MEM_ERR)) = 0;
	    baddr = (char *)blockaddr;
	}

	if (*((u_int *)PHYS_TO_K1(KN02BA_MEM_SIZE)) & KN02BA_16MB_MEM) 
	    banksize = 16 * 1024 * 1024;
	else
	    banksize = 4 * 1024 * 1024;
	
	/* There are 8 banks, numbered 0 - 7 */
	
	bank = (int)svtophy(baddr) / banksize;
	
	/* There are 16 simms, numbered 0 - 15 */
	if (low)
	    simm = (bank * 2);
	else
	    simm = (bank * 2) + 1;
	/*
	 * Increment error counts
	 */
	switch (type) {
	  case TRANSPAR:
	  default:
	    kn02ba_tcount[simm]++;
	    if (kn02ba_tcount[simm] > 255) {
		mprintf("Transient parity error count on simm in BANK # %d, %s reached 255, reset to zero.\n", bank, (low ? "D0-15" : "D16-31"));
		kn02ba_tcount[simm] = 0;
	    }
	    break;
	  case SOFTPAR:
	    kn02ba_scount[simm]++;
	    break;
	  case HARDPAR:
	    kn02ba_hcount[simm]++;
	    break;
	}
	memreg = MEMREGFMT(simm, type, byte, dp, kn02ba_tcount[simm], 
			   kn02ba_scount[simm], kn02ba_hcount[simm]);
	return(memreg);
}

#ifdef ERRLOG_DEBUG

#define NO_TEST		0
#define HARD_PAR_TEST	1
#define SOFT_PAR_TEST	2
#define TRANS_PAR_TEST	3
#define WTO_TEST	4
#define TRAP_TEST	5
#define UNALIGNED_TEST	6
#define RTMO_TEST	7

int kn02ba_errlog; 
u_int dummy;


kn02ba_errlog_testing()
{
    u_char *addr;
    int	i, cnt;

    switch (kn02ba_errlog) {
      case NO_TEST:
	break;
      case HARD_PAR_TEST:
	cprintf("value of MER = 0x%x\n", *(u_int *)(PHYS_TO_K1(KN02BA_MEM_ERR)));
	cprintf("executing HARD_PAR_TEST (cached)\n");
	/* read all of the last 4 meg trying to find 	*/
	/* bad memory					*/
	addr = (u_char *)(0x81c00000);
	for (i = 0; i < 0x400000; i++, addr++) {
	    *addr = 0x0;
	    dummy = *addr;
	    if (dummy != 0x0) cprintf("dummy (%d) != 0x0, addr = 0x%x\n", dummy, addr);
	    *addr = 0xff;
	    dummy = *addr;
	    if (dummy != 0xff) cprintf("dummy (%d) != 0xff, addr = 0x%x\n", dummy, addr);
	    *addr = 0x1;
	    dummy = *addr;
	    if (dummy != 0x1) cprintf("dummy (%d) != 0x1, addr = 0x%x\n", dummy, addr);
	    *addr = 0xaa;
	    dummy = *addr;
	    if (dummy != 0xaa) cprintf("dummy (%d) != 0xaa, addr = 0x%x\n", dummy, addr);
	    *addr = 0x55;
	    dummy = *addr;
	    if (dummy != 0x55) cprintf("dummy (%d) != 0x55, addr = 0x%x\n", dummy, addr);
	}
	cprintf("executing HARD_PAR_TEST (uncached)\n");
	addr = (u_char *)(0xa1c00000);
	for (i = 0; i < 0x400000; i++, addr++) {
	    *addr = 0x0;
	    dummy = *addr;
	    if (dummy != 0x0) cprintf("dummy (%d) != 0x0, addr = 0x%x\n", dummy, addr);
	    *addr = 0xff;
	    dummy = *addr;
	    if (dummy != 0xff) cprintf("dummy (%d) != 0xff, addr = 0x%x\n", dummy, addr);
	    *addr = 0x1;
	    dummy = *addr;
	    if (dummy != 0x1) cprintf("dummy (%d) != 0x1, addr = 0x%x\n", dummy, addr);
	    *addr = 0xaa;
	    dummy = *addr;
	    if (dummy != 0xaa) cprintf("dummy (%d) != 0xaa, addr = 0x%x\n", dummy, addr);
	    *addr = 0x55;
	    dummy = *addr;
	    if (dummy != 0x55) cprintf("dummy (%d) != 0x55, addr = 0x%x\n", dummy, addr);
	}
	cprintf("executing HARD_PAR_TEST (addr bit)\n");
	addr = (u_char *)(0xa1c00000);
	cnt = 0;
	while (cnt < 0x400000) {
	    for (i = 0; ((i < 254) && (cnt < 0x400000)); i++, cnt++)
		*addr++ = i;
	}
	addr = (u_char *)(0xa1c00000);
        cnt = 0;
	while (cnt < 0x400000) {
	    for (i = 0; ((i < 254) && (cnt < 0x400000)); i++, cnt++) {
		dummy = *addr++;
		if (dummy != i) {
		    cprintf("dummy (%x) != i (%x), addr = %x\n", dummy, i, addr);
		}
	    }
	}

	cprintf("done\n");
	break;
      case SOFT_PAR_TEST:
	break;
      case TRANS_PAR_TEST:
	printf("isolatepar returns %x\n", kn02ba_isolatepar(0x010205f8, 0, 4));
	break;
      case WTO_TEST:
	cprintf("executing WTO_TEST\n");
	/* write to turbo slot 0 */
	*(u_int *)(0xb0000000) = 0;
	break;
      case TRAP_TEST:
	cprintf("executing TRAP_TEST\n");
	dummy = *(u_int *) 0;
	break;
      case UNALIGNED_TEST:
	cprintf("executing UNALIGNED_TEST\n");
	dummy = *(u_int *)(0x80000002);
	break;
      case RTMO_TEST:
	cprintf("executing RTMO_TEST\n");
	dummy = *(u_int *)(0xb0000000);
	break;

      default:
	break;
    }

    kn02ba_errlog = 0;

    timeout(kn02ba_errlog_testing, (caddr_t) 0, 5 * hz);
}
#endif /* ERRLOG_DEBUG */

kn02ba_conf_clk_speed()
{
    register volatile struct rt_clock *rt =(struct rt_clock *)rt_clock_addr;
    register volatile int dummy;
    register int s, counter = 0;
    int save_rega, save_regb;

    s = splextreme();

    /* allow TOY interrupt to get to the CPU */
    *(u_int *)(PHYS_TO_K1(KN02BA_SIRM_ADDR)) = TOY_INTR;

    /* enable periodic interrupt */
    save_rega = rt->rt_rega;
    save_regb = rt->rt_regb;
    rt->rt_rega = RTA_DV32K|RTA_4ms;
    rt->rt_regb = RTB_DMBINARY|RTB_24HR|RTB_PIE;

    /* clear any old interrupts */
    dummy = rt->rt_regc;

    /* wait for start */
    while ((get_cause() & SR_IBIT6) == 0);

    dummy = rt->rt_regc;

    /* wait for finish and count */
    while ((get_cause() & SR_IBIT6) == 0)
	counter++;

    dummy = rt->rt_regc;
    rt->rt_rega = save_rega;
    rt->rt_regb = save_regb;

    splx(s);
    return (counter);
}
