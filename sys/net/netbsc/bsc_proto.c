#ifndef lint
static	char	*sccsid = "@(#)bsc_proto.c	4.1 bsc_proto.c		7/2/90";
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
/* bsc_proto.c -  Based on ../netinet/in_proto.c 			*
 *	6/25/85 - U. Sinkewicz						*
 ************************************************************************/

#include "../h/param.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/mbuf.h"
#include "../net/if.h"
#include "../netbsc/bsc.h"

/*
 * BSC protocol;
 */
int	bsc_states();
int	bsc_input();
int	bsc_usrreq();
int	bsc_init();
int	bsc_ifioctl();
int	bsc_slowtimo();

extern	int raw_usrreq();

struct protosw bscnetsw[] = {
{ SOCK_DGRAM,	&bscdomain,	0,		PR_WANTRCVD,
  bsc_input,	0,		0,		0,
  bsc_usrreq,
  bsc_init,	0,		bsc_slowtimo,	0,
  0,		0,		bsc_ifioctl,
},
{ SOCK_STREAM,	&bscdomain,	0,		PR_WANTRCVD,
  bsc_input,	0,		0,		0,
  bsc_usrreq,
  bsc_init,	0,		bsc_slowtimo,	0,
  0,		0,		bsc_ifioctl,
},
};

/*
 *  The next structure fills information into a domain type structure but
 *  note that the last element, &bscnetsw[...] fills in struct domain dom_next
 *  and basically identifies how many elements are in the bscnetsw array.
 *  Dom_next may be internet for all we know.
 */
struct domain bscdomain =
{AF_BSC,"bsc",0,0,0,bscnetsw,&bscnetsw[sizeof(bscnetsw)/sizeof(bscnetsw[0])]};

