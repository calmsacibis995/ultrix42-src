#ifndef lint
static char *sccsid = "@(#)up.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *
 *			Modification History
 *
 * 26-July-89 - Alan Frechette
 *	Conditionalize out the dump code.
 *
 * 13-Jun-86   -- jaw 	fix to uba reset and drivers.
 *
 *	22-May-86	Paul Shaughnessy
 *	Added saving of the u_area to the partial dump code.
 *
 *	09-Apr-86	Paul Shaughnessy
 *	Added partial dump code support, and removed common dump code.
 *
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 *	24-Sep-85	Stephen Reilly
 * 002- Added new ioctl request that returns the default partition table
 *
 *	30-Nov-84	Stephen Reilly
 * 001 - Fixed up an error message
 *
 *	14-Nov-84	Stephen Reilly
 * 001 - Modifed driver to now handle the disk partitioning scheme
 *
 *	up.c	6.1	83/07/29
 ***********************************************************************/


#include "up.h"
#if NSC > 0 || defined(BINARY)
/*
 * UNIBUS disk driver with:
 *	overlapped seeks,
 *	ECC recovery, and
 *	bad sector forwarding.
 *
 * TODO:
 *	Check that offset recovery code works
 */

#include "../h/dump.h"
#include "../data/up_data.c"
int	upprobe(), upslave(), upattach(), updgo(), upintr();

u_short	upstd[] = { 0776700 };
struct	uba_driver scdriver =
    { upprobe, upslave, upattach, updgo, upstd, "up", updinfo, "sc", upminfo };

struct	upst {
	short	nsect;		/* # sectors/track */
	short	ntrak;		/* # tracks/cylinder */
	short	nspc;		/* # sectors/cylinder */
	short	ncyl;		/* # cylinders */
	struct	size *sizes;	/* partition tables */
	short	sdist;		/* seek distance metric */
	short	rdist;		/* rotational distance metric */
} upst[] = {
	{ 32,	19,	32*19,	815,	up9300_sizes,	3, 4 },	/* 9300 */
	{ 32,	19,	32*19,	823,	up9766_sizes,	3, 4 },	/* 9766 */
	{ 32,	10,	32*10,	823,	up160_sizes,	3, 4 },	/* fuji 160m */
	{ 32,	16,	32*16,	1024,	upam_sizes,	7, 8 },	/* Capricorn */
	{ 32,	5,	32*5,	823,	up980_sizes,	3, 4 }, /* DM980 */
	{ 0,	0,	0,	0,	0,		0, 0 }
};

u_char	up_offset[16] = {
	UPOF_P400, UPOF_M400, UPOF_P400, UPOF_M400,
	UPOF_P800, UPOF_M800, UPOF_P800, UPOF_M800, 
	UPOF_P1200, UPOF_M1200, UPOF_P1200, UPOF_M1200,
	0, 0, 0, 0
};


#define	b_cylin b_resid

#ifdef INTRLVE
daddr_t dkblock();
#endif

int	upwstart, upwatch();		/* Have started guardian */
int	upseek;
int	upwaitdry;

/*ARGSUSED*/
upprobe(reg)
	caddr_t reg;
{

#ifdef lint	
	upintr(0);
#endif
	((struct updevice *)reg)->upcs1 = UP_IE|UP_RDY;
	DELAY(10);
	((struct updevice *)reg)->upcs1 = 0;
	return (sizeof (struct updevice));
}

upslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{
	register struct updevice *upaddr = (struct updevice *)reg;

	upaddr->upcs1 = 0;		/* conservative */
	upaddr->upcs2 = ui->ui_slave;
	upaddr->upcs1 = UP_NOP|UP_GO;
	if (upaddr->upcs2&UPCS2_NED) {
		upaddr->upcs1 = UP_DCLR|UP_GO;
		return (0);
	}
	return (1);
}

upattach(ui)
	register struct uba_device *ui;
{

	if (upwstart == 0) {
		timeout(upwatch, (caddr_t)0, hz);
		upwstart++;
	}
	if (ui->ui_dk >= 0)
		dk_mspw[ui->ui_dk] = .0000020345;
	upip[ui->ui_ctlr][ui->ui_slave] = ui;
	up_softc[ui->ui_ctlr].sc_ndrive++;
	ui->ui_type = upmaptype(ui);
}

upmaptype(ui)
	register struct uba_device *ui;
{
	register struct updevice *upaddr = (struct updevice *)ui->ui_addr;
	int type = ui->ui_type;
	register struct upst *st;

	upaddr->upcs1 = 0;
	upaddr->upcs2 = ui->ui_slave;
	upaddr->uphr = UPHR_MAXTRAK;
	for (st = upst; st->nsect != 0; st++)
		if (upaddr->uphr == st->ntrak - 1) {
			type = st - upst;
			break;
		}
	if (st->nsect == 0)
		printf("up%d: uphr=%x\n", ui->ui_slave, upaddr->uphr);
	if (type == 0) {
		upaddr->uphr = UPHR_MAXCYL;
		if (upaddr->uphr == 822)
			type++;
	}
	upaddr->upcs2 = UPCS2_CLR;
	return (type);
}
 
upopen(dev)
	dev_t dev;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui;
	register int nspc;			/* 001 */
	register int i;				/* 001 */
	struct uba_ctlr	*um;			/* 001 */
	struct updevice *upaddr;		/* 001 */
	int upstrategy();			/* 001 */

	if (unit >= nNUP || (ui = updinfo[unit]) == 0 || ui->ui_alive == 0)
		return (ENXIO);

	/*
	 *	Get address for up device in question
	 */
	ui = updinfo[unit] ;
	um = ui->ui_mi;
	upaddr = ( struct updevice *)um->um_addr;

	/*
	 *	See if we need to read in the partition table from the disk.
	 *	The conditions we will have to read from the disk is if the
 	 *	partition table valid bit has not been set for the volume
	 *	is invalid.
	 */

	if (((upaddr->upds & UPDS_VV) == 0 )|| ( up_part[unit].pt_valid ==0)) {
		/*
		 *	Assume that the default values before trying to
		 *	see if the partition tables are on the pack. The
		 *	reason that we do this is that the strategy routine
		 *	is used to read in the superblock but uses the 
		 *	partition info.  So we must first assume the
		 *	default values.
		 */

		int nspc = upst[ui->ui_type].nspc;

		for( i = 0; i <= 7; i++ ) {
			up_part[unit].pt_part[i].pi_nblocks = 
				upst[ui->ui_type].sizes[i].nblocks;
			up_part[unit].pt_part[i].pi_blkoff =
				upst[ui->ui_type].sizes[i].cyloff * nspc;
		}

		up_part[unit].pt_valid = PT_VALID;	/*001 Validate the pt*/

		/*
		 *	Default partition are now set. Call rsblk to set
		 *	the driver's partition tables, if any exists, from
		 *	the "a" partition superblock
		 */

		rsblk( upstrategy, dev, &up_part[unit] );
	}
	return (0);
}

upstrategy(bp)
	register struct buf *bp;
{
	register struct uba_device *ui;
	register struct upst *st;
	register int unit;
	register struct buf *dp;
	register struct pt *pt;			/* 001 */
	int xunit = minor(bp->b_dev) & 07;
	long bn, sz;

	sz = (bp->b_bcount+511) >> 9;
	unit = dkunit(bp);
	if (unit >= nNUP)
		goto bad;
	ui = updinfo[unit];
	if (ui == 0 || ui->ui_alive == 0)
		goto bad;
	st = &upst[ui->ui_type];
	/*
	 *	Get partition table for the pack
	 */

	pt = &up_part[unit];
	if ( up_part[unit].pt_valid != PT_VALID )
		panic("upstrategy: invalid partition table ");

	if (bp->b_blkno < 0 ||
	    (bn = dkblock(bp))+sz > pt->pt_part[xunit].pi_nblocks) /* 001 */
		goto bad;
	bp->b_cylin = bn/st->nspc + pt->pt_part[xunit].pi_blkoff / st->nspc;
	(void) spl5();
	dp = &uputab[ui->ui_unit];
	disksort(dp, bp);
	if (dp->b_active == 0) {
		(void) upustart(ui);
		bp = &ui->ui_mi->um_tab;
		if (bp->b_actf && bp->b_active == 0)
			(void) upstart(ui->ui_mi);
	}
	(void) spl0();
	return;

bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

/*
 * Unit start routine.
 * Seek the drive to be where the data is
 * and then generate another interrupt
 * to actually start the transfer.
 * If there is only one drive on the controller,
 * or we are very close to the data, don't
 * bother with the search.  If called after
 * searching once, don't bother to look where
 * we are, just queue for transfer (to avoid
 * positioning forever without transferrring.)
 */
upustart(ui)
	register struct uba_device *ui;
{
	register struct buf *bp, *dp;
	register struct uba_ctlr *um;
	register struct updevice *upaddr;
	register struct upst *st;
	daddr_t bn;
	int sn, csn;
	/*
	 * The SC21 cancels commands if you just say
	 *	cs1 = UP_IE
	 * so we are cautious about handling of cs1.
	 * Also don't bother to clear as bits other than in upintr().
	 */
	int didie = 0;

	if (ui == 0)
		return (0);
	um = ui->ui_mi;
	dk_busy &= ~(1<<ui->ui_dk);
	dp = &uputab[ui->ui_unit];
	if ((bp = dp->b_actf) == NULL)
		goto out;
	/*
	 * If the controller is active, just remember
	 * that this device would like to be positioned...
	 * if we tried to position now we would confuse the SC21.
	 */
	if (um->um_tab.b_active) {
		up_softc[um->um_ctlr].sc_softas |= 1<<ui->ui_slave;
		return (0);
	}
	/*
	 * If we have already positioned this drive,
	 * then just put it on the ready queue.
	 */
	if (dp->b_active)
		goto done;
	dp->b_active = 1;
	upaddr = (struct updevice *)um->um_addr;
	upaddr->upcs2 = ui->ui_slave;
	/*
	 * If drive has just come up,
	 * setup the pack.
	 */
	if ((upaddr->upds & UPDS_VV) == 0 || upinit[ui->ui_unit] == 0) {
		struct buf *bbp = &bupbuf[ui->ui_unit];

		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
		upinit[ui->ui_unit] = 1;
		upaddr->upcs1 = UP_IE|UP_DCLR|UP_GO;
		upaddr->upcs1 = UP_IE|UP_PRESET|UP_GO;
		upaddr->upof = UPOF_FMT22;
		didie = 1;
		st = &upst[ui->ui_type];
		bbp->b_flags = B_READ|B_BUSY;
		bbp->b_dev = bp->b_dev;
		bbp->b_bcount = 512;
		bbp->b_un.b_addr = (caddr_t)&upbad[ui->ui_unit];
		bbp->b_blkno = st->ncyl * st->nspc - st->nsect;
		bbp->b_cylin = st->ncyl - 1;
		dp->b_actf = bbp;
		bbp->av_forw = bp;
		bp = bbp;
	}
	/*
	 * If drive is offline, forget about positioning.
	 */
	if ((upaddr->upds & (UPDS_DPR|UPDS_MOL)) != (UPDS_DPR|UPDS_MOL))
		goto done;
	/*
	 * If there is only one drive,
	 * dont bother searching.
	 */
	if (up_softc[um->um_ctlr].sc_ndrive == 1)
		goto done;
	/*
	 * Figure out where this transfer is going to
	 * and see if we are close enough to justify not searching.
	 */
	st = &upst[ui->ui_type];
	bn = dkblock(bp);
	sn = bn%st->nspc;
	sn = (sn + st->nsect - st->sdist) % st->nsect;
	if (bp->b_cylin - upaddr->updc)
		goto search;		/* Not on-cylinder */
	else if (upseek)
		goto done;		/* Ok just to be on-cylinder */
	csn = (upaddr->upla>>6) - sn - 1;
	if (csn < 0)
		csn += st->nsect;
	if (csn > st->nsect - st->rdist)
		goto done;
search:
	upaddr->updc = bp->b_cylin;
	/*
	 * Not on cylinder at correct position,
	 * seek/search.
	 */
	if (upseek)
		upaddr->upcs1 = UP_IE|UP_SEEK|UP_GO;
	else {
		upaddr->upda = sn;
		upaddr->upcs1 = UP_IE|UP_SEARCH|UP_GO;
	}
	didie = 1;
	/*
	 * Mark unit busy for iostat.
	 */
	if (ui->ui_dk >= 0) {
		dk_busy |= 1<<ui->ui_dk;
		dk_seek[ui->ui_dk]++;
	}
	goto out;
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
		dp->b_active = 2;
	}
out:
	return (didie);
}

/*
 * Start up a transfer on a drive.
 */
upstart(um)
	register struct uba_ctlr *um;
{
	register struct buf *bp, *dp;
	register struct uba_device *ui;
	register struct updevice *upaddr;
	struct upst *st;
	daddr_t bn;
	int dn, sn, tn, cmd, waitdry;

loop:
	/*
	 * Pull a request off the controller queue
	 */
	if ((dp = um->um_tab.b_actf) == NULL)
		return (0);
	if ((bp = dp->b_actf) == NULL) {
		um->um_tab.b_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy, and
	 * determine destination of this request.
	 */
	um->um_tab.b_active++;
	ui = updinfo[dkunit(bp)];
	bn = dkblock(bp);
	dn = ui->ui_slave;
	st = &upst[ui->ui_type];
	sn = bn%st->nspc;
	tn = sn/st->nsect;
	sn %= st->nsect;
	upaddr = (struct updevice *)ui->ui_addr;
	/*
	 * Select drive if not selected already.
	 */
	if ((upaddr->upcs2&07) != dn)
		upaddr->upcs2 = dn;
	/*
	 * Check that it is ready and online
	 */
	waitdry = 0;
	while ((upaddr->upds&UPDS_DRY) == 0) {
		printf("up%d: ds wait ds=%o\n",dkunit(bp),upaddr->upds);
		if (++waitdry > 512)
			break;
		upwaitdry++;
	}
	if ((upaddr->upds & UPDS_DREADY) != UPDS_DREADY) {
		printf("up%d: not ready", dkunit(bp));
		if ((upaddr->upds & UPDS_DREADY) != UPDS_DREADY) {
			printf("\n");
			um->um_tab.b_active = 0;
			um->um_tab.b_errcnt = 0;
			dp->b_actf = bp->av_forw;
			dp->b_active = 0;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			goto loop;
		}
		/*
		 * Oh, well, sometimes this
		 * happens, for reasons unknown.
		 */
		printf(" (flakey)\n");
	}
	/*
	 * Setup for the transfer, and get in the
	 * UNIBUS adaptor queue.
	 */
	upaddr->updc = bp->b_cylin;
	upaddr->upda = (tn << 8) + sn;
	upaddr->upwc = -bp->b_bcount / sizeof (short);
	if (bp->b_flags & B_READ)
		cmd = UP_IE|UP_RCOM|UP_GO;
	else
		cmd = UP_IE|UP_WCOM|UP_GO;
	um->um_cmd = cmd;
	(void) ubago(ui);
	return (1);
}

/*
 * Now all ready to go, stuff the registers.
 */
updgo(um)
	struct uba_ctlr *um;
{
	register struct updevice *upaddr = (struct updevice *)um->um_addr;

	um->um_tab.b_active = 2;	/* should now be 2 */
	upaddr->upba = um->um_ubinfo;
	upaddr->upcs1 = um->um_cmd|((um->um_ubinfo>>8)&0x300);
}

/*
 * Handle a disk interrupt.
 */
upintr(sc21)
	register sc21;
{
	register struct buf *bp, *dp;
	register struct uba_ctlr *um = upminfo[sc21];
	register struct uba_device *ui;
	register struct updevice *upaddr = (struct updevice *)um->um_addr;
	register unit;
	struct up_softc *sc = &up_softc[um->um_ctlr];
	int as = (upaddr->upas & 0377) | sc->sc_softas;
	int needie = 1, waitdry;

	sc->sc_wticks = 0;
	sc->sc_softas = 0;
	/*
	 * If controller wasn't transferring, then this is an
	 * interrupt for attention status on seeking drives.
	 * Just service them.
	 */
	if (um->um_tab.b_active != 2 && !sc->sc_recal) {
		if (upaddr->upcs1 & UP_TRE)
			upaddr->upcs1 = UP_TRE;
		goto doattn;
	}
	um->um_tab.b_active = 1;
	/*
	 * Get device and block structures, and a pointer
	 * to the uba_device for the drive.  Select the drive.
	 */
	dp = um->um_tab.b_actf;
	bp = dp->b_actf;
	ui = updinfo[dkunit(bp)];
	dk_busy &= ~(1 << ui->ui_dk);
	if ((upaddr->upcs2&07) != ui->ui_slave)
		upaddr->upcs2 = ui->ui_slave;
	if (bp->b_flags&B_BAD) {
		if (upecc(ui, CONT))
			return;
	}
	/*
	 * Check for and process errors on
	 * either the drive or the controller.
	 */
	if ((upaddr->upds&UPDS_ERR) || (upaddr->upcs1&UP_TRE)) {
		waitdry = 0;
		while ((upaddr->upds & UPDS_DRY) == 0) {
			if (++waitdry > 512)
				break;
			upwaitdry++;
		}
		if (upaddr->uper1&UPER1_WLE) {
			/*
			 * Give up on write locked devices
			 * immediately.
			 */
			printf("up%d: write locked\n", dkunit(bp));
			bp->b_flags |= B_ERROR;
		} else if (++um->um_tab.b_errcnt > 27) {
			/*
			 * After 28 retries (16 without offset, and
			 * 12 with offset positioning) give up.
			 * If the error was header CRC, the header is 
			 * screwed up, and the sector may in fact exist
			 * in the bad sector table, better check...
			 */
			if (upaddr->uper1&UPER1_HCRC) {
				if (upecc(ui, BSE))
					return;
			}
	hard:
			harderr(bp, "up");
			printf("cn=%d tn=%d sn=%d cs2=%b er1=%b er2=%b\n",
			        upaddr->updc, ((upaddr->upda)>>8)&077,
			        (upaddr->upda)&037,
				upaddr->upcs2, UPCS2_BITS,
				upaddr->uper1, UPER1_BITS,
				upaddr->uper2, UPER2_BITS);
			bp->b_flags |= B_ERROR;
		} else if (upaddr->uper2 & UPER2_BSE) {
			if (upecc(ui, BSE))
				return;
			else
				goto hard;
		} else {
			/*
			 * Retriable error.
			 * If a soft ecc, correct it (continuing
			 * by returning if necessary.
			 * Otherwise fall through and retry the transfer
			 */
			if ((upaddr->uper1&(UPER1_DCK|UPER1_ECH))==UPER1_DCK) {
				if (upecc(ui, ECC))
					return;
			} else
				um->um_tab.b_active = 0; /* force retry */
		}
		/*
		 * Clear drive error and, every eight attempts,
		 * (starting with the fourth)
		 * recalibrate to clear the slate.
		 */
		upaddr->upcs1 = UP_TRE|UP_IE|UP_DCLR|UP_GO;
		needie = 0;
		if ((um->um_tab.b_errcnt&07) == 4 && um->um_tab.b_active == 0) {
			upaddr->upcs1 = UP_RECAL|UP_IE|UP_GO;
			sc->sc_recal = 0;
			goto nextrecal;
		}
	}
	/*
	 * Advance recalibration finite state machine
	 * if recalibrate in progress, through
	 *	RECAL
	 *	SEEK
	 *	OFFSET (optional)
	 *	RETRY
	 */
	switch (sc->sc_recal) {

	case 1:
		upaddr->updc = bp->b_cylin;
		upaddr->upcs1 = UP_SEEK|UP_IE|UP_GO;
		goto nextrecal;
	case 2:
		if (um->um_tab.b_errcnt < 16 || (bp->b_flags&B_READ) == 0)
			goto donerecal;
		upaddr->upof = up_offset[um->um_tab.b_errcnt & 017] | UPOF_FMT22;
		upaddr->upcs1 = UP_IE|UP_OFFSET|UP_GO;
		goto nextrecal;
	nextrecal:
		sc->sc_recal++;
		um->um_tab.b_active = 1;
		return;
	donerecal:
	case 3:
		sc->sc_recal = 0;
		um->um_tab.b_active = 0;
		break;
	}
	/*
	 * If still ``active'', then don't need any more retries.
	 */
	if (um->um_tab.b_active) {
		/*
		 * If we were offset positioning,
		 * return to centerline.
		 */
		if (um->um_tab.b_errcnt >= 16) {
			upaddr->upof = UPOF_FMT22;
			upaddr->upcs1 = UP_RTC|UP_GO|UP_IE;
			while (upaddr->upds & UPDS_PIP)
				DELAY(25);
			needie = 0;
		}
		um->um_tab.b_active = 0;
		um->um_tab.b_errcnt = 0;
		um->um_tab.b_actf = dp->b_forw;
		dp->b_active = 0;
		dp->b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_resid = (-upaddr->upwc * sizeof(short));
		iodone(bp);
		/*
		 * If this unit has more work to do,
		 * then start it up right away.
		 */
		if (dp->b_actf)
			if (upustart(ui))
				needie = 0;
	}
	as &= ~(1<<ui->ui_slave);
	/*
	 * Release unibus resources and flush data paths.
	 */
	ubadone(um);
doattn:
	/*
	 * Process other units which need attention.
	 * For each unit which needs attention, call
	 * the unit start routine to place the slave
	 * on the controller device queue.
	 */
	while (unit = ffs(as)) {
		unit--;		/* was 1 origin */
		as &= ~(1<<unit);
		upaddr->upas = 1<<unit;
		if (unit < UPIPUNITS && upustart(upip[sc21][unit]))
			needie = 0;
	}
	/*
	 * If the controller is not transferring, but
	 * there are devices ready to transfer, start
	 * the controller.
	 */
	if (um->um_tab.b_actf && um->um_tab.b_active == 0)
		if (upstart(um))
			needie = 0;
	if (needie)
		upaddr->upcs1 = UP_IE;
}

upread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	if (unit >= nNUP)
		return (ENXIO);
	return (physio(upstrategy, &rupbuf[unit], dev, B_READ, minphys, uio));
}

upwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	if (unit >= nNUP)
		return (ENXIO);
	return (physio(upstrategy, &rupbuf[unit], dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
upioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	struct pt *pt = (struct pt *)data;
	int unit = minor(dev) >> 3;
	register struct uba_device *ui;
	register int nspc;
	register int i;	
	int error;

	switch (cmd) {

	case DIOCGETPT:	/* 001 get partition table info */
		/*
		 *	Do a structure copy into the user's data area
		 */

		*pt = up_part[unit];
		return(0);

	case DIOCDGTPT:	/* 002 Return that default partition table */
		ui = updinfo[unit];

		/*
		 * Get number sector per cylinder
		 */
		nspc = upst[ui->ui_type].nspc;

		/*
		 *  Get and store the default block count and offset
		 */
		for( i = 0; i <= 7; i++ ) {
			pt->pt_part[i].pi_nblocks = 
				upst[ui->ui_type].sizes[i].nblocks;
			pt->pt_part[i].pi_blkoff =
				upst[ui->ui_type].sizes[i].cyloff * nspc;
		}

		return(0);
	case DIOCSETPT: /* 001 set the driver partition tables */
		/*
		 *	Only super users can set the pack's partition
		 *	table
		 */

		if ( !suser() )
			return(EACCES);
		
		/*
		 *	Before we set the new partition tables make sure
		 *	that it will no corrupt any of the kernel data
		 *	structures
		 */
		if ( ( error = ptcmp( dev, &up_part[unit], pt ) ) != 0 )
			return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */

		up_part[unit] = *pt;

		/*
		 *	See if we need to update the superblock of the
		 *	"a" partition of this disk
		 */
		ssblk(dev,pt);

		/*
		 *	Just make sure that we set the valid bit
		 */

		up_part[unit].pt_valid = PT_VALID;
		return(0);
	default:
		return (ENXIO);
	}
}
/*
 * Correct an ECC error, and restart the i/o to complete
 * the transfer if necessary.  This is quite complicated because
 * the transfer may be going to an odd memory address base and/or
 * across a page boundary.
 */
upecc(ui, flag)
	register struct uba_device *ui;
	int flag;
{
	register struct updevice *up = (struct updevice *)ui->ui_addr;
	register struct buf *bp = uputab[ui->ui_unit].b_actf;
	register struct uba_ctlr *um = ui->ui_mi;
	register struct upst *st;
	struct uba_regs *ubp = ui->ui_hd->uh_uba;
	register int i;
	caddr_t addr;
	int reg, bit, byte, npf, mask, o, cmd, ubaddr;
	int bn, cn, tn, sn;

	/*
	 * Npf is the number of sectors transferred before the sector
	 * containing the ECC error, and reg is the UBA register
	 * mapping (the first part of) the transfer.
	 * O is offset within a memory page of the first byte transferred.
	 */
	if (flag == CONT)
		npf = bp->b_error;
	else
		npf = btop((up->upwc * sizeof(short)) + bp->b_bcount);
	reg = btop(um->um_ubinfo&0x3ffff) + npf;
	o = (int)bp->b_un.b_addr & PGOFSET;
	mask = up->upec2;
#ifdef UPECCDEBUG
	printf("npf %d reg %x o %d mask %o pos %d\n", npf, reg, o, mask,
	    up->upec1);
#endif
	bn = dkblock(bp);
	st = &upst[ui->ui_type];
	cn = bp->b_cylin;
	sn = bn%st->nspc + npf;
	tn = sn/st->nsect;
	sn %= st->nsect;
	cn += tn/st->ntrak;
	tn %= st->ntrak;
	ubapurge(um);
	um->um_tab.b_active=2;
	/*
	 * action taken depends on the flag
	 */
	switch(flag){
	case ECC:
		npf--;
		reg--;
		mask = up->upec2;
		printf("up%d%c: soft ecc sn%d\n", dkunit(bp),
			'a'+(minor(bp->b_dev)&07), bp->b_blkno + npf);
		/*
		 * Flush the buffered data path, and compute the
		 * byte and bit position of the error.  The variable i
		 * is the byte offset in the transfer, the variable byte
		 * is the offset from a page boundary in main memory.
		 */
		i = up->upec1 - 1;		/* -1 makes 0 origin */
		bit = i&07;
		i = (i&~07)>>3;
		byte = i + o;
		/*
		 * Correct while possible bits remain of mask.  Since mask
		 * contains 11 bits, we continue while the bit offset is > -11.
		 * Also watch out for end of this block and the end of the whole
		 * transfer.
		 */
		while (i < 512 && (int)ptob(npf)+i < bp->b_bcount && bit > -11) {
			struct pte pte;

			pte = ubp->uba_map[reg + btop(byte)];
			addr = ptob(pte.pg_pfnum) + (byte & PGOFSET);
#ifdef UPECCDEBUG
			printf("addr %x map reg %x\n",
				addr, *(int *)(&ubp->uba_map[reg+btop(byte)]));
			printf("old: %x, ", getmemc(addr));
#endif
			putmemc(addr, getmemc(addr)^(mask<<bit));
#ifdef UPECCDEBUG
			printf("new: %x\n", getmemc(addr));
#endif
			byte++;
			i++;
			bit -= 8;
		}
		if (up->upwc == 0)
			return (0);
		npf++;
		reg++;
		break;
	case BSE:
		/*
		 * if not in bad sector table, return 0
		 */
		if ((bn = isbad(&upbad[ui->ui_unit], cn, tn, sn)) < 0)
			return(0);
		/*
		 * flag this one as bad
		 */
		bp->b_flags |= B_BAD;
		bp->b_error = npf + 1;
#ifdef UPECCDEBUG
		printf("BSE: restart at %d\n",npf+1);
#endif
		bn = st->ncyl * st->nspc -st->nsect - 1 - bn;
		cn = bn / st->nspc;
		sn = bn % st->nspc;
		tn = sn / st->nsect;
		sn %= st->nsect;
		up->upwc = -(512 / sizeof (short));
#ifdef UPECCDEBUG
		printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
#endif
		break;
	case CONT:
#ifdef UPECCDEBUG
		printf("upecc, CONT: bn %d cn %d tn %d sn %d\n", bn, cn, tn, sn);
#endif
		bp->b_flags &= ~B_BAD;
		up->upwc = -((bp->b_bcount - (int)ptob(npf)) / sizeof(short));
		if (up->upwc == 0)
			return(0);
		break;
	}
	if (up->upwc == 0) {
		um->um_tab.b_active = 0;
		return (0);
	}
	/*
	 * Have to continue the transfer... clear the drive,
	 * and compute the position where the transfer is to continue.
	 * We have completed npf+1 sectors of the transfer already;
	 * restart at offset o of next sector (i.e. in UBA register reg+1).
	 */
#ifdef notdef
	up->uper1 = 0;
	up->upcs1 |= UP_GO;
#else
	up->upcs1 = UP_TRE|UP_IE|UP_DCLR|UP_GO;
	up->updc = cn;
	up->upda = (tn << 8) | sn;
	ubaddr = (int)ptob(reg) + o;
	up->upba = ubaddr;
	cmd = (ubaddr >> 8) & 0x300;
	cmd |= ((bp->b_flags&B_READ)?UP_RCOM:UP_WCOM)|UP_IE|UP_GO;
	um->um_tab.b_errcnt = 0;
	up->upcs1 = cmd;
#endif
	return (1);
}

/*
 * Reset driver after UBA init.
 * Cancel software state of all pending transfers
 * and restart all units and the controller.
 */
upreset(uban)
	int uban;
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register sc21, unit;

	for (sc21 = 0; sc21 < nNSC; sc21++) {
		if ((um = upminfo[sc21]) == 0 || um->um_ubanum != uban ||
		    um->um_alive == 0)
			continue;
		printf(" sc%d", sc21);
		um->um_tab.b_active = 0;
		um->um_tab.b_actf = um->um_tab.b_actl = 0;
		up_softc[sc21].sc_recal = 0;
		up_softc[sc21].sc_wticks = 0;
		if (um->um_ubinfo) {
			printf("<%d>", (um->um_ubinfo>>28)&0xf);
			um->um_ubinfo = 0;
		}
		((struct updevice *)(um->um_addr))->upcs2 = UPCS2_CLR;
		for (unit = 0; unit < nNUP; unit++) {
			if ((ui = updinfo[unit]) == 0)
				continue;
			if (ui->ui_alive == 0 || ui->ui_mi != um)
				continue;
			uputab[unit].b_active = 0;
			(void) upustart(ui);
		}
		(void) upstart(um);
	}
}

/*
 * Wake up every second and if an interrupt is pending
 * but nothing has happened increment a counter.
 * If nothing happens for 20 seconds, reset the UNIBUS
 * and begin anew.
 */
upwatch()
{
	register struct uba_ctlr *um;
	register sc21, unit;
	register struct up_softc *sc;

	timeout(upwatch, (caddr_t)0, hz);
	for (sc21 = 0; sc21 < nNSC; sc21++) {
		um = upminfo[sc21];
		if (um == 0 || um->um_alive == 0)
			continue;
		sc = &up_softc[sc21];
		if (um->um_tab.b_active == 0) {
			for (unit = 0; unit < nNUP; unit++)
				if (uputab[unit].b_active &&
				    updinfo[unit]->ui_mi == um)
					goto active;
			sc->sc_wticks = 0;
			continue;
		}
active:
		sc->sc_wticks++;
		if (sc->sc_wticks >= 20) {
			sc->sc_wticks = 0;
			printf("sc%d: lost interrupt\n", sc21);
			ubareset(um->um_ubanum);
		}
	}
}

#define	DBSIZE	20

updump(dev, dumpinfo)
	dev_t dev;			/* dump device */
	struct dumpinfo dumpinfo;	/* dump info */
{
#ifdef notdef

	struct updevice *upaddr;
	char *start;
	char *start_tmp;
	int blk, unit, ubatype;
	register struct uba_regs *uba;
	register struct uba_device *ui;
	register short *rp;
	struct upst *st;
	register int retry;

	unit = minor(dev) >> 3;
	if (unit >= nNUP)
		return (ENXIO);
#define	phys(cast, addr) ((cast)((int)addr & 0x7fffffff))
	ui = phys(struct uba_device *, updinfo[unit]);
	if (ui->ui_alive == 0)
		return (ENXIO);
	uba = phys(struct uba_hd *, ui->ui_hd)->uh_physuba;
	ubatype = phys(struct uba_hd *, ui->ui_hd)->uba_type;
	ubainit(uba,ubatype);
	upaddr = (struct updevice *)ui->ui_physaddr;
	DELAY(5000000);
	upaddr->upcs2 = unit;
	DELAY(100);
	upaddr->upcs1 = UP_DCLR|UP_GO;
	upaddr->upcs1 = UP_PRESET|UP_GO;
	upaddr->upof = UPOF_FMT22;
	retry = 0;
	do {
		DELAY(25);
		if (++retry > 527)
			break;
	} while ((upaddr->upds & UP_RDY) == 0);
	if ((upaddr->upds & UPDS_DREADY) != UPDS_DREADY)
		return (EFAULT);
	start = start_tmp = 0;
	st = &upst[ui->ui_type];

	/*
	 * If a full dump is being performed, then this loop
	 * will dump all of core. If a partial dump is being
	 * performed, then as much of core as possible will be
	 * dumped, leaving room for the u_area and error logger
	 * buffer. Please note that dumpsys predetermined what
	 * type of dump will be performed.
	 */

	while ((dumpinfo.size_to_dump > 0) || (dumpinfo.partial_dump)) {
		register struct pte *io;
		register int i;
		int cn, sn, tn;
		daddr_t bn;

		blk = dumpinfo.size_to_dump > DBSIZE ? DBSIZE : dumpinfo.size_to_dump;
		io = uba->uba_map;
		for (i = 0; i < blk; i++)
			*(int *)io++ = (btop(start)+i) | (1<<21) | UBAMR_MRV;
		*(int *)io = 0;
		bn = dumplo + btop(start_tmp);
		cn = bn/st->nspc + dumpinfo.blkoffs / st->nspc;
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn = sn%st->nsect;
		upaddr->updc = cn;
		rp = (short *) &upaddr->upda;
		*rp = (tn << 8) + sn;
		*--rp = 0;
		*--rp = -blk*NBPG / sizeof (short);
		*--rp = UP_GO|UP_WCOM;
		retry = 0;
		do {
			DELAY(25);
			if (++retry > 527)
				break;
		} while ((upaddr->upcs1 & UP_RDY) == 0);
		if ((upaddr->upds & UPDS_DREADY) != UPDS_DREADY) {
			printf("up%d: not ready", unit);
			if ((upaddr->upds & UPDS_DREADY) != UPDS_DREADY) {
				printf("\n");
				return (EIO);
			}
			printf(" (flakey)\n");
		}
		if (upaddr->upds&UPDS_ERR)
			return (EIO);
		start += blk*NBPG;
		start_tmp += blk*NBPG;
		dumpinfo.size_to_dump -= blk;
		if ((dumpinfo.size_to_dump <= 0) && (dumpinfo.partial_dump)) {
			
			/*
			 * If a partial dump is being performed....
			 */

			/* Set size_to_dump to the number of pages of elbuf */
			dumpinfo.size_to_dump = 
			  dumpinfo.pdump[NUM_TO_DUMP-dumpinfo.partial_dump].num_blks;
			/* Set start to starting address */
			start = 0;
			start += 
			  dumpinfo.pdump[NUM_TO_DUMP-dumpinfo.partial_dump].start_addr;
			dumpinfo.partial_dump--;
		}
	}
	return (0);
#endif notdef
}

upsize(dev)
	dev_t dev;
{
	int unit = minor(dev) >> 3;
	struct uba_device *ui;
	struct upst *st;

	if (unit >= nNUP || (ui = updinfo[unit]) == 0 || ui->ui_alive == 0)
		return (-1);
	/*
	 *	Sanity check		001
	 */
	if ( up_part[unit].pt_valid != PT_VALID )
		panic("upsize: invalid partition table ");

	return (up_part[unit].pt_part[minor(dev) & 07].pi_nblocks); /* 001 */
}
#endif
