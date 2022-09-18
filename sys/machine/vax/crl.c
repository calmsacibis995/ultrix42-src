/*
 * VAX8600 console rl disk drvier - crl.c
 */

#ifndef lint
static	char	*sccsid = "@(#)crl.c	4.1 (ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Modification History:
 *
 * 30-May-89	darrell
 *	Added include of ../../machine/common/cpuconf.h -- cpu types
 *	were moved there.
 *
 * 27-Feb-85 -tresvik
 *	Support of the VAX8600.
 *
 */
#if VAX8600
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/uio.h"

#include "../vax/cons.h"
#include "../vax/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../vax/crl.h"
#include "../vax/mtpr.h"

struct {
	short	crl_state;		/* open and busy flags */
	short	crl_active;		/* driver state flag */
	struct	buf *crl_buf;		/* buffer we're using */
	unsigned short *crl_xaddr;	/* transfer address */
	short	crl_errcnt;
} crltab;
struct {
	unsigned int	crl_cs;		/* controller status */
	unsigned int	crl_ds;		/* drive status */
} crlstat;

/*ARGSUSED*/
crlopen(dev, flag)
	dev_t dev;
	int flag;
{
	struct buf *geteblk();

	if (cpu != VAX_8600)
		return (ENODEV);
	if (crltab.crl_state != 0)
		return (EALREADY);
	crltab.crl_state = CRL_OPEN;
	crltab.crl_buf = geteblk(512);
	return (0);
}

/*ARGSUSED*/
crlclose(dev, flag)
	dev_t dev;
	int flag;
{

	brelse(crltab.crl_buf);
	crltab.crl_state = 0;
}

crloperation(rw, uio)
	enum uio_rw rw;
	struct uio *uio;
{
	register struct buf *bp;
	register int i;
	int error;

	/*
	 * Assume one block read/written for each call - 
	 * and enforce this by checking for block size of 512.
	 */
	if (uio->uio_resid == 0) 
		return (0);
	(void) spl4();
	while (crltab.crl_state & CRL_BUSY)
		sleep((caddr_t)&crltab, PRIBIO);
	crltab.crl_state |= CRL_BUSY;
	(void) spl0();

	bp = crltab.crl_buf;
	error = 0;
	while ((i = imin(512, uio->uio_resid)) > 0) {
		bp->b_blkno = uio->uio_offset >> 9;
		if (bp->b_blkno >= MAXSEC || (uio->uio_offset & 0777) != 0)
			return (EIO);
		if (rw == UIO_WRITE) {
			error = uiomove(bp->b_un.b_addr, i, UIO_WRITE, uio);
			if (error)
				break;
		}
		bp->b_flags = rw == UIO_WRITE ? B_WRITE : B_READ;
		(void) spl4(); 
		crlstart();
		while ((bp->b_flags & B_DONE) == 0)
			sleep((caddr_t)bp, PRIBIO);	
		(void) spl0();
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			break;
		}
		if (rw == UIO_READ) {
			error = uiomove(bp->b_un.b_addr, i, UIO_READ, uio);
			if (error)
				break;
		}
	}
	crltab.crl_state &= ~CRL_BUSY;
	wakeup((caddr_t)&crltab);
	return (error);
}

/*ARGSUSED*/
crlread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (crloperation(UIO_READ, uio));
}

/*ARGSUSED*/
crlwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (crloperation(UIO_WRITE, uio));
}

crlstart()
{
	register struct buf *bp;

	bp = crltab.crl_buf;
	crltab.crl_errcnt = 0;
	crltab.crl_xaddr = (unsigned short *) bp->b_un.b_addr;
	bp->b_resid = 0;
	bp->b_bcount = CRLBYSEC; /* always transfer a full sector */

	if ((mfpr(STXCS) & STXCS_RDY) == 0)
		/* not ready to receive order */
		return;
	/*
	 * Wake up rl02 console software with command
	 */
	if ((bp->b_flags&B_READ) == B_READ) {
		crltab.crl_active = CRL_F_READ;
		mtpr(STXCS, ((bp->b_blkno<<8) | STXCS_IE | CRL_F_READ));
	} else {
		crltab.crl_active = CRL_F_WRITE;
		mtpr(STXCS, ((bp->b_blkno<<8) | STXCS_IE | CRL_F_WRITE));
	}
}
crlintr()
{
	register struct buf *bp;
	unsigned int sts;
	bp = crltab.crl_buf;

	sts = mfpr(STXCS);
	switch (sts >> 24) {
	case CRL_S_XCMPLT:
		switch (crltab.crl_active) {
		case CRL_F_READ:
		case CRL_F_WRITE:
			bp->b_flags |= B_DONE;
			break;
		case CRL_F_RETSTS:
			crlstat.crl_ds = mfpr(STXDB);
			printf("crlcs=0x%b, crlds=0x%b\n",
				crlstat.crl_cs, CRLCS_BITS,
				crlstat.crl_ds, CRLDS_BITS);
			break;
		}	
		crltab.crl_active = CRL_IDLE;
		wakeup((caddr_t)bp);
		break;
	case CRL_S_XCONT:
		switch (crltab.crl_active) {
		case CRL_F_READ:
			*(crltab.crl_xaddr++) = mfpr(STXDB);
			--bp->b_bcount;
			mtpr(STXCS, ((bp->b_blkno<<8) | STXCS_IE|CRL_F_READ));
			break;
		case CRL_F_WRITE:
			mtpr(STXDB, *(crltab.crl_xaddr++));
			--bp->b_bcount;
			mtpr(STXCS, ((bp->b_blkno<<8) | STXCS_IE|CRL_F_WRITE));
			break;
		}
		break;
	case CRL_S_ABORT: 
		crltab.crl_active = CRL_F_RETSTS;
		mtpr(STXCS, (STXCS_IE | CRL_F_RETSTS));
		bp->b_flags |= (B_DONE | B_ERROR);
		break;
	case CRL_S_RETSTS:
		crlstat.crl_cs = mfpr(STXDB);
		mtpr(STXCS, (STXCS_IE | CRL_F_RETSTS));
		break;
	case CRL_S_HNDSHK:
		printf("crl: hndshk error\n");
		crltab.crl_active = CRL_IDLE;
		bp->b_flags |= (B_DONE | B_ERROR);
		wakeup((caddr_t)bp);
		break;
	case CRL_S_HWERR:
		printf("crl: hard error sn%d\n", bp->b_blkno);
		crltab.crl_active = CRL_F_ABORT;
		mtpr(STXCS, (STXCS_IE | CRL_F_ABORT));
		bp->b_flags |= (B_DONE | B_ERROR);
		break;
	}
}
#endif
