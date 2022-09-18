#ifndef lint
static	char	*sccsid = "@(#)raw_cb.c	4.1		(ULTRIX)	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1984,1988 by			*
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
 *	22-Mar-90	Ursula Sinkewicz
 *		Fix to locking.  Needed to separate the rawintrq from
 *		the rawcb queue.  Added rawcb_lk.  Goes with changes
 *		in raw_usrreq.c.
 *	28-Feb-89	Ursula Sinkewicz
 *    		SMP/mips merge.  (Added R. Bhanukitsiri changes 2/5/89).
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes. Use new memory allocation
 *	scheme for mbufs.
 *
 */ 
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	raw_cb.c	6.6 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/errno.h"
#include "../h/smp_lock.h"

#include "../net/net/if.h"
#include "../net/net/route.h"
#include "../net/net/raw_cb.h"
#include "../net/netinet/in.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../h/kmalloc.h"

/*
 * Routines to manage the raw protocol control blocks. 
 *
 * TODO:
 *	hash lookups by protocol family/protocol + address family
 *	take care of unique address problems per AF?
 *	redo address binding to allow wildcards
 */

/*
 * Allocate a control block and a nominal amount
 * of buffer space for the socket.
 */

extern struct lock_data lock_rawcb_d;	
extern struct lock_t rawcb_lk;

raw_attach(so, proto)
	register struct socket *so;
	int proto;
{
#ifdef old
	struct mbuf *m;
#endif
	register struct rawcb *rp;
	int s;

	KM_ALLOC(rp, struct rawcb *, sizeof(struct rawcb), KM_PCB, KM_CLEAR|KM_NOWAIT);
	if(rp == NULL)
		return(ENOBUFS);
	if (sbreserve(&so->so_snd, RAWSNDQ) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, RAWRCVQ) == 0)
		goto bad2;
	rp->rcb_socket = so;
	so->so_pcb = (caddr_t)rp;
	rp->rcb_pcb = 0;
	rp->rcb_proto.sp_family = so->so_proto->pr_domain->dom_family;
	rp->rcb_proto.sp_protocol = proto;
	so->ref = 30;
	smp_unlock(&so->lk_socket);
		smp_lock(&rawcb_lk, LK_RETRY);
		insque(rp, &rawcb);
		smp_unlock(&rawcb_lk);
	smp_lock(&so->lk_socket, LK_RETRY);
	so->ref = 0;
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	KM_FREE(rp, KM_PCB);
	return (ENOBUFS);
}

/*
 * Detach the raw connection block and discard
 * socket resources.
 */
raw_detach(rp)
	register struct rawcb *rp;
{
	struct socket *so = rp->rcb_socket;

	RTLOCK();
	if (rp->rcb_route.ro_rt)
		rtfree(rp->rcb_route.ro_rt);
	RTUNLOCK();
	so->so_pcb = 0;
	so->ref = 31;
	smp_unlock(&so->lk_socket);
		smp_lock(&rawcb_lk, LK_RETRY);
		remque(rp);
		smp_unlock(&rawcb_lk);
	smp_lock(&so->lk_socket, LK_RETRY);
	so->ref = 0;
	if(rp->rcb_options)
		m_freem(dtom(rp->rcb_options));
	KM_FREE(rp, KM_PCB);
	sofree(so);

}

/*
 * Disconnect and possibly release resources.
 */
raw_disconnect(rp)
	struct rawcb *rp;
{

	rp->rcb_flags &= ~RAW_FADDR;
	if (rp->rcb_socket->so_state & SS_NOFDREF)
		raw_detach(rp);
}

raw_bind(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
	register struct rawcb *rp;

	if (ifnet == 0)
		return (EADDRNOTAVAIL);
/* BEGIN DUBIOUS */
	/*
	 * Should we verify address not already in use?
	 * Some say yes, others no.
	 */
	switch (addr->sa_family) {

#ifdef INET
	case AF_IMPLINK:
	case AF_INET: {
		if (((struct sockaddr_in *)addr)->sin_addr.s_addr &&
		    ifa_ifwithaddr(addr) == 0)
			return (EADDRNOTAVAIL);
		break;
	}
#endif

	default:
		return (EAFNOSUPPORT);
	}
/* END DUBIOUS */
	rp = sotorawcb(so);
	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_laddr, sizeof (*addr));
	rp->rcb_flags |= RAW_LADDR;
	return (0);
}

/*
 * Associate a peer's address with a
 * raw connection block.
 */
raw_connaddr(rp, nam)
	struct rawcb *rp;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);

	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_faddr, sizeof(*addr));
	rp->rcb_flags |= RAW_FADDR;
}
