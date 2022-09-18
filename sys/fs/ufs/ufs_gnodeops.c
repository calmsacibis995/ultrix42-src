#ifndef lint
static	char	*sccsid = "@(#)ufs_gnodeops.c	4.5	(ULTRIX)	2/28/91";
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
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 22-Jan-91 - prs
 *      Added fast symbolic link support.
 *
 * 27 Sep 90 -- prs
 *	Protect ufs_rwgp() from a negative uio_offset
 *	being passed to it.
 *
 *  7 Jul 90 -- chet
 *	Make call to ufs_gupdat() in all cases in ufs_rwgp() instead
 *	of only when updating meta-data.
 * 
 *  7 Feb 89 -- prs
 *	Returned number of bytes written from a call to ufs_rwgp()
 *	if part of request is successful.
 *
 * 10 Dec 89 -- chet
 *	Add age_wbuffers policy switch.
 *
 * 25 Jul 89 -- chet
 *	Changes for new buffer cache organization and syncronous filesystems
 *
 * 25 Jul 88 -- jmartin
 *	Lock mfind/munhash operation with lk_cmap.
 *
 *  4 Apr 88 -- Fred G.
 *	Modify ufs_rlock () to declare kernel_locking as external.
 *
 *  2 Mar 88 -- chet
 *	Added age_buffers and stickyhack switches in ufs_rwgp().
 * 
 * 26 Jan 88 -- chet
 *	add access check in ufs_getdirent()
 *
 * 26 Jan 88 -- fglover
 *	create ufs_rlock, which will support both kernal and daemon 
 *	based Sys-V region locking
 *
 * 14 Jan 88 -- chet
 *	return gennum in stat structure from stat() and fstat()
 *
 * 14 Jul 87 -- cb
 * 	Added dev_t to mknod
 *
 *
 * 10 Jun 87 -- prs
 *	Initialized structure member in ufs_open.
 *
 * 11 May 87 -- chet
 *	Changed ufs_bmap() interface to get feedback about
 *	on-disk structure changes. Avoid doing synchronous
 *	disk inode updates unless this structure changes.
 *
 * 28 Apr 87 -- prs
 *	Changed ufs_rwgp to use the ucred real uid instead of
 *	the real uid in the user structure
 *
 * 29 Jan 87 -- chet
 *	add new arg to bdwrite() calls.
 *
 * 15 Dec 86 -- depp
 *	Returned to rdwri() the munhashing of text blocks on write.
 *	This caused the "panic: chgd c_page".
 *	
 * 04 Dec 86 -- prs
 *	Fixed code in ufs_open. Code now closes the file is an
 *	attempt was made to open with write mode a write locked
 *	device. Also ufs_open will return after the open call to
 *	the driver if the NDELAY flag is set.
 *
 * 23 Oct 86 -- chet
 *	implemented IO_SYNC operation in ufs_rwgp(); made sure
 *	that gnode was updated before return.
 *
 * 23 Oct 86 -- prs
 *	Added code to return EROFS in ufs_open, if an attempt is being
 *	made to open a write locked block or character special file for
 *	writing.
 *
 * 11 Sep 86 -- koehler
 *	introduced bmap function, made changes to fsdata so that it
 *	returned the correct data
 *
 * 11 Mar 86 -- lp
 *	Added n-buffered hooks to rwip & ioctl routines.
 *
 * 23 Dec 85 -- Shaughnessy
 *	Added code to set/reset the syncronous write flag in
 *	the inode. Also made sure inode was updated immediately after
 *	syncronous write is performed. 
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 16 Apr 85 -- depp
 *	Fixed routine "openi"; It didn't return error correctly when
 *	routine "openp" was called
 *									*
 * 15 Mar 85 -- funding							*
 *	Added named pipe support (re. System V named pipes)		*
 *									*
 * 	I added some fixes from berkeley to fix the inode reference
 * count from going negative.
 * Rich
 *
 *	Modified 25-Oct-84 -- jrs
 *	Add various bug fixes
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode_common.h"	
#include "../ufs/ufs_inode.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../ufs/fs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/devio.h"
#include "../h/tty.h"
#include "../h/cmap.h"
#include "../h/stat.h"
#include "../h/kernel.h"
#include "../h/exec.h"

#define OSF_FASTLINK 0x0001

/*
 * Don't cache blocks in non-executable files with the sticky bit
 * set. Used to keep swap files from cluttering up the data cache.
 */
int	stickyhack = 0;

	extern int kernel_locking;  /* Sys-V locking: default is kernel */
                                    /* kernel == 1;  daemon == 0        */


ufs_rwgp(gp, uio, rw, ioflag, cred)
	register struct gnode *gp;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	dev_t dev = (dev_t)gp->g_rdev;
	struct buf *bp;
	struct fs *fs;
	daddr_t lbn, bn;
	register int n, on, type;
	int size;
	long bsize;
	extern int mem_no;
	int error = 0;
	int transfer_count = uio->uio_resid;
	u_short execmask = (GEXEC | (GEXEC >> 3) | (GEXEC >> 6));
	extern int delay_wbuffers;

	/*
	 * update_flag is a strange beast. If called to do a
	 * synchronous write (ioflag == IO_SYNC), then we must
	 * flush the gnode to disk after doing the write
	 * when any on-disk structures have changed.
	 * For performance reasons, we want to
	 * avoid this expensive operation if they haven't changed.
	 * Since ufs_bmap() can change these structures, we must
	 * be able to tell when it did. This is done by sending
	 * ufs_bmap() the address of update_flag as the sync flag
	 * when we are doing a synchronous write, and 0 otherwise.
	 * ufs_bmap() will set update_flag to 1 when we must
	 * flush the gnode, and will leave it unchanged otherwise
	 * We set update_flag ourselves whenever the file is extended,
	 * since the new size must be put out to disk.
	 */
	int update_flag = 0;

	type = gp->g_mode & GFMT;

	if (uio->uio_resid == 0)
		return (0);

	if (uio->uio_offset < 0) {
		u.u_error = EINVAL;
		return(EINVAL);
	}
	dev = gp->g_dev;
	fs = FS(gp);
	bsize = fs->fs_bsize;
	do {
		lbn = uio->uio_offset / bsize;
		on = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);

		if (rw == UIO_READ) {
		  register int diff = gp->g_size - uio->uio_offset;
		  if (diff <= 0)
		    return (0);
		  if (diff < n)
		    n = diff;
		}

		bn = ufs_bmap(gp, (int)lbn,
			      rw==UIO_WRITE ? B_WRITE : B_READ,
			      (int)(on+n),
			      (ioflag & IO_SYNC) ? &update_flag : 0);

		if (u.u_error || rw == UIO_WRITE && (long)bn<0) {
			/*
			 * In POSIX mode, if part of this write request
			 * was successful, return no error so the actual
			 * number of bytes written can be calculated in 
			 * rwuio(). Setting u_error to zero here ensures
			 * the number of bytes transfered will be returned
			 * and syscall() won't overwrite with a -1;
			 */
			if ((u.u_procp->p_progenv == A_POSIX) &&
			    (rw == UIO_WRITE) &&
			    (transfer_count != uio->uio_resid))
				u.u_error = 0;
			return (u.u_error);
		}
		if (rw == UIO_WRITE &&
		    uio->uio_offset + n > gp->g_size &&
		    (type == GFDIR || type == GFREG ||
		     type == GFLNK)) {
		  gp->g_size = uio->uio_offset + n;
		  if (ioflag & IO_SYNC)
		    /* we must flush gnode before return */
		    update_flag = 1;
		}

		size = blksize(fs, gp, lbn);
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else if (gp->g_lastr + 1 == lbn)
				bp = breada(dev, bn, size, rablock, rasize, 
					    (struct gnode *) NULL);
			else
				bp = bread(dev, bn, size,
					   (struct gnode *) NULL);
			gp->g_lastr = lbn;
		} else {
			int i, count;
			struct text *xp = gp->g_textp;
			extern struct cmap *mfind();
			int s;

			count = howmany(size, DEV_BSIZE);
			/*
			 * mfind/munhash can be a lengthy operation, so we
			 * lock/unlock through each circuit of the loop.
			 */
			s = splimp();
			for (i = 0; i < count; i += CLSIZE)  {
				smp_lock(&lk_cmap, LK_RETRY);
				if (mfind(dev, bn + i, gp))
					munhash(dev, bn + i, gp);
				smp_unlock(&lk_cmap);
			}
			(void)splx(s);
			if (xp) {
				/* sanity check the structs */
				if (xp->x_gptr != gp) {
					printf("ufs_rwgp: messed up gp, xp");
					printf("gp %X xp %X\n",gp,xp);
					panic("ufs_rwgp: messed up gp, xp");
				}
				if (xp->x_count > 1 || (gp->g_flag & GSVTX)) {
					printf("textp = %X gp = %X\n",xp,gp);
					panic("ufs_rwgp: illegal text reuse");
				}
				xuntext(xp);
			}
			if (n == bsize) 
				bp = getblk(dev, bn, size,
					    (struct gnode *) NULL);
			else
				bp = bread(dev, bn, size,
					   (struct gnode *) NULL);
		}
		n = MIN(n, size - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error =
		    uiomove(bp->b_un.b_addr+on, n, rw, uio);

		/* Don't cache non-executable files with the sticky bit on; */
		/* used to avoid cacheing remote swap files */
		if (stickyhack && (ioflag & IO_SYNC) && (gp->g_mode & GSVTX)
		    && (gp->g_mode & execmask) == 0)
		  bp->b_flags |= B_NOCACHE;

		if (rw == UIO_READ) {
			brelse(bp);
		} else {

			/*
			 * If writing a directory, or performing syncronous
			 * writes, then call bwrite to write synchronously.
			 */

			if (
			    ((gp->g_mode & GFMT) == GFDIR) ||
			    (ioflag & IO_SYNC) ||
			    (gp->g_mp->m_flags & M_SYNC)
			   )
				bwrite(bp);
			else if (n + on == bsize) {
				if (delay_wbuffers)
					bdwrite(bp);
				else
					bawrite(bp);
			} else
				bdwrite(bp);
			gp->g_flag |= GUPD|GCHG;
			if (cred->cr_ruid != 0)
				gp->g_mode &= ~(GSUID|GSGID);
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);

	/*
	 * If synchronous write, and on-disk structures
	 * changed, then get gnode out to disk now, else only update
	 * the modification time.
	 */
	if (ioflag & IO_SYNC) {
		if (update_flag)
			(void) ufs_gupdat(gp, timepick, timepick, 1,
					  (struct ucred *) 0);
		else
			(void) ufs_gupdat(gp, timepick, timepick, 0,
					  (struct ucred *) 0);
	}

	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}


ufs_fcntl(gp, cmd, arg, flag, cred)
	register struct gnode *gp;
	register int cmd;
	register caddr_t arg;
	register int flag;
	struct ucred *cred;
{
	register int fmt = gp->g_mode & GFMT;
	dev_t dev;

	switch (fmt) {
		case GFREG:
		case GFDIR:
			if (cmd == FIONREAD) {
				*(off_t *)arg = gp->g_size - flag;
				return (0);
			}
			if (cmd == FIONBIO || cmd == FIOASYNC ||
			    cmd == FIOSINUSE || cmd == FIOCINUSE) /* XXX */
				return (0);			/* XXX */
			/* fall into ... */
		default:
			return (ENOTTY);
	}
}

ufs_select(gp, rw, cred)
	register struct gnode *gp;
	register int rw;
	struct ucred *cred;
{ 
  return (1);		/* XXX */
}

ufs_close(gp, flag)
	register struct gnode *gp;
	register int flag;
{
  return;
}

ufs_open(gp, mode)
	register struct gnode *gp;
	register int mode;
{
	return (0);
}

ufs_symlink(ndp, target_name)
	register struct nameidata *ndp;
	register char *target_name;
{
	register int len;
	struct uio auio;
	struct iovec aiov;
	register struct gnode *gp;
	struct gnode *ufs_maknode();
	
	if((gp = ufs_maknode(GFLNK | 0777, (dev_t) 0, ndp)) == NULL) 
		return;

	len = strlen(target_name);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = target_name;
	aiov.iov_len = auio.uio_resid = len;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_offset = 0;
	
	u.u_error = ufs_rwgp(gp, &auio, UIO_WRITE, 0,  u.u_cred);
	gput(gp);
}


ufs_readlink(gp, auio)
	register struct gnode *gp;
	register struct uio *auio;
{
	int size, error;

	if (gp->g_size > auio->uio_resid)
	        return(ERANGE);
	if (G_TO_I(gp)->di_flags & OSF_FASTLINK) {
		size = gp->g_size;
		error = uiomove(G_TO_I(gp)->di_db, size, UIO_READ, auio);
		gp->g_flag |= GACC;
		u.u_error = error;
		return(error);
	}
	u.u_error = ufs_rwgp(gp, auio, UIO_READ, 0, u.u_cred);
}

ufs_stat(gp, sb)
	register struct gnode *gp;
	register struct stat *sb;
{

        gfs_lock(gp);
	if ((gp)->g_flag&(GUPD|GACC|GCHG)) {
		(gp)->g_flag |= GMOD;
		if ((gp)->g_flag&GACC) {
			(gp)->g_atime.tv_sec = timepick->tv_sec;
			(gp)->g_atime.tv_usec = timepick->tv_usec;
		}
		if ((gp)->g_flag&GUPD) {
			(gp)->g_mtime.tv_sec = timepick->tv_sec;
			(gp)->g_mtime.tv_usec = timepick->tv_usec;
		}
		if ((gp)->g_flag&GCHG) {
			(gp)->g_ctime.tv_sec = timepick->tv_sec;
			(gp)->g_ctime.tv_usec = timepick->tv_usec;
		}
		(gp)->g_flag &= ~(GACC|GUPD|GCHG);
	}
	gfs_unlock(gp);

	/*
	 * Copy from gnode table
	 */
	sb->st_dev = gp->g_dev;
	sb->st_ino = gp->g_number;
	sb->st_mode = gp->g_mode;
	sb->st_nlink = gp->g_nlink;
	sb->st_uid = gp->g_uid;
	sb->st_gid = gp->g_gid;
	sb->st_rdev = (dev_t)gp->g_rdev;
	sb->st_size = gp->g_size;
	sb->st_atime = gp->g_atime.tv_sec;
	sb->st_spare1 = gp->g_atime.tv_usec;
	sb->st_mtime = gp->g_mtime.tv_sec;
	sb->st_spare2 = gp->g_mtime.tv_usec;
	sb->st_ctime = gp->g_ctime.tv_sec;
	sb->st_spare3 = gp->g_ctime.tv_usec;
	/* this doesn't belong here */
	if ((gp->g_mode & GFMT) == GFBLK)
		sb->st_blksize = BLKDEV_IOSIZE;
	else if ((gp->g_mode & GFMT) == GFCHR)
		sb->st_blksize = MAXBSIZE;
	else
		sb->st_blksize = FS(gp)->fs_bsize;
	sb->st_blocks = gp->g_blocks;
	sb->st_gennum = gp->g_gennum;
	sb->st_spare4 = 0;
	return (0);
}

struct fs_data *
ufs_getfsdata(mp)
	register struct mount *mp;
{
	register struct fs_data *fs_data = mp->m_fs_data;
	register struct fs *fs;
	
	fs = (struct fs *) mp->m_bufp->b_un.b_fs;
	fs_data->fd_gtot = fs->fs_ncg * fs->fs_ipg;
	fs_data->fd_gfree = fs->fs_cstotal.cs_nifree;
	fs_data->fd_btot = fs->fs_dsize * fs->fs_fsize / FSDUNIT;
	fs_data->fd_bfree = (fs->fs_cstotal.cs_nbfree * fs->fs_frag +
	fs->fs_cstotal.cs_nffree) * fs->fs_fsize / FSDUNIT;
	fs_data->fd_otsize = fs->fs_bsize;
	fs_data->fd_mtsize = MAXBSIZE;
	fs_data->fd_bfreen = freespace(fs, fs->fs_minfree) *
	fs->fs_fsize / FSDUNIT;
	fs_data->fd_dev = mp->m_dev;
	return(fs_data);
}

ufs_getdirent(gp, auio, cred)
	register struct gnode *gp;
	register struct uio *auio;
	register struct ucred *cred;
{

	if(access(gp, GREAD)) {	/* must be able to read the dir */
		u.u_error = EPERM;
		return;
	}

	u.u_error = ufs_rwgp(gp, auio, UIO_READ, 0, cred);

}

int
ufs_rlock(gp, ld, cmd, fp)
	struct gnode *gp;
	struct flock *ld;
	int cmd;
	struct file *fp;
{

	/*
	 *	There are two mechanisms by which ufs Sys-V locking
	 *	are supported.  The default support is kernel based
	 *	and the optional support is daemon based, and requires
	 *	that nfs is configured and that daemon based locking
	 *	has been enabled (via nfssetup).
	 */

	if (kernel_locking) {   /* kernel based locking */

		switch(cmd) {

			case F_GETLK:

				/* get region lock */

				if (u.u_error = getflck (fp, ld)) {
					break;
				}

				break;

			default:

				if (cmd == F_SETLK) {
					u.u_error = setflck (fp, ld, 0);
				} else { 
					u.u_error = setflck (fp, ld, 1);
				}

				break;

		} /* End switch */


	} else {
	
		/* daemon based locking */

		u.u_error = klm_drlock (gp, ld, cmd, fp->f_cred);
	}

	return (u.u_error); 

}
