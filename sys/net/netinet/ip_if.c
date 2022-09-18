
#ifndef lint
static	char	*sccsid = "@(#)ip_if.c	4.1		(ULTRIX)	7/2/90";
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
 * routines to handle protocol specific needs of interface drivers.
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"

#include "../h/smp_lock.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/net/if.h"
#include "../net/net/route.h"

ip_ifoutput(ifp, m, sockdst, type, linkdst)

	struct ifnet *ifp;  	/* output device */
	struct mbuf *m;        /* output msg without link level encapsulation */
	struct sockaddr *sockdst;   /* destination socket address */
	int *type;		/* place link level data type here */
	char *linkdst;		/* place link level destination address here */
{
	return(0);
}



ip_ifinput(m, ifp, inq)
	struct mbuf *m;
	struct ifnet *ifp;
	struct ifqueue *inq;
{
	return(0);
}



ip_ifioctl(ifp, cmd, data)
	struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	return(0);
}
