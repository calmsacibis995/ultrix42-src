#ifndef lint
static char *sccsid = "@(#)scsi_asc.c	4.11      (ULTRIX)  2/26/91";
#endif lint

extern dumphex();
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * scsi_asc.c
 *
 * 3MAX SCSI device driver (ASC routines)		
 *
 * 01/29/91	Robin Miller
 *   o	Setup address/count for various CD-ROM commands which require
 *	data in/data out.
 *   o	Added support for disk send/receive diagnostic functions.
 *
 * 12/05/90	Robin Miller
 *   o	Added support for 10-byte Read/Write CDB's.
 *   o	Use function sz_cdb_length() to obtain the Command Descriptor
 *	Block (CDB) length.
 *
 * 11/27/90     Maria Vella             v00.00.37
 *	Removed two calls to DELAY() in asc_phase_change() which were
 *	there for debugging purposes and should have been removed. 
 *	They caused the loss of 1/10th of a second of time on the
 *	system clock.  Fixes QAR #06476.
 *
 * 10/07/90     Maria Vella             v00.00.36
 *
 *	a.  Major modifications to add PDMA support.
 *	b.  Fixed QAR having to do with setting sz_unit_rcvdiag[] in
 *          ascprobe().  Was being indexed by targid.
 *     
 * 08/30/90	Robin Miller		v00.00.35
 *	Added check in probe routine to recognize RX26 device type
 *	and setup accordingly for floppy device.
 *
 * 08/24/90     Maria Vella             v00.00.34
 *      a.  In asc_state_change(), added check for selection completed
 *          but no command phase asserted by target due to the target
 *          being busy.  Problem shows up when RDAT is attached and
 *          doing its diagnostics while booting.  This must have
 *          fallen through the cracks during the past few modofications.
 *
 * 08/10/90     Maria Vella             v00.00.33
 *     
 *      a.  In asc_state_change(), added check for all 4 states in
 *          which a Request Sense command failed selection due to a
 *          reselection from another device.

 * 07/14/90	mitch mcconnell		v00.00.32
 *	a.  Newest attempt to fix RDAT problem.  When a select failed due
 *	    to being reselected, the code essentially restarted the entire
 *	    command by setting state/event back to begin/cont.  If the 
 *	    command which failed was a RQSNS, the "restart" would cause
 *	    the drive to send the next data record, and the RQSNS info
 *	    was lost.  The fix is to use the pxstate field in the softc
 *	    (as the  Vax code does) to store the state to return to in
 *	    this situation.
 *	b.  Changed retry states to RETRY_CMD and RETRY_SNS. 
 *
 *
 * 07/14/90	mitch mcconnell		v00.00.31
 *	a.  Reset chip values for req/ack offset as soon as they are
 *	    renegotiated. (on hold...)
 *	b.  Removed poll mode for RQSNS.  Now will be handled in state
 *	    machine in scsi.c.  Fix for RDAT reselect timeout problem.
 *
 * 06/06/90	Bill Dallas
 *	In asc_startdma() got rid of hard define of SZ_RQSNS_LEN.
 *	Used scsi command packet for the lenght.
 *
 * 03/14/90	John A. Gallant
 *	Added checks within the data moving code to not go past the true
 *	end of the user's buffer for reads and writes.  Added zero fill
 *	for writes less than a sector size.
 *
 * 03/01/90     Janet Schank
 *      Added retries on busy target in the probe routine.
 *
 * 01/18/90	Mitchell McConnell	v00.00.30
 *  a.	Fixed bug in asc_scsistart for "INTP not set", where printing
 *	SCSI cdb was not incrementing a pointer.  (Only if SZDEBUG on).
 *  b.	Recompiled everything to make sure SZDEBUG off works.
 *
 * 12/22/89	Mitchell McConnell	v00.00.29
 *  a.	Checkpoint version. 
 *  b.	Added more debug tools for rz56 problem.
 *  c.	Re-compile with temporary rz56 "fix" - set req/ack offset to 7.
 *
 * 12/20/89	Mitchell McConnell	v00.00.28
 *  a.	Made the cnf1,2, & 3 registers variables.  Now set cnf3 to
 *	use the 8 byte threshhold for data phase transfers.  
 *  b.	Now set up sync offset and sync per registers even for
 *	Select W/O ATN, since this mode is used for the Request
 *	Sense after a Check Condition.
 *  c.	Fixed bug in asc_busy_target - was hard-coded for a single
 *	controller.
 *
 * 12/14/89	Mitchell McConnell	v00.00.27
 *  o 	Made the sync. xfer per. and req/ack variables which can be
 *	set via xkvar.
 *  o	Removed asc_was_reset as redundant.
 *  o	In asc_reset_disc..., added test for sc_attached after checking
 *	sc_alive.
 *  o	In asc_dumpregs, use the stored version of sr,isr, and ssr 
 *	registers, since re-reading the chip may give misleading results.
 *
 * 12/11/89	Mitchell McConnell	v00.00.26
 *  o	Moved call to asc_reset_discon_io_tasks to asc_reset() as it
 *	is in PMAX.  Because of this, we were not recovering properly
 *	from parity errors.
 *
 * 12/08/89	Mitchell McConnell	v00.00.25
 *  o	Changed parameter for get_sys_ptes from hard-coded 130
 *	to btoc(64k) + 2.          (per Fred Canter).
 *  o	Now use sc_oddbyte as a flag sc_no_disconnects to allow a
 *	particular target to run without disconnecting.
 *
 * 12/06/89	Mitchell McConnell	v00.00.24
 *  o	Statistics were not getting updated because the 53C94 select &
 *	transfer command bypassed asc_sendcmd.  Now collect them in
 *	ascintr (yecchh!) when we know the select & transfer completed
 *	succesfully.
 *
 * 12/01/89	Mitchell McConnell	v00.00.23
 *  o	Another attempt to fix RQSNS problem.  While attempting to select
 *	a target for the RQSNS, we were reselected by a different target.
 *	This made things go quite haywire.  Fix is to not reenable selection
 *	for the 53C94 chip when we get a status of CHECK CONDITION.
 *
 * 11/29/89	Mitchell McConnell	v00.00.22
 *  o	Hack fix for state machine problem with RQSNS - now we will treat
 *	sc_actcmd as a polled command and not return until finished.  See
 *	additional comments in asc_scsistart.
 *
 * 11/08/89	Mitchell McConnell	v00.00.21
 *  o	Checkpoint version.
 *  o   Added ifdef'ed test statements for error logging.
 *
 * 11/03/89	Mitchell McConnell	v00.00.20
 *  o	Merged with ISIS pool.
 *
 * 10/31/89	Mitchell McConnell	v00.00.19
 *  o	a. Add controller to some PRINTD's to debug multiple controllers.
 *  o   Added #defines to compensate for hard-coded references to sii 
 *	in softc, e.g., sc_ascnum is really sc_siinum, (the controller
 *	number).
 *  o	Replaced large amount of asc_probe to be consistent with vax/pmax.
 *	No longer call routine asc_dodev.
 *  o	Added call to get_scsiid for host adapter id.
 *
 * 10/25/89	Mitchell McConnell	v00.00.18
 *  o	a. Fixed bug in asc_data_intr.  Only call restartdma if the
 *	   current phase is still DATAI/DATAO, else asc_phase_change
 *	   will handle it.
 *	b. Adjust asc_dboff_len in asc_data_intr.  It used to be done
 *	   in asc_msgin when SDP was received.
 *	c. Put in print_sense routine as a PRINTD for SCSID_CMD_EXP.
 *
 * 10/23/89	Mitchell McConnell	v00.00.17
 *  o	a. Checkpoint.  Fixed several bugs working on new DMA code.
 *  o	b. Fixed mixup between fstate values (SZ_DATAO_PHA, etc.) and
 *	   SCSI phase (SCS_DATAO).  
 *  o	c. Removed calls to asc_getactive_target.  Made inline code for
 *	   ascintr, elsewhere it is passed as an argument.
 *  o	d. Moved get of savcnt from SDP to MESSI.  By the time we know
 *	   we have SDP, the TC is always zero, since we use DMA for 
 *	   message bytes.
 *  o	e. Moved setting of DMA_DISC flag to SDP message.  PMAX driver 
 *	   makes assumption that DISC will always follow SDP, AND polls
 *	   for it anyway.  Because we handle each message byte individually,
 *	   the test for prevpha == DATAI/DATAO does not work.
 *	   
 * 10/20/89	Mitchell McConnell	v00.00.16
 *  o	a. Fairly radical changes to ascintr DMA handling to more exactly
 *	imitate PMAX SII.
 *  o	b. Make test for phase mismatch (BS || FC) && (PHASE(sr) != fstate).
 *  o	c. Moved data phase handling to new routine asc_data_intr().
 *
 * 10/19/89	Mitchell McConnell	v00.00.15
 *  o   a. Checkpoint.  Flush FIFO fix for 14 (what alliteration!) appears
 *	to have worked.
 *  o	b. In ascintr, try and simultate the SII_MIS (phase mismatch) code.
 *	Instead of BS || FC, now test for BS && (current phase != fstate).
 *
 * 10/17/89	Mitchell McConnell	v00.00.14
 *  o	Checkpoint version.  Both dskx and mtx running simultaneously.
 *  o	Added support to print sense information for debugging.
 *  o	Removed flush fifo at data out phase.
 *
 * 10/17/89	Mitchell McConnell	v00.00.13
 *  o	Added check for !RSEL in state_change.  More printfs to find 
 *	out why reselect is not working.	
 *  o	Removed call to sz_start for reselect case.  Should get restarted
 *	when the reselecting target finishes and disconnects. 
 *  o	Check for scsi_bus_idle in scsistart.  If not idle, return
 *	BUSBUSY.  Removed setting scsi_bus_idle to 0 for BUSY and
 *	command complete cases in ascintr.
 *  o	When reselected, reset the synchronous offset and period registers
 *	for the appropriate values for that target. 
 *  
 * 10/13/89	Mitchell McConnell	v00.00.12
 *  o	Fixed bug that kept disconnect/reconnect from working! 
 *	Initialization sc did not occur because asc_nNASC was equal to
 *	4.  
 *
 * 10/11/89	Mitchell McConnell	v00.00.11
 *  o	Checkpoint version.  Putting printfs in start to find out why
 *	bpcount = 0 when starting DMA.
 *  o	Fixed asc_state_change to recognize reselection during selection.
 *	Must reset state of target which was selecting to Bus Busy so it
 *	will retry later.  Also, must set sc_active to the new target id!
 *
 * 10/09/89	Mitchell McConnell	v00.00.10
 *  o	Fixed bug for synchronous data transfer.  NEVER issue Flush
 *	FIFO when the phase = sync. data in.
 *  o	Fixed bug in ascintr- when checking for state change, used
 *	wrong mnemonic for testing the RESELECT bit in the isr.
 *
 * 10/03/89	Mitchell McConnell	v00.00.09
 *  o	Several misc. bug fixes, esp. invalid use of GETCNTR macro
 *	in asc_ext_messg.
 *  o	Added call to prom_getenv for scsiid of controller.
 *  o 	Began adding support for scsi_logerr (with ifdefs) in
 *	place of mprintf's.  Before release, the mprintfs will
 *	go away, leaving only the error logging. Changed calls
 *	to asc_dumpregs to use 'who' parameter, since scsi_logerr
 *	also calls it.
 *
 * 09/28/89	Mitchell McConnell	v00.00.08
 *  o	Newest attempt at handling extended messages.  Initially,
 *	always DMA one message byte.  If it is an extended message,
 *	set the EXTMESSG flag in szflags.  Issue the MSGACPT
 *	command and return.  If the last command was MSGACPT,
 *	issue a DMA XINFO command to get the length byte.
 *      At the next interrupt, if the EXTMESSG flag is already
 *	set and the last command was DMA XINFO, save the length
 *	byte and issue MSGACPT.  If the last command was MSGACPT,
 *	issue a DMA XINFO for the remaining bytes of the extended
 *	message.  The next interrupt should be FC and (msg in?).
 *	Check the Transfer Count register.  If > 1, copy the
 *	remaining extended message bytes and go process the 
 *	message.  Issue MSGACPT to release ACK.  The next (and
 *	final) interrupt should be BS with phase COMMAND.
 *
 * 09/26/89	Mitchell McConnell	v00.00.07
 *  o	Control version - first built under new "isis-sys" pool.
 *  o   Fixed bug in ascintr - used ASC_XINFO instead of 
 *	(ASC_DMA | ASC_XINFO).
 *
 * 09/21/89	Mitchell McConnell	v00.00.06
 *
 *  o	First version to support reads and writes.  Attempt to keep
 *	some rudimentary form of version control until placed under
 *	SCCS.
 *
 *  o	Removed the 'attn' flag from asc_senddata, since it was not
 *	being used, and anyway, the ASC will assert ATN when sending
 *	bytes to the target during message out phase.
 *
 *  0	Give some more intelligence to asc_select_target to know 
 *	whether to issue the SELECT with or without ATN.  For now,
 *	just use the asc_sentsync (sp?) as a flag.  This assumes that
 *	the ONLY time we will need to do this is after a reset to
 *	(re)set the synchronous parameters.
 *   
 *  o	Try using DMA to send command
 ***********************************************************************/

#define SZDEBUG

char  asc_version[] = "ASC Version 00.00.32b";

#define ASC_LOGERR	1      
#define ASC_TEST_LOG	1
#define ASCDEBUG	1 

/************************************************************************/

/* WARNING! If the softc EVER changes, these defines need to be re-examined.*/

#define	sc_ascnum		sc_siinum
#define sc_ascsentsync		sc_siisentsync
#define sc_ascdmacount		sc_siidmacount
#define sc_ascreqack		sc_siireqack
#define sc_ascdboff		sc_siidboff
#define enable_sel_rsel 	use_programmed_io 
#define sc_no_disconnects 	sc_oddbyte

#include "../data/scsi_data.c"
#include "scsi_debug.h"
#include "../../tc/tc.h"
#include "pdma3min.h"

#define MAXASC nNASC

extern char szbufmap[];
extern int scsidebug;
extern short sz_timetable[];
extern int cpu;

int asc_wait_after_inquiry = 1000;

int asc_sync_xfer_per = ASC_SYNC_XFER_PER;
int asc_sync_xfer_reg = ASC_SYNC_XFER_REG;
char asc_def_cnf1 = ASC_C1_PARITY;
char asc_def_cnf2 = 0;
char asc_def_cnf3 = 0; 

/*
 * Autoconfiguration information
 */
extern  u_short szstd[];
int     szslave(), szattach(), sz_start(), szerror(), ascprobe();
struct	uba_driver ascdriver = { ascprobe, szslave, szattach, sz_start,
				 szstd, "rz", szdinfo, "asc", szminfo,
				 0 };

int asc_busy_target();			/* for forward reference */

short asc_wait_count = ASC_WAIT_COUNT;	/* Delay count for ASC chip */
short asc_reject_message = 0;		/* Message Reject Flag      */
short asc_assert_attn = 0;		/* Assert Attention Flag    */
short asc_nNASC = NASC;			/* Number of ASC controllers*/
short asc_firstcall = 1;		/* First call to probe flag */

#if ASC_TEST_LOG
short	asc_tested_log = 1;
#endif ASC_TEST_LOG

/*
 * Number of seconds to wait for SCSI devices
 * to come ready enough to respond to inquiry
 * after bus reset. Needs to be a variable so
 * users can change it (I am such a nice guy).
 * Factory default is 7 seconds (in scsi_data.c).
 */

extern int sz_wait_for_devices;
extern int sz_max_wait_for_devices;

extern int asc_scsistart();
extern int asc_reset();
extern int tc_addr_to_name();

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

/******************************************************************
 *
 * Name:	ascprobe
 *
 * Abstract:	Probe for any SCSI devices on this controller.
 *		Initialize any fields in the softc structure.  For
 *		each possible target:
 *	    	    o	build SCSI Inquiry command
 *		    o	call asc_scsistart()
 *		    o	if command successful,
 *			    initialize target-specific data structures
 *			    allocate PTEs for buffer mapping
 *			    call szattach()
 *			    set up buffer for non-DMA commands
 *
 *		Clear any pending interrupts for this controller.
 *
 * Inputs:
 *
 * 	cntrl	- the controller number for this bus.
 *
 * Outputs:	None.
 *
 * Return values: None.
 *
 ******************************************************************/

/* rpbfix: fake out the config process */
extern int cpu;

ascprobe(vbaddr, um) 
volatile char *vbaddr;		/* base address for this option slot */
register struct uba_ctlr *um;
{
	ASC_REG *ascaddr;	/* pointer for the ASC registers */
	int targid, unit;
	int dboff;
	int rz_slotsize, tz_slotsize, cz_slotsize;
	int i, s, stat;
	struct sz_inq_dt *idp;
	struct sz_rdcap_dt *rdp;
	struct scsi_devtab *sdp;
	struct scsi_devtab *usdp;
	int sdp_match;
	char *p;
	char *env;
	int alive;
	int retries, status;
	int cntlr = um->um_ctlr;
	register struct sz_softc *sc = &sz_softc[cntlr];
        char modname[TC_ROMNAMLEN+1];
        int opt_type=0;

	s = splbio();

	alive = 1; 		/* On 3MAX asc is always present */
        sc->ioasicp = 0;

	if ((cpu == DS_5000_100) && (cntlr == 0))  
            {
            sc->ioasicp =  (char *) PHYS_TO_K1( BASE_IOASIC );
            }

	/*
	 * Initialize certain fields in the softc structure
	 * and reset the ASC chip.
	 */

	if (alive) {
	    sc->sc_ascnum = cntlr;		/* save a pointer to the asc */
	    sc->sc_slotvaddr = vbaddr;		/* init the base address */
	    sc->sc_sysid = get_scsiid(um->um_ctlr);

	    sc->sc_active = 0;			/* init the active flag */
	    sc->port_start = asc_scsistart;	/* init the port_start switch */
	    sc->port_reset = asc_reset;		/* init the port_reset switch */
	    sc->sc_scsiaddr = (caddr_t) vbaddr; /* Save the cntr address */
	    sc->sc_rambuff = ASC_BUF_ADDR;	/* init the RAM buffer pointer*/

#ifdef SZDEBUG
	    if (asc_firstcall)
		printf("%s\n", asc_version);
#endif SZDEBUG


	  /* Attach the dma subsystem.  The cpu value is passed to indicate
	    what type of system the PDMA code needs to setup for.  This call
	    needs to be made before any target data xfers take place. */

	    PRINTD( 0xFF, 0x8000,
		("asc_probe: calling PDMA attach cpu %d\n", cpu ));

            if( tc_addr_to_name(sc->sc_slotvaddr, modname) == -1)
                cprintf("tc_addr_to_name failed ");

            if( !strcmp(modname, "PMAZ-AA ") )  {
                opt_type = DS_5000;
            }
            else
                if( !strcmp(modname, "PMAZ-AB ") )  {
                    opt_type = DS_5000_100;
                }
            else
                opt_type = cpu;

	    if( pdma_attach( sc, opt_type ) != PDMA_SUCCESS ) {
                cprintf("asc_probe: pdma_attach( %x, %x ) failed\n", sc, opt_type );
		alive = 0;
	    }
	    else {
	        asc_reset(sc);			/* reset the ASC chip */
	        ascaddr = ASC_REG_ADDR;		/* set the register pointer */
	    }

	}	/* end if (alive) */

	/*
	 * Use the inquiry command to determine number
	 * and type of targets on this controller.
	 * If this controller does not exist alive will
	 * be zero and the for loop won't do anything.
	 */

	for (targid = 0; targid < NDPS; targid++) { 

		if (alive == 0)
			break;

		sc->scsi_bus_idle = 1;

		sc->sc_active = (1 << targid);		/* signal active */
		sc->sc_selstat[targid] = SZ_IDLE;
		sc->sc_ascsentsync[targid] = 0;
		sc->sc_ascreqack[targid] = 0;
		sc->sc_attached[targid] = 0;		
		sc->sc_ascsyncper[targid] = 0;
		sc->sc_no_disconnects[targid] = 0;
		sc->sc_rzspecial[targid] = 0;
		sc->sc_rmv_media &= ~(1 << targid);

		if (targid == sc->sc_sysid)
			continue;	/* skip initiator */

		retries = 5;
		status = 1;
		i = sz_wait_for_devices;
		while (retries)
		{
		  /* Clear where the inquiry data will be going.  This is like
		    putting the cart before the horse, the offsets have not 
		    been setup yet.  However, scsistart() calls recvdata() and
		    recvdata() uses the values in ascdboff[] in transfering the
		    data.  This causes all transfers to occur in the first 
		    ram buffer "page".  And valid inquiry data from the previou
		    target is still there.  This can cause some interesting
		    device types. */

/*RPS --- This stuff is non HBA specific.  The sc_rambuff entry in the
  softc structure isn't used by the 3min so this may cause a problem. */
                    if ( !sc->ioasicp )
                        bzero((char *)(sc->sc_rambuff + sc->sc_ascdboff[targid]),
               	            DBOFF_LEN);

		    sc->sc_szflags[targid] = SZ_NORMAL; 
		    sc->sc_curcmd[targid] = SZ_INQ;
	    	    sc->sc_ascsentsync[targid] = 1;

		    sz_bldpkt(sc, targid, SZ_INQ, 0, 0);

		    stat = asc_scsistart(sc, targid, 0);

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
		    }			/* end if stat == SUCCESS */
		    else 
			if (stat == SZ_RET_ABORT) {
			    PRINTD(targid, SCSID_ERRORS,
			       ("asc_probe: stat == RET_ABORT\n"));
			    status = 1;
			    break;
			}
		    /* end else stat == RET_ABORT */

		    DELAY(1000);	/* JAG different # ? */
		    retries--;
		    continue;

		}	/* end while */

		if (status != 0)
		    continue;

		/*
		 * Initialize data structures for this target and
		 * save all pertinent inquiry data (device type, etc.).
		 */

		idp = (struct sz_inq_dt *)&sc->sz_dat[targid];

		/* Save removable media bit for each target */

		if (idp->rmb)
		    sc->sc_rmv_media |= (1 << targid);

		PRINTD(targid, SCSID_CMD_EXP,
		   ("\n",asc_print_inq_info(idp)));

		/*
		 * Zero device name and revsion level
		 * ASCII strings, so we know whether or
		 * not they were loaded by the INQUIRY.
		 */

		for (i = 0; i < SZ_DNSIZE; i++)
		    sc->sc_devnam[targid][i] = (char)0;

		for (i = 0; i < SZ_REV_LEN; i++)
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

		PRINTD(targid, SCSID_FLOW,
		  ("asc_probe: targid %d perfdt=0x%x\n", targid, idp->perfdt));

	    switch(idp->perfdt) {
	    default:		/* Unknown device type */
		printf("asc_probe: scsi %d targetID %d: %s (%d).\n",
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
		 * likely stall, spin, crash, burn!  Account for 
		 * max transfer size (64k) plus 2 guard pages.
		 */

		i = get_sys_ptes(btoc(ASC_MAX_XFER) + 2, 
				 &sc->sc_szbufmap[targid]);

		if (i == 0) {
		    printf("asc_probe: scsi %d targetID %d: %s\n",
			um->um_ctlr, targid, "cannot get %d PTEs for bufmap",
			   (btoc(ASC_MAX_XFER) + 2)  );
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

			    }	/* end if strncmp(devnam, sdp->name) */

		    }	/* end if (sdp->name) */

		}	/* end for */

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

		}	/* end if !match */

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
		    sc->sc_ascsentsync[targid] = 0;
		else
		    sc->sc_ascsentsync[targid] = 1;

		if (sdp->flags & SCSI_REQSNS) {
		    sc->sc_curcmd[targid] = SZ_RQSNS;
		    sz_bldpkt(sc, targid, SZ_RQSNS, 0, 1);
		    asc_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_STARTUNIT) {
		    /*
		     * Send two start unit commands because a pending unit
		     * attention may cause the first one to fail. We don't
		     * for the drive to spin up here (happens in rzopen).
		     */
		    sc->sc_curcmd[targid] = SZ_P_SSUNIT;
		    sz_bldpkt(sc, targid, SZ_P_SSUNIT, 0, 1);
		    asc_scsistart(sc, targid, 0);
		    asc_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_TESTUNITREADY) {
		    sc->sc_curcmd[targid] = SZ_TUR;
		    sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
		    asc_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_READCAPACITY) {
		    sc->sc_curcmd[targid] = SZ_RDCAP;
		    sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
		    asc_scsistart(sc, targid, 0);
		}
		if (sdp->probedelay > 0) {
		    DELAY(sdp->probedelay);
		}

		if (sdp->flags & SCSI_NODIAG)
		    sz_unit_rcvdiag[(cntlr * NDPS) + targid] = 1;

		break;
	    }		/* end of switch */
	    /*
	     * Just to be sure the bus is free after inquiry.
	     * RRD40 may hold bus for a while.
	     */

	    DELAY(asc_wait_after_inquiry);

	}		/* end of for loop */


	if (alive) {
	    stat = ascaddr->asc_intr;	/* read clears INTP */
	    wbflush();

	}	/* end if alive */

	DELAY(10000);

	sc->sc_active = 0;			/* clear active */

	dboff = 0x0;

	/*
         * Setup ram buffer slots for each target to
	 * be used for non READ/WRITE DMA Transfers on the ASC.
	 */

	for (i = 0; i < NDPS; i++) {
	    sc->sc_ascdboff[i] = dboff;
	    dboff += DBOFF_LEN;
	}	/* end for */

	dboff = TARGBUF_START;

	/* determine rz slot size, must be > 16kb */

	cz_slotsize = tz_slotsize = rz_slotsize = TARGBUF_LEN;

	for (targid = 0; targid < NDPS; targid++) {

	    if (targid == sc->sc_sysid)
		continue;

	    if (sc->sc_alive[targid] == 0)
		continue;

	    sc->sc_dboff[targid][0] = dboff;
	    sc->sc_dboff[targid][1] = dboff + ASC_DMA_XLEN;
	    sc->sc_segcnt[targid] = ASC_MAX_XFER;

	    dboff += rz_slotsize;

	    PRINTD(targid, SCSID_CMD_EXP,
		("asc_probe: cntlr=%d targid=%d devtype=0x%x ", cntlr,
		targid, sc->sc_devtyp[targid]));

	    PRINTD(targid, SCSID_CMD_EXP,
	      ("req/ack=%d slotsize=%d\n", sc->sc_ascreqack[targid], 
	           sc->sc_segcnt[targid]));

	}	/* end for targ=0, etc. */

	asc_firstcall = 0;

	splx(s);

	return(alive);

}	/* end asc_probe */



/******************************************************************
 *
 * Name:	asc_scsistart
 *
 * Abstract:	Start a SCSI operation on the 53c94 controller.
 *
 *		o	Get register address from softc
 *		o	Load cdb into FIFO.
 *		o	Issue Select With ATN command.
 *		o	If not polled mode, return SZ_IP to upper state machine.
 *		o	If polled mode, spin wait for interrupt, then
 *			call ascintr() to process.
 *
 * Inputs:
 *
 * 	sc	- a pointer to the controller data structure
 *	targid  - the target id
 *	bp	- the buf pointer for this operation
 *	
 *
 * Outputs:		None.
 *
 * Return values: 	SZ_IP		- command in progress
 *			SZ_SUCCESS	- command completed successfully
 *			SZ_RET_ERR	- command completed with an error
 *			SZ_RET_ABORT	- the cmd was aborted due to an error
 *
 ******************************************************************/

asc_scsistart(sc, targid, bp)

register struct sz_softc *sc;
int targid;
register struct buf *bp;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR;	/* setup the register pointer */
    int retval;
    int timer;
    int phase;
    u_char stat;
    int	s;
    int	save_flags;
    int	save_xfercnt;
    int	save_bpcount;
    int	save_b_bcount;
    int	save_resid;
    int save_dmaxfer;

#if ASC_TEST_LOG
    if (!asc_tested_log)
	asc_test_log(sc, targid);
#endif ASC_TEST_LOG

    /*
     * If "bp" is "0" we use polled scsi mode, disallow reselect
     * attempts, and disable interrupts for all transfers. We 
     * poll for completion instead of allowing interrupts.
     */

    PRINTD(targid, SCSID_ENTER_EXIT,
       ("\nasc_scsistart: STARTING cmd = 0x%x\n", sc->sz_opcode));

    /* Fix for select/reselect problem */

    if (!sc->scsi_bus_idle) {

	if ((sc->sc_devtyp[targid] & SZ_TAPE) && 
	    (sc->sc_curcmd[targid] == SZ_RQSNS)) {
	    PRINTD(targid, 0x8000,	
	       ("asc_scsistart: bus !idle for RQSNS!\n"));
	}

	PRINTD(targid, SCSID_DISCONNECT,
	   ("asc_scsistart: targ %x, bus not idle, returning BUSY\n",
	       targid));

	return(SZ_BUSBUSY);

    }	/* end if !idle */

    if (bp == (struct buf *)0) {
	sc->scsi_polled_mode = 1;
    }
    else {
        sc->scsi_polled_mode = 0;
    }

    /* Start a SCSI command on the passed target. */

    sc->scsi_completed[targid] = 0;
    sc->scsi_bus_idle = 0;
    sc->sc_dmaxfer [targid] = 0;
    sc->sc_messgptr [targid] = &sc->sc_message [targid];

    /* Check if we need to send a Synchronous DataXfer Message.  At this time
      assume all commands will go via the manual process.  Create a sel/cmd
      routine to do the select/ATN/CDB command. */

    /* Perform target arbitration and selection */

    if ((retval = asc_select_target(sc, targid)) != SZ_SUCCESS) {

#ifdef ASC_LOGERR
	/* if bp == 0, this is probe(), and it is not an error to timeout
	   then... */

	if (bp != 0) {	 
	    scsi_logerr(sc, 0, targid, SZ_ET_SELTIMO, 0, 0, SZ_HARDERR);
	}
#else ASC_LOGERR
	printf("asc_scsistart: bad rtn code from select...0x%x\n", retval);
#endif ASC_LOGERR

	return(retval);

    }	/* end if asc_select_target != SUCCESS */

    /* If in poll mode wait for an interrupt pending and then call the 
       interrupt handler to take care of the rest. */

    while (sc->scsi_polled_mode) {

	PRINTD(targid, SCSID_PHASE_STATE,
	    ("asc_scsistart: COMMAND IN PROGRESS poll mode\n"));

	SZWAIT_UNTIL((ascaddr->asc_stat & ASC_INTP),asc_wait_count,retval);
	if (retval >= asc_wait_count) {
	    int flags;
#ifdef ASCDEBUG
	    {
	    int i;
	    char *p;

	    asc_show_regs(sc, sc->sc_ascnum, targid);

	    p = (char*)&sc->sz_opcode;
	    cprintf("cdb: ");
	    for (i = 0; i < sz_cdb_length(sc->sz_opcode,targid); i++) {
		cprintf(" %x", *p++);
	    }
	    cprintf("\n");
	    }
#endif ASCDEBUG

#ifdef ASC_LOGERR
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x45, 0, flags);
#else ASC_LOGERR
	    printf("asc_scsistart: ASC_INTP not set in poll mode.\n");
#endif ASC_LOGERR
	    goto HANDLE_ABORT;

	}	/* end if retval > asc_wait_count */

	ascintr(cntlr);		/* let the intr handle clean up */

	PRINTD(targid, SCSID_ENTER_EXIT,
	       ("asc_scsistart: flags after interrupt 0x%x\n",
		sc->sc_szflags[targid]));

	/* If the target is still working on the last command w/polling, go
	   back and wait for the next interrupt to be serviced.    */

	if ( !sc->scsi_bus_idle ) {
	    PRINTD( targid, SCSID_ENTER_EXIT,
		("asc_scsistart: in poll, bus not idle loop again\n"));
	    continue;
	}	/* end if !scsi_bus_idle */


	/*
	 * Check the status of the SCSI operation. If the SCSI
	 * operation completed with a good status return success, otherwise
	 * indicate a problem.  If a timeout occured during selection then
	 * return abort.
	 */


	if (sc->sc_szflags[targid] & SZ_SELTIMEOUT) {
	    return(SZ_RET_ABORT);
	}	/* end if SELTIMEOUT */

	if (sc->sc_status[targid] == SZ_GOOD)
	    return(SZ_SUCCESS);	
	else
	    return(SZ_RET_ERR);	

    }	/* end while scsi_polled_mode */ 

    /* Return the "inprogress status" allow the interrupt handler to take
    care of the rest of the command etal. */

    return( SZ_IP );


HANDLE_ABORT:

    /* Abort the current SCSI operation due to error */

    PRINTD(targid, SCSID_ERRORS,
	("asc_scsistart: command aborted (bus=asc%d target=%d cmd=0x%x)\n",
	    cntlr, targid, sc->sc_curcmd[targid]));

    PRINTD(targid, SCSID_ERRORS, ("", asc_dumpregs(cntlr, WHO_ASC)));

    asc_reset(sc);

    sc->sc_selstat[targid] = SZ_IDLE;
    sc->sc_active = 0;

    return(SZ_RET_ABORT);

}	/* end asc_scsistart */


/******************************************************************
 *
 * Name:	asc_select_target
 *
 * Abstract:	Issue Select (With ATN) cmd to ASC after loading the 
 *		(ID byte and the) CDB into the ASC FIFO.  The ASC will 
 *		arbitrate for the bus, send the contents of the FIFO to 
 *		the target, then generate an interrupt with the Function 
 *		Complete bit and Bus Service bits set, and with the 
 *		phase equal to Data In/Data Out.
 *
 *		Note: we have to save the targid in sc_active here,
 *		because there is no other way at interrupt time to 
 *		"remember" who we selected.
 *
 *	TO DO:
 *
 *	If necessary to send sync, issue Select w atn and stop command.
 *	At the BS interrupt, (phase = msg out?) for each other msg byte,
 *	issue Set ATN command, write byte to FIFO, and issue non-dma(?)
 *	Xfer Info command.....?????
 *
 * Inputs:

 *
 * 	sc	- the softc data structure
 *  targid	- the target id
 *
 * Outputs:		None.
 *
 * Return values: None.  
 *
 ******************************************************************/

asc_select_target(sc, targid)

register struct sz_softc *sc;
int targid;
{
    int cntlr = sc->sc_ascnum;
    DMA_AR 	*ascar = ASC_AR_ADDR;
    ASC_REG 	*ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    int 	retval, i, ffr;
    u_long 	messg;
    int		cmdcnt = 0;
    int 	lun = sc->sz_t_read.lun;
    char	*cmdptr, *cmdout, cmdstr[20];
    u_char	command;

    /*
     * Begin the selection phase on the ASC chip with or without
     * disconnects. Setup the target Register and the 
     * Command Register on the ASC to select a target on the SCSI 
     * bus.
     */

    sc->sc_ascdmacount[targid] = 0;

    cmdout = &cmdstr[0];

    sc->sc_active = (1 << targid);	/* See Note, above 	*/

    ascaddr->asc_dbid = targid;		/* Set the dest. bus id 	*/
    wbflush();

    /* Set Select/Reselect Timeout to maximum */
    
    ascaddr->asc_srto = ASC_TIMEOUT;
    wbflush();

    if (sc->scsi_polled_mode || sc->sc_no_disconnects[targid]) {

        cmdcnt = sz_cdb_length (sc->sz_opcode, targid);

	bcopy((char*)&(sc->sz_command), &cmdstr[0], cmdcnt);

	ascaddr->asc_so = sc->sc_ascreqack[targid];
	wbflush();

	ascaddr->asc_sp = sc->sc_ascsyncper[targid];
	wbflush();

	command = ASC_SELECT;			/* 0x41 - No ATN!	*/

    }	/* end if polled_mode || no_disc */
    else
	{
	messg = SZ_ID_DIS | lun;  	/* Allow disconnects */

        PRINTD(targid, SCSID_ENTER_EXIT,
	   ("asc_sel_tar: selecting target %d messg 0x%x, sentsync = %x\n",
	       targid, messg, sc->sc_ascsentsync[targid]));

	/* If we haven't sent the sync message, send only 1 identify message
	   byte to the target.  This should generate a BS intr. with the 
	   phase = MESSAGE OUT.  */

	if (!(sc->sc_ascsentsync[targid])) {

	    PRINTD(targid, (SCSID_DISCONNECT | SCSID_FLOW),
	      ("asc_sel_tar: !sentsync, issuing SELATNSTOP cmd, msg = 0x%x\n",
	        messg));

	    /* Write only a single message byte to FIFO (Identify) */

	    cmdstr[0] = messg;
	    cmdcnt = 1;

	    ascaddr->asc_so = sc->sc_ascreqack[targid];
	    wbflush();

	    ascaddr->asc_sp = sc->sc_ascsyncper[targid];
	    wbflush();

	    command = ASC_SELATNSTOP;		/* 0x43 */

        }	/* end if !sentsync */
	else {

	    PRINTD(targid, SCSID_DISCONNECT,
	       ("asc_sel_tar: syncper= 0x%x, reqack = 0x%x\n",
		sc->sc_ascsyncper[targid],sc->sc_ascreqack[targid]));

	    /* Set up the sync. period and sync. offset registers	*/

	    ascaddr->asc_so = sc->sc_ascreqack[targid];
	    wbflush();

	    ascaddr->asc_sp = sc->sc_ascsyncper[targid];
	    wbflush();

	    cmdptr = (char*)&sc->sz_command;

	    *cmdout++ = messg;
	    cmdcnt++;

	    for (i = 0; i < sz_cdb_length(sc->sz_opcode,targid); i++) {
		*cmdout++ = *cmdptr++;
		cmdcnt++;

	    }	/* end for */

	    command = ASC_SELATN;		/* 0x42 */

        }	/* end else (sentsync == 1) */


    }	/* end else ! polled mode */

    PRINTD(targid, SCSID_CMD_EXP,
       ("asc_sel_tar: ASC cmd: 0x%x, cmdcnt = 0x%x, CDB: ", 
	   (command | ASC_DMA), cmdcnt));

    SZDEBUG_EXPAND(targid, &cmdstr[0], cmdcnt);

    PRINTD(targid, SCSID_CMD_EXP, ("\n"));

    /* Setup the FIFO for this target, and then start the transfer. */
    asc_FIFOsenddata (sc, command, &cmdstr[0], cmdcnt);

    return(SZ_SUCCESS);

}	/* end asc_select_target */



/******************************************************************
 *            
 * Name:	asc_startdma
 *
 * Abstract:	If a non-READ/WRITE command, set up the data count
 *		for the command.  Set up special softc fields (sort
 *		of a pseudo-"bp") used by DMA routines.  Either start
 *		or restart the DMA transfer as appropriate.  The ASC
 *		will generate a Terminal Count zero interrupt if the
 *		command completes normally.  The Bus Service bit should
 *		also be set in the ISR and the phase requested should
 *		be Status, unless we get disconnected, which is a whole
 *		other story to be told later....
 *
 * Novelized Version:
 *
 *	Various other variables and how they are used and set, both in this
 *	routine and in restartdma and ascintr. 
 *
 *	byteptr (local) - initialized address for all non-read/write 
 *	commands to place their data.  Points to sc->sz_dat[t].  HOWEVER,
 *	if rzspecial[t], then set to point to sc->sc_rzparams[t].
 *
 *	datacnt - initialized on a per-command basis for the expected length 
 *	of the transfer.
 *
 *	
 *
 * Inputs:
 *
 * 	sc	- the softc data structure
 *   iodir	- the direction of the DMA transfer
 *
 * Outputs:		None.
 *
 * Return values: None.
 *
 ******************************************************************/

asc_startdma(sc, iodir, targid)

register struct sz_softc *sc;
int 	iodir;
int	targid;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    u_char *byteptr;
    char *bufp;
    int datacnt, i;
    int retval, offset; 
    int dmacount;

    struct format_params *fp;
    struct reassign_params *rp;
    struct read_defect_params *rdp;
    struct defect_descriptors *dd;
    struct mode_sel_sns_params *msp;
    struct io_uxfer *iox;


    PRINTD(targid, SCSID_ENTER_EXIT,
       ("asc_startdma: enter, iodir = 0x%x\n", iodir));

    /*
     * Handle non READ/WRITE scsi commands that transfer data.
    */

    if ( (sc->sz_opcode == SZ_WRITE) || (sc->sz_opcode == SZ_READ) ||
         (sc->sz_opcode == SZ_WRITE_10) || (sc->sz_opcode == SZ_READ_10) ) {

	PRINTD(targid, SCSID_DMA_FLOW,
         ("asc_startdma: READ/WRITE COMMAND STARTING  ****************\n"));

    }	/* end if READ || WRITE */

    if ( (sc->sz_opcode != SZ_WRITE) && (sc->sz_opcode != SZ_READ) &&
         (sc->sz_opcode != SZ_WRITE_10) && (sc->sz_opcode != SZ_READ_10) ) {

	byteptr = (u_char *)&sc->sz_dat[targid];

	switch(sc->sz_opcode) {

	case SZ_MODSEL:

	    byteptr = (u_char *)&sc->sc_dat[0];
	    datacnt = (int) sc->sz_modsel.pll;

	    if(sc->sc_rzspecial[targid]) {
	        msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
                byteptr = (u_char *)sc->sc_rzaddr[targid];
                datacnt = msp->msp_length;
	    }

    	    PRINTD(targid, SCSID_CMD_EXP, 
		   ("\n\nasc_startdma: MODE SELECT data:"));

	    SZDEBUG_EXPAND(targid, byteptr, datacnt);  

    	    PRINTD(targid, SCSID_CMD_EXP, ("\n"));

	    break;	/* end case MODE_SELECT */

	case SZ_RQSNS:

	    byteptr = (u_char *)&sc->sc_sns[targid];
	    datacnt = sc->sz_rqsns.alclen;

	    break;	/* end case REQUEST SENSE */

	case SZ_INQ:

	    datacnt = SZ_INQ_MAXLEN;

	    if(sc->sc_rzspecial[targid])
                byteptr = (u_char *)sc->sc_rzaddr[targid];

	    break;	/* end case INQUIRY */

	case SZ_RDCAP:

	    datacnt = SZ_RDCAP_LEN;

	    break;	/* end case READ CAPACITY */

	case SZ_MODSNS:

	    datacnt = (int) sc->sz_modsns.alclen;

	    if(sc->sc_rzspecial[targid]) {
	        msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
                byteptr = (u_char *)sc->sc_rzaddr[targid];
                datacnt = msp->msp_length;
	    }

	    break;	/* end case MODE SENSE */

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
	    break;	/* end case RECEIVE DIAGNOSTICS */
	}
	case SZ_REASSIGN:

            rp = (struct reassign_params *)sc->sc_rzparams[targid];
            byteptr = (u_char *)sc->sc_rzparams[targid];
            datacnt = ((rp->rp_header.defect_len0 << 0) & 0x00ff) +
    		      ((rp->rp_header.defect_len1 << 8) & 0xff00) + 4;

            break;	/* end case REASSIGN BLOCK */

	case SZ_FORMAT:

            dd = (struct defect_descriptors *)sc->sc_rzaddr[targid];
            byteptr = (u_char *)sc->sc_rzaddr[targid];
            datacnt = ((dd->dd_header.fu_hdr.defect_len0 << 0) & 0x00ff) +
    		      ((dd->dd_header.fu_hdr.defect_len1 << 8) & 0xff00) + 4;

	    break;	/* end case FORMAT */

	case SZ_RDD:
            rdp = (struct read_defect_params *)sc->sc_rzparams[targid];
            byteptr = (u_char *)sc->sc_rzaddr[targid];
            datacnt = rdp->rdp_alclen;

            break;	/* end case RDD */

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
	    printf("asc_startdma: unknown scsi cmd 0x%x\n", sc->sz_opcode);
	    return(SZ_RET_ABORT);
	    break;
	}	/* end switch(opcode) */

	/*
         * Setup softc structure entries for special SCSI DISK
	 * commands that do dma. (FORMAT UNIT), (READ DEFECT DATA),
	 * (REASSIGN BLOCK), (MODE SELECT), (MODE SENSE) and
	 * (INQUIRY).  Q. Are these fields normally set by one of
	 * the upper layers for reads and writes???
	 */

	 if (!(sc->sc_szflags[targid] & SZ_DMA_DISCON)) {

	     PRINTD(targid, SCSID_DMA_FLOW,
		    ("asc_startdma: !SZ_DMA_DISCON\n"));

	     sc->sc_b_bcount[targid] = datacnt;
	     sc->sc_bpcount[targid] = datacnt;
	     sc->sc_bufp[targid] = (char *)byteptr;
	     sc->sc_xfercnt[targid] = 0;

	    }	/* end if !DMA_DISCON */ 
	else {
	    PRINTD(targid, SCSID_DMA_FLOW,
		("asc_startdma: ========> SZ_DMA_DISCON set <========\n"));
	}

    }	/* end if opcode != READ/WRITE */

SETUP_DMA:
        /*
         * Start of DMA code for a READ or WRITE scsi command, setup
         * the count, the RAM buffer offset, and the DMA registers.
         */

	PRINTD(targid, SCSID_DMA_FLOW,
	       ("asc_startdma: at SETUP_DMA:  szflags = 0x%x   \n",
			sc->sc_szflags[targid]));

	sc->sc_iodir[targid] = iodir;

            /* Setup the DMA subsystem for this target, and then start the DMA.
            The DMA subsystem is called for each amount of data to be transfered.
            Once the target has disconnected the DMA is considered "done". The
            value in sc_dmaxfer[] keeps track of the overall pieces of DMA.  The
            value in sc_xfercnt[] is used to keep track of the "segments" that
            the user's request was broken into by the state machine. */
    
            for (i=0; (*sc->dma_setup)( sc, targid, iodir,
                (sc->sc_bpcount[targid] - sc->sc_dmaxfer[targid]),
                (sc->sc_bufp[targid] + sc->sc_xfercnt[targid] +
                sc->sc_dmaxfer[targid])) == PDMA_RETRY &&
                i<10000 ; i++ )
                {
                }
            if (i > 9999 )
                {
                cprintf( "asc_startdma:  pdma_setup not returning PDMA_IDLE\n" );
                } 
            PRINTD(targid, 0x8000, ("asc_startdma: starting dma\n"));
    
            (*sc->dma_start)( sc, targid, DMA_CMD(ASC_XINFO) );
    
    return(SZ_SUCCESS);
    
    }        /* end asc_startdma */
    
/******************************************************************
 *
 * Name:	asc_sendcmd
 *
 * Abstract:	Send the CDB to the target by loading from the 
 *		softc to the ASC FIFO and issuing the transfer
 *		information command to the ASC.  Note that this
 *		case should only occur after a bus reset, when
 *		we have to renegotiate the synchronous offset
 *		with the drive. Under most cases, because of 
 *		the ASC's combination SELECT plus send CDB 
 *		command, this routine will not be called.
 *		It possibly could occur if during the select 
 *		an error occurred which left the target selected
 *		but without having transferred the CDB.  The 
 *		interrupt would indicate phase COMMAND, and the
 *		CDB would be loaded and transferred as indicated above.
 *
 * Inputs:
 *
 * 	sc	- the softc data structure
 *
 * Outputs:		None.
 *
 * Return values: None.
 *
 ******************************************************************/

asc_sendcmd(sc, targid)

register struct sz_softc *sc;
int	targid;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    u_char *byteptr;
    int datacnt, i;
    int cmd_type;
    int cmdcnt;

    sc->sc_szflags[targid] = 0;
    sc->sc_savcnt[targid] = 0;
    sc->sc_ascdmacount[targid] = 0;
    byteptr = (u_char *)&sc->sz_command;
    cmd_type = *byteptr;

    cmdcnt = sz_cdb_length (cmd_type, targid);

    /* Put the scsi command onto the scsi bus */
    asc_FIFOsenddata(sc, ASC_XINFO, byteptr, cmdcnt);

    /* Statistics update for READS and WRITES */

    if ( (cmd_type == SZ_WRITE) || (cmd_type == SZ_READ) ||
         (cmd_type == SZ_WRITE_10) || (cmd_type == SZ_READ_10) ) {
	if (sc->sc_dkn[targid] >= 0) {
	    dk_busy |= 1 << sc->sc_dkn[targid];
	    dk_xfer[sc->sc_dkn[targid]]++;
	    dk_wds[sc->sc_dkn[targid]] += sc->sc_bpcount[targid] >> 6;

	}	/* end if dkn[targid] >= 0 */

    }	/* end if READ/WRITE */

    return(SZ_SUCCESS);

}	/* end asc_sendcmd */

/******************************************************************
 *
 * Name:	asc_getstatus
 *
 * Abstract:	Read the status and message bytes from the FIFO by 
 *		issuing the Initiator Command Complete command. 
 *		Set the "combined cmd" state to ASC_CMD_CMPLT.
 *		For normal command completion, the ASC will 
 *		generate a function complete interrupt, with BOTH 
 *		the status and message bytes in the FIFO and the
 *		phase will be msg in.
 *
 * Inputs:
 *
 * 	sc	- the softc data structure
 *	io	- boolean flag - if true, then call asc_recvdata, 
 *		  else, status byte already in softc.
 *
 * Outputs:		None.
 *
 * Return values: None.
 *
 ******************************************************************/

asc_getstatus(sc, targid)

register struct sz_softc *sc;
int	targid;

{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */

    PRINTD(targid, (SCSID_CMD_EXP | SCSID_PHASE_STATE),
	   ("asc_getstatus: status byte = 0x%x = ",
	       sc->sc_status[targid]));

    PRINTD(targid, (SCSID_CMD_EXP | SCSID_PHASE_STATE),
	   ("", asc_print_status((int)sc->sc_status[targid])));


    /* To check the status a switch table is used to handle future, more
       complicated, status checking. */

    /* Save the status byte for the error log */

    if (sc->sc_curcmd[targid] == sc->sc_actcmd[targid])
	sc->sc_statlog[targid] = sc->sc_status[targid];

    switch(sc->sc_status[targid])
    {
      /* All went well onto the next phase. Fall through to the return() */

	case SZ_GOOD :
	break;

      /* Set the SZ_NEED_SENSE flag, the state mach. will handle the rest. */
      /* Also, disable reselection until the RQSNS is completed.     */

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
	default:
	    printf("asc_getstatus: target %d unexpected status = %x\n", 
		   targid, sc->sc_status[targid]);
	    return(SZ_RET_ABORT);	/* Assume bad failure for now */
	break;
    }

    return(SZ_SUCCESS);			/* every thing went well */

}	/* end asc_getstatus */



/******************************************************************
 *
 * name:	asc_msgout
 *
 * abstract:	Call the routine asc_sendata() to send the message
 *		using DMA.  Note that the send completion is 
 *		asynchronous!  We should get an interrupt with
 *		TC set in the status register and with BS set
 *		in the interrupt status register.  The phase
 *		bits will indicate what action to take next.
 *
 * inputs:
 *
 * 	sc	- the softc data structure
 *
 * outputs:		none.
 *
 * return values: none.
 *
 ******************************************************************/

asc_msgout(sc, targid)

register struct sz_softc *sc;
int	targid;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    u_long messg;
    int lun = sc->sz_t_read.lun;

    PRINTD(targid, SCSID_ENTER_EXIT,
       ("asc_msgout: enter, targid = %d\n", targid));

    /* Check if we need to send a Message Reject Message */

    if (asc_reject_message) {

	cprintf("asc_msgout: reject message TRUE\n");

	asc_reject_message = 0;

 	messg = SZ_MSGREJ;

        PRINTD(targid, SCSID_PHASE_STATE,
	   ("asc_msgout: sending Message Reject Message\n"));

        /* Put the Message Reject Message onto the scsi bus */
        asc_FIFOsenddata(sc, ASC_XINFO, &messg, 1);

    }	/* end if reject_message */

    /* Send the Identify Message with or without disconnects */

    else {
	/* Clear the "asc_assert_attn" flag */

	asc_assert_attn = 0;

        /* Check if we need to send a Synchronous DataXfer Message */

	if (!sc->sc_ascsentsync[targid]) {
	    sc->sc_ascsentsync[targid] = 1;

	    PRINTD(targid, SCSID_PHASE_STATE,
		("asc_msgout: sending Sync Data Transfer Message:\n"));

            /* Put the Synchronous Data Xfer Message onto the scsi bus */

            sc->sc_extmessg[targid][0] = SZ_EXTMSG;
            sc->sc_extmessg[targid][1] = 0x3;
            sc->sc_extmessg[targid][2] = SZ_SYNC_XFER;
            sc->sc_extmessg[targid][3] = asc_sync_xfer_per;
            sc->sc_extmessg[targid][4] = sc->asc_sync_offset; 

	    asc_FIFOsenddata(sc, ASC_XINFO, &sc->sc_extmessg[targid][0], 5);

        }	/* end if !sentsync */
	else
	{
	    /* Send a no-op just to get out of msgout phase. */

	    PRINTD(targid, SCSID_PHASE_STATE,
		("asc_msgout: sending Noop Message\n"));

	    messg = SZ_NOP;

	    /* Put the Message Reject Message onto the scsi bus */
	    asc_FIFOsenddata(sc, ASC_XINFO, &messg, 1);

	}	/* end else */

    }	/* end else */

    return(SZ_SUCCESS);

}	/* end asc_msgout */


/******************************************************************
 *
 * name:	asc_msgin
 *
 * abstract:	If Function Complete bit set, read status and message
 *		bytes from FIFO and call asc_getstatus to process the 
 *		status byte, else issue transfer Info command to
 *		cause the target to send the message byte(s).  
 *		Process the message byte.
 * inputs:
 *
 * 	sc	- the softc data structure
 *	io	- boolean flag - if TRUE, call asc_recvdata(), else
 *		  message byte already in softc structure.
 *
 * outputs:		none.
 *
 * return values: none.
 *
 ******************************************************************/

asc_msgin(sc, targid)

register struct sz_softc *sc;
int	targid;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    int len, i;
    int retval;
    int	flags;

    PRINTD(targid, SCSID_DISCONNECT,
       ("asc_mi: sc_message = 0x%x, sc_messgptr = 0x%x\n",
	    sc->sc_message[targid], sc->sc_messgptr[targid]));

    /* Switch on the type of message received */

    switch(sc->sc_message[targid]) {

    case SZ_CMDCPT:

	PRINTD(targid, SCSID_PHASE_STATE, 
	       ("asc_mi: SZ_CMDCPT message\n"));

	sc->sc_fstate = 0;
	sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	sc->sc_actbp[targid] = BPFREE;
	sc->sc_dboff_busy[targid][0] = 0;
	sc->sc_dboff_busy[targid][1] = 0;

	/* Assumes one command at a time for each target */

	if (sc->sc_dkn[targid] >= 0)
	    dk_busy &= ~(1 << sc->sc_dkn[targid]);

	sc->scsi_completed[targid] = 1;
	break;

    case SZ_SDP:

	PRINTD(targid, (SCSID_PHASE_STATE | SCSID_DISCONNECT),
	   ("\nasc_mi: ==> SZ_SDP message, sc_savcnt = %d\n\n", 
	        sc->sc_savcnt[targid]));

	sc->sc_szflags[targid] &= ~SZ_DID_DMA;

	sc->sc_dboff_len[targid][sc->sc_actbp[targid]] -= sc->sc_savcnt[targid];

	ASC_LOADCNTR(ascaddr, 0);
	wbflush();

	break;

    case SZ_DISCON:

	PRINTD(targid, (SCSID_PHASE_STATE | SCSID_DISCONNECT),
	   ("asc_mi: SZ_DISCON message\n")); 

	sc->sc_szflags[targid] |= SZ_WAS_DISCON;

	ASC_LOADCNTR(ascaddr, 0);
	wbflush();

/*	if ((sc->sc_prevpha == SZ_DATAI_PHA) || (sc->sc_prevpha == SZ_DATAO_PHA))*/

	if (sc->sc_szflags[targid] & SZ_DMA_INTR) {
	    sc->sc_szflags[targid] &= ~SZ_DMA_INTR;
	    sc->sc_szflags[targid] |= SZ_DMA_DISCON;	
	    PRINTD(targid, SCSID_DISCONNECT,
	       ("asc_mi: DMA_INTR -> DMA_DISCON\n"));

	}	/* end if DMA_INTR */

	break;
		
    case SZ_EXTMSG:

	PRINTD(targid, (SCSID_PHASE_STATE | SCSID_DISCONNECT),
	   ("asc_mi: SZ_EXTMSG msg, targ = %x, sc_szflags = 0x%x\n",
	        targid, sc->sc_szflags[targid]));

	sc->sc_extmessg[targid][0] = sc->sc_message[targid];

	if (!(sc->sc_szflags[targid] & SZ_EXTMESSG)) {
	    sc->sc_szflags[targid] |= SZ_EXTMESSG;
            sc->sc_messgptr [targid] = &sc->sc_extmessg [targid][0];
	    sc->sc_messg_len[targid] = -1;

	    break;

	}	/* end if */
	else if (sc->sc_messg_len [targid] == 0) {

	    PRINTD(targid, SCSID_DISCONNECT,
	        ("\n===>>>> asc_mi: think we have complete ext. messg\n"));

	    SZDEBUG_EXPAND(targid, &sc->sc_extmessg[targid][0], 
		    (int)(sc->sc_extmessg[targid][1]) + 2);

#ifdef SZDEBUG
	    DELAY(20000);	/* time to look at message */
#endif SZDEBUG
	    /*
	     * If the extended message is a Synchronous Data
	     * Transfer Request message then set the REQ/ACK
	     * offset for the current target otherwise reject
	     * the message.
	     *
	     */

	    if (sc->sc_extmessg[targid][2] == SZ_SYNC_XFER) {

		if (sc->sc_extmessg[targid][4] > sc->asc_sync_offset) {
		    sc->sc_extmessg[targid][4] = sc->asc_sync_offset;
		}

		sc->sc_ascreqack[targid] = sc->sc_extmessg[targid][4];
		sc->sc_ascsyncper[targid] = asc_sync_xfer_reg;

		PRINTD(targid, SCSID_CMD_EXP,
		   ("asc_mi: req ack offset = 0x%x, sync per = 0x%x\n",
		        sc->sc_ascreqack[targid], sc->sc_ascsyncper[targid]));


	    }	/* end if SYNC_XFER */
	    else  
		cprintf("asc_msgin: recv extmessg %x\n", 
		       sc->sc_extmessg[targid][2]);

	    sc->sc_szflags[targid] &= ~SZ_EXTMESSG;

	    if (sc->sc_alive[targid] == 0) { 		/* PROBING */
		u_long messg = SZ_MSGREJ;

		cprintf("asc_mi: recvd SYNC req in probe\n");
		asc_assert_attn = 1;
		asc_reject_message = 0;
		asc_FIFOsenddata(sc, ASC_XINFO, &messg, 1);

		break;

	    }	/* end if alive == 0 */

        }	/* end else */

	break;

    case SZ_ID_NODIS:
	PRINTD(targid, SCSID_PHASE_STATE, 
	   ("asc_mi: SZ_ID_NODIS message\n"));
	break;

    case SZ_ID_DIS:
	PRINTD(targid, SCSID_PHASE_STATE, 
	   ("asc_mi: SZ_ID_DIS message\n"));
	break;

    case SZ_RDP:
	PRINTD(targid, SCSID_PHASE_STATE, ("asc_mi: SZ_RDP message\n"));
	break;

    case SZ_MSGREJ:
	PRINTD(targid, SCSID_PHASE_STATE, 
	   ("asc_mi: SZ_MSGREJ message\n"));
	break;

    case SZ_LNKCMP:
	PRINTD(targid, SCSID_PHASE_STATE, 
	   ("asc_mi: SZ_LNKCMP message\n"));
	break;

    case SZ_LNKCMPF:
	PRINTD(targid, SCSID_PHASE_STATE, 
	   ("asc_mi: SZ_LNKCMPF message\n"));
	break;

    default:
	PRINTD(targid, SCSID_PHASE_STATE, 
	   ("asc_mi: unknown message = 0x%x\n", sc->sc_message[targid]));

#ifdef ASC_LOGERR
	flags = SZ_HARDERR | SZ_LOGMSG;
	scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x73, 0, flags);
#endif ASC_LOGERR

	return(SZ_RET_ABORT);

    }	/* end switch */

    /*
     * Assert attention as long as the "asc_assert_attn" flag is
     * set. Attention gets deasserted during a message out phase
     * and the "asc_assert_attn" flags gets cleared.
     */

    if (asc_assert_attn) {

	cprintf("asc_mi: asc_assert_atn TRUE, issuing ASC_SETATN\n");

	ascaddr->asc_cmd = ASC_SETATN;
	wbflush();

    }	/* end if asc_assert_atn */

    /* Note: this should generate a Disconnect Interrupt */

    PRINTD(targid, (SCSID_FLOW | SCSID_DISCONNECT),
       ("asc_mi: issuing MSGACPT command\n"));

    sc->sc_asccmd = ASC_MSGACPT;
    ascaddr->asc_cmd = ASC_MSGACPT;
    wbflush();

    return(SZ_SUCCESS);

}	/* end asc_msgin */

/******************************************************************
 *
 * name:	asc_clear_discon_io_tasks
 *
 * abstract:	Clear any disconnected io requests due to a bus reset.
 *
 * inputs:
 *
 * 	sc	- the softc data structure
 *
 * outputs:		none.
 *
 * return values: none.
 *
 ******************************************************************/

asc_clear_discon_io_tasks(sc)

register struct sz_softc *sc;
{

    int targid;
    int unit;
    struct buf *dp, *bp;

    /* Find out if any targets have I/O requests that disconnected */

    for (targid = 0; targid < NDPS; targid++) {
	sc->sc_ascsentsync[targid] = 0;

	if (targid == sc->sc_sysid)	/* skip initiator */
	    continue;

	if (sc->sc_alive[targid] == 0)	/* non existent target */
	    continue;

	if (sc->sc_attached[targid] == 0)
	    continue;

	unit = sc->sc_unit[targid];

	dp = (struct buf *)&szutab[unit];

	if (!dp->b_active)		/* target not active */
	    continue;

	if (dp->b_actf == NULL)
	    continue;			/* no IO requests pending */

	if (!(sc->sc_szflags[targid] & SZ_WAS_DISCON))
	    continue;			/* was not disconnected */

	PRINTD(targid, SCSID_ERRORS,
	       ("asc_clear_discon_io_tasks: clearing target %d\n", targid));

	printf("asc_clear_discon_io_tasks: clearing target %d\n", targid);

	bp = dp->b_actf;
	dp->b_actf = bp->av_forw;
	dp->b_active = 0;
	bp->b_resid = sc->sc_resid[targid];
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	biodone(bp);
	sc->sc_xstate[targid] = SZ_NEXT;
	sc->sc_xevent[targid] = SZ_BEGIN;

    }	/* end for */

}	/* end asc_clear_disc_io_tasks */

/******************************************************************
 *
 * name:	asc_reset
 *
 * abstract:	Reset the ASC chip.	
 *
 * inputs:
 *
 * 	sc	- the softc data structure
 *
 * outputs:		none.
 *
 * return values: none.
 *
 ******************************************************************/

asc_reset(sc)

register struct sz_softc *sc;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    int	i, retval;
    char modname[TC_ROMNAMLEN+1];
    char max[TC_ROMNAMLEN+1];
    char min[TC_ROMNAMLEN+1];
    int opt_type=0;


    /* Reset the ASC chip. */

    ascaddr->asc_cmd = ASC_RESET;
    wbflush();

    DELAY(25);				/* Just in case... */

    ascaddr->asc_cmd = ASC_NOOP;	/* recommended for some parts.. */
    wbflush();
    DELAY(25);

    ascaddr->asc_srto = 0xff;		/* Set select/reselect timeout to max*/
    wbflush();
    DELAY(25);

    /* Set the clock conversion factor up for 25 MHz system */

    ascaddr->asc_so = 0;
    wbflush();
    ascaddr->asc_ffss = 0;
    wbflush();

    /* Set our SCSI id and indicate use parity */

    ascaddr->asc_cnf1 = asc_def_cnf1 | sc->sc_sysid;
    wbflush();
    DELAY(25);

    ascaddr->asc_cnf2 = asc_def_cnf2;
    wbflush();
    DELAY(25);

    ascaddr->asc_cnf3 = asc_def_cnf3;
    wbflush();
    DELAY(25);

    sc->asc_sync_offset = ASC_SYNC_OFFSET;
    /* check if 3MIN or 3MAX card */
    if( cpu != DS_5500)  {

       if( tc_addr_to_name(sc->sc_slotvaddr, modname) == -1)
          cprintf("asc_reset():tc_addr_to_name failed ");

       if( !strcmp(modname, "PMAZ-AA ") )  {
          opt_type = DS_5000;
       }
       else
       if( !strcmp(modname, "PMAZ-BA ") )  {
          opt_type = DS_5000_100;
       }
       else  {
          cprintf("asc_reset: tc_... returned '%s', assuming 3MAX\n", modname);
          opt_type = DS_5000;
       }
    }

    sc->ioasicp = 0;		/* default to no IOASIC in use */

    switch (cpu) {
      case DS_5500:
         ascaddr->asc_ccf = 5;		
         wbflush();
         DELAY(25);
         break;
      case DS_5000:    
         ascaddr->asc_ccf = 5;		
         wbflush();
         DELAY(25);
	 break;
      case DS_5000_100:
	 if(opt_type == DS_5000)  {     /* 3MAX option card */
            ascaddr->asc_ccf = 3;		
            wbflush();
            DELAY(25);
         }
         else  {                   /* 3MIN opt. or base controller */
            ascaddr->asc_ccf = 5;		
            wbflush();
            DELAY(25);
            sc->ioasicp =  (char *) PHYS_TO_K1( BASE_IOASIC );
                      /* to tell the isr that an ioasic is here */
                      /* This will have to be modified to point to other */
                      /* addresses when the IOASIC based option cards */
                      /* become available */
         }
	 break;
      default:
	 panic ("asc_reset(): Unknown cpu type\n");
	 break;
    }

    /*
     * Assert SCSI bus reset for at least 25 Usec to clear the 
     * world. 
     */
    sc->sc_asccmd = ASC_RSTBUS;

    ascaddr->asc_cmd = ASC_RSTBUS;
    wbflush();
    DELAY(35);  /* 25 - ok for 3MAX 35 - ok for 3MIN */

    /*
     * Clear any pending interrupts from the reset.
     */
    sc->sc_asc_ssr = ascaddr->asc_ss;
    sc->sc_asc_sr = ascaddr->asc_stat;
    sc->sc_asc_isr = ascaddr->asc_intr;	/* will clear the interrupt */

    asc_clear_discon_io_tasks(sc);

/*  RPS - removing to avoid re-kmalloc'ing memory in init routine.
    perhaps pdma should have a reset entry point as well.
    (*sc->dma_init)(sc);
*/

    DELAY(sz_wait_for_devices * 1000000);
}	/* end asc_reset */

/******************************************************************
 *
 * Name:	ascintr
 *
 * Abstract:	Handle interrupts from the 53c94 chip.  Check for any
 * 		gross errors, then process the SCSI phase as reported in
 *		the status register if the bus service bit is set.
 *		Make sure that the sequence step register indicates 
 *		that the CDB bytes were sent.  
 *		When disconnect interrupt received AND not running 
 *		at probe time, start a new I/O.
 *
 *		Interrupt status register processing:
 *
 *		Disconnect - Ordinarily set after command completion
 *		when the target disconnects.  May be set to indicate
 *		select/reselect timeout.
 *
 *		Bus Service - Indicates that a target is requesting
 *		an information transfer phase.
 *
 *		Function Complete - When this bit is set, it indicates
 *		that the Selection of the target is complete (although
 *		the CDB may not have been transferred - see Sequence
 *		Step register), or that the status and message bytes
 *		are in the FIFO (after Command Complete has been 
 *		issued), or to indicate that a Transfer Info command
 *		has terminated prematurely, and that the target is
 *		requesting Message In phase.
 *
 *		Reselected - Set when a previously disconnected target
 *		reselects the ASC.  Note: make sure that a Select was
 *		not in progress!
 *
 * Inputs:

 *
 * 	cntrl	- the controller number which is interrupting
 *	
 *
 * Outputs:	None.
 *
 * Return values: None.
 *
 ******************************************************************/

ascintr(cntlr)
int cntlr;
{
    register struct sz_softc *sc = &sz_softc[cntlr];

    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    int targid, retval, timer;
    int s;
    int	flags;
    int fstate, dma_lcount;

    if (sc == NULL) { 		/* Stray interrupt on reboot */
    	/* Reset the ASC chip. */
    	ascaddr->asc_cmd = ASC_RESET;	/* sc is non-existant */
        wbflush();
        cprintf("ascintr: noprobe intr\n");
	return;
    }	/* end if sc == NULL */

    /* check if interrupt from IOASIC */
    if ( sc->ioasicp )	/* is there an IOASIC involved? */
        {
        if( ioasicint(sc, targid, cntlr ) )
            return;
        }

    /* Initialize variables */

    for (targid = 0; targid < NDPS; targid++)
	if (sc && sc->sc_active & (1 << targid))
	    break;

    if (targid >= NDPS)
	targid = -1;

    /* Read and save the relevant ASC registers */

    sc->sc_asc_ssr = ascaddr->asc_ss;	/* Sequence Step Register	*/
    sc->sc_asc_sr = ascaddr->asc_stat;	/* Status Register		*/
    sc->sc_asc_isr = ascaddr->asc_intr;	/* Interrupt Status - will clear intr*/

    PRINTD(targid, (SCSID_FLOW | SCSID_PHASE_STATE),
	   ("",asc_show_regs(sc, cntlr, targid)));
    PRINTD(targid, (SCSID_FLOW | SCSID_PHASE_STATE),
       ("ascintr: sc_szflags = 0x%x, sc_actbp = 0x%x\n", 
	    sc->sc_szflags[targid], sc->sc_actbp[targid]));

    /* Check for interrupt from a disconnected target */

    if (targid == -1 || sc->sc_active == 0) {

	/* Check if there are valid interrupts pending.  The only way
	   the above condition should be true is if we are just selecting
	   for the 1st time, or if we are reselected.     */

	if (!(sc->sc_asc_sr & ASC_INTP)) {
	    int flags;
#if ASC_LOGERR
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, -1, SZ_ET_STRYINTR, 0, 0, flags); /* ELDEBUG */
	    cprintf("ascintr: spurious interrupt from asc%d\n", cntlr);
#else ASC_LOGERR
	    cprintf("ascintr: spurious interrupt from asc%d\n", cntlr);
#endif ASC_LOGERR
	    return;
	}	/* end if ! ASC_INTP */

    }	/* end if targ == -1 || active == 0 */
	
    s = splbio();

    if (sc->sc_asc_sr & ASC_INTP) {
	if (sc->sc_asc_sr & ASC_PE) {		/* Parity error ? */
#ifdef ASC_LOGERR
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 0, 0, flags);
#else ASC_LOGERR
	    printf("ascintr: asc%d parity error\n", cntlr);
#endif ASC_LOGERR
	    goto HANDLE_ERROR;
	}	/* end if ASC_PE */

	if (sc->sc_asc_sr & ASC_GE) {		/* Gross error ? */
#ifdef ASC_LOGERR
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0, 0, flags);
#else ASC_LOGERR
	    printf("ascintr: asc%d gross error\n", cntlr);
#endif ASC_LOGERR
	    goto HANDLE_ERROR;

	}	/* end if ASC_GE */

	if (sc->sc_asc_isr & ASC_ILLCMD) {

	    printf("ascintr: asc%d, illegal cmd intr, cmd = 0x%x\n", 
		   cntlr, sc->sc_asccmd);
	    goto HANDLE_ERROR;
	}	/* end if ASC_ILLCMD */

	if (sc->sc_asc_isr & ASC_SCSIRST) {	/* Bus Reset ? */

#ifdef ASC_LOGERR
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 0, 0, SZ_HARDERR);
#else ASC_LOGERR
	    printf("ascintr: scsi %d bus reset\n", cntlr);
#endif ASC_LOGERR

	    PRINTD(targid, SCSID_PHASE_STATE, 
		   ("ascintr: scsi %d bus reset\n", cntlr));
	    goto HANDLE_ERROR;
        }	/* end if SCSIRST */

        /* Check for a STATE CHANGE : DIS or RSEL are set or BS and the
			target was idle (a selection) */

        if ((sc->sc_asc_isr & ASC_DIS) || (sc->sc_asc_isr & ASC_RSEL) ||
	    ((sc->sc_asc_isr & ASC_BS) && sc->sc_selstat[targid] == SZ_IDLE) ) { 
	    if (asc_state_change(sc, &targid) != SZ_SUCCESS) {
		PRINTD(targid, SCSID_ERRORS,
		   ("ascintr: bad rc from asc_state_change\n"));
		goto HANDLE_ERROR;

	    }	/* end if asc_state_change != SUCCESS */

	}	/* end complicated STATE CHANGE boolean 'or' */

	/* If disconnected and went to BUS FREE STATE then break */

	if (sc->scsi_bus_idle) {
	    goto done;

	}	/* end if scsi_bus_idle */

	/* See if we were interrupted during a xfer info phase because
	   a transfer completed.  If the current phase is the same 
	   phase as before the interrupt call the dma_cont() routine, there
	   is still more "informaion" to be xfered.  The value of sc_fstate
	   is still valid, we have not called asc_phase_change() yet. 
	   Note on extended message handling:  the continue routine
	   will simply return when handling extended messages, and the
	   "cleanup" will be handled by dma_end().  
	*/

	    /* If the SZ_DID_DMA flag is set, the DMA system was used.
	    Call the dma_end() routine to complete the transfer. */

	    /* If the phase is a data phase update the dmaxfer[] count.  This
		will set the current point for the next dma data xfer. */

	if (sc->sc_szflags[targid] & SZ_DID_DMA) {
	    fstate = phase2state (PHASE(sc->sc_asc_sr));

	    if (sc->sc_fstate != fstate) {
	    	PRINTD(targid, 0x8000,
		    ("ascintr: call dma_end,sc_fstate = %x, fstate = %x\n",
		    sc->sc_fstate, fstate));

	    	dma_lcount = (*sc->dma_end) (sc, targid);
                sc->sc_szflags[targid] &= ~SZ_PIO_INTR;

	    	PRINTD(targid, 0x8000,
		    ("ascintr: rtn fr dma_end, dma_lcount = %x\n",
			dma_lcount));

	    	sc->sc_dmaxfer[targid] += dma_lcount;
	    } else if ( (sc->sc_asc_sr & ASC_TC) ||
                       (sc->sc_szflags[targid] & SZ_PIO_INTR) ) {
	    	PRINTD(targid, 0x8000,
		    ("ascintr: (TC) call dma_cont, fstate = %x\n", fstate));

                sc->sc_szflags[targid] &= ~SZ_PIO_INTR;
	    	(void) (*sc->dma_cont) (sc, targid);
	    	splx(s);
	    	return;
	    }
	}

	/* The SII driver here checks for SII_MIS - phase mismatch.  We
	   are going to attempt a very literal rendition of that code in
	   the following manner: */

        if (((sc->sc_asc_isr & ASC_BS) || (sc->sc_asc_isr & ASC_FC)) ||
	    (PHASE(sc->sc_asc_sr) != sc->sc_fstate)) {		   /* 16b */

	    /* Handle the current bus phase */

	    if (asc_phase_change(sc, sc->sc_asc_sr, targid) != SZ_SUCCESS) {
		PRINTD(targid, SCSID_ERRORS,
		   ("ascintr: bad rc from asc_phase_change\n"));
	        goto HANDLE_ERROR;
	    }

	}	/* end if PHASE != sc_fstate */

    }	/* end of if intr pend */

/*-------------*/
/*             */
/*   d o n e   */
/*             */
/*-------------*/

done:

    PRINTD(targid, SCSID_FLOW,
       ("ascintr: at label - done:, szflags = 0x%x\n",sc->sc_szflags[targid]));

    if (sc->sc_szflags[targid] & SZ_WAS_DISCON) {

	PRINTD(targid, SCSID_DISCONNECT, ("ascintr: WAS_DISCON!\n"));

	sc->sc_actbp[targid] = BPFREE;
	sc->sc_dboff_busy[targid][0] = 0;
	sc->sc_dboff_busy[targid][1] = 0;

    }	/* end if flags && was_disconnected */

    /*
     * Check the status of the current SCSI operation. If the SCSI
     * operation completed or disconnected then start the next SCSI 
     * operation, otherwise wait for the DMA to complete.
     * 
     */

    if ((sc->scsi_bus_idle) && (!sc->scsi_polled_mode)) {
        if (sc->scsi_completed[targid])
	{
	  /* The command has completed, check for BUSY status.  If the target
	    was busy, leave the command on the queue, dp->b_active = 1.  Setup
	    a timer to wait a bit.  Call sz_start() to kick off the next
	    command on the queue.  The timer routine will handle "re-queueing"
	    the command in the state machine. */

	    if ( (sc->sc_szflags[ targid ] & SZ_BUSYTARG) != 0 )
	    {
	        timeout(asc_busy_target,
		       (caddr_t)sc->sc_unit[targid],
		       SZ_BUSY_WAIT );

		PRINTD(targid, (SCSID_FLOW | SCSID_ERRORS),
		   ("ascintr: COMMAND 0x%x COMPLETED with BUSY\n",sc->sz_opcode));

		sc->sc_active = 0;

		sc->scsi_completed[targid] = 0;

		sz_start( sc, -1 );		/* Start next I/O request */

	    }	/* end if BUSYTARG != 0 */
	    else
	    {
		PRINTD(targid, SCSID_FLOW,
		   ("ascintr: COMMAND 0x%x COMPLETED successfully\n", 
		        sc->sz_opcode));

#ifdef SZDEBUG
		if (sc->sz_opcode == SZ_RQSNS) {
		    PRINTD(targid, SCSID_CMD_EXP,
		       ("\n",asc_print_sense(sc, targid)));
		}
#endif SZDEBUG

		sc->sc_active = 0;

		sc->scsi_completed[targid] = 0;

		sz_start(sc, targid);	   /* Finish current I/O request */

	    }	/* end else */

	    splx(s);
	    return;
        }
	else if (sc->sc_szflags[targid] & SZ_WAS_DISCON) {

	    PRINTD(targid, SCSID_DISCONNECT,
	       ("ascintr: COMMAND 0x%x IN PROGRESS disconnected\n",
		   sc->sz_opcode));

	    sc->sc_active = 0;

    	    sz_start(sc, -1);			/* Start next I/O request */

	    splx(s);
	    return; /* NEW */
	}
	else if (sc->sc_szflags[targid] & SZ_SELTIMEOUT) {

	    PRINTD(targid, (SCSID_PHASE_STATE | SCSID_ERRORS),
	       ("ascintr: Selection time out on target\n"));

	    sc->sc_active = 0;

    	    sz_start(sc, -1);			/* Start next I/O request */

	    splx(s);
	    return; /* NEW */
	}
	else {
            sc->sc_active = 0;
	    sc->sc_fstate = 0;
	    /* sc->sc_szflags[targid] = (SZ_NEED_SENSE|SZ_RETRY_CMD; */
	    sc->sc_szflags[targid] = (SZ_NEED_SENSE);

	    sz_start(sc, targid);

	    splx(s);
	    return;
	}	/* end else */

    }	/* end if bus_idle and !polled_mode */

    else {

	splx(s);

	return;

    }	/* end else */

HANDLE_ERROR:

#ifdef SZDEBUG
       asc_show_regs(sc, cntlr, targid);
#endif SZDEBUG

#ifdef ASC_LOGERR
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 0, 0, flags);
#else ASC_LOGERR
        printf("ascintr: command aborted (bus=asc%d target=%d cmd=0x%x)\n",
	    cntlr, targid, sc->sc_curcmd[targid]);
#endif ASC_LOGERR

    /* Abort the current SCSI operation due to error */

    PRINTD(targid, (SCSID_PHASE_STATE | SCSID_ERRORS),
	("ascintr: command aborted (bus=asc%d target=%d cmd=0x%x)\n",
	cntlr, targid, sc->sc_curcmd[targid]));

    PRINTD(targid, (SCSID_PHASE_STATE | SCSID_ERRORS), 
	   ( "", asc_dumpregs(cntlr, WHO_ASC)));

#ifdef ASC_LOGERR
    scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 2, 0, SZ_HARDERR);
#else ASC_LOGERR
    printf("ascintr: resetting bus\n");
#endif ASC_LOGERR

    asc_reset(sc);

    sc->sc_selstat[targid] = SZ_IDLE;
    sc->sc_szflags[targid] |= SZ_ENCR_ERR;
    sc->sc_xstate[targid] = SZ_ERR;
    sc->sc_xevent[targid] = SZ_ABORT;
    sc->sc_active = 0;

    sz_start(sc, targid);

    splx(s);

} 	/* end ascintr */

/******************************************************************************
 *
	 * name:	phase2state
 *
 * abstract:	Convert SCSI phase 'phase' to state machine 'fstate'
 *
 *****************************************************************************/

phase2state (phase)
char phase;
{
    switch (phase) {
	case SCS_DATAO:		return (SZ_DATAO_PHA);
	case SCS_DATAI:		return (SZ_DATAI_PHA);	
	case SCS_CMD:		return (SZ_CMD_PHA);
	case SCS_STATUS:	return (SZ_STATUS_PHA);
	case SCS_MESSO:		return (SZ_MESSO_PHA);
	case SCS_MESSI:		return (SZ_MESSI_PHA);
    }
}

/******************************************************************
 *
 * name:	asc_state_change
 *
 * abstract:	Handle Select completion, Reselection, or a 
 *		Disconnect.
 *
 * inputs:
 *	sc	- the softc pointer for this controller
 * activetargid - pointer to target to be filled in here
 *
 * outputs:	sc_selstat - set to indicate selection status
 *
 * return values:	SZ_SUCCESS
 *
 ******************************************************************/

asc_state_change(sc, activetargid)

register struct sz_softc *sc;
int	*activetargid;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; 
    u_char targid, tident;
    struct buf *dp;

    /* If Function Complete set and the bus was previously idle, then
       this should be the response to a SELECT.	*/

    /* Note: according to the NCR spec, FC should not be set if reselected,
       but I have seen it happen - thus the extra test for RSEL */

      if (!(sc->sc_asc_isr & ASC_RSEL) &&
	  ASC_SELECT_CMD(DMA_CMD(sc->sc_asccmd)) && (sc->sc_asc_isr & ASC_FC)) {

	if ((sc->sc_asc_ssr & ASC_SSTEP_MSK) == ASC_SSTEP_CMPLT) {
	    u_char	*byteptr;
	    int		cmd_type;

	    /* If here, we issued a Select sequence command which will
	       have automatically sent our command bytes to the target,
	       hence, we have to update our statistics here, because
	       asc_sendcmd will never get called for this transaction. */

	    targid = (u_char)(*activetargid);

	    sc->sc_selstat[targid] = SZ_SELECT;

	    /* Statistics update for READS and WRITES */

	    byteptr = (u_char*)&sc->sz_command;
	    cmd_type = *byteptr;

	    if ((cmd_type == SZ_WRITE) || (cmd_type == SZ_READ)) {
		if (sc->sc_dkn[targid] >= 0) {
		    dk_busy |= 1 << sc->sc_dkn[targid];
		    dk_xfer[sc->sc_dkn[targid]]++;
		    dk_wds[sc->sc_dkn[targid]] += sc->sc_bpcount[targid] >> 6;

		}	/* end if dkn[targid] >= 0 */

	    }	/* end if READ/WRITE */

	    return(SZ_SUCCESS);

	}	/* end if FC and SSTEP_MASK == SSTEP_CMPLT */

    	else 
	    /* If here, we issued Select w/ATN and Stop.  The ASC should
	       have sent one message byte.  The phase SHOULD equal
	       message out!	*/

	    if ((sc->sc_asc_ssr & ASC_SSTEP_MSK) == ASC_SSTEP_MESSO) {

		targid = (u_char)(*activetargid);

		sc->sc_selstat[targid] = SZ_SELECT;

		return(SZ_SUCCESS);

	    }	/* end if FC and SSTEP_MASK == SSTEP_MESSO */

        else   /* check if selected but target did not assert command
                  phase - probably busy */
	    if ((sc->sc_asc_ssr & ASC_SSTEP_MSK) == ASC_SSTEP_SELNOCMD) {
		targid = (u_char)(*activetargid);

		sc->sc_selstat[targid] = SZ_SELECT;

		return(SZ_SUCCESS);

	    }	/* end if FC and SSTEP_MASK == SSTEP_SELNOCMD */
    	else {
	    /* Check for a select timeout */

	    if ((sc->sc_asc_ssr & ASC_SSTEP_MSK) == ASC_SSTEP_TO)   {
	        PRINTD(targid, SCSID_ERRORS,
		 ("asc_st_ch: select timeout target = %d\n", targid));

                /* Since the select failed, don't assume that any
	          bytes are left in the FIFO, so flush to get back to a 
	          clean state.     */

	        ascaddr->asc_cmd = ASC_FLUSH;
	        wbflush();
	        return(SZ_RET_ABORT);
	    }	/* end if FC and SSTEP_MASK == SSTEP_TO */

	}	/* end else */

    }	/* end if ASC_FC */ 


  /* Here only for a disconnect or Re/Selection.  Check the
    copy of the interrupt register to see what is needed. */

    if (!(sc->sc_asc_isr & ASC_DIS)) {

      /* Handle a select or reselect here */

	sc->scsi_bus_idle = 0;

	/* Handle a reselect */

	if (sc->sc_asc_isr & ASC_RSEL) {
	    int ffr;

	    ffr =  (int)(ascaddr->asc_ffss & ASC_FIFO_MSK);
	    wbflush();

	    /* According to the 53C90 User's Guide, p. 12, a reselected device
	       will have two bytes in its FIFO. */

	    if (ffr != 2)
		printf("\nasc_st_ch: Warning - ffr != 2, => 0x%x\n", ffr);

	    targid = ascaddr->asc_fifo;		/* read bus id */
	    tident = ascaddr->asc_fifo;		/* read identify message */

    PRINTD(targid, SCSID_DISCONNECT,
       ("asc_st_ch: sr 0x%x, isr 0x%x, targ 0x%x resel, id mesg = 0x%x\n", 
        sc->sc_asc_sr, sc->sc_asc_isr, targid, tident));

	    /* Note: the targid is already unencoded - just the same effect
	       as (1 << targid), but it also contains the initiator's id 
	       (i.e., the ASC's).  Therefore we have to mask off our own
	       id before setting sc_active. 	*/

	    targid &= ~(1 << sc->sc_sysid);
	    sc->sc_active = targid;

	    targid = asc_encode_busid(targid);

	    /* Reset the synchronous offset & period for the device that
	       is reconnecting... */

	    ascaddr->asc_so = sc->sc_ascreqack[targid];
	    wbflush();

	    ascaddr->asc_sp = sc->sc_ascsyncper[targid];
	    wbflush();

	    PRINTD(targid,SCSID_DISCONNECT,
	       ("asc_st_ch: encoded target id = 0x%x\n", targid));

	    sc->sc_selstat[targid] = SZ_RESELECT;
    	    sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;
	    sc->sc_message[targid] = tident;

	}	/* end if RSEL */

	/* Handle a select */

	else {

	    DELAY(10000);
	    printf("\n\nasc_st_ch: Warning --- Selected as target!!!!!!!!!\n\n");
	    printf("---------sr=%x, isr=%x, ssr=%x, cmd=%x\n",
		sc->sc_asc_sr, sc->sc_asc_isr, sc->sc_asc_ssr, sc->sc_asccmd);

	    targid = *activetargid;

	    PRINTD(targid, SCSID_PHASE_STATE,
		("asc_st_ch: target ID %d selected\n",targid));

	    sc->sc_active = (1 << targid);

	    sc->sc_selstat[targid] = SZ_SELECT;

	}	/* end else (SELECT) */

    }	/* end not disconnect */

    /* Handle a disconnect here */

    else {

	sc->scsi_bus_idle = 1;	/* a disconnect makes the bus idle */

	ascaddr->asc_cmd = ASC_ESELRSEL;	/* enable select/reselect */
	wbflush();

	/*
	 * Check for a back-to-back disconnect occurring in 
	 * which case the active target ID will be -1 and the 
	 * SCSI bus will be idle. KLUDGE for CDROM device.
	 */

	if (*activetargid == -1) {
	    return(SZ_SUCCESS);
	}

	targid = *activetargid;

	PRINTD(targid, (SCSID_PHASE_STATE | SCSID_DISCONNECT),
	    ("asc_st_ch: target ID %d disconnected\n",targid));

	/* Check to determine if the Disconnect interrupt was due to a 
	timeout during selection.  If the select state of the target
	is IDLE, a selection must ? have been in progress.  Set the
	selection timeout flag to inform who was attempting the selection
	that it timed out. */

	if (sc->sc_selstat[targid] == SZ_IDLE ) {
    	    sc->sc_szflags[targid] |= SZ_SELTIMEOUT;

	    PRINTD(targid, (SCSID_PHASE_STATE | SCSID_DISCONNECT),
		("asc_st_ch: target ID %d select timeout\n",targid));
	    ascaddr->asc_cmd = ASC_FLUSH;
	    wbflush();

	}	/* end if selstate == IDLE */

	if (sc->scsi_completed[targid])
	    sc->sc_selstat[targid] = SZ_IDLE;
	else
	    sc->sc_selstat[targid] = SZ_DISCONN;

    }	/* end else (disconnect) */

    /* Check to see if we were reselected while attempting a selection.  If
       so, the passed-in activetargid will be different from the targid we
       just got from the FIFO.  If so, set the old active target's state
       machine up to do something reasonable, like to retry later. */


    if ((*activetargid != -1) && (*activetargid != targid)) {

	sc->sc_active = (1 << targid);		/* Set new active target */

/* debug 	I kept this around for debug --- dallas ***************
 *
 *		PRINTD(targid, 0x8000,
 *		 ("asc_st_ch: RQSNS (xstate) ==>> while sel %d, resel by %d, ", 
 *                      *activetargid, targid));
 *
 *		PRINTD(targid, 0x8000,
 *		   ("xstate = %x, pxstate = %x, actcmd = %x, curcmd = %x\n",
 *		    sc->sc_xstate[*activetargid],sc->sc_pxstate[*activetargid],
 *	            sc->sc_actcmd[*activetargid],
 *	            sc->sc_curcmd[*activetargid]));
 *
*/


	PRINTD(targid, SCSID_DISCONNECT,
	   ("asc_st_ch: while selecting %d, reselected by %d\n", 
                 *activetargid, targid));

	sc->sc_selstat[*activetargid] = SZ_BBWAIT;	/* Bus Busy */
	sc->sc_xstate[*activetargid] = sc->sc_pxstate[*activetargid];

	if ((sc->sc_xstate[*activetargid] == SZ_SP_START)  ||
	    (sc->sc_xstate[*activetargid] == SZ_SP_CONT)  ||
	    (sc->sc_xstate[*activetargid] == SZ_R_DMA)  ||
	    (sc->sc_xstate[*activetargid] == SZ_W_DMA))  {

                if ((sc->sc_actcmd[*activetargid] == SZ_RQSNS) &&
                    (sc->sc_curcmd[*activetargid] != SZ_RQSNS) ) {
                    sc->sc_xevent[*activetargid] = SZ_SELRETRY_SNS;
                }
                else  
                    sc->sc_xevent[*activetargid] = SZ_SELRETRY_CMD;
        }
        else {
             sc->sc_xevent[*activetargid] = SZ_SELRETRY_CMD;
	}

	dp = (struct buf *)&szutab[(sc->sc_unit[*activetargid])];
	dp->b_active = 0;

	sz_start(sc, *activetargid);	/* Tell state machine to retry later */

    }	/* end if *active != targid */

    *activetargid = targid;

    return(SZ_SUCCESS);

}	/* end asc_state_change */

/******************************************************************
 *
 * name:	asc_phase_change
 *
 * abstract:	Handle a phase change on the ASC.  Called whenever
 *		the Bus Service bit is set.  First, check the
 *		combined command state to see if any special processing
 *		needs to be done.  Call the phase handler routine with
 *		a flag indicating whether or not any information is to
 *		be read from the bus (via send/recvdata).
 *
 * inputs:
 *
 * 	sc	- the softc data structure
 *	dstat	- ???
 *
 * outputs:		none.
 *
 * return values: none.
 *
 ******************************************************************/

asc_phase_change(sc, stat, targid)

register struct sz_softc *sc;
u_char 	stat;
int	targid;
{
    int cntlr = sc->sc_ascnum;
    ASC_REG *ascaddr = ASC_REG_ADDR; /* setup the register pointer */
    int tmp_state;
    int tmp_phase;
    int phase;
    int	ffr;
    int	io = 0;
    int	flags;
    short	i;

    /* Get the current bus phase */

    phase = PHASE(stat);

    PRINTD(targid, (SCSID_PHASE_STATE | SCSID_ENTER_EXIT),
	   ("asc_ph_ch: current bus phase = "));

    PRINTD(targid, (SCSID_PHASE_STATE | SCSID_ENTER_EXIT),
	   ("", asc_print_phase(phase)));

    ffr =  (int)ascaddr->asc_ffss;
    wbflush();

    ffr &= ASC_FIFO_MSK;

    switch(phase) {

    case SCS_MESSI:

	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_MESSI_PHA;

	/* If we were reselected, asc_state_change has already read the 
	   message byte out of the FIFO & stored it in sc_message, so
	   we can go process the message directly. */

	if (sc->sc_asc_isr & ASC_RSEL) {

	    if (asc_msgin(sc, targid) != SZ_SUCCESS)
		return(SZ_RET_ABORT);

	    return(SZ_SUCCESS);

	}	/* end if RSEL */

	/* According to the chip spec, if Function Complete, could 
	   either be the response to a previous ICCS or Xfer Info
	   (note: only if phase == Msg In).  For now, to differentiate
	   between the two cases, we will look at the ASC command
	   register. If the last command was ICCS, read both the status 
	   and message bytes from the FIFO, then call the processing
	   routines with the I/O flag set appropriately.  If is was 
	   Xfer Info, then go process the message via asc_msgin().
	 */

	if (sc->sc_asc_isr & ASC_FC) {

	    /*if (sc->sc_asccmd == DMA_CMD(ASC_XINFO)) {*/
	    if (sc->sc_asccmd == ASC_XINFO) {

		/* Here, we assume that on a previous iteration of this code
		   that we issued an DMA XINFO to read in a message
		   byte.  That byte should be in the rambuf now. */

/* bug check */
		if (sc->sc_messgptr[targid] == (u_char*)0) {
		    cprintf("asc_ph_ch: MESSI - sc_messgptr bad\n");
		}
/* end bug check */

		*(sc->sc_messgptr [targid]) = ascaddr->asc_fifo;

		if (sc->sc_szflags [targid] & SZ_EXTMESSG) {
		    if (sc->sc_messg_len [targid] == -1)
			sc->sc_messg_len [targid] = *(sc->sc_messgptr [targid]);
		    else
			(sc->sc_messg_len [targid])--;
		}
		/* Go process the message */

		return(asc_msgin(sc, targid));

	    }	/* end if asccmd == XINFO */

	    if (sc->sc_asccmd == ASC_ICCS) {

		/* Sanity check! */

		if (ffr != 2) {
		  PRINTD(targid, (SCSID_FLOW | SCSID_PHASE_STATE),
	          ("\n\nasc_ph_ch: PANIC -> FC & cmd=ICCS, ffr = %d\n\n", 
		   ffr));
	        }	/* end if ffr != 2 */

		sc->sc_status[targid] = ascaddr->asc_fifo;
		sc->sc_message[targid] = ascaddr->asc_fifo;
	    
		if (asc_getstatus(sc, targid) != SZ_SUCCESS)
		      return(SZ_RET_ABORT);

		if (asc_msgin(sc, targid) != SZ_SUCCESS)
		      return(SZ_RET_ABORT);

		return(SZ_SUCCESS);

	      	}	/* end if ICCS */

	}	/* end if FC */

	if (ffr != 0) {
	    PRINTD(targid, SCSID_DISCONNECT,
	       ("\nasc_ph_ch: (bug check)   ffr = %d\n", ffr));
	}
	/* If we were doing DATA OUT phase, we have to adjust things for
	   the # of bytes left in the FIFO but not transferred when the
	   target changed phase. */
	   
	if (sc->sc_prevpha == SZ_DATAO_PHA) {

	    ASC_GETCNTR(ascaddr, sc->sc_savcnt[targid]);      

	    sc->sc_ascdmacount[targid] += ffr;

	    PRINTD(targid, SCSID_DISCONNECT,
	       ("asc_ph_ch: new sc_ascdmacount = %d, sc_savcnt = %d\n",
	       sc->sc_ascdmacount[targid], sc->sc_savcnt[targid]));

	    PRINTD(targid, SCSID_DISCONNECT,
		   ("asc_ph_ch: flushing FIFO...\n"));

	    ascaddr->asc_cmd = ASC_FLUSH;  
	    wbflush();

	}	/* end if prevpha == DATAO */

	PRINTD(targid, SCSID_DISCONNECT,
	   ("asc_ph_ch: recieving sc_message\n"));

	/* save location for getting the message out of FIFO after XINFO */
	if (sc->sc_szflags [targid] & SZ_EXTMESSG)
	    (sc->sc_messgptr [targid])++;
	else
	    sc->sc_messgptr [targid] = &sc->sc_message [targid];

	/* get the message from SCSI bus */
	sc->sc_asccmd = ASC_XINFO;
	ascaddr->asc_cmd = ASC_XINFO;

	return(SZ_SUCCESS);

	break;

    case SCS_MESSO:

/* Try and catch chip anomaly, see note below for CMD */

	if (sc->sc_prevpha == SZ_MESSO_PHA) {
	    cprintf("asc_phase_state: CHIP FIFO ANOMALY? Issuing XINFO\n");
    	    cprintf("FFR=%x SS=%x ST=%x CMD=%x ",  (int)ascaddr->asc_ffss,
    			ascaddr->asc_ss,
    			ascaddr->asc_stat,
			ascaddr->asc_cmd );
	    sc->sc_asccmd = ASC_XINFO;
	    ascaddr->asc_cmd = ASC_XINFO;	/* Note: NO DMA ALLOWED! */
	    wbflush();
	    return(SZ_SUCCESS);  /* maria */
	}	/* end if anomaly */

        sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_MESSO_PHA;
	sc->sc_szflags[targid] = 0;

	if (asc_msgout(sc, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);

	break;

    case SCS_CMD:

/* Chip anomaly - if the previous phase was CMD, then see NCR SCSI Engineering
   Notes No. 810, 08/88, Rev 1.2	*/

	if (sc->sc_prevpha == SZ_CMD_PHA) {
	    printf("asc_ph_ch: CHIP FIFO ANOMALY? Issuing XINFO\n");
	    sc->sc_asccmd = ASC_XINFO;
	    ascaddr->asc_cmd = ASC_XINFO;	/* Note: NO DMA ALLOWED! */
	    wbflush();

	    return(SZ_SUCCESS);

	}	/* end if */

	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_CMD_PHA;

	if (asc_sendcmd(sc, targid) != SZ_SUCCESS)
	   return(SZ_RET_ABORT);

	break;

    case SCS_STATUS:

	/* Flush the FIFO to make up for chip anomaly */

	ascaddr->asc_cmd = ASC_FLUSH;	/* Flush the fifo */
	wbflush();

	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_STATUS_PHA;

	/* If the Tranfer Count Zero bit is set, issue the Initiator 
	   Command Complete command to xfer both the status and message
	   bytes.  Our next interrupt should be message in phase with
	   the Function Complete bit set.  We can then call the 
	   asc_getstatus and asc_msgin routines with the io flag false. */

	if (!(sc->sc_asc_sr & ASC_TC)) {
	    int		cnt, ffr;
	    char	cmd;

	    /* See what the Transfer counter is, just for laughs */
	    
	    ASC_GETCNTR(ascaddr, cnt);
	    
	    PRINTD(targid, (SCSID_CMD_EXP | SCSID_ERRORS),
	       ("asc_ph_ch: TC bit not set, TC count = %d\n", cnt));

	}	/* end if !TC */

	PRINTD(targid, SCSID_FLOW,

	   ("asc_ph_ch: issuing ICCS for status phase\n"));

	sc->sc_asccmd = ASC_ICCS;
	ascaddr->asc_cmd = ASC_ICCS;
	wbflush();

	return(SZ_SUCCESS);

	break;

    case SCS_DATAO:
	
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAO_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];

	wbflush(); 

	if (asc_startdma(sc, SZ_DMA_WRITE, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);

	break;

    case SCS_DATAI:

	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAI_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];

	wbflush();

	if (asc_startdma(sc, SZ_DMA_READ, targid) != SZ_SUCCESS)
	    return(SZ_RET_ABORT);

	break;

    default:
	PRINTD(targid, SCSID_PHASE_STATE, 
	       ("asc_ph_ch: unexpected bus phase = "));
	PRINTD(targid, SCSID_PHASE_STATE, ("", asc_print_phase(phase)));

#ifdef ASC_LOGERR
	scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4c, 0, SZ_HARDERR);
#else ASC_LOGERR
	mprintf("asc_phase_change: unexpected bus phase 0x%x\n", phase);
#endif ASC_LOGERR

	return(SZ_RET_ABORT);

	break;

    }	/* end switch(phase) */

    return(SZ_SUCCESS);

}	/* end asc_phase_change */


/******************************************************************************
 *
 * name:	asc_FIFOsenddata
 *
 * abstract:	Put 'count' numbers of bytes pointed to by 'data' on the
 *		ASC FIFO, then issue the non-DMA ASC command 'ascCmd'.
 *
 *****************************************************************************/

asc_FIFOsenddata (sc, ascCmd, data, count)
register struct sz_softc	*sc;
u_char				ascCmd,	*data;
int				count;
{
    ASC_REG *ascaddr = ASC_REG_ADDR;	/* bad indirect reference of sc */
    int i;

    for (i = 0; i < count; i++)		/* put data on FIFO */
	ascaddr->asc_fifo = *data++;
    wbflush ();

    sc->sc_asccmd = ascCmd;		/* push data out to SCSI bus */
    ascaddr->asc_cmd = ascCmd;
    wbflush ();
}

/******************************************************************
 *
 * Dump out the registers on the ASC chip.
 *
 ******************************************************************/
asc_dumpregs(cntlr, who)
int	cntlr;
int	who;
{
    register struct sz_softc *sc = &sz_softc[cntlr];
    ASC_REG *ascaddr = ASC_REG_ADDR; /* must change for > 1 ASC */

	/*
	 * Called from scsi_logerr() binary error log.
	 * Print most meaningful registers (limit one line).
	 */

    if (who) {
	cprintf("ASC %d regs: ", cntlr);
	cprintf("sr=0x%x ", sc->sc_asc_sr & 0xff);
	cprintf("isr=0x%x ", sc->sc_asc_isr & 0xff);
	cprintf("ssr=0x%x ", sc->sc_asc_ssr & 0xff);
	cprintf("cmd=0x%x ", ascaddr->asc_cmd & 0xff);
	cprintf("\n");
	return;

    }	/* end if who */

    cprintf("\t\tSCSI ASC register dump:\n");
    cprintf("\t\tasc_tclsb = 0x%x\n", ascaddr->asc_tclsb & 0xff);
    cprintf("\t\tasc_tcmsb = 0x%x\n", ascaddr->asc_tcmsb & 0xff);
    cprintf("\t\tasc_cmd   = 0x%x\n", ascaddr->asc_cmd & 0xff);
    cprintf("\t\tasc_stat  = 0x%x\n", sc->sc_asc_sr & 0xff);
    cprintf("\t\tasc_ss    = 0x%x\n", sc->sc_asc_ssr & 0xff);
    cprintf("\t\tasc_intr  = 0x%x\n", sc->sc_asc_isr & 0xff);
    cprintf("\t\tasc_ffss  = 0x%x\n", ascaddr->asc_ffss & ASC_FIFO_MSK);
    cprintf("\t\tasc_cnf1  = 0x%x\n", ascaddr->asc_cnf1 & 0xff);
    cprintf("\t\tasc_cnf2  = 0x%x\n", ascaddr->asc_cnf2 & 0xff);
    cprintf("\t\tasc_cnf3  = 0x%x\n", ascaddr->asc_cnf3 & 0xff);

}	/* end asc_dumpregs */

/******************************************************************
 *
 * Print out the current bus phase.
 *
 ******************************************************************/

asc_print_phase(phase)

int phase;
{

    switch(phase) {
    case SCS_DATAO:
	PRINTD(NTARG, 0xff,("SCS_DATAO\n"));
	break;
    case SCS_DATAI:
	PRINTD(NTARG, 0xff, ("SCS_DATAI\n"));
	break;
    case SCS_MESSI:
	PRINTD(NTARG, 0xff, ("SCS_MESSI\n"));
	break;
    case SCS_MESSO:
	PRINTD(NTARG, 0xff,("SCS_MESSO\n"));
	break;
    case SCS_CMD:
	PRINTD(NTARG, 0xff,("SCS_CMD\n"));
	break;
    case SCS_STATUS:
	PRINTD(NTARG, 0xff, ("SCS_STATUS\n"));

	break;
    default:
	PRINTD(NTARG, 0xff, ("UNKNOWN\n"));
	break;
    }	/* end switch(phase) */

}	/* end asc_printphase */

/******************************************************************
 *
 * Print out the current command status.
 *
 ******************************************************************/

asc_print_status(status)

int status;
{

    switch(status) {
    case SZ_GOOD:
	PRINTD(NTARG, 0x24, ("SZ_GOOD "));
	break;
    case SZ_CHKCND:
	PRINTD(NTARG, 0x24, ("SZ_CHKCND "));
	break;
    case SZ_INTRM:
	PRINTD(NTARG, 0x24,("SZ_INTRM "));
	break;
    case SZ_RESCNF:
	PRINTD(NTARG, 0x24,("SZ_RESCNF "));
	break;
    case SZ_BUSY:
	PRINTD(NTARG, 0x24, ("SZ_BUSY "));
	break;
    default:
	PRINTD(NTARG, 0x24, ("Bogus status byte ??? "));
	break;
    }
    PRINTD(NTARG,0x24,("\n"));
}

/******************************************************************
 *
 * Print out the inquiry data information.
 *
 ******************************************************************/

asc_print_inq_info(idp)

struct sz_inq_dt *idp;
{
    
    char hold[SZ_PID_LEN+1];
    int i;
    u_char *ptr;

    PRINTD(NTARG, SCSID_CMD_EXP,
	   ("***************************************************\n"));
    PRINTD(NTARG, SCSID_CMD_EXP,("\nDumping Out Inquiry Data from 0x%x:\n", idp));

    for (i = 0; i < SZ_VID_LEN; i++)
	hold[i] = idp->vndrid[i];

    hold[i] = '\0';
    PRINTD(NTARG, SCSID_CMD_EXP, ("Vendor ID = %s\n", hold));

    for (i = 0; i < SZ_PID_LEN; i++)
	hold[i] = idp->prodid[i];
    hold[i] = '\0';
    PRINTD(NTARG,SCSID_CMD_EXP,("Product ID = %s\n", hold));
    PRINTD(NTARG,SCSID_CMD_EXP,("Peripheral Device Type = 0x%x\n",idp->perfdt));
    PRINTD(NTARG,SCSID_CMD_EXP,("Device Type Qualifier = 0x%x\n",idp->devtq));

    for (i = 0; i < SZ_REV_LEN; i++)
	hold[i] = idp->revlvl[i];
    hold[i] = '\0';
    PRINTD(NTARG,SCSID_CMD_EXP,("Revision Level = %s\n\n", hold));
    PRINTD(NTARG,SCSID_CMD_EXP,
	   ("***************************************************\n"));

}	/* end asc_print_inq_info */

#ifdef mips
/*
#ifdef ASC_TEST_REMOVE
*/

/******************************************************************
 *
 * name:	asc_get_validbuf
 *
 * abstract:	Figure out which of the "double buffers" is actually
 *		in use and return it.
 *
 * inputs:
 *
 * 	sc	- the softc data structure
 *	targid  - the target id
 *
 * outputs:		none.
 *
 * return values:  0,1		The buffer to use
 *		   -1		"can't happen"     :-)    (shouldn't)
 *
 ******************************************************************/

asc_get_validbuf(sc, targid)

struct sz_softc *sc;
int targid;
{

    PRINTD(targid, SCSID_ENTER_EXIT,
       ("asc_get_validbuf: enter - sc = 0x%x, targid = %d\n", sc, targid));

	if (sc->sc_dboff_busy[targid][0] == 1)
		if(sc->sc_dboff_busy[targid][1] == 1)
		    return(-1);
		else
		    return(1);
	else
		return(0);
}


#endif mips

/******************************************************************
 *
 * Name:	asc_busy_target
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

asc_busy_target(unit)

int unit;
{

    register struct uba_device *ui;
    register struct sz_softc *sc;
    int		cntlr;
    ASC_REG 	*ascaddr; 
    int 	targid, s;
    int 	flags;
    struct buf 	*dp, *bp;

    ui = szdinfo[unit];
    cntlr = ui->ui_ctlr;
    targid = ui->ui_slave;

    sc = &sz_softc[cntlr];
    ascaddr = ASC_REG_ADDR; /* setup the register pointer */

    dp = (struct buf *)&szutab[unit];

    PRINTD(targid, 0xff,
       ("\n\nasc_busy_target: ***>>>> cntlr = 0x%x, targid = %d\n\n",
	   cntlr, targid));

  /* Check and make sure that the target is in the busy state.  For now report
    the error and continue.  In theory the only way to here is via the status
    in routine. */

    if (!(sc->sc_szflags[targid] & SZ_BUSYTARG) )
    {
#ifdef ASC_LOGERR
	scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x73, 0, flags);
#else
	mprintf( "asc_busy_target: BUSY flag not set on %d\n", targid );
#endif ASC_LOGERR
    }

    sc->sc_szflags[targid] &= ~SZ_BUSYTARG;	/* clear the flag */

    /* Clear all associated states and flags for this target */

    sc->sc_xstate[targid] = SZ_NEXT;
    sc->sc_xevent[targid] = SZ_BEGIN;
    sc->sc_szflags[targid] = SZ_NORMAL;
    sc->sc_flags[targid] &= ~DEV_HARDERR;
    sc->sc_selstat[targid] = SZ_IDLE;

    PRINTD(targid, 0xff, 
       ("asc_busy_target: requeueing scsi target %d after BUSY status\n",
		targid));

    /* If the SCSI bus is not busy then restart this target, the b_active
    flag is cleared.  The start routine will be able to act on this queue. */

    s = splbio();
    dp->b_active = 0;			/* set to non-active */

    if (sc->sc_active == 0)
	sz_start(sc, targid);

    splx(s);

}	/* end asc_busy_target */

/******************************************************************
 *
 * Name:	asc_encode_busid
 *
 * Abstract:	Take a busid as passed on the SCSI bus and as stored
 *		in sc_active (where the bit position is the id, 
 *		e.g., 0x01 = id 0, 0x02 = id 1, etc.) and convert
 *		to the intended decimal representation.
 *
 * Inputs:
 * 
 *	ubid	unencoded bus id from SCSI bus
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/

asc_encode_busid(ubid)

int	ubid;		/* Unencoded Bus ID */
{		
int i;		
                
for (i = 0; i < NDPS; i++)		
    if (ubid & (1 << i))			
	return(i);			
	                                
cprintf("asc_encode_busid: invalid unencoded id = 0x%x\n", ubid);	
				    
}	/* end asc_encode_busid */


#ifdef ASCDEBUG
/******************************************************************
 *
 * Name:	asc_show_regs
 *
 * Abstract:	Print out the ASC status, interrupt status, and
 *		sequence step registers, and the last command issued.
 *
 * Inputs:
 * 
 *	sc	The softc structure.
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/

asc_show_regs(sc, cntlr, targid)
register struct sz_softc *sc;
int	cntlr;
int	targid;
{

cprintf("\nASC REGS: c=%x, t=%x SR %x (", cntlr, targid, sc->sc_asc_sr);

if (sc->sc_asc_sr & ASC_INTP)	cprintf("INTP ");
if (sc->sc_asc_sr & ASC_TC)	cprintf("TC ");
if (sc->sc_asc_sr & ASC_PE)	cprintf("PE ");
if (sc->sc_asc_sr & ASC_GE)	cprintf("GE");

cprintf(" ");

/* Only print the phase if the Bus Service bit is set */

if (sc->sc_asc_isr & ASC_BS)
  switch(PHASE(sc->sc_asc_sr))
    {
    case SCS_DATAO:	cprintf("DATA OUT");
                        break;

    case SCS_DATAI:	cprintf("DATA IN");
                        break;

    case SCS_CMD:    	cprintf("CMD");
                        break;

    case SCS_STATUS:    cprintf("STATUS");
                        break;

    case SCS_MESSO:     cprintf("MSG OUT");
                        break;

    case SCS_MESSI:     cprintf("MSG IN");
                        break;

    default:            cprintf("BAD PHASE");
                        break;
    
}	/* end switch */
      
cprintf(") ISR %x (", sc->sc_asc_isr);

if (sc->sc_asc_isr & ASC_SEL)	cprintf("SEL ");
if (sc->sc_asc_isr & ASC_SATN)	cprintf("SATN ");
if (sc->sc_asc_isr & ASC_RSEL)	cprintf("RSEL ");
if (sc->sc_asc_isr & ASC_FC)	cprintf("FC ");
if (sc->sc_asc_isr & ASC_BS)	cprintf("BS ");
if (sc->sc_asc_isr & ASC_DIS)	cprintf("DIS ");
if (sc->sc_asc_isr & ASC_ILLCMD)	cprintf("ILLCMD ");
if (sc->sc_asc_isr & ASC_SCSIRST)	cprintf("SCSIRST");

cprintf(") SSR %x ASCCMD %x", sc->sc_asc_ssr, sc->sc_asccmd);
cprintf("\n");

}	/* end asc_show_regs */

/******************************************************************
 *
 * Name:	asc_dumpsc
 *
 * Abstract:	Dump selected fields from the softc.
 *
 * Inputs:
 * 
 *	sc	softc
 *    targid	the target id
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/

asc_dumpsc(sc, targid)

register struct sz_softc *sc;
int	targid;

{
cprintf("asc_dumpsc: ascsentsync = 0x%x\n", sc->sc_ascsentsync[targid]);
DELAY(10000);
cprintf("asc_dumpsc: ascdmacount = 0x%x\n", sc->sc_ascdmacount[targid]);
DELAY(10000);
cprintf("asc_dumpsc: szflags = 0x%x\n", sc->sc_szflags[targid]);
DELAY(10000);
cprintf("asc_dumpsc: sc_b_bcount = 0x%x\n", sc->sc_b_bcount[targid]);
DELAY(10000);
cprintf("asc_dumpsc: sc_bpcount = 0x%x\n", sc->sc_bpcount[targid]);
DELAY(10000);
cprintf("asc_dumpsc: sc_segcnt = 0x%x\n", sc->sc_segcnt[targid]);
DELAY(10000);
cprintf("asc_dumpsc: sc_savcnt = 0x%x\n", sc->sc_savcnt[targid]);
DELAY(10000);
cprintf("asc_dumpsc: sz_opcode = 0x%x\n", sc->sz_opcode);
DELAY(10000);
cprintf("asc_dumpsc: ascsyncper = 0x%x\n", sc->sc_ascsyncper[targid]);
DELAY(10000);
cprintf("asc_dumpsc: ascreqack = 0x%x\n", sc->sc_ascreqack[targid]);
DELAY(10000);
cprintf("asc_dumpsc: iodir = 0x%x\n", sc->sc_iodir[targid]);
DELAY(10000);

}	/* end asc_dumpsc */

/******************************************************************
 *
 * Name:	asc_print_sense
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

asc_print_sense(sc, targid)

register struct sz_softc *sc;
int targid;
{
    u_char *byteptr;
    int i;

    byteptr = (u_char *)&sc->sc_sns[targid];

    cprintf("\nasc_print_sense: (targid=%d) ", targid);

    for (i = 0; i < SZ_RQSNS_LEN; i++)
	cprintf("%x ", *(byteptr + i));

    switch (sc->sc_sns[targid].snskey) {

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

}	/* end asc_print_sense */

#ifdef ASC_TEST_LOG

/******************************************************************
 *
 * Name:	asc_test_log
 *
 * Abstract:	Allows fake errors to be logged for testing logging.
 *
 * Inputs:	sc - softc pointer
 *		targid
 *
 * Outputs:	None.
 *
 * Return values: None.
 ******************************************************************/

asc_test_log(sc, targid)

register struct sz_softc *sc;
int	targid;

{

    if (!asc_tested_log) {
	int flags;

	cprintf("asc: testing error log, please ignore next messages\n");

	flags = SZ_HARDERR | SZ_LOGMSG;
	scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x73, 0, flags);

	flags = SZ_HARDERR | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 0, 0, flags);

	scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 0, 0, SZ_HARDERR);

	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 0, 0, flags);

	scsi_logerr(sc, 0, -1, SZ_ET_RSTBUS, 2, 0, SZ_HARDERR);

	asc_tested_log = 1;

	cprintf("asc: end of ignorable error log messages - debug on\n");

    }	/* end if asc_tested_log */

}	/* end asc_test_log */
#endif ASC_TEST_LOG

#endif ASCDEBUG


