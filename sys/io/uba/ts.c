#ifndef lint
static char *sccsid = "@(#)ts.c	4.1      (ULTRIX)        7/2/90";
#endif	lint

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
 * ts.c    6.1	   07/29/83
 *
 * Modification history
 *
 * TS11/TSU05/TSV05/TU80 tape driver
 *
 * 22-Jan-85 - Larry Cohen
 *
 *	Derived from 4.2BSD labeled: ts.c	6.1	83/07/29.
 *	Make probe routine interrupt device instead of hard wired
 *	interrupt vectors. LSC001:
 *
 *  6-Mar-85 - Larry Cohen
 *
 *	Make probe route interrupt correctly. LSC002:
 *
 * 13-Mar-85 - jaw
 *
 *	Add support for VAX8200.
 *
 * 19-Jun-85 - jaw
 *
 *	VAX8200 name change.
 *
 * 11-Jul-85 - jaw
 *
 *	Fix bua/bda map registers.
 *
 *  6-Aug-85 - ricky palmer
 *
 *	Added new ioctl functionality as well as new code to handle
 *	EOT correctly. Driver now supports new minor number convention
 *	to allow for more than 4 tape devices. V2.0
 *
 *  8-Feb-86 - ricky palmer
 *
 *	Added streaming tape support for ts05/tsv05 subsystem. V2.0
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
 * 30-Oct-86 - lp
 *
 *	TPMARK & HARDERR now prevent further n-buffered requests.
 *
 * 04-Dec-86 - pmk
 *
 *	Changed mprintf to log just one entry.
 *
 * 08-Jul-87 - rsp
 *
 *	Corrected attach routine to correctly fill in TS05 device string.
 *
 * 05-MAY-89 - dallas
 *	Modified the ts_init and ts_probe routines to not set any of the
 *	bits in the extended characteristics data word for the wrt. char.
 *	command. The set of these bits are for diag's purposes only. To
 * 	bad the documentation leads you to believe that you should be
 *	mucking with the word. Setting of the bits in the word really
 *	confuses the controller. Also added global int ts_softerr and a
 *	mprintf - for field service and css purposes. With adb the setting
 *	of the int allows all soft errors to be reported to the errlog. 
 *
 * 05-Jun-89 - Tim Burke
 *    Added a new MTIOCTOP called MTFLUSH.  This is used on caching drives
 *    to force a flush of the controller's write back cache.  Since this
 *    is not a caching drive the MTFLUSH always returns ENXIO.
 *
 * 22-Jul-89 - kuo-hsiung hsieh
 *	Added codes for mipsfair (DS_5400).
 *
 * 05-DEC-89 - Bill Dallas
 *	In tswait changed the declaration of register int s to
 *	register volatile int s This change is for the mips compiler
 *	optimization of the assign out of the do while loop if
 *	not declared as volatile.
 */

#include "ts.h"
#if NTS > 0 || defined(BINARY)

#include "../data/ts_data.c"

extern int cpu;

u_short tsstd[] = { TSSTD, 0 };
int	tsprobe(), tsslave(), tsattach(), tsdgo(), tsintr();

struct	uba_driver zsdriver = { tsprobe, tsslave, tsattach, tsdgo,
				tsstd, "ts", tsdinfo, "zs",
				tsminfo, 0 };

int ts_softerr = 0;	/* declared for soft error reports..Field service
		 	 * needs this.. Reasons for not streaming - dallas */

/*
 * Determine if there is a controller for
 * a ts at address reg.  The goal is to make the
 * device interrupt.
 */
tsprobe(reg, ctrl)
	caddr_t reg;
	int ctrl;
{
	int i, ubaddr, uba_cmdaddr;
	struct ts_softc *sc;

#ifdef lint
	tsintr(0);
#endif lint

	((struct tsdevice *)reg)->tssr = 0;
/*
 * LSC001:
 * LSC002:
 */
	DELAY(5000000);
	if ((((struct tsdevice *)reg)->tssr & TS_NBA) == 0)
		return(0);

	sc = &ts_softc[ctrl];
	ctsbuf[ctrl].b_un.b_addr = (caddr_t)sc;
	ctsbuf[ctrl].b_bcount = sizeof(*sc);
	ubaddr = ubasetup(numuba, &ctsbuf[ctrl], 0);

	i = ubaddr & 0777777;
	sc->sc_ubaddr = (struct ts_softc *)i;
	uba_cmdaddr = (int)&sc->sc_ubaddr->sc_cmd;

	sc->sc_uba = (u_short)(uba_cmdaddr + ((uba_cmdaddr>>16)&3));
	sc->sc_char.char_addr = (int)&sc->sc_ubaddr->sc_sts;
	sc->sc_char.char_size = sizeof(struct ts_sts);
	sc->sc_char.char_mode = TS_ESS;
	/* dallas get rid of following line these bits are only for diags.
	 * and seems to really confuse the drive/controller 
	 * sc->sc_char.char_modext = TS_ENHSP|TS_ENBUF;*/	
	sc->sc_char.char_modext = 0; /* make sure 0 for tsv and tsu controllers,
					for ts11 controllers it ignores them. */ 
	sc->sc_cmd.c_cmd = TS_ACK | TS_IE | TS_SETCHR;
	i = (int)&sc->sc_ubaddr->sc_char;
	sc->sc_cmd.c_loba = i;
	sc->sc_cmd.c_hiba = (i>>16)&3;
	sc->sc_cmd.c_size = sizeof(struct ts_char);
	((struct tsdevice *)reg)->tsdb = sc->sc_uba;
	DELAY(5000000);

	if (cvec && cvec == 0x200)  /* check for interrupt */
		ubarelse(numuba, &ubaddr); /* release resources */
	else
		sc->sc_mapped++;
	((struct tsdevice *)reg)->tssr = 0; /* reset device so
					     * first open
					     * does not hang
					     */
	DELAY(100000);

	return (sizeof (struct tsdevice));
}

/*
 * TS11 only supports one drive per controller;
 * check for ui_slave == 0.
 */
tsslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{
	if (ui->ui_slave)	/* non-zero slave not allowed */
		return(0);
	return (1);
}

/*
 * Record attachment of the unit to the controller.
 */
tsattach(ui)
	struct uba_device *ui;
{
	register struct ts_softc *sc = &ts_softc[ui->ui_unit];

	sc->sc_softcnt = 0;
	sc->sc_hardcnt = 0;
	if(sc->sc_sts.s_xs2 & TS_TU80) {
		bcopy(DEV_TU80,sc->sc_device,strlen(DEV_TU80));
	} else {
		if(ui->ui_hd->uba_type & (UBAUVI|UBAUVII)) {
			bcopy(DEV_TS05,sc->sc_device,strlen(DEV_TS05));
		} else {
			bcopy(DEV_TS11,sc->sc_device,strlen(DEV_TS11));
		}
	}


}

/*
 * Open the device.  Tapes are unique open
 * devices, so we refuse if it is already open.
 * We also check that a tape is available, and
 * don't block waiting here; if you want to wait
 * for a tape you should timeout in user code.
 */
tsopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct uba_device *ui;
	register struct ts_softc *sc;
	register int unit = UNIT(dev);

	if (unit >= nNTS || (sc = &ts_softc[unit])->sc_openf ||
	    (ui = tsdinfo[unit]) == 0 || ui->ui_alive == 0) {
		return (ENXIO);
	}

	if ((sc->sc_sts.s_xs0 & TS_EOT) && (dis_eot_ts[unit] !=
	    DISEOT)) {
		sc->sc_flags &= DEV_EOM;
	} else {
		sc->sc_flags = 0;
	}
	sc->sc_category_flags = 0;

	if (tsinit(unit))
		return (ENXIO);

	tscommand(dev, TS_SENSE, 1);
	sc->sc_category_flags |= DEV_1600BPI;

	if((strcmp(DEV_TS05,sc->sc_device)) == 0) {
		DELAY(100000);
		tscommand(dev, TS_SENSE, 1);
	}

	if ((sc->sc_sts.s_xs0 & TS_ONL) == 0) {
		sc->sc_flags |= DEV_OFFLINE;
		if(!(flag & FNDELAY)) {
			DEV_UGH(sc->sc_device,unit,"offline");
			return(EIO);
		}
	}

	if (sc->sc_sts.s_xs0 & TS_WLK) {
		sc->sc_flags |= DEV_WRTLCK;
	}

	if ((flag & (FREAD|FWRITE)) == FWRITE &&
	    (sc->sc_sts.s_xs0 & TS_WLK)) {
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

/*
 * Close tape device.
 *
 * If tape was open for writing or last operation was
 * a write, then write two EOF's and backspace over the last one.
 * Unless this is a non-rewinding special file, rewind the tape.
 * Make the tape available to others.
 */
tsclose(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct ts_softc *sc = &ts_softc[UNIT(dev)];
	register int unit = UNIT(dev);
	register int sel = SEL(dev);

	sc->sc_flags &= ~DEV_EOM;

	if (flag == FWRITE || (flag&FWRITE) &&
	    (sc->sc_flags & DEV_WRITTEN)) {
		tscommand(dev, TS_WEOF, 1);
		sc->sc_flags &= ~DEV_EOM;
		tscommand(dev, TS_WEOF, 1);
		sc->sc_flags &= ~DEV_EOM;
		tscommand(dev, TS_SREV, 1);
		sc->sc_flags &= ~DEV_EOM;
	}

	if (sel == MTHR)
		/*
		 * 0 count means don't hang waiting for rewind complete
		 * rather ctsbuf stays busy until the operation
		 * completes preventing further opens from completing
		 * by preventing a TS_SENSE from completing.
		 */
		tscommand(dev, TS_REW, 0);

	sc->sc_openf = 0;

	if ((sc->sc_sts.s_xs0 & TS_EOT) && (dis_eot_ts[unit] !=
	    DISEOT)) {
		sc->sc_flags |= DEV_EOM;
	}
}

/*
 * Initialize the TS11.  Set up bus mapping for command
 * packets and set device characteristics.
 */
tsinit(unit)
	register int unit;
{
	register struct ts_softc *sc = &ts_softc[unit];
	register struct uba_ctlr *um = tsminfo[unit];
	register struct tsdevice *addr = (struct tsdevice *)um->um_addr;
	register int i;

	/*
	 * Map the command and message packets into bus
	 * address space.  We do all the command and message
	 * packets at once to minimize the amount of bus
	 * mapping necessary.
	 */
	if (sc->sc_mapped == 0) {
		ctsbuf[unit].b_un.b_addr = (caddr_t)sc;
		ctsbuf[unit].b_bcount = sizeof(*sc);
		i = ubasetup(um->um_ubanum, &ctsbuf[unit], 0);
		i &= 0777777;
		sc->sc_ubaddr = (struct ts_softc *)i;
		sc->sc_mapped++;
	}
	/*
	 * Now initialize the TS11 controller.
	 * Set the characteristics.
	 */
	if (addr->tssr & (TS_NBA|TS_OFL)) {
		addr->tssr = 0; 	/* subsystem initialize */
		tswait(addr);
		i = (int)&sc->sc_ubaddr->sc_cmd;  /* bus addr of cmd */
		sc->sc_uba = (u_short)(i + ((i>>16)&3));
		sc->sc_char.char_addr = (int)&sc->sc_ubaddr->sc_sts;
		sc->sc_char.char_size = sizeof(struct ts_sts);
		sc->sc_char.char_mode = TS_ESS;
		/* get rid of this line, bits only for diags and really
		 * confuses the tsv05
		 * sc->sc_char.char_modext = TS_ENHSP|TS_ENBUF; */ 
		sc->sc_char.char_modext = 0; /* for tsv and tsu controllers
						ts11 controllers ignore them. */
		sc->sc_cmd.c_cmd = TS_ACK | TS_SETCHR;
		i = (int)&sc->sc_ubaddr->sc_char;
		sc->sc_cmd.c_loba = i;
		sc->sc_cmd.c_hiba = (i>>16)&3;
		sc->sc_cmd.c_size = sizeof(struct ts_char);
		addr->tsdb = sc->sc_uba;
		tswait(addr);
		if (addr->tssr & TS_NBA)
			return(1);
	}
	return(0);
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 */
tscommand(dev, com, count)
	register dev_t dev;
	register int com;
	register int count;
{
	register struct buf *bp = &ctsbuf[UNIT(dev)];
	register int s = splbio();

	while (bp->b_flags & B_BUSY) {
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags & B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}

	bp->b_flags = B_BUSY|B_READ;
	splx(s);
	bp->b_dev = dev;
	bp->b_repcnt = count;
	bp->b_command = com;
	bp->b_blkno = 0;
	tsstrategy(bp);

	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return;

	iowait(bp);

	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;
}

/*
 * Queue a tape operation.
 */
tsstrategy(bp)
	register struct buf *bp;
{
	register struct uba_ctlr *um;
	register struct ts_softc *sc = &ts_softc[UNIT(bp->b_dev)];
	register struct buf *dp;
	register int s;
	int unit = UNIT(bp->b_dev);

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_ts[unit] & DISEOT))) {
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

	/*
	 * Put transfer at end of controller queue
	 */
	bp->av_forw = NULL;
	um = tsdinfo[unit]->ui_mi;
	s = splbio();
	dp = &tsutab[unit];
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	um->um_tab.b_actf = um->um_tab.b_actl = dp;
	/*
	 * If the controller is not busy, get
	 * it going.
	 */
	if (um->um_tab.b_active == 0)
		tsstart(um);
	splx(s);
}

/*
 * Start activity on a ts controller.
 */
tsstart(um)
	register struct uba_ctlr *um;
{
	register struct tsdevice *addr = (struct tsdevice *)um->um_addr;
	register struct buf *bp;
	register struct ts_softc *sc;
	register struct ts_cmd *tc;
	struct uba_device *ui;
	int cmd, unit;
	daddr_t blkno;

loop:
	/*
	 * Start the controller if there is something for it to do.
	 */
	if ((bp = um->um_tab.b_actf->b_actf) == NULL)
		return;

	unit = UNIT(bp->b_dev);
	ui = tsdinfo[unit];
	sc = &ts_softc[unit];
	tc = &sc->sc_cmd;

	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_ts[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		goto next;
	}

	if (((sc->sc_category_flags&DEV_TPMARK) ||(sc->sc_flags&DEV_HARDERR)) &&
	    (bp->b_flags & B_READ) && (bp->b_flags&B_RAWASYNC)) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		goto next;
	}

	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in tsintr().
	 */
	sc->sc_flags &= ~DEV_WRITTEN;

	if (sc->sc_openf < 0 || (addr->tssr & TS_OFL)) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		bp->b_flags |= B_ERROR;
		goto next;
	}

	if (bp == &ctsbuf[unit]) {
		/*
		 * Execute control operation with the specified count.
		 */
		um->um_tab.b_active =
		    bp->b_command == TS_REW ? SREW : SCOM;
		tc->c_repcnt = bp->b_repcnt;
		goto dobpcmd;
	}

	if (sc->sc_blkno != bdbtofsb(bp->b_blkno) &&
	    !um->um_tab.b_errcnt)
		sc->sc_blkno = bdbtofsb(bp->b_blkno);

	sc->sc_nxrec = bdbtofsb(bp->b_blkno) + 1;

	/*
	 * If the data transfer command is in the correct place,
	 * set up all the registers except the csr, and give
	 * control over to the BUS adapter routines, to
	 * wait for resources to start the i/o.
	 */
	if ((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		tc->c_size = bp->b_bcount;
		if ((bp->b_flags&B_READ) == 0)
			cmd = TS_WCOM;
		else
			cmd = TS_RCOM;
		if (um->um_tab.b_errcnt) {
			cmd |= TS_RETRY;
			sc->sc_softcnt++;
			sc->sc_flags |= DEV_RETRY;
		}
		um->um_tab.b_active = SIO;
		tc->c_cmd = TS_ACK | TS_CVC | TS_IE | cmd;
		(void) ubago(ui);
		return;
	}

	/*
	 * Tape positioned incorrectly;
	 * set to seek forwards or backwards to the correct spot.
	 * This happens for raw tapes only on error retries.
	 */
	um->um_tab.b_active = SSEEK;
	if (blkno < bdbtofsb(bp->b_blkno)) {
		bp->b_command = TS_SFORW;
		tc->c_repcnt = bdbtofsb(bp->b_blkno) - blkno;
	} else {
		bp->b_command = TS_SREV;
		tc->c_repcnt = blkno - bdbtofsb(bp->b_blkno);
	}

dobpcmd:
	/*
	 * Do the command in bp.
	 */
	tc->c_cmd = TS_ACK | TS_CVC | TS_IE | bp->b_command;
	addr->tsdb = sc->sc_uba;
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 * Release UBA resources (if any), dequeue
	 * the transfer and continue processing this slave.
	 */
	if (um->um_ubinfo)
		ubadone(um);
	um->um_tab.b_errcnt = 0;
	um->um_tab.b_actf->b_actf = bp->av_forw;
	iodone(bp);
	goto loop;
}

/*
 * The BUS resources we needed have been
 * allocated to us; start the device.
 */
tsdgo(um)
	register struct uba_ctlr *um;
{
	register struct tsdevice *addr = (struct tsdevice *)um->um_addr;
	register struct ts_softc *sc = &ts_softc[um->um_ctlr];
	register int i;

	i = um->um_ubinfo & 0777776;	/* odd address transfer fix */
	sc->sc_cmd.c_loba = i;
	sc->sc_cmd.c_hiba = (i>>16)&3;
	addr->tsdb = sc->sc_uba;
}

/*
 * Ts interrupt routine.
 */
tsintr(ts11)
	int ts11;
{
	register struct buf *bp;
	register struct uba_ctlr *um = tsminfo[ts11];
	register struct tsdevice *addr;
	register struct ts_softc *sc;
	register int state;
	register int unit;

	if ((bp = um->um_tab.b_actf->b_actf) == NULL)
		return;

	unit = UNIT(bp->b_dev);
	addr = (struct tsdevice *)tsdinfo[unit]->ui_addr;

	/*
	 * If last command was a rewind, and tape is still
	 * rewinding, wait for the rewind complete interrupt.
	 */
	if (um->um_tab.b_active == SREW) {
		um->um_tab.b_active = SCOM;
		if ((addr->tssr&TS_SSR) == 0)
			return;
	}

	/*
	 * An operation completed... record status
	 */
	sc = &ts_softc[unit];

#ifdef mips
	clean_dcache(PHYS_TO_K0(svtophy(&sc->sc_sts)), sizeof(struct ts_sts));
#endif mips

	if ((bp->b_flags & B_READ) == 0)
		sc->sc_flags |= DEV_WRITTEN;

	state = um->um_tab.b_active;
	um->um_tab.b_active = 0;

	/*
	 * Check for errors.
	 */
	if (addr->tssr&TS_SC) {

		switch (addr->tssr & TS_TC) {

		case TS_UNREC:		/* unrecoverable */
		case TS_FATAL:		/* fatal error */
		case TS_ATTN:		/* attention */
		case TS_RECNM:		/* recoverable, no motion */
			break;

		case TS_SUCC:		/* success termination */
			goto ignoreerr;

		case TS_ALERT:		/* tape status alert */
			/*
			 * If we hit the end-of-tape (EOT),
			 * just return.
			 */
			if (sc->sc_sts.s_xs0 & TS_EOT) {
				sc->sc_flags |= DEV_EOM;
				goto opdone;
			}
			/*
			 * If we hit a tapemark,
			 * update our position.
			 */
			if (sc->sc_sts.s_xs0 & TS_TMK) {
			    sc->sc_category_flags |= DEV_TPMARK;
			    if (bp == &ctsbuf[unit]) {
				    if (sc->sc_blkno >
					bdbtofsb(bp->b_blkno)) {
					/* reversing */
					sc->sc_nxrec =
						bdbtofsb(bp->b_blkno) -
						sc->sc_sts.s_rbpcr;
					sc->sc_blkno = sc->sc_nxrec;
				    } else {
					/* spacing forward */
					sc->sc_blkno =
						bdbtofsb(bp->b_blkno) +
						sc->sc_sts.s_rbpcr;
					sc->sc_nxrec = sc->sc_blkno - 1;
				    }
				    goto seteof;
			    }
			    /* eof on read */
			    sc->sc_nxrec = bdbtofsb(bp->b_blkno);
seteof:
			    state = SCOM;	/* force completion */
			    /*
			     * Stuff bc so it will be unstuffed correctly
			     * later to get resid.
			     */
			    sc->sc_sts.s_rbpcr = bp->b_bcount;
			    goto opdone;
			}
			/*
			 * If we were reading raw tape and the record was
			 * too long or too short, then we don't consider
			 * this an error.
			 */
			if (bp != &ctsbuf[unit] &&
			    (bp->b_flags&B_READ) &&
			    sc->sc_sts.s_xs0&(TS_RLS|TS_RLL)) {
				sc->sc_category_flags |= DEV_SHRTREC;
				goto ignoreerr;
			}

		case TS_RECOV:		/* recoverable, tape moved */
			/*
			 * If this was an i/o operation retry 8 times.
			 */
			if (state==SIO) {
				if (++um->um_tab.b_errcnt < 7) {
					/* for soft error reporting - dallas */
					if(ts_softerr) {
						mprintf("%s: unit#:%d soft err blk#:%d \
						xs0:%b xs1:%b xs2:%b xs3:%b xs4:%b\n",
						sc->sc_device, unit, bp->b_blkno,
						sc->sc_sts.s_xs0, TSXS0_BITS,
						sc->sc_sts.s_xs1, TSXS1_BITS,
						sc->sc_sts.s_xs2, TSXS2_BITS,
						sc->sc_sts.s_xs3, TSXS3_BITS,
						sc->sc_sts.s_xs4, TSXS4_BITS);
						}
					sc->sc_softcnt++;
					ubadone(um);
					goto opcont;
				} else
					sc->sc_blkno++;
			} else {
				/*
				 * Non-i/o errors on non-raw tape
				 * cause it to close.
				 */
				if (sc->sc_openf>0 &&
				    bp == &ctsbuf[unit])
					sc->sc_openf = -1;
			}
			break;

		case TS_REJECT: 	/* function reject */
			if (state == SIO && sc->sc_sts.s_xs0 & TS_WLE)
				sc->sc_flags |= DEV_WRTLCK;
			if ((sc->sc_sts.s_xs0 & TS_ONL) == 0)
				sc->sc_flags |= DEV_OFFLINE;
			break;
		}

		/*
		 * Couldn't recover error
		 */
		sc->sc_flags |= DEV_HARDERR;
		sc->sc_hardcnt++;

		mprintf("%s: unit#:%d hard err blk#:%d \
			xs0:%b xs1:%b xs2:%b xs3:%b xs4:%b\n",
			sc->sc_device, unit, bp->b_blkno,
			sc->sc_sts.s_xs0, TSXS0_BITS,
			sc->sc_sts.s_xs1, TSXS1_BITS,
			sc->sc_sts.s_xs2, TSXS2_BITS,
			sc->sc_sts.s_xs3, TSXS3_BITS,
			sc->sc_sts.s_xs4, TSXS4_BITS);

		bp->b_flags |= B_ERROR;
		goto opdone;
	}

	/*
	 * Advance tape control FSM.
	 */
ignoreerr:
	if (sc->sc_sts.s_xs0 & TS_EOT) {
		sc->sc_flags |= DEV_EOM;
	}

	switch (state) {

	case SIO:
		/*
		 * Read/write increments tape block number
		 */
		sc->sc_blkno++;
		goto opdone;

	case SCOM:
		/*
		 * For forw./back. space record update current position.
		 */
		if (bp == &ctsbuf[unit])
		switch (bp->b_command) {

		case TS_SFORW:
			sc->sc_blkno += bp->b_repcnt;
			break;

		case TS_SREV:
			sc->sc_blkno -= bp->b_repcnt;
			break;
		}
		goto opdone;

	case SSEEK:
		sc->sc_blkno = bdbtofsb(bp->b_blkno);
		goto opcont;

	default:
		panic("tsintr");
	}

opdone:
	/*
	 * Reset error count and remove
	 * from device queue.
	 */
	sc->sc_flags |= DEV_DONE;
	um->um_tab.b_errcnt = 0;
	um->um_tab.b_actf->b_actf = bp->av_forw;
	bp->b_resid = sc->sc_sts.s_rbpcr;
	ubadone(um);
/* hsieh - Due to the delay in processing large nbuf io (> 24k buffer) 
 * with the bufflush routine we must see if there
 * is anything on the queue first and then start it.
 * We must keep the tape streaming.  Thrashing occurs if tape
 * drive hit the next tape gap and we are still hanging in
 * the bufflush code.  The problem is seen at buffer size greater
 * than 24kb system.  Dallas suggested I kicked the controller
 * with next command before calling bufflush.	
 */
	if (um->um_tab.b_actf->b_actf != 0)
		tsstart(um);
#ifdef mips
	if(cpu != DS_5800 && (bp->b_flags & B_READ)){
		bufflush(bp);
	}
#endif mips
	iodone(bp);
	return;
opcont:
	tsstart(um);
}

tsread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = UNIT(dev);

	return (physio(tsstrategy, &rtsbuf[unit], dev, B_READ,
		minphys, uio));
}

tswrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = UNIT(dev);

	return (physio(tsstrategy, &rtsbuf[unit], dev, B_WRITE,
		minphys, uio));
}

tsreset(uban)
	register int uban;
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register struct ts_softc *sc;
	register struct buf *dp;
	register ts11;

	for (ts11 = 0; ts11 < nNTS; ts11++) {
		if ((um = tsminfo[ts11]) == 0 || um->um_alive == 0 ||
		   um->um_ubanum != uban)
			continue;
		sc = &ts_softc[ts11];
		mprintf("ts reset");
		um->um_tab.b_active = 0;
		um->um_tab.b_actf = um->um_tab.b_actl = 0;
		if (sc->sc_openf > 0)
			sc->sc_openf = -1;
		if (um->um_ubinfo) {
			mprintf("<%d>", (um->um_ubinfo>>28)&0xf);
			um->um_ubinfo = 0;
		}
		if ((ui = tsdinfo[ts11]) && ui->ui_mi == um &&
		    ui->ui_alive) {
			dp = &tsutab[ts11];
			dp->b_active = 0;
			dp->b_forw = 0;
			if (um->um_tab.b_actf == NULL)
				um->um_tab.b_actf = dp;
			else
				um->um_tab.b_actl->b_forw = dp;
			um->um_tab.b_actl = dp;
		}
		sc->sc_mapped = 0;		/* so it won't think
						 * it is still
						 * allocated
						 * and tsinit will remap
						 * itself
						 */
		(void) tsinit(ts11);
		tsstart(um);
	}
}

tsioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct uba_device *ui = tsdinfo[UNIT(dev)];
	register struct ts_softc *sc = &ts_softc[UNIT(dev)];
	register struct buf *bp = &ctsbuf[UNIT(dev)];
	register callcount;
	register int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	struct devget *devget;
	int unit = UNIT(dev);

	/* we depend of the values and order of the MT codes here */
	static tsops[] = { TS_WEOF, TS_SFORWF, TS_SREVF, TS_SFORW,
			   TS_SREV, TS_REW, TS_OFFL, TS_SENSE };

	switch (cmd) {

	case MTIOCTOP:				/* tape operation */
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {

		case MTWEOF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;

		case MTFSF: case MTBSF:
		case MTFSR: case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
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
			dis_eot_ts[unit] = 0;
			return(0);

		case MTDISEOT:
			dis_eot_ts[unit] = DISEOT;
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
			tscommand(dev, tsops[mtop->mt_op], fcount);
			if ((mtop->mt_op == MTFSR ||
			    mtop->mt_op == MTBSR) &&
			    bp->b_resid)
				return (EIO);
			if ((bp->b_flags & B_ERROR) ||
			    sc->sc_sts.s_xs0 & TS_BOT)
				break;
		}
		return (geterror(bp));

	case MTIOCGET:				/* tape status */
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = 0;
		mtget->mt_erreg = sc->sc_sts.s_xs0;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISTS;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TAPE;

		if(ui->ui_hd->uba_type & (UBAUVI|UBAUVII)) {
			devget->bus = DEV_QB;
		} else {
			devget->bus = DEV_UB;
		}

		switch (devget->bus) {

		case DEV_UB:
			if(sc->sc_sts.s_xs2 & TS_TU80) {
				bcopy(DEV_TUU80,devget->interface,
				      strlen(DEV_TUU80));
			} else {
				if (sc->sc_sts.s_xs4 & TS_HSP) {
					bcopy(DEV_TSU05,
					      devget->interface,
					      strlen(DEV_TSU05));
				} else {
					bcopy(DEV_TSU11,
					      devget->interface,
					      strlen(DEV_TSU11));
				}
			}
			break;

		case DEV_QB:
			bcopy(DEV_TSV05,devget->interface,
			      strlen(DEV_TSV05));
			break;
		}

		bcopy(sc->sc_device,devget->device,
		      strlen(sc->sc_device));		/* t[su]	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA/QB */
		devget->ctlr_num = ui->ui_ctlr; 	/* which interf.*/
		devget->slave_num = ui->ui_slave;	/* which plug	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "ts"	*/
		devget->unit_num = unit;		/* which t[su]??*/
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

tsdump()
{
	register struct uba_device *ui;
	register struct uba_regs *up;
	register struct tsdevice *addr;
	register int blk;
	register int num;
	register int start;
	int	ubatype;

	start = 0;
	num = maxfree;
	if (tsdinfo[0] == 0)
		return (ENXIO);
	ui = PHYS(tsdinfo[0], struct uba_device *);
	up = PHYS(ui->ui_hd, struct uba_hd *)->uh_physuba;
	ubatype = PHYS(ui->ui_hd, struct uba_hd *)->uba_type;
	ubainit(up,ubatype);
	DELAY(1000000);
	addr = (struct tsdevice *)ui->ui_physaddr;
	addr->tssr = 0;
	tswait(addr);
	while (num > 0) {
		blk = num > DBSIZE ? DBSIZE : num;
		tsdwrite(start, blk, addr, up);
		start += blk;
		num -= blk;
	}
	tseof(addr);
	tseof(addr);
	tswait(addr);
	if (addr->tssr&TS_SC)
		return (EIO);
	addr->tssr = 0;
	tswait(addr);
	return (0);
}

tsdwrite(dbuf, num, addr, up)
	register int dbuf;
	register int num;
	register struct tsdevice *addr;
	struct uba_regs *up;
{
	register struct pte *io;
	register struct ts_softc *sc = &ts_softc[0];
	register int npf;

	tswait(addr);
	io = up->uba_map;
	npf = num+1;
	while (--npf != 0)
		 *(int *)io++ = (dbuf++ | (1<<UBAMR_DPSHIFT) |
				 UBAMR_MRV);
	*(int *)io = 0;
	sc->sc_cmd.c_size = -(num*NBPG);	/* byte count */
	sc->sc_cmd.c_loba = 0;			/* low buffer address */
	sc->sc_cmd.c_hiba = 0;			/* high buf. address */
	sc->sc_cmd.c_cmd = TS_ACK | TS_CVC | TS_IE | TS_WCOM;	/* doit */
}

tswait(addr)
	register struct tsdevice *addr;
{
	register volatile int s;

	do
		s = addr->tssr;
	while ((s & TS_SSR) == 0);
}

tseof(addr)
	register struct tsdevice *addr;
{
	register struct ts_softc *sc = &ts_softc[0];

	tswait(addr);
	sc->sc_cmd.c_cmd = TS_ACK | TS_CVC | TS_IE | TS_WEOF;	/* doit */
}
#endif


