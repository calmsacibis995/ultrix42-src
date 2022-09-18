#ifndef lint
static	char	*sccsid = "@(#)ufs_subr.c	4.2	(ULTRIX)	2/28/91";
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/ufs_subr.c
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 25 Jul 89 -- chet
 *	Change ufs_suncgp() to use new bflush()
 *
 * 22 Sep 88 -- chet
 *	Added lower priority level in ufs_syncgp()
 *
 * 28 Jul 88 -- prs
 *	SMP - System call turn on.
 *
 * 23 Oct 86 -- chet
 *	Add arg to ufs_bmap() call
 *
 * 11 Sep 86 -- koehler
 *	introduced the bmap function
 *
 * 06 Nov 84 -- jrs
 *	Add small fix to update and Berkeley change to syncip() to
 *	cut overhead when syncing large files.
 *
 * 26 Oct 84 -- jrs
 *	Add small change for nami cache support
 *
 * 17 Jul 84 -- jmcg
 *	Added code to keep track of inode lockers and unlockers as a
 *	debugging aid.  Conditionally compiled with RECINODELOCK
 *
 * 17 Jul 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		ufs_subr.c	6.1	83/07/29
 *
 * ------------------------------------------------------------------------
 */

#ifdef KERNEL
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
#else
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <ufs/fs.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/gnode_common.h>
#include <ufs/ufs_inode.h>
#include <sys/gnode.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/quota.h>
#endif

#ifdef KERNEL

int	ufs_syncgp_size = 16;

/*
 * Flush all the blocks associated with a gnode.
 * There are two strategies based on the size of the file;
 * large files are those with more than ufs_syncgp_size blocks.
 *
 * Large files
 *	Call bflush() to synchronously push any dirty buffers
 *	associated with the device on which the file resides.
 * Small files
 *	Look up each block in the file to see if it is in the 
 *	buffer pool writing any that are found to disk.
 *	Note that we make a more stringent check of
 *	writing out any block in the buffer pool that may
 *	overlap the gnode. This brings the gnode up to
 *	date with recent mods to the cooked (block) device.
 */
ufs_syncgp(gp, cred)
	register struct gnode *gp;
	struct ucred *cred;
{
	register struct fs *fs;
	register struct buf *bp;
	struct buf *lastbufp;
	register long lbn, lastlbn;
	register daddr_t blkno;
	int s;
	
	fs = FS(gp);
	lastlbn = howmany(gp->g_size, fs->fs_bsize);
	if (lastlbn < ufs_syncgp_size) {	/* small file */
		for (lbn = 0; lbn < lastlbn; lbn++) {
			blkno = ufs_bmap(gp, (int)lbn, B_READ, 0, 0);
			blkflush(gp->g_dev, blkno, blksize(fs, gp, lbn),
				 (struct gnode *) NULL);
		}
	} else {	/* large file */
		bflush(gp->g_dev, (struct gnode *) 0, 1);
	}

	gp->g_flag |= GCHG;
	ufs_gupdat(gp, timepick, timepick, 1, cred);
}
#endif

extern	int around[9];
extern	int inside[9];
extern	u_char *fragtbl[];

/*
 * Update the frsum fields to reflect addition or deletion 
 * of some frags.
 */
fragacct(fs, fragmap, fraglist, cnt)
	register struct fs *fs;
	int fragmap;
	register long fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}

#ifdef KERNEL
/*
 * Check that a specified block number is in range.
 */
badblock(fs, bn)
	register struct fs *fs;
	register daddr_t bn;
{

	if ((unsigned)bn >= fs->fs_size) {
		printf("bad block %d, ", bn);
		fserr(fs->fs_fsmnt, "bad block");
		return (1);
	}
	return (0);
}
#endif

/*
 * block operations
 *
 * check if a block is available
 */
isblock(fs, cp, h)
	register struct fs *fs;
	register unsigned char *cp;
	register daddr_t h;
{
	register unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		panic("isblock");
		return (NULL);
	}
}

/*
 * take a block out of the map
 */
clrblock(fs, cp, h)
	register struct fs *fs;
	register u_char *cp;
	register daddr_t h;
{

	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		panic("clrblock");
	}
}

/*
 * put a block into the map
 */
setblock(fs, cp, h)
	register struct fs *fs;
	register unsigned char *cp;
	register daddr_t h;
{

	switch (fs->fs_frag) {

	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		panic("setblock");
	}
}
