#ifndef lint
static	char	*sccsid = "@(#)gfs_gnodeops.c	4.5	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88,89 by			*
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
 * 28 Feb 91 -- prs
 *	Added support for a configurable number of open
 *	file descriptors.
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping
 *
 * 21 Jan 91 -- prs
 *	Added routine divorce_dev() to disassociate gnode from file
 *	table entry. The gnode pointer is no longer zeroed, it will
 *	simply stay around until the file table entry goes away, but
 *	any I/O requests with the file table entry will result in an
 *	error return. Once divorce_dev runs, the physical device will
 *	not be called with any open file descriptors. vhangup() and
 *	gno_close() call divirce_dev().
 *
 * 06 Mar 90 -- scott
 *	allow negative uio_offset for AUD_NO
 *
 *  9 Feb 90 -- prs
 * 	Fixed a bug in gno_lock() where a branch to `again' didn't
 *	unlock the gp.
 *
 * 12 Jan 90 -- prs
 *	Changed gno_close() to not check pipes or fifos for file
 *	table references before calling sfs close routine.
 *
 * 14 Nov 89 -- prs
 *	Removed setting u_error to zero from gno_close().
 *
 * 09 nov 89 -- fran
 *	Add logic to forceclose to break connection to controlling
 *	terminal.
 *
 * 25 Jul 89 -- chet
 *	Use new bflush() interface in gno_close()
 *
 * 14 Jun 89 -- prs
 *	Enhanced vhangup() logic to interoperate with closef().
 *
 * 09 Feb 89 -- prs
 *	Initialized the stack variable `type' in rwgp(). This fixes the
 *	problem with the routine failing to check if a write would exceed
 *	the current rlimit for the user.
 *
 * 04 Jan 89 -- prs
 *	Removed the DTYPE_PORT check in gno_close(). Skipping named pipes
 *	allows the code to only check special devices for multiple file 
 *	table references for the same device. This fixes a bug where the 
 *	fifo close routine would only be called on the last reference to 
 *	the fifo, instead of every call propagating to the sfs close.
 *
 * 28 Sep 88 -- chet
 *      Add cacheinvalall in unlikely event that nextgnodeid wraps.
 *
 * 9  Sep 88 -- condylis
 *	Added SMP changes to gno_lock and gno_unlock
 *
 * 19 Jul 88 -- prs
 *	Cleared g_fifo field in getegnode() routine, to fix a problem
 *	with fifo gnode reclaiming a non fifo gnode over nfs.
 *
 * 11 Jul 88 -- prs
 *	Modified gno_ioctl to pass the flags filed of a file pointer
 *	 to the fifo ioctl routine.
 *
 * 8  Jul 88 -- condylis
 *	All gnode types except regular file and directory will now call
 *	rwgp from gno_rw without the gnode lock held.
 *	Test added to end of rwgp to insure that gnode lock is held
 *	while setting GACC bit in gnode flag.
 *
 * 19 May 88 -- cb
 *	Changed GFS interface - SMP safe gnode manipulation.
 *
 * 15 Feb 88 -- Tim Burke
 *	Added field to uio structure called uio_flag. This field will be used 
 *	pass the file descriptor modes down into the device driver on reads
 *	and writes to distinguish POSIX nonblocking I/O/
 *
 * 10 Feb 88 -- prs
 *	Modified to handle the new fifo code.
 *
 * 10 Feb 88 -- map
 *	Don't send SIGXFSZ if in System V mode.
 *
 * 08 Feb 88 -- prs
 *	Changed getegnode to process g_dquot field in gnode.
 *	Now, gnode returned has this field nulled out, as routine
 *	name suggests.
 *
 * 12 Jan 88 -- Fred Glover
 *	Add gno_lockrelease routine for Sys-V lock cleanup upon file
 *	close or process termination
 *
 * 28 Dec 87 -- Tim Burke
 *  	Moved u.u_ttyp to u.u_procp->p_ttyp.
 *
 * 23 Mar 87 -- prs
 *	Fixed gno_close to decrement reference count in gnode
 *	before returning from special cases.
 *
 * 03 Mar 87 -- chase
 *	refresh gnode attributes before an append mode write
 *
 * 03 Mar 87 -- prs
 *	Changed gno_close to verify block device was not mounted before
 *	flushing and invalidating all its buffers.
 *
 * 29 Jan 87 -- chet
 *	moved gput() after GCLOSE in gno_close().
 *
 * 19 Sep 86 -- lp
 *	fixed bug in ioctl code when enabling n-buff.
 * 11 Sep 86 -- koehler
 *	fixed close routine
 *
 ***********************************************************************/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/cmap.h"
#include "../h/stat.h"
#include "../h/kernel.h"
#include "../h/exec.h"
#include "../h/cpudata.h"
#ifdef QUOTA
#include "../h/quota.h"
#endif


int	gno_rw(), gno_ioctl(), gno_select(), gno_close();
struct 	fileops gnodeops =
	{ gno_rw, gno_ioctl, gno_select, gno_close };

int 	divorce_rw(), divorce_ioctl(), divorce_select(), divorce_close();
struct	fileops divorceops =
	{ divorce_rw, divorce_ioctl, divorce_select, divorce_close };

gno_rw(fp, rw, uio)
	register struct file *fp;
	register enum uio_rw rw;
	register struct uio *uio;
{
	register struct gnode *gp = (struct gnode *)fp->f_data;
	register int error;
	
	if ((gp->g_mode&GFMT) == GFREG) {
		gfs_lock(gp);
		if (fp->f_flag&FAPPEND && rw == UIO_WRITE) {
			(void)GGETVAL(gp);
			uio->uio_offset = fp->f_offset = gp->g_size;
		}

		/*
		 * If synchronous write flag was passed in the file
		 * pointer from the open or fcntl system calls,
		 * then do a synchronous write.
		 */

		if (fp->f_flag & O_FSYNC)
			error = rwgp(gp, uio, rw, IO_SYNC, fp->f_cred);
		else
			error = rwgp(gp, uio, rw, IO_ASYNC, fp->f_cred);

		gfs_unlock(gp);			
	} else {
		uio->uio_flag = fp->f_flag;
		if ((fp->f_flag & FNBLOCK) || (fp->f_flag & FNDELAY))
			error = rwgp(gp, uio, rw, IO_ASYNC|FNDELAY, fp->f_cred);
		else
			error = rwgp(gp, uio, rw, IO_ASYNC, fp->f_cred);
	}

	return (error);
}

rdwri(rw, gp, base, len, offset, segflg, aresid)
	register struct gnode *gp;
	register caddr_t base;
	int len, offset, segflg;
	register int *aresid;
	enum uio_rw rw;
{
	struct uio _auio;
	register struct uio *auio = &_auio;
	struct iovec _aiov;
	register struct iovec *aiov = &_aiov;
	register int error;

	auio->uio_iov = aiov;
	auio->uio_iovcnt = 1;
	aiov->iov_base = base;
	aiov->iov_len = len;
	auio->uio_resid = len;
	auio->uio_offset = offset;
	auio->uio_segflg = segflg;
	error = rwgp(gp, auio, rw, IO_ASYNC, u.u_cred);
	if (aresid)
		*aresid = auio->uio_resid;
	else
		if (auio->uio_resid)
			error = EIO;
	return (error);
}

rwgp(gp, uio, rw, ioflag, cred)
	register struct gnode *gp;
	register struct uio *uio;
	register enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	dev_t dev = (dev_t)gp->g_rdev;
	register int type = (gp->g_mode & GFMT);
	extern int mem_no;
	register int ret;
	
	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwgp");
	if (rw == UIO_READ && uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0 &&
	    (type != GFCHR || (mem_no != major(dev) && major(dev) != AUD_NO)))
		return (EINVAL);
	
	
	if (rw == UIO_WRITE && type == GFREG && uio->uio_offset +
	uio->uio_resid > u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		if (u.u_procp->p_progenv == A_BSD) /* BSD baggage */
			psignal(u.u_procp, SIGXFSZ); 
		return (EFBIG);
	}
	
	ret = GRWGP(gp, uio, rw, ioflag, cred);
	if (!ret && rw == UIO_READ) {
		if (ISLOCAL(gp->g_mp)) {
			if (smp_owner(&gp->g_lk))
				gp->g_flag |= GACC;
			else {
				gfs_lock(gp);
				gp->g_flag |= GACC;
				gfs_unlock(gp);
			}
		}
	}
	return(ret);
}


gno_ioctl(fp, com, data, cred)
	register struct file *fp;
	register int com;
	register caddr_t data;
	struct ucred *cred;
{
 	register struct gnode *gp = ((struct gnode *)fp->f_data);
	register int fmt = gp->g_mode & GFMT;
	dev_t dev;
	register int flag;
	
	switch (fmt) {
		case GFREG:
		case GFDIR:
			if (com == FIONREAD) 
				flag = fp->f_offset;
			break;
		case GFPIPE:
		case GFPORT:
			flag = fp->f_flag;
			break;
		case GFCHR:
			flag = fp->f_flag;
			dev = gp->g_rdev;
		       /*
			* cdevsw[].d_strat implies this device can
			*  do multi-buffered operations. Otherwise
		 	*  just fall into device ioctl.
			*/
			if (cdevsw[major(dev)].d_strat) {
				if (com == FIONBUF) {
					int *acount = (int *)data;
					if (*acount < 0)
						return(ENXIO);
				   	if (*acount > 0) {
						fp->f_flag |= FNBUF;
						gp->g_flag |= ASYNC;
						startasync(dev, acount,
						fp->f_flag);
				   	} else if (fp->f_flag&FNBUF) {
						if (!asyncclose(dev, fp->f_flag))
							gp->g_flag &= ~ASYNC;
						fp->f_flag &= ~FNBUF;
				   	}
			 	  	return(0);
				}
				if (com == FIONBDONE)
					return(aiodone(dev, *(int *)data,
					fp->f_flag));
				if (com == FIONBIO || com == FIOASYNC)
						return(0); 
			} else if (com == FIONBUF)
				return(ENXIO);

	}

	if ((flag = GFNCTL(gp, com, data, flag, fp->f_cred)) == GNOFUNC)
		flag = EOPNOTSUPP;
	return(flag);
}

gno_select(fp, which)
	register struct file *fp;
	register int which;
{
	register struct gnode *gp = (struct gnode *)fp->f_data;

	return (GSELECT(gp, which, fp->f_cred));
}

#ifdef notdef
gno_clone()
{

	return (EOPNOTSUPP);
}
#endif

gno_stat(gp, sb)
	register struct gnode *gp;
	register struct stat *sb;
{

	return (GSTAT(gp, sb));
}

gno_close(fp)
	register struct file *fp;
{
	register struct file *ffp;
	register struct gnode *gp = (struct gnode *)fp->f_data;
	register int flag;
	dev_t dev;
	register int mode;	
	register struct gnode *tgp;
	register struct mount *mp;
	
	flag = fp->f_flag;
	if (flag & (FSHLOCK|FEXLOCK))
		gno_unlock(fp, FSHLOCK|FEXLOCK);
	fp->f_ops = &divorceops; /* So we wont be looked at ! */
	dev = (dev_t)gp->g_rdev;
	mode = gp->g_mode & GFMT;
	if (mode == GFCHR || mode == GFBLK) {
		/*
		 * Check for references to the device. If the device is
		 * referenced by another process, simply remove our
		 * reference to the gnode. However, if we are the last
		 * reference to the device, call spec_close() which will
		 * call the drivers close routine, shutting down the device.
		 */
		for (ffp = file; ffp < fileNFILE; ffp++) {
			if (ffp->f_count == 0)
				continue;
			if (ffp->f_type != DTYPE_INODE)		/* XXX */
				continue;
			if (ffp->f_ops == &divorceops)
				continue;
			if ((tgp = (struct gnode *)ffp->f_data) &&
			    tgp->g_rdev == dev && (tgp->g_mode&GFMT) == mode) {
				/*
				 * Decrement ref count in gnode
				 */
				grele(gp);
				goto done;
			}
		}
		if (mode == GFBLK) {
			/*
			 * On last close of a block device (that isn't mounted)
			 * we must invalidate any in core blocks, so that
			 * we can, for instance, change floppy disks.
			 */
			GETMP(mp, dev);
			/*
			 * If GETMP returns a mp, then the block device
			 * is mounted on, so return.
			 */
			if (mp != NULL) {
				/*
				 * Decrement ref count in gnode
				 */
				grele(gp);
				goto done;
			}
			bflush(dev, (struct gnode *) 0, 0);
			binval(dev, (struct gnode *) 0);
		}
		if (setjmp(&u.u_qsave)) {
			/*
			 * If device close routine is interrupted,
			 * must return so closef can clean up.
			 */
			 if (u.u_error == 0)
			 	u.u_error = EINTR;	/* ??? */
			 goto done;
		}
	}
	if (GCLOSE(gp, flag) == GNOFUNC)
		u.u_error = EOPNOTSUPP;

	if (mode == GFBLK || mode == GFCHR)
		divorce_dev(gp->g_rdev, mode);
	gfs_lock(gp);
	gput(gp);
done:
	return(u.u_error);

}

/*
 * Place an advisory lock on an inode.
 */
gno_lock(fp, cmd)
	register struct file *fp;
	register int cmd;
{
	register int priority = PLOCK;
	register struct gnode *gp = (struct gnode *)fp->f_data;

	if ((cmd & LOCK_EX) == 0)
		priority++;
	if (setjmp(&u.u_qsave)) {
                if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
                        return(EINTR);
		u.u_eosys = RESTARTSYS;
                return (0);
        }
	/*
	 * If there's a exclusive lock currently applied
	 * to the file, then we've gotta wait for the
	 * lock with everyone else.
	 */

again:
	/* SMP lock gnode during locking operation */
	gfs_lock(gp);
	while (gp->g_flag & GEXLOCK) {
		/*
		 * If we're holding an exclusive
		 * lock, then release it.
		 */
		if (fp->f_flag & FEXLOCK) {
			gno_unlock(fp, FEXLOCK);
			continue;
		}
		if (cmd & LOCK_NB) {
			gfs_unlock(gp);
                        return (EWOULDBLOCK);
		}
		gp->g_flag |= GLWAIT;
		sleep_unlock((caddr_t)&gp->g_exlockc, priority, &gp->g_lk);
		gfs_lock(gp);
	}
	if (cmd & LOCK_EX && (gp->g_flag & GSHLOCK)) {
		/*
		 * Must wait for any shared locks to finish
		 * before we try to apply a exclusive lock.
		 *
                 * If we're holding a shared
                 * lock, then release it.
		 */
		if (fp->f_flag & FSHLOCK) {
			gno_unlock(fp, FSHLOCK);
			gfs_unlock(gp);
			goto again;
		}
		if (cmd & LOCK_NB) {
			gfs_unlock(gp);
                        return (EWOULDBLOCK);
		}
		gp->g_flag |= GLWAIT;
		sleep_unlock((caddr_t)&gp->g_shlockc, PLOCK, &gp->g_lk);
		goto again;
	}
	if (fp->f_flag & FEXLOCK)
		panic("gno_lock");
	if (cmd & LOCK_EX) {
		cmd &= ~LOCK_SH;
		gp->g_exlockc++;
		gp->g_flag |= GEXLOCK;
		fp->f_flag |= FEXLOCK;
	}
	if ((cmd & LOCK_SH) && (fp->f_flag & FSHLOCK) == 0) {
		gp->g_shlockc++;
		gp->g_flag |= GSHLOCK;
		fp->f_flag |= FSHLOCK;
	}
	gfs_unlock(gp);
	return (0);
}

/*
 * Unlock a file.
 */
gno_unlock(fp, kind)
	register struct file *fp;
	register int kind;
{
	register struct gnode *gp = (struct gnode *)fp->f_data;
	register int flags;
	register int unlck = 0;

	kind &= fp->f_flag;
	if (gp == NULL || kind == 0)
		return;
	/* Will be true when gno_unlock is called from flock()	*/
	if (!smp_owner(&gp->g_lk)) {
		unlck = 1;
		gfs_lock(gp);
	}
	flags = gp->g_flag;
	if (kind & FSHLOCK) {
		if ((flags & GSHLOCK) == 0)
			panic("gno_unlock: SHLOCK");
		if (--gp->g_shlockc == 0) {
			gp->g_flag &= ~GSHLOCK;
			if (flags & GLWAIT)
				wakeup((caddr_t)&gp->g_shlockc);
		}
		fp->f_flag &= ~FSHLOCK;
	}
	if (kind & FEXLOCK) {
		if ((flags & GEXLOCK) == 0)
			panic("gno_unlock: EXLOCK");
		if (--gp->g_exlockc == 0) {
			gp->g_flag &= ~(GEXLOCK|GLWAIT);
			if (flags & GLWAIT)
				wakeup((caddr_t)&gp->g_exlockc);
		}
		fp->f_flag &= ~FEXLOCK;
	}
	if (unlck)
		gfs_unlock(gp);
}


/*
 * Revoke access the current tty by all processes.
 * Used only by the super-user in init
 * to give ``clean'' terminals at login.
 */
vhangup()
{
	struct tty *tp;

	if (!suser())
		return;
	if ((tp = u.u_procp->p_ttyp) == NULL)
		return;
	forceclose(u.u_ttyd);
	if ((tp->t_state) & TS_ISOPEN)
		gsignal(tp->t_pgrp, SIGHUP);
}


/*
 * vhangup() is called when a terminal line is to be cleared of any
 * references by any other process when a login is attempted. This
 * handles the case where a terminal line is open and a login occured
 * on the same line, any write (or read or select or....) will be
 * handled by the divorceops. 
 */

#define	D_TO_TTY(dev) \
		(cdevsw[major((dev))].d_ttys \
		? (struct tty *)&cdevsw[major(dev)].d_ttys[minor(dev)] \
		: (struct tty *)0)

forceclose(dev)
	dev_t dev;
{
	struct tty *tp = D_TO_TTY(dev);
	int saveaffinity;
	int flag;

	if (flag = divorce_dev(dev, GFCHR)) {
		CALL_TO_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
		(*cdevsw[major(dev)].d_close) (dev, flag);
		RETURN_FROM_NONSMP_DRIVER(cdevsw[major(dev)], saveaffinity);
	}
	/*
	 *  zap ctltty so /dev/tty connection is broken
	 */
	if (tp) {
	    FORALLPROC(
		if (pp->p_ttyp == tp && proc_get(pp->p_pid) == pp) {
		    if (pp->p_ttyp == tp)	/* change during proc_get? */
			pp->p_ttyp = (struct tty *)0;
		    proc_rele(pp);
	    }
		       )
	}
	/*
	 * Since a call to the drivers close routine can take a real
	 * long time, lets do it again for any file table enties that
	 * were being opened while we were blocked closing the device.
	 */
	divorce_dev(dev, GFCHR);
}

/*
 * divorce_dev() will walk through the file table. setting the file ops
 * vector array to divorce_ops. Any further requests with the file
 * descriptor will result in an error. When the file descriptor is closed,
 * the gnode reference count will be decremented as always.
 */

divorce_dev(dev, mode)
	dev_t dev;
	int mode;
{
	register struct file *fp;
	register struct gnode *gp;
	register caddr_t value;
	int flag = 0;

top:
	/*
	 * We hold the file table througout the search because
	 * f_data is now protected with it (if f_type == DTYPE_INODE).
	 */
	smp_lock(&lk_file, LK_RETRY);
	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_count == 0)
			continue;
		if (fp->f_type != DTYPE_INODE)
			continue;
		if (fp->f_ops == &divorceops)
			continue;
		gp = (struct gnode *)fp->f_data;
		if (gp == 0)
			continue;
		if ((gp->g_mode & GFMT) != mode)
			continue;
		if (gp->g_rdev != dev)
			continue;
		if (gp->g_flag & GINUSE) {
			smp_unlock(&lk_file);
			(*fp->f_ops->fo_ioctl)(fp, FIOCINUSE, value, 
					       fp->f_cred);
			wakeup((caddr_t)&gp->g_flag);
			gfs_lock(gp);
			gp->g_flag &= ~(GINUSE);
			gfs_unlock(gp);
			goto top;
		}
		fp->f_ops = &divorceops;
		flag = fp->f_flag;
	}
	smp_unlock(&lk_file);
	return(flag);
}

divorce_rw(fp, rw, uio)
	register struct file *fp;
	register enum uio_rw rw;
	register struct uio *uio;
{
	return(EBADF);
}


divorce_ioctl(fp, com, data)
	register struct file *fp;
	register int com;
	register caddr_t data;
{
	return(EBADF);
}


divorce_select(fp, which)
	register struct file *fp;
	register int which;
{
	return(EBADF);
}

divorce_close(fp)
	register struct file *fp;
{
	register struct gnode *gp = (struct gnode *)fp->f_data;

	/*
	 * We don't have to hold the file table lock here,
	 * because nobody cares about file table entries with
	 * divorceops.
	 */
	gfs_lock(gp);
	gput(gp);
	return(0);
}

/*
 * Remove a gnode from the gnode cache (make the gnode active )
 */

void
gremque(gp)
	register struct gnode *gp;
{
	register struct gnode *gq;

	gq = gp->g_freef;
	*(gp->g_freeb) = gq;
	if (gq)
		gq->g_freeb = gp->g_freeb;
	else
		gfreet = gp->g_freeb;
	gp->g_freef = NULL;
	gp->g_freeb = NULL;
}

/*
 * This routine is called for every file close (excluding kernel processes
 * that call closef() directly) in order to implement the
 * SVID 'feature' that the FIRST close of a descriptor that refers to
 * a locked object causes all the locks to be released for that object.
 * It is called, for example, by close(), exit(), exec(), & dup2().
 *
 * NOTE: If the SVID ever changes to hold locks until the LAST close,
 *       then this routine might be moved to closef().
 *
 */
int
gno_lockrelease(fp)
	register struct file *fp;
{
	/*
	 * Only do extra work if the process has done record-locking.
	 */

	if (u.u_procp->p_file & SLKDONE) {
		register struct gnode *gp;
		register struct file *ufp;
		register int i;
		register int locked;
		struct flock ld;

		locked = 0;	
		u.u_procp->p_file &= ~SLKDONE;	/* reset process flag */
		gp = (struct gnode *)fp->f_data;

		/*
		 * Check all open files to see if there's a lock
		 * possibly held for this gnode.
		 */

		for (i = u.u_omax; i-- > 0; ) {
			if (((ufp = U_OFILE(i)) != NULL) &&
			    (U_POFILE(i) & UF_FDLOCK)) {

				/* the current file has an active lock */
				if ((struct gnode *)ufp->f_data == gp) {

					/* release this lock */
					locked = 1;	/* (later) */
					U_POFILE_SET(i, 
						     U_POFILE(i) & ~UF_FDLOCK);
				} else {

					/* another file is locked */
					u.u_procp->p_file |= SLKDONE;
				}
			}
		}	/* for all files */

		/*
		 * If 'locked' is set, release any locks that this process
		 * is holding on this file.  If record-locking on any other
		 * files was detected, the process was marked (SLKDONE) to
		 * run thru this loop again at the next file close.
		 */

		 if (locked) {
			ld.l_type = F_UNLCK;	/* set to unlock entire file */
			ld.l_whence = 0;	/* unlock from start of file */
			ld.l_start = 0;
			ld.l_len = 0;		/* do entire file */
			return (GRLOCK(gp, &ld, F_SETLK, fp));
		 }
	}
	return (0);
}
