/*
#ifndef lint
static  char    *sccsid = "@(#)snmp.h	4.1  (ULTRIX)        7/2/90";
#endif lint
*/
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

/************************************************************************
 *			Modification History				*
 *
 * 03/09/89	R. Bhanukitsiri
 *		Initial Release.
 *									*
 ************************************************************************/

/*
 * SNMP Specific Definitions
 */
#define	LOCALHOST	"127.0.0.1"
#define	ERROR		-1

/*
 * ipRouteProto MIB Definitions (RFC 1066)
 */
#define	ROUTEPROTO_OTHER	1
#define	ROUTEPROTO_LOCAL	2
#define	ROUTEPROTO_NETMGMT	3
#define	ROUTEPROTO_RIP		8
/*
 * ipRoute extensions definitions
 */
#define	RT_VAR_SIZE		9	/* length of _ipRouteEntry_ipRoute* */
#define RT_SIZE			10	/*   object identifid */

struct sockaddr_in snmpaddr;
unsigned short agentport;			/* well-known SNMPD port */
