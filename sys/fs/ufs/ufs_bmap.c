#ifndef lint
static	char	*sccsid = "@(#)ufs_bmap.c	4.3	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987 by			*
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

/*
 *		Modification History
 *
 * 10 Apr 91 -- prs
 *	Ensure the existence of a block in the on-disk inode before
 *	checking whether to extend a fragment into a block.
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 25 Jul 89 -- chet
 *	Fix syncronous filesystems and meet new bdwrite() interface.
 *
 * 11 May 87 -- chet
 *	Changed ufs_bmap() sync argument to provide feedback
 *	to caller about on-disk structure changes. Removed
 *	bwrite() calls for zeroed blocks when doing synchronous
 *	non-directory writes.
 *
 * 29 Jan 87 -- chet
 *	add new arg to bdwrite() calls.
 *
 * 1/14/87 -- rr
 *	take out extra calls from ufs_bmap() to bmap() so now we only
 *	have a ufs_bmap that everyone calls through GFS.
 *
 * 003 - 10/23/86 -- chet
 *	Implemented new sync argument for synchronous writes in ufs_bmap()
 *	and bmap() on behalf of an IO_SYNC request from ufs_rwgp().
 *	Changes overlap 001.
 *	
 * 002 - 09/11/86 -- koehler
 *	added bmap function
 *
 * 001 - 12/23/85 -- Paul Shaughnessy
 *	Added code to perform syncronous writes if the syncronous
 *	write flag in the inode is set.
 *
 **********************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/gnode_common.h"
#include "../ufs/ufs_inode.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../ufs/fs.h"
#include "../h/kernel.h"


/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead.
 * Return -1 on any error and disk block otherwise.
 */

ufs_bmap(gp,bn,rwflg,size,sync)
register struct gnode *gp;
daddr_t bn;	/* virtual block of file to un-map */
int rwflg;	/* purpose of call */
int size;	/* supplied only when rwflg == B_WRITE */
int *sync;	/* supplied only when rwflg == B_WRITE
		 * If sync has a non-zero value, then assume
		 * that the caller is doing a synchronous write
		 * and wants to know if this routine has modified
		 * any on-disk structures.
		 * Set *sync to 1 if these structures have changed;
		 * leave it untouched otherwise.
		 * If sync has a zero value then assume that the
		 * caller is doing an asynchronous write.
		 */
{
	register int i;
	int osize, nsize;
	register struct buf *bp, *nbp;
	struct fs *fs;
	register int j, sh;
	daddr_t nb, lbn, *bap, pref, blkpref();

	if (bn < 0) {
		u.u_error = EFBIG;
		return (-1);
	}
	fs = FS(gp);
	rablock = 0;
	rasize = 0;		/* conservative */

	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 */
	nb = lblkno(fs, gp->g_size);
	if (rwflg == B_WRITE && nb < NDADDR && nb < bn && 
	    G_TO_I(gp)->di_db[nb] != 0) {
		osize = blksize(fs, gp, nb);
		if (osize < fs->fs_bsize && osize > 0) {
			bp = realloccg(gp, G_TO_I(gp)->di_db[nb],
				blkpref(gp,nb,(int)nb,&(G_TO_I(gp)->di_db[0])),
				osize, (int)fs->fs_bsize);
			if (bp == NULL)
				return (-1);
			gp->g_size = (nb + 1) * fs->fs_bsize;
			G_TO_I(gp)->di_db[nb] = dbtofsb(fs, bp->b_blkno);
			gp->g_flag |= GUPD|GCHG;
			/*
			 * if synchronous operation is specified, then
			 * write out the new block synchronously, then
			 * update the inode to make sure it points to it
			 */
			if (sync || gp->g_mp->m_flags & M_SYNC)
			{
				bwrite(bp);
				(void) ufs_gupdat(gp, timepick, timepick,
					1, (struct ucred *) 0);
			}
			else
				bdwrite(bp);
		}
	}
	/*
	 * The first NDADDR blocks are direct blocks
	 */
	if (bn < NDADDR) {
		nb = G_TO_I(gp)->di_db[bn];
		if (rwflg == B_READ) {
			if (nb == 0)
				return (-1);
			goto gotit;
		}
		if (nb == 0 || gp->g_size < (bn + 1) * fs->fs_bsize) {
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, blkoff(fs, gp->g_size));
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;
				bp = realloccg(gp, nb,
					blkpref(gp, bn, (int)bn, 
					&(G_TO_I(gp)->di_db[0])),
					osize, nsize);
			} else {
				if (gp->g_size < (bn + 1) * fs->fs_bsize)
					nsize = fragroundup(fs, size);
				else
					nsize = fs->fs_bsize;
				bp = alloc(gp,
					blkpref(gp, bn, (int)bn, 
					&(G_TO_I(gp)->di_db[0])),
					nsize);
			}
			if (bp == NULL)
				return (-1);
			nb = dbtofsb(fs, bp->b_blkno);
			if (sync)
				*sync = 1;
			if ((gp->g_mode&GFMT) == GFDIR ||
			    gp->g_mp->m_flags & M_SYNC)
				/*
				 * Write directory blocks synchronously
				 * so they never appear with garbage in
				 * them on the disk.
				 */
				bwrite(bp);
			else
				bdwrite(bp);
			G_TO_I(gp)->di_db[bn] = nb;
			gp->g_flag |= GUPD|GCHG;
		}
gotit:
		if (bn < NDADDR - 1) {
			rablock = fsbtodb(fs, G_TO_I(gp)->di_db[bn + 1]);
			rasize = blksize(fs, gp, bn + 1);
		}
		return(fsbtodb(fs, nb));
	}

	/*
	 * Determine how many levels of indirection.
	 */
	pref = 0;
	sh = 1;
	lbn = bn;
	bn -= NDADDR;
	for (j = NIADDR; j>0; j--) {
		sh *= NINDIR(fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0) {
		u.u_error = EFBIG;
		return (-1);
	}

	/*
	 * fetch the first indirect block
	 */
	nb = G_TO_I(gp)->di_ib[NIADDR - j];
	if (nb == 0) {
		if (rwflg == B_READ)
			return (-1);
		pref = blkpref(gp, lbn, 0, (daddr_t *)0);
	        bp = alloc(gp, pref, (int)fs->fs_bsize);
		if (bp == NULL)
			return (-1);
		nb = dbtofsb(fs, bp->b_blkno);
		/*
		 * Write synchronously so that indirect blocks
		 * never point at garbage.
		 */
		bwrite(bp);
		G_TO_I(gp)->di_ib[NIADDR - j] = nb;
		gp->g_flag |= GUPD|GCHG;
		if (sync)
			*sync = 1;
	}

	/*
	 * fetch through the indirect blocks
	 */
	for (; j <= NIADDR; j++) {
		bp = bread(gp->g_dev, fsbtodb(fs, nb), (int)fs->fs_bsize, 
			   (struct gnode *) NULL);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (-1);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (bn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			if (rwflg==B_READ) {
				brelse(bp);
				return (-1);
			}
			if (pref == 0)
				if (j < NIADDR)
					pref = blkpref(gp, lbn, 0,
						       (daddr_t *)0);
				else
					pref = blkpref(gp, lbn, i, &bap[0]);
		        nbp = alloc(gp, pref, (int)fs->fs_bsize);
			if (nbp == NULL) {
				brelse(bp);
				return (-1);
			}
			nb = dbtofsb(fs, nbp->b_blkno);
			if ((j < NIADDR || (gp->g_mode&GFMT) == GFDIR) ||
			    gp->g_mp->m_flags & M_SYNC)
				/*
				 * Write synchronously so indirect blocks
				 * never point at garbage and blocks
				 * in directories never contain garbage.
				 */
				bwrite(nbp);
			else
				bdwrite(nbp);
			bap[i] = nb;
			if (sync || gp->g_mp->m_flags & M_SYNC)
				bwrite(bp);
			else
				bdwrite(bp);
		} else
			brelse(bp);
	}

	/*
	 * calculate read-ahead.
	 */
	if (i < NINDIR(fs) - 1) {
		rablock = fsbtodb(fs, bap[i+1]);
		rasize = fs->fs_bsize;
	}
	return(fsbtodb(fs, nb));
}
