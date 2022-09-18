#ifndef lint
static	char	*sccsid = "@(#)sys_generic.c	4.4	(ULTRIX)	3/7/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987,88 by			*
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
 *			Modification History				*
 *									*
 *
 * 28-Feb-91 -- prs
 *	Added support for a configurable number of
 *	open file descriptors.
 *
 * 16-Apr-90 -- jaw
 *	performance fixes for single cpu.
 *
 *  9 Mar 90 -- chet
 *	Hold file table entry locked in rwuio for files, directories,
 *	and block specials. This corrects problems with multiple
 *	readers getting the same data, and lost writes with multiple
 *	writers.
 *
 *  6 Mar 90 -- jaw						*
 *	ipl fix in select.
 *
 *	009 - Feb 28 89 - jaw						*
 *		move SSEL flag to own flag field.
 *
 *	008 - Jan 23 89 - jaw						*
 *		reduce contention on lk_select by reducing how		*
 *		long we hold it in selwakeup				*
 *	007 - Jul 28 88 - miche						*
 *		protect scheduling fields in proc structure		*
 * 	006 - Sept 11 86 - koehler					*
 *	 	gnode name change					*
 *									*
 * 	005 - Nov 11, 1985  - Depp					*
 *		Removed all conditional compiles for System V IPC.	*
 *									*
 *	004 - Sept 16 1985  - Reilly					*
 *		Modified code for the 4.3bsd namei code.		*
 *	001 - March 11 1985 - Larry Cohen				*
 *		modify select to handle more than 32 file descriptors	*
 *		- code taken from Berkeley				*
 *									*
 *	002 - April 4 1985 - Larry Cohen				*
 *		changes to support open block if in use			*
 *									*
 *	003 - April  9, 1985 - depp					*
 *		Added System V Named pipe support			*
 *									*
 *	004 - April 13, 1985 - Larry Cohen
 *		call f_ops ioctl for INUSE cmds.
 ************************************************************************/

/*	sys_generic.c	6.7	85/02/08	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/stat.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/kmalloc.h"

/*
 * Read system call.
 */
read()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = (caddr_t)uap->cbuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	rwuio(&auio, UIO_READ);
}

readv()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_READ);
}

/*
 * Write system call
 */
write()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		int	count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = uap->cbuf;
	aiov.iov_len = uap->count;
	rwuio(&auio, UIO_WRITE);
}

writev()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_WRITE);
}

rwuio(uio, rw)
	register struct uio *uio;
	enum uio_rw rw;
{
	struct a {
		int	fdes;
	};
	register struct file *fp;
	register struct iovec *iov;
	register struct gnode *gp;
	register int i, count;
	int locked = 0;


	GETF(fp, ((struct a *)u.u_ap)->fdes);
	if ((fp->f_flag&(rw==UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}

	gp = (struct gnode *)fp->f_data; /* only use with DTYPE_INODE! */

	uio->uio_resid = 0;
 	uio->uio_segflg = UIO_USERSPACE;	/*004 */
	iov = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	count = uio->uio_resid;

	/*
	 * This is read and write. The types of things that file
	 * descriptors reference are:
	 *
	 * 	DTYPE_INODE:
	 *		regular files (GFREG)
	 *		directories (GFDIR)
	 *		block special (GFBLK)
	 *		char special (GFCHR)
	 *		symlink (GFLNK)
	 *
	 *	DTYPE_SOCKET:
	 *		sockets, including UNIX-domain sockets (GFSOCK)
	 *
	 *	DTYPE_PORT:
	 *		named pipes (GFPORT)
	 *
	 *	DTYPE_PIPE:
	 *		plain old pipes (GFPIPE)
	 *
	 * We must serialize related processes on the file table entry
	 * for regular files, directories, and block special files.
	 * Not serializing will result in multiple processes getting
	 * the same uio_offset; this in turn leads to multiple readers
	 * getting the same data or lost writes.
	 *
	 * First check for DTYPE_INODE, then exclude char special since
	 * those reads can wait for completion indefinitely.
	 * Note that symlinks are not read/written directly, but if
	 * they were we'd probably include them too.
	 */
	if (fp->f_type == DTYPE_SOCKET) {
		uio->uio_offset = fp->f_offset;
		
		if (setjmp(&u.u_qsave)) {
        	        if (uio->uio_resid == count) {
        	                if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
					u.u_error = EINTR;
        	                else
        	                        u.u_eosys = RESTARTSYS;
        	        }
       	 	} else
                	u.u_error = (*fp->f_ops->fo_rw)(fp, rw, uio);
		u.u_r.r_val1 = count - uio->uio_resid;

		if (!smp) {
			fp->f_offset += u.u_r.r_val1;
			return;
		} else{
			smp_lock(&fp->f_lk, LK_RETRY);
			fp->f_offset += u.u_r.r_val1;
			smp_unlock(&fp->f_lk);
		}
		return;

	}
	if (fp->f_type == DTYPE_INODE && (gp->g_mode&GFMT) != GFCHR) {
		locked = 1;
		smp_lock(&fp->f_lk, LK_RETRY);
	}

	uio->uio_offset = fp->f_offset;

	if (setjmp(&u.u_qsave)) {
                if (uio->uio_resid == count) {
                        if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
				u.u_error = EINTR;
                        else
                                u.u_eosys = RESTARTSYS;
                }
        } else
                u.u_error = (*fp->f_ops->fo_rw)(fp, rw, uio);
	u.u_r.r_val1 = count - uio->uio_resid;

	if (fp->f_type == DTYPE_PORT  &&  u.u_error == 0){ /* 003 */
		register struct gnode *gp;

		gp = (struct gnode *)fp->f_data;
		gfs_lock(gp);
		if(rw == UIO_READ)
			gp->g_size -= u.u_r.r_val1;
		else
			gp->g_size += u.u_r.r_val1;
		gfs_unlock(gp);
	}

	if (!locked)
		smp_lock(&fp->f_lk, LK_RETRY);

	if (fp->f_type != DTYPE_PORT  && fp->f_type != DTYPE_PIPE)
		fp->f_offset += u.u_r.r_val1;

	smp_unlock(&fp->f_lk);
}

/*
 * Ioctl system call
 */
ioctl()
{
	register struct file *fp;
	struct a {
		int	fdes;
		int	cmd;
		caddr_t	cmarg;
	} *uap;
	register int com;
	register u_int size;
	register struct gnode *gp;
	char data[_IOCPARM_MASK+1];

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	if ((fp->f_flag & (FREAD|FWRITE)) == 0 || fp->f_data == (caddr_t)0) {
		u.u_error = EBADF;
		return;
	}
	com = uap->cmd;

#if defined(vax) && defined(COMPAT)
	/*
	 * Map old style ioctl's into new for the
	 * sake of backwards compatibility (sigh).
	 */
	if ((com&~0xffff) == 0) {
		com = mapioctl(com);
		if (com == 0) {
			u.u_error = EINVAL;
			return;
		}
	}
#endif
	if (com == FIOCLEX) {
		U_POFILE_SET(uap->fdes, U_POFILE(uap->fdes) | UF_EXCLOSE);
		return;
	}
	if (com == FIONCLEX) {
		U_POFILE_SET(uap->fdes, U_POFILE(uap->fdes) & ~UF_EXCLOSE);
		return;
	}

	/*
	 * Interpret high order word to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
	size = (com &~ (_IOC_INOUT|_IOC_VOID)) >> 16;
	if (size > sizeof (data)) {
		u.u_error = EFAULT;
		return;
	}
	if (com&_IOC_IN) {
		if (size) {
			u.u_error =
			    copyin(uap->cmarg, (caddr_t)data, (u_int)size);
			if (u.u_error)
				return;
		} else
			*(caddr_t *)data = uap->cmarg;
	} else if ((com&_IOC_OUT) && size)
		/*
		 * Zero the buffer on the stack so the user
		 * always gets back something deterministic.
		 */
		bzero((caddr_t)data, size);
	else if (com&_IOC_VOID)
		*(caddr_t *)data = uap->cmarg;

	switch (com) {

	case FIONBIO:
	        smp_lock(&fp->f_lk, LK_RETRY);
		u.u_error = fset(fp, FNDELAY, *(int *)data);
		smp_unlock(&fp->f_lk);
		return;

	case FIOASYNC:
		smp_lock(&fp->f_lk, LK_RETRY);
		u.u_error = fset(fp, FASYNC, *(int *)data);
		smp_unlock(&fp->f_lk);
		return;

	case FIOSETOWN:
		smp_lock(&fp->f_lk, LK_RETRY);
		u.u_error = fsetown(fp, *(int *)data);
		smp_unlock(&fp->f_lk);
		return;

	case FIOGETOWN:
		smp_lock(&fp->f_lk, LK_RETRY);
		u.u_error = fgetown(fp, (int *)data);
		smp_unlock(&fp->f_lk);
		return;
	case FIOSINUSE: 			/*002*/
		gp = (struct gnode *)fp->f_data;
		if (gp->g_flag & GINUSE) {
			u.u_error = EALREADY;
			return;
		}
		else {
			U_POFILE_SET(uap->fdes, U_POFILE(uap->fdes) | UF_INUSE);
			gfs_lock(gp);
			gp->g_flag |= GINUSE;
			gfs_unlock(gp);
		}
		break;
	case FIOCINUSE:
		gp = (struct gnode *)fp->f_data;
		gfs_lock(gp);
		gp->g_flag &= ~(GINUSE);
		gfs_unlock(gp);
		U_POFILE_SET(uap->fdes, U_POFILE(uap->fdes) & ~(UF_INUSE));
		wakeup((caddr_t)&gp->g_flag);
		break;
	}
	u.u_error = (*fp->f_ops->fo_ioctl)(fp, com, data);
	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (u.u_error == 0 && (com&_IOC_OUT) && size)
		u.u_error = copyout(data, uap->cmarg, (u_int)size);
}

int	unselect();
int	nselcoll;

/*
 * Select system call.
 */
select()
{
	register struct uap  {
		int	nd;
		fd_set	*in, *ou, *ex;
		struct	timeval *tv;
	} *uap = (struct uap *)u.u_ap;
	fd_mask *ibits, *obits;
	fd_mask *fd_maskp;
	fd_mask input_fd_set[3 * (howmany(NOFILE_IN_U, NFDBITS))];
	fd_mask output_fd_set[3 * (howmany(NOFILE_IN_U, NFDBITS))];
	struct timeval atv;
	int  ncoll, ni;
	label_t lqsave;
	int x;
	int error = 0;

	if (uap->nd > (u.u_omax + 1))
		uap->nd = u.u_omax + 1;	/* forgiving, if slightly wrong */

	ni = howmany(uap->nd, NFDBITS);

	/*
	 * Lets use stack space for callers of select() with <= 64
	 * file descriptors.
	 */
	if (uap->nd <= NOFILE_IN_U) {
		ibits = input_fd_set;
		obits = output_fd_set;
		bzero(ibits, sizeof(input_fd_set));
		bzero(obits, sizeof(output_fd_set));
	} else {
		KM_ALLOC(ibits, fd_mask *, 3 * (ni * sizeof (fd_mask)),
			 KM_NOFILE, KM_CLEAR);
		KM_ALLOC(obits, fd_mask *, 3 * (ni * sizeof (fd_mask)),
			 KM_NOFILE, KM_CLEAR);
	}

	/*
	 * Don't copy in the entire fd_set structure for
	 * readable, writeable and exceptioned file descriptors.
	 * Instead calculate the number of fd_masks we are selecting,
	 * and just copy in that amount. Then we just have to manipulate
	 * these variable sized arrays of fd_masks.
	 */

	for (x = 0, fd_maskp = ibits; x < 3; x++) {
		switch(x) {
		      case 0:
			if (uap->in)
				u.u_error = copyin((caddr_t)uap->in,
						   (caddr_t)fd_maskp,
						   (unsigned)(ni *
						   sizeof(fd_mask)));
			break;
		      case 1:
			if (uap->ou)
				u.u_error = copyin((caddr_t)uap->ou,
						   (caddr_t)fd_maskp,
						   (unsigned)(ni * 
						   sizeof(fd_mask)));
			break;
		      case 2:
			if (uap->ex)
				u.u_error = copyin((caddr_t)uap->ex,
						   (caddr_t)fd_maskp,
						   (unsigned)(ni * 
						   sizeof(fd_mask)));
			break;
		}
		fd_maskp += ni;
		if (u.u_error)
			goto done;
	}
	if (uap->tv) {
		u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
			sizeof (atv));
		if (u.u_error)
			goto done;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			goto done;
		}
		splhigh(); timevaladd(&atv, &time); spl0();
	}
retry:
if (smp) {
	spl6();
	smp_lock(&lk_select, LK_RETRY);
retry2:
	ncoll = nselcoll;
	u.u_procp->p_select |= SSEL;
	smp_unlock(&lk_select);
	spl0();
} else {
retry3:
	ncoll = nselcoll;
	u.u_procp->p_select = SSEL;
}
	u.u_r.r_val1 = selscan(ibits, obits, uap->nd);
	if (u.u_error || u.u_r.r_val1)
		goto done;

	(void)spl6();
	/* this should be timercmp(&time, &atv, >=) */
	if (uap->tv && (time.tv_sec > atv.tv_sec ||
	    time.tv_sec == atv.tv_sec && time.tv_usec >= atv.tv_usec)) {
	    	spl0();
		goto done;
	}
	smp_lock(&lk_select,LK_RETRY);
	if ((u.u_procp->p_select) == 0 || nselcoll != ncoll) {
		if (smp) goto retry2;
		else goto retry3;
	}
	u.u_procp->p_select = 0;

	if (uap->tv) {
		lqsave = u.u_qsave;
		if (setjmp(&u.u_qsave)) {
			untimeout(unselect, (caddr_t)u.u_procp);
			u.u_error = EINTR;
			spl0();
			goto done;
		}
		timeout(unselect, (caddr_t)u.u_procp, hzto(&atv));
	}
	if (smp) sleep_unlock((caddr_t)&selwait, PZERO+1,&lk_select);
	else  sleep_unlock((caddr_t)&selwait, PZERO+1,0);
	if (uap->tv) {
		u.u_qsave = lqsave;
		untimeout(unselect, (caddr_t)u.u_procp);
	}
	spl0();
	goto retry;
done:
	if (u.u_error)
		goto out;
	for (x = 0, fd_maskp = obits; x < 3; x++) {
		switch(x) {
		      case 0:
			if (uap->in)
				error = copyout((caddr_t)fd_maskp,
						(caddr_t)uap->in,
						(unsigned)(ni * 
						sizeof(fd_mask)));
			break;
		      case 1:
			if (uap->ou)
				error = copyout((caddr_t)fd_maskp,
						(caddr_t)uap->ou,
						(unsigned)(ni * 
						sizeof(fd_mask)));
			break;
		      case 2:
			if (uap->ex)
				error = copyout((caddr_t)fd_maskp,
						(caddr_t)uap->ex,
						(unsigned)(ni * 
						sizeof(fd_mask)));
			break;
		}
		fd_maskp += ni;
		if (error)
			u.u_error = error;
	}
out:
	if (uap->nd > NOFILE_IN_U) {
		KM_FREE(ibits, KM_NOFILE);
		KM_FREE(obits, KM_NOFILE);
	}
}
unselect(p)
	register struct proc *p;
{
	register int s = spl6();
if (smp) {
	smp_lock(&lk_rq,LK_RETRY);
	switch (p->p_stat) {

	case SSLEEP:
		setrun(p);
		break;

	case SSTOP:
		unsleep(p);
		break;
	}
	smp_unlock(&lk_rq);
	splx(s);
} else {

	switch (p->p_stat) {

	case SSLEEP:
		setrun(p);
		break;

	case SSTOP:
		unsleep(p);
		break;
	}
	splx(s);


}

}

/*
 * Don't pass down pointers to fd_set structures, because they are large
 * and wastefull. Instead, pass in variable sized arrays of fd_mask 
 * structures sized by the number of file descriptors we are scanning. 
 * A visual aid:
 *	1 -  32 nfd ---> ibits&obits point to arrays of 3 fd_masks in length
 *	33 - 64 nfd ---> ibits&obits point to arrays of 6 fd_masks in length
 *	65 - 96 nfd ---> ibits&obits point to arrays of 9 fd_masks in length
 */

selscan(ibits, obits, nfd)
	fd_mask *ibits, *obits;
{
	register int which, i, j;
	register fd_mask bits;
	int flag;
	struct file *fp;
	int n = 0;

	for (which = 0; which < 3; which++) {
		switch (which) {

		case 0:
			flag = FREAD;
			break;

		case 1:
			flag = FWRITE;
			break;

		case 2:
			flag = 0;
			break;
		}
		for (i = 0; i < nfd; i += NFDBITS) {
			bits = *ibits;
			while ((j = ffs(bits)) && i + --j < nfd) {
				register int fd;

				bits &= ~(1 << j);
				fd = i + j;
				if ((fd > u.u_omax) || 
				    ((fp = U_OFILE(fd)) == NULL)) {
					u.u_error = EBADF;
					break;
				}
				if ((*fp->f_ops->fo_select)(fp, flag)) {
					*obits |= (1 << (fd % NFDBITS));
					n++;
				}
			}
			ibits++;
			obits++;
		}
	}
	return (n);
}

/*ARGSUSED*/
seltrue(dev, flag)
	dev_t dev;
	int flag;
{

	return (1);
}

selwakeup(p, coll)
	register struct proc *p;
	int coll;
{
	register int s;
	int dowakeup = 0;

if (smp) {
	s = spl6();
	smp_lock(&lk_select,LK_RETRY);
	if (coll) {
		nselcoll++;
		dowakeup = 1;
	}
	if (p) {
		if (p->p_select & SSEL) {
			p->p_select &= ~SSEL;
		} else {
			smp_lock(&lk_rq,LK_RETRY);
			if (p->p_wchan == (caddr_t)&selwait) {
				if (p->p_stat == SSLEEP)
					setrun(p);
				else
					unsleep(p);
			} 
			smp_unlock(&lk_rq);
		} 
	} 
	smp_unlock(&lk_select);
	splx(s);
	if (dowakeup)	wakeup((caddr_t)&selwait);
} else {

	if (coll) {
		nselcoll++;
		wakeup((caddr_t)&selwait);
	}
	if (p) {
		s = splhigh();
		if (p->p_wchan == (caddr_t)&selwait) {
			if (p->p_stat == SSLEEP)
				setrun(p);
			else
				unsleep(p);
		} else if (p->p_select)
			p->p_select = 0;
		splx(s);
	}






}

}
