#ifndef lint
static	char	*sccsid = "@(#)cdfs_subr.c	4.2	(ULTRIX)	2/28/91";
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
 *			Modification History
 *	fs/cdfs/cdfs_subr.c
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping
 *
 *  9-Nov-90 -- prs
 *	Initial creation.
 *
 ***********************************************************************/
#ifdef KERNEL
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../fs/cdfs/cdfs_fs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/gnode_common.h"
#include "../fs/cdfs/cdfs_inode.h"
#include "../h/gnode.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/quota.h"
#include "../h/kernel.h"
#include "../h/stat.h"
#include "../h/kmalloc.h"
#else
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <cdfs/cdfs_fs.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/gnode_common.h>
#include <cdfs/cdfs_inode.h>
#include <sys/gnode.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/quota.h>
#include <sys/kmalloc.h>
#endif

#ifdef KERNEL

cdfs_syncgp(gp, cred)
	register struct gnode *gp;
	struct ucred *cred;
{
	return(u.u_error = EROFS);
}

cdfs_isodir_to_idir(fs, iso_dir, hsg_dir, idir)
struct fs *fs;
struct iso_dir *iso_dir;
struct hsg_dir *hsg_dir;
struct iso_idir *idir;
{
	union {
		unsigned char incoming[4];
		unsigned int  outgoing;
	} iso_convert_int;

	if (fs->fs_format == ISO_9660) {
		idir->dir_len = iso_dir->dir_len;
		idir->dir_xar = iso_dir->dir_xar;
		bcopy(iso_dir->dir_extent_lsb, 
		      iso_convert_int.incoming, sizeof(int));
		idir->dir_extent = iso_convert_int.outgoing;
		bcopy(iso_dir->dir_dat_len_lsb, 
		      iso_convert_int.incoming, sizeof(int));
		idir->dir_dat_len = iso_convert_int.outgoing;
		bcopy(iso_dir->dir_dt, idir->dir_dt, 
		      sizeof(iso_dir->dir_dt));
		idir->dir_file_flags = iso_dir->dir_file_flags;
		idir->dir_file_unit_size = 
			iso_dir->dir_file_unit_size;
		idir->dir_inger_gap_size = 
			iso_dir->dir_inger_gap_size;
		idir->dir_vol_seq_no = iso_dir->dir_vol_seq_no_lsb;
	} else {
		idir->dir_len = hsg_dir->dir_len;
		idir->dir_xar = hsg_dir->dir_xar;
		bcopy(hsg_dir->dir_extent_lsb, 
		      iso_convert_int.incoming, sizeof(int));
		idir->dir_extent = iso_convert_int.outgoing;
		bcopy(hsg_dir->dir_dat_len_lsb, 
		      iso_convert_int.incoming, sizeof(int));
		idir->dir_dat_len = iso_convert_int.outgoing;
		bcopy(hsg_dir->dir_dt, idir->dir_dt, 
		      sizeof(hsg_dir->dir_dt));
		idir->dir_file_flags = hsg_dir->dir_file_flags;
		idir->dir_file_unit_size = 
			hsg_dir->dir_file_unit_size;
		idir->dir_inger_gap_size = 
			hsg_dir->dir_inger_gap_size;
		idir->dir_vol_seq_no = hsg_dir->dir_vol_seq_no_lsb;
	}
}

/*
 * Read an iso directory record. Stuff record into file system
 * specific portion of gnode. gp->g_number is set to the
 * disk location of the directory record we will read.
 */

cdfs_readisodir(gp)
	register struct gnode *gp;
{
	struct buf *bp;
	unsigned int loc;
	unsigned int off;
	struct iso_dir *iso_dir = 0;
	struct hsg_dir *hsg_dir = 0;
	struct fs *fs;
	union {
		unsigned char incoming[4];
		unsigned int  outgoing;
	} iso_convert_int;

	fs = FS(gp);
	/*
	 * Since a directory record cannot span a logical sector,
	 * read in the entire sector.
	 */

	off = gp->g_number % fs->fs_ibsize;
	loc = gp->g_number - off;
	bp = bread(gp->g_dev, btodb(loc), fs->fs_ibsize, (struct gnode *)NULL);
	if (bp->b_flags & B_ERROR) {
		printf("cdfs_readisodir: Cannot read block %d of dev 0x%x\n",
		       btodb(loc), gp->g_dev);
		brelse(bp);
		return(0);
	}

	if (fs->fs_format == ISO_9660)
		iso_dir = (struct iso_dir *)
			((unsigned int)bp->b_un.b_addr + off);
	else
		hsg_dir = (struct hsg_dir *)
			((unsigned int)bp->b_un.b_addr + off);

	cdfs_isodir_to_idir(fs, iso_dir, hsg_dir, &G_TO_DIR(gp)->iso_dirp);
	brelse(bp);
	return(1);
}

cdfs_readxar(gp)
	struct gnode *gp;
{
	struct fs *fs;
	int bn;
	int loc;
	int off;
	int lbs;
	struct buf *bp;
	struct iso_xar *iso_xarp;
	struct hsg_xar *hsg_xarp;

	fs = (struct fs *)FS(gp);
	lbs = ISOFS_LBS(fs);

	off = (G_TO_DIR(gp)->iso_dir_extent * lbs) % fs->fs_ibsize;
	loc = (G_TO_DIR(gp)->iso_dir_extent * lbs) - off;

	bn = btodb(loc);
	bp = bread(gp->g_dev, bn, fs->fs_ibsize, (struct gnode *)NULL);

	if (bp->b_flags & B_ERROR)
		goto out;

	if (fs->fs_format == ISO_9660) {
		iso_xarp = (struct iso_xar *)((unsigned int)bp->b_un.b_addr +
					      off);
		if (iso_xarp->iso_xar_version != 1) {
			printf("cdfs_readisodir: XAR has %d for version\n",
			       iso_xarp->iso_xar_version);
			goto out;
		}
		if (iso_xarp->iso_xar_oid)
			gp->g_uid = iso_xarp->iso_xar_oid;
		if (iso_xarp->iso_xar_gid_lsb)
			gp->g_gid = iso_xarp->iso_xar_gid_lsb;
		if (iso_xarp->iso_xar_perm & ISO_NOT_OWN_READ)
			gp->g_mode &= ~S_IRUSR;
		if (iso_xarp->iso_xar_perm & ISO_NOT_OWN_EXEC)
			gp->g_mode &= ~S_IXUSR;
		if (iso_xarp->iso_xar_perm & ISO_NOT_GRP_READ)
			gp->g_mode &= ~S_IRGRP;
		if (iso_xarp->iso_xar_perm & ISO_NOT_GRP_EXEC)
			gp->g_mode &= ~S_IXGRP;
		if (iso_xarp->iso_xar_perm & ISO_NOT_OTH_READ)
			gp->g_mode &= ~S_IROTH;
		if (iso_xarp->iso_xar_perm & ISO_NOT_OTH_EXEC)
			gp->g_mode &= ~S_IXOTH;
	} else {
		hsg_xarp = (struct hsg_xar *)((unsigned int)bp->b_un.b_addr +
					      off);
		if (hsg_xarp->iso_xar_version != 1) {
			printf("cdfs_readisodir: XAR has %d for version\n",
			       iso_xarp->iso_xar_version);
			goto out;
		}
		if (hsg_xarp->iso_xar_oid)
			gp->g_uid = hsg_xarp->iso_xar_oid;
		if (hsg_xarp->iso_xar_gid_lsb)
			gp->g_gid = hsg_xarp->iso_xar_gid_lsb;
		if (hsg_xarp->iso_xar_perm & ISO_NOT_OWN_READ)
			gp->g_mode &= ~S_IRUSR;
		if (hsg_xarp->iso_xar_perm & ISO_NOT_OWN_EXEC)
			gp->g_mode &= ~S_IXUSR;
		if (hsg_xarp->iso_xar_perm & ISO_NOT_GRP_READ)
			gp->g_mode &= ~S_IRGRP;
		if (hsg_xarp->iso_xar_perm & ISO_NOT_GRP_EXEC)
			gp->g_mode &= ~S_IXGRP;
		if (hsg_xarp->iso_xar_perm & ISO_NOT_OTH_READ)
			gp->g_mode &= ~S_IROTH;
		if (hsg_xarp->iso_xar_perm & ISO_NOT_OTH_EXEC)
			gp->g_mode &= ~S_IXOTH;
	}

out:
	brelse(bp);
}

static	int	dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
extern struct timezone tz;
#define	dysize(A) (((A)%4)? 365: 366)
cdfs_tounixdate(y,m,d,h,mi,s,o)
char y;
char m;
char d;
char h;
char mi;
char s;
char o;
{
	register int i;
	register int val = 0;
	int iy;
	int im;


	if (y < 0 || y > 100 || m < 1 || m > 12 || d < 1 || d > 31 ||
	    h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59)
		return(timepick->tv_sec);
	/*
	 * Added up the seconds since the epoch
	 *  val = o - tz.tz_minuteswest;  should divide into 24 hour periods
	 */
	iy = y + 1900;
	for (i = 1970; i < iy; i++)
		val += dysize(i);
	im = m;
	/* 
	 * Leap year 
	 */
	if (dysize(iy) == 366 && im >= 3)
		val++;
	/*
	 * Do the current year
	 */
	while(--im)
		val += dmsize[im-1];
	val += d-1;
	val = 24*val + h;
	val = 60*val + m;
	val = 60*val + s;
	return (val);
}
#endif
