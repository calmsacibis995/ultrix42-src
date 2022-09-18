#ifndef lint
static	char	*sccsid = "@(#)clnt_kudp.c	4.3	(ULTRIX)	2/28/91";
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
 * clnt_kudp.c
 * Implements a kernel UDP/IP based, client side RPC.
 */

/*
 *
 *   Modification history:
 *
 * 29 Jan 91 -- chet
 *	Make kernel RPC calls interruptible.
 *
 * 10 Dec 89 -- chet
 *	Add arphasmbuf() call in clntkudp_callit() after timeouts
 *	to free an mbuf on the arpresolve trash heap (avoids long sleeps).
 *
 *  30 May 88 -- U. Sinkewicz
 *	Added SO_LOCK to replace smp_lock(&so->lk_socket, LK_RETRY)
 *	as part of an smp bug fix.  Fix guarantees that socket doesn't 
 *	change while unlocked during sleeps or for the lock heirarchy.
 *
 *  1 Sep 88 -- chet
 *	Remove transaction ID - it's now in the client handle
 *	passed down from the application
 *
 * 06-Jun-88	condylis
 *	Added SMP locking for clntkudpxid and mbuf data contained in
 *	outbuf member of cku_private structure.  New routine to 
 *	initialize rpc smp locks (rpcinit).  Added interaction with
 *	socket locks.  Added SMP locking of RPC client statistics.
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added checks for null pointers
 *	in clntkudp_create
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../h/smp_lock.h"

struct mbuf	*ku_recvfrom();
int		ckuwakeup();

enum clnt_stat	clntkudp_callit();
void		clntkudp_abort();
void		clntkudp_error();
bool_t		clntkudp_freeres();
bool_t		clntkudp_control();
void		clntkudp_destroy();

void		xdrmbuf_init();

/*
 * Operations vector for UDP/IP based RPC
 */
/* static */ struct clnt_ops udp_ops = {
	clntkudp_callit,	/* do rpc call */
	clntkudp_abort,		/* abort call */
	clntkudp_error,		/* return error status */
	clntkudp_freeres,	/* free results */
	clntkudp_destroy,	/* destroy rpc handle */
	clntkudp_control	/* the ioctl() of rpc */
};

/*
 * Private data per rpc handle.  This structure is allocated by
 * clntkudp_create, and freed by cku_destroy.
 */
struct cku_private {
	u_int			 cku_flags;	/* see below */
	CLIENT			 cku_client;	/* client handle */
	int			 cku_retrys;	/* request retrys */
	struct socket		*cku_sock;	/* open udp socket */
	struct sockaddr_in	 cku_addr;	/* remote address */
	struct rpc_err		 cku_err;	/* error status */
	XDR			 cku_outxdr;	/* xdr routine for output */
	XDR			 cku_inxdr;	/* xdr routine for input */
	u_int			 cku_outpos;	/* position of in output mbuf */
	char			*cku_outbuf;	/* output buffer */
	char			*cku_inbuf;	/* input buffer */
	struct mbuf		*cku_inmbuf;	/* input mbuf */
	struct ucred		*cku_cred;	/* credentials */
/* SMP lock for protecting mbuf	data of cku_private	
 * This lock is used with CKU_BUFBUSY and CKU_BUFWANTED flags
 */
	struct	lock_t	cku_lk_outbuf;
};

/* SMP lock for xids contained in client handles	*/
struct	lock_t	lk_rpcxid;


struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
	int	rcbadverfs;
} rcstat;


#define	ptoh(p)		(&((p)->cku_client))
#define	htop(h)		((struct cku_private *)((h)->cl_private))

/* cku_flags */
#define	CKU_TIMEDOUT	0x001
#define	CKU_BUSY	0x002
#define	CKU_WANTED	0x004
#define	CKU_BUFBUSY	0x008
#define	CKU_BUFWANTED	0x010
#define CKU_ONCE_ONLY	0x020

/* Times to retry */
#define	RECVTRIES	2
#define	SNDTRIES	4

/* Need the externs for rpcinit routine 	*/
extern	struct lock_t	lk_rpccallout;
extern	struct lock_t	lk_rpcdupreq;
extern	struct lock_t	lk_rpcrqcred;
extern	struct lock_t	lk_nfsstat;

clntkudp_once(cl, flag)
	CLIENT  *cl;
{
	struct cku_private *p = (struct cku_private *)cl->cl_private;

	if (flag != 0) {
		p->cku_flags |= CKU_ONCE_ONLY;
	} else {
		p->cku_flags &= ~CKU_ONCE_ONLY;
	}
}

static
noop()
{
}

static
buffree(p)
	struct cku_private *p;
{
	int	s;

	s = splimp();
	smp_lock(&p->cku_lk_outbuf, LK_RETRY);
	p->cku_flags &= ~CKU_BUFBUSY;
	if (p->cku_flags & CKU_BUFWANTED) {
		p->cku_flags &= ~CKU_BUFWANTED;
		wakeup((caddr_t)&p->cku_outbuf);
	}
	smp_unlock(&p->cku_lk_outbuf);
	splx(s);
}

/*
 * Create an rpc handle for a udp rpc connection.
 * Allocates space for the handle structure and the private data, and
 * opens a socket.  Note sockets and handles are one to one.
 */
CLIENT *
clntkudp_create(addr, pgm, vers, retrys, cred)
	struct sockaddr_in *addr;
	u_long pgm;
	u_long vers;
	int retrys;
	struct ucred *cred;
{
	register CLIENT *h;
	register struct cku_private *p;
	int error = 0;
	struct rpc_msg call_msg;
	struct mbuf *m, *mclgetx();
	extern int nfs_portmon;

#ifdef RPCDEBUG
	rpc_debug(4, "clntkudp_create(%X, %d, %d, %d\n",
	    addr->sin_addr.s_addr, pgm, vers, retrys);
#endif
	kmem_alloc(p, struct cku_private *, (u_int)sizeof *p, KM_RPC);
	if(p == NULL)
		panic("clntkudp_create: kmem_alloc returns 0");
	h = ptoh(p);

	/* handle */
	h->cl_ops = &udp_ops;
	h->cl_private = (caddr_t) p;
	h->cl_auth = authkern_create();

	/* call message, just used to pre-serialize below */
	call_msg.rm_xid = 0;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = pgm;
	call_msg.rm_call.cb_vers = vers;

	/* private */
	clntkudp_init(h, addr, retrys, cred);
	kmem_alloc(p->cku_outbuf, char *, (u_int)UDPMSGSIZE, KM_RPC);
	if(p->cku_outbuf == NULL)
		panic("clntkudp_create: kmem_alloc returns 0 for p->cku_outbuf");

	/* Initialize SMP lock for mbuf data buffer		*/
	lockinit(&p->cku_lk_outbuf, &lock_cku_d);

	m = mclgetx(noop, 0, p->cku_outbuf, UDPMSGSIZE, M_DONTWAIT);
	if (m == NULL)
		goto bad;
	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);

	/* pre-serialize call message header */
	if (! xdr_callhdr(&(p->cku_outxdr), &call_msg)) {
		printf("clntkudp_create - Fatal header serialization error.");
		(void) m_freem(m);
		goto bad;
	}
	p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	(void) m_free(m);

	/*
	 * Open udp socket.
	 *
	 * We must clear u.u_error after calling socreate() because
	 * socreate calls suser() which may set u.u_error as a side
	 * effect.  This is really a bug in socreate() which should
	 * be fixed someday, but for now we just patch things up here.
	 */
	error = socreate(AF_INET, &p->cku_sock, SOCK_DGRAM, IPPROTO_UDP);
	u.u_error = 0;		/* XXX */
	if (error) {
		printf("clntkudp_create: socket creation problem, %d", error);
		goto bad;
	}
	if (error = bindresvport(p->cku_sock)) {
		printf("clntkudp_create: socket bind problem, %d", error);
		goto bad;
	}
	return (h);

bad:
	kmem_free((caddr_t)p->cku_outbuf, KM_RPC);
	kmem_free((caddr_t)(caddr_t)p, KM_RPC);
#ifdef RPCDEBUG
	rpc_debug(4, "create failed\n");
#endif
	return ((CLIENT *)NULL);
}

clntkudp_init(h, addr, retrys, cred)
	CLIENT *h;
	struct sockaddr_in *addr;
	int retrys;
	struct ucred *cred;
{
	struct cku_private *p = htop(h);

	p->cku_retrys = retrys;
	p->cku_addr = *addr;
	p->cku_cred = cred;
	p->cku_flags &= (CKU_BUFBUSY | CKU_BUFWANTED);
}

/*
 * Time out back off function. tim is in hz
 */
#define MAXTIMO	(60 * hz)
#define backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

/*
 * Call remote procedure.
 * Most of the work of rpc is done here.  We serialize what is left
 * of the header (some was pre-serialized in the handle), serialize
 * the arguments, and send it off.  We wait for a reply or a time out.
 * Timeout causes an immediate return, other packet problems may cause
 * a retry on the receive.  When a good packet is received we deserialize
 * it, and check verification.  A bad reply code will cause one retry
 * with full (longhand) credentials.
 */

enum clnt_stat 
clntkudp_callit(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait)
	register CLIENT	*h;
	u_long		procnum;
	xdrproc_t	xdr_args;
	caddr_t		argsp;
	xdrproc_t	xdr_results;
	caddr_t		resultsp;
	struct timeval	wait;
{
	register struct cku_private *p = htop(h);
	register XDR	   	   *xdrs;
	register struct socket	   *so = p->cku_sock;
	int			   rtries;
	int			   stries = p->cku_retrys;
	struct sockaddr_in	   from;
	int			   s;
	struct ucred		   *tmpcred;
	struct mbuf		   *m = NULL;
	int timohz;
	u_long xid;
	u_int rempos = 0;
	int refreshes = 2;	/* number of times to refresh credential */
 	int interrupted;        /* return from sleep() */
 	int smask;              /* saved signal mask */
 	struct proc *pp = u.u_procp;

#ifdef RPCDEBUG
	rpc_debug(4, "cku_callit\n");
#endif
	smp_lock(&lk_nfsstat, LK_RETRY);
	rcstat.rccalls++;
	smp_unlock(&lk_nfsstat);

	while (p->cku_flags & CKU_BUSY) {
		smp_lock(&lk_nfsstat, LK_RETRY);
		rcstat.rcwaits++;
		smp_unlock(&lk_nfsstat);
		p->cku_flags |= CKU_WANTED;
		(void) sleep((caddr_t)h, PZERO-2);
	}
	p->cku_flags |= CKU_BUSY;

	if ((p->cku_flags & CKU_ONCE_ONLY) != 0)
		stries = 1;

	/*
	 * Set credentials into the u structure
	 */
	tmpcred = u.u_cred;
	u.u_cred = p->cku_cred;

	/*
	 * This is dumb but easy: keep the time out in units of hz
	 * so it is easy to call timeout and modify the value.
	 */
	timohz = wait.tv_sec * hz + (wait.tv_usec * hz) / 1000000;

call_again:

	/*
	 * Wait til buffer gets freed then make a type 2 mbuf point at it
	 * The buffree routine clears CKU_BUFBUSY and does a wakeup when
	 * the mbuf gets freed.
	 */

	/*
	 * SMP lock this sleep wakeup mechanism which is described
	 * above.
	 */
	s = splimp();
	smp_lock(&p->cku_lk_outbuf, LK_RETRY);

	while (p->cku_flags & CKU_BUFBUSY) {
		p->cku_flags |= CKU_BUFWANTED;
		/*
		 * This is a kludge to avoid deadlock in the case of a
		 * loop-back call.  The client can block wainting for
		 * the server to free the mbuf while the server is blocked
		 * waiting for the client to free the reply mbuf.  Avoid
		 * this by flushing the input queue every once in a while
		 * while we are waiting.
		 */
		timeout(wakeup, (caddr_t)&p->cku_outbuf, hz);
		sleep_unlock((caddr_t)&p->cku_outbuf, PZERO-3, &p->cku_lk_outbuf);
		untimeout(wakeup, (caddr_t)&p->cku_outbuf);
		/* SMP lock socket during flush			*/
		SO_LOCK(so);
		/* smp_lock(&so->lk_socket, LK_RETRY); */
		sbflush(&so->so_rcv);
		smp_unlock(&so->lk_socket);
		smp_lock(&p->cku_lk_outbuf, LK_RETRY);
	}
	p->cku_flags |= CKU_BUFBUSY;
	smp_unlock(&p->cku_lk_outbuf);
	(void) splx(s);
	m = mclgetx(buffree, (caddr_t)p, p->cku_outbuf, UDPMSGSIZE, M_WAIT);
	if (m == NULL) {
		p->cku_err.re_status = RPC_SYSTEMERROR;
		p->cku_err.re_errno = ENOBUFS;
		buffree(p);
		goto done;
	}

	xdrs = &p->cku_outxdr;
	/*
	 * The transaction id is the first thing in the
	 * preserialized output buffer.
	 */
	(*(u_long *)(p->cku_outbuf)) = h->cl_xid;

	xdrmbuf_init(xdrs, m, XDR_ENCODE);

	if (rempos != 0) {
		XDR_SETPOS(xdrs, rempos);
	} else {
		/*
		 * Serialize dynamic stuff into the output buffer.
		 */
		XDR_SETPOS(xdrs, p->cku_outpos);
		if ((! XDR_PUTLONG(xdrs, (long *)&procnum)) ||
		    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
		    (! (*xdr_args)(xdrs, argsp))) {
			p->cku_err.re_status = RPC_CANTENCODEARGS;
			p->cku_err.re_errno = EIO;
			(void) m_freem(m);
			goto done;
		}
		rempos = XDR_GETPOS(xdrs);
	}
	m->m_len = rempos;

	if ((p->cku_err.re_errno = ku_sendto_mbuf(so, m, &p->cku_addr)) != 0) {
		p->cku_err.re_status = RPC_CANTSEND;
		p->cku_err.re_errno = EIO;
		goto done;
	}

	for (rtries = RECVTRIES; rtries; rtries--) {
		s = splnet();
		/* SMP lock socket while doing setting up to sleep */
		/* smp_lock(&so->lk_socket, LK_RETRY); */
		SO_LOCK(so);
		while (so->so_rcv.sb_cc == 0) {
			/*
			 * Set timeout then wait for input, timeout,
 			 * or interrupt.
			 */
#ifdef RPCDEBUG
			rpc_debug(3, "callit: waiting %d, sec %d usec %d\n",
				timohz, time.tv_sec, time.tv_usec);
#endif
			timeout(ckuwakeup, (caddr_t)p, timohz);
			so->so_rcv.sb_flags |= SB_WAIT;

			if (!(pp->p_vm & SNFSPGN)) {
				/*
				 * If not doing page-in,
				 * allow interruptions for hangup, interrupt,
				 * quit, kill, and termination.
				 */
				smask = pp->p_sigmask;
				pp->p_sigmask |=
					~(sigmask(SIGHUP) |
					  sigmask(SIGINT) | sigmask(SIGQUIT) |
					  sigmask(SIGKILL) | sigmask(SIGTERM));
				interrupted =
				       sleep_unlock((caddr_t)&so->so_rcv.sb_cc,
						    PZERO+1 | PCATCH,
						    &so->lk_socket);
				pp->p_sigmask = smask;
				untimeout(ckuwakeup, (caddr_t)p);
				if (interrupted) {
					(void) splx(s);
					p->cku_err.re_status = RPC_INTR;
					p->cku_err.re_errno = EINTR;
					goto done;
				}
			} else {
				sleep_unlock((caddr_t)&so->so_rcv.sb_cc,
						    PRIBIO,
						    &so->lk_socket);
				untimeout(ckuwakeup, (caddr_t)p);
			}

			if (p->cku_flags & CKU_TIMEDOUT) {
				p->cku_flags &= ~CKU_TIMEDOUT;
				/*
				 * Avoid long sleep on m above (call_again)
				 * if the server failed to arp resolve and m
				 * is being held ransom (actually for three
				 * minutes). 
				 * The arphasmbuf() call will force it free,
				 * and buffree() will be called as a result.
				 */
				if (p->cku_flags & CKU_BUFBUSY)
					arphasmbuf(m);
				(void) splx(s);
				p->cku_err.re_status = RPC_TIMEDOUT;
				p->cku_err.re_errno = ETIMEDOUT;
				smp_lock(&lk_nfsstat, LK_RETRY);
				rcstat.rctimeouts++;
				smp_unlock(&lk_nfsstat);
				goto done;
			}
		SO_LOCK(so);
		/* smp_lock(&so->lk_socket, LK_RETRY); */
		}

		if (so->so_error) {
			so->so_error = 0;
			smp_unlock(&so->lk_socket);
			(void) splx(s);
			continue;
		}

		p->cku_inmbuf = ku_recvfrom(so, &from);
		smp_unlock(&so->lk_socket);
		(void) splx(s);
		if (p->cku_inmbuf == NULL) {
			continue;
		}
		p->cku_inbuf = mtod(p->cku_inmbuf, char *);

		if (p->cku_inmbuf->m_len < sizeof(u_long)) {
			m_freem(p->cku_inmbuf);
			continue;
		}
		/*
		 * If reply transaction id matches id sent
		 * we have a good packet.
		 */
		if (*((u_long *)(p->cku_inbuf))
		    != *((u_long *)(p->cku_outbuf))) {
			smp_lock(&lk_nfsstat, LK_RETRY);
			rcstat.rcbadxids++;
			smp_unlock(&lk_nfsstat);
			m_freem(p->cku_inmbuf);
			continue;
		}
		/*
		 * Flush the rest of the stuff on the input queue
		 * for the socket.
		 */
		s = splnet();
		SO_LOCK(so);
		/* smp_lock(&so->lk_socket, LK_RETRY); */
		sbflush(&so->so_rcv);
		smp_unlock(&so->lk_socket);
		(void) splx(s);
		break;
	} 

	if (rtries == 0) {
		p->cku_err.re_status = RPC_CANTRECV;
		p->cku_err.re_errno = EIO;
		goto done;
	}

	/*
	 * Process reply
	 */

	xdrs = &(p->cku_inxdr);
	xdrmbuf_init(xdrs, p->cku_inmbuf, XDR_DECODE);

	{
		/*
		 * Declare this variable here to have smaller
		 * demand for stack space in this procedure.
		 */
		struct rpc_msg		   reply_msg;

		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = resultsp;
		reply_msg.acpted_rply.ar_results.proc = xdr_results;

		/*
		 * Decode and validate the response.
		 */
		if (xdr_replymsg(xdrs, &reply_msg)) {
			_seterr_reply(&reply_msg, &(p->cku_err));

			if (p->cku_err.re_status == RPC_SUCCESS) {
				/*
				 * Reply is good, check auth.
				 */
				if (! AUTH_VALIDATE(h->cl_auth,
				    &reply_msg.acpted_rply.ar_verf)) {
					p->cku_err.re_status = RPC_AUTHERROR;
					p->cku_err.re_why = AUTH_INVALIDRESP;
					rcstat.rcbadverfs++;
				}
				if (reply_msg.acpted_rply.ar_verf.oa_base !=
				    NULL) {
					/* free auth handle */
					xdrs->x_op = XDR_FREE;
					(void) xdr_opaque_auth(xdrs,
					    &(reply_msg.acpted_rply.ar_verf));
				}
			} else {
				/*
				 * Maybe our credential needs refreshed
				 */
				if (refreshes > 0 && AUTH_REFRESH(h->cl_auth)) {
					refreshes--;
					rcstat.rcnewcreds++;
					rempos = 0;
				}
			}
		} else {
			p->cku_err.re_status = RPC_CANTDECODERES;
		}
	}

	m_freem(p->cku_inmbuf);

#ifdef RPCDEBUG
	rpc_debug(4, "cku_callit done\n");
#endif
done:
	if ((p->cku_err.re_status != RPC_SUCCESS) &&
 	    (p->cku_err.re_status != RPC_INTR) &&
	    (p->cku_err.re_status != RPC_CANTENCODEARGS) &&
	    (--stries > 0) ) {
		smp_lock(&lk_nfsstat, LK_RETRY);
		rcstat.rcretrans++;
		smp_unlock(&lk_nfsstat);
		timohz = backoff(timohz);
		if (p->cku_err.re_status == RPC_SYSTEMERROR ||
		    p->cku_err.re_status == RPC_CANTSEND) {
			/*
			 * Errors due to lack o resources, wait a bit
			 * and try again.
			 */
			(void) sleep((caddr_t)&lbolt, PZERO-4);
		}
		goto call_again;
	}
	u.u_cred = tmpcred;
	p->cku_flags &= ~CKU_BUSY;
	if (p->cku_flags & CKU_WANTED) {
		p->cku_flags &= ~CKU_WANTED;
		wakeup((caddr_t)h);
	}
	if (p->cku_err.re_status != RPC_SUCCESS) {
		smp_lock(&lk_nfsstat, LK_RETRY);
		rcstat.rcbadcalls++;
		smp_unlock(&lk_nfsstat);
	}
	return (p->cku_err.re_status);
}

/*
 * Wake up client waiting for a reply.
 */
ckuwakeup(p)
	register struct cku_private *p;
{

#ifdef RPCDEBUG
	rpc_debug(4, "cku_timeout\n");
#endif
	p->cku_flags |= CKU_TIMEDOUT;
	sbwakeup(&p->cku_sock->so_rcv);
}

/*
 * Return error info on this handle.
 */
void
clntkudp_error(h, err)
	CLIENT *h;
	struct rpc_err *err;
{
	register struct cku_private *p = htop(h);

	*err = p->cku_err;
}

/* static */ bool_t
clntkudp_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	caddr_t res_ptr;
{
	register struct cku_private *p = (struct cku_private *)cl->cl_private;
	register XDR *xdrs = &(p->cku_outxdr);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

void 
clntkudp_abort()
{
}

bool_t
clntkudp_control()
{
	return (FALSE);
}


/*
 * Destroy rpc handle.
 * Frees the space used for output buffer, private data, and handle
 * structure, and closes the socket for this handle.
 */
void
clntkudp_destroy(h)
	CLIENT *h;
{
	register struct cku_private *p = htop(h);

#ifdef RPCDEBUG
	rpc_debug(4, "cku_destroy %x\n", h);
#endif
	(void) soclose(p->cku_sock);
	kmem_free((caddr_t)(caddr_t)p->cku_outbuf, KM_RPC);
	kmem_free((caddr_t)(caddr_t)p, KM_RPC);
}

/*
 * try to bind to a reserved port
 */
bindresvport(so)
	struct socket *so;
{
	struct sockaddr_in *sin;
	struct mbuf *m;
	u_short i;
	int error;
	struct ucred *tmpcred;
	struct ucred *savecred;

#	define MAX_PRIV	(IPPORT_RESERVED-1)
#	define MIN_PRIV	(IPPORT_RESERVED/2)

	m = m_get(M_WAIT, MT_SONAME);
	if (m == NULL) {
		printf("bindresvport: couldn't alloc mbuf");
		return(ENOBUFS);
	}

	sin = mtod(m, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	m->m_len = sizeof(struct sockaddr_in);

	/*
	 * use our own cred for sobind call.
	 */
	tmpcred = crdup(u.u_cred);
	savecred = u.u_cred;
	u.u_cred = tmpcred;
	u.u_uid = 0;
	
	error = EADDRINUSE;
	for (i = MAX_PRIV; error == EADDRINUSE && i >= MIN_PRIV; i--) {
		sin->sin_port = htons(i);
		error = sobind(so, m);
	}
	(void) m_freem(m);
	u.u_cred = savecred;
	crfree(tmpcred);
	return (error);
}


/* Called from main in sys/init_main.c to setup rpc smp locks	*/
rpcinit()
	{

	lockinit(&lk_rpcxid, &lock_rpcxid_d);
	lockinit(&lk_rpcrqcred, &lock_rpcrqcred_d);
	lockinit(&lk_rpccallout, &lock_rpccallout_d);
	lockinit(&lk_rpcdupreq, &lock_rpcdupreq_d);

	}
