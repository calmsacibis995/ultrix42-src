#ifndef lint
static char *sccsid = "@(#)bsc_pcb.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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

/*	bsc_pcb.c	U. Sinkewicz	1.0	4/20/85			*/
/*	1/15/88 	L. Gottfredsen					*
 *			changes for malloc'd mbufs			*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/cpudata.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/kernel.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/net/if.h"
#include "../net/netbsc/bsc.h"
#include "../net/net/route.h"
#include "../net/netbsc/bsc_var.h"
#include "../net/netbsc/bsc_states.h"
#include "../net/netbsc/bsc_messages.h"
#include "../h/protosw.h"
#include "../h/types.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/errno.h"

int bscqmaxlen = IFQ_MAXLEN;
int saveaffinity;
/*
 * Initialize the head of the pcb queue before manipulating/making
 * the queue with insque().
 */
bsc_init()
	{
	saveaffinity = switch_affinity(boot_cpu_mask);
	bsb.bscp_next = bsb.bscp_prev = &bsb;
	bscintrq.ifq_maxlen = bscqmaxlen;
	}


bsc_ifioctl(ifp, cmd, data)
	struct ifnet *ifp;
	int cmd;
	caddr_t data;
	{
		return(0);
	}
	

int	bsc_sendspace = 1024*2;
int	bsc_recvspace = 1024*2;
/*
 * Attach BSC protocol to socket, allocating
 * a bsc protocol control block, and
 * buffer space.
 */
bsc_attach(so)
	struct socket *so;
{
	register struct bscpcb *bsc;
	int error;

	/* Allocate once, space for the socket send and receive queues.  */
	error = soreserve(so, bsc_sendspace, bsc_recvspace);
	if (error)
		goto bad;
	error = bsc_pcballoc(so, &bsb);
	if (error)
		goto bad;
	bsc = sotobscpcb(so);
	return (0);
bad:
	return (error);
}

bsc_pcballoc(so, head)
	struct socket *so;
	struct bscpcb *head;
{
	register struct bscpcb *bsc;

	KM_ALLOC(bsc, struct bscpcb *, sizeof(struct bscpcb), MT_PCB, KM_CLEAR|KM_NOWAIT);
	if (bsc == NULL)
		return (ENOBUFS);

	bsc->bscp_head = head;
	bsc->bscp_socket = so;
	
	/* Now put 'head' as the first element in the queue.  */
	insque(bsc, head);
	so->so_pcb = (caddr_t)bsc;
	return (0);
}
	
bsc_pcbbind(bsc, nam)
	register struct bscpcb *bsc;
	struct mbuf *nam;
{
	register struct socket *so = bsc->bscp_socket;
	register struct bscpcb *head = bsc->bscp_head;
	register struct sockaddr_bsc *sin;
	int i;

	if (bsc_ifaddr == 0)
		return (EADDRNOTAVAIL);

	/* 
	 * nam is (caddr_t)sin where sin is brought down from the
	 * application layer and contains the address family and
	 * an id# for the interface.
	 */
	sin = mtod(nam, struct sockaddr_bsc *);

	if (ifa_ifwithaddr((struct sockaddr *)sin) == 0){
		return (EADDRNOTAVAIL);
	}

	/* Hook for a modem with a synchronous autodialer.  Not used
	 * in ULTRIX V2.0
	 */
	for( i = 0; i<14; i++)
		bsc->bscp_laddr.sin_addr[i]=sin->sin_addr[i];

	bsc->bscp_laddr.sin_family = AF_BSC;
	return (0);
}

/*
 * Connect from a socket to a specified address.
 * When connect is called, we know that the interface exists and we have
 * stored the info in bscpcb ( all done in bind).
 * Hooks are in for a synchronous autodialer - the df126 does not
 * have a synch adialer so the code isn't exercised in V2.0.
 * To acitvate autodialing, in bscconifg, copy the phone number into
 * sin->sin_addr after you do the bind to the interface address.
 * In bsc_pcbconnect, copy sin->sin_addr into bsc->bscp_modem and
 * get this down to the driver as data.
 */
bsc_pcbconnect(bsc, nam)
	struct bscpcb *bsc;
	struct mbuf *nam;
{
	struct bsc_ifaddr *bia;
	register struct sockaddr_bsc *sin = mtod(nam, struct sockaddr_bsc *);
	int i;

	if (sin->sin_family != AF_BSC)
		return (EAFNOSUPPORT);
	if (bsc->bscp_state == MODEM){
		for ( i = 0; i<14; i++)
			 bsc->bscp_modem[i] = sin->sin_addr[i];
	}
	return (0);
}

/*
 * Initiate (or continue) disconnect.
 * Mark socket disconnecting and drop
 * current input data.
 */
struct bscpcb *
bsc_disconnect(bsc)
	register struct bscpcb *bsc;
{
	struct socket *so = bsc->bscp_socket;

	if ((so->so_options & SO_LINGER) && so->so_linger == 0)
		bsc= bsc_drop(bsc, 0);
	else {
		soisdisconnecting(so);
		sbflush(&so->so_rcv);
		bsc= bsc_usrclosed(bsc);
	}
	return (bsc);
}

/*
 * User issued close.
 */
struct bscpcb *
bsc_usrclosed(bsc)
	register struct bscpcb *bsc;
{

	bsc->bscp_state = CLOSED;
	bsc = bsc_close(bsc);

	return (bsc);
}

bsc_pcbdisconnect(bsc)
	struct bscpcb *bsc;
{

	if (bsc->bscp_socket->so_state & SS_NOFDREF)
		bsc_pcbdetach(bsc);
}

bsc_pcbdetach(bsc)
	struct bscpcb *bsc;
{
	struct socket *so = bsc->bscp_socket;

	so->so_pcb = 0;
	sofree(so);
	remque(bsc);
	KM_FREE(bsc, MT_PCB);
}

struct bscpcb *
bsc_drop(bsc, errno)
	register struct bscpcb *bsc;
	int errno;
{
	struct socket *so;

	so = bsc->bscp_socket;
	so->so_error = errno;
	return (bsc_close(bsc));
}

bsc_abort(bsc)
	struct bscpcb *bsc;
{
	(void) bsc_close((struct bscpcb *)bsc->bscp_ppcb);
}

/*
 * Close a BSC control block:
 *	discard all space held
 *	discard protocol block
 *	wake up any sleepers
 */
struct bscpcb *
bsc_close(bsc)
	register struct bscpcb *bsc;
{
	struct socket *so = bsc->bscp_socket;
	register struct mbuf *m;

	m = dtom(bsc);
	remque(bsc);
	(void)m_free(m);

	soisdisconnected(so);
	so->so_pcb = 0;
	sofree(so);
	return ((struct bscpcb *)0);

}

bsc_drain()
{
	return(0);
}

#ifdef BSC
/*
 * Generic bsc control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 */
bsc_control(so, cmd, data, ifp)
	struct socket *so;
	int cmd;
	caddr_t data;
	register struct ifnet *ifp;
{
	register struct ifreq *ifr = (struct ifreq *)data;
	register struct bsc_ifaddr *bia = 0;
	struct ifaddr *ifa;
	int error;
	int saveaffinity;

	saveaffinity = switch_affinity(boot_cpu_mask);
	/*
	 * Find addrlist for this interface, if it exists.
	 * Note that ifp was found earlier and was picked according to
	 * the device type.
	 */

	if (ifp){
		for (bia = bsc_ifaddr; bia; bia = bia->bia_next)
			if (bia->bia_ifp == ifp){
				break;
			}
	}

	switch (cmd) { 

	case SIOCGIFADDR:
	case SIOCGIFBRDADDR:
	case SIOCGIFDSTADDR:
	case SIOCGIFNETMASK:
		if (bia == (struct bsc_ifaddr *)0)
			return(EADDRNOTAVAIL);
		break;
	case SIOCSIFDSTADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFADDR:
	case SIOCSIFNETMASK:
	/* Found an ifp based on unit type.  Make an addrlist and assign
	 * it to the protocol family you are using.
	 */
	if (!suser())
		return( u.u_error);
	if (ifp == 0)
		panic(" bsc_control");
	if (bia == (struct bsc_ifaddr *)0) {
		struct bsc_ifaddr *tbsc;
		KM_ALLOC(tbsc, struct bsc_ifaddr *, sizeof(struct bsc_ifaddr), MT_IFADDR, KM_CLEAR|KM_NOWAIT);
		if (tbsc == NULL){
			return (ENOBUFS);}
		if (bia = bsc_ifaddr){
			for( ; bia->bia_next; bia = bia->bia_next)
				;
			bia->bia_next = tbsc;
		} else{
			bsc_ifaddr = tbsc;
		}
		bia = tbsc;
		if (ifa = ifp->if_addrlist){
			for( ; ifa->ifa_next; ifa = ifa->ifa_next)
				;
			ifa->ifa_next = (struct ifaddr *)bia;
		}else{
			ifp->if_addrlist = (struct ifaddr *)bia;}
		bia->bia_ifp = ifp;
	 	BIA_SIN(bia)->sin_family = AF_BSC; 
	}
		break;
	}

	switch (cmd) {
	
	case SIOCGIFADDR:
		ifr->ifr_addr = bia->bia_addr;
		break;
	case SIOCGIFBRDADDR:
	case SIOCGIFDSTADDR:
	case SIOCGIFNETMASK:
#define satosin(sa)	((struct sockaddr_bsc *)(sa))
	satosin(&ifr->ifr_addr)->sin_family = AF_BSC;
	break;

	case SIOCSIFDSTADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFADDR:
		bsc_ifinit(ifp, bia, &ifr->ifr_addr); 
		break;
	case SIOCSIFNETMASK:
		break;
	default:
		if (ifp == 0 || ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
		error = ((*ifp->if_ioctl)(ifp, cmd, data));
		RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		return(error);
	}
	return (0);
}

/*
 * Initialize an interface's address.
 */
bsc_ifinit(ifp, bia, sin)
	register struct ifnet *ifp;
	register struct bsc_ifaddr *bia;
	struct sockaddr_bsc *sin;
{
	struct ifaddr *ifa;
	int s = splimp(), error;
	int i;
	int saveaffinity;

	bia->bia_addr = *(struct sockaddr *)sin;	
	bia->bia_addr.sa_family  = AF_BSC; 		

	 /* Give the interface a chance to initialize.	*/
	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, bia);
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
	if (error){
		splx(s);
		bzero((caddr_t)&bia->bia_addr, sizeof(bia->bia_addr));
		return (error);
	}

	splx(s);
	return (0);
}

/*
 * Routine to find bsc.
 * YOU MAY WANT TO MAKE THIS A MORE SELECTIVE SEARCH!!
 */
struct bscpcb *
bsc_pcblookup(head)
	struct bscpcb *head;

{
	register struct bscpcb *bscp = 0;

	for ( bscp = head->bscp_next; bscp != head; bscp= bscp->bscp_next){
		if (bscp->bscp_laddr.sin_family == AF_BSC){
			return( bscp );
		}
	}

	return((struct bscpcb *)0);
}

struct bsc_ifaddr *
bsc_biafinder(ifp)
	struct ifnet *ifp;
{
	register struct bsc_ifaddr *bia = 0;

	for( bia = bsc_ifaddr; bia; bia->bia_next)
		if( bia->bia_ifp == ifp )
			return(bia);

	return((struct bsc_ifaddr *)0);
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifnet *
bsc_ifpfinder(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp = 0;
	register struct ifaddr *ifa;

#define	equal(a1, a2) \
	(bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if (equal(&ifa->ifa_addr, addr)){
			return (ifp);
		}
	}
	return ((struct ifnet *)0);
}

#endif

