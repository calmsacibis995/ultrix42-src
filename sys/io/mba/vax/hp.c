
#ifndef lint
static char *sccsid = "@(#)hp.c	4.1	ULTRIX	7/2/90";
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
 * hp.c    6.2	   09/25/83
 *
 * Modification history
 *
 * RH/RM03/RM05/RM80/RP05/RP06/RP07 disk driver
 *
 * 26-July-89 - Alan Frechette
 *	Conditionalize out the dump code.
 *
 * 19-Sep-84 - tresvik
 *
 *	Derived from 4.2BSD labeled: hp.c	6.2	83/09/25.
 *	Fixed the algorithm for determining where a bad sector is.
 *	The fix checks that the byte count at the time of an error is
 *	sector aligned.
 *
 *  2-Oct-84 - reilly
 *
 *	Added code to start handling the disk partitioning scheme. -001
 *
 * 30-Nov-84 - reilly
 *
 *	Fixed error message. -002
 *
 * 19-Dec-84 - tresvik
 *
 *	Unfixed algorithm for determining where a bad sector is.  The
 *	fix was added for RP07 which resulted in the RM05 exhibiting the
 *	same symptom.
 *
 *  6-Jun-85 - tresvik
 *
 *	Report contents of hpdc instead of hpcc for soft ecc errors.
 *	hpcc unused on RM03 drives.
 *
 * 24-Sep-85 - reilly
 *
 *	Added new ioctl call that will return the default partition
 *	table.
 *
 *  4-Dec-85 - Darrell Dunnuck
 *
 *	Really fixed the algorithm for determining where a bad sector
 *	is.  The most conservative half of MBA byte count register
 *	is read to determine which block is bad.  Also removed a
 *	statement that fudged the variable "npf" - which was off by
 *	one due to reading the wrong half of the MBA byte count register.
 *	Added a bug fix so that when a BSE error occurs, only the needed
 *	number of bytes is read, not always 512 as before.
 *
 * 30-Jan-86 - Tom Tresvik
 *
 *	Replaced fudge factor "npf" removed below to fix the soft ECC
 *	correction algorithm.
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
 *
 *	Changed mprintf's to log just one entry.
 */

#include "hp.h"
#if NHP > 0 || defined(BINARY)

#ifdef HPDEBUG
int	hpdebug;
#endif HPDEBUG
#ifdef HPBDEBUG
int	hpbdebug;
#endif HPBDEBUG

#include "../h/dump.h"
#include "../data/hp_data.c"

/*
 * Table for converting Massbus drive types into
 * indices into the partition tables.  Slots are
 * left for those drives defined from other means
 * (e.g. SI, AMPEX, etc.).
 */
short	hptypes[] = {
#define HPDT_RM03	0
	MBDT_RM03,
#define HPDT_RM05	1
	MBDT_RM05,
#define HPDT_RP06	2
	MBDT_RP06,
#define HPDT_RM80	3
	MBDT_RM80,
#define HPDT_RP04	4
	MBDT_RP04,
#define HPDT_RP05	5
	MBDT_RP05,
#define HPDT_RP07	6
	MBDT_RP07,
#define HPDT_ML11A	7
	MBDT_ML11A,
#define HPDT_ML11B	8
	MBDT_ML11B,
#define HPDT_9775	9
	-1,
#define HPDT_9730	10
	-1,
#define HPDT_CAPRICORN	11
	-1,
#define HPDT_EAGLE	12
	-1,
#define HPDT_9300	13
	-1,
#define HPDT_RM02	14
	MBDT_RM02,		/* beware, actually capricorn or eagle */
	0
};
int	hpattach(),hpustart(),hpstart(),hpdtint();
struct	mba_driver hpdriver =
	{ hpattach, 0, hpustart, hpstart, hpdtint, 0,
	  hptypes, "hp", 0, hpinfo };

/*
 * Beware, sdist and rdist are not well tuned
 * for many of the drives listed in this table.
 * Try patching things with something i/o intensive
 * running and watch iostat.
 */
struct hpst {
	short	nsect;		/* # sectors/track */
	short	ntrak;		/* # tracks/cylinder */
	short	nspc;		/* # sector/cylinders */
	short	ncyl;		/* # cylinders */
	struct	size *sizes;	/* partition tables */
	short	sdist;		/* seek distance metric */
	short	rdist;		/* rotational distance metric */
} hpst[] = {
	{ 32,	5,	32*5,	823,	rm03_sizes,	3, 4 }, /* RM03 */
	{ 32,	19,	32*19,	823,	rm05_sizes,	3, 4 }, /* RM05 */
	{ 22,	19,	22*19,	815,	rp06_sizes,	3, 4 }, /* RP06 */
	{ 31,	14,	31*14,	559,	rm80_sizes,	3, 4 }, /* RM80 */
	{ 22,	19,	22*19,	411,	rp05_sizes,	3, 4 }, /* RP04 */
	{ 22,	19,	22*19,	411,	rp05_sizes,	3, 4 }, /* RP05 */
	{ 50,	32,	50*32,	630,	rp07_sizes,	7, 8 }, /* RP07 */
	{ 1,	1,	1,	1,	0,		0, 0 }, /* ML11A */
	{ 1,	1,	1,	1,	0,		0, 0 }, /* ML11B */
	{ 32,	40,	32*40,	843,	cdc9775_sizes,	3, 4 }, /* 9775 */
	{ 32,	10,	32*10,	823,	cdc9730_sizes,	3, 4 }, /* 9730 */
	{ 32,	16,	32*16,	1024,	capricorn_sizes,7, 8 }, /* Capricorn */
	{ 48,	20,	48*20,	842,	eagle_sizes,	7, 8 }, /* EAGLE */
	{ 32,	19,	32*19,	815,	ampex_sizes,	3, 4 }, /* 9300 */
};

u_char	hp_offset[16] = {
    HPOF_P400, HPOF_M400, HPOF_P400, HPOF_M400,
    HPOF_P800, HPOF_M800, HPOF_P800, HPOF_M800,
    HPOF_P1200, HPOF_M1200, HPOF_P1200, HPOF_M1200,
    0, 0, 0, 0,
};


#define b_cylin b_resid

/* #define ML11 0  to remove ML11 support */
#define ML11	(hptypes[mi->mi_type] == MBDT_ML11A)
#define RP06	(hptypes[mi->mi_type] <= MBDT_RP06)
#define RM80	(hptypes[mi->mi_type] == MBDT_RM80)

#define MASKREG(reg)	((reg)&0xffff)

#ifdef INTRLVE
daddr_t dkblock();
#endif INTRLVE

/*ARGSUSED*/
hpattach(mi)
	register struct mba_device *mi;
{
	register struct hp_softc *sc = &hp_softc[mi->mi_unit];

	mi->mi_type = hpmaptype(mi);
	if (!ML11 && mi->mi_dk >= 0) {
		struct hpst *st = &hpst[mi->mi_type];

		dk_mspw[mi->mi_dk] = 1.0 / 60 / (st->nsect * 256);
		sc->sc_softcnt = 0;
		sc->sc_hardcnt = 0;

		switch (*(mi->mi_driver->md_type + mi->mi_type)) {

		case MBDT_RM03:
			bcopy(DEV_RM03,sc->sc_device,
			      strlen(DEV_RM03));
			break;

		case MBDT_RM05:
			bcopy(DEV_RM05,sc->sc_device,
			      strlen(DEV_RM05));
			break;

		case MBDT_RM80:
			bcopy(DEV_RM80,sc->sc_device,
			      strlen(DEV_RM80));
			break;

		case MBDT_RP05:
			bcopy(DEV_RP05,sc->sc_device,
			      strlen(DEV_RP05));
			break;

		case MBDT_RP06:
			bcopy(DEV_RP06,sc->sc_device,
			      strlen(DEV_RP06));
			break;

		case MBDT_RP07:
			bcopy(DEV_RP07,sc->sc_device,
			      strlen(DEV_RP07));
			break;

		default:
			bcopy(DEV_UNKNOWN,sc->sc_device,
			      strlen(DEV_UNKNOWN));
			break;
		}
	}
}

/*
 * Map apparent MASSBUS drive type into manufacturer
 * specific configuration.  For SI controllers this is done
 * based on codes in the serial number register.  For
 * EMULEX controllers, the track and sector attributes are
 * used when the drive type is an RM02 (not supported by DEC).
 */
hpmaptype(mi)
	register struct mba_device *mi;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register int type = mi->mi_type;

	/*
	 * Model-byte processing for SI controllers.
	 * NB:	Only deals with RM03 and RM05 emulations.
	 */
	if (type == HPDT_RM03 || type == HPDT_RM05) {
		int hpsn = hpaddr->hpsn;

		if ((hpsn & SIMB_LU) != mi->mi_drive)
			return (type);
		switch ((hpsn & SIMB_MB) & ~(SIMB_S6|SIRM03|SIRM05)) {

		case SI9775D:
			printf("hp%d: 9775 (direct)\n", mi->mi_unit);
			type = HPDT_9775;
			break;

		case SI9730D:
			printf("hp%d: 9730 (direct)\n", mi->mi_unit);
			type = HPDT_9730;
			break;

		/*
		 * Beware, since the only SI controller we
		 * have has a 9300 instead of a 9766, we map the
		 * drive type into the 9300.  This means that
		 * on a 9766 you lose the last 8 cylinders (argh).
		 */
		case SI9766:
			printf("hp%d: 9300\n", mi->mi_unit);
			type = HPDT_9300;
			break;

		case SI9762:
			printf("hp%d: 9762\n", mi->mi_unit);
			type = HPDT_RM03;
			break;

		case SICAPD:
			printf("hp%d: capricorn\n", mi->mi_unit);
			type = HPDT_CAPRICORN;
			break;

		case SI9751D:
			printf("hp%d: eagle\n", mi->mi_unit);
			type = HPDT_EAGLE;
			break;
		}
		return (type);
	}

	/*
	 * EMULEX SC750 or SC780.  Poke the holding register.
	 */
	if (type == HPDT_RM02) {
		int ntracks, nsectors;

		hpaddr->hpof = HPOF_FMT22;
		mbclrattn(mi);
		hpaddr->hpcs1 = HP_NOP;
		hpaddr->hphr = HPHR_MAXTRAK;
		ntracks = MASKREG(hpaddr->hphr) + 1;
		if (ntracks == 16) {
			printf("hp%d: capricorn\n", mi->mi_unit);
			type = HPDT_CAPRICORN;
			goto done;
		}
		if (ntracks == 19) {
			printf("hp%d: 9300\n", mi->mi_unit);
			type = HPDT_9300;
			goto done;
		}
		hpaddr->hpcs1 = HP_NOP;
		hpaddr->hphr = HPHR_MAXSECT;
		nsectors = MASKREG(hpaddr->hphr) + 1;
		if (ntracks == 20 && nsectors == 48) {
			type = HPDT_EAGLE;
			printf("hp%d: eagle\n", mi->mi_unit);
			goto done;
		}
		printf("hp%d: ntracks %d, nsectors %d: unknown device\n",
			mi->mi_unit, ntracks, nsectors);
done:
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		mbclrattn(mi);		/* conservative */
		return (type);
	}

	/*
	 * Map all ML11's to the same type.  Also calculate
	 * transfer rates based on device characteristics.
	 */
	if (type == HPDT_ML11A || type == HPDT_ML11B) {
		register struct hp_softc *sc = &hp_softc[mi->mi_unit];
		register int trt;

		sc->sc_mlsize = hpaddr->hpmr & HPMR_SZ;
		if ((hpaddr->hpmr & HPMR_ARRTYP) == 0)
			sc->sc_mlsize >>= 2;
		if (mi->mi_dk >= 0) {
			trt = (hpaddr->hpmr & HPMR_TRT) >> 8;
			dk_mspw[mi->mi_dk] = 1.0 / (1<<(20-trt));
		}
		type = HPDT_ML11A;
	}
	return (type);
}

hpopen(dev, flag)
	register dev_t dev;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct mba_device *mi;
	register struct hp_softc *sc;
	register struct hpdevice *hpaddr;

	if (unit >= nNHP || (mi = hpinfo[unit]) == 0 ||
	    mi->mi_alive == 0) {
		return (ENXIO);
	}

	hpaddr = (struct hpdevice *)mi->mi_drv;
	sc = &hp_softc[unit];
	sc->sc_flags = 0;
	sc->sc_category_flags = 0;

	if (!(hpaddr->hpds & HPDS_MOL)) {
		sc->sc_flags |= DEV_OFFLINE;
	}

	if (hpaddr->hpds & HPDS_WRL) {
		sc->sc_flags |= DEV_WRTLCK;
	}

	/*
	 *	See if we need to read in the partition table from the disk.
	 *	The conditions we will have to read from the disk is if the
	 *	partition table valid bit has not been set or the volume is
	 *	in valid
	 */
	if (((hpaddr->hpds & HPDS_VV) == 0 ) || (hp_part[unit].pt_valid == 0)){
		/*
		 *	Assume that the default values before trying to
		 *	see if the partition tables are on the pack. The
		 *	reason that we do this is that the strategy routine
		 *	is used to read in the superblock but uses the
		 *	partition info. So we must first set the default
		 *	values.
		 *
		 */
		int nspc = hpst[mi->mi_type].nspc;
		int i, hpstrategy();			/* 001 */

		for ( i = 0; i <= 7; i++) {
			hp_part[unit].pt_part[i].pi_nblocks =
			hpst[mi->mi_type].sizes[i].nblocks;

			hp_part[unit].pt_part[i].pi_blkoff =
			hpst[mi->mi_type].sizes[i].cyloff * nspc;

			hp_part[unit].pt_valid = PT_VALID;
		}

		/*
		 *	Go see if the partition tables are on the pack
		 */

		rsblk( hpstrategy, dev, &hp_part[unit]);
	}
	return (0);
}

hpstrategy(bp)
	register struct buf *bp;
{
	register struct mba_device *mi;
	register struct hpst *st;
	register int unit = dkunit(bp);
	register struct pt *pt;
	register struct hp_softc *sc = &hp_softc[unit];
	long sz, bn;
	int xunit = minor(bp->b_dev) & 07;
	int s;

	sz = bp->b_bcount;
	sz = (sz+511) >> 9;
	if (unit >= nNHP)
		goto bad;
	mi = hpinfo[unit];
	/*
	 *	Get the partition tables for the pack in question
	 */

	pt = &hp_part[unit];
	if ( ( pt->pt_valid != PT_VALID ) )
		panic("hpstrategy: invalid partition table ");
	if (mi == 0 || mi->mi_alive == 0)
		goto bad;
	st = &hpst[mi->mi_type];
	if (ML11) {
		if (bp->b_blkno < 0 ||
		    dkblock(bp)+sz > sc->sc_mlsize) {
			sc->sc_flags |= DEV_EOM;
			goto bad;
		}
		bp->b_cylin = 0;
	} else {
		if (bp->b_blkno < 0 ||
		    (bn = dkblock(bp))+sz > pt->pt_part[xunit].pi_nblocks) {
			sc->sc_flags |= DEV_EOM;
			goto bad;
		}
		/*
		 *	Must convert blkoff to cylinder
		 */
		bp->b_cylin = bn/st->nspc + pt->pt_part[xunit].pi_blkoff /
					 st->nspc;
	}
	s = spl5();
	disksort(&mi->mi_tab, bp);
	if (mi->mi_tab.b_active == 0)
		mbustart(mi);
	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
	if ((sc->sc_flags & DEV_EOM)) {
		bp->b_error = ENOSPC;
	}
	iodone(bp);
	return;
}

hpustart(mi)
	register struct mba_device *mi;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st;
	register struct hp_softc *sc = &hp_softc[mi->mi_unit];
	register daddr_t bn;
	int sn, dist;

	st = &hpst[mi->mi_type];
	hpaddr->hpcs1 = 0;
	if ((hpaddr->hpcs1&HP_DVA) == 0)
		return (MBU_BUSY);
	if ((hpaddr->hpds & HPDS_VV) == 0 || !sc->sc_hpinit) {
		struct buf *bbp = &bhpbuf[mi->mi_unit];

		sc->sc_hpinit = 1;
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		if (mi->mi_mba->mba_drv[0].mbd_as & (1<<mi->mi_drive))
			printf("%s: unit# %d: DCLR attn\n",sc->sc_device,
			       mi->mi_unit);
		hpaddr->hpcs1 = HP_PRESET|HP_GO;
		if (!ML11)
			hpaddr->hpof = HPOF_FMT22;
		mbclrattn(mi);
		if (!ML11) {
			bbp->b_flags = B_READ|B_BUSY;
			bbp->b_dev = bp->b_dev;
			bbp->b_bcount = 512;
			bbp->b_un.b_addr = (caddr_t)&hpbad[mi->mi_unit];
			bbp->b_blkno = st->ncyl*st->nspc - st->nsect;
			bbp->b_cylin = st->ncyl - 1;
			mi->mi_tab.b_actf = bbp;
			bbp->av_forw = bp;
			bp = bbp;
		}
	}
	if (mi->mi_tab.b_active || mi->mi_hd->mh_ndrive == 1)
		return (MBU_DODATA);
	if (ML11)
		return (MBU_DODATA);
	if ((hpaddr->hpds & HPDS_DREADY) != HPDS_DREADY)
		return (MBU_DODATA);
	bn = dkblock(bp);
	sn = bn%st->nspc;
	sn = (sn + st->nsect - st->sdist) % st->nsect;
	if (bp->b_cylin == MASKREG(hpaddr->hpdc)) {
		if (sc->sc_doseeks)
			return (MBU_DODATA);
		dist = (MASKREG(hpaddr->hpla) >> 6) - st->nsect + 1;
		if (dist < 0)
			dist += st->nsect;
		if (dist > st->nsect - st->rdist)
			return (MBU_DODATA);
	} else
		hpaddr->hpdc = bp->b_cylin;
	if (sc->sc_doseeks)
		hpaddr->hpcs1 = HP_SEEK|HP_GO;
	else {
		hpaddr->hpda = sn;
		hpaddr->hpcs1 = HP_SEARCH|HP_GO;
	}
	return (MBU_STARTED);
}

hpstart(mi)
	register struct mba_device *mi;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st = &hpst[mi->mi_type];
	register struct hp_softc *sc = &hp_softc[mi->mi_unit];
	register daddr_t bn;
	int sn, tn;

	bn = dkblock(bp);
	if (ML11)
		hpaddr->hpda = bn;
	else {
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn %= st->nsect;
		hpaddr->hpdc = bp->b_cylin;
		hpaddr->hpda = (tn << 8) + sn;
	}
	if (sc->sc_hdr) {
		if (bp->b_flags & B_READ)
			return (HP_RHDR|HP_GO);
		else
			return (HP_WHDR|HP_GO);
	}
	return (0);
}

hpdtint(mi, mbsr)
	register struct mba_device *mi;
	int mbsr;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st;
	register int er1, er2;
	register struct hp_softc *sc = &hp_softc[mi->mi_unit];
	int retry = 0;

	st = &hpst[mi->mi_type];
	if (bp->b_flags&B_BAD && hpecc(mi, CONT))
		return (MBD_RESTARTED);
	if (hpaddr->hpds&HPDS_ERR || mbsr&MBSR_EBITS) {
		er1 = hpaddr->hper1;
		er2 = hpaddr->hper2;
#ifdef HPDEBUG
		if (hpdebug) {
			int dc = hpaddr->hpdc, da = hpaddr->hpda;

			printf("hperr: bp %x cyl %d blk %d as %o ",
				bp, bp->b_cylin, bp->b_blkno,
				hpaddr->hpas&0xff);
			printf("dc %x da %x\n",MASKREG(dc), MASKREG(da));
			printf("errcnt %d ", mi->mi_tab.b_errcnt);
			printf("mbsr=%b ", mbsr, mbsr_bits);
			printf("er1=%b er2=%b\n", MASKREG(er1), HPER1_BITS,
			    MASKREG(er2), HPER2_BITS);
			DELAY(1000000);
		}
#endif
		if (er1 & HPER1_HCRC) {
			er1 &= ~(HPER1_HCE|HPER1_FER);
			er2 &= ~HPER2_BSE;
		}
		if (er1&HPER1_WLE) {
			sc->sc_flags |= DEV_WRTLCK;
			bp->b_flags |= B_ERROR;
		} else if (MASKREG(er1) == HPER1_FER && RP06 && !sc->sc_hdr) {
			if (hpecc(mi, BSE))
				return (MBD_RESTARTED);
			goto hard;
		} else if (++mi->mi_tab.b_errcnt > 27 ||
		    mbsr & MBSR_HARD ||
		    er1 & HPER1_HARD ||
		    sc->sc_hdr ||
		    (!ML11 && (er2 & HPER2_HARD))) {
			/*
			 * HCRC means the header is screwed up and the sector
			 * might well exist in the bad sector table,
			 * better check....
			 */
			if ((er1&HPER1_HCRC) &&
			    !ML11 && !sc->sc_hdr && hpecc(mi, BSE))
				return (MBD_RESTARTED);
hard:
			if (ML11)
				bp->b_blkno = MASKREG(hpaddr->hpda);
			else
				bp->b_blkno = MASKREG(hpaddr->hpdc) * st->nspc +
				   (MASKREG(hpaddr->hpda) >> 8) * st->nsect +
				   (hpaddr->hpda&0xff);
			/*
			 * If we have a data check error or a hard
			 * ecc error the bad sector has been read/written,
			 * and the controller registers are pointing to
			 * the next sector...
			 */
			if (er1&(HPER1_DCK|HPER1_ECH) || sc->sc_hdr)
				bp->b_blkno--;
			harderr(bp, "hp");
			sc->sc_hardcnt++;
			sc->sc_flags |= DEV_HARDERR;

			/* Do an hex dump of interesting registers
			 * for the hard disk error.  First, print a row of
			 * appropriate register names followed by a row of
			 * register contents lined up below the header line
			 * (Hopefully).  Bit expansions are not practical for
			 * this type of dump, using the Kernel printf.
			 */

			switch ((hpaddr -> hpdt) & 0x1ff)
			{
			case MBDT_RP04:
			case MBDT_RP05:
			case MBDT_RP06:
			case MBDT_RP07:
			    mprintf("%s: unit#:%d hard err blk#:%d \
				mbsr:%b rpcs1:%x rpds:%x rper1:%x rpmr:%x \
				rpas:%x rpda:%x rpdt:%x rpla:%x rpsn:%x \
				rpof:%x rpdc:%x rpcc:%x rper2:%x rper3:%x \
				rpec1:%x rpec2:%x\n",
				        sc->sc_device, mi->mi_unit,
					bp->b_blkno, mbsr, mbsr_bits,
					MASKREG(hpaddr->hpcs1),
					MASKREG(hpaddr->hpds),
					MASKREG(hpaddr->hper1),
					MASKREG(hpaddr->hpmr),
					MASKREG(hpaddr->hpas),
					MASKREG(hpaddr->hpda),
					MASKREG(hpaddr->hpdt),
					MASKREG(hpaddr->hpla),
					MASKREG(hpaddr->hpsn),
					MASKREG(hpaddr->hpof),
					MASKREG(hpaddr->hpdc),
					MASKREG(hpaddr->hpcc),
					MASKREG(hpaddr->hpmr2),
					MASKREG(hpaddr->hper2),
					MASKREG(hpaddr->hpec1),
					MASKREG(hpaddr->hpec2));
				break;

			case MBDT_RM03:
			case MBDT_RM05:
			case MBDT_RM80:
			    mprintf("%s: unit#:%d hard err blk#:%d \
				mbsr:%b rmcs1:%x rmds:%x rmer1:%x rmmr1:%x \
				rmas:%x rmda:%x rmdt:%x rmla:%x rmsn:%x \
				rmof:%x rmdc:%x rmhr:%x rmmr2:%x rmer2:%x \
				rmec1:%x rmec2:%x\n",
				        sc->sc_device, mi->mi_unit,
					bp->b_blkno, mbsr, mbsr_bits,
					MASKREG(hpaddr->hpcs1),
					MASKREG(hpaddr->hpds),
					MASKREG(hpaddr->hper1),
					MASKREG(hpaddr->hpmr),
					MASKREG(hpaddr->hpas),
					MASKREG(hpaddr->hpda),
					MASKREG(hpaddr->hpdt),
					MASKREG(hpaddr->hpla),
					MASKREG(hpaddr->hpsn),
					MASKREG(hpaddr->hpof),
					MASKREG(hpaddr->hpdc),
					MASKREG(hpaddr->hpcc),
					MASKREG(hpaddr->hpmr2),
					MASKREG(hpaddr->hper2),
					MASKREG(hpaddr->hpec1),
					MASKREG(hpaddr->hpec2));
				break;

			case MBDT_ML11A:
			case MBDT_ML11B:
			    mprintf("%s: unit#:%d hard err blk#:%d \
				mbsr:%b mlcs1:%x mlds:%x mler:%x mlmr:%x \
				mlas:%x mlda:%x mldt:%x mlpa:%x mlsn:%x \
				mle1:%x mle2:%x mld1:%x mld2:%x mlee:%x \
				mlel:%x mlpd:%x\n",
				        sc->sc_device, mi->mi_unit,
					bp->b_blkno, mbsr, mbsr_bits,
					MASKREG(hpaddr->hpcs1),
					MASKREG(hpaddr->hpds),
					MASKREG(hpaddr->hper1),
					MASKREG(hpaddr->hpmr),
					MASKREG(hpaddr->hpas),
					MASKREG(hpaddr->hpda),
					MASKREG(hpaddr->hpdt),
					MASKREG(hpaddr->hpla),
					MASKREG(hpaddr->hpsn),
					MASKREG(hpaddr->hpof),
					MASKREG(hpaddr->hpdc),
					MASKREG(hpaddr->hpcc),
					MASKREG(hpaddr->hpmr2),
					MASKREG(hpaddr->hper2),
					MASKREG(hpaddr->hpec1),
					MASKREG(hpaddr->hpec2));
				break;

			default:
				/*
				 * Original Error handler (Maintained for all
				 * of the unsupported devices) modified to use 
				 * mprintf
				 */
			        mprintf("%s: unit#:%d hard err blk#:%d \
				    mbsr:%b er1:%b er2:%b mr:%x mr2:%x\n",
				    sc->sc_device, mi->mi_unit,
				    bp->b_blkno, mbsr, mbsr_bits,
				    MASKREG(hpaddr->hper1), HPER1_BITS,
				    MASKREG(hpaddr->hper2), HPER2_BITS,
				    MASKREG(hpaddr->hpmr),
				    MASKREG(hpaddr->hpmr2));
				if (sc->sc_hdr)
					mprintf("(hdr i/o)\n");

				/* End of Original Error Handler */

				break;
			}
			bp->b_flags |= B_ERROR;
			retry = 0;
			sc->sc_recal = 0;
		} else if ((er2 & HPER2_BSE) && !ML11) {
			if (hpecc(mi, BSE))
				return (MBD_RESTARTED);
			goto hard;
		} else if (RM80 && er2&HPER2_SSE) {
			(void) hpecc(mi, SSE);
			return (MBD_RESTARTED);
		} else if ((er1&(HPER1_DCK|HPER1_ECH))==HPER1_DCK) {
			if (hpecc(mi, ECC))
				return (MBD_RESTARTED);
			/* else done */
		} else
			retry = 1;
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		if (ML11) {
			if (mi->mi_tab.b_errcnt >= 16)
				goto hard;
		} else if ((mi->mi_tab.b_errcnt&07) == 4) {
			hpaddr->hpcs1 = HP_RECAL|HP_GO;
			sc->sc_recal = 1;
			return (MBD_RESTARTED);
		}
		if (retry) {
			sc->sc_flags |= DEV_RETRY;
			return (MBD_RETRY);
		}
	}
#ifdef HPDEBUG
	else
		if (hpdebug && sc->sc_recal) {
			printf("recal %d ", sc->sc_recal);
			printf("errcnt %d\n", mi->mi_tab.b_errcnt);
			printf("mbsr=%b ", mbsr, mbsr_bits);
			printf("er1=%b er2=%b\n",
			    hpaddr->hper1, HPER1_BITS,
			    hpaddr->hper2, HPER2_BITS);
		}
#endif
	switch (sc->sc_recal) {

	case 1:
		hpaddr->hpdc = bp->b_cylin;
		hpaddr->hpcs1 = HP_SEEK|HP_GO;
		sc->sc_recal++;
		return (MBD_RESTARTED);
	case 2:
		if (mi->mi_tab.b_errcnt < 16 ||
		    (bp->b_flags & B_READ) == 0)
			goto donerecal;
		hpaddr->hpof = hp_offset[mi->mi_tab.b_errcnt & 017]|HPOF_FMT22;
		hpaddr->hpcs1 = HP_OFFSET|HP_GO;
		sc->sc_recal++;
		return (MBD_RESTARTED);
	donerecal:
	case 3:
		sc->sc_recal = 0;
		sc->sc_flags |= DEV_RETRY;
		return (MBD_RETRY);
	}
	sc->sc_hdr = 0;
	bp->b_resid = MASKREG(-mi->mi_mba->mba_bcr);
	if (mi->mi_tab.b_errcnt >= 16) {
		/*
		 * This is fast and occurs rarely; we don't
		 * bother with interrupts.
		 */
		hpaddr->hpcs1 = HP_RTC|HP_GO;
		while (hpaddr->hpds & HPDS_PIP)
			;
		mbclrattn(mi);
	}
	if (!ML11) {
		hpaddr->hpof = HPOF_FMT22;
		hpaddr->hpcs1 = HP_RELEASE|HP_GO;
	}
	sc->sc_flags |= DEV_DONE;
	return (MBD_DONE);
}

hpread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(hpstrategy, &rhpbuf[unit], dev, B_READ, minphys, uio));
}

hpwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	return (physio(hpstrategy, &rhpbuf[unit], dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
hpioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct mba_device *mi = hpinfo[unit];
	register struct pt *pt = (struct pt *)data;
	register struct hp_softc *sc = &hp_softc[unit];

	struct dkop *dkop;
	struct dkget *dkget;
	struct devget *devget;
	register int i;
	int nspc;
	int error;

	switch (cmd) {

	case DKIOCHDR:	/* do header read/write */
		sc->sc_hdr = 1;
		break;

	case DIOCGETPT: /* 001 get partition table info */
		/*
		 *	Do a structure copy into the user's data area
		 */

		*pt = hp_part[unit];
		break;

	case DIOCDGTPT: /* Get default partition table */
		/*
		 *	Get number of sectors per cylinder
		 */
		nspc = hpst[mi->mi_type].nspc;

		/*
		 *	Get the partition size and offset
		 */
		for ( i = 0; i <= 7; i++) {
			pt->pt_part[i].pi_nblocks =
			hpst[mi->mi_type].sizes[i].nblocks;

			pt->pt_part[i].pi_blkoff =
			hpst[mi->mi_type].sizes[i].cyloff * nspc;

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
		if ( ( error = ptcmp( dev, &hp_part[unit], pt ) ) != 0 )
			return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */

		hp_part[unit] = *pt;

		/*
		 *	See if we need to update the superblock of the
		 *	"a" partition of this disk
		 */
		ssblk(dev,pt);

		/*
		 *	Just make sure that we set the valid bit
		 */

		hp_part[unit].pt_valid = PT_VALID;
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
		devget->bus = DEV_MB;			/* Massbus	*/
		bcopy(DEV_RH,devget->interface,
		      strlen(DEV_RH));			/* RH???	*/
		bcopy(sc->sc_device,devget->device,
		      strlen(sc->sc_device));		/* R[MP]??	*/
		devget->adpt_num = mi->mi_adpt; 	/* which adapter*/
		devget->nexus_num = mi->mi_nexus;	/* which nexus	*/
		devget->bus_num = mi->mi_mbanum;	/* which MBA	*/
		devget->ctlr_num = mi->mi_mbanum;	/* which RH	*/
		devget->slave_num = mi->mi_drive;	/* which plug	*/
		bcopy(mi->mi_driver->md_dname,
		      devget->dev_name,
		      strlen(mi->mi_driver->md_dname)); /* Ultrix "hp"	*/
		devget->unit_num = unit;		/* which hp??	*/
		devget->soft_count = sc->sc_softcnt;	/* soft er. cnt.*/
		devget->hard_count = sc->sc_hardcnt;	/* hard er. cnt.*/
		devget->stat = sc->sc_flags;		/* status	*/
		devget->category_stat = DEV_DISKPART;	/* which prtn.	*/
		break;

	default:
		return (ENXIO);
	}
	return(0);
}

hpecc(mi, flag)
	register struct mba_device *mi;
	int flag;
{
	register struct mba_regs *mbp = mi->mi_mba;
	register struct hpdevice *rp = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st = &hpst[mi->mi_type];
	int npf, o;
	int bn, cn, tn, sn;
	int bcr;

#ifdef HPDEBUG

	if(hpdebug) {
		printf("b_flags = 0x%x  ", bp->b_flags);
		printf("mba_bcr = 0x%x  ", mbp->mba_bcr);
	}
#endif HPDEBUG


	bcr = (mbp->mba_bcr);
	/*
	 * On a read, the most conservative half of the byte count
	 * register is the SBI transfer half (lower 16 bits).
	 * On a write, the most conservative half of the byte count
	 * register is the Massbus half (upper 16 bits).
	 *
	 * If the transfer is not a read, it is most likely
	 * a write.
	 */
	if ((bp->b_flags & B_READ) != B_READ)
		bcr = (bcr >> 16);
	bcr = MASKREG(bcr);
	if (bcr)
		bcr |= 0xffff0000;		/* sxt */
	if (flag == CONT)
		npf = bp->b_error;
	else
		npf = btop(bcr + bp->b_bcount);

#ifdef HPDEBUG
	if(hpdebug)
		printf("npf = %d\n", npf);
#endif HPDEBUG

	o = (int)bp->b_un.b_addr & PGOFSET;
	bn = dkblock(bp);
	cn = bp->b_cylin;
	sn = bn%(st->nspc) + npf;
	tn = sn/st->nsect;
	sn %= st->nsect;
	cn += tn/st->ntrak;
	tn %= st->ntrak;
	switch (flag) {
	case ECC: {
		register int i;
		caddr_t addr;
		struct pte mpte;
		int bit, byte, mask;

		npf--;			/* error was in the previous block*/

		mprintf("hp%d%c: soft ecc sn:%d cyl:%d trk:%d sect:%d\n",
		    dkunit(bp), 'a'+(minor(bp->b_dev)&07), bp->b_blkno + npf,
		    MASKREG(rp->hpdc), ((MASKREG(rp->hpda) >> 8) & 0xff),
		    (MASKREG(rp->hpda) & 0xff));

		mask = MASKREG(rp->hpec2);
		i = MASKREG(rp->hpec1) - 1;		/* -1 makes 0 origin */
		bit = i&07;
		i = (i&~07)>>3;
		byte = i + o;
		while (i < 512 && (int)ptob(npf)+i < bp->b_bcount && bit > -11) {
			mpte = mbp->mba_map[npf+btop(byte)];
			addr = ptob(mpte.pg_pfnum) + (byte & PGOFSET);
			putmemc(addr, getmemc(addr)^(mask<<bit));
			byte++;
			i++;
			bit -= 8;
		}
		if (bcr == 0)
			return (0);
		npf++;
		break;
		}

	case SSE:
		rp->hpof |= HPOF_SSEI;
		mbp->mba_bcr = -(bp->b_bcount - (int)ptob(npf));
		break;

	case BSE:
#ifdef HPBDEBUG
		if (hpbdebug)
		printf("hpecc, BSE: bn %d cn %d tn %d sn %d\n", bn, cn, tn, sn);
#endif
		if (rp->hpof&HPOF_SSEI)
			sn++;
		if ((bn = isbad(&hpbad[mi->mi_unit], cn, tn, sn)) < 0)
			return (0);
		bp->b_flags |= B_BAD;
		bp->b_error = npf + 1;
		bn = st->ncyl*st->nspc - st->nsect - 1 - bn;
		cn = bn/st->nspc;
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn %= st->nsect;
		mbp->mba_bcr = -MIN(512, bp->b_bcount - (int)ptob(npf));
		rp->hpof &= ~HPOF_SSEI;
#ifdef HPBDEBUG
		if (hpbdebug)
		printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
#endif
		break;

	case CONT:
#ifdef HPBDEBUG
		if (hpbdebug)
		printf("hpecc, CONT: bn %d cn %d tn %d sn %d\n", bn,cn,tn,sn);
#endif
		npf = bp->b_error;
		bp->b_flags &= ~B_BAD;
		mbp->mba_bcr = -(bp->b_bcount - (int)ptob(npf));
		if (MASKREG(mbp->mba_bcr) == 0)
			return (0);
		break;
	}
	rp->hpcs1 = HP_DCLR|HP_GO;
	if (rp->hpof&HPOF_SSEI)
		sn++;
	rp->hpdc = cn;
	rp->hpda = (tn<<8) + sn;
	mbp->mba_sr = -1;
	mbp->mba_var = (int)ptob(npf) + o;
	rp->hpcs1 = bp->b_flags&B_READ ? HP_RCOM|HP_GO : HP_WCOM|HP_GO;
	mi->mi_tab.b_errcnt = 0;	/* error has been corrected */
	return (1);
}

#define DBSIZE	20

hpdump(dev, dumpinfo)
	dev_t dev;			/* dump device */
	struct dumpinfo dumpinfo;	/* dump info */
{
#ifdef notdef
	register struct mba_device *mi;
	register struct mba_regs *mba;
	struct hpdevice *hpaddr;
	char *start;
	char *start_tmp;
	int unit;
	register struct hpst *st;

	start = start_tmp = 0;
	unit = minor(dev) >> 3;
	if (unit >= nNHP)
		return (ENXIO);
#define phys(a,b)	((b)((int)(a)&0x7fffffff))
	mi = phys(hpinfo[unit],struct mba_device *);
	if (mi == 0 || mi->mi_alive == 0)
		return (ENXIO);
	mba = phys(mi->mi_hd, struct mba_hd *)->mh_physmba;
	mba->mba_cr = MBCR_INIT;
	hpaddr = (struct hpdevice *)&mba->mba_drv[mi->mi_drive];
	if ((hpaddr->hpds & HPDS_VV) == 0) {
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		hpaddr->hpcs1 = HP_PRESET|HP_GO;
		hpaddr->hpof = HPOF_FMT22;
	}
	st = &hpst[mi->mi_type];

	/*
	 * If a full dump is being performed, then this loop
	 * will dump all of core. If a partial dump is being
	 * performed, then as much of core as possible will be
	 * dumped, leaving room for the u_area and error logger
	 * buffer. Please note that dumpsys predetermined what
	 * type of dump will be performed.
	 */

	while ((dumpinfo.size_to_dump > 0) || (dumpinfo.partial_dump)) {
		register struct pte *hpte = mba->mba_map;
		register int i;
		int blk, cn, sn, tn;
		daddr_t bn;

		blk = dumpinfo.size_to_dump > DBSIZE ? DBSIZE : dumpinfo.size_to_dump;
		bn = dumplo + btop(start_tmp);
		cn = bn/st->nspc + dumpinfo.blkoffs / st->nspc;
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn = sn%st->nsect;
		hpaddr->hpdc = cn;
		hpaddr->hpda = (tn << 8) + sn;
		for (i = 0; i < blk; i++)
			*(int *)hpte++ = (btop(start)+i) | PG_V;
		mba->mba_sr = -1;
		mba->mba_bcr = -(blk*NBPG);
		mba->mba_var = 0;
		hpaddr->hpcs1 = HP_WCOM | HP_GO;
		while ((hpaddr->hpds & HPDS_DRY) == 0)
			;
		if (hpaddr->hpds&HPDS_ERR)
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

hpsize(dev)
	register dev_t dev;
{
	register int unit = minor(dev) >> 3;
	register struct mba_device *mi;

	if (unit >= nNHP || (mi = hpinfo[unit]) == 0 || mi->mi_alive == 0)
		return (-1);
	/*
	 *	Sanity check		001
	 */
	if ( hp_part[unit].pt_valid != PT_VALID )
		panic("hpsize: invalid partition table ");

	return ((int)hp_part[unit].pt_part[minor(dev) & 07].pi_nblocks);/*001*/
}
#endif
