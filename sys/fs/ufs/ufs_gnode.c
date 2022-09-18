#ifndef lint
static	char	*sccsid = "@(#)ufs_gnode.c	4.6	(ULTRIX)	4/25/91";
#endif lint

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

/* ---------------------------------------------------------------------
 * Modification History: /sys/sys/ufs_gnode.c
 *
 * 10 Apr 91 -- prs
 *	Fixed ufs_gtrunc() to allocate a block if truncating into a hole.
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 22-Jan-91 -- prs
 *      Added fast symbolic link support.
 *
 *  9 Mar 90 -- chet
 *	Fix inode generation number incrementation.
 *
 * 25 Jul 89 -- chet
 *	Changes for syncronous filesystems and remove old debugging code
 *
 * 06 Apr 89 -- prs
 *	Added SMP quota locking.
 *
 * 28 Jul 88 -- prs
 *	SMP - system call work.
 *
 * 10 Feb 88 -- prs
 *	Modified to support new fifo code.
 *
 * 08 Feb 88 -- prs
 *	Removed call to dqrele in ufs_gget. The call is now done
 *	in getegnode.
 *
 * 14 Jan 88 -- chet
 *	Added gennum++ in ufs_grele when freeing the inode for reuse
 *
 * 28 Sep 87 -- prs
 *	Added a call to ufs_gunlock in the B_ERROR case of ufs_gget.
 *
 * 25 Aug 87 -- prs
 *	Removed a gput call in ufs_gget in the B_ERROR case, that
 *	would cause the system to panic if condition happened twice.
 *
 * 14 Jul 87 -- cb
 *	Code Cleanup - removed some lingering device special code.
 *
 * 08 May 87 -- logcher
 *	Added else for rdev from new rdev sync described in 01-Apr
 *	problem.
 *
 * 01 Apr 87 -- logcher
 *	Added fix for 0, 0 problem in ufs_gget()
 *
 * 02 Mar 87 -- logcher
 *	Merged in diskless changes, added support for new ufs_ops
 *	and specfs
 *
 * 29 Jan 87 -- chet
 *	add new arg to bdwrite() calls.
 *
 * 03 Nov 86 -- bglover for koehler
 *	change bread call indirtrunc rtn (final parameter NULL)
 *
 * 11 Sep 86 -- koehler
 *	gfs changes -- check locking and do synchronous fs writes
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 15 Mar 85 -- funding
 *	Added named pipe support (re. System V named pipes)
 *
 * 16 Oct 84 -- jrs
 *	Fix hash, quota release, add igrab and nami cache support
 *
 * 17 Jul 84 -- jmcg
 *	Inserted code to track lockers and unlockers of inodes for
 *	debugging.  Conditionally compiled by defining RECINODELOCKS.
 *	Changed a blind lock to an GLOCK (based on suspicions of a
 *	bug from the net).
 *
 * 17 Jul 84 --jmcg
 *	Began modification history with sources from ULTRIX-32, Rel. V1.0.
 *	Derived from 4.2BSD, labeled:
 *		ufs_inode.c	6.1	83/07/29
 *
 * ---------------------------------------------------------------------
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode_common.h"
#include "../ufs/ufs_inode.h"
#include "../h/gnode.h"
#include "../ufs/fs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#ifdef QUOTA
#include "../h/quota.h"
#endif
#include "../h/kernel.h"
#include "../h/fs_types.h"
#include "../h/cmap.h"

#define OSF_FASTLINK 0x0001

extern struct gnode_ops *ufs_gnode_ops;

int
ufs_ginit(gp, iflag, ptr)
	struct gnode *gp;
	int iflag;
	caddr_t ptr;
{
	struct mount *mp = gp->g_mp;
	struct fs *fs;
	struct buf *bp;
	struct ufs_inode *dp;	
	int type;
		
	gp->g_ops = ufs_gnode_ops;

	if (iflag == RECLAIM_GNODE)
		panic("ufs reclaim trying too hard");

	fs = mp->m_bufp->b_un.b_fs;
	bp = bread(mp->m_dev, fsbtodb(fs, itod(fs, gp->g_number)),
		(int)fs->fs_bsize, (struct gnode *) NULL);
			/*
	 * Check I/O errors
	 */
	if ((bp->b_flags&B_ERROR) != 0) {
		brelse(bp);
		return(0);
	}
	dp = bp->b_un.b_dino;
	dp += itoo(fs, gp->g_number);
	G_TO_I(gp)->di_ic = dp->di_ic;
	gp->g_blocks = G_TO_I(gp)->di_blocks;
	gp->g_gennum = G_TO_I(gp)->di_gennum;

	/*
	 * check for specvp
 	 */
	type = gp->g_mode & GFMT;
	if ((type == GFCHR) || (type == GFBLK) || (type == GFPORT)) {
		gp->g_rdev = G_TO_I(gp)->di_rdev;
		specvp(gp);
	} else
		gp->g_rdev = NODEV;
	brelse(bp);
	return(1);
}

/*
 * Make a locked gnode inactive.  Another process may create a reference
 * to this gnode while we're operating on it, but it won't be able to
 * lock it until we're done.  By then we will have marked it so the
 * other process will know to initialize it again.
	 */

ufs_inactive(gp)
	register struct gnode *gp;
{
	register int mode;

	gassert(gp);

	if (gp->g_nlink <= 0) {
		mode = gp->g_mode & GFMT;
		if ((mode == GFBLK) || (mode == GFCHR))
			gp->g_rdev = G_TO_I(gp)->di_rdev = 0;
		gp->g_nlink = 0;				
		gp->g_mode = 0;
		G_TO_I(gp)->di_flags = 0; /* For OSF_FASTLINK */
		G_TO_I(gp)->di_gennum = ++(gp->g_gennum);
		/*
		 * gtrunc does the ufs_gupdat()
		 */
		ufs_gtrunc(gp, (u_long)0, (struct ucred *) 0);
		ufs_gfree(gp, gp->g_number, mode);
#ifdef QUOTA
		(void) chkiq(gp->g_dev, gp, gp->g_uid, 0);
		dquot_lock(gp->g_dquot);
		dqrele(gp->g_dquot);
		dquot_unlock(gp->g_dquot);
		gp->g_dquot = NODQUOT;
#endif
	}

	ufs_gupdat(gp, timepick, timepick, 0, (struct ucred *) 0);
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */
/*
 * Truncate the gnode gp to at most
 * length size.  Free affected disk
 * blocks -- the blocks of the file
 * are removed in reverse order.
 *
 * NB: triple indirect blocks are untested.
 */
ufs_gtrunc(ogp, length, cred)
	register struct gnode *ogp;
	u_long length;
	struct ucred *cred;
{
	register daddr_t lastblock;
	daddr_t bn, lbn, lastiblock[NIADDR];
	register struct fs *fs;
	register struct ufs_inode *ufs_ip;
	struct buf *bp;
	int offset, osize, size, count, level, s;
	int updat_flag = 0;
	long nblocks, blocksreleased = 0;
	register int i;
	struct gnode tip;
	long indirtrunc();
	extern struct cmap *mfind();

	/*
	 * Disable the trunc up case for now. We have to worry about
	 * special files before this feature can be enabled.
	 */	
	if (length >= ogp->g_size) {
		ogp->g_flag |= GCHG|GUPD;
                ufs_gupdat(ogp, timepick, timepick, 0, (struct ucred *) 0);
		return(0);
	}
	/*
	 * Fast symbolic links have no storage.  Can truncate in place.
	 */
	if ((G_TO_I(ogp)->di_flags & OSF_FASTLINK) &&
	    (ogp->g_size > length)) {
		bzero(&(G_TO_I(ogp)->di_db[length]), ogp->g_size - length);
		ogp->g_size = length;
		ogp->g_flag |= GCHG|GUPD;
 		ufs_gupdat(ogp, timepick, timepick, 0, (struct ucred *) 0);
		return;
	}

	fs = FS(ogp);
	offset = blkoff(fs, length);
	lbn = lblkno(fs, length - 1);

	if (length > ogp->g_size) {
		/*
		 * Trunc up case.  ufs_bmap will insure that the right blocks
		 * are allocated.  This includes extending the old frag to a
		 * full block (if needed) in addition to doing any work
		 * needed for allocating the last block.
		 */
		if (offset == 0)
			bn = ufs_bmap(ogp, lbn, B_WRITE, (int)fs->fs_bsize,
				&updat_flag);
		else
			bn = ufs_bmap(ogp, lbn, B_WRITE, offset, &updat_flag);
 
		if (u.u_error == 0 || bn >= (daddr_t)0) {
			ogp->g_size = length;
			ogp->g_flag |= GCHG;
		}
 
		if (updat_flag != 0)
			ufs_gupdat(ogp, timepick, timepick, 1,
				   (struct ucred *) 0);
		else
			ufs_gupdat(ogp, timepick, timepick, 0,
				   (struct ucred *) 0);
		return (u.u_error);
	}
	/*
	 * Calculate index into gnode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Update the size of the file. If the file is not being
	 * truncated to a block boundry, the contents of the
	 * partial block following the end of the file must be
	 * zero'ed in case it ever become accessable again because
	 * of subsequent file growth.
	 */
	osize = ogp->g_size;
	if (offset == 0) {
		ogp->g_size = length;
	} else {
		bn = ufs_bmap(ogp, lbn, B_WRITE, offset, 0);
		if (u.u_error || (long)bn < 0)
			return(u.u_error);
		ogp->g_size = length;
		size = blksize(fs, ogp, lbn);
		count = howmany(size, DEV_BSIZE);
		s = splimp();
		for (i = 0; i < count; i += CLSIZE) {
			smp_lock(&lk_cmap, LK_RETRY);
			if (mfind(ogp->g_dev, bn + i, ogp))
				munhash(ogp->g_dev, bn + i, ogp);
			smp_unlock(&lk_cmap);
		}
		splx(s);
		bp = bread(ogp->g_dev, bn, size, (struct gnode *)0);
		if (bp->b_flags & B_ERROR) {
			u.u_error = EIO;
			ogp->g_size = osize;
			brelse(bp);
			return(EIO);
		}
		bzero(bp->b_un.b_addr + offset, (unsigned)(size - offset));
		bdwrite(bp);
	}
	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 */
	tip = *ogp;			/*  structure copy */
	tip.g_size = osize;
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			G_TO_I(ogp)->di_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--) {
		G_TO_I(ogp)->di_db[i] = 0;
	}
	
	ogp->g_size = length;
	ogp->g_flag |= GCHG|GUPD;
	ufs_gupdat(ogp, timepick, timepick, 1, (struct ucred *) 0);

	/*
	 * Indirect blocks first.
	 */
	ufs_ip = G_TO_I(&tip);
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ufs_ip->di_ib[level];
		if (bn != 0) {
			blocksreleased +=
			indirtrunc(&tip, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				ufs_ip->di_ib[level] = 0;
				free(&tip, bn, (off_t)fs->fs_bsize);
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		register int bsize;

		bn = ufs_ip->di_db[i];
		if (bn == 0)
			continue;
		ufs_ip->di_db[i] = 0;
		bsize = (off_t)blksize(fs, &tip, i);
		free(&tip, bn, bsize);
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ufs_ip->di_db[lastblock];
	if (bn != 0) {
		int oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, &tip, lastblock);
		tip.g_size = length;
		newspace = blksize(fs, &tip, lastblock);
		if (newspace == 0)
			panic("ufs_gtrunc: newspace");
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			free(&tip, bn, oldspace - newspace);
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
/* BEGIN PARANOIA */
	for (level = SINGLE; level <= TRIPLE; level++)
		if (ufs_ip->di_ib[level] != G_TO_I(ogp)->di_ib[level])
			panic("ufs_gtrunc1");
	for (i = 0; i < NDADDR; i++)
		if (ufs_ip->di_db[i] != G_TO_I(ogp)->di_db[i])
			panic("ufs_gtrunc2");
/* END PARANOIA */
	ogp->g_blocks -= blocksreleased;
	G_TO_I(ogp)->di_blocks = ogp->g_blocks;
	if (G_TO_I(ogp)->di_blocks < 0) {		/* sanity */
		ogp->g_blocks = G_TO_I(ogp)->di_blocks = 0;
	}
	
	ogp->g_flag |= GCHG;
#ifdef QUOTA
	(void) chkdq(ogp, -blocksreleased, 0);
#endif
	return(0);
}

/*
 * Release blocks associated with the gnode gp and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * NB: triple indirect blocks are untested.
 */
long
indirtrunc(gp, bn, lastbn, level)
	register struct gnode *gp;
	daddr_t bn, lastbn;
	int level;
{
	register int i;
	struct buf *bp, *copy;
	register daddr_t *bap;
	register struct fs *fs = FS(gp);
	register daddr_t nb;
	daddr_t last;
	register long factor;
	int blocksreleased = 0, nblocks;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
/*
	printf("indirtrunc: fs 0x%x\n", fs);	
*/
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those 
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = geteblk((int)fs->fs_bsize);
	bp = bread(gp->g_dev, fsbtodb(fs, bn), (int)fs->fs_bsize, 
		(struct gnode *) NULL);
	if (bp->b_flags&B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
	  (u_int)(NINDIR(fs) - (last + 1)) * sizeof (daddr_t));
	bwrite(bp);
	bp = copy, bap = bp->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
		nb = bap[i];
		if (nb == 0)
			continue;
		if (level > SINGLE)
			blocksreleased +=
			    indirtrunc(gp, nb, (daddr_t)-1, level - 1);
		free(gp, nb, (int)fs->fs_bsize);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0)
			blocksreleased += indirtrunc(gp, nb, last, level - 1);
	}
	brelse(bp);
	return (blocksreleased);
}

/*
 * Lock a gnode. If its already locked, set the WANT bit and sleep.
 */
ufs_glock(gp)
	register struct gnode *gp;
{
	if (gp->g_mp->m_fstype != GT_ULTRIX) {
		printf("ufs_glock: gp 0x%x type %d\n", gp,
		gp->g_mp->m_fstype);
		panic("ufs_glock: gp type not GT_ULTRIX");
	}
	gfs_lock(gp);
}

/*
 * Unlock a gnode.  If WANT bit is on, wakeup.
 */
ufs_gunlock(gp)
	register struct gnode *gp;
{
	if (!glocked(gp)) {
		cprintf("ufs_gunlock: gp unlocked, dev 0x%x gno %d\n",
		gp->g_dev, gp->g_number);
		panic("ufs_gunlock");
	}
	if (gp->g_mp->m_fstype != GT_ULTRIX) {
		printf("ufs_gulock: gp 0x%x type %d\n", gp,
		gp->g_mp->m_fstype);
		panic("ufs_gulock: gp type not GT_ULTRIX");
	}
	gfs_unlock(gp);
}

/*
 * Check accessed and update flags on
 * a gnode structure.
 * If any is on, update the gnode
 * with the appropriate time.
 * If waitfor is given, then must insure
 * i/o order so wait for write to complete.
 */
struct timeval guniqtm;	/* unique UFS timestamp */
extern struct lock_t lk_gnode;
extern struct timeval atime;
extern struct timeval btime;

ufs_gupdat(gp, ta, tm, waitfor, cred)
	register struct gnode *gp;
	register struct timeval *ta, *tm;
	int waitfor;
	struct ucred *cred;
{
	register struct buf *bp;
	register struct ufs_inode *dp;
	register struct fs *fs;

	gassert(gp);

	if ((gp->g_flag & (GUPD|GACC|GCHG|GMOD)) != 0) {
		fs = FS(gp);
		G_TO_I(gp)->di_blocks = gp->g_blocks;
		if (((gp->g_mode & GFMT) == GFCHR) ||
		    ((gp->g_mode & GFMT) == GFBLK))
			G_TO_I(gp)->di_rdev = gp->g_rdev;

		if (fs->fs_ronly)
			return(EROFS);
		bp = bread(gp->g_dev, fsbtodb(fs, itod(fs, gp->g_number)),
			(int)fs->fs_bsize, (struct gnode *) NULL);
			
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return(EIO);
		}
		if ((gp->g_flag & (GUPD|GACC|GCHG)) != 0) {
			/*
			 * Mark gnode with the current (unique) timestamp.
			 * We need to synchronize with a spin lock to do this,
			 * lk_gnode seems reasonable to me -- chet.
			 */
			smp_lock(&lk_gnode, LK_RETRY);
			if (timepick->tv_sec > guniqtm.tv_sec ||
			    timepick->tv_usec > guniqtm.tv_usec)
				guniqtm = *(timepick);
			else
				guniqtm.tv_usec++;
			if (gp->g_flag & GACC) {
				if (ta == &atime || ta == &btime)
					gp->g_atime = guniqtm;
				else
					gp->g_atime= *ta;
			}
			if (gp->g_flag & GUPD) {
				if (tm == &atime || tm == &btime)
					gp->g_mtime = guniqtm;
				else
					gp->g_mtime = *tm;
			}
			if (gp->g_flag & GCHG)
				gp->g_ctime = guniqtm;

			smp_unlock(&lk_gnode);
		}
		
		gp->g_flag &= ~(GUPD|GACC|GCHG|GMOD);

		dp = bp->b_un.b_dino + itoo(fs, gp->g_number);
		bcopy(&G_TO_I(gp)->di_ic, &dp->di_ic,
		      sizeof(struct ufs_inode));
		if (waitfor)
			bwrite(bp);
		else {
			if (gp->g_mp->m_flags & M_SYNC)
				bwrite(bp);
			else
				bdwrite(bp);
		}
	}
	return(u.u_error);
}
