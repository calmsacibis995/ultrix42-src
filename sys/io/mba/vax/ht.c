
#ifndef lint
static char *sccsid = "@(#)ht.c	4.1      (ULTRIX)        7/2/90";
#endif lint

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
 * ht.c    6.1	   07/29/83
 *
 * Modification history
 *
 * TM03/TE16/TU45/TU77 tape driver
 *
 *
 *  6-Aug-85 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: ht.c	6.1	83/07/29.
 *	Added new ioctl functionality as well as new code to handle
 *	EOT correctly. Driver now supports new minor number convention
 *	to allow for more than 4 tape devices. V2.0
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code along with
 *	soft and hard error counters. V2.0
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 * 30-Oct-86 - lp
 *	TPMARK & HARDERR now prevent further n-buffered requests from 
 *      being posted.
 *
 * 04-Dec-86 - pmk
 *	Changed mprintf's to log just one entry.
 *
 * 05-Jun-89 - Tim Burke
 *	Added a new MTIOCTOP called MTFLUSH.  This is used on caching drives
 *	to force a flush of the controller's write back cache.  Since this 
 *	is not a caching drive the MTFLUSH always returns ENXIO.
 */

#include "tu.h"
#if NHT > 0 || defined(BINARY)

#include "../data/ht_data.c"

short	httypes[] = { MBDT_TM03, MBDT_TE16, MBDT_TU45, MBDT_TU77, 0 };
int	htattach(), htslave(), htustart(), htndtint(), htdtint();

struct	mba_driver htdriver = { htattach, htslave, htustart, 0,
				htdtint, htndtint, httypes, "ht",
				"tu", htinfo };

htattach(mi)
	register struct mba_device *mi;
{
}

htslave(mi, ms, sn)
	register struct mba_device *mi;
	register struct mba_slave *ms;
	register int sn;
{
	register struct htdevice *htaddr = (struct htdevice *)mi->mi_drv;
	register struct tu_softc *sc = &tu_softc[ms->ms_unit];
	register int s = spl7();
	int rtn = 0;

	htaddr->httc = sn;

	if (htaddr->htdt & HTDT_SPR) {
		sc->sc_mi = mi;
		sc->sc_slave = sn;
		tutoht[ms->ms_unit] = mi->mi_unit;
		rtn = 1;
		sc->sc_softcnt = 0;
		sc->sc_hardcnt = 0;
		switch(*(mi->mi_driver->md_type + mi->mi_type)) {

		case MBDT_TE16:
			bcopy(DEV_TE16,sc->sc_device,
			      strlen(DEV_TE16));
			break;

		case MBDT_TU45:
			bcopy(DEV_TU45,sc->sc_device,
			      strlen(DEV_TU45));
			break;

		case MBDT_TU77: case MBDT_TM03: /* TM03/TU77 default */
			bcopy(DEV_TU77,sc->sc_device,
			      strlen(DEV_TU77));
			break;

		default:
			bcopy(DEV_UNKNOWN,sc->sc_device,
			      strlen(DEV_UNKNOWN));
			break;
		}
	}

	splx(s);
	return (rtn);
}

htopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct mba_device *mi;
	register struct tu_softc *sc;
	register int unit = UNIT(dev);
	register int sel = SEL(dev);

	if (unit >= nNTU || (sc = &tu_softc[unit])->sc_openf ||
	    (mi = htinfo[HTUNIT(dev)]) == 0 || mi->mi_alive == 0 ||
	    sc->sc_mi == 0) {
		return (ENXIO);
	}

	sc->sc_dsreg = (u_short) MASKREG(((struct htdevice *)mi->mi_drv)->htds);
	if ((sc->sc_dsreg & HTDS_EOT) &&
	    (dis_eot_tu[unit] != DISEOT)) {
		sc->sc_flags &= DEV_EOM;
	} else {
		sc->sc_flags = 0;
	}
	sc->sc_category_flags = 0;

	sc->sc_dens = (((sel == MTHR) || (sel == MTHN)) ?
			      HTTC_1600BPI:HTTC_800BPI)|HTTC_PDP11|
			      sc->sc_slave;
	htcommand(dev, HT_SENSE, 1);
	if((sel == MTHR) || (sel == MTHN)) {
		sc->sc_category_flags |= DEV_1600BPI;
	} else {
		sc->sc_category_flags |= DEV_800BPI;
	}

	if (!(sc->sc_dsreg & HTDS_MOL)) {
		sc->sc_flags |= DEV_OFFLINE;
		if(!(flag & FNDELAY)) {
			DEV_UGH(sc->sc_device,unit,"offline");
			return(EIO);
		}
	}

	if (sc->sc_dsreg & HTDS_WRL) {
		sc->sc_flags |= DEV_WRTLCK;
	}
	if ((flag & FWRITE) && (sc->sc_dsreg & HTDS_WRL)) {
		if(!(flag & FNDELAY)) {
			DEV_UGH(sc->sc_device,unit,"write locked");
			return(EIO);
		}
	}

	sc->sc_openf = 1;
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	return (0);
}

htclose(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct tu_softc *sc = &tu_softc[UNIT(dev)];
	register int unit = UNIT(dev);
	register int sel = SEL(dev);

	sc->sc_flags &= ~DEV_EOM;

	if (flag == FWRITE || ((flag & FWRITE) &&
	    (sc->sc_flags & DEV_WRITTEN))) {
		htcommand(dev, HT_WEOF, 1);
		sc->sc_flags &= ~DEV_EOM;
		htcommand(dev, HT_WEOF, 1);
		sc->sc_flags &= ~DEV_EOM;
		htcommand(dev, HT_SREV, 1);
		sc->sc_flags &= ~DEV_EOM;
	}

	if ((sel == MTLR) || (sel == MTHR)) {
		htcommand(dev, HT_REW, 0);
	}

	sc->sc_openf = 0;

	if ((sc->sc_dsreg & HTDS_EOT) && (dis_eot_tu[unit] != DISEOT)){
		sc->sc_flags |= DEV_EOM;
	}
}

htcommand(dev, com, count)
	register dev_t dev;
	register int com;
	register int count;
{
	register struct buf *bp = &chtbuf[HTUNIT(dev)];
	register int s = spl5();

	while (bp->b_flags & B_BUSY) {
		if(bp->b_repcnt == 0 && (bp->b_flags & B_DONE))
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
	htstrategy(bp);

	if (count == 0)
		return;

	iowait(bp);

	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);

	bp->b_flags &= B_ERROR;
}

htstrategy(bp)
	register struct buf *bp;
{
	register struct mba_device *mi = htinfo[HTUNIT(bp->b_dev)];
	register struct htdevice *htaddr = (struct htdevice *)mi->mi_drv;
	register struct tu_softc *sc = &tu_softc[UNIT(bp->b_dev)];
	register struct buf *dp;
	register int s;
	int unit = UNIT(bp->b_dev);

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_tu[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	if((bp->b_flags&B_READ) && (bp->b_flags&B_RAWASYNC) &&
	((sc->sc_category_flags&DEV_TPMARK) || (sc->sc_flags&DEV_HARDERR))) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	bp->av_forw = NULL;
	dp = &mi->mi_tab;
	s = spl5();
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if (dp->b_active == 0)
		mbustart(mi);
	splx(s);
}

htustart(mi)
	register struct mba_device *mi;
{
	register struct htdevice *htaddr = (struct htdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct tu_softc *sc = &tu_softc[UNIT(bp->b_dev)];
	register int htunit = HTUNIT(bp->b_dev);
	register daddr_t blkno;
	int unit = UNIT(bp->b_dev);

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_tu[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		return (MBU_NEXT);
	}

	if(((sc->sc_category_flags&DEV_TPMARK) || (sc->sc_flags&DEV_HARDERR))
	    && (bp->b_flags & B_READ) && (bp->b_flags&B_RAWASYNC)) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		return (MBU_NEXT);
	}

	htaddr->httc = sc->sc_dens;
	sc->sc_dsreg = htaddr->htds;
	sc->sc_erreg = htaddr->hter;
	sc->sc_resid = htaddr->htfc;
	sc->sc_flags &= ~DEV_WRITTEN;
	sc->sc_category_flags &= ~DEV_RWDING;

	if ((htaddr->htdt & HTDT_SPR) == 0 || (htaddr->htds & HTDS_MOL) == 0)
		if (sc->sc_openf > 0)
			sc->sc_openf = -1;

	if (sc->sc_openf < 0) {
		bp->b_flags |= B_ERROR;
		return (MBU_NEXT);
	}

	if (bp != &chtbuf[htunit]) {
		if (sc->sc_blkno != bdbtofsb(bp->b_blkno) &&
			!mi->mi_tab.b_errcnt)
			sc->sc_blkno = bdbtofsb(bp->b_blkno);
			sc->sc_nxrec = bdbtofsb(bp->b_blkno) + 1;
	} else {
		if (bp->b_command == HT_SENSE) {
			sc->sc_flags |= DEV_DONE;
			return (MBU_NEXT);
		}
		if (bp->b_command == HT_REW)
			sc->sc_category_flags |= DEV_RWDING;
		else
			htaddr->htfc = -bp->b_bcount;
		htaddr->htcs1 = bp->b_command|HT_GO;
		return (MBU_STARTED);
	}

	if ((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		htaddr->htfc = -bp->b_bcount;
		if ((bp->b_flags&B_READ) == 0) {
			if (mi->mi_tab.b_errcnt) {
				if ((sc->sc_flags & DEV_ERASED) == 0) {
					sc->sc_flags |= DEV_ERASED;
					sc->sc_softcnt++;
					htaddr->htcs1 = HT_ERASE | HT_GO;
					return (MBU_STARTED);
				}
				sc->sc_flags &= ~DEV_ERASED;
			}
		}
		return (MBU_DODATA);
	}

	if (blkno < bdbtofsb(bp->b_blkno)) {
		htaddr->htfc = blkno - bdbtofsb(bp->b_blkno);
		htaddr->htcs1 = HT_SFORW|HT_GO;
	} else {
		htaddr->htfc = bdbtofsb(bp->b_blkno) - blkno;
		htaddr->htcs1 = HT_SREV|HT_GO;
	}

	return (MBU_STARTED);
}

htdtint(mi, mbsr)
	register struct mba_device *mi;
	int mbsr;
{
	register struct htdevice *htaddr = (struct htdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct tu_softc *sc = &tu_softc[UNIT(bp->b_dev)];
	register int ds = sc->sc_dsreg = MASKREG(htaddr->htds);
	register int mbs = mbsr;
	int htunit = HTUNIT(bp->b_dev);
	int unit = UNIT(bp->b_dev);
	int er = sc->sc_erreg = MASKREG(htaddr->hter);
	u_short ht_regs[10];

	sc->sc_resid = MASKREG(htaddr->htfc);
	sc->sc_blkno++;

	if((bp->b_flags & B_READ) == 0)
		sc->sc_flags |= DEV_WRITTEN;

	if((ds & HTDS_EOT) && (dis_eot_tu[unit] != DISEOT)){
		sc->sc_flags |= DEV_EOM;
		if (!((ds & (HTDS_ERR|HTDS_MOL)) != HTDS_MOL ||
		    mbs & MBSR_EBITS)) {
			sc->sc_flags |= DEV_DONE;
			bp->b_resid = 0;
			return(MBD_DONE);
		}
		bp->b_resid = 0;
	}else{
		if(dis_eot_tu[unit] != DISEOT){
		sc->sc_flags &= ~(DEV_EOM|DEV_CSE);
		}
	}

	if ((ds & (HTDS_ERR|HTDS_MOL)) != HTDS_MOL || mbs & MBSR_EBITS) {
		/*
		 * save the registers before we clear them
		 */
		ht_regs[0] = MASKREG(htaddr->htcs1);
		ht_regs[1] = MASKREG(htaddr->htds);
		ht_regs[2] = MASKREG(htaddr->hter);
		ht_regs[3] = MASKREG(htaddr->htmr);
		ht_regs[4] = MASKREG(htaddr->htas);
		ht_regs[5] = MASKREG(htaddr->htfc);
		ht_regs[6] = MASKREG(htaddr->htdt);
		ht_regs[7] = MASKREG(htaddr->htck);
		ht_regs[8] = MASKREG(htaddr->htsn);
		ht_regs[9] = MASKREG(htaddr->httc);
		htaddr->htcs1 = HT_DCLR|HT_GO;
		mbclrattn(mi);

		if (bp != &chtbuf[htunit]) {
			er &= ~HTER_FCE;
			mbs &= ~(MBSR_DTABT|MBSR_MBEXC);
		}

		if (bp->b_flags & B_READ && ds & HTDS_PES) {
			sc->sc_category_flags |= DEV_SHRTREC;
			er &= ~(HTER_CSITM|HTER_CORCRC);
		}

		if (er & HTER_HARD || mbs & MBSR_EBITS ||
		    (ds & HTDS_MOL) == 0 || er &&
		    ++mi->mi_tab.b_errcnt >= 7) {
			sc->sc_flags |= DEV_HARDERR;
			sc->sc_hardcnt++;
			if ((ds & HTDS_MOL) == 0 && sc->sc_openf > 0) {
				sc->sc_flags |= DEV_OFFLINE;
				sc->sc_openf = -1;
			}
			if ((er & HTER_HARD) == HTER_FCE &&
			    (mbs & MBSR_EBITS) == (MBSR_DTABT|MBSR_MBEXC)
			    && (ds & HTDS_MOL))
				goto noprint;

			mprintf("%s: unit#:%d hard err blk#:%d mbsr:%b \
			        htcs1:%x htds:%x hter:%x htmr:%x htas:%x \
				htfc:%x htdt:%x htck:%x htsn:%x httc:%x\n",
				sc->sc_device, unit, bp->b_blkno,
				mbsr, mbsr_bits, ht_regs[0], ht_regs[1], 
				ht_regs[2], ht_regs[3], ht_regs[4], ht_regs[5],
				ht_regs[6], ht_regs[7], ht_regs[8], ht_regs[9]);
noprint:
			bp->b_flags |= B_ERROR;
			sc->sc_flags |= DEV_DONE;
			return (MBD_DONE);
		}

		if (er) {
			sc->sc_flags |= DEV_RETRY;
			return (MBD_RETRY);
		}
	}

	bp->b_resid = 0;
	if (bp->b_flags & B_READ)
		if (ds & HTDS_TM) {
			sc->sc_category_flags |= DEV_TPMARK;
			bp->b_resid = bp->b_bcount;
			sc->sc_nxrec = bdbtofsb(bp->b_blkno);
		} else if(bp->b_bcount > MASKREG(htaddr->htfc))
			bp->b_resid = bp->b_bcount -
				      MASKREG(htaddr->htfc);
	sc->sc_flags |= DEV_DONE;
	return (MBD_DONE);
}

htndtint(mi)
	register struct mba_device *mi;
{
	register struct htdevice *htaddr = (struct htdevice *)mi->mi_drv;
	register struct tu_softc *sc;
	register struct buf *bp = mi->mi_tab.b_actf;
	register int htunit;
	register int unit;
	int er, ds, fc;
	u_short ht_regs[10];

	if (bp == 0) {
		return (MBN_SKIP);	/* Just return if unexpected */
	}

	htunit = HTUNIT(bp->b_dev);
	unit = UNIT(bp->b_dev);
	sc = &tu_softc[unit];
	sc->sc_erreg = er = MASKREG(htaddr->hter);
	sc->sc_dsreg = ds = MASKREG(htaddr->htds);
	sc->sc_resid = fc = MASKREG(htaddr->htfc);

	if (er) {

		/*
		 * save the registers before we clear them
		 */
		ht_regs[0] = MASKREG(htaddr->htcs1);
		ht_regs[1] = MASKREG(htaddr->htds);
		ht_regs[2] = MASKREG(htaddr->hter);
		ht_regs[3] = MASKREG(htaddr->htmr);
		ht_regs[4] = MASKREG(htaddr->htas);
		ht_regs[5] = MASKREG(htaddr->htfc);
		ht_regs[6] = MASKREG(htaddr->htdt);
		ht_regs[7] = MASKREG(htaddr->htck);
		ht_regs[8] = MASKREG(htaddr->htsn);
		ht_regs[9] = MASKREG(htaddr->httc);
		htaddr->htcs1 = HT_DCLR|HT_GO;
		mbclrattn(mi);
	}

	if((ds & HTDS_EOT) && (dis_eot_tu[unit] != DISEOT)) {
		sc->sc_flags |= DEV_EOM;
	} else {
		if(dis_eot_tu[unit] != DISEOT)
			sc->sc_flags &= ~(DEV_EOM|DEV_CSE);
	}

	if (bp == &chtbuf[htunit]) {

		switch (bp->b_command) {

		case HT_REWOFFL:
			sc->sc_flags |= DEV_OFFLINE;
			ds |= HTDS_MOL;
			break;

		case HT_SREV:
			sc->sc_category_flags &= ~DEV_RWDING;
			if (er == (HTER_NEF|HTER_FCE) && ds & HTDS_BOT &&
			    bp->b_repcnt == INF) {
				er &= ~HTER_NEF;
				sc->sc_flags |= DEV_BOM;
			}
			break;
		}

		er &= ~HTER_FCE;
		if (er == 0)
			ds &= ~HTDS_ERR;
	}

	if ((ds & (HTDS_ERR|HTDS_MOL)) != HTDS_MOL) {
		sc->sc_flags |= DEV_HARDERR;
		sc->sc_hardcnt++;
		if ((ds & HTDS_MOL) == 0 && sc->sc_openf > 0) {
			sc->sc_flags |= DEV_OFFLINE;
			sc->sc_openf = -1;
		}

		mprintf("%s: unit#:%d hard err blk#:%d \
		        htcs1:%x htds:%x hter:%x htmr:%x htas:%x \
			htfc:%x htdt:%x htck:%x htsn:%x httc:%x\n",
			sc->sc_device, unit, bp->b_blkno,
			ht_regs[0], ht_regs[1], ht_regs[2], ht_regs[3],
			ht_regs[4], ht_regs[5], ht_regs[6], ht_regs[7],
			ht_regs[8], ht_regs[9]);

		bp->b_flags |= B_ERROR;
		sc->sc_flags |= DEV_DONE;
		return (MBN_DONE);
	}

	if (bp == &chtbuf[htunit]) {
		if (sc->sc_category_flags & DEV_RWDING) {
			if (ds & HTDS_BOT) {
				sc->sc_flags |= (DEV_DONE | DEV_BOM);
				sc->sc_dsreg &= ~HTDS_EOT;
			} else {
				sc->sc_flags |= DEV_RETRY;
			}
			return (ds & HTDS_BOT ? MBN_DONE : MBN_RETRY);
		}
	bp->b_resid = -sc->sc_resid;
	sc->sc_flags |= DEV_DONE;
	return (MBN_DONE);
	}
	if (ds & HTDS_TM) {
		sc->sc_category_flags |= DEV_TPMARK;
		if (sc->sc_blkno > bdbtofsb(bp->b_blkno)) {
			sc->sc_nxrec = bdbtofsb(bp->b_blkno) - fc;
			sc->sc_blkno = sc->sc_nxrec;
		} else {
			sc->sc_blkno = bdbtofsb(bp->b_blkno) + fc;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
	} else
		sc->sc_blkno = bdbtofsb(bp->b_blkno);

	sc->sc_flags |= DEV_RETRY;
	return (MBN_RETRY);
}

htread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int htunit = HTUNIT(dev);

	return (physio(htstrategy, &rhtbuf[htunit], dev, B_READ,
		minphys, uio));
}

htwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int htunit = HTUNIT(dev);

	return (physio(htstrategy, &rhtbuf[htunit], dev, B_WRITE,
		minphys, uio));
}

htioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct mba_device *mi = htinfo[HTUNIT(dev)];
	register struct tu_softc *sc = &tu_softc[UNIT(dev)];
	register struct buf *bp = &chtbuf[HTUNIT(dev)];
	register int callcount;
	register int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	struct devget *devget;
	int unit = UNIT(dev);

	/* we depend of the values and order of the MT codes here */
	static htops[] = { HT_WEOF,HT_SFORW,HT_SREV,HT_SFORW,HT_SREV,
			   HT_REW,HT_REWOFFL,HT_SENSE };

	switch (cmd) {

	case MTIOCTOP:				/* tape operation */
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {

		case MTWEOF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;

		case MTFSF: case MTBSF:
			callcount = mtop->mt_count;
			fcount = INF;
			break;

		case MTFSR: case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
			break;

		case MTREW: case MTOFFL:
			sc->sc_flags &= ~DEV_EOM;
			callcount = 1;
			fcount = 1;
			break;

		case MTNOP: case MTCACHE:
		case MTNOCACHE: 
			return(0);

		case MTCSE:
			sc->sc_flags |= DEV_CSE;
			sc->sc_category_flags &= ~DEV_TPMARK;
			return(0);

		case MTCLX: case MTCLS:
			return(0);

		case MTENAEOT:
			dis_eot_tu[unit] = 0;
			return(0);

		case MTDISEOT:
			dis_eot_tu[unit] = DISEOT;
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
			htcommand(dev, htops[mtop->mt_op], fcount);
			if ((mtop->mt_op == MTFSR || mtop->mt_op
			    == MTBSR) && bp->b_resid) {
				return (EIO);
			}
			if ((bp->b_flags & B_ERROR) ||
			    sc->sc_dsreg & HTDS_BOT) {
				break;
			}
		}
		return (geterror(bp));

	case MTIOCGET:				/* tape status */
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_dsreg;
		mtget->mt_erreg = sc->sc_erreg;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISHT;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TAPE;
		devget->bus = DEV_MB;
		bcopy(DEV_TM03,devget->interface,
		      strlen(DEV_TM03));
		bcopy(sc->sc_device,devget->device,
		      strlen(sc->sc_device));
		devget->adpt_num = mi->mi_adpt; 	/* which adapter*/
		devget->nexus_num = mi->mi_nexus;	/* which nexus	*/
		devget->bus_num = mi->mi_mbanum;	/* which MBA	*/
		devget->ctlr_num = mi->mi_drive;	/* which TM03	*/
		devget->slave_num = sc->sc_slave;	/* which plug	*/
		bcopy(mi->mi_driver->md_sname,
		      devget->dev_name,
		      strlen(mi->mi_driver->md_sname)); /* Ult. "t[eu]" */
		devget->unit_num = unit;		/* which t[eu]??*/
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

htdump()
{
	register struct mba_device *mi;
	register struct mba_regs *mp;
	register struct htdevice *htaddr;
	register int blk;
	register int num;
	register int start;

	start = 0;
	num = maxfree;
	if (htinfo[0] == 0)
		return (ENXIO);
	mi = PHYS(htinfo[0], struct mba_device *);
	mp = PHYS(mi->mi_hd, struct mba_hd *)->mh_physmba;
	mp->mba_cr = MBCR_IE;
	htaddr = (struct htdevice *)&mp->mba_drv[mi->mi_drive];
	htaddr->httc = HTTC_PDP11|HTTC_1600BPI;
	htaddr->htcs1 = HT_DCLR|HT_GO;
	while (num > 0) {
		blk = num > DBSIZE ? DBSIZE : num;
		htdwrite(start, blk, htaddr, mp);
		start += blk;
		num -= blk;
	}
	hteof(htaddr);
	hteof(htaddr);
	htwait(htaddr);
	if (htaddr->htds&HTDS_ERR)
		return (EIO);
	htaddr->htcs1 = HT_REW|HT_GO;
	return (0);
}

htdwrite(dbuf, num, htaddr, mp)
	register int dbuf;
	register int num;
	register struct htdevice *htaddr;
	register struct mba_regs *mp;
{
	register struct pte *io;
	register int i;

	htwait(htaddr);
	io = mp->mba_map;
	for (i = 0; i < num; i++)
		*(int *)io++ = dbuf++ | PG_V;
	htaddr->htfc = -(num*NBPG);
	mp->mba_sr = -1;
	mp->mba_bcr = -(num*NBPG);
	mp->mba_var = 0;
	htaddr->htcs1 = HT_WCOM|HT_GO;
}

htwait(htaddr)
	register struct htdevice *htaddr;
{
	register int s;

	do
		s = htaddr->htds;
	while ((s & HTDS_DRY) == 0);
}

hteof(htaddr)
	register struct htdevice *htaddr;
{
	htwait(htaddr);
	htaddr->htcs1 = HT_WEOF|HT_GO;
}
#endif
