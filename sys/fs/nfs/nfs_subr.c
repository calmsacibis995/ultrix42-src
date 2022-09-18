#ifndef lint
static	char	*sccsid = "@(#)nfs_subr.c	4.6	(ULTRIX)	2/28/91";
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
 *	Clean up for server view of fhandle.  
 *
 * 27 Feb 91 -- chet
 *	Change nfs_attrcache() calls (yet again).
 *
 * 10 Feb 91 -- chet
 *	Change nfs_attrcache() calls (again).
 *
 * 29 Jan 91 -- chet
 *	Make kernel RPC calls interruptible.
 *
 *  7 Jul 90 -- chet
 *	Change nfs_attrcache() calls.
 *
 *  9 Mar 90 -- chet
 *	Change nfs_attrcache() calls for new argument.
 *	Add clearing of rnode in nfs_ginit().
 *
 * 12 Jan 90 -- prs
 *	Added a lockinit of nfsargs lock structure to nfsinit().
 *
 * 08 Dec 89 -- cb
 *	fixed export list corruption bug. Also check for null gp
 *	in makenfsnode().
 *
 * 19 Jul 89 -- cb
 *	modified exportfs() and makefh() to support export options on
 *	directories.
 *
 * 15 May 89 -- condylis
 *	Moved specvp call from makenfsnode() to nfs_ginit().
 *
 * 10 Mar 89 -- chet
 *	Added nfs_system_size() routine that returns
 *	"small | medium | large | xlarge";
 *	use it to size # of client handles.
 *
 *  7 Mar 89 -- condylis
 *	Free client handle for a time between RPC timeout cycles
 *	when trying to reach a hard mounted server.
 *
 * 28 Feb 89 -- chet
 *	Added scheme to prevent "stale fhandle" messages from
 *	locking out server console.
 *
 *  6 Feb 89 -- prs
 *	Added gput() to a return path in exportfs().
 *
 *  1 Sep 88 -- chet 
 *	Put a unique transaction ID into rfscall
 *
 * 18-Jul-88 -- condylis
 *	Added SMP locking for CLIENT handle table in clget and clfree.
 *	Added nfsinit to initialize NFS SMP locks.  Replaced unsafe 
 *	modifications of gnodes with calls to SMP gnode primitives. 
 *	Added SMP locking of NFS client statistics.
 *	
 * 10-Jun-88 -- jaw 
 * 	add parameter to ISSIG for SMP.... this makes going to stop
 *	state atomic.
 *
 * 17 Aug 88 -- chet
 *      Clear the group IDs for a root mapped to nobody
 *
 * 8 Mar 88 -- chet
 *	exportfs() changes
 *
 * 4 Feb 88 -- chet
 *	Change fhtovp() to not return fhandle unless filesystem
 *	is exported.
 *
 * 26 Jan 88 -- chet
 *	Fix newname() so that it comes up with a more unique name.
 *	This routine is used by nfs_unlink() when unlinking open files.
 *
 * 14 Jan 88 -- chet
 *	Expanded stale file handle messages
 *
 * 12-28-87	Tim Burke
 *  	Moved u.u_ttyp to u.u_procp->p_ttyp.
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 * 14-Jul-87 -- logcher
 *	Revised logic in exportfs.  Fixed declaration of uap
 *	arguments.  Added logic to only set m_flags if M_NOFH or
 *	M_EXRONLY and no other flags ar set.  Added logic to clear
 *	the M_NOFH and M_EXRONLY flags before setting.  This way a
 *	flag is turned off without having to reboot or adb the kernel.
 *
 * 17-Jun-87 -- logcher
 *	Changed short cast to an int in exportfs
 *
 * 15-Jun-87 -- logcher
 *	Added suser check to exportfs(), cleaned up returns
 *
 * 05-May-87 -- logcher
 *	Changed usage of m_exflags to m_flags and added M_EXRONLY
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added exportfs and check in
 *	fhtovp for root mapping, added check for specvp
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/fs_types.h"
#include "../h/mount.h"
#include "../h/gnode.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"
#include "../net/netinet/in.h"
#include "../net/rpc/types.h"
#include "../net/rpc/xdr.h"
#include "../net/rpc/auth.h"
#include "../net/rpc/clnt.h"
#include "../net/rpc/svc.h"
#include "../net/net/if.h"
#define NFSSERVER
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/vfs.h"
#include "../nfs/vnode.h"
#include "../ufs/fs.h"

int do_uprintf = 0;
extern struct vnodeops nfs_vnodeops;
extern struct gnode_ops *nfs_gnode_ops;
extern struct mount_ops *nfs_mount_ops;
struct gnode *rfind();
extern struct lock_t lk_gnode;

/*
 * Client side utilities
 */

/*
 * client side statistics
 */
struct {
        int     nclsleeps;              /* client handle waits */
        int     nclgets;                /* client handle gets */
        int     ncalls;                 /* client requests */
        int     nbadcalls;              /* rpc failures */
        int     reqs[32];               /* count of each request */
} clstat;


/* SMP lock for client handle table		*/
struct	lock_t	lk_nfschtable;
/* SMP lock for nfs and rpc statistics		*/
struct	lock_t	lk_nfsstat;

/* Need extern for nfsinit routine */
extern	struct	lock_t lk_nfsbiod;
extern	struct	lock_t lk_nfsdnlc;
extern	struct	lock_t lk_nfsrrok;
extern	struct	lock_t lk_nfsargs;
/* lk_rpcxid is used by rpc clients for updating client handle xids */
/* (also used by klm_lockmgr.c)  */
extern	struct	lock_t lk_rpcxid;

#define MINCLIENTS      8
#define MAXCLIENTS      36
struct chtab {
	int	ch_timesused;
	bool_t	ch_inuse;
	CLIENT	*ch_client;
} chtable[MAXCLIENTS];

int	chtable_max = 0;	/* chtable high water mark */

int	clwanted = 0;

/* NFS transaction ID */
int     nfs_next_xid = 0;

struct chtab *
clget(mi, cred)
	struct mntinfo *mi;
	struct ucred *cred;
{
	register struct chtab *ch;
	int retrans;

	/*
	 * Set client handle table high water mark based on system "size"
	 */
	if (!chtable_max) {
		smp_lock(&lk_nfschtable, LK_RETRY);
		if (!chtable_max) {
			chtable_max = 8 * nfs_system_size();
			chtable_max = MAX(MINCLIENTS, chtable_max);
			chtable_max = MIN(MAXCLIENTS, chtable_max);
		}
		smp_unlock(&lk_nfschtable);
	}

	/*
	 * If soft mount and server is down just try once
	 */
	if (!mi->mi_hard && mi->mi_down) {
		retrans = 1;
	} else {
		retrans = mi->mi_retrans;
	}

	/*
	 * Find an unused handle or create one if not at limit yet.
	 */
	for (;;) {
		smp_lock(&lk_nfsstat, LK_RETRY);
		clstat.nclgets++;
		smp_unlock(&lk_nfsstat);

		/* SMP lock client handle table during access */
		smp_lock(&lk_nfschtable, LK_RETRY);

		for (ch = chtable; ch < &chtable[chtable_max]; ch++) {
			if (!ch->ch_inuse) {
				ch->ch_inuse = TRUE;
				smp_unlock(&lk_nfschtable);
				if (ch->ch_client == NULL) {
					ch->ch_client =
					    clntkudp_create(&mi->mi_addr,
					    NFS_PROGRAM, NFS_VERSION,
					    retrans, cred);
					if (ch->ch_client == NULL)
						panic("clget: null client");

				} else {
					clntkudp_init(ch->ch_client,
					    &mi->mi_addr, retrans, cred);
				}
				ch->ch_timesused++;
				return (ch);
			}
		}
		/*
		 * If we got here there are no available handles
		 */
		clwanted++;
		sleep_unlock((caddr_t)chtable, PRIBIO, &lk_nfschtable);
		smp_lock(&lk_nfsstat, LK_RETRY);
		clstat.nclsleeps++;
		smp_unlock(&lk_nfsstat);
	}
}

clfree(ch)
	struct chtab *ch;
{
	/* SMP lock client handle table during access		*/
	smp_lock(&lk_nfschtable, LK_RETRY);
	ch->ch_inuse = FALSE;
	if (clwanted) {
		clwanted = 0;
		wakeup((caddr_t)chtable);
	}
	smp_unlock(&lk_nfschtable);
}

char *rpcstatnames[] = {
	"Success", "Can't encode arguments", "Can't decode result",
	"Unable to send", "Unable to receive", "Timed out",
	"Incompatible versions of RPC", "Authentication error",
	"Program unavailable", "Program/version mismatch",
	"Procedure unavailable", "Server can't decode arguments",
	"Remote system error", "Unknown host", "Port mapper failure",
	"Program not registered", "Failed (unspecified error)",
	"Unknown protocol", "Interrupted"
	};

char *rfsnames[] = {
	"null", "getattr", "setattr", "unused", "lookup", "readlink", "read",
	"unused", "write", "create", "remove", "rename", "link", "symlink",
	"mkdir", "rmdir", "readdir", "fsstat" };

/*
 * Back off for retransmission timeout, MAXTIMO is in 10ths of a sec
 */
#define MAXTIMO	300
#define backoff(tim)	((((tim) << 2) > MAXTIMO) ? MAXTIMO : ((tim) << 2))

int
rfscall(mi, which, xdrargs, argsp, xdrres, resp, cred)
	register struct mntinfo *mi;
	int	 which;
	xdrproc_t xdrargs;
	caddr_t	argsp;
	xdrproc_t xdrres;
	caddr_t	resp;
	struct ucred *cred;
{
	struct chtab *cht;
	CLIENT *client;
	register enum clnt_stat status;
	struct rpc_err rpcerr;
	struct timeval wait;
	struct ucred *newcred;
	int timeo;
	int user_told;
	bool_t tryagain;
	int hold_nfs_xid;
	extern nfs_hardpause();

	if (cred == NULL)
		panic("rfscall: NULL cred");

	smp_lock(&lk_nfsstat, LK_RETRY);
	clstat.ncalls++;
	clstat.reqs[which]++;
	smp_unlock(&lk_nfsstat);

	rpcerr.re_errno = 0;
	newcred = NULL;
	timeo = mi->mi_timeo;
	user_told = 0;
retry:
	cht = clget(mi, cred);
	client = cht->ch_client;

        /* NFS transaction ID */
	smp_lock(&lk_rpcxid, LK_RETRY);
        if (!nfs_next_xid)
                nfs_next_xid = timepick->tv_sec;
        client->cl_xid = nfs_next_xid++;
	smp_unlock(&lk_rpcxid);

	/*
	 * If hard mounted fs, retry call forever unless hard error occurs
	 */
	do {
		tryagain = FALSE;

		wait.tv_sec = timeo / 10;
		wait.tv_usec = 100000 * (timeo % 10);
		status = CLNT_CALL(client, which, xdrargs, argsp,
		    xdrres, resp, wait);
		switch (status) {
		case RPC_SUCCESS:
			break;

		/*
		 * Unrecoverable errors: give up immediately
		 */
		case RPC_AUTHERROR:
		case RPC_CANTENCODEARGS:
		case RPC_CANTDECODERES:
		case RPC_VERSMISMATCH:
		case RPC_PROGVERSMISMATCH:
		case RPC_CANTDECODEARGS:
			break;

		default:
			if (mi->mi_hard) {
				if (mi->mi_int && status == RPC_INTR) {
					tryagain = FALSE;
					break;
				} else {
					tryagain = TRUE;
 					if (status == RPC_INTR)
 						continue;
					timeo = backoff(timeo);
					if (!mi->mi_printed) {
						mi->mi_printed = 1;
      mprintf("NFS server %s not responding, still trying\n", mi->mi_hostname);
					}
					if (!user_told && u.u_procp->p_ttyp) {
						user_told = 1;
      uprintf("NFS server %s not responding, still trying\n", mi->mi_hostname);
					}
					/*
				 	 * Pause to free up the client handle 
				 	 * for a while. (minimum of 1
					 * second)
				 	 */
					hold_nfs_xid = client->cl_xid;
					clfree(cht);
					timeout(nfs_hardpause,
						(caddr_t)u.u_procp,
						MAX(((timeo * hz) / 10), hz));
					sleep((caddr_t)u.u_procp, PRIBIO);
        				cht = clget(mi, cred);
        				client = cht->ch_client;
        				client->cl_xid = hold_nfs_xid;
				}
			}
		}
	} while (tryagain);

	if (status != RPC_SUCCESS) {
		if (status == RPC_INTR) {
			rpcerr.re_status = RPC_INTR;
			rpcerr.re_errno = EINTR;
			u.u_error = EINTR;
		} else {
			CLNT_GETERR(client, &rpcerr);
			mprintf("NFS %s failed for server %s: %s\n",
				rfsnames[which], mi->mi_hostname,
				rpcstatnames[(int)status]);
			if (do_uprintf && u.u_procp->p_ttyp)
				uprintf("NFS %s failed for server %s: %s\n",
					rfsnames[which], mi->mi_hostname,
					rpcstatnames[(int)status]);
			if (!u.u_error)
				u.u_error = EIO; /* if in doubt ... */
		}
		smp_lock(&lk_nfsstat, LK_RETRY);
		clstat.nbadcalls++;
		smp_unlock(&lk_nfsstat);
		mi->mi_down = 1;
	} else if (resp && *(int *)resp == EACCES &&
		   newcred == NULL && cred->cr_uid == 0 &&
		   cred->cr_ruid != 0) {
		/*
		 * Boy is this a kludge!  If the reply status is EACCES
		 * it may be because we are root (no root net access).
		 * Check the real uid, if it isn't root make that
		 * the uid instead and retry the call.
		 */
		newcred = crdup(cred);
		cred = newcred;
		cred->cr_uid = cred->cr_ruid;
		clfree(cht);
		goto retry;
	} else if (mi->mi_hard) {
		if (mi->mi_printed) {
			mprintf("NFS server %s ok\n", mi->mi_hostname);
			mi->mi_printed = 0;
		}
		if (user_told) {
			uprintf("NFS server %s ok\n", mi->mi_hostname);
		}
	} else {
		mi->mi_down = 0;
	}

	clfree(cht);
	if (newcred) {
		crfree(newcred);
	}

	return (rpcerr.re_errno);
}

/*
 * Set vattr structure to a null value.
 */
void
vattr_null(vap)
	struct vattr *vap;
{
	register int n;
	register char *cp;

	n = sizeof(struct vattr);
	cp = (char *)vap;
	while (n--) {
		*cp++ = -1;
	}
}

int attrdebug = 0;

vattr_to_sattr(vap, sa)
	register struct vattr *vap;
	register struct nfssattr *sa;
{
	sa->sa_mode = vap->va_mode;
	sa->sa_uid = vap->va_uid;
	sa->sa_gid = vap->va_gid;
	sa->sa_size = vap->va_size;
	sa->sa_atime  = vap->va_atime;
	sa->sa_mtime  = vap->va_mtime;

if (attrdebug) {
	printf("vattr_to_sattr: atime: %d, %d    mtime: %d, %d\n",
		sa->sa_atime.tv_sec, sa->sa_atime.tv_usec,
		sa->sa_mtime.tv_sec, sa->sa_mtime.tv_usec);
}

}

setdiropargs(da, nm, dvp)
	struct nfsdiropargs *da;
	char *nm;
	struct vnode *dvp;
{

	da->da_fhandle = *vtofh(dvp);
	da->da_name = nm;
}

/*
 * Return a gnode for the given fhandle.  If no gnode exists for this
 * fhandle create one and put it in the gnode table.  If a gnode is found,
 * return it with its reference count incremented.
 */

struct rnode_data {
	fhandle_t	*rn_fh;
	struct nfsfattr	*rn_attr;
	int		rn_newnode;
};

/*
 * Return a gnode for the given fhandle.  If no gnode exists for this
 * fhandle create one and put it in the gnode table.  If a gnode is found,
 * return it with its reference count incremented.  KLUDGE: the GFS buffer
 * hashing scheme will not work unless we reuse the same gnode slot when
 * we reopen a given file.  For this reason we leave gnodes on their
 * hash chain when we free them, so gget can find them and reclaim them.
 * We must take care when reinitializing reclaimed gnodes; some parts must
 * be reinitialized and others must be kept from the last invocation (such
 * as the modify time).
 */
struct vnode *
makenfsnode(fh, attr, vfsp, mp)
	fhandle_t *fh;
	struct nfsfattr *attr;
	struct vfs *vfsp;
	struct mount *mp;
{
	char newnode = 0;
	register struct gnode *gp;
	struct rnode_data rnd;
	register struct vnode *vp;
	gno_t	gno;

	rnd.rn_fh = fh;
	rnd.rn_attr = attr;
	rnd.rn_newnode = 0;

	gno = (attr) ? attr->na_nodeid : fh->fh_fno;

	gp = gget(mp, gno, 1, &rnd);
	if (gp != NULL) {
		gfs_unlock(gp);
		if (attr && rnd.rn_newnode == 0)
			nfs_attrcache(gp, attr, SFLUSH);
	}
	return ((struct vnode *)gp);
}

int
nfs_match(gp, rdp)
	struct gnode *gp;
	struct rnode_data *rdp;
{
	return(!bcmp(vtofh((struct vnode *)gp), rdp->rn_fh,
		sizeof(fhandle_t)));
}

/*
	 * Initialize the gnode if this is the first reference (the gnode
	 * is either new or reclaimed).
	 */

nfs_ginit(gp, iflag, rdp)
	struct gnode *gp;
	int iflag;
	struct rnode_data *rdp;
{
	register struct vnode *vp = (struct vnode *)gp;
	register struct nfsfattr *attr;
	int type;

/*
 * if we get here via ggrab, rdp is null, so we need to return an
 * error, so ufs_namei will keep its hands off this gnode...
 */
	if (rdp == NULL)	
		return(NULL);

	attr = rdp->rn_attr;
 	gp->g_ops = nfs_gnode_ops;

	bzero(vtor((struct vnode *)gp), sizeof(struct rnode));
	vtor(vp)->r_fh = *(rdp->rn_fh);

	if (attr)
		vp->v_type = (enum vtype)attr->na_type;
	vp->v_vfsp = MP_TO_VFSP(gp->g_mp);
	((struct mntinfo *)(vp->v_vfsp->vfs_data))->mi_refct++;
	if (attr)
		nfs_attrcache(gp, attr,
			      (iflag == NEW_GNODE) ? NOFLUSH : SFLUSH);
	type = gp->g_mode & GFMT;
	if ((type == GFCHR) || (type == GFBLK) || (type == GFPORT))
		specvp(gp);

	rdp->rn_newnode = 1;
	return(1);
}

/*
 * Kludge an "invisible temp name from an existing name.
 * Attempt to make a unique one,
 * but can't guarantee this. This new name is used for the
 * "rename game" - renaming an open file that is unlinked over NFS.
 * Use hostid, pid, and a generation # in the temp name.
 */

#define	PREFIXLEN	4
static char prefix[PREFIXLEN+1] = ".nfs";
u_long rename_gennum;

char *
newname(s)
	char *s;
{
	register char *news;
	register char *s1, *s2;
	register int i;
	register u_long id;

	kmem_alloc(news, char *, (u_int)NFS_MAXNAMLEN, KM_NFS);
	for (s1 = news, s2 = prefix; s2 < &prefix[PREFIXLEN]; ) {
		*s1++ = *s2++;
	}

	id = hostid;
	for (i = 0; i < sizeof(id)*2; i++) {
		*s1++ = "0123456789ABCDEF"[id & 0x0f];
		id = id >> 4;
	}
	id = u.u_procp->p_pid;
	for (i = 0; i < sizeof(id)*2; i++) {
		*s1++ = "0123456789ABCDEF"[id & 0x0f];
		id = id >> 4;
	}
	if (!rename_gennum)
		rename_gennum = timepick->tv_usec;
	id = rename_gennum++;
	for (i = 0; i < sizeof(id)*2; i++) {
		*s1++ = "0123456789ABCDEF"[id & 0x0f];
		id = id >> 4;
	}

	*s1 = '\0';
	return (news);
}


/*
 * Server side utilities
 */

vattr_to_nattr(vap, na)
	register struct vattr *vap;
	register struct nfsfattr *na;
{

	na->na_type = (enum nfsftype)vap->va_type;
	na->na_mode = vap->va_mode;
	na->na_uid = vap->va_uid;
	na->na_gid = vap->va_gid;
	na->na_fsid = vap->va_fsid;
	na->na_nodeid = vap->va_nodeid;
	na->na_nlink = vap->va_nlink;
	na->na_size = vap->va_size;
	na->na_atime = vap->va_atime;
	na->na_mtime = vap->va_mtime;
	na->na_ctime = vap->va_ctime;
	na->na_rdev = vap->va_rdev;
	na->na_blocks = vap->va_blocks;
	na->na_blocksize = vap->va_blocksize;

	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * VFIFO type to the special over-the-wire type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if ((vap->va_mode&GFMT) == GFPORT) {
		na->na_type = (enum nfsftype)VCHR;
		na->na_rdev = NFS_FIFO_DEV;
		na->na_mode = (na->na_mode&~GFMT)|GFCHR; 
	}
}

sattr_to_vattr(sa, vap)
	register struct nfssattr *sa;
	register struct vattr *vap;
{
	vattr_null(vap);
	vap->va_mode = sa->sa_mode;
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;
	vap->va_atime = sa->sa_atime;
	vap->va_mtime = sa->sa_mtime;

	if (attrdebug) {
		printf("sattr_to_vattr: atime: %d, %d    mtime: %d, %d\n",
			sa->sa_atime.tv_sec, sa->sa_atime.tv_usec,
			sa->sa_mtime.tv_sec, sa->sa_mtime.tv_usec);
	}
}

/*
 * Make an fhandle from a ufs gnode
 */
makefh(fh, gp, enumber, egen)
	register fhandle_t *fh;
	struct gnode *gp;
	u_long enumber, egen;
{
	if (gp->g_mp->m_ops == nfs_mount_ops)
		return(EREMOTE);
	bzero((caddr_t)fh, NFS_FHSIZE);
	fh->fh_fsid = gp->g_dev;
	fh->fh_fno = gp->g_number;
	fh->fh_fgen = gp->g_gennum;

	fh->fh_eno = enumber;
	fh->fh_egen = egen;
	return (0);
}

/*
 * Global "constants" and defines used for stale file handle processing
 */
long stalefh_count = 0;

long nfs_n_nomount = 0;
long nfs_last_nomount = 0;

long nfs_nomount_thresh = 10;
long nfs_nomount_timeout = 600;

long nfs_noexport_thresh = 10;
long nfs_noexport_timeout = 600;

long nfs_stale_thresh = 10;
long nfs_stale_timeout = 600;

#define NFS_N_NOEXPORT(mp) ((mp)->m_fs_data->fd_spare[100])
#define NFS_LAST_NOEXPORT(mp) ((mp)->m_fs_data->fd_spare[101])
#define NFS_N_STALE(mp) ((mp)->m_fs_data->fd_spare[102])
#define NFS_LAST_STALE(mp) ((mp)->m_fs_data->fd_spare[103])

/*
 * Convert a fhandle into a gnode.  Uses the inode number in the
 * fhandle (fh_fno) to get the locked inode.  The inode is unlocked
 * and used to get the gnode.  WARNING: users of this routine must
 * do a VN_RELE on the gnode when they are done with it.
 */
struct vnode *
fhtovp(fh, xprt, which, xpd)
	fhandle_t *fh;
	SVCXPRT *xprt;
	int	which;
	struct exportdata *xpd;
{

	register struct mount *mp;
 	register struct gnode *gp;
        register int *grp;      /* group ID pointer */
	register struct gnode *rgp;
	struct export *ep;
	extern struct mount *getmp();
	extern struct gnode *fref();
	int	print;

	print = 1; 
	/*
	 * Check that the file system is mounted on and
	 * if so get a ref on it.
	 */
	GETMP(mp, fh->fh_fsid);
	if ((rgp = fref(mp, fh->fh_fsid)) == NULL) {
		/*
		 * These messages can be a pain - sometimes you want them,
		 * sometimes (most of the time) you don't.
		 * Print the first nfs_nomount_thresh, then keep a
		 * timestamp and throw the rest away until nfs_nomount_timeout
		 * seconds have passed; after that, start over with the
		 * same algorithm.
		 *
		 * If nfs_nomount_thresh is set (via adb or the like)
		 * to zero, then all messages will be printed.
		 *
		 * If nfs_nomount_thresh is positive, and
		 * nfs_nomount_timeout is set to zero,
		 * then no more than nfs_nomount_thresh messages will be
		 * printed.
		 */
		if (nfs_nomount_thresh) {
			++nfs_n_nomount;
			if (nfs_n_nomount == nfs_nomount_thresh) {
				nfs_last_nomount = timepick->tv_sec;
			}
			else if (nfs_n_nomount > nfs_nomount_thresh) {
				if (nfs_nomount_timeout &&
					timepick->tv_sec > nfs_last_nomount +
					nfs_nomount_timeout) {
						nfs_n_nomount = 1;
				}
				else {
					print = 0;
				}
			}
		}
		if (print) {
			printf("NFS server: fs(%d,%d) not mounted,",
				major(fh->fh_fsid), minor(fh->fh_fsid));
			printf(" %s, client address = %u.%u.%u.%u\n",
				rfsnames[which],
				xprt->xp_raddr.sin_addr.s_net,
				xprt->xp_raddr.sin_addr.s_host,
				xprt->xp_raddr.sin_addr.s_lh,
				xprt->xp_raddr.sin_addr.s_impno);
			}
		goto stale;
	}

	/*
	 * verify that the filesystem is exported
	 */
	if (!(xpd->x_flags & M_EXPORTED)) {
		/*
		 * See comment above for `not mounted ' messages.
		 * A similar algorithm is used here.
		 */
		if (nfs_noexport_thresh) {
			++NFS_N_NOEXPORT(mp);
			if (NFS_N_NOEXPORT(mp) == nfs_noexport_thresh) {
				NFS_LAST_NOEXPORT(mp) =
					(u_long)timepick->tv_sec;
			}
			else if (NFS_N_NOEXPORT(mp) > nfs_noexport_thresh) {
				if (nfs_noexport_timeout &&
					(u_long)timepick->tv_sec >
					NFS_LAST_NOEXPORT(mp) +
					nfs_noexport_timeout) {
						NFS_N_NOEXPORT(mp) = 1;
				}
				else {
					print = 0;
				}
			}
		}
		if (print) {
			printf("NFS server: unexported fs(%d, %d) file %d,",
			       major(fh->fh_fsid), minor(fh->fh_fsid),
			       fh->fh_fno);
			printf(" %s, client address = %u.%u.%u.%u\n",
			       rfsnames[which],
			       xprt->xp_raddr.sin_addr.s_net,
			       xprt->xp_raddr.sin_addr.s_host,
			       xprt->xp_raddr.sin_addr.s_lh,
			       xprt->xp_raddr.sin_addr.s_impno);
		}
		grele(rgp);
		goto stale;
	}


 	gp = (struct gnode *) gget(mp, fh->fh_fno, NOMOUNT, NULL);
 	if (gp == NULL) {
		printf("NFS server: couldn't get fs(%d, %d) file %d,",
			major(fh->fh_fsid), minor(fh->fh_fsid), 
			fh->fh_fno);
		grele(rgp);
		goto stale; 
	}
	/*
	 * Release ref on file system
	 */
	grele(rgp);


 	if (gp->g_gennum != fh->fh_fgen || gp->g_nlink <=0) {
		/*
		 * See comment above for `not mounted ' messages.
		 * A similar algorithm is used here.
		 */
		if (nfs_stale_thresh) {
			++NFS_N_STALE(mp);
			if (NFS_N_STALE(mp) == nfs_stale_thresh) {
				NFS_LAST_STALE(mp) = (u_long)timepick->tv_sec;
			}
			else if (NFS_N_STALE(mp) > nfs_stale_thresh) {
				if (nfs_stale_timeout &&
					(u_long) timepick->tv_sec >
					NFS_LAST_STALE(mp) +
					nfs_stale_timeout) {
						NFS_N_STALE(mp) = 1;
				}
				else {
					print = 0;
				}
			}
		}
		if (print) {
	     printf("NFS server: stale file handle fs(%d,%d) file %d gen %d\n",
			       major(fh->fh_fsid), minor(fh->fh_fsid), 
			       fh->fh_fno, fh->fh_fgen);
			printf("             local gen %d nlink %d,",
				gp->g_gennum, gp->g_nlink);
			printf(" %s, client address = %u.%u.%u.%u\n",
				rfsnames[which],
				xprt->xp_raddr.sin_addr.s_net,
				xprt->xp_raddr.sin_addr.s_host,
				xprt->xp_raddr.sin_addr.s_lh,
				xprt->xp_raddr.sin_addr.s_impno);
			}
		if (gp->g_count == 1) {
			gp->g_init = RECLAIM_GNODE;
			gfs_unlock(gp);
			grele(gp);
		} else
			gput(gp);
		goto stale;
	}

        /*
         * set root mapping
         */
        if (u.u_uid == 0) {
                u.u_uid = u.u_ruid = xpd->x_rootmap;
                /*
                 * HACK:
                 *      If mapping root to nobody (-2), then zap
                 *      all of the client's root group IDs.
                 *      This fills a security hole while we decide
                 *      what (if anything) to do about UID and GID
                 *      mappings.
                 */
                if (u.u_uid == -2) {
                        u.u_rgid = u.u_gid = -2;
                        u.u_cred->cr_groups[0] = -2;
                        for (grp = &u.u_cred->cr_groups[1];
                             grp < &u.u_cred->cr_groups[NGROUPS];
                             grp++) {
                                *grp = NOGROUP;
                        }
                }
        }
 	return ((struct vnode *)gp);

stale:
	/* NULL return means fhandle not converted */
	stalefh_count++;
	return (NULL);
}

struct export *exported;

/*
 * Exportfs system call
 */
exportfs()
{
	register struct a {
		short option;	/* 1=create, 2=delete, 3=read */
		u_int *cookie;
		struct exportfsdata *buf;
	} *uap;
	register struct gnode *gp=NULL;
	register int error;
	register struct nameidata *ndp = &u.u_nd;
	struct export *ep, **tail, *tmp, *tmp2;
	struct exportfsdata *e;
	int cookie, i;
	uap = (struct a *)u.u_ap;

	if (!suser())
		return;

	KM_ALLOC(e, struct exportfsdata *, sizeof(struct exportfsdata), 
		 KM_NFS, KM_CLEAR);
	if (e == NULL) {
		u.u_error = EIO;
		return;
	}
	if (uap->option != EXPORTFS_READ) {
		if (uap->buf != NULL) {
			u.u_error = copyin((caddr_t)uap->buf, (caddr_t)e,
					   sizeof (struct exportfsdata));
		}
		else {
			KM_FREE(e, KM_NFS);
			u.u_error = EINVAL;
			return;
		}
	}
	switch (uap->option) {

	case EXPORTFS_CREATE:


		KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NFS, KM_NOARG);
		if (ndp->ni_dirp == NULL) {
			KM_FREE(e, KM_NFS);
			u.u_error = EIO;
			return;
		}

		bcopy(e->e_path, ndp->ni_dirp, MAXPATHLEN);

		ndp->ni_nameiop = LOOKUP | FOLLOW;
		gp = GNAMEI(ndp);
		KM_FREE(ndp->ni_dirp, KM_NFS);
		if (gp == NULL) {
			printf("exportfs: namei returned null gp - errno= %d\n",
			       u.u_error);
			KM_FREE(e, KM_NFS);
			return;
		}
		if (u.u_error)  {
			KM_FREE(e, KM_NFS);
			return;
		}

		KM_ALLOC(ep, struct export *, sizeof(struct export), KM_NFS,
			 KM_CLEAR);
		if (ep == NULL) {
			gput(gp);
			KM_FREE(e, KM_NFS);
			u.u_error = EIO;
			return;
		}
		ep->e_pathlen = strlen(e->e_path)+1;
		KM_ALLOC(ep->e_path, char *, ep->e_pathlen, KM_NFS, KM_CLEAR);
		if (ep->e_path == NULL) {
			gput(gp);
			KM_FREE(e, KM_NFS);
			KM_FREE(ep, KM_NFS);
			u.u_error = EIO;
			return;
		}
		bcopy(e->e_path, ep->e_path, ep->e_pathlen);

		ep->e_fsid = gp->g_dev;
		ep->e_gnum = gp->g_number;
		ep->e_gen = gp->g_gennum;

	     	/* Set root uid mapping */
		ep->e_rootmap = e->e_rootmap;

		ep->e_flags = e->e_flags;
		KM_FREE(e, KM_NFS);

		/*
		 * Commit the new information to the export list, making
		 * sure to delete the old entry for the fs, if one exists.
		 */
		smp_lock(&lk_gnode, LK_RETRY);
		tail = &exported;
		tmp=NULL;
		while (*tail != NULL) {
			if (((*tail)->e_fsid == gp->g_dev) &&
			    ((*tail)->e_gnum == gp->g_number) &&
			    ((*tail)->e_gen == gp->g_gennum)) {
				(*tail)->e_rootmap = ep->e_rootmap;
				(*tail)->e_flags = ep->e_flags;
				smp_unlock(&lk_gnode);
				KM_FREE(ep->e_path, KM_NFS);
				KM_FREE(ep, KM_NFS);
				gput(gp);
				return;
			} else {
				tmp=(*tail);
				tail = &(*tail)->e_next;
			}
		}
		ep->e_next = exported;
		exported = ep;
		smp_unlock(&lk_gnode);
		gput(gp);
		return;


	case EXPORTFS_REMOVE:

		smp_lock(&lk_gnode, LK_RETRY);
		tail = &exported;
		tmp=NULL;
		while (*tail != NULL) {
			if (strcmp((*tail)->e_path,e->e_path) == 0) {
				if (tmp != NULL) {
					tmp2 = *tail;
					tmp->e_next = (*tail)->e_next;
					smp_unlock(&lk_gnode);
					KM_FREE(e, KM_NFS);
					exportfree(tmp2);
					return (0);
				}
				else {
					tmp = *tail;
					exported = (*tail)->e_next;
					smp_unlock(&lk_gnode);
					KM_FREE(e, KM_NFS);
					exportfree(tmp);
					return (0);
				}
			} else {
				tmp=(*tail);
				tail = &(*tail)->e_next;
			}
		}
		smp_unlock(&lk_gnode);
		KM_FREE(e, KM_NFS);
		u.u_error = ENOENT;
		return;

	case EXPORTFS_READ:

		if (u.u_error = copyin(uap->cookie, &cookie, 
				       sizeof(cookie)))
	                break;
			
		if (exported == NULL) {
			u.u_error = ENOENT;
			KM_FREE(e, KM_NFS);
			return;
		}
		if (cookie < 0) {
			u.u_error = EINVAL;
			KM_FREE(e, KM_NFS);
	                return;
		}
		i=0;

		smp_lock(&lk_gnode, LK_RETRY);
		ep = exported;
		while (ep != NULL) {
			if (i++ == cookie) {
				e->e_flags = ep->e_flags;
				e->e_fsid = ep->e_fsid;
				e->e_gnum = ep->e_gnum;
				e->e_gen = ep->e_gen;
				e->e_rootmap = ep->e_rootmap;
				bcopy(ep->e_path, e->e_path, ep->e_pathlen);
				if (ep->e_next == NULL)
					e->e_more = 0;
				else
					e->e_more = 1;

				break;
			} else {
				ep = ep->e_next;
			}
		}
		smp_unlock(&lk_gnode);

		cookie=i;
		copyout(&cookie, uap->cookie, sizeof(cookie));

		copyout((caddr_t)e, (caddr_t) uap->buf, 
			sizeof(struct exportfsdata));

			KM_FREE(e, KM_NFS);
		
	}
}



/*
 * Free an entire export list node
 */
exportfree(exi)
	struct export *exi;
{
	KM_FREE(exi->e_path, KM_NFS);
	KM_FREE(exi, KM_NFS);
}

/*
 * General utilities
 */

/*
 * Returns the prefered transfer size in bytes based on
 * what network interfaces are available.
 */
nfstsize()
{
	return (8192);
}

nfs_hardpause(chan)
	caddr_t chan;
{
	wakeup((caddr_t)chan);
}

/* Routine to initialize NFS SMP locks		*/

nfsinit()

{
	lockinit(&lk_nfschtable, &lock_nfschtable_d);
	lockinit(&lk_nfsbiod, &lock_nfs_biod_d);
	lockinit(&lk_nfsdnlc, &lock_nfsdnlc_d);
	lockinit(&lk_nfsstat, &lock_nfsstat_d);
	lockinit(&lk_nfsrrok, &lock_udpdata_d);
	lockinit(&lk_nfsargs, &lock_nfsargs_d);
}

#define NFS_SMALL	1
#define NFS_MEDIUM	2
#define NFS_LARGE	3
#define NFS_XLARGE	4

nfs_system_size()
{
	extern int maxusers;

	if (maxusers < 32)
		return (NFS_SMALL);

	if (maxusers < 128)
		return (NFS_MEDIUM);

	if (maxusers < 256)
		return (NFS_LARGE);

	return (NFS_XLARGE);
}


