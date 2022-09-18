#ifndef lint
static char *sccsid = "@(#)gfs_xxx.c	4.1	(ULTRIX)	7/2/90";
#endif

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
 *
 *			Modification History
 *
 *	09-Jan-89	Condylis
 *	Made SMP changes to ustat.
 *
 * 	12-11-87	Robin L. and Larry C.
 *	Added new kmalloc memory allocation to system.
 *
 *	11 Sep 86	Koehler
 *	namei interface change
 *
 *	31-Oct-85	Stephen Reilly
 *	Added check to the ustat call to insure that the mount structure
 *	is valid.
 *
 * 	07-Oct-85	Stephen Reilly
 * 	Added the code for ustat system call
 *
 * 	09-Sep-85 	Stephen Reilly
 *	Modified to handle the new 4.3BSD namei code.
 *
 *	27-Mar-85	Stephen Reilly
 * 003- Removed an optimization that would cause the superblock not to get
 *	updated with the new paritition information
 *
 *	16-Nov-84	Stephen Reilly
 * 002- Added a new panic meesage that checks to make sure that there is
 *	an equivalent blk to char ioctl
 *
 *	06-Oct-84	Stephen Reilly
 * 001- Added rsblk,ssblk ptcmp which is used by any of the disk
 *      driver's to get the paritition info.
 *
 *	ufs_xxx.c	6.1	83/07/29	
 *
 ***********************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/errno.h"
#include "../h/kmalloc.h"
#include "../h/ustat.h"

#ifdef COMPAT
#include "../h/file.h"
#include "../h/kernel.h"

/*
 * Oh, how backwards compatibility is ugly!!!
 */
struct	ostat {
	dev_t	ost_dev;
	u_short	ost_ino;
	u_short ost_mode;
	short  	ost_nlink;
	short  	ost_uid;
	short  	ost_gid;
	dev_t	ost_rdev;
	int	ost_size;
	int	ost_atime;
	int	ost_mtime;
	int	ost_ctime;
};

/*
 * The old fstat system call.
 */
ofstat()
{
	register struct file *fp;
	register struct a {
		int	fd;
		struct ostat *sb;
	} *uap = (struct a *)u.u_ap;
	extern struct file *getgnode();

	fp = getgnode(uap->fd);
	if (fp == NULL)
		return;
	if (fp->f_data == (caddr_t)0) {
		u.u_error = EBADF;
		return;
	}
	ostat1((struct gnode *)fp->f_data, uap->sb);
}

/*
 * Old stat system call.  This version follows links.
 */
ostat()
{
	register struct gnode *gp;
	register struct a {
		char	*fname;
		struct ostat *sb;
	} *uap;
 	register struct nameidata *ndp = &u.u_nd;

	uap = (struct a *)u.u_ap;
 	ndp->ni_nameiop = LOOKUP | FOLLOW;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if(ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if(u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
	(u_int *) 0)) {
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		return;
	}

 	gp = GNAMEI(ndp);

	KM_FREE(ndp->ni_dirp, KM_NAMEI);
	if (gp == NULL)
		return;
	ostat1(gp, uap->sb);
	gput(gp);
}

ostat1(gp, ub)
	register struct gnode *gp;
	register struct ostat *ub;
{
	struct ostat _ds;
	register struct ostat *ds = &_ds;
	
	gp->g_flags |= (GACC | GUPD);
	if (GUPDATE(gp, &time, &time, 0, u.u_cred) == GNOFUNC) {
		u.u_error = EOPNOTSUPP;
		return;
	}
	/*
	 * Copy from gnode table
	 */
	ds->ost_dev = gp->g_dev;
	ds->ost_ino = (short)gp->g_number;
	ds->ost_mode = (u_short)gp->g_mode;
	ds->ost_nlink = gp->g_nlink;
	ds->ost_uid = (short)gp->g_uid;
	ds->ost_gid = (short)gp->g_gid;
	ds->ost_rdev = (dev_t)gp->g_rdev;
	ds->ost_size = (int)gp->g_size;
	ds->ost_atime = (int)gp->g_atime;
	ds->ost_mtime = (int)gp->g_mtime;
	ds->ost_ctime = (int)gp->g_ctime;
	u.u_error = copyout((caddr_t)ds, (caddr_t)ub, sizeof(struct ostat));
}

/*
 * Set GUPD and GACC times on file.
 * Can't set GCHG.
 */
outime()
{
	register struct a {
		char	*fname;
		time_t	*tptr;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp;
	time_t tv[2];
	struct timeval tv0, tv1;

	if ((gp = owner(uap->fname, FOLLOW)) == NULL)
		return;
	u.u_error = copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof (tv));
	if (u.u_error == 0) {
		gp->g_flag |= GACC|GUPD|GCHG;
		tv0.tv_sec = tv[0]; tv0.tv_usec = 0;
		tv1.tv_sec = tv[1]; tv1.tv_usec = 0;
		if (GUPDATE(gp, &tv0, &tv1, 0, u.u_cred) == GNOFUNC) {
			u.u_error = EOPNOTSUPP;
			return;
		}
	}
	gput(gp);
}
#endif

/*
 *	The ustat system call routine
 */
ustat()
{
	register struct a {
		dev_t	dev;			/* major minor number */
		struct ustat *buf;		/* user destination */
	} *uap = (struct a *)u.u_ap;		/* users parameters */
	struct ustat ustat;			/* computed ustat struct */
	register struct fs_data *fs_data;
	register struct mount *mp;
	register struct ustat *pustat = &ustat;
	register struct gnode *rgp;
	extern struct gnode *fref();
	
	/*
	 * Create ref on file system to hold it in place.
	 */
	GETMP(mp, uap->dev);
	if ((rgp = fref(mp, uap->dev)) == NULL) {
		u.u_error = EINVAL;
		return;
	}
	fs_data = GGETFSDATA(mp);
	smp_lock(&mp->m_lk, LK_RETRY);
	pustat->f_tfree = fs_data->fd_bfree;
	pustat->f_tinode = fs_data->fd_gfree;
	bcopy(fs_data->fd_path, pustat->f_fname, sizeof(pustat->f_fname));
	smp_unlock(&mp->m_lk);
	grele(rgp);
	pustat->f_fpack[0] = '\0';
	if (copyout(pustat, uap->buf, sizeof(struct ustat)))
		u.u_error = EFAULT;
}
