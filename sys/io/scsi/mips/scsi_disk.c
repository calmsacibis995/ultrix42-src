#ifndef lint
static char *sccsid = "@(#)scsi_disk.c	4.6	(ULTRIX)	3/7/91";
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
 * scsi_disk.c	06/27/89
 *
 * PVAX/FIREFOX/PMAX SCSI device driver (disk routines)
 *
 * Modification history:
 *
 * 05-Mar-91	Robin Miller
 *   o	Modified kernel allocation requests (KM_ALLOC) to include the flag
 *	KM_NOWAIT to prevent sleeping if resource isn't available.  This
 *	created a race condition when multiple processes were accessing
 *	the device which resulted in a "km_free: bad reference count"
 *	system panic.
 *   o	Corrected two places in the rzspecial() function where the wrong
 *	number of bytes were being returned for the CDROM_TOC_ENTRYS and
 *	CDROM_READ_SUBCHANNEL ioctl() commands.
 *
 * 19-Feb-91	Robin Miller
 *   o	Correct race condition in the rzcommand() function by adding the
 *	appropriate spl synchronization.  Previously, if multiple processes
 *	attempted simultaneous access to a drive, processes could be left
 *	in an uninterruptable sleep state.
 *   o	Correct sleep priority when waiting for prior I/O to complete.
 *   o	Catch signals if sleeping at an interruptable priority so buffer
 *	resources can be released and proper error code (EINTR) returned.
 *   o	Allow multiple processes to issue CD-ROM audio commands.
 *
 * 30-Jan-91	Robin Miller
 *	Modified check done on MODE SELECT commands, so non-super users
 *	can issue this command to set the volume levels in the audio
 *	control page.
 *
 * 20-Nov-90	Robin Miller
 *	Added support for CD-ROM audio commands.
 *
 * 11-Sep-90	Robin Miller
 *	Added check in DEVIOCGET ioctl() command for Extra Density (ED)
 *	2.88MB RX26 floppy diskette.  Also set Double Density (DD) flag
 *	appropriately for RX23/RX26 3.5" or RX33 5.25" diskettes.
 *
 * 07-Sept-90	Maria Vella
 *	Added support for new console turbo-channel ROMs.
 *
 * 09-Aug-90	Robin Miller
 *	Modified check for write protect status for DEVIOCGET ioctl()
 *	so write lock is returned for all CD-ROM's, not just RRD40.
 *
 * 06-Feb-90	Mitch McConnell
 *	Change error return from EIO to ENOSPC to more accurately reflect
 *	error.
 *
 * 06-Dec-89    Mitch McConnell
 *	Added test for sc_attached to sc_alive in rzopen, return 
 *	ENXIO if not.
 *
 * 11-Nov-89    Janet Schank
 *      Changed the refrence of nNSCSI to nNSCSIBUS
 *
 * 29-Oct-89	Fred Canter
 *	Changed the DEVGETGEOM ioctl mode sense code to validate the
 *	page code, i.e., make sure the disk gave us the page we asked for.
 *	One of the non-DEC disks returns page 56 instead of a check
 *	condition when we ask for page 5 (unsupported page for hard disks).
 *
 * 08-Oct-89	Fred Canter
 *	Remove error log #ifdef OLDWAY.
 *
 * 07-Oct-89	Fred Canter
 *	Added DEVGETGEOM ioctl to return disk geometry information.
 *
 * 01-Oct-89	Fred Canter
 *	Fixed a bug in the RX23 SoftPC hooks. Media type ID returned
 *	by DEVIOGET ioctl was incorrect because first read capacity
 *	after unit attention failed (media change).
 *
 *	Bug fix. Disks (except RRD40) were not reporting correct
 *	write locked status via the devioget ioctl.
 *
 * 23-Jul-89	Fred Canter
 *	Convert DBBR printfs to error log calls.
 *
 * 22-Jul-89	John A. Gallant
 *	Added SCSI_NODBBR flag for disks without BBR.
 *	Added error messages to DBBR code.
 *
 * 16-Jul-89	Fred Canter
 *	Do a mode select to get disk geometry.
 *
 * 27-Jun-89	John A. Gallant
 *	Added the disk command completion routine.  Added the DBBR code.
 *
 * 12-Jun-89	Fred Canter
 *	Restructured the RDCAP code in rzopen() to handle the case where
 *	RDCAP will fail if there is an unformatted floppy in the drive.
 *
 * 11-Jun-89	Fred Canter
 *	Added media changed and density information to devget structure.
 *	So softpc application can make better use of the floppy drive.
 *
 * 08-Jun-89	Fred Canter
 *	Removed RX23 RDCAP hack and handle case where RDCAP can fail if
 *	an unformatted floppy is in the RX23, in rzopen().
 *
 * 06-Jun-89	Fred Canter
 *	Set iostat dk_mspw transfer rate for RX23 in rzopen(), depending
 *	on which type floppy (HD/DD) in loaded in the drive.
 *
 * 24-May-89	Fred Canter
 *	Changes to match the new, more general rzdisk utility.
 *	Changed the mode select data structures so an address and
 *	length are passed with the ioctl. This allows pages to be
 *	added to rzdisk without requiring a kernel rebuild.
 *
 * 04/13/89	John A. Gallant
 *      Added b_comand to replace b_command for local command buffers.
 *      Use b_gid instead of b_resid to store command.
 *
 * 03/01/89	John A. Gallant
 *	Added the pseudo command codes for to allow the tape to unload.  I
 *	followed the same conventions as the firefox/pvax code.  Removed the
 *	extra SSUNIT command during the online retry code in rzopen().
 *
 * 12/14/88	John A. Gallant
 *	More merge with firefox code.
 *	Return an error if either of the KMALLOC calls in rzspecial fails.
 *	Changes to rzspecial(). Allow all commands except: format,
 *	reassign block, and read defect data on the CDROM.
 *	Return EROFS instead of ENXIO if command not allowed on CDROM.
 *	Wait 20 seconds in rzopen for CDROM to come on-line (was 10).
 *
 * 12/02/88	John A. Gallant
 *	Report DEV_WRTLCK flag with DEVIOCGET ioctl so RRD40 will return
 *	write locked and fsck will be NO WRITE.
 *
 * 11/09/88	John A. Gallant
 *	Started the merge with the V3.0 source level.  Due to time constraints
 *	only the changes for the new IOCTL support will be merged.  Others
 *	changes will hopefully occur as time permits.  Minor re-orginization
 *	of code for minimization of the #ifdef/#endif changes.
 *   COMMENTS from V3.0:
 *   03-Nov-88	Alan Frechette
 *	Added in support for disk maintainence. Added the commands
 *	FORMAT UNIT, REASSIGN BLOCK, READ DEFECT DATA, MODE SENSE,
 *	MODE SELECT, INQUIRY and VERIFY DATA. These are all new 
 *	ioctl calls.
 *
 * 25-Aug-88    Ricky Palmer
 *      Ifdef'ed again for vax and mips. This time it is based on
 *      my August 1, 1988 ifdef's of the original scsi.c "glob" driver.
 *
 * 17-Aug-88	Fred Canter
 *	Created this file by moving the SCSI disk specific files
 *	from the old combined driver (scsi.c) to scsi_disk.c.
 *
 ***********************************************************************/

#include "../data/scsi_data.c"
#include "scsi_debug.h"

extern int geterror(), wakeup();
extern int hz;
extern int sz_unit_rcvdiag[];   /* If zero, need unit's selftest status */
extern int sz_retries[];	/* retry counter */

/*
 * Unit on line flag. Set to one if the
 * device is on-line. Set to zero on any unit
 * attention condition.
 */
extern int sz_unit_online[];

/*
 * Disk open routine (need func header)
 */
rzopen(dev, flag)
dev_t dev;
int flag;
{
	register struct sz_softc *sc;
	register struct uba_device *ui;

	int unit = minor(dev) >> 3;
	int cntlr;
	int targid;
	int retry, retrylimit;
	int retval;
	int dev_ready, ssunit_cnt;
	int i;
	struct size *stp;
	struct sz_rdcap_dt *rdp;
	int rzstrategy();

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

	if (sc->sc_alive[targid] == 0)
		return (ENXIO);

	if (((sc->sc_devtyp[targid] & SZ_DISK) == 0) &&
	    ((sc->sc_devtyp[targid] & SZ_CDROM) == 0))
		return (ENXIO);
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
	sc->sc_szflags[targid] &= ~SZ_NODEVICE;
	/*
	 * Get selftest result, if we haven't already.
	 * The rz_rcvdiag() routine will return
	 * SZ_NODEVICE if anything is wrong.
	 */
	/*
	 * TODO:
	 *	The RRD40 is the only disk that supports the
	 *	receive diagnostics command. We need to write
	 *	a rz_rcvdiag() routine if we decide to use it.
	 */
/*	if (sz_unit_rcvdiag[unit] == 0)	*/
/*	    sc->sc_szflags[targid] |= rz_rcvdiag(dev);	*/

	/*
	 * Try to bring the drive on line.
	 *
	 * The disks take about 10 seconds to spin up.
	 * They should already be spinning by the time we get here,
	 * but we nust make sure the drive is spinning to cover
	 * the remote possibility that it went off line.
	 * We try for 20 seconds to bring the drive on-line.
	 *
	 * The RRD40 comes online almost instantly (about 1 second).
	 * So, we try for 5 seconds to bring the drive on-line.
	 * This allows the user some think time to realize the
	 * CDROM is off-line and load the CD.
	 */
	dev_ready = 0;
	ssunit_cnt = 0;
	retrylimit = 20;
	for (retry = 0; retry < retrylimit; retry++) {
	    if (sc->sc_szflags[targid] & SZ_NODEVICE) {
		if (flag & FNDELAY)
		    break;
		else
		    return(ENXIO);
	    }
	    rzcommand(dev, SZ_TUR, 1, 0);
	    if (sc->sc_c_status[targid] == SZ_GOOD) {
		dev_ready = 1;
		break;
	    }
	    else if (sc->sc_c_status[targid] == SZ_BAD) {
		continue;
	    }
	    else if (sc->sc_c_status[targid] == SZ_CHKCND) {
		/* HERE */
		/* Can't be here, call to rzcommand sets new status info. */
/*		rzcommand(dev, SZ_P_SSUNIT, 1, 0);	*/
		retval = SZ_RETRY;
		switch(sc->sc_c_snskey[targid]) {
		/* TODO: this shouldn't happen! */
		case SZ_NOSENSE:
		case SZ_RECOVERR:
		    retval = SZ_SUCCESS;
		    dev_ready = 1;
		    break;
		case SZ_NOTREADY:
		    /*
		     * Send start unit command twice. The first one
		     * could fail due to unit attention.
		     * We don't confuse the issue with error checking!
		     */
		    if (ssunit_cnt < 2) {
			rzcommand(dev, SZ_P_SSUNIT, 1, 0);
			ssunit_cnt++;
		    }

			PRINTD(targid, 0x8000, 
			   ("rzopen: sleeping for disk spin-up\n"));

			timeout(wakeup, (caddr_t)&sc->sc_alive[targid], hz);

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
		    /* TODO: tape specific? */
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
		/* TODO: debug */
		printf("rzopen: impossible sc_c_status (val=%d)\n",
		    sc->sc_c_status[targid]);
		continue;	/* retry */
	    }
	}	/* end for loop */
	if (retry >= retrylimit) {
	    if (!(flag & FNDELAY)) {
	    	DEV_UGH(sc->sc_device[targid], unit, "offline");
	    	return(EIO);
	    }
	    sc->sc_flags[targid] |= DEV_OFFLINE;
	}
	/*
	 * If SZ_NODEVICE is not set (i.e., the device exists)
	 * and the device is off-line, do a mode select.
	 * This sets the Logical Block Size to 512 bytes.
	 * NOTE: RZ disk drives have 512 bytes as the
	 *	 default LBN size, but we set and check it anyway.
	 * NOTE: production RRD40 drives default to 512 byte lbn size.
	 *	 The RRD40's physical sector size is 2KB.
	 */
	if (!(sc->sc_szflags[targid] & SZ_NODEVICE) &&
	    (sz_unit_online[unit] == 0)) {
	    /*
	     * Use MODSEL to specify 512 byte LBNs.
	     * TODO: later on we may want to change other parameters.
	     */
	    for (retry = 0; retry < 5; retry++) {
		rzcommand(dev, SZ_MODSEL, 1, 0);
		if (sc->sc_c_status[targid] == SZ_GOOD)
		    break;
	    }
	    if ((retry >= 5) && ((flag & FNDELAY) == 0)) {
		printf("rzopen: %s unit %d: mode select failed\n",
		    sc->sc_device[targid], unit);
		return(EIO);
	    }

	    /*
	     * Use RDCAP to determine the size of the disk.
	     * Also verify LBN size is really 512 bytes.
	     * NOTE: CD size depends on how much data was mastered
	     *	     on the CD. Must get size after each media change.
	     * NOTE: RDCAP will fail if the floppy is unformatted.
	     */
	    for (retry = 0; retry < 5; retry++) {
		rzcommand(dev, SZ_RDCAP, 1, 0);
		if (sc->sc_c_status[targid] == SZ_GOOD)
		    break;
	    }
	    if (retry >= 5) {
		sc->sc_disksize[targid] = 0;
		if ((flag & FNDELAY) == 0) {
		    printf("rzopen: %s unit %d: read capacity failed\n",
			sc->sc_device[targid], unit);
		    return(EIO);
		}
	    }
	    else {
		rdp = (struct sz_rdcap_dt *)&sc->sz_dat[targid];
		i = (rdp->lbaddr3 << 24) & 0xff000000;
		i |= (rdp->lbaddr2 << 16) & 0x00ff0000;
		i |= (rdp->lbaddr1 << 8) & 0x0000ff00;
		i |= rdp->lbaddr0 & 0x000000ff;
		/*
		 * RDCAP returns the address of the last LBN.
		 * We must add one to get the number of LBNs.
		 */
		sc->sc_disksize[targid] = (daddr_t)(i + 1);
		/*
		 * Verify LBN size is 512 bytes.
		 */
		i = (rdp->blklen3 << 24) & 0xff000000;
		i |= (rdp->blklen2 << 16) & 0x00ff0000;
		i |= (rdp->blklen1 << 8) & 0x0000ff00;
		i |= rdp->blklen0 & 0x000000ff;
		if (i != 512) {
		    if (!(flag & FNDELAY)) {
			printf("rzopen: %s unit %d: %s (size = %d)\n",
			    sc->sc_device[targid], unit,
			    "could not set LBN size to 512 bytes", i);
			return(EIO);
		    }
		}
	    }
	}
	/*
	 *	See if we need to read in the partition table from the disk.
	 *	The conditions we will have to read from the disk is if the
 	 *	partition table valid bit has not been set for the volume
	 *	is invalid.
	 */

	/*
	 *	Assume that the default values before trying to
	 *	see if the partition tables are on the disk. The
	 *	reason that we do this is that the strategy routine
	 *	is used to read in the superblock but uses the 
	 *	partition info.  So we must first assume the
	 *	default values.
	 */

	/* TODO1: this looks wrong. If rsblk fails next time
	 * we will think the partition table is good, but
	 * it will be the default pt not the one read from the disk.
	 */
	if ((sz_part[unit].pt_valid == 0) || (sz_unit_online[unit] == 0)) {
		stp = sc->sc_dstp[targid];
		for( i = 0; i < 8; i++ ) {
			sz_part[unit].pt_part[i].pi_nblocks = stp[i].nblocks;
			sz_part[unit].pt_part[i].pi_blkoff = stp[i].blkoffs;
		}

		sz_part[unit].pt_valid = PT_VALID;	

	/*
	 *	Default partition are now set. Call rsblk to set
	 *	the driver's partition tables, if any exists, from
	 *	the "a" partition superblock
	 */

		/*
		 * TODO: this is a temporary fix!
		 *	 If rsblk returns an error, revert
		 *	 back to the default partition table.
		 *
		 * NOTE: the read by rsblk wipes out the current pt.
		 * ABOVE comment wrong.
		 *
		 *	 Must get pt every time media changed!
		 *	 RZ are fixed media disks, but.....
		 */
		i = rsblk(rzstrategy, dev, &sz_part[unit]);
	}
	/* So open nodelay doesn't falsely set on-line! */
	if (dev_ready)
	    sz_unit_online[unit] = 1;
	return (0);
}

/* TODO: need funct header */
rzsize(dev)
{
	register struct sz_softc *sc;
	register struct uba_device *ui;

	int unit = minor(dev) >> 3;
	int targid;
	int part;
	daddr_t psize;

	/* TODO: check meaning of ui_flags == 0 in udsize???? */
	if (unit >= nNSZ || (ui = szdinfo[unit]) == 0 || ui->ui_alive == 0)
		return(-1);
	sc = &sz_softc[ ui->ui_ctlr ];
	targid = ui->ui_slave;

	/*
	 *	Sanity check		
	 */
	if ( sz_part[unit].pt_valid != PT_VALID )
	    panic("rzsize: invalid partition table ");

	part = minor(dev) & 7;
	if (sz_part[unit].pt_part[part].pi_nblocks != -1) 
	    psize = sz_part[unit].pt_part[part].pi_nblocks; 
	else
	    psize = sc->sc_disksize[targid] - sz_part[unit].pt_part[part].pi_blkoff;
	return(psize);
}

/*
 * rzcommand() - Execute a SCSI Command.
 *
 * Inputs:	dev = The device major/minor number.
 *		com = The SCSI command code.
 *		count = The command count.
 *		retry = The retry count.
 *
 * Return Value:
 *		Returns 0 for SUCCESS, or error code.
 */
int
rzcommand (dev, com, count, retry)
register dev_t dev;
register int com;
register int count;
register int retry;
{
	int unit = minor(dev) >> 3;
	register struct buf *bp = &cszbuf[unit];
	register struct sz_softc *sc;
	register struct uba_device *ui;
	register int s;
	int cntlr, targid, error;

	ui = szdinfo[unit];
	cntlr = ui->ui_ctlr;
	targid = ui->ui_slave;
	sc = &sz_softc[cntlr];

	if (error = rziowait (bp, sc->sc_rzspecial[targid])) {
		return (error);
	}

	/*
	 * Load the buffer.  The bp field usage is (see scsireg.h):
	 *
	 *	b_bcount  = The command count.
	 *	b_gid     = The command mnemonic.
	 *	b_bufsize = The retry count.
	 *
	 * These two fields are "known" to be "save" to use for this purpose.
	 * (Most other drivers also use these fields in this way.)
	 */
	bp->b_flags = B_BUSY|B_READ;
	bp->b_dev = dev;
	bp->b_comand = com;
	bp->b_bcount = count;
	bp->b_blkno = 0;
	bp->b_retry = retry;
	bp->b_error = 0;
	bp->b_proc = u.u_procp;

	rzstrategy(bp);

	iowait(bp);

	s = splbio();
	if (bp->b_flags & B_WANTED) {
		wakeup ((caddr_t)bp);
	}
	splx(s);
	bp->b_flags &= ~(B_BUSY | B_WANTED);
	return (geterror (bp));
}

/*
 * rziowait() - Wait For Prior I/O To Complete (if busy).
 *
 * Inputs:	bp = The buffer to check.
 *		special = Special I/O active flag.
 *
 * Return Value:
 *		Return 0 for SUCCESS, or EINTR if sleep interrupted.
 */
static int
rziowait (bp, special)
register struct buf *bp;
register int special;
{
	register int error = 0;
	register int s;

	s = splbio();
	while (bp->b_flags & B_BUSY) {
	    bp->b_flags |= B_WANTED;
	    if (special) {
		if (sleep ((caddr_t)bp, (PCATCH | PZERO + 1))) {
		    error = EINTR;
		}
	    } else {
		(void) sleep ((caddr_t)bp, PRIBIO + 1);
	    }
	}
	splx (s);
	return (error);
}

rzstrategy(bp)
	register struct buf *bp;
{
	register struct sz_softc *sc;
	struct uba_device *ui;
	register struct buf *dp;
	register int s;
	int unit = minor(bp->b_dev) >> 3;
	int targid;
	int cntlr;
	int bad, err;
	daddr_t sz, maxsz;
	int part;
	struct pt *pt;

	ui = szdinfo[unit];
	targid = ui->ui_slave;
	cntlr = ui->ui_ctlr;
	sc = &sz_softc[cntlr];

	/* TODO1: note - no queue sorting! */
	/*
	 * Validate things like block number,
	 * unit number, on-line, etc.
	 */
	bad = 0;
	while(1) {
	    if (unit >= nNSZ) {
		bad = 1;
		err = ENXIO;
		break;
	    }
	    /*
	     * CDROM is read only!
	     */
	    if ((sc->sc_devtyp[targid] & SZ_CDROM) &&
		((bp->b_flags & B_READ) == 0)) {
		bad = 1;
		err = EROFS;
		break;
	    }

	    /*
	     * RSP says we must have this check.
	     * It prevents open no delay burning us if the
	     * disk's partition table is not valid.
	     * We only do this check for data transfer commands.
	     * House keeping commands (TUR, RDCAP, MODSEL, etc.)
	     * also call rzstrategy.
	     */
	    if (bp != &cszbuf[unit]) {
		if (sz_part[unit].pt_valid != PT_VALID) {
		    if (sc->sc_szflags[targid] & SZ_NODEVICE) {
			bad = 1;
			err = ENXIO;
			break;
		    }
		    else {
			panic("rzstrategy: invalid partition table ");
		    }
		}
	    }

	    /* TODO: check ui == 0 || ui->ui_alive == 0 here - someday */

	    /* TODO: EOM maybe, CSE - don't think so */
	    /* TODO: don't think this can happen for disks */
	    if ((sc->sc_flags[targid]&DEV_EOM) && !((sc->sc_flags[targid]&DEV_CSE) ||
		(dis_eot_sz[unit] & DISEOT))) {
		    bad = 1;
		    err = ENOSPC;
		    break;
	    }

	    /* 
	     * If SZ_NODEVICE is set, the device was opened
	     * with FNDELAY, but the device didn't respond.
	     * We'll try again to see if the device is here,
	     * if it is not, return an error.
	     */
	    if (sc->sc_szflags[targid] & SZ_NODEVICE) {
		DEV_UGH(sc->sc_device[targid], unit, "offline");
		bad = 1;
	        err = ENXIO;
		break;
	    }
	    /*
	     * Check transfer size,
	     * to be sure it does not overflow the
	     * bounds of the partition.
	     * Only check data transfer commands.
	     */
	    if (bp == &cszbuf[unit])
		break;
	    part = minor(bp->b_dev) & 7;
	    sz = (bp->b_bcount + 511) >> 9;
	    pt = &sz_part[unit];
	    if (pt->pt_part[part].pi_nblocks == -1)
		maxsz = sc->sc_disksize[targid] - pt->pt_part[part].pi_blkoff;
	    else
		maxsz = pt->pt_part[part].pi_nblocks;
	    if ((bp->b_blkno < 0) || (bp->b_blkno+sz > maxsz) ||
		(pt->pt_part[part].pi_blkoff >= sc->sc_disksize[targid])) {
		    bad = 1;
			err = ENOSPC;
		    break;
	    }
	    break;
	}
	if (bad) {
	    bp->b_flags |= B_ERROR;
	    bp->b_resid = bp->b_bcount;
	    bp->b_error = err;
	    biodone(bp);
	    return;
	}
	/* TODO: DISKLOG stuff in uda.c?????? */
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
	bp->av_forw = NULL;
	if ((dp->b_active == 0) && (sc->sc_active == 0)) {
		sc->sc_xstate[targid] = SZ_NEXT;
		sc->sc_xevent[targid] = SZ_BEGIN;
		sz_start(sc, targid);
	}
	splx(s);
}

rzread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	int unit = minor(dev) >> 3;

	return (physio(rzstrategy, &rszbuf[unit], dev, B_READ, minphys, uio));
}

rzwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct sz_softc *sc;
	struct uba_device *ui;

	int unit = minor(dev) >> 3;	
	int targid;

	ui = szdinfo[unit];
	targid = ui->ui_slave;
	sc = &sz_softc[ui->ui_ctlr];

	/*
	 * CDROM is read only!
	 */
	if (sc->sc_devtyp[targid] & SZ_CDROM)
	    return(EROFS);

	return (physio(rzstrategy, &rszbuf[unit], dev, B_WRITE, minphys, uio));
}

rzioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct sz_softc *sc;
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	int unit = minor(dev) >> 3;
	int cntlr;
	int targid;

	register struct buf *bp = &cszbuf[unit];
	struct devget *devget;
	int i;
	struct size *stp;
	struct pt *pt = (struct pt *)data;
	int error;
	struct scsi_devtab *sdp;
	struct sz_rdcap_dt *rdp;
	struct sz_rzmodsns_dt *msdp;
	struct page_code_3 *p3;
	struct page_code_4 *p4;
	struct page_code_5 *p5;
	DEVGEOMST *devgeom;

	ui = szdinfo[unit];
	cntlr = ui->ui_ctlr;
        um = szminfo[cntlr];
	targid = ui->ui_slave;
	sc = &sz_softc[cntlr];
	sdp = (struct scsi_devtab *)sc->sc_devtab[targid];
	switch (cmd) {

	case DIOCGETPT:		/* Return disk partition table to user */
	case DIOCDGTPT:		/* Return default disk partition table */
		if (cmd == DIOCGETPT) {
		    /*
		     * Copy pt structure into user's data area.
		     */
		    *pt = sz_part[unit];
		}
		else {
		    /*
		     * Copy the default partition table to user's data area.
		     */
		    stp = sc->sc_dstp[targid];
		    for (i=0; i<8; i++) {
			pt->pt_part[i].pi_nblocks = stp[i].nblocks;
			pt->pt_part[i].pi_blkoff = stp[i].blkoffs;
		    }
		}
		/*
		 * Change all -1 nblocks to disk unit size.
		 */
		for (i=0; i<8; i++) {
		   if (pt->pt_part[i].pi_nblocks == -1)
			pt->pt_part[i].pi_nblocks =
			    sc->sc_disksize[targid] - pt->pt_part[i].pi_blkoff;
		}
		pt->pt_magic = PT_MAGIC;
		break;

	/* TODO1: what if user does this with open no delay? */
	case DIOCSETPT: /* set the driver partition tables */
		/*
		 * Only super users can set the pack's partition table.
		 */
		if (!suser())
		    return(EACCES);

		/*
		 * CDROM is read only, don't allow set partition table.
		 */
		if (sc->sc_devtyp[targid] & SZ_CDROM)
		    return(EROFS);
		
		/*
		 * Before we set the new partition tables make sure
		 * that it will no corrupt any of the kernel data
		 * structures.
		 */
		if ((error = ptcmp(dev, &sz_part[unit], pt)) != 0)
		    return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */
		sz_part[unit] = *pt;

		/*
		 * See if we need to update the superblock of the
		 * "a" partition of this disk.
		 */
		ssblk(dev, pt);

		/*
		 * Just make sure that we set the valid bit.
		 */

		sz_part[unit].pt_valid = PT_VALID;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_DISK;
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
		bcopy("rz", devget->dev_name, 3);
		devget->unit_num = unit;
		devget->soft_count = sc->sc_softcnt[targid];
		devget->hard_count = sc->sc_hardcnt[targid];
		devget->stat = sc->sc_flags[targid];
		if (sc->sc_devtyp[targid] & SZ_CDROM) {
		    /* To make sure we don't break the installation! */
		    devget->stat |= DEV_WRTLCK;
		}
		else {
		    /*
		     * Do a mode sense (page one) to see if the
		     * device is write locked. First one can fail
		     * due to unit attention.
		     * NOTE: RX23 fails if we don't ask for a page.
		     */
		    rzcommand(dev, SZ_MODSNS, 1, 0);
		    if (sc->sc_c_status[targid] != SZ_GOOD)
			rzcommand(dev, SZ_MODSNS, 1, 0);
		    if (sc->sc_c_status[targid] == SZ_GOOD) {
			msdp = (struct sz_rzmodsns_dt *)&sc->sz_dat[targid];
			if (msdp->wp)
			    devget->stat |= DEV_WRTLCK;
		    }
		}
		devget->category_stat = DEV_DISKPART;
		if ((sdp->flags & SCSI_REMOVABLE_DISK) == 0)
		    break;
		/* first one can fail due to unit attention */
		rzcommand(dev, SZ_RDCAP, 1, 0);
		rzcommand(dev, SZ_RDCAP, 1, 0);
		if (sc->sc_c_status[targid] == SZ_GOOD) {
		    rdp = (struct sz_rdcap_dt *)&sc->sz_dat[targid];
		    i = (rdp->lbaddr3 << 24) & 0xff000000;
		    i |= (rdp->lbaddr2 << 16) & 0x00ff0000;
		    i |= (rdp->lbaddr1 << 8) & 0x0000ff00;
		    i |= rdp->lbaddr0 & 0x000000ff;
		    /*
		     * RDCAP returns the address of the last LBN.
		     * We must add one to get the number of LBNs.
		     */
		    i++;
		    switch(i) {
		    case 5760:
			devget->category_stat |= DEV_3_ED2S;
			break;
		    case 2880:
			devget->category_stat |= DEV_3_HD2S;
			break;
		    case 1440:
			if (sc->sc_devtyp[targid] == RX33) {
			    devget->category_stat |= DEV_5_DD2S;
			} else {
			    devget->category_stat |= DEV_3_DD2S;
			}
			break;
		    case 2400:
			devget->category_stat |= DEV_5_HD2S;
			break;
		    case 800:
			devget->category_stat |= DEV_5_DD1S;
			break;
		    case 720:
			devget->category_stat |= DEV_5_LD2S;
			break;
		    default:
			devget->category_stat |= DEV_X_XXXX;
			break;
		    }
		}
		else
		    devget->category_stat |= DEV_X_XXXX;
		devget->category_stat |= DEV_MC_COUNT;
		devget->category_stat |= (sc->sc_mc_cnt[targid] << 16);
		break;

	case DEVGETGEOM:			/* disk geometry info */
		devgeom = (DEVGEOMST *)data;

		bzero(devgeom, sizeof(DEVGEOMST));

		if (sc->sc_rmv_media & (1 << targid))
		    devgeom->geom_info.attributes |= DEVGEOM_REMOVE;

		/*
		 * Get disk size via read capacity command.
		 * First one can fail due to unit attention.
		 */
		rzcommand(dev, SZ_RDCAP, 1, 0);
		if (sc->sc_c_status[targid] != SZ_GOOD)
		    rzcommand(dev, SZ_RDCAP, 1, 0);
		if (sc->sc_c_status[targid] == SZ_GOOD) {
		    rdp = (struct sz_rdcap_dt *)&sc->sz_dat[targid];
		    i = sz_sbtol(&rdp->lbaddr3);
		    /*
		     * RDCAP returns the address of the last LBN.
		     * We must add one to get the number of LBNs.
		     */
		    i++;
		    devgeom->geom_info.dev_size = i;
		}
		else
		    return(EIO);
		/*
		 * Get disk geometry from some combination of mode sense
		 * pages 3, 4, and 5. Normally 3 and 4 for hard disks,
		 * 5 for floppy disks. We get the current values.
		 * NOTE: we check the page code because a brain damaged
		 *       hard disk returned a bogus page instead of a
		 *       check condition for an unsupported page (page 5).
		 */
		rzcommand(dev, SZ_MODSNS, 3, 0);		/* page 3 */
		if (sc->sc_c_status[targid] == SZ_GOOD) {
		    msdp = (struct sz_rzmodsns_dt *)&sc->sz_dat[targid];
		    p3 = (struct page_code_3 *)msdp->mspage;
		    if (p3->pgcode == 3) {
			i = (p3->spt1 << 8) & 0x0000ff00;
			i |= p3->spt0 & 0x000000ff;
			devgeom->geom_info.nsectors = i;
		    }
		}

		rzcommand(dev, SZ_MODSNS, 4, 0);		/* page 4 */
		if (sc->sc_c_status[targid] == SZ_GOOD) {
		    msdp = (struct sz_rzmodsns_dt *)&sc->sz_dat[targid];
		    p4 = (struct page_code_4 *)msdp->mspage;
		    if (p4->pgcode == 4) {
			i = (p4->ncyl2 << 16) & 0x00ff0000;
			i |= (p4->ncyl1 << 8) & 0x0000ff00;
			i |= p4->ncyl0 & 0x000000ff;
			devgeom->geom_info.ncylinders = i;
			devgeom->geom_info.ntracks = p4->nheads;
		    }
		}

		rzcommand(dev, SZ_MODSNS, 5, 0);		/* page 5 */
		if (sc->sc_c_status[targid] == SZ_GOOD) {
		    msdp = (struct sz_rzmodsns_dt *)&sc->sz_dat[targid];
		    p5 = (struct page_code_5 *)msdp->mspage;
		    if (p5->pgcode == 5) {
			i = (p5->num_cyl1 << 8) & 0x0000ff00;
			i |= (p5->num_cyl0 & 0x000000ff);
			devgeom->geom_info.ncylinders = i;
			devgeom->geom_info.ntracks = p5->num_heads;
			devgeom->geom_info.nsectors = p5->sec_per_trk;
		    }
		}

		/* fail if no geometry info available (RRD40 does this) */
		if ((devgeom->geom_info.ntracks == 0) ||
		    (devgeom->geom_info.nsectors == 0) ||
		    (devgeom->geom_info.ncylinders == 0))
			return(EIO);

		break;

	case CDROM_PAUSE_PLAY:
	case CDROM_RESUME_PLAY:
	case CDROM_PLAY_AUDIO:
	case CDROM_PLAY_AUDIO_MSF:
	case CDROM_PLAY_AUDIO_TI:
	case CDROM_PLAY_AUDIO_TR:
	case CDROM_TOC_HEADER:
	case CDROM_TOC_ENTRYS:
	case CDROM_EJECT_CADDY:
	case CDROM_READ_SUBCHANNEL:
	case CDROM_READ_HEADER:
	case CDROM_PLAY_VAUDIO:
	case CDROM_PLAY_MSF:
	case CDROM_PLAY_TRACK:
	case CDROM_PLAYBACK_CONTROL:
	case CDROM_PLAYBACK_STATUS:
	case CDROM_SET_ADDRESS_FORMAT:
		if ((sc->sc_devtyp[targid] & SZ_CDROM) == 0) {
		    return (EINVAL);
		}
		/* FALLTHROUGH */
	case SCSI_STOP_UNIT:
	case SCSI_START_UNIT:
	case SCSI_ALLOW_REMOVAL:
	case SCSI_PREVENT_REMOVAL:
	case SCSI_READ_CAPACITY:
	case SCSI_SEEK_POSITION:
	case SCSI_SEND_DIAGNOSTIC:
	case SCSI_RECEIVE_DIAGNOSTIC:
	case SCSI_FORMAT_UNIT:
	case SCSI_REASSIGN_BLOCK:
	case SCSI_READ_DEFECT_DATA:
	case SCSI_VERIFY_DATA:
	case SCSI_MODE_SENSE:
	case SCSI_MODE_SELECT:
	case SCSI_GET_SENSE:
	case SCSI_GET_INQUIRY_DATA:
	case SCSI_READ_LONG:
	case SCSI_WRITE_LONG:
		return(rzspecial(dev, cmd, data));
		break;

	/* TODO1: do I need DKIOCACC (bbr from sdc.c) radisk uses it */
	default:
		return (ENXIO);
	}
	return (0);
}

/*
 *
 * Name:		rzcomplete	-Disk completion routine
 *
 * Abstract:		This routine is called from the scsi state machine
 *			to perform the completion work for the current data
 *			transfer.
 *
 * Inputs:
 *
 * bp			Buffer pointer of command to be completed.
 *
 * Outputs: 		Only printing debug messages.
 *
 * Return Values: 	Nothing formal, b_resid is updated.
 *
 * Side Effects:
 *			biodone() is called to free up the current bp.
 *			The buffer queue is changed to free up the front entry.
 *			The DBBR code will be called if a read error occured.
 *
 */

int rzcomplete( bp )
    struct buf *bp;
{

    struct sz_softc *sc;
    struct uba_device *ui;
    register struct buf *dp;
    int unit;
    int targid;
    int release;

    unit = minor(bp->b_dev) >> 3;
    ui = szdinfo[unit];
    sc = &sz_softc[ui->ui_ctlr];
    targid = ui->ui_slave;
    dp = &szutab[unit];
    release = 1;		/* release the buffer from the queue */

    PRINTD( targid, 0x01, ("rzcomplete called unit %d\n", unit) );

  /* Check for existing BBR state or a if bad block was encountered. */
    if((sc->sc_szflags[targid] & SZ_RECOVERED) || (sc->sc_bbr_active[ targid ]))
    {
      /* Call the BBR routine to handle the BBR tasks. */
	release = rz_bbr( sc, targid, bp );
    }

    /* Remove the completed request from the queue and release the buffer. */
    /* TODO: are we absolutely sure dp is valid? */

    if( release )
    {
      /* Update the pointer value for bp.  It is possible that the BBR code had
	inserted and removed a buffer on the queue.  If the "current" one did
	belong to the BBR code, it has been removed and the value for bp from
	the parameter call is incorrect.  To be sure to grab the correct buffer
	from the front of the queue, get it via dp. */

        bp = dp->b_actf;                /* go via dp, just in case */
	    
	dp->b_actf = bp->av_forw;
	bp->b_resid = sc->sc_resid[targid];

	PRINTD(targid, 0x5, ("rzcomplete: resid = %d\n", bp->b_resid));

        PRINTD(targid, 0x4, ("biodone on bp %x\n", bp));
	biodone(bp);

	sc->sc_flags[targid] |= DEV_DONE;
	sc->sc_xevent[targid] = SZ_BEGIN;
	sz_retries[sc->sc_unit[targid]] = 0;

	/* The comand has ended */

	dp->b_active = 0;
    }

    PRINTD( targid, 0x01, ("rzcomplete exiting %d\n", unit) );
}

rzspecial(dev, cmd, data)
	dev_t dev;
	register int cmd;
	caddr_t data;
{
	struct format_params *fp; 
	struct reassign_params *rp; 
	struct read_defect_params *rdp; 
	struct verify_params *vp; 
	struct mode_sel_sns_params *msp; 
	struct extended_sense *es; 
	struct defect_descriptors *dd;
	struct inquiry_info *inq;
	struct io_uxfer *iox;
	struct buf *bp;
	register struct uba_device *ui;
	register struct sz_softc *sc;
	int unit;
	int cntlr;
	int reterr;
	int targid;
	int rzaddr_length;

	unit = minor(dev) >> 3;

	ui = szdinfo[unit];
	cntlr = ui->ui_ctlr;
	targid = ui->ui_slave;
	sc = &sz_softc[cntlr];
	bp = &cszbuf[unit];
	reterr = 0;

	/*
	 * CDROM is read only. Cannot allow all commands.
	 */
	if (sc->sc_devtyp[targid] & SZ_CDROM) {

	    switch (cmd) {

		case SCSI_FORMAT_UNIT:
		case SCSI_REASSIGN_BLOCK:
		case SCSI_READ_DEFECT_DATA:
			return (EROFS);

		default:
			break;
	    }
	    /*
	     * Ensure no other I/O is active for this CD-ROM.  This
	     * allows multiple processes to issue audio commands.
	     */
	    if (reterr = rziowait (bp, sc->sc_rzspecial[targid])) {
		return (reterr);
	    }
	} else {
	    if (sc->sc_rzspecial[targid]) {
		return (EBUSY);
	    }
	    /*
	     * Ensure buffer is available before setting the special
	     * flag below, since this flag controls error logging.
	     */
	    if (reterr = rziowait (bp, sc->sc_rzspecial[targid])) {
		return (reterr);
	    }
	}

	/*
	 * Allocate kernel memory for the scsi command ioctl 
	 * parameters and data, and for the defect lists.
	 * Allocate the header once and don't free it.
	 */
	if (sc->sc_rzparams[targid] == NULL) {
		KM_ALLOC(sc->sc_rzparams[targid], char *, 
			sizeof(union rzdisk_params), KM_DEVBUF, KM_CLEAR);
		if(sc->sc_rzparams[targid] == NULL)
			return(ENOMEM);
	}
	PRINTD (targid, 0x20,
		("Allocated rzparam buffer at 0x%x of %d bytes.\n",
		sc->sc_rzparams[targid], sizeof(union rzdisk_params)));

	if ((cmd == SCSI_MODE_SELECT) ||
		(cmd == SCSI_MODE_SENSE) ||
		    (cmd == SCSI_GET_INQUIRY_DATA)) {
		bcopy(data, sc->sc_rzparams[targid], sizeof(*msp));
		msp = (struct mode_sel_sns_params *)sc->sc_rzparams[targid];
		KM_ALLOC(sc->sc_rzaddr[targid], char *, 
			msp->msp_length, KM_DEVBUF, (KM_CLEAR | KM_NOWAIT));
		rzaddr_length = msp->msp_length;
	}
	else {
		KM_ALLOC(sc->sc_rzaddr[targid], char *, 
			sizeof(*dd), KM_DEVBUF, (KM_CLEAR | KM_NOWAIT));
		rzaddr_length = sizeof(*dd);
	}
	PRINTD (targid, 0x20,
		("Allocated rzaddr buffer at 0x%x of %d bytes.\n",
		sc->sc_rzaddr[targid], rzaddr_length));

	if(sc->sc_rzaddr[targid] == NULL)
		return(ENOMEM);

	sc->sc_rzspecial[targid] = 1;	/* Show special command active. */

	switch(cmd) {
	case SCSI_FORMAT_UNIT:
		/*
		 * Must be super user to FORMAT a disk.
		 */
		if (!suser()) {
		    	reterr = EACCES;
			break;
		}
		fp = (struct format_params *)data;
		bcopy(data, sc->sc_rzparams[targid], sizeof(*fp));
		/*
		 * Copy in the defect list from user space.
		 * We don't supply a defect list with the format command.
		 * The length field is always zero.
		 */
		if(fp->fp_length != 0) {
		    if(copyin(fp->fp_addr, sc->sc_rzaddr[targid],
				sizeof(*dd)) != 0) {
			reterr = EFAULT;
			break;
		    }
		}
		if ((reterr = rzcommand (dev, SZ_FORMAT, 1, 0)) == EINTR) {
			break;
		}
		/* mark unit offline (format could change disk's size) */
		sz_unit_online[unit] = 0;
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: format unit failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case SCSI_REASSIGN_BLOCK:
		/*
		 * Must be super user to REASSIGN a bad block.
		 */
		if (!suser()) {
		    	reterr = EACCES;
			break;
		}
		bcopy(data, sc->sc_rzparams[targid], sizeof(*rp));
		if ((reterr = rzcommand (dev, SZ_REASSIGN, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: reassign block failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	
	case SCSI_READ_DEFECT_DATA:
		rdp = (struct read_defect_params *)data;
		bcopy(data, sc->sc_rzparams[targid], sizeof(*rdp));
/*		vslock(rdp->rdp_addr, sizeof(*dd));	*/
		if ((reterr = rzcommand (dev, SZ_RDD, 1, 0)) == EINTR) {
			break;
		}
/*		vsunlock(rdp->rdp_addr, sizeof(*dd), B_READ);	*/
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: read defect data failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		/*
		 * Copy out the defect list to user space.
		 */
		if(copyout(sc->sc_rzaddr[targid], rdp->rdp_addr, 
				sizeof(*dd)) != 0)
		    reterr = EFAULT;
		break;
	
	case SCSI_WRITE_LONG:
		iox = (struct io_uxfer *)data;
		if( iox->io_cnt > sizeof(*dd) ){
		    reterr = EINVAL;
		    break;
		}
		bcopy(data, sc->sc_rzparams[targid], sizeof(*iox));
		/*
		 * Copy in the write long data from user space.
		 */
		if(copyin( iox->io_addr, sc->sc_rzaddr[targid],
				iox->io_cnt) != 0){
		    reterr = EFAULT;
		    break;
		}
		if ((reterr = rzcommand (dev, SZ_WRITEL, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: write lond data failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		break;
	
	case SCSI_READ_LONG:
		iox = (struct io_uxfer *)data;
		if( iox->io_cnt > sizeof(*dd) ){
		    reterr = EINVAL;
		    break;
		}
		bcopy(data, sc->sc_rzparams[targid], sizeof(*iox));
		if ((reterr = rzcommand (dev, SZ_READL, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: read lond data failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		/*
		 * Copy out the read long data to user space.
		 */
		if(copyout(sc->sc_rzaddr[targid], iox->io_addr, 
				iox->io_cnt) != 0)
		    reterr = EFAULT;
		break;

	case SCSI_VERIFY_DATA:
		bcopy(data, sc->sc_rzparams[targid], sizeof(*vp));
		if ((reterr = rzcommand (dev, SZ_VFY_DATA, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: verify data failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	
	case SCSI_MODE_SENSE:
		if ((reterr = rzcommand (dev, SZ_MODSNS, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: mode sense failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		/*
		 * Copy out the mode sense data to user space.
		 */
		if(copyout(sc->sc_rzaddr[targid], msp->msp_addr, 
				msp->msp_length) != 0)
		    reterr = EFAULT;
		break;
	
	case SCSI_MODE_SELECT:
		/*
		 * Must be super user to CHANGE the disk parameters.
		 */
		if ( !(sc->sc_devtyp[targid] & SZ_CDROM) && !suser() ) {
		    	reterr = EACCES;
			break;
		}
		/*
		 * Copy in the mode select data from user space.
		 */
		if(copyin(msp->msp_addr, sc->sc_rzaddr[targid],
				msp->msp_length) != 0) {
			reterr = EFAULT;
			break;
		}
		if ((reterr = rzcommand (dev, SZ_MODSEL, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: mode select failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case SCSI_GET_SENSE:
		es = (struct extended_sense *)data;
		bcopy((char *)&sc->sc_rzsns[targid], data, sizeof(*es));
		bzero((char *)&sc->sc_rzsns[targid], 
				sizeof(sc->sc_rzsns[targid]));
		break;

	case SCSI_GET_INQUIRY_DATA:
		if ((reterr = rzcommand (dev, SZ_INQ, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: inquiry failed\n",
		    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		if(copyout(sc->sc_rzaddr[targid], msp->msp_addr, 
				msp->msp_length) != 0)
		    reterr = EFAULT;
		break;
	
	case SCSI_READ_CAPACITY: {
		struct read_capacity *capacity;

		if ((reterr = rzcommand (dev, SZ_RDCAP, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: read capacity failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
		}
		capacity = (struct read_capacity *)&sc->sz_dat[targid];
		bcopy (capacity, data, sizeof(*capacity));
		break;
	}
	case SCSI_ALLOW_REMOVAL:
		if ((reterr = rzcommand (dev, SZ_MEDREMOVAL, SZ_ALLOW_REMOVAL, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: allow removal failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case SCSI_PREVENT_REMOVAL:
		if ((reterr = rzcommand (dev, SZ_MEDREMOVAL, SZ_PREVENT_REMOVAL, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: prevent removal failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case SCSI_SEND_DIAGNOSTIC: {
		struct diagnostic_params *dp;

		dp = (struct diagnostic_params *)sc->sc_rzparams[targid];
		bcopy (data, dp, sizeof(*dp));
		if (dp->dp_length > rzaddr_length) {
			reterr = EINVAL;
			break;
		}
		/*
		 * Copy in the Diagnostic data (if any) from user space.
		 */
		if (dp->dp_length && dp->dp_buffer) {
		    if (copyin (dp->dp_buffer, sc->sc_rzaddr[targid],
				dp->dp_length) != 0) {
			reterr = EFAULT;
			break;
		    }
		}
		if ((reterr = rzcommand (dev, SZ_SNDDIAG, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: send diagnostic failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	case SCSI_RECEIVE_DIAGNOSTIC: {
		struct diagnostic_params *dp;

		dp = (struct diagnostic_params *)sc->sc_rzparams[targid];
		bcopy (data, dp, sizeof(*dp));
		if (dp->dp_length > rzaddr_length) {
			reterr = EINVAL;
			break;
		}
		if ((reterr = rzcommand (dev, SZ_RECDIAG, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: receive diagnostic failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		/*
		 * Copy out the Diagnostic data (if any) to user space.
		 */
		if (dp->dp_length && dp->dp_buffer) {
		    if (copyout (sc->sc_rzaddr[targid], dp->dp_buffer,
				dp->dp_length) != 0) {
			reterr = EFAULT;
		    }
		}
		break;
	}
	case CDROM_EJECT_CADDY:
		if ((reterr = rzcommand (dev, SZ_P_EJECT, 1, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: eject failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case CDROM_PAUSE_PLAY:
		if ((reterr = rzcommand (dev, SZ_PAUSE_RESUME, SZ_AUDIO_PAUSE, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: pause failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case CDROM_RESUME_PLAY:
		if ((reterr = rzcommand (dev, SZ_PAUSE_RESUME, SZ_AUDIO_RESUME, 0)) == EINTR) {
			break;
		}
		if(sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: resume failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case CDROM_PLAY_AUDIO:
	case CDROM_PLAY_VAUDIO: {
		register struct cd_play_audio *pa;
		int scsi_cmd;

		pa = (struct cd_play_audio *) data;
		bcopy (data, sc->sc_rzparams[targid], sizeof(*pa));
		if (cmd == CDROM_PLAY_AUDIO) {
			scsi_cmd = SZ_PLAY_AUDIO;
		} else {
			scsi_cmd = SZ_PLAY_VAUDIO;
		}
		if ((reterr = rzcommand (dev, scsi_cmd, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: play audio failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	case CDROM_PLAY_MSF:
	case CDROM_PLAY_AUDIO_MSF: {
		register struct cd_play_audio_msf *msf;
		int scsi_cmd;

		msf = (struct cd_play_audio_msf *) data;
		bcopy (data, sc->sc_rzparams[targid], sizeof(*msf));
		if (cmd == CDROM_PLAY_MSF) {
			scsi_cmd = SZ_PLAY_MSF;
		} else {
			scsi_cmd = SZ_PLAY_AUDIO_MSF;
		}
		if ((reterr = rzcommand (dev, scsi_cmd, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: play audio MSF failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	case CDROM_PLAY_AUDIO_TI: {
		register struct cd_play_audio_ti *ti;

		ti = (struct cd_play_audio_ti *) data;
		bcopy (data, sc->sc_rzparams[targid], sizeof(*ti));
		if ((reterr = rzcommand (dev, SZ_PLAY_AUDIO_TI, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: play audio track/index failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	case CDROM_PLAY_AUDIO_TR: {
		register struct cd_play_audio_tr *tr;

		tr = (struct cd_play_audio_tr *) data;
		bcopy (data, sc->sc_rzparams[targid], sizeof(*tr));
		if ((reterr = rzcommand (dev, SZ_PLAY_TRACK_REL, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: play track relative failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	case CDROM_PLAY_TRACK: {
		register struct cd_play_track *pt;

		pt = (struct cd_play_track *) data;
		bcopy (data, sc->sc_rzparams[targid], sizeof(*pt));
		if ((reterr = rzcommand (dev, SZ_PLAY_TRACK, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: play track failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	case SCSI_START_UNIT:
		if ((reterr = rzcommand (dev, SZ_P_SSUNIT, SZ_START_UNIT, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: start failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case SCSI_STOP_UNIT:
		if ((reterr = rzcommand (dev, SZ_P_SSUNIT, SZ_STOP_UNIT, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: stop failed\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;

	case SCSI_SEEK_POSITION: {
		u_long seek_address;

		seek_address = *(u_long *) data;
		if ((reterr = rzcommand (dev, SZ_SEEK_10, seek_address, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: seek position\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	case CDROM_TOC_HEADER: {
		register struct cd_toc *toc;
		register struct cd_toc_header *th;

		toc = (struct cd_toc *)sc->sc_rzparams[targid];
		th = (struct cd_toc_header *)sc->sc_rzaddr[targid];
		toc->toc_starting_track = 0;
		toc->toc_alloc_length = sizeof(*th);
		if ((reterr = rzcommand (dev, SZ_READ_TOC, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: read TOC failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		bcopy (th, data, sizeof(*th));
		break;
	}
	case CDROM_TOC_ENTRYS: {
		register struct cd_toc *toc;
		register struct cd_toc_header *th;
		int toc_length;

		toc = (struct cd_toc *)sc->sc_rzparams[targid];
		bcopy (data, toc, sizeof(*toc));
		if (toc->toc_alloc_length > rzaddr_length) {
			reterr = EINVAL;
			break;
		}
		if ((reterr = rzcommand (dev, SZ_READ_TOC, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: read TOC entry failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		th = (struct cd_toc_header *)sc->sc_rzaddr[targid];
		/*
		 * Calculate the TOC length.
		 */
		toc_length = (int) (((th->th_data_len1 << 8) +
			      th->th_data_len0) + sizeof(*th)) & 0xffff;

		if (toc_length > rzaddr_length) {
		    PRINTD (targid, 0x20,
	     ("rzspecial: toc_length (%d) > rzaddr_length (%d), adjusting...\n",
						toc_length, rzaddr_length));
		    toc_length = rzaddr_length;
		}
		/*
		 * Copy out the TOC header & entries to user space.
		 */
		if (copyout (th, toc->toc_buffer, toc_length) != 0) {
		    reterr = EFAULT;
		}
		break;
	}
	case CDROM_READ_SUBCHANNEL: {
		register struct cd_sub_channel *sch;
		register struct cd_subc_header *sh;
		int subc_length;

		sch = (struct cd_sub_channel *)sc->sc_rzparams[targid];
		bcopy (data, sch, sizeof(*sch));
		if (sch->sch_alloc_length > rzaddr_length) {
			reterr = EINVAL;
			break;
		}
		if ((reterr = rzcommand (dev, SZ_READ_SUBCHAN, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: read sub-channel failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		sh = (struct cd_subc_header *)sc->sc_rzaddr[targid];
		/*
		 * Calculate the Sub-Channel length.
		 */
		subc_length = (int) (((sh->sh_data_len1 << 8) +
				sh->sh_data_len0) + sizeof(*sh)) & 0xffff;

		if (subc_length > rzaddr_length) {
		    mprintf("rzspecial: %s unit %d: subc_length (%d) > rzaddr_length (%d)\n",
		    sc->sc_device[targid], unit, subc_length, rzaddr_length);
		    reterr = EFAULT;
		    break;
		}
		/*
		 * Copy out the Sub-Channel information to user space.
		 */
		if (copyout (sh, sch->sch_buffer, subc_length) != 0) {
		    reterr = EFAULT;
		}
		break;
	}
	case CDROM_READ_HEADER: {
		register struct cd_read_header *rh;
		register struct cd_read_header_data *rhd;

		rh = (struct cd_read_header *)sc->sc_rzparams[targid];
		bcopy (data, rh, sizeof(*rh));
		if (rh->rh_alloc_length > rzaddr_length) {
			reterr = EINVAL;
			break;
		}
		if ((reterr = rzcommand (dev, SZ_READ_HEADER, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: read header failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		rhd = (struct cd_read_header_data *)sc->sc_rzaddr[targid];
		/*
		 * Copy out the Read Header Data to user space.
		 */
		if (copyout (rhd, rh->rh_buffer, sizeof(*rhd)) != 0) {
		    reterr = EFAULT;
		}
		break;
	}
	case CDROM_PLAYBACK_CONTROL: {
		register struct cd_playback *pb;
		register struct cd_playback_control *pc;

		pb = (struct cd_playback *)sc->sc_rzparams[targid];
		pc = (struct cd_playback_control *)sc->sc_rzaddr[targid];
		bcopy (data, pb, sizeof(*pb));
		if (pb->pb_alloc_length > rzaddr_length) {
			reterr = EINVAL;
			break;
		}
		/*
		 * Copy in the Playback Control data from user space.
		 */
		if (copyin (pb->pb_buffer, sc->sc_rzaddr[targid],
				pb->pb_alloc_length) != 0) {
			reterr = EFAULT;
			break;
		}
		if ((reterr = rzcommand (dev, SZ_PLAYBACK_CONTROL, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: playback control failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		break;
	}
	case CDROM_PLAYBACK_STATUS: {
		register struct cd_playback *pb;
		register struct cd_playback_status *ps;

		pb = (struct cd_playback *)sc->sc_rzparams[targid];
		bcopy (data, pb, sizeof(*pb));
		if (pb->pb_alloc_length > rzaddr_length) {
			reterr = EINVAL;
			break;
		}
		if ((reterr = rzcommand (dev, SZ_PLAYBACK_STATUS, 1, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf("rzspecial: %s unit %d: playback status failed\n",
				    sc->sc_device[targid], unit);
		    reterr = EIO;
		    break;
	        }
		ps = (struct cd_playback_status *)sc->sc_rzaddr[targid];
		/*
		 * Copy out the Playback Status information to user space.
		 */
		if (copyout (ps, pb->pb_buffer, sizeof(*ps)) != 0) {
		    reterr = EFAULT;
		}
		break;
	}
	case CDROM_SET_ADDRESS_FORMAT: {
		int address_format;

		address_format = *(int *) data;
		if ((reterr = rzcommand (dev, SZ_SET_ADDRESS_FORMAT,
					address_format, 0)) == EINTR) {
			break;
		}
		if (sc->sc_c_status[targid] != SZ_GOOD) {
		    mprintf ("rzspecial: %s unit %d: set address format\n",
					    sc->sc_device[targid], unit);
		    reterr = EIO;
	        }
		break;
	}
	default:
		reterr = ENXIO;
		break;
	}
	sc->sc_rzspecial[targid] = 0;
	KM_FREE(sc->sc_rzaddr[targid], KM_DEVBUF);
	return(reterr);
}

rzdump()
{
}

/*
 *
 * Name:		rz_bbr		-Disk bad block replacement routine
 *
 * Abstract:		This routine is called from the disk completion routine
 *			to perform BBR on LBN contained in the sense info.
 *			The LBN is read, reassigned, and written back.  The 
 *			original request is repeated and it's status returned
 *			to the user.
 * Inputs:
 *
 * sc			Pointer to controller's sz_softc structure.
 * targid		Device SCSI target ID (0 - 7).
 * cbp			Buffer pointer of the current command.
 *
 * Outputs:		Error messages depending on the success/fail of the BBR
 *			progress.
 *
 * Return Values: 	A flag back to the completion routine signaling what
 *			to do with the existing buffer on the front of the
 *			queue.
 *
 * Side Effects: 	The buffer queue is changed to insert or remove the
 *			bbr buffer on the front.  The state machine is entered
 *			many times.  However the current operation is repeated
 *			to get the necessary information back to the user.
 *			The macros KM_ALLOC/KM_FREE are called.
 *
 * To Do:		Make the BBR SM multipass, w/single pass it is a little
 *			ugly.  Work out a good handshake with rzdisk.  If rzdisk
 *			goes to replace a ECC block, this routine will do it
 *			before the read data gets back to rzdisk.  Workup a way
 *			to turnoff error logging ?
 */

int
rz_bbr( sc, targid, cbp )
    register struct sz_softc *sc;
    register int targid;
    struct buf *cbp;			/* bp for current operation */
{
    register struct buf *dp;
    register struct buf *bp;
    struct reassign_params *rp;
    int unit;
    int release;
    int flags;

    char *p;		/* general purpose byte pointers */
    char *q;
    int i;

    unit = minor(cbp->b_dev) >> 3;
    dp = &szutab[ unit ];
    bp = &bszbuf[ targid ];

    release = 1;		/* Default: release front buffer */

  /* Check to see if DBBR is disabled for this device.  If the SCSI_NODBBR
    flag is set return to the completion routine. */

    if( ((sc->sc_devtab[targid])->flags & SCSI_NODBBR ) != 0)
	return( release );

  /* Check the active flag, is the target already in BBR state. */

    if( !( sc->sc_bbr_active[ targid ] == 1 ))
    {
      /* Check out the additional sence codes.  BBR will be done on ECC level
	errors.  Read retry errors for now will be ignored. */

	if( sc->sc_sns[targid].asb.rz_asb.asc == SZ_ASC_RRETRY )
	{
	  /* On a read retry, simply leave and allow the current buffer to 
	    complete. */

	    PRINTD( targid, 0x200, ("BBR Recovered error a read retry\n"));
	    return( 1 );	/* free up the buffer */

	  /* FUTURE: save LBN for compare on next RRETRY */
	}
	else if( sc->sc_sns[targid].asb.rz_asb.asc != SZ_ASC_RERROR )
	{
	  /* On a unhandled error code, simply leave and allow the current
	    buffer to complete. */

	    PRINTD( targid, 0x200, ("BBR Recovered error not a read ECC\n"));
	    return( 1 );	/* free up the buffer */
	}

      /* On an error with ECC correction, go through the BBR process. */

      /* Allocate a working data buffer for the read data. Two 512 chuncks
	are allocated one is for the read/write data the other is for the
	reassign data block containing the bad block info. */
/* JAG remove magic ##'s and use defines */

	KM_ALLOC( sc->sc_bbraddr[targid], char *, 
			1024, KM_DEVBUF, (KM_NOWAIT|KM_CLEAR) );

	if(sc->sc_bbraddr[targid] == NULL)
	{
	    mprintf("rz_bbr: Unable to alloc working buffer for DBBR\n" );
	    return( 1 );	/* free up the buffer */
	}
	sc->sc_bbrparams[targid] = sc->sc_bbraddr[targid] + 512;

      /* Load the bp etal with what ever is needed.  Then let the SCSI State
        Machine handle the work.  The BBR states will handle "what just 
	happened". */

	bp->b_flags = (B_BUSY | B_READ );	/* read into ker mem */
	bp->b_un.b_addr = sc->sc_bbraddr[ targid ];	/* where to read */
	bp->b_bcount = 512;				/* MAGIC # one block */
	bp->b_retry = 0;				/* no retry in SM */
	bp->b_resid = 0;

      /* Setup the block # and dev entry for the bad block.  The block is kept
	in the info bytes of the sence data.  The block is "absolute" from the
	drives point of view.  The b_dev entry has to be modified to fake the
	state machine out to use C partition */

	bp->b_dev = (((cbp->b_dev) & ~(7)) | 2 );	/* C is #2 */
	bp->b_blkno = sz_sbtol( &(sc->sc_sns[targid].infobyte3) ); /* LBN # */

      /* Pre-load the reassign parameter part of the data block.  Only the one
	block will be reassigned.  This information will be use in the 
	reassign part of the SM. */

	rp = (struct reassign_params *)sc->sc_bbrparams[ targid ];
	rp->rp_header.defect_len1 = 0;
	rp->rp_header.defect_len0 = 4;		/* only 1 LBN = 4 bytes */

	p = (char *)&(rp->rp_header.defect_len0);
	p++;				/* inc to the defect desc section */
	q = (char *)&(sc->sc_sns[ targid ].infobyte3); 	/* LBN # */

	for( i = 0; i < 4; i++ )	/* xfer the bytes from sns to rp */
	{
	    *p++ = *q++;
	}

      /* Set the BBR active flag, and place the buffer onto the front of the
	queue.  This will leave the bp that "failed" still on the queue and
	next in line.  When the BBR has completed, this bp can be acted upon.

      ASSUMPTION: Playing with the queue.  The priority level should still
	be high enough to block. */

	sc->sc_bbr_active[ targid ] = 1;	/* now ready to work */

	bp->b_actf = dp->b_actf;	/* copy forward link */
	dp->b_actf = bp;		/* add to the front of the queue */

	sc->sc_bbr_state[targid] = BBR_READ;
	sc->sc_bbr_read[targid] = BBR_COUNT;	/* counts decrement */
	sc->sc_bbr_oper[targid] = SZ_RW_START;	/* signal a normal cmd */

	sz_retries[unit] = SZ_RW_RTCNT;		/* preset cnt for no retries */

      /* Setup the state for the SM and clear active.  The SM will scan the
	queue and find the next available entry to startup. */

	sc->sc_xstate[targid] = SZ_NEXT;
	sc->sc_xevent[targid] = SZ_BEGIN;
	dp->b_active = 0;		/* no longer an active queue */

    }
    else
    {
      /* The target is in BBR state.  Using the bbr state variables in the sc
	structure work on what is needed.  */

	PRINTD( targid, 0x200, ("BBR state: %d\n", sc->sc_bbr_state[targid]));
	switch( sc->sc_bbr_state[ targid ] )
	{
	    case BBR_READ :
	      /* Check the sence key(s) has the block gone from bad to good
		or worse ? */

		if( sc->sc_sns[targid].snskey == SZ_NOSENSE )
		{
		  /* Stop BBR things appear to be back to normal. */

		    sc->sc_bbr_active[ targid ] = 0;	/* all done */
		    break;
		}

		if( sc->sc_sns[targid].snskey == SZ_MEDIUMERR )
		{
		  /* The block has completly gone bad.  Stop any BBR, report
		    the error, logging was disabled in the SM during BBR. */
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 0, 0, flags);

		    sc->sc_bbr_active[ targid ] = 0;	/* all done */
		    break;
		}

	      /* Check the read counts for BBR_READ.  If the count is not 0
		try the read again. */

		if( --sc->sc_bbr_read[targid] != 0 )
		{
		  /* Restart the current bp at the front of the queue. */

		    bp->b_flags = (B_BUSY | B_READ );	/* reload */
		    bp->b_bcount = 512;				/* MAGIC # */
		}
		else
		{
		  /* Setup the BBR related stuff.  The parameter and defect 
		    information has already been setup. */

		    sc->sc_bbr_state[targid] = BBR_REASSIGN;
		    sc->sc_bbr_rawr[targid] = BBR_COUNT;	/* counts dec */
		    sc->sc_bbr_oper[targid] = SZ_SP_START;	/* special */

		    sz_retries[unit] = SZ_SP_RTCNT;	/* preset: no retries */

		  /* Setup the buffer to contain the information needed for
		    the reassign. */

		    bp->b_comand = SZ_REASSIGN;

		  /* Use the rzspecial flag to fake out the SM and have it
		    use some of the logic for reassign. */

		    sc->sc_rzspecial[ targid ] = 1;
		    sc->sc_rzparams[ targid ] = sc->sc_bbrparams[ targid ];
		}
	    break;

	    case BBR_REASSIGN :

	      /* Clear the rzspecial flag it is used only during the Reassign
		and should not be around. */

		sc->sc_rzspecial[ targid ] = 0;

	      /* Was the reassign successfull?  If not fail with an error
		message.  Else go an try the write. */

		if( sc->sc_sns[targid].snskey != SZ_NOSENSE ) 
		{
		    flags = SZ_HARDERR;
		    scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 1, 0, flags);

		    sc->sc_bbr_active[ targid ] = 0;	/* error out */
		}
		else
		{
		  /* Setup for the write of the buffered data. */

		    sc->sc_bbr_state[targid] = BBR_WRITE;
		    sc->sc_bbr_write[targid] = BBR_COUNT;	/* counts dec */
		    sc->sc_bbr_oper[targid] = SZ_RW_START;	/* normal cmd */

		    sz_retries[unit] = SZ_RW_RTCNT;	/* preset: 0 retries */

		    bp->b_flags = ( B_BUSY );	/* write from ker mem */
		    bp->b_bcount = 512;			/* MAGIC # one block */
		    bp->b_retry = 0;			/* no retry in SM */
		    bp->b_resid = 0;
		}
	    break;

	    case BBR_WRITE :

	      /* Check the sence key(s) has the block successufully been
		written to the disk ?. */

		if( sc->sc_sns[targid].snskey == SZ_NOSENSE )
		{
		  /* Stop BBR the write has completed.  The previous buffer
		    on the queue will be re-tried to allow the command to
		    retry. */
		    flags = SZ_SOFTERR;
		    scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 2, 0, flags);

		    sc->sc_bbr_active[ targid ] = 0;	/* all done */
		    release = 0;			/* repeat the command */
		    break;
		}

	      /* Check the write counts for BBR_WRITE.  If the count is not 0
		try the write again.  If the write counts have reached zero
		try to reassign the block again. */

		if( --sc->sc_bbr_write[targid] != 0 )
		{
		  /* Restart the current bp at the front of the queue. */

		    bp->b_flags = ( B_BUSY );		/* reload */
		    bp->b_bcount = 512;				/* MAGIC # */
		}
		else
		{
		  /* Check the reassign/write count.  Reset the BBR state to
		    reassign and try again.  If the reassign/write counts
		    have also reached zero, stop BBR and report the failure. */

		    if( --sc->sc_bbr_rawr[targid] != 0 )
		    {
		      /* Setup the BBR related stuff.  The parameter and defect 
			information has already been setup. */

			sc->sc_bbr_state[targid] = BBR_REASSIGN;
			sc->sc_bbr_oper[targid] = SZ_SP_START; /* special cmd */

			sz_retries[unit] = SZ_SP_RTCNT;	/* preset for 0 retry */

		      /* Setup the buffer to contain the information needed for
			the reassign. */

			bp->b_comand = SZ_REASSIGN;

		      /* Use the rzspecial flag to fake out the SM and have it
			use some of the logic for reassign. */

			sc->sc_rzspecial[ targid ] = 1;
			sc->sc_rzparams[ targid ] = sc->sc_bbrparams[ targid ];
		    }
		    else	/* reassign-write count == 0 */
		    {
		      /* The BBR has failed, report the error and terminate
			the BBR state. */
			flags = SZ_HARDERR;
			scsi_logerr(sc, bp, targid, SZ_ET_DBBR, 3, 0, flags);

			sc->sc_bbr_active[targid] = 0;	/* stop */
		    }
		}
	    break;

	    default:		 /* something is wrong release the buffer */
		sc->sc_bbr_active[ targid ] = 0;
	    break;
	}
    }

  /* Check out the BBR active flag.  If the target is still in BBR mode
    leave the routine.  If BBR is done free up the allocated memory and
    remove the BBR bp and allow the front buffer to be dequeued.  If active
    is set setup the state for the SM. */

    if( sc->sc_bbr_active[ targid ] == 0 )
    {
	PRINTD( targid, 0x200, ("BBR done: cleaning up\n"));
	KM_FREE(sc->sc_bbraddr[targid], KM_DEVBUF);
	sc->sc_bbr_oper[targid] = 0;	/* clear the opcode */

	bp = dp->b_actf;		/* pop off BBR buffer */
	dp->b_actf = bp->b_actf;
    }
    else
    {
      /* Set the release flag to "no", the state for the command will be setup
	so that the SM will scan the queue and find the next available entry
	to startup.  The snskey is pre-cleared, it is not cleared with in the
	SM. */

	PRINTD( targid, 0x200, ("BBR active setup for SM\n"));

	sc->sc_sns[targid].snskey = SZ_NOSENSE;		/* to make sure */
	release = 0;			/* do not remove front buffer */
    }

  /* Return what the completion code needes to do with the existing buffer
    on the front of the queue. */

    if( release == 0 )
    {
      /* Setup the state for the SM and clear b_active.  The SM will scan the
	queue and find the next available entry to startup.  The snskey is
	pre-cleared, it is not cleared with in the SM. */

	sc->sc_xstate[targid] = SZ_NEXT;
	sc->sc_xevent[targid] = SZ_BEGIN;
	dp->b_active = 0;		/* no longer an active queue */
    }

    return( release );
}
