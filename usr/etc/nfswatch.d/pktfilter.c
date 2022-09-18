#ifndef lint
static char *sccsid = "@(#)pktfilter.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/pktfilter.c,v 3.0 91/01/23 08:23:17 davy Exp $";
 */

/*
 * pktfilter.c - filters to count the packets.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	pktfilter.c,v $
 * Revision 3.0  91/01/23  08:23:17  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.3  91/01/04  15:56:25  davy
 * Bug fix from Jeff Mogul.
 * 
 * Revision 1.2  90/08/17  15:47:42  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:49  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#ifndef ultrix
#include <net/if_arp.h>
#endif /* ultrix */
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_var.h>
#include <netinet/udp_var.h>
#ifdef sun
#  define NND	1
#  include <sun/ndio.h>
#endif /* sun */
#include <stdio.h>

#include "nfswatch.h"
#include "externs.h"
#include "screen.h"

#ifdef ultrix
#include "ultrix.map.h"
#include "ipports.h"
#endif /* ultrix */

/*
 * Ethernet broadcast address.
 */
static	struct ether_addr ether_broadcast = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/*
 * pkt_filter - count a packet, and pass it off to the appropriate filter.
 */
void
pkt_filter(cp, length)
u_int length;
char *cp;
{
	struct ip *ip;
	struct ether_arp *arp;
	int packet[PACKETSIZE];
	register int bdcst, want;
	struct ether_header eheader;

	/*
	 * Count this packet in the network totals.
	 */
	int_pkt_total++;
	pkt_total++;

	/*
	 * Extract the ethernet header.
	 */
	(void) bcopy(cp, (char *) &eheader, sizeof(struct ether_header));
	(void) bcopy(cp + sizeof(struct ether_header), (char *) packet,
		(int) (length - sizeof(struct ether_header)));

	/*
	 * See if it's a broadcast packet, and count it if it is.
	 */
#ifdef	ultrix
	bdcst = !bcmp((char *) eheader.ether_dhost, (char *) &ether_broadcast,
			sizeof(struct ether_addr));
#else
	bdcst = !bcmp((char *) &eheader.ether_dhost, (char *) &ether_broadcast,
			sizeof(struct ether_addr));
#endif	ultrix

	if (bdcst) {
		pkt_counters[PKT_BROADCAST].pc_interval++;
		pkt_counters[PKT_BROADCAST].pc_total++;
	}

	/*
	 * Figure out what kind of packet it is, and pass
	 * it off to the appropriate filter.
	 */
	switch (ntohs(eheader.ether_type)) {
	case ETHERTYPE_IP:		/* IP packet			*/
		ip = (struct ip *) packet;
		want = want_packet(ip->ip_src.s_addr, ip->ip_dst.s_addr);

		/*
		 * If we want this packet, count it in the host
		 * totals and pass it off.
		 */
		if (bdcst || want) {
			int_dst_pkt_total++;
			dst_pkt_total++;

			ip_filter(ip, length, ip->ip_src.s_addr,
				ip->ip_dst.s_addr);
		}

		break;
	case ETHERTYPE_ARP:		/* Address Resolution Protocol	*/
		arp = (struct ether_arp *) packet;
		want = want_packet(arp->arp_spa, arp->arp_tpa);

		/*
		 * If we want this packet, count it in the host
		 * totals and then count it in the packet
		 * type counters.
		 */
		if (bdcst || want) {
			int_dst_pkt_total++;
			dst_pkt_total++;

			pkt_counters[PKT_ARP].pc_interval++;
			pkt_counters[PKT_ARP].pc_total++;
		}

		break;
	case ETHERTYPE_REVARP:		/* Reverse Addr Resol Protocol	*/
		arp = (struct ether_arp *) packet;
		want = want_packet(arp->arp_spa, arp->arp_tpa);

		/*
		 * If we want this packet, count it in the host
		 * totals and then count it in the packet
		 * type counters.
		 */
		if (bdcst || want) {
			int_dst_pkt_total++;
			dst_pkt_total++;

			pkt_counters[PKT_RARP].pc_interval++;
			pkt_counters[PKT_RARP].pc_total++;
		}

		break;
#ifdef notdef
	case ETHERTYPE_PUP:		/* Xerox PUP			*/
#endif /* notdef */
	default:			/* who knows...			*/
		int_dst_pkt_total++;
		dst_pkt_total++;

		pkt_counters[PKT_OTHER].pc_interval++;
		pkt_counters[PKT_OTHER].pc_total++;
		break;
	}
}

/*
 * ip_filter - strip off the IP header and pass off to the appropriate
 *	       filter.
 */
void
ip_filter(ip, length, src, dst)
register struct ip *ip;
u_long src, dst;
u_int length;
{
	register int *data;
	register int datalength;

	data = (int *) ip;
	data += ip->ip_hl;
	datalength = ntohs(ip->ip_len) - (4 * ip->ip_hl);

	/*
	 * Figure out what kind of IP packet this is, and
	 * pass it off to the appropriate filter.
	 */
	switch (ip->ip_p) {
	case IPPROTO_TCP:		/* transmission control protocol*/
		tcp_filter((struct tcphdr *) data, datalength,
			src, dst);
		break;
	case IPPROTO_UDP:		/* user datagram protocol	*/
		udp_filter((struct udphdr *) data, datalength,
			src, dst);
		break;
	case IPPROTO_ND:		/* Sun Network Disk protocol	*/
		nd_filter((char *) data, datalength, src, dst);
		break;
	case IPPROTO_ICMP:		/* control message protocol	*/
		icmp_filter((struct icmp *) data, datalength,
			src, dst);
		break;
#ifdef notdef
	case IPPROTO_IGMP:		/* group message protocol	*/
	case IPPROTO_GGP:		/* gateway-gateway protocol	*/
	case IPPROTO_EGP:		/* exterior gateway protocol	*/
	case IPPROTO_PUP:		/* Xerox pup protocol		*/
	case IPPROTO_IDP:		/* XNS IDP			*/
#endif /* notdef */
	default:			/* who knows...			*/
		break;
	}
}

/*
 * tcp_filter - count TCP packets.
 */
void
tcp_filter(tcp, length, src, dst)
register struct tcphdr *tcp;
u_long src, dst;
u_int length;
{
	/*
	 * Just count the packet.
	 */
	pkt_counters[PKT_TCP].pc_interval++;
	pkt_counters[PKT_TCP].pc_total++;
}

/*
 * udp_filter - count UDP packets, pass RPC packets to the RPC filter.
 */
void
udp_filter(udp, length, src, dst)
register struct udphdr *udp;
u_long src, dst;
u_int length;
{
	/*
	 * Count as a UDP packet.
	 */
	pkt_counters[PKT_UDP].pc_interval++;
	pkt_counters[PKT_UDP].pc_total++;

	/*
	 * See what type of packet it is.  Pass off
	 * anything we don't recognize to the RPC
	 * filter.
	 */
	switch (ntohs(udp->uh_sport)) {
	case IPPORT_ROUTESERVER:	/* routing control protocol	*/
		pkt_counters[PKT_ROUTING].pc_interval++;
		pkt_counters[PKT_ROUTING].pc_total++;
		break;
#ifdef notdef
					/* network standard functions	*/
	case IPPORT_ECHO:		/* packet echo server		*/
	case IPPORT_DISCARD:		/* packet discard server	*/
	case IPPORT_SYSTAT:		/* system stats			*/
	case IPPORT_DAYTIME:		/* time of day server		*/
	case IPPORT_NETSTAT:		/* network stats		*/
	case IPPORT_FTP:		/* file transfer		*/
	case IPPORT_TELNET:		/* remote terminal service	*/
	case IPPORT_SMTP:		/* simple mail transfer protocol*/
	case IPPORT_TIMESERVER:		/* network time synchronization	*/
	case IPPORT_NAMESERVER:		/* domain name lookup		*/
	case IPPORT_WHOIS:		/* white pages			*/
	case IPPORT_MTP:		/* ???				*/
					/* host specific functions	*/
	case IPPORT_TFTP:		/* trivial file transfer	*/
	case IPPORT_RJE:		/* remote job entry		*/
	case IPPORT_FINGER:		/* finger			*/
	case IPPORT_TTYLINK:		/* ???				*/
	case IPPORT_SUPDUP:		/* SUPDUP			*/
					/* UNIX TCP services		*/
	case IPPORT_EXECSERVER:		/* rsh				*/
	case IPPORT_LOGINSERVER:	/* rlogin			*/
	case IPPORT_CMDSERVER:		/* rcmd				*/
					/* UNIX UDP services		*/
	/* case IPPORT_BIFFUDP:		/* biff mail notification	*/
	/* case IPPORT_WHOSERVER:	/* rwho				*/
#endif /* notdef */
	default:			/* might be an RPC packet	*/
		rpc_filter((char *) udp + sizeof(struct udphdr),
			ntohs(udp->uh_ulen) - sizeof(struct udphdr),
			src, dst);
		break;
	}
}

/*
 * nd_filter - count Sun ND packets.
 */
void
nd_filter(data, length, src, dst)
u_long src, dst;
u_int length;
char *data;
{
#ifdef sun
	register struct ndpack *nd;

	nd = (struct ndpack *) (data - sizeof(struct ip));

	/*
	 * Figure out whether it's a read or a write.
	 */
	switch (nd->np_op & NDOPCODE) {
	case NDOPREAD:
		pkt_counters[PKT_NDREAD].pc_interval++;
		pkt_counters[PKT_NDREAD].pc_total++;
		break;
	case NDOPWRITE:
		pkt_counters[PKT_NDWRITE].pc_interval++;
		pkt_counters[PKT_NDWRITE].pc_total++;
		break;
	case NDOPERROR:
	default:
		pkt_counters[PKT_OTHER].pc_interval++;
		pkt_counters[PKT_OTHER].pc_total++;
		break;
	}
#else /* sun */
	pkt_counters[PKT_OTHER].pc_interval++;
	pkt_counters[PKT_OTHER].pc_total++;
#endif /* sun */
}

/*
 * icmp_filter - count ICMP packets.
 */
void icmp_filter(icp, length, src, dst)
register struct icmp *icp;
u_long src, dst;
u_int length;
{
	pkt_counters[PKT_ICMP].pc_interval++;
	pkt_counters[PKT_ICMP].pc_total++;
}
