#ifndef lint
static char *sccsid = "@(#)rx50.c	4.1	ULTRIX	7/2/90";
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

/* ---------------------------------------------------------------------
 * Modification History
 *
 *	30-May-89	darrell
 *		Added include of ../../machine/common/cpuconf.h -- cpu
 *		types were moved there.
 *
 * 	26-Apr-88    jaw
 *		Add VAX8820 support.
 *
 *	10-Feb-89 -- jaw
 *		Add call in open routine to test if 8200 rx50 is present.
 *
 *	10-Feb-87 -- reilly (Stephen Reilly)
 *		Added the ioctl function to get the default partition
 *		tables.
 *		
 *	26-Aug-86 -- rsp (Ricky Palmer)
 *		Cleaned up devioctl code to (1) zero out devget structure
 *		upon entry and (2) use strlen instead of fixed storage
 *		for bcopy's.
 *
 *	 5-Aug-86 - fred (Fred Canter)
 *		Changed DEV_NOB to DEV_NB.
 *
 *	11-Jul-86 - rsp added adpt and nexus fields.
 *
 *	03-Jul-86 -- rsp added devioctl bug fix
 *
 *	02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 *	26 Nov 85 -- jaw
 *		rx50 error needs to be reported to console.
 *
 *	11 Nov 85 -- depp
 *		Removed System V conditional compiles.
 *
 *	19-Jun-85 -- jaw
 *		VAX8200 name change.
 *
 *	17 Jun 85 -- jaw
 *		had to redo mapping so image activation would work.
 *
 *	06 Jun 85 -- jaw
 *		made drive work.
 *
 *	20 Mar 85 -- jaw
 *		first cut at VAX8200 rx50 console media.
 *
 * ---------------------------------------------------------------------
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"

#include "../machine/mtpr.h"
#include "../machine/clock.h"
#include "../machine/cons.h"
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/pte.h"
#include "../machine/nexus.h"


#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../machine/rx50.h"
#include "../fs/ufs/fs.h"
#include "../h/ioctl.h"
#include "../h/dkio.h"
#include "../h/devio.h"
#include "../h/ipc.h"
#include "../h/shm.h"

struct rx5tab rx5tab;

struct size {
	daddr_t nblocks;
	int cyloff;
} rx5_sizes[8] = {
	800,	0,
	0,	0,
	800,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
};


struct pt rx5_part[2];


struct rxspace {
	char rx_pad[512];
};

struct rxspace RX_bufmap[];



cs_ioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct pt *pt = (struct pt *)data;
	register int unit = (minor(dev) >> 3)-1;
	register int i, error;
	struct dkop *dkop;
	struct dkget *dkget;
	struct devget *devget;

	switch (cmd) {

	case DIOCDGTPT:	/* Get default partition table */

		/*
		 *	Get the partition size and offset
		 */
		for (i=0; i<=7; i++){
			pt->pt_part[i].pi_nblocks =
					rx5_sizes[i].nblocks;

			pt->pt_part[i].pi_blkoff =
					rx5_sizes[i].cyloff * 10;
		}

		break;

	case DIOCGETPT:  /* 001 get partition table info */
		/*
		 *	Do a structure copy into the user's data area
		 */

		*pt = rx5_part[unit];
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
		if ( ( error = ptcmp( dev, &rx5_part[unit], pt ) ) != 0 )
			return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */

		rx5_part[unit] = *pt;

		/*
		 *	See if we need to update the superblock of the
		 *	"a" partition of this disk
		 */
		ssblk(dev,pt);

		/*
		 *	Just make sure that we set the valid bit
		 */

		rx5_part[unit].pt_valid = PT_VALID;
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
		devget->category = DEV_DISK;		       /* Disk	       */
		devget->bus = DEV_NB;			       /* No bus con.  */
		bcopy(DEV_UNKNOWN,devget->interface,
		      strlen(DEV_UNKNOWN));		       /* n/a	       */
		bcopy(DEV_RX50,devget->device,
		      strlen(DEV_RX50));		       /* rx50	       */
		devget->adpt_num = -1;			       /* n/a	       */
		devget->nexus_num = -1; 		       /* n/a	       */
		devget->bus_num = -1;			       /* n/a	       */
		devget->ctlr_num = -1;			       /* no contrl.   */
		devget->slave_num = unit+1;		       /* which plug   */
		bcopy("cs", devget->dev_name,3);	       /* Ultrix "cs"  */
		devget->unit_num = unit+1;		       /* which cs??   */
		devget->soft_count = 0; 		       /* soft er.cnt. */
		devget->hard_count = 0; 		       /* hard er.cnt. */
		devget->stat = 0;			       /* status       */
		devget->category_stat = DEV_DISKPART;	       /* which prtn.  */
		break;

	default:
		return (ENXIO);
	}
	return(0);
}


cs_open(dev,flg)
	register dev_t	 dev ;
	short	flg ;
{
	register int i;
	register int unit;
	int cs_strategy();

	unit = minor(dev)>>3;
	unit = unit-1;
	if ((cpu != VAX_8200) && (cpu != VAX_8800)) return(ENXIO);

	if (cpu == VAX_8200)
		if (ka8200rxopen(unit)) return(ENXIO);

	/* set up partition table for unit */
	if (rx5_part[unit].pt_valid == 0) {

		for (i=0; i<=7; i++){
			rx5_part[unit].pt_part[i].pi_nblocks =
					rx5_sizes[i].nblocks;

			rx5_part[unit].pt_part[i].pi_blkoff =
					rx5_sizes[i].cyloff * 10;
			rx5_part[unit].pt_valid = PT_VALID;
		}
		rsblk(cs_strategy,dev,&rx5_part[unit]);

		rx5_part[unit].pt_valid = PT_VALID;
	}


	rx5tab.rx5_state |= RX5OPEN;
	return(0);

}


cs_read(dev,uio)
	dev_t	dev;
	struct	uio *uio;
{
	register int unit;
	int cs_strategy() ;
	unit = ((int) minor(dev)>>3) - 1;
	return(physio(cs_strategy,&rrx5buf[unit],dev,B_READ,minphys,uio));
}


cs_write(dev,uio)
	dev_t	dev;
	struct	uio *uio;
{
	register int unit;
	int cs_strategy();
	unit = ((int) minor(dev)>>3) - 1;
	return(physio(cs_strategy,&rrx5buf[unit],dev,B_WRITE,minphys,uio));
}


cs_strategy(bp)
	register struct  buf *bp;
{
	register unsigned v;
	register int s,npf,o;
	int sz;
	int rx5part;
	register struct pte *pte;
	register struct pte *mpte;
	register struct proc *rp;


	/* must wait here for structure to be free */
	s=spl4();
	while (rx5tab.rx5_state & RX5BUSY)
		sleep((caddr_t)&rx5tab,PRIBIO);

	rx5tab.rx5_state |= RX5BUSY;

	splx(s);

	/* save off needed info */
	rx5tab.rx5blk = bp->b_blkno;
	rx5tab.rx5_buf = bp;
	rx5tab.rx5resid = bp->b_bcount;

	rx5_unit = (minor(bp->b_dev)>>3);
	rx5_unit=rx5_unit-1;	/* note it is units 1 and 2 */

	rx5part = bp->b_dev & 07;

	sz = (bp->b_bcount+511) >> 9;

	/* calculate physical block to read from */
	rx5tab.rx5blk = bp->b_blkno +
			 rx5_part[rx5_unit].pt_part[rx5part].pi_blkoff;

	if (rx5_unit >= N_RX5 ||	/* only two drives */
	   ((bp->b_blkno + sz)		/* only 800 blocks */
			> rx5_part[rx5_unit].pt_part[rx5part].pi_nblocks) ||
	   (bp->b_bcount > RX5_MAX_TRANSFER)) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		rx5tab.rx5_state &= ~ RX5BUSY;
		wakeup((caddr_t)&rx5tab);
		return;
	}

	/* following code figures out the proper ptes to
	   remap into system space so interrupt routine can
	   copy into buf structure. */
	v = btop(bp->b_un.b_addr);
	o = (int)bp->b_un.b_addr & PGOFSET;
	npf = btoc(rx5tab.rx5resid + o) + 1;
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	if ((bp->b_flags & B_PHYS) == 0)
	{
		rx5tab.rx5addr = bp->b_un.b_addr;
	}
	else {
		if (bp->b_flags & B_UAREA)
			pte = &rp->p_addr[v];
		else if (bp->b_flags & B_PAGET)
			pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		else if ((bp->b_flags & B_SMEM)  &&	/* SHMEM */
					((bp->b_flags & B_DIRTY) == 0))
			pte = ((struct smem *)rp)->sm_ptaddr + v;
		else
			pte = vtopte(rp, v);


		rx5tab.rx5addr = (char *)((int)RX_bufmap + (int)o);
		mpte = (struct pte *)rxbufmap;

		while (--npf >= 0) {
			*(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
			mtpr(TBIS, (char *) RX_bufmap + (npf*NBPG));
		}
	}


	cs_start();
}


cs_start()
{
	if (cpu == VAX_8800)
		ka8800startrx();
	else
		ka8200startrx();
}


cs_close(dev,flg)
	dev_t	dev ;
	short	flg ;
{
	rx5tab.rx5_state &= ~RX5OPEN;
}

cs_size(dev)
	dev_t dev;
{
	int part = minor(dev) & 07;

	return((int) rx5_sizes[part].nblocks);
}

