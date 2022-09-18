#ifndef lint
static	char	*sccsid = "@(#)kudp_fastsend.c	4.2	(ULTRIX)	2/21/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 *
 * 20 Feb 91	Uttam Shikarpur
 *		Counters that were added for network management
 *		were getting updated improperly. This was fixed.
 *
 * 09 May 90	Fred L. Templin
 *	Changed "m_off" sizing from "sizeof (struct ether_header)" to
 *	"M_NETPAD" to genericize for various LAN'S.
 *
 * 10 Dec 89 -- chet
 *	Check properly for checksum overflow in ku_fastsend().
 *
 *	24-Oct-89	Uttam Shikarpur
 *		Made MAXTTL writeable
 *		Added counters for network management
 *		1) Number of IP packets sent out (ips_totalsent)
 *		2) Number of fragments created (ips_totalfrag)
 *		3) Number of packets fragmented (ips_outpktsfrag)
 *		
 *	30-May-89	U. Sinkewicz
 *		Replace smp_lock(&so->lk_socket, LK_RETRY) with SO_LOCK
 *		as part of an smp bug fix.  Fix guarantees that socket 
 *		doesn't change while unlocked during sleeps or for the
 *		lock hierarchy.
 *
 *	27-Mar-89	U. Sinkewicz
 *		Reflected ip routing changes (lp 3/16/89).
 *
 *	18-Aug-88	condylis
 *		Added lk_rpcroute SMP lock for static route structure used
 *		by ku_fastsend.  Added use of lk_rtentry lock in 
 *		ku_fastsend.
 *
 *	21-Jun-88	condylis
 *		Added interaction with SMP socket lock
 *
 *	lp 5-Apr-88
 *		Initialize mm to zero as it was getting junk off the
 *		stack.
 *
 *	lp 15-Jan-88
 *		Change for malloced mbufs.
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/file.h"

/* This group of includes supports nonsmp net devices. 8.9.88.us */
#include "../h/cpudata.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_var.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/udp.h"
#include "../netinet/udp_var.h"
#include "../rpc/types.h"
#include "../h/errno.h"

/* SMP lock for ku_fastsend's static route structure	*/
struct lock_t lk_rpcroute;


static
buffree()
{
}

/*
 * This is the "fast path" to send a UDP packet.  The return value from
 * ku_fastsend is a little weird: 0 if success, -1 if nonfatal error (the
 * mbuf chain has not been freed), errno if fatal error (the mbuf chain
 * has been freed).
 */
ku_fastsend(so, am, to)
	struct socket *so;		/* socket data is sent from */
	register struct mbuf *am;	/* data to be sent */
	struct sockaddr_in *to;		/* destination data is sent to */
{
	register int datalen;		/* length of all data in packet */
	register int maxlen;		/* max length of fragment */
	register int curlen;		/* data fragment length */
	register int fragoff;		/* data fragment offset */
	register int sum;		/* ip header checksum */
	register int grablen;		/* number of mbuf bytes to grab */
	register struct udpiphdr *ui;	/* udp/ip header */
	register struct mbuf *m;	/* ip header mbuf */
	struct ip *ip;			/* ip header */
	struct ifnet *ifp;		/* interface */
	struct mbuf *lam;		/* last mbuf in chain to be sent */
	struct sockaddr	*dst;		/* packet destination */
	struct inpcb *inp;		/* inpcb for binding */
	struct ip *nextip;		/* ip header for next fragment */
	static struct route *route[MAXCPU];/* route to send packet */
	static struct route zero_route;	/* to initialize route */
	int	s;			/* to save spl state	*/
	struct in_ifaddr *ia;
	struct sockaddr_in *ifaddr;
	extern int udpcksum;
	int error;
	int saveaffinity;	/* supports nonsmp net devices 8.9.88.us */

	/*
	 * Determine length of data.
	 * This should be passed in as a parameter.
	 */
	datalen = 0;
	for (m = am; m; m = m->m_next) {
		datalen += m->m_len;
	}

	/*
	 * Routing.
	 * We worry about routing early so we get the right ifp.
	 */
	{
		register struct route *ro;

		if ((ro = route[CURRENT_CPUDATA->cpu_num]) == NULL) {
			kmem_alloc(ro, struct route *, (u_int)sizeof(struct route), KM_RPC);
			route[CURRENT_CPUDATA->cpu_num] = ro;
		}
			
		s = splnet();
		RTLOCK();
		if (ro->ro_rt == 0 || (ro->ro_rt->rt_flags & RTF_UP) == 0 ||
		    ((struct sockaddr_in *)&ro->ro_dst)->sin_addr.s_addr !=
		    to->sin_addr.s_addr) {
			if (ro->ro_rt) {
				rtfree(ro->ro_rt);
			}
			*ro = zero_route;
			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *)&ro->ro_dst)->sin_addr =
			    to->sin_addr;
			rtalloc(ro);
			if (ro->ro_rt == 0 || ro->ro_rt->rt_ifp == 0) {
				(void) m_freem(am);
				RTUNLOCK();
				splx(s);
				return (ENETUNREACH);
			}
		}
		ifp = ro->ro_rt->rt_ifp;
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
			dst = &ro->ro_rt->rt_gateway;
		} else {
			dst = &ro->ro_dst;
		}
		RTUNLOCK();
		splx(s);

		s = splnet();
		smp_lock(&lk_in_ifaddr, LK_RETRY);
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if (ia->ia_ifp == ifp)
				break;
		if (ia == 0) {
			ia = in_ifaddr;
			if (ia == 0) {
				smp_unlock(&lk_in_ifaddr);
				splx(s);
				printf ("ku_fastsend: no interface\n");
				return (-1);
			}
		}
		ifaddr = (struct sockaddr_in *)&ia->ia_addr;
		smp_unlock(&lk_in_ifaddr);
		splx(s);

	}
	/*
	 * Bind port, if necessary.
	 */
	/* SMP lock is for access to socket structure in sotoinpcb
	 * and for call to in_pcbbind
 	 */
	s = splnet();
	SO_LOCK(so);
	/* smp_lock(&so->lk_socket, LK_RETRY); */
	inp = sotoinpcb(so);
	if (inp->inp_laddr.s_addr == INADDR_ANY && inp->inp_lport==0) {
		(void)in_pcbbind(inp, (struct mbuf *)0);
	}
	smp_unlock(&so->lk_socket);
	splx(s);
	/*
	 * Get mbuf for ip, udp headers.
	 */
	MGET(m, M_WAIT, MT_DATA);
	if (m == NULL) {
		(void) m_freem(am);
		return (ENOBUFS);
	}
	/*
	 * Next statement guarantees longword alignment. This is
	 * absolutely required for MIPS...
	 */
	m->m_off = MMINOFF + M_NETPAD;
	m->m_len = sizeof (struct udpiphdr);
	m->m_next = am;
	/*
	 * Create UDP/IP header for checksumming.
	 */
	ui = mtod(m, struct udpiphdr *);
	ui->ui_next = ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_short)(datalen + sizeof (struct udphdr)));
	ui->ui_src = ifaddr->sin_addr;
	ui->ui_dst = to->sin_addr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = to->sin_port;
	ui->ui_ulen = ui->ui_len;
	ui->ui_sum = 0;

	if(udpcksum)
		if ((ui->ui_sum = in_cksum(m, sizeof (struct udpiphdr) + datalen)) == 0)
			ui->ui_sum = -1;

	/*
	 * Fill in rest of IP header.
	 */
	ip = (struct ip *)ui;
	ip->ip_hl = sizeof (struct ip) >> 2;
	ip->ip_v = IPVERSION;
	ip->ip_tos = 0;
	ip->ip_id = ip_id++;
	ip->ip_off = 0;
	ip->ip_ttl = maxttl;
	ip->ip_sum = 0;

	/*
	 * Fragment the data into packets big enough for the
	 * interface, prepend the header, and send them off.
	 */
	maxlen = (ifp->if_mtu - sizeof (struct ip)) & ~7;
	curlen = sizeof (struct udphdr);
	fragoff = 0;
	for (;;) {
		register struct mbuf *mm = NULL;
		register outtotalfrag;   /* for statistics */
		register outpktsfrag;    /* for statistics */

		/* initialize the counters */
		outtotalfrag = 0;
		outpktsfrag = 0;

		m->m_next = am;
		lam = m;
		while (am->m_len + curlen <= maxlen) {
			curlen += am->m_len;
			lam = am;
			am = am->m_next;
			if (am == 0) {
				ip->ip_off = htons((u_short) (fragoff >> 3));
				goto send;
			}
		}
		if (curlen == maxlen) {
			/*
			 * Incredible luck: last mbuf exactly
			 * filled out the packet.
			 */
			lam->m_next = 0;
		} else {
			/*
			 * We can squeeze part of the next
			 * mbuf into this packet, so we
			 * get a type 2 mbuf and point it at
			 * this data fragment.
			 */
			MGET(mm, M_WAIT, MT_DATA);
			if (mm == NULL) {
				(void)m_free(m);
				mm = dtom(ip);
				if (mm != m) {
					(void)m_free(mm);
				}
				printf("ku_sendit: MGET failed\n");
				return (-1);
			}
			grablen = maxlen - curlen;
			mm->m_off = mtod(am, int);
			mm->m_len = grablen;
			mm->m_cltype = 2;
			mm->m_clfun = buffree;
			mm->m_clswp = NULL;
			lam->m_next = mm;
			am->m_len -= grablen;
			am->m_off += grablen;
			curlen = maxlen;
		}

		/*
		 * m now points to the head of an mbuf chain which
		 * contains the max amount that can be sent in a packet.
		 */
		ip->ip_off = htons((u_short) ((fragoff >> 3) | IP_MF));
		/*
		 * There are more frags, so we save
		 * a copy of the ip hdr for the next
		 * frag.
		 */
		MGET(mm, M_WAIT, MT_DATA);
		if (mm == 0) {
			(void)m_free(dtom(ip));
			printf("ku_sendit: MGET failed\n");
			return (-1);
		}
		/*
		 * Next statement guarantees longword alignment. This is
		 * absolutely required for MIPS...
		 */
		mm->m_off = MMINOFF + M_NETPAD;
		mm->m_len = sizeof (struct ip);
		nextip = mtod(mm, struct ip *);
		*nextip = *ip;
send:
		/*
		 * Set ip_len and calculate the ip header checksum.
		 */
		ip->ip_len = htons(sizeof (struct ip) + curlen);
#define	ips ((u_short *) ip)
		sum = ips[0] + ips[1] + ips[2] + ips[3] + ips[4] + ips[6] +
			ips[7] + ips[8] + ips[9];

		/* now, handle sum (16 bit) overflows correctly */
		if (sum > 0xffff) {
			sum = (0xffff & sum) + (sum >> 16);
			if (sum > 0xffff)
				sum = (0xffff & sum) + (sum >> 16);
		}
		ip->ip_sum = ~sum;

#undef ips
		/*
		 * At last, we send it off to the ethernet.
		 * Before we send this off to the ethernet
		 * we make local copies of the statistics
		 * that need to be updated. After the packet
		 * has been sent we actually update it.
		 * REASON: The updating requires refrencing
		 * the mbuf, which gets freed once the packet
		 * has been sent.
		 */
		if (ip->ip_off & IP_MF) {
			/* bump up the # of fragments created */
			outtotalfrag++;
			/* The number of packets fragmented should
			 * be bumped only once in the for() loop.
			 * "fragoff" is used to prevent bumping the
			 * counter more than once.
			 */
			if (fragoff == 0) /* a packet was fragmented */
				outpktsfrag++;

			/* This is added to include the last fragment that
			 * is sent. It will not have the IP_MF flag set.
			 */
			 if (!(ip->ip_off & IP_MF) && fragoff != 0) {
				/* bump up the fragmentation count */
				outpktsfrag++;
			 }
		}
		CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
		error = (*ifp->if_output)(ifp, m, dst);
		RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
		if (error) {
			printf("ku_fastsend: transmit failed on %s%d, errno %d\n",
 				ifp->if_name, ifp->if_unit, error);
			if (am)
				m_free(am);
			if (mm)
				m_freem(mm);
			if (error < 0)
				error = EIO;
			return (error);
		} else { /* bump up counters for network management */

			IPSTAT(ips_totalsent++);
			/* check for fragmentation */
			if (outtotalfrag) {
				/* bump up the # of fragments created */
				IPSTAT(ips_outtotalfrag += outtotalfrag);
			}
 			/* check if a packet was fragmented */
			if (outpktsfrag) {
				/* bump up the # of pkts. fragmented */
				IPSTAT(ips_outpktsfrag += outpktsfrag);
			}
		}
				

		if (am == 0) {
			return (0);
		}
		ip = nextip;
		m = dtom(ip);
		mm = NULL;
		fragoff += curlen;
		curlen = 0;
	}
}

#ifdef DEBUG

pr_mbuf(p, m)
	char *p;
	struct mbuf *m;
{
	register char *cp, *cp2;
	register struct ip *ip;
	register int len;

	len = 28;
	printf("%s: ", p);
	if (m && m->m_len >= 20) {
		ip = mtod(m, struct ip *);
		printf("hl %d v %d tos %d len %d id %d mf %d off %d ttl %d p %d sum %d src %x dst %x\n",
			ip->ip_hl, ip->ip_v, ip->ip_tos, ip->ip_len,
			ip->ip_id, ip->ip_off >> 13, ip->ip_off & 0x1fff,
			ip->ip_ttl, ip->ip_p, ip->ip_sum, ip->ip_src.s_addr,
			ip->ip_dst.s_addr);
		len = 0;
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		m = m->m_next;
	} else if (m) {
		printf("pr_mbuf: m_len %d\n", m->m_len);
	} else {
		printf("pr_mbuf: zero m\n");
	}
	while (m) {
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		cp = mtod(m, caddr_t);
		cp2 = cp + m->m_len;
		while (cp < cp2) {
			if (len-- < 0) {
				break;
			}
			printf("%x ", *cp & 0xFF);
			cp++;
		}
		m = m->m_next;
		printf("\n");
	}
}

#endif DEBUG
