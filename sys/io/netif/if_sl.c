#ifndef lint
static	char	*sccsid = "@(#)if_sl.c	4.3		(ULTRIX)		2/1/91";
#endif lint
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Based on -
 *	Van Jacobson (van@helios.ee.lbl.gov), Dec 31, 1989:
 *	- Initial distribution.
 *
 * As modified for Ultrix V4.0 by -
 *      Tim Theisen             Department of Computer Sciences
 *      tim@cs.wisc.edu         University of Wisconsin-Madison
 *      uwvax!tim               1210 West Dayton Street
 *      (608)262-0438           Madison, WI   53706
 *
 * And finally the NSL version is by -
 *      Michael Reilly          Network Systems Lab
 *      reilly@nsl.dec.com      Digital Equipment corporation
 *                              505 Hamilton Avenue
 *                              Palo Alto, CA  94301
 *
 * For Ultrix V3.1 or V4.0 on a mips based DECstation
 *
 */

/*
 * NOTE:  You must define either ULTRIX_3 for an Ultrix V3.1 driver or
 *                               ULTRIX_4 for an Ultrix V4.0 driver
 */

/*
 * Serial Line interface
 *
 * Originally written by
 * 	Rick Adams
 * 	Center for Seismic Studies
 * 	1300 N 17th Street, Suite 1450
 * 	Arlington, Virginia 22209
 * 	(703)276-7900
 * 	rick@seismo.css.gov
 * 	seismo!rick
 *
 * Pounded on heavily by Chris Torek (chris@mimsy.umd.edu, umcp-cs!chris).
 * N.B.: this belongs in netinet, not net, the way it stands now.
 * Should have a link-layer type designation, but wouldn't be
 * backwards-compatible.
 *
 * Converted to 4.3BSD Beta by Chris Torek.
 * Other changes made at Berkeley, based in part on code by Kirk Smith.
 *
 * Hacked almost beyond recognition by Van Jacobson (van@helios.ee.lbl.gov).
 * Added priority queuing for "interactive" traffic; hooks for TCP
 * header compression; ICMP filtering (at 2400 baud, some cretin
 * pinging you can use up all your bandwidth); conditionals for Sun
 * OS 3.x in addition to 4.x BSD.  Made low clist behavior more robust
 * and slightly less likely to hang serial line.  Sped up a bunch of
 * things.
 */

/* originally from if_sl.c,v 1.11 84/10/04 12:54:47 rick Exp */
/* And from if_sl.c,v 1.12 90/08/20 14:24:28 tim Exp */

#define ULTRIX_4

#ifdef ultrix
#if !defined(ULTRIX_3) && !defined(ULTRIX_4)
#include "Either option ULTRIX_3 or ULTRIX_4 must be defined"
#endif
#endif

#include "sl.h"
#if NSL > 0

#if defined(ultrix) && defined(ULTRIX_3)
#include "../data/if_sl_data.c"
#endif

#if defined(ultrix) && defined(ULTRIX_4)
#include "../../data/if_sl_data.c"
#endif

#ifdef ultrix
#if defined(ULTRIX_3) && defined(ULTRIX_4)
#include "Error - both ULTRIX_3 and ULTRIX_4 are defined"
#endif
#endif

/*
 * The following are patchable defaults for the three options
 * in the interface flags word.  If desired, they should be set
 * by config file options SL_DOCOMPRESS, SL_ALLOWCOMPRESS and
 * SL_NOICMP.
 *
 *   sl_docompress	If = 1, compression for a line will default to "on"
 *
 *   sl_allowcompres	If = 1, compression for a line will default to "off"
 *			but will be turned on if a compressed packet is
 *			received. 
 *
 *   sl_noicmp		If = 1, outbound ICMP packets will be discarded.
 *			XXX - shouldn't have to set this but some cretin
 *			pinging us can drive our throughput to zero (not
 *			to mention the raft of quenches we'll get if we're
 *			unlucky enough to have to traverse the milnet.
 */
#ifndef SL_DOCOMPRESS
#define SL_DOCOMPRESS 0
#endif
#ifndef SL_ALLOWCOMPRESS
#define SL_ALLOWCOMPRESS 0
#endif
#ifndef SL_NOICMP
#define SL_NOICMP 0
#endif
int sl_docompress = SL_DOCOMPRESS;
int sl_allowcompress = SL_ALLOWCOMPRESS;
int sl_noicmp = SL_NOICMP;

/* Compatibility with 4.2 BSD (and variants) */
#ifndef MCLBYTES
#ifdef ultrix
#define MCLBYTES M_CLUSTERSZ
#else
#define MCLBYTES CLBYTES
#endif
#endif
/*
 * SLMAX is a hard limit on input packet size.  To simplify the code
 * and improve performance, we require that packets fit in an mbuf
 * cluster, that there be enough extra room for the ifnet pointer that
 * IP input requires and, if we get a compressed packet, there's
 * enough extra room to expand the header into a max length tcp/ip
 * header (128 bytes).  So, SLMAX can be at most
 *	MCLBYTES - sizeof(struct ifnet *) - 128
 *
 * SLMTU is a hard limit on output packet size.  To insure good
 * interactive response, SLMTU wants to be the smallest size that
 * amortizes the header cost.  (Remember that even with
 * type-of-service queuing, we have to wait for any in-progress
 * packet to finish.  I.e., we wait, on the average, 1/2 * mtu /
 * cps, where cps is the line speed in characters per second.
 * E.g., 533ms wait for a 1024 byte MTU on a 9600 baud line.  The
 * average compressed header size is 6-8 bytes so any MTU > 90
 * bytes will give us 90% of the line bandwidth.  A 100ms wait is
 * tolerable (500ms is not), so want an MTU around 296.  (Since TCP
 * will send 256 byte segments (to allow for 40 byte headers), the
 * typical packet size on the wire will be around 260 bytes).  In
 * 4.3tahoe+ systems, we can set an MTU in a route so we do that &
 * leave the interface MTU relatively high (so we don't IP fragment
 * when acting as a gateway to someone using a stupid MTU).
 *
 * Similar considerations apply to SLIP_HIWAT:  It's the amount of
 * data that will be queued 'downstream' of us (i.e., in clists
 * waiting to be picked up by the tty output interrupt).  If we
 * queue a lot of data downstream, it's immune to our t.o.s. queuing.
 * E.g., if SLIP_HIWAT is 1024, the interactive traffic in mixed
 * telnet/ftp will see a 1 sec wait, independent of the mtu (the
 * wait is dependent on the ftp window size but that's typically
 * 1k - 4k).  So, we want SLIP_HIWAT just big enough to amortize
 * the cost (in idle time on the wire) of the tty driver running
 * off the end of its clists & having to call back slstart for a
 * new packet.  For a tty interface with any buffering at all, this
 * cost will be zero.  Even with a totally brain dead interface (like
 * the one on a typical workstation), the cost will be <= 1 character
 * time.  So, setting SLIP_HIWAT to ~100 guarantees that we'll lose
 * at most 1% while maintaining good interactive response.
 */
#define BUFOFFSET (128+sizeof(struct ifnet **))
#define	SLMAX	(MCLBYTES - BUFOFFSET)
#define	SLMTU	296
#define	SLIP_HIWAT	roundup(50,CBSIZE)
#define	CLISTRESERVE	1024	/* Can't let clists get too low */

/*
 * The following disgusting hack gets around the problem that IP TOS
 * can't be set in BSD/Sun OS yet.  We want to put "interactive"
 * traffic on a high priority queue.  To decide if traffic is
 * interactive, we check that a) it is TCP and b) one of it's ports
 * if telnet, rlogin or ftp control.
 */
static u_short interactive_ports[8] = {
	0,	513,	0,	0,
	0,	21,	0,	23,
};
#define INTERACTIVE(p) (interactive_ports[(p) & 7] == (p))

#define FRAME_END		0xc0		/* Frame End */
#define FRAME_ESCAPE		0xdb		/* Frame Esc */
#define TRANS_FRAME_END		0xdc		/* transposed frame end */
#define TRANS_FRAME_ESCAPE	0xdd		/* transposed frame esc */

#ifdef sun
#define t_sc t_linep
#else
#define t_sc T_LINEP
#endif

#if defined(SL_NIT) && defined(NIT)
#include "../h/time.h"
#include "../net/nit.h"
#include "../netinet/if_ether.h"
extern struct nit_cb nitcb;
int sl_donit = 0;
#endif

/* Sun OS3.x doesn't have an MCLGET or MCLFREE.  Sun OS4.x doesn't
 * have an MCLFREE.  The code below should work on either Sun OS
 * (this driver, however will *NOT* work on a stock Sun OS4 system
 * because of the slow, broken, worthless, System-V "streams" tty
 * driver).
 *
 * All BSD systems since 4.1c have both MCLGET and MCLFREE.
 * However 4.2bsd had a second, unused parameter to MCLGET.  Some
 * versions of cpp may complain if you compile this routine under
 * a stock 4.2bsd.  For reasons I have never understood, various
 * vendors chose to break BSD in mysterious ways (e.g., DEC's Ultrix
 * changed the definition of m_off to make mbuf cluster use
 * a factor of two more costly -- no doubt this was done to encourage
 * the sale of faster processors since there is no technical
 * justification whatsoever for the change).  If you are stuck with
 * one of these abortions and, for reasons best known to yourself,
 * don't want to upgrade to the BSD network code freely available via
 * anonymous ftp from ucbarpa.berkeley.edu, the following macros
 * might give you a prototype to work from.  Then again, they might
 * not.  Good luck.
 */
#ifdef sun
#ifndef MCLGET
#define MCLGET(m) { int ms = splimp(); (void)mclget(m); (void) splx(ms); }
#endif
#ifndef MCLFREE
#define	MCLFREE(p) { \
	extern char *mclrefcnt; \
	struct mbuf mxxx; \
	mxxx.m_len = MCLBYTES; \
	mxxx.m_off = (int)p - (int)xm; \
	mxxx.m_cltype = 1; \
	mclput(&mxxx); \
	}
#endif
#endif

#ifdef ultrix
/* Ultrix 3.x on Vaxen misdefines the MCLGET macro to cause the m_len field
 * to be set to half of the amount actually allocated.  Thus this re-
 * definition is necessary.
 */

#if defined(vax) && defined(ULTRIX_3)
#undef MCLGET
#define MCLGET(m, p) \
        KM_ALLOC((p), struct mbuf *, M_CLUSTERSZ, KM_CLUSTER, KM_NOWAIT); \
        if((p)) { \
                (m)->m_cltype = M_CLTYPE1; (m)->m_clptr = (caddr_t) p; \
                m->m_off = (int)(p); m->m_len = M_CLUSTERSZ; \
                mbstat.m_clfree++; mbstat.m_clusters++; \
        }
#endif defined(vax) && defined(ULTRIX_3)

#ifndef MCLFREE
#define	MCLFREE(p) { \
	KM_FREE((p), KM_CLUSTER); \
	mbstat.m_clusters--; mbstat.m_clfree--; \
	}
#endif
#endif

int sloutput(), slioctl();

/*
 * Called from boot code to establish sl interfaces.
 */
slattach()
{
	register struct sl_softc *sc;
	register int i = 0;

#if defined(ultrix) && defined(ULTRIX_4)
	int s;
#endif

	if (sl_softc[0].sc_if.if_name == NULL) {
		for (sc = sl_softc; i < nNSL; sc++) {
			sc->sc_if.if_name = "sl";
			sc->sc_if.if_unit = i++;
			sc->sc_if.if_mtu = SLMTU;
			sc->sc_if.if_flags = IFF_POINTOPOINT;
			sc->sc_if.if_ioctl = slioctl;
			sc->sc_if.if_output = sloutput;
			sc->sc_if.if_snd.ifq_maxlen = 50;
			sc->sc_fastq.ifq_maxlen = 32;
#if defined(ultrix) && defined(ULTRIX_4)
			s = splnet();	/* SMP */
			if_attach(&sc->sc_if);
			splx(s);
#else
			if_attach(&sc->sc_if);
#endif
		}
	}
}

static int
slinit(sc)
	register struct sl_softc *sc;
{

	if (sc->sc_ep == (u_char *) 0) {
		struct mbuf m;

#ifdef ultrix
		struct mbuf *p;
		MCLGET((&m),p);
#else
		MCLGET((&m));
#endif
		if (m.m_len == MCLBYTES)
			sc->sc_ep = mtod(&m,u_char *) + (BUFOFFSET + SLMAX);
		else {
			printf("sl%d: can't allocate buffer\n", sc - sl_softc);
			sc->sc_if.if_flags &= ~IFF_UP;
			return (0);
		}
	}
	sc->sc_buf = sc->sc_ep - SLMAX;
	sc->sc_mp = sc->sc_buf;
	if (sl_docompress)
		sc->sc_if.if_flags |= IFF_D1;
	else 
		sc->sc_if.if_flags &=~ IFF_D1;
	if (sl_allowcompress)
		sc->sc_if.if_flags |= IFF_D2;
	else 
		sc->sc_if.if_flags &=~ IFF_D2;
	if (sl_noicmp)
		sc->sc_if.if_flags |= IFF_D3;
	else 
		sc->sc_if.if_flags &=~ IFF_D3;
	sl_compress_init(&sc->sc_comp);
	return (1);
}

/*
 * Line specific open routine.
 * Attach the given tty to the first available sl unit.
 */
/* ARGSUSED */
slopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct sl_softc *sc;
	register int nsl;

	if (!suser())
		return (EPERM);

#ifdef ultrix
	slattach();
#define SLIPDISC SLPDISC
#endif

	if (tp->t_line == SLIPDISC)
		return (EBUSY);

	for (nsl = nNSL, sc = sl_softc; --nsl >= 0; sc++)
		if (sc->sc_ttyp == NULL) {
			if (slinit(sc) == 0)
				return (ENOBUFS);
			tp->t_sc = (caddr_t)sc;
			sc->sc_ttyp = tp;
			ttyflush(tp, FREAD | FWRITE);
			return (0);
		}
	return (ENXIO);
}

#ifdef sun
/* XXX - Sun OS3 is missing these 4bsd routines.
 *
 * Mark an interface down and notify protocols of
 * the transition.
 */
static void
if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifqueue *ifq = &ifp->if_snd;
	register struct mbuf *m, *n;

	ifp->if_flags &=~ IFF_UP;
	pfctlinput(PRC_IFDOWN, &ifp->if_addr);

	/* Flush the interface queue. */
	n = ifq->ifq_head;
	while (m = n) {
		n = m->m_act;
		m_freem(m);
	}
	ifq->ifq_head = 0;
	ifq->ifq_tail = 0;
	ifq->ifq_len = 0;
}

static void
if_rtdelete(ifp)
	register struct ifnet *ifp;
{
	static struct rtentry route;

	if (ifp->if_flags & IFF_ROUTE) {
		route.rt_dst = ifp->if_dstaddr;
		route.rt_gateway = ifp->if_addr;
		route.rt_flags = RTF_HOST|RTF_UP;
		(void) rtrequest(SIOCDELRT, &route);
		ifp->if_flags &=~ IFF_ROUTE;
	}
}
#endif

/*
 * Line specific close routine.
 * Detach the tty from the sl unit.
 * Mimics part of ttyclose().
 */
slclose(tp)
	struct tty *tp;
{
	register struct sl_softc *sc;
	int s;

	ttywflush(tp);
	tp->t_line = 0;
	s = splimp();
	sc = (struct sl_softc *)tp->t_sc;
	if (sc != NULL) {

#ifdef sun
		if_rtdelete(&sc->sc_if);
#endif

		if_down(&sc->sc_if);
		rtpurge(&sc->sc_if);
		in_reminterface(&sc->sc_if);
		sc->sc_if.if_addrlist = NULL;
		sc->sc_ttyp = NULL;
		tp->t_sc = NULL;
		MCLFREE((struct mbuf *)(sc->sc_ep - (SLMAX + BUFOFFSET)));
		sc->sc_ep = 0;
		sc->sc_mp = 0;
		sc->sc_buf = 0;
	}
	splx(s);
}

/*
 * Line specific (tty) ioctl routine.
 * Provide a way to get the sl unit number.
 */
/* ARGSUSED */
sltioctl(tp, cmd, data, flag)
	struct tty *tp;
	caddr_t data;
{

	if (cmd == TIOCGETD) {
		*(int *)data = ((struct sl_softc *)tp->t_sc)->sc_if.if_unit;
		return (0);
	}
	return (EINVAL);
}

/*
 * Queue a packet.  Start transmission if not active.
 */
sloutput(ifp, m, dst)
	register struct ifnet *ifp;
	register struct mbuf *m;
	struct sockaddr *dst;
{
	register struct sl_softc *sc;
	register struct ip *ip;
	register struct ifqueue *ifq;
	int s;

	/*
	 * `Cannot happen' (see slioctl).  Someday we will extend
	 * the line protocol to support other address families.
	 */
	if (dst->sa_family != AF_INET) {
		printf("sl%d: af%d not supported\n", ifp->if_unit,
			dst->sa_family);
		m_freem(m);
		return (EAFNOSUPPORT);
	}

	sc = &sl_softc[ifp->if_unit];
	if (sc->sc_ttyp == NULL) {
		m_freem(m);
		return (ENETDOWN);	/* sort of */
	}
	if ((sc->sc_ttyp->t_state & TS_CARR_ON) == 0) {
		m_freem(m);
		return (EHOSTUNREACH);
	}
	
	ifq = &ifp->if_snd;
	if ((ip = mtod(m, struct ip *))->ip_p == IPPROTO_TCP) {
		register int p = -1;
		if (m->m_len > sizeof(struct ip))
			p = ((int *)ip)[ip->ip_hl];

		if (INTERACTIVE(p & 0xffff) || INTERACTIVE(p >> 16)) {
			ifq = &sc->sc_fastq;
			p = 1;
		} else
			p = 0;

		if (sc->sc_if.if_flags & IFF_D1) {
			/*
			 * The last parameter turns off connection id
			 * compression for background traffic:  Since
			 * fastq traffic can jump ahead of the background
			 * traffic, we don't know what order packets will
			 * go on the line.
			 */
		  
			p = sl_compress_tcp(m, ip, &sc->sc_comp, p);
			*mtod(m, u_char *) |= p;
		}
	} else if ((sc->sc_if.if_flags & IFF_D3) && ip->ip_p == IPPROTO_ICMP) {
		m_freem(m);
		return (0);
	}
	s = splimp();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		m_freem(m);
		splx(s);
		sc->sc_if.if_oerrors++;
		return (ENOBUFS);
	}
	IF_ENQUEUE(ifq, m);
	if (sc->sc_ttyp->t_outq.c_cc == 0) {
	  splx(s);
	  slstart(sc->sc_ttyp);
	} else
	    splx(s);

	return (0);
}

/*
 * Start output on interface.  Get another datagram
 * to send from the interface queue and map it to
 * the interface before starting output.
 */
slstart(tp)
	register struct tty *tp;
{
	register struct sl_softc *sc = (struct sl_softc *)tp->t_sc;
	register struct mbuf *m;
	register u_char *cp;
	int s;
	struct mbuf *m2;

#if !defined(ultrix) || defined(ULTRIX_3)
	extern int cfreecount;
#endif
	
	for (;;) {
		/*
		 * If there is more in the output queue, just send it now.
		 * We are being called in lieu of ttstart and must do what
		 * it would.
		 */
#ifdef vax
	  s=splimp();
#else
#ifdef mips
	  s=spltty();  /* Mips requires higher IPL than Vax */
#endif mips
#endif vax

		if (tp->t_outq.c_cc != 0) {
		  (*tp->t_oproc)(tp);
		  if (tp->t_outq.c_cc > SLIP_HIWAT) {
		    splx(s);
		    return;
		  }
		}
	  
		/*
		 * This happens briefly when the line shuts down.
		 */
	  if (sc == NULL) {
	    splx(s);
	    return;
	  }
	  
		/*
		 * Get a packet and send it to the interface.
		 */

		IF_DEQUEUE(&sc->sc_fastq, m);
		if (m == NULL)
			IF_DEQUEUE(&sc->sc_if.if_snd, m);

#if defined(SL_NIT) && defined(NIT)
		if (m && sl_donit && nitcb.ncb_next != &nitcb) {
			/* do nit processing if there's anyone listening */
			static struct ether_header oheader = { { 1 }, { 2 } };
			struct nit_ii nii;
			int len = 0;

			m2 = m;
			do {
				len += m2->m_len;
			} while (m2 = m2->m_next);

			oheader.ether_type = *mtod(m, u_char *);
			nii.nii_header = (caddr_t)&oheader;
			nii.nii_hdrlen = sizeof(oheader);
			nii.nii_type = oheader.ether_type;
			nii.nii_datalen = len;
			nii.nii_promisc = 0;
			nit_tap(&sc->sc_if, m, &nii);
		}
#endif

		splx(s);
		if (m == NULL)
			return;
		/*
		 * If system is getting low on clists, just flush our
		 * output queue (if the stuff was important, it'll get
		 * retransmitted).
		 */

#if !defined(ultrix) || defined(ULTIX_3)
		if (cfreecount < CLISTRESERVE + SLMTU) {
			m_freem(m);
			sc->sc_if.if_collisions++;
			continue;
		}
#endif

		/*
		 * The extra FRAME_END will start up a new packet, and thus
		 * will flush any accumulated garbage.  We do this whenever
		 * the line may have been idle for some time.
		 */
		
		if (tp->t_outq.c_cc == 0) {
			++sc->sc_bytessent;
			(void) putc(FRAME_END, &tp->t_outq);
		}

		while (m) {
			register u_char *ep;

			cp = mtod(m, u_char *); ep = cp + m->m_len;
			while (cp < ep) {
				/*
				 * Find out how many bytes in the string we can
				 * handle without doing something special.
				 */
				register u_char *bp = cp;

				while (cp < ep) {
					switch (*cp++) {
					case FRAME_ESCAPE:
					case FRAME_END:
						--cp;
						goto out;
					}
				}
				out:
				if (cp > bp) {
					/*
					 * Put n characters at once
					 * into the tty output queue.
					 */
					if (b_to_q((char *)bp, cp - bp, &tp->t_outq))
						break;
					sc->sc_bytessent += cp - bp;
				}
				/*
				 * If there are characters left in the mbuf,
				 * the first one must be special..
				 * Put it out in a different form.
				 */
				if (cp < ep) {
					if (putc(FRAME_ESCAPE, &tp->t_outq))
						break;
					if (putc(*cp++ == FRAME_ESCAPE ?
					   TRANS_FRAME_ESCAPE : TRANS_FRAME_END,
					   &tp->t_outq)) {
						(void) unputc(&tp->t_outq);
						break;
					}
					sc->sc_bytessent += 2;
				}
			}
			MFREE(m, m2);
			m = m2;
		}

		if (putc(FRAME_END, &tp->t_outq)) {
			/*
			 * Not enough room.  Remove a char to make room
			 * and end the packet normally.
			 * If you get many collisions (more than one or two
			 * a day) you probably do not have enough clists
			 * and you should increase "nclist" in param.c.
			 */
			(void) unputc(&tp->t_outq);
			(void) putc(FRAME_END, &tp->t_outq);
			sc->sc_if.if_collisions++;
		} else {
			++sc->sc_bytessent;
			sc->sc_if.if_opackets++;
		}
	}
}

/*
 * Copy data buffer to mbuf chain; add ifnet pointer.
 */
static struct mbuf *
sl_btom(sc, len)
	register struct sl_softc *sc;
	register int len;
{
	register u_char *cp;
	register struct mbuf *m;

	MGET(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (NULL);

	/*
	 * If we have more than MLEN bytes, it's cheaper to
	 * queue the cluster we just filled & allocate a new one
	 * for the input buffer.  Otherwise, fill the mbuf we
	 * allocated above.  Note that code in the input routine
	 * guarantees that packet + ifp will fit in a cluster and
	 * initial setup left room for interface pointer.
	 */
	cp = sc->sc_buf;

#if BSD==43
	if (len >= MLEN - sizeof(struct ifnet *)) {
		MCLGET(m);
		if (m->m_len != MCLBYTES) {
			/* we couldn't get a cluster - if memory's this
			 * low, it's time to start dropping packets. */
			m_freem(m);
			return (NULL);
		}
		sc->sc_ep = mtod(m, u_char *) + (BUFOFFSET + SLMAX);
		m->m_off = (int)cp - (int)m - sizeof(struct ifnet *);
	} else {
		bcopy((caddr_t)cp, mtod(m, caddr_t) + sizeof(struct ifnet *),
			len);
	}
	m->m_len = len + sizeof(struct ifnet *);
	*mtod(m, struct ifnet **) = &sc->sc_if;

#else

#ifdef ultrix
	if (len >= MLEN) {
		struct mbuf *p;
		MCLGET(m,p);
		if (m->m_len != MCLBYTES) {
			/* we couldn't get a cluster - if memory's this
			 * low, it's time to start dropping packets. */
			m_freem(m);
			return (NULL);
		}
		m->m_clptr = (caddr_t)(sc->sc_ep - (SLMAX + BUFOFFSET));
		sc->sc_ep = mtod(m, u_char *) + (BUFOFFSET + SLMAX);
		m->m_off = (int)cp;
	} else {
		bcopy((caddr_t)cp, mtod(m, caddr_t), len);
	}
	m->m_len = len;

#ifdef ULTRIX_3
	m->ifnetptr= (char *) &sc->sc_if;
#else
	m->m_ifp = &sc->sc_if;
#endif ULTRIX_3

#else
	if (len >= MLEN) {
		MCLGET(m);
		if (m->m_len != MCLBYTES) {
			/* we couldn't get a cluster - if memory's this
			 * low, it's time to start dropping packets. */
			m_freem(m);
			return (NULL);
		}
		sc->sc_ep = mtod(m, u_char *) + (BUFOFFSET + SLMAX);
		m->m_off = (int)cp - (int)m;
	} else {
		bcopy((caddr_t)cp, mtod(m, caddr_t), len);
	}
	m->m_len = len;
#endif
#endif
	return (m);
}

/*
 * tty interface receiver interrupt.
 */
slinput(c, tp)
	register int c;
	register struct tty *tp;
{
	register struct sl_softc *sc;
	register struct mbuf *m;
	register int len;
	int s;

#if defined(ultrix) && defined(ULTRIX_4)
		  struct ifqueue *inq;
#endif

	tk_nin++;
	sc = (struct sl_softc *)tp->t_sc;
	if (sc == NULL)
		return;

	++sc->sc_bytesrcvd;
	c &= 0xff;
	switch (c) {

	case TRANS_FRAME_ESCAPE:
		if (sc->sc_escape)
			c = FRAME_ESCAPE;
		break;

	case TRANS_FRAME_END:
		if (sc->sc_escape)
			c = FRAME_END;
		break;

	case FRAME_ESCAPE:
		sc->sc_escape = 1;
		return;

	case FRAME_END:
		len = sc->sc_mp - sc->sc_buf;
		if (len < 3)
			/* less than min length packet - ignore */
			goto newpack;

#if defined(SL_NIT) && defined(NIT)
		if (sl_donit && nitcb.ncb_next != &nitcb) {
			/* do nit processing if there's anyone listening */
			static struct ether_header iheader = { { 2 }, { 1 } };
			static struct mbuf mb;
			struct nit_ii nii;

			m = &mb;
			m->m_len = len;
			m->m_off = (int)sc->sc_buf - (int)m;
			iheader.ether_type = *sc->sc_buf;
			nii.nii_header = (caddr_t)&iheader;
			nii.nii_hdrlen = sizeof(iheader);
			nii.nii_type = iheader.ether_type;
			nii.nii_datalen = len;
			nii.nii_promisc = 0;
			nit_tap(&sc->sc_if, m, &nii);
		}
#endif
		if ((c = (*sc->sc_buf & 0xf0)) != (IPVERSION << 4)) {
			if (c & 0x80)
				c = TYPE_COMPRESSED_TCP;
			else if (c == TYPE_UNCOMPRESSED_TCP)
				*sc->sc_buf &= 0x4f;
			/*
			 * we've got something that's not an IP packet.
			 * If compression is enabled, try to uncompress it.
			 * Otherwise, if `auto-enable' compression is on and
			 * it's a reasonable packet, uncompress it then
			 * enable compression.  Otherwise, drop it.
			 */
			if (sc->sc_if.if_flags & IFF_D1) {
				len = sl_uncompress_tcp(&sc->sc_buf, len,
							(u_int)c, &sc->sc_comp);
				if (len <= 0)
					goto error;
			} else if ((sc->sc_if.if_flags & IFF_D2) &&
				   c == TYPE_UNCOMPRESSED_TCP && len >= 40) {
				len = sl_uncompress_tcp(&sc->sc_buf, len,
							(u_int)c, &sc->sc_comp);
				if (len <= 0)
					goto error;
				sc->sc_if.if_flags |= IFF_D1;
			} else
				goto error;
		}
		m = sl_btom(sc, len);
		if (m == NULL)
			goto error;

		sc->sc_if.if_ipackets++;
		s = splimp();

#if defined(ultrix) && defined(ULTRIX_4)
		  inq = &ipintrq;
		  smp_lock(&inq->lk_ifqueue, LK_RETRY);
#endif

		if (IF_QFULL(&ipintrq)) {
			IF_DROP(&ipintrq);
			sc->sc_if.if_ierrors++;
			m_freem(m);
		} else {
			IF_ENQUEUE(&ipintrq, m);
			schednetisr(NETISR_IP);
		}

#if defined(ultrix) && defined(ULTRIX_4)
		  smp_unlock(&inq->lk_ifqueue);
#endif

		  splx(s);
		  goto newpack;
	}
	if (sc->sc_mp < sc->sc_ep) {
		*sc->sc_mp++ = c;
		sc->sc_escape = 0;
		return;
	}
error:
	sc->sc_if.if_ierrors++;
newpack:
	sc->sc_mp = sc->sc_buf = sc->sc_ep - SLMAX;
	sc->sc_escape = 0;
}

/*
 * Process an ioctl request.
 */
slioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int s = splimp(), error = 0;
#ifndef sun
	register struct ifaddr *ifa = (struct ifaddr *)data;

	switch (cmd) {

	case SIOCSIFADDR:
		if (ifa->ifa_addr.sa_family == AF_INET)
			ifp->if_flags |= IFF_UP;
		else
			error = EAFNOSUPPORT;
		break;

	case SIOCSIFDSTADDR:
		if (ifa->ifa_addr.sa_family != AF_INET)
			error = EAFNOSUPPORT;
		break;

	default:
		error = EINVAL;
	}
#else
	switch (cmd) {
	case SIOCSIFADDR:
		if_rtdelete(ifp);
		ifp->if_addr = *(struct sockaddr *)data;
		ifp->if_net = in_netof(((struct sockaddr_in *)data)->sin_addr);
		ifp->if_flags |= IFF_UP|IFF_RUNNING;
		ifp->if_flags &=~ IFF_BROADCAST;
		/* set up routing table entry */
		error = rtinit(&ifp->if_dstaddr, &ifp->if_addr,
				RTF_HOST|RTF_UP);
		ifp->if_flags |= IFF_ROUTE;
		break;

	case SIOCSIFDSTADDR:
		/* all the real work is done for us in ../net/if.c */
		break;

	default:
		error = EINVAL;
	}
#endif
	splx(s);
	return (error);
}
#endif
