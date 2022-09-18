#ifndef lint
static	char	*sccsid = "@(#)svc_kudp.c	4.4	(ULTRIX)	3/7/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by			        *
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
 * 25 Aug 90 -- chet
 *	Allow for Prestoserve in sizing of duplicate request cache.
 *	Shrink dupreq struct and fix NOTDUP macro.
 *
 *  9 Mar 90 -- chet
 *	Add dynamic allocation of duplicate request cache hash lists.
 *
 * 10 Dec 89 -- chet
 *	Re-do duplicate request cache allocation and locking scheme.
 *
 *  30 May 89 -- U. Sinkewicz
 *	Replaced smp_lock(&so->lk_socket, LK_RETRY) with SO_LOCK()
 *	as part of an smp bug fix.  Fix guarantees that socket doesn't
 *	changed while unlocked during sleeps or for the lock hierarchy.
 *
 *  6 Mar 89 -- chet
 *      Made duplicate request cache size a function of "system size".
 *
 * 19-Dec-88	condylis
 *	Restructured some of dupreq cache code because of inability to call
 *	kmem_alloc while holding a spin lock.
 *
 * 10-Jun-88	condylis
 *	Added smp locking to dupreq hash list and dupreq event counters
 *	and interaction with socket locks.  Added SMP locking for
 *	RPC server statistics.
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 * 11 May 87 -- chet
 *	Changed/added new server duplicate
 *	transaction cache interface routines.
 *
 */

/*
 * svc_kudp.c,
 * Server side for UDP/IP based RPC in the kernel.
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/time.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../h/kernel.h"
#include "../h/smp_lock.h"
#include "../h/cpudata.h"

extern struct timeval *timepick;

#define rpc_buffer(xprt) ((xprt)->xp_p1)

/*
 * Routines exported through ops vector.
 */
bool_t		svckudp_recv();
bool_t		svckudp_send();
enum xprt_stat	svckudp_stat();
bool_t		svckudp_getargs();
bool_t		svckudp_freeargs();
void		svckudp_destroy();

/*
 * Server transport operations vector.
 */
struct xp_ops svckudp_op = {
	svckudp_recv,		/* Get requests */
	svckudp_stat,		/* Return status */
	svckudp_getargs,	/* Deserialize arguments */
	svckudp_send,		/* Send reply */
	svckudp_freeargs,	/* Free argument data space */
	svckudp_destroy		/* Destroy transport handle */
};


struct mbuf	*ku_recvfrom();
void		xdrmbuf_init();

/*
 * Transport private data.
 * Kept in xprt->xp_p2.
 */
struct udp_data {
	int	ud_flags;			/* flag bits, see below */
	u_long 	ud_xid;				/* id */
	struct	mbuf *ud_inmbuf;		/* input mbuf chain */
	XDR	ud_xdrin;			/* input xdr stream */
	XDR	ud_xdrout;			/* output xdr stream */
	char	ud_verfbody[MAX_AUTH_BYTES];	/* verifier */
	struct	lock_t	u_lk_udpdata;		/* smp lock for type 2*/
						/* mbuf data  */
};

/*
 * Flags
 */
#define	UD_BUSY		0x001		/* buffer is busy */
#define	UD_WANTED	0x002		/* buffer wanted */

/*
 * Server statistics
 */
struct {
	int	rscalls;
	int	rsbadcalls;
	int	rsnullrecv;
	int	rsbadlen;
	int	rsxdrcall;
} rsstat;

extern struct lock_t lk_nfsstat;


/*
 * Create a transport record.
 * The transport record, output buffer, and private data structure
 * are allocated.  The output buffer is serialized into using xdrmem.
 * There is one transport record per user process which implements a
 * set of services.
 */
SVCXPRT *
svckudp_create(sock, port)
	struct socket	*sock;
	u_short		 port;
{
	register SVCXPRT	 *xprt;
	register struct udp_data *ud;

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_create so = %x, port = %d\n", sock, port);
#endif
	kmem_alloc(xprt, SVCXPRT *, (u_int)sizeof(SVCXPRT), KM_RPC);
	kmem_alloc(rpc_buffer(xprt), caddr_t, (u_int)UDPMSGSIZE, KM_RPC);
	kmem_alloc(ud, struct udp_data *, (u_int)sizeof(struct udp_data), KM_RPC);
	/* Initialize smp lock for outgoing type 2 mbuf data	*/
	lockinit(&ud->u_lk_udpdata, &lock_udpdata_d);
	xprt->xp_addrlen = 0;
	xprt->xp_p2 = (caddr_t)ud;
	xprt->xp_verf.oa_base = ud->ud_verfbody;
	xprt->xp_ops = &svckudp_op;
	xprt->xp_port = port;
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}
 
/*
 * Destroy a transport record.
 * Frees the space allocated for a transport record.
 */
void
svckudp_destroy(xprt)
	register SVCXPRT   *xprt;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

#ifdef RPCDEBUG
	rpc_debug(4, "usr_destroy %x\n", xprt);
#endif
	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	kmem_free((caddr_t)ud, KM_RPC);
	kmem_free((caddr_t)rpc_buffer(xprt), KM_RPC);
	kmem_free((caddr_t)xprt, KM_RPC);
}

/*
 * Receive rpc requests.
 * Pulls a request in off the socket, checks if the packet is intact,
 * and deserializes the call packet.
 */
bool_t
svckudp_recv(xprt, msg)
	register SVCXPRT	 *xprt;
	struct rpc_msg		 *msg;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR	 *xdrs = &(ud->ud_xdrin);
	register struct mbuf	 *m;
	int			  s;

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_recv %x\n", xprt);
#endif
	smp_lock(&lk_nfsstat, LK_RETRY);
	rsstat.rscalls++;
	smp_unlock(&lk_nfsstat);
	s = splnet();
	/* smp lock socket during receive		*/
	/* smp_lock(&xprt->xp_sock->lk_socket, LK_RETRY); */
	SO_LOCK(xprt->xp_sock);
	m = ku_recvfrom(xprt->xp_sock, &(xprt->xp_raddr));
	smp_unlock(&xprt->xp_sock->lk_socket);
	(void) splx(s);
	if (m == NULL) {
		/*
		 * This looks silly, but this stat is useless
		 * on an SMP machine by design. Leave it here
		 * for non-SMP use only.
		 */
		if (!smp) {
			smp_lock(&lk_nfsstat, LK_RETRY);
			rsstat.rsnullrecv++;
			smp_unlock(&lk_nfsstat);
		}
		return (FALSE);
	}

	if (m->m_len < 4*sizeof(u_long)) {
		smp_lock(&lk_nfsstat, LK_RETRY);
		rsstat.rsbadlen++;
		smp_unlock(&lk_nfsstat);
		goto bad;
	}
	xdrmbuf_init(&ud->ud_xdrin, m, XDR_DECODE);
	if (! xdr_callmsg(xdrs, msg)) {
		smp_lock(&lk_nfsstat, LK_RETRY);
		rsstat.rsxdrcall++;
		smp_unlock(&lk_nfsstat);
		goto bad;
	}
	ud->ud_xid = msg->rm_xid;
	ud->ud_inmbuf = m;
#ifdef RPCDEBUG
	rpc_debug(5, "svckudp_recv done\n");
#endif
	return (TRUE);

bad:
	m_freem(m);
	ud->ud_inmbuf = NULL;
	smp_lock(&lk_nfsstat, LK_RETRY);
	rsstat.rsbadcalls++;
	smp_unlock(&lk_nfsstat);
	return (FALSE);
}

static
noop()
{
}

static
buffree(ud)
	register struct udp_data *ud;
{
	int	s;

	s = splimp();
	smp_lock(&ud->u_lk_udpdata, LK_RETRY);
	ud->ud_flags &= ~UD_BUSY;
	if (ud->ud_flags & UD_WANTED) {
		ud->ud_flags &= ~UD_WANTED;
		wakeup((caddr_t)ud);
	}
	smp_unlock(&ud->u_lk_udpdata);
	splx(s);
}


/*
 * Send rpc reply.
 * Serialize the reply packet into the output buffer then
 * call ku_sendto to make an mbuf out of it and send it.
 */
bool_t
/* ARGSUSED */
svckudp_send(xprt, msg)
	register SVCXPRT *xprt; 
	struct rpc_msg *msg; 
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR *xdrs = &(ud->ud_xdrout);
	register int slen;
	register int stat = FALSE;
	int s;
	struct mbuf *m, *mclgetx();

#ifdef RPCDEBUG
	rpc_debug(4, "svckudp_send %x\n", xprt);
#endif
	s = splimp();
	/* SMP lock mbuf data while setting up for sleep	*/
	smp_lock(&ud->u_lk_udpdata, LK_RETRY);
	while (ud->ud_flags & UD_BUSY) {
		ud->ud_flags |= UD_WANTED;
		sleep_unlock((caddr_t)ud, PZERO-2, &ud->u_lk_udpdata);
		smp_lock(&ud->u_lk_udpdata, LK_RETRY);
	}
	ud->ud_flags |= UD_BUSY;
	smp_unlock(&ud->u_lk_udpdata);
	(void) splx(s);
	m = mclgetx(buffree, (caddr_t)ud, rpc_buffer(xprt), UDPMSGSIZE, M_WAIT);
	if (m == NULL) {
		buffree(ud);
		return (stat);
	}

	xdrmbuf_init(&ud->ud_xdrout, m, XDR_ENCODE);
	msg->rm_xid = ud->ud_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
		if (m->m_next == 0) {		/* XXX */
			m->m_len = slen;
		}
		if (!ku_sendto_mbuf(xprt->xp_sock, m, &xprt->xp_raddr))
			stat = TRUE;
	} else {
		printf("svckudp_send: xdr_replymsg failed\n");
		m_freem(m);
	}
	/*
	 * This is completely disgusting.  If public is set it is
	 * a pointer to a structure whose first field is the address
	 * of the function to free that structure and any related
	 * stuff.  (see rrokfree in nfs_xdr.c).
	 */
	if (xdrs->x_public) {
		(**((int (**)())xdrs->x_public))(xdrs->x_public);
	}
#ifdef RPCDEBUG
	rpc_debug(5, "svckudp_send done\n");
#endif
	return (stat);
}

/*
 * Return transport status.
 */
/*ARGSUSED*/
enum xprt_stat
svckudp_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE); 
}

/*
 * Deserialize arguments.
 */
bool_t
svckudp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{

	return ((*xdr_args)(&(((struct udp_data *)(xprt->xp_p2))->ud_xdrin), args_ptr));
}

bool_t
svckudp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{
	register XDR *xdrs =
	    &(((struct udp_data *)(xprt->xp_p2))->ud_xdrin);
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	ud->ud_inmbuf = (struct mbuf *)0;
	if (args_ptr) {
		xdrs->x_op = XDR_FREE;
		return ((*xdr_args)(xdrs, args_ptr));
	} else {
		return (TRUE);
	}
}

/*
 * The dup cacheing routines below provide a cache of recent
 * transaction id's. Rpc service routines can use this to detect
 * retransmissions and either ignore them, re-send a non-failure
 * response, or re-process.
 *
 * svckudp_dupenter() is the only routine that performs memory allocation
 *	and entry initialization.
 * svckudp_dupbusy() is the only routine that marks an entry busy
 *	(in-progress).
 * svckudp_dupsave() updates an entry with completion information.
 * svckudp_dup() searches for a duplicate and returns completion
 *	information if found.
 * svckudp_dupdone() is the only routine that makes a busy entry un-busy.
 */

struct dupreq {
	u_long		dr_xid;		/* 0:  unique transaction ID */
	u_long		dr_addr;	/* 4:  client address */
	u_long		dr_proc;	/* 8: proc within prog, vers */
	u_long		dr_flags;	/* 12: DUP_BUSY, DUP_DONE, DUP_FAIL */
	struct timeval	dr_time;	/* 16: time associated with req */
	struct dupreq	*dr_next;	/* 24: linked list of all entries */
	struct dupreq	*dr_chain;	/* 28: hash chain */
};

/*
 * dupcache_max is the number of cached items.  It is set
 * based on "system size". It should be large enough to hold
 * transaction history long enough so that a given entry is still
 * around for a few retransmissions of that transaction.
 */
#define MINDUPREQS	1024
#define	MAXDUPREQS	8192
struct dupreq **drhashtbl; /* array of heads of hash lists */
int drhashsz;		   /* number of hash lists */
int drhashszminus1;	   /* hash modulus */

/*
 * cache support functions:
 *	duplicate request -> xid
 *	xid hash function (the remainder function samples all 32 bits of xid)
 *	xid 		  -> head of hash list
 *	duplicate request -> head of hash list
 */

#define	REQTOXID(req)	(((struct udp_data *)((req)->rq_xprt->xp_p2))->ud_xid)
#define	XIDHASH(xid)	((xid) % drhashszminus1)
#define XIDTOLIST(xid) 	((struct dupreq *)(drhashtbl[XIDHASH(xid)]))
#define	REQTOLIST(dr)	XIDTOLIST((dr)->dr_xid)

/* SMP lock for dupreq hash list and dupreq event counters */
struct	lock_t	lk_rpcdupreq;

int	ndupreqs;
int	busyreqs;
int	secondreqs;
int	dupreqs_done;
int	dupreqs_busy;
int	dupreqs_fail;
int	dupcache_max;       /* duplicate cache high water mark */

/* routine to compare dup cache entries */
#define NOTDUP(dr, xid, req) (dr->dr_xid != xid || \
			      dr->dr_proc != req->rq_proc || \
			      dr->dr_addr != \
			      req->rq_xprt->xp_raddr.sin_addr.s_addr)

/*
 * drmru points to the head of a circular linked list in lru order.
 * drmru->dr_next == least recently entered (i.e. oldest) entry.
 * entries are not moved on this list when they are modified.
 */
struct dupreq *dupreqcache, *drmru;

svckudp_dupsave(req, transtime, transmark)
	register struct svc_req *req;
	struct timeval transtime;
	int	transmark;
{
	register struct dupreq *dr;
	u_long xid;

	xid = REQTOXID(req);

	smp_lock(&lk_rpcdupreq, LK_RETRY);

	dr = XIDTOLIST(xid); 
	while (dr != NULL) { 
		if (NOTDUP(dr, xid, req)) {
			dr = dr->dr_chain;
			continue;
		}
		break;
	}
	if (dr == NULL)	/* if not there, put it there */
		svckudp_dupenter(req, transtime, transmark);
	else { /* simply update time and completion status */
		dr->dr_time = transtime;
		dr->dr_flags = DUP_BUSY | transmark;
	}

	smp_unlock(&lk_rpcdupreq);
}

svckudp_dupenter(req, transtime, transmark)
	register struct svc_req *req;
	struct timeval transtime;
	int	transmark;
{
	register struct dupreq *dr;

	/*
	 * NB:
	 *	This routine must only be called while holding
	 *	the lk_rpcdupreq lock.
	 */
	dr = drmru->dr_next;
	unhash(dr);
	drmru = dr;
	dr->dr_xid = REQTOXID(req);
	dr->dr_proc = req->rq_proc;
	dr->dr_addr = req->rq_xprt->xp_raddr.sin_addr.s_addr;
	dr->dr_time = transtime;
	dr->dr_flags = DUP_BUSY | transmark;
	dr->dr_chain = REQTOLIST(dr);
	REQTOLIST(dr) = dr;
}

svckudp_dup(req, ptime, pmark)
	register struct svc_req *req;
	struct timeval *ptime;
	int *pmark;
{
	register struct dupreq *dr;
	u_long xid;
	 
	xid = REQTOXID(req);

	smp_lock(&lk_rpcdupreq, LK_RETRY);
	dr = XIDTOLIST(xid); 
	while (dr != NULL) { 
		if (NOTDUP(dr, xid, req)) {
			dr = dr->dr_chain;
			continue;
		}
		if (!(dr->dr_flags & DUP_BUSY))
			mprintf("RPC dup cache: lost entry\n");
		if (dr->dr_flags & DUP_DONE) {
			dupreqs_done++;
			*pmark = DUP_DONE;
			*ptime = dr->dr_time;
			smp_unlock(&lk_rpcdupreq);
			return (1);
		}
		if (dr->dr_flags & DUP_BUSY)
			dupreqs_busy++;
		else
			dupreqs_fail++;

		smp_unlock(&lk_rpcdupreq);
		return (0);
	}

	smp_unlock(&lk_rpcdupreq);
	return (0);

}

int avoid_seconds = 1;

svckudp_dupbusy(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;
	register struct dupreq **dt;
	u_long xid;
	extern int prattached;

	xid = REQTOXID(req);

	smp_lock(&lk_rpcdupreq, LK_RETRY);

	/* First time through, allocate and init hash list and cache area */
	if (!dupreqcache) {
		int i;
		dupcache_max = 1024 * nfs_system_size();
		/* Prestoserve allows a server to cycle many more requests */
		if (prattached)		/* Prestoserve is present */
			dupcache_max *= 2;
		dupcache_max = MAX(MINDUPREQS, dupcache_max);
		dupcache_max = MIN(MAXDUPREQS, dupcache_max);
		drhashsz = dupcache_max / 16;
		drhashszminus1 = drhashsz - 1;
		smp_unlock(&lk_rpcdupreq);
		/*
		 * The following assumes that kmem_alloc will block
		 * until success and clear the space. We give up the
		 * lock in case we block.
		 */
		kmem_alloc(dr, struct dupreq *,
			   sizeof(*dr) * dupcache_max, KM_RPC);
		kmem_alloc(dt, struct dupreq **,
			   sizeof(struct dupreq *) * drhashsz, KM_RPC);
		smp_lock(&lk_rpcdupreq, LK_RETRY);
		if (!dupreqcache) { /* we got it first */
			for (i = 0; i < dupcache_max; i++)
				dr[i].dr_next = &(dr[i + 1]);
			dr[dupcache_max - 1].dr_next = dr;
			ndupreqs = dupcache_max;
			dupreqcache = dr;
			drmru = dr;
			drhashtbl = dt;
		}
		else  { /* someone beat us to it */
			kmem_free((caddr_t)dr, KM_RPC);
			kmem_free((caddr_t)dt, KM_RPC);
		}
	}

	dr = XIDTOLIST(xid); 
	while (dr != NULL) { 
		if (NOTDUP(dr, xid, req)) {
			dr = dr->dr_chain;
			continue;
		}
		if (dr->dr_flags & DUP_BUSY) {
			busyreqs++;
			smp_unlock(&lk_rpcdupreq);
			return (1);
		}
		if (avoid_seconds && (dr->dr_flags & DUP_DONE) &&
		    timepick->tv_sec - dr->dr_time.tv_sec < avoid_seconds) {
			secondreqs++;
			smp_unlock(&lk_rpcdupreq);
			return (1);
		}
		dr->dr_flags |= DUP_BUSY;
		smp_unlock(&lk_rpcdupreq);
		return (0);
	}

	svckudp_dupenter(req, *timepick, DUP_BUSY);
	smp_unlock(&lk_rpcdupreq);
	return (0);
}

svckudp_dupdone(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;
	u_long xid;
	 
	xid = REQTOXID(req);

	smp_lock(&lk_rpcdupreq, LK_RETRY);
	dr = XIDTOLIST(xid); 
	while (dr != NULL) { 
		if (NOTDUP(dr, xid, req)) {
			dr = dr->dr_chain;
			continue;
 		}
		dr->dr_flags &= ~DUP_BUSY;
		smp_unlock(&lk_rpcdupreq);
 		return (0);
	}

	smp_unlock(&lk_rpcdupreq);
	return (0);

}

static
unhash(dr)
	struct dupreq *dr;
{
	struct dupreq *drt;
	struct dupreq *drtprev = NULL;
	 
	/*
	 * NB:
	 *	This routine must only be called while holding
	 *	the lk_rpcdupreq lock.
	 */

	drt = REQTOLIST(dr); 
	while (drt != NULL) { 
		if (drt == dr) { 
			if (drtprev == NULL) {
				REQTOLIST(dr) = drt->dr_chain;
			} else {
				drtprev->dr_chain = drt->dr_chain;
			}
			return; 
		}	
		drtprev = drt;
		drt = drt->dr_chain;
	}	
}

