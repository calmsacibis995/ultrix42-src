/*
#ifndef lint
static char 	*sccsid = "@(#)scsi_kzq.c	4.2 (ULTRIX)		4/30/91";
#endif lint
/*

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * scsi_kzq.c	09/07/90
 *
 * PVAX/FIREFOX/PMAX SCSI KZQSA device driver (SII/Qbus routines)
 *
 * Modification history:
 *
 *  4/24/91	Brian Nadeau / Richard Napolitano
 *	Add get_validbuf_kzq() and change all references to
 *      get_validbuf() to use the new routine.
 *	Also - merge 10 byte cdb support and rrd42 work-around.
 *
 * 09/07/90     Charles M. Richmond	(IIS Corp)
 *	Modified existing scsi_sii.c file for the KZQSA board. These
 *	changes encompass the lack of padding in the kzqsa/sii regs
 *	and the differences in qbus/ibus implementations. EG the
 *	different calls for probe. For previous history see the 
 *	scsi_sii.c file.
 *
 ***********************************************************************/

#include "../data/scsi_data.c"

/*
 * Define ELDEBUG to create error for uerf debuging.
 * NOTE: must not be defined at submit time (performance hit).
 */
/*#define	ELDEBUG	*/

#include "scsi_debug.h"
int kzqdebug = 0;
int kzqtarget = 0;
int sii_rse_flag;

#ifdef vax
extern char cvqmsi[] [512*NBPG];
extern char cvqmsirb[];
#endif vax
extern char szbufmap[];
extern short sz_timetable[];
extern int cpu;

int kzq_wait_after_inquiry = 1000;

/* For quick hacks at debugging. */
#undef DEBUG

int kzq_busy_target();			/* for forware reference */
int bzero(), bcopy();

/******************************************************************
 *
 * Probe routine for KZQ.
 *
 ******************************************************************/
short kzq_reject_message = 0;
short kzq_assert_attn = 0;		/* Assert Attention Flag    */

/*
 * Number of seconds to wait for SCSI devices
 * to come ready enough to respond to inquiry
 * after bus reset. Needs to be a variable so
 * users can change it (I am such a nice guy).
 * Factory default is 7 seconds (in scsi_data.c).
 * Note on note... I think that the above is Fred... cmr
 */
extern int sz_wait_for_devices;
extern int sz_max_wait_for_devices;

extern int kzq_scsistart();
extern int kzq_reset();

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
 * Name:	kzq_probe
 *
 * Abstract:	The KZQ probe entry point routine from auto-configure
 *		code. Determine if the KZQ controller exists. Reset the
 *		SCSI bus. Size the number and type of targets on the
 *		SCSI bus. Set up the "sz_softc" structure and the 128K
 *		hardware buffer.
 *
 * Inputs:
 * reg		SCSI bus controller CSR address.
 * cntlr	SCSI bus controller number.
 *
 * Outputs:
 * sz_softc
 *
 *  sc_sysid		SCSI bus ID of initiator (CPU).
 *  *port_start()	SCSI command start routine - kzq_scsistart().
 *  *port_reset()	SCSI bus reset - kzq_reset().
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
short kzq_sent_cmd = 0;			/* SCSI Command Sent flag    */
short kzq_wait_count = KZQ_WAIT_COUNT;	/* Delay count for SII chip */
short kzq_use_programmed_io = 0;	/* Programmed IO Mode flag  */
short kzq_firstcall = 1;		/* First call to probe flag */
short kzq_debug_probe = 0;		/* Used for debugging probe */
short kzq_test = 0;			/* Used for testing purposes */

/*
 * The configuration code on the vax side needs to be modified
 * to pass a pointer to uba_ctlr instead on the cntlr number.
 */
kzq_probe(reg, cntlr)
caddr_t reg;
int cntlr;
{
	KZQ_REG *kzqaddr = (struct kzq_regs *) reg;
	register struct sz_softc *sc;
	int targid, unit;
	int dboff;
	int rz_slotsize, tz_slotsize, cz_slotsize;
	int ncz, ntz;
	int i, s, stat, found;
	struct sz_inq_dt *idp;
	struct sz_rdcap_dt *rdp;
	struct scsi_devtab *sdp;
	struct scsi_devtab *usdp;
	struct uba_ctlr *kzq_um;
	int sdp_match;
	char *p;
	int alive;
	int cnt;
	int retries, status;
	int save_kzqdebug;
	int save_kzqtarget;
	int kzq_intr();
	extern int stray();

#ifdef SZDEBUG
	/* Check if debug probe flag is set */
	if(kzq_debug_probe) {
	    save_kzqdebug = kzqdebug;
	    save_kzqtarget = kzqtarget;
	    kzqdebug = 0x1f;
	    kzqtarget = -1;
	}
	PRINTD(-1, 0x8, ("kzq_probe: start probing the SCSI bus\n"));
#endif SZDEBUG

	/*
	 * Probe must fail if controller not configured.
	 */
	alive = 1;
	if(cntlr >= nNKZQ)
	    alive = 0;

	/*
	 * Initialize certain fields in the softc structure
	 * and reset the SII chip.
	 */
	if(alive) {
	    kzq_stray_intr[cntlr] = 1;
	    sc = &sz_softc[cntlr];
	    sc->sc_siinum = cntlr;
	    sc->scsi_polled_mode = 1;
	    sc->scsi_bus_idle = 0;
	    sc->sc_sysid =
		    get_scsiid(cntlr);      /* init the host adapter ID */
	    sc->sc_active = 0;		    /* init the active flag */
	    sc->port_start = kzq_scsistart; /* init the port_start switch */
	    sc->port_reset = kzq_reset;	    /* init the port_reset switch */
	     
	    sc->rmbcopy = bcopy;
	    sc->wmbcopy = bcopy;
	    sc->wmbzero = bzero;
	    

	    /* The following reads the jumpers for the rambuffer 128k addr */
	    /* shifts the bits into place and converts to Qbus virtual by  */
	    /* adding in 'qmem'						   */
	    sc->sc_rambuff =(char *)(((kzqaddr->kzq_vector & 0xf800)<<6)
								+(int)qmem); 


	    sc->sc_scsiaddr = (caddr_t)reg; /* init the sii addres */
	    kzq_reset(sc); 		    /* reset the SII chip */
	    sc->sii_was_reset = 0;
	    
	    /* The following indexes through the ubminit array of 	*/
	    /* structs until it reaches the kzq controller or until it	*/
	    /* runs out of valid struct addresses			*/
	    found=0;
	    kzq_um = ubminit;
	    while((found == 0) && ((kzq_um++)->um_driver))
   	     {
   	     if (strcmp(kzq_um->um_ctlrname,"kzq")==0)
	          {
		found=1;
	      /* The following for loop finds the first vacant vector and */
	      /* loads it with the kzqsa interupt address			*/
	      for(cnt=QVEC_BCNT;cnt>=0;cnt--)
	        {
	        if(uba_hd[kzq_um->um_ubanum].uh_vec[cnt]== stray) /*first free*/
		  {
		  uba_hd[kzq_um->um_ubanum].uh_vec[cnt] = kzq_intr; /*address*/
		  kzqaddr->kzq_vector = cnt<<2;
		  cvec= cnt<<2;
		  break;
		  }
	        }	
   	      }
   	    }
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

		(sc->wmbzero)((char *)(sc->sc_rambuff+sc->sc_siidboff[targid]),
			256);	/* magic # !! check with siidboff alloc */

		sc->sc_siisentsync[targid] = 1;
		sz_bldpkt(sc, targid, SZ_INQ, 0, 0);
		stat = kzq_scsistart(sc, targid, 0);
		PRINTD(targid, 0x2,
		       ("kzq_probe: targid %d stat from scsistart=%x\n",
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
	    PRINTD(targid, 0x8, ("", kzq_print_inq_info(idp)));
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
		printf("kzq_probe: scsi %d targetID %d: %s (%d).\n",
		       cntlr, targid, "unknown peripheral device type",
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
		    printf("kzq_probe: scsi %d targetID %d: %s\n",
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
		if ((sdp->devtype == RX23) || (sdp->devtype == RX33)) {
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
		    kzq_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_STARTUNIT) {
		    /*
		     * Send two start unit commands because a pending unit
		     * attention may cause the first one to fail. We don't
		     * for the drive to spin up here (happens in rzopen).
		     */
		    sc->sc_curcmd[targid] = SZ_P_SSUNIT;
		    sz_bldpkt(sc, targid, SZ_P_SSUNIT, 0, 1);
		    kzq_scsistart(sc, targid, 0);
		    kzq_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_TESTUNITREADY) {
		    sc->sc_curcmd[targid] = SZ_TUR;
		    sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
		    kzq_scsistart(sc, targid, 0);
		}
		if (sdp->flags & SCSI_READCAPACITY) {
		    sc->sc_curcmd[targid] = SZ_RDCAP;
		    sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
		    kzq_scsistart(sc, targid, 0);
		}
		if (sdp->probedelay > 0)
		    DELAY(sdp->probedelay);

		if (sdp->flags & SCSI_NODIAG)
		    sz_unit_rcvdiag[(cntlr*NDPS)+targid] = 1;

		break;
	    }		/* end of switch */
	    /*
	     * Just to be sure the bus is free after inquiry.
	     * RRD40 may hold bus for a while.
	     */
	    DELAY(kzq_wait_after_inquiry);
	}		/* end of for loop */
	/*
	 * Clean out any left over interrupts.
	 */
	if (alive) {
		kzqaddr->sii_cstat = kzqaddr->sii_cstat;
		kzqaddr->sii_dstat = kzqaddr->sii_dstat;
		kzqaddr->sii_csr = ( SII_RSE | SII_SLE | SII_PCE | SII_IE);
#ifdef mips
    		wbflush();
#endif mips
	}

	/*
	 * TODO: should use map to allocate 128KB buffer
	 *
	 * If last (really 2nd) call to kzq_probe,
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
	if ((nNKZQ == 1) || (kzq_firstcall == 0)) {
	    dboff = 0x0;
	    /*
             * Setup 128 byte ram buffer slots for each target to
	     * be used for non READ/WRITE DMA Transfers on the SII.
	     */
	    for(i=0; i<NDPS; i++) {
		sc->sc_siidboff[i] = dboff;
		dboff += 128;
		}
	    dboff = 1024;

	    /* determine rz slot size, must be > 16kb */
	    cz_slotsize = tz_slotsize = rz_slotsize = 16 * 1024;
	    for (cntlr=0; cntlr<nNKZQ; cntlr++) {
		sc = &sz_softc[cntlr];
		for (targid=0; targid<NDPS; targid++) {
		    if (targid == sc->sc_sysid)
			continue;
		    if (sc->sc_alive[targid] == 0)
			continue;
		    sc->sc_dboff[targid][0] = dboff;
		    sc->sc_dboff[targid][1] = dboff+KZQ_MAX_DMA_XFER_LENGTH;
		    sc->sc_segcnt[targid] = 64 * 1024; /* Get rid of in freds code */
		    dboff += rz_slotsize;
		    PRINTD(targid, 0x20,
			("kzq_probe: cntlr=%d targid=%d devtype=%x ", cntlr,
			targid, sc->sc_devtyp[targid]));
		    PRINTD(targid, 0x20, ("req/ack=%d slotsize=%d\n",
			     sc->sc_siireqack[targid], sc->sc_segcnt[targid]));
		}
	    }
	}
	if (kzq_firstcall)
	    kzq_firstcall = 0;

#ifdef SZDEBUG
	PRINTD(-1, 0x8, ("kzq_probe: done probing the SCSI bus\n"));
	if(kzq_debug_probe) {
	    kzqdebug = save_kzqdebug;
	    kzqtarget = save_kzqtarget;
	}
#endif SZDEBUG


#ifdef mips
   wbflush();
#endif mips
   

return(alive);
}

/******************************************************************
 *
 * Start a SCSI operation on the SII chip.
 *
 ******************************************************************/
kzq_scsistart(sc, targid, bp)
register struct sz_softc *sc;
int targid;
register struct buf *bp;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
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
    kzqaddr->sii_csr |= SII_SLE;
#ifdef mips
    wbflush();
#endif mips
    if(bp == (struct buf *)0) {
        kzqaddr->sii_csr &= ~(SII_RSE | SII_IE);
	sc->scsi_polled_mode = 1;
    }
    else
	{
	 /* Have to clear out the rambuffer area.  It is possible for a target
	    to not not return all the requested bytes.  We have to make sure
	    that at least the extra bytes are cleared. */

	    if(sc->sc_rzspecial[targid]) {
		struct mode_sel_sns_params *msp;

	      /* Some risk here using get_validbuf_kzq() w/out any error checking.
		It is probably low, at this point the target should not be
		doing anything, and ready for the upcomming command.  Note:
		this should be moved into the DMA level code. */
		
		msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
		if(sc->sc_actcmd[targid] == SZ_MODSNS)
		{
		    stv = sc->sc_rambuff +
			(sc->sc_dboff[targid][get_validbuf_kzq(sc, targid)] );
		    (sc->wmbzero)(stv, msp->msp_length);
		}
	    }
        kzqaddr->sii_csr |= ( SII_RSE | SII_IE);
        sc->scsi_polled_mode = 0;
    }
#ifdef mips
    wbflush();
#endif mips

    /* Perform target arbitration and selection */
    if((retval = kzq_select_target(sc, targid)) != SZ_SUCCESS) {
	return(retval);
    }

BEGIN_LOOP:
    /* Loop through all bus phases until command complete */
    sc->scsi_completed[targid] = 0;
    sc->scsi_bus_idle = 0;
    cstat = kzqaddr->sii_cstat;
    dstat = kzqaddr->sii_dstat;
    do {
/*
XPRINTF(XPR_NFS, "B: %x", sc->sc_siidmacount[targid],0,0,0);
cprintf("B: %x %x", sc->sc_siidmacount[targid],bp,0,0);
*/
	if(cstat & (SII_CI|SII_DI)) {

        /* Check for a BUS ERROR */
        if(cstat & SII_BER) {
  	    kzqaddr->sii_cstat = SII_BER;
#ifdef mips
            wbflush();
#endif mips
	}

        /* Check for a PARITY ERROR */
	if(dstat & SII_IPE) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 0, 0, flags);
	    PRINTD(targid, 0x10, ("kzq_scsistart: scsi %d parity error\n",
		cntlr));
	    goto HANDLE_ABORT;
	}

	/* Check for a BUS RESET */
	if(cstat & SII_RST_ONBUS) {
	    kzqaddr->sii_cstat = SII_RST_ONBUS;
#ifdef mips
            wbflush();
#endif mips
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 0, 0, SZ_HARDERR);
	    PRINTD(targid, 0x10, ("kzq_scsistart: scsi %d bus reset\n", cntlr));
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
		    ("kzq_scsistart: target %d BUSY rtn SZ_IP\n", targid ));
		return(SZ_IP);
	    }

	    kzqaddr->sii_cstat = SII_SCH;	/* clear the intr */
#ifdef mips
	    wbflush();				/* wait for write buffers */
#endif mips
            if(kzq_state_change(sc, &targid) != SZ_SUCCESS)
                goto HANDLE_ABORT;
	}

	/* If disconnected and went to BUS FREE STATE then break */
	if(sc->scsi_bus_idle)
	    break;

        /* Check for a PHASE MISMATCH */
        if(dstat & SII_MIS) {

            /* Check for a BUS ERROR */
            if(cstat & SII_BER) {
  	        kzqaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* Always clear DID DMA flag on a phase change */
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

            /* Handle the current bus phase */
            if(kzq_phase_change(sc, dstat) != SZ_SUCCESS)
                goto HANDLE_ABORT;

            /* Wait for the next bus phase */
            if(!sc->scsi_completed[targid] && 
		  !(sc->sc_szflags[targid] & (SZ_WAS_DISCON|SZ_DID_DMA))) {
		timer = 1000;
		while(--timer && !(kzqaddr->sii_dstat & SII_MIS));
    		dstat = kzqaddr->sii_dstat;
		if(!sc->scsi_polled_mode && (timer == 0) && 
			!(dstat & (SII_CI|SII_DI)) && 
				(sc->sc_actcmd[targid] != SZ_RQSNS)) {
                    PRINTD(targid, 0x4,
			("kzq_scsistart: SII_MIS didn't set rtn SZ_IP\n"));
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
	    if(!kzq_restartdma(sc))
		goto HANDLE_ABORT;
	}

	/* Sometimes the target stays in the same phase */
	if((dstat & (SII_IBF|SII_TBE)) && 
		!(dstat & SII_MIS) &&
			((sc->sc_fstate != SZ_DATAI_PHA) &&
				(sc->sc_fstate != SZ_DATAO_PHA))) {

            /* Check for a BUS ERROR */
            if(cstat & SII_BER) {
  	        kzqaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* Handle the current bus phase */
	    if(kzq_phase_change(sc, dstat) != SZ_SUCCESS)
	        goto HANDLE_ABORT;
	  }
	}
    dstat = kzqaddr->sii_dstat;
    cstat = kzqaddr->sii_cstat;
  
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
		("kzq_scsistart: COMMAND COMPLETED successfully\n"));
	    sc->scsi_completed[targid] = 0;
            sc->sc_active = 0;
	    if (sc->sc_szflags[targid] & SZ_BUSYTARG)
		{
	        return(SZ_IP);
		}
    	    if(sc->sc_status[targid] == SZ_GOOD)
		{
	        return(SZ_SUCCESS);	
		}
	    else
		{
	        return(SZ_RET_ERR);	
		}
        } 
	else if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
            PRINTD(targid, 0x4, 
		("kzq_scsistart: COMMAND IN PROGRESS disconnected\n"));
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
		("kzq_scsistart: COMMAND IN PROGRESS dma poll mode\n"));
            kzqaddr->sii_csr &= ~SII_IE;
#ifdef mips
            wbflush();
#endif mips
            SZWAIT_UNTIL((kzqaddr->sii_dstat & SII_DNE),kzq_wait_count,retval);
	    kzqaddr->sii_dstat = SII_DNE;
#ifdef mips
            wbflush();
#endif mips
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

	    /* Update the remaining dma count for this current transfer */
	    if(sc->sc_siidmacount[targid] > KZQ_MAX_DMA_XFER_LENGTH)
		{
		sc->sc_siidmacount[targid] -= 
			(KZQ_MAX_DMA_XFER_LENGTH - kzqaddr->sii_dmlotc);
		}
	    else
		sc->sc_siidmacount[targid] -= 
			(sc->sc_siidmacount[targid] - kzqaddr->sii_dmlotc);
	    if(retval >= kzq_wait_count)
	        goto HANDLE_ABORT;
	    else
	    	goto BEGIN_LOOP;
	}
	/* Wait for interrupt to signal DMA completion */
	else {
            PRINTD(targid, 0x4,
		("kzq_scsistart: COMMAND IN PROGRESS dma interrupt mode\n"));
	    return(SZ_IP);
   	}
    } else if(!sc->scsi_polled_mode)
	{
	return(SZ_IP); 	/* dont spin in here!!! */
	}

HANDLE_ABORT:
    /* Abort the current SCSI operation due to error */
    PRINTD(targid, 0x10,
	("kzq_scsistart: command aborted (bus=%d target=%d cmd=0x%x)\n",
	    cntlr, targid, sc->sc_curcmd[targid]));
    PRINTD(targid, 0x10, ("", kzq_dumpregs(cntlr, 0)));
	flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
	scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 0, 0, flags);
    kzq_reset(sc);
    kzqaddr->sii_cstat = 0xffff;
    kzqaddr->sii_dstat = 0xffff;
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
kzq_select_target(sc, targid)
register struct sz_softc *sc;
int targid;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int retval, i;
    int retries = 3;
    int sii_select_wait = 5000;

    /* Loop till retries exhausted */
    for(i = 0; i < retries; i++) {
      /* RDAT FIX
       * Determine if reselections are currently enabled.  If so,
       * disable them during the selection.
       */
      sii_rse_flag = kzqaddr->sii_csr & SII_RSE;
      if (sii_rse_flag) {
	   kzqaddr->sii_csr &= ~SII_RSE;
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

	if( (kzqaddr->sii_sc1 & (SII_SC1_BSY | SII_SC1_SEL)) !=0 )
	{
	    PRINTD(targid, 0x104,
		("kzq_select_target: Bus BUSY on select of ID %d\n",targid));

	    /* RDAT FIX
	     * Turn reselections back on before leaving.
	     */
	    if (sii_rse_flag) 
	    {
		 kzqaddr->sii_csr |= SII_RSE;
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
        kzqaddr->sii_slcsr = targid;
#ifdef mips
    	wbflush();
#endif mips
        kzqaddr->sii_comm = (SII_SELECT|SII_ATN);
        /* RDAT FIX
	 * It should be safe now to enable reselections.
	 */
        if (sii_rse_flag) {
	     kzqaddr->sii_csr |= SII_RSE;
	}
        PRINTD(targid, 0x104,
	    ("kzq_select_target: starting select of ID %d\n",targid));
#ifdef mips
        wbflush();
#endif mips
    
        /* Start timer to wait for a select to occur */
        SZWAIT_UNTIL((kzqaddr->sii_cstat & SII_SCH),sii_select_wait,retval);

	/* If a state change did occur then make sure we are connected */
	if((kzqaddr->sii_cstat & SII_SCH) && !(kzqaddr->sii_cstat & SII_CON))
            SZWAIT_UNTIL((kzqaddr->sii_cstat & SII_CON),kzq_wait_count,retval);

        /* Check for connection attempt */
        if(kzqaddr->sii_cstat & SII_CON) {
            /* Check for a Reselection Attempt and handle it in "kzq_intr" */
            if(kzqaddr->sii_cstat & SII_DST_ONBUS) {
	        targid = (kzqaddr->sii_destat & SII_IDMSK);
          	PRINTD(targid, 0x104,
		    ("kzq_select_target: reselect of ID %d in progress\n",
		    targid));
    	        return(SZ_BUSBUSY);
            }
            /* Check for a Selection Attempt and handle it here */
            else {
    	        kzqaddr->sii_cstat = SII_SCH;
#ifdef mips
                wbflush();
#endif mips
        	targid = (kzqaddr->sii_slcsr & SII_IDMSK);
                PRINTD(targid, 0x104,
		    ("kzq_select_target: target ID %d selected\n",targid));
                sc->sc_active = (1 << targid);
                sc->sc_selstat[targid] = SZ_SELECT;
		if(!sc->scsi_polled_mode) {
		    kzqaddr->sii_csr &= ~(SII_SLE | SII_RSE);
#ifdef mips
                    wbflush();
#endif mips
		}
	      /* Set the sii_dmctrl register to the rec/ack offset for the
		selected target.  The sii_dmctrl needes to be set if/when
		data phase is entered. */

		kzqaddr->sii_dmctrl = sc->sc_siireqack[targid];
#ifdef mips
                wbflush();
#endif mips
    	        return(SZ_SUCCESS);
            }
        }
	else
	{
            PRINTD(targid, 0x114,
		( "kzq_select_target: select of ID %d failed pass %d\n",
		targid, i ));
    	    /* 
	     * Selection timed out, clear necessary bus signals and
	     * abort the selection if the selection_in_progress bit
	     * (SII_SIP) is set. We abort the selection attempt by
	     * sending the DISCONNECT command. If the (SII_SIP) bit
	     * is not set then we most likely have a RESELECT from
	     * another target occuring.
	     */
	    if(kzqaddr->sii_cstat & SII_SIP) {
    	        kzqaddr->sii_cstat = SII_SCH;
    	        kzqaddr->sii_comm = SII_DISCON;
#ifdef mips
	        wbflush();
#endif mips
    	        SZWAIT_UNTIL((kzqaddr->sii_cstat & SII_SCH),sii_select_wait,
		    retval);
    	        kzqaddr->sii_cstat = SII_SCH;
    	        kzqaddr->sii_comm = 0;
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
kzq_getactive_target(sc)
register struct sz_softc *sc;
{

    int targid;

    for(targid=0; targid<NDPS; targid++)
	if(sc && sc->sc_active & (1 << targid))
	    {
	    return(targid);
	    }
    return(-1);
}

/******************************************************************
 *
 * Perform the DATA IN/DATA OUT PHASE on the SII chip.
 *
 ******************************************************************/
kzq_startdma(sc, iodir)
register struct sz_softc *sc;
int iodir;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
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
    int targid = kzq_getactive_target(sc);

    /*
     * Handle non READ/WRITE scsi commands that transfer data.
     */
    if((sc->sz_opcode != SZ_WRITE && sc->sz_opcode != SZ_READ) &&
      (sc->sz_opcode != SZ_WRITE_10) && (sc->sz_opcode != SZ_READ_10)) {
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
    	    PRINTD(targid, 0x20, ("kzq_startdma: mode select data:"));
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
	    PRINTD(targid, 0x10, ("kzq_startdma: unknown scsi cmd 0x%x\n",
		sc->sz_opcode));
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
	    if(kzq_senddata(sc, byteptr, datacnt, 0) != SZ_SUCCESS)
		{
		return(SZ_RET_ABORT);
		}
	}

	/* Get the data from the scsi bus */
        else {
	    if(kzq_recvdata(sc, byteptr, datacnt) != SZ_SUCCESS)
		{
		return(SZ_RET_ABORT);
		}
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

	kzqaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
#ifdef mips
        wbflush();
#endif mips
    	sc->sc_savcnt[targid] = 0;
	/* Handle the case of the DMA being disconnected */
	if(sc->sc_szflags[targid] & SZ_DMA_DISCON) {
	    sc->sc_szflags[targid] &= ~SZ_DMA_DISCON;
	    kzq_restartdma(sc);
	} else { /* Handle the case of the DMA just starting */
            /* Setup the dmacount and the offset into the RAM buffer */
	    sc->sc_siidmacount[targid] = sc->sc_bpcount[targid];
	    kzq_restartdma(sc);
	}
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Perform the COMMAND PHASE on the SII chip.
 *
 ******************************************************************/
kzq_sendcmd(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    u_char *byteptr;
    int datacnt, i;
    int cmd_type;
    int cmdcnt;
    int targid = kzq_getactive_target(sc);

    sc->sc_szflags[targid] = 0;
    sc->sc_savcnt[targid] = 0;
    sc->sc_status[targid] = 0xff;
    sc->sc_siidmacount[targid] = 0;
    byteptr = (u_char *)&sc->sz_command;
    cmd_type = *byteptr;

    /* Get the size of the scsi command */
    cmdcnt = 6;
    if(((cmd_type >> 5) & 0x7) == 1)
	cmdcnt = 10;

#ifdef SZDEBUG
    /* JAG make this a subroutine call with DEBUG */
    PRINTD(targid, 0x20, ("kzq_sendcmd: scsi cmd pkt:")); 
    for(i=0; i < cmdcnt; i++)
    {
	PRINTD(targid, 0x20, (" %x", *(byteptr+i)));
    }
    PRINTD(targid, 0x20, ("	( %s )\n", scsi_cmdtable[cmd_type]));
#endif SZDEBUG

    /* Put the scsi command onto the scsi bus */
    if(kzq_senddata(sc, byteptr, cmdcnt, 0) != SZ_SUCCESS)
	{
	return(SZ_RET_ABORT);
	}

    /* Statistics update for READS and WRITES */
    if((cmd_type == SZ_WRITE) || (cmd_type == SZ_READ) || 
       (cmd_type == SZ_WRITE_10) || (cmd_type == SZ_READ_10)) {
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
kzq_getstatus(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int targid = kzq_getactive_target(sc);

    /* Get the status byte from the scsi bus */
    if(kzq_recvdata(sc, &sc->sc_status[targid], 1) != SZ_SUCCESS)
	return(SZ_RET_ABORT);
    /* Save the status byte for the error log */
    if (sc->sc_curcmd[targid] == sc->sc_actcmd[targid])
	sc->sc_statlog[targid] = sc->sc_status[targid];

    PRINTD(targid, 0x24, ("kzq_getstatus: status byte = 0x%x = ",
   					 sc->sc_status[targid]));

    PRINTD(targid, 0x24, ("", kzq_print_status((int)sc->sc_status[targid])));


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
kzq_msgout(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
#ifdef vax
    u_char messg;
#endif vax
#ifdef mips
    u_long messg;
#endif mips
    int targid = kzq_getactive_target(sc);
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
	    ("kzq_msgout: sending NOP Message\n"));
#endif SZDEBUG

        /* Put the NOP Message onto the scsi bus */
        kzq_senddata(sc, &messg, 1, 0);
	return(SZ_SUCCESS);
    }

    /* Check if we need to send a Message Reject Message */
    if(kzq_reject_message) {
	kzq_reject_message = 0;
	messg = SZ_MSGREJ;

        PRINTD(targid, 0x4, ("kzq_msgout: sending Message Reject Message\n"));

        /* Put the Message Reject Message onto the scsi bus */
        if(kzq_senddata(sc, &messg, 1, 0) != SZ_SUCCESS)
	    {
	    return(SZ_RET_ABORT);
	    }
    }

    /* Send the Identify Message with or without disconnects */
    else {
    /* Clear the "kzq_assert_attn" flag */
    kzq_assert_attn = 0;
	/* Setup for disconnects or no disconnects */
        if(sc->scsi_polled_mode)
            messg = SZ_ID_NODIS | lun;	/* Allow no disconnects */
        else
            messg = SZ_ID_DIS | lun;  	/* Allow disconnects */

        /* Check if we need to send a Synchronous DataXfer Message */
	if(!sc->sc_siisentsync[targid]) {
	    sc->sc_siisentsync[targid] = 1;

	    PRINTD(targid, 0x4,
		("kzq_msgout: sending Identify Message = 0x%x\n",messg));

            /* Put the Identify Message onto the scsi bus */
            if(kzq_senddata(sc, &messg, 1, SII_ATN) != SZ_SUCCESS)
		{
	        return(SZ_RET_ABORT);
		}
	    PRINTD(targid, 0x4,
		("kzq_msgout: sending Sync Data Transfer Message\n"));

            /* Put the Synchronous Data Xfer Message onto the scsi bus */
            sc->sc_extmessg[targid][0] = SZ_EXTMSG;
            sc->sc_extmessg[targid][1] = 0x3;
            sc->sc_extmessg[targid][2] = SZ_SYNC_XFER;
            sc->sc_extmessg[targid][3] = 63; /* 25, 12 is fast! */
            sc->sc_extmessg[targid][4] = SII_SYNC;


	    { int i;
	    for(i=0; i < 4; i++)
            if(kzq_senddata(sc, &sc->sc_extmessg[targid][i], 1, SII_ATN) 
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
            if(kzq_senddata(sc, &sc->sc_extmessg[targid][4], 1, 0) 
				!= SZ_SUCCESS) {
		/* For all those weird devices, can't do SYNC */
		sc->sc_siireqack[targid] = 0;
	        return(SZ_SUCCESS);
	    }
        }
	else {

	    PRINTD(targid, 0x4,
		("kzq_msgout: sending default Identify Msg = 0x%x\n",messg));

            /* Put the Identify Message onto the scsi bus */
            messg = SZ_ID_NODIS | lun;	/* Allow no disconnects */
            if(kzq_senddata(sc, &messg, 1, 0) != SZ_SUCCESS)
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

kzq_msgin(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int len, i;
    int retval;
    int targid = kzq_getactive_target(sc);
    u_short olddmlotc = kzqaddr->sii_dmlotc;
    int flags;

    /* Get the message from the scsi bus */
    if(kzq_recvdata(sc, &sc->sc_message[targid], 1) != SZ_SUCCESS)
	return(SZ_RET_ABORT);

#ifdef	ELDEBUG
    if ((cntlr == 0) && (targid == sz_eldb_buserr73)) {
	    sc->sc_message[targid] = 0x5;	/* SZ_ID initiator detected error */
	sz_eldb_buserr73 = -1;
    }
#endif	ELDEBUG
    /* Switch on the type of message received */
    switch(sc->sc_message[targid]) {
    case SZ_CMDCPT:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_CMDCPT message\n"));
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
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_SDP message\n"));
	    sc->sc_savcnt[targid] = olddmlotc;
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;

	sc->sc_dboff_len[targid][sc->sc_actbp[targid]] -= sc->sc_savcnt[targid];
	kzqaddr->sii_dmlotc = 0;

            /* Read the disconnect message now */
            if(kzq_recvdata(sc, &sc->sc_message[targid], 1) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);
	    if(sc->sc_message[targid] != SZ_DISCON)
		break;
	/* FALL THROUGH */

    case SZ_DISCON:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_DISCON message\n"));
	    sc->sc_szflags[targid] |= SZ_WAS_DISCON;
	    kzqaddr->sii_dmlotc = 0;
	    if(sc->sc_siidmacount[targid] !=0 )
	        sc->sc_szflags[targid] |= SZ_DMA_DISCON;
	    break;
		
    case SZ_EXTMSG:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_EXTMSG message\n"));
	    sc->sc_extmessg[targid][0] = sc->sc_message[targid];

            /* Read the extended message length */
            if(kzq_recvdata(sc, &sc->sc_extmessg[targid][1], 1) != SZ_SUCCESS)
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
		    kzqaddr->sii_comm |= SII_ATN;
	    }

            /* Read the extended message */
            if(kzq_recvdata(sc, &sc->sc_extmessg[targid][2], len) != SZ_SUCCESS)
	        return(SZ_RET_ABORT);
#ifdef SZDEBUG
	    /* JAG make a subroutine w/DEBUG */
	    PRINTD(targid, 0x4, ("kzq_msgin: extended message:")); 
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
		kzqaddr->sii_dmctrl = sc->sc_siireqack[targid];
	    }
	    if(sc->sc_alive[targid] == 0) { /* PROBING */
		u_long messg = SZ_MSGREJ;


	    	kzq_assert_attn = 1;
	    	kzq_reject_message = 0;
        	kzq_senddata(sc, &messg, 1, SII_ATN);

	    }
	    break;

    case SZ_ID_NODIS:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_ID_NODIS message\n"));
	    break;

    case SZ_ID_DIS:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_ID_DIS message\n"));
	    break;

	case SZ_RDP:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_RDP message\n"));
	    break;

	case SZ_MSGREJ:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_MSGREJ message\n"));
	    break;

	case SZ_LNKCMP:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_LNKCMP message\n"));
	    break;

	case SZ_LNKCMPF:
	    PRINTD(targid, 0x4, ("kzq_msgin: SZ_LNKCMPF message\n"));
	    break;

    default:
	    flags = SZ_HARDERR | SZ_LOGMSG;
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x73, 0, flags);
	    PRINTD(targid, 0x4, ("kzq_msgin: unknown message = 0x%x\n",
		sc->sc_message[targid]));
	    return(SZ_RET_ABORT);
    }

    /*
     * Assert attention as long as the "kzq_assert_attn" flag is
     * set. Attention gets deasserted during a message out phase
     * and the "kzq_assert_attn" flags gets cleared.
     */
    if(kzq_assert_attn) {
	kzqaddr->sii_comm |= SII_ATN;
        }
#ifdef mips
    wbflush();
#endif mips
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Send data to the scsi bus.
 *
 ******************************************************************/
kzq_senddata(sc, data, count, attn)
register struct sz_softc *sc;
u_char *data;
int count;
int attn;
{
    int cnltr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int retval;
    int tmp_state;
    int tmp_phase;
    int i;
    int targid = kzq_getactive_target(sc);

    /* Move the SII to the new phase */
    tmp_phase = kzqaddr->sii_dstat & SII_PHA_MSK;
    tmp_state = kzqaddr->sii_cstat & SII_STATE_MSK;
    kzqaddr->sii_comm = (tmp_state | tmp_phase | attn);
#ifdef mips
    wbflush();
#endif mips

    /* Send the data to the scsi bus using programmed IO */
    if(kzq_use_programmed_io && (sc->sc_siireqack[targid] == 0)) {
        for(i=0; i<count; i++) {
	SZWAIT_UNTIL((kzqaddr->sii_dstat & SII_TBE),kzq_wait_count,retval);
	if (retval >= kzq_wait_count) {
	    scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4a, 0, SZ_HARDERR);
	    PRINTD(targid, 0x10, ("kzq_senddata: SII_TBE not set\n"));
	    return(SZ_RET_ABORT);
	}
	kzqaddr->sii_data = *data++;
	tmp_phase = kzqaddr->sii_dstat & SII_PHA_MSK;
	tmp_state = kzqaddr->sii_cstat & SII_STATE_MSK;
	kzqaddr->sii_comm = (SII_INXFER | tmp_state | tmp_phase);
#ifdef mips
        wbflush();
#endif mips
    }
    kzqaddr->sii_dstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
    return(SZ_SUCCESS);
    }

    /* Send the data to the scsi bus using DMA */
    else {
	u_short aligndata;

#ifdef mips
       if((u_long)data & 0x1) {
	aligndata = (u_short)(*data & 0x00ff);
       (sc->wmbcopy)(&aligndata,(sc->sc_rambuff+sc->sc_siidboff[targid]),count);
	} 
	else
	{
    	(sc->wmbcopy)(data, (sc->sc_rambuff + sc->sc_siidboff[targid]), count);
	}
	kzqaddr->sii_dmaddrl = ((sc->sc_siidboff[targid]) & 0x0ffff);
	kzqaddr->sii_dmaddrh = ((((sc->sc_siidboff[targid]) & 0x0fffffff))>>16);

wbflush();
#endif mips
    	kzqaddr->sii_dmlotc = count;
        kzqaddr->sii_comm = 
			(SII_DMA | SII_INXFER | tmp_state | tmp_phase | attn);
#ifdef mips
        wbflush();
#endif mips

	/* Wait for DMA to complete */
        SZWAIT_UNTIL((kzqaddr->sii_dstat & SII_DNE),kzq_wait_count,retval);
        kzqaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
	kzqaddr->sii_dstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
	if(retval >= kzq_wait_count) {
	    return(SZ_RET_ABORT);
	}
	return(SZ_SUCCESS);
    }
}

/******************************************************************
 *
 * Receive data from the scsi bus.
 *
 ******************************************************************/

#ifdef	ELDEBUG
/* Set ID of target to cause buserr 0x49 (bus 0 only) */
/* DOES NOT WORK because programmed I/O is never used in kzq_recvdata() */
int	sz_eldb_buserr49 = -1;
#endif	ELDEBUG

kzq_recvdata(sc, data, count)
register struct sz_softc *sc;
u_char *data;
int count;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int retval;
    int tmp_state;
    int tmp_phase;
    int i;
    int targid = kzq_getactive_target(sc);



    /* Move the SII to the new phase */
    tmp_phase = kzqaddr->sii_dstat & SII_PHA_MSK;
    tmp_state = kzqaddr->sii_cstat & SII_STATE_MSK;
    kzqaddr->sii_comm = (tmp_state | tmp_phase);
#ifdef mips
    wbflush();
#endif mips

    /* Recieve the data from the scsi bus using programmed IO */
    if(kzq_use_programmed_io && (sc->sc_siireqack[targid] == 0)) {
        for(i=0; i<count; i++) {
	    SZWAIT_UNTIL((kzqaddr->sii_dstat & (SII_IBF|SII_MIS)),
		kzq_wait_count, retval);
#ifdef	ELDEBUG
	    if ((sc->sc_curcmd[targid] == SZ_READ) && (cntlr == 0) &&
		(targid == sz_eldb_buserr49)) {
		retval = kzq_wait_count + 1;
		sz_eldb_buserr49 = -1;
	    }
#endif	ELDEBUG
	    if (retval >= kzq_wait_count) {
		scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x49, 0, SZ_HARDERR);
		PRINTD(targid, 0x10, ("kzq_recvdata: SII_IBF not set\n"));
		return(SZ_RET_ABORT);
	    }

	    /* If a phase change occured then we are done */
	    if(kzqaddr->sii_dstat & SII_MIS)
		break;
	    *data++ = kzqaddr->sii_data;
	    tmp_phase = kzqaddr->sii_dstat & SII_PHA_MSK;
	    tmp_state = kzqaddr->sii_cstat & SII_STATE_MSK;
	    kzqaddr->sii_comm = (SII_INXFER | tmp_state | tmp_phase);
#ifdef mips
	    wbflush();
#endif mips
	}
	kzqaddr->sii_comm &= ~SII_INXFER;
	kzqaddr->sii_dstat = SII_DNE;
#ifdef mips
	wbflush();
#endif mips
	return(SZ_SUCCESS);
    }

    /* Recieve the data from the scsi bus using DMA */
    else {
#ifdef vax
    	kzqaddr->sii_dmaddrl = (sc->sc_siidboff[targid] & 0xffff);
    	kzqaddr->sii_dmaddrh = (sc->sc_siidboff[targid] >> 16);
#endif vax
#ifdef mips
    	kzqaddr->sii_dmaddrl = ((sc->sc_siidboff[targid]) & 0x0ffffff);
    	kzqaddr->sii_dmaddrh = ((((sc->sc_siidboff[targid]) & 0x0fffffff))>>16);
    wbflush();
    	kzqaddr->sii_dmlotc = count;
        kzqaddr->sii_comm = (SII_DMA | SII_INXFER | tmp_state | tmp_phase);
    wbflush();
#endif mips

      /* Wait for DMA to complete, MIS is also checked.  If the target changes
	phases, this loop will not have to time out. */

        SZWAIT_UNTIL((kzqaddr->sii_dstat & (SII_DNE|SII_MIS)), kzq_wait_count,
	    retval);
        kzqaddr->sii_comm &= ~(SII_INXFER | SII_DMA);	/* set DNE */
	if(retval >= kzq_wait_count && !(kzqaddr->sii_dstat & SII_DNE))
	{
	    i = 10000;			/* wait for DNE */
	    while(--i && ((kzqaddr->sii_dstat & SII_DNE) == 0))
		;
	    kzqaddr->sii_dstat = SII_DNE;
	}
	else
	    kzqaddr->sii_dstat = SII_DNE;
	kzqaddr->sii_dmlotc = 0;
#ifdef mips
    wbflush();
#endif mips
	if(retval >= kzq_wait_count && !(kzqaddr->sii_dstat & SII_MIS))
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
kzq_clear_discon_io_tasks(sc)
register struct sz_softc *sc;
{

    int targid;
    int unit;
    struct buf *dp, *bp;

    PRINTD(targid, 0x10, ("kzq_clear_discon_io_tasks: scanning IDs\n"));

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
	    ("kzq_clear_discon_io_tasks: clearing ID %d\n", targid));
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
kzq_reset(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int targid;

    /* Reset the SII chip. */
    kzqaddr->sii_comm = SII_CHRESET;
    /* SII is always ID 7, and set up max REQ/ACK offset. */
    kzqaddr->sii_id = SII_ID_IO | sc->sc_sysid; 
    /* Enable SII to drive SCSI bus. */
    kzqaddr->sii_dictrl = SII_PRE;
    /*
     * Assert SCSI bus reset for at least 25 Usec to clear the 
     * world. SII_RST is self clearing.
     */
    kzqaddr->sii_comm = SII_RST;
    DELAY(25);
    /*
     * Clear any pending interrupts from the reset.
     */
    kzqaddr->sii_cstat = kzqaddr->sii_cstat;
    kzqaddr->sii_dstat = kzqaddr->sii_dstat;
    /************************************************************/
    /*								*/
    /* Clear out KZQ DMA registers.				*/
    /************************************************************/
    kzqaddr->kzq_lbar = 0;
    kzqaddr->kzq_qbar = 0;
    kzqaddr->kzq_wc = 0;

    /*
     * Set up SII for SCSI parity checking,
     * Select Enable, Reselect Enable, and Interrupt Enable.
     */
    kzqaddr->sii_csr = ( SII_RSE |SII_SLE | SII_PCE | SII_IE);
    
    /*
     * Clear all Active and Disconnected IO requests.
     */
    kzq_clear_discon_io_tasks(sc);
    sc->sii_was_reset = 1;
    DELAY(sz_wait_for_devices * 1000000);
}

/******************************************************************
 *
 * Reset all the SII controllers (crash dump reset only).
 *
 ******************************************************************/
kzqreset()
{
#ifdef mips
    register struct sz_softc *sc = &sz_softc[0];
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
#endif mips

    /* Reset the SII chip. */
    kzqaddr->sii_comm = SII_CHRESET;
    /* SII is always ID 7, and set up max REQ/ACK offset. */
    kzqaddr->sii_id = SII_ID_IO | sc->sc_sysid; 
    /* Enable SII to drive SCSI bus. */
    kzqaddr->sii_dictrl = SII_PRE;
    /*
     * Assert SCSI bus reset for at least 25 Usec to clear the 
     * world. SII_RST is self clearing.
     */
    kzqaddr->sii_comm = SII_RST;
    DELAY(25);
    /*
     * Clear any pending interrupts from the reset.
     */
    kzqaddr->sii_cstat = kzqaddr->sii_cstat;
    kzqaddr->sii_dstat = kzqaddr->sii_dstat;
    /*
     * Set up SII for SCSI parity checking,
     * Select Enable, Reselect Enable, and Interrupt Enable.
     */
    kzqaddr->sii_csr = ( SII_RSE |SII_SLE | SII_PCE | SII_IE);
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

kzq_intr(cntlr)
int cntlr;
{
    register struct sz_softc *sc = &sz_softc[cntlr];
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int targid, retval, timer;
    int dmacount, offset, tmp_phase, tmp_state;
    int handle_reselect;
    u_short cstat, dstat;
    int flags;

#ifdef mips
    if(kzq_stray_intr[cntlr] != 1) { /* Stray interrupt on reboot */
    		/* Reset the SII chip. */
    		kzqaddr->sii_comm = SII_CHRESET;
                printf("kzq_intr: noprobe intr\n");
		return;
    }
#endif mips
    /* Initialize variables */
    targid = kzq_getactive_target(sc);
#ifdef vax
    sc->scsi_bus_idle = 0;
    sc->scsi_completed[targid] = 0;
#endif vax
    handle_reselect = 0;

    /*the toggling of the IE bit in the csr is to allow the latched IRQ */
    /* line to toggle. On the Ibus there is no latch, since it is based */
    /* on levels. On the Qbus the transition is necessary.		*/
    /* Thus the reset/set of the enable allows the latch to reflect	*/
    /* the new incoming state and thus to pass that to the SII IRQ	*/
    /* line. Without this toggle it is possible to miss an interrupt.	*/

    kzqaddr->sii_csr &= ~SII_IE;	/*reset/set IE to toggle kzq IRQ */

    cstat = kzqaddr->sii_cstat;
    dstat = kzqaddr->sii_dstat;

    kzqaddr->sii_csr |= SII_IE;		/* enable ints after grabbing regs */

    /* Check for interrupt from a disconnected target */
    if(targid == -1 || sc->sc_active == 0) {
	/* Check if there are valid interrupts pending */
	if(cstat & (SII_CI|SII_DI)) {

	    /* We must wait for a STATE CHANGE to occur first */
	    timer = 10000;
	    while(--timer && ((kzqaddr->sii_cstat & SII_SCH) == 0));
	    if(timer == 0) {
/*cprintf("timer expired in interrupt, cstat= %x, dstat= %x\n", cstat, dstat);*/
		/*
		 * Check if a reselect occurred without the STATE
		 * CHANGE bit (SII_SCH) being set. This condition
		 * can occur when a reselect immediately follows a
		 * disconnect and therefore we only get one STATE
		 * CHANGE interrupt.
		 */
		cstat = kzqaddr->sii_cstat;
		if((cstat & SII_DST_ONBUS) && 
				(cstat & SII_CON))
		    handle_reselect = 1;
		else {
		if(cstat & SII_RST_ONBUS) {
			kzqaddr->sii_cstat &= ~SII_IE;
			kzqaddr->sii_cstat = SII_RST_ONBUS;
			kzqaddr->sii_cstat = SII_SCH;
			kzqaddr->sii_cstat = SII_IE;
#ifdef mips
                        wbflush();
#endif mips
		}
	    		return;
		}
            }
        }
	/* No interrupts pending, spurious interrupt occurred */
	else {
	    PRINTD(0xFF, 0x1,
		("kzq_intr: spurious interrupt from inactive target\n"));
	    return;
	}
    }/*end of int from disconnected target.*/
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
	
XPRINTF(XPR_NFS, "intB: %x %x %x %x", sc->sc_siidmacount[targid],cstat,dstat,kzqaddr->sii_comm);
    offset = (sc->sc_bpcount[targid] - sc->sc_siidmacount[targid]);
	if(cstat & (SII_CI|SII_DI)) {


        /* Check for a BUS ERROR */
        if(cstat & SII_BER) {
  	    kzqaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	}

        /* Check for a PARITY ERROR */
        if(dstat & SII_IPE) {
	    flags = SZ_HARDERR | SZ_LOGREGS;
	    scsi_logerr(sc, 0, targid, SZ_ET_PARITY, 1, 0, flags);
	    PRINTD(targid, 0x10, ("kzq_intr: scsi %d parity error\n", cntlr));
	    goto HANDLE_ERROR;
        }

        /* Check for a BUS RESET */
        if(cstat & SII_RST_ONBUS) {
  	    kzqaddr->sii_cstat = SII_RST_ONBUS;
#ifdef mips
    wbflush();
#endif mips
	    scsi_logerr(sc, 0, -1, SZ_ET_BUSRST, 1, 0, SZ_HARDERR);
	    PRINTD(targid, 0x10, ("kzq_intr: scsi %d bus reset\n", cntlr));
	    goto HANDLE_ERROR;
        }

        /* Check for a STATE CHANGE */
        while((kzqaddr->sii_cstat & SII_SCH) || handle_reselect) { 
	    handle_reselect = 0;
	    PRINTD(targid, 0x4, ("kzq_intr: state change occurred\n"));
	    if(kzq_state_change(sc, &targid) != SZ_SUCCESS) {
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
		{
		    return;
		    }

		/*
		 * If a quick Reselect occurred from the same target
	         * that Disconnected then handle it now rather than
		 * wait for another interrupt.
		 */
	        if((cstat & SII_DST_ONBUS) &&
		   (cstat & SII_CON) &&
		   (kzqaddr->sii_destat & SII_IDMSK) == targid) {
		    PRINTD(targid, 0x4,("kzq_intr: quick reselect occurred\n"));
	    	    timer = 10000;
	    	    /* Wait for the STATE CHANGE bit to get set first */
	    	    while(--timer && !(kzqaddr->sii_cstat & SII_SCH));
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
	    kzqaddr->sii_dstat |= SII_DNE;   /* resetting the dne bit  CMR*/
#ifdef mips
    wbflush();
#endif mips
	    PRINTD(targid, 0x4,
		("kzq_intr: transfer done occurred, (dmlotc = %d)\n",
		kzqaddr->sii_dmlotc));
	    sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	    if(sc->sc_siidmacount[targid] > KZQ_MAX_DMA_XFER_LENGTH)
	      {
		sc->sc_siidmacount[targid] -= 
			(KZQ_MAX_DMA_XFER_LENGTH - (sc->sc_savcnt[targid] ?
			sc->sc_savcnt[targid] : kzqaddr->sii_dmlotc));
	      }
	    else
		{
		sc->sc_siidmacount[targid] -= 
			(sc->sc_siidmacount[targid] - (sc->sc_savcnt[targid] ?
			sc->sc_savcnt[targid] : kzqaddr->sii_dmlotc));
		}

	}

        /* Check for a PHASE MISMATCH */
        if(dstat & SII_MIS) {

            /* Check for a BUS ERROR */
            if(cstat & SII_BER) {
  	        kzqaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* If a transfer was in progress, but didn't generate */
	    /* a DNE because the SII was setup for a larger DMA   */
	    /* transfer than actually occurred, then clear the    */
	    /* transfer command and wait for the DNE interrupt.   */
	    if(kzqaddr->sii_comm & SII_INXFER) {
	        kzqaddr->sii_comm &= ~SII_INXFER;
	        timer = 10000;
	        while(--timer && ((kzqaddr->sii_dstat & SII_DNE) == 0));
#ifdef mips
    wbflush();
#endif mips
		kzqaddr->sii_dstat = SII_DNE;

#ifdef mips
    wbflush();
		dstat = kzqaddr->sii_dstat;
#endif mips
	/* Check for ODD BYTE BOUNDARY condition */
	if((dstat & SII_OBB)) {
		if(kzqaddr->sii_dmlotc) {
/*			cprintf("OBB %x %x\n", sc->sc_savcnt[targid], kzqaddr->sii_dmlotc); */
			sc->sc_siioddbyte[targid] = kzqaddr->sii_dmabyte;
			kzqaddr->sii_dmlotc++;
			wbflush();
		}/* else
			cprintf("OBB done %x %x %x\n", sc->sc_dboff_len[targid][sc->sc_actbp[targid]], dstat, kzqaddr->sii_comm); */
	}/*end of oddbyte */
	        /* Clear DMA in progress flag and adjust DMA count */
	        sc->sc_szflags[targid] &= ~SZ_DID_DMA;
	        if(sc->sc_siidmacount[targid] > KZQ_MAX_DMA_XFER_LENGTH)
		    sc->sc_siidmacount[targid] -= 
			    (KZQ_MAX_DMA_XFER_LENGTH - (sc->sc_savcnt[targid] ? sc->sc_savcnt[targid] : kzqaddr->sii_dmlotc));
	        else
		    sc->sc_siidmacount[targid] -= 
			    (sc->sc_siidmacount[targid] - (sc->sc_savcnt[targid] ? sc->sc_savcnt[targid] : kzqaddr->sii_dmlotc));

	    }

	    /* Handle the current bus phase */
	    if(kzq_phase_change(sc, dstat) != SZ_SUCCESS) {
/*		cprintf("intr: phase change error\n"); */
	        goto HANDLE_ERROR;
	    }


/* THIS CODE BELOW GOES IF WE WRITE NEW COMM */
            /* Wait for the next bus phase */
            if(!sc->scsi_completed[targid] && 
		  !(sc->sc_szflags[targid] & (SZ_WAS_DISCON|SZ_DID_DMA))) {
		goto done;
#ifdef out
                SZWAIT_UNTIL((kzqaddr->sii_dstat & SII_MIS),kzq_wait_count,retval);
		dstat = kzqaddr->sii_dstat;
                if((retval >= kzq_wait_count) &&
			!(kzqaddr->sii_dstat & SII_IBF)) {
		    /* JAG BAD ERROR remove later */
	            cprintf("kzq_intr: SII_MIS didn't set\n");
		    PRINTD(targid, 0x10, ("kzq_intr: SII_MIS didn't set\n"));
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
	    if(kzq_restartdma(sc)) {
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
				    (sc->sc_dboff[targid][oldact]), 
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
		cprintf("kzq_intr: DMA error in sii_restart: aborting\n");
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
  	        kzqaddr->sii_cstat = SII_BER;
#ifdef mips
    wbflush();
#endif mips
	    }

	    /* Handle the current bus phase */
	    if(kzq_phase_change(sc, dstat) != SZ_SUCCESS) {
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
			    (sc->sc_dboff[targid][oldact]), 
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
		timeout( kzq_busy_target, (caddr_t)sc->sc_unit[ targid ], 
		    SZ_BUSY_WAIT );		/* wait for the targit */
		sc->scsi_bus_idle = 0;
		PRINTD(targid, 0x4,
		    ("kzq_intr: COMMAND COMPLETED with BUSY\n"));
		sc->sc_active = 0;
		sc->scsi_completed[targid] = 0;
		sz_start( sc, -1 );		/* Start next I/O request */
		return;
	    }
	    else
	    {
		sc->scsi_bus_idle = 0;
		PRINTD(targid, 0x4,
		    ("kzq_intr: COMMAND COMPLETED successfully\n"));
		sc->sc_active = 0;
		sc->scsi_completed[targid] = 0;
		sz_start(sc, targid);		/* Finish current I/O request */
		return;
	    }
        }
	else if(sc->sc_szflags[targid] & SZ_WAS_DISCON) {
	    PRINTD(targid, 0x4,
		("kzq_intr: COMMAND IN PROGRESS disconnected\n"));
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
	    ("kzq_intr: COMMAND IN PROGESS dma interrupt mode\n"));
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
	("kzq_intr: command aborted (bus=%d target=%d cmd=0x%x)\n",
	cntlr, targid, sc->sc_curcmd[targid]));
    PRINTD(targid, 0x4, ( "", kzq_dumpregs(cntlr, 0)));

    flags = SZ_HARDERR | SZ_LOGCMD | SZ_LOGREGS;
    scsi_logerr(sc, 0, targid, SZ_ET_CMDABRTD, 1, 0, flags);
    kzq_reset(sc);
    sc->sc_selstat[targid] = SZ_IDLE;
    sc->sc_szflags[targid] |= SZ_ENCR_ERR;
    sc->sc_xstate[targid] = SZ_ERR;
    sc->sc_xevent[targid] = SZ_ABORT;
    kzqaddr->sii_cstat = 0xffff;
    kzqaddr->sii_dstat = 0xffff;
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
kzq_restartdma(sc)
register struct sz_softc *sc;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int dmacount, offset, which = -1;
    int tmp_state, tmp_phase;
    int targid = kzq_getactive_target(sc);
    
    kzqaddr->sii_comm &= ~(SII_INXFER | SII_DMA);
    kzqaddr->sii_cstat = SII_DNE;
#ifdef mips
    wbflush();
#endif mips
    offset = (sc->sc_bpcount[targid] - sc->sc_siidmacount[targid]);



    if(sc->sc_siidmacount[targid] > 0) {

            /* Calculate the DMA transfer length */
            dmacount = sc->sc_siidmacount[targid];
    	    if(dmacount > KZQ_MAX_DMA_XFER_LENGTH)
                dmacount = KZQ_MAX_DMA_XFER_LENGTH;
    
	    if(sc->sc_actbp[targid] == -1) {
	    /* Get idle buffer */
	    which = get_validbuf_kzq(sc, targid);

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
			(sc->sc_dboff[targid][which]), dmacount);
		    wbflush();
		}

		if( len > 0 )  /* anything to copy */
		{
		    (sc->wmbcopy)( sc->sc_bufp[targid] + sc->sc_xfercnt[targid]
			+ offset, sc->sc_rambuff +
			(sc->sc_dboff[targid][which]), len);
		    wbflush();
		}
	    }
	    } else 
		{
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
        kzqaddr->sii_dmaddrl = 
    			((sc->sc_dboff[targid][which]) & 0xffff);
        kzqaddr->sii_dmaddrh = 
    			((sc->sc_dboff[targid][which]) >> 16);
#endif vax
#ifdef mips


        kzqaddr->sii_dmaddrl = 
    			(((sc->sc_dboff[targid][which])) & 0x0fffffff);
        kzqaddr->sii_dmaddrh = 
    			(((((sc->sc_dboff[targid][which])) & 0x0fffffff))>>16);
#endif mips
    	kzqaddr->sii_dmlotc = dmacount;
	kzqaddr->sii_dmabyte = sc->sc_siioddbyte[targid];
#ifdef mips
    wbflush();
#endif mips
        PRINTD(targid, 0x04,
	    ("kzq_restartdma: offset = %d count = %d\n", offset, dmacount));
    	/* Restart the DMA operation */
        sc->sc_szflags[targid] |= SZ_DID_DMA;
        tmp_phase = kzqaddr->sii_dstat & SII_PHA_MSK;
        tmp_state = kzqaddr->sii_cstat & SII_STATE_MSK;
        kzqaddr->sii_csr |= SII_IE;
        kzqaddr->sii_comm = 
    		    	(SII_DMA | SII_INXFER | tmp_state | tmp_phase);
#ifdef mips
    wbflush();
#endif mips
	    if(sc->sc_siidmacount[targid] - dmacount >= 0) {
		offset += dmacount; /* Bump user offset by last xfer */
		/* Copy data to RAM buffer if DMA WRITE operation */
    	   	 if(sc->sc_iodir[targid] == SZ_DMA_WRITE) {
	    	        /* Get idle buffer */
	    	        which = get_validbuf_kzq(sc, targid);
            		/* Calculate the DMA transfer length */
            		dmacount = sc->sc_siidmacount[targid] - dmacount;
    	    		if(dmacount > KZQ_MAX_DMA_XFER_LENGTH)
                		dmacount = KZQ_MAX_DMA_XFER_LENGTH;
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
				    (sc->sc_dboff[targid][which]), dmacount);
				wbflush();
			    }

			    if( len > 0 )  /* anything to copy */
			    {
				(sc->wmbcopy)( sc->sc_bufp[targid] +
				    sc->sc_xfercnt[targid] + offset,
				    sc->sc_rambuff +
				    (sc->sc_dboff[targid][which]), len);
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
kzq_state_change(sc, activetargid)
register struct sz_softc *sc;
int *activetargid;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int targid, timer;
    u_short cstat;

    kzqaddr->sii_cstat = SII_SCH;
#ifdef mips
    wbflush();
#endif mips

  /* If targid is -1, there is no active target and  the SII will soon be
    connected to.  Wait and make sure the CON is set. */

    if(*activetargid == -1)
    {
      /* CON can assert much later than DST so spin a bit... */
	timer = 1000;
	while (((kzqaddr->sii_cstat & SII_CON) == 0) && --timer)
	    ;
    }

    cstat = kzqaddr->sii_cstat;
    /* Handle a select or reselect here */
    if(cstat & SII_CON) {

	sc->scsi_bus_idle = 0;
	/* Handle a reselect */
	if(cstat & SII_DST_ONBUS) {
	    targid = (kzqaddr->sii_destat & SII_IDMSK);
	    PRINTD(targid, 0x4,
		("kzq_state_change: target ID %d reselected\n",targid));
	    sc->sc_active = (1 << targid);
	    sc->sc_selstat[targid] = SZ_RESELECT;
    	    sc->sc_szflags[targid] &= ~SZ_WAS_DISCON;
	    kzqaddr->sii_comm = (kzqaddr->sii_cstat & SII_STATE_MSK);
#ifdef mips
    wbflush();
#endif mips
	}
	/* Handle a select */
	else {
	    targid = (kzqaddr->sii_slcsr & SII_IDMSK);
	    PRINTD(targid, 0x4,
		("kzq_state_change: target ID %d selected\n",targid));
	    sc->sc_active = (1 << targid);
	    sc->sc_selstat[targid] = SZ_SELECT;
	}
	if(!sc->scsi_polled_mode) {
            kzqaddr->sii_csr &= ~(SII_SLE | SII_RSE);
#ifdef mips
    wbflush();
#endif mips
	}
      /* Set the sii_dmctrl register to the rec/ack offset for the target.
	It does not matter if the state change is a selection or reselection,
	the sii_dmctrl needes to be set if/when data phase is entered. */

        kzqaddr->sii_dmctrl = sc->sc_siireqack[targid];
#ifdef mips
    wbflush();
#endif mips

    }
    /* Handle a disconnect here */
    else {
	sc->scsi_bus_idle = 1;		/* because !CON bus is idle */
	kzqaddr->sii_comm = 0;
#ifdef mips
    wbflush();
#endif mips
        kzqaddr->sii_dmctrl = 0;	/* clear out any previous offset */
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

	targid = kzq_getactive_target(sc);
	PRINTD(targid, 0x4,
	    ("kzq_state_change: target ID %d disconnected\n",targid));
	if(sc->scsi_completed[targid])
	    sc->sc_selstat[targid] = SZ_IDLE;
	else
	    sc->sc_selstat[targid] = SZ_DISCONN;
	if(!sc->scsi_polled_mode)
            kzqaddr->sii_csr |= ( SII_RSE |SII_SLE | SII_IE);
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

kzq_phase_change(sc, dstat)
register struct sz_softc *sc;
u_short dstat;
{
    int cntlr = sc->sc_siinum;
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
    int tmp_state;
    int tmp_phase;
    int phase;
    int targid = kzq_getactive_target(sc);

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
    PRINTD(targid, 0x4, ("kzq_phase_change: current bus phase = "));
    PRINTD(targid, 0x4, ("", kzq_print_phase(phase)));

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
	if(kzq_msgin(sc) != SZ_SUCCESS)
	    {
	    return(SZ_RET_ABORT);
	    }
	break;

    case SCS_MESSO:
        sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_MESSO_PHA;
	sc->sc_szflags[targid] = 0;
	if(kzq_msgout(sc) != SZ_SUCCESS)
	    {
	    return(SZ_RET_ABORT);
	    }
	break;

    case SCS_CMD:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_CMD_PHA;
	if(kzq_sendcmd(sc) != SZ_SUCCESS)
	    {
	   return(SZ_RET_ABORT);
	   }
	break;

    case SCS_STATUS:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_STATUS_PHA;
	if(kzq_getstatus(sc) != SZ_SUCCESS)
	    {
	    return(SZ_RET_ABORT);
	    }
	break;

    case SCS_DATAO:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAO_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];
	if(kzq_startdma(sc, SZ_DMA_WRITE) != SZ_SUCCESS)
	    {
	    return(SZ_RET_ABORT);
	    }
	break;

    case SCS_DATAI:
	sc->sc_prevpha = sc->sc_fstate;
	sc->sc_fstate = SZ_DATAI_PHA;
	sc->sz_opcode = sc->sc_actcmd[targid];
	if(kzq_startdma(sc, SZ_DMA_READ) != SZ_SUCCESS)
	    {
	    return(SZ_RET_ABORT);
	    }
	break;

    default:
	PRINTD(targid, 0x4, ("kzq_phase_change: unexpected bus phase = "));
	PRINTD(targid, 0x4, ("", kzq_print_phase(phase)));
	scsi_logerr(sc, 0, targid, SZ_ET_BUSERR, 0x4c, 0, SZ_HARDERR);
	    {
	    return(SZ_RET_ABORT);
	    }
	break;
    }
    return(SZ_SUCCESS);
}

/******************************************************************
 *
 * Dump out the registers on the SII chip.
 *
 ******************************************************************/
kzq_dumpregs(cntlr, who)
int	cntlr;
int	who;
{
        register struct kzq_regs *kzqaddr =
		(struct kzq_regs *)sz_softc[cntlr].sc_scsiaddr;

/*
	printd1("\t\tSII register dump:\n");
	printd1("\t\tsii_sc1 = 0x%x\n", kzqaddr->sii_sc1 & 0xffff);
	printd1("\t\tsii_csr = 0x%x\n", kzqaddr->sii_csr & 0xffff);
	printd1("\t\tsii_id = 0x%x\n", kzqaddr->sii_id & 0xffff);
	printd1("\t\tsii_slcsr = 0x%x\n", kzqaddr->sii_slcsr & 0xffff);
	printd1("\t\tsii_destat = 0x%x\n", kzqaddr->sii_destat & 0xffff);
	printd1("\t\tsii_data = 0x%x\n", kzqaddr->sii_data & 0xffff);
	printd1("\t\tsii_dmctrl = 0x%x\n", kzqaddr->sii_dmctrl & 0xffff);
	printd1("\t\tsii_dmlotc = 0x%x\n", kzqaddr->sii_dmlotc & 0xffff);
	printd1("\t\tsii_dmaddrl = 0x%x\n", kzqaddr->sii_dmaddrl & 0xffff);
	printd1("\t\tsii_dmaddrh = 0x%x\n", kzqaddr->sii_dmaddrh & 0xffff);
	printd1("\t\tsii_dmabyte = 0x%x\n", kzqaddr->sii_dmabyte & 0xffff);
	printd1("\t\tsii_cstat = 0x%x\n", kzqaddr->sii_cstat & 0xffff);
	printd1("\t\tsii_dstat = 0x%x\n", kzqaddr->sii_dstat & 0xffff);
	printd1("\t\tsii_comm = 0x%x\n", kzqaddr->sii_comm & 0xffff);
	printd1("\t\tkzq_dmacsr = 0x%x\n", kzqaddr->kzq_dmacsr & 0xffff);
	printd1("\t\tkzq_qbar = 0x%x\n", kzqaddr->kzq_qbar & 0xffff);
	printd1("\t\tkzq_lbar = 0x%x\n", kzqaddr->kzq_lbar & 0xffff);
	printd1("\t\tkzq_wc = 0x%x\n", kzqaddr->kzq_wc & 0xffff);
	printd1("\t\tkzq_vector = 0x%x\n", kzqaddr->kzq_vector & 0xffff);
*/
	/*
	 * Called from scsi_logerr() binary error log.
	 * Print most meaningful registers (limit one line, six regs).
	 */
	if (who) {
	    cprintf("SII %d regs: ", cntlr);
	    cprintf("sc1=%x ", kzqaddr->sii_sc1 & 0xffff);
	    cprintf("slcsr=%x ", kzqaddr->sii_slcsr & 0xffff);
	    cprintf("destat=%x ", kzqaddr->sii_destat & 0xffff);
	    cprintf("cstat=%x ", kzqaddr->sii_cstat & 0xffff);
	    cprintf("dstat=%x ", kzqaddr->sii_dstat & 0xffff);
	    cprintf("comm=%x\n", kzqaddr->sii_comm & 0xffff);
	    return;
	}
	printf("\t\tSCSI register dump:\n");
	printf("\t\tsii_sc1 = 0x%x\n", kzqaddr->sii_sc1 & 0xffff);
	printf("\t\tsii_csr = 0x%x\n", kzqaddr->sii_csr & 0xffff);
	printf("\t\tsii_id = 0x%x\n", kzqaddr->sii_id & 0xffff);
	printf("\t\tsii_slcsr = 0x%x\n", kzqaddr->sii_slcsr & 0xffff);
	printf("\t\tsii_destat = 0x%x\n", kzqaddr->sii_destat & 0xffff);
	printf("\t\tsii_data = 0x%x\n", kzqaddr->sii_data & 0xffff);
	printf("\t\tsii_dmctrl = 0x%x\n", kzqaddr->sii_dmctrl & 0xffff);
	printf("\t\tsii_dmlotc = 0x%x\n", kzqaddr->sii_dmlotc & 0xffff);
	printf("\t\tsii_dmaddrl = 0x%x\n", kzqaddr->sii_dmaddrl & 0xffff);
	printf("\t\tsii_dmaddrh = 0x%x\n", kzqaddr->sii_dmaddrh & 0xffff);
	printf("\t\tsii_dmabyte = 0x%x\n", kzqaddr->sii_dmabyte & 0xffff);
	printf("\t\tsii_cstat = 0x%x\n", kzqaddr->sii_cstat & 0xffff);
	printf("\t\tsii_dstat = 0x%x\n", kzqaddr->sii_dstat & 0xffff);
	printf("\t\tsii_comm = 0x%x\n", kzqaddr->sii_comm & 0xffff);
	printf("\t\tkzq_dmacsr = 0x%x\n", kzqaddr->kzq_dmacsr & 0xffff);
	printf("\t\tkzq_qbar = 0x%x\n", kzqaddr->kzq_qbar & 0xffff);
	printf("\t\tkzq_lbar = 0x%x\n", kzqaddr->kzq_lbar & 0xffff);
	printf("\t\tkzq_wc = 0x%x\n", kzqaddr->kzq_wc & 0xffff);
	printf("\t\tkzq_vector = 0x%x\n", kzqaddr->kzq_vector & 0xffff);
}

#ifdef SZDEBUG
/******************************************************************
 *
 * Print out the current bus phase.
 *
 ******************************************************************/
kzq_print_phase(phase)
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
kzq_print_status(status)
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
kzq_print_inq_info(idp)
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

#ifdef mips

kzq_dodev(sc, targid, devtype)
struct sz_softc *sc;
{
	struct scsi_devtab *sdp = scsi_devtab;
	struct sz_inq_dt *idp = (struct sz_inq_dt *)&sc->sz_dat[targid];


	PRINTD(targid, 0x1, ("kzq_dodev: checking on %x\n", devtype));
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
				kzq_scsistart(sc, targid, 0);
			}
			if(sdp->flags & SCSI_STARTUNIT) {
				int starttried = 0;
				sc->sc_curcmd[targid] = SZ_TUR;
				sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
				kzq_scsistart(sc, targid, 0);
				if(sc->sc_status[targid] != SZ_SUCCESS && !starttried) {
					sc->sc_curcmd[targid] = SZ_P_SSUNIT;
					sz_bldpkt(sc, targid, SZ_P_SSUNIT, 0, 1);
					kzq_scsistart(sc, targid, 0);
					starttried++;
				}
				do{
				DELAY(1000000);
				sc->sc_curcmd[targid] = SZ_TUR;
				sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
				kzq_scsistart(sc, targid, 0);
				} while(sc->sc_status[targid] != SZ_SUCCESS && starttried++ < 30);
				sc->sc_curcmd[targid] = SZ_RDCAP;
				sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
				kzq_scsistart(sc, targid, 0);
			}
			if(sdp->flags & SCSI_TESTUNITREADY) {
				sc->sc_curcmd[targid] = SZ_TUR;
				sz_bldpkt(sc, targid, SZ_TUR, 0, 1);
				kzq_scsistart(sc, targid, 0);
			}
			if(sdp->flags & SCSI_READCAPACITY) {
				sc->sc_curcmd[targid] = SZ_RDCAP;
				sz_bldpkt(sc, targid, SZ_RDCAP, 0, 1);
				kzq_scsistart(sc, targid, 0);
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
#endif mips


/******************************************************************
 *
 * Name:	kzq_busy_target
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
kzq_busy_target(unit)
int unit;
{
    int cntlr = (unit >> 3) & 1;
    register struct sz_softc *sc = &sz_softc[cntlr];
    register struct kzq_regs *kzqaddr = (struct kzq_regs *)sc->sc_scsiaddr;
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
	mprintf( "kzq_busy_target: BUSY flag not set on %d\n", targid );
    }
    sc->sc_szflags[targid] &= ~SZ_BUSYTARG;	/* clear the flag */

    /* Clear all associated states and flags for this target */

    sc->sc_xstate[targid] = SZ_NEXT;
    sc->sc_xevent[targid] = SZ_BEGIN;
    sc->sc_szflags[targid] = SZ_NORMAL;
    sc->sc_flags[targid] &= ~DEV_HARDERR;
    sc->sc_selstat[targid] = SZ_IDLE;

    PRINTD(targid, 0x24, 
	("kzq_busy_target: requeueing scsi target %d after BUSY status\n",
		targid));

    /* If the SCSI bus is not busy then restart this target, the b_active
    flag is cleared.  The start routine will be able to act on this queue. */

    s = splbio();
    dp->b_active = 0;			/* set to non-active */
    if(sc->sc_active == 0)
	sz_start(sc, targid);
    splx(s);
}

/******************************************************************
 *
 * Name: get_validbuf_kzq
 *
 * This routine is the same as get_validbuf from scsi_sii.c .
 * We need a duplicate here since we are not assured that the
 * sii module will also be in the kernel.
 * 
 ******************************************************************/

get_validbuf_kzq(sc, targid)
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

