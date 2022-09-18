#ifndef lint
static	char	*sccsid = "@(#)db_glue.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1990 by			*
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
 * Copyright (c) 1986 Regents of the University of California
 *	All Rights Reserved
 * static char sccsid[] = "@(#)db_glue.c	4.2 (Berkeley) 1/14/89";
 */

/*
 * Modification History:
 *
 * 28-Feb-90	sue
 *	Increased timeout on second connect in icmp_ping() from
 *	2 to 30 seconds.
 *
 * 7-Feb-90	bbrown
 *	Added 4.8.2 and Kerberos
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <ctype.h>
#include <netdb.h>
#include <arpa/nameser.h>
#ifdef ULTRIXFUNC
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <resolv.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#endif ULTRIXFUNC
#include "ns.h"
#include "db.h"

#ifdef ULTRIXFUNC
extern struct protoent *getprotoent_local();
extern struct servent *getservent_local();
extern struct protoent *getprotobyname_local();
#define MAXPACKET       4096			/* max packet size */
extern int errno;
#endif ULTRIXFUNC

struct valuelist {
	struct valuelist *next, *prev;
	char	*name;
	char	*proto;
	short	port;
} *servicelist, *protolist;

buildservicelist()
{
	struct servent *sp;
	struct valuelist *slp;

#ifdef ULTRIXFUNC
	setservent_local(1);
	while (sp = getservent_local()) {
#else ULTRIXFUNC
	setservent(1);
	while (sp = getservent()) {
#endif ULTRIXFUNC
		slp = (struct valuelist *)malloc(sizeof(struct valuelist));
		slp->name = savestr(sp->s_name);
		slp->proto = savestr(sp->s_proto);
		slp->port = ntohs((u_short)sp->s_port);
		slp->next = servicelist;
		slp->prev = NULL;
		if (servicelist)
			servicelist->prev = slp;
		servicelist = slp;
	}
#ifdef ULTRIXFUNC
	endservent_local();
#else ULTRIXFUNC
	endservent();
#endif ULTRIXFUNC

}

buildprotolist()
{
	struct protoent *pp;
	struct valuelist *slp;

#ifdef ULTRIXFUNC
	setprotoent_local(1);
	while (pp = getprotoent_local()) {
#else ULTRIXFUNC
	setprotoent(1);
	while (pp = getprotoent()) {
#endif ULTRIXFUNC
		slp = (struct valuelist *)malloc(sizeof(struct valuelist));
		slp->name = savestr(pp->p_name);
		slp->port = pp->p_proto;
		slp->next = protolist;
		slp->prev = NULL;
		if (protolist)
			protolist->prev = slp;
		protolist = slp;
	}
#ifdef ULTRIXFUNC
	endprotoent_local();
#else ULTRIXFUNC
	endprotoent();
#endif ULTRIXFUNC
}

/*
 * Convert service name or (ascii) number to int.
 */
servicenumber(p)
	char *p;
{

	return (findservice(p, &servicelist));
}

/*
 * Convert protocol name or (ascii) number to int.
 */
protocolnumber(p)
	char *p;
{

	return (findservice(p, &protolist));
}

findservice(s, list)
	register char *s;
	register struct valuelist **list;
{
	register struct valuelist *lp = *list;
	int n;

	for (; lp != NULL; lp = lp->next)
		if (strcasecmp(lp->name, s) == 0) {
			if (lp != *list) {
				lp->prev->next = lp->next;
				if (lp->next)
					lp->next->prev = lp->prev;
				(*list)->prev = lp;
				lp->next = *list;
				*list = lp;
			}
			return(lp->port);
		}
	(void) sscanf(s, "%d", &n);
	if (n <= 0)
		n = -1;
	return(n);
}

struct servent *
cgetservbyport(port, proto)
	u_short port;
	char *proto;
{
	register struct valuelist **list = &servicelist;
	register struct valuelist *lp = *list;
	static struct servent serv;

	port = htons(port);
	for (; lp != NULL; lp = lp->next) {
		if (port != lp->port)
			continue;
		if (strcasecmp(lp->proto, proto) == 0) {
			if (lp != *list) {
				lp->prev->next = lp->next;
				if (lp->next)
					lp->next->prev = lp->prev;
				(*list)->prev = lp;
				lp->next = *list;
				*list = lp;
			}
			serv.s_name = lp->name;
			serv.s_port = htons((u_short)lp->port);
			serv.s_proto = lp->proto;
			return(&serv);
		}
	}
	return(0);
}

struct protoent *
cgetprotobynumber(proto)
	register int proto;
{

	register struct valuelist **list = &protolist;
	register struct valuelist *lp = *list;
	static struct protoent prot;

	for (; lp != NULL; lp = lp->next)
		if (lp->port == proto) {
			if (lp != *list) {
				lp->prev->next = lp->next;
				if (lp->next)
					lp->next->prev = lp->prev;
				(*list)->prev = lp;
				lp->next = *list;
				*list = lp;
			}
			prot.p_name = lp->name;
			prot.p_proto = lp->port;
			return(&prot);
		}
	return(0);
}

char *
protocolname(num)
	int num;
{
	static	char number[8];
	struct protoent *pp;

	pp = cgetprotobynumber(num);
	if(pp == 0)  {
		(void) sprintf(number, "%d", num);
		return(number);
	}
	return(pp->p_name);
}

char *
servicename(port, proto)
	u_short port;
	char *proto;
{
	static	char number[8];
	struct servent *ss;

	ss = cgetservbyport(htons(port), proto);
	if(ss == 0)  {
		(void) sprintf(number, "%d", port);
		return(number);
	}
	return(ss->s_name);
}

#ifdef ULTRIXFUNC

#define ICMP_PACK_SZ 64
/* icmp_ping
 *
 *	This routine was developed from ping.c source
 *
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 *
 */
icmp_ping(addr)
	struct in_addr	*addr;
{
	static int s;
	static int cc;
	static int ident;
	static icmp_init = 0;
	static struct sockaddr_in whereto;/* Who to ping */

	static u_char outpack[MAXPACKET];
	static u_char inpack[MAXPACKET];


	struct protoent *proto;

	struct timeval *tp = (struct timeval *) &outpack[8];
	struct timeval timeout;
	struct timezone tz;	/* leftover */

	
	struct ip *ip = (struct ip *) inpack;
	struct icmp *icp = (struct icmp *) outpack;

	struct sockaddr_in from;
	int fromlen = sizeof (from);

	int i;
	int size;
	int len = sizeof (inpack);
	int looper;

	int readfd, nfound;

	if(!icmp_init) {
		register u_char *datap = &outpack[8+sizeof(struct timeval)];

		bzero( (char *)&whereto, sizeof(struct sockaddr) );

		whereto.sin_family = AF_INET;

		ident = getpid() & 0xFFFF;

		if ((proto = getprotobyname_local("icmp")) == NULL) {
			syslog(LOG_ERR, "icmp_ping: protobyname error");
			return(-1);
		}

		if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
			syslog(LOG_ERR, "icmp_ping: socket error");
			return(-1);
		}

		if( s >= 32 ) {
			syslog(LOG_ERR, "icmp_ping: socket leak exists");
			close(s);
			return(-1);
		}	

		icp->icmp_type = ICMP_ECHO;
		icp->icmp_code = 0;
		icp->icmp_cksum = 0;
		icp->icmp_seq = 0;
		icp->icmp_id = ident;		/* ID */

		cc = ICMP_PACK_SZ;		/* skips ICMP portion */

		for( i=8; i < ICMP_PACK_SZ - 8; i++)
			*datap++ = i;

		icmp_init = 1;
	}

	whereto.sin_addr = *addr;

	icp->icmp_seq++;

	gettimeofday( tp, &tz );

	icp->icmp_cksum = 0;

	/* Compute ICMP checksum here */
	icp->icmp_cksum = in_cksum( icp, cc );

	for(looper = 0; looper < 10; looper++) {
		len = sizeof (inpack);

		i = sendto(s, outpack, cc, 0, &whereto,
				sizeof(struct sockaddr) );

		if( i < 0 || i != cc )  {
			syslog(LOG_ERR,"zoneref: sendto error: %d, %d", i, cc);
			return(-1);
		}

		readfd = 1<<s;
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;

		if((nfound = select(s+1, &readfd, (int *)NULL, (int *)NULL, &timeout))
			== 0) {

			i = sendto( s, outpack, cc, 0, &whereto,
				sizeof(struct sockaddr) );

			if( i < 0 || i != cc )  {
				syslog(LOG_ERR,"zoneref: sendto error: %d, %d", i, cc);
				return(-1);
			}
			readfd = 1<<s;
			timeout.tv_sec = 30;
			timeout.tv_usec = 0;

			if((nfound = select(s + 1, &readfd, (int *)NULL, (int *)NULL,
				&timeout)) == 0) {
#ifdef DEBUG
				if(debug)
					fprintf(ddt, "select failed in icmp_ping\n");
#endif DEBUG
				return(-1);
			}
		}

		errno = 0;
		if ( (size = recvfrom(s, inpack, len, 0, &from, &fromlen)) < 0) {
			if( errno == EINTR ) {
				if ( (size = recvfrom(s, inpack, len, 0,
						&from, &fromlen)) < 0) {
#ifdef DEBUG
					if(debug)
						fprintf(ddt, "recvfrom failed in icmp_ping, errno %d, size %d\n", errno, size);
#endif DEBUG
					return(-1);
				}
			} else {
#ifdef DEBUG
				if(debug)
					fprintf(ddt, "recvfrom failed in icmp_ping, errno %d, size %d\n", errno, size);

#endif DEBUG
				return(-1);
			}
		}

		len = ip->ip_hl << 2;
		if (size < len + ICMP_MINLEN) {
#ifdef DEBUG
			if(debug)
				fprintf(ddt, "len problem in icmp_ping, len %d, size %d\n", len, size);
#endif DEBUG
			return(-1);
		}

		icp = (struct icmp *)(inpack + len);

		if( icp->icmp_id == ident ) {
			break;			/* 'Twas our ECHO */
		}

		/* 'Twas not our ECHO */

#ifdef DEBUG
		if(debug)
			fprintf(ddt, "Was not our echo in icmp_ping, msg id %d, our id %d\n", (int)icp->icmp_id, (int)ident);
#endif DEBUG

	}

	if( icp->icmp_type != ICMP_ECHOREPLY ) { /*Can't get there from here*/
#ifdef DEBUG
		if(debug)
			fprintf(ddt, "Can't get there from here in icmp_ping, type %d\n", (int)icp->icmp_type);
#endif DEBUG
		return(-1);
	}

	return(0);				/* 'Twas our ECHO */

}



/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;
 	u_short odd_byte = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
               *(u_char *)(&odd_byte) = *(u_char *)w;
               sum += odd_byte;
	}


	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

#endif ULTRIXFUNC
