/*
 * Copyright (c) 1988-1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * SCCSID: @(#)print-ip.c	4.1	ULTRIX	1/25/91
 * Based on:
 *rcsid[] = "@(#) $Header: print-ip.c,v 1.20 90/12/04 17:11:33 mccanne Exp $ (LBL)"
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>

#include "interface.h"
#include "addrtoname.h"

/*
 * print the recorded route in an IP RR, LSRR or SSRR option.
 */
static void
ip_printroute(type, cp, length)
	char *type;
	register u_char *cp;
	int length;
{
	int ptr = cp[2] - 1;
	int len;

	printf(" %s{", type);
	if ((length + 1) & 3)
		printf(" [bad length %d]", length);
	if (ptr < 3 || ((ptr + 1) & 3) || ptr > length + 1)
		printf(" [bad ptr %d]", cp[2]);

	type = "";
	for (len = 3; len < length; len += 4) {
		if (ptr == len)
			type = "#";
#ifdef ALIGN
		{
		struct in_addr addr;
		bcopy((char *)&cp[len], (char *)&addr, sizeof(addr));
		printf("%s%s", type, ipaddr_string(&addr));
		}
#else
		printf("%s%s", type, ipaddr_string(&cp[len]));
#endif
		type = " ";
	}
	printf("%s}", ptr == len? "#" : "");
}

/*
 * print IP options.
 */
static void
ip_optprint(cp, length)
	register u_char *cp;
	int length;
{
	int len;

	for (; length > 0; cp += len, length -= len) {
		switch (*cp) {
		case IPOPT_EOL:
			printf(" EOL");
			if (++len < length)
				printf("-%d", length - len);
			length = 0;
			break;
		case IPOPT_NOP:
			printf(" NOP");
			len = 1;
			break;
		case IPOPT_TS:
			len = cp[1];
			printf(" TS{%d}", len);
			break;
		case IPOPT_SECURITY:
			len = cp[1];
			printf(" SECURITY{%d}", len);
			break;
		case IPOPT_RR:
			len = cp[1];
			printf(" RR{%d}=", len);
			ip_printroute("RR", cp, len);
			break;
		case IPOPT_SSRR:
			len = cp[1];
			ip_printroute("SSRR", cp, len);
			break;
		case IPOPT_LSRR:
			len = cp[1];
			ip_printroute("LSRR", cp, len);
			break;
		default:
			len = cp[1];
			printf(" IPOPT-%d{%d}", cp[0], len);
			break;
		}
	}
}

/*
 * print an IP datagram.
 */
void
ip_print(ip, length)
	register struct ip *ip;
	register int length;
{
	register int hlen;
	register int len;
	register unsigned char *cp;

#ifdef ALIGN
	static u_char *abuf;
	/*
	 * The IP header is not word aligned, so copy into abuf.
	 * This will never happen with BPF.  Maybe with should #ifdef
	 * on that too. XXX
	 */
	if ((int)ip & (sizeof(long)-1)) {
		if (abuf == 0)
			abuf = (u_char *)malloc(snaplen);
		bcopy((char *)ip, (char *)abuf, min(length, snaplen));
		snapend += abuf - (u_char *)ip;
		packetp = abuf;
		ip = (struct ip *)abuf;
	}
#endif
	if (length < sizeof (struct ip)) {
		(void)printf("truncated-ip %d", length);
		return;
	}
	hlen = ip->ip_hl * 4;

	NTOHS(ip->ip_len);
	NTOHS(ip->ip_off);
	NTOHS(ip->ip_id);

	len = ip->ip_len - hlen;
	if (length < ip->ip_len)
		(void)printf("truncated-ip - %d bytes missing!",
			ip->ip_len - length);

	/*
	 * If this is fragment zero, hand it to the next higher
	 * level protocol.
	 */
	if ((ip->ip_off & 0x1fff) == 0) {
		cp = (unsigned char *)ip + hlen;
		switch (ip->ip_p) {

		case IPPROTO_TCP:
			tcp_print((struct tcphdr *)cp, len, ip);
			break;
		case IPPROTO_UDP:
			udp_print((struct udphdr *)cp, len, ip);
			break;
		case IPPROTO_ICMP:
			icmp_print((struct icmp *)cp, ip);
			break;
		case IPPROTO_ND:
			(void)printf("%s > %s:", ipaddr_string(&ip->ip_src),
				ipaddr_string(&ip->ip_dst));
			(void)printf(" nd %d", len);
			break;
		default:
			(void)printf("%s > %s:", ipaddr_string(&ip->ip_src),
				ipaddr_string(&ip->ip_dst));
			(void)printf(" ip-proto-%d %d", ip->ip_p, len);
			break;
		}
	}
	/*
	 * for fragmented datagrams, print id:size@offset.  On all
	 * but the last stick a "+".  For unfragmented datagrams, note
	 * the don't fragment flag.
	 */
	if (ip->ip_off & 0x3fff) {
		/*
		 * if this isn't the first frag, we're missing the
		 * next level protocol header.  print the ip addr.
		 */
		if (ip->ip_off & 0x1fff)
			(void)printf("%s > %s:", ipaddr_string(&ip->ip_src),
				      ipaddr_string(&ip->ip_dst));
		(void)printf(" (frag %x:%d@%d%s)", ip->ip_id, len,
			(ip->ip_off & 0x1fff) * 8,
			(ip->ip_off & IP_MF)? "+" : "");
	} else if (ip->ip_off & IP_DF)
		(void)printf(" (DF)");

	if (ip->ip_tos)
		(void)printf(" [tos 0x%x]", (int)ip->ip_tos);
	if (ip->ip_ttl <= 1)
		(void)printf(" [ttl %d]", (int)ip->ip_ttl);

	if (vflag) {
		char *sep = "";

		printf("(");
		if (ip->ip_ttl > 1) {
			(void)printf("%sttl %d", sep, (int)ip->ip_ttl);
			sep = ", ";
		}
		if ((ip->ip_off & 0x3fff) == 0) {
			(void)printf("%sid %d", sep, (int)ip->ip_id);
			sep = ", ";
		}
		if ((hlen -= sizeof(struct ip)) > 0) {
			(void)printf("%soptlen=%d", sep, hlen);
			ip_optprint((u_char *)(ip + 1), hlen);
		}
		printf(")");
	}
}
