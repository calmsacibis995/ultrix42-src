#ifndef lint
static  char    *sccsid = "@(#)scsi_tape.c	4.6  (ULTRIX)        1/22/91";
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
 * scsi_tape.c	06/27/89
 *
 * PVAX/FIREFOX/PMAX SCSI device driver (tape routines)
 *
 * Modification history:
 *
 *  08-Jan-91	Robin Miller
 *	Modified DEVIOCGET ioctl() code to properly return tape density
 *	codes.  To resolve this problem, the density_table[] was defined
 *	to decode the mode sense density code, and additional code was
 *	added to return default density codes.
 *
 *  15-Nov-90	Robin Miller
 *	Removed clearing of the DEV_TPMARK flag in tzstrategy() for nbuf
 *	I/O requests (B_RAWASYNC).  This caused a race condition with code
 *	in the SCSI state machine with outstanding read requests.  This
 *	problem caused the queued reads to read past the tape file mark.
 *
 *  21-Sept-90	Bill Dallas
 *	Fixed 2 problems with the devget ioctl.
 *	Problem one caused a panic if someone made a minor
 *	number by hand (aka mknod) which was out side the
 *	controllers range. The second problem was minor,
 *	we never gave back to the user the lower 4 bits
 *	of the devget.category_stat field.
 *
 * 07-Sept-90	Maria Vella
 *	Added support for new console turbo-channel ROMs.
 *
 *  30-Jul-90	Bill Dallas   
 *	Added fixed block tape units tape mark handling.
 *	This included a new falg in sc_category_flags called
 *	TPMARK_PENDING
 *
 * 20-May-90	Bill Dallas
 *	Added the option table support and for semi tape drives..
 *	IE. qic format units no end of file marks... Use blankchk
 *	detection if the ONE_FM flag is set in the option 
 *	table.
 *
 * 06-Dec-89    Mitch McConnell
 *	Added test for sc_attached to sc_alive in tzopen, return 
 *	ENXIO if not.
 *
 * 19-Oct-89    Janet L. Schank / John A. Gallant
 *      Added an immediate exit from the retry loop on SZ_NOTREADY
 *      if the FNDELAY flag is set in the open routine.  Changed the
 *      refrence of nNSCSI to nNSCSIBUS.
 * 
 * 01-Oct-89	Fred Canter
 *	Bug fix. Tapes were not reporting write locked status via the
 *	devioget ioctl.
 *
 * 09/22/89     Janet L. Schank
 *      Changed some defines and ifdefs to include and use sii.h.
 *      Removed alot of "ifdef vax"'s.  The softc structure is now
 *      used in the same way as on the vax.  The scsiaddr is now taken
 *      from the softc structure.
 *
 * 24-Jul-89	Fred Canter
 *	Bug fix for dump (MT CACHE ioctls not supported by SCSI).
 *
 * 16-Jul-89	Fred Canter
 *	Changed meaning of count field for MODSNS tzcommand/rzcommand.
 *
 * 15-Jul-89	Fred Canter
 *	Merged Dynamic BBR and error log changes.
 *
 * 13-Jul-89	Fred Canter
 *	Special mode select/sense handling for the EXABYTE tape.
 *
 * 27-Jun-89	John A. Gallant
 *	Added the tape command completion routine.
 *
 * 13-Jun-89	Fred Canter
 *	Added MTFLUSH to tzioctl (always returns ENXIO).
 *
 * 04/13/89	John A. Gallant
 *      Added b_comand to replace b_command for local command buffers.
 *      Use b_gid instead of b_resid to store command.
 *  
 * 03/01/89	John A. Gallant
 *	Added the pseudo command codes for to allow the tape to unload.  I
 *	followed the same conventions as the firefox/pvax code.
 *
 * 02/24/89	John A. Gallant
 *	In tzopen(), during the test for tape loop, the timeout for "NOT_READY"
 *	is increased to 6 seconds, giving an overall timeout loop of 2 minutes.
 *
 * 01/16/89	John A. Gallant
 *	Clear sc_category_flags in tzopen() so left over DEV_TPMARK
 *	does not casue drive to fail all commands after encountering
 *	a tape mark.
 *	Fixed a bug which caused a space command (via ioctl) to
 *	space over a tape mark without failing (as it should).
 *	In tzioctl(), fail the ioctl if DEV_TPMARK set.
 *
 * 11/22/88	John A. Gallant
 * 	If the device is already opened, EBUSY is returned instead of ENXIO.
 *	Added some more debug statements.
 *
 * 11/09/88	John A. Gallant
 *	Started the merge with the V3.0 source level.  Due to time constraints
 *	only the changes for the new IOCTL support will be merged.  Others
 *	changes will hopefully occur as time permits.  Minor re-orginization
 *	of code for minimization of the #ifdef/#endif changes.
 *
 * 25-Aug-88    Ricky Palmer
 *      Ifdef'ed again for vax and mips. This time it is based on
 *      my August 1, 1988 ifdef's of the original scsi.c "glob" driver.
 *
 * 17-Aug-88	Fred Canter
 *	Created this file by moving the SCSI tape specific files
 *	from the old combined driver (scsi.c) to scsi_tape.c.
 *
 ***********************************************************************/

#include "sii.h"
#include "scsi.h"

#if NSCSI > 0 || NSII > 0 || defined(BINARY)

#include "../data/scsi_data.c"
#include "scsi_debug.h"

/*
 * Define the tape density table.
 */
static int density_table[] = {
	0,				/* 0x00 - Default density.	*/
	DEV_800BPI,			/* 0x01 - 800 BPI   (NRZI, R)	*/
	DEV_1600BPI,			/* 0x02 - 1600 BPI  (PE, R)	*/
	DEV_6250BPI,			/* 0x03 - 6250 BPI  (GCR, R)	*/
	DEV_8000_BPI,			/* 0x04 - 8000 BPI  (GCR, C)	*/
	DEV_8000_BPI,			/* 0x05 - 8000 BPI  (GCR, C)	*/
	0,				/* 0x06 - 3200 BPI  (PE, R)	*/
	0,				/* 0x07 - 6400 BPI  (IMFM, C)	*/
	DEV_8000_BPI,			/* 0x08 - 8000 BPI  (GCR, CS)	*/
	DEV_38000BPI,			/* 0x09 - 37871 BPI (GCR, C)	*/
	DEV_6666BPI,			/* 0x0A - 6667 BPI  (MFM, C)	*/
	DEV_1600BPI,			/* 0x0B - 1600 BPI  (PE, C)	*/
	0,				/* 0x0C - 12690 BPI (GCR, C)	*/
	DEV_10000_BPI,			/* 0x0D - QIC-120 with ECC.	*/
	DEV_10000_BPI,			/* 0x0E - QIC-150 with ECC.	*/
	DEV_10000_BPI,			/* 0x0F - QIC-120   (GCR, C)	*/
	DEV_10000_BPI,			/* 0x10 - QIC-150   (GCR, C)	*/
	DEV_16000_BPI,			/* 0x11 - QIC-320   (GCR, C)	*/
	0,				/* 0x12 - QIC-1350  (RLL, C)	*/
	DEV_61000_BPI,			/* 0x13 - 4mm Tape  (DDS, CS)	*/
	DEV_54000_BPI			/* 0x14 - 8mm Tape  (???, CS)	*/
};
static int density_entrys = sizeof(density_table) / sizeof(int);

/*
 * TODO:
 *	Temporary(?) debug variable.
 *	If nonzero, SZ_NODEVICE is returned from tz_rcvdiag()
 *	if the tape fails self test or its firmware revision
 *	level is too far out of date.
 *	If zero, tz_rcvdiag() results are ignored.
 */
int sz_open_fst = 1;

int	wakeup();
extern int hz;

extern int sz_unit_rcvdiag[];	/* If zero, need unit's selftest status */

/*
 * Unit on line flag. Set to one if the
 * device is on-line. Set to zero on any unit
 * attention condition.
 */
extern int sz_unit_online[];

tzopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct uba_device *ui;
	register struct sz_softc *sc;
	int unit = UNIT(dev);
	int cntlr;
	int targid;
	int retry = 0;
	int retval;
	int dev_ready;
	struct sz_modsns_dt *sdp;

	/*
	 * Order of following checks is important.
	 */
	if (unit >= nNSZ)
	    return(ENXIO);
	ui = szdinfo[unit];
	if ((ui == 0) || (ui->ui_alive == 0))
	    return(ENXIO);
	cntlr = ui->ui_ctlr;
	if (cntlr >= nNSCSIBUS)
	    return(ENXIO);
	targid = ui->ui_slave;
	sc = &sz_softc[cntlr];

	if ((sc->sc_alive[targid] == 0) || (sc->sc_attached[targid] == 0))
	    return (ENXIO);

	if ((sc->sc_devtyp[targid] & SZ_TAPE) == 0)
	    return(ENXIO);

	/* Is the device already opened?, only one user at a time is allowed. */
	if( sc->sc_openf[targid] ) 
		return(EBUSY);
	/*
	 * This is a strange use of the FNDELAY flag.  It
	 * is here to allow the installation finder program
	 * to open the device when the tape cartridge is not
	 * inserted.  The installation finder program needs
	 * to do an ioctl, so open must succeed whether or
	 * not a cartridge is present.
	 */
	/*
	 * Clear sc_flags, device will lockup after any
	 * hard error (DEV_HARDERR set) if we don't.
	 * TODO: other driver look at dis_eot_??[]!
	 */
	sc->sc_flags[targid] = 0;

	sc->sc_category_flags[targid] = 0;
	sc->sc_szflags[targid] &= ~SZ_NODEVICE;

	/*
	 * Get selftest result, if we haven't already.
	 * The tz_rcvdiag() routine will return
	 * SZ_NODEVICE if anything is wrong.
	 *
	 * Fix for the nodiag flag in devtab..Some units
	 * must have a senddiag cmd before a recvdiag cmd 
 	 * or data is garbage for the recv diag cmd. We 
	 * will just look at the NO_DIAG flag in the devtab
	 * struct for this type unit.
	 */
	if ((sz_unit_rcvdiag[unit] == 0) && 
		((sc->sc_devtab[targid]->flags & SCSI_NODIAG) == 0))
	    {
	    sc->sc_szflags[targid] |= tz_rcvdiag(dev);
	}
	/*
	 * Try to bring the drive on line.
	 * The TZK50 takes about 25 seconds come ready after
	 * a cartridge change. The TZ30 takes about 30 seconds.
	 * The worst case in on an installtion with tape, not only
	 * does the tape have to rewind, it also has to reposition.
	 * The sleep time for NOT_READY, has been changed to allow for 2
	 * minutes before the tape is determined to be "off-line".
	 * This also should allow the user plenty of time to realize that the
	 * tape is off-line and load the cartridge.
	 */
	dev_ready = 0;
	for (retry = 0; retry < 20; retry++) {
	    if (sc->sc_szflags[targid] & SZ_NODEVICE) {
		if (flag & FNDELAY)
		    break;
		else
		    return(ENXIO);
	    }
	    tzcommand(dev, SZ_TUR, 1, 0);
	    if (sc->sc_c_status[targid] == SZ_GOOD) {
		dev_ready = 1;
		break;
	    }
	    else if (sc->sc_c_status[targid] == SZ_BAD) {
		continue;
	    }
	    else if (sc->sc_c_status[targid] == SZ_CHKCND) {
		retval = SZ_RETRY;
		switch(sc->sc_c_snskey[targid]) {
		/* TODO: this shouldn't happen! */
		case SZ_NOSENSE:
		case SZ_RECOVERR:
		    retval = SZ_SUCCESS;
		    dev_ready = 1;
		    break;

		case SZ_NOTREADY:

		  /* If FNDELAY is set don't enter the stall loop.  The retry
		    count is set to the limit and the loop will terminate with
		    the device "off-line".  Worst case: a rewinding tape will
		    be flaged as off-line, only if FNDELAY is set. */

		    if (flag & FNDELAY) {
			retry = 20; 		/* force exit of the loop */
			break;
		    }

                    timeout(wakeup, (caddr_t)&sc->sc_alive[targid], (hz*2));
                    /* TODO: check priority */
		    sleep((caddr_t)&sc->sc_alive[targid], PZERO + 1);
		    if (!(sc->sc_flags[targid] & DEV_EOM)) {
			sc->sc_flags[targid] = 0;
		    }
		    break;

		case SZ_UNITATTEN:
		    sz_unit_online[unit] = 0;
		    /* just retry */
		    break;

		default:
		    /* TODO: may want to retry? */
		    if (!(sc->sc_flags[targid] & DEV_EOM)) {
			sc->sc_flags[targid] = 0;
		    }
		    if(flag & FNDELAY) {
			sc->sc_szflags[targid] |= SZ_NODEVICE;
			retval = SZ_SUCCESS;
		    }
		    else {
			return(ENXIO);
		    }
		    break;
		}	/* end of switch */
		if (retval == SZ_SUCCESS)
		    break;		/* from for loop */
		else
		    continue;		/* with for loop */
	    }
	    else {
		printf("tzopen: impossible sc_c_status (val=%d)\n",
		    sc->sc_c_status[targid]);
		continue;	/* retry */
	    }
	}	/* end for loop */
	if (retry >= 20) {
	    if (!(flag & FNDELAY)) {
	    	DEV_UGH(sc->sc_device[targid], unit, "offline");
	    	return(EIO);
	    }
	    sc->sc_flags[targid] |= DEV_OFFLINE;
	}
	/*
	 * If SZ_NODEVICE is not set, the device exists,
	 * and we want to do a SZ_MODSEL command
	 * TODO: would be nice not to do this on every open.
	 */
	if (!(sc->sc_szflags[targid] & SZ_NODEVICE)) {
	    if (tz_exabyte_modsns) {
		tzcommand(dev, SZ_MODSNS, -1, 0);
		if (sc->sc_c_status[targid] != SZ_GOOD)
		    printf("tzopen: %s unit %d: mode sense failed\n",
			sc->sc_device[targid], unit);
		sdp = (struct sz_modsns_dt *)&sc->sz_dat[targid];
		printf("vu = 0x%x, mt = 0x%x, rt = 0x%x\n", sdp->vulen,
			sdp->pad[0], sdp->pad[1]);
	    }
	    for (retry = 0; retry < 5; retry++) {
		tzcommand(dev, SZ_MODSEL, 1, 0);
		if (sc->sc_c_status[targid] == SZ_GOOD)
		    break;
	    }
	    if ((retry >= 5) && ((flag & FNDELAY) == 0)) {
		printf("tzopen: %s unit %d: mode select failed\n",
		    sc->sc_device[targid], unit);
		return(EIO);
	    }
	}


/* If you want to see what was it set at..... uncomment
 *	tzcommand(dev, SZ_MODSNS, -1, 0);
 *	sdp = (struct sz_modsns_dt *)&sc->sz_dat[targid];
 *
 *	printf("speed = %x bufmode = %x, wp %x bdeclen %x\n", sdp->speed, 
 *		sdp->bufmode, sdp->wp, sdp->bdeclen);
 *	printf("density = %x blk2 = %x blk1 = %x blk0 = %x\n", sdp->density,
 *		sdp->numofblk2, sdp->numofblk1, sdp->numofblk0);
 *	printf("bl2 = %x bl1 = %x bl0 =%x\n",sdp->blklen2, sdp->blklen1,
 *		sdp->blklen0);
 *	printf("vu = 0x%x, mt = 0x%x, rt = 0x%x\n", sdp->vulen,
 *		sdp->pad[0], sdp->pad[1]);
*/

	/* TODO: may not be wright place & use to cntl modsel? */
	/* So open nodelay doesn't falsely set on-line! */
	if (dev_ready)
	    sz_unit_online[unit] = 1;
	sc->sc_openf[targid] = 1;
	return (0);
}

/*
 * TZ30/TZK50 Receive Diagnostics Routine
 */
int tz_rcvdiag(dev)
	register dev_t dev;
{
	register struct uba_device *ui;
	register struct sz_softc *sc;
	int unit = UNIT(dev);
	int targid;
	int i;
	u_char *byteptr;

        ui = szdinfo[unit];
        sc = &sz_softc[ui->ui_ctlr];
        targid = ui->ui_slave;
       
	/* zero the receive data area */
	byteptr = (u_char *)&sc->sz_dat[targid];
	for (i = 0; i < SZ_RECDIAG_LEN; i++)
	    *byteptr++ = 0;

	/*
	 * Try 10 times to receive diagnostic results.
	 * First try after power up (or other unit attention)
	 * will fail.
	 */
	for (i = 0; i < 10; i++) {
	    tzcommand(dev, SZ_RECDIAG, 1, 0);
	    if (sc->sc_c_status[targid] == SZ_GOOD)
		break;
	}
	if (i >= 10) {
	    printf("%s unit %d: receive diagnostics command failed.\n",
		sc->sc_device[targid], unit);
		if (sz_open_fst)
		    return(SZ_NODEVICE);
	}
	if (sc->sz_dat[targid].dat.recdiag.ctlr_selftest != 0) {
	    printf("%s unit %d: controller selftest failed.\n",
		sc->sc_device[targid], unit);
	    if (sz_open_fst)
		return(SZ_NODEVICE);
	}
	if (sc->sz_dat[targid].dat.recdiag.drv_selftest != 0 ) {
	    printf("%s unit %d: drive selftest failed, code = 0x%x\n",
		sc->sc_device[targid], unit,
		sc->sz_dat[targid].dat.recdiag.drv_selftest);
	    if (sz_open_fst)
		return(SZ_NODEVICE);
	}
	/*
	 * Clear unit_rcvdiag flag, only if everything is ok,
	 * so we don't call tz_rcvdiag() on every open.
	 */
	sz_unit_rcvdiag[unit] = 1;
	return(SZ_SUCCESS);
}
/*
TODO1: this and all other routines need function headers!
*/
/* TODO: what if - open FNDELAY then close (could rewind tape)? */
tzclose(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct sz_softc *sc;
	register struct uba_device *ui;
	int unit = UNIT(dev);
	int targid;
	register int sel = SEL(dev);
	struct scsi_devtab *sdp;
	struct tape_opt_tab *todp;
	struct tape_info *ddp;

	ui = szdinfo[unit];
	targid = ui->ui_slave;
	sc = &sz_softc[ui->ui_ctlr];
	

	sdp = (struct scsi_devtab *)sc->sc_devtab[targid];
	
	/* 
	 * get our tape option struct if available
	*/
	if( sdp->opt_tab){
	    todp = (struct tape_opt_tab *)sdp->opt_tab;
	    /*
	     * since there is no bp must do it by the dev number
	    */
	    ddp = &todp->tape_info[((minor(dev)&DENS_MASK)>>3)];
	}

	/* TODO: do we really need to clear this flag 3 times? */
	sc->sc_flags[targid] &= ~DEV_EOM;

	if (sz_unit_online[unit]) {	/* only if unit still on-line */
	    if (flag == FWRITE || ((flag & FWRITE) &&
	       (sc->sc_flags[targid] & DEV_WRITTEN))) {
		/* TODO: may want to retry this one? */
		/* TODO: need to check for errors */
		
		/* 
		 * check to see if the one_fm flag is set..
		 * we  write one file  mark...  This is
		 * done for QIC type units.. blankchk
		 * is the logical end of tape detection
		*/
		if(sdp->opt_tab){
		    if( (ddp->tape_flags & ONE_FM) == 0){
			tzcommand(dev, SZ_WFM, 2, 0);
		    }
		    else {
			tzcommand(dev, SZ_WFM, 1, 0);
		    }
		}
		else{
		    tzcommand(dev, SZ_WFM, 2, 0);
		}
		sc->sc_flags[targid] &= ~DEV_EOM;
		/* TODO: need to check for errors */
		if(sdp->opt_tab){
		    if( (ddp->tape_flags & ONE_FM) == 0 ){
			tzcommand(dev, SZ_P_BSPACEF, 1, 0);
		    }
		}
		else{
		    tzcommand(dev, SZ_P_BSPACEF, 1, 0);
		}
		sc->sc_flags[targid] &= ~DEV_EOM;
	    }
	    /* if we need to rewind... */
	    if ( (sel & NO_REWIND) == 0 ) {
		/* no error check, bucasue we don't wait for completion */
		tzcommand(dev, SZ_REWIND, 0, 0);
		/* 
		 * must clear out the tpmark pending for fixed
		 * units
		*/
		sc->sc_category_flags[targid] = 0;

	    }
	    /* 
	     * to maintain tape position across closes we
	     * look at the tape mark pending flag (fixed block units)
	     * if so we back space across it.
	    */
	  
	    if( sc->sc_category_flags[targid] & TPMARK_PENDING){

		tzcommand(dev, SZ_P_BSPACEF, 1, 0);
		sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
		sc->sc_category_flags[targid] &= ~DEV_TPMARK;
	    }
	}

	sc->sc_openf[targid] = 0;
}

tzcommand(dev, com, count, retry)
	register dev_t dev;
	register int com;
	register int count;
	register int retry;
{
	int unit = UNIT(dev);
	register struct buf *bp = &cszbuf[unit];

	while (bp->b_flags & B_BUSY) {
		if(bp->b_bcount == 0 && (bp->b_flags & B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	/* Load the buffer.  The b_count field gets used to hold the command
	 * count.  The b_resid field gets used to hold the command mnermonic.
	 * These two fields are "known" to be "save" to use for this purpose.
	 * (Most other drivers also use these fields in this way.)
	 */
	bp->b_flags = B_BUSY|B_READ;
	bp->b_dev = dev;
	bp->b_comand = com;
	bp->b_bcount = count;
	bp->b_blkno = 0;
	bp->b_retry = retry;
	tzstrategy(bp);
	/*
	 * In the case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return;

	iowait(bp);

	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);

	bp->b_flags &= B_ERROR;
}

tzstrategy(bp)
	register struct buf *bp;
{
	struct uba_device *ui;
	register struct uba_ctlr *um;
	register struct sz_softc *sc;
	register struct buf *dp;
	register int s;
	int unit = UNIT(bp->b_dev);
	int targid;
	int cntlr;

	ui = szdinfo[unit];
	targid = ui->ui_slave;
	cntlr = ui->ui_ctlr;
	sc = &sz_softc[cntlr];

	if ((sc->sc_flags[targid]&DEV_EOM) && !((sc->sc_flags[targid]&DEV_CSE) ||
	    (dis_eot_sz[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	if ((bp->b_flags&B_READ) && (bp->b_flags&B_RAWASYNC) && 
	    ((sc->sc_category_flags[targid]&DEV_TPMARK) ||
	    (sc->sc_flags[targid]&DEV_HARDERR))) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	/*
	 * Fixed block tapes.... If the tape mark pending
	 * flag is set then set DEV_TPMARK and clear pending
	 * for nbuf ..and reads (sync). 
	*/
	if ((bp->b_flags&B_READ) && !(bp->b_flags&B_RAWASYNC) &&
	    (sc->sc_category_flags[targid] & TPMARK_PENDING)) {
		sc->sc_category_flags[targid] |= DEV_TPMARK;
		sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}
	bp->av_forw = NULL;
	/* 
	 * If SZ_NODEVICE is set, the device was opened
	 * with FNDELAY, but the device didn't respond.
	 * We'll try again to see if the device is here,
	 * if it is not, return an error
	 */
	if (sc->sc_szflags[targid] & SZ_NODEVICE) {
	    DEV_UGH(sc->sc_device[targid],unit,"offline");
	    bp->b_resid = bp->b_bcount;
	    bp->b_error = ENOSPC;
	    bp->b_flags |= B_ERROR;
	    biodone(bp);
	    return;
	}
#ifdef vax
	s = spl5();
#endif vax
#ifdef mips
	s = splbio();
#endif mips
	dp = &szutab[unit];
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if ((dp->b_active == 0) && (sc->sc_active == 0)) {
		sc->sc_xstate[targid] = SZ_NEXT;
		sc->sc_xevent[targid] = SZ_BEGIN;
		sz_start(sc, targid);
	}
	splx(s);
}

tzread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	int unit = UNIT(dev);

	return (physio(tzstrategy, &rszbuf[unit], dev, B_READ, minphys, uio));
}

tzwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	int unit = UNIT(dev);

	return (physio(tzstrategy, &rszbuf[unit], dev, B_WRITE, minphys, uio));
}

tzioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct uba_device *ui;
	register struct sz_softc *sc;
	register struct uba_ctlr *um;
	int unit = UNIT(dev);
	int cntlr;
	int targid;
	register int callcount;
	register int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	struct devget *devget;
	struct sz_modsns_dt *msdp;
	struct scsi_devtab *sdp;
	struct tape_opt_tab *todp;

	/* we depend of the values and order of the MT codes here 
	 * static stops[] = { SZ_WFM,SZ_P_FSPACEF,SZ_P_BSPACEF,SZ_P_FSPACER,
	 *		   SZ_P_BSPACER,SZ_REWIND,SZ_P_UNLOAD,SZ_P_CACHE,
	 *		   SZ_P_NOCACHE,SZ_RQSNS };
	*/
#define SZ_SZNOP SZ_INQ

	/* we depend on the values and order of the MT codes here */
	static stops[] = { SZ_WFM,SZ_P_FSPACEF,SZ_P_BSPACEF,SZ_P_FSPACER,
			/* MTWEOF    MTFSF       MTBSF       MTFSR	*/
			   SZ_P_BSPACER,SZ_REWIND,SZ_P_UNLOAD,SZ_SZNOP,
			/* MTBSR          MTREW     MTOFFL     MTNOP	*/
			   SZ_P_CACHE,SZ_P_NOCACHE,SZ_RQSNS,SZ_RQSNS,
			/* MTCACHE    MTNOCACHE    MTCSE    MTCLX	*/
			   SZ_RQSNS,SZ_SZNOP,SZ_SZNOP,SZ_SZNOP,SZ_SZNOP,SZ_SZNOP,
			/* MTCLS  MTENAEOT MTDISEOT MTFLUSH MTGTON MTGTOFF */
			   SZ_P_RETENSION};
			/* MTRETEN */

	ui = szdinfo[unit];
	cntlr = ui->ui_ctlr;
        um = szminfo[cntlr];
	targid = ui->ui_slave;
	sc = &sz_softc[cntlr];
	sdp = (struct scsi_devtab *)sc->sc_devtab[targid];
	
	/* 
	 * get our tape option struct if available
	*/
	if( sdp->opt_tab){
	    todp = (struct tape_opt_tab *)sdp->opt_tab;
	}

	PRINTD(targid, 0x2, ("tzioctl: switch cmd %x\n", cmd ) );
	switch (cmd) {

	case MTIOCTOP:				/* tape operation */
		/* 
		 * If SZ_NODEVICE is set, the device was opened
		 * with FNDELAY, but the device didn't respond.
		 * We'll try again to see if the device is here,
		 * if it is not, return an error
		 */
		if (sc->sc_szflags[targid] & SZ_NODEVICE) {
		    DEV_UGH(sc->sc_device[targid],unit,"offline");
		    return(ENXIO);
		}
		mtop = (struct mtop *)data;
		PRINTD(targid, 0x2, ("tzioctl: switch MTIOCTOP data %x\n",
		    mtop->mt_op));
		switch (mtop->mt_op) {

		case MTWEOF:
			callcount = 1;
			if( sc->sc_category_flags[targid] & TPMARK_PENDING){
			    fcount = mtop->mt_count - 1;
			    sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
			}
			else {
			    fcount = mtop->mt_count;
			}
			break;
		case MTFSF: 
			callcount = 1;
			if( sc->sc_category_flags[targid] & TPMARK_PENDING){
			    fcount = mtop->mt_count - 1;
			    if (fcount < 0 ){
				fcount = 0;
			    }
			    if( fcount == 0){
				sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
				sc->sc_category_flags[targid] |= DEV_TPMARK;
			    }
			    else {
				sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
			    }
			}
			else {
			    fcount = mtop->mt_count;
			}
			break;
		case MTBSF:
			callcount = 1;
			if( sc->sc_category_flags[targid] & TPMARK_PENDING){
			    fcount = mtop->mt_count + 1;
			    sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
			}
			else {
			    fcount = mtop->mt_count;
			}
			break;

		case MTFSR:
			callcount = 1;
			if( sc->sc_category_flags[targid] & TPMARK_PENDING){
			    fcount = mtop->mt_count - 1;

			    if (fcount < 0 ){
				fcount = 0;
			    }
			    if( fcount == 0){
				sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
				sc->sc_category_flags[targid] |= DEV_TPMARK;
			    }
				
			    else {
				sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
			    }
			}
			else {
			    fcount = mtop->mt_count;
			}
			break;

		case MTBSR:
			callcount = 1;
			if( sc->sc_category_flags[targid] & TPMARK_PENDING){
			    fcount = mtop->mt_count + 1;
			    sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
			}
			else {
			    fcount = mtop->mt_count;
			}
			break;

		case MTOFFL:
			sc->sc_flags[targid] |= DEV_OFFLINE;
			sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
		case MTREW:
			sc->sc_flags[targid] &= ~DEV_EOM;
			sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
			callcount = 1;
			fcount = 1;
			break;

		case MTNOP:
			return(0);
		case MTCACHE:
		case MTNOCACHE:
			return(ENXIO);

		case MTFLUSH:
			return (ENXIO);

		case MTCSE:
			/*
			 * Clear Serious Exception, used by tape utilities
			 * to clean up after Nbuf I/O and end of media.
			 */
			sc->sc_category_flags[targid] &= ~DEV_TPMARK;
			sc->sc_flags[targid] |= DEV_CSE;
			return(0);

		case MTCLX: case MTCLS:
			return(0);

		case MTENAEOT:
			dis_eot_sz[unit] = 0;
			return(0);

		case MTDISEOT:
			dis_eot_sz[unit] = DISEOT;
			sc->sc_flags[targid] &= ~DEV_EOM;
			return(0);

		case MTRETEN:	/* RETENSION command... */
			sc->sc_flags[targid] &= ~DEV_EOM;
			sc->sc_category_flags[targid] &= ~TPMARK_PENDING;
			callcount = 1;
			fcount = 1;
			break;

		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (EINVAL);
		while (--callcount >= 0) {
			tzcommand(dev, stops[mtop->mt_op], fcount, 0);
			if (sc->sc_c_status[targid] != SZ_GOOD) {
			    if ((sc->sc_c_snskey[targid] != SZ_NOSENSE) &&
                                (sc->sc_c_snskey[targid] != SZ_RECOVERR))
				    return(EIO);
			    if (sc->sc_category_flags[targid] & DEV_TPMARK)
				    return(EIO);
			}
		}
		return(0);

	/* TODO: these are bogus values, fix and test if possible! */
	case MTIOCGET:				/* tape status */
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_flags[targid];  /* send cur flags */
		mtget->mt_erreg = 0;
		/* TODO: not correct value? */
		mtget->mt_resid = sc->sc_resid[targid];
		mtget->mt_type = MT_ISST;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TAPE;
#ifdef vax
		devget->bus = DEV_NB;
		bcopy(DEV_VS_SCSI, devget->interface, strlen(DEV_VS_SCSI));
#endif vax
#ifdef mips
		devget->bus = DEV_SCSI;
		bcopy(DEV_SCSI_GEN, devget->interface, strlen(DEV_SCSI_GEN));
#endif mips
		bcopy(sc->sc_device[targid], devget->device, DEV_SIZE);
		devget->adpt_num = um->um_adpt;
		devget->nexus_num = 0;
		devget->bus_num = 0;
		devget->ctlr_num = cntlr;
		devget->rctlr_num = 0;
		devget->slave_num = targid;
		bcopy("tz", devget->dev_name, 3);
		devget->unit_num = unit;
		devget->soft_count = sc->sc_softcnt[targid];
		devget->hard_count = sc->sc_hardcnt[targid];
		devget->stat = sc->sc_flags[targid];
		/* 
		 * we only want the lower 4 bits at this time 
		 * the rest is density which gwts filled in later 
		*/
		devget->category_stat = (sc->sc_category_flags[targid] & 0X0F);

		/*
		 * Do a mode sense to check for write locked drive.
		 * First one can fail due to unit attention.
		 */
		tzcommand(dev, SZ_MODSNS, -1, 0);
		if (sc->sc_c_status[targid] != SZ_GOOD)
		    tzcommand(dev, SZ_MODSNS, -1, 0);
		if (sc->sc_c_status[targid] == SZ_GOOD) {
		    msdp = (struct sz_modsns_dt *)&sc->sz_dat[targid];
		    if (msdp->wp) {		/* Tape is write locked. */
			devget->stat |= DEV_WRTLCK;
		    }
		    /*
		     * Setup the tape density.
		     */
		    if (msdp->density <= density_entrys) {
			devget->category_stat |= density_table[msdp->density];
		    }
		    /*
		     * Setup default density codes.
		     */
		    if (msdp->density == SCSI_DENS_DEFAULT) {

			switch (sdp->devtype) {

			    case TZ05:
				devget->category_stat |= DEV_1600BPI;
				break;

			    case TZ07:
				devget->category_stat |= DEV_6250BPI;
				break;

			    case TZ30:
			    case TZK50:
				devget->category_stat |= DEV_6666BPI;
				break;

			    case TZK10:
				devget->category_stat |= DEV_16000_BPI;
				break;

			    case TLZ04:
				devget->category_stat |= DEV_61000_BPI;
				break;

			    case TZK08:
				devget->category_stat |= DEV_54000_BPI;
				break;

			    default:
				if (sdp->opt_tab) {
		        	    if (todp->opt_flags & SCSI_QIC) {
					devget->category_stat |= DEV_10000_BPI;
		        	    } else if (todp->opt_flags & SCSI_9TRK) {
					devget->category_stat |= DEV_6250BPI;
				    }
				}
				break;

			} /* End 'switch (sdp->devtype)' */

		    } /* End 'if (msdp->density == SCSI_DENS_DEFAULT)' */

		} /* End 'if (sc->sc_c_status[targid] == SZ_GOOD)' */
		break;

	default:
		return (ENXIO);
		break;
	}
	return (0);
}

/*
 *
 * Name:		tzcomplete	-Tape completion routine
 *
 * Abstract:		This routine is called from the scsi state machine
 *			to perform the completion work for the current data
 *			transfer.
 *
 * Inputs:
 *
 * bp			Buffer pointer of command to be completed.
 *
 * Outputs:
 *			Only printing debug messages.
 *
 * Return Values: 	Nothing formal, b_resid is updated.
 *
 * Side Effects:
 *			biodone() is called to free up the current bp.
 *			The buffer queue is changed to free up the front entry.
 *
 */

int tzcomplete( bp )
    struct buf *bp;
{
    struct uba_device *ui;
    struct sz_softc *sc;
    register struct buf *dp;
    int unit;
    int targid;

    unit = UNIT(bp->b_dev);
    ui = szdinfo[unit];
    sc = &sz_softc[ui->ui_ctlr];
    targid = ui->ui_slave;
    dp = &szutab[unit];

    PRINTD( targid, 0x01, ("tzcomplete called unit %d\n", UNIT(bp->b_dev)) );

    /*
     * Remove the completed request from the queue
     * and release the buffer.
     */
    /* TODO: are we absolutely sure dp is valid? */
    dp->b_actf = bp->av_forw;
    bp->b_resid = sc->sc_resid[targid];

    PRINTD(targid, 0x5,
	("tzcomplete: resid = %d\n", bp->b_resid));

    biodone(bp);

    sc->sc_flags[targid] |= DEV_DONE;
    sc->sc_xevent[targid] = SZ_BEGIN;
    sz_retries[sc->sc_unit[targid]] = 0;

    /*
     * The comand has ended
     */
    dp->b_active = 0;

}

#endif 


