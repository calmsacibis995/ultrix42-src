#ifndef lint
static char *sccsid = "@(#)ka60.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88,89 by			*
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
 * Modification History:	ka60.c
 *
 * 27-Nov-89    Paul Grist
 *      added frame_type argument to logmck() calls.
 *
 * 03-Oct-89	Darrell A. Dunnuck (darrell)
 *	A 4 processor firefox now prints the correct processor name --
 *	VAXstation 3540, and also fixed a problem with ka60memerr()
 *	where the condition in an if statement was incorrect such
 *	that we always assumed an FQAM, and never executed the FTAM
 *	code.
 *
 * 13-Sep-89	Darrell A. Dunnuck (darrell)
 *	Added a call to ka60custconfig() to allow config support of
 *	3rd-party M-bus devices.
 *
 * 14-Aug-89	Darrell A. Dunnuck (darrell)
 *	Fixed a bug in ka60memerr() that was causing startcpu() to
 *	hang.
 *
 * 17-Jul-89	Darrell A. Dunnuck (darrell)
 *	Handle the Q-bus errors on the new FQAM (FQAM -II).  Clean up
 *	code by removing code within #ifdef notdef -- #endif.
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
 * 12-May-89	darrell
 *	Merged V3.1 changes.
 *
 * 16-Feb-89	darrell
 *	Added a panic to the ka60memerr() routine.  If the frozen bit
 *	in the buscsr register of any FBIC is set after memerr interrupt
 *	we will panic.
 *
 * 14-Feb-89	darrell
 *	Added code to ka60crderr(), ka60memerr(), and added the routines
 *	ka60clrmbint() and ka60setmbint().
 *
 * 20-Jan-89	darrell
 *	Changed the EL_ESR60 entry to EL_ESR650 to use the already
 *	defined structure, and made the Mbus I/O Error printout
 *	an mprintf.
 *
 * 20-Jan-89	darrell
 *	Added error logger support for for memory error, Mbus Errors
 *	and Error and Status Registers.  Enabled more if the machine
 *	check handler.
 *
 * 18-Nov-88	darrell
 *	Cleanup.
 *
 * 28-Sep-88	darrell
 *	Changed all writes of fbicsr to be read_modify_write to preserve
 *	the diagnostic selftest information in the LEDS on all hardware
 *	modules
 *
 * 01-Sep-88	darrell
 *	Added code to enable I and D stream caching in the CVAX
 *	internal cache.
 *
 * 16-Jun-88	darrell
 *	Removed definitions that were intended to be I/O space 
 *	structure definitions that were actually allocating space in BSS.
 *
 * 07-Jun-88	darrell
 *	Code to support ibus is complete.
 *
 * 12-7-87	darrell
 *	Copied this file from ka650.c, and started gutting it to boot
 *	Firefox.
 *
 **********************************************************************/

#include "../h/types.h"
#include "../h/time.h"
#include "../machine/cons.h"
#include "../machine/clock.h"
#include "uba.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/dmap.h"
#include "../h/reboot.h"
#include "../h/devio.h"
#include "../h/errlog.h"
#include "../h/config.h"

#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/ioa.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/cvax.h"
#include "../machine/ka60.h"
#include "../io/uba/fcreg.h"
#include "../io/uba/fgioctl.h"

/* save record of sbis present for sbi error logging for 780 and 8600 */
extern long sbi_there;	/* bits 0-15 for nexi,sbi0; 16-31 for nexi on sbi1*/
extern int ws_display_type;	/* type of console on workstations */
extern	struct uba_driver fcdriver;


extern int cache_state;
unsigned long ka60_memcon;	/* memory config (memcsr's 0-15) */
unsigned int ka60_module;	/* which module had a hard memory error */
int ka60_mchkprog = 0;		/* machine check in progress */

extern Xka60memerr;
extern Xipintr;

/*
 * per M-bus node structure (mb_node) space allocation
 */
struct mb_node mbus_nodes[9];

/*
 * Declare (allocate) space for the Firefox local register spaces.
 * The structures are in ka60.h.
 */
struct ssc_regs cvqssc[];	/* SSC regs				      */
struct cvqbm_regs cvqbm[];	/* Qbus map registers			      */
struct cqbic_regs ffqregs[];	/* Firefox CQBIC registers		      */
struct fc_regs ffcons[];	/* Firefox console registers		      */
struct ffcrom_regs ffcrom[];	/* Firefox console ROM registers	      */
struct fbic_regs ffiom[];	/* Firefox FBIC register map		      */
int fqamcsr;			/* FQAM CSR				      */

extern long cpu_fbic_addr;
extern struct cpusw *cpup;	/* pointer to cpusw entry */

/*
 * ka60 configuration routine.
 * Maps local register space, clears map registers set by VMB,
 *   calls unifind for device configuration, set console program restart flag.
 */

ka60conf()
{
	register unsigned int *mapaddr;	/* phys address of Qbus map registers */
	register struct mb_node *mbp;
	register struct fbic_regs *fbaddr;
	register int i;
	struct uba_device *ui;
	struct uba_ctlr *um;
	struct uba_hd *uhp;
	struct uba_driver *udp;
	int alive;
	int base = (int)scb.scb_ipl14;
	char *iov;			/* virtual address in I/O space	      */
	char *iop;			/* physical address in I/O space      */
	extern int fl_ok;
	int pri_cpuid;			/* CPUID from Primary CPU	      */

	pri_cpuid = mfpr(CPUID);
	printf("Primary CPU is %x ", pri_cpuid);
	if (fl_ok)
		printf("\n");
	else
		printf(", without an FPU\n");
	/*
	 * We now have the scb set up enough so we can handle
	 * interrupts if any are pending.
	 */
	(void) spl0();

	printf("CCA Base = 0x%x\n", cvqssc->ssc_cca_addr);
	cpu_avail = cca_setup() + 1;

	/*
	 * Map the first page of the console ROM
	 */
	nxaccess((long *)(MB_SLOT0_BASE + ((pri_cpuid & 0x1e) << 23) + FF_BSROM_OFF), FFCROMmap, 512);
	if((*cpup->badaddr)((caddr_t)ffcrom, 4)) {
	    cprintf("Base System ROM not found\n");
	}
	/*
	 * Print CVAX chip microcode rev level and
	 * KA60 processor firmware rev level
	 */
	if (cpu_avail > 2)
	    printf("VAXstation 3540,  ");
	else
	    printf("VAXstation 3520,  ");
	printf("%d processors available\n", cpu_avail);
	printf("	CPU microcode rev = %d, processor firmware rev = %d\n",
		(mfpr(SID)) & 0xff, ffcrom->ffcrom_firmrev);
	/*
	 * Enable all interrupts on the primary CPU
	 */

	fbaddr = ((struct mb_node *)ka60getmbnode(cpu_fbic_addr,
		0x01010108))->mb_vaddr;
	fbaddr->f_fbicsr |= FFCSR_EXCAEN | FFCSR_IRQE_3 | FFCSR_IRQE_1 |
		FFCSR_PRI0EN |FFCSR_IRQE_0 | FFCSR_LEDS_OFF | FFCSR_NORMAL;

	/*
	 * Configure the I/O system
	 */
	/*
	 * Say uba0 alive (so installation sizer will see it).
	 */
	config_set_alive("ibus", 0);
	config_set_alive("ibus", 1);
	config_set_alive("ibus", 2);
	config_set_alive("ibus", 3);
	config_set_alive("ibus", 4);
	config_set_alive("ibus", 5);
	config_set_alive("ibus", 7);

	/*
	 * Match up the Mbus "devices" with what is in the config file
	 */
	mbp = mbus_nodes;
	for (mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
	    switch (mbp->mb_modtype & 0xff) {
		case FMOD_QBUS:
		    /*
		     * Clear the map registers that were set by VMB (8192 of them).
		     * This is necessary so that the QDSS sizing will work when
		     *   there is more than one QDSS.
		     */
		    mapaddr = (unsigned int *)cvqbm->cvqbm_uba.cqba.qb_map;
		    for (i = 0; i < 8192; i++) {
			*mapaddr = i;
			mapaddr++;
		    }
		    sbi_there |= 1<<0;
		    printf("Q22 bus\n");
		    uba_hd[0].uba_type = UBAUVII;
		    iov = (char *)cvqbm;
		    iop = (char *)cpup->nexaddr(0,3);
		    unifind ((&((struct cvqbm_regs *)iov)->cvqbm_uba.uba), 
			(&((struct cvqbm_regs *)iop)->cvqbm_uba.uba), 
			qmem[0],
			cpup->umaddr(0,0),
			cpup->pc_umsize,
			cpup->udevaddr(0,0),
			QMEMmap[0], cpup->pc_haveubasr,(long) 0, (long) 0);

		    /*
		     * Clear bits from the nxm probe and autoconf mchecks.
		     */
		    ffqregs->cq_dser |= DSER_CLEAR;
		    enafbiclog();
		    ka60setscbvec(((struct fbic_regs *)mbp->mb_vaddr), 0x17,
			(int)&Xka60memerr, SCB_ISTACK);
		    ka60setscbvec(((struct fbic_regs *)mbp->mb_vaddr), 0x16,
			(int)&Xka60memerr, SCB_ISTACK);
		    break;

		case FMOD_GRAPHICS:
		    alive |= ib_config_dev(mbp->mb_vaddr, mbp->mb_physaddr, mbp->mb_slot, "fg", (base + (mbp->mb_slot * 0x20) + 0x0));
		    ka60setscbvec(((struct fbic_regs *)mbp->mb_vaddr), 0x17,
			(int)&Xka60memerr, SCB_ISTACK);
		    break;

		case FMOD_IO:
		    alive |= ib_config_dev(ffcons, (MBUS_BASEADDR(mbp->mb_slot) + FF_DZ_OFF), mbp->mb_slot, "fc", (base + (mbp->mb_slot * 0x20) + 0x8));
		    alive |= ib_config_dev(cvqni, (MBUS_BASEADDR(mbp->mb_slot) + FF_NI_OFF), mbp->mb_slot, "ln", (base + (mbp->mb_slot * 0x20) + 0x4));
		    alive |= ib_config_cont(cvqmsi, (MBUS_BASEADDR(mbp->mb_slot) + FF_SI_OFF), mbp->mb_slot, "sii", (base + (mbp->mb_slot * 0x20) + 0x0));
		    ka60setscbvec(((struct fbic_regs *)mbp->mb_vaddr), 0x17,
			(int)&Xka60memerr, SCB_ISTACK);
		    break;

		case FMOD_CPU:
		    ka60setscbvec(((struct fbic_regs *)mbp->mb_vaddr), 0x17,
			(int)&Xka60memerr, SCB_ISTACK);
		    break;

		case FMOD_MEM:
		    ka60setscbvec(((struct fbic_regs *)mbp->mb_vaddr), 0x17,
			(int)&Xka60memerr, SCB_ISTACK);
		    break;

		default:
		    if (ka60custconfig(mbp) != 0) {
		    	printf("Unknown MBUS node modtype 0x%x\n",
			    mbp->mb_modtype);
		    }
		    break;
	    }
	}

	/*
	 * We're now done probing the I/O busses, so enable interrupts
	 */
	fbaddr = ((struct mb_node *)ka60getmbnode(cpu_fbic_addr, 0x01010108))->mb_vaddr;
	fbaddr->f_fbicsr |= FFCSR_EXCAEN | FFCSR_IRQE_X | FFCSR_LEDS_OFF |
		 FFCSR_PRI0EN | FFCSR_HALTEN | FFCSR_NORMAL;

	/*
	 * Record memory configuration (memcsr's 0-15) in "ka60_memcon".
	 */
	/*
	 * Tell the console program that we've booted and
	 * that we speak English and would like to restart
	 * if the machine panics.
	 */
#define RB_CV_RESTART 0x1	/* Restart on Console Halt */
	cvqssc->ssc_cpmbx |= RB_CV_RESTART;
	return(0);
}

/*
 * This routine sets the cache to the state passed:  enabled/disabled.
 * Enable the  first level cache.  The second
 * level cache is write back, and was turned on by
 * the console, and should not be turned off without
 * having been flushed.  Flushing this cache with
 * memory management enabled is not an easy task.
 *
 * Enable both I-stream and D-stream.
 */

ka60setcache(state)
int state;
{

	mtpr (CADR, state);
	return(0);
}

ka60cachenbl()
{

	cache_state = 0xfc;
	return(0);

}

/*
 * Enable CRD interrupts.
 * This runs at regular (15 min) intervals, turning on the interrupt.
 * It is called by the timeout call in memenable in machdep.c
 * The interrupt is turned off, in ka60crderr(), when 3 error interrupts
 *   occur in 1 time period.  Thus we report at most once per memintvl
 * (15 mins).
 */
ka60memenable()
{
	register struct mb_node *mbp;
	struct fmdc_regs *fmp;

	/*
	 * enable SBE interrupts on all memory modules
	 */
	for (mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
	    if ((mbp->mb_modtype & FMOD_INTERFACE) != FMOD_FMDC)
		continue;
	    fmp = (struct fmdc_regs *)mbp->mb_vaddr;
	    fmp->fm_fmdcsr &= ~FMDCSR_INH_SBE_REPORT;
	}
}

ka60tocons(c)
	register int c;
{
	return(0);
}

/*
 * Local Registers on the ka650 are mapped as 6 separate sections
 * since they are so disjoint.
 *
 * "nexnum" is passed in as the section (1 to 6) that we are
 * currently mapping.  The address constants are defined in ka650.h
 */
short *
ka60nexaddr(ioadpt,nexnum)
	int ioadpt, nexnum;
{
	switch (nexnum) {
	case 1:
		return(CVQMERRADDR);
		break;
	case 2:
		return(CVQCBADDR);
		break;
	case 3:
		return(CVQBMADDR);
		break;
	case 4:
		return(CVQSSCADDR);
		break;
	case 5:
		return(CVQCACHEADDR);
		break;
	case 6:
		return(CVQIPCRADDR);
		break;
	case 7:
		return(CVQROMADDR);
		break;
	default:
		return(CVQMERRADDR);
		break;
	}
}


char *
ka60umaddr(ioadpt,nexnum)
	int ioadpt,nexnum;
{
	return(QMEMCVQ);
}

u_short *
ka60udevaddr(ioadpt,nexnum)
	int ioadpt,nexnum;
{
	return(QDEVADDRCVQ);
}

/*
 * Machine check handler.
 * Called from locore thru the cpu switch in response to a trap at SCB 4
 * We recover from any that we can if hardware "retry" is possible.
 */

ka60machcheck (cmcf)
caddr_t cmcf;
{
	register u_int type;
	register struct mcCVAXframe *mcf;
	register int *cacheptr;		/* ptr to flush 2nd level cache */
	register int *cacheend; 	/* ptr to end of 2nd level cache */
	register int recover;		/* set to 1 if we can recover from this error */
	register u_int time;		/* from TODR */
	int cpunum;	/* 0 for uniprocessor */
	int retry;	/* set to 1 if the hardware can retry the instr */
	int ws_disp;	/* type of work station display for ioctl call */

	/*
	 * Caches on Firefox are not disabled on a machine check.  This is because
	 * Firefox caches use a write back snoopy cache algorithm.
	 */
	/*
	 * Do not allow recursive machine check.
	 * Halt the processor, then a restart should get a dump.
	 */
	if (ka60_mchkprog == 0)
		ka60_mchkprog = 1;
	else {
		asm("halt");
	}
	type = ((struct mcframe	 *) cmcf)->mc_summary;
	mcf = (struct mcCVAXframe *) cmcf;
	recover = 0;
	cpunum = 0;
	retry = 0;
	/*
	 * First note the time; then determine if hardware retry is
	 * possible, which will be used for the recoverable cases.
	 */
	time = ka60readtodr();

	switch (type) {
	case 1:
	case 2:
	case 3:
	case 4:
		if (time - cfpa_errcnt.cfpa_prev > TIME_THRESH) {
			if (retry)
				recover = 1;
		}
		logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
		if (recover) {
			cfpa_errcnt.cfpa_prev = cfpa_errcnt.cfpa_last;
			cfpa_errcnt.cfpa_last = time;
		} else {
			ka60consprint(2,type,mcf);
		}
		break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
		ka60consprint(2,type,mcf);
		break;
	case 0x80:
	case 0x81:
		/*
		 * There are several possible causes for this mcheck.
		 * Check for which error caused the mcheck and take
		 *   appropriate action.
		 */
		if ((mfpr(MSER) & FF_MSER_MCD) != 0) {
			/*
			 * If console is a graphics device,
			 * force printf messages directly to screen.
			 */
			if (ws_display_type) {
				ws_disp = ws_display_type << 8;
				(*cdevsw[ws_display_type].d_ioctl)(ws_disp, QD_KERN_UNLOOP, 0, 0);
			}
			/*
			 * Log the machine check, and the error status regs.
			 * If we can recover update the times,
			 * else print errors on the console.
			 * Last, clear the error bits.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka60logesrpkt(recover);
			if (recover) {
				cdal_errcnt.cdal_prev = cdal_errcnt.cdal_last;
				cdal_errcnt.cdal_last = time;
			} else {
				ka60consprint(2,type,mcf);
				ka60consprint(3,0,0);
			}
			mtpr(MSER,1);
		} else if ((mfpr(MSER) & FF_MSER_MCC) != 0) {
			/*
			 * 1st Level Cache Parity Err (CPU disables & flushes).
			 * If recovery is possible, do so, else log and quit.
			 */
			if (retry) {
				recover = 1;
				logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
				mtpr(MSER,1);
			} else {
				logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
				ka60consprint(2,type,mcf);
				mtpr(MSER,1);
				break;		/* out of switch */
			}
			/*
			 * Since we can recover update the times,
			 */
			cache_errcnt.cache_prev = cache_errcnt.cache_last;
			cache_errcnt.cache_last = time;
		} else {
			/*
			 * Undefined Machine check 0x80, 0x81.
			 * Log the mcheck, ESR Packet, & Mem Packet.
			 * We can't recover so print errors on the console.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			mprintf("No primary error flag - unspecified error type\n");
			ka60logesrpkt(recover);
			ka60logmempkt(recover);
			ka60consprint(2,type,mcf);
			cprintf("No primary error flag - unspecified error type\n");
			ka60consprint(3,0,0);
			ka60consprint(4,0,0);
		}
		break;
	case 0x82:
	case 0x83:
		/*
		 * What is logged & printed depends on the cause of the error.
		 * If we can't recover print errors on the console.
		 * Last, clear the error bits.
		 */
		/*
		 * Unspecific mcheck 0x82 or 0x83: log everything.
		 * We can't recover so print errors on console.
		 */
		logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
		mprintf("No primary error flag - unspecified error type\n");
		ka60logesrpkt(recover);
		ka60logmempkt(recover);
		ka60consprint(2,type,mcf);
		printf("No primary error flag - unspecified error type\n");
		ka60consprint(3,0,0);
		ka60consprint(4,0,0);
		break;
	default:
		/*
		 * Unrecognized mcheck: these are non-recoverable.
		 * Log the mcheck, err status regs & memerr packets
		 * Also print to the console.
		 */
		logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
		ka60logesrpkt(recover);
		ka60logmempkt(recover);
		type = 0;
		ka60consprint(2,type,mcf);
		ka60consprint(3,0,0);
		ka60consprint(4,0,0);
		break;
	}
	if (!recover) {
		/* might want to log FBICs here */
		panic ("mchk");
	}
	ka60_mchkprog = 0;
	return(0);
}


/*
 * Firefox memerrs are Mbus I/O errors.  They come in at ipl 1d
 * from the cpu fbic, and ipl 17 from all the other fbics in the system.
 * CQBIC errors will come in at ipl 16, and come here also.
 */
extern int cold;
ka60memerr()
{
        register struct mb_node *mbp;
	register struct fbic_regs *fbp;
	int frozen = 0;

	if (!cold) {
	    /*
	     * Need to check for CQBIC errors
	     * and report them
	     */
	    mbp = mbus_nodes;
	    if ((mbp->mb_flags & FBIC_MAPPED) &&
	    	(mbp->mb_vaddr->f_modtype & FMOD_QBUS)) {
		if ((mbp->mb_vaddr->f_cpuid & 0x3) == FCPU_PROCB) {
		    /*
		     * Handle FTAM errors
		     */
		    if(ffqregs->cq_dser) {
			/*
			 * log the CQBIC error
			 */
			ka60logesrpkt(EL_PRIHIGH);
			/*
			 * Clear the CQBIC error bits
			 */
			ffqregs->cq_dser = 0xffffffff;
		    }
		}
		else {
		    /*
		     * Must be a FQAM
		     */
		    /*
		     * If the Dump Error bit is set, the error is
		     * fatal.
		     */
		    if(fqamcsr & FQCSR_DUMP_ERR) {
			panic("Qbus Adapter Dump Error");
		    }
		}
	    }

	    /*
	     * Make a quick pass through the FBICs to see if
	     * any of the BUSCSR registers are frozen.  If the
	     * frozen bit is cleared, the FBIC error information
	     * is frozen. Only log errors if the error information
	     * is frozen..
	     */
	    for (mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
	    	if(~mbp->mb_vaddr->f_buscsr & FBCSR_FRZN)
	    		frozen++;
	    }
	    if (frozen) {
	    	ka60logfbicpkt(EL_PRIHIGH);
	    	mprintf("Mbus I/O error\n");
	    	panic("Mbus I/O Error");
	    }
	}
	enafbiclog();
}


/*
 * Log CRD memory errors in kernel buffer:
 *
 * Traps through SCB vector 54: correctable memory errors.
 *
 * These errors are recoverable.
 */
ka60crderr()
{
	register struct mb_node *mbp;
	register u_int time;		/* from TOY clock */
	struct fmdc_regs *fmp;

	time = ka60readtodr();
	mprintf("Corrected Read Data interrupt\n");
	ka60logfbicpkt(EL_PRILOW);
	if(time - crd_errcnt.crd_prev <= TIME_THRESH) {
	    /*
	     * disable SBE reporting on all modules
	     */
	    mprintf("Hi-Rate CRD log\n");
	    for (mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
	        if ((mbp->mb_modtype & FMOD_INTERFACE) != FMOD_FMDC)
		    continue;
	        fmp = (struct fmdc_regs *)mbp->mb_vaddr;
	        fmp->fm_fmdcsr |= FMDCSR_INH_SBE_REPORT;
	    }
	}
	crd_errcnt.crd_prev = crd_errcnt.crd_last;
	crd_errcnt.crd_last = time;

	/*
	 * clear syndrome bits and error address
	 */
	for (mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
	    if ((mbp->mb_modtype & FMOD_INTERFACE) != FMOD_FMDC)
		continue;
	    fmp = (struct fmdc_regs *)mbp->mb_vaddr;
	    fmp->fm_eccsynd0 = 0;
	    fmp->fm_eccaddr0 = 0;
	    fmp->fm_eccsynd1 = 0;
	    fmp->fm_eccaddr1 = 0;
	}
}

/*
 * Print error packet to the console.
 * This is only done when we are about to panic on the error.
 *
 * Note: side-effect.
 *	If console is a graphics device, ioctl is done to force kernel printfs
 *	directly to the screen.
 */

ka60consprint(pkt, type, mcf)
	int pkt;	/* error pkt desired:	2 = mcheck frame
			   			3 = error status registers
			   			4 = memory CSRs */
	int type;	/* machine check type (for pkt type 2) */
	register struct mcCVAXframe *mcf; /* mcheck frame pointer (for type 2)*/
{
	register int i;
	register struct mb_node *mbp = mbus_nodes;
	register struct fmdc_regs *fmdcp;
	int ws_disp;

	/*
	 * If console is a graphics device,
	 * force printf messages directly to screen.
	 */
	if (ws_display_type) {
	    ws_disp = ws_display_type << 8;
	    (*cdevsw[ws_display_type].d_ioctl)(ws_disp, FG_KERN_UNLOOP, 0, 0);
	}

	switch (pkt) {
	case 2:
		printf("\nmachine check %x: ", type);
		/*
		 * Types are disjoint. Have to convert some to linear range.
		 */
		if (type >= 0x80)
			type = type - 0x80 + MCcVAXDISJ;
		printf("%s\n", mcCVAX[type]);
		printf("\tcode\t= %x\n", mcf->mc1_summary);
		printf("\tmost recent virtual addr\t=%x\n", mcf->mc1_vap);
		printf("\tinternal state 1\t=%x\n", mcf->mc1_internal_state1);
		printf("\tinternal state 2\t=%x\n", mcf->mc1_internal_state2);
		printf("\tpc\t= %x\n", mcf->mc1_pc);
		printf("\tpsl\t= %x\n\n", mcf->mc1_psl);
		break;
	case 3:
		if((mbp->mb_modtype & FMOD_QBUS) == FMOD_QBUS) {
		    if ((mbp->mb_vaddr->f_cpuid & 0x3) == FCPU_PROCB) {  /* FTAM */
			printf("\tdser\t= %x\n",ffqregs->cq_dser);
			printf("\tmear\t= %x\n", ffqregs->cq_mear);
			printf("\tsear\t= %x\n", ffqregs->cq_sear);
		    }
		    if ((mbp->mb_vaddr->f_cpuid & 0x3) == FCPU_PROCA) {  /* FQAM */
			/* Log FQAM data here */
			;
		    }
		}
		printf("\tcadr\t= %x\n", mfpr(CADR));
		printf("\tmser\t= xx\n", mfpr(MSER));
		break;
	case 4:
		for (mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
		    if ((mbp->mb_modtype & FMOD_CLASS) == FMOD_FMDC) {
			fmdcp = (struct fmdc_regs *)mbp->mb_vaddr;
			if (fmdcp->fm_fmdcsr & FMDC_ERROR) {
			    if ((fmdcp->fm_eccsynd0 & FMDC_MBE) ||
				(fmdcp->fm_eccsynd1 & FMDC_MBE)) {
				printf("Multiple Bit Error, Mbus Slot %d\n",
				    MBUS_SLOT(mbp->mb_physaddr));
			    }
			    else if ((fmdcp->fm_eccsynd0 & FMDC_SBE) ||
				(fmdcp->fm_eccsynd1 & FMDC_SBE)) {
				printf("Multiple Bit Error, Mbus Slot %d\n",
				    MBUS_SLOT(mbp->mb_physaddr));
			    }
			    printf("\tmodtype \t = 0x%x\n", fmdcp->fm_modtype);
			    printf("\tbuscsr  \t = 0x%x\n", fmdcp->fm_buscsr);
			    printf("\tbusctl  \t = 0x%x\n", fmdcp->fm_busctl);
			    printf("\tbusaddr \t = 0x%x\n", fmdcp->fm_busaddr);
			    printf("\tbusdat  \t = 0x%x\n", fmdcp->fm_busdat);
			    printf("\tfmdcsr  \t = 0x%x\n", fmdcp->fm_fmdcsr);
			    printf("\tbaseaddr\t = 0x%x\n", fmdcp->fm_baseaddr);
			    printf("\teccaddr0\t = 0x%x\n", fmdcp->fm_eccaddr0);
			    printf("\teccaddr1\t = 0x%x\n", fmdcp->fm_eccaddr1);
			    printf("\teccsynd0\t = 0x%x\n", fmdcp->fm_eccsynd0);
			    printf("\teccsynd1\t = 0x%x\n", fmdcp->fm_eccsynd1);
			}
		    }
		}
		break;
	default:
		printf("\nCase %d not implemented\n", pkt);
		break;
	}
}

/*
 * Log Error & Status Registers (packets 3 & 5 of KA650 error spec)
 */

ka60logesrpkt(priority)
	int priority;		/* for pkt priority */
{
	register struct mb_node *mbp = mbus_nodes;
	struct el_rec *elrp;

	switch (priority) {
	case 0:	/* non-recoverable mchecks & memory errs */
		priority = EL_PRISEVERE;
		break;
	case 1:	/* recoverable mchecks */
		priority = EL_PRIHIGH;
		break;
	case 2:	/* recoverable CRDs */
		priority = EL_PRILOW;
		break;
	}
	elrp = ealloc(sizeof(struct el_esr650), priority);
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_ESR650,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		elrp->el_body.elesr650.esr_cacr = -1;
		elrp->el_body.elesr650.esr_dser = -1;
		elrp->el_body.elesr650.esr_qbear = -1;
		elrp->el_body.elesr650.esr_dear = -1;
		elrp->el_body.elesr650.esr_cbtcr = -1;
		elrp->el_body.elesr650.esr_ipcr0 = -1;
		elrp->el_body.elesr650.esr_cadr = mfpr(CADR);
		elrp->el_body.elesr650.esr_mser = mfpr(MSER);

		/* if an FTAM is present */
		if((mbp->mb_modtype & FMOD_QBUS) == FMOD_QBUS) {
			if ((mbp->mb_vaddr->f_cpuid & 0x3) == FCPU_PROCB) {  /* FTAM */
				elrp->el_body.elesr650.esr_dser = ffqregs->cq_dser;
				elrp->el_body.elesr650.esr_qbear = ffqregs->cq_mear;
				elrp->el_body.elesr650.esr_dear = ffqregs->cq_sear;
			}
			if ((mbp->mb_vaddr->f_cpuid & 0x3) == FCPU_PROCA) {  /* FQAM */
				/* Log FQAM data here */
				;
			}
		}
		EVALID(elrp);
	}
}

/*
 * Log Memory CSRs (packet 4 of KA650 error spec)
 *
 * Note: side-effect.
 *	logmempkt sets the global var ka650_module to a coded number,
 *	thus logmempkt must be called before consprint() for pkt 4.
 */

ka60logmempkt(recover)
	int recover;		/* for pkt priority */
{
	register struct mb_node *mbp;
	register struct fmdc_regs *fmp;
	struct el_rec *elrp;
	struct el_mem *mrp;
	int merrtype = EL_UNDEF;

	for (mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
	    if ((mbp->mb_modtype & FMOD_CLASS) == FMOD_FMDC) {
		fmp = (struct fmdc_regs *)mbp->mb_vaddr;
		if (fmp->fm_fmdcsr & FMDC_ERROR) {
		    elrp = ealloc(EL_MEMSIZE,recover ? EL_PRIHIGH : EL_PRISEVERE);
		    if (elrp != NULL) {
			LSUBID(elrp,ELCT_MEM, EL_UNDEF,ELMCNTR_60,EL_UNDEF, EL_UNDEF,EL_UNDEF);
			if ((fmp->fm_eccsynd0 & FMDC_MBE) ||
			    (fmp->fm_eccsynd1 & FMDC_MBE)) {
			    merrtype = FF_MEM_RDS;
			}
			else if ((fmp->fm_eccsynd0 & FMDC_SBE) ||
			    (fmp->fm_eccsynd1 & FMDC_SBE)) {
			    merrtype = FF_MEM_CRD;
			}
			mrp = &elrp->el_body.elmem;
			mrp->elmem_cnt = 1;
			mrp->elmemerr.cntl = ((mbp->mb_flags & FMDC_CNTLR) >> 28);
			mrp->elmemerr.type = merrtype;
			mrp->elmemerr.numerr = 1;
			mrp->elmemerr.regs[0] = fmp->fm_eccaddr0;
			mrp->elmemerr.regs[1] = fmp->fm_eccaddr1;
			mrp->elmemerr.regs[2] = fmp->fm_eccsynd0;
			mrp->elmemerr.regs[3] = fmp->fm_eccsynd1;
			EVALID(elrp);

			ka60_module = MBUS_SLOT(mbp->mb_physaddr);
		    }
		}
	    }
	    else {
	    /* should have a case for FMCM, but is obsolete */
		;
	    }
	}
}

/*
 * The toy clock register on the SSC chip is in TODR
 * format, but cannot be accessed as a IPR in Firefox.
 * It is in I/O space.
 */
ka60readtodr()
{
	register struct ssc_regs *ssc = (struct ssc_regs *)cvqssc;
	return(ssc->ssc_toy);
}

/*
 * The toy clock register on the SSC chip is in TODR
 * format, but cannot be accessed as a IPR in Firefox.
 * It is in I/O space.
 */
ka60writetodr(yrtime)
u_int yrtime;
{
	register struct ssc_regs *ssc = (struct ssc_regs *)cvqssc;
	ssc->ssc_toy = TODRZERO + (100 * yrtime);
}

/*
 * Clear the FBCSR_FRZN bit and any other error bits in the FBIC and
 * enable FBIC error logging.
 */
int
enafbiclog()
{
	register struct mb_node *mbp;
	register struct fbic_regs *fbaddr;
	
	for(mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
		fbaddr = mbp->mb_vaddr;
		/*
		 * Firestarter memory does not have busaddr or busctl 
		 * registers.
		 */
		if ((mbp->mb_modtype & (FMOD_CLASS | FMOD_INTERFACE)) !=
		    (FMOD_MEM | FMOD_FIRESTARTER)) {
			fbaddr->f_busaddr = 0;
			fbaddr->f_busctl = 0;
		}
		fbaddr->f_buscsr = FBCSR_CLEAR;
	}
}


/*
 * Dump the interesting (frozen) FBIC registers to
 * the console and error logger.
 */
int
ka60fbicdump()
{
	register struct mb_node *mbp;
	register struct fbic_regs *fbaddr;
	register struct fmdc_regs *fmaddr;
	int frozen = 0;
	
	for(mbp=mbus_nodes; ((mbp->mb_flags&FBIC_MAPPED)&&(frozen==0)); mbp++) {
		fbaddr = mbp->mb_vaddr;
		/*
		 * NOTE: f_buscsr register bits are active low.
		 */
		if(!(~fbaddr->f_buscsr & FBCSR_FRZN))
			continue;
		else
			frozen++;
	}
	if(frozen) {
	    printf("FBIC register dump\n");
	    for(mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
			fbaddr = mbp->mb_vaddr;
			/*
			 * NOTE: f_buscsr register bits are active low.
			 */
			if(!(~fbaddr->f_buscsr & FBCSR_FRZN))
				continue;
			printf("\tmodtype = 0x%x\n", fbaddr->f_modtype);
			printf("\tbuscsr   = 0x%x\n", fbaddr->f_buscsr);
		if ((mbp->mb_modtype & (FMOD_CLASS | FMOD_INTERFACE)) == 
			(FMOD_MEM | FMOD_FIRESTARTER)) {
			printf("\tfmdcsr   = 0x%x\n", fbaddr->f_busctl);
			printf("\tbaseaddr = 0x%x\n", fbaddr->f_busaddr);
		}
		else if ((mbp->mb_modtype & FMOD_CLASS) == FMOD_MEM) {
			fmaddr = (struct fmdc_regs *)fbaddr;
			printf("\tbusctl   = 0x%x\n", fmaddr->fm_busctl);
			printf("\tbusadr   = 0x%x\n", fmaddr->fm_busaddr);
			printf("\tbusdat   = 0x%x\n", fmaddr->fm_busdat);
			printf("\tfmdcsr   = 0x%x\n", fmaddr->fm_fmdcsr);
			printf("\tbaseaddr = 0x%x\n", fmaddr->fm_baseaddr);
			printf("\teccaddr0 = 0x%x\n", fmaddr->fm_eccaddr0);
			printf("\teccaddr1 = 0x%x\n", fmaddr->fm_eccaddr1);
			printf("\teccsynd0 = 0x%x\n", fmaddr->fm_eccsynd0);
			printf("\teccsynd1 = 0x%x\n", fmaddr->fm_eccsynd1);
		}
		else {
			printf("\tbusctl   = 0x%x\n", fbaddr->f_busctl);
			printf("\tbusadr   = 0x%x\n", fbaddr->f_busaddr);
			printf("\tbusdat   = 0x%x\n", fbaddr->f_busdat);
			printf("\tfbicsr   = 0x%x\n", fbaddr->f_fbicsr);
			printf("\trange    = 0x%x\n", fbaddr->f_range);
			printf("\tipdvint  = 0x%x\n", fbaddr->f_ipdvint);
			printf("\tcpuid    = 0x%x\n", fbaddr->f_cpuid);
			printf("\tiadr1    = 0x%x\n", fbaddr->f_iadr1);
			printf("\tiadr2    = 0x%x\n", fbaddr->f_iadr2);
		}
	    }
	}
}

/*
 * Name:	ka60getmbnode
 *
 * Args:	addr	- The physical address of the I/O space page
 *			containing the FBIC registers
 *
 * 		modtype	- The contents of the FBIC Modtype register
 *
 * Returns:	A pointer to the first unused mbus_nodes structure or
 *		if the FBIC was already mapped, a pointer to the mbus_nodes
 *		structure that was initialized at the time the FBIC regs
 *		were mapped.
 */
int
ka60getmbnode(addr, modtype)
u_long	addr;
u_long	modtype;
{
	register struct mb_node *mbp;

	for(mbp = mbus_nodes; mbp->mb_flags & FBIC_MAPPED; mbp++) {
		if((mbp->mb_modtype == modtype) && (mbp->mb_physaddr == addr)) {
			if(mbp->mb_modtype != FMOD_CPU)
				break;
			else if(mbp->mb_flags & FF_WHICH_PROC(addr))
				break;
		}
	}
	return((int)mbp);
}

ka60_mbfillin(mptr, fptr, addr, memctlr)
struct mb_node *mptr;
struct fbic_regs *fptr;
char *addr;
int memctlr;
{
	mptr->mb_modtype = fptr->f_modtype;
	mptr->mb_physaddr = (u_long)addr;
	mptr->mb_vaddr = fptr;
	mptr->mb_flags = FBIC_MAPPED | FBIC_ALIVE | (memctlr << 28);
	mptr->mb_slot = MBUS_SLOT(addr);
}

ka60_initcpufbic(fptr)
struct fbic_regs *fptr;
{

	register int (**addr)();    /* double indirection neccessary to keep
					the C compiler happy */
	int base = (int)scb.scb_ipl14;
	int cpuid;
	int scbaddr;

	cpuid = fptr->f_cpuid;
	fptr->f_busaddr = 0;
	fptr->f_busctl = 0;
	fptr->f_buscsr = FBCSR_CLEAR;
	fptr->f_fbicsr |= FFCSR_EXCAEN | FFCSR_LEDS_OFF |
		 FFCSR_PRI0EN | FFCSR_HALTEN | FFCSR_NORMAL;

	/* calculate the IP interrupt vector */

	scbaddr = (base + ((cpuid >> 2) * 0x20) + 0x08);
	fptr->f_ipdvint = FIPD_IPUNIT | (scbaddr & 0x1ff);
	addr = (int (**)())scbaddr;
	/*
	 * Fill in the address of the f_ipdvint register to be
	 * used for generating IP interrupts.
	 */
	ka60_ip[(cpuid >> 1) & 0xf] = (u_long *)&fptr->f_ipdvint;
	/*
	 * Plug the IP interrupt vector into the SCB
	 */
	*addr = scbentry(&Xipintr,SCB_ISTACK);
}

/*
 * Initialization code that the slave processor must run
 *  before starting up
 */

ka60initslave()
{

	ka60cachenbl();
	ka60setcache(cache_state);

}

ka60setscbvec(fptr, ipl, address, stack)
struct fbic_regs *fptr;
int ipl;	 /* in hex */
int	address;
int	stack;
{
	register int (**addr)();    /* double indirection neccessary to keep
					the C compiler happy */
	int base = (int)scb.scb_ipl14;
	int cpuid;
	int scbaddr;

	cpuid = fptr->f_cpuid;
	scbaddr = (base + ((cpuid >> 2) * 0x20) + ((ipl - 0x14) * 4));
	addr = (int (**)())scbaddr;
	*addr = scbentry(address, stack);
}

/*
 * Log FBIC registers
 */

ka60logfbicpkt(priority)
	int priority;		/* for pkt priority */
{
	register struct mb_node *mbp;
	register struct fbic_regs *fbp;
	struct el_fmdc *fmdcp;
	struct el_fbic *fbicp;
	struct el_fmcm *fmcmp;
	struct fmdc_regs *fmp;
	struct el_rec *elrp;
	int i, j, end;
	u_long *elrp_tmp;

	switch (priority) {
	case 0: /* non-recoverable mchecks & memory errs */
		priority = EL_PRISEVERE;
		break;
	case 1: /* recoverable mchecks */
		priority = EL_PRIHIGH;
		break;
	case 2: /* recoverable CRDs */
		priority = EL_PRILOW;
		break;
	}
	elrp = ealloc(sizeof(struct el_mbus), priority);
	if (elrp != NULL) {
		/*
		 * zero the allocated buffer as we won't write all entries
		 */
		for (i = 0; i < 14; i++) {
			for (j = 0; j < 12; j++)
				elrp->el_body.elmbus.elmb_module_log[i][j] = 0;
		}

		LSUBID(elrp,ELCT_MBUS,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);

		/*
		 * Log all of the FBICs, FMDCs, and FMCMs in the
		 * system whether or not they are frozen
		 */
		for (mbp = mbus_nodes, i = 0; mbp->mb_flags & FBIC_MAPPED; mbp++,i++) {
		    fbp = mbp->mb_vaddr;
		    switch(mbp->mb_modtype & FMOD_INTERFACE) {
			case FMOD_FBIC:
			    fbicp = (struct el_fbic *)&elrp->el_body.elmbus.elmb_module_log[i][0];
			    fbicp->fbic_size = 0x2c;
			    fbicp->fbic_cpuid = (u_char)(fbp->f_cpuid & 0x1f);
			    fbicp->fbic_modtype = fbp->f_modtype;
			    fbicp->fbic_buscsr = fbp->f_buscsr;
			    fbicp->fbic_busctl = fbp->f_busctl;
			    fbicp->fbic_busaddr = fbp->f_busaddr;
			    fbicp->fbic_busdat = fbp->f_busdat;
			    fbicp->fbic_fbicsr = fbp->f_fbicsr;
			    fbicp->fbic_range = fbp->f_range;
			    fbicp->fbic_ipdvint = fbp->f_ipdvint;
			    fbicp->fbic_iadr1 = fbp->f_iadr1;
			    fbicp->fbic_iadr2 = fbp->f_iadr2;
			    if (fbicp->fbic_buscsr & FBCSR_FRZN)
				fbicp->fbic_valid = FBIC_VALID | FBIC_ANALYZE;
			    else
				fbicp->fbic_valid = FBIC_VALID;
			    break;
	
			case FMOD_FMDC:
			    fmdcp = (struct el_fmdc *)&elrp->el_body.elmbus.elmb_module_log[i][0];
			    fmp = (struct fmdc_regs *)fbp;
			    fmdcp->fmdc_size = 0x30;
			    fmdcp->fmdc_cpuid = (u_char)(MBUS_SLOT(mbp->mb_physaddr) << 2);
			    fmdcp->fmdc_modtype = fmp->fm_modtype;
			    fmdcp->fmdc_buscsr = fmp->fm_buscsr;
			    fmdcp->fmdc_busctl = fmp->fm_busctl;
			    fmdcp->fmdc_busaddr = fmp->fm_busaddr;
			    fmdcp->fmdc_busctl = fmp->fm_busctl;
			    fmdcp->fmdc_fmdcsr = fmp->fm_fmdcsr;
			    fmdcp->fmdc_baseaddr = fmp->fm_baseaddr;
			    fmdcp->fmdc_eccaddr0 = fmp->fm_eccaddr0;
			    fmdcp->fmdc_eccaddr1 = fmp->fm_eccaddr1;
			    fmdcp->fmdc_eccsynd0 = fmp->fm_eccsynd0;
			    fmdcp->fmdc_eccsynd1 = fmp->fm_eccsynd1;
			    if (fmdcp->fmdc_buscsr & FBCSR_FRZN)
				fmdcp->fmdc_valid = FBIC_VALID | FBIC_ANALYZE;
			    else
				fmdcp->fmdc_valid = FBIC_VALID;
			    break;
	
			case FMOD_FMCM:
			    fmcmp = (struct el_fmcm *)&elrp->el_body.elmbus.elmb_module_log[i][0];
			    fmp = (struct fmdc_regs *)fbp;
			    fmcmp->fmcm_size = 0x14;
			    fmcmp->fmcm_cpuid = (u_char)(MBUS_SLOT(mbp->mb_physaddr) << 2);
			    fmcmp->fmcm_modtype = fmp->fm_modtype;
			    fmcmp->fmcm_buscsr = fmp->fm_buscsr;
			    fmcmp->fmcm_busctl = fmp->fm_busctl;
			    fmcmp->fmcm_baseaddr = fmp->fm_busaddr;
			    /*
			     * Error entry is valid, but there aren't enough
			     * error bits in Firestarter memory to include
			     * it in the error analysis
			     */
			    fmcmp->fmcm_valid = FBIC_VALID;
			    break;
	
			default:
				break;
	
		    }
		    /*
		     * elmb_size is the number of module_logs plus the size
		     * of each module log plus the three long words at the
		     * top of the el_mbus structure.
		     */
		    elrp->el_body.elmbus.elmb_size = (i * 14) + 12;
		    elrp->el_body.elmbus.elmb_count = i + 1;
		    /*
		     * If time permits to do some error analysis
		     * it can be put here.  For now set these
		     * error summary fields to -1;
		     */
		    elrp->el_body.elmbus.elmb_dominant = -1;
		    elrp->el_body.elmbus.elmb_flags = -1;
		    elrp->el_body.elmbus.elmb_mod_err = -1;
		    EVALID(elrp);
		}
	}
}
/*
 * Disable IPL 17 interrupts from the MBUS
 */
ka60clrmbint()
{
	register struct fbic_regs *fbaddr;

	fbaddr = ((struct mb_node *)ka60getmbnode(cpu_fbic_addr,
		0x01010108))->mb_vaddr;
	fbaddr->f_fbicsr &= ~FFCSR_IRQE_3;
}

/*
 * Enable IPL 17 interrupts from the MBUS
 */
ka60setmbint()
{
	register struct fbic_regs *fbaddr;

	fbaddr = ((struct mb_node *)ka60getmbnode(cpu_fbic_addr,
		0x01010108))->mb_vaddr;
	enafbiclog();
	fbaddr->f_fbicsr |= FFCSR_IRQE_3;
}
