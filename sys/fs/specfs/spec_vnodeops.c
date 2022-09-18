#ifndef lint
static	char	*sccsid = "@(#)spec_vnodeops.c	4.5	(ULTRIX)	4/30/91";
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

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	Modification History
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 30-Jan-91 -- lp
 *	Return all nbufio errors to application (rather than block).
 *	Fix for SYBASE.
 *
 * 21 Jan 91 -- dws
 *	Added code to spec_open() to prevent opening a raw disk device
 *	for write if it has a filesystem mounted on the corresponding
 *	block device.
 *	
 * 06 Mar 90 -- scott
 *	allow negative uio_offset for AUD_NO
 *
 * 25 Jul 89 -- chet
 *	Changes for syncronous filesystems and new bdwrite() interface
 *
 * 20 Jan 89 -- prs
 *      Set the GACC bit in the gnode flags field when reading
 *      a character device in spec_rwgp().
 *
 * 02 Nov 88 -- jaw
 * 	put switch to master around n-buffered I/O calles.
 *
 * 14 Sep 88 -- jaw
 *	added lock around select calls to driver routines.
 *
 * 20 Aug 88 -- jmartin
 *	lock mfind/munhash pair
 *
 * 19 May 88 -- prs
 *	SMP - Added locks around calls to driver routines.
 *
 * 16 May 88 -- prs
 *	Removed code in spec_open that called the device ioctl
 *	routine, and verified the device was not write locked.
 *
 * 24 Mar 88 -- prs
 *	Added code to spec_open that prevents opening with
 *	write permission a mounted block device (except root).
 *
 * 10 Feb 88 -- prs
 *	Modified to support new fifo code.
 *
 * 12 Jan 88 -- Fred Glover
 *	Add spec_rlock routine for Sys-V file locking
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/kernel.h"
#include "../h/gnode_common.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/devio.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/cmap.h"
#include "../ufs/ufs_mount.h"
#include "../net/netinet/in.h"
#include "../net/rpc/types.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/vfs.h"
#include "../net/rpc/lockmgr.h"
#include "../h/cpudata.h"

struct gnode * spec_namei();
int spec_open();
int spec_close();
int spec_rwgp();
int spec_ioctl();
int spec_select();
int spec_access();
int spec_link();
int spec_unlink();
int spec_lock();
int spec_unlock();
int spec_rele();
int spec_inactive();
int spec_badop();
int spec_rename();
int spec_noop();
int spec_trunc();
int spec_syncgp();
int ufs_seek();
int spec_stat();
int spec_gupdat();
int spec_getval();
int spec_rlock();

struct gnode_ops SPEC_gnode_ops = {
	spec_namei,	/* namei */
	spec_link,
	spec_unlink,
	0,		/* mkdir */
	spec_noop,	/* rmdir */
	0,		/* maknode */
	spec_rename,	/* rename */
	spec_noop,	/* getdirents */
	spec_rele,	/* rele */
	spec_syncgp,
	spec_trunc,	/* trunc */
	0,		/* getval */
	spec_rwgp,
	spec_rlock,	/* rlock */
	ufs_seek,
	spec_stat,
	spec_lock,
	spec_unlock,
	spec_gupdat,
	spec_open,
	spec_close,
	spec_select,
	0,		/* readlink */
	0,		/* symlink */
	spec_ioctl,	/* fcntl */
	0,		/* freegn */
	0		/* bmap */
};

struct gnode_ops *spec_gnodeops = &SPEC_gnode_ops;

/*
 * open a special file (device)
 */
/*ARGSUSED*/
int
spec_open(gp, mode)
	register struct gnode *gp;
	register int mode;
{
	
	dev_t dev = (dev_t)gp->g_rdev;
	register int maj = major(dev);
	register struct mount *mp;
	int saveaffinity, status;

	switch ((gp->g_mode) & GFMT) {

	case GFCHR:
		if ((u_int)maj >= nchrdev)
			return (ENXIO);

		CALL_TO_NONSMP_DRIVER(cdevsw[maj], saveaffinity);
		status = (*cdevsw[maj].d_open)(dev, mode);
		/*
		 * If opening a disk device for write, force a failure if 
		 * the corresponding block device has a mounted filesystem
		 * on it.  
		 */
		if ((status == 0) && (mode & FWRITE)) { 
			struct devget devget;
			int stat;

			bzero(&devget, sizeof(devget));
			stat = (*cdevsw[maj].d_ioctl)(dev, DEVIOCGET, &devget);

			if (stat == 0 && devget.category == DEV_DISK) {
				struct ufs_partid partid;
				struct gnode *rgp;

				devget_to_partid(&devget, &partid);
				for (mp = mount; mp < &mount[nmount]; mp++) {
					if (rgp = (struct gnode *)fref(mp, (dev_t)0)) {
						if (mp->m_fs_data && ISLOCAL(mp) &&
						    !bcmp((char *)&mp->m_fs_data->fd_spare[0], 
						    (char *)&partid, sizeof(partid))) {
							grele(rgp);
							(*cdevsw[maj].d_close)(dev, 0);
							status = EROFS;
							break;
						}
						grele(rgp);
					} 
				}
			}
		}
		RETURN_FROM_NONSMP_DRIVER(cdevsw[maj], saveaffinity);
		return(status);

	case GFBLK:
		if ((u_int)major(dev) >= nblkdev)
			return (ENXIO);
		/*
		 * Prevent opening a block device that has a file system
		 * mounted on it with write mode, except for root. We must 
		 * allow root to be opened because of fsck (for now).
		 */
		if (mode & FWRITE) {
		       GETMP(mp, dev);
		       /*
			* If GETMP returns a mp, then the block device is 
			* mounted. If the mp returned is not the first slot in 
			* the mount table, then we know the device we are
			* trying to open is not the root device.
			*/
		       if ((mp) && (mp != &mount[0]))
				return(EROFS);
	       }
	       CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	       status = (*bdevsw[major(dev)].d_open)(dev, mode);
	       RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
	       return (status);

	}
	return (0);
}

/*ARGSUSED*/
int
spec_close(gp, flag)
	struct gnode *gp;
	int flag;
{
	struct mount *mp;
	int saveaffinity;
	dev_t dev;

	dev = gp->g_rdev;
	switch((gp->g_mode)& GFMT) {
	case GFCHR:
		if ((u_int)major(dev) >= nchrdev)
			return;
		CALL_TO_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
		(*cdevsw[major(dev)].d_close) (dev, flag);
		RETURN_FROM_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
		break;

	case GFBLK:
		if ((u_int)major(dev) >= nblkdev)
			return;
		GETMP(mp,dev);
		if ((mp != NULL) && (mp != (struct mount *) MSWAPX))
			return;

		CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
		(*bdevsw[major(dev)].d_close) (dev, flag);
		RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)], saveaffinity);
		break;

	default:
		return (0);
	}
}

/*
 * read or write a vnode
 */
/*ARGSUSED*/
int
spec_rwgp(gp, uiop, rw, ioflag, cred)
	struct gnode *gp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	dev_t dev = (dev_t) gp->g_rdev;
	struct buf *bp;
	daddr_t lbn, bn;
	register int n, on, type;
	int size;
	long bsize;
	extern int mem_no;
	int error = 0;
	int saveaffinity;

	type = gp->g_mode & GFMT;

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwsp");
	if (rw == UIO_READ && uiop->uio_resid == 0)
		return (0);
	if ((uiop->uio_offset < 0 || (uiop->uio_offset + uiop->uio_resid) < 0)
	    && !(type == GFCHR && (mem_no == major(dev) || major(dev) == AUD_NO))) {
		return (EINVAL);
	}
	if (type == GFCHR) {
		if (rw == UIO_READ) {
			gp->g_flag |= GACC;
			if (cdevsw[major(dev)].d_strat && (gp->g_flag&ASYNC)) {
				CALL_TO_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
				error=aphysio(cdevsw[major(dev)].d_strat,
					dev, B_READ, uiop);
				RETURN_FROM_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
			} else {
				CALL_TO_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
				error = (*cdevsw[major(dev)].d_read)(dev, uiop);
				RETURN_FROM_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
			  }	
		} else {
			gp->g_flag |= GUPD|GCHG;
			if (cdevsw[major(dev)].d_strat && (gp->g_flag&ASYNC)) {
				CALL_TO_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
				error=aphysio(cdevsw[major(dev)].d_strat,
					dev, B_WRITE, uiop);
				RETURN_FROM_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
			} else {
			        CALL_TO_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
				error = (*cdevsw[major(dev)].d_write)(dev, uiop);
			        RETURN_FROM_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
			  }

		}
		return (error);
	} else if (type != GFBLK) {
		return (EOPNOTSUPP);
	}
	if (uiop->uio_resid == 0) {
		return (0);
	}
	bsize = BLKDEV_IOSIZE;
	u.u_error = 0;
	do {
		lbn = uiop->uio_offset / bsize;
		on = uiop->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uiop->uio_resid);
		bn = lbn * (BLKDEV_IOSIZE/DEV_BSIZE);
		rablock = bn + (BLKDEV_IOSIZE/DEV_BSIZE);
		rasize = size = bsize;
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else if (gp->g_lastr + 1 == lbn)
				bp = breada(dev, bn, size, rablock,
					    rasize, (struct gnode *) NULL);
			else
				bp = bread(dev, bn, size, 
					   (struct gnode *)NULL);
			gp->g_lastr = lbn;
		} else {
			int i, count;
			extern struct cmap *mfind();
			int s;

			count = howmany(size, DEV_BSIZE);
			s = splimp();
			for (i = 0; i < count; i += CLBYTES/DEV_BSIZE) {
				smp_lock(&lk_cmap, LK_RETRY);
				if (mfind(dev, (daddr_t)(bn + i), gp))
					munhash(dev, (daddr_t)(bn + i), gp);
				smp_unlock(&lk_cmap);
			}
			(void)splx(s);
			if (n == bsize) 
				bp = getblk(dev, bn, size, 
					    (struct gnode *) NULL);
			else
				bp = bread(dev, bn, size, 
	                                    (struct gnode *)NULL);
		}
		n = MIN(n, bp->b_bcount - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uiop);
		if (rw == UIO_READ) {
			brelse(bp);
		} else {
			if (ioflag & IO_SYNC) 
				bwrite(bp);
			else if (n + on == bsize) {
				bawrite(bp);
			} else
				bdwrite(bp);
			gp->g_flag |= GUPD|GCHG;
		}
	} while (u.u_error == 0 && uiop->uio_resid > 0 && n != 0);
	if (
	    (rw == UIO_WRITE) &&
	    (ioflag & IO_SYNC) &&
	    (gp->g_flag & (GUPD|GCHG))
	)
		(void) gp->g_altops->go_gupdat(gp, timepick, timepick, 1,
					       (struct ucred *) 0);

	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}

/*ARGSUSED*/
int
spec_ioctl(gp, com, data, flag, cred)
	struct gnode *gp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	dev_t dev = gp->g_rdev;
	register int type = gp->g_mode & GFMT;
	int saveaffinity;
	int error;

	if (type == GFCHR) {
	       	u.u_r.r_val1 = 0;
		if (setjmp(&u.u_qsave)) {
			if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
                               	return(EINTR);
			u.u_eosys = RESTARTSYS;
			return (0);
		}
		CALL_TO_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
		error = (*cdevsw[major(dev)].d_ioctl)(dev, com, data, flag);
		RETURN_FROM_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
		return(error);
	} else
	        return(ENOTTY);
}

/*ARGSUSED*/
int
spec_select(gp, rw, cred)
	struct gnode *gp;
	int rw;
	struct ucred *cred;
{
	int saveaffinity,error;

	if ((gp->g_mode & GFMT) != GFCHR)
		panic("spec_select");



	CALL_TO_NONSMP_DRIVER(cdevsw[major(gp->g_rdev)], saveaffinity);
	error = (*cdevsw[major(gp->g_rdev)].d_select)(gp->g_rdev, rw);
	RETURN_FROM_NONSMP_DRIVER(cdevsw[major(gp->g_rdev)], saveaffinity);

	return (error);
}
int
spec_link(gp, ndp)
	struct gnode *gp;
	struct nameidata *ndp;
{
	return (gp->g_altops->go_link (gp, ndp));
}
int
spec_unlink(gp, ndp)
	struct gnode *gp;
	struct nameidata *ndp;
{
	return (gp->g_altops->go_unlink (gp, ndp));
}

int
spec_rele(gp)
	struct gnode *gp;
{
	return (gp->g_altops->go_rele (gp));
}

int
spec_lock(gp)
	struct gnode *gp;
{
	return (gp->g_altops->go_lock (gp));
}

int
spec_unlock(gp)
	struct gnode *gp;
{
	return (gp->g_altops->go_unlock (gp));
}

struct gnode *
spec_namei(ndp)
	struct nameidata *ndp;
{
	return (0);
}


spec_syncgp(gp, cred)
	register struct gnode *gp;
	struct ucred *cred;

{
	if ((gp->g_flag	& (GCHG|GUPD)) == 0)
		return (0);

	
	return(gp->g_altops->go_syncgp(gp,cred));
}
int
spec_stat(gp, sb)
	struct gnode *gp;
	struct stat *sb;
{
	(gp->g_altops->go_stat)(gp, sb);
	if ((gp->g_mode & GFMT) == GFBLK)
		sb->st_blksize = BLKDEV_IOSIZE;
	else if ((gp->g_mode & GFMT) == GFCHR)
		sb->st_blksize = MAXBSIZE;	

	return(0);
}
int
spec_gupdat(gp, ta, tm, waitfor, cred)
	struct gnode *gp;
	struct timeval *ta, *tm;
	int waitfor;
	struct ucred *cred;
{
	return(gp->g_altops->go_gupdat(gp, ta, tm, waitfor, cred));
}

int
spec_rename(gp, ssd, s_ndp, tsd, t_ndp, flag)
	register struct gnode *gp;
	struct gnode *ssd, *tsd;
	struct nameidata *s_ndp;
	struct nameidata *t_ndp;
	int flag;

{
	return (gp->g_altops->go_rename(gp, ssd, s_ndp, tsd, t_ndp, flag));
}
int
spec_noop()
{
	return (EINVAL);
}
int
spec_trunc(gp, newsize, cred)
	struct gnode *gp;
	unsigned newsize;
	struct ucred *cred;
{
	return (gp->g_altops->go_trunc(gp,newsize,cred));
}
int
spec_getval(gp)
	struct gnode *gp;
{
	return(gp->g_altops->go_getval(gp, u.u_cred));
}


int
spec_badop()
{
	panic("spec_badop");
}

/*
 * Record-locking requests are passed back to the real vnode handler.
 */

int
spec_rlock(gp, ld, cmd, fp)
	struct gnode *gp;
	struct flock *ld;
	int cmd;
	struct file *fp;
{
	return (gp->g_altops->go_rlock(gp, ld, cmd, fp));
}
