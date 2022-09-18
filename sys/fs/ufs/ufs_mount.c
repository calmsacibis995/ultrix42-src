#ifndef lint
static	char	*sccsid = "@(#)ufs_mount.c	4.4	(ULTRIX)	2/28/91";
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
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 21 Jan 91 -- dws
 *	Added initialization of partition ID in ufs_mount().
 *
 * 17 Nov 89 -- prs
 *	Fixed the error message in ufs_mount() when the default
 *	clean byte timeout factor does not exist.
 *
 * 14 Jun 89 -- condylis
 *      Added code to insure we are placing a unique dev
 *      number in the mount table entry.
 *
 * 14 Jun 89 -- prs
 *	Added clean byte timeout logic.
 *
 * 13-Jun-89 -- Fred Canter
 *	Fix improper use of DEVIOCGET category_stat.
 *
 * 03-May-89 -- Tim Burke
 *      Allow for the unit number of root disks to be 3 digits; ie ra100a.
 *
 * 06 Apr 89 -- prs
 *	Added SMP quota locks.
 *
 * 28 Jul 88 -- prs
 *	SMP - System call turn on.
 *
 * 19 May 88 -- prs
 *	SMP - Added locks around calls to driver routines.
 *
 * 06 Apr 88 -- prs
 *      Changed handling of the clean byte in the super block
 *      from the fs_fmod field to fs_clean.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added new kmalloc memory allocation to system.
 *
 * 27 Oct 87 -- rsp
 *	Added bzero just before DEVIOCGET request.
 *
 * 12 May 87 -- prs
 *	Added call to check_mountp to verify local mount point
 *	exists.
 *
 * 02 Mar 87 -- logcher
 *	Merged in diskless changes, added changes for new ops
 *
 * 15 Jan 87 -- prs
 *	Added or'ing in the M_QUOTA bit to the mount structures flags
 *	field.
 *
 * 04 Dec 86 -- prs
 *	Made changes to not set FS_CLEAN byte if a file system
 *	was mounted with -o force in ufs_sbupdat.
 *
 * 11 Sep 86 -- koehler
 *	made changes for synchronous fs and user level mounts
 *
 ***********************************************************************/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode_common.h"
#include "../ufs/ufs_inode.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../ufs/fs.h"
#include "../h/fs_types.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/kernel.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/devio.h"
#include "../ufs/ufs_mount.h"
#include "../h/kmalloc.h"
#include "../h/cpudata.h"

int		ufs_umount(),	ufs_sbupdat(),	ufs_ginit(),	ufs_inactive();
struct gnode	*ufs_namei();
int		ufs_link(),	ufs_unlink(),	ufs_rmdir();
struct gnode	*ufs_mkdir();
struct gnode	*ufs_maknode();
int		ufs_rename(),	ufs_getdirent();
struct gnode	*ufs_galloc();
int		ufs_syncgp(),	ufs_gfree(),	ufs_gtrunc();
int		ufs_rwgp();
int		ufs_rlock();
int		ufs_seek(),	ufs_stat(),	ufs_glock();
int		ufs_gunlock(),	ufs_gupdat(),	ufs_open();
int		ufs_close(),	ufs_select(),	ufs_readlink();
int		ufs_symlink();
struct fs_data	*ufs_getfsdata();
int		ufs_fcntl(),	ufs_bmap();


struct	mount_ops Ufs_mount_ops = {
/* begin mount ops */
	ufs_umount,
	ufs_sbupdat,
	ufs_ginit,
	0,		/* match */
	0,		/* reclaim */
	ufs_inactive,
	ufs_getfsdata,
};
struct	gnode_ops Ufs_gnode_ops = {
/* begin gnode ops */
	ufs_namei,
	ufs_link,
	ufs_unlink,
	ufs_mkdir,
	ufs_rmdir,
	ufs_maknode,
	ufs_rename,
	ufs_getdirent,
	0,
	ufs_syncgp,
	ufs_gtrunc,
	0, 		/*getval*/
	ufs_rwgp,
	ufs_rlock,
	ufs_seek,
	ufs_stat,
	ufs_glock,
	ufs_gunlock,
	ufs_gupdat,
	ufs_open,
	ufs_close,
	ufs_select,
	ufs_readlink,
	ufs_symlink,
	ufs_fcntl,
	0,		/* gfreegn */
	ufs_bmap
};

struct  mount_ops  *ufs_mount_ops = &Ufs_mount_ops;
struct  gnode_ops  *ufs_gnode_ops = &Ufs_gnode_ops;

gno_t rootino = (gno_t) ROOTINO;

/* this routine has lousy error codes */
/* this routine has races if running twice */
struct mount *
ufs_mount(devname, name, ronly, mp, fs_specific)
	char *devname;
	char *name;
	int ronly;
	register struct mount *mp;
	struct ufs_specific *fs_specific;
{
	dev_t	dev;
	register struct buf *tp = NULL;
	register struct buf *bp = NULL;
	register struct fs *fs;
	register struct mount *nmp;
	register struct gnode *gp;
	int blks;
	caddr_t space;
	int i, size;
	int root;
	struct gnode gnode;
	struct devget devget;
	struct ufs_specific ufs_sp;
	int force = 0;
	int saveaffinity;
	extern struct lock_t lk_mount_table;

	/* special case this when we are mounting the root fs */
	
	root = devname == NULL;
	
	if (!root) { /* we are not mounting / */
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
			&ufs_sp, sizeof(ufs_sp));
			if (u.u_error)
				goto done;
			fs_specific = &ufs_sp;
		}
		/*
		 * check_mountp will verify local mount point exists and is
		 * ok to mount on.
		 */
		if (!(check_mountp(mp, name)))
			goto done;
	} else {
		force = 1;
		dev = rootdev;		/* map the root device from config */
	}

	/* 
	 * this is a hack but I can't see a way around it
	 * we need a gnode before we have the system mounted so we push 
	 * one on the stack and hand craft it. The gget at the end replaces
	 * this fake gnode with the real one.
	 */
		
	gnode.g_mode = GFBLK;
	gnode.g_dev = gnode.g_rdev = dev;
	gnode.g_ops = ufs_gnode_ops;	/* set pointer to file ops */
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
	mp->m_ops = ufs_mount_ops;

	mp->m_flags = ((ronly) ? M_RONLY : 0) | M_LOCAL;
	if (fs_specific != NULL) {
		int pg_thresh = fs_specific->ufs_pgthresh * 1024;
		mp->m_flags |= (fs_specific->ufs_flags & (M_RONLY | M_NOEXEC |
			M_QUOTA | M_NOSUID | M_NODEV | M_FORCE | M_SYNC));

		mp->m_fs_data->fd_pgthresh = 
			clrnd(btoc((pg_thresh > MINPGTHRESH) ?
				pg_thresh : MINPGTHRESH));
		
		/* only the superuser can mount forceably */

		force = (fs_specific->ufs_flags & M_FORCE) && (u.u_uid == 0);
	} else {
		/*
		 * if we don't specify the threshhold, use 64kB
		 */
		mp->m_fs_data->fd_pgthresh = clrnd(btoc(MINPGTHRESH * 8));
	}

	CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	u.u_error = (*bdevsw[major(dev)].d_open)(dev, ronly ? FREAD : FREAD|FWRITE);
	RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	if (u.u_error)
		goto done;

	bzero(&devget,sizeof(struct devget));
	CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	u.u_error = (*bdevsw[major(dev)].d_ioctl)(dev, DEVIOCGET, &devget, 0);
	RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);

	if (u.u_error) {
		if (!root) {
			printf("file system device ioctl failure");
		} else {
			printf("root file system device ioctl failure");
		}
	}

	if (devget.stat & DEV_OFFLINE) {
		u.u_error = ENODEV;
		goto ERROR;
	}
	if ((devget.stat & DEV_WRTLCK) && !ronly) {
		if (!root) {
			u.u_error = EROFS;
			goto ERROR;
		} else {
			CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
			(*bdevsw[major(dev)].d_close)(dev,
			 ronly ? FREAD : FREAD | FWRITE);
			RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
			ronly = 1;
			CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
			u.u_error = (*bdevsw[major(dev)].d_open)
					(dev, ronly ? FREAD : FREAD|FWRITE);
			RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
			if (u.u_error)
				goto done;
			u.u_error = EROFS;
		}
	}

	/* get the superblock */
	tp = bread(dev, SBLOCK, SBSIZE, (struct gnode *) NULL);
	if (tp->b_flags & B_ERROR)
		goto ERROR;

	fs = tp->b_un.b_fs;

	/*
	 *	Check the magic number and see 
	 *	if we have a valid filesystem.
	 */
	if (fs->fs_magic != FS_MAGIC ) {		/*001*/
 		u.u_error = EINVAL;		/* also needs translation */
		goto ERROR;
	}
	
	/* 
	 * only root can mount a non-cleaned filesystem and then only
	 * forcibly
	 */

	if ((fs->fs_clean != FS_CLEAN) && !force) {
		uprintf("ufs_mount: fs %s not cleaned -- please fsck\n",
		devname);
		u.u_error = EINVAL;
		goto ERROR;
	}

	bp = geteblk((int)fs->fs_sbsize);
	mp->m_bufp = bp;

	/* map the superblock into a buffer not connected with a device */
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
	   (u_int)fs->fs_sbsize);
	brelse(tp);
	tp = 0;

	fs = bp->b_un.b_fs;
	if (!root)
		bcopy(name, fs->fs_fsmnt, MAXMNTLEN);
	else
		bcopy("/", fs->fs_fsmnt, sizeof("/"));
		
	fs->fs_ronly = (ronly != 0);
		
	/* need to check writeability of device */
	if (ronly == 0)
		mp->m_flags |= M_MOD;
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	KM_ALLOC(space, caddr_t, fs->fs_cssize, KM_TEMP, KM_CALL);

	if (space == 0) {
 		u.u_error = ENOMEM;
		goto ERROR;
	}
	
	/* get the cylinder groups */
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		tp = bread(dev, fsbtodb(fs, fs->fs_csaddr + i), size, 
			   (struct gnode *) NULL);
		if (tp->b_flags&B_ERROR) {
		        KM_FREE(space, KM_TEMP);
			goto ERROR;
		}
		bcopy((caddr_t)tp->b_un.b_addr, space, (u_int)size);
		fs->fs_csp[i / fs->fs_frag] = (struct csum *)space;
		space += size;
		brelse(tp);
		tp = 0;
	}
	
	/* gfs has no knowledge of the following parameters, set them here */
	mp->m_bsize = fs->fs_bsize;
	mp->m_fstype = GT_ULTRIX;
	
	(void) ufs_getfsdata(mp);
	
	/* this gget is very order dependent, do it last */
	
	if ((gp = gget(mp, ROOTINO, 0, NULL)) == NULL) 
		panic("ufs_mount: cannot find root inode");
	ufs_gunlock(gp);
	
	/* point the mount table toward the root of the filesystem */
	
	mp->m_rootgp = gp;
	gp->g_ops = ufs_gnode_ops;

	/*
	 * once we are mounted, make no presumptions on the cleanliness
	 * of the filesystem.
	 */
	
	fs->fs_clean = 0;
	mp->m_nupdate = FSCLEAN_UPDATES;

	if ((!root) && (!ronly))
		CHECK_CLEAN_THRESHOLD(fs, 
				      devname, 
				      (fs->fs_deftimer == 0 ? 1 : fs->fs_deftimer),
				      "mounts");
	if(!(ronly || root))
		ufs_sbupdat(mp, 0);
	if (root) {
		devname = mp->m_fs_data->fd_devname;
		bcopy("/dev/",devname,5);
		bcopy(devget.dev_name,&devname[5],strlen(devget.dev_name));
		i = strlen(devname);
		if(devget.unit_num > 99) {
			devname[i++] = '0' + devget.unit_num/100;
		}
		if(devget.unit_num > 9) {
			devname[i++] = '0' + ((devget.unit_num%100)/10);
		}
		devname[i++] = '0' + devget.unit_num%10;
		devname[i++] = 'a' + (devget.category_stat & DEV_DPMASK);
		devname[i++] = '\0';
		bcopy("/", mp->m_fs_data->fd_path, sizeof("/"));
		inittodr(fs->fs_time);
	}

	/*
	 * Set up partition id
	 */
	devget_to_partid(&devget, (struct ufs_partid *)&mp->m_fs_data->fd_spare[0]);

	return (mp);
ERROR:
	CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	(*bdevsw[major(dev)].d_close)(dev, ronly ? FREAD : FREAD | FWRITE);
	RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);

	/* something happened and we need to invalidate the buffer cache
	 * so that later we may re-use the device
	 */
	binval(dev, (struct gnode *) 0);

done:
	return(NULL);
}

ufs_umount(mp, force)
	register struct	mount	*mp;
	register int force;
{
	register struct gnode *gp = mp->m_gnodp;
	register struct fs  *fs;
	register int stillopen;
	dev_t	dev = mp->m_dev;
	int saveaffinity;

	nchinval(dev); /* flush the name cache */
	xumount(dev); /* get rid of the sticky bitted files */
	if (!ISREADONLY(mp))
		ufs_sbupdat(mp, 0);		/* flush the superblock */

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
	
	/* mark the filesystem as clean */
	if (!ISREADONLY(mp))
		ufs_sbupdat(mp, 1);		

	/* free the cylinder group stuff */
	fs = mp->m_bufp->b_un.b_fs;
	KM_FREE((caddr_t)fs->fs_csp[0], KM_TEMP);
	
	if (!stillopen) {
		CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
		(*bdevsw[major(dev)].d_close)(dev, !fs->fs_ronly);
		RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	}

	return(NULL);
}
/*
 ************************* Disk Layout ***********************
 *  |-------------|------------|------------|--------------------|--------|
 *  | Super block | cg blk  #0 | inode blk  | Data blocks        | Alt sb |
 *  |             |            |            |                    |        | ...
 *  | cg totals   | cg 0 sum   | ipg inodes | data blk 1 of cg 0 |        |
 *  |             |            |            | contains cgsum     |        |
 *  |             |            |            | structures for all |        |
 *  |             |            |            | cg's in fs.        |        |
 *  |---------------------------------------------------------------------|
 *
 *  ufs_alloc's routines update the cg totals in the super block, and
 *  the cylinder group summary information in the cg block, and the
 *  appropriate cg summary structure in the first data block of cg 0.
 *  However, it only synchronously writes out the cg block, and only
 *  marks the super block as modified, and does nothing to update the
 *  information in the data block. What ufs_sbupdat is doing, is
 *  synchronously writing the super block, and the cgsum information in
 *  the first data block of cg 0. By this time, the cg block summary info
 *  is on the disk. UFS uses all three summaries as a consistency check.
 *
 *  Since the super block contains the free block maps, inconsistencies
 *  can occur if a power failure happens, after a cg block is synched
 *  to disk, and before the super block is written out. I am not sure
 *  if this can or should be fixed. However, with this in mind, we should
 *  still make some attempt to keep the cgsum info in each cg, consistent
 *  with the copy in the data block 1. Currently, this is not guarenteed
 *  because fs_lock(mp) cannot be held during a bwrite. This is not
 *  serious, because most cgsum areas are 1K in length.
 */

ufs_sbupdat(mp, flag)
	register struct mount *mp;
	int flag;
{
	register struct fs *fs = mp->m_bufp->b_un.b_fs;
	register struct buf *bp;
	register int blks, i, size;
	caddr_t space;
	
	bp = getblk(mp->m_dev, SBLOCK, (int)fs->fs_sbsize,
		    (struct gnode *) NULL);
	fs_lock(mp);
	mp->m_flags &= ~ M_MOD;
	fs->fs_time = timepick->tv_sec;
	/*
	 * Don't set FS_CLEAN byte if file system was force mounted.
	 */
	fs->fs_clean = (flag == 1 && !(mp->m_fs_data->fd_flags & M_FORCE))
	  ? FS_CLEAN : 1;

	if (mp->m_rootgp != NULL)
		verify_clean_threshold(fs, mp);
	/*
	 * we use 0 instead of gp because the superblock and such is not
	 * really in the buffer cache
	 */
	
	bcopy((caddr_t)fs, bp->b_un.b_addr, (u_int)fs->fs_sbsize);
	fs_unlock(mp);
	bwrite(bp);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp[0];
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(mp->m_dev, fsbtodb(fs, fs->fs_csaddr + i),
			    size, (struct gnode *) NULL);
	        fs_lock(mp);
		bcopy(space, bp->b_un.b_addr, (u_int)size);
		space += size;
		fs_unlock(mp);
		bwrite(bp);
	}
}

verify_clean_threshold(fs, mp)
	register struct fs *fs;
	register struct mount *mp;
{
	u_int sdelta;
	u_int edelta;
	char *devname = mp->m_fs_data->fd_devname;

	sdelta = fs->fs_lastfsck / 86400;
	edelta = timepick->tv_sec / 86400;
	if (fs->fs_lastfsck) {
		if ((sdelta > edelta) || (edelta - sdelta >= 60)) {
			fs->fs_cleantimer = 0;
			fs->fs_lastfsck = 0;
		}
	}

	if (--mp->m_nupdate <= 1) {
		if (!fs->fs_cleantimer || fs->fs_cleantimer-- <= 1)
			fs->fs_cleantimer = 0;
		mp->m_nupdate = FSCLEAN_UPDATES;
	}
}
