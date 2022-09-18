
#ifndef lint
static char *sccsid = "@(#)idc.c	4.1	ULTRIX	7/2/90";
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
 * idc.c    6.1     07/29/83
 *
 * Modification history
 *
 * IDC/RL02/R80 disk driver
 *
 * 26-July-89 - Alan Frechette
 *	Conditionalize out the dump code.
 *
 * 22-Feb-84 - tresvik
 *
 *	Derived from 4.2BSD labeled: idc.c	 6.1	 83/07/29.
 *	Changed printf to mprintf for hard and soft error reporting.
 *
 * 29-Oct-84 - reilly
 *
 *	Added code for the disk partitioning scheme. -001
 *
 *  2-Nov-84 - tresvik
 *
 *	Changed std address from 0174400 to 0175606, where it really is.
 *	This will prevent machine checks on 780's with RL11 controllers
 *	on the bus.
 *
 * 30-Nov-84 - reilly
 *
 *	Fixed up an error message. -001
 *
 * 24-Sep-85 - reilly
 *
 *	Added new ioctl request that will return the default partition
 *	table.
 *
 * 18-Mar-86 - jaw
 *
 *	br/cvec changed to NOT use registers.
 *
 *  9-Apr-86 - prs
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
 * 25-Sep-86 - pmk
 *
 *	Added code to detect OPI errors and drive not ready, because
 *	of lost interrupt hangs.
 *
 * 16-Oct-86 - pmk
 *
 *	Added code to detect drive not ready in the open routine to set
 *	drive sc_flags to offline for devioctl.
 */

#include "rb.h"
#if NIDC > 0 || defined(BINARY)

#ifdef IDCDEBUG
int	idcdebug = 0;
#define printd if(idcdebug)printf
int	idctrb[1000];
int	*trp = idctrb;
#define trace(a,b) {*trp++ = *(int*)a; *trp++ = (int)b; if(trp>&idctrb[998])trp=idctrb;}
#endif IDCDEBUG

#include "../h/dump.h"
#include "../data/idc_data.c"

#define dar_dar 	dar_l		/* the whole disk address */
#define dar_cyl 	dar_w[1]	/* cylinder address */
#define dar_trk 	dar_b[1]	/* track */
#define dar_sect	dar_b[0]	/* sector */
#define sc_dar		sc_un.dar_dar
#define sc_cyl		sc_un.dar_cyl
#define sc_trk		sc_un.dar_trk
#define sc_sect 	sc_un.dar_sect

int	idcprobe(), idcslave(), idcattach(), idcdgo(), idcintr();

u_short idcstd[] = { 0175606 };
struct	uba_driver idcdriver =
 { idcprobe, idcslave, idcattach, idcdgo, idcstd, "rb", idcdinfo, "idc", idcminfo, 0 };
struct	idcst {
	short	nbps;
	short	nsect;
	short	ntrak;
	short	nspc;
	short	ncyl;
	struct	size *sizes;
} idcst[] = {
	256, NRB02SECT, NRB02TRK, NRB02SECT*NRB02TRK, NRB02CYL, rb02_sizes,
	512, NRB80SECT, NRB80TRK, NRB80SECT*NRB80TRK, NRB80CYL, rb80_sizes,
};

#define b_cylin b_resid

#ifdef INTRLVE
daddr_t dkblock();
#endif INTRLVE

int	idcwstart, idcwticks, idcwatch();

/*ARGSUSED*/
idcprobe(reg)
	caddr_t reg;
{
	register struct idcdevice *idcaddr;

	idcaddr = (struct idcdevice *)((caddr_t)uba_hd[0].uh_uba + 0x200);
	idcaddr->idccsr = IDC_ATTN|IDC_IE;
	while ((idcaddr->idccsr & IDC_CRDY) == 0)
		;		/* needs to timeout usin todr */
				/* also should do a badaddr because */
				/* we aren't using reg Why aren't we?*/
	idcaddr->idccsr = IDC_ATTN|IDC_CRDY;
	return (sizeof (struct idcdevice));
}

/*ARGSUSED*/
idcslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{
	register struct idcdevice *idcaddr;
	register struct idc_softc *sc = &idc_softc;

	idcaddr = (struct idcdevice *)((caddr_t)uba_hd[0].uh_uba + 0x200);
	ui->ui_type = 0;
	/* set drive to be offline until the first access occurs */
	sc->sc_offline[ui->ui_unit] = 1;
	/* clear any attention bit */
	idcaddr->idccsr = IDC_CRDY|(1<<(ui->ui_slave+16));
	(void) idcwait(idcaddr, 0);
	/* setup for and get status of drive to see if it is there. */
	/* this also resets the drive */
	idcaddr->idcmpr = IDCGS_GETSTAT;
	idcaddr->idccsr = IDC_GETSTAT|(ui->ui_slave<<8);
	(void) idcwait(idcaddr, 0);
	/*
	 * OPI means the drive really isn't there
	 */
	if (idcaddr->idccsr & IDC_OPI)
		return(0);
	/*
	 * Read header to synchronize microcode.
	 * This is accomplished by issuing a READ HEADER command to
	 * the selected unit followed by two reads of the MPR register.
	 */
	idcaddr->idccsr = (ui->ui_slave<<8)|IDC_RHDR;
	(void) idcwait(idcaddr, 0);
	if (idcaddr->idcmpr == idcaddr->idcmpr); /* reads the MPR twice */
	if (idcaddr->idccsr&IDC_R80)
		ui->ui_type = 1;
	return (1);
}

idcattach(ui)
	register struct uba_device *ui;
{
	register struct idc_softc *sc = &idc_softc;

	/*
	 * Fix all addresses to correspond
	 * to the "real" IDC address.
	 */
	ui->ui_mi->um_addr = ui->ui_addr = (caddr_t)uba_hd[0].uh_uba + 0x200;
	ui->ui_physaddr = (caddr_t)uba_hd[0].uh_physuba + 0x200;
	if (idcwstart == 0) {
		timeout(idcwatch, (caddr_t)0, hz);
		idcwstart++;
	}
	if (ui->ui_dk >= 0) {
		if (ui->ui_type) {
			dk_mspw[ui->ui_dk] = 1.0 / (60 * NRB80SECT * 256);
			bcopy(DEV_R80,sc->sc_device[ui->ui_unit],
			      strlen(DEV_R80));
		} else {
			dk_mspw[ui->ui_dk] = 1.0 / (60 * NRB02SECT * 128);
			bcopy(DEV_RL02,sc->sc_device[ui->ui_unit],
			      strlen(DEV_RL02));
		}
		sc->sc_softcnt[ui->ui_unit] = 0;
		sc->sc_hardcnt[ui->ui_unit] = 0;
	}
	idccyl[ui->ui_unit].dar_dar = -1;
	ui->ui_flags = 0;
}

idcopen(dev, flag)
	register dev_t dev;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui;
	register struct idc_softc *sc;
	register struct idcdevice *idcaddr;
	int savstat, savcsr;

	if (unit >= nNRB || (ui = idcdinfo[unit]) == 0 ||
	    ui->ui_alive == 0) {
		return (ENXIO);
	}

	idcaddr = (struct idcdevice *)ui->ui_addr;
	sc = &idc_softc;
	sc->sc_flags[unit] = 0;
	sc->sc_category_flags[unit] = 0;

	idcaddr->idcmpr = IDCGS_GETSTAT;
	idcaddr->idccsr = IDC_GETSTAT|(ui->ui_slave<<8);
	(void) idcwait(idcaddr, 0);
	savstat = idcaddr->idcmpr;
	savcsr = idcaddr->idccsr;
	if (savstat & IDCDS_WL) {
		sc->sc_flags[unit] |= DEV_WRTLCK;
	}
	if (!(savcsr & IDC_DRDY)) {
		sc->sc_flags[unit] |= DEV_OFFLINE;
	}
	if (savcsr & IDC_ERR) {
		mprintf("%s: unit# %d: drive error: csr=%b ds=%b\n",
		    sc->sc_device[unit], unit, savcsr, IDCCSR_BITS, savstat,
		    ui->ui_type?IDCRB80DS_BITS:IDCRB02DS_BITS);
	}

	idcaddr->idccsr = IDC_IE|IDC_CRDY|(1<<(ui->ui_slave+16));

	/*
	 *	We only need to read the partition table if the volume is
	 *	not valid or the partition table is invalid.
	 */

	if ((sc->sc_flags[unit] & DEV_OFFLINE) ||
	    (idc_part[unit].pt_valid != PT_VALID)){
		int nspc = idcst[ui->ui_type].nspc;
		int i, idcstrategy();		       /* 001 */

		for( i = 0; i <= 7; i++ ) {
			idc_part[unit].pt_part[i].pi_nblocks =
				idcst[ui->ui_type].sizes[i].nblocks;
			idc_part[unit].pt_part[i].pi_blkoff =
				idcst[ui->ui_type].sizes[i].cyloff * nspc;
		}

		idc_part[unit].pt_valid = PT_VALID;	/*001 Validate the pt*/

		/*
		 *	Default partition are now set. Call rsblk to set
		 *	the driver's partition tables, if any exists, from
		 *	the "a" partition superblock
		 */

		rsblk( idcstrategy, dev, &idc_part[unit] );
	}
	return (0);
}

idcstrategy(bp)
	register struct buf *bp;
{
	register struct uba_device *ui;
	register struct idcst *st;
	register int unit = dkunit(bp);
	register struct pt *pt; 			/* 001 */
	register struct idc_softc *sc = &idc_softc;
	struct buf *dp;
	int xunit = minor(bp->b_dev) & 07;
	long bn, sz;

	sz = (bp->b_bcount+511) >> 9;
	if (unit >= nNRB)
		goto bad;
	ui = idcdinfo[unit];
	if (ui == 0 || ui->ui_alive == 0)
		goto bad;
	st = &idcst[ui->ui_type];
	pt = &idc_part[unit];
	if ( pt->pt_valid != PT_VALID ) 			/* 001 */
		panic("idcstrategy: invalid partition table");	/* 001 */
	if (bp->b_blkno < 0 ||
	    (bn = dkblock(bp))+sz > pt->pt_part[xunit].pi_nblocks) {
		sc->sc_flags[unit] |= DEV_EOM;
		goto bad;
	}
	if (ui->ui_type == 0)
		bn *= 2;
	bp->b_cylin = bn/st->nspc + pt->pt_part[xunit].pi_blkoff / st->nspc; /*001 */
	(void) spl5();
#ifdef IDCDEBUG
	trace("strt",bp);
#endif IDCDEBUG
	dp = &idcutab[ui->ui_unit];
	disksort(dp, bp);
	if (dp->b_active == 0) {
#ifdef IDCDEBUG
		trace("!act",dp);
#endif IDCDEBUG
		(void) idcustart(ui);
		bp = &ui->ui_mi->um_tab;
		if (bp->b_actf && bp->b_active == 0)
			(void) idcstart(ui->ui_mi);
	}
	(void) spl0();
	return;

bad:
	bp->b_flags |= B_ERROR;
	if (sc->sc_flags[unit] & DEV_EOM) {
		bp->b_error = ENOSPC;
	}
	iodone(bp);
	return;
}

idcustart(ui)
	register struct uba_device *ui;
{
	register struct idcdevice *idcaddr;
	register struct buf *bp;
	register struct uba_ctlr *um;
	register struct idcst *st;
	register struct idc_softc *sc;
	struct buf *dp;
	union idc_dar cyltrk;
	daddr_t bn;
	int unit;

	if (ui == 0)
		return (0);
	dk_busy &= ~(1<<ui->ui_dk);
	dp = &idcutab[ui->ui_unit];
	um = ui->ui_mi;
	unit = ui->ui_slave;
	sc = &idc_softc;
#ifdef IDCDEBUG
	trace("ust", dp);
#endif IDCDEBUG
	idcaddr = (struct idcdevice *)um->um_addr;
	if (um->um_tab.b_active) {
		sc->sc_softas |= 1<<unit;
#ifdef IDCDEBUG
		trace("umac",sc->sc_softas);
#endif IDCDEBUG
		return (0);
	}
	if ((bp = dp->b_actf) == NULL) {
#ifdef IDCDEBUG
		trace("!bp",0);
#endif IDCDEBUG
		return (0);
	}
	if (dp->b_active) {
#ifdef IDCDEBUG
		trace("dpac",dp->b_active);
#endif IDCDEBUG
		goto done;
	}
	dp->b_active = 1;
	/* CHECK DRIVE READY? */
	bn = dkblock(bp);
#ifdef IDCDEBUG
	trace("seek", bn);
#endif IDCDEBUG
	if (ui->ui_type == 0)
		bn *= 2;
	st = &idcst[ui->ui_type];
	cyltrk.dar_cyl = bp->b_cylin;
	cyltrk.dar_trk = (bn / st->nsect) % st->ntrak;
	cyltrk.dar_sect = 0;
#ifdef IDCDEBUG
	printd("idcustart, unit %d, cyltrk 0x%x\n", unit, cyltrk.dar_dar);
#endif IDCDEBUG
	/*
	 * If on cylinder, no need to seek.
	 */
	if (cyltrk.dar_dar == idccyl[ui->ui_unit].dar_dar)
		goto done;
	/*
	 * RB80 can change heads (tracks) just by loading
	 * the disk address register, perform optimization
	 * here instead of doing a full seek.
	 */
	if (ui->ui_type && cyltrk.dar_cyl == idccyl[ui->ui_unit].dar_cyl) {
		idcaddr->idccsr = IDC_CRDY|IDC_IE|IDC_SEEK|(unit<<8);
		idcaddr->idcdar = cyltrk.dar_dar;
		idccyl[ui->ui_unit].dar_dar = cyltrk.dar_dar;
		goto done;
	}
	/*
	 * Need to do a full seek.  Select the unit, clear
	 * its attention bit, set the command, load the
	 * disk address register, and then go.
	 */
	idcaddr->idccsr =
	    IDC_CRDY|IDC_IE|IDC_SEEK|(unit<<8)|(1<<(unit+16));
	idcaddr->idcdar = cyltrk.dar_dar;
	idccyl[ui->ui_unit].dar_dar = cyltrk.dar_dar;
#ifdef IDCDEBUG
	printd("  seek");
#endif IDCDEBUG
	idcaddr->idccsr = IDC_IE|IDC_SEEK|(unit<<8);
	if (ui->ui_dk >= 0) {
		dk_busy |= 1<<ui->ui_dk;
		dk_seek[ui->ui_dk]++;
	}
	/*
	 * RB80's initiate seeks very quickly.	Wait for it
	 * to come ready rather than taking the interrupt.
	 */
	if (ui->ui_type) {
		if (idcwait(idcaddr, 10) == 0)
			return (1);
		idcaddr->idccsr &= ~IDC_ATTN;
		/* has the seek completed? */
		if (idcaddr->idccsr & IDC_DRDY) {
#ifdef IDCDEBUG
			printd(", drdy");
#endif IDCDEBUG
			idcaddr->idccsr =
			    IDC_CRDY|IDC_IE|IDC_SEEK|(unit<<8)|(1<<(unit+16));
			goto done;
		}
	}
#ifdef IDCDEBUG
	printd(", idccsr = 0x%x\n", idcaddr->idccsr);
#endif IDCDEBUG
	return (1);
done:
	if (dp->b_active != 2) {
#ifdef IDCDEBUG
		trace("!=2",dp->b_active);
#endif IDCDEBUG
		dp->b_forw = NULL;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else {
#ifdef IDCDEBUG
			trace("!NUL",um->um_tab.b_actl);
#endif IDCDEBUG
			um->um_tab.b_actl->b_forw = dp;
		}
		um->um_tab.b_actl = dp;
		dp->b_active = 2;
	}
	sc->sc_flags[ui->ui_unit] |= DEV_DONE;
	return (0);
}

idcstart(um)
	register struct uba_ctlr *um;
{
	register struct buf *bp, *dp;
	register struct uba_device *ui;
	register struct idcdevice *idcaddr;
	register struct idc_softc *sc;
	struct idcst *st;
	daddr_t bn;
	int sn, tn, cmd;

loop:
	if ((dp = um->um_tab.b_actf) == NULL) {
#ifdef IDCDEBUG
		trace("nodp",um);
#endif IDCDEBUG
		return (0);
	}
	if ((bp = dp->b_actf) == NULL) {
#ifdef IDCDEBUG
		trace("nobp", dp);
#endif IDCDEBUG
		um->um_tab.b_actf = dp->b_forw;
		goto loop;
	}
	um->um_tab.b_active = 1;
	ui = idcdinfo[dkunit(bp)];
	bn = dkblock(bp);
#ifdef IDCDEBUG
	trace("star",bp);
#endif IDCDEBUG
	if (ui->ui_type == 0)
		bn *= 2;
	sc = &idc_softc;
	st = &idcst[ui->ui_type];
	sn = bn%st->nspc;
	tn = sn/st->nsect;
	sn %= st->nsect;
	sc->sc_sect = sn;
	sc->sc_trk = tn;
	sc->sc_cyl = bp->b_cylin;
	idcaddr = (struct idcdevice *)ui->ui_addr;
#ifdef IDCDEBUG
	printd("idcstart, unit %d, dar 0x%x", ui->ui_slave, sc->sc_dar);
#endif IDCDEBUG
	if (bp->b_flags & B_READ)
		cmd = IDC_IE|IDC_READ|(ui->ui_slave<<8);
	else
		cmd = IDC_IE|IDC_WRITE|(ui->ui_slave<<8);
	idcaddr->idccsr = IDC_CRDY|cmd;
	if ((idcaddr->idccsr&IDC_DRDY) == 0) {
		mprintf("rb%d: not ready\n", dkunit(bp));
		um->um_tab.b_active = 0;
		um->um_tab.b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		dp->b_active = 0;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		goto loop;
	}
	idccyl[ui->ui_unit].dar_dar = sc->sc_dar;
	idccyl[ui->ui_unit].dar_sect = 0;
	sn = (st->nsect - sn) * st->nbps;
	if (sn > bp->b_bcount)
		sn = bp->b_bcount;
	sc->sc_bcnt = sn;
	sc->sc_resid = bp->b_bcount;
	sc->sc_unit = ui->ui_slave;
#ifdef IDCDEBUG
	printd(", bcr 0x%x, cmd 0x%x\n", sn, cmd);
#endif IDCDEBUG
	um->um_cmd = cmd;
	(void) ubago(ui);
	return (1);
}

idcdgo(um)
	register struct uba_ctlr *um;
{
	register struct buf *bp;
	register struct uba_device *ui = idcdinfo[dkunit(bp)];
	register struct idcdevice *idcaddr = (struct idcdevice *)um->um_addr;
	register struct idc_softc *sc = &idc_softc;

	/*
	 * VERY IMPORTANT: must load registers in this order.
	 */
	idcaddr->idcbar = sc->sc_ubaddr = um->um_ubinfo&0x3ffff;
	idcaddr->idcbcr = -sc->sc_bcnt;
	idcaddr->idcdar = sc->sc_dar;


#ifdef IDCDEBUG
	printd("idcdgo, ubinfo 0x%x, cmd 0x%x\n", um->um_ubinfo, um->um_cmd);
#endif IDCDEBUG
	idcaddr->idccsr = um->um_cmd;
#ifdef IDCDEBUG
	trace("go", um);
#endif IDCDEBUG
	um->um_tab.b_active = 2;
	/*** CLEAR SPURIOUS ATTN ON R80? ***/
}

idcintr(idc)
	int idc;
{
	register struct uba_ctlr *um = idcminfo[idc];
	register struct uba_device *ui;
	register struct idcdevice *idcaddr = (struct idcdevice *)um->um_addr;
	register struct idc_softc *sc = &idc_softc;
	register struct buf *bp, *dp;
	struct idcst *st;
	int unit, as, er, cmd, savcsr, ds = 0;

	savcsr = idcaddr->idccsr;
#ifdef IDCDEBUG
	printd("idcintr, idccsr 0x%x", idcaddr->idccsr);
#endif IDCDEBUG
top:
	idcwticks = 0;
#ifdef IDCDEBUG
	trace("intr", um->um_tab.b_active);
#endif IDCDEBUG
	if (um->um_tab.b_active == 2) {
		/*
		 * Process a data transfer complete interrupt.
		 */
		um->um_tab.b_active = 1;
		dp = um->um_tab.b_actf;
		bp = dp->b_actf;
		ui = idcdinfo[dkunit(bp)];
		unit = ui->ui_slave;
		st = &idcst[ui->ui_type];
		idcaddr->idccsr = IDC_IE|IDC_CRDY|(unit<<8);
		if ((er = idcaddr->idccsr) & IDC_ERR) {
			if (er & IDC_DE) {
				idcaddr->idcmpr = IDCGS_GETSTAT;
				idcaddr->idccsr = IDC_GETSTAT|(unit<<8);
				(void) idcwait(idcaddr, 0);
				ds = idcaddr->idcmpr;
				idcaddr->idccsr =
				    IDC_IE|IDC_CRDY|(1<<(unit+16));
			}
#ifdef IDCDEBUG
			printd(", er 0x%x, ds 0x%x", er, ds);
#endif IDCDEBUG
			if (ds & IDCDS_WL) {
				sc->sc_flags[unit] |= DEV_WRTLCK;
				bp->b_flags |= B_ERROR;
			} else if (++um->um_tab.b_errcnt > 28 || er&IDC_HARD) {
hard:
				harderr(bp, "rb");
				sc->sc_hardcnt[unit]++;
				sc->sc_flags[unit] |= DEV_HARDERR;
		mprintf("%s: unit# %d: hard error block# %d\n csr=%b ds=%b\n",
		    sc->sc_device[unit], ui->ui_unit, bp->b_blkno, er,
		    IDCCSR_BITS, ds, ui->ui_type?IDCRB80DS_BITS:IDCRB02DS_BITS);
				bp->b_flags |= B_ERROR;
			} else if (er & IDC_DCK) {
				switch (er & IDC_ECS) {
				case IDC_ECS_NONE:
					break;
				case IDC_ECS_SOFT:
					idcecc(ui);
					sc->sc_softcnt[unit]++;
					sc->sc_flags[unit] |= DEV_SOFTERR;
					break;
				case IDC_ECS_HARD:
				default:
					goto hard;
				}
			} else
				/* recoverable error, set up for retry */
				goto seek;
		}
		if ((sc->sc_resid -= sc->sc_bcnt) != 0) {
			sc->sc_ubaddr += sc->sc_bcnt;
			/*
			 * Current transfer is complete, have
			 * we overflowed to the next track?
			 */
			if ((sc->sc_sect += sc->sc_bcnt/st->nbps) == st->nsect) {
				sc->sc_sect = 0;
				if (++sc->sc_trk == st->ntrak) {
					sc->sc_trk = 0;
					sc->sc_cyl++;
				} else if (ui->ui_type) {
					/*
					 * RB80 can change heads just by
					 * loading the disk address register.
					 */
					idcaddr->idccsr = IDC_SEEK|IDC_CRDY|
					    IDC_IE|(unit<<8);
#ifdef IDCDEBUG
					printd(", change to track 0x%x", sc->sc_dar);
#endif IDCDEBUG
					idcaddr->idcdar = sc->sc_dar;
					idccyl[ui->ui_unit].dar_dar = sc->sc_dar;
					idccyl[ui->ui_unit].dar_sect = 0;
					goto cont;
				}
				/*
				 * Changing tracks on RB02 or cylinders
				 * on RB80, start a seek.
				 */
seek:
				cmd = IDC_IE|IDC_SEEK|(unit<<8);
				idcaddr->idccsr = cmd|IDC_CRDY;
				idcaddr->idcdar = sc->sc_dar;
#ifdef IDCDEBUG
				printd(", seek to 0x%x\n", sc->sc_dar);
#endif IDCDEBUG
				idccyl[ui->ui_unit].dar_dar = sc->sc_dar;
				idccyl[ui->ui_unit].dar_sect = 0;
				sc->sc_bcnt = 0;
				idcaddr->idccsr = cmd;
				if (ui->ui_type) {
					if (idcwait(idcaddr, 10) == 0)
						return;
					idcaddr->idccsr &= ~IDC_ATTN;
					if (idcaddr->idccsr & IDC_DRDY)
						goto top;
				}
			} else {
				/*
				 * Continue transfer on current track.
				 */
cont:
				sc->sc_bcnt = (st->nsect-sc->sc_sect)*st->nbps;
				if (sc->sc_bcnt > sc->sc_resid)
					sc->sc_bcnt = sc->sc_resid;
				if (bp->b_flags & B_READ)
					cmd = IDC_IE|IDC_READ|(unit<<8);
				else
					cmd = IDC_IE|IDC_WRITE|(unit<<8);
				idcaddr->idccsr = cmd|IDC_CRDY;
				idcaddr->idcbar = sc->sc_ubaddr;
				idcaddr->idcbcr = -sc->sc_bcnt;
				idcaddr->idcdar = sc->sc_dar;
#ifdef IDCDEBUG
				printd(", continue I/O 0x%x, 0x%x\n", sc->sc_dar, sc->sc_bcnt);
#endif IDCDEBUG
				idcaddr->idccsr = cmd;
				um->um_tab.b_active = 2;
			}
			return;
		}
		/*
		 * Entire transfer is done, clean up.
		 */
		ubadone(um);
		dk_busy &= ~(1 << ui->ui_dk);
		um->um_tab.b_active = 0;
		um->um_tab.b_errcnt = 0;
		um->um_tab.b_actf = dp->b_forw;
		dp->b_active = 0;
		dp->b_errcnt = 0;
		dp->b_actf = bp->av_forw;
#ifdef IDCDEBUG
		trace("done", dp); trace(&um->um_tab.b_actf, dp->b_actf);
#endif IDCDEBUG
		bp->b_resid = sc->sc_resid;
#ifdef IDCDEBUG
		printd(", iodone, resid 0x%x\n", bp->b_resid);
#endif IDCDEBUG
		iodone(bp);
		if (dp->b_actf)
			if (idcustart(ui))
				return;
	} else if (um->um_tab.b_active == 1) {
		/*
		 * Got an interrupt while setting up for a command
		 * or doing a mid-transfer seek.  Save any attentions
		 * for later and process a mid-transfer seek complete.
		 */
		as = idcaddr->idccsr;
		idcaddr->idccsr = IDC_IE|IDC_CRDY|(as&IDC_ATTN);
		as = (as >> 16) & 0xf;
		dp = um->um_tab.b_actf;
		bp = dp->b_actf;
		ui = idcdinfo[dkunit(bp)];
		unit = sc->sc_unit;
		sc->sc_softas |= as & ~(1<<unit);
		if (as & (1<<unit)) {
#ifdef IDCDEBUG
			printd(", seek1 complete");
#endif IDCDEBUG
			um->um_tab.b_active = 2;
			goto top;
		}
#ifdef IDCDEBUG
		printd(", as1 %o\n", as);
#endif IDCDEBUG
		return;
	}
	/*
	 * Process any seek initiated or complete interrupts.
	 */
	if ((savcsr & IDC_OPI) && !(savcsr & IDC_DRDY)) {
	    unit = ((savcsr & IDC_DS) >> 8);
	    ui = idcdinfo[unit];
	    if (sc->sc_offline[ui->ui_unit]) {
		mprintf("%s: unit# %d: offline\n",sc->sc_device[unit], unit);
	    }
	    else {
		mprintf("%s: unit# %d: drive error: csr=%b\n",
		    sc->sc_device[unit], unit, savcsr, IDCCSR_BITS);
	    }
	    dp = &idcutab[ui->ui_unit];
	    bp = dp->b_actf;
	    dp->b_actf = bp->av_forw;
	    dp->b_active = 0;
	    bp->b_flags |= B_ERROR;
	    iodone(bp);
	    if (idcustart(ui)) {
	        return;
	    }
	}
	as = idcaddr->idccsr;
	idcaddr->idccsr = IDC_IE|IDC_CRDY|(as&IDC_ATTN);
	as = ((as >> 16) & 0xf) | sc->sc_softas;
	sc->sc_softas = 0;
#ifdef IDCDEBUG
	trace("as", as);
	printd(", as %o", as);
#endif IDCDEBUG
	for (unit = 0; unit < nNRB; unit++)
		if (as & (1<<unit)) {
			as &= ~(1<<unit);
			idcaddr->idccsr = IDC_IE|IDC_CRDY|(unit<<8);
			ui = idcdinfo[unit];
			if (ui) {
#ifdef IDCDEBUG
				printd(", attn unit %d", unit);
#endif IDCDEBUG
				if (idcaddr->idccsr & IDC_DRDY) {
					if (sc->sc_offline[ui->ui_unit]) {
						/*
						 * read header to synch ucode
						 * drive has be spun up
						 */
						idcaddr->idccsr = (ui->ui_slave<<8)|IDC_RHDR;
						(void) idcwait(idcaddr, 0);
						/* read the MPR twice */
						if (idcaddr->idcmpr == idcaddr->idcmpr);
						/* reset the drive */
						idcaddr->idcmpr = IDCGS_GETSTAT;
						idcaddr->idccsr = IDC_GETSTAT|(unit<<8);
						(void) idcwait(idcaddr, 0);
#ifdef IDCDEBUG
						printd(", drive ready");
#endif IDCDEBUG
						sc->sc_offline[ui->ui_unit] = 0;
					}
					if (idcustart(ui)) {
						sc->sc_softas = as;
						return;
					}
				} else {
					/* drive spun down */
#ifdef IDCDEBUG
					printd(", drive not ready");
#endif IDCDEBUG
					sc->sc_flags[ui->ui_unit] |= DEV_OFFLINE;
					sc->sc_offline[ui->ui_unit]++;
				}
			} else {
#ifdef IDCDEBUG
				printd(", unsol. intr. unit %d", unit);
#endif IDCDEBUG
			}
		}
#ifdef IDCDEBUG
	printd("\n");
#endif IDCDEBUG
	if (um->um_tab.b_actf && um->um_tab.b_active == 0) {
#ifdef IDCDEBUG
		trace("stum",um->um_tab.b_actf);
#endif IDCDEBUG
		(void) idcstart(um);
	}
}

idcwait(addr, n)
	register struct idcdevice *addr;
	register int n;
{
	register int i;

	while (--n && (addr->idccsr & IDC_CRDY) == 0)
		for (i = 10; i; i--)
			;
	return (n);
}

idcread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(idcstrategy, &ridcbuf[unit], dev, B_READ, minphys, uio));
}

idcwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(idcstrategy, &ridcbuf[unit], dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
idcioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui = idcdinfo[unit];
	register struct pt *pt = (struct pt *)data;
	register struct idc_softc *sc = &idc_softc;

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

		*pt = idc_part[unit];
		break;

	case DIOCDGTPT: /* return default partition table */
		/*
		 * Get number of sectors per cylinder
		 */
		nspc = idcst[ui->ui_type].nspc;

		/*
		 * Get default block count and offset
		 */
		for( i = 0; i <= 7; i++ ) {
			pt->pt_part[i].pi_nblocks =
				idcst[ui->ui_type].sizes[i].nblocks;
			pt->pt_part[i].pi_blkoff =
				idcst[ui->ui_type].sizes[i].cyloff * nspc;
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
		if ( ( error = ptcmp( dev, &idc_part[unit], pt ) ) != 0 )
			return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */

		idc_part[unit] = *pt;


		/*
		 *	See if we need to update the superblock of the
		 *	"a" partition of this disk
		 */
		ssblk(dev,pt);

		/*
		 *	Just make sure that we set the valid bit
		 */

		idc_part[unit].pt_valid = PT_VALID;
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
		bcopy(DEV_IDC,devget->interface,
		      strlen(DEV_IDC)); 		/* IDC		*/
		bcopy(sc->sc_device[unit],
		      devget->device,
		      strlen(sc->sc_device[unit]));	/* RB?? 	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA	*/
		devget->ctlr_num = ui->ui_ctlr; 	/* IDC 0 only	*/
		devget->slave_num = ui->ui_slave;	/* which plug	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "rb"	*/
		devget->unit_num = unit;		/* which rb??	*/
		devget->soft_count = sc->sc_softcnt[unit];    /* soft er. cnt.*/
		devget->hard_count = sc->sc_hardcnt[unit];    /* hard er. cnt.*/
		devget->stat = sc->sc_flags[unit];	      /* status       */
		devget->category_stat = DEV_DISKPART;	/* which prtn.	*/
		break;

	default:
		return (ENXIO);
	}
	return(0);
}

idcecc(ui)
	register struct uba_device *ui;
{
	register struct idcdevice *idc = (struct idcdevice *)ui->ui_addr;
	register struct buf *bp = idcutab[ui->ui_unit].b_actf;
	register struct uba_ctlr *um = ui->ui_mi;
	register struct idcst *st;
	register int i;
	struct idc_softc *sc = &idc_softc;
	struct uba_regs *ubp = ui->ui_hd->uh_uba;
	int bit, byte, mask;
	caddr_t addr;
	int reg, npf, o;
	int cn, tn, sn;

	npf = btop(idc->idcbcr + sc->sc_bcnt) - 1;;
	reg = btop(sc->sc_ubaddr) + npf;
	o = (int)bp->b_un.b_addr & PGOFSET;
	st = &idcst[ui->ui_type];
	cn = sc->sc_cyl;
	tn = sc->sc_trk;
	sn = sc->sc_sect;
	um->um_tab.b_active = 1;	/* Either complete or continuing... */
	mprintf("rb%d%c: soft ecc sn%d\n", dkunit(bp),
	    'a'+(minor(bp->b_dev)&07),
	    (cn*st->ntrak + tn) * st->nsect + sn + npf);
	mask = idc->idceccpat;
	i = idc->idceccpos - 1; 	/* -1 makes 0 origin */
	bit = i&07;
	i = (i&~07)>>3;
	byte = i + o;
	while (i < 512 && (int)ptob(npf)+i < sc->sc_bcnt && bit > -11) {
		struct	pte	pte;

		pte = ubp->uba_map[reg + btop(byte)];
		addr = ptob(pte.pg_pfnum)+ (byte & PGOFSET);
		putmemc(addr, getmemc(addr)^(mask<<bit));
		byte++;
		i++;
		bit -= 8;
	}
	sc->sc_bcnt += idc->idcbcr;
	um->um_tab.b_errcnt = 0;	/* error has been corrected */
	return;
}

idcreset(uban)
	int uban;
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register unit;

	if ((um = idcminfo[0]) == 0 || um->um_ubanum != uban ||
	    um->um_alive == 0)
		return;
	printf(" idc0");
	um->um_tab.b_active = 0;
	um->um_tab.b_actf = um->um_tab.b_actl = 0;
	if (um->um_ubinfo) {
		printf("<%d>", (um->um_ubinfo>>28)&0xf);
		um->um_ubinfo = 0;
	}
	for (unit = 0; unit < nNRB; unit++) {
		if ((ui = idcdinfo[unit]) == 0 || ui->ui_alive == 0)
			continue;
		idcutab[unit].b_active = 0;
		(void) idcustart(ui);
	}
	(void) idcstart(um);
}

idcwatch()
{
	register struct uba_ctlr *um;
	register unit;

	timeout(idcwatch, (caddr_t)0, hz);
	um = idcminfo[0];
	if (um == 0 || um->um_alive == 0)
		return;
	if (um->um_tab.b_active == 0) {
		for (unit = 0; unit < nNRB; unit++)
			if (idcutab[unit].b_active)
				goto active;
		idcwticks = 0;
		return;
	}
active:
	idcwticks++;
	if (idcwticks >= 20) {
		idcwticks = 0;
		printf("idc0: lost interrupt\n");
		idcintr(0);
	}
}

/*ARGSUSED*/
idcdump(dev, dumpinfo)
	dev_t dev;			/* dump device */
	struct dumpinfo  dumpinfo;	/* dump info */
{
#ifdef notdef

	struct idcdevice *idcaddr;
	char *start;
	char *start_tmp;
	int blk, unit;
	register struct uba_regs *uba;
	register struct uba_device *ui;
	struct idcst *st;
	union idc_dar dar;
	int nspg,ubatype;

	unit = minor(dev) >> 3;
	if (unit >= nNRB)
		return (ENXIO);
#define phys(cast, addr) ((cast)((int)addr & 0x7fffffff))
	ui = phys(struct uba_device *, idcdinfo[unit]);
	if (ui->ui_alive == 0)
		return (ENXIO);
	uba = phys(struct uba_hd *, ui->ui_hd)->uh_physuba;
	ubatype = phys(struct uba_hd *, ui->ui_hd)->uba_type;
	ubainit(uba,ubatype);
	idcaddr = (struct idcdevice *)ui->ui_physaddr;
	if (idcwait(idcaddr, 100) == 0)
		return (EFAULT);
	/*
	 * Since we can only transfer one track at a time, and
	 * the rl02 has 256 byte sectors, all the calculations
	 * are done in terms of physical sectors (i.e. num and blk
	 * are in sectors not NBPG blocks.
	 */
	st = phys(struct idcst *, &idcst[ui->ui_type]);
	nspg = NBPG / st->nbps;
	dumpinfo.size_to_dump *= nspg;
	start = start_tmp = 0;

	/*
	 * If a full dump is being performed, then this loop
	 * will dump all of core. If a partial dump is being
	 * performed, then as much of core as possible will be
	 * dumped, leaving room for the u_area and the error logger
	 * buffer. Please note that dumpsys predetermined what
	 * type of dump will be performed.
	 */

	while ((dumpinfo.size_to_dump > 0) || (dumpinfo.partial_dump)) {
		register struct pte *io;
		register int i;
		daddr_t bn;

		bn = (dumplo + btop(start_tmp)) * nspg;
		dar.dar_cyl = bn / st->nspc + dumpinfo.blkoffs / st->nspc;
		bn %= st->nspc;
		dar.dar_trk = bn / st->nsect;
		dar.dar_sect = bn % st->nsect;
		blk = st->nsect - dar.dar_sect;
		if (dumpinfo.size_to_dump < blk)
			blk = dumpinfo.size_to_dump;

		io = uba->uba_map;
		for (i = 0; i < (blk + nspg - 1) / nspg; i++)
			*(int *)io++ = (btop(start)+i) | (1<<21) | UBAMR_MRV;
		*(int *)io = 0;

		idcaddr->idccsr = IDC_CRDY | IDC_SEEK | unit<<8;
		if ((idcaddr->idccsr&IDC_DRDY) == 0)
			return (EFAULT);
		idcaddr->idcdar = dar.dar_dar;
		idcaddr->idccsr = IDC_SEEK | unit << 8;
		while ((idcaddr->idccsr & (IDC_CRDY|IDC_DRDY))
			!= (IDC_CRDY|IDC_DRDY))
			;
		if (idcaddr->idccsr & IDC_ERR) {
			printf("rb%d: seek, csr=%b\n",
				unit, idcaddr->idccsr, IDCCSR_BITS);
			return (EIO);
		}

		idcaddr->idccsr = IDC_CRDY | IDC_WRITE | unit<<8;
		if ((idcaddr->idccsr&IDC_DRDY) == 0)
			return (EFAULT);
		idcaddr->idcbar = 0;			/* start addr 0 */
		idcaddr->idcbcr = - (blk * st->nbps);
		idcaddr->idcdar = dar.dar_dar;
		idcaddr->idccsr = IDC_WRITE | unit << 8;
		while ((idcaddr->idccsr & (IDC_CRDY|IDC_DRDY))
			!= (IDC_CRDY|IDC_DRDY))
			;
		if (idcaddr->idccsr & IDC_ERR) {
			printf("rb%d: write, csr=%b\n",
				unit, idcaddr->idccsr, IDCCSR_BITS);
			return (EIO);
		}

		start += blk * st->nbps;
		start_tmp += blk * st->nbps;
		dumpinfo.size_to_dump -= blk;
		if ((dumpinfo.size_to_dump <= 0) && (dumpinfo.partial_dump)) {

			/*
			 * If a partial dump is being performed....
			 */

			/* Set size_to_dump to the number of pages to dump */
			dumpinfo.size_to_dump =
			  dumpinfo.pdump[NUM_TO_DUMP-dumpinfo.partial_dump].num_blks * nspg;
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

idcsize(dev)
	dev_t dev;
{
	int unit = minor(dev) >> 3;
	struct uba_device *ui;
	struct idcst *st;

	if (unit >= nNRB || (ui = idcdinfo[unit]) == 0 || ui->ui_alive == 0)
		return (-1);
	/*
	 *	Sanity check		001
	 */
	if ( idc_part[unit].pt_valid != PT_VALID )
		panic("idcsize: invalid partition table ");

	return (idc_part[unit].pt_part[minor(dev) & 07].pi_nblocks);
}
#endif
