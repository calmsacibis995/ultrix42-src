#ifndef lint
static	char	*sccsid = "@(#)route.c	4.2		(ULTRIX)		7/17/90";
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
/*
 *	05-Aug-89	jsd
 *	removed rtpurge cprintf debug
 *
 *	27-Mar-89	U. Sinkewicz
 *	Added lp changes to routing from 3/16/89.  Put lk_rtentry
 *	within macros RTLOCK() and RTFREE().
 *
 *	28-Feb-89	U. Sinkewicz
 *	SMP/mips merge.  Added changes from R. Bhanukitsiri 2/5/89.
 *
 *	25-Aug-88	U. Sinkewicz
 *	Added locks to routing tables.  Lock is called lk_rtentry and it
 *	protects access to rthost, rtnet, ipforward.	Note that lk_rtentry
 *	is also set in netinet to protect the routing table when it's being
 * 	manipulated by the protocols.
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
 *	@(#)route.c	6.10 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../net/net/if.h"
#include "../net/net/af.h"
#include "../net/net/route.h"
#include "../h/kmalloc.h"

int	rttrash;		/* routes not in table but not freed */
struct	sockaddr wildcard;	/* zero valued cookie for wildcard searches */
int	rthashsize = RTHASHSIZ;	/* for netstat, etc. */

/*
 * Packet routing routines.
 */
rtalloc(ro)
	register struct route *ro;
{
	register struct rtentry *rt;
	register u_long hash;
	struct sockaddr *dst = &ro->ro_dst;
	int (*match)(), doinghost, s;
	struct afhash h;
	u_int af = dst->sa_family;
	struct rtentry **table;

	if (ro->ro_rt && ro->ro_rt->rt_ifp && (ro->ro_rt->rt_flags & RTF_UP)){
		return;	
	}			 /* XXX */
	if (af >= AF_MAX){
		return;
	}
	(*afswitch[af].af_hash)(dst, &h);
	match = afswitch[af].af_netmatch;
	hash = h.afh_hosthash, table = rthost, doinghost = 1;
	s = splnet();

again:
	for (rt = table[RTHASHMOD(hash)]; rt; rt = rt->rt_next) {
		if (rt->rt_hash != hash)
			continue;
		if ((rt->rt_flags & RTF_UP) == 0 ||
		    (rt->rt_ifp->if_flags & IFF_UP) == 0)
			continue;
		if (doinghost) {
			if (bcmp((caddr_t)&rt->rt_dst, (caddr_t)dst,
			    sizeof (*dst)))
				continue;
		} else {
			if (rt->rt_dst.sa_family != af ||
			    !(*match)(&rt->rt_dst, dst))
				continue;
		}
		rt->rt_refcnt++;
		if (dst == &wildcard)
			rtstat.rts_wildcard++;
		ro->ro_rt = rt;
		splx(s);
		return;
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash, table = rtnet;
		goto again;
	}
	/*
	 * Check for wildcard gateway, by convention network 0.
	 */
	if (dst != &wildcard) {
		dst = &wildcard, hash = 0;
		goto again;
	}
	rtstat.rts_unreach++;
	splx(s);
}

rtfree(rt)
	register struct rtentry *rt;
{
	if (rt == 0)
		panic("rtfree");
	rt->rt_refcnt--;
	if (rt->rt_refcnt == 0 && (rt->rt_flags&RTF_UP) == 0) {
		rttrash--;
		KM_FREE(rt, KM_RTABLE); 
	}
}

/*
 * Force a routing table entry to the specified
 * destination to go through the given gateway.
 * Normally called as a result of a routing redirect
 * message from the network layer.
 *
 * N.B.: must be called at splnet or higher
 * SMP.  Call with lk_rtentry asserted.  Note recursive call to
 * rtinit, therefore must unlock before calling rtinit.
 */
rtredirect(dst, gateway, flags, src)
	struct sockaddr *dst, *gateway, *src;
	int flags;
{
	struct route ro;
	register struct rtentry *rt;

	/* verify the gateway is directly reachable */
	if (ifa_ifwithnet(gateway) == 0) {
		rtstat.rts_badredirect++;
		return;
	}
	ro.ro_dst = *dst;
	ro.ro_rt = 0;
	
	RTLOCK();
	rtalloc(&ro);
	rt = ro.ro_rt;
#define equal(a1, a2) \
	(bcmp((caddr_t)(a1), (caddr_t)(a2), sizeof(struct sockaddr)) == 0)
	/*
	 * If the redirect isn't from our current router for this dst,
	 * it's either old or wrong. If it redirects us to ourselves,
 	 * we have a routing loop, perhaps as a result of an interface
	 * going down recently.
	 */
	if ((rt && !equal(src, &rt->rt_gateway)) || ifa_ifwithaddr(gateway)) {
		rtstat.rts_badredirect++;
		if (rt != 0)
			rtfree(rt);
		RTUNLOCK();
		return;
	}
	/*
	 * Create a new entry if we just got back a wildcard entry
	 * or the the lookup failed.  This is necessary for hosts
	 * which use routing redirects generated by smart gateways
	 * to dynamically build the routing tables.
	 */
	if (rt &&
	    (*afswitch[dst->sa_family].af_netmatch)(&wildcard, &rt->rt_dst)) {
		rtfree(rt);
		rt = 0;
	}
	if (rt == 0) {
		rtstat.rts_dynamic++;
		RTUNLOCK();
		rtinit(dst, gateway, (int)SIOCADDRT, 
			(flags & RTF_HOST) | RTF_GATEWAY | RTF_DYNAMIC);
		return;
	}
	/*
	 * Don't listen to the redirect if it's
	 * for a route to an interface. 
	 */
	if (rt->rt_flags & RTF_GATEWAY) {
		if (((rt->rt_flags & RTF_HOST) == 0) && (flags & RTF_HOST)) {
			/*
			 * Changing from route to net => route to host.
			 * Create new route, rather than smashing route to net.
			 */
			RTUNLOCK();
			rtinit(dst, gateway, (int)SIOCADDRT,
				 flags | RTF_DYNAMIC);
			RTLOCK();
			rtstat.rts_dynamic++;
		} else {
			/*
			 * Smash the current notion of the gateway to
			 * this destination.  This is probably not right,
			 * as it's conceivable a flurry of redirects could
			 * cause the gateway value to fluctuate wildly during
			 * dynamic routing reconfiguration.
			 */
			rt->rt_gateway = *gateway;
			rt->rt_flags |= RTF_MODIFIED;
			rtstat.rts_newgateway++;
		}
	} else
		rtstat.rts_badredirect++;
	rtfree(rt);
	RTUNLOCK();
}

/*
 * Routing table ioctl interface.
 */
rtioctl(cmd, data)
	int cmd;
	caddr_t data;
{
	int retval = 0;

	if (cmd != SIOCADDRT && cmd != SIOCDELRT)
		return (EINVAL);
	if (!suser())
		return (u.u_error);
	RTLOCK();
	retval = rtrequest(cmd, (struct rtentry *)data);
	RTUNLOCK();
	return(retval);
}

/*
 * Carry out a request to change the routing table.  Called by
 * interfaces at boot time to make their ``local routes'' known,
 * for ioctl's, and as the result of routing redirects.
 */
rtrequest(req, entry)
	int req;
	register struct rtentry *entry;
{
	register struct rtentry **mprev;
	struct rtentry **mfirst;
	
	register struct rtentry *rt;
	struct afhash h;
	int s, error = 0, (*match)();
	u_int af;
	u_long hash;
	struct ifaddr *ifa;
	struct ifaddr *ifa_ifwithdstaddr();

	af = entry->rt_dst.sa_family;
	if (af >= AF_MAX)
		return (EAFNOSUPPORT);
	(*afswitch[af].af_hash)(&entry->rt_dst, &h);

	if (entry->rt_flags & RTF_HOST) {
		hash = h.afh_hosthash;
		mprev = &rthost[RTHASHMOD(hash)];
	} else {
		hash = h.afh_nethash;
		mprev = &rtnet[RTHASHMOD(hash)];
	}
	match = afswitch[af].af_netmatch;
	s = splimp(); 
	for (mfirst = mprev; rt = *mprev; mprev = &rt->rt_next) {
		if (rt->rt_hash != hash)
			continue;
		if (entry->rt_flags & RTF_HOST) {
			if (!equal(&rt->rt_dst, &entry->rt_dst))
				continue;
		} else {
			if (rt->rt_dst.sa_family != entry->rt_dst.sa_family ||
			    (*match)(&rt->rt_dst, &entry->rt_dst) == 0)
				continue;
		}
		if (equal(&rt->rt_gateway, &entry->rt_gateway))
			break;
	}
	switch (req) {

	case SIOCDELRT:
		if (rt == 0) {
			error = ESRCH;
			goto bad;
		}
		*mprev = rt->rt_next;
		if (rt->rt_refcnt > 0) {
			rt->rt_flags &= ~RTF_UP;
			rttrash++;
			rt->rt_next = 0;
		} else
			KM_FREE(rt, KM_RTABLE); 
		break;

	case SIOCADDRT:
		if (rt) {
			error = EEXIST;
			goto bad;
		}
		
		if ((entry->rt_flags & RTF_GATEWAY) == 0) {
			/*
			 * If we are adding a route to an interface,
			 * and the interface is a pt to pt link
			 * we should search for the destination
			 * as our clue to the interface.  Otherwise
			 * we can use the local address.
			 */
			ifa = 0;
			if (entry->rt_flags & RTF_HOST) 
				ifa = ifa_ifwithdstaddr(&entry->rt_dst);
			if (ifa == 0)
				ifa = ifa_ifwithaddr(&entry->rt_gateway);
		} else {
			/*
			 * If we are adding a route to a remote net
			 * or host, the gateway may still be on the
			 * other end of a pt to pt link.
			 */
			ifa = ifa_ifwithdstaddr(&entry->rt_gateway);
		}
		if (ifa == 0) {
			ifa = ifa_ifwithnet(&entry->rt_gateway);
			if (ifa == 0) {
				error = ENETUNREACH;
				goto bad;
			}
		}
		KM_ALLOC(rt, struct rtentry *, sizeof(struct rtentry), KM_RTABLE, KM_NOWAIT);
		if(rt == NULL) {
			error = ENOBUFS;
			goto bad;	
		}
		rt->rt_next = *mfirst;
		*mfirst = rt;
		rt->rt_hash = hash;
		rt->rt_dst = entry->rt_dst;
		rt->rt_gateway = entry->rt_gateway;
		rt->rt_flags = RTF_UP |
		    (entry->rt_flags & (RTF_HOST|RTF_GATEWAY|RTF_DYNAMIC));
		rt->rt_refcnt = 0;
		rt->rt_use = 0;
		rt->rt_ifp = ifa->ifa_ifp;
		break;
	}
bad:
	splx(s);
	return (error);
}

/*
 * Set up a routing table entry, normally
 * for an interface.
 */
rtinit(dst, gateway, cmd, flags)
	struct sockaddr *dst, *gateway;
	int cmd, flags;
{
	struct rtentry route;

	bzero((caddr_t)&route, sizeof (route));
	route.rt_dst = *dst;
	route.rt_gateway = *gateway;
	route.rt_flags = flags;
	RTLOCK();
	(void) rtrequest(cmd, &route);
	RTUNLOCK();
}
rtpurge(ifp)
	struct ifnet *ifp;
{
	register struct rtentry *rt;
	int doinghost, s, i;
	struct rtentry **table;

	table = rthost, doinghost = 1;
	s = splnet();
	RTLOCK();
again:
	for (i = 0; i<rthashsize; i++) {
		if(table[i] == 0)
			continue;	
		rt = table[i];
		while(rt) {
			if(rt->rt_dst.sa_family == AF_INET) {
			/* cprintf("rtpurge %x %x\n", rt->rt_ifp, ifp); */
				if(rt->rt_ifp == ifp) {
					/* delete route */
					/* cprintf("delrt %x\n", rt); */
					rtrequest(SIOCDELRT, rt);
					rtfree(rt);
				}
			}
			rt = rt->rt_next;
		}

	}
	if (doinghost) {
		doinghost = 0;
		table = rtnet;
		goto again;
	}
	RTUNLOCK();
	splx(s);
}
