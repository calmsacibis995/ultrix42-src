#ifndef lint
static	char	*sccsid = "@(#)cdfs_gnode.c	4.1	(ULTRIX)	11/9/90";
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
/************************************************************************
 *									*
 *			Modification History				*
 *	/fs/cdfs/cdfs_gnode.c						*
 *									*
 * 22-Oct-90 -- prs							*
 *	Initial creation.						*
 *									*
 ************************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode_common.h"
#include "../h/gnode.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/stat.h"
#include "../h/fs_types.h"
#include "../fs/cdfs/cdfs_fs.h"
#include "../fs/cdfs/cdfs_inode.h"
#include "../fs/cdfs/cdfs_mount.h"

extern struct gnode_ops *cdfs_gnode_ops;
extern cdfs_readisodir();
extern cdfs_readxar();

int
cdfs_ginit(gp, iflag, ptr)
	struct gnode *gp;
	int iflag;
	caddr_t ptr;
{
	struct fs *fs;

	if (iflag == RECLAIM_GNODE)
		panic("cdfs_ginit: reclaim trying too hard");

	fs = FS(gp);
	/*
	 * Initialize FS specific portion of gnode with fields from
	 * directory record.
	 */
	if (cdfs_readisodir(gp) == 0)
		return(0);
	/*
	 * If the volume sequence number located in the directory record,
	 * does not match the primary volume descriptors volume sequence
	 * number, don't initialize gnode.
	 *
	 * XXX - for HSG format. Do check only if primary volume descriptor
	 * set size is greater than one (1).
	 */
	if ((fs->fs_format == ISO_HSG && ISOFS_SETSIZE(fs) > 1 &&
	    G_TO_DIR(gp)->iso_dir_vol_seq_no != ISOFS_VOLSEQNUM(fs)) ||
	    (fs->fs_format == ISO_9660 &&
	     G_TO_DIR(gp)->iso_dir_vol_seq_no != ISOFS_VOLSEQNUM(fs))) {
		printf("cdfs_ginit: g_number %d has %d vol seq number\n",
		       gp->g_number, G_TO_DIR(gp)->iso_dir_vol_seq_no);
		return(0);
	}
	/*
	 * Fill in default values.
	 */
	gp->g_atime.tv_usec = gp->g_mtime.tv_usec = gp->g_ctime.tv_usec = 0;
	gp->g_atime.tv_sec = gp->g_mtime.tv_sec = gp->g_ctime.tv_sec = 
		cdfs_tounixdate(G_TO_DIR(gp)->iso_dir_dt[0], 
			       G_TO_DIR(gp)->iso_dir_dt[1],
			       G_TO_DIR(gp)->iso_dir_dt[2], 
			       G_TO_DIR(gp)->iso_dir_dt[3],
			       G_TO_DIR(gp)->iso_dir_dt[4], 
			       G_TO_DIR(gp)->iso_dir_dt[5],
			       (fs->fs_format == ISO_9660 ? 
				G_TO_DIR(gp)->iso_dir_dt[6] : 0));
	gp->g_uid = 0;
	gp->g_gid = 1;
	if (G_TO_DIR(gp)->iso_dir_file_flags&ISO_FLG_DIR) {
	 	gp->g_mode = GFDIR|(0555);
	 	gp->g_nlink = 2;
	} else {
		gp->g_mode = GFREG|(0555);
	 	gp->g_nlink = 1;
	}
	/*
	 * If file system mounted with XAR option and XAR exists, call
	 * cdfs_readxar to further initialize gnode.
	 */
	if ((gp->g_mp)->m_flags & M_NODEFPERM && 
	    G_TO_DIR(gp)->iso_dir_file_flags&ISO_FLG_PROTECT)
		cdfs_readxar(gp);
	gp->g_size = (int)G_TO_DIR(gp)->iso_dir_dat_len;
	gp->g_blocks = howmany(gp->g_size, ISOFS_LBS(fs));
	gp->g_gennum = gp->g_number;
	gp->g_rdev = NODEV; /* no special devices on ISO Volumes */
	gp->g_ops = cdfs_gnode_ops;
	return(1);
}

/*
 * Make a locked gnode inactive.  Another process may create a reference
 * to this gnode while we're operating on it, but it won't be able to
 * lock it until we're done.  By then we will have marked it so the
 * other process will know to initialize it again.
 */

cdfs_inactive(gp)
	register struct gnode *gp;
{
	gassert(gp);

	if (gp->g_nlink <= 0) {
		gp->g_rdev = 0;
		gp->g_nlink = 0;				
		gp->g_mode = 0;
		gp->g_gennum++;
	}
}

/*
 * Lock a gnode.
 */
cdfs_glock(gp)
	register struct gnode *gp;

{
	if(gp->g_mp->m_fstype != GT_CDFS) {
		printf("cdfs_glock: gp 0x%x type %d\n", gp,
		gp->g_mp->m_fstype);
		panic("cdfs_glock: gp type not GT_CDFS");
	}
	gfs_lock(gp);
}

/*
 * Unlock a gnode.
 */
cdfs_gunlock(gp)
	register struct gnode *gp;
{
	if (!glocked(gp)) {
		cprintf("cdfs_gunlock: gp unlocked, dev 0x%x gno %d\n",
		gp->g_dev, gp->g_number);
		panic("cdfs_gunlock: gp not locked");
	}
	if (gp->g_mp->m_fstype != GT_CDFS) {
		printf("cdfs_gulock: gp 0x%x type %d\n", gp,
		gp->g_mp->m_fstype);
		panic("cdfs_gulock: gp type not GT_CDFS");
	}
	gfs_unlock(gp);
}

cdfs_gupdat(gp, ta, tm, waitfor, cred)
	register struct gnode *gp;
	register struct timeval *ta, *tm;
	int waitfor;
	struct ucred *cred;
{
	gp->g_flag &= ~(GUPD|GACC|GCHG|GMOD);
	return(EROFS);
}
