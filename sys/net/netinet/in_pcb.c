#ifndef lint
static char *sccsid = "@(#)in_pcb.c	4.2	(ULTRIX)		12/6/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/************************************************************************
 *			Modification History				*
 *	8-Nov-89	Ursula Sinkewicz
 *		Remove lk_ifnet and lk_in_ifaddr to coincide with
 *		changes to slip.
 *
 *      05-May-89       Michael G. Mc Menemy
 *              Add XTI Support.
 *	27-Mar-89	U. Sinkewicz
 *		Lowered ipl of lk_rtentry, lk_ifnet, lk_in_ifaddr, as per
 *		lp changes 3/16/89. 	 
 *	3-Mar-89	U. Sinkewicz
 *		Changes to support new directory layout.
 *	13-Feb-89	Ursula Sinkewicz
 *		Added lk_ifnet and lk_in_ifaddr.	
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes. Use new memory allocation
 *	scheme for mbufs.
 *
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/



/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	in_pcb.c	7.5 (Berkeley) 8/24/87
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/smp_lock.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/net/if.h"
#include "../net/net/route.h"
#include "../net/netinet/in_pcb.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/tcp.h"		/* SMP */
#include "../net/netinet/tcp_timer.h"	/* SMP */
#include "../net/netinet/tcp_var.h"		/* SMP */
#include "../h/protosw.h"

struct	in_addr zeroin_addr;
extern struct  lock_t  lk_udb;
/*
 * SMP: Need to lock the pcb chain before update because another
 * process can be doing another in_pcballoc for another socket but for
 * the same protocol.  Routine is used for tcp and udp so lock tdb and
 * udb queues outside in_pcballoc.  Bug fix  6.3.76.us
 * Socket lock held coming in.  Should be no need to elevate ipl.
 */
in_pcballoc(so, head)
	struct socket *so;
	struct inpcb *head;
{
	register struct inpcb *inp;

	KM_ALLOC(inp, struct inpcb *, sizeof(struct inpcb), KM_PCB, KM_CLEAR|KM_NOWAIT); 
	if(inp == NULL)
		return(ENOBUFS);
	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic("in_pcballoc not lock owner");
	}
	inp->inp_head = head;
	inp->inp_socket = so;
	insque(inp, head);
	so->so_pcb = (caddr_t)inp;
	return (0);
}
	
/*
 * SMP:  Enter with socket lock set.  Called from tcp_usrreq, PRU_BIND,
 * and PRU_CONNECT.
 */
in_pcbbind(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct socket *so = inp->inp_socket;
	register struct inpcb *head = inp->inp_head;
	register struct sockaddr_in *sin;
	u_short lport = 0;
	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic("in_pcbbind not lock owner");
	}
	if (in_ifaddr == 0)
		return (EADDRNOTAVAIL);
	if (inp->inp_lport || inp->inp_laddr.s_addr != INADDR_ANY)
		return (EINVAL);
	if (nam == 0)
		goto noname;
	sin = mtod(nam, struct sockaddr_in *);
	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	if (sin->sin_addr.s_addr != INADDR_ANY) {
		int tport = sin->sin_port;

		sin->sin_port = 0;		/* yech... */
		if (ifa_ifwithaddr((struct sockaddr *)sin) == 0)
			return (EADDRNOTAVAIL);
		sin->sin_port = tport;
	}
	lport = sin->sin_port;
	if (lport) {
		u_short aport = ntohs(lport);
		int wild = 0;

		/* GROSS */
		if (aport < IPPORT_RESERVED && u.u_uid != 0)
			return (EACCES);
		/* even GROSSER, but this is the Internet */
		if ((so->so_options & SO_REUSEADDR) == 0 &&
		    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
		     (so->so_options & SO_ACCEPTCONN) == 0))
			wild = INPLOOKUP_WILDCARD;
		if (in_pcblookup(head,
		    zeroin_addr, 0, sin->sin_addr, lport, wild))
			return (EADDRINUSE);
	}
	inp->inp_laddr = sin->sin_addr;
noname:
	if (lport == 0)
		do {
			if (head->inp_lport++ < IPPORT_RESERVED ||
				head->inp_lport > IPPORT_USERRESERVED)
				head->inp_lport = IPPORT_RESERVED;
			lport = htons(head->inp_lport);
		} while (in_pcblookup(head,
			    zeroin_addr, 0, inp->inp_laddr, lport, INPLOOKUP_WILDCARD));
	inp->inp_lport = lport;
	return (0);
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
/* SMP: socket lock held coming in, at splnet.
 */
in_pcbconnect(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
	if (smp_debug){
		if (smp_owner(&inp->inp_socket->lk_socket) == 0)
			panic("in_pcbconnect not lock owner");
	}
	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	return(in_pcbsetaddr(inp,sin));
}

in_pcbsetaddr(inp, sin)
	struct inpcb *inp;
	register struct sockaddr_in *sin;
{
	struct in_ifaddr *ia;
	struct sockaddr_in *ifaddr;

	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (in_ifaddr) {
		/*
		 * If the destination address is INADDR_ANY,
		 * use the primary local address.
		 * If the supplied address is INADDR_BROADCAST,
		 * and the primary interface supports broadcast,
		 * choose the broadcast address for that interface.
		 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
		if (sin->sin_addr.s_addr == INADDR_ANY)
		    sin->sin_addr = IA_SIN(in_ifaddr)->sin_addr;
		else if (sin->sin_addr.s_addr == (u_long)INADDR_BROADCAST &&
		  (in_ifaddr->ia_ifp->if_flags & IFF_BROADCAST))
		    sin->sin_addr = satosin(&in_ifaddr->ia_broadaddr)->sin_addr;
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		register struct route *ro;
		struct ifnet *ifp;

		ia = (struct in_ifaddr *)0;
		/* 
		 * If route is known or can be allocated now,
		 * our src addr is taken from the i/f, else punt.
		 */
		ro = &inp->inp_route;
		RTLOCK();
		if (ro->ro_rt &&
		    (satosin(&ro->ro_dst)->sin_addr.s_addr !=
			sin->sin_addr.s_addr || 
		    inp->inp_socket->so_options & SO_DONTROUTE)) {
			rtfree(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)0;
		}
		if ((inp->inp_socket->so_options & SO_DONTROUTE) == 0 && /*XXX*/
		    (ro->ro_rt == (struct rtentry *)0 ||
		    ro->ro_rt->rt_ifp == (struct ifnet *)0)) {
			/* No route yet, so try to acquire one */
			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				sin->sin_addr;
			rtalloc(ro);
		}
		/*
		 * If we found a route, use the address
		 * corresponding to the outgoing interface
		 * unless it is the loopback (in case a route
		 * to our address on another net goes to loopback).
		 */
		if (ro->ro_rt && (ifp = ro->ro_rt->rt_ifp) &&
		    (ifp->if_flags & IFF_LOOPBACK) == 0){
			for (ia = in_ifaddr; ia; ia = ia->ia_next)
				if (ia->ia_ifp == ifp)
					break;
		}
		RTUNLOCK();
		if (ia == 0) {
			int fport = sin->sin_port;

			sin->sin_port = 0;
			ia = (struct in_ifaddr *)
			    ifa_ifwithdstaddr((struct sockaddr *)sin);
			sin->sin_port = fport;
			if (ia == 0)
				ia = in_iaonnetof(in_netof(sin->sin_addr));
			if (ia == 0){
				ia = in_ifaddr;
			}
			if (ia == 0)
				return (EADDRNOTAVAIL);
		}
		ifaddr = (struct sockaddr_in *)&ia->ia_addr;
	}
	if (in_pcblookup(inp->inp_head,
	    sin->sin_addr,
	    sin->sin_port,
	    inp->inp_laddr.s_addr ? inp->inp_laddr : ifaddr->sin_addr,
	    inp->inp_lport,
	    0))
		return (EADDRINUSE);
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		if (inp->inp_lport == 0)
			(void)in_pcbbind(inp, (struct mbuf *)0);
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	return (0);
}

in_pcbdisconnect(inp)
	struct inpcb *inp;
{
	if (smp_debug){
		if (smp_owner(&inp->inp_socket->lk_socket) == 0)
			panic("in_pcbdisconnect not lock owner");
	}
	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;
	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
}

/*
 * SMP: Queue manipulation must be done with the inp queue (tcb) intact.
 * Grab the tcb lock and then delete the pcb.  Look at in_pcballoc() for
 * the parallel situation of inserting an element into the tcb list.
 */
in_pcbdetach(inp)
	struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;
	if (smp_debug){
		if (smp_owner(&inp->inp_socket->lk_socket) == 0)
			panic("in_pcbdetach not lock owner");
	}
	if (inp->inp_options)
		(void)m_free(inp->inp_options);
	RTLOCK();
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
	RTUNLOCK();
	remque(inp);
	KM_FREE(inp, KM_PCB);
	so->so_pcb = 0;
	sofree(so);
	return;
}

in_setsockaddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

in_setpeeraddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  Call the protocol specific
 * routine (if any) to handle each connection.
 */
/*
 * SMP: Enter here through tcp_ctlinput, udp_ctlinput.
 * Lock head before calling in_pcbnotify.  
 */
in_pcbnotify(head, dst, errno, notify)
	struct inpcb *head;
	register struct in_addr *dst;
	int errno, (*notify)();
{
	register struct inpcb *inp, *oinp;
	/* Removed by SMP: int s = splimp(); */

	/* Make sure we have something  && May need to check
	   for local address match as well to force remote side
	   to hear notify */
	for (inp = head->inp_next; inp != head;) {
		if (inp->inp_faddr.s_addr != dst->s_addr ||
		    inp->inp_socket == 0) {
			inp = inp->inp_next;
			continue;
		}
		if (errno) 
			inp->inp_socket->so_error = errno;
		oinp = inp;
		inp = inp->inp_next;
		if (notify)
			(*notify)(oinp);
	}
	/* removed by SMP:  splx(s); */
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */
in_losing(inp)
	struct inpcb *inp;
{
	register struct rtentry *rt;

	RTLOCK();
	if ((rt = inp->inp_route.ro_rt)) {
		if (rt->rt_flags & RTF_DYNAMIC)
			(void) rtrequest((int)SIOCDELRT, rt);
		rtfree(rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated
		 * the next time output is attempted.
		 */
	}
	RTUNLOCK();
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
in_rtchange(inp)
	register struct inpcb *inp;
{
	RTLOCK();
	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
	/* SHOULD NOTIFY HIGHER-LEVEL PROTOCOLS */
	RTUNLOCK();
}

struct inpcb *
in_pcblookup(head, faddr, fport, laddr, lport, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_short fport, lport;
	int flags;
{
	register struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;
	int s; /* SMP */

	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			/* receiver not looking for broadcast */
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
	return (match);
}
#ifdef XTI
/*
 * SMP: Queue manipulation must be done with the inp queue (tcb) intact.
 * Grab the tcb lock and then delete the pcb.  Look at in_pcballoc() for
 * the parallel situation of inserting an element into the tcb list.
 */
xtiin_pcbunbind(inp)
	struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;
	if (smp_debug){
		if (smp_owner(&inp->inp_socket->lk_socket) == 0)
			panic("xtiin_pcbunbind not lock owner");
	}
	if (inp->inp_options) {
		(void)m_free(inp->inp_options);
		inp->inp_options = 0;
	      }

	RTLOCK();
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
	RTUNLOCK();

	bzero(&inp->inp_route, sizeof(struct route));
	bzero(&inp->inp_faddr, sizeof(struct in_addr));
	bzero(&inp->inp_laddr, sizeof(struct in_addr));
	inp->inp_fport = 0;
	inp->inp_lport = 0;

	/* all set except for inp->inp_ppcb which will be
	 * cleaned up at a higher level
	 */
	
}
#endif XTI
