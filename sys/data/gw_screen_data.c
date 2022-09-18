#ifndef lint
static char *sccsid = "@(#)gw_screen_data.c	4.2	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *                      Copyright (c) 1990 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *									*
 ************************************************************************/

/*
 *
 *   Modification history:
 *
 *   24 May 1990 - Jeffrey Mogul/DECWRL
 *      created file
 *
*/

#include "gwscreen.h"
#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/time.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/net/if.h"
#include "../net/net/af.h"
#include "../net/net/gw_screen.h"

#if	NGWSCREEN == 0

/* global data */
int gw_screen_on = SCREENMODE_OFF;		/* forever */

screen_init_freelist(dummy)
char *dummy;
{
}

screen_space_needed()
{
	return(0);
}

gw_forwardscreen(pkt, ifp, af, fwdproc, errorproc)
struct mbuf *pkt;
struct ifnet *ifp;
int af;
void (*fwdproc)();
void (*errorproc)();
{
	/* always forward */
	(*fwdproc)(pkt, ifp);
}

screen_control() {
	return(EOPNOTSUPP);
}

#else

/* SCREEN_MAXPEND can be changed if necessary, but probably shouldn't be */
#define	SCREEN_MAXPEND	32	/* default max # of packets pending */
int screen_maxpend = SCREEN_MAXPEND;

#endif	NGWSCREEN
