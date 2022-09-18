#ifndef lint
static  char    *sccsid = "@(#)snmp.c	4.1  (ULTRIX)        7/2/90";
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
 *  $Header: snmp.c,v 1.55 88/07/22 12:05:46 fedor Exp $
 *
 */

/************************************************************************
 *			Modification History				*
 *
 * 03/09/89	R. Bhanukitsiri
 *		Initial Release.
 *									*
 ************************************************************************/

#ifdef SNMP

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
#include <protocols/routed.h>
#include <syslog.h>

#include <protocols/snmp.h>

#include "table.h"
#include "snmp.h"
#include "trace.h"

extern int snmp_socket;
extern char Rt_Var[];
extern char Rt_Metric1[];
extern char Rt_Proto[];
extern char Rt_Age[];



/*
 *  Process an incoming request from SNMPD.  We trade speed for elegance here.
 */
snmpin(from, size, pkt)
struct sockaddr *from;
int size;
char *pkt;
{
	struct sockaddr_in *sin_from = (struct sockaddr_in *)from;
	struct sockaddr_in reqdst;
	struct rt_entry *grte;
	int  rspsize, codetype, reqmet, rttype;
	char *req = pkt;
	char agntrsppkt[SNMPSTRLEN];
	char how_learned[SNMPSTRLEN];
	char *rsp = agntrsppkt;

        TRACE_EXT("snmpin: AGENT packet type %d received from %s, ",
                        *req, inet_ntoa(sin_from->sin_addr));
        TRACE_EXT("Size: %d bytes\n", size);

	switch (*req) {
		case AGENT_REG:
		case AGENT_RSP:
		case AGENT_ERR:
			syslog(LOG_ERR, "snmpin: unexpected AGENT pkt type");
			TRACE_TRC("snmpin: unexpected AGENT pkt type\n");
			return;
		case AGENT_REQ:
			req += 2;		/* Point to start of OID */
        		*rsp++ = AGENT_RSP;
			rspsize = 1;
			if (bcmp(req, Rt_Var, RT_VAR_SIZE) == 0) {
				req += RT_VAR_SIZE;	/* Point to attribute tag */
				codetype = *req++;
				bzero((char *)&reqdst, sizeof(reqdst));
				reqdst.sin_family = AF_INET;
				bcopy(req, (char *)&reqdst.sin_addr.s_addr,
				      sizeof(u_long));
				/*
				 * Get routing table entry for the requested
				 * destination.
				 */
				grte = rtlookup(&reqdst);
				if (grte == NULL) {
					agntrsppkt[0] = AGENT_ERR;
					rspsize = 1;
					break;
				}
				TRACE_UPD("snmpin: Route %s ",
					   inet_ntoa(reqdst.sin_addr));
				/*
				 * Return requested attribute
				 */
				if (codetype == 10) {	/* _ipRouteAge */
					reqmet = grte->rt_timer;
					TRACE_UPD("age: %d\n", reqmet);
					*rsp++ = INT;
					*rsp++ = sizeof(int);
					rspsize += (sizeof(int) + 2);
					bcopy((char *)&reqmet, rsp, sizeof(int));
					break;
				}
				if (codetype == 9) {	/* _ipRouteProto */
					reqmet = ROUTEPROTO_RIP;
					TRACE_UPD("proto: %d\n", reqmet);
					*rsp++ = INT;
					*rsp++ = sizeof(int);
					rspsize += (sizeof(int) + 2);
					bcopy((char *)&reqmet, rsp, sizeof(int));
					break;
				}
				if (codetype == 3) {	/* _ipRouteMetric1 */
					*rsp++ = INT;
					*rsp++ = sizeof(int);
					rspsize += (sizeof(int) + 2);
					reqmet = grte->rt_metric;
					TRACE_UPD("metric: %d\n", reqmet);
					bcopy((char *)&reqmet, rsp, sizeof(int));
					break;
				}
			} /* if RT_VAR */
			else {
				agntrsppkt[0] = AGENT_ERR;
				rspsize = 1;
				break;
			}
		default:
			syslog(LOG_ERR, "snmpin: invalid AGENT pkt type");
			TRACE_EXT("snmpin: invalid AGENT pkt type\n");
			agntrsppkt[0] = AGENT_ERR;
			rspsize = 1;
			break;
	} /* switch */

	if (sendto(snmp_socket, agntrsppkt, rspsize, 0,
		  (struct sockaddr *)sin_from,
		  sizeof(struct sockaddr_in)) < 0) {
		syslog(LOG_ERR, "snmpin: sendto: %m");
		if (snmpdebug) perror("snmpin: sendto");
	}
        return;
}


/*
 * TITLE:	register_snmp_vars
 *
 * FUNCTIONAL DESCRIPTIONS:
 *	This routine registers all of our supported variables
 *	with SNMPD.  Our supported variables are defined in
 *	ext.c.  The protocol is:
 *
 *	byte 0:		agent register code
 *	byte 1:		length of OID to register
 *	byte 3:		OID to register
 *	 ...
 *	byte n:		length of OID to register
 *	byte n+1:	OID to register
 *	 ...
 *
 *	This message is sent to SNMPD via the well-known SNMP routing agent
 *	port (snmp-rt).
 */
register_snmp_vars()
{
	struct sockaddr_in dst;
	int asize;
 	char agntpkt[MAXPACKETSIZE];
	char *p = agntpkt;

	*p++ = AGENT_REG;
	asize = 1;

	/* Register _ipRouteMetric1 */
	*p++ = RT_SIZE;
	bcopy(Rt_Metric1, p, RT_SIZE);
	p += RT_SIZE;
	asize += (RT_SIZE + 1);

	/* Register _ipRouteAge */
	*p++ = RT_SIZE;
	bcopy(Rt_Age, p, RT_SIZE);
	p += RT_SIZE;
	asize += (RT_SIZE + 1);

	/* Register _ipRouteProto */
	*p++ = RT_SIZE;
	bcopy(Rt_Proto, p, RT_SIZE);
	p += RT_SIZE;
	asize += (RT_SIZE + 1);

	/*
	 * Send message to SNMPD to register our OIDs
	 */
        bzero((char *)&dst, sizeof(struct sockaddr_in));
        dst.sin_family = AF_INET;
        dst.sin_port = agentport;
        dst.sin_addr.s_addr = inet_addr(LOCALHOST);

        if (sendto(snmp_socket, agntpkt, asize, 0,
		  (struct sockaddr *)&dst, sizeof(struct sockaddr_in)) < 0) {
		syslog(LOG_ERR, "register_snmp_vars: sendto: %m");
		if (snmpdebug) perror("register_snmp_vars: sendto");
	}
}


/*
 * TITLE:	snmp_init
 *
 * FUNCTIONAL DESCRIPTIONS:
 *	This routine returns a socket for communicating with SNMPD.
 *	In addition, it returns the SNMP Routing Agent well-known port
 *	that SNMPD listens to.
 */

int snmp_init()
{
	struct servent *dap;
	int snmpinits;

	snmpaddr.sin_family = AF_INET;
	snmpaddr.sin_port = 0;
	snmpinits = get_snmp_socket(AF_INET, SOCK_DGRAM, &snmpaddr);
	if (snmpinits < 0) return(ERROR);

	dap = getservbyname("snmp-rt", "udp");
	if (dap == NULL) {
		syslog(LOG_NOTICE, "snmp_init: snmp-rt service not defined");
		if (snmpdebug) printf("snmp_init: snmp-rt service not defined\n");
		return(ERROR);
	}
	agentport = (u_short)dap->s_port;
        return(snmpinits);
}


/*
 * TITLE:	get_snmp_socket
 *
 * FUNCTIONAL DESCRIPTIONS:
 *	This routine returns a socket for communicating with SNMPD and
 *	bind us to it.
 */

int get_snmp_socket(domain, type, sin)
int domain, type;
struct sockaddr_in *sin;
{
	int snmpsocks, on = 1;

	if ((snmpsocks = socket(domain, type, 0)) < 0) {
		syslog(LOG_ERR,"get_snmp_socket: socket: %m");
		if (snmpdebug) perror("get_snmp_socket: socket");
		return (ERROR);
	}
#ifdef SO_RCVBUF
	on = 48*1024;
	if (setsockopt(snmpsocks, SOL_SOCKET, SO_RCVBUF,
		       (char *)&on, sizeof(on)) < 0) {
		syslog(LOG_ERR,"get_snmp_socket: setsockopt: %m");
		if (snmpdebug) perror("setsockopt SO_RCVBUF");
	}
#endif SO_RCVBUF
	if (bind(snmpsocks, sin, sizeof (*sin)) < 0) {
		syslog(LOG_ERR,"get_snmp_socket: bind: %m");
		if (snmpdebug) perror("get_snmp_socket: bind");
		(void) close(snmpsocks);
		return (ERROR);
	}
	return (snmpsocks);
}

#endif SNMP

