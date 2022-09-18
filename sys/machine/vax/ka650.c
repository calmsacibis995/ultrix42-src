#ifndef lint
static char *sccsid = "@(#)ka650.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 - 1989 by			*
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
 * Modification History:	(ka655/ka650/)ka640.c
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
 * 12-May-89 -- Todd M. Katz		TMK0001
 *	Changes to MSI port driver configuration by ka650conf():
 *	1. Modified interface to msi port driver probe routine.
 *	2. Always mark msi adapters alive.
 *	3. Print out msi adapter configuration information.
 *	4. Always explicitly initialize the appropriate SCB vector.
 *
 * 15-Feb-89	afd (Al Delorey)
 *	Re-synch with VAX 3.0 for 3.2 pool merge.
 *
 * 31-Jan-89	map (Mark Parenti)
 *	Change include syntax for merged pool.
 *
 *  6-Aug-88 -- robin
 *	Changed the test around what processor accessed cache.  This will
 *	allow the ka655 and the ka650 to access it but not the ka640.
 *
 * 16-Aug-88 -- robin
 *	Changed the way panic strings were handled so that the
 *	ka650, ka655, ka640 will all report the correct cpu but
 *	still share the same code.
 *	I also removed the code that was ifdef'ed to not be used
 *	because it was moved to cvax.c by Darrell.
 *
 * 17-Jun-88 -- afd
 *	Removed the static declaration of the virtual names that
 *	correspond to IO space maps.  These declarations are unnecessary
 *	(the virtual names are in spt.s) and were wasting physical
 *	memory space and PTEs.
 *
 * 15-Apr-88	Robin
 *	Added routines to find devices or controllers on the IBUS.
 *	this code should be moved to a new file after Darrell gets
 *	his new code in the pool.
 *
 * 12-Nov-88 -- fred (Fred Canter)
 *	Moved cache2_state to machdep and made it extern in this file.
 *	This prevents clash with same variable in ka420.c.
 *
 * 05-Oct-87 -- afd
 *	Do not allow recursive machine check to occur.  Use a flag
 *	    and if machine check is call recursively, "restart".
 *
 * 24-Nov-87 -- robin
 *	Added support for KA640:
 *		ka650conf(): check sysdep field for ka640 or ka650
 *		Don't access 2nd level cache if ka640
 *
 * 10-Sep-87 -- afd
 *	Machine check/error recovery modifications:
 *	Eliminate use of cache_errcnt.cache_state it was redundant to
 *	    global "cache_state" word (first level cache state).
 *	First and Second Level cache errors are given longer time
 *	    thresh-holds for 3 errors to occur.
 *	When 1st level cache is selectively disabled, reset timers.
 *	Recover from 2nd level cache error as memerr interrupt (like mcheck 82).
 *
 * 24-Aug-87 -- afd
 *	KA650crderr must explicitely return 0, else it looks like no
 *	"memerr" routine is configured.
 *
 * 27-Jul-87 -- afd
 *	Default caches back to 'on' now that pass 3 chips are available.
 *	Print CVAX chip microcode rev level and
 *	KA650 processor firmware rev level in ka650conf routine.
 *
 * 13-July-87 -- afd
 *	If 1st level cache is disabled (both set 1 and set 2) in machine
 *	check handler, set CADR bits for I and D stream so 2nd level cache
 *	can opperate.
 *
 *	The default case (no primary flag) of machine check 82/83 was doing
 *	a cprintf of the mem module as if it were an uncorrectable ECC error.
 *
 *	In ka650conf report the processor type as "KA650 processor" not
 *	"MicroVAX 3600" since the configurations are indistinguishable.
 *
 *	For machine checks 80/81 and 82/83 if no primary error flags match
 *	put out an ASCII message saying that no primary error flag was found.
 *
 *	Machine check modifications:
 *	    - made variables "recover" and "time" register vars for speed.
 *	    - try to recover from mcheck types 1 thru 4 (CFPA errors).
 *	    - try to recover from mcheck 82/83 primary flags: MEM_CDAL, MEM_RDS
 *	    - for pass 3 CVAX CPU bug:
 *		mcheck 80 primary flag DSER_NOGRANT was moved up in chain
 *			of events to test and it is non-recoverable.
 *		primary flag MEM_CDAL had to be added to mcheck 80 since
 *			mcheck 82 with that flag can be reported as an 80.
 *
 * 01-June-87 -- afd
 *	Remove error case of CBTCR bits <31:30> from machine check and 
 *	    memerr routines, and remove timer for it.
 *	Fix access to CACR register so we don't index into it.
 *
 * 20-May-87 -- afd
 *	Default caches to OFF so installation succeeds!
 *	Fix memcon bits <27:24>
 *
 * 19-May-87 -- afd
 *	Use long word accesses (not bytes) and pointer (not index) to
 *		flush the 2nd level cache (else it takes too long).
 *	Check both bit 31 and bit 30 in CBTCR for bus timeout error.
 *	Disable ECC on uncorrectable ECC errors, else core dump fails.
 *	Turn off graphics console loopback of kernel printf's
 *		when the system panics.
 *
 * 12-May-87 -- afd
 *	Changes for V1.5 of the error spec
 *	    mear => qbear;   sear => dear
 *	    New order of errors tested.
 *	Fixed a bug in ka650_machcheck where retry was always set to false.
 *	Fixed recording of cache states in ka650setcache().
 *
 * 20-Apr-87 -- afd
 *	Make "long memcon" "unsigned long ka650_memcon"
 *	For Mayfair error spec V1.4:
 *	    Changed memcon to new format (as given in V1.4 of ka650 error spec)
 *	    Map IPCRs so we can get IPCR0 for pkt 3, & print it in consprint()
 *	    Log IPCR0 in pkt 3 (ka650 ESR pkt)
 *	    Improved panic strings and printf strings
 *	    Changed a couple of machine check strings
 *	    Routine "logmempkt" now sets "ka650_module" to the module in error
 *	Code "Clean-ups":
 *	    Moved declaration of space for Mayfair local registers &
 *		declaration of array with CVAX machine check strings,
 *		to here from ka650.h since the ka650.h header file is now
 *		included by cpuconf.c and machdep.c as well as ka650.c
 *	    Took out debug printfs
 *	    Removed some unused variables in ka650conf
 *	    Changed routine names of consprint, logserpkt, logmempkt, by adding
 *		the prefix "ka650" to the routine names.
 *	Bug Fixes:
 *	    Convert mcheck types 80-83 in consprint, so # prints correctly
 *	    Clear ka650_memcon <27:24> in consprint() & logmempkt() before
 *	          checking for which bank had an error (in case its
 *	          already set to a bank from a prior error)
 *	    Print memcsr that matched in consprint() of pkt 4
 *	Changed name CVAXQ to VAX3600 for Mayfair.
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
#include "../machine/cvax.h"
#include "../machine/ka650.h"
#include "../h/types.h"
#include "../h/config.h"

/* save record of sbis present for sbi error logging for 780 and 8600 */
extern long sbi_there;	/* bits 0-15 for nexi,sbi0; 16-31 for nexi on sbi1*/
extern int ws_display_type;	/* type of console on workstations */
extern struct cpusw *cpup;	/* pointer to cpusw entry */

extern int cache2_state;		/* state of 2nd level cache: 0=off, 1=on */
unsigned long ka650_memcon;	/* memory config (memcsr's 0-15) */
unsigned int ka650_module;	/* which module had a hard memory error */
int ka650_mchkprog = 0;		/* machine check in progress */

/*
 * ka650 configuration routine.
 * Maps local register space, clears map registers set by VMB,
 *   calls unifind for device configuration, set console program restart flag.
 */

char Ka650_processor[] = "KA650 processor ";
char Ka650_mchk[] = "KA650 mchk";
char Ka650_memerr[] = "KA650 memerr";
char Ka655_processor[] = "KA655 processor ";
char Ka655_mchk[] = "KA655 mchk";
char Ka655_memerr[] = "KA655 memerr";
char Ka640_processor[] = "KA640 processor ";
char Ka640_mchk[] = "KA640 mchk";
char Ka640_memerr[] = "KA640 memerr";

char *processor_string;
char *memerr_string;
char *mchk_string;

ka650conf()
{
	register int i;
	register u_long *mp;		/* ptr to memcsr 0-15 */
	register unsigned int *mapaddr;	/* phys address of Qbus map registers */
	char *iov;			/* virtual address in I/O space */
	char *iop;			/* physical address in I/O space */
	extern int fl_ok;
	int ret;

	/*
	 * We now have the scb set up enough so we can handle
	 * interrupts if any are pending.
	 */
	(void) spl0();
	/*
	 * Map the Local Register space (nexus space on other VAXen).
	 */
	nxaccess ((char *) CVQMERRADDR, CVQMERRmap, CVQMERRSIZE);
	nxaccess ((char *) CVQCBADDR, CVQCBmap, CVQCBSIZE);
	nxaccess ((char *) CVQBMADDR, CVQBMmap, CVQBMSIZE);
	nxaccess ((char *) CVQSSCADDR, CVQSSCmap, CVQSSCSIZE);
	nxaccess ((char *) CVQCACHEADDR, CVQCACHEmap, CVQCACHESIZE);
	nxaccess ((char *) CVQIPCRADDR, CVQIPCRmap, CVQIPCRSIZE);
	nxaccess ((char *) CVQROMADDR, CVQROMmap, CVQROMSIZE);
	/*
	 * In order to map the NI area as one structure the following
	 * calls to nxaccess are relative to one area in the PTE table.
	 * This allows the NI driver probe to work for both Mayfair-II and
	 * Firefox systems.
	 */
	nxaccess ((char *) CVQNIADDR, CVQNImap, CVQNISIZE);
	nxaccess ((char *) CVQNIDPADDR, (int) CVQNImap + sizeof (struct pte), CVQNIDPSIZE);
	nxaccess ((char *) CVQNILRBADDR, (int) CVQNImap + (sizeof (struct pte) * 2), CVQNILRBSIZE);
	/*
	 * End of NI area for Mayfair-II and Firefox
	 */
	nxaccess ((char *) CVQMSIADDR, CVQMSImap, CVQMSISIZE);
	nxaccess ((char *) CVQMSIRBADDR, CVQMSIRBmap, CVQMSIRBSIZE);

	/*
	 * Print CVAX chip microcode rev level and
	 * KA650/KA640 processor firmware rev level
	 */
	if (cvqrom->cvq7_sysdep == SB_KA640){
		processor_string = Ka640_processor;
		memerr_string = Ka640_memerr;
		mchk_string = Ka640_mchk;
	}
	else 
	if (cvqrom->cvq7_sysdep == SB_KA655){
		processor_string = Ka655_processor;
		memerr_string = Ka655_memerr;
		mchk_string = Ka655_mchk;
	}
	else {
		processor_string = Ka650_processor;
		memerr_string = Ka650_memerr;
		mchk_string = Ka650_mchk;
	     }
	printf("%s",processor_string);
	if (fl_ok)
		printf("with an FPU\n");
	else
		printf("without an FPU\n");
	printf("	CPU microcode rev = %d, processor firmware rev = %d\n",
		(mfpr(SID)) & 0xFF, cvqrom->cvq7_firmrev);
 

	/* The CVAX CPU chip rev 3 or lower has a POLY-PASSIVE RELEASE problem,
	 * If this rev is seen let the owner know about it.
	 */
	if (cvqrom->cvq7_sysdep == SB_KA650) {
		if((((mfpr(SID)) & 0xFF) < POLY_FIX_REV))
			printf("<<< WARNING -- This KA650 processor is out of Rev\nContact your Digital Equipment Representative for a replacement. >>>\n");
	}


	/*
	 * Clear the map registers that were set by VMB (8192 of them).
	 * This is necessary so that the QDSS sizing will work when
	 *   there is more than one QDSS.
	 */
	mapaddr = (unsigned int *)cvqbm->cvqbm_uba.cqba.qb_map;
	for (i = 0; i < 8192; i++)
		{
		*mapaddr = i;
		mapaddr++;
		}
	/*
	 * See if there is anything there (cvqmerr starts local reg I/O space).
	 */
	if ((*cpup->badaddr)((caddr_t) cvqmerr, 4))
		return(-1);
	sbi_there |= 1<<0;
	if (cvqrom->cvq7_sysdep == SB_KA640){
		extern	int	nummsi, nNMSI;
		config_set_alive("ibus", 0, 0, 0);
		ib_config_dev(cvqni,CVQNIADDR,0,"ln",0);
		(void)printf("msi%d at address 0x%x (SII)\n", 0, cvqmsi);
		if (nummsi > 0 )
			(void)printf("msi%d unsupported\n", nummsi);
		else if (nummsi >= nNMSI )
			(void)printf("msi%d not configured\n", nummsi);
		else {
			extern int	(*msiintv[])(), msi_probe();
			register int	( *stray )();

			stray = scb.scb_stray5;
			scb.scb_stray5 = scbentry(msiintv[nummsi], SCB_ISTACK);
			if( msi_probe(nummsi, cvqmsi, cvqmsirb )) {
				(void)config_set_alive("msi", 0, 0, 0);
			} else {
				scb.scb_stray5 = stray;
			}
		}
		nummsi++;
	}
	printf("Q22 bus\n");
	uba_hd[0].uba_type = UBAUVII;
	iov = (char *)cvqbm;
	iop = (char *)CVQBMADDR;
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
	cvqmerr->cvq1_dser |= DSER_CLEAR;
	cvqssc->ssc_cbtcr |= (CBTCR_BTO|CBTCR_RWT);

	/*
	 * Record memory configuration (memcsr's 0-15) in "ka650_memcon".
	 */
	mp = &(cvqmerr->cvq1_memcsr0);
	for (i = 31; i > 15; i--, mp++) {
		ka650_memcon |= ((*mp & MEM_BNKENBLE) >> i);
	}
	ka650_memcon |= ((cvqmerr->cvq1_memcsr0 & MEM_BNKUSAGE) << 16);
	ka650_memcon |= ((cvqmerr->cvq1_memcsr4 & MEM_BNKUSAGE) << 18);
	ka650_memcon |= ((cvqmerr->cvq1_memcsr8 & MEM_BNKUSAGE) << 20);
	ka650_memcon |= ((cvqmerr->cvq1_memcsr12 & MEM_BNKUSAGE) << 22);
	/*
	 * Tell the console program that we've booted and
	 * that we speak English and would like to restart
	 * if the machine panics.
	 */
	cvqssc->ssc_cpmbx=RB_CV_RESTART;
	return(0);
}

/*
 * This routine sets the cache to the state passed:  enabled/disabled.
 * The CVAX chip has a first level cache enabled through IPR CADR.
 * The KA650 processor has a second level cache enabled through
 *     local register CACR.
 * Also set state flags in the error count structures.
 */

ka650setcache(state)
int state;
{
	register int *cacheptr;		/* ptr to flush 2nd level cache */
	register int *cacheend; 	/* ptr to end of 2nd level cache */

	mtpr (CADR, state);
	if (state != 0 && (cvqrom->cvq7_sysdep != SB_KA640)) {
		/*
		 * Flush 2nd level cache before enabling
		 */
		cacheptr = cvqcache->cvq5_cache;
		cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
		for (; cacheptr < cacheend; ) {
			*cacheptr++ = 0;
		}
		cvqcb->cvq2_cacr1 |= CACR_CEN;
		cache2_state = 1;
	}
	return(0);
}

/*
 * Enable cache.  Both D_stream & I-stream, both Set-1 and Set-2.
 * These bits are in the IPR CADR for the CVAX chip.
 */

extern	int	cache_state;

ka650cachenbl()
{
	cache_state = (CVAX_CEND | CVAX_CENI | CVAX_SEN1 | CVAX_SEN2);
	return(0);
}

/*
 * Enable CRD interrupts.
 * This runs at regular (15 min) intervals, turning on the interrupt.
 * It is called by the timeout call in memenable in machdep.c
 * The interrupt is turned off, in ka650crderr(), when 3 error interrupts
 *   occur in 1 time period.  Thus we report at most once per memintvl (15 mins).
 */
ka650memenable()
{
	cvqmerr->cvq1_memcsr17 |= MEM_CRDINT;
	return(0);
}

ka650tocons(c)
	register int c;
{
	while ((mfpr (TXCS) & TXCS_RDY) == 0)
		continue;
	mtpr (TXDB, c);
	return(0);
}


char *
ka650umaddr(ioadpt,nexnum)
	int ioadpt,nexnum;
{
	return(QMEMCVQ);
}

u_short *
ka650udevaddr(ioadpt,nexnum)
	int ioadpt,nexnum;
{
	return(QDEVADDRCVQ);
}

/*
 * Machine check handler.
 * Called from locore thru the cpu switch in response to a trap at SCB 4
 * We recover from any that we can if hardware "retry" is possible.
 */

ka650machcheck (cmcf)
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
	 * Disable 2nd level cache to protect execution of machine check code.
	 * NOTE: Do NOT put any executable code in this routine before
	 *	 disabling the cache, the cache may be what caused the mcheck.
	 */
	if (cvqrom->cvq7_sysdep != SB_KA640)
		cvqcb->cvq2_cacr1 &= ~CACR_CEN;
	/*
	 * Do not allow recursive machine check.
	 * Halt the processor, then a restart should get a dump.
	 */
	if (ka650_mchkprog == 0)
		ka650_mchkprog = 1;
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
	time = mfpr(TODR);
	if (((mcf->mc1_psl & PSL_FPD) != 0) ||
	    (((mcf->mc1_psl & PSL_FPD) == 0) && ((mcf->mc1_internal_state2 & IS2_VCR) == 0)))
		retry = 1;

	switch (type) {
	case 1:
	case 2:
	case 3:
	case 4:
		/*
		 * CFPA errors
		 * Re-enable 2nd level cache to log in correct state.
		 * If fewer than 3 errors in 1 time period, try to recover
		 * else we will crash.
		 */
		if (cache2_state) {
			cacheptr = cvqcache->cvq5_cache;
			cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
			for (; cacheptr < cacheend; ) {
				*cacheptr++ = 0;
			}
			cvqcb->cvq2_cacr1 |= CACR_CEN;
		}
		if (time - cfpa_errcnt.cfpa_prev > TIME_THRESH) {
			if (retry)
				recover = 1;
		}
		logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
		if (recover) {
			cfpa_errcnt.cfpa_prev = cfpa_errcnt.cfpa_last;
			cfpa_errcnt.cfpa_last = time;
		} else {
			ka650consprint(2,type,mcf);
		}
		break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		/*
		 * These are non-recoverable:
		 * Re-enable 2nd level cache to log in correct state.
		 * Log the mcheck & print mcheck frame to console.
		 */
		if (cache2_state) {
			cacheptr = cvqcache->cvq5_cache;
			cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
			for (; cacheptr < cacheend; ) {
				*cacheptr++ = 0;
			}
			cvqcb->cvq2_cacr1 |= CACR_CEN;
		}
		logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
		ka650consprint(2,type,mcf);
		break;
	case 0x80:
	case 0x81:
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
			cacheptr = cvqcache->cvq5_cache;
			cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
			for (; cacheptr < cacheend; ) {
				*cacheptr++ = 0;
			}
			cvqcb->cvq2_cacr1 |= CACR_CEN;
		}
		if ((mfpr(MSER) & MSER_MCD) != 0) {
			/*
			 * CDAL Bus Parity Err/2nd Level Cache Data Parity Err.
			 * See if 3 within 1 time period.
			 */
			if (time - cdal_errcnt.cdal_prev <= TIME_THRESH_C2) {
				/*
				 * Got 3 errors within 1 time period.
				 * Action depends on prior state of 2nd level cache:
				 *    was on:  Try to Recover
				 *    was off: No Recover (always for ka640)
				 */
				if (cache2_state) {
					/*
					 * Cache was on, 3 errs in 1 time period
					 */
					if (retry)
						recover = 1;
					else {
						/*
						 * If console is a graphics device,
						 * force printf messages directly to screen.
						 */
						if (ws_display_type) {
						    ws_disp = ws_display_type << 8;
						    (*cdevsw[ws_display_type].d_ioctl)(ws_disp, QD_KERN_UNLOOP, 0, 0);
						}
					}
					printf("2nd Level Cache DISABLED by software on mchk\n");
					cache2_state = 0;
					cdal_errcnt.cdal_last = 0;
					time = 0;
				}
					/*
					 * Note: 3 err/time period w/ cache disabled => panic
					 */
			} else {
				/*
				 * Got fewer than 3 errors within 1 time period.
				 * Action depends on prior state of 2nd level cache
				 *    was on:  Flush cache & try to recover
				 *    was off: try to recover
				 */
				if (cache2_state) {
					/*
					 * Cache was on, < 3 errs in 1 time period
					 * Flush & Re-enable the 2nd level cache
					 */
					cacheptr = cvqcache->cvq5_cache;
					cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
					for (; cacheptr < cacheend; ) {
						*cacheptr++ = 0;
					}
					cvqcb->cvq2_cacr1 |= CACR_CEN;
					mprintf("2nd level cache re-enabled by software on mcheck\n");
					if (retry)
						recover = 1;
				} else {
					/*
					 * Cache was off, < 3 errs in 1 time period
					 */
					if (retry)
						recover = 1;
				}
			}
			/*
			 * Log the machine check, and the error status regs.
			 * If we can recover update the times,
			 * else print errors on the console.
			 * Last, clear the error bits.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			if (recover) {
				cdal_errcnt.cdal_prev = cdal_errcnt.cdal_last;
				cdal_errcnt.cdal_last = time;
			} else {
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
			}
			mtpr(MSER,1);
		} else if ((mfpr(MSER) & MSER_MCC) != 0) {
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
				ka650consprint(2,type,mcf);
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
					ka650consprint(2,type,mcf);
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
					ka650consprint(2,type,mcf);
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
		} else if ((cvqmerr->cvq1_dser & DSER_NOGRANT) != 0) {
			/*
			 * Q-22 Bus No Grant timeout on cpu demand R/W.
			 * If fewer than 3 errs in 1 time period, try to recover
			 * else we will crash.
			 * If mcheck type is 80, no recover due to bug in
			 *    pass3 CVAX CPU chip which incorreclty reports
			 *    an mcheck 82 as 80.
			 */
			if (type == 0x80)
				recover = 0;
			else if (time - qngr_errcnt.qngr_prev > TIME_THRESH) {
				if (retry)
					recover = 1;
			}
			/*
			 * Log the machine check, and the error status regs.
			 * If we can recover update the times,
			 * else print errors on the console.
			 * Last, clear the error bits.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			if (recover) {
				qngr_errcnt.qngr_prev = qngr_errcnt.qngr_last;
				qngr_errcnt.qngr_last = time;
			} else {
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
			}
			cvqmerr->cvq1_dser |= DSER_CLEAR;
		} else if ((cvqmerr->cvq1_memcsr16 & MEM_CDAL) != 0) {
			/*
			 * CDAL Bus Parity Error
			 * Log the mcheck, ESR Packet, & Mem Packet.
			 *
			 * Note:
			 *	This is really a machine check 82, but may
			 * 	be incorrectly reported as 80 due to a bug
			 * 	in the pass 3 CVAX CPU chip.
			 */
			if (time - cdalW_errcnt.cdalW_prev > TIME_THRESH) {
				if (retry)
					recover = 1;
			}
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			ka650logmempkt(recover);
			if (recover) {
				cdalW_errcnt.cdalW_prev = cdalW_errcnt.cdalW_last;
				cdalW_errcnt.cdalW_last = time;
			} else {
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
				ka650consprint(4,0,0);
			}
			cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
		} else if ((cvqmerr->cvq1_memcsr16 & MEM_RDS) != 0) {
			/*
			 * Main memory uncorrectable ECC error:
			 * Disable Main Mem error detection & correction
			 *     to protect mcheck code & to get core dump.
			 * Log the mcheck, ESR Packet, & Mem Packet.
			 * We can't recover so print errors on the console.
			 * NOTE: logmempkt sets the global var ka650_module
			 *	 "ka650_module" is set to a coded number!
			 * NOTE: consprint() sets up display for cprintf here!
			 * Last, clear the error bits.
			 */
			cvqmerr->cvq1_memcsr17 |= MEM_ERRDIS;
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			ka650logmempkt(recover);
			ka650consprint(2,type,mcf);
			ka650consprint(3,0,0);
			cprintf("Uncorrectable ECC Error detected in Main Memory <%x>\n", ka650_module);
			ka650consprint(4,0,0);
			cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
			cvqmerr->cvq1_dser |= DSER_CLEAR;
		} else if ((cvqmerr->cvq1_dser & DSER_QNXM) != 0) {
			/*
			 * Q-22 Bus Non-existent Memory.
			 * If fewer than 3 errs in 1 time period, try to recover
			 * else we will crash.
			 */
			if (time - qnxm_errcnt.qnxm_prev > TIME_THRESH) {
				if (retry)
					recover = 1;
			}
			/*
			 * Log the machine check, and the error status regs.
			 * If we can recover update the times,
			 * else print errors on the console.
			 * Last, clear the error bits.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			if (recover) {
				qnxm_errcnt.qnxm_prev = qnxm_errcnt.qnxm_last;
				qnxm_errcnt.qnxm_last = time;
			} else {
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
			}
			cvqmerr->cvq1_dser |= DSER_CLEAR;
		} else if ((cvqmerr->cvq1_dser & DSER_QPE) != 0) {
			/*
			 * Q-22 Bus device parity error.
			 * If fewer than 3 errs in 1 time period, try to recover
			 * else we will crash.
			 */
			if (time - qpe_errcnt.qpe_prev > TIME_THRESH) {
				if (retry)
					recover = 1;
			}
			/*
			 * Log the machine check, and the error status regs.
			 * If we can recover update the times,
			 * else print errors on the console.
			 * Last, clear the error bits.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			if (recover) {
				qpe_errcnt.qpe_prev = qpe_errcnt.qpe_last;
				qpe_errcnt.qpe_last = time;
			} else {
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
			}
			cvqmerr->cvq1_dser |= DSER_CLEAR;
		} else if ((cvqmerr->cvq1_dser & DSER_DNXM) != 0) {
			/*
			 * DMA NXM
			 * If fewer than 3 errs in 1 time period, try to recover
			 * else we will crash.
			 */
			if (time - dnxm_errcnt.dnxm_prev > TIME_THRESH) {
				if (retry)
					recover = 1;
			}
			/*
			 * Log the machine check, and the error status regs.
			 * If we can recover update the times,
			 * else print errors on the console.
			 * Last, clear the error bits.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			if (recover) {
				dnxm_errcnt.dnxm_prev = dnxm_errcnt.dnxm_last;
				dnxm_errcnt.dnxm_last = time;
			} else {
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
			}
			cvqmerr->cvq1_dser |= DSER_CLEAR;
		} else {
			/*
			 * Undefined Machine check 0x80, 0x81.
			 * Log the mcheck, ESR Packet, & Mem Packet.
			 * We can't recover so print errors on the console.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			mprintf("No primary error flag - unspecified error type\n");
			ka650logesrpkt(recover);
			ka650logmempkt(recover);
			ka650consprint(2,type,mcf);
			cprintf("No primary error flag - unspecified error type\n");
			ka650consprint(3,0,0);
			ka650consprint(4,0,0);
			cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
		}
		break;
	case 0x82:
	case 0x83:
		/*
		 *   Re-enable (after flushing) the 2nd level cache if cache
		 *   was enabled before we entered mcheck (we may recover, plus
		 *   we log it in the state it was in when mcheck occured).
		 */
		if (cache2_state) {
			cacheptr = cvqcache->cvq5_cache;
			cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
			for (; cacheptr < cacheend; ) {
				*cacheptr++ = 0;
			}
			cvqcb->cvq2_cacr1 |= CACR_CEN;
		}
		/*
		 * What is logged & printed depends on the cause of the error.
		 * If we can't recover print errors on the console.
		 * Last, clear the error bits.
		 */
		if ((cvqmerr->cvq1_memcsr16 & MEM_CDAL) != 0) {
			/*
			 * CDAL Bus Parity Error
			 * Log the mcheck, ESR Packet, & Mem Packet.
			 */
			if (time - cdalW_errcnt.cdalW_prev > TIME_THRESH) {
				if (retry)
					recover = 1;
			}
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			ka650logmempkt(recover);
			if (recover) {
				cdalW_errcnt.cdalW_prev = cdalW_errcnt.cdalW_last;
				cdalW_errcnt.cdalW_last = time;
			} else {
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
				ka650consprint(4,0,0);
			}
			cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
		} else if ((cvqmerr->cvq1_memcsr16 & MEM_RDS) != 0) {
			/*
			 * Main memory uncorrectable ECC error:
			 * If not recovering, disable Main Mem error
			 *	detection & correction to get core dump.
			 * NOTE: logmempkt sets the global var ka650_module
			 *	 "ka650_module" is set to a coded number!
			 * NOTE: consprint() sets up display for cprintf here!
			 */
			if (time - rdsW_errcnt.rdsW_prev > TIME_THRESH) {
				if (retry)
					recover = 1;
			}
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			ka650logmempkt(recover);
			if (recover) {
				rdsW_errcnt.rdsW_prev = rdsW_errcnt.rdsW_last;
				rdsW_errcnt.rdsW_last = time;
			} else {
				cvqmerr->cvq1_memcsr17 |= MEM_ERRDIS;
				ka650consprint(2,type,mcf);
				ka650consprint(3,0,0);
				ka650consprint(4,0,0);
				cprintf("Uncorrectable ECC Error detected in Main Memory <%x>\n", ka650_module);
			}
			cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
		} else if ((cvqmerr->cvq1_dser & DSER_NOGRANT) != 0) {
			/*
			 * No Grant time-out on cpu writing to CQBIC regs
			 * We can't recover so print errors on console.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			ka650logesrpkt(recover);
			ka650consprint(2,type,mcf);
			ka650consprint(3,0,0);
			cvqmerr->cvq1_dser |= DSER_CLEAR;
		} else {
			/*
			 * Unspecific mcheck 0x82 or 0x83: log everything.
			 * We can't recover so print errors on console.
			 */
			logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
			mprintf("No primary error flag - unspecified error type\n");
			ka650logesrpkt(recover);
			ka650logmempkt(recover);
			ka650consprint(2,type,mcf);
			cprintf("No primary error flag - unspecified error type\n");
			ka650consprint(3,0,0);
			ka650consprint(4,0,0);
		}
		break;
	default:
		/*
		 *   Re-enable (after flushing) the 2nd level cache if cache
		 *   was enabled before we entered mcheck (this is so we
		 *   log it in the state it was in when mcheck occured).
		 */
		if (cache2_state) {
			cacheptr = cvqcache->cvq5_cache;
			cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
			for (; cacheptr < cacheend; ) {
				*cacheptr++ = 0;
			}
			cvqcb->cvq2_cacr1 |= CACR_CEN;
		}
		/*
		 * Unrecognized mcheck: these are non-recoverable.
		 * Log the mcheck, err status regs & memerr packets
		 * Also print to the console.
		 */
		logmck((int *)cmcf, ELMCKT_CVAX, cpunum, recover);
		ka650logesrpkt(recover);
		ka650logmempkt(recover);
		type = 0;
		ka650consprint(2,type,mcf);
		ka650consprint(3,0,0);
		ka650consprint(4,0,0);
		break;
	}
	if (!recover){
		panic (mchk_string);
	}
	ka650_mchkprog = 0;
	return(0);
}


/*
 * Log memory errors in kernel buffer:
 * Always log packet 3 (error status registers),
 * log pkt 4 (memory CSRs) if RDS or undefined memory error.
 *
 * Traps through SCB vector 60: uncorrectable memory errors.
 */
ka650memerr()
{
	register int recover;	/* set to 1 if we can recover from this error */
	register u_int time;	/* from TODR */

	recover = 0;
	/*
	 * First note the time; then determine if hardware retry is
	 * possible, which will be used for the recoverable cases.
	 */
	time = mfpr(TODR);
	mprintf("memerr interrupt\n");
	if ((cvqmerr->cvq1_memcsr16 & MEM_CDAL) != 0) {
		/*
		 * CDAL parity error
		 * Log ESR Packet & Mem Packet.
		 */
		if (time - cdalW_errcnt.cdalW_prev > TIME_THRESH) {
			recover = 1;
		}
		ka650logesrpkt(recover);
		ka650logmempkt(recover);
		if (recover) {
			cdalW_errcnt.cdalW_prev = cdalW_errcnt.cdalW_last;
			cdalW_errcnt.cdalW_last = time;
		} else {
			ka650consprint(3,0,0);
			ka650consprint(4,0,0);
		}
		cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
	} else if ((cvqmerr->cvq1_memcsr16 & MEM_RDS) != 0) {
		/*
		 * Main memory uncorrectable ECC error:
		 * Disable Main Mem error detection & correction
		 *     to protect mcheck code & to get core dump.
		 * NOTE: logmempkt sets the global var ka650_module
		 *	 "ka650_module" is set to a coded number!
		 * NOTE: consprint() sets up display for cprintf here!
		 */
		cvqmerr->cvq1_memcsr17 |= MEM_ERRDIS;
		ka650logesrpkt(0);
		ka650logmempkt(0);
		ka650consprint(3,0,0);
		cprintf("Uncorrectable ECC Error detected in Main Memory <%x>\n", ka650_module);
		ka650consprint(4,0,0);
		cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
		cvqmerr->cvq1_dser |= DSER_CLEAR;
	} else if ((cvqmerr->cvq1_dser & DSER_QNXM) != 0) {
		/*
		 * Q-22 Bus NXM
		 */
		ka650logesrpkt(0);
		ka650consprint(3,0,0);
		cvqmerr->cvq1_dser |= DSER_CLEAR;
	} else if ((cvqmerr->cvq1_dser & DSER_DNXM) != 0) {
		/*
		 * Q-22 Bus DMA NXM (main memory NXM via QBus map access)
		 */
		ka650logesrpkt(0);
		ka650consprint(3,0,0);
		cvqmerr->cvq1_dser |= DSER_CLEAR;
	} else {
		/*
		 * Undefined memory error interrupt.
		 */
		ka650logesrpkt(0);
		ka650logmempkt(0);
		ka650consprint(3,0,0);
		ka650consprint(4,0,0);
	}
	panic (memerr_string);
}

/*
 * Log CRD memory errors in kernel buffer:
 * Log packet 3 (error status registers) &
 *     packet 4 (memory CSRs) if a true memory error (not a cache parity error).
 *
 * Traps through SCB vector 54: correctable memory errors.
 *
 * These errors are recoverable.
 */
ka650crderr()
{
	register int *cacheptr;		/* ptr to flush 2nd level cache */
	register int *cacheend; 	/* ptr to end of 2nd level cache */
	int recover;	/* set to 1 if we can recover from this error */
	u_int time;

	recover = 0;
	time = mfpr(TODR);
	if ((cvqrom->cvq7_sysdep != SB_KA640) &&
	    (cvqcb->cvq2_cacr1 & CACR_CPE) != 0) {
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
			mprintf("CRD interrupt: 2nd level cache tag parity\n");
			recover = 1;
			if (time - tag_errcnt.tag_prev <= TIME_THRESH_C2) {
				/*
				 * Cache was on, 3 errs in 1 time period
				 */
				printf("2nd Level Cache DISABLED by software on CRD\n");
				ka650logesrpkt(recover);
				cvqcb->cvq2_cacr1 |= CACR_CPE;
				cache2_state = 0;
				tag_errcnt.tag_last = 0;
				time = 0;
			} else {
				/*
				 * Cache was on, < 3 errs in 1 time period
				 * Flush and re-enable cache
				 * Note: must clear error bit before re-enable
				 */
				ka650logesrpkt(recover);
				cvqcb->cvq2_cacr1 |= CACR_CPE;
				cacheptr = cvqcache->cvq5_cache;
				cacheend = cvqcache->cvq5_cache + CACHE_SIZE;
				for (; cacheptr < cacheend; ) {
					*cacheptr++ = 0;
				}
				cvqcb->cvq2_cacr1 |= CACR_CEN;
				mprintf("2nd Level Cache re-enabled by software\n");
			}
			tag_errcnt.tag_prev = tag_errcnt.tag_last;
			tag_errcnt.tag_last = time;
		} else {
			/*
			 * Cache was off: no recover.
			 * NOTE: consprint() sets up display for cprintf here!
			 */
			ka650logesrpkt(recover);
			ka650consprint(3,0,0);
			printf("2nd Level Cache Tag Parity Error occured with Cache Disabled!\n");
			cvqcb->cvq2_cacr1 |= CACR_CPE;
			panic("ka650 Cache Tag Parity error");
		}
	} else {
		/*
		 * Main memory correctable ECC error (& catchall):
		 * Log the error status regs & memCSRs.
		 * If 3 within 1 time period disable CRD interrupts, and post "hi-rate".
		 *   (They are automatically re-enabled through timeout
		 *    calls to memenable in machdep.c)
		 * Finally clear the error bit & return.
		 */
		mprintf("CRD interrupt\n");
		ka650logesrpkt(2);
		ka650logmempkt(2);
		if (time - crd_errcnt.crd_prev <= TIME_THRESH) {
			cvqmerr->cvq1_memcsr17 &= ~(MEM_CRDINT);
			mprintf("Hi-Rate CRD log\n");
		}
		crd_errcnt.crd_prev = crd_errcnt.crd_last;
		crd_errcnt.crd_last = time;
		cvqmerr->cvq1_memcsr16 |= MEM_EMASK;
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

ka650consprint(pkt, type, mcf)
	int pkt;	/* error pkt desired:	2 = mcheck frame
			   			3 = error status registers
			   			4 = memory CSRs */
	int type;	/* machine check type (for pkt type 2) */
	register struct mcCVAXframe *mcf; /* mcheck frame pointer (for type 2)*/
{
	register int i;
	register u_long *mp;		/* ptr to memcsr 0-15 */
	register u_long memcsr16;
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
			type = type - 0x80 + MCcVAXDISJ;
		cprintf("%s\n", mcCVAX[type]);
		cprintf("\tcode\t= %x\n", mcf->mc1_summary);
		cprintf("\tmost recent virtual addr\t=%x\n", mcf->mc1_vap);
		cprintf("\tinternal state 1\t=%x\n", mcf->mc1_internal_state1);
		cprintf("\tinternal state 2\t=%x\n", mcf->mc1_internal_state2);
		cprintf("\tpc\t= %x\n", mcf->mc1_pc);
		cprintf("\tpsl\t= %x\n\n", mcf->mc1_psl);
		break;
	case 3:
		if (cvqrom->cvq7_sysdep != SB_KA640)
			cprintf("\tcacr\t= %x\n", cvqcb->cvq2_cacr1);
		cprintf("\tdser\t= %x\n", cvqmerr->cvq1_dser);
		cprintf("\tqbear\t= %x\n", cvqmerr->cvq1_qbear);
		cprintf("\tdear\t= %x\n", cvqmerr->cvq1_dear);
		cprintf("\tcbtcr\t= %x\n", cvqssc->ssc_cbtcr);
		cprintf("\tipcr0\t= %x\n", cvqipcr->cvq6_ipcr0);
		cprintf("\tcadr\t= %x\n", mfpr(CADR));
		cprintf("\tmser\t= %x\n", mfpr(MSER));
		break;
	case 4:
		memcsr16 = cvqmerr->cvq1_memcsr16;
		cprintf("\tmemcsr16\t= %x\n", memcsr16);
		cprintf("\tmemcsr17\t= %x\n", cvqmerr->cvq1_memcsr17);
		mp = &(cvqmerr->cvq1_memcsr0);
		/*
		 * Clear ka650_memcon <27:24> so it can be set to the memory
		 *   csr number that had an error.
		 * Look at all 16 MEMCSRs and determine which bank had the error
		 */
		ka650_memcon = (ka650_memcon & 0xf0ffffff);
		for (i = 0; i < 16; i++, mp++) {
			if (((*mp) & MEM_BNKENBLE) &&
			    ((*mp) & MEM_BNK) == (memcsr16 & MEM_BNK)) {
				ka650_memcon = ka650_memcon | (i << 24);
				cprintf("\tmemcsr%d\t= 0x%x\n", i, *mp);
				cprintf("\tmemcon\t= 0x%x\n", ka650_memcon);
				break;
			}
		}
		break;
	}
}

/*
 * Log Error & Status Registers (packets 3 & 5 of KA650 error spec)
 */

ka650logesrpkt(priority)
	int priority;		/* for pkt priority */
{
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
		if (cvqrom->cvq7_sysdep == SB_KA640)
			elrp->el_body.elesr650.esr_cacr = -1;
		else
			elrp->el_body.elesr650.esr_cacr = cvqcb->cvq2_cacr1 |
			    cvqcb->cvq2_cacr2 << 8 | cvqcb->cvq2_cacr3 << 16 ;
		elrp->el_body.elesr650.esr_dser = cvqmerr->cvq1_dser;
		elrp->el_body.elesr650.esr_qbear = cvqmerr->cvq1_qbear;
		elrp->el_body.elesr650.esr_dear = cvqmerr->cvq1_dear;
		elrp->el_body.elesr650.esr_cbtcr = cvqssc->ssc_cbtcr;
		elrp->el_body.elesr650.esr_ipcr0 = cvqipcr->cvq6_ipcr0;
		elrp->el_body.elesr650.esr_cadr = mfpr(CADR);
		elrp->el_body.elesr650.esr_mser = mfpr(MSER);
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

ka650logmempkt(recover)
	int recover;		/* for pkt priority */
{
	register u_long memcsr16;
	register u_long *mp;		/* ptr to memcsr 0-15 */
	register int i;
	int merrtype = EL_UNDEF;
	struct el_rec *elrp;
	struct el_mem *mrp;

	memcsr16 = cvqmerr->cvq1_memcsr16;
	/*
	 * Clear ka650_memcon <27:24> so it can be set to the memory
	 *   csr number that had an error.
	 * Look at all 16 MEMCSRs and determine which bank had the error
	 */
	ka650_memcon = (ka650_memcon & 0xf0ffffff);
	mp = &(cvqmerr->cvq1_memcsr0);
	for (i = 0; i < 16; i++, mp++) {
		if (((*mp) & MEM_BNKENBLE) &&
		    ((*mp) & MEM_BNK) == (memcsr16 & MEM_BNK)) {
			ka650_memcon = ka650_memcon | (i << 24);
			elrp = ealloc(EL_MEMSIZE,recover ? EL_PRIHIGH : EL_PRISEVERE);
			if (elrp != NULL) {
				LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_650,EL_UNDEF,EL_UNDEF,EL_UNDEF);
				if (memcsr16 & MEM_RDS)
					merrtype = ELMETYP_RDS;
				else if (memcsr16 & MEM_CRD)
					merrtype = ELMETYP_CRD;
				mrp = &elrp->el_body.elmem;
				mrp->elmem_cnt = 1;
				mrp->elmemerr.cntl = 1;
				mrp->elmemerr.type = merrtype;
				mrp->elmemerr.numerr = 1;
				mrp->elmemerr.regs[0] = memcsr16;
				mrp->elmemerr.regs[1] = cvqmerr->cvq1_memcsr17;
				mrp->elmemerr.regs[2] = *mp;	/* the memcsr */
				mrp->elmemerr.regs[3] = ka650_memcon;
				EVALID(elrp);
			}
			switch ((ka650_memcon & 0x0f000000) >> 24) {
			case 0: case 1: case 2: case 3:
				ka650_module = 0x39;
				break;
			case 4: case 5: case 6: case 7:
				ka650_module = 0x3B;
				break;
			case 8: case 9: case 10: case 11:
				ka650_module = 0x3D;
				break;
			case 12: case 13: case 14: case 15:
				ka650_module = 0x3F;
				break;
			default:
				ka650_module = 0x0;
				break;
			}
			break;
		}
	}
}
