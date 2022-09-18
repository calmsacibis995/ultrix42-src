#ifndef lint
static char *sccsid = "@(#)lat_if.c	6.2	(ULTRIX)	1/28/88";
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
 *	Larry Cohen  -	09/16/85					*
 * 		protosw and ETHERTYPE changes caused by subnet routing  *
 *									*
 ************************************************************************/

/*	lat_if.c	0.0	11/26/84	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"

#include "../vax/cpu.h"
#include "../vax/mtpr.h"

extern struct ifqueue latintrq;

/*
 * Routines to handle protocol specific needs of interface drivers.
 */

/*
 *		l a t _ i f o u t p u t
 *
 * Perform protocol specific functions for message transmission.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	ifp		= Pointer to output device structure
 *	m		= Pointer to MBUF chain to be output
 *	sockdst		= Destination socket address
 *	type		= Location to return link level protocol type
 *	linkdst		= Location(s) to return link level destination address
 */
lat_ifoutput( ifp,m,sockdst,type,linkdst )
struct ifnet *ifp;
struct mbuf *m;
struct lataddr *sockdst;
int *type;
char *linkdst;
{
    bcopy(sockdst->lat_addr, linkdst, sizeof(sockdst->lat_addr));
    *type = ETHERTYPE_LAT;
}

/*
 *		l a t _ i f i n p u t
 *
 * Perform protocol specific functions for message reception.
 *
 * Returns:		Pointer to the (possibly modified) MBUF chain of
 *			the received message
 *			0 if allocation failed.
 *
 * Inputs:
 *	m		= Pointer to MBUF chain of received message
 *	ifp		= Pointer to input device structure
 *	inq		= Location to return pointer to protocol input queue
 *	eh		= Pointer to received Ethernet header
 */
struct mbuf *lat_ifinput( m,ifp,inq,eh )
struct mbuf *m;
struct ifnet *ifp;
struct ifqueue **inq;
struct ether_header *eh;
{
    struct mbuf *m0;
    register struct latrecv *recv;

    if (m0 = m_get(M_DONTWAIT, MT_DATA))
    {
	m0->m_next = m;
	recv = mtod(m0, struct latrecv *);
	recv->rcv_hdr = *eh;
	recv->rcv_ifp = ifp;
	schednetisr(NETISR_LAT);
	*inq = &latintrq;
	return (m0);
    }
    m_freem(m);
    return (0);
}

/*
 *		l a t _ i f i o c t l
 *
 * Perform protocol specific function for I/O control processing.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	ifp		= Pointer to device structure
 *	cmd		= I/O control command
 *	data		= Pointer to I/O control data
 */
lat_ifioctl( ifp,cmd,data )
struct ifnet *ifp;
int cmd;
caddr_t data;
{
}
