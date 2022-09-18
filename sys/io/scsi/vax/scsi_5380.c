#ifndef lint
static	char	*sccsid = "@(#)scsi_5380.c	4.4  (ULTRIX)        1/3/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,87,88,89 by		*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * scsi_5380.c	17-Aug-88
 *
 * VAX SCSI device driver (NCR 5380 routines)
 *
 * Modification history:
 *
 *   11/21/90	Robin Miller
 *	o  Added support for 10-byte Read/Write CDB's.
 *	o  Removed '#ifdef FORMAT' conditionalization, since this special
 *	   interface is always desired.
 *	o  Use function sz_cdb_length() to obtain the Command
 *	   Descriptor Block (CDB) length.
 *
 *   08/30/90	Robin Miller
 *	Added check in probe routine to recognize RX26 device type
 *	and setup accordingly for floppy device.
 *
 *   05-Jul-90	Bill Dallas
 *	Added support for the TZK10 and the new device option table
 *
 *   01-Mar-90  Janet Schank / Fred Canter
 *      Added retries on busy in the probe routine.
 *
 *   02-Dec-89	Fred Canter
 *	Calculate the number of PTEs needed to double map the user's
 *	buffer in a machine independant way (no more magic numbers).
 *
 *   13-Nov-89  Janet Schank
 *      scsi_data.c has moved from data/vax/scsi_data.c to data/scsi_data.c
 *
 *   08-Oct-89	Fred Canter
 *	Remove error log #ifdef OLDWAY.
 *
 *   07-Oct-89	Fred Canter
 *	Save removable media bit from inquiry data in sz_softc.
 *
 *   01-Oct-89	Fred Canter
 *	Added error log debug code (#ifdef ELDEBUG). This allows the
 *	driver to produce most error types for uerf debuging.
 *
 *   23-Jul-89	Fred Canter
 *	RX33 support.
 *
 *   14-Jul-89	Fred Canter
 *	Merged Dynamic BBR and error log changes.
 *
 *   24-Jun-89	Fred Canter
 *	Convert printf/mprintf error messages to scsi_logerr() calls.
 *
 *   23-Jun-89	Fred Canter
 *	Save status byte for current command for error log.
 *
 *   22-Jun-89	John A. Gallant
 *	Added the driver completion routine pieces.
 *
 *   18-Jun-89	Fred Canter
 *	Convert to scsi_devtab.
 *
 *   10-Jun-89	Fred Canter
 *	Added #ifdef SZ_BT_DEBUG around disconnect BSY timing debug code.
 *
 *   03-Jun-89	Fred Canter
 *	Move soft copy of scs_selena register to sz_softc.
 *
 *   02-Jun-89	Fred Canter
 *	Fixed a latent bug in the probe routine which cuased a panic
 *	during booting if the second SCSI controller was not present.
 *
 *   29-May-89	Fred Canter
 *	Changed disconnect timeout (timer2) from 4 seconds to 10 seconds
 *	for hard disks and 30 seconds for floppy disks.
 *
 *   24-May-89	Fred Canter
 *	Changes to match the new, more general rzdisk utility.
 *	Changed the mode select data structures so an address and
 *	length are passed with the ioctl. This allows pages to be
 *	added to rzdisk without requiring a kernel rebuild.
 *
 *   24-Apr-89	Fred Canter
 *	Added support for the RX23s SCSI floppy.
 *
 *   23-Apr-89	Fred Canter
 *	If already in the interrupt routine, then don't take another
 *	interrupt when a target disconnects, unless its slow dropping BSY.
 *
 *   06-Apr-89	Fred Canter
 *	Added TZxx (EXABYTE) support:
 *		Add TZxx code to szprobe().
 *		Don't send VU bits with mode select (modsel.pll = 12).
 *		Handle target returning busy status (EXABYTE does this).
 *
 *	Check for hung bus after SSUNIT in szprobe. Reset bus if hung.
 *	Done so inquiry for next device will not fail.
 *
 *	Only timeout disconnect on disk read, write, and read capacity
 *	commands. Disk start unit takes much longer than 4 seconds.
 *
 *	Added the capability to do extended mode select for TZxx tapes.
 *
 *   11-Feb-89	Fred Canter
 *	Fixed yet another bug with 64KB transfers. Use blkcpy instead
 *	of bcopy (bcopy max byte count is 64KB -1 ) for copying
 *	data to and from the users' buffer.
 *
 *   09-Feb-89	Fred Canter
 *	Fixed a bug which cause an inquiry command to hang in data in
 *	phase when executed from rzdisk (DMA data in). The byte count
 *	was never setup. Only worked by luck previously.
 *	Fixed minor typo (SCS_PARCK | SCS_PARCK) in two places.
 *
 *   08-Feb-89	Fred Canter
 *	Changed sc_sel_retry (select timeout retry count) to per target.
 *
 *   07-Feb-89	Fred Canter
 *	Add function header comments for each routine.
 *
 *   06-Feb-89	Fred Canter
 *	Remove rz_max_xfer, cz_max_xfer, and tz_max_xfer.
 *	Cleanup comments and minor code rearranement.
 *
 *   04-Feb-89	Fred Canter
 *	Modified 128KB hardware DMA buffer allocation scheme:
 *	    Allocate in 512 byte chunks instead of 1024 byte chunks.
 *	    Don't use last page (5380 1 byte prefetch on DMA write).
 *	    Allocate fragment to a disk (use all of the buffer).
 *
 *	Fixed a panic caused by transferring odd byte counts, like
 *	8193 to a tape. The 5380 prefetching an extra byte on the
 *	DMA write caused the saved count to be 0 when it should have
 *	been -1 (really one byte left to transfer).
 *
 *   30-Dec-88	Fred Canter
 *	Added szreset() to reset SCSI controllers before calling the
 *	VMB boot drive to write out a crash dump.
 *
 *   21-Dec-88	Fred Canter
 *	Fixed bug in the save data pointer code which reset the sc_savcnt
 *	if the previous phase was not data out. This made the defect
 *	list appear trashed.
 *
 *   18-Dec-88	Fred Canter
 *	Added implied save data pointer to disconnect (only if target
 *	disconnects without sending a save data pointer nmessage first).
 *
 *   17-Dec-88	Fred Canter
 *	Added pseudo commands to resolve the conflict between
 *	SZ_UNLOAD and SZ_SSUNIT both being opcode 0x1b.
 *
 *   16-Dec-88	Fred Canter
 *	Restructure the lost arbitration code to simplify bus reset
 *	detection and improve readability.
 *	Removed sz_ss_oldsel debug code (spin instead of time select).
 *	Close (almost completely) the window for missing detection
 *	of a bus reset in the sz_scsistart() arbitration code.
 *
 *   11-Dec-88	Fred Canter
 *	Modified parity checking and bus reset detection so that we
 *	don't read any of the SCSI controller registers while a DMA
 *	is in progress. Under the old method (using INTPAR) a parity
 *	error on a read would cause a read of the scs_status register
 *	in szintr. Now use PARCK being cleared for bus reset detection.
 *	PARCK is now always on (except during arbitration).
 *
 *	Slight change in select timing to it in line with SCSI spec,
 *	i.e., drop IDs from data bus at same time drop SEL.
 *
 *   04-Dec-88	Fred Canter
 *	Remove hack which limited maximum transfer to 64KB - 512.
 *	64KB transfers used to fail (or so I thought), but work now.
 *	Added debug variable to allow selected target(s) to be ignored.
 *
 *   03-Dec-88	Fred Canter
 *	Don't do the 4 second disconnect timeout on RZSPECIAL commands
 *	(format, reassign, vfy_data, and rdd).
 *
 *   03-Nov-88	Alan Frechette
 *	Added in support for disk maintainence. Added the commands
 *	FORMAT UNIT, REASSIGN BLOCK, READ DEFECT DATA, and VERIFY 
 *	DATA. Modified MODE SELECT and MODE SENSE. Figured out a
 *	better way to determine the length of a SCSI command in
 *	"sz_scsistart()" and "szintr()" during the COMMAND PHASE.
 *	Made the DATA OUT phase in "sz_scsistart()" and "szintr()"
 *	into a switch statement.
 *
 *   16-Oct-88 -- Fred Canter
 *	Massive code and comment cleanup.
 *	Moved #define SZDEBUG to scsireg.h.
 *	Add variable to control width of bus reset.
 *	Add RZxx (non-DEC) disk support.
 *	Fixed a bug in the selection timeout code and
 *	added retires of select failures.
 *	Take a phase mismatch interrupt instead of spinning
 *	waiting for phase changes (critical ones only).
 *	Put #ifdefs arount spin stats code.
 *	Added delay after write to scd_adr register to
 *	work around SCSI/SCSI controller hardware bug.
 *	Modified arbitration and selection code to work around
 *	the latest NCR 5380 chip problem.
 *	Clean up code which waits for BSY to drop after command complete.
 *
 *   28-Sep-88 -- Fred Canter
 *	Clean up comments.
 *	Clean up code and comments for device failed to select timeout.
 *	Remove work around code for hardware loosing interrupts.
 *	Bug fix - waited for 1 second after abort for bus to free up,
 *	changed to 1 MS.
 *	The sz_timer1() is now used as a background timer to catch
 *	bus hangs when the drive is not active.
 *	Dump SCSI registers on CMDCPT BSY held too long error.
 *	Modify order of DMA address and direction register writes
 *	when starting DMA. Also add 3 Usec delay after write to
 *	address register, This works around a bug in SCSI/SCSI chip.
 *	which hangs the SCSI/SCSI controller.
 *	Don't do reselect timeout (timer2) on RRD40. It tries the
 *	reselect for ever.
 *
 *   13-Sep-88 -- Fred Canter
 *	Only check parity on DMA in and read of current data register.
 *	Add spl6() around critical code segment in szintr reselect.
 *	All BSY checks now three times (wire or glitch).
 *	New routine to dump SCSI registers.
 *
 *   21-Aug-88 -- Fred Canter
 *	Improved device failed to select error handling. Spin for a
 *	short time. Use a timeout for the remaining time.
 *	Insure all messages accounted for in unknown/unsupported
 *	message error (MESSI:).
 *	Added sz_reset() and other support for bus reset.
 *
 * 17-Aug-88	Fred Canter
 *	Created this file by moving the SCSI NCR 5380 specific files
 *	from the old combined driver (scsi.c) to scsi_5380.c.
 *
 ***********************************************************************/

/*
 ***********************************************************************
 *
 * NOTES:
 *
 *  1.	SCSI controller registers must not be accessed while a DMA
 *	is in progress. Hardware engineering says the scd_adr & scd_cnt
 *	registers can be accessed safely, but this has not been verified.
 *	They also say all registers can be accessed during a DMA on
 *	the SCSI/SCSI controller. This also has not been verified.
 *
 *  2.	Reading the SCSI bus data bits (scs_curdata) when the signals
 *	are not driven can (and most likely will) cause a psrity error.
 *	In general the signals are not driven when BSY is false. One
 *	exception to this is when a disconnected target reselects the
 *	initator (VS3100 system). BSY is false but the signals are driven.
 *
 *  3.	The ANSI SCSI spec calls for a 250 MS selection timeout. The
 *	timeout is implemented in scsistart and any routines which
 *	call scsistart. If scsistart is called from the probe routine,
 *	we spin for 250 MS in scsistart. Calling scsistart from the
 *	probe routine is always a special case.
 *	On normal calls to scsistart, we spin for 20 MS in scsistart
 *	and if BSY does not set we start a series of timeours (timer3).
 *	We do one 30 MS timeout then four 50 MS timeouts. The messy
 *	part of this is that the routine calling scsistart must
 *	unwind its call and be prepared to be called back when the
 *	timer decides the selection completed or timed out.
 *
 *  4.	The RST (bus reset) bit in the cur_stat register is not latched.
 *	So, the bus reset condition my be gone by the time we get the
 *	interrupt. We always have PARCK set in the mode register.
 *	If PARCK ever gets cleared, we assume a bus reset was the cause.
	PARCK is cleared during arbitration, so the resetting of the
	ARB bit is used to detect a bus reset during arbitration.
 *
 *  5.	This driver contains many code sequences like the following:
 *
 *		if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
 *		    ((szaddr->scs_curstat & SCS_BSY) == 0) &&
 *		    ((szaddr->scs_curstat & SCS_BSY) == 0)) {
 *			do_something_wonderfull.......
 *		}
 *
 *	Checking a bus signal three times takes into account the 400
 *	nanosecond bus settle delay (wired or glitch).
 *
 *  6.	The 5380 detects parity errors on DMA input and CPU reads of
 *	the scs_curdata register. We read this register during programmed
 *	I/O input, reselection, and arbitration. Parity checking (PARCK
 *	bit in mode register) must be disabled during arbitration. This
 *	is because several targets can be driving the bus data lines
 *	during arbitration. This can (in fact does) cause parity errors!
 *
 *
 *  7.	When checking for a hung bus, must realize that any target
 *	on the bus can arbitrate and assert BSY when the target (or
 *	initator) who currently has BSY asserted releases BSY. The
 *	time between one target droping BSY and another target
 *	asserting BSY can be very short (faster than the driver can
 *	detect with a spin loop). If BSY is asserted the bus is
 *	probably hung. However, if SEL and I/O are asserted some
 *	target is doing a reselect (bus not hung).
 *
 ***********************************************************************
 */

#include "../data/scsi_data.c"

/*
 * Define ELDEBUG to create error for uerf debuging.
 * NOTE: must not be defined at submit time (performance hit).
 */
/*#define	ELDEBUG	*/

/*
 * The #define for SZDEBUG is in scsireg.h.
 */
#ifdef SZDEBUG
extern int szdebug;
#define printd if(szdebug)printf
#define printd1 if (szdebug > 1) printf
#define printd2 if (szdebug > 2) printf
#define printd3 if (szdebug > 3) printf
#define printd4 if (szdebug > 4) printf
#define printd5 if (szdebug > 5) printf
#define printd6 if (szdebug > 6) printf
#endif SZDEBUG

/*
 * Controls debug code whichs makes sure each
 * command does a status phase. This bug was fixed
 * but the code hangs around just in case.
 */
/*#define	SZ_DSP_DEBUG*/

/*
 * DEBUG - normally not defined.
 * Number of 10 Usec increments each device
 * held BSY after command complete.
 * CAUTION - must increase array size if NSCSI > 2
 */
/* #define	SZ_BSYCC_STATS	*/

#ifdef	SZ_BSYCC_STATS

int	sz_i_bsycc[16];
int	sz_ss_bsycc[16];

#endif	SZ_BSYCC_STATS

u_char	inicmd_tmp;		/* use to write scs_inicmd reg (can't |= ) */

/*
 * NOTE:
 *	The probe routine is called before cache(s)
 *	are enabled. Microdelay() now deals with this.
 *	Turning cache on broke the ST506 disk driver (sdc.c).
 *	It failed during read if XBNs (DO NOT KNOW WHY).
 */

int	sz_strncmp();
int	sz_strcmp();

extern int rzcomplete();	/* disk driver completion routine */
extern int tzcomplete();	/* tape driver completion routine */
extern int sz_cdb_length();

extern struct nexus nexus[];

extern	int	cpu;

/*
 * Number of seconds to wait for SCSI devices
 * to come ready enough to respond to inquiry
 * after bus reset. Needs to be a variable so
 * users can change it (I am such a nice guy).
 * Factory default is 7 seconds (in scsi_data.c).
 */
extern int sz_wait_for_devices;
extern int sz_max_wait_for_devices;

/*
 * Number of microseconds we wait after each inquiry
 * command. Just to be absolutely sure the bus is free.
 * This is delay may not be necessary, but at one point
 * it appeared the RRD40 held the bus after inquiry.
 */
int	sz_wait_after_inquiry = 1000;

int	sz_scsistart();
int	sz_reset();
int	sz_timer1();


/*
 * Next four variables defined in scsi.c because
 * they are shared between scsi_5380.c and scsi_sii.c.
 */
extern int szp_firstcall;	/* first call to probe routine */
extern int szp_nrz;		/* number of RZ disks found */
extern int szp_ntz;		/* number of TZ tapes found */
extern int szp_ncz;		/* number of CDROM optical disks found */
extern int szp_nrx;		/* number of RZ23 SCSI floppy disks found */
extern struct scsi_devtab szp_rz_udt;
extern struct scsi_devtab szp_tz_udt;
extern struct scsi_devtab szp_cz_udt;

/*
 * Length of SCSI bus reset in Microseconds.
 * Set in data/scsi_data.c so customers can
 * change it.
 * Factory default is 50 Usec.
 */
extern int sz_reset_width;

/*
 * Each array element represents a SCSI bus.
 * Set bit 0 to ignore target ID 0, etc.
 * Ment for debugging only, but.....
 * Allow for up to 8 SCSI busses (current max is 2).
 * Not in scsi_data.c so novice uses won't play with it!
 */
#define	SZ_MAX_SCSI	8
int	sz_ignore_target[SZ_MAX_SCSI] = {0, 0, 0, 0, 0, 0, 0, 0};


/*
 *
 * Name:		szprobe		-Probe routine
 *
 * Abstract:		Probe entry point from auto-configure code.
 *			Determine if controller exists. Reset SCSI bus.
 *			Size number and type of targets on SCSI bus.
 *			Set up sz_softc and 128KB hardware buffer.
 *
 * Inputs:
 *
 * reg			SCSI bus controller CSR address (not used).
 * cntlr		SCSI bus controller number (0 = bus A, 1 = bus B).
 *
 * Outputs:
 *
 * sz_softc
 *
 *  sc_sysid		SCSI bus ID of initiator (CPU).
 *  sc_cntlr_alive	Controller alive or not.
 *  *port_start()	SCSI command start routine - sz_scsistart().
 *  *port_reset()	SCSI bus reset - sz_reset() [set but not used].
 *  *device_comp()	SCSI device completion routine
 *  *sc_rambuff		Virtual address of 128KB hardware RAM buffer.
 *  sc_segcnt[]		Per target - 128 KB buffer slot size (max byte cnt).
 *  sc_dstp[]		Per target - disk default partition table pointer.
 *  sc_devtyp[]		Per target - device type (see scsireg.h).
 *  sc_alive[]		Per target - device present at this SCSI bus ID.
 *  sc_device[][]	Per target - product ID, vendor ID, rev in ascii.
 *  sc_SZ_bufmap[]	Per target - virtual address for buffer mapping.
 *  sc_szbufmap[]	Per target - PTEs from get_sys_ptes().
 *  sc_dboff[]		Per target - offset into 128 KB buffer.
 *  sc_devnam[][]	Per target - vendor & product ID in ascii.
 *  sc_revlvl[][]	Per target - revision level in ascii.
 *
 * Return values:
 *
 * 0			Controller does not exist or did not respond.
 *
 * 1			Controller exists and is alive.
 *
 * Side Effects:
 *
 *			The NCR 5380 chip, the DMA controller, and the
 *			SCSI bus are reset. Much of sz_softc is set up.
 *			Port start and reset routine pointers set up.
 *			Address of 128 KB RAM buffer set up.
 *			Clears diagnostic timer register (nb_diagtime).
 *			Starts hung bus detection timer (sz_timer1).
 *
 */

szprobe(reg, cntlr)
	caddr_t reg;	/* NOT USED */
	int cntlr;
{
	register struct sz_regs *szaddr = (struct sz_regs *)szmem + cntlr;
	register struct nb_regs *sziaddr = (struct nb_regs *)nexus;
	register struct sz_softc *sc;
	int targid, ctlr;
	int dboff;
	int rz_slotsize, tz_slotsize, cz_slotsize, rx_slotsize;
	int rz_fragsize, rz_fragused;
	int ncz, ntz;
	int i, s;
	struct sz_inq_dt *idp;
	struct sz_rdcap_dt *rdp;
	struct scsi_devtab *sdp;
	struct scsi_devtab *usdp;
	int sdp_match;
	char *p;
	int alive;
	int stat, wedged;
	int retries;

#ifdef SZDEBUG
	printd6("szprobe: probing SCSI bus %d\n", cntlr);
#endif SZDEBUG

	/*
	 * Only on VS3100,
	 * return 0 if any other system.
	 */
	if (cpu != C_VAXSTAR)
		return(0);
	/*
	 * This driver needs the ST506/SCSI or
	 * SCSI/SCSI controller to run.
	 */
	if (((vs_cfgtst & VS_SC_TYPE) != VS_SCSI_SCSI) &&
	    ((vs_cfgtst & VS_SC_TYPE) != VS_ST506_SCSI))
		return(0);
	/*
	 * Probe must fail if controller not configured.
	 */
	alive = 1;
	if (cntlr >= nNSCSI)
	    alive = 0;
	/*
	 * Only SCSI/SCSI controller can have 2nd SCSI bus.
	 */
	if ((cntlr != 0) && ((vs_cfgtst & VS_SC_TYPE) != VS_SCSI_SCSI))
	    alive = 0;

	/*
	 * Assert SCSI bus reset for 50 Usec to
	 * clear the world and generate an interrupt.
	 */
	if (alive ) {
	    sziaddr->nb_int_reqclr = (SCS_IC_BIT >> cntlr);
	    sziaddr->nb_int_msk |= (SCS_IC_BIT >> cntlr);
	    szaddr->scs_inicmd = SCS_INI_RST;
	    DELAY(sz_reset_width);  /* hold bus reset for 50 Usec (min = 25) */
	    szaddr->scs_inicmd = 0;
	    i = szaddr->scs_reset;
	    sziaddr->nb_int_reqclr = (SCS_IC_BIT >> cntlr);
	    sziaddr->nb_int_msk &= ~(SCS_IC_BIT >> cntlr); /* disable intr */

	    /*
	     *	SCSI devices take some time after power on or bus reset
	     *	before they are ready to speak SCSI and respond to inquiry.
	     *	The RRD40 takes longest of the DEC devices (about 6 sec).
	     *
	     *	This delay can be changed via sz_wait_for_devices.
	     */
	    DELAY(sz_wait_for_devices * 1000000);
	}


	if (alive) {
	    sc = &sz_softc[cntlr];
	    sc->sc_active = 0;
	    sc->port_start = sz_scsistart;/* initialize the port_start switch */
	    sc->port_reset = sz_reset;    /* initialize the port_reset switch */
	    sc->sc_rambuff = 
		(char *)cvseddbmem;       /* initialize the RAM buffer pointer*/
	    /*
	     * Read system's SCSI bus ID from NVR
	     * and save it in sz_softc structure.
	     * Bits 2-4 are cntlr 0 ID, bits 5-7 are cntlr 1 ID.
	     */
	    i = sziaddr->nb_cpu_scsi_id >> 2;
	    if (cntlr == 0)
		i &= 0x7;
	    else
		i = (i >> 3) & 0x7;
	    sc->sc_sysid = (1 << i);
	    szaddr->scs_mode = SCS_PARCK;	/* for bus reset detection */
	}
	/*
	 * Use the inquiry command to determine number
	 * and type of targets on this controller.
	 * If this controller does not exist alive will
	 * be zero and the for loop won't do anything.
	 */
	for (targid=0; targid<NDPS; targid++) {
	    /* CAUTION: if cntlr not alive, sz_softc pointer not set! */
	    if (alive == 0)
		break;
	    /*
	     * Must set sc_unit here because it is used before it
	     * gets set in szslave(). This assumes a fixed mapping
	     * of logical to physical unit number.
	     */
	    sc->sc_unit[targid] = (8 * cntlr) + targid;
	    sc->sc_rzspecial[targid] = 0;
	    sc->sc_rmv_media &= ~(1 << targid);
	    if ((1 << targid) == sc->sc_sysid)
		continue;		/* skip initiator */
	    /*
	     * Allows target(s) to be ignored without
	     * being physically disconnected from the bus.
	     */
	    if (cntlr < SZ_MAX_SCSI) {
	        if (sz_ignore_target[cntlr] & (1 << targid))
		    continue;
	    }
	    sc->sc_curcmd[targid] = SZ_INQ;
	    sz_bldpkt(sc, targid, SZ_INQ, 0, 0);
	    /*
	     * Each failed select costs about .3 seconds.
	     * All selects fail if no target present at ID.
	     */
	    retries = 3;
	    i = sz_wait_for_devices;		/* # seconds already waited */
	    while (retries) {
		/* Zero inquiry data so we don't get stale information. */
		bzero ((struct sz_inq_dt *)&sc->sz_dat[targid],
						sizeof(struct sz_inq_dt));
	        stat = sz_scsistart(sc, targid, 0);
		if (sc->sc_szflags[targid] & SZ_BUSYTARG) {
		    sc->sc_szflags[targid] &= ~SZ_BUSYTARG;
		    DELAY(1000000);			/* delay 1 second */
		    if (++i >= sz_max_wait_for_devices)
			break;
		    continue;
		}
	        if (stat == SZ_SUCCESS)
		    break;
		retries--;
	    }
	    /*
	     * If the bus is wedged (BSY or SEL stuck)
	     * reset the bus. This is painfull but should allow
	     * remaining targets to be seen and prevent system
	     * hang cause by a wedged SCSI bus.
	     */
	    wedged = 0;
	    if ((szaddr->scs_curstat & SCS_BSY) &&
	        (szaddr->scs_curstat & SCS_BSY) &&
	        (szaddr->scs_curstat & SCS_BSY))
		    wedged++;
	    if ((szaddr->scs_curstat & SCS_SEL) &&
	        (szaddr->scs_curstat & SCS_SEL) &&
	        (szaddr->scs_curstat & SCS_SEL))
		    wedged++;
	    if (wedged) {
	        printf("szprobe: (bus=%d ID=%d) %s\n",
		    cntlr, targid, "BSY or SEL stuck after inquiry");
	        sz_dumpregs(cntlr, 0);	/* printf */
	        szaddr->scs_inicmd = SCS_INI_RST;
	        DELAY(sz_reset_width); /* hold bus reset 50 Usec (min = 25) */
	        szaddr->scs_inicmd = 0;
	        i = szaddr->scs_reset;
	        DELAY(sz_wait_for_devices * 1000000);
	        szaddr->scs_mode = SCS_PARCK;
	    }

	    sc->sc_szflags[targid] = SZ_NORMAL;		/* house keeping */
	    if (stat != SZ_SUCCESS)
	        continue;
	    /*
	     * Initialize data structures for this target and
	     * save all pertinent inquiry data (device type, etc.).
	     * NOTE: 1st 10KB of 128KB data buffer allocated
	     *	 to ST506 disk controller (only if present).
	     * NOTE: remaining buffer divided between other devices.
	     * TODO: need a resource map for the 128KB data buffer.
	     */
	    idp = (struct sz_inq_dt *)&sc->sz_dat[targid];

	    /* Save removable media bit for each target */
	    if (idp->rmb)
		sc->sc_rmv_media |= (1 << targid);

	    /*
	     * Zero device name and revsion level
	     * ASCII strings, so we know whether or
	     * not they were loaded by the INQUIRY.
	     */
	    for (i=0; i<SZ_DNSIZE; i++)
		sc->sc_devnam[targid][i] = (char)0;
	    for (i=0; i<SZ_REV_LEN; i++)
		sc->sc_revlvl[targid][i] = (char)0;

	    /*
	     * Save the device name and revision level.
	     * DEC combines vendor & product ID strings.
	     */
	    p = &sc->sc_devnam[targid][0];
	    for (i=0; i<SZ_VID_LEN; i++)
		*p++ = idp->vndrid[i];
	    for (i=0; i<SZ_PID_LEN; i++)
		*p++ = idp->prodid[i];
	    p = &sc->sc_revlvl[targid][0];
	    for (i=0; i<SZ_REV_LEN; i++)
		*p++ = idp->revlvl[i];

	    switch(idp->perfdt) {
	    default:		/* Unknown device type */
		printf("szprobe: scsi %d targetID %d: %s (%d).\n", cntlr,
		    targid, "unknown peripheral device type", idp->perfdt);
		/* NO 128 KB data buffer slot will be assigned! */
		sc->sc_alive[targid] = 0;
		sc->sc_devtyp[targid] = SZ_UNKNOWN;
		sc->sc_xstate[targid] = SZ_NEXT;
		sc->sc_xevent[targid] = SZ_BEGIN;
		bcopy(DEV_UNKNOWN, sc->sc_device[targid],
			strlen(DEV_UNKNOWN));
		break;
	    case 0:		/* Direct-access device (disk) */
	    case 1:		/* Sequential-access device (tape) */
	    case 5:		/* Read-only direct-access device (CDROM) */
		/*
		 * Allocate PTEs for data buffer double mapping.
		 * We are in BIG trouble if this fails! We print
		 * an error message, but the system will most
		 * likely stall, spin, crash, burn!
		 * Get enough PTEs to map 64kb + two guard pages.
		 */
		i = get_sys_ptes(btoc(64*1024)+2, &sc->sc_szbufmap[targid]);
		if (i == 0) {
		    printf("szprobe: scsi %d targetID %d: %s\n",
			cntlr, targid, "cannot get PTEs for bufmap");
		    break;
		}
		else
		    sc->sc_SZ_bufmap[targid] = (char *)i;
		sc->sc_alive[targid] = 1;
		sc->sc_szflags[targid] = SZ_NORMAL;	/* house keeping */
		sc->sc_xstate[targid] = SZ_NEXT;
		sc->sc_xevent[targid] = SZ_BEGIN;
		/*
		 * Find this device in the scsi_devtab in scsi_data.c.
		 * The answer could come back as unknown or missing.
		 */
		usdp = (struct scsi_devtab *)0;
		sdp_match = 0;
		for (sdp=scsi_devtab; sdp->namelen; sdp++) {
		    if ((idp->perfdt == 0) && ((sdp->devtype&SZ_DISK) == 0))
			continue;
		    if ((idp->perfdt == 1) && ((sdp->devtype&SZ_TAPE) == 0))
			continue;
		    if ((idp->perfdt == 5) && ((sdp->devtype&SZ_CDROM) == 0))
			continue;
		    /* Save address of unknown device entry, if it exists. */
		    if ((sdp->name) && (strcmp("UNKNOWN", sdp->name) == 0))
			usdp = (struct scsi_devtab *)sdp;

		    /* HACK: DEC tapes don't use vendor/product ID fields. */
		    if ((sdp->devtype & SZ_TAPE) &&
			(idp->perfdt == 1) &&
			(sc->sc_devnam[targid][0] == 0) &&
			(idp->devtq == sdp->tapetype)) {
				sdp_match++;
				break;
		    }
		    if (sdp->name) {
			if (strncmp(sc->sc_devnam[targid], sdp->name,
						    sdp->namelen) == 0) {
				sdp_match++;
				break;
			}
		    }
		}
		/*
		 * If the device name did not match call it RZxx or TZxx.
		 * Use the UNKNOWN entry from scsi_devtab (if it exists),
		 * otherwise use our internal UNKNOWN entry.
		 */
		if (!sdp_match) {
		    if (usdp)
			sdp = usdp;
		    else if (idp->perfdt == 0)
			sdp = &szp_rz_udt;
		    else if (idp->perfdt == 1)
			sdp = &szp_tz_udt;
		    else
			sdp = &szp_cz_udt;
		}
		/*
		 * Update counters and set the pointer to the completion 
		 * handler.
		 */
		if (sdp->devtype & SZ_DISK){
			szp_nrz++;
			sc->device_comp[targid] = rzcomplete;
		}
		if (sdp->devtype & SZ_TAPE){
			szp_ntz++;
			sc->device_comp[targid] = tzcomplete;
		}
		if (sdp->devtype & SZ_CDROM){
			szp_ncz++;
			sc->device_comp[targid] = rzcomplete;
		}
		if ((sdp->devtype == RX23) || (sdp->devtype == RX33)
		    			   || (sdp->devtype == RX26)) {
			szp_nrx++;
			sc->sc_mc_cnt[targid] = 1;
			sc->device_comp[targid] = rzcomplete;
		}
		/* TODO: assumes length < 8 bytes */
		bcopy(sdp->sysname, sc->sc_device[targid],
			strlen(sdp->sysname));
		sc->sc_devtab[targid] = sdp;
		sc->sc_devtyp[targid] = sdp->devtype;
		sc->sc_dstp[targid] = sdp->disksize;

		/*
		 * Act on the flags in device's scsi_devtab entry.
		 * NOTE: SCSI_STARTUNIT and SCSI_NODIAG are the only flags
		 *	 that should be set for the NCR 5380. However,
		 *	 setting any other flags should be harmless.
		 */

		/* SCSI_TRYSYNC not supported (NCR 5380 is async only) */

		if (sdp->flags & SCSI_REQSNS) {
		    sc->sc_curcmd[targid] = SZ_RQSNS;
		    sz_bldpkt(sc, targid, SZ_RQSNS, 0, 1);
		    sz_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_STARTUNIT) {
		    /*
		     * Send two start unit commands because a pending unit
		     * attention may cause the first one to fail. We don't
		     * for the drive to spin up here (happens in rzopen).
		     */
		    sc->sc_curcmd[targid] = SZ_P_SSUNIT;
		    sz_bldpkt(sc, targid, SZ_P_SSUNIT, 0, 1);
		    sz_scsistart(sc, targid, 0);
		    sz_scsistart(sc, targid, 0);
		    DELAY(sz_wait_after_inquiry);
		    /*
		     * If the bus is wedged (BSY or SEL stuck)
		     * reset the bus. This is painfull but should allow
		     * remaining targets to be seen and prevent system
		     * hang cause by a wedged SCSI bus.
		     */
		    wedged = 0;
		    if ((szaddr->scs_curstat & SCS_BSY) &&
			(szaddr->scs_curstat & SCS_BSY) &&
			(szaddr->scs_curstat & SCS_BSY))
			    wedged++;
		    if ((szaddr->scs_curstat & SCS_SEL) &&
			(szaddr->scs_curstat & SCS_SEL) &&
			(szaddr->scs_curstat & SCS_SEL))
			    wedged++;
		    if (wedged) {
			printf("szprobe: (bus=%d ID=%d) %s\n",cntlr, 
				targid, "BSY or SEL stuck after start unit");
			sz_dumpregs(cntlr, 0);	/* printf */
			szaddr->scs_inicmd = SCS_INI_RST;
			DELAY(sz_reset_width); /* hold bus reset 50 Usec */
			szaddr->scs_inicmd = 0;
			i = szaddr->scs_reset;
			DELAY(sz_wait_for_devices * 1000000);
			szaddr->scs_mode = SCS_PARCK;
		    }
		}
		if (sdp->flags & SCSI_TESTUNITREADY) {
		    sc->sc_curcmd[targid] = SZ_TUR;
		    sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
		    sz_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_READCAPACITY) {
		    sc->sc_curcmd[targid] = SZ_RDCAP;
		    sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
		    sz_scsistart(sc, targid, 0);
		}
		if (sdp->probedelay > 0)
		    DELAY(sdp->probedelay);

		if (sdp->flags & SCSI_NODIAG)
		    sz_unit_rcvdiag[targid] = 1;

		break;
	    }		/* end of switch */
	    /*
	     * Just to be sure the bus is free after inquiry.
	     * RRD40 may hold bus for a while.
	     */
	    DELAY(sz_wait_after_inquiry);
	}		/* end of for loop */
	/*
	 * Clean out any left over interrupts.
	 * NCR 5380 queues interrupts, so reset it
	 * twice just to be safe.
	 * Then re-enable interrupts.
	 */
	if (alive) {
	    i = szaddr->scs_reset;
	    i = szaddr->scs_reset;
	    sziaddr->nb_int_reqclr = (SCS_IC_BIT >> cntlr);
	    sziaddr->nb_int_msk |= (SCS_IC_BIT >> cntlr);
	}

	/*
	 * NOTE: fixed allocation of the 128KB buffer is used for
	 *	 this first release of the SCSI driver. A resource
	 *	 map to manage the 128KB buffer is needed.
	 *
	 * If last (really 2nd) call to szprobe,
	 * or only one controller configured,
	 * assign 128K data buffer slots.
	 *
	 * 128K data buffer allocation strategy (512 byte chunks):
	 *	19 - ST506 controller
	 *	32 - each tape unit
	 *	16 - each cdrom unit
	 *	18 - each scsi floppy unit
	 *	?? - each hard disk unit
	 *	 1 - guard page
	 * The guard page protects against accessing off the end of the
	 * buffer due to the 5380 prefetching 1 extra byte on DMA writes.
	 * ?? is what's left after tapes and cdroms divided by # of hard disks.
	 * Any left over pages (fragment) allocated to the first disk.
	 * ?? must be >= 8KB, should be >= 16KB, if not reduce
	 * number of cdroms to 2, then number of tapes to 2.
	 * If the 128KB buffer slot size for disks is still less
	 * than 8KB, warn the user and try to continue. The
	 * system may not function and will surely be slow.
	 * In any "real" VS3100 configuration, we should
	 * never hit these limits.
	 */
	if ((nNSCSI == 1) || (szp_firstcall == 0)) {
	    if ((vs_cfgtst & VS_SC_TYPE) == VS_SCSI_SCSI)
		dboff = 0x0;
	    else
		dboff = (19 * 512);
	    /* determine rz slot size, must be > 16kb */
	    while (1) {
		rz_slotsize = 256 - 1;		/* guard page */
		rz_slotsize -= (dboff / 512);
		rz_slotsize -= (szp_ntz * 32);
		rz_slotsize -= (szp_ncz * 16);
		rz_slotsize -= (szp_nrx * 18);
		if ((rz_slotsize > 0) && (szp_nrz - szp_nrx)) {
		    rz_fragsize = rz_slotsize % (szp_nrz - szp_nrx);
		    rz_slotsize = rz_slotsize / (szp_nrz - szp_nrx);
		}
		if (rz_slotsize < 32) {
		    if (szp_ncz > 2) {
			printf("Only 2 CDROM (too many other devices)\n");
			szp_ncz = 2;
			continue;
		    }
		}
		if (rz_slotsize < 32) {
		    if (szp_ntz > 2) {
			printf("Only 2 tz (too many other devices)\n");
			szp_ntz = 2;
			continue;
		    }
		}
		if (rz_slotsize < 32) {
		    if (rz_slotsize >= 16)
			printf("Poor disk performance (too many devices)\n");
		    else
			printf("System may not function (too many devices)\n");
		}
		break;
	    }
	    if (rz_slotsize > 128) {
		rz_slotsize = 128;
		rz_fragsize = 0;
	    }
	    if ((rz_slotsize + rz_fragsize) > 128)
		rz_fragsize = 0;
	    rz_slotsize *= 512;
	    tz_slotsize = (32 * 512);
	    cz_slotsize = (16 * 512);
	    rx_slotsize = (18 * 512);
	    ncz = ntz = 0;
	    rz_fragused = 0;
	    /* NOTE: must use ctlr NOT cntlr! */
	    for (ctlr=0; ctlr<nNSCSI; ctlr++) {
		sc = &sz_softc[ctlr];
		for (targid=0; targid<NDPS; targid++) {
		    if ((1 << targid) == sc->sc_sysid)
			continue;
		    if (sc->sc_alive[targid] == 0)
			continue;
		    if (sc->sc_devtyp[targid] & SZ_CDROM) {
			if (++ncz > szp_ncz) {
			    sc->sc_alive[targid] = 0;
			    continue;
			}
			sc->sc_dboff[targid] = dboff;
			sc->sc_segcnt[targid] = cz_slotsize;
			dboff += cz_slotsize;
		    }
		    else if (sc->sc_devtyp[targid] & SZ_TAPE) {
			if (++ntz > szp_ntz) {
			    sc->sc_alive[targid] = 0;
			    continue;
			}
			sc->sc_dboff[targid] = dboff;
			sc->sc_segcnt[targid] = tz_slotsize;
			dboff += tz_slotsize;
		    }
		    else if (sc->sc_devtyp[targid] & SZ_DISK) {
			if ((sc->sc_devtyp[targid] == RX23) ||
			    (sc->sc_devtyp[targid] == RX33)) {
				sc->sc_dboff[targid] = dboff;
				sc->sc_segcnt[targid] = rx_slotsize;
				dboff += rx_slotsize;
			}
			else if (rz_fragused || (rz_fragsize == 0)) {
			    sc->sc_dboff[targid] = dboff;
			    sc->sc_segcnt[targid] = rz_slotsize;
			    dboff += rz_slotsize;
			}
			else {
			    sc->sc_dboff[targid] = dboff;
			    sc->sc_segcnt[targid] = rz_slotsize;
			    sc->sc_segcnt[targid] += (rz_fragsize * 512);
			    dboff += (rz_slotsize + (rz_fragsize * 512));
			    rz_fragused++;
			}
		    }
		    else
			continue;
		}
	    }
	}
	/*
	 * Start timer on 2nd (last) call to szprobe.
	 */
	if (szp_firstcall) {
	    szp_firstcall = 0;
	}
	else {
	    /*
	     * Zero the diagnostice timer register.
	     * It is used by some optional driver stats code.
	     */
	    sziaddr->nb_diagtime = 0;
	    /*
	     * Start background timer (0.2 sec) to detect
	     * hung bus when driver is not active.
	     */
	    timeout(sz_timer1, 0, 20);
	}
	if (alive) {
	    sc = (struct sz_softc *)&sz_softc[cntlr];
	    sc->sc_cntlr_alive = 1;
	}
	return(alive);

}

int	sz_do_timer1 = 1;	/* if 0, don't run driver idle bus hung timer */
int	sz_t1_dorst = 1;	/* if 0, don't reset bus in timer1 */


/*
 *
 * Name:		sz_timer2	-timeout disconnected disks
 *
 * Abstract:		A 10/30 second timeout starts when a hard/floppy
 *			disk disconnects from the bus. This routine is
 *			called if the disconnected disk has not done a
 *			reselect after 10/30 seconds. This routine sets an
 *			error condition on the disk and calls sz_start.
 *			NOTE: floppy timeout is 30 seconds because retires
 *			      can take a long time.
 *			NOTE: we cannot access the 5380's registers
 *			      the bus is active, so we backoff .1 sec.
 *			NOTE: just return if bus reset in progress.
 *
 * Inputs:
 *
 * unit			The ULTRIX logical unit number of the disk. This
 *			will be 0 -7 for bus A or 8 - 15 for bus B.
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 * Side Effects:
 *			IPL raised to 15.
 *			Another timeout queued if the bus is busy.
 *			Disk's context in sz_softc is modified.
 *			sz_start() called.
 *
 */

sz_timer2(unit)
int unit;
{
	register struct sz_softc *sc;
	register int cntlr, targid;
	register int s;
	int flags;

	s = spl5();
	cntlr = (unit >> 3) & 1;
	sc = &sz_softc[cntlr];
	targid = unit & 7;

	if (sc->sc_rip) {	/* Bus reset in progress, ignore timers */
	    splx(s);
	    return;
	}

	if (sc->sc_active) {
	    /* TODO: may want variable backoff delay */
	    timeout(sz_timer2, (caddr_t)unit, 10);	/* .1 sec */
	    splx(s);
	    return;
	}

	/* This does not happen unless something really went wrong */
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGSELST;
	scsi_logerr(sc, 0, targid, SZ_ET_DISTIMO, 0, 0, flags);
	sc->sc_szflags[targid] &= ~SZ_TIMERON;
	/* Do not think this can happen */
	if (sc->sc_selstat[targid] != SZ_DISCONN) {
#ifdef	notdef
	    mprintf("sz_timer2: (bus=%d ID=%d) target not disconnected\n",
		cntlr, targid);
#endif	notdef
	    splx(s);
	    return;
	}
	sc->sc_selstat[targid] = SZ_IDLE;
	sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	sc->sc_active = 0;
	sc->sc_fstate = 0;
	sc->sc_szflags[targid] |= SZ_NEED_SENSE;
	/* force retry even if RQSNS data says NOSENSE */
	sc->sc_szflags[targid] |= SZ_RSTIMEOUT;
	if (sc->sc_dkn[targid] >= 0)
	    dk_busy &= ~(1 << sc->sc_dkn[targid]);
	sz_start(sc, targid);
	splx(s);
}



/*
 *
 * Name:		sz_timer4	-timeout busy target
 *
 * Abstract:		If a target cannot accept a command it returns
 *			a BUSY status. In this case a timer is started.
 *			This routine is called when the timer expires.
 *			This routine calls sz_start() to send the command
 *			again. Timeout length is 0.5 seconds.
 *			NOTE: we cannot access the 5380's registers
 *			      the bus is active, so we backoff .1 sec.
 *			NOTE: just return if bus reset in progress.
 *
 * Inputs:
 *
 * unit			The ULTRIX logical unit number of the disk. This
 *			will be 0 -7 for bus A or 8 - 15 for bus B.
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 * Side Effects:
 *			IPL raised to 15.
 *			Another timeout queued if the bus is busy.
 *			sz_start() called.
 *
 */

sz_timer4(unit)
int unit;
{
	register struct sz_softc *sc;
	register int cntlr, targid;
	register int s;

	s = spl5();
	cntlr = (unit >> 3) & 1;
	sc = &sz_softc[cntlr];
	targid = unit & 7;

	if (sc->sc_rip) {	/* Bus reset in progress, ignore timers */
	    splx(s);
	    return;
	}

	if (sc->sc_active) {
	    /* TODO: may want variable backoff delay */
	    timeout(sz_timer4, (caddr_t)unit, 10);	/* .1 sec */
	    splx(s);
	    return;
	}

	/* Do not think this can happen */
	if ((sc->sc_szflags[targid] & SZ_BUSYTARG) == 0) {
#ifdef	notdef
	    mprintf("sz_timer4: (bus=%d ID=%d) target not busy\n",
		cntlr, targid);
#endif	notdef
	    splx(s);
	    return;
	}
	sc->sc_szflags[targid] &= ~SZ_BUSYTARG;
	sz_start(sc, targid);
	splx(s);
}


/*
 *
 * Name:		sz_timer3	-selection timeout timer
 *
 * Abstract:		This routine is part of the 250 MS selection
 *			timeout procedure. The first 20 MS is a spin
 *			loop in sz_scsistart(). If the target does not
 *			respond to the selection after 20 MS, then a
 *			30 MS timer is started. After 30 MS sz_timer3
 *			is called, which starts a 50 MS timer 4 times.
 *			On each call sz_timer3 checks for BSY asserted.
 *			If BSY false and time less than 250 MS, then
 *			queue another 50 MS timeout. If BSY true or
 *			time greater than 250 MS, then call sz_start.
 *			NOTE: code assumes a bus reset cannot happen
 *			      while we are timing out a select.
 *
 * Inputs:
 *
 * unit			The ULTRIX logical unit number the target.
 *			Will be 0 - 7 for bus A or 8 - 15 for bus B.
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 * Side Effects:
 *			IPL raised to 15.
 *			sz_start() called.
 *			Another timer may be queued.
 *
 */

sz_timer3(unit)
int unit;
{
	register struct sz_regs *szaddr;
	register struct sz_softc *sc;
	register int cntlr, targid;
	register int s;

	s = spl5();
	cntlr = (unit >> 3) & 1;
	sc = &sz_softc[cntlr];
	targid = unit & 7;
	szaddr = (struct sz_regs *)szmem + cntlr;

	/* Do not think this can ever happen */
	if (sc->sc_active == 0) {
	    splx(s);
	    return;
	}

	/*
	 * Keep the select timer running until 250 MS
	 * timeout expires or the target asserts BSY.
	 */
	if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
	    ((szaddr->scs_curstat & SCS_BSY) == 0) &&
	    ((szaddr->scs_curstat & SCS_BSY) == 0)) {
	    if (++sc->sc_swcount < 5) {
		timeout(sz_timer3, (caddr_t)unit, 5);	/* 50 MS */
		splx(s);
		return;
	    }
	}

	/*
	 * Call the sz_start routine, which will call
	 * sz_scsistart() to handle the select timeout
	 * or complete the select, depending on whether
	 * or not the target asserted BSY within 250 MS.
	 */
	sz_start(sc, targid);
	splx(s);
}


/*
 *
 * Name:		sz_timer1	-hung bus detection timer
 *
 * Abstract:		This timeout occurs every 0.2 seconds. It is
 *			strated in the probe routine. If the driver
 *			thinks the bus is idle (sc_active == 0) and
 *			the bus is hung, then sz_timer1 resets the bus
 *			and all software state information. sz_start
 *			is called to get things going again. We declare
 *			the bus hung if BSY is true and SEL is false
 *			for 1 MS.
 *			NOTE: have not tested the case where both
 *			      busses hang at the same time.
 *			NOTE: cannot access 5380 registers if bus active.
 *			NOTE: just return if bus reset in progress.
 *
 * Inputs:		None.
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 * Side Effects:
 *			IPL raised to 15.
 *			The SCSI bus may be reset.
 *			sz_start() called.
 *			Another .2 second timeout will be queued.
 *
 */

#ifdef	ELDEBUG
/* set to cntlr # to cause error */
int	sz_eldb_rstbus0 = -1;
/* set to 0 to cause error, MUST also force an abort in szintr */
int	sz_eldb_rstbus1 = -1;
/* set to 0 to cause error, MUST also force an abort in scsistart */
int	sz_eldb_rstbus2 = -1;
/* set to cntlr # to cause error */
int	sz_eldb_busrst = -1;
/* set to cntrl # to cause a parity error */
int	sz_eldb_parity = -1;

/* Set to ID of target for command abort (on bus 0 only) */
int	sz_eldb_cmdabrtd0 = -1;
int	sz_eldb_cmdabrtd3 = -1;
#endif	ELDEBUG

sz_timer1(unused)
int unused;
{
	register struct sz_softc *sc;
	register struct sz_regs *szaddr;
	int i, s;
	int cntlr;
	int wedged;
	int flags;
#ifdef	ELDEBUG
	int inicmd_tmp;
#endif	ELDEBUG

	while (1) {
	    s = spl5();
	    if (sz_do_timer1 == 0)
		break;

	    for (cntlr = 0; cntlr < nNSCSI; cntlr++) {
		sc = &sz_softc[cntlr];
		if (sc->sc_cntlr_alive == 0)	/* cntlr does not exist */
		    continue;
		if (sc->sc_rip)			/* bus reset in progress */
		    continue;
		if (sc->sc_active)		/* bus active, not safe to */
		    continue;			/* access 5380's registers */

		/*
		 * We assume bus hung if BSY stuck for 1 MS.
		 */
		szaddr = (struct sz_regs *)szmem + cntlr;
#ifdef	ELDEBUG
		if (cntlr == sz_eldb_busrst) {
		    /* NOTE: spl5 - no need to mask interrupts */
		    szaddr->scs_inicmd = SCS_INI_RST;
		    DELAY(25);
		    szaddr->scs_inicmd = 0;
		    sz_eldb_busrst = -1;
		}

		if (cntlr == sz_eldb_parity) {
		    /* NOTE: read undriven data bus to cause parity error */
		    i = szaddr->scs_curdata;
		    sz_eldb_parity = -1;
		}

#endif	ELDEBUG
		wedged = 1;
		for (i = 0; i < 1000; i++) {
		    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
			((szaddr->scs_curstat & SCS_BSY) == 0) &&
			((szaddr->scs_curstat & SCS_BSY) == 0)) {
			    wedged = 0;
			    break;
		    }
		    DELAY(1);
		}
#ifdef	ELDEBUG
		if (cntlr == sz_eldb_rstbus0) {
		    wedged = 1;
		    sz_eldb_rstbus0 = -1;
		}
#endif	ELDEBUG
		if (wedged == 0)
		    continue;
		/*
		 * BSY stuck, we assume bus is hung.
		 * Reset the bus and driver state.
		 * Restart I/O operations.
		 */
		flags = SZ_HARDERR | SZ_LOGREGS;
		scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 0, 0, flags);
		if (sz_t1_dorst == 0) {		/* TODO: debug */
		    splx(s);
		    return;	/* kill timer, so we don't loop! */
		}
		sz_reset(cntlr);
	    }
	    break;
	}
	/* queue another timer and return */
	timeout(sz_timer1, 0, 20);	/* 0.2 sec */
	splx(s);
	return;
}

/*
 * If set to one, move the zeroing of the mode
 * register to before the spin on phase loop in szintr().
 * The idea is to match Martin's note.
 * Never proved this made any difference one way or
 * the other. Leave as is because all testing done
 * with the code this way (sz_i_moveit = 1).
 */

int	sz_i_moveit = 1;


/*
 * Controls whether on not we timeout
 * disconnected disks.
 * Leave this control on (sz_do_rsto = 1).
 */
int	sz_do_rsto = 1;

/*
 * Count the number of Save Data Pointers,
 * Restore Data Pointers messages, and Implied
 * Save Data Pointers (disconnect not preceded by
 * a save data pointer message).
 * NOTE: this is debug information that came in handy
 *	 so many times I decided to keep it around.
 */
extern	int	sz_sdp[];
extern	int	sz_rdp[];
extern	int	sz_isdp[];

/*
 * These variables control the new code which
 * uses a phase mismatch interrupt instead of spinning
 * waiting for the phase to change (in critical cases).
 * To use the interrupt, set 1 & 3, but not 2.
 * TODO: this code needs more work.
 */
int	sz_no_spin1 = 1;
int	sz_no_spin2 = 0;
int	sz_no_spin3 = 1;

#ifdef	SZ_DSP_DEBUG
int	sz_nsp_print = 0;	/* print, if command didn't do status phase */
#endif	SZ_DSP_DEBUG

int	sz_do_reset = 1;	/* TODO: debug (cntl reset hung bus) */

int	wakeup();
extern int hz;

/*
 * Controls whether we reset the bus or
 * just ignore the reselect attempt,
 * when multiple IDs are detected.
 * Set to one to reset the bus.
 * TODO: not usre which is correct.
 */
int	sz_i_midrst = 0;

/*
 * DEBUG - normally off
 *
 * If set, causes stray interrupt and reselect timeout
 * messages to console (they always go to the error log).
 */

/*
 * If set, do an implied save data pointer
 * if the target disconnects without SDP.
 */
int	sz_do_isdp = 1;


/*
 *
 * Name:		szintr		-Interrupt service routine
 *
 * Abstract:		This routine fields interrupts from the SCSI bus
 *			controllers (A & B) and performs the appropriate
 *			actions. General flow is as follows:
 *
 *			If the bus is active (target selected):
 *
 *			  1. Find ID and bring target into context.
 *			  2. Check for disconnect (loss of BSY).
 *			  3. Follow phases set by target.
 *
 *			If the bus is not active (no target selected):
 *			  1. If its not a reselect - log stray interrupt.
 *			  2. If reselect, bring target into context and
 *			     follow phases set by target.
 *
 * Inputs:
 *
 * IPL_device		Device IPL is 14 (IPL 16 in critical code path).
 * cntlr		SCSI bus controller number (0 = bus A, 1 = bus B).
 *
 * Interrupts		A target reselects the initiator.
 *			EOP - DMA count reaches zero (NOT USED).
 *			Bus Parity Error (NOT USED).
 *			Phase Mismatch (instead of EOP).
 *			A target disconnected from the bus.
 *			SCSI bus reset (NOT LATCHED!).
 *
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 *
 * Side Effects:
 *
 *			Target's context in sz_softc is updated.
 *			DMA transfers may be initiated or continued.
 *			The SCSI bus can be reset (if hung).
 *			Next command started by calling sz_start().
 *			Error messages may be logged to the console
 *			and/or the error log.
 *
 */

/*
 * BSY timing loop count (see SZ_DISCON: in SZ_MESSI:).
 * Don't change, 500 is a good balance for both
 * 90 and 60 NS CPUs.
 * NOTE: to disable BSY timing, set sz_i_btcnt = 1.
 */
int	sz_i_btcnt = 500;

/*#define SZ_BT_DEBUG*/
#ifdef	SZ_BT_DEBUG
int	sz_i_bt[64];	/* 64 units - allows for 8 5380 cntlrs */
#endif	SZ_BT_DEBUG

#ifdef	ELDEBUG
/* Set ID of target to cause reselect error 0 (on bus 0 only) */
int	sz_eldb_reselerr0 = -1;

/* Set to cntlr # to cause error */
int	sz_eldb_stryintr0 = -1;
int	sz_eldb_stryintr1 = -1;

/* Set ID of target to cause activity status error (on bus 0 only) */
int	sz_eldb_actstat = -1;

/* Set ID of target to cause DBBR message to be logged (on bus 0 only) */
int	sz_eldb_dbbr0 = -1;
int	sz_eldb_dbbr1 = -1;
int	sz_eldb_dbbr2 = -1;
int	sz_eldb_dbbr3 = -1;

/* Set ID of target to cause REQ failed to set (on bus 0 only) */
int	sz_eldb_buserra0 = -1;
/* Set ID of target to cause REQ failed to clear (on bus 0 only) */
int	sz_eldb_buserr31 = -1;

/* Set ID of target to cause rcv < 5 data bytes (on bus 0 only) */
int	sz_eldb_buserr12 = -1;

/* Set ID of target to cause unknown message (on bus 0 only) */
int	sz_eldb_buserr73 = -1;
int	sz_eldb_buserrf3 = -1;

/* Set ID of target to cause BSY hung on CMDCPT (on bus 0 only) */
int	sz_eldb_buserr74 = -1;

/* Set ID of target to cause BSY not set on phase change (on bus 0 only) */
int	sz_eldb_buserr4e = -1;
/* Set ID of target to cause REQ not set on phase change (on bus 0 only) */
int	sz_eldb_buserr4f = -1;
#endif	ELDEBUG

szintr(cntlr)
int cntlr;
{
    int unit, targid;
    int curstat, save_selstat;
    int tid, scsi_id, nbits;
    register struct sz_softc *sc;
    register struct buf *dp;
    register struct buf *bp;
    register struct sz_regs *szaddr = (struct sz_regs *)szmem + cntlr;
    register struct nb_regs *sziaddr = (struct nb_regs *)nexus;
    register int reg_cnt;	/* MUST be a register for timing */
    char *stv;			/* virtual address of st page tables	      */
    struct pte *pte, *mpte;	/* used for mapping page table entries	      */
    char *bufp;
    unsigned v;
    int o, npf;
    struct proc *rp;
    u_char *byteptr;
    int i, s, retval, count;
    int cmdcnt;
    int complete;
    int	num_expected, num_received;
    u_char cmd_type;
    int exmsg_byte, exmsg_len, exmsg_cnt, exmsg_sdtr, exmsg_data;
    int ftt;
    int wedged, force_reset;
    int flags, subtyp;
    struct format_params *fp;
    struct reassign_params *rbp;
    struct read_defect_params *rdp;
    struct defect_descriptors *dd;
    struct mode_sel_sns_params *msp;
    struct io_uxfer *iox;
    int datacnt;

    force_reset = 0;

    sc = &sz_softc[cntlr];

#ifdef	ELDEBUG
    if (cntlr == sz_eldb_stryintr0) {
	flags = SZ_HARDERR | SZ_LOGREGS;
	scsi_logerr(sc, 0, -1, SZ_ET_STRYINTR, 0, 0, flags); /* ELDEBUG */
	sz_eldb_stryintr0 = -1;
    }

    if (cntlr == sz_eldb_stryintr1) {
	flags = SZ_HARDERR | SZ_LOGREGS | SZ_LOGSELST;
	scsi_logerr(sc, 0, -1, SZ_ET_STRYINTR, 1, 0, flags); /* ELDEBUG */
	sz_eldb_stryintr1 = -1;
    }
#endif	ELDEBUG

    /*
     * Ingore interrupts if bus reset in progress.
     * No interrupts should occur during reset wait period.
     */
    if (sc->sc_rip) {
	i = szaddr->scs_reset;
	return;
    }

    ftt = 1;
    targid = -1;	/* So sz_start() won't be called in abort: below */

    /*
     *	The RST bit in the scs_curstat register is not latched!
     *	So, we use the PARCK bit in the mode register to detect
     *	a bus reset. We set PARCK and never clear it, so if it
     *	gets cleared we assume a bus reset did it.
     *	If we detect a bus reset, we abort the current operation,
     *	then force another bus reset. This makes sure we get back
     *	to a known state if the reset was a glitch (not every
     *	device detected the reset).
     */
    if ((szaddr->scs_curstat & SCS_RST) ||
	((szaddr->scs_mode & SCS_PARCK) == 0)) {
	scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 0, 0, SZ_HARDERR);
	force_reset = 1;
    }


    /*
     * Use SCSI target ID to establish needed context.
     * If channel active use sc_active.
     * If not, read SCSI data bus during reselect.
     */
    if (sc->sc_active) {
	for (targid = 0; targid < NDPS; targid++)
	    if (sc->sc_active & (1 << targid))
		break;
	if (szaddr->scs_status & SCS_PARERR) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 0, 0, flags);
	    i = szaddr->scs_reset;
	    szaddr->scs_mode = SCS_PARCK;
	    goto abort;
	}
	unit = sc->sc_unit[targid];
	dp = &szutab[unit];
	bp = dp->b_actf;
	if ((dp->b_active == 0) || (bp == 0)) {
	    scsi_logerr(sc, 0, targid, SZ_ET_ACTSTAT, 0, 0, SZ_HARDERR);
	    i = szaddr->scs_reset;
	    if (force_reset)
		goto abort;
	    else
		return;
	}
	/*
	 * TODO:
	 *	Debug code. Can't remember ever hitting this one.
	 */
	if ((sc->sc_selstat[targid] != SZ_SELECT) &&
	    (sc->sc_selstat[targid] != SZ_RESELECT)) {
	    flags = SZ_HARDERR | SZ_LOGSELST;
	    scsi_logerr(sc, 0, targid, SZ_ET_ACTSTAT, 1, 0, flags);
	    i = szaddr->scs_reset;
	    goto abort;
	}
#ifdef	ELDEBUG
	if ((cntlr == 0) && (targid == sz_eldb_actstat)) {
	    flags = SZ_HARDERR | SZ_LOGSELST;
	    scsi_logerr(sc, 0, targid, SZ_ET_ACTSTAT, 1, 0, flags);/* ELDEBUG */
	    sz_eldb_actstat = -1;
	}
#endif	ELDEBUG
	save_selstat = sc->sc_selstat[targid];
	/* if only DMA set, was phase mismatch intr */
	if (sz_no_spin1) {
	    if (sz_no_spin3)
		szaddr->scs_mode &= ~SCS_DMA;
	    else
		if ((szaddr->scs_mode & (SCS_DMA|SCS_INTEOP)) == SCS_DMA)
		    szaddr->scs_mode &= ~SCS_DMA;
	}
    }
    if (force_reset)	/* bus reset detected, but we were not active */
	goto abort;


    /*
     * If BSYERR set, target has disconnected. Say we are no
     * longer active. Start disconnect timer if target is a
     * disk. Call sz_start() to start next I/O, if possible.
     * Could also be a busy target (see below).
     */
    if((szaddr->scs_status & SCS_BSYERR) == SCS_BSYERR) {
	szaddr->scs_mode = SCS_PARCK;		/* clears MONBSY */
	i = szaddr->scs_reset;
	if (sc->sc_szflags[targid] & SZ_BUSYTARG) {
	    /*
	     * Not a disconnect. Target returned a busy status and
	     * cannot accept a command right now. So we queue a
	     * timeout to resend the command later.
	     */
	    sc->sc_selstat[targid] = SZ_IDLE;
	    sc->sc_fstate = 0;
/*	    szaddr->scs_mode = SCS_PARCK;	*/
	    szaddr->scs_tarcmd = 0;	/* make sure phases match for 5380 */
	    szaddr->scs_outdata = 0;
	    sc->sc_active = 0;		/*  channel no longer active */
	    if (sc->sc_dkn[targid] >= 0)
		dk_busy &= ~(1 << sc->sc_dkn[targid]);
	    /* TODO: time should be device specific! */
	    timeout(sz_timer4, (caddr_t)unit, 50);	/* 0.5 sec */
	}
	else {
sz_i_discon:

#ifdef	SZDEBUG
	    printd2("szintr: disconnect\n");
#endif	SZDEBUG
#ifdef	DCT_STATS
	    sc->sc_dcstart[targid] = sziaddr->nb_diagtime;
#endif	DCT_STATS
	    sc->sc_selstat[targid] = SZ_DISCONN;
/*	    szaddr->scs_mode = SCS_PARCK;	*/
	    szaddr->scs_tarcmd = 0;	/* make sure phases match for 5380 */
	    szaddr->scs_outdata = 0;
	    sc->sc_active = 0;		/*  channel no longer active */
	    if (sz_do_rsto) {
		/*
		 * TODO: needs more work.
		 * Ok for RZ22, RZ23, and RZ55. Who knows for RZxx?
		 * Use longer timeout for floppy disk.
		 * Do this at all? Use command timeout instead?
		 * No timeout on format, reassign, vfy_data, and rdd commands.
		 * Only timeout read, write, read capacity commands.
		 */
		if ((sc->sc_devtyp[targid] & SZ_DISK) &&
				!sc->sc_rzspecial[targid]) {
		    switch (sc->sc_curcmd[targid]) {
		    case SZ_READ:
		    case SZ_WRITE:
		    case SZ_READ_10:
		    case SZ_WRITE_10:
		    case SZ_RDCAP:
			/* TODO: what about non-DEC floppy? */
			if ((sc->sc_devtyp[targid] == RX23) ||
			    (sc->sc_devtyp[targid] == RX33)) {
			    timeout(sz_timer2, (caddr_t)unit, hz*30);
			}
			else
			    timeout(sz_timer2, (caddr_t)unit, hz*10);
			sc->sc_szflags[targid] |= SZ_TIMERON;
			break;
		    default:
			break;
		   }
		}
	    }
	}

	/*
	 * Select enable should already be on, BUT...
	 */
	szaddr->scs_selena = sc->sc_sysid;
	sc->sc_scs_selena = sc->sc_sysid;
	DELAY(10);

	/*
	 * The bus can be busy again by the time we get here
	 * because of the delay between the time BSYERR set
	 * and we service the loss of BSY interrupt.
	 * If SEL is true, then a device (can even be the one
	 * that just disconnected) has grabbed the bus. So,
	 * we don't call sz_start() to start the next I/O, since
	 * the bus would be busy and arbitration would fail.
	 * As soon as the IPL drops we will get a reselect interrupt.
	 */
	if (szaddr->scs_status & SCS_INTREQ) {
	    if (szaddr->scs_curstat & SCS_SEL)
		return;
	}
	sz_start(sc, -1);	/* Start next I/O (if one is ready) */
	return;
    }

    /*
     * If we are not active, check for reselect.
     */
    if(sc->sc_active == 0) {
	/* NOTE: make sure BSY false for bus settle delay (400 ns) */
	/*
	 * There is critical timing in the reselect code path.
	 * Do not remove the spl6(). Even the clock must not
	 * be allowed to interrupt this code. The result could
	 * be multiple targets selected on the SCSI bus (very bad).
	 */
	s = spl6();
	curstat = (szaddr->scs_curstat & SCS_BSY);
	curstat |= (szaddr->scs_curstat & SCS_BSY);
	curstat |= szaddr->scs_curstat;
	if ((curstat & (SCS_BSY|SCS_SEL|SCS_IO)) == (SCS_SEL|SCS_IO)) {
	    i = szaddr->scs_mode;
	    szaddr->scs_mode |= SCS_PARCK;	/* be sure parity checking on */
#ifdef	SZDEBUG
	    printd2("szintr: reselect\n");
#endif	SZDEBUG
	    /*
	     * Read bus ID bits from current data register.
	     * Make sure only two ID bits on the bus
	     * and one of them is the initiator's.
	     * Don't respond to the reselect if there is a parity error.
	     * Only log the parity error if ID bits on the data bus.
	     * No IDs on the bus means its not being driven, i.e.,
	     * the target droped the reselect after the 5380 set INTREQ.
	     */
	    scsi_id = szaddr->scs_curdata;
	    szaddr->scs_mode = i;	/* restore mode regsiter */
	    if (szaddr->scs_status & SCS_PARERR) {
		if (scsi_id != 0) {
		    flags = SZ_HARDERR | SZ_LOGREGS | SZ_LOGBUS;
		    scsi_logerr(sc, 0, -1, SZ_ET_PARITY, 1, scsi_id, flags);
		}
		i = szaddr->scs_reset;
		splx(s);
		return;
	    }
	    nbits = 0;
	    for (tid = 0; tid < NDPS; tid++)
		if (scsi_id & (1 << tid))
		    nbits++;
#ifdef	ELDEBUG
	    for (tid = 0; tid < NDPS; tid++) {
		if ((1 << tid) == sc->sc_sysid)
		    continue;	/* skip initiator */
		if (scsi_id & (1 << tid))
		    break;
	    }
	    if ((cntlr == 0) && (tid == sz_eldb_reselerr0)) {
		nbits = 3;
		sz_eldb_reselerr0 = -1;
	    }
#endif	ELDEBUG
	    if ((nbits != 2) || ((scsi_id & sc->sc_sysid) == 0)) {
		flags = SZ_HARDERR | SZ_LOGBUS | SZ_LOGREGS;
		scsi_logerr(sc, 0, -1, SZ_ET_RESELERR, 0, scsi_id, flags);
		i = szaddr->scs_reset;
		splx(s);
		/* TODO: which way is correct? */
		if (sz_i_midrst) {
		    force_reset = 1;
		    goto abort;
		}
		else
		    return;
	    }
	    /*
	     * Must assert BSY within 200 Usec after we detect
	     * reselect, so we set BSY as soon as we can.
	     */
	    szaddr->scs_inicmd = SCS_INI_BSY;
	    for (tid = 0; tid < NDPS; tid++) {
		if ((1 << tid) == sc->sc_sysid)
		    continue;	/* skip initiator */
		if (scsi_id & (1 << tid))
		    break;
	    }
	    sc->sc_active = (scsi_id & ~sc->sc_sysid);
	    splx(s);		/* end of critical timing sequence */
	    targid = tid;
	    save_selstat = sc->sc_selstat[targid];
	    if (sc->sc_selstat[targid] != SZ_DISCONN) {
		flags = SZ_HARDERR | SZ_LOGSELST;
		scsi_logerr(sc, 0, targid, SZ_ET_RESELERR, 1, 0, flags);
		i = szaddr->scs_reset;
		szaddr->scs_inicmd = 0;	/* clear BSY */
		sc->sc_selstat[targid] = save_selstat;
		/* TODO: return or abort? */
		/* TODO: this error is very bad news! what to do? */
		return;
	    }
	    SZWAIT_WHILE(((szaddr->scs_curstat & SCS_SEL) == SCS_SEL),10000,retval);
	    if(retval >= 10000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_RESELERR, 2, 0, flags);
		/* TODO: temp read reset reg so don't loose interrupts */
		i = szaddr->scs_reset;
		szaddr->scs_inicmd = 0;		/* clear BSY */
		sc->sc_active = 0;
		sc->sc_selstat[targid] = save_selstat;
		goto abort;
	    }
	    szaddr->scs_inicmd = 0;		/* clear BSY */
	    /* NOTE: we are looking for BSY from the target */
	    DELAY(1);
	    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0)) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_RESELERR, 3, 0, flags);
		i = szaddr->scs_reset;
		sc->sc_active = 0;
		sc->sc_selstat[targid] = save_selstat;
		return;
	    }
	    DELAY(200);
	    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0)) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_RESELERR, 4, 0, flags);
		i = szaddr->scs_reset;
		sc->sc_active = 0;
		sc->sc_selstat[targid] = save_selstat;
		return;
	    }
	    /* NOTE: must turn scs_selena back on for any subsequent error */
	    sc->sc_selstat[targid] = SZ_RESELECT;
	    unit = sc->sc_unit[targid];
	    dp = &szutab[unit];
	    bp = dp->b_actf;
	    if ((dp->b_active == 0) || (bp == 0)) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_RESELERR, 5, 0, flags);
		i = szaddr->scs_reset;
		sc->sc_active = 0;
		szaddr->scs_selena = sc->sc_sysid;
		sc->sc_scs_selena = sc->sc_sysid;
		sc->sc_selstat[targid] = save_selstat;
		return;
	    }
#ifdef	DCT_STATS
	    sc->sc_dcend[targid] = sziaddr->nb_diagtime;
	    /* diagtime counter wraps after 16 bits */
	    if (sc->sc_dcend[targid] < sc->sc_dcstart[targid])
		sc->sc_dcend[targid] += 65536;
	    sc->sc_dcdiff[targid] = sc->sc_dcend[targid]-sc->sc_dcstart[targid];
	    if (sc->sc_dcdiff[targid] > sc->sc_dclongest[targid])
		sc->sc_dclongest[targid] = sc->sc_dcdiff[targid];
#endif	DCT_STATS
	}
	else {
	    splx(s);	/* cuz we set spl6() in above if statement */
	    /*
	     * We get here when an interrupt occurs and we
	     * are not active and the bus is free. We
	     * assume this is a reselect timeout interrupt.
	     * If no disconnected targets, then its a stray interrupt.
	     * We reset the controller and ignore the interrupt.
	     * We depend on the disconnect timeout (sz_timer2)
	     * to kick the target and get it going again.
	     * First, check for parity error.
	     */
	    if (szaddr->scs_status & SCS_PARERR) {
		flags = SZ_HARDERR | SZ_LOGREGS;
		scsi_logerr(sc, 0, -1, SZ_ET_PARITY, 2, 0, flags);
		i = szaddr->scs_reset;
		return;
	    }
	    count = 0;
	    for (i = 0; i < NDPS; i++)
		count += sc->sc_selstat[i];
	    if (count == 0) {
		flags = SZ_HARDERR | SZ_LOGREGS;
		scsi_logerr(sc, 0, -1, SZ_ET_STRYINTR, 0, 0, flags);
	    }
	    else {
		flags = SZ_HARDERR | SZ_LOGREGS | SZ_LOGSELST;
		scsi_logerr(sc, 0, -1, SZ_ET_STRYINTR, 1, 0, flags);
	    }
	    i = szaddr->scs_reset;
	    return;
	}
    }

    /*
     * THIS CANNOT HAPPEN, unless the hardware
     * goes insane (like the first SCSI/SCSI controller).
     */
    if (sc->sc_active == 0) {
	scsi_logerr(sc, 0, -1, SZ_ET_ACTSTAT, 2, 0, SZ_HARDERR);
	if (szaddr->scs_status & SCS_INTREQ)
	    i = szaddr->scs_reset;
	    return;
    }

#ifdef	ELDEBUG
    if ((cntlr == 0) && (targid != -1) && (targid == sz_eldb_cmdabrtd0)) {
	sz_eldb_cmdabrtd0 = -1;
	goto abort;
    }
#endif	ELDEBUG

    /*
     * The target controls the bus phase. We wait for REQ and BSY
     * to be asserted on the bus before determining which phase
     * is active.  Once REQ and BSY are asserted, the appropiate action
     * is taken. 
     */

    /* TODO: moved from phase spin loop! */
    if (sz_i_moveit) {
	szaddr->scs_mode = SCS_PARCK;
	/*
	 * Turn off the SCS_INI_ENOUT driver
	 */
	inicmd_tmp &= ~SCS_INI_ENOUT;
	szaddr->scs_inicmd = inicmd_tmp;
    }
    /* TODO: end of moved code! */

    exmsg_byte = 0;
    exmsg_sdtr = 0;
    complete = 0;
    do {
	if (sz_no_spin2) {
	    if ((ftt == 0) && (szaddr->scs_status & SCS_MATCH)) {
		szaddr->scs_mode |= SCS_DMA;
		return;
	    }
	    ftt = 0;
	}
	for (retval = 0; retval < 100000; retval++) {
	    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0)) {
		retval = 1000000;
		break;
	    }
	    if (szaddr->scs_curstat & SCS_REQ)
		break;
	    DELAY(100);
	}
#ifdef	SPIN_STATS
	if ((retval > sc->sc_i_spin1[targid]) && (retval < 100000)) {
	    sc->sc_i_spin1[targid] = retval;
	    sc->sc_i_spcmd[targid] = sc->sc_curcmd[targid];
	    sc->sc_i_phase[targid] = ((szaddr->scs_curstat & SCS_PHA_MSK) >> 2);
	}
#endif	SPIN_STATS
#ifdef	ELDEBUG
	if ((cntlr == 0) && (targid == sz_eldb_buserr4e)) {
	    retval = 1000000;
	    sz_eldb_buserr4e = -1;
	}

	if ((cntlr == 0) && (targid == sz_eldb_buserr4f)) {
	    retval = 100000;
	    sz_eldb_buserr4f = -1;
	}
#endif	ELDEBUG
	if (retval >= 100000) {
	    if (retval == 1000000)
		subtyp = 0x4e;	/* BSY dropped */
	    else
		subtyp = 0x4f;	/* REQ failed to set */
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, subtyp, 0, flags);
	    /* TODO: temp read reset reg so don't loose interrupts */
	    /* TODO: caution, in a loop! */
	    i = szaddr->scs_reset;
	    sc->sc_active = 0;
	    szaddr->scs_selena = sc->sc_sysid;
	    sc->sc_scs_selena = sc->sc_sysid;
	    /*
	     * This code is in a loop. How we handle this error
	     * depends on whether or not its the first time thru.
	     * If the target is in the select state we must abort.
	     * If its in the reselect state we drop it all
	     * on the floor and hope the target tries again.
	     * We set the state back to what it was before the reselect.
	     */
	    if (sc->sc_selstat[targid] == SZ_SELECT)
		goto abort;
	    sc->sc_selstat[targid] = save_selstat;
	    return;
	}
	if (sc->sc_szflags[targid] & SZ_TIMERON) {
	    untimeout(sz_timer2, (caddr_t)unit);
	    sc->sc_szflags[targid] &= ~SZ_TIMERON;
	}
	/* TODO: may want to check RST/PARERR sooner in loop! */
	if ((szaddr->scs_curstat & SCS_RST) ||
	    ((szaddr->scs_mode & SCS_PARCK) == 0)) {
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 1, 0, SZ_HARDERR);
	    force_reset = 1;
	}
	if (szaddr->scs_status & SCS_PARERR) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 3, 0, flags);
	    /* TODO: temp read reset reg so don't loose interrupts */
	    i = szaddr->scs_reset;
	    goto abort;
	}
	if (force_reset)
	    goto abort;
	if (szaddr->scs_status & SCS_INTREQ) {
	    i = szaddr->scs_reset;		/* This reset is needed! */
	}
	/*
	 * Zero the scs_mode register to clear all 
	 * DMA status bits from the last DMA transfer
	 */
if (sz_i_moveit == 0) {
	szaddr->scs_mode = SCS_PARCK;
	/*
	 * Turn off the SCS_INI_ENOUT driver
	 */
	inicmd_tmp &= ~SCS_INI_ENOUT;
	szaddr->scs_inicmd = inicmd_tmp;
}
	/*
	 * Read the bus phase, set the phase to match.
	 */
	szaddr->scs_tarcmd = ((szaddr->scs_curstat & SCS_PHA_MSK) >> 2);
	switch (szaddr->scs_curstat & SCS_PHA_MSK) {

	case SCS_DATAO:
#ifdef SZDEBUG
	    printd2("szintr: SCS_DATAO:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_DATAO_PHA;
	    if ( (sc->sz_opcode != SZ_WRITE) &&
		 (sc->sz_opcode != SZ_WRITE_10) ) {
		byteptr = (u_char *)&sc->sc_dat[0];
		switch(sc->sz_opcode) {
		case SZ_MODSEL:
		    cmdcnt = sc->sz_modsel.pll;
	    	    if(sc->sc_rzspecial[targid]) {
	        	msp = (struct mode_sel_sns_params *)
						sc->sc_rzparams[targid];
                	byteptr = (u_char *)sc->sc_rzaddr[targid];
                	datacnt = msp->msp_length;
	    	    }
		    break;

		case SZ_REASSIGN:
            	    rbp = (struct reassign_params *)sc->sc_rzparams[targid];
                    byteptr = (u_char *)sc->sc_rzparams[targid];
                    datacnt = ((rbp->rp_header.defect_len0 << 0) & 0x00ff) +
    		      	      ((rbp->rp_header.defect_len1 << 8) & 0xff00) + 4;
            	    break;

		case SZ_FORMAT:
            	    dd = (struct defect_descriptors *)sc->sc_rzaddr[targid];
            	    byteptr = (u_char *)sc->sc_rzaddr[targid];
            	    datacnt = ((dd->dd_header.fu_hdr.defect_len0<<0)&0x00ff) +
    		      	      ((dd->dd_header.fu_hdr.defect_len1<<8)&0xff00) + 4;
	    	    break;
		default:
#ifdef SZDEBUG
		    printd ("szintr: SCS_DATAO: unknown cmd 0x%x\n",
			    sc->sz_opcode);
#endif SZDEBUG
		    goto abort;
		    break;
		}
	        /*
                 * Setup softc structure entries for special SCSI DISK
	         * commands that do dma. (FORMAT UNIT), (REASSIGN BLOCK)
	         * and (MODE SELECT).
	         */
        	if(sc->sc_rzspecial[targid] &&
			(sc->sc_curcmd[targid] == sc->sc_actcmd[targid])) {
	            sc->sc_b_bcount[targid] = datacnt;
	            sc->sc_bpcount[targid] = datacnt;
	            sc->sc_bufp[targid] = (char *)byteptr;
	            sc->sc_xfercnt[targid] = 0;
		    goto SETUP1_DMA;
                }
		inicmd_tmp = SCS_INI_ENOUT;
		szaddr->scs_inicmd = inicmd_tmp;
		/*
		 * Send the command packet
		 */
#ifdef SZDEBUG
		printd2("Data Output Packet: \n");		/* 1 of 3 */
#endif SZDEBUG
		for ( ; (cmdcnt > 0); cmdcnt--) {
		    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x0, 0, flags);
			goto abort;
		    }
#ifdef SZDEBUG
		    printd2 (" %x", *byteptr);			/* 2 of 3 */
#endif SZDEBUG
		    szaddr->scs_outdata = *byteptr++;
		    inicmd_tmp |= SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x1, 0, flags);
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		}
#ifdef SZDEBUG
		printd2 ("\n");					/* 3 of 3 */
#endif SZDEBUG
		break;
	    }
SETUP1_DMA:
	    /*
	     * Set up to transfer data to the target via DMA.
	     *
	     * If initial DMA setup, then:
	     *	count - full count for the xfer segment
	     *	addr  - offset of 0 in target's 128KB buffer slot
	     *	bcopy - copy data from user to 128 KB buffer
	     *
	     * If continuing DMA after disconnect, then:
	     *	count - from save data pointer count
	     *	addr  - use saved count to calculate buffer offset
	     *  bcopy - no bcopy needed
	     */

	    /*
	     * 5380 prefetching an extra byte burned us!
	     * On an odd byte count (like 8193) the tape takes 8912
	     * bytes then disconnects, but the 5380 takes on extra
	     * byte, so the saved count is 0 instead of -1.
	     * This hack compensates for the 5380 hack.
	     */
	    if ((sc->sc_devtyp[targid] & SZ_TAPE) &&
		(sc->sc_szflags[targid] & SZ_WAS_DISCON) &&
		(sc->sc_savcnt[targid] == 0)) {
		    sc->sc_savcnt[targid] = -1;
	    }

	    /*
	     * Set up count, address, and bocpy data (if needed).
	     */
	    if ((sc->sc_szflags[targid] & SZ_WAS_DISCON) &&
		(sc->sc_savcnt[targid] != 0)) {
		/*
		 * This is a quick way to subtract the the number of
		 * bytes not transfered before the disconnect, from
		 * the total number of bytes to transfer.  The result
		 * is the number of bytes that need to be transfered.
		 * sc->sc_savcnt is in two's complement form.  The
		 * number of bytes to be transfered is used as an offset
		 * and added to bufp to get the address of where to start
		 * transfering data from.
		 */
		szaddr->scd_cnt = sc->sc_savcnt[targid];
		sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;
		bufp = (char *)(sc->sc_bpcount[targid] + sc->sc_savcnt[targid]);
	    }
	    else {
		szaddr->scd_cnt = -(sc->sc_bpcount[targid]);
		stv = sc->sc_rambuff + sc->sc_dboff[targid];
		bufp = sc->sc_bufp[targid] + sc->sc_xfercnt[targid];
/*		bcopy (bufp, stv, sc->sc_b_bcount[targid]);	*/
		blkcpy (bufp, stv, sc->sc_b_bcount[targid]);
		bufp = 0;	/* 128KB h/w data buf offset-offset */
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
    	     */
	    if (sz_no_spin3)
		szaddr->scs_mode = (SCS_DMA | SCS_PARCK);
	    else
		szaddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK);
	    inicmd_tmp = SCS_INI_ENOUT;
	    szaddr->scs_inicmd = inicmd_tmp;
	    /*
	     * Set the starting address in the 128K buffer.
	     */
	    szaddr->scd_adr = sc->sc_dboff[targid] + (long)bufp;
	    /*
	     * Next four lines of code provide a 1.2 Usec delay.
	     * This delay prevents the SCSI/SCSI controller
	     * hanging when a DMA is started on one bus while
	     * a DMA is in progress on the other bus.
	     * Hardware engineering says we need four reads!
	     */
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    szaddr->scd_dir = SCD_DMA_OUT;
	    /*
	     * Start the DMA transfer
	     */
	    szaddr->scs_dmasend = 1;
	    sc->sc_szflags[targid] |= SZ_DID_DMA;
	    return;
	    break;

	case SCS_DATAI:
#ifdef SZDEBUG
	    printd2("szintr: SCS_DATAI:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_DATAI_PHA;
	    if ( (sc->sz_opcode != SZ_READ) &&
		 (sc->sz_opcode != SZ_READ_10) ) {
		byteptr = (u_char *)&sc->sc_sns[targid];
		switch (sc->sz_opcode) {
		    case SZ_RQSNS:
			num_expected = sc->sz_rqsns.alclen;
			break;

		    case SZ_INQ:
			num_expected = SZ_INQ_MAXLEN;
			byteptr = (u_char *)&sc->sz_dat[targid];
	    	        if(sc->sc_rzspecial[targid]) {
                	    byteptr = (u_char *)sc->sc_rzaddr[targid];
			    datacnt = num_expected;
			}
			break;

		    case SZ_RDCAP:
			num_expected = SZ_RDCAP_LEN;
			byteptr = (u_char *)&sc->sz_dat[targid];
			break;

		    case SZ_MODSNS:
			num_expected = sc->sz_modsns.alclen;
			byteptr = (u_char *)&sc->sz_dat[targid];
	    		if(sc->sc_rzspecial[targid]) {
	        	    msp = (struct mode_sel_sns_params *)
						sc->sc_rzparams[targid];
                	    byteptr = (u_char *)sc->sc_rzaddr[targid];
                	    datacnt = msp->msp_length;
	    	    	}
			break;
		    case SZ_RDD:
            	        rdp = (struct read_defect_params *)
						sc->sc_rzparams[targid];
                        byteptr = (u_char *)sc->sc_rzaddr[targid];
            		datacnt = rdp->rdp_alclen;
            		break;

		    case SZ_READL:
		    case SZ_WRITEL:
			iox = (struct io_uxfer *)sc->sc_rzparams[targid];
			byteptr = (u_char *)sc->sc_rzaddr[targid];
			datacnt = iox->io_cnt;
			break;

		    case SZ_RECDIAG:
			num_expected = SZ_RECDIAG_LEN;
			byteptr = (u_char *)&sc->sz_dat[targid];
			break;

		    default:
#ifdef SZDEBUG
			printd("szintr: SCS_DATAI: unexpected cmd = %x\n",
				sc->sz_opcode);
#endif SZDEBUG
		    break;
		}
	        /*
                 * Setup softc structure entries for special SCSI 
	         * DISK commands that do dma. (READ DEFECT DATA),
	         * (INQUIRY) and (MODE SENSE).
	         */
        	if(sc->sc_rzspecial[targid] &&
			(sc->sc_curcmd[targid] == sc->sc_actcmd[targid])) {
	            sc->sc_b_bcount[targid] = datacnt;
	            sc->sc_bpcount[targid] = datacnt;
	            sc->sc_bufp[targid] = (char *)byteptr;
	            sc->sc_xfercnt[targid] = 0;
		    goto SETUP2_DMA;
                }
		inicmd_tmp = 0;
		szaddr->scs_inicmd = inicmd_tmp;
		/*
		 * Receive the data packet
		 */
#ifdef SZDEBUG
		printd2("Data In Packet:");			/* 1 of 3 */
#endif SZDEBUG
		/*
		 * Receive data until maximum number of bytes
		 * expected is reached or the phase changes.
		 */
		szaddr->scs_mode |= SCS_PARCK;
		for (num_received = 0; (num_expected > 0); num_expected--) {
		    for (retval = 0; retval < 10000; retval++) {
			/* TODO: should check for loss of BSY */
			if ((szaddr->scs_status & SCS_MATCH) == 0)
			    break;
			if (szaddr->scs_curstat & SCS_REQ)
			    break;
			DELAY(100);
		    }
		    if ((szaddr->scs_status & SCS_MATCH) == 0)
			break;
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x10, 0, flags);
			goto abort;
		    }
		    *byteptr++ = szaddr->scs_curdata;
		    num_received++;
#ifdef SZDEBUG
		    printd2(" %x", szaddr->scs_curdata);	/* 2 of 3 */
#endif SZDEBUG
		    inicmd_tmp |= SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x11, 0, flags);
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		}
#ifdef SZDEBUG
		printd2("\n");					/* 3 of 3 */
#endif SZDEBUG
		/* TODO: need num_minimum for each data type */
#ifdef	ELDEBUG
		if ((cntlr == 0) && (targid == sz_eldb_buserr12)) {
		    num_received = 4;
		    sz_eldb_buserr12 = -1;
		}
#endif	ELDEBUG
		if (num_received < 5) {
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x12, 0, flags);
		}
		break;
	    }
SETUP2_DMA:
	    /*
	     * Start of DMA code.
	     *
	     * Set up count and address.
	     */
	    if ((sc->sc_szflags[targid] & SZ_WAS_DISCON) &&
		(sc->sc_savcnt[targid] != 0)) {
		szaddr->scd_cnt = sc->sc_savcnt[targid];
		bufp = (char *)(sc->sc_bpcount[targid] + sc->sc_savcnt[targid]);
#ifdef SZDEBUG
		printd1("szintr#2: scd_cnt = 0x%x, sc_savcnt = 0x%x\n",
			szaddr->scd_cnt, sc->sc_savcnt[targid]);
#endif SZDEBUG
	    }
	    else {
		szaddr->scd_cnt = -(sc->sc_bpcount[targid]);
		bufp = 0;
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
	     */
	    if (sz_no_spin3)
		szaddr->scs_mode = (SCS_DMA | SCS_PARCK);
	    else
		szaddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK);
	    szaddr->scs_inicmd = 0;
	    /*
	     * Set the starting address in the 128K buffer.
	     */
	    szaddr->scd_adr = sc->sc_dboff[targid] + (long)bufp;
	    /*
	     * Next four lines of code provide a 1.2 Usec delay.
	     * This delay prevents the SCSI/SCSI controller
	     * hanging when a DMA is started on one bus while
	     * a DMA is in progress on the other bus.
	     * Hardware engineering says we need four reads!
	     */
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    szaddr->scd_dir = SCD_DMA_IN;
	    /*
	     * Start the DMA transfer
	     */
	    szaddr->scs_dmaircv = 1;
	    sc->sc_szflags[targid] |= SZ_DID_DMA;
	    return;
	    break;

	case SCS_CMD:
	    /* Debug counter showed we never do command phase in szintr */
#ifdef SZDEBUG
	    printd2("szintr: SCS_CMD:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_CMD_PHA;
	    /*
	     * clear savecnt and status
	     */
	    sc->sc_savcnt[targid] = 0;
	    sc->sc_status[targid] = 0xff;
#ifdef	SZ_DSP_DEBUG
	    sc->sc_szflags[targid] &= ~SZ_DID_STATUS;
#endif	SZ_DSP_DEBUG
	    inicmd_tmp = SCS_INI_ENOUT;
	    szaddr->scs_inicmd = inicmd_tmp;
	    byteptr = (u_char *)&sc->sz_command;
	    cmd_type = *byteptr;
	    /*
	     * Send the command packet
	     */
	    i = sz_cdb_length (cmd_type, targid);
#ifdef SZDEBUG
	    if (i == SZ_RDCAP_CMD_LEN) {
		printd2("cmd pkt: %x %x %x %x %x %x %x %x %x %x\n",
		    *byteptr, *(byteptr + 1), *(byteptr + 2), *(byteptr + 3),
		    *(byteptr + 4), *(byteptr + 5), *(byteptr + 6),
		    *(byteptr + 7), *(byteptr + 8), *(byteptr + 9));
	    }
	    else {
		printd2("cmd pkt: %x %x %x %x %x %x\n",
		    *byteptr, *(byteptr + 1), *(byteptr + 2), *(byteptr + 3),
		    *(byteptr + 4), *(byteptr + 5));
	    }
#endif SZDEBUG
	    for (cmdcnt = i; (cmdcnt > 0); cmdcnt--) {
		SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		if (retval >= 10000) {
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x20, 0, flags);
		    goto abort;
		}
		szaddr->scs_outdata = *byteptr++;
		inicmd_tmp |= SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
		SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		if (retval >= 10000) {
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x21, 0, flags);
		    goto abort;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
	    }
	    if ( (cmd_type == SZ_WRITE) || (cmd_type == SZ_READ) ||
         	 (cmd_type == SZ_WRITE_10) || (cmd_type == SZ_READ_10) ) {
		if (sc->sc_dkn[targid] >= 0) {
		    dk_busy |= 1 << sc->sc_dkn[targid];
		    dk_xfer[sc->sc_dkn[targid]]++;
		    dk_wds[sc->sc_dkn[targid]] += sc->sc_bpcount[targid] >> 6;
		}
	    }
	    break;

	case SCS_STATUS:
#ifdef SZDEBUG
	    printd2("szintr: SCS_STATUS:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_STATUS_PHA;
#ifdef	SZ_DSP_DEBUG
	    sc->sc_szflags[targid] |= SZ_DID_STATUS;
#endif	SZ_DSP_DEBUG

	    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
	    if (retval >= 10000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x30, 0, flags);
		goto abort;
	    }
	    szaddr->scs_mode |= SCS_PARCK;
	    sc->sc_status[targid] = szaddr->scs_curdata;
	    /* Save the status byte for the error log */
	    if (sc->sc_curcmd[targid] == sc->sc_actcmd[targid])
		sc->sc_statlog[targid] = sc->sc_status[targid];
	    inicmd_tmp = SCS_INI_ACK;
	    szaddr->scs_inicmd = inicmd_tmp;
#ifdef SZDEBUG
	    printd2("szintr: SCS_STATUS: ppha = 0x%x, status = 0x%x, scd_cnt = 0x%x\n",
		sc->sc_prevpha, sc->sc_status[targid], szaddr->scd_cnt);
#endif SZDEBUG
	    SZWAIT_WHILE(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),50000,retval);
#ifdef	ELDEBUG
	    if ((cntlr == 0) && (targid == sz_eldb_buserr31)) {
		retval = 50000;
		sz_eldb_buserr31 = -1;
	    }
#endif	ELDEBUG
	    if (retval >= 50000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x31, 0, flags);
		goto abort;
	    }
	    inicmd_tmp = 0;
	    szaddr->scs_inicmd = inicmd_tmp;
	    /*
	     * Check the status
	     */
	    if (sc->sc_status[targid] == SZ_BUSY) {
		sc->sc_szflags[targid] |= SZ_BUSYTARG;
		break;
	    }
	    if (sc->sc_status[targid] != SZ_GOOD) {
		sc->sc_szflags[targid] |= SZ_NEED_SENSE;
	    }
	    break;

	case SCS_MESSO:
#ifdef SZDEBUG
	    printd2("szintr: SCS_MESSO:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_MESSO_PHA;
	    /*
	     * Send the Identify message.
	     * Unless we got here due to a sync data xfer
	     * request. In that case send message reject.
	     */
	    if(szaddr->scs_curstat & SCS_REQ) {
		if (exmsg_sdtr) {
		    szaddr->scs_outdata = SZ_MSGREJ;
		    exmsg_sdtr = 0;
		    exmsg_byte = 0;
		}
		else {
		    szaddr->scs_outdata = SZ_ID_DIS;
		}
		inicmd_tmp |= (SCS_INI_ENOUT | SCS_INI_ATN);
		szaddr->scs_inicmd = inicmd_tmp;
		DELAY(2);
		inicmd_tmp &= ~SCS_INI_ATN;
		szaddr->scs_inicmd = inicmd_tmp;
		inicmd_tmp |= SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
		SZWAIT_WHILE((szaddr->scs_curstat & SCS_REQ),100000,retval);
		if(retval >= 100000) {
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x60, 0, flags);
		    goto abort;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;

		if (szaddr->scs_status & SCS_PARERR) {
		    flags = SZ_HARDERR | SZ_LOGREGS;
		    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 4, 0, flags);
		    goto abort;
		}
	    }
	    break;

	case SCS_MESSI:
		/* for spin, next phase could also be MESSI! */
		ftt = 1;
#ifdef SZDEBUG
	    printd2("szintr: SCS_MESSI:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_MESSI_PHA;
	    /*
	     * If exmsg_byte is nonzero we are receiving an
	     * extended message. Save the byte as data.
	     */
	    szaddr->scs_mode |= SCS_PARCK;
	    if (exmsg_byte)
		exmsg_data = szaddr->scs_curdata;
	    else
		sc->sc_message[targid] = szaddr->scs_curdata;
	    inicmd_tmp |= SCS_INI_ACK;
	    szaddr->scs_inicmd = inicmd_tmp;
	    SZWAIT_WHILE(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
	    if(retval >= 10000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x71, 0, flags);
		goto abort;
	    }
	    /*
	     * If extended message don't drop ACK yet.
	     * We may have to assert ATTN later.
	     */
	    if (sc->sc_message[targid] != SZ_EXTMSG) {
		inicmd_tmp &= ~SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
	    }
#ifdef	ELDEBUG
	    if ((cntlr == 0) && (targid == sz_eldb_buserr73)) {
		sc->sc_message[targid] = SZ_IDE;
		sz_eldb_buserr73 = -1;
	    }
#endif	ELDEBUG
	    switch (sc->sc_message[targid]) {
		case SZ_EXTMSG:
#ifdef	SZDEBUG
		    printd("szintr: SZ_EXTMSG\n");
#endif	SZDEBUG
		    /*
		     * Extended message. If its a sync data xfer message:
		     * receive message, raise attn, send message reject
		     * to force async mode. Otherwise: receive message
		     * and hope the device terminates the MESSI phase.
		     */
		    if (exmsg_byte == 0) {		/* ext msg start */
			exmsg_byte = 1;
		    }
		    else if (exmsg_byte == 1) {		/* ext msg length */
			exmsg_len = exmsg_data;
			exmsg_cnt = 0;
			exmsg_byte = 2;
		    }
		    else if (exmsg_byte == 2) {		/* ext msg type */
			exmsg_byte = 3;
			if (exmsg_data == SZ_SYNC_XFER)
			    exmsg_sdtr = 1;		/* sync data xfer req */
			else
			    exmsg_sdtr = 0;		/* other ext msg type */
			exmsg_cnt++;
		    }
		    else {				/* rcv rest of extmsg */
			exmsg_cnt++;			/* discard each byte */
			exmsg_byte++;
			if (exmsg_cnt >= exmsg_len) {	/* last exmsg byte */
			    if (exmsg_sdtr) {
				/* if ext msg is sync data xfer req, */
				/* assert attn, target should goto msg out */
				inicmd_tmp |= SCS_INI_ATN;
				szaddr->scs_inicmd = inicmd_tmp;
			    }
			    exmsg_byte = 0;		/* end of ext msg */
			}
		    }
		    /* drop ACK to complete handshake */
		    inicmd_tmp &= ~SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		    break;

		case SZ_CMDCPT:
#ifdef	SZDEBUG
		    printd("szintr: SZ_CMDCPT:\n");
#endif	SZDEBUG
#ifdef	SZ_DSP_DEBUG
		    if ((sc->sc_szflags[targid] & SZ_DID_STATUS) == 0) {
		        if (sz_nsp_print)
			{
			printf("szintr: (bus=%d ID=%d cmd=0x%x acmd=0x%x) %s\n",
			    cntlr, targid, sc->sc_curcmd[targid],
			    sc->sc_actcmd[targid],
			    "NO status phase");
			}
		    }
		    else
			sc->sc_szflags[targid] &= ~SZ_DID_STATUS;
#endif	SZ_DSP_DEBUG
		    /*
		     * The command has completed, release the bus.
		     */
		    inicmd_tmp = 0;
		    szaddr->scs_inicmd = 0;
		    szaddr->scs_mode = SCS_PARCK;
		    szaddr->scs_tarcmd = 0;
		    /*
		     * clear SCS_PARERR, SCS_INTREQ, and SCS_BSYERR 
		     * in mode reg, and return 
		     */
		    sc->sc_selstat[targid] = SZ_IDLE;
		    sc->sc_fstate = 0;
		    /*
		     * Call sz_start() to complete this transfer
		     * and, possibly, start the next transfer.
		     */
		    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
		    /*
		     * Wait 10 MS for device to drop BSY after command
		     * complete. Log an error if BSY doesn't drop.
		     * If just BSY true, device is holding BSY.
		     * If SEL true, some other device grabbed bus.
		     * NOTE: check 3 times is for bus settle delay (400 ns)
		     * We call sz_start(), which will find the bus busy
		     * and go idle, then timer1 should reset the bus.
		     * NOTE: TZ30 held BSY up to 800 Usec.
		     * TODO: verify TZ30 fixed in firmware rev ?
		     */
		    for (i = 0; i < 1000; i++) {
			if (((szaddr->scs_curstat & (SCS_BSY|SCS_SEL)) == 0) &&
			    ((szaddr->scs_curstat & (SCS_BSY|SCS_SEL)) == 0) &&
			    ((szaddr->scs_curstat & (SCS_BSY|SCS_SEL)) == 0)) {
				break;	/* bus is free */
			}
			/* bus still busy */
			if ((szaddr->scs_curstat & SCS_SEL) &&
			    (szaddr->scs_curstat & SCS_SEL) &&
			    (szaddr->scs_curstat & SCS_SEL)) {
				break; /* sel true, somebody else grabbed bus */
			}
			DELAY(10);
		    }
#ifdef	ELDEBUG
		    if ((cntlr == 0) && (targid == sz_eldb_buserr74)) {
			i = 1001;
			sz_eldb_buserr74 = -1;
		    }
#endif	ELDEBUG
		    if (i >= 1000) {	/* busy way too long! */
			flags = SZ_HARDERR |SZ_LOGREGS;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x74, 0, flags);
		    }
#ifdef	SZ_BSYCC_STATS
		    else {
			if (i > sz_i_bsycc[unit])
			    sz_i_bsycc[unit] = i;
		    }
#endif	SZ_BSYCC_STATS
		    szaddr->scs_selena = sc->sc_sysid;
		    sc->sc_scs_selena = sc->sc_sysid;
		    DELAY(10);
		    sc->sc_active = 0;
		    /* Assumes one command at a time for each target */
		    if (sc->sc_dkn[targid] >= 0)
			dk_busy &= ~(1 << sc->sc_dkn[targid]);

		    /* TODO - EXABYTE: call start if SEL asserted? */
		    if (sc->sc_szflags[targid] & SZ_BUSYTARG) {
			sc->sc_xstate[targid] = sc->sc_pxstate[targid];
			if ((sc->sc_xstate[targid] == SZ_SP_START) &&
			    (sc->sc_actcmd[targid] == SZ_RQSNS) &&
			    (sc->sc_curcmd[targid] != SZ_RQSNS)) {
				sc->sc_xevent[targid] = SZ_SELWAIT2;
			}
			else {
				sc->sc_xevent[targid] = SZ_SELWAIT1;
			}
			sz_start(sc, -1);
			timeout(sz_timer4, (caddr_t)unit, 50);	/* 0.5 sec */
			complete++;
			break;
		    }
		    /*
		     * If SEL asserted then the bus is busy.
		     * Don't waste time attempting to start the
		     * next command if the bus is busy.
		     */
		    sc->sc_szflags[targid] &= ~SZ_BUSYBUS;
		    if (szaddr->scs_status & SCS_INTREQ) {
			if (szaddr->scs_curstat & SCS_SEL)
			    sc->sc_szflags[targid] |= SZ_BUSYBUS;
		    }
#ifdef	ELDEBUG
		    if ( ((sc->sc_curcmd[targid] == SZ_READ) ||
			  (sc->sc_curcmd[targid] == SZ_READ_10)) &&
			  (cntlr == 0) && (targid == sz_eldb_dbbr0)) {
			flags = SZ_HARDERR;
/* ELDEBUG */		scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 0, 0, flags);
			sz_eldb_dbbr0 = -1;
		    }
		    if ( ((sc->sc_curcmd[targid] == SZ_READ) ||
			  (sc->sc_curcmd[targid] == SZ_READ_10)) &&
			  (cntlr == 0) && (targid == sz_eldb_dbbr1)) {
			flags = SZ_HARDERR;
/* ELDEBUG */		scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 1, 0, flags);
			sz_eldb_dbbr1 = -1;
		    }
		    if ( ((sc->sc_curcmd[targid] == SZ_READ) ||
			  (sc->sc_curcmd[targid] == SZ_READ_10)) &&
			  (cntlr == 0) && (targid == sz_eldb_dbbr2)) {
			flags = SZ_SOFTERR;
/* ELDEBUG */		scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 2, 0, flags);
			sz_eldb_dbbr2 = -1;
		    }
		    if ( ((sc->sc_curcmd[targid] == SZ_READ) ||
			  (sc->sc_curcmd[targid] == SZ_READ_10)) &&
			  (cntlr == 0) && (targid == sz_eldb_dbbr3)) {
			flags = SZ_HARDERR;
/* ELDEBUG */		scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 3, 0, flags);
			sz_eldb_dbbr3 = -1;
		    }
#endif	ELDEBUG
		    sz_start(sc, targid);
		    complete++;
		    break;
			
		case SZ_SDP:
		    sz_sdp[unit]++;
		    if (sc->sc_prevpha == SZ_DATAO_PHA) {
			/*
			 * The scd_cnt register is 32 bits long, but
			 * bits 17 - 31 always read as zeros. This means
			 * we have to fake sign extension on scd_cnt reads.
			 * If bit 16 set (negative count) force bits 17-31 set.
			 */
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid] = (szaddr->scd_cnt|0xfffe0000)-1;
			else
			    sc->sc_savcnt[targid] = szaddr->scd_cnt;
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
		    }
/* Fix read defect data bug. Defect list appeared trashed */
/*		    else {				*/
/*			sc->sc_savcnt[targid] = 0;	*/
/*		    }					*/

		    if (sc->sc_prevpha == SZ_DATAI_PHA) {
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid]= (szaddr->scd_cnt|0xfffe0000);
			else
			    sc->sc_savcnt[targid]= szaddr->scd_cnt;
		    }
#ifdef SZDEBUG
		    printd1("szintr: SZ_SDP, scd_cnt = 0x%x\n",
			sc->sc_savcnt[targid]);

#endif SZDEBUG
		    break;

		case SZ_DISCON:
#ifdef SZDEBUG
		    printd2("szintr: disconnect message\n");
#endif SZDEBUG
		    /* NOTE: this also disables BSY timing below. */
		    if (sz_do_isdp == 0) {	/* don't do implied SDP */
			szaddr->scs_mode = (SCS_MONBSY | SCS_PARCK);
			return;
			break;	/* NOTREACHED */
		    }
		    /*
		     * Implied save data pointer. Only if target disconnects
		     * without sending a save data pointer message. Previous
		     * phase will be a data in/out phase in this case.
		     */
		    if (sc->sc_prevpha == SZ_DATAO_PHA) {
			sz_isdp[unit]++;
			/*
			 * The scd_cnt register is 32 bits long, but
			 * bits 17 - 31 always read as zeros. This means
			 * we have to fake sign extension on scd_cnt reads.
			 * If bit 16 set (negative count) force bits 17-31 set.
			 */
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid] = (szaddr->scd_cnt|0xfffe0000)-1;
			else
			    sc->sc_savcnt[targid] = szaddr->scd_cnt;
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
		    }
		    if (sc->sc_prevpha == SZ_DATAI_PHA) {
			sz_isdp[unit]++;
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid]= (szaddr->scd_cnt|0xfffe0000);
			else
			    sc->sc_savcnt[targid]= szaddr->scd_cnt;
		    }
		    /*
		     * This code saves about 60 interrupts/sec on a busy bus.
		     * Spin or take an interrupt depending on how long the
		     * after the disconnect message the deivces holds BSY.
		     * CAUTION: changing this code will effect the timing!
		     */
		    for (reg_cnt = 0; reg_cnt < 10000000; reg_cnt++) {
			if (reg_cnt >= sz_i_btcnt) {
			    szaddr->scs_mode = (SCS_MONBSY | SCS_PARCK);
			    return;
			}
			if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
			    ((szaddr->scs_curstat & SCS_BSY) == 0) &&
			    ((szaddr->scs_curstat & SCS_BSY) == 0)) {
			    break;
			}
		    }
#ifdef	SZ_BT_DEBUG
		    if (reg_cnt > sz_i_bt[unit])
			sz_i_bt[unit] = reg_cnt;
#endif	SZ_BT_DEBUG
		    goto sz_i_discon;		/* I know, but its necessary */
		    break;	/* NOTREACHED */
			
		case SZ_ID_NODIS:
		    /*
		     * A target should never send this message.
		     * Bit 6 only set by the initiator.
		     * Should be an error, but never seen one and
		     * doubt it would cause a problem anyway.
		     */
#ifdef SZDEBUG
		    printd2("szintr: identify message\n");
#endif SZDEBUG
		    break;

		case SZ_ID_DIS:
#ifdef SZDEBUG
		    printd2("szintr: identify with disconnect message\n");
#endif SZDEBUG
		    break;

		case SZ_RDP:
		    sz_rdp[unit]++;
#ifdef	SZDEBUG
		    printd("szintr: SZ_RDP:\n");
#endif	SZDEBUG
		    break;

		/*
		 * These messages are unsupported or should never
		 * be received (only sent) by the initiator.
		 */
		case SZ_IDE:		/* Initiator Detected Error */
		case SZ_ABT:		/* Abort */
		case SZ_MSGREJ:		/* Message (should handle this one) */
		case SZ_NOP:		/* No Operation */
		case SZ_MSGPE:		/* Message Parity error */
		case SZ_LNKCMP:		/* Linked Command Complete */
		case SZ_LNKCMPF:	/* Linked Command Complete with flag */
		case SZ_DEVRST:		/* Bus Device Reset */
		    /* FALLTHROUGH */
		default:
		    /*
		     * Unknown message or identify with non zero sub-unit.
		     */

		    flags = SZ_HARDERR | SZ_LOGMSG;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x73, 0, flags);
		    /*
		     * We abort! Should assert ATTN, hope target goes to
		     * message out, and send an abort message, but
		     * its just too messy and this error crashes the
		     * world every time it happens (so far).
		     */
		    goto abort;
	    }
	    break;

	default:
	    ;
#ifdef SZDEBUG
	    mprintf("szintr: unknown phase = 0x%x\n",
		szaddr->scs_status & SCS_PHA_MSK);
#endif SZDEBUG
	}
    } /* end of do loop */
    while (complete == 0);
    return;

abort:
    exmsg_byte = 0;
    exmsg_sdtr = 0;
    sc->sc_active = 0;
    /*
     * If the bus is wedged (BSY and/or SEL stuck) or
     * force_reset is nonzero
     * reset the bus, controller, and the rest of the world.
     * We allow about 1 MS for the bus to free up.
     * If SEL & I/O asserted bus not hung (reselect waiting).
     * NOTE: it is possible for BSY to be true with SEL false,
     *       but only for a very short time (a few Usec at most).
     */
    /* we can get here without detecting reset */
    if (force_reset == 0) {
	if ((szaddr->scs_curstat & SCS_RST) ||
	    ((szaddr->scs_mode & SCS_PARCK) == 0)) {
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 2, 0, SZ_HARDERR);
	    force_reset = 1;
	}
    }
    szaddr->scs_mode |= SCS_PARCK;
/* TODO: need to turn off ENOUT? */
    if (sz_do_reset) {
	wedged = 1;
	for (retval = 0; retval < 1000; retval++) {
	    if (force_reset)
		break;
	    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0)) {
		if ((szaddr->scs_curstat&(SCS_SEL|SCS_IO))==(SCS_SEL|SCS_IO)) {
			wedged = 0;
			break;
		}
		if (((szaddr->scs_curstat & SCS_SEL) == 0) &&
		    ((szaddr->scs_curstat & SCS_SEL) == 0) &&
		    ((szaddr->scs_curstat & SCS_SEL) == 0)) {
			wedged = 0;
			break;
		}
	    }
	    DELAY(1);
	}
    }
#ifdef	ELDEBUG
    if (sz_eldb_rstbus1 >= 0) {
	wedged = 1;
	sz_eldb_rstbus1 = -1;
    }
#endif	ELDEBUG
    if (sz_do_reset && wedged) {
	if (targid >= 0) {
	    flags = SZ_HARDERR | SZ_LOGCMD;
	    scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 0, 0, flags);
	}
	flags = SZ_HARDERR | SZ_LOGREGS;
	scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 1, 0, flags);
	sz_reset(cntlr);
	return;
    }
    /*
     * We aborted somewhere above. We can't do much
     * about it if the target ID is unknown (i.e., = -1).
     * The controller has already been reset (read of scs_reset reg.)
     * by the time we get here, so don't reset again or an
     * interrupt could get lost.
     */
    if (targid == -1) {
	scsi_logerr(sc, 0, -1, SZ_ET_CMDABRTD, 1, 0, SZ_HARDERR);
	return;
    }
    /*
     * If we aborted due to an interrupt from an inactive
     * target, just call sz_start() to kick off next command.
     */
    dp = &szutab[sc->sc_unit[targid]];
    if ((dp->b_active == 0) || (dp->b_actf == 0)) {
	sz_start(sc, -1);
	return;
    }
    /*
     * Tell sz_start() we aborted and let it
     * finish cleaning up.
     */
    flags = SZ_HARDERR | SZ_LOGCMD;
    scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 2, 0, flags);
    sc->sc_szflags[targid] |= SZ_ENCR_ERR;
    sc->sc_xstate[targid] = SZ_ERR;
    sc->sc_xevent[targid] = SZ_ABORT;
    sz_start(sc, targid);
    return;
}


/*
 * sz_scsistart is called to start the transfer on the SCSI bus.
 *
 * If bp is zero the call came from the probe routine
 * and requires speical handling.
 */


#ifdef	SPIN_STATS
int	sz_ss_sel[16];
#endif	SPIN_STATS


/*
 *
 * Name:		sz_scsistart	-SCSI command start routine
 *
 * Abstract:		This routine starts a SCSI command on the
 *			specified target device. General flow is:
 *
 *			1. Tell the 5380 to arbitrate for the bus.
 *			2. Back off if 5380 looses arbitration.
 *			3. Select (with attention) the target device.
 *			4. Timeout select if it fails.
 *			5. Follow phases set by target (normally):
 *			   Message out - identify with disconnect.
 *			   Command - send commmand bytes to target.
 *			   For data xfer and tape motion commands,
 *			   set up for phase mismatch interrupt, return.
 *			   Data in/out (for some commands).
 *			   Status phase.
 *			   Message in - command complete.
 *
 *
 * Inputs:
 *
 * sc			Pointer to sz_softc for this controller.
 * targid		Targid ID of device (0 - 7).
 * bp			Buffer pointer for I/O request. Zero if
 *			called from the probe routine.
 *
 * Outputs:		None.
 *
 * Return Values:
 *
 * SZ_SUCCESS		Command completed successfully.
 * SZ_IP		Command in progress waiting for interrupt.
 * SZ_RET_ERR		Command failed (returned bad status).
 * SZ_RET_ABORT		Command aborted.
 * SZ_BUSBUSY		Bus is busy, retry the command later.
 * SZ_SELBUSY		Waiting for 250 MS select timeout to expire.
 * SZ_RET_RESET		Resetting bus, retry command after bus reset.
 *
 * Side Effects:
 *
 *			Target's context in sz_softc is updated.
 *			DMA transfers may be initiated.
 *			SCSI bus can be reset (if bus hang detected).
 *			Error messages logged to console and error log.
 *			IPL raised to 16 in critical code path.
 *
 *
 */

#ifdef	ELDEBUG
/* Set ID of target to select timeout (on bus 0 only) */
int	sz_eldb_seltimo = -1;
#endif	ELDEBUG

sz_scsistart(sc, targid, bp)
register struct sz_softc *sc;
int targid;
register struct buf *bp;
{
    int cntlr = sc - &sz_softc[0];
    int unit = sc->sc_unit[targid];
    register reg_cnt;	/* MUST be a register (for ARB/SEL timing below) */
    register struct sz_regs *szaddr = (struct sz_regs *)szmem + cntlr;
    register struct nb_regs *sziaddr = (struct nb_regs *)nexus;
    char *stv;
    char *bufp;
    u_char *byteptr;
    u_char cmd_type;
    int i, s, retval, waitval;
    int cmdcnt;
    int complete;
    int	num_expected, num_received;
    int exmsg_byte, exmsg_len, exmsg_cnt, exmsg_sdtr, exmsg_data;
    int nospin;
    int wedged, force_reset;
    int aipfts, lostarb;
    int abort_parck;
    int flags, subtyp;
    struct format_params *fp;
    struct reassign_params *rbp;
    struct read_defect_params *rdp;
    struct defect_descriptors *dd;
    struct mode_sel_sns_params *msp;
    struct io_uxfer *iox;
    int datacnt;

    force_reset = 0;
    if ((szaddr->scs_curstat & SCS_RST) ||
	((szaddr->scs_mode & SCS_PARCK) == 0)) {
	scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 3, 0, SZ_HARDERR);
	force_reset = 1;
	goto abort;
    }

    /*
     * If normal scsistart call, i.e., to start a command,
     * arbitrate and begin the selection.
     * If select timeout wait callback, skip arbitration/selection
     * and complete the 250 MS selection timeout.
     * The SZ_SELWAIT flag is set if its a select wait callback.
     */
ss_sel_rt:		/* We come here to retry after a select failure */
    if ((sc->sc_szflags[targid] & SZ_SELWAIT) == 0) {

	/*
	 * Turn on select enable so we get an interrupt
	 * when a dicsonnected target reselects us.
	 * Only really needed first time thru.
	 */
	szaddr->scs_selena = sc->sc_sysid;
	sc->sc_scs_selena = sc->sc_sysid;

	if (sc->sc_active != 0) {		/* SHOULD NEVER HAPPEN! */
	    for (i = 0; i < NDPS; i++)
		if (sc->sc_active & (1 << i))
		    break;
	    scsi_logerr(sc, 0, i, SZ_ET_ACTSTAT, 3, 0, SZ_HARDERR);
	    goto abort;
	}

	/*
	 * If in SZ_BBWAIT state, we were waiting for bus to free up.
	 * If we get here we are no longer waiting. So we set the
	 * select state to SZ_IDLE. This is only for more accurate
	 * state tracking.
	 */
	if (sc->sc_selstat[targid] == SZ_BBWAIT)
	    sc->sc_selstat[targid] = SZ_IDLE;

	/*
	 * Arbitrate for the bus, and select the device.
	 *
	 * Set system's ID on data lines, and start arbitration for bus
	 * (the 5380 chip waits for bus free before starting abritration).
	 *
	 * NOTE:
	 *	Parity checking must be disabled during arbitration because
	 *	we read the curdata register and multiple targets can be
	 *	driving the data bus, which could (does) cause parity errors.
	 * NOTE:
	 *	As painfull as it is the spl6 is necessary, because
	 * 	the arbitration/selection timing is critical due to
	 *	the arbitration hacks in the NCR 5380 chip.
	 */

	/*
	 * Place initiator's ID in the output data register
	 * and tell the 5380 to arbitrate.
	 */
	s = spl6();
	force_reset = 0;
	if ((szaddr->scs_curstat & SCS_RST) ||
	    ((szaddr->scs_mode & SCS_PARCK) == 0)) {
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 4, 0, SZ_HARDERR);
	    force_reset = 1;
	    splx(s);
	    goto abort;
	}
	szaddr->scs_mode &= ~SCS_PARCK;	/* make sure parity checking off */
	while (1) {
	    szaddr->scs_outdata = sc->sc_sysid;
	    szaddr->scs_mode |= SCS_ARB;

	    /*
	     * Spin for about 20 Usec waiting for Arbitration In Progress
	     * to set. Exit the spin loop as soon as AIP sets.
	     */
	    for (reg_cnt = 0; reg_cnt < 20; reg_cnt++)
		if (szaddr->scs_inicmd & SCS_INI_AIP)
		    break;

	    /*
	     * If AIP failed to set the bus is busy and we must wait.
	     * Must clear 5380 ARB bit ASAP.
	     * Timing is critical, no debug printfs.
	     */
	    lostarb = aipfts = 0;
	    if ((szaddr->scs_inicmd & SCS_INI_AIP) == 0) {
		aipfts++;
		break;
	    }
	    for (reg_cnt = 0; reg_cnt < 22; reg_cnt++);	/* 2.2 Usec arb delay */

	    /*
	     * Check for lost arbitration, if haven't lost,
	     * then check for higher IDs on the bus. If my
	     * ID is the highest, I've won the arbitration
	     * Check lost arbitration again (higher IDs can
	     * grab the bus from me at the last Usec).
	     * Return bus busy status if we loose arbitration.
	     */
	    if ((szaddr->scs_inicmd & SCS_INI_LA) == 0) {
		if ((szaddr->scs_curdata & ~sc->sc_sysid) <= sc->sc_sysid) {
		    /*
		     * Check for lost arbitration again,
		     * just to be sure (MUST DO THIS).
		     */
		    if (szaddr->scs_inicmd & SCS_INI_LA) {
			lostarb++;
			break;
		    }
		}
		else {
		    lostarb++;
		    break;
		}
	    }
	    else {
		lostarb++;
		break;
	    }
	    break;
	}
	if (aipfts || lostarb) {
	    /*
	     * Either we could not arbitrate (AIP failed to set)
	     * because the bus was busy or we lost arbitration.
	     * Clean out bits we set in the mode register
	     * and return bus busy status.
	     * Command will be retried later.
	     * NOTE: if ARB is clear then a bus reset occurred.
	     */
	    if (aipfts)
		sc->sc_aipfts++;
	    else
		sc->sc_lostarb++;
	    szaddr->scs_outdata = 0;
	    force_reset = 0;
	    if ((szaddr->scs_curstat & SCS_RST) ||
		((szaddr->scs_mode & SCS_ARB) == 0)) {
		scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 5, 0, SZ_HARDERR);
		force_reset = 1;
		splx(s);
		goto abort;
	    }
	    szaddr->scs_mode &= ~(SCS_ARB);
	    szaddr->scs_mode |= SCS_PARCK;
	    splx(s);
	    return(SZ_BUSBUSY);
	}
	/*
	 * If we get here, we have jumped thru
	 * all the hoops and won arbitration.
	 */

	/*
	 * Turn off select enable so we don't generate
	 * an interrupt when we select the target.
	 * This is necessary for two reasons: First, the
	 * conditions for setting INTREQ will be true during
	 * the select (system's ID on bus, BSY false, & SEL true).
	 * We don't want to set INTREQ because we would have to
	 * clear it, by reading scs_reset and writing nb_int_reqclr),
	 * either of which could cause us to loose an interrupt.
	 * Second, the NCR 5380 appears to have a bug which causes
	 * the chip to glitch (set then reset) INTREQ instead
	 * of latching it during a select enable interrupt. This
	 * causes the VS3100 interrupt controller's request bit to be
	 * set without the 5380's INTREQ set (stray interrupt).
	 */
	szaddr->scs_selena = 0;
	sc->sc_scs_selena = 0;

	/*
	 * Select the target device.
	 */
	szaddr->scs_tarcmd = 0;		/* phase matching, make 5380 happy */
	/*
	 * Load the initiator's IDs into the output data register.
	 * Assert SEL, BSY, ATN, and ENOUT and clear arbitration.
	 *
	 * NOTE: the 1.2 Usec delay before placing the IDs on the
	 *	 bus is met (actually exceeded) by amount of code
	 *	 executed before we get here.
	 */
	szaddr->scs_outdata = sc->sc_sysid;
	inicmd_tmp=(SCS_INI_ATN|SCS_INI_SEL|SCS_INI_BSY|SCS_INI_ENOUT);
	szaddr->scs_inicmd = inicmd_tmp;
	for (reg_cnt = 0; reg_cnt < 12; reg_cnt++);	/* 1.2 Usec delay */
	force_reset = 0;
	if ((szaddr->scs_curstat & SCS_RST) ||
	    ((szaddr->scs_mode & SCS_ARB) == 0)) {
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 6, 0, SZ_HARDERR);
	    force_reset = 1;
	    splx(s);
	    goto abort;
	}
	/*
	 * There is a small window from here to where PARCK
	 * gets set again below, in which we will miss a
	 * bus reset if one occurs.
	 */
	szaddr->scs_mode &= ~(SCS_ARB);

	/*
	 * In the past there were problems with devices who lost
	 * arbitration not removing their ID from the bus fast enough.
	 * All the DIGITAL devices have been cleaned up, but this
	 * check remains just in case.
	 *
	 * The maximum wait time is unknown, so we allow about
	 * 500 Usec for all target IDs to drop. This is an
	 * instruction counted spin loop so the time will be
	 * less if on a faster CPU, but should still be long enough.
	 * NOTE: may consider retrying the select, but abort for now.
	 */
	for (reg_cnt = 0; reg_cnt < 500; reg_cnt++) {
	    if ((i = szaddr->scs_curdata) == sc->sc_sysid)
		break;
	}
	if (reg_cnt >= 500) {
	    /* NOTE: debug message, so don't call scsi_logerr(). */
	    printf("scsistart: %s (bus=%d ID=%d cur_data=0x%x)\n",
		"ID value on data bus incorrect",
		cntlr, targid, i);
	    sz_dumpregs(cntlr, 1);	/* mprintf */
	    szaddr->scs_outdata = 0;
	    szaddr->scs_inicmd = 0;
	    splx(s);
	    szaddr->scs_mode |= SCS_PARCK;
	    goto abort;
	}
	/*
	 * Place initiator and target IDs on the bus.
	 */
	i = (sc->sc_sysid | (1 << targid));
#ifdef	ELDEBUG
	if ((cntlr == 0) && (targid == sz_eldb_seltimo)) {
	    i = sc->sc_sysid;
	    sz_eldb_seltimo = -1;
	}
#endif	ELDEBUG
	szaddr->scs_outdata = i;
	for (reg_cnt = 0; reg_cnt < 10; reg_cnt++);	/* 1 Usec delay */
	inicmd_tmp &= ~SCS_INI_BSY;
	szaddr->scs_inicmd = inicmd_tmp;
	/*
	 * Delay at least 400ns, and start looking for BSY
	 * from the target.
	 * NOTE: we actually wait about 1 Usec.
	 */
	for (reg_cnt = 0; reg_cnt < 10; reg_cnt++);	/* 1 Usec delay */
	splx(s);
	szaddr->scs_mode |= SCS_PARCK;

	/*
	 * Wait 250 MS for the target to assert BSY.
	 * Then invoke select timeout procedure.
	 *
	 * If called from the probe routine we spin for the
	 * entire 250 MS here is scsistart.
	 * Otherwise, we spin for 20 MS waiting for BSY.
	 * If BSY does not set, we start timer3 for 30 MS.
	 * If BSY still not set, timer3 waits 200 more MS, in
	 * four 50 MS timeouts.
	 */
	if (bp == 0)	/* called from szprobe */
	    waitval = 2500;	/* 250 MS */
	else
	    waitval = 200;	/* 20 MS */
	/* TODO: debug for SEL hang */
	for (retval = 0; retval < waitval; retval++) {
	    if ((szaddr->scs_curstat & SCS_BSY) &&
		(szaddr->scs_curstat & SCS_BSY) &&
		(szaddr->scs_curstat & SCS_BSY))
		    break;
	    DELAY(100);
	}
#ifdef	SPIN_STATS
	if ((bp != 0) && (retval < waitval) && (retval > sz_ss_sel[unit]))
	    sz_ss_sel[unit] = retval;
#endif	SPIN_STATS
	if (bp != 0) {		/* not called from szprobe */
	    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0)) {
		sc->sc_szflags[targid] |= SZ_SELWAIT;
		sc->sc_swcount = 0;
		timeout(sz_timer3, unit, 3);	/* call sz_timer3 after 30 MS */
		/* don't need to clear sc_dkn, not set until CMD phase */
		sc->sc_active = (1 << targid);
		sc->sc_selstat[targid] = SZ_SELTIMO;
		return(SZ_SELBUSY);
	    }
	}
    }
    /*
     * Execution begins here on select timeout wait callback.
     */
    sc->sc_active = 0;
    sc->sc_selstat[targid] = SZ_IDLE;
    sc->sc_szflags[targid] &= ~SZ_SELWAIT;
    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
	((szaddr->scs_curstat & SCS_BSY) == 0) &&
	((szaddr->scs_curstat & SCS_BSY) == 0)) {
	szaddr->scs_outdata = 0;
	/* Spec says minimum of 200 Usec (no spl6 needed). */
	DELAY(200);
	if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
	    ((szaddr->scs_curstat & SCS_BSY) == 0) &&
	    ((szaddr->scs_curstat & SCS_BSY) == 0)) {
	    inicmd_tmp &= ~(SCS_INI_SEL | SCS_INI_ATN | SCS_INI_ENOUT);
	    szaddr->scs_inicmd = inicmd_tmp;
/*	    szaddr->scs_mode = SCS_PARCK;	TODO: ? */
	    if (bp != 0) {		/* not called from probe routine */
		scsi_logerr(sc, 0, targid, SZ_ET_SELTIMO, 0, 0, SZ_HARDERR);
	    }
	    if (bp == 0)	/* called from probe, don't retry select */
		goto abort;
	    if (++sc->sc_sel_retry[targid] <= 2)
		goto ss_sel_rt;			/* retry the select */
	    else
		goto abort;
	}
    }
    /*
     * reset select and data bus
     */
    sc->sc_sel_retry[targid] = 0;			/* clear select retry count */
    inicmd_tmp &= ~(SCS_INI_SEL | SCS_INI_ENOUT);
    szaddr->scs_inicmd = inicmd_tmp;
    sc->sc_selstat[targid] = SZ_SELECT;
    sc->sc_active = (1 << targid);
    szaddr->scs_mode |= SCS_PARCK;

    /*
     * Turn select enable back on after select done.
     */
    szaddr->scs_selena = sc->sc_sysid;
    sc->sc_scs_selena = sc->sc_sysid;

#ifdef	ELDEBUG
    if ((cntlr == 0) && (targid == sz_eldb_cmdabrtd3)) {
	sz_eldb_cmdabrtd3 = -1;
	goto abort;
    }
#endif	ELDEBUG

    /*
     * The target controls the bus phase. We wait for REQ and BSY
     * to be asserted on the bus before determining which phase
     * is active.  Once REQ and BSY are asserted, the appropiate action
     * is taken. 
     */

    exmsg_byte = 0;
    exmsg_sdtr = 0;
    complete = 0;
    do {
	for (retval = 0; retval < 100000; retval++) {
	    if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0) &&
		((szaddr->scs_curstat & SCS_BSY) == 0)) {
		retval = 1000000;
		break;
	    }
	    if (szaddr->scs_curstat & SCS_REQ)
		break;
	    DELAY(100);
	}
#ifdef	SPIN_STATS
	if (bp && (retval > sc->sc_ss_spin1[targid]) && (retval < 100000)) {
	    sc->sc_ss_spin1[targid] = retval;
	    sc->sc_ss_spcmd[targid] = sc->sc_curcmd[targid];
	    sc->sc_ss_phase[targid] = ((szaddr->scs_curstat&SCS_PHA_MSK) >> 2);
	}
#endif	SPIN_STATS
	if (retval >= 100000) {
	    if (retval == 1000000)
		subtyp = 0x0ce;	/* BSY dropped */
	    else
		subtyp = 0x0cf;	/* REQ failed to set */
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, subtyp, 0, flags);
	    goto abort;
	}
	if ((szaddr->scs_curstat & SCS_RST) ||
	    ((szaddr->scs_mode & SCS_PARCK) == 0)) {
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 7, 0, SZ_HARDERR);
	    force_reset = 1;
	}
	if (szaddr->scs_status & SCS_PARERR) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 5, 0, flags);
	    i = szaddr->scs_reset;
	    goto abort;
	}
#ifdef	bogus
	if (force_reset)
	    goto abort;
#endif	bogus
	/*
	 * This reset was needed to clear the interrupt request
	 * generated because the select was done with select enable on.
	 */
	/* i = szaddr->scs_reset; */

	/*
	 * Zero the scs_mode register to clear all 
	 * DMA status bits from the last DMA transfer
	 */
	szaddr->scs_mode = SCS_PARCK;
	/*
	 * Turn off the SCS_INI_ENOUT driver
	 */
	inicmd_tmp &= ~SCS_INI_ENOUT;
	szaddr->scs_inicmd = inicmd_tmp;
	/*
	 * Read the bus phase, set the phase to match.
	 */
	szaddr->scs_tarcmd = ((szaddr->scs_curstat & SCS_PHA_MSK) >> 2);
	switch (szaddr->scs_curstat & SCS_PHA_MSK) {

	case SCS_DATAO:
#ifdef SZDEBUG
	    printd2("scsistart: SCS_DATAO:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_DATAO_PHA;
	    if ( (sc->sz_opcode != SZ_WRITE) &&
		 (sc->sz_opcode != SZ_WRITE_10) ) {
		byteptr = (u_char *)&sc->sc_dat[0];
		switch(sc->sz_opcode) {
		case SZ_MODSEL:
		    cmdcnt = sc->sz_modsel.pll;
	    	    if(sc->sc_rzspecial[targid]) {
	        	msp = (struct mode_sel_sns_params *)
						sc->sc_rzparams[targid];
                	byteptr = (u_char *)sc->sc_rzaddr[targid];
                	datacnt = msp->msp_length;
	    	    }
		    break;

		case SZ_REASSIGN:
            	    rbp = (struct reassign_params *)sc->sc_rzparams[targid];
                    byteptr = (u_char *)sc->sc_rzparams[targid];
                    datacnt = ((rbp->rp_header.defect_len0 << 0) & 0x00ff) +
    		      	      ((rbp->rp_header.defect_len1 << 8) & 0xff00) + 4;
            	    break;

		case SZ_FORMAT:
            	    dd = (struct defect_descriptors *)sc->sc_rzaddr[targid];
            	    byteptr = (u_char *)sc->sc_rzaddr[targid];
            	    datacnt = ((dd->dd_header.fu_hdr.defect_len0<<0)&0x00ff) +
    		      	      ((dd->dd_header.fu_hdr.defect_len1<<8)&0xff00) + 4;
	    	    break;
		default:
#ifdef SZDEBUG
		    printd ("szintr: SCS_DATAO: unknown cmd 0x%x\n",
			    sc->sz_opcode);
#endif SZDEBUG
		    goto abort;
		    break;
		}
	        /*
                 * Setup softc structure entries for special SCSI DISK
	         * commands that do dma. (FORMAT UNIT), (REASSIGN BLOCK)
	         * and (MODE SELECT).
	         */
        	if(sc->sc_rzspecial[targid] &&
			(sc->sc_curcmd[targid] == sc->sc_actcmd[targid])) {
	            sc->sc_b_bcount[targid] = datacnt;
	            sc->sc_bpcount[targid] = datacnt;
	            sc->sc_bufp[targid] = (char *)byteptr;
	            sc->sc_xfercnt[targid] = 0;
		    goto SETUP1_DMA;
                }
		inicmd_tmp = SCS_INI_ENOUT;
		szaddr->scs_inicmd = inicmd_tmp;
		/*
		 * Send the command packet
		 */
#ifdef SZDEBUG
		printd2("Data Output Packet: \n");		/* 1 of 3 */
#endif SZDEBUG
		for ( ; (cmdcnt > 0); cmdcnt--) {
		    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x80, 0, flags);
			goto abort;
		    }
#ifdef SZDEBUG
		    printd2 (" %x", *byteptr);			/* 2 of 3 */
#endif SZDEBUG
		    szaddr->scs_outdata = *byteptr++;
		    inicmd_tmp |= SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x81, 0, flags);
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		}
#ifdef SZDEBUG
		printd2 ("\n");					/* 3 of 3 */
#endif SZDEBUG
		break;
	    }
SETUP1_DMA:
	    if (bp == 0) {	/* called from szprobe */
		printf("scsistart: SCS_DATAO: %s\n",
		    "data transfer command from szprobe");
		goto abort;	/* cannot happen */
	    }
	    /*
	     * Set up to transfer data to the target via DMA.
	     *
	     * If initial DMA setup, then:
	     *	count - full count for the xfer segment
	     *	addr  - offset of 0 in target's 128KB buffer slot
	     *	bcopy - copy data from user to 128 KB buffer
	     *
	     * If continuing DMA after disconnect, then:
	     *	count - from save data pointer count
	     *	addr  - use saved count to calculate buffer offset
	     *  bcopy - no bcopy needed
	     */

	    /*
	     * 5380 prefetching an extra byte burned us!
	     * On an odd byte count (like 8193) the tape takes 8912
	     * bytes then disconnects, but the 5380 takes on extra
	     * byte, so the saved count is 0 instead of -1.
	     * This hack compensates for the 5380 hack.
	     * NOTE:
	     *	This case can't happen in scsistart, but the code
	     *	stays so it matches the code in szintr - SCS_DATAO:.
	     */
	    if ((sc->sc_devtyp[targid] & SZ_TAPE) &&
		(sc->sc_szflags[targid] & SZ_WAS_DISCON) &&
		(sc->sc_savcnt[targid] == 0)) {
		    sc->sc_savcnt[targid] = -1;
	    }

	    /*
	     * Set up count, address, and bcopy data (if needed).
	     */
	    if ((sc->sc_szflags[targid] & SZ_WAS_DISCON) &&
		(sc->sc_savcnt[targid] != 0)) {
		/*
		 * This case can't happen in scsistart, but the code
		 * stays so it matches the code in szintr - SCS_DATAO:.
		 */
		/*
		 * This is a quick way to subtract the the number of
		 * bytes not transfered before the disconnect, from
		 * the total number of bytes to transfer.  The result
		 * is the number of bytes that need to be transfered.
		 * sc->sc_savcnt is in two's complement form.  The
		 * number of bytes to be transfered is used as an offset
		 * and added to bufp to get the address of where to start
		 * transfering data from.
		 */
		szaddr->scd_cnt = sc->sc_savcnt[targid];
		sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;
		bufp = (char *)(sc->sc_bpcount[targid] + sc->sc_savcnt[targid]);
	    }
	    else {
		szaddr->scd_cnt = -(sc->sc_bpcount[targid]);
		stv = sc->sc_rambuff + sc->sc_dboff[targid];
		bufp = sc->sc_bufp[targid] + sc->sc_xfercnt[targid];
/*		bcopy (bufp, stv, sc->sc_b_bcount[targid]);	*/
		blkcpy (bufp, stv, sc->sc_b_bcount[targid]);
		bufp = 0;
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
    	     */
	    if (sz_no_spin3)
		szaddr->scs_mode = (SCS_DMA | SCS_PARCK);
	    else
		szaddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK);
	    inicmd_tmp = SCS_INI_ENOUT;
	    szaddr->scs_inicmd = inicmd_tmp;
	    /*
	     * Set the starting address in the 128K buffer.
	     */
	    szaddr->scd_adr = sc->sc_dboff[targid] + (long)bufp;
	    /*
	     * Next four lines of code provide a 1.2 Usec delay.
	     * This delay prevents the SCSI/SCSI controller
	     * hanging when a DMA is started on one bus while
	     * a DMA is in progress on the other bus.
	     * Hardware engineering says we need four reads!
	     */
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    szaddr->scd_dir = SCD_DMA_OUT;
	    /*
	     * Start the DMA transfer
	     */
	    szaddr->scs_dmasend = 1;
	    sc->sc_szflags[targid] |= SZ_DID_DMA;
	    return(SZ_IP);
	    break;

	case SCS_DATAI:
#ifdef SZDEBUG
	    printd2("scsistart: SCS_DATAI:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_DATAI_PHA;
	    if ( (sc->sz_opcode != SZ_READ) &&
		 (sc->sz_opcode != SZ_READ_10) ) {
		byteptr = (u_char *)&sc->sc_sns[targid];
		switch (sc->sz_opcode) {
		    case SZ_RQSNS:
			num_expected = sc->sz_rqsns.alclen;
			break;

		    case SZ_INQ:
			num_expected = SZ_INQ_MAXLEN;
			byteptr = (u_char *)&sc->sz_dat[targid];
	    	        if(sc->sc_rzspecial[targid]) {
                	    byteptr = (u_char *)sc->sc_rzaddr[targid];
			    datacnt = num_expected;
			}
			break;

		    case SZ_RDCAP:
			num_expected = SZ_RDCAP_LEN;
			byteptr = (u_char *)&sc->sz_dat[targid];
			break;

		    case SZ_MODSNS:
			num_expected = sc->sz_modsns.alclen;
			byteptr = (u_char *)&sc->sz_dat[targid];
	    		if(sc->sc_rzspecial[targid]) {
	        	    msp = (struct mode_sel_sns_params *)
						sc->sc_rzparams[targid];
                	    byteptr = (u_char *)sc->sc_rzaddr[targid];
                	    datacnt = msp->msp_length;
	    	    	}
			break;
		    case SZ_RDD:
            	        rdp = (struct read_defect_params *)
						sc->sc_rzparams[targid];
                        byteptr = (u_char *)sc->sc_rzaddr[targid];
            		datacnt = rdp->rdp_alclen;
            		break;

		    case SZ_READL:
		    case SZ_WRITEL:
			iox = (struct io_uxfer *)sc->sc_rzparams[targid];
			byteptr = (u_char *)sc->sc_rzaddr[targid];
			datacnt = iox->io_cnt;
			break;

		    case SZ_RECDIAG:
			num_expected = SZ_RECDIAG_LEN;
			byteptr = (u_char *)&sc->sz_dat[targid];
			break;

		    default:
#ifdef SZDEBUG
			printd("scsistart: SCS_DATAI: unexpected cmd = %x\n",
				sc->sz_opcode);
#endif SZDEBUG
		    break;
		}
	        /*
                 * Setup softc structure entries for special SCSI
	         * DISK commands that do dma. (READ DEFECT DATA),
	         * (INQUIRY) and (MODE SENSE).
	         */
        	if(sc->sc_rzspecial[targid] &&
			(sc->sc_curcmd[targid] == sc->sc_actcmd[targid])) {
	            sc->sc_b_bcount[targid] = datacnt;
	            sc->sc_bpcount[targid] = datacnt;
	            sc->sc_bufp[targid] = (char *)byteptr;
	            sc->sc_xfercnt[targid] = 0;
		    goto SETUP2_DMA;
                }
		inicmd_tmp = 0;
		szaddr->scs_inicmd = inicmd_tmp;
		/*
		 * Receive the data packet
		 */
#ifdef SZDEBUG
		printd2("Data In Packet:");			/* 1 of 3 */
#endif SZDEBUG
		/*
		 * Receive data until maximum number of bytes
		 * expected is reached or the phase changes.
		 */
		szaddr->scs_mode |= SCS_PARCK;
		for (num_received = 0; (num_expected > 0); num_expected--) {
		    for (retval = 0; retval < 10000; retval++) {
			/* TODO: should check for loss of BSY */
			if ((szaddr->scs_status & SCS_MATCH) == 0)
			    break;
			if (szaddr->scs_curstat & SCS_REQ)
			    break;
			DELAY(100);
		    }
		    if ((szaddr->scs_status & SCS_MATCH) == 0)
			break;
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x90, 0, flags);
			goto abort;
		    }
		    *byteptr++ = szaddr->scs_curdata;
		    num_received++;
#ifdef SZDEBUG
		    printd2(" %x", szaddr->scs_curdata);	/* 2 of 3 */
#endif SZDEBUG
		    inicmd_tmp |= SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
			flags = SZ_HARDERR;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0x91, 0, flags);
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		}
#ifdef SZDEBUG
		printd2("\n");					/* 3 of 3 */
#endif SZDEBUG
		/* TODO: need num_minimum for each data type */
		if (num_received < 5) {
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x92, 0, flags);
		}
		break;
	    }
SETUP2_DMA:
	    /*
	     * Start of DMA code.
	     *
	     * Set up count and address.
	     */
	    if (bp == 0) {	/* called from szprobe */
		mprintf("scsistart: SCS_DATAI: data xfer cmd from szprobe\n");
		goto abort;	/* cannot happen */
	    }
	    if ((sc->sc_szflags[targid] & SZ_WAS_DISCON) &&
		(sc->sc_savcnt[targid] != 0)) {
		/*
		 * This case can't happen in scsistart, but the code
		 * stays so it matches the code in szintr - SCS_DATAI:.
		 */
		szaddr->scd_cnt = sc->sc_savcnt[targid];
		bufp = (char *)(sc->sc_bpcount[targid] + sc->sc_savcnt[targid]);
#ifdef SZDEBUG
		printd1("scsistart#2: scd_cnt = 0x%x, sc_savcnt = 0x%x\n",
			szaddr->scd_cnt, sc->sc_savcnt[targid]);
#endif SZDEBUG
	    }
	    else {
		szaddr->scd_cnt = -(sc->sc_bpcount[targid]);
		bufp = 0;
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
	     */
	    if (sz_no_spin3)
		szaddr->scs_mode = (SCS_DMA | SCS_PARCK);
	    else
		szaddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK);
	    szaddr->scs_inicmd = 0;
	    /*
	     * Set the starting address in the 128K buffer.
	     */
	    szaddr->scd_adr = sc->sc_dboff[targid] + (long)bufp;
	    /*
	     * Next four lines of code provide a 1.2 Usec delay.
	     * This delay prevents the SCSI/SCSI controller
	     * hanging when a DMA is started on one bus while
	     * a DMA is in progress on the other bus.
	     * Hardware engineering says we need four reads!
	     */
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    i = sziaddr->nb_diagtime;
	    szaddr->scd_dir = SCD_DMA_IN;
	    /*
	     * Start the DMA transfer
	     */
	    szaddr->scs_dmaircv = 1;
	    sc->sc_szflags[targid] |= SZ_DID_DMA;
	    return(SZ_IP);
	    break;

	case SCS_CMD:
#ifdef SZDEBUG
	    printd2("scsistart: SCS_CMD:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_CMD_PHA;
	    /*
	     * clear savecnt and status
	     */
	    sc->sc_savcnt[targid] = 0;
	    sc->sc_status[targid] = 0xff;
#ifdef	SZ_DSP_DEBUG
	    sc->sc_szflags[targid] &= ~SZ_DID_STATUS;
#endif	SZ_DSP_DEBUG
	    inicmd_tmp = SCS_INI_ENOUT;
	    szaddr->scs_inicmd = inicmd_tmp;
	    byteptr = (u_char *)&sc->sz_command;
	    cmd_type = *byteptr;
	    nospin = 0;
	    if (sz_no_spin1 && (bp != 0)) {
		switch(cmd_type) {
		case SZ_TUR:
		case SZ_RQSNS:
		case SZ_INQ:
		case SZ_MODSEL:
		case SZ_MODSNS:
		case SZ_SSLU:		/* TODO: not sure about this one! */
					/* cmd is really SSUNIT or UNLOAD */
		case SZ_RECDIAG:
		    nospin = 0;
		    break;
		default:
		    nospin = 1;
		    break;
		}
	    }
	    /*
	     * Send the command packet
	     */
	    i = sz_cdb_length (cmd_type, targid);
#ifdef SZDEBUG
	    if (i == SZ_RDCAP_CMD_LEN) {
		printd2("cmd pkt: %x %x %x %x %x %x %x %x %x %x\n",
		    *byteptr, *(byteptr + 1), *(byteptr + 2), *(byteptr + 3),
		    *(byteptr + 4), *(byteptr + 5), *(byteptr + 6),
		    *(byteptr + 7), *(byteptr + 8), *(byteptr + 9));
	    }
	    else {
		printd2("cmd pkt: %x %x %x %x %x %x\n",
		    *byteptr, *(byteptr + 1), *(byteptr + 2), *(byteptr + 3),
		    *(byteptr + 4), *(byteptr + 5));
	    }
#endif SZDEBUG
	    for (cmdcnt = i; (cmdcnt > 0); cmdcnt--) {
		SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
#ifdef	ELDEBUG
		if ((cntlr == 0) && (targid == sz_eldb_buserra0)) {
		    retval = 10000;
		    sz_eldb_buserra0 = -1;
		}
#endif	ELDEBUG
		if (retval >= 10000) {
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0xa0, 0, flags);
		    goto abort;
		}
		szaddr->scs_outdata = *byteptr++;
		inicmd_tmp |= SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
		SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		if (retval >= 10000) {
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0xa1, 0, flags);
		    goto abort;
		}
		/*
		 * If nospin set, take an interrupt instead
		 * of spinning waiting for phase change.
		 */
		if (nospin && (cmdcnt == 1)) {
		    szaddr->scs_tarcmd = 4;	/* RSVD phase */
		    szaddr->scs_mode |= SCS_DMA;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
	    }
	    if ( (cmd_type == SZ_WRITE) || (cmd_type == SZ_READ) ||
		 (cmd_type == SZ_WRITE_10) || (cmd_type == SZ_READ_10) ) {
		if (sc->sc_dkn[targid] >= 0) {
		    dk_busy |= 1 << sc->sc_dkn[targid];
		    dk_xfer[sc->sc_dkn[targid]]++;
		    dk_wds[sc->sc_dkn[targid]] += sc->sc_bpcount[targid] >> 6;
		}
	    }
	    if (nospin)
		return(SZ_IP);
	    break;

	case SCS_STATUS:
#ifdef SZDEBUG
	    printd2("scsistart: SCS_STATUS:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_STATUS_PHA;
#ifdef	SZ_DSP_DEBUG
	    sc->sc_szflags[targid] |= SZ_DID_STATUS;
#endif	SZ_DSP_DEBUG
	    SZWAIT_UNTIL(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
	    if (retval >= 10000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0xb0, 0, flags);
		goto abort;
	    }
	    szaddr->scs_mode |= SCS_PARCK;
	    sc->sc_status[targid] = szaddr->scs_curdata;
	    /* Save the status byte for the error log */
	    if (sc->sc_curcmd[targid] == sc->sc_actcmd[targid])
		sc->sc_statlog[targid] = sc->sc_status[targid];
	    inicmd_tmp = SCS_INI_ACK;
	    szaddr->scs_inicmd = inicmd_tmp;
#ifdef SZDEBUG
	    printd2("scsistart: SCS_STATUS: ppha = 0x%x, status = 0x%x, scd_cnt = 0x%x\n",
		sc->sc_prevpha, sc->sc_status[targid], szaddr->scd_cnt);
#endif SZDEBUG
	    SZWAIT_WHILE(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),50000,retval);
	    if (retval >= 50000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0xb1, 0, flags);
		goto abort;
	    }
	    inicmd_tmp = 0;
	    szaddr->scs_inicmd = inicmd_tmp;
	    /*
	     * Check the status
	     */
	    if (sc->sc_status[targid] == SZ_BUSY) {
		sc->sc_szflags[targid] |= SZ_BUSYTARG;
		break;
	    }
	    if (sc->sc_status[targid] != SZ_GOOD) {
		sc->sc_szflags[targid] |= SZ_NEED_SENSE;
	    }
	    break;

	case SCS_MESSO:
#ifdef SZDEBUG
	    printd2("scsistart: SCS_MESSO:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_MESSO_PHA;
	    /*
	     * Send the Identify message.
	     * Unless we got here due to a sync data xfer
	     * request. In that case send message reject.
	     */
	    if(szaddr->scs_curstat & SCS_REQ) {
		if (exmsg_sdtr) {
		    szaddr->scs_outdata = SZ_MSGREJ;
		    exmsg_sdtr = 0;
		    exmsg_byte = 0;
		}
		else {
		    /*
		     * If called from szprobe, cannot allow
		     * target to disconnect.
		     */
		    if (bp == 0)
			szaddr->scs_outdata = SZ_ID_NODIS;
		    else
			szaddr->scs_outdata = SZ_ID_DIS;
		}
		inicmd_tmp |= (SCS_INI_ENOUT | SCS_INI_ATN);
		szaddr->scs_inicmd = inicmd_tmp;
		DELAY(2);
		inicmd_tmp &= ~SCS_INI_ATN;
		szaddr->scs_inicmd = inicmd_tmp;
		inicmd_tmp |= SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
		SZWAIT_WHILE((szaddr->scs_curstat & SCS_REQ),100000,retval);
		if(retval >= 100000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0xe0, 0, flags);
		    goto abort;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;

		if (szaddr->scs_status & SCS_PARERR) {
		    flags = SZ_HARDERR | SZ_LOGREGS;
		    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 6, 0, flags);
		    goto abort;
		}
	    }
	    break;

	case SCS_MESSI:
#ifdef SZDEBUG
	    printd2("scsistart: SCS_MESSI:\n");
#endif SZDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = SZ_MESSI_PHA;
	    /*
	     * If exmsg_byte is nonzero we are receiving an
	     * extended message. Save the byte as data.
	     */
	    szaddr->scs_mode |= SCS_PARCK;
	    if (exmsg_byte)
		exmsg_data = szaddr->scs_curdata;
	    else
		sc->sc_message[targid] = szaddr->scs_curdata;
	    inicmd_tmp |= SCS_INI_ACK;
	    szaddr->scs_inicmd = inicmd_tmp;
	    SZWAIT_WHILE(((szaddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
	    if(retval >= 10000) {
		flags = SZ_HARDERR;
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0xf1, 0, flags);
		goto abort;
	    }
	    /*
	     * Set MONBSY to generate an interrupt when the target
	     * goes bus free after the command complete phase.
	     * This "fake disconnect" is used to start the next command.
	     * Set MONBSY here (before ACK) to gaurantee BSYERR interrupt.
	     */
	    if (sc->sc_message[targid] == SZ_CMDCPT) {
		if (sc->sc_szflags[targid] & SZ_BUSYTARG)
		    szaddr->scs_mode |= SCS_MONBSY;
	    }
	    /*
	     * If extended message don't drop ACK yet.
	     * We may have to assert ATTN later.
	     */
	    if (sc->sc_message[targid] != SZ_EXTMSG) {
		inicmd_tmp &= ~SCS_INI_ACK;
		szaddr->scs_inicmd = inicmd_tmp;
	    }
#ifdef	ELDEBUG
	    if ((cntlr == 0) && (targid == sz_eldb_buserrf3)) {
		sc->sc_message[targid] = SZ_IDE;
		sz_eldb_buserrf3 = -1;
	    }
#endif	ELDEBUG
	    switch (sc->sc_message[targid]) {
		case SZ_EXTMSG:
#ifdef	SZDEBUG
		    printd("scsistart: SZ_EXTMSG\n");
#endif	SZDEBUG
		    /*
		     * Extended message. If its a sync data xfer message:
		     * receive message, raise attn, send message reject
		     * to force async mode. Otherwise: receive message
		     * and hope the device terminates the MESSI phase.
		     */
		    if (exmsg_byte == 0) {		/* ext msg start */
			exmsg_byte = 1;
		    }
		    else if (exmsg_byte == 1) {		/* ext msg length */
			exmsg_len = exmsg_data;
			exmsg_cnt = 0;
			exmsg_byte = 2;
		    }
		    else if (exmsg_byte == 2) {		/* ext msg type */
			exmsg_byte = 3;
			if (exmsg_data == SZ_SYNC_XFER)
			    exmsg_sdtr = 1;		/* sync data xfer req */
			else
			    exmsg_sdtr = 0;		/* other ext msg type */
			exmsg_cnt++;
		    }
		    else {				/* rcv rest of extmsg */
			exmsg_cnt++;			/* discard each byte */
			exmsg_byte++;
			if (exmsg_cnt >= exmsg_len) {	/* last exmsg byte */
			    if (exmsg_sdtr) {
				/* if ext msg is sync data xfer req, */
				/* assert attn, target should goto msg out */
				inicmd_tmp |= SCS_INI_ATN;
				szaddr->scs_inicmd = inicmd_tmp;
			    }
			    exmsg_byte = 0;		/* end of ext msg */
			}
		    }
		    /* drop ACK to complete handshake */
		    inicmd_tmp &= ~SCS_INI_ACK;
		    szaddr->scs_inicmd = inicmd_tmp;
		    break;

		case SZ_CMDCPT:
#ifdef	SZDEBUG
		    printd("scsistart: SZ_CMDCPT:\n");
#endif	SZDEBUG
#ifdef	SZ_DSP_DEBUG
		    if ((sc->sc_szflags[targid] & SZ_DID_STATUS) == 0) {
		     if (sz_nsp_print)
		     {
		     printf("scsistart: (bus=%d ID=%d cmd=0x%x acmd=0x%x) %s\n",
			    cntlr, targid, sc->sc_curcmd[targid],
			    sc->sc_actcmd[targid],
			    "NO status phase");
		     }
		    }
		    else
			sc->sc_szflags[targid] &= ~SZ_DID_STATUS;
#endif	SZ_DSP_DEBUG
		    if (bp && (sc->sc_szflags[targid] & SZ_BUSYTARG)) {
			return(SZ_TARGBUSY);
		    }
		    /*
		     * The command has completed, release the bus.
		     */
		    inicmd_tmp = 0;
		    szaddr->scs_inicmd = 0;
		    szaddr->scs_mode = SCS_PARCK;
		    szaddr->scs_tarcmd = 0;
		    /*
		     * clear SCS_PARERR, SCS_INTREQ, and SCS_BSYERR 
		     * in mode reg, and return 
		     */
		    sc->sc_active = 0;
		    /*
		     * Wait 10 MS for device to drop BSY after command
		     * complete. Log an error if BSY doesn't drop.
		     * If just BSY true, device is holding BSY.
		     * If SEL true, some other device grabbed bus.
		     * NOTE: check 3 times is for bus settle delay (400 ns)
		     * We do normal command completion and assume timer1
		     * will reset the bus.
		     * NOTE: TZ30 held BSY up to 800 Usec.
		     * TODO: verify TZ30 fixed in firmware rev ?
		     */
		    for (i = 0; i < 1000; i++) {
			if (((szaddr->scs_curstat & (SCS_BSY|SCS_SEL)) == 0) &&
			    ((szaddr->scs_curstat & (SCS_BSY|SCS_SEL)) == 0) &&
			    ((szaddr->scs_curstat & (SCS_BSY|SCS_SEL)) == 0)) {
				break;	/* bus is free */
			}
			/* bus still busy */
			if ((szaddr->scs_curstat & SCS_SEL) &&
			    (szaddr->scs_curstat & SCS_SEL) &&
			    (szaddr->scs_curstat & SCS_SEL)) {
				break; /* sel true, somebody else grabbed bus */
			}
			DELAY(10);
		    }
		    if (i >= 1000) {	/* busy way too long! */
			flags = SZ_HARDERR |SZ_LOGREGS;
			scsi_logerr(sc, 0, targid, SZ_ET_BUSERR,0xf4, 0, flags);
		    }
#ifdef	SZ_BSYCC_STATS
		    else {
			if (i > sz_ss_bsycc[unit])
			    sz_ss_bsycc[unit] = i;
		    }
#endif	SZ_BSYCC_STATS
		    szaddr->scs_selena = sc->sc_sysid;
		    sc->sc_scs_selena = sc->sc_sysid;
		    DELAY(10);
		/* TODO: clear SZ_DID_DMA in szflags? */
		    sc->sc_selstat[targid] = SZ_IDLE;
		    sc->sc_fstate = 0;
		    /* Assumes one command at a time for each target */
		    if (sc->sc_dkn[targid] >= 0)
			dk_busy &= ~(1 << sc->sc_dkn[targid]);

		    complete++;
		    if (sc->sc_szflags[targid] & SZ_BUSYTARG)
			return(SZ_TARGBUSY);
		    if (sc->sc_status[targid] == SZ_GOOD) {
			return(SZ_SUCCESS);
		    }
		    else {
			return(SZ_RET_ERR);
		    }
		    break;	/* NOTREACHED */
			
		case SZ_SDP:
		    sz_sdp[unit]++;
		    if (bp == 0)	/* called from szprobe */
			break;		/* ignore save data pointer */
		    if (sc->sc_prevpha == SZ_DATAO_PHA) {
			/*
			 * The scd_cnt register is 32 bits long, but
			 * bits 17 - 31 always read as zeros. This means
			 * we have to fake sign extension on scd_cnt reads.
			 * If bit 16 set (negative count) force bits 17-31 set.
			 */
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid] = (szaddr->scd_cnt|0xfffe0000)-1;
			else
			    sc->sc_savcnt[targid] = szaddr->scd_cnt;
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
		    }
/* Fix read defect data bug. Defect list appeared trashed */
/*		    else {				*/
/*			sc->sc_savcnt[targid] = 0;	*/
/*		    }					*/

		    if (sc->sc_prevpha == SZ_DATAI_PHA) {
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid] = (szaddr->scd_cnt|0xfffe0000);
			else
			    sc->sc_savcnt[targid] = szaddr->scd_cnt;
		    }
#ifdef SZDEBUG
		    printd1("scsistart: SZ_SDP, scd_cnt = 0x%x\n",
			sc->sc_savcnt[targid]);

#endif SZDEBUG
		    break;

		case SZ_DISCON:
#ifdef SZDEBUG
		    printd2("scsistart: disconnect message\n");
#endif SZDEBUG
		    if (sz_do_isdp == 0) {	/* don't do implied SDP */
			szaddr->scs_mode = (SCS_MONBSY | SCS_PARCK);
			return;
			break;	/* NOTREACHED */
		    }
		    /*
		     * Implied save data pointer. Only if target disconnects
		     * without sending a save data pointer message. Previous
		     * phase will be a data in/out phase in this case.
		     */
		    if (sc->sc_prevpha == SZ_DATAO_PHA) {
			sz_isdp[unit]++;
			/*
			 * The scd_cnt register is 32 bits long, but
			 * bits 17 - 31 always read as zeros. This means
			 * we have to fake sign extension on scd_cnt reads.
			 * If bit 16 set (negative count) force bits 17-31 set.
			 */
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid] = (szaddr->scd_cnt|0xfffe0000)-1;
			else
			    sc->sc_savcnt[targid] = szaddr->scd_cnt;
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
		    }
		    if (sc->sc_prevpha == SZ_DATAI_PHA) {
			sz_isdp[unit]++;
			sc->sc_szflags[targid] |= SZ_WAS_DISCON;
			if(szaddr->scd_cnt & 0x10000)
			    sc->sc_savcnt[targid]= (szaddr->scd_cnt|0xfffe0000);
			else
			    sc->sc_savcnt[targid]= szaddr->scd_cnt;
		    }
		    szaddr->scs_mode = (SCS_MONBSY | SCS_PARCK);
		    return(SZ_IP);
		    break;	/* NOTREACHED */
			
		case SZ_ID_NODIS:
		    /*
		     * A target should never send this message.
		     * Bit 6 only set by the initiator.
		     * Should be an error, but never seen one and
		     * doubt it would cause a problem anyway.
		     */
#ifdef SZDEBUG
		    printd2("scsistart: identify message\n");
#endif SZDEBUG
		    break;

		case SZ_ID_DIS:
#ifdef SZDEBUG
		    printd2("scsistart: identify with disconnect message\n");
#endif SZDEBUG
		    break;

		case SZ_RDP:
		    sz_rdp[unit]++;
#ifdef	SZDEBUG
		    printd("scsistart: SZ_RDP:\n");
#endif	SZDEBUG
		    break;

		/*
		 * These messages are unsupported or should never
		 * be received (only sent) by the initiator.
		 */
		case SZ_IDE:		/* Initiator Detected Error */
		case SZ_ABT:		/* Abort */
		case SZ_MSGREJ:		/* Message (should handle this one) */
		case SZ_NOP:		/* No Operation */
		case SZ_MSGPE:		/* Message Parity error */
		case SZ_LNKCMP:		/* Linked Command Complete */
		case SZ_LNKCMPF:	/* Linked Command Complete with flag */
		case SZ_DEVRST:		/* Bus Device Reset */
		    /* FALLTHROUGH */
		default:
		    /*
		     * Unknown message or identify with non zero sub-unit.
		     */

		    flags = SZ_HARDERR | SZ_LOGMSG;
		    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0xf3, 0, flags);
		    /*
		     * We abort! Should assert ATTN, hope target goes to
		     * message out, and send an abort message, but
		     * its just too messy and this error crashes the
		     * world every time it happens (so far).
		     */
		    goto abort;
	    }
	    break;

	default:
	    ;
#ifdef SZDEBUG
	    mprintf("scsistart: unknown phase = 0x%x\n",
		szaddr->scs_status & SCS_PHA_MSK);
#endif SZDEBUG
	}
    } /* end of do loop */
    while (complete == 0);
    return;	/* NOTREACHED */

abort:
    sc->sc_sel_retry[targid] = 0;			/* clear select retry count */
    abort_parck = (szaddr->scs_mode & SCS_PARCK); /* remember PARCK state */
    szaddr->scs_mode |= SCS_PARCK;
    /*
     * Return command abort status due to some fatal error,
     * such as, device failed to select, or arbitration failed
     * to start. The caller must decide how to handle aborts.
     */
    /*
     * TODO:
     *		What other info to log?
     *		Should really retry command?????
     */
    exmsg_byte = 0;
    exmsg_sdtr = 0;
    sc->sc_active = 0;
    sc->sc_selstat[targid] = SZ_IDLE;
    if (bp != 0) { 	/* don't log abort if called from szprobe */
	flags = SZ_HARDERR | SZ_LOGCMD;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 3, 0, flags);
    }
    sc->sc_szflags[targid] |= SZ_ENCR_ERR;
    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
    /*
     * Turn on select enable, just in case
     * we aborted with it off (in select code).
     */
    szaddr->scs_selena = sc->sc_sysid;
    sc->sc_scs_selena = sc->sc_sysid;
    if (bp == 0)	/* called from szprobe (it handles hung bus) */
	return(SZ_RET_ABORT);
    if (sz_do_reset == 0)
	return(SZ_RET_ABORT);
    /*
     * If the bus is wedged (BSY and/or SEL stuck) or
     * force_reset is nonzero
     * reset the bus, controller, and the rest of the world.
     * We allow about 1 MS for the bus to free up.
     * If SEL & I/O asserted bus not hung (reselect waiting).
     * NOTE: it is possible for BSY to be true with SEL false,
     *       but only for a very short time (a few Usec at most).
     */
/* TODO: need to turn off ENOUT? */
    /* we can get here without detecting reset */
    if (force_reset == 0) {
	if ((szaddr->scs_curstat & SCS_RST) || (abort_parck == 0)) {
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 8, 0, SZ_HARDERR);
	    force_reset = 1;
	}
    }
    wedged = 1;
    for (retval = 0; retval < 1000; retval++) {
	if (force_reset)
	    break;
	if (((szaddr->scs_curstat & SCS_BSY) == 0) &&
	    ((szaddr->scs_curstat & SCS_BSY) == 0) &&
	    ((szaddr->scs_curstat & SCS_BSY) == 0)) {
		if ((szaddr->scs_curstat&(SCS_SEL|SCS_IO))==(SCS_SEL|SCS_IO)) {
			wedged = 0;
			break;
		}
		if (((szaddr->scs_curstat & SCS_SEL) == 0) &&
		    ((szaddr->scs_curstat & SCS_SEL) == 0) &&
		    ((szaddr->scs_curstat & SCS_SEL) == 0)) {
			wedged = 0;
			break;
		}
	}
	DELAY(1);
    }
#ifdef	ELDEBUG
    if (sz_eldb_rstbus2 >= 0) {
	wedged = 1;
	sz_eldb_rstbus2 = -1;
    }
#endif	ELDEBUG
    if (wedged) {
	flags = SZ_HARDERR | SZ_LOGREGS;
	scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 2, 0, flags);
	sz_reset(cntlr);
	return(SZ_RET_RESET);
    }
    return(SZ_RET_ABORT);
}


/*
 *
 * Name:		sz_reset	-reset SCSI bus and controller
 *
 * Abstract:		This routine resets the SCSI bus and controller.
 *			It delays sz_wait_for_devices seconds after the
 *			reset to allow devices time to complete their
 *			self tests and become ready. Calls sc_start to
 *			retry commands in progress when bus was reset.
 *
 * Inputs:
 *
 * cntlr		SCSI bus controller number (0 = bus A, 1 = bus B).
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 * Side Effects:
 *
 *			SCSI bus is reset.
 *			Reset In Progress (sc_rip) flag locks out szintr
 *			and sz_start routines, and some timers.
 *			sz_start called.
 *
 */

sz_reset(cntlr)
int cntlr;
{
	register struct sz_regs *szaddr = (struct sz_regs *)szmem + cntlr;
	register struct nb_regs *sziaddr = (struct nb_regs *)nexus;
	register struct sz_softc *sc = (struct sz_softc *)&sz_softc[cntlr];
	register struct buf *dp;
	int i;
	int targid, unit;

	if (sc->sc_rip == 0) {
	    /*
	     * Reset not already in progress. Reset the controller
	     * and the SCSI bus. Start a timer to wait for devices
	     * to become ready after the reset. Zap any disconnect timers.
	     * TODO: rq disk cmds, error for tape cmds!
	     */
	    for (targid = 0; targid < NDPS; targid++) {
		if ((1 << targid) == sc->sc_sysid)
		    continue;	/* skip initiator */
		unit = sc->sc_unit[targid];
		if (sc->sc_szflags[targid] & SZ_TIMERON) {
		    untimeout(sz_timer2, (caddr_t)unit);
		    sc->sc_szflags[targid] &= ~SZ_TIMERON;
		}
		dp = (struct buf *)&szutab[unit];
/* TODO: zap tape commands (except rewind?) NIY */
		if (dp->b_active) {
		    dp->b_active = 0;
		    sc->sc_xstate[targid] = SZ_NEXT;
		    sc->sc_xevent[targid] = SZ_BEGIN;
		}
		sc->sc_selstat[targid] = SZ_IDLE;
		sc->sc_szflags[targid] = SZ_NORMAL;
		sc->sc_flags[targid] &= ~DEV_HARDERR;
	    }
	    sc->sc_active = 0;
	    sziaddr->nb_int_msk &= ~(SCS_IC_BIT >> cntlr);
	    szaddr->scs_inicmd = SCS_INI_RST;
	    DELAY(sz_reset_width); /* hold bus reset for 50 Usec (min = 25) */
	    szaddr->scs_inicmd = 0;
	    szaddr->scs_mode = SCS_PARCK;
	    i = szaddr->scs_reset;
	    sziaddr->nb_int_reqclr = (SCS_IC_BIT >> cntlr);
	    sc->sc_rip = 1;		/* hold off all driver activity */
	    timeout(sz_reset, cntlr, (sz_wait_for_devices * hz));
	}
	else {
	    /*
	     * Reset in progress. Re-enable interrupts and
	     * restart driver activity.
	     */
	    i = szaddr->scs_reset;
	    i = szaddr->scs_reset;
	    i = szaddr->scs_reset;
	    sziaddr->nb_int_reqclr = (SCS_IC_BIT >> cntlr);
	    sziaddr->nb_int_msk |= (SCS_IC_BIT >> cntlr);
	    sc->sc_rip = 0;
	    /*
	     * Subtract one from the sc_lastid, so the current
	     * target is the one which gets restarted.
	     */
	    if (sc->sc_lastid == 0)
		sc->sc_lastid = 7;
	    else
		sc->sc_lastid -= 1;
	    /*
	     * Make sure we don't set sc_lastid == initiator's ID.
	     * This will hang sz_start in an endless loop.
	     */
	    if ((1 << sc->sc_lastid) == sc->sc_sysid)
		sc->sc_lastid -= 1;
	    if (sc->sc_lastid < 0)
		sc->sc_lastid = 7;
	    sz_start(sc, -1);
	}
}


/*
 *
 * Name:		szreset		-Reset SCSI busses (crash)
 *
 * Abstract:		This routine is called from dumpsys() in machdep.c
 *			to reset all SCSI busses prior to taking a crash
 *			dump. The VMB boot driver requires this reset to
 *			stop any DMA that might be in progress at the
 *			time of the crash. We delay 3 seconds to allow
 *			devices to complete their self tests and become
 *			ready, before we call to boot driver.
 *			NOTE: called in physical mode (mapping off).
 *
 *
 * Inputs:		None.
 *
 * Outputs:		None.
 *
 * Return Values:	None.
 *
 * Side Effects:
 *
 *			All SCSI busses are reset.
 *
 */

szreset()
{
	register struct sz_regs *szaddr;
	register struct nb_regs *sziaddr = (struct nb_regs *)NEXUVII;
	register struct sz_softc *sc;
	register int i, cntlr;

	if (mfpr(MAPEN) & 1)	/* memory management must be off, just */
	   return;		/* return if m/m on.		       */

	for (cntlr = 0; cntlr < nNSCSI; cntlr++) {
	    sc = &sz_softc[cntlr];
	    if (sc->sc_cntlr_alive == 0)
		continue;
	    szaddr = (struct sz_regs *)SZMEMCVAXSTAR;
	    szaddr += cntlr;
	    sziaddr->nb_int_msk &= ~(SCS_IC_BIT >> cntlr);
	    szaddr->scs_inicmd = SCS_INI_RST;
	    for (i = 0; i < 1000; i++);	/* hold reset at least 25 Usec */
	    szaddr->scs_inicmd = 0;
	    i = szaddr->scs_reset;
	    i = szaddr->scs_reset;
	    i = szaddr->scs_reset;
	}

	/* delay 3 seconds (DO NOT use DELAY() in physical mode */
	sziaddr->nb_diagtime = 0;
	while (sziaddr->nb_diagtime < 300) ;
}


/*
 *
 * Name:		sz_dumpregs	-print contents of SCSI regs
 *
 * Abstract:		This routine reads the contents of the SCSI
 *			hardware registers for the specified controller
 *			into local variables, then prints the variables.
 *			NOTE: order of register reads in important.
 *			CAUTION: a controller reset (i = szaddr->scs_reset)
 *				 must follow the call to sz_dumpregs().
 *
 * Inputs:
 *
 * cntlr		SCSI bus controller number (0 = bus A, 1 = bus B).
 * how			Where to print: 0 - console and error log,
 *			1 - errorlog only, 2 - console only.
 *
 * Outputs:
 *
 *			Calls to printf and mprintf.
 *
 * Return Values:	None.
 *
 * Side Effects:
 *
 *			how = 1, messages written to error log.
 *			how = 0, printf to console (at high IPL).
 *
 */

sz_dumpregs(cntlr, how)
int cntlr;
int how;
{
    register struct sz_regs *szaddr = (struct sz_regs *)szmem + cntlr;
    int curdata, inicmd, mode, tarcmd, curstat, status;

    inicmd = szaddr->scs_inicmd;
    mode = szaddr->scs_mode;
    tarcmd = szaddr->scs_tarcmd;
    curstat = szaddr->scs_curstat;
    status = szaddr->scs_status;
    /* read curdata last, so if we cause a parity error it won't print */
    curdata = szaddr->scs_curdata;

    if (how == 0) {
	printf("%s 80=%x 84=%x 88=%x 8c=%x 90=%x 94=%x adr=%x cnt=%x dir=%x\n",
	    (cntlr == 0 ? "SCSI 0 regs:" : "SCSI 1 regs:"), curdata, inicmd,
	    mode, tarcmd, curstat, status, szaddr->scd_adr,
	    szaddr->scd_cnt, szaddr->scd_dir);
    }
    else if (how == 1) {
	mprintf("%s 80=%x 84=%x 88=%x 8c=%x 90=%x 94=%x adr=%x cnt=%x dir=%x\n",
	    (cntlr == 0 ? "SCSI 0 regs:" : "SCSI 1 regs:"), curdata, inicmd,
	    mode, tarcmd, curstat, status, szaddr->scd_adr,
	    szaddr->scd_cnt, szaddr->scd_dir);
    } else {
	cprintf("%s 80=%x 84=%x 88=%x 8c=%x 90=%x 94=%x adr=%x cnt=%x dir=%x\n",
	    (cntlr == 0 ? "SCSI 0 regs:" : "SCSI 1 regs:"), curdata, inicmd,
	    mode, tarcmd, curstat, status, szaddr->scd_adr,
	    szaddr->scd_cnt, szaddr->scd_dir);
    }
}
