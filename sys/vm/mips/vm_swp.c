#ifndef lint
static char *sccsid = "@(#)vm_swp.c	4.1	(ULTRIX)	%G";
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
/*
 *
 *   Modification history:
 *
 *  2 Feb 88 - lp
 *	Fixed a problem on close of a non-nbuffered channel (check
 *	for no async_bp).
 *
 * 26 Jan 88 -- lp
 *	Fixed a race condition in asyncclose. This was uncovered
 *	by use of the new allocator in that some buffers were not 
 *	being freed as well as premature exit of the closing process.
 *
 * 14 Dec 87 -- jaa
 *	Added new KM_ALLOC/KM_FREE macros
 *
 * 15 Dec 86 -- depp
 *	The error messages in swkill() had become inconsistent and confusing,
 *	so I cleaned it up a tad (it really needs some rethinking).
 *
 * 11 Sep 86 -- lp
 *	Corrected problem in buffers which errored.
 *
 * 11 Sep 86 -- koehler
 *	corrected the text pointer name
 *
 * 20 Aug 86 -- koehler
 *	local execution/shmem fix (i.e., no text point attachment)
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines
 *
 * 11 Mar 86 -- lp
 *	Added routines for n-bufferring to raw devices.
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 11 Mar 85 -- depp
 *	Added System V shared memory support.
 *
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/trace.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/kmalloc.h"
#include "../h/cmap.h"
#include "../h/gnode.h"
#include "../h/mount.h"

/*
 * Swap IO headers -
 * They contain the necessary information for the swap I/O.
 * At any given time, a swap header can be in three
 * different lists. When free it is in the free list, 
 * when allocated and the I/O queued, it is on the swap 
 * device list, and finally, if the operation was a dirty 
 * page push, when the I/O completes, it is inserted 
 * in a list of cleaned pages to be processed by the pageout daemon.
 */
struct	buf *swbuf;

/*
 * swap I/O -
 *
 * If the flag indicates a dirty page push initiated
 * by the pageout daemon, we map the page into the i th
 * virtual page of process 2 (the daemon itself) where i is
 * the index of the swap header that has been allocated.
 * We simply initialize the header and queue the I/O but
 * do not wait for completion. When the I/O completes,
 * iodone() will link the header to a list of cleaned
 * pages to be processed by the pageout daemon.
 */
swap(p, dblkno, addr, nbytes, rdflg, flag, dev, pfcent)
	struct proc *p;
	swblk_t dblkno;
	caddr_t addr;
	int nbytes, rdflg, flag;
	dev_t dev;
	u_int pfcent;
{
	register struct buf *bp;
	register u_int c;
	register int p2dp;
	register struct pte *dpte, *vpte;
	register struct gnode *gp;
	int s;
	extern swdone();
	int (*strat)();
#ifdef GFSDEBUG
	extern short GFS[];
#endif

#ifdef mips
	XPRINTF(XPR_VM,"enter swap",0,0,0,0);
#endif mips
#ifdef vax
	s = spl6();
#endif vax
#ifdef mips
	s = splbio();
#endif mips
	while (bswlist.av_forw == NULL) {
		bswlist.b_flags |= B_WANTED;
		sleep((caddr_t)&bswlist, PSWP+1);
	}
	bp = bswlist.av_forw;
	bswlist.av_forw = bp->av_forw;
	splx(s);

#ifdef GFSDEBUG
	if(GFS[18])
		cprintf("swap: pid %d gp 0x%x (%d) nbytes %d addr 0x%x op %s\n",
		p->p_pid, p->p_textp ? p->p_textp->x_gptr : NULL,
		p->p_textp ? p->p_textp->x_gptr->g_number : NULL,
		nbytes, addr, rdflg == B_READ ? "READ" : "WRITE");
#endif
	bp->b_flags = B_BUSY | B_PHYS | rdflg | flag;
	if ((bp->b_flags & (B_DIRTY|B_PGIN)) == 0)
		if (rdflg == B_READ)
			sum.v_pswpin += btoc(nbytes);
		else
			sum.v_pswpout += btoc(nbytes);
	bp->b_proc = p;
	if (flag & B_DIRTY) {
		p2dp = ((bp - swbuf) * CLSIZE) * KLMAX;
		dpte = dptopte(&proc[2], p2dp);
		if(flag & B_SMEM)	/* SHMEM */
			vpte = ((struct smem *)p)->sm_ptaddr + btop(addr);
		else
			vpte = vtopte(p, btop(addr));
		for (c = 0; c < nbytes; c += NBPG) {
			if (vpte->pg_pfnum == 0 || vpte->pg_fod)
				panic("swap bad pte");
			*dpte++ = *vpte++;
		}
		bp->b_un.b_addr = (caddr_t)ctob(dptov(&proc[2], p2dp));
		bp->b_flags |= B_CALL;
		bp->b_iodone = swdone;
		bp->b_pfcent = pfcent;
	} else
		bp->b_un.b_addr = addr;
	if(flag & B_SMEM) {	/* SHMEM */
		gp = (struct gnode *) NULL;
		strat = bdevsw[major(dev)].d_strategy;
	} else {
		gp = p->p_textp ? p->p_textp->x_gptr : NULL;
		if(gp && GIOSTRATEGY(gp) && (dev == gp->g_dev)) {
			strat = GIOSTRATEGY(gp);
		} else
			strat = bdevsw[major(dev)].d_strategy;
	}
	while (nbytes > 0) {
		bp->b_bcount = nbytes;
		minphys(bp);
		c = bp->b_bcount;
		bp->b_blkno = dblkno;
		bp->b_dev = dev;
#ifdef TRACE
		trace(TR_SWAPIO, dev, bp->b_blkno);
#endif
		bp->b_gp = gp;
		physstrat(bp, strat, PSWP);
		if (flag & B_DIRTY) {
			if (c < nbytes)
				panic("big push");
			return;
		}
		bp->b_un.b_addr += c;
		bp->b_flags &= ~B_DONE;
		if (bp->b_flags & B_ERROR) {
			if ((flag & (B_UAREA|B_PAGET)) || rdflg == B_WRITE) {
				cprintf(
				"swap err: bp = 0x%x, bp->b_blkno = 0x%x\n",
				bp,bp->b_blkno);
				panic("hard IO err in swap");
			}
			if((bp->b_flags & B_SMEM) == 0)	/* SHMEM */
				swkill(p, (char *)0);
		}
		nbytes -= c;
		dblkno += btodb(c);
	}
#ifdef vax
	s = spl6();
#endif vax
#ifdef mips
	s = splbio();
#endif mips
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_PAGET|B_UAREA|B_DIRTY
						|B_SMEM);	/* SHMEM */
	bp->av_forw = bswlist.av_forw;
	bswlist.av_forw = bp;
	if (bswlist.b_flags & B_WANTED) {
		bswlist.b_flags &= ~B_WANTED;
		wakeup((caddr_t)&bswlist);
		wakeup((caddr_t)&proc[2]);
	}
	splx(s);
}

/*
 * Put a buffer on the clean list after I/O is done.
 * Called from biodone.
 */
swdone(bp)
	register struct buf *bp;
{
	register int s;

#ifdef mips
	XPRINTF(XPR_VM,"enter swdone",0,0,0,0);
#endif mips
	if (bp->b_flags & B_ERROR)
		panic("IO err in push");
#ifdef vax
	s = spl6();
#endif vax
#ifdef mips
	s = splbio();
#endif mips
	bp->av_forw = bclnlist;
	cnt.v_pgout++;
	cnt.v_pgpgout += bp->b_bcount / NBPG;
	bclnlist = bp;
	if (bswlist.b_flags & B_WANTED)
		wakeup((caddr_t)&proc[2]);
	splx(s);
}

/*
 * If rout == 0 then killed on swap error, else
 * rout is the name of the routine where we either ran out of
 * swap space, or some other process killing error occured.
 */
swkill(p, rout)
	register struct proc *p;
	register char *rout;
{
	register int pid = (int) p->p_pid;

#ifdef mips
	XPRINTF(XPR_VM,"enter swkill",0,0,0,0);
#endif mips
	if (rout) {
		printf("pid %d was killed in %s\n", pid, rout);
		uprintf("sorry, pid %d was killed: %s\n",pid, rout);
	}
	else {
		printf("pid %d was killed on swap error\n",pid);
		uprintf("sorry, pid %d was killed on swap error\n",pid);
	}

	/*
	 * To be sure no looping (e.g. in vmsched trying to
	 * swap out) mark process locked in core (as though
	 * done by user) after killing it so noone will try
	 * to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SULOCK;
}

/*
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose
 *	The device number
 *	Read/write flag
 * Essentially all the work is computing physical addresses and
 * validating them.
 * If the user has the proper access privilidges, the process is
 * marked 'delayed unlock' and the pages involved in the I/O are
 * faulted and locked. After the completion of the I/O, the above pages
 * are unlocked.
 */
physio(strat, bp, dev, rw, mincnt, uio)
	int (*strat)();
	register struct buf *bp;
	dev_t dev;
	int rw;
	unsigned (*mincnt)();
	struct uio *uio;
{
	register struct iovec *iov = uio->uio_iov;
	register int c;
	register char *a;
	register int s, error = 0;

#ifdef mips
	XPRINTF(XPR_VM,"enter physio",0,0,0,0);
#endif mips
nextiov:
	if (uio->uio_iovcnt == 0)
		return (0);
	if (useracc(iov->iov_base,(u_int)iov->iov_len,rw==B_READ?B_WRITE:B_READ) == NULL)
		return (EFAULT);
#ifdef vax
	s = spl6();
#endif vax
#ifdef mips
	s = splbio();
#endif mips
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO+1);
	}
	splx(s);
	if(bp->b_flags&B_RAWASYNC) {
		if(uio->uio_iovcnt > 1 ) /* readv/writev dont work w/n-buffer */
			return(EFAULT);
	}
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_un.b_addr = iov->iov_base;
	while (iov->iov_len > 0) {
		if(bp->b_flags&B_RAWASYNC)
			bp->b_flags |= B_BUSY | B_PHYS | rw;
		else
			bp->b_flags = B_BUSY | B_PHYS | rw;
		bp->b_dev = dev;
		bp->b_blkno = btodb(uio->uio_offset);
		bp->b_bcount = iov->iov_len;
		(*mincnt)(bp);
		c = bp->b_bcount;
#ifdef vax
		s = spl6();
#endif vax
#ifdef mips
		s = splbio();
#endif mips
		u.u_procp->p_flag |= SPHYSIO;
		vslock(a = bp->b_un.b_addr, c);
		splx(s);
			
		physstrat(bp, strat, PRIBIO);

		if(bp->b_flags&B_RAWASYNC) {
			/* Transfer has not started */
			if(bp->b_flags&B_ERROR)
				return(geterror(bp));
			/* else transfer has started */
			iov->iov_len -= c;
			uio->uio_resid -= c;
			uio->uio_offset += c;
			return(0);
		}
#ifdef vax
		(void) spl6();
#endif vax
#ifdef mips
		(void) splbio();
#endif mips
		vsunlock(a, c, rw);
		u.u_procp->p_flag &= ~SPHYSIO;
		if (bp->b_flags&B_WANTED)
			wakeup((caddr_t)bp);
		splx(s);
		c -= bp->b_resid;
		bp->b_un.b_addr += c;
		iov->iov_len -= c;
		uio->uio_resid -= c;
		uio->uio_offset += c;
		/* temp kludge for tape drives */
		if (bp->b_resid || (bp->b_flags&B_ERROR))
			break;
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
	error = geterror(bp);
	/* temp kludge for tape drives */
	if (bp->b_resid || error)
		return (error);
	uio->uio_iov++;
	uio->uio_iovcnt--;
	goto nextiov;
}

#define MAXPHYS (64 * 1024)

unsigned
minphys(bp)
	register struct buf *bp;
{

#ifdef mips
	XPRINTF(XPR_VM,"enter minphys",0,0,0,0);
#endif mips
	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
}

/*** Start or n-bufferring code ***/

#define ASYNC_RCOLL 	0x01
#define ASYNC_WCOLL	0x02
#define b_selflags b_pfcent

struct abuf {
	struct abuf *b_forw, *b_back;
	struct buf bp;
};
struct abuf *async_bp = (struct abuf *)0;

/*
 * doingasync is used to decide if this device is really able to
 * do n-bufferring.
 */

doingasync(dev, flag)
	dev_t dev;
	int flag;
{
	register struct buf *bp;
	register struct abuf *dp;

#ifdef mips
	XPRINTF(XPR_VM,"enter doingasync",0,0,0,0);
#endif mips
	if((flag&FNBUF) == 0 || async_bp == (struct abuf *)0)
		return(0);
	for(dp = async_bp->b_forw; dp != async_bp; dp = dp->b_forw) {
		bp = &dp->bp;
		if(bp->b_dev == dev)
			return(1);
		if(dp->b_forw == async_bp)
			break;
	}
	return(0);
}

/* 
 * Startasync is called by an ioctl. Its passed the number of buffers
 * to be used for n-bufferring while this device is open.
 */

startasync(dev, count, flags)
	dev_t dev;
	int *count;
	int flags;	/* fp flags */
{
	register int found = 0, s;
	register struct abuf *tdp;
	register struct proc *p = u.u_procp;
			
#ifdef mips
	XPRINTF(XPR_VM,"enter startasync",0,0,0,0);
#endif mips
		if(async_bp == 0) {
			KM_ALLOC(async_bp, struct abuf *, sizeof(struct abuf), KM_DEVBUF, KM_CLEAR);
			async_bp->b_forw = async_bp;
			async_bp->b_back = async_bp;
		}
	for(;found < *count; found++) {
			KM_ALLOC(tdp, struct abuf *, sizeof(struct abuf), KM_DEVBUF, KM_CLEAR | KM_NOWAIT);
			if(tdp == (struct abuf *)0)
				break;
#ifdef vax
			s = spl5();
#endif vax
#ifdef mips
			s = splbio();
#endif mips
			if(async_bp == 0) {
				cprintf("No async_bp head\n");
			} else
				binshash(tdp, async_bp);	
			splx(s);
			tdp->bp.b_flags = 0;
			if(flags & FASYNC) {
				tdp->bp.b_flags |= B_WANTED;
			}
			tdp->bp.b_proc = p;
			tdp->bp.b_dev = dev;
		}
		*count = found;
}

/* 
 * aiodone is called via an ioctl to check on a buffers status.
 * It might block (unless non-blocking io enabled) until the
 * buffer completes.
 */

aiodone(dev, addr, flags)
	dev_t dev;
	int addr;
	int flags;	/* fp flags! */
{
	register struct abuf *dp;
	register struct buf *bp;

#ifdef mips
	XPRINTF(XPR_VM,"enter aiodone",0,0,0,0);
#endif mips
	if(async_bp == (struct abuf *)0)
		return(EINVAL);	
	for(dp = async_bp->b_forw; dp != async_bp; dp = dp->b_forw) {
		register int s;
		bp = &dp->bp;
		if(dev != bp->b_dev) /* On correct device */
			continue;
		if(u.u_procp != bp->b_proc) /* By me */
			continue;

		if(addr == (int) bp->b_un.b_addr) {
#ifdef vax
			s=spl5();
#endif vax
#ifdef mips
			s=splbio();
#endif mips
			if(bp->b_flags&B_BUSY) {/* Wait till done */
				if(flags&FNDELAY) {
					u.u_error = EWOULDBLOCK;
					splx(s);
					return(-1);
				}
				sleep((caddr_t) bp, PRIBIO);
			}
			splx(s);
			/* We must be done here */
			if(bp->b_flags&B_ERROR) {
				bp->b_flags &= ~(B_BUSY|B_PHYS|B_DONE);
				return(geterror(bp));
			}
			/* Clear flags out so I can reuse buffer */
			bp->b_flags &= ~(B_BUSY|B_PHYS|B_ERROR|B_DONE);
			u.u_r.r_val1 = bp->b_bcount - bp->b_resid;
			return(0);
		}
	}
	return(EINVAL);
}

/* 
 * Select code for raw devices (which can do n-bufferring).
 */

asyncsel(dev, rw)
	dev_t dev;
	int rw;
{
	register struct buf *bp;
	register struct abuf *dp;
#ifdef vax
	register int s = spl5();
#endif vax
#ifdef mips
	register int s = splbio();
#endif mips
	register int doown = 0;
	
#ifdef mips
	XPRINTF(XPR_VM,"enter asyncsel",0,0,0,0);
#endif mips
	if(async_bp == (struct abuf *)0)
		return(EINVAL);	
	for(dp = async_bp->b_forw; dp != async_bp; dp = dp->b_forw) {
		bp = &dp->bp;
		if(bp->b_dev != dev)
			continue;
		if(bp->b_proc != u.u_procp)
			continue;
		doown++;
		if(bp->b_flags&B_BUSY)
			continue;
		splx(s);
		return(1); /* Found a non-busy buffer */
	}
	if(bp->b_proc->p_wchan == (caddr_t) &selwait)
		bp->b_selflags |= (rw == FREAD ? ASYNC_RCOLL : ASYNC_WCOLL);
	splx(s);
	if(!doown) /* None owned so must be ok */
		return(1);
	return(0);	
}

/*
 * asyncclose is called to clean things up when the file desc. gets
 * closed down. Frees buffers and blocks till pending buffers finish.
 */

asyncclose(dev, flag)
	dev_t dev;
	register flag;
{
	register int otherasync = 0;
	register struct abuf *dp, *next;
	register int s;

#ifdef mips
	XPRINTF(XPR_VM,"enter asyncclose",0,0,0,0);
#endif mips
	if (doingasync(dev, flag)) { /* This dev was doing async */
		for(dp = async_bp->b_forw; dp != async_bp;) {
			if(dp->bp.b_dev != dev) {
				dp = dp->b_forw;
				continue;
			}
			if(dp->bp.b_proc != u.u_procp) {
				otherasync++;
				dp = dp->b_forw;
				continue;
			}
#ifdef vax
			s = spl5();
#endif vax
#ifdef mips
			s = splbio();
#endif mips
			if(dp->bp.b_flags&B_BUSY)
				sleep((caddr_t) &dp->bp, PRIBIO);
			splx(s);
			if(dp->bp.b_flags&B_WANTED)
				wakeup((caddr_t)&dp->bp);
#ifdef vax
			s = spl5();
#endif vax
#ifdef mips
			s = splbio();
#endif mips
			next = dp->b_forw;
			bremhash(dp);
			splx(s);
			KM_FREE(dp, KM_DEVBUF);
			dp = next;
		}
	}
	return(otherasync);
}

/* 
 * Asyncdone is called from interrupt level to mark the passed buffer
 * as being done. Someone might be blocked waiting for the buffer 
 * so we issue a wakeup. Someone also might want to be notified 
 * and if so issue a sigio.
 */
asyncdone(bp)
register struct buf *bp;
{
	register struct pte *pte;
	register int npf;
	register struct cmap *c;

#ifdef mips
	XPRINTF(XPR_VM,"enter asyncdone",0,0,0,0);
#endif mips
	pte = vtopte(bp->b_proc, btop(bp->b_un.b_addr));
	npf = btoc(bp->b_bcount + ((int)bp->b_un.b_addr & CLOFSET));
	while (npf > 0) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		MUNLOCK(c);
		if(bp->b_flags&B_READ)
			pte->pg_m = 1;
		pte += CLSIZE;
		npf -= CLSIZE;
	}
	bp->b_flags &= ~B_BUSY;
	/* Someone waiting on a select */
	if(bp->b_proc->p_wchan == (caddr_t)&selwait)
		selwakeup(bp->b_proc, bp->b_selflags & 
			((bp->b_flags&B_READ) ? ASYNC_RCOLL : ASYNC_WCOLL));
	/* Someone waiting on SIGIO */
	if(bp->b_flags & B_WANTED) /* Someone wants to be notified */
		psignal(bp->b_proc, SIGIO);
	/* Anyone waiting on this buffer */
	wakeup((caddr_t) bp);
}

/* 
 * aphysio is the front end to physio for n-buffered case. It finds
 * an available buffer & calls physio.
 */

aphysio(strat, dev, rw, uio)
	dev_t dev;
	struct uio *uio;
	int rw;
	int (*strat)();
{
	register struct abuf *dp;
	register struct buf *bp;
	register int anyerror = 0, s;


#ifdef mips
	XPRINTF(XPR_VM,"enter aphysio",0,0,0,0);
#endif mips
		if(async_bp == (struct abuf *)0)
			return(EINVAL);	
		for(dp = async_bp->b_forw; dp != async_bp; dp = dp->b_forw) {
			bp = &dp->bp;
			if(bp->b_dev != dev) /* On correct device */
				continue;
			if(bp->b_proc != u.u_procp)
				continue;
#ifdef vax
			s = spl5();
#endif vax
#ifdef mips
			s = splbio();
#endif mips
			if(bp->b_flags&B_BUSY) { /* bp done ? */
				splx(s);
				continue;
			} else {
			splx(s);
			if(bp->b_flags&B_PHYS) /* Have I used it yet ? */
				continue;
			goto found;
			}
		}
			return(ENOBUFS);
found:
		bp->b_resid = 0;
		bp->b_flags &= ~(B_ERROR|B_BUSY);
		bp->b_flags |= B_RAWASYNC|B_CALL;
		bp->b_iodone = asyncdone;

		anyerror = physio(strat, bp, dev, rw, minphys, uio);
	return (anyerror);
}
