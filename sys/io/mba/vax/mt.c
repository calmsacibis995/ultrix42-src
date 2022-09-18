
#ifndef lint
static char *sccsid = "@(#)mt.c	4.1      (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1987, 1989 by		*
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
 * mt.c    6.1	   07/29/83
 *
 * Modification history
 *
 * TM78/TU78 tape driver
 *
 * V1.0 - tresvik
 *
 *	Derived from 4.2BSD labeled: mt.c	6.1	83/07/29.
 *	Changed error message format.
 *
 *  5-Oct-84 - tresvik
 *
 *	Keep the tape from running off the end of the tape.  This fix
 *	will at least return an error it the utility cares to look at
 *	it.  `cat', for example ignores errors on output and the tape
 *	will still go off the end.
 *
 *  6-Aug-85 - ricky palmer
 *
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
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 * 30-Oct-86 - lp
 *
 *	TPMARK & HARDERR now prevent further n-buffered i/o requests.
 *
 * 04-Dec-86 - pmk
 *
 *	Changed mprintf's to log just one entry.
 *
 * 08-Jan-87 - lp
 *
 *	Change MTSENSE in open to happen before looking at any status.
 *
 * 29-Jan-87 - lp
 *
 *	changed order of MTSENSE in open to guarantee correct state.
 *
 * 11-Feb-87 - robin
 *
 *	changed the slave routine to fail only if the drive is not there and
 *	to report unexpected interupts to the console and error log.
 *
 * 02-Mar-87 - pmk
 *
 *	Changed slave routine: return if sn > 4 (max # of slaves),
 *	check slave attn. addr. for correct unit & do MTID_CLR on
 *	controller errors.
 *
 * 09-May-87 - Ricky Palmer (rsp)
 *
 *	Added new code to correctly handle EOT in all cases, hard errors,
 *	and dual ported subsystems.
 *
 * 08-Jul-87 - rsp
 *
 *	Fixed sporadic problem with file skip/read combo causing EIO error.
 *	Also, added new code based on 4.3BSD driver to correctly handle
 *	MT_UNREAD and MT_RDOPP interrupts (also involved minor change to
 *	MT_DONE data interrupt code).
 *
 * 07-Dec-87 - rsp
 *
 *	Fixed minor bug that showed up infrequently on semi-faulty
 *	formatter hardware involving a reset off a formatter that
 *	momentarily drops offline. Change is in mtndtint.
 *
 * 14-Feb-1989 - Tim Burke
 *
 *	Changed calls to DELAY(x) where the value of x is less than 10000 to
 *	be 100,000.  This is necessary because the granularity of the clock
 *	is 10000 as the smallest value (10 miliseconds).
 *
 * 05-Jun-89 - Tim Burke
 *	Added a new MTIOCTOP called MTFLUSH.  This is used on caching drives
 *	to force a flush of the controller's write back cache.  Since this 
 *	is not a caching drive the MTFLUSH always returns ENXIO.
 */

#include "mu.h"
#if NMT > 0 || defined(BINARY)

#include "../data/mt_data.c"

short	mttypes[] = { MBDT_TU78, 0 };
int	mtattach(),mtslave(),mtustart(),mtstart(),mtndtint(),mtdtint();

struct	mba_driver mtdriver = { mtattach, mtslave, mtustart, mtstart,
				mtdtint, mtndtint, mttypes, "mt",
				"mu", mtinfo };
mtattach(mi)
	register struct mba_device *mi;
{
}

mtslave(mi, ms, sn)
	register struct mba_device *mi;
	register struct mba_slave *ms;
	register int sn;
{
	register struct mtdevice *mtaddr = (struct mtdevice *)mi->mi_drv;
	register struct mu_softc *sc = &mu_softc[ms->ms_unit];
	register int s = spl7();
	int ner, ds;
	int rtn = 0;

	if (sn < 4) {

	    mtaddr->mtid = MTID_CLR;
	    DELAY(100000);
	    while ((mtaddr->mtid & MTID_RDY) == 0)
		    ;
	    mtaddr->mtas = -1;
	    mtaddr->mtncs[sn] = MT_SENSE|MT_GO;
	    DELAY(100000);
	    while (mtaddr->mtas == 0)
		    ;
	    ner = mtaddr->mtner;
	    ds = mtaddr->mtds;

	    if (((ner >> 8) & 0x3) == sn) {
		if (ds & MTDS_PRES) {
		    sc->sc_mi = mi;
		    sc->sc_slave = sn;
		    mutomt[ms->ms_unit] = mi->mi_unit;
		    rtn = 1;
		    sc->sc_softcnt = 0;
		    sc->sc_hardcnt = 0;
		    switch(*(mi->mi_driver->md_type + mi->mi_type)) {

		    case MBDT_TU78:
			bcopy(DEV_TU78,sc->sc_device, strlen(DEV_TU78));
			break;

		    default:
			bcopy(DEV_UNKNOWN,sc->sc_device, strlen(DEV_UNKNOWN));
			break;
		    }
		}
	    }

	    switch (ner & MTER_INTCODE) {
	    case MTER_DONE:
		break;

	    case MTER_TMFLTB:
	    case MTER_MBFLT:
	    case MTER_NOTAVL:
		if (!(ds & MTDS_ONL)) {
		    mtaddr->mtid = MTID_CLR;
		    DELAY(100000);
		    while ((mtaddr->mtid & MTID_RDY) == 0)
			    ;
		}
		if(ner & MTER_NOTAVL || ner & MTER_ONLINE)
		    break;
	    default:
		printf("mu%d: Unexpected Interupt 0%o Failure Code 0%o ds 0%o\n",
			((ner >> 8) & 0x3), (ner & MTER_INTCODE),
			((ner >> 10) & 0x3f), ds & 0xffff);
		break;
	    }

	    mtaddr->mtas = mtaddr->mtas;
	    DELAY(100000);
	}

	splx(s);
	return (rtn);
}

mtopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct mba_device *mi;
	register struct mu_softc *sc;
	register int unit = UNIT(dev);
	register int sel = SEL(dev);

	if (unit >= nNMU || (sc = &mu_softc[unit])->sc_openf ||
	    (mi = mtinfo[MTUNIT(dev)]) == 0 || mi->mi_alive == 0 ||
	    sc->sc_mi == 0 ) {
		return (ENXIO);
	}

	sc->sc_dens =
	    ((sel == MTHR) || (sel == MTHN)) ? MT_GCR : 0;

	sc->sc_flags = 0;

	mtcommand(dev, MT_SENSE, 1);

	if((sc->sc_dsreg & MTDS_AVAIL) == 0) {
		return(ENXIO);
	}

	if ((sc->sc_dsreg & MTDS_EOT) &&
	    (dis_eot_mu[unit] != DISEOT)) {
		sc->sc_flags = DEV_EOM;
	} else {
		sc->sc_flags = 0;
	}

	sc->sc_category_flags = 0;

	if((sel == MTHR) || (sel == MTHN)) {
		sc->sc_category_flags |= DEV_6250BPI;
	} else {
		sc->sc_category_flags |= DEV_1600BPI;
	}

	if (!(sc->sc_dsreg & MTDS_ONL)) {
		sc->sc_flags |= DEV_OFFLINE;
		if(!(flag & FNDELAY)) {
			DEV_UGH(sc->sc_device,unit,"offline");
			return(EIO);
		}
	}

	if (sc->sc_dsreg & MTDS_FPT) {
		sc->sc_flags |= DEV_WRTLCK;
	}
	if ((flag & FWRITE) && (sc->sc_dsreg & MTDS_FPT)) {
		if(!(flag & FNDELAY)) {
			DEV_UGH(sc->sc_device,unit,"write locked");
			return(EIO);
		}
	}

	sc->sc_openf = 1;
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	sc->sc_firsttime = 1;
	sc->sc_changedstate = 0;
	sc->sc_savestate = sc->sc_dsreg;
	return (0);
}

mtclose(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct mu_softc *sc = &mu_softc[UNIT(dev)];
	register int unit = UNIT(dev);
	register int sel = SEL(dev);

	sc->sc_flags &= ~DEV_EOM;

	if (flag == FWRITE || ((flag & FWRITE) && (sc->sc_flags &
	    DEV_WRITTEN))) {
		mtcommand(dev, MT_CLS|sc->sc_dens, 1);
		sc->sc_flags &= ~DEV_EOM;
	}

	if ((sel == MTLR) || (sel == MTHR)) {
		mtcommand(dev, MT_REW, 0);
	}

	sc->sc_openf = 0;

	if ((sc->sc_dsreg & MTDS_EOT) && (dis_eot_mu[unit] != DISEOT)){
		sc->sc_flags |= DEV_EOM;
	}
}

mtcommand(dev, com, count)
	register dev_t dev;
	register int com;
	register int count;
{
	register struct buf *bp = &cmtbuf[MTUNIT(dev)];
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
	mtstrategy(bp);

	if (count == 0)
		return;

	iowait(bp);

	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);

	bp->b_flags &= B_ERROR;
}

mtstrategy(bp)
	register struct buf *bp;
{
	register struct mba_device *mi = mtinfo[MTUNIT(bp->b_dev)];
	register struct mtdevice *mtaddr = (struct mtdevice *)mi->mi_drv;
	register struct mu_softc *sc = &mu_softc[UNIT(bp->b_dev)];
	register struct buf *dp;
	register int s;
	int unit = UNIT(bp->b_dev);

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_mu[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	if((bp->b_flags&B_READ) && (bp->b_flags&B_RAWASYNC) &&
	((sc->sc_category_flags&DEV_TPMARK)||(sc->sc_flags&DEV_HARDERR))) {
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

mtustart(mi)
	register struct mba_device *mi;
{
	register struct mtdevice *mtaddr = (struct mtdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct mu_softc *sc = &mu_softc[UNIT(bp->b_dev)];
	register int mtunit = MTUNIT(bp->b_dev);
	register daddr_t blkno;
	int unit = UNIT(bp->b_dev);

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_mu[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		return (MBU_NEXT);
	}

	if (((sc->sc_category_flags&DEV_TPMARK)||(sc->sc_flags&DEV_HARDERR))
	    && (bp->b_flags & B_READ) && (bp->b_flags&B_RAWASYNC)) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		return (MBU_NEXT);
	}

	sc->sc_flags &= ~DEV_WRITTEN;

	if (sc->sc_openf < 0) {
		bp->b_flags |= B_ERROR;
		return (MBU_NEXT);
	}

	if (bp != &cmtbuf[mtunit]) {
		if (sc->sc_blkno != bdbtofsb(bp->b_blkno) &&
			!mi->mi_tab.b_errcnt)
			sc->sc_blkno = bdbtofsb(bp->b_blkno);
	sc->sc_nxrec = bdbtofsb(bp->b_blkno) + 1;
	} else {
		mtaddr->mtncs[unit] =
			(bp->b_repcnt<<8)|bp->b_command|MT_GO;
		return (MBU_STARTED);
	}

	if ((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		if (mi->mi_tab.b_errcnt == 2) {
			mtaddr->mtca = unit;
		} else {
			mtaddr->mtbc = bp->b_bcount;
			mtaddr->mtca = (1<<2)|unit;
		}
		return (MBU_DODATA);
	}

	if (blkno < bdbtofsb(bp->b_blkno))
		mtaddr->mtncs[unit] =
		  (min((unsigned)(bdbtofsb(bp->b_blkno) - blkno),
		   0377) << 8) | MT_SFORW|MT_GO;
	else
		mtaddr->mtncs[unit] =
		  (min((unsigned)(blkno - bdbtofsb(bp->b_blkno)),
		   0377) << 8) | MT_SREV|MT_GO;

	return (MBU_STARTED);
}

mtstart(mi)
	register struct mba_device *mi;
{
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct mu_softc *sc = &mu_softc[UNIT(bp->b_dev)];

	if (bp->b_flags & B_READ)
		if (mi->mi_tab.b_errcnt == 2) {
			return(MT_READREV|MT_GO);
		} else {
			if(sc->sc_firsttime || sc->sc_changedstate) {
				sc->sc_changedstate = 0;
				sc->sc_firsttime = 0;
				sc->sc_savestate = sc->sc_dsreg;
				return(MT_READ|MT_SENSE|MT_GO);
			} else
				return(MT_READ|MT_GO);
		}
	else
		return(MT_WRITE|sc->sc_dens|MT_GO);
}

mtdtint(mi, mbsr)
	register struct mba_device *mi;
	int mbsr;
{
	register struct mtdevice *mtaddr = (struct mtdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct mu_softc *sc = &mu_softc[UNIT(bp->b_dev)];
	register int mtunit = MTUNIT(bp->b_dev);
	register int unit = UNIT(bp->b_dev);

	sc->sc_erreg = mtaddr->mter;

	if((bp->b_flags & B_READ) == 0)
		sc->sc_flags |= DEV_WRITTEN;

	switch (sc->sc_erreg & MTER_INTCODE) {

	case MTER_DONE:
	case MTER_LONGREC:
		sc->sc_flags |= DEV_DONE;
		if (mi->mi_tab.b_errcnt != 2)
			sc->sc_blkno++;
		if (mi->mi_tab.b_errcnt == 2) {
			bp->b_bcount = bp->b_resid;
			bp->b_resid -= MASKREG(mtaddr->mtbc);
			if ((bp->b_resid > 0) && (bp == &cmtbuf[mtunit]))
				bp->b_flags |= B_ERROR;
		} else {
			bp->b_resid = 0;
		}
		break;

	case MTER_NOTCAP:
		sc->sc_flags |= DEV_BLANK;
		goto err;

	case MTER_EOT:
		if (dis_eot_mu[unit] != DISEOT) {
			sc->sc_flags |= DEV_EOM;
			if (mi->mi_tab.b_errcnt != 2) {
				sc->sc_blkno++;
			}
			bp->b_resid = 0;
		}
		break;

	case MTER_TM:
		sc->sc_category_flags |= DEV_TPMARK;
		sc->sc_blkno++;
	err:
		bp->b_resid = bp->b_bcount;
		sc->sc_nxrec = bdbtofsb(bp->b_blkno);
		break;

	case MTER_SHRTREC:
		sc->sc_category_flags |= DEV_SHRTREC;
		sc->sc_blkno++;
		if (bp == &cmtbuf[mtunit])
			bp->b_flags |= B_ERROR;
		if (mi->mi_tab.b_errcnt == 2)
			bp->b_bcount = bp->b_resid;
		bp->b_resid = bp->b_bcount - mtaddr->mtbc;
		break;

	case MTER_RDOPP:
		sc->sc_flags |= DEV_DONE;
		sc->sc_category_flags |= DEV_RDOPP;
		mi->mi_tab.b_errcnt = 2;
		if ((bp->b_bcount = MASKREG(mtaddr->mtbc)) == 0)
			bp->b_bcount = 1;
		if (bp->b_bcount > bp->b_resid)
			bp->b_bcount = bp->b_resid;
		bp->b_bcount = -(bp->b_bcount);
		return(MBD_RETRY);

	case MTER_RETRY:
		sc->sc_flags |= DEV_RETRY;
		sc->sc_softcnt++;
		mi->mi_tab.b_errcnt = 1;
		return(MBD_RETRY);

	case MTER_OFFLINE: case MTER_NOTAVL:
		sc->sc_flags |= DEV_OFFLINE;
		if (sc->sc_openf > 0) {
			sc->sc_openf = -1;
		}
		bp->b_error |= ENXIO;
		bp->b_flags |= B_ERROR;
		sc->sc_flags |= DEV_DONE;
		return (MBD_DONE);

	case MTER_FPT:
		sc->sc_flags |= DEV_WRTLCK;
		bp->b_flags |= B_ERROR;
		break;

	case MTER_UNREAD:
		sc->sc_blkno++;
		bp->b_bcount = bp->b_resid;
		bp->b_resid -= MIN(MASKREG(mtaddr->mtbc), bp->b_bcount);

	default:
		sc->sc_flags |= DEV_HARDERR;
		sc->sc_hardcnt++;

		mprintf("%s: unit#:%d hard err blk#:%d mbsr:%b \
			mtcs:%x mter:%x mtca:%x mtmr1:%x mtas:%x \
			mtbc:%x mtdt:%x mtds:%x mtsn:%x mtmr2:%x \
			mtmr3:%x mtner:%x mtncs0:%x mtncs1:%x mtncs2:%x \
			mtncs3:%x mtia:%x mtid:%x\n",
			sc->sc_device, unit, bp->b_blkno, mbsr, mbsr_bits,
			MASKREG(mtaddr->mtcs),
			MASKREG(mtaddr->mter),
			MASKREG(mtaddr->mtca),
			MASKREG(mtaddr->mtmr1),
			MASKREG(mtaddr->mtas),
			MASKREG(mtaddr->mtbc),
			MASKREG(mtaddr->mtdt),
			MASKREG(mtaddr->mtds),
			MASKREG(mtaddr->mtsn),
			MASKREG(mtaddr->mtmr2),
			MASKREG(mtaddr->mtmr3),
			MASKREG(mtaddr->mtner),
			MASKREG(mtaddr->mtncs[0]),
			MASKREG(mtaddr->mtncs[1]),
			MASKREG(mtaddr->mtncs[2]),
			MASKREG(mtaddr->mtncs[3]),
			MASKREG(mtaddr->mtia),
			MASKREG(mtaddr->mtid));

		bp->b_flags |= B_ERROR;
		mtaddr->mtid = MTID_CLR;	/* reset the TM78 */
		DELAY(100000);
		while ((mtaddr->mtid & MTID_RDY) == 0)	/* wait for it */
			;
		break;
	}
	sc->sc_flags |= DEV_DONE;
	return (MBD_DONE);
}

mtndtint(mi)
	register struct mba_device *mi;
{
	register struct mtdevice *mtaddr = (struct mtdevice *)mi->mi_drv;
	register struct mu_softc *sc;
	register struct buf *bp = mi->mi_tab.b_actf;
	register int mtunit;
	register int unit;
	int er, ds, fc, ner;

	if (bp == 0) {
		ner = mtaddr->mtner;
		mtunit = MTUNIT((ner >> 8) & 0x3);
		unit = UNIT((ner >> 8) & 0x3);
		sc = &mu_softc[unit];
		ds = MASKREG(mtaddr->mtds);
		if((ds & MTDS_EOT) && (dis_eot_mu[unit] != DISEOT)) {
			sc->sc_dsreg = ds;
			sc->sc_flags |= DEV_EOM;
		}
		return (MBN_SKIP);	/* Just return if unexpected */
	}

	mtunit = MTUNIT(bp->b_dev);
	unit = UNIT(bp->b_dev);
	sc = &mu_softc[unit];
	sc->sc_erreg = er = MASKREG(mtaddr->mtner);
	sc->sc_dsreg = ds = MASKREG(mtaddr->mtds);
	sc->sc_resid = fc = (mtaddr->mtncs[unit] >> 8) & 0xff;

	if(sc->sc_savestate != sc->sc_dsreg) {
		sc->sc_changedstate = 1;
	}
	if((sc->sc_dsreg & MTDS_EOT) && (dis_eot_mu[unit] != DISEOT)) {
		sc->sc_flags |= DEV_EOM;
	}

	switch (er & MTER_INTCODE) {

	case MTER_DONE:
		if (bp == &cmtbuf[mtunit]) {
	done:
			if (bp->b_command == MT_SENSE)
				sc->sc_dsreg = MASKREG(mtaddr->mtds);
			bp->b_resid = fc;
			sc->sc_flags |= DEV_DONE;
			return (MBN_DONE);
		}
		if ((fc = bdbtofsb(bp->b_blkno) - sc->sc_blkno) < 0)
			sc->sc_blkno -= MIN(0377, -fc);
		else
			sc->sc_blkno += MIN(0377, fc);
		sc->sc_flags |= DEV_RETRY;
		return (MBN_RETRY);

	case MTER_RWDING:
		sc->sc_category_flags |= DEV_RWDING;
		return (MBN_SKIP);

	case MTER_NOTCAP:
		sc->sc_flags |= DEV_BLANK;
		return (MBN_SKIP);

	case MTER_EOT:
	case MTER_LEOT:
		if((ds & MTDS_EOT) && (dis_eot_mu[unit] != DISEOT)) {
			sc->sc_flags |= DEV_EOM;
		}
		sc->sc_flags |= DEV_DONE;
		return (MBN_DONE);

	case MTER_TM:
		sc->sc_category_flags |= DEV_TPMARK;
		if (sc->sc_blkno > bdbtofsb(bp->b_blkno)) {
			sc->sc_nxrec = bdbtofsb(bp->b_blkno) + fc;
			sc->sc_blkno = sc->sc_nxrec;
		} else {
			sc->sc_blkno = bdbtofsb(bp->b_blkno) - fc;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
		sc->sc_flags |= DEV_DONE;
		return (MBN_DONE);

	case MTER_FPT:
		sc->sc_flags |= DEV_WRTLCK;
		bp->b_flags |= B_ERROR;
		sc->sc_flags |= DEV_DONE;
		return (MBN_DONE);

	case MTER_OFFLINE: case MTER_NOTAVL:
		sc->sc_flags |= DEV_OFFLINE;
		if (sc->sc_openf > 0) {
			sc->sc_openf = -1;
		}
		bp->b_flags |= B_ERROR;
		sc->sc_flags |= DEV_DONE;
		return (MBN_DONE);

	case MTER_BOT:
		sc->sc_flags |= DEV_BOM;
		sc->sc_category_flags &= ~DEV_RWDING;
		if (bp == &cmtbuf[mtunit])
			goto done;

	case MTER_ONLINE:
                sc->sc_flags &= ~DEV_OFFLINE;
                sc->sc_flags |= DEV_DONE;
                return (MBN_DONE);

	default:
		sc->sc_flags |= DEV_HARDERR;
		sc->sc_hardcnt++;

		mprintf("%s: unit#:%d hard err blk#:%d \
			mtcs:%x mter:%x mtca:%x mtmr1:%x mtas:%x \
			mtbc:%x mtdt:%x mtds:%x mtsn:%x mtmr2:%x \
			mtmr3:%x mtner:%x mtncs0:%x mtncs1:%x mtncs2:%x \
			mtncs3:%x mtia:%x mtid:%x\n",
			sc->sc_device, unit, bp->b_blkno,
			MASKREG(mtaddr->mtcs),
			MASKREG(mtaddr->mter),
			MASKREG(mtaddr->mtca),
			MASKREG(mtaddr->mtmr1),
			MASKREG(mtaddr->mtas),
			MASKREG(mtaddr->mtbc),
			MASKREG(mtaddr->mtdt),
			MASKREG(mtaddr->mtds),
			MASKREG(mtaddr->mtsn),
			MASKREG(mtaddr->mtmr2),
			MASKREG(mtaddr->mtmr3),
			MASKREG(mtaddr->mtner),
			MASKREG(mtaddr->mtncs[0]),
			MASKREG(mtaddr->mtncs[1]),
			MASKREG(mtaddr->mtncs[2]),
			MASKREG(mtaddr->mtncs[3]),
			MASKREG(mtaddr->mtia),
			MASKREG(mtaddr->mtid));

		mtaddr->mtid = MTID_CLR;	/* reset the TM78 */
		DELAY(100000);
		while ((mtaddr->mtid & MTID_RDY) == 0)	/* wait for it */
			;
		bp->b_flags |= B_ERROR;
		sc->sc_flags |= DEV_DONE;
		return (MBN_DONE);
	}
}

mtread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int mtunit = MTUNIT(dev);

	return (physio(mtstrategy, &rmtbuf[mtunit], dev, B_READ,
		minphys, uio));
}

mtwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int mtunit = MTUNIT(dev);

	return (physio(mtstrategy, &rmtbuf[mtunit], dev, B_WRITE,
		minphys, uio));
}

mtioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct mba_device *mi = mtinfo[MTUNIT(dev)];
	register struct mu_softc *sc = &mu_softc[UNIT(dev)];
	register struct buf *bp = &cmtbuf[MTUNIT(dev)];
	register callcount;
	register int op;
	struct mtop *mtop;
	struct mtget *mtget;
	struct devget *devget;
	int unit = UNIT(dev);
	int fcount;

	/* we depend of the values and order of the MT codes here */
	static mtops[] = { MT_WTM, MT_SFLEOT, MT_SREVF, MT_SFORW,
			   MT_SREV, MT_REW, MT_UNLOAD, MT_SENSE };

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
			fcount = 1;
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
			dis_eot_mu[unit] = 0;
			return(0);

		case MTDISEOT:
			dis_eot_mu[unit] = DISEOT;
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

		op = mtops[mtop->mt_op];

		if (op == MT_WTM)
			op |= sc->sc_dens;

		while (--callcount >= 0) {
			register int n;

			do {
				n = MIN(fcount, 0xff);
				mtcommand(dev, op, n);
				fcount -= n;
			} while (fcount);

			if ((mtop->mt_op == MTFSR || mtop->mt_op ==
			    MTBSR) && bp->b_resid)
				return (EIO);

			if (bp->b_flags & B_ERROR)
				break;
		}
		return (geterror(bp));

	case MTIOCGET:				/* tape status */
		mtget = (struct mtget *)data;
		mtget->mt_erreg = sc->sc_erreg;
		mtget->mt_resid = sc->sc_resid;
		mtcommand(dev, MT_SENSE, 1);
		mtget->mt_dsreg = sc->sc_dsreg;
		mtget->mt_type = MT_ISMT;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TAPE;
		devget->bus = DEV_MB;
		bcopy(DEV_TM78,devget->interface,
		      strlen(DEV_TM78));
		bcopy(sc->sc_device,devget->device,
		      strlen(sc->sc_device));
		devget->adpt_num = mi->mi_adpt; 	/* which adapter*/
		devget->nexus_num = mi->mi_nexus;	/* which nexus	*/
		devget->bus_num = mi->mi_mbanum;	/* which MBA	*/
		devget->ctlr_num = mi->mi_drive;	/* which TM78	*/
		devget->slave_num = sc->sc_slave;	/* which plug	*/
		bcopy(mi->mi_driver->md_sname,
		      devget->dev_name,
		      strlen(mi->mi_driver->md_sname)); /* Ult. "mu"	*/
		devget->unit_num = unit;		/* which mu??	*/
		mtcommand(dev, MT_SENSE, 1); /* TU78 hack */
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

mtdump()
{
	register struct mba_device *mi;
	register struct mba_regs *mp;
	register struct mtdevice *mtaddr;
	register int blk;
	register int num;
	register int start;

	start = 0;
	num = maxfree;

	if (mtinfo[0] == 0)
		return (ENXIO);

	mi = PHYS(mtinfo[0], struct mba_device *);
	mp = PHYS(mi->mi_hd, struct mba_hd *)->mh_physmba;
	mp->mba_cr = MBCR_IE;
	mtaddr = (struct mtdevice *)&mp->mba_drv[mi->mi_drive];
	mtaddr->mtcs = MTID_CLR|MT_GO;

	while (num > 0) {
		blk = num > DBSIZE ? DBSIZE : num;
		mtdwrite(start, blk, mtaddr, mp);
		start += blk;
		num -= blk;
	}

	mteof(mtaddr);
	mteof(mtaddr);
	mtwait(mtaddr);
	mtaddr->mtcs = MT_REW|MT_GO;
	return (0);
}

mtdwrite(dbuf, num, mtaddr, mp)
	register int dbuf;
	register int num;
	register struct mtdevice *mtaddr;
	register struct mba_regs *mp;
{
	register struct pte *io;
	register int i;

	mtwait(mtaddr);
	io = mp->mba_map;

	for (i = 0; i < num; i++)
		*(int *)io++ = dbuf++ | PG_V;

	mtaddr->mtbc = -(num*NBPG);
	mp->mba_sr = -1;
	mp->mba_bcr = -(num*NBPG);
	mp->mba_var = 0;
	mtaddr->mtcs = MT_WRITE|MT_GCR|MT_GO;
}

mtwait(mtaddr)
	register struct mtdevice *mtaddr;
{
	register int s;

	do
		s = mtaddr->mtds;
	while ((s & MTDS_RDY) == 0);
}

mteof(mtaddr)
	register struct mtdevice *mtaddr;
{

	mtwait(mtaddr);
	mtaddr->mtcs = MT_CLS|MT_GCR|MT_GO;
}
#endif
