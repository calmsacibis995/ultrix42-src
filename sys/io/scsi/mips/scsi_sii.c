#ifndef lint
static char *sccsid = "@(#)scsi_sii.c	4.10      (ULTRIX)  3/13/91";
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

/************************************************************************
 *
 * scsi_sii.c	05/03/90
 *
 * PVAX/FIREFOX/PMAX SCSI device driver (SII routines)
 *
 * Modification history:
 *
 * 03/12/91	Randall Brown
 *	Made a fix to sii_intr() if it was called do to a stray sii intr.
 *	The address of the chip must be setup.
 *
 * 02/13/91	Paul Grist
 *	Addded latent support for 2nd scsi controller on mipsmate.
 *
 * 02/12/91	Robin Miller
 *   o	Merge in PMAX wmbcopy(), rmbcopy(), & wmbzero() functions which
 *	John Gallant rewrote for the PDMA design.  These functions properly
 *	handle odd byte counts to/from the PMAX SCSI RAM buffer.
 *   o	Added two calls to wbflush() in function sii_reset() where the sii
 *	registers were modified, but were not being flushed.
 *
 * 01/29/91	Robin Miller
 *   o	Setup address/count for various CD-ROM commands which require
 *	data in/data out.
 *   o	Added support for disk send/receive diagnostic functions.
 *
 * 11/21/90	Robin Miller
 *   o	Added support for 10-byte Read/Write CDB's.
 *   o	Use function sz_cdb_length() to obtain the Command Descriptor
 *	Block (CDB) length.
 *
 * 09/26/90	Robin Miller
 *	Fixed problem introduced in 8/29/90 edit.  I inadvertantly left
 *	off the target ID index when referencing the SII DMA count field
 *	(sc_siidmacount[targid]) on disconnect phases.  This would have
 *	caused problems when multiple drives disconnected at the same time.
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
 * 08/07/90     Richard L. Napolitano 
 *      (First Ultrix modification)
 *      In the routine sii_select_target, we must flush the write
 *      to the sii_slcsr register before writing the sii_comm 
 *      register to start selection. This change insures that the
 *      SCSI id of the target is written to the SII prior to starting 
 *      selection.
 *
 * 07/16/90     Janet L. Schank
 *      During selection of a target, it was possible for a reselection
 *      to occur after the ATN line was raised.  The RDAT is broken
 *      and can't deal with this.  So during the selection process,
 *      reselections are now disabled.  This is a slight performance
 *      hit, so if the RDAT is ever fixed this hack should be removed.
 * 
 * 06/04/90	Bill Dallas
 *	In sii_startdma() got rid of a hard define of SZ_RQSNS_LEN. 
 *	Used the scsi command packet for the length.
 *
 * 05/30/90     Janet L. Schank
 *      Modified to use kn230_bzero, kn230_rbcopy, and kn230_wbcopy.
 *
 * 05/14/90     Janet L. Schank
 *      Added Mipsmate rambuffer copy and zero functions to deal with the
 *      hardware bug.
 *
 * 05/03/90	John A. Gallant
 *	Cleaned up a lot of the SCSI bus RESET/clear_io_tasks problems.
 *	removed the bug on sync xfers and RESET in sii_intr.  Removed the
 *	clear io call in sii_scsistart.
 *
 * 03/19/90	John A. Gallant
 *	Placed the setting/clearing of the req/ack offset control register
 *	for the SII in the state change code and in the selection code.
 *	Now sii_dmctrl is set prior to any data phases occuring.
 *
 * 03/14/90	John A. Gallant
 *	Added checks within the data moving code to not go past the true
 *	end of the user's buffer for reads and writes.  Added zero fill
 *	for writes less than a sector size.
 *
 * 01-Mar-90    Janet Schank
 *      Added retries on busy target in the probe routine.
 *
 * 20-Dec-89    Janet L. Schank
 *      Fixed the "sc" initialization bug in probe.  Replaced sii_nNSII
 *      with nNSII.  Moved sii_stray_intr to scsi_data.c.
 *
 * 12-Dec-89    Mitch McConnell
 *	Initialize sc_attached to zero.
 *
 * 01-Dec-89	Fred Canter
 *	Calculate the number of PTEs needed to double map the user's
 *	buffer in a machine independant way (no more magic number).
 *
 * 02-Nov-89    Janet L. Schank
 *      Added a call to sii_clear_discon_io_tasks in sii_reset.
 *      This will clear all active and disconnected IO requests.
 *      Fixed wmbzero clearing error, wasn't clearing all that was needed.
 *
 * 17-Oct-89    Janet L. Schank / JAG
 *      During the message in phase, if an unsupported extended message
 *      is received, ATN is asserted before the last message byte
 *      is acknowledged.  In message out, if polled mode, really send
 *      an identify with NO disconnects.  Added a call in the probe routine
 *      to get_scsiid.
 *
 * 08-Oct-89	Fred Canter
 *	Remove error log #ifdef OLDWAY.
 *
 * 07-Oct-89	Fred Canter
 *	Save removable media bit from inquiry data in sz_softc.
 *
 * 03-Oct-89	Fred Canter
 *	Added error log debug code (#ifdef ELDEBUG). This allows the
 *	driver to produce most error types for uerf debuging.
 *
 * 09/22/89     Janet L. Schank
 *      Changed some defines and ifdefs to include and use sii.h.
 *      Removed alot of "ifdef vax"'s.  The softc structure is now used 
 *      in the same way as on the vax-side.  Replaced siiprobe with
 *      sii_probe taken from the vax code.  The sii reg. addr. is
 *      received as an argument to sii_probe and is saved in the softc
 *      structure, which is then used for all further refrences to
 *      "scsiaddr".
 *
 * 24-Aug-89	Alan Frechette
 *	Added routine siireset() to reset the SII chip for the
 *	crash dump code in order to always get crash dumps. This
 *	is needed for the PROM boot drivers to work correctly.
 *
 * 15-Jul-89	Fred Canter
 *	Merged Dynamic BBR and error log changes.
 *
 * 11-Jul-89	Fred Canter
 *	Convert error messages from printf to scsi_logerr().
 *	Save status byte for current command for error log.
 *	Modify sii_dumpregs() for use by sii_logerr().
 *
 * 27-Jun-89	John A. Gallant
 *	Added the initialization of the completion code routine pointers.  In
 *	the start dma code added support for the read/write long commands.
 *
 * 24-May-89	Fred Canter
 *	Changes to match the new, more general rzdisk utility.
 *	Changed the mode select data structures so an address and
 *	length are passed with the ioctl. This allows pages to be
 *	added to rzdisk without requiring a kernel rebuild.
 *
 * 04/13/89	John A. Gallant
 *	Removed the b_command fixup in sii_busy_target().
 *
 * 03/22/89	John A. Gallant
 *	Able to handle a target returning a status of busy.  The command is
 *	left on the queue and after a BUS_WAIT time is restarted.  The start
 *	dma routine for mode select uses the parameter length in the CDB for
 *	the data count.  The select target routine checks the SII to determine
 *	if it can start a selecton command.
 *
 * 03/01/89	John A. Gallant
 *	Added the pseudo command codes for to allow the tape to unload.  I
 *	followed the same conventions as the firefox/pvax code.  The start/stop
 *	command was changed to a pseudo command.
 *
 * 01/16/89	John A. Gallant
 *	Improved the Selection code, made it more forgiving.  If selection 
 *	fails, the SII is checked to see if it is being reselected.
 *
 * 12/21/88	John A. Gallant
 *	In sii_recvdata() during a dma transfer, the MIS bit is also checked
 *	to watch for the target changing phase.  When the MIS bit is set the
 *	WAIT loop will terminate.
 *
 * 12/20/88	John A. Gallant
 *	During the probe routine, the siidboff[] region is cleared.  This
 *	removes the previous inquiry data that was put there.  Introduced a 
 *	new routine, wmbzero().  It writes 0 to the ram buffer.
 *
 * 12/15/88	John A. Gallant
 *	Modified the wait for CON in sii_state_change(), only wait if
 *	a connect is expected.  Also in the same routine, if CON is not
 *	set bus_idle is set to TRUE, immediatly.
 *
 * 12/13/88	John A. Gallant
 *	Added the changes from firefox:
 *	Had to make a change due to new firmware in CDROM. The CDROM
 *	now sends a message reject message and then disconnects from
 *	the bus when you send it an Extended Synchronous Data Tranfer
 *	Request Message. This is now handled correctly.
 *	
 * 11/21/88	John A. Gallant
 *	Changes to help the "slower" tape drive, in select_target, returns
 *	ABORT, in scsistart the spin loop breaks out and returns IP.
 *	Revamped the debug statements to be able to track a single target.
 *
 * 11/09/88	John A. Gallant
 *	Started the merge with the V3.0 source level.  Due to time constraints
 *	only the changes for the new IOCTL support will be merged.  Others
 *	changes will hopefully occur as time permits.  
 *
 * 11/09/88	Alan E. Frechette
 *	The following fixes were made to support the CDROM device:
 *	1: Call "sii_dodev()" in probe for CDROM device.
 *	
 *	2: Ignore the second disconnect if we get back-to-back 
 *	   disconnects from the CDROM device. 
 *
 *	3: Fixed the routine "sii_select_target" to handle the
 *	   slower CDROM device. Wait for the connect bit (SII_CON)
 *	   after we get the state change bit (SII_SCH). 
 *
 *	Had to change a few things to get "rzdisk" to work but they
 *	were minor.
 *
 *   COMMENTS from V3.0:
 *   03-Nov-88	Alan Frechette
 *	Added in support for disk maintainence. Added the commands
 *	FORMAT UNIT, REASSIGN BLOCK, READ DEFECT DATA and VERIFY DATA.
 *	Made changes to MODE SENSE and MODE SELECT. 
 *
 *  11/07/88	John A. Gallant
 *	The sii_scsistart routine now returns success/or error condition.  This
 *	allowed the state machine to handle error conditions correctly, this
 *	is very useful for tape and filemarks.
 *   COMMENTS from V3.0:
 *   03-Nov-88	Alan Frechette
 *	.............................................. If a scsi command
 *	did not have good status then return the value "SZ_RET_ERR"
 *	in "sii_scsistart()" otherwise return "SZ_SUCCESS". This
 *	was a nasty bug.
 *
 *  7-Sep-88	Ricky Palmer (rsp)
 *	Did some major re-writing of this code for PMAX. Basically
 *	had some major race conditions on the new architecture/processor
 *	as well as some delays due to how original code was constructed.
 *	I re-wrote the interrupt routine to be single threaded and
 *	not looping. I also updated the pieces of this code that
 *	came from Mike Nielsen as he found bugs in his original
 *	SCSI code that had made their way into this code. Larry Palmer
 *	suggested the new buffer scheme that allows this code to
 *	now support 7 devices plus the host adaptor with normal
 *	transfer requests upto 64KB from the minphys code. Some
 *	other changes were targeted at getting performance up
 *	by a factor of 1.5 to 3.0 depending on the request size.
 *	Also added support for the RZ23 as what was here did not
 *	work correctly. Thanks also goes to Larry Palmer for his efforts
 *	in tracking various bugs and problems along with me.
 *	We both spent considerable time architecting, writing new and 
 *	re-writing old code to make this really work for PMAX.
 *
 * 23-Aug-88	Alan Frechette
 *	Fixed the GENERIC kernel boot problem for the SII on FIREFOX.
 *	The GENERIC kernel now boots off a FIREFOX workstation.
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
 * 01-Aug-88 -- Ricky Palmer
 *      Ifdef'ed and modified for mips and vax. Wrote some sii code and
 *	also picked up sii code from Mike Nielsen/Alan Frechette/
 *	Fred Canter/Darrell Dunnuck/Larry Palmer/Bob Rodriguez.
 *
 ***********************************************************************/

#include "../data/scsi_data.c"

/*
 * Define ELDEBUG to create error for uerf debuging.
 * NOTE: must not be defined at submit time (performance hit).
 */
/*#define	ELDEBUG	*/

#include "scsi_debug.h"
int siidebug = 0;
int siitarget = 0;

#ifdef vax
extern char cvqmsi[] [512*NBPG];
extern char cvqmsirb[];
#endif vax
extern char szbufmap[];
extern short sz_timetable[];
extern int cpu;

int sii_wait_after_inquiry = 1000;

/* For quick hacks at debugging. */
#undef DEBUG

int sii_busy_target();			/* for forware reference */
int wmbcopy(), rmbcopy(), wmbzero(), bzero(), bcopy();
extern int kn230_bzero(), kn230_wbcopy(), kn230_rbcopy();

/******************************************************************
 *
 * Probe routine for SII chip.
 *
 ******************************************************************/
short sii_reject_message = 0;
short sii_assert_attn = 0;		/* Assert Attention Flag    */

/*
 * Number of seconds to wait for SCSI devices
 * to come ready enough to respond to inquiry
 * after bus reset. Needs to be a variable so
 * users can change it (I am such a nice guy).
 * Factory default is 7 seconds (in scsi_data.c).
 */
extern int sz_wait_for_devices;
extern int sz_max_wait_for_devices;

extern int sii_scsistart();
extern int sii_reset();

extern struct scsi_devtab szp_rz_udt;
extern struct scsi_devtab szp_tz_udt;
extern struct scsi_devtab szp_cz_udt;
extern int szp_nrz;
extern int szp_ntz;
extern int szp_ncz;
extern int szp_nrx;
extern int rz_max_xfer;
extern int cz_max_xfer;
extern int tz_max_xfer;
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
short sii_sent_cmd = 0;			/* SCSI Command Sent flag    */
short sii_wait_count = SII_WAIT_COUNT;	/* Delay count for SII chip */
short sii_use_programmed_io = 0;	/* Programmed IO Mode flag  */
short sii_firstcall = 1;		/* First call to probe flag */
short sii_debug_probe = 0;		/* Used for debugging probe */
short sii_test = 0;			/* Used for testing purposes */

/*
 * The configuration code on the vax side needs to be modified
 * to pass a pointer to uba_ctlr instead on the cntlr number.
 */
sii_probe(reg, um)
caddr_t reg;
register struct uba_ctlr *um;
{
	int cntlr = um->um_ctlr;
	SII_REG *siiaddr = (struct sii_regs *) reg;
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
	    sii_stray_intr[cntlr] = 1;
	    sc = &sz_softc[cntlr];
	    sc->sc_siinum = cntlr;
	    sc->scsi_polled_mode = 1;
	    sc->scsi_bus_idle = 0;
	    sc->sc_sysid =
		    get_scsiid(cntlr);      /* init the host adapter ID */
	    sc->sc_active = 0;		    /* init the active flag */
	    sc->port_start = sii_scsistart; /* init the port_start switch */
	    sc->port_reset = sii_reset;	    /* init the port_reset switch */
	    /*
	     * Determine the rambuffer copy and zero routines.  The PMAX
             * utilizes a 16bit rambuffer where there are 16bit holes.
	     * The Firefox and Mipsmate utilize 32bit rambuffers where
	     * there are no such holes.
	     */

	    /* 
	     * Need to init these here, to support 2nd sii controller
	     */
	    sc->sc_rambuff = SII_BUF_ADDR;  /* init the RAM buffer pointer*/ 
	    sc->sc_scsiaddr = (caddr_t)reg; /* init the sii addres */

	    if (cpu == DS_3100) {
		    sc->rmbcopy = rmbcopy;
	            sc->wmbcopy = wmbcopy;
                    sc->wmbzero = wmbzero;
	    }
	    else if (cpu == DS_5100) {
		    sc->rmbcopy = kn230_rbcopy;
		    sc->wmbcopy = kn230_wbcopy;
	 	    sc->wmbzero = kn230_bzero;

		    /* 
		     * If 2nd kn230 controller, set-up accordingly:
		     *
		     *   - 16bit pmax bcopy
		     *   - different start address
		     *   - different ram buffer address
		     */

#define KN230_OPT_REG (volatile struct sii_regs *)PHYS_TO_K1(0x15000000) 
#define KN230_OPT_BUF (volatile char *)PHYS_TO_K1(0x15200000)

		    if( cntlr == 1 ){ 
			    sc->rmbcopy = rmbcopy;
			    sc->wmbcopy = wmbcopy;
			    sc->wmbzero = wmbzero;			    
			    siiaddr = KN230_OPT_REG;
			    sc->sc_scsiaddr = (caddr_t)KN230_OPT_REG;
			    sc->sc_rambuff = KN230_OPT_BUF; 
		    }
	    }
	    else {
	            sc->rmbcopy = bcopy;
		    sc->wmbcopy = bcopy;
	 	    sc->wmbzero = bzero;
	    }

	    sii_reset(sc); 		    /* reset the SII chip */
	    sc->sii_was_reset = 0;
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
	    sc->sc_attached[targid] = 0;
	    sc->sc_rzspecial[targid] = 0;
	    sc->sc_rmv_media &= ~(1 << targid);
	    if(targid == sc->sc_sysid)
		continue;	/* skip initiator */

	    retries = 5;
	    status = 1;
	    i = sz_wait_for_devices;
	    while (retries)
	    {
		/* Clear where the inquiry data will be going.  This is like
		   putting the cart before the horse, the offsets have not 
		   been setup yet.  However, scsistart() calls recvdata() and
		   recvdata() uses the values in siidboff[] in transfering the
		   data.  This causes all transfers to occur in the first 
		   ram buffer "page".  And valid inquiry data from the previous
		   targit is still there.  This can cause some interesting
		   device types. */

		sc->sc_szflags[targid] = SZ_NORMAL; 
		sc->sc_curcmd[targid] = SZ_INQ;

		(sc->wmbzero)((char *)(sc->sc_rambuff + sc->sc_siidboff[targid]),
			256);	/* magic # !! check with siidboff alloc */

		sc->sc_siisentsync[targid] = 1;
		sz_bldpkt(sc, targid, SZ_INQ, 0, 0);
		stat = sii_scsistart(sc, targid, 0);
		PRINTD(targid, 0x2,
		       ("sii_probe: targid %d stat from scsistart=%x\n",
			targid, stat));
		if (sc->sc_szflags[targid] & SZ_BUSYTARG) {
		    sc->sc_szflags[targid] &= ~SZ_BUSYTARG;
		    DELAY(1000000);			/* delay 1 second */
		    if (++i >= sz_max_wait_for_devices)
			break;
		    continue;
		}
		if (stat == SZ_SUCCESS) {
			status = 0;
			break;
		}
		else if(stat == SZ_RET_ABORT) {
			status = 1;
			break;
		}
		DELAY(1000);
		retries--;
		continue;
	    }
	    if (status != SZ_SUCCESS)
		    continue;

	    /*
	     * Initialize data structures for this target and
	     * save all pertinent inquiry data (device type, etc.).
	     */
	    idp = (struct sz_inq_dt *)&sc->sz_dat[targid];

	    /* Save removable media bit for each target */
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
		printf("sii_probe: scsi %d targetID %d: %s (%d).\n",
		       um->um_ctlr, targid, "unknown peripheral device type",
		       idp->perfdt);
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
			um->um_ctlr, targid, "cannot get PTEs for bufmap");
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
#ifdef vax
		dboff += 128;
#endif vax
#ifdef mips
		dboff += 256;
#endif mips
	    }
#ifdef vax
	    dboff = (1 * 1024);
#endif vax
#ifdef mips
	    dboff = (2 * 1024);
#endif mips
	    /* determine rz slot size, must be > 16kb */
	    cz_slotsize = tz_slotsize = rz_slotsize = 16 * 1024;
	    for (cntlr=0; cntlr<nNSII; cntlr++) {
		sc = &sz_softc[cntlr];
		for (targid=0; targid<NDPS; targid++) {
		    if (targid == sc->sc_sysid)
			continue;
		    if (sc->sc_alive[targid] == 0)
			continue;
		    sc->sc_dboff[targid][0] = dboff;
		    sc->sc_dboff[targid][1] = dboff+SII_MAX_DMA_XFER_LENGTH;
		    sc->sc_segcnt[targid] = 64 * 1024; /* Get rid of in freds code */
		    dboff += rz_slotsize;
		    PRINTD(targid, 0x20,
			("sii_probe: cntlr=%d targid=%d devtype=%x ", cntlr,
			targid, sc->sc_devtyp[targid]));
		    PRINTD(targid, 0x20, ("req/ack=%d slotsize=%d\n",
			     sc->sc_siireqack[targid], sc->sc_segcnt[targid]));
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
 * Start a SCSI operation on the SII chip.
 *
 ******************************************************************/
sii_scsistart(sc, targid, bp)
register struct sz_softc *sc;
int targid;
register struct buf *bp;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    volatile char *stv;
    int retval;
    int timer;
    int phase;
    u_short cstat, dstat;
    int flags;

    /*
     * If "bp" is "0" we use polled scsi mode, disallow reselect
     * attempts, and disable interrupts for all DMA transfers. We 
     * poll for DMA completion instead of allowing interrupts.
     */
    siiaddr->sii_csr |= SII_SLE;
#ifdef mips
    wbflush();
#endif mips
    if(bp == (struct buf *)0) {
        siiaddr->sii_csr &= ~(SII_RSE | SII_IE);
	sc->scsi_polled_mode = 1;
    }
    else
	{
	 /* Have to clear out the rambuffer area.  It is possible for a target
	    to not not return all the requested bytes.  We have to make sure
	    that at least the extra bytes are cleared. */

	    if(sc->sc_rzspecial[targid]) {
		struct mode_sel_sns_params *msp;

	      /* Some risk here using get_validbuf() w/out any error checking.
		It is probably low, at this point the target should not be
		doing anything, and ready for the upcomming command.  Note:
		this should be moved into the DMA level code. */
		
		msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
		if(sc->sc_actcmd[targid] == SZ_MODSNS)
		{
		    stv = sc->sc_rambuff +
			(sc->sc_dboff[targid][get_validbuf(sc, targid)] * 2);
		    (sc->wmbzero)(stv, msp->msp_length);
		}
	    }
        siiaddr->sii_csr |= (SII_RSE | SII_IE);
        sc->scsi_polled_mode = 0;
    }
#ifdef mips
    wbflush();
#endif mips

    /* Perform target arbitration and selection */
    if((retval = sii_select_target(sc, targid)) != SZ_SUCCESS) {
	return(retval);
    }

BEGIN_LOOP:
    /* Loop through all bus phases until command complete */
    sc->scsi_completed[targid] = 0;
    sc->scsi_bus_idle = 0;
    cstat = siiaddr->sii_cstat;
    dstat = siiaddr->sii_dstat;
    do {
/*
XPRINTF(XPR_NFS, "B: %x", sc->sc_siidmacount[targid],0,0,0);
cprintf("B: %x %x", sc->sc_siidmacount[targid],bp,0,0);
*/
	if(cstat & (SII_CI|SII_DI)) {

        /* Check for a BUS ERROR */
        if(cstat & SII_BER) {
  	    siiaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	}

        /* Check for a PARITY ERROR */
	if(dstat & SII_IPE) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 0, 0, flags);
	    PRINTD(targid, 0x10, ("sii_scsistart: scsi %d parity error\n",
		cntlr));
	    goto HANDLE_ABORT;
	}

	/* Check for a BUS RESET */
	if(cstat & SII_RST_ONBUS) {
	    siiaddr->sii_cstat = SII_RST_ONBUS;
#ifdef mips
    wbflush();
#endif mips
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 0, 0, SZ_HARDERR);
	    PRINTD(targid, 0x10, ("sii_scsistart: scsi %d bus reset\n", cntlr));
	    goto HANDLE_ABORT;
	}

        /* Check for a STATE CHANGE */
        if(cstat & SII_SCH)
	{
	  /* If SZ_BUSYTARG is set, the target returned a BUSY status for the
	    current command.   The target has disconnected from the bus.
	    Return SZ_IP, and allow the interrupt handler take care of the
	    disconnect. */

	    if( bp && (sc->sc_szflags[targid] & SZ_BUSYTARG) )
	    {
		PRINTD(targid, 0x4,
		    ("sii_scsistart: target %d BUSY rtn SZ_IP\n", targid ));
		return(SZ_IP);
	    }

	    siiaddr->sii_cstat = SII_SCH;	/* clear the intr */
#         ifdef mips
	    wbflush();				/* wait for write buffers */
#         endif mips
            if(sii_state_change(sc, &targid) != SZ_SUCCESS)
                goto HANDLE_ABORT;
	}

	/* If disconnected and went to BUS FREE STATE then break */
	if(sc->scsi_bus_idle)
	    break;

        /* Check for a PHASE MISMATCH */
        if(dstat & SII_MIS) {

            /* Check for a BUS ERROR */
            if(cstat & SII_BER) {
  	        siiaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* Always clear DID DMA flag on a phase change */
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

            /* Handle the current bus phase */
            if(sii_phase_change(sc, dstat) != SZ_SUCCESS)
                goto HANDLE_ABORT;

            /* Wait for the next bus phase */
            if(!sc->scsi_completed[targid] && 
		  !(sc->sc_szflags[targid] & (SZ_WAS_DISCON|SZ_DID_DMA))) {
		timer = 1000;
		while(--timer && !(siiaddr->sii_dstat & SII_MIS));
    		dstat = siiaddr->sii_dstat;
		if(!sc->scsi_polled_mode && (timer == 0) && 
			!(dstat & (SII_CI|SII_DI)) && 
				(sc->sc_actcmd[targid] != SZ_RQSNS)) {
                    PRINTD(targid, 0x4,
			("sii_scsistart: SII_MIS didn't set rtn SZ_IP\n"));
		    return(SZ_IP);
		}
	    }
	}

	/* Check for fragmented DMA transfers (>8K) */
	if(sc->scsi_polled_mode && !(dstat & SII_MIS) &&
		(dstat & (SII_TBE|SII_IBF)) &&
	  		((sc->sc_fstate == SZ_DATAI_PHA) || 
				(sc->sc_fstate == SZ_DATAO_PHA))) {

	    /* Restart the DMA transfer */
	    if(!sii_restartdma(sc))
		goto HANDLE_ABORT;
	}

	/* Sometimes the target stays in the same phase */
	if((dstat & (SII_IBF|SII_TBE)) && 
		!(dstat & SII_MIS) &&
			((sc->sc_fstate != SZ_DATAI_PHA) &&
				(sc->sc_fstate != SZ_DATAO_PHA))) {

            /* Check for a BUS ERROR */
            if(cstat & SII_BER) {
  	        siiaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* Handle the current bus phase */
	    if(sii_phase_change(sc, dstat) != SZ_SUCCESS)
	        goto HANDLE_ABORT;
	}
	}
    dstat = siiaddr->sii_dstat;
    cstat = siiaddr->sii_cstat;
  
 /*   } while(sc->scsi_polled_mode && 	/* dont spin in here!!!! */
    } while(!sc->scsi_bus_idle && 
		!(sc->sc_szflags[targid] & (SZ_WAS_DISCON|SZ_DID_DMA)));

/*
XPRINTF(XPR_NFS, "E: %x", sc->sc_siidmacount[targid],0,0,0);
cprintf("E: %x", sc->sc_siidmacount[targid],0,0,0);
*/
    /*
     * Check the status of the current SCSI operation. If the SCSI
     * operation completed or disconnected then start the next SCSI 
     * operation, otherwise wait for the DMA to complete.
     * 
     */
    if(sc->scsi_bus_idle || (sc->sc_szflags[targid] & SZ_WAS_DISCON)) {
        if(sc->scsi_completed[targid]) {
            PRINTD(targid, 0x4,
		("sii_scsistart: COMMAND COMPLETED successfully\n"));
	    sc->scsi_completed[targid] = 0;
            sc->sc_active = 0;
	    if (sc->sc_szflags[targid] & SZ_BUSYTARG)
	        return(SZ_IP);
    	    if(sc->sc_status[targid] == SZ_GOOD)
	        return(SZ_SUCCESS);	
	    else
	        return(SZ_RET_ERR);	
        } 
	else if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
            PRINTD(targid, 0x4, 
		("sii_scsistart: COMMAND IN PROGRESS disconnected\n"));
	    return(SZ_IP);
        }
	else {
            sc->sc_active = 0;
#ifdef NO_TIMER	/* JAG */
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
#endif NO_TIMER	/* JAG */
	    return(SZ_RET_ERR);
	}
    } 
    else if(sc->sc_szflags[targid] & SZ_DID_DMA) {
	/* Poll and busy wait for DMA completion */
	if(sc->scsi_polled_mode) {
            PRINTD(targid, 0x4, 
		("sii_scsistart: COMMAND IN PROGRESS dma poll mode\n"));
            siiaddr->sii_csr &= ~SII_IE;
#ifdef mips
    wbflush();
#endif mips
            SZWAIT_UNTIL((siiaddr->sii_dstat & SII_DNE),sii_wait_count,retval);
	    siiaddr->sii_dstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

	    /* Update the remaining dma count for this current transfer */
	    if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		sc->sc_siidmacount[targid] -= 
			(SII_MAX_DMA_XFER_LENGTH - siiaddr->sii_dmlotc);
	    else
		sc->sc_siidmacount[targid] -= 
			(sc->sc_siidmacount[targid] - siiaddr->sii_dmlotc);
	    if(retval >= sii_wait_count)
	        goto HANDLE_ABORT;
	    else
	    	goto BEGIN_LOOP;
	}
	/* Wait for interrupt to signal DMA completion */
	else {
            PRINTD(targid, 0x4,
		("sii_scsistart: COMMAND IN PROGRESS dma interrupt mode\n"));
	    return(SZ_IP);
   	}
    } else if(!sc->scsi_polled_mode)
	return(SZ_IP); 	/* dont spin in here!!! */

HANDLE_ABORT:
    /* Abort the current SCSI operation due to error */
    PRINTD(targid, 0x10,
	("sii_scsistart: command aborted (bus=%d target=%d cmd=0x%x)\n",
	    cntlr, targid, sc->sc_curcmd[targid]));
    PRINTD(targid, 0x10, ("", sii_dumpregs(cntlr, 0)));
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 0, 0, flags);
    sii_reset(sc);
    siiaddr->sii_cstat = 0xffff;
    siiaddr->sii_dstat = 0xffff;
#ifdef mips
    wbflush();
#endif mips
    sc->sc_selstat[targid] = SZ_IDLE;
    sc->sc_active = 0;
    return(SZ_RET_ABORT);
}

/******************************************************************
 *
 * Perform the arbitration/selection phases for the SII chip.
 *
 ******************************************************************/
sii_select_target(sc, targid)
register struct sz_softc *sc;
int targid;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int retval, i;
    int retries = 3;
    int sii_select_wait = 5000;
    int sii_rse_flag;

    /* Loop till retries exhausted */
    for(i = 0; i < retries; i++) {
      /*
       * Determine if reselections are currently enabled.  If so,
       * disable them during the selection.
       */
      sii_rse_flag = siiaddr->sii_csr & SII_RSE;
      if (sii_rse_flag) {
	   siiaddr->sii_csr &= ~SII_RSE;
#ifdef mips
    wbflush();
#endif mips
      }

      /* Do a quick check on the bus to see if it is in the busy state.  If
        BUSY or SEL is asserted, another device on the bus is in a selection
	phase.  Return SZ_BUSBUSY.  SZ_BUSBUSY can be returned and the
	statemachine can leave.  This is a single Initiator bus, and a
	selection is either mine or a reselection, in which the interrupt
	handler will be called. */

	if( (siiaddr->sii_sc1 & (SII_SC1_BSY | SII_SC1_SEL)) !=0 )
	{
	    PRINTD(targid, 0x104,
		("sii_select_target: Bus BUSY on select of ID %d\n",targid));
	    /*
	     * Turn reselections back on before leaving.
	     */
	    if (sii_rse_flag) {
		 siiaddr->sii_csr |= SII_RSE;
#ifdef mips
    wbflush();
#endif mips
	    }
	    return( SZ_BUSBUSY );
	}

        /*
         * Begin the selection phase on the SII chip with or without
         * reselects. Setup the Selector Control Register and the 
	 * Command Register on the SII to select a target on the SCSI 
	 * bus.
         */
        siiaddr->sii_slcsr = targid;
#ifdef mips
         wbflush();		/* Flush SCSI id prior to starting selection. */
#endif mips
        siiaddr->sii_comm = (SII_SELECT|SII_ATN);
        /*
	 * It should be safe now to enable reselections.
	 */
        if (sii_rse_flag) {
	     siiaddr->sii_csr |= SII_RSE;
	}
        PRINTD(targid, 0x104,
	    ("sii_select_target: starting select of ID %d\n",targid));
#ifdef mips
    wbflush();
#endif mips
    
        /* Start timer to wait for a select to occur */
        SZWAIT_UNTIL((siiaddr->sii_cstat & SII_SCH),sii_select_wait,retval);

	/* If a state change did occur then make sure we are connected */
	if((siiaddr->sii_cstat & SII_SCH) && !(siiaddr->sii_cstat & SII_CON))
            SZWAIT_UNTIL((siiaddr->sii_cstat & SII_CON),sii_wait_count,retval);

        /* Check for connection attempt */
        if(siiaddr->sii_cstat & SII_CON) {
            /* Check for a Reselection Attempt and handle it in "sii_intr" */
            if(siiaddr->sii_cstat & SII_DST_ONBUS) {
	        targid = (siiaddr->sii_destat & SII_IDMSK);
          	PRINTD(targid, 0x104,
		    ("sii_select_target: reselect of ID %d in progress\n",
		    targid));
    	        return(SZ_BUSBUSY);
            }
            /* Check for a Selection Attempt and handle it here */
            else {
    	        siiaddr->sii_cstat = SII_SCH;
#ifdef mips
    wbflush();
#endif mips
        	targid = (siiaddr->sii_slcsr & SII_IDMSK);
                PRINTD(targid, 0x104,
		    ("sii_select_target: target ID %d selected\n",targid));
                sc->sc_active = (1 << targid);
                sc->sc_selstat[targid] = SZ_SELECT;
		if(!sc->scsi_polled_mode) {
		    siiaddr->sii_csr &= ~(SII_SLE | SII_RSE);
#ifdef mips
    wbflush();
#endif mips
		}
	      /* Set the sii_dmctrl register to the rec/ack offset for the
		selected target.  The sii_dmctrl needes to be set if/when
		data phase is entered. */

		siiaddr->sii_dmctrl = sc->sc_siireqack[targid];
#ifdef mips
    wbflush();
#endif mips
    	        return(SZ_SUCCESS);
            }
        }
	else
	{
            PRINTD(targid, 0x114,
		( "sii_select_target: select of ID %d failed pass %d\n",
		targid, i ));
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
#ifdef mips
    wbflush();
#endif mips
    	        SZWAIT_UNTIL((siiaddr->sii_cstat & SII_SCH),sii_select_wait,
		    retval);
    	        siiaddr->sii_cstat = SII_SCH;
    	        siiaddr->sii_comm = 0;
#ifdef mips
    wbflush();
#endif mips
	    }
	}
    }

    return(SZ_RET_ABORT);

}

/******************************************************************
 *
 * Get the currently active scsi target.
 *
 ******************************************************************/
sii_getactive_target(sc)
register struct sz_softc *sc;
{

    int targid;

    for(targid=0; targid<NDPS; targid++)
	if(sc && sc->sc_active & (1 << targid))
	    return(targid);
    return(-1);
}

/******************************************************************
 *
 * Perform the DATA IN/DATA OUT PHASE on the SII chip.
 *
 ******************************************************************/
sii_startdma(sc, iodir)
register struct sz_softc *sc;
int iodir;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    SII_BUFF *stv;
    u_char *byteptr;
    char *bufp;
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
    int targid = sii_getactive_target(sc);

    /*
     * Handle non READ/WRITE scsi commands that transfer data.
     */
    if ( (sc->sz_opcode != SZ_WRITE) && (sc->sz_opcode != SZ_READ) &&
         (sc->sz_opcode != SZ_WRITE_10) && (sc->sz_opcode != SZ_READ_10) ) {
	byteptr = (u_char *)&sc->sz_dat[targid];
	switch(sc->sz_opcode) {
	case SZ_MODSEL:
	    byteptr = (u_char *)&sc->sc_dat[0];
	    datacnt = (int) sc->sz_modsel.pll;

#ifdef NOTNOW
	    datacnt = SZ_MODSEL_LEN;
	    if((sc->sc_devtyp[targid] & SZ_TAPE) == 0)
		datacnt -= 2;
#endif NOTNOW
	    if(sc->sc_rzspecial[targid]) {
	        msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
                byteptr = (u_char *)sc->sc_rzaddr[targid];
                datacnt = msp->msp_length;
	    }
#ifdef SZDEBUG
	    /* JAG make this a subroutine with DEBUG */
    	    PRINTD(targid, 0x20, ("sii_startdma: mode select data:"));
    	    for(i=0; i < datacnt; i++)
	    {
	        PRINTD(targid, 0x20, (" %x", *(byteptr+i)));
	    }
    	    PRINTD(targid, 0x20, ("\n"));
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
 * Why was it this way? Fred -- 7/13/89
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
	case SZ_SNDDIAG: {
	    if (sc->sc_devtyp[targid] & (SZ_DISK|SZ_CDROM)) {
		struct diagnostic_params *dp;

		dp = (struct diagnostic_params *) sc->sc_rzparams[targid];
		datacnt = dp->dp_length;
		byteptr = (u_char *)sc->sc_rzaddr[targid];
	    } else {
		datacnt = SZ_RECDIAG_LEN;	/* For tape driver. */
	    }
	    break;
	}
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
	
	case SZ_READ_TOC: {
	    register struct cd_toc *toc;

	    toc = (struct cd_toc *)sc->sc_rzparams[targid];
	    datacnt = toc->toc_alloc_length;
            byteptr = (u_char *)sc->sc_rzaddr[targid];
	    break;
	}
	case SZ_READ_SUBCHAN: {
	    register struct cd_sub_channel *sch;

	    sch = (struct cd_sub_channel *)sc->sc_rzparams[targid];
	    datacnt = sch->sch_alloc_length;
            byteptr = (u_char *)sc->sc_rzaddr[targid];
	    break;
	}
	case SZ_READ_HEADER: {
	    register struct cd_read_header *rh;

	    rh = (struct cd_read_header *)sc->sc_rzparams[targid];
	    datacnt = rh->rh_alloc_length;
            byteptr = (u_char *)sc->sc_rzaddr[targid];
	    break;
	}
	case SZ_PLAYBACK_CONTROL:
	case SZ_PLAYBACK_STATUS: {
	    register struct cd_playback *pb;

	    pb = (struct cd_playback *)sc->sc_rzparams[targid];
	    datacnt = pb->pb_alloc_length;
            byteptr = (u_char *)sc->sc_rzaddr[targid];
	    break;
	}
	default:
	    PRINTD(targid, 0x10, ("sii_startdma: unknown scsi cmd 0x%x\n",
		sc->sz_opcode));
	    return(SZ_RET_ABORT);
	    break;
	}

	PRINTD (targid, 0x20,
		("Using byteptr at 0x%x of datacnt %d bytes.\n",
						byteptr, datacnt));

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
	    if(sii_senddata(sc, byteptr, datacnt, 0) != SZ_SUCCESS)
		return(SZ_RET_ABORT);
	}

	/* Get the data from the scsi bus */
        else {
	    if(sii_recvdata(sc, byteptr, datacnt) != SZ_SUCCESS)
		return(SZ_RET_ABORT);
        }
    } 
    else {
SETUP_DMA:
        /*
         * Start of DMA code for a READ or WRITE scsi command, setup
         * the count, the RAM buffer offset, and the DMA registers.
         */
	sc->sc_iodir[targid] = iodir;
/*
XPRINTF(XPR_NFS, "sta: %x %x %x", iodir, sc->sc_fstate,sc->sc_siidmacount[targid],0);
*/

	siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
#ifdef mips
    wbflush();
#endif mips
    	sc->sc_savcnt[targid] = 0;
	/* Handle the case of the DMA being disconnected */
	if(sc->sc_szflags[targid] & SZ_DMA_DISCON) {
	    sc->sc_szflags[targid] &= ~SZ_DMA_DISCON;
	    sii_restartdma(sc);
	} else { /* Handle the case of the DMA just starting */
            /* Setup the dmacount and the offset into the RAM buffer */
	    sc->sc_siidmacount[targid] = sc->sc_bpcount[targid];
	    sii_restartdma(sc);
	}
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Perform the COMMAND PHASE on the SII chip.
 *
 ******************************************************************/
sii_sendcmd(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    u_char *byteptr;
    int datacnt, i;
    int cmd_type;
    int cmdcnt;
    int targid = sii_getactive_target(sc);

    sc->sc_szflags[targid] = 0;
    sc->sc_savcnt[targid] = 0;
    sc->sc_status[targid] = 0xff;
    sc->sc_siidmacount[targid] = 0;
    byteptr = (u_char *)&sc->sz_command;
    cmd_type = *byteptr;
    cmdcnt = sz_cdb_length (cmd_type, targid);

#ifdef SZDEBUG
    /* JAG make this a subroutine call with DEBUG */
    PRINTD(targid, 0x20, ("sii_sendcmd: scsi cmd pkt:")); 
    for(i=0; i < cmdcnt; i++)
    {
	PRINTD(targid, 0x20, (" %x", *(byteptr+i)));
    }
    PRINTD(targid, 0x20, ("      ( %s )\n", scsi_cmdtable[cmd_type]));
#endif SZDEBUG

    /* Put the scsi command onto the scsi bus */
    if(sii_senddata(sc, byteptr, cmdcnt, 0) != SZ_SUCCESS)
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
 * Perform the STATUS PHASE on the SII chip.
 *
 ******************************************************************/
sii_getstatus(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int targid = sii_getactive_target(sc);

    /* Get the status byte from the scsi bus */
    if(sii_recvdata(sc, &sc->sc_status[targid], 1) != SZ_SUCCESS)
	return(SZ_RET_ABORT);
    /* Save the status byte for the error log */
    if (sc->sc_curcmd[targid] == sc->sc_actcmd[targid])
	sc->sc_statlog[targid] = sc->sc_status[targid];

    PRINTD(targid, 0x24, ("sii_getstatus: status byte = 0x%x = ",
	sc->sc_status[targid]));
    PRINTD(targid, 0x24, ("", sii_print_status((int)sc->sc_status[targid])));

    /* Check the status a switch table is used to handle future abilitys
    in status checking. */

    switch( sc->sc_status[ targid ] )
    {
      /* All went well onto the next phase. Fall through to the return() */

	case SZ_GOOD :
	break;

      /* Set the SZ_NEED_SENCE flag, the state mach. will handle the rest. */

	case SZ_CHKCND :
	    sc->sc_szflags[targid] |= SZ_NEED_SENSE;
	break;

      /* Have to wait a bit for the target to be able to handle the request.
	Set the BUSYTARG flag to signal the interrupt handler of the BUSY
	condition. */

	case SZ_BUSY :
	    sc->sc_szflags[targid] |= SZ_BUSYTARG;	/* set BUSY flag */
	break;

	case SZ_INTRM :			/* not handled for now */
	case SZ_RESCNF :
	default :
	    return(SZ_RET_ABORT);	/* Assume bad failure for now */
	break;
    }

    return(SZ_SUCCESS);			/* every thing went well */
}

/******************************************************************
 *
 * Perform the MESSAGE OUT PHASE on the SII chip.
 *
 ******************************************************************/
sii_msgout(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
#ifdef vax
    u_char messg;
#endif vax
#ifdef mips
    u_long messg;
#endif mips
    int targid = sii_getactive_target(sc);
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
        sii_senddata(sc, &messg, 1, 0);
	return(SZ_SUCCESS);
    }

    /* Check if we need to send a Message Reject Message */
    if(sii_reject_message) {
	sii_reject_message = 0;
	messg = SZ_MSGREJ;

        PRINTD(targid, 0x4, ("sii_msgout: sending Message Reject Message\n"));

        /* Put the Message Reject Message onto the scsi bus */
        if(sii_senddata(sc, &messg, 1, 0) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
    }

    /* Send the Identify Message with or without disconnects */
    else {
    /* Clear the "sii_assert_attn" flag */
    sii_assert_attn = 0;
	/* Setup for disconnects or no disconnects */
        if(sc->scsi_polled_mode)
            messg = SZ_ID_NODIS | lun;	/* Allow no disconnects */
        else
            messg = SZ_ID_DIS | lun;  	/* Allow disconnects */

        /* Check if we need to send a Synchronous DataXfer Message */
	if(!sc->sc_siisentsync[targid]) {
	    sc->sc_siisentsync[targid] = 1;

	    PRINTD(targid, 0x4,
		("sii_msgout: sending Identify Message = 0x%x\n",messg));

            /* Put the Identify Message onto the scsi bus */
            if(sii_senddata(sc, &messg, 1, SII_ATN) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);
	    PRINTD(targid, 0x4,
		("sii_msgout: sending Sync Data Transfer Message\n"));

            /* Put the Synchronous Data Xfer Message onto the scsi bus */
            sc->sc_extmessg[targid][0] = SZ_EXTMSG;
            sc->sc_extmessg[targid][1] = 0x3;
            sc->sc_extmessg[targid][2] = SZ_SYNC_XFER;
            sc->sc_extmessg[targid][3] = 63; /* 25, 12 is fast! */
            sc->sc_extmessg[targid][4] = SII_SYNC;
	    { int i;
	    for(i=0; i < 4; i++)
            if(sii_senddata(sc, &sc->sc_extmessg[targid][i], 1, SII_ATN) 
				!= SZ_SUCCESS) {
		if(i == 1) {
			/* For all those weird devices, can't do EXTMSG */
			sc->sc_siireqack[targid] = 0;
	        	return(SZ_SUCCESS);
		}
		return(SZ_RET_ABORT);
	    }
	    }
	    /* The following should probably not ever return failure
	       since there are non-synchronous devices that will just
	       reset the bus if given this message. The devices that
	       this works on will succeed in any case. */
            if(sii_senddata(sc, &sc->sc_extmessg[targid][4], 1, 0) 
				!= SZ_SUCCESS) {
		/* For all those weird devices, can't do SYNC */
		sc->sc_siireqack[targid] = 0;
	        return(SZ_SUCCESS);
	    }
        }
	else {

	    PRINTD(targid, 0x4,
		("sii_msgout: sending Identify Message = 0x%x\n",messg));

            /* Put the Identify Message onto the scsi bus */
            if(sii_senddata(sc, &messg, 1, 0) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);
	}
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Perform the MESSAGE IN PHASE on the SII chip.
 *
 ******************************************************************/

#ifdef	ELDEBUG
/* Set ID of target to cause DBBR message to be logged (on bus 0 only) */
int	sz_eldb_dbbr0 = -1;
int	sz_eldb_dbbr1 = -1;
int	sz_eldb_dbbr2 = -1;
int	sz_eldb_dbbr3 = -1;

/* Set ID of target to cause unknown message error (bus 0 only) */
int	sz_eldb_buserr73 = -1;
#endif	ELDEBUG

sii_msgin(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int len, i;
    int retval;
    int targid = sii_getactive_target(sc);
    u_short olddmlotc = siiaddr->sii_dmlotc;
    int flags;

    /* Get the message from the scsi bus */
    if(sii_recvdata(sc, &sc->sc_message[targid], 1) != SZ_SUCCESS)
	return(SZ_RET_ABORT);

#ifdef	ELDEBUG
    if ((cntlr == 0) && (targid == sz_eldb_buserr73)) {
	    sc->sc_message[targid] = 0x5;	/* SZ_ID initiator detected error */
	sz_eldb_buserr73 = -1;
    }
#endif	ELDEBUG
    /* Switch on the type of message recieved */
    switch(sc->sc_message[targid]) {
    case SZ_CMDCPT:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_CMDCPT message\n"));
	    sc->sc_fstate = 0;
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	    sc->sc_actbp[targid] = -1;
	    sc->sc_dboff_busy[targid][0] = 0;
	    sc->sc_dboff_busy[targid][1] = 0;
	    /* Assumes one command at a time for each target */
	    if(sc->sc_dkn[targid] >= 0)
		dk_busy &= ~(1 << sc->sc_dkn[targid]);
	    sc->scsi_completed[targid] = 1;
#ifdef	ELDEBUG
	    if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
		(targid == sz_eldb_dbbr0)) {
		flags = SZ_HARDERR;
/* ELDEBUG */	scsi_logerr(sc, sc->sc_bp[targid], targid, SZ_ET_DBBR, 0, 0, flags);
		sz_eldb_dbbr0 = -1;
	    }
	    if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
		(targid == sz_eldb_dbbr1)) {
		flags = SZ_HARDERR;
/* ELDEBUG */	scsi_logerr(sc, sc->sc_bp[targid], targid, SZ_ET_DBBR, 1, 0, flags);
		sz_eldb_dbbr1 = -1;
	    }
	    if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
		(targid == sz_eldb_dbbr2)) {
		flags = SZ_SOFTERR;
/* ELDEBUG */	scsi_logerr(sc, sc->sc_bp[targid], targid, SZ_ET_DBBR, 2, 0, flags);
		sz_eldb_dbbr2 = -1;
	    }
	    if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
		(targid == sz_eldb_dbbr3)) {
		flags = SZ_HARDERR;
/* ELDEBUG */	scsi_logerr(sc, sc->sc_bp[targid], targid, SZ_ET_DBBR, 3, 0, flags);
		sz_eldb_dbbr3 = -1;
	    }
#endif	ELDEBUG
	    break;

    case SZ_SDP:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_SDP message\n"));
	    sc->sc_savcnt[targid] = olddmlotc;
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

	sc->sc_dboff_len[targid][sc->sc_actbp[targid]] -= sc->sc_savcnt[targid];
	siiaddr->sii_dmlotc = 0;

            /* Read the disconnect message now */
            if(sii_recvdata(sc, &sc->sc_message[targid], 1) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);
	    if(sc->sc_message[targid] != SZ_DISCON)
		break;
	/* FALL THROUGH */

    case SZ_DISCON:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_DISCON message\n"));
	    sc->sc_szflags[targid] |= SZ_WAS_DISCON;
	    siiaddr->sii_dmlotc = 0;
	    if (sc->sc_siidmacount[targid] != 0) {
	        sc->sc_szflags[targid] |= SZ_DMA_DISCON;
	    }
	    break;
		
    case SZ_EXTMSG:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_EXTMSG message\n"));
	    sc->sc_extmessg[targid][0] = sc->sc_message[targid];

            /* Read the extended message length */
            if(sii_recvdata(sc, &sc->sc_extmessg[targid][1], 1) != SZ_SUCCESS)
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
            if(sii_recvdata(sc, &sc->sc_extmessg[targid][2], len) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);
#ifdef SZDEBUG
	    /* JAG make a subroutine w/DEBUG */
	    PRINTD(targid, 0x4, ("sii_msgin: extended message:")); 
    	    for(i=0; i<(len+2); i++)
		PRINTD(targid, 0x4, (" %x", sc->sc_extmessg[targid][i]));
	    PRINTD(targid, 0x4, ("\n"));
#endif SZDEBUG

	    /*
	     * If the extended message is a Synchronous Data
	     * Transfer Request message then set the REQ/ACK
	     * offset for the current target otherwise reject
	     * the message.  Also reload the dmctrl register with
	     * the new (possibly) offset.
	     *
	     */
	    if(sc->sc_extmessg[targid][0] == SZ_SYNC_XFER) {
		if(sc->sc_extmessg[targid][4] > SII_SYNC)
		    sc->sc_extmessg[targid][4] = SII_SYNC;
		sc->sc_siireqack[targid] = sc->sc_extmessg[targid][4];
		siiaddr->sii_dmctrl = sc->sc_siireqack[targid];
	    }
	    if(sc->sc_alive[targid] == 0) { /* PROBING */
		u_long messg = SZ_MSGREJ;

	    	sii_assert_attn = 1;
	    	sii_reject_message = 0;
        	sii_senddata(sc, &messg, 1, SII_ATN);
	    }
	    break;

    case SZ_ID_NODIS:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_ID_NODIS message\n"));
	    break;

    case SZ_ID_DIS:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_ID_DIS message\n"));
	    break;

	case SZ_RDP:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_RDP message\n"));
	    break;

	case SZ_MSGREJ:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_MSGREJ message\n"));
	    break;

	case SZ_LNKCMP:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_LNKCMP message\n"));
	    break;

	case SZ_LNKCMPF:
	    PRINTD(targid, 0x4, ("sii_msgin: SZ_LNKCMPF message\n"));
	    break;

    default:
	    flags = SZ_HARDERR | SZ_LOGMSG;
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x73, 0, flags);
	    PRINTD(targid, 0x4, ("sii_msgin: unknown message = 0x%x\n",
		sc->sc_message[targid]));
	    return(SZ_RET_ABORT);
    }

    /*
     * Assert attention as long as the "sii_assert_attn" flag is
     * set. Attention gets deasserted during a message out phase
     * and the "sii_assert_attn" flags gets cleared.
     */
    if(sii_assert_attn) {
	siiaddr->sii_comm |= SII_ATN;
#ifdef mips
    wbflush();
#endif mips
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Send data to the scsi bus.
 *
 ******************************************************************/
sii_senddata(sc, data, count, attn)
register struct sz_softc *sc;
u_char *data;
int count;
int attn;
{
    int cnltr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int retval;
    int tmp_state;
    int tmp_phase;
    int i;
    int targid = sii_getactive_target(sc);

    /* Move the SII to the new phase */
    tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
    tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
    siiaddr->sii_comm = (tmp_state | tmp_phase | attn);
#ifdef mips
    wbflush();
#endif mips

    /* Send the data to the scsi bus using programmed IO */
    if(sii_use_programmed_io && (sc->sc_siireqack[targid] == 0)) {
        for(i=0; i<count; i++) {
	SZWAIT_UNTIL((siiaddr->sii_dstat & SII_TBE),sii_wait_count,retval);
	if (retval >= sii_wait_count) {
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4a, 0, SZ_HARDERR);
	    PRINTD(targid, 0x10, ("sii_senddata: SII_TBE not set\n"));
	    return(SZ_RET_ABORT);
	}
	siiaddr->sii_data = *data++;
	tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
	tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
	siiaddr->sii_comm = (SII_INXFER | tmp_state | tmp_phase);
#ifdef mips
    wbflush();
#endif mips
    }
    siiaddr->sii_dstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
    return(SZ_SUCCESS);
    }

    /* Send the data to the scsi bus using DMA */
    else {
#ifdef mips
    	(sc->wmbcopy)(data, (sc->sc_rambuff + sc->sc_siidboff[targid]), count);
#ifdef mips
    wbflush();
#endif mips
    	siiaddr->sii_dmaddrl = ((sc->sc_siidboff[targid]) & 0x00ffffff)/2;
    	siiaddr->sii_dmaddrh = ((((sc->sc_siidboff[targid]) & 0x00ffffff)/2)>>16);
#ifdef mips
    wbflush();
#endif mips
#endif mips
    	siiaddr->sii_dmlotc = count;
        siiaddr->sii_comm = 
			(SII_DMA | SII_INXFER | tmp_state | tmp_phase | attn);
#ifdef mips
    wbflush();
#endif mips

	/* Wait for DMA to complete */
        SZWAIT_UNTIL((siiaddr->sii_dstat & SII_DNE),sii_wait_count,retval);
        siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
	siiaddr->sii_dstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
	if(retval >= sii_wait_count) {
	    return(SZ_RET_ABORT);
	}
	return(SZ_SUCCESS);
    }
}

/******************************************************************
 *
 * Recieve data from the scsi bus.
 *
 ******************************************************************/

#ifdef	ELDEBUG
/* Set ID of target to cause buserr 0x49 (bus 0 only) */
/* DOES NOT WORK because programmed I/O is never used in sii_recvdata() */
int	sz_eldb_buserr49 = -1;
#endif	ELDEBUG

sii_recvdata(sc, data, count)
register struct sz_softc *sc;
u_char *data;
int count;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int retval;
    int tmp_state;
    int tmp_phase;
    int i;
    int targid = sii_getactive_target(sc);

    /* Move the SII to the new phase */
    tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
    tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
    siiaddr->sii_comm = (tmp_state | tmp_phase);
#ifdef mips
    wbflush();
#endif mips

    /* Recieve the data from the scsi bus using programmed IO */
    if(sii_use_programmed_io && (sc->sc_siireqack[targid] == 0)) {
        for(i=0; i<count; i++) {
	    SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_IBF|SII_MIS)),
		sii_wait_count, retval);
#ifdef	ELDEBUG
	    if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
		(targid == sz_eldb_buserr49)) {
		retval = sii_wait_count + 1;
		sz_eldb_buserr49 = -1;
	    }
#endif	ELDEBUG
	    if (retval >= sii_wait_count) {
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x49, 0, SZ_HARDERR);
		PRINTD(targid, 0x10, ("sii_recvdata: SII_IBF not set\n"));
		return(SZ_RET_ABORT);
	    }

	    /* If a phase change occured then we are done */
	    if(siiaddr->sii_dstat & SII_MIS)
		break;
	    *data++ = siiaddr->sii_data;
	    tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
	    tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
	    siiaddr->sii_comm = (SII_INXFER | tmp_state | tmp_phase);
#ifdef mips
	    wbflush();
#endif mips
	}
	siiaddr->sii_comm &= ~SII_INXFER;
	siiaddr->sii_dstat = SII_DNE;
#ifdef mips
	wbflush();
#endif mips
	return(SZ_SUCCESS);
    }

    /* Recieve the data from the scsi bus using DMA */
    else {
#ifdef vax
    	siiaddr->sii_dmaddrl = (sc->sc_siidboff[targid] & 0xffff);
    	siiaddr->sii_dmaddrh = (sc->sc_siidboff[targid] >> 16);
#endif vax
#ifdef mips
    	siiaddr->sii_dmaddrl = ((sc->sc_siidboff[targid]) & 0x00ffffff)/2;
    	siiaddr->sii_dmaddrh = ((((sc->sc_siidboff[targid]) & 0x00ffffff)/2) >> 16);
#ifdef mips
    wbflush();
#endif mips
#endif mips
    	siiaddr->sii_dmlotc = count;
        siiaddr->sii_comm = (SII_DMA | SII_INXFER | tmp_state | tmp_phase);
#ifdef mips
    wbflush();
#endif mips

      /* Wait for DMA to complete, MIS is also checked.  If the target changes
	phases, this loop will not have to time out. */

        SZWAIT_UNTIL((siiaddr->sii_dstat & (SII_DNE|SII_MIS)), sii_wait_count,
	    retval);
        siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);	/* set DNE */
	if(retval >= sii_wait_count && !(siiaddr->sii_dstat & SII_DNE))
	{
	    i = 10000;			/* wait for DNE */
	    while(--i && ((siiaddr->sii_dstat & SII_DNE) == 0))
		;
	    siiaddr->sii_dstat = SII_DNE;
	}
	else
	    siiaddr->sii_dstat = SII_DNE;
	siiaddr->sii_dmlotc = 0;
#ifdef mips
    wbflush();
#endif mips
	if(retval >= sii_wait_count && !(siiaddr->sii_dstat & SII_MIS))
	    return(SZ_RET_ABORT);
    	(sc->rmbcopy)((sc->sc_rambuff + sc->sc_siidboff[targid]), data, count);
#ifdef mips
    wbflush();
#endif mips
	return(SZ_SUCCESS);
    }
}


/******************************************************************
 *
 * Clear all disconnected IO tasks due to a BUS RESET.
 *
 ******************************************************************/
sii_clear_discon_io_tasks(sc)
register struct sz_softc *sc;
{

    int targid;
    int unit;
    struct buf *dp, *bp;

    PRINTD(targid, 0x10, ("sii_clear_discon_io_tasks: scanning IDs\n"));

    /* Find out if any targets have I/O requests that disconnected */
    for(targid=0; targid<NDPS; targid++) {
	if( sc->sc_siireqack[targid] !=0 )/* if set: need to renegotiate */
	    sc->sc_siisentsync[targid] = 0;
	if(targid == sc->sc_sysid)	/* skip initiator */
	    continue;
	if(sc->sc_alive[targid] == 0)	/* non existent target */
	    continue;
	unit = sc->sc_unit[targid];
	dp = (struct buf *)&szutab[unit];
	if(!dp->b_active)		/* target not active */
	    continue;
	if (dp->b_actf == NULL)
	    continue;			/* no IO requests pending */
	if(!(sc->sc_szflags[targid] & SZ_WAS_DISCON))
	{
	    sc->sc_resid[targid] = sc->sc_b_bcount[targid];
	    continue;			/* was not disconnected */
	}
	PRINTD(targid, 0x10,
	    ("sii_clear_discon_io_tasks: clearing ID %d\n", targid));
	bp = dp->b_actf;
	dp->b_actf = bp->av_forw;
	dp->b_active = 0;
	bp->b_resid = sc->sc_resid[targid];
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	biodone(bp);
	sc->sc_xstate[targid] = SZ_NEXT;
	sc->sc_xevent[targid] = SZ_BEGIN;
    }
}

/******************************************************************
 *
 * Reset the SII chip.
 *
 ******************************************************************/
sii_reset(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int targid;

    /* Reset the SII chip. */
    siiaddr->sii_comm = SII_CHRESET;
    /* Set arbitrated bus mode. */
    siiaddr->sii_csr = SII_HPM;
    /* SII is always ID 7, and set up max REQ/ACK offset. */
    siiaddr->sii_id = SII_ID_IO | sc->sc_sysid; 
    /* Enable SII to drive SCSI bus. */
    siiaddr->sii_dictrl = SII_PRE;
    /*
     * Assert SCSI bus reset for at least 25 Usec to clear the 
     * world. SII_RST is self clearing.
     */
    siiaddr->sii_comm = SII_RST;
    wbflush();
    DELAY(25);
    /*
     * Clear any pending interrupts from the reset.
     */
    siiaddr->sii_cstat = siiaddr->sii_cstat;
    siiaddr->sii_dstat = siiaddr->sii_dstat;
    /*
     * Set up SII for arbitrated bus mode, SCSI parity checking,
     * Select Enable, Reselect Enable, and Interrupt Enable.
     */
    siiaddr->sii_csr = (SII_HPM | SII_RSE | SII_SLE | SII_PCE | SII_IE);
    wbflush();
    
    /*
     * Clear all Active and Disconnected IO requests.
     */
    sii_clear_discon_io_tasks(sc);
    sc->sii_was_reset = 1;
    DELAY(sz_wait_for_devices * 1000000);
}

/******************************************************************
 *
 * Reset all the SII controllers (crash dump reset only).
 *
 ******************************************************************/
siireset()
{
#ifdef mips
    register struct sz_softc *sc = &sz_softc[0];
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
#endif mips

    /* Reset the SII chip. */
    siiaddr->sii_comm = SII_CHRESET;
    /* Set arbitrated bus mode. */
    siiaddr->sii_csr = SII_HPM;
    /* SII is always ID 7, and set up max REQ/ACK offset. */
    siiaddr->sii_id = SII_ID_IO | sc->sc_sysid; 
    /* Enable SII to drive SCSI bus. */
    siiaddr->sii_dictrl = SII_PRE;
    /*
     * Assert SCSI bus reset for at least 25 Usec to clear the 
     * world. SII_RST is self clearing.
     */
    siiaddr->sii_comm = SII_RST;
    DELAY(25);
    /*
     * Clear any pending interrupts from the reset.
     */
    siiaddr->sii_cstat = siiaddr->sii_cstat;
    siiaddr->sii_dstat = siiaddr->sii_dstat;
    /*
     * Set up SII for arbitrated bus mode, SCSI parity checking,
     * Select Enable, Reselect Enable, and Interrupt Enable.
     */
    siiaddr->sii_csr = (SII_HPM | SII_RSE | SII_SLE | SII_PCE | SII_IE);
    DELAY(5000000);
}

/******************************************************************
 *
 * Handle interrupts from the SII chip.
 *
 ******************************************************************/

#ifdef	ELDEBUG
/* Set to ID of target to log a parity error (bus 0 only) */
int	sz_eldb_parity = -1;
/* Set to ID of target to log a reset detected error (bus 0 only) */
int	sz_eldb_busrst = -1;
#endif	ELDEBUG

sii_intr(cntlr)
int cntlr;
{
    register struct sz_softc *sc = &sz_softc[cntlr];
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int targid, retval, timer;
    int dmacount, offset, tmp_phase, tmp_state;
    int handle_reselect;
    u_short cstat, dstat;
    int flags;

#ifdef mips
    if(sii_stray_intr[cntlr] != 1) { /* Stray interrupt on reboot */
	        switch (cntlr) {
		  case 0:    

#define KN01SII_ADDR   (struct sii_regs *)PHYS_TO_K1(0x1a000000)
		    /* note address is the same for pmax and mipsmate */
		    siiaddr = KN01SII_ADDR;
		    break;
		  case 1:
		    siiaddr = (struct sii_regs *)KN230_OPT_REG;
		    break;
		   
		  default:
		    printf("sii_intr stray, cntlr = %d, do not know chip address\n");
		    break;
		}
    		/* Reset the SII chip. */
    		siiaddr->sii_comm = SII_CHRESET;
                printf("sii_intr: noprobe intr\n");
		return;
    }
#endif mips
    /* Initialize variables */
    targid = sii_getactive_target(sc);
#ifdef vax
    sc->scsi_bus_idle = 0;
    sc->scsi_completed[targid] = 0;
#endif vax
    handle_reselect = 0;
    cstat = siiaddr->sii_cstat;
    dstat = siiaddr->sii_dstat;

    /* Check for interrupt from a disconnected target */
    if(targid == -1 || sc->sc_active == 0) {
	/* Check if there are valid interrupts pending */
	if(cstat & (SII_CI|SII_DI)) {

	    /* We must wait for a STATE CHANGE to occur first */
	    timer = 10000;
	    while(--timer && ((siiaddr->sii_cstat & SII_SCH) == 0));
	    if(timer == 0) {
/*cprintf("timer expired in interrupt, cstat= %x, dstat= %x\n", cstat, dstat);*/
		/*
		 * Check if a reselect occurred without the STATE
		 * CHANGE bit (SII_SCH) being set. This condition
		 * can occur when a reselect immediately follows a
		 * disconnect and therefore we only get one STATE
		 * CHANGE interrupt.
		 */
		cstat = siiaddr->sii_cstat;
		if((cstat & SII_DST_ONBUS) && 
				(cstat & SII_CON))
		    handle_reselect = 1;
		else {
		if(cstat & SII_RST_ONBUS) {
			siiaddr->sii_cstat &= ~SII_IE;
			siiaddr->sii_cstat = SII_RST_ONBUS;
			siiaddr->sii_cstat = SII_SCH;
			siiaddr->sii_cstat = SII_IE;
		}
	    		return;
		}
            }
        }
	/* No interrupts pending, spurious interrupt occurred */
	else {
	    PRINTD(0xFF, 0x1,
		("sii_intr: spurious interrupt from inactive target\n"));
	    return;
	}
    }
#ifdef	ELDEBUG
	if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
	    (targid == sz_eldb_parity)) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 1, 0, flags);
	    sz_eldb_parity = -1;
	}
	if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
	    (targid == sz_eldb_busrst)) {
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 1, 0, SZ_HARDERR);
	    sz_eldb_busrst = -1;
	    goto HANDLE_ERROR;
	}
#endif	ELDEBUG
	
XPRINTF(XPR_NFS, "intB: %x %x %x %x", sc->sc_siidmacount[targid],cstat,dstat,siiaddr->sii_comm);
    offset = (sc->sc_bpcount[targid] - sc->sc_siidmacount[targid]);
	if(cstat & (SII_CI|SII_DI)) {


        /* Check for a BUS ERROR */
        if(cstat & SII_BER) {
  	    siiaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	}

        /* Check for a PARITY ERROR */
        if(dstat & SII_IPE) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 1, 0, flags);
	    PRINTD(targid, 0x10, ("sii_intr: scsi %d parity error\n", cntlr));
	    goto HANDLE_ERROR;
        }

        /* Check for a BUS RESET */
        if(cstat & SII_RST_ONBUS) {
  	    siiaddr->sii_cstat = SII_RST_ONBUS;
#ifdef mips
    wbflush();
#endif mips
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 1, 0, SZ_HARDERR);
	    PRINTD(targid, 0x10, ("sii_intr: scsi %d bus reset\n", cntlr));
	    goto HANDLE_ERROR;
        }

        /* Check for a STATE CHANGE */
        while((siiaddr->sii_cstat & SII_SCH) || handle_reselect) { 
	    handle_reselect = 0;
	    PRINTD(targid, 0x4, ("sii_intr: state change occurred\n"));
	    if(sii_state_change(sc, &targid) != SZ_SUCCESS) {
		goto HANDLE_ERROR;
	    }

	    /* Check for a quick Reselect from the Disconnected target */
	    if(sc->scsi_bus_idle) {
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
	        if((cstat & SII_DST_ONBUS) &&
		   (cstat & SII_CON) &&
		   (siiaddr->sii_destat & SII_IDMSK) == targid) {
		    PRINTD(targid, 0x4,("sii_intr: quick reselect occurred\n"));
	    	    timer = 10000;
	    	    /* Wait for the STATE CHANGE bit to get set first */
	    	    while(--timer && !(siiaddr->sii_cstat & SII_SCH));
#ifdef out
if(timer <= 0)
	cprintf("state change never set within timer\n");
#endif out
/*
XPRINTF(XPR_NFS, "C1: %x", sc->sc_siidmacount[targid],0,0,0);
*/
		    continue;
		}
	    }
	    break;
	}

	/* If disconnected and went to BUS FREE STATE then break */
	if(sc->scsi_bus_idle)
	    goto done;

        /* Check for DMA COMPLETION */
        if(dstat & SII_DNE) {
	    siiaddr->sii_dstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
	    PRINTD(targid, 0x4,
		("sii_intr: transfer done occurred, (dmlotc = %d)\n",
		siiaddr->sii_dmlotc));
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	    if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		sc->sc_siidmacount[targid] -= 
			(SII_MAX_DMA_XFER_LENGTH - (sc->sc_savcnt[targid] ?
			sc->sc_savcnt[targid] : siiaddr->sii_dmlotc));
	    else
		sc->sc_siidmacount[targid] -= 
			(sc->sc_siidmacount[targid] - (sc->sc_savcnt[targid] ?
			sc->sc_savcnt[targid] : siiaddr->sii_dmlotc));

	}

        /* Check for a PHASE MISMATCH */
        if(dstat & SII_MIS) {

            /* Check for a BUS ERROR */
            if(cstat & SII_BER) {
  	        siiaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* If a transfer was in progress, but didn't generate */
	    /* a DNE because the SII was setup for a larger DMA   */
	    /* transfer than actually occurred, then clear the    */
	    /* transfer command and wait for the DNE interrupt.   */
	    if(siiaddr->sii_comm & SII_INXFER) {
	        siiaddr->sii_comm &= ~SII_INXFER;
	        timer = 10000;
	        while(--timer && ((siiaddr->sii_dstat & SII_DNE) == 0));
#ifdef mips
    wbflush();
#endif mips
		siiaddr->sii_dstat = SII_DNE;

#ifdef mips
    wbflush();
		dstat = siiaddr->sii_dstat;
#endif mips
	/* Check for ODD BYTE BOUNDARY condition */
	if((dstat & SII_OBB)) {
		if(siiaddr->sii_dmlotc) {
/*			cprintf("OBB %x %x\n", sc->sc_savcnt[targid], siiaddr->sii_dmlotc); */
			sc->sc_siioddbyte[targid] = siiaddr->sii_dmabyte;
			siiaddr->sii_dmlotc++;
			wbflush();
		}/* else
			cprintf("OBB done %x %x %x\n", sc->sc_dboff_len[targid][sc->sc_actbp[targid]], dstat, siiaddr->sii_comm); */
	}
	        /* Clear DMA in progress flag and adjust DMA count */
	        sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	        if(sc->sc_siidmacount[targid] > SII_MAX_DMA_XFER_LENGTH)
		    sc->sc_siidmacount[targid] -= 
			    (SII_MAX_DMA_XFER_LENGTH - (sc->sc_savcnt[targid] ? sc->sc_savcnt[targid] : siiaddr->sii_dmlotc));
	        else
		    sc->sc_siidmacount[targid] -= 
			    (sc->sc_siidmacount[targid] - (sc->sc_savcnt[targid] ? sc->sc_savcnt[targid] : siiaddr->sii_dmlotc));

	    }

	    /* Handle the current bus phase */
	    if(sii_phase_change(sc, dstat) != SZ_SUCCESS) {
/*		cprintf("intr: phase change error\n"); */
	        goto HANDLE_ERROR;
	    }


/* THIS CODE BELOW GOES IF WE WRITE NEW COMM */
            /* Wait for the next bus phase */
            if(!sc->scsi_completed[targid] && 
		  !(sc->sc_szflags[targid] & (SZ_WAS_DISCON|SZ_DID_DMA))) {
		goto done;
#ifdef out
                SZWAIT_UNTIL((siiaddr->sii_dstat & SII_MIS),sii_wait_count,retval);
		dstat = siiaddr->sii_dstat;
                if((retval >= sii_wait_count) &&
			!(siiaddr->sii_dstat & SII_IBF)) {
		    /* JAG BAD ERROR remove later */
	            cprintf("sii_intr: SII_MIS didn't set\n");
		    PRINTD(targid, 0x10, ("sii_intr: SII_MIS didn't set\n"));
	            goto HANDLE_ERROR;
		}
#endif out
            }
	}

	/* Check for fragmented DMA transfers (>8K) */
	if((dstat & (SII_TBE|SII_IBF)) && !(sc->sc_szflags[targid] & SZ_DID_DMA) &&
	  	!(dstat & SII_MIS) &&
	  		((sc->sc_fstate == SZ_DATAI_PHA) || 
				(sc->sc_fstate == SZ_DATAO_PHA))) {

	    /* If the DMA was restarted then return */
	    int oldact = sc->sc_actbp[targid];
	    if(sii_restartdma(sc)) {
		if(oldact != -1) {
/*
XPRINTF(XPR_NFS, "oa0: %x %d %x", sc->sc_siidmacount[targid],oldact, offset,0);
*/
			if((sc->sc_iodir[targid] & SZ_DMA_READ)) { /* Read was done */
			    int len = sc->sc_dboff_len[targid][oldact];
XPRINTF(XPR_NFS, "+oa0: f %x t %x l %d", sc->sc_rambuff + (sc->sc_dboff[targid][oldact]*2),sc->sc_bufp[targid] + sc->sc_xfercnt[targid]+offset, len,0);

			  /* Compare the current/end location for the
			    dest aginst the orig count for the section from
			    sc_b_bount.  This will abort xfers past the end of
			    the user's buffer. */

			    if( (offset + len) > sc->sc_b_bcount[targid] )
			    {
			      /* Calc the new len to the end. */
				len = sc->sc_b_bcount[targid] - offset;
			    }

			    if( len > 0 )  /* anything to copy */
			    {
				(sc->rmbcopy)(sc->sc_rambuff +
				    (sc->sc_dboff[targid][oldact]*2), 
				    sc->sc_bufp[targid] +
				    sc->sc_xfercnt[targid] + offset, len);
			    }

			if(sc->sc_dboff_busy[targid][1-oldact])
				sc->sc_actbp[targid] = 1-oldact;
			else
				sc->sc_actbp[targid] = -1;
			}
	        if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
	    	    sc->sc_actbp[targid] = -1;
		    sc->sc_dboff_busy[targid][0] = 0;
		    sc->sc_dboff_busy[targid][1] = 0;
	        }
		sc->sc_dboff_busy[targid][oldact] = 0;
		}
		return;
	    }
/*
	    else {
		cprintf("sii_intr: DMA error in sii_restart: aborting\n");
	        goto HANDLE_ERROR;
	    }
*/
	}

/* THIS MAY GO TOO IF WE WRITE THE COMM */
	/* Sometimes the target stays in the same phase */
	if((dstat & (SII_IBF|SII_TBE)) && 
		!(dstat & SII_MIS) &&
			((sc->sc_fstate != SZ_DATAI_PHA) &&
				(sc->sc_fstate != SZ_DATAO_PHA))) {

            /* Check for a BUS ERROR */
            if(cstat & SII_BER) {
  	        siiaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* Handle the current bus phase */
	    if(sii_phase_change(sc, dstat) != SZ_SUCCESS) {
/*		cprintf("intr: phase change error 2\n"); */
	        goto HANDLE_ERROR;
	    }
	}
	}

done:
XPRINTF(XPR_NFS, "intE: %x %d %d", sc->sc_siidmacount[targid],sc->scsi_bus_idle,sc->scsi_completed[targid],0);
/*cprintf("intE: %x %d %d\n", sc->sc_siidmacount[targid],sc->scsi_bus_idle,sc->scsi_completed[targid]); */
    if(!(sc->sc_szflags[targid] & (SZ_DID_DMA))) {
	int oldact = sc->sc_actbp[targid];
	if(oldact != -1) {
/*
XPRINTF(XPR_NFS, "oa: %x %d %x", sc->sc_siidmacount[targid],oldact, offset,0);
*/
		if((sc->sc_iodir[targid] & SZ_DMA_READ))
		{ /* Read was done */
		    int len = sc->sc_dboff_len[targid][oldact];
XPRINTF(XPR_NFS, "+oa: f %x t %x l %d", sc->sc_rambuff + (sc->sc_dboff[targid][oldact]*2),sc->sc_bufp[targid] + sc->sc_xfercnt[targid]+offset, len,0);

		  /* Compare the current/end location for the
		    dest aginst the orig count for the section from sc_b_bount.
		    This will abort xfers past the end of
		    the user's buffer. */

		    if( (offset + len) > sc->sc_b_bcount[targid] )
		    {
		      /* Calc the new len to the end. */
			len = sc->sc_b_bcount[targid] - offset;
		    }

		    if( len > 0 )  /* anything to copy */
		    {
			(sc->rmbcopy)(sc->sc_rambuff +
			    (sc->sc_dboff[targid][oldact]*2), 
			    sc->sc_bufp[targid] +
			    sc->sc_xfercnt[targid] + offset, len);
		    }
/*
XPRINTF(XPR_NFS, "aoa: %x %d %x %x", sc->sc_dboff_busy[targid][1-oldact],oldact, sc->sc_actbp[targid],sc->sc_szflags[targid]);
*/
		if(sc->sc_dboff_busy[targid][1-oldact])
			sc->sc_actbp[targid] = 1-oldact;
		else
			sc->sc_actbp[targid] = -1;
		} 
	if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
		sc->sc_actbp[targid] = -1;
		sc->sc_dboff_busy[targid][0] = 0;
		sc->sc_dboff_busy[targid][1] = 0;
	}
	sc->sc_dboff_busy[targid][oldact] = 0;
	}
    }
    /*
     * Check the status of the current SCSI operation. If the SCSI
     * operation completed or disconnected then start the next SCSI 
     * operation, otherwise wait for the DMA to complete.
     * 
     */
    if(sc->scsi_bus_idle) {
        if(sc->scsi_completed[targid])
	{
	  /* The command has completed, check for BUSY status.  If the target
	    was busy, leave the command on the queue, dp->b_active = 1.  Setup
	    a timer to wait a bit.  Call sz_start() to kick off the next
	    command on the queue.  The timer routine will handle "re-queueing"
	    the command in the state machine. */

	    if( (sc->sc_szflags[ targid ] & SZ_BUSYTARG) != 0 )
	    {
#ifdef vax
	      /* Clear any pending timeout for this target, we will be starting
		a new timer. */
		if(sc->sc_szflags[targid] & SZ_TIMERON)
		{
		    untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
		    sc->sc_szflags[targid] &= ~SZ_TIMERON;
		}
#endif vax
		timeout( sii_busy_target, (caddr_t)sc->sc_unit[ targid ], 
		    SZ_BUSY_WAIT );		/* wait for the targit */
		sc->scsi_bus_idle = 0;
		PRINTD(targid, 0x4,
		    ("sii_intr: COMMAND COMPLETED with BUSY\n"));
		sc->sc_active = 0;
		sc->scsi_completed[targid] = 0;
		sz_start( sc, -1 );		/* Start next I/O request */
		return;
	    }
	    else
	    {
		sc->scsi_bus_idle = 0;
		PRINTD(targid, 0x4,
		    ("sii_intr: COMMAND COMPLETED successfully\n"));
		sc->sc_active = 0;
		sc->scsi_completed[targid] = 0;
		sz_start(sc, targid);		/* Finish current I/O request */
		return;
	    }
        }
	else if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
	    PRINTD(targid, 0x4,
		("sii_intr: COMMAND IN PROGRESS disconnected\n"));
	    sc->sc_active = 0;
    	    sz_start(sc, -1);			/* Start next I/O request */
	return; /* NEW */
#ifdef mips
	}
#endif mips
	else {
#ifdef NO_TIMER	/* JAG */
	    if(sc->sc_szflags[targid] & SZ_TIMERON) {
	        untimeout(sii_timer, (caddr_t)sc->sc_unit[targid]);
	        sc->sc_szflags[targid] &= ~SZ_TIMERON;
	    }
#endif NO_TIMER	/* JAG */
            sc->sc_active = 0;
	    sc->sc_fstate = 0;
	    /* sc->sc_szflags[targid] = (SZ_NEED_SENSE|SZ_RETRY_CMD); */
	    sc->sc_szflags[targid] = (SZ_NEED_SENSE);
	    sz_start(sc, targid);
	    return;
	}
    } 
    else {
	PRINTD(targid, 0x4,
	    ("sii_intr: COMMAND IN PROGESS dma interrupt mode\n"));
#ifdef out
        if(sc->sc_bp[targid] && sc->sc_alive[targid] && sc->scsi_completed[targid]) {
	    sc->scsi_completed[targid] = 0;
    	    sz_start(sc, targid);		/* Finish current I/O request */
        }
#endif out
	return;
    }

HANDLE_ERROR:
    /* Abort the current SCSI operation due to error */

    PRINTD(targid, 0x4,
	("sii_intr: command aborted (bus=%d target=%d cmd=0x%x)\n",
	cntlr, targid, sc->sc_curcmd[targid]));
    PRINTD(targid, 0x4, ( "", sii_dumpregs(cntlr, 0)));

    flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
    scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 1, 0, flags);
    sii_reset(sc);
    sc->sc_selstat[targid] = SZ_IDLE;
    sc->sc_szflags[targid] |= SZ_ENCR_ERR;
    sc->sc_xstate[targid] = SZ_ERR;
    sc->sc_xevent[targid] = SZ_ABORT;
    siiaddr->sii_cstat = 0xffff;
    siiaddr->sii_dstat = 0xffff;
#ifdef mips
    wbflush();
#endif mips
    sc->sc_active = 0;
    sz_start(sc, targid);
}

/******************************************************************
 *
 * Restart a DMA transfer that is (>8K). The SII only handles
 * DMA transfers of upto 8K at a time. For large DMA transfers
 * the SII must restart the DMA after each 8K transfer.
 *
 ******************************************************************/
sii_restartdma(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int dmacount, offset, which = -1;
    int tmp_state, tmp_phase;
    int targid = sii_getactive_target(sc);
    
    siiaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
    siiaddr->sii_cstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
    offset = (sc->sc_bpcount[targid] - sc->sc_siidmacount[targid]);



    if(sc->sc_siidmacount[targid] > 0) {

            /* Calculate the DMA transfer length */
            dmacount = sc->sc_siidmacount[targid];
    	    if(dmacount > SII_MAX_DMA_XFER_LENGTH)
                dmacount = SII_MAX_DMA_XFER_LENGTH;
    
	    if(sc->sc_actbp[targid] == -1) {
	    /* Get idle buffer */
	    which = get_validbuf(sc, targid);

	    if(which == -1) {
/*		cprintf("cant get valid buffer - die\n"); */
		return;
	    }

	    sc->sc_actbp[targid] = which;

	    /* Copy data to RAM buffer if DMA WRITE operation */
    	    if(sc->sc_iodir[targid] == SZ_DMA_WRITE) {

		int len = dmacount;	/* just a local var for the copy */

XPRINTF(XPR_NFS, "rsta w: %x w=%d f %x t %x", sc->sc_siidmacount[targid],which,sc->sc_bufp[targid] + sc->sc_xfercnt[targid]+offset, sc->sc_rambuff + (sc->sc_dboff[targid][which]*2));

	      /* Compare the current/end location for the src aginst the
		orig count for the section from sc_b_bount.  This will
		abort xfers past the end of the user's buffer. */

		if( (offset + len) > sc->sc_b_bcount[targid] )
		{
		  /* Calc the new len to the end. */
		    len = sc->sc_b_bcount[targid] - offset;

		  /* Zero out the half of the RAM buffer, this will deal with
		    the zero fill requirement. */

		    (sc->wmbzero)(sc->sc_rambuff +
			(sc->sc_dboff[targid][which]*2), dmacount);
		    wbflush();
		}

		if( len > 0 )  /* anything to copy */
		{
		    (sc->wmbcopy)( sc->sc_bufp[targid] + sc->sc_xfercnt[targid]
			+ offset, sc->sc_rambuff +
			(sc->sc_dboff[targid][which]*2), len);
		    wbflush();
		}
	    }
	    } else {
		which = sc->sc_actbp[targid];
		if(sc->sc_iodir[targid] == SZ_DMA_READ) {
		if(sc->sc_dboff_busy[targid][which]) {
			sc->sc_dboff_busy[targid][which] = 0;
			which = 1-which;
		}
		} else {
		sc->sc_dboff_busy[targid][which] = 0;
		which = 1 - sc->sc_actbp[targid];
		sc->sc_actbp[targid] = which;
		}
/*
XPRINTF(XPR_NFS, "rsta +w: %x w=%d cnt=%x", sc->sc_siidmacount[targid],which,dmacount,0);
*/
	    }
	    sc->sc_dboff_busy[targid][which] = 1;
	    sc->sc_dboff_len[targid][which] = dmacount;
	    
    
        /* Set the starting address in the 128K RAM buffer. */
#ifdef vax
        siiaddr->sii_dmaddrl = 
    			((sc->sc_dboff[targid][which]) & 0xffff);
        siiaddr->sii_dmaddrh = 
    			((sc->sc_dboff[targid][which]) >> 16);
#endif vax
#ifdef mips
        siiaddr->sii_dmaddrl = 
    			(((sc->sc_dboff[targid][which])) & 0x00ffffff);
        siiaddr->sii_dmaddrh = 
    			(((((sc->sc_dboff[targid][which])) & 0x00ffffff))>>16);
#endif mips
    	siiaddr->sii_dmlotc = dmacount;
	siiaddr->sii_dmabyte = sc->sc_siioddbyte[targid];
#ifdef mips
    wbflush();
#endif mips
        PRINTD(targid, 0x04,
	    ("sii_restartdma: offset = %d count = %d\n", offset, dmacount));

    	/* Restart the DMA operation */
        sc->sc_szflags[targid] |= SZ_DID_DMA;
        tmp_phase = siiaddr->sii_dstat & SII_PHA_MSK;
        tmp_state = siiaddr->sii_cstat & SII_STATE_MSK;
        siiaddr->sii_csr |= SII_IE;
        siiaddr->sii_comm = 
    		    	(SII_DMA | SII_INXFER | tmp_state | tmp_phase);
#ifdef mips
    wbflush();
#endif mips
	    if(sc->sc_siidmacount[targid] - dmacount >= 0) {
		offset += dmacount; /* Bump user offset by last xfer */
		/* Copy data to RAM buffer if DMA WRITE operation */
    	   	 if(sc->sc_iodir[targid] == SZ_DMA_WRITE) {
	    	        /* Get idle buffer */
	    	        which = get_validbuf(sc, targid);
            		/* Calculate the DMA transfer length */
            		dmacount = sc->sc_siidmacount[targid] - dmacount;
    	    		if(dmacount > SII_MAX_DMA_XFER_LENGTH)
                		dmacount = SII_MAX_DMA_XFER_LENGTH;
			if(which != -1 && dmacount > 0) {

			    int len = dmacount;	/* local var for the copy */

XPRINTF(XPR_NFS, "rsta +w0: %x w=%d f %x t %x", sc->sc_siidmacount[targid],
which,sc->sc_bufp[targid] + sc->sc_xfercnt[targid]+offset,
sc->sc_rambuff + (sc->sc_dboff[targid][which]*2));

			  /* Compare the current/end location for the src
			    aginst the orig count for the section from
			    sc_b_bount.  This will abort xfers past the
			    end of the user's buffer. */

			    if( (offset + len) > sc->sc_b_bcount[targid] )
			    {
			      /* Calc the new len to the end. */
				len = sc->sc_b_bcount[targid] - offset;

			      /* Zero out the half of the RAM buffer, this
				will deal with the zero fill requirement. */

				(sc->wmbzero)( sc->sc_rambuff +
				    (sc->sc_dboff[targid][which]*2), dmacount);
				wbflush();
			    }

			    if( len > 0 )  /* anything to copy */
			    {
				(sc->wmbcopy)( sc->sc_bufp[targid] +
				    sc->sc_xfercnt[targid] + offset,
				    sc->sc_rambuff +
				    (sc->sc_dboff[targid][which]*2), len);
				wbflush();
			    }
			sc->sc_dboff_busy[targid][which] = 1;
	    		}
		} 
	     }
    	return(1);
    }
    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
    return(0);
}

/******************************************************************
 *
 * Handle a state change on the SII chip.
 *
 ******************************************************************/
sii_state_change(sc, activetargid)
register struct sz_softc *sc;
int *activetargid;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int targid, timer;
    u_short cstat;

    siiaddr->sii_cstat = SII_SCH;
#ifdef mips
    wbflush();
#endif mips

  /* If targid is -1, there is no active target and  the SII will soon be
    connected to.  Wait and make sure the CON is set. */

    if(*activetargid == -1)
    {
      /* CON can assert much later than DST so spin a bit... */
	timer = 1000;
	while (((siiaddr->sii_cstat & SII_CON) == 0) && --timer)
	    ;
    }

    cstat = siiaddr->sii_cstat;
    /* Handle a select or reselect here */
    if(cstat & SII_CON) {

	sc->scsi_bus_idle = 0;
	/* Handle a reselect */
	if(cstat & SII_DST_ONBUS) {
	    targid = (siiaddr->sii_destat & SII_IDMSK);
	    PRINTD(targid, 0x4,
		("sii_state_change: target ID %d reselected\n",targid));
	    sc->sc_active = (1 << targid);
	    sc->sc_selstat[targid] = SZ_RESELECT;
    	    sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;
	    siiaddr->sii_comm = (siiaddr->sii_cstat & SII_STATE_MSK);
#ifdef mips
    wbflush();
#endif mips
	}
	/* Handle a select */
	else {
	    targid = (siiaddr->sii_slcsr & SII_IDMSK);
	    PRINTD(targid, 0x4,
		("sii_state_change: target ID %d selected\n",targid));
	    sc->sc_active = (1 << targid);
	    sc->sc_selstat[targid] = SZ_SELECT;
	}
	if(!sc->scsi_polled_mode) {
            siiaddr->sii_csr &= ~(SII_SLE | SII_RSE);
#ifdef mips
    wbflush();
#endif mips
	}
      /* Set the sii_dmctrl register to the rec/ack offset for the target.
	It does not matter if the state change is a selection or reselection,
	the sii_dmctrl needes to be set if/when data phase is entered. */

        siiaddr->sii_dmctrl = sc->sc_siireqack[targid];
#ifdef mips
    wbflush();
#endif mips

    }
    /* Handle a disconnect here */
    else {
	sc->scsi_bus_idle = 1;		/* because !CON bus is idle */
	siiaddr->sii_comm = 0;
#ifdef mips
    wbflush();
#endif mips
        siiaddr->sii_dmctrl = 0;	/* clear out any previous offset */
#ifdef mips
    wbflush();
#endif mips
	/*
	 * Check for a back-to-back disconnect occurring in 
	 * which case the active target ID will be -1 and the 
	 * SCSI bus will be idle. KLUDGE for CDROM device.
	 */
	if(*activetargid == -1)
	    return(SZ_SUCCESS);

	targid = sii_getactive_target(sc);
	PRINTD(targid, 0x4,
	    ("sii_state_change: target ID %d disconnected\n",targid));
	if(sc->scsi_completed[targid])
	    sc->sc_selstat[targid] = SZ_IDLE;
	else
	    sc->sc_selstat[targid] = SZ_DISCONN;
	if(!sc->scsi_polled_mode)
            siiaddr->sii_csr |= (SII_SLE | SII_RSE | SII_IE);
#ifdef mips
	wbflush();
#endif mips
    }
    *activetargid = targid;
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Handle a phase change on the SII chip.
 *
 ******************************************************************/

#ifdef	ELDEBUG
/* Set ID of target to cause unknown phase error (bus 0 only) */
int	sz_eldb_buserr4c = -1;
#endif	ELDEBUG

sii_phase_change(sc, dstat)
register struct sz_softc *sc;
u_short dstat;
{
    int cntlr = sc->sc_siinum;
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int tmp_state;
    int tmp_phase;
    int phase;
    int targid = sii_getactive_target(sc);

    /* Get the current bus phase */
    phase = ((dstat & SII_PHA_MSK) << 2);
/* WHY ISNT COMM WRITTEN HERE AS IN MN DRIVER */

#ifdef out
    /* Switch on the new phase */
    { extern unsigned xpr_flags;
	xpr_flags = XPR_NFS;
	XPRINTF(XPR_NFS, "phase: %x %x", phase, sc->sc_fstate,0,0);
    }
#endif out
    PRINTD(targid, 0x4, ("sii_phase_change: current bus phase = "));
    PRINTD(targid, 0x4, ("", sii_print_phase(phase)));

#ifdef	ELDEBUG
    if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
	(targid == sz_eldb_buserr4c)) {
	phase = 5;
	sz_eldb_buserr4c = -1;
    }
#endif	ELDEBUG

    switch(phase) {
    case SCS_MESSI:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_MESSI_PHA;
	if(sii_msgin(sc) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_MESSO:
        sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_MESSO_PHA;
	sc->sc_szflags[targid] = 0;
	if(sii_msgout(sc) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_CMD:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_CMD_PHA;
	if(sii_sendcmd(sc) != SZ_SUCCESS)
	   return(SZ_RET_ABORT);
	break;

    case SCS_STATUS:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_STATUS_PHA;
	if(sii_getstatus(sc) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_DATAO:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAO_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];
	if(sii_startdma(sc, SZ_DMA_WRITE) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    case SCS_DATAI:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAI_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];
	if(sii_startdma(sc, SZ_DMA_READ) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);
	break;

    default:
	PRINTD(targid, 0x4, ("sii_phase_change: unexpected bus phase = "));
	PRINTD(targid, 0x4, ("", sii_print_phase(phase)));
	scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4c, 0, SZ_HARDERR);
	return(SZ_RET_ABORT);
	break;
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Dump out the registers on the SII chip.
 *
 ******************************************************************/
sii_dumpregs(cntlr, who)
int	cntlr;
int	who;
{
        register struct sii_regs *siiaddr =
		(struct sii_regs *)sz_softc[cntlr].sc_scsiaddr;

/*
	printd1("\t\tSII register dump:\n");
	printd1("\t\tsii_sc1 = 0x%x\n", siiaddr->sii_sc1 & 0xffff);
	printd1("\t\tsii_csr = 0x%x\n", siiaddr->sii_csr & 0xffff);
	printd1("\t\tsii_id = 0x%x\n", siiaddr->sii_id & 0xffff);
	printd1("\t\tsii_slcsr = 0x%x\n", siiaddr->sii_slcsr & 0xffff);
	printd1("\t\tsii_destat = 0x%x\n", siiaddr->sii_destat & 0xffff);
	printd1("\t\tsii_data = 0x%x\n", siiaddr->sii_data & 0xffff);
	printd1("\t\tsii_dmctrl = 0x%x\n", siiaddr->sii_dmctrl & 0xffff);
	printd1("\t\tsii_dmlotc = 0x%x\n", siiaddr->sii_dmlotc & 0xffff);
	printd1("\t\tsii_dmaddrl = 0x%x\n", siiaddr->sii_dmaddrl & 0xffff);
	printd1("\t\tsii_dmaddrh = 0x%x\n", siiaddr->sii_dmaddrh & 0xffff);
	printd1("\t\tsii_dmabyte = 0x%x\n", siiaddr->sii_dmabyte & 0xffff);
	printd1("\t\tsii_cstat = 0x%x\n", siiaddr->sii_cstat & 0xffff);
	printd1("\t\tsii_dstat = 0x%x\n", siiaddr->sii_dstat & 0xffff);
	printd1("\t\tsii_comm = 0x%x\n", siiaddr->sii_comm & 0xffff);
*/
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
	printf("\t\tSCSI register dump:\n");
	printf("\t\tsii_sc1 = 0x%x\n", siiaddr->sii_sc1 & 0xffff);
	printf("\t\tsii_csr = 0x%x\n", siiaddr->sii_csr & 0xffff);
	printf("\t\tsii_id = 0x%x\n", siiaddr->sii_id & 0xffff);
	printf("\t\tsii_slcsr = 0x%x\n", siiaddr->sii_slcsr & 0xffff);
	printf("\t\tsii_destat = 0x%x\n", siiaddr->sii_destat & 0xffff);
	printf("\t\tsii_data = 0x%x\n", siiaddr->sii_data & 0xffff);
	printf("\t\tsii_dmctrl = 0x%x\n", siiaddr->sii_dmctrl & 0xffff);
	printf("\t\tsii_dmlotc = 0x%x\n", siiaddr->sii_dmlotc & 0xffff);
	printf("\t\tsii_dmaddrl = 0x%x\n", siiaddr->sii_dmaddrl & 0xffff);
	printf("\t\tsii_dmaddrh = 0x%x\n", siiaddr->sii_dmaddrh & 0xffff);
	printf("\t\tsii_dmabyte = 0x%x\n", siiaddr->sii_dmabyte & 0xffff);
	printf("\t\tsii_cstat = 0x%x\n", siiaddr->sii_cstat & 0xffff);
	printf("\t\tsii_dstat = 0x%x\n", siiaddr->sii_dstat & 0xffff);
	printf("\t\tsii_comm = 0x%x\n", siiaddr->sii_comm & 0xffff);
}

#ifdef SZDEBUG
/******************************************************************
 *
 * Print out the current bus phase.
 *
 ******************************************************************/
sii_print_phase(phase)
int phase;
{

    switch(phase) {
    case SCS_DATAO:
	PRINTD(0xFF, 0x4, ("SCS_DATAO\n"));
	break;
    case SCS_DATAI:
	PRINTD(0xFF, 0x4, ("SCS_DATAI\n"));
	break;
    case SCS_MESSI:
	PRINTD(0xFF, 0x4, ("SCS_MESSI\n"));
	break;
    case SCS_MESSO:
	PRINTD(0xFF, 0x4, ("SCS_MESSO\n"));
	break;
    case SCS_CMD:
	PRINTD(0xFF, 0x4, ("SCS_CMD\n"));
	break;
    case SCS_STATUS:
	PRINTD(0xFF, 0x4, ("SCS_STATUS\n"));
	break;
    default:
	PRINTD(0xFF, 0x4, ("UNKNOWN\n"));
	break;
    }
}

/******************************************************************
 *
 * Print out the current command status.
 *
 ******************************************************************/
sii_print_status(status)
int status;
{

    switch(status) {
    case SZ_GOOD:
	PRINTD(0xFF, 0x24, ("SZ_GOOD "));
	break;
    case SZ_CHKCND:
	PRINTD(0xFF, 0x24, ("SZ_CHKCND "));
	break;
    case SZ_INTRM:
	PRINTD(0xFF, 0x24, ("SZ_INTRM "));
	break;
    case SZ_RESCNF:
	PRINTD(0xFF, 0x24, ("SZ_RESCNF "));
	break;
    case SZ_BUSY:
	PRINTD(0xFF, 0x24, ("SZ_BUSY "));
	break;
    default:
	PRINTD(0xFF, 0x24, ("??? "));
	break;
    }
    PRINTD(0xFF, 0x24, ("\n"));
}

/******************************************************************
 *
 * Print out the inquiry data information.
 *
 ******************************************************************/
sii_print_inq_info(idp)
struct sz_inq_dt *idp;
{
    
    char hold[SZ_PID_LEN+1];
    int i;
    u_char *ptr;

    PRINTD(0xFF, 0x20, ("Dumping Out Inquiry Data from %x:\n", idp));
    for(i=0; i<SZ_VID_LEN; i++)
	hold[i] = idp->vndrid[i];
    hold[i] = '\0';
    PRINTD(0xFF, 0x20, ("Vendor ID = %s\n", hold));
    for(i=0; i<SZ_PID_LEN; i++)
	hold[i] = idp->prodid[i];
    hold[i] = '\0';
    PRINTD(0xFF, 0x20, ("Product ID = %s\n", hold));
    PRINTD(0xFF, 0x20, ("Peripheral Device Type = %x\n",idp->perfdt));
    PRINTD(0xFF, 0x20, ("Device Type Qualifier = %x\n",idp->devtq));
    for(i=0; i<SZ_REV_LEN; i++)
	hold[i] = idp->revlvl[i];
    hold[i] = '\0';
    PRINTD(0xFF, 0x20, ("Revision Level = %s\n", hold));
}
#endif SZDEBUG

/* ---------------------------------------------------------------------- */

/* Move data from user space to the RAM buffer.  In the xfer loops the RAM
buffer pointer is incremented twice.  The 16 bit words in the RAM buffer are
aligned on 32 bit boundries.  The data is moves and then short pointer is
incremented again to the next word in the RAM buffer. */

int
wmbcopy(src, dst, len)
    register u_char *src;	/* pointer to user data */
    register u_short *dst;	/* pointer to the RAM buffer */
    register u_int len;		/* count of bytes to xfer */
{
    int oddmv;			/* flag for odd count in length */
    register u_short tword;	/* local copy for word xfers to RAM */
    register u_short *wsrc;	/* for word aligned xfers */

    PRINTD(0xFF, 0x1, ("wmbcopy: s %x d %x l %d\n", src, dst, len ));

  /* Check for an odd count in len.  The byte will be moved at the bottom of
    the routine. */

    oddmv = (len & 0x1);	/* set the odd count flag is true */

    len >>= 1;			/* shift to word count */

    if( len != 0 )		/* was there only one byte to move ? */
    {
      /* If the source address is odd the data is "unaligned".  The xfer
	will have to be done a word at a time.  The local register
	variable tword is used to load the bytes from user space, one
	at a time, and then write the word to the RAM buffer.  This
	allows a single access to the RAM buffer for the word write. */

	if((u_int)src & 0x01)
	{
	    while(len-- != 0)			/* compare aginst 0 */
	    {
		tword = *src++;			/* fill lsb */
		tword |= (*src++ << 8);		/* fill msb */
		*dst++ = tword; dst++;		/* load word, incr to next */
	    }
	}
	else 		/* user data is word aligned */
	{
	    wsrc = (u_short *)src;		/* copy to word pointer */

	  /* The data xfer loops are unrolled.  The len variable is shifted
	    left to the next byte boundry and data is moved if necessary. 
	    The final xfer loop is a 16 byte while loop. */

	    if( len & 0x1 )			/* word check */
	    {
		*dst++ = *wsrc++; dst++;	/* move word */
	    }

	    len >>= 1;				/* move to long word bnd */
	    if( len != 0 )			/* any more data */
	    {
		if( len & 0x1 )			/* long word check */
		{
		    *dst++ = *wsrc++; dst++;	/* move long word */
		    *dst++ = *wsrc++; dst++;
		}

		len >>= 1;			/* move to quad word bnd */
		if( len != 0 )			/* any more data */
		{
		    if( len & 0x1 )		/* quad word check */
		    {
			*dst++ = *wsrc++; dst++;/* move quad word */
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
		    }

		    len >>= 1;			/* move to hex word bnd */
		    while( len-- != 0 )		/* loop till out of data */
		    {
			*dst++ = *wsrc++; dst++;/* move hex word */
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
			*dst++ = *wsrc++; dst++;
		    }
		}
	    }
	    src = (u_char *)wsrc;		/* update the src pointer */
	}
    }

  /* Check the odd byte flag, if it is set move the last byte from user space
    to the RAM buffer using the local word variable. */

    if( oddmv )
    {
	tword = *dst;		/* read in the full 16 bits */
	tword &= 0xFF00;	/* clear out LSB */

	tword |= *src;		/* move the last byte */
	*dst = tword;		/* into the RAM buffer word */
    }
}

/* ---------------------------------------------------------------------- */

/* Move data from the RAM buffer to user space.  In the xfer loops the RAM
buffer pointer is incremented twice.  The 16 bit words in the RAM buffer are
aligned on 32 bit boundries.  The data is moved and then short pointer is
incremented again to the next word in the RAM buffer. */

int
rmbcopy(src, dst, len)
    register u_short *src;	/* pointer to the RAM buffer */
    register u_char *dst;	/* pointer to user data */
    register u_int len;		/* count of bytes to xfer */
{
    int oddmv;			/* flag for odd count in length */
    register u_short tword;	/* local copy for word xfers to RAM */
    register u_short *wdst;	/* for word aligned xfers */

    PRINTD(0xFF, 0x1, ("rmbcopy: s %x d %x l %d\n", src, dst, len ));

  /* Check for an odd count in len.  The byte will be moved at the bottom of
    the routine. */

    oddmv = (len & 0x1);	/* set the odd count flag is true */

    len >>= 1;			/* shift to word count */

    if( len != 0 )		/* was there only one byte to move ? */
    {
      /* If the dest address is odd the data is "unaligned".  The xfer
	will have to be done a word at a time.  The local register
	variable tword is used to get the word from the RAM buffer and
	xfer the bytes into user space, one at a time.  This
	allows a single access to the RAM buffer for the word read. */

	if( (u_int)dst & 0x01 )
	{
	    while(len-- != 0)			/* compare aginst 0 */
	    {
		tword = *src++; src++;		/* read word, incr to next */

		*dst++ = (u_char)tword;		/* xfer lsb */
		*dst++ = (u_char)(tword >> 8);	/* xfer msb */
	    }
	}
	else 		/* user data is word aligned */
	{
	    wdst = (u_short *)dst;		/* copy to word pointer */

	  /* The data xfer loops are unrolled.  The len variable is shifted
	    left to the next byte boundry and data is moved if necessary. 
	    The final xfer loop is a 16 byte while loop. */

	    if( len & 0x1 )			/* word check */
	    {
		*wdst++ = *src++; src++;	/* move word */
	    }

	    len >>= 1;				/* move to long word bnd */
	    if( len != 0 )			/* any more data */
	    {
		if( len & 0x1 )			/* long word check */
		{
		    *wdst++ = *src++; src++;	/* move long word */
		    *wdst++ = *src++; src++;
		}

		len >>= 1;			/* move to quad word bnd */
		if( len != 0 )			/* any more data */
		{
		    if( len & 0x1 )		/* quad word check */
		    {
			*wdst++ = *src++; src++;/* move quad word */
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
		    }

		    len >>= 1;			/* move to hex word bnd */
		    while( len-- != 0 )		/* loop till out of data */
		    {
			*wdst++ = *src++; src++;/* move hex word */
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
			*wdst++ = *src++; src++;
		    }
		}
	    }
	    dst = (u_char *)wdst;		/* update the dst pointer */
	}
    }

  /* Check the odd byte flag, if it is set move the last byte from user space
    to the RAM buffer using the local word variable. */

    if( oddmv )
    {
	tword = *src;			/* move the last byte */
	*dst = (u_char)tword;		/* from the RAM buffer word */
    }
}

/* ---------------------------------------------------------------------- */
/* Zero out a region in the RAM buffer.  NOTE: this routine will not bother
to check for odd counts. */

int
wmbzero(dst, len)
	register volatile u_short *dst;
	register u_int len;
{
    PRINTD(0xFF, 0x1, ("wmbzero: d %x l %d\n", dst, len ));

    len >>= 1;				/* shift to word counts */
    while(len-- != 0)
    {
	*dst++ = 0; dst++;
    }
}

get_validbuf(sc, targid)
struct sz_softc *sc;
int targid;
{
	if(sc->sc_dboff_busy[targid][0] == 1)
		if(sc->sc_dboff_busy[targid][1] == 1)
			return(-1);
		else
			return(1);
	else
		return(0);
}

sii_dodev(sc, targid, devtype)
struct sz_softc *sc;
{
	struct scsi_devtab *sdp = scsi_devtab;
	struct sz_inq_dt *idp = (struct sz_inq_dt *)&sc->sz_dat[targid];


	PRINTD(targid, 0x1, ("sii_dodev: checking on %x\n", devtype));
	while(sdp) {
		if((sdp->name && (strncmp(sc->sc_devnam[targid], 
		    sdp->name, sdp->namelen) == 0)) || 

/* Sorry hack for sorry dec tape drive which is not SCSI compliant */
		    ((devtype == SZ_TAPE) && (idp->devtq == sdp->tapetype)) ||

		    (sdp->name && (strcmp("UNKNOWN", sdp->name) == 0))) {
			bcopy(sdp->sysname, sc->sc_device[targid],
			    strlen(sdp->sysname));
			sc->sc_devtab[targid] = sdp;	/* save devtab ptr */
			sc->sc_devtyp[targid] = sdp->devtype;
			sc->sc_dstp[targid] = sdp->disksize;
			if(sdp->flags & SCSI_TRYSYNC)
				sc->sc_siisentsync[targid] = 0;
			else
	    	    		sc->sc_siisentsync[targid] = 1;
			if(sdp->flags & SCSI_REQSNS) {
				sc->sc_curcmd[targid] = SZ_RQSNS;
				sz_bldpkt(sc, targid, SZ_RQSNS, 0, 1);
				sii_scsistart(sc, targid, 0);
			}
			if(sdp->flags & SCSI_STARTUNIT) {
				int starttried = 0;
				sc->sc_curcmd[targid] = SZ_TUR;
				sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
				sii_scsistart(sc, targid, 0);
				if(sc->sc_status[targid] != SZ_SUCCESS && !starttried) {
					sc->sc_curcmd[targid] = SZ_P_SSUNIT;
					sz_bldpkt(sc, targid, SZ_P_SSUNIT, 0, 1);
					sii_scsistart(sc, targid, 0);
					starttried++;
				}
				do{
				DELAY(1000000);
				sc->sc_curcmd[targid] = SZ_TUR;
				sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
				sii_scsistart(sc, targid, 0);
				} while(sc->sc_status[targid] != SZ_SUCCESS && starttried++ < 30);
				sc->sc_curcmd[targid] = SZ_RDCAP;
				sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
				sii_scsistart(sc, targid, 0);
			}
			if(sdp->flags & SCSI_TESTUNITREADY) {
				sc->sc_curcmd[targid] = SZ_TUR;
				sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
				sii_scsistart(sc, targid, 0);
			}
			if(sdp->flags & SCSI_READCAPACITY) {
				sc->sc_curcmd[targid] = SZ_RDCAP;
				sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
				sii_scsistart(sc, targid, 0);
			}
			if(sdp->probedelay > 0)
				DELAY(sdp->probedelay)

			if(sdp->flags & SCSI_NODIAG)
				sz_unit_rcvdiag[targid] = 1;

			break;
		}
		sdp++;
	}
}


/******************************************************************
 *
 * Name:	sii_busy_target
 *
 * Abstract:	Allow a target to retry a SCSI operations after 
 *		it indicated that it was busy.
 *
 * Inputs:
 * unit		The ULTRIX logical unit number of the scsi device.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/
sii_busy_target(unit)
int unit;
{
    int cntlr = (unit >> 3) & 1;
    register struct sz_softc *sc = &sz_softc[cntlr];
    register struct sii_regs *siiaddr = (struct sii_regs *)sc->sc_scsiaddr;
    int targid, s;
    struct buf *dp, *bp;

    targid = sc->sc_unit[ unit ];
    dp = (struct buf *)&szutab[unit];

  /* Check and make sure that the target is in the busy state.  For now report
    the error and continue.  In theory the only way to here is via the status
    in routine. */

    if( !(sc->sc_szflags[targid] & SZ_BUSYTARG) )
    {
	/* NOTE: debug, do not call scsi_logerr() */
	mprintf( "sii_busy_target: BUSY flag not set on %d\n", targid );
    }
    sc->sc_szflags[targid] &= ~SZ_BUSYTARG;	/* clear the flag */

    /* Clear all associated states and flags for this target */

    sc->sc_xstate[targid] = SZ_NEXT;
    sc->sc_xevent[targid] = SZ_BEGIN;
    sc->sc_szflags[targid] = SZ_NORMAL;
    sc->sc_flags[targid] &= ~DEV_HARDERR;
    sc->sc_selstat[targid] = SZ_IDLE;

    PRINTD(targid, 0x24, 
	("sii_busy_target: requeueing scsi target %d after BUSY status\n",
		targid));

    /* If the SCSI bus is not busy then restart this target, the b_active
    flag is cleared.  The start routine will be able to act on this queue. */

    s = splbio();
    dp->b_active = 0;			/* set to non-active */
    if(sc->sc_active == 0)
	sz_start(sc, targid);
    splx(s);
}

