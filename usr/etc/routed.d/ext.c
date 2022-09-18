#ifndef lint
static  char    *sccsid = "@(#)ext.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 *   CENTER FOR THEORY AND SIMULATION IN SCIENCE AND ENGINEERING
 *			CORNELL UNIVERSITY
 *
 *      Portions of this software may fall under the following
 *      copyrights: 
 *
 *	Copyright (c) 1983 Regents of the University of California.
 *	All rights reserved.  The Berkeley software License Agreement
 *	specifies the terms and conditions for redistribution.
 *
 */

/*
 *  Copyright (c) NYSERNet Incorporated, 1988, All Rights Reserved
 *  The NYSERNet software License Agreement specifies the terms and
 *  conditions for redistribution.
 *
 *  $Header: ext.c,v 1.1 88/07/22 14:45:39 fedor Exp $
 *
 */

/************************************************************************
 *			Modification History				*
 *
 * 03/09/89	R. Bhanukitsiri
 *		Initial Release.
 *									*
 ************************************************************************/

#include <sys/types.h>
#include <sys/mbuf.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <strings.h>

#include <net/if.h>
#include <net/route.h>
#include <syslog.h>

#ifdef	AGENT_SNMP
#include <protocols/snmp.h>
#endif	AGENT_SNMP

#include <protocols/routed.h>

#ifdef SNMP

/*
 * SNMP related definitions
 */
int snmp_socket;			/* socket to communicate with snmpd */

/*
 * Routing variables in MIB RFC 1066 that can't be handled in snmpd
 */
char Rt_Var[] = {			/* _ipRouteEntry */
	0x01, 0x03, 0x06, 0x01, 0x02, 0x01, 0x04, 0x15, 0x01
};
char Rt_Metric1[] = {			/* _ipRouteEntry_ipRouteMetric1 */
	0x01, 0x03, 0x06, 0x01, 0x02, 0x01, 0x04, 0x15, 0x01, 0x03
};
char Rt_Proto[] = {			/* _ipRouteEntry_ipRouteProto */
	0x01, 0x03, 0x06, 0x01, 0x02, 0x01, 0x04, 0x15, 0x01, 0x09
};
char Rt_Age[] = {			/* _ipRouteEntry_ipRouteAge */
	0x01, 0x03, 0x06, 0x01, 0x02, 0x01, 0x04, 0x15, 0x01, 0xa
};

#endif SNMP

