#ifndef lint
static	char	*sccsid = "@(#)vnodeops_gfs.c	4.6	(ULTRIX)	4/11/91";
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

/*
 *
 *   Modification history:
 *
 * 11 Apr 91 -- chet
 *	Fix truncate past end of file case for remote mapped files.
 *
 * 03 March 91 -- larry
 *	Fix vop_create().  Will not create a non-zero length file.
 *
 * 27 Feb 91 -- chet
 *	Fix remote utimes(2).
 *
 * 18 Feb 91 -- prs
 *	Added SAVE_DIRP to the source namei op in vop_rename(). Since
 *	the local file system rename routine assumes the source name
 *	string has not been altered, we must ensure namei restores it.
 *	namei will trash ni_dirp when symbolic links are followed without
 *	SAVE_DIRP.
 *
 * 27 Aug 90 -- chet
 *	Redo vop_link since target gp now arrives unlocked.
 *
 * 07 Mar 90 -- prs
 *	Fixed vop_rename() to unlock parent directories of target
 *	and source node before calling local rename routine.
 *
 * 09 Oct 89 -- prs
 *	Added checks to vop_rmdir() and vop_rename() to prevent 
 *	operations on mount points.
 *
 * 21 Sep 89 -- prs
 *	Modified vop_rename() to unlock sdp before call to GNAMEI
 *	to prevent a panic condition.
 *
 * 24 Apr 89 -- chet
 *	change vop_create() to not call GNAMEI with LOCKPARENT;
 *	LOCKPARENT required write permission in directory to overwrite
 *	a file with write permission.
 *
 * 06 Apr 89 -- prs
 *	Added SMP quota locks.
 *
 * 11 Oct 88 -- prs
 *	Added the restriction to vop_rmdir() that insures the gnode
 *	being operated on is a directory.
 *
 * 28 Sep 88 -- chet
 *	Add casts to unsigned for -1 checks in unsigned vattr fields
 *
 * 22 Aug 88 -- prs
 *	Changed vop_setattr() and vop_create() to only allow root to
 *	set the sticky bit on a regular file, and allow the owner of
 *	a directory to set the sticky bit on a directory they own.
 *
 * 04 Aug 88 -- chet
 *	Add checks for LINK_MAX and linked directories in vop_link().
 *
 * 28 Jul 88 -- prs
 *	Decremented the link count of the source gnode in vop_link(),
 * 	when an error condition is detected.
 *
 * 07 Mar 88 -- prs
 *	Removed parity bit check in vop_create() for I18N.
 * 
 * 14 Jan 88 -- chet
 *	Changed permission checks in vop_setattr() for access and mod times
 *	to owner
 *
 * 13 Nov 87 -- chet
 *	Removed vulnerability to protocol violation in vop_create().
 *
 * 15 Oct 87 -- chet
 *	Removed panics in vop_remove and vop_rdwr
 *
 * 14-Jul-87 -- cb
 *	mknod interface changed.
 *
 * 31-Mar-87 -- logcher
 *	Merged in Chet's changes, see comments from 1.16.1.1
 * 
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added VFIFO to gftovt_tab and
 *	GFPORT to vttogf_tab, changed GMAKNODE line in vop_create
 *	to allow mknod 
 */

/*
 * This file is an attempt to emulate the VFS operations done by the
 * NFS code with GFS operations. By convention, all gnodes are passed
 * in unlocked and remain unlocked, and gnodes returned are unlocked.
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../ufs/fs.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mount.h"
#include "../h/kernel.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/limits.h"
#include "../net/netinet/in.h"
#include "../net/rpc/types.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/vfs.h"
#include "../nfs/nfs.h"
#include "../nfs/vnode.h"
#include "../ufs/ufs_inode.h"
#ifdef QUOTA
#include "../h/quota.h"
#endif

/*
 * Convert between vnode types and gnode formats
 */

enum vtype gftovt_tab[] = {

	VNON, VCHR, VDIR, VBLK, VREG, VLNK, VSOCK, VFIFO, VBAD
};
int vttogf_tab[] = {
	0, GFREG, GFDIR, GFBLK, GFCHR, GFLNK, GFSOCK, GFPORT, GFMT
};

int
vop_getattr(gp, vap, cred)
	struct gnode *gp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int ret;
	
	ret = GGETVAL(gp);
	vap->va_type = IFTOVT(gp->g_mode);
	vap->va_mode = gp->g_mode;
	vap->va_uid = gp->g_uid;
	vap->va_gid = gp->g_gid;
	vap->va_fsid = gp->g_dev;
	vap->va_nodeid = gp->g_number;
	vap->va_nlink = gp->g_nlink;
	vap->va_size = gp->g_size;
	vap->va_atime = gp->g_atime;
	vap->va_mtime = gp->g_mtime;
	vap->va_ctime = gp->g_ctime;
	vap->va_rdev = gp->g_rdev;
	vap->va_blocks = gp->g_blocks;
	switch(gp->g_mode & GFMT) {

	case GFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;		
		break;

	case GFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = gp->g_mp->m_fs_data->fd_bsize;
		break;
	}
	return (0);
}

int
vop_setattr(gp, vap, cred)
	register struct gnode *gp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int error = 0;
	int changed = 0;
	struct timeval atime;
	struct timeval mtime;
	
	atime = *(timepick);
	mtime = *(timepick);
	
	/*
	 * cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1)) {
		return (EINVAL);
	}

	/* sanity check time values */
	if (vap->va_atime.tv_sec != -1) {
		if (vap->va_atime.tv_sec < 0)
			return (EINVAL);
		if (vap->va_atime.tv_usec < 0 ||
		    vap->va_atime.tv_usec >= 1000000)
			vap->va_atime.tv_usec = 0;
	}
	if (vap->va_mtime.tv_sec != -1) {
		if (vap->va_mtime.tv_sec < 0)
			return (EINVAL);
		if (vap->va_mtime.tv_usec < 0 ||
		    vap->va_mtime.tv_usec >= 1000000)
			vap->va_mtime.tv_usec = 0;
	}

	/*
	 * Change file access modes. Must be owner or su.
	 */
	if (vap->va_mode != (u_short) -1) {
		error = OWNER(gp, cred);
		if (error)
			goto out;
		gp->g_mode &= GFMT;
		gp->g_mode |= vap->va_mode & ~GFMT;
		/*
		 * Only root can set the sticky bit on a regular file.
		 * However, the owner of a directory can set the sticky
		 * bit, for their directory.
		 */
		if ((gp->g_mode & GFMT) != GFDIR) {
			if (u.u_uid)
				gp->g_mode &= ~GSVTX;
		}
		if (u.u_uid) {
			if (!groupmember(gp->g_gid))
				gp->g_mode &= ~GSGID;
		}
		gp->g_flag |= GCHG;
		++changed;
		/* Remember to xrele text if su ever allowed over net. */
	}

	/*
	 * Change file ownership (must be su).
	 */
	if ( ((vap->va_uid != -1) && (vap->va_uid != gp->g_uid)) ||
	     ((vap->va_gid != -1) && (vap->va_gid != gp->g_gid)) )
	{
		if (!suser()) {
			error = u.u_error;
			goto out;
		}
		error = chown2(gp, vap->va_uid, vap->va_gid);
		if (error)
			goto out;
		++changed;
	}

	/*
	 * Truncate file. Must have write permission and not be a directory.
	 */
	if (vap->va_size != (u_long) -1) {
		if ((gp->g_mode & GFMT) == GFDIR) {
			error = EISDIR;
			goto out;
		}
		if (access(gp, GWRITE)) {
			error = u.u_error;
			goto out;
		}
		/*
		 * Do the right thing with truncate size.
		 * Setting the size beyond current size is a mapped
		 * file'ism first seen with SysVR4 SunOS at Cthon '91.
		 */
		if (vap->va_size > gp->g_size) {
			struct iovec iov;
			struct uio uio;
			char c = '\0';

			iov.iov_base = &c;
			iov.iov_len = 1;
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_segflg = UIO_SYSSPACE;
			uio.uio_offset = vap->va_size;
			uio.uio_resid = 1;
			error = vop_rdwr(gp, &uio, UIO_WRITE,
					 IO_SYNC, u.u_cred);
		} else {
			(void)GTRUNC(gp, vap->va_size, cred);
			++changed;
		}
	}

	/*
	 * Change file access or modified times.
	 */

	if (vap->va_atime.tv_sec != -1) {
		error = OWNER(gp, cred);
		if (error) {
			goto out;
		}
		atime.tv_sec = vap->va_atime.tv_sec;
		atime.tv_usec = vap->va_atime.tv_usec;
		gp->g_flag |= GACC|GCHG;
		++changed;
	}
	if (vap->va_mtime.tv_sec != -1) {
		error = OWNER(gp, cred);
		if (error) {
			goto out;
		}
		mtime.tv_sec = vap->va_mtime.tv_sec;
		mtime.tv_usec = vap->va_mtime.tv_usec;
		gp->g_flag |= GUPD|GCHG;
		++changed;
	}

out:
	if (changed)
		(void) GUPDATE(gp, &atime, &mtime, 1, cred);
	return (error);
}

int
vop_lookup(dgp, name, gpp, cred)
	struct gnode *dgp;
	char *name;
	struct gnode **gpp;
	struct ucred *cred;
{
	struct nameidata *ndp = &u.u_nd;

	ndp->ni_dirp = name;
	ndp->ni_nameiop = LOOKUP | NOMOUNT;
	u.u_cdir = dgp;
	gfs_unlock(dgp);
	if(*gpp = GNAMEI(ndp))
		gfs_unlock(*gpp);
	gfs_lock(dgp);
	return (u.u_error);
}

int
vop_readlink(gp, uiop, cred)
	struct gnode *gp;
	struct uio *uiop;
	struct ucred *cred;
{
	(void) GREADLINK(gp, uiop);
	return (u.u_error);
}

int
vop_access(gp, mode, cred)
	struct gnode *gp;
	int mode;
	struct ucred *cred;
{
	u.u_error = 0;
	access(gp, mode);
	return (u.u_error);
}

int
vop_rdwr(gp, uiop, rw, syncflg, cred)
	struct gnode *gp;
	struct uio *uiop;
	int rw;
	int syncflg;
	struct ucred *cred;
{
	int error;

	if ((gp->g_mode & GFMT) == GFREG) {
		error = GRWGP(gp, uiop, rw, syncflg, cred);
	}
	else {
		error = u.u_error = EINVAL;
	}
	return (error);
}

int
vop_create(dgp, name, vap, exclusive, garbage, gpp, cred)
	struct gnode *dgp;
	register char *name;
	struct vattr *vap;
	int exclusive;
	int garbage;
	struct gnode **gpp;
	struct ucred *cred;
{
	register struct nameidata *ndp = &u.u_nd;
	register struct gnode *gp;
	register char *cp;
	
	/*
	 * Disallow filenames with slashes, since the GFS
	 * pathname parsing scheme assigns a special meaning to them.
	 * This is a problem: Sun's implementation allows clients to
	 * use whatever they want for pathname delimiters, whereas ours
	 * presupposes that "/" is a delimiter and cannot allow it
	 * to be used in a filename.  This may someday become an issue
	 * in a heterogeneous environment with Ultrix NFS servers.
	 */
	
	cp = name;
	while (*cp)
		/*
		 * Removed parity bit check for I18N.
		 */
		if (*cp == '/') {
			u.u_error = EINVAL;
			return(u.u_error);
		}
		else
			cp++;
	
	ndp->ni_dirp = name;
	ndp->ni_nameiop = CREATE | NOMOUNT;
	u.u_cdir = dgp;
	gfs_unlock(dgp);
	*gpp = GNAMEI(ndp);

	if (*gpp == NULL) {
		if (u.u_error)
			return (u.u_error);
		/*
		 * Only root can create a regular file with the sticky bit
		 * set.
		 */
		if (u.u_uid)
			vap->va_mode &= ~GSVTX;

		if ((vap->va_size != (u_long) 0) &&
		    (vap->va_size != (u_long) -1)) {
			u.u_error = EINVAL;		/* ??? */
			*gpp = ndp->ni_pdir;
			goto bad;
		}

		/*
		 * allow mknod over the wire 
		 */
		if ((*gpp = GMAKNODE(vap->va_mode, vap->va_rdev, ndp))
		    == (struct gnode *) GNOFUNC) {
			u.u_error = EOPNOTSUPP;
		        goto bad2;
		}

		if (*gpp == NULL)
			goto bad2;

	} else {
		if (exclusive) {
			u.u_error = EEXIST;
			goto bad;
		}

		gp = *gpp;

		if (access(gp, GWRITE))
			goto bad;

		if ((gp->g_mode&GFMT) == GFDIR) {
			u.u_error = EISDIR;
			goto bad;
		}

		if (vap->va_size == 0) {
			if (GTRUNC(gp, (u_long) 0, u.u_cred) == GNOFUNC) {
				u.u_error == EOPNOTSUPP;
			}
			if (u.u_error)
				goto bad;
		}
	}
	return(u.u_error);

bad:
	gput(*gpp);

bad2:	/* entry for *gpp already unlocked */
	*gpp = NULL;
	return (u.u_error);
}

int
vop_remove(pgp, name, cred)
	struct gnode *pgp;
	char *name;
	struct ucred *cred;
{
	struct nameidata *ndp = &u.u_nd;
	struct gnode *gp;
	
	ndp->ni_nameiop = DELETE | LOCKPARENT | NOMOUNT;
	ndp->ni_dirp = name;
	u.u_cdir = pgp;
	gfs_unlock(pgp);
	gp = GNAMEI(ndp);
	if (gp == NULL) {
		return (u.u_error);
	}

	/* Can't unlink directories -- use vop_rmdir. */
	if ((gp->g_mode & GFMT) == GFDIR) {
		u.u_error = EISDIR;
		goto out;
	}
	if (gp->g_dev != pgp->g_dev) {
		u.u_error = EBUSY;
		goto out;
	}
	if (gp->g_flag & GTEXT)
		xrele(gp);
	GUNLINK(gp, ndp);
out:
	if (gp != pgp) 
		gput(gp);
	gput(pgp);

	return (u.u_error);
}

int
vop_link(sgp, tdgp, name, savectime, cred)
        struct gnode *sgp;
        struct gnode *tdgp;
        char *name;
	struct timeval *savectime;
        struct ucred *cred;
{
        register struct nameidata *ndp = &u.u_nd;
        struct gnode *gp;
        int     error = 0;

        if (sgp->g_nlink >= LINK_MAX) {
                gput(sgp);
                grele(tdgp);
                u.u_error = EMLINK;
                return (u.u_error);
        }
        if (((sgp->g_mode & GFMT) == GFDIR) && !suser()) {
                gput(sgp);
                grele(tdgp);
                u.u_error = EPERM;
                return (u.u_error);
        }
        sgp->g_nlink++;
        sgp->g_flag |= GCHG;
        (void) GUPDATE(sgp, timepick, timepick, 1, cred);
	/* save ctime for successful cases */
	*savectime = sgp->g_ctime;	/* save ctime for successful cases */

        ndp->ni_nameiop = CREATE | NOMOUNT;
        ndp->ni_dirp = name;
        u.u_cdir = tdgp;
        gfs_unlock(sgp);
        gp = GNAMEI(ndp);
        if (u.u_error) {
                error = u.u_error;
                goto errout;
        }

        if (gp) {
                gput(gp);
                error = EEXIST;
                goto errout;
        }

        if (sgp->g_mp != tdgp->g_mp) {
                error = EXDEV;
                goto errout;
        }
        /* The UFS routine will do the de-referencing */
        (void) GLINK(sgp, ndp);
	grele(tdgp);
        return (u.u_error);

errout:
        gfs_lock(sgp);
        sgp->g_nlink--;
        sgp->g_flag |= GCHG;
        (void) GUPDATE(sgp, timepick, timepick, 1, cred);
	/* don't bother saving ctime for unsuccessful cases */
        gput(sgp);
        grele(tdgp);
        return (u.u_error = error);
}


int
vop_rename(sdp, sname, tdp, tname, cred)
	struct gnode *sdp;
	char *sname;
	struct gnode *tdp;
	char *tname;
	struct ucred *cred;
{
	struct nameidata tnd;
	struct nameidata *sndp = &u.u_nd;
	struct gnode *sgp;
	int ret;

	tnd.ni_nameiop = CREATE | LOCKPARENT | NOMOUNT;
	tnd.ni_segflg = UIO_SYSSPACE;
	tnd.ni_dirp = tname;

	/*
	 * If sname is a symbolic link, ni_dirp can be bogus
	 * when passed down to the local rename routine. SAVE_DIRP
	 * ensures the original ni_dirp remains in tact.
	 */
	sndp->ni_nameiop = DELETE | LOCKPARENT | NOMOUNT | SAVE_DIRP;
	sndp->ni_dirp = sname;
	u.u_cdir = sdp;
	/*
	 * XXX - Unlock sdp and tdp to guard against
	 * rename("/mnt/a", "/mnt/a/b")
	 */
	gfs_unlock(sdp);
	if (tdp != sdp)
		gfs_unlock(tdp);

	sgp = GNAMEI(sndp);
	if (sgp == NULL) {
		return (u.u_error);
	}
	/*
	 * XXX (cont) - If sgp and tdp are equal, target
	 * is a direct descendent of source.
	 */
	if (sgp == tdp) {
		gput(sgp);
		if (sgp == sdp)
			grele(sdp);
		else
			gput(sdp);
		u.u_error = EINVAL;
		return (u.u_error);
	}
	if (sgp->g_flag & GMOUNT) {
		gput(sgp);
		if (sgp == sdp)
			grele(sdp);
		else
			gput(sdp);
		u.u_error = EBUSY;
		return (u.u_error);
	}
	u.u_cdir = tdp;
	ret = GRENAMEG(sgp, sdp, sndp, tdp, &tnd, NOMOUNT);
	return (u.u_error);
}

int
vop_symlink(pgp, fname, vap, tname, cred)
	struct gnode *pgp;
	char *fname;
	struct vattr *vap;	/* NOT DEALT WITH */
	char *tname;
	struct ucred *cred;
{
	register struct nameidata *ndp = &u.u_nd;
	struct 	uio auio;
	struct iovec aiov;
	register struct gnode *gp;
	
	ndp->ni_nameiop = CREATE | LOCKPARENT | NOMOUNT;
	ndp->ni_dirp = fname;
	u.u_cdir = pgp;
	gfs_unlock(pgp);
	gp = GNAMEI(ndp);
	if (gp) {
		gput(gp);
		gput(ndp->ni_pdir);
		return (EEXIST);
	}
	if (u.u_error)
		return (u.u_error);
	if(GSYMLINK(ndp, tname) == GNOFUNC)
		u.u_error = EOPNOTSUPP;
	return (u.u_error);
}

int
vop_mkdir(pgp, name, vap, gpp, cred)
	struct gnode *pgp;
	char *name;
	struct vattr *vap;
	struct gnode **gpp;
	struct ucred *cred;
{
	register struct nameidata *ndp = &u.u_nd;

	ndp->ni_nameiop = CREATE | LOCKPARENT | NOMOUNT;
	ndp->ni_dirp = name;
	u.u_cdir = pgp;
	gfs_unlock(pgp);
	*gpp = GNAMEI(ndp);
	if (u.u_error) {
		return (u.u_error);
	}
	if (*gpp != NULL) {
		gput(*gpp);
		gput(ndp->ni_pdir);
		return (EEXIST);
	}
	*gpp = GMKDIR(ndp->ni_pdir, name, vap->va_mode);
	return (u.u_error);	
}

int
vop_rmdir(pgp, name, cred)
	struct gnode *pgp;
	char *name;
	struct ucred *cred;
{
	register struct nameidata *ndp = &u.u_nd;
	struct gnode *gp;
	int ret;
	
	ndp->ni_nameiop = DELETE | LOCKPARENT | NOMOUNT;
	ndp->ni_dirp = name;
	u.u_cdir = pgp;
	gfs_unlock(pgp);
	gp = GNAMEI(ndp);
	if (gp == NULL) {
		return (u.u_error);
	}
	/*
	 * GFS requires the sfs be passed a directory gp. Lets make
	 * sure the client was as thoughtful.
	 */
	if ((gp->g_mode & GFMT) != GFDIR) {
		/*
		 * Two gputs are ok here because rmdir(".") will
		 * never execute this conditional.
		 */
		gput(ndp->ni_pdir);
		gput(gp);
		u.u_error = ENOTDIR;
		return(ENOTDIR);
	}
	/*
	 * Don't allow rmdir's of mount points. Name translations
	 * would fail when attempting to traverse removed entry.
	 */
	if (gp->g_flag & GMOUNT) {
		gput(gp);
		if (gp == ndp->ni_pdir)
			grele(ndp->ni_pdir);
		else
			gput(ndp->ni_pdir);
		u.u_error = EBUSY;
		return(EBUSY);
	}
	ret = GRMDIR(gp, ndp);
	return (u.u_error);
}

int
vop_readdir(gp, uiop, cred)
	struct gnode *gp;
	struct uio *uiop;
	struct ucred *cred;
{
	if (access(gp, GREAD)) {
		u.u_error = EPERM;
		return(EPERM);			/* XXX */
	}
	(void) GGETDIRENTS(gp, uiop, cred);
	return (u.u_error);
}

int
vop_brelse(gp, bp)
	struct gnode *gp;
	struct buf *bp;
{
	bp->b_resid = 0;
	brelse(bp);
}

int
vop_bread(gp, lbn, bpp)
	struct gnode *gp;
	daddr_t lbn;
	struct buf **bpp;
{
	register struct buf *bp;
	register daddr_t bn;
	register int size;

	size = blksize(FS(gp), gp, lbn);
	bn = GBMAP(gp, lbn, B_READ, 0, 0);

	if ((long)bn < 0) {
		bp = geteblk(size);
		clrbuf(bp);
	} else if (gp->g_lastr + 1 == lbn) {
		bp = breada(gp->g_dev, bn, size, rablock, rasize,
			(struct gnode *) 0);
	} else {
		bp = bread(gp->g_dev, bn, size, (struct gnode *) 0);
	}
	gp->g_lastr = lbn;
	gp->g_flag |= GACC;
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	} else {
		*bpp = bp;
		return (0);
	}
}

/*
 * Perform chown operation on gnode gp.  This is similar to chown1()
 * in gfs_syscalls.c, but we leave some preliminary error checking and
 * the GUPDATE operation for the caller to do.  This routine should not
 * exist; please merge these someday.
 */

struct dquot *inoquota();

chown2(gp, uid, gid)
	register struct gnode *gp;
	register int uid, gid;
{
#ifdef QUOTA
	register long change;
#endif

	if (uid == -1)
		uid = gp->g_uid;
	if (gid == -1)
		gid = gp->g_gid;

#ifdef QUOTA
	if (gp->g_uid == uid)
		change = 0;
	else
		change = gp->g_blocks;
	(void) chkdq(gp, -change, 1);
	(void) chkiq(gp->g_dev, gp, gp->g_uid, 1);
	dquot_lock(gp->g_dquot);
	dqrele(gp->g_dquot);
	dquot_unlock(gp->g_dquot);
#endif
	gp->g_uid = uid;
	gp->g_gid = gid;
	gp->g_flag |= (GCHG | GCID);
		
	if (u.u_ruid != 0)
		gp->g_mode &= ~(GSUID|GSGID);

#ifdef QUOTA
	gp->g_dquot = inoquota(gp);
	(void) chkdq(gp, change, 1);
	(void) chkiq(gp->g_dev, (struct gnode *)NULL, uid, 1);
	return (u.u_error);	
#else
	return(0);
#endif
}

gattr_to_nattr(gp, na)
	register struct gnode *gp;
	register struct nfsfattr *na;
{
	na->na_type = (enum nfsftype)IFTOVT(gp->g_mode);
	na->na_mode = gp->g_mode;
	na->na_uid = gp->g_uid;
	na->na_gid = gp->g_gid;
	na->na_fsid = gp->g_dev;
	na->na_nodeid = gp->g_number;
	na->na_nlink = gp->g_nlink;
	na->na_size = gp->g_size;
	na->na_atime = gp->g_atime;
	na->na_mtime = gp->g_mtime;
	na->na_ctime = gp->g_ctime;
	na->na_rdev = gp->g_rdev;
	na->na_blocks = gp->g_blocks;
/*	na->na_blocksize =  KLUDGE: CALLER MUST DO THIS */
}
/*
 * Special kludge for fifos (named pipes)  
 * [to adhere to (unwritten) NFS Protocol Spec]
 *
 * VFIFO is not in the protocol spec (VNON will be replaced by VFIFO)
 * so the over-the-wire representation is VCHR with a '-1' device number.
 *
 * NOTE: This kludge becomes unnecessary with the Protocol Revision,
 *       but it may be necessary to support it (backwards compatibility).
 */

/* identify fifo in nfs attributes */
#define NA_ISFIFO(NA)	(((NA)->na_type == NFS_FIFO_TYPE) && \
			    ((NA)->na_rdev == NFS_FIFO_DEV))


nattr_to_gattr(gp, na)
	register struct gnode *gp;
	register struct nfsfattr *na;
{
	gp->g_mode = na->na_mode;
	gp->g_uid = na->na_uid;
	gp->g_gid = na->na_gid;
	gp->g_nlink = na->na_nlink; 
	gp->g_size = na->na_size; 
	gp->g_atime = na->na_atime;
	gp->g_mtime = na->na_mtime;
	gp->g_ctime = na->na_ctime; 
	gp->g_blocks = na->na_blocks;
	gp->g_rdev = na->na_rdev;

	/*   *** From nfssrc 4.0 ***
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * special over-the-wire type to the VFIFO type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (NA_ISFIFO(na)) {
		gp->g_mode = (gp->g_mode & ~GFMT) | GFPORT;
		gp->g_rdev = 0;
	}
}

