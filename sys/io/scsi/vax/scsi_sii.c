#ifndef lint
static	char	*sccsid = "@(#)scsi_sii.c	4.6  (ULTRIX)        1/3/91";
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
 * scsi_sii.c	17-Aug-88
 *
 * VAX SCSI device driver (SII routines)
 *
 * Modification history:
 *
 * 11/21/90	Robin Miller
 *   o	Added support for 10-byte Read/Write CDB's.
 *   o  Removed '#ifdef FORMAT' conditionalization, since this special
 *	interface is always desired.
 *   o	Use function sz_cdb_length() to obtain the Command Descriptor
 *	Block (CDB) length.
 *
 * 09/26/90	Robin Miller
 *	Fixed problem introduced in 8/29/90 edit.  I inadvertantly left
 *	off the target ID index when referencing the SII DMA count field
 *	(sc_siidmacount[targid]) on disconnect phases.  This problem was
 *	causing a "Panic: Protection Fault" when booting.
 *
 * 08/30/90	Robin Miller
 *	Added check in probe routine to recognize RX26 device type
 *	and setup accordingly for floppy device.
 *
 * 08/29/90	Robin T. Miller
 *	Changed the check made in sii_msgin() to set the SZ_DMA_DISCON
 *	flag (disconnect occured during dma) when there is residual dma
 *	count for this target.  Previously, this flag was only set if
 *	the previous phase was a data in or a data out.  This check was
 *	insufficient for our CDROMs, broke both the RRD40 & RRD42, since
 *	the previous two phases were message in.
 *
 * 08-Aug_90 Bill Dallas
 *	Got rid of the -2 of the modsns len for disks. This was
 *	causing problems when we got rid of hard define for MODSNS_LEN
 *	which was 14 ( tapes). Caused problems on booting. In
 *	sii_startdma is where the changed was made.
 *
 * 05-Jul-90	Bill Dallas
 *	Added support for the new device option table
 *	Got rid of hard define
 *
 * 02-Dec-89	Fred Canter
 *	Calculate the number of PTEs needed to double map the user's
 *	buffer in a machine independant way (no more magic numbers).
 *
 * 13-Nov-89    Janet L. Schank
 *      During the message in phase, if an unsupported extended message
 *      is received, ATN is asserted before the last message byte
 *      is acknowledged.  Remove refrences to SII, replaced with nSII.
 *
 * 08-Oct-89	Fred Canter
 *	Removed error log #ifdef OLDWAY.
 *
 * 07-Oct-89	Fred Canter
 *	Save removable media bit from inquiry data in sz_softc.
 *
 * 17-Aug-89    Janet L. Schank
 *      sii_scsistart now uses sz_opcode as an index into the sz_timetable.
 *      This was done since pseudo-opcode values aren't in his table.  A
 *      check is also being done on the sz_opcode to make sure its in the
 *      table.
 *
 * 23-Jul-89	Fred Canter
 *	RX33 support.
 *
 * 16-Jul-89	Fred Canter
 *	Bug fix - mode sense byte was not set correctly.
 *	Allows mode sense to get disk geometry.
 *
 * 14-Jul-89	Fred Canter
 *	Merged Dynamic BBR and error log changes.
 *
 * 28-Jun-89	Fred Canter
 *	Convert error messages from printf to scsi_logerr().
 *
 * 23-Jun-89	Fred Canter
 *	Save status byte for current command for error log.
 *
 * 23-Jun-89	John A. Gallant
 *	Added the initialization of the completion code routine pointers.  In
 *	the start dma code added support for the read/write long commands.
 *
 * 20-Jun-89	Fred Canter
 *	Convert to scsi_devtab.
 *
 * 24-May-89	Fred Canter
 *	Changes to match the new, more general rzdisk utility.
 *	Changed the mode select data structures so an address and
 *	length are passed with the ioctl. This allows pages to be
 *	added to rzdisk without requiring a kernel rebuild.
 *
 * 06-Apr-89	Fred Canter
 *	Added b_comand to replace b_command for local command buffers.
 *	Use b_gid instead of b_resid to store command.
 *	No longer need to restore bp->b_command on error retry.
 *
 * 22-Feb-89	Alan Frechette
 *	A few minor probe changes. Fixed BUS DEVICE RESET handling.
 * 	Added the routine "sii_restart_target()" to restart driver
 *	activity for a particular target after a BUS DEVICE RESET 
 *	occurred on that particular target.
 *
 * 13-Feb-89	Alan Frechette
 *	Added in function headers for all subroutines. Allow an
 *	additional retry of the inquiry command in "sii_probe()"
 *	in the case that SZ_RET_ABORT is returned. Added an 
 *	additional check in "sii_intr()" when checking for DMA
 *	transfers ">8K".
 *
 * 12-Feb-89	Fred Canter
 *	Removed some comments which no longer apply.
 *
 * 11-Feb-89	Fred Canter
 *	Fixed yet another bug with 64KB transfers. Use blkcpy instead
 *	of bcopy (bcopy max byte count is 64KB -1 ) for copying
 *	data to and from the users' buffer.
 *
 *  5-Feb-89	Fred Canter
 *	Remove rz_max_xfer, cz_max_xfer, and tz_max_xfer.
 *
 * 27-Dec-88	Alan Frechette
 *	Made a change to break out of the (SZWAIT_UNTIL) loop in the
 *	routines "sii_recvdata()" and "sii_senddata()" during a DMA
 *	transfer if either the (SII_MIS) or (SII_DNE) bits are set.
 *
 *	Added debug printf's (PRINTD) to track timing of spin loops.
 *
 *	We no longer spin wait for phase changes in the routines
 *	"sii_scsistart()" and "sii_intr()". We now wait for the 
 *	interrupt to occur. We only spin wait for the "msgout" and 
 *	"command" phases. All other phases we wait for the interrupt.
 *
 *	Changed "scsi_completed" variable to be an array. It is now
 *	defined as "short scsi_completed[NDPS]".
 *
 * 19-Dec-88	Alan Frechette
 *	Added a small spinwait in "sii_state_change()" to wait for
 *	the (SII_CON) bit in the CSTAT REGISTER to be asserted if 
 *	the SII was expecting to be connected to a target. This is
 *	needed because the connect bit (SII_CON) can be asserted much 
 *	later than the reselect bit (SII_DST_ONBUS) on the SII.
 *
 *	Removed all the include files that were surrounded by the
 *	"#ifdef notdef" and "#endif notdef" construct.
 *
 *	Zero out ram buffer location in "sii_probe()" before issuing
 *	the inquiry command. This is needed to clear out any previous
 *	data in the ram buffer.
 *
 * 17-Dec-88	Fred Canter
 *	Added pseudo commands to resolve the conflict between
 *	SZ_UNLOAD and SZ_SSUNIT both being opcode 0x1b.
 *
 * 06-Dec-88	Alan Frechette
 *	Changed the select wait count in "sii_select_target()" to be
 *	2500 instead of 10000. Increased number of retries from 2 to 3.
 *	Fixed the sending of the DISCONNECT command in this routine
 *	also. The DISCONNECT command should only be sent if a selection 
 *	attempt failed and the selection in progress bit (SII_SIP) is
 *	set in the CSTAT REGISTER. The new select wait count is based
 *	on the Selection Timeout Delay in the scsi specification. The
 *	selection timeout value is 250 milliseconds.
 *
 *	Fixed probe to allow 64K transfers for the disk. Probe was
 *	allowing only (64K - 512) as the maximum disk transfer size.
 *
 * 01-Dec-88	Alan Frechette
 *	Had to make a change due to new firmware in CDROM. The CDROM
 *	now sends a message reject message and then disconnects from
 *	the bus when you send it an Extended Synchronous Data Tranfer
 *	Request Message. This is now handled correctly.
 *
 *	Clear sent synchronous message flags "sc->sc_siisentsync[]"
 *	in "sii_reset()" when operating in scsi polled mode during
 *	boot up. If we get a reset we must clear these flags or else
 *	the system will not come up.
 *
 *	Reworked the debugging for the SII. Improved it considerably.
 *
 * 03-Nov-88	Alan Frechette
 *	Added in support for disk maintainence. Added the commands
 *	FORMAT UNIT, REASSIGN BLOCK, READ DEFECT DATA and VERIFY DATA.
 *	Made changes to MODE SENSE and MODE SELECT. Made some more 
 *	performance improvements to the SII. I now pass "targid" to 
 *	every subroutine I call and got rid of the useless subroutine 
 *	"sii_getactive_target()". 
 *
 *	Add in support for non-DEC disks. This is done in the probe
 *	routine and the non-DEC disks are designated by "RZxx". Also
 *	removed "#define SZDEBUG" from this file.
 *
 *	Changed the spin loops for PHASE CHANGES. If a PHASE CHANGE 
 *	does not occur within the spin loop time period then the code
 *	will return and wait for the PHASE CHANGE interrupt from the SII. 
 *
 *	Added scsi command timeout logic to the SII. The routine 
 *	"sii_timer()" handles the command timeout logic. The SII
 *	uses the command timeout table "sz_timetable[]" defined
 *	in the file "scsi.c".
 *
 *	Made a change to handle devices that do not support scsi
 *	Extended Messages in "sii_msgout()". If a scsi command
 *	did not have good status then return the value "SZ_RET_ERR"
 *	in "sii_scsistart()" otherwise return "SZ_SUCCESS". This
 *	was a nasty bug.
 *
 *	Implemented the Bus Device Reset message to reset only a
 *	particular SCSI device and not the entire SCSI bus. Also
 *	implemented the Abort message to abort the current command
 *	due to an error.
 *
 *	Fixed the routine "sii_clear_discon_io_requests()" to not
 *	return an IO error on the disconnected IO request after a 
 *	reset occurred on the SCSI bus.
 *
 *	Fixed the entire "reset" handling throughout this driver.
 *
 * 08-Sep-88	Alan Frechette
 *	Fixed the handling of spurious interrupts on the SII. The
 *	interrupt routine will ignore all spurious interrupts from 
 *	the SII and will simply return. 
 *
 *	The following fixes were made to support the CDROM device:
 *	1: Fixed the handling of the Synchronous Data Transfer Request 
 *	   message with the CDROM device. The CDROM device would switch
 *	   phase before it would recieve the last byte of this message. 
 *	
 *	2: Ignore the second disconnect if we get back-to-back 
 *	   disconnects from the CDROM device. 
 *
 *	3: Rewrote the routine "sii_select_target" to handle the
 *	   slower CDROM device. Wait for the connect bit (SII_CON)
 *	   after we get the state change bit (SII_SCH). Corrected 
 *	   some of the logic in this routine.
 *
 * 01-Sep-88	Alan Frechette
 *	Save the value of the DMA Length Of Transfer Count register
 *	in "sii_msgin". Since the SII now uses DMA for all information
 *	transfers we must save the this value in case the message
 *	is a SAVE DATA POINTER message. Otherwise we will lose the
 *	true state of the DMA for that target and how much data was 
 *	actually transfered.
 *
 *	Made a fix to "sii_senddata" when using programmed io. Added
 *	the setting of the "attn" variable when setting the Command
 *	register in the SII. 
 *
 * 23-Aug-88	Alan Frechette
 *	Fixed the GENERIC kernel boot problem for SII systems.
 *	The GENERIC kernel now boots off an SII based workstation.
 *
 *	Added support to handle SYNCHRONOUS and ASYNCHRONOUS devices
 *	at the same time. I simply save the req/ack offset for each
 *	SCSI device out on the bus after issuing the Synchronous Data
 *	Transfer Request extended message. The req/ack offset is used
 *	to set the DMA Control Register of the SII for synchronous or
 *	asynchronous data transfers.
 *
 *	Fixed a few bugs in the SII interrupt routine dealing with
 *	reselects and handle quick reselects there rather then wait
 *	for another interrupt.
 *
 * 17-Aug-88	Fred Canter
 *	Created this file by moving the SCSI SII specific files
 *	from the old combined driver (scsi.c) to scsi_sii.c.
 *	The SII code was written by Darrell Dunnuck and Alan Frechette.
 *
 ***********************************************************************/

#include "../data/scsi_data.c"

#ifdef SZDEBUG
/*********************************************************************
 *	      NOTE ON HOW DEBUGGING WORKS FOR SII DRIVER
 *
 * The scsi debug variable "siidebug" controls the level of debugging
 * and is defined as follows:
 *
 * bit 0: prints out only scsi bus phase/state values.	(siidebug=0x1)
 * bit 1: prints out only the expansion of commands.  	(siidebug=0x2)
 * bit 2: prints out only errors encountered.	       	(siidebug=0x4)
 * bit 3: prints out only code flow through routines. 	(siidebug=0x8)
 * bit 4: prints out only spurious interrupts. 		(siidebug=0x10)
 * bit 5: prints out only timer values of spin loops. 	(siidebug=0x20)
 *
 * The scsi target variable "siitarget" controls the debugging of a
 * specific target only. 
 *
 * bit 0: prints out only debugging for target 0	(siitarget=0x1)
 * bit 1: prints out only debugging for target 1	(siitarget=0x2)
 * bit 2: prints out only debugging for target 2	(siitarget=0x4)
 * bit 3: prints out only debugging for target 3	(siitarget=0x8)
 * bit 4: prints out only debugging for target 4	(siitarget=0x10)
 * bit 5: prints out only debugging for target 5	(siitarget=0x20)
 * bit 6: prints out only debugging for target 6	(siitarget=0x40)
 * bit 7: prints out only debugging for target 7	(siitarget=0x80)
 * all  : prints out debugging for all targets		(siitarget=-1)
 *
 *
 * EXAMPLE: Debug (target 3) with bitmask (bit0|bit1|bit2)
 *
 * 		adb -w -k /vmunix /dev/mem
 * 		siitarget/W 8		(Set to target 3)
 * 		siidebug/W 7		(Set debug bitmask to 7)
 * 		$q
 *
 ********************************************************************/
int siidebug = 0;
int siitarget = 0;
#define PRINTD(T,F,X) {						\
    if(siidebug & F) {						\
	if((siitarget & (1<<T)) || siitarget == -1 || T == -1)	\
	    cprintf X;						\
    }								\
}
#endif SZDEBUG


/*
 * Number of seconds to wait for SCSI devices
 * to come ready enough to respond to inquiry
 * after a bus reset. Needs to be a variable so
 * users can change it (I am such a nice guy).
 * Factory default is 7 seconds (in scsi_data.c).
 */
extern int sz_wait_for_devices;

int	sii_wait_after_inquiry = 1000;

/*
 * More external declarations for the SII.
 */
extern char cvqmsi[] [512*NBPG];/* The SII registers in IO SPACE */
extern char cvqmsirb[];		/* The 128K RAM buffer for the SII */
extern char szbufmap[];		/* Holds PTE's from get_sys_ptes() */
extern short sz_timetable[];	/* Command timeout table for SII chip */
extern int szp_nrz;		/* Number of RZ disks found */
extern int szp_ntz;		/* Number of TZ tapes found */
extern int szp_ncz;		/* Number of CDROM optical disks found */
extern int szp_nrx;		/* Number of CDROM optical disks found */
extern struct scsi_devtab szp_rz_udt;
extern struct scsi_devtab szp_tz_udt;
extern struct scsi_devtab szp_cz_udt;
extern int sii_scsistart();	/* SII scsi start routine */
extern int sii_reset();		/* SII scsi reset routine */
int sii_restart_target();	/* SII scsi restart target routine */
int sz_strncmp();
int sz_strcmp();
extern int rzcomplete();
extern int tzcomplete();
extern int sz_cdb_length();

/**********************************************************************
 *
 * Name:	sii_probe
 *
 * Abstract:	The SII probe entry point routine from auto-configure
 *		code. Determine if the SII controller exists. Reset the
 *		SCSI bus. Size the number and type of targets on the
 *		SCSI bus. Set up the "sz_softc" structure and the 128K
 *		hardware buffer.
 *
 * Inputs:
 * reg		SCSI bus controller CSR address (not used).
 * cntlr	SCSI bus controller number (0 = bus A, 1 = bus B).
 *
 * Outputs:
 * sz_softc
 *
 *  sc_sysid		SCSI bus ID of initiator (CPU).
 *  *port_start()	SCSI command start routine - sii_scsistart().
 *  *port_reset()	SCSI bus reset - sii_reset().
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
 *  sc_siisentsync[]	Per target - sent synchronous message flag.
 *  sc_siireqack[]	Per target - the req/ack offset for synchronous.
 *  sc_siidboff[]	SCSI bus special RAM buffer slots.
 *
 * Return values:
 * 0			Controller does not exist or did not respond.
 * 1			Controller exists and is alive.
 *
 * Side Effects:
 *			The SII chip, the DMA controller, and the
 *			SCSI bus are reset. Much of sz_softc is set up.
 *			Port start and reset routine pointers set up.
 *			Address of 128 KB RAM buffer set up.
 *
 **********************************************************************/
short scsi_bus_idle = 0;		/* SCSI bus is idle flag     */
short scsi_completed[NDPS];		/* Command Complete flag     */
short scsi_polled_mode = 0;		/* SCSI Polled Mode flag     */
short sii_sent_cmd = 0;			/* SCSI Command Sent flag    */
short sii_wait_count = SII_WAIT_COUNT;	/* Delay count for SII chip  */
short sii_no_disconnects = 0;		/* Allow No Disconnects flag */
short sii_use_programmed_io = 0;	/* Programmed IO Mode flag   */
short sii_firstcall = 1;		/* First call to probe flag  */
short sii_debug_probe = 0;		/* Used for debugging probe  */
short sii_test = 0;			/* Used for testing purposes */

sii_probe(reg, cntlr)
caddr_t reg;
int cntlr;
{
	register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
	register struct sz_softc *sc;
	int targid, unit;
	int dboff;
	int rz_slotsize, tz_slotsize, cz_slotsize;
	int ncz, ntz;
	int i, s, stat;
	struct sz_inq_dt *idp;
	struct sz_rdcap_dt *rdp;
	struct scsi_devtab *sdp;
	struct scsi_devtab *usdp;
	int sdp_match;
	char *p;
	int alive;
	int retries, status;
	int save_siidebug;
	int save_siitarget;

#ifdef SZDEBUG
	/* Check if debug probe flag is set */
	if(sii_debug_probe) {
	    save_siidebug = siidebug;
	    save_siitarget = siitarget;
	    siidebug = 0x1f;
	    siitarget = -1;
	}
	PRINTD(-1, 0x8, ("sii_probe: start probing the SCSI bus\n"));
#endif SZDEBUG

	/*
	 * Probe must fail if controller not configured.
	 */
	alive = 1;
	if(cntlr >= nNSII)
	    alive = 0;

	/*
	 * Initialize certain fields in the softc structure
	 * and reset the SII chip.
	 */
	if(alive) {
	    scsi_polled_mode = 1;
	    sc = &sz_softc[cntlr];
	    sc->sc_sysid = 7;		    /* init the host adapter ID */
	    sc->sc_active = 0;		    /* init the active flag */
	    sc->sc_rip = 0;		    /* init reset in progress flag */
	    sc->port_start = sii_scsistart; /* init the port_start switch */
	    sc->port_reset = sii_reset;	    /* init the port_reset switch */
	    sc->sc_rambuff = cvqmsirb;	    /* init the RAM buffer pointer*/
	    sii_reset(sc); 		    /* reset the SII chip */
	}

	/*
	 * Use the inquiry command to determine number
	 * and type of targets on this controller.
	 * If this controller does not exist alive will
	 * be zero and the for loop won't do anything.
	 */
	for(targid=0; targid<NDPS; targid++) {
	    if(alive == 0)
		break;
	    sc->sc_siisentsync[targid] = 0;
	    sc->sc_siireqack[targid] = 0;
	    sc->sc_rzspecial[targid] = 0;
	    sc->sc_rmv_media &= ~(1 << targid);
	    if(targid == sc->sc_sysid)
		continue;	/* skip initiator */
	    retries = 3;
	    while(retries--) {
		sc->sc_szflags[targid] = SZ_NORMAL; 
		sc->sc_curcmd[targid] = SZ_INQ;
		bzero((sc->sc_rambuff + sc->sc_siidboff[targid]),
					sizeof(sc->sz_dat[targid]));
		sz_bldpkt(sc, targid, SZ_INQ, 0, 0);
		stat = sii_scsistart(sc, targid, 0);
		if(stat == SZ_SUCCESS)
		    break;
	    }
	    if(stat != SZ_SUCCESS)
		continue;
	    /*
	     * Initialize data structures for this target and
	     * save all pertinent inquiry data (device type, etc.).
	     */
	    idp = (struct sz_inq_dt *)&sc->sz_dat[targid];

	    /* Save removalbe media bit for each target */
	    if (idp->rmb)
		sc->sc_rmv_media |= (1 << targid);

#ifdef SZDEBUG
	    PRINTD(targid, 0x8, ("", sii_print_inq_info(idp)));
#endif SZDEBUG

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
		printf("sii_probe: scsi %d targetID %d: %s (%d).\n", cntlr,
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
		    printf("sii_probe: scsi %d targetID %d: %s\n",
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
		 */
		if (sdp->flags & SCSI_TRYSYNC)
		    sc->sc_siisentsync[targid] = 0;
		else
		    sc->sc_siisentsync[targid] = 1;
		if (sdp->flags & SCSI_REQSNS) {
		    sc->sc_curcmd[targid] = SZ_RQSNS;
		    sz_bldpkt(sc, targid, SZ_RQSNS, 0, 1);
		    sii_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_STARTUNIT) {
		    /*
		     * Send two start unit commands because a pending unit
		     * attention may cause the first one to fail. We don't
		     * for the drive to spin up here (happens in rzopen).
		     */
		    sc->sc_curcmd[targid] = SZ_P_SSUNIT;
		    sz_bldpkt(sc, targid, SZ_P_SSUNIT, 0, 1);
		    sii_scsistart(sc, targid, 0);
		    sii_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_TESTUNITREADY) {
		    sc->sc_curcmd[targid] = SZ_TUR;
		    sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
		    sii_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_READCAPACITY) {
		    sc->sc_curcmd[targid] = SZ_RDCAP;
		    sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
		    sii_scsistart(sc, targid, 0);
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
	    DELAY(sii_wait_after_inquiry);
	}		/* end of for loop */
	/*
	 * Clean out any left over interrupts.
	 */
	if (alive) {
		siiaddr->sii_cstat = siiaddr->sii_cstat;
		siiaddr->sii_dstat = siiaddr->sii_dstat;
		siiaddr->sii_csr = (SII_HPM | SII_RSE | SII_SLE | SII_PCE | SII_IE);
	}

	/*
	 * TODO: should use map to allocate 128KB buffer
	 *
	 * If last (really 2nd) call to sii_probe,
	 * or only one controller configured,
	 * assign 128K data buffer slots.
	 *
	 * 128K data buffer allocation strategy:
	 *	1KB  - SII controller for non READ/WRITE DMA transfers
	 *	16KB - for each tape unit
	 *	 8KB - for each cdrom unit
	 *	??KB - for each disk unit
	 * ?? is what's left after tapes and cdroms divided by # of disks.
	 * ?? must be >= 8KB, should be >= 16KB, if not reduce
	 * number of cdroms to 2, then number of tapes to 2.
	 * If that don't fix it panic!
	 * In any "real" configuration, we should
	 * never hit these limits.
	 */
	if ((nNSII == 1) || (sii_firstcall == 0)) {
	    dboff = 0x0;
	    /*
             * Setup 128 byte ram buffer slots for each target to
	     * be used for non READ/WRITE DMA Transfers on the SII.
	     */
	    for(i=0; i<NDPS; i++) {
		sc->sc_siidboff[i] = dboff;
		dboff += 128;
	    }
	    dboff = (1 * 1024);
	    /* determine rz slot size, must be > 16kb */
	    while (1) {
		rz_slotsize = 128 - (dboff/1024);
		rz_slotsize -= (szp_ntz * 16);
		rz_slotsize -= (szp_ncz * 8);
		if ((rz_slotsize > 0) && szp_nrz)
		    rz_slotsize = rz_slotsize / szp_nrz;
		if (rz_slotsize < 16) {
		    if (szp_ncz > 2) {
			printf("Only 2 CDROM's (too many other devices)\n");
			szp_ncz = 2;
			continue;
		    }
		}
		if (rz_slotsize < 16) {
		    if (szp_ntz > 2) {
			printf("Only 2 TAPE's (too many other devices)\n");
			szp_ntz = 2;
			continue;
		    }
		}
		if (rz_slotsize < 16) {
		    if (rz_slotsize >= 8)
			printf("Poor disk performance (too many devices)\n");
		    else
			panic ("Impossible number of devices");
		}
		break;
	    }
	    if (rz_slotsize > 64)
		rz_slotsize = 64;
	    rz_slotsize *= 1024;
	    tz_slotsize = (16 * 1024);
	    cz_slotsize = (8 * 1024);
	    ncz = ntz = 0;
	    for (cntlr=0; cntlr<nNSII; cntlr++) {
		sc = &sz_softc[cntlr];
		for (targid=0; targid<NDPS; targid++) {
		    if (targid == sc->sc_sysid)
			continue;
		    if (sc->sc_alive[targid] == 0)
			continue;
		    /* TODO: check for dboff pase end of 128K buffer? */
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
			sc->sc_dboff[targid] = dboff;
			sc->sc_segcnt[targid] = rz_slotsize;
			dboff += rz_slotsize;
		    }
		    else
			continue;
#ifdef SZDEBUG
		    PRINTD(-1, 0x8, ("sii_probe: cntlr=%d targid=%d ",
			     cntlr, targid));
		    PRINTD(-1, 0x8, ("devtype=%x req/ack=%d slotsize=%d\n",
			     sc->sc_devtyp[targid], sc->sc_siireqack[targid], 
			     sc->sc_segcnt[targid]));
#endif SZDEBUG
		}
	    }
	}
	if (sii_firstcall)
	    sii_firstcall = 0;

#ifdef SZDEBUG
	PRINTD(-1, 0x8, ("sii_probe: done probing the SCSI bus\n"));
	if(sii_debug_probe) {
	    siidebug = save_siidebug;
	    siitarget = save_siitarget;
	}
#endif SZDEBUG
	return(alive);
}

/******************************************************************
 *
 * Name:	sii_timer 	
 *
 * Abstract:	Handle command timeouts on the SII chip. 
 *
 * Inputs:
 * unit		The ULTRIX logical unit number of the scsi device.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_timer(unit)
int unit;
{

    int cntlr, targid, s, retval;
    register struct sz_softc *sc;
    register struct sii_regs *siiaddr;
    int flags;

    s = spl5();
    cntlr = (unit >> 3) & 1;
    siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    sc = &sz_softc[cntlr];
    targid = unit & 7;

    /* Ignore timeouts for the tape device */
    if(sc->sc_devtyp[targid] & SZ_TAPE) {
#ifdef SZDEBUG
    	PRINTD(targid, 0x4,
	    ("sii_timer: SZ_TAPE cmd timeout (bus=%d target=%d cmd=0x%x)\n",
	    	cntlr, targid, sc->sc_curcmd[targid]));
#endif SZDEBUG
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDTIMO, 0, 0, flags);
	sc->sc_szflags[targid] &= ~SZ_TIMERON;
	splx(s);
	return;
    }

    /*
     * The target disconnected and never reselected the SII.
     * If we can select the target then issue a Bus Device
     * Reset message to reset that target. This will cause
     * the SII to go to the bus free state. In the interrupt
     * routine the SII will then call "sz_start()" to finish
     * up this command. If we cannot select the target then
     * we simply reset the SCSI bus.
     */
    if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
	/* Wait for scsi bus to be idle */
	if (sc->sc_active) {
    	    timeout(sii_timer, (caddr_t)sc->sc_unit[targid], 10);
	    splx(s);
	    return;
	}
#ifdef SZDEBUG
    	PRINTD(targid, 0x4,
	    ("sii_timer: DISCON cmd timeout (bus=%d target=%d cmd=0x%x)\n",
	    	cntlr, targid, sc->sc_curcmd[targid]));
#endif SZDEBUG
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDTIMO, 1, 0, flags);

        /* Try to select the disconnected target */
        switch(retval = sii_select_target(sc, targid)) {
	case SZ_BUSBUSY:
	    /* Target reselected so handle it now. */
	    if((siiaddr->sii_destat & SII_IDMSK) != targid)
    	        timeout(sii_timer, (caddr_t)sc->sc_unit[targid], 10);
	    else
    	        timeout(sii_timer, (caddr_t)sc->sc_unit[targid], 30*hz);
	    break;

	case SZ_SUCCESS:
	    /* Target selected so set flag to reset it. */
	    sc->sc_szflags[targid] = SZ_RESET_DEV;
	    break;

	default:
	    /* Target is hung so reset the SCSI bus. */
	    scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 0, 0, SZ_HARDERR);
            sii_reset(sc);
	}
    }
    /*
     * The currently active target has hung the SCSI bus.
     * Reset the SCSI bus and then return.
     */
    else {
#ifdef SZDEBUG
    	PRINTD(targid, 0x4,
	    ("sii_timer: ACTIVE cmd timeout (bus=%d target=%d cmd=0x%x)\n",
	    	cntlr, targid, sc->sc_curcmd[targid]));
#endif SZDEBUG
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDTIMO, 2, 0, flags);
	scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 1, 0, SZ_HARDERR);
        sii_reset(sc);
    }
    splx(s);
}

/******************************************************************
 *
 * Name:	sii_scsistart
 *
 * Abstract:	Start a SCSI operation on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 * bp		Buffer pointer for I/O request.
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_IP		Command in progress waiting for interrupt.
 * SZ_RET_ERR		Command failed (returned bad status).
 * SZ_RET_ABORT		Command aborted.
 * SZ_BUSBUSY		Bus is busy, retry the command later.
 * SZ_RET_RESET		Resetting bus, retry command after bus reset.
 ******************************************************************/
sii_scsistart(sc, targid, bp)
register struct sz_softc *sc;
int targid;
register struct buf *bp;
{
    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int retval, phase, timer, tvalue;
    int flags;

    /* Initialize variables */
    scsi_bus_idle = 0;
    scsi_completed[targid] = 0;
    scsi_polled_mode = 0;
    sii_sent_cmd = 0;

    /*
     * If "bp" is "0" we use polled scsi mode, disallow reselect
     * attempts, and disable interrupts for all DMA transfers. We 
     * poll for DMA completion instead of allowing interrupts.
     */
    siiaddr->sii_csr |= (SII_RSE | SII_IE | SII_SLE);
    if(bp == (struct buf *)0) {
        siiaddr->sii_csr &= ~(SII_RSE | SII_IE);
	scsi_polled_mode = 1;
    }

    /* Perform target arbitration and selection */
    if((retval = sii_select_target(sc, targid)) != SZ_SUCCESS)
	return(retval);

    /* Make sure that the command is in the sz_timetable. */
    if (sc->sz_opcode < SZ_TIMETBL_SZ) {
	/* Set the timeout value for the current scsi command */
	sc->sc_szflags[targid] |= SZ_TIMERON;
	if(sz_timetable[sc->sz_opcode] == 0) {
	    tvalue = 2;
	    timeout(sii_timer, (caddr_t)sc->sc_unit[targid], tvalue*30*hz);
	}
	else {
	    tvalue = sz_timetable[sc->sz_opcode];
	    timeout(sii_timer, (caddr_t)sc->sc_unit[targid], tvalue*30*hz);
	}
    }

BEGIN_LOOP:
    /* Loop through all bus phases until command complete */
    do {
        /* Check for a BUS ERROR */
        if(siiaddr->sii_cstat & SII_BER)
  	    siiaddr->sii_cstat = SII_BER;

        /* Check for a PARITY ERROR */
	if(siiaddr->sii_dstat & SII_IPE) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 0, 0, flags);
#ifdef SZDEBUG
	    PRINTD(targid, 0x4, 
		("sii_scsistart: scsi %d parity error\n", cntlr));
#endif SZDEBUG
	    goto HANDLE_ERROR;
	}

	/* Check for a BUS RESET */
	if(siiaddr->sii_cstat & SII_RST_ONBUS) {
	    siiaddr->sii_cstat = SII_RST_ONBUS;
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 0, 0, SZ_HARDERR);
#ifdef SZDEBUG
	    PRINTD(targid, 0x4, 
		("sii_scsistart: scsi %d bus reset\n", cntlr));
#endif SZDEBUG
	    goto HANDLE_ERROR;
	}

        /* Check for a STATE CHANGE */
        if(siiaddr->sii_cstat & SII_SCH) {
	    siiaddr->sii_cstat = SII_SCH;
            sii_state_change(sc, &targid);
	}

	/* If disconnected and went to BUS FREE STATE then break */
	if(scsi_bus_idle)
	    break;

        /* Check for a PHASE CHANGE */
        if(siiaddr->sii_dstat & SII_MIS) {

            /* Check for a BUS ERROR */
            if(siiaddr->sii_cstat & SII_BER)
  	        siiaddr->sii_cstat = SII_BER;

	    /* Always clear DID DMA flag on a phase change */
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

            /* Handle the current bus phase */
            if(sii_phase_change(sc, targid) != SZ_SUCCESS)
                goto HANDLE_ERROR;

            /* Wait for the next bus phase */
            if(!scsi_completed[targid] && 
		  !(sc->sc_szflags[targid] & (SZ_WAS_DISCON|SZ_DID_DMA))) {
		if(!scsi_polled_mode && sii_sent_cmd &&
			!(siiaddr->sii_cstat & (SII_CI|SII_DI)) &&
				(sc->sc_actcmd[targid] != SZ_RQSNS))
		    return(SZ_IP);
	    }
	}

	/* Check for fragmented DMA transfers (>8K) */
	if(scsi_polled_mode && !(siiaddr->sii_dstat & SII_MIS) &&
		(siiaddr->sii_dstat & (SII_TBE|SII_IBF)) &&
		    !(sc->sc_szflags[targid] & SZ_DID_DMA) &&
	  		((sc->sc_fstate == SZ_DATAI_PHA) || 
				(sc->sc_fstate == SZ_DATAO_PHA))) {

	    /* Restart the DMA transfer */
	    if(!sii_restartdma(sc, targid)) {
#ifdef SZDEBUG
		PRINTD(targid, 0x4,
		    ("sii_scsistart: DMA error in sii_restartdma\n"));
#endif SZDEBUG
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x0cd, 0, SZ_HARDERR);
	        goto HANDLE_ERROR;
	    }
	}

	/* Sometimes the target stays in the same phase */
	if((siiaddr->sii_dstat & (SII_IBF|SII_TBE)) && 
		!(siiaddr->sii_dstat & SII_MIS) &&
			((sc->sc_fstate != SZ_DATAI_PHA) &&
				(sc->sc_fstate != SZ_DATAO_PHA))) {

            /* Check for a BUS ERROR */
            if(siiaddr->sii_cstat & SII_BER)
  	        siiaddr->sii_cstat = SII_BER;

	    /* Handle the current bus phase */
	    if(sii_phase_change(sc, targid) != SZ_SUCCESS)
	        goto HANDLE_ERROR;
	}
    } while(!scsi_bus_idle && 
		!(sc->sc_szflags[targid] & (SZ_WAS_DISCON|SZ_DID_DMA)));

    /*
     * Check the status of the current SCSI operation. If the SCSI
     * operation completed or disconnected then start the next SCSI 
     * operation, otherwise wait for the DMA to complete.
     * 
     */
    if(scsi_bus_idle || (sc->sc_szflags[targid] & SZ_WAS_DISCON)) {
	/* Handle BUS DEVICE RESET and ABORT processing. */
        if(sc->sc_szflags[targid] & (SZ_RESET_DEV|SZ_ABORT_CMD)) {
#ifdef SZDEBUG
            PRINTD(targid, 0x4,
		("sii_scsistart: BUS DEVICE RESET and ABORT processing\n"));
#endif SZDEBUG
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
	    if (sc->sc_dkn[targid] >= 0)
	        dk_busy &= ~(1 << sc->sc_dkn[targid]);

	    /* For a BUS DEVICE RESET we must wait for device. */
            if(sc->sc_szflags[targid] & SZ_RESET_DEV) {
		scsi_logerr(sc, 0, targid, SZ_ET_RSTTARG, 0, 0, SZ_HARDERR);
                timeout(sii_restart_target, 
			sc->sc_unit[targid], sz_wait_for_devices*hz);
            	sc->sc_active = 0;
	        sc->sc_fstate = 0;
		return(SZ_RET_RESET);
	    }
	    /* For an ABORT we simply retry the command. */
	    else {
	        sc->sc_active = 0;
	        sc->sc_fstate = 0;
	        sc->sc_szflags[targid] = (SZ_NEED_SENSE|SZ_RETRY_CMD);
	        return(SZ_RET_ERR);
	    }
	}
        else if(scsi_completed[targid]) {
#ifdef SZDEBUG
            PRINTD(targid, 0x8,
		("sii_scsistart: COMMAND COMPLETED successfully\n"));
#endif SZDEBUG
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
            sc->sc_active = 0;
    	    if(sc->sc_status[targid] == SZ_GOOD)
	        return(SZ_SUCCESS);	
	    else
	        return(SZ_RET_ERR);	
        } 
	else if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
#ifdef SZDEBUG
            PRINTD(targid, 0x8,
		("sii_scsistart: COMMAND IN PROGRESS disconnected\n"));
#endif SZDEBUG
	    return(SZ_IP);
        }
	else {
#ifdef SZDEBUG
            PRINTD(targid, 0x4,
		("sii_scsistart: (targid=%d) went to BUS FREE unexpectedly\n",
			targid));
#endif SZDEBUG
            sc->sc_active = 0;
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
	    return(SZ_RET_ERR);
	}
     } 
     else if(sc->sc_szflags[targid] & SZ_DID_DMA) {
	/* Poll and busy wait for DMA completion */
	if(scsi_polled_mode) {
#ifdef SZDEBUG
            PRINTD(targid, 0x8,
		("sii_scsistart: COMMAND IN PROGRESS dma poll mode\n"));
#endif SZDEBUG
            siiaddr->sii_csr &= ~SII_IE;
            SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_DNE|SII_MIS)),sii_wait_count,retval);
            SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_DNE|SII_MIS)),sii_wait_count,retval);
	    siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
	    siiaddr->sii_dstat = SII_DNE;
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

	    /* Update the remaining dma count for this current transfer */
	    if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		sc->sc_siidmacount[targid] -= 
			(SII_MAX_DMA_XFER_LENGTH - siiaddr->sii_dmlotc);
	    else
		sc->sc_siidmacount[targid] -= 
			(sc->sc_siidmacount[targid] - siiaddr->sii_dmlotc);
	    if(retval >= sii_wait_count)
	        goto HANDLE_ERROR;
	    else
	    	goto BEGIN_LOOP;
	}
	/* Wait for interrupt to signal DMA completion */
	else {
#ifdef SZDEBUG
            PRINTD(targid, 0x8,
		("sii_scsistart: COMMAND IN PROGRESS dma interrupt mode\n"));
#endif SZDEBUG
	    return(SZ_IP);
   	}
    }

HANDLE_ERROR:
    /* Abort the current SCSI operation due to error */
#ifdef SZDEBUG
    PRINTD(targid, 0x4,
	("sii_scsistart: command aborted (bus=%d target=%d cmd=0x%x)\n",
	    cntlr, targid, sc->sc_curcmd[targid]));
    PRINTD(targid, 0x4, ("", sii_dumpregs(cntlr, 0)));
#endif SZDEBUG
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 0, 0, flags);
    if(!scsi_polled_mode) {
	    scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 2, 0, SZ_HARDERR);
    }
    sii_reset(sc);
    return(SZ_RET_RESET);
}

/******************************************************************
 *
 * Name:	sii_select_target
 *
 * Abstract:	Perform the arbitration/selection phases for the 
 *		SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 * SZ_BUSBUSY		Bus is busy, retry the command later.
 ******************************************************************/
sii_select_target(sc, targid)
register struct sz_softc *sc;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int retval, i;
    int retries = 3;
    int sii_select_wait = 2500;

    /* Loop till retries exhausted */
    for(i=0; i<retries; i++) {
	/*
	 * Check to see if the SII is already connected or is in
	 * the selection process.
	 */
	if ((siiaddr->sii_cstat & (SII_CON | SII_SIP)) != 0)
	{
#ifdef SZDEBUG
        PRINTD(targid, 0x1,
	    ("sii_select_target: The SII is already connected\n"));
#endif SZDEBUG
	    return(SZ_BUSBUSY);
	}

        /*
         * Begin the arbitration/selection phase on the SII chip with 
	 * or without reselects. Setup the Selector Control Register 
   	 * and the Command Register on the SII to select a target on 
 	 * the SCSI bus.
         */
#ifdef SZDEBUG
        PRINTD(targid, 0x1,
	    ("sii_select_target: starting select of ID %d\n", targid));
#endif SZDEBUG
        siiaddr->sii_slcsr = targid;
        siiaddr->sii_comm = (SII_SELECT|SII_ATN);
    
	/*
         * Start timer to wait for a state change to occur on the
	 * SII. Wait approximately 250 ms for the target to assert
	 * BUSY on the SCSI BUS and for the state change interrupt
	 * bit (SII_SCH) to be set on the SII. The 250 ms value is 
	 * the selection timeout period as specified in the scsi 
	 * specification. (250 milliseconds is recommended)
	 */
        SZWAIT_UNTIL((siiaddr->sii_cstat & SII_SCH),sii_select_wait,retval);
#ifdef SZDEBUG
        PRINTD(targid, 0x20, 
            ("sii_select_target: (STATE select wait) start=%d end=%d\n",
	        sii_select_wait, (sii_select_wait - retval)));
#endif SZDEBUG

	/*
	 * If a state change did occur (SII_SCH is set) then make 
	 * sure we are connected. We got a state change so the SII
	 * has successfully SELECTED a target or was RESELECTED by
	 * a disconnected target.
	 */
	if((siiaddr->sii_cstat & SII_SCH) && !(siiaddr->sii_cstat & SII_CON))
            SZWAIT_UNTIL((siiaddr->sii_cstat & SII_CON),sii_select_wait,retval);

        /* Check for connection attempt */
        if(siiaddr->sii_cstat & SII_CON) {
            /* Check for a Reselection Attempt and handle it in "sii_intr" */
            if(siiaddr->sii_cstat & SII_DST_ONBUS) {
	        targid = (siiaddr->sii_destat & SII_IDMSK);
#ifdef SZDEBUG
          	PRINTD(targid, 0x1,
		    ("sii_select_target: reselect of ID %d has occurred\n",
				targid));
#endif SZDEBUG
    	        return(SZ_BUSBUSY);
            }
            /* Check for a Selection Attempt and handle it here */
            else {
    	        siiaddr->sii_cstat = SII_SCH;
        	targid = (siiaddr->sii_slcsr & SII_IDMSK);
#ifdef SZDEBUG
                PRINTD(targid, 0x1,
		    ("sii_select_target: select of ID %d succeeded\n",targid));
#endif SZDEBUG
                sc->sc_active = (1 << targid);
                sc->sc_selstat[targid] = SZ_SELECT;
		if(!scsi_polled_mode)
		    siiaddr->sii_csr &= ~(SII_SLE | SII_RSE);
    	        return(SZ_SUCCESS);
            }
        }
	else {
#ifdef SZDEBUG
	    PRINTD(targid, 0x4,
	        ("sii_select_target: select of ID %d timed out (%d TIMES)\n",
			targid, i+1));
#endif SZDEBUG
    	    /* 
	     * Selection timed out, clear necessary bus signals and
	     * abort the selection if the selection_in_progress bit
	     * (SII_SIP) is set. We abort the selection attempt by
	     * sending the DISCONNECT command. If the (SII_SIP) bit
	     * is not set then we most likely have a RESELECT from
	     * another target occuring.
	     */
	    if(siiaddr->sii_cstat & SII_SIP) {
    	        siiaddr->sii_cstat = SII_SCH;
    	        siiaddr->sii_comm = SII_DISCON;
    	        SZWAIT_UNTIL((siiaddr->sii_cstat & SII_SCH),sii_select_wait,retval);
#ifdef SZDEBUG
                PRINTD(targid, 0x20, 
                    ("sii_select_target: (STATE discon wait) start=%d end=%d\n",
	                sii_select_wait, (sii_select_wait - retval)));
#endif SZDEBUG
    	        siiaddr->sii_cstat = SII_SCH;
    	        siiaddr->sii_comm = 0;
	    }
	}
    }
    return(SZ_RET_ABORT);
}

/******************************************************************
 *
 * Name:	sii_startdma
 *
 * Abstract:	Perform the DATA IN/DATA OUT PHASE on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * iodir	The IO direction (DMA read or DMA write).
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_startdma(sc, iodir, targid)
register struct sz_softc *sc;
int iodir;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    u_char *byteptr;
    char *stv, *bufp;
    int datacnt, i;
    int retval, offset; 
    int tmp_phase;
    int tmp_state;
    int dmacount;
    struct format_params *fp;
    struct reassign_params *rp;
    struct read_defect_params *rdp;
    struct defect_descriptors *dd;
    struct mode_sel_sns_params *msp;
    struct io_uxfer *iox;

    /*
     * Handle non READ/WRITE scsi commands that transfer data.
     */
    if ( (sc->sz_opcode != SZ_WRITE) && (sc->sz_opcode != SZ_READ) &&
         (sc->sz_opcode != SZ_WRITE_10) && (sc->sz_opcode != SZ_READ_10) ) {
	byteptr = (u_char *)&sc->sz_dat[targid];
	switch(sc->sz_opcode) {
	case SZ_MODSEL:
	    byteptr = (u_char *)&sc->sc_dat[0];
	    datacnt = (int)sc->sz_modsel.pll;

	    if(sc->sc_rzspecial[targid]) {
	        msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
                byteptr = (u_char *)sc->sc_rzaddr[targid];
                datacnt = msp->msp_length;
	    }
#ifdef SZDEBUG
    	    PRINTD(targid, 0x2, ("sii_startdma: mode select data:")); 
    	    for(i=0; i < datacnt; i++) {
		PRINTD(targid, 0x2, (" %x", *(byteptr+i)));
    	    }
    	    PRINTD(targid, 0x2, ("\n"));
#endif SZDEBUG
	    break;	

	case SZ_RQSNS:
	    byteptr = (u_char *)&sc->sc_sns[targid];
	    datacnt = sc->sz_rqsns.alclen;
	    break;

	case SZ_INQ:
	    datacnt = SZ_INQ_MAXLEN;
	    if(sc->sc_rzspecial[targid])
                byteptr = (u_char *)sc->sc_rzaddr[targid];
	    break;

	case SZ_RDCAP:
	    datacnt = SZ_RDCAP_LEN;
	    break;

	case SZ_MODSNS:
/*
 * Why was it this way? Fred -- 7/16/89
	    if(sc->sc_devtyp[targid] & SZ_TAPE)
	        datacnt = SZ_MODSNS_LEN;
*/
	    datacnt = (int) sc->sz_modsns.alclen;
	    if(sc->sc_rzspecial[targid]) {
	        msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
                byteptr = (u_char *)sc->sc_rzaddr[targid];
                datacnt = msp->msp_length;
	    }
	    break;

	case SZ_RECDIAG:
	    datacnt = SZ_RECDIAG_LEN;
	    break;

	case SZ_REASSIGN:
            rp = (struct reassign_params *)sc->sc_rzparams[targid];
            byteptr = (u_char *)sc->sc_rzparams[targid];
            datacnt = ((rp->rp_header.defect_len0 << 0) & 0x00ff) +
    		      ((rp->rp_header.defect_len1 << 8) & 0xff00) + 4;
            break;

	case SZ_FORMAT:
            dd = (struct defect_descriptors *)sc->sc_rzaddr[targid];
            byteptr = (u_char *)sc->sc_rzaddr[targid];
            datacnt = ((dd->dd_header.fu_hdr.defect_len0 << 0) & 0x00ff) +
    		      ((dd->dd_header.fu_hdr.defect_len1 << 8) & 0xff00) + 4;
	    break;

	case SZ_RDD:
            rdp = (struct read_defect_params *)sc->sc_rzparams[targid];
            byteptr = (u_char *)sc->sc_rzaddr[targid];
            datacnt = rdp->rdp_alclen;
            break;

	case SZ_READL:
	case SZ_WRITEL:
	    iox = (struct io_uxfer *)sc->sc_rzparams[targid];
            byteptr = (u_char *)sc->sc_rzaddr[targid];
	    datacnt = iox->io_cnt;
	    break;
	
	default:
#ifdef SZDEBUG
	    PRINTD(targid, 0x4,
		("sii_startdma: unknown scsi cmd 0x%x\n",sc->sz_opcode));
#endif SZDEBUG
	    return(SZ_RET_ABORT);
	    break;
	}

	/*
         * Setup softc structure entries for special SCSI DISK
	 * commands that do dma. (FORMAT UNIT), (READ DEFECT DATA),
	 * (REASSIGN BLOCK), (MODE SELECT), (MODE SENSE) and
	 * (INQUIRY).
	 */
        if(sc->sc_rzspecial[targid] &&
		(sc->sc_curcmd[targid] == sc->sc_actcmd[targid])) {
	    if(!(sc->sc_szflags[targid] & SZ_DMA_DISCON)) {
	        sc->sc_b_bcount[targid] = datacnt;
	        sc->sc_bpcount[targid] = datacnt;
	        sc->sc_bufp[targid] = (char *)byteptr;
	        sc->sc_xfercnt[targid] = 0;
	    }
	    goto SETUP_DMA;
        }

	/* Put the data onto the scsi bus */
	if(iodir == SZ_DMA_WRITE) {
	    if(sii_senddata(sc, targid, byteptr, datacnt, 0) != SZ_SUCCESS)
		return(SZ_RET_ABORT);
	}

	/* Get the data from the scsi bus */
        else {
	    if(sii_recvdata(sc, targid, byteptr, datacnt) != SZ_SUCCESS)
		return(SZ_RET_ABORT);
        }

#ifdef SZDEBUG
	if(sc->sz_opcode == SZ_RQSNS)
    	    PRINTD(targid, 0x4, ("", sii_print_sense(sc,targid)));
#endif SZDEBUG
    } 
    else {
SETUP_DMA:
        /*
         * Start of DMA code for a READ or WRITE scsi command, setup
         * the count, the RAM buffer offset, and the DMA registers.
         */
	siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
        stv = sc->sc_rambuff + sc->sc_dboff[targid];
	/* Handle the case of the DMA being disconnected */
	if(sc->sc_szflags[targid] & SZ_DMA_DISCON) {
#ifdef SZDEBUG
            PRINTD(targid, 0x1,
	    ("sii_startdma: continuing DMA %s transfer of %d bytes\n",
	    (iodir == SZ_DMA_WRITE)?"WRITE":"READ",sc->sc_bpcount[targid]));
#endif SZDEBUG

	    sc->sc_szflags[targid] &= ~SZ_DMA_DISCON;

            /* Setup the dmacount and the offset into the RAM buffer */
	    offset = sc->sc_bpcount[targid] - sc->sc_siidmacount[targid];
	    if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		dmacount = SII_MAX_DMA_XFER_LENGTH;
	    else
		dmacount = sc->sc_siidmacount[targid];
	}
	/* Handle the case of the DMA just starting */
        else {
#ifdef SZDEBUG
            PRINTD(targid, 0x1,
	    ("sii_startdma: starting DMA %s transfer of %d bytes\n",
	    (iodir == SZ_DMA_WRITE)?"WRITE":"READ",sc->sc_bpcount[targid]));
#endif SZDEBUG

	    /* Copy data to RAM buffer if DMA WRITE operation */
    	    if(iodir == SZ_DMA_WRITE) {
		bufp = sc->sc_bufp[targid] + sc->sc_xfercnt[targid];
/*    	        bcopy (bufp, stv, sc->sc_b_bcount[targid]);	*/
    	        blkcpy (bufp, stv, sc->sc_b_bcount[targid]);
	    }

            /* Setup the dmacount and the offset into the RAM buffer */
	    sc->sc_siidmacount[targid] = sc->sc_bpcount[targid];
	    offset = 0;
	    if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		dmacount = SII_MAX_DMA_XFER_LENGTH;
	    else
		dmacount = sc->sc_siidmacount[targid];
        }
    

        /*
         * Set the starting address in the 128K buffer.
         */
        siiaddr->sii_dmaddrl = ((sc->sc_dboff[targid] + offset) & 0xffff);
        siiaddr->sii_dmaddrh = ((sc->sc_dboff[targid] + offset) >> 16);
    	siiaddr->sii_dmlotc = dmacount;
	siiaddr->sii_dmabyte = sc->sc_siioddbyte[targid];
    
        /*
         * Start the DMA transfer.
         */
        sc->sc_szflags[targid] |= SZ_DID_DMA;
        tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
        tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
	if(!scsi_polled_mode)
    	    siiaddr->sii_csr |= SII_IE;
        siiaddr->sii_comm = (SII_DMA | SII_INXFER | tmp_state | tmp_phase);
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Name:	sii_sendcmd
 *
 * Abstract:	Perform the COMMAND PHASE on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_sendcmd(sc, targid)
register struct sz_softc *sc;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    u_char *byteptr;
    int datacnt, i;
    int cmd_type;
    int cmdcnt;

    sc->sc_savcnt[targid] = 0;
    sc->sc_status[targid] = 0xff;
    sc->sc_siidmacount[targid] = 0;
    siiaddr->sii_dmlotc = 0;
    sii_sent_cmd = 1;
    byteptr = (u_char *)&sc->sz_command;
    cmd_type = *byteptr;
    cmdcnt = sz_cdb_length (cmd_type, targid);

#ifdef SZDEBUG
    PRINTD(targid, 0x2, ("sii_sendcmd: (targid=%d) scsi cmd pkt:", targid)); 
    for(i=0; i < cmdcnt; i++) {
	PRINTD(targid, 0x2, (" %x", *(byteptr+i)));
    }
    PRINTD(targid, 0x2, ("\n"));
#endif SZDEBUG

    /* Put the scsi command onto the scsi bus */
    if(sii_senddata(sc, targid, byteptr, cmdcnt, 0) != SZ_SUCCESS)
	return(SZ_RET_ABORT);

    /* Statistics update for READS and WRITES */
    if ( (cmd_type == SZ_WRITE) || (cmd_type == SZ_READ) ||
         (cmd_type == SZ_WRITE_10) || (cmd_type == SZ_READ_10) ) {
	if(sc->sc_dkn[targid] >= 0) {
	    dk_busy |= 1 << sc->sc_dkn[targid];
	    dk_xfer[sc->sc_dkn[targid]]++;
	    dk_wds[sc->sc_dkn[targid]] += sc->sc_bpcount[targid] >> 6;
	}
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Name:	sii_getstatus
 *
 * Abstract:	Perform the STATUS PHASE on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_getstatus(sc, targid)
register struct sz_softc *sc;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;

    /* Get the status byte from the scsi bus */
    if(sii_recvdata(sc, targid, &sc->sc_status[targid], 1) != SZ_SUCCESS)
	return(SZ_RET_ABORT);
    /* Save the status byte for the error log */
    if (sc->sc_curcmd[targid] == sc->sc_actcmd[targid])
	sc->sc_statlog[targid] = sc->sc_status[targid];

#ifdef SZDEBUG
    PRINTD(targid, 0x1,
	("sii_getstatus: status byte = 0x%x = ",sc->sc_status[targid]));
    PRINTD(targid, 0x1, ("", sii_print_status((int)sc->sc_status[targid])));
#endif SZDEBUG

    /* Check the status */
    if(sc->sc_status[targid] != SZ_GOOD) {
	sc->sc_szflags[targid] |= SZ_NEED_SENSE;
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Name:	sii_msgout
 *
 * Abstract:	Perform the MESSAGE OUT PHASE on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_msgout(sc, targid)
register struct sz_softc *sc;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    u_char messg;
    int retval, i;
    int lun = sc->sz_t_read.lun;

    /*
     * If the current state isn't SZ_SELECT then this message out
     * phase is incorrect.  Send a NOP to get the target out of this
     * phase.  NOTE -- this will need to be changed when we support
     * targets as initiators.  This code is being added to get around
     * the sii's selection problem where the select command is ignored
     * leaving ATN asserted which causes the target to go to msgout.
     */
    if(sc->sc_selstat[targid] != SZ_SELECT) {
	messg = SZ_NOP;

#ifdef SZDEBUG
        PRINTD(targid, 0x1,
	    ("sii_msgout: sending NOP Message\n"));
#endif SZDEBUG

        /* Put the NOP Message onto the scsi bus */
        sii_senddata(sc, targid, &messg, 1, 0);
	return(SZ_SUCCESS);
    }

    /* Clear the assert attention flag */
    sc->sc_szflags[targid] &= ~SZ_ASSERT_ATN;

    /* Check if we need to send a Message Reject Message */
    if(sc->sc_szflags[targid] & SZ_REJECT_MSG) {
        sc->sc_szflags[targid] &= ~SZ_REJECT_MSG;
	messg = SZ_MSGREJ;

#ifdef SZDEBUG
        PRINTD(targid, 0x1,
	    ("sii_msgout: sending Message Reject Message\n"));
#endif SZDEBUG

        /* Put the Message Reject Message onto the scsi bus */
        if(sii_senddata(sc, targid, &messg, 1, 0) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
    }

    /* Check if we need to send a Bus Device Reset Message */
    else if(sc->sc_szflags[targid] & SZ_RESET_DEV) {
	if(sc->sc_siireqack[targid] != 0)
	    sc->sc_siisentsync[targid] = 0;
	messg = SZ_DEVRST;

#ifdef SZDEBUG
        PRINTD(targid, 0x1,
	     ("sii_msgout: sending Bus Device Reset Message\n"));
#endif SZDEBUG

        /* Put the Bus Device Reset Message onto the scsi bus */
        if(sii_senddata(sc, targid, &messg, 1, 0) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
    }

    /* Check if we need to send an Abort Message */
    else if(sc->sc_szflags[targid] & SZ_ABORT_CMD) {
	messg = SZ_ABT;

#ifdef SZDEBUG
        PRINTD(targid, 0x1,
	    ("sii_msgout: sending Abort Message\n"));
#endif SZDEBUG

        /* Put the Bus Device Reset Message onto the scsi bus */
        if(sii_senddata(sc, targid, &messg, 1, 0) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
    }

    /* Send the Identify Message with or without disconnects */
    else {
	/* Setup for disconnects or no disconnects */
        if(scsi_polled_mode || sii_no_disconnects)
            messg = SZ_ID_NODIS | lun;	/* Allow no disconnects */
        else
            messg = SZ_ID_DIS | lun;  	/* Allow disconnects */

        /* Check if we need to send a Synchronous DataXfer Message */
	if(!sc->sc_siisentsync[targid]) {

#ifdef SZDEBUG
            PRINTD(targid, 0x1,
		("sii_msgout: sending Identify Message = 0x%x\n",messg));
#endif SZDEBUG

            /* Put the Identify Message onto the scsi bus */
            if(sii_senddata(sc, targid, &messg, 1, SII_ATN) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);

	    /* Now setup the Synchronous Data Xfer Message */
	    sc->sc_siisentsync[targid] = 1;
            sc->sc_extmessg[targid][0] = SZ_EXTMSG;
            sc->sc_extmessg[targid][1] = 0x3;
            sc->sc_extmessg[targid][2] = SZ_SYNC_XFER;
            sc->sc_extmessg[targid][3] = 25;
            sc->sc_extmessg[targid][4] = SII_SYNC;

#ifdef SZDEBUG
            PRINTD(targid, 0x1,
		("sii_msgout: sending Sync Data Transfer Message\n"));
    	    PRINTD(targid, 0x1, ("sii_msgout: extended message data:")); 
    	    for(i=0; i<5; i++) {
		PRINTD(targid, 0x1, (" %x", sc->sc_extmessg[targid][i]));
	    }
    	    PRINTD(targid, 0x1, ("\n"));
#endif SZDEBUG

            /* Put the Synchronous Data Xfer Message onto the scsi bus */
            if(!(siiaddr->sii_dstat & SII_MIS) &&
		    sii_senddata(sc, targid, &sc->sc_extmessg[targid][0], 
				4, SII_ATN) != SZ_SUCCESS) {
		if(siiaddr->sii_dstat & SII_MIS)
		    return(SZ_SUCCESS);
		else
	            return(SZ_RET_ABORT);
	    }

	    /* Send out the last byte of the Synchronous Data Xfer Message */
            if(!(siiaddr->sii_dstat & SII_MIS) &&
		    sii_senddata(sc, targid, &sc->sc_extmessg[targid][4], 
				1, 0) != SZ_SUCCESS) {
		if(siiaddr->sii_dstat & SII_MIS)
		    return(SZ_SUCCESS);
		else
	            return(SZ_RET_ABORT);
	    }
        }
	else {

#ifdef SZDEBUG
            PRINTD(targid, 0x1,
		("sii_msgout: sending Identify Message = 0x%x\n",messg));
#endif SZDEBUG

            /* Put the Identify Message onto the scsi bus */
            if(sii_senddata(sc, targid, &messg, 1, 0) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);
	}
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Name:	sii_msgin
 *
 * Abstract:	Perform the MESSAGE IN PHASE on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_msgin(sc, targid)
register struct sz_softc *sc;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int len, i;
    int retval, save_dmlotc;
    int flags;

    /*
     * We must save the value in DMA Length Of Transfer Count 
     * register if we are not using programmed io to receive the 
     * message data. The reason for this is that if the message 
     * is a SAVE DATA POINTER message than we will lose the saved 
     * count in the DMA Length Of Transfer Count register when we
     * use DMA to receive the message.
     */
    if(!sii_use_programmed_io)
	save_dmlotc = siiaddr->sii_dmlotc;

    /* Get the message from the scsi bus */
    if(sii_recvdata(sc, targid, &sc->sc_message[targid], 1) != SZ_SUCCESS)
	return(SZ_RET_ABORT);

    /* Switch on the type of message recieved */
    switch(sc->sc_message[targid]) {
    case SZ_CMDCPT:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_CMDCPT message\n"));
#endif SZDEBUG
	    sc->sc_fstate = 0;
	    sc->sc_szflags[targid] &= ~(SZ_DID_DMA | SZ_DMA_DISCON);
	    /* Assumes one command at a time for each target */
	    if(sc->sc_dkn[targid] >= 0)
		dk_busy &= ~(1 << sc->sc_dkn[targid]);
	    scsi_completed[targid] = 1;
	    break;

    case SZ_SDP:
	    if(!sii_use_programmed_io)
	        sc->sc_savcnt[targid] = save_dmlotc;
	    else
	        sc->sc_savcnt[targid] = siiaddr->sii_dmlotc;
	    siiaddr->sii_dmlotc = 0;
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	    if((sc->sc_prevpha != SZ_DATAI_PHA) &&
	       		(sc->sc_prevpha != SZ_DATAO_PHA))
	        sc->sc_savcnt[targid] = 0;

#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_SDP message: savcnt = %d\n",
					sc->sc_savcnt[targid]));
#endif SZDEBUG

            /* Read the disconnect message now */
            if(sii_recvdata(sc, targid, &sc->sc_message[targid], 1) 
				!= SZ_SUCCESS)
	        return(SZ_RET_ABORT);
	    if(sc->sc_message[targid] != SZ_DISCON)
		break;

    case SZ_DISCON:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_DISCON message\n"));
#endif SZDEBUG
	    sc->sc_szflags[targid] |= SZ_WAS_DISCON;
	    siiaddr->sii_dmlotc = 0;
	    if (sc->sc_siidmacount[targid] != 0) {
	        sc->sc_szflags[targid] |= SZ_DMA_DISCON;
	    }
	    break;
		
    case SZ_EXTMSG:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_EXTMSG message\n"));
#endif SZDEBUG
	    sc->sc_extmessg[targid][0] = sc->sc_message[targid];

            /* Read the extended message length */
            if(sii_recvdata(sc, targid, &sc->sc_extmessg[targid][1], 1) 
				!= SZ_SUCCESS)
	        return(SZ_RET_ABORT);

	    len = (int)sc->sc_extmessg[targid][1];

	    /*
	     * At this time the only extended messaged which is
	     * supported is SZ_SYNC_XFER.  If the incoming message
	     * isn't a SZ_SYNC_XFER, assert ATN now!  This will
	     * prevent the target from becomming confused and 
	     * beleiving that the message was accepted.
	     */
	    if(sc->sc_extmessg[targid][0] != SZ_SYNC_XFER) {
		    sc->sc_szflags[targid] |= SZ_REJECT_MSG;
		    siiaddr->sii_comm |= SII_ATN;
	    }

            /* Read the extended message */
            if(sii_recvdata(sc, targid, &sc->sc_extmessg[targid][2], len) 
				!= SZ_SUCCESS)
	        return(SZ_RET_ABORT);
#ifdef SZDEBUG
    	    PRINTD(targid, 0x1, ("sii_msgin: extended message data:")); 
    	    for(i=0; i<(len+2); i++)
		PRINTD(targid, 0x1, (" %x", sc->sc_extmessg[targid][i]));
    	    PRINTD(targid, 0x1, ("\n"));
#endif SZDEBUG

	    /*
	     * If the extended message is a Synchronous Data
	     * Transfer Request message then set the REQ/ACK
	     * offset for the current target otherwise reject
	     * the message.
	     *
	     */
	    if(sc->sc_extmessg[targid][0] == SZ_SYNC_XFER) {
		if(sc->sc_extmessg[targid][4] > SII_SYNC)
		    sc->sc_extmessg[targid][4] = SII_SYNC;
		sc->sc_siireqack[targid] = sc->sc_extmessg[targid][4];
	    }
	    else {
		sc->sc_szflags[targid] |= SZ_ASSERT_ATN;
		sc->sc_szflags[targid] |= SZ_REJECT_MSG;
	    }
	    break;

    case SZ_ID_NODIS:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_ID_NODIS message\n"));
#endif SZDEBUG
	    break;

    case SZ_ID_DIS:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_ID_DIS message\n"));
#endif SZDEBUG
	    break;

	case SZ_RDP:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_RDP message\n"));
#endif SZDEBUG
	    break;

	case SZ_MSGREJ:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_MSGREJ message\n"));
#endif SZDEBUG
	    break;

	case SZ_LNKCMP:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_LNKCMP message\n"));
#endif SZDEBUG
	    break;

	case SZ_LNKCMPF:
#ifdef SZDEBUG
	    PRINTD(targid, 0x1, ("sii_msgin: SZ_LNKCMPF message\n"));
#endif SZDEBUG
	    break;

    default:
	    flags = SZ_HARDERR | SZ_LOGMSG;
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x73, 0, flags);
#ifdef SZDEBUG
	    PRINTD(targid, 0x4, ("sii_msgin: unknown message = 0x%x\n",
		sc->sc_message[targid]));
#endif SZDEBUG
	    return(SZ_RET_ABORT);
    }

    /*
     * Assert attention as long as the SZ_ASSERT_ATN flag is set.
     * Attention gets deasserted during a message out phase and
     * the SZ_ASSERT_ATN flag gets cleared.
     */
    if(sc->sc_szflags[targid] & SZ_ASSERT_ATN)
	siiaddr->sii_comm |= SII_ATN;
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Name:	sii_senddata
 *
 * Abstract:	Send data to the scsi bus.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 * data		Pointer to data buffer of data to send.
 * count	The number of bytes of data to send.
 * attn		Value to set ATTN bit (1 set ATTN) (0 don't set ATTN).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_senddata(sc, targid, data, count, attn)
register struct sz_softc *sc; int targid;
u_char *data;
int count;
int attn;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int retval;
    int tmp_state;
    int tmp_phase;
    int timer, i;

    /* Move the SII to the new phase */
    tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
    tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
    siiaddr->sii_comm = (tmp_state | tmp_phase | attn);

    /* Send the data to the scsi bus using programmed IO */
    if(sii_use_programmed_io && (siiaddr->sii_dmctrl == 0)) {
        for(i=0; i<count; i++) {
    	    SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_TBE|SII_MIS)),sii_wait_count,retval);
    	    if (retval >= sii_wait_count) {
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4a, 0, SZ_HARDERR);
#ifdef SZDEBUG
    	        PRINTD(targid, 0x4, ("sii_senddata: SII_TBE not set\n"));
#endif SZDEBUG
    	        return(SZ_RET_ABORT);
    	    }

    	    /* If a phase change occured then we are done */
    	    if(siiaddr->sii_dstat & SII_MIS)
    	        break;
    	    siiaddr->sii_data = *data++;
    	    tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
    	    tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
    	    siiaddr->sii_comm = (SII_INXFER | tmp_state | tmp_phase | attn);
        }
        siiaddr->sii_comm &= ~SII_INXFER;
        siiaddr->sii_dstat = SII_DNE;
        return(SZ_SUCCESS);
    }

    /* Send the data to the scsi bus using DMA */
    else {
    	bcopy(data, (sc->sc_rambuff + sc->sc_siidboff[targid]), count);
    	siiaddr->sii_dmaddrl = (sc->sc_siidboff[targid] & 0xffff);
    	siiaddr->sii_dmaddrh = (sc->sc_siidboff[targid] >> 16);
    	siiaddr->sii_dmlotc = count;
        siiaddr->sii_comm = 
			(SII_DMA | SII_INXFER | tmp_state | tmp_phase | attn);

	/* Wait for DMA to complete */
        SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_DNE|SII_MIS)),sii_wait_count,retval);
        siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
	timer = 1000;
	while(--timer && !(siiaddr->sii_dstat & SII_DNE));
	siiaddr->sii_dstat = SII_DNE;
	siiaddr->sii_dmlotc = 0;
	if(retval >= sii_wait_count && !(siiaddr->sii_dstat & SII_MIS)) {
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x48, 0, SZ_HARDERR);
	    return(SZ_RET_ABORT);
	}
	else
	    return(SZ_SUCCESS);
    }
}

/******************************************************************
 *
 * Name:	sii_recvdata
 *
 * Abstract:	Recieve data from the scsi bus.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 * data		Pointer to data buffer of data to recieve.
 * count	The number of bytes of data to recieve.
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_recvdata(sc, targid, data, count)
register struct sz_softc *sc;
int targid;
u_char *data;
int count;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int retval;
    int tmp_state;
    int tmp_phase;
    int timer, i;

    /* Move the SII to the new phase */
    tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
    tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
    siiaddr->sii_comm = (tmp_state | tmp_phase);

    /* Recieve the data from the scsi bus using programmed IO */
    if(sii_use_programmed_io && (siiaddr->sii_dmctrl == 0)) {
        for(i=0; i<count; i++) {
    	    SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_IBF|SII_MIS)),sii_wait_count,retval);
    	    if (retval >= sii_wait_count) {
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x49, 0, SZ_HARDERR);
#ifdef SZDEBUG
    	        PRINTD(targid, 0x4, ("sii_recvdata: SII_IBF not set\n"));
#endif SZDEBUG
    	        return(SZ_RET_ABORT);
    	    }
    
    	    /* If a phase change occured then we are done */
    	    if(siiaddr->sii_dstat & SII_MIS)
    	        break;
    	    *data++ = siiaddr->sii_data;
    	    tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
    	    tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
    	    siiaddr->sii_comm = (SII_INXFER | tmp_state | tmp_phase);
        }
        siiaddr->sii_comm &= ~SII_INXFER;
        siiaddr->sii_dstat = SII_DNE;
        return(SZ_SUCCESS);
    }

    /* Recieve the data from the scsi bus using DMA */
    else {
    	siiaddr->sii_dmaddrl = (sc->sc_siidboff[targid] & 0xffff);
    	siiaddr->sii_dmaddrh = (sc->sc_siidboff[targid] >> 16);
    	siiaddr->sii_dmlotc = count;
        siiaddr->sii_comm = (SII_DMA | SII_INXFER | tmp_state | tmp_phase);

	/* Wait for DMA to complete */
        SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_DNE|SII_MIS)),sii_wait_count,retval);
        siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
	timer = 1000;
	while(--timer && !(siiaddr->sii_dstat & SII_DNE));
	siiaddr->sii_dstat = SII_DNE;
	siiaddr->sii_dmlotc = 0;
    	bcopy((sc->sc_rambuff + sc->sc_siidboff[targid]), data, count);
	if(retval >= sii_wait_count && !(siiaddr->sii_dstat & SII_MIS)) {
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x47, 0, SZ_HARDERR);
	    return(SZ_RET_ABORT);
	}
	else
	    return(SZ_SUCCESS);
    }
}

/******************************************************************
 *
 * Name:	sii_clear_discon_io_requests
 *
 * Abstract:	Clear all Active and Disconnected IO requests due 
 *		to a BUS RESET.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_clear_discon_io_requests(sc)
register struct sz_softc *sc;
{

    int targid;
    int unit;
    struct buf *dp, *bp;

    /* Find all targets that have IO requests Active and Disconnected */
    for(targid=0; targid<NDPS; targid++) {
	if(sc->sc_siireqack[targid] != 0)
	    sc->sc_siisentsync[targid] = 0;
	if(targid == sc->sc_sysid)	/* skip initiator */
	    continue;
	if(sc->sc_alive[targid] == 0)	/* non existent target */
	    continue;
	unit = sc->sc_unit[targid];
	dp = (struct buf *)&szutab[unit];
	if(dp->b_active) {		/* target is active */
#ifdef SZDEBUG
	    PRINTD(-1, 0x4,
	        ("sii_clear_discon_io_requests: clearing ID %d\n",targid));
#endif SZDEBUG
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4b, 0, SZ_HARDERR);
	    dp->b_active = 0;
	    sc->sc_xstate[targid] = SZ_NEXT;
	    sc->sc_xevent[targid] = SZ_BEGIN;

	    /* Clear any pending timeout for this target. */
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }

#ifdef	notdef
	    /* Fix "bp->b_command" if this was a special command. */
	    bp = dp->b_actf;
	    if(bp == &cszbuf[unit])
	        bp->b_command = sc->sc_curcmd[targid];
#endif	notdef
	}
	sc->sc_selstat[targid] = SZ_IDLE;
	sc->sc_szflags[targid] = SZ_NORMAL;
	sc->sc_flags[targid] &= ~DEV_HARDERR;
    }
}

/******************************************************************
 *
 * Name:	sii_restart_target
 *
 * Abstract:	Allow a target to restart SCSI operations after 
 *		a BUS DEVICE RESET occured.
 *
 * Inputs:
 * unit		The ULTRIX logical unit number of the scsi device.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_restart_target(unit)
int unit;
{

    int cntlr, targid, s, retval;
    register struct sz_softc *sc;
    register struct sii_regs *siiaddr;
    struct buf *dp, *bp;

    s = spl5();
    cntlr = (unit >> 3) & 1;
    siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    sc = &sz_softc[cntlr];
    targid = unit & 7;
    dp = (struct buf *)&szutab[unit];

    /* Clear any pending timeout for this target */
    if(sc->sc_szflags[targid] & SZ_TIMERON) {
        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
        sc->sc_szflags[targid] &= ~SZ_TIMERON;
    }

    /* Clear all associated states and flags for this target */
    dp->b_active = 0;
    sc->sc_xstate[targid] = SZ_NEXT;
    sc->sc_xevent[targid] = SZ_BEGIN;
    sc->sc_szflags[targid] = SZ_NORMAL;
    sc->sc_flags[targid] &= ~DEV_HARDERR;
    sc->sc_selstat[targid] = SZ_IDLE;
    if(sc->sc_siireqack[targid] != 0)
	sc->sc_siisentsync[targid] = 0;

#ifdef	notdef
    /* Fix "bp->b_command" if this was a special command. */
    bp = dp->b_actf;
    if(bp == &cszbuf[unit])
        bp->b_command = sc->sc_curcmd[targid];
#endif	notdef

#ifdef SZDEBUG
    PRINTD(-1, 0x4, 
	("sii_restart_target: restarting scsi target %d after device reset\n",
		targid));
#endif SZDEBUG

    /* If the SCSI bus is not busy then restart this target */
    if(sc->sc_active == 0)
	sz_start(sc, targid);
    splx(s);
}

/******************************************************************
 *
 * Name:	sii_reset
 *
 * Abstract:	Reset the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_reset(sc)
register struct sz_softc *sc;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int targid, s;
    int unit;
    struct buf *dp, *bp;

    /*
     * Reset the SII if a reset is not already in progress.
     * If "sc->sc_rip" is zero then no reset is in progress.
     */
    if(sc->sc_rip == 0) {
        /* Reset the SII chip. */
        siiaddr->sii_comm = SII_CHRESET;
        /* Set arbitrated bus mode. */
        siiaddr->sii_csr = SII_HPM;
        /* SII is always ID 7. */
        siiaddr->sii_id = SII_ID_IO | sc->sc_sysid; 
        /* Enable SII to drive SCSI bus. */
        siiaddr->sii_dictrl = SII_PRE;
        /*
         * Assert SCSI bus reset for at least 25 Usec to clear
         * the world. SII_RST is self clearing.
         */
        siiaddr->sii_comm = SII_RST;
        DELAY(25); DELAY(25);
	/* Clear away any pending interrupts from the reset. */
        siiaddr->sii_cstat = 0xffff;
        siiaddr->sii_dstat = 0xffff;
        /*
         * Set up SII for arbitrated bus mode, SCSI parity checking,
         * Select Enable, Reselect Enable, and Interrupt Enable.
         */
        siiaddr->sii_csr = (SII_HPM | SII_RSE | SII_SLE | SII_PCE | SII_IE);

        /* Clear all Active and Disconnected IO requests. */
        if(!scsi_polled_mode) {
            sii_clear_discon_io_requests(sc);
	    sc->sc_rip = 1;
            timeout(sii_reset, (caddr_t)sc, sz_wait_for_devices*hz);
	}
	else {
	    /* Clear all the sent synchronous transfer message flags. */
    	    for(targid=0; targid<NDPS; targid++)
	        sc->sc_siisentsync[targid] = 0;
    	    /* Figure out the current active target. */
    	    for(targid=0; targid<NDPS; targid++)
    	        if(sc->sc_active & (1 << targid))
        	    break;
    	    if(targid >= NDPS)
		targid = -1;
	    if(targid != -1 && sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
            DELAY(sz_wait_for_devices * 1000000);
	}
	/* Clear away any pending interrupts from the reset. */
        siiaddr->sii_cstat = 0xffff;
        siiaddr->sii_dstat = 0xffff;
    }
    /*
     * A reset is in progress. Call back into "sz_start()" to 
     * restart all scsi driver activity.
     */
    else {
	/* Clear away any pending interrupts from the reset. */
        s = spl5();
        siiaddr->sii_cstat = 0xffff;
        siiaddr->sii_dstat = 0xffff;
#ifdef SZDEBUG
	PRINTD(-1, 0x4, 
	    ("sii_reset: restarting scsi driver activity after reset\n"));
#endif SZDEBUG
    	/* Figure out the current active target. */
    	for(targid=0; targid<NDPS; targid++)
    	    if(sc->sc_active & (1 << targid))
                break;
    	if(targid >= NDPS)
	    targid = -1;

#ifdef	notdef
	/* Fix "bp->b_command" if this was a special command. */
	if(targid != -1) {
	    unit = sc->sc_unit[targid];
	    dp = (struct buf *)&szutab[unit];
	    bp = dp->b_actf;
	    if(bp == &cszbuf[unit])
	        bp->b_command = sc->sc_curcmd[targid];
	}
#endif	notdef
	sc->sc_rip = 0;
        sc->sc_active = 0;
	sz_start(sc, targid);
	splx(s);
    }
}

/******************************************************************
 *
 * Name:	sii_intr
 *
 * Abstract:	Handle interrupts from the SII chip.
 *
 * Inputs:
 * cntlr	SCSI bus controller number (0 = bus A, 1 = bus B).
 *
 * Outputs:	None.
 *
 * Return values: None.
 *
 * Interrupts Handled:
 *		State Change interrupt (Connect/Reselect/Disconnect).
 *		Phase Change interrupt.
 *		Bus Error interrupt (IGNORED).
 *		Parity Error interrupt (CAUSES COMMAND ABORT).
 *		Bus Reset interrupt.
 *		DMA Complete interrupt.
 *		Transmit Buffer Empty interrupt.
 *		Input Buffer Full interrupt.
 ******************************************************************/
sii_intr(cntlr)
int cntlr;
{
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    register struct sz_softc *sc = &sz_softc[cntlr];
    int targid, retval, timer;
    int dmacount, offset, tmp_phase, tmp_state;
    int handle_reselect;
    int flags;

    /* Ingore interrupts if a bus reset is in progress */
    if(sc->sc_rip)
	return;

    /* Initialize variables */
    scsi_bus_idle = 0;
    handle_reselect = 0;

    /* Figure out the current active target */
    for(targid=0; targid<NDPS; targid++)
	if(sc->sc_active & (1 << targid))
	    break;
    if(targid >= NDPS)
	targid = -1;

    /* Check if there are valid interrupts pending on the SII */
    if(!(siiaddr->sii_cstat & (SII_CI|SII_DI))) {
	/* No interrupts pending, spurious interrupt occurred */
#ifdef SZDEBUG
	PRINTD(targid, 0x10,
	    ("sii_intr: spurious interrupt occurred on the SII\n"));
#endif SZDEBUG
	return;
    }

    /* Check for interrupt from a disconnected target */
    if(targid == -1 || sc->sc_active == 0) {
	/* We must wait for a STATE CHANGE to occur first */
	timer = 1000;
	while(--timer && !(siiaddr->sii_cstat & SII_SCH));
#ifdef SZDEBUG
	PRINTD((siiaddr->sii_destat & SII_IDMSK), 0x20,
	("sii_intr: (STATE reselect wait 1) start=1000 end=%d\n", timer));
#endif SZDEBUG
	if(timer == 0) {
	    /*
	     * Check if a reselect occurred without the STATE
	     * CHANGE bit (SII_SCH) being set. This condition
	     * can occur when a reselect immediately follows a
	     * disconnect and therefore we only get one STATE
	     * CHANGE interrupt.
	     */
	    if(siiaddr->sii_cstat & SII_DST_ONBUS)
	        handle_reselect = 1;
	    else 
	        return;
	}
    }
	
    /* Loop through all bus phases until command complete */
    do {
	/* If no more interrupts pending then return */
	if(!(siiaddr->sii_cstat & (SII_CI|SII_DI)) && !handle_reselect)
	    return;

        /* Check for a BUS ERROR */
        if(siiaddr->sii_cstat & SII_BER)
  	    siiaddr->sii_cstat = SII_BER;

        /* Check for a PARITY ERROR */
        if(siiaddr->sii_dstat & SII_IPE) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 1, 0, flags);
#ifdef SZDEBUG
	    PRINTD(targid, 0x4, ("sii_intr: scsi %d parity error\n", cntlr));
#endif SZDEBUG
	    goto HANDLE_ERROR;
        }

        /* Check for a BUS RESET */
        if(siiaddr->sii_cstat & SII_RST_ONBUS) {
  	    siiaddr->sii_cstat = SII_RST_ONBUS;
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 1, 0, SZ_HARDERR);
#ifdef SZDEBUG
	    PRINTD(targid, 0x4, ("sii_intr: scsi %d bus reset\n", cntlr));
#endif SZDEBUG
	    goto HANDLE_ERROR;
        }

        /* Check for a STATE CHANGE */
        while((siiaddr->sii_cstat & SII_SCH) || handle_reselect) {
	    siiaddr->sii_cstat = SII_SCH;
	    handle_reselect = 0;

	    /* Handle the current state change */
	    sii_state_change(sc, &targid);

	    /* Check for a quick Reselect from the Disconnected target */
	    if(scsi_bus_idle) {

		/*
		 * Check for a back-to-back disconnect occurring in 
		 * which case the target ID will be -1 and the SCSI 
		 * bus will be idle. KLUDGE for CDROM device.
		 */
		if(targid == -1)
		    return;

		/*
		 * If a quick Reselect occurred from the same target
	         * that Disconnected then handle it now rather than
		 * wait for another interrupt.
		 */
	        if((siiaddr->sii_cstat & SII_DST_ONBUS) &&
		   (siiaddr->sii_cstat & SII_CON) &&
		   (siiaddr->sii_destat & SII_IDMSK) == targid) {

	    	    /* Wait for the STATE CHANGE bit to get set first */
	    	    timer = 1000;
	    	    while(--timer && !(siiaddr->sii_cstat & SII_SCH));
#ifdef SZDEBUG
		    PRINTD(targid, 0x20, 
	    	    ("sii_intr: (STATE reselect wait 2) start=1000 end=%d\n",
			timer));

	    	    PRINTD(targid, 0x8,
			("sii_intr: quick reselect occurred\n"));
#endif SZDEBUG
		    continue;
		}
	    }
	    break;
	}

	/* If disconnected and went to BUS FREE STATE then break */
	if(scsi_bus_idle)
	    break;

        /* Check for DMA COMPLETION */
        if(siiaddr->sii_dstat & SII_DNE) {
	    siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
	    siiaddr->sii_dstat = SII_DNE;
#ifdef SZDEBUG
	    PRINTD(targid, 0x8, ("sii_intr: DMA transfer done: (dmlotc = %d)\n",
			siiaddr->sii_dmlotc));
#endif SZDEBUG
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	    if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		sc->sc_siidmacount[targid] -= 
			(SII_MAX_DMA_XFER_LENGTH - siiaddr->sii_dmlotc);
	    else
		sc->sc_siidmacount[targid] -= 
			(sc->sc_siidmacount[targid] - siiaddr->sii_dmlotc);
	}

	/* Check for ODD BYTE BOUNDARY condition */
	if((siiaddr->sii_dstat & SII_OBB) && 
	   		(sc->sc_fstate == SZ_DATAI_PHA)) {
	    sc->sc_siioddbyte[targid] = siiaddr->sii_dmabyte;
	}

        /* Check for a PHASE CHANGE */
        if(siiaddr->sii_dstat & SII_MIS) {

            /* Check for a BUS ERROR */
            if(siiaddr->sii_cstat & SII_BER)
  	        siiaddr->sii_cstat = SII_BER;

	    /* If a transfer was in progress, but didn't generate */
	    /* a DNE because the SII was setup for a larger DMA   */
	    /* transfer than actually occurred, then clear the    */
	    /* transfer command and wait for the DNE interrupt.   */
	    if(siiaddr->sii_comm & SII_INXFER) {
	        siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
	        timer = 1000;
	        while(--timer && !(siiaddr->sii_dstat & SII_DNE));
		siiaddr->sii_dstat = SII_DNE;
#ifdef SZDEBUG
		PRINTD(targid, 0x20, 
	    	("sii_intr: (DMA DONE wait) start=1000 end=%d\n", timer));

	    	PRINTD(targid, 0x8, 
		    ("sii_intr: DMA transfer not done: (dmlotc = %d)\n",
			siiaddr->sii_dmlotc));
#endif SZDEBUG

	        /* Clear DMA in progress flag and adjust DMA count */
	        sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	        if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		    sc->sc_siidmacount[targid] -= 
			    (SII_MAX_DMA_XFER_LENGTH - siiaddr->sii_dmlotc);
	        else
		    sc->sc_siidmacount[targid] -= 
			    (sc->sc_siidmacount[targid] - siiaddr->sii_dmlotc);
	    }

	    /* Handle the current bus phase */
	    if(sii_phase_change(sc, targid) != SZ_SUCCESS)
	        goto HANDLE_ERROR;
	}

	/* Check for fragmented DMA transfers (>8K) */
	if((siiaddr->sii_dstat & (SII_TBE|SII_IBF)) &&
	  	!(siiaddr->sii_dstat & SII_MIS) &&
		    !(sc->sc_szflags[targid] & SZ_DID_DMA) &&
	  		((sc->sc_fstate == SZ_DATAI_PHA) || 
				(sc->sc_fstate == SZ_DATAO_PHA))) {

	    /* If the DMA was restarted then return */
	    if(sii_restartdma(sc, targid))
		return;
	    else {
#ifdef SZDEBUG
		PRINTD(targid, 0x4,
		    ("sii_intr: DMA error in sii_restartdma\n"));
#endif SZDEBUG
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4d, 0, SZ_HARDERR);
	        goto HANDLE_ERROR;
	    }
	}

	/* Sometimes the target stays in the same phase */
	if((siiaddr->sii_dstat & (SII_IBF|SII_TBE)) && 
		!(siiaddr->sii_dstat & SII_MIS) &&
			((sc->sc_fstate != SZ_DATAI_PHA) &&
				(sc->sc_fstate != SZ_DATAO_PHA))) {

            /* Check for a BUS ERROR */
            if(siiaddr->sii_cstat & SII_BER)
  	        siiaddr->sii_cstat = SII_BER;

	    /* Handle the current bus phase */
	    if(sii_phase_change(sc, targid) != SZ_SUCCESS)
	        goto HANDLE_ERROR;
	}
    } while(!scsi_bus_idle && !(sc->sc_szflags[targid] & SZ_DID_DMA));

    /*
     * Check the status of the current SCSI operation. If the SCSI
     * operation completed or disconnected then start the next SCSI 
     * operation, otherwise wait for the DMA to complete.
     * 
     */
    if(scsi_bus_idle) {
	/* Handle BUS DEVICE RESET and ABORT processing. */
        if(sc->sc_szflags[targid] & (SZ_RESET_DEV|SZ_ABORT_CMD)) {
#ifdef SZDEBUG
            PRINTD(targid, 0x4,
	        ("sii_intr: BUS DEVICE RESET and ABORT processing\n"));
#endif SZDEBUG
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
	    if (sc->sc_dkn[targid] >= 0)
	        dk_busy &= ~(1 << sc->sc_dkn[targid]);

	    /* For a BUS DEVICE RESET we must wait for device. */
            if(sc->sc_szflags[targid] & SZ_RESET_DEV) {
		scsi_logerr(sc, 0, targid, SZ_ET_RSTTARG, 1, 0, SZ_HARDERR);
                timeout(sii_restart_target, 
			sc->sc_unit[targid], sz_wait_for_devices*hz);
            	sc->sc_active = 0;
    	    	sz_start(sc, -1);	/* Start next I/O request */
		return;
	    }
	    /* For an ABORT we simply retry the command. */
	    else {
	        sc->sc_active = 0;
	        sc->sc_fstate = 0;
	        sc->sc_szflags[targid] = (SZ_NEED_SENSE|SZ_RETRY_CMD);
    	        sz_start(sc, targid);	/* Retry current I/O request */
	        return;
	    }
        }
        else if(scsi_completed[targid]) {
#ifdef SZDEBUG
            PRINTD(targid, 0x8,
		("sii_intr: COMMAND COMPLETED successfully\n"));
#endif SZDEBUG
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
            sc->sc_active = 0;
    	    sz_start(sc, targid);	/* Finish current I/O request */
	    return;
        }
	else if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
#ifdef SZDEBUG
            PRINTD(targid, 0x8,
		("sii_intr: COMMAND IN PROGRESS disconnected\n"));
#endif SZDEBUG
            sc->sc_active = 0;
    	    sz_start(sc, -1);		/* Start next I/O request */
	    return;
        }
	else {
#ifdef SZDEBUG
            PRINTD(targid, 0x4,
		("sii_intr: (targid=%d) went to BUS FREE unexpectedly\n",
			targid));
#endif SZDEBUG
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
            sc->sc_active = 0;
	    sc->sc_fstate = 0;
	    sc->sc_szflags[targid] = (SZ_NEED_SENSE|SZ_RETRY_CMD);
    	    sz_start(sc, targid);	/* Retry current I/O request */
	    return;
	}
    } 
    else if(sc->sc_szflags[targid] & SZ_DID_DMA) {
#ifdef SZDEBUG
            PRINTD(targid, 0x8,
		("sii_intr: COMMAND IN PROGRESS dma interrupt mode\n"));
#endif SZDEBUG
	return;
    }

HANDLE_ERROR:
    /* Abort the current SCSI operation due to error */
#ifdef SZDEBUG
    PRINTD(targid, 0x4,
	("sii_intr: command aborted (bus=%d target=%d cmd=0x%x)\n",
	    cntlr, targid, sc->sc_curcmd[targid]));
    PRINTD(targid, 0x4, ("", sii_dumpregs(cntlr, 0)));
#endif SZDEBUG
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 1, 0, flags);
    scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 3, 0, SZ_HARDERR);
    sii_reset(sc);
}

/******************************************************************
 *
 * Name:	sii_restartdma
 *
 * Abstract:	Restart a DMA transfer that is (>8K). The SII only 
 *		handles DMA transfers of upto 8K at a time. For large 
 *		DMA transfers the SII must restart the DMA after each 
 *		8K transfer.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * 1		The DMA was restarted.
 * 0		The DMA was not restarted.
 ******************************************************************/
sii_restartdma(sc, targid)
register struct sz_softc *sc;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int dmacount, offset;
    int tmp_state, tmp_phase;
    
    siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
    siiaddr->sii_cstat = SII_DNE;
    if(sc->sc_siidmacount[targid] > 0) {
    
        /* Calculate the DMA transfer length */
        dmacount = sc->sc_siidmacount[targid];
    	if(dmacount > SII_MAX_DMA_XFER_LENGTH)
            dmacount = SII_MAX_DMA_XFER_LENGTH;
    
    	/* Calculate the DMA RAM buffer offset */
	offset = sc->sc_bpcount[targid] - sc->sc_siidmacount[targid];
    

        /* Set the starting address in the 128K RAM buffer. */
        siiaddr->sii_dmaddrl = 
    			((sc->sc_dboff[targid] + offset) & 0xffff);
        siiaddr->sii_dmaddrh = 
    			((sc->sc_dboff[targid] + offset) >> 16);
    	siiaddr->sii_dmlotc = dmacount;
	siiaddr->sii_dmabyte = sc->sc_siioddbyte[targid];
#ifdef SZDEBUG
        PRINTD(targid, 0x8,
	    ("sii_restartdma: offset = %d count = %d\n", offset, dmacount));
#endif SZDEBUG

    	/* Restart the DMA operation */
        sc->sc_szflags[targid] |= SZ_DID_DMA;
        tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
        tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
	if(!scsi_polled_mode)
    	    siiaddr->sii_csr |= SII_IE;
        siiaddr->sii_comm = 
    		    	(SII_DMA | SII_INXFER | tmp_state | tmp_phase);
    	return(1);
    }
    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
    return(0);
}

/******************************************************************
 *
 * Name:	sii_state_change
 *
 * Abstract:	Handle a state change on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * activetargid	Active target Id of device (0 - 7).
 *
 * Outputs:
 * activetargid	New active target Id of device (0 - 7).
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 ******************************************************************/
sii_state_change(sc, activetargid)
register struct sz_softc *sc;
int *activetargid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int targid, timer;

    /*
     * If no target is active then we know the SII will soon
     * be connected to. Spinwait to make sure the (SII_CON)
     * bit in the CSTAT REGISTER is set. We need to do this
     * because the (SII_CON) bit can assert much later than
     * both the (SII_SCH) bit and the (SII_DST_ONBUS) bit.
     */
    if(*activetargid == -1) {
	timer = 1000;
	while(--timer && !(siiaddr->sii_cstat & SII_CON));
#ifdef SZDEBUG
	PRINTD(targid, 0x20, 
	("sii_state_change: (CONNECT wait) start=1000 end=%d\n", timer));
#endif SZDEBUG
    }

    /* Handle a select or reselect here */
    if(siiaddr->sii_cstat & SII_CON) {
	scsi_bus_idle = 0;
	/* Handle a reselect */
	if(siiaddr->sii_cstat & SII_DST_ONBUS) {
	    targid = (siiaddr->sii_destat & SII_IDMSK);
#ifdef SZDEBUG
            PRINTD(targid, 0x1,
		("sii_state_change: target ID %d reselected\n",targid));
#endif SZDEBUG
	    sc->sc_active = (1 << targid);
	    sc->sc_selstat[targid] = SZ_RESELECT;
    	    sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;
	    siiaddr->sii_comm = (siiaddr->sii_cstat & SII_STATE_MSK);
	}
	/* Handle a select */
	else {
	    targid = (siiaddr->sii_slcsr & SII_IDMSK);
#ifdef SZDEBUG
            PRINTD(targid, 0x1,
		("sii_state_change: target ID %d selected\n",targid));
#endif SZDEBUG
	    sc->sc_active = (1 << targid);
	    sc->sc_selstat[targid] = SZ_SELECT;
	}
	if(!scsi_polled_mode)
            siiaddr->sii_csr &= ~(SII_SLE | SII_RSE);
    }
    /* Handle a disconnect here */
    else {
	scsi_bus_idle = 1;
	/*
	 * Check for a back-to-back disconnect occurring in 
	 * which case the active target ID will be -1 and the 
	 * SCSI bus will be idle. KLUDGE for CDROM device.
	 */
	if(*activetargid == -1)
	    return(SZ_SUCCESS);

	targid = *activetargid;
#ifdef SZDEBUG
        PRINTD(targid, 0x1,
	    ("sii_state_change: target ID %d disconnected\n",targid));
#endif SZDEBUG
	if(!scsi_polled_mode)
            siiaddr->sii_csr |= (SII_SLE | SII_RSE | SII_IE);
	siiaddr->sii_comm = 0;

	if(scsi_completed[targid])
	    sc->sc_selstat[targid] = SZ_IDLE;
	else
	    sc->sc_selstat[targid] = SZ_DISCONN;
    }
    *activetargid = targid;
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Name:	sii_phase_change
 *
 * Abstract:	Handle a phase change on the SII chip.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values:
 * SZ_SUCCESS		Command completed successfully.
 * SZ_RET_ABORT		Command aborted.
 ******************************************************************/
sii_phase_change(sc, targid)
register struct sz_softc *sc;
int targid;
{

    int cntlr = sc - &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;
    int tmp_state;
    int tmp_phase;
    int phase;

    /* Get the current bus phase */
    phase = ((siiaddr->sii_dstat & SII_PHA_MSK) << 2);
    siiaddr->sii_dmctrl = 0;
#ifdef SZDEBUG
    PRINTD(targid, 0x1, ("sii_phase_change: current bus phase = "));
    PRINTD(targid, 0x1, ("", sii_print_phase(phase)));
#endif SZDEBUG

    /* Switch on the new phase */
    switch(phase) {
    case SCS_MESSI:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_MESSI_PHA;
	if(sii_msgin(sc, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_MESSO:
        sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_MESSO_PHA;
	if(sii_msgout(sc, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_CMD:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_CMD_PHA;
	if(sii_sendcmd(sc, targid) != SZ_SUCCESS)
	   return(SZ_RET_ABORT);
	break;

    case SCS_STATUS:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_STATUS_PHA;
	if(sii_getstatus(sc, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_DATAO:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAO_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];
        siiaddr->sii_dmctrl = sc->sc_siireqack[targid];
	if(sii_startdma(sc, SZ_DMA_WRITE, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_DATAI:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAI_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];
        siiaddr->sii_dmctrl = sc->sc_siireqack[targid];
	if(sii_startdma(sc, SZ_DMA_READ, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    default:
#ifdef SZDEBUG
	PRINTD(targid, 0x4, ("sii_phase_change: unexpected bus phase = "));
        PRINTD(targid, 0x4, ("", sii_print_phase(phase)));
#endif SZDEBUG
	scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4c, 0, SZ_HARDERR);
	return(SZ_RET_ABORT);
	break;
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Name:	sii_dumpregs
 *
 * Abstract:	DEBUG ROUTINE to dump out the SII chip registers.
 *		Now, also used by scsi_logerr() to print some registers.
 *
 * Inputs:
 * cntlr	SCSI bus controller number (0 = bus A, 1 = bus B).
 * who		0 - called from SII debug, 1 - called by scsi_logerr().
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_dumpregs(cntlr, who)
int	cntlr;
int	who;
{
	register struct sii_regs *siiaddr = (struct sii_regs *)cvqmsi + cntlr;

	/*
	 * Called from scsi_logerr() binary error log.
	 * Print most meaningful registers (limit one line, six regs).
	 */
	if (who) {
	    cprintf("SII %d regs: ", cntlr);
	    cprintf("sc1=%x ", siiaddr->sii_sc1 & 0xffff);
	    cprintf("slcsr=%x ", siiaddr->sii_slcsr & 0xffff);
	    cprintf("destat=%x ", siiaddr->sii_destat & 0xffff);
	    cprintf("cstat=%x ", siiaddr->sii_cstat & 0xffff);
	    cprintf("dstat=%x ", siiaddr->sii_dstat & 0xffff);
	    cprintf("comm=%x\n", siiaddr->sii_comm & 0xffff);
	    return;
	}

	cprintf("\t\tSII register dump:\n");
	cprintf("\t\tsii_sc1 = 0x%x\n", siiaddr->sii_sc1 & 0xffff);
	cprintf("\t\tsii_csr = 0x%x\n", siiaddr->sii_csr & 0xffff);
	cprintf("\t\tsii_id = 0x%x\n", siiaddr->sii_id & 0xffff);
	cprintf("\t\tsii_slcsr = 0x%x\n", siiaddr->sii_slcsr & 0xffff);
	cprintf("\t\tsii_destat = 0x%x\n", siiaddr->sii_destat & 0xffff);
	cprintf("\t\tsii_data = 0x%x\n", siiaddr->sii_data & 0xffff);
	cprintf("\t\tsii_dmctrl = 0x%x\n", siiaddr->sii_dmctrl & 0xffff);
	cprintf("\t\tsii_dmlotc = 0x%x\n", siiaddr->sii_dmlotc & 0xffff);
	cprintf("\t\tsii_dmaddrl = 0x%x\n", siiaddr->sii_dmaddrl & 0xffff);
	cprintf("\t\tsii_dmaddrh = 0x%x\n", siiaddr->sii_dmaddrh & 0xffff);
	cprintf("\t\tsii_dmabyte = 0x%x\n", siiaddr->sii_dmabyte & 0xffff);
	cprintf("\t\tsii_cstat = 0x%x\n", siiaddr->sii_cstat & 0xffff);
	cprintf("\t\tsii_dstat = 0x%x\n", siiaddr->sii_dstat & 0xffff);
	cprintf("\t\tsii_comm = 0x%x\n", siiaddr->sii_comm & 0xffff);
}

#ifdef	SZDEBUG
/******************************************************************
 *
 * Name:	sii_print_phase
 *
 * Abstract:	DEBUG ROUTINE to print out the current bus phase.
 *
 * Inputs:
 * phase	The current bus phase.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_print_phase(phase)
int phase;
{

    switch(phase) {
    case SCS_DATAO:
	cprintf("SCS_DATAO\n");
	break;
    case SCS_DATAI:
	cprintf("SCS_DATAI\n");
	break;
    case SCS_MESSI:
	cprintf("SCS_MESSI\n");
	break;
    case SCS_MESSO:
	cprintf("SCS_MESSO\n");
	break;
    case SCS_CMD:
	cprintf("SCS_CMD\n");
	break;
    case SCS_STATUS:
	cprintf("SCS_STATUS\n");
	break;
    default:
	cprintf("UNKNOWN\n");
	break;
    }
}

/******************************************************************
 *
 * Name:	sii_print_status
 *
 * Abstract:	DEBUG ROUTINE to print out the current command status.
 *
 * Inputs:
 * status	The current command status.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_print_status(status)
int status;
{

    switch(status) {
    case SZ_GOOD:
	cprintf("SZ_GOOD ");
	break;
    case SZ_CHKCND:
	cprintf("SZ_CHKCND ");
	break;
    case SZ_INTRM:
	cprintf("SZ_INTRM ");
	break;
    case SZ_RESCNF:
	cprintf("SZ_RESCNF ");
	break;
    case SZ_BUSY:
	cprintf("SZ_BUSY ");
	break;
    }
    cprintf("\n");
}

/******************************************************************
 *
 * Name:	sii_print_sense
 *
 * Abstract:	DEBUG ROUTINE to print out the request sense data.
 *
 * Inputs:
 * sc		Pointer to sz_softc structure for this controller.
 * targid	Target Id of device (0 - 7).
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_print_sense(sc, targid)
register struct sz_softc *sc;
int targid;
{
    u_char *byteptr;
    int i;

    byteptr = (u_char *)&sc->sc_sns[targid];
    cprintf("sii_print_sense: (targid=%d) ", targid);
    for(i=0; i<SZ_RQSNS_LEN; i++)
	cprintf("%x ", *(byteptr + i));
    switch(sc->sc_sns[targid].snskey) {
    case 0x0:
	cprintf("(No Sense)");
	break;
    case 0x1:
	cprintf("(Soft Error)");
	break;
    case 0x2:
	cprintf("(Not Ready)");
	break;
    case 0x3:
	cprintf("(Medium Error)");
	break;
    case 0x4:
	cprintf("(Hardware Error)");
	break;
    case 0x5:
	cprintf("(Illegal Request)");
	break;
    case 0x6:
	cprintf("(Unit Attention)");
	break;
    case 0x7:
	cprintf("(Write Protected)");
	break;
    case 0x8:
	cprintf("(Blank Check)");
	break;
    case 0x9:
	cprintf("(Vendor Unique)");
	break;
    case 0xa:
	cprintf("(Copy Aborted)");
	break;
    case 0xb:
	cprintf("(Aborted Command)");
	break;
    case 0xc:
	cprintf("(Equal Error)");
	break;
    case 0xd:
	cprintf("(Volume Overflow)");
	break;
    case 0xe:
	cprintf("(Miscompare Error)");
	break;
    case 0xf:
	cprintf("(Reserved)");
	break;
    default:
	cprintf("(Unknown)");
	break;
    }
    if(sc->sc_sns[targid].filmrk)
	cprintf(" filmrk");
    else if(sc->sc_sns[targid].eom)
	cprintf(" eom");
    else if(sc->sc_sns[targid].ili)
	cprintf(" ili");
    cprintf("\n");
}

/******************************************************************
 *
 * Name:	sii_print_inq_info
 *
 * Abstract:	DEBUG ROUTINE to print out the inquiry data info.
 *
 * Inputs:
 * idp		Pointer to the inquiry data structure.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_print_inq_info(idp)
struct sz_inq_dt *idp;
{
    
    char hold[SZ_PID_LEN+1];
    int i;
    u_char *ptr;

    cprintf("***********************************\n");
    cprintf("Dumping Out Inquiry Data:\n");
    for(i=0; i<SZ_VID_LEN; i++)
	hold[i] = idp->vndrid[i];
    hold[i] = '\0';
    cprintf("Vendor ID = %s\n", hold);
    for(i=0; i<SZ_PID_LEN; i++)
	hold[i] = idp->prodid[i];
    hold[i] = '\0';
    cprintf("Product ID = %s\n", hold);
    cprintf("Peripheral Device Type = %x\n",idp->perfdt);
    cprintf("Device Type Qualifier = %x\n",idp->devtq);
    for(i=0; i<SZ_REV_LEN; i++)
	hold[i] = idp->revlvl[i];
    hold[i] = '\0';
    cprintf("Revision Level = %s\n", hold);
    cprintf("***********************************\n");
}
#endif SZDEBUG
