#ifndef lint
static char *sccsid = "@(#)if.c	4.5    (ULTRIX)        3/7/91";
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
 *
 *      Michael G. Mc Menemy 07-Mar-91
 *              Fixed smp lock position messup.
 *
 *      Mary Walker 26-Nov-90
 *              Added length check to if_name matching.  (ex. "l" will no 
 *              longer match "ln")
 *
 *      Chran-Ham Chang - 9-Aug-90
 *		Added code to support SIOCEEUPDATE ioctl - FDDI EEPROM
 *		update and SIOCIFRSET ioctl - reset the interface
 *
 *	Fred Templin - 3-May-90
 *		Added "se/ln" compatibility lines.
 *
 *	Matt Thomas - 8-Nov-89
 *		Add support for interfaces with units > 9
 *
 *	Ursula Sinkewicz - 8-Nov-89
 *		Removed lk_ifnet.
 *
 *	Ursula Sinkewicz - 10-Aug-89
 *		Ifconf() had lk_ifnet set over a bcopy from kernel space
 *	to user space.  Fix is to copy from kernel space to a kmalloced
 *	buffer with a lock held, then to copy out to the uarea. 
 *
 *	Ursula Sinkewicz - 9-June-89
 *		Modifications for supporting asymmetric network drivers.
 *
 *	Ursula Sinkewicz - 28-Feb-89
 *		SMP/mips merge (Added R. Bhanukitsiri changes 2/6/89).
 *
 *	JAW - 15-Feb-89
 *		change copyout's in ifconf to bcopy because 
 *		ioctl system call creates a stack buffer to 
 *		copy the data into.  Thus copyout is not needed.
 *
 *	Ursula Sinkewicz - 13-Feb-89
 *		Added lk_ifnet.
 *									*
 *	Larry Palmer - 15-Jan-88					*
 *		Final 43bsd version					*
 *									*
 *	Vince Wallace - Jul-23-1987					*
 *		Add code for ioctl SIOCARPREQ - to broadcast an		*
 *		arp request packet.					*
 *									*
 *	Marc Teitelbaum - May 5 1987
 *		Add suser() check in ifioctl for setting
 *		hardware address, multicast address, zeroing counters...
 *
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routine		*
 *									*
 *	Robin Lewis  -  05/05/86					*
 *		Added check for super user in ioctl to set flags	*
 ************************************************************************/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if.c	6.7 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/dir.h"
#include "../h/kmalloc.h"

#include "../h/cpudata.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../net/net/if.h"
#include "../net/net/if_llc.h"
#include "../net/net/af.h"
#include "../h/smp_lock.h"

#include "../net/netinet/in.h"
#include "../net/netinet/if_ether.h"
#include "../net/net/gw_screen.h"

#include "ether.h"
#include "fddi.h"

int	ifqmaxlen = IFQ_MAXLEN;

char	*prcrequests[] = PRC_REQLIST;

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */

ifinit()
{
	register struct ifnet *ifp;
	int saveaffinity;
	int s;


	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_init) {
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
			(*ifp->if_init)(ifp->if_unit);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			s = splnet();
			if (ifp->if_snd.ifq_maxlen == 0)
				ifp->if_snd.ifq_maxlen = ifqmaxlen;
			splx(s);
		}
	if_slowtimo();
}

/*
 * Call each interface on a Unibus reset.
 */
ifubareset(uban)
	int uban;
{
	register struct ifnet *ifp;
	int saveaffinity;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_reset){
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
			(*ifp->if_reset)(ifp->if_unit, uban);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		}
}

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
if_attach(ifp)
	struct ifnet *ifp;
{
	register struct ifnet **p;
	int s;

	s = splnet();
	smp_lock(&lk_ifnet, LK_RETRY);
	p = &ifnet;

	while (*p)
		p = &((*p)->if_next);
	*p = ifp;
	smp_unlock(&lk_ifnet);
	splx(s);
}

/* 
 * Detach an interface out of the list of "active" interfaces.
 * Notify higher layers that this interface is gone.
 */
if_detach(ifp)
	struct ifnet *ifp;
{
	register struct ifnet **p;
	int s;

	s = splnet();
	smp_lock(&lk_ifnet, LK_RETRY);

	p = &ifnet;
	while(*p && (*p)->if_next != ifp)
		p = &((*p)->if_next);

	if(*p && (*p)->if_next) {			
		((*p)->if_next)->if_addr.sa_family = 0;
		((*p)->if_next)->if_mtu = 0;
		((*p)->if_next)->if_flags = 0;
		((*p)->if_next)->if_ioctl = NULL;
		((*p)->if_next)->if_output = NULL;
	}	
	smp_unlock(&lk_ifnet);
	splx(s);
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register struct ifaddr *lifa = 0;

#define	equal(a1, a2) \
	(bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if (equal(&ifa->ifa_addr, addr))
			lifa = ifa;
		if ((ifp->if_flags & IFF_BROADCAST) &&
		    equal(&ifa->ifa_broadaddr, addr))
			lifa = ifa;
	}
	if(lifa)
		return(lifa);
	return ((struct ifaddr *)0);
}

/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) 
	    if (ifp->if_flags & IFF_POINTOPOINT)
		for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr.sa_family != addr->sa_family)
				continue;
			if (equal(&ifa->ifa_dstaddr, addr)){
				return (ifa);
			}
	}
	return ((struct ifaddr *)0);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is first found.
 */
struct ifaddr *
ifa_ifwithnet(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register u_int af = addr->sa_family;
	register int (*netmatch)();

	if (af >= AF_MAX)
		return (0);
	netmatch = afswitch[af].af_netmatch;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if ((*netmatch)(&ifa->ifa_addr, addr)){
			return (ifa);
		}
	}
	return ((struct ifaddr *)0);
}

#ifdef notdef
/*
 * Find an interface using a specific address family
 */
struct ifaddr *
ifa_ifwithaf(af)
	register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr.sa_family == af){
			return (ifa);
		}
	return ((struct ifaddr *)0);
}
#endif

/*
 * Mark an interface down and notify protocols of
 * the transition. Change routes.
 * NOTE: must be called at splnet or eqivalent.
 */
if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		pfctlinput(PRC_ROUTEDEAD, (caddr_t)&ifa->ifa_addr);
	}
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
if_slowtimo()
{
	register struct ifnet *ifp;
	int saveaffinity;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog){
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
			(*ifp->if_watchdog)(ifp->if_unit);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		}
	}
	timeout(if_slowtimo, (caddr_t)0, hz / IFNET_SLOWHZ);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(name)
	register char *name;
{
	register char *cp, *cp2;
	register struct ifnet *ifp;
	int unit;

	for (cp = name; cp < name + IFNAMSIZ && *cp; cp++)
		if (*cp >= '0' && *cp <= '9')
			break;
	if (*cp == '\0' || cp == name + IFNAMSIZ)
		return ((struct ifnet *)0);
	for (unit = 0, cp2 = cp; cp2 < name + IFNAMSIZ && *cp2; cp2++)
		unit = unit * 10 + (*cp2 - '0');
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
	        if ((unsigned)(cp - name) != strlen(ifp->if_name))
		        continue;
		/*
		 * Following lines for se/ln compatibility
		 */
		if (((unsigned)(cp - name) == 2) && (!bcmp(name, "se", 2))) {
			if (bcmp(ifp->if_name, "ln", 2))
				continue;
		} else {
			if (bcmp(ifp->if_name, name, (unsigned)(cp - name)))
				continue;
		}
		if (unit == ifp->if_unit)
			break;
	}
	return (ifp);
}

/*
 *  From an ifp, generate the name of an interface.
 */
ifname(ifp, name)
    struct ifnet *ifp;
    char *name;
{
    register int i = 0, unit = ifp->if_unit;
    char digits[6], *p = ifp->if_name;

    /*
     *  Copy the interfaces base name (assume everythings ok).
     */
    while (*p)
	*name++ = *p++;
    /*
     *  Calculate the unit's digits in reverse order.
     */
    do {
	digits[i++] = (unit % 10) + '0';
	unit = unit/10;
    } while (unit);

    /*
     * Append the digits in reverse order.  This will
     * result in a correct number.
     */
    while (i--)
	*name++ = digits[i];
    *name = '\0';
}

/*
 * Interface ioctls.
 */
ifioctl(so, cmd, data)
	struct socket *so;
	int cmd;
	caddr_t data;
{
	register struct ifnet *ifp;
	register struct ifreq *ifr;
	int saveaffinity;
	int error;
	int s;
	int owner = 0; /* SMP */

	switch (cmd) {

	case SIOCSCREENON:
	case SIOCSCREEN:
	case SIOCSCREENSTATS:
		return(screen_control(so, cmd, data, (struct ifnet *)0));

	case SIOCGIFCONF:
		if (smp_owner(&so->lk_socket)){
			smp_unlock(&so->lk_socket);
			owner=1;
		}
		error = ifconf(cmd,data);
		if (owner) {
			SO_LOCK(so);
		}
		return (error);

#if defined(INET) && ( NETHER > 0 || NFDDI > 0)
	case SIOCSARP:
	case SIOCDARP:
		if (!suser())
			return (u.u_error);
		/* FALL THROUGH */
	case SIOCGARP:
		return (arpioctl(cmd, data));
#endif
	}
	ifr = (struct ifreq *)data;
	ifp = ifunit(ifr->ifr_name);
	if (ifp == 0)
		return (ENXIO);
	switch (cmd) {

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCSIFFLAGS:
		if (!suser())
			return (u.u_error);
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			s = splnet();
			/*
			 * must obey lock ordering
			 */
			if (smp_owner(&so->lk_socket)){
			  so->ref = 32;
			  owner = 1;
			  smp_unlock(&so->lk_socket);
			}
			if_down(ifp);
			if (owner){
			  smp_lock(&so->lk_socket,LK_RETRY);
			  so->ref = 0;
			  owner = 0;
			}
			splx(s);
		}
		if ((ifr->ifr_flags & IFF_802HDR) &&
		    (ifp->if_type == IFT_ETHER)) {
			s = splnet();
			ifp->if_type = IFT_ISO88023;
			ifp->if_flags &= (IFF_NOTRAILERS|IFF_802HDR);
			ifp->if_mtu = ETHERMTU - sizeof (struct llc);
			splx(s);
		}
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		/* Note that we don't do an affinity flag check here as
		 * an optimization where if set, we could go to the driver 
		 * without unlocking.  The reason we do not do that here, 
		 * is that so far, the network drivers can sleep for DECnet. 
		 * When we eleminate sleeps in the drivers, we can delete the
		 * owner check and unlocks here.
		 */
		if (smp_owner(&so->lk_socket)){
			so->ref = 8;
			owner = 1;
			smp_unlock(&so->lk_socket);
		}
	        if (ifp->if_ioctl){
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
			(void) (*ifp->if_ioctl)(ifp, cmd, data);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		}
		if (owner){
			smp_lock(&so->lk_socket,LK_RETRY);
			so->ref = 0;
			owner = 0;
		}
		break;
	case SIOCSIFMETRIC:
		if (!suser())
			return (u.u_error);
		ifp->if_metric = ifr->ifr_metric;
		break;
        case SIOCENABLBACK:
        case SIOCDISABLBACK:
	case SIOCSPHYSADDR: 
	case SIOCDELMULTI: 
	case SIOCADDMULTI: 
	case SIOCRDZCTRS:
	case SIOCIFRESET:
	case SIOCEEUPDATE:
		if(!suser())
			return(u.u_error);
		/* FALL THROUGH */
	case SIOCRDCTRS:
        case SIOCRPHYSADDR: 
		if (ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		if (smp_owner(&so->lk_socket)){
			owner = 1;
			so->ref = 8;
			smp_unlock(&so->lk_socket);
			}
		CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
		error = (*ifp->if_ioctl)(ifp, cmd, data);
		RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		if (owner){
			smp_lock(&so->lk_socket,LK_RETRY);
			so->ref = 0;
			owner = 0;
			}
		return(error);
	case SIOCARPREQ:
		if (suser())
			if (nINET == 1)
				return (arpwhohas(ifp,ifr->ifr_addr.sa_data));
			else
				return (ENETDOWN);
		else
			return (EACCES);


	/*
	 * protocol specific ioctls that must deal with the interface
	 * are handled in the default case by the protocol specific
	 * usrreq routine.
	 */
	default:  
		if (so->so_proto == 0)  {
			mprintf("ifioctl: no socket proto\n");
			return (EOPNOTSUPP);
		}
		return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
			cmd, data, ifp));
	}
	return (0);
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
ifconf(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp = ifnet;
	register struct ifaddr *ifa;
	struct ifreq ifr, *ifrp;
	struct ifreq *u_sptr, *u_tmp ;
	int space = ifc->ifc_len;
	int space_tmp = ifc->ifc_len;
	int s;
	int error;

	KM_ALLOC( u_sptr, struct ifreq *, space, KM_TEMP, KM_CLEAR);
	u_tmp = u_sptr;
	ifrp = ifc->ifc_req;

	for (; space > sizeof (ifr) && ifp; ifp = ifp->if_next) {
		ifname(ifp, ifr.ifr_name);
		if ((ifa = ifp->if_addrlist) == 0) {
			bzero((caddr_t)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			/* bcopy((caddr_t)&ifr, (caddr_t)ifrp, sizeof (ifr)); */
			bcopy((caddr_t)&ifr, (caddr_t)u_sptr, sizeof (ifr)); 
			space -= sizeof (ifr), u_sptr++;
		} else 
		    for ( ; space > sizeof (ifr) && ifa; ifa = ifa->ifa_next) {
			ifr.ifr_addr = ifa->ifa_addr;
			bcopy((caddr_t)&ifr, (caddr_t)u_sptr, sizeof (ifr));
			space -= sizeof (ifr), u_sptr++;
		}
	}

	error =copyout((caddr_t)u_tmp, (caddr_t)ifrp, ifc->ifc_len-space);
	KM_FREE(u_tmp, KM_TEMP);
	ifc->ifc_len -= space;

	return (error);
}

