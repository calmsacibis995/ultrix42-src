#ifndef lint
static char *sccsid = "@(#)kn5800.c	4.3      (ULTRIX)  10/9/90";
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
 * 30-Apr-90	Randall Brown
 *	Added call to spl_init() to intialize spl function pointers and
 *      the intr() function pointer.
 *
 * 29-Mar-90	gmm
 *	enable CRDID in secondary_init()
 *
 * 06-Mar-90	gmm
 *	Log soft error on cache FIFO overflow. Also do the extra write even
 *	if lock bit set/clear in bbssi/bbcci
 *
 * 08-Dec-89	gmm
 *	Soft error logging changes merged from v3.1C. A workaround for the
 *	console bug to find the number of processors with both 3.1 and 4.0
 *	console ROMs. Bug fix to map the correct IP interrupt address for
 *	all valid slots (was using VAX page size before).
 *
 * 14-Nov-89    gmm
 *	Change to start a secondary processor, changes to bbssi() and bbcci()
 *	to flush cache(). This should get fixed in hardware soon.
 *
 * 13-Nov-89	burns
 *	Fixed nxaccess to deal properly with I/O address space, made
 *	interrupt vectors reads handle passive releases.
 *
 * 09-Nov-89	bp
 *	Optimized Vaxmap to use mips KSEG0 space to minimize tblmiss hits.
 *
 * 26-Oct-89    gmm
 *	Moved bbssi() and bbcci() from interlock.c to this file.
 *
 * 13-Oct-89	gmm
 *	SMP changes. Error log buffer gets allocated on per cpu basis. Added
 *	routines like kn5800_init_secondary() and modified cca_setup() etc.
 *
 * 12-Sep-89	burns
 *	New way of doing a write buffer flush that is much faster.
 *	Removed printfs that were used for debug.
 *	Removed hack code for "erroneous interrupts".
 *
 * 11-Sep-89	Pete Keilty
 *	Change alloc_vaxmap() to use a define Maxvax_dbpte which
 *	is defined in scamachmac.h
 *
 * 27-Jan-89	Bill Burns
 *	Created the file.  This file contains routines specific
 *	to ISIS systems.
 */



#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/user.h"			/* gets time.h and debug.h too */
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/errno.h"
#include "../h/systm.h"
#include "../h/types.h"
#include "../h/errlog.h"
#include "../h/cmap.h"
#include "../h/ioctl.h"

#include "../h/tty.h"
#include "../h/cpudata.h"
#include "../h/kmalloc.h"
#include "../h/vmmac.h"
#include "../io/uba/ubareg.h"
#include "../machine/cpu.h"
#include "../machine/reg.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/ssc.h"
#include "../machine/cons.h"
#include "../machine/cca.h"
#include "../machine/clock.h"
#include "../machine/hwconf.h"
#include "../machine/param.h"
#include "../machine/kn5800.h"
#include "../io/xmi/xmireg.h"
#include "../io/xmi/xmareg.h"
#include "../io/scs/scamachmac.h"


#define ESRPKT 1
#define MEMPKT 2


struct xmidata	*head_xmidata;
extern int (*(scb[]))();
extern caddr_t	vatophys();
extern struct bidata bidata[];
extern int cache_state;
extern int nNVAXBI;
struct kn5800_regs *kn5800_regp;
int	powerfail;
int	cache_state;		/* in autoconf for vax */
int	*kn5800_wbflush_addr;	/* Holds the address to read to trigger
				   a write buffer flush */
int	wbflush_dummy;		/* target destination for a write buffer
				   flush - its external to prevent compiler
				   from getting too smart and optimizing out
				   a write buffer flush */
struct	cca	*ccabase;	/* console communications area */
extern	char	*kn5800_ip_addr[];
/* struct ssc_regs cvqssc[1];		/* SSC regs */
struct v5800csr *v5800csr = (struct v5800csr *)CSR5800;

struct vaxpte *vax_map;
extern struct pte eSysmap[];
extern struct vaxpte *Vaxmap;	/* pointer to the base of Vaxmap */
extern int Vaxmap_size;		/* size of the entire Vaxmap in pages */
int Vax_page_factor;		/* Vax pages per mips page */
extern	int gvp_max_bds;	/* Number of data structs that need 128 ptes */
#define VAX_PAGE_SIZE 512

/*
 * Interrupt handlers for the 6 hardware interrupts and 2 software
 * interrupts for ISIS
 */

extern softclock(), softnet(), kn5800_intr0(), kn5800_intr1(),
	kn5800_intr2(), kn5800_intr3(), kn5800_halt(), fpuintr();

int (*kn5800_intr_vec[8])() = {
	softclock,		/* softint 0	       		    */
	softnet,		/* softint 1	       		    */
	kn5800_intr0,		/* hardint 0 - XMI level 4/5 and SSC*/
	kn5800_intr1,		/* hardint 1 - XMI level 6, IP and Cloc */
	kn5800_intr2,		/* hardint 2 - XMI level 7	     */
	kn5800_intr3,		/* hardint 3 - Hard and soft errors  */
	kn5800_halt,		/* hardint 4 - Halt interrupt	     */
	fpuintr			/* hardint 5 - FPU interrupt	     */
};

#ifndef SR_HALT
#define SR_HALT	SR_IBIT7
#endif	SR_HALT

int kn5800_iplmask[8] = {
	SR_IMASK1|SR_IEC|SR_HALT,	/* soft clock		*/
	SR_IMASK2|SR_IEC|SR_HALT,	/* soft net		*/
	SR_IMASK3|SR_IEC|SR_HALT,	/* XMI 4/5 and SSC	*/
	SR_IMASK4|SR_IEC|SR_HALT,	/* XMI 6, IP and clock	*/
	SR_IMASK5|SR_IEC|SR_HALT,	/* XMI 7		*/
	SR_IMASK6|SR_IEC|SR_HALT,	/* Hard and soft errors	*/
	SR_IMASK7|SR_IEC,		/* Halt			*/
	SR_IMASK8|SR_IEC|SR_HALT	/* FPU			*/
};

/*
 * This array conatins the interrupt mask to use for spl calls. Indexed
 * by the defines in cpu.h (SPLBIO, etc).
 */
int kn5800_splm[SPLMSIZE] = {
	SR_IEC | SR_IMASK0 | SR_HALT,	/* 0 SPLNONE			*/
	SR_IEC | SR_IMASK1 | SR_HALT,	/* 1 SPLSOFTC			*/
	SR_IEC | SR_IMASK2 | SR_HALT,	/* 2 SPLNET			*/
	0,				/* 3 NOT_USED 			*/
	0,				/* 4 NOT_USED 			*/
	SR_IEC | SR_IMASK3 | SR_HALT,	/* 5 SPLBIO, SPLIMP, SPLTTY	*/
	SR_IEC | SR_IMASK4 | SR_HALT,	/* 6 SPLCLOCK			*/
	SR_IEC | SR_IMASK6 | SR_HALT,	/* 7 SPLMEM			*/
	SR_IEC | SR_IMASK8 | SR_HALT,	/* 8 SPLFPU, EXTREME, HIGH, 7, 6*/
};


/*
 * alloc_vaxmap -- allocate space for a "vax" page table. This table consists
 * of two parts. First a VAX version of the kernel "sysmap". This is required
 * for the relative queues that are used by the devices that requires vax
 * virtual support. The queue heads and elements are kmalloc'ed and use
 * virtual indexes, thus the Vaxmap and sysmap must coincide virtually.
 * TODO: This first part of Vaxmap may only need to correspond to kmalloc
 * space.
 *
 * The second part of Vaxmap (indcated by Vaxdbpte) is space reserved to
 * hold Vax pte's that are used for actual data transfer operations. 128 + 8
 * ptes per "gvp_max_bds" give us enough room for the 64kb data transfer
 * for every device that is configured that needs Vax virtual support.
 */
alloc_vaxmap()
{
	register int base_size, dbpte_size;
	register int map_size, i,  pfn;
	register struct vaxpte *pte;

	/* HACK - needs to be fixed for mips page sizes other than 4k */
	Vax_page_factor = (NBPG/VAX_PAGE_SIZE);
	/* End HACK */

	/*
	 * Calculate Vaxmap_size which is the size of the Vaxmap in vax pages.
	 */

	base_size = ((eSysmap - Sysmap) * Vax_page_factor);

	/*
	 * Calculate the size map for db
	 */

	dbpte_size = (gvp_max_bds * Maxvax_dbpte);

	/*
	 * VAX pte space required for map.
	 */

	map_size = (((base_size + dbpte_size) * sizeof (struct vaxpte)) + 
			VAX_PAGE_SIZE - 1) / VAX_PAGE_SIZE; 
	Vaxmap_size = base_size + map_size + dbpte_size;

	if ((vax_map = (struct vaxpte *) kseg0_alloc(
				(Vaxmap_size * sizeof (struct vaxpte)))));
	else panic("alloc_vaxmap: allocate failed");
	Vaxmap = (struct vaxpte *)vax_map;

	/*
	 * 
	 */

	Dbptemap = (struct vaxpte *) (Vaxmap + base_size);

	/*
	 * Map the map itself
	 */

	for (pfn = btoc(K0_TO_PHYS(Vaxmap)) << 3, 
		pte = Vaxmap + base_size + dbpte_size, i = 0; i < map_size; 
			i++, pte++, pfn++) * (int *) pte = pfn | VAX_PG_V;

	/*
	 * Vax virtual sysmap
	 */

	VSysmap = (struct vaxpte *)(VAX_SYSVA +  
			(base_size + dbpte_size) * VAX_PGSIZE);

	/*
	 * Need a VAX virtual address in VSysmap for dbpte.
	 */

	Vaxdbpte = (struct pte *) (VSysmap + base_size);
}


#define	SSC_INTR_LEVEL	0x00000000
extern	int cnrint(), cnxint();
extern 	int cpu_ip_intr();	/* IPI handler */
/*
 * Configuration routine.
 */
kn5800_conf()
{
	extern int cold;
	extern unsigned cpu_systype;
	extern int cpu;			/* Ultrix internal System type */
	extern struct cpusw *cpup;
extern struct pte eSysmap[];
extern struct vaxpte *Vaxmap;

	char *nxv;
	int 	nxp;

	register int i;
	register int xcp_node;
	register struct xmidata *xmidata;
	struct xcp_reg *xcp;
	char *start;
	struct pte *pte;
	int cpu_subtype;
	struct kn5800_regs *x3p_regs;

	cold = 1;

	hwconf_init();

	powerfail = 0;		/* reset count of powerfail interrupts */

	/* Report what system we are on */
	switch (cpu) {
	case DS_5800:
		printf("KN5800 processor - system rev %d\n", (GETHRDREV(cpu_systype)));
		break;
	default:
		panic("kn5800_conf called, wrong system");
		break;
	}
	/*
	 * Begin "calypso-ish" initialization
	 */

	/* 
	 * coproc_find will cause the system's first write buffer flush in
	 * dealing with the floating point unit. So we must set up the
	 * the write_buffer flush address first. The write buffer flush
	 * address is the external (io) address of the boot processor's
	 * gpr register.
	 */
	/* allocate up 1 xmi structure */
	KM_ALLOC(xmidata,struct xmidata *,sizeof(struct xmidata ),KM_DEVBUF,KM_NOW_CL_CO_CA);

	head_xmidata = xmidata;
	xmidata->next = 0;
	xmidata->xminum = 0;
	xmidata->xmivirt = (struct xmi_reg *)PHYS_TO_K1(XMI_START_PHYS);
	xcp_node = v5800csr->csr1 & XMI_NODE_ID;
	xcp = ((struct xcp_reg *)PHYS_TO_K1(XMI_START_PHYS) + xcp_node);

	kn5800_wbflush_addr = (int *)&(xcp->xcp_gpr);
	coproc_find();			/* Deal with FPU */

	/*
	 * set up mapping for interrupt vector locations.
	 */
	mapin (btop(kn5800_vectors), btop(KN5800_VEC_REG_BASE),
						(PG_KW|PG_N|PG_V|PG_G));

	scb[0xf8/4] = cnrint;
	scb[0xfc/4] = cnxint;
	scb[0x80/4] = cpu_ip_intr;  /* put the ipi handler */
	init_ssc();

	/*
	 * allocate Vaxmap - a vax system page table for use by devices that need
	 * vax virutal address: CI, BVP.
	 */
	alloc_vaxmap();

	KM_ALLOC(x3p_regs, struct kn5800_regs *, (sizeof(struct kn5800_regs) * MAXCPU),
		                 KM_DEVBUF, KM_NOW_CL_CO_CA);
	kn5800_regp = x3p_regs;

	/*
	 * fill in table of IP interrupt addresses.  Addresses 
	 * are of the form:
	 *
	 *  vax	 2101nnnn
	 *  mips b101nnnn
	 *          where nnnn is the decoded XMI node number
	 *	
	 */

#define	IP_INTR_ADDRESS_BASE	0x11010000
	start = (char *) PHYS_TO_K1(IP_INTR_ADDRESS_BASE);
	for (i=0 ; i< MAX_XMI_NODE ; i++ ) {
		nxp = IP_INTR_ADDRESS_BASE + (1<<i);
		if (i < 12) /* for the 4k page size */
			nxv =  start + (1<<i);
		else {
			nxv = start + ((i-11) * NBPG);
		}
		kn5800_ip[i] = nxv;

	}

	xcp_node = v5800csr->csr1 & XMI_NODE_ID;
	xmidata->xmiintr_dst = (1 << xcp_node);
	kn5800_enable_cache() ;
	

#ifdef mips
	xmidata->xmiphys = (struct xmi_reg *) XMI_START_PHYS;
	xmidata->xmivec_page = &scb[0];
#endif mips
	spl0();  			/* Ready for interrupts */
	cpu_avail  = cca_setup() ; /* due to a console bug, the processor 
				      always shown to be in cosole mode */
	


	/* allocate space for machine specific error packets */
	KM_ALLOC(CURRENT_CPUDATA->cpu_machdep,char *,sizeof(struct el_xcpsoft),KM_DEVBUF,KM_NOW_CL_CO_CA);
	printf("DECsystem 58%d0 server\n",cpu_avail);

	/*
	 * configure the I/O system 
	 */
	xmiconf(0,cpup);

	/* clear warm and cold boot flags */
	ccabase->cca_hflag &= ~(CCA_V_BOOTIP|CCA_V_WARMIP);


	/*
	 * Configure swap area and related system
	 * parameter based on device(s) used.
	 */
#ifndef ultrix
	setconf();
	swapconf();
#endif not ultrix
	cold = 0;
	kn5800_memenable();
	return (0);
}

/* below define is from machine/vax/mem.h - needs to be put somewhere more portable. */
#define	MEMINTVL	(60 * 15)	/* 15 minutes */
int	memintvl = MEMINTVL;

kn5800_memenable()
{
	int xminode;
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	struct xmidata *xmidata;
	int i;
	extern int hz;

	xmidata = get_xmi(0);

	/*
	 * Enable XMAs crd interrupts
	 */
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		    ((short) (nxv->xmi_dtype) == XMI_XMA)) {
	
			/* clear disable CRD flag in memory controller */
			((struct xma_reg *)nxv)->xma_mctl1 
					      &= ~(XMA_CTL1_CRD_DISABLE);
		}
	}

	/*
	 * Enable cache soft error logging
	 */
	CURRENT_CPUDATA->cpu_state &= ~(CPU_SOFT_DISABLE);

	/*
	 * Schedule next call to ourself for the primary cpu. Secondary cpus
	 * should schedule their own
	 */
	if (memintvl > 0)
		timeout (kn5800_memenable, (caddr_t) 0, memintvl * hz);

	return(0);
}

/*
 * Initialization routine for kn5800 processor (ISIS).
 */
kn5800_init()
{
	extern int c0vec_tblsize;
	extern int (*c0vec_tbl[])();
	extern int iplmask[];
	extern int splm[];
	extern int hz;
	extern int tick;
	extern int tickadj;
	extern struct cpusw *cpup;
	register int xcp_node;
	struct xcp_reg *xcp;

	/*
	 * Initialize the interrupt dispatch table c0vec_tbl.
	 * Initialize the spl table for the system we are running on.
	 * Initialize the interrupt masks for the system we are running on.
	 * This must be done very early on at boot time and must
	 * be done before any interrupt is allowed.
	 */
	bcopy((int *)kn5800_intr_vec, c0vec_tbl, c0vec_tblsize);
	bcopy(kn5800_iplmask, iplmask, IPLSIZE * sizeof(int));
	bcopy(kn5800_splm, splm, (SPLMSIZE) * sizeof(int));

	/* Initialize the spl dispatch table and the intr dispatch routine */
	spl_init();

	clear_bev();
	splextreme();
	/*
	 * Set up the system specific value for "hz", the number of clock
	 * interrupts per second; and corresponding tick and tickadj values.
	 */
	hz = cpup->HZ;
	tick = 1000000 / hz;
	tickadj = 240000 / (60 * hz);
	xcp_node = v5800csr->csr1 & XMI_NODE_ID;
	xcp = ((struct xcp_reg *)PHYS_TO_K1(XMI_START_PHYS) + xcp_node);

	kn5800_wbflush_addr = (int *)&(xcp->xcp_gpr);
	return (0);
}


/*
 * kn5800_halt - R3000 interrupt pin 5
 *
 *	Halt interrupt
 *
 *	Since mips doesn't have a halt instruction we just call the console.
 */
int	haltflag;

kn5800_halt (ep)
u_int	*ep;
{
	panic ("kn5800_halt");
}




/*
 * intr3 - R3000 interrupt pin 3 - Hard and soft system errors.
 *	Possible causes:
 *		Powerfail
 *		Memory errors
 *		CRD - corrected read data and memory errors
 *		First level invalidate fifo overflow
 *
 * 	No vector
 *
 *	Since Ultrix doesn't support powerfail restarts we wait to see it
 *	it is a real powerfail or not. If not we just return, if so we...
 */
kn5800_intr3 (ep)
u_int	*ep;
{
	int	csr1, csr2, xbe;
	int	tmp;
	struct xcp_reg *xcp_node;
	register struct xmidata *xmidata;
	register struct kn5800_regs *reg;
	register struct el_xcpsoft *sptr;

	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt +
					(v5800csr->csr1 & XMI_NODE_ID);
	sptr = (struct el_xcpsoft *)CURRENT_CPUDATA->cpu_machdep;
	/*
	 * Gather up X3P registers.
	 */
	reg = kn5800_regp;
	reg->kn5800_csr1 = v5800csr->csr1;
	reg->kn5800_dtype = xcp_node->xcp_dtype;
	reg->kn5800_xbe = xcp_node->xcp_xbe;
	reg->kn5800_fadr = xcp_node->xcp_fadr;
	reg->kn5800_gpr = xcp_node->xcp_gpr;
	reg->kn5800_csr2 = xcp_node->xcp_csr2;

	/*
	 * See if it's a powerfail
	 */
	if (!(reg->kn5800_csr1 & XMI_ACLO)) {		/* Powerfail - duck */
		powerfail++;
		/* Delay one second, if we are still alive, continue */
		DELAY(1000000);
	}
	if (reg->kn5800_csr1 & IFIFOFL) {
		if ((CURRENT_CPUDATA->cpu_state & CPU_SOFT_DISABLE) == 0)
				kn5800_log_soft(reg,sptr);
		kn5800_flush_cache();
	}

	/*
	 * If we detect any fatal memory errors call kn5800_memerr to
	 * report, crash and burn.
	 */

	if (reg->kn5800_xbe & (XBE_FATAL_BITS | XBE_RER | XBE_RIDNAK)) {
		kn5800_memerr (ep, reg, EL_PRISEVERE);
	}

	if (reg->kn5800_csr2 & (CSR2_WDPE)) {
		kn5800_memerr (ep, reg, EL_PRISEVERE);
	}

	/*
	 * Call routines that check for other errors, such as CRD's and
	 * second level cache errors.
	 */
	kn5800_crderr (reg);
	return(0);
}



/*
 * intr2 - R3000 interrupt pin 2
 *	Possible causes:
 *		XMI level 7 interrupt
 *	Read register at 0x4000005c to get a vector.
 */
kn5800_intr2 (ep, code)
u_int	*ep;
int	code;
{
	int	vector;

	vector = read_nofault(&kn5800_vectors[VEC_LEVEL3 / 4]);
	vector &= 0xff;
	(*(scb[vector/4]))(ep, (vector % 512));
}


/*
 * intr1 - R3000 interrupt pin 1
 *	Possible causes:
 *		Interval timer (hardclock timer).
 *		XMI level 6 interrupt
 *		IP interrupts
 *	Read register at 0x30000058 to get a vector for xmi level 6.
 *	
 *
 */
kn5800_intr1 (ep, code)
u_int	*ep;
int	code;
{
	int	vector;

	/*
	 * Check for a clock interrupt, if not we check for a vectored
	 * XMI level 6 interrupt.
	 */
	if (v5800csr->csr1 & INTMR) {
		hardclock(ep);
	} else {
		vector = read_nofault(&kn5800_vectors[VEC_LEVEL2 / 4]);
		(*(scb[vector/4]))(ep, (vector % 512));
	}
}


/*
 * intr0 - R3000 interrupt pin 0
 *	Possible causes:
 *		XMI level 5 interrupt
 *		SSC chip (programmable timers and console).
 *		XMI level 4 interrupt
 *
 *	The SSC interrupts are grouped with level 4 by the INTR1 bit
 *	in csr1. If INTR1 is asserted there is a XMI level 14 or SSC
 *	interrupt. If INTR1 is deasserted there is an XMI level 15
 *	interrupt pending.
 *
 *	Read register at 0x30000050 or 0x30000054 to get a vector.
 *
 */

kn5800_intr0 (ep, code)
u_int	*ep;
int	code;
{
	register int	vector;
	register int	csr1;

	csr1 = v5800csr->csr1;
	if (csr1 & INTR1) {
		vector = read_nofault(&kn5800_vectors[VEC_LEVEL0 / 4]);
	} else {
		vector = read_nofault(&kn5800_vectors[VEC_LEVEL1 / 4]);
	}
	(*(scb[vector/4]))(ep, (vector % 512));
}



/*
 * Enable ISIS secondary cache
 */
kn5800_enable_cache() {

	int save_led_val;

	 /* enable the cache */
	save_led_val = v5800csr->csr1;
	v5800csr->csr1 = (save_led_val | FMISS | FCI);
	v5800csr->csr1 =
		 (save_led_val & ~(FMISS | FCI));
}


/*
 * kn5800_memerr -- Called from an interrupt or trap 
 *
 *	Here are the fatal error bits checked:
 *		XBE:WDNAK  -- write data noAck.
 *		XBE:TE	   -- Transmit Error.
 *		XBE:CNAK   -- Command NoAck.
 *		CSR2:WDPE  -- DAL write Parity Error.
 *		XBE:WEI    -- XMI write Error IVINTR's.
 * 		XBE:XFAULT -- XMI fault assertion.
 *		All forms of Ident errors (which bits?).
 */


kn5800_memerr(ep, ptr, priority)
register u_int *ep;			/* Exception frame pointer */
struct kn5800_regs *ptr;		/* Pointer to saved processor regs */
int priority;				/* Priority for packet */
{
	int cpunum;
	struct xcp_reg *xcp_node;
	register struct xmidata *xmidata;

	/*
	 * Get xmi node info.
	 */
	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v5800csr->csr1 & 0xf);

	/*
	 * Log a memory error packet and print processor info on console.
	 */
	kn5800_logmempkt(ep, ptr, priority);
	kn5800_consprint(MEMPKT, ep);

	/*
	 * Handle case of a passive release on a interrupt vector read
	 * due to resetting a device.
	 */
   	if (   (xcp_node->xcp_xbe & (XMI_RER|XMI_NRR)) && ((xcp_node->xcp_xbe & 0xf)==9)) {
		kn5800_clear_xbe();
		return(0);
	}

	/*
	 * We are crashing so print out x3p specific stuff.
	 */
	cprintf("Fatal error detected by processor at node %x\n",
					v5800csr->csr1 & XMI_NODE_ID);

	cprintf("\tx3p_csr1\t= 0x%x\n", ptr->kn5800_csr1);
	cprintf("\tx3p_dtype\t= 0x%x\n", ptr->kn5800_dtype);
	cprintf("\tx3p_xbe\t= 0x%x\n", ptr->kn5800_xbe);
	cprintf("\tx3p_fadr\t= 0x%x\n", ptr->kn5800_fadr);
	cprintf("\tx3p_csr2\t= 0x%x\n", ptr->kn5800_csr2);

	/*
	 * Print out any pending XMA errors.
	 */
	xma_check_errors(xcp_node,EL_PRIHIGH);

	/*
	 * kill system
	 */
	panic("memory error");
	/* no return */
}

kn5800_logmempkt(ep, ptr, priority)
register u_int *ep;			/* Exception frame pointer */
struct kn5800_regs *ptr;		/* Pointer to saved processor regs */
int priority;				/* Priority for packet */
{
	struct el_rec *elrp;

 	elrp = ealloc(sizeof(struct el_esr), priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_ESR,ELESR_5800,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		elrp->el_body.elesr.elesr.el_esr5800.esr_cause = ep[EF_CAUSE];
		elrp->el_body.elesr.elesr.el_esr5800.esr_epc = ep[EF_EPC];
		elrp->el_body.elesr.elesr.el_esr5800.esr_status = ep[EF_SR];
		elrp->el_body.elesr.elesr.el_esr5800.esr_badva = ep[EF_BADVADDR];
		elrp->el_body.elesr.elesr.el_esr5800.esr_sp = ep[EF_SP];
		elrp->el_body.elesr.elesr.el_esr5800.x3p_csr1 = ptr->kn5800_csr1;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_dtype = ptr->kn5800_dtype;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_xbe = ptr->kn5800_xbe;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_fadr = ptr->kn5800_fadr;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_gpr = ptr->kn5800_gpr;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_csr2 = ptr->kn5800_csr2;
  	        EVALID(elrp);
	}
}

/*
 *
 */
xma_check_errors(xcp,priority)
struct xcp_reg *xcp;
int priority;
{
	struct el_xma *xma_pkg;
	struct xmi_reg *nxv;
	struct xma_reg *xma;
	struct xmidata *xmidata;
	int xminode;
	struct el_rec *elrp;

	xmidata = (struct xmidata *)get_xmi(0);

	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		   ((short) (nxv->xmi_dtype) == XMI_XMA)) {
		    	xma = (struct xma_reg *) nxv;
		    	if ((xma->xma_xbe & XMI_ES)  ||
	    		     (xma->xma_mctl1 &	(XMA_CTL1_LOCK_ERR|
			     	XMA_CTL1_UNLOCK_ERR|XMA_CTL1_RDS_WRITE)) ||
	    		     (xma->xma_mecer & (XMA_ECC_RDS_ERROR))) {
				log_mem_error(xcp,xma,priority);
			}
		}	
	}
}



/*
 *
 */
kn5800_clear_xbe()
{
	struct xmidata *xmidata;
	struct xcp_reg *xcp_node;

	/* get pointer to xcp node that machine checked */
	xmidata = get_xmi(0);
	xcp_node = (struct xcp_reg *)xmidata->xmivirt +
					(v5800csr->csr1 & XMI_NODE_ID);
	
	xcp_node->xcp_xbe = xcp_node->xcp_xbe & ~XMI_XBAD;
	
}


/* log XMA memory error.  If priority is not LOW then print to 
 *	console.
 */
log_mem_error(xcp,xma,priority)
	int priority;
	struct xma_reg *xma;
	struct xcp_reg *xcp;
{
	struct el_xma *xma_pkg;
	struct xmidata *xmidata;
	struct el_rec *elrp;
	

	/* log the XMA error */
	elrp = ealloc(sizeof(struct el_xma),priority);
	if (elrp) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_6200,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		xma_pkg = &elrp->el_body.el_xma;
#ifdef vax
		xma_pkg->xma_node = ((svtophy(xma))>>19)&0xf;
#endif vax
		xma_pkg->xma_dtype = xma->xma_type;
		xma_pkg->xma_xbe = xma->xma_xbe;
		xma_pkg->xma_seadr = xma->xma_seadr;
		xma_pkg->xma_mctl1 = xma->xma_mctl1;
		xma_pkg->xma_mecer = xma->xma_mecer;
		xma_pkg->xma_mecea = xma->xma_mecea;
		xma_pkg->xma_mctl2 = xma->xma_mctl2;
					
  	
     		EVALID(elrp);
	}
	/* print to console if HIGH priority */
	if (priority < EL_PRILOW) {
		cprintf("Fatal memory error \n");
#ifdef vax
		cprintf("xma_phys = %x\n",svtophy(xma));
#endif vax
		cprintf("xma_type = %x\n",xma->xma_type);
		cprintf("xma_xbe = %x\n",xma->xma_xbe);
		cprintf("xma_seadr = %x\n",xma->xma_seadr);
		cprintf("xma_mctl1 = %x\n",xma->xma_mctl1);
		cprintf("xma_mecer = %x\n",xma->xma_mecer);
		cprintf("xma_mecea = %x\n",xma->xma_mecea);
		cprintf("xma_mctl2 = %x\n",xma->xma_mctl2);
				
	}
}


/*
 *  kn5800_crderr() ---
 *
 *		XBE:CRD    - Correctable main memory errors.
 *
 * 	2nd Level cache errors.
 *		CSR2:VBPE  - Valid bit parity error.
 *		CSR2:TPE   - Tag parity error.
 *		CSR2:IQO   - Invailidate Queue Overflow.
 *		CSR2:DTPE  - Duplicate Tag Store Parity Error.
 *		CSR2:CFE   - Cache Fill Error.,
 *		
 *	XMI soft errors.
 *		XBE:CC	   - Corrected conformation Errors.
 *		XBE:IPE    - Inconsistent Parity Error.
 *		XBE:PE	   - Parity Error.
 */


kn5800_crderr(ptr)
struct kn5800_regs *ptr;
{
	struct xcp_reg *xcp_node;
	int save_led_val;
	register struct xmidata *xmidata ;
	int node;
	register struct el_xcpsoft *sptr;
	register int csr2_errs,xbe_errs, flush_the_cache=0;

	csr2_errs = ptr->kn5800_csr2;
	xbe_errs = ptr->kn5800_xbe;

	xmidata = get_xmi(0);
	node = v5800csr->csr1 & XMI_NODE_ID;
	sptr = (struct el_xcpsoft *)CURRENT_CPUDATA->cpu_machdep;
	xcp_node =(struct xcp_reg *)xmidata->xmivirt +
					(v5800csr->csr1 & XMI_NODE_ID);
	/*
	 * clear the error indicators
	 */
	xcp_node->xcp_csr2 = xcp_node->xcp_csr2;
	xcp_node->xcp_xbe = xcp_node->xcp_xbe;

	/*
	 * test for IQO error -- count and dismiss
	 */
	if (csr2_errs & CSR2_IQO) {
		if (sptr)
			sptr->xcp_iqo++;
		flush_the_cache++;
	} 

	/* test for cache error */
	if (csr2_errs & (CSR2_VBPE|CSR2_TPE|CSR2_CFE|CSR2_DTPE)) {
		if (sptr) {
			if (csr2_errs & CSR2_VBPE) sptr->xcp_vbpe++;
			if (csr2_errs & CSR2_TPE) sptr->xcp_tpe++;
			if (csr2_errs & CSR2_CFE) sptr->xcp_cfe++;
			if (csr2_errs & CSR2_DTPE) sptr->xcp_dtpe++;
			if ((CURRENT_CPUDATA->cpu_state & CPU_SOFT_DISABLE) == 0)
				kn5800_log_soft(ptr,sptr);
		}
		flush_the_cache++;
	}

    	/* test for XMI error */
	if (xbe_errs & (XMI_CC|XMI_IPE|XMI_PE)) {
		if (sptr) {
			if (xbe_errs & XMI_CC) sptr->xcp_cc++;
			if (xbe_errs & XMI_IPE) sptr->xcp_ipe++;
			if (xbe_errs & XMI_PE) sptr->xcp_pe++;
			if ((CURRENT_CPUDATA->cpu_state & CPU_SOFT_DISABLE) == 0)
				kn5800_log_soft(ptr,sptr);
		}
		flush_the_cache++;
	} 

	/* test for CRD error */
	if (xbe_errs & (XMI_CRD))  {
		xma_check_crd(xcp_node,xmidata);
	}

	if (flush_the_cache) {
		/* now enable cache by method specified in XCP spec */
		save_led_val = v5800csr->csr1;
		v5800csr->csr1 = (FMISS | FCI | save_led_val);
		v5800csr->csr1 = (save_led_val & ~(FMISS | FCI));
		flush_cache();		/* flush the first level cache also */

	}
	return(0);
}


/*
 *	Routine kn5800_log_soft
 *
 *	Description:  This routine logs a soft error for processor pointed
 *	to by the pointer xcp_node.  A maximum of 1 packet are logged every
 * 	15 minutes.
 *
 */

kn5800_log_soft(reg_ptr,sptr) 
	struct kn5800_regs *reg_ptr;
	register struct el_xcpsoft *sptr;
{
	struct el_rec *elrp;
	register struct el_xcp54 *ptr;

	elrp = ealloc(sizeof(struct el_xcp54),EL_PRILOW);
	if (elrp) {
		LSUBID(elrp,ELCT_6200_INT54,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		ptr = &elrp->el_body.el_xcp54;

		ptr->xcp_csr1 = v5800csr->csr1;

		ptr->xcp_dtype = reg_ptr->kn5800_dtype;	
		ptr->xcp_xbe = reg_ptr->kn5800_xbe;	
		ptr->xcp_fadr = reg_ptr->kn5800_fadr;	
		ptr->xcp_gpr = reg_ptr->kn5800_gpr;	
		ptr->xcp_csr2 = reg_ptr->kn5800_csr2;
		ptr->xcp_soft = *sptr;
	
    		EVALID(elrp);	
	}
	else mprintf("log failed\n");


	CURRENT_CPUDATA->cpu_state|= CPU_SOFT_DISABLE;

}



/*
 *	Routine:	xma_check_crd
 *
 *	Discription:	This routine scans all of the Calypso arrays
 *	looking for the board that signaled the CRD error.  When found
 *	the error is logged and CRD's are disabled from that board.
 */
xma_check_crd(xcp,xmidata) 
	struct xcp_reg *xcp;
	struct xmidata *xmidata;
{
	int xminode;
	struct xma_reg *xma;

	xma = (struct xma_reg *) xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,xma++) {

		/* if node present then check to see if it is an 
		   XMA */
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		   ((short) (xma->xma_type) == XMI_XMA)) {

	   		/* check for CRD error and check that CRD 
			   logging for board has not been disabled */
			if ((xma->xma_mecer&(XMA_ECC_CRD_ERROR)) && 
			   ((xma->xma_mctl1&XMA_CTL1_CRD_DISABLE)==0)){
				/* log error */
				log_mem_error(xcp,xma,EL_PRILOW);

				/* clear CRD request */
				xma->xma_mecer = XMA_ECC_CRD_ERROR;
		
				/* disable further CRD's */
				xma->xma_mctl1|=(XMA_CTL1_CRD_DISABLE);
			}

		}
	}	
}




/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

kn5800memenable ()
{
	int xminode;
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	struct xmidata *xmidata;

	xmidata = get_xmi(0);

	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		    ((short) (nxv->xmi_dtype) == XMI_XMA)) {
	
			/* clear disable CRD flag in memory controller */
			((struct xma_reg *)nxv)->xma_mctl1 
					      &= ~(XMA_CTL1_CRD_DISABLE);
		}
	}
	return(0);

}


/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

kn5800setcache(state)
int state;

{
  	if (state) {
		/* enable the cache */
		kn5800_enable_cache();
		return(0);
	}
}


/*
 *
 */
kn5800cachenbl()
{
	cache_state = 0x1;
	return(0);
}




/*
 *
 */
kn5800tocons(c)
	register int c;
{
	register int timeo;

	timeo = ssc_console_timeout;

	while ((ssc_ptr->ssc_ctcs & TXCS_RDY) == 0) {
		if (timeo-- <= 0) {
			return(0);
		}
	}
	ssc_ptr->ssc_ctdb = c;
	return(0);
}



/*
 *
 */
int kn5800badaddr(addr,len)
caddr_t addr;
int len;
{
	register int foo,s,i;	
	register struct bi_nodespace *biptr;
	
#ifdef lint
	len=len;
#endif lint

	s = spl7();
	for (i=0; i < nNVAXBI ; i++) {
		if (biptr = bidata[i].cpu_biic_addr) {
			biptr->biic.biic_err = biptr->biic.biic_err;
			foo = biptr->biic.biic_gpr0;
		}
	}

	foo = bbadaddr(addr,len);

	for (i=0; i < nNVAXBI ; i++) {
		if (biptr = bidata[i].cpu_biic_addr) {
			if ((biptr->biic.biic_err 
				& ~BIERR_UPEN)!=0) foo=1;
			biptr->biic.biic_err = biptr->biic.biic_err;
		}
	}

	kn5800_clear_xbe();
	splx(s);
	return(foo);
}




/*
 *
 */
kn5800reboot() {
	/* clear warm and cold boot flags */
	ccabase->cca_hflag |=(CCA_V_BOOTIP|CCA_V_REBOOT|CCA_V_WARMIP);
}



/*
 * ISIS halt interrupt recieved, for now we just reboot
 */
kn5800halt()
{

	prom_reboot ();
}



/*
 * print processor module info
 */
x3p_init(nxv,nxp,cpup,xminumber,xminode)
char *nxv;
char *nxp;
struct cpusw *cpup;
int xminumber,xminode;
{
#ifdef lint
	nxv=nxv; nxp=nxp;
	cpup = cpup;
	xminumber=xminumber;
	xminode = xminode;
#endif lint

	printf("x3p cpu at xmi%x node %x\n",xminumber,xminode);

}



/*
 * kn5800nexaddr -- return a physical address for a particular nexus.
 *
 * Only 1 XMI bus on ISIS, thus the xminumber is ignored.
 *
 */
kn5800nexaddr(xminumber,xminode) 
int	xminumber;
int	xminode;
{

	return((XMI_START_PHYS + (xminode * 0x80000)));
}



/*
 * ISISumaddr -- return BI 
 */
kn5800umaddr(binumber,binode) 
int	binumber;
int	binode;
{
    return(((int) bidata[binumber].biphys) + 0x400000 + (0x40000 * binode));
}



/*
 * ISISudevaddr - return
 */
kn5800udevaddr(binumber,binode) 
int	binumber;
int	binode;
{
    return (((int) bidata[binumber].biphys)
				+ 0x400000 + 0x3e000 + (0x40000 * binode));

}

/*
 * write buffer flush for kn5800
 */
kn5800_wbflush()
{
	KN5800_WBFLUSH();
	return (0);
}



kn5800_clean_icache(addr, len)
caddr_t	addr;
int	len;
{
	kn5800_cln_icache(addr, len, kn5800_wbflush_addr);
	return (0);
}

kn5800_clean_dcache(addr, len)
caddr_t	addr;
int	len;
{
	kn5800_cln_dcache(addr, len, kn5800_wbflush_addr);
	return (0);
}

kn5800_page_iflush(addr)
caddr_t	addr;
{
	kn5800_pg_iflush(addr, kn5800_wbflush_addr);
	return (0);
}

kn5800_page_dflush(addr)
caddr_t	addr;
{
	kn5800_pg_dflush(addr, kn5800_wbflush_addr);
	return (0);
}

kn5800_flush_cache()
{
	int	s;

	s = splextreme();
	kn5800_flsh_cache(&v5800csr->csr1);
	v5800csr->csr1 &= ~RINVAL;
	splx(s);
	return (0);
}

/* returns the cpu id (XMI node number) */
kn5800_cpuid()
{
	return(v5800csr->csr1 & XMI_NODE_ID); 
}

/* initialization code that the secondary processor must run
   before starting up */

kn5800_init_secondary()
{
	struct xcp_reg *xcp_node;
	register struct xmidata *xmidata ;

	init_ssc();
	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt +
					(v5800csr->csr1 & XMI_NODE_ID);
	xcp_node->xcp_csr2 = xcp_node->xcp_csr2 & ~(0x4000); /* CRDID is set
						   disabled on secondary */
	xcp_node->xcp_xbe = xcp_node->xcp_xbe;

	kn5800_enable_cache();

	if (!CURRENT_CPUDATA->cpu_machdep) 
		/* Allocate memory to store machine check information */
		KM_ALLOC(CURRENT_CPUDATA->cpu_machdep,char *,
			 sizeof(struct el_xcpsoft),KM_DEVBUF,KM_NOW_CL_CO_CA);

	kn5800_enable_softlog();
}

kn5800_enable_softlog()
{
	/*
	 * Enable cache soft error logging
	 */
	CURRENT_CPUDATA->cpu_state &= ~(CPU_SOFT_DISABLE);

	if (memintvl > 0)
		timeout (kn5800_enable_softlog, (caddr_t) 0, memintvl * hz);
	return(0);
}

/*
 * init_ssc -- set up ssc address, set to interrupt at proper level.
 *
 */
init_ssc ()
{
	ssc_ptr = (struct ssc_regs *)PHYS_TO_K1(DEFAULT_SSC_PHYS);
	ssc_ptr->ssc_adc0mask = 0x00000004;	/* required for cache */
/*	ssc_ptr->ssc_ssccr &= (~SSCCR_IPLMASK);	*/
	ssc_ptr->ssc_ssccr |= SSCCR_CPT;	/* enable ^p detect */
}


reprime_ssc_clock()
{
	v5800csr->csr1 &= ~EINTMR;
	v5800csr->csr1 |= EINTMR;
	return (0);
}

/*
 * kn5800_start_clock
 */
kn5800_start_clock()
{
	v5800csr->csr1 |= EINTMR;
	return (0);
}

kn5800_stop_clock()
{
	v5800csr->csr1 &= (~EINTMR);
	v5800csr->csr1 |= EINTMR;
	v5800csr->csr1 &= (~EINTMR);
	return (0);
}

/*
 * taken from cvax.c
 */
cca_setup()
{
	register int i, j;
	register int num_cpu = 0;
	register int stkpaddr;
	int timeout;

	ccabase = (struct cca *)xtob((char *)(prom_getenv ("cca")));

	/* sanity check CCA */
	if ((ccabase->cca_indent0 != 'C') ||
	    (ccabase->cca_indent1 != 'C')) {
		panic("no CCA");
	}

	/* Start processors upto the number configured */
	for (i=0; i < ccabase->cca_nproc; i++ ) {
		if (ccabase->cca_console & (1 << i)) {
			num_cpu++;
		} 
	}
	if( !(ccabase->cca_console & (1 << (v5800csr->csr1 & XMI_NODE_ID)))) 
			    /* if the boot cpu's console bit is not cleared */
		num_cpu++; /* for backward compatibilty with a bug in console
			      ROM. Because of the bug, cca_console was not
			      cleared even after the processor was started */
	return(num_cpu);
}


/*
 * taken from cvax.c
 */
cca_startcpu(cpunum)
int cpunum;
{
	int timeout;

	if (ccabase->cca_console & (1 << cpunum)) {
	    if( (ccabase->cca_enabled & (1 << cpunum)) && get_cpudata(cpunum)) {
	    /*
	     * Init the slave
	     */
	    /* The start address for the kernel is set by the loader to  be
	       0x80030000. If this value is ever changed (through config),
	       this string should also be changed. Since the command is being
	       sent to the CVAX chip and not R3000, use the EXIT command and
	       not 'go'.*/
		    cca_send("EXIT 80030000\r", cpunum);
		    timeout=10;
		    while ( (ccabase->cca_console & (1<<cpunum)) ) { 
			    DELAY(1000000)
				    if (--timeout < 0) break;
		    }
		    if(!(ccabase->cca_console & (1 << cpunum))) {
			    return(1);
		    } 
	   }
	}
	return(0); 
}

cca_send(str,cpunum) 
register char *str;
register int cpunum;
{
	register int index;
	int timeout;

	timeout=10;
	while ( (ccabase->cca_buf[cpunum].flags & RXRDY) != 0) {
		DELAY(1000000);
		if (--timeout < 0) {
			printf("processor %x not ready\n",cpunum);
			return;
		}
	}

	index = 0;
	while (*str != '\0') {
		ccabase->cca_buf[cpunum].rx[index] = *str;
		str++;
		index++;
	}
	ccabase->cca_buf[cpunum].rxlen = index;
	ccabase->cca_buf[cpunum].flags |= RXRDY;

	timeout=10;
	while ( (ccabase->cca_buf[cpunum].flags & RXRDY) != 0) {
		DELAY(1000000);
		if (--timeout < 0) {
			printf("processor %x not ready\n",cpunum);
			return;
		}
	

	}

}		

/*
 * nxaccess for kn5800. This is tricky stuff. The 5800 systems exchange
 * bit 28 and 29 of addresses comming out of the CPU/MMU area to split
 * accesses from KSEG0 and 1 between memory and I/O. This exchange happens
 * for all addresses output from the processor area independent of the
 * virtual address. (In other words, accesses to KSEG0, 1 and 2 as well
 * as KUSEG.) When we map I/O space, upper XMI addresses in the range
 * 0x30000000 to 0x3fffffff are ok as both bit 28 and 29 are set so
 * the exchange is a nop. For nexi in lower xmi slots where the XMI
 * physical address is in the range of 0x20000000 to 0x2fffffff we must
 * exchange 28 and 29 in the pte so that the hardware exchange puts
 * it back. Confused?
 */

#define	XMI_IO_BITS	0x30000000
nxaccess(physa, pte, nexsize)
struct nexus *physa;
register struct pte *pte;
int nexsize;
{
	register int i = btop(nexsize);
	register unsigned pfn;
	register int vpn = (((pte - Sysmap)/4) + btop(K2BASE));

	if (((int)physa & XMI_IO_BITS) == 0x20000000)
		physa = (struct nexus *)((int)physa - 0x10000000);
	pfn = btop(physa);
/*
cprintf("kn5800_nxaccess: phys = 0x%x, pte = 0x%x, size = 0x%x, nptes = 0x%x\n",
physa, pte, nexsize, i);
*/
	do {
	    *(int *)pte++ = ((pfn << PTE_PFNSHIFT)|PG_V|PG_KW|PG_N|PG_M|PG_G);
	    pfn++;
#ifdef PROBE_BUG
	    CKPROBE(tlbdropin(0, ptob(vpn), *(int *)pte));
#else
	    tlbdropin(0, ptob(vpn), *(int *)pte);
#endif
	    vpn++;
	} while (--i > 0);

/**	mtpr(TBIA, 0); What to do for ISIS ?? **/

}


/*
 * Routine to handle trap errors: user-mode ibe & dbe, & all kernel mode traps.
 * We try to recover from user-mode errors and panic on kernel mode errors.
 */


kn5800_trap_error(ep, code, sr, cause, signo)
	register u_int *ep;		/* exception frame ptr */
	register u_int code;		/* trap code (trap type) */
	u_int sr, cause;		/* status and cause regs */
	int *signo;			/* set if we want to kill process */
{
	caddr_t pa;			/* the physical addr of the error */	
	int epc;			/* the EPC of the error */	
	unsigned memreg;		/* memory parity error info */
	int pagetype;          		/* type of page */
	int vaddr;			/* virt addr of error */
	register struct proc *p;	/* ptr to current proc struct */
	struct xcp_reg *xcp_node;
	register struct xmidata *xmidata;
	register struct kn5800_regs *reg;

	xcp_node = (struct xcp_reg *)PHYS_TO_K1(XMI_START_PHYS) +
					(v5800csr->csr1 & XMI_NODE_ID);

	reg = kn5800_regp;
	reg->kn5800_csr1 = v5800csr->csr1;
	reg->kn5800_dtype = xcp_node->xcp_dtype;
	reg->kn5800_xbe = xcp_node->xcp_xbe;
	reg->kn5800_fadr = xcp_node->xcp_fadr;
	reg->kn5800_gpr = xcp_node->xcp_gpr;
	reg->kn5800_csr2 = xcp_node->xcp_csr2;

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
			if (SHAREDPG(pa)) {
				kn5800_memerr(ep, reg, EL_PRISEVERE);
				kn5800_consprint(MEMPKT, ep);
				panic("memory parity error in shared page");
			} else {
				kn5800_memerr(ep, reg, EL_PRIHIGH);
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
				kn5800_memerr(ep, reg, EL_PRISEVERE);
				kn5800_consprint(MEMPKT, ep);
				panic("memory parity error in kernel mode");
			} else {
				kn5800_logesrpkt(ep, reg, EL_PRISEVERE);
				kn5800_consprint(ESRPKT, ep);
				panic("bus timeout");
			}
			break;
		case EXC_CPU:
			kn5800_logesrpkt(ep, reg, EL_PRISEVERE);
			kn5800_consprint(ESRPKT, ep);
			panic("coprocessor unusable");
			break;
		case EXC_RADE:
		case EXC_WADE:
			kn5800_logesrpkt(ep, reg, EL_PRISEVERE);
			kn5800_consprint(ESRPKT, ep);
			panic("unaligned access");
			break;
		default:
			kn5800_logesrpkt(ep, reg, EL_PRISEVERE);
			kn5800_consprint(ESRPKT, ep);
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
kn5800_logesrpkt(ep, ptr, priority)
register u_int *ep;			/* exception frame ptr */
struct kn5800_regs *ptr;		/* pointer to saved processor register */
int priority;				/* for pkt priority */
{
	struct el_rec *elrp;

	elrp = ealloc(sizeof(struct el_esr), priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_ESR,ELESR_5800,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		elrp->el_body.elesr.elesr.el_esr5800.esr_cause = ep[EF_CAUSE];
		elrp->el_body.elesr.elesr.el_esr5800.esr_epc = ep[EF_EPC];
		elrp->el_body.elesr.elesr.el_esr5800.esr_status = ep[EF_SR];
		elrp->el_body.elesr.elesr.el_esr5800.esr_badva = ep[EF_BADVADDR];
		elrp->el_body.elesr.elesr.el_esr5800.esr_sp = ep[EF_SP];
		elrp->el_body.elesr.elesr.el_esr5800.x3p_csr1 = ptr->kn5800_csr1;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_dtype = ptr->kn5800_dtype;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_xbe = ptr->kn5800_xbe;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_fadr = ptr->kn5800_fadr;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_gpr = ptr->kn5800_gpr;
		elrp->el_body.elesr.elesr.el_esr5800.x3p_csr2 = ptr->kn5800_csr2;
		EVALID(elrp);
	}
}


/*
 * Print error packet to the console.
 * This is only done when we are about to panic on the error.
 */
kn5800_consprint(pkt, ep)
int pkt;		/* error pkt: Error & Stat Regs / memory pkt */
register u_int *ep;	/* exception frame ptr */
{
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
	return;
}

bbssi(bitpos,base)
register u_long bitpos;                   /* bitmask */
register u_long *base;                    /* interlock target address */

{
	register u_long data;
	register int s;

	/*
	 * Lock and read target address.
	 */
	s = splextreme();
	data = interlock_read(base);
	/*
	 * Check if bit set.  If no, set it while location is locked, then unlock location.
         * If already set, unlock location.
	 */
	if ((data & (1<<bitpos)) == 0) {
		data |= (1<<bitpos);
		*base = data; /* To sync cache with memory */
		unlock_write(base, data);
		splx(s);
		return(1);
	} 
	else {
		*base = data; /* Hardware folks insist on this */
		unlock_write(base, data);
		splx(s);
		return(0);

	}
}

bbcci(bitpos,base)
register u_long bitpos;                   /* bitmask */
register u_long *base;                    /* interlock target address */

{
	register u_long data;
	register int s;

	/*
	 * Lock and read target address.
	 */
	s = splextreme();
	data = interlock_read(base);
	/*
	 * Check if bit clear.  If no, clear it while location is locked, 
	 * then unlock location. If already clear, unlock location.
	 */
	if ((data & (1<<bitpos))) {
		data &= ~(1<<bitpos);
		*base = data; /* To sync cache with memory */
		unlock_write(base, data);
		splx(s);
		return(1);
	} 
	else {
		*base = data; /* Hardware folks insist on this */
		unlock_write(base, data);
		splx(s);
		return(0);
	}

}
