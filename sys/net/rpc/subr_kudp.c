#ifndef lint
static	char	*sccsid = "@(#)subr_kudp.c	4.1	(ULTRIX)	7/2/90";
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
 *
 *	30-May-89	U. Sinkewicz
 *	Replaced smp_lock(&so->lk_socket, LK_RETRY) with SO_LOCK()
 *	as part of an smp bug fix.  Fix guarantees that socket
 *	doesn't change while unlocked during sleeps or for the
 *	lock hierarchy.
 *
 *	21-Jun-88	condylis
 *	Replaced sblock call with smp safe sblock2
 *
 *	15-Jan-88	lp
 *	Changes for final 43BSD release (m_act/m_next change). 
 */

/*
 * subr_kudp.c
 * Subroutines to do UDP/IP sendto and recvfrom in the kernel
 *
 */
#include "../h/param.h"
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

struct mbuf     *mclgetx();

/*
 * General kernel udp stuff.
 * The routines below are used by both the client and the server side
 * rpc code.
 */

/*
 * Kernel recvfrom.
 * Pull address mbuf and data mbuf chain off socket receive queue.
 */
struct mbuf *
ku_recvfrom(so, from)
	register struct socket *so;
	struct sockaddr_in *from;
{
	register struct mbuf	*m;
	register struct mbuf	*m0;
	int		eor = 0;
	int		len = 0;

#ifdef RPCDEBUG
	rpc_debug(4, "ku_recvfrom so=%X\n", so);
#endif
/* 	New sblock2 for smp
	sblock(&so->so_rcv); */
	sblock2(&so->so_rcv, so);
	m = so->so_rcv.sb_mb;
	if (m == NULL) {
		sbunlock(&so->so_rcv);
		return (m);
	}

	/*
	 * ASSUMPTIONS: sb_mb looks like addr->data->addr...
	 * where addr and data are linked by m_act, and the data chain is
	 * linked by m_next. First we copy out the address and skip to the
	 * head of the data chain. We move sb_mb past the data chain to the
	 * addr for the next packet.  Zero length mbufs are stripped from the
	 * front of the data chain, and the chain is passed to the caller.
	 */
	*from = *mtod(m, struct sockaddr_in *);
	sbfree(&so->so_rcv, m);

	/*
	 * m0 points to the data chain
	 */
	/* Hopefully no rights here else this wont work - lp */
	m0 = m->m_next;

	/*
	 * set sock buf to point past data and skip zero length mbufs 
	 */
	so->so_rcv.sb_mb = m->m_act;

	/*
	 * free the addr mbuf
	 */
	m_free(m);

	for ( ; ; ) {
		if (m0 == NULL) {
			printf("cku_recvfrom: no body!\n");
			so->so_rcv.sb_mb = m0;
			sbunlock(&so->so_rcv);
			return (NULL);
		}
		if (m0->m_len == 0) {
			sbfree(&so->so_rcv, m0);
			m0 = m_free(m0);
		} else {
			break;
		}
	}

	/*
	 * Walk down mbuf chain till m_next NULL (end of packet),
	 * freeing socket buffer space as we go.
	 */
	m = m0;
	for (;;) {
		sbfree(&so->so_rcv, m);
		len += m->m_len;
		if (m->m_next == NULL) {
			break;
		}
		m = m->m_next;
	}

	if (len > UDPMSGSIZE) {
		printf("ku_recvfrom: len = %d\n", len);
	}

#ifdef RPCDEBUG
	rpc_debug(4, "urrecvfrom %d from %X\n", len, from->sin_addr.s_addr);
#endif

	sbunlock(&so->so_rcv);
	return (m0);
}

struct mbuf *mclcopy();
struct mbuf *m_copy2();

int Sendtries = 0;
int Sendok = 0;

int use_fastsend = 1;
int use_type1 = 0;

/*
 * Kernel sendto.
 * Set addr and send off via UDP.
 * Use ku_fastsend if possible.
 */
int
ku_sendto_mbuf(so, m, addr)
	struct socket *so;
	struct mbuf *m;
	struct sockaddr_in *addr;
{
	register struct inpcb *inp = sotoinpcb(so);
	int error;
	int s;
	struct in_addr laddr;

#ifdef RPCDEBUG
	rpc_debug(4, "ku_sendto_mbuf %X\n", addr->sin_addr.s_addr);
#endif

	Sendtries++;
	if (use_type1) {
		struct mbuf *m1;
		m1 = m_copy2(m);
		m_freem(m);
		m = m1;
	}
	else if (use_fastsend) {
		if ((error = ku_fastsend(so, m, addr)) == 0) {
			Sendok++;
			return (0);
		} else {
			/*
			 * ku_fastsend has failed...if it's a valid errno just
			 * give up (the mbuf chain has been freed), else fall
			 * into the code below and try to send it via udp_output.
			 */
			if (error > 0)
				return(error);
		}
	}
	s = splnet();
	laddr = inp->inp_laddr;
	if (error = in_pcbsetaddr(inp, addr)) {
		printf("ku_sendto_mbuf: pcbsetaddr failed %d\n", error);
		(void)splx(s);
 		m_freem(m);
		return (error);
	}
	error = udp_output(inp, m);
	/* This routine expects to see socket lock on	*/
	/* smp_lock(&so->lk_socket, LK_RETRY); */
	SO_LOCK(so);
	in_pcbdisconnect(inp);
	smp_unlock(&so->lk_socket);
	inp->inp_laddr = laddr;

#ifdef RPCDEBUG
	rpc_debug(4, "ku_sendto returning %d\n", error);
#endif

	Sendok++;
	(void)splx(s);
	return (error);
}

/*
 * Copy a type 2 mbuf chain into a chain of traditional 1k mbuf clusters.
 * Copying is presumed to be necessary, no checks are made.  This
 * routine is intended to be used for "safe" fragmentation of type 2
 * mbuf chains, for communication drivers that expect cluster mbuf
 * data to be page-aligned and/or physically contiguous.
 */

struct mbuf *
m_copy2(m)
	register struct mbuf *m;
{
	register struct mbuf *n, **np;
	struct mbuf *top;
	struct mbuf *p;

	np = &top;
	top = 0;
	while(m) {
		if (m->m_off > MMAXOFF && m->m_cltype == 2) {
			*np = n = mclcopy(m, 0, m->m_len);
			if (n == 0)
				goto nospace;
		}
		else panic("m_copy2 got a bad mbuf");

		m = m->m_next;
		np = &n->m_next;
		while (*np)
			np = &(((struct mbuf *)*np)->m_next);
	}
	return (top);
nospace:
	m_freem(top);
	return (0);
}



struct mbuf *
mclcopy(m, off, len)
	struct mbuf *m;
	int off, len;
{
	struct mbuf *n = 0, *mcl;
	struct mbuf *top = 0;
	struct mbuf **ln = &top;
	int count;

	while (len) {
		MGET(n, M_WAIT, m->m_type);
		if (n == 0)
			goto nospace;
		MCLGET(n, mcl);
		if (mcl == 0)
			goto nospace;
		count = MIN(len, NCLBYTES);
		bcopy(mtod(m, caddr_t) + off, (caddr_t)mcl, count);
		n->m_len = count;
		*ln = n;
		ln = &(n->m_next);
		len -= count;
		off += count;
	}
	*ln = (struct mbuf *)NULL;
	return(top);
nospace:
	m_freem(top);
	return((struct mbuf *)NULL);
}

#ifdef RPCDEBUG

int rpcdebug = 2;

/* VARARGS2 */
rpc_debug(level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
        int level;
        char *str;
        int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
        if (level <= rpcdebug)
                printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

#endif RPCDEBUG
