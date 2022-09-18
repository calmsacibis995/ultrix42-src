#ifndef lint
static char *sccsid = "@(#)rl.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1988, 1989 by		*
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

/*
 * rl.c    6.1	   07/29/83
 *
 * Modification history
 *
 * RLU211/RLV211/RL02 disk driver
 *
 * 26-Oct-84 - Stephen Reilly
 *
 *	Derived from 4.2BSD labeled: rl.c	6.1	83/07/29.
 *	Modified driver to handle the disk partition scheme. -001
 *
 * 24-Sep-85 - Stephen Reilly
 *
 *	Added new ioctl request that will return the default
 *	partition table. -002
 *
 * 18-Mar-86 - jaw
 *
 *	br/cvec changed to NOT use registers.
 *
 * 16-Apr-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code along with
 *	soft and hard error counters. V2.0
 *
 * 13-Jun-86 - jaw
 *
 *	Fix to uba reset and drivers.
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *

 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 * 04-Dec-86 - pmk
 *	Changed mprintf's to log just one entry.
 *
 * 24-Jul-88 - dallas
 * 	changed rlintr to fix the drive hanging problem. We would
 * 	forget about the attention summary if we needed to do another
 *	seek to finish off this transfer.
 * 
 * 13-Aug-89 - Tim Burke
 *	Prevent driver hangs by putting timeouts in the wait routines.
 *	This is needed to prevent the driver from spinning forever waiting
 *	for a bit which will never clear.
 *
 *	Modified the open routine to correctly determine the drive's status
 *	so that it doesn't try to perform operations on an offline drive
 *	which was hanging the system when calling the wait routines which
 *	previously did not have any timeouts.  The wait routines would spin
 *	forever because the drive was not online so it would never become ready.
 */

#include "rl.h"
#if NRL > 0 || defined(BINARY)

#include "../data/rl_data.c"

int	rlprobe(), rlslave(), rlattach(), rldgo(), rlintr();

/* RL02 driver structure */
u_short rlstd[] = { 0174400 };
struct	uba_driver hldriver =
    { rlprobe, rlslave, rlattach, rldgo, rlstd, "rl", rldinfo, "hl", rlminfo };

/* RL02 drive structure */
struct	RL02 {
	short	nbpt;		/* Number of 512 byte blocks/track */
	short	ntrak;
	short	nbpc;		/* Number of 512 byte blocks/cylinder */
	short	ncyl;
	short	btrak;		/* Number of bytes/track */
	struct	size *sizes;
} rl02 = {
	20,	2,	40,	512,	20*512, rl02_sizes /* rl02/DEC*/
};

int rldebug = 0;		/* Print out debug messages */

#define b_cylin b_resid 	/* Last seek as CYL<<1 | HD */

#ifdef INTRLVE
daddr_t dkblock();
#endif INTRLVE

int	rlwstart, rlwatch();		/* Have started guardian */

/* Check that controller exists */
/*ARGSUSED*/
rlprobe(reg)
	caddr_t reg;
{

#ifdef lint
	rlintr(0);
#endif
	((struct rldevice *)reg)->rlcs = RL_IE | RL_NOOP;
	DELAY(100000);
	((struct rldevice *)reg)->rlcs &= ~RL_IE;
	return (sizeof (struct rldevice));
}

rlslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{
	register struct rldevice *rladdr = (struct rldevice *)reg;
	short ctr = 0;

	/*
	 * DEC reports that:
	 * For some unknown reason the RL02 (seems to be only drive 1)
	 * does not return a valid drive status the first time that a
	 * GET STATUS request is issued for the drive, in fact it can
	 * take up to three or more GET STATUS requests to obtain the
	 * correct status.
	 * In order to overcome this, the driver has been modified to
	 * issue a GET STATUS request and validate the drive status
	 * returned.  If a valid status is not returned after eight
	 * attempts, then an error message is printed.
	 */
	 /* s.p. miller 11/83
	  * change test of "ok" status such that only care if we get
	  * a composite error (MSB) and return in CSR.	So, for
	  * autoconfigure, device is
	  * considered ok even if spun down, as long as it is there.
	  */
	do {
		rladdr->rlda.getstat = RL_RESET;
		rladdr->rlcs = (ui->ui_slave <<8) | RL_GETSTAT; /* Get status*/
		rlwait(rladdr);
	    }
	while ((rladdr->rlcs&RLCS_STATUS) != RLCS_STATOK && ++ctr<8);

	if ((rladdr->rlcs & RL_DE) || (ctr >= 8)) {
		return (0);
	}
	if ((rladdr->rlmp.getstat & RLMP_DT) == 0 ) {
		printf("rl%d: rl01's not supported\n", ui->ui_slave);
		return(0);
	}
	return (1);
}

rlattach(ui)
	register struct uba_device *ui;
{
	register struct rldevice *rladdr;
	register struct rl_softc *sc = &rl_softc[ui->ui_ctlr];

	if (rlwstart == 0) {
		timeout(rlwatch, (caddr_t)0, hz);
		rlwstart++;
	}
	/* Initialize iostat values */
	if (ui->ui_dk >= 0) {
		dk_mspw[ui->ui_dk] = .000003906;   /* 16bit transfer time? */
		bcopy(DEV_RL02,sc->sc_device[ui->ui_unit],
		      strlen(DEV_RL02));
		sc->sc_softcnt[ui->ui_unit] = 0;
		sc->sc_hardcnt[ui->ui_unit] = 0;
	}
	rlip[ui->ui_ctlr][ui->ui_slave] = ui;
	sc->rl_ndrive++;
	rladdr = (struct rldevice *)ui->ui_addr;
	/* reset controller */
	rladdr->rlda.getstat = RL_RESET;	/* SHOULD BE REPEATED? */
	rladdr->rlcs = (ui->ui_slave << 8) | RL_GETSTAT; /* Reset DE bit */
	rlwait(rladdr);
	/* determine disk posistion */
	rladdr->rlcs = (ui->ui_slave << 8) | RL_RHDR;
	rlwait(rladdr);
	/* save disk drive posistion */
	rl_stat[ui->ui_ctlr].rl_cyl[ui->ui_slave] =
	     (rladdr->rlmp.readhdr & 0177700) >> 6;
	rl_stat[ui->ui_ctlr].rl_dn = -1;
}

rlopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui;
	register struct rl_softc *sc;
	struct uba_ctlr *um;
	register struct rldevice *rladdr;
	register int i = 0; 			/* 001 */
	int rlstrategy();			/* 001 */

	if (unit >= nNRL || (ui = rldinfo[unit]) == 0 ||
	    ui->ui_alive == 0) {
		return (ENXIO);
	}

	um = ui->ui_mi;
	rladdr = (struct rldevice *)um->um_addr;
	sc = &rl_softc[ui->ui_ctlr];
	sc->sc_flags[ui->ui_unit] = 0;
	sc->sc_category_flags[ui->ui_unit] = 0;

	do {
		rladdr->rlda.getstat = RL_GSTAT;
		rladdr->rlcs = (ui->ui_slave <<8) | RL_GETSTAT;
		rlwait(rladdr);
	    }
	while ((rladdr->rlcs&RLCS_STATUS) != RLCS_STATOK && ++i<8)
		;
	i = rladdr->rlmp.getstat;
	switch( i & RLMP_STATMASK ) {
		case RLMP_SPINDOWN:
			sc->sc_flags[ui->ui_unit] |= DEV_OFFLINE;
			if (rldebug > 1)
				printf("rlopen: unit %d spun down.\n",
					ui->ui_unit);
			break;
		case RLMP_UNLOADED:
			sc->sc_flags[ui->ui_unit] |= DEV_OFFLINE;
			if (rldebug > 1)
				printf("rlopen: unit %d unloaded.\n",
					ui->ui_unit);
			break;
		default:
			sc->sc_flags[ui->ui_unit] &= ~DEV_OFFLINE;
	} ;
	if (i & RLMP_WL) {
		sc->sc_flags[ui->ui_unit] |= DEV_WRTLCK;
	}

	/*
	 *	We must check to see if the pack on the drive has a
	 *	partition table on it, but first set the default
	 *	partition tables becuase the strategy routine need them.
	 *
	 *	Only attempt disk activity to an online drive.
	 */

        if(((sc->sc_flags[ui->ui_unit] & DEV_OFFLINE) == 0) &&
	   (rl_part[unit].pt_valid == 0)) {
		int nbpc = rl02.nbpc;
		/*
		 *	Set default partition tables
		 */
		for( i = 0; i <= 7; i++ ) {
			rl_part[unit].pt_part[i].pi_nblocks = rl02_sizes[i].nblocks;
				rl_part[unit].pt_part[i].pi_blkoff =
					rl02_sizes[i].cyloff * nbpc;
			}

		rl_part[unit].pt_valid = PT_VALID;	/*001 Validate the pt*/

			/*
			 *	Default partition are now set. Call rsblk to set
			 *	the driver's partition tables, if any exists, from
			 *	the "a" partition superblock
			 */

		rsblk( rlstrategy, dev, &rl_part[unit] );
	}
	return (0);
}

rlstrategy(bp)
	register struct buf *bp;
{
	register int unit = dkunit(bp);
	register struct uba_device *ui = rldinfo[unit];
	register struct pt *pt; 			 /* 001 */
	register struct buf *dp;
	register struct rl_softc *sc = &rl_softc[ui->ui_ctlr];
	int xunit = minor(bp->b_dev) & 07;
	long bn, sz;
	int s;

	sz = (bp->b_bcount+511) >> 9;
	if (unit >= nNRL)
		goto bad;
	if ((ui == 0) || (ui->ui_alive == 0) ||
		(sc->sc_flags[ui->ui_unit] & DEV_OFFLINE))
		goto bad;
	pt = &rl_part[unit];
	if ( pt->pt_valid != PT_VALID ) 			/* 001 */
		panic("rlstrategy: invalid partition table ");	/* 001 */
	if (bp->b_blkno < 0 ||
	    ((bn = dkblock(bp))+sz > pt->pt_part[xunit].pi_nblocks /* 001*/
	    && pt->pt_part[xunit].pi_nblocks >= 0)) {
		sc->sc_flags[ui->ui_unit] |= DEV_EOM;
		goto bad;
	}
	/* 001 bn is in 512 byte block size */
	bp->b_cylin = bn/rl02.nbpc + pt->pt_part[xunit].pi_blkoff / rl02.nbpc;
	s = spl5();
	dp = &rlutab[ui->ui_unit];
	disksort(dp, bp);
	if (dp->b_active == 0) {
		rlustart(ui);
		bp = &ui->ui_mi->um_tab;
		if (bp->b_actf && bp->b_active == 0)
			rlstart(ui->ui_mi);
	}
	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
	if (sc->sc_flags[ui->ui_unit] & DEV_EOM) {
		bp->b_error = ENOSPC;
	}
	iodone(bp);
	return;
}

/*
 * Unit start routine.
 * Seek the drive to be where the data is
 * and then generate another interrupt
 * to actually start the transfer.
 */
rlustart(ui)
	register struct uba_device *ui;
{
	register struct buf *bp, *dp;
	register struct uba_ctlr *um;
	register struct rldevice *rladdr;
	register struct rl_softc *sc;
	daddr_t bn;
	short hd, diff;

	if (ui == 0)
		return;
	um = ui->ui_mi;
	sc = &rl_softc[um->um_ctlr];
	dk_busy &= ~(1 << ui->ui_dk);
	dp = &rlutab[ui->ui_unit];
	if ((bp = dp->b_actf) == NULL)
		return;
	/*
	 * If the controller is active, just remember
	 * that this device has to be positioned...
	 */
	if (um->um_tab.b_active) {
		sc->rl_softas |= (1<<ui->ui_slave);
		return;
	}
	/*
	 * If we have already positioned this drive,
	 * then just put it on the ready queue.
	 */
	if (dp->b_active)
		goto done;
	dp->b_active = 1;	/* positioning drive */
	rladdr = (struct rldevice *)um->um_addr;

	/*
	 * Figure out where this transfer is going to
	 * and see if we are seeked correctly.
	 */
	bn = dkblock(bp);		/* Block # desired */
	/*
	 * Map 512 byte logical disk blocks
	 * to 256 byte sectors (rl02's are stupid).
	 */
	hd = (bn / rl02.nbpt) & 1;	/* Get head required */
	diff = (rl_stat[um->um_ctlr].rl_cyl[ui->ui_slave] >> 1) - bp->b_cylin;
	if ( diff == 0 && (rl_stat[um->um_ctlr].rl_cyl[ui->ui_slave] & 1) == hd)
		goto done;		/* on cylinder and head */
	/*
	 * Not at correct position.
	 */
	rl_stat[um->um_ctlr].rl_cyl[ui->ui_slave] = (bp->b_cylin << 1) | hd;
	if (diff < 0)
		rladdr->rlda.seek = -diff << 7 | RLDA_HGH | hd << 4;
	else
		rladdr->rlda.seek = diff << 7 | RLDA_LOW | hd << 4;
	rladdr->rlcs = (ui->ui_slave << 8) | RL_SEEK;

	/*
	 * Mark unit busy for iostat.
	 */
	if (ui->ui_dk >= 0) {
		dk_busy |= 1<<ui->ui_dk;
		dk_seek[ui->ui_dk]++;
	}
	rlwait(rladdr);
done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller
	 * (unless its already there.)
	 */
	if (dp->b_active != 2) {
		dp->b_forw = NULL;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else
			um->um_tab.b_actl->b_forw = dp;
		um->um_tab.b_actl = dp;
		dp->b_active = 2;	/* Request on ready queue */
	}
}

/*
 * Start up a transfer on a drive.
 */
rlstart(um)
	register struct uba_ctlr *um;
{
	register struct buf *bp, *dp;
	register struct uba_device *ui;
	register struct rldevice *rladdr;
	register struct rl_stat *st = &rl_stat[um->um_ctlr];
	daddr_t bn;
	short sn, cyl, cmd;

loop:
	if ((dp = um->um_tab.b_actf) == NULL) {
		st->rl_dn = -1;
		st->rl_cylnhd = 0;
		st->rl_bleft = 0;
		st->rl_bpart = 0;
		return;
	}
	if ((bp = dp->b_actf) == NULL) {
		um->um_tab.b_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy, and
	 * determine destination.
	 */
	um->um_tab.b_active++;
	ui = rldinfo[dkunit(bp)];		/* Controller */
	bn = dkblock(bp);			/* 512 byte Block number */
	cyl = bp->b_cylin << 1; 		/* Cylinder */
	cyl |= (bn / rl02.nbpt) & 1;		/* Get head required */
	sn = (bn % rl02.nbpt) << 1;		/* Sector number */
	rladdr = (struct rldevice *)ui->ui_addr;
	rlwait(rladdr);
	rladdr->rlda.rw = cyl<<6 | sn;
	/* save away current transfers drive status */
	st->rl_dn = ui->ui_slave;
	st->rl_cylnhd = cyl;
	st->rl_bleft = bp->b_bcount;
	st->rl_bpart = rl02.btrak - (sn * NRLBPSC);
	/*
	 * RL02 must seek between cylinders and between tracks,
	 * determine maximum data transfer at this time.
	 */
	if (st->rl_bleft < st->rl_bpart)
		st->rl_bpart = st->rl_bleft;
	rladdr->rlmp.rw = -(st->rl_bpart >> 1);
	if (bp->b_flags & B_READ)
		cmd = RL_IE | RL_READ | (ui->ui_slave << 8);
	else
		cmd = RL_IE | RL_WRITE | (ui->ui_slave << 8);
	um->um_cmd = cmd;
	(void) ubago(ui);
}

rldgo(um)
	register struct uba_ctlr *um;
{
	register struct rldevice *rladdr = (struct rldevice *)um->um_addr;

	rldwait(rladdr);		/* DS - Add drive ready test */
	rladdr->rlba = um->um_ubinfo;
	rladdr->rlcs = um->um_cmd|((um->um_ubinfo>>12)&RL_BAE);
}

/*
 * Handle a disk interrupt.
 */
rlintr(rl21)
	int rl21;
{
	register struct uba_ctlr *um = rlminfo[rl21];
	register struct uba_device *ui;
	register struct rldevice *rladdr = (struct rldevice *)um->um_addr;
	register struct buf *bp, *dp;
	register struct rl_softc *sc = &rl_softc[um->um_ctlr];
	int unit;
	struct rl_stat *st = &rl_stat[um->um_ctlr];
	int as = sc->rl_softas, status;

	sc->rl_wticks = 0;
	sc->rl_softas = 0;
	dp = um->um_tab.b_actf;
	bp = dp->b_actf;
	ui = rldinfo[dkunit(bp)];
	dk_busy &= ~(1 << ui->ui_dk);

	/*
	 * Check for and process errors on
	 * either the drive or the controller.
	 */
	if (rladdr->rlcs & RL_ERR) {
		u_short err;
		rlwait(rladdr);
		err = rladdr->rlcs;
		/* get status and reset controller */
		rladdr->rlda.getstat = RL_GSTAT;
		rladdr->rlcs = (ui->ui_slave << 8) | RL_GETSTAT;
		rlwait(rladdr);
		status = rladdr->rlmp.getstat;
		/* reset drive */
		rladdr->rlda.getstat = RL_RESET;
		rladdr->rlcs = (ui->ui_slave <<8) | RL_GETSTAT; /* Get status*/
		rlwait(rladdr);
		if ((status & RLMP_WL) == RLMP_WL) {
			/*
			 * Give up on write protected devices
			 * immediately.
			 */
			bp->b_flags |= B_ERROR;
			sc->sc_flags[ui->ui_unit] |= DEV_WRTLCK;
		} else if (++um->um_tab.b_errcnt > 10) {
			/*
			 * After 10 retries give up.
			 */
			harderr(bp, "rl");
			sc->sc_hardcnt[ui->ui_unit]++;
			sc->sc_flags[ui->ui_unit] |= DEV_HARDERR;

			mprintf("%s: unit#:%d hard err blk#:%d cs:%b mp:%b\n",
			    sc->sc_device[ui->ui_unit], ui->ui_unit,
			    bp->b_blkno, err, RLCS_BITS, status, RLER_BITS);

			bp->b_flags |= B_ERROR;
		} else {
			sc->sc_softcnt[ui->ui_unit]++;
			sc->sc_flags[ui->ui_unit] |= DEV_SOFTERR;
			um->um_tab.b_active = 0;	 /* force retry */
		}
		/* determine disk position */
		rladdr->rlcs = (ui->ui_slave << 8) | RL_RHDR;
		rlwait(rladdr);
		/* save disk drive position */
		st->rl_cyl[ui->ui_slave] =
		    (rladdr->rlmp.readhdr & 0177700) >> 6;
	}
	/*
	 * If still ``active'', then don't need any more retries.
	 */
	if (um->um_tab.b_active) {
		/* RL02 check if more data from previous request */
		if ((bp->b_flags & B_ERROR) == 0 &&
		     (int)(st->rl_bleft -= st->rl_bpart) > 0) {
			/*
			 * The following code was modeled from the rk07
			 * driver when an ECC error occured.  It has to
			 * fix the bits then restart the transfer which is
			 * what we have to do (restart transfer).
			 */
			int reg, npf, o, cmd, ubaddr, diff, head;

			/* seek to next head/track */
			/* increment head and/or cylinder */
			st->rl_cylnhd++;
			diff = (st->rl_cyl[ui->ui_slave] >> 1) -
				(st->rl_cylnhd >> 1);
			st->rl_cyl[ui->ui_slave] = st->rl_cylnhd;
			head = st->rl_cylnhd & 1;
			rlwait(rladdr);
			if (diff < 0)
				rladdr->rlda.seek =
				    -diff << 7 | RLDA_HGH | head << 4;
			else
				rladdr->rlda.seek =
				    diff << 7 | RLDA_LOW | head << 4;
			rladdr->rlcs = (ui->ui_slave << 8) | RL_SEEK;
			npf = btop( bp->b_bcount - st->rl_bleft );
			reg = btop(um->um_ubinfo&0x3ffff) + npf;
			o = (int)bp->b_un.b_addr & PGOFSET;
			ubapurge(um);
			um->um_tab.b_active++;
			rlwait(rladdr);
			rladdr->rlda.rw = st->rl_cylnhd << 6;
			if (st->rl_bleft < (st->rl_bpart = rl02.btrak))
				st->rl_bpart = st->rl_bleft;
			rladdr->rlmp.rw = -(st->rl_bpart >> 1);
			cmd = (bp->b_flags&B_READ ? RL_READ : RL_WRITE) |
			    RL_IE | (ui->ui_slave << 8);
			ubaddr = (int)ptob(reg) + o;
			cmd |= ((ubaddr >> 12) & RL_BAE);
			rladdr->rlba = ubaddr;
			rladdr->rlcs = cmd;
			/* since we clear the atten sum previously if we don't
			   forget about it here we must set it back - dallas
			*/
			sc->rl_softas |= as;
			return;
		}
		um->um_tab.b_active = 0;
		um->um_tab.b_errcnt = 0;
		dp->b_active = 0;
		dp->b_errcnt = 0;
		/* "b_resid" words remaining after error */
		bp->b_resid = st->rl_bleft;
		um->um_tab.b_actf = dp->b_forw;
		dp->b_actf = bp->av_forw;
		st->rl_dn = -1;
		st->rl_bpart = st->rl_bleft = 0;
		iodone(bp);
		/*
		 * If this unit has more work to do,
		 * then start it up right away.
		 */
		if (dp->b_actf)
			rlustart(ui);
		as &= ~(1<<ui->ui_slave);
	} else
		as |= (1<<ui->ui_slave);
	ubadone(um);
	/* reset state info */
	st->rl_dn = -1;
	st->rl_cylnhd = st->rl_bpart = st->rl_bleft = 0;
	/*
	 * Process other units which need attention.
	 * For each unit which needs attention, call
	 * the unit start routine to place the slave
	 * on the controller device queue.
	 */
	while (unit = ffs(as)) {
		unit--; 	/* was 1 origin */
		as &= ~(1<<unit);
		rlustart(rlip[rl21][unit]);
	}
			
	/*
	 * If the controller is not transferring, but
	 * there are devices ready to transfer, start
	 * the controller.
	 */
	if (um->um_tab.b_actf && um->um_tab.b_active == 0)
		rlstart(um);
}

rldwait(rladdr)
	register struct rldevice *rladdr;
{
	register int totaldelay = 0;

	/*
	 * Spin waiting for the DRDY bit to clear.  Wait a maximum of
	 * 10 seconds to prevent system hangs.
	 */
	while (((rladdr->rlcs & RL_DRDY) == 0) && (totaldelay <= 1000)) {
		totaldelay++;
		DELAY(10000);	/* Wait .01 seconds - smallest allowable */
	}
	if ((rldebug) && ((rladdr->rlcs & RL_DRDY) == 0)) {
		printf("rldwait: RL_DRDY failed to clear.\n");
	}
}

rlwait(rladdr)
	register struct rldevice *rladdr;
{

	register int totaldelay = 0;

	/*
	 * Spin waiting for the CRDY bit to clear.  Wait a maximum of
	 * 10 seconds to prevent system hangs.
	 */
	while (((rladdr->rlcs & RL_CRDY) == 0) && (totaldelay <= 1000)) {
		totaldelay++;
		DELAY(10000);	/* Wait .01 seconds */
	}
	if ((rldebug) && ((rladdr->rlcs & RL_CRDY) == 0)) {
		printf("rlwait: RL_CRDY failed to clear.\n");
	}
}

rlread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(rlstrategy, &rrlbuf[unit], dev, B_READ, minphys, uio));
}

rlwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(rlstrategy, &rrlbuf[unit], dev, B_WRITE, minphys, uio));
}


/*ARGSUSED*/
rlioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui = rldinfo[unit];
	register struct pt *pt = (struct pt *)data;
	register struct rl_softc *sc = &rl_softc[ui->ui_ctlr];

	struct dkop *dkop;
	struct dkget *dkget;
	struct devget *devget;
	register int i;
	int error;
	int nbpc;

	switch (cmd) {

	case DKIOCHDR:	/* do header read/write */
		break;

	case DIOCGETPT: /* 001 get partition table info */
		/*
		 *	Do a structure copy into the user's data area
		 */

		*pt = rl_part[unit];
		break;

	case DIOCDGTPT: /* 002 Return the default partition table
		/*
		 * Get number bytes per cylinder group
		 */
		nbpc = rl02.nbpc;

		/*
		 * Get and store the default block count and offset
		 */
		for( i = 0; i <= 7; i++ ) {
			pt->pt_part[i].pi_nblocks = rl02_sizes[i].nblocks;
			pt->pt_part[i].pi_blkoff =
				rl02_sizes[i].cyloff * nbpc;
		}

		break;

	case DIOCSETPT: /* 001 set the driver partition tables */
		/*
		 *	Only super users can set the pack's partition
		 *	table
		 */

		if ( !suser() )
			return(EACCES);

		/*
	   	 * Don't mess with an offline disk.
		 */
		if (sc->sc_flags[ui->ui_unit] & DEV_OFFLINE)
			return(EIO);

		/*
		 *	Before we set the new partition tables make sure
		 *	that it will no corrupt any of the kernel data
		 *	structures
		 */
		if ( ( error = ptcmp( dev, &rl_part[unit], pt ) ) != 0 )
			return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */

		rl_part[unit] = *pt;

		/*
		 *	See if we need to update the superblock of the
		 *	"a" partition of this disk
		 */
		ssblk(dev,pt);

		/*
		 *	Just make sure that we set the valid bit
		 */

		rl_part[unit].pt_valid = PT_VALID;
		break;

	case DKIOCDOP:				/* Disk operation */
		if ( !suser() )
			return(EACCES);
		break;

	case DKIOCGET:				/* Get disk status */
		break;

	case DEVIOCGET: 			/* Device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_DISK;		/* Disk 	*/

		if(ui->ui_hd->uba_type & (UBAUVI|UBAUVII)) {
			devget->bus = DEV_QB;
		} else {
			devget->bus = DEV_UB;
		}

		switch (devget->bus) {

		case DEV_UB:

			bcopy(DEV_RLU211,devget->interface,
			      strlen(DEV_RLU211));	/* RLU211	*/
			break;

		case DEV_QB:

			bcopy(DEV_RLV211,devget->interface,
			      strlen(DEV_RLV211));	/* RLV211	*/
			break;
		}
		bcopy(sc->sc_device[ui->ui_unit],
		      devget->device,
		      strlen(sc->sc_device[ui->ui_unit])); /* RL02	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA/QB */
		devget->ctlr_num = ui->ui_ctlr; 	/* which RL211	*/
		devget->slave_num = ui->ui_slave;	/* which plug	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "rl"	*/
		devget->unit_num = unit;		/* which rl??	*/
		devget->soft_count = sc->sc_softcnt[ui->ui_unit];    /* soft er. cnt.*/
		devget->hard_count = sc->sc_hardcnt[ui->ui_unit];    /* hard er. cnt.*/
		devget->stat = sc->sc_flags[ui->ui_unit];	     /* status	     */
		devget->category_stat = DEV_DISKPART;	/* which prtn.	*/
		break;

	default:
		return (ENXIO);
	}
	return(0);
}

/*
 * Reset driver after UBA init.
 * Cancel software state of all pending transfers
 * and restart all units and the controller.
 */
rlreset(uban)
	int uban;
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register struct rldevice *rladdr;
	register struct rl_stat *st;
	register int rl21, unit;

	for (rl21 = 0; rl21 < nNHL; rl21++) {
		if ((um = rlminfo[rl21]) == 0 || um->um_ubanum != uban ||
		    um->um_alive == 0)
			continue;
		printf(" hl%d", rl21);
		rladdr = (struct rldevice *)um->um_addr;
		st = &rl_stat[rl21];
		um->um_tab.b_active = 0;
		um->um_tab.b_actf = um->um_tab.b_actl = 0;
		if (um->um_ubinfo) {
			printf("<%d>", (um->um_ubinfo>>28)&0xf);
			um->um_ubinfo = 0;
		}
		/* reset controller */
		st->rl_dn = -1;
		st->rl_cylnhd = 0;
		st->rl_bleft = 0;
		st->rl_bpart = 0;
		rlwait(rladdr);
		for (unit = 0; unit < nNRL; unit++) {
			rladdr->rlcs = (unit << 8) | RL_GETSTAT;
			rlwait(rladdr);
			/* Determine disk posistion */
			rladdr->rlcs = (unit << 8) | RL_RHDR;
			rlwait(rladdr);
			/* save disk drive posistion */
			st->rl_cyl[unit] =
				(rladdr->rlmp.readhdr & 0177700) >> 6;
			if ((ui = rldinfo[unit]) == 0)
				continue;
			if (ui->ui_alive == 0 || ui->ui_mi != um)
				continue;
			rlutab[unit].b_active = 0;
			rlustart(ui);
		}
		rlstart(um);
	}
}

/*
 * Wake up every second and if an interrupt is pending
 * but nothing has happened increment a counter.
 * If nothing happens for 20 seconds, reset the UNIBUS
 * and begin anew.
 */
rlwatch()
{
	register struct uba_ctlr *um;
	register rl21, unit;
	register struct rl_softc *sc;

	timeout(rlwatch, (caddr_t)0, hz);
	for (rl21 = 0; rl21 < nNHL; rl21++) {
		um = rlminfo[rl21];
		if (um == 0 || um->um_alive == 0)
			continue;
		sc = &rl_softc[rl21];
		if (um->um_tab.b_active == 0) {
			for (unit = 0; unit < nNRL; unit++)
				if (rlutab[unit].b_active &&
				    rldinfo[unit]->ui_mi == um)
					goto active;
			sc->rl_wticks = 0;
			continue;
		}
active:
		sc->rl_wticks++;
		if (sc->rl_wticks >= 20) {
			sc->rl_wticks = 0;
			printf("hl%d: lost interrupt\n", rl21);
			/* get rid of this - dallas
			ubareset(um->um_ubanum);
			*/
			printf("RESETING RL\n");
			rlreset(um->um_ubanum); /* and do this */
		}
	}
}

/*ARGSUSED*/
rldump(dev)
	dev_t dev;
{

	/* should do a partial dump if possible. */
}

rlsize(dev)
	dev_t dev;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui;

	if (unit >= nNRL || (ui = rldinfo[unit]) == 0 || ui->ui_alive == 0)
		return (-1);
	/*
	 *	Sanity check		001
	 */
	if ( rl_part[unit].pt_valid != PT_VALID )
		panic("rlsize: invalid partition table ");

	return (rl_part[unit].pt_part[minor(dev) & 07].pi_nblocks);	/*001*/
}
#endif

