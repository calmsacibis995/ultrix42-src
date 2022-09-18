#ifndef lint
static char *sccsid = "@(#)ip_screen.c	4.1	(ULTRIX)	9/11/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *									*
 *	16 December 1988	Jeffrey Mogul/DECWRL			*
 *		Created.						*
 *									*
 ************************************************************************/

/*
 * IP Screening mechanism
 *	Basically, just an interface to protocol-independent
 *	stuff in net/gw_screen.c
 *
 */

#include "../h/param.h"
#include "../h/ioctl.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/systm.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../net/af.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/icmp_var.h"

void ip_forward();
void ip_gwbounce();

/*
 * This procedure is instead of ip_forward() from ipintr().
 */
ip_forwardscreen(ip, ifp)
	struct ip *ip;
	struct ifnet *ifp;
{
	gw_forwardscreen(ip, ifp, AF_INET, ip_forward, ip_gwbounce);
}

/*
 * Called from gateway forwarding code if the packet is rejected
 * and the sender should be notified.
 */
void
ip_gwbounce(pkt, ifp)
	struct mbuf *pkt;
	struct ifnet *ifp;
{
	struct in_addr dest;
		    
	dest.s_addr = 0;
	icmp_error(pkt, ICMP_UNREACH, ICMP_UNREACH_HOST, ifp, dest);
}
