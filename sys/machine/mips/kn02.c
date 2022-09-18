#ifndef lint
static char *sccsid = "@(#)kn02.c	4.8      (ULTRIX)  4/11/91";
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
 * 11-Apr-91    Paul Grist
 *	Modified trap routine to account for NVRAM if it is present,
 *	and handle errors on NVRAM appropriately.
 *
 * 07-Mar-91	Randall Brown
 *	Fixed a bug in kn02conf().  Must have a return value.
 *
 * 07-Feb-91	Paul Grist
 *	Added latent support for Prestoserve on 3max platform.
 *
 * 13-Sep-90    szczypek
 *	Added support for new 3max TURBOchannel console callbacks.  Call
 *	rex_halt() if new ROM present.
 *
 * 07-Sep-90	sekhar
 *	fixed kn02consprint() to set the pkt type field.
 *	fixed to log an esr pkt on write timeout only when panicing.
 *
 * 06-Sep-90	Randall Brown 
 *	Changed slot_order to config_order. 
 *
 * 13-Aug-90	sekhar
 *	changes to support memory mapped devices:
 *	added kn02consinfo_t to capture error information.
 *	added a new function kn02_print_consinfo() exported through cpusw.
 *	modified bus timeout code to capture error information and 
 *	post softnet() interrupt.
 *	Also integrated kn02_print_consinfo and kn02consprint to avoid 
 *	duplication of code.
 *
 * 03-Aug-90	Randall Brown
 *	Added call to spl_init() to intialize spl function pointers and
 *      the intr() function pointer.
 *
 * 26-Jan-90	Randall Brown
 *	Moved support of the TURBOchannel to io/tc/tc.c
 *
 * 29-Dec-89	afd
 *	Added 2 new fields to kn02_ioslot data struct, for when to enable
 *	the interrupt line for devices.  This info comes out of the
 *	maxoption data table.
 *
 *	Added module name (from ROM) to kn02_ioslot struct.
 *
 *	Changed kn02trap_error to use soft copies of "erradr" and "chksyn".
 *
 *	Add adapter config code.
 *
 * 02-Dec-89 -- afd
 *	Set fixtick to the number of microsec's that the clock looses per
 *	second.  This is used in hardclock() to fix the time.
 *
 * 30-Oct-89	afd
 *	Added error handling code.  Added kn02where_option routine
 *	    so consinit can figure out if there is a cfb module.
 *	Shift phys addr from erradr reg in consprint.
 *	Catch repeat bus error when dumping after an ECC error.
 *
 * 13-Oct-89	gmm
 * 	Moved the position for including cpu.h. Needed after smp changes
 *
 * 29-Sept-89 afd
 *     First boot level: a few bug fixes, several clean-ups.
 *
 * 15-Sept-89	afd
 *	Set up the order in which to configure the IO slots in "config_order".
 *	This determines how the unit numbers are asigned.
 *
 *	Added the kn02ie_mask word so we can enable just the IO slots that
 *	configured properly.
 *
 * 12-Sept-89	afd
 *	Created this file for kn02 (3max) support.
 */
#include "../machine/pte.h"
#include "../machine/cpu.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/user.h"			/* gets time.h and debug.h too */
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/errlog.h"
#include "../h/cmap.h"
#include "../h/config.h"
#include "../h/presto.h"

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/tc/tc.h"

#include "../machine/fpu.h"
#include "../machine/hwconf.h"
#include "../machine/reg.h"
#include "../machine/mc146818clock.h"
#include "../machine/entrypt.h"
#include "../../machine/common/cpuconf.h"

#define ESR_INTR_PKT 1			/* pkt types for consprint */
#define ESR_BUS_PKT 2
#define MEMPKT 3

#define KN02CSR_ADDR	0x1ff00000		/* CSR (incl IO intr bits) */
#define KN02IE_OFFSET	16			/* offset in CSR to IE bits */
#define KN02CSR_PSWARN	0x08000000		/* 32MB modules if set,else 8 */
#define KN02CSR_ECCMD	0x0000c000		/* ECC mode bits */
#define KN02CSR_ECCCOR	0x00002000		/* ECC correct bits */
#define KN02CSR_BNK32M	0x00000400		/* 32MB modules if set,else 8 */

#define KN02ERR_ADDR	0x1fd80000		/* Error register */
#define ERR_VALID	0x80000000		/* ERRADR valid bit */
#define ERR_TYPE	0x70000000		/* ERRADR error type bits */
#define ERR_WECC	0x70000000		 /* CPU partial mem write ECC */
#define ERR_WTMO	0x60000000		 /* CPU write timeout */
#define ERR_RECC	0x50000000		 /* CPU memory read ECC */
#define ERR_RTMO	0x40000000		 /* CPU read timeout */
#define ERR_DMAWOVR	0x20000000		 /* DMA write overrun */
#define ERR_DMARECC	0x10000000		 /* DMA memory read ECC */
#define ERR_DMAROVR	0x00000000		 /* DMA read overrun */
#define ERR_UKN		0x00000001		 /* unknown error */
#define ERR_ADDR	0x07ffffff		/* ERRADR error addr bits */
#define ERR_COLADDR	0x00000fff		/* ERRADR column addr bits */

#define KN02CHKSYN_ADDR	0x1fd00000		/* ECC check/syndrome reg */
#define CHKSYN_VLDLO	0x00008000		/* chksyn valid lo bit */
#define CHKSYN_SNGLO	0x00000080		/* chksyn lo-order single bit */
#define CHKSYN_VLDHI	0x80000000		/* chksyn valid hi bit */
#define CHKSYN_SNGHI	0x00800000		/* chksyn hi-order single bit */

/*
 * Bits in chksyn_plus (logged in memory error packet)
 * <31>		pc valid bit, set on bus error, cleared on intr error
 * <30:20>	error count for this module (max count for field is 2k)
 * <19:16>	module number in error (0 to 14)
 * <15:0>	half of chksyn that had valid error data
 */
#define CPLUS_VALID	0x80000000
#define CPLUS_EOFF	20
#define CPLUS_EMASK	0x7ff00000
#define CPLUS_MOFF	16
#define CPLUS_MMASK	0x000f0000
#define CPLUS_CHK	0x0000ffff
#define MAXERRCNT	2048

int kn02pswarn = 0;			/* set true if we had a ps-warning */
#define MEM_MODULES 15			/* numbered 0..MEM_MODULES-1 */
int kn02memerrs[MEM_MODULES] =		/* mem err count per module */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#define CRDINTVL (60*15)	/* time delta to enable CRD log - 15 minutes */
int kn02crdintvl = CRDINTVL;	/* global var so we can change it */
int kn02crdlog = 1;		/* is CRD logging currently enabled */

#define PSINTVL (60*1)		/* time delta to check pswarn bit */
int kn02psintvl = PSINTVL;	/* global var so we can change it */

struct crd_errcnt {             /* MEM_CRD */
        long   crd_last;	/* time of most recent CRD err */
        long   crd_prev;	/* time of previous CRD err */
} crd_errcnt = { 0, 0 };

int kn02eccpanic = 0;		/* set true if already panic'ing */
int kn02erradr;			/* software copy of hardware erradr */
int kn02chksyn;			/* software copy of hardware chksyn */

/*
 * Info for 3max I/O device "slots"
 */
#define OPTION_SLOTS 3

#define SLOT_0_ADDR 0x1e000000
#define SLOT_1_ADDR 0x1e400000
#define SLOT_2_ADDR 0x1e800000
#define SLOT_3_ADDR 0x0
#define SLOT_4_ADDR 0x0
#define SLOT_5_ADDR 0x1f400000
#define SLOT_6_ADDR 0x1f800000
#define SLOT_7_ADDR 0x1fc00000

#define	KN02DC_ADDR	0x1fe00000		/* phys addr of dc7085 (dz) */

#define	KN02CLOCK_ADDR	0x1fe80000		/* phys addr of clock chip */

u_int kn02_slotaddr[TC_IOSLOTS] = {SLOT_0_ADDR, SLOT_1_ADDR, SLOT_2_ADDR,
	SLOT_3_ADDR, SLOT_4_ADDR, SLOT_5_ADDR, SLOT_6_ADDR, SLOT_7_ADDR};

/*
 * Program the order in which to probe the IO slots on the system.
 * This determines the order in which we assign unit numbers to like devices.
 * It also determines how many slots (and what slot numbers) there are to probe.
 * Terminate the list with a -1.
 * Note: this agrees with the console's idea of unit numbers
 */
int kn02config_order [] = { 5, 6, 7, 0, 1, 2, -1 };

/*
 * We will set bits in this mask as we config the dev/ctlr in each slot.
 * So interrupts will only be enabled for a slot that configures ok.
 */
u_int kn02ie_mask = 0;

caddr_t vatophys();		/* function  typedef */
int kn02enable_option();
int kn02disable_option();
int kn02clear_errors();
int kn02crdenable();
int kn02pscheck();
extern int nofault;		/* to test for parity errors */
extern u_int printstate;	/* how to print to the console */
extern char *rt_clock_addr;	/* addr of the mc146818clock chip */
extern struct timeval time;	/* the system time */
extern int hz;
extern int tick;
extern int tickadj;
extern int fixtick;		/* set to the number of microsec's that the
				   clock looses per second */


/*
 * NVRAM/Prestoserve support definitions
 *
 */

#define KN02_NVRAM_DIAG		0x3f8	      /* offset to diag reg on nvram */
#define KN02_NVRAM_ID		0x3fc	      /* offset to NVRAM ID location */
#define KN02_NVRAM_IDENTIFIER   0x03021966    /* NVRAM board ID signature */
#define KN02_NVRAM_START	0x400	      /* offset to NVRAM cache addr */ 

#define KN02_NVRAM_RO      0x00000002         /* read only test failed */
#define KN02_NVRAM_RW      0x00000004         /* read/write test failed */
#define KN02_NVRAM_FAILED  0x00000008         /* MASK out fail bit */
#define NVRAM_SIZE         0x000000f0         /* MASK out nvram size bits */


#define KN02_NVRAM_BDISC    0x00000002        /* status reg mask for bdisc */
#define KN02_NVRAM_BOK      0x00000001        /* status reg mask for bok   */

volatile unsigned int kn02_nvram_csr;         /* status/control reg addr*/
volatile unsigned int kn02_nvram_diag;        /* diagnostic reg addr*/

int kn02_nvram_found = 0;		      /* nvram is present on system*/
short kn02_test = 0;
short kn02_nvram_debug = 0;
#define Dprintf  if(kn02_nvram_debug)printf

/*
 * Interrupt handlers for the 6 hardware interrupts and 2 software
 * interrupts for 3MAX.
 */
extern softclock(), softnet(), kn02iointr(), hardclock(), kn02stray(),
	kn02errintr(), kn02halt(), fpuintr();

/* The routines that correspond to each of the 8 interrupt lines */
int (*kn02intr_vec[IPLSIZE])() = {
	softclock,		/* softint 0	*/
	softnet,		/* softint 1	*/
	kn02iointr,		/* hardint 0	*/
	hardclock,		/* hardint 1	*/
	kn02stray,		/* hardint 2	*/
	kn02errintr,		/* hardint 3  	*/
	kn02stray,		/* hardint 4	*/
	fpuintr			/* hardint 5	*/
};

/* The masks to use to look at each of the 8 interrupt lines */
int kn02iplmask[IPLSIZE] = {
	SR_IMASK1|SR_IEC,
	SR_IMASK2|SR_IEC,
	SR_IMASK3|SR_IEC,
	SR_IMASK4|SR_IEC,
	SR_IMASK5|SR_IEC,
	SR_IMASK6|SR_IEC,
	SR_IMASK7|SR_IEC,
	SR_IMASK8|SR_IEC
};

/* The SR reg masks for splxxx usage */
int kn02splm[SPLMSIZE] = {
	SR_IEC | SR_IMASK0,		/* 0 SPLNONE			*/
	SR_IEC | SR_IMASK1,		/* 1 SPLSOFTC			*/
	SR_IEC | SR_IMASK2,		/* 2 SPLNET			*/
	0,				/* 3 NOT_USED 			*/
	0,				/* 4 NOT_USED 			*/
	SR_IEC | SR_IMASK3,		/* 5 SPLBIO, SPLIMP, SPLTTY	*/
	SR_IEC | SR_IMASK4,		/* 6 SPLCLOCK			*/
	SR_IEC | SR_IMASK6,		/* 7 SPLMEM			*/
	SR_IEC | SR_IMASK8,		/* 8 SPLFPU			*/
};

/*
 *	The structure kn02consinfo_t can be used to store information
 *	to be printed on a console. This information can then be printed
 *	by kn02_print_consinfo() which is exported  through the cpusw.
 *
 *	Currently this is used only by the bus timeout code(although it 
 *	is a general pupose mechanism).
 */
 
/* error and information to be dumped on a console on a ESR_INTR_PKT type */

struct kn02consinfo_intr_t {
	u_int	cause;	/* from the exception frame */
	u_int	sr;	/* from the exception frame */
	u_int 	sp;	/* from the exception frame */
	u_int	csr;
	u_int	erradr;
};

/* error information to be dumped on a console on a ESR_BUS_PKT type */

struct kn02consinfo_bus_t {
	u_int	cause;		/* from the exception frame */
	u_int	sr;		/* from the exception frame */
	u_int 	sp;		/* from the exception frame */
	u_int	epc; 		/* from the exception frame */
	u_int	badvaddr; 	/* from the exception frame */
	u_int	csr;
	u_int	erradr;
};

/* console information to be dumped on a console on a MEMPKT type */

struct kn02consinfo_mem_t {
	u_int	csr;
	u_int	erradr;
	u_int	chksyn_plus;
	u_int	epc;
};

struct kn02consinfo_t {
	int	pkt_type;		/* pkt type */
	union {
	    struct  kn02consinfo_intr_t	intrp;
	    struct  kn02consinfo_bus_t 	busp;
	    struct  kn02consinfo_mem_t 	memp;
	} pkt;
} kn02consinfo;

/*
 *	The structure kn02log_errinfo_t can be used to store information
 *	to be logged to the error log file. This information can be logged 
 *	by kn02_log_errinfo() which is exported  through the cpusw.
 *
 *	Currently this is used only by the bus timeout code(although it 
 *	is a general pupose mechanism).
 */

struct kn02log_errinfo_t {
	int	pkt_type;
	u_int	cause;		/* from the exception frame */
	u_int	sr;		/* from the exception frame */
	u_int 	sp;		/* from the exception frame */
	u_int	epc; 		/* from the exception frame */
	u_int	badvaddr; 	/* from the exception frame */
	u_int	csr;
	u_int	erradr;
} kn02log_errinfo;


/*
 * Initialization routine for kn02 processor (3max).
 */
kn02init()
{
	int i;
	extern int c0vec_tblsize;
	extern int (*c0vec_tbl[])();
	extern int iplmask[];
	extern int splm[];
	extern struct cpusw *cpup;

	/*
	 * Initialize the interrupt dispatch table c0vec_tbl.
	 * Initialize the spl table for the system we are running on.
	 * Initialize the interrupt masks for the system we are running on.
	 * This must be done very early on at boot time and must
	 * be done before any interrupt is allowed.
	 */
	bcopy((int *)kn02intr_vec, c0vec_tbl, c0vec_tblsize);
	bcopy(kn02iplmask, iplmask, IPLSIZE * sizeof(int));
	bcopy(kn02splm, splm, (SPLMSIZE) * sizeof(int));

	/* Initialize the spl dispatch table and the intr dispatch routine */
	spl_init();

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
	rt_clock_addr = (char *)PHYS_TO_K1(KN02CLOCK_ADDR);

	/*
	 * Clear the memory error counters.
	 */
	for (i = 0; i < MEM_MODULES; i++)
		kn02memerrs[i] = 0;
	
	/* Initialize the TURBOchannel */
	tc_init();

	/* Fill in the TURBOchannel slot addresses */
	for (i = 0; i < TC_IOSLOTS; i++)
	    tc_slotaddr[i] = kn02_slotaddr[i];

	/* Fill in the TURBOchannel switch table */
	tc_sw.enable_option = kn02enable_option;
	tc_sw.disable_option = kn02disable_option;
	tc_sw.clear_errors = kn02clear_errors;
	tc_sw.config_order = kn02config_order;

	/*
	 * Fixed 3max IO devices (slots 3 & 4 unused)
	 */
	tc_slot[3].slot = 3;
	tc_slot[4].slot = 4;

	strcpy(tc_slot[5].devname,"asc");
	strcpy(tc_slot[5].modulename, "PMAZ-AA ");
	tc_slot[5].slot = 5;
	tc_slot[5].module_width = 1;
	tc_slot[5].physaddr = SLOT_5_ADDR;
	tc_slot[5].intr_b4_probe = 0;
	tc_slot[5].intr_aft_attach = 1;
	tc_slot[5].adpt_config = 0;

	strcpy(tc_slot[6].devname,"ln");
	strcpy(tc_slot[6].modulename, "PMAD-AA ");
	tc_slot[6].slot = 6;
	tc_slot[6].module_width = 1;
	tc_slot[6].physaddr = SLOT_6_ADDR;
	tc_slot[6].intr_b4_probe = 0;
	tc_slot[6].intr_aft_attach = 1;
	tc_slot[6].adpt_config = 0;

	strcpy(tc_slot[7].devname,"dc");
	tc_slot[7].slot = 7;
	tc_slot[7].module_width = 1;
	tc_slot[7].physaddr = KN02DC_ADDR;
	tc_slot[7].intr_b4_probe = 0;
	tc_slot[7].intr_aft_attach = 1;
	tc_slot[7].adpt_config = 0;

	return(0);
}

/*
 * Configuration routine for kn02 processor (3max).
 */
kn02conf()
{
	extern int cold;
	extern u_int cpu_systype;

	cold = 1;

	/*
 	 * Initialize PROM environment entries.
	 */
	hwconf_init();

	/* 
	 * Report what system we are on
	 */
	printf("KN02 processor - system rev %d\n", (GETHRDREV(cpu_systype)));

	coproc_find();

	/* Turn off all interrupts in the CSR */
	*(u_int *)PHYS_TO_K1(KN02CSR_ADDR) &= (~0x00ff0000);
	wbflush();

	/*
	 * Probe the TURBOchannel and find all devices
	 */
	tc_find();

	timeout (kn02crdenable, (caddr_t) 0, kn02crdintvl * hz);
	timeout (kn02pscheck, (caddr_t) 0, kn02psintvl * hz);
	cold = 0;

	/*
	 * configure nvram option, this enables
	 * Prestoserve if the NVRAM is present.
	 *
	 * This will go out an probe for NVRAM.
	 *
	 */
	 kn02_config_nvram();

	return (0);	/* tell configure() we have configured correctly */
}

/*
 *	kn02clear_errors()
 *
 *	Clears any pending errors in the error register
 */
kn02clear_errors()
{
        *(u_int *)PHYS_TO_K1(KN02ERR_ADDR) = 0;
	wbflush();
}

/*
 *	kn02enable_option()
 *	
 *	Takes a slot number as an argument.
 *
 *	This function enables an option's interrupt on the TURBOchannel
 *	to interrupt the system at the I/O interrupt level.  
 *	This is done by setting the option's slot number as a valid
 *	interrupt to service.
 */
kn02enable_option(slot)
        int slot;
{
	kn02ie_mask |= (1 << slot);
	*(u_int *)PHYS_TO_K1(KN02CSR_ADDR) |= (1 << slot+KN02IE_OFFSET);
	wbflush();
}

/*
 *	kn02disable_option()
 *	
 *	Takes a slot number as an argument.
 *
 *	This function disables an option's interrupt on the TURBOchannel
 *	to interrupt the system at the I/O interrupt level.  
 *	This is done by resetting the option's slot number as a valid
 *	interrupt to service.
 */	
kn02disable_option(slot)
        int slot;
{
	kn02ie_mask &= ~(1 << slot);
	*(u_int *)PHYS_TO_K1(KN02CSR_ADDR) &= ~(1 << slot+KN02IE_OFFSET);
	wbflush();
}

/*
 * 3max halt interrupt routine.
 * It calls the console PROM halt routine with an address (ep) where it will
 * start dumping out values & the length to dump (size of exception frame, in
 * long words).
 */
kn02halt(ep)
{
	extern int rex_base;

	if(rex_base)
		rex_halt(ep, EF_SIZE/4); /* TURBOchannel'd 3max ROM callback */
	else
		prom_halt(ep, EF_SIZE/4);
}

/*
 * 3max stray interrupt routine.
 */
kn02stray(ep)
	u_int *ep;		/* exception frame ptr */
{
	printf("Stray interrupt, CAUSE = 0x%x, STATUS = 0x%x\n",
		ep[EF_CAUSE], ep[EF_SR]);
}

/*
 * Check Power Supply over-heat Warning.
 * If its overheating, warn to shut down the system.
 * If its gone from overheat to OK, cancel the warning.
 */
kn02pscheck()
{
	register u_int kn02csr;		/* a copy of the real csr */

	kn02csr = *(u_int *)PHYS_TO_K1(KN02CSR_ADDR);

	if (kn02csr & KN02CSR_PSWARN) {
		printf("System Overheating - suggest immediate shutdown and power-off\n");
		kn02pswarn = 1;
	} else {
		if (kn02pswarn) {
			printf("System OK - cancel overheat shutdown\n");
			kn02pswarn = 0;
		}
	}
	timeout (kn02pscheck, (caddr_t) 0, kn02psintvl * hz);
}

/*
 * Enable CRD (single bit ECC) error logging
 */
kn02crdenable()
{
	kn02crdlog = 1;
	timeout (kn02crdenable, (caddr_t) 0, kn02crdintvl * hz);
}

/*
 * 3max I/O interrupt routine.
 * The IOint bits in the CSR tell which slot interrupted.
 * Look up the slot number in the tc_slot table to determine which
 *	interrupt routine to call.
 *
 * Note that more than 1 interrupt bit can be set in the CSR.
 * Also note that software does not need to clear the IO bits in the CSR,
 * they are cleared when the Interrupt line is cleared.
 */
kn02iointr(ep)
	u_int *ep;
{
	register u_int kn02csr;		/* a copy of the real csr */
	register int ioint;		/* the ioint bits in the CSR */

	kn02csr = *(u_int *)PHYS_TO_K1(KN02CSR_ADDR);

	/*
	 * This is a clever way of making a fast jump table which converts
	 * a bit set in the ioint portion of the kn02 csr to an index into
	 * the tc_slot table.  We are handling interrupts, so speed
	 * is of the essence.
	 */
	ioint = (kn02csr >> KN02IE_OFFSET) & (kn02csr & 0xff);

	if (ioint != 0) {
		/*
		 * Check for bits set in the high nibble
		 */
		switch (ioint & 0xf0) {

		case 0xf0:
		case 0xe0:
		case 0xd0:
		case 0xc0:
		case 0xb0:
		case 0xa0:
		case 0x90:
		case 0x80:
			/*
			 * We always handle the serial line ctlr first.
			 * This will give the mouse highest priority.
			 * For debug: pass ep to dcintr() so it can print epc
			 */
			(*(tc_slot[7].intr))(tc_slot[7].unit, ep);
			break;
		case 0x70:
		case 0x60:
		case 0x50:
		case 0x40:
			(*(tc_slot[6].intr))(tc_slot[6].unit);
			break;
		case 0x30:
		case 0x20:
			(*(tc_slot[5].intr))(tc_slot[5].unit);
			break;
		/*
		 * slot 4 unused on 3max
		 */
		}

		/*
		 * Check for bits set in the low nibble
		 */
		switch (ioint & 0xf) {

		/*
		 * slot 3 unused on 3max
		 */
		case 0x7:
		case 0x6:
		case 0x5:
		case 0x4:
			(*(tc_slot[2].intr))(tc_slot[2].unit);
			break;
		case 0x3:
		case 0x2:
			(*(tc_slot[1].intr))(tc_slot[1].unit);
			break;
		case 0x1:
			(*(tc_slot[0].intr))(tc_slot[0].unit);
			break;
		}
	}
}


/*
 * 3max delay routine.
 * Tested from 5 seconds down to 4,000 usecs (4 mSec clock accuracy).
 */
int kn02_delay_mult = 13;		/* multiplier for DELAY (optimized) */

kn02delay(n)
	int n;
{
	register int N = kn02_delay_mult*(n); 
	while (--N > 0); 
	return(0);
}

/*
 * Error interrupt.
 * With buffered writes, these are not reported synchronously, thus
 * there is no process context to terminate a user process, so we panic.
 *
 * Causes:
 *   Memory Errors:
 *	CPU read single bit ECC
 *	DMA read overrun, DMA write overrun
 *	CPU partial memory write ECC, DMA memory read ECC
 *   Timeout Errors:
 *	CPU I/O write timeout
 */
kn02errintr(ep)
	u_int *ep;			/* exception frame ptr */
{
	register int s;
	register u_int kn02csr;		/* copy of csr reg */
	register u_int erradr;		/* copy of erradr reg */
	register u_int chksyn;		/* copy of chksyn reg */
	register u_int chksyn_plus;	/* valid half of chksyn + errcnt + pc */
	int errtype;			/* local record of error type */
	int pa;				/* the physical address of the error */
	int module;			/* module number with error */
	long currtime;			/* current time value */
        struct kn02consinfo_t *pcons;   /* pointer to console info */
        struct kn02log_errinfo_t *plog; 	/* pointer to log info */

	kn02csr = *(u_int *)PHYS_TO_K1(KN02CSR_ADDR);
	erradr = *(u_int *)PHYS_TO_K1(KN02ERR_ADDR);
	chksyn = *(u_int *)PHYS_TO_K1(KN02CHKSYN_ADDR);

	if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_RECC) &&
	    (((chksyn & CHKSYN_VLDLO) && (chksyn & CHKSYN_SNGLO)) ||
	     ((chksyn & CHKSYN_VLDHI) && (chksyn & CHKSYN_SNGHI))))
		errtype = ERR_RECC;	/* singlebit memory read ECC */
	else if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_DMARECC) &&
	    ((chksyn & CHKSYN_VLDLO) || (chksyn & CHKSYN_VLDHI)))
		errtype = ERR_DMARECC;	/* DMA memory read ECC */
	else if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_WECC) &&
	    ((chksyn & CHKSYN_VLDLO) || (chksyn & CHKSYN_VLDHI)))
		errtype = ERR_WECC;	/* CPU partial mem write ECC */
	else if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_DMAROVR))
		errtype = ERR_DMAROVR;	/* DMA read overrun */
	else if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_DMAWOVR))
		errtype = ERR_DMAWOVR;	/* DMA write overrun */
	else if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_WTMO))
		errtype = ERR_WTMO;	/* CPU write timeout */
	else errtype = ERR_UKN;

	switch (errtype) {
	case ERR_RECC:
	case ERR_DMARECC:
	case ERR_WECC:
		erradr = (erradr & (~ERR_COLADDR)) | (((int)erradr -5) & ERR_COLADDR);
		pa = (erradr & ERR_ADDR) << 2;
		if (kn02csr & KN02CSR_BNK32M)
			module = pa / (32*(1024*1024));
		else	module = pa / (8*(1024*1024));
		if (module >= 0 && module < MEM_MODULES) {
			kn02memerrs[module]++;
			if (kn02memerrs[module] > MAXERRCNT) {
				kn02memerrs[module] = 0;
				mprintf("Error count on memory module %d reached %d, resetting count to zero.\n", module, MAXERRCNT);
			}
		} else
			module = -1;
		/*
		 * Build chksyn_plus.
		 */
		chksyn_plus = 0;
		if (chksyn & CHKSYN_VLDLO) {
			chksyn_plus = (chksyn & CPLUS_CHK);
		} else
			chksyn_plus = ((chksyn >> CPLUS_MOFF) & CPLUS_CHK);
		if (module == -1) {
			chksyn_plus |= (0 << CPLUS_EOFF);
			chksyn_plus |= (0 << CPLUS_MOFF);
		} else {
			chksyn_plus |= (kn02memerrs[module] << CPLUS_EOFF);
			chksyn_plus |= (module << CPLUS_MOFF);
		}
		chksyn_plus &= ~CPLUS_VALID;
		if (((chksyn & CHKSYN_VLDLO) && (chksyn & CHKSYN_SNGLO)) ||
	            ((chksyn & CHKSYN_VLDHI) && (chksyn & CHKSYN_SNGHI))) {
			/*
			 * Single bit ECC error (CRD)
			 * If we get 3 or more in 1 second then disable logging
			 * them for 15 minutes.  The variable "kn02stopcrdlog"
			 * is cleared by the kn02crdenable routine.
			 */
			if (kn02crdlog) {
				currtime = time.tv_sec;
				if (currtime == crd_errcnt.crd_prev) {
					kn02crdlog = 0;
					mprintf("High rate of corrected single-bit ECC errors, logging disabled for 15 minutes\n");
					crd_errcnt.crd_last = 0;
					currtime = 0;
				}
				kn02logmempkt(EL_PRIHIGH, ep, ELMETYP_CRD, kn02csr, erradr, chksyn_plus);
				crd_errcnt.crd_prev = crd_errcnt.crd_last;
				crd_errcnt.crd_last = currtime;
			}
			/*
			 * Scrub the single bit error
			 */
			*(u_int *)PHYS_TO_K0(pa) = *(u_int *)PHYS_TO_K0(pa);
			*(u_int *)PHYS_TO_K1(KN02ERR_ADDR) = 0;
			wbflush();
		} else {
			/*
			 * Multibit error, panic.
			 */
			kn02logmempkt(EL_PRISEVERE, ep, ELMETYP_RDS, kn02csr, erradr, chksyn_plus);
			kn02consprint(MEMPKT, ep, kn02csr, erradr, chksyn_plus);
			*(u_int *)PHYS_TO_K1(KN02ERR_ADDR) = 0;
			kn02eccpanic = 1;
			wbflush();
			panic("multibit memory ECC error");
		}
		break;
	case ERR_DMAWOVR:
	case ERR_DMAROVR:
		kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, erradr);
		kn02consprint(ESR_INTR_PKT, ep, kn02csr, erradr, 0);
		*(u_int *)PHYS_TO_K1(KN02ERR_ADDR) = 0;
		wbflush();
		panic("DMA overrun");
		break;
	case ERR_WTMO:
		/*
		 * if we are still processing an previous interrupt
		 * then simply crash. we don't queue these interrupts.
		 */
		if (CURRENT_CPUDATA->cpu_wto_event) {
			kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, erradr);
			kn02consprint(ESR_INTR_PKT, ep, kn02csr, erradr, 0);
			*(u_int *)PHYS_TO_K1(KN02ERR_ADDR) = 0;
			wbflush();
			panic("CPU write timeout");
		}
		else {
			/*
 			 *	capture error information in kn02consinfo.
			 *	softnet() interrupt will print this info
			 *	if panicing on the console.
			 */
			pcons = &kn02consinfo;
			pcons->pkt_type 	= ESR_INTR_PKT;
			pcons->pkt.intrp.cause	= ep[EF_CAUSE];	
			pcons->pkt.intrp.sr	= ep[EF_SR];	
			pcons->pkt.intrp.sp	= ep[EF_SP];	
			pcons->pkt.intrp.csr	= kn02csr;
			pcons->pkt.intrp.erradr	= erradr;
			/*
 			 *	capture log information in kn02log_errinfo.
			 *	softnet() interrupt will log this info
			 *	if panicing in the error log buffer.
			 */
			plog = &kn02log_errinfo;
			plog->pkt_type 	= ESR_INTR_PKT;
			plog->cause	= ep[EF_CAUSE];	
			plog->sr	= ep[EF_SR];	
			plog->sp	= ep[EF_SP];	
			plog->epc	= ep[EF_EPC];
			plog->badvaddr	= ep[EF_BADVADDR];
			plog->csr	= kn02csr;
			plog->erradr	= erradr;
			CURRENT_CPUDATA->cpu_consinfo	= (char *) &kn02consinfo;
			CURRENT_CPUDATA->cpu_log_errinfo = (char *) &kn02log_errinfo;
			CURRENT_CPUDATA->cpu_wto_pfn    = btop((erradr & ERR_ADDR) << 2);
			CURRENT_CPUDATA->cpu_wto_event = 1;
			*(u_int *)PHYS_TO_K1(KN02ERR_ADDR) = 0;
			wbflush();
			setsoftnet();
		}
		break;
	case ERR_UKN:
	default:
		kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, erradr);
		kn02consprint(ESR_BUS_PKT, ep, kn02csr, erradr, 0);
		cprintf("\tChecksyn register\t= 0x%x\n", chksyn);
		*(u_int *)PHYS_TO_K1(KN02ERR_ADDR) = 0;
		wbflush();
		panic("Unknown memory error interrupt");
		break;
	}
	return(0);
}


/*
 * Routine to handle trap errors: user-mode ibe & dbe, & all kernel mode traps.
 * We try to recover from user-mode errors and panic on kernel mode errors.
 *
 * Entry conditions:
 *	kn02erradr and kn02chksyn are set in locore on bus errors
 *	(VEC_dbe and VEC_ibe) to be a copy of the hardware registers.
 */
kn02trap_error(ep, code, sr, cause, signo)
	register u_int *ep;		/* exception frame ptr */
	u_int code;			/* trap code (trap type) */
	u_int sr, cause;		/* status and cause regs */
	int *signo;			/* set if we want to kill process */
{
	caddr_t pa;			/* the physical addr of the error */	
	register struct proc *p;	/* ptr to current proc struct */
	register u_int kn02csr;		/* copy of csr reg */
	register u_int erradr;		/* copy of erradr reg */
	register u_int chksyn;		/* copy of chksyn reg */
	register u_int chksyn_plus;	/* chksyn + pc valid bit & err count */
	int errtype;			/* local record of error type */
	int module;			/* module number with error */
	int kn02_physmem;		/* adjusted physmem value */

	p = u.u_procp;
	kn02csr = *(u_int *)PHYS_TO_K1(KN02CSR_ADDR);
	erradr = kn02erradr;
	chksyn = kn02chksyn;
	if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_RECC) &&
	    (((chksyn & CHKSYN_VLDLO) && ((chksyn & CHKSYN_SNGLO) == 0))||
	     ((chksyn & CHKSYN_VLDHI) && ((chksyn & CHKSYN_SNGHI) == 0))))
		errtype = ERR_RECC;	/* Multibit memory read ECC */
	else if ((erradr & ERR_VALID) && ((erradr & ERR_TYPE) == ERR_RTMO))
		errtype = ERR_RTMO;	/* CPU read timeout */
	else errtype = ERR_UKN;

	/*
	 * If nvram present adjust physmem value to include the 1Mbyte
	 * of NVRAM which is located in memory. physmem naturally doesn't know
	 * about the NVRAM because it is not marked in the bitmap like
	 * regular memory. Note physmem is in pages, so 1MB is 256 pages.
	 * Also, there is a small window before kn02_nvram_found is set in
	 * the configure() routine, if we take an ECC error in this window,
	 * it will be handled properly, but logged incorrectly, so a 
	 * multi-bit ECC error would be logged as taking place on a non
	 * existent memory module, single bits will just be corrected and 
	 * logged.
	 *
	 */
	if (kn02_nvram_found)
	    kn02_physmem = physmem + 256;
	else
	    kn02_physmem = physmem;

	if (USERMODE(sr)) {
		switch (errtype) {
		case ERR_RECC:
			pa = vatophys(ep[EF_BADVADDR]);
			if ( (int)pa != -1 && (btop((int)pa) < kn02_physmem) )
			    if (kn02csr & KN02CSR_BNK32M)
				    module = (int)pa / (32*(1024*1024));
			    else    module = (int)pa / ( 8*(1024*1024));
			else
				module = -1;
			if (module >= 0 && module < MEM_MODULES) {
				kn02memerrs[module]++;
				if (kn02memerrs[module] > MAXERRCNT) {
					kn02memerrs[module] = 0;
					mprintf("Error count on memory module %d reached %d, resetting count to zero.\n", module, MAXERRCNT);
				}
			} else
				module = -1;
			erradr = (erradr & (~ERR_COLADDR)) | (((int)erradr -5) & ERR_COLADDR);
			chksyn_plus = 0;
			if (chksyn & CHKSYN_VLDLO) {
				chksyn_plus = (chksyn & CPLUS_CHK);
			} else
				chksyn_plus = ((chksyn >> CPLUS_MOFF) & CPLUS_CHK);
			if (module == -1)
				chksyn_plus |= (0 << CPLUS_EOFF)
					| (0 < CPLUS_MOFF) | CPLUS_VALID;
			else
				chksyn_plus |= (kn02memerrs[module] << CPLUS_EOFF)
					| (module < CPLUS_MOFF) | CPLUS_VALID;
			if (SHAREDPG(pa)) {
				kn02logmempkt(EL_PRISEVERE, ep, ELMETYP_RDS, kn02csr, erradr, chksyn_plus);
				kn02consprint(MEMPKT, ep, kn02csr, erradr, chksyn_plus);
				kn02eccpanic = 1;
				if (module == -1)
					panic("multibit ECC error reported on non-existent memory module");
				else 
					panic("multibit memory ECC error in shared page");
			} else {
				kn02logmempkt(EL_PRISEVERE, ep, ELMETYP_RDS, kn02csr, erradr, chksyn_plus);
				printf("pid %d (%s) was killed on multibit memory ECC error\n",
					p->p_pid, u.u_comm);
				uprintf("pid %d (%s) was killed on multibit memory ECC error\n",
					p->p_pid, u.u_comm);
			}
			break;
			
		case ERR_RTMO:
			uprintf("pid %d (%s) was killed on CPU read bus timeout\n",
				p->p_pid, u.u_comm);
			
			break;
		case ERR_UKN:
		default:
			uprintf("pid %d (%s) was killed on unknown bus error\n",
				p->p_pid, u.u_comm);
			break;
		}
	} else {
		/*
		 * Kernel mode errors.
		 * They all panic, its just a matter of what we log
		 * and what panic message we issue.
		 */
		switch (code) {

		case EXC_DBE:
			/*
			 * If we are already dumping from an ECC error
			 * and we get a data bus CPU read ECC error, then
			 * just ignore this error.
			 */
			if ((errtype == ERR_RECC) && kn02eccpanic) {
				return(0);
			}
			/* fall thru */
		case EXC_IBE:
			switch (errtype) {
			case ERR_RECC:	/* Multibit memory read ECC */
				pa = vatophys(ep[EF_BADVADDR]);
				if ( (int)pa != -1 && (btop((int)pa) < kn02_physmem) ) {
				    if (kn02csr & KN02CSR_BNK32M)
					    module = (int)pa / (32*(1024*1024));
				    else    module = (int)pa / ( 8*(1024*1024));
				} else
					module = -1;
				if (module >= 0 && module < MEM_MODULES) {
					kn02memerrs[module]++;
					if (kn02memerrs[module] > MAXERRCNT) {
						kn02memerrs[module] = 0;
						mprintf("Error count on memory module %d reached %d, resetting count to zero.\n", module, MAXERRCNT);
					}
				} else
					module = -1;
				erradr = (erradr & (~ERR_COLADDR)) | (((int)erradr -5) & ERR_COLADDR);
				chksyn_plus = 0;
				if (chksyn & CHKSYN_VLDLO) {
					chksyn_plus = (chksyn & CPLUS_CHK);
				} else
					chksyn_plus = ((chksyn >> CPLUS_MOFF) & CPLUS_CHK);
				if (module == -1)
					chksyn_plus |= (0 << CPLUS_EOFF)
						| (0 < CPLUS_MOFF) | CPLUS_VALID;
				else
					chksyn_plus |= (kn02memerrs[module] << CPLUS_EOFF)
						| (module < CPLUS_MOFF) | CPLUS_VALID;
				kn02logmempkt(EL_PRISEVERE, ep, ELMETYP_RDS, kn02csr, erradr, chksyn_plus);
				kn02consprint(MEMPKT, ep, kn02csr, erradr, chksyn_plus);
				kn02eccpanic = 1;
				if (module == -1)
					panic("multibit ECC error reported on non-existent memory module");
				else 
					panic("multibit memory ECC error");
				break;
				
			case ERR_RTMO:
				kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, erradr);
				kn02consprint(ESR_BUS_PKT, ep, kn02csr, erradr, 0);
				panic("CPU read bus timeout");
				break;
			case ERR_UKN:
			default:
				kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, erradr);
				kn02consprint(ESR_BUS_PKT, ep, kn02csr, erradr, 0);
				panic("Unknown bus timeout");
				break;
			}
			break;
		case EXC_CPU:
			kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, 0);
			kn02consprint(ESR_BUS_PKT, ep, kn02csr, 0, 0);
			panic("coprocessor unusable");
			break;
		case EXC_RADE:
		case EXC_WADE:
			kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, 0);
			kn02consprint(ESR_BUS_PKT, ep, kn02csr, 0, 0);
			panic("unaligned access");
			break;
		default:
			kn02logesrpkt(EL_PRISEVERE, ep, kn02csr, 0);
			kn02consprint(ESR_BUS_PKT, ep, kn02csr, 0, 0);
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

#define KN02_LOG_ESRPKT(elrp, cause,epc,sr,badvaddr,sp,csr,erradr) 		\
	elrp->el_body.elesr.elesr.el_esrkn02.esr_cause = cause;		\
	elrp->el_body.elesr.elesr.el_esrkn02.esr_epc = epc;		\
	elrp->el_body.elesr.elesr.el_esrkn02.esr_status = sr;		\
	elrp->el_body.elesr.elesr.el_esrkn02.esr_badva = badvaddr;	\
	elrp->el_body.elesr.elesr.el_esrkn02.esr_sp = sp;		\
	elrp->el_body.elesr.elesr.el_esrkn02.esr_csr = csr;		\
	elrp->el_body.elesr.elesr.el_esrkn02.esr_erradr = erradr;	\

/*
 * Log Error & Status Registers to the error log buffer
 */
kn02logesrpkt(priority, ep, kn02csr, erradr)
	int priority;		/* for pkt priority */
	register u_int *ep;	/* exception frame ptr */
	u_int kn02csr;
	u_int erradr;
{
	struct el_rec *elrp;

	elrp = ealloc(sizeof(struct el_esr), priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_ESR,ELESR_kn02,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		KN02_LOG_ESRPKT(elrp, ep[EF_CAUSE], ep[EF_EPC], ep[EF_SR], ep[EF_BADVADDR], 
				ep[EF_SP], kn02csr, erradr);

		EVALID(elrp);
	}
}

/*
 * Log a memory error packet, so uerf can find it as a main memory error.
 */
kn02logmempkt(priority, ep, type, kn02csr, erradr, chksyn_plus)
	int priority;		/* pkt priority: panic: severe; else: high */
	register u_int *ep;	/* exception frame ptr */
	int type;		/* error type: RDS, CRD, DMAOVR */
	u_int kn02csr;		/* copy of kn02csr to log */
	u_int erradr;		/* copy of erradr to log */
	u_int chksyn_plus;	/* chksyn + error count + pc valid bit */
{
	struct el_rec *elrp;
	register struct el_mem *mrp;

	elrp = ealloc(EL_MEMSIZE, priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_kn02,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		mrp = &elrp->el_body.elmem;
		mrp->elmem_cnt = 1;
		mrp->elmemerr.cntl = 1;
		mrp->elmemerr.type = type;
		mrp->elmemerr.numerr = 1;
		mrp->elmemerr.regs[0] = ep[EF_EPC];
		mrp->elmemerr.regs[1] = kn02csr;
		mrp->elmemerr.regs[2] = erradr;
		mrp->elmemerr.regs[3] = chksyn_plus;
		EVALID(elrp);
	}
}

/*
 * 	Logs error information to the error log buffer.
 *	Exported through the cpu switch.
 */

kn02_log_errinfo(p)
struct kn02log_errinfo_t *p;
{
	struct el_rec *elrp;

	switch (p->pkt_type) {

	case ESR_INTR_PKT:
		elrp = ealloc(sizeof(struct el_esr), EL_PRISEVERE);
		if (elrp != NULL) {
			LSUBID(elrp,ELCT_ESR,ELESR_kn02,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
			KN02_LOG_ESRPKT(elrp, p->cause, p->epc, p->sr, p->badvaddr, 
					p->sp, p->csr, p->erradr);
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
kn02consprint(pkt, ep, kn02csr, erradr, chksyn_plus)
	int pkt;		/* error pkt: Error & Stat Regs / memory pkt */
	register u_int *ep;	/* exception frame ptr */
	u_int kn02csr;		/* kn02csr to print */
	u_int erradr;		/* kn02 erradr to print */
	u_int chksyn_plus;	/* chksyn + error count + pc valid bit */
{
	register int i;
	struct kn02consinfo_t p;
	

	p.pkt_type = pkt;
	switch (pkt) {
	case ESR_INTR_PKT:
		p.pkt.intrp.cause	= ep[EF_CAUSE];
		p.pkt.intrp.sr		= ep[EF_SR];
		p.pkt.intrp.sp		= ep[EF_SP];
		p.pkt.intrp.csr		= kn02csr;
		p.pkt.intrp.erradr	= erradr;
		break;

	case ESR_BUS_PKT:
		p.pkt.busp.cause	= ep[EF_CAUSE];
		p.pkt.busp.epc		= ep[EF_EPC];
		p.pkt.busp.sr		= ep[EF_SR];
		p.pkt.busp.badvaddr	= ep[EF_BADVADDR];
		p.pkt.busp.sp		= ep[EF_SP];
		p.pkt.busp.csr		= kn02csr;
		p.pkt.busp.erradr	= erradr;
		break;

	case MEMPKT:
		if (chksyn_plus & 0x80000000)
		    p.pkt.memp.epc	= ep[EF_EPC];

		p.pkt.memp.csr		= kn02csr;
		p.pkt.memp.erradr	= erradr;
		p.pkt.memp.chksyn_plus	= chksyn_plus;
		break;

	default:
		cprintf("bad consprint\n");
		return;
	}
	kn02_print_consinfo(&p);
	
}

/*
 *	This routine is similar to kn02consprint().
 *	This is exported through the cpusw structure. 
 *	
 */

kn02_print_consinfo(p)
struct kn02consinfo_t *p;
{
	/*
	 * If console is a graphics device,
	 * force printf messages directly to screen.
	 */
	printstate |= PANICPRINT;

	switch (p->pkt_type) {
	case ESR_INTR_PKT:
		cprintf("\nException condition\n");
		cprintf("\tCause reg\t= 0x%x\n", p->pkt.intrp.cause);
		cprintf("\tException PC\t= invalid for this error\n");
		cprintf("\tStatus reg\t= 0x%x\n", p->pkt.intrp.sr);
		cprintf("\tBad virt addr\t= invalid for this error\n");
		cprintf("\tStack ptr\t= 0x%x\n", p->pkt.intrp.sp);
		cprintf("\tSystem CSR\t= 0x%x\n", p->pkt.intrp.csr);
		cprintf("\tERRADR register\t= 0x%x\n", p->pkt.intrp.erradr);
		cprintf("\t  Phys addr\t= 0x%x\n", (p->pkt.intrp.erradr&ERR_ADDR) << 2);
		break;

	case ESR_BUS_PKT:
		cprintf("\nException condition\n");
		cprintf("\tCause reg\t= 0x%x\n", p->pkt.busp.cause);
		cprintf("\tException PC\t= 0x%x\n", p->pkt.busp.epc);
		cprintf("\tStatus reg\t= 0x%x\n", p->pkt.busp.sr);
		cprintf("\tBad virt addr\t= 0x%x\n", p->pkt.busp.badvaddr);
		cprintf("\tStack ptr\t= 0x%x\n", p->pkt.busp.sp);
		cprintf("\tSystem CSR\t= 0x%x\n", p->pkt.busp.csr);
		cprintf("\tERRADR register\t= 0x%x\n", p->pkt.busp.erradr);
		cprintf("\t  Phys addr\t= 0x%x\n", (p->pkt.busp.erradr&ERR_ADDR) << 2);
		break;

	case MEMPKT:
		cprintf("\nMemory Error\n");
		if (p->pkt.memp.chksyn_plus & 0x80000000)
			cprintf("\tException PC\t= 0x%x\n", p->pkt.memp.chksyn_plus);
		else
			cprintf("\tException PC\t= invalid for this error\n");
		cprintf("\tSystem CSR\t= 0x%x\n", p->pkt.memp.csr);
		cprintf("\tERRADR register\t= 0x%x\n", p->pkt.memp.erradr);
		cprintf("\t  Phys addr\t= 0x%x\n", (p->pkt.memp.erradr&ERR_ADDR) << 2);
		cprintf("\tError count for module %d = %d\n",
			((p->pkt.memp.chksyn_plus & CPLUS_MMASK) >> CPLUS_MOFF),
			((p->pkt.memp.chksyn_plus & CPLUS_EMASK) >> CPLUS_EOFF));
		break;

	default:
		cprintf("bad print_consinfo \n");
		break;
	}
}


/*
 *
 * kn02_config_nvram
 *
 *    - initialize pointers to cpu specific routines for Prestoserve
 *    - Check for nvram presence
 *    - initialize status and control register location according to 
 *      nvram location
 *    - make call to presto_init to initialize Prestoserve 
 *
 *
 */
int kn02_cache_nvram = 1;
int kn02_nvram_mapped = 0;
unsigned nv_csr = 0x1;
unsigned nv_diag = 0x13;

kn02_config_nvram()
{
  	extern kn02_nvram_status();
	extern kn02_nvram_battery_status();
	extern kn02_nvram_battery_enable();
	extern kn02_nvram_battery_disable();
	extern u_int kn02_machineid();

	extern int memlimit;
	unsigned kn02_nvram_start_addr;
	unsigned int kn02_nvram_location;
	unsigned kn02_nvram_size;

	unsigned kn02_nvram_physaddr;
	unsigned nvram_id = 0;
	short i = 0;

	static unsigned addr[] = { 0x800000,   0x1000000,  0x1800000, 
				   0x2000000,  0x2800000,  0x3000000,
				   0x3800000,  0x4000000,  0x4800000, 
				   0x5000000,  0x5800000,  0x6000000, 
				   0x6800000,  0x7000000,  0x8000000,
				   0xa000000,  0xc000000,  0xe000000,  
				   0x10000000, 0x12000000, 0x14000000, 
				   0x16000000, 0x18000000, 
				   0x1a000000, 0x1c000000 }; 

	/*
	 * For 3max testing, using main memory instead of NVRAM
	 * Use memlimit to stop memory at 32Mb, memlimit=0x2000000
	 * Set up necessary registers, to simulate NVRAM interface
	 *
	 */

	if (kn02_test)
	  {
	   if (memlimit) 
	    {
       	     printf("kn02 NVRAM debug: Simmulating NVRAM in manin memory\n");
	     printf("kn02 NVRAM debug: Starting address = 0x%x\n",memlimit);

	     *(int *)PHYS_TO_K1((memlimit + 0x3f8)) = nv_diag; 	   /* diag */
	     *(int *)PHYS_TO_K1((memlimit + 0x3fc)) = 0x03021966;  /* id */
	     *(int *)PHYS_TO_K1((memlimit + 0x400000)) = nv_csr;   /* csr */

	    }
	  }

	/* 
	 * Initialize the structure that will point to the cpu specific
	 * functions for Prestoserve
	 */

	presto_interface0.nvram_status = kn02_nvram_status;
	presto_interface0.nvram_battery_status= kn02_nvram_battery_status;
	presto_interface0.nvram_battery_disable= kn02_nvram_battery_disable;
	presto_interface0.nvram_battery_enable= kn02_nvram_battery_enable;

	/* 
	 * configure the NVRAM option if it is present
	 *
	 * Manually probe the possible locations where nvram address
	 * space can begin, if there is memory there, then look for
	 * the know signature at the id location.
	 *
	 */

	for ( i=0 ; i <= 24 ; i+=1)
	    {
		if ( bbadaddr(PHYS_TO_K1(addr[i]+KN02_NVRAM_ID),4) &&
		     bbadaddr(PHYS_TO_K1(addr[i]+KN02_NVRAM_ID),4))
		    {
		     Dprintf("kn02: No memory board present at 0x%x, i=%d\n",
			       PHYS_TO_K1(addr[i]+KN02_NVRAM_ID),i);
		    }
		else
		    {
			nvram_id = *(int *)PHYS_TO_K1(addr[i]+KN02_NVRAM_ID);

			Dprintf("%d: ID from memory = 0x%x\n",i, nvram_id);

			if ( nvram_id == KN02_NVRAM_IDENTIFIER)
			    {
				kn02_nvram_found = 1;
				Dprintf("kn02: Found NVRAM board\n");
				kn02_nvram_physaddr = addr[i];
				break;
			    }
			else Dprintf("kn02: Not an NVRAM board\n");

		    }
	    }


	if (kn02_nvram_found)

	  {

	    kn02_nvram_location = PHYS_TO_K1(kn02_nvram_physaddr) ;
	    kn02_nvram_start_addr = kn02_nvram_physaddr + KN02_NVRAM_START ;
 
	    kn02_nvram_diag = PHYS_TO_K1(kn02_nvram_location+KN02_NVRAM_DIAG);

	    kn02_nvram_csr = kn02_nvram_location + 0x400000; 

	    kn02_nvram_size = (*(int *)kn02_nvram_diag & NVRAM_SIZE) >> 4;
	    kn02_nvram_size *= 0x100000;  /* convert to bytes */
	    kn02_nvram_size -= 0x400; 	  /* - reserved space */

	    Dprintf("NVRAM: Starting address = 0x%x\n", kn02_nvram_start_addr);
	    Dprintf("NVRAM: Diag reg address = 0x%x\n", kn02_nvram_diag);
	    Dprintf("NVRAM: Diag reg contents = 0x%x\n",
		    *(int *)PHYS_TO_K1(kn02_nvram_diag));

	    presto_init(kn02_nvram_start_addr, kn02_nvram_size, 
			kn02_nvram_mapped, kn02_cache_nvram,
		        kn02_machineid());
	  }
}

/*
 *
 * 	kn02_nvram_status
 *
 *     	provide presto with status of diagnostics run on nvram
 *    	 hence, let presto know what to do - recover, etc...
 *
 *	RETURN value:  -1 if no status set, otherwise pr.h defined value
 *
 */

kn02_nvram_status()
{
	if ( *(int *)kn02_nvram_diag & KN02_NVRAM_FAILED)
		return(NVRAM_BAD);
	else if ( *(int *)kn02_nvram_diag & KN02_NVRAM_RO)
		return(NVRAM_RDONLY);
	else if ( *(int *)kn02_nvram_diag & KN02_NVRAM_RW)
		return(NVRAM_RDWR);
	else {
		printf("kn02_nvram_status: No nvram diag bits set for status");
		return(-1);
	}
}

/*
 * 	kn02_nvram_battery_status
 *
 *     	update the global battery information structure for Prestoserve
 *     
 *	RETURN value:  0, if batteries are ok;  1, if problem
 */
kn02_nvram_battery_status()
{

	nvram_batteries0.nv_nbatteries = 1;	   /* one battery */
	nvram_batteries0.nv_minimum_ok = 1;	   /* it must be good */
	nvram_batteries0.nv_primary_mandatory = 1; /* primary must be OK */
	nvram_batteries0.nv_test_retries = 3;	   /* call this routine 3 times
						    * for each "test" */

	/* for 3max simulation - always return true, enable zeros BOK */

	if ( (*(int *)kn02_nvram_csr & KN02_NVRAM_BOK) || (kn02_test) )
		{
			nvram_batteries0.nv_status[0] = BATT_OK;
			return(0);
		}
	else
	    return(1);
	  
}

/*
 *	 kn02_nvram_battery_enable
 *
 *  	- arms the battery 
 *  	- required action is to zero BDISC control bit, this disables the 
 *    	  battery disconect circuit.
 *
 *	RETURN value:  1, if batteries successfully enabled, 0 if not  
 */
kn02_nvram_battery_enable()
{
	
	*(int *)kn02_nvram_csr = 0x0;
	wbflush();

	if ( (*(int *)kn02_nvram_csr & KN02_NVRAM_BDISC))
		return(1);
	else
		return(0);
}

/*
 * 	kn02_nvram_battery_disable
 *
 *	- unarms battery
 *	- required action is to send sequence "11001" to control register
 *	- this enables the battery disconnect circuit
 *	- The excessive wbflushes are used to protect against merged writes
 *
 *	RETURN value:  0, if batteries successfully disabled, 1 if not  
 */
kn02_nvram_battery_disable()
{

/*psgfix - may be able to do dummy writes inbetween  writes, and 1 wbflush*/

	*(int *)kn02_nvram_csr = 0x1;
	wbflush();
	
	*(int *)kn02_nvram_csr = 0x1;
	wbflush();
	
	*(int *)kn02_nvram_csr = 0x0;
	wbflush();
	
	*(int *)kn02_nvram_csr = 0x0;
	wbflush();

	*(int *)kn02_nvram_csr = 0x1;
	wbflush();
	
	
	if ( *(int *)kn02_nvram_csr & KN02_NVRAM_BDISC )
		return(0);
	else
		return(1);
	
}

#include "../h/socket.h"
#include "../net/net/if.h"
/*
 * Get the hardware E_net address for on-board ln0 and use it
 * to build a poor man's version of a unique processor ID.
 */
u_int
kn02_machineid()
{
	register struct ifnet *ifp = ifnet;
	struct ifdevea ifr;
	u_int i = 0;
	int error;

	if (ifnet == NULL) {
		printf("kn220: ifnet NULL\n");
		return (i);
	}

	while (ifp != NULL) {
		if (ifp->if_name[0] == 'l' && ifp->if_name[1] == 'n' &&
		    ifp->if_unit == 0) {	/* found ln0 */
			error = (*ifp->if_ioctl)(ifp, SIOCRPHYSADDR, &ifr);
			if (error)
				return (i);
			i = (u_int)ifr.default_pa[2];
			i = i << 8;
			i += (u_int)ifr.default_pa[3];
			i = i << 8;
			i += (u_int)ifr.default_pa[4];
			i = i << 8;
			i += (u_int)ifr.default_pa[5];
			return (i);
		}
		ifp = ifp->if_next;
	}
	return (i);
}
