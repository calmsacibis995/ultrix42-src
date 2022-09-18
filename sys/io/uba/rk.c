
#ifndef lint
static char *sccsid = "@(#)rk.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
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
 * rk.c    6.1	   07/29/83
 *
 * Modification history
 *
 * RK711/RK07 disk driver
 *
 * 26-July-89 - Alan Frechette
 *	Conditionalize out the dump code.
 *
 * 11-Oct-84 - Stephen Reilly
 *
 *	Derived from 4.2BSD labeled: rk.c	6.1	83/07/29.
 *	Implemented the new partition table scheme. -001
 *
 * 30-Nov-84 - Stephen Reilly
 *
 *	Fixed up an error message. -002
 *
 * 11-Mar-85 - Pete Keilty
 *
 *	Fixed rkintr() do handle drive coming on line or going off line.
 *	Removed check for vv in rkopen() because of bug per steve r.
 *	request. Will be looked into in the future. -003
 *
 * 24-Sep-85 - Stephen Reilly
 *
 *	Added new ioctl request that will return the default
 *	partition table. -004
 *
 * 18-Mar-86 - jaw
 *
 *	br/cvec changed to NOT use registers.
 *
 *  9-Apr-86 - Paul Shaughnessy
 *
 *	Added partial dump code support, and removed common dump code.
 *
 * 16-Apr-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code along with
 *	soft and hard error counters. V2.0
 *
 * 22-May-86 - Paul Shaughnessy
 *
 *	Added saving of the u_area to the partial dump code.
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
 *	Changed mprintf's to log just on entry.
 */

#include "rk.h"
#if defined(BINARY) || NHK > 0

#ifdef RKDEBUG
int	rkpip;		/* DEBUG */
int	rknosval;	/* DEBUG */
int	rkdebug;
#endif RKDEBUG
#ifdef RKBDEBUG
int	rkbdebug;
#endif RKBDEBUG
#ifdef RKDEBUG
int	rktrb[1000];
int	*rktrp = rktrb;
#define trace(a,b) { *rktrp++ = *(int *)a; *rktrp++ = (int)b; \
		     if(rktrp > &rktrb[998]) rktrp = rktrb; }
#endif RKDEBUG

#include "../h/dump.h"
#include "../data/rk_data.c"

#define MASKREG(reg)	((reg)&0xffff)

short	rktypes[] = { RK_CDT, 0 };

int	rkprobe(), rkslave(), rkattach(), rkdgo(), rkintr();

u_short rkstd[] = { 0777440, 0 };
struct	uba_driver hkdriver =
 { rkprobe, rkslave, rkattach, rkdgo, rkstd, "rk", rkdinfo, "hk", rkminfo, 1 };

struct	rkst {
	short	nsect;
	short	ntrak;
	short	nspc;
	short	ncyl;
	struct	size *sizes;
} rkst[] = {
	NRKSECT, NRKTRK, NRKSECT*NRKTRK,	NRK7CYL,	rk7_sizes,
	NRKSECT, NRKTRK, NRKSECT*NRKTRK,	NRK6CYL,	rk6_sizes,
};

u_char	rk_offset[16] =
  { RKAS_P400,RKAS_M400,RKAS_P400,RKAS_M400,RKAS_P800,RKAS_M800,RKAS_P800,
    RKAS_M800,RKAS_P1200,RKAS_M1200,RKAS_P1200,RKAS_M1200,0,0,0,0
  };

#define b_cylin b_resid

#ifdef INTRLVE
daddr_t dkblock();
#endif INTRLVE

int	rkwstart, rkwatch();

rkprobe(reg)
	caddr_t reg;
{

#ifdef lint
	rkintr(0);
#endif
	((struct rkdevice *)reg)->rkcs1 = RK_CDT|RK_IE|RK_CRDY;
	DELAY(10);
	((struct rkdevice *)reg)->rkcs1 = RK_CDT;
	return (sizeof (struct rkdevice));
}

rkslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{
	register struct rkdevice *rkaddr = (struct rkdevice *)reg;
	register struct rk_softc *sc = &rk_softc[ui->ui_ctlr];

	sc->sc_offline[ui->ui_unit] = 1;
	ui->ui_type = 0;
	rkaddr->rkcs1 = RK_CCLR;
	rkaddr->rkcs2 = ui->ui_slave;
	rkaddr->rkcs1 = RK_CDT|RK_DCLR|RK_GO;
	rkwait(rkaddr);
	DELAY(50);
	if (rkaddr->rkcs2&RKCS2_NED || (rkaddr->rkds&RKDS_SVAL) == 0) {
		rkaddr->rkcs1 = RK_CCLR;
		return (0);
	}
	if (rkaddr->rkcs1&RK_CERR && rkaddr->rker&RKER_DTYE) {
		ui->ui_type = 1;
		rkaddr->rkcs1 = RK_CCLR;
	}
	return (1);
}

rkattach(ui)
	register struct uba_device *ui;
{
	register struct rk_softc *sc = &rk_softc[ui->ui_ctlr];

	if (rkwstart == 0) {
		timeout(rkwatch, (caddr_t)0, hz);
		rkwstart++;
	}
	rkip[ui->ui_ctlr][ui->ui_slave] = ui;
	sc->sc_ndrive++;
	rkcyl[ui->ui_unit] = -1;
	ui->ui_flags = 0;
	if (ui->ui_dk >= 0) {
		dk_mspw[ui->ui_dk] = 1.0 / (60 * NRKSECT * 256);
		bcopy(DEV_RK07,sc->sc_device[ui->ui_unit],
		      strlen(DEV_RK07));
		sc->sc_softcnt[ui->ui_unit] = 0;
		sc->sc_hardcnt[ui->ui_unit] = 0;
	}
}

rkopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui;
	register struct rk_softc *sc;
	struct uba_ctlr *um;
	register struct rkdevice *rkaddr;
	register int i; 			/* 001 */
	int rkstrategy();			/* 001 */

	if (unit >= nNRK || (ui  = rkdinfo[unit]) == 0 ||
	    ui->ui_alive == 0) {
		return (ENXIO);
	}

	um = ui->ui_mi; 			/* 001 */
	rkaddr = (struct rkdevice *)um->um_addr;
	sc = &rk_softc[ui->ui_ctlr];
	sc->sc_flags[ui->ui_unit] = 0;
	sc->sc_category_flags[ui->ui_unit] = 0;

	rkaddr->rkcs1 = RK_CCLR;
	rkaddr->rkcs2 = ui->ui_unit;
	rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_SELECT|RK_GO;
	rkwait(rkaddr);
	i = rkaddr->rkds;
	if (!(i & RKDS_DRDY)) {
		sc->sc_flags[ui->ui_unit] |= DEV_OFFLINE;
	}
	if (i & RKDS_WRL) {
		sc->sc_flags[ui->ui_unit] |= DEV_WRTLCK;
	}
	rkaddr->rkcs1 = rktypes[ui->ui_type] | RK_DCLR|RK_GO;
	rkwait(rkaddr);

	/*
	 *	See if we need to read in the partition table from the disk.
	 *	The conditions we will have to read from the disk is if the
	 *	partition table valid bit has not been set or the volume
	 *	is invalid.
	 */

	/*
	 *	Assume that the default values before trying to
	 *	see if the partition tables are on the pack. The
	 *	reason that we do this is that the strategy routine
	 *	is used to read in the superblock but uses the
	 *	partition info.  So we must first assume the
	 *	default values.
	 */

	if((sc->sc_flags[ui->ui_unit] & DEV_OFFLINE) ||
	   (rk_part[unit].pt_valid == 0)) {
		int nspc = rkst[ui->ui_type].nspc;

		for( i = 0; i <= 7; i++ ) {
			rk_part[unit].pt_part[i].pi_nblocks =
				rkst[ui->ui_type].sizes[i].nblocks;
			rk_part[unit].pt_part[i].pi_blkoff =
				rkst[ui->ui_type].sizes[i].cyloff * nspc;
		}

		rk_part[unit].pt_valid = PT_VALID;	/*001 Validate the pt*/

		/*
		 *	Default partition are now set. Call rsblk to set
		 *	the driver's partition tables, if any exists, from
		 *	the "a" partition superblock
		 */

		rsblk( rkstrategy, dev, &rk_part[unit] );
	}
	return (0);
}

rkstrategy(bp)
	register struct buf *bp;
{
	register int unit = dkunit(bp);
	register struct uba_device *ui = rkdinfo[unit];
	register struct rkst *st;
	register struct pt *pt; 		/* 001 */
	register struct rk_softc *sc = &rk_softc[ui->ui_ctlr];
	struct buf *dp;
	int xunit = minor(bp->b_dev) & 07;
	long bn, sz;
	int s;

	sz = (bp->b_bcount+511) >> 9;
	if (unit >= nNRK)
		goto bad;
	if (ui == 0 || ui->ui_alive == 0)
		goto bad;
	st = &rkst[ui->ui_type];
	/*
	 *	Get partition table for the pack
	 */

	pt = &rk_part[unit];
	if ( rk_part[unit].pt_valid != PT_VALID )
		panic("rkstrategy: invalid partition table ");
	if (bp->b_blkno < 0 ||
	    (bn = dkblock(bp))+sz > pt->pt_part[xunit].pi_nblocks) { /* 001 */
		sc->sc_flags[ui->ui_unit] |= DEV_EOM;
		goto bad;
	}
	bp->b_cylin = bn/st->nspc + pt->pt_part[xunit].pi_blkoff / st->nspc;
	s = spl5();
#ifdef RKDEBUG
	trace("stra",bp);
#endif RKDEBUG
	dp = &rkutab[ui->ui_unit];
	disksort(dp, bp);
	if (dp->b_active == 0) {
#ifdef RKDEBUG
		trace("!act",dp);
#endif RKDEBUG
		(void) rkustart(ui);
		bp = &ui->ui_mi->um_tab;
		if (bp->b_actf && bp->b_active == 0)
			(void) rkstart(ui->ui_mi);
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

rkustart(ui)
	register struct uba_device *ui;
{
	register struct buf *bp, *dp;
	register struct uba_ctlr *um;
	register struct rkdevice *rkaddr;
	register struct rk_softc *sc;

	if (ui == 0)
		return;
	dk_busy &= ~(1<<ui->ui_dk);
	dp = &rkutab[ui->ui_unit];
	um = ui->ui_mi;
	sc = &rk_softc[um->um_ctlr];
#ifdef RKDEBUG
	trace("ustr",dp);
#endif RKDEBUG
	rkaddr = (struct rkdevice *)um->um_addr;
	if (um->um_tab.b_active) {
		sc->sc_softas |= 1<<ui->ui_slave;
#ifdef RKDEBUG
		trace("umac",sc->sc_softas);
#endif RKDEBUG
		return;
	}
	if ((bp = dp->b_actf) == NULL) {
#ifdef RKDEBUG
		trace("!bp",0);
#endif RKDEBUG
		return;
	}
	rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_CERR;
	rkaddr->rkcs2 = ui->ui_slave;
	rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_DCLR|RK_GO;
	rkwait(rkaddr);
	if ((rkaddr->rkds & RKDS_VV) == 0 || ui->ui_flags == 0) {
		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
		struct rkst *st = &rkst[ui->ui_type];
		struct buf *bbp = &brkbuf[ui->ui_unit];

		rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_PACK|RK_GO;
		ui->ui_flags = 1;
		bbp->b_flags = B_READ|B_BUSY;
		bbp->b_dev = bp->b_dev;
		bbp->b_bcount = 512;
		bbp->b_un.b_addr = (caddr_t)&rkbad[ui->ui_unit];
		bbp->b_blkno = st->ncyl*st->nspc - st->nsect;
		bbp->b_cylin = st->ncyl - 1;
		dp->b_actf = bbp;
		bbp->av_forw = bp;
		bp = bbp;
		rkwait(rkaddr);
	}
	if (dp->b_active)
		goto done;
	dp->b_active = 1;
#ifdef RKDEBUG
	trace("dpac",1);
#endif RKDEBUG
	if ((rkaddr->rkds & RKDS_DREADY) != RKDS_DREADY)
		goto done;
	if (sc->sc_ndrive == 1)
		goto done;
	if (bp->b_cylin == rkcyl[ui->ui_unit])
		goto done;
	rkaddr->rkcyl = bp->b_cylin;
	rkcyl[ui->ui_unit] = bp->b_cylin;
	rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_IE|RK_SEEK|RK_GO;
#ifdef RKDEBUG
	trace("seek",bp->b_cylin);
#endif RKDEBUG
	if (ui->ui_dk >= 0) {
		dk_busy |= 1<<ui->ui_dk;
		dk_seek[ui->ui_dk]++;
	}
	goto out;
done:
	if (dp->b_active != 2) {
#ifdef RKDEBUG
		trace("!=2",dp->b_active);
#endif RKDEBUG
		dp->b_forw = NULL;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else {
			um->um_tab.b_actl->b_forw = dp;
#ifdef RKDEBUG
			trace("!nul",um->um_tab.b_actl);
#endif RKDEBUG
		}
		um->um_tab.b_actl = dp;
		dp->b_active = 2;
	}
out:
	return;
}

rkstart(um)
	register struct uba_ctlr *um;
{
	register struct buf *bp, *dp;
	register struct uba_device *ui;
	register struct rkdevice *rkaddr;
	struct rkst *st;
	daddr_t bn;
	int sn, tn, cmd;

loop:
	if ((dp = um->um_tab.b_actf) == NULL) {
#ifdef RKDEBUG
		trace("nodp",um);
#endif RKDEBUG
		return;
	}
	if ((bp = dp->b_actf) == NULL) {
#ifdef RKDEBUG
		trace("nobp",dp);
#endif RKDEBUG
		um->um_tab.b_actf = dp->b_forw;
		goto loop;
	}
	um->um_tab.b_active++;
	ui = rkdinfo[dkunit(bp)];
	bn = dkblock(bp);
#ifdef RKDEBUG
	trace("star",dp);
#endif RKDEBUG
	st = &rkst[ui->ui_type];
	sn = bn%st->nspc;
	tn = sn/st->nsect;
	sn %= st->nsect;
	rkaddr = (struct rkdevice *)ui->ui_addr;
retry:
	rkaddr->rkcs1 = RK_CCLR;
	rkaddr->rkcs2 = ui->ui_slave;
	rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_DCLR|RK_GO;
	rkwait(rkaddr);
	if ((rkaddr->rkds&RKDS_SVAL) == 0) {
#ifdef RKDEBUG
		rknosval++;
#endif RKDEBUG
		goto nosval;
	}
	if (rkaddr->rkds&RKDS_PIP) {
#ifdef RKDEBUG
		rkpip++;
#endif RKDEBUG
		goto retry;
	}
	if ((rkaddr->rkds&RKDS_DREADY) != RKDS_DREADY) {
		mprintf("rk%d: not ready\n", dkunit(bp));
		if ((rkaddr->rkds&RKDS_DREADY) != RKDS_DREADY) {
			rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_DCLR|RK_GO;
			rkwait(rkaddr);
			rkaddr->rkcs1 = RK_CCLR;
			rkwait(rkaddr);
			um->um_tab.b_active = 0;
			um->um_tab.b_errcnt = 0;
			dp->b_actf = bp->av_forw;
			dp->b_active = 0;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			goto loop;
		}
		mprintf(" (came back!)\n");
	}
nosval:
	rkaddr->rkcyl = bp->b_cylin;
	rkcyl[ui->ui_unit] = bp->b_cylin;
	rkaddr->rkda = (tn << 8) + sn;
	rkaddr->rkwc = -bp->b_bcount / sizeof (short);
	if (bp->b_flags & B_READ)
		cmd = rktypes[ui->ui_type]|RK_IE|RK_READ|RK_GO;
	else
		cmd = rktypes[ui->ui_type]|RK_IE|RK_WRITE|RK_GO;
	um->um_cmd = cmd;
	(void) ubago(ui);
}

rkdgo(um)
	register struct uba_ctlr *um;
{
	register struct rkdevice *rkaddr = (struct rkdevice *)um->um_addr;

	um->um_tab.b_active = 2;	/* should now be 2 */
	rkaddr->rkba = um->um_ubinfo;
	rkaddr->rkcs1 = um->um_cmd|((um->um_ubinfo>>8)&0x300);
#ifdef RKDEBUG
	trace("rkgo",um);
#endif RKDEBUG
}

rkintr(rk11)
	int rk11;
{
	register struct uba_ctlr *um = rkminfo[rk11];
	register struct uba_device *ui;
	register struct rkdevice *rkaddr = (struct rkdevice *)um->um_addr;
	register struct buf *bp, *dp;
	register struct rk_softc *sc = &rk_softc[um->um_ctlr];
	int unit;
	int as = (rkaddr->rkatt >> 8) | sc->sc_softas;

	sc->sc_wticks = 0;
	sc->sc_softas = 0;
#ifdef RKDEBUG
	trace("intr",um->um_tab.b_active);
#endif RKDEBUG
	if (um->um_tab.b_active == 2 || sc->sc_recal) {
		um->um_tab.b_active = 1;
		dp = um->um_tab.b_actf;
		bp = dp->b_actf;
		ui = rkdinfo[dkunit(bp)];
		dk_busy &= ~(1 << ui->ui_dk);
		if (bp->b_flags&B_BAD)
			if (rkecc(ui, CONT))
				return;
		if (rkaddr->rkcs1 & RK_CERR) {
			int recal;
			u_short ds = rkaddr->rkds;
			u_short cs2 = rkaddr->rkcs2;
			u_short er = rkaddr->rker;
#ifdef RKDEBUG
			if (rkdebug) {
				printf("cs2=%b ds=%b er=%b\n",
				    cs2, RKCS2_BITS, ds,
				    RKDS_BITS, er, RKER_BITS);
			}
#endif
			if (er & RKER_WLE) {
				bp->b_flags |= B_ERROR;
				sc->sc_flags[ui->ui_unit] |= DEV_WRTLCK;
			} else if (++um->um_tab.b_errcnt > 28 ||
			    ds&RKDS_HARD || er&RKER_HARD || cs2&RKCS2_HARD) {
hard:
				harderr(bp, "rk");
				sc->sc_hardcnt[ui->ui_unit]++;
				sc->sc_flags[ui->ui_unit] |= DEV_HARDERR;

				mprintf("%s: unit#:%d hard err blk#:%d \
				    rkcs1:%x rkwc:%x rkba:%x rkda:%x rkcs2:%x \
				    rkds:%x rker:%x rkatt:%x rkdc:%x rkdb:%x \
				    rkmr1:%x rkec1:%x rkec2:%x rkmr2:%x \
				    rkmr3:%x\n",
					sc->sc_device[ui->ui_unit],
					ui->ui_unit, bp->b_blkno,
					MASKREG(rkaddr->rkcs1),
					MASKREG(rkaddr->rkwc),
					MASKREG(rkaddr->rkba),
					MASKREG(rkaddr->rkda),
					MASKREG(rkaddr->rkcs2),
					MASKREG(rkaddr->rkds),
					MASKREG(rkaddr->rker),
					MASKREG(rkaddr->rkatt),
					MASKREG(rkaddr->rkcyl),
					MASKREG(rkaddr->rkdb),
					MASKREG(rkaddr->rkmr1),
					MASKREG(rkaddr->rkec1),
					MASKREG(rkaddr->rkec2),
					MASKREG(rkaddr->rkmr2),
					MASKREG(rkaddr->rkmr3));

				bp->b_flags |= B_ERROR;
				sc->sc_recal = 0;
			} else if (er & RKER_BSE) {
				if (rkecc(ui, BSE))
					return;
				else
					goto hard;
			} else {
				if ((er & (RKER_DCK|RKER_ECH)) == RKER_DCK) {
					if (rkecc(ui, ECC))
						return;
				} else
					um->um_tab.b_active = 0;
			}
			if (cs2&RKCS2_MDS) {
				rkaddr->rkcs2 = RKCS2_SCLR;
				goto retry;
			}
			recal = 0;
			if (ds&RKDS_DROT || er&(RKER_OPI|RKER_SKI|RKER_UNS) ||
			    (um->um_tab.b_errcnt&07) == 4)
				recal = 1;
			rkaddr->rkcs1 = RK_CCLR;
			rkaddr->rkcs2 = ui->ui_slave;
			rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_DCLR|RK_GO;
			rkwait(rkaddr);
			if (recal && um->um_tab.b_active == 0) {
				rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_IE|RK_RECAL|RK_GO;
				rkcyl[ui->ui_unit] = -1;
				sc->sc_recal = 0;
				goto nextrecal;
			}
		}
retry:
		switch (sc->sc_recal) {

		case 1:
			rkaddr->rkcyl = bp->b_cylin;
			rkcyl[ui->ui_unit] = bp->b_cylin;
			rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_IE|RK_SEEK|RK_GO;
			goto nextrecal;
		case 2:
			if (um->um_tab.b_errcnt < 16 ||
			    (bp->b_flags&B_READ) == 0)
				goto donerecal;
			rkaddr->rkatt = rk_offset[um->um_tab.b_errcnt & 017];
			rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_IE|RK_OFFSET|RK_GO;
			/* fall into ... */
		nextrecal:
			sc->sc_recal++;
			rkwait(rkaddr);
			um->um_tab.b_active = 1;
			return;
		donerecal:
		case 3:
			sc->sc_recal = 0;
			um->um_tab.b_active = 0;
			break;
		}
		ubadone(um);
		if (um->um_tab.b_active) {
			um->um_tab.b_active = 0;
			um->um_tab.b_errcnt = 0;
			um->um_tab.b_actf = dp->b_forw;
			dp->b_active = 0;
			dp->b_errcnt = 0;
			dp->b_actf = bp->av_forw;
#ifdef RKDEBUG
			trace("done",dp);
			trace(&um->um_tab.b_actf,dp->b_actf);
#endif RKDEBUG
			bp->b_resid = -rkaddr->rkwc * sizeof(short);
			iodone(bp);
			if (dp->b_actf)
				rkustart(ui);
		}
		as &= ~(1<<ui->ui_slave);
	}
#ifdef RKDEBUG
	trace("as =",as);					     /* 003 */
#endif RKDEBUG
	for (unit = 0; as;  unit++)				     /* 003 */
		if (as & (1<<unit)) {				     /* 003 */
			as &= ~(1<<unit);			     /* 003 */
			ui = rkip[rk11][unit];			     /* 003 */
			if (ui) {				     /* 003 */
			    rkaddr->rkcs1 = RK_CCLR;		     /* 003 */
			    rkaddr->rkcs2 = unit;		     /* 003 */
			    rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_SELECT|RK_GO;
			    rkwait(rkaddr);			     /* 003 */
			    if (rkaddr->rkds & RKDS_DRDY) {	     /* 003 */
				if (sc->sc_offline[ui->ui_unit]) {	    /* 003 */
#ifdef RKDEBUG
				    trace("rdy",unit);		     /* 003 */
#endif RKDEBUG
				    rkaddr->rkcs1 = rktypes[ui->ui_type]|
							RK_DCLR|RK_GO;
				    rkwait(rkaddr);		     /* 003 */
				    sc->sc_offline[ui->ui_unit] = 0;	    /* 003 */
				}
#ifdef RKDEBUG
				trace("ui =",rkip[rk11][unit]);      /* 003 */
#endif RKDEBUG
				rkustart(rkip[rk11][unit]);	     /* 003 */
			    }
			    else {
#ifdef RKDEBUG
				trace("!rdy",unit);		     /* 003 */
#endif RKDEBUG
				rkaddr->rkcs1 = rktypes[ui->ui_type]|
						    RK_DCLR|RK_GO;
				rkwait(rkaddr); 		     /* 003 */
				sc->sc_flags[ui->ui_unit] |= DEV_OFFLINE;
				sc->sc_offline[ui->ui_unit] = 1;	    /* 003 */
				dp = &rkutab[unit];		     /* 003 */
				bp = dp->b_actf;		     /* 003 */
				if (dp->b_active) {		     /* 003 */
				    dp->b_active = 0;		     /* 003 */
				    bp->b_flags |= B_ERROR;	     /* 003 */
				    dp->b_actf = bp->av_forw;	     /* 003 */
				    iodone(bp); 		     /* 003 */
				    if (dp->b_actf)
					rkustart(rkip[rk11][unit]);  /* 003 */
				}
			    }
			}
			else {
				mprintf("intr on unit %d ?",unit);    /* 003 */
				rkaddr->rkcs1 = RK_CCLR;	      /* 003 */
				rkaddr->rkcs2 = unit;		      /* 003 */
				rkaddr->rkcs1 = RK_CDT|RK_DCLR|RK_GO; /* 003 */
				rkwait(rkaddr); 		      /* 003 */
			}
		}
	if (um->um_tab.b_actf && um->um_tab.b_active == 0)
		rkstart(um);
	if (((rkaddr->rkcs1) & RK_IE) == 0)
		rkaddr->rkcs1 = RK_IE;
}

rkwait(addr)
	register struct rkdevice *addr;
{

	while ((addr->rkcs1 & RK_CRDY) == 0)
		;
}

rkread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(rkstrategy, &rrkbuf[unit], dev, B_READ, minphys, uio));
}

rkwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(rkstrategy, &rrkbuf[unit], dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
rkioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui = rkdinfo[unit];
	register struct pt *pt = (struct pt *)data;
	register struct rk_softc *sc = &rk_softc[ui->ui_ctlr];

	struct dkop *dkop;
	struct dkget *dkget;
	struct devget *devget;
	register int i;
	int error;
	int nspc;

	switch (cmd) {

	case DKIOCHDR:	/* do header read/write */
		break;

	case DIOCGETPT: /* 001 get partition table info */

		/*
		 *	Do a structure copy into the user's data area
		 */

		*pt = rk_part[unit];
		break;

	case DIOCDGTPT: /* 004 Return default partition table */
		/*
		 * Get number of sector per cyliinder
		 */
		nspc = rkst[ui->ui_type].nspc;

		/*
		 * Get and store the default block count and offset
		 */
		for( i = 0; i <= 7; i++ ) {
			pt->pt_part[i].pi_nblocks =
				rkst[ui->ui_type].sizes[i].nblocks;
			pt->pt_part[i].pi_blkoff =
				rkst[ui->ui_type].sizes[i].cyloff * nspc;
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
		 *	Before we set the new partition tables make sure
		 *	that it will no corrupt any of the kernel data
		 *	structures
		 */
		if ( ( error = ptcmp( dev, &rk_part[unit], pt ) ) != 0 )
			return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */

		rk_part[unit] = *pt;

		/*
		 *	See if we need to update the superblock of the
		 *	"a" partition of this disk
		 */
		ssblk(dev,pt);

		/*
		 *	Just make sure that we set the valid bit
		 */

		rk_part[unit].pt_valid = PT_VALID;
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
		devget->bus = DEV_UB;			/* Unibus	*/
		bcopy(DEV_RK711,devget->interface,
		      strlen(DEV_RK711));		/* RK711	*/
		bcopy(sc->sc_device[ui->ui_unit],
		      devget->device,
		      strlen(sc->sc_device[ui->ui_unit])); /* RK??	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA	*/
		devget->ctlr_num = ui->ui_ctlr; 	/* which RK711	*/
		devget->slave_num = ui->ui_slave;	/* which plug	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "rk"	*/
		devget->unit_num = unit;		/* which rk??	*/
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

rkecc(ui, flag)
	register struct uba_device *ui;
{
	register struct rkdevice *rk = (struct rkdevice *)ui->ui_addr;
	register struct buf *bp = rkutab[ui->ui_unit].b_actf;
	register struct uba_ctlr *um = ui->ui_mi;
	register struct rkst *st;
	struct uba_regs *ubp = ui->ui_hd->uh_uba;
	caddr_t addr;
	int reg, npf, o, cmd, ubaddr;
	int bn, cn, tn, sn;

	if (flag == CONT)
		npf = bp->b_error;
	else
		npf = btop((rk->rkwc * sizeof(short)) + bp->b_bcount);
	reg = btop(um->um_ubinfo&0x3ffff) + npf;
	o = (int)bp->b_un.b_addr & PGOFSET;
	bn = dkblock(bp);
	st = &rkst[ui->ui_type];
	cn = bp->b_cylin;
	sn = bn%st->nspc + npf;
	tn = sn/st->nsect;
	sn %= st->nsect;
	cn += tn/st->ntrak;
	tn %= st->ntrak;
	ubapurge(um);
	switch (flag) {
	case ECC:
		{
		register int i;
		int bit, byte, mask;

		npf--;
		reg--;

		mprintf("rk%d%c: soft ecc sn#:%d cyl:%d trk:%d sect:%d\n",
		    dkunit(bp), 'a'+(minor(bp->b_dev)&07), bp->b_blkno + npf,
		    rk->rkcyl, (rk->rkda >> 8) & 0xff, rk->rkda & 0xff);

		mask = rk->rkec2;
		i = rk->rkec1 - 1;		/* -1 makes 0 origin */
		bit = i&07;
		i = (i&~07)>>3;
		byte = i + o;
		while(i < 512 && (int)ptob(npf)+i < bp->b_bcount && bit > -11){
			struct	pte	pte;

			pte = ubp->uba_map[reg + btop(byte)];
			addr = ptob(pte.pg_pfnum)+ (byte & PGOFSET);
			putmemc(addr, getmemc(addr)^(mask<<bit));
			byte++;
			i++;
			bit -= 8;
		}
		if (rk->rkwc == 0) {
			um->um_tab.b_active = 0;
			return (0);
		}
		npf++;
		reg++;
		break;
		}

	case BSE:
#ifdef RKBDEBUG
		if (rkbdebug)
	printf("rkecc, BSE: bn %d cn %d tn %d sn %d\n", bn, cn, tn, sn);
#endif
		if ((bn = isbad(&rkbad[ui->ui_unit], cn, tn, sn)) < 0)
			return(0);
		bp->b_flags |= B_BAD;
		bp->b_error = npf + 1;
		bn = st->ncyl*st->nspc - st->nsect - 1 - bn;
		cn = bn/st->nspc;
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn %= st->nsect;
#ifdef RKBDEBUG
		if (rkbdebug)
	printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
#endif
		rk->rkwc = -(512 / sizeof (short));
		break;

	case CONT:
#ifdef RKBDEBUG
		if (rkbdebug)
	printf("rkecc, CONT: bn %d cn %d tn %d sn %d\n", bn,cn,tn,sn);
#endif
		bp->b_flags &= ~B_BAD;
		rk->rkwc = -((bp->b_bcount - (int)ptob(npf)) / sizeof (short));
		if (rk->rkwc == 0)
			return (0);
		break;
	}
	rk->rkcs1 = RK_CCLR;
	rk->rkcs2 = ui->ui_slave;
	rk->rkcs1 = rktypes[ui->ui_type]|RK_DCLR|RK_GO;
	rkwait(rk);
	rk->rkcyl = cn;
	rk->rkda = (tn << 8) | sn;
	ubaddr = (int)ptob(reg) + o;
	rk->rkba = ubaddr;
	cmd = (bp->b_flags&B_READ ? RK_READ : RK_WRITE)|RK_IE|RK_GO;
	cmd |= (ubaddr >> 8) & 0x300;
	cmd |= rktypes[ui->ui_type];
	rk->rkcs1 = cmd;
	um->um_tab.b_active = 2;	/* continuing */
	um->um_tab.b_errcnt = 0;	/* error has been corrected */
	return (1);
}

rkreset(uban)
	int uban;
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register rk11, unit;
	struct rk_softc *sc;

	for (rk11 = 0; rk11 < nNHK; rk11++) {
		if ((um = rkminfo[rk11]) == 0 || um->um_ubanum != uban ||
		    um->um_alive == 0)
			continue;
		printf(" hk%d", rk11);
		sc = &rk_softc[um->um_ctlr];
		um->um_tab.b_active = 0;
		um->um_tab.b_actf = um->um_tab.b_actl = 0;
		sc->sc_recal = 0;
		sc->sc_wticks = 0;
		if (um->um_ubinfo) {
			printf("<%d>", (um->um_ubinfo>>28)&0xf);
			um->um_ubinfo = 0;
		}
		for (unit = 0; unit < nNRK; unit++) {
			if ((ui = rkdinfo[unit]) == 0)
				continue;
			if (ui->ui_alive == 0 || ui->ui_mi != um)
				continue;
			rkutab[unit].b_active = 0;
			(void) rkustart(ui);
		}
		(void) rkstart(um);
	}
}

rkwatch()
{
	register struct uba_ctlr *um;
	register rk11, unit;
	register struct rk_softc *sc;

	timeout(rkwatch, (caddr_t)0, hz);
	for (rk11 = 0; rk11 < nNHK; rk11++) {
		um = rkminfo[rk11];
		if (um == 0 || um->um_alive == 0)
			continue;
		sc = &rk_softc[rk11];
		if (um->um_tab.b_active == 0) {
			for (unit = 0; unit < nNRK; unit++)
				if (rkutab[unit].b_active &&
				    rkdinfo[unit]->ui_mi == um)
					goto active;
			sc->sc_wticks = 0;
			continue;
		}
active:
		sc->sc_wticks++;
		if (sc->sc_wticks >= 30) {
			sc->sc_wticks = 0;
			printf("hk%d: lost interrupt\n", rk11);
			ubareset(um->um_ubanum);
		}
	}
}

#define DBSIZE	20

rkdump(dev, dumpinfo)
	dev_t dev;			/* dump device */
	struct dumpinfo dumpinfo;	/* dump info */
{
#ifdef notdef

	struct rkdevice *rkaddr;
	char *start;
	char *start_tmp;
	int blk, unit;
	register struct uba_regs *uba;
	register struct uba_device *ui;
	register short *rp;
	struct rkst *st;
	int ubatype;

	unit = minor(dev) >> 3;
	if (unit >= nNRK)
		return (ENXIO);
#define phys(cast, addr) ((cast)((int)addr & 0x7fffffff))
	ui = phys(struct uba_device *, rkdinfo[unit]);
	if (ui->ui_alive == 0)
		return (ENXIO);
	uba = phys(struct uba_hd *, ui->ui_hd)->uh_physuba;
	ubatype = phys(struct uba_hd *, ui->ui_hd)->uba_type;
	ubainit(uba,ubatype);
	rkaddr = (struct rkdevice *)ui->ui_physaddr;
	start = start_tmp = 0;
	rkaddr->rkcs1 = RK_CCLR;
	rkaddr->rkcs2 = unit;
	rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_DCLR|RK_GO;
	rkwait(rkaddr);
	if ((rkaddr->rkds & RKDS_VV) == 0) {
		rkaddr->rkcs1 = rktypes[ui->ui_type]|RK_IE|RK_PACK|RK_GO;
		rkwait(rkaddr);
	}
	st = &rkst[ui->ui_type];

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
		rkaddr->rkcyl = cn;
		rp = (short *) &rkaddr->rkda;
		*rp = (tn << 8) + sn;
		*--rp = 0;
		*--rp = -blk*NBPG / sizeof (short);
		*--rp = rktypes[ui->ui_type]|RK_GO|RK_WRITE;
		rkwait(rkaddr);
		if (rkaddr->rkcs1 & RK_CERR)
			return (EIO);
		start += blk*NBPG;
		start_tmp += blk*NBPG;
		dumpinfo.size_to_dump -= blk;
		if ((dumpinfo.size_to_dump <= 0) && (dumpinfo.partial_dump)) {

			/*
			 * If a partial dump is being performed....
			 */

			/* Set size_to_dump to the number of pages to dump */
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

rksize(dev)
	register dev_t dev;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui;
	register struct rkst *st;

	if (unit >= nNRK || (ui = rkdinfo[unit]) == 0 || ui->ui_alive == 0)
		return (-1);
	/*
	 *	Sanity check		001
	 */
	if ( rk_part[unit].pt_valid != PT_VALID )
		panic("rksize: invalid partition table ");

	return (rk_part[unit].pt_part[minor(dev) & 07].pi_nblocks); /* 001 */
}
#endif
