#ifndef lint
static	char	*sccsid = "@(#)ufs_alloc.c	4.3	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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

/* ------------------------------------------------------------------------
 * Modification History:
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 *  2 Jan 91 -- prs
 *	Enhanced blkpref() by adding an alogorithm which will
 *	attempt to make the slightly random writer block allocations 
 *	sequential. This is accomplished by looking for a block close
 *	to the one being allocated to determine block preference.
 *
 * 12 Jan 90 -- prs
 *	Removed gp had blocks message.
 *
 * 25 Jul 89 -- chet
 *	Fix syncronous filesystems and meet new bdwrite() interface.
 *
 * 25 Jul 88 -- jmartin
 *	Lock mfind/munhash operation with lk_cmap.
 *
 * 19 May 88 -- prs
 * 	SMP - Added ufs locks.
 *
 * 29 Jan 87 -- chet
 *	add new arg to bdwrite() calls
 *
 * 14 Jan 87 -- rr
 *	change of bwrite back to bdwrite
 *
 * 11 Sep 86 -- koehler
 *	change of panic string to reflect reality
 *
 * 06 Nov 84 -- jrs
 *	Add macro definitions and speedups from latest Berkeley stuff
 *
 * 19 Feb 84 -- jmcg
 *	There are three instances of buffers not being properly released.
 *	This results in system hangs after some delay.  The fix has been
 *	"blessed" by Kirk McKusick and verified by Robert Elz.
 *
 * 19 Feb 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		ufs_alloc.c	6.2	83/09/28
 *
 * ------------------------------------------------------------------------
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../ufs/fs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/gnode_common.h"
#include "../ufs/ufs_inode.h"
#include "../h/gnode.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/quota.h"
#include "../h/kernel.h"
#include "../h/fs_types.h"
#include "../h/cmap.h"

extern u_long		hashalloc();
extern gno_t		ialloccg();
extern daddr_t		alloccg();
extern daddr_t		alloccgblk();
extern daddr_t		fragextend();
extern daddr_t		blkpref();
extern daddr_t		mapsearch();
extern int		inside[], around[];
extern unsigned char	*fragtbl[];

/*
 * Allocate a block in the file system.
 * 
 * The size of the requested block is given, which must be some
 * multiple of fs_fsize and <= fs_bsize.
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate a block:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate a block in the same cylinder group.
 *   4) quadradically rehash into other cylinder groups, until an
 *      available block is located.
 * If no block preference is given the following heirarchy is used
 * to allocate a block:
 *   1) allocate a block in the cylinder group that contains the
 *      gnode for the file.
 *   2) quadradically rehash into other cylinder groups, until an
 *      available block is located.
 */
struct buf *
alloc(gp, bpref, size)
	register struct gnode *gp;
	daddr_t bpref;
	register int size;
{
	register daddr_t bno;
	register struct fs *fs;
	register struct buf *bp;
	register int cg;

	fs = FS(gp);
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    gp->g_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("alloc: bad size");
	}
	if (size == fs->fs_bsize && fs->fs_cstotal.cs_nbfree == 0)
		goto nospace;
	if (u.u_uid != 0 && freespace(fs, fs->fs_minfree) <= 0)
		goto nospace;
#ifdef QUOTA
	u.u_error = chkdq(gp, (long)btodb(size), 0);
	if (u.u_error)
		return (NULL);
#endif
	if (bpref >= fs->fs_size)
		bpref = 0;
	if (bpref == 0)
		cg = itog(fs, gp->g_number);
	else
		cg = dtog(fs, bpref);
	bno = (daddr_t)hashalloc(gp, cg, (long)bpref, size,
		(u_long (*)())alloccg);
	if (bno <= 0)
		goto nospace;
	gp->g_blocks += btodb(size);
	G_TO_I(gp)->di_blocks = gp->g_blocks;
	gp->g_flag |= GUPD|GCHG;
	bp = getblk(gp->g_dev, fsbtodb(fs, bno), size, (struct gnode *) NULL);
	clrbuf(bp);
	return (bp);
nospace:
	fserr(gp->g_mp->m_fs_data->fd_path, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * Reallocate a fragment to a bigger size
 *
 * The number and size of the old block is given, and a preference
 * and new size is also specified. The allocator attempts to extend
 * the original block. Failing that, the regular block allocator is
 * invoked to get an appropriate block.
 */
struct buf *
realloccg(gp, bprev, bpref, osize, nsize)
	register struct gnode *gp;
	daddr_t bprev, bpref;
	int osize, nsize;
{
	register struct fs *fs;
	register struct buf *bp, *obp;
	int cg, request;
	register daddr_t bno;
	daddr_t bn;
	int i, count, s;
	extern struct cmap *mfind();
	
	fs = FS(gp);
	if ((unsigned)osize > fs->fs_bsize || fragoff(fs, osize) != 0 ||
	    (unsigned)nsize > fs->fs_bsize || fragoff(fs, nsize) != 0) {
		printf("dev = 0x%x, bsize = %d, osize = %d, nsize = %d, fs = %s\n",
		    gp->g_dev, fs->fs_bsize, osize, nsize, fs->fs_fsmnt);
		panic("realloccg: bad size");
	}
	if (u.u_uid != 0 && freespace(fs, fs->fs_minfree) <= 0)
		goto nospace;
	if (bprev == 0) {
		printf("dev = 0x%x, bsize = %d, bprev = %d, fs = %s\n",
		    gp->g_dev, fs->fs_bsize, bprev, fs->fs_fsmnt);
		panic("realloccg: bad bprev");
	}
#ifdef QUOTA
	u.u_error = chkdq(gp, (long)btodb(nsize - osize), 0);
	if (u.u_error)
		return (NULL);
#endif
	cg = dtog(fs, bprev);
	bno = fragextend(gp, cg, (long)bprev, osize, nsize);
	if (bno != 0) {
		do {
			bp = bread(gp->g_dev, fsbtodb(fs, bno), osize, 
				   (struct gnode *) NULL);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return (NULL);
			}
		} while (brealloc(bp, nsize) == 0);
		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
		gp->g_blocks += btodb(nsize - osize);
		G_TO_I(gp)->di_blocks = gp->g_blocks;
		gp->g_flag |= GUPD|GCHG;
		bp->b_flags |= B_DONE;
		return (bp);
	}
	if (bpref >= fs->fs_size)
		bpref = 0;
	fs_lock(gp->g_mp);
	switch ((int)fs->fs_optim) {
	case FS_OPTSPACE:
		/*
		 * Allocate an exact sized fragment. Although this makes 
		 * best use of space, we will waste time relocating it if 
		 * the file continues to grow. If the fragmentation is
		 * less than half of the minimum free reserve, we choose
		 * to begin optimizing for time.
		 */
		request = nsize;
		if (fs->fs_minfree < 5 ||
		    fs->fs_cstotal.cs_nffree >
		    fs->fs_dsize * fs->fs_minfree / (2 * 100))
			break;
		fs->fs_optim = FS_OPTTIME;
		break;
	case FS_OPTTIME:
		/*
		 * At this point we have discovered a file that is trying
		 * to grow a small fragment to a larger fragment. To save
		 * time, we allocate a full sized block, then free the 
		 * unused portion. If the file continues to grow, the 
		 * `fragextend' call above will be able to grow it in place
		 * without further copying. If aberrant programs cause
		 * disk fragmentation to grow within 2% of the free reserve,
		 * we choose to begin optimizing for space.
		 */
		request = fs->fs_bsize;
		if (fs->fs_cstotal.cs_nffree <
		    fs->fs_dsize * (fs->fs_minfree - 2) / 100)
			break;
		fs->fs_optim = FS_OPTSPACE;
		break;
	default:
		printf("dev = 0x%x, optim = %d, fs = %s\n",
		    gp->g_dev, fs->fs_optim, fs->fs_fsmnt);
		panic("realloccg: bad optim");
		/* NOTREACHED */
	}
	fs_unlock(gp->g_mp);
	bno = (daddr_t)hashalloc(gp, cg, (long)bpref, request,
		(u_long (*)())alloccg);
	if (bno > 0) {
		obp = bread(gp->g_dev, fsbtodb(fs, bprev), osize, 
			    (struct gnode *) NULL);
		if (obp->b_flags & B_ERROR) {
			brelse(obp);
			return (NULL);
		}
		bn = fsbtodb(fs, bno);
		bp = getblk(gp->g_dev, bn, nsize, (struct gnode *) NULL);
		bcopy(obp->b_un.b_addr, bp->b_un.b_addr, (u_int)osize);
		count = howmany(osize, DEV_BSIZE);

		/*
		 * mfind/munhash can be a lengthy operation, so we
		 * lock/unlock through each circuit of the loop.
		 */
		s = splimp();
		for (i = 0; i < count; i += CLBYTES / DEV_BSIZE) {
			smp_lock(&lk_cmap, LK_RETRY);
			if (mfind(gp->g_dev, bn + i, gp))
				munhash(gp->g_dev, bn + i, gp);
			smp_unlock(&lk_cmap);
		}
		splx(s);

		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
		if (obp->b_flags & B_DELWRI) {
			obp->b_flags &= ~B_DELWRI;
			u.u_ru.ru_oublock--;		/* delete charge */
		}
		brelse(obp);
		free(gp, bprev, (off_t)osize);
		if (nsize < request)
			free(gp, bno + numfrags(fs, nsize),
				(off_t)(request - nsize));
		gp->g_blocks += btodb(nsize - osize);
		G_TO_I(gp)->di_blocks = gp->g_blocks;
		gp->g_flag |= GUPD|GCHG;
		return (bp);
	}
nospace:
	/*
	 * no space available
	 */
	fserr(gp->g_mp->m_fs_data->fd_path, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * Allocate a gnode in the file system.
 * 
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate an gnode:
 *   1) allocate the requested gnode.
 *   2) allocate an gnode in the same cylinder group.
 *   3) quadradically rehash into other cylinder groups, until an
 *      available gnode is located.
 * If no gnode preference is given the following heirarchy is used
 * to allocate an gnode:
 *   1) allocate an gnode in cylinder group 0.
 *   2) quadradically rehash into other cylinder groups, until an
 *      available gnode is located.
 */
struct gnode *
ufs_galloc(pgp, gpref, mode)
	register struct gnode *pgp;
	register gno_t gpref;
	int mode;
{
	register gno_t gno;
	register struct fs *fs;
	register struct gnode *gp;
	register int cg;
	
	fs = FS(pgp);
	if (fs->fs_cstotal.cs_nifree == 0)
		goto nognodes;
#ifdef QUOTA
	u.u_error = chkiq(pgp->g_dev, (struct gnode *)NULL, u.u_uid, 0);
	if (u.u_error)
		return (NULL);
#endif
	if (gpref >= fs->fs_ncg * fs->fs_ipg)
		gpref = 0;
	cg = itog(fs, gpref);
	gno = (gno_t)hashalloc(pgp, cg, (long)gpref, mode, ialloccg);
	if (gno == 0)
		goto nognodes;
	gp = gget(pgp->g_mp, gno, 0);
	if (gp == NULL) {
		ufs_gfree(pgp, gno, 0);
		return (NULL);
	}
	if (gp->g_mp->m_fstype != GT_ULTRIX) 
		panic("ufs_galloc: gget returned wrong fs type");
	if (gp->g_mode) {
		printf("mode = 0%o, gnum = %d, fs = %s\n",
		    gp->g_mode, gp->g_number, fs->fs_fsmnt);
		panic("ufs_galloc: dup alloc");
	}
	if (gp->g_blocks) {				/* XXX */
		gp->g_blocks = 0;
	}
	return (gp);
nognodes:
	fserr(pgp->g_mp->m_fs_data->fd_path, "out of gnodes");
	uprintf("\n%s: create/symlink failed, no gnodes free\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * Find a cylinder to place a directory.
 *
 * The policy implemented by this algorithm is to select from
 * among those cylinder groups with above the average number of
 * free gnodes, the one with the smallest number of directories.
 */
gno_t
dirpref(fs)
	register struct fs *fs;
{
	register int cg, minndir, mincg, avgifree;

	avgifree = fs->fs_cstotal.cs_nifree / fs->fs_ncg;
	minndir = fs->fs_ipg;
	mincg = 0;
	for (cg = 0; cg < fs->fs_ncg; cg++)
		if (fs->fs_cs(fs, cg).cs_ndir < minndir &&
		    fs->fs_cs(fs, cg).cs_nifree >= avgifree) {
			mincg = cg;
			minndir = fs->fs_cs(fs, cg).cs_ndir;
		}
	return ((gno_t)(fs->fs_ipg * mincg));
}

/*
 * Select the desired position for the next block in a file.  The file is
 * logically divided into sections. The first section is composed of the
 * direct blocks. Each additional section contains fs_maxbpg blocks.
 * 
 * If no blocks have been allocated in the first section, the policy is to
 * request a block in the same cylinder group as the gnode that describes
 * the file. If no blocks have been allocated in any other section, the
 * policy is to place the section in a cylinder group with a greater than
 * average number of free blocks.  An appropriate cylinder group is found
 * by using a rotor that sweeps the cylinder groups. When a new group of
 * blocks is needed, the sweep begins in the cylinder group following the
 * cylinder group from which the previous allocation was made. The sweep
 * continues until a cylinder group with greater than the average number
 * of free blocks is found. If the allocation is for the first block in an
 * indirect block, the information on the previous allocation is unavailable;
 * here a best guess is made based upon the logical block number being
 * allocated.
 * 
 * If a section is already partially allocated, the policy is to
 * contiguously allocate fs_maxcontig blocks.  The end of one of these
 * contiguous blocks and the beginning of the next is physically separated
 * so that the disk head will be in transit between them for at least
 * fs_rotdelay milliseconds.  This is to allow time for the processor to
 * schedule another I/O transfer.
 */
daddr_t
blkpref(gp, lbn, indx, bap)
	register struct gnode *gp;
	daddr_t lbn;
	register int indx;
	daddr_t *bap;
{
	register struct fs *fs;
	register int cg;
	register int avgbfree, startcg;
	register daddr_t nextblk;
	int lookbehind = 1;
	extern int ufs_blkpref_lookbehind;

	fs = FS(gp);

	/*
	 * If we are writing at least the second block,
	 * try to locate the closest block, within
	 * ufs_blkpref_lookbehind blocks.
	 */
	if (ufs_blkpref_lookbehind > 1 && bap && indx > 0) {
		int floor;
		int ind;

		if (indx <= ufs_blkpref_lookbehind)
			floor = 0;
		else
			floor = indx - ufs_blkpref_lookbehind;
		/*
		 * Start at the block immediatly preceeding the block
		 * we want to allocate, and search until we find an
		 * allocated block, or until ufs_blkpref_lookbehind
		 * blocks have been searched.
		 */
		for (ind = indx - 1, lookbehind = 1; ind >= floor; 
		     ind--, lookbehind++)
			if (bap[ind] != 0)
				break;
		if (ind < floor)
			lookbehind = 1;
	}

	/*
	 * At this point, lookbehind is either set to one, meaning that
	 * no blocks within ufs_blkpref_lookbehind blocks preceeding
	 * block we want to allcate are currently allocated. Or if
	 * lookbehind > 1, bap[indx - lookbehind] is allocated, and
	 * we should determine our preference from that block. The former
	 * case will drop into the following if, the latter will jump
	 * around the if.
	 */
		
	if (indx % fs->fs_maxbpg == 0 || bap[indx - lookbehind] == 0) {
		if (lbn < NDADDR) {
			cg = itog(fs, gp->g_number);
			return (fs->fs_fpg * cg + fs->fs_frag);
		}
		/*
		 * Find a cylinder group with greater than average number of
		 * unused data blocks.
		 */
		if (indx == 0 || bap[indx - lookbehind] == 0)
			startcg = itog(fs, gp->g_number) + lbn / fs->fs_maxbpg;
		else
			startcg = dtog(fs, bap[indx - lookbehind]) + 1;
		startcg %= fs->fs_ncg;
		avgbfree = fs->fs_cstotal.cs_nbfree / fs->fs_ncg;
		for (cg = startcg; cg < fs->fs_ncg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		for (cg = 0; cg <= startcg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		return (NULL);
	}
	/*
	 * One or more previous blocks have been laid out. If less
	 * than fs_maxcontig previous blocks are contiguous, the
	 * next block is requested contiguously, otherwise it is
	 * requested rotationally delayed by fs_rotdelay milliseconds.
	 *
	 * lookbehind is set to the offset from indx in bap where a
	 * block is already allocated. Simply add (lookbehind * number
	 * of frags per block) to the block number of previosly allocated
	 * block, and we will have our preference.
	 */
	nextblk = bap[indx - lookbehind] + (lookbehind * fs->fs_frag);
#ifdef notdef
	if (indx > fs->fs_maxcontig &&
	    bap[indx - fs->fs_maxcontig] + blkstofrags(fs, fs->fs_maxcontig)
	    != nextblk)
		return (nextblk);
#endif
	if (fs->fs_rotdelay != 0 && indx != 0 && 
	    indx % fs->fs_maxcontig == 0) {
		/*
	 	 * fs->fs_rotdelay != 0 && new block is on 
		 * fs_maxcontig boundry.
	 	 *
	 	 * Here we convert ms of delay to frags as:
	 	 * (frags) = (ms) * (rev/sec) * (sect/rev) /
	 	 *	((sect/frag) * (ms/sec))
	 	 * then round up to the next block.
	 	 */
		nextblk += 
		   roundup(fs->fs_rotdelay * fs->fs_rps * fs->fs_nsect
			   /(NSPF(fs) * 1000), fs->fs_frag);
	}
		  
#ifdef notdef
	if (fs->fs_rotdelay != 0)
		nextblk += roundup(fs->fs_rotdelay * fs->fs_rps * fs->fs_nsect /
		   (NSPF(fs) * 1000), fs->fs_frag);
#endif

	return (nextblk);
}

/*
 * Implement the cylinder overflow algorithm.
 *
 * The policy implemented by this algorithm is:
 *   1) allocate the block in its requested cylinder group.
 *   2) quadradically rehash on the cylinder group number.
 *   3) brute force search for a free block.
 */
/*VARARGS5*/
u_long
hashalloc(gp, cg, pref, size, allocator)
	register struct gnode *gp;
	register int cg;
	long pref;
	int size;	/* size for data blocks, mode for gnodes */
	register u_long (*allocator)();
{
	register struct fs *fs;
	register long result;
	register int i;
	int icg = cg;

	fs = FS(gp);
	/*
	 * 1: preferred cylinder group
	 */
	result = (*allocator)(gp, cg, pref, size);
	if (result)
		return (result);
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->fs_ncg)
			cg -= fs->fs_ncg;
		result = (*allocator)(gp, cg, 0, size);
		if (result)
			return (result);
	}
	/*
	 * 3: brute force search
	 * Note that we start at i == 2, since 0 was checked initially,
	 * and 1 is always checked in the quadratic rehash.
	 */
	cg = (icg + 2) % fs->fs_ncg;
	for (i = 2; i < fs->fs_ncg; i++) {
		result = (*allocator)(gp, cg, 0, size);
		if (result)
			return (result);
		cg++;
		if (cg == fs->fs_ncg)
			cg = 0;
	}
	return (NULL);
}

/*
 * Determine whether a fragment can be extended.
 *
 * Check to see if the necessary fragments are available, and 
 * if they are, allocate them.
 */
daddr_t
fragextend(gp, cg, bprev, osize, nsize)
	register struct gnode *gp;
	int cg;
	long bprev;
	int osize, nsize;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	register long bno;
	int frags, bbase;
	register int i;

	fs = FS(gp);
	if (fs->fs_cs(fs, cg).cs_nffree < numfrags(fs, nsize - osize))
		return (NULL);
	frags = numfrags(fs, nsize);
	bbase = fragnum(fs, bprev);
	if (bbase > fragnum(fs, (bprev + frags - 1))) {
		/* cannot extend across a block boundry */
		return (NULL);
	}
	bp = bread(gp->g_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize,
		   (struct gnode *) NULL);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = timepick->tv_sec;
	bno = dtogd(fs, bprev);
	for (i = numfrags(fs, osize); i < frags; i++)
		if (isclr(cgp->cg_free, bno + i)) {
			brelse(bp);
			return (NULL);
		}
	/*
	 * the current fragment can be extended
	 * deduct the count on fragment being extended into
	 * increase the count on the remaining fragment (if any)
	 * allocate the extended piece
	 */
	for (i = frags; i < fs->fs_frag - bbase; i++)
		if (isclr(cgp->cg_free, bno + i))
			break;
	cgp->cg_frsum[i - numfrags(fs, osize)]--;
	if (i != frags)
		cgp->cg_frsum[i - frags]++;
	fs_lock(gp->g_mp);
	for (i = numfrags(fs, osize); i < frags; i++) {
		clrbit(cgp->cg_free, bno + i);
		cgp->cg_cs.cs_nffree--;
		fs->fs_cstotal.cs_nffree--;
		fs->fs_cs(fs, cg).cs_nffree--;
	}
	gp->g_mp->m_flags |= M_MOD;
	fs_unlock(gp->g_mp);
	if (gp->g_mp->m_flags & M_SYNC)
		bwrite(bp);
	else
		bdwrite(bp);
	return (bprev);
}

/*
 * Determine whether a block can be allocated.
 *
 * Check to see if a block of the apprpriate size is available,
 * and if it is, allocate it.
 */
daddr_t
alloccg(gp, cg, bpref, size)
	struct gnode *gp;
	int cg;
	daddr_t bpref;
	int size;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	register int bno;
	register int allocsiz;
	int frags;
	register int i;

	fs = FS(gp);
	if (fs->fs_cs(fs, cg).cs_nbfree == 0 && size == fs->fs_bsize)
		return (NULL);
	bp = bread(gp->g_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize,
		   (struct gnode *) NULL);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC ||
	    (cgp->cg_cs.cs_nbfree == 0 && size == fs->fs_bsize)) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = timepick->tv_sec;
	if (size == fs->fs_bsize) {
		bno = alloccgblk(fs, cgp, bpref, gp->g_mp);
	if (gp->g_mp->m_flags & M_SYNC)
		bwrite(bp);
	else
		bdwrite(bp);
		return (bno);
	}
	/*
	 * check to see if any fragments are already available
	 * allocsiz is the size which will be allocated, hacking
	 * it down to a smaller size if necessary
	 */
	frags = numfrags(fs, size);
	for (allocsiz = frags; allocsiz < fs->fs_frag; allocsiz++)
		if (cgp->cg_frsum[allocsiz] != 0)
			break;
	if (allocsiz == fs->fs_frag) {
		/*
		 * no fragments were available, so a block will be 
		 * allocated, and hacked up
		 */
		if (cgp->cg_cs.cs_nbfree == 0) {
			brelse(bp);
			return (NULL);
		}
		bno = alloccgblk(fs, cgp, bpref, gp->g_mp);
		bpref = dtogd(fs, bno);
		for (i = frags; i < fs->fs_frag; i++)
			setbit(cgp->cg_free, bpref + i);
		i = fs->fs_frag - frags;
		cgp->cg_cs.cs_nffree += i;

		fs_lock(gp->g_mp);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		gp->g_mp->m_flags |= M_MOD;
		fs_unlock(gp->g_mp);

		cgp->cg_frsum[i]++;
		if (gp->g_mp->m_flags & M_SYNC)
			bwrite(bp);
		else
			bdwrite(bp);
		return (bno);
	}
	bno = mapsearch(fs, cgp, bpref, allocsiz);
	if (bno < 0) {
		brelse(bp);
		return (NULL);
	}
	for (i = 0; i < frags; i++)
		clrbit(cgp->cg_free, bno + i);
	cgp->cg_cs.cs_nffree -= frags;

	fs_lock(gp->g_mp);
	fs->fs_cstotal.cs_nffree -= frags;
	fs->fs_cs(fs, cg).cs_nffree -= frags;
	gp->g_mp->m_flags |= M_MOD;
	fs_unlock(gp->g_mp);

	cgp->cg_frsum[allocsiz]--;
	if (frags != allocsiz)
		cgp->cg_frsum[allocsiz - frags]++;
	if (gp->g_mp->m_flags & M_SYNC)
		bwrite(bp);
	else
		bdwrite(bp);
	return (cg * fs->fs_fpg + bno);
}

/*
 * Allocate a block in a cylinder group.
 *
 * This algorithm implements the following policy:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate the next available block on the block rotor for the
 *      specified cylinder group.
 * Note that this routine only allocates fs_bsize blocks; these
 * blocks may be fragmented by the routine that allocates them.
 */
daddr_t
alloccgblk(fs, cgp, bpref, mp)
	register struct fs *fs;
	register struct cg *cgp;
	register daddr_t bpref;
	struct mount *mp;
{
	register daddr_t bno;
	int cylno, pos, delta;
	short *cylbp;
	register int i;

	if (bpref == 0) {
		bpref = cgp->cg_rotor;
		goto norot;
	}
	bpref = blknum(fs, bpref);
	bpref = dtogd(fs, bpref);
	/*
	 * if the requested block is available, use it
	 */
	if (isblock(fs, cgp->cg_free, fragstoblks(fs, bpref))) {
		bno = bpref;
		goto gotit;
	}
	/*
	 * check for a block available on the same cylinder
	 */
	cylno = cbtocylno(fs, bpref);
	if (cgp->cg_btot[cylno] == 0)
		goto norot;
	if (fs->fs_cpc == 0) {
		/*
		 * block layout info is not available, so just have
		 * to take any block in this cylinder.
		 */
		bpref = howmany(fs->fs_spc * cylno, NSPF(fs));
		goto norot;
	}
	/*
	 * check the summary information to see if a block is 
	 * available in the requested cylinder starting at the
	 * requested rotational position and proceeding around.
	 */
	cylbp = cgp->cg_b[cylno];
	pos = cbtorpos(fs, bpref);
	for (i = pos; i < NRPOS; i++)
		if (cylbp[i] > 0)
			break;
	if (i == NRPOS)
		for (i = 0; i < pos; i++)
			if (cylbp[i] > 0)
				break;
	if (cylbp[i] > 0) {
		/*
		 * found a rotational position, now find the actual
		 * block. A panic if none is actually there.
		 */
		pos = cylno % fs->fs_cpc;
		bno = (cylno - pos) * fs->fs_spc / NSPB(fs);
		if (fs->fs_postbl[pos][i] == -1) {
			printf("pos = %d, i = %d, fs = %s\n",
			    pos, i, fs->fs_fsmnt);
			panic("alloccgblk: cyl groups corrupted");
		}
		for (i = fs->fs_postbl[pos][i];; ) {
			if (isblock(fs, cgp->cg_free, bno + i)) {
				bno = blkstofrags(fs, (bno + i));
				goto gotit;
			}
			delta = fs->fs_rotbl[i];
			if (delta <= 0 || delta > MAXBPC - i)
				break;
			i += delta;
		}
		printf("pos = %d, i = %d, fs = %s\n", pos, i, fs->fs_fsmnt);
		panic("alloccgblk: can't find blk in cyl");
	}
norot:
	/*
	 * no blocks in the requested cylinder, so take next
	 * available one in this cylinder group.
	 */
	bno = mapsearch(fs, cgp, bpref, (int)fs->fs_frag);
	if (bno < 0)
		return (NULL);
	cgp->cg_rotor = bno;
gotit:
	clrblock(fs, cgp->cg_free, (long)fragstoblks(fs, bno));
	cgp->cg_cs.cs_nbfree--;
	cylno = cbtocylno(fs, bno);
	cgp->cg_b[cylno][cbtorpos(fs, bno)]--;
	cgp->cg_btot[cylno]--;

	fs_lock(mp);
	fs->fs_cstotal.cs_nbfree--;
	fs->fs_cs(fs, cgp->cg_cgx).cs_nbfree--;
	mp->m_flags |= M_MOD;
	fs_unlock(mp);

	return (cgp->cg_cgx * fs->fs_fpg + bno);
}
	
/*
 * Determine whether an gnode can be allocated.
 *
 * Check to see if an gnode is available, and if it is,
 * allocate it using the following policy:
 *   1) allocate the requested gnode.
 *   2) allocate the next available gnode after the requested
 *      gnode in the specified cylinder group.
 */
gno_t
ialloccg(gp, cg, gpref, mode)
	register struct gnode *gp;
	int cg;
	register daddr_t gpref;
	int mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	struct buf *bp;
	int start, len, loc;
	register int map, i;

	fs = FS(gp);
	if (fs->fs_cs(fs, cg).cs_nifree == 0)
		return (NULL);
	bp = bread(gp->g_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize,
	(struct gnode *) NULL);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC ||
	    cgp->cg_cs.cs_nifree == 0) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = timepick->tv_sec;
	if (gpref) {
		gpref %= fs->fs_ipg;
		if (isclr(cgp->cg_iused, gpref))
			goto gotit;
	}
	start = cgp->cg_irotor / NBBY;
	len = howmany(fs->fs_ipg - cgp->cg_irotor, NBBY);
	loc = skpc(0xff, len, &cgp->cg_iused[start]);
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = skpc(0xff, len, &cgp->cg_iused[0]);
		if (loc == 0) {
			printf("cg = %s, irotor = %d, fs = %s\n",
			    cg, cgp->cg_irotor, fs->fs_fsmnt);
			panic("ialloccg: map corrupted");
			/* NOTREACHED */
		}
	}
	i = start + len - loc;
	map = cgp->cg_iused[i];
	gpref = i * NBBY;
	for (i = 1; i < (1 << NBBY); i <<= 1, gpref++) {
		if ((map & i) == 0) {
			cgp->cg_irotor = gpref;
			goto gotit;
		}
	}
	printf("fs = %s\n", fs->fs_fsmnt);
	panic("ialloccg: block not in map");
	/* NOTREACHED */
gotit:
	setbit(cgp->cg_iused, gpref);
	cgp->cg_cs.cs_nifree--;

	fs_lock(gp->g_mp);
	fs->fs_cstotal.cs_nifree--;
	fs->fs_cs(fs, cg).cs_nifree--;
	gp->g_mp->m_flags |= M_MOD;
	if ((mode & GFMT) == GFDIR) {
		cgp->cg_cs.cs_ndir++;
		fs->fs_cstotal.cs_ndir++;
		fs->fs_cs(fs, cg).cs_ndir++;
	}
	fs_unlock(gp->g_mp);

	if (gp->g_mp->m_flags & M_SYNC)
		bwrite(bp);
	else
		bdwrite(bp);
	return (cg * fs->fs_ipg + gpref);
}

/*
 * Free a block or fragment.
 *
 * The specified block or fragment is placed back in the
 * free map. If a fragment is deallocated, a possible 
 * block reassembly is checked.
 */
free(gp, bno, size)
	register struct gnode *gp;
	register daddr_t bno;
	off_t size;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int cg, blk, frags, bbase;
	register int i;

	fs = FS(gp);
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    gp->g_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("free: bad size");
	}
	cg = dtog(fs, bno);
	if (badblock(fs, bno)) {
		printf("bad block %d, gno %d\n", bno, gp->g_number);
		return;
	}
	bp = bread(gp->g_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize,
		   (struct gnode *) NULL);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return;
	}
	cgp->cg_time = timepick->tv_sec;
	bno = dtogd(fs, bno);
	if (size == fs->fs_bsize) {
		if (isblock(fs, cgp->cg_free, fragstoblks(fs, bno))) {
			printf("dev = 0x%x, block = %d, fs = %s\n",
			    gp->g_dev, bno, fs->fs_fsmnt);
			panic("free: freeing free block");
		}
		setblock(fs, cgp->cg_free, fragstoblks(fs, bno));
		cgp->cg_cs.cs_nbfree++;
		i = cbtocylno(fs, bno);
		cgp->cg_b[i][cbtorpos(fs, bno)]++;
		cgp->cg_btot[i]++;

		fs_lock(gp->g_mp);
		fs->fs_cstotal.cs_nbfree++;
		fs->fs_cs(fs, cg).cs_nbfree++;
		gp->g_mp->m_flags |= M_MOD;
		fs_unlock(gp->g_mp);

	} else {
		bbase = bno - fragnum(fs, bno);
		/*
		 * decrement the counts associated with the old frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		fragacct(fs, blk, cgp->cg_frsum, -1);
		/*
		 * deallocate the fragment
		 */
		frags = numfrags(fs, size);
		for (i = 0; i < frags; i++) {
			if (isset(cgp->cg_free, bno + i)) {
				printf("dev = 0x%x, block = %d, fs = %s\n",
				    gp->g_dev, bno + i, fs->fs_fsmnt);
				panic("free: freeing free frag");
			}
			setbit(cgp->cg_free, bno + i);
		}
		cgp->cg_cs.cs_nffree += i;
		/*
		 * add back in counts associated with the new frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		fragacct(fs, blk, cgp->cg_frsum, 1);
		/*
		 * Update superblock information. If a complete block
		 * has been reassembled, account for it
		 */

		fs_lock(gp->g_mp);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		if (isblock(fs, cgp->cg_free, fragstoblks(fs, bbase))) {
			cgp->cg_cs.cs_nffree -= fs->fs_frag;
			fs->fs_cstotal.cs_nffree -= fs->fs_frag;
			fs->fs_cs(fs, cg).cs_nffree -= fs->fs_frag;
			cgp->cg_cs.cs_nbfree++;
			fs->fs_cstotal.cs_nbfree++;
			fs->fs_cs(fs, cg).cs_nbfree++;
			i = cbtocylno(fs, bbase);
			cgp->cg_b[i][cbtorpos(fs, bbase)]++;
			cgp->cg_btot[i]++;
		}
		gp->g_mp->m_flags |= M_MOD;
		fs_unlock(gp->g_mp);
	}
	if (gp->g_mp->m_flags & M_SYNC)
		bwrite(bp);
	else
		bdwrite(bp);
}

/*
 * Free a gnode.
 *
 * The specified gnode is placed back in the free map.
 */
ufs_gfree(gp, gno, mode)
	register struct gnode *gp;
	register gno_t gno;
	int mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	register int cg;

	fs = FS(gp);
	if ((unsigned)gno >= fs->fs_ipg*fs->fs_ncg) {
		printf("dev = 0x%x, gno = %d, fs = %s\n",
		    gp->g_dev, gno, fs->fs_fsmnt);
		panic("ufs_gfree: range");
	}
	cg = itog(fs, gno);
	bp = bread(gp->g_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize,
		   (struct gnode *) NULL);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return;
	}
	cgp->cg_time = timepick->tv_sec;
	gno %= fs->fs_ipg;
	if (isclr(cgp->cg_iused, gno)) {
		printf("dev = 0x%x, gno = %d, fs = %s block %d\n",
		    gp->g_dev, gno, fs->fs_fsmnt, fsbtodb(fs, cgtod(fs, cg)));
		panic("ufs_gfree: freeing free gnode");
	}
	clrbit(cgp->cg_iused, gno);
	if (gno < cgp->cg_irotor)
		cgp->cg_irotor = gno;
	cgp->cg_cs.cs_nifree++;

	fs_lock(gp->g_mp);
	fs->fs_cstotal.cs_nifree++;
	fs->fs_cs(fs, cg).cs_nifree++;
	if ((mode & GFMT) == GFDIR) {
		cgp->cg_cs.cs_ndir--;
		fs->fs_cstotal.cs_ndir--;
		fs->fs_cs(fs, cg).cs_ndir--;
	}
	gp->g_mp->m_flags |= M_MOD;
	fs_unlock(gp->g_mp);

	if (gp->g_mp->m_flags & M_SYNC)
		bwrite(bp);
	else
		bdwrite(bp);
}

/*
 * Find a block of the specified size in the specified cylinder group.
 *
 * It is a panic if a request is made to find a block if none are
 * available.
 */
daddr_t
mapsearch(fs, cgp, bpref, allocsiz)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
	int allocsiz;
{
	register daddr_t bno;
	register int i;
	register int field, subfield;
	int start, len, loc;
	int blk, pos;

	/*
	 * find the fragment by searching through the free block
	 * map for an appropriate bit pattern
	 */
	if (bpref)
		start = dtogd(fs, bpref) / NBBY;
	else
		start = cgp->cg_frotor / NBBY;
	len = howmany(fs->fs_fpg, NBBY) - start;
	loc = scanc((unsigned)len, (caddr_t)&cgp->cg_free[start],
		(caddr_t)fragtbl[fs->fs_frag],
		(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = scanc((unsigned)len, (caddr_t)&cgp->cg_free[0],
			(caddr_t)fragtbl[fs->fs_frag],
			(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
		if (loc == 0) {
			printf("start = %d, len = %d, fs = %s\n",
			    start, len, fs->fs_fsmnt);
			panic("alloccg: map corrupted");
			/* NOTREACHED */
		}
	}
	bno = (start + len - loc) * NBBY;
	cgp->cg_frotor = bno;
	/*
	 * found the byte in the map
	 * sift through the bits to find the selected frag
	 */
	for (i = bno + NBBY; bno < i; bno += fs->fs_frag) {
		blk = blkmap(fs, cgp->cg_free, bno);
		blk <<= 1;
		field = around[allocsiz];
		subfield = inside[allocsiz];
		for (pos = 0; pos <= fs->fs_frag - allocsiz; pos++) {
			if ((blk & field) == subfield)
				return (bno + pos);
			field <<= 1;
			subfield <<= 1;
		}
	}
	printf("bno = %d, fs = %s\n", bno, fs->fs_fsmnt);
	panic("alloccg: block not in map");
	return (-1);
}
