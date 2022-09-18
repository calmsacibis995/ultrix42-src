#ifndef lint
static	char	*sccsid = "@(#)gfs_mount.c	4.3	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87 by			*
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
 *	Fix filesystem timestamping.
 *	Add dev argument to gfs_gupdat().
 *
 * 09 Nov 89 -- jaw
 *	remove support for asymmetric system calls.  Had to add code to 
 *	mount to force control to boot_cpu.  This is unfinished work
 *	in SMP.
 *
 * 25 Jul 89 -- chet
 *	Changes for new bflush() interface and redo update() routine
 *
 * 14 Jun 89 -- condylis
 *	Modified getpdev() to return a dev number that does not
 *	currently exist in the mount table.
 *
 * 06 Feb 89 -- prs
 *	Changed return value from EPERM to EACCES if a user level
 *	mount is attempted on a device with no execute permission in
 *	getmdev().
 *
 * 09 Jan 89 -- condylis
 *	Modified smount and umount to run on any processor.  Added
 *	mteinit() to initialize mount table entry SMP locks.
 *
 * 07 Dec 88 -- condylis
 *	Changed gfs_gupdat to insure it has a valid ref on a gnode
 *	before calling GUPDATE and to call grele to release the ref.
 *
 * 05 May 88 -- prs
 *	SMP - Added initialization of m_lk lock, that protects local
 *	file system data in super block.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added new kmalloc memory allocation to system.
 *
 * 29 Oct 87 -- chet
 *	add u.uerror value set in check_mountp()
 *
 * 12 May 87 -- prs
 *	Removed call to namei in smount. Created a new routine
 * 	check_mountp to perform namei and checking of the local
 *	mount point which is called from sfs mount routines
 *
 * 28 Apr 87 -- prs
 *	Turned on user level mount. Fixed smount to verify
 *	local mount point exists after call to sfs mount
 *	routine
 *
 * 10 Apr 87 -- logcher
 *	Added Charlie Brigg's fix to if statement in gfs_gupdat
 *	to update specfs
 *
 * 10 Mar 87 -- chet
 *	turn off user-level mounts unless GFSDEBUG is on
 *
 * 13 Feb 87 -- prs
 *	Changed call to bflush in update to only pass the mp parameter
 *
 * 15 Jan 87 -- prs
 *	Added nulling out the fs_data pointer in the mount structure if
 *	the sfs mount call fails
 *
 * 11 Sep 86 -- koehler
 *	changed name interface, allow the new options, fix umount 
 *	problems
 *
 * 16 Oct 86 -- koehler
 *	fixed a problem with mounting hp0a (major/minor 0/0)
 *
 ***********************************************************************/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/kernel.h"
#include "../h/conf.h"
#include "../h/kmalloc.h"
#include "../h/limits.h"
#include "../h/cpudata.h"

int turn_off_usrmnt = 0;

int nmount = 0;
extern struct lock_t lk_gnode;
struct lock_t lk_mount_table;

check_mountp(mp, pathname)
struct mount *mp;
char *pathname;
{
	register struct gnode	*gp;
	register struct nameidata *ndp = &u.u_nd;
	char *temp_pathname;

	temp_pathname = pathname;

	ndp->ni_nameiop = LOOKUP | FOLLOW | NOCACHE;
	ndp->ni_dirp = pathname;
	if ((gp = GNAMEI(ndp)) == NULL) {
		if (!u.u_error)
			u.u_error = EIO; /* just in case it wasn't set */
		return(NULL);
	}
	if (gp->g_count != 1) {		/* someone has this inode open */
		u.u_error = EBUSY;
		nchinval(gp->g_dev);
		gput(gp);
		return(NULL);
	}
	if ((gp->g_mode & GFMT) != GFDIR) { /* this mount point is not a dir */
		u.u_error = ENOTDIR;
		nchinval(gp->g_dev);		
		gput(gp);
		return(NULL);
	}
	if (u.u_uid) 
		if (turn_off_usrmnt || (gp->g_uid != u.u_uid)) {
			u.u_error = EPERM;
			nchinval(gp->g_dev);		
			gput(gp);
			return(NULL);
		}
	mp->m_gnodp = gp;
	return(1);
}

smount()
{
	register struct a {
		char	*fspecial;
		char	*fpathname;
		int	ronly;
		int	fs_type;	/* new gfs entry */
		char	*opts;		/* new gfs entry */
	} *uap;
	register struct mount	*mp;
	register struct fs_data *fs_data;
	int			num;
	struct mount		*(*mountsfs)();
	char *pathname;
	char *devname;
	int saveaffinity;
	extern struct gnode *rootdir;
	
	uap = (struct a *)u.u_ap;

	KM_ALLOC(pathname, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (pathname == NULL)
	{
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->fpathname, pathname, MAXPATHLEN,
				  (u_int *) 0)) {
		KM_FREE(pathname, KM_NAMEI);
		return;
	}
	if (*pathname != '/') {		/* we must have absolute pathnames */
		u.u_error = EINVAL;
		KM_FREE(pathname, KM_NAMEI);
		return;
	}
	KM_ALLOC(devname, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (devname == NULL) {
		KM_FREE(pathname, KM_NAMEI);
		u.u_error = EIO;
		return;
	}
	if (u.u_error = copyinstr(uap->fspecial, devname, MAXPATHLEN,
				  (u_int *) 0)) {
		KM_FREE(devname, KM_NAMEI);
		KM_FREE(pathname, KM_NAMEI);
		return;
	}
	/* system call not smp safe */
	saveaffinity = switch_affinity(boot_cpu_mask);
	
	for (mp = mount; mp < &mount[NMOUNT]; mp++) {
		if (mp->m_bufp == 0) {
			smp_lock(&mp->m_lk, LK_RETRY);
			if (mp->m_bufp == 0) {
				/* for reservation */
				mp->m_bufp = (struct buf *) NODEV;
				mp->m_dev = NODEV;
				smp_unlock(&mp->m_lk);
				break;
			}
			smp_unlock(&mp->m_lk);
       
		}
	}
	
	if (mp == &mount[NMOUNT]) {
		u.u_error = EMFILE;
		KM_FREE(pathname, KM_NAMEI);
		KM_FREE(devname, KM_NAMEI);
		switch_affinity(saveaffinity);
		return;
	}
	
	num = uap->fs_type;
	if (num < 0 || num >= NUM_FS) {	/* fs_type not in range 0-0xff */
		u.u_error = ENXIO;	/* this is a new error return */
		mp->m_bufp = NULL;
		KM_FREE(pathname, KM_NAMEI);
		KM_FREE(devname, KM_NAMEI);
		switch_affinity(saveaffinity);
		return;
	}
	mountsfs = MOUNTFS(num);
	if (!mountsfs) {			/* fs_type not configured */
		u.u_error = EOPNOTSUPP;	/* this is another new error */
		mp->m_bufp = NULL;
		KM_FREE(pathname, KM_NAMEI);
		KM_FREE(devname, KM_NAMEI);
		switch_affinity(saveaffinity);
		return;
	}
	KM_ALLOC(mp->m_fs_data, struct fs_data *, sizeof(struct fs_data), 
		 KM_MOUNT, KM_CLEAR);
	fs_data = mp->m_fs_data;

	if (mp == &mount[nmount])
		nmount++;
	fs_data->fd_uid = u.u_uid;		/* who is mounting */
	mp->m_gnodp = NULL;
	if (mp != (*mountsfs)(devname, pathname, uap->ronly, mp, uap->opts)) {
	        /*
		 *  something went wrong in the sfs, it will return
		 *  the error
		 */
		mp->m_dev = NODEV;
		KM_FREE(mp->m_fs_data, KM_MOUNT);
		mp->m_fs_data = NULL;
		if (mp == &mount[nmount - 1])
			nmount--;
		if (mp->m_gnodp) {
			nchinval(mp->m_gnodp->g_dev);
			gput(mp->m_gnodp);
			mp->m_gnodp = NULL;
		}
		mp->m_bufp = NULL;
		KM_FREE(devname, KM_NAMEI);
		KM_FREE(pathname, KM_NAMEI);
		switch_affinity(saveaffinity);
		return;
	}
	/* 
	 * Because of relative links, 
	 * we have to put what the user typed into the mount table 
	 */
 	copyinstr(uap->fpathname, fs_data->fd_path, MAXPATHLEN, (u_int *) 0); 
	bcopy(devname, fs_data->fd_devname, MAXPATHLEN);
	KM_FREE(devname, KM_NAMEI);
	KM_FREE(pathname, KM_NAMEI);
	fs_data->fd_dev = mp->m_dev;
	
	if (u.u_uid)
		mp->m_flags |= M_USRMNT;	/* nosuid & nodev */

	mp->m_gnodp->g_mpp = mp;
	mp->m_fstype = num;
	mp->m_gnodp->g_flag |= GMOUNT;
	gfs_unlock(mp->m_gnodp);
	mp->m_flgs |= MTE_DONE;
	switch_affinity(saveaffinity);
}

umount()
{
	register struct a {
		dev_t	fdev;
	} *uap = (struct a *) u.u_ap;
	dev_t	 		dev = uap->fdev;	
	register struct mount	*mp;

	GETMP(mp, dev);
	if ((mp == NULL) || (mp == (struct mount *) MSWAPX)) {
		u.u_error = EINVAL;
		return;
	}

	/* check perm and allow user level mount and umount */
	if (u.u_uid) {
		if (u.u_uid != mp->m_fs_data->fd_uid) {
			u.u_error = EPERM;
			return;
		}
	}

	/*
	 * Stop concurrent umounts on same mount table entry here.
	 * If we still have the correct mount table entry and
	 * this entry in mounted on and there isn't a umount
	 * already in progress on this entry.
	 */
	smp_lock(&mp->m_lk, LK_RETRY);
	if ((mp->m_dev == dev) && (mp->m_flgs & MTE_DONE) &&
	    !(mp->m_flgs & MTE_UMOUNT)) {
		mp->m_flgs |= MTE_UMOUNT;
		smp_unlock(&mp->m_lk);
	}
	else {
		smp_unlock(&mp->m_lk);
		u.u_error = EBUSY;
		return;
	}

	gfs_gupdat(mp->m_dev);
	bflush(mp->m_dev, (struct gnode *) 0, 0);

	if ((u.u_error = GUMOUNT(mp, 0)) == 0) {
		if (mp->m_bufp != (struct buf *) (NODEV))
			brelse(mp->m_bufp);
		KM_FREE(mp->m_fs_data, KM_MOUNT);
		mpurge(mp - &mount[0]);
		mp->m_dev = NODEV;
		mp->m_forw[0] = NULL;
		mp->m_forw[1] = NULL;
		mp->m_pad = 0;
		mp->m_fs_data = NULL;
		mp->m_gnodp = NULL;
		mp->m_rootgp = NULL;
		mp->m_qinod = NULL;
		mp->m_ops = NULL;
		mp->iostrat = NULL;
		smp_lock(&mp->m_lk, LK_RETRY);
		mp->m_flgs = 0;
		mp->m_bufp = NULL;
		smp_unlock(&mp->m_lk);
		binval(dev, (struct gnode *) 0);
		if ((mp - mount) == (nmount - 1))
			nmount--;
	} else {
		/*
		 * Allow umounts on this mount table entry again
		 */
		smp_lock(&mp->m_lk, LK_RETRY);
		mp->m_flgs &= ~MTE_UMOUNT;
		smp_unlock(&mp->m_lk);
	}
}

update()
{
	register struct mount *mp;
	register struct gnode *rgp;
	register int ret;
	extern	struct gnode *fref();
	
	for (mp = mount; mp < &mount[nmount]; mp++) {
		if (mp->m_dev == (dev_t) NODEV)
			continue;
		/*
		 * mount table entry is active
		 */
		if ((rgp = fref(mp, (dev_t)0)) != NULL) {
			if (mp->m_flags & M_MOD) {
				if (ISREADONLY(mp)) {
					printf("fs= %s\n",mp->m_path);
					panic("update: Read only file system");
				}
				ret = GSBUPDATE(mp, 0);
			} 
			grele(rgp);
		}
	}
	bflush(NODEV, (struct gnode *) 0, 0); /* flush 'em all */
}

/*
 * Note: this routine is called *ONLY* by sync() and umount()
 */
gfs_gupdat(dev)
	dev_t dev;
{
        register struct gnode *gp;
	register int ret;
	
	for (gp = gnode; gp < gnodeNGNODE; gp++) {
		if (smp_lock(&gp->g_lk, 0) == LK_WON) {
			if (gp->g_count == 0 ||
			    !gisready(gp) ||
			    ((gp->g_flag & (GMOD|GACC|GUPD|GCHG)) == 0))
				goto skip;

			if (dev != NODEV && gp->g_dev != dev)
				goto skip;

			if (!ISLOCAL(gp->g_mp) && 
			    (((gp->g_mode & GFMT) == GFREG) || 
			     ((gp->g_mode & GFMT) == GFDIR))) 
				goto skip;
		
			smp_lock(&lk_gnode, LK_RETRY);
			if (gp->g_count && gisready(gp)) {
				gp->g_count++;
				smp_unlock(&lk_gnode);
			}
			else {
				smp_unlock(&lk_gnode);
				goto skip;
			}

			ret = GUPDATE(gp, &time, &time, 0, u.u_cred);

			smp_unlock(&gp->g_lk);
			grele(gp);
			continue;
			
skip:	      		smp_unlock(&gp->g_lk);
		}
	}
}

int pdev_major = 0;
int pdev_minor = 0;

dev_t
getpdev()
{
	register int i = SHRT_MAX - (nblkdev+nchrdev+1);
	register int tnmount = nmount;
	register struct mount *tmp;
	register dev_t tdev;

	while  (i--) {
		if (pdev_major == 0) pdev_major = (nblkdev+nchrdev+1);
		/*
		 * minor 0 - 255
		 */
		if (++pdev_minor > (1<<8)-1) {
			/*
			 * major 0 - 127 (dev_t is short)
			 */
			if (++pdev_major > (1<<7)-1)
				pdev_major = (nblkdev+nchrdev+1);
			pdev_minor=0;
		}
		tdev = makedev(pdev_major,pdev_minor);
		/* Search mount table for entry using tdev */
		for (tmp = mount; tmp < &mount[tnmount]; tmp++) {
			if (tmp->m_dev == tdev)
				break;
		}
		/* Found a dev number not in use */
		if (tmp == &mount[tnmount])
			return(tdev);
	}
	return((dev_t)0);
}


/*
 * Common code for ufs_mount, ufs_umount and setquota.
 * Check that the user's argument is a reasonable
 * thing on which to mount, and return the device number if so.
 */
getmdev(pdev, fname, usr)
	dev_t *pdev;
	char *fname;
	int usr;
{
	register dev_t dev;
	register struct gnode *gp;
 	register struct nameidata *ndp = &u.u_nd;

	if (u.u_uid && !usr) {
		u.u_error = EPERM;
		return (EPERM);
	}

 	ndp->ni_nameiop = LOOKUP | FOLLOW;
 	ndp->ni_dirp = fname;
 	if ((gp = GNAMEI(ndp)) == NULL)
		return (u.u_error);

	/* user level mount -- user must have execute permission on dev */

	if (usr)
		if (access(gp, GEXEC)) {
			gput(gp);
			return(EACCES);
		}

	if ((gp->g_mode&GFMT) != GFBLK) {
		gput(gp);
		return (ENOTBLK);
	}
	dev = gp->g_rdev;
	if (major(dev) >= nblkdev) {
		gput(gp);
		return (ENXIO);
	}
	gput(gp);
	*pdev = dev;
	return (0);
}

/*
 * Initializes per mount entry smp locks
 */
mteinit()
{
	register struct mount *mp;

	lockinit(&lk_mount_table, &lock_eachfs_d);
	for (mp = mount; mp < &mount[NMOUNT]; mp++) {
		lockinit(&mp->m_lk, &lock_eachfs_d);
	}
}

