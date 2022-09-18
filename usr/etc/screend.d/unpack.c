#ifndef lint
static char *sccsid = "@(#)unpack.c	4.3	(ULTRIX)	4/30/91";
#endif
/*
 * unpack.c
 *
 * Take an IP datagram header stack and "unpack" it into our canonical
 * representation.
 *
 * Modification history:
 *
 * 21 December 1988	Jeffrey Mogul/DECWRL
 *	Created.
 *
 */
/*
 *               Copyright 1989, 1990 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-free
 * right and license under any changes, enhancements or extensions made to the
 * core functions of the software, including but not limited to those affording
 * compatibility with other hardware or software environments, but excluding
 * applications which incorporate this software.  Users further agree to use
 * their best efforts to return to Digital any such changes, enhancements or
 * extensions that they make and inform Digital of noteworthy uses of this
 * software.  Correspondence should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301  
 * 
 * Comments and bug reports may also be sent using electronic mail to:
 * 			screend-reports@decwrl.dec.com
 * 
 * 	>> This software may NOT be distributed to third parties. <<
 *   
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include "screentab.h"

#define	IPOFF_MASK	(0x1FFF)	/* should be in <netinet/in.h> */

extern int debug;

/*
 * NOTE:
 *	Before this packet left the kernel, ipintr() in netinet/ip_input.c
 *	had already converted ip_off, ip_len, and ip_id into HOST order.
 */

UnpackIP(bufp, buflen, unpp)
register char *bufp;
register int buflen;
register struct unpacked_hdrs *unpp;
{
	register struct ip *ipp;
	struct tcphdr *tcpp;
	struct udphdr *udpp;
	struct icmp *icmpp;
	register int remlen;
	register char *ipdata;

	remlen = buflen - sizeof(*ipp);
	if (remlen < 0) {
	    if (debug)
	    	fprintf(stderr, "Runt, len %d\n", buflen);
	    return(0);
	}
	ipp = (struct ip *)bufp;
	ipdata = &(bufp[ipp->ip_hl * sizeof(long)]);

	unpp->src.addr = ipp->ip_src;
	unpp->dst.addr = ipp->ip_dst;
	unpp->proto = ipp->ip_p;

	/*
	 * Sleazy kludge to avoid being hacked via source routes;
	 * lazy assumption that any packet with options might have
	 * a source route, and that if one cares about source routes
	 * then one cares about the destination, so pretending that
	 * the packet destination is 255.255.255.255 (broadcast) will
	 * be a conservative estimate.
	 */
	if (ipp->ip_hl != 5) {	/* IP header options present */
	    /* Should parse options and extract info from any source route */
	    unpp->dst.addr.s_addr = 0xFFFFFFFF;
	}

	if (ipp->ip_off & IPOFF_MASK) {
	    /* do we have to look for fragment leader */
	    switch (ipp->ip_p) {
	    case IPPROTO_UDP:
	    case IPPROTO_TCP:
	    case IPPROTO_ICMP:
		/*
		 * For these protocols, we parse into the "ip data"
		 * segment, so we need information out of fragment
		 * 0.
		 */
		return(FindFragLeader(ipp, unpp));
		 
	    default:
		/* Other protocols: all fragments have enough info */
		break;		/* drop through */
	    }
	}
	
	switch (ipp->ip_p) {
	case IPPROTO_UDP:
	    if (remlen < sizeof(*udpp)) {
		if (debug)
		    fprintf(stderr, "Runt UDP header (%d short)\n",
				sizeof(*udpp) - remlen);
		return(0);
	    }
	    udpp = (struct udphdr *)ipdata;
	    unpp->src.port = udpp->uh_sport;
	    unpp->dst.port = udpp->uh_dport;
	    break;
	case IPPROTO_TCP:
	    if (remlen < sizeof(*tcpp)) {
		if (debug)
		    fprintf(stderr, "Runt TCP header (%d short)\n",
			sizeof(*tcpp) - remlen);
		return(0);
	    }
	    tcpp = (struct tcphdr *)ipdata;
	    unpp->src.port = tcpp->th_sport;
	    unpp->dst.port = tcpp->th_dport;
	    break;
	/* known but not handled protocols: */
	case IPPROTO_ICMP:
	    if (remlen < sizeof(*icmpp)) {
		if (debug)
		    fprintf(stderr, "Runt ICMP header (%d short)\n",
			sizeof(*icmpp) - remlen);
		return(0);
	    }
	    icmpp = (struct icmp *)ipdata;
	    unpp->src.port = icmpp->icmp_type;
	    break;
	default:
	    break;
	}

	/* See if we expect more fragments */
	if (ipp->ip_off & IP_MF) {
	    /*
	     * This is the first of several fragments (we already
	     * know the offset is zero) so remember it if this is
	     * a protocol that needs the info.
	     */
	    switch (ipp->ip_p) {
	    case IPPROTO_UDP:
	    case IPPROTO_TCP:
	    case IPPROTO_ICMP:
		RecordFragLeader(ipp, unpp);
		break;
	    default:
		/* nothing worth saving for this protocol */
		break;
	    }
	}

	return(1);
}

PrintAnnotatedHdrs(ahp)
register struct annotated_hdrs *ahp;
{
	char *ProtoNumberToName();
	char *protoname;

	printf("[%s]", inet_ntoa(ahp->hdrs.src.addr));
	if (ahp->srcnote.net.s_addr) {
	    printf(" net %s", inet_ntoa(ahp->srcnote.net));
	}
	if (ahp->srcnote.subnet.s_addr) {
	    printf(" subnet %s", inet_ntoa(ahp->srcnote.subnet));
	}
	printf("->");
	printf("[%s]", inet_ntoa(ahp->hdrs.dst.addr));
	if (ahp->dstnote.net.s_addr) {
	    printf(" net %s", inet_ntoa(ahp->dstnote.net));
	}
	if (ahp->dstnote.subnet.s_addr) {
	    printf(" subnet %s", inet_ntoa(ahp->dstnote.subnet));
	}
	
	protoname = ProtoNumberToName(ahp->hdrs.proto);
	if (protoname) {
	    printf(" %s", protoname);
	}
	else
	    printf(" proto %d", ahp->hdrs.proto);

	switch (ahp->hdrs.proto) {
	case IPPROTO_UDP:
	case IPPROTO_TCP:
		printf(" (%d->%d)",
			ntohs(ahp->hdrs.src.port), ntohs(ahp->hdrs.dst.port));
		break;
	case IPPROTO_ICMP:
		printf(" (%d)", ahp->hdrs.src.port);
		break;
	default:
		break;
	}
}

PrintUnpackedHdrs(uhp)
register struct unpacked_hdrs *uhp;
{
	char *ProtoNumberToName();
	char *protoname;

	printf("[%s]", inet_ntoa(uhp->src.addr));
	printf("->");
	printf("[%s]", inet_ntoa(uhp->dst.addr));

	protoname = ProtoNumberToName(uhp->proto);
	if (protoname) {
	    printf(" %s", protoname);
	}
	else
	    printf(" proto %d", uhp->proto);

	switch (uhp->proto) {
	case IPPROTO_UDP:
	case IPPROTO_TCP:
		printf(" (%d->%d)",
			ntohs(uhp->src.port), ntohs(uhp->dst.port));
		break;
	case IPPROTO_ICMP:
		printf(" (%d)", uhp->src.port);
		break;
	default:
		break;
	}
}
