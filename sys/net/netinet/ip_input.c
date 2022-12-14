#ifndef lint
static char *sccsid = "@(#)ip_input.c	4.4	(ULTRIX)	9/11/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/************************************************************************
 *			Modification History				*
 *
 *	31-Jul-90	jsd
 *		fixes for killer packets with IP options LSR and SRR
 *
 *	24-Jul-90	jaw
 *		submitted bug fix for locking problem in ipintr....
 *
 *	16-July-90	U. Sinkewicz
 *		Fix to locking around ip_reass().
 *
 *	7-Jul-90	lp
 *		FDDI performance changes.
 *
 *	14-11-89	Ursula Sinkewicz
 *		Removed lk_ifnet and lk_in_ifaddr to coincide with
 *		slip changes.
 *
 *	17-Oct-89	Uttam Shikarpur					*
 *		Added counter to track the number of bad options for    *
 *		network management. (ips_badoptions)		   	*
 *									*
 *	27-Mar-89	Ursula Sinkewicz
 *		Lowered ipl in lk_rtentry, lk_ifnet, lk_in_ifaddr, exchanged
 *		ip statistics locking for a macro, as per lp changes 
 *		made 3/16/89.
 *
 *	3-Mar-89	Ursula Sinkewicz
 *		Added support for new directory layout; added pmax
 *		support.
 *
 *	13-Feb-89	Ursula Sinkewicz
 *		SMP: Added lk_in_ifaddr and lk_ifnet.
 *	
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes. Use new memory allocation
 *	scheme for mbufs.
 *
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 *	Larry Cohen  -  01/17/86					*
 *		- will now accept broadcast of ones or zeros   		*
 *									*
 *	Marc Teitelbaum and Fred Templin - 08/21/86			*
 *		Added 4.3bsd beta tape enhancements. "ipintr()" now	*
 *		handles subet broadcasts and packet forwarding properly.*
 *		(Packets are only forwarded by hosts configured with	*
 *		"GATEWAY" option).					*
 ************************************************************************/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ip_input.c	7.7 (Berkeley) 10/31/87
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/smp_lock.h"

#include "../net/net/if.h"
#include "../net/net/route.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_pcb.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/ip_icmp.h"
#include "../net/netinet/tcp.h"

u_char	ip_protox[IPPROTO_MAX];
int	ipqmaxlen = IFQ_MAXLEN;
struct	in_ifaddr *in_ifaddr;			/* first inet address */
struct	lock_t	lk_ipstat;	/* SMP: ipstat lock, see ip_var.h */
struct	lock_t  lk_ipq;		/* SMP: ip reassembly queue lock */

/*
 * We need to save the IP options in case a protocol wants to respond
 * to an incoming packet over the same route if the packet got here
 * using IP source routing.  This allows connection establishment and
 * maintenance when the remote end is on a network that is not known
 * to us.
 */
int	ip_nhops = 0;
static	struct ip_srcrt {
	char	nop;				/* one NOP to align */
	char	srcopt[IPOPT_OFFSET + 1];	/* OPTVAL, OLEN and OFFSET */
	struct	in_addr route[MAX_IPOPTLEN];
} ip_srcrt;

extern struct lock_t lk_arptab;
/*
 * IP initialization: fill in IP protocol switch table.
 * All protocols not implemented in kernel go to raw IP protocol handler.
 */
ip_init()
{
	register struct protosw *pr;
	register int i;

	pr = pffindproto(PF_INET, IPPROTO_RAW, SOCK_RAW);
	if (pr == 0)
		panic("ip_init");
	for (i = 0; i < IPPROTO_MAX; i++)
		ip_protox[i] = pr - inetsw;
	for (pr = inetdomain.dom_protosw;
	    pr < inetdomain.dom_protoswNPROTOSW; pr++)
		if (pr->pr_domain->dom_family == PF_INET &&
		    pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
			ip_protox[pr->pr_protocol] = pr - inetsw;
	/* SMP: Initailize locks here - ipstat, ipq, and ipintrq.  4.22.87.us */
	lockinit(&lk_ipq, &lock_ipq_d);
	lockinit(&lk_ipstat, &lock_ipstat_d);
	ipq.next = ipq.prev = &ipq;
	ip_id = time.tv_sec & 0xffff;
	ipintrq.ifq_maxlen = ipqmaxlen;
	lockinit(&ipintrq.lk_ifqueue, &lock_ifqueue_d);
	lockinit(&lk_arptab, &lock_arptab_d);
}

u_char	ipcksum = 1;
struct	ip *ip_reass();
struct	sockaddr_in ipaddr = { AF_INET };
struct 	route ipforward_rt;

/*
 * Ip input routine.  Checksum and byte swap header.  If fragmented
 * try to reassamble.  If complete and fragment queue exists, discard.
 * Process options.  Pass to next level.
 */
ipintr()
{
	register struct ip *ip;
	register struct mbuf *m;
	struct mbuf *m0;
	register int i;
	register struct ipq *fp;
	register struct in_ifaddr *ia;
	struct ifnet *ifp;
	int hlen, s;
        /* 
         * This is to keep these long aligned
         */
        struct in_addr dst;
        struct in_addr src;

next:
	/*
	 * Get next datagram off input queue and get IP header
	 * in first mbuf.
	 */
	s = splimp();
	/* SMP: Lock element taken off ipintrq along with
	 * next/prev pointers in the queue.  4.22.87.us
	 */
	smp_lock(&ipintrq.lk_ifqueue, LK_RETRY);
	IF_DEQUEUEIF(&ipintrq, m, ifp);
	smp_unlock(&ipintrq.lk_ifqueue);
	splx(s);
	if (m == 0)
		return;
	/*
	 * If no IP addresses have been set yet but the interfaces
	 * are receiving, can't do anything with incoming packets yet.
 	 */
	if (in_ifaddr == NULL){
		goto bad;
	}
	IPSTAT(ips_total++);
	if ((m->m_off > MMAXOFF || m->m_len < sizeof (struct ip)) &&
	    (m = m_pullup(m, sizeof (struct ip))) == 0) {
		IPSTAT(ips_toosmall++);
		goto next;
	}
	ip = mtod(m, struct ip *);
	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip)) {	/* minimum header length */
		IPSTAT(ips_badhlen++);
		goto bad;
	}
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == 0) {
			IPSTAT(ips_badhlen++);
			goto next;
		}
		ip = mtod(m, struct ip *);
	}
	if (ipcksum)
		if (ip->ip_sum = in_cksum(m, hlen)) {
			IPSTAT(ips_badsum++);
			goto bad;
		}

	/*
	 * Convert fields to host representation.
	 */
	ip->ip_len = ntohs((u_short)ip->ip_len);
	if (ip->ip_len < hlen) {
		IPSTAT(ips_badlen++);
		goto bad;
	}
	ip->ip_id = ntohs(ip->ip_id);
	ip->ip_off = ntohs((u_short)ip->ip_off);

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IP header would have us expect.
	 * Trim mbufs if longer than we expect.
	 * Drop packet if shorter than we expect.
	 */
	i = -(u_short)ip->ip_len;
	m0 = m;
	for (;;) {
		i += m->m_len;
		if (m->m_next == 0)
			break;
		m = m->m_next;
	}
	if (i != 0) {
		if (i < 0) {
			IPSTAT(ips_tooshort++);
			m = m0;
			goto bad;
		}
		if (i <= m->m_len)
			m->m_len -= i;
		else
			m_adj(m0, -i);
	}
	m = m0;

	/*
	 * Process options and, if not destined for us,
	 * ship it on.  ip_dooptions returns 1 when an
	 * error was detected (causing an icmp message
	 * to be sent and the original packet to be freed).
	 */
	ip_nhops = 0;		/* for source routed packets */
	if (hlen > sizeof (struct ip) && ip_dooptions(ip, ifp))
		goto next;

	/*
	 * Check our list of addresses, to see if the packet is for us.
	 */
        bcopy(&ip->ip_dst, &dst, sizeof(struct in_addr));
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
#define	satosin(sa)	((struct sockaddr_in *)(sa))

		if (IA_SIN(ia)->sin_addr.s_addr == dst.s_addr) {
			goto ours;
		}
		if (
#ifdef DIRECTED_BROADCAST
			ia->ia_ifp == ifp &&
#endif DIRECTED_BROADCAST
			(ia->ia_ifp->if_flags & IFF_BROADCAST)) {
		    u_long t;

		    if (satosin(&ia->ia_broadaddr)->sin_addr.s_addr ==
			dst.s_addr) {
			    goto ours;
		    }
		    if (dst.s_addr == ia->ia_netbroadcast.s_addr) {
			    goto ours;
		    }
		    /*
		     * Look for all-0's host part (old broadcast addr),
		     * either for subnet or net.
		     */
		    t = ntohl(dst.s_addr);
		    if (t == ia->ia_subnet) {
			    goto ours;
		    }
		    if (t == ia->ia_net) {
			    goto ours;
		    }
		}
	}
	if (dst.s_addr == (u_long)INADDR_BROADCAST)
		goto ours;
	if (dst.s_addr == INADDR_ANY)
		goto ours;

	/*
	 * Not for us; forward if possible and desirable.
	 */
	ip_forwardscreen(ip, ifp);
	goto next;

ours:
	/*
	 * Look for queue of fragments
	 * of this datagram.
	 */
	smp_lock(&lk_ipq, LK_RETRY);
	if(ipq.next != &ipq) {
        	bcopy(&ip->ip_src, &src, sizeof(struct in_addr));
		for (fp = ipq.next; fp != &ipq; fp = fp->next)
			if (ip->ip_id == fp->ipq_id &&
		    	src.s_addr == fp->ipq_src.s_addr &&
		    	dst.s_addr == fp->ipq_dst.s_addr &&
		    	ip->ip_p == fp->ipq_p) {
				goto found;
		    	}
	}
	fp = 0;
found:

	/*
	 * Adjust ip_len to not reflect header,
	 * set ip_mff if more fragments are expected,
	 * convert offset of this to bytes.
	 */
	ip->ip_len -= hlen;
	((struct ipasfrag *)ip)->ipf_mff = 0;
	if (ip->ip_off & IP_MF)
		((struct ipasfrag *)ip)->ipf_mff = 1;
	ip->ip_off <<= 3;

	/*
	 * If datagram marked as having more fragments
	 * or if this is not the first fragment,
	 * attempt reassembly; if it succeeds, proceed.  
	 */
	if (((struct ipasfrag *)ip)->ipf_mff || ip->ip_off) {
		IPSTAT(ips_fragments++);

#ifdef mips
                /*
                 * if the ip header is not aligned properly, copy the
                 * mbuf to avoid lots of bcopy's later when reassembling
                 * fragments due to placement of fragment pointers within
                 * ip header.
                   */
                if (((u_int)ip & 0x3) != 0) {
                        register struct mbuf *mnew;

                        mnew = m_copy(m, 0, m->m_len);
                        mnew->m_next = m_free(m);
                        m = mnew;
                        ip = mtod(m, struct ip *);
                }
#endif mips
		ip = ip_reass((struct ipasfrag *)ip, fp);
        	smp_unlock(&lk_ipq);
		if (ip == 0)
			goto next;
		m = dtom(ip);
	} else{
		if (fp) {
			ip_freef(fp);
		}
        	smp_unlock(&lk_ipq);
	}
	/*
	 * Switch out to protocol's input routine.
	 */
	(*inetsw[ip_protox[ip->ip_p]].pr_input)(m, ifp);
	goto next;
bad:
	m_freem(m);
	goto next;
}

/*
 * Take incoming datagram fragment and try to
 * reassemble it into whole datagram.  If a chain for
 * reassembly of this datagram already exists, then it
 * is given as fp; otherwise have to make a chain.
 */

struct ip *
ip_reass(ip, fp)
	register struct ipasfrag *ip;
	register struct ipq *fp;
{
	register struct mbuf *m = dtom(ip), *mm;
	register struct ipasfrag *q;
	int hlen = ip->ip_hl << 2;
	int i, next;

	/*
	 * Presence of header sizes in mbufs
	 * would confuse code below.
	 */
	m->m_off += hlen;
	m->m_len -= hlen;

	/*
	 * If first fragment to arrive, create a reassembly queue.
	 */
	if (fp == 0) {
		KM_ALLOC(fp, struct ipq *, sizeof(struct ipq),KM_FTABLE, KM_NOWAIT);
		if(fp == NULL)
			goto dropfrag;
		insque(fp, &ipq);
		fp->ipq_ttl = IPFRAGTTL;
		fp->ipq_p = ip->ip_p;
		fp->ipq_id = ip->ip_id;
		fp->ipq_next = fp->ipq_prev = (struct ipasfrag *)fp;
		fp->ipq_src = ((struct ip *)ip)->ip_src;
		fp->ipq_dst = ((struct ip *)ip)->ip_dst;
		q = (struct ipasfrag *)fp;
		goto insert;
	} 

	/*
	 * Find a segment which begins after this one does.
	 */
	/*
	 * This is a cheap append if everything is all set 
	 */
	if(fp->ipq_prev->ip_off + fp->ipq_prev->ip_len == ip->ip_off) {
		q = (struct ipasfrag *)fp;
		goto insert;
	}
	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = q->ipf_next)
		if (q->ip_off > ip->ip_off)
			break;

	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if (q->ipf_prev != (struct ipasfrag *)fp) {
		i = q->ipf_prev->ip_off + q->ipf_prev->ip_len - ip->ip_off;
		if (i > 0) {
			if (i >= ip->ip_len){
				goto dropfrag;
			}
			m_adj(dtom(ip), i);
			ip->ip_off += i;
			ip->ip_len -= i;
		}
	}

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	while (q != (struct ipasfrag *)fp && ip->ip_off + ip->ip_len > q->ip_off) {
		i = (ip->ip_off + ip->ip_len) - q->ip_off;
		if (i < q->ip_len) {
			q->ip_len -= i;
			q->ip_off += i;
			m_adj(dtom(q), i);
			break;
		}
		q = q->ipf_next;
		m_freem(dtom(q->ipf_prev));
		ip_deq(q->ipf_prev);
	}

insert:
	/*
	 * Stick new segment in its place;
	 * check for complete reassembly.
	 */
	ip_enq(ip, q->ipf_prev);
	next = 0;
	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = q->ipf_next) {
		if (q->ip_off != next) {
			return (0);
		}
		next += q->ip_len;
	}
	if (q->ipf_prev->ipf_mff) {
		return (0);
	}
	/*
	 * Reassembly is complete; concatenate fragments.
	 */
	q = fp->ipq_next;
	m = dtom(q);
	mm = m->m_next;
	m->m_next = 0;
	m_cat(m, mm);
	q = q->ipf_next;
	while (q != (struct ipasfrag *)fp) {
		mm = dtom(q);
		q = q->ipf_next;
		m_cat(m, mm);
	}

	/*
	 * Create header for new ip packet by
	 * modifying header of first packet;
	 * dequeue and discard fragment reassembly header.
	 * Make header visible.
	 */
	ip = fp->ipq_next;
	ip->ip_len = next;
	((struct ip *)ip)->ip_src = fp->ipq_src;
	((struct ip *)ip)->ip_dst = fp->ipq_dst;
	remque(fp);
	KM_FREE(fp, KM_FTABLE);
	m = dtom(ip);
	m->m_len += (ip->ip_hl << 2);
	m->m_off -= (ip->ip_hl << 2);
	return ((struct ip *)ip);

dropfrag:
	IPSTAT(ips_fragdropped++);
	m_freem(m);
	return (0);
}

/*
 * Free a fragment reassembly header and all
 * associated datagrams.
 */
/* 12.9.88.us  Enter with lk_ipq set.  */ 
ip_freef(fp)
	struct ipq *fp;
{
	register struct ipasfrag *q, *p;

	if (smp_debug){
		if (smp_owner(&lk_ipq) == 0)
			panic("ip_freef not lock owner");
	}
	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = p) {
		p = q->ipf_next;
		ip_deq(q);
		m_freem(dtom(q));
	}
	remque(fp);
	KM_FREE(fp, KM_FTABLE);
}

/*
 * Put an ip fragment on a reassembly chain.
 * Like insque, but pointers in middle of structure.
 */
/* 12.9.88.us  Enter with lk_ipq set.  */ 
ip_enq(p, prev)
	register struct ipasfrag *p, *prev;
{

	p->ipf_prev = prev;
	p->ipf_next = prev->ipf_next;
	prev->ipf_next->ipf_prev = p;
	prev->ipf_next = p;
}

/*
 * To ip_enq as remque is to insque.
 */
/* 12.9.88.us  Enter with lk_ipq set.  */ 
ip_deq(p)
	register struct ipasfrag *p;
{

	p->ipf_prev->ipf_next = p->ipf_next;
	p->ipf_next->ipf_prev = p->ipf_prev;
}

/*
 * IP timer processing;
 * if a timer expires on a reassembly
 * queue, discard it.
 */
ip_slowtimo()
{
	register struct ipq *fp;
	int s = splnet();

	/* 12.9.88.us.  Bug fix - panicing from a null referenced pointer
	 * from inadequate locking on lk_ipq.  Note that all fixes on 
	 * 12.9.88 are to fix this panic.  */
	smp_lock(&lk_ipq, LK_RETRY);
	fp = ipq.next;
	if (fp == 0) {
		smp_unlock(&lk_ipq);
		splx(s);
		return;
	}
	while (fp != &ipq) {
		--fp->ipq_ttl;
		fp = fp->next;
		if (fp->prev->ipq_ttl == 0) {
			IPSTAT(ips_fragtimeout++);
			ip_freef(fp->prev);
		}
	}
	/* 12.8.88.us */
	smp_unlock(&lk_ipq);
	splx(s);
}

/*
 * Drain off all datagram fragments.
 */
ip_drain()
{

	/* 12.8.88.us */
	smp_lock(&lk_ipq, LK_RETRY);
	while (ipq.next != &ipq) {
		IPSTAT(ips_fragdropped++);
		ip_freef(ipq.next);
	}
	/* 12.8.88.us */
	smp_unlock(&lk_ipq);
}

extern struct in_ifaddr *ifptoia();
struct in_ifaddr *ip_rtaddr();
int icmp_badoptlen=0;		/* debug only: count of bad optlens */
int icmp_badlsrroffset=0;	/* debug only: count of bad LSRR options */

/*
 * Do option processing on a datagram,
 * possibly discarding it if bad options
 * are encountered.
 */
ip_dooptions(ip, ifp)
	register struct ip *ip;
	struct ifnet *ifp;
{
	register u_char *cp;
	int opt, optlen, cnt, off, code, type = ICMP_PARAMPROB;
	register struct ip_timestamp *ipt;
	register struct in_ifaddr *ia;
	struct in_addr *sin;
	n_time ntime;
	int s;

	cp = (u_char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				icmp_badoptlen++;
				m_freem(dtom(ip));
				goto bad2;	/* don't attempt reply pkt */
			}
		}
		switch (opt) {

		default:
			break;

		/*
		 * Source routing with record.
		 * Find interface with current destination address.
		 * If none on this machine then drop if strictly routed,
		 * or do nothing if loosely routed.
		 * Record interface address and bring up next address
		 * component.  If strictly routed make sure next
		 * address on directly accessible net.
		 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				icmp_badlsrroffset++;
				m_freem(dtom(ip));
				goto bad2; /* don't attempt reply pkt */
			}
#ifdef mips
                        bcopy(&ip->ip_dst, &ipaddr.sin_addr,
                                sizeof(ip->ip_dst));
#endif mips
#ifdef vax
                        ipaddr.sin_addr = ip->ip_dst;
#endif vax
			ia = (struct in_ifaddr *)
				ifa_ifwithaddr((struct sockaddr *)&ipaddr);
			if (ia == 0) {
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward.
				 */
				break;
			}
			off--;			/* 0 origin */
			if (off > optlen - sizeof(struct in_addr)) {
				/*
				 * End of source route.  Should be for us.
				 */
				save_rte(cp, ip->ip_src);
				break;
			}
			/*
			 * locate outgoing interface
			 */
			bcopy((caddr_t)(cp + off), (caddr_t)&ipaddr.sin_addr,
			    sizeof(ipaddr.sin_addr));
			if ((opt == IPOPT_SSRR &&
			    in_iaonnetof(in_netof(ipaddr.sin_addr)) == 0) ||
			    (ia = ip_rtaddr(ipaddr.sin_addr)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_SRCFAIL;
				goto bad;
			}
#ifdef mips
                        bcopy(&ipaddr.sin_addr, &ip->ip_dst,
                                sizeof(ip->ip_dst));
#endif mips
#ifdef vax
			ip->ip_dst = ipaddr.sin_addr;
#endif vax
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_RR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore.
			 */
			off--;			/* 0 origin */
			if (off > optlen - sizeof(struct in_addr))
				break;
			bcopy((caddr_t)(&ip->ip_dst), (caddr_t)&ipaddr.sin_addr,
			    sizeof(ipaddr.sin_addr));
			/*
			 * locate outgoing interface
			 */
			if ((ia = ip_rtaddr(ipaddr.sin_addr)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (u_char *)ip;
			ipt = (struct ip_timestamp *)cp;
			if (ipt->ipt_len < 5)
				goto bad;
			if (ipt->ipt_ptr > ipt->ipt_len - sizeof (long)) {
				if (++ipt->ipt_oflw == 0)
					goto bad;
				break;
			}
			sin = (struct in_addr *)(cp + ipt->ipt_ptr - 1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				ia = ifptoia(ifp);
				bcopy((caddr_t)&IA_SIN(ia)->sin_addr,
				    (caddr_t)sin, sizeof(struct in_addr));
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			case IPOPT_TS_PRESPEC:
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				bcopy((caddr_t)sin, (caddr_t)&ipaddr.sin_addr,
				    sizeof(struct in_addr));
				if (ifa_ifwithaddr((struct sockaddr *)&ipaddr) == 0)
					continue;
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			default:
				goto bad;
			}
			ntime = iptime();
			bcopy((caddr_t)&ntime, (caddr_t)cp + ipt->ipt_ptr - 1,
			    sizeof(n_time));
			ipt->ipt_ptr += sizeof(n_time);
		}
	}
	return (0);
bad:
	ip->ip_len -= sizeof(struct ip);  /* deduct header added by ipintr() */
	icmp_error(ip, type, code, ifp, NULL);
	/* fallthrough */
bad2:
	IPSTAT(ips_badoptions++); /* track number of bad options */
	return (1);
}

/*
 * Given address of next destination (final or next hop),
 * return internet address info of interface to be used to get there.
 */
struct in_ifaddr *
ip_rtaddr(dst)
	 struct in_addr dst;
{
	register struct sockaddr_in *sin;
	register struct in_ifaddr *ia;

	RTLOCK();
	sin = (struct sockaddr_in *) &ipforward_rt.ro_dst;

	if (ipforward_rt.ro_rt == 0 || dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			rtfree(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_addr = dst;
		rtalloc(&ipforward_rt);
	}
	if (ipforward_rt.ro_rt == 0){
		RTUNLOCK();
		return ((struct in_ifaddr *)0);
	}

	/*
	 * Find address associated with outgoing interface.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (ia->ia_ifp == ipforward_rt.ro_rt->rt_ifp)
			break;
	RTUNLOCK();
	return (ia);
}

/*
 * Save incoming source route for use in replies,
 * to be picked up later by ip_srcroute if the receiver is interested.
 */
save_rte(option, dst)
	u_char *option;
	struct in_addr dst;
{
	unsigned olen;
	extern ipprintfs;

	olen = option[IPOPT_OLEN];
	if (olen > sizeof(ip_srcrt) - 1) {
		if (ipprintfs)
			printf("save_rte: olen %d\n", olen);
		return;
	}
	bcopy((caddr_t)option, (caddr_t)ip_srcrt.srcopt, olen);
	ip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);
	ip_srcrt.route[ip_nhops++] = dst;
}

/*
 * Retrieve incoming source route for use in replies,
 * in the same form used by setsockopt.
 * The first hop is placed before the options, will be removed later.
 */
struct mbuf *
ip_srcroute()
{
	register struct in_addr *p, *q;
	register struct mbuf *m;

	if (ip_nhops == 0)
		return ((struct mbuf *)0);
	m = m_get(M_DONTWAIT, MT_SOOPTS);
	if (m == 0)
		return ((struct mbuf *)0);
	m->m_len = ip_nhops * sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1;

	/*
	 * First save first hop for return route
	 */
	p = &ip_srcrt.route[ip_nhops - 1];
	*(mtod(m, struct in_addr *)) = *p--;

	/*
	 * Copy option fields and padding (nop) to mbuf.
	 */
	ip_srcrt.nop = IPOPT_NOP;
	bcopy((caddr_t)&ip_srcrt, mtod(m, caddr_t) + sizeof(struct in_addr),
	    IPOPT_OFFSET + 1 + 1);
	q = (struct in_addr *)(mtod(m, caddr_t) +
	    sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1);
	/*
	 * Record return path as an IP source route,
	 * reversing the path (pointers are now aligned).
	 */
	while (p >= ip_srcrt.route)
		*q++ = *p--;
	return (m);
}

/*
 * Strip out IP options, at higher
 * level protocol in the kernel.
 * Second argument is buffer to which options
 * will be moved, and return value is their length.
 */
ip_stripoptions(ip, mopt)
	struct ip *ip;
	struct mbuf *mopt;
{
	register int i;
	register struct mbuf *m;
	register caddr_t opts;
	int olen, optsoff = 0;

	olen = (ip->ip_hl<<2) - sizeof (struct ip);
	m = dtom(ip);
	opts = (caddr_t)(ip + 1);

	if (mopt) {
		/*
		 * If m_len is 0, we're dealing with an option set which
		 * ip_srcroute found no source routing in.  So, we've got an
		 * empty mbuf, into the * beginning of which we have to coerce
		 * a "first hop" address.  In a packet with no source routing,
		 * this would be the destination * address.  Otherwise, m_len
		 * is real, and we're just appending to the mbuf coming out
		 * of ip_srcroute.
		 */
		if (!mopt->m_len) {
			mopt->m_len = sizeof(struct in_addr);
			bcopy(&ip->ip_dst, mtod(mopt, caddr_t), mopt->m_len);
		}

		/*
		 * Push the rest of the options in.  We don't have to worry
		 * about the other IP level options like we do the source
		 * routing, so just search for them and insert them into the
		 * mbuf.  Notice that anything dealing with source routing is
		 * ignored, since you would want to do that in ip_srcroute
		 * instead.
		 */
		while (optsoff + 1 <= olen) {
			switch(opts[optsoff]) {
			case IPOPT_LSRR:
			case IPOPT_SSRR:
				optsoff += opts[optsoff + IPOPT_OLEN];
				break;
			case IPOPT_EOL:
			case IPOPT_NOP:
				mopt->m_dat[mopt->m_len++] = opts[optsoff++];
				break;
			default:
				bcopy(&opts[optsoff],&mopt->m_dat[mopt->m_len],
					opts[optsoff + IPOPT_OLEN]);
				mopt->m_len += opts[optsoff + IPOPT_OLEN];
				optsoff += opts[optsoff + IPOPT_OLEN];
				break;
			}
		}
		mopt->m_off = MMINOFF;
	}  

	i = m->m_len - (sizeof (struct ip) + olen);
	bcopy(opts  + olen, opts, (unsigned)i);
	m->m_len -= olen;
	ip->ip_hl = sizeof(struct ip) >> 2;
}

u_char inetctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		0,		EHOSTDOWN,	EHOSTUNREACH,
	ENETUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

#ifndef IPFORWARDING
#define IPFORWARDING	1
#endif
#ifndef IPSENDREDIRECTS
#define IPSENDREDIRECTS 1
#endif
int	ipprintfs = 0;
int	ipforwarding = IPFORWARDING;
extern int in_interfaces;
int ipsendredirects = IPSENDREDIRECTS;
int ipgateway = 0;

/*
 * Forward a packet.  If some error occurs return the sender
 * an icmp packet.  Note we can't always generate a meaningful
 * icmp message because icmp doesn't have a large enough repertoire
 * of codes and types.
 *
 * If not forwarding (possibly because we have only a single external
 * network), just drop the packet.  This could be confusing if ipforwarding
 * was zero but some routing protocol was advancing us as a gateway
 * to somewhere.  However, we must let the routing protocol deal with that.
 */
/*
 * SMP: No smp locks held coming in.
 * No lock held for call to ip_output. 5.27.87.us
 */
ip_forward(ip, ifp)
	register struct ip *ip;
	struct ifnet *ifp;
{
	register int error, type = 0, code;
	register struct sockaddr_in *sin;
	struct mbuf *mcopy;
	struct in_addr dest;

	dest.s_addr = 0;
	if (ipprintfs)
		printf("forward: src %x dst %x ttl %x\n", ip->ip_src,
			ip->ip_dst, ip->ip_ttl);
	ip->ip_id = htons(ip->ip_id);
	if (ipforwarding == 0 || in_interfaces <= 1) {
		IPSTAT(ips_cantforward++);
		if (ipgateway) {
			/* can't tell difference between net and host */
			type = ICMP_UNREACH, code = ICMP_UNREACH_NET;
			goto sendicmp;
		} else {
			m_freem(dtom(ip));
			return;
		}	
	}
	if (in_canforward(ip->ip_dst) == 0) {
		m_freem(dtom(ip));
		return;
	}
	if (ip->ip_ttl <= IPTTLDEC) {
		type = ICMP_TIMXCEED, code = ICMP_TIMXCEED_INTRANS;
		goto sendicmp;
	}
	ip->ip_ttl -= IPTTLDEC;

	/*
	 * Save at most 64 bytes of the packet in case
	 * we need to generate an ICMP message to the src.
	 */
	mcopy = m_copy(dtom(ip), 0, imin((int)ip->ip_len, 64));
	RTLOCK();
	sin = (struct sockaddr_in *)&ipforward_rt.ro_dst;
	if (ipforward_rt.ro_rt == 0 ||
	    ip->ip_dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			rtfree(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_addr = ip->ip_dst;
		rtalloc(&ipforward_rt);
	}
	/*
	 * If forwarding packet using same interface that it came in on,
	 * perhaps should send a redirect to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a default route
	 * or a route modfied by a redirect.
	 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
	if (ipforward_rt.ro_rt && ipforward_rt.ro_rt->rt_ifp == ifp &&
	    (ipforward_rt.ro_rt->rt_flags & (RTF_DYNAMIC|RTF_MODIFIED)) == 0 &&
	    satosin(&ipforward_rt.ro_rt->rt_dst)->sin_addr.s_addr != 0 &&
	    ipsendredirects && ip->ip_hl == (sizeof(struct ip) >> 2)) {
		struct in_ifaddr *ia;
		u_long src = ntohl(ip->ip_src.s_addr);
		u_long dst = ntohl(ip->ip_dst.s_addr);

		if ((ia = ifptoia(ifp)) &&
		   (src & ia->ia_subnetmask) == ia->ia_subnet) {
		    if (ipforward_rt.ro_rt->rt_flags & RTF_GATEWAY)
			dest = satosin(&ipforward_rt.ro_rt->rt_gateway)->sin_addr;
		    else
			dest = ip->ip_dst;
		    /*
		     * If the destination is reached by a route to host,
		     * is on a subnet of a local net, or is directly
		     * on the attached net (!), use host redirect.
		     * (We may be the correct first hop for other subnets.)
		     */
		    type = ICMP_REDIRECT;
		    code = ICMP_REDIRECT_NET;
		    if ((ipforward_rt.ro_rt->rt_flags & RTF_HOST) ||
		       (ipforward_rt.ro_rt->rt_flags & RTF_GATEWAY) == 0)
			code = ICMP_REDIRECT_HOST;
		    else{ 
		    for (ia = in_ifaddr; ia = ia->ia_next; )
			if ((dst & ia->ia_netmask) == ia->ia_net) {
			    if (ia->ia_subnetmask != ia->ia_netmask)
				    code = ICMP_REDIRECT_HOST;
			    break;
			}
		    }
		    if (ipprintfs)
		        printf("redirect (%d) to %x\n", code, dest);
		}
	}
	RTUNLOCK();
	/* SMP: call ip_output with no smp lock.  4.21.87.us */
	error = ip_output(dtom(ip), (struct mbuf *)0, &ipforward_rt,
		IP_FORWARDING, NULL);
	if (error) { 
		IPSTAT(ips_cantforward++);
	} else if (type) {
		IPSTAT(ips_redirectsent++);
	} else {
		if (mcopy)
			m_freem(mcopy);
		IPSTAT(ips_forward++);
		return;
	}
	if (mcopy == NULL)
		return;
	ip = mtod(mcopy, struct ip *);
	type = ICMP_UNREACH;
	switch (error) {

	case 0:				/* forwarded, but need redirect */
		type = ICMP_REDIRECT;
		/* code set above */
		break;

	case ENETUNREACH:
	case ENETDOWN:
		if (in_localaddr(ip->ip_dst))
			code = ICMP_UNREACH_HOST;
		else
		code = ICMP_UNREACH_NET;
		break;

	case EMSGSIZE:
		code = ICMP_UNREACH_NEEDFRAG;
		break;

	case EPERM:
		code = ICMP_UNREACH_PORT;
		break;

	case ENOBUFS:
		type = ICMP_SOURCEQUENCH;
		break;

	case EHOSTDOWN:
	case EHOSTUNREACH:
		code = ICMP_UNREACH_HOST;
		break;
	}
sendicmp:
	icmp_error(ip, type, code, ifp, dest);
}
