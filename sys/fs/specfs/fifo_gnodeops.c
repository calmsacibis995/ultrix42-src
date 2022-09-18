#ifndef lint
static	char	*sccsid = "@(#)fifo_gnodeops.c	4.2	(ULTRIX)	2/28/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988,89 by				*
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

/************************************************************************
 *			Modification History
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping
 *
 * 15-Feb-90 -- prs
 *	Added an smp primitive workaround to fifo_rele().
 *
 * 12-Jan-90 -- prs
 *	Modified to use internal ref counters for allocation and
 *	deallocation of fifo structures.
 *
 * 24-Jul-89 -- prs
 *	Fixed setjmp case in fifo_open().
 *
 * 04-Jan-89 -- prs
 *	Fixed an ordering bug in fifo_rele that attempted to unlock
 *	a gnode after its memory segment was released.
 *
 * 28-Sep-88 -- jaw
 *	add locks to fifo_select
 *
 * 14-Sep-88 -- prs
 *	Added fifo_gupdat() and support to update time fields for
 *	fifo gnodes. This change was required by POSIX.
 *
 * 06-Sep-88 -- prs
 *	SMP changes to fifo_open(), fifo_close() and fifo_rele().
 *
 * 11-Jul-88 -- prs
 *	Added FIOGETOWN, FIOSETOWN, and FIOASYNC cases to fifo_ioctl().
 *	Added the routine fifo_wakeup() to send a SIGIO signal,
 *	when ASYNC is enabled on a fifo. Also, added calls to fifo_wakeup
 *	from fifo_close(), when either side of a pipe is shutting down.
 *
 * 27-Apr-88 -- prs
 *	Fixed fifo_select code.
 *
 * 08-Apr-88 -- prs
 *	Fixed fifo_stat to bzero stat structure if type is GFPIPE.
 *
 * 02-Mar-88 -- prs
 *      Fixed fifo_lock and fifo_unlock routines to handle locking
 *      a GFPIPE.
 *
 * 10-Feb-88 -- prs
 *	New fifo code.
 *
 ************************************************************************/

#include "fifonode.h"
#include "fifo.h"
#include "../net/rpc/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/kernel.h"
#include "../h/gnode_common.h"
#include "../h/gnode.h"
#include "../h/ioctl.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/devio.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/cmap.h"
#include "../h/exec.h"

/* SystemV-compatible FIFO implementation */

static struct fifo_bufhdr *fifo_bufalloc();
static struct fifo_bufhdr *fifo_buffree();

struct gnode * spec_namei();
int fifo_open();
int fifo_close();
int fifo_rwgp();
int fifo_ioctl();
int fifo_select();
int spec_access();
int spec_link();
int spec_unlink();
int fifo_lock();
int fifo_unlock();
int fifo_rele();
int spec_inactive();
int spec_badop();
int spec_rename();
int spec_noop();
int spec_trunc();
int spec_syncgp();
int ufs_seek();
int fifo_stat();
int fifo_gupdat();
int spec_getval();

struct gnode_ops FIFO_gnode_ops = {
	spec_namei,	/* namei */
	spec_link,	/* link */
	spec_unlink,	/* unlink */
	0,		/* mkdir */
	spec_noop,	/* rmdir */
	0,		/* maknode */
	spec_rename,	/* rename */
	spec_noop,	/* getdirents */
	fifo_rele,	/* rele */
	spec_syncgp,	/* syncgp */
	0,		/* trunc */
	0,		/* getval */
	fifo_rwgp,	/* read, write */
	0,		/* rlock */
	ufs_seek,	/* seek */
	fifo_stat,	/* stat */
	fifo_lock,	/* lock */
	fifo_unlock,	/* unlock */
	fifo_gupdat,	/* update */
	fifo_open,	/* open */
	fifo_close,	/* close */
	fifo_select,	/* select */
	0,		/* readlink */
	0,		/* symlink */
	fifo_ioctl,	/* fcntl */
	0,		/* freegn */
	0		/* bmap */
};

struct gnode_ops *fifo_gnodeops = &FIFO_gnode_ops;
/*
 * open a fifo -- sleep until there are at least one reader & one writer
 */
/*ARGSUSED*/
int
fifo_open(gp, flag)
	struct gnode *gp;
	int flag;
{
	register struct fifonode *fp;

	gfs_lock(gp);
	if (gp->g_fifo == NULL) {
                KM_ALLOC(gp->g_fifo, struct fifonode *,
                         sizeof(struct fifonode), KM_TEMP, KM_CLEAR);
                if (gp->g_fifo == NULL) {
                        printf("fifo_open: Couldn't alloc %d bytes\n",
                                sizeof(struct fifonode));
                        panic("fifo_open: KM_ALLOC");
                }
	}
	/*
	 * Setjmp in case open is interrupted.
	 * If it is, close and return error.
	 */
	if (setjmp(&u.u_qsave)) {
		(void) fifo_close(gp, flag & FMASK);
		return (EINTR);
	}
	fp = GTOF(gp);

	if (flag & FREAD) {
		if (fp->fn_rcnt++ == 0)
			/* if any writers waiting, wake them up */
			wakeup((caddr_t) &fp->fn_rcnt);
	}

	if (flag & FWRITE) {
		if ((flag & (FNDELAY|FNBLOCK)) && (fp->fn_rcnt == 0)) {
			gfs_unlock(gp);
			return (ENXIO);
		}
		if (fp->fn_wcnt++ == 0)
			/* if any readers waiting, wake them up */
			wakeup((caddr_t) &fp->fn_wcnt);
	}

	if (flag & FREAD) {
		while (fp->fn_wcnt == 0) {
			/* if no delay, or data in fifo, open is complete */
			if ((flag & (FNDELAY|FNBLOCK)) || fp->fn_size) {
				gfs_unlock(gp);
				return (0);
			}
			(void) sleep_unlock((caddr_t) &fp->fn_wcnt, PPIPE, 
					    &gp->g_lk);
			gfs_lock(gp);
		}
	}

	if (flag & FWRITE) {
		while (fp->fn_rcnt == 0) {
			(void) sleep_unlock((caddr_t) &fp->fn_rcnt, PPIPE,
				     &gp->g_lk);
			gfs_lock(gp);
		}
	}
	gfs_unlock(gp);
	return (0);
}

/*
 * close a fifo
 * On final close, all buffered data goes away
 */
/*ARGSUSED*/
int
fifo_close(gp, flag)
	struct gnode *gp;
	int flag;
{
	register struct fifonode *fp;
	register struct fifo_bufhdr *bp;
	register struct	ucred *cred = u.u_cred;

	gfs_lock(gp);
	fp = GTOF(gp);

	if (flag & FREAD) {
		if (--fp->fn_rcnt == 0) {
			if (fp->fn_flag & IFIW) {
				fp->fn_flag &= ~IFIW;
				wakeup((caddr_t) &fp->fn_wcnt);
			}
			if (fp->fn_wselp) {
				selwakeup(fp->fn_wselp, fp->fn_flag & IF_WCOLL);
				fp->fn_flag &= ~IF_WCOLL;
				fp->fn_wselp = 0;
			}
			if ((fp->fn_flag & IF_RASYNC) || 
			    (fp->fn_flag & IF_WASYNC)) {
				fifo_wakeup(fp);
				fp->fn_flag &= ~IF_RASYNC;
				fp->fn_rpgrp = 0;
			}
		}
	}

	if (flag & FWRITE) {
		if (--fp->fn_wcnt == 0) {
			if (fp->fn_flag & IFIR) {
				fp->fn_flag &= ~IFIR;
				wakeup((caddr_t) &fp->fn_rcnt);
			}
			if (fp->fn_rselp) {
				selwakeup(fp->fn_rselp, fp->fn_flag & IF_RCOLL);
				fp->fn_flag &= ~IF_RCOLL;
				fp->fn_rselp = 0;
			}
			if ((fp->fn_flag & IF_RASYNC) || 
			    (fp->fn_flag & IF_WASYNC)) {
				fifo_wakeup(fp);
				fp->fn_flag &= ~IF_WASYNC;
				fp->fn_wpgrp = 0;
			}
		}
	}

	if ((fp->fn_rcnt == 0) && (fp->fn_wcnt == 0)) {
		/* free all buffers associated with this fifo */
		for (bp = fp->fn_buf; bp != NULL; ) {
			bp = fifo_buffree(bp, fp);
		}

		/* update times only if there were bytes flushed from fifo */
		if (fp->fn_size != 0) {
			gp->g_size = 0;
			gp->g_flag |= GUPD|GCHG;
			(void) fifo_gupdat(gp, timepick, timepick, 0, cred);
		}
		/* free fifo structure */
		kmem_free((caddr_t)GTOF(gp), KM_TEMP);
		gp->g_fifo = NULL;
	}
	gfs_unlock(gp);
	return (0);
}


/*
 * read/write a fifo
 */
/*ARGSUSED*/
int
fifo_rwgp(gp, uiop, rw, ioflag, cred)
	struct gnode *gp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct fifonode *fp;
	register struct fifo_bufhdr *bp;
	register u_int count;
	register int off;
	register unsigned i;
	register int rval = 0;
	int first_pass = 1;

#ifdef SANITY
	if ((ioflag & IO_APPEND) == 0)
		printf("fifo_rdwr: no append flag\n");
	if (uiop->uio_offset != 0)
		printf("fifo_rdwr: non-zero offset: %d\n", uiop->uio_offset);
#endif SANITY

	fp = GTOF(gp);
	gfs_lock(gp);

	if (rw == UIO_WRITE) {				/* UIO_WRITE */
		/*
		 * PIPE_BUF: max number of bytes buffered per open pipe
		 * PIPE_MAX: max size of single write to a pipe
		 *
		 * If the count is less than PIPE_BUF, it must occur
		 * atomically.  If it does not currently fit in the
		 * kernel pipe buffer, either sleep or return 0 bytes
		 * written, depending on FNDELAY  (*** EAGAIN? ***).
		 *
		 * If the count is greater than PIPE_BUF, it will be
		 * non-atomic (FNDELAY clear).  If FNDELAY is set,
		 * write as much as will fit into the kernel pipe buffer
		 * and return the number of bytes written.
		 *
		 * If the count is greater than PIPE_MAX, return EINVAL.
		 */
		if ((unsigned)uiop->uio_resid > PIPE_MAX) {
			rval = EINVAL;
			goto rdwrdone;
		}

		while (count = uiop->uio_resid) {
			if (fp->fn_rcnt == 0) {
				/* no readers anymore! */
				psignal(u.u_procp, SIGPIPE);
				rval = EPIPE;
				goto rdwrdone;
			}
			if ((count + fp->fn_size) > PIPE_BUF) {
				if (ioflag & (FNDELAY|FNBLOCK)) { /* NO DELAY */
					if (count <= PIPE_BUF) {
						/*
						 * Write will be satisfied
						 * atomically.
						 * Scholars Differ ::
						 * P1003 may require EAGAIN
						 */
						if (first_pass)
							if (u.u_procp->p_progenv == A_POSIX)
								rval = EAGAIN;
							else
								rval = EWOULDBLOCK;
						goto rdwrdone;
					} else if (fp->fn_size >= PIPE_BUF) {
					    /*
					     * Write will never be atomic.
					     * At this point, it cannot even be
					     * partial.   However, some portion
					     * of the write may already have
					     * succeeded.  If so, uio_resid
					     * reflects this.   If not, we will
					     * have to keep track of this fact
					     * in order to return EAGAIN.
					     */
						if (first_pass)
							if (u.u_procp->p_progenv == A_POSIX)
								rval = EAGAIN;
							else
								rval = EWOULDBLOCK;
						goto rdwrdone;
					}
				} else {			/* DELAY */
					if ( (count <= PIPE_BUF) ||
					    (fp->fn_size >= PIPE_BUF) ) {
				/*
				 * Sleep until there is room for this request.
				 * On wakeup, go back to the top of the loop.
				 */
						fp->fn_flag |= IFIW;
						(void) sleep_unlock((caddr_t)
						    &fp->fn_wcnt, PPIPE, 
						    &gp->g_lk);
						gfs_lock(gp);
						goto wrloop;
					}
				}
				/* at this point, can do a partial write */
				count = PIPE_BUF - fp->fn_size;
			}
			/*
			 * Can write 'count' bytes to pipe now.   Make sure
			 * there is enough space in the allocated buffer list.
			 * If not, try to allocate more.
			 * If allocation does not succeed immediately, go back
			 * to the  top of the loop to make sure everything is
			 * still cool.
			 */

#ifdef SANITY
			if ((fp->fn_wptr - fp->fn_rptr) != fp->fn_size)
			    printf("fifo_write: ptr mismatch...size:%d  wptr:%d  rptr:%d\n",
				fp->fn_size, fp->fn_wptr, fp->fn_rptr);

			if (fp->fn_rptr > PIPE_BSZ)
			    printf("fifo_write: rptr too big...rptr:%d\n",
				fp->fn_rptr);
			if (fp->fn_wptr > (fp->fn_nbuf * PIPE_BSZ))
			    printf("fifo_write: wptr too big...wptr:%d  nbuf:%d\n",
				fp->fn_wptr, fp->fn_nbuf);
#endif SANITY

			while (((fp->fn_nbuf * PIPE_BSZ) - fp->fn_wptr)
			    < count) {
				if ((bp = fifo_bufalloc(gp, fp)) == NULL) {
					goto wrloop;	/* fifonode unlocked */
				}
				/* new buffer...tack it on the of the list */
				bp->fb_next = (struct fifo_bufhdr *) NULL;
				if (fp->fn_buf == (struct fifo_bufhdr *) NULL) {
					fp->fn_buf = bp;
				} else {
					fp->fn_bufend->fb_next = bp;
				}
				fp->fn_bufend = bp;
			}
			/*
			 * There is now enough space to write 'count' bytes.
			 * Find append point and copy new data.
			 */
			bp = fp->fn_buf;
			for (off = fp->fn_wptr; off >= PIPE_BSZ;
			    off -= PIPE_BSZ)
				bp = bp->fb_next;
		
			while (count) {
				i = PIPE_BSZ - off;
				i = MIN(count, i);
				if (rval =
				    uiomove(&bp->fb_data[off], (int) i,
				    UIO_WRITE, uiop)){
					/* error during copy from user space */
					/* NOTE:LEAVE ALLOCATED BUFS FOR NOW */
					goto rdwrdone;
				}
				fp->fn_size += i;
				fp->fn_wptr += i;
				count -= i;
				off = 0;
				bp = bp->fb_next;
			}
			gp->g_flag |= GUPD|GCHG;
			(void) fifo_gupdat(gp, timepick, timepick, 0, cred);
			/* wake up any sleeping readers */
			if (fp->fn_flag & IFIR) {
				fp->fn_flag &= ~IFIR;
				wakeup((caddr_t) &fp->fn_rcnt);
			}
			/* wake up any procs sleeping on select */
			if (fp->fn_rselp) {
				selwakeup(fp->fn_rselp, fp->fn_flag & IF_RCOLL);
				fp->fn_flag &= ~IF_RCOLL;
				fp->fn_rselp = 0;
			}
wrloop: 		/* bottom of write 'while' loop */
			first_pass = 0;
			continue;
		}

	} else {					/* UIO_READ */
		/*
		 * Handle zero-length reads specially here
		 */
		if ((count = uiop->uio_resid) == 0) {
			goto rdwrdone;
		}
		while ((i = fp->fn_size) == 0) {
			if (fp->fn_wcnt == 0) {
				/* no data in pipe and no writers...(EOF) */
				goto rdwrdone;
			}
			/* no data in pipe, but writer is there */
			if (ioflag & (FNDELAY|FNBLOCK)) {
				rval = EAGAIN;
				goto rdwrdone;
			}
			fp->fn_flag |= IFIR;
			(void) sleep_unlock((caddr_t) &fp->fn_rcnt, PPIPE, 
					    &gp->g_lk);
			gfs_lock(gp);
			/* loop to make sure there is still a writer */
		}

#ifdef SANITY
		if ((fp->fn_wptr - fp->fn_rptr) != fp->fn_size)
			printf("fifo_read: ptr mismatch...size:%d  wptr:%d  rptr:%d\n",
			    fp->fn_size, fp->fn_wptr, fp->fn_rptr);

		if (fp->fn_rptr > PIPE_BSZ)
			printf("fifo_read: rptr too big...rptr:%d\n",
			    fp->fn_rptr);

		if (fp->fn_wptr > (fp->fn_nbuf * PIPE_BSZ))
			printf("fifo_read: wptr too big...wptr:%d  nbuf:%d\n",
			    fp->fn_wptr, fp->fn_nbuf);
#endif SANITY

		/*
		 * Get offset into first buffer at which to start getting data.
		 * Truncate read, if necessary, to amount of data available.
		 */
		off = fp->fn_rptr;
		bp = fp->fn_buf;
		count = MIN(count, i);	/* smaller of pipe size and read size */

		while (count) {
			i = PIPE_BSZ - off;
			i = MIN(count, i);
			if (rval =
			    uiomove(&bp->fb_data[off], (int)i, UIO_READ, uiop)){
				goto rdwrdone;
			}
			fp->fn_size -= i;
			fp->fn_rptr += i;
			count -= i;
			off = 0;

#ifdef SANITY
			if (fp->fn_rptr > PIPE_BSZ)
				printf("fifo_read: rptr after uiomove too big...rptr:%d\n",
				    fp->fn_rptr);
#endif SANITY

			if (fp->fn_rptr == PIPE_BSZ) {
				fp->fn_rptr = 0;
				bp = fifo_buffree(bp, fp);
				fp->fn_buf = bp;
				fp->fn_wptr -= PIPE_BSZ;
			}
			/*
			 * At this point, if fp->fn_size is zero, there may be
			 * an allocated, but unused, buffer.  [In this case,
			 * fp->fn_rptr == fp->fn_wptr != 0.]
			 * NOTE: FOR NOW, LEAVE THIS EXTRA BUFFER ALLOCATED.
			 * NOTE: fifo_buffree() CAN'T HANDLE A BUFFER NOT 1ST.
			 */
		}

		gp->g_flag |= GACC;    
                (void) fifo_gupdat(gp, timepick, timepick, 0, cred);

		/* wake up any sleeping writers */
		if (fp->fn_flag & IFIW) {
			fp->fn_flag &= ~IFIW;
			wakeup((caddr_t) &fp->fn_wcnt);
		}
		/* wake up any procs sleeping on select */
		if (fp->fn_wselp) {
			selwakeup(fp->fn_wselp, fp->fn_flag & IF_WCOLL);
			fp->fn_flag &= ~IF_WCOLL;
			fp->fn_wselp = 0;
		}
	}		/* end of UIO_READ code */

rdwrdone:
	gfs_unlock(gp);
	uiop->uio_offset = 0;		/* guarantee that f_offset stays 0 */
	return (rval);
}

/*
 * test for fifo selections
 */
/*ARGSUSED*/
int
fifo_select(gp, flag, cred)
	struct gnode *gp;
	int flag;
	struct ucred *cred;
{
	register struct fifonode *fp;
	int ret_val=0;

	fp = GTOF(gp);

	gfs_lock(gp);
	switch (flag) {
	case FREAD:
		if ((fp->fn_size != 0) || (fp->fn_wcnt == 0))
			ret_val = 1;
		else  {
			if (fp->fn_rselp)
				fp->fn_flag |= IF_RCOLL;
			else
				fp->fn_rselp = u.u_procp;
		}
		break;

	case FWRITE:
		if ((fp->fn_size < PIPE_BUF) || (fp->fn_rcnt == 0))
			ret_val=1;
		else {
			if (fp->fn_wselp)
				fp->fn_flag |= IF_WCOLL;
			else
				fp->fn_wselp = u.u_procp;
		}		
		break;
	}
	gfs_unlock(gp);

	return (ret_val);
}

int
fifo_rele(gp)
	struct gnode *gp;
{
	extern struct lock_t lk_gnode;

	gfs_lock(gp);

	if ((gp->g_mode & GFMT) == GFPIPE) {
		/*
		 * We have to take out a spin lock here to workaround an
		 * smp lock primitive problem. When a sleep lock resides
		 * in km_alloc space a race condition can exist between
		 * km_freeing the memory, and checking the wakeup bit in the
		 * lock. Its ugly, but it works... prs
		 */
		smp_lock(&lk_gnode, LK_RETRY);
		if (gp->g_count == 1) {
			gfs_unlock(gp);
			smp_unlock(&lk_gnode);
			kmem_free((caddr_t)gp, KM_TEMP);
		}
		else {
			gp->g_count--;
			gfs_unlock(gp);
			smp_unlock(&lk_gnode);
		}
		return (0);
	}

	gfs_unlock(gp);
}

int
fifo_gupdat(gp, ta, tm, waitfor, cred)
	struct gnode *gp;
	struct timeval *ta, *tm;
	int waitfor;
	struct ucred *cred;
{
	if ((gp->g_mode & GFMT) == GFPIPE) {
		if (gp->g_flag & GACC)
			gp->g_atime = *ta;
		if (gp->g_flag & GUPD)
			gp->g_mtime = *tm;
		if (gp->g_flag & GCHG)
			gp->g_ctime = *timepick;
		
		gp->g_flag &= ~(GUPD|GACC|GCHG|GMOD);
		return (0);
	}
	return (gp->g_altops->go_gupdat(gp, ta, tm, waitfor, cred));
}
		
fifo_wakeup(fp)
	register struct fifonode *fp;
{
	register struct proc *p;

	/*
	 * If ASYNC is set on fifo, send SIGIO signal to
	 * procs in process group.
	 */
	if (fp->fn_flag & IF_RASYNC) {
	        if (fp->fn_rpgrp < 0)
			gsignal(-fp->fn_rpgrp, SIGIO);
		else if ((fp->fn_rpgrp > 0) &&
		    (p = pfind(fp->fn_rpgrp)) != 0)
			psignal(p, SIGIO);
	}
	if (fp->fn_flag & IF_WASYNC) {
	        if (fp->fn_wpgrp < 0)
			gsignal(-fp->fn_wpgrp, SIGIO);
		else if ((fp->fn_wpgrp > 0) &&
		    (p = pfind(fp->fn_wpgrp)) != 0)
			psignal(p, SIGIO);
	}
}

/*
 * allocate a buffer for a fifo
 * return NULL if had to sleep
 */
static struct fifo_bufhdr *
fifo_bufalloc(gp, fp)
	register struct gnode *gp;
	register struct fifonode *fp;
{
	register struct fifo_bufhdr *bp;

	if (fifo_alloc >= PIPE_MNB) {
		/*
		 * Impose a system-wide maximum on buffered data in pipes.
		 * NOTE: This could lead to deadlock!
		 */
		(void) sleep_unlock((caddr_t) &fifo_alloc, PPIPE, &gp->g_lk);
		gfs_lock(gp);
		return ((struct fifo_bufhdr *)NULL);
	}

	/* the call to kmem_alloc() might sleep, so leave fifonode locked */

	fifo_alloc += FIFO_BUFFER_SIZE;
	kmem_alloc(bp, struct fifo_bufhdr *, (u_int)FIFO_BUFFER_SIZE, KM_TEMP);
	fp->fn_nbuf++;
	return ((struct fifo_bufhdr *) bp);
}


/*
 * deallocate a fifo buffer
 */
static struct fifo_bufhdr *
fifo_buffree(bp, fp)
	struct fifo_bufhdr *bp;
	struct fifonode *fp;
{
	register struct fifo_bufhdr *nbp;

	fp->fn_nbuf--;

	/*
	 * NOTE: THE FOLLOWING ONLY WORKS IF THE FREED BUFFER WAS THE 1ST ONE.
	 */
	if (fp->fn_bufend == bp)
		fp->fn_bufend = (struct fifo_bufhdr *) NULL;

	nbp = bp->fb_next;
	kmem_free((caddr_t)bp, KM_TEMP);

	if (fifo_alloc >= PIPE_MNB) {
		wakeup((caddr_t) &fifo_alloc);
	}
	fifo_alloc -= FIFO_BUFFER_SIZE;

	return (nbp);
}

int
fifo_lock(gp)
	struct gnode *gp;
{
	if ((gp->g_mode & GFMT) == GFPIPE) {
	        while (glocked(gp)) {
		       gp->g_flag |= GWANT;
		       sleep((caddr_t)gp, PINOD);
		}
		gfs_lock(gp);
		return (0);
	}
	return (gp->g_altops->go_lock (gp));
}

int
fifo_unlock(gp)
	struct gnode *gp;
{
	if ((gp->g_mode & GFMT) == GFPIPE) {
		gfs_unlock(gp);
		if (gp->g_flag&GWANT) {
		       gp->g_flag &= ~GWANT;
		       wakeup((caddr_t)gp);
		}
		return (0);
	}
	return (gp->g_altops->go_unlock (gp));
}

int
fifo_stat(gp, sb)
	struct gnode *gp;
	struct stat *sb;
{
	register struct fifonode *fp;

	fp = GTOF(gp);
	if ((gp->g_mode & GFMT) == GFPORT)	/* Named pipe */
		gp->g_altops->go_stat (gp, sb);
	else {
		bzero((caddr_t)sb, sizeof(*sb));
		sb->st_atime = gp->g_atime.tv_sec;
		sb->st_spare1 = gp->g_atime.tv_usec;
		sb->st_mtime = gp->g_mtime.tv_sec;
		sb->st_spare2 = gp->g_mtime.tv_usec;
		sb->st_ctime = gp->g_ctime.tv_sec;
		sb->st_spare3 = gp->g_ctime.tv_usec;
	}
	if (fp)
		sb->st_size = fp->fn_size;
	sb->st_blksize = PIPE_BUF;
	return (0);
}

/*ARGSUSED*/
int
fifo_ioctl(gp, com, data, flag, cred)
	struct gnode *gp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	register struct fifonode *fp;
	int error = 0;

	switch (com) {

		case FIONREAD:
			fp = GTOF(gp);
			*(off_t *)data = fp->fn_size;
			break;

		case FIOGETOWN:
			fp = GTOF(gp);
			if (flag & FREAD)
			      *(int *)data = fp->fn_rpgrp;
			else
			      *(int *)data = fp->fn_wpgrp;
			break;
		case FIOSETOWN:
			fp = GTOF(gp);
			if (flag & FREAD)
			      fp->fn_rpgrp = *(int *)data;
			else
			      fp->fn_wpgrp = *(int *)data;
			break;
		case FIOASYNC:
			fp = GTOF(gp);
			if (*(int *)data) {
			      if (flag & FREAD)
			            fp->fn_flag |= IF_RASYNC;
			      else
			            fp->fn_flag |= IF_WASYNC;
			}
			else {
			      if (flag & FREAD)
				    fp->fn_flag &= ~IF_RASYNC;
			      else
				    fp->fn_flag &= ~IF_WASYNC;
			}
			break;
		case FIONBIO:
			break;

		default:
			error = ENOTTY;
			break;
	}
	return (error);
}
