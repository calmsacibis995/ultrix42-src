#ifndef lint
static	char	*sccsid = "@(#)nfs_vfsops.c	4.5	(ULTRIX)	4/25/91";
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
 * 08 Mar 91 -- dws
 *	Initialize root credential in callrpc().
 *
 * 28 Feb 91 -- dws
 *	Cleaned up client view of fhandle.
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 *  9 Mar 89 -- chet
 *	Use NFS_MAXDATA for local blocksize, not what server sends.
 *
 * 13 Dec 89 -- chet
 *	Add attribute cache timeout values.
 *
 * 10 Dec 89 -- chet
 *	Remove server permanent Arp table entry stuff in nfs_mount().
 *
 * 25 Jul 89 -- chet
 *	Add NFS type synchronous filesystems
 *
 * 13 Jun 89 -- condylis
 *	Added locking around calls to getpdev().
 *
 * 09 Feb 89 -- prs
 *      Modified nfs_statfs to return file system stats in 1K
 *      denominations.
 *
 * 12 Jan 88    Fred Glover
 *	Add nfs_rlock function for sys-V file lock support.
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 * 29 Oct 87 -- chet
 *	make check_mountp() error leg in nfs_mount
 *	go through error code before return
 *
 * 24 Aug 87 -- logcher
 *	Removed VROOT.  Overlaps with existing gnode flag, and root
 *	busy check is already performed in GFS.
 *
 * 20 Jul 87 -- logcher
 *	Added bzero to nfs_mountroot for fhandle stuffing.
 *
 * 11 May 87 -- chet
 *	Remove nfs_statfs() hack for hanging getmnt() stats.
 *
 * 28 Apr 87 -- chet
 *	Removed meaningless mount table option flags (M_SYNC, M_FORCE).
 *	Fixed spurious "bad arp" nfs_mount error message.
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added support for specfs,
 *	added ability to mount an NFS filesystem as the root filesystem
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/fs_types.h"
#include "../h/mount.h"
#include "../h/socket.h"
#include "../h/fs_types.h"
#include "../h/ioctl.h"
#include "../h/smp_lock.h"
#include "../net/net/if.h"
#include "../net/netinet/in.h"
#include "../net/rpc/types.h"
#include "../net/rpc/xdr.h"
#include "../net/rpc/auth.h"
#include "../net/rpc/clnt.h"
#include "../net/rpc/pmap_prot.h"
#ifdef NFSDEBUG
#define NFSSERVER
#endif
#include "../nfs/nfs.h"
#include "../nfs/nfs_gfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/vfs.h"
#include "../nfs/vnode.h"
#include "../net/rpcsvc/mount.h"
#include "../sas/mop.h"

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

struct vnode *makenfsnode();
int nfsmntno;

extern struct lock_t lk_mount_table;

/*
 * nfs vfs operations.
 */
struct mount *nfs_mount();
int nfs_statfs();
int		nfs_match(),	nfs_ginit(),	nfs_inactive();
int		nfs_umount(),	nfs_sbupdate();
struct gnode	*ufs_gget(),	*nfs_namei();
int		nfs_glink(),	nfs_unlink(),	nfs_grmdir();
struct gnode	*nfs_gmkdir();
struct gnode	*nfs_makenode();
int		nfs_grename(),	nfs_readdir();
int		nfs_rele();
struct gnode	*ufs_galloc();
int		nfs_syncgp(),	ufs_gfree(),	nfs_trunc();
int		nfs_rwgp(),	nfs_stat();
int		nfs_lock(),	nfs_unlock();
int		nfs_gupdat(),	nfs_open();
int		nfs_close(),	nfs_getval();
int		nfs_select(),	nfs_greadlink(), nfs_gsymlink();
struct fs_data	*nfs_getfsdata();
int		ufs_fcntl(), nfs_gbmap();
int		ufs_seek();
int		nfs_rlock();
extern nfs_strategy();

struct	mount_ops NFS_mount_ops = {
	nfs_umount,
	nfs_sbupdate,
	nfs_ginit,
	nfs_match,
	nfs_ginit, 		/*  move to mount_ops */
	nfs_inactive,
 	nfs_getfsdata,
};
struct	gnode_ops NFS_gnode_ops = {
	nfs_namei,
	nfs_glink,
	nfs_unlink,
	nfs_gmkdir,
	nfs_grmdir,
	nfs_makenode,
	nfs_grename,
	nfs_readdir,
	nfs_rele,
	nfs_syncgp,
	nfs_trunc,
	nfs_getval,
	nfs_rwgp,
	nfs_rlock,
	ufs_seek,	/* seek */
	nfs_stat,
	nfs_lock,
	nfs_unlock,
	nfs_gupdat,
	nfs_open,
	nfs_close,
	nfs_select,
	nfs_greadlink,
	nfs_gsymlink,
	0,		/* fcntl */
	0,
	nfs_gbmap
};

struct  mount_ops  *nfs_mount_ops = &NFS_mount_ops;
struct  gnode_ops  *nfs_gnode_ops = &NFS_gnode_ops;

/*
 * nfs_mount is called from the generic mount system call
 */

struct mount *
nfs_mount(special, path, flag, mp, ops)
caddr_t special;
caddr_t path;
int flag;
struct mount *mp;
struct nfs_gfs_mount *ops;
{

	struct vnode *rootvp = NULL;	/* root vnode */
	struct vfs *vfsp;		/* nfs vfs handle */
	struct mntinfo *mi;		/* mount info, pointed at by vfs */
	struct vattr va;		/* root vnode attributes */
	struct nfsfattr na;		/* root vnode attributes in nfs form */
	struct nfs_gfs_mount nfs_gfs_mount;
	struct nfs_gfs_mount *ngp = &nfs_gfs_mount;
	struct statfs sb;		/* server's file system stats */
	fhandle_t fh;
	dev_t tdev;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_mount: special=%s, path=%s, flag=%d\n",
		special, path, flag);
	dprint(nfsdebug, 4, "nfs_mount: mp = 0x%x, ops = 0x%x\n",
		mp, ops);
#endif
	/*
	 * Copy NFS mount arguments out of userspace
	 */
	u.u_error = copyin((caddr_t)ops, (caddr_t)ngp, sizeof(nfs_gfs_mount));
	if (u.u_error)
		goto error;

	/*
	 * Set up data structures in fsdata spare area
	 */
	mi = MP_TO_MIP(mp);
	vfsp = MP_TO_VFSP(mp);
	mp->m_bufp = (struct buf *) (NODEV);	/* to reserve this slot */
	smp_lock(&lk_mount_table, LK_RETRY);
	if (!(tdev = getpdev())) {		/* pseudo-device number */
		smp_unlock(&lk_mount_table);
		u.u_error = EBUSY;
		goto error;
	}
	mp->m_dev = tdev;
	smp_unlock(&lk_mount_table);
	mp->iostrat = nfs_strategy;		/* set it immediately */
 	mp->m_ops = nfs_mount_ops;
	mp->m_fstype = GT_NFS;
	mp->m_flags = (flag ? M_RONLY : 0);
	mp->m_flags |= (ngp->gfs_flags &
		(M_NOEXEC | M_NOSUID | M_NODEV | M_SYNC));

	if (ngp->flags & NFSMNT_PGTHRESH) {
		int pg_thresh = ngp->pg_thresh * 1024;
		mp->m_fs_data->fd_pgthresh =
			clrnd(btoc((pg_thresh > MINPGTHRESH) ?
				pg_thresh : MINPGTHRESH));
	} else {
		mp->m_fs_data->fd_pgthresh = clrnd(btoc(MINPGTHRESH * 8));
	}

	u.u_error = copyin((caddr_t)ngp->fh, (caddr_t)&fh, sizeof(fh));
	if (u.u_error)
		goto error;
	u.u_error = copyin((caddr_t)ngp->addr, (caddr_t)(&mi->mi_addr),
		sizeof(mi->mi_addr));
	if (u.u_error)
		goto error;
	if (!(ngp->flags & NFSMNT_HOSTNAME))		/* XXX */
		addr_to_str(&(mi->mi_addr), mi->mi_hostname);
	else {
		u.u_error = copyinstr(ngp->hostname, mi->mi_hostname,
			HOSTNAMESZ, (caddr_t)0);
		if (u.u_error)
			goto error;
	}

	if (ngp->optstr) {
		u.u_error = copyinstr(ngp->optstr, mi->mi_optstr,
			MNTMAXSTR, (caddr_t)0);
		if (u.u_error)
			goto error;
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
		"nfs_mount: saddr 0x%x, fh dev 0x%x gno %d\n",
		&(mi->mi_addr), fh.fh_fsid, fh.fh_fno);
#endif

	/*
	 * Create a mount record
	 */
	mi->mi_refct = 0;
	mi->mi_stsize = 0;
	mi->mi_hard = ((ngp->flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((ngp->flags & NFSMNT_INT) == NFSMNT_INT);
	mi->mi_noac = ((ngp->flags & NFSMNT_NOAC) == NFSMNT_NOAC);
	if (!mi->mi_noac) {
		mi->mi_acregmin = ngp->acregmin;
		mi->mi_acregmax = ngp->acregmax;
		mi->mi_acdirmin = ngp->acdirmin;
		mi->mi_acdirmax = ngp->acdirmax;
	}
	if (ngp->flags & NFSMNT_RETRANS) {
		mi->mi_retrans = ngp->retrans;
		if (ngp->retrans < 0) {
			u.u_error = EINVAL;
			goto error;
		}
	} else {
		mi->mi_retrans = NFS_RETRIES;
	}
	if (ngp->flags & NFSMNT_TIMEO) {
		mi->mi_timeo = ngp->timeo;
		if (ngp->timeo <= 0) {
			u.u_error = EINVAL;
			goto error;
		}
	} else {
		mi->mi_timeo = NFS_TIMEO;
	}
	mi->mi_mntno = nfsmntno++;
	mi->mi_printed = 0;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_mount: mi_addr 0x%x port %d addr 0x%x\n",
		&mi->mi_addr, mi->mi_addr.sin_port, (int)mi->mi_addr.sin_addr.s_addr);

#endif

	/*
	 * For now we just support AF_INET
	 */
	if (mi->mi_addr.sin_family != AF_INET) {
		u.u_error = EPFNOSUPPORT;
		goto error;
	}

	/*
	 * Make a vfs struct for nfs.  We do this here instead of below
	 * because rootvp needs a vfs before we can do a getattr on it.
	 */
	VFS_INIT(vfsp, (caddr_t)mi);

	/*
	 * Make the root vnode
	 */
	rootvp = makenfsnode(&fh, (struct nfsfattr *) 0, vfsp, mp);
	if(rootvp == NULL) {
		goto error;
	}

	/*
	 * Get real attributes of the root vnode, and remake it.
	 * While we're at it, get the transfer size for this filesystem.
	 */
	u.u_error = nfs_makeroot(&fh, &rootvp);
	if (u.u_error) {
		goto error;
	}

	mp->m_rootgp = (struct gnode *) rootvp;
	mi->mi_rootvp = rootvp;

	if(u.u_uid) {
		((struct gnode *) rootvp)->g_uid = u.u_uid;
		((struct gnode *) rootvp)->g_flag |= GCHG;
	}

	/*
	 * Get server's filesystem stats.  Use these to set transfer sizes
	 */
	mi->mi_tsize = min(NFS_MAXDATA, nfstsize());
	if (ngp->flags & NFSMNT_RSIZE) {
		if (ngp->rsize <= 0) {
			u.u_error = EINVAL;
			goto error;
		}
		mi->mi_tsize = MIN(mi->mi_tsize, ngp->rsize);
	}

	if (ngp->flags & NFSMNT_WSIZE) {
		if (ngp->wsize <= 0) {
			u.u_error = EINVAL;
			goto error;
		}
		mi->mi_stsize = ngp->wsize; /* touched by nfs_getfsdata */
	}

	(void) nfs_getfsdata(mp);
	if (u.u_error)
		goto error;

#ifdef NFSDEBUG
        dprint(nfsdebug, 10,
		"nfs_mount: vfs %x: vnodecov = %x,	data = %x\n",
		vfsp, vfsp->vfs_vnodecovered, vfsp->vfs_data);
	dprint(nfsdebug, 10, "rootvp %x: vfs %x\n",
		rootvp, rootvp->v_vfsp);
	dprint(nfsdebug, 4,
	    "nfs_mount: hard %d timeo %d retries %d wsize %d rsize %d\n",
	    mi->mi_hard, mi->mi_timeo, mi->mi_retrans, mi->mi_stsize,
	    mi->mi_tsize);
	dprint(nfsdebug, 4, "nfs_mount: mp->m_flags = 0x%x\n",
		mp->m_flags);
	dprint(nfsdebug, 4, "nfs_mount: fd_otsize = %d, fd_mtsize=%d\n",
		mp->m_fs_data->fd_otsize, mp->m_fs_data->fd_mtsize);
#endif

	/*
	 * Should set read only here!
	 */

	mi->mi_bsize = NFS_MAXDATA;
	vfsp->vfs_bsize = mi->mi_bsize;
	mp->m_bsize = mi->mi_bsize;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
	    "nfs_mount: vfs_bsize %d\n", vfsp->vfs_bsize);
#endif
        
	dnlc_purge();

	/*
	 * check_mountp will verify local mount point exists and is
	 * ok to mount on.
	 */

	if (!(check_mountp(mp, path)))
		goto error;

	return(mp);

error:
#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_mount: returning error, %d\n",
		u.u_error);
#endif
	if (rootvp) {
		VN_RELE(rootvp);
	}
	return((struct mount *)NULL);
}

/*
 * nfs_mountroot is called from mount_root() to mount the rootfs
 */

struct mount *
nfs_mountroot(special, path, flag, mp, ops)
caddr_t special;
caddr_t path;
int flag;
struct mount *mp;
struct nfs_gfs_mount *ops;
{

        struct vnode *rootvp = NULL;    /* root vnode */
        struct vnode *hidvp = NULL;     /* local vnode to be hidden */
        struct vfs *vfsp;               /* nfs vfs handle */
        struct mntinfo *mi;             /* mount info, pointed at by vfs */
        struct vattr va;                /* root vnode attributes */
        struct nfsfattr na;             /* root vnode attributes in nfs form */
        struct statfs sb;               /* server's file system stats */
        fhandle_t fh;
        int i;

#ifdef NFSDEBUG
        nfsdebug=0;
        dprint(nfsdebug, 4, "nfs_mount: special=%s, path=%s, flag=%d\n",
                special, path, flag);
        dprint(nfsdebug, 4, "nfs_mount: mp = 0x%x, ops = 0x%x\n",
                mp, ops);
#endif

        /*
         * Set up data structures in fsdata spare area
         */
        mi = MP_TO_MIP(mp);
        vfsp = MP_TO_VFSP(mp);
        mp->m_bufp = (struct buf *) (NODEV);    /* to reserve this slot */
        mp->m_dev = rootdev = getpdev();        /* pseudo-device number */
        mp->iostrat = nfs_strategy;             /* set it immediately */
        mp->m_ops = nfs_mount_ops;
        mp->m_fstype = GT_NFS;
        mp->m_flags = (flag ? M_RONLY : 0);
        mp->m_flags |= (ops->gfs_flags &
                        (M_NOEXEC | M_NOSUID | M_NODEV | M_SYNC));

        if (ops->flags & NFSMNT_PGTHRESH) {
                int pg_thresh = ops->pg_thresh * 1024;
                mp->m_fs_data->fd_pgthresh =
                        (pg_thresh > MINPGTHRESH) ?
                                pg_thresh : MINPGTHRESH;
        } else {
                mp->m_fs_data->fd_pgthresh = MINPGTHRESH * 8;
        }

	/*
	 * Clear out full size of fh and stuff in fhandle from ops
	 */
	bcopy(ops->fh, &fh, sizeof(fhandle_t));
        bcopy(ops->addr,&mi->mi_addr,sizeof(struct sockaddr_in));
        bcopy(ops->hostname,mi->mi_hostname,sizeof(mi->mi_hostname));

#ifdef NFSDEBUG
        dprint(nfsdebug, 4,
                "nfs_mount: saddr 0x%x, fh dev 0x%x gno %d\n",
                &(mi->mi_addr), fh.fh_fsid, fh.fh_fno);
#endif

        /*
         * Create a mount record
         */
        mi->mi_refct = 0;
        mi->mi_stsize = 0;
        mi->mi_hard = ((ops->flags & NFSMNT_SOFT) == 0);
        mi->mi_int = ((ops->flags & NFSMNT_INT) == NFSMNT_INT);
	mi->mi_noac = ((ops->flags & NFSMNT_NOAC) == NFSMNT_NOAC);
	if (!mi->mi_noac) {
		mi->mi_acregmin = ops->acregmin;
		mi->mi_acregmax = ops->acregmax;
		mi->mi_acdirmin = ops->acdirmin;
		mi->mi_acdirmax = ops->acdirmax;
	}
        if (ops->flags & NFSMNT_RETRANS) {
                mi->mi_retrans = ops->retrans;
                if (ops->retrans < 0) {
                        u.u_error = EINVAL;
                        goto error;
                }
        } else {
                mi->mi_retrans = NFS_RETRIES;
        }
        if (ops->flags & NFSMNT_TIMEO) {
                mi->mi_timeo = ops->timeo;
                if (ops->timeo <= 0) {
                        u.u_error = EINVAL;
                        goto error;
                }
        } else {
                mi->mi_timeo = NFS_TIMEO;
        }
        mi->mi_mntno = nfsmntno++;
        mi->mi_printed = 0;

#ifdef NFSDEBUG
        dprint(nfsdebug, 4, "nfs_mount: mi_addr 0x%x port %d addr 0x%x\n",
                &mi->mi_addr, mi->mi_addr.sin_port, (int)mi->mi_addr.sin_addr.s_addr);
#endif

        /*
         * For now we just support AF_INET
         */
        if (mi->mi_addr.sin_family != AF_INET) {
                u.u_error = EPFNOSUPPORT;
                goto error;
        }

        /*
         * Make a vfs struct for nfs.  We do this here instead of below
         * because rootvp needs a vfs before we can do a getattr on it.
         */
        VFS_INIT(vfsp, (caddr_t)mi);

        /*
         * Make the root vnode
         */
        rootvp = makenfsnode(&fh, (struct nfsfattr *) 0, vfsp, mp);

        /*
         * Get attributes of the root vnode.
         */
        u.u_error = nfs_makeroot(&fh, &rootvp);
        if (u.u_error) {
                goto error;
        }

        mp->m_rootgp = (struct gnode *) rootvp;
        mi->mi_rootvp = rootvp;

        if(u.u_uid) {
                ((struct gnode *) rootvp)->g_uid = u.u_uid;
                ((struct gnode *) rootvp)->g_flag |= GCHG;
        }

        /*
         * Get server's filesystem stats.  Use these to set transfer sizes
         */
        mi->mi_tsize = min(NFS_MAXDATA, nfstsize());
        if (ops->flags & NFSMNT_RSIZE) {
                if (ops->rsize <= 0) {
                        u.u_error = EINVAL;
                        goto error;
                }
                mi->mi_tsize = MIN(mi->mi_tsize, ops->rsize);
        }

        if (ops->flags & NFSMNT_WSIZE) {
                if (ops->wsize <= 0) {
                        u.u_error = EINVAL;
                        goto error;
                }
                mi->mi_stsize = ops->wsize; /* touched by nfs_getfsdata */
        }

        (void) nfs_getfsdata(mp);
        if (u.u_error) {
                goto error;
	}

#ifdef NFSDEBUG
        dprint(nfsdebug, 10,
                "nfs_mount: vfs %x: vnodecov = %x,      data = %x\n",
                vfsp, vfsp->vfs_vnodecovered, vfsp->vfs_data);
        dprint(nfsdebug, 10, "rootvp %x: vfs %x\n",
                rootvp, rootvp->v_vfsp);
        dprint(nfsdebug, 4,
            "nfs_mount: hard %d timeo %d retries %d wsize %d rsize %d\n",
            mi->mi_hard, mi->mi_timeo, mi->mi_retrans, mi->mi_stsize,
            mi->mi_tsize);
        dprint(nfsdebug, 4, "nfs_mount: mp->m_flags = 0x%x\n",
                mp->m_flags);
        dprint(nfsdebug, 4, "nfs_mount: fd_otsize = %d, fd_mtsize=%d\n",
                mp->m_fs_data->fd_otsize, mp->m_fs_data->fd_mtsize);
#endif

        /*
         * Should set read only here!
         */

	mi->mi_bsize = NFS_MAXDATA;
        vfsp->vfs_bsize = mi->mi_bsize;
        mp->m_bsize = mi->mi_bsize;

        bzero(mp->m_fs_data->fd_path,sizeof(mp->m_fs_data->fd_path));
        bcopy("/",mp->m_fs_data->fd_path,sizeof("/"));
        bzero(mp->m_fs_data->fd_devname,sizeof(mp->m_fs_data->fd_devname));
        bcopy(special,mp->m_fs_data->fd_devname,NFS_MAXPATHLEN);
        mp->m_fs_data->fd_dev=mp->m_dev;
       inittodr(rootvp->g_in.gn.gc_ctime);

#ifdef NFSDEBUG
        dprint(nfsdebug, 4,
            "nfs_mount: vfs_bsize %d\n",vfsp->vfs_bsize);
#endif

        dnlc_purge();
        return(mp);
error:
#ifdef NFSDEBUG
        dprint(nfsdebug, 4, "nfs_mount: returning error, %d\n",
                u.u_error);
#endif
        if (u.u_error) {
                if (hidvp) {
                        VN_RELE(hidvp);

                }
                if (rootvp) {
                        VN_RELE(rootvp);
                }
        }
        return((struct mount *)NULL);
}

/*
 * callrpc
 */

enum clnt_stat callrpc(sin, prognum, versnum, procnum, inproc, in, outproc, out)
        struct sockaddr_in *sin;
        int prognum, versnum, procnum;
        xdrproc_t inproc, outproc;
        char *in, *out;
{
        CLIENT *cl;
        struct timeval tv;
        enum clnt_stat cl_stat;
	int i;
	struct ucred *tmpcred, *savecred;

	/* Set up credential */
        tmpcred = crdup(u.u_cred);
	savecred = u.u_cred;
	u.u_cred = tmpcred;
	u.u_uid = 0;
	u.u_gid = 0;
	for (i=1; i<NGROUPS; i++)
		u.u_groups[i] = NOGROUP;
	
        cl = clntkudp_create(sin, prognum, versnum, 1, u.u_cred);
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        cl_stat = CLNT_CALL(cl, procnum, inproc, in, outproc, out, tv);
        AUTH_DESTROY(cl->cl_auth);
        CLNT_DESTROY(cl);

	/*
	 * Reset credentials
	 */
	u.u_cred = savecred;
	crfree(tmpcred);
        return (cl_stat);
}

/*
 * Call mount daemon on server sin to mount path.
 * sin_port is set to nfs port and fh is the fhandle
 * returned from the server.
 */
mountrpc(sin, path, fh)
        struct sockaddr_in *sin;
        char *path;
        fhandle_t *fh;
{
        struct fhstatus fhs;
        int error;
        int     i;
        enum clnt_stat status;

        error = pmap_kgetport(sin, MOUNTPROG, MOUNTVERS, IPPROTO_UDP);
        if (error) {
                printf("mountrpc: pmap_kgetport returned error %d\n", error);
                panic("nfs_mountrpc cannot get port for mount service");
        }
        status = callrpc(sin, MOUNTPROG, MOUNTVERS, MOUNTPROC_MNT, xdr_path,
                &path, xdr_fhstatus, &fhs);
        if (status != RPC_SUCCESS) {
                printf("mountrpc: callrpc returned error %d\n", status);
                panic("mountrpc: cannot NFS mount file");
        }
        sin->sin_port = htons(NFS_PORT);
	bcopy((caddr_t)&fhs.fhs_fh, (caddr_t)fh, sizeof(fhandle_t));
        return (fhs.fhs_status);
}

/*
 * nfs_resolvfh - resolve a file handle using the MOP server IP and
 *                input path
 *
 *      inputs:   destination file handle ptr, source pathname
 *
 */

nfs_resolvfh(fhdl,path)
	fhandle_t *fhdl;
	char *path;
{
        int erret;
        struct sockaddr_in sin;
	extern struct netblk *netblk_ptr;

        bzero(&sin, sizeof(sin));
        sin.sin_addr.s_addr=ntohl(netblk_ptr->srvipadr); 
        sin.sin_family=AF_INET;
        sin.sin_port=htons(NFS_PORT);
        erret=mountrpc(&sin,path,fhdl);
        if(erret!=0)
                panic("resolvfh: cannot resolve file handle");
        return(0);
}

/*
 * Get file system statistics.
 */
int
nfs_statfs(mp, sbp)
register struct mount *mp;
struct statfs *sbp;
{
	struct nfsstatfs fs;
	struct mntinfo *mi;
	fhandle_t *fh;

	mi = MP_TO_MIP(mp);

	fh = vtofh(mi->mi_rootvp);

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_statfs fh %o %d\n", fh->fh_fsid, fh->fh_fno);
#endif

	u.u_error = rfscall(mi, RFS_STATFS, xdr_fhandle,
	    (caddr_t)fh, xdr_statfs, (caddr_t)&fs, u.u_cred);
	if (u.u_error) {
		register struct fs_data *fsd = mp->m_fs_data;
		if (u.u_error == ETIMEDOUT) {
			u.u_error = 0;
			sbp->f_bsize = fsd->fd_bsize;
			sbp->f_blocks = fsd->fd_btot;
			sbp->f_bfree = fsd->fd_bfree;
			sbp->f_bavail = fsd->fd_bfreen;
		}
	} else {
		if (!(u.u_error = geterrno(fs.fs_status))) {
			if (mi->mi_stsize) {
				mi->mi_stsize = min(mi->mi_stsize, fs.fs_tsize);
			} else {
				mi->mi_stsize = fs.fs_tsize;
			}
			/*
                         * XXX - Calculate file system stats returned
                         * by the server into 1K denominations. User level
                         * land expects this.
                         */
			sbp->f_bsize = fs.fsstat_bsize;
                        sbp->f_blocks =
                            (long)(fs.fs_blocks * fs.fsstat_bsize) / FSDUNIT;
                        sbp->f_bfree =
			    (long)(fs.fs_bfree * fs.fsstat_bsize) / FSDUNIT;
                        sbp->f_bavail =
                            (long)(fs.fs_bavail * fs.fsstat_bsize) / FSDUNIT;
			/*
			 * ULTRIX doesn't use the fsid
			 */
			bzero((caddr_t)sbp->f_fsid, sizeof(fsid_t));
		}
	}
	
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_statfs returning %d\n", u.u_error);
	dprint(nfsdebug, 5, "fs_tsize = %d, fsstat_bsize = %d, fs_blocks = %d\n",
		fs.fs_tsize, fs.fsstat_bsize, fs.fs_blocks);
	dprint(nfsdebug, 5, "fs_bfree = %d, fs_bavail = %d\n",
		fs.fs_bfree, fs.fs_bavail);

#endif
	return (u.u_error);
}

static char *
itoa(n, str)
	u_short n;
	char *str;
{
	char prbuf[11];
	register char *cp;

	cp = prbuf;
	do {
		*cp++ = "0123456789"[n%10];
		n /= 10;
	} while (n);
	do {
		*str++ = *--cp;
	} while (cp > prbuf);
	return (str);
}

/*
 * Convert a INET address into a string for printing
 */
addr_to_str(addr, str)
	struct sockaddr_in *addr;
	char *str;
{
	str = itoa(addr->sin_addr.s_net, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_host, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_lh, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_impno, str);
	*str = '\0';
}

int
nfs_makeroot(fh, vpp)
	fhandle_t *fh;
	struct vnode **vpp;
{
	struct nfsattrstat *ns;
	int error = 0;
	struct mount *mp;
	struct vfs *vfsp;
	struct vnode *vp;
	
	vp = *vpp;
	mp = (*vpp)->g_mp;
	vfsp = (*vpp)->v_vfsp;

	kmem_alloc(ns, struct nfsattrstat *, (u_int)sizeof(*ns), KM_NFS);
	error = rfscall(vtomi(*vpp), RFS_GETATTR, xdr_fhandle, (caddr_t)vtofh(vp),
	    xdr_attrstat, (caddr_t)ns, u.u_cred);
	if (!error) {
		if (error = geterrno(ns->ns_status))
		goto out;
	}
	nfs_rele(*vpp);
	*vpp = makenfsnode(fh, &(ns->ns_attr), vfsp, mp);
	if (*vpp == NULL)
		error = u.u_error;

out:
	kmem_free(ns, KM_NFS);
	return(error);
}
