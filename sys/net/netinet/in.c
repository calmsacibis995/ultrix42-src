#ifndef lint
static char *sccsid = "@(#)in.c	4.1		(ULTRIX)	7/2/90";
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
 *		Remove lk_ifnet and lk_in_ifaddr to coincides with 
 *		changes to slip.
 *
 *	9-June-89	Ursula Sinkewicz
 *		Added changes for asymmetric network drivers.
 *
 *	27-Mar-89	Ursula Sinkewicz
 *		Lower ipl of lk_ifnet, lk_in_ifaddr, lk_retentry as
 *		per lp changes 3//16/89.
 *
 *	3-Mar-89	Ursula Sinkewicz
 *		Pmax/smp merge.  Added CLASSA/B checks in in_localaddr.
 *
 *	13-Feb-89	Ursula Sinkewicz
 *		Changed kmalloc flag to nowait.  Added assertions of 
 *		lk_in_ifaddr and lk_ifnet.
 *	
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 *	Larry Cohen  -  01/17/86					*
 *		Add boolean: net_conservative.  If set then a different *
 *		subnet will be viewed as non-local in in_localaddr().   *
 *		This will result in tcp using the default maxseg size   *
 *		instead of the mtu of the local interface.		*
 *									*
 *	Marc Teitelbaum and Fred Templin - 08/21/86			*
 *		Added 4.3BSD beta tape enhancements. "in_interfaces"	*
 *		to count number of physical interfaces attached. Also,	*
 *		new code for SICSIFBRDADDR ioctl to init		*
 *		"ia_netbroadcast"					*
 *									*
 *	12/16/86 - lp							*
 *		Bugfix for changing addr.				*
 *									*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	in.c	7.5 (Berkeley) 6/4/87
 */

#include "../h/param.h"
#include "../h/ioctl.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"
#include "../h/dir.h"

#include "../h/cpudata.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/smp_lock.h"
#include "../net/netinet/in_systm.h"
#include "../net/net/if.h"
#include "../net/net/route.h"
#include "../net/net/af.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_var.h"

struct lock_t lk_in_ifaddr;	/* smp */
#ifdef INET
inet_hash(sin, hp)
	register struct sockaddr_in *sin;
	struct afhash *hp;
{
	register u_long n;

	n = in_netof(sin->sin_addr);
	if (n)
	    while ((n & 0xff) == 0)
		n >>= 8;
	hp->afh_nethash = n;
	hp->afh_hosthash = ntohl(sin->sin_addr.s_addr);
}

inet_netmatch(sin1, sin2)
	struct sockaddr_in *sin1, *sin2;
{

	return (in_netof(sin1->sin_addr) == in_netof(sin2->sin_addr));
}

/*
 * Formulate an Internet address from network + host.
 */
struct in_addr
in_makeaddr(net, host)
	u_long net, host;
{
	register struct in_ifaddr *ia;
	register u_long mask;
	u_long addr;

	if (IN_CLASSA(net))
		mask = IN_CLASSA_HOST;
	else if (IN_CLASSB(net))
		mask = IN_CLASSB_HOST;
	else
		mask = IN_CLASSC_HOST;
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if ((ia->ia_netmask & net) == ia->ia_net) {
			mask = ~ia->ia_subnetmask;
			break;
		}
	addr = htonl(net | (host & mask));
	return (*(struct in_addr *)&addr);
}

/*
 * Return the network number from an internet address.
 */
u_long
in_netof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net;
	register struct in_ifaddr *ia;

	if (IN_CLASSA(i))
		net = i & IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		net = i & IN_CLASSB_NET;
	else if (IN_CLASSC(i))
		net = i & IN_CLASSC_NET;
	else
		return (0);

	/*
	 * Check whether network is a subnet;
	 * if so, return subnet number.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if ((ia->ia_netmask & net) == ia->ia_net){
			return (i & ia->ia_subnetmask);
		}
	return (net);
}

/*
 * Return the host portion of an internet address.
 */
u_long
in_lnaof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net, host;
	register struct in_ifaddr *ia;
	u_long host_return;

	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		host = i & IN_CLASSA_HOST;
	} else if (IN_CLASSB(i)) {
		net = i & IN_CLASSB_NET;
		host = i & IN_CLASSB_HOST;
	} else if (IN_CLASSC(i)) {
		net = i & IN_CLASSC_NET;
		host = i & IN_CLASSC_HOST;
	} else
		return (i);

	/*
	 * Check whether network is a subnet;
	 * if so, use the modified interpretation of `host'.
	 */
/* NO LOCK */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if ((ia->ia_netmask & net) == ia->ia_net){
			host_return = host &~ ia->ia_subnetmask;
			return (host_return);
		}
	return (host);
}

#ifndef SUBNETSARELOCAL
#define SUBNETSARELOCAL 1
#endif
int subnetsarelocal = SUBNETSARELOCAL;
/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */
in_localaddr(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register struct in_ifaddr *ia;
	u_long net;

        if (IN_CLASSA(i))
                net = i & IN_CLASSA_NET;
        else if (IN_CLASSB(i))
                net = i & IN_CLASSB_NET;
        else
                net = i & IN_CLASSC_NET;

        for (ia = in_ifaddr; ia; ia = ia->ia_next)
                if (net == (subnetsarelocal ? ia->ia_net : ia->ia_subnet)){
                        return (1);
		}
	return (0);
}

/*
 * Determine whether an IP address is in a reserved set of addresses
 * that may not be forwarded, or whether datagrams to that destination
 * may be forwarded.
 */
in_canforward(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net;

	if (IN_EXPERIMENTAL(i))
		return (0);
	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		if (net == 0 || net == IN_LOOPBACKNET)
			return (0);
	}
	return (1);
}

int	in_interfaces;		/* number of external internet interfaces */
extern	struct ifnet loif;

/*
 * Generic internet control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 */
/* ARGSUSED */
in_control(so, cmd, data, ifp)
	struct socket *so;
	int cmd;
	caddr_t data;
	register struct ifnet *ifp;
{
	register struct ifreq *ifr = (struct ifreq *)data;
	register struct in_ifaddr *ia = 0;
	u_long tmp;
	struct ifaddr *ifa;
	struct mbuf *m;
	int error;
	int saveaffinity;  /* support for nonsmp dvrs.  8.9.88.us */

	/*
	 * Find address for this interface, if it exists.
	 */
	if (ifp){
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if (ia->ia_ifp == ifp)
				break;
	}

	switch (cmd) {

	case SIOCSIFADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFDSTADDR:
		if (!suser())
			return (u.u_error);

		if (ifp == 0)
			panic("in_control");
		if (ia == (struct in_ifaddr *)0) {
			struct in_ifaddr *ina;
			KM_ALLOC(ina, struct in_ifaddr *, sizeof(struct in_ifaddr), KM_IFADDR, KM_CLEAR|KM_NOWAIT);
			if (ina == NULL)
				return(ENOBUFS);
			if (ia = in_ifaddr) { /* add to end of inet list */
				for ( ; ia->ia_next; ia = ia->ia_next)
					;
				ia->ia_next = ina;
			} else  /* start list */
				in_ifaddr = ina;
			ia = ina;
			if (ifa = ifp->if_addrlist) {
				/* add to end of interface list of addr. 
					families supported by the device */
				for ( ; ifa->ifa_next; ifa = ifa->ifa_next)
					;
				ifa->ifa_next = (struct ifaddr *) ia;
			} else  /* start list */
				ifp->if_addrlist = (struct ifaddr *) ia;
			ia->ia_ifp = ifp;
			IA_SIN(ia)->sin_family = AF_INET;
			if (ifp != &loif)  /* only count real interfaces */
				in_interfaces++;
		}
		break;

	case SIOCSIFBRDADDR:
		if (!suser())
			return (u.u_error);
		/* FALLTHROUGH */

	default:
		if (ia == (struct in_ifaddr *)0)
			return (EADDRNOTAVAIL);
		break;
	}

	switch (cmd) {

	case SIOCGIFADDR:
		ifr->ifr_addr = ia->ia_addr;
		break;

	case SIOCGIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0){
			return (EINVAL);
		}
		ifr->ifr_dstaddr = ia->ia_broadaddr;
		break;

	case SIOCGIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0){
			return (EINVAL);
		}
		ifr->ifr_dstaddr = ia->ia_dstaddr;
		break;

	case SIOCGIFNETMASK:
#define	satosin(sa)	((struct sockaddr_in *)(sa))
		satosin(&ifr->ifr_addr)->sin_family = AF_INET;
		satosin(&ifr->ifr_addr)->sin_addr.s_addr = htonl(ia->ia_subnetmask);
		break;

	case SIOCSIFDSTADDR:
	    {
		struct sockaddr oldaddr;
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0){
			return (EINVAL);
		}
		oldaddr = ia->ia_dstaddr;
		ia->ia_dstaddr = ifr->ifr_dstaddr;
		if (ifp->d_affinity != boot_cpu_mask)
		        error = (*ifp->if_ioctl)(ifp, SIOCSIFDSTADDR, ia);
		else{
			smp_unlock(&so->lk_socket);
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
		        error = (*ifp->if_ioctl)(ifp, SIOCSIFDSTADDR, ia);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			smp_lock(&so->lk_socket, LK_RETRY);
		}
		if (ifp->if_ioctl && error) {
			ia->ia_dstaddr = oldaddr;
			return (error);
		}
		if (ia->ia_flags & IFA_ROUTE) {
			rtinit(&oldaddr, &ia->ia_addr, (int)SIOCDELRT,
				RTF_HOST);
			rtinit(&ia->ia_dstaddr, &ia->ia_addr, (int)SIOCADDRT,
				RTF_HOST|RTF_UP);
		}
	    }
		break;

	case SIOCSIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0){
			return (EINVAL);
		}
		ia->ia_broadaddr = ifr->ifr_broadaddr;
		tmp = ntohl(satosin(&ia->ia_broadaddr)->sin_addr.s_addr);
		if ((tmp &~ ia->ia_subnetmask) == ~ia->ia_subnetmask)
			tmp |= ~ia->ia_netmask;
		else if ((tmp &~ ia->ia_subnetmask) == 0)
			tmp &= ia->ia_netmask;
		ia->ia_netbroadcast.s_addr = htonl(tmp);
		break;

	case SIOCSIFADDR:
		return (in_ifinit(ifp, ia, &ifr->ifr_addr));

	case SIOCSIFNETMASK:
		ia->ia_subnetmask = ntohl(satosin(&ifr->ifr_addr)->sin_addr.s_addr);
		break;

	default:
		if (ifp == 0 || ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		if (ifp->d_affinity != boot_cpu_mask)
			error = (*ifp->if_ioctl)(ifp, cmd, data);
		else{
			smp_unlock(&so->lk_socket);
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
			error = (*ifp->if_ioctl)(ifp, cmd, data);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			smp_lock(&so->lk_socket, LK_RETRY);
		}
		return (error);
	}
	return (0);
}

/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
in_ifinit(ifp, ia, sin)
	register struct ifnet *ifp;
	register struct in_ifaddr *ia;
	struct sockaddr_in *sin;
{
	register u_long i = ntohl(sin->sin_addr.s_addr);
	struct sockaddr oldaddr;
	struct sockaddr_in netaddr;
	int s = splnet(), error;
	int saveaffinity;

	oldaddr = ia->ia_addr;
	ia->ia_addr = *(struct sockaddr *)sin;
	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, ia);
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
	if (ifp->if_ioctl && error){
		ia->ia_addr = oldaddr;
		splx(s);
		return (error);
	}
	/*
	 * Delete any previous route for an old address.
	 */
	bzero((caddr_t)&netaddr, sizeof (netaddr));
	netaddr.sin_family = AF_INET;
	if (ia->ia_flags & IFA_ROUTE) {
		if (ifp->if_flags & IFF_LOOPBACK){
		    rtinit(&oldaddr, &oldaddr, (int)SIOCDELRT, RTF_HOST);
		}
		else if (ifp->if_flags & IFF_POINTOPOINT){
		    rtinit(&ia->ia_dstaddr, &oldaddr, (int)SIOCDELRT,
			RTF_HOST);
		}
		else {
		    netaddr.sin_addr = in_makeaddr(ia->ia_subnet,
			INADDR_ANY);
		    rtinit((struct sockaddr *)&netaddr, &oldaddr,
			(int)SIOCDELRT, 0); 
		}
		ia->ia_flags &= ~IFA_ROUTE;
	}
	if (IN_CLASSA(i))
		ia->ia_netmask = IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		ia->ia_netmask = IN_CLASSB_NET;
	else
		ia->ia_netmask = IN_CLASSC_NET;
	ia->ia_net = i & ia->ia_netmask;
	/*
	 * The subnet mask includes at least the standard network part,
	 * but may already have been set to a larger value.
	 */
	ia->ia_subnetmask |= ia->ia_netmask;
	ia->ia_subnet = i & ia->ia_subnetmask;
	if (ifp->if_flags & IFF_BROADCAST) {
		ia->ia_broadaddr.sa_family = AF_INET;
		((struct sockaddr_in *)(&ia->ia_broadaddr))->sin_addr =
			in_makeaddr(ia->ia_subnet, INADDR_BROADCAST);
		ia->ia_netbroadcast.s_addr =
		    htonl(ia->ia_net | (INADDR_BROADCAST &~ ia->ia_netmask));
	}
	splx(s);
	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, ia);
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
	if (ifp->if_ioctl && error){
		bzero((caddr_t)&ia->ia_addr, sizeof(ia->ia_addr));
		return (error);
	}
	/*
	 * Add route for the network.
	 */
	if (ifp->if_flags & IFF_LOOPBACK)
		rtinit(&ia->ia_addr, &ia->ia_addr, (int)SIOCADDRT,
			RTF_HOST|RTF_UP);
	else if (ifp->if_flags & IFF_POINTOPOINT)
		rtinit(&ia->ia_dstaddr, &ia->ia_addr, (int)SIOCADDRT,
			RTF_HOST|RTF_UP);
	else {
		netaddr.sin_addr = in_makeaddr(ia->ia_subnet, INADDR_ANY);
		rtinit((struct sockaddr *)&netaddr, &ia->ia_addr,
			(int)SIOCADDRT, RTF_UP);
	}
	ia->ia_flags |= IFA_ROUTE;
	return (0);
}

/*
 * Return address info for specified internet network.
 */
struct in_ifaddr *
in_iaonnetof(net)
	u_long net;
{
	register struct in_ifaddr *ia;

	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (ia->ia_subnet == net){
			return (ia);
		}
	return ((struct in_ifaddr *)0);
}

/*
 * Return 1 if the address might be a local broadcast address.
 */
in_broadcast(in)
	struct in_addr in;
{
	register struct in_ifaddr *ia;
	u_long t;

	/*
	 * Look through the list of addresses for a match
	 * with a broadcast address.
	 */
/* NO LOCK */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
	    if (ia->ia_ifp->if_flags & IFF_BROADCAST) {
		if (satosin(&ia->ia_broadaddr)->sin_addr.s_addr == in.s_addr) {
		    return (1);
		}
		/*
		 * Check for old-style (host 0) broadcast.
		 */
		if ((t = ntohl(in.s_addr)) == ia->ia_subnet || t == ia->ia_net) {
		    return (1);
		}
	}
	if (in.s_addr == INADDR_BROADCAST || in.s_addr == INADDR_ANY) {
		return (1);
	}
	return (0);
}

in_reminterface(ifp) 
struct ifnet *ifp;
{
	struct in_ifaddr *ia, *lia;

again:
	if(ifp)
		for(lia = ia = in_ifaddr; ia; ia = ia->ia_next) {
			if(ia->ia_ifp == ifp) {
				if(ia == in_ifaddr)
					in_ifaddr = ia->ia_next;
				else
					lia->ia_next = ia->ia_next;
			/*	KM_FREE(ia, KM_IFADDR);  */
				ia = 0;
				in_interfaces--;
				goto again;
			}
		    lia = ia;
		}
}
#endif INET

