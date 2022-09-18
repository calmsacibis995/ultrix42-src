
#ifndef lint
static char *sccsid = "@(#)md.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 by			*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * md.c
 *
 * Modification history
 *
 * SAS memory device driver
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 *  5-Aug-86 - fred (Fred Canter)
 *
 *	Changed DEV_NOB to DEV_NB.
 *
 * 12-Mar-86 - tresvik
 *
 *	This  driver  is  intended to be used in the Ultrix standalone
 *	environment where a device named md0a will be the root device.
 *	md0b  will  be made the swap device, but the kernel must never
 *	try  to page.  A monstrous lie is told when the drive is asked
 *	about  the  size  of  the swap partition.  This environment is
 *	intended  primarily to support an installation which will have
 *	limited commands and functions available.  It runs single user
 *	only,  with  minimal  kernel function built in.  This is in an
 *	effort	to  minimize  it's  size as the associated memory file
 *	system occupies a fair amount of memory too.  This driver will
 *	not  be  supported  in	any  other running environment in it's
 *	current state.	- tresvik
 *
 * 23-Apr-86 - Ricky Palmer
 *
 *	Added new DEVIOCGET ioctl request code.  V2.0
 *
 * 18-Jun-86 - Tresvik
 *
 *	Prevent reads and writes on partitions other than 0.
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/devio.h"

#include "../vax/mtpr.h"

#include "md.h"

int mdsize=NMD;
char memdev[NMD*512]=1; 		/* allocate in data space */

struct	mdspace {
	char md_pad[512];
};
struct	mdspace MD_bufmap[];

mdstrategy(bp)
register struct buf *bp;
{
	long sz, bn;
	register char *mdv;
	register struct pte *pte, *mpte;
	int *bufp;
	unsigned v;
	int i, o, npf;
	struct proc *rp;
	int md_unit;

	md_unit = dkunit(bp);
	sz = (bp->b_bcount + 511) >> 9;
	if (md_unit != 0 || bp->b_blkno < 0 ||
	    (minor(bp->b_dev) != 0) ||
	    (bn = dkblock(bp)) + sz > mdsize) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
		}
	mdv = (char *) memdev + (bn*NBPG);
	if ((bp->b_flags & B_PHYS) == 0) {
		bufp = (int *)bp->b_un.b_addr;
	}
	else {
		/*
		 * map to user space
		 */
		v = btop(bp->b_un.b_addr);
		o = (int)bp->b_un.b_addr & PGOFSET;
		npf = btoc(bp->b_bcount + o);
		rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
		if (bp->b_flags & B_UAREA) {
			pte = &rp->p_addr[v];
		}
		else if (bp->b_flags & B_PAGET) {
			pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		}
		else {
			pte = vtopte(rp, v);
		}
		bufp = (int *)MD_bufmap;
		mpte = (struct pte *)mdbufmap;
		while (--npf >= 0) {
			if (pte->pg_pfnum == 0)
				panic("mda0: zero pfn in pte");
			*(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
			mtpr(TBIS, (char *) MD_bufmap + (npf*NBPG));
		}
	}
	if (bp->b_flags & B_READ) {
		bcopy (mdv, bufp, bp->b_bcount);
	} else {
		bcopy (bufp, mdv, bp->b_bcount);
	}
	iodone (bp);
}


md_size(dev)
	dev_t dev;
{
	/*
	 * Lie a little
	 */
	return (33440);
}

mdioctl(dev, cmd, data, flag)
	register dev_t dev;
	register int cmd;
	register caddr_t data;
	register int flag;
{
	register struct devget *devget;

	switch (cmd) {

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_SPECIAL; 	/* special disk */
		devget->bus = DEV_NB;			/* no bus con.	*/
		bcopy(DEV_UNKNOWN,devget->interface,
		      strlen(DEV_UNKNOWN));		/* n/a		*/
		bcopy(DEV_RAMDISK,devget->device,
		      strlen(DEV_RAMDISK));		/* memory disk	*/
		devget->adpt_num = -1;			/* n/a		*/
		devget->nexus_num = -1; 		/* n/a		*/
		devget->bus_num = -1;			/* n/a		*/
		devget->ctlr_num = -1;			/* n/a		*/
		devget->slave_num = -1; 		/* n/a		*/
		bcopy("md",devget->dev_name,3); 	/* Ultrix "md"	*/
		devget->unit_num = 0;			/* md0		*/
		devget->soft_count = 0; 		/* always 0	*/
		devget->hard_count = 0; 		/* always 0	*/
		devget->stat = 0;			/* always 0	*/
		devget->category_stat = DEV_DISKPART;	/* parition ?	*/
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

