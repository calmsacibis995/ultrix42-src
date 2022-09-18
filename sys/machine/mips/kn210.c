#ifndef lint
static char *sccsid = "@(#)kn210.c	4.2      (ULTRIX)  8/9/90";
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
 * Revision History:
 *
 * 04-Aug-90	Randall Brown
 *	Added call to spl_init() to intialize spl function pointers and
 *      the intr() function pointer.
 *
 * 10-July-89	burns
 *	Added kn210 init routine to set up cpu specific items that are
 *	required very early on.
 *
 * 13-Jan-89	Kong
 *	Created the file.  This file contains routines specific
 *	to Mipsfair systems (KN210).
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/types.h"
#include "../h/user.h"			/* gets time.h and debug.h too */
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/errlog.h"
#include "../h/cmap.h"
#include "../machine/cpu.h"
#include "../machine/ssc.h"
#include "../machine/hwconf.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/reg.h"
#include "../machine/kn210.h"
#include "../io/uba/ubavar.h"

#define ESRPKT 1
#define MEMPKT 2
#define SHAREDPG(pt) (((pt) & CSMEM) || ((pt) & CTEXT))


unsigned int kn210_memcon;	/* memory config (memcsr's 0-15) */

extern	struct qbm_regs *qb_ptr;	/* Points to unibus adaptor regs */

struct esr {
	u_long esr_wear;	/* Write Error Address Reg */
	u_long esr_dser;	/* DMA System Error Reg */
	u_long esr_qbear;	/* QBus Error Address Reg */
	u_long esr_dear;	/* DMA Error Address Reg */
	u_long esr_cbtcr;	/* CDAL Bus Timeout Control Reg */
	u_long esr_isr;		/* Interrupt Status Reg	*/
} esr[1];

extern u_int printstate;
extern int nNMSI, nummsi, qmapbase;
extern int softclock(),softnet(),kn210hardintr0(),kn210hardintr1(),
	   kn210hardintr2(),kn210harderrintr(),kn210haltintr(),fpuintr();

/* The routines that correspond to each of the 8 interrupt lines */
int (*kn210intr_vec[8])() = {
	softclock,		/* softint 1	*/
	softnet,		/* softint 2	*/
	kn210hardintr0,		/* hardint 1	*/
	kn210hardintr1,		/* hardint 2	*/
	kn210hardintr2,		/* hardint 3	*/
	kn210harderrintr,	/* hardint 4	*/
	kn210haltintr,		/* hardint 5	*/
	fpuintr			/* hardint 6	*/
};

/* The masks to use to look at each of the 8 interrupt lines */
int kn210iplmask[8] = {
	SR_IMASK1|SR_IEC,
	SR_IMASK2|SR_IEC,
	SR_IMASK3|SR_IEC,
	SR_IMASK4|SR_IEC,
	SR_IMASK5|SR_IEC,
	SR_IMASK6|SR_IEC,
	SR_IMASK7|SR_IEC,
	SR_IMASK8|SR_IEC
};

/* The status register masks for splxxx usage */
#define SR_HALT	0x4000			/* Halt interrupt */
int kn210splm[SPLMSIZE] = {
	SR_IEC | SR_IMASK0 | SR_HALT,	/* 0 SPLNONE			*/
	SR_IEC | SR_IMASK1 | SR_HALT,	/* 1 SPLSOFTC			*/
	SR_IEC | SR_IMASK2 | SR_HALT,	/* 2 SPLNET			*/
	0,				/* 3 NOT_USED 			*/
	0,				/* 4 NOT_USED 			*/
	SR_IEC | SR_IMASK4 | SR_HALT,	/* 5 SPLBIO, SPLIMP, SPLTTY	*/
	SR_IEC | SR_IMASK5 | SR_HALT,	/* 6 SPLCLOCK			*/
	SR_IEC | SR_IMASK6 | SR_HALT,	/* 7 SPLMEM			*/
	SR_IEC | SR_IMASK8 | SR_HALT,	/* 8 SPLFPU			*/
};

/*
 * Configuration routine.
 */
kn210conf()
{
	extern unsigned cpu_systype;
	extern (*(scb[]))();		/* 1st page of scb vectors */
	extern int cold;

	struct memcsr *memcsrptr;	/* Pointer to memory csrs */
	register unsigned int *mp;
	register int i;
	extern cnrint(),cnxint();

	cold = 1;			/* Set boot flag */

	/*
	 * Fill in the vector in SCB for handling console interrupts.
	 *
	 */
	scb[0xf8/4] = cnrint;	/* general console receive  interrupt */
	scb[0xfc/4] = cnxint;	/* general console transmit interrupt */

	printf("DECsystem 5400 - system rev %d\n", GETHRDREV(cpu_systype));

	/*
	 * Read memory CSRs to determine configuration.
	 */
	printf("Main memory...\n");
	memcsrptr = (struct memcsr *)MEMCSR;/* Get address of memory csrs */
	mp = (unsigned int *)MEMCSR;	/* Get address of memory csrs */
	for (i = 31; i > 15; i--, mp++) {
		kn210_memcon |= ((*mp & MEM_BNKENBLE) >> i);
	}
	kn210_memcon |= ((memcsrptr->memcsr0 & MEM_BNKUSAGE) << 16);
	kn210_memcon |= ((memcsrptr->memcsr4 & MEM_BNKUSAGE) << 18);
	kn210_memcon |= ((memcsrptr->memcsr8 & MEM_BNKUSAGE) << 20);
	kn210_memcon |= ((memcsrptr->memcsr12 & MEM_BNKUSAGE) << 22);
	if ((memcsrptr->memcsr0 & MEM_BNKUSAGE)==0x3) {
		printf("Board 0 (MS650-BA) ");
		if ((kn210_memcon & 0xf) == 0xf) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}
	else if ((memcsrptr->memcsr0 & MEM_BNKUSAGE)==0x2) {
		printf("Board 0 (MS650-AA) ");
		if ((kn210_memcon & 0xf) == 0xf) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}

	if ((memcsrptr->memcsr4 & MEM_BNKUSAGE)==0x3) {
		printf("Board 1 (MS650-BA) ");
		if ((kn210_memcon & 0xf0) == 0xf0) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}
	else if ((memcsrptr->memcsr4 & MEM_BNKUSAGE)==0x2) {
		printf("Board 1 (MS650-AA) ");
		if ((kn210_memcon & 0xf0) == 0xf0) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}

	if ((memcsrptr->memcsr8 & MEM_BNKUSAGE)==0x3) {
		printf("Board 2 (MS650-BA) ");
		if ((kn210_memcon & 0xf00) == 0xf00) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}
	else if ((memcsrptr->memcsr8 & MEM_BNKUSAGE)==0x2) {
		printf("Board 2 (MS650-AA) ");
		if ((kn210_memcon & 0xf00) == 0xf00) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}

	if ((memcsrptr->memcsr12 & MEM_BNKUSAGE)==0x3) {
		printf("Board 3 (MS650-BA) ");
		if ((kn210_memcon & 0xf000) == 0xf000) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}
	else if ((memcsrptr->memcsr12 & MEM_BNKUSAGE)==0x2) {
		printf("Board 3 (MS650-AA) ");
		if ((kn210_memcon & 0xf000) == 0xf000) 
			printf("OK.\n");
		else
			printf("not all banks enabled.\n");
	}

	coproc_find();		/* Deal with FPU */

	/*
	 * Configure all the devices.  I/O devices include
	 * an on-board Ethernet interface, an on-board DSSI interface, 
	 * and a Q22 Bus.
	 *
	 * On board devices sit on a virtual bus called the "ibus".
	 */
	splnone();

	config_set_alive("ibus",0,0,0);

	/* Configure on-board ethernet */
	ib_config_dev(PHYS_TO_K1(KN210LANCE_ADDR),
		KN210LANCE_ADDR,0,"ln",(int)scb+LANCE_OFFSET);
	
	/* Configure on-board DSSI */
	if (nNMSI) {
		extern int (*msiintv[])();
		scb[0xc4/4] = msiintv[nummsi];	/* Stuff the SCB */
		if (msi_probe(nummsi,PHYS_TO_K1(KN210MSIREG_ADDR),
			PHYS_TO_K1(KN210SIIBUF_ADDR))) {
			config_set_alive("msi",0,0,0);
			printf("msi%d\n",nummsi);
			}
	}

	/* Configure Q22 bus */
	printf("Q22 bus\n");
	uba_hd[0].uba_type = UBAUVII;
	/* Set up the Qbus map base register */
	*(int *)PHYS_TO_K1(KN210QMAPBASEREG) = qmapbase;

	/*
	 * This call to unifind uses mapped address space (KSEG2)
	 * to touch the Q bus csrs.  It also initialises the i/o
	 * data structures to use KSEG2 to access the csrs.
	 */
	unifind(qb_ptr,0x10087800,qmem,
		0x14000000,512*8192,0x10000000,QMEMmap,0,0,0);

	cold = 0;		/* Clear boot flag */
	return(0);
}
/*
 * Initialization routine for kn210 processor (MIPSFAIR).
 */
kn210init()
{
	extern int c0vec_tblsize;
	extern int (*c0vec_tbl[])();
	extern int iplmask[];
	extern int splm[];
	extern int hz;
	extern int tick;
	extern int tickadj;
	extern struct cpusw *cpup;

	qb_ptr = (struct qbm_regs *)(PHYS_TO_K1(KN210QBUSREG)); /* Points to unibus adaptor regs */
	/*
	 * Initialize the interrupt dispatch table c0vec_tbl.
	 * Initialize the spl table for the system we are running on.
	 * Initialize the interrupt masks for the system we are running on.
	 * This must be done very early on at boot time and must
	 * be done before any interrupt is allowed.
	 */
	bcopy((int *)kn210intr_vec, c0vec_tbl, c0vec_tblsize);
	bcopy(kn210iplmask, iplmask, IPLSIZE * sizeof(int));
	bcopy(kn210splm, splm, (SPLMSIZE) * sizeof(int));

	/* Initialize the spl dispatch table and the intr dispatch routine */
	spl_init();

	/*
	 * Set up the system specific value for "hz", the number of clock
	 * interrupts per second; and corresponding tick and tickadj values.
	 */
	hz = cpup->HZ;
	tick = 1000000 / hz;
	tickadj = 240000 / (60 * hz);
	return (0);
}


/*
 * Routine to start the 10mS hardclock running.
 * This is simply to start up the 10mS interval timer so
 * that it interrupts every 10mS.  The interval timer
 * on KN210 interrupts the R3000 cpu at hardware line 2 (0 being
 * lowest priority, 5 being highest).  When the interrupt handler
 * reads the vector, the interval timer returns a vector of 0xc0.
 */
kn210startrtclock()
{
int *tcsr;
	tcsr = (int *)TCSR;
	*tcsr = TCSR_IE;
	return(0);
}

/*
 * Routine to stop the 10mS hardclock from interrupting.
 */
kn210stopclocks()
{
int *tcsr;
	tcsr = (int *)TCSR;
	*tcsr = 0;
	return(0);
}


/*
 * A halt condition is received.  This is an interrupt
 * handler.
 *
 * ISR<3> is set, need to clear it.  The rom routine is
 * supposed to clear it.
 */
kn210haltintr(ep)
int *ep;
{
prom_halt(ep);
}

/*
 * Hard error interrupt.  This can be of three conditions:
 *	1. Powerfail interrupt.
 *	2. Memory error by CQBIC or CMCTL.
 *	3. Write error, address contained in WEAR register is valid.
 *
 * The MIPSFAIR special register in physical address 0x10084000
 * can be read to determine which error condition occurred.  Format
 * of ISR:
 *	bit<2> set if powerfail,
 *	bit<1> set if CQBIC or CMCTL error,
 *	bit<0> set if write error.
 *
 * The ISR must have the proper bit cleared to enable subsequent interrupts.
 *
 * Note that multiple bits can be set.  I for now arbitrarily handle
 * the error with the following priority (from high to low):
 *	write error, CQBIC or CMCTL error, powerfail.
 *
 * Parameter:
 *	ep		Pointer to exception frame.
 *
 */
kn210harderrintr(ep)
int	*ep;
{
int	isr;	/* Content of ISR */
u_long	pa;	/* Physical address */
u_long	dser;

	isr = *(int *)ISR;	/* Read content of ISR */

	/*
	 * Log DMA system error register, error address register, ..etc.
	 */
	esr[0].esr_wear = * (u_long *)KN210WEAR;
	esr[0].esr_dser = * (u_long *)KN210DSER;
	esr[0].esr_qbear = * (u_long *)KN210QBEAR;
	esr[0].esr_dear = * (u_long *)KN210DEAR;
	esr[0].esr_cbtcr = * (u_long *)KN210CBTCR;
	esr[0].esr_isr = isr;

	/* Clear the error bits */
	*(u_long *)KN210DSER = 0xc0bd;
	*(u_long *)KN210CBTCR = 0xc0000000 | esr[0].esr_cbtcr;

	*(int *)ISR = 0;	/* Acknowledge all error interrupts */

	if (isr & ISR_WEAR) {		/* Write error interrupt */
		/* physical address is in WEAR */		
		pa = esr[0].esr_wear;
		if (btop((int)pa) < physmem) {
			/* was writing to main memory */
			kn210logmempkt(EL_PRISEVERE, ep, pa);
			kn210consprint(MEMPKT, ep, pa, 0);
			panic("Memory write error");
		}
		else	{
			kn210logesrpkt(ep, esr, EL_PRISEVERE);
			kn210consprint(ESRPKT, ep, pa, 0);
			panic("Bus write error");
		}
	}

	if (isr & ISR_CERR) {		/* CQBIC or CMCTL error interrupt */
		/* compute physical address */
		dser = esr[0].esr_dser;
		if (dser & 0xa0) {
			/*
			 * Q22 bus write cycle timeout after 10uS,
			 * or Q22 bus parity error.
			 * QBEAR contains Qbus physical address.
			 */
			pa = esr[0].esr_qbear << 9;
			kn210logesrpkt(ep, esr, EL_PRISEVERE);
			kn210consprint(ESRPKT, ep, pa, 1);
			panic("Q22 bus error");
		}
		else if (dser & 0x11) {
			/*
			 * DMA transfer to non-existent main memory location,
			 * or main memory error.
			 * DEAR contains physical address.
			 */
			pa = esr[0].esr_dear << 9;
			/* was writing to main memory */
			kn210logmempkt(EL_PRISEVERE, ep, pa);
			kn210consprint(MEMPKT, ep, pa, 0);
			panic("Memory error");
		}
	}

	if (isr & ISR_PWF) {		/* Powerfail interrupt */
		powerfail_intr(ep);
	}

}

/*
 * Pending powerfail, currently ULTRIX has no handling
 * strategy on VAX and MIPS.  Further, there is no restart
 * parameter block on MIPS.  
 *
 */
powerfail_intr(ep)
int *ep;
{
	printf("Power fail..\n");
	DELAY(1000000);	/* Delay 1 second, if we are still alive, continue */
}

/*
 * Interrupts from Qbus level 7 or 10mS clock.  Level 7 is reserved for
 * Qbus real-time devices.  Currently not used in ULTRIX.
 */
kn210hardintr2(ep)
int *ep;
{
extern int (*(scb[]))();
register int vrr;

	vrr = read_nofault(VRR3);	/* Read interrupt vector */
					/* 0 = passive release	*/
	vrr &= 0xffff;
	(*(scb[vrr/4]))(ep,vrr);	/* Call the interrupt handler 	*/
}

/*
 * Interrupts from NI, DSSI, or Qbus level 6.
 */
kn210hardintr1(ep)
int *ep;
{
extern int (*(scb[]))();
register int vrr;

	vrr = read_nofault(VRR2);	/* Read interrupt vector */
					/* 0 = passive release	*/
	vrr &= 0xffff;
	(*(scb[vrr/4]))(ep,vrr);	/* Call the interrupt handler 	*/
}


/*
 * Interrupts from the Qbus level 5 and 4,
 * and from the SSC chip (programmable timers, console).
 *
 * VRR0 contains vector for Qbus level 4 & 5, don't read VRR1 for
 * reason I don't understand.
 */
kn210hardintr0(ep)
int *ep;
{
extern int (*(scb[]))();
register int vrr;

	vrr = read_nofault(VRR0);	/* Read interrupt vector */
					/* 0 = passive release	*/
	vrr &= 0xffff;
	(*(scb[vrr/4]))(ep,vrr);	/* Call the interrupt handler 	*/
}



/*
 * Routine to handle trap errors: user-mode ibe & dbe, & all kernel mode traps.
 * We try to recover from user-mode errors and panic on kernel mode errors.
 */
kn210trap_error(ep, code, sr, cause, signo)
register u_int *ep;		/* exception frame ptr */
register u_int code;		/* trap code (trap type) */
u_int sr, cause;		/* status and cause regs */
int *signo;			/* set if we want to kill process */
{
	caddr_t pa;			/* the physical addr of the error */	
	int epc;			/* the EPC of the error */	
	int pagetype;          		/* type of page */
	int vaddr;			/* virt addr of error */
	register struct proc *p;	/* ptr to current proc struct */
	
	/*
	 * Log DMA system error register, error address register, ..etc.
	 */
	esr[0].esr_wear = * (u_long *)KN210WEAR;
	esr[0].esr_dser = * (u_long *)KN210DSER;
	esr[0].esr_qbear = * (u_long *)KN210QBEAR;
	esr[0].esr_dear = * (u_long *)KN210DEAR;
	esr[0].esr_cbtcr = * (u_long *)KN210CBTCR;

	/* Clear the error bits */
	*(u_long *)KN210DSER = 0xc0bd;
	*(u_long *)KN210CBTCR = 0xc0000000 | esr[0].esr_cbtcr;

	p = u.u_procp;
	if (USERMODE(sr)) {
		/*
		 * It is a memory read error:
		 *    a) on a private process page, terminate the process
		 *	 (by setting signo = SIGBUS)
		 *    b) on a shared page, crash the system.
		 * TBD: on a non-modified page, re-read the page (page fault),
		 *	and continue the process.
		 * TBD: on a shared page terminate all proc's sharing the page,
		 *	instead of crash system.
		 * TBD: on hard errors map out the page.
		 */
		pa = (caddr_t)vatophys(ep[EF_BADVADDR]);
		if ( (int)pa != -1 && (btop((int)pa) < physmem) ) {
			pagetype = cmap[pgtocm(btop(pa))].c_type;
			if (SHAREDPG(pagetype)) {
				kn210logmempkt(EL_PRISEVERE, ep,pa);
				kn210consprint(MEMPKT, ep, pa, 0);
				panic("memory read error in shared page");
			} else {
				kn210logmempkt(EL_PRIHIGH, ep, pa);
				printf("pid %d (%s) was killed on memory read error\n",
					p->p_pid, u.u_comm);
				uprintf("pid %d (%s) was killed on memory read error\n",
					p->p_pid, u.u_comm);
			}
		} else {
			uprintf("pid %d (%s) was killed on bus read error\n",
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
			pa = (caddr_t)vatophys(ep[EF_BADVADDR]);
			if ( (int)pa != -1 && (btop((int)pa) < physmem) ) {
				kn210logmempkt(EL_PRISEVERE, ep, pa);
				kn210consprint(MEMPKT, ep, pa, 0);
				panic("memory read error in kernel mode");
			} else {
				kn210logesrpkt(ep, esr, EL_PRISEVERE);
				kn210consprint(ESRPKT, ep, 0, 0);
				panic("read bus timeout");
			}
			break;
		case EXC_CPU:
			kn210logesrpkt(ep, esr, EL_PRISEVERE);
			kn210consprint(ESRPKT, ep, 0, 0);
			panic("coprocessor unusable");
			break;
		case EXC_RADE:
		case EXC_WADE:
			kn210logesrpkt(ep, esr, EL_PRISEVERE);
			kn210consprint(ESRPKT, ep, 0, 0);
			panic("unaligned access");
			break;
		default:
			kn210logesrpkt(ep, esr, EL_PRISEVERE);
			kn210consprint(ESRPKT, ep, 0, 0);
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


/*
 * Log Error & Status Registers to the error log buffer.
 */
kn210logesrpkt(ep, ptr, priority)
register u_int *ep;	/* exception frame ptr */
struct esr *ptr;	/* pointer to esr */
int priority;		/* for pkt priority */
{
	struct el_rec *elrp;

	elrp = ealloc(sizeof(struct el_esr), priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_ESR,ELESR_5400,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		elrp->el_body.elesr.elesr.el_esr5400.esr_cause = ep[EF_CAUSE];
		elrp->el_body.elesr.elesr.el_esr5400.esr_epc = ep[EF_EPC];
		elrp->el_body.elesr.elesr.el_esr5400.esr_status = ep[EF_SR];
		elrp->el_body.elesr.elesr.el_esr5400.esr_badva = ep[EF_BADVADDR];
		elrp->el_body.elesr.elesr.el_esr5400.esr_sp = ep[EF_SP];
		elrp->el_body.elesr.elesr.el_esr5400.esr_wear = ptr->esr_wear;
		elrp->el_body.elesr.elesr.el_esr5400.esr_dser = ptr->esr_dser;
		elrp->el_body.elesr.elesr.el_esr5400.esr_qbear = ptr->esr_qbear;
		elrp->el_body.elesr.elesr.el_esr5400.esr_dear = ptr->esr_dear;
		elrp->el_body.elesr.elesr.el_esr5400.esr_cbtcr = ptr->esr_cbtcr;
		elrp->el_body.elesr.elesr.el_esr5400.esr_isr = ptr->esr_isr;
		EVALID(elrp);
	}
}

/*
 * Log a memory error packet, so uerf can find it as a main memory error.
 * Determine the type of memory error by reading MEMCSR16.
 *
 * Side effect: clear error bits in MEMCSR16.
 */
kn210logmempkt(priority, ep, pa)
int priority;		/* pkt priority: panic: severe; else: high */
register u_int *ep;	/* exception frame ptr */
int pa;			/* physical addr where memory err occured */
{
	struct el_rec *elrp;
	register struct el_mem *mrp;
	struct memcsr *memcsrptr = (struct memcsr *)MEMCSR;/* Pointer to memory csr */

	elrp = ealloc(EL_MEMSIZE, priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_5400,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		mrp = &elrp->el_body.elmem;
		mrp->elmem_cnt = 1;
		mrp->elmemerr.cntl = 1;
		
		mrp->elmemerr.numerr = 1;
		mrp->elmemerr.regs[0] = memcsrptr->memcsr16;
		if (mrp->elmemerr.regs[0] & 0xc0000000)
			mrp->elmemerr.type = ELMETYP_RDS;
		else if (mrp->elmemerr.regs[0] & 0x20000000)
			mrp->elmemerr.type = ELMETYP_CRD;
		else if (mrp->elmemerr.regs[0] & 0x100)
			mrp->elmemerr.type = ELMETYP_CNTL;
		else	
			mrp->elmemerr.type = 0;
		memcsrptr->memcsr16 = mrp->elmemerr.regs[0]; /* Clear errors*/
		mrp->elmemerr.regs[1] = pa;
		mrp->elmemerr.regs[2] = ep[EF_EPC];;
		mrp->elmemerr.regs[3] = ep[EF_BADVADDR];;
		EVALID(elrp);
	}
}

/*
 * Print error packet to the console.
 * This is only done when we are about to panic on the error.
 *
 * Note: side-effect.
 *	If console is a graphics device, printstate is changed  to force
 *	kernel printfs directly to the screen.
 */
kn210consprint(pkt, ep, pa, qbus)
int pkt;		/* error pkt: Error & Stat Regs / memory pkt */
register u_int *ep;	/* exception frame ptr */
unsigned pa;		/* For MEMPKT: physical addr of error */	
int	qbus;		/* 1 if pa is a qbus address, 0 otherwise */
{

	/*
	 * If console is a graphics device,
	 * force printf messages directly to screen.
	 * Note: DS_5400 currently does not support qdss, 
	 * but just in case...
	 */
        printstate |= PANICPRINT;
	switch (pkt) {
	case ESRPKT:
		cprintf("\nException condition\n");
		break;
	case MEMPKT:
		cprintf("\nMemory Error\n");
		break;
	default:
		cprintf("bad consprint\n");
		break;
	}
	cprintf("\tCause reg\t= 0x%x\n", ep[EF_CAUSE]);
	cprintf("\tException PC\t= 0x%x\n", ep[EF_EPC]);
	cprintf("\tStatus reg\t= 0x%x\n", ep[EF_SR]);
	cprintf("\tBad virt addr\t= 0x%x\n", ep[EF_BADVADDR]);

	if (pa) {
		if (qbus) {
			cprintf("\tQ22 Bus physical address of error: 0x%x\n",
			pa);
		}
		else {
			cprintf("\tPhysical address of error: 0x%x\n",pa);
		}
	}
	return;
}

