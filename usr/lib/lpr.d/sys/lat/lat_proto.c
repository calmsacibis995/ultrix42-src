#ifndef lint
static char *sccsid = "@(#)lat_proto.c	4.1.1.4	2/29/88";
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
 *	Chung Wong - 1/7/88						*
 *		Added LATPROTO_TRACE for the trace socket.		*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		protosw and ETHERTYPE changes caused by subnet routing  *
 *									*
 ************************************************************************/

/*	lat_proto.c	0.0	5/4/84	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/mbuf.h"

#include "../lat/lat.h"

/*
 * Definitions of protocols supported in the LAT domain.
 */
extern int lat_usrreq(),lat_init(),lat_slowtimo(),lat_ifoutput(),lat_ifinput(),
	    lat_ifioctl(), ltt_usrreq(), lat_ctloutput();

struct protosw latsw[] =
{
    { SOCK_DGRAM,     &latdomain,           LATPROTO_CTL,    PR_CONNREQUIRED,
      0,              0,                0,               lat_ctloutput,
      lat_usrreq,     lat_init,         0,               lat_slowtimo,
      0,              lat_ifoutput,     lat_ifinput,     lat_ifioctl
    },
    { SOCK_SEQPACKET, &latdomain,	LATPROTO_TRACE,	 PR_ATOMIC,
      0,	      0,                0,               0,
      ltt_usrreq,     0,                0,               0,
      0
    }
};

struct domain latdomain =
    { AF_LAT, "LAT", 0, 0, 0, 
     latsw, &latsw[sizeof(latsw)/sizeof(latsw[0])] };
