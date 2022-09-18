#ifndef lint
static	char	*sccsid = "@(#)nfs_server.c	4.5	(ULTRIX)	2/28/91";
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 * 28 Feb 91 -- dws
 *	Cleanup for server view of fhandle.
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 *  9 Nov 90 -- prs
 *	Don't call VOP_BREAD unless vnode refers to a
 *	UFS file in rfs_read().
 *
 * 27 Aug 90 -- chet
 *	Changed locking protocol for rfs_link.
 *
 * 25 Aug 90 -- chet
 *	Fixed remove's use of duplicate request cache.
 *	Move svckudp_dupdone() call  in rfs_dispatch().
 *
 * 12 Jan 90 -- prs
 *	Moved lockinit of nfsargs to nfs_subr.
 *
 * 14 Nov 89 -- prs
 *	Fixed null pointer reference problem in rfs_dispatch().
 *
 * 28 Feb 89 -- chet
 *      Fix use of duplicate request cache for rfs_remove and
 *      rfs_symlink
 *
 * 22 Aug 88 -- condylis for cb
 *	Added gfs_unlock in rfs_mkdir after VOP_LOOKUP call.
 *
 * 17 Aug 88 -- chet
 *	Set ruid and rgid fields in server daemon's u area
 *
 * 13 Mar 88 -- chet
 *	Get rid of nobody variable and change rdonly macro.
 *	Fhtovp() now does root mapping and exported fs check.
 *
 * 4 Feb 88 -- chet for cb
 *	add fifo mode fix to rfs_create().
 *
 * 26 Jan 88 -- chet
 *	Modified trailer portion of stale fhandle messages
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 * 11 Nov 87 -- chet
 *	Added defensive check for non-directories to rfs_lookup().
 *
 * 14 Jul 87 -- cb
 *	Changed mknod interface.
 *
 * 19 May 87 -- logcher
 *	Changed soreserve back from 24000 to 65000 since SB_MAX 
 *	is now 65K
 *
 * 13 May 87 -- williams 
 *	Changes soreserve from 65000 to 24000 because the max socket
 * 	size has been dropped from 64k to 32k.   This should be set
 *	back to 65000 once the socket code is debugged.
 *
 * 11 May 87 -- chet
 *	Changed server action routines to use new server duplicate
 *	transaction cache interface routines.
 *
 * 05-May-87 -- logcher
 *	Changed usage of m_exflags to m_flags.
 *
 * 10-Apr-87 -- logcher
 *	Added Larry Palmer's to modify soreserve_size
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added export read-only to
 *	rdonly macro, added check in rfs_lookup to not give 
 *	out fhandle if NOFH flag set, added code to allow mknod
 *	for non-regular files 
 */

#include "../h/param.h"
#include "../h/mount.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/socketvar.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/gnode.h"
#include "../h/mbuf.h"
#include "../h/fs_types.h"
#include "../h/kernel.h"
#include "../h/cpudata.h"
#include "../net/netinet/in.h"
#include "../net/rpc/types.h"
#include "../net/rpc/auth.h"
#include "../net/rpc/auth_unix.h"
#include "../net/rpc/svc.h"
#include "../net/rpc/xdr.h"
#define  NFSSERVER
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/vfs.h"
#include "../nfs/vnode.h"
#include "../ufs/fs.h"	/* WARNING: BEWARE of future UFS define */

extern char *rfsnames[];
struct nfs_dupstat {
	int writes;
	int creates;
	int removes;
	int links;
	int symlinks;
	int mkdirs;
	int rmdirs;
	int renames;
	int setattrs;
	int throwaways;
} nfs_dupstats;
int nfs_n_daemons = 0;	/* number of running NFS daemon processes */

/*
 * rpc service program version range supported
 */
#define	VERSIONMIN	2
#define	VERSIONMAX	2

/*
 * Returns true iff filesystem for a given fsid is 
 *	exported read-only and mounted read-only
 *
 */

#define rdonly(gp, xpd) (((xpd)->x_flags & M_EXRONLY) || ISREADONLY(gp->g_mp))

struct vnode	*fhtovp();
struct file	*getsock();
void		svcerr_progvers();
void		rfs_dispatch();

extern struct export *exported;

struct lock_t lk_nfsargs;
extern struct lock_t lk_gnode;
extern struct lock_t lk_nfsstat;
extern struct lock_t lk_nfsbiod;	/* used for count of biods and nfsds */

struct {
        int     ncalls;         /* number of calls received */
        int     nbadcalls;      /* calls that failed */
        int     reqs[32];       /* count for each request */
} svstat;
int soreserve_size = 65000;

/*
 * NFS Server system call.
 * Does all of the work of running an NFS server.
 * sock is the fd of an open UDP socket.
 */
nfs_svc()
{
	struct a {
		int     sock;
	} *uap = (struct a *)u.u_ap;
	struct gnode	*rdir;
	struct gnode	*cdir;
	struct socket   *so;
	struct file	*fp;
	SVCXPRT *xprt;
	u_long	vers;
	int	error;
 
	fp = getsock(uap->sock);
	if (fp == 0) {
		u.u_error = EBADF;
		return;
	}

	so = (struct socket *)fp->f_data;

	/*
	 *	Allocate extra space for this socket, to minimize
	 *	lost requests for NFS.  We don't want everyone to do
	 *	this, so do it here, rather than in udp_usrreq().
	 */

	error = soreserve(so, soreserve_size, 
		soreserve_size + 2 *(sizeof(struct sockaddr)));
	if (error)	{
		u.u_error = error;
		return;
	}
	/*
	 * Be sure that rdir (the server's root vnode) is set.
	 * Save the current directory and restore it again when
	 * the call terminates.  rfs_lookup uses u.u_cdir for lookupname.
	 */
	rdir = u.u_rdir;
	cdir = u.u_cdir;
	if (rdir == (struct gnode *)0) {
		u.u_rdir = u.u_cdir;
	}
	xprt = svckudp_create(so, NFS_PORT);
	smp_lock(&lk_nfsbiod, LK_RETRY); 	/* same lock used for biod count	*/
	++nfs_n_daemons; /* increment running server count */
	smp_unlock(&lk_nfsbiod);
	for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
		(void) svc_register(xprt, NFS_PROGRAM, vers, rfs_dispatch,
		    FALSE);
	}
	if (setjmp(&u.u_qsave)) {
		/* shut the service down when the last daemon exits */
		smp_lock(&lk_nfsbiod, LK_RETRY);
		if (--nfs_n_daemons <= 0) {
			for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
				svc_unregister(NFS_PROGRAM, vers);
			}
			smp_unlock(&lk_nfsbiod);
			SVC_DESTROY(xprt);
		} else 
			smp_unlock(&lk_nfsbiod);
		u.u_error = EINTR;
	} else {
		svc_run(xprt);  /* never returns */
	}
	u.u_rdir = rdir;
	u.u_cdir = cdir;
}


/*
 * Get file handle system call.
 * Takes open file descriptor and returns a file handle for it.
 */
nfs_getfh()
{
	register struct a {
		int	fdes1;
		int	fdes2;
		fhandle_t	*fhp;
	} *uap = (struct a*)u.u_ap;
	register struct file *fp1, *fp2;
	fhandle_t fh;
	struct vnode *vp1, *vp2;

	if (!suser()) {
		return;
	}
	fp1 = getf(uap->fdes1);
	if (fp1 == NULL) {
		return;
	}
	fp2 = getf(uap->fdes2);
	if (fp2 == NULL) {
		return;
	}
	vp1 = (struct vnode *)fp1->f_data;
	vp2 = (struct vnode *)fp2->f_data;
	gfs_lock(vp1);
	if (vp1 != vp2)
		gfs_lock(vp2);
	u.u_error = makefh(&fh, vp1, vp2->g_number, vp2->g_gennum);
	gfs_unlock(vp1);
	if (vp1 != vp2)
		gfs_unlock(vp2);
	if (!u.u_error) {
		u.u_error =
		    copyout((caddr_t)&fh, (caddr_t)uap->fhp, sizeof(fh));
	}
	return;
}

	
/*
 * These are the interface routines for the server side of the
 * Networked File System.  See the NFS protocol specification
 * for a description of this interface.
 */

/*
 * Get file attributes.
 * Returns the current attributes of the file with the given fhandle.
 */
int
rfs_getattr(fhp, ns, req, xprt, xpd)
	fhandle_t *fhp;
	register struct nfsattrstat *ns;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *vp;
	struct vattr va;

	vp = fhtovp(fhp, xprt, RFS_GETATTR, xpd);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return(ESTALE);
	}
	error = VOP_GETATTR(vp, &va, u.u_cred);
	if (!error) {
		vattr_to_nattr(&va, &ns->ns_attr);
	}
	ns->ns_status = puterrno(error);
	gput(vp);
	return(0);
}

/*
 * Set file attributes.
 * Sets the attributes of the file with the given fhandle.  Returns
 * the new attributes.
 */
int
rfs_setattr(args, ns, req, xprt, xpd)
	struct nfssaargs *args;
	register struct nfsattrstat *ns;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *vp;
	struct vattr va;
	struct timeval duptime;
	int dupmark;

	vp = fhtovp(&args->saa_fh, xprt, RFS_SETATTR, xpd);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(vp,xpd)) {
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == vp->g_ctime.tv_sec &&
			duptime.tv_usec == vp->g_ctime.tv_usec)
				++nfs_dupstats.setattrs;
		else {
			sattr_to_vattr(&args->saa_sa, &va);
			error = VOP_SETATTR(vp, &va, u.u_cred);
		}

		if (!error) {
			error = VOP_GETATTR(vp, &va, u.u_cred);
			if (!error) {
				vattr_to_nattr(&va, &ns->ns_attr);
			}
		}

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, vp->g_ctime, DUP_DONE);
		}
	}
	ns->ns_status = puterrno(error);
	gput(vp);
	return (0);
}

/*
 * Directory lookup.
 * Returns an fhandle and file attributes for file name in a directory.
 */
int
rfs_lookup(da, dr, req, xprt, xpd)
	struct nfsdiropargs *da;
	register struct  nfsdiropres *dr;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *dvp;
	struct vnode *vp;
	struct vattr va;

	dvp = fhtovp(&da->da_fhandle, xprt, RFS_LOOKUP, xpd);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return(ESTALE);
	}

	/* Make sure that we are positioned at a directory */
	if ((dvp->g_mode & GFMT) != GFDIR) {
 		dr->dr_status = NFSERR_NOTDIR;
		gput(dvp);
 		return(0);
	}

	/*
 	 * Check if NOFH flag set from /etc/exports 
	 * and giving out root inode
 	 */
/* 	if ((dvp->g_mp->m_flags & M_NOFH) && dvp->g_number == ROOTINO) {
 		dr->dr_status = NFSERR_PERM;
		gput(dvp);
 		return(0);
 	}
*/ 	/*
	 * do lookup.
	 */
	error = VOP_LOOKUP(dvp, da->da_name, &vp, u.u_cred);
	gfs_unlock(dvp);
	if (error) {
		vp = (struct vnode *)0;
	} else {
		/*
 	 	 * Check if NOFH flag set from /etc/exports 
		 * and giving out root inode
 	 	 */
/*		if ((vp->g_mp->m_flags & M_NOFH) && vp->g_number == ROOTINO) {
			dr->dr_status = NFSERR_PERM;
			grele(dvp);
			grele(vp);
			return(0);
		}
*/		gfs_lock(vp);

		error = VOP_GETATTR(vp, &va, u.u_cred);
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, vp, 
				       da->da_fhandle.fh_eno,
				       da->da_fhandle.fh_egen);
		}
		gfs_unlock(vp);
	}
	dr->dr_status = puterrno(error);
	grele(dvp);
	if (vp) {
		grele(vp);
	}
	return(0);
}

/*
 * Read symbolic link.
 * Returns the string in the symbolic link at the given fhandle.
 */
int
rfs_readlink(fhp, rl, req, xprt, xpd)
	fhandle_t *fhp;
	register struct nfsrdlnres *rl;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	struct iovec iov;
	struct uio uio;
	struct vnode *vp;

	vp = fhtovp(fhp, xprt, RFS_READLINK, xpd);
	if (vp == NULL) {
		rl->rl_status = NFSERR_STALE;
		return(ESTALE);
	}

	/*
	 * Allocate data for pathname.  This will be freed by rfs_rlfree.
	 */
	kmem_alloc(rl->rl_data, char *, MAXPATHLEN, KM_NFS);

	/*
	 * Set up io vector to read sym link data
	 */
	iov.iov_base = rl->rl_data;
	iov.iov_len = NFS_MAXPATHLEN;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = 0;
	uio.uio_resid = NFS_MAXPATHLEN;

	/*
	 * read link
	 */
	error = VOP_READLINK(vp, &uio, u.u_cred);

	/*
	 * Clean up
	 */
	if (error) {	
		kmem_free(rl->rl_data, KM_NFS);
		rl->rl_count = 0;
		rl->rl_data = NULL;
	} else {
		rl->rl_count = NFS_MAXPATHLEN - uio.uio_resid;
	}
	rl->rl_status = puterrno(error);
	gput(vp);
	return(0);
}

/*
 * Free data allocated by rfs_readlink
 */
rfs_rlfree(rl)
	struct nfsrdlnres *rl;
{
	if (rl->rl_data) {
		kmem_free(rl->rl_data, KM_NFS);
	}
}

/*
 * Read data.
 * Returns some data read from the file at the given fhandle.
 */

int
rfs_read(ra, rr, req, xprt, xpd)
	struct nfsreadargs *ra;
	register struct nfsrdresult *rr;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	struct vnode *vp;
	struct iovec iov;
	struct uio uio;
	int offset, fsbsize;
	struct buf *bp;

	rr->rr_data = NULL;
	rr->rr_count = 0;
	vp = fhtovp(&ra->ra_fhandle, xprt, RFS_READ, xpd);
	if (vp == NULL) {
		rr->rr_status = NFSERR_STALE;
		return(ESTALE);
	}
	/*
	 * This is a kludge to allow reading of files created
	 * with no read permission.  The owner of the file
	 * is always allowed to read it.
	 */
	if (u.u_uid != vp->g_uid) {
		error = VOP_ACCESS(vp, VREAD, u.u_cred);
		if (error) {
			/*
			 * Exec is the same as read over the net because
			 * of demand loading.
			 */
			error = VOP_ACCESS(vp, VEXEC, u.u_cred);
		}
		if (error) {
			goto bad;
		}
	}

	/*
	 * Check whether we can do this with bread, which would
	 * save the copy through the uio.
	 */
	fsbsize = ((struct gnode *)vp)->g_mp->m_fs_data->fd_otsize;
	offset = ra->ra_offset % fsbsize;
	/*
	 * Don't call VOP_BREAD unless UFS file system.
	 * VOP_BREAD is tailored to UFS.
	 */
	if ((((struct gnode *)vp)->g_mp->m_fstype == GT_ULTRIX) && 
	    (offset + ra->ra_count <= fsbsize) && (offset % 4 == 0)) {
		if (ra->ra_offset >= vp->g_size) {
			rr->rr_count = 0;
			gattr_to_nattr(vp, &rr->rr_attr);
			rr->rr_attr.na_blocksize = fsbsize;
			error = 0;
			goto done;
		}
		error = VOP_BREAD(vp, ra->ra_offset / fsbsize, &bp);
		if (error == 0) {
			rr->rr_data = bp->b_un.b_addr + offset;
			rr->rr_count = min(
			    (u_int)(vp->g_size - ra->ra_offset),
			    (u_int)ra->ra_count);
			rr->rr_bp = bp;
			rr->rr_vp = vp;
			gref(vp);
			gattr_to_nattr(vp, &rr->rr_attr);
			rr->rr_attr.na_blocksize = fsbsize;
			goto done;
		} else {
			printf("nfs read: failed, errno %d\n", error);
		}
	}

	rr->rr_bp = (struct buf *) 0;
			
	/*
	 * Allocate space for data.  This will be freed by xdr_rdresult
	 * when it is called with x_op = XDR_FREE.
	 */
	kmem_alloc(rr->rr_data, char *, ra->ra_count, KM_NFS);
	rr->rr_bufallocaddr = rr->rr_data;
	rr->rr_bufallocsize = ra->ra_count;

	/*
	 * Set up io vector
	 */
	iov.iov_base = rr->rr_data;
	iov.iov_len = ra->ra_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = ra->ra_offset;
	uio.uio_resid = ra->ra_count;
	/*
	 * for now we assume no append mode and ignore
	 * totcount (read ahead)
	 */

	error = VOP_RDWR(vp, &uio, UIO_READ, IO_SYNC, u.u_cred);
	if (error) {
		goto bad;
	}
	gattr_to_nattr(vp, &rr->rr_attr);
	rr->rr_attr.na_blocksize = fsbsize;
	rr->rr_count = ra->ra_count - uio.uio_resid;
bad:
	if (error && rr->rr_data != NULL) {
		kmem_free(rr->rr_bufallocaddr, KM_NFS);
		rr->rr_data = NULL;
		rr->rr_count = 0;
	}
done:
	rr->rr_status = puterrno(error);
	gput(vp);
	return(0);
}

/*
 * Free data allocated by rfs_read.
 */
rfs_rdfree(rr)
	struct nfsrdresult *rr;
{
	if (rr->rr_bp == 0 && rr->rr_data) {
		kmem_free(rr->rr_bufallocaddr, KM_NFS);
	}
}

/*
 * Write data to file.
 * Returns attributes of a file after writing some data to it.
 */
int
rfs_write(wa, ns, req, xprt, xpd)
	struct nfswriteargs *wa;
	struct nfsattrstat *ns;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *vp;
	struct vattr va;
	struct iovec iov;
	struct uio uio;
	struct timeval duptime;
	int dupmark;

	vp = fhtovp(&wa->wa_fhandle, xprt, RFS_WRITE, xpd);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(vp, xpd)) {
		error = EROFS;
	} else {

		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == vp->g_mtime.tv_sec &&
			duptime.tv_usec == vp->g_mtime.tv_usec) {
			++nfs_dupstats.writes;
		}
		else {
			if (u.u_uid != vp->g_uid) {
				/*
				 * This is a kludge to allow writes of
				 * files created with read only permission.
				 * The owner of the file
				 * is always allowed to write it.
				 */
				error = VOP_ACCESS(vp, VWRITE, u.u_cred);
			}
			if (!error) {
				iov.iov_base = wa->wa_data;
				iov.iov_len = wa->wa_count;
				uio.uio_iov = &iov;
				uio.uio_iovcnt = 1;
				uio.uio_segflg = UIO_SYSSPACE;
				uio.uio_offset = wa->wa_offset;
				uio.uio_resid = wa->wa_count;
				/*
				 * for now we assume no append mode
				 */
				error = VOP_RDWR(vp, &uio, UIO_WRITE,
					IO_SYNC, u.u_cred);
			}
		}

		if (!error) {
			/*
			 * Get attributes again so we send the latest mod
			 * time to the client side for his cache.
			 */
			error = VOP_GETATTR(vp, &va, u.u_cred);
		}

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			vattr_to_nattr(&va, &ns->ns_attr);
			svckudp_dupsave(req, vp->g_mtime, DUP_DONE);
		}
	}

	ns->ns_status = puterrno(error);
	gput(vp);
	return(0);
}

/*
 * Create a file.
 * Creates a file with given attributes and returns those attributes
 * and an fhandle for the new file.
 */
int
rfs_create(args, dr, req, xprt, xpd)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	struct vattr va;
	struct vnode *vp;
	register struct vnode *dvp;
	struct timeval duptime;
	int dupmark;

	sattr_to_vattr(&args->ca_sa, &va);

        /*
         * This is a completely gross hack to make mknod
         * work over the wire until we can wack the protocol
         */
#define IFMT            0170000         /* type of file */
#define IFCHR           0020000         /* character special */
#define IFBLK           0060000         /* block special */
#define IFSOCK          0140000         /* socket */
        if ((va.va_mode & IFMT) == IFCHR) {
                va.va_type = VCHR;
                if (va.va_size == (u_long)NFS_FIFO_DEV) {
                        va.va_type = VFIFO; /* xtra kludge for namedpipe */
			va.va_mode = (va.va_mode & ~GFMT) | GFPORT;
		} else 
                        va.va_rdev = (dev_t)va.va_size;
                va.va_size = 0;
 
        } else if ((va.va_mode & IFMT) == IFBLK) {
                va.va_type = VBLK;
                va.va_rdev = (dev_t)va.va_size;
                va.va_size = 0;
 
        } else if ((va.va_mode & IFMT) == IFSOCK) {
                va.va_type = VSOCK;
        } else {
                va.va_type = VREG;
		va.va_mode = (va.va_mode & ~GFMT) | GFREG;
        }
 
	/*
	 * XXX Should get exclusive flag and use it.
	 */
	dvp = fhtovp(&args->ca_da.da_fhandle, xprt, RFS_CREATE, xpd);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(dvp, xpd)) {
		gfs_unlock(dvp);
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
		    duptime.tv_sec == dvp->g_ctime.tv_sec &&
		    duptime.tv_usec == dvp->g_ctime.tv_usec) {
			++nfs_dupstats.creates;
			error = VOP_LOOKUP(dvp, args->ca_da.da_name,
				&vp, u.u_cred);
			if (!error) {
				gfs_lock(vp);
			}
			gfs_unlock(dvp);
		}
		else {
			error = VOP_CREATE(dvp, args->ca_da.da_name,
				&va, NONEXCL, VWRITE, &vp, u.u_cred);
		}

		if (!error) {
			error = VOP_GETATTR(vp, &va, u.u_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
				error = makefh(&dr->dr_fhandle, vp, 
					       args->ca_da.da_fhandle.fh_eno, 
					       args->ca_da.da_fhandle.fh_egen);
			}
			gput(vp);
		}

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, dvp->g_ctime, DUP_DONE);
		}
	}
	dr->dr_status = puterrno(error);
	grele(dvp);
	return(0);
}

/*
 * Remove a file.
 * Remove named file from parent directory.
 */
int
rfs_remove(da, status, req, xprt, xpd)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *vp;
	struct timeval duptime;
	int dupmark;

	vp = fhtovp(&da->da_fhandle, xprt, RFS_REMOVE, xpd);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(vp, xpd)) {
		gfs_unlock(vp);
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == vp->g_ctime.tv_sec &&
			duptime.tv_usec == vp->g_ctime.tv_usec) {
			++nfs_dupstats.removes;
			gfs_unlock(vp);
		}
		else
			error = VOP_REMOVE(vp, da->da_name, u.u_cred);

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, vp->g_ctime, DUP_DONE);
		}
	}
	*status = puterrno(error);
	grele(vp);
	return(0);
}

/*
 * rename a file
 * Give a file (from) a new name (to).
 */
int
rfs_rename(args, status, req, xprt, xpd)
	struct nfsrnmargs *args;
	enum nfsstat *status; 
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *fromvp;
	register struct vnode *tovp;
	struct timeval duptime;
	int dupmark;

	fromvp = fhtovp(&args->rna_from.da_fhandle, xprt, RFS_RENAME, xpd);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(fromvp, xpd)) {
		error = EROFS;
		gfs_unlock(fromvp);
		goto fromerr;
	}
	if ((args->rna_from.da_fhandle.fh_fsid ==
	     args->rna_to.da_fhandle.fh_fsid) &&
	    (args->rna_from.da_fhandle.fh_fno ==
	     args->rna_to.da_fhandle.fh_fno) &&
	    (args->rna_from.da_fhandle.fh_fgen ==
	     args->rna_to.da_fhandle.fh_fgen))
		tovp=fromvp;
	else
		tovp = fhtovp(&args->rna_to.da_fhandle, xprt, RFS_RENAME, xpd);

	if (tovp == NULL) {
		*status = NFSERR_STALE;
		gput(fromvp);
		return(ESTALE);
	}
	if (rdonly(tovp, xpd)) {
		gfs_unlock(fromvp);
		gfs_unlock(tovp);
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == fromvp->g_ctime.tv_sec &&
			duptime.tv_usec == fromvp->g_ctime.tv_usec) {
			++nfs_dupstats.renames;
			if (tovp != fromvp)
				gfs_unlock(tovp);
			gfs_unlock(fromvp);
		} else
			error = VOP_RENAME(fromvp, args->rna_from.da_name,
			    tovp, args->rna_to.da_name, u.u_cred);

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, fromvp->g_ctime, DUP_DONE);
		}
	}
	if (tovp != fromvp)
	    grele(tovp);

fromerr:
	grele(fromvp);
	*status = puterrno(error); 
	return(0);
} 

/*
 * Link to a file.
 * Create a file (to) which is a hard link to the given file (from).
 */
int
rfs_link(args, status, req, xprt, xpd)
	struct nfslinkargs *args;
	enum nfsstat *status;  
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *fromvp;
	register struct vnode *tovp;
	struct timeval duptime, savectime;
	int dupmark;

	tovp = fhtovp(&args->la_to.da_fhandle, xprt, RFS_LINK, xpd);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		return(ESTALE);
	}
	gfs_unlock(tovp);

	fromvp = fhtovp(&args->la_from, xprt, RFS_LINK, xpd);
	if (fromvp == NULL) {
		grele(tovp);
		*status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(tovp, xpd)) {
		gput(fromvp);
		grele(tovp);
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == fromvp->g_ctime.tv_sec &&
			duptime.tv_usec == fromvp->g_ctime.tv_usec) {
			++nfs_dupstats.links;
			savectime = fromvp->g_ctime;
			gput(fromvp);
			grele(tovp);
		} else
			error = vop_link(fromvp, tovp, args->la_to.da_name,
					 &savectime, u.u_cred);

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, savectime, DUP_DONE);
		}
	}
	*status = puterrno(error);
	return(0);
} 
 
/*
 * Symbolicly link to a file.
 * Create a file (to) with the given attributes which is a symbolic link
 * to the given path name (to).
 */
int
rfs_symlink(args, status, req, xprt, xpd)
	struct nfsslargs *args;
	enum nfsstat *status;   
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{		  
	register int error = 0;
	struct vattr va;
	register struct vnode *vp;
	struct timeval duptime;
	int dupmark;

	sattr_to_vattr(&args->sla_sa, &va);
	va.va_type = VLNK;
	vp = fhtovp(&args->sla_from.da_fhandle, xprt, RFS_SYMLINK, xpd);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(vp, xpd)) {
		gfs_unlock(vp);
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == vp->g_ctime.tv_sec &&
			duptime.tv_usec == vp->g_ctime.tv_usec) {
			++nfs_dupstats.symlinks;
			gfs_unlock(vp);
		}
		else
			error = VOP_SYMLINK(vp, args->sla_from.da_name,
					    &va, args->sla_tnm, u.u_cred);

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, vp->g_ctime, DUP_DONE);
		}
	}
	*status = puterrno(error);
	grele(vp);
	return(0);
}  
  
/*
 * Make a directory.
 * Create a directory with the given name, parent directory, and attributes.
 * Returns a file handle and attributes for the new directory.
 */
int
rfs_mkdir(args, dr, req, xprt, xpd)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	struct vattr va;
	struct vnode *dvp;
	register struct vnode *vp;
	struct timeval duptime;
	int dupmark;

	sattr_to_vattr(&args->ca_sa, &va);
	va.va_type = VDIR;
	/*
	 * Should get exclusive flag and pass it on here
	 */
	vp = fhtovp(&args->ca_da.da_fhandle, xprt, RFS_MKDIR, xpd);
	if (vp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(vp, xpd)) {
		gfs_unlock(vp);
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == vp->g_ctime.tv_sec &&
			duptime.tv_usec == vp->g_ctime.tv_usec) {
			++nfs_dupstats.mkdirs;
			error = VOP_LOOKUP(vp, args->ca_da.da_name, &dvp,
				u.u_cred);
			if (!error) {
				error = VOP_GETATTR(dvp, &va, u.u_cred);
			}
			gfs_unlock(vp);
		}
		else {
			error = VOP_MKDIR(vp, args->ca_da.da_name, &va,
				&dvp, u.u_cred);
		}
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			gfs_lock(dvp);
			error = makefh(&dr->dr_fhandle, dvp, 
				       args->ca_da.da_fhandle.fh_eno,
				       args->ca_da.da_fhandle.fh_egen);
			gput(dvp);
		}

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, vp->g_ctime, DUP_DONE);
		}

	}
	dr->dr_status = puterrno(error);
	grele(vp);
	return(0);
}

/*
 * Remove a directory.
 * Remove the given directory name from the given parent directory.
 */
int
rfs_rmdir(da, status, req, xprt, xpd)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register struct vnode *vp;
	struct timeval duptime;
	int dupmark;

	vp = fhtovp(&da->da_fhandle, xprt, RFS_RMDIR, xpd);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return(ESTALE);
	}
	if (rdonly(vp, xpd)) {
		gfs_unlock(vp);
		error = EROFS;
	} else {
		if (svckudp_dup(req, &duptime, &dupmark) &&
			duptime.tv_sec == vp->g_ctime.tv_sec &&
			duptime.tv_usec == vp->g_ctime.tv_usec) {
				++nfs_dupstats.rmdirs;
				gfs_unlock(vp);
		}
		else {
			error = VOP_RMDIR(vp, da->da_name, u.u_cred);
		}

		if (error) {
			svckudp_dupsave(req, *timepick, DUP_FAIL);
		} else {
			svckudp_dupsave(req, vp->g_ctime, DUP_DONE);
		}
	}
	*status = puterrno(error);
	grele(vp);
	return(0);
}

int
rfs_readdir(rda, rd, req, xprt, xpd)
	struct nfsrddirargs *rda;
	register struct nfsrddirres  *rd;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{
	register int error = 0;
	register u_long offset;
	register u_long skipped;
	register struct vnode *vp;
	register struct direct *dp;
	struct iovec iov;
	struct uio uio;
	vp = fhtovp(&rda->rda_fh, xprt, RFS_READDIR, xpd);
	if (vp == NULL) {
		rd->rd_status = NFSERR_STALE;
		return(ESTALE);
	}
	/*
	 * check cd access to dir.  we have to do this here because
	 * the opendir doesn't go over the wire.
	 */
	error = VOP_ACCESS(vp, VEXEC, u.u_cred);
	if (error) {
		goto bad;
	}

	/*
	 * Allocate data for entries.  This will be freed by rfs_rdfree.
	 */
	rd->rd_bufallocsize = rd->rd_bufsize = rda->rda_count;
	kmem_alloc(rd->rd_bufallocaddr, char *, rda->rda_count, KM_NFS);
	rd->rd_entries = (struct direct *)rd->rd_bufallocaddr;

nxtblk:

	rd->rd_offset = rda->rda_offset & ~(DIRBLKSIZ -1);

	/*
	 * Set up io vector to read directory data
	 */
	iov.iov_base = (caddr_t)rd->rd_bufallocaddr;
	iov.iov_len = rda->rda_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = rd->rd_offset;
	uio.uio_resid = rda->rda_count;

	/*
	 * read directory
	 */
	error = VOP_READDIR(vp, &uio, u.u_cred);

	/*
	 * Clean up
	 */
	if (error) {	
		rd->rd_size = 0;
		goto bad;
	}

	/*
	 * set size and eof
	 */
	if (uio.uio_resid) {
		rd->rd_size = rda->rda_count - uio.uio_resid;
		rd->rd_eof = TRUE;
	} else {
		rd->rd_size = rda->rda_count;
		rd->rd_eof = FALSE;
	}

	/*
	 * if client request was in the middle of a block
	 * or block begins with null entries skip entries
	 * til we are on a valid entry >= client's requested
	 * offset.
	 */
	dp = rd->rd_entries;
	offset = rd->rd_offset;
	skipped = 0;
	while ((skipped < rd->rd_size) &&
	    ((offset + dp->d_reclen <= rda->rda_offset) || (dp->d_ino == 0))) {
		skipped += dp->d_reclen;
		offset += dp->d_reclen;
		dp = (struct direct *)((int)dp + dp->d_reclen);
	}
	/*
	 * Reset entries pointer
	 */
	if (skipped) {
		rd->rd_size -= skipped;
		if (rd->rd_size == 0 && !rd->rd_eof) {
			/*
			 * we have skipped a whole block, reset offset
			 * and read another block (unless eof)
			 */
			rda->rda_offset = offset;
			goto nxtblk;
		}
		rd->rd_bufsize -= skipped;
		rd->rd_offset = offset;
		rd->rd_entries = dp;
	}
bad:
	rd->rd_status = puterrno(error);
	gput(vp);
	return(0);
}

rfs_rddirfree(rd)
	struct nfsrddirres *rd;
{
	if (rd->rd_bufallocaddr)
		kmem_free(rd->rd_bufallocaddr, KM_NFS);
}

rfs_statfs(fh, fs, req, xprt, xpd)
	fhandle_t *fh;
	register struct nfsstatfs *fs;
	struct svc_req *req;
	SVCXPRT *xprt;
	struct exportdata *xpd;
{

	register struct gnode *gp;
	struct fs_data *fsd;
	gp = (struct gnode *)fhtovp(fh, xprt, RFS_STATFS, xpd);
	if (gp == NULL) {
		fs->fs_status = NFSERR_STALE;
		return(ESTALE);
	}

	fsd = GGETFSDATA(gp->g_mp);
	fs->fs_tsize = nfstsize();
	fs->fsstat_bsize = FSDUNIT;
	fs->fs_blocks = fsd->fd_btot;
	fs->fs_bfree = fsd->fd_bfree;
	fs->fs_bavail = fsd->fd_bfreen;
	gput(gp);
	return(0);
}

/*ARGSUSED*/
rfs_null(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	/* do nothing */

	return (0);
}

/*ARGSUSED*/
rfs_error(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	return (EOPNOTSUPP);
}

int
nullfree()
{
}

/*
 * rfs dispatch table
 * Indexed by version,proc
 */

struct rfsdisp {
	int	  (*dis_proc)();	/* proc to call */
	xdrproc_t dis_xdrargs;		/* xdr routine to get args */
	int	  dis_argsz;		/* sizeof args */
	xdrproc_t dis_xdrres;		/* xdr routine to put results */
	int	  dis_ressz;		/* size of results */
	int	  (*dis_resfree)();	/* frees space allocated by proc */
} rfsdisptab[][RFS_NPROC]  = {
	{
	/*
	 * VERSION 2
	 * Changed rddirres to have eof at end instead of beginning
	 */
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof(fhandle_t),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_saargs, sizeof(struct nfssaargs),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof(struct nfsdiropargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof(fhandle_t),
	    xdr_rdlnres, sizeof(struct nfsrdlnres), rfs_rlfree},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof(struct nfsreadargs),
	    xdr_rdresult, sizeof(struct nfsrdresult), rfs_rdfree},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof(struct nfswriteargs),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_creatargs, sizeof(struct nfscreatargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof(struct nfsdiropargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof(struct nfsrnmargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof(struct nfslinkargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_slargs, sizeof(struct nfsslargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_creatargs, sizeof(struct nfscreatargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof(struct nfsdiropargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof(struct nfsrddirargs),
	    xdr_putrddirres, sizeof(struct nfsrddirres), rfs_rddirfree},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof(fhandle_t),
	    xdr_statfs, sizeof(struct nfsstatfs), nullfree},
	}
};

struct rfsspace {
	struct rfsspace *rs_next;
	caddr_t		rs_dummy;
};

struct rfsspace *rfsfreesp = NULL;

int rfssize = 0;

caddr_t
rfsget()
{
	int i;
	struct rfsdisp *dis;
	caddr_t ret;

	smp_lock(&lk_nfsargs, LK_RETRY);
	if (rfssize == 0) {
		for (i = 0; i < 1 + VERSIONMAX - VERSIONMIN; i++) {
			for (dis = &rfsdisptab[i][0];
			     dis < &rfsdisptab[i][RFS_NPROC];
			     dis++) {
				rfssize = MAX(rfssize, dis->dis_argsz);
				rfssize = MAX(rfssize, dis->dis_ressz);
			}
		}
	}

	if (rfsfreesp) {
		ret = (caddr_t)rfsfreesp;
		rfsfreesp = rfsfreesp->rs_next;
		smp_unlock(&lk_nfsargs);
	} else {
		smp_unlock(&lk_nfsargs);
		kmem_alloc(ret, caddr_t, rfssize, KM_NFS);
	}
	return (ret);
}

rfsput(rs)
	struct rfsspace *rs;
{
	smp_lock(&lk_nfsargs, LK_RETRY);
	rs->rs_next = rfsfreesp;
	rfsfreesp = rs;
	smp_unlock(&lk_nfsargs);
}

/*
 * If nfsportmon is set, then clients are required to use
 * privileged ports (ports < IPPORT_RESERVED) in order to get NFS services.
 */
int nfs_second_chance=1;
int nfsportmon = 0;
int nfs_cpu_count[32];
int nfs_throwaways[RFS_NPROC];
void
rfs_dispatch(req, xprt)
	register struct svc_req *req;
	register SVCXPRT *xprt;
{
	register struct rfsdisp *disp = NULL;
	register int error = 0;
	register caddr_t *args = NULL;
	register caddr_t *res = NULL;
	struct exportdata xpd;
	struct export *ep;
	fhandle_t *fh;

	int which;
	int vers;
	struct authunix_parms *aup;
	int *gp;
	struct ucred *tmpcr;
	struct ucred *newcr = NULL;
	int dup_xid = 0;

	smp_lock(&lk_nfsstat, LK_RETRY);
	svstat.ncalls++;
	smp_unlock(&lk_nfsstat);
	nfs_cpu_count[CURRENT_CPUDATA->cpu_num]++;

	which = req->rq_proc;
	if (which < 0 || which >= RFS_NPROC) {
		svcerr_noproc(req->rq_xprt);
		error++;
		goto done;
	}

	/* check for in-progress request in server cache. */
	/* if not found, this request is added and flagged */
	/* in-progress. */

	if (svckudp_dupbusy(req)) {
		dup_xid = 1;
		nfs_dupstats.throwaways++;
		++nfs_throwaways[which];
		goto done;
	}

	vers = req->rq_vers;
	if (vers < VERSIONMIN || vers > VERSIONMAX) {
		svcerr_progvers(req->rq_xprt, (u_long)VERSIONMIN,
		    (u_long)VERSIONMAX);
		error++;
		goto done;
	}
	vers -= VERSIONMIN;
	disp = &rfsdisptab[vers][which];

	/*
	 * Clean up as if a system call just started
	 */
	u.u_error = 0;

	/*
	 * Allocate args struct and deserialize into it.
	 */
	args = (caddr_t *)rfsget();
	bzero((caddr_t)args, rfssize);
	if ( ! SVC_GETARGS(xprt, disp->dis_xdrargs, args)) {
		svcerr_decode(xprt);
		error++;
		goto done;
	}

	/*
	 * Find export information and check authentication,
	 * setting the credential if everything is ok.
	 */
	if (which != RFS_NULL) {
		/*
		 * XXX: this isn't really quite correct. Instead of doing
		 * this blind cast, we should extract out the fhandle for
		 * each NFS call. What's more, some procedures (like rename)
		 * have more than one fhandle passed in, and we should check
		 * that the two fhandles point to the same exported path.
		 */
		fhandle_t *fh = (fhandle_t *) args;
		if ((nfs_second_chance == 1) && (fh->fh_eno == 0)) {
			fh->fh_eno = fh->fh_fno;
			fh->fh_egen = fh->fh_fgen;
		}

		xpd.x_flags=0; 		/* mark it unexported. . .*/
		smp_lock(&lk_gnode, LK_RETRY);
		ep = exported;
		while (ep != NULL) {
			if ((ep->e_fsid == fh->fh_fsid) &&
			    (ep->e_gnum == fh->fh_eno) &&
			    (ep->e_gen == fh->fh_egen)) {
				xpd.x_flags=ep->e_flags | M_EXPORTED;
				xpd.x_rootmap=ep->e_rootmap;
				break;
			} else {
				ep = ep->e_next;
			}
		}
		smp_unlock(&lk_gnode);
	}

	/*
	 * Check for unix style credentials
	 */
	if (req->rq_cred.oa_flavor != AUTH_UNIX && which != RFS_NULL) {
		svcerr_weakauth(xprt);
		error++;
		goto done;
	}

	if (nfsportmon) {
		/*
		* Check for privileged port number
		*/
       	static count = 0;
		if (ntohs(xprt->xp_raddr.sin_port) >= IPPORT_RESERVED) {
			svcerr_weakauth(xprt);
			if (count == 0) {
				printf("NFS request from unprivileged port, ");
				printf("source IP address = %u.%u.%u.%u\n",
					xprt->xp_raddr.sin_addr.s_net,
					xprt->xp_raddr.sin_addr.s_host,
					xprt->xp_raddr.sin_addr.s_lh,
					xprt->xp_raddr.sin_addr.s_impno);
			}
			count++;
			count %= 256;
			error++;
			goto done;
		}
	}


	/*
	 * Set uid, gid, and gids to auth params
	 */
	if (which != RFS_NULL) {
		aup = (struct authunix_parms *)req->rq_clntcred;
		newcr = crget();
		/*
		 * root over the net gets mapped in fhtovp()
		 */
                newcr->cr_uid = newcr->cr_ruid = aup->aup_uid;
                newcr->cr_gid = newcr->cr_rgid = aup->aup_gid;
		bcopy((caddr_t)aup->aup_gids, (caddr_t)newcr->cr_groups,
		    aup->aup_len * sizeof(newcr->cr_groups[0]));
		for (gp = &newcr->cr_groups[aup->aup_len];
		     gp < &newcr->cr_groups[NGROUPS];
		     gp++) {
			*gp = NOGROUP;
		}
		tmpcr = u.u_cred;
		u.u_cred = newcr;
	}

	/*
	 * Allocate results struct.
	 */
	res = (caddr_t *)rfsget();
	bzero((caddr_t)res, rfssize);

	smp_lock(&lk_nfsstat, LK_RETRY);
	svstat.reqs[which]++;
	smp_unlock(&lk_nfsstat);

	/*
	 * Call service routine with arg struct and results struct
	 */
	(*disp->dis_proc)(args, res, req, xprt, &xpd);

done:
	if (CURRENT_CPUDATA->cpu_hlock != NULL)
		panic("nfsd holding lock");

	/*
	 * Free arguments struct
	 */
	if (disp) {
		if (!SVC_FREEARGS(xprt, disp->dis_xdrargs, args))
			error++;
	} else
		if (!SVC_FREEARGS(xprt, NULL, NULL))
			error++;

	if (args != NULL) {
		rfsput((struct rfsspace *)args);
	}

	/*
	 * Serialize and send results struct
	 */
	if (!error && !dup_xid) {
		if (!svc_sendreply(xprt, disp->dis_xdrres, (caddr_t)res)) {
			error++;
		}
	}

	/*
	 * Free results struct
	 */
	if (res != NULL) {
		if ( disp->dis_resfree != nullfree ) {
			(*disp->dis_resfree)(res);
		}
		rfsput((struct rfsspace *)res);
	}
	/*
	 * restore original credentials
	 */
	if (newcr) {
		u.u_cred = tmpcr;
		crfree(newcr);
	}
	smp_lock(&lk_nfsstat, LK_RETRY);
	svstat.nbadcalls += error;
	smp_unlock(&lk_nfsstat);

	/* mark this request not in-progress in server cache */
	if (!dup_xid)
		svckudp_dupdone(req);

}
