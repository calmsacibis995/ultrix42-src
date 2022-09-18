#ifndef lint
static char *sccsid = "@(#)lat_mcast.c	4.1.1.3	2/29/88";
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

/*	lat_mcast.c	0.0	11/9/84	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"

extern struct ifqueue latintrq;
extern struct sclass *sclass[];
extern u_short mtimer;

/*
 * LAT directory service subroutines.
 */

/*
 *		l a t _ i n i t
 *
 * This routine is called during the system boot sequence to initialise the
 * LAT protocol module.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 */
lat_init()
{

    latintrq.ifq_maxlen = IFQ_MAXLEN;
    latmulticast();

}

/*
 *		l a t m u l t i c a s t
 *
 * Scan the network interface list and try to build and send LAT service
 * class directory messages for all supported service classes on all
 * broadcast interfaces.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 */
latmulticast()
{
    register struct ifnet *ifp = ifnet;
    register int i;

    while (ifp)
    {
	if ((ifp->if_flags & (IFF_BROADCAST|IFF_DYNPROTO|IFF_UP)) == (IFF_BROADCAST|IFF_DYNPROTO|IFF_UP))
	{
	    for (i = 1; i <= MAXCLASS; i++)
	    {
		if (sclass[i] && (sclass[i]->scl_state == LST_RUNNING))
		{
		    (*sclass[i]->scl_direct)(ifp);
		}
	    }
	}
	ifp = ifp->if_next;
    }
    timeout(latmulticast, 0, mtimer*hz);
}

