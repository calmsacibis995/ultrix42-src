/* 
#ifndef lint
static  char    *sccsid = "@(#)defs.h	4.1  (ULTRIX)        7/2/90";
#endif lint
*/
/************************************************************************
 *									*
 *			Copyright (c) 1984,1988 by			*
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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	defs.h	5.3 (Berkeley) 5/30/86
 */

/************************************************************************
 *			Modification History				*
 *
 * 03/09/89	R. Bhanukitsiri
 *		Add protocol support so that we can listen to both
 *		RIP and Extensible SNMP Agent messages.
 *
 *									*
 ************************************************************************/

/*
 * Internal data structure definitions for
 * user routing process.  Based on Xerox NS
 * protocol specs with mods relevant to more
 * general addressing scheme.
 */
#include <sys/types.h>
#include <sys/socket.h>

#include <net/route.h>
#include <netinet/in.h>
#include <protocols/routed.h>

#include <stdio.h>
#include <netdb.h>

#include "trace.h"
#include "interface.h"
#include "table.h"
#include "af.h"

/*
 * When we find any interfaces marked down we rescan the
 * kernel every CHECK_INTERVAL seconds to see if they've
 * come up.
 */
#define	CHECK_INTERVAL	(1*60)

#ifdef	SNMP
/*
 * Define the interval for which SNMP registration message
 * will be sent.
 */
#define	SNMP_TIME	(10*TIMER_RATE)
#endif	SNMP

/*
 * ROUTED Supported protocols
 */
#ifndef IPPROTO_RIP
#define	IPPROTO_RIP	520
#endif	IPPROTO_RIP
#ifndef	IPPROTO_SNMP
#ifdef	SNMP
#define	IPPROTO_SNMP	163
#endif	SNMP
#endif	IPPROTO_SNMP

#define equal(a1, a2) \
	(bcmp((caddr_t)(a1), (caddr_t)(a2), sizeof (struct sockaddr)) == 0)
#define	min(a,b)	((a)>(b)?(b):(a))

struct	sockaddr_in addr;	/* address of daemon's socket */

int	s;			/* source and sink of all data */
int	kmem;
int	supplier;		/* process should supply updates */
int	install;		/* if 1 call kernel */
int	lookforinterfaces;	/* if 1 probe kernel for new up interfaces */
int	performnlist;		/* if 1 check if /vmunix has changed */
int	externalinterfaces;	/* # of remote and local interfaces */
int	timeval;		/* local idea of time */

char	packet[MAXPACKETSIZE+1];
struct	rip *msg;

char	**argv0;
struct	servent *sp;		/* ptr to "route" service port */

extern	char *sys_errlist[];
extern	int errno;

struct	in_addr inet_makeaddr();
int	inet_addr();
char	*malloc();
int	exit();
int	sendmsg();
int	supply();
int	timer();
int	cleanup();
