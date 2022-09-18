#ifndef lint
static	char	*sccsid = "@(#)if_ether.c	4.4	(ULTRIX)	11/9/90";
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
 *			Modification History				
 *
 *	27-Jul-90 lp
 *	Fix for MP in arpresolve.
 *
 *	9-Jul-90 lp
 *	Cache last arpresolve for small packets - FDDI.
 *
 * 10 Dec 89 -- chet
 *	Add arphasmbuf() routine (for Larry Palmer) that frees
 *	an incomplete arp table entry upon request.
 *
 *	3-Mar-89	U. Sinkewicz
 *		Folded in reverse arp (plus pmax/smp).
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 *	22-Apr-87 - marc
 *		In arpinput(), update an existing entries sender
 *		hardware address always, not just when the entry
 *		is incomplete.  This allows for more dynamic adjusting
 *		of arp translations in an environment where hosts
 *		ethernet addresses can change (like ours).  If a host
 *		changes hardware addresses, and is smart enough to
 *		broadcast it, we will immediately pick up the change.
 *		ARPT_KILLC should probably be bumped back up to 15
 *		minutes, but until this change has a chance to soak
 *		in the field i hesitate to do that.
 *									
 *	08-Jan-87 - lp						
 *		Changed ARPT_KILLC to 5 minutes.	 		
 *									
 *	Larry Cohen  -	09/16/85					
 * 		Add 43bsd alpha tape changes for subnet routing		
 *									
 ************************************************************************/


/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	if_ether.c	7.5 (Berkeley) 10/30/87
 */

/*
 * Ethernet address resolution protocol.
 * TODO:
 *	run at splnet (add ARP protocol intr.)
 *	link entries onto hash chains, keep free list
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/smp_lock.h"
#include "../h/cpudata.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"

#include "../net/net/if.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/if_ether.h"

#ifdef GATEWAY
#define	ARPTAB_BSIZ	16		/* bucket size */
#define	ARPTAB_NB	37		/* number of buckets */
#else
#define	ARPTAB_BSIZ	9		/* bucket size */
#define	ARPTAB_NB	19		/* number of buckets */
#endif
#define	ARPTAB_SIZE	(ARPTAB_BSIZ * ARPTAB_NB)
struct lock_t lk_arptab;		/* SMP lock for arp table */
struct	arptab arptab[ARPTAB_SIZE];
int	arptab_size = ARPTAB_SIZE;	/* for arp command */

/*
 * ARP trailer negotiation.  Trailer protocol is not IP specific,
 * but ARP request/response use IP addresses.
 */
#define ETHERTYPE_IPTRAILERS ETHERTYPE_TRAIL

#define	ARPTAB_HASH(a) \
	((u_long)(a) % ARPTAB_NB)

#define	ARPTAB_LOOK(at,addr) { \
	register int n; \
	at = &arptab[ARPTAB_HASH(addr) * ARPTAB_BSIZ]; \
	for (n = 0 ; n < ARPTAB_BSIZ ; n++,at++) \
		if (at->at_iaddr.s_addr == addr) \
			break; \
	if (n >= ARPTAB_BSIZ) \
		at = 0; \
}
#ifdef RARP
#define RARPTAB_LOOK(at, addr) {\
      register int n; \
      at = &arptab[0]; \
      for (n = 0 ; n < ARPTAB_SIZE ; n++, at++) \
           if (!bcmp((caddr_t)&((at)->at_enaddr[0]), (caddr_t)(addr), \
                      sizeof((at)->at_enaddr))) \
                      break; \
      if (n >= ARPTAB_SIZE) \
              at = 0; \
}
#endif RARP

/* timer values */
#define	ARPT_AGE	(60*1)	/* aging timer, 1 min. */
#define	ARPT_KILLC	20	/* kill completed entry in 20 mins. */
#define	ARPT_KILLI	3	/* kill incomplete entry in 3 minutes */

u_char	etherbroadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
extern struct ifnet loif;
int useloopback = 1;

/*
 * Timeout routine.  Age arp_tab entries once a minute.
 */
arptimer()
{
	register struct arptab *at;
	register int i;
	int s;

	timeout(arptimer, (caddr_t)0, ARPT_AGE * hz);
	s = splimp();
	smp_lock(&lk_arptab, LK_RETRY);
	at = &arptab[0];
	for (i = 0; i < ARPTAB_SIZE; i++, at++) {
		if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
			continue;
		if (++at->at_timer < ((at->at_flags&ATF_COM) ?
		    ARPT_KILLC : ARPT_KILLI))
			continue;
		/* timer has expired, clear entry */
		arptfree(at);
	}
	smp_unlock(&lk_arptab);
	splx(s);
}

/*
 * Check/Clear routine (used by nfs right now) to see if we're waiting
 * for arp resolution. We should only get here when nfs has timed
 * out and found that the mbuf it sent us has still not returned.
 * If we find an incomplete entry for this mbuf we'll blast it.
 */
arphasmbuf(m)
struct mbuf *m;
{
	register struct arptab *at;
	register int i;
	int s;

	s = splimp();
	smp_lock(&lk_arptab, LK_RETRY);
	at = &arptab[0];
	for (i = 0; i < ARPTAB_SIZE; i++, at++) {
		if (at->at_flags & ATF_PERM)
			continue;
		if (at->at_hold == m 
			|| (at->at_hold && at->at_hold->m_next == m)) {
				if ((at->at_flags & ATF_COM) == 0) {
					arptfree(at);
				}
			break;
		}
	}
	smp_unlock(&lk_arptab);
	splx(s);
}

/*
 * Broadcast an ARP packet, asking who has addr on interface ac.
 */
arpwhohas(ac, addr)
	register struct arpcom *ac;
	struct in_addr *addr;
{
	register struct mbuf *m;
	register struct ether_header *eh;
	register struct ether_arp *ea;
	struct sockaddr sa;
	int saveaffinity;

	if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	m->m_len = sizeof *ea;
	m->m_off = MMAXOFF - m->m_len;
	ea = mtod(m, struct ether_arp *);
	eh = (struct ether_header *)sa.sa_data;
	bzero((caddr_t)ea, sizeof (*ea));
	bcopy((caddr_t)etherbroadcastaddr, (caddr_t)eh->ether_dhost,
	    sizeof(eh->ether_dhost));
	eh->ether_type = ETHERTYPE_ARP;		/* if_output will swap */
	/* Should issue type 1 always - for bridges */
	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETHERTYPE_IP);
	ea->arp_hln = sizeof(ea->arp_sha);	/* hardware address length */
	ea->arp_pln = sizeof(ea->arp_spa);	/* protocol address length */
	ea->arp_op = htons(ARPOP_REQUEST);
	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
	   sizeof(ea->arp_sha));
	bcopy((caddr_t)&ac->ac_ipaddr, (caddr_t)ea->arp_spa,
	   sizeof(ea->arp_spa));
	bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa, sizeof(ea->arp_tpa));
	sa.sa_family = AF_UNSPEC;
	CALL_TO_NONSMP_DRIVER( ac->ac_if, saveaffinity);
	(*ac->ac_if.if_output)(&ac->ac_if, m, &sa);
	RETURN_FROM_NONSMP_DRIVER( ac->ac_if, saveaffinity);
}

/*
 * Resolve an IP address into an ethernet address.  If success, 
 * desten is filled in.  If there is no entry in arptab,
 * set one up and broadcast a request for the IP address.
 * Hold onto this mbuf and resend it once the address
 * is finally resolved.  A return value of 1 indicates
 * that desten has been filled in and the packet should be sent
 * normally; a 0 return indicates that the packet has been
 * taken over here, either now or for later transmission.
 *
 * We do some (conservative) locking here at splimp, since
 * arptab is also altered from input interrupt service (ecintr/ilintr
 * calls arpinput when ETHERTYPE_ARP packets come in).
 */
arpresolve(ac, m, destip, desten, usetrailers)
	register struct arpcom *ac;
	struct mbuf *m;
	register struct in_addr *destip;
	register u_char *desten;
	int *usetrailers;
{
	register struct arptab *at;
	static struct arptab *at_cache = 0;
	struct sockaddr_in sin;
	u_long lna;
	int s;

	*usetrailers = 0;

	if(!smp && at_cache && at_cache->at_iaddr.s_addr == destip->s_addr) {
		s = splimp();
		smp_lock(&lk_arptab, LK_RETRY);
		at = at_cache;
		if (at && at->at_flags & ATF_COM) /* entry IS complete */
			goto cached;
		smp_unlock(&lk_arptab);
		splx(s);
	}

	at_cache = 0;
	if (in_broadcast(*destip)) {	/* broadcast address */
		bcopy((caddr_t)etherbroadcastaddr, (caddr_t)desten,
		    sizeof(etherbroadcastaddr));
		return (1);
	}
	lna = in_lnaof(*destip);
	/* if for us, use software loopback driver if up */
	if (destip->s_addr == ac->ac_ipaddr.s_addr) {
		/*
		 * This test used to be
		 *	if (loif.if_flags & IFF_UP)
		 * It allowed local traffic to be forced
		 * through the hardware by configuring the loopback down.
		 * However, it causes problems during network configuration
		 * for boards that can't receive packets they send.
		 * It is now necessary to clear "useloopback"
		 * to force traffic out to the hardware.
		 */
		if (useloopback) {
			sin.sin_family = AF_INET;
			sin.sin_addr = *destip;
			(void) looutput(&loif, m, (struct sockaddr *)&sin);
			/*
			 * The packet has already been sent and freed.
			 */
			return (0);
		} else {
			bcopy((caddr_t)ac->ac_enaddr, (caddr_t)desten,
			    sizeof(ac->ac_enaddr));
			return (1);
		}
	}
	s = splimp();
	smp_lock(&lk_arptab, LK_RETRY);
	ARPTAB_LOOK(at, destip->s_addr);
	if (at == 0) {			/* not found */
		if (ac->ac_if.if_flags & IFF_NOARP) {
			bcopy((caddr_t)ac->ac_enaddr, (caddr_t)desten, 3);
			desten[3] = (lna >> 16) & 0x7f;
			desten[4] = (lna >> 8) & 0xff;
			desten[5] = lna & 0xff;
			smp_unlock(&lk_arptab);
			splx(s);
			return (1);
		} else {
			at = arptnew(destip);
			if (at == 0)
				panic("arpresolve: no free entry");
			at->at_hold = m;
			smp_unlock(&lk_arptab);
			arpwhohas(ac, destip);
			splx(s);
			return (0);
		}
	}
	at->at_timer = 0;		/* restart the timer */
	if (at->at_flags & ATF_COM) {	/* entry IS complete */
cached:
		at->at_timer = 0;
		at_cache = at;
		bcopy((caddr_t)at->at_enaddr, (caddr_t)desten,
		    sizeof(at->at_enaddr));
		if (at->at_flags & ATF_USETRAILERS)
			*usetrailers = BSD_TRAILERS;
		smp_unlock(&lk_arptab);
		splx(s);
		return (1);
	}
	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 */
	if (at->at_hold)
		m_freem(at->at_hold);
	at->at_hold = m;
	smp_unlock(&lk_arptab);
	arpwhohas(ac, destip);		/* ask again */
	splx(s);
	return (0);
}

/*
 * Called from 10 Mb/s Ethernet interrupt handlers
 * when ether packet type ETHERTYPE_ARP
 * is received.  Common length and type checks are done here,
 * then the protocol-specific routine is called.
 * SMP: no smp locks required here
 */
arpinput(ac, m)
	struct arpcom *ac;
	struct mbuf *m;
{
	register struct arphdr *ar;

	if (ac->ac_if.if_flags & IFF_NOARP)
		goto out;
/* #ifdef 43
	IF_ADJ(m);
#endif 43 */
	if (m->m_len < sizeof(struct arphdr))
		goto out;
	ar = mtod(m, struct arphdr *);
	if (ntohs(ar->ar_hrd) != ARPHRD_802 &&
		ntohs(ar->ar_hrd) != ARPHRD_ETHER)
		goto out;
	if (m->m_len < sizeof(struct arphdr) + 2 * ar->ar_hln + 2 * ar->ar_pln)
		goto out;

	switch (ntohs(ar->ar_pro)) {

	case ETHERTYPE_IP:
	case ETHERTYPE_IPTRAILERS:
		in_arpinput(ac, m);
		return;

	default:
		break;
	}
out:
	m_freem(m);
}

/*
 * ARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 826.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 * We also handle negotiations for use of trailer protocol:
 * ARP replies for protocol type ETHERTYPE_TRAIL are sent
 * along with IP replies if we want trailers sent to us,
 * and also send them in response to IP replies.
 * This allows either end to announce the desire to receive
 * trailer packets.
 * We reply to requests for ETHERTYPE_TRAIL protocol as well,
 * but don't normally send requests.
 */
in_arpinput(ac, m)
	register struct arpcom *ac;
	struct mbuf *m;
{
	register struct ether_arp *ea;
	struct ether_header *eh;
	register struct arptab *at;  /* same as "merge" flag */
	struct mbuf *mcopy = 0;
	struct sockaddr_in sin;
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	int proto, op, s, completed = 0;
	int saveaffinity;

	myaddr = ac->ac_ipaddr;
	ea = mtod(m, struct ether_arp *);
	proto = ntohs(ea->arp_pro);
	op = ntohs(ea->arp_op);
	bcopy((caddr_t)ea->arp_spa, (caddr_t)&isaddr, sizeof (isaddr));
	bcopy((caddr_t)ea->arp_tpa, (caddr_t)&itaddr, sizeof (itaddr));
#ifdef RARP
        if (op == RARPOP_REQUEST)
                {
                rarp_reply(ac, m);
                return;
                }
#endif RARP
#ifdef mips
        bcopy(&((struct in_addr *)ea->arp_spa)->s_addr, &isaddr.s_addr,
                sizeof(struct in_addr));
        bcopy(&((struct in_addr *)ea->arp_tpa)->s_addr, &itaddr.s_addr,
                sizeof(struct in_addr));
#endif mips
	if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)ac->ac_enaddr,
	  sizeof (ea->arp_sha)))
		goto out;	/* it's from me, ignore it. */
	if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)etherbroadcastaddr,
	    sizeof (ea->arp_sha))) {
		mprintf("arp: ether address is broadcast for IP address %x!\n",
		    ntohl(isaddr.s_addr));
		goto out;
	}
	if (isaddr.s_addr == myaddr.s_addr) {
		mprintf("%s: %s\n",
			"duplicate IP address!! sent from ethernet address",
			ether_sprintf(ea->arp_sha));
		itaddr = myaddr;
		if (op == ARPOP_REQUEST)
			goto reply;
		goto out;
	}
	s = splimp();
	smp_lock(&lk_arptab, LK_RETRY);
	ARPTAB_LOOK(at, isaddr.s_addr);
	if (at) {
		bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
		    sizeof(ea->arp_sha));
		if ((at->at_flags & ATF_COM) == 0)
			completed = 1;
		at->at_flags |= ATF_COM;
		if (at->at_hold) {
			sin.sin_family = AF_INET;
			sin.sin_addr = isaddr;
			/* cannot hold smp lock when calling driver output */
			smp_unlock(&lk_arptab);
			CALL_TO_NONSMP_DRIVER( ac->ac_if, saveaffinity);
			(*ac->ac_if.if_output)(&ac->ac_if, 
			    at->at_hold, (struct sockaddr *)&sin);
			RETURN_FROM_NONSMP_DRIVER( ac->ac_if, saveaffinity);
			smp_lock(&lk_arptab, LK_RETRY);
			at->at_hold = 0;
		}
	}
	if (at == 0 && itaddr.s_addr == myaddr.s_addr) {
		/* ensure we have a table entry */
		if (at = arptnew(&isaddr)) {
			bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
				sizeof(ea->arp_sha));
			completed = 1;
			at->at_flags |= ATF_COM;
		}
	}
	smp_unlock(&lk_arptab);
	splx(s);
reply:
	switch (proto) {

	case ETHERTYPE_IPTRAILERS:
		/* partner says trailers are OK */
		if (at)
				at->at_flags |= ATF_USETRAILERS;
		/*
		 * Reply to request iff we want trailers.
		 */
		if (op != ARPOP_REQUEST || ac->ac_if.if_flags & IFF_NOTRAILERS)
			goto out;
		break;

	case ETHERTYPE_IP:
		/*
		 * Reply if this is an IP request,
		 * or if we want to send a trailer response.
		 * Send the latter only to the IP response
		 * that completes the current ARP entry.
		 */
		if (op != ARPOP_REQUEST &&
		    (completed == 0 || ac->ac_if.if_flags & IFF_NOTRAILERS))
			goto out;
	}
#if defined(GATEWAY) && defined(mips)
        /*
         *      Do "promiscuous arp".  Ie., reply with my hardware address for
         *      any machine not on the requestor's net, but on one of mine.
         *      I will then deal with delivery of the packet.
         */
        if (in_netof(itaddr) != in_netof(ea->arp_spa))
                if (arp_cangate(itaddr) && arp_do_prom) {
                        bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
                            sizeof(ea->arp_sha));
                        bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
                            sizeof(ea->arp_sha));
                        goto send;
                }
#endif GATEWAY && mips
	if (itaddr.s_addr == myaddr.s_addr) {
		/* I am the target */
		bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
		    sizeof(ea->arp_sha));
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
		    sizeof(ea->arp_sha));
	} else {
		s = splimp(); /* SMP */
		smp_lock(&lk_arptab, LK_RETRY);
		ARPTAB_LOOK(at, itaddr.s_addr);
		if (at == NULL || (at->at_flags & ATF_PUBL) == 0) {
			smp_unlock(&lk_arptab);
			splx(s); /* SMP */
			goto out;
		}
		bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
		    sizeof(ea->arp_sha));
		bcopy((caddr_t)at->at_enaddr, (caddr_t)ea->arp_sha,
		    sizeof(ea->arp_sha));
		smp_unlock(&lk_arptab);
		splx(s); /* SMP */
	}

	bcopy((caddr_t)ea->arp_spa, (caddr_t)ea->arp_tpa,
	    sizeof(ea->arp_spa));
	bcopy((caddr_t)&itaddr, (caddr_t)ea->arp_spa,
	    sizeof(ea->arp_spa));
	ea->arp_op = htons(ARPOP_REPLY); 
	/*
	 * If incoming packet was an IP reply,
	 * we are sending a reply for type IPTRAILERS.
	 * If we are sending a reply for type IP
	 * and we want to receive trailers,
	 * send a trailer reply as well.
	 */
	if (op == ARPOP_REPLY)
		ea->arp_pro = htons(ETHERTYPE_IPTRAILERS);
	else if (proto == ETHERTYPE_IP &&
	    (ac->ac_if.if_flags & IFF_NOTRAILERS) == 0)
		mcopy = m_copy(m, 0, (int)M_COPYALL);
	eh = (struct ether_header *)sa.sa_data;
	bcopy((caddr_t)ea->arp_tha, (caddr_t)eh->ether_dhost,
	    sizeof(eh->ether_dhost));
	eh->ether_type = ETHERTYPE_ARP;
	sa.sa_family = AF_UNSPEC;
	CALL_TO_NONSMP_DRIVER( ac->ac_if, saveaffinity);
	(*ac->ac_if.if_output)(&ac->ac_if, m, &sa);
	RETURN_FROM_NONSMP_DRIVER( ac->ac_if, saveaffinity);
	if (mcopy) {
		ea = mtod(mcopy, struct ether_arp *);
		ea->arp_pro = htons(ETHERTYPE_IPTRAILERS);
		CALL_TO_NONSMP_DRIVER( ac->ac_if, saveaffinity);
		(*ac->ac_if.if_output)(&ac->ac_if, mcopy, &sa);
		RETURN_FROM_NONSMP_DRIVER( ac->ac_if, saveaffinity);
	}
	return;
out:
	m_freem(m);
	return;
}

/*
 * Free an arptab entry.
 * SMP: arptfree must be called with arptab lock asserted
 */
arptfree(at)
	register struct arptab *at;
{
	int s = splimp();

	if (at->at_hold)
		m_freem(at->at_hold);
	at->at_hold = 0;
	at->at_timer = at->at_flags = 0;
	at->at_iaddr.s_addr = 0;
	splx(s);
}

/*
 * Enter a new address in arptab, pushing out the oldest entry 
 * from the bucket if there is no room.
 * This always succeeds since no bucket can be completely filled
 * with permanent entries (except from arpioctl when testing whether
 * another permanent entry will fit).
 * MUST BE CALLED AT SPLIMP.
 * SMP: arptnew must be called with arptab lock asserted
 */
struct arptab *
arptnew(addr)
	struct in_addr *addr;
{
	register int n;
	int oldest = -1;
	register struct arptab *at, *ato = NULL;
	static int first = 1;

	if (first) {
		first = 0;
		timeout(arptimer, (caddr_t)0, hz);
	}
	at = &arptab[ARPTAB_HASH(addr->s_addr) * ARPTAB_BSIZ];
	for (n = 0; n < ARPTAB_BSIZ; n++,at++) {
		if (at->at_flags == 0)
			goto out;	 /* found an empty entry */
		if (at->at_flags & ATF_PERM)
			continue;
		if ((int) at->at_timer > oldest) {
			oldest = at->at_timer;
			ato = at;
		}
	}
	if (ato == NULL)
		return (NULL);
	at = ato;
	arptfree(at);
out:
	at->at_iaddr = *addr;
	at->at_flags = ATF_INUSE;
	return (at);
}

arpioctl(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct arpreq *ar = (struct arpreq *)data;
	register struct arptab *at;
	register struct sockaddr_in *sin;
	int s;

	if (ar->arp_pa.sa_family != AF_INET ||
	    ar->arp_ha.sa_family != AF_UNSPEC)
		return (EAFNOSUPPORT);
	sin = (struct sockaddr_in *)&ar->arp_pa;
	s = splimp();
	smp_lock(&lk_arptab, LK_RETRY);
	ARPTAB_LOOK(at, sin->sin_addr.s_addr);
	if (at == NULL) {		/* not found */
		if (cmd != SIOCSARP) {
			smp_unlock(&lk_arptab);
			splx(s);
			return (ENXIO);
		}
		smp_unlock(&lk_arptab);		
		if (ifa_ifwithnet(&ar->arp_pa) == NULL) {
			splx(s);
			return (ENETUNREACH);
		}
		smp_lock(&lk_arptab, LK_RETRY);
	}
	switch (cmd) {

	case SIOCSARP:		/* set entry */
		if (at == NULL) {
			at = arptnew(&sin->sin_addr);
			if (at == NULL) {
				smp_unlock(&lk_arptab);
				splx(s);
				return (EADDRNOTAVAIL);
			}
			if (ar->arp_flags & ATF_PERM) {
			/* never make all entries in a bucket permanent */
				register struct arptab *tat;
				
				/* try to re-allocate */
				tat = arptnew(&sin->sin_addr);
				if (tat == NULL) {
					arptfree(at);
					smp_unlock(&lk_arptab);
					splx(s);
					return (EADDRNOTAVAIL);
				}
				arptfree(tat);
			}
		}
		bcopy((caddr_t)ar->arp_ha.sa_data, (caddr_t)at->at_enaddr,
		    sizeof(at->at_enaddr));
		at->at_flags = ATF_COM | ATF_INUSE |
			(ar->arp_flags & (ATF_PERM|ATF_PUBL|ATF_USETRAILERS));
		at->at_timer = 0;
		break;

	case SIOCDARP:		/* delete entry */
		arptfree(at);
		break;

	case SIOCGARP:		/* get entry */
		bcopy((caddr_t)at->at_enaddr, (caddr_t)ar->arp_ha.sa_data,
		    sizeof(at->at_enaddr));
		ar->arp_flags = at->at_flags;
		break;
	}
	smp_unlock(&lk_arptab);
	splx(s);
	return (0);
}

/*
 * Convert Ethernet address to printable (loggable) representation.
 */
char *
ether_sprintf(ap)
	register u_char *ap;
{
	register int i;
	static char etherbuf[18];
	register char *cp = etherbuf;
	static char digits[] = "0123456789abcdef";

	for (i = 0; i < 6; i++) {
		*cp++ = digits[*ap >> 4];
		*cp++ = digits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = 0;
	return (etherbuf);
}
#ifdef RARP
/*
 * When the rarp packet is a request (arp_op = RARPOP_REQUEST = 3), then
 *      arp_sha is the ethernet address of the sender of the request;
 *      arp_spa is undefined;
 *      arp_tha is the 'target' hardware address, i.e. the sender's address,
 *      arp_tpa is undefined.
 * The rarp reply (arp_op = RARPOP_REPLY = 4) looks like:
 *      arp_sha is the responder's (our) ethernet address;
 *      arp_spa is the responder's (our) IP address;
 *      arp_tha is identical to the request packet;
 *      arp_tpa is the request's desired IP address.
 */

rarp_reply(ac, m)
        register struct arpcom *ac;
        register struct mbuf *m;
{
        register struct ether_arp *ea;
        register struct arptab *him;  /* same as "merge" flag */
        struct ether_header *eh;
        struct sockaddr sa;

        ea = mtod(m, struct ether_arp *);
        RARPTAB_LOOK(him, ea->arp_tha);
        if (him)
                {
                bcopy((caddr_t)&ac->ac_ipaddr,ea->arp_spa,sizeof(ea->arp_spa));
                bcopy(ac->ac_enaddr, ea->arp_sha, sizeof(ea->arp_sha));
                bcopy((caddr_t)&him->at_iaddr,ea->arp_tpa,sizeof(ea->arp_tpa));
                ea->arp_op = htons(RARPOP_REPLY);
                eh = (struct ether_header *)sa.sa_data;
                bcopy((caddr_t)ea->arp_tha, (caddr_t)eh->ether_dhost,
                    sizeof(eh->ether_dhost));
                bcopy((caddr_t)ea->arp_sha, (caddr_t)eh->ether_shost,
                    sizeof(eh->ether_shost));
                eh->ether_type = ETHERTYPE_RARP;
                sa.sa_family = AF_UNSPEC;
                (*ac->ac_if.if_output)(&ac->ac_if, m, &sa);
                } /* if at, ie. can reply */
        return;
} /* rarp_reply() */
#endif RARP
#if defined(GATEWAY) && defined(mips)
/*
 *      Can we gateway to this network?  We already know that this
 *      target is not on the network that the ARP request was made on,
 *      so if we can talk to this network, we can gateway between the
 *      two.
 */

arp_cangate(target)
        struct in_addr target;
{
        register struct in_ifaddr *ia;
	int s;
        
	s = splimp();
        for (ia = in_ifaddr; ia; ia = ia->ia_next)
                if (in_netof(target) == ia->ia_net){
			splx(s);
                        return (1);
		}
	splx(s);
        return (0);
}
#endif GATEWAY && mips
