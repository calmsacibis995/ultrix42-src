
#ifndef lint
static char *sccsid = "@(#)ut.c	4.1      ULTRIX  7/2/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1989 by		*
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
 * ut.c    6.1	   07/29/83
 *
 * Modification history
 *
 * TU45 tape driver
 * System Industries Model 9700 Tape Drive,
 * emulates a TU45 on the UNIBUS.
 *
 *  9-Feb-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: ut.c	6.1	83/07/29.
 *	Added new ioctl functionality as well as new code to handle
 *	EOT correctly. Driver now supports new minor number convention
 *	to allow for more than 4 tape devices. V2.0
 *
 * 18-Mar-86 - jaw
 *
 *	br/cvec changed to NOT use registers.
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
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
 * 14-Aug-87 - rjl
 *
 *	Fixed two mprintf's that had a extra argument which caused
 *	the system to panic when a tape error was encountered.
 *
 * 05-Jun-89 - Tim Burke
 *	Added a new MTIOCTOP called MTFLUSH.  This is used on caching drives
 *	to force a flush of the controller's write back cache.  Since this 
 *	is not a caching drive the MTFLUSH always returns ENXIO.
 */

#include "tj.h"
#if NUT > 0 || defined(BINARY)

#include "../data/ut_data.c"

u_short utstd[] = { UTSTD, 0 };
int utprobe(), utslave(), utattach(), utdgo(), utintr(), uttimer();

struct uba_driver utdriver = { utprobe, utslave, utattach, utdgo,
			       utstd, "tj", tjdinfo, "ut",
			       utminfo, 0 };

utprobe(reg)
	caddr_t reg;
{

#ifdef lint
	utintr(0);
#endif

	/*
	 * The SI documentation says you must set the RDY bit
	 * (even though it's read-only) to force an interrupt.
	 */
	((struct utdevice *) reg)->utcs1 = UT_IE|UT_NOP|UT_RDY;
	DELAY(10000);
	return (sizeof (struct utdevice));
}

utslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{
	/*
	 * A real TU45 would support the slave present bit
	 * int the drive type register, but this thing doesn't,
	 * so there's no way to determine if a slave is present or not.
	 */
	 return(1);
}

utattach(ui)
	struct uba_device *ui;
{
	register struct tj_softc *sc = &tj_softc[ui->ui_unit];

	tjtout[ui->ui_unit] = ui->ui_mi->um_ctlr;
	bcopy(DEV_UNKNOWN,sc->sc_device,strlen(DEV_UNKNOWN));
}

/*
 * Open the device with exclusive access.
 */
utopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct uba_device *ui;
	register struct tj_softc *sc;
	register struct utdevice *utaddr;
	register int sel = SEL(dev);
	int unit = UNIT(dev);
	int s;

	if (unit >= nNTJ || (sc = &tj_softc[unit])->sc_openf ||
	    (ui  = tjdinfo[unit]) == 0 || ui->ui_alive == 0) {
		return (ENXIO);
	}

	utaddr	= (struct utdevice *)ui->ui_addr;
	if ((utaddr->utds & UTDS_EOT) && (dis_eot_tj[unit] != DISEOT)) {
		sc->sc_flags &= DEV_EOM;
	} else {
		sc->sc_flags = 0;
	}
	sc->sc_category_flags = 0;

	sc->sc_dens = (((sel == MTHR) || (sel == MTHN)) ?
			      UTTC_GCR : ((sel == MTMR) ||
			      (sel == MTMN)) ? UTTC_PE : UTTC_NRZI) |
			      UTTC_PDP11FMT | (ui->ui_slave&07);
	if((sel == MTHR) || (sel == MTHN)) {
		sc->sc_category_flags |= DEV_6250BPI;
	}
	if((sel == MTMR) || (sel == MTMN)) {
		sc->sc_category_flags |= DEV_1600BPI;
	}
	if((sel == MTLR) || (sel == MTLN)) {
		sc->sc_category_flags |= DEV_800BPI;
	}

get:
	utcommand(dev, UT_SENSE, 1);
	if (sc->sc_dsreg & UTDS_PIP) {
		sleep((caddr_t) & lbolt, PZERO+1);
		goto get;
	}

	if ((sc->sc_dsreg & UTDS_MOL) == 0) {
		sc->sc_flags |= DEV_OFFLINE;
		if(!(flag & FNDELAY)) {
			DEV_UGH(sc->sc_device,unit,"offline");
			return(EIO);
		}
	}

	if ((flag & FWRITE) && (sc->sc_dsreg & UTDS_WRL)) {
		sc->sc_flags |= DEV_WRTLCK;
		if(!(flag & FNDELAY)) {
			DEV_UGH(sc->sc_device,unit,"write locked");
			return(EIO);
		}
	}

	sc->sc_openf = 1;
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;

	/*
	 * For 6250 bpi take exclusive use of the UNIBUS.
	 */
	ui->ui_driver->ud_xclu = (sc->sc_dens & (UTTC_PE|UTTC_GCR))
				  == UTTC_GCR;
	s = spl6();
	if (sc->sc_tact == 0) {
		sc->sc_timo = INF;
		sc->sc_tact = 1;
		timeout(uttimer, (caddr_t)dev, 5*hz);
	}
	splx(s);
	return (0);
}

utclose(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct tj_softc *sc = &tj_softc[UNIT(dev)];
	register int unit = UNIT(dev);
	register int sel = SEL(dev);

	sc->sc_flags &= ~DEV_EOM;

	if (flag == FWRITE || (flag & FWRITE) &&
	    (sc->sc_flags & DEV_WRITTEN)) {
		utcommand(dev, UT_WEOF, 1);
		sc->sc_flags &= ~DEV_EOM;
		utcommand(dev, UT_WEOF, 1);
		sc->sc_flags &= ~DEV_EOM;
		utcommand(dev, UT_SREV, 1);
		sc->sc_flags &= ~DEV_EOM;
	}

	if ((sel == MTLR) || (sel == MTMR) || (sel == MTHR)) {
		utcommand(dev, UT_REW, 0);
	}

	sc->sc_openf = 0;

	if ((sc->sc_dsreg & UTDS_EOT) && (dis_eot_tj[unit] != DISEOT)) {
		sc->sc_flags |= DEV_EOM;
	}
}

utcommand(dev, com, count)
	register dev_t dev;
	register int com;
	register int count;
{
	register struct buf *bp = &cutbuf[UTUNIT(dev)];
	register int s = spl5();

	while (bp->b_flags&B_BUSY) {
		if(bp->b_repcnt == 0 && (bp->b_flags&B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	splx(s);
	bp->b_dev = dev;
	bp->b_command = com;
	bp->b_repcnt = count;
	bp->b_blkno = 0;
	utstrategy(bp);
	if (count == 0)
		return;
	iowait(bp);
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;
}

/*
 * Queue a tape operation.
 */
utstrategy(bp)
	register struct buf *bp;
{
	register struct uba_ctlr *um;
	register struct tj_softc *sc = &tj_softc[UNIT(bp->b_dev)];
	register struct buf *dp;
	register int s;
	register int unit = UNIT(bp->b_dev);

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_tj[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	sc->sc_category_flags &= ~DEV_TPMARK;

	/*
	 * Put transfer at end of unit queue
	 */
	dp = &tjutab[unit];
	bp->av_forw = NULL;
	s = spl5();
	if (dp->b_actf == NULL) {
		dp->b_actf = bp;
		/*
		 * Transport not active, so...
		 * put at end of controller queue
		 */
		dp->b_forw = NULL;
		um = tjdinfo[unit]->ui_mi;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else
			um->um_tab.b_actl->b_forw = dp;
		um->um_tab.b_actl = dp;
	} else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	/*
	 * If the controller is not busy, set it going.
	 */
	if (um->um_tab.b_state == 0)
		utstart(um);
	splx(s);
}

utstart(um)
	register struct uba_ctlr *um;
{
	register struct utdevice *addr = (struct utdevice *)um->um_addr;
	register struct buf *bp, *dp;
	register struct tj_softc *sc;
	register struct uba_device *ui;
	int utunit, unit;
	daddr_t blkno;

loop:
	/*
	 * Scan controller queue looking for units with
	 * transaction queues to dispatch
	 */
	if ((dp = um->um_tab.b_actf) == NULL)
		return;

	if ((bp = dp->b_actf) == NULL) {
		um->um_tab.b_actf = dp->b_forw;
		goto loop;
	}

	utunit = UTUNIT(bp->b_dev);
	unit = UNIT(bp->b_dev);
	ui = tjdinfo[unit];
	sc = &tj_softc[unit];
	/* note slave select, density, and format were merged on open */
	addr->uttc = sc->sc_dens;
	sc->sc_dsreg = addr->utds;
	sc->sc_erreg = addr->uter;
	sc->sc_resid = MASKREG(addr->utfc);

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_tj[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		goto next;
	}

	if ((sc->sc_category_flags & DEV_TPMARK) && 
	    (bp->b_flags & B_READ)) {
		bp->b_resid = 0;
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		goto next;
	}

	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in utintr().
	 */
	sc->sc_flags &= ~DEV_WRITTEN;

	if (sc->sc_openf < 0 || (addr->utds&UTDS_MOL) == 0) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		bp->b_flags |= B_ERROR;
		goto next;
	}

	if (bp == &cutbuf[utunit]) {
		/*
		 * Execute a control operation with the specified
		 * count.
		 */
		if (bp->b_command == UT_SENSE)
			goto next;
		/*
		 * Set next state; handle timeouts
		 */
		if (bp->b_command == UT_REW) {
			um->um_tab.b_state = SREW;
			sc->sc_timo = 5*60;
		} else {
			um->um_tab.b_state = SCOM;
			sc->sc_timo = imin(imax(10*(int)-
					   bp->b_repcnt,60),5*60);
		}
		if (bp->b_command >= UT_SFORW &&
		    bp->b_command <= UT_SREVF)
			addr->utfc = -bp->b_repcnt;
		goto dobpcmd;
	}

	if (sc->sc_blkno != bdbtofsb(bp->b_blkno) &&
	    !um->um_tab.b_errcnt)
		sc->sc_blkno = bdbtofsb(bp->b_blkno);

	sc->sc_nxrec = bdbtofsb(bp->b_blkno)+1;

	/*
	 * If the tape is correctly positioned, set up all the
	 * registers but the csr, and give control over to the
	 * UNIBUS adaptor routines, to wait for resources to
	 * start I/O.
	 */
	if ((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		addr->utwc = -(((bp->b_bcount)+1)>>1);
		addr->utfc = -bp->b_bcount;
		if ((bp->b_flags&B_READ) == 0) {
			/*
			 * On write error retries erase the
			 * inter-record gap before rewriting.
			 */
			if (um->um_tab.b_errcnt) {
				if (um->um_tab.b_state != SERASED) {
					um->um_tab.b_state = SERASE;
					sc->sc_timo = 60;
					addr->utcs1 = UT_ERASE|UT_IE|
						      UT_GO;
					return;
				}
			}
			um->um_cmd = UT_WCOM;
		} else
			um->um_cmd = UT_RCOM;
		sc->sc_timo = 60;
		um->um_tab.b_state = SIO;
		(void) ubago(ui);
		return;
	}

	/*
	 * Tape positioned incorrectly; seek forwards or
	 * backwards to the correct spot.  This happens for
	 * raw tapes only on error retries.
	 */
	um->um_tab.b_state = SSEEK;
	if (blkno < bdbtofsb(bp->b_blkno)) {
		addr->utfc = blkno - bdbtofsb(bp->b_blkno);
		bp->b_command = UT_SFORW;
	} else {
		addr->utfc = bdbtofsb(bp->b_blkno) - blkno;
		bp->b_command = UT_SREV;
	}

	sc->sc_timo = imin(imax(10 * -addr->utfc, 60), 5*60);

dobpcmd:
	/*
	 * Perform the command setup in bp.
	 */
	addr->utcs1 = bp->b_command|UT_IE|UT_GO;
	return;

next:
	/*
	 * Advance to the next command in the slave queue,
	 * posting notice and releasing resources as needed.
	 */
	if (um->um_ubinfo)
		ubadone(um);
	um->um_tab.b_errcnt = 0;
	dp->b_actf = bp->av_forw;
	iodone(bp);
	goto loop;
}

/*
 * Start operation on controller --
 * UNIBUS resources have been allocated.
 */
utdgo(um)
	register struct uba_ctlr *um;
{
	register struct utdevice *addr = (struct utdevice *)um->um_addr;

	addr->utba = (u_short) um->um_ubinfo;
	addr->utcs1 = um->um_cmd|((um->um_ubinfo>>8)&0x300)|UT_IE|UT_GO;
}

/*
 * UT interrupt handler
 */
utintr(ut11)
	int ut11;
{
	register struct buf *bp;
	register struct uba_ctlr *um = utminfo[ut11];
	register struct utdevice *addr;
	register struct tj_softc *sc;
	register int state;
	register struct buf *dp;
	int utunit, unit;
	u_short cs2, cs1;

	if ((dp = um->um_tab.b_actf) == NULL)
		return;

	bp = dp->b_actf;
	utunit = UTUNIT(bp->b_dev);
	unit = UNIT(bp->b_dev);
	addr = (struct utdevice *)tjdinfo[unit]->ui_addr;
	sc = &tj_softc[unit];

	/*
	 * Record status...
	 */
	sc->sc_timo = INF;
	sc->sc_dsreg = addr->utds;
	sc->sc_erreg = addr->uter;
	sc->sc_resid = MASKREG(addr->utfc);
	if ((bp->b_flags&B_READ) == 0)
		sc->sc_flags |= DEV_WRITTEN;
	state = um->um_tab.b_state;
	um->um_tab.b_state = 0;

	/*
	 * Check for errors...
	 */
	if ((addr->utds & UTDS_ERR) || (addr->utcs1 & UT_TRE)) {
		/*
		 * To clear the ERR bit, we must issue a drive clear
		 * command, and to clear the TRE bit we must set the
		 * controller clear bit.
		 */
		cs2 = addr->utcs2;
		if ((cs1 = addr->utcs1)&UT_TRE)
			addr->utcs2 |= UTCS2_CLR;
		while ((addr->utcs1&UT_RDY) == 0)
			;
		addr->utcs1 = UT_CLEAR|UT_GO;

		if (sc->sc_dsreg & UTDS_EOT) {
			sc->sc_flags |= DEV_EOM;
			goto opdone;
		}

		/*
		 * If we were reading at 1600 or 6250 bpi and the error
		 * was corrected, then don't consider this an error.
		 */
		if (sc->sc_erreg & UTER_COR && (bp->b_flags & B_READ) &&
		    (addr->uttc & UTTC_DEN) != UTTC_NRZI) {
			mprintf("%s: unit# %d: soft error block# %d\n",
				sc->sc_device, unit, bp->b_blkno);
			mprintf("utcs1=%b uter=%b utcs2=%b utds=%b\n",
				cs1, UT_BITS, sc->sc_erreg,
				UTER_BITS, cs2, UTCS2_BITS,
				sc->sc_dsreg, UTDS_BITS);
			sc->sc_erreg &= ~UTER_COR;
		}

		/*
		 * If we were reading from a raw tape and the only error
		 * was that the record was too long, then we don't
		 * consider this an error.
		 */
		if (bp != &cutbuf[utunit] && (bp->b_flags&B_READ) &&
		    (sc->sc_erreg&UTER_FCE))
			sc->sc_erreg &= ~UTER_FCE;
		if (sc->sc_erreg == 0)
			goto ignoreerr;

		/*
		 * Fix up errors which occur due to backspacing
		 * "over" the front of the tape.
		 */
		if ((sc->sc_dsreg & UTDS_BOT) && bp->b_command ==
		    UT_SREV && ((sc->sc_erreg &=
		    ~(UTER_NEF|UTER_FCE)) == 0))
			goto opdone;

		/*
		 * Retry soft errors up to 8 times
		 */
		if ((sc->sc_erreg&UTER_HARD) == 0 && state == SIO) {
			if (++um->um_tab.b_errcnt < 7) {
				sc->sc_blkno++;
				sc->sc_flags |= DEV_RETRY;
				ubadone(um);
				goto opcont;
			}
		}

		/*
		 * Hard or non-I/O errors on non-raw tape
		 * cause it to close.
		 */
		if (sc->sc_openf > 0 && bp == &cutbuf[utunit])
			sc->sc_openf = -1;
		/*
		 * Couldn't recover error.
		 */
		sc->sc_flags |= DEV_HARDERR;
		mprintf("%s: unit# %d: hard error block# %d\n",
			sc->sc_device, unit, bp->b_blkno);
		mprintf("utcs1=%b uter=%b utcs2=%b utds=%b\n",
			cs1, UT_BITS, sc->sc_erreg,
			UTER_BITS, cs2, UTCS2_BITS,
			sc->sc_dsreg, UTDS_BITS);
		bp->b_flags |= B_ERROR;
		goto opdone;
	}

ignoreerr:
	/*
	 * If we hit EOT set flag.
	 */
		if (sc->sc_dsreg & UTDS_EOT) {
			sc->sc_flags |= DEV_EOM;
		}
	/*
	 * If we hit a tape mark update our position.
	 */
	if (sc->sc_dsreg & UTDS_TM && bp->b_flags & B_READ) {
		    sc->sc_category_flags |= DEV_TPMARK;
		/*
		 * Set blkno and nxrec
		 */
		if (bp == &cutbuf[utunit]) {
			if (sc->sc_blkno > bdbtofsb(bp->b_blkno)) {
				sc->sc_nxrec =
				     bdbtofsb(bp->b_blkno) - addr->utfc;
				sc->sc_blkno = sc->sc_nxrec;
			} else {
				sc->sc_blkno =
				     bdbtofsb(bp->b_blkno) + addr->utfc;
				sc->sc_nxrec = sc->sc_blkno-1;
			}
		} else
			sc->sc_nxrec = bdbtofsb(bp->b_blkno);
		/*
		 * Note: if we get a tape mark on a read, the
		 * frame count register will be zero, so b_resid
		 * will be calculated correctly below.
		 */
		goto opdone;
	}
	/*
	 * Advance tape control FSM.
	 */
	switch (state) {

	case SIO:		/* read/write increments tape block # */
		sc->sc_blkno++;
		break;

	case SCOM:		/* motion commands update position */
		if (bp == &cutbuf[utunit])
		switch (bp->b_command) {

		case UT_SFORW:
			sc->sc_blkno -= bp->b_repcnt;
			break;

		case UT_SREV:
			sc->sc_blkno += bp->b_repcnt;
			break;

		case UT_REWOFFL:
			addr->utcs1 = UT_CLEAR|UT_GO;
			break;
		}
		break;

	case SSEEK:
		sc->sc_blkno = bdbtofsb(bp->b_blkno);
		goto opcont;

	case SERASE:
		/*
		 * Completed erase of the inter-record gap due to a
		 * write error; now retry the write operation.
		 */
		um->um_tab.b_state = SERASED;
		goto opcont;

	case SREW:			/* clear attention bit */
		addr->utcs1 = UT_CLEAR|UT_GO;
		break;

	default:
		mprintf("bad state %d\n", state);
		panic("utintr");
	}

opdone:
	/*
	 * Reset error count and remove
	 * from device queue
	 */
	um->um_tab.b_errcnt = 0;
	dp->b_actf = bp->av_forw;
	sc->sc_flags |= DEV_DONE;
	/*
	 * For read command, frame count register contains
	 * actual length of tape record.  Otherwise, it
	 * holds negative residual count.
	 */
	if (state == SIO && um->um_cmd == UT_RCOM) {
		bp->b_resid = 0;
		if (bp->b_bcount > MASKREG(addr->utfc))
			bp->b_resid = bp->b_bcount - MASKREG(addr->utfc);
	} else
		bp->b_resid = MASKREG(-addr->utfc);
	ubadone(um);
	iodone(bp);
	/*
	 * Circulate slave to end of controller queue
	 * to give other slaves a chance
	 */
	um->um_tab.b_actf = dp->b_forw;
	if (dp->b_actf) {
		dp->b_forw = NULL;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else
			um->um_tab.b_actl->b_forw = dp;
		um->um_tab.b_actl = dp;
	}
	if (um->um_tab.b_actf == 0)
		return;
opcont:
	utstart(um);
}

/*
 * Watchdog timer routine.
 */
uttimer(dev)
	register int dev;
{
	register struct tj_softc *sc = &tj_softc[UNIT(dev)];
	register int utunit = UTUNIT(dev);
	register int unit = UNIT(dev);
	register short x;

	if (sc->sc_timo != INF && (sc->sc_timo -= 5) < 0) {
		mprintf("%s: unit# %d: lost interrupt\n", sc->sc_device, unit);
		sc->sc_timo = INF;
		x = spl5();
		utintr(utunit);
		(void) splx(x);
	}
	timeout(uttimer, (caddr_t)dev, 5*hz);
}

/*
 * Raw interface for a read
 */
utread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int utunit = UTUNIT(dev);

	return (physio(utstrategy, &rutbuf[utunit], dev, B_READ,
		minphys, uio));
}

/*
 * Raw interface for a write
 */
utwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int utunit = UTUNIT(dev);

	return (physio(utstrategy, &rutbuf[utunit], dev, B_WRITE,
		minphys, uio));
}

utioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct uba_device *ui = tjdinfo[UTUNIT(dev)];
	register struct tj_softc *sc = &tj_softc[UNIT(dev)];
	register struct buf *bp = &cutbuf[UTUNIT(dev)];
	register int callcount;
	register int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	struct devget *devget;
	int unit = UNIT(dev);

	/* we depend of the values and order of the MT codes here */
	static utops[] = { UT_WEOF, UT_SFORWF, UT_SREVF, UT_SFORW,
			   UT_SREV, UT_REW, UT_REWOFFL, UT_SENSE };

	switch (cmd) {

	case MTIOCTOP:				/* tape operation */
		mtop = (struct mtop *)data;
		switch(mtop->mt_op) {

		case MTWEOF:
		case MTFSF: case MTBSF:
		case MTFSR: case MTBSR:
			callcount = mtop->mt_count;
			fcount = 1;
			break;

		case MTREW: case MTOFFL:
			sc->sc_flags &= ~DEV_EOM;
			callcount = 1;
			fcount = 1;
			break;

		case MTNOP: case MTCACHE: case MTNOCACHE:
		case MTCLX: case MTCLS: 
			return(0);

		case MTCSE:
			sc->sc_flags |= DEV_CSE;
			sc->sc_category_flags &= ~DEV_TPMARK;
			return(0);

		case MTENAEOT:
			dis_eot_tj[unit] = 0;
			return(0);

		case MTDISEOT:
			dis_eot_tj[unit] = DISEOT;
			sc->sc_flags &= ~DEV_EOM;
			return(0);
	
		case MTFLUSH:
			/*
			 * Flush controller's write back cache.  Since this
			 * driver can not support this functionality, return
			 * ENXIO to indicate the lack of support.
			 */
			return (ENXIO);

		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (EINVAL);
		while (--callcount >= 0) {
			utcommand(dev, utops[mtop->mt_op], fcount);
			if ((bp->b_flags & B_ERROR) ||
			    (sc->sc_dsreg & UTDS_BOT))
				break;
		}
		return (geterror(bp));

	case MTIOCGET:				/* tape status */
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_dsreg;
		mtget->mt_erreg = sc->sc_erreg;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISUT;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TAPE;
		devget->bus = DEV_UB;
		bcopy(DEV_UNKNOWN,devget->interface,
		      strlen(DEV_UNKNOWN));
		bcopy(sc->sc_device,devget->device,
		      strlen(sc->sc_device));
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA	*/
		devget->ctlr_num = ui->ui_ctlr; 	/* which interf.*/
		devget->slave_num = ui->ui_slave;	/* which plug	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "tj"	*/
		devget->unit_num = unit;		/* which tj??	*/
		devget->soft_count = sc->sc_softcnt;	/* soft er. cnt.*/
		devget->hard_count = sc->sc_hardcnt;	/* hard er. cnt.*/
		devget->stat = sc->sc_flags;		/* status	*/
		devget->category_stat = sc->sc_category_flags;	/* c.st.*/
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

utreset(uban)
	register int uban;
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register struct buf *dp;
	register int ut11;
	register int unit;
	struct tj_softc *sc;

	for (ut11 = 0; ut11 < nNUT; ut11++) {
		if ((um = utminfo[ut11]) == 0 || um->um_alive == 0 ||
		   um->um_ubanum != uban)
			continue;
		um->um_tab.b_state = 0;
		um->um_tab.b_actf = um->um_tab.b_actl = 0;
		if (um->um_ubinfo) {
			mprintf("ut reset");
			mprintf("<%d>", (um->um_ubinfo>>28)&0xf);
			um->um_ubinfo = 0;
		}
		((struct utdevice *)(um->um_addr))->utcs1 =
							 UT_CLEAR|UT_GO;
		((struct utdevice *)(um->um_addr))->utcs2 |= UTCS2_CLR;
		for (unit = 0; unit < nNTJ; unit++) {
			if ((ui = tjdinfo[unit]) == 0 || ui->ui_mi != um ||
			    ui->ui_alive == 0)
				continue;
			sc = &tj_softc[unit];
			dp = &tjutab[unit];
			dp->b_state = 0;
			dp->b_forw = 0;
			if (um->um_tab.b_actf == NULL)
				um->um_tab.b_actf = dp;
			else
				um->um_tab.b_actl->b_forw = dp;
			um->um_tab.b_actl = dp;
			if (sc->sc_openf > 0)
				sc->sc_openf = -1;
		}
		utstart(um);
	}
}

/*
 * Do a stand-alone core dump to tape --
 * from here down, routines are used only in dump context
 */
utdump()
{
	register struct uba_device *ui;
	register struct uba_regs *up;
	register struct utdevice *addr;
	register int blk;
	register int num = maxfree;
	register int start = 0;
	int	ubatype;

	if (tjdinfo[0] == 0)
		return (ENXIO);
	ui = PHYS(tjdinfo[0], struct uba_device *);
	up = PHYS(ui->ui_hd, struct uba_hd *)->uh_physuba;
	ubatype = PHYS(ui->ui_hd, struct uba_hd *)->uba_type;
	ubainit(up,ubatype);
	DELAY(500000);
	addr = (struct utdevice *)ui->ui_physaddr;
	utwait(addr);
	/*
	 * Be sure to set the appropriate density here.  We use
	 * 6250, but maybe it should be done at 1600 to insure the
	 * tape can be read by most any other tape drive available.
	 */
	addr->uttc = UTTC_GCR|UTTC_PDP11FMT;   /* implicit slave 0 */
	addr->utcs1 = UT_CLEAR|UT_GO;
	while (num > 0) {
		blk = num > DBSIZE ? DBSIZE : num;
		utdwrite(start, blk, addr, up);
		if ((addr->utds&UTDS_ERR) || (addr->utcs1&UT_TRE))
			return(EIO);
		start += blk;
		num -= blk;
	}
	uteof(addr);
	uteof(addr);
	utwait(addr);
	if ((addr->utds&UTDS_ERR) || (addr->utcs1&UT_TRE))
		return(EIO);
	addr->utcs1 = UT_REW|UT_GO;
	return (0);
}

utdwrite(dbuf, num, addr, up)
	register int dbuf;
	register int num;
	register struct utdevice *addr;
	register struct uba_regs *up;
{
	register struct pte *io;
	register int npf;

	utwait(addr);
	io = up->uba_map;
	npf = num + 1;
	while (--npf != 0)
		*(int *)io++ = (dbuf++ | (1<<UBAMR_DPSHIFT) | UBAMR_MRV);
	*(int *)io = 0;
	addr->utwc = -((num*NBPG)>>1);
	addr->utfc = -(num*NBPG);
	addr->utba = 0;
	addr->utcs1 = UT_WCOM|UT_GO;
}

utwait(addr)
	register struct utdevice *addr;
{
	register int s;

	do
		s = addr->utds;
	while ((s&UTDS_DRY) == 0);
}

uteof(addr)
	register struct utdevice *addr;
{

	utwait(addr);
	addr->utcs1 = UT_WEOF|UT_GO;
}
#endif
