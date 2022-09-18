#ifndef lint
static	char	*sccsid = "@(#)cdfs_mount.c	4.1	(ULTRIX)	11/9/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 87, 89 by			*
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
 *		Modification History
 *	fs/cdfs/cdfs_mount.c
 *
 * 08-Jun-90 -- prs
 *	Initial creation cdfs_mount.c
 *
 ***********************************************************************/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode_common.h"
#include "../fs/cdfs/cdfs_fs.h"
#include "../fs/cdfs/cdfs_inode.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/fs_types.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/kernel.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/devio.h"
#include "../cdfs/cdfs_mount.h"
#include "../h/kmalloc.h"
#include "../h/cpudata.h"

int		cdfs_umount(),	cdfs_ginit(), cdfs_inactive();
struct gnode	*cdfs_namei();
int		cdfs_link(),	cdfs_unlink(),	cdfs_rmdir();
struct gnode	*cdfs_mkdir();
struct gnode	*cdfs_maknode();
int		cdfs_rename(),	cdfs_getdirent();
struct gnode	*cdfs_galloc();
int		cdfs_syncgp(),	cdfs_gfree(),	cdfs_gtrunc();
int		cdfs_rwgp(), cdfs_rlock();
int		cdfs_seek(),	cdfs_stat(),	cdfs_glock();
int		cdfs_gunlock(),	cdfs_gupdat(),	cdfs_open();
int		cdfs_close(),	cdfs_select(),	cdfs_readlink();
int		cdfs_symlink();
struct fs_data	*cdfs_getfsdata();
int		cdfs_fcntl(),	cdfs_bmap();


struct	mount_ops Cdfs_mount_ops = {
/* begin mount ops */
	cdfs_umount,
	0,		/* sb update */
	cdfs_ginit,
	0,		/* match */
	0,		/* reclaim */
	cdfs_inactive,
	cdfs_getfsdata,
};
struct	gnode_ops Cdfs_gnode_ops = {
/* begin gnode ops */
	cdfs_namei,
	cdfs_link,
	cdfs_unlink,
	cdfs_mkdir,
	cdfs_rmdir,
	cdfs_maknode,
	cdfs_rename,
	cdfs_getdirent,
	0,
	cdfs_syncgp,
	0,		/* truncate */
	0, 		/*getval*/
	cdfs_rwgp,
	cdfs_rlock,
	cdfs_seek,
	cdfs_stat,
	cdfs_glock,
	cdfs_gunlock,
	cdfs_gupdat,
	cdfs_open,
	cdfs_close,
	cdfs_select,
	cdfs_readlink,
	cdfs_symlink,
	cdfs_fcntl,
	0,		/* gfreegn */
	cdfs_bmap
};

struct  mount_ops  *cdfs_mount_ops = &Cdfs_mount_ops;
struct  gnode_ops  *cdfs_gnode_ops = &Cdfs_gnode_ops;

int	isodebug = 0;

int strat_begin_initialized = 0;
struct iso_strat iso_strat_begin;

/* this routine has lousy error codes */
/* this routine has races if running twice */
struct mount *
cdfs_mount(devname, name, ronly, mp, fs_specific)
	char *devname;
	char *name;
	int ronly;
	register struct mount *mp;
	struct iso_specific *fs_specific;
{
	dev_t	dev;
	register struct buf *tp = NULL;
	register struct buf *bp = NULL;
	register struct iso_fs *iso_fs = 0;
	register struct hsg_fs *hsg_fs = 0;
	register struct fs *fs;
	register struct mount *nmp;
	register struct gnode *gp;
	
	struct iso_dir *iso_tdir;
	struct hsg_dir *hsg_tdir;
	int blks;
	caddr_t space;
	int i, size;
	struct gnode gnode;
	struct devget devget;
	struct iso_specific iso_sp;
	int saveaffinity;
	unsigned int loc;
	unsigned int primary_loc, supplementary_loc;
	union {
		unsigned char incoming[4];
		unsigned int outgoing;
	} convert_extent;
	extern struct lock_t lk_mount_table;

#ifdef notdef
	if (strat_begin_initialized == 0) {
		strat_begin_initialized++;
		iso_strat_begin.strat_forw = &iso_strat_begin;
		iso_strat_begin.strat_back = &iso_strat_begin;
	}
#endif notdef

	if (u.u_error = getmdev(&dev, devname, u.u_uid)) {
		goto done;
	}
	GETMP(nmp, dev);
	if (nmp != NULL) {
		u.u_error = EBUSY; /* someone has us mounted */
	        goto done;
	}
	if (fs_specific != NULL) {
		u.u_error = copyin((caddr_t) fs_specific, (caddr_t)
		&iso_sp, sizeof(iso_sp));
		if (u.u_error)
			goto done;
		fs_specific = &iso_sp;
	}

	dnlc_purge();

	/*
	 * check_mountp will verify local mount point exists and is
	 * ok to mount on.
	 */

	if (!(check_mountp(mp, name)))
		goto done;
	/* 
	 * this is a hack but I can't see a way around it
	 * we need a gnode before we have the system mounted so we push 
	 * one on the stack and hand craft it. The gget at the end replaces
	 * this fake gnode with the real one.
	 */
		
	gnode.g_mode = GFBLK;
	gnode.g_dev = gnode.g_rdev = dev;
	gnode.g_ops = cdfs_gnode_ops;	/* set pointer to file ops */
	gnode.g_mp = mp;
	mp->m_rootgp = &gnode;
	
	/* set up some stuff that smount cannot accomplish */
	
	/* Search mount table for entry using tdev */
	smp_lock(&lk_mount_table, LK_RETRY);
	for (nmp = mount; nmp < &mount[nmount]; nmp++) {
		if (nmp->m_dev == dev) {
			smp_unlock(&lk_mount_table);
			u.u_error = EBUSY;
			goto done;
		}
	}
		
	mp->m_dev = dev;
	smp_unlock(&lk_mount_table);
	mp->iostrat = bdevsw[major(dev)].d_strategy;	/* set this now! */
	mp->m_ops = cdfs_mount_ops;

	mp->m_flags = M_RONLY | M_LOCAL;
	if (fs_specific != NULL) {
		int pg_thresh = fs_specific->iso_pgthresh * 1024;
		mp->m_flags |= (fs_specific->iso_flags & (M_DEFPERM |
			M_NODEFPERM | M_NOVERSION | M_PRIMARY));

		mp->m_fs_data->fd_pgthresh = 
			clrnd(btoc((pg_thresh > MINPGTHRESH) ?
				pg_thresh : MINPGTHRESH));
	} else {
		/*
		 * if we don't specify the threshhold, use 64kB
		 */
		mp->m_fs_data->fd_pgthresh = clrnd(btoc(MINPGTHRESH * 8));
		mp->m_flags |= M_DEFPERM;
	}

	/*
	 * Non super users cannot look at entire disk by default.
	 */
	if (!suser())
		mp->m_flags &= ~M_PRIMARY;

	CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	u.u_error = (*bdevsw[major(dev)].d_open)(dev, FREAD);
	RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	if (u.u_error)
		goto done;

	bzero(&devget,sizeof(struct devget));
	CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	u.u_error = (*bdevsw[major(dev)].d_ioctl)(dev, DEVIOCGET, &devget, 0);
	RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);

	if (u.u_error) {
			printf("file system device ioctl failure");
	}

	if (devget.stat & DEV_OFFLINE) {
		u.u_error = ENODEV;
		goto ERROR;
	}
	primary_loc = supplementary_loc = 0;
	loc = PVD_BLOCK;
	for (;;) {
		/*
		 * Read Primary Volume Descriptor
		 */
		tp = bread(dev, loc, ISO_SECSIZE, (struct gnode *) NULL);
		if (tp->b_flags & B_ERROR) {
			uprintf("cdfs_mount: Terminating volume descriptor not found\n");
			brelse(tp);
			goto ERROR;
		}

		iso_fs = (struct iso_fs *)tp->b_un.b_fs;
		/*
		 *	Check the magic number and see 
		 *	if we have a valid filesystem.
		 */
		if(!strncmp(iso_fs->iso_std_id,"CD001", 5)) {
			switch(iso_fs->iso_vol_desc_type) {
			      case TERMINATING_VOL_DESC:
				      brelse(tp);
				      hsg_fs = (struct hsg_fs *)NULL;
				      goto found;
			      case PRIMARY_VOL_DESC:
				      primary_loc = loc;
				      break;
			      case SUPPLEMENTARY_VOL_DESC:
				      if (!strncmp(iso_fs->iso_system_id, 
						   "DEC_ULTRIX", 10)) {
					      supplementary_loc = loc;
				      }
			}
			loc += (ISO_SECSIZE / DEV_BSIZE);
			brelse(tp);
			continue;
		}
		hsg_fs = (struct hsg_fs *)tp->b_un.b_fs;
		/*
		 * Check the magic number and see
		 * if formatted in HSG.
		 */
		if(!strncmp(hsg_fs->iso_std_id,"CDROM", 5)) {
			switch(hsg_fs->iso_vol_desc_type) {
			      case TERMINATING_VOL_DESC:
				      brelse(tp);
				      iso_fs = (struct iso_fs *)NULL;
				      goto found;
			      case PRIMARY_VOL_DESC:
				      primary_loc = loc;
				      break;
			      case SUPPLEMENTARY_VOL_DESC:
				      if (!strncmp(hsg_fs->iso_system_id, 
						   "DEC_ULTRIX", 10)) {
					      supplementary_loc = loc;
				      }
			}
			loc += (ISO_SECSIZE / DEV_BSIZE);
			brelse(tp);
			continue;
		}
		uprintf("cdfs_mount: Unknown descriptor type\n");
		brelse(tp);
		goto ERROR;
	}
	
found:
	if (primary_loc == 0) {
		uprintf("cdfs_mount: Volume Descriptor Terminator found before Primary volume descriptor\n");
		goto ERROR;
	}
	if (supplementary_loc == 0 || mp->m_flags & M_PRIMARY)
		loc = primary_loc;
	else {
		loc = supplementary_loc;
		if (!suser()) {
			mp->m_flags &= ~M_DEFPERM;
			mp->m_flags |= M_NODEFPERM;
		}
	}
	tp = bread(dev, loc, ISO_SECSIZE, (struct gnode *) NULL);
	if (tp->b_flags & B_ERROR) {
		uprintf("cdfs_mount: I/O error reading descriptor block\n");
		brelse(tp);
		goto ERROR;
	}
	if (iso_fs)
		iso_fs = (struct iso_fs *)tp->b_un.b_fs;
	else
		hsg_fs = (struct hsg_fs *)tp->b_un.b_fs;

	bp = geteblk(MAXBSIZE);
	mp->m_bufp = bp;

	/* map the volume desc into a buffer not connected with a device */
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)&bp->b_un.b_fs->fs_block,
	      ISO_SECSIZE);
	tp->state |= B_INVAL; /* Since bp does not have an associated gp */
	brelse(tp);
	tp = 0;

	fs = bp->b_un.b_fs;
	if (iso_fs) {
		fs->fs_format = ISO_9660;
		iso_fs = (struct iso_fs *) &fs->fs_block.isofs;
		iso_tdir = &iso_fs->iso_root_dir;
	} else {
		fs->fs_format = ISO_HSG;
		hsg_fs = (struct hsg_fs *) &fs->fs_block.hsgfs;
		hsg_tdir = &hsg_fs->iso_root_dir;
	}
	if (isodebug) {
		printf("cdfs_mount: Volume Info. Block follows:\n");
		printf("Volume Descriptor type = %d\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_vol_desc_type :
			 hsg_fs->iso_vol_desc_type));
		printf("std_id = <%5s>\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_std_id :
			 hsg_fs->iso_std_id));
		printf("vol desc version = %d\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_vol_desc_vers :
			 hsg_fs->iso_vol_desc_vers));
		printf("sys id = <%32s>\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_system_id :
			 hsg_fs->iso_system_id));
		printf("vol id = <%32s>\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_vol_id :
			 hsg_fs->iso_vol_id));
		printf("vol space size = %d\n",
			(fs->fs_format == ISO_9660 ? iso_fs->iso_vol_space_size_lsb:
			 hsg_fs->iso_vol_space_size_lsb));
		printf("vol set size = %d\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_vol_set_size_lsb :
			 hsg_fs->iso_vol_set_size_lsb));
		printf("vol seq number = %d\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_vol_seq_num_lsb :
			 hsg_fs->iso_vol_seq_num_lsb));
		printf("logical block size = %d\n", ISOFS_LBS(fs));

		printf("Root Directory:\n");
		printf("\t entry len = %d\n", (fs->fs_format == ISO_9660 ? 
						iso_tdir->dir_len :
						hsg_tdir->dir_len));
		printf("\t xar len = %d\n", (fs->fs_format == ISO_9660 ? 
					      iso_tdir->dir_xar :
					      hsg_tdir->dir_xar));
		bcopy((fs->fs_format == ISO_9660 ? iso_tdir->dir_extent_lsb :
		       hsg_tdir->dir_extent_lsb), convert_extent.incoming, 
		      sizeof(int));
		printf("\t location = %d\n", convert_extent.outgoing);
		bcopy((fs->fs_format == ISO_9660 ? iso_tdir->dir_dat_len_lsb :
		       hsg_tdir->dir_dat_len_lsb), convert_extent.incoming,
		      sizeof(int));
		printf("\t data len = %d\n", convert_extent.outgoing);
		printf("\t file flags = 0x%x\n", (fs->fs_format == ISO_9660 ? 
						   iso_tdir->dir_file_flags :
						   hsg_tdir->dir_file_flags));
		printf("\t unit size = %d\n", (fs->fs_format == ISO_9660 ? 
						iso_tdir->dir_file_unit_size :
						hsg_tdir->dir_file_unit_size));
		printf("\t gap size = %d\n", (fs->fs_format == ISO_9660 ? 
					       iso_tdir->dir_inger_gap_size :
					       hsg_tdir->dir_inger_gap_size));
		printf("\t vol seq no = %d\n", (fs->fs_format == ISO_9660 ? 
						 iso_tdir->dir_vol_seq_no_lsb :
						 hsg_tdir->dir_vol_seq_no_lsb));
		printf("\t dir name len = %d\n", (fs->fs_format == ISO_9660 ? 
						   iso_tdir->dir_namelen :
						   hsg_tdir->dir_namelen));
		printf("\t dir name = %s\n", (fs->fs_format == ISO_9660 ? 
					       iso_tdir->dir_name :
					       hsg_tdir->dir_name));

		printf("vol set size id = %60s\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_vol_set_id :
			 hsg_fs->iso_vol_set_id));
		printf("pub id = %60s\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_pub_id :
			 hsg_fs->iso_pub_id));
		printf("application id = %60s\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_application_id :
			 hsg_fs->iso_application_id));
		printf("data preparer = %60s\n", 
			(fs->fs_format == ISO_9660 ? iso_fs->iso_preparer_id :
			 hsg_fs->iso_preparer_id));
		printf("cdfs_mount: Volume Info. Block END\n");
	}
	fs->fs_ronly = 1;

	/* gfs has no knowledge of the following parameters, set them here */
	/*
	 * PRS - For field test, set m_bsize to MAXBSIZE
	 */
	mp->m_bsize = MAXBSIZE;
	fs->fs_ibsize = MAXBSIZE;
	mp->m_fstype = GT_CDFS;
	
	(void) cdfs_getfsdata(mp);

	/*
	 * Since ISO9660 files do not have unique numbers on disk,
	 * we must make our own. We will use the disk address of
	 * the corresponding directory entry.
	 */
	if (fs->fs_format == ISO_9660) {
		bcopy(iso_tdir->dir_extent_lsb, convert_extent.incoming, 
		      sizeof(int));
		fs->iso_rootino = 
			(convert_extent.outgoing + iso_tdir->dir_xar) * 
			(int)ISOFS_LBS(fs);
	} else {
		bcopy(hsg_tdir->dir_extent_lsb, convert_extent.incoming, 
		      sizeof(int));
		fs->iso_rootino = 
			(convert_extent.outgoing + hsg_tdir->dir_xar) * 
			(int)ISOFS_LBS(fs);
	}
	if (fs->iso_rootino % ISO_SECSIZE) {
		printf("cdfs_mount: directory record does not begin on a logical sector boundary\n");
		goto ERROR;
	}

	/*
	 * this gget is very order dependent, do it last
	 */
	
	if ((gp = gget(mp, fs->iso_rootino, 0, NULL)) == NULL) {
		uprintf("cdfs_mount: cannot find root inode\n");
		goto ERROR;
	}
	cdfs_gunlock(gp);

	/* point the mount table toward the root of the filesystem */
	
	mp->m_rootgp = gp;

	if (isodebug) {
		cprintf("cdfs_mount: rootgp 0x%x (%d)\n", gp, gp->g_number);
		cprintf("cdfs_mount: g_mp 0x%x ops 0x%x\n", gp->g_mp,
		gp->g_mp->m_ops);
	}

	return (mp);
ERROR:
	CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	(*bdevsw[major(dev)].d_close)(dev, FREAD);
	RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);

	/* something happened and we need to invalidate the buffer cache
	 * so that later we may re-use the device
	 */
	binval(dev, (struct gnode *) 0);

done:
	return(NULL);
}

cdfs_umount(mp, force)
	register struct	mount	*mp;
	register int force;
{
	register struct gnode *gp = mp->m_gnodp;
	register int stillopen;
	dev_t	dev = mp->m_dev;
	int saveaffinity;
	int i;

	dnlc_purge();
	xumount(dev); /* get rid of the sticky bitted files */

	 /* try to flush gnodes */
#ifdef QUOTA
	stillopen = gflush(dev, mp->m_qinod, mp->m_rootgp);
#else
	stillopen = gflush(dev, mp->m_rootgp);
#endif
	
 	if (stillopen < 0) {             /* someone has a file open */
		return(EBUSY);
	}

	(void) grele(mp->m_rootgp);
#ifdef QUOTA
	closedq(mp, 0);

	/* there is a nasty piece of baggage with quotas, we must reflush
	 * all the gnodes for the device to get rid of the quota gnode
	 */
	
	(void) gflush(dev, (struct gnode *) NULL, (struct gnode *) NULL);
#endif
	
	/* make the mounted directory accessible again */
	gp->g_flag &= ~GMOUNT;
	(void) grele(gp);

	if (!stillopen) {
		CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
		(*bdevsw[major(dev)].d_close)(dev, FREAD);
		RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	}

	return(NULL);
}
