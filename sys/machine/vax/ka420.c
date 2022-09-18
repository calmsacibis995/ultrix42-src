#ifndef lint
static	char	*sccsid = "@(#)ka420.c	4.2	(ULTRIX)	10/10/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987,88 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:	ka420.c
 *
 * 18-Apr-90    Tony Griffiths, TaN Engineering (Australia)
 *
 *      Add code to allow config of the dsh (DST32/DSH32 Sync) device
 *      driver on the uVAX-3100.
 *
 * 02-Dec-89	Fred Canter
 *	Added code to config "sp" pseudo driver for user devices.
 *
 * 27-Nov-89    Paul Grist
 *      added frame_type argument to logmck() calls.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 03-May-89 -- gmm  (v3.1 merge)
 *	Added support for TMII (MicroVAX 3100). If cpu_sub_subtype indicates
 *	SB_TMII, set cvs_cache_on to 1 to indicate only primary cache
 *
 * 15-Feb-89 -- afd (Al Delorey)
 *	Re-sync with VAX 3.0 for 3.2 pool merge.
 *
 * 31-Jan-89 -- map (Mark Parenti)
 *	Change include syntax for merged pool.
 *
 * 25-Sep-88 -- fred (Fred Canter)
 *	Remove sz_setcache debug code and cleanup comments.
 *
 * 07-Jun-88 -- fred (Fred Canter)
 *	Back out SCSI/SCSI controller address change (hardware
 *	changed their mind).
 *
 * 07-Jun-88 -- fred (Fred Canter)
 *	Bug fix for cvs_cache_on global undefined if VAX420 not configured.
 *
 * 06-jun-88 -- fred (Fred Canter)
 *	Use different address for SCSI/SCSI controller.
 *
 * 31-May-88 -- afd (Al Delorey)
 *	Fixed machine check & CRD interrupt (cache tag parity) error
 *	    handling code.
 *	Log errors and recover when possible.
 *	TODO: terminate user-mode processes.
 *
 * 19-May-88 -- fred (Fred Canter)
 *	Minor code and comment cleanup.
 *	Made cvs_cache_on permanent feature.
 *	Added autoconf support for SCSI devices.
 *
 * 11-Nov-87 -- fred (Fred Canter)
 *	Created this machine dependent code file for CVAXstar
 *	(based on CVAX ka650.c).
 *
 **********************************************************************/

#include "../h/types.h"
#include "../h/time.h"
#include "../machine/cons.h"
#include "../machine/clock.h"
/*
#include "uba.h"
*/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vmmac.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/reboot.h"
#include "../h/devio.h"
#include "../h/errlog.h"
#include "../io/uba/qdioctl.h"	/* for QD_KERN_UNLOOP below */

#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/ioa.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../../machine/common/cpuconf.h"

extern int ws_display_type;	/* type of console on workstations */

/*
 * TODO: it would be nice to clean up the arrangement of these #defines.
 *
 * Parity Control Register (PAR_CTL) bit definitions
 *
 */

#define	PAR_CTL_DPEN	0x00000001	/* DMA parity enable (bit 0) */
#define	PAR_CTL_CPEN	0x00000002	/* CPU parity enable (bit 1) */
#define	PAR_CTL_DMA	0x01000000	/* LANCE chip DMA control (bit 24) */

/*
 * Time limits for errors.  Recoverable errors are fatal if 3 errors occur
 * within the time period.  All times are given in 10 ms units (100ths of secs)
 * to be used with the 10ms units of the standard VAX TODR.
 */
#define TIME_THRESH	100		/* 1 sec max for most error types */
#define TIME_THRESH_C1	6000		/* 60 sec max for 1st lev cache errs */
#define TIME_THRESH_C2	30000		/* 5 mins max for 2nd lev cache errs */

/*
 * These structures are used to keep track of the frequency of errors.
 * We keep the time of the last 2 errors for each category.
 * When a third error occurs, if the time elapsed since the "prev" one
 *   is less than 1 second, then we got 3 errors (prev, last, current)
 *   within 1 second.
 */
struct cfpa_errcnt {		/* machine checks 1 thru 4 (& 5-8) */
	u_int	cfpa_last;	/* time of most recent CFPA error */
	u_int	cfpa_prev;	/* time of previous CFPA error */
};

struct dal_errcnt {		/* MSER_MCD */
	u_int	dal_last;	/* time of most recent DAL parity error */
	u_int	dal_prev;	/* time of previous DAL parity error */
};

#define CADR_SETMASK 0xC0
#define SET_BOTH (CVAX_SEN2 | CVAX_SEN1)
#define SET_TWO  CVAX_SEN2
#define SET_ONE  CVAX_SEN1
#define SET_NONE 0

struct cache_errcnt {		/* MSER_MCC */
	u_int	cache_last;	/* time of most recent 1st level cache parity error */
	u_int	cache_prev;	/* time of previous 1st level cache parity error */
};

struct tag_errcnt {		/* CACR_CPE */
	u_int	tag_last;	/* time of most recent 2nd lev cache tag parity err */
	u_int	tag_prev;	/* time of prev 2nd lev cache tag parity err */
};

/* timers & counters for errors:    used with following error bit... */

struct cfpa_errcnt cfpa_errcnt;		/* machine checks 1 thru 4 (& 5 - 8) */
struct dal_errcnt dal_errcnt;		/* MSER_MCD */
struct cache_errcnt cache_errcnt;	/* MSER_MCC */
struct tag_errcnt tag_errcnt;		/* CACR_CPE */

extern int cache2_state;		/* state of 2nd level cache: 0=off, 1=on */
int ka420_mchkprog = 0;		/* machine check in progress */

/*
 * Machine Check codes for CVAXstar CPU.
 * Defines for number of machine check codes (in following array)
 *    and the index number of first disjoint code.
 */
#define NMCcVAXSTAR 15
#define MCcVAXSTARDISJ 11
struct mcCVAXSTARframe {
	int	mc1_bcnt;			/* byte count == 0xc */
	int	mc1_summary;			/* summary parameter */
	int	mc1_vap;			/* most recent virtual addr */
	int	mc1_internal_state1;		/* internal state 1 */
	int	mc1_internal_state2;		/* internal state 2 */
	int	mc1_pc; 			/* trapped pc */
	int	mc1_psl;			/* trapped psl */
};

/*
 * Machine Check codes for CVAXstar CPU.
 */
char *mcCVAXSTAR[] = {
	"unknown machine check type code",		/* 0 */
	"CFPA protocol error",				/* 1 */
	"CFPA reserved instruction",			/* 2 */
	"CFPA unknown error",				/* 3 */
	"CFPA unknown error",				/* 4 */
	"process PTE in P0 space (TB miss)",		/* 5 */
	"process PTE in P1 space (TB miss)",		/* 6 */
	"process PTE in P0 space (M = 0)",		/* 7 */
	"process PTE in P1 space (M = 0)",		/* 8 */
	"undefined interrupt ID code",			/* 9 */
	"impossible microcode state (MOVCx)",		/* 10 */
	"read bus error, normal read",			/* 80 */
	"read bus error, SPTE, PCB, or SCB read",	/* 81 */
	"write bus error, normal write",		/* 82 */
	"write bus error, SPTE, or PCB write",		/* 83 */
};


/*
 * Configure routine for CVAXstar (VS420).
 *
 * Please forgive the "unibus flavor" of this code.
 * Much of it was copied from the MicroVAX-II configure code,
 * which was copied from the unibus VAX configure code.
 * Fred Canter -- 11/9/87
 */

extern	int	nNUBA;
extern  int	dkn;
extern	struct	uba_driver	sdcdriver;
extern	struct	uba_driver	stcdriver;
extern	struct	uba_driver	scsidriver;
extern	struct	uba_driver	ssdriver;
extern	struct	uba_driver	lndriver;
extern	struct	uba_driver	smdriver;
extern	struct	uba_driver	sgdriver;
extern	struct	uba_driver	shdriver;
extern	struct	uba_driver	spdriver;
extern  struct  uba_driver      dshdriver;  /* DST/DSH32 Sync Device driver */

extern	int cache_state;
extern	int cvs_cache_on;  
extern	int cpu_sub_subtype;

ka420conf()
{
	register int i;
	extern int fl_ok;
	struct uba_device *ui;
	struct uba_ctlr *um;
	struct uba_hd *uhp;
	struct uba_driver *udp;
	extern int catcher[256];
	int (**ivec)();
	caddr_t addr;

	/*
	 * TODO: would be nice to print CVAX chip revision level
	 *       and, possibly, other nice to know information.
	 */
	if(cpu_sub_subtype == SB_TMII) {
		printf("KA41-A/B ");  /* We don't distinguish between
					 Timeshare (A) and Server (B) systems*/
		cvs_cache_on = 1; /* Only Primary cache present */
	}
	else
		printf("KA420 processor ");
	if( fl_ok )
		printf("with an FPU\n");
	else
		printf("without an FPU\n");
	/*
	 * We now have the scb set up enough so we can handle
	 * interrupts some if they are waiting to happen.
	 */
	(void) spl0();

	uhp = &uba_hd[numuba];
	uhp->uh_vec = SCB_UNIBUS_PAGE(numuba);
	uhp->uh_lastiv = 0x200;
	for (i=0; i<(uhp->uh_lastiv/4); i++)
		uhp->uh_vec[i] = scbentry(&catcher[i*2], SCB_ISTACK);

	/*
	 * Say uba0 alive (so installation sizer will see it).
	 */
	config_set_alive("uba", 0);

	/*
	 * Check each unibus mass storage controller.
	 * For each one which is potentially on this uba,
	 * see if it is really there, and if it is record it and
	 * then go looking for slaves.
	 *
	 * For CVAXstar/PVAX, only config: sdc, stc, scsi controllers.
	 */
	for (um = ubminit; udp = um->um_driver; um++) {
		if(um->um_alive)
			continue;
		if (udp == &sdcdriver)
			addr = (caddr_t)0x200c0000;
		else if (udp == &stcdriver)
			addr = (caddr_t)0x200c0080;
		else if (udp == &scsidriver) {
			if (um->um_ctlr == 0) {
				addr = (caddr_t)0x200c0080;
			}
			else if (um->um_ctlr == 1) {
				addr = (caddr_t)0x200c0180;
			}
			else {
				continue;
			}
		}
		else
			continue;
		cvec = 0x200;
		i = (*udp->ud_probe)(addr, um->um_ctlr);
		if (i == 0)
			continue;
		um->um_ubanum = numuba;
		config_fillin(um);
		printf(" csr 0x%x ", addr);
		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec 0x%x, ipl 0x%x\n", cvec, br);
		um->um_alive = 1;
		um->um_hd = &uba_hd[numuba];
		um->um_addr = (caddr_t)addr;
		um->um_physaddr = (caddr_t)addr;
		udp->ud_minfo[um->um_ctlr] = um;
		for (ivec = um->um_intr; *ivec; ivec++) {
			um->um_hd->uh_vec[cvec/4] =
			    scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		/*
		 * Only allows CVAXstar/PVAX ST506 disks/tapes
		 * and SCSI disks/tapes/CDROM.
		 */
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if ((ui->ui_driver != udp) ||
			    (ui->ui_alive) ||
			    (um->um_ctlr != ui->ui_ctlr))
				continue;
			if ((*udp->ud_slave)(ui)) {
				ui->ui_alive = 1;
				ui->ui_ctlr = um->um_ctlr;
				ui->ui_ubanum = numuba;
				ui->ui_hd = &uba_hd[numuba];
				ui->ui_addr = (caddr_t)addr;
				ui->ui_physaddr = addr; /* rpb: swap on boot */
				if(ui->ui_dk && dkn < DK_NDRIVE)
					ui->ui_dk = dkn++;
				else
					ui->ui_dk = -1;
				ui->ui_mi = um;
				/* ui_type comes from driver */
				udp->ud_dinfo[ui->ui_unit] = ui;
				printf("%s%d at %s%d slave %d",
				    ui->ui_devname, ui->ui_unit,
				    udp->ud_mname, um->um_ctlr, ui->ui_slave);
				(*udp->ud_attach)(ui);
				/* device type sometimes printed by attach */
				printf("\n");
			}
		}
	}

	/*
	 * Configure remaining CVAXstar/PVAX devices, no others.
	 */
	for (ui=ubdinit; udp=ui->ui_driver; ui++) {
		if (udp == &ssdriver)
			addr = (caddr_t)0x200a0000;
		else if (udp == &smdriver)
			addr = (caddr_t)0x200f0000;
		else if (udp == &sgdriver)
			addr = (caddr_t)0x3c000000;
		else if (udp == &shdriver)
			addr = (caddr_t)0x38000000;
                else if (udp == &dshdriver)     /* DST32/DSH32 sync driver  */
                        addr = (caddr_t)0x38000000;
		else if (udp == &spdriver)	/* user device pseudo driver */
			addr = ui->ui_addr;	/* get CSR from config line */
		else
			continue;
		/*
		 * Following should never happen on a CVAXstar.
		 */
		if ((ui->ui_ubanum != numuba && ui->ui_ubanum != '?') ||
		    ui->ui_alive || ui->ui_slave != -1)
			continue;
		cvec = 0x200;
		i = (*udp->ud_probe)(addr);
		if(i == 0)
			continue;
                if (udp == &dshdriver)      /* DST/DSH32 driver returns CSR */
                        addr = (caddr_t)i;  /* address from probe() routine */
		ui->ui_ubanum = numuba;
		config_fillin(ui);
		printf(" csr 0x%x ", addr);
		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec 0x%x, ipl 0x%x\n", cvec, br);
		ui->ui_hd = &uba_hd[numuba];
		for (ivec=ui->ui_intr; *ivec; ivec++) {
			ui->ui_hd->uh_vec[cvec/4] =
				scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		ui->ui_alive = 1;
		ui->ui_addr = (caddr_t)addr;
		ui->ui_physaddr = 0;
		ui->ui_dk = -1;
		udp->ud_dinfo[ui->ui_unit] = ui;
		(*udp->ud_attach)(ui);
	}
	/*
	 * Configure remaining CVAXstar/PVAX devices, no others.
	 */

 	config_set_alive("ibus", 0);
	for (ui=ubdinit; udp=ui->ui_driver; ui++) {
		if (udp == &lndriver && ui->ui_unit==0)
			addr = (caddr_t) 0x200e0000;
		else
			continue;
		/*
		 * Following should never happen on a CVAXstar.
		 */
		if (ui->ui_ubanum != -1 ||
		    ui->ui_alive || ui->ui_slave != -1)
			continue;
		cvec = 0x200;
		i = (*udp->ud_probe)(addr);
		if(i == 0)
			continue;
		ui->ui_adpt = 0;
		config_fillin(ui);
		printf(" csr 0x%x ", addr);
		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec 0x%x, ipl 0x%x\n", cvec, br);
		ui->ui_hd = &uba_hd[numuba];
		for (ivec=ui->ui_intr; *ivec; ivec++) {
			ui->ui_hd->uh_vec[cvec/4] =
				scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		ui->ui_alive = 1;
		ui->ui_addr = (caddr_t)addr;
		ui->ui_physaddr = 0;
		ui->ui_dk = -1;
		udp->ud_dinfo[ui->ui_unit] = ui;
		(*udp->ud_attach)(ui);
	}
	/*
	 * Set parity control register DPEN and CPEN bits
	 * and clear DMA bit. This enables CPU and DMA parity
	 * checking and selects the lo address (0 - 16MB) for
	 * LANCE chip DMA transfers.
	 */
	((struct nb_regs *)nexus)->nb_par_ctl = PAR_CTL_CPEN | PAR_CTL_DPEN;
	/*
	 * Tell the console program that we've booted and
	 * that we would like to restart if the machine panics.
	 */
	((struct nb_regs *)nexus)->nb_cpmbx=RB_VS_RESTART;
	return(0);
}


/*
 * Bits in CACR: Cache Control Register (2nd level cache) (cvqcb->cvq2_cacr)
 */
#define CACR_CEN	0x00000010	/* <4>  Cache enable */
#define CACR_CPE	0x00000020	/* <5>  Cache Parity Error */
					/* NOTE: VS3100 spec calls this TPE */

/*
 * Bits in MSER: Memory System Error Register (IPR 39 or 0x27)
 */
#define MSER_DAL	0x00000040	/* <6> DAL or 2nd level cache data store parity */
#define MSER_MCD	0x00000020	/* <5> mcheck due to DAL parity error */
#define MSER_MCC	0x00000010	/* <4> mcheck due to 1st lev cache parity */
#define MSER_DAT	0x00000002	/* <1> data parity in 1st level cache */
#define MSER_TAG	0x00000001	/* <0> tag parity in 1st level cache */

/*
 * Bits in CADR: Cache Disable Register (IPR 37)
 */
#define CADR_STMASK	0xF0		/* 1st level cache state mask */
#define CVAX_SEN2	0x00000080	/* <7> 1st level cache set 2 enabled */
#define CVAX_SEN1	0x00000040	/* <6> 1st level cache set 1 enabled */
#define CVAX_CENI	0x00000020	/* <5> 1st level I-stream caching enabled */
#define CVAX_CEND	0x00000010	/* <4> 1st level D-stream caching enabled */

/*
 * Bits in PSL: Processor Status Longword (for mcheck recovery)
 */
#define PSL_FPD		0x08000000	/* <27> First Part Done flag */

/*
 * Bits in Internal Status Info 2: (for mcheck recovery)
 */
#define IS2_VCR		0x00008000	/* <15> VAX Can't Restart flag */

/*
 * This routine sets the cache to the state passed:  enabled/disabled.
 * The CVAX chip has a first level cache enabled through IPR CADR.
 * The KA420 processor has a second level cache enabled through
 *     local register CACR.
 * Also set state flags in the error count structures.
 */

/*
 * This variable defines the state of the
 * 1st and 2nd level caches, i.e., bit 0 is set
 * if 1st level cache is on and bit 1 is set if
 * 2nd level cache is on. The normal state is
 * both caches on (cvs_cache_on = 3).
 */
extern	int	cvs_cache_on;

ka420setcache(state)
int state;
{
	register int i;
	register struct nb6_regs *cdp = (struct nb6_regs *)cvscachemem;

	/*
	 * Enable 1st level cache.
	 * Write to CADR flushes 1st level cache (if DIA bit = 0).
	 */
	mtpr (CADR, state);
	if (cvs_cache_on & 2) {
		/*
		 * Flush 2nd level cache before enabling.
		 */
		((struct nb_regs *)nexus)->nb_cacr = CACR_CPE;
		for (i=0; i<CVS_CACHE_SIZE; i++)
			cdp->nb_cvscache[i] = (u_long)0;
		((struct nb_regs *)nexus)->nb_cacr |= CACR_CEN;
		cache2_state = 1;
	}
	return(0);
}

/*
 * Enable cache.  Both D_stream & I-stream, both Set-1 and Set-2.
 * These bits are in the IPR CADR for the CVAX chip.
 *
 * The variable cvs_cache_on defines the state of the
 * 1st and 2nd level caches, i.e., bit 0 is set
 * if 1st level cache is on and bit 1 is set if
 * 2nd level cache is on. The normal state is
 * both caches on (cvs_cache_on = 3).
 */

extern	int	cache_state;


ka420cachenbl()
{
	if (cvs_cache_on & 1)
	    cache_state = (CVAX_CEND | CVAX_CENI | CVAX_SEN1 | CVAX_SEN2);
	else
	    cache_state = (CVAX_CEND | CVAX_CENI);
	return(0);
}

/*
 * CVAXstar has no console register to write to,
 * but routine is called from machdep and locore,
 * and the system will panic if no tocons routine.
 */

ka420tocons(c)
	register int c;
{
	return(0);
}

short *ka420nexaddr(ioadpt,nexnum)
	int nexnum,ioadpt;
{

	return(NEXUVII);

}

char *
ka420umaddr(ioadpt,nexnum)
	int ioadpt,nexnum;
{
	return(QMEMVAXSTAR);
}

/*
 * Machine check handler.
 * Called from locore thru the cpu switch in response to a trap at SCB 4
 * We recover from any that we can if hardware "retry" is possible.
 */

ka420machcheck (cmcf)
caddr_t cmcf;
{
	register struct nb_regs *nb_regs;
	register struct nb6_regs *cdp;	/* ptr to 2nd level cache diag space */
	register int i;			/* counter for cache clearing code */
	register struct mcCVAXSTARframe *mcf;
	register int recover;		/* set to 1 if error is recoverable */
	register u_int time;		/* from TODR */
	u_int type;			/* machine check type */
	int cpunum;		/* 0 for uniprocessor */
	int retry;		/* set to 1 if hardware can retry the instr */
	int ws_disp;		/* type of work station display for ioctl */

	/*
	 * Disable 2nd level cache to protect execution of machine check code.
	 * NOTE: Do NOT put any executable code in this routine before
	 *	 disabling the cache, the cache may be what caused the mcheck.
	 */
	nb_regs = (struct nb_regs *)nexus;
	nb_regs->nb_cacr &= ~CACR_CEN;

	/*
	 * Set up pointer needed to clear 2nd level cache data storage.
	 */
	cdp = (struct nb6_regs *)cvscachemem;

	/*
	 * Do not allow recursive machine check.
	 * Halt the processor, then a restart should get a dump.
	 */
	if (ka420_mchkprog == 0)
		ka420_mchkprog = 1;
	else
		asm("halt");
	type = ((struct mcframe	 *) cmcf)->mc_summary;
	mcf = (struct mcCVAXSTARframe *) cmcf;
	recover = 0;
	cpunum = 0;
	retry = 0;
	/*
	 * First note the time; then switch on machine check type.
	 */
	time = ka420readtodr();

	switch (type) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		if (((mcf->mc1_psl & PSL_FPD) == 0) && ((mcf->mc1_internal_state2 & IS2_VCR) == 0))
			retry = 1;
		/*
		 * CFPA errors (1-4) and memory management errors (5-8).
		 * Re-enable 2nd level cache to log in correct state.
		 * If fewer than 3 errors in 1 time period, try to recover
		 * else we will crash.
		 * todo: if in user-mode only crash the user process.
		 */
		if (cache2_state) {
			nb_regs->nb_cacr |= CACR_CPE;
			for (i=0; i<CVS_CACHE_SIZE; i++)
				cdp->nb_cvscache[i] = (u_long)0;
			nb_regs->nb_cacr |= CACR_CEN;
		}
		if (time - cfpa_errcnt.cfpa_prev > TIME_THRESH) {
			if (retry)
				recover = 1;
		}
		logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
		if (recover) {
			cfpa_errcnt.cfpa_prev = cfpa_errcnt.cfpa_last;
			cfpa_errcnt.cfpa_last = time;
		} else {
			ka420consprint(2,type,mcf);
		}
		break;
	case 9:
	case 10:
		/*
		 * These are non-recoverable:
		 * Re-enable 2nd level cache to log in correct state.
		 * Log the mcheck & print mcheck frame to console.
		 * todo: if in user-mode only crash the process.
		 */
		if (cache2_state) {
			nb_regs->nb_cacr |= CACR_CPE;
			for (i=0; i<CVS_CACHE_SIZE; i++)
				cdp->nb_cvscache[i] = (u_long)0;
			nb_regs->nb_cacr |= CACR_CEN;
		}
		logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
		ka420consprint(2,type,mcf);
		break;
	case 0x80:
	case 0x81:
		if (((mcf->mc1_psl & PSL_FPD) != 0) ||
		    (((mcf->mc1_psl & PSL_FPD) == 0) && ((mcf->mc1_internal_state2 & IS2_VCR) == 0)))
			retry = 1;
		/*
		 * There are several possible causes for this mcheck.
		 * First re-enable the 2nd level cache if its not
		 *   a cache error and the  cache was enabled before
		 *   we entered mcheck (for recovery and so we log it
		 *   in the state it was in when mcheck occured).
		 * Check for which error caused the mcheck and take
		 *   appropriate action.
		 */
		if (((mfpr(MSER) & MSER_MCD) == 0) && cache2_state) {
			nb_regs->nb_cacr |= CACR_CPE;
			for (i=0; i<CVS_CACHE_SIZE; i++)
				cdp->nb_cvscache[i] = (u_long)0;
			nb_regs->nb_cacr |= CACR_CEN;
		}
		if ((mfpr(MSER) & MSER_MCD) != 0) {
			/*
			 * DAL Bus Parity Err/2nd Level Cache Data Parity Err.
			 * Determine if its main memory or 2nd level cache
			 *   by counting the frequency of errors.  If we get 3
			 *   in the time-out period (even after flushing the
			 *   cache) then we assume its main memory.
			 */
			if (time - dal_errcnt.dal_prev <= TIME_THRESH_C2) {
				/*
				 * Got 3 errors within 1 time period.
				 * This must be a real parity memory error (not
				 *   cache), so crash the process or the system.
		 		 * todo: if in user-mode only crash the user process.
		 		 * todo: map out the bad page.
				 */
				dal_errcnt.dal_last = 0;
				time = 0;
				recover = 0;
			} else {
				/*
				 * Got fewer than 3 errors within 1 time period.
				 * This may be a second level cache error, so
				 *   flush/re-enable the cache & count the errors.
		 		 * todo: if we can't retry the instruction
				 *	 if in usermode only crash user process.
				 */
				nb_regs->nb_cacr |= CACR_CPE;
				for (i=0; i<CVS_CACHE_SIZE; i++)
					cdp->nb_cvscache[i] = (u_long)0;
				nb_regs->nb_cacr |= CACR_CEN;
				mprintf("2nd level cache re-enabled by software on mcheck\n");
				if (retry)
					recover = 1;
			}
			/*
			 * Log the machine check, and the error status regs.
			 * If we can recover update the times,
			 * else print errors on the console.
			 * Note: if we can't recover log it as a memory error.
			 * Last, clear the error bits.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
			if (recover) {
				ka420logesrpkt(recover);
				dal_errcnt.dal_prev = dal_errcnt.dal_last;
				dal_errcnt.dal_last = time;
			} else {
				ka420logmempkt(recover, ka420getpa(mcf, type));
				ka420consprint(2,type,mcf);
				ka420consprint(4,type,mcf);
			}
			mtpr(MSER,1);
		} else if ((mfpr(MSER) & MSER_MCC) != 0) {
			/*
			 * 1st Level Cache Parity Err (CPU disables & flushes).
			 * If recovery is possible, do so, else log and quit.
			 */
			if (retry) {
				recover = 1;
				logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
				mtpr(MSER,1);
			} else {
		 	 	/* todo: if in user-mode only crash the user process. */
				logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
				ka420consprint(2,type,mcf);
				mtpr(MSER,1);
				break;		/* out of switch */
			}
			/*
			 * Recovery procedures:
			 */
			if (time - cache_errcnt.cache_prev <= TIME_THRESH_C1) {
				/*
				 * Got 3 errors within 1 time period.
				 * Action depends on prior state of 1st level cache:
				 * Reenable whichever cache sets were NOT on.
				 * And reset the timers to require 3
				 * errs/time period with the new cache setting.
				 */
				switch (cache_state & CADR_SETMASK) {
				case SET_BOTH:
					cache_state &= ~(CVAX_SEN2);
					mtpr (CADR, cache_state);
					printf("1st Level Cache, Set 2 DISABLED, Set 1 Enabled by software on mchk\n");
					cache_errcnt.cache_last = 0;
					time = 0;
					break;
				case SET_ONE:
					cache_state |= (CVAX_SEN2);
					cache_state &= ~(CVAX_SEN1);
					mtpr (CADR, cache_state);
					printf("1st Level Cache, Set 1 DISABLED, Set 2 Enabled by software on mchk\n");
					cache_errcnt.cache_last = 0;
					time = 0;
					break;
				case SET_TWO:
					cache_state &= ~(CVAX_SEN2);
					/*
					 * Set I & D stream for 2nd level
					 * cache operation.
					 */
					mtpr (CADR, cache_state);
					printf("1st Level Cache Completely DISABLED by software on mchk\n");
					cache_errcnt.cache_last = 0;
					time = 0;
					break;
				case SET_NONE:
					recover = 0;	/* don't recover */
					ka420consprint(2,type,mcf);
					printf("Got a 1st Level Cache Parity Error with the 1st Level Cache Disabled!\n");
					break;
				default:
					cache_state = (CVAX_CEND | CVAX_CENI | CVAX_SEN1 | CVAX_SEN2);
					mtpr (CADR, cache_state);
					break;
				}
			} else {
				/*
				 * Fewer than 3 errs in 1 time period,
				 * reenable whichever cache sets were on.
				 */
				switch (cache_state & CADR_SETMASK) {
				case SET_BOTH:
					mtpr (CADR, cache_state);
					mprintf("1st Level Cache, Re-enabled by software on mchk\n");
					break;
				case SET_ONE:
					mtpr (CADR, cache_state);
					mprintf("1st Level Cache, Set 1 Re-enabled, Set 2 left Disabled by software on mchk\n");
					break;
				case SET_TWO:
					mtpr (CADR, cache_state);
					mprintf("1st Level Cache, Set 2 Re-enabled, Set 1 left Disabled by software on mchk\n");
					break;
				case SET_NONE:
					recover = 0;	/* don't recover */
					ka420consprint(2,type,mcf);
					printf("Got a 1st Level Cache Parity Error with the 1st Level Cache Disabled!\n");
					break;
				default:
					cache_state = (CVAX_CEND | CVAX_CENI | CVAX_SEN1 | CVAX_SEN2);
					mtpr (CADR, cache_state);
					mprintf("1st Level Cache, Re-enabled by software on mchk\n");
					break;
				}
			}
			/*
			 * Since we can recover update the times,
			 */
			cache_errcnt.cache_prev = cache_errcnt.cache_last;
			cache_errcnt.cache_last = time;
		} else {
			/*
			 * Undefined Machine check 0x80, 0x81.
			 * Log the mcheck & ESR Packet.
			 * We can't recover so print errors on the console.
		 	 * todo: if in user-mode only crash the user process.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
			mprintf("No primary error flag - unspecified error type\n");
			ka420logesrpkt(recover);
			ka420consprint(2,type,mcf);
			cprintf("No primary error flag - unspecified error type\n");
			ka420consprint(3,0,0);
		}
		break;
	case 0x82:
	case 0x83:
		/*
		 *   Re-enable (after flushing) the 2nd level cache if cache
		 *   was enabled before we entered mcheck (to log it in the
		 *   state it was in when mcheck occured).
		 */
		if (cache2_state) {
			nb_regs->nb_cacr |= CACR_CPE;
			for (i=0; i<CVS_CACHE_SIZE; i++)
				cdp->nb_cvscache[i] = (u_long)0;
			nb_regs->nb_cacr |= CACR_CEN;
		}
		/*
		 * We can't recover so print errors on the console.
		 * Last, clear the error bits.
		 */
		logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
		ka420logesrpkt(recover);
		ka420consprint(2,type,mcf);
		ka420consprint(3,0,0);
		break;
	default:
		/*
		 *   Re-enable (after flushing) the 2nd level cache if cache
		 *   was enabled before we entered mcheck (this is so we
		 *   log it in the state it was in when mcheck occured).
		 */
		if (cache2_state) {
			nb_regs->nb_cacr |= CACR_CPE;
			for (i=0; i<CVS_CACHE_SIZE; i++)
				cdp->nb_cvscache[i] = (u_long)0;
			nb_regs->nb_cacr |= CACR_CEN;
		}
		/*
		 * Unrecognized mcheck: these are non-recoverable.
		 * Log the mcheck & ESR packet.
		 * Also print to the console.
		 */
		logmck((int *)cmcf, ELMCKT_CVAX,  cpunum, recover);
		ka420logesrpkt(recover);
		type = 0;
		ka420consprint(2,type,mcf);
		ka420consprint(3,0,0);
		break;
	}
	if (!recover)
		panic ("ka420 mchk");
	ka420_mchkprog = 0;
	return(0);
}

/*
 * Get the Physical Addr where the memory parity error occured so we can log it.
 *
 * For mcheck 0x80, virtual (+4) is given in mcheck frame, convert to physical.
 * For mcheck 0x81, physical (+4) is given in mcheck frame.
 *
 * The macro to determine if an address is in system space may appear in
 *     vmmac.h in a future version of ULTRIX.
 */

#define SYSADDR(v) (v & 0x80000000)

ka420getpa(mcf, type)
	register struct mcCVAXSTARframe *mcf;
	int type;
{
	struct pte *pte;	/* PTE ptr to obtain physical addr */
	int pa;			/* the physical address where mem err occured */

	if (type == 0x80) {
		if (SYSADDR (mcf->mc1_vap -4)) {
			pte = &(Sysmap[btop((int)(mcf->mc1_vap -4) & ~VA_SYS)]);
		} else {
			/*
			 * user space address
			 */
			pte = vtopte(u.u_procp, btop(mcf->mc1_vap -4));
		}
		/*
		 * Validity check the pte before picking up the page address
		 */
		if ((!SYSADDR((int)pte)) || (!pte->pg_v))
			pa = -1;
		else
			pa = (int)ptob(pte->pg_pfnum);
	} else
		pa = mcf->mc1_vap -4;
	return (pa);
}

/*
 * Log 2nd level cache tag parity error.
 * Log packet 3 (error status registers).
 *
 * Traps through SCB vector 54: correctable memory errors.
 *
 * These errors are recoverable.
 */
ka420crderr()
{
	register struct nb_regs *nb_regs = (struct nb_regs *)nexus;
	register struct nb6_regs *cdp = (struct nb6_regs *)cvscachemem;
	register int i;
	int recover;	/* set to 1 if we can recover from this error */
	u_int time;

	recover = 0;
	time = ka420readtodr();
	if (nb_regs->nb_cacr & CACR_CPE) {
		/*
		 * Cache Tag Parity error (Cache automatically disabled):
		 * Will cause a cache miss so a memory read will be done
		 *   after we REI from the interrupt.
		 * Action depends on prior 2nd level cache state:
		 *    was on:  Recover
		 *    was off: Panic
		 */
		if (cache2_state) {
			/*
			 * Second Level Cache was on:
			 * Action depends on number of errors:
			 *    3 within 1 time period:   Leave Cache disabled
			 *    < 3 within 1 time period: Flush Cache & re-enable
			 */
			mprintf("Tag parity interrupt: 2nd level cache tag parity\n");
			recover = 1;
			if (time - tag_errcnt.tag_prev <= TIME_THRESH_C2) {
				/*
				 * Cache was on, 3 errs in 1 time period
				 */
				printf("2nd Level Cache DISABLED by software on tag parity\n");
				ka420logesrpkt(recover);
				nb_regs->nb_cacr |= CACR_CPE;
				cache2_state = 0;
				tag_errcnt.tag_last = 0;
				time = 0;
			} else {
				/*
				 * Cache was on, < 3 errs in 1 time period
				 * Flush and re-enable cache
				 */
				ka420logesrpkt(recover);
				nb_regs->nb_cacr |= CACR_CPE;
				for (i=0; i<CVS_CACHE_SIZE; i++)
					cdp->nb_cvscache[i] = (u_long)0;
				nb_regs->nb_cacr |= CACR_CEN;
				mprintf("2nd Level Cache re-enabled by software\n");
			}
			tag_errcnt.tag_prev = tag_errcnt.tag_last;
			tag_errcnt.tag_last = time;
		} else {
			/*
			 * Cache was off: no recover.
			 * NOTE: consprint() sets up display for cprintf here!
			 */
			ka420logesrpkt(recover);
			ka420consprint(3,0,0);
			printf("2nd Level Cache Tag Parity Error occured with Cache Disabled!\n");
			nb_regs->nb_cacr |= CACR_CPE;
			panic("ka420 Cache Tag Parity error");
		}
	} else {
		/*
		 * Catch-all: vector 54 interrupt without CACR-CPE bit set.
		 * Dismiss the error and REI.
		 */
		mprintf("Tag parity interrupt without tag parity bit\n");
	}
	return(0);
}

/*
 * Print error packet to the console.
 * This is only done when we are about to panic on the error.
 *
 * Note: side-effect.
 *	If console is a graphics device, ioctl is done to force kernel printfs
 *	directly to the screen.
 */

ka420consprint(pkt, type, mcf)
	int pkt;	/* error pkt desired:	2 = mcheck frame
			   			3 = error status registers
			   			4 = memory error packet */
	int type;				/* machine check type */
	register struct mcCVAXSTARframe *mcf;	/* mcheck frame pointer */
{
	register struct nb_regs *nb_regs = (struct nb_regs *)nexus;
	register int i;
	int ws_disp;

	/*
	 * If console is a graphics device,
	 * force printf messages directly to screen.
	 */
	if (ws_display_type) {
	    ws_disp = ws_display_type << 8;
	    (*cdevsw[ws_display_type].d_ioctl)(ws_disp, QD_KERN_UNLOOP, 0, 0);
	}

	switch (pkt) {
	case 2:
		cprintf("\nmachine check %x: ", type);
		/*
		 * Types are disjoint. Have to convert some to linear range.
		 */
		if (type >= 0x80)
			type = type - 0x80 + MCcVAXSTARDISJ;
		cprintf("%s\n", mcCVAXSTAR[type]);
		cprintf("\tcode\t= %x\n", mcf->mc1_summary);
		cprintf("\tmost recent virtual addr\t=%x\n", mcf->mc1_vap);
		cprintf("\tinternal state 1\t=%x\n", mcf->mc1_internal_state1);
		cprintf("\tinternal state 2\t=%x\n", mcf->mc1_internal_state2);
		cprintf("\tpc\t= %x\n", mcf->mc1_pc);
		cprintf("\tpsl\t= %x\n\n", mcf->mc1_psl);
		break;
	case 3:
		cprintf("\tcacr\t= %x\n", nb_regs->nb_cacr);
		cprintf("\tcadr\t= %x\n", mfpr(CADR));
		cprintf("\tmser\t= %x\n", mfpr(MSER));
		break;
	case 4:
		cprintf("\tcacr\t= %x\n", nb_regs->nb_cacr);
		cprintf("\tcadr\t= %x\n", mfpr(CADR));
		cprintf("\tmser\t= %x\n", mfpr(MSER));
		cprintf("\tphys addr\t= 0x%x\n", ka420getpa(mcf, type));
		break;
	default:
		cprintf("bogus ka420consprint\n");
		break;
	}
}

/*
 * Log Error & Status Registers
 */

ka420logesrpkt(priority)
	int priority;		/* for pkt priority */
{
	struct el_rec *elrp;
	register struct nb_regs *nb_regs = (struct nb_regs *)nexus;

	switch (priority) {
	case 0:	/* non-recoverable mchecks */
		priority = EL_PRISEVERE;
		break;
	case 1:	/* recoverable mchecks */
		priority = EL_PRIHIGH;
		break;
	case 2:	/* recoverable CRDs (cache tag parity errors) */
		priority = EL_PRILOW;
		break;
	}
	elrp = ealloc(sizeof(struct el_esr420), priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_ESR420,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		elrp->el_body.elesr420.esr_cacr = nb_regs->nb_cacr;
		elrp->el_body.elesr420.esr_cadr = mfpr(CADR);
		elrp->el_body.elesr420.esr_mser = mfpr(MSER);
		EVALID(elrp);
	}
}

/*
 * Log Error & Status Registers (and physical address where error occured)
 * as a memory error packet, so uerf can find it as a main memory error.
 */

ka420logmempkt(recover, pa)
	int recover;		/* for pkt priority */
	int pa;			/* physical addr where memory err occured */
{
	struct el_rec *elrp;
	register struct el_mem *mrp;

	elrp = ealloc(EL_MEMSIZE,recover ? EL_PRIHIGH : EL_PRISEVERE);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_420,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		mrp = &elrp->el_body.elmem;
		mrp->elmem_cnt = 1;
		mrp->elmemerr.cntl = 1;
		mrp->elmemerr.type = ELMETYP_PAR;
		mrp->elmemerr.numerr = 1;
		mrp->elmemerr.regs[0] = ((struct nb_regs *)nexus)->nb_cacr;
		mrp->elmemerr.regs[1] = mfpr(CADR);
		mrp->elmemerr.regs[2] = mfpr(MSER) ;
		mrp->elmemerr.regs[3] = pa;
		EVALID(elrp);
	}
}

ka420readtodr()
{

	u_int s,todr;
	struct tm tm;
	register struct nb_regs *nb_regs = (struct nb_regs *)nexus;

	/*
	 * All CVAXstar toy clock registers (including NVR)
	 * read/write bits 2 - 9 instead of 0 - 7.
	 */
	/*
	 * Copy the toy register contents into tm so that we can
	 * work with. The toy must be completely read in 2.5 millisecs.
	 *
	 *
	 * Wait for update in progress to be done.
	 */
	while( nb_regs->nb_toycsra & (QBT_UIP << 2) )
			;
	s = spl7();
	tm.tm_sec = ((nb_regs->nb_toysecs >> 2) & 0xff);
	tm.tm_min = ((nb_regs->nb_toymins >> 2) & 0xff);
	tm.tm_hour = ((nb_regs->nb_toyhours >> 2) & 0xff);
	tm.tm_mday = ((nb_regs->nb_toyday >> 2) & 0xff);
	tm.tm_mon = ((nb_regs->nb_toymonth >> 2) & 0xff);
	tm.tm_year = ((nb_regs->nb_toyyear >> 2) & 0xff);
	splx( s );

	todr = toyread_convert(tm);

	return(todr);
}


ka420writetodr(yrtime)
u_int yrtime;
{
	struct tm xtime;
	int s;
	register struct nb_regs *nb_regs = (struct nb_regs *)nexus;

	toywrite_convert(&xtime,yrtime);

	nb_regs->nb_toycsrb = (QBT_SETUP << 2);
	s = spl7();
	nb_regs->nb_toysecs = (xtime.tm_sec << 2);
	nb_regs->nb_toymins = (xtime.tm_min << 2);
	nb_regs->nb_toyhours = (xtime.tm_hour << 2);
	nb_regs->nb_toyday = (xtime.tm_mday << 2);
	nb_regs->nb_toymonth = (xtime.tm_mon << 2);
	nb_regs->nb_toyyear = (xtime.tm_year << 2);
	/*
 	 * Start the clock again.
 	 */
	nb_regs->nb_toycsra = (QBT_SETA << 2);
	nb_regs->nb_toycsrb = (QBT_SETB << 2);
	splx( s );
}
