#ifndef lint
static	char	*sccsid = "@(#)gfs_bio.c	4.4	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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


/***********************************************************************
 *
 *		Modification History
 *
 * 27 Feb 91 -- chet
 *	Fix MP window in getblk().
 *
 *  7 Jul 90 -- chet
 *	add support for B_FORCEWRITE flag
 *
 * 06 Mar 90 -- jaw
 *	add missing splx's.
 *
 * 10 Dec 89 -- chet
 *	Replace nomatch() calls; change binvalfree() to use
 *	cacheinval(). 
 *	
 * 10-Nov-89 -- jaw
 * 	fix locking in bflush.  There was one path where lk_bio was 
 *	not being released.
 *
 * 22 Aug 89 -- prs
 *	Fixed binvalfree().
 *
 * 25 Jul 89 -- chet
 *	Buffer cache overhaul
 *
 * 14 Jun 89 -- chet
 *	Removed debugging code
 *
 * 02 Nov 88 -- jaw
 *	optimize locking in biodone.
 *
 * 18 Oct 88 -- cb
 *	Removed mprintf in biodone().
 *
 * 22 Sep 88 -- chet
 *	Added lower priority level and stat gathering stuff.
 *
 * 27 Apr 88 -- chet
 *	Add SMP support.
 *
 * 15 Mar 88 -- chet
 *	Add B_NOCACHE support.
 *
 * 26 Jan 88 -- chet
 *	Try to finally fix bhalt() once and for all.
 *
 * 14 Jul 87 -- chet
 *	Added binvalfree() primitive to only invalidate buffers that are
 *	on free lists; binval() clobbers all, even busy buffers.
 *
 * 17 Jun 87 -- cb
 *      Changed bhalt to only wait on local busy buffers.
 *
 * 13 Feb 87 -- prs
 *	Changed bhalt to wait on busy buffers
 *
 * 12 Feb 87 -- chet
 *	match buffer and gnode before bwrite() GIOSTRATEGY call
 *
 * 29 Jan 87 -- chet
 *	add new arg to bdwrite()
 *
 * 22 Jan 87 -- koehler
 *	changed bhalt so that busy buffers aren't flushed
 *
 * 31 Oct 86 -- koehler
 *	changed bwrite so that gp isn't needed
 *
 * 11 Sep 86 -- koehler
 *	changes for synchronous filesystems
 *
 ***********************************************************************/

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/smp_lock.h"
#include "../h/vm.h"
#include "../h/trace.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/cpudata.h"

struct bufstats bufstats;

int bflush_debug;

struct lock_t lk_bio;
int bfreelist_wanted;

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *
bread(dev, blkno, size, gp)
	dev_t dev;
	register daddr_t blkno;
	register int size;
	register struct gnode *gp;
{
	register struct buf *bp;
	int saveaffinity;

	if (size == 0)
		panic("bread: size 0");

loop:
	bp = getblk(dev, blkno, size, gp);
	if (bp->b_flags&B_DONE) {
		trace(TR_BREADHIT, dev, blkno);
		++bufstats.readhit;
		return(bp);
	}

	/*
	 * See if the block is a delayed write.
	 * If it is, write it synchronously before returning it to the reader.
	 */
	if (bp->b_flags&B_DELWRI) {
		++bufstats.delwrite;
		bwrite(bp);
		goto loop;
	}

	bp->b_flags |= B_READ;
	if (bp->b_bcount > bp->b_bufsize)
		panic("bread");
	STRATEGY(gp, dev, bp, saveaffinity);
	trace(TR_BREADMISS, dev, blkno);
	++bufstats.readmiss;
	u.u_ru.ru_inblock++;		/* pay for read */
	biowait(bp);
	return (bp);
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 */
struct buf *
breada(dev, blkno, size, rablkno, rabsize, gp)
	dev_t dev;
	register daddr_t blkno; 
	register int size;
	register daddr_t rablkno;
	register struct gnode *gp;
	int rabsize;
{
	register struct buf *bp, *rabp;
	int saveaffinity;

	bp = NULL;
	/*
	 * If the block isn't in core, then allocate
	 * a buffer and initiate i/o (getblk checks
	 * for a cache hit).
	 */
	if (!incore(dev, blkno, gp)) {
		bp = getblk(dev, blkno, size, gp);

		if ((bp->b_flags&B_DONE) == 0) {
			bp->b_flags |= B_READ;
			if (bp->b_bcount > bp->b_bufsize)
				panic("breada");
			STRATEGY(gp, dev, bp, saveaffinity);
			trace(TR_BREADMISS, dev, blkno);
			++bufstats.readmiss;
			u.u_ru.ru_inblock++;		/* pay for read */
		} else {
			trace(TR_BREADHIT, dev, blkno);
			++bufstats.readhit;
		}
	}

	/*
	 * If there's a read-ahead block, start i/o
	 * on it also (as above).
	 */
	if (rablkno) {
		if (!incore(dev, rablkno, gp)) {
			rabp = getblk(dev, rablkno, rabsize, gp);
			if (rabp->b_flags & B_DONE) {
				brelse(rabp);
				trace(TR_BREADHITRA, dev, blkno);
				++bufstats.readahit;
			} else {
				rabp->b_flags |= B_READ|B_ASYNC;
				if (rabp->b_bcount > rabp->b_bufsize)
					panic("breadrabp");
				STRATEGY(gp, dev, rabp, saveaffinity);
				trace(TR_BREADMISSRA, dev, rablock);
				++bufstats.readamiss;
				u.u_ru.ru_inblock++;    /* pay in advance */
			}
		}
		else {
			trace(TR_BREADHITRA, dev, blkno);
			++bufstats.readahit;
		}
	}

	/*
	 * If block was in core, let bread get it.
	 * If block wasn't in core, then the read was started
	 * above, and just wait for it.
	 */
	if (bp == NULL)
		return (bread(dev, blkno, size, gp));
	biowait(bp);
	return (bp);
}

/*
 * Write a buffer; if synchronous, wait for completion and
 * release the buffer.
 */
bwrite(bp)
	register struct buf *bp;
{
	register int flag;
	register struct gnode *gp = bp->b_gp;
	register struct mount *mp;
	int saveaffinity;

	flag = bp->b_flags; /* save incoming flag values */
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	if ((flag&B_DELWRI) == 0)
		u.u_ru.ru_oublock++;		/* noone paid yet */
	trace(TR_BWRITE, bp->b_dev, bp->b_blkno);
	if (bp->b_bcount > bp->b_bufsize)
		panic("bwrite");
	STRATEGY(gp, bp->b_dev, bp, saveaffinity);
	/*
	 * If the write was synchronous, then await i/o completion.
	 * If the write was "delayed", then we put the buffer on
	 * the q of blocks awaiting i/o completion status.
	 */
	if ((flag&B_ASYNC) == 0) {
		++bufstats.sync_write;
		biowait(bp);
		brelse(bp);
	}
	else
		++bufstats.async_write;
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (used e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 */
bdwrite(bp)
	register struct buf *bp;
{
	register int flags = 0;

	if ((bp->b_flags&B_DELWRI) == 0)
		u.u_ru.ru_oublock++;		/* noone paid yet */

	/* hack: check for local fs */
	if (major(bp->b_dev) < nblkdev) {
		if (bdevsw[major(bp->b_dev)].d_flags & B_TAPE) {
			bawrite(bp);
			return;
		}
	}

	bp->b_flags |= B_DELWRI | B_DONE;
	brelse(bp);
}

/*
 * bawrite() is now a macro in buf.h
 */ 

/*
 * Release the buffer, with no I/O implied.
 */
brelse(bp)
	register struct buf *bp;
{
	register struct buf *flist;
	register int s;

	++bufstats.brelse;
	if (bp->b_flags&B_ERROR)
		bp->b_dev = NODEV;

	/*
	 * Stick the buffer back on a free list.
	 *
	 * New handling of delayed write buffers:
	 *	Instead of the old AGE and LRU lists, there are now
	 *	DIRTY and CLEAN lists. Delayed write buffers are placed
	 *	at the tail of the DIRTY list. NOCACHE and INVAL
	 *	buffers are placed at the head of the CLEAN list.
	 *	All others are placed at the tail of the CLEAN list.
	 *	Having a DIRTY list makes flush operations much faster.
	 *
	 * The B_AGE flag is now ignored.
	 */
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	if (bp->state & B_BUSY) {	/* take off of busy list */
		brembusy(bp);
	}
	else
		++bufstats.brelsenotbusy;

	if (bp->b_flags & B_NOCACHE) { /* reuse NOCACHE buffers first */
		bp->state |= B_INVAL;
	}
	if (bp->b_bufsize <= 0) {
		/* block has no buffer... put at front of unused buffer list */
		flist = &bfreelist[BQ_EMPTY];
		binsheadfree(bp, flist);
	} else if ((bp->b_flags & B_ERROR) || (bp->state & B_INVAL)) {
		/* block has no info ... put at front of most free list */
		flist = &bfreelist[BQ_CLEAN];
		binsheadfree(bp, flist);
	} else {
		if (bp->b_flags & B_DELWRI)
			flist = &bfreelist[BQ_DIRTY];
		else
			flist = &bfreelist[BQ_CLEAN];
		binstailfree(bp, flist);
	}

	/*
	 * If someone's waiting for the buffer, or
	 * is waiting for any free buffer wake 'em up.
	 */
	if (bp->state&B_WANTED)
		wakeup((caddr_t)bp);
	if (bfreelist_wanted) {
		bfreelist_wanted = 0;
		wakeup((caddr_t)bfreelist);
	}

	bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC|B_AGE|B_NOCACHE|B_FORCEWRITE);
	bp->state &= ~(B_WANTED|B_BUSY);
	smp_unlock(&lk_bio);
	splx(s);
}

/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada).
 *
 * NOTE: this function is advisory only.
 */
incore(dev, blkno, gp)
	dev_t dev;
	register daddr_t blkno;
	register struct gnode *gp;
{
	register struct buf *bp;
	register struct buf *dp;
	register int s;

	dp = BUFHASH(dev, blkno, gp);

	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if ((bp->b_blkno != blkno) || (bp->b_dev != dev) ||
		    (bp->state & B_INVAL) || !matchgp(bp, gp) || 
		    !matchgid(bp, gp))
			continue;
		smp_unlock(&lk_bio);
		splx(s);
		return(1);
	}
	smp_unlock(&lk_bio);
	splx(s);
	return(0);
}

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 */
int getblk_mp_window = 0;

struct buf *
getblk(dev, blkno, size, gp)
	dev_t dev;
	register daddr_t blkno;
	register int size;
	register struct gnode *gp;
{
	register struct buf *bp, *dp;
	register int s;

	/* CJXXX - this must mean something to somebody */
	if ((unsigned)blkno >= 1 << (sizeof(int)*NBBY-DEV_BSHIFT)) { /* XXX */
		mprintf("getblk: invalid blkno %d, dev %d,%d",
		       blkno, major(dev), minor(dev));
		if (gp)
			mprintf(" gp 0x%x number %d mode %o\n",
				gp, gp->g_number, gp->g_mode);
		else
			mprintf(" gp NULL\n");
		blkno = 1 << ((sizeof(int)*NBBY-DEV_BSHIFT) + 1);
	}
	
	/*
	 * Search the cache for the block.  If we hit, but
	 * the buffer is in use for i/o, then we wait until
	 * the i/o has completed.
	 */
	dp = BUFHASH(dev, blkno, gp);
loop:
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {

		if ((bp->b_blkno != blkno) || (bp->b_dev != dev) ||
		    (bp->state&B_INVAL) || !matchgp(bp, gp))
			continue;

		/* Before doing matchgid(), wait for BUSY buffers
		 * to complete. This handles the race between reads and
		 * asynchronous writes in progress (NFS). For UFS, we've
		 * already matched, and matchgid() will always succeed.
		 */
		if (bp->state&B_BUSY) {
			bp->state |= B_WANTED;
			sleep_unlock((caddr_t)bp, PRIBIO+1, &lk_bio);
			splx(s);
			goto loop;
		}
		if (!matchgid(bp, gp))
			continue;

		notavail(bp);
		smp_unlock(&lk_bio);
		splx(s);

		if (size != bp->b_bcount) {
			if (bp->b_flags & B_DELWRI) {
				++bufstats.delwrite;
				bwrite(bp);
				goto loop;
			}
			if (brealloc(bp, size) == 0)
				goto loop;
		}
		bp->b_flags |= B_CACHE;
		return(bp);
	}
	smp_unlock(&lk_bio);
	splx(s);

	bp = getnewbuf();

	splbio();
	smp_lock(&lk_bio, LK_RETRY);
	/*
	 * We gave up lk_bio to call getnewbuf(). Getnewbuf() can
	 * start asynchronous writes - getting and releasing lk_bio.
	 * Check to make sure that our request for this block
	 * hasn't already been granted by getnewbuf() to some other
	 * thread (here thanks to MASPAR).
	 */
	{
		register struct buf *tbp;

		for (tbp = dp->b_forw; tbp != dp; tbp = tbp->b_forw) {
			if ((tbp->b_blkno != blkno) || (tbp->b_dev != dev) ||
			    (tbp->state&B_INVAL) || !matchgp(tbp, gp) ||
			    !matchgid(tbp, gp))
				continue;
			++getblk_mp_window;
			bp->state |= B_INVAL;
			bp->b_dev = (dev_t)NODEV;
			bp->b_error = 0;
			smp_unlock(&lk_bio);
			splx(s);
			brelse(bp);
			goto loop;
		}
	}

	bremhash(bp);
	binshash(bp, dp);
	bp->b_dev = dev;
	bp->b_blkno = blkno;
	bp->b_gp = gp;
	if (gp)
		bp->b_gid = gp->g_id;
	else
		bp->b_gid = 0;
	bp->b_bcount = 0;
	smp_unlock(&lk_bio);
	splx(s);

	bp->b_error = 0;
	bp->b_resid = 0;
	if (brealloc(bp, size) == 0)
		goto loop;
	return(bp);
}

/*
 * get an empty block,
 * not assigned to any particular device
 */
struct buf *
geteblk(size)
	register int size;
{
	register struct buf *bp, *flist;
	register int s;

	++bufstats.geteblk;
loop:
	bp = getnewbuf();
	flist = &bfreelist[BQ_CLEAN];

	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	bremhash(bp);
	binshash(bp, flist);
	brembusy(bp);	/* we don't want this seen by synchronous bflush */
	bp->state = B_INVAL;
	bp->b_dev = (dev_t)NODEV;
	bp->b_gp = NULL;
	bp->b_gid = 0;
	bp->b_bcount = 0;
	smp_unlock(&lk_bio);
	splx(s);

	bp->b_resid = 0;
	bp->b_error = 0;
	if (brealloc(bp, size) == 0)
		goto loop;
	return(bp);
}

/*
 * Allocate space associated with a buffer.
 * If can't get space, buffer is released
 */
brealloc(bp, size)
	register struct buf *bp;
	register int size;
{
	register daddr_t start, last;
	register struct buf *ep;
	register struct buf *dp;
	register int s;
	struct gnode *gp;

	/*
	 * First need to make sure that all overlapping previous I/O
	 * is dispatched with.
	 */
	if (size == bp->b_bcount)
		return (1);
	if (size < bp->b_bcount) { 
		if (bp->b_flags & B_DELWRI) {
			++bufstats.delwrite;
			bwrite(bp);
			return (0);
		}
		++bufstats.realloc;
		if (smp) {	/* we don't move memory around w/smp */
			bp->b_bcount = size;
			return (1);
		}
		return (allocbuf(bp, size));
	}
	bp->b_flags &= ~B_DONE;
	if (bp->b_dev == NODEV) {
		++bufstats.realloc;
		if (smp) {	/* we don't move memory around w/smp */
			bp->b_bcount = size;
			return (1);
		}
		return (allocbuf(bp, size));
	}
	/*
	 * Search cache for any buffers that overlap the one that we
	 * are trying to allocate. Overlapping buffers must be marked
	 * invalid, after being written out if they are dirty (indicated
	 * by B_DELWRI). A disk block must be mapped by at most one buffer
	 * at any point in time. Care must be taken to avoid deadlocking
	 * when two buffers are trying to get the same set of disk blocks.
	 */
	start = bp->b_blkno;
	last = start + btodb(size) - 1;

	gp = bp->b_gp;
	dp = BUFHASH(bp->b_dev, bp->b_blkno, gp);
loop:
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if ((ep->b_dev != bp->b_dev) || (ep == bp) ||
		    (ep->state&B_INVAL) || !matchgp(ep, gp))
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno > last ||
		    ep->b_blkno + btodb(ep->b_bcount) <= start)
			continue;
		if (ep->state&B_BUSY) {
			ep->state |= B_WANTED;
			sleep_unlock((caddr_t)ep, PRIBIO+1, &lk_bio);
			splx(s);
			goto loop;
		}
		if (!matchgid(ep, gp))
			continue;
		notavail(ep);
		if (ep->b_flags & B_DELWRI) {
			smp_unlock(&lk_bio);
			splx(s);
			++bufstats.delwrite;
			bwrite(ep);
			goto loop;
		}
		ep->state |= B_INVAL;
		smp_unlock(&lk_bio);
		splx(s);
		brelse(ep);
		goto loop;
	}
	smp_unlock(&lk_bio);
	splx(s);
	++bufstats.realloc;
	if (smp) {	/* we don't move memory around w/smp */
		bp->b_bcount = size;
		return (1);
	}
	return (allocbuf(bp, size));
}

/*
 * Find a buffer which is available for use.
 * Select something from a free list.
 * Preference is to first search CLEAN list, then the DIRTY list
 * (flushing a DIRTY buffer takes time).
 */
struct buf *
getnewbuf()
{
	register struct buf *bp, *dp;
	register int s;

loop:
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (dp = &bfreelist[BQ_CLEAN]; dp > bfreelist; dp--)
		if (dp->av_forw != dp)
			break;
	if (dp == bfreelist) {		/* no free blocks */
		bfreelist_wanted = 1;
		sleep_unlock((caddr_t)dp, PRIBIO+1, &lk_bio);
		splx(s);
		goto loop;
	}
	bp = dp->av_forw;
	bp->state = 0;

	/*
	 * Remove the buffer from the free list and push it out if delayed.
	 */
	notavail(bp);
	smp_unlock(&lk_bio);
	splx(s);
	if (bp->b_flags & B_DELWRI) {
		bp->b_flags |= B_ASYNC;
		bp->b_flags |= B_FORCEWRITE;
		++bufstats.delwrite;
		++bufstats.forcewrite;
		bwrite(bp);
		goto loop;
	}
	trace(TR_BRELSE, bp->b_dev, bp->b_blkno);
	++bufstats.newbuf;

	/*
	 * Clean out the flag word, but leave the B_BUSY bit 
	 * (notavail set state)
	 */
	bp->b_flags = B_BUSY;

	return (bp);
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
biowait(bp)
	register struct buf *bp;
{
	register int s;

	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	while (!(bp->b_flags&B_DONE)) {
		sleep_unlock((caddr_t)bp, PRIBIO, &lk_bio);
		smp_lock(&lk_bio, LK_RETRY);
	}
	smp_unlock(&lk_bio);
	splx(s);
	if (u.u_error == 0)			/* XXX */
		u.u_error = geterror(bp);
}

/*
 * Mark I/O complete on a buffer.
 * If someone should be called, e.g. the pageout
 * daemon, do so.  Otherwise, wake up anyone
 * waiting for it.
 */
biodone(bp)
	register struct buf *bp;
{
	register int s;

	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);

	if (bp->b_flags & B_ERROR)
		++bufstats.biodone_errs;

	if (bp->b_flags & B_DONE)
		panic("dup biodone");
	bp->b_flags |= B_DONE;

	if (bp->b_flags & B_CALL) {
		bp->b_flags &= ~B_CALL;
		smp_unlock(&lk_bio);
		splx(s);
		(*bp->b_iodone)(bp);
	} else 	if (bp->b_flags&B_ASYNC) {
		smp_unlock(&lk_bio);
		splx(s);
		brelse(bp);
	} else {
		bp->state &= ~B_WANTED;
		smp_unlock(&lk_bio);
		splx(s);
		wakeup((caddr_t)bp);
	}
}

flushblocks(gp)
register struct gnode *gp;
{
	register int ret;
	if (ISLOCAL(gp->g_mp)) 
	        ret = bflush(gp->g_dev, (struct gnode *) 0, 0);
	else 
	        ret = bflush(NODEV, gp, 0); /* nfs is easy since bp has gp */
}


/*
 * Insure that no part of a specified block is in an incore buffer.
 */
blkflush(dev, blkno, size, gp)
	dev_t dev;
	register daddr_t blkno;
	long size;
	register struct gnode *gp;	
{
	register struct buf *ep;
	register struct buf *dp;
	register daddr_t start, last;
	register int flushed = 0;
	register int s;
	
	++bufstats.blkflush_call;
	if (gp)
		++bufstats.blkflushgp;
	start = blkno;
	last = start + btodb(size) - 1;
	dp = BUFHASH(dev, blkno, gp);

loop:
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		++bufstats.blkflush_look;
		if ((ep->b_dev != dev) || (ep->state & B_INVAL) ||
		     !matchgp(ep, gp))
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno > last ||
		    ep->b_blkno + btodb(ep->b_bcount) <= start)
			continue;
		if (ep->state&B_BUSY) {
			++bufstats.blkflush_sleep;
			ep->state |= B_WANTED;
			sleep_unlock((caddr_t)ep, PRIBIO+1, &lk_bio);
			splx(s);
			goto loop;
		}
		if (!matchgid(ep, gp))
			continue;
		if (ep->b_flags & B_DELWRI) {
			++bufstats.blkflush_flush;
			notavail(ep);
			++bufstats.delwrite;
			smp_unlock(&lk_bio);
			splx(s);
			bwrite(ep);
			flushed++;
			goto loop;
		}
	}
	smp_unlock(&lk_bio);
	splx(s);
	return (flushed);
}

/*
 * Make sure all write-behind blocks associated with:
 *	gp, if specified,
 *	else dev (NODEV for all)
 * are flushed out.
 * Note that a non-NULL gp overrides dev;
 * if gp is NULL, and dev is NODEV, then all write behind blocks
 * are flushed.
 * 
 * Called from unmount routines, update(), and sfs sync routines.
 *
 * The waitfor argument specifies whether the busy list should be waited
 * on after the dirty list is flushed.
 *
 * We look ONLY at the dirty list, because that's the only place
 * a DELWRI buffer should be.
 *
 * Whenever we find one, we rescan the list from scratch, to avoid
 * races.  This should not really add to the running time, since
 * we are just grabbing the first item on the list.
 *
 * This routine is a merge of the old bflush() and bflushgp().
 */

bflush(dev, gp, waitfor)
	dev_t dev;
	struct gnode *gp;
	int waitfor;
{
	register struct buf *bp;
	register struct buf *flist;
	register int s;
	register int flushed = 0;
	register int n_look = 0;
	register long start_time;

	struct bflush_dirty *bd;
	struct bflush_busy *bb;

	int n_sleep = 0;
	int n_clean = 0;
	int n_dirty = 0;
	int n_empty = 0;
	int n_busy = 0;

	/* account for type of call */
	if (!waitfor) { /* asynchronous call */
		if (gp)
			bd = &bufstats.gp_async;
		else if (dev != NODEV)
			bd = &bufstats.dev_async;
		else
			bd = &bufstats.all_async;
	}
	else { /* synchronous call */
		if (gp) {
			bd = &bufstats.gp_sync;
			bb = &bufstats.gp_busy;
		}
		else if (dev != NODEV) {
			bd = &bufstats.dev_sync;
			bb = &bufstats.dev_busy;
		}
		else {
			bd = &bufstats.all_sync;
			bb = &bufstats.all_busy;
		}
	}
	++bd->call;

	/* if debugging, count the queues */
	if (bflush_debug) {
		s = splbio();
		smp_lock(&lk_bio, LK_RETRY);
		flist = &bfreelist[BQ_CLEAN];
		for (bp = flist->av_forw; bp != flist; bp = bp->av_forw)
			++n_clean;
		flist = &bfreelist[BQ_DIRTY];
		for (bp = flist->av_forw; bp != flist; bp = bp->av_forw)
			++n_dirty;
		flist = &bfreelist[BQ_EMPTY];
		for (bp = flist->av_forw; bp != flist; bp = bp->av_forw)
			++n_empty;
		flist = &bbusylist;
		for (bp = flist->av_forw; bp != flist; bp = bp->av_forw)
			++n_busy;
		smp_unlock(&lk_bio);
		splx(s);
        }

	/* Flush dirty buffers.
	/* Look only at the dirty queue. */
	/* If we find a buffer that fits the bill, push it and start over. */
	flist = &(bfreelist[BQ_DIRTY]);
loop:
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (bp = flist->av_forw; bp != flist; bp = bp->av_forw) {
		++n_look;
		if ((bp->b_flags & B_DELWRI) == 0) {
			printf("bflush(%x) bp %x not DELWRI\n", dev, bp);
			continue;
		}				
		if (gp) { /* gp, if specified, has priority */
			if (!matchgp(bp, gp))
				continue;
		}
		else { /* gp was not specified */
			if (dev != NODEV && dev != bp->b_dev)
				continue;
		}
		/* now, we have a dirty buffer that interests us */
		notavail(bp);
		smp_unlock(&lk_bio);
		splx(s);
		bp->b_flags |= B_ASYNC;
		++bufstats.delwrite;
		bwrite(bp);
		++flushed;
		/* put an upper bound on how many buffers we'll push */
		if (flushed == nbuf) {	/* we've tried long enough! */
			++bd->loop;
			goto done1;
		}
		goto loop;
	}
	smp_unlock(&lk_bio);
	splx(s);

done1:
	if (bflush_debug) {
	mprintf("bflush %d,%d gp 0x%x n %d free %d %d %d %d - %d chk %d wrt %d\n",
	       major(dev), minor(dev), gp, nbuf, n_dirty, n_clean, n_empty,
	       n_busy, nbuf-n_clean-n_dirty-n_empty, n_look, flushed);
        }

	/* account for number of buffers inspected and pushed */
	bd->look += n_look;
	bd->flush += flushed;

	/* If requested, wait for busy buffers. */
	/* Look only at the busy queue. */
	/* If we find a buffer that fits the bill, sleep and start over. */
	if (!waitfor)
		return (flushed);

	start_time = time.tv_sec; /* remember our start time */
	n_look = 0;
	flist = &bbusylist;
	s = splbio();

	if (bflush_debug) {
		smp_lock(&lk_bio, LK_RETRY);
		for (bp = flist->busy_forw; bp != flist; bp = bp->busy_forw) {
			++n_busy;
		}
		if (dev == NODEV)
			printf("bflush busy: %d busy\n", n_busy);
		else
			mprintf("bflush busy: %d busy\n", n_busy);
		smp_unlock(&lk_bio);
	}

loop2:
	smp_lock(&lk_bio, LK_RETRY);
	for (bp = flist->busy_forw; bp != flist; bp = bp->busy_forw) {
		/* put an upper bound on how many times we'll sleep */
		if (n_sleep == nbuf) { /* we've tried long enough! */
			++bb->loop;
			goto done2;
		}
		++n_look;
		/* look only at writes for real devs */
		if (bp->b_flags & B_READ || bp->b_dev == NODEV ||
		    bp->state & B_INVAL)
			continue;
		if (gp) { /* gp, if specified, has priority */
			if (!matchgp(bp, gp))
				continue;
		}
		else { /* gp was not specified */
			if (dev == NODEV) { /* dev was wildcarded */
				/*
				 * special case (i.e. hack):   
				 * if NODEV, i.e. coming down, don't sleep
				 * on anything but local buffers.
				 */
				if (major(bp->b_dev) >= nblkdev)
					continue;
			}
			else if (dev != bp->b_dev) /* unique dev specified */
				continue;	   /* but doesn't match */
		}
		/* we have found a busy buffer that interests us */
		/* skip ones that have arrived since we started */ 
		if (bp->busy_time > start_time)
			continue;
		if (!(bp->b_flags & B_DONE)) {
			++n_sleep;
			bp->state |= B_WANTED;
			sleep_unlock((caddr_t)bp, PRIBIO, &lk_bio);
			goto loop2;
		}
	}
done2:
	smp_unlock(&lk_bio);
	splx(s);


	if (bflush_debug) {
		if (dev == NODEV)		
			printf("bflush busy: n %d chk %d sleep %d\n",
			       n_busy, n_look, n_sleep);
		else
			mprintf("bflush busy: n %d chk %d sleep %d\n",
				n_busy, n_look, n_sleep);
	}

	/* account for number of buffers inspected and slept on */
	bb->look += n_look;
	bb->sleep += n_sleep;
	if (n_sleep > flushed)
		++bb->more;

	return (flushed);
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 */
geterror(bp)
	register struct buf *bp;
{
	register int error = 0;

	if (bp->b_flags&B_ERROR)
		if ((error = bp->b_error)==0)
			return (EIO);
	return (error);
}

/*
 * Invalidate in core blocks belonging to closed or umounted filesystem
 *
 * This is not nicely done at all - the buffer ought to be removed from the
 * hash chains & have its dev/blkno fields clobbered, but unfortunately we
 * can't do that here, as it is quite possible that the block is still
 * being used for i/o. Eventually, all disc drivers should be forced to
 * have a close routine, which ought ensure that the queue is empty, then
 * properly flush the queues. Until that happy day, this suffices for
 * correctness.						... kre
 */
binval(dev, gp)
	dev_t dev;
	register struct gnode *gp;
{
	register struct buf *bp;
	register struct bufhd *hp;
	register int s;
#define dp ((struct buf *)hp)

	++bufstats.binval_call;
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (hp = bufhash; hp < &bufhash[bufhsz]; hp++) {
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
			++bufstats.binval_look;
			if ((bp->b_dev == dev) || (gp && matchgp(bp, gp))) {
				++bufstats.binval_inval;
				bp->state |= B_INVAL;
			}
		}
	}
	smp_unlock(&lk_bio);
	splx(s);
}

/*
 * Invalidate all buffers that are holding soft references
 * to gnodes (for now this is NFS only).
 */
binvalallgid()
{
	register struct buf *bp;
	register struct bufhd *hp;
	register int s;
#define dp ((struct buf *)hp)

	++bufstats.binvalallgid_call;
	s = splbio();
	smp_lock(&lk_bio, LK_RETRY);
	for (hp = bufhash; hp < &bufhash[bufhsz]; hp++) {
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
			if (bp->b_gp)
				bp->state |= B_INVAL;
		}
	}
	smp_unlock(&lk_bio);
	splx(s);
}

/*
 * Invalidate blocks associated with gp which are on the clean list.
 * Make sure all write-behind blocks associated with gp are flushed out.
 * Used only for NFS buffers.
 */

int binvalfree_inval=0;

binvalfree(gp)
struct gnode *gp;
{
	register struct buf *bp;
	register struct buf *flist;
	register int s;

	++bufstats.binvalfree_call;
	bflush(NODEV, gp, 0); /* flush delayed write blocks asynchronously */

	/*
	 * The cheap, dirty, fast way to invalidate buffers
	 * is to disassociate the gnode from them.
	 * This leaves the buffers in place in the clean list, however.
	 * A more laborious (historical) method is to search them out
	 * and place them at the head of the clean list for re-use.
	 */
	if (binvalfree_inval) {
		flist = &bfreelist[BQ_CLEAN];
		s = splbio();
		smp_lock(&lk_bio, LK_RETRY);
		for (bp = flist->av_forw; bp != flist; bp = bp->av_forw) {
			++bufstats.binvalfree_look;
			if (!matchgp(bp, gp))
				continue;
			++bufstats.binvalfree_inval;
			bp->state |= B_INVAL;
		}
		smp_unlock(&lk_bio);
		(void) splx(s);
	}
	else
		cacheinval(gp);
}
