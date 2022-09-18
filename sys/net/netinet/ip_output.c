#ifndef lint
static	char	*sccsid = "@(#)ip_output.c	4.5	(ULTRIX)	11/9/90";
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
 *	02-Aug-90	lp
 *		Fix ROUTETOIF case in route cache extensions.
 *
 *	31-Jul-90	jsd
 *		word align optlen for mips when inserting IP options
 *
 *	7-Jul-90	lp
 *		FDDI performance.
 *
 *	2-Jan-90	U. Sinkewicz
 *		Performance enhancements to uniprocessor kernel.
 *
 *	11-14-89	Ursula Sinkewicz
 *		Removed lk_ifnet and lk_in_ifaddr to coincide with
 *		slip changes.
 *
 *	10-18-89   	Uttam Shikarpur					*
 *		Added counters for network management to keep track	*
 *		of: 1) Number of IP packets sent out (ips_totalsent)	*
 *		    2) Number of error-free IP packets discarded	*
 *		       (ips_outdiscard)					*
 *		    3) Number of packets that were fragmented 	  	*
 *		       (ips_outpktsfrag)				*
 *		    4) Number of fragments that were created		*
 *		       (ips_outtotalfrag)				*
 *		    5) Number of packets that could not be fragmented 	*
 *		       (ips_outfragsfail)				*
 *									*
 *	30-May-89	U. Sinkewicz
 *		Added support for nonsymmetric network drivers.
 *
 *	11-Apr-89	Ursula Sinkewicz
 *		Picked up memenemy changes from 2/10/89 making ip options
 *		long word aligned before insertion in output packets.
 *
 *	27-Mar-89	Ursula Sinkewicz
 *		Lowered ipl on lk_rtentry, lk_ifnet, lk_in_ifaddr as per
 *		lp changes on 3/16/89.
 *
 *	3 Mar 89	Ursula Sinkewicz
 *		Added support for new directory layout to smp file.
 *
 * 	13 Feb 89	Ursula Sinkewicz
 *		SMP: Added lk_in_ifaddr, lk_ifnet.
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 *	Larry Cohen - 01/28/87
 *		Add ip ouput control routine and ip options processing
 *			routine
 *									*
 *	Chet Juszczak - 03/12/86					*
 *		Add new packet fragmentation code for NFS		*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	ip_output.c	7.6 (Berkeley) 6/20/87
 */

#include "../h/param.h"
#include "../h/cpudata.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/errno.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/smp_lock.h"

#include "../net/net/if.h"
#include "../net/net/route.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/in_pcb.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif

struct mbuf *ip_insertoptions();

/*
 * IP output. The packet in mbuf chain m contains a skeletal IP
 * header (with len, off, ttl, proto, tos, src, dst).
 * The mbuf chain containing the packet will be freed.
 * The mbuf opt, if present, will not be freed.
/* 
 * SMP: enter from tcp_output and protosw with NO locks set.
 * Results in a hack in tcp_output but you cannot require
 * locks be set coming in because of possibility of being triggered from
 * protosw.  Further, there is no data manipulated here that is under the
 * control of the socket lock.
 */

ip_output(m, opt, ro, flags, so)
	struct mbuf *m;
	struct mbuf *opt;
	struct route *ro;
	int flags;
	struct socket *so;
{
	register struct ip *ip;
	register struct ifnet *ifp;
	int len, hlen = sizeof (struct ip), off, error = 0;
	struct route iproute;
	struct sockaddr_in *dst;
	int saveaffinity;

	if (opt)				
		m = ip_insertoptions(m, opt, &hlen);
	ip = mtod(m, struct ip *);
	/*
	 * Fill in IP header.
	 */
	if ((flags & IP_FORWARDING) == 0) {
		ip->ip_v = IPVERSION;
		ip->ip_off &= IP_DF;
		ip->ip_id = htons(ip_id++);
/*
 * CJ - SUN has the following line before this if statement.
 */
		ip->ip_hl = hlen >> 2;
	} else
		hlen = ip->ip_hl << 2;

	/*
	 * Route packet.
	 */
	RTLOCK();
	if (ro == 0) {
		ro = &iproute;
		bzero((caddr_t)ro, sizeof (*ro));
	}
	dst = (struct sockaddr_in *)&ro->ro_dst;
	/*
	 * If there is a cached route,
	 * check that it is to the same destination
	 * and is still up.  If not, free it and try again.
	 */
	if (ro->ro_rt && (ro->ro_rt->rt_flags & RTF_UP) &&
	   dst->sin_addr.s_addr == ip->ip_dst.s_addr) {
		if((ifp = ro->ro_rt->rt_ifp) && (flags&IP_ROUTETOIF)) {
			RTUNLOCK();
			goto fastout;
		}

	} else {
		if(ro->ro_rt) {
			rtfree(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)0;
		}
	}
	if (ro->ro_rt == 0) {
		dst->sin_family = AF_INET;
		dst->sin_addr = ip->ip_dst;
	}
	/*
	 * If routing to interface only,
	 * short circuit routing lookup.
	 */
	if (flags & IP_ROUTETOIF) {
		struct in_ifaddr *ia;

		ia = (struct in_ifaddr *)ifa_ifwithdstaddr(dst);
		if (ia == 0)
			ia = in_iaonnetof(in_netof(ip->ip_dst));
		if (ia == 0) {
			error = ENETUNREACH;
			RTUNLOCK();
			IPSTAT(ips_outdiscard++); /* a packet was discarded*/
			goto bad;
		}
		ifp = ia->ia_ifp;
	} else {
		if (ro->ro_rt == 0)
			rtalloc(ro);
		if (ro->ro_rt == 0 || (ifp = ro->ro_rt->rt_ifp) == 0) {
			if (in_localaddr(ip->ip_dst))
				error = EHOSTUNREACH;
			else
				error = ENETUNREACH;
			RTUNLOCK();
			IPSTAT(ips_outdiscard++); /* a packet was discarded */
			goto bad;
		}
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst = (struct sockaddr_in *)&ro->ro_rt->rt_gateway;
	}
	RTUNLOCK();

	/*
	 * Look for broadcast address and
	 * and verify user is allowed to send
	 * such a packet.
	 */
	if (in_broadcast(dst->sin_addr)) {
		if ((ifp->if_flags & IFF_BROADCAST) == 0) {
			error = EADDRNOTAVAIL;
			IPSTAT(ips_outdiscard++); /* a packet was discarded */
			goto bad;
		}
		if ((flags & IP_ALLOWBROADCAST) == 0) {
			error = EACCES;
			goto bad;
		}
		/* don't allow broadcast messages to be fragmented */
		if (ip->ip_len > ifp->if_mtu) {
			error = EMSGSIZE;
			IPSTAT(ips_outfragsfail++); /* a packet could
						     * not be fragmented
						     */
			goto bad;
		}
	}

fastout:
	/*
	 * If source address not specified yet, use address
	 * of outgoing interface.
	 */
	if (ip->ip_src.s_addr == INADDR_ANY) {
		register struct in_ifaddr *ia;

		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if (ia->ia_ifp == ifp) {
				ip->ip_src = IA_SIN(ia)->sin_addr;
				break;
			}
	}

	/*
	 * If small enough for interface, can just send directly.
	 */
	if (ip->ip_len <= ifp->if_mtu) {
		ip->ip_len = htons((u_short)ip->ip_len);
		ip->ip_off = htons((u_short)ip->ip_off);
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(m, hlen);
		/* Support for nonsymm net devices. 8.9.88.us */
		if ( !smp)
			error = (*ifp->if_output)(ifp, m, (struct sockaddr *)dst);
		else {
		if ( ifp->d_affinity != boot_cpu_mask)
			error = (*ifp->if_output)(ifp, m, (struct sockaddr *)dst);
		else{

			if (so){
				so->ref = 24;
				smp_unlock(&so->lk_socket);
			}
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
			error = (*ifp->if_output)(ifp, m, (struct sockaddr *)dst);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			if ( so  ){		
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
			}
		}
		}
		/* for network management */
		if (!error)	/* if there was no error while sending */
			IPSTAT(ips_totalsent++); /* counter for total
						  * IP packets sent out.
						  */
		goto done;
	}

	/*
	 * Too large for interface; fragment if possible.
	 * Must be able to put at least 8 bytes per fragment.
	 */
	if (ip->ip_off & IP_DF) {
		error = EMSGSIZE;
		IPSTAT(ips_outfragsfail++); /* a packet could
					     * not be fragmented
					     */
		goto bad;
	}
	len = (ifp->if_mtu - hlen) &~ 7;
	if (len < 8) {
		error = EMSGSIZE;
		IPSTAT(ips_outfragsfail++); /* a packet could
					     * not be fragmented
					     */
		goto bad;
	}

	/*
	 * CJ:
	 * Call to new packet fragmentation routine.
	 * Added for NFS.
	 */
	if (hlen == sizeof (struct ip) &&
	    ip_frag2(m, ip, len, &error, ifp, dst, so)) {
		IPSTAT(ips_outpktsfrag++); /* packet was successfully frag.*/
		goto done;
	}

	/*
	 * Discard IP header from logical mbuf for m_copy's sake.
	 * Loop through length of segment, make a copy of each
	 * part and output.
	 */
	m->m_len -= sizeof (struct ip);
	m->m_off += sizeof (struct ip);
	for (off = 0; off < ip->ip_len-hlen; off += len) {
		struct mbuf *mh = m_get(M_DONTWAIT, MT_DATA);
		struct ip *mhip;

		if (mh == 0) {
			error = ENOBUFS;
			IPSTAT(ips_outdiscard++); /* a packet was discarded */
			goto bad;
		}
		mh->m_off = MMAXOFF - hlen;
		mhip = mtod(mh, struct ip *);
		*mhip = *ip;
		if (hlen > sizeof (struct ip)) {
			int olen = ip_optcopy(ip, mhip, off);
			mh->m_len = sizeof (struct ip) + olen;
		} else
			mh->m_len = sizeof (struct ip);
		mhip->ip_off = (off >> 3) + (ip->ip_off & ~IP_MF);
		if (ip->ip_off & IP_MF)
			mhip->ip_off |= IP_MF;
		if (off + len >= ip->ip_len-hlen)
			len = mhip->ip_len = ip->ip_len - hlen - off;
		else {
			mhip->ip_len = len;
			mhip->ip_off |= IP_MF;
		}
		mhip->ip_len += sizeof (struct ip);
		mhip->ip_len = htons((u_short)mhip->ip_len);
		mh->m_next = m_copy(m, off, len);
		if (mh->m_next == 0) {
			(void) m_free(mh);
			error = ENOBUFS;	/* ??? */
			IPSTAT(ips_outdiscard++); /* a packet was discarded */
			goto bad;
		}
		mhip->ip_off = htons((u_short)mhip->ip_off);
		mhip->ip_sum = 0;
		mhip->ip_sum = in_cksum(mh, hlen);
		/* Support for nonsymm net devices. 8.9.88.us */
		if (!smp)
			error = (*ifp->if_output)(ifp, mh, (struct sockaddr *)dst);
		else{
		if (ifp->d_affinity != boot_cpu_mask)
			error = (*ifp->if_output)(ifp, mh, (struct sockaddr *)dst);
		else{
			if (so){	
				so->ref = 25;
				smp_unlock(&so->lk_socket);
			}
			CALL_TO_NONSMP_DRIVER( (*ifp),saveaffinity);
			error = (*ifp->if_output)(ifp, mh, (struct sockaddr *)dst);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			if (so){
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
			}
		}
		}
		if (error)
			break;
		else { /* bump up counters for network management */
			IPSTAT(ips_totalsent++);

			/* a packet was successfully fragmented */
			if(ip->ip_off & IP_MF) {
				IPSTAT(ips_outpktsfrag++);
			}
		}
	}
bad:
	m_freem(m);
	m = NULL;
done:
	RTLOCK();
	if (ro == &iproute && (flags & IP_ROUTETOIF) == 0 && ro->ro_rt)
		rtfree(ro->ro_rt);
	RTUNLOCK();
	return (error);
}

/*
 * Insert IP options into preformed packet.
 * Adjust IP destination as required for IP source routing,
 * as indicated by a non-zero in_addr at the start of the options.
 */
struct mbuf *
ip_insertoptions(m, opt, phlen)
	register struct mbuf *m;
	struct mbuf *opt;
	int *phlen;
{
	register struct ipoption *p = mtod(opt, struct ipoption *);
	struct mbuf *n;
	register struct ip *ip = mtod(m, struct ip *);
	unsigned optlen;

	optlen = opt->m_len - sizeof(p->ipopt_dst);
#ifdef mips
	while(optlen&0x03)	/* word align options, if necessary */
		optlen++;
#endif mips
	if (p->ipopt_dst.s_addr)
		ip->ip_dst = p->ipopt_dst;
	if (m->m_off >= MMAXOFF || MMINOFF + optlen > m->m_off) {
		MGET(n, M_DONTWAIT, MT_DATA);
		if (n == 0)
			return (m);
		m->m_len -= sizeof(struct ip);
		m->m_off += sizeof(struct ip);
		n->m_next = m;
		m = n;
		m->m_off = MMAXOFF - sizeof(struct ip) - optlen;
		m->m_len = optlen + sizeof(struct ip);
		bcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
	} else {
		m->m_off -= optlen;
		m->m_len += optlen;
		ovbcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
	}
	ip = mtod(m, struct ip *);
	bcopy((caddr_t)p->ipopt_list, (caddr_t)(ip + 1), (unsigned)optlen);
	*phlen = sizeof(struct ip) + optlen;
	ip->ip_len += optlen;
	return (m);
}

/*
 * Copy options from ip to jp.
 * If off is 0 all options are copied
 * otherwise copy selectively.
 */
ip_optcopy(ip, jp, off)
	struct ip *ip, *jp;
	int off;
{
	register u_char *cp, *dp;
	int opt, optlen, cnt;

	cp = (u_char *)(ip + 1);
	dp = (u_char *)(jp + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else
			optlen = cp[IPOPT_OLEN];
		if (optlen > cnt)			/* XXX */
			optlen = cnt;			/* XXX */
		if (off == 0 || IPOPT_COPIED(opt)) {
			bcopy((caddr_t)cp, (caddr_t)dp, (unsigned)optlen);
			dp += optlen;
		}
	}
	for (optlen = dp - (u_char *)(jp+1); optlen & 0x3; optlen++)
		*dp++ = IPOPT_EOL;
	return (optlen);
}

/*
 * IP socket option processing.
 */
ip_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;
{
	int error = 0;
	struct inpcb *inp = sotoinpcb(so);

	if (level != IPPROTO_IP)
		error = EINVAL;
	else switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case IP_OPTIONS:
			return (ip_pcbopts(&inp->inp_options, *m));

		default:
			error = EINVAL;
			break;
		}
		break;

	case PRCO_GETOPT:
		switch (optname) {
		case IP_OPTIONS:
			*m = m_get(M_DONTWAIT, MT_SOOPTS);
			if (*m == NULL){
				error = ENOBUFS;
				break;
			}
			if (inp->inp_options) {
				(*m)->m_off = inp->inp_options->m_off;
				(*m)->m_len = inp->inp_options->m_len;
				bcopy(mtod(inp->inp_options, caddr_t),
				    mtod(*m, caddr_t), (unsigned)(*m)->m_len);
			} else
				(*m)->m_len = 0;
			break;
		default:
			error = EINVAL;
			break;
		}
		break;
	}
	if (op == PRCO_SETOPT && *m)
		(void)m_free(*m);
	return (error);
}

/*
 * Set up IP options in pcb for insertion in output packets.
 * Store in mbuf with pointer in pcbopt, adding pseudo-option
 * with destination address if source routed.
 */
ip_pcbopts(pcbopt, m)
	struct mbuf **pcbopt;
	register struct mbuf *m;
{
	register int cnt, optlen;
	register u_char *cp;
	u_char opt;

	/* turn off any old options */
	if (*pcbopt)
		(void)m_free(*pcbopt);
	*pcbopt = 0;
	if (m == (struct mbuf *)0 || m->m_len == 0) {
		/*
		 * Only turning off any previous options.
		 */
		if (m)
			(void)m_free(m);
		return (0);
	}

	if (m->m_len % sizeof(long))
		goto bad;

	/*
	 * IP first-hop destination address will be stored before
	 * actual options; move other options back
	 * and clear it when none present.
	 */
#if	MAX_IPOPTLEN >= MMAXOFF - MMINOFF
	if (m->m_off + m->m_len + sizeof(struct in_addr) > MAX_IPOPTLEN)
		goto bad;
#else
	if (m->m_off + m->m_len + sizeof(struct in_addr) > MMAXOFF)
		goto bad;
#endif
	cnt = m->m_len;
	m->m_len += sizeof(struct in_addr);
	cp = mtod(m, u_char *) + sizeof(struct in_addr);
	ovbcopy(mtod(m, caddr_t), (caddr_t)cp, (unsigned)cnt);
	bzero(mtod(m, caddr_t), sizeof(struct in_addr));

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= IPOPT_OLEN || optlen > cnt)
				goto bad;
		}
		switch (opt) {

		default:
			break;

		case IPOPT_LSRR:
		case IPOPT_SSRR:
			/*
			 * user process specifies route as:
			 *	->A->B->C->D
			 * D must be our final destination (but we can't
			 * check that since we may not have connected yet).
			 * A is first hop destination, which doesn't appear in
			 * actual IP option, but is stored before the options.
			 */
			if (optlen < IPOPT_MINOFF - 1 + sizeof(struct in_addr))
				goto bad;
			m->m_len -= sizeof(struct in_addr);
			cnt -= sizeof(struct in_addr);
			optlen -= sizeof(struct in_addr);
			cp[IPOPT_OLEN] = optlen;
			/*
			 * Move first hop before start of options.
			 */
			bcopy((caddr_t)&cp[IPOPT_OFFSET+1], mtod(m, caddr_t),
			    sizeof(struct in_addr));
			/*
			 * Then copy rest of options back
			 * to close up the deleted entry.
			 */
			ovbcopy((caddr_t)(&cp[IPOPT_OFFSET+1] +
			    sizeof(struct in_addr)),
			    (caddr_t)&cp[IPOPT_OFFSET+1],
			    (unsigned)cnt + sizeof(struct in_addr));
			break;
		}
	}
	*pcbopt = m;
	return (0);

bad:
	(void)m_free(m);
	return (EINVAL);
}


/*
 * Attempt to fragment type 2 mbuf chain.
 * Works only if each mbuf is smaller than a packet.
 * This saves copying all the data.
 */
ip_frag2(m, ip, maxpacketlen, errorp, ifp, dst, so)
	register struct mbuf *m;
	register struct ip *ip;
	register int maxpacketlen;
	int *errorp;
	struct ifnet *ifp;
	struct sockaddr *dst;
	struct socket *so;
{
	struct mbuf *mm;
	struct mbuf *lastm;
	register struct mbuf *mh;
	register int fraglen, fragoff, pktlen, n;
	struct ip *nextip;
	int saveaffinity;

	/*
	 * Check whether we can do it.
	 */
	mm = m;
	n = 0;
	while (m) {
		if (m->m_off > MMAXOFF && m->m_cltype == 2) {
			n++;
		}
		if (m->m_len + sizeof (struct ip) > maxpacketlen) {
			return (0);
		}
		m = m->m_next;
	}
	if (n == 0) {	/* higher level does type 1 chain better */
		return (0);
	}
	m = mm;
	fragoff = 0;
	while (m) {
		pktlen = 0;
		mm = m;
		/*
		 * Gather up all the mbufs that will fit in a frag.
		 */
		while (m && pktlen + m->m_len <= maxpacketlen) {
			pktlen += m->m_len;
			lastm = m;
			m = m->m_next;
		}
		fraglen = pktlen - sizeof (struct ip);
		lastm->m_next = 0;
		if (m) {
			/*
			 * There are more frags, so we prepend
			 * a copy of the ip hdr to the rest
			 * of the chain.
			 */
			MGET(mh, M_DONTWAIT, MT_DATA);
			if (mh == 0) {
				*errorp = ENOBUFS;
				IPSTAT(ips_outdiscard++); /* a packet was
							   * discarded
							   */
				break;
			}
			mh->m_off = MMAXOFF - sizeof (struct ip) - 8;
			nextip = mtod(mh, struct ip *);
			/* copy the ip header */
			*nextip = *ip;
			mh->m_len = sizeof (struct ip);
			mh->m_next = m;
			m = mh;
			if (n = (fraglen & 7)) {
				/*
				 * IP fragments must be a multiple of
				 * 8 bytes long so we must play games.
				 */
				bcopy(mtod(lastm, caddr_t) + lastm->m_len
					- n, (caddr_t) (nextip + 1), n);
				lastm->m_len -= n;
				mh->m_len += n;
				pktlen -= n;
				fraglen -= n;
			}
			ip->ip_off = htons((u_short) ((fragoff >> 3) | IP_MF));
		} else {
			ip->ip_off = htons((u_short) (fragoff >> 3));
		}
		/*
		 * Fix up the ip header for the mm chain and send it off.
		 */
		if (ip->ip_len < pktlen) {
			ip->ip_len = htons((u_short) ip->ip_len);
			if (m) {
				m_freem(m);
				m = 0;
			}
		} else {
			ip->ip_len = htons((u_short) pktlen);
			if (m) {
				nextip->ip_len -= fraglen;
			}
		}
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(mm, sizeof (struct ip));
		/* Support for nonsymm net devices. 8.9.88.us */
		if (!smp)
			*errorp = (*ifp->if_output)(ifp, mm, dst);
		else{
		if (ifp->d_affinity != boot_cpu_mask)
			*errorp = (*ifp->if_output)(ifp, mm, dst);
		else{
			if (so){
				so->ref = 26;
				smp_unlock(&so->lk_socket);
			}
			CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
			*errorp = (*ifp->if_output)(ifp, mm, dst);
			RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			if (so){
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
			}
		}
		}
		if (*errorp){
			if(m)
				m_freem(m);
			break;
		} else {
			IPSTAT(ips_totalsent++); 
			/* fragmentation was successfull */ 
			IPSTAT(ips_outtotalfrag++); 
		}
		ip = nextip;
		fragoff += fraglen;
	}
	return (1);
}
