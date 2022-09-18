#ifndef lint
static char *sccsid = "@(#)raw_usrreq.c	4.1	ULTRIX		7/2/90";
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
 *		Fixed the locking.  Needed to separate the rawintrq from
 *		the rawcb queue.  Added rawcb_lk.  Goes with changes in
 *		in raw_cb.c
 *	30-May-89	Ursula Sinkewicz
 *		Added SO_LOCK to plug smp hole caused by unlocking a
 *	socket strictly to accommodate the hierarchy or uiomove().
 *		
 *	28-Feb-89	Ursula Sinkewicz
 *		SMP/mips merge.  Added changes by R. Bhanukitsiri 2/5/89
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
 *	raw_usrreq.c	6.8 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/net/if.h"
#include "../net/net/route.h"
#include "../net/net/netisr.h"
#include "../net/net/raw_cb.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax

extern struct lock_data lock_rawcb_d;
struct lock_t rawcb_lk;

/*
 * Initialize raw connection block q.
 */
raw_init()
{

	int s;

	/* SMP 1.31.89.us.  Use the imbeded lock in rawintrq to control
	 * rawintrq and rawcb.  */
	lockinit(&rawintrq.lk_ifqueue, &lock_ipstat_d);
	lockinit(&rawcb_lk, &lock_rawcb_d);
	s = splimp();
	smp_lock(&rawintrq.lk_ifqueue, LK_RETRY);
	smp_lock(&rawcb_lk, LK_RETRY);
	rawcb.rcb_next = rawcb.rcb_prev = &rawcb;
	rawintrq.ifq_maxlen = IFQ_MAXLEN;
	smp_unlock(&rawcb_lk);
	smp_unlock(&rawintrq.lk_ifqueue);
	splx(s);
}

/*
 * Raw protocol interface.
 */
raw_input(m0, proto, src, dst)
	struct mbuf *m0;
	struct sockproto *proto;
	struct sockaddr *src, *dst;
{
	register struct mbuf *m;
	struct raw_header *rh;
	int s;

	/*
	 * Rip off an mbuf for a generic header.
	 */
	m = m_get(M_DONTWAIT, MT_DATA);
	if (m == 0) {
		m_freem(m0);
		return;
	}
	m->m_next = m0;
	m->m_len = sizeof(struct raw_header);
	rh = mtod(m, struct raw_header *);
	rh->raw_dst = *dst;
	rh->raw_src = *src;
	rh->raw_proto = *proto;

	/*
	 * Header now contains enough info to decide
	 * which socket to place packet in (if any).
	 * Queue it up for the raw protocol process
	 * running at software interrupt level.
	 */
	s = splimp();
	smp_lock(&rawintrq.lk_ifqueue, LK_RETRY);
	if (IF_QFULL(&rawintrq))
		m_freem(m);
	else
		IF_ENQUEUE(&rawintrq, m);
	smp_unlock(&rawintrq.lk_ifqueue);
	splx(s);
	schednetisr(NETISR_RAW);
}

/*
 * Raw protocol input routine.  Process packets entered
 * into the queue at interrupt time.  Find the socket
 * associated with the packet(s) and move them over.  If
 * nothing exists for this packet, drop it.
 */
rawintr()
{
	int s;
	struct mbuf *m;
	register struct rawcb *rp;
	register struct raw_header *rh;
	struct socket *last;

next:
	s = splimp();
	smp_lock(&rawintrq.lk_ifqueue, LK_RETRY);
	IF_DEQUEUE(&rawintrq, m);
	smp_unlock(&rawintrq.lk_ifqueue);
	splx(s);
	if (m == 0)
		return;
	rh = mtod(m, struct raw_header *);
	last = 0;
	smp_lock(&rawcb_lk, LK_RETRY);
	for (rp = rawcb.rcb_next; rp != &rawcb; rp = rp->rcb_next) {
		if (rp->rcb_proto.sp_family != rh->raw_proto.sp_family)
			continue;
		if (rp->rcb_proto.sp_protocol  &&
		    rp->rcb_proto.sp_protocol != rh->raw_proto.sp_protocol)
			continue;
		/*
		 * We assume the lower level routines have
		 * placed the address in a canonical format
		 * suitable for a structure comparison.
		 */
#define equal(a1, a2) \
	(bcmp((caddr_t)&(a1), (caddr_t)&(a2), sizeof (struct sockaddr)) == 0)
		if ((rp->rcb_flags & RAW_LADDR) &&
		    !equal(rp->rcb_laddr, rh->raw_dst))
			continue;
		if ((rp->rcb_flags & RAW_FADDR) &&
		    !equal(rp->rcb_faddr, rh->raw_src))
			continue;
		if (last) {
			struct mbuf *n;
			smp_lock(&last->lk_socket, LK_RETRY);
			if (n = m_copy(m->m_next, 0, (int)M_COPYALL)) {
				if (sbappendaddr(&last->so_rcv, &rh->raw_src,
				    n, (struct mbuf *)0) == 0)
					/* should notify about lost packet */
					m_freem(n);
				else
					sorwakeup(last);
			}
			smp_unlock(&last->lk_socket);	
		}
		last = rp->rcb_socket;
	}
	if (last) {
		smp_lock(&last->lk_socket, LK_RETRY);
		if (sbappendaddr(&last->so_rcv, &rh->raw_src,
		    m->m_next, (struct mbuf *)0) == 0)
			m_freem(m->m_next);
		else
			sorwakeup(last);
		(void) m_free(m);		/* header */
		smp_unlock(&last->lk_socket);
	} else
		m_freem(m);
	smp_unlock(&rawcb_lk);

	goto next;
}

/*ARGSUSED*/
raw_ctlinput(cmd, arg)
	int cmd;
	struct sockaddr *arg;
{

	if (cmd < 0 || cmd > PRC_NCMDS)
		return;
	/* INCOMPLETE */
}

/*ARGSUSED*/
raw_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	register struct rawcb *rp = sotorawcb(so);
	register int error = 0;

	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic( "raw_usrreq not lock owner");
	}
	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);
	if (rights && rights->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (rp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	/*
	 * Allocate a raw control block and fill in the
	 * necessary info to allow packets to be routed to
	 * the appropriate raw interface routine.
	 */
	case PRU_ATTACH:
		if ((so->so_state & SS_PRIV) == 0) {
			error = EACCES;
			break;
		}
		if (rp) {
			error = EINVAL;
			break;
		}
		error = raw_attach(so, (int)nam);
		break;

	/*
	 * Destroy state just before socket deallocation.
	 * Flush data or not depending on the options.
	 */
	case PRU_DETACH:
		if (rp == 0) {
			error = ENOTCONN;
			break;
		}
		raw_detach(rp);
		break;

	/*
	 * If a socket isn't bound to a single address,
	 * the raw input routine will hand it anything
	 * within that protocol family (assuming there's
	 * nothing else around it should go to). 
	 */
	case PRU_CONNECT:
		if (rp->rcb_flags & RAW_FADDR) {
			error = EISCONN;
			break;
		}
		raw_connaddr(rp, nam);
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		goto release;

	case PRU_BIND:
		if (rp->rcb_flags & RAW_LADDR) {
			error = EINVAL;			/* XXX */
			break;
		}
		error = raw_bind(so, nam);
		break;

	case PRU_DISCONNECT:
		if ((rp->rcb_flags & RAW_FADDR) == 0) {
			error = ENOTCONN;
			break;
		}
		raw_disconnect(rp);
		soisdisconnected(so);
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
		if (nam) {
			if (rp->rcb_flags & RAW_FADDR) {
				error = EISCONN;
				break;
			}
			raw_connaddr(rp, nam);
		} else if ((rp->rcb_flags & RAW_FADDR) == 0) {
			error = ENOTCONN;
			break;
		}
		error = (*so->so_proto->pr_output)(m, so);
		m = NULL;
		if (nam)
			rp->rcb_flags &= ~RAW_FADDR;
		break;

	case PRU_ABORT:
		raw_disconnect(rp);
		soisdisconnected(so);
		/* SMP 1.31.89.  Nuked sofree - redundant. */
		/* sofree(so); */
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
	case PRU_RCVD:
		return(EOPNOTSUPP);

	case PRU_LISTEN:
	case PRU_ACCEPT:
	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		bcopy((caddr_t)&rp->rcb_laddr, mtod(nam, caddr_t),
		    sizeof (struct sockaddr));
		nam->m_len = sizeof (struct sockaddr);
		break;

	case PRU_PEERADDR:
		bcopy((caddr_t)&rp->rcb_faddr, mtod(nam, caddr_t),
		    sizeof (struct sockaddr));
		nam->m_len = sizeof (struct sockaddr);
		break;

	default:
		panic("raw_usrreq");
	}
release:
	if (m != NULL)
		m_freem(m);
	return (error);
}
