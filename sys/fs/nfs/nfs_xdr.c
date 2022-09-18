#ifndef lint
static	char	*sccsid = "@(#)nfs_xdr.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 * 28 Aug 89 -- condylis
 *	Added SMP locking around sleep/wakeup mechanism for
 *	returning of type 2 mbuf data in rrokfree() and rrokwakeup().
 */


#include "../h/param.h"
#include "../h/mount.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/dir.h"
#include "../h/mbuf.h"
#include "../h/fs_types.h"
#include "../h/smp_lock.h"
#include "../net/netinet/in.h"
#include "../nfs/nfs_clnt.h"
#include "../net/rpc/types.h"
#include "../net/rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../nfs/vfs.h"
#include "../nfs/vnode.h"

#ifdef GFSDEBUG
extern short GFS[];
#endif

#ifdef NFSDEBUG
extern int nfsdebug;

char *xdropnames[] = {"encode", "decode", "free"};
#endif

/*
 * SMP lock for sleep/wakeup mechanism around releasing of
 * type 2 mbuf data by networking code.
 */
struct	lock_t	lk_nfsrrok;
/*
 * These are the XDR routines used to serialize and deserialize
 * the various structures passed as parameters accross the network
 * between NFS clients and servers.
 */

/*
 * File access handle
 * The fhandle struct is treated a opaque data on the wire
 */
bool_t
xdr_fhandle(xdrs, fh)
	XDR *xdrs;
	fhandle_t *fh;
{

	if (xdr_opaque(xdrs, (caddr_t)fh, NFS_FHSIZE)) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_fhandle: %s %o %d\n",
		    xdropnames[(int)xdrs->x_op], fh->fh_fsid, fh->fh_fno);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_fhandle %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}


/*
 * Arguments to remote write and writecache
 */
bool_t
xdr_writeargs(xdrs, wa)
	XDR *xdrs;
	struct nfswriteargs *wa;
{
	if (xdr_fhandle(xdrs, &wa->wa_fhandle) &&
	    xdr_long(xdrs, (long *)&wa->wa_begoff) &&
	    xdr_long(xdrs, (long *)&wa->wa_offset) &&
	    xdr_long(xdrs, (long *)&wa->wa_totcount) &&
	    xdr_bytes(xdrs, &wa->wa_data, (u_int *)&wa->wa_count,
	      NFS_MAXDATA) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_writeargs: %s off %d ct %d\n",
		    xdropnames[(int)xdrs->x_op],
		    wa->wa_offset, wa->wa_totcount);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_writeargs: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}


/*
 * File attributes
 */
bool_t
xdr_fattr(xdrs, na)
	XDR *xdrs;
	register struct nfsfattr *na;
{
	register long *ptr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_fattr: %s\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	if (xdrs->x_op == XDR_ENCODE) {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			IXDR_PUT_ENUM(ptr, na->na_type);
			IXDR_PUT_LONG(ptr, na->na_mode);
			IXDR_PUT_LONG(ptr, na->na_nlink);
			IXDR_PUT_LONG(ptr, na->na_uid);
			IXDR_PUT_LONG(ptr, na->na_gid);
			IXDR_PUT_LONG(ptr, na->na_size);
			IXDR_PUT_LONG(ptr, na->na_blocksize);
			IXDR_PUT_LONG(ptr, na->na_rdev);
			IXDR_PUT_LONG(ptr, na->na_blocks);
			IXDR_PUT_LONG(ptr, na->na_fsid);
			IXDR_PUT_LONG(ptr, na->na_nodeid);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_usec);
			return (TRUE);
		}
	} else {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			na->na_type = IXDR_GET_ENUM(ptr, enum nfsftype);
			na->na_mode = IXDR_GET_LONG(ptr);
			na->na_nlink = IXDR_GET_LONG(ptr);
			na->na_uid = IXDR_GET_LONG(ptr);
			na->na_gid = IXDR_GET_LONG(ptr);
			na->na_size = IXDR_GET_LONG(ptr);
			na->na_blocksize = IXDR_GET_LONG(ptr);
			na->na_rdev = IXDR_GET_LONG(ptr);
			na->na_blocks = IXDR_GET_LONG(ptr);
			na->na_fsid = IXDR_GET_LONG(ptr);
			na->na_nodeid = IXDR_GET_LONG(ptr);
			na->na_atime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_atime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_usec = IXDR_GET_LONG(ptr);
			return (TRUE);
		}
	}
	if (xdr_enum(xdrs, (enum_t *)&na->na_type) &&
	    xdr_u_long(xdrs, &na->na_mode) &&
	    xdr_u_long(xdrs, &na->na_nlink) &&
	    xdr_u_long(xdrs, &na->na_uid) &&
	    xdr_u_long(xdrs, &na->na_gid) &&
	    xdr_u_long(xdrs, &na->na_size) &&
	    xdr_u_long(xdrs, &na->na_blocksize) &&
	    xdr_u_long(xdrs, &na->na_rdev) &&
	    xdr_u_long(xdrs, &na->na_blocks) &&
	    xdr_u_long(xdrs, &na->na_fsid) &&
	    xdr_u_long(xdrs, &na->na_nodeid) &&
	    xdr_timeval(xdrs, &na->na_atime) &&
	    xdr_timeval(xdrs, &na->na_mtime) &&
	    xdr_timeval(xdrs, &na->na_ctime) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_fattr: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Arguments to remote read
 */
bool_t
xdr_readargs(xdrs, ra)
	XDR *xdrs;
	struct nfsreadargs *ra;
{

	if (xdr_fhandle(xdrs, &ra->ra_fhandle) &&
	    xdr_long(xdrs, (long *)&ra->ra_offset) &&
	    xdr_long(xdrs, (long *)&ra->ra_count) &&
	    xdr_long(xdrs, (long *)&ra->ra_totcount) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_readargs: %s off %d ct %d\n",
		    xdropnames[(int)xdrs->x_op],
		    ra->ra_offset, ra->ra_totcount);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_raedargs: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Info necessary to free the bp which is also an mbuf
 */
struct rrokinfo {
	int	(*func)();
	int	done;
	struct vnode *vp;
	struct buf *bp;
};
	
static
rrokfree(rip)
	struct rrokinfo *rip;
{
	int s;
	struct mbuf *n;

	s = splimp();
	smp_lock(&lk_nfsrrok, LK_RETRY);
	while (rip->done == 0) {
		sleep_unlock((caddr_t)rip, PZERO-1, &lk_nfsrrok);
		smp_lock(&lk_nfsrrok, LK_RETRY);
	}

	smp_unlock(&lk_nfsrrok);
	(void) splx(s);
	VOP_BRELSE(rip->vp, rip->bp);
	VN_RELE(rip->vp);
	MFREE(dtom(rip), n);
#ifdef lint
	n = n;
#endif lint
}

/*
 * Wake up user process to free bp and vp (rrokfree)
 */
static
rrokwakeup(rip)
	struct rrokinfo *rip;
{

	int s;

	s = splimp();
	smp_lock(&lk_nfsrrok, LK_RETRY);
	rip->done = 1;
	smp_unlock(&lk_nfsrrok);
	(void) splx(s);
	wakeup((caddr_t)rip);
}

/*
 * Status OK portion of remote read reply
 */
bool_t
xdr_rrok(xdrs, rrok)
	XDR *xdrs;
	struct nfsrrok *rrok;
{

	if (xdr_fattr(xdrs, &rrok->rrok_attr)) {
		if (xdrs->x_op == XDR_ENCODE && rrok->rrok_bp) {
			/* server side */
			struct mbuf *m;
			struct rrokinfo *rip;

			MGET(m, M_WAIT, MT_DATA);
			if (m == NULL) {
				printf("xdr_rrok: FAILED, can't get mbuf\n");
				/*
				 *  Release resources since rrokfree
				 *  will not be called
				 */
				VOP_BRELSE(rrok->rrok_vp, rrok->rrok_bp);
				VN_RELE(rrok->rrok_vp);
				return(FALSE);
			}
			rip = mtod(m, struct rrokinfo *);
			rip->func = rrokfree;
			rip->done = 0;
			rip->vp = rrok->rrok_vp;
			rip->bp = rrok->rrok_bp;
			xdrs->x_public = (caddr_t)rip;
			if (xdrmbuf_putbuf(xdrs, rrok->rrok_data,
			    (u_int)rrok->rrok_count, rrokwakeup, (int)rip)) {
#ifdef NFSDEBUG
				dprint(nfsdebug, 6, "xdr_rrok: %s %d addr %x\n",
				    xdropnames[(int)xdrs->x_op],
				    rrok->rrok_count, rrok->rrok_data);
#endif
				return (TRUE);
			} else {
				rip->done = 1;
			}
		} else {			/* client side */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
			    (u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {
#ifdef NFSDEBUG
				dprint(nfsdebug, 6, "xdr_rrok: %s %d addr %x\n",
				    xdropnames[(int)xdrs->x_op],
				    rrok->rrok_count, rrok->rrok_data);
#endif
				return (TRUE);
			}
		}
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rrok: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim rdres_discrim[2] = {
	{ (int)NFS_OK, xdr_rrok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Reply from remote read
 */
bool_t
xdr_rdresult(xdrs, rr)
	XDR *xdrs;
	struct nfsrdresult *rr;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_rdresult: %s\n", xdropnames[(int)xdrs->x_op]);
#endif
	if (xdr_union(xdrs, (enum_t *)&(rr->rr_status),
	      (caddr_t)&(rr->rr_ok), rdres_discrim, xdr_void) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rdresult: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * File attributes which can be set
 */
bool_t
xdr_sattr(xdrs, sa)
	XDR *xdrs;
	struct nfssattr *sa;
{

	if (xdr_u_long(xdrs, &sa->sa_mode) &&
	    xdr_u_long(xdrs, &sa->sa_uid) &&
	    xdr_u_long(xdrs, &sa->sa_gid) &&
	    xdr_u_long(xdrs, &sa->sa_size) &&
	    xdr_timeval(xdrs, &sa->sa_atime) &&
	    xdr_timeval(xdrs, &sa->sa_mtime) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6,
		    "xdr_sattr: %s mode %o uid %d gid %d size %d\n",
		    xdropnames[(int)xdrs->x_op], sa->sa_mode, sa->sa_uid,
		    sa->sa_gid, sa->sa_size);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_sattr: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim attrstat_discrim[2] = {
	{ (int)NFS_OK, xdr_fattr },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Reply status with file attributes
 */
bool_t
xdr_attrstat(xdrs, ns)
	XDR *xdrs;
	struct nfsattrstat *ns;
{

	if (xdr_union(xdrs, (enum_t *)&(ns->ns_status),
	      (caddr_t)&(ns->ns_attr), attrstat_discrim, xdr_void) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_attrstat: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], ns->ns_status);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_attrstat: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * NFS_OK part of read sym link reply union
 */
bool_t
xdr_srok(xdrs, srok)
	XDR *xdrs;
	struct nfssrok *srok;
{

	if (xdr_bytes(xdrs, &srok->srok_data, (u_int *)&srok->srok_count,
	    NFS_MAXPATHLEN) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_srok: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim rdlnres_discrim[2] = {
	{ (int)NFS_OK, xdr_srok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Result of reading symbolic link
 */
bool_t
xdr_rdlnres(xdrs, rl)
	XDR *xdrs;
	struct nfsrdlnres *rl;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_rdlnres: %s\n", xdropnames[(int)xdrs->x_op]);
#endif
	if (xdr_union(xdrs, (enum_t *)&(rl->rl_status),
	      (caddr_t)&(rl->rl_srok), rdlnres_discrim, xdr_void) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rdlnres: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Arguments to readdir
 */
bool_t
xdr_rddirargs(xdrs, rda)
	XDR *xdrs;
	struct nfsrddirargs *rda;
{
	if (xdr_fhandle(xdrs, &rda->rda_fh) &&
	    xdr_u_long(xdrs, &rda->rda_offset) &&
	    xdr_u_long(xdrs, &rda->rda_count) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6,
		    "xdr_rddirargs: %s fh %o %d, off %d, cnt %d\n",
		    xdropnames[(int)xdrs->x_op],
		    rda->rda_fh.fh_fsid, rda->rda_fh.fh_fno, rda->rda_offset,
		    rda->rda_count);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rddirargs: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Directory read reply:
 * union (enum status) {
 *	NFS_OK: entlist;
 *		boolean eof;
 *	default:
 * }
 *
 * Directory entries
 *	struct  direct {
 *		u_long  d_fileno;	       * inode number of entry *
 *		u_short d_reclen;	       * length of this record *
 *		u_short d_namlen;	       * length of string in d_name *
 *		char    d_name[MAXNAMLEN + 1];  * name no longer than this *
 *	};
 * are on the wire as:
 * union entlist (boolean valid) {
 * 	TRUE: struct dirent;
 *	      u_long nxtoffset;
 * 	      union entlist;
 *	FALSE:
 * }
 * where dirent is:
 * 	struct dirent {
 *		u_long	de_fid;
 *		string	de_name<NFS_MAXNAMELEN>;
 *	}
 */

#define	nextdp(dp)	((struct direct *)((int)(dp) + (dp)->d_reclen))
#undef DIRSIZ
#define DIRSIZ(dp)	(sizeof(struct direct) - MAXNAMLEN + (dp)->d_namlen)

/*
 * ENCODE ONLY
 */
bool_t
xdr_putrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsrddirres *rd;
{
	register struct direct *dp;
	register int size;
	u_long offset;
	char *name;
	u_int namlen;
	int xdrpos;
	bool_t true = TRUE;
	bool_t false = FALSE;

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_putrddirres: %s size %d offset %d\n",
	    xdropnames[(int)xdrs->x_op], rd->rd_size, rd->rd_offset);
#endif
	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}
	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}

	xdrpos = XDR_GETPOS(xdrs);
	for (offset = rd->rd_offset, size = rd->rd_size, dp = rd->rd_entries;
	     size > 0;
	     size -= dp->d_reclen, dp = nextdp(dp) ) {
		if (dp->d_reclen == 0 || DIRSIZ(dp) > dp->d_reclen) {
#ifdef NFSDEBUG
			dprint(nfsdebug, 2, "xdr_putrddirres: bad directory\n");
#endif
			return (FALSE);
		}
		offset += dp->d_reclen;

#ifdef NFSDEBUG
		dprint(nfsdebug, 10,
		    "xdr_putrddirres: entry %d %s(%d) %d %d %d %d\n",
		    dp->d_fileno, dp->d_name, dp->d_namlen, offset,
		    dp->d_reclen, XDR_GETPOS(xdrs), size);
#endif
		if (dp->d_fileno == 0) {
			continue;
		}
		name = dp->d_name;
		namlen = dp->d_namlen;
		if (!xdr_bool(xdrs, &true) ||
		    !xdr_u_long(xdrs, &dp->d_fileno) ||
		    !xdr_bytes(xdrs, &name, &namlen,
			NFS_MAXNAMLEN) ||
		    !xdr_u_long(xdrs, &offset) ) {

				return (FALSE);
		}

		if (XDR_GETPOS(xdrs) - xdrpos >= rd->rd_bufallocsize) {
			rd->rd_eof = FALSE;
			break;
		}
	}
	if (!xdr_bool(xdrs, &false)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	return (TRUE);
}

/* there was an error in calculating the record length fo each directory */
#define	roundtoint(x)	(((x) + (sizeof(int) - 1)) & ~(sizeof(int) - 1))
#define	reclen(dp)	roundtoint(((dp)->d_namlen + 1 + sizeof(u_long) +\
				2 * sizeof(u_short)))

/*
 * DECODE ONLY
 */
bool_t
xdr_getrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsrddirres *rd;
{
	register struct direct *dp;
	register int size;
	bool_t valid;
	u_long offset = (u_long)-1;

	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "getrddires failed doing status\n");
#endif
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_getrddirres: %s size %d\n",
	    xdropnames[(int)xdrs->x_op], rd->rd_size);
#endif
	size = rd->rd_size;
	dp = rd->rd_entries;
	for (;;) {
		if (!xdr_bool(xdrs, &valid)) {
#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "getrddires failed doing valid\n");
#endif
			return (FALSE);
		}

		if (valid) {

#ifdef NFSDEBUG
			if (!xdr_u_long(xdrs, &dp->d_fileno)) {
				printf("entry err on fileno\n");
				return (FALSE);
			}
			if (!xdr_u_short(xdrs, &dp->d_namlen)) {
				printf("entry error on namlen\n");
				return (FALSE);
			}
			if (reclen(dp) > size) {
printf("entry error on reclen -- reclen %d  size %d\n", reclen(dp), size);
				return (FALSE);
			}
			if (!xdr_opaque(xdrs, dp->d_name,
				(u_int)dp->d_namlen)) {
				printf("entry error on name\n");
				return (FALSE);
			}
			if (!xdr_u_long(xdrs, &offset) ) {
				printf("entry error on offset\n");
				return (FALSE);
			}
#else NFSDEBUG
			if (!xdr_u_long(xdrs, &dp->d_fileno) ||
			    !xdr_u_short(xdrs, &dp->d_namlen) ||
			    (reclen(dp) > size) ||
			    !xdr_opaque(xdrs, dp->d_name, (u_int)dp->d_namlen)||
			    !xdr_u_long(xdrs, &offset) ) {
				return (FALSE);
			}
#endif NFSDEBUG

			dp->d_reclen = reclen(dp);
			dp->d_name[dp->d_namlen] = '\0';
#ifdef NFSDEBUG
			dprint(nfsdebug, 10,
			    "xdr_getrddirres: entry %d %s(%d) %d %d\n",
			    dp->d_fileno, dp->d_name, dp->d_namlen,
			    dp->d_reclen, offset);
#endif NFSDEBUG
		} else {
			break;
		}
		size -= dp->d_reclen;
		if (size <= 0) {
			return (FALSE);
		}
		dp = nextdp(dp);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	rd->rd_size = (int)dp - (int)(rd->rd_entries);
	rd->rd_offset = offset;
#ifdef NFSDEBUG
	dprint(nfsdebug, 6,
	    "xdr_getrddirres: returning size %d offset %d eof %d\n",
	    rd->rd_size, rd->rd_offset, rd->rd_eof);
#endif
	return (TRUE);
}

/*
 * Arguments for directory operations
 */
bool_t
xdr_diropargs(xdrs, da)
	XDR *xdrs;
	struct nfsdiropargs *da;
{

	if (xdr_fhandle(xdrs, &da->da_fhandle) &&
	    xdr_string(xdrs, &da->da_name, NFS_MAXNAMLEN) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_diropargs: %s %o %d '%s'\n",
		    xdropnames[(int)xdrs->x_op], da->da_fhandle.fh_fsid,
		    da->da_fhandle.fh_fno, da->da_name);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_diropargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * NFS_OK part of directory operation result
 */
bool_t
xdr_drok(xdrs, drok)
	XDR *xdrs;
	struct nfsdrok *drok;
{

	if (xdr_fhandle(xdrs, &drok->drok_fhandle) &&
	    xdr_fattr(xdrs, &drok->drok_attr) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_drok: FAILED\n");
#endif
	return (FALSE);
}

struct xdr_discrim diropres_discrim[2] = {
	{ (int)NFS_OK, xdr_drok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Results from directory operation 
 */
bool_t
xdr_diropres(xdrs, dr)
	XDR *xdrs;
	struct nfsdiropres *dr;
{

	if (xdr_union(xdrs, (enum_t *)&(dr->dr_status),
	      (caddr_t)&(dr->dr_drok), diropres_discrim, xdr_void) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_diropres: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], dr->dr_status);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_diropres: FAILED\n");
#endif
	return (FALSE);
}

/*
 * Time structure
 */
bool_t
xdr_timeval(xdrs, tv)
	XDR *xdrs;
	struct timeval *tv;
{

	if (xdr_long(xdrs, &tv->tv_sec) &&
	    xdr_long(xdrs, &tv->tv_usec) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_timeval: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to setattr
 */
bool_t
xdr_saargs(xdrs, argp)
	XDR *xdrs;
	struct nfssaargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
	    xdr_sattr(xdrs, &argp->saa_sa) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_saargs: %s %o %d\n",
		    xdropnames[(int)xdrs->x_op], argp->saa_fh.fh_fsid,
		    argp->saa_fh.fh_fno);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_saargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to create and mkdir
 */
bool_t
xdr_creatargs(xdrs, argp)
	XDR *xdrs;
	struct nfscreatargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->ca_da) &&
	    xdr_sattr(xdrs, &argp->ca_sa) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_creatargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to link
 */
bool_t
xdr_linkargs(xdrs, argp)
	XDR *xdrs;
	struct nfslinkargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->la_from) &&
	    xdr_diropargs(xdrs, &argp->la_to) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_linkargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to rename
 */
bool_t
xdr_rnmargs(xdrs, argp)
	XDR *xdrs;
	struct nfsrnmargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->rna_from) &&
	    xdr_diropargs(xdrs, &argp->rna_to) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rnmargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to symlink
 */
bool_t
xdr_slargs(xdrs, argp)
	XDR *xdrs;
	struct nfsslargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->sla_from) &&
	    xdr_string(xdrs, &argp->sla_tnm, (u_int)MAXPATHLEN) &&
	    xdr_sattr(xdrs, &argp->sla_sa) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_slargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * NFS_OK part of statfs operation
 */
xdr_fsok(xdrs, fsok)
	XDR *xdrs;
	struct nfsstatfsok *fsok;
{
#ifdef GFSDEBUG
	if(GFS[14])
		printf("xdr_fsok: called\n");
#endif
	if (xdr_long(xdrs, (long *)&fsok->fsok_tsize) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bsize) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_blocks) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bfree) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bavail) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6,
		    "xdr_fsok: %s tsz %d bsz %d blks %d bfree %d bavail %d\n",
		    xdropnames[(int)xdrs->x_op], fsok->fsok_tsize,
		    fsok->fsok_bsize, fsok->fsok_blocks, fsok->fsok_bfree,
		    fsok->fsok_bavail);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_fsok: FAILED\n");
#endif
	return (FALSE);
}

struct xdr_discrim statfs_discrim[2] = {
	{ (int)NFS_OK, xdr_fsok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * Results of statfs operation
 */
xdr_statfs(xdrs, fs)
	XDR *xdrs;
	struct nfsstatfs *fs;
{
#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "xdr_statfs: were here\n");
#endif
	if (xdr_union(xdrs, (enum_t *)&(fs->fs_status),
	      (caddr_t)&(fs->fs_fsok), statfs_discrim, xdr_void) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_statfs: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], fs->fs_status);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_statfs: FAILED\n");
#endif
	return (FALSE);
}
