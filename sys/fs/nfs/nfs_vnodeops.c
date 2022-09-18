#ifndef lint
static	char	*sccsid = "@(#)nfs_vnodeops.c	4.10	(ULTRIX)	4/25/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by			*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 * 25 Apr 91 -- chet
 *	Add access check in nfs_lookup() after dnlc hit.
 *
 * 28 Feb 91 -- dws
 *	Clean up client view of fhandle.
 *
 * 27 Feb 91 -- chet
 *	Fix client side attribute problems when modify time goes backwards.
 *
 * 15 Feb 91
 *	Fixed nfs_fsync for case where file size > (0x7fffffff - (blksize-1)).
 *
 * 10 Feb 91 -- chet
 *	Change nfs_attrcache() calls (again).
 *
 * 29 Jan 91 -- chet
 *	Make kernel RPC calls interruptible.
 *
 * 28 Oct 90 -- chet
 *	Change nfs_fsync() scheme to keep rnode dirty and gp locked
 *	during complete operation.
 *
 *  8-Sep-90 -- Fred
 *	Transfer complete file handle to lock handle structure in nfs_rlock ().
 *
 *  7 Jul 90 -- chet
 *	Change nfs_write(), nfs_fsync() and nfs_attrcache() schemes;
 *	remove count of outstanding write buffers.
 *	
 * 20 Jun 90 -- cb
 *	Fix the attribute cache.
 *
 *  9 Mar 90 -- chet
 *	Change nfs_fsync() and nfs_attrcache() schemes to
 *	use a count of outstanding write buffers.
 *
 * 23-Feb-90 -- sekhar
 *      Merged Joe Martin's fix for 3.1 cld. When copying user PTEs,
 *      check for page crossing and reevaluate vtopte.
 *
 * 15 Feb 90 -- prs
 *	Added referencing the gnode in nfs_strategy() before scheduling a
 *	wakeup to a sleeping async daemon. This way, a biod will hold
 *	a ref to a gnode even if the invoking process closes the file.
 *
 *  7 Feb 90 -- chet
 *	Change asynch daemon work list scheme.
 *
 *  6 Feb 90 -- chet
 *	Change block I/O gnode referencing and nfs_fsync() gnode
 *	locking schemes.
 *
 *  2 Feb 90 -- chet
 *	Change asynchronous I/O gnode and buffer synchronization
 *
 *  1 Feb 90 -- prs
 *	Fixed leaf node caching into dnlc.
 *
 * 13 Dec 89 -- chet
 *	Add attribute cache timeout values.
 *
 * 10 Dec 89 -- chet
 *	Remove dnlc purge call for non-directories in nfs_attrcache.
 *
 * 05 Oct 89 -- prs
 *	Added locking of the [vg]node to nfs_close().
 *
 * 25 Jul 89 -- chet
 *	Changes for new bflush() and faster cache invalidations
 *
 *  6 Mar 89 -- chet
 *	Put regular files into dnlc; make size of dnlc a function
 *	of system size.
 *
 * 06 Feb 89 -- prs
 *      Modified nfs_readdir() and nfs_close() to set u.u_error before
 *      returning with an error value. These routines are called by
 *      GFS directly, and assumed to set u.u_error before returning.
 *
 * 21 Nov 88 -- condylis
 *	Added freeing of vnode credential to nfs_inactive.
 *
 * 07 Nov 88 -- dgg
 *      Corrected calculation of pte from proc pointer in nfs_strategy.
 *      [ dgg001 ]
 *
 * 28 Sep 88 -- chet
 *	Put R_IN_FSYNC define where it belongs (vnode.h)
 *
 *  1 Sep 88 -- chet
 *	Set u.u_error after a failed rfscall to readdir
 *
 * 5 Aug 88 -- condylis
 *	Merge of 2.4 changes.  Minimize number of calls to vtor and
 *	check change in file size in nfs_attrcache.
 *
 * 18 Jul 88 -- condylis
 *	Add SMP locking for bio daemon buffer list and async_daemon_count
 *	variable.  dnlc_lookup now bumps ref count of returned gnode;
 *	removed VN_HOLD after call.  Replaced unsafe modification of gnodes
 *	with calls to SMP gnode primitives.
 *
 * 2 Mar 88 -- chet
 *	Add RNOCACHE stuff for locked files to rwvp().
 *
 * 4 Feb 88 -- chet for cb
 *	add fifo mode fix in nfs_create()
 *
 * 26 Jan 88 -- chet
 *	Put access check in nfs_getdirent(); lock gnode before doing
 *	the access check to synchronize with any other process that has
 *	it locked.
 *
 * 12 Jan 88 - Fred Glover
 *	Add routine nfs_rlock for Sys-V file locking.
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 * 14 Jul 87 -- cb
 *	added in rr's changes to remote execution.
 *
 * 14 Jul 87 -- chet
 *	Changed binval() calls to binvalfree() calls in nfs_attrcache()
 *	and nfs_close().
 *
 * 16 Jun 87 -- cb
 *      Added check to biod code to die gracefully on termination.
 *
 * 12 May 87 -- chet
 *	Added commented check for opening special files.
 *
 * 11-May-87 -- logcher
 *	Removed the mpurge which had wrong logic anyways.  Does
 *	not gain much if right logic was there.
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, removed an unused argument from
 *	nfs_create, added support for mknod of non-regular files,
 *	added code to support swapping to an NFS file.
 */

#include "../h/param.h"
#include "../h/mount.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/cmap.h"
#include "../h/proc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/vmmac.h"
#include "../net/netinet/in.h"
#include "../net/rpc/types.h"
#include "../net/rpc/auth.h"
#include "../net/rpc/clnt.h"
#include "../net/rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/vfs.h"
#include "../nfs/vnode.h"
#include "../h/fs_types.h"
#include "../net/rpc/lockmgr.h"

/* SMP lock for biod bfr list */
struct lock_t	lk_nfsbiod;

/* MPC Counter to monitor wasted biod wakeups */
int	biod_has_work;
int	biod_has_no_work;

struct vnode *makenfsnode();
struct vnode *dnlc_lookup();
char *newname();

int nfs_dnlc = 1;

#define check_stale_fh(errno, vp) if ((errno) == ESTALE) { dnlc_purge_vp(vp); }
#define	nfsattr_inval(vp)	(vtor(vp)->r_nfsattrtime.tv_sec = 0)

#define ISVDEV(t) ((t == VBLK) || (t == VCHR) || (t == VFIFO))


/*
 * These are the vnode ops routines which implement the vnode interface to
 * the networked file system.  These routines just take their parameters,
 * make them look networkish by putting the right info into interface structs,
 * and then calling the appropriate remote routine(s) to do the work.
 *
 * Note on directory name lookup cacheing:  we desire that all operations
 * on a given client machine come out the same with or without the cache.
 * This is the same property we have with the disk buffer cache.  In order
 * to guarantee this, we serialize all operations on a given directory,
 * by using lock and unlock around rfscalls to the server.  This way,
 * we cannot get into races with ourself that would cause invalid information
 * in the cache.  Other clients (or the server itself) can cause our
 * cached information to become invalid, the same as with data buffers.
 * Also, if we do detect a stale fhandle, we purge the directory cache
 * relative to that vnode.  This way, the user won't get burned by the
 * cache repeatedly.
 */


int
nfs_open(vp, flag)
	register struct	vnode *vp;
	int flag;
{
	register struct ucred *cred = u.u_cred;
	register int error;

	/*
	 * refresh cached attributes
	 */
	nfsattr_inval(vp);
	error = nfs_getattr(vp, cred);
	if (!error) {
		vtor(vp)->r_flags |= ROPEN;
	}

	return (error);
}

int
nfs_close(vp, flag)
	register struct	vnode *vp;
	int	flag;
{
	register struct ucred *cred = u.u_cred;
	register struct rnode *rp = vtor(vp);

	gfs_lock(vp);
	rp->r_flags &= ~ROPEN;

	/*
	 * If this is a close of a file open for writing or an unlinked
	 * open file or a file that has had an asynchronous write error,
	 * flush synchronously. This allows us to invalidate the file's
	 * buffers if there was a write error or the file was unlinked.
	 */
	if (flag & FWRITE || rp->r_unldvp != NULL || rp->r_error) {
		(void) nfs_fsync(vp); /* NB: nfs_getattr() will unlock vp */
	}
	if (rp->r_unldvp != NULL || rp->r_error) {
		cacheinval((struct gnode *)vp);		
		/* binvalfree((struct gnode *)vp); */
		dnlc_purge_vp(vp);
	}
	gfs_unlock(vp);

	if (flag & FWRITE) {
		/* r_error is never cleared on a ref'd gnode */
		u.u_error = rp->r_error;
                return;
        }
}

int nfs_read_incore = 0;
int nfs_read_not_incore = 0;
int nfs_bread1 = 0;
int nfs_breada1 = 0;
int nfs_bread2 = 0;
int nfs_rfsread_count = 0;

int
rwvp(vp, uio, rw, ioflag)
	register struct vnode *vp;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	register struct buf *bp;
	register struct rnode *rp = vtor(vp);
	register int n, on, size;
	daddr_t bn, mapped_bn, mapped_rabn;
	int eof = 0;
	int error = 0;


	if (uio->uio_resid == 0) {
		return (0);
	}
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0) {
		return (EINVAL);
	}
	if (rw == UIO_WRITE && vp->v_type == VREG &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
	size = vtoblksz(vp);
	size &= ~(DEV_BSIZE - 1);
	if (size <= 0) {
		panic("rwvp: zero size");
	}

	do {
		bn = uio->uio_offset / size;
		on = uio->uio_offset % size;
		n = MIN((unsigned)(size - on), uio->uio_resid);
		nfs_bmap(vp, bn, &mapped_bn);

		/* Don't use cache for RNOCACHE reads */
		if (rp->r_flags & RNOCACHE) {
			bp = geteblk(size);
			if (rw == UIO_READ) {
				error = nfsread(bp, vp, bp->b_un.b_addr+on,
						uio->uio_offset, n,
						u.u_cred, NFS_NOT_BLOCKIO);
				if (error) {
					brelse(bp);
					goto bad;
				}
			}

	        } else if (rw == UIO_READ) {
			if (incore(vp->g_dev, mapped_bn, vp)) {
				/*
				 * get attributes to check whether in
				 * core data is stale
				 */
				++nfs_read_incore;
				(void) nfs_getattr(vp, u.u_cred);
			}
			else
				++nfs_read_not_incore;

			if (vp->g_lastr + 1 == bn) {
				nfs_bmap(vp, bn + 1, &mapped_rabn);
				++nfs_breada1;
				bp = breada(vp->g_dev, mapped_bn,
					    size, mapped_rabn, size,
					    vp);
			} else {
				++nfs_bread1;
				bp = bread(vp->g_dev, mapped_bn,
					   size, vp);
			}

		} else {  /* (rw == UIO_WRITE) */

			struct gnode *gp = (struct gnode *) (vp);

			if (rp->r_error) {
				error = rp->r_error;
				goto bad;
			}

			if (gp->g_textp) xuntext(gp->g_textp);
			if (n == size) {
				bp = getblk(vp->g_dev, mapped_bn, size, vp);
			} else {
				++nfs_bread2;
				bp = bread(vp->g_dev, mapped_bn, size, vp);
			}
		}
/* CJXXX */
		if (bp->b_flags & B_ERROR) {
			error = geterror(bp);
			brelse(bp);
			goto bad;
		}

		/*
		 * The following code was moved down here when RNOCACHE
		 * was put in so that both cached and non-cached reads
		 * fall through.
		 */
		if (rw == UIO_READ) {
			int diff;

			vp->g_lastr = bn;
			diff = vp->g_size - uio->uio_offset;
			if (diff < n) {
				if (diff <= 0) {
					brelse(bp);
					return(0);
				}
				n = diff;
				eof = 1;
			}
		}

		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);

		if (rw == UIO_READ) {
			if (rp->r_flags & RNOCACHE)
			  bp->b_flags |= B_NOCACHE;
			brelse(bp);

		} else {   /* (rw == UIO_WRITE) */
			/*
			 * g_size is the maximum number of bytes known
			 * to be in the file.
			 * Make sure it is at least as high as the last
			 * byte we just wrote into the buffer.
			 */
			if (vp->g_size < uio->uio_offset) {
				vp->g_size = uio->uio_offset;
			}
 
			/* Don't cache any RNOCACHE writes */
			if (rp->r_flags & RNOCACHE) {
				error = nfswrite(bp, vp, bp->b_un.b_addr+on,
						 uio->uio_offset-n, n,
						 u.u_cred, NFS_NOT_BLOCKIO);
				bp->b_flags |= B_NOCACHE;
				brelse(bp);

                        } else  {
				rp->r_flags |= RDIRTY;
				if (n + on == size) {
					bp->b_resid = 0;
					if ((ioflag & IO_SYNC) ||
					    vp->g_mp->m_flags & M_SYNC)
						bwrite(bp);
				        else
						bawrite(bp);
				} else {
					/*
					 * The bp->b_resid field is the number
					 * of bytes in the buffer that are NOT
					 * part of the file. We first compute
					 * how many bytes beyond the end of
					 * the file the last byte in the buffer
					 * is (think about it). If the file
					 * continues past the end of the block
					 * then this value will be negative,
					 * and we set it to zero since all the
					 * bytes in the buffer are valid. The
					 * result is used by nfswrite to decide
					 * how many bytes to send to the
					 * server.
					 */
					bp->b_resid = (size * (bn + 1)) -
					  vp->g_size;
					if (bp->b_resid < 0)
					  bp->b_resid = 0;
					if (ioflag & IO_SYNC ||
					    vp->g_mp->m_flags & M_SYNC)
						bwrite(bp);
				        else {
						/* gid could have changed */
						/* due to a cacheinval as */
						/* part of bread() above; */
						/* set gid so that buffer */
						/* is not orphaned */
						bp->b_gid = vp->g_id;
						bdwrite(bp);
					}
				}
			}
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && !eof);

	if (rw == UIO_WRITE && uio->uio_resid && u.u_error == 0) {
		printf("rwvp: short write. resid %d vp %x bn %d\n",
		    uio->uio_resid, vp, bn);
	}

	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:

	return (error);
}

/*
 * Write to a remote file.
 * Writes to remote server in largest size chunks that the server can
 * handle.  Write is synchronous from the client's point of view.
 */
int
nfswrite(bp, vp, base, offset, count, cred, blockio)
	struct buf *bp;
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	struct ucred *cred;
	int blockio;	/* Block I/O?:
			 *   1 (NFS_BLOCKIO) if called by do_bio(),
			 *   0 (NFS_NOT_BLOCKIO) if called directly by rwvp(),
			 *     i.e. NFS locked files.
			 */
{
	register struct rnode *rp = vtor(vp);
	int error;
	struct nfswriteargs wa;
	struct nfsattrstat *ns;
	int tsize;
	int async = 0;

	/*
	 * Check for write-behind (asynchronous) errors, and
	 * throw this request away if there has been one.
	 */
	error = rp->r_error; /* N.B.: gnode may not be locked here */
	if (error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
		if (blockio)	/* If doing block I/O, free the buffer */
			iodone(bp);
		return(error);
	}
		
	kmem_alloc(ns, struct nfsattrstat *, (u_int)sizeof(*ns), KM_NFS);
	do {
		tsize = min(vtomi(vp)->mi_stsize, count);
		wa.wa_data = base;
		wa.wa_fhandle = *vtofh(vp);
		wa.wa_begoff = offset;
		wa.wa_totcount = tsize;
		wa.wa_count = tsize;
		wa.wa_offset = offset;
		error = rfscall(vtomi(vp), RFS_WRITE, xdr_writeargs,
				(caddr_t)&wa, xdr_attrstat, (caddr_t)ns, cred);
		if (!error) {
			error = geterrno(ns->ns_status);
			check_stale_fh(error, vp);
		}
		count -= tsize;
		base += tsize;
		offset += tsize;
	} while (!error && count);

	if (bp->b_flags & B_ASYNC)
		async = 1;

	if (error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
		
	}

	/*
	 * N.B. It's easy to forget (or not understand in the first place)
	 * that an async write is handled by a
	 * biod only if there is one available; if there isn't, then
	 * the calling process gets blocked. This can be the process
	 * actually doing writes, the update daemon, or some completely 
	 * random victim that stumbled into a DELWRI buffer.
	 * Synchronous writes can be the writing (or nfs_fsync()'ing) process
	 * or a random buffer cache victim.
	 * Thus, the gnode may or may not be held locked by the calling
	 * process. This can cause deadlock problems between
	 * buffers and gnodes.
	 * 
	 * The general gnode/buffer rule to avoid deadlocks is:
	 * 	`Don't hold a buffer busy while attempting to lock a gnode.'
	 *
	 * This is why we free the buffer before calling nfs_attrcache().
	 * If the write fails and it was asynchronous all future writes will
	 * get an error. We must do the rp->r_error assignment before
	 * releasing the buffer for proper nfs_fsync() synchronization, but
	 * can't get the gnode lock since we could deadlock. 
	 *
	 * Although it would be desirable to never throw attributes away,
	 * there is a (hopefully rare) deadlock case where we are
	 * writing for the random victim which may have anything locked.
	 * In this case, we must throw attributes away.
	 * 
	 * Since trying to account for all write buffers leads to deadlocks,
	 * we rely on the RDIRTY flag used by nfs_fsync() when reconciling
	 * file attributes in nfs_attrcache().
	 * When we release buffers here, that frees nfs_fsync() to complete
	 * and reset RDIRTY before async write processes (which would
	 * block on vp) have run through nfs_attrcache().
	 * So as to not create a race with nfs_attrcache() which could
	 * result in confused file attributes, we never call
	 * it on behalf of async writes.
	 */

	if (error && async)
		rp->r_error = error;

	/*
	 * Free the buffer and rely on nfs_attrcache() to worry about
	 * races with other processes that provide fresher attributes
	 * than ours.
	 */
	if (blockio)	/* If doing block I/O, free the buffer */
		iodone(bp);

	/* Never cache async or error attributes */
	if (!async && !error)
		nfs_attrcache(vp, &ns->ns_attr, NOFLUSH);

	kmem_free(ns, KM_NFS);
	switch (error) {
	      case 0:
	      case EDQUOT:
		break;

	      case ENOSPC:
		printf("NFS write error, server %s, remote file system full\n",
		   vtomi(vp)->mi_hostname);
		break;

	      default:
		printf("NFS write error %d on host %s fh ", error, vtomi(vp)->mi_hostname);
		printfhandle((caddr_t)vtofh(vp));
		printf("\n");
		break;
	}

	return (error);
}


/*
 * Print a file handle
 */
printfhandle(fh)
	caddr_t fh;
{
	int i;
	int fhint[NFS_FHSIZE / sizeof (int)];

	bcopy(fh, (caddr_t)fhint, sizeof (fhint));
	for (i = 0; i < (sizeof (fhint) / sizeof (int)); i++) {
		printf("%x ", fhint[i]);
	}
}


/*
 * Read from a remote file.
 * Reads data in largest chunks our interface can handle
 */

int
nfsread(bp, vp, base, offset, count, cred, blockio)
	struct buf *bp;
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	struct ucred *cred;
	int blockio;	/* Block I/O?:
			 *   1 (NFS_BLOCKIO) if called by do_bio(),
			 *   0 (NFS_NOT_BLOCKIO) if called by rwvp(),
			 *     i.e. NFS locked files.
			 */
{
	int error;
	struct nfsreadargs ra;
	struct nfsrdresult rr;
	register int tsize;

	do {
		tsize = min(vtomi(vp)->mi_tsize, count);
		rr.rr_data = base;
		ra.ra_fhandle = *vtofh(vp);
		ra.ra_offset = offset;
		ra.ra_totcount = tsize;
		ra.ra_count = tsize;
		++nfs_rfsread_count;
		error = rfscall(vtomi(vp), RFS_READ, xdr_readargs,
				(caddr_t)&ra, xdr_rdresult,
				(caddr_t)&rr, cred);
		if (!error) {
			error = geterrno(rr.rr_status);
			check_stale_fh(error, vp);
		}
		if (!error) {
			count -= rr.rr_count;
			base += rr.rr_count;
			offset += rr.rr_count;
		}
	} while (!error && count && rr.rr_count == tsize);

	if (error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
	} else if (count) {
		bzero(bp->b_un.b_addr + bp->b_bcount - count,
		      (u_int)count);
	}

	/*
	 * The gnode may or may not be held locked by the calling
	 * process. This can cause lock ordering problems
	 * between buffers and gnodes. We free the buffer in all cases
	 * and rely on nfs_attrcache() to worry about races with
	 * other processes that provide fresher attributes than ours.
	 */
	if (blockio)	/* If doing block I/O, free the buffer */
		iodone(bp);
	if (!error) {
		nfs_attrcache(vp, &rr.rr_attr, SFLUSH);
	}
	
	return (error);
}

int binval_debug = 0;

/* returns true if attributes are strictly newer, not equal */
#define ATTR_NEW(vp, na) (((na)->na_mtime.tv_sec > (vp)->g_mtime.tv_sec) ||   \
		          ((na)->na_mtime.tv_sec == (vp)->g_mtime.tv_sec &&   \
		 	   (na)->na_mtime.tv_usec > (vp)->g_mtime.tv_usec) || \
			  ((na)->na_ctime.tv_sec > (vp)->g_ctime.tv_sec) ||   \
			  ((na)->na_ctime.tv_sec == (vp)->g_ctime.tv_sec &&   \
		 	   (na)->na_ctime.tv_usec > (vp)->g_ctime.tv_usec))

/* returns true if modify time is strictly older and change time is equal */
#define ATTR_OLD(vp, na) ((((na)->na_mtime.tv_sec < (vp)->g_mtime.tv_sec) ||  \
		           ((na)->na_mtime.tv_sec == (vp)->g_mtime.tv_sec &&  \
		 	    (na)->na_mtime.tv_usec < (vp)->g_mtime.tv_usec))&&\
			  ((na)->na_ctime.tv_sec == (vp)->g_ctime.tv_sec  && \
			   (na)->na_ctime.tv_usec == (vp)->g_ctime.tv_usec))

/* returns true if attributes are strictly equal */
#define ATTR_SAME(vp, na) ((na)->na_mtime.tv_sec == (vp)->g_mtime.tv_sec   && \
		           (na)->na_mtime.tv_usec == (vp)->g_mtime.tv_usec && \
			   (na)->na_ctime.tv_sec == (vp)->g_ctime.tv_sec   && \
		           (na)->na_ctime.tv_usec == (vp)->g_ctime.tv_usec)

int
nfs_attrcache(vp, na, fflag)
	register struct vnode *vp;
	register struct nfsfattr *na;
	enum staleflush fflag;
{
	register struct rnode *rp = vtor(vp);
	int oldsize;
	int need_lock;
	char *s;

	if (binval_debug) {
		if (glocked((struct gnode *)vp) == LK_TRUE)
			s = "locked";
		else
			s = "!locked";
		mprintf(
"attr: 0x%x %s %s %s\n ntime %d %d nsize %d\n  time %d %d size %d\n",
			vp, (vp->v_type == VREG)?"file":"!file",
			(fflag)?"flush":"!flush",
			s,
			na->na_mtime.tv_sec, na->na_mtime.tv_usec,
			na->na_size, 
			vp->g_mtime.tv_sec, vp->g_mtime.tv_usec,
			vp->g_size);
	}  

	/*
	 * Lock the gnode if it's not already locked
	 * (asynchronous I/O is one way for vp to be unlocked,
	 * see nfswrite() comments for others).
	 */
	if (glocked(vp) != LK_TRUE) {
		need_lock = 1;
		gfs_lock((struct gnode *)vp);
		/*
		 * If we lose a race to update attributes,
		 * throw these away.
		 */
		if (ATTR_OLD(vp, na)) /* strictly old, not equal */
		       goto done;
	}
	else { /* we have the gnode locked already */
		need_lock = 0;
	}

	if (binval_debug) {
		mprintf(
"cache: 0x%x %s %s %s\n ntime %d %d nsize %d\n  time %d %d size %d\n",
			vp, (vp->v_type == VREG)?"file":"!file",
			(fflag)?"flush":"!flush",
			s,
			na->na_mtime.tv_sec, na->na_mtime.tv_usec,
			na->na_size, 
			vp->g_mtime.tv_sec, vp->g_mtime.tv_usec,
			vp->g_size);
	}
        /*
	 * Check the new modify time against the old modify time
	 * to see if cached data is stale
	 */
	if (na->na_mtime.tv_sec != vp->g_mtime.tv_sec ||
	    na->na_mtime.tv_usec != vp->g_mtime.tv_usec) {
		/*
		 * The file has changed on the server.
		 *
		 * If this was unexpected (SFLUSH), and we are not actively
		 * modifying the file ourselves,
		 * then flush delayed write blocks associated with this vnode
		 * from the buffer cache and invalidate cached blocks
		 * on the free list.
		 *
		 * If this is a text file, stop running copies.
		 *
		 * If this is a directory, purge the name lookup cache.
		 */
		if (fflag == SFLUSH && !(rp->r_flags & RDIRTY)) {
			if (binval_debug) {
				mprintf("do binvalfree\n");
			}  
			binvalfree((struct gnode *)vp);
		}

		/* if it is a text, clean out running procs and vm */
		if (vp->v_flag & VTEXT) {
			xinval((struct gnode *)vp);
			cacheinval((struct gnode *)vp);
			/* binval((dev_t)NODEV, (struct gnode *)vp); */
		}

		if (vp->v_type == VDIR)		
			dnlc_purge_vp(vp);
	}

	/*
	 * Copy the new attributes into the gnode and timestamp them.
	 *
	 * Any writes caused by binvalfree() call above will be async,
	 * so those returned attributes will be discarded.
	 * If this thread didn't have the gnode locked coming in,
	 * when we got it above, we would have discarded old attributes.
	 * The only way (famous last words) we can now have old attributes
	 * is if we lost a race getting the gnode again after going to the
	 * wire in nfs_getattr().
	 *
	 * There is some weirdness with the gnode size here.  We must
	 * keep the old size if the file has unaccounted writes and
	 * the old size is greater than the new size, since these writes
	 * may make the file grow.
	 *
	 */
	if (!(ATTR_OLD(vp, na))) {
		oldsize = vp->g_size;
		nattr_to_gattr(vp, na);
		if (oldsize > vp->g_size) { /* our size > server size */
		 	/* Keep our size if there are writes outstanding. */
			if (rp->r_flags & RDIRTY) {
				vp->g_size = oldsize;
			}
		}
	}

	/* 
	 * If no attributes caching, ignore time-to-life of attributes.
	 */
	if (!(vtomi(vp)->mi_noac)) {
		nfs_ttlattr(vp, na);
	}

 done:
	if (need_lock)
		gfs_unlock((struct gnode *)vp);
}	

int
nfs_ttlattr(vp, na)
	register struct vnode *vp;
	register struct nfsfattr *na;
{
	register int delta;
	register struct rnode *rp = vtor(vp);

	/* ***It is assumed that vp is locked by caller*** */
	rp->r_nfsattrtime = *(timepick);

	/*
	 * Delta is the number of seconds that we will cache
	 * attributes of the file.  It is based on the number of seconds
	 * since the last change (i.e. files that changed recently
	 * are likely to change soon), but there is a minimum and
	 * a maximum for regular files and for directories.
	 */

	delta = (timepick->tv_sec - na->na_mtime.tv_sec) >> 4;
	if (vp->v_type == VDIR) {
		if (delta < vtomi(vp)->mi_acdirmin) {
			delta = vtomi(vp)->mi_acdirmin;
		} else if (delta > vtomi(vp)->mi_acdirmax) {
			delta = vtomi(vp)->mi_acdirmax;
		}
	} else {
		if (delta < vtomi(vp)->mi_acregmin) {
			delta = vtomi(vp)->mi_acregmin;
		} else if (delta > vtomi(vp)->mi_acregmax) {
			delta = vtomi(vp)->mi_acregmax;
		}
	}
	rp->r_nfsattrtime.tv_sec += delta;
}

int
nfs_getattr(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct rnode *rp = vtor(vp);
	int error;
	struct nfsattrstat *ns;
	int locked = 0;

	if (!vtomi(vp)->mi_noac) { /* i.e. maybe use cached attributes */
		if ((timepick->tv_sec < rp->r_nfsattrtime.tv_sec) ||
		    ((timepick->tv_sec == rp->r_nfsattrtime.tv_sec) && 
		    (timepick->tv_usec < rp->r_nfsattrtime.tv_usec))) {
				/*
				 * Use cached attributes.
				 */
				return (0);
		}
	}

	if (rp->r_flags & RDIRTY) {
		nfs_fsync(vp); /* nfs_fsync() goes directly to the wire */
	}

	kmem_alloc(ns, struct nfsattrstat *, (u_int)sizeof(*ns), KM_NFS);

	/* don't hold gnode locked while going over the wire */
	if (glocked((struct gnode *)vp) == LK_TRUE) {
		locked = 1;
		gfs_unlock((struct gnode *)vp);
	}

	error = rfscall(vtomi(vp), RFS_GETATTR, xdr_fhandle,
			(caddr_t)vtofh(vp), xdr_attrstat, (caddr_t)ns, cred);

	/* lock gnode again if it was locked coming in */
	if (locked)
		gfs_lock((struct gnode *)vp);

	if (!error) {
		error = geterrno(ns->ns_status);
		if (!error) {
			nfs_attrcache(vp, &ns->ns_attr, SFLUSH);
		}
		else {
			check_stale_fh(error, vp);
		}
	}
	kmem_free(ns, KM_NFS);
	return (error);
}

int
nfs_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct rnode *rp = vtor(vp);
	int error;
	struct nfssaargs args;
	struct nfsattrstat *ns;

	kmem_alloc(ns, struct nfsattrstat *, (u_int)sizeof(*ns), KM_NFS);
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_ctime.tv_sec != -1) || (vap->va_ctime.tv_usec != -1)) {
		error = EINVAL;
	} else {
		if (rp->r_flags & RDIRTY) {
			nfs_fsync(vp); /* NB: nfs_getattr() will unlock vp */
		}
		if (vap->va_size != (u_long) -1) {
			vp->g_size = vap->va_size;
		}
		vattr_to_sattr(vap, &args.saa_sa);
		args.saa_fh = *vtofh(vp);
		error = rfscall(vtomi(vp), RFS_SETATTR, xdr_saargs,
				(caddr_t)&args, xdr_attrstat,
				(caddr_t)ns, cred);
		if (!error) {
			error = geterrno(ns->ns_status);
			if (!error) {
				nfs_attrcache(vp, &ns->ns_attr, SFLUSH);
			}
			else {
				check_stale_fh(error, vp);
			}
		}
	}
	kmem_free(ns, KM_NFS);
	return (error);
}

int
nfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	int error;
	struct nfsrdlnres rl;

	if(vp->v_type != VLNK)
		return (ENXIO);
	kmem_alloc(rl.rl_data, char *, (u_int)NFS_MAXPATHLEN, KM_NFS);
	error =
	    rfscall(vtomi(vp), RFS_READLINK, xdr_fhandle, (caddr_t)vtofh(vp),
		    xdr_rdlnres, (caddr_t)&rl, cred);
	if (!error) {
		error = geterrno(rl.rl_status);
		if (!error) {
			error = uiomove(rl.rl_data, (int)rl.rl_count,
			    UIO_READ, uiop);
		}
		else {
			check_stale_fh(error, vp);
		}
	}
	kmem_free(rl.rl_data, KM_NFS);
	return (error);
}

int nfs_fsync_size = 16;

int
nfs_fsync(vp)
	struct vnode *vp;
{
	register struct rnode *rp = vtor(vp);
	register int offset, blksize;
	register long lastlbn;
	struct nfsattrstat *ns;
	int error;
	int need_lock = 0;

	/* Do nothing if the file is not dirty */
	if (rp->r_flags & RDIRTY) {
		/* First off, synchronize processes by locking vp */
		if (glocked((struct gnode *)vp) != LK_TRUE) {
			need_lock = 1;
			gfs_lock((struct gnode *)vp);
		}
		if (!(rp->r_flags & RDIRTY)) {
			/* someone beat us to it */
			if (need_lock) {
				gfs_unlock((struct gnode *)vp);
			} else {
				panic("nfs_fsync: lost RDIRTY");
			}
			return (rp->r_error);
		}
		/*
		 * Now that we have the gnode locked, we can reset the
		 * RDIRTY flag when we are done since no
		 * writes can occur "behind" us while we are flushing.
		 *
		 * This flag is used by nfs_attrcache() to determine if the
		 * delayed write block completions that we are flushing
		 * are allowed to update the g_size field with the server's
		 * notion of file size.
		 *
		 * If we reset the flag before we are done, there may be an
		 * orphaned delayed write block at the end of the file
		 * (the loop terminates early because the g_size field
		 * is down-sized). If this happens, then the next sync()
		 * may get left holding the bag with a buffer for a gnode
		 * that has been inactive'd.
		 */
		blksize = vtoblksz(vp);
		lastlbn = howmany((unsigned long)vp->g_size,
				  (unsigned long)blksize);
		if (lastlbn < nfs_fsync_size) {
			/* small file - synchronous writes */
			error = u.u_error; /* save state */
			for (offset = 0; offset < vp->g_size;
			     offset += blksize)
				{
					blkflush(vp->g_dev,
						 (daddr_t)(offset/DEV_BSIZE),
						 (long)blksize, vp);
				}
			/*
			 * If there were writes performed above by blkflush()
			 * they were synchronous, and geterror() posted any
			 * error directly into u.u_error. To make this
			 * policy work like the one below, remove any
			 * change to u.u_error caused by us.
			 */
			if (u.u_error) {
				rp->r_error = u.u_error;
				if (!error)
					u.u_error = 0;
			}
		}
		else {
			/* large file - asynchronous writes */
			bflush(NODEV, vp, 1); /* sync delayed writes */
			/*
			 * If there were writes performed above by bflush()
			 * they were asynchronous, and nfswrite() posted
			 * any error in rp->r_error. If an asynch write error
			 * occurred prior to this routine being called
			 * (or during the call), then all remaing dirty
			 * buffers are "tossed" in nfs_strategy() or
			 * nfswrite().
			 */
		}
		/*
		 * NB: Note that the file has been flushed.
		 * All writes to this file, including writes handed off
		 * to biods prior to or during this invocation,
		 * have been accounted for.
		 * Since the biods would pile up sleeping on gp
		 * (in nfs_attrcache()) until we unlock it, nfswrite()
		 * doesn't call nfs_attrcache() for async writes.
		 * Thus, we are not put in a race with nfs_attrcache()
		 * which could result in confused file attributes
		 * (most probably size).
		 *
		 * We ensure that when this call completes
		 * the file attributes are up to date with any
		 * writes that we have pushed out. We go directly to the
		 * wire (holding gp locked) to get current ones.
		 */
		kmem_alloc(ns, struct nfsattrstat *, (u_int)sizeof(*ns),
			   KM_NFS);

		error = rfscall(vtomi(vp), RFS_GETATTR, xdr_fhandle,
				(caddr_t)vtofh(vp), xdr_attrstat, (caddr_t)ns,
				u.u_cred);

		if (!error) {
			error = geterrno(ns->ns_status);
			if (!error) {
				nfs_attrcache(vp, &ns->ns_attr, NOFLUSH);
			}
			else {
				check_stale_fh(error, vp);
			}
		}
		kmem_free(ns, KM_NFS);

		rp->r_flags &= ~RDIRTY; /* open the gate in nfs_attrcache() */

		if (need_lock)
			gfs_unlock((struct gnode *)vp);
	}
	return (rp->r_error);
}

/*
 * Make an NFS gnode inactive.
 * Weirdness: if the file was removed while it was open it got
 * renamed (by nfs_remove) instead.  Here we remove the renamed
 * file.  Note: the gnode must be in a consistent state when this
 * routine is called, since we may block in rfscall.
 */
/*ARGSUSED*/
int
nfs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct rnode *rp = vtor(vp);
	int error;
	struct nfsdiropargs da;
	enum nfsstat status;

	if (rp->r_unlname != NULL) {
		setdiropargs(&da, rp->r_unlname, rp->r_unldvp);
		error = rfscall(vtomi(rp->r_unldvp), RFS_REMOVE,
		    xdr_diropargs, (caddr_t)&da,
		    xdr_enum, (caddr_t)&status, rp->r_unlcred);
		if (!error) {
			error = geterrno(status);
		}

		VN_RELE(rp->r_unldvp);
		kmem_free((caddr_t)rp->r_unlname, KM_NFS);
		crfree(rp->r_unlcred);

		rp->r_unldvp = NULL;
		rp->r_unlname = NULL;
		rp->r_unlcred = NULL;
	}
	if (rp->r_cred) {
		crfree(rp->r_cred);
		rp->r_cred = NULL;
	}

	return (0);
}

/*
 * Remote file system operations having to do with directory manipulation.
 */

int
nfs_lookup(dvp, nm, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfsdiropargs da;
	struct nfsdiropres *dr;

	/*
	 * Before checking dnlc, call getattr to be
	 * sure directory hasn't changed.  getattr
	 * will purge dnlc if a change has occurred.
	 */
	if (error = nfs_getattr(dvp, cred)) {
		*vpp = (struct vnode *)0;
		return (error);
	}

	*vpp = (struct vnode *) dnlc_lookup(dvp, nm, cred);
	if (*vpp) {
		nfs_lock(dvp);	/* synchronize with any other process */
		                /* that has vp locked before the check, */
		                /* where it may be temporarily unlocked */
		if (access(dvp, GEXEC)) { /* must be able to scan the dir */
			nfs_unlock(dvp);
			return (u.u_error);
		}
		nfs_unlock(dvp);
		return (0);
	}

	kmem_alloc(dr, struct nfsdiropres *, (u_int)sizeof(*dr), KM_NFS);
	setdiropargs(&da, nm, dvp);
	error = rfscall(vtomi(dvp), RFS_LOOKUP, xdr_diropargs, (caddr_t)&da,
			xdr_diropres, (caddr_t)dr, cred);
	if (!error) {
		error = geterrno(dr->dr_status);
		check_stale_fh(error, dvp);
	}
	
	if (!error) {
		*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr, dvp->v_vfsp,
				   ((struct gnode *) dvp)->g_mp);
		if(*vpp == NULL)
			error = u.u_error;
		else if (nfs_dnlc)
			dnlc_enter(dvp, nm, *vpp, cred);
	} else {
		*vpp = (struct vnode *)0;
	}
	kmem_free(dr, KM_NFS);
	return (error);
}

/*ARGSUSED*/
int
nfs_create(dvp, nm, va, exclusive, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	enum vcexcl exclusive;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct  nfsdiropres *dr;

	if (exclusive == EXCL) {
		/*
		 * This is buggy: there is a race between the lookup and the
		 * create.  We should send the exclusive flag over the wire.
		 */
		error = nfs_lookup(dvp, nm, vpp, cred);
		if (error != ENOENT) {
 			if (*vpp)
				VN_RELE(*vpp);
			return (error ? error : EEXIST);
		}
		
	}
	*vpp = (struct vnode *)0;
	kmem_alloc(dr, struct nfsdiropres *, (u_int)sizeof(*dr), KM_NFS);
	setdiropargs(&args.ca_da, nm, dvp);
 
        /*
         * This is a completely gross hack to make mknod
         * work over the wire until we can wack the protocol
         */
#define IFCHR           0020000         /* character special */
#define IFBLK           0060000         /* block special */
#define IFSOCK          0140000         /* socket */
        if (va->va_type == VCHR) {
                va->va_mode |= IFCHR;
                va->va_size = (u_long)va->va_rdev;
        } else if (va->va_type == VBLK) {
                va->va_mode |= IFBLK;
                va->va_size = (u_long)va->va_rdev;
        } else if (va->va_type == VFIFO) {
		/* xtra kludge for namedpipe */
                va->va_mode = (va->va_mode & ~GFMT) | IFCHR;
                va->va_size = (u_long)NFS_FIFO_DEV;     /* blech */
        } else if (va->va_type == VSOCK) {
                va->va_mode |= IFSOCK;
        }


	vattr_to_sattr(va, &args.ca_sa);
	dnlc_remove(dvp, nm);
	error = rfscall(vtomi(dvp), RFS_CREATE, xdr_creatargs, (caddr_t)&args,
			xdr_diropres, (caddr_t)dr, cred);
	nfsattr_inval(dvp);
	if (!error) {
		error = geterrno(dr->dr_status);
		if (error) {
			check_stale_fh(error, dvp);
		}
		else {
			*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr,
					   dvp->v_vfsp,
					   ((struct gnode *)dvp)->g_mp);
			if (*vpp != NULL) {
				((struct gnode *)*vpp)->g_size = 0;
				if (nfs_dnlc) {
					dnlc_enter(dvp, nm, *vpp, cred);
				}
			} else
				error = u.u_error;
		}
	}
	kmem_free(dr, KM_NFS);
	return (error);
}

/*
 * Weirdness: if the vnode to be removed is open
 * we rename it instead of removing it and nfs_inactive
 * will remove the new name.
 */
int
nfs_remove(dvp, vp, nm, cred)
	struct vnode *dvp;
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register struct rnode *rp = vtor(vp);
	int error;
	struct nfsdiropargs da;
	enum nfsstat status;
	char *tmpname;

	status = NFS_OK;
	/*
	 * We need to flush the name cache so we can
	 * check the real reference count on the vnode
	 */
	dnlc_purge_vp(vp);
	if (((struct gnode *)vp)->g_count > 1 && rp->r_unlname == NULL) {
		tmpname = newname(nm);
		error = nfs_rename(dvp, nm, dvp, tmpname, cred);
		if (error) {
			kmem_free(tmpname, KM_NFS);
		} else {
			VN_HOLD(dvp);
			rp->r_unldvp = dvp;
			rp->r_unlname = tmpname;
			if (rp->r_unlcred != NULL) {
				crfree(rp->r_unlcred);
			}
			crhold(cred);
			rp->r_unlcred = cred;
		}
	} else {
		setdiropargs(&da, nm, dvp);
		error = rfscall(vtomi(dvp), RFS_REMOVE, xdr_diropargs,
				(caddr_t)&da, xdr_enum, (caddr_t)&status,
				cred);
		nfsattr_inval(dvp);	/* mod time changed */
		nfsattr_inval(vp);	/* link count changed */
		check_stale_fh(error ? error : geterrno(error), dvp);

	}
	if (!error) {
		error = geterrno(status);
	}
	return (error);
}

int
nfs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	int error;
	struct nfslinkargs args;
	enum nfsstat status;

	args.la_from = *vtofh(vp);
	setdiropargs(&args.la_to, tnm, tdvp);
	error = rfscall(vtomi(vp), RFS_LINK, xdr_linkargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	nfsattr_inval(tdvp);	/* mod time changed */
	nfsattr_inval(vp);	/* link count changed */
	if (!error) {
		error = geterrno(status);
		check_stale_fh(error, vp);
		check_stale_fh(error, tdvp);
	}
	return (error);
}

int
nfs_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsrnmargs args;

	if (!bcmp(onm, ".", 2) || !bcmp(onm, "..", 3) 
		|| !bcmp(nnm, ".", 3) || !bcmp(nnm, "..", 3)) {
		error = EINVAL;
	} else {
		dnlc_remove(odvp, onm);
		dnlc_remove(ndvp, nnm);
		setdiropargs(&args.rna_from, onm, odvp);
		setdiropargs(&args.rna_to, nnm, ndvp);
		error = rfscall(vtomi(odvp), RFS_RENAME, xdr_rnmargs,
				(caddr_t)&args, xdr_enum, (caddr_t)&status,
				cred);
		nfsattr_inval(odvp);	/* mod time changed */
		nfsattr_inval(ndvp);	/* mod time changed */
		if (!error) {
			error = geterrno(status);
			check_stale_fh(error, odvp);
			check_stale_fh(error, ndvp);
		}
	}
	return (error);
}

int
nfs_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct  nfsdiropres *dr;

	kmem_alloc(dr, struct nfsdiropres *, (u_int)sizeof(*dr), KM_NFS);
	setdiropargs(&args.ca_da, nm, dvp);
	vattr_to_sattr(va, &args.ca_sa);
	dnlc_remove(dvp, nm);
	error = rfscall(vtomi(dvp), RFS_MKDIR, xdr_creatargs, (caddr_t)&args,
			xdr_diropres, (caddr_t)dr, cred);
	nfsattr_inval(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(dr->dr_status);
		check_stale_fh(error, dvp);
	}
	if (!error) {
		/*
		 * Due to a 4.0 reference port fiasco, the attributes that
		 * come back on a mkdir may not be correct. Use them only
		 * to set the vnode type in makenfsnode, then invalidate them.
		 */
		*vpp = makenfsnode(&dr->dr_fhandle, NULL, dvp->v_vfsp,
				   ((struct gnode *) dvp)->g_mp);
		if (*vpp == NULL) {
			error = u.u_error;
		}
		else {
			nfsattr_inval(*vpp); /* don't believe attributes */
			/*
			 * This getattr should be removed if if turns
			 * out that it is safe to leave uninitialized
			 * attributes. For now, an extra getattr is a small
			 * price to pay for correctness. -chet
			 */
			error = nfs_getattr(*vpp, cred);
			if (!error && nfs_dnlc) {
				dnlc_enter(dvp, nm, *vpp, cred);
			}
		}
	} else {
		*vpp = (struct vnode *)0;
	}

	kmem_free(dr, KM_NFS);
	return (error);
}

int
nfs_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsdiropargs da;

	setdiropargs(&da, nm, dvp);
	dnlc_purge_vp(dvp);
	error = rfscall(vtomi(dvp), RFS_RMDIR, xdr_diropargs, (caddr_t)&da,
			xdr_enum, (caddr_t)&status, cred);
	nfsattr_inval(dvp);
	if (!error) {
		error = geterrno(status);
		check_stale_fh(error, dvp);
	}
	return (error);
}

int
nfs_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{
	int error;
	struct nfsslargs args;
	enum nfsstat status;

	setdiropargs(&args.sla_from, lnm, dvp);
	vattr_to_sattr(tva, &args.sla_sa);
	args.sla_tnm = tnm;
	error = rfscall(vtomi(dvp), RFS_SYMLINK, xdr_slargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	nfsattr_inval(dvp);
	if (!error) {
		error = geterrno(status);
		check_stale_fh(error, dvp);
	}
	return (error);
}

/*
 * Read directory entries.
 * There are some weird things to look out for here.  The uio_offset
 * field is either 0 or it is the offset returned from a previous
 * readdir.  It is an opaque value used by the server to find the
 * correct directory block to read.  The byte count must be at least
 * vtoblksz(vp) bytes.  The count field is the number of blocks to
 * read on the server.  This is advisory only, the server may return
 * only one block's worth of entries.  Entries may be compressed on
 * the server.
 */
int
nfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	register int error = 0;
	register struct iovec *iovp;
	register unsigned count;
	register struct rnode *rp = vtor(vp);
	struct nfsrddirargs rda;
	struct nfsrddirres  rd;


	nfs_lock(vp);	/* synchronize with any other process */
			/* that has vp locked before the access() check, */
			/* where it may be temporarily unlocked */
	if (access(vp, GREAD)) {	/* must be able to read the dir */
		nfs_unlock(vp);
		return;
	}
	nfs_unlock(vp);

	if ((rp->r_flags & REOF) &&
	    (vp->g_size == (u_long)uiop->uio_offset)) {
		return;
        }
	iovp = uiop->uio_iov;
	count = iovp->iov_len;

	/*
	 * XXX We should do some kind of test for count >= DEV_BSIZE
	 */
	if (uiop->uio_iovcnt != 1) {
		u.u_error = EINVAL;
		return;
	}
	count = MIN(count, vtomi(vp)->mi_tsize);
	rda.rda_count = count;
	rda.rda_offset = uiop->uio_offset;
	rda.rda_fh = *vtofh(vp);
	rd.rd_size = count;
	kmem_alloc(rd.rd_entries, struct direct *, (u_int)count, KM_NFS);

	error = rfscall(vtomi(vp), RFS_READDIR, xdr_rddirargs, (caddr_t)&rda,
			xdr_getrddirres, (caddr_t)&rd, cred);
	if (!error) {
		error = geterrno(rd.rd_status);
		check_stale_fh(error, vp);
	}
	if (!error) {
		/*
		 * move dir entries to user land
		 */
		if (rd.rd_size) {
			error = uiomove((caddr_t)rd.rd_entries,
					(int)rd.rd_size, UIO_READ, uiop);
			rda.rda_offset = rd.rd_offset;
			uiop->uio_offset = rd.rd_offset;
		}
		if (rd.rd_eof) {
			rp->r_flags |= REOF;
			/* removed for VMS server compatibility, */
			/* not sure if it was right anyway */
			/* vp->g_size = uiop->uio_offset; */
		}
	}
	kmem_free(rd.rd_entries, KM_NFS);

	if (error)
		u.u_error = error;
	return;
}

/*
 * GFS operation for getting block maps
 */

int
nfs_gbmap(vp, vbn, rw, size, sync)
	register struct vnode *vp;	/* gnode */
	register daddr_t vbn;		/* virtual block */
	int rw, size, sync;		/* ignore for nfs */
{
	daddr_t lbn;
	nfs_bmap(vp, vbn, &lbn);
	return((int)lbn);
}

/*
 * Convert from file system blocks to device blocks
 */
int
nfs_bmap(vp, bn, bnp)
	struct vnode *vp;	/* file's vnode */
	daddr_t bn;		/* fs block number */
	daddr_t *bnp;		/* RETURN device block number */
{
	int bsize;		/* server's block size in bytes */

	if (bnp) {
		bsize = vtoblksz(vp);
		*bnp = bn * (bsize / DEV_BSIZE);
	}
	return (0);
}

struct buf *async_bufhead;
int async_daemon_count;	/* number of nfs_biod() processes available for work */

#include "../h/vm.h"
#include "../h/map.h"
#include "../machine/pte.h"

int async_buf_count;	/* number of buffers on nfs_biod() work list */

int
nfs_strategy(bp)
	register struct buf *bp;
{
	register struct buf *bp1;
	register struct gnode *gp = bp->b_gp;
	register struct rnode *rp = vtor((struct vnode *)gp);
	
	/*
	 * If there was an asynchronous write error on this gnode
	 * then we just return the old error code. This continues
	 * until the gnode goes away (zero ref count). We do this because
	 * there can be many procs writing this gnode.
	 */
	if (rp->r_error) {
		bp->b_error = rp->r_error;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	if (bp->b_flags & B_PHYS) {
		register int npte;
		register int n;
		register long a;
		register struct pte *pte, *kpte;
		caddr_t va;
		int o;
		caddr_t saddr;
  		struct proc *p;
  		unsigned v;

 		if (!(bp->b_flags & B_PAGET)) {
			int user_addr = 0;
 			/*
 			 * Buffer's data is in userland, or in some other
 			 * currently inaccessable place. We get a hunk of
 			 * kernel address space and map it in.
 			 */
 			v = btop(bp->b_un.b_addr);
 			o = (int)bp->b_un.b_addr & PGOFSET;
 			npte = btoc(bp->b_bcount + o);
 			p = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;

                        if (bp->b_flags & B_UAREA)      /* [ dgg001 ] */
                               pte = &p->p_addr[v];
                        else if ((bp->b_flags & B_SMEM)  &&
                                ((bp->b_flags & B_DIRTY) == 0))
                                        pte = ((struct smem *)p)->sm_ptaddr
						+ v;
			else {
				pte = (struct pte *)0;
				user_addr++;
			}

 			while ((a = rmalloc(kernelmap, (long)clrnd(npte))) 
			       == NULL) {
 				kmapwnt++;
 				sleep((caddr_t)kernelmap, PSWP+4);
 			}
 			kpte = &Usrptmap[a];
 			for (n = npte; n--; kpte++, pte++, v++) {
			   	if (user_addr &&
			       	    (((int)pte & PGOFSET) < CLSIZE*sizeof(struct pte)	
				    || pte->pg_pfnum == 0))
				   	pte = vtopte(p, v);
			   	if (pte->pg_pfnum == 0)
				   	panic("nfs zero uentry");
#ifdef mips
				*(int *)kpte = (*(int *)pte & PG_PFNUM);
#endif mips
#ifdef vax
				*(int *)kpte = PG_NOACC | 
					(*(int *)pte & PG_PFNUM);
#endif vax
			}
 			va = (caddr_t)kmxtob(a);
#ifdef mips
			vmaccess(&Usrptmap[a], va, npte, DO_CACHE);
#endif mips
#ifdef vax
			vmaccess(&Usrptmap[a], va, npte);
#endif vax
 			saddr = bp->b_un.b_addr;
 			bp->b_un.b_addr = va + o;
		}
		/*
		 * do the io
		 */
		do_bio(bp);
		/*
		 * Release kernel maps
		 */
 		if (!(bp->b_flags & B_PAGET)) {
 			bp->b_un.b_addr = saddr;
 			kpte = &Usrptmap[a];
 			for (n = npte; n-- ; kpte++)
#ifdef mips
				*(int *)kpte = 0;
#endif  mips
#ifdef vax
				*(int *)kpte = PG_NOACC;
#endif vax
 			rmfree(kernelmap, (long)clrnd(npte), a);
 		}
	} else if (async_daemon_count && (bp->b_flags & B_ASYNC) &&
		   async_buf_count < async_daemon_count) {
		/*
		 * We never allow more buffers onto async_bufhead than
		 * there are biods waiting to process them. Since
		 * biods may need to go through nfs_attrcache() (which
		 * locks gp), and since nfs_fsync() holds gp locked for
		 * its duration, if a buffer needed to complete nfs_fsync()
		 * is placed where we cannot guarantee processing
		 * up to iodone(), then it's deadlock time. This may be
		 * viewed as conservative, but if there is no biod,
		 * then why wait to start I/O?
		 */
		smp_lock(&lk_nfsbiod, LK_RETRY);
		if (!async_daemon_count ||
		    async_buf_count >= async_daemon_count) {
			smp_unlock(&lk_nfsbiod);
			do_bio(bp);
		} else {
			if (async_bufhead) {
				bp1 = async_bufhead;
				while (bp1->b_actf) {
					bp1 = bp1->b_actf;
				}
				bp1->b_actf = bp;
			} else {
				async_bufhead = bp;
			}
			gref(gp);
			bp->b_actf = NULL;
			++async_buf_count;
			smp_unlock(&lk_nfsbiod);
			wakeup_type((caddr_t) &async_bufhead, WAKE_ONE);
		}
	} else {
		do_bio(bp);
	}
}

int
nfs_biod()
{
	register struct buf *bp;
	register struct gnode *gp;
	register struct proc *p = u.u_procp;

	if (setjmp(&u.u_qsave)) {
		/* Protect counters */
		smp_lock(&lk_nfsbiod, LK_RETRY);
		async_daemon_count--;
		if (async_buf_count > async_daemon_count) {
			/*
			 * We must follow rules described in
			 * nfs_strategy(), but we lost a race with a
			 * new buffer. Note there are too many by
			 * at most one. If this i/o doesn't complete
			 * it will take two signals to get us!
			 */
			bp = async_bufhead;
			async_bufhead = bp->b_actf;
			--async_buf_count;
			smp_unlock(&lk_nfsbiod);
			gp = bp->b_gp;
			do_bio(bp);
			grele(gp);
			exit (0);
		}
		smp_unlock(&lk_nfsbiod);
		exit(0);
	}

	for (;;) {
		/* Protect biod buffer list */
		smp_lock(&lk_nfsbiod, LK_RETRY);
		async_daemon_count++;
		while (async_bufhead == NULL) {
			sleep_unlock((caddr_t)&async_bufhead, PZERO + 1,
				     &lk_nfsbiod);
			smp_lock(&lk_nfsbiod, LK_RETRY);
			if (async_bufhead == NULL)
				biod_has_no_work++;
			else
				biod_has_work++;
		}
		async_daemon_count--;
		bp = async_bufhead;
		async_bufhead = bp->b_actf;
		--async_buf_count;
		smp_unlock(&lk_nfsbiod);
		gp = bp->b_gp;
		do_bio(bp);
		grele(gp);
		/* See if this I/O was interrupted */
		if (p->p_cursig) {
		   mprintf("NFS biod (pid %d) exiting on signal %d\n",
			   u.u_procp->p_pid, p->p_cursig);
			exit (0);
		}
	}
}

int
do_bio(bp)
	register struct buf *bp;
{
	register struct gnode *gp = bp->b_gp;
	register struct rnode *rp = vtor((struct vnode *)gp);

	/*
	 * Ref the gnode to handle sync/close races that might
	 * drop the gnode ref count to zero
	 */
	gref(gp); /* all NFS buffers hold refs */

	if ((bp->b_flags & B_READ) == B_READ) {
		nfsread(bp, (struct vnode *)(gp),
			bp->b_un.b_addr,
			bp->b_blkno * DEV_BSIZE,
			(int)bp->b_bcount,
			rp->r_cred, NFS_BLOCKIO);
	} else {
		nfswrite(bp, (struct vnode *)(gp),
			 bp->b_un.b_addr,
			 bp->b_blkno * DEV_BSIZE,
			 bp->b_bcount - bp->b_resid,
			 rp->r_cred, NFS_BLOCKIO);
	}

	grele(gp); /* all NFS buffers hold refs */

}

int
nfs_badop()
{
	panic("nfs_badop");
}

/*
 * Remote Record-locking requests are passed to the local Lock-Manager daemon
 * to be passed along to the server Lock-Manager daemon.
 */

int
nfs_rlock(gp, ld, cmd, fp)
	struct gnode *gp;
	struct flock *ld;
	int cmd;
	struct file *fp;
{
	register struct rnode *rp = vtor((struct vnode *)gp);
	lockhandle_t lh;
	extern int kernel_locking;	/* Sys-V locking system: 1 = kernel */
					/*			 0 = daemon */

	/*
	 *	ULTRIX supports both kernel based and daemon based 
	 *	region locking; however, only daemon based locking
	 *	supports NFS file locking.
	 *	If daemon based locking has not been enabled, then
	 *	kernel locking is enabled, and NFS lock requests are 
	 *	not permissible.
	 *
	 *	Note: this routine is called when an attempt is made to 
	 *	lock a remote file.
	 */

	if (kernel_locking) {
		return (EACCES);
	}

#ifndef lint
	if (sizeof (lh.lh_id) != sizeof (fhandle_t))
		panic("fhandle and lockhandle-id are not the same size!");
#endif

	/*
	 * If we are setting a lock, mark the rnode NOCACHE so the buffer
	 * cache does not give inconsistent results on locked files shared
	 * between clients. The NOCACHE flag is never turned off as long
	 * as the gnode is active because it is hard to figure out when 
	 * the last lock is gone.
	 */

	if (((rp->r_flags & RNOCACHE) == 0) &&
	    (ld->l_type != F_UNLCK) && (cmd != F_GETLK)) {
		rp->r_flags |= RNOCACHE;
		binvalfree(gp);
	}

	lh.lh_gp = gp;
	lh.lh_servername = vtomi((struct vnode *)gp)->mi_hostname;
	bcopy((caddr_t)vtofh((struct vnode *)gp), (caddr_t)&lh.lh_id,
	      sizeof(fhandle_t));

	return (klm_lockctl(&lh, ld, cmd, fp->f_cred));
}
