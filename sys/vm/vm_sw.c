#ifndef lint
static char *sccsid = "@(#)vm_sw.c	4.1	ULTRIX	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *
 *			Modification History
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 09-Nov-89 jaw
 *	remove support for asymmetric system calls...swapon not mpsafe yet.
 *
 * 16-Oct-89 bp
 *	collect swap I/O statistics in swstrategy
 *
 * 12-Jun-89  gg
 *	dynamic swap changes -- replaced variable dmmax with swapfrag.
 *	Added a check in swfree() to return 0 if no swap device is 
 *	configured.
 *	
 * 9-Nov-88 jaa
 *	swfree now will check the error return code when it 
 *	opens the device and return failures.
 *
 * 4-Apr-88 jaa
 *	now that swapmap is km_alloc'd, check that it's been alloc'd
 *	in swfree, panic if not 
 *
 *	Amato 14 Dec 87
 *	Added new KM_ALLLOC/KM_FREE code
 *
 *	Logcher 02 Mar 87
 *	Merged in diskless changes, added support for remote swapping 
 *
 *	Koehler	11 Sept 86
 *	made changes to gfs namei interface
 *
 *	Stephen Reilly, 09-Sept-85
 *	Modified to handle the new 4.3BSD namei code.
 *
 *	vm_sw.c	6.1	83/07/29
 ***********************************************************************/
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/kmalloc.h"
#include "../h/cpudata.h"

extern struct gnode gpp;
extern int swapfrag;
extern int availvas;

int availswap;

struct	buf rswbuf;
/*
 * Indirect driver for multi-controller paging.
 */
swstrategy(bp)
	register struct buf *bp;
{
	int sz, off, seg;
	dev_t dev;
	struct swdevt *swp;

#ifdef GENERIC
	/*
	 * A mini-root gets copied into the front of the swap
	 * and we run over top of the swap area just long
	 * enough for us to do a mkfs and restor of the real
	 * root (sure beats rewriting standalone restor).
	 */
#define	MINIROOTSIZE	4096
	if (rootdev == dumpdev)
		bp->b_blkno += MINIROOTSIZE;
#endif
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (bp->b_blkno+sz > nswap) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if (nswdev > 1) {
		off = bp->b_blkno % swapfrag;
		if (off+sz > swapfrag) {
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
		seg = bp->b_blkno / swapfrag;
		swp = &swdevt[seg % nswdev];
		seg /= nswdev;
		bp->b_blkno = seg*swapfrag + off;
	} else
		swp = &swdevt[0];
	swp->sw_nio += sz;
        if (swp->sw_type == SW_RAW)
		{
		dev = swp->sw_dev;
                bp->b_dev = swp->sw_dev;
		if (dev == 0)
			panic("swstrategy");
		(*bdevsw[major(dev)].d_strategy)(bp); 
		}
        if (swp->sw_type == SW_NFS)
		{
                bp->b_gp = (struct gnode *) swp->sw_gptr;
        	SW_IOSTRAT(swp,bp);
		}
/* 	NOTE SW_IOSTRAT needs ufs_strategy to be complete	 */
/*	otherwise the following line remains in the SW_RAW case  */
/*	(*bdevsw[major(dev)].d_strategy)(bp);			 */
}

swread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(swstrategy, &rswbuf, dev, B_READ, minphys, uio));
}

swwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(swstrategy, &rswbuf, dev, B_WRITE, minphys, uio));
}

/*
 * System call swapon(name) enables swapping on device name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this device.
 */
swapon()
{
	struct a {
		char	*name;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp;
	dev_t dev;
 	register struct nameidata *ndp = &u.u_nd;
	register struct swdevt *sp;
	int saveaffinity;

 	if (!suser())
 		return;

	/* system call NOT SMP SAFE...TODO */
	saveaffinity = switch_affinity(boot_cpu_mask);

 	ndp->ni_nameiop = LOOKUP | FOLLOW;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if(ndp->ni_dirp == NULL) {
		switch_affinity(saveaffinity);
		return;
	}
 	if(u.u_error = copyinstr(uap->name, ndp->ni_dirp, MAXPATHLEN, (u_int *)
	0)) {
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		switch_affinity(saveaffinity);
		return;
	}
 	gp = gfs_namei(ndp);
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
	if (gp == NULL) {
		switch_affinity(saveaffinity);
		return;
	}
	if ((gp->g_mode&GFMT) != GFBLK) {
		u.u_error = ENOTBLK;
		gput(gp);
		switch_affinity(saveaffinity);
		return;
	}
	dev = (dev_t)gp->g_rdev;
	gput(gp);
	if (major(dev) >= nblkdev) {
		u.u_error = ENXIO;
		switch_affinity(saveaffinity);
		return;
	}
	/*
	 * Search starting at second table entry,
	 * since first (primary swap area) is freed at boot.
	 */
	for (sp = &swdevt[1]; sp->sw_dev; sp++)
		if (sp->sw_dev == dev) {
			if (sp->sw_freed) {
				u.u_error = EBUSY;
				switch_affinity(saveaffinity);
				return;
			}
			if(swfree(sp - swdevt) == 0)
				u.u_error = EIO;
			switch_affinity(saveaffinity);
			return;
		}
	u.u_error = ENODEV;
	switch_affinity(saveaffinity);
}

/*
 * Swfree(index) frees the index'th portion of the swap map.
 * Each of the nswdev devices provides 1/nswdev'th of the swap
 * space, which is laid out with blocks of swapfrag pages circularly
 * among the devices.
 */
swfree(index)
	int index;
{
	register swblk_t vsbase, dvbase;
	register long blk;
	dev_t dev;
	register int nblks, s;

	if (swdevt[index].sw_type == SW_RAW) {
		dev = swdevt[index].sw_dev;
		if((*bdevsw[major(dev)].d_open)(dev, FREAD|FWRITE))
			return(0);
	}
	swdevt[index].sw_freed = 1;
	nblks = swdevt[index].sw_nblks;
	for (dvbase = 0; dvbase < nblks; dvbase += swapfrag) {
		blk = nblks - dvbase;
		if ((vsbase = index*swapfrag + dvbase*nswdev) >= nswap)
			panic("swfree");
		if (blk > swapfrag)
			blk = swapfrag;
		if (vsbase == 0) {
			/*
			 * Can't free a block starting at 0 in the swapmap
			 * Simply throw away the first 1K 
			 */
			if(swapmap == (struct map *) NULL) 
				panic("swfree: no swapmap");
			rminit(swapmap,
			       (long)((long)(blk-ctod(CLSIZE))-(long)ctod(CLSIZE)),
				(long)ctod(CLSIZE), "swap", nswapmap);
		} else
			rmfree(swapmap, blk, vsbase);
	}
	s = splimp();
	smp_lock(&lk_totalswap, LK_RETRY);
	availvas += dtoc(nblks);
	availswap += nblks;
	if(index == 0) {
		availswap -= ctod(CLSIZE << 1);
		availvas -= (CLSIZE << 1);
	}
	smp_unlock(&lk_totalswap);
	(void)splx(s);
	return(1);
}
